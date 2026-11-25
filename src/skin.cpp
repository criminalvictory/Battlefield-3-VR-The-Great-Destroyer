// Skinning research. A skinned mesh such as the first-person arms is drawn with a block of bone
// matrices in a constant buffer; the vertex shader bends the mesh with them. BF3's gameplay
// skeleton (finder.cpp, F12) turned out not to be what's drawn; these uploads are.
//
// Found in game: BF3 draws every skinned mesh with general-purpose 60-bone blocks, each mesh cut
// into chunks of up to 60 bones and drawn in several passes. Lifting blocks by their shape alone
// moved the gun but flickered, because only some chunks and passes matched. So blocks are picked
// by where they are instead: the player's own meshes are the only skinned ones at the camera.
// Each bone matrix is the bone's world transform times its inverse bind pose, whose translation
// lands near the mesh's origin, which for the player is within a couple of metres of the camera.
//
// Second round: counting blocks near the world origin as well caught small 4x4 blocks of other
// objects, and lifting uploads made with WRITE_NO_OVERWRITE re-lifted data the GPU was already
// drawing from. Both tore meshes into pieces. Third round: with only 3x4 rows near the camera in
// the world, nothing was lifted at all; the nearest seemed 2 m away. Fourth round, lifting rows
// near the origin instead (a camera-relative view model), found only a few there, while the arm
// and gun chunks (54-59 and 7-17 bones) sat 0.3-0.7 m from the camera in the world after all.
// The third round's camera position had come from the render camera, which moves in memory and
// can be stale; the weapon camera (fixed in bf3.exe) now gives it. Fifth round, 1.5 m from it:
// still nothing visible, and while standing still almost no rows were that close. The arms rig's
// origin is at the feet, so a bone matrix's translation can land a metre or two away from the
// camera depending on how the bone is turned. Insert now cycles the radius live (1.5, 3, 6 m)
// and the log counts uploads within each.
// Sixth round, radii 1.5/3/6 m: nothing within 3 m at all; at 6 m the nearest NPC (3.5 m away)
// jumped. So the player's arms aren't world-space blocks near the camera. The gun is most likely
// rigid camera-relative pieces: the small 4x4 blocks ~7 cm from the origin, which the first round
// lifted (and those rounds since excluded).
// Seventh round, run by a script (tools/lifttest.ps1): lifting those camera-relative blocks moved
// nothing either. The one round that moved the gun had also lifted appended data in 64 KB ring
// buffers. Now ring buffers get shadow copies, so the exact bytes appended can be found and only
// those are inspected and changed (the rest may be in use by the GPU).
// Lifted: camera-relative rows in any upload, and appended rows within 3 m of the camera.
//
// Draw hunt (2026-10-29, the user judging live in the headset): the gloves are one draw, index
// count 13755, start index 15051, drawn with 55 bones (3x4) from a fresh 4096-byte upload. The
// bones' translations sit ~1.9 m from the origin: they're in the character's own space, not the
// world, which is why every world-position search missed them. Forearm skin, sleeves and gun are
// draws of their own. Hand shift (Insert) draws that one draw with a changed copy of its bones,
// bound in place of BF3's buffer, so nothing BF3 uploaded is touched.

#include "skin.h"
#include "config.h"

#include <windows.h>
#include <d3d11.h>
#include <MinHook.h>
#include <dxgi.h>
#include <share.h>

#include <algorithm>
#include <cctype>
#include <atomic>
#include <cmath>
#include <map>
#include <cstring>
#include <functional>
#include <iterator>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "aim.h"
#include "aimrules.h"
#include "ammo.h"
#include "at4shot.h"
#include "barehold.h"
#include "barestate.h"
#include "footsteps.h"
#include "camera.h"
#include "effects.h"
#include "ground.h"
#include "gunbodies.h"
#include "gunprofile.h"
#include "handleruns.h"
#include "hands.h"
#include "heldpalms.h"
#include "loose.h"
#include "m320.h"
#include "m249rows.h"
#include "lidride.h"
#include "headmove.h"
#include "input.h"
#include "launchreport.h"
#include "log.h"
#include "overlay.h"
#include "reload.h"
#include "magposes.h"
#include "scopezoom.h"
#include "seatrest.h"
#include "xr_runtime.h"
#include "shaders.h"
#include "shapemake.h"
#include "sounds.h"
#include "streamview.h"
#include "pad.h"
#include "postpin.h"
#include "scopeview.h"
#include "views.h"
#include "viewturn.h"
#include "window.h"

namespace {

using MapFn = HRESULT(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, UINT, D3D11_MAP, UINT,
                                          D3D11_MAPPED_SUBRESOURCE*);
using UnmapFn = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, UINT);
using DrawIndexedFn = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, UINT, UINT, INT);

MapFn g_origMap[2] = {};
UnmapFn g_origUnmap[2] = {};
DrawIndexedFn g_origDrawIndexed[2] = {};

constexpr UINT kMinMatrices = 4;
constexpr float kLift = 0.15f;
constexpr float kViewModel = 1.0f;  // metres from the origin: camera-relative pieces
constexpr float kBucket = 0.5f;     // metres: grid for the where-relative-to-the-eyes histogram

// Layout 0: 3x4, three float4 rows with the translation in .w (48 bytes).
// Layout 1: 4x4 with the translation in the fourth row (64 bytes).
struct Signature {
    UINT size;
    UINT start;   // byte offset of the first matrix
    UINT count;
    UINT layout;
    bool operator<(const Signature& o) const {
        return std::tie(size, start, count, layout) < std::tie(o.size, o.start, o.count, o.layout);
    }
};

struct Seen {
    UINT uploads = 0;
    UINT viewUploads = 0;   // mean translation within kViewModel of the origin (camera-relative)
    UINT appended = 0;      // ... of those, uploads made with WRITE_NO_OVERWRITE (never touched)
    float nearestCamera = 1e9f;
    float nearestOrigin = 1e9f;
};

// Where blocks sit relative to the eyes, on a kBucket grid in the camera's own axes. Over a few
// seconds of walking, other characters' blocks drift between cells; the player's own arms, if
// drawn in the world, stay in the same few cells right at the eyes.
struct Cell {
    int right, up, forward;
    UINT layout;
    bool appended;
    bool operator<(const Cell& o) const {
        return std::tie(right, up, forward, layout, appended) <
               std::tie(o.right, o.up, o.forward, o.layout, o.appended);
    }
};

std::mutex g_mutex;
std::map<Signature, Seen> g_seen;
std::map<Cell, UINT> g_cells;
std::atomic<bool> g_lift{false};
// Research hooks: bone searches in every upload, ring-buffer comparisons, draw counting for the
// hunt. They ran on every graphics call and cost frame rate (the game went from about 87 to 45
// fps while they grew), so they run only while research is on (Pause, or F15 / Insert research).
std::atomic<bool> g_research{false};
// Camera position and axes (right, up, true forward), written on the render thread.
float g_camera[3] = {};
float g_axes[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

// ---- Draw hunt ----
// A draw is "skinned" if a block of bone matrices was uploaded on the same thread just before it.
// A mesh's draws keep the same index count, start index and base vertex every frame, so those
// identify it. The hunt hides half the candidates at a time until the arms disappear.

struct DrawKey {
    UINT indexCount;
    UINT startIndex;
    INT baseVertex;
    bool operator==(const DrawKey& o) const {
        return indexCount == o.indexCount && startIndex == o.startIndex && baseVertex == o.baseVertex;
    }
};
struct DrawKeyHash {
    size_t operator()(const DrawKey& k) const {
        return (static_cast<size_t>(k.indexCount) * 73856093u) ^ (static_cast<size_t>(k.startIndex) * 19349663u) ^
               static_cast<size_t>(k.baseVertex);
    }
};
using DrawSet = std::unordered_set<DrawKey, DrawKeyHash>;

// The bone block uploaded most recently on this thread, for whichever draw follows it.
struct BoneInfo {
    UINT count = 0, layout = 0, size = 0;
    UINT start = 0;        // byte offset of the first matrix
    bool fresh = false;
    float first[12] = {};  // the first matrix's three rows
    float mean[3] = {};    // mean translation
    float dCamera = 0.0f, dOrigin = 0.0f;
};
thread_local BoneInfo t_bones;
thread_local int t_drawsSinceBones = 1000;
constexpr int kSkinnedWithin = 3;  // draws after a bone upload that still count as using it

std::mutex g_drawMutex;
std::unordered_map<DrawKey, UINT, DrawKeyHash> g_skinned;  // skinned draws seen, with counts
std::vector<DrawKey> g_huntRange;                          // candidates still in the running
bool g_haveFinal = false;
DrawKey g_final{};
BoneInfo g_finalBones;
bool g_finalSeen = false;
// Draws to hide. Published whole and never freed (a handful per hunt), so draw threads can read
// it without a lock.
std::atomic<const DrawSet*> g_hidden{nullptr};
// First-person draws: BF3 marks the gun and arms with stencil 32, set as the stencil reference of
// the draws that make them. The gun's attachments (scope, magazine, foregrip) are rigid meshes, not
// skinned, so a hunt for one of them ("hunt.txt" saying "first person") picks from these instead.
constexpr UINT kFirstPersonStencil = 32;
std::unordered_map<DrawKey, UINT, DrawKeyHash> g_firstPerson;  // seen while research is on
std::atomic<bool> g_huntFirstPerson{false};
int g_huntOnly = -1;  // hunt.txt "only N"

// ---- Hand shift ----
constexpr DrawKey kGloves{13755, 15051, 0};  // from the hunt
// Shifting the gloves left a black hand shape behind: another pass, a draw of its own with its
// own 60 bones (a second hunt found it). Matching its bones to the gloves' didn't work.
struct HandDraw {
    DrawKey key;
    UINT bones;
};
constexpr HandDraw kHandDraws[] = {{kGloves, 55}, {{7617, 49995, 0}, 60}};
constexpr UINT kGloveUploadSize = 4096;
constexpr UINT kGunPrefix = 6 * 48;  // bytes of a bone upload the gun's draws use (bones 0-5)
constexpr float kHandShift = 0.20f;  // metres up
std::atomic<bool> g_handShift{false};

// The gun's own colour pass (the draw hunt found it: hidden, the gun turned black), 59 bones in a
// 4096-byte upload like the gloves'. Its bones carry the parts its animations move, the magazine
// among them. weapon.txt next to the DLL shifts chosen ones ("0-29 shift 0 -0.3 0"), to find them.
// Other soldiers' copies of the same gun are drawn with the same draw, their bones far out in the
// world; only the one whose bones sit near the origin (the player's, in the body's own space) moves.
constexpr DrawKey kWeapon{12768, 0, 0};
// With only this colour pass shifted, a black copy of the gun stayed behind: the gun is drawn in
// more passes, each cutting the mesh its own way. The colour pass's 12768 + 312 indices make the
// 13080 of another pass that draws it whole. Each draw has a bone palette of its own, so weapon.txt
// names the bones per pass. (Shifting 9273/12 and 9288/9288 along, other meshes, left more black.)
constexpr DrawKey kWeaponPasses[] = {kWeapon, {312, 12768, 0}, {13080, 13080, 0}};
constexpr size_t kWeaponPassCount = std::size(kWeaponPasses);
constexpr UINT kPaletteBones = 60;  // BF3's bone blocks hold up to 60
constexpr float kFirstPersonReach = 5.0f;  // metres from the origin
std::atomic<bool> g_weaponShift{false};  // weapon.txt names bones
struct PassSelection {
    bool bones[64] = {};
    bool any = false;
};
PassSelection g_weaponSelected[kWeaponPassCount];
float g_weaponOffset[3] = {};
bool g_weaponHide = false;  // weapon.txt "hide": the named bones vanish instead of moving
std::atomic<float> g_floor{0.0f};  // weapon.txt "floor": the ground's height in the body's space
std::atomic<bool> g_handsActive{false};  // free hands on (hands.cpp); read by the hooks without a lock
void NoteRecentUpload(ID3D11Resource* resource, const uint8_t* data, UINT size);
std::atomic<UINT> g_weaponShifted{0}, g_weaponFar{0};

// The player's own gun: bone 0 sits in the body's space, other soldiers' copies of it far out in
// the world.
bool PlayersOwn(const std::vector<uint8_t>& bones) {
    if (bones.size() < 48)
        return false;
    const auto* first = reinterpret_cast<const float*>(bones.data());
    return std::fabs(first[3]) < kFirstPersonReach && std::fabs(first[7]) < kFirstPersonReach &&
           std::fabs(first[11]) < kFirstPersonReach;
}

// weapon.txt "watch": how far each palette bone of each of the gun's passes moves against a
// reference bone (the colour pass's receiver, bone 1; bone 0 in the others) over each second. The
// gun's sway moves every bone alike; a reload moves the magazine some 20 cm, in every pass, which
// shows its bone in each palette.
std::atomic<bool> g_weaponWatch{false};
std::atomic<bool> g_weaponTesting{false};  // weapon.txt "wcam": logs what follows the moved camera
std::atomic<int> g_wiggleBone{-1};  // weapon.txt "wiggle N": that gun bone slides to and fro (DrawGun)
constexpr UINT kReceiverBone = 1;
struct WatchPass {
    UINT draws = 0;
    bool any = false;
    float lo[kPaletteBones][3] = {}, hi[kPaletteBones][3] = {};
};
std::mutex g_watchMutex;
WatchPass g_watch[kWeaponPassCount];
// Also with "watch": the matrices found with the game's FOV (the gun and arms' projection, which
// RescaleProjections widens), as found, one of each kind per second. If they carry the view as
// well, they say where the eyes are in the arms' space: what the hands need.
struct SeenProjection {
    float m[16];
    UINT size, offset;
    bool columns;
};
std::mutex g_projMutex;
std::vector<SeenProjection> g_seenProjections;

// The first-person view-projections (see skin::FirstPersonViews): in a 352-byte upload at +176 in
// the arms' own space, in an 80-byte upload at +0 in the world (found with "watch", 2026-10-30).
std::mutex g_fpMutex;
float g_fpArms[16] = {}, g_fpWorld[16] = {};
ULONGLONG g_fpArmsTime = 0, g_fpWorldTime = 0;

// The eye: where a view-projection's clip x, y and w rows are all zero.
bool SolveEye(const float* m, float* eye) {
    const float* r[3] = {m, m + 4, m + 12};
    auto det3 = [](const float a[3][3]) {
        return a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1]) - a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0]) +
               a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]);
    };
    float a[3][3], b[3];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            a[i][j] = r[i][j];
        b[i] = -r[i][3];
    }
    float det = det3(a);
    if (!(std::fabs(det) > 1e-9f))
        return false;
    for (int k = 0; k < 3; ++k) {
        float c[3][3];
        std::memcpy(c, a, sizeof(c));
        for (int i = 0; i < 3; ++i)
            c[i][k] = b[i];
        eye[k] = det3(c) / det;
    }
    return true;
}

// The view's own space (the eye at the origin, x right, y up, z back) -> the arms' space, from the
// latest first-person view-projection: its axes and eye. The hands' targets come in the view's
// space and each draw places them with this, so they stay put relative to the picture.
float g_viewToArms[12] = {};
bool g_haveViewToArms = false;
// ... and per constant buffer the view-projection went into: a draw uses the one bound to it, the
// view it is really drawn with. The latest upload, from another thread, was sometimes the next
// frame's: the gun jittered while walking by a frame's worth of movement (user, 2026-10-30).
struct ViewByBuffer {
    ID3D11Resource* buffer = nullptr;
    float viewToArms[12] = {};
    float vp[16] = {};  // its view-projection of the arms' space
    int eye = -1;  // per-eye views: which eye's view this is (0 left, 1 right; -1 not known)
};
// Per-eye views: which eye each first-person view upload is for (eight a frame, all into one
// buffer, on one thread; their draws use one viewport the width of the whole picture). views.cpp
// shifts each eye's first-person camera half the eye distance along its right, so a frame's uploads
// sit in two places along that right, the left eye's further left. Their order in a frame stays
// the same from frame to frame, so the last frame's, read that way, gives each upload its eye as
// it comes. (Taking the first half for the left eye, as before, labelled some of the left eye's
// passes the right's if the passes take turns between the eyes: the right eye got ~6 more hand
// draws a frame, and the user saw a black layer around the arms and gun, 2026-10-30.)
std::atomic<uint32_t> g_frameSerial{0};  // frames shown so far (skin::Update)
uint32_t g_armsFrame = 0, g_armsUploads = 0, g_armsUploadsBefore = 0;  // under g_fpMutex
float g_frameFirstEye[3] = {}, g_frameRight[3] = {};  // this frame's first upload's eye and right
std::vector<float> g_frameSides;  // each upload's eye along that right, from the first's
std::vector<int8_t> g_eyeOrder;   // the eye of each upload, by its place in the frame
ViewByBuffer g_viewByBuffer[8];
int g_viewByBufferNext = 0;
// The constant buffers the first-person view-projection in the world went into (effects.cpp's
// census tells draws by them).
ID3D11Resource* g_worldViewBuffers[4] = {};
int g_worldViewNext = 0;
thread_local ID3D11Resource* t_uploadResource = nullptr;  // the upload being looked at
std::atomic<uint32_t> g_viewBound{0}, g_viewLatest{0}, g_viewDiffered{0};
// weapon.txt "trace": the next 120 gun colour draws, each with the eye the hands were placed from
// and the game's own gun body as drawn: while walking, a view a frame off shows as the gap between
// the two jumping by a frame's movement.
struct TraceRow {
    double time;
    DWORD thread;
    float eye[3], gun[3];
    uint32_t viewUploads;
};
std::mutex g_traceMutex;
std::vector<TraceRow> g_trace;
std::atomic<bool> g_tracing{false};
std::atomic<uint32_t> g_viewUploads{0};  // first-person view-projections uploaded so far
// The arms' space's origin in the world: whole metres (it jumps one as the player walks across).
// Only a near whole-metre difference between the two latest views counts: one of them a frame
// older while moving fast gives something in between, which is left out.
float g_fpOrigin[3] = {};
bool g_haveFpOrigin = false;
// The main cameras of the latest view builds, under g_fpMutex: the head's space -> the world (right, up, back | the middle of
// the eyes) and the game's eye each was made from. A draw's first-person view has that eye: drawing runs builds behind, and
// the newest while walking was a frame's walk off the picture being drawn.
struct MainCamera {
    ULONGLONG at = 0;
    float gameEye[3] = {};
    float toWorld[12] = {};
};
constexpr int kMainCameras = 4;
constexpr ULONGLONG kMainCameraFresh = 250;  // ms: older ones are from before a load or a cutscene
constexpr float kMainCameraOff = 0.2f;       // m: a first-person view this far from whole metres off a build's eye isn't of it
MainCamera g_mainCameras[kMainCameras];
int g_mainCameraNext = 0;

// weapon.txt "dumpview": the next 352-byte upload with the first-person view in it is logged
// whole, to find the rest of what BF3 keeps per view there (the main view-projection).
std::atomic<bool> g_dumpView{false};
// weapon.txt "eyetrace": the next 40 first-person view uploads are logged with the frame they fell
// in, their place in it, the eye given and the thread (the right eye got ~6 more hand draws a frame
// than the left, 2026-10-30: one pass of the left eye's taken for the right's?).
std::atomic<int> g_eyeTrace{0};
// weapon.txt "bodylock 0": in a scripted picture turned to the head, the views the player's own body is drawn with
// left as the game made them, moving with the head; on by default (LockBody).
std::atomic<bool> g_bodyLock{true};
// weapon.txt "bodytrace 120": in a scripted picture, the next 120 views the body lock reads logged in order, each with
// what it made of it, its eye, and where that is along the turned camera's right (which eye's each is).
std::atomic<int> g_bodyTrace{0};
// weapon.txt "bodystereo 0": the body of a scripted picture left in the middle with both eyes drawing it alike, as the
// game draws it, not moved to each eye's place (LockBody), its own depth. The first try showed black holes round the
// legs, arms and gun in the right eye (2026-11-01): not what the right eye's pass keeps from the left's (the tail showed
// both depth-stencil buffers cleared in each pass), but the eyes' labels learned from the frame before, wrong in a frame
// whose body views came in another number; each eye's picture now tells them in the frame itself (NoteEyePicture).
std::atomic<bool> g_bodyStereo{true};
// weapon.txt "fpunknown": the next three frames' first-person draws (drawn with a first-person
// view's constants bound) that the mod leaves where the game put them, each kind logged once: the
// ACOG's lens tint and reticle stayed at the game's own gun, floating in the sky (user, 2026-10-30).
std::atomic<int> g_fpProbeFrames{0};
std::atomic<int> g_drawScan{0};  // weapon.txt "drawscan", "allrec": ScanOwnDraw
std::atomic<int> g_armTrace{0};  // weapon.txt "armtrace N": for that many frames, a line for each of the arms' draws: where it was put
std::atomic<bool> g_gunPartLog{false};  // weapon.txt "gunpartlog": twice a second, each part of the rifle against its body, as the game has them
// weapon.txt "partshift 7848 1 x y z": that part's bone (draw of that many indices) so far from where the game has it, before anything else looks at it: a pose of the game's on foot, to see which part is which. "partpin 0": seated, the parts left where the game has them.
struct PartShift {
    uint32_t count;
    int bone;
    float by[3];
};
PartShift g_partShift[6] = {};
std::atomic<int> g_partShifts{0};
std::atomic<bool> g_partPin{true};
std::atomic<bool> g_drawScanRec{false};
std::atomic<bool> g_rifleSpread{true};  // weapon.txt "riflespread 0": the M416's scatter as the game has it (ClearRifleSpread)
void ApplyRifleSpread(bool on);
std::atomic<bool> g_launcherHands{true};  // weapon.txt "launcherhands 0": LauncherHands
std::atomic<bool> g_launcherSteer{true};  // "launchersteer 0": LauncherSteer
std::atomic<int> g_rocketTest{0};         // "rockettest 1": RocketTest
std::atomic<bool> g_rocketGame{false};    // "rocketgame": TakeRocketGameTest
std::atomic<bool> g_launcherOn{true};  // weapon.txt "launcher 0": as the game places it (DrawLauncher)
std::atomic<float> g_launcherUp{0.7f};  // weapon.txt "launcherup": LauncherUp
std::atomic<float> g_rifleUp{0.2865f};  // weapon.txt "rifleup": RifleUp
std::atomic<bool> g_rifleShotOn{true};  // weapon.txt "rifleshot 0": its bullets left to start from the game's camera
std::atomic<uintptr_t> g_rifleShot[32] = {};  // its ShotConfigDatas' InitialPosition (ClearRifleSpread)
float g_rifleShotLast[std::size(g_rifleShot)][3] = {};  // what was last written at each of those. Under g_handsMutex
std::atomic<float> g_rifleShotBy[3] = {};  // weapon.txt "rifleshotby 2 0 0": its bullets' start so far off (left, up, forward), to see that it counts
std::atomic<bool> g_reticleFar{true};  // weapon.txt "reticlefar 0": the scope's reticle where its model has it, not as far off as a real one
std::atomic<bool> g_launcherZoom{false};  // "launcherzoom 1": the game's zoom held with it
std::atomic<bool> g_launcherSpawn{true};  // "launcherspawn 0": its rocket left to start from the game's camera
std::atomic<uintptr_t> g_launcherShot[16] = {};  // its ShotConfigDatas' InitialPosition (ClearLauncherSpread): the SMAW's own firing data's only
std::atomic<bool> g_launcherLog{false};  // "launcherlog": the view on its frame, each second
std::atomic<bool> g_rifleLog{false};  // "riflelog": the view on the rifle's body, each second; with "zoom", the sights' line
std::atomic<bool> g_armBonesLog{false};  // "armbones": each second, where each bone of the arms' draws is put: the forearm's middle through it
std::atomic<int> g_armNudge{-1};  // "armnudge 3": that bone of the bare forearms' draw lifted 30 cm: which vertices are on it
std::atomic<int> g_armNudgeCount{0};  // "armnudge 3 13755": of the draw with that many indices instead
std::atomic<bool> g_armDumpWanted{false};  // "armdump": a frame's first palette of each arm draw written out (build\armdump_N.bin): what every vertex is drawn with
float g_launcherShotLast[std::size(g_launcherShot)][3] = {};  // what was last written at each of those. Under g_handsMutex
// Each one's firing data's numbers as the scan took it for the SMAW's own (at4shot::SmawOwn: by its slot whatever they are, or Venice-EBX's): what
// its writer and its putting back hold it to each time. Under g_handsMutex, set with its place (AddLauncherShots).
at4shot::Launcher g_launcherShotWho[std::size(g_launcherShot)] = {};
std::atomic<bool> g_launcherScanWanted{false};  // those aren't its data any more (a load moved it): look again
std::atomic<ULONGLONG> g_launcherAt{0};
std::atomic<uint32_t> g_launcherFrame{0};  // the frame (g_frameSerial) it was last drawn in
// Any other gun in hand (DrawHeldPart): when its body was last drawn, which one (its body's index count), weapon.txt "heldguns 0" as the game places it, "heldlog 1" its numbers.
std::atomic<ULONGLONG> g_heldAt{0};
std::atomic<uint32_t> g_heldFrame{0};
std::atomic<uint32_t> g_heldKind{0};
std::atomic<uint32_t> g_heldAtKind{0};  // the gun whose draw stamped g_heldAt: till the rig has taken it (g_heldKind), it's a switch's moment (skin::HeldNow)
std::atomic<bool> g_heldOn{true};
std::atomic<bool> g_heldLog{false};
// weapon.txt "heldshot 0": a held gun's bullets from its own sights' line whatever sight is on it, and its data looked for again only for another kind
// of gun, as before (on: on the line through its sight's glass, aimrules::HeldShotLine; its data looked for again after a take of the same kind too).
std::atomic<bool> g_heldShotOn{true};
// weapon.txt "handleruns": a held gun's handle with no bone of its own (handleruns.h: the Mk11's T handle) drawn with the hand as it's pulled, 2 with the
// shaft out of the receiver behind it (the default, with no line), 1 without the shaft, 0 not at all: the handle stays home as the game draws it, as before.
std::atomic<int> g_handleRuns{2};
std::atomic<int> g_heldTest{0};  // weapon.txt "heldtest": skin::HeldTest
std::atomic<bool> g_gunNearCut{false};  // weapon.txt "gunnearcut 1": what's in the hands cut off 6 cm in front of the eye again, as the game has it
std::atomic<float> g_scopePower{3.44f};  // the ACOG: the game's own zoom, FOV 17.2 against the unzoomed 55  // weapon.txt "scopepower 0": the rifle's scope not magnifying, as the game draws it; another number, that power
// The ACOG's eyepiece in its own model, and its radius (weapon.txt "scopelens x y z r").
// (Placed by eye against the scope's rear ring, from both eyes, 2026-11-05: at 0.065 it floated in front of the scope, nearer the stock: the user saw it. That's past the end of the mesh's own numbers.)
float g_acogEyepiece[3] = {0.0f, 0.032f, -0.067f};  // its glass disc, in its own model
float g_acogEyepieceRadius = 0.011f;
// The frame the ACOG was last placed on the rifle in a hand (DrawArms), for recoil's sight at the eye (skin::Sight). Under g_handsMutex.
uint32_t g_acogFrame = 0;
bool g_haveAcogFrame = false;
std::atomic<int> g_glassDump{0};            // weapon.txt "glassdump N": NoteGlass
std::atomic<bool> g_scopeGlassOver{false};  // weapon.txt "scopeglass 1": the game's own glass drawn as well, over what's seen through it
// Any of the enemies' sights' lens moved from its table's place (weapon.txt "sightlens n x y z r"), to place it by eye.
float g_sightLensOff[8][4] = {};
// In hand: drawn within the last so many frames. Frames, not milliseconds: a stall of the game or the headset (a hitch, a picture saved) isn't the gun going away. By the
// clock a rocket in the hand was lost with it and the arms were left at the game's place for a frame (the review, 2026-11-04).
constexpr uint32_t kGoneFrames = 30;
bool DrawnLately(const std::atomic<ULONGLONG>& at, const std::atomic<uint32_t>& frame) {
    return at.load(std::memory_order_relaxed) && g_frameSerial.load(std::memory_order_relaxed) - frame.load(std::memory_order_relaxed) <= kGoneFrames;
}
// weapon.txt "pslog <checksum>": the next three frames' draws with that pixel shader (LogByShader).
std::atomic<int> g_psLogFrames{0};
std::atomic<uint64_t> g_psLogSum{0};
std::atomic<ULONGLONG> g_bareMovedAt{0};  // when the bare arms were last drawn moved (the body hidden meanwhile)
std::atomic<ULONGLONG> g_bareSeenAt{0};   // when the player's own bare arms were last drawn at all, moved or not: a level without gloves (a held gun's magazine's seat)
// weapon.txt "barehands 0" leaves the game's own bare arms; "barehands reset" learns their offsets again
// from the controllers (DrawBareArms).
std::atomic<bool> g_bareOn{true};
std::atomic<bool> g_bareRelearn{false};
std::atomic<bool> g_bareRelearnPistol{false};  // weapon.txt "barehands pistol": its hold learned again
std::atomic<bool> g_bareWrist{true};   // weapon.txt "barewrist 0": the bare forearms whole with their hands
// The bare arms with a gun in hand (Semper Fidelis's train after the grapple, 2026-11-18: the right fist under the receiver, the left flat and open, the left forearm a
// torn flap). weapon.txt "barehold 0": as before (each palm at its controller's line-up, the game's own fingers). On: the bare hands go through the gloves' own rig
// (barehold.h; DrawBareArms): the game's hold on the gun, the fitted hands. "bareelbowreach M": how far from the arm's own elbow (the triangle's) a bare forearm's
// elbow end may go when the wrist's range turns it (armik::Params::elbowReach; 0 none, as before). "barecap 0 / 1 / 2": the open elbow end's cap never, only with
// the hand beyond the arm's reach (armik::EndCap; never for a hand in reach), always.
std::atomic<bool> g_bareHold{true};
std::atomic<float> g_bareElbowReach{0.05f};
std::atomic<int> g_bareCap{1};
// A magazine's own draw (HeldMagBones::partCount: the M249's cloth pouch, the AK-74M's piece, the Mk11's plate and sleeve 1902/0), the in-place one and its
// copies off the gun (in a hand, lying, falling). In-game 2026-11-18 the M249's pouch was drawn nearly black off the gun in both eyes, lit seated, while the
// M240's box (a bone of its body) and every loose magazine of a body's own bones kept their light; offline its copies were found the same draw's as the lit
// one, all its vertices on its bone 0, its palette's other entries other draws' leftovers. Its shaders, named by the run's own survey and in build\ps since
// 2026-11-02 (docs/research/m249.md): the vertex shader reads its one bone and the view's constants, nothing else of the palette; the pixel shader, a
// G-buffer one, reads no bones, its own per-draw light probe and material constants. What's left between the lit draw and its copies: where the game's
// palette is bound besides the one vertex shader slot the mod's own goes into (BoundSlot's last), or the light those pixels get after the shaders. Its first
// draw says once its shaders and every vertex and pixel shader slot holding its palette. weapon.txt "partcopy 1", a test (0, as before, by default): the
// mod's palette for such a draw in every one of those slots, the copies' and the in-place's.
std::atomic<int> g_partCopy{0};
// Blackburn's arms whole (the user, 2026-11-18: the camo sleeves, shoulders and upper body as the levels with gloves have them, not forearms ending past the elbow).
// weapon.txt "baresleeves 0": the forearms alone and the body hidden, as before. "barestrip 0": the strip over the shoulders (222 indices) hidden as before, the
// torso and sleeves still drawn (posed for the first time here: unposed it was two white sheets in the corners of the view).
std::atomic<bool> g_bareSleeves{true}, g_bareStrip{true};
// weapon.txt "baretwist L R" (degrees). None by default: the right's -55 was tuned while each hand was
// placed by its thumb's last bone (kBarePalm), and made no difference once the pistol was held (16:34).
std::atomic<float> g_bareTwist[2] = {0.0f, 0.0f};
// weapon.txt "bareshare E A B": the share of the hand's roll each bare forearm frame takes (the elbow end with
// the stub past the elbow, twist bone A, twist bone B at the wrist). 0, 0.5, 0.95 left the forearm wrung like
// a rope with the palm up, the twist at its 150 degree clamp (the user's video 17:53; the arm study measured
// 8-13% thinning at 90-120 degrees, 2-3% with these). The user, 18:25: "a lot better ... this fixes
// everything". The elbow end not past half: it swings the stub past the elbow round.
std::atomic<float> g_bareShare[3] = {0.5f, 0.8f, 1.0f};
// The right palm from the pistol's frame as the game holds it, learned from adjacent draws in one eye.
float g_barePistolHand[12] = {};
std::atomic<bool> g_haveBarePistolHand{false};
// The left palm from it alike, the game's own two-handed M9 hold, and its fingers as the left glove's bones
// (handpose.cpp's 0-22) relative to that palm: the left hand takes them while it holds the pistol (hands.cpp).
float g_barePistolLeft[12] = {};
float g_barePistolLeftFingers[handpose::kBones][12] = {};
std::atomic<bool> g_haveBarePistolLeft{false};
ULONGLONG g_barePistolLearnedAt = 0;  // the first pair taken: a nearer one is looked for 1.5 s from it
float g_barePistolLearnedFrom = 0.0f;  // the kept pair's right palm from the grip (m)
// weapon.txt "pistolfingers 25 25 25": degrees the middle, ring and little fingers of the right hand are opened
// on the pistol's grip from the game's own, holding it in one hand (they went into it: the user, 16:50).
// "pistolfingers2 8 25 25": the same with the left hand on it too, blended as the left hand takes it. The game's
// own grip has them 13-17 mm inside (its hand is posed for a pistol 17% bigger than an M9, the clip study
// 2026-10-31: screenshots\wf_clip_1730): 8 took off only 2-3 mm, and nothing up to 25 clears them, as BendRight
// gives the knuckle a third of the bend; 25 halves the ring's and little's, which wrap round the grip. In both
// hands the middle's tip comes out through the back of the game's left hand from 12 degrees: 8 there.
std::atomic<float> g_pistolOpen[3] = {25.0f, 25.0f, 25.0f};
std::atomic<float> g_pistolOpen2[3] = {8.0f, 25.0f, 25.0f};
// weapon.txt "pistoltest 1": the stand-in left hand (nobody in the headset) holds the pistol in both hands,
// to see the two-handed hold without a squeeze (hands.cpp PistolSupport); "pistoltest 0" lets go.
std::atomic<bool> g_pistolTest{false};
// weapon.txt "reloadrec 6": for 6 s, every train pistol draw's and bare arm draw's own bones (the game's, before
// the mod moves them) go to build\reloadrec.bin with the frame, eye, pass and the ammo: to read the game's own
// pistol reload (the magazine's way out and in, the slide, the left hand's poses) for manual reloads (the user,
// 18:45: the magazine from the pelvis, a flat palm to push it home, the slide racked by hand). Each record:
// kind (0 pistol, 1 arms), frame, eye, pass (eye*4 + PS + stencil 32 * 2), ammo loaded, spare, ms since the
// start, the bone count, then that many 3x4 matrices.
std::mutex g_recMutex;
FILE* g_recFile = nullptr;
std::atomic<ULONGLONG> g_recFrom{0}, g_recUntil{0};
std::atomic<uint32_t> g_recCount{0};

void Record(uint32_t kind, int eye, int pass, const std::vector<uint8_t>& bytes, uint32_t bones) {
    const ULONGLONG now = GetTickCount64();
    if (now >= g_recUntil.load(std::memory_order_relaxed)) {
        std::lock_guard lock(g_recMutex);
        if (g_recFile) {
            fclose(g_recFile);
            g_recFile = nullptr;
            Log("Reload recording: done, %u records in build\\reloadrec.bin", g_recCount.load());
        }
        g_recUntil = 0;
        return;
    }
    if (bytes.size() < bones * 48u)
        return;
    int loaded = -1, spare = -1;
    ammo::Get(loaded, spare);
    const int32_t head[8] = {static_cast<int32_t>(kind), static_cast<int32_t>(g_frameSerial.load(std::memory_order_relaxed)),
                             eye, pass, loaded, spare, static_cast<int32_t>(now - g_recFrom.load()),
                             static_cast<int32_t>(bones)};
    std::lock_guard lock(g_recMutex);
    if (!g_recFile)
        return;
    fwrite(head, sizeof(head), 1, g_recFile);
    fwrite(bytes.data(), 48, bones, g_recFile);
    g_recCount.fetch_add(1, std::memory_order_relaxed);
}

void StartRecording(int seconds) {
    wchar_t path[MAX_PATH] = {};
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&StartRecording), &self);
    GetModuleFileNameW(self, path, MAX_PATH);
    if (wchar_t* slash = wcsrchr(path, L'\\'))
        wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path), L"reloadrec.bin");
    std::lock_guard lock(g_recMutex);
    if (g_recFile)
        fclose(g_recFile);
    g_recFile = nullptr;
    if (_wfopen_s(&g_recFile, path, L"wb") != 0 || !g_recFile) {
        g_recFile = nullptr;
        Log("Reload recording: build\\reloadrec.bin could not be opened");
        return;
    }
    g_recCount = 0;
    g_recFrom = GetTickCount64();
    g_recUntil = GetTickCount64() + static_cast<ULONGLONG>(seconds) * 1000;
    Log("Reload recording: the pistol's and the bare arms' own bones for %d s", seconds);
}

// The draw's pass the way DrawBareArms keys it: eye*4 + a pixel shader bound + stencil reference 32 * 2.
int DrawPass(ID3D11DeviceContext* ctx, int eye) {
    int view = eye >= 0 && eye <= 1 ? eye + 1 : 0;
    ID3D11PixelShader* ps = nullptr;
    ctx->PSGetShader(&ps, nullptr, nullptr);
    ID3D11DepthStencilState* state = nullptr;
    UINT ref = 0;
    ctx->OMGetDepthStencilState(&state, &ref);
    view = view * 4 + (ps ? 1 : 0) + (ref == 32 ? 2 : 0);
    if (ps)
        ps->Release();
    if (state)
        state->Release();
    return view;
}
std::atomic<ULONGLONG> g_barePistolAt{0};  // when the pistol was last drawn moved
float g_barePistolFrame[3][12] = {};  // the game's pistol frame per eye (none, left, right) and its frame
uint32_t g_barePistolFrameNo[3] = {};
ULONGLONG g_pistolSince = 0;  // when the pistol came into hand (DrawBarePistol): its holds are learned a second on
std::atomic<bool> g_bareElbow{true};   // weapon.txt "bareelbow 0": the forearms' elbow end left as drawn
std::atomic<bool> g_bareLearn{false};  // weapon.txt "barehands learn": offsets learned from the game's own arms
// weapon.txt "barehands learn 5": for 5 s the game's own arms are shown, then each hand's offset from its
// controller is learned from them: the user puts the real hands where they are first (a line-up).
std::atomic<ULONGLONG> g_bareLineUpAt{0};
// weapon.txt "scopetrace": the next three frames' draws of the scope's mesh (its index buffer, as
// the player's own scope body binds it), each with the bones it was drawn with and what became of it.
std::atomic<int> g_scopeTraceFrames{0};
std::atomic<ID3D11Buffer*> g_scopeIndices{nullptr};
// The scope's see-through parts drawn again on themselves: the reticle (12 indices, its red chevron
// texture) and the red cable on top (129) looked faint in the headset; the user chose the strongest
// of three levels tried (2026-10-31): the reticle five times more, the cable three. They're blended
// (premultiplied), so each draw adds their red again. weapon.txt "scopeboost 12 2 129 1" sets other
// pairs (up to four), "scopeboost" alone none.
constexpr size_t kBoostRules = 4;
constexpr uint32_t kDefaultBoost[kBoostRules] = {12u << 8 | 5u, 129u << 8 | 3u, 0, 0};
std::atomic<uint32_t> g_scopeBoost[kBoostRules] = {kDefaultBoost[0], kDefaultBoost[1], 0, 0};  // count << 8 | extra

int ScopeBoost(UINT indexCount) {
    for (const auto& rule : g_scopeBoost)
        if (const uint32_t r = rule.load(std::memory_order_relaxed); r && r >> 8 == indexCount)
            return static_cast<int>(r & 0xFF);
    return 0;
}
void ResetScopeMaterials();
void LogScopeMaterial(ID3D11DeviceContext* ctx, const DrawKey& key);
ID3D11Resource* g_armsViewBuffers[8] = {};  // buffers the first-person views were uploaded into
int g_armsViewNext = 0;
std::mutex g_fpProbeMutex;
std::unordered_set<uint64_t> g_fpProbeSeen;

void NoteFirstPersonView(const float* m, UINT size, UINT offset) {
    bool arms = size == 352 && offset == 176, world = size == 80 && offset == 0;
    if (!arms && !world)
        return;
    if (arms && g_dumpView.exchange(false)) {
        const float* base = m - offset / 4;
        for (UINT r = 0; r < size / 16; ++r)
            Log("View dump: +%3u: %12.5f %12.5f %12.5f %12.5f", r * 16, base[r * 4], base[r * 4 + 1], base[r * 4 + 2],
                base[r * 4 + 3]);
    }
    std::lock_guard lock(g_fpMutex);
    std::memcpy(arms ? g_fpArms : g_fpWorld, m, sizeof(g_fpArms));
    (arms ? g_fpArmsTime : g_fpWorldTime) = GetTickCount64();
    if (ID3D11Resource* buffer = t_uploadResource; world && buffer &&
        std::find(std::begin(g_worldViewBuffers), std::end(g_worldViewBuffers), buffer) == std::end(g_worldViewBuffers)) {
        g_worldViewBuffers[g_worldViewNext] = buffer;
        g_worldViewNext = (g_worldViewNext + 1) % 4;
    }
    if (ID3D11Resource* buffer = t_uploadResource; arms && buffer &&
        std::find(std::begin(g_armsViewBuffers), std::end(g_armsViewBuffers), buffer) == std::end(g_armsViewBuffers)) {
        g_armsViewBuffers[g_armsViewNext] = buffer;
        g_armsViewNext = (g_armsViewNext + 1) % 8;
    }
    float eyeArms[3];
    if (!SolveEye(g_fpArms, eyeArms))
        return;
    if (arms) {
        float axes[3][3] = {{m[0], m[1], m[2]}, {m[4], m[5], m[6]}, {-m[12], -m[13], -m[14]}};  // right, up, back
        for (auto& a : axes) {
            float length = std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
            if (!(length > 1e-6f))
                return;
            a[0] /= length, a[1] /= length, a[2] /= length;
        }
        // Right = up x back: the x row may also carry where in the picture the view goes (one half
        // with per-eye views), which tilts it; the up and back rows never do.
        axes[0][0] = axes[1][1] * axes[2][2] - axes[1][2] * axes[2][1];
        axes[0][1] = axes[1][2] * axes[2][0] - axes[1][0] * axes[2][2];
        axes[0][2] = axes[1][0] * axes[2][1] - axes[1][1] * axes[2][0];
        for (int i = 0; i < 3; ++i) {
            g_viewToArms[i * 4] = axes[0][i], g_viewToArms[i * 4 + 1] = axes[1][i];
            g_viewToArms[i * 4 + 2] = axes[2][i], g_viewToArms[i * 4 + 3] = eyeArms[i];
        }
        // Which eye: by the place the last frame's uploads were in, in its order.
        if (uint32_t frame = g_frameSerial.load(std::memory_order_relaxed); frame != g_armsFrame) {
            g_armsFrame = frame;
            g_armsUploadsBefore = g_armsUploads;
            g_armsUploads = 0;
            g_eyeOrder.clear();
            if (g_frameSides.size() >= 2) {
                const auto [lo, hi] = std::minmax_element(g_frameSides.begin(), g_frameSides.end());
                const bool leftIsLeft = views::EyeOffset(0) <= views::EyeOffset(1);  // unless swapped
                if (*hi - *lo > 0.01f)  // two places a few centimetres apart
                    for (float side : g_frameSides)
                        g_eyeOrder.push_back((side < (*lo + *hi) / 2) == leftIsLeft ? 0 : 1);
            }
            g_frameSides.clear();
        }
        const uint32_t k = g_armsUploads++, n = g_armsUploadsBefore;
        if (k == 0) {
            std::memcpy(g_frameFirstEye, eyeArms, sizeof(eyeArms));
            std::memcpy(g_frameRight, axes[0], sizeof(g_frameRight));
        }
        float side = 0.0f;
        for (int i = 0; i < 3; ++i)
            side += (eyeArms[i] - g_frameFirstEye[i]) * g_frameRight[i];
        g_frameSides.push_back(side);
        int eye = -1;
        if (views::Active() && n == g_eyeOrder.size() && k < n)
            eye = g_eyeOrder[k];
        else if (views::Active() && n >= 2 && n % 2 == 0)
            eye = k < n / 2 ? 0 : 1;  // no order learned: the first half
        if (g_eyeTrace.load(std::memory_order_relaxed) > 0) {
            --g_eyeTrace;
            Log("Eye trace: frame %u, upload %u of %u last frame -> eye %d (%s), %+.1f mm along the first's right, "
                "thread %lu",
                g_armsFrame, k, n, eye, n == g_eyeOrder.size() ? "by place" : "first half", side * 1000.0f,
                GetCurrentThreadId());
        }
        g_haveViewToArms = true;
        ++g_viewUploads;
        if (ID3D11Resource* buffer = t_uploadResource) {
            ViewByBuffer* slot = nullptr;
            for (ViewByBuffer& v : g_viewByBuffer)
                if (v.buffer == buffer)
                    slot = &v;
            if (!slot) {
                slot = &g_viewByBuffer[g_viewByBufferNext];
                g_viewByBufferNext = (g_viewByBufferNext + 1) % 8;
                slot->buffer = buffer;
            }
            std::memcpy(slot->viewToArms, g_viewToArms, sizeof(g_viewToArms));
            std::memcpy(slot->vp, g_fpArms, sizeof(slot->vp));
            slot->eye = eye;
        }
    }
    float eyeWorld[3];
    if (!g_fpWorldTime || !SolveEye(g_fpWorld, eyeWorld))
        return;
    float origin[3];
    for (int i = 0; i < 3; ++i) {
        float d = eyeWorld[i] - eyeArms[i];
        origin[i] = std::round(d);
        if (std::fabs(d - origin[i]) > 0.1f)
            return;
    }
    if (g_haveFpOrigin && std::memcmp(origin, g_fpOrigin, sizeof(origin)) != 0 &&
        g_weaponWatch.load(std::memory_order_relaxed))
        Log("Hands: the arms' space's origin moved to (%.0f %.0f %.0f)", origin[0], origin[1], origin[2]);
    std::memcpy(g_fpOrigin, origin, sizeof(origin));
    g_haveFpOrigin = true;
}

void Compose(const float* a, const float* b, float* out);
bool Invert(const float* m, float* out);
// The latest draw's head -> the arms' space as the world's picture has it (ToArms's seenFrom), and that space's origin: the hand's speed into the world for something let
// go of, and the room's way into the world for the loose things (hands.cpp). Under g_fpMutex.
float g_lastSeenFrom[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
float g_lastSeenOrigin[3] = {};
bool g_haveLastSeenFrom = false;

// The eye in the world from the camera its views are built with (views.cpp), against the same eye in the arms' space. The
// 80-byte view-projection in the world above isn't always seen (never on the train, 2026-11-02): without the origin, what's
// moved with the gun in the world swung round the world's own origin, kilometres away, or wasn't moved at all.
void NoteWorldEyeLocked(const float* eyeWorld) {
    if (!g_haveViewToArms)
        return;
    const float eyeArms[3] = {g_viewToArms[3], g_viewToArms[7], g_viewToArms[11]};
    float origin[3];
    for (int i = 0; i < 3; ++i) {
        const float d = eyeWorld[i] - eyeArms[i];
        origin[i] = std::round(d);
        if (std::fabs(d - origin[i]) > 0.12f)
            return;
    }
    if (!g_haveFpOrigin || (g_weaponWatch.load(std::memory_order_relaxed) && std::memcmp(origin, g_fpOrigin, sizeof(origin)) != 0))
        Log("Hands: the arms' space's origin at %.0f %.0f %.0f (the eye at %.2f %.2f %.2f in it)", origin[0], origin[1], origin[2],
            eyeArms[0], eyeArms[1], eyeArms[2]);
    std::memcpy(g_fpOrigin, origin, sizeof(origin));
    g_haveFpOrigin = true;
}

// The head's space -> the arms' space as the world's picture has it, for a draw with the first-person view v, and the arms'
// space's origin that goes with v: the main camera of that view's build, less that origin. The first-person view has the game's
// own place and rows, in ordinary play neither the head's move nor its turn; not drawing two views, it's all there is. The
// origin is this draw's own: the noted one goes over a metre a build after the views do, and what's moved in the world with
// the gun would be a metre off for that moment.
float g_seenEye[3] = {};  // WorldViewLocked's main camera: the game's eye. Under g_fpMutex
void WorldViewLocked(const float* v, float* out, float* origin, bool& haveOrigin) {
    std::memcpy(out, v, 12 * sizeof(float));
    std::memcpy(origin, g_fpOrigin, sizeof(g_fpOrigin));
    haveOrigin = g_haveFpOrigin;
    if (!views::Active())
        return;
    const ULONGLONG now = GetTickCount64();
    const MainCamera* best = nullptr;
    float bestOff = 0.0f;
    for (int k = 1; k <= kMainCameras; ++k) {  // the newest of those as near
        const MainCamera& c = g_mainCameras[(g_mainCameraNext - k + kMainCameras) % kMainCameras];
        if (now - c.at > kMainCameraFresh)
            continue;
        float off = 0.0f;
        for (int i = 0; i < 3; ++i) {
            const float d = c.gameEye[i] - v[i * 4 + 3];
            off += (d - std::round(d)) * (d - std::round(d));
        }
        if (!best || off < bestOff - 1e-5f)
            best = &c, bestOff = off;
    }
    if (!best || bestOff > kMainCameraOff * kMainCameraOff)
        return;
    std::memcpy(g_seenEye, best->gameEye, sizeof(g_seenEye));
    std::memcpy(out, best->toWorld, 12 * sizeof(float));
    for (int i = 0; i < 3; ++i) {
        origin[i] = std::round(best->gameEye[i] - v[i * 4 + 3]);
        out[i * 4 + 3] -= origin[i];
    }
    haveOrigin = true;
}

// Hand draws per eye since the last log (not known or one view, left, right).
std::atomic<uint32_t> g_drawsByEye[3];

// The hands' targets, from the view's space into the arms' space with the view this draw is drawn
// with (the latest, if its constant buffer isn't known). False until a view has been seen.
bool ToArms(skin::HandTargets& t, ID3D11DeviceContext* ctx, int* eyeOut = nullptr) {
    ID3D11Buffer* bound[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = {};
    ctx->VSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, bound);
    float v[12], seenFrom[12];
    int viewEye = -1;
    {
        std::lock_guard lock(g_fpMutex);
        if (!g_haveViewToArms) {
            for (ID3D11Buffer* b : bound)
                if (b)
                    b->Release();
            return false;
        }
        std::memcpy(v, g_viewToArms, sizeof(v));
        bool found = false;
        for (ID3D11Buffer* b : bound)
            for (const ViewByBuffer& view : g_viewByBuffer)
                if (b && !found && view.buffer == b) {
                    found = true;
                    if (std::memcmp(view.viewToArms, v, sizeof(v)) != 0)
                        ++g_viewDiffered;
                    std::memcpy(v, view.viewToArms, sizeof(v));
                    viewEye = view.eye;
                }
        ++(found ? g_viewBound : g_viewLatest);
        WorldViewLocked(v, seenFrom, t.seenOrigin, t.seenHaveOrigin);
        std::memcpy(t.seenEye, g_seenEye, sizeof(t.seenEye));
    }
    for (ID3D11Buffer* b : bound)
        if (b)
            b->Release();

    // The view's place in the arms' space jumping more than 4 cm between the left eye's draws: the
    // game's camera stepping (the user: the bare hands jump swaying left and right, 15:06). Only while
    // the bare arms are on.
    if (viewEye == 0 && GetTickCount64() - g_bareMovedAt.load(std::memory_order_relaxed) < 250) {
        static float s_last[3] = {};
        static bool s_have = false;
        const float d[3] = {v[3] - s_last[0], v[7] - s_last[1], v[11] - s_last[2]};
        const float moved = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        static ULONGLONG s_logged = 0;
        if (s_have && moved > 0.04f && GetTickCount64() - s_logged > 200) {
            s_logged = GetTickCount64();
            Log("Bare hands: the view moved %.0f cm in the arms' space between draws", moved * 100.0f);
        }
        s_last[0] = v[3], s_last[1] = v[7], s_last[2] = v[11];
        s_have = true;
    }
    // Per-eye views: this draw's view is one eye's, half the eye distance from the middle of the
    // head the targets are measured from; that's taken off along the view's right. (Its viewport
    // can't tell: BF3 draws both views with one the width of the whole picture.)
    for (int i = 0; i < 3; ++i)
        t.eyeInArms[i] = v[i * 4 + 3];  // (the view's own eye: what's taken off below moves the head's middle the targets are measured from)
    {
        int eye = views::Active() ? viewEye : -1;
        if (eyeOut)
            *eyeOut = eye;
        ++g_drawsByEye[eye + 1];
        if (float offset = views::EyeOffset(eye); eye >= 0 && offset != 0.0f)
            for (int i = 0; i < 3; ++i)
                v[i * 4 + 3] -= v[i * 4] * offset;
    }
    float pose[12];
    // The picture's own head into this draw, before the hands' own correction below: what the world goes through (the hands' head is another, by off; through it, a
    // case lying 1.5 m off was drawn up to 4 cm off whenever the two heads differed, a judder as the head turned: the review, 2026-11-05).
    float vWorld[12];
    std::memcpy(vWorld, v, sizeof(vWorld));
    // The hands were measured from one head and this picture was drawn from another (camera.h HandsOffDrawn): the gun and arms put right for it.
    if (float off[12]; camera::HandsOffDrawn(off)) {
        Compose(v, off, pose);
        std::memcpy(v, pose, sizeof(v));
    }
    // The rifle lying where it fell (free hands, let go of): kept in the world, placed by each draw as the loose magazines are
    // (WorldToDrawn's way), so it stays put as the head turns. Its pose in the world with its own origin, into this draw's.
    float lying[12] = {};
    if (t.gunLying) {
        std::memcpy(lying, t.gunWorld, sizeof(lying));
        for (int i = 0; i < 3; ++i)
            lying[i * 4 + 3] += t.gunWorldOrigin[i] - t.seenOrigin[i];
        std::memcpy(t.gunSeen, lying, sizeof(lying));
    } else {
        Compose(seenFrom, t.gunPose, t.gunSeen);
    }
    Compose(seenFrom, t.pistolPose, t.pistolSeen);
    // A launcher-pose gun (the SMAW, any other gun held as it is) lying where it fell: kept in the world as the rifle is, its frame into this draw's.
    float lyingL[12] = {};
    if (t.launcherLying) {
        std::memcpy(lyingL, t.launcherWorld, sizeof(lyingL));
        for (int i = 0; i < 3; ++i)
            lyingL[i * 4 + 3] += t.launcherWorldOrigin[i] - t.seenOrigin[i];
        std::memcpy(t.launcherSeen, lyingL, sizeof(lyingL));
    } else {
        Compose(seenFrom, t.launcherPose, t.launcherSeen);
    }
    // The world into this draw: back to the head as the world's picture has it, then out as this eye's first-person view places the head.
    t.haveDrawnFromWorld = false;
    if (float seenInverse[12]; Invert(seenFrom, seenInverse)) {
        Compose(vWorld, seenInverse, t.drawnFromWorld);
        t.haveDrawnFromWorld = true;
        std::lock_guard lock(g_fpMutex);
        std::memcpy(g_lastSeenFrom, seenFrom, sizeof(g_lastSeenFrom));
        std::memcpy(g_lastSeenOrigin, t.seenOrigin, sizeof(g_lastSeenOrigin));
        g_haveLastSeenFrom = t.seenHaveOrigin;
    }
    if (t.gunLying) {
        if (t.haveDrawnFromWorld)
            Compose(t.drawnFromWorld, lying, pose);
        else
            std::memcpy(pose, lying, sizeof(pose));  // (not drawn: DrawGun and DrawArms skip a lying rifle without the world's way in)
    } else {
        Compose(v, t.gunPose, pose);
    }
    std::memcpy(t.gunPose, pose, sizeof(pose));
    Compose(v, t.leftPose, pose);
    std::memcpy(t.leftPose, pose, sizeof(pose));
    for (int side = 0; side < 2; ++side)
        if (t.bareArm[side])
            for (int k = 0; k < barehold::kArmFrames; ++k) {  // (the upper arm and the collar too: zeros without a sleeve)
                Compose(v, t.bareForearm[side][k], pose);
                std::memcpy(t.bareForearm[side][k], pose, sizeof(pose));
            }
    if (t.haveBareChest) {
        const float* p = t.bareChest;
        float q[3];
        for (int i = 0; i < 3; ++i)
            q[i] = v[i * 4] * p[0] + v[i * 4 + 1] * p[1] + v[i * 4 + 2] * p[2] + v[i * 4 + 3];
        std::memcpy(t.bareChest, q, sizeof(q));
    }
    if (t.gun) {
        Compose(v, t.pistolPose, pose);
        std::memcpy(t.pistolPose, pose, sizeof(pose));
        if (!t.launcherLying) {
            Compose(v, t.launcherPose, pose);
            std::memcpy(t.launcherPose, pose, sizeof(pose));
        }
    } else if (t.launcherDrawn && !t.launcherLying) {  // (flying into a hand: in the view's space, as when held)
        Compose(v, t.launcherPose, pose);
        std::memcpy(t.launcherPose, pose, sizeof(pose));
    }
    if (t.launcherLying) {  // placed from the world, as the rifle lying (not drawn without the world's way in: DrawLauncher, DrawHeldPart)
        if (t.haveDrawnFromWorld)
            Compose(t.drawnFromWorld, lyingL, t.launcherPose);
        else
            std::memcpy(t.launcherPose, lyingL, sizeof(lyingL));
    }
    // (Case 3's rel is on the launcher's frame: DrawLauncher composes it with the launcher as drawn.)
    if (t.launcherCase == 2) {
        Compose(v, t.launcherCasePose, pose);
        std::memcpy(t.launcherCasePose, pose, sizeof(pose));
    }
    if (t.m320HandRound) {  // the grenade launcher's round in the hand, alike
        Compose(v, t.m320HandRoundPose, pose);
        std::memcpy(t.m320HandRoundPose, pose, sizeof(pose));
    }
    for (int i = 0; i < t.heldShells && i < static_cast<int>(std::size(t.heldShellPose)); ++i)
        if (!t.heldShellOnGun[i]) {  // a shotgun's shell in a hand, alike (one on the gun goes with the gun as drawn: DrawHeldPart)
            Compose(v, t.heldShellPose[i], pose);
            std::memcpy(t.heldShellPose[i], pose, sizeof(pose));
        }
    if (t.right) {
        Compose(v, t.rightPose, pose);
        std::memcpy(t.rightPose, pose, sizeof(pose));
    }
    // The right glove's own placings (skin::RightGlove): the gun it's drawn on as the game holds it, or its own place.
    if (t.rightGlove == skin::kRightOnGrip) {
        Compose(v, t.rightGunPose, pose);
        std::memcpy(t.rightGunPose, pose, sizeof(pose));
        Compose(v, t.rightLauncherPose, pose);  // (a launcher-pose gun's: the same as launcherPose while the right holds it, converted alike)
        std::memcpy(t.rightLauncherPose, pose, sizeof(pose));
    } else if (t.rightGlove == skin::kRightOwn) {
        Compose(v, t.rightGlovePose, pose);
        std::memcpy(t.rightGlovePose, pose, sizeof(pose));
    }
    if (t.mag) {
        Compose(v, t.magPose, pose);
        std::memcpy(t.magPose, pose, sizeof(pose));
    }
    if (t.pistolMag) {  // the train pistol's, alike
        Compose(v, t.pistolMagPose, pose);
        std::memcpy(t.pistolMagPose, pose, sizeof(pose));
    }
    for (float* p : {t.rightWrist, t.leftWrist}) {
        float q[3];
        for (int i = 0; i < 3; ++i)
            q[i] = v[i * 4] * p[0] + v[i * 4 + 1] * p[1] + v[i * 4 + 2] * p[2] + v[i * 4 + 3];
        std::memcpy(p, q, sizeof(q));
    }
    // A forearm solved from its hand (build\arms.txt "on"): its three bones and its fold point, alike.
    for (int side = 0; side < 2; ++side) {
        if (!t.arm[side])
            continue;
        for (float* bone : t.forearm[side]) {
            Compose(v, bone, pose);
            std::memcpy(bone, pose, sizeof(pose));
        }
        const float* p = t.fold[side];
        float q[3];
        for (int i = 0; i < 3; ++i)
            q[i] = v[i * 4] * p[0] + v[i * 4 + 1] * p[1] + v[i * 4 + 2] * p[2] + v[i * 4 + 3];
        std::memcpy(t.fold[side], q, sizeof(q));
    }
    // The game's own left hand's arm on a launcher-pose gun (rigLeftArm), alike.
    if (t.rigLeftArm) {
        for (float* bone : t.rigLeftForearm) {
            Compose(v, bone, pose);
            std::memcpy(bone, pose, sizeof(pose));
        }
        const float* p = t.rigLeftFold;
        float q[3];
        for (int i = 0; i < 3; ++i)
            q[i] = v[i * 4] * p[0] + v[i * 4 + 1] * p[1] + v[i * 4 + 2] * p[2] + v[i * 4 + 3];
        std::memcpy(t.rigLeftFold, q, sizeof(q));
    }
    if (t.haveChest) {
        const float* p = t.chest;
        float q[3];
        for (int i = 0; i < 3; ++i)
            q[i] = v[i * 4] * p[0] + v[i * 4 + 1] * p[1] + v[i * 4 + 2] * p[2] + v[i * 4 + 3];
        std::memcpy(t.chest, q, sizeof(q));
    }
    return true;
}

void NoteProjection(const float* m, UINT size, UINT offset, bool columns) {
    std::lock_guard lock(g_projMutex);
    if (g_seenProjections.size() >= 8)
        return;
    for (const SeenProjection& s : g_seenProjections)
        if (s.size == size && s.offset == offset)
            return;
    SeenProjection s{};
    std::memcpy(s.m, m, sizeof(s.m));
    s.size = size;
    s.offset = offset;
    s.columns = columns;
    g_seenProjections.push_back(s);
}

// weapon.txt "dump": the gun's mesh as bound for its colour pass (vertex and index buffers) and its
// bones are written next to the DLL once, to read the magazine's shape from. "dump 13755 15051"
// dumps that draw instead (the gloves), as mesh_<index count>_<start>_*.bin.
std::atomic<bool> g_dumpWanted{false};
std::atomic<bool> g_dumpOwn{false};
DrawKey g_dumpKey = kWeapon;  // written before g_dumpWanted is set
struct MeshDump {
    ID3D11Buffer* vb[4] = {};
    UINT stride[4] = {}, offset[4] = {};
    ID3D11Buffer* ib = nullptr;
    DXGI_FORMAT ibFormat = DXGI_FORMAT_UNKNOWN;
    UINT ibOffset = 0;
    D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    DrawKey key{};
    std::vector<uint8_t> bones;
};
std::mutex g_dumpMutex;
MeshDump* g_dump = nullptr;  // taken on a draw thread, written out by the next Probe

// A copy of this thread's latest bone upload, kept only while hand shift is on and only for
// blocks the size of the gloves'.
// Small fresh uploads on this thread so far: tells whether an upload came before or after a draw.
thread_local uint32_t t_uploadSerial = 0;
// The latest per-view constants (352 bytes: view, projection, view-projection at +176, eye at
// +320), as they went to the graphics card: what the draws that follow are drawn with.
struct ViewConstants {
    ID3D11Resource* resource = nullptr;
    uint8_t bytes[352];
};
thread_local ViewConstants t_viewConstants;
struct Upload {
    ID3D11Resource* resource = nullptr;
    std::vector<uint8_t> bytes;
    BoneInfo bones;
    uint32_t serial = 0;  // t_uploadSerial when it was made
};
// Gloves draws since the last log: shifted, or drawn normally because there was no fresh copy of
// their bones, or because BF3's buffer wasn't bound to the vertex shader.
std::atomic<UINT> g_shifted{0}, g_noCopy{0}, g_noSlot{0}, g_notSkinned{0};

thread_local Upload t_upload;

// Bone selector, for working out which gloves bones belong to which hand: hands.txt next to the
// DLL lists gloves palette numbers ("0-27, 40" or "all"); only those are shifted. Read by Probe.
// Found with it (the user judging in the headset): bones 0-23 are the left hand, 24-54 the right;
// each group lifts its hand whole, without stretching. Bone 23 alone moves nothing visible. The
// second pass's black forearm lifts along with the right hand: its forearm hangs on hand bones.
constexpr UINT kLeftHandBones[2] = {0, 23};
constexpr UINT kRightHandBones[2] = {24, 54};
std::mutex g_selMutex;
bool g_selectAll = true;
bool g_selected[64] = {};
std::vector<float> g_selValues;  // the selected gloves bones' latest matrices, 12 floats each
float g_shift[3] = {0.0f, kHandShift, 0.0f};
constexpr float kSameBone = 0.02f;

bool MatchesSelected(const float* bone) {
    for (size_t i = 0; i + 12 <= g_selValues.size(); i += 12) {
        bool same = true;
        for (int j = 0; j < 12 && same; ++j)
            same = std::fabs(bone[j] - g_selValues[i + j]) < kSameBone;
        if (same)
            return true;
    }
    return false;
}

void ReadSelection() {
    static FILETIME lastWrite{};
    wchar_t path[MAX_PATH];
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&ReadSelection), &self);
    GetModuleFileNameW(self, path, MAX_PATH);
    if (wchar_t* slash = wcsrchr(path, L'\\'))
        wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path), L"hands.txt");
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (!GetFileAttributesExW(path, GetFileExInfoStandard, &info) ||
        CompareFileTime(&info.ftLastWriteTime, &lastWrite) == 0)
        return;
    lastWrite = info.ftLastWriteTime;
    FILE* f = nullptr;
    if (_wfopen_s(&f, path, L"r") != 0 || !f)
        return;
    char text[512] = {};
    fread(text, 1, sizeof(text) - 1, f);
    fclose(f);

    // "shift x y z" (metres, in the bones' own space) after the bone list sets the direction.
    float shift[3] = {0.0f, kHandShift, 0.0f};
    if (char* s = std::strstr(text, "shift")) {
        sscanf_s(s + 5, "%f %f %f", &shift[0], &shift[1], &shift[2]);
        *s = 0;
    }
    bool all = std::strstr(text, "all") != nullptr;
    bool selected[64] = {};
    for (const char* p = text; *p;) {
        if (*p < '0' || *p > '9') {
            ++p;
            continue;
        }
        char* end = nullptr;
        long a = std::strtol(p, &end, 10), b = a;
        p = end;
        if (*p == '-') {
            b = std::strtol(p + 1, &end, 10);
            p = end;
        }
        for (long k = a; k <= b && k < 64; ++k)
            if (k >= 0)
                selected[k] = true;
    }
    std::lock_guard lock(g_selMutex);
    g_selectAll = all;
    std::memcpy(g_selected, selected, sizeof(selected));
    std::memcpy(g_shift, shift, sizeof(shift));
    g_selValues.clear();
    Log("Hands: selection now: %s, shifted by (%.2f, %.2f, %.2f)", all ? "all bones" : text, shift[0], shift[1],
        shift[2]);
}
// The grenade launcher's barrel swung open (radians), its round in it, slid out (metres): see kM320Hinge.
std::atomic<float> g_m320Barrel{0.0f}, g_m320RoundOut{0.0f};
std::atomic<bool> g_m320RoundIn{true};
std::atomic<int> g_m320TestMode{0};   // weapon.txt "m320test N" (m320.h Frame::test)
std::atomic<bool> g_m320Log{false};   // weapon.txt "m320log 1": the reload by hand's state each frame
std::atomic<bool> g_tubeByHand{true};  // weapon.txt "tubebyhand 0": a tube shotgun's shells the game's own again (skin::TubeByHand)
std::atomic<int> g_shellTest{0};       // weapon.txt "shelltest N": hands.cpp's scripted hand on the shotgun (skin::ShellTest)
std::atomic<bool> g_shellLog{false};   // weapon.txt "shelllog 1": the shells' state each frame
std::atomic<bool> g_looseOn{true};  // magazines let go of as rigid bodies in the world (loose.h); weapon.txt "loose 0" the old straight fall

void ReadWeaponSelection() {
    static FILETIME lastWrite{};
    wchar_t path[MAX_PATH];
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&ReadWeaponSelection), &self);
    GetModuleFileNameW(self, path, MAX_PATH);
    if (wchar_t* slash = wcsrchr(path, L'\\'))
        wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path), L"weapon.txt");
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (!GetFileAttributesExW(path, GetFileExInfoStandard, &info)) {
        scopeview::SettingsRead();  // (no file: the defaults are what's read)
        return;
    }
    if (CompareFileTime(&info.ftLastWriteTime, &lastWrite) == 0)
        return;
    lastWrite = info.ftLastWriteTime;
    FILE* f = nullptr;
    if (_wfopen_s(&f, path, L"r") != 0 || !f)
        return;
    char text[1024] = {};
    fread(text, 1, sizeof(text) - 1, f);
    fclose(f);
    // Line by line:
    //   "37 shift 0 -0.3 0"  the colour pass's bones and the shift (metres, in the bones' space);
    //                        "hide" instead of the shift makes them vanish
    //   "pass 2: 41"         bones of kWeaponPasses[2] (its own palette) to treat alike
    //   "watch"              log which bones move, per pass;  "dump"  write the gun's mesh once
    //   "floor 0.05"         the ground's height in the body's space, for the dropped magazine
    //   "wcam 0.3 0 0"       the weapon camera moved along its right, up and back (camera.cpp)
    //   "hidefx 36 234"      draws with those index counts hidden (effects.cpp), to tell which
    //                        are the muzzle flash and the casings
    //   "movefx 36"          draws with those index counts drawn at the gun in the hand (effects.cpp)
    //   "dumpfx 234"         the next three draws with that index count log their block
    //   "hideps 1B77A09C"    draws with that pixel shader hidden (as the census logs it this run)
    //   "wiggle 3"           that bone of the gun slides 2 cm to and fro along the barrel
    //   "flashmats 2"        which view constants moved draws change (effects.cpp)
    //   "flashlift 1"        what's moved with the gun lifted this far too (to see it plainly)
    //   "refill"             300 spare rounds for the gun in hand
    //   "tail"               log the next frame's draws to window-sized targets (effects.cpp)
    //   "viewdiff 60"        for 60 s, log where the two eye views' descriptions differ (views.cpp)
    //   "offcentre 0.2 0.2"  a test: each eye's picture drawn off-centre, its window moved 0.2 of its width outward and 0.2 of its height down (views.cpp); only
    //                        while the line is there: without it, or with "offcentre 0 0", the pictures are centred as ever
    //   "renderfov 110"      a test: the eyes' pictures drawn 110 degrees wide while the line is there, whatever bf3vr.ini's render_fov says (xr_runtime.cpp); the
    //                        picture's size in pixels stays, so it is that much less sharp
    //   "lodfov 100"         a test: the eyes' level of detail picked as for a picture 100 degrees wide, however wide it is drawn (views.cpp); without the line, by
    //                        the angle it is drawn with
    //   "shadowres 2560"     a test: the eyes' sun shadow maps 2560 texels square while the line is there (scopeview.cpp); without it, bf3vr.ini's
    //                        [world] shadowmap_resolution, or the game's own
    //   "postpin 0"          a test: the vignette and the suppression blur's clear middle left to the picture's size (0) or held as at 100 degrees (1) while the
    //                        line is there (postpin.cpp); without it, bf3vr.ini's [stereo] pin_effects
    //   "turnby 30"          the view turned 30 degrees right at once, -30 left (input.cpp)
    //   "qte 2"              (the default) the grips answer quick-time events: a fresh squeeze of either grip presses exactly
    //                        the keys the game's live input triggers await (qtewatch.cpp, qte.h, input.cpp); "qte 1": version 1,
    //                        in a scripted moment a squeeze holds E, F, Space, Z, Left Ctrl and both mouse buttons down; "qte 0": off
    //   "qtemash 0"          version 2: a tapped key once a squeeze ("qtemash 1", the default: again every 130 ms while squeezed)
    //   "qtefight 0"         a fist fight's prompts not read: its six quick-time triggers press nothing ("qtefight 1", the default:
    //                        its open prompt read from its prefab's gates, its one key pressed alone, every other quick-time
    //                        key the mod would send held up while it's open; qtefight.h, qtewatch.cpp, input.cpp)
    //   "qtetest 1.5"        a test: the left grip as let go 0.3 s and then squeezed 1.5 s, by the same rules, with nobody
    //                        in the headset too (the stand-in on, the game in front); with version 2, for 1.5 s the grip is
    //                        squeezed from 0.3 s after the game awaits a key until it awaits none
    //   "cutsceneturn 0"     a scripted camera (a cutscene's or a ride's) not turned to the head: the picture
    //                        held still in the room as before ("cutsceneturn 1", the default, turns it; camera.cpp)
    //   "bodylock 0"         in a scripted picture turned to the head, the views the player's own body is drawn
    //                        with left as the game made them, moving with the head ("bodylock 1", the default,
    //                        turns them with it: LockBody)
    //   "bodystereo 0"       that body left in the middle, both eyes drawing it alike, as the game draws it
    //                        ("bodystereo 1", the default, moves it to each eye's place: its own depth)
    //   "bodytrace 120"      the next 120 views the body lock reads logged in order (which eye's pass each is)
    //   "steer 35 65"        the left hand on the handguard steers the gun only within 35 degrees of the
    //                        right hand's line, not at all past 65 (without it: while out ahead of the
    //                        body, the left hand aims and the right levels: hands.cpp, steer.h)
    //   "empty"              no rounds left in it: the game's reload (R) is then its empty one
    //   "dumpview"           log BF3's per-view constants once
    //   "eyetrace"           log the next 40 first-person view uploads and the eye each is given
    //   "sound bolt"         play that reload sound (build\sounds\bolt.wav), a little to the right
    //   "standin 1"          a stand-in right hand holds the rifle while the controller sleeps
    //                        (input.cpp; stays as set until "standin 0")
    //   "pull 30"            the charging handle held 30 mm back (reload.cpp), to see the cover
    //   "handleruns 1"       a held gun's handle with no bone of its own (handleruns.h: the Mk11's T handle) drawn with the hand but without the
    //                        shaft drawn out of the receiver behind it; "handleruns 0": left home as the game draws it, as before; without the
    //                        line (or "handleruns 2"), with its shaft
    //   "hudfix 1"           a HUD experiment (overlay.cpp)
    //   "scopetrace"         log three frames' draws of the scope's mesh and what became of each
    //   "scopeboost 12 2"    the scope's 12-index part (the reticle) drawn twice more on itself
    //   "hudskip 10-19 40"   those HUD draws of each frame left out (overlay.cpp), to tell which is which
    //   "hudskiptex 64x32"   the HUD draws with a texture that size left out
    //   "orangemarkers 1"    the enemies' orange markers left on (taken off by default: overlay.cpp)
    //   "crosshair 1"        the game's own crosshair left on (taken off by default: overlay.cpp)
    //   "lensflare 1"        the sun's lens flare ghost left on (taken off by default: shaders.cpp);
    //                        "flareps 1a2b..." more of its pixel shaders by checksum (as "tail" logs them)
    //   "fingertest 1"       the glove's trigger finger drawn as if the trigger were pulled (nothing fires)
    //   "filmtrigger 2 1 3"  a film's pulls of the right trigger (0 the left), 2 s each, 3 of them, through the mod's own trigger (input.cpp: its stop
    //                        at an empty magazine too); real rounds, only with the stand-in's head 45 degrees down or more; "filmtrigger 0" stops it
    //   "casings 0"          the spent casings where the game throws them (effects.cpp); "casings 1 0.45
    //                        1.3": out of the ejection port, all the way within 0.45 m of it, not at 1.3 m
    //   "flashquads 0"       the flash and smoke where the game has them (effects.cpp); "flashquads 1 0.6 2.2": at the gun in the
    //                        hand, all the way within 0.6 m of the barrel's line up to 2.2 m past the muzzle
    //   "flashsurvey 3000"   every kind of draw for that many ms from the next press of the fire button, with what it places near
    //                        the game's gun: one line a kind when it's over (what isn't moved with the gun yet shows there)
    //   "flashscan"          for 800 ms from the next press of the fire button, every blended draw logged (a stall)
    //   "flashburst 16"      that many frames from the next press saved, as they were (build\eye_burst_N.bmp); "burstnow 1" at once
    //   "gunpartlog"         twice a second, each part of the rifle against its body, as the game has them
    //   "partshift 7848 1 x y z" that part's bone moved so far before anything looks at it (a pose of the game's, tried on foot); "partpin 0": seated, the parts left as the game has them
    //   "streamview 0"       the desktop window (and OBS) shows both eyes side by side, not the left eye alone
    //                        ("streamview 1"; Home in game, [stereo] stream_view; streamview.cpp)
    //   "streamfov 85"       the stream view's degrees across;  "streamtilt 5"  it looks 5 degrees lower
    //   "streamaspect 1.7778" the shape it's drawn for, squeezed into the window (0: the window's own)
    //   "streamfit 1"        the whole eye's picture fitted in as the headset shows it, black at its sides
    //   "streamsteady 0"     the stream view as the head moves; "streamsteady 1 0.5 3" steadied, the horizon
    //                        level (held still 0.5 Hz, 3 Hz faster per radian a second of turning)
    //   "riflespread 0"      the M416's scatter as the game has it unzoomed (on by default: as it is zoomed; ClearRifleSpread)
    //   "rifleup 0.29"       the M416 raised so far from the aim, so its sights look along it (with "zoom" and "riflelog": measured)
    //   "rifleshot 0"        its bullets left to start from the game's camera (on by default: on the sights' line, at the muzzle)
    //   "riflelog"           each second, the view on its body, the aim on it, the drawn rifle against the aim, its bullets' start
    //   "reticlefar 0"       the ACOG's chevron where its model has it (on by default: on each eye's own line along the sights, as a real one looks)
    //   "launcherhands 0"    the launcher all as the game's animation has it (on by default: the left hand free, onto the front
    //                        grip with a squeeze, the rockets loaded by hand: rocket.cpp); "launchersteer 0" the left hand on
    //                        the grip doesn't point it; "rockettest 1" a scripted left hand holds a new case, "rockettest 3" on the front grip,
    //                        "rockettest 4" the whole reload, "rockettest 5" the same without rolling the wrist; 0 stops it (they stay on)
    float shift[3] = {}, floor = 0.0f, wcam[3] = {};
    bool hide = false, watch = false, dump = false, any = false, wcamSet = false;
    UINT hideFx[4] = {};
    UINT moveFx[4] = {};
    uintptr_t hidePs[4] = {};
    int wiggle = -1;
    int flashMats = 2 | 4;
    float flashLift = 0.0f;
    int hidePsCount = 0;
    float testPull = -1.0f;

    int zoomHold = 0;
    int hudFix = 0;
    uint32_t boost[kBoostRules] = {kDefaultBoost[0], kDefaultBoost[1], kDefaultBoost[2], kDefaultBoost[3]};
    uint64_t hudSkip[4] = {};
    uint32_t hudSkipTex[4] = {};
    int keepOrange = 0;
    int keepFlare = 0;
    float sunGlare = -1.0f;  // (weapon.txt "sunglare": tests only; -1 the built-in amount)
    int keepCrosshair = 0;
    uint64_t flarePs[8] = {};
    int flarePsCount = 0;
    float steerCone[2] = {-1.0f, -1.0f};  // the body decides (hands.cpp) unless "steer 35 65"
    float offCentre[2] = {};  // "offcentre": the eyes' pictures centred unless the file says otherwise
    float testFov = 0.0f;  // "renderfov": bf3vr.ini's angle unless the file says otherwise
    float lodFov = 0.0f;  // "lodfov": the level of detail by the angle drawn unless the file says otherwise
    int shadowRes = 0;  // "shadowres": bf3vr.ini's shadow map size unless the file says otherwise
    int noBlur = -1;  // "noblur": bf3vr.ini's motion_blur_passes unless the file says otherwise
    int postPin = -1;  // "postpin": bf3vr.ini's pin_effects unless the file says otherwise
    float fingerTest = 0.0f;
    int casings = 1;
    float casingReach[2] = {0.45f, 1.3f};
    int moveCount = 0;
    int hideCount = 0;
    DrawKey dumpKey = kWeapon;
    PassSelection selected[kWeaponPassCount] = {};
    int partShifts = 0;
    int aimFind = 1, heldShot = 1;  // "aimfind", "heldshot": on unless a line says otherwise (the line taken away, on again at the next read)
    int handleRuns = 2;             // "handleruns": likewise, 2 unless a line says otherwise
    // "qte", "qtemash", "qtetest", "qtefight": a line taken away is its default again at the next read (version 2, mashing on, no test, the fights'
    // reader on): a test's "qte 0" or its grip left over from a headless run must not carry into the player's game.
    bool sawQte = false, sawQteMash = false, sawQteTest = false, sawQteFight = false;
    g_partPin = !std::strstr(text, "partpin 0");
    for (char* line = text; line && *line;) {
        char* next = std::strchr(line, '\n');
        if (next)
            *next++ = 0;
        if (const char* bs = std::strstr(line, "bodystereo")) {
            int on = 1;
            sscanf_s(bs + 10, "%d", &on);
            if (g_bodyStereo.exchange(on != 0) != (on != 0))
                Log("Camera: the body of a scripted picture %s", on ? "moved to each eye's place (its own depth)"
                                                                    : "left in the middle, both eyes drawing it alike");
            line = next;
            continue;
        }
        // The body lock's trace, before "trace" (the gun's) would take it.
        if (const char* bt = std::strstr(line, "bodytrace")) {
            int n = 120;
            sscanf_s(bt + 9, "%d", &n);
            g_bodyTrace = n;
            line = next;
            continue;
        }
        // A film's pulls of a trigger (input.cpp), early: no other word is part of it. Started by each read with the line in it.
        if (const char* fp = std::strstr(line, "filmtrigger")) {
            float seconds = 0.0f;
            int hand = 1, pulls = 1;
            sscanf_s(fp + 11, "%f %d %d", &seconds, &hand, &pulls);
            input::StartFilmPull(seconds, hand, pulls);
            line = next;
            continue;
        }
        // The head's place in the room moving the view (headmove.h), first: no other word is part of these.
        if (const char* six = std::strstr(line, "sixdof")) {
            int on = 1;
            sscanf_s(six + 6, "%d", &on);
            headmove::SetOn(on != 0);
            line = next;
            continue;
        }
        if (const char* six = std::strstr(line, "sixwalk")) {
            int on = 1;
            sscanf_s(six + 7, "%d", &on);
            headmove::SetWalk(on != 0);
            line = next;
            continue;
        }
        if (const char* six = std::strstr(line, "sixcrouch")) {
            int on = 1;
            sscanf_s(six + 9, "%d", &on);
            headmove::SetCrouch(on != 0);
            line = next;
            continue;
        }
        if (const char* six = std::strstr(line, "sixlean")) {
            float free = 0.25f, most = 0.32f;
            sscanf_s(six + 7, "%f %f", &free, &most);
            headmove::SetLean(free, most);
            line = next;
            continue;
        }
        if (const char* six = std::strstr(line, "sixlog")) {
            int seconds = 60;
            sscanf_s(six + 6, "%d", &seconds);
            headmove::LogFor(seconds);
            line = next;
            continue;
        }
        if (const char* ht = std::strstr(line, "headtest")) {
            float place[3] = {};
            if (sscanf_s(ht + 8, "%f %f %f", &place[0], &place[1], &place[2]) == 3)
                headmove::SetHeadTest(place);
            else
                headmove::SetHeadTest(nullptr);  // "headtest off"
            line = next;
            continue;
        }
        if (const char* mt = std::strstr(line, "movetest")) {
            float length = 0.5f, seconds = 1.5f, degrees = 0.0f;
            sscanf_s(mt + 8, "%f %f %f", &length, &seconds, &degrees);
            headmove::StartMoveTest(length, seconds, degrees);
            line = next;
            continue;
        }
        // The window fitted to the monitor or as big as the game draws (window.cpp).
        if (const char* wf = std::strstr(line, "windowfit")) {
            int on = 1;
            sscanf_s(wf + 9, "%d", &on);
            window::SetFit(on != 0);
            line = next;
            continue;
        }
        // The stream view's (streamview.cpp), first: no other word is part of them.
        if (const char* sv = std::strstr(line, "streamview")) {
            int on = 1;
            sscanf_s(sv + 10, "%d", &on);
            streamview::SetOn(on != 0);
            line = next;
            continue;
        }
        if (const char* sf = std::strstr(line, "streamfov")) {
            float degrees = 90.0f;
            sscanf_s(sf + 9, "%f", &degrees);
            streamview::SetFov(degrees);
            line = next;
            continue;
        }
        if (const char* sfit = std::strstr(line, "streamfit")) {
            int on = 1;
            sscanf_s(sfit + 9, "%d", &on);
            streamview::SetFit(on != 0);
            line = next;
            continue;
        }
        if (const char* sa = std::strstr(line, "streamaspect")) {
            float aspect = 16.0f / 9.0f;
            sscanf_s(sa + 12, "%f", &aspect);
            streamview::SetAspect(aspect);
            line = next;
            continue;
        }
        if (const char* ss = std::strstr(line, "streamsteady")) {
            int on = 1;
            float still = -1.0f, speedUp = -1.0f;
            sscanf_s(ss + 12, "%d %f %f", &on, &still, &speedUp);
            streamview::SetSteady(on != 0, still, speedUp);
            line = next;
            continue;
        }
        if (const char* st = std::strstr(line, "streamtilt")) {
            float degrees = 0.0f;
            sscanf_s(st + 10, "%f", &degrees);
            streamview::SetTilt(degrees);
            line = next;
            continue;
        }
        if (const char* si = std::strstr(line, "standin")) {
            int on = 1;
            float pitch = 0.0f, yaw = 0.0f, shift[3] = {};
            sscanf_s(si + 7, "%d %f %f %f %f %f", &on, &pitch, &yaw, &shift[0], &shift[1], &shift[2]);
            input::SetStandIn(on != 0, pitch, yaw, shift);
            line = next;
            continue;
        }
        if (const char* pl = std::strstr(line, "pull")) {
            float mm = 0.0f;
            if (sscanf_s(pl + 4, "%f", &mm) == 1)
                testPull = mm / 1000.0f;
            line = next;
            continue;
        }
        if (const char* hf = std::strstr(line, "hudfix")) {
            sscanf_s(hf + 6, "%d", &hudFix);
            line = next;
            continue;
        }
        if (const char* ft = std::strstr(line, "fingertest")) {
            fingerTest = 1.0f;
            sscanf_s(ft + 10, "%f", &fingerTest);
            line = next;
            continue;
        }
        if (const char* om = std::strstr(line, "orangemarkers")) {
            keepOrange = 1;
            sscanf_s(om + 13, "%d", &keepOrange);
            line = next;
            continue;
        }
        if (const char* ch = std::strstr(line, "crosshair")) {
            keepCrosshair = 1;
            sscanf_s(ch + 9, "%d", &keepCrosshair);
            line = next;
            continue;
        }
        if (const char* sg = std::strstr(line, "sunglare")) {
            sunGlare = 1.0f;
            sscanf_s(sg + 8, "%f", &sunGlare);
            line = next;
            continue;
        }
        if (const char* lf = std::strstr(line, "lensflare")) {
            keepFlare = 1;
            sscanf_s(lf + 9, "%d", &keepFlare);
            line = next;
            continue;
        }
        if (const char* fp = std::strstr(line, "flareps")) {
            unsigned long long c[8] = {};
            const int n = sscanf_s(fp + 7, "%llx %llx %llx %llx %llx %llx %llx %llx", &c[0], &c[1], &c[2], &c[3], &c[4],
                                   &c[5], &c[6], &c[7]);
            for (int i = 0; i < n && flarePsCount < 8; ++i)
                flarePs[flarePsCount++] = c[i];
            line = next;
            continue;
        }
        if (const char* ca = std::strstr(line, "casings")) {
            sscanf_s(ca + 7, "%d %f %f", &casings, &casingReach[0], &casingReach[1]);
            line = next;
            continue;
        }
        if (const char* ht = std::strstr(line, "hudskiptex")) {  // before "hudskip", which it contains
            unsigned w[4] = {}, h[4] = {};
            const int n = sscanf_s(ht + 10, "%ux%u %ux%u %ux%u %ux%u", &w[0], &h[0], &w[1], &h[1], &w[2], &h[2], &w[3],
                                   &h[3]);
            for (int i = 0; i < 4; ++i)
                hudSkipTex[i] = 2 * i + 1 < n ? w[i] << 16 | (h[i] & 0xFFFF) : 0;
            line = next;
            continue;
        }
        if (const char* hs = std::strstr(line, "hudskip")) {
            for (const char* p = hs + 7; *p;) {  // numbers and ranges, as "10-19 40"
                if (*p < '0' || *p > '9') {
                    ++p;
                    continue;
                }
                char* end = nullptr;
                long a = std::strtol(p, &end, 10), b = a;
                p = end;
                if (*p == '-') {
                    b = std::strtol(p + 1, &end, 10);
                    p = end;
                }
                for (long k = std::max(a, 0L); k <= b && k < 256; ++k)
                    hudSkip[k / 64] |= 1ull << (k % 64);
            }
            line = next;
            continue;
        }
        if (const char* w = std::strstr(line, "wcam")) {
            wcamSet = sscanf_s(w + 4, "%f %f %f", &wcam[0], &wcam[1], &wcam[2]) == 3;
            line = next;
            continue;
        }
        if (std::strstr(line, "refill")) {
            ammo::Refill();
            line = next;
            continue;
        }
        if (std::strstr(line, "tail")) {
            effects::LogFrameTail();
            overlay::LogNextFrame();
            line = next;
            continue;
        }
        if (const char* at = std::strstr(line, "aimtest")) {
            float degrees = 20.0f;
            sscanf_s(at + 7, "%f", &degrees);
            aim::StartTest(degrees);
            line = next;
            continue;
        }
        if (const char* ls = std::strstr(line, "launchersteer")) {  // (before "steer", which it contains: it was taken for that, and stopped the left hand steering)
            int on = 1;
            sscanf_s(ls + 13, "%d", &on);
            g_launcherSteer = on != 0;
            line = next;
            continue;
        }
        if (const char* st = std::strstr(line, "steer")) {  // the left hand's steering cone (hands.cpp, steer.h)
            sscanf_s(st + 5, "%f %f", &steerCone[0], &steerCone[1]);
            line = next;
            continue;
        }
        if (const char* sc = std::strstr(line, "scripted")) {  // scripted moments lead the view (aim.cpp)
            int on = 1;
            sscanf_s(sc + 8, "%d", &on);
            aim::SetScriptedFollow(on != 0);
            line = next;
            continue;
        }
        if (const char* tb = std::strstr(line, "turnby")) {  // the view turned at once (input.cpp)
            float degrees = 0.0f;
            sscanf_s(tb + 6, "%f", &degrees);
            input::TurnBy(degrees);
            line = next;
            continue;
        }
        if (const char* mf = std::strstr(line, "mirrorfix")) {  // a mirrored cutscene camera (camera.cpp)
            int on = 1;
            sscanf_s(mf + 9, "%d", &on);
            camera::SetMirrorFix(on != 0);
            line = next;
            continue;
        }
        if (const char* qf = std::strstr(line, "qtefight")) {  // a fist fight's prompt read from its gates (before "qte", which it contains)
            int on = 1;
            sscanf_s(qf + 8, "%d", &on);
            input::SetQteFight(on != 0);
            sawQteFight = true;
            line = next;
            continue;
        }
        if (const char* qm = std::strstr(line, "qtemash")) {  // version 2's tapped keys again and again while squeezed (before "qte", which it contains)
            int on = 1;
            sscanf_s(qm + 7, "%d", &on);
            input::SetQteMash(on != 0);
            sawQteMash = true;
            line = next;
            continue;
        }
        if (const char* qt = std::strstr(line, "qtetest")) {  // a test's squeeze of a grip (before "qte", which it contains); "qtetest 600 held": held throughout
            float seconds = 0.0f;
            sscanf_s(qt + 7, "%f", &seconds);
            input::StartQteTest(seconds, std::strstr(qt + 7, "held") != nullptr);
            sawQteTest = true;
            line = next;
            continue;
        }
        if (const char* qt = std::strstr(line, "qte")) {  // the game's quick-time events on the grips (input.cpp, qte.h)
            int version = 2;  // (0 off, 1 version 1: every key in a scripted moment, 2 version 2: what the game awaits)
            sscanf_s(qt + 3, "%d", &version);
            input::SetQte(version);
            sawQte = true;
            line = next;
            continue;
        }
        if (const char* ct = std::strstr(line, "cutsceneturn")) {  // a scripted camera turned to the head (camera.cpp)
            int on = 1;
            sscanf_s(ct + 12, "%d", &on);
            camera::SetCutsceneTurn(on != 0);
            line = next;
            continue;
        }
        if (const char* bl = std::strstr(line, "bodylock")) {  // the body's own views turned with the head (LockBody)
            int on = 1;
            sscanf_s(bl + 8, "%d", &on);
            g_bodyLock = on != 0;
            Log("Camera: in a scripted picture turned to the head, the views the player's own body is drawn with are %s "
                "(weapon.txt bodylock %d)",
                on ? "turned with the head as they're uploaded" : "left as the game made them, moving with the head",
                on ? 1 : 0);
            line = next;
            continue;
        }
        if (const char* vl = std::strstr(line, "viewlog")) {
            int seconds = 10;
            sscanf_s(vl + 7, "%d", &seconds);
            camera::LogTurns(seconds);
            line = next;
            continue;
        }
        if (const char* vd = std::strstr(line, "viewdiff")) {
            int seconds = 60;
            sscanf_s(vd + 8, "%d", &seconds);
            views::WatchDifferences(seconds);
            line = next;
            continue;
        }
        if (std::strstr(line, "empty")) {
            ammo::Empty();
            line = next;
            continue;
        }
        if (const char* lz = std::strstr(line, "launcherzoom")) {  // (before "zoom", which it contains)
            int on = 0;
            sscanf_s(lz + 12, "%d", &on);
            g_launcherZoom = on != 0;
            line = next;
            continue;
        }
        if (const char* zm = std::strstr(line, "zoom")) {
            zoomHold = 1;
            sscanf_s(zm + 4, "%d", &zoomHold);
            line = next;
            continue;
        }

        if (const char* fq = std::strstr(line, "flashquads")) {
            int on = 1;
            float across = 0.6f, along = 2.2f;
            sscanf_s(fq + 10, "%d %f %f", &on, &across, &along);
            effects::SetFlashQuads(on != 0, across, along);
            line = next;
            continue;
        }

        if (const char* fs = std::strstr(line, "flashsurvey")) {
            int ms = 3000;
            sscanf_s(fs + 11, "%d", &ms);
            effects::FlashSurvey(ms);
            line = next;
            continue;
        }
        if (std::strstr(line, "flashscan")) {
            effects::FlashScan();
            line = next;
            continue;
        }
        if (const char* bn = std::strstr(line, "burstnow")) {  // pictures without a press (the launcher held off, a menu)
            int frames = 1, every = 1;
            sscanf_s(bn + 8, "%d %d", &frames, &every);
            effects::BurstNow(frames, every);
            line = next;
            continue;
        }
        if (const char* fb = std::strstr(line, "flashburst")) {
            int frames = 12;
            sscanf_s(fb + 10, "%d", &frames);
            effects::FlashBurst(frames);
            line = next;
            continue;
        }


        if (const char* fl = std::strstr(line, "flashlift")) {
            sscanf_s(fl + 9, "%f", &flashLift);
            line = next;
            continue;
        }
        if (const char* fm = std::strstr(line, "flashmats")) {
            sscanf_s(fm + 9, "%d", &flashMats);
            line = next;
            continue;
        }
        if (const char* wg = std::strstr(line, "wiggle")) {
            if (sscanf_s(wg + 6, "%d", &wiggle) != 1)
                wiggle = -1;
            line = next;
            continue;
        }
        if (const char* hp = std::strstr(line, "hideps")) {
            unsigned int p[4] = {};
            hidePsCount = sscanf_s(hp + 6, "%x %x %x %x", &p[0], &p[1], &p[2], &p[3]);
            for (int k = 0; k < 4; ++k)
                hidePs[k] = p[k];
            line = next;
            continue;
        }
        if (const char* df = std::strstr(line, "dumpfx")) {
            UINT count = 0;
            if (sscanf_s(df + 6, "%u", &count) == 1)
                effects::DumpNext(count);
            line = next;
            continue;
        }
        if (const char* mv = std::strstr(line, "movefx")) {
            moveCount = sscanf_s(mv + 6, "%u %u %u %u", &moveFx[0], &moveFx[1], &moveFx[2], &moveFx[3]);
            line = next;
            continue;
        }
        if (const char* h = std::strstr(line, "hidefx")) {
            hideCount = sscanf_s(h + 6, "%u %u %u %u", &hideFx[0], &hideFx[1], &hideFx[2], &hideFx[3]);
            line = next;
            continue;
        }
        if (std::strstr(line, "dumpview")) {
            g_dumpView = true;
            line = next;
            continue;
        }
        if (std::strstr(line, "eyetrace")) {
            g_eyeTrace = 40;
            line = next;
            continue;
        }
        if (const char* ma = std::strstr(line, "mouseaim")) {  // the view steered with the mouse (aim.cpp)
            int on = 1;
            sscanf_s(ma + 8, "%d", &on);
            aim::SetMouseAim(on != 0);
            line = next;
            continue;
        }
        if (const char* af = std::strstr(line, "aimfind")) {  // the look angles' search, mouse aim's last step, the learned pitch limits (aim.cpp): "aimfind 0" as before
            sscanf_s(af + 7, "%d", &aimFind);
            line = next;
            continue;
        }
        if (const char* hs = std::strstr(line, "heldshot")) {  // a held gun's bullets on its sight's line, its data looked for again at a take: "heldshot 0" as before
            sscanf_s(hs + 8, "%d", &heldShot);
            line = next;
            continue;
        }
        if (const char* hr = std::strstr(line, "handleruns")) {  // a handle with no bone drawn with the hand: 2 with its shaft, 1 without, "handleruns 0" as before
            sscanf_s(hr + 10, "%d", &handleRuns);
            handleRuns = std::clamp(handleRuns, 0, 2);
            line = next;
            continue;
        }
        if (const char* bt = std::strstr(line, "baretwist")) {  // the bare forearms' roll offset, degrees
            float l = 0.0f, r = 0.0f;
            sscanf_s(bt + 9, "%f %f", &l, &r);
            g_bareTwist[0] = l, g_bareTwist[1] = r;
            Log("Bare hands: the forearms' roll offset %+.0f (left) %+.0f (right) degrees", l, r);
            line = next;
            continue;
        }
        if (const char* bw = std::strstr(line, "barewrist")) {  // the bare forearms solved from their hands
            int on = 1;
            sscanf_s(bw + 9, "%d", &on);
            g_bareWrist = on != 0;
            Log("Bare hands: the forearms %s", on ? "solved from the hands (bending at the wrist)" : "whole with the hands");
            line = next;
            continue;
        }
        if (const char* br = std::strstr(line, "bareelbowreach")) {  // the bare forearms' elbow end kept in the shoulder's reach (before "bareelbow": its name starts with it)
            float reach = 0.05f;
            sscanf_s(br + 14, "%f", &reach);
            reach = std::isfinite(reach) ? std::clamp(reach, 0.0f, 1.0f) : 0.05f;
            if (g_bareElbowReach.exchange(reach) != reach)
                Log("Bare arms: the elbow end %s", reach > 0.0f ? "kept within the upper arm's length and this of the shoulder (a wrist past its range bends on)" : "wherever the wrist's range turns it (as before)");
            line = next;
            continue;
        }
        if (const char* bc = std::strstr(line, "barecap")) {  // the bare forearms' open elbow end capped
            int cap = 1;
            sscanf_s(bc + 7, "%d", &cap);
            cap = std::clamp(cap, 0, 2);
            if (g_bareCap.exchange(cap) != cap)
                Log("Bare arms: the open elbow end %s", cap == 0 ? "never capped" : cap == 1 ? "capped off the arm's own line" : "always capped");
            line = next;
            continue;
        }
        if (const char* pc = std::strstr(line, "partcopy")) {  // a magazine's own draw with the mod's palette in every slot holding the game's (a test)
            int mode = 0;
            sscanf_s(pc + 8, "%d", &mode);
            mode = mode == 1 ? 1 : 0;
            if (g_partCopy.exchange(mode) != mode)
                Log("Held gun: a magazine's own draw (the M249's pouch; the AK-74M's piece and the Mk11's plate and sleeve alike) %s",
                    mode ? "with the mod's palette in every vertex and pixel shader slot holding the game's (partcopy 1, a test)"
                         : "with the mod's palette in one vertex shader slot, as before (partcopy 0)");
            line = next;
            continue;
        }
        if (const char* bs = std::strstr(line, "baresleeves")) {  // the bare arms whole up to the shoulders: the torso's sleeves posed on them
            int on = 1;
            sscanf_s(bs + 11, "%d", &on);
            if (g_bareSleeves.exchange(on != 0) != (on != 0))
                Log("Bare arms: %s", on ? "whole up to the shoulders, the sleeves on them" : "the forearms alone, the body hidden (as before)");
            line = next;
            continue;
        }
        if (const char* bp = std::strstr(line, "barestrip")) {  // the strip of uniform over the shoulders drawn with the sleeves
            int on = 1;
            sscanf_s(bp + 9, "%d", &on);
            if (g_bareStrip.exchange(on != 0) != (on != 0))
                Log("Bare arms: the strip over the shoulders %s", on ? "drawn with the sleeves" : "hidden (as before)");
            line = next;
            continue;
        }
        if (const char* bo = std::strstr(line, "barehold")) {  // the bare hands on a held gun as the gloves are
            int on = 1;
            sscanf_s(bo + 8, "%d", &on);
            if (g_bareHold.exchange(on != 0) != (on != 0))
                Log("Bare arms: with a gun in hand %s", on ? "the hands on it as the gloves are (the game's hold, the fitted hands)" : "each palm at its controller, the game's own fingers (as before)");
            line = next;
            continue;
        }
        if (const char* be = std::strstr(line, "bareelbow")) {  // the bare forearms' elbow end folded in
            int on = 1;
            sscanf_s(be + 9, "%d", &on);
            g_bareElbow = on != 0;
            Log("Bare hands: the forearms' elbow end %s", on ? "folded in" : "left as drawn");
            line = next;
            continue;
        }
        if (const char* bs = std::strstr(line, "bareshare")) {  // the bare forearms' shares of the hand's roll
            float e = 0.0f, a = 0.5f, b = 0.95f;
            sscanf_s(bs + 9, "%f %f %f", &e, &a, &b);
            g_bareShare[0] = std::clamp(e, 0.0f, 1.0f), g_bareShare[1] = std::clamp(a, 0.0f, 1.0f);
            g_bareShare[2] = std::clamp(b, 0.0f, 1.0f);
            Log("Bare hands: the forearms take %.2f (elbow end), %.2f (middle) and %.2f (wrist end) of the hand's roll", e,
                a, b);
            line = next;
            continue;
        }
        if (const char* bh = std::strstr(line, "barehands")) {  // the bare arms on the controllers
            if (const char* ln = std::strstr(bh, "learn")) {
                float seconds = 5.0f;
                sscanf_s(ln + 5, "%f", &seconds);
                g_bareLearn = true;
                g_bareLineUpAt = GetTickCount64() + static_cast<ULONGLONG>(seconds * 1000.0f);
                Log("Bare hands: the game's own arms shown for %.0f s, then the hands lined up with them", seconds);
            } else if (std::strstr(bh, "pistol")) {
                g_bareRelearnPistol = true;
                Log("Bare hands: the pistol's holds (both hands) learned again");
            } else if (std::strstr(bh, "glove")) {
                g_bareLearn = false;
                Log("Bare hands: placed as the left glove's line-up (the right mirrored)");
            } else {
                int on = 1;
                sscanf_s(bh + 9, "%d", &on);
                g_bareOn = on != 0;
                Log("Bare hands: %s", on ? "on" : "off (the game's own arms)");
            }
            line = next;
            continue;
        }
        if (const char* su = std::strstr(line, "raisehand")) {
            float up = 0.0f;
            sscanf_s(su + 9, "%f", &up);
            input::SetStandInUp(up);
            line = next;
            continue;
        }
        if (const char* lh = std::strstr(line, "launcherhands")) {
            int on = 1;
            sscanf_s(lh + 13, "%d", &on);
            g_launcherHands = on != 0;
            line = next;
            continue;
        }
        if (const char* hg = std::strstr(line, "heldguns")) {
            int on = 1;
            sscanf_s(hg + 8, "%d", &on);
            g_heldOn = on != 0;
            line = next;
            continue;
        }
        if (const char* sg = std::strstr(line, "sightlens")) {
            int n = 0;
            float off[4] = {};
            if (sscanf_s(sg + 9, "%d %f %f %f %f", &n, &off[0], &off[1], &off[2], &off[3]) == 5 && n >= 0 && n < 8)
                std::memcpy(g_sightLensOff[n], off, sizeof(off));
            line = next;
            continue;
        }
        if (const char* gd = std::strstr(line, "glassdump")) {
            int n = 1;
            sscanf_s(gd + 9, "%d", &n);
            if (n != g_glassDump.load())
                ResetScopeMaterials();
            g_glassDump = n;
            line = next;
            continue;
        }
        if (const char* go = std::strstr(line, "scopeglass")) {
            int on = 1;
            sscanf_s(go + 10, "%d", &on);
            g_scopeGlassOver = on != 0;
            line = next;
            continue;
        }
        if (const char* sl = std::strstr(line, "scopelens")) {
            sscanf_s(sl + 9, "%f %f %f %f", &g_acogEyepiece[0], &g_acogEyepiece[1], &g_acogEyepiece[2], &g_acogEyepieceRadius);
            line = next;
            continue;
        }
        if (const char* sz = std::strstr(line, "scopepower")) {
            float power = 4.0f;
            sscanf_s(sz + 10, "%f", &power);
            g_scopePower = power;
            line = next;
            continue;
        }
        if (const char* he = std::strstr(line, "halfeye")) {
            int on = 1;
            sscanf_s(he + 7, "%d", &on);
            vr::SetHalfEye(on != 0);
            line = next;
            continue;
        }
        if (const char* fl = std::strstr(line, "forceheadless")) {
            int on = 1;
            sscanf_s(fl + 13, "%d", &on);
            vr::SetForceHeadless(on != 0);
            line = next;
            continue;
        }
        if (const char* fh = std::strstr(line, "forcehand")) {
            int on = 1;
            sscanf_s(fh + 9, "%d", &on);
            input::SetForceHand(on != 0);
            line = next;
            continue;
        }
        if (const char* sh = std::strstr(line, "steadyhands")) {
            int on = 1;
            sscanf_s(sh + 11, "%d", &on);
            input::SetSteadyHands(on != 0);
            line = next;
            continue;
        }
        if (const char* pp = std::strstr(line, "padpress")) {
            unsigned mask = 0, ms = 300;
            sscanf_s(pp + 8, "%x %u", &mask, &ms);
            pad::PressButtons(static_cast<unsigned short>(mask), ms);
            line = next;
            continue;
        }
        // "scopegov 1": the view through a scope's frame-rate guard acts (scopegov.h: 3 seconds under 60 frames a second with a scope's own picture on, the
        // lens magnifies the eye's own picture a while, back at the next raise); "scopegov 0", the default, only watches and says what it would do.
        if (const char* sg = std::strstr(line, "scopegov")) {
            int on = 1;
            sscanf_s(sg + 8, "%d", &on);
            scopeview::SetGovernor(on != 0);
            line = next;
            continue;
        }
        if (const char* sv = std::strstr(line, "scopeview")) {
            if (std::strstr(sv, "scopeviewfov")) {
                float degrees = 0.0f;
                sscanf_s(sv + 12, "%f", &degrees);
                scopeview::SetFov(degrees * 3.14159265f / 180.0f);
            } else if (std::strstr(sv, "scopeviewshare")) {
                int mask = 0xFF;
                sscanf_s(sv + 14, "%i", &mask);
                scopeview::SetShare(mask);
            } else if (std::strstr(sv, "scopeviewparticles")) {
                int on = 1;
                sscanf_s(sv + 18, "%d", &on);
                scopeview::SetParticles(on != 0);
            } else if (std::strstr(sv, "scopeviewgain")) {
                float gain = 1.0f;
                sscanf_s(sv + 13, "%f", &gain);
                scopezoom::SetOwnGain(gain);
            } else if (std::strstr(sv, "scopeviewshadows")) {
                int on = 1;
                sscanf_s(sv + 16, "%d", &on);
                scopeview::SetShadows(on != 0);
            } else if (std::strstr(sv, "scopeviewterrain")) {
                int own = 0;
                sscanf_s(sv + 16, "%d", &own);
                scopeview::SetOwnTerrain(own != 0);
            } else if (std::strstr(sv, "scopeviewdetail")) {
                float ratio = 1.15f;
                sscanf_s(sv + 15, "%f", &ratio);
                scopeview::SetDetail(ratio);
            } else if (std::strstr(sv, "scopeviewforce")) {
                int on = 1;
                sscanf_s(sv + 14, "%d", &on);
                scopeview::SetForce(on != 0);
            } else if (std::strstr(sv, "scopeviewset")) {
                int offset = 0, bytes = 1;
                float value = 0.0f;
                sscanf_s(sv + 12, "%x %d %f", &offset, &bytes, &value);
                scopeview::PokeSetting(offset, bytes, value);
            } else {
                int size = 0;
                sscanf_s(sv + 9, "%d", &size);
                scopeview::SetSize(size);
            }
            line = next;
            continue;
        }
        if (std::strstr(line, "m320shot")) {
            input::TestM320Shot();
            line = next;
            continue;
        }
        // The grenade launcher's reload by hand (m320.h), tuned with the user in the headset: "m320byhand 0" the game's own reload on an empty pull again; "m320tip 30 0.3" the
        // muzzle-up angle a round in the open barrel starts to slide at and its friction; "m320flick 9" the wrist's flick that opens and shuts it (rad/s; 0 off, below 0 the other
        // way round); "m320snap 0.6" how much of the way shut it snaps the rest; "m320jab 25" the jab that throws the round back (m/s/s; 0 off); "m320magnet 0.10 0.15" how near
        // the open breech a round goes home by itself and how long it takes; "m320test 6" a scripted flick; "m320log 1" its state each frame.
        if (const char* mb = std::strstr(line, "m320byhand")) {
            int on = 1;
            sscanf_s(mb + 10, "%d", &on);
            if (m320::ByHand() != (on != 0)) {
                m320::SetByHand(on != 0);
                m320::Clear();
                Log("M320: the reload by hand %s", on ? "on" : "off: the game's own reload on an empty pull");
            }
            line = next;
            continue;
        }
        if (const char* mt = std::strstr(line, "m320tip")) {
            float degrees = 30.0f, friction = 0.3f;
            if (sscanf_s(mt + 7, "%f %f", &degrees, &friction) >= 1)
                m320::SetTip(degrees, friction), Log("M320: a round in the open barrel slides out past %.0f degrees, friction %.2f", degrees, friction);
            line = next;
            continue;
        }
        if (const char* mf = std::strstr(line, "m320flick")) {
            float rate = 9.0f;
            if (sscanf_s(mf + 9, "%f", &rate) == 1)
                m320::SetFlick(rate), Log("M320: the wrist's flick at %.1f rad/s%s", rate, rate == 0.0f ? " (off)" : "");
            line = next;
            continue;
        }
        if (const char* ms = std::strstr(line, "m320snap")) {
            float share = 0.6f;
            if (sscanf_s(ms + 8, "%f", &share) == 1)
                m320::SetSnapShut(share), Log("M320: it snaps shut from %.0f%% of the way", share * 100.0f);
            line = next;
            continue;
        }
        if (const char* mj = std::strstr(line, "m320jab")) {
            float jab = 25.0f;
            if (sscanf_s(mj + 7, "%f", &jab) == 1)
                m320::SetJab(jab), Log("M320: a jab of %.0f m/s/s throws the round back%s", jab, jab == 0.0f ? " (off)" : "");
            line = next;
            continue;
        }
        if (const char* mm = std::strstr(line, "m320magnet")) {
            float reach = 0.10f, seconds = 0.15f;
            if (sscanf_s(mm + 10, "%f %f", &reach, &seconds) >= 1)
                m320::SetMagnet(reach, seconds), Log("M320: a round within %.0f cm of the open breech goes home by itself in %.2f s", reach * 100.0f, seconds);
            line = next;
            continue;
        }
        if (const char* mt = std::strstr(line, "m320test")) {
            int mode = 0;
            sscanf_s(mt + 8, "%d", &mode);
            g_m320TestMode = mode;
            line = next;
            continue;
        }
        if (const char* ml = std::strstr(line, "m320log")) {
            int on = 1;
            sscanf_s(ml + 7, "%d", &on);
            g_m320Log = on != 0;
            line = next;
            continue;
        }
        // A tube shotgun's shells by hand (shellload.h, the M1014): "tubebyhand 0" the game's own reload again; "shelltest N" a scripted hand with nobody in
        // the headset (hands.cpp: 1 a shell up the loading port, 2 the empty gun's port load and the release, 3 the handle racked); "shelllog 1" its state
        // each frame.
        if (const char* tb = std::strstr(line, "tubebyhand")) {
            int on = 1;
            sscanf_s(tb + 10, "%d", &on);
            if (g_tubeByHand.exchange(on != 0) != (on != 0))
                Log("Shotgun: its shells by hand %s (weapon.txt tubebyhand)", on ? "on" : "off: the game's own reload");
            line = next;
            continue;
        }
        if (const char* st = std::strstr(line, "shelltest")) {
            int mode = 0;
            sscanf_s(st + 9, "%d", &mode);
            g_shellTest = mode;
            line = next;
            continue;
        }
        if (const char* sl = std::strstr(line, "shelllog")) {
            int on = 1;
            sscanf_s(sl + 8, "%d", &on);
            g_shellLog = on != 0;
            line = next;
            continue;
        }
        // The game's own ground for things dropped (ground.h): "groundprobe 1" a ray down from the eye each second, logged ("2" a map of heights too); "groundat 1" cast at
        // the end of the level's update; "groundflags 0xB5" the rays' flags; "ground 0" nothing of the game's called, the soldier's feet answer.
        if (const char* gp = std::strstr(line, "groundprobe")) {
            int level = 1;
            sscanf_s(gp + 11, "%d", &level);
            ground::SetProbe(level);
            line = next;
            continue;
        }
        if (const char* ga = std::strstr(line, "groundat")) {
            int where = 1;
            sscanf_s(ga + 8, "%d", &where);
            ground::SetWhere(where);
            line = next;
            continue;
        }
        if (const char* gf = std::strstr(line, "groundflags")) {
            unsigned flags = 0xB5;
            sscanf_s(gf + 11, "%x", &flags);
            ground::SetFlags(flags);
            line = next;
            continue;
        }
        if (const char* gr = std::strstr(line, "ground")) {
            int on = 1;
            sscanf_s(gr + 6, "%d", &on);
            ground::SetOn(on != 0);
            line = next;
            continue;
        }
        // Tuning them in the headset (loose.h): "loosepush 0.6" the push out of a well (m/s), "loosefriction 0.3" its walls', "looselie 60" how long they lie (s),
        // "loosebounce 1" how bouncy every ground is.
        if (const char* lp = std::strstr(line, "loosepush")) {
            float v = 0.0f;
            if (sscanf_s(lp + 9, "%f", &v) == 1)
                loose::SetPush(v), Log("Loose: the push out of a well %.2f m/s", v);
            line = next;
            continue;
        }
        if (const char* lf = std::strstr(line, "loosefriction")) {
            float v = 0.0f;
            if (sscanf_s(lf + 13, "%f", &v) == 1)
                loose::SetWellFriction(v), Log("Loose: the well's friction %.2f", v);
            line = next;
            continue;
        }
        if (const char* ll = std::strstr(line, "looselie")) {
            float v = 0.0f;
            if (sscanf_s(ll + 8, "%f", &v) == 1)
                loose::SetLie(v), Log("Loose: they lie %.0f s", v);
            line = next;
            continue;
        }
        if (const char* lb = std::strstr(line, "loosebounce")) {
            float v = 0.0f;
            if (sscanf_s(lb + 11, "%f", &v) == 1)
                loose::SetBounce(v), Log("Loose: the ground's bounce times %.2f", v);
            line = next;
            continue;
        }
        if (const char* lo = std::strstr(line, "loose")) {  // "loose 0": the old straight fall for magazines let go of
            int on = 1;
            sscanf_s(lo + 5, "%d", &on);
            if (g_looseOn.exchange(on != 0) != (on != 0))
                Log("Loose: magazines let go of %s", on ? "as rigid bodies in the world" : "falling straight, the old way");
            line = next;
            continue;
        }
        // Tests of the grenade launcher's reload by hand, as drawn: "m320open 15" its barrel swung open that many degrees, "m320round 0" no round in it, "m320slide 0.05" the
        // round slid that far back out.
        if (const char* mo = std::strstr(line, "m320open")) {
            float degrees = 0.0f;
            sscanf_s(mo + 8, "%f", &degrees);
            g_m320Barrel = std::clamp(degrees, 0.0f, 20.0f) * 3.14159265f / 180.0f;
            line = next;
            continue;
        }
        if (const char* mr = std::strstr(line, "m320round")) {
            int in = 1;
            sscanf_s(mr + 9, "%d", &in);
            g_m320RoundIn = in != 0;
            line = next;
            continue;
        }
        if (const char* ms = std::strstr(line, "m320slide")) {
            float metres = 0.0f;
            sscanf_s(ms + 9, "%f", &metres);
            g_m320RoundOut = std::clamp(metres, 0.0f, 0.3f);
            line = next;
            continue;
        }
        if (const char* sl = std::strstr(line, "steplog")) {
            int on = 1;
            sscanf_s(sl + 7, "%d", &on);
            footsteps::SetLog(on != 0);
            line = next;
            continue;
        }
        if (const char* oc = std::strstr(line, "offcentre")) {  // each eye's picture drawn off-centre, outward and down (views.cpp): a test
            sscanf_s(oc + 9, "%f %f", &offCentre[0], &offCentre[1]);
            line = next;
            continue;
        }
        if (const char* rf = std::strstr(line, "renderfov")) {  // the eyes' pictures drawn this many degrees wide while the line is there (xr_runtime.cpp): a test
            sscanf_s(rf + 9, "%f", &testFov);
            line = next;
            continue;
        }
        if (const char* lf = std::strstr(line, "lodfov")) {  // the eyes' level of detail picked as for a picture this many degrees wide (views.cpp): a test
            sscanf_s(lf + 6, "%f", &lodFov);
            line = next;
            continue;
        }
        if (const char* sr = std::strstr(line, "shadowres")) {  // the eyes' sun shadow maps this many texels square while the line is there (scopeview.cpp): a test
            sscanf_s(sr + 9, "%d", &shadowRes);
            line = next;
            continue;
        }
        if (const char* nb = std::strstr(line, "noblur")) {  // the eyes' motion blur passes off (1) or left alone (0) while the line is there (scopeview.cpp): a test
            noBlur = 1;
            sscanf_s(nb + 6, "%d", &noBlur);
            line = next;
            continue;
        }
        if (const char* pp = std::strstr(line, "postpin")) {  // the vignette held as at 100 degrees (1) or not (0) while the line is there (postpin.cpp): a test
            sscanf_s(pp + 7, "%d", &postPin);
            line = next;
            continue;
        }
        if (const char* np = std::strstr(line, "nearplane")) {
            float metres = 0.0f;
            sscanf_s(np + 9, "%f", &metres);
            views::SetNearPlane(metres);
            Log("Views: the near plane %s %.3f m", metres > 0.0f ? "at" : "the game's own,", metres > 0.0f ? metres : 0.06f);
            line = next;
            continue;
        }
        if (const char* ss = std::strstr(line, "scopesharpen")) {
            float amount = 0.35f;
            sscanf_s(ss + 12, "%f", &amount);
            scopezoom::SetSharpen(amount);
            line = next;
            continue;
        }
        if (const char* ht = std::strstr(line, "heldtest")) {
            int mode = 1;
            sscanf_s(ht + 8, "%d", &mode);
            g_heldTest = mode;
            line = next;
            continue;
        }
        if (const char* gn = std::strstr(line, "gunnearcut")) {
            int on = 1;
            sscanf_s(gn + 10, "%d", &on);
            g_gunNearCut = on != 0;
            line = next;
            continue;
        }
        if (const char* hk = std::strstr(line, "heldknock")) {  // how fast a fresh magazine has to hit the old one, m/s along the gun
            float v = 0.0f;
            if (sscanf_s(hk + 9, "%f", &v) == 1)
                handmag::SetKnockSpeed(v);
            line = next;
            continue;
        }
        if (const char* hm = std::strstr(line, "heldmagnet")) {  // how near the well a magazine goes home by itself, m
            float v = 0.0f;
            if (sscanf_s(hm + 10, "%f", &v) == 1)
                handmag::SetMagnetReach(v);
            line = next;
            continue;
        }
        if (const char* hf = std::strstr(line, "heldfly")) {  // how long it takes, s
            float v = 0.0f;
            if (sscanf_s(hf + 7, "%f", &v) == 1)
                handmag::SetFlyTime(v);
            line = next;
            continue;
        }
        if (const char* hl = std::strstr(line, "heldlog")) {
            int on = 1;
            sscanf_s(hl + 7, "%d", &on);
            g_heldLog = on != 0;
            line = next;
            continue;
        }
        if (const char* rt = std::strstr(line, "rockettest")) {
            int mode = 1;
            sscanf_s(rt + 10, "%d", &mode);
            g_rocketTest = mode;
            line = next;
            continue;
        }
        if (std::strstr(line, "rocketgame")) {  // as X held with the launcher empty: the game's own reload
            g_rocketGame = true;
            line = next;
            continue;
        }
        if (const char* rs = std::strstr(line, "riflespread")) {
            int on = 1;
            sscanf_s(rs + 11, "%d", &on);
            if (g_rifleSpread.exchange(on != 0) != (on != 0))
                ApplyRifleSpread(on != 0);
            line = next;
            continue;
        }
        if (const char* lu = std::strstr(line, "launcherup")) {
            float up = 0.7f;
            sscanf_s(lu + 10, "%f", &up);
            g_launcherUp = up;
            Log("Launcher: the tube raised %.2f degrees from the aim", up);
            line = next;
            continue;
        }
        if (const char* ls = std::strstr(line, "launcherspawn")) {
            int on = 1;
            sscanf_s(ls + 13, "%d", &on);
            g_launcherSpawn = on != 0;
            line = next;
            continue;
        }
        if (std::strstr(line, "launcherlog")) {
            g_launcherLog = true;
            line = next;
            continue;
        }
        if (std::strstr(line, "riflelog")) {
            g_rifleLog = true;
            line = next;
            continue;
        }
        if (const char* ru = std::strstr(line, "rifleup")) {
            float up = 0.2865f;
            sscanf_s(ru + 7, "%f", &up);
            g_rifleUp = up;
            Log("Rifle: raised %.2f degrees from the aim", up);
            line = next;
            continue;
        }
        if (const char* rf = std::strstr(line, "reticlefar")) {
            int on = 1;
            sscanf_s(rf + 10, "%d", &on);
            g_reticleFar = on != 0;
            line = next;
            continue;
        }
        if (const char* pb = std::strstr(line, "posebydraw")) {
            int on = 1;
            sscanf_s(pb + 10, "%d", &on);
            camera::SetPoseByDraw(on != 0);
            line = next;
            continue;
        }
        if (const char* nb = std::strstr(line, "nameby")) {
            int rule = 0;
            sscanf_s(nb + 6, "%d", &rule);
            camera::SetNameBy(rule);
            line = next;
            continue;
        }
        {
            static const char* const kTries[camera::kPoseTryCount] = {"posefilter", "placefilter", "poseahead", "posetest", "poselead", "freezeview", "edgefill", "fillshot", "poseadapt", "poserate"};
            bool tried = false;
            for (int k = 0; k < camera::kPoseTryCount && !tried; ++k)
                if (const char* at = std::strstr(line, kTries[k])) {
                    float value = 0.0f;
                    sscanf_s(at + std::strlen(kTries[k]), "%f", &value);
                    camera::SetPoseTries(k, value);
                    tried = true;
                }
            if (tried) {
                line = next;
                continue;
            }
        }
        if (const char* pr = std::strstr(line, "purerows")) {
            int on = 1;
            sscanf_s(pr + 8, "%d", &on);
            camera::SetPureRows(on != 0);
            line = next;
            continue;
        }
        if (const char* ea = std::strstr(line, "eyearm")) {
            float arm = 0.0773f;
            sscanf_s(ea + 6, "%f", &arm);
            headmove::SetEyeArm(arm);
            line = next;
            continue;
        }
        if (const char* ab = std::strstr(line, "armby")) {
            int rule = 0;
            sscanf_s(ab + 5, "%d", &rule);
            headmove::SetArmBy(rule);
            line = next;
            continue;
        }
        if (const char* hw = std::strstr(line, "headwave")) {
            float degrees = 0.0f, seconds = 2.0f;
            sscanf_s(hw + 8, "%f %f", &degrees, &seconds);
            camera::SetHeadWave(degrees, seconds);
            line = next;
            continue;
        }
        if (const char* rb = std::strstr(line, "rifleshotby")) {  // (before "rifleshot", which it contains)
            float by[3] = {};
            sscanf_s(rb + 11, "%f %f %f", &by[0], &by[1], &by[2]);
            for (int i = 0; i < 3; ++i)
                g_rifleShotBy[i] = by[i];
            line = next;
            continue;
        }
        if (const char* rs = std::strstr(line, "rifleshot")) {
            int on = 1;
            sscanf_s(rs + 9, "%d", &on);
            g_rifleShotOn = on != 0;
            line = next;
            continue;
        }
        if (std::strstr(line, "armbones")) {
            g_armBonesLog = true;
            line = next;
            continue;
        }
        if (std::strstr(line, "armdump")) {
            g_armDumpWanted = true;
            line = next;
            continue;
        }
        if (const char* an = std::strstr(line, "armnudge")) {
            int bone = -1, drawCount = 2016;
            sscanf_s(an + 8, "%d %d", &bone, &drawCount);
            g_armNudge = bone;
            g_armNudgeCount = drawCount;
            line = next;
            continue;
        }
        if (const char* lc = std::strstr(line, "launcher")) {  // DrawLauncher
            int on = 1;
            sscanf_s(lc + 8, "%d", &on);
            g_launcherOn = on != 0;
            Log("Launcher: %s", on ? "in the right hand, the game's own hold and reload" : "as the game places it");
            line = next;
            continue;
        }
        if (std::strstr(line, "gunpartlog")) {
            g_gunPartLog = true;
            line = next;
            continue;
        }
        if (std::strstr(line, "partpin")) {
            line = next;
            continue;
        }
        if (const char* ps = std::strstr(line, "partshift")) {
            PartShift s = {};
            if (sscanf_s(ps + 9, "%u %d %f %f %f", &s.count, &s.bone, &s.by[0], &s.by[1], &s.by[2]) == 5 && partShifts < static_cast<int>(std::size(g_partShift)))
                g_partShift[partShifts++] = s;
            line = next;
            continue;
        }
        if (const char* at = std::strstr(line, "armtrace")) {
            int frames = 60;
            sscanf_s(at + 8, "%d", &frames);
            g_armTrace = frames;
            line = next;
            continue;
        }
        if (const char* ds = std::strstr(line, "drawscan")) {  // ScanOwnDraw
            int frames = 60;
            sscanf_s(ds + 8, "%d", &frames);
            g_drawScan = frames;
            line = next;
            continue;
        }
        if (const char* ar = std::strstr(line, "allrec")) {  // "reloadrec", every draw with the player's own bones
            int seconds = 8;
            sscanf_s(ar + 6, "%d", &seconds);
            g_drawScanRec = true;
            StartRecording(std::clamp(seconds, 1, 30));
            line = next;
            continue;
        }
        if (const char* rr = std::strstr(line, "reloadrec")) {  // the game's own pistol reload recorded
            int seconds = 6;
            sscanf_s(rr + 9, "%d", &seconds);
            StartRecording(std::clamp(seconds, 1, 30));
            line = next;
            continue;
        }
        if (const char* pt = std::strstr(line, "pistoltest")) {  // the stand-in left hand on the pistol
            int on = 1;
            sscanf_s(pt + 10, "%d", &on);
            g_pistolTest = on != 0;
            Log("Bare hands: the stand-in left hand %s", on ? "holds the pistol (two-handed test)" : "lets go of the pistol");
            line = next;
            continue;
        }
        if (const char* pf = std::strstr(line, "pistolfingers2")) {  // the same, the left hand on the pistol too
            float m = 8.0f, r = 25.0f, l = 25.0f;
            sscanf_s(pf + 14, "%f %f %f", &m, &r, &l);
            g_pistolOpen2[0] = m, g_pistolOpen2[1] = r, g_pistolOpen2[2] = l;
            Log("Bare hands: on the pistol in both hands the middle, ring and little fingers opened %.0f, %.0f and %.0f "
                "degrees",
                m, r, l);
            line = next;
            continue;
        }
        if (const char* pf = std::strstr(line, "pistolfingers")) {  // the right hand's last three fingers opened
            float m = 25.0f, r = 25.0f, l = 25.0f;
            sscanf_s(pf + 13, "%f %f %f", &m, &r, &l);
            g_pistolOpen[0] = m, g_pistolOpen[1] = r, g_pistolOpen[2] = l;
            Log("Bare hands: on the pistol in one hand the middle, ring and little fingers opened %.0f, %.0f and %.0f "
                "degrees",
                m, r, l);
            line = next;
            continue;
        }
        if (const char* pl = std::strstr(line, "pslog")) {
            unsigned long long sum = 0;
            sscanf_s(pl + 5, "%llx", &sum);
            g_psLogSum = sum;
            g_psLogFrames = 3;
            Log("PS log: the next three frames' draws with pixel shader %016llx", sum);
            line = next;
            continue;
        }
        if (std::strstr(line, "fpunknown")) {
            {
                std::lock_guard lock(g_fpProbeMutex);
                g_fpProbeSeen.clear();
            }
            g_fpProbeFrames = 3;
            Log("First-person probe: the next three frames' first-person draws the mod leaves in place");
            line = next;
            continue;
        }
        if (const char* sb = std::strstr(line, "scopeboost")) {
            UINT v[2 * kBoostRules] = {};
            const int n = sscanf_s(sb + 10, "%u %u %u %u %u %u %u %u", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5],
                                   &v[6], &v[7]);
            for (size_t r = 0; r < kBoostRules; ++r)  // none read: none
                boost[r] = static_cast<int>(2 * r + 1) < n ? v[2 * r] << 8 | std::min(v[2 * r + 1], 8u) : 0;
            line = next;
            continue;
        }
        if (std::strstr(line, "scopetrace")) {
            ResetScopeMaterials();
            g_scopeTraceFrames = 3;
            Log("Scope trace: the next three frames' draws of the scope's mesh");
            line = next;
            continue;
        }
        if (const char* sd = std::strstr(line, "sound")) {
            char name[32] = {};
            if (sscanf_s(sd + 5, "%31s", name, static_cast<unsigned>(sizeof(name))) == 1)
                sounds::Play(name, 0.3f);
            line = next;
            continue;
        }
        if (const char* f = std::strstr(line, "floor")) {
            sscanf_s(f + 5, "%f", &floor);
            line = next;
            continue;
        }
        if (const char* d = std::strstr(line, "dump")) {
            dump = true;
            UINT count = 0, start = 0;
            // "dumpown": only the player's own, another soldier's parts share these numbers.
            const bool own = std::strncmp(d + 4, "own", 3) == 0;
            g_dumpOwn = own;
            dumpKey = sscanf_s(d + (own ? 7 : 4), "%u %u", &count, &start) == 2 ? DrawKey{count, start, 0} : kWeapon;
            line = next;
            continue;
        }
        watch |= std::strstr(line, "watch") != nullptr;
        if (std::strstr(line, "trace")) {
            std::lock_guard lock(g_traceMutex);
            g_trace.clear();
            g_tracing = true;
            line = next;
            continue;
        }
        size_t pass = 0;
        const char* list = line;
        if (const char* p = std::strstr(line, "pass")) {
            pass = std::strtoul(p + 4, nullptr, 10);
            const char* colon = std::strchr(p, ':');
            list = colon ? colon + 1 : "";
            if (pass >= kWeaponPassCount)
                list = "";
        } else {
            hide |= std::strstr(line, "hide") != nullptr;
            if (char* s = std::strstr(line, "shift")) {
                sscanf_s(s + 5, "%f %f %f", &shift[0], &shift[1], &shift[2]);
                *s = 0;
            }
        }
        for (const char* p = list; *p;) {
            if (*p < '0' || *p > '9') {
                ++p;
                continue;
            }
            char* end = nullptr;
            long a = std::strtol(p, &end, 10), b = a;
            p = end;
            if (*p == '-') {
                b = std::strtol(p + 1, &end, 10);
                p = end;
            }
            for (long k = a; k <= b && k < 64; ++k)
                if (k >= 0)
                    selected[pass].bones[k] = selected[pass].any = any = true;
        }
        line = next;
    }
    {
        std::lock_guard lock(g_selMutex);
        std::copy(std::begin(selected), std::end(selected), std::begin(g_weaponSelected));
        std::memcpy(g_weaponOffset, shift, sizeof(shift));
        g_weaponHide = hide;
    }
    g_weaponShift = any;
    g_weaponWatch = watch;
    g_partShifts = partShifts;
    if (dump) {
        g_dumpKey = dumpKey;
        g_dumpWanted = true;
    }
    if (floor != g_floor.load())
        Log("Weapon: the floor for a dropped magazine is at %.2f m", floor);
    g_floor = floor;
    camera::SetWeaponTest(wcamSet ? wcam : nullptr);
    g_weaponTesting = wcamSet;
    effects::SetHidden(hideFx, std::max(hideCount, 0));
    effects::SetMoved(moveFx, std::max(moveCount, 0));
    effects::SetHiddenShaders(hidePs, std::max(hidePsCount, 0));
    effects::SetMoveMatrices(flashMats);
    effects::SetLift(flashLift);
    reload::TestPull(testPull);

    input::SetZoomHold(zoomHold != 0);
    overlay::SetHudFix(hudFix);
    overlay::SetHudSkip(hudSkip);
    overlay::SetHudSkipTextures(hudSkipTex);
    overlay::SetKeepOrange(keepOrange != 0);
    shaders::SetLensFlare(keepFlare != 0, flarePs, flarePsCount);
    effects::SetSunGlare(sunGlare);
    overlay::SetKeepCrosshair(keepCrosshair != 0);
    hands::SetSteerCone(steerCone[0], steerCone[1]);
    views::SetOffCentre(offCentre[0], offCentre[1]);
    camera::SetTestFov(testFov);
    views::SetDetailFov(lodFov);
    scopeview::SetEyesShadowSize(shadowRes);
    scopeview::SetEyesBlurTest(noBlur);
    postpin::SetTest(postPin);
    input::SetTriggerTest(std::clamp(fingerTest, 0.0f, 1.0f));
    effects::SetCasings(casings != 0, casingReach[0], casingReach[1]);
    aim::SetFind(aimFind != 0);
    if (!sawQte)
        input::SetQte(2);
    if (!sawQteMash)
        input::SetQteMash(true);
    if (!sawQteTest)
        input::StartQteTest(0.0f);
    if (!sawQteFight)
        input::SetQteFight(true);
    if (g_heldShotOn.exchange(heldShot != 0) != (heldShot != 0))
        Log("Held gun: its bullets %s", heldShot ? "on its sight's line (through its glass, a sight of the table on it); its data looked for again after a take"
                                                 : "on its own sights' line whatever sight is on it, as before (heldshot 0)");
    if (g_handleRuns.exchange(handleRuns) != handleRuns)
        Log("Held gun: a handle with no bone of its own (the Mk11's) %s", handleRuns >= 2 ? "drawn with the hand, its shaft out of the receiver behind it (handleruns 2)"
                                                                          : handleRuns == 1 ? "drawn with the hand, without its shaft (handleruns 1)"
                                                                                            : "left home as the game draws it, as before (handleruns 0)");
    for (size_t r = 0; r < kBoostRules; ++r)
        if (const uint32_t was = g_scopeBoost[r].exchange(boost[r]); was != boost[r])
            Log("Weapon: the scope's %u-index part drawn %u more times on itself", (boost[r] ? boost[r] : was) >> 8,
                boost[r] & 0xFF);
    if (g_wiggleBone.exchange(wiggle) != wiggle)
        Log("Weapon: %s", wiggle >= 0 ? "a bone slides to and fro (wiggle)" : "no bone wiggles");
    Log("Weapon: bones %s (in %d passes), %s; watch %s%s", any ? "chosen" : "none",
        static_cast<int>(std::count_if(std::begin(selected), std::end(selected),
                                       [](const PassSelection& s) { return s.any; })),
        hide ? "hidden" : "shifted", watch ? "on" : "off", dump ? "; dumping the gun's mesh" : "");
    if (any && !hide)
        Log("Weapon: shift (%.2f, %.2f, %.2f)", shift[0], shift[1], shift[2]);
    scopeview::SettingsRead();
}

thread_local ID3D11Buffer* t_shiftBuffer = nullptr;  // this thread's stand-in for BF3's buffer
thread_local UINT t_shiftSize = 0;

void PublishHidden(std::vector<DrawKey>::const_iterator begin, std::vector<DrawKey>::const_iterator end) {
    g_hidden = new DrawSet(begin, end);
}

// The pointer each in-progress Map wrote to; Map and Unmap of one buffer happen on one thread.
struct PendingMap {
    ID3D11Resource* resource;
    void* data;
    UINT size;
    bool fresh;  // WRITE_DISCARD: the whole buffer is new, nothing in it is in use by the GPU
};
thread_local PendingMap t_pending[4] = {};

// ---- The game's uploads into memory that reads slowly (AMD) ----
// The pointer Map gives for a dynamic constant buffer can be write-combined, CPU-visible video memory (AMD's cards, above all with Smart
// Access Memory): writes are fast, but every read crosses PCIe uncached. An RX 7900 XT on 0.1.3: 0.79 us a 4-byte read, and the mod's reads
// at Unmap (the first-person FOV scan, the vignette's copy, the bones' copy) took about 5 s a frame. There the game writes into a copy of
// the mod's own (cached), which the mod reads and changes at Unmap as before and then copies into the card's memory in one pass of writes
// (RenderDoc's and 3DMigoto's way). Decided from the first 8 maps: the pages' protection (write-combined or not cached) or 64 reads timed;
// bf3vr.ini [render] upload_copy on/off overrides it. Off (an NVIDIA card's cached memory): nothing changes, the driver's pointer as before.
// A copy is found at Unmap by its context and buffer on whatever thread, so a map's bytes always reach the card; WRITE_DISCARD maps only
// (the whole buffer new), never a ring buffer's NO_OVERWRITE append.
std::atomic<int> g_uploadCopy{-1};  // -1 undecided, 0 off, 1 on
struct ShadowEntry {
    ID3D11DeviceContext* ctx = nullptr;
    ID3D11Resource* resource = nullptr;  // in use while set
    void* real = nullptr;
    uint8_t* buf = nullptr;
    UINT cap = 0, size = 0;
};
constexpr int kShadowEntries = 64;
ShadowEntry g_shadowEntries[kShadowEntries];
SRWLOCK g_shadowLock = SRWLOCK_INIT;
std::mutex g_probeMutex;
int g_probes = 0;
double g_probeBest = 1e9;
DWORD g_probeProtect = 0;
std::atomic<long long> g_shadowFull{0};

// The second way of the draw hooks (the contexts' tables): a table may have Map swapped and not Unmap, so a copy could never reach the
// card there: never a copy on that way (set where InstallHooks chooses it).
std::atomic<bool> g_secondWay{false};
std::atomic<unsigned> g_probeTick{0};
bool g_uploadForced = false;
constexpr double kSlowReadUs = 2.0;  // 64 reads of the mapped pointer slower than this: memory that reads slowly (NVIDIA's cached: ~0.3 us)

// One look at a mapped pointer: its pages' protection, and 64 reads timed, the best of 3 passes (us).
void ProbeOnce(void* p, UINT size, DWORD& protect, double& us) {
    MEMORY_BASIC_INFORMATION mbi{};
    protect = VirtualQuery(p, &mbi, sizeof(mbi)) ? mbi.Protect : 0;
    LARGE_INTEGER f, a, b;
    QueryPerformanceFrequency(&f);
    const volatile uint32_t* q = static_cast<const volatile uint32_t*>(p);
    const UINT n = std::min<UINT>(size / 4, 64);
    us = 1e9;
    for (int pass = 0; pass < 3 && n; ++pass) {
        uint32_t sum = 0;
        QueryPerformanceCounter(&a);
        for (UINT i = 0; i < n; ++i)
            sum += q[i];
        QueryPerformanceCounter(&b);
        (void)sum;
        us = std::min(us, double(b.QuadPart - a.QuadPart) * 1e6 / double(f.QuadPart) * 64.0 / n);
    }
    if (!n)
        us = 0.0;
}

// Whether this WRITE_DISCARD map's bytes go through a copy of the mod's own: decided on the first 8 maps, and while it reads fast, looked
// at again every 512th map (memory that turns slow later, a level's buffers or the driver moving them, is caught; never back to off).
bool UploadCopyOn(void* p, UINT size) {
    const int v = g_uploadCopy.load(std::memory_order_relaxed);
    if (v == 1)
        return true;
    if (g_secondWay.load(std::memory_order_relaxed))
        return false;
    if (v == 0 && (g_uploadForced || (g_probeTick.fetch_add(1, std::memory_order_relaxed) & 511u) != 0))
        return false;
    if (v < 0) {
        const int forced = config::Get().uploadCopy;
        if (forced) {
            std::lock_guard lock(g_probeMutex);
            if (g_uploadCopy.load() < 0) {
                g_uploadForced = true;
                g_uploadCopy.store(forced == 2 ? 1 : 0);
                Log("Skin: the game's uploads %s (bf3vr.ini [render] upload_copy %s)",
                    forced == 2 ? "written into the mod's own copy of each, which goes to the card at Unmap" : "read where the driver put them",
                    forced == 2 ? "on" : "off");
            }
            return g_uploadCopy.load() == 1;
        }
    }
    DWORD protect = 0;
    double us = 0.0;
    ProbeOnce(p, size, protect, us);
    const bool flagged = (protect & (PAGE_WRITECOMBINE | PAGE_NOCACHE)) != 0;
    std::lock_guard lock(g_probeMutex);
    const int now = g_uploadCopy.load();
    if (now == 1)
        return true;
    if (now == 0) {
        if (!flagged && us <= kSlowReadUs)
            return false;
        g_uploadCopy.store(1);
        Log("Skin: the game's uploads read slowly now (their pages %s, protection 0x%X; 64 reads in %.2f us): from here the game writes "
            "into the mod's own copy of each, which goes to the card at Unmap (AMD's video memory)",
            flagged ? "write-combined or not cached" : "cached", static_cast<unsigned>(protect), us);
        return true;
    }
    g_probeBest = std::min(g_probeBest, us);
    g_probeProtect |= protect;
    if (++g_probes < 8)
        return false;  // read in place until decided
    const bool anyFlagged = (g_probeProtect & (PAGE_WRITECOMBINE | PAGE_NOCACHE)) != 0;
    const bool slow = anyFlagged || g_probeBest > kSlowReadUs;
    g_uploadCopy.store(slow ? 1 : 0);
    Log("Skin: the game's uploads %s (their pages %s, protection 0x%X; 64 reads in %.2f us at the best of 8)",
        slow ? "read slowly: the game writes into the mod's own copy of each, which goes to the card at Unmap (AMD's video memory)"
             : "read fast: read where the driver put them, as before (looked at again every 512th upload)",
        anyFlagged ? "write-combined or not cached" : "cached", static_cast<unsigned>(g_probeProtect), g_probeBest);
    return slow;
}

// The mod's copy for one map, or nullptr (all 64 in use: the driver's pointer as before).
uint8_t* ShadowBegin(ID3D11DeviceContext* ctx, ID3D11Resource* resource, void* real, UINT size) {
    uint8_t* buf = nullptr;
    AcquireSRWLockExclusive(&g_shadowLock);
    ShadowEntry* pick = nullptr;
    for (ShadowEntry& e : g_shadowEntries) {
        if (e.resource)
            continue;
        if (!pick || (e.cap >= size && (pick->cap < size || e.cap < pick->cap)))
            pick = &e;  // the smallest free one that fits, else any free one (grown)
    }
    if (pick) {
        if (pick->cap < size) {
            _aligned_free(pick->buf);
            pick->cap = (size + 255u) & ~255u;
            pick->buf = static_cast<uint8_t*>(_aligned_malloc(pick->cap, 64));
            if (!pick->buf)
                pick->cap = 0;
        }
        if (pick->buf) {
            *pick = {ctx, resource, real, pick->buf, pick->cap, size};
            buf = pick->buf;
        }
    }
    ReleaseSRWLockExclusive(&g_shadowLock);
    if (!buf)
        g_shadowFull.fetch_add(1, std::memory_order_relaxed);
    return buf;
}

// At Unmap: the copy's bytes (the mod's changes in them) into the card's memory, the entry free again.
void ShadowEnd(ID3D11DeviceContext* ctx, ID3D11Resource* resource) {
    ShadowEntry* found = nullptr;
    AcquireSRWLockShared(&g_shadowLock);
    for (ShadowEntry& e : g_shadowEntries) {
        if (e.resource == resource && e.ctx == ctx) {
            found = &e;
            break;
        }
    }
    ReleaseSRWLockShared(&g_shadowLock);
    if (!found)
        return;
    std::memcpy(found->real, found->buf, found->size);  // (outside the lock: the entry stays taken until it's freed below)
    AcquireSRWLockExclusive(&g_shadowLock);
    found->resource = nullptr;
    found->ctx = nullptr;
    found->real = nullptr;
    ReleaseSRWLockExclusive(&g_shadowLock);
}

void BeepAsync(DWORD frequency, DWORD duration) {
    DWORD packed = (frequency << 16) | duration;
    if (HANDLE t = CreateThread(nullptr, 0, [](LPVOID p) -> DWORD {
            DWORD v = static_cast<DWORD>(reinterpret_cast<uintptr_t>(p));
            Beep(v >> 16, v & 0xFFFF);
            return 0;
        }, reinterpret_cast<LPVOID>(static_cast<uintptr_t>(packed)), 0, nullptr))
        CloseHandle(t);
}

float Dot3(const float* a, const float* b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// Three rotation rows: unit length (a little scale allowed) and at right angles.
bool RotationRows(const float* a, const float* b, const float* c) {
    for (const float* v : {a, b, c}) {
        float len = Dot3(v, v);
        if (!(len > 0.8f && len < 1.25f))  // also rejects NaN
            return false;
    }
    return std::fabs(Dot3(a, b)) < 0.05f && std::fabs(Dot3(a, c)) < 0.05f && std::fabs(Dot3(b, c)) < 0.05f;
}

bool IsMatrix(const float* m, UINT layout) {
    if (layout == 0)
        return RotationRows(m, m + 4, m + 8);
    return RotationRows(m, m + 4, m + 8) && m[3] == 0.0f && m[7] == 0.0f && m[11] == 0.0f && m[15] == 1.0f;
}

void Translation(const float* m, UINT layout, float* out) {
    if (layout == 0)
        out[0] = m[3], out[1] = m[7], out[2] = m[11];
    else
        out[0] = m[12], out[1] = m[13], out[2] = m[14];
}

// Every row of bone matrices in an upload.
void FindRows(const float* data, UINT size, std::vector<Signature>& out) {
    UINT floats = size / 4;
    for (UINT layout = 0; layout < 2; ++layout) {
        UINT stride = layout == 0 ? 12 : 16;
        for (UINT i = 0; i + stride <= floats;) {
            UINT count = 0;
            while (i + (count + 1) * stride <= floats && IsMatrix(data + i + count * stride, layout))
                ++count;
            if (count >= kMinMatrices)
                out.push_back({size, i * 4, count, layout});
            i += count ? count * stride : 4;
        }
    }
}

// Looks at `size` bytes of an upload: a whole buffer filled with WRITE_DISCARD (fresh), or just
// the bytes BF3 appended to a ring buffer with WRITE_NO_OVERWRITE. Either way nothing in it has
// reached the GPU yet, so it's safe to change.
void Inspect(void* data, UINT size, bool fresh) {
    if (size < kMinMatrices * 48)
        return;
    std::vector<Signature> rows;
    FindRows(static_cast<float*>(data), size, rows);
    if (rows.empty())
        return;
    bool lift = g_lift.load(std::memory_order_relaxed);
    std::lock_guard lock(g_mutex);
    UINT largest = 0;
    for (const Signature& s : rows) {
        float* m = static_cast<float*>(data) + s.start / 4;
        UINT stride = s.layout == 0 ? 12 : 16;
        float mean[3] = {};
        for (UINT k = 0; k < s.count; ++k) {
            float t[3];
            Translation(m + k * stride, s.layout, t);
            for (int i = 0; i < 3; ++i)
                mean[i] += t[i] / s.count;
        }
        float toCamera[3] = {mean[0] - g_camera[0], mean[1] - g_camera[1], mean[2] - g_camera[2]};
        float dCamera = std::sqrt(Dot3(toCamera, toCamera));
        float dOrigin = std::sqrt(Dot3(mean, mean));
        bool cameraRelative = dOrigin < kViewModel;
        if (s.count > largest) {
            largest = s.count;
            t_bones.count = s.count;
            t_bones.layout = s.layout;
            t_bones.size = size;
            t_bones.start = s.start;
            t_bones.fresh = fresh;
            std::memcpy(t_bones.first, m, sizeof(t_bones.first));
            std::memcpy(t_bones.mean, mean, sizeof(t_bones.mean));
            t_bones.dCamera = dCamera;
            t_bones.dOrigin = dOrigin;
            t_drawsSinceBones = 0;
        }
        Seen& seen = g_seen[s];
        ++seen.uploads;
        seen.viewUploads += cameraRelative;
        seen.appended += !fresh;
        seen.nearestCamera = std::min(seen.nearestCamera, dCamera);
        seen.nearestOrigin = std::min(seen.nearestOrigin, dOrigin);
        if (dCamera < 3.0f) {
            Cell cell{static_cast<int>(std::floor(Dot3(toCamera, g_axes[0]) / kBucket)),
                      static_cast<int>(std::floor(Dot3(toCamera, g_axes[1]) / kBucket)),
                      static_cast<int>(std::floor(Dot3(toCamera, g_axes[2]) / kBucket)), s.layout, !fresh};
            ++g_cells[cell];
        }

        // Camera-relative pieces in any upload; appended rows near the camera in the world too
        // (the first round, which moved the gun, had lifted appended data).
        if (lift && (cameraRelative || (!fresh && dCamera < 3.0f)))
            for (UINT k = 0; k < s.count; ++k)
                m[k * stride + (s.layout == 0 ? 7 : 13)] += kLift;  // vertical translation
    }
}

HRESULT Map(int which, ID3D11DeviceContext* ctx, ID3D11Resource* resource, UINT sub, D3D11_MAP type, UINT flags,
            D3D11_MAPPED_SUBRESOURCE* mapped) {
    HRESULT hr = g_origMap[which](ctx, resource, sub, type, flags, mapped);
    if (SUCCEEDED(hr))
        postpin::NoteMap(ctx, resource);  // any refill, seen below or not, ends the vignette's copy of that buffer on this context (postpin.h)
    if (FAILED(hr) || !mapped || (type != D3D11_MAP_WRITE_DISCARD && type != D3D11_MAP_WRITE_NO_OVERWRITE) || scopeview::Drawing())
        return hr;
    D3D11_RESOURCE_DIMENSION dim;
    resource->GetType(&dim);
    if (dim != D3D11_RESOURCE_DIMENSION_BUFFER)
        return hr;
    D3D11_BUFFER_DESC desc;
    static_cast<ID3D11Buffer*>(resource)->GetDesc(&desc);
    if (!(desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER)) {
        effects::NoteMapped(resource, mapped->pData);  // weapon.txt "flashsurvey"
        return hr;
    }
    for (PendingMap& p : t_pending) {
        if (!p.resource) {
            // Memory that reads slowly (AMD): the game writes into the mod's own copy, read below and at Unmap, then copied to the card.
            if (type == D3D11_MAP_WRITE_DISCARD && UploadCopyOn(mapped->pData, desc.ByteWidth))
                if (uint8_t* copy = ShadowBegin(ctx, resource, mapped->pData, desc.ByteWidth))
                    mapped->pData = copy;
            p = {resource, mapped->pData, desc.ByteWidth, type == D3D11_MAP_WRITE_DISCARD};
            break;
        }
    }
    return hr;
}

// ---- First-person FOV ----
// With the render FOV widened (camera.cpp), the gun and arms were still drawn at the game's own
// 55 degrees, shown life-size twice too big; writing the render object's first-person view
// changed its matrices but not the gun. So the fix is made in what reaches the graphics card: in
// every constant buffer upload, a matrix that projects with the game's own FOV is rescaled to the
// wide one. Such a matrix (projection, or view times projection) has the x and y columns of its
// rotation part scaled by 1/tan(FOV/2) and a unit w column; stored transposed, the same in rows.
// Scaling those two columns (all four rows) by wide/game gives the wide projection exactly.
std::atomic<float> g_gameScale{0.0f}, g_wideScale{0.0f};  // 1/tan(FOV/2); 0 = off
// Matrices rescaled since the last log, by where they came from: a small buffer filled whole,
// a ring buffer filled whole, or bytes appended to a ring buffer.
enum UploadKind { kSmall, kRingWhole, kRingAppend, kUploadKinds };
std::atomic<uint32_t> g_rescaled[kUploadKinds];

bool Near(float a, float b) {
    return std::fabs(a - b) < 0.002f * b;  // also rejects NaN
}

float Length3(float x, float y, float z) {
    return std::sqrt(x * x + y * y + z * z);
}

std::atomic<int64_t> g_rescaleTicks{0};  // time spent scanning, for the log
// Uploads scanned and matched, by size in 256-byte steps (the last bucket: 16 KB and up).
constexpr int kSizeBuckets = 65;
std::atomic<uint32_t> g_scannedBySize[kSizeBuckets], g_matchedBySize[kSizeBuckets];

int SizeBucket(UINT size) {
    return std::min(static_cast<int>(size / 256), kSizeBuckets - 1);
}
// Scanning every upload cost about 2 ms a frame (85,000 uploads a second), most of them sizes that
// never held a match (the 4 KB bone blocks, 1-1.5 KB blocks). So only sizes that have matched
// are scanned, plus everything for one frame every two seconds, which finds new ones.
std::atomic<bool> g_sizeMatched[kSizeBuckets];
std::atomic<bool> g_scanAll{true};

// The gun's and arms' projections are made apart from the camera blocks, with a near plane of their own: on the one the
// eyes have (views.h NearPlane), or the gun is all further off than the world around it and goes behind it. Depth is
// z = a w + b; a matrix with b for the game's 6 cm has b moved, whichever matrix: the rows z, w start at m + zAt, m + wAt, the step between a row's lanes being step.
inline void NearerPlane(float* m, int zAt, int wAt, int step) {
    const float wanted = views::NearPlane();
    if (!(wanted > 0.0f))
        return;
    int big = 0;
    for (int k = 1; k < 3; ++k)
        if (std::fabs(m[wAt + k * step]) > std::fabs(m[wAt + big * step]))
            big = k;
    if (std::fabs(m[wAt + big * step]) < 0.5f)
        return;
    const float a = m[zAt + big * step] / m[wAt + big * step];
    const float b = m[zAt + 3 * step] - a * m[wAt + 3 * step];
    if (!(a > 0.99f && a < 1.01f) || std::fabs(b + 0.06f * a) > 0.002f)
        return;
    m[zAt + 3 * step] += (0.06f - wanted) * a;
}

void RescaleProjections(uint8_t* data, UINT size, UploadKind kind) {
    float game = g_gameScale.load(std::memory_order_relaxed), wide = g_wideScale.load(std::memory_order_relaxed);
    if (!(game > 0.0f && wide > 0.0f) || Near(wide, game))
        return;
    int bucket = SizeBucket(size);
    if (!g_sizeMatched[bucket].load(std::memory_order_relaxed) && !g_scanAll.load(std::memory_order_relaxed))
        return;
    LARGE_INTEGER t0, t1;
    QueryPerformanceCounter(&t0);
    g_scannedBySize[bucket].fetch_add(1, std::memory_order_relaxed);
    float k = wide / game;
    // Runs on every constant buffer upload, thousands a frame: squared lengths first, and only
    // a near match pays for the rest.
    float lo = game * game * 0.995f, hi = game * game * 1.005f;
    auto f = reinterpret_cast<float*>(data);
    UINT count = size / 4;
    // With the launcher in hand the game's zoom is held, and its first person is then drawn at a FOV of the weapon's own: that
    // view (352 bytes, at +176: NoteFirstPersonView) is widened from whatever it has.
    const bool anyFov = size == 352 && g_launcherOn.load(std::memory_order_relaxed) &&
                        DrawnLately(g_launcherAt, g_launcherFrame);
    for (UINT i = 0; i + 16 <= count; i += 4) {  // a matrix starts at any 16-byte register
        float* m = f + i;
        if (anyFov && i * 4 == 176) {
            const float scale = Length3(m[0], m[1], m[2]);
            if (scale > 1.5f && scale < 8.0f && Near(Length3(m[4], m[5], m[6]), scale) &&
                Near(Length3(m[12], m[13], m[14]), 1.0f)) {
                NoteFirstPersonView(m, size, i * 4);
                const float widen = wide / scale;
                for (int c = 0; c < 4; ++c)
                    m[c] *= widen, m[4 + c] *= widen;
                g_rescaled[kind].fetch_add(1, std::memory_order_relaxed);
                continue;
            }
        }
        float column = m[0] * m[0] + m[4] * m[4] + m[8] * m[8];
        float row = m[0] * m[0] + m[1] * m[1] + m[2] * m[2];
        if (!(column > lo && column < hi) && !(row > lo && row < hi))
            continue;
        bool watch = g_weaponWatch.load(std::memory_order_relaxed);
        if (Near(Length3(m[0], m[4], m[8]), game) && Near(Length3(m[1], m[5], m[9]), game) &&
            Near(Length3(m[3], m[7], m[11]), 1.0f)) {
            if (watch)
                NoteProjection(m, size, i * 4, true);
            for (int r = 0; r < 4; ++r)
                m[4 * r] *= k, m[4 * r + 1] *= k;
            NearerPlane(m, 2, 3, 4);
        } else if (Near(Length3(m[0], m[1], m[2]), game) && Near(Length3(m[4], m[5], m[6]), game) &&
                   Near(Length3(m[12], m[13], m[14]), 1.0f)) {
            if (watch)
                NoteProjection(m, size, i * 4, false);
            NoteFirstPersonView(m, size, i * 4);
            for (int c = 0; c < 4; ++c)
                m[c] *= k, m[4 + c] *= k;
            NearerPlane(m, 8, 12, 1);
            // Its depth's near and far, once: z = a w + b, whichever way round the game has it.
            if (static bool s_said = false; !s_said && std::fabs(m[12]) + std::fabs(m[13]) + std::fabs(m[14]) > 0.5f) {
                s_said = true;
                const float big = std::fabs(m[12]) > std::fabs(m[13]) ? (std::fabs(m[12]) > std::fabs(m[14]) ? m[12] : m[14]) : (std::fabs(m[13]) > std::fabs(m[14]) ? m[13] : m[14]);
                const float a = (std::fabs(m[12]) > std::fabs(m[13]) ? (std::fabs(m[12]) > std::fabs(m[14]) ? m[8] : m[10]) : (std::fabs(m[13]) > std::fabs(m[14]) ? m[9] : m[10])) / big;
                const float b = m[11] - a * m[15];
                Log("First-person view's depth: z = %.6f w + %.6f: depth 0 at %.3f m, depth 1 at %.1f m", a, b, std::fabs(a) > 1e-6f ? -b / a : 0.0f, std::fabs(1.0f - a) > 1e-6f ? b / (1.0f - a) : 0.0f);
            }
        } else {
            continue;
        }
        g_rescaled[kind].fetch_add(1, std::memory_order_relaxed);
        g_matchedBySize[bucket].fetch_add(1, std::memory_order_relaxed);
        g_sizeMatched[bucket].store(true, std::memory_order_relaxed);
        i += 12;  // past this matrix
    }
    QueryPerformanceCounter(&t1);
    g_rescaleTicks.fetch_add(t1.QuadPart - t0.QuadPart, std::memory_order_relaxed);
}

// Ring buffers: big constant buffers BF3 appends to with WRITE_NO_OVERWRITE between DISCARDs.
// Earlier parts may already be in use by the GPU, so only what was just written may be touched.
// A shadow copy of each ring buffer shows which bytes changed since the last upload.
constexpr UINT kMinRingSize = 16384;
std::mutex g_shadowMutex;
std::unordered_map<ID3D11Resource*, std::vector<uint8_t>> g_shadows;

// The changed bytes between data and shadow, in whole 16-byte registers: [begin, end).
bool ChangedRange(const uint8_t* data, const std::vector<uint8_t>& shadow, UINT size, UINT& begin, UINT& end) {
    UINT first = size, last = 0;
    for (UINT i = 0; i + 16 <= size; i += 16) {
        if (std::memcmp(data + i, shadow.data() + i, 16) != 0) {
            first = std::min(first, i);
            last = i + 16;
        }
    }
    begin = first;
    end = last;
    return first < last;
}

// ---- The body lock ----
// In a scripted picture (a cutscene's or a ride's camera turned to the head: camera.cpp, views.cpp) the game draws the
// player's own body (its first-person arms, gun and legs) with views of its own, which the head's turn wasn't in: the
// body moved with the head (the user, 2026-11-01, the drive of Comrades: "everywhere I look, I still see my knees and
// hands hovering"; turning the view description's first-person camera changed nothing). Each of its views is turned
// here as it's uploaded (viewturn.h LockBodyPerView, LockBodyWorldView), by the turn the view build put on the main
// camera, before anything else reads it, and drawn at the picture's field of view (viewturn.h WidenPerView,
// WidenViewProjection): at the game's FOV the FOV fix widens the turned one next, and the eyes' labels and the free
// hands' places come from it, all as if the game had built it turned; at another the lock widens it itself, the FOV
// fix finding nothing to widen (in the drive it found none at the game's 49.5, all the drive through, where in ordinary
// play the body comes at the game's 55: left so in the 100-degree picture, it was drawn 2.3 times too big about where
// the head looks, and off the scene as the head turned). A frame's uploads are turned by one build's turn, latched at
// its first (viewturn::BodyPick): the one the main view's own upload names (its turned camera's rows); before it has,
// the one as old as the last frame's main view named, given more than one (with none named all session, the newest,
// given three); else they're left as the game made them. Only views of the scripted camera's handedness are turned.
// Found by their shape, not by the game's FOV. weapon.txt "bodylock 0" leaves them as the game made them (g_bodyLock).
constexpr int kBodyLatched = 4;  // view builds' turns a frame looks among (camera.cpp keeps as many)
struct BodyLatch {
    uint32_t frame = 0xFFFFFFFFu;  // g_frameSerial it was taken at
    camera::BodyTurn builds[kBodyLatched];
    int count = 0;              // of them (0: no scripted picture)
    viewturn::BodyPick pick;    // the one this frame's views are turned by
    int lastNamed = -1;         // the age the last frame's main view named (the next frame's guess; -1 none yet)
    bool counted = false;       // the frame counted as named or guessed
    uint32_t turned = 0, turnedBefore = 0;  // the body's 352-byte views turned this frame so far, and the last frame
    double firstEye[3] = {};  // this frame's first one's eye
    // Which eye's pass each of the frame's turned body views falls in. The game draws one view, then the other (views.cpp
    // makes them the eyes), each pass starting with a body view before its main view's (bodytrace, 2026-11-01: five
    // body views a pass): a body view goes with the next main view's eye, which comes only after it, so each frame's
    // order is learned for the next, as NoteFirstPersonView learns the arms' eyes in ordinary play.
    double passAlong[2] = {}, lastPassAlong[2] = {};  // each pass's main view's eye along the turned right (m)
    int passes = 0, lastPasses = 0;                   // the passes seen this frame, and the last frame (0 to 2)
    int8_t order[32] = {}, lastOrder[32] = {};        // each body view's pass, this frame and the last
    uint32_t pendingFrom = 0;                         // body views from here on wait for the next main view's pass
    // The eyes' pictures drawn this frame so far, and the last frame (skin::NoteEyePicture): each ends its eye's pass,
    // so the body's views after the first are the second eye's, told in the frame itself (the learned order went
    // wrong where the number of body views changed from frame to frame, as at the drive's crash).
    int pictures = 0, lastPictures = 0;
};
// Since the last log (LogBodyLock), under g_bodyMutex.
struct BodyStats {
    uint32_t byEye[3] = {};  // the body's 352-byte views turned: eye not known, left, right
    uint32_t world = 0;      // its 80-byte view-projections in the world turned
    // Left as they were (viewturn::BodyVerdict).
    uint32_t main = 0, before = 0, away = 0, elsewhere = 0, otherHand = 0, unnamed = 0;
    // Frames turned by the build the main view named (by its age: the newest, the one before, ...: how far the frame
    // drawn is behind the views built), or with none named yet by the age the last frame's named (guessed), and of
    // those how many the main view then named another for (renamed: their first views turned a build off).
    uint32_t named[kBodyLatched] = {}, guessed[kBodyLatched] = {}, renamed = 0;
    // The body's views widened to the picture's field of view (viewturn::kFovWidened), and the fields of view they
    // came at (degrees, the lowest and the highest); left for the FOV fix at the game's; at the picture's already; left
    // at their own with no picture's to widen to (the FOV fix off).
    uint32_t widened = 0, atGame = 0, atPicture = 0, fovLeft = 0;
    float widenedLo = 0.0f, widenedHi = 0.0f;
    double spread = 0.0;  // the most a frame's body views' eyes were apart (m)
    // Turned by the build they were made in (viewturn::OwnBuild), of those a build off the frame's own pick, and the
    // 352-byte ones turned with their eye past the arms' space's reach (viewturn::kBodyReach).
    uint32_t own = 0, offPick = 0, perViewWorld = 0;
    // The body's 352-byte views moved to their eye's place (BodyLatch's passes), and the most the eyes were apart (m).
    uint32_t shifted = 0;
    double eyesApart = 0.0;
    bool loggedView = false, loggedOdd = false;  // this scripted run's first body view and odd upload logged
    bool loggedElsewhere = false;                // and its first 352-byte view in the world left as it was
    int namedLogged = 0, namedWorldLogged = 0;   // views of frames the main view named logged this second (LogLeftView)
};
std::mutex g_bodyMutex;
BodyLatch g_bodyLatch;
BodyStats g_bodyStats;
std::atomic<uint32_t> g_bodyNotPerView{0};  // 352-byte uploads not per-view constants, since the last log

float DegreesApart(const float* a, const float* b) {  // two directions, any length
    const float cross[3] = {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
    return std::atan2(Length3(cross[0], cross[1], cross[2]), a[0] * b[0] + a[1] * b[1] + a[2] * b[2]) * 57.29578f;
}

// A field of view in degrees from its scale (1/tan of half of it; 0 for none).
float FovDegrees(float scale) {
    return scale > 0.0f ? 2.0f * std::atan(1.0f / scale) * 57.29578f : 0.0f;
}

// A scripted run's first body view as the game uploaded it (made), to tell what it's made from: its eye in the arms'
// space, its forward from the scripted camera's and from the view's first-person camera's (+0x930) as the game made
// them, and from the turned main camera's; its field of view against the picture's and the game's own, and what the
// lock made of it (fov: viewturn::WidenPerView's); its 22 rows.
void LogBodyView(const float* made, const viewturn::BodyView& v, const camera::BodyTurn& build, viewturn::BodyFov fov) {
    const float* p = made + viewturn::kProjectionAt;
    const float wide = g_wideScale.load(std::memory_order_relaxed), game = g_gameScale.load(std::memory_order_relaxed);
    const bool firstPerson = Length3(build.firstPersonMade[6], build.firstPersonMade[7], build.firstPersonMade[8]) > 0.5f;
    char fromFirstPerson[48] = "no first-person camera";
    if (firstPerson)
        std::snprintf(fromFirstPerson, sizeof(fromFirstPerson), "%.2f", DegreesApart(v.rows + 6, build.firstPersonMade + 6));
    static const char* const kFov[] = {"left as it is: no picture's to widen it to", "the picture's already",
                                       "the game's: the FOV fix widens it", "widened here to the picture's"};
    Log("Camera: the body's first-person view as the game uploaded it (this scripted run's first): its eye (%.3f %.3f "
        "%.3f) in the arms' space; its forward %.2f degrees from the scripted camera's and %s from the view's "
        "first-person camera's (+0x930) as the game made them, %.2f from the turned main camera's; its field of view "
        "%.1f by %.1f degrees (the picture's %.1f, the game's own %.1f: %s); its projection's w row %+.0f",
        v.eye[0], v.eye[1], v.eye[2], DegreesApart(v.rows + 6, build.before + 6), fromFirstPerson,
        DegreesApart(v.rows + 6, build.after + 6), FovDegrees(p[0]), FovDegrees(p[5]), FovDegrees(wide),
        FovDegrees(game), kFov[fov], p[14]);
    for (int r = 0; r < 22; ++r)
        Log("Camera:   +%3d: %12.5f %12.5f %12.5f %12.5f", r * 16, made[4 * r], made[4 * r + 1], made[4 * r + 2],
            made[4 * r + 3]);
}

// A view in a frame the main view named, against each of the frame's builds (the shots the body went in, 2026-11-01):
// its eye and how far from each build's scripted camera; its rows' most off each build's camera as made (before), as
// turned (after), and its first-person camera as made and turned; for a 352-byte one its projection's scales.
void LogLeftView(const float* f, const viewturn::BodyView& v, const BodyLatch& l, bool perView, const char* what) {
    auto off = [&v](const float* rows) {
        float most = 0.0f;
        for (int i = 0; i < 9; ++i)
            most = std::max(most, std::fabs(v.rows[i] - rows[i]));
        return most;
    };
    char builds[4][160] = {};
    for (int b = 0; b < l.count && b < 4; ++b) {
        const camera::BodyTurn& t = l.builds[b];
        const double d[3] = {v.eye[0] - t.place[0], v.eye[1] - t.place[1], v.eye[2] - t.place[2]};
        std::snprintf(builds[b], sizeof(builds[b]), " [%d: %.2f m; off made %.4f, turned %.4f, fp made %.4f, fp turned %.4f]",
                      b, std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]), off(t.before), off(t.after),
                      off(t.firstPersonMade), off(t.firstPerson));
    }
    char projection[64] = "";
    if (perView)
        std::snprintf(projection, sizeof(projection), "; its projection's scales %.3f %.3f, z %+.0f",
                      f[viewturn::kProjectionAt], f[viewturn::kProjectionAt + 5], v.zSign);
    Log("Camera: %s in a frame the main view named (build %d): its eye (%.2f %.2f %.2f)%s;%s%s%s%s", what, l.pick.use,
        v.eye[0], v.eye[1], v.eye[2], projection, builds[0], builds[1], builds[2], builds[3]);
}

// A new frame for the body lock (under g_bodyMutex), once a frame: its builds latched, the last frame's passes kept.
void BeginBodyFrame(BodyLatch& l) {
    const uint32_t frame = g_frameSerial.load(std::memory_order_relaxed);
    if (l.frame == frame)
        return;
    l.frame = frame;
    l.count = camera::BodyTurns(l.builds, kBodyLatched);
    l.pick = viewturn::BodyPick{};
    l.pick.guess = l.lastNamed;
    l.counted = false;
    // The frame before's body views after its last main view's go with that one's pass; its order and its passes'
    // places are the new frame's to go by.
    for (uint32_t i = l.pendingFrom; i < l.turned && i < 32; ++i)
        l.order[i] = static_cast<int8_t>(l.passes > 0 ? l.passes - 1 : 0);
    std::memcpy(l.lastOrder, l.order, sizeof(l.order));
    std::memcpy(l.lastPassAlong, l.passAlong, sizeof(l.passAlong));
    l.lastPasses = l.passes;
    l.passes = 0;
    l.pendingFrom = 0;
    l.lastPictures = l.pictures;
    l.pictures = 0;
    l.turnedBefore = l.turned;
    l.turned = 0;
}

// The body's views turned with the head as they're uploaded, in a scripted picture (above). data: a small upload of
// size bytes, before the FOV fix.
void LockBody(uint8_t* data, UINT size) {
    if (!g_bodyLock.load(std::memory_order_relaxed) || !camera::BodyTurnWanted())
        return;
    auto* f = reinterpret_cast<float*>(data);
    const bool perView = size == viewturn::kPerViewBytes;
    // A first look without the lock: per-view constants have a view's rows at +48 and a view-projection's w row (the
    // forward) at +224, an 80-byte view-projection that w row at +48, each unit length.
    auto unit = [](const float* r) { return std::fabs(r[0] * r[0] + r[1] * r[1] + r[2] * r[2] - 1.0f) < 4e-3f; };
    const bool forward = unit(f + (perView ? viewturn::kViewProjectionAt + 12 : 12));
    if (!(forward || (perView && unit(f + viewturn::kViewAt)))) {
        if (perView)
            g_bodyNotPerView.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    std::lock_guard lock(g_bodyMutex);
    BodyLatch& l = g_bodyLatch;
    BodyStats& s = g_bodyStats;
    BeginBodyFrame(l);
    float made[viewturn::kPerViewBytes / 4] = {};
    const bool keep = perView && !(s.loggedView && s.loggedOdd);
    if (keep)
        std::memcpy(made, f, sizeof(made));
    viewturn::BodyView v;
    viewturn::PerViewFault fault = viewturn::kPerViewOk;
    const viewturn::BodyPick was = l.pick;
    const viewturn::BodyVerdict verdict = perView ? viewturn::LockBodyPerView(f, l.builds, l.count, l.pick, v, fault)
                                                  : viewturn::LockBodyWorldView(f, l.builds, l.count, l.pick, v);
    if (verdict != viewturn::kNotView && l.count > 0 && g_bodyTrace.load(std::memory_order_relaxed) > 0) {
        --g_bodyTrace;
        static const char* const kVerdict[] = {"not a view", "the main view's", "turned before", "away", "elsewhere",
                                               "the other hand's", "unnamed", "the body's, turned"};
        const float* right = l.builds[0].after;  // the newest build's camera as turned: its right
        const double along = v.eye[0] * right[0] + v.eye[1] * right[1] + v.eye[2] * right[2];
        Log("Body trace: frame %u, %u bytes, %s, its eye (%.4f %.4f %.4f), %+.1f mm along the turned right", l.frame,
            size, kVerdict[verdict], v.eye[0], v.eye[1], v.eye[2], along * 1000.0);
    }
    switch (verdict) {
    case viewturn::kMainView: {
        ++s.main;
        if (!was.named && l.pick.named) {
            l.lastNamed = l.pick.use;
            // The frame's views turned before it by the last frame's age: by another build than its own?
            s.renamed += was.use >= 0 && was.use != l.pick.use;
        }
        // Its eye's pass: its place along the turned right a centimetre or more from the first pass's (and within an eye
        // distance or so) is the other's. The body views since the last main view's go with this one's.
        const float* right = l.builds[0].after;
        const double along = v.eye[0] * right[0] + v.eye[1] * right[1] + v.eye[2] * right[2];
        int pass = -1;
        if (l.passes == 0)
            l.passAlong[0] = along, l.passes = 1, pass = 0;
        else if (std::fabs(along - l.passAlong[l.passes - 1]) < 0.01)
            pass = l.passes - 1;
        else if (l.passes == 1 && std::fabs(along - l.passAlong[0]) < 0.2)
            l.passAlong[1] = along, l.passes = 2, pass = 1;
        else if (l.passes == 2)
            pass = std::fabs(along - l.passAlong[0]) < std::fabs(along - l.passAlong[1]) ? 0 : 1;
        if (pass >= 0) {
            for (uint32_t i = l.pendingFrom; i < l.turned && i < 32; ++i)
                l.order[i] = static_cast<int8_t>(pass);
            l.pendingFrom = l.turned;
        }
        if (effects::TailRecording()) {
            char note[120];
            std::snprintf(note, sizeof(note), "the main view's upload: pass %d, %+.1f mm along the turned right from the "
                          "first pass's", pass, (along - l.passAlong[0]) * 1000.0);
            effects::TailNote(note);
        }
        return;
    }
    case viewturn::kTurnedBefore:
        ++s.before;
        break;  // already turned (the view description's first-person camera), but at the picture's FOV too
    case viewturn::kAway:
        ++s.away;
        return;
    case viewturn::kElsewhere: {
        ++s.elsewhere;
        if (!perView || l.count <= 0)
            return;
        // Logged only if it could be the body's: its rows near a build's camera as made, or its eye within the arms'
        // space's reach across (another camera, hundreds of metres off, used the logs up: the review, 2026-11-01).
        int nearest = 0;
        float nearestOff = 1e9f;
        for (int b = 0; b < l.count; ++b) {
            float off = 0.0f;
            for (int i = 0; i < 9; ++i)
                off = std::max(off, std::fabs(v.rows[i] - l.builds[b].before[i]));
            if (off < nearestOff)
                nearestOff = off, nearest = b;
        }
        if (!(nearestOff < 0.05f ||
              (std::fabs(v.eye[0]) < viewturn::kBodyReach && std::fabs(v.eye[2]) < viewturn::kBodyReach)))
            return;
        // In a frame the main view has named (the shots the body went in, 2026-11-01): against each of the frame's
        // builds, two a second at most, to tell what it is.
        if (l.pick.named && s.namedLogged < 2) {
            static int s_budget = 60;
            if (s_budget > 0) {
                --s_budget;
                ++s.namedLogged;
                LogLeftView(f, v, l, true, "a 352-byte view left as it was");
            }
        }
        // One a scripted run: against the nearest build (its scripted camera's rows as made, and how far its eye is).
        if (!s.loggedElsewhere) {
            s.loggedElsewhere = true;
            const float* at = l.builds[nearest].place;
            const double d[3] = {v.eye[0] - at[0], v.eye[1] - at[1], v.eye[2] - at[2]};
            Log("Camera: a 352-byte view left as it was in a scripted picture (this run's first): its eye (%.2f %.2f %.2f), "
                "%.2f m from the scripted camera of the build its rows are nearest (%d builds old), its rows up to %.5f "
                "off that camera's as made",
                v.eye[0], v.eye[1], v.eye[2], std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]), nearest, nearestOff);
        }
        return;
    }
    case viewturn::kOtherHand:
        ++s.otherHand;
        return;
    case viewturn::kUnnamed:
        ++s.unnamed;
        return;
    case viewturn::kNotView:
        if (!perView || fault == viewturn::kPerViewOk)
            return;
        g_bodyNotPerView.fetch_add(1, std::memory_order_relaxed);
        // One a run with a view-projection's forward at +176 the body lock doesn't read, had the game laid out its
        // per-view constants otherwise there.
        if (keep && forward && !s.loggedOdd) {
            s.loggedOdd = true;
            static const char* const kWhy[] = {"", "the view at +48 isn't rigid", "the projection at +112 isn't a perspective one",
                                               "the view-projection at +176 isn't the projection times the view",
                                               "+240 isn't the projection times the view's turn alone"};
            Log("Camera: a 352-byte upload with a view-projection's forward at +176 the body lock doesn't read as "
                "per-view constants (this scripted run's first: %s), as the game uploaded it:",
                kWhy[fault]);
            for (int r = 0; r < 22; ++r)
                Log("Camera:   +%3d: %12.5f %12.5f %12.5f %12.5f", r * 16, made[4 * r], made[4 * r + 1], made[4 * r + 2],
                    made[4 * r + 3]);
        }
        return;
    case viewturn::kBodyTurned:
        break;
    }
    // At the picture's field of view, as the FOV fix makes a first-person view: at the game's it widens it next.
    const float wide = g_wideScale.load(std::memory_order_relaxed), game = g_gameScale.load(std::memory_order_relaxed);
    float from = 0.0f;
    const viewturn::BodyFov fov = perView ? viewturn::WidenPerView(f, wide, game, from)
                                          : viewturn::WidenViewProjection(f, wide, game, from);
    if (fov == viewturn::kFovWidened) {
        const float degrees = FovDegrees(from);
        s.widenedLo = s.widened ? std::min(s.widenedLo, degrees) : degrees;
        s.widenedHi = s.widened ? std::max(s.widenedHi, degrees) : degrees;
        ++s.widened;
    }
    s.atGame += fov == viewturn::kFovGame;
    s.atPicture += fov == viewturn::kFovPicture;
    s.fovLeft += fov == viewturn::kFovLeft;
    if (verdict == viewturn::kTurnedBefore)
        return;
    s.own += v.own >= 0;
    s.offPick += v.own >= 0 && v.picked >= 0 && v.own != v.picked;
    s.perViewWorld += perView && v.world;
    if (!l.counted) {
        l.counted = true;
        ++(l.pick.named ? s.named[l.pick.use] : s.guessed[l.pick.use]);
    }
    if (!perView) {
        ++s.world;
        // Beside the 352-byte ones left as they were in a frame the main view named: the 80-byte one it turned.
        if (l.pick.named && s.namedWorldLogged < 1) {
            static int s_budget = 30;
            if (s_budget > 0) {
                --s_budget;
                ++s.namedWorldLogged;
                LogLeftView(f, v, l, false, "an 80-byte view-projection turned");
            }
        }
        return;
    }
    // Its eye's pass, as the same place in the last frame's was (both passes seen then), else the first half of the last
    // frame's; with both passes' places known, moved half the eye distance that way, as views.cpp moves each eye's
    // cameras: the game draws the body from one camera for both eyes, flat against the stereo scene ("like I'm looking
    // cross-eyed" with the jammer raised to the face, the gun inspected close up, the user, 2026-11-01). weapon.txt
    // "bodystereo 0" leaves it in the middle, both eyes drawing it alike.
    const uint32_t k = l.turned++, n = l.turnedBefore;
    int pass = -1;
    if (l.lastPictures == 2)  // the last frame drew both eyes' pictures: this one's tell its passes
        pass = l.pictures >= 1 ? 1 : 0;
    else if (l.lastPasses == 2 && k < n && k < 32)
        pass = l.lastOrder[k];
    else if (n >= 2 && n % 2 == 0)
        pass = k < n / 2 ? 0 : 1;
    double moved = 0.0;
    // Moved only with its pass told by this frame's eye pictures: the learned order and the halves stay for the
    // per-eye counts. They go wrong in any frame with another number of body views than the last (looking down, the
    // legs' views come and go), and a view moved to the other eye's place is the black holes of the first try. With
    // no eye pictures counted (a mission whose eyes' pictures are drawn another way), the body stays flat, as the game
    // draws it (the review, 2026-11-01).
    if (pass >= 0 && l.lastPictures == 2 && l.lastPasses == 2 && g_bodyStereo.load(std::memory_order_relaxed)) {
        const double mid = 0.5 * (l.lastPassAlong[0] + l.lastPassAlong[1]);
        moved = l.lastPassAlong[pass] - mid;
        viewturn::ShiftPerView(f, v, moved);
        ++s.shifted;
        s.eyesApart = std::max(s.eyesApart, std::fabs(l.lastPassAlong[1] - l.lastPassAlong[0]));
    }
    if (effects::TailRecording()) {
        char note[120];
        std::snprintf(note, sizeof(note), "a body view, %u of the frame: pass %d, moved %+.1f mm", k, pass, moved * 1000.0);
        effects::TailNote(note);
    }
    const int eye = !views::Active() || pass < 0 ? -1
                    : l.lastPasses == 2      ? (l.lastPassAlong[pass] <= l.lastPassAlong[1 - pass] ? 0 : 1)
                                             : pass;
    ++s.byEye[eye + 1];
    if (k == 0) {
        std::memcpy(l.firstEye, v.eye, sizeof(l.firstEye));
    } else {
        const double d[3] = {v.eye[0] - l.firstEye[0], v.eye[1] - l.firstEye[1], v.eye[2] - l.firstEye[2]};
        s.spread = std::max(s.spread, std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]));
    }
    if (keep && !s.loggedView) {
        s.loggedView = true;
        LogBodyView(made, v, l.builds[v.own >= 0 ? v.own : l.pick.use], fov);
    }
}

// Once a second (Probe), while the body lock is on in a scripted picture: what it turned, and what it left as it was.
void LogBodyLock() {
    const bool on = g_bodyLock.load(std::memory_order_relaxed) && camera::BodyTurnWanted();
    BodyStats s;
    {
        std::lock_guard lock(g_bodyMutex);
        s = g_bodyStats;
        g_bodyStats = BodyStats{};
        if (on) {  // logged once a scripted run: a gap of a second or more starts another
            g_bodyStats.loggedView = s.loggedView;
            g_bodyStats.loggedOdd = s.loggedOdd;
            g_bodyStats.loggedElsewhere = s.loggedElsewhere;
        }
    }
    const uint32_t notPerView = g_bodyNotPerView.exchange(0, std::memory_order_relaxed);
    const uint32_t turned = s.byEye[0] + s.byEye[1] + s.byEye[2];
    auto sum = [](const uint32_t* n) { return n[0] + n[1] + n[2] + n[3]; };
    if (turned || s.world) {
        char unknown[48] = "";
        if (s.byEye[0])
            std::snprintf(unknown, sizeof(unknown), ", %u eye not known", s.byEye[0]);
        char fov[200] = "";
        if (s.widened) {
            const float wide = g_wideScale.load(std::memory_order_relaxed);
            const float game = g_gameScale.load(std::memory_order_relaxed);
            char from[48];
            if (s.widenedHi - s.widenedLo >= 0.05f)
                std::snprintf(from, sizeof(from), "%.1f to %.1f", s.widenedLo, s.widenedHi);
            else
                std::snprintf(from, sizeof(from), "%.1f", s.widenedLo);
            std::snprintf(fov, sizeof(fov),
                          "%u widened here from %s degrees to the picture's %.1f (the FOV fix looks for the game's "
                          "%.1f), ",
                          s.widened, from, FovDegrees(wide), FovDegrees(game));
        }
        Log("Camera: the body's first-person views turned with the head: %u uploads this second (%u left eye, %u right "
            "eye%s); %u of its view-projections in the world (80 bytes) too; their field of view: %s%u at the game's "
            "for the FOV fix, %u at the picture's already, %u left at their own (the FOV fix off); frames turned by "
            "the view build the main view's own upload named %u (the newest %u, the one before %u, older %u), with "
            "none named yet by the age the last frame's named %u (%u of them then named another); %u turned by the "
            "build they were made in (%u of them a build off the frame's), %u with their eye past the arms' space's "
            "5 m; %u moved to their eye's place (the eyes %.0f mm apart); left as they were: %u the main view's, %u "
            "turned already, %u looking away from the scripted camera, %u other views, %u of the other handedness, %u "
            "with no build to turn them by yet; a frame's body views' eyes up to %.0f mm apart",
            turned, s.byEye[1], s.byEye[2], unknown, s.world, fov, s.atGame, s.atPicture, s.fovLeft, sum(s.named),
            s.named[0], s.named[1], s.named[2] + s.named[3], sum(s.guessed), s.renamed, s.own, s.offPick,
            s.perViewWorld, s.shifted, s.eyesApart * 1000.0, s.main, s.before, s.away, s.elsewhere, s.otherHand,
            s.unnamed, s.spread * 1000.0);
    } else if (on) {
        Log("Camera: the body's first-person views: none seen this second in a scripted picture (left as they were: %u "
            "the main view's, %u turned already, %u looking away from the scripted camera, %u other views, %u of the "
            "other handedness, %u with no build to turn them by yet; %u 352-byte uploads not per-view constants)",
            s.main, s.before, s.away, s.elsewhere, s.otherHand, s.unnamed, notPerView);
    }
}

void HandleUpload(const PendingMap& p) {
    auto data = static_cast<uint8_t*>(p.data);
    bool research = g_research.load(std::memory_order_relaxed);
    if (p.size < kMinRingSize) {
        if (p.fresh) {
            ++t_uploadSerial;
            if (g_weaponWatch.load(std::memory_order_relaxed))
                NoteRecentUpload(p.resource, data, p.size);  // before the rescale changes it
            // Every per-view upload of the frame, as the game made it, by the view build whose rows it carries (camera.h NoteDrawnView): they name the head pose the
            // picture is drawn with (its first, or by weapon.txt "nameby" what its main views carried), and the world's turn is measured from them. With each goes
            // this thread's count of small uploads: those between one view and the next are the per-draw constants of what is drawn with it.
            if (p.size == viewturn::kPerViewBytes) {
                viewturn::BodyView seen;
                if (viewturn::ReadPerView(reinterpret_cast<const float*>(data), seen) == viewturn::kPerViewOk)
                    camera::NoteDrawnView(seen.rows, seen.eye, t_uploadSerial);
            }
            // In a scripted picture the body's own views turned with the head, before anything else reads them.
            if (p.size == viewturn::kPerViewBytes || p.size == viewturn::kWorldViewBytes)
                LockBody(data, p.size);
            // At any field of view: a scripted picture's body comes at the picture's already, the FOV fix finding nothing in it.
            if (p.size == viewturn::kPerViewBytes || p.size == viewturn::kWorldViewBytes) {
                float* view = reinterpret_cast<float*>(data) + (p.size == viewturn::kPerViewBytes ? 44 : 0);
                if (Near(Length3(view[12], view[13], view[14]), 1.0f))
                    NearerPlane(view, 8, 12, 1);
            }
            t_uploadResource = p.resource;
            RescaleProjections(data, p.size, kSmall);
            t_uploadResource = nullptr;
            if (p.size == sizeof(t_viewConstants.bytes)) {
                t_viewConstants.resource = p.resource;
                std::memcpy(t_viewConstants.bytes, data, p.size);
            }
            if (research)
                Inspect(data, p.size, true);
            // Every upload the gloves' size, not only those whose bones all passed as matrices:
            // only a third did (scaled bones break the row).
            // The gun's draws read only its first bones: those are always kept, for the magazine.
            // Whole uploads (some 600 a frame) only while something else needs them.
            if (p.size == kGloveUploadSize) {
                bool whole = g_handShift.load(std::memory_order_relaxed) ||
                             g_weaponShift.load(std::memory_order_relaxed) ||
                             g_weaponWatch.load(std::memory_order_relaxed) ||
                             g_dumpWanted.load(std::memory_order_relaxed) ||
                             g_handsActive.load(std::memory_order_relaxed);
                t_upload.resource = p.resource;
                t_upload.serial = t_uploadSerial;
                t_upload.bytes.assign(data, data + (whole ? p.size : kGunPrefix));
                t_upload.bones = t_drawsSinceBones == 0 ? t_bones : BoneInfo{};
            } else if (p.resource == t_upload.resource) {
                t_upload.resource = nullptr;  // refilled with something else: the copy is stale
            }
        }
        return;
    }
    if (p.resource == t_upload.resource)
        t_upload.resource = nullptr;
    // Finding what was appended to a ring buffer means comparing all of it with a shadow copy on
    // every upload: only research needs that.
    if (!research) {
        if (p.fresh)
            RescaleProjections(data, p.size, kRingWhole);
        return;
    }
    std::lock_guard lock(g_shadowMutex);
    std::vector<uint8_t>& shadow = g_shadows[p.resource];
    if (p.fresh || shadow.size() != p.size) {
        if (p.fresh) {
            RescaleProjections(data, p.size, kRingWhole);
            Inspect(data, p.size, true);
        }
        shadow.assign(data, data + p.size);
        return;
    }
    UINT begin = 0, end = 0;
    if (!ChangedRange(data, shadow, p.size, begin, end))
        return;
    RescaleProjections(data + begin, end - begin, kRingAppend);
    Inspect(data + begin, end - begin, false);
    std::memcpy(shadow.data() + begin, data + begin, end - begin);  // including any lift
}

void Unmap(int which, ID3D11DeviceContext* ctx, ID3D11Resource* resource, UINT sub) {
    for (PendingMap& p : t_pending) {
        if (p.resource == resource) {
            // Still writable here: the data reaches the GPU only once Unmap is called.
            if (!scopeview::Drawing())
                HandleUpload(p);
            postpin::NoteUpload(ctx, resource, p.data, p.size, p.fresh);  // the vignette's constants as the card will read them (postpin.h)
            p = {};
            break;
        }
    }
    if (g_uploadCopy.load(std::memory_order_relaxed) == 1)
        ShadowEnd(ctx, resource);  // the mod's copy (read and changed above) into the card's memory, before the driver takes it
    g_origUnmap[which](ctx, resource, sub);
}

// The vertex shader's constant buffer slot a buffer is bound in, or -1.
int BoundSlot(ID3D11DeviceContext* ctx, ID3D11Resource* resource) {
    ID3D11Buffer* bound[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = {};
    ctx->VSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, bound);
    int slot = -1;
    for (UINT i = 0; i < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i) {
        if (bound[i] && bound[i] == resource)
            slot = static_cast<int>(i);
        if (bound[i])
            bound[i]->Release();
    }
    return slot;
}

// This thread's own constant buffer of that size, made on first use.
ID3D11Buffer* OwnBuffer(ID3D11DeviceContext* ctx, UINT size) {
    if (!t_shiftBuffer || t_shiftSize != size) {
        if (t_shiftBuffer)
            t_shiftBuffer->Release();
        t_shiftBuffer = nullptr;
        ID3D11Device* device = nullptr;
        ctx->GetDevice(&device);
        D3D11_BUFFER_DESC desc{size, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0};
        HRESULT hr = device->CreateBuffer(&desc, nullptr, &t_shiftBuffer);
        device->Release();
        if (FAILED(hr))
            return nullptr;
        t_shiftSize = size;
    }
    return t_shiftBuffer;
}

// Draws with these bones bound in place of BF3's buffer (in the slot it's in), then puts BF3's
// back. The buffer is refilled with WRITE_DISCARD, so it can serve several draws in a row.
// Cracks in the gloves' mesh (tools/handview.cpp "a=1"): three thin slivers where two edges meet but
// were never joined, their sides on different bones (the middle and ring fingers' webbing, the back
// of the little finger's knuckle, the thumb's web). Hidden in the game's own poses, they opened into
// holes with the fingers spread or curled (user, 2026-10-30). Each is closed with a triangle of the
// same vertices of the gloves' buffer, both ways round (the one facing out is drawn).
constexpr uint16_t kGlovePatch[] = {7, 8, 1723, 7, 1723, 8, 9, 11, 1597, 9, 1597, 11, 13, 14, 1393, 13, 1393, 14};

void DrawGlovePatch(int which, ID3D11DeviceContext* ctx, INT base) {
    static std::once_flag created;
    static ID3D11Buffer* s_patch = nullptr;
    std::call_once(created, [ctx] {
        ID3D11Device* device = nullptr;
        ctx->GetDevice(&device);
        if (!device)
            return;
        D3D11_BUFFER_DESC desc{sizeof(kGlovePatch), D3D11_USAGE_IMMUTABLE, D3D11_BIND_INDEX_BUFFER, 0, 0, 0};
        D3D11_SUBRESOURCE_DATA data{kGlovePatch, 0, 0};
        if (FAILED(device->CreateBuffer(&desc, &data, &s_patch)))
            s_patch = nullptr;
        device->Release();
    });
    if (!s_patch)
        return;
    ID3D11Buffer* indices = nullptr;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    UINT offset = 0;
    ctx->IAGetIndexBuffer(&indices, &format, &offset);
    ctx->IASetIndexBuffer(s_patch, DXGI_FORMAT_R16_UINT, 0);
    g_origDrawIndexed[which](ctx, static_cast<UINT>(std::size(kGlovePatch)), 0, base);
    ctx->IASetIndexBuffer(indices, format, offset);
    if (indices)
        indices->Release();
}

// The eye close behind a scope (ScopeWindow): the lens reads the picture where the scope's line goes, there under the lens's own disc. Everything in the hands is left out of that bit of the picture, not only the scope's body, or the rail, the
// mount's bolts and whatever else of the gun is under it show magnified in the glass (the user, 2026-11-05: "I can see the nail, the bolts or nails through the scope"). None of it is seen: the disc covers it. By eye; as last drawn, this frame's or the last.
struct LensHole {
    uint32_t frame = 0xFFFFFF00u;
    LONG rect[4] = {};
};
LensHole g_lensHole[2];
std::atomic<uint32_t> g_lensHoleFrame{0xFFFFFF00u};
thread_local bool t_noHole = false;  // the disc itself, and what cuts its own window
bool ViewOfDraw(ID3D11DeviceContext* ctx, float* vp, int& eye, float* viewToArms = nullptr);
ID3D11RasterizerState* WithScissor(ID3D11DeviceContext* ctx, ID3D11RasterizerState* state);

const LONG* HoleOfDraw(ID3D11DeviceContext* ctx) {
    const uint32_t frame = g_frameSerial.load(std::memory_order_relaxed);
    if (t_noHole || frame - g_lensHoleFrame.load(std::memory_order_relaxed) > 1)
        return nullptr;
    float vp[16];
    int eye = -1;
    if (!ViewOfDraw(ctx, vp, eye) || eye < 0 || eye > 1 || frame - g_lensHole[eye].frame > 1)
        return nullptr;
    return g_lensHole[eye].rect;
}

void DrawRoundHole(int which, ID3D11DeviceContext* ctx, UINT count, UINT start, INT base, const LONG* hole) {
    ID3D11RasterizerState* state = nullptr;
    ctx->RSGetState(&state);
    if (ID3D11RasterizerState* scissored = WithScissor(ctx, state)) {
        UINT rectCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        D3D11_RECT kept[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
        ctx->RSGetScissorRects(&rectCount, kept);
        ctx->RSSetState(scissored);
        constexpr LONG kAll = 16384;
        const D3D11_RECT round[4] = {{0, 0, kAll, hole[1]}, {0, hole[3], kAll, kAll}, {0, hole[1], hole[0], hole[3]}, {hole[2], hole[1], kAll, hole[3]}};
        for (const D3D11_RECT& rect : round) {
            ctx->RSSetScissorRects(1, &rect);
            g_origDrawIndexed[which](ctx, count, start, base);
        }
        ctx->RSSetScissorRects(rectCount, kept);
        ctx->RSSetState(state);
    } else {
        g_origDrawIndexed[which](ctx, count, start, base);
    }
    if (state)
        state->Release();
}

// Every slot of the vertex and of the pixel shader that holds that buffer now (bit i: slot i).
void PaletteSlots(ID3D11DeviceContext* ctx, ID3D11Resource* resource, unsigned& vs, unsigned& ps) {
    ID3D11Buffer* bound[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = {};
    vs = ps = 0;
    for (int stage = 0; stage < 2; ++stage) {
        if (stage == 0)
            ctx->VSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, bound);
        else
            ctx->PSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, bound);
        for (UINT i = 0; i < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i) {
            if (bound[i] && bound[i] == resource)
                (stage == 0 ? vs : ps) |= 1u << i;
            if (bound[i])
                bound[i]->Release();
            bound[i] = nullptr;
        }
    }
}

// everySlot (weapon.txt "partcopy 1", a test on a magazine's own draw): the mod's palette also in every other slot of the vertex and pixel shaders holding the
// game's, all put back after.
bool DrawWithPalette(int which, ID3D11DeviceContext* ctx, UINT count, UINT start, INT base,
                     ID3D11Resource* original, const uint8_t* bytes, UINT size, bool patchGloves = false,
                     int extra = 0, bool everySlot = false) {
    int slot = BoundSlot(ctx, original);
    ID3D11Buffer* own = slot < 0 ? nullptr : OwnBuffer(ctx, size);
    D3D11_MAPPED_SUBRESOURCE mapped;
    // BF3's own Map/Unmap, not the hooked ones: this upload isn't one to inspect.
    if (!own || FAILED(g_origMap[which](ctx, own, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        return false;
    std::memcpy(mapped.pData, bytes, size);
    g_origUnmap[which](ctx, own, 0);
    ctx->VSSetConstantBuffers(slot, 1, &own);
    unsigned vsMore = 0, psMore = 0;
    if (everySlot) {
        PaletteSlots(ctx, original, vsMore, psMore);
        vsMore &= ~(1u << slot);
        for (UINT i = 0; i < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i) {
            if ((vsMore >> i) & 1u)
                ctx->VSSetConstantBuffers(i, 1, &own);
            if ((psMore >> i) & 1u)
                ctx->PSSetConstantBuffers(i, 1, &own);
        }
    }
    const LONG* hole = HoleOfDraw(ctx);
    for (int i = 0; i <= extra; ++i) {  // drawn again on itself: a blended part stands out more
        if (hole)
            DrawRoundHole(which, ctx, count, start, base, hole);
        else
            g_origDrawIndexed[which](ctx, count, start, base);
    }
    if (patchGloves)
        DrawGlovePatch(which, ctx, base);
    auto buffer = static_cast<ID3D11Buffer*>(original);
    ctx->VSSetConstantBuffers(slot, 1, &buffer);
    for (UINT i = 0; (vsMore | psMore) && i < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i) {
        if ((vsMore >> i) & 1u)
            ctx->VSSetConstantBuffers(i, 1, &buffer);
        if ((psMore >> i) & 1u)
            ctx->PSSetConstantBuffers(i, 1, &buffer);
    }
    return true;
}

// As DrawWithPalette, for several runs of one draw's indices with the same bones (a body drawn round a handle drawn apart: handleruns.h): the bones put
// in once, each run drawn (round the lens's hole as any draw is). None: nothing to draw, done.
bool DrawRangesWithPalette(int which, ID3D11DeviceContext* ctx, const handleruns::Run* runs, int runCount, INT base, ID3D11Resource* original, const uint8_t* bytes,
                           UINT size) {
    if (runCount <= 0)
        return true;
    int slot = BoundSlot(ctx, original);
    ID3D11Buffer* own = slot < 0 ? nullptr : OwnBuffer(ctx, size);
    D3D11_MAPPED_SUBRESOURCE mapped;
    // BF3's own Map/Unmap, not the hooked ones: this upload isn't one to inspect.
    if (!own || FAILED(g_origMap[which](ctx, own, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        return false;
    std::memcpy(mapped.pData, bytes, size);
    g_origUnmap[which](ctx, own, 0);
    ctx->VSSetConstantBuffers(slot, 1, &own);
    const LONG* hole = HoleOfDraw(ctx);
    for (int i = 0; i < runCount; ++i) {
        if (hole)
            DrawRoundHole(which, ctx, runs[i].count, runs[i].start, base, hole);
        else
            g_origDrawIndexed[which](ctx, runs[i].count, runs[i].start, base);
    }
    auto buffer = static_cast<ID3D11Buffer*>(original);
    ctx->VSSetConstantBuffers(slot, 1, &buffer);
    return true;
}

// Draws the gloves (or with weapon set, a pass of the gun, those bones) with a shifted copy of
// their bones: the copy goes into this thread's own constant buffer, bound in the slot where BF3's
// buffer is, then BF3's is put back.
bool DrawShifted(int which, ID3D11DeviceContext* ctx, UINT count, UINT start, INT base, UINT bones,
                 const PassSelection* weapon = nullptr) {
    const Upload& u = t_upload;
    if (!u.resource || u.bytes.size() != kGloveUploadSize) {  // or only the gun's first bones
        ++g_noCopy;
        return false;
    }
    if (weapon && !PlayersOwn(u.bytes)) {
        ++g_weaponFar;
        return false;
    }
    int slot = BoundSlot(ctx, u.resource);
    if (slot < 0) {
        ++g_noSlot;
        return false;
    }

    UINT size = static_cast<UINT>(u.bytes.size());
    if (!OwnBuffer(ctx, size))
        return false;
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (FAILED(g_origMap[which](ctx, t_shiftBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        return false;
    // The bones are changed in a copy of the mod's own, then written to the card's memory once: reading back what was written to a
    // mapped pointer crosses PCIe uncached on AMD's cards (write-combined video memory).
    thread_local std::vector<float> t_shiftCopy;
    t_shiftCopy.resize((size + 3) / 4);
    float* m = t_shiftCopy.data();
    std::memcpy(m, u.bytes.data(), size);
    // The bones start at the front of the buffer. (They pass as matrices only some of the time,
    // scaled bones break the row, so the count comes from the hunt instead.)
    // Only the selected bones (hands.txt). The gloves' palette is numbered as the selection is; the
    // second pass has a palette of its own, so its bones are picked by matching the selected
    // gloves bones' values (the same bone has the same matrix in both).
    bool gloves = !weapon && bones == kHandDraws[0].bones;
    std::lock_guard lock(g_selMutex);
    if (gloves)
        g_selValues.clear();
    const float* shift = weapon ? g_weaponOffset : g_shift;
    for (UINT k = 0; k < bones && k * 12 + 12 <= size / 4; ++k) {
        float* bone = m + k * 12;
        bool selected = weapon ? k < 64 && weapon->bones[k]
                               : g_selectAll || (gloves ? k < 64 && g_selected[k] : MatchesSelected(bone));
        if (!selected)
            continue;
        if (weapon && g_weaponHide) {
            std::memset(bone, 0, 12 * sizeof(float));  // its vertices all land on one point
            continue;
        }
        if (gloves && !g_selectAll)
            g_selValues.insert(g_selValues.end(), bone, bone + 12);
        bone[3] += shift[0];  // translation (y is up in character space)
        bone[7] += shift[1];
        bone[11] += shift[2];
    }
    std::memcpy(mapped.pData, m, size);
    if (weapon)
        ++g_weaponShifted;
    g_origUnmap[which](ctx, t_shiftBuffer, 0);

    ctx->VSSetConstantBuffers(slot, 1, &t_shiftBuffer);
    g_origDrawIndexed[which](ctx, count, start, base);
    auto original = static_cast<ID3D11Buffer*>(u.resource);
    ctx->VSSetConstantBuffers(slot, 1, &original);
    ++g_shifted;
    return true;
}

// ---- Magazine ----
// X ejects the magazine: it vanishes from the gun, falls to the floor and lies there a while. The
// mesh dump showed it's a bone of its own in each of the gun's passes: bone 4 of the colour pass (a
// 21 cm part below the gun), bone 0 of the 312-index draw (the rounds at its top), bones 4 and 5
// of the depth pass that draws the gun whole. The gun's draws use bones 0-5 only.
struct MagBone {
    size_t pass;
    UINT bone;
};
constexpr MagBone kMagazineBones[] = {{0, 4}, {1, 0}, {2, 4}, {2, 5}};
constexpr size_t kMagBones = std::size(kMagazineBones);
// The magazine's box in the gun's model space (from the dump): its centre is what it tumbles
// around, its corners what touches the floor.
constexpr float kMagMin[3] = {-0.011f, -0.304f, 0.038f}, kMagMax[3] = {0.011f, -0.090f, 0.131f};
constexpr float kGravity = 9.81f;
constexpr float kEjectSpeed = 0.8f;  // m/s out of the magazine well
constexpr float kSpin = 4.0f;        // rad/s, end over end
// On the floor before it goes (the user, 2026-11-05: "stay there for a minute", then the same day "disappear after 30 seconds ... I don't want to flood the map"; loose.h's
// kLieSeconds alike). Lying there it had followed the player around (tested 2026-10-30), drawn as the aim camera placed it; kept in the world now and placed by each draw
// (DrawnToWorld, WorldToDrawn), it lies where it fell.
constexpr double kLieSeconds = 30.0;

std::atomic<bool> g_magOut{false};       // hidden in the gun
std::atomic<bool> g_dropShown{false};    // the dropped one is drawn
std::atomic<ULONGLONG> g_rifleDrawn{0};  // when the gun with this magazine was last drawn
std::atomic<uint32_t> g_rifleFrame{0};   // ... in which frame (g_frameSerial)

// Kept in the world (the arms' space with origin0, as the world's picture has it), not as one draw placed it: each draw places it again through its own HandTargets::drawnFromWorld
// (DropBone), so both eyes see it where it lies, with their own depth, and it stays put as the head turns or the gun swings. Drawn as last placed in the first-person view, it had
// no depth between the eyes and moved with the aim (the user, 2026-11-05: "a weird depth to it... like some other dimension").
struct Drop {
    float last[kMagBones][12] = {};   // the magazine's bones as last drawn in the gun, in the world (with lastOrigin)
    float lastOrigin[kMagBones][3] = {};
    bool seen[kMagBones] = {};
    float start[kMagBones][12] = {};  // ... when it came out
    bool started[kMagBones] = {};
    float c0[3] = {};                 // its centre then
    float pos[3] = {}, vel[3] = {};   // its centre since, and its speed
    float axis[3] = {1, 0, 0};        // what it tumbles around
    float angle = 0.0f;
    float q[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};  // its turn since it came out
    float centre[3] = {};             // where it's drawn: pos, kept still in the world
    float origin0[3] = {};            // the arms' space's origin when it came out
    bool falling = false;
    double landedAt = 0.0, updatedAt = 0.0;
    float held[kMagBones][12] = {};   // a magazine in either hand: its bones as last drawn there, in the world (with heldOrigin)
    float heldOrigin[kMagBones][3] = {};
    bool heldSeen[kMagBones] = {};
    double heldAt[kMagBones] = {};    // ... when (Seconds()): a drop from the hand takes only what's fresh
    float spin = kSpin;               // rad/s
};
// The box of whichever magazine it is, in its own vertices' space (the M416's, unless DrawHeldPart has drawn another).
const float* g_magBoxMin = kMagMin;
const float* g_magBoxMax = kMagMax;
std::mutex g_magMutex;
Drop g_drop;

double Seconds() {
    static const double frequency = [] {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        return static_cast<double>(f.QuadPart);
    }();
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return static_cast<double>(now.QuadPart) / frequency;
}

// A turn of angle radians around the unit axis a, as a row-major 3x3 matrix.
void AxisAngle(const float* a, float angle, float* q) {
    float c = std::cos(angle), s = std::sin(angle), t = 1.0f - c;
    q[0] = t * a[0] * a[0] + c, q[1] = t * a[0] * a[1] - s * a[2], q[2] = t * a[0] * a[2] + s * a[1];
    q[3] = t * a[0] * a[1] + s * a[2], q[4] = t * a[1] * a[1] + c, q[5] = t * a[1] * a[2] - s * a[0];
    q[6] = t * a[0] * a[2] - s * a[1], q[7] = t * a[1] * a[2] + s * a[0], q[8] = t * a[2] * a[2] + c;
}

void Normalize(float* v) {
    float length = Length3(v[0], v[1], v[2]);
    if (length > 1e-6f)
        v[0] /= length, v[1] /= length, v[2] /= length;
}

// A bone matrix (three rows: rotation | translation) moved the way the dropped magazine has moved:
// turned by q around its centre at the start, c0, which is now at centre.
void MoveBone(const float* m, const Drop& d, float* out) {
    for (int i = 0; i < 3; ++i) {
        const float* q = d.q + i * 3;
        for (int j = 0; j < 3; ++j)
            out[i * 4 + j] = q[0] * m[j] + q[1] * m[4 + j] + q[2] * m[8 + j];
        out[i * 4 + 3] = q[0] * (m[3] - d.c0[0]) + q[1] * (m[7] - d.c0[1]) + q[2] * (m[11] - d.c0[2]) + d.centre[i];
    }
}

// A draw's way from the world into what it draws (HandTargets::drawnFromWorld, ToArms) and the origin it goes with: kept past the block that has the draw's targets.
struct WorldPlacing {
    float fromWorld[12] = {};
    float origin[3] = {};
    bool have = false;
    void Take(const skin::HandTargets& t) {
        std::memcpy(fromWorld, t.drawnFromWorld, sizeof(fromWorld));
        std::memcpy(origin, t.seenOrigin, sizeof(origin));
        have = t.haveDrawnFromWorld;
    }
};

// A bone as drawn (by a draw with placing p) into the world, with the origin it was seen with; and back out, for a draw, from the world with origin `from`. Without the draw's
// way from the world (hands off), it stays as drawn.
void DrawnToWorld(const WorldPlacing& p, const float* drawn, float* world, float* origin) {
    float back[12];
    if (p.have && Invert(p.fromWorld, back))
        Compose(back, drawn, world);
    else
        std::memcpy(world, drawn, 12 * sizeof(float));
    std::memcpy(origin, p.origin, 3 * sizeof(float));
}
void WorldToDrawn(const WorldPlacing& p, const float* world, const float* from, float* drawn) {
    float shifted[12];
    std::memcpy(shifted, world, sizeof(shifted));
    for (int i = 0; i < 3; ++i)
        shifted[i * 4 + 3] += from[i] - p.origin[i];
    if (p.have)
        Compose(p.fromWorld, shifted, drawn);
    else
        std::memcpy(drawn, shifted, sizeof(shifted));
}

// ---- Free hands (hands.cpp works out where they go) ----
std::mutex g_handsMutex;
bool g_handsOn = false;
skin::HandTargets g_hands;
// The launcher-pose gun drawn last as the targets came (SetHands: the rig's kind then, 0 the launcher's, else the held gun's colour count): with no hand
// on it, only its draws are placed by them (in the first frame of a switch the game makes while one lies, the new gun's draws aren't put where it lies).
uint32_t g_handsKind = 0;
// The gun drawn nowhere (HandTargets::stowed): its draws left out since that began, by kind (the rifle's body, DrawGun; its parts, DrawArms' 'G' draws: the ACOG and
// the M320; a launcher, DrawLauncher; any other gun, DrawHeldPart), for SetHands' line as it ends. When it began, and when the rifle was last left out with no
// 15 s gap before (under g_handsMutex): its data's scan goes by the gap since it was last in a hand or so left out, the game drawing it all the while (no load
// meanwhile; a load while it's away is still a gap).
enum StowedKind { kStowedBody, kStowedParts, kStowedLauncher, kStowedHeld, kStowedKinds };
std::atomic<uint32_t> g_stowedSkips[kStowedKinds];
bool g_stowedWas = false;
ULONGLONG g_stowedSince = 0, g_rifleStowedAt = 0;
// Its lines (NoteStowed): at most kStowedLines in a second, the rest counted for the next one said (a hand at a place going in and out of sight every frame).
constexpr int kStowedLines = 4;
ULONGLONG g_stowedSecond = 0;
int g_stowedSaid = 0;
uint32_t g_stowedUnsaid = 0;

bool RigThisFrame();  // a launcher-pose gun in hand as this frame began (g_rigThisFrame, with the launcher's draws below)
// The free hands' newer poses with a number not finite (hands.cpp guards them first): drawn nowhere rather than torn. The right glove by its own pose, or
// on a gun pose of its own, folded away; the rifle with no hand on it folded onto one point. Today's (the rifle in a hand, the right glove on it by the
// gun's own pose) aren't looked at. And the rifle in a hand with no right glove mode given (skin::kRightGame, the default) has the glove on its grip as it always
// has (never break what works: a right glove left as the game sent it floats at the game's own hold). Under g_handsMutex, once a frame (SetHands).
void GuardFreeHands(skin::HandTargets& t) {
    // The gun drawn nowhere (stowed): no right glove on its grip, nor the game's own at its hold, holding nothing in front of the eyes, and no forearm solved
    // to that grip; one by its own pose (an empty hand, seen) is drawn as it is.
    if (t.stowed && (t.rightGlove == skin::kRightGame || t.rightGlove == skin::kRightOnGrip)) {
        t.rightGlove = skin::kRightHidden;
        t.arm[1] = false;
    }
    if (t.gun && t.rightGlove == skin::kRightGame) {
        t.rightGlove = skin::kRightOnGrip;
        std::memcpy(t.rightGunPose, t.gunPose, sizeof(t.rightGunPose));
        std::memcpy(t.rightLauncherPose, t.launcherPose, sizeof(t.rightLauncherPose));
        if (static bool s_said = false; !s_said) {
            s_said = true;
            Log("Hands: no right glove mode came with the rifle in a hand: drawn on its grip as before");
        }
    }
    auto finite = [](const float* m, size_t n) {
        for (size_t i = 0; i < n; ++i)
            if (!std::isfinite(m[i]))
                return false;
        return true;
    };
    // With a launcher-pose gun in hand, the right glove on its grip with no pose of its own for it (a real one never has a turn of all zeros): on the gun as
    // drawn, as before.
    const bool rig = RigThisFrame();
    if (rig && t.rightGlove == skin::kRightOnGrip && finite(t.launcherPose, 12) && std::memcmp(t.rightLauncherPose, t.launcherPose, sizeof(t.launcherPose)) != 0) {
        const float* r = t.rightLauncherPose;
        if (r[0] == 0.0f && r[1] == 0.0f && r[2] == 0.0f && r[4] == 0.0f && r[5] == 0.0f && r[6] == 0.0f && r[8] == 0.0f && r[9] == 0.0f && r[10] == 0.0f) {
            std::memcpy(t.rightLauncherPose, t.launcherPose, sizeof(t.rightLauncherPose));
            if (static bool s_said = false; !s_said) {
                s_said = true;
                Log("Hands: no right glove pose came with the launcher or other gun in a hand: drawn on its grip as before");
            }
        }
    }
    const char* what = nullptr;
    if (t.rightGlove == skin::kRightOwn && !(finite(t.rightGlovePose, 12) && finite(&t.rightFingers[0][0], std::size(t.rightFingers) * 12))) {
        t.rightGlove = skin::kRightHidden;
        what = "the right glove's own pose";
    } else if (t.rightGlove == skin::kRightOnGrip && std::memcmp(t.rightGunPose, t.gunPose, sizeof(t.gunPose)) != 0 && !finite(t.rightGunPose, 12)) {
        t.rightGlove = skin::kRightHidden;
        what = "the right glove's gun pose";
    } else if (rig && t.rightGlove == skin::kRightOnGrip && std::memcmp(t.rightLauncherPose, t.launcherPose, sizeof(t.launcherPose)) != 0 &&
               !finite(t.rightLauncherPose, 12)) {
        // (Only with a launcher-pose gun in hand: nothing else draws on it, and the rifle's right glove is never hidden for it.)
        t.rightGlove = skin::kRightHidden;
        what = "the right glove's launcher pose";
    }
    if (!t.gun && t.gunDrawn && !(t.gunLying ? finite(t.gunWorld, 12) && finite(t.gunWorldOrigin, 3) : finite(t.gunPose, 12))) {
        // Every vertex of it on one point (left out, the game's own would show where the game holds it).
        std::memset(t.gunPose, 0, sizeof(t.gunPose));
        std::memset(t.gunWorld, 0, sizeof(t.gunWorld));
        std::memset(t.gunWorldOrigin, 0, sizeof(t.gunWorldOrigin));
        what = "the rifle with no hand on it";
    }
    // A launcher-pose gun with no hand on it (free hands, every gun), alike.
    if (!t.gun && t.launcherDrawn && !(t.launcherLying ? finite(t.launcherWorld, 12) && finite(t.launcherWorldOrigin, 3) : finite(t.launcherPose, 12))) {
        std::memset(t.launcherPose, 0, sizeof(t.launcherPose));
        std::memset(t.launcherWorld, 0, sizeof(t.launcherWorld));
        std::memset(t.launcherWorldOrigin, 0, sizeof(t.launcherWorldOrigin));
        what = "the gun with no hand on it";
    }
    // The launcher's case on its frame while it lies: drawn nowhere rather than torn.
    if (t.launcherCase == 3 && !finite(t.launcherCaseRel, 12)) {
        t.launcherCase = 1;
        what = "the launcher's case on it";
    }
    if (static ULONGLONG s_said = 0; what && GetTickCount64() - s_said >= 5000) {
        s_said = GetTickCount64();
        Log("Hands: %s came with a number not finite: not drawn", what);
    }
}
// The gun drawn nowhere (HandTargets::stowed) as that starts and ends, for the films: how long, and how many of its draws were left out, by kind. At most
// kStowedLines of these lines a second, each flushed to the file under the hands' lock: a hand going in and out of sight at a place could start and end it every
// frame. The rest are counted in the next one said. Under g_handsMutex, once a frame (SetHands).
void NoteStowed(bool stowed) {
    if (stowed == g_stowedWas)
        return;
    g_stowedWas = stowed;
    const ULONGLONG now = GetTickCount64();
    if (stowed) {
        g_stowedSince = now;
        for (std::atomic<uint32_t>& n : g_stowedSkips)
            n.store(0, std::memory_order_relaxed);
    }
    if (now - g_stowedSecond >= 1000)
        g_stowedSecond = now, g_stowedSaid = 0;
    if (g_stowedSaid >= kStowedLines) {
        ++g_stowedUnsaid;
        return;
    }
    ++g_stowedSaid;
    char unsaid[64] = "";
    if (g_stowedUnsaid)
        std::snprintf(unsaid, sizeof(unsaid), " (%u more starts and ends before it without a line)", g_stowedUnsaid);
    g_stowedUnsaid = 0;
    if (stowed) {
        Log("Hands: the gun drawn nowhere (put away, or its hand not seen at a place): its draws left out, what lies and what a hand holds drawn as before%s", unsaid);
        return;
    }
    uint32_t n[kStowedKinds];
    for (int k = 0; k < kStowedKinds; ++k)
        n[k] = g_stowedSkips[k].load(std::memory_order_relaxed);
    Log("Hands: the gun drawn nowhere for %.2f s, no more%s: %u of its draws left out (the rifle's body %u, its parts %u, a launcher %u, another gun %u)%s",
        static_cast<double>(now - g_stowedSince) / 1000.0, g_handsOn ? "" : " (free hands off: the game's own arms and gun)",
        n[kStowedBody] + n[kStowedParts] + n[kStowedLauncher] + n[kStowedHeld], n[kStowedBody], n[kStowedParts], n[kStowedLauncher], n[kStowedHeld], unsaid);
}
// The move found this frame: the gun's (and the right hand's on it), from its body bone, for draws
// that don't have that bone.
float g_gunDelta[12] = {};
bool g_haveGunDelta = false;
// What the effects take: the same move to where the world's picture shows the gun (HandTargets::gunSeen), and the origin of the
// arms' space it's in.
struct EffectMove {
    float delta[12] = {};
    float origin[3] = {};
    bool haveOrigin = false;
    void Note(const skin::HandTargets& t, const float* seen, const float* inverse) {
        Compose(seen, inverse, delta);
        std::memcpy(origin, t.seenOrigin, sizeof(origin));
        haveOrigin = t.seenHaveOrigin;
    }
};
EffectMove g_gunEffect;
ULONGLONG g_gunDeltaTime = 0;  // when it was last worked out: stale once the gun isn't moved
// The same for the train's pistol (DrawBarePistol), and its frame bone as the game has it.
EffectMove g_pistolEffect;
float g_pistolFrameOrig[12] = {};
ULONGLONG g_pistolDeltaTime = 0;
// And for the launcher (DrawLauncher).
EffectMove g_launcherEffect;
float g_launcherFrameOrig[12] = {};
ULONGLONG g_launcherDeltaTime = 0;
constexpr float kEyeHeight = 1.6f;  // a dropped magazine's floor this far below the eye (standing)
// Bones placed from a bone of another draw (the gun's body): their offset from it, learned as
// they're drawn. A move worked out in that other draw and reused here was a frame off whenever the
// game's own gun bobbed (walking): black copies, flicker (user's video).
struct Learned {
    float rel[kPaletteBones][12] = {};
    bool have[kPaletteBones] = {};
};
std::unordered_map<DrawKey, Learned, DrawKeyHash> g_learned;  // under g_handsMutex
// The game's own left glove bones (handpose.cpp's 0-22) in its gun's model space, as its animation
// last held the gun (under g_handsMutex). Kept, not timed out: with a half-second limit the grip
// fell back to another between learnings and the two flickered (user, 2026-10-30).
float g_gameLeft[handpose::kBones][12] = {};
bool g_haveGameLeft = false;
// Which frame the gun's body was drawn in (the count of view uploads then: a frame's start uploads
// its views, its draws follow). An offset is only learned from a reference of the same frame:
// across frames the arms' space moves a frame's walk, or a whole metre when its origin jumps, and
// the right glove, scope and stock jumped about while walking (user's video, 2026-10-30).
uint32_t g_gunBodyView = 0;
// Draws of the gun and arms that couldn't be moved, since the last log: no copy of their bones,
// someone else's, or BF3's bone buffer not bound.
std::atomic<uint32_t> g_handsUnmoved{0}, g_handsMovedDraws{0};
std::atomic<uint32_t> g_heldMagDraws{0};  // a magazine in either hand drawn (pouch.cpp), since the last log
// ... by draw and why: 0 no copy of a whole bone buffer, 1 someone else's bones, 2 BF3's buffer not
// bound for the draw (the copy is of another upload).
std::mutex g_unmovedMutex;
std::map<std::tuple<UINT, UINT, int>, uint32_t> g_unmovedWhy;

void NoteUnmoved(const DrawKey& key, int why) {
    ++g_handsUnmoved;
    std::lock_guard lock(g_unmovedMutex);
    ++g_unmovedWhy[{key.indexCount, key.startIndex, why}];
}

// A bone's offset from a reference bone (both as the game drew them in the same frame).
void Learn(Learned& l, size_t k, const float* referenceInverse, const float* bone) {
    Compose(referenceInverse, bone, l.rel[k]);
    l.have[k] = true;
}

// Affine 3x4 matrices: three rows, rotation | translation.
void Compose(const float* a, const float* b, float* out) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j)
            out[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        out[i * 4 + 3] += a[i * 4 + 3];
    }
}

bool Invert(const float* m, float* out) {
    float a = m[0], b = m[1], c = m[2], d = m[4], e = m[5], f = m[6], g = m[8], h = m[9], k = m[10];
    float det = a * (e * k - f * h) - b * (d * k - f * g) + c * (d * h - e * g);
    if (!(std::fabs(det) > 1e-8f))
        return false;
    float r[9] = {(e * k - f * h) / det, (c * h - b * k) / det, (b * f - c * e) / det,
                  (f * g - d * k) / det, (a * k - c * g) / det, (c * d - a * f) / det,
                  (d * h - e * g) / det, (b * g - a * h) / det, (a * e - b * d) / det};
    for (int i = 0; i < 3; ++i) {
        out[i * 4] = r[i * 3], out[i * 4 + 1] = r[i * 3 + 1], out[i * 4 + 2] = r[i * 3 + 2];
        out[i * 4 + 3] = -(r[i * 3] * m[3] + r[i * 3 + 1] * m[7] + r[i * 3 + 2] * m[11]);
    }
    return true;
}

// A bone moved by delta (in place).
void MoveBy(const float* delta, float* bone) {
    float moved[12];
    Compose(delta, bone, moved);
    std::memcpy(bone, moved, sizeof(moved));
}

// The rifle's ejection port cover (bone 3) swung open by angle (radians) on its hinge: the rod
// along its bottom edge, at this x and y in the gun's model space (x left, y up, z to the muzzle;
// the mesh dump as posed, 2026-10-30). Opening, the top swings out to the right and down. bones
// is the gun's palette, its body (bone 2) already where it's drawn.
void SwingCover(float* bones, float angle) {
    constexpr float kHinge[2] = {-0.021f, 0.033f};
    if (angle <= 0.0f)
        return;
    const float c = std::cos(angle), s = std::sin(angle), hx = kHinge[0], hy = kHinge[1];
    // About the model's z axis through the hinge: p' = R (p - h) + h.
    const float turn[12] = {c, -s, 0.0f, hx - (c * hx - s * hy),
                            s, c,  0.0f, hy - (s * hx + c * hy),
                            0.0f, 0.0f, 1.0f, 0.0f};
    const float* body = bones + 2 * 12;
    float inverse[12], local[12], around[12], moved[12];
    if (!Invert(body, inverse))
        return;
    Compose(body, turn, local);
    Compose(local, inverse, around);  // the turn where the gun is: body, turn, body inverse
    Compose(around, bones + 3 * 12, moved);
    std::memcpy(bones + 3 * 12, moved, sizeof(moved));
}

// A bone whose vertices all land on one point: folded away.
void Collapse(float* bone, const float* point) {
    std::memset(bone, 0, 12 * sizeof(float));
    bone[3] = point[0], bone[7] = point[1], bone[11] = point[2];
}

// The first-person arms' draws (the mesh dumps, 2026-10-30), each with a palette of its own, and
// what each palette entry is, from where its vertices sit in the bind pose: L / R a hand's bone,
// moved with that hand; l / r that side's arm or sleeve, folded into the middle of its forearm
// (kForearmMiddle); c the body and legs, folded onto the line between those two fold points
// (zero-area triangles, never drawn); '-' no vertices.
// The bare forearms' draw is moved whole with its hand: folded into the palm, its far end's
// triangles stretched to it lit black (a folded bone's matrix is all zeros, its normals too) and the
// depth passes' differently cut copy of them left black spikes on the left forearm below the watch
// (the user's picture, 2026-10-31). Now the forearm is whole; the depth passes' arms fold into the
// middle of it, inside the drawn forearm, where their stretched triangles are hidden, except looking
// into the forearm's open elbow end along it: there a black disc shows (depth without colour).
// Wrist movement, every level: the forearm's bones are placed where the arm solved from the hand and a shoulder puts them (armik.h, hands.cpp), so the hand bends at the wrist;
// the glove's cuff goes with the forearm, and the folds stay in its middle.
struct ArmDraw {
    DrawKey key;
    const char* roles;
    int rightRoot = -1;  // the right hand's root bone, where the draw has it (for the gun's grip)
    // Which of the gloves' left-hand bones (handpose.cpp's 0-22) each 'L' entry is, where the
    // numbering isn't the gloves' own (-1: none, moved with the glove's cuff).
    const int8_t* leftBones = nullptr;
    size_t leftBoneCount = 0;
    // A gun part placed as another draw is (the scope's parts as its body): drawn only in a later
    // pass, where its own offsets from the gun are never learned, it takes that draw's.
    DrawKey placedLike{};
    // With a forearm solved from its hand (HandTargets::arm), what each entry is
    // instead (kGlovesForearm and the others below): 0-2 the solved forearm's elbow end and twist
    // bones A, B; 3 its upper arm; 4 its collar bone; 10 + g the left glove's bone g (handpose.cpp's), as the gloves draw it; -1 placed as
    // without.
    const int8_t* forearm = nullptr;
    size_t forearmCount = 0;
};
// Matched by vertices at the same place in the gloves' mesh dump and these depth passes', so the
// fingers bend alike in every pass. The bare forearms' 'L' entries go with the glove bone their depth
// copy (in the body's depth pass) is drawn with: 2 with the cuff's twist bone 4 (the body's depth 19),
// 4 with bone 0 (20), 8 with the hand's bone 1 (21), 9 with 18 (38), the rest (their depth copies fold
// inside the forearm) with the forearm's bone 5. All on bone 0 before, the forearm and its depth copy
// were drawn up to 33 degrees apart while the glove held the handguard (the game's grip turns the
// cuff's bones apart): black patches below the watch (the user's picture, 2026-10-31 08:04).
constexpr int8_t kGloveDepthLeft[] = {-1, -1, 14, 13, 16, -1, 8, 9, 19, 15, 18, 17, 2};
constexpr int8_t kBodyDepthLeft[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 4,
                                     0,  1,  6,  7,  21, 22, 12, 11, 20, 10, 14, 13, 16, 3,  8,  9,  19, 15, 18, 17, 2};
constexpr int8_t kForearmLeft[] = {5, 5, 4, 5, 0, -1, 5, -1, 1, 18};
// The bare forearms' 'R' entries (10 on) as the right glove's bones, for the right glove drawn by its own pose (skin::kRightOwn): kForearmLeft's twins (the
// left's 5 and 4 are the right's 44, its 0 the right's 37), and 17, at the wrist, with the hand's frame (32, the left's 1 mirrored). -1: no vertices.
constexpr int8_t kForearmRight[] = {44, 44, 44, 44, 37, -1, 44, 32};
// Wrist movement (armik.h; the judge's synthesis, 2026-10-31): the entries that are a forearm's bones
// in the game's rig, matched by vertices at the same bind place with the same weights in the dumps,
// so every pass draws them alike (a depth copy placed apart from its colour shows black). The right's alike, looked at again in the dumps 2026-11-04. The gloves:
// the cuff, bone 0 on twist bone B (the body's depth 20), 4 on A (19), 5 on the forearm's own bone (18: 3 vertices); the right's 37 on B, 44 on A.
constexpr int8_t kGlovesForearm[] = {2,  -1, -1, -1, 1,  0,  -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                     -1, -1, -1, -1, -1, -1, -1, 2,  -1, -1, -1, -1, -1, -1, 1};
// The gloves' depth pass: the right twist bones only (the left forearm isn't in it; 16 is the bare forearm's 12, 17 its 14; the bare forearm's 10, 11, 13 and 16 are this pass's 15, 13, 38
// and 14, which fold inside the forearm as the left's do in the body's depth pass).
constexpr int8_t kGloveDepthForearm[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                         3,  3,  0,  1,  2,  -1, -1, -1, -1, -1, -1, -1, -1,
                                         -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0};
// The chest and sleeves: each arm's collar bone, upper arm (two bones of the game's) and elbow end, by where their vertices lie in the bind pose (2026-11-04).
constexpr int8_t kSleevesForearm[] = {-1, 3, 3, 4, 0, 1, 0, -1, 4, 3, 0, 1, 3, 0};
// The bare forearms: the forearm's bone and its helpers (the body's depth 18/16/41/17) on the elbow
// end, 2 on A (19), 4 on B (20), and the two at the wrist with the glove's bones they share vertices
// with (8 its bone 1, the body's depth 21; 9 its bone 18, 38); then the right's alike (its 17, at the wrist, goes with the hand's frame as an 'R' does).
// 1 and 6 (and the right's 11 and 16) are the upper arm's bones, its 8529's 1 and 2: on the upper arm with it whole, or the elbow's seam tore from the sleeve's.
constexpr int8_t kBareForearm[] = {0, 3, 1, 0, 2, -1, 3, -1, 11, 28, 0, 3, 1, 0, 2, -1, 3, -1};
// The body's depth pass: the left arm whole (15 its collar bone, 16 and 17 its upper arm, 18 and 41 its elbow end, 19 and 20 its twist bones), and the right's collar bone, upper arm and
// elbow end (42, 43 and 44, 45, 46 and 54).
constexpr int8_t kBodyDepthForearm[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                        -1, 4,  3,  3,  0,  1,  2,  -1, -1, -1, -1, -1, -1, -1,
                                        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,
                                        4,  3,  3,  0,  1,  -1, -1, -1, -1, -1, -1, -1, 0};
// The right hand root's vertices' centre in the gloves' bind pose (the mesh dump).
constexpr float kRightPalm[3] = {-0.505f, 1.129f, 0.150f};
// The middle of each forearm in the bind pose, where its bare part's elbow end joins its wrist end
// (the bare forearms' and the body depth's dumps, 2026-10-31): left, then right.
constexpr float kForearmMiddle[2][3] = {{0.440f, 1.215f, 0.061f}, {-0.440f, 1.215f, 0.061f}};
constexpr DrawKey kBareForearms{2016, 13035, 0};
// How far the trigger finger curls past the game's grip with the trigger pulled all the way
// (degrees over its three joints: about 20 mm at the tip, handpose::BendRightIndex).
constexpr float kTriggerBend = 22.0f;
// The gun's body bone as the game animated it, and from it and the right hand, where the game's
// hand holds the gun in the gun's model space (smoothed).
float g_gunBodyOrig[12] = {};
bool g_haveGunBodyOrig = false;
// The magazine's seat in the gun: each of its bones' (kMagazineBones) offset from the gun's body as
// the game draws it in the well, learned while the game holds its gun normally (its reload moves the
// magazine about). A magazine in either hand is drawn at its pose times these.
float g_magSeat[kMagBones][12] = {};
bool g_haveMagSeat[kMagBones] = {};
float g_gripModel[3] = {};
bool g_haveGrip = false;
// The M416's parts on its body, as they always are on foot (weapon.txt "gunpartlog", 2026-11-04; also through the game's own reload). Seated in the car at the start of Rock and a
// Hard Place the game has them elsewhere: the grenade launcher's tube (7848's bone 1) 8 cm back, 6 cm down and turned a quarter about its length, hanging out under the launcher's frame
// with its window empty; the grenade (1440's bone) 14 cm along; the magazine 2 cm; and they slide back as the soldier gets out (the user, 2026-11-04: "It's the weirdest thing ever, and
// it's not supposed to do that"). Kept here whatever the game's pose. Within limit of these only: another gun with draws of these sizes isn't touched.
struct PartRest {
    DrawKey key;
    size_t bone;
    float at[3];
    unsigned tells;  // off here, with the other that tells: the soldier is seated
};
constexpr unsigned kTellsLauncher = 1, kTellsFront = 2, kTellsSeated = kTellsLauncher | kTellsFront;
constexpr PartRest kPartRests[] = {
    {{7848, 0, 0}, 0, {0.000f, 0.008f, -0.142f}, 0},
    {{7848, 0, 0}, 1, {-0.019f, 0.012f, 0.454f}, kTellsFront},
    {{1440, 7848, 0}, 0, {0.015f, 0.023f, 0.231f}, kTellsLauncher},
    {{9288, 9288, 0}, 0, {0.000f, 0.008f, -0.142f}, 0},
    {{9288, 9288, 0}, 1, {-0.019f, 0.012f, 0.454f}, kTellsFront},
    {{9288, 9288, 0}, 2, {0.015f, 0.023f, 0.231f}, kTellsLauncher},
};
constexpr float kPartRestLimit = 0.25f;
constexpr float kPartOff = 0.003f;  // the game's own values never leave these by a millimetre on foot

// The grenade launcher's barrel swung open by hand (its reload by hand: the user, 2026-11-05, "I also want a manual reload for this M320 launcher"). The real M320's breech end
// swings out to the gun's left about a vertical pin at the front of the barrel, on its right (its army manual, TM 3-22.31; H&K's patent); the game's own reload swings this tube
// the same way, 15 degrees, about the axis (0,-1,0) through (-0.018, *, 0.751) in the gun's model space, its breech 6.7 cm out (build\survey\M320_vanilla.bin, 4.44-4.66 s).
// Radians, 0 shut (g_m320Barrel). The round in it: there or not (fired and pulled, or tipped out), and slid this far back out of it along the open barrel (metres): g_m320RoundIn,
// g_m320RoundOut (declared before ReadWeaponSelection, whose test lines set them).
constexpr float kM320Hinge[3] = {-0.018f, 0.0f, 0.751f};
constexpr float kM320OpenMost = 15.0f * 3.14159265f / 180.0f;

// Which of the launcher's parts a 'G' bone is: 1 its tube, 2 its round (the colour pass's and the depth pass's), 0 neither.
int M320PartOf(const DrawKey& key, size_t bone) {
    if ((key == DrawKey{7848, 0, 0} || key == DrawKey{9288, 9288, 0}) && bone == 1)
        return 1;
    if ((key == DrawKey{1440, 7848, 0} && bone == 0) || (key == DrawKey{9288, 9288, 0} && bone == 2))
        return 2;
    return 0;
}

// The turn about the hinge, in the gun's model space: rows rotation | translation.
void M320Hinge(float angle, float* out) {
    const float c = std::cos(angle), s = std::sin(angle);
    const float r[9] = {c, 0.0f, -s, 0.0f, 1.0f, 0.0f, s, 0.0f, c};  // about -y: the breech end (behind the pin) goes to +x, the gun's left
    for (int i = 0; i < 3; ++i) {
        out[i * 4] = r[i * 3], out[i * 4 + 1] = r[i * 3 + 1], out[i * 4 + 2] = r[i * 3 + 2];
        out[i * 4 + 3] = kM320Hinge[i] - (r[i * 3] * kM320Hinge[0] + r[i * 3 + 1] * kM320Hinge[1] + r[i * 3 + 2] * kM320Hinge[2]);
    }
}

// The round as the game's own animation has it (its learned offset from the gun's body at its last draw: g_handsMutex), for hands.cpp's watch of the game's own reload; and
// that reload's old case let go of (loose has it: hidden here until the hand comes back up with the new one).
float g_m320RoundSeen[12] = {};
ULONGLONG g_m320RoundSeenAt = 0;
std::atomic<bool> g_m320CaseDropped{false};
// The launcher's tube met with the player's bones (any draw of it, the mod's hands on or not: DrawIndexed): it's on this rifle, for as long as the rifle
// is out (forgotten once no rifle has been drawn for 10 s: Update), and when it was last. A later level's M416 without one is let go of as its body
// without it (skin::RifleM320Seen).
std::atomic<bool> g_m320Met{false};
std::atomic<ULONGLONG> g_m320MetAt{0};
constexpr ULONGLONG kM320Forget = 10000;

// A launcher part's offset from the gun's body as the barrel and round are now (rel: as the game has it, shut and loaded). False: the round isn't there (drawn on one point).
// The reload by hand's when it has them (t.m320Own: m320.h), else weapon.txt's test lines (as the game has them, untouched).
bool M320PartNow(const skin::HandTargets& t, int part, const float* rel, float* out) {
    float roundOut = 0.0f;
    if (part == 2) {
        if (g_m320CaseDropped.load(std::memory_order_relaxed))  // (the game's own reload's old case let go of: loose has it)
            return false;
        if (t.m320Own) {
            if (t.m320Round == 1)
                return false;
            if (t.m320Round == 2) {  // on its way home by itself, off the barrel's line
                std::memcpy(out, t.m320RoundOnGun, 12 * sizeof(float));
                return true;
            }
            roundOut = t.m320RoundOut;
        } else {
            if (!g_m320RoundIn.load(std::memory_order_relaxed))
                return false;
            roundOut = g_m320RoundOut.load(std::memory_order_relaxed);
        }
    }
    float moved[12];
    std::memcpy(moved, rel, sizeof(moved));
    if (part == 2)  // slid back along the barrel's own length (its axis is the gun's z, shut)
        moved[11] -= roundOut;
    float hinge[12];
    M320Hinge(t.m320Own ? t.m320Barrel : g_m320Barrel.load(std::memory_order_relaxed), hinge);
    Compose(hinge, moved, out);
    return true;
}
// The rifle's own bones are the game's to move (its reload moves the magazine, and only that), but not seated. In the colour pass (0) and the depth pass (2); bone 5 of the depth pass
// and the one bone of the rounds' own draw (pass 1) are the rounds.
struct BoneRest {
    size_t pass;
    UINT bone;
    float at[3];
};
constexpr BoneRest kBoneRests[] = {
    {0, 1, {0.000f, 0.045f, 0.175f}}, {0, 3, {-0.022f, -0.082f, 0.415f}}, {0, 4, {0.000f, 0.131f, 0.343f}},
    {2, 1, {0.000f, 0.045f, 0.175f}}, {2, 3, {-0.022f, -0.082f, 0.415f}}, {2, 4, {0.000f, 0.131f, 0.343f}}, {2, 5, {0.000f, 0.173f, 0.309f}},
};
constexpr float kRoundsRest[3] = {0.000f, 0.173f, 0.309f};
constexpr float kBoneRestLimit = 0.10f;
std::atomic<unsigned> g_partsOff{0};
std::atomic<ULONGLONG> g_partsToldAt{0};  // when the M416's parts that tell were last placed (PinPart)

// Seated (the car at Rock and a Hard Place's start): the M416's own parts tell it, so only while they're drawn. Putting the rifle away sets the tell, and
// with the rifle away nothing clears it: the SMAW in hand stayed asleep, "the rifle's seated pose" (free hands' films, 2026-11-11).
bool GunSeated() {
    return g_partsOff.load(std::memory_order_relaxed) == kTellsSeated && GetTickCount64() - g_partsToldAt.load(std::memory_order_relaxed) < 500;
}

void PinPart(const DrawKey& key, size_t bone, float* rel) {
    for (const PartRest& p : kPartRests) {
        if (!(p.key == key) || p.bone != bone)
            continue;
        const float off = Length3(rel[3] - p.at[0], rel[7] - p.at[1], rel[11] - p.at[2]);
        if (off > kPartRestLimit)
            return;
        if (p.tells) {
            // The tube's tell: the car's own quarter turn about its length (its y axis level: rel[5] near 0). The launcher's own reload swings it 15 degrees about the gun's
            // y (rel[5] stays 1) and was taken for seated, the barrel pinned shut through it (the review, 2026-11-05). Once set, the tell stays until the tube is home, so
            // the slide back out of the car stays pinned as before. The round's tell as before.
            const bool home = off <= kPartOff;
            const bool away = p.tells == kTellsFront ? rel[5] < 0.7f : !home;
            if (away)
                g_partsOff |= p.tells;
            else if (home)
                g_partsOff &= ~p.tells;
            g_partsToldAt.store(GetTickCount64(), std::memory_order_relaxed);
        }
        if (GunSeated() && g_partPin.load(std::memory_order_relaxed)) {  // all of it: lined up with the body, as on foot (the tube is turned as well)
            const float rest[12] = {1.0f, 0.0f, 0.0f, p.at[0], 0.0f, 1.0f, 0.0f, p.at[1], 0.0f, 0.0f, 1.0f, p.at[2]};
            std::memcpy(rel, rest, sizeof(rest));
        }
        return;
    }
}

// The rifle's bones in one of its draws as the game sent them, with its body's inverse: seated, back to where they are on foot.
void PinSeatedBones(size_t pass, const float* inverse, float* bones) {
    if (!GunSeated())
        return;
    const float* body = bones + 2 * 12;
    for (const BoneRest& r : kBoneRests) {
        if (r.pass != pass)
            continue;
        float* b = bones + r.bone * 12;
        const float p[3] = {b[3], b[7], b[11]};
        float d[3];
        for (int i = 0; i < 3; ++i)
            d[i] = r.at[i] - (inverse[i * 4] * p[0] + inverse[i * 4 + 1] * p[1] + inverse[i * 4 + 2] * p[2] + inverse[i * 4 + 3]);
        if (Length3(d[0], d[1], d[2]) < kBoneRestLimit)
            for (int i = 0; i < 3; ++i)
                b[i * 4 + 3] += body[i * 4] * d[0] + body[i * 4 + 1] * d[1] + body[i * 4 + 2] * d[2];
    }
}

// The game's hands on its gun are copied (the right hand's grip and bones, the left hand's hold for
// the handguard) only while it holds the gun as it normally does: its sprint moves both hands on the
// gun, and copying that slid the gloves about on it and swung the gun in the hand twice a second (the
// user's videos, 2026-10-31 02:49 and 02:51). Normal: not sprinting (nor within a second of it), the
// game's right palm within 3 cm of its grip so far and its left within 4 cm of its hold. A hold kept
// steadily elsewhere for 5 s is taken as the new one (another hold to begin with, say). The verdict
// is judged in the gloves' draw; every draw of a frame learns by the one of the frame before
// (g_learnHold), so all of them follow the game's hands or keep theirs together (a depth copy a frame
// apart from the coloured glove shows black).
constexpr float kLeftPalm[3] = {0.537f, 1.098f, 0.207f};  // the left hand's centre in the gloves' bind pose
constexpr float kRightHoldTolerance = 0.03f, kLeftHoldTolerance = 0.04f;
std::atomic<bool> g_holdNormal{true};
std::atomic<bool> g_learnHold{true};  // g_holdNormal and not sprinting, at the last frame's end (Update)
ULONGLONG g_armsSeenAt = 0, g_learnAgainAt = 0;  // when the arms were last learned from, when learning goes on again after a gap (LearnArms)
// This frame's hand bones on the gun, waiting for its verdict (LearnArms, TakeFrameOfHands). Under g_handsMutex.
std::unordered_map<DrawKey, Learned, DrawKeyHash> g_learnedNext;
float g_gameLeftNext[handpose::kBones][12] = {};
bool g_gameLeftNextWhole = false;
// The launcher under the rifle in hand: the game's left hand goes onto its own pistol grip (palm at kM320Palm in the M416's model space: the vanilla select recorded 2026-11-05,
// build\survey\M320_vanilla.bin, 0.94-1.48 s after the press). That pose is the hand's second hold (hands.cpp), learned apart; never taken as the handguard's.
constexpr float kM320Palm[3] = {0.040f, -0.117f, 0.490f};
float g_gameLeftM320[handpose::kBones][12] = {}, g_gameLeftM320Next[handpose::kBones][12] = {};
bool g_haveGameLeftM320 = false, g_gameLeftM320NextWhole = false;
bool g_leftOnM320 = false;  // this draw's game left hand is on the launcher's grip
// Kept beside the DLL once learned (build\m320grip.bin): with the launcher in the game's hand only for the moment of a shot (input.cpp), its hand may never get there in a run.
std::wstring BesideDll(const wchar_t* name);
ULONGLONG g_m320GripLearnedAt = 0, g_m320GripDirtySince = 0;
bool g_m320GripDirty = false, g_m320GripLoadTried = false, g_m320GripSaved = false;
constexpr uint32_t kM320GripMagic = 0x3032334Du;  // "M320"

void SaveM320Grip() {
    FILE* f = nullptr;
    if (_wfopen_s(&f, BesideDll(L"m320grip.bin").c_str(), L"wb") != 0 || !f)
        return;
    const uint32_t head[2] = {kM320GripMagic, static_cast<uint32_t>(handpose::kBones)};
    const bool ok = fwrite(head, sizeof(head), 1, f) == 1 && fwrite(g_gameLeftM320, sizeof(g_gameLeftM320), 1, f) == 1;
    fclose(f);
    if (!ok || !g_m320GripSaved)
        Log("Hands: the game's left hand on the grenade launcher's grip %s build\\m320grip.bin", ok ? "kept in" : "couldn't be written to");
    g_m320GripSaved = g_m320GripSaved || ok;
}

void LoadM320Grip() {
    g_m320GripLoadTried = true;
    FILE* f = nullptr;
    if (_wfopen_s(&f, BesideDll(L"m320grip.bin").c_str(), L"rb") != 0 || !f)
        return;
    uint32_t head[2] = {};
    float bones[handpose::kBones][12];
    const bool ok = fread(head, sizeof(head), 1, f) == 1 && head[0] == kM320GripMagic && head[1] == handpose::kBones && fread(bones, sizeof(bones), 1, f) == 1;
    fclose(f);
    if (ok) {
        std::memcpy(g_gameLeftM320, bones, sizeof(bones));
        g_haveGameLeftM320 = true;
        Log("Hands: the game's left hand on the grenade launcher's grip read from build\\m320grip.bin");
    }
}
bool g_frameJudged = false, g_frameHoldNormal = true;  // the hold judged this frame, and good in every draw that judged it
float g_leftHoldModel[3] = {};  // the game's left palm on its gun, held normally
bool g_haveLeftHold = false;
// The first hold after loading (and one newly taken) is provisional until both palms stay put.
bool g_holdSettled = false;
ULONGLONG g_settledMs = 0;
// Where the palms have been instead (right, left), for how long the game was seen running.
float g_elsewhere[2][3] = {};
bool g_haveElsewhere = false;
ULONGLONG g_elsewhereMs = 0;
// Only time the game is seen running counts: when the hold was last judged, and since when the
// game's gun has stood bit for bit still from frame to frame (a paused game still drawn).
ULONGLONG g_lastJudged = 0, g_frozenSince = 0;
uint32_t g_judgedFrame = 0;
float g_lastBody[12] = {};
// Judgements of the hold since the last log (Update): in the gloves' own draw, at the gun's draw
// after them, and gloves drawn with no gun body in their view to judge by.
uint32_t g_judgedHere = 0, g_judgedLater = 0, g_notJudged = 0;

// most: it doesn't follow a palm further off than this. Followed all the way within the tolerance, the put-away's slow start dragged the hold 6-8 cm along, and every return
// to the rifle then ran the 5 s rule with the handguard's grip that far off (the review, 2026-11-04).
void FollowLeftHold(const float* left, float most = 1e9f) {
    if (!left)
        return;
    if (g_haveLeftHold && Length3(left[0] - g_leftHoldModel[0], left[1] - g_leftHoldModel[1], left[2] - g_leftHoldModel[2]) > most)
        return;
    for (int i = 0; i < 3; ++i)
        g_leftHoldModel[i] = g_haveLeftHold ? g_leftHoldModel[i] * 0.9f + left[i] * 0.1f : left[i];
    g_haveLeftHold = true;
}

// Whether the game holds its gun as it normally does in this draw, from its palms in the gun's model
// space (left: none in this draw); the normal hold follows small changes. Under g_handsMutex.
bool HoldIsNormal(const float* right, const float* left) {
    const ULONGLONG now = GetTickCount64();
    const ULONGLONG step = now - g_lastJudged;
    g_lastJudged = now;
    if (const uint32_t frame = g_frameSerial.load(std::memory_order_relaxed); frame != g_judgedFrame) {
        g_judgedFrame = frame;
        const bool same = std::memcmp(g_lastBody, g_gunBodyOrig, sizeof(g_lastBody)) == 0;
        std::memcpy(g_lastBody, g_gunBodyOrig, sizeof(g_lastBody));
        g_frozenSince = !same ? 0 : g_frozenSince ? g_frozenSince : now;
    }
    // A gap between judgements (a pause, the rifle put away) or a quarter of a second frozen: not
    // seen running, any time counted starts over. (One still frame alone isn't a pause: the game's
    // animation may step less often than it draws.)
    const bool live = step < 250 && !(g_frozenSince && now - g_frozenSince > 250);
    const float dr = !g_haveGrip ? 0.0f
                                 : Length3(right[0] - g_gripModel[0], right[1] - g_gripModel[1], right[2] - g_gripModel[2]);
    const float dl = !g_haveLeftHold || !left ? 0.0f
                                              : Length3(left[0] - g_leftHoldModel[0], left[1] - g_leftHoldModel[1],
                                                        left[2] - g_leftHoldModel[2]);
    const bool sprinting = input::Sprinting();
    // The launcher under the rifle in hand, or just put away (the game's hand still on its way back): the left hand there is its own hold (learned apart), never the gun's,
    // not even provisionally. (Going back to the handguard, a hold learned from the hand's last frames on the way had its fingers still moving: the user's "clicks first, and then
    // it refixes itself", 2026-11-05.)
    if (ammo::M320UpOrRecent()) {
        g_haveElsewhere = false;
        g_settledMs = 0;
        return false;
    }
    if (!g_holdSettled) {
        // Provisional: the first frames may be mid-reload or mid-raise, so the hold follows the game's
        // hands (and they're copied, as before this guard) until both palms stay within 1 cm of it
        // for a second of the game running. Not while sprinting.
        g_haveElsewhere = false;
        if (sprinting) {
            g_settledMs = 0;
            return false;
        }
        // ... but a jump isn't a hold (the last frame of a put-away, half a metre off), unless they stay there 2 s of the game running: then it is one.
        constexpr float kHoldJump = 0.12f;
        static ULONGLONG s_jumpedMs = 0;
        if ((g_haveGrip && dr > kHoldJump) || (g_haveLeftHold && left && dl > kHoldJump)) {
            s_jumpedMs = live ? s_jumpedMs + step : s_jumpedMs;
            if (s_jumpedMs < 2000) {
                g_settledMs = 0;
                return false;
            }
            g_haveGrip = g_haveLeftHold = false;
        }
        s_jumpedMs = 0;
        g_settledMs = live && dr < 0.01f && dl < 0.01f ? g_settledMs + step : 0;
        if (g_settledMs > 1000) {
            g_holdSettled = true;
            Log("Hands: the game's hold on its gun learned: the right palm at (%.3f %.3f %.3f) in its model space",
                g_gripModel[0], g_gripModel[1], g_gripModel[2]);
        }
        FollowLeftHold(left);
        return true;
    }
    if (!sprinting && dr < kRightHoldTolerance && dl < kLeftHoldTolerance) {
        g_haveElsewhere = false;
        FollowLeftHold(left, 0.01f);
        return true;
    }
    static ULONGLONG s_told = 0;
    if (now - s_told > 1000) {
        s_told = now;
        Log("Hands: the game's hands are off their usual hold on its gun (%s; right palm %.0f mm away, left %.0f mm): "
            "the gloves keep theirs",
            sprinting ? "sprinting" : "one of its moves", dr * 1000.0f, dl * 1000.0f);
    }
    // Held elsewhere: one of its moves, unless both palms stay there for 5 s of the game running
    // (never while sprinting). Then that's the hold, on the side that's off; it's provisional again.
    const bool moved =
        !g_haveElsewhere ||
        Length3(right[0] - g_elsewhere[0][0], right[1] - g_elsewhere[0][1], right[2] - g_elsewhere[0][2]) > 0.01f ||
        (left && Length3(left[0] - g_elsewhere[1][0], left[1] - g_elsewhere[1][1], left[2] - g_elsewhere[1][2]) > 0.02f);
    if (sprinting || !live || moved) {
        std::memcpy(g_elsewhere[0], right, sizeof(g_elsewhere[0]));
        if (left)
            std::memcpy(g_elsewhere[1], left, sizeof(g_elsewhere[1]));
        g_haveElsewhere = !sprinting;
        g_elsewhereMs = 0;
        return false;
    }
    g_elsewhereMs += step;
    if (g_elsewhereMs > 5000) {
        Log("Hands: the game held its gun elsewhere for 5 s (right palm %.0f mm from its grip, left %.0f mm): taken as "
            "its hold",
            dr * 1000.0f, dl * 1000.0f);
        if (dr >= kRightHoldTolerance)
            g_haveGrip = false;
        if (dl >= kLeftHoldTolerance)
            g_haveLeftHold = false;
        g_holdSettled = false;
        g_settledMs = 0;
        g_haveElsewhere = false;
        return true;
    }
    return false;
}

constexpr ArmDraw kArmDraws[] = {
    {{13755, 15051, 0}, "LLLLLLLLLLLLLLLLLLLLLLLRRRRRRRRRRRRRRRRRRRRRR", 37, nullptr, 0, {}, kGlovesForearm,
     std::size(kGlovesForearm)},  // gloves
    {{7617, 49995, 0}, "--LLL-LLLLLLLrrrRRRRRRRRRRRRRRRRRRRRRRr", -1, kGloveDepthLeft, std::size(kGloveDepthLeft), {},
     kGloveDepthForearm, std::size(kGloveDepthForearm)},  // their depth
    {kBareForearms, "LLLLL-L-LLRRRRR-RR", -1, kForearmLeft, std::size(kForearmLeft), {}, kBareForearm,
     std::size(kBareForearm)},  // bare forearms, whole
    {{8529, 0, 0}, "cllllll-rrrrrr", -1, nullptr, 0, {}, kSleevesForearm, std::size(kSleevesForearm)},  // chest, sleeves
    {{21189, 28806, 0}, "cc-ccccccccccccllllLLLLLLLLLLLLLLLLLLLLLLlrrrrrRRRRRRRr", -1, kBodyDepthLeft,
     std::size(kBodyDepthLeft), {}, kBodyDepthForearm, std::size(kBodyDepthForearm)},  // body's depth
    {{4506, 8529, 0}, "c---ccccccccccccc", -1},  // legs
};
// Each forearm map has an entry for each of its draw's palette entries.
static_assert(std::char_traits<char>::length(kArmDraws[0].roles) == std::size(kGlovesForearm) &&
              std::char_traits<char>::length(kArmDraws[1].roles) == std::size(kGloveDepthForearm) &&
              std::char_traits<char>::length(kArmDraws[2].roles) == std::size(kBareForearm) &&
              std::char_traits<char>::length(kArmDraws[3].roles) == std::size(kSleevesForearm) &&
              std::char_traits<char>::length(kArmDraws[4].roles) == std::size(kBodyDepthForearm));
// The right glove's maps cover its draws' 'R' entries: the depth passes' every entry, the bare forearms' from 10 on.
static_assert(std::char_traits<char>::length(kArmDraws[1].roles) == std::size(handpose::kGloveDepthRight) &&
              std::char_traits<char>::length(kArmDraws[4].roles) == std::size(handpose::kBodyDepthRight) &&
              std::char_traits<char>::length(kArmDraws[2].roles) == 10 + std::size(kForearmRight));

// Which of the right glove's bones (23-44) an 'R' entry of an arms' draw is, for the right glove drawn by its own pose (skin::kRightOwn): the gloves' own
// numbering, the depth passes' maps (handpose.h, matched by vertices), the bare forearms' kForearmRight; its root (37) where none is known.
int RightGloveBone(const ArmDraw& draw, size_t k) {
    int g = -1;
    if (&draw == &kArmDraws[0])
        g = static_cast<int>(k);
    else if (&draw == &kArmDraws[1])
        g = k < std::size(handpose::kGloveDepthRight) ? handpose::kGloveDepthRight[k] : -1;
    else if (&draw == &kArmDraws[4])
        g = k < std::size(handpose::kBodyDepthRight) ? handpose::kBodyDepthRight[k] : -1;
    else if (&draw == &kArmDraws[2])
        g = k >= 10 && k - 10 < std::size(kForearmRight) ? kForearmRight[k - 10] : -1;
    return g >= handpose::kRightFirst && g < handpose::kRightFirst + handpose::kRightBones ? g : 37;
}

// The gun's other skinned model (the mesh dumps: 7848 + 1440 indices drawn in colour, the 9288 of
// its depth pass; bones 0-2 near the gun's back end, the stock): it goes wherever the gun goes.
// The scope is a rigid model (no bone weights) placed by a matrix of a bone buffer of its own, which
// has the gun's turn (the scope probe, 2026-10-30). Its see-through parts (the scope trace and its
// materials, 2026-10-31): the lens tint 108 from 9486 (the dark circle), another dark lens 72 from
// 9285, the red cable on top 129 from 9357 and the reticle 12 from 0 (its red chevron texture). They
// are drawn in a later pass than the gun's body, where their own offsets from it were never learned,
// so they floated where the game holds its gun: they're placed as the body is. Each scope part moves
// its first eight bones with the gun (a part ignores the ones it doesn't use).
constexpr DrawKey kScopeBody{9273, 12, 0};
constexpr ArmDraw kGunPartDraws[] = {
    {{7848, 0, 0}, "GG", -1},
    {{1440, 7848, 0}, "G", -1},
    {{9288, 9288, 0}, "GGG", -1},
    {kScopeBody, "GGGGGGGG", -1},
    {{12, 0, 0}, "GGGGGGGG", -1, nullptr, 0, kScopeBody},
    {{72, 9285, 0}, "GGGGGGGG", -1, nullptr, 0, kScopeBody},
    {{129, 9357, 0}, "GGGGGGGG", -1, nullptr, 0, kScopeBody},
    {{108, 9486, 0}, "GGGGGGGG", -1, nullptr, 0, kScopeBody},
    {{9273, 9594, 0}, "GGGGGGGG", -1, nullptr, 0, kScopeBody},
};

const ArmDraw* FindArmDraw(const DrawKey& key) {
    for (const ArmDraw& d : kGunPartDraws)
        if (d.key == key)
            return &d;
    for (const ArmDraw& d : kArmDraws)
        if (d.key == key)
            return &d;
    return nullptr;
}

// The dropped magazine's fall, once a frame: gravity, a tumble, and a stop at the floor (its box's
// lowest corner). The arms' space the bones are in stays put in the world, except that its origin
// jumps a whole metre now and then as the player walks: that jump is taken off.
void UpdateDrop() {
    float origin[3] = {};
    bool haveOrigin = false;
    float floor = g_floor.load();
    {
        std::lock_guard lock(g_fpMutex);
        if (floor == 0.0f && g_haveViewToArms)
            floor = g_viewToArms[7] - input::StanceEyeHeight();  // the soldier's feet: the game's eye less its height in the stance
    }
    {
        std::lock_guard lock(g_fpMutex);
        if (g_haveFpOrigin) {
            std::memcpy(origin, g_fpOrigin, sizeof(origin));
            haveOrigin = true;
        }
    }
    std::lock_guard lock(g_magMutex);
    Drop& d = g_drop;
    if (!g_dropShown.load())
        return;
    // The floor in the drop's own origin's space (it was compared as the arms' space has it now: a metre off as the player walked over one).
    if (haveOrigin)
        floor += origin[1] - d.origin0[1];
    double now = Seconds();
    float dt = static_cast<float>(std::min(now - d.updatedAt, 0.05));
    d.updatedAt = now;
    if (d.falling) {
        d.vel[1] -= kGravity * dt;
        for (int i = 0; i < 3; ++i)
            d.pos[i] += d.vel[i] * dt;
        d.angle += d.spin * dt;
        AxisAngle(d.axis, d.angle, d.q);
        const float* m = d.start[0];
        float lowest = 1e9f;
        for (int c = 0; c < 8; ++c) {
            float v[3] = {c & 1 ? g_magBoxMax[0] : g_magBoxMin[0], c & 2 ? g_magBoxMax[1] : g_magBoxMin[1],
                          c & 4 ? g_magBoxMax[2] : g_magBoxMin[2]};
            float p[3];
            for (int i = 0; i < 3; ++i)
                p[i] = m[i * 4] * v[0] + m[i * 4 + 1] * v[1] + m[i * 4 + 2] * v[2] + m[i * 4 + 3] - d.c0[i];
            lowest = std::min(lowest, d.q[3] * p[0] + d.q[4] * p[1] + d.q[5] * p[2] + d.pos[1]);
        }
        if (lowest <= floor) {
            d.pos[1] += floor - lowest;
            d.falling = false;
            d.landedAt = now;
        }
    } else if (now - d.landedAt > kLieSeconds) {
        g_dropShown = false;
        return;
    }
    std::memcpy(d.centre, d.pos, sizeof(d.centre));  // (in the world with origin0: each draw places it, WorldToDrawn)
}

// The magazine falls: out of the gun's well (fromHand false), or from the hand where it was last
// drawn (let go of: either hand), starting at velocityView (the view's space; turned into the arms').
bool StartDrop(bool fromHand = false, const float* velocityView = nullptr, float spin = kSpin) {
    float toArms[9] = {};  // the head's space -> the world's, as the latest draw's picture has it (a turn only: for the hand's speed; the aim camera's turned it by how far the gun's aim is off the head)
    {
        std::lock_guard lock(g_fpMutex);
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                toArms[i * 3 + j] = g_lastSeenFrom[i * 4 + j];
    }
    std::lock_guard lock(g_magMutex);
    Drop& d = g_drop;
    bool seen[kMagBones];
    const double now = Seconds();
    for (size_t i = 0; i < kMagBones; ++i)
        seen[i] = fromHand ? d.heldSeen[i] && now - d.heldAt[i] < 0.1 : d.seen[i];
    if (fromHand)
        std::fill(std::begin(d.heldSeen), std::end(d.heldSeen), false);
    if (!seen[0])
        return false;  // not drawn there (lately)
    std::memcpy(d.start, fromHand ? d.held : d.last, sizeof(d.start));
    std::memcpy(d.started, seen, sizeof(d.started));
    // In the world with its first bone's origin (another draw's can be a metre off, the player walking over one).
    const float (*origins)[3] = fromHand ? d.heldOrigin : d.lastOrigin;
    std::memcpy(d.origin0, origins[0], sizeof(d.origin0));
    for (size_t b = 1; b < kMagBones; ++b)
        if (seen[b])
            for (int i = 0; i < 3; ++i)
                d.start[b][i * 4 + 3] += origins[b][i] - d.origin0[i];
    const float* m = d.start[0];
    float cm[3] = {(g_magBoxMin[0] + g_magBoxMax[0]) / 2, (g_magBoxMin[1] + g_magBoxMax[1]) / 2, (g_magBoxMin[2] + g_magBoxMax[2]) / 2};
    d.spin = spin;
    for (int i = 0; i < 3; ++i)
        d.c0[i] = d.pos[i] = d.centre[i] = m[i * 4] * cm[0] + m[i * 4 + 1] * cm[1] + m[i * 4 + 2] * cm[2] + m[i * 4 + 3];
    // Out of the well along its own length (the model's -y), tumbling around the gun's sideways
    // axis (the model's x); from the hand at the hand's speed as it let go.
    float down[3] = {-m[1], -m[5], -m[9]};
    Normalize(down);
    for (int i = 0; i < 3; ++i)
        d.vel[i] = velocityView ? toArms[i * 3] * velocityView[0] + toArms[i * 3 + 1] * velocityView[1] + toArms[i * 3 + 2] * velocityView[2]
                   : !fromHand  ? down[i] * kEjectSpeed
                                : 0.0f;
    d.axis[0] = m[0], d.axis[1] = m[4], d.axis[2] = m[8];
    Normalize(d.axis);
    d.angle = 0.0f;
    AxisAngle(d.axis, 0.0f, d.q);
    d.falling = true;
    d.updatedAt = Seconds();
    g_dropShown = true;
    return true;
}

// ---- Loose things (loose.h): the magazines and cases let go of, each a rigid body in the world (the user, 2026-11-05: "I want magazine physics. I want the launcher physics.
// When I take out the empty shell, I want all of that, and I want it to hit the floor. I want it to stay there for a minute"). hands.cpp steps them once a frame (loose::Step)
// and gives their poses here (SetLoose); each draw places each one through its own drawnFromWorld, as the kept drop above. weapon.txt "loose 0": the old straight fall (g_looseOn,
// declared before ReadWeaponSelection). ----
enum LooseKind { kLooseRifleMag = 1, kLooseHeldMag = 2, kLooseM320Case = 3, kLooseSmawCase = 4 /* hands.cpp's from rocket.cpp's fall */,
                 kLooseGun = 5 /* hands.cpp's: the rifle let go of, drawn by its own draws (HandTargets::gunLying), never by a copy here */,
                 kLooseShell = 6 /* hands.cpp's: a shotgun's shell let go of or thrown out by the bolt, drawn with the gun's own shell draw (DrawHeldPart) */ };
// The launcher's round as its mesh draws it (its bone's vertices' space: its base's middle at (0.0003, -0.0584, 0.2795), along +z, 11.9 cm long, 5.1 cm across: the mesh
// dump placed at the bone's rest) into the case's own space as loose has it (its base's middle at 0, along +z).
constexpr float kM320RoundToCase[12] = {1, 0, 0, -0.0003f, 0, 1, 0, 0.0584f, 0, 0, 1, -0.2795f};
// ... and the case's spec: the whole round the mesh draws (the game's own reload pulls the whole of it out), not the real 46 mm case, so what's drawn is what lies.
const loose::Spec* LooseM320CaseSpec() {
    static const loose::Spec s_spec = [] {
        loose::Spec s{};
        s.name = "m320 round as drawn";
        s.round = 2;                   // round about z
        s.rings = 2;
        s.ringAt[0] = 0.0f, s.ringAt[1] = 0.119f;
        s.ringRadius[0] = 0.0255f, s.ringRadius[1] = 0.0255f;
        s.head = 0.35f;                // its aluminium base's share of the weight
        s.massEmpty = 0.06f, s.massRound = 0.17f;  // a spent case; a live round (dropped from the hand) its grenade's weight more
        s.capacity = 1;
        s.bounce = 0.8f;
        s.boxMin[0] = -0.0255f, s.boxMin[1] = -0.0255f, s.boxMin[2] = 0.0f;
        s.boxMax[0] = 0.0255f, s.boxMax[1] = 0.0255f, s.boxMax[2] = 0.119f;
        return s;
    }();
    return &s_spec;
}
// A magazine's other bones on its first (the one loose moves), per loose id; the bones it was seen with.
struct LooseMag {
    int id = 0;
    float seat[kMagBones][12] = {};
    bool have[kMagBones] = {};
};
std::mutex g_looseMutex;  // the poses below and g_looseMags
loose::Pose g_loosePoses[loose::kAll];
int g_loosePoseCount = 0;
std::atomic<int> g_looseDraws{0};  // copies drawn since hands.cpp's log last asked (TakeLooseDraws)
float g_looseOrigin[3] = {};  // the arms' space's origin the poses are in
LooseMag g_looseMags[loose::kMost];

// The magazine's spec for loose by the gun it's from (the M416's, or the held gun's shape).
const loose::Spec* MagazineSpec(bool held) {
    const char* shape = held ? skin::HeldShape() : "m416";
    return shape ? loose::SpecFor(shape) : nullptr;
}

// The kept drop's start (its bones in the world, origin0) handed to loose: out of the well with the button (eject), let go of (fromHand), or knocked out (velocityView).
// Let go of by the right hand (side 1): hand is that hand now (hands.cpp's), and the magazine leaves by that hand's own motion (loose::ReleaseBy); side 0 is the left's, as always.
// False: loose isn't on, the hands haven't given it a frame yet, or the right hand isn't tracked now: the old fall then.
bool StartLooseMagazine(bool fromHand, const float* velocityView, float spin, bool held, int side = 0, const loose::HandAt* hand = nullptr) {
    const loose::Frame* frame = hands::LooseFrame();
    const loose::Spec* spec = MagazineSpec(held);
    if (!g_looseOn.load(std::memory_order_relaxed) || !frame || !spec)
        return false;
    // The right hand not tracked, or its numbers not finite: false before the drawn place is taken below, so the old fall still finds it.
    auto finite = [](const float* m) { return std::all_of(m, m + 12, [](float x) { return std::isfinite(x); }); };
    const bool byRight = fromHand && side == 1;
    if (byRight && !(hand && hand->tracked && finite(hand->room)))
        return false;
    float bones[kMagBones][12], origin0[3];
    bool seen[kMagBones];
    {
        std::lock_guard lock(g_magMutex);
        Drop& d = g_drop;
        const double now = Seconds();
        for (size_t i = 0; i < kMagBones; ++i)
            seen[i] = fromHand ? d.heldSeen[i] && now - d.heldAt[i] < 0.1 : d.seen[i];
        if (!seen[0])
            return false;
        if (fromHand)
            std::fill(std::begin(d.heldSeen), std::end(d.heldSeen), false);
        std::memcpy(bones, fromHand ? d.held : d.last, sizeof(bones));
        const float (*origins)[3] = fromHand ? d.heldOrigin : d.lastOrigin;
        std::memcpy(origin0, origins[0], sizeof(origin0));
        for (size_t b = 1; b < kMagBones; ++b)
            if (seen[b])
                for (int i = 0; i < 3; ++i)
                    bones[b][i * 4 + 3] += origins[b][i] - origin0[i];
    }
    // Into the frame's own origin (loose takes and gives everything in the arms' space with Frame::origin).
    float first[12];
    std::memcpy(first, bones[0], sizeof(first));
    for (int i = 0; i < 3; ++i)
        first[i * 4 + 3] += origin0[i] - static_cast<float>(frame->origin[i]);
    LooseMag mag;
    float firstInverse[12];
    if (!Invert(bones[0], firstInverse))
        return false;
    for (size_t b = 0; b < kMagBones; ++b)
        if ((mag.have[b] = seen[b]))
            Compose(firstInverse, bones[b], mag.seat[b]);
    const int kind = held ? kLooseHeldMag : kLooseRifleMag;
    int id = 0;
    if (byRight) {
        // The right hand alike (free hands: the rifle in the left hand, or held by its handguard alone): its grip and its controller's own speed and turn as hands.cpp has
        // them now, and the start carried by the hand's move since the frame had it, as the left's below (not carried: its grip stays the frame's, as the left's does).
        loose::Frame now = *frame;
        const loose::HandAt& was = frame->hands[1];
        const bool wasThere = was.tracked && finite(was.room);
        bool carried = false;
        if (wasThere) {
            float handInverse[12], armsInverse[12], a[12], b[12], c[12], moved[12];
            if (Invert(was.room, handInverse) && Invert(frame->roomToArms, armsInverse)) {
                Compose(hand->room, handInverse, a);  // the hand's move in the room since the magazine was drawn
                Compose(frame->roomToArms, a, b);
                Compose(b, armsInverse, c);  // ... in the arms' space (frame->origin's, as first is)
                Compose(c, first, moved);
                if ((carried = finite(moved)))
                    std::memcpy(first, moved, sizeof(first));
            }
        }
        now.hands[1] = *hand;
        if (wasThere && !carried)
            std::memcpy(now.hands[1].room, was.room, sizeof(now.hands[1].room));
        id = loose::ReleaseBy(*spec, kind, first, 0, now, 1);
    } else if (fromHand) {
        // This frame's hand (its grip as steadied now: a jump confirmed real is already put back), and its controller's own speed and turn: the magazine leaves where the
        // glove is drawn this frame, at the let-go frame's reading. (It had left from last frame's place at last frame's reading, a step behind a hard throw's hand: the
        // review, 2026-11-05.) The start moves with the hand's own move since then, so it and the hand's place stay together.
        loose::Frame now = *frame;
        if (XrPosef g{}; input::HandPose(0, g)) {
            const float x = g.orientation.x, y = g.orientation.y, z = g.orientation.z, w = g.orientation.w;  // (as hands.cpp's QuatToMatrix: rows)
            const float gripNow[12] = {1 - 2 * (y * y + z * z), 2 * (x * y - z * w),     2 * (x * z + y * w),     g.position.x,
                                       2 * (x * y + z * w),     1 - 2 * (x * x + z * z), 2 * (y * z - x * w),     g.position.y,
                                       2 * (x * z - y * w),     2 * (y * z + x * w),     1 - 2 * (x * x + y * y), g.position.z};
            bool carried = false;
            if (frame->hand) {
                float handInverse[12], armsInverse[12], a[12], b[12], c[12], moved[12];
                if (Invert(frame->handRoom, handInverse) && Invert(frame->roomToArms, armsInverse)) {
                    Compose(gripNow, handInverse, a);  // the hand's move in the room since the magazine was drawn
                    Compose(frame->roomToArms, a, b);
                    Compose(b, armsInverse, c);  // ... in the arms' space (frame->origin's, as first is)
                    Compose(c, first, moved);
                    std::memcpy(first, moved, sizeof(first));
                    carried = true;
                }
            }
            if (carried || !frame->hand) {
                now.hand = true;
                std::memcpy(now.handRoom, gripNow, sizeof(now.handRoom));
            }
            now.handSensed = now.hand && input::HandVelocity(0, now.handVelocity, now.handSpin);
        }
        id = loose::Release(*spec, kind, first, 0, now);
    } else if (velocityView) {
        float toArms[9];
        {
            std::lock_guard lock(g_fpMutex);
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    toArms[i * 3 + j] = g_lastSeenFrom[i * 4 + j];
        }
        float velocity[3], axis[3] = {first[0], first[4], first[8]};  // tumbling about the gun's sideways, as it was knocked
        for (int i = 0; i < 3; ++i)
            velocity[i] = toArms[i * 3] * velocityView[0] + toArms[i * 3 + 1] * velocityView[1] + toArms[i * 3 + 2] * velocityView[2];
        Normalize(axis);
        id = loose::Flick(*spec, kind, first, 0, velocity, axis, spin, *frame);
    } else if (frame->gun) {
        // Out with the button: seated on the gun's frame bone where it's drawn, out along its own length (the model's -y).
        float gunArms[12], gunInverse[12], onGun[12];
        Compose(frame->roomToArms, frame->gunRoom, gunArms);
        if (!Invert(gunArms, gunInverse))
            return false;
        Compose(gunInverse, first, onGun);
        float axisOut[3] = {-onGun[1], -onGun[5], -onGun[9]};
        Normalize(axisOut);
        // How far it slides from its seat to clear of the well's mouth, as measured on each gun (the M416's: pouch.cpp); any other 10 cm.
        const char* shape = held ? skin::HeldShape() : "m416";
        const float well = !shape                              ? 0.10f
                           : std::strcmp(shape, "m416") == 0   ? 0.087f
                           : std::strcmp(shape, "m16a4") == 0  ? 0.075f
                           : std::strcmp(shape, "m4a1") == 0   ? 0.077f
                           : std::strcmp(shape, "mk11") == 0   ? 0.032f
                           : std::strcmp(shape, "kh2002") == 0 ? 0.05f
                                                               : 0.10f;
        id = loose::Eject(*spec, kind, onGun, axisOut, well, 0.0f, 0, *frame);
    }
    if (!id)
        return false;
    mag.id = id;
    std::lock_guard lock(g_looseMutex);
    static size_t s_next = 0;
    LooseMag* slot = nullptr;
    for (LooseMag& m : g_looseMags)  // one whose magazine is gone
        if (!m.id || !loose::Has(m.id)) {
            slot = &m;
            break;
        }
    if (!slot)  // (loose keeps no more than this many either: it's pushed the oldest out)
        slot = &g_looseMags[s_next++ % std::size(g_looseMags)];
    *slot = mag;
    return true;
}

// A loose magazine's bones for this draw: its pose (as loose gave it) times each bone's seat, out through the draw's placing; into palette at each of this pass's bones.
// held: a held gun's (its seat bones), else the M416's (kMagazineBones).
bool LooseMagazinePalette(const loose::Pose& pose, const LooseMag& mag, size_t pass, const WorldPlacing& placing, float* palette) {
    bool any = false;
    for (size_t i = 0; i < kMagBones; ++i) {
        if (!mag.have[i] || kMagazineBones[i].pass != pass)
            continue;
        float world[12];
        Compose(pose.bone, mag.seat[i], world);
        WorldToDrawn(placing, world, g_looseOrigin, palette + kMagazineBones[i].bone * 12);
        any = true;
    }
    return any;
}

// A point moved by a 3x4 matrix.
void TransformPoint(const float* m, const float* p, float* out) {
    for (int i = 0; i < 3; ++i)
        out[i] = m[i * 4] * p[0] + m[i * 4 + 1] * p[1] + m[i * 4 + 2] * p[2] + m[i * 4 + 3];
}

// Learns an arms (or gun part) draw's offsets from the game's gun body drawn in the same view
// (gunInverse: its inverse), from the bones the game drew it with: the gloves first judge the game's
// hold (HoldIsNormal); a hand's bones are learned from the normal hold only, a gun part's always.
// Under g_handsMutex.
void LearnArms(const ArmDraw& draw, const float* bones, const float* gunInverse, bool later) {
    // Back after a gap (the rifle put away, a pause): its first frames may be mid-raise, nothing's learned from them.
    if (const ULONGLONG now = GetTickCount64(); now - g_armsSeenAt > 250)
        g_learnAgainAt = now + 300;
    g_armsSeenAt = GetTickCount64();
    if (draw.rightRoot >= 0) {
        // Where the game's own right hand holds the gun: its palm, as the game drew it, in the
        // gun's model space; the gloves' draw has the left palm too (its bone 1).
        float palm[3], grip[3], at[3], leftGrip[3];
        TransformPoint(bones + draw.rightRoot * 12, kRightPalm, palm);
        TransformPoint(gunInverse, palm, grip);
        const bool gloves = &draw == &kArmDraws[0];
        if (gloves) {
            TransformPoint(bones + 1 * 12, kLeftPalm, at);
            TransformPoint(gunInverse, at, leftGrip);
            // Only while it rests there: the frames it's arriving or leaving have its fingers on the move (a hold learned from them hovers).
            static float s_lastLeft[3] = {};
            const bool still = Length3(leftGrip[0] - s_lastLeft[0], leftGrip[1] - s_lastLeft[1], leftGrip[2] - s_lastLeft[2]) < 0.0015f;
            std::memcpy(s_lastLeft, leftGrip, sizeof(s_lastLeft));
            g_leftOnM320 = ammo::M320Up() && still && Length3(leftGrip[0] - kM320Palm[0], leftGrip[1] - kM320Palm[1], leftGrip[2] - kM320Palm[2]) < 0.025f;
        }
        const bool held = Length3(grip[0], grip[1], grip[2]) < 1.0f;  // within the gun's reach: a real hold
        const bool normal = held && HoldIsNormal(grip, gloves ? leftGrip : nullptr);
        g_holdNormal = normal;
        g_frameJudged = true;
        g_frameHoldNormal = g_frameHoldNormal && normal;
        ++(later ? g_judgedLater : g_judgedHere);
        // (The hold followed only while it's about there, as FollowLeftHold.)
        if (normal && (!g_haveGrip || !g_holdSettled || Length3(grip[0] - g_gripModel[0], grip[1] - g_gripModel[1], grip[2] - g_gripModel[2]) < 0.01f)) {
            for (int i = 0; i < 3; ++i)
                g_gripModel[i] = g_haveGrip ? g_gripModel[i] * 0.9f + grip[i] * 0.1f : grip[i];
            g_haveGrip = true;
        }
    }
    // A hand's bones are only noted here: taken at the frame's end if the whole frame's verdict is good (TakeFrameOfHands). By the verdict of the frame
    // before, the one frame in which the game's hands leave the gun as it's put away (half a metre at once) taught them: the rifle came back from
    // the launcher with the right arm floating ahead of it until the 5 s rule took a new hold (the user, 2026-11-04: "my arms take a while").
    Learned& next = g_learnedNext[draw.key];
    Learned& learned = g_learned[draw.key];
    for (size_t k = 0; draw.roles[k] && k < kPaletteBones; ++k) {
        const float* bone = bones + k * 12;
        switch (draw.roles[k]) {
        case 'L':
            // The game's own left hand as its animation holds the gun, learned against the game's gun
            // (hands.cpp draws it on the free gun while the hand holds the handguard).
            if (&draw == &kArmDraws[0] && k < handpose::kBones) {
                // On the launcher's grip (the launcher in hand, the palm there): its own hold, not the handguard's.
                Compose(gunInverse, bone, g_leftOnM320 ? g_gameLeftM320Next[k] : g_gameLeftNext[k]);
                if (k + 1 == handpose::kBones)
                    (g_leftOnM320 ? g_gameLeftM320NextWhole : g_gameLeftNextWhole) = true;
            }
            break;
        case 'R':
            Learn(next, k, gunInverse, bone);
            break;
        case 'G':
            Learn(learned, k, gunInverse, bone);
            for (int s = 0; s < g_partShifts.load(std::memory_order_relaxed); ++s)
                if (g_partShift[s].count == draw.key.indexCount && g_partShift[s].bone == static_cast<int>(k))
                    learned.rel[k][3] += g_partShift[s].by[0], learned.rel[k][7] += g_partShift[s].by[1], learned.rel[k][11] += g_partShift[s].by[2];
            if (g_gunPartLog.load(std::memory_order_relaxed)) {
                static std::unordered_map<uint32_t, ULONGLONG> s_saidAt;
                ULONGLONG& said = s_saidAt[draw.key.indexCount * 64u + static_cast<uint32_t>(k)];
                if (GetTickCount64() - said >= 500) {
                    said = GetTickCount64();
                    const float* m = learned.rel[k];
                    Log("Gun parts: %u from %u bone %d on the body: at (%.3f %.3f %.3f), its z (%.3f %.3f %.3f), its y (%.3f %.3f %.3f), its x (%.3f %.3f %.3f)", draw.key.indexCount, draw.key.startIndex, static_cast<int>(k), m[3], m[7],
                        m[11], m[2], m[6], m[10], m[1], m[5], m[9], m[0], m[4], m[8]);
                }
            }
            PinPart(draw.key, k, learned.rel[k]);
            break;
        }
    }
}

// The frame's end: the hands' bones noted in it count now if the game held its gun normally all through it (no judgement in it at all, as the last
// one was: a launch whose draws never meet the gun's body must go on learning). Under g_handsMutex.
void TakeFrameOfHands() {
    const bool good = g_frameJudged ? g_frameHoldNormal : g_holdNormal.load(std::memory_order_relaxed);
    const bool usable = good && !input::Sprinting();
    const bool trust = usable && GetTickCount64() >= g_learnAgainAt;  // (a bone with nothing yet, the first raise of a launch, needn't wait)
    for (auto& [key, next] : g_learnedNext) {
        Learned* learned = nullptr;
        for (size_t k = 0; k < kPaletteBones; ++k) {
            if (!next.have[k])
                continue;
            next.have[k] = false;
            if (!usable)
                continue;
            if (!learned)
                learned = &g_learned[key];
            if (!trust && learned->have[k])
                continue;
            std::memcpy(learned->rel[k], next.rel[k], sizeof(learned->rel[k]));
            learned->have[k] = true;
        }
    }
    if (g_gameLeftNextWhole && (trust || (usable && !g_haveGameLeft))) {
        std::memcpy(g_gameLeft, g_gameLeftNext, sizeof(g_gameLeft));
        g_haveGameLeft = true;
    }
    g_gameLeftNextWhole = false;
    // The game's left hand on the launcher's grip, its own pose there (taken every frame it's there: the hand's second hold, hands.cpp).
    if (g_gameLeftM320NextWhole) {
        if (!g_haveGameLeftM320)
            Log("Hands: the game's left hand on the grenade launcher's grip learned (the second hold)");
        std::memcpy(g_gameLeftM320, g_gameLeftM320Next, sizeof(g_gameLeftM320));
        g_haveGameLeftM320 = true;
        if (!g_m320GripDirty)
            g_m320GripDirtySince = GetTickCount64();
        g_m320GripDirty = true;
        g_m320GripLearnedAt = GetTickCount64();
    }
    g_gameLeftM320NextWhole = false;
    // Kept once the hand's been away from it a second and a half (its last frames there the steadiest), or after 2 s there.
    if (g_m320GripDirty && (GetTickCount64() - g_m320GripLearnedAt > 1500 || GetTickCount64() - g_m320GripDirtySince > 2000)) {
        g_m320GripDirty = false;
        SaveM320Grip();
    }
    g_frameJudged = false;
    g_frameHoldNormal = true;
}

// Arms (and gun parts) drawn before the gun's body in their view: which draw comes first differs
// from launch to launch, and in some the gloves never had the gun's body to learn from, the right
// glove left at the game's hip (2026-10-31, until a reload changed the order). Their bones are kept
// until the gun's body is drawn in the same view and learned from then (DrawGun); they're placed by
// what was learned before. Under g_handsMutex.
struct PendingArms {
    const ArmDraw* draw = nullptr;
    uint32_t view = 0;
    bool waiting = false;
    float bones[kPaletteBones][12] = {};
};
std::vector<PendingArms> g_pendingArms;

void KeepForLater(const ArmDraw& draw, const float* bones, uint32_t view) {
    auto it = std::find_if(g_pendingArms.begin(), g_pendingArms.end(), [&](const PendingArms& p) { return p.draw == &draw; });
    if (it == g_pendingArms.end())
        it = g_pendingArms.insert(g_pendingArms.end(), PendingArms{&draw});
    else if (it->waiting && draw.rightRoot >= 0)
        ++g_notJudged;  // the gloves kept before never met the gun's body
    size_t used = 0;
    while (used < kPaletteBones && draw.roles[used])
        ++used;
    std::memcpy(it->bones, bones, used * sizeof(it->bones[0]));
    it->view = view;
    it->waiting = true;
}

// The rounds at the magazine's top (the gun's second draw, no body bone of its own): learned like
// a gun part.
constexpr ArmDraw kRoundsDraw{kWeaponPasses[1], "G", -1};

// The gun's body was just drawn (g_gunBodyOrig, view): the draws kept from this view learn now.
void LearnKept(uint32_t view) {
    float gunInverse[12];
    bool haveInverse = false;
    for (PendingArms& p : g_pendingArms) {
        if (!p.waiting)
            continue;
        p.waiting = false;
        if (p.view != view) {  // from a view gone by
            if (p.draw->rightRoot >= 0)
                ++g_notJudged;
            continue;
        }
        if (!haveInverse && !(haveInverse = Invert(g_gunBodyOrig, gunInverse)))
            return;
        LearnArms(*p.draw, &p.bones[0][0], gunInverse, true);
    }
}

// New places added to those known (each is looked at before every write, and let go of if it's not the gun's any more). Returns how many are known.
template <size_t N>
size_t AddPlaces(std::atomic<uintptr_t> (&known)[N], const std::vector<uintptr_t>& found) {
    for (const uintptr_t place : found) {
        bool have = false;
        for (const auto& k : known)
            have = have || k.load(std::memory_order_relaxed) == place;
        if (have)
            continue;
        for (auto& k : known)
            if (!k.load(std::memory_order_relaxed)) {
                k.store(place, std::memory_order_relaxed);
                break;
            }
    }
    size_t count = 0;
    for (const auto& k : known)
        count += k.load(std::memory_order_relaxed) != 0;
    return count;
}

// Where a shot should start, as the game's data has it: from the game's eye, along the game's aim (x left, y up, z forward), for a point on a gun where the world's picture shows it
// (seen: gunSeen, launcherSeen).
bool ShotStart(const skin::HandTargets& t, const float* seen, const float* point, float* start) {
    float yaw = 0.0f, pitch = 0.0f;
    if (!t.seenHaveOrigin || !camera::GameWeaponAngles(yaw, pitch))
        return false;
    float d[3];
    TransformPoint(seen, point, d);
    for (int i = 0; i < 3; ++i)
        d[i] += t.seenOrigin[i] - t.seenEye[i];
    const float cp = std::cos(pitch);
    const float f[3] = {-std::sin(yaw) * cp, std::sin(pitch), std::cos(yaw) * cp};
    float left[3] = {f[2], 0.0f, -f[0]};
    const float length = Length3(left[0], left[1], left[2]);
    if (length <= 1e-4f)
        return false;
    for (float& c : left)
        c /= length;
    const float up[3] = {f[1] * left[2] - f[2] * left[1], f[2] * left[0] - f[0] * left[2], f[0] * left[1] - f[1] * left[0]};
    start[0] = d[0] * left[0] + d[1] * left[1] + d[2] * left[2];
    start[1] = d[0] * up[0] + d[1] * up[1] + d[2] * up[2];
    start[2] = d[0] * f[0] + d[1] * f[1] + d[2] * f[2];
    return true;
}

// The M416's sights' line on its body, zoomed (weapon.txt "riflelog"): the game's eye at (0, 0.112, 0.148), looking 0.29 degrees below z. At its barrel's end, z 0.78.
constexpr float kRifleSightsAtMuzzle[3] = {0.0f, 0.109f, 0.78f};
constexpr float kRifleZoomedEyeZ = 0.148f;
// The nearest of the M416 itself on that line toward the eye with no ACOG seen on it (skin::Sight's rearZ): a flat top, so the rear of its rail, where
// whatever sits on the line stands (its collision shape, build\guns\m416.bin: nothing of the gun within 3 cm of the line before it).
constexpr float kRifleRearZ = 0.294f;

// One of the gun's passes: moved into the right hand (free hands), without its magazine while it's
// out, and drawn again with only the dropped magazine's bones while that falls. Its magazine bones
// are noted, as drawn, for the moment it comes out. False: draw as usual.
// The M416's scatter without the zoom taken down to what it is zoomed. Its GunSwayData has 2.5 degrees at the least standing still
// (about 22 cm off at 5 m), 3 moving, 0.2 and 1 zoomed; free hands never zoom, and the sights looked along a line the bullets
// only scattered round (the user: "airtight and solid"). Only this gun's numbers, with its recoil's as well: a loose match over the
// launcher's data once wrote over the game's and crashed it. weapon.txt "riflespread 0" puts them back. Held by its pistol grip alone (recoil's hold,
// design 6: skin::HoldSpread) a gun's rows are the game's own again, so a rifle held like a pistol scatters as the game's hip fire does.
struct SpreadRow {
    uintptr_t place;
    float game[4], wanted[4];
    size_t kind = ~size_t(0);  // whose: ClearRifleSpread's place in its table, or kAt4Gun (KindDataThere checks a kind's own rows)
};
std::mutex g_rifleSpreadMutex;
std::vector<SpreadRow> g_rifleSpreadRows;

// The scans of the game's memory for a gun's data (ClearRifleSpread, FindAt4, FindStinger, ClearLauncherSpread: all of its read-write pages, 0.6-1.1 s of
// a core each; the audit of 2026-11-19: 17 in the owner's 24 minutes, one at every swap, 2-7 frames lost in a tight second). Each runs at the lowest
// priority Windows gives a thread for its processor, memory and disk (THREAD_MODE_BACKGROUND_BEGIN), and one at a time: StartScan counts them, and a
// draw that wants one while another runs asks again the next frame.
std::atomic<int> g_scansRunning{0};
struct ScanThread {
    ScanThread() { SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN); }
    ~ScanThread() {
        SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_END);
        g_scansRunning.fetch_sub(1, std::memory_order_acq_rel);
    }
};
bool ScanRunning() {
    return g_scansRunning.load(std::memory_order_acquire) > 0;
}
// Starts one (its function opens with a ScanThread); false if the thread couldn't be made.
bool StartScan(LPTHREAD_START_ROUTINE scan, LPVOID arg) {
    g_scansRunning.fetch_add(1, std::memory_order_acq_rel);
    if (HANDLE thread = CreateThread(nullptr, 0, scan, arg, 0, nullptr)) {
        CloseHandle(thread);
        return true;
    }
    g_scansRunning.fetch_sub(1, std::memory_order_acq_rel);
    return false;
}
std::atomic<bool> g_spreadOneHand{false};  // the rows as the game has them for one hand on the gun (HoldSpread): changed under g_rifleSpreadMutex

// Each known row toward the scatter as it is zoomed (zoomed) or as the game has it, only where the other is still there: a load leaves other things at
// these addresses. How many were written. With forgotten (recoil's hold, at each change of it), a row that reads as neither, or can't be read, is
// dropped from the list on the way and counted there: the list otherwise only grows, level after level. Under g_rifleSpreadMutex.
int WriteRifleSpread(bool zoomed, size_t* forgotten = nullptr) {
    const HANDLE self = GetCurrentProcess();
    int written = 0;
    size_t kept = 0;
    for (size_t i = 0; i < g_rifleSpreadRows.size(); ++i) {
        const SpreadRow row = g_rifleSpreadRows[i];
        float now[4];
        SIZE_T got = 0;
        const float* from = zoomed ? row.game : row.wanted;
        const float* to = zoomed ? row.wanted : row.game;
        const bool read = ReadProcessMemory(self, reinterpret_cast<LPCVOID>(row.place), now, sizeof(now), &got) && got == sizeof(now);
        const bool there = read && std::memcmp(now, from, sizeof(now)) == 0;
        if (there && WriteProcessMemory(self, reinterpret_cast<void*>(row.place), to, sizeof(now), nullptr))
            ++written;
        if (forgotten && !there && !(read && std::memcmp(now, to, sizeof(now)) == 0))
            ++*forgotten;
        else
            g_rifleSpreadRows[kept++] = row;
    }
    g_rifleSpreadRows.resize(kept);
    return written;
}

void ApplyRifleSpread(bool on) {
    std::lock_guard lock(g_rifleSpreadMutex);
    const bool oneHand = g_spreadOneHand.load(std::memory_order_relaxed);
    const int written = WriteRifleSpread(on && !oneHand);
    Log("Rifle: its scatter without the zoom %s (%d of %zu rows)", !on ? "as the game has it" : oneHand ? "as the game has it, held by one hand" : "as it is zoomed",
        written, g_rifleSpreadRows.size());
}

// recoil's hold for the scatter (HoldSpread): when it was last asked; and since when the hold asked for has been the other one than the rows have, without
// a break (0: it hasn't; the clock is never 0 by then). The presenting thread's (hands.cpp, skin::Update).
std::atomic<ULONGLONG> g_spreadAskedAt{0}, g_spreadOtherSince{0};
// Milliseconds a change of hold is asked for without a break before it's written (HoldSpread). recoil.h's SpreadFor already waits 0.2 s for a change of
// hold to last: this only keeps a frame or three recoil clears in (hipSpread false for it) from writing; 200 here too made it 0.4 s in all.
constexpr ULONGLONG kSpreadSettle = 50;

// The hold asked for (oneHand) on to the rows, quietly, once it has been asked for without a break for `settle` milliseconds (HoldSpread's kSpreadSettle,
// after recoil.h's own 0.2 s: a frame or three recoil clears in never writes them; the lapse's none, its half second
// is its own), and never while the scan or weapon.txt is writing them (tried again the next frame). Rows whose data is gone are forgotten on the way. A
// line at most every 5 s.
void StepHoldSpread(bool oneHand, ULONGLONG now, ULONGLONG settle) {
    constexpr ULONGLONG kSayEvery = 5000;
    if (oneHand == g_spreadOneHand.load(std::memory_order_relaxed)) {
        g_spreadOtherSince = 0;
        return;
    }
    ULONGLONG since = g_spreadOtherSince.load();
    if (!since)
        g_spreadOtherSince = since = now;
    if (now - since < settle)
        return;
    std::unique_lock lock(g_rifleSpreadMutex, std::try_to_lock);
    if (!lock.owns_lock())
        return;
    g_spreadOneHand = oneHand;
    g_spreadOtherSince = 0;
    const bool zoomedOn = g_rifleSpread.load(std::memory_order_relaxed);  // (weapon.txt "riflespread 0": they're the game's already, and stay so)
    size_t forgotten = 0;
    const int written = zoomedOn ? WriteRifleSpread(!oneHand, &forgotten) : 0;
    if (forgotten)
        Log("Rifle: %zu rows of its scatter forgotten, their data gone (a load): %zu still known", forgotten, g_rifleSpreadRows.size());
    static ULONGLONG s_saidAt = 0;
    static int s_unsaid = 0;
    if (now - s_saidAt >= kSayEvery) {
        s_saidAt = now;
        Log("Rifle: held by %s: its scatter without the zoom %s (%d of %zu rows; %d changes of hold since the last line)", oneHand ? "one hand" : "both hands",
            !zoomedOn || oneHand ? "as the game has it" : "as it is zoomed", written, g_rifleSpreadRows.size(), s_unsaid);
        s_unsaid = 0;
    } else {
        ++s_unsaid;
    }
}

// Exactly a speed of one of the guns in ClearRifleSpread's kGuns: their ShotConfigDatas.
bool IsBulletSpeed(float v) {
    // (440 the AKS-74u's, 320 the UMP-45's; 300 the M1014's buckshot, 450 its flechettes; 550 the Mk11's, no other gun's; 620 the M249's, no other gun's)
    return v == 600.0f || v == 580.0f || v == 650.0f || v == 500.0f || v == 610.0f || v == 440.0f || v == 320.0f || v == 300.0f || v == 450.0f || v == 550.0f ||
           v == 620.0f;
}

// Which gun's (its place in kGuns): the M416's as it's drawn; another's only once it's in the player's hand, so an enemy's isn't changed before then.
DWORD WINAPI ClearRifleSpread(LPVOID which) {
    const ScanThread scanning;  // (StartScan's: the lowest priority, one at a time)
    const size_t wanted = reinterpret_cast<size_t>(which);
    const ULONGLONG began = GetTickCount64();
    const HANDLE self = GetCurrentProcess();
    // Each gun's own numbers (Venice-EBX, its GunSwayData): the unzoomed stand, its recoil's that follow them, the zoomed, the crouched, the prone.
    struct Sway {
        float stand[30], zoom[8], crouch[8], prone[8];
        float speed;  // its bullets', m/s: how their ShotConfigData is known
        float speed2 = 0.0f;  // ... its other ammunition's (a shotgun's flechettes), 0 none
    };
    static constexpr Sway kGuns[] = {
        {{2.5f, 7, 0.1f, 15, 3, 7, 0.1f, 15, 7, 7, 0.1f, 15, 4, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15, 10, 0.26f, -0.3f, 0.1f, 10, 18},
         {0.2f, 3, 0.1f, 15, 1, 3, 0.1f, 15},
         {2, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15},
         {1.5f, 7, 0.1f, 15, 2, 7, 0.1f, 15}, 600},  // the M416
        {{2.5f, 7, 0.1f, 15, 3, 7, 0.1f, 15, 7, 7, 0.1f, 15, 4, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15, 10, 0.3f, -0.3f, 0.2f, 10, 18},
         {0.2f, 3, 0.1f, 15, 1, 3, 0.1f, 15},
         {2, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15},
         {1.5f, 7, 0.1f, 15, 2, 7, 0.1f, 15}, 600},  // the AN-94
        {{3.5f, 7, 0.1f, 15, 4.5f, 7, 0.1f, 15, 7, 7, 0.1f, 15, 7, 7, 0.1f, 15, 3.5f, 7, 0.1f, 15, 3.5f, 7, 0.1f, 15, 10, 0.3f, -0.2f, 0.2f, 10, 13},
         {0.4f, 3, 0.1f, 15, 1.5f, 3, 0.1f, 15},
         {3, 7, 0.1f, 15, 4, 7, 0.1f, 15},
         {2.5f, 7, 0.1f, 15, 3.5f, 7, 0.1f, 15}, 600},  // the RPK-74M
        {{2.5f, 7, 0.1f, 15, 3, 7, 0.1f, 15, 7, 7, 0.1f, 15, 4, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15, 10, 0.2f, -0.3f, 0.5f, 10, 18},
         {0.3f, 3, 0.1f, 15, 1, 3, 0.1f, 15},
         {2, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15},
         {1.5f, 7, 0.1f, 15, 2, 7, 0.1f, 15}, 580},  // the AEK-971
        {{2.5f, 7, 0.1f, 15, 3, 7, 0.1f, 15, 7, 7, 0.1f, 15, 4, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15, 10, 0.28f, -0.3f, 0.2f, 10, 18},
         {0.2f, 3, 0.1f, 15, 1, 3, 0.1f, 15},
         {2, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15},
         {1.5f, 7, 0.1f, 15, 2, 7, 0.1f, 15}, 600},  // the AK-74M
        {{3.5f, 7, 0.12f, 15, 4, 7, 0.12f, 15, 7, 7, 0.12f, 15, 5, 7, 0.12f, 15, 2.5f, 7, 0.12f, 15, 2.5f, 7, 0.12f, 15, 10, 0.45f, -0.2f, 0.2f, 10, 15},
         {0.1f, 3, 0.12f, 15, 1.5f, 3, 0.12f, 15},
         {3, 7, 0.12f, 15, 3.5f, 7, 0.12f, 15},
         {2.5f, 7, 0.12f, 15, 3, 7, 0.12f, 15}, 500},  // the G3A3
        {{2, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15, 7, 7, 0.1f, 15, 5, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15, 10, 0.2f, -0.4f, 0.4f, 10, 18},
         {0.2f, 3, 0.1f, 15, 1, 3, 0.1f, 15},
         {1.5f, 7, 0.1f, 15, 2, 7, 0.1f, 15},
         {1, 7, 0.1f, 15, 1.5f, 7, 0.1f, 15}, 650},  // the KH2002
        {{5, 7, 0.12f, 15, 6, 7, 0.12f, 15, 7, 7, 0.12f, 15, 7, 7, 0.12f, 15, 5, 7, 0.12f, 15, 5, 7, 0.12f, 15, 10, 0.7f, -0.5f, 0.5f, 10, 12},
         {0.5f, 3, 0.12f, 15, 1.5f, 3, 0.12f, 15},
         {4, 7, 0.12f, 15, 5, 7, 0.12f, 15},
         {3, 7, 0.12f, 15, 4, 7, 0.12f, 15}, 610},  // the M240
        {{2.5f, 7, 0.1f, 15, 3, 7, 0.1f, 15, 7, 7, 0.1f, 15, 5, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15, 10, 0.26f, -0.4f, 0.1f, 10, 18},
         {0.2f, 3, 0.1f, 15, 1, 3, 0.1f, 15},
         {2, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15},
         {1.5f, 7, 0.1f, 15, 2, 7, 0.1f, 15}, 650},  // the M16A4
        {{2, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15, 7, 7, 0.1f, 15, 3, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15, 10, 0.26f, -0.4f, 0.1f, 10, 18},
         {0.4f, 3, 0.1f, 15, 0.8f, 3, 0.1f, 15},
         {1.5f, 7, 0.1f, 15, 2, 7, 0.1f, 15},
         {1, 7, 0.1f, 15, 1.5f, 7, 0.1f, 15}, 580},  // the M4A1
        {{2, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15, 7, 7, 0.1f, 15, 3, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15, 10, 0.28f, -0.3f, 0.2f, 10, 18},
         {0.4f, 3, 0.1f, 15, 0.8f, 3, 0.1f, 15},
         {1.5f, 7, 0.1f, 15, 2, 7, 0.1f, 15},
         {1, 7, 0.1f, 15, 1.5f, 7, 0.1f, 15}, 440},  // the AKS-74u
        {{1, 7, 0.1f, 15, 1.5f, 7, 0.1f, 15, 7, 7, 0.1f, 15, 3, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15, 2.5f, 7, 0.1f, 15, 10, 0.25f, -0.2f, 0.2f, 10, 20},
         {0.5f, 3, 0.1f, 15, 0.5f, 3, 0.1f, 15},
         {1, 7, 0.1f, 15, 1.25f, 7, 0.1f, 15},
         {1, 7, 0.1f, 15, 1.25f, 7, 0.1f, 15}, 320},  // the UMP-45 (its crouched and prone rows alike: found one after the other)
        // (Venice-EBX, Weapons/M1014/M1014_Gunsway: its crouched and prone rows are the same numbers, found one after the other; its pellets' own cone is
        // not in these rows and stays the game's.)
        {{1.5f, 7, 0.6f, 15, 2, 7, 0.6f, 15, 7, 7, 0.6f, 15, 4, 7, 0.6f, 15, 4, 7, 0.6f, 15, 4, 7, 0.6f, 15, 10, 1.2f, -0.3f, -0.1f, 10, 15},
         {0.4f, 2, 0.6f, 15, 0.8f, 7, 0.6f, 15},
         {1, 7, 0.6f, 15, 1.5f, 7, 0.6f, 15},
         {1, 7, 0.6f, 15, 1.5f, 7, 0.6f, 15}, 300, 450},  // the M1014 (its buckshot 300 m/s, its flechettes 450)
        // (Venice-EBX, Weapons/MK11/Mk11's GunSwayData; its standing, crouched and prone rows as tools/recoil/stances.tsv has them from the install.)
        {{3.5f, 7, 0.8f, 15, 4.5f, 7, 0.8f, 15, 7, 7, 0.8f, 15, 5, 7, 0.8f, 15, 3.5f, 7, 0.8f, 15, 3.5f, 7, 0.8f, 15, 10, 1.5f, -0.3f, -0.1f, 10, 6},
         {0.0167f, 2, 0.8f, 15, 1.5f, 7, 0.8f, 15},
         {3, 7, 0.8f, 15, 4, 7, 0.8f, 15},
         {2.5f, 7, 0.8f, 15, 3.5f, 7, 0.8f, 15}, 550},  // the Mk11 Mod 0
        // (Venice-EBX, Weapons/M249/M249's own GunSwayData 93453CE1, inline in its blueprint: standing base, moving, jumping, sprinting, the two vaults and
        // its recoil; its zoomed, crouched and prone base and moving rows as tools/recoil/stances.tsv has them from the install.)
        {{5, 7, 0.1f, 15, 6, 7, 0.1f, 15, 7, 7, 0.1f, 15, 7, 7, 0.1f, 15, 5, 7, 0.1f, 15, 5, 7, 0.1f, 15, 10, 0.4f, -0.4f, 0.4f, 10, 12},
         {0.5f, 3, 0.1f, 15, 1.5f, 3, 0.1f, 15},
         {4, 7, 0.1f, 15, 5, 7, 0.1f, 15},
         {3, 7, 0.1f, 15, 4, 7, 0.1f, 15}, 620},  // the M249 (its 5.56 at 620 m/s)
    };
    constexpr size_t kStandFloats = 30;
    constexpr size_t kSwayFloats = 512;  // the rest of its GunSwayData comes within this
    auto same = [](const float* at, const float* wanted, size_t n) {
        for (size_t k = 0; k < n; ++k)
            if (!(std::fabs(at[k] - wanted[k]) < 1e-4f))
                return false;
        return true;
    };
    // The standing block as this gun's own: its 30 floats, each of its kick's three a shot (25-27: up, least and most sideways) its own or 0, taken out
    // of its data by recoil (design 6), which leaves its caps and its decrease (24, 28, 29) as they are. One by one: a scan that reads the block between
    // those three writes still knows it.
    auto standSame = [&](const float* at, const float* want) {
        if (!same(at, want, 25) || !same(at + 28, want + 28, 2))
            return false;
        for (size_t k = 25; k < 28; ++k)
            if (at[k] != 0.0f && !same(at + k, want + k, 1))
                return false;
        return true;
    };
    std::vector<float> chunk(256 * 1024);
    const SIZE_T chunkBytes = chunk.size() * sizeof(float);
    const auto own = reinterpret_cast<uintptr_t>(chunk.data());
    std::vector<SpreadRow> rows;
    std::vector<uintptr_t> shots;
    int found = 0;
    MEMORY_BASIC_INFORMATION info;
    for (uintptr_t at = 0x10000; VirtualQuery(reinterpret_cast<LPCVOID>(at), &info, sizeof(info)) == sizeof(info);) {
        const uintptr_t end = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
        if (info.State == MEM_COMMIT && info.Protect == PAGE_READWRITE) {
            for (uintptr_t from = reinterpret_cast<uintptr_t>(info.BaseAddress); from + 128 <= end;) {
                const SIZE_T want = std::min<SIZE_T>(chunkBytes, end - from);
                SIZE_T got = 0;
                if (!ReadProcessMemory(self, reinterpret_cast<LPCVOID>(from), chunk.data(), want, &got) || got < 128)
                    break;
                const size_t floats = got / sizeof(float);
                for (size_t i = 0; i + kStandFloats <= floats; ++i) {
                    // Its ShotConfigData: InitialPosition none, InitialDirection straight, InitialSpeed 600, each padded to 16 bytes, its InheritWeaponSpeedAmount none. A checkpoint's data is
                    // the same data (a start rewritten isn't none, and isn't taken again).
                    if ((chunk[i + 10] == kGuns[wanted].speed || (kGuns[wanted].speed2 != 0.0f && chunk[i + 10] == kGuns[wanted].speed2)) && chunk[i + 6] == 1.0f && chunk[i] == 0.0f && chunk[i + 1] == 0.0f && chunk[i + 2] == 0.0f && chunk[i + 4] == 0.0f && chunk[i + 5] == 0.0f &&
                        chunk[i + 8] == 0.0f && chunk[i + 9] == 0.0f && chunk[i + 12] == 0.0f) {
                        const uintptr_t place = from + i * sizeof(float);
                        if ((place < own || place >= own + chunkBytes) && shots.size() < 32)
                            shots.push_back(place);
                    }
                    if (const Sway& gun = kGuns[wanted]; chunk[i] == gun.stand[0] && standSame(&chunk[i], gun.stand)) {
                        const uintptr_t place = from + i * sizeof(float);
                        if (place >= own && place < own + chunkBytes)
                            continue;
                        // The zoomed numbers and the other stances' further on in the same data: all there, or it isn't that gun's.
                        size_t zoom = 0, crouch = 0, prone = 0;
                        for (size_t k = kStandFloats; k + 8 <= kSwayFloats && i + k + 8 <= floats; ++k) {
                            if (!zoom && same(&chunk[i + k], gun.zoom, 8))
                                zoom = k;
                            else if (!crouch && same(&chunk[i + k], gun.crouch, 8))
                                crouch = k;
                            else if (!prone && same(&chunk[i + k], gun.prone, 8))
                                prone = k;
                        }
                        if (!zoom || !crouch || !prone || zoom > crouch || crouch > prone)
                            continue;
                        ++found;
                        for (const size_t k : {size_t{0}, crouch, prone})
                            for (size_t row = 0; row < 2; ++row) {
                                SpreadRow r{place + (k + row * 4) * sizeof(float)};
                                std::memcpy(r.game, &chunk[i + k + row * 4], sizeof(r.game));
                                std::memcpy(r.wanted, &gun.zoom[row * 4], sizeof(r.wanted));
                                r.kind = wanted;
                                rows.push_back(r);
                            }
                    }
                }
                if (got < want || want < chunkBytes)
                    break;
                from += want - kSwayFloats * sizeof(float);  // its data across the seam is seen whole
            }
        }
        if (end <= at)
            break;
        at = end;
    }
    // Added to those known: one rewritten already isn't found again and stays known. It had to be 6 or fewer, or none was written, and each new scan replaced the last: with 7, 9 and 7 in the
    // user's run of 2026-11-04 (more 600 m/s guns' data loaded), their bullets went from the soldier's head all run ("check the aiming again"), and an old place would have kept a stale start.
    if (const size_t known = AddPlaces(g_rifleShot, shots); !shots.empty())
        Log("Rifle: its bullets' start found in %zu more places, %zu known", shots.size(), known);
    if (found > 24) {
        Log("Rifle: %d places like its scatter's, %llu ms: left alone", found, GetTickCount64() - began);
        return 0;
    }
    if (found) {
        {
            std::lock_guard lock(g_rifleSpreadMutex);
            for (const SpreadRow& r : rows)
                if (std::none_of(g_rifleSpreadRows.begin(), g_rifleSpreadRows.end(), [&](const SpreadRow& o) { return o.place == r.place; }))
                    g_rifleSpreadRows.push_back(r);
        }
        Log("Rifle: its scatter's numbers found in %d places, %llu ms", found, GetTickCount64() - began);
        if (g_rifleSpread.load(std::memory_order_relaxed))
            ApplyRifleSpread(true);
    } else {
        Log("Rifle: no scatter's numbers left to change, %llu ms", GetTickCount64() - began);
    }
    return 0;
}

bool DrawGun(int which, ID3D11DeviceContext* ctx, UINT count, UINT start, INT base, size_t pass) {
    const Upload& u = t_upload;
    if (!u.resource || u.bytes.size() < kGunPrefix || !PlayersOwn(u.bytes))
        return false;
    if (pass == 0) {
        g_rifleDrawn = GetTickCount64();
        g_rifleFrame = g_frameSerial.load(std::memory_order_relaxed);
        g_magBoxMin = kMagMin, g_magBoxMax = kMagMax;
    }
    bool out = g_magOut.load(std::memory_order_relaxed), shown = g_dropShown.load(std::memory_order_relaxed);
    // Whole palettes as big as BF3's buffer; the gun reads only its first bones.
    thread_local std::vector<uint8_t> t_gun, t_dropped, t_held;
    t_gun.assign(kGloveUploadSize, 0);
    std::memcpy(t_gun.data(), u.bytes.data(), kGunPrefix);
    auto* bones = reinterpret_cast<float*>(t_gun.data());
    bool moved = false, held = false;
    bool stowed = false;  // drawn nowhere (HandTargets::stowed): what rides this draw and isn't the gun still drawn
    WorldPlacing placing;  // this draw's, for the magazine kept in the world
    // A magazine in either hand (pouch.cpp): only its bones, at its pose and their seats. Under g_handsMutex, with t through ToArms.
    auto magInHand = [&](const skin::HandTargets& t) {
        t_held.assign(kGloveUploadSize, 0);  // every other bone of the gun on one point
        for (size_t i = 0; i < kMagBones; ++i)
            if (kMagazineBones[i].pass == pass && g_haveMagSeat[i]) {
                Compose(t.magPose, g_magSeat[i], reinterpret_cast<float*>(t_held.data() + kMagazineBones[i].bone * 48));
                held = true;
            }
    };
    {
        std::lock_guard lock(g_handsMutex);
        if (pass != 1) {  // bone 2, the gun's body, in the colour and the depth pass
            std::memcpy(g_gunBodyOrig, bones + 2 * 12, sizeof(g_gunBodyOrig));
            g_haveGunBodyOrig = true;
            g_gunBodyView = g_viewUploads.load();
            LearnKept(g_gunBodyView);
        }
        skin::HandTargets t = g_hands;
        if (t.heldMagHidden)  // the hand is drawing it down the well: it's the one in the hand that's drawn
            out = true;
        // Drawn by the mod in the hand (gun), or with no hand on it (gunDrawn): flying into a hand, or lying where it fell. Put away, or its hand not seen at a place
        // (stowed): drawn nowhere, never where the game holds it (the stamps above still say it's the game's weapon); only what rides this draw and isn't the gun,
        // below: the magazines lying (loose, and the old fall's), one in a hand.
        if (g_handsOn && t.stowed) {
            stowed = true;
            // Drawn all the while: no load (its data's scan, below, goes by the gap). Not after a gap of 15 s without it, in a hand or left out: a load while it was
            // away is one, and a frame or two still put away after it would hide it.
            if (const ULONGLONG nowMs = GetTickCount64(); nowMs - std::max(g_gunDeltaTime, g_rifleStowedAt) <= 15000)
                g_rifleStowedAt = nowMs;
            if (ToArms(t, ctx)) {
                placing.Take(t);
                if (t.mag)
                    magInHand(t);
            }
        } else if (g_handsOn && (t.gun || t.gunDrawn) && ToArms(t, ctx)) {
            if (!t.gun && t.gunLying && !t.haveDrawnFromWorld)
                return true;  // lying, it's placed from the world: not drawn by a draw without the world's way into it
            placing.Take(t);
            // Bone 2 is the gun's body in the colour and depth passes: the move comes from this
            // draw's own. The rounds' draw has none: its bone goes where its learned offset from
            // the gun's body (as last drawn) puts it.
            float inverse[12];
            if (pass != 1 && Invert(bones + 2 * 12, inverse)) {
                if (static ULONGLONG s_partAt[3] = {}; g_gunPartLog.load(std::memory_order_relaxed) && GetTickCount64() - s_partAt[pass] >= 500) {
                    s_partAt[pass] = GetTickCount64();
                    for (int k = 0; k < 6; ++k) {
                        float rel[12];
                        Compose(inverse, bones + k * 12, rel);
                        Log("Gun parts: the rifle's pass %d bone %d on its body: at (%.3f %.3f %.3f), its z (%.3f %.3f %.3f), its y (%.3f %.3f %.3f)", static_cast<int>(pass), k, rel[3], rel[7], rel[11], rel[2], rel[6], rel[10], rel[1], rel[5], rel[9]);
                    }
                }
                PinSeatedBones(pass, inverse, bones);
                if (static ULONGLONG s_aimAt = 0; pass == 0 && g_rifleLog.load(std::memory_order_relaxed) && GetTickCount64() - s_aimAt >= 1000) {
                    s_aimAt = GetTickCount64();
                    float view[12], rel[12];
                    {
                        std::lock_guard fp(g_fpMutex);
                        std::memcpy(view, g_viewToArms, sizeof(view));
                    }
                    Compose(inverse, view, rel);
                    // The game's aim, as written before any turn of the mod's: back = (sin yaw cos pitch, -sin pitch, -cos yaw cos pitch).
                    float yaw = 0.0f, pitch = 0.0f, aim[3] = {};
                    if (camera::GameWeaponAngles(yaw, pitch)) {
                        const float f[3] = {-std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch)};
                        const float* body = bones + 2 * 12;
                        for (int c = 0; c < 3; ++c)
                            aim[c] = body[c] * f[0] + body[4 + c] * f[1] + body[8 + c] * f[2];
                    }
                    // And the rifle as it's drawn, against that aim: its z's turn from it, right and up (degrees), its sights' 0.29 below z taken off they should be none.
                    float off[2] = {};
                    if (camera::GameWeaponAngles(yaw, pitch)) {
                        const float gunZ[3] = {t.gunSeen[2], t.gunSeen[6], t.gunSeen[10]};
                        const float cp = std::cos(pitch);
                        const float f[3] = {-std::sin(yaw) * cp, std::sin(pitch), std::cos(yaw) * cp};
                        const float left[3] = {f[2], 0.0f, -f[0]};
                        const float upward[3] = {f[1] * left[2] - f[2] * left[1], f[2] * left[0] - f[0] * left[2], f[0] * left[1] - f[1] * left[0]};
                        const float l = Length3(left[0], left[1], left[2]);
                        if (l > 1e-4f) {
                            off[0] = -(gunZ[0] * left[0] + gunZ[1] * left[1] + gunZ[2] * left[2]) / l * 57.29578f;
                            off[1] = (gunZ[0] * upward[0] + gunZ[1] * upward[1] + gunZ[2] * upward[2]) / l * 57.29578f;
                        }
                    }
                    Log("Rifle: the view on its body: eye (%.3f %.3f %.3f), forward (%.4f %.4f %.4f), the aim on it (%.4f %.4f %.4f), FOV source %.1f; drawn, its z off the aim %+.2f right %+.2f up", rel[3], rel[7], rel[11], -rel[2], -rel[6], -rel[10],
                        aim[0], aim[1], aim[2], camera::SourceFov() * 57.29578f, off[0], off[1]);
                }
                if (g_learnHold.load(std::memory_order_relaxed))  // the magazine's seat, as drawn now
                    for (size_t i = 0; i < kMagBones; ++i)
                        if (kMagazineBones[i].pass == pass) {
                            Compose(inverse, bones + kMagazineBones[i].bone * 12, g_magSeat[i]);
                            g_haveMagSeat[i] = true;
                        }
                Compose(t.gunPose, inverse, g_gunDelta);
                // What goes with the gun only while a hand holds it (gun): the game's effects moved with it, its bullets' start, its scatter's numbers. Lying or flying,
                // none of them (nothing fires then: the effects stay where the game has them).
                if (t.gun)
                    g_gunEffect.Note(t, t.gunSeen, inverse);
                // Its bullets start from the game's camera plus this, along the game's aim: made the sights' line at the muzzle's distance, so they go exactly where the sights point
                // wherever the head is (the camera is the soldier's, the eye the player's: leaning to the sights moved the sights off the bullets' line, the user's "little bit of an
                // off aim", 2026-11-04).
                static uint32_t s_bulletFrame = 0;  // once a frame
                if (float bullet[3]; t.gun && pass == 0 && s_bulletFrame != g_frameSerial.load(std::memory_order_relaxed) && g_rifleShotOn.load(std::memory_order_relaxed) &&
                    ShotStart(t, t.gunSeen, kRifleSightsAtMuzzle, bullet) && Length3(bullet[0], bullet[1], bullet[2]) < 3.0f) {
                    s_bulletFrame = g_frameSerial.load(std::memory_order_relaxed);
                    for (int i = 0; i < 3; ++i)
                        bullet[i] += g_rifleShotBy[i].load(std::memory_order_relaxed);
                    for (size_t n = 0; n < std::size(g_rifleShot); ++n) {
                        const uintptr_t place = g_rifleShot[n].load(std::memory_order_relaxed);
                        if (!place)
                            continue;
                        // Only over its own data, as it was left: a load can move it, and this memory is then something else's.
                        float there[11];
                        SIZE_T got = 0;
                        const bool ours = ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(place), there, sizeof(there), &got) && got == sizeof(there) && there[4] == 0.0f && there[5] == 0.0f &&
                                          there[6] == 1.0f && there[8] == 0.0f && there[9] == 0.0f && IsBulletSpeed(there[10]) &&
                                          ((there[0] == 0.0f && there[1] == 0.0f && there[2] == 0.0f) || std::memcmp(there, g_rifleShotLast[n], sizeof(g_rifleShotLast[n])) == 0);
                        if (!ours) {
                            g_rifleShot[n] = 0;
                            Log("Rifle: its bullets' start at %p isn't there any more: not written", reinterpret_cast<void*>(place));
                            continue;
                        }
                        if (WriteProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(place), bullet, sizeof(bullet), nullptr))
                            std::memcpy(g_rifleShotLast[n], bullet, sizeof(bullet));
                    }
                    if (static ULONGLONG s_said = 0; g_rifleLog.load(std::memory_order_relaxed) && GetTickCount64() - s_said >= 1000) {
                        s_said = GetTickCount64();
                        Log("Rifle: its bullets start at (%.2f %.2f %.2f) from the game's eye (left, up, forward)", bullet[0], bullet[1], bullet[2]);
                    }
                }
                if (t.gun) {
                    g_haveGunDelta = true;
                    const ULONGLONG gunBefore = std::max(g_gunDeltaTime, g_rifleStowedAt);  // (put away, the game drew it all the while: no load)
                    g_gunDeltaTime = GetTickCount64();
                    // Once for each level's data: its addresses are new after a load, which a long gap without the gun may have been.
                    // (One scan at a time, StartScan: wanted while another is under way, it's kept and started the first frame after.)
                    static bool s_spreadPending = false;
                    if (static std::atomic<ULONGLONG> s_spread{0};
                        s_spreadPending || !s_spread.load() || g_gunDeltaTime - s_spread.load() > 600000 ||
                        (g_gunDeltaTime - gunBefore > 15000 && g_gunDeltaTime - s_spread.load() > 15000)) {
                        s_spreadPending = ScanRunning();
                        if (!s_spreadPending) {
                            s_spread = g_gunDeltaTime;
                            StartScan(ClearRifleSpread, nullptr);
                        }
                    }
                }
                for (int k = 0; k < 6; ++k)
                    MoveBy(g_gunDelta, bones + k * 12);
                // The moving parts (the user saw them, 2026-10-30): the charging handle (bone 0)
                // slides back with the left hand (reload.cpp), and the ejection port's cover
                // (bone 3) swings open on its hinge when the bolt moves.
                {
                    const float* body = bones + 2 * 12;
                    const float z[3] = {body[2], body[6], body[10]};  // the barrel's way (forward)
                    const float length = std::sqrt(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
                    if (length > 0.0f) {
                        const float s = -reload::HandleBack() / length;
                        bones[3] += z[0] * s, bones[7] += z[1] * s, bones[11] += z[2] * s;
                    }
                    SwingCover(bones, reload::CoverAngle());
                }
                // weapon.txt "wiggle N": bone N slides 2 cm back and forth along the barrel, to tell
                // which part of the gun it is.
                if (const int w = g_wiggleBone.load(std::memory_order_relaxed); w >= 0 && w < 6 && w != 2) {
                    const float* body = bones + 2 * 12;
                    float z[3] = {body[2], body[6], body[10]};  // the barrel's way, in the arms' space
                    const float length = std::sqrt(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
                    const float s = length > 0.0f ? 0.02f * std::sin(6.2831853f * static_cast<float>(Seconds())) / length : 0.0f;
                    float* b = bones + w * 12;
                    b[3] += z[0] * s, b[7] += z[1] * s, b[11] += z[2] * s;
                }
                moved = true;
                if (pass == 0 && g_tracing.load(std::memory_order_relaxed)) {
                    TraceRow r{Seconds(), GetCurrentThreadId(), {}, {g_gunBodyOrig[3], g_gunBodyOrig[7], g_gunBodyOrig[11]},
                               g_viewUploads.load()};
                    {
                        std::lock_guard fp(g_fpMutex);
                        r.eye[0] = g_viewToArms[3], r.eye[1] = g_viewToArms[7], r.eye[2] = g_viewToArms[11];
                    }
                    std::lock_guard trace(g_traceMutex);
                    g_trace.push_back(r);
                    if (g_trace.size() >= 120)
                        g_tracing = false;
                }
            } else if (pass == 1) {
                // Learned from the gun's body drawn earlier in this view, or once it's drawn (as the
                // arms are: which comes first differs from launch to launch).
                Learned& l = g_learned[kWeaponPasses[1]];
                if (const uint32_t view = g_viewUploads.load();
                    g_haveGunBodyOrig && g_gunBodyView == view && Invert(g_gunBodyOrig, inverse))
                    Learn(l, 0, inverse, bones);
                else
                    KeepForLater(kRoundsDraw, bones, view);
                float rounds[12];
                std::memcpy(rounds, l.rel[0], sizeof(rounds));
                if (static ULONGLONG s_roundsAt = 0; g_gunPartLog.load(std::memory_order_relaxed) && GetTickCount64() - s_roundsAt >= 500) {
                    s_roundsAt = GetTickCount64();
                    Log("Gun parts: the rifle's pass 1 bone 0 on its body: at (%.3f %.3f %.3f), its z (%.3f %.3f %.3f), its y (%.3f %.3f %.3f)", rounds[3], rounds[7], rounds[11], rounds[2], rounds[6], rounds[10], rounds[1], rounds[5], rounds[9]);
                }
                if (GunSeated() && Length3(rounds[3] - kRoundsRest[0], rounds[7] - kRoundsRest[1], rounds[11] - kRoundsRest[2]) < kBoneRestLimit)
                    rounds[3] = kRoundsRest[0], rounds[7] = kRoundsRest[1], rounds[11] = kRoundsRest[2];
                if (g_learnHold.load(std::memory_order_relaxed) && l.have[0]) {
                    std::memcpy(g_magSeat[1], rounds, sizeof(g_magSeat[1]));  // kMagazineBones[1]: these rounds
                    g_haveMagSeat[1] = true;
                }
                if (l.have[0]) {
                    Compose(t.gunPose, rounds, bones);
                    moved = true;
                }
            }
            if (t.mag)
                magInHand(t);
        }
    }
    {
        std::lock_guard lock(g_magMutex);
        Drop& d = g_drop;
        if (!out && !stowed) {
            for (size_t i = 0; i < kMagBones; ++i) {
                if (kMagazineBones[i].pass != pass)
                    continue;
                DrawnToWorld(placing, bones + kMagazineBones[i].bone * 12, d.last[i], d.lastOrigin[i]);
                d.seen[i] = true;
            }
        }
        if (stowed)  // drawn nowhere: no magazine ejected out of it from where the game holds it
            for (size_t i = 0; i < kMagBones; ++i)
                if (kMagazineBones[i].pass == pass)
                    d.seen[i] = false;
        if (!moved && !out && !shown && !stowed)
            return false;
        if (out)
            for (size_t i = 0; i < kMagBones; ++i)
                if (kMagazineBones[i].pass == pass)
                    std::memset(bones + kMagazineBones[i].bone * 12, 0, 48);  // all on one point
        if (shown) {
            t_dropped.assign(kGloveUploadSize, 0);  // every other bone of the gun on one point
            for (size_t i = 0; i < kMagBones; ++i)
                if (kMagazineBones[i].pass == pass && d.started[i]) {
                    float world[12];
                    MoveBone(d.start[i], d, world);
                    WorldToDrawn(placing, world, d.origin0, reinterpret_cast<float*>(t_dropped.data() + kMagazineBones[i].bone * 48));
                }
        }
        if (held) {  // where it falls from if it's let go of
            const double now = Seconds();
            for (size_t i = 0; i < kMagBones; ++i)
                if (kMagazineBones[i].pass == pass) {
                    DrawnToWorld(placing, reinterpret_cast<const float*>(t_held.data() + kMagazineBones[i].bone * 48), d.held[i], d.heldOrigin[i]);
                    d.heldSeen[i] = true;
                    d.heldAt[i] = now;
                }
        }
    }
    if (stowed) {
        g_stowedSkips[kStowedBody].fetch_add(1, std::memory_order_relaxed);
    } else if (!(moved || out) ||
               !DrawWithPalette(which, ctx, count, start, base, u.resource, t_gun.data(), kGloveUploadSize)) {
        g_origDrawIndexed[which](ctx, count, start, base);
        if (moved || out)
            NoteUnmoved(kWeaponPasses[pass], 2);
    } else if (moved) {
        ++g_handsMovedDraws;
    }
    if (shown)
        DrawWithPalette(which, ctx, count, start, base, u.resource, t_dropped.data(), kGloveUploadSize);
    if (held && DrawWithPalette(which, ctx, count, start, base, u.resource, t_held.data(), kGloveUploadSize))
        ++g_heldMagDraws;
    // The magazines lying or flying (loose): each its own copy of this draw, its bones where it is, every other bone on one point.
    if (placing.have) {
        thread_local std::vector<uint8_t> t_loose;
        std::lock_guard lock(g_looseMutex);
        for (int p = 0; p < g_loosePoseCount; ++p) {
            const loose::Pose& pose = g_loosePoses[p];
            if (pose.kind != kLooseRifleMag)
                continue;
            for (const LooseMag& mag : g_looseMags) {
                if (mag.id != pose.id)
                    continue;
                t_loose.assign(kGloveUploadSize, 0);
                if (LooseMagazinePalette(pose, mag, pass, placing, reinterpret_cast<float*>(t_loose.data())) &&
                    DrawWithPalette(which, ctx, count, start, base, u.resource, t_loose.data(), kGloveUploadSize))
                    g_looseDraws.fetch_add(1, std::memory_order_relaxed);
                break;
            }
        }
    }
    return true;
}

// weapon.txt "scopetrace": a line for a draw of the scope's mesh, with the bones it was drawn with
// and what became of it.
void TraceScopeDraw(ID3D11DeviceContext* ctx, const ArmDraw& draw, const Upload& used, const char* outcome) {
    ID3D11Buffer* indices = nullptr;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    UINT indexOffset = 0;
    ctx->IAGetIndexBuffer(&indices, &format, &indexOffset);
    if (indices)
        indices->Release();
    const bool usedOwn = used.resource && used.bytes.size() == kGloveUploadSize && PlayersOwn(used.bytes);
    if (draw.key == kScopeBody && usedOwn)
        g_scopeIndices = indices;  // the player's own scope body names the scope's mesh
    if (!indices || indices != g_scopeIndices.load())
        return;
    LogScopeMaterial(ctx, draw.key);
    ID3D11Buffer* vertices = nullptr;
    UINT stride = 0, vertexOffset = 0;
    ctx->IAGetVertexBuffers(0, 1, &vertices, &stride, &vertexOffset);
    if (vertices)
        vertices->Release();
    ID3D11DepthStencilState* depth = nullptr;
    UINT ref = 0;
    ctx->OMGetDepthStencilState(&depth, &ref);
    if (depth)
        depth->Release();
    const auto* b = used.bytes.size() >= 48 ? reinterpret_cast<const float*>(used.bytes.data()) : nullptr;
    float gun[3] = {};
    {
        std::lock_guard lock(g_handsMutex);
        gun[0] = g_gunBodyOrig[3], gun[1] = g_gunBodyOrig[7], gun[2] = g_gunBodyOrig[11];
    }
    Log("Scope trace: %u/%u stride %u, stencil ref %u: latest upload %p (%s), first bone at (%.2f %.2f %.2f), the "
        "game's gun body at (%.2f %.2f %.2f): %s",
        draw.key.indexCount, draw.key.startIndex, stride, ref, static_cast<void*>(used.resource),
        !used.resource ? "none" : BoundSlot(ctx, used.resource) < 0 ? "not bound" : "bound", b ? b[3] : 0.0f,
        b ? b[7] : 0.0f, b ? b[11] : 0.0f, gun[0], gun[1], gun[2], outcome);
}

// Empty hands (The Great Destroyer, 2026-10-31: Blackburn bare-armed, a handcuff on his right wrist,
// no gun; the user: free hands there, but not in the interrogation and its QTEs). The bare arms are
// one draw, 18108 indices from 16053 in every launch, with the skin shader (code 090cf267a0729457),
// stencil 32, bones in the body's space like the gloves': 26-50 the left forearm and hand, 0-24 the
// right, each laid out alike (41 and 15 the palm, 46-50 and 20-24 the forearm: kBarePalm). Their
// rest pose is the gloves' (the left hand's middle at x +0.554 in the vertex buffer, the gloves'
// kLeftPalm +0.537; the right mirrored): taken the other way round at first, the left controller
// moved the right arm a metre off (the user's video, 14:30). Each arm moves whole with its
// controller, its fingers as the game animates them: the palm bone is put where the left glove's
// line-up puts the gloves' (the right mirrored; weapon.txt "barehands learn 5": learned from the
// game's own arms instead), and every bone of that side moved as the palm bone was. The cuff's ring
// (2682 from 13371) moves as the right arm last did. On only while xr_runtime.cpp allows it (no
// cutscene picture and no scripted turn for a few seconds, the aim found) and no gun is in hand.
// The draws themselves are barestate.h's, in both states the game has them in: handcuffed (the arms 18108 from 16053, the cuff, the body's
// 5604 / 7545 / 26346 / 7815) and with the cuffs off after Semper Fidelis's grapple (the same arms 18108 from 13371, no cuff, the body's
// draws moved: 2026-11-18, the arms were the game's own with the AKS-74u in hand). barestate::KindOf says which a draw is.
constexpr DrawKey kBareChain{3492, 0, 0};       // the chain and the open cuff hanging from it (bones 0-2 the links, 3-4 the cuff)
std::atomic<int> g_bareArmsState{-1};  // barestate::kStates' row the arms were last drawn in (said in the log when it changes)
// The body with the bare arms on: the legs, torso and sleeves (7545 + 5604, hidden: the camo and the
// feet went, 2026-10-31 14:45); folded away as the gloves' body is, or the sleeves stayed where the
// game holds the arms and the feet showed looking down (the user).
// Since 2026-11-18 the torso with its sleeves (5604) and the strip over the shoulders (222) are drawn again, posed on the arms solved from the hands as the
// gloves' levels' chest and sleeves are (DrawBareSleeves: each arm whole up to its shoulder, the chest folded between the shoulders); hidden as before only
// without an arm solved whole ("baresleeves 0", a hand not tracked). The legs, the depth-only copies, the chain and the pistol's copy stay hidden.
// With them their depth-only copies (stride 16, the same shader): the body's 26346 from 34161, one hand's
// 7815 from 60507 (a skeleton of its own, 21 bones), the chain's 3492 from 3492. Left where the game holds
// the arms they showed black, low by the waist (the user's video, 14:50); the moved arms draw without them.
// And 222 from 13149 (the legs' shader): a small piece of uniform each side, two white sheets in the
// corners of the view (the user, 15:14).
// With them the chain and its open cuff (kBareChain) for now: moved with the hand they jittered whichever palm they went by (the user:
// "still absolutely jittery", 15:23; set aside); and the pistol's depth-only copy (11970 from 11970), left where the game holds it (a black
// pistol). All of them barestate.h's kBody, with what each is with the cuffs off.
constexpr uint64_t kSkinShader = 0x090cf267a0729457ull;
// The palms: 41 (left) and 15 (right), the gloves' 1 and 32 by their vertices at the same rest place
// (screenshotsbonematch.ps1: 79 and 80 vertices within 8 mm), the bone the gloves' hand is placed by.
// 45 and 19, taken for them at first by their vertex count, are the THUMB'S LAST BONES (the gloves' 2
// and 39, 115 vertices within 4.5 mm): the whole hand went where the game's thumb went, rolled and
// pulled off its wrist (the user's thin, twisted "candy wrapper" wrist, and the pistol learned from a
// thumb, its fingers through the grip; 2026-10-31 15:52-16:36).
using barehold::kBarePalm;  // left, right
// Each bare arm bone as the gloves' (bonematch.ps1, the vertices at the same rest place): the right hand's
// 0-20 the right glove's 23-44, the left's 26-46 the left glove's 0-22, -1 the bare forearms' own (and the
// upper arms'). So handpose.cpp's fingers work on the bare hands: BendRight with this as its map, and the
// left glove's poses put on the bare left hand. (The table itself: barehold.h.)
using barehold::kBareBones;
using barehold::kBareGlove;
// Each side's palette entries, first and last (left, right): 51 and 25 with them, the upper arm's, which a few
// elbow-end vertices lean on; left in place they stretched a white sheet of skin and sleeve from the moved
// arm back to where the game holds it (the user's "piece of my uniform", 15:07).
using barehold::kBareSide;
std::atomic<bool> g_bareAllowed{false};
float g_bareOffset[2][12] = {};          // the palm bone from its controller's pose (under g_handsMutex)
bool g_haveBareOffset[2] = {};
float g_bareDelta[2][12] = {};           // each side's last move (for the cuff)
bool g_haveBareDelta[2] = {};
// The right palm bone as the game last drew it, per view (no eye, left, right): the cuff and chain go by
// it. The game poses the arms for each eye's view of its own, the palm 4-40 cm apart between them (the
// log, 15:13): one kept for both, the chain took the other eye's and jittered, doubled (the user).
float g_bareGamePalm[12][12] = {};  // by view and pass (BarePass)
float g_bareGameWrist[12][12] = {}; // the right forearm's wrist end (20, twist B) alike: the cuff goes by it
// The game's bones in its last four arm draws, both hands (the pistol learns its holds from them).
float g_barePalmRing[4][kBareBones][12] = {};
uint32_t g_barePalmFrame[4] = {};  // each one's frame (g_frameSerial)
int g_barePalmEye[4] = {-2, -2, -2, -2};
thread_local uint64_t t_drawSerial = 0;  // the draws so far (DrawIndexed)
int g_barePalmNext = 0;
bool g_haveBareGamePalm[12] = {};
std::atomic<uint32_t> g_bareDraws{0};

bool SkinShaderBound(ID3D11DeviceContext* ctx) {
    ID3D11PixelShader* ps = nullptr;
    ctx->PSGetShader(&ps, nullptr, nullptr);
    if (!ps)
        return false;
    const bool skin = shaders::ChecksumOf(ps) == kSkinShader;
    ps->Release();
    return skin;
}

void LearnPistolHand(const float* frame, const float* game);
bool PistolLearning();
// The gloves' draw placed on a gun in hand (DrawLauncherRig's own placing, below): for the bare arms with a gun in hand, in the gloves' numbers.
struct RigPlaced {
    bool here[2] = {};      // each hand drawn (left, right): not folded away
    float fold[2][3] = {};  // where each side folds when it isn't
};
bool RigDrawnLately();
bool PlaceOnRig(const skin::HandTargets& t, const ArmDraw& draw, float* bones, ULONGLONG now, bool learn = true, const float* forearmsDraw = nullptr, RigPlaced* placedOut = nullptr);
// The right hand as last drawn on a held gun, per view and pass (as g_bareGamePalm): the cuff still round the wrist (the train before the grapple) goes with it.
float g_bareRigPalm[12][12] = {};
bool g_haveBareRigPalm[12] = {}, g_bareRigRightHere[12] = {}, g_bareRigRightSolved[12] = {};
uint32_t g_bareRigFrame[12] = {};
float FistToGrip(const float* frame, const float* root, bool left = false);
// The game's right fist within this of the pistol's grip: the hand that holds it (the other pass's arms, or
// the pistol on its way up, are further). An estimate from offline fits (about 2 cm in a closed hand).
constexpr float kFistReach = 0.08f;
// Two draws of one frame (or the next): BF3 records on several threads, each with its own draw count
// (t_drawSerial), so draws are paired by the frames shown (g_frameSerial) instead.
inline bool FramesNear(uint32_t a, uint32_t b) {
    return (a > b ? a - b : b - a) <= 1;
}
// The arms as last drawn moved in each view and pass (as g_bareGamePalm; under g_handsMutex): which of them had its upper arm's bones on the solved upper arm (its
// sleeve goes on it), in which frame, and the frames they were drawn on (in that view's space: each arm's elbow end, twist bones, upper arm, collar; the chest's point).
struct BareSleeveView {
    bool on[2] = {};
    uint32_t frame = 0;
    float frames[2][barehold::kArmFrames][12] = {};
    float chest[3] = {};
};
BareSleeveView g_bareSleeve[12];
static_assert(sizeof(BareSleeveView::frames) == sizeof(skin::HandTargets::bareForearm));

// The pistol in the right hand (its hold learned and the pistol drawn moved a moment ago).
bool PistolHeld() {
    return g_haveBarePistolHand.load(std::memory_order_acquire) && GetTickCount64() - g_barePistolAt.load() <= 300;
}

bool DrawBareArms(int which, ID3D11DeviceContext* ctx, UINT count, UINT start, INT base, bool cuff,
                  bool depth = false) {
    if (!g_bareOn.load(std::memory_order_relaxed) || !g_bareAllowed.load(std::memory_order_relaxed) ||
        GetTickCount64() - g_rifleDrawn.load() < 1000)
        return false;
    // A line-up under way: the game's own arms until it's time, then learned from them.
    if (const ULONGLONG at = g_bareLineUpAt.load(std::memory_order_relaxed); at) {
        if (GetTickCount64() < at)
            return false;
        g_bareLineUpAt = 0;
        g_bareRelearn = true;
        Log("Bare hands: lined up with the game's arms");
    }
    const Upload& u = t_upload;
    // Any pass of it: the depth pass has no pixel shader, and left out it drew a black copy of the arms
    // where the game holds them (the user). Its bones being the player's own keeps other soldiers out.
    if (!u.resource || u.bytes.size() != kGloveUploadSize || !PlayersOwn(u.bytes))
        return false;
    thread_local std::vector<uint8_t> t_bare;
    t_bare = u.bytes;
    auto* bones = reinterpret_cast<float*>(t_bare.data());
    bool moved = false;
    {
        std::lock_guard lock(g_handsMutex);
        if (!g_handsOn)
            return false;
        skin::HandTargets t = g_hands;
        int eye = -1;
        if (!ToArms(t, ctx, &eye))
            return false;
        // Each eye draws the arms twice a frame, in two passes with bones of their own: the palm 5-67 cm
        // apart between them (the log, 15:18). The cuff and chain go by the arms' draw of their own pass:
        // this view, its stencil reference, a pixel shader or none. One palm for all passes, they jumped
        // between two poses (the user: "extremely jittery").
        int view = eye >= 0 && eye <= 1 ? eye + 1 : 0;
        {
            ID3D11PixelShader* ps = nullptr;
            ctx->PSGetShader(&ps, nullptr, nullptr);
            ID3D11DepthStencilState* state = nullptr;
            UINT ref = 0;
            ctx->OMGetDepthStencilState(&state, &ref);
            view = view * 4 + (ps ? 1 : 0) + (ref == 32 ? 2 : 0);
            if (ps)
                ps->Release();
            if (state)
                state->Release();
        }
        if (g_recUntil.load(std::memory_order_relaxed))  // weapon.txt "reloadrec": the game's own bones
            Record(1, eye, view, u.bytes, kBareBones);
        if (g_bareRelearn.exchange(false))
            g_haveBareOffset[0] = g_haveBareOffset[1] = false;
        // A gun in hand (any but the train pistol, which has its own rig): the bare arms are the gloves under another numbering (barehold.h). The game's palette in
        // the gloves' numbers goes through the gloves' own placing (PlaceOnRig, DrawLauncherRig's): the game's hold on the gun's frame is learned from it (the right
        // hand round the pistol grip, the game's left on the handguard, the palms' rest: NoteHeldPalms) and each hand placed as a glove is: on the gun as the game
        // holds it, or by its own pose and fitted fingers (the magazine's hold, the lever's fists, the handguard, a shell's pinch), or folded away. What comes back
        // is the hands; the forearms go by the arms solved from those hands as drawn (hands.cpp PlaceBareForearms), their open elbow ends capped off the arm's line.
        // (Before, 2026-11-18: each palm at its controller's line-up with the game's own fingers: the right a fist under the receiver, the left flat and open.)
        const bool onRig = g_bareHold.load(std::memory_order_relaxed) && RigDrawnLately() && !PistolHeld() && (t.gun || t.launcherDrawn || t.stowed);
        const uint32_t frameNow = g_frameSerial.load(std::memory_order_relaxed);
        bool sleeveOn[2] = {};  // each arm drawn here with its upper arm's bones on the solved upper arm: its sleeve goes on it (DrawBareSleeves, this view's)
        bool sides = !onRig;  // each side placed at its controller (below): with no gun in hand; with one, only the cuff going by its solved forearm
        if (onRig && cuff) {
            // The cuff round the right wrist: with the forearm's wrist end as solved (below, as ever); with none solved, as the right palm was moved in this view's
            // arms' draw; not drawn with the hand folded away, or before the arms' own draw of this view.
            if (!g_haveBareRigPalm[view] || !FramesNear(g_bareRigFrame[view], frameNow) || !g_haveBareGamePalm[view])
                return false;
            if (!g_bareRigRightHere[view])
                return true;
            sides = t.bareArm[1] && g_bareRigRightSolved[view];
            if (!sides) {
                float inverse[12], delta[12];
                if (!Invert(g_bareGamePalm[view], inverse))
                    return false;
                Compose(g_bareRigPalm[view], inverse, delta);
                for (int k = 0; k < static_cast<int>(kPaletteBones); ++k)
                    MoveBy(delta, bones + k * 12);
                moved = true;
            }
        } else if (onRig) {
            const float* game = reinterpret_cast<const float*>(u.bytes.data());
            float gloves[barehold::kGloveBones * 12], forearms[barehold::kForearmsDrawBones * 12];
            barehold::ToGloves(game, gloves);
            for (int k = 0; k < barehold::kForearmsDrawBones; ++k)
                std::memcpy(forearms + k * 12, game, 12 * sizeof(float));
            barehold::ToForearmsDraw(game, forearms);
            // (Learned from the first-person colour pass alone, stencil 32 with a pixel shader: a level's other pass of the arms has bones of its own. Every pass is
            // placed by what's learned.)
            RigPlaced rig;
            if (!PlaceOnRig(t, kArmDraws[0], gloves, GetTickCount64(), view % 4 == 3, forearms, &rig))
                return false;
            std::memcpy(g_bareGamePalm[view], game + kBarePalm[1] * 12, sizeof(g_bareGamePalm[view]));
            std::memcpy(g_bareGameWrist[view], game + 20 * 12, sizeof(g_bareGameWrist[view]));
            g_haveBareGamePalm[view] = true;
            barehold::Arm arms[2];
            for (int side = 0; side < 2; ++side) {
                arms[side].have = t.bareArm[side];
                arms[side].frame = t.bareForearm[side];
                arms[side].cap = t.bareCap[side];
                arms[side].sleeve = t.bareSleeve[side] && t.haveBareChest;
            }
            const barehold::Placed placed = barehold::FromRig(gloves, rig.here, rig.fold, arms, bones);
            sleeveOn[0] = placed.sleeve[0], sleeveOn[1] = placed.sleeve[1];
            std::memcpy(g_bareRigPalm[view], bones + kBarePalm[1] * 12, sizeof(g_bareRigPalm[view]));
            g_haveBareRigPalm[view] = true;
            g_bareRigRightHere[view] = rig.here[1];
            g_bareRigRightSolved[view] = placed.solved[1];
            g_bareRigFrame[view] = frameNow;
            if (static ULONGLONG s_said = 0; GetTickCount64() - s_said >= 1000) {
                s_said = GetTickCount64();
                auto how = [&](int side) {
                    return !rig.here[side] ? "folded away" : placed.solved[side] ? "its forearm solved from it" : t.bareArm[side] ? "its forearm whole with it (solved from another hand)" : "its forearm whole with it";
                };
                Log("Bare arms: on the gun in hand as the gloves are: the left %s (%s), the right %s (%s); wrists %.0f / %.0f mm from their solved forearms; caps %.2f / %.2f",
                    !rig.here[0] ? "none" : (t.launcherLeftFree || t.stowed) && t.left ? "the player's own" : "the game's on the gun", how(0),
                    !rig.here[1] ? "none" : t.rightGlove == skin::kRightOnGrip ? "on the grip as the game holds it" : t.rightGlove == skin::kRightOwn ? "its own pose" : "the game's", how(1),
                    placed.wristOff[0] * 1000.0f, placed.wristOff[1] * 1000.0f, t.bareCap[0], t.bareCap[1]);
            }
            moved = true;
        }
        for (int side = 0; side < 2 && sides; ++side) {
            const bool learn = g_bareLearn.load(std::memory_order_relaxed);
            const bool tracked = side == 0 ? t.left : learn ? t.gun : t.right;
            const float* pose = side == 0 ? t.leftPose : learn ? t.gunPose : t.rightPose;
            float delta[12];
            {
                // The cuff and its chain go with the right hand: its palm as the game drew it for this
                // view (g_bareGamePalm) put by this draw's own view. Moved as the arms' last draw was,
                // they took the other eye's view and doubled (the user's video, 14:50).
                if (cuff && (side == 0 || !g_haveBareGamePalm[view]))
                    continue;
                if (!tracked)
                    continue;
                const float* palm = cuff ? g_bareGamePalm[view] : bones + kBarePalm[side] * 12;
                // With the wrist solved the cuff goes with the forearm's wrist end, not the palm: bending
                // the wrist swung it through the wrist (the user, 15:27).
                if (cuff && t.bareArm[1]) {
                    float inverse[12];
                    if (!Invert(g_bareGameWrist[view], inverse))
                        continue;
                    Compose(t.bareForearm[1][2], inverse, delta);
                    for (int k = 0; k < static_cast<int>(kPaletteBones); ++k)
                        MoveBy(delta, bones + k * 12);
                    moved = true;
                    continue;
                }
                if (!cuff && side == 1 && !depth) {
                    // The game's own right palm moving more than 4 cm between this view's draws: its
                    // animation jumping, which the arm follows (the user: the hands jump swaying, 15:06).
                    if (g_haveBareGamePalm[view]) {
                        const float* last = g_bareGamePalm[view];
                        const float d[3] = {palm[3] - last[3], palm[7] - last[7], palm[11] - last[11]};
                        const float moved = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
                        static ULONGLONG s_logged = 0;
                        if (moved > 0.04f && GetTickCount64() - s_logged > 200) {
                            s_logged = GetTickCount64();
                            Log("Bare hands: the game's right palm jumped %.0f cm between view %d's draws",
                                moved * 100.0f, view);
                        }
                    }
                    std::memcpy(g_bareGamePalm[view], palm, sizeof(g_bareGamePalm[view]));
                    g_haveBareGamePalm[view] = true;
                    std::memcpy(g_bareGameWrist[view], bones + 20 * 12, sizeof(g_bareGameWrist[view]));
                    // The game's own bones, both hands (the left side's are already moved in t_bare).
                    const float* game = reinterpret_cast<const float*>(u.bytes.data());
                    std::memcpy(g_barePalmRing[g_barePalmNext], game, sizeof(g_barePalmRing[0]));
                    g_barePalmFrame[g_barePalmNext] = g_frameSerial.load(std::memory_order_relaxed);
                    g_barePalmEye[g_barePalmNext] = eye;
                    if (const int s = eye >= 0 && eye <= 1 ? eye + 1 : 0;
                        g_barePistolFrameNo[s] && FramesNear(g_barePalmFrame[g_barePalmNext], g_barePistolFrameNo[s]) &&
                        PistolLearning() && FistToGrip(g_barePistolFrame[s], game + kBarePalm[1] * 12) < kFistReach)
                        LearnPistolHand(g_barePistolFrame[s], game);
                    g_barePalmNext = (g_barePalmNext + 1) % 4;
                }
                float inverse[12], target[12];
                if (learn && !g_haveBareOffset[side]) {
                    if (cuff || depth)
                        continue;  // learned from the arms' own colour pass
                    float poseInverse[12];
                    if (!Invert(pose, poseInverse))
                        continue;
                    Compose(poseInverse, palm, g_bareOffset[side]);
                    g_haveBareOffset[side] = true;
                    Log("Bare hands: the %s hand's offset from its controller learned", side == 0 ? "left" : "right");
                }
                if (!Invert(palm, inverse))
                    continue;
                // By default the palm bone is put as the left glove's line-up puts the gloves' (their
                // skeletons alike; the right mirrored): learned where the game drew it, each hand
                // landed somewhere of its own (the user: "in complete different positions").
                if (learn)
                    Compose(pose, g_bareOffset[side], target);
                else
                    std::memcpy(target, pose, sizeof(target));
                Compose(target, inverse, delta);
                std::memcpy(g_bareDelta[side], delta, sizeof(delta));
                g_haveBareDelta[side] = true;
            }
            const int first = cuff ? 0 : kBareSide[side][0], last = cuff ? kPaletteBones - 1 : kBareSide[side][1];
            for (int k = first; k <= last && k < static_cast<int>(kPaletteBones); ++k)
                MoveBy(delta, bones + k * 12);
            // The forearm's elbow end (21 and 47, with the rolled sleeve's hem) folded onto the next bone
            // in: the whole forearm moves rigidly with the hand, and with the hand low the hem swung up
            // into a corner of the view (the user, 14:55). weapon.txt "bareelbow 0" keeps it.
            // The forearm solved from the hand (hands.cpp PlaceBareForearms): its bones put on the solved
            // elbow end and twist bones, so the hand bends at the wrist and the forearm stays on the arm's
            // line. Matched by where they sit in the rest pose, wrist to elbow (as the gloves' kGlovesForearm
            // has theirs): 46 / 20 on twist B, 49 / 23 on A, the rest (50, 47 and past the elbow 48, 51;
            // the right's 24, 21, 22, 25) on the elbow end.
            // (The table: barehold.h's kForearm. The open end's rim, 47 / 48 / 51 and 21 / 22 / 25, drawn in on the elbow joint by the cap, armik::CapFrame: none on
            // the arm's own line, where the frame is as it was.)
            // With the arm's sleeve on it (t.bareSleeve: the torso's draw posed on the same arm, DrawBareSleeves) the upper arm's two, 47 / 48 and 21 / 22, go on the
            // solved upper arm as the sleeve does, and nothing is capped: the arm is whole.
            if (!cuff && t.bareArm[side]) {
                sleeveOn[side] = t.bareSleeve[side] && t.haveBareChest;
                barehold::PlaceForearmBones(side, t.bareForearm[side], t.bareCap[side], sleeveOn[side], bones);
            } else if (!cuff && g_bareElbow.load(std::memory_order_relaxed)) {
                // Onto the forearm's middle as the next bone in carries it (the rest pose's, from the
                // vertex buffer: the left's at x +0.437, the right's mirrored).
                const int end = side == 0 ? 47 : 21, next = side == 0 ? 46 : 20;
                const float rest[3] = {side == 0 ? 0.4367f : -0.4367f, 1.1995f, 0.0656f};
                float point[3];
                const float* m = bones + next * 12;
                for (int i = 0; i < 3; ++i)
                    point[i] = m[i * 4] * rest[0] + m[i * 4 + 1] * rest[1] + m[i * 4 + 2] * rest[2] + m[i * 4 + 3];
                Collapse(bones + end * 12, point);
                Collapse(bones + (side == 0 ? 51 : 25) * 12, point);  // the upper arm's, with it
            }
            // The fingers, handpose.cpp's through kBareGlove. While the right hand holds the pistol the left's
            // are as the controller holds them, blended into the game's own two-handed hold as the left hand
            // takes the pistol (hands.cpp: the user, "use this left-hand model ... for two-handed mode only",
            // 16:55); and the right's middle, ring and little fingers open a little on the grip (they went
            // into it: the user, 16:50).
            if (!cuff && side == 0 && t.bareLeftOn) {
                const float* palm = bones + kBarePalm[0] * 12;
                for (int k = kBareSide[0][0]; k <= kBareSide[0][1]; ++k)
                    if (kBareGlove[k] > 1)  // not the palm (1) nor the forearm's wrist end (0)
                        Compose(palm, t.bareLeftFingers[kBareGlove[k]], bones + k * 12);
            }
            if (!cuff && side == 1 && PistolHeld()) {
                // One hand's opening, easing to both hands' as the left hand takes the pistol (hands.cpp).
                const float both = std::clamp(t.pistolTwoHanded, 0.0f, 1.0f);
                auto opening = [both](int i) {
                    const float one = g_pistolOpen[i].load(std::memory_order_relaxed),
                                two = g_pistolOpen2[i].load(std::memory_order_relaxed);
                    return -(one + (two - one) * both);
                };
                const float open[5] = {0.0f, 0.0f, opening(0), opening(1), opening(2)};
                handpose::BendRight(reinterpret_cast<float(*)[12]>(bones), open, kBareGlove, kBareBones);
            }
            moved = true;
        }
        // What this view's sleeves go by (DrawBareSleeves, the torso's draw after this one): the very frames these arms were drawn on, so the two are never of two poses.
        if (!cuff && moved) {
            BareSleeveView& s = g_bareSleeve[view];
            s.on[0] = sleeveOn[0], s.on[1] = sleeveOn[1];
            s.frame = frameNow;
            if (sleeveOn[0] || sleeveOn[1]) {
                std::memcpy(s.frames, t.bareForearm, sizeof(s.frames));
                std::memcpy(s.chest, t.bareChest, sizeof(s.chest));
            }
        }
    }
    if (!moved)
        return false;
    const bool drawn = DrawWithPalette(which, ctx, count, start, base, u.resource, t_bare.data(), kGloveUploadSize);
    if (drawn) {
        g_bareDraws.fetch_add(1, std::memory_order_relaxed);
        if (!cuff)
            g_bareMovedAt = GetTickCount64();
    }
    return drawn;
}

// The body's draws while the bare arms are on the controllers: left out (their bones the player's own).
// The pistol Blackburn picks up in the train (11970 from 0: 0 magazine, 1 hammer, 2 trigger, 3 frame,
// 4 slide; one draw an eye), one-handed: held in the bare right hand as the game holds it. Its frame
// bone from the game's right palm (kBarePistolHold) is learned once, from the arm draw whose palm sits
// at the pistol's grip (each eye has two arm draws, their palms up to 67 cm apart: the nearer one is
// the hand that holds it), then the pistol is put at the right hand's place times that and every bone
// moved as the frame was. The user, 2026-10-31: the gun in the right hand first, two-handed after.
constexpr DrawKey kBarePistol{11970, 0, 0};
// The grip in the frame bone's (3) bind space: the seated magazine's middle. (-0.0001 -0.1514 0.1001), taken
// for it at first, is the magazine's middle where its vertices lie apart in the vertex buffer, 13.6 cm off
// below and ahead of the gun (bone 0 seats it: 16 degrees about x, 13.8 cm up; the slide's vertices lie 30
// cm ahead too. The arm and grip study, screenshots\wf_pistol_grip\pistol_assembled.png, 2026-10-31).
constexpr float kPistolGripRest[3] = {-0.0001f, -0.0354f, 0.0299f};
// Each fist's middle in the arms' bind space: the hand root's (15; the left's 41 mirrored) vertices' middle,
// 4 cm out of the palm. A grip sits about 2 cm from it in a closed hand (the study's offline fits).
constexpr float kRightFistRest[3] = {-0.5045f, 1.0692f, 0.1936f};
constexpr float kLeftFistRest[3] = {0.5045f, 1.0692f, 0.1936f};
// The game's left hand round the right (its two-handed hold): its fist 4.9 cm from the grip; 21-26 cm while
// the pistol comes up, before the left hand joins it (the log, 17:10:38), and 9.4 cm on its way in.
constexpr float kLeftFistReach = 0.07f;
bool g_barePistolFitted = false;  // the hold is the offline fit's (nothing learned in 5 s): still looked for

float FistToGrip(const float* frame, const float* root, bool left) {
    float grip[3], fist[3];
    TransformPoint(frame, kPistolGripRest, grip);
    TransformPoint(root, left ? kLeftFistRest : kRightFistRest, fist);
    return Length3(fist[0] - grip[0], fist[1] - grip[1], fist[2] - grip[2]);
}

// Still learning: no hold yet (or only the offline fit), or within 1.5 s of it (pairs then are logged, and
// the left hand's hold taken from one that agrees with the right's if the first had the left hand off the
// pistol); never in the pistol's first second in hand (it's picked up or raised then, as the M416's hold
// waits: HoldIsNormal). weapon.txt "barehands pistol" starts again. Under g_handsMutex.
bool PistolLearning() {
    const ULONGLONG now = GetTickCount64();
    if (g_bareRelearnPistol.exchange(false)) {
        g_haveBarePistolHand = g_haveBarePistolLeft = false;
        g_barePistolFitted = false;
        g_barePistolLearnedAt = 0;
        g_pistolSince = now;
        Log("Bare hands: the pistol's holds forgotten: learned again in a second");
    }
    if (!g_pistolSince || now - g_pistolSince < 1000)
        return false;
    // Until both hands are learned from one pair, up to 10 s (the game's left hand joins the pistol after
    // the right: 0.45 s later at 17:10:38).
    return !g_haveBarePistolHand || g_barePistolFitted ||
           now - g_barePistolLearnedAt < (g_haveBarePistolLeft ? 1500u : 10000u);
}

// The game's hands on the pistol, from its frame and an arm draw's bones (game: all the game's, one pose).
// The right hand root's hold is kept from the first pair with its fist on the grip; the left palm and its
// fingers with it if the left hand is at the grip too, the game's two-handed hold.
void LearnPistolHand(const float* frame, const float* game) {
    auto apart = [](const float* a, const float* b) { return Length3(a[0] - b[0], a[1] - b[1], a[2] - b[2]); };
    const float* palm = game + kBarePalm[1] * 12;
    const float* left = game + kBarePalm[0] * 12;
    float inverse[12];
    const float d = FistToGrip(frame, palm), dl = FistToGrip(frame, left, true);
    static ULONGLONG s_logged = 0;
    if (d > kFistReach || !Invert(frame, inverse)) {
        if (GetTickCount64() - s_logged > 2000) {
            s_logged = GetTickCount64();
            Log("Bare hands: the pistol's hand not learned: the game's fist %.1f cm from its grip", d * 100.0f);
        }
        return;
    }
    float hold[12], leftHold[12];
    Compose(inverse, palm, hold);
    Compose(inverse, left, leftHold);
    // A pair that agreed with the kept hold at least 150 ms before: the pose has settled when neither fist
    // has moved 2 mm on the pistol since (the game's right fist still came 1 cm in over the first 0.1 s and
    // the left hand joins after, 17:10:38; one frame's pairs, both eyes, are alike and prove nothing).
    static float s_ref[12] = {}, s_refLeft[12] = {};
    static ULONGLONG s_refAt = 0;
    static bool s_haveRef = false;
    bool agrees = true, settled = false;
    if (g_haveBarePistolHand && !g_barePistolFitted && d < g_barePistolLearnedFrom - 0.01f) {
        // A pair whose fist sits a centimetre nearer the grip than the kept one's: the kept one was the other
        // pass's (its fist 3.4-3.9 cm off, the holding pass's 1.5-2.3: 19:04, when the first pair taken was the
        // other pass's, every later one 4 cm off it, and the two-handed hold never settled). Taken instead.
        std::memcpy(g_barePistolHand, hold, sizeof(hold));
        Log("Bare hands: the right hand's hold on the pistol taken again from a nearer pair (the game's fist %.1f cm "
            "from its grip, the kept one's %.1f cm)",
            d * 100.0f, g_barePistolLearnedFrom * 100.0f);
        g_barePistolLearnedFrom = d;
        s_haveRef = false;
    }
    if (g_haveBarePistolHand && !g_barePistolFitted) {
        // A later pair: the same pose as the kept one if its fist sits within 1.5 cm of the kept one's on the
        // pistol (each eye draws the arms in two passes: the other's fist is 4.5-5 cm off it).
        float kept[3], now[3];
        TransformPoint(g_barePistolHand, kRightFistRest, kept);
        TransformPoint(hold, kRightFistRest, now);
        agrees = apart(kept, now) < 0.015f;
        if (agrees && s_haveRef && GetTickCount64() - s_refAt >= 150) {
            float ref[3], refLeft[3], nowLeft[3];
            TransformPoint(s_ref, kRightFistRest, ref);
            TransformPoint(s_refLeft, kLeftFistRest, refLeft);
            TransformPoint(leftHold, kLeftFistRest, nowLeft);
            settled = apart(ref, now) < 0.002f && apart(refLeft, nowLeft) < 0.002f;
            s_haveRef = false;  // this pair is the next reference
        }
        static int s_pairs = 0;
        if (s_pairs < 16) {
            ++s_pairs;
            Log("Bare hands: a later pistol pair: the right fist %.1f cm from the grip, the left %.1f cm; it %.1f cm "
                "from the kept hold's",
                d * 100.0f, dl * 100.0f, apart(kept, now) * 100.0f);
        }
    } else {
        std::memcpy(g_barePistolHand, hold, sizeof(hold));
        g_barePistolLearnedAt = GetTickCount64();
        g_barePistolLearnedFrom = d;
        g_haveBarePistolHand = true;
        g_barePistolFitted = false;
        s_haveRef = false;
        Log("Bare hands: the right hand's hold on the pistol learned (the game's fist %.1f cm from its grip; its left "
            "fist %.1f cm)",
            d * 100.0f, dl * 100.0f);
    }
    if (agrees && !s_haveRef) {
        std::memcpy(s_ref, hold, sizeof(hold));
        std::memcpy(s_refLeft, leftHold, sizeof(leftHold));
        s_refAt = GetTickCount64();
        s_haveRef = true;
    }
    // Both hands from this one pair, once the game's left hand is on the pistol and the pose has settled: the
    // right's hold taken again with the left's, so the two hands sit on it as the game holds them together.
    if (g_haveBarePistolLeft || !settled || dl > kLeftFistReach)
        return;
    float palmInverse[12];
    if (!Invert(left, palmInverse))
        return;
    std::memcpy(g_barePistolHand, hold, sizeof(hold));
    g_barePistolLearnedFrom = d;
    std::memcpy(g_barePistolLeft, leftHold, sizeof(leftHold));
    for (int b = 0; b < handpose::kBones; ++b) {  // the gloves' bones the bare hand hasn't (4, 5) left as the palm
        std::memset(g_barePistolLeftFingers[b], 0, sizeof(g_barePistolLeftFingers[b]));
        g_barePistolLeftFingers[b][0] = g_barePistolLeftFingers[b][5] = g_barePistolLeftFingers[b][10] = 1.0f;
    }
    for (int k = kBareSide[0][0]; k <= kBareSide[0][1]; ++k)
        if (kBareGlove[k] >= 0)
            Compose(palmInverse, game + k * 12, g_barePistolLeftFingers[kBareGlove[k]]);
    g_haveBarePistolLeft = true;
    Log("Bare hands: both hands' hold on the pistol learned from one settled pair (the game's right fist %.1f cm from "
        "its grip, the left %.1f cm)",
        d * 100.0f, dl * 100.0f);
}

// ---- The train pistol's manual reload, as the mod draws it (docs\research\pistol-manual-reload.md) ----
// Only while hands.cpp sets HandTargets::pistolReload (build\pistol.txt "reload on", the pistol in the right hand);
// otherwise the pistol is drawn as before. Its own numbers, from the game's own M9 reload (weapon.txt "reloadrec",
// read offline in screenshots\wf_reload), none of the M416's.
// The magazine's seat, bone 0 on the frame bone (3) home: 16 degrees about x, 13.8 cm up the frame (every frame of
// the recordings within 0.17 mm of it, never turned). Its own y is the grip's line, the way in and out. The game's
// own, taken while it's within 2 mm of this (its reload moves the magazine about), replaces it. Under g_handsMutex.
constexpr float kPistolSeat[12] = {1, 0, 0, 0, 0, 0.96137f, -0.27524f, 0.13766f, 0, 0.27524f, 0.96137f, -0.02470f};
float g_pistolSeat[12] = {};
bool g_havePistolSeat = false;
// The magazine's box in bone 0's bind space, where its vertices lie (13.6 cm below and ahead of the gun: read as the
// gun's space, the review found its tumble going round a point 13.8 cm off), and its top middle, the feed lips.
// Seated, the top is 11.08 cm up the grip from its bottom, the well's mouth: kPistolMouth is the top's place along
// the line from home there.
constexpr float kPistolMagMin[3] = {-0.0132f, -0.2041f, 0.0743f}, kPistolMagMax[3] = {0.0130f, -0.0926f, 0.1298f};
constexpr float kPistolMagTop[3] = {-0.0001f, -0.0926f, 0.1001f};
constexpr float kPistolMouth = -0.1108f;
// The slide on the frame: a pure move along the frame's z, never turned (the recordings), home at z -0.29956.
constexpr float kPistolSlideHome = -0.29956f;

// The magazine falling (DropPistolMagazine), as the M416's does (UpdateDrop) but with its own numbers, one bone (0)
// in one pass. Out of the grip it first slides along its line, the well guiding it, gravity's pull down the line
// adding to its start, and doesn't turn (tumbling inside the grip it would go through it); then it falls, tumbles
// end over end about the gun's sideways axis and vanishes as it lands (lying there it would follow the player about,
// as the M416's did: kLieSeconds).
constexpr float kPistolDropSpeed = 0.5f;  // m/s along the line at the start: it drops free by its own weight after
constexpr float kPistolSpin = 2.0f;       // rad/s once out of the grip: it drops rather than being flung
constexpr double kPistolDrawnLately = 0.1, kPistolFallMost = 3.0;  // s
struct PistolMagDrop {
    float grip[12] = {};   // bone 0 as last drawn in the grip, and when (Seconds())
    double gripAt = 0.0;
    float held[12] = {};   // ... as last drawn held (HandTargets::pistolMag), when, and its top's place along the
    double heldAt = 0.0;   //     line from home then (m: 0 seated, kPistolMouth at the mouth)
    float heldAlong = 0.0f;
    Drop fall;             // the fall, kept as the M416's is (MoveBone): fall.start[0] its bone 0 as it came out
    float out[3] = {};     // down its line out of the grip (the arms' space), how much further it has to go along it
    float toClear = 0.0f;  // to clear the grip, and how fast
    float speed = 0.0f;
    double startedAt = 0.0;
};
std::mutex g_pistolDropMutex;
PistolMagDrop g_pistolDrop;
std::atomic<bool> g_pistolDropShown{false};

// The seat in use. Under g_handsMutex.
const float* PistolSeat() {
    return g_havePistolSeat ? g_pistolSeat : kPistolSeat;
}

// The seat as the game draws it (seat: its bone 0 on its frame, before they're moved), taken when it's within 2 mm
// of the recordings' at every corner of the magazine's box: not while the game's own reload moves the magazine.
// Logged once when first taken, and the first time it isn't (how far). Under g_handsMutex.
void LearnPistolSeat(const float* seat) {
    float most = 0.0f;
    for (int c = 0; c < 8; ++c) {
        const float v[3] = {c & 1 ? kPistolMagMax[0] : kPistolMagMin[0], c & 2 ? kPistolMagMax[1] : kPistolMagMin[1],
                            c & 4 ? kPistolMagMax[2] : kPistolMagMin[2]};
        float a[3], b[3];
        TransformPoint(seat, v, a);
        TransformPoint(kPistolSeat, v, b);
        most = std::max(most, Length3(a[0] - b[0], a[1] - b[1], a[2] - b[2]));
    }
    static bool s_took = false, s_off = false;
    if (most < 0.002f) {
        std::memcpy(g_pistolSeat, seat, sizeof(g_pistolSeat));
        g_havePistolSeat = true;
        if (!s_took) {
            s_took = true;
            Log("Pistol reload: the magazine's seat taken from the game's own draw, %.2f mm from the recordings'",
                most * 1000.0f);
        }
    } else if (!s_off) {
        s_off = true;
        Log("Pistol reload: the game draws its magazine %.1f mm from the recordings' seat (its own reload moving it, "
            "or a seat of its own): %s kept",
            most * 1000.0f, g_havePistolSeat ? "the last one taken" : "the recordings'");
    }
}

// The magazine's fall, once a frame (skin::Update), in the arms' space with its origin's jumps taken off (as
// UpdateDrop): along its line until it's out of the grip, then gravity and the tumble down to the floor (its box's
// lowest corner), where it goes; or after 3 s wherever it is.
void UpdatePistolDrop() {
    if (!g_pistolDropShown.load())
        return;
    float origin[3] = {};
    bool haveOrigin = false;
    float floor = g_floor.load();
    {
        std::lock_guard lock(g_fpMutex);
        if (floor == 0.0f && g_haveViewToArms)
            floor = g_viewToArms[7] - kEyeHeight;  // the eye's height in the arms' space
        if (g_haveFpOrigin) {
            std::memcpy(origin, g_fpOrigin, sizeof(origin));
            haveOrigin = true;
        }
    }
    std::lock_guard lock(g_pistolDropMutex);
    if (!g_pistolDropShown.load())
        return;
    PistolMagDrop& d = g_pistolDrop;
    Drop& f = d.fall;
    const double now = Seconds();
    const float dt = static_cast<float>(std::min(now - f.updatedAt, 0.05));
    f.updatedAt = now;
    // The origin's jumps since it came out: what's still in the world is that much less in the arms' space now.
    float jumped[3] = {};
    if (haveOrigin)
        for (int i = 0; i < 3; ++i)
            jumped[i] = origin[i] - f.origin0[i];
    if (d.toClear > 0.0f) {
        d.speed += std::max(-kGravity * d.out[1], 0.0f) * dt;  // gravity's pull down the line (the arms' y is up)
        const float step = std::min(d.speed * dt, d.toClear);
        for (int i = 0; i < 3; ++i)
            f.pos[i] += d.out[i] * step;
        d.toClear -= step;
        if (d.toClear <= 0.0f)
            for (int i = 0; i < 3; ++i)
                f.vel[i] = d.out[i] * d.speed;  // out: it falls on from here
    } else {
        f.vel[1] -= kGravity * dt;
        for (int i = 0; i < 3; ++i)
            f.pos[i] += f.vel[i] * dt;
        f.angle += kPistolSpin * dt;
        AxisAngle(f.axis, f.angle, f.q);
        const float* m = f.start[0];
        float lowest = 1e9f;
        for (int c = 0; c < 8; ++c) {
            const float v[3] = {c & 1 ? kPistolMagMax[0] : kPistolMagMin[0],
                                c & 2 ? kPistolMagMax[1] : kPistolMagMin[1],
                                c & 4 ? kPistolMagMax[2] : kPistolMagMin[2]};
            float p[3];
            for (int i = 0; i < 3; ++i)
                p[i] = m[i * 4] * v[0] + m[i * 4 + 1] * v[1] + m[i * 4 + 2] * v[2] + m[i * 4 + 3] - f.c0[i];
            lowest = std::min(lowest, f.q[3] * p[0] + f.q[4] * p[1] + f.q[5] * p[2] + f.pos[1] - jumped[1]);
        }
        if (lowest <= floor || now - d.startedAt > kPistolFallMost) {
            g_pistolDropShown = false;
            Log("Pistol reload: the dropped magazine %s %.2f s after it came out, %.2f m below where it started",
                lowest <= floor ? "landed" : "went (no floor met)", now - d.startedAt, f.c0[1] - f.pos[1]);
            return;
        }
    }
    for (int i = 0; i < 3; ++i)
        f.centre[i] = f.pos[i] - jumped[i];
}

// The magazine falls from where it was drawn lately (DropPistolMagazine): from the grip, or held on the grip's line,
// out along the line the rest of the way from where it is (the review: from a pose kept from before, it jumped); or
// let go of in the hand at the hand's speed (velocityView, the view's space: turned into the arms'). toClear: how
// far it has to go out of the grip.
bool StartPistolDrop(skin::PistolDrop from, const float* velocityView, float& toClear) {
    float origin[3] = {};
    float toArms[9] = {};  // the view's space -> the arms' (a turn only: for the hand's speed)
    {
        std::lock_guard lock(g_fpMutex);
        std::memcpy(origin, g_fpOrigin, sizeof(origin));
        if (g_haveViewToArms)
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    toArms[i * 3 + j] = g_viewToArms[i * 4 + j];
    }
    std::lock_guard lock(g_pistolDropMutex);
    PistolMagDrop& d = g_pistolDrop;
    const bool grip = from == skin::PistolDrop::kGrip;
    double& at = grip ? d.gripAt : d.heldAt;
    const double now = Seconds();
    if (at <= 0.0 || now - at > kPistolDrawnLately)
        return false;  // not drawn there lately
    at = 0.0;          // taken: not dropped twice
    Drop& f = d.fall;
    std::memcpy(f.start[0], grip ? d.grip : d.held, sizeof(f.start[0]));
    const float* m = f.start[0];
    const float middle[3] = {(kPistolMagMin[0] + kPistolMagMax[0]) / 2, (kPistolMagMin[1] + kPistolMagMax[1]) / 2,
                             (kPistolMagMin[2] + kPistolMagMax[2]) / 2};
    for (int i = 0; i < 3; ++i)
        f.c0[i] = f.pos[i] = f.centre[i] =
            m[i * 4] * middle[0] + m[i * 4 + 1] * middle[1] + m[i * 4 + 2] * middle[2] + m[i * 4 + 3];
    // Out along its own -y (the grip's line), tumbling about its own x (the gun's sideways axis) once out.
    float down[3] = {-m[1], -m[5], -m[9]};
    Normalize(down);
    std::memcpy(d.out, down, sizeof(d.out));
    d.toClear = toClear = grip                              ? -kPistolMouth
                          : from == skin::PistolDrop::kWell ? std::max(d.heldAlong - kPistolMouth, 0.0f)
                                                            : 0.0f;
    d.speed = kPistolDropSpeed;
    for (int i = 0; i < 3; ++i)
        f.vel[i] = from != skin::PistolDrop::kHand ? down[i] * kPistolDropSpeed
                   : velocityView ? toArms[i * 3] * velocityView[0] + toArms[i * 3 + 1] * velocityView[1] +
                                        toArms[i * 3 + 2] * velocityView[2]
                                  : 0.0f;
    f.axis[0] = m[0], f.axis[1] = m[4], f.axis[2] = m[8];
    Normalize(f.axis);
    f.angle = 0.0f;
    AxisAngle(f.axis, 0.0f, f.q);
    std::memcpy(f.origin0, origin, sizeof(origin));
    f.updatedAt = d.startedAt = now;
    g_pistolDropShown = true;
    return true;
}

bool DrawBarePistol(int which, ID3D11DeviceContext* ctx, UINT count, UINT start, INT base) {
    if (!g_bareOn.load(std::memory_order_relaxed) || !g_bareAllowed.load(std::memory_order_relaxed))
        return false;
    const Upload& u = t_upload;
    if (!u.resource || u.bytes.size() != kGloveUploadSize || !PlayersOwn(u.bytes))
        return false;
    // With the manual reload on, the pistol is drawn again with only a held magazine's bone, and only the dropped
    // one's (as DrawGun draws the M416's).
    thread_local std::vector<uint8_t> t_pistol, t_pistolHeld, t_pistolDropped;
    t_pistol = u.bytes;
    auto* bones = reinterpret_cast<float*>(t_pistol.data());
    const float* frame = bones + 3 * 12;
    // What a drop takes from this draw: the magazine as drawn in the grip, or held (and its top's place along the
    // grip's line from home then).
    bool reload = false, inGrip = false, held = false;
    float grip[12] = {}, heldAlong = 0.0f;
    {
        std::lock_guard lock(g_handsMutex);
        if (!g_handsOn)
            return false;
        skin::HandTargets t = g_hands;
        int eye = -1;
        if (!ToArms(t, ctx, &eye) || !t.gun)
            return false;
        // The game's hands on it: learned from an arm draw of the same eye and frame (or the next: this one
        // before an arm draw, DrawBareArms), so both are of one pose; of that eye's two arm passes, the one
        // whose right fist is on the grip.
        const ULONGLONG now = GetTickCount64();
        if (g_recUntil.load(std::memory_order_relaxed))  // weapon.txt "reloadrec": the game's own bones
            Record(0, eye, DrawPass(ctx, eye), u.bytes, 5);
        if (!g_pistolSince || now - g_barePistolAt.load() > 1000)
            g_pistolSince = now;  // just picked up, or back after a while
        const int slot = eye >= 0 && eye <= 1 ? eye + 1 : 0;
        std::memcpy(g_barePistolFrame[slot], frame, sizeof(g_barePistolFrame[slot]));
        g_barePistolFrameNo[slot] = g_frameSerial.load(std::memory_order_relaxed);
        if (PistolLearning()) {
            int best = -1;
            float nearest = kFistReach;
            for (int k = 0; k < 4; ++k)
                if (g_barePalmEye[k] == eye && FramesNear(g_barePalmFrame[k], g_barePistolFrameNo[slot]))
                    if (const float d = FistToGrip(frame, g_barePalmRing[k][kBarePalm[1]]); d < nearest)
                        nearest = d, best = k;
            if (best >= 0)
                LearnPistolHand(frame, g_barePalmRing[best][0]);
        }
        // Nothing learned 5 s after it came into hand: the offline fit's hold (the study's fit_s3: the fist
        // on the grip, the index finger 1.3-1.5 cm from the trigger), so the hand never floats beside it. A
        // pair found later still takes over.
        if (!g_haveBarePistolHand && now - g_pistolSince > 5000) {
            static constexpr float kFitHold[12] = {0.32383f,  -0.93555f, -0.14136f, 1.18636f, 0.71319f,  0.14262f,
                                                   0.68577f,  0.04766f,  -0.62155f, -0.32352f, 0.71374f, -0.06254f};
            std::memcpy(g_barePistolHand, kFitHold, sizeof(kFitHold));
            g_haveBarePistolHand = true;
            g_barePistolFitted = true;
            Log("Bare hands: the pistol's hold not learned in 5 s: the offline fit used (still looked for)");
        }
        g_barePistolAt = now;
        // Its frame bone put where the right controller holds it (hands.cpp pistolPose: aimed as the
        // M416 is, its grip at the hand's grip point), every bone moved as the frame was, so its slide,
        // hammer, trigger and magazine keep the game's animation. Learned from the game's own hand
        // instead, the pairing of the pistol with its arm draw failed or took the other pass's pose: the
        // game's own pistol was left, moving against the hand ("inverted", the user, 15:46 and 15:51).
        float inverse[12], delta[12];
        if (!Invert(frame, inverse))
            return false;
        // The manual reload (HandTargets::pistolReload) starts from the game's own slide and magazine on its own
        // frame, as it animates them, kept before they're moved.
        reload = t.pistolReload;
        float gameSlide[12] = {}, gameMag[12] = {};
        if (reload) {
            Compose(inverse, bones + 4 * 12, gameSlide);
            Compose(inverse, bones, gameMag);
        }
        Compose(t.pistolPose, inverse, delta);
        g_pistolEffect.Note(t, t.pistolSeen, inverse);
        std::memcpy(g_pistolFrameOrig, frame, sizeof(g_pistolFrameOrig));
        g_pistolDeltaTime = now;
        for (int k = 0; k < static_cast<int>(kPaletteBones); ++k)
            MoveBy(delta, bones + k * 12);
        if (reload) {
            LearnPistolSeat(gameMag);
            // The slide where the mod has it (the left hand on it, the slide stop): moved along its own z, the
            // frame's, from where the game's animation has it to that far back from home.
            if (t.pistolSlideBack >= 0.0f) {
                const float gameBack = std::max(kPistolSlideHome - gameSlide[11], 0.0f);
                float* slide = bones + 4 * 12;
                if (const float length = Length3(slide[2], slide[6], slide[10]); length > 0.0f) {
                    const float s = (gameBack - t.pistolSlideBack) / length;
                    slide[3] += slide[2] * s, slide[7] += slide[6] * s, slide[11] += slide[10] * s;
                }
                static bool s_logged = false;
                if (!s_logged) {
                    s_logged = true;
                    Log("Pistol reload: the slide first drawn where the mod has it (%.1f cm back)",
                        t.pistolSlideBack * 100.0f);
                }
            }
            // Its own magazine: in the grip, kept for a drop out of it; out, drawn nowhere (bone 0 all on one point,
            // as the M416's: none of its vertices is on another bone).
            inGrip = !t.pistolMagOut;
            if (inGrip) {
                std::memcpy(grip, bones, sizeof(grip));
            } else {
                std::memset(bones, 0, 12 * sizeof(float));
                static bool s_logged = false;
                if (!s_logged) {
                    s_logged = true;
                    Log("Pistol reload: the pistol first drawn with its magazine out of the grip");
                }
            }
            // A magazine in the left hand or on its way in: the pistol drawn again with only bone 0, at pistolMagPose
            // times the seat, and its top's place along the grip's line from home (0 seated), for a drop out of it.
            if (t.pistolMag) {
                t_pistolHeld.assign(kGloveUploadSize, 0);  // the pistol's other bones all on one point
                auto* mag = reinterpret_cast<float*>(t_pistolHeld.data());
                float home[12], top[3], topHome[3];
                Compose(t.pistolMagPose, PistolSeat(), mag);
                Compose(t.pistolPose, PistolSeat(), home);  // where it would sit in the grip as the pistol is drawn
                TransformPoint(mag, kPistolMagTop, top);
                TransformPoint(home, kPistolMagTop, topHome);
                if (const float length = Length3(home[1], home[5], home[9]); length > 0.0f)  // its own y: the line
                    heldAlong = ((top[0] - topHome[0]) * home[1] + (top[1] - topHome[1]) * home[5] +
                                 (top[2] - topHome[2]) * home[9]) /
                                length;
                held = true;
                static bool s_logged = false;
                if (!s_logged) {
                    s_logged = true;
                    Log("Pistol reload: a magazine first drawn held, %.1f cm along the grip's line from home (the seat "
                        "%s)",
                        heldAlong * 100.0f, g_havePistolSeat ? "taken from the game's draw" : "the recordings'");
                }
            }
        }
    }
    // Kept for a drop (DropPistolMagazine), and the dropped one where it has fallen to (UpdatePistolDrop).
    bool dropped = false;
    if (reload) {
        std::lock_guard lock(g_pistolDropMutex);
        PistolMagDrop& d = g_pistolDrop;
        const double now = Seconds();
        if (inGrip) {
            std::memcpy(d.grip, grip, sizeof(d.grip));
            d.gripAt = now;
        }
        if (held) {
            std::memcpy(d.held, t_pistolHeld.data(), sizeof(d.held));
            d.heldAt = now;
            d.heldAlong = heldAlong;
        }
        dropped = g_pistolDropShown.load();
        if (dropped) {
            t_pistolDropped.assign(kGloveUploadSize, 0);  // the pistol's other bones all on one point
            MoveBone(d.fall.start[0], d.fall, reinterpret_cast<float*>(t_pistolDropped.data()));
        }
    }
    if (!DrawWithPalette(which, ctx, count, start, base, u.resource, t_pistol.data(), kGloveUploadSize))
        return false;
    if (dropped)
        DrawWithPalette(which, ctx, count, start, base, u.resource, t_pistolDropped.data(), kGloveUploadSize);
    if (held)
        DrawWithPalette(which, ctx, count, start, base, u.resource, t_pistolHeld.data(), kGloveUploadSize);
    return true;
}

bool HideBareBody() {
    return GetTickCount64() - g_bareMovedAt.load(std::memory_order_relaxed) < 250 && t_upload.resource &&
           PlayersOwn(t_upload.bytes);
}

// Blackburn's arms whole up to the shoulders (the user, 2026-11-18: the camo sleeves, the shoulders and the upper body, as the levels with gloves have them): the
// torso's draw (5604 indices, both cuff states: barestate::kTorso) and the strip over the shoulders (222), posed on the arms solved from the hands. Each arm's bones
// (upper arm, collar, the sleeve's hem on the forearm's elbow end) go on the frames this view's arms were drawn on just before (g_bareSleeve: the arms' own draw of
// this view and pass, this frame), the chest, belly and hips fold onto the point between the shoulders (barehold::PlaceSleeves, as the gloved chest and sleeves).
// False (the draw is then hidden as before, HideBareBody; or the game's own while the bare arms aren't moved at all: a cutscene, a QTE): "baresleeves 0", no arm of
// this view drawn with its upper arm solved this frame (a hand not tracked, a forearm gone whole with its hand), someone else's bones.
bool DrawBareSleeves(int which, ID3D11DeviceContext* ctx, UINT count, UINT start, INT base, barestate::Part part) {
    const bool strip = part == barestate::kStripPart;
    if (!g_bareSleeves.load(std::memory_order_relaxed) || (strip && !g_bareStrip.load(std::memory_order_relaxed)) || !g_bareOn.load(std::memory_order_relaxed) ||
        !g_bareAllowed.load(std::memory_order_relaxed))
        return false;
    const Upload& u = t_upload;
    if (!u.resource || u.bytes.size() != kGloveUploadSize || !PlayersOwn(u.bytes))
        return false;
    thread_local std::vector<uint8_t> t_sleeves;
    t_sleeves = u.bytes;
    bool on[2] = {};
    {
        std::lock_guard lock(g_handsMutex);
        if (!g_handsOn)
            return false;
        skin::HandTargets t = g_hands;
        int eye = -1;
        if (!ToArms(t, ctx, &eye))
            return false;
        const BareSleeveView& s = g_bareSleeve[DrawPass(ctx, eye)];
        if (!FramesNear(s.frame, g_frameSerial.load(std::memory_order_relaxed)) || !(s.on[0] || s.on[1]))
            return false;
        const float (*const frames[2])[12] = {s.frames[0], s.frames[1]};
        barehold::PlaceSleeves(strip ? barehold::kSleeveStripRoles : barehold::kSleeveTorsoRoles, strip ? barehold::kSleeveStrip : barehold::kSleeveTorso,
                               strip ? barehold::kSleeveStripBones : barehold::kSleeveTorsoBones, s.on, frames, s.chest, reinterpret_cast<float*>(t_sleeves.data()));
        on[0] = s.on[0], on[1] = s.on[1];
    }
    if (!DrawWithPalette(which, ctx, count, start, base, u.resource, t_sleeves.data(), kGloveUploadSize))
        return false;
    if (static std::atomic<ULONGLONG> s_said{0}; !strip && GetTickCount64() - s_said.load(std::memory_order_relaxed) >= 1000) {
        s_said = GetTickCount64();
        const int state = barestate::TorsoState(count, start, base);
        Log("Bare arms: the sleeves on the arms (%s; the torso's %u indices from %u): the left %s, the right %s", state >= 0 ? barestate::kStates[state].name : "?", count,
            start, on[0] ? "on" : "folded away", on[1] ? "on" : "folded away");
    }
    return true;
}

// The ACOG's chevron (the mesh dump of the scope's 12 indices; with "armnudge" to see which it is) is a little shape in the tube, 29 cm ahead of where the game's zoomed eye is: seen from an eye
// a centimetre off the tube's axis, it's 2 degrees off where the bullets go, where a real sight's reticle stays on target wherever the eye is in the tube. In each eye it's drawn on that eye's
// own line along the sights (the rifle's own 0.29 degrees below its z), the same size to the eye: only an eye looking down the tube can see it, and it sees it where the bullets will go. Its
// tip is the point: on the zoomed sights' line in its model to half a millimetre. The user, 2026-11-04: "a little bit of an off aim".
std::wstring BesideDll(const wchar_t* name);

// weapon.txt "armdump": the first palette of each arm draw in a frame (the depth pass's and the colour's of one eye), as sent.
void DumpArmPalette(const DrawKey& key, const void* data, size_t size) {
    static uint32_t s_frame = 0;
    static std::vector<uint32_t> s_done;
    if (!g_armDumpWanted.load(std::memory_order_relaxed))
        return;
    const uint32_t frame = g_frameSerial.load(std::memory_order_relaxed);
    if (!s_frame) {
        s_frame = frame;
        s_done.clear();
    }
    if (frame != s_frame) {
        s_frame = 0;
        g_armDumpWanted = false;
        Log("Arms: palettes written (build\\armdump_*.bin)");
        return;
    }
    if (std::find(s_done.begin(), s_done.end(), key.indexCount) != s_done.end())
        return;
    s_done.push_back(key.indexCount);
    wchar_t name[40];
    swprintf_s(name, L"armdump_%u.bin", key.indexCount);
    if (FILE* f = nullptr; _wfopen_s(&f, BesideDll(name).c_str(), L"wb") == 0 && f) {
        fwrite(data, 1, size, f);
        fclose(f);
    }
}

constexpr DrawKey kScopeReticle{12, 0, 0};
constexpr UINT kScopeGlassStart = 9285;  // the ACOG's glass disc (72 indices); 129/9357 and 108/9486 are its red fibre
uint32_t g_acogLensFrame = 0;            // the frame its lens was last drawn in
constexpr float kReticleTip[3] = {0.0f, 0.0320f, 0.0214f};
constexpr float kSightsFall = 0.005f;        // the sights' line, down per metre along the rifle's z
constexpr float kReticleFromEye = 0.289f;    // how far from the zoomed eye it is in the model: the size it's seen at
constexpr float kReticleMostOff = 0.025f;    // the eye further off than this can't look down the tube: not drawn (nearer, the tube itself hides it as it should)

void PlaceReticle(const skin::HandTargets& t, const float* rel, float* bone) {
    float inverse[12], eye[3], tip[3];
    if (!Invert(t.gunPose, inverse)) {
        Compose(t.gunPose, rel, bone);
        return;
    }
    TransformPoint(inverse, t.eyeInArms, eye);
    TransformPoint(rel, kReticleTip, tip);
    const float along = tip[2] - eye[2];
    const float onLine[3] = {eye[0], eye[1] - kSightsFall * along, tip[2]};
    if (along < 0.03f || Length3(onLine[0] - tip[0], onLine[1] - tip[1], 0.0f) > kReticleMostOff) {
        float at[3];
        TransformPoint(t.gunPose, tip, at);
        Collapse(bone, at);
        return;
    }
    const float size = along / kReticleFromEye;
    float moved[12];
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c)
            moved[r * 4 + c] = rel[r * 4 + c] * size;
        moved[r * 4 + 3] = onLine[r] - (moved[r * 4] * kReticleTip[0] + moved[r * 4 + 1] * kReticleTip[1] + moved[r * 4 + 2] * kReticleTip[2]);
    }
    Compose(t.gunPose, moved, bone);
}

// A point of the arms' space in this view's pixels. False behind the eye.
bool ToPixels(const float* vp, const D3D11_VIEWPORT& view, const float* p, float* pixel) {
    const float x = vp[0] * p[0] + vp[1] * p[1] + vp[2] * p[2] + vp[3];
    const float y = vp[4] * p[0] + vp[5] * p[1] + vp[6] * p[2] + vp[7];
    const float w = vp[12] * p[0] + vp[13] * p[1] + vp[14] * p[2] + vp[15];
    if (w < 0.01f)
        return false;
    pixel[0] = view.TopLeftX + (x / w * 0.5f + 0.5f) * view.Width;
    pixel[1] = view.TopLeftY + (0.5f - y / w * 0.5f) * view.Height;
    return true;
}

// The scope's body drawn with a window cut in it, where its line goes, so the world behind it is in the picture for the lens to read. Only with the eye nearly behind the eyepiece, where the
// lens itself then covers the window; with the eye further off the lens simply shows what's there, the scope's own dark inside, as a real one goes dark. window: left, top, right, bottom, pixels.
bool ScopeWindow(ID3D11DeviceContext* ctx, const float* vp, const float* eye, const float* gun, const float* through, float fall, float radius, float power, LONG* window) {
    D3D11_VIEWPORT view{};
    UINT viewports = 1;
    ctx->RSGetViewports(&viewports, &view);
    if (viewports != 1)
        return false;
    float distant[3], edge[3], atLens[2], atFar[2], atEdge[2];
    for (int i = 0; i < 3; ++i) {
        distant[i] = eye[i] + 1000.0f * (gun[i * 4 + 2] - fall * gun[i * 4 + 1]);
        edge[i] = through[i] + radius * gun[i * 4 + 1];
    }
    if (!ToPixels(vp, view, through, atLens) || !ToPixels(vp, view, distant, atFar) || !ToPixels(vp, view, edge, atEdge))
        return false;
    const float lensRadius = Length3(atEdge[0] - atLens[0], atEdge[1] - atLens[1], 0.0f);
    const float off = Length3(atFar[0] - atLens[0], atFar[1] - atLens[1], 0.0f);
    // What the lens reads: the lens's own disc, shrunk by the power towards where the line goes: about far + (lens - far) / power, lens radius / power across (scopezoom.cpp: each pixel, its slope from the line divided by the power).
    const float half = lensRadius / power + 5.0f;
    const float cx = atFar[0] + (atLens[0] - atFar[0]) / power, cy = atFar[1] + (atLens[1] - atFar[1]) / power;
    float box[4] = {cx - half, cy - half, cx + half, cy + half};
    // Well inside the disc: its rim is a little nearer the eye, and covers its edge from off to one side. With the eye too far off for all of it, as much as is under the disc: the lens's far side then shows the scope's own inside, dark, as a real one does; it doesn't black out whole (the user, 2026-11-05: "I kind of don't like how it blacks out").
    if (off * (1.0f - 1.0f / power) + half * 1.42f > lensRadius * 0.9f) {
        const float inner = lensRadius * 0.9f * 0.7071f;
        box[0] = std::fmax(box[0], atLens[0] - inner), box[1] = std::fmax(box[1], atLens[1] - inner);
        box[2] = std::fmin(box[2], atLens[0] + inner), box[3] = std::fmin(box[3], atLens[1] + inner);
        if (box[0] >= box[2] || box[1] >= box[3])
            return false;
    }
    for (int i = 0; i < 4; ++i)
        window[i] = static_cast<LONG>(box[i]);
    return true;
}

// The game's rasterizer state with the scissor on: one kept for each.
ID3D11RasterizerState* WithScissor(ID3D11DeviceContext* ctx, ID3D11RasterizerState* state) {
    static std::mutex s_mutex;
    static std::unordered_map<ID3D11RasterizerState*, ID3D11RasterizerState*> s_made;
    std::lock_guard lock(s_mutex);
    if (const auto it = s_made.find(state); it != s_made.end())
        return it->second;
    D3D11_RASTERIZER_DESC desc{};
    if (state) {
        state->GetDesc(&desc);
    } else {
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_BACK;
        desc.DepthClipEnable = TRUE;
    }
    desc.ScissorEnable = TRUE;
    ID3D11Device* device = nullptr;
    ctx->GetDevice(&device);
    ID3D11RasterizerState* made = nullptr;
    if (device) {
        device->CreateRasterizerState(&desc, &made);
        device->Release();
    }
    s_made[state] = made;
    return made;
}

// The game's rasterizer state not cutting off what's nearer than its near plane, 6 cm in front of the eye: the sights are brought closer than that, and the stock and the receiver pass right under the eye. One kept for each.
ID3D11RasterizerState* WithoutNearCut(ID3D11DeviceContext* ctx, ID3D11RasterizerState* state) {
    static std::mutex s_mutex;  // (drawn from any recording thread)
    static std::unordered_map<ID3D11RasterizerState*, ID3D11RasterizerState*> s_made;
    std::lock_guard lock(s_mutex);
    if (const auto it = s_made.find(state); it != s_made.end())
        return it->second;
    D3D11_RASTERIZER_DESC desc{};
    if (state) {
        state->GetDesc(&desc);
    } else {
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_BACK;
    }
    desc.DepthClipEnable = FALSE;
    ID3D11Device* device = nullptr;
    ctx->GetDevice(&device);
    ID3D11RasterizerState* made = nullptr;
    if (device) {
        device->CreateRasterizerState(&desc, &made);
        device->Release();
    }
    s_made[state] = made;
    return made;
}

// For as long as it lasts.
class NoNearCut {
public:
    NoNearCut(ID3D11DeviceContext* ctx, bool wanted) : ctx_(ctx) {
        if (!wanted)
            return;
        ctx_->RSGetState(&kept_);
        if (ID3D11RasterizerState* made = WithoutNearCut(ctx_, kept_)) {
            ctx_->RSSetState(made);
            on_ = true;
        }
    }
    ~NoNearCut() {
        if (on_)
            ctx_->RSSetState(kept_);
        if (kept_)
            kept_->Release();
    }

private:
    ID3D11DeviceContext* ctx_;
    ID3D11RasterizerState* kept_ = nullptr;
    bool on_ = false;
};

// The game's rasterizer state with front and back the other way round: for a mesh drawn mirrored. One kept for each.
ID3D11RasterizerState* FacesTurned(ID3D11DeviceContext* ctx, ID3D11RasterizerState* state) {
    static std::mutex s_mutex;
    static std::unordered_map<ID3D11RasterizerState*, ID3D11RasterizerState*> s_made;
    std::lock_guard lock(s_mutex);
    if (const auto it = s_made.find(state); it != s_made.end())
        return it->second;
    D3D11_RASTERIZER_DESC desc{};
    if (state) {
        state->GetDesc(&desc);
    } else {
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_BACK;
        desc.DepthClipEnable = TRUE;
    }
    desc.FrontCounterClockwise = !desc.FrontCounterClockwise;
    ID3D11Device* device = nullptr;
    ctx->GetDevice(&device);
    ID3D11RasterizerState* made = nullptr;
    if (device) {
        device->CreateRasterizerState(&desc, &made);
        device->Release();
    }
    s_made[state] = made;
    return made;
}

// For as long as it lasts.
class MirroredFaces {
public:
    explicit MirroredFaces(ID3D11DeviceContext* ctx) : ctx_(ctx) {
        ctx_->RSGetState(&kept_);
        if (ID3D11RasterizerState* made = FacesTurned(ctx_, kept_)) {
            ctx_->RSSetState(made);
            on_ = true;
        }
    }
    ~MirroredFaces() {
        if (on_)
            ctx_->RSSetState(kept_);
        if (kept_)
            kept_->Release();
    }

private:
    ID3D11DeviceContext* ctx_;
    ID3D11RasterizerState* kept_ = nullptr;
    bool on_ = false;
};

// The game's rasterizer state drawing both faces of every triangle: a gun lying (freehands.txt "twosided 1"), whose first-person mesh may lack the sides first
// person never shows. One kept for each.
ID3D11RasterizerState* BothFaces(ID3D11DeviceContext* ctx, ID3D11RasterizerState* state) {
    static std::mutex s_mutex;
    static std::unordered_map<ID3D11RasterizerState*, ID3D11RasterizerState*> s_made;
    std::lock_guard lock(s_mutex);
    if (const auto it = s_made.find(state); it != s_made.end())
        return it->second;
    D3D11_RASTERIZER_DESC desc{};
    if (state) {
        state->GetDesc(&desc);
    } else {
        desc.FillMode = D3D11_FILL_SOLID;
        desc.DepthClipEnable = TRUE;
    }
    desc.CullMode = D3D11_CULL_NONE;
    ID3D11Device* device = nullptr;
    ctx->GetDevice(&device);
    ID3D11RasterizerState* made = nullptr;
    if (device) {
        device->CreateRasterizerState(&desc, &made);
        device->Release();
    }
    s_made[state] = made;
    return made;
}

// For as long as it lasts, when wanted.
class NoCull {
public:
    NoCull(ID3D11DeviceContext* ctx, bool wanted) : ctx_(ctx) {
        if (!wanted)
            return;
        ctx_->RSGetState(&kept_);
        if (ID3D11RasterizerState* made = BothFaces(ctx_, kept_)) {
            ctx_->RSSetState(made);
            on_ = true;
        }
    }
    ~NoCull() {
        if (on_)
            ctx_->RSSetState(kept_);
        if (kept_)
            kept_->Release();
    }

private:
    ID3D11DeviceContext* ctx_;
    ID3D11RasterizerState* kept_ = nullptr;
    bool on_ = false;
};

// A scope's body drawn round its window (ScopeWindow): the four strips about it.
bool DrawWithWindow(int which, ID3D11DeviceContext* ctx, UINT count, UINT start, INT base, ID3D11Resource* original, const uint8_t* bytes, const LONG* window) {
    ID3D11RasterizerState* state = nullptr;
    ctx->RSGetState(&state);
    ID3D11RasterizerState* scissored = WithScissor(ctx, state);
    bool all = false;
    if (scissored) {
        UINT rectCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        D3D11_RECT kept[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
        ctx->RSGetScissorRects(&rectCount, kept);
        ctx->RSSetState(scissored);
        constexpr LONG kAll = 16384;
        const D3D11_RECT round[4] = {{0, 0, kAll, window[1]}, {0, window[3], kAll, kAll}, {0, window[1], window[0], window[3]}, {window[2], window[1], kAll, window[3]}};
        all = true;
        t_noHole = true;
        for (const D3D11_RECT& rect : round) {
            ctx->RSSetScissorRects(1, &rect);
            all = DrawWithPalette(which, ctx, count, start, base, original, bytes, kGloveUploadSize) && all;
        }
        t_noHole = false;
        ctx->RSSetScissorRects(rectCount, kept);
        ctx->RSSetState(state);
    }
    if (state)
        state->Release();
    return all;
}

// The window this eye's lens has, for all that's in the hands (LensHole).
void NoteLensHole(int eye, const LONG* window) {
    if (eye < 0 || eye > 1)
        return;
    const uint32_t frame = g_frameSerial.load(std::memory_order_relaxed);
    g_lensHole[eye].frame = frame;
    std::memcpy(g_lensHole[eye].rect, window, sizeof(g_lensHole[eye].rect));
    g_lensHoleFrame = frame;
}

// weapon.txt "glassdump N": how each of a scope's see-through draws is drawn, once for each N (zoomed and not), and its textures into build\glass.
bool NoteGlass(ID3D11DeviceContext* ctx, const DrawKey& key) {
    const int trigger = g_glassDump.load(std::memory_order_relaxed);
    if (!trigger)
        return false;
    {
        static std::mutex s_mutex;
        static std::vector<std::pair<int, DrawKey>> s_done;
        std::lock_guard lock(s_mutex);
        for (const auto& d : s_done)
            if (d.first == trigger && d.second == key)
                return false;
        s_done.push_back({trigger, key});
    }
    char name[64];
    std::snprintf(name, sizeof(name), "%d_%u_%u", trigger, key.indexCount, key.startIndex);
    LogScopeMaterial(ctx, key);
    scopezoom::NoteMaterial(ctx, name);
    return true;
}

// This draw's view: its view-projection of the arms' space, and which eye. False if its constant buffer isn't known.
bool ViewOfDraw(ID3D11DeviceContext* ctx, float* vp, int& eye, float* viewToArms) {
    ID3D11Buffer* bound[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = {};
    ctx->VSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, bound);
    bool found = false;
    {
        std::lock_guard lock(g_fpMutex);
        for (ID3D11Buffer* b : bound)
            for (const ViewByBuffer& view : g_viewByBuffer)
                if (b && !found && view.buffer == b) {
                    found = true;
                    eye = view.eye;
                    std::memcpy(vp, view.vp, 16 * sizeof(float));
                    if (viewToArms)
                        std::memcpy(viewToArms, view.viewToArms, sizeof(view.viewToArms));
                }
    }
    for (ID3D11Buffer* b : bound)
        if (b)
            b->Release();
    // As it reached the graphics card: it's kept as the game uploaded it, before RescaleProjections widens it to the picture's field of view. At the game's 55 degrees everything worked out from it, what a lens's
    // pixel looks at above all, was 2.29 times too narrow.
    if (const float wide = g_wideScale.load(std::memory_order_relaxed); found && wide > 0.0f) {
        const float scale = Length3(vp[4], vp[5], vp[6]);
        if (scale > 0.01f)
            for (int c = 0; c < 4; ++c)
                vp[c] *= wide / scale, vp[4 + c] *= wide / scale;
    }
    return found;
}

// The lens as this draw's eye sees it, once for each eye in a frame. gun: the arms' space; through: the eyepiece's middle there; fall: how much the sights' line falls along the gun.
bool ScopeLens(ID3D11DeviceContext* ctx, const skin::HandTargets& t, const float* gun, const float* through, float fall, float radius, float power, scopezoom::Lens& lens, bool once = true) {
    int eye = -1;
    if (!ViewOfDraw(ctx, lens.vp, eye, lens.viewToArms))
        return false;
    static uint32_t s_done[3] = {0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu};
    const uint32_t frame = g_frameSerial.load(std::memory_order_relaxed);
    if (once && s_done[eye + 1] == frame)
        return false;
    s_done[eye + 1] = frame;
    lens.eyeIndex = eye;
    lens.haveView = eye == 0 || eye == 1;
    float z[3];
    for (int i = 0; i < 3; ++i)
        z[i] = gun[i * 4 + 2] - fall * gun[i * 4 + 1];
    const float length = Length3(z[0], z[1], z[2]);
    for (int i = 0; i < 3; ++i) {
        lens.scope[i * 4] = gun[i * 4];
        lens.scope[i * 4 + 1] = gun[i * 4 + 1];
        lens.scope[i * 4 + 2] = z[i] / length;
        lens.scope[i * 4 + 3] = through[i];
        lens.eye[i] = t.eyeInArms[i];
        lens.middle[i] = 0.0f;
    }
    lens.radius = radius;
    lens.power = power;
    return true;
}

// One of the first-person arms' draws with free hands on: each hand's bones moved with it, the
// arms, sleeves and body folded away. False: draw as usual. (Other soldiers' copies of the scope
// share its draws, their bones far out in the world: those are left as the game draws them.)
bool DrawArms(int which, ID3D11DeviceContext* ctx, UINT count, UINT start, INT base, const ArmDraw& draw) {
    const Upload& u = t_upload;
    // The gun the mod knows isn't in hand. (Its parts only ever come with it: in the frame it comes back, its first depth pass is before its colour says so.) Drawn
    // nowhere (stowed) it still is, by its stamps: the rifle's here, a launcher's or another gun's in DrawLauncherRig. With none of them drawn lately the game's own
    // arms, put away or not (a cutscene's, a knife's, a gun the hands don't know: as always).
    if (draw.roles[0] != 'G' && !DrawnLately(g_rifleDrawn, g_rifleFrame))
        return false;
    const bool tracing = g_scopeTraceFrames.load(std::memory_order_relaxed) > 0;
    auto trace = [&](const char* outcome) {
        if (tracing)
            TraceScopeDraw(ctx, draw, u, outcome);
    };
    if (!u.resource || u.bytes.size() != kGloveUploadSize || !PlayersOwn(u.bytes)) {
        NoteUnmoved(draw.key, !u.resource || u.bytes.size() != kGloveUploadSize ? 0 : 1);
        trace(!u.resource || u.bytes.size() != kGloveUploadSize ? "left in place: no whole bones"
                                                                : "left in place: someone else's bones");
        return false;
    }
    bool zoomWanted = false, windowWanted = false;
    LONG window[4] = {};
    scopezoom::Lens zoomLens;
    thread_local std::vector<uint8_t> t_arms;
    t_arms = u.bytes;
    auto* bones = reinterpret_cast<float*>(t_arms.data());
    bool placed = false;  // a gun part's bone put with the gun
    bool rightPlaced = false;  // ... a right glove's bone put on the gun's grip as the game holds it (skin::kRightOnGrip)
    WorldPlacing placing;  // this draw's, for the launcher's cases lying in the world
    bool m320HandRound = false;  // the grenade launcher's round in the hand (its reload by hand), its bone as this draw places it
    float m320HandRoundPose[12] = {};
    // What rides the grenade launcher's round's draws and isn't the gun, after the part itself.
    auto drawRounds = [&] {
        // A round in the hand (the reload by hand): the round's mesh again, its bone where the hand has it, every other bone on one point. Never cut by the scope's
        // window (it's not the gun's: held between the eye and the eyepiece, a square of it went, the review 2026-11-05), nor the cases below.
        t_noHole = true;
        if (const int roundBone = draw.key == DrawKey{1440, 7848, 0} ? 0 : draw.key == DrawKey{9288, 9288, 0} ? 2 : -1; roundBone >= 0 && m320HandRound) {
            thread_local std::vector<uint8_t> t_inHand;
            t_inHand.assign(kGloveUploadSize, 0);
            std::memcpy(reinterpret_cast<float*>(t_inHand.data()) + roundBone * 12, m320HandRoundPose, sizeof(m320HandRoundPose));
            DrawWithPalette(which, ctx, count, start, base, u.resource, t_inHand.data(), kGloveUploadSize);
        }
        // The launcher's spent cases lying or falling (loose): the round's mesh, each its own copy of this draw, its bone where the case is, every other bone on one point.
        if (const int roundBone = draw.key == DrawKey{1440, 7848, 0} ? 0 : draw.key == DrawKey{9288, 9288, 0} ? 2 : -1; roundBone >= 0 && placing.have) {
            thread_local std::vector<uint8_t> t_case;
            std::lock_guard lock(g_looseMutex);
            for (int p = 0; p < g_loosePoseCount; ++p) {
                const loose::Pose& pose = g_loosePoses[p];
                if (pose.kind != kLooseM320Case)
                    continue;
                float world[12];
                Compose(pose.bone, kM320RoundToCase, world);
                t_case.assign(kGloveUploadSize, 0);
                WorldToDrawn(placing, world, g_looseOrigin, reinterpret_cast<float*>(t_case.data()) + roundBone * 12);
                if (DrawWithPalette(which, ctx, count, start, base, u.resource, t_case.data(), kGloveUploadSize))
                    g_looseDraws.fetch_add(1, std::memory_order_relaxed);
            }
        }
        t_noHole = false;
    };
    {
        std::unique_lock lock(g_handsMutex);
        if (!g_handsOn)
            return false;
        skin::HandTargets t = g_hands;
        if (!ToArms(t, ctx))
            return draw.roles[0] == 'G' && t.stowed;  // (a part of the gun put away is drawn nowhere, even with no view known yet)
        // The rifle lying where it fell: its parts, like its body (DrawGun), are placed from the world, so not drawn by a draw without the world's way into it.
        if (draw.roles[0] == 'G' && !t.gun && t.gunLying && !t.haveDrawnFromWorld)
            return true;
        placing.Take(t);
        if ((m320HandRound = t.m320HandRound))
            std::memcpy(m320HandRoundPose, t.m320HandRoundPose, sizeof(m320HandRoundPose));
        // The gun put away, or its hand not seen at a place (stowed): its parts drawn nowhere, as its body (DrawGun), never where the game holds it (the ACOG and
        // the M320 in front of the eyes); nor placed, so nothing of its sight is noted (no lens, no window cut: the scope's own picture goes off by itself). What
        // rides these draws and isn't the gun is drawn as before.
        if (draw.roles[0] == 'G' && t.stowed) {
            lock.unlock();
            g_stowedSkips[kStowedParts].fetch_add(1, std::memory_order_relaxed);
            drawRounds();
            trace("not drawn: the gun put away");
            return true;
        }
        // Offsets learned from the game's gun body drawn earlier in this view, or once it's drawn
        // (the bones as the game sent them, before they're moved below).
        const uint32_t view = g_viewUploads.load();
        if (float gunInverse[12]; g_haveGunBodyOrig && g_gunBodyView == view && Invert(g_gunBodyOrig, gunInverse))
            LearnArms(draw, bones, gunInverse, false);
        else
            KeepForLater(draw, bones, view);
        Learned& learned = g_learned[draw.key];
        // Where each side's arm folds: the middle of its forearm, inside the forearm drawn whole
        // (kForearmMiddle); the hand's palm until that's known.
        float fold[2][3];
        std::memcpy(fold[0], t.leftWrist, sizeof(fold[0]));
        std::memcpy(fold[1], t.rightWrist, sizeof(fold[1]));
        float forearm[2][12];
        bool haveForearm[2] = {t.left, false};
        if (t.left)
            Compose(t.leftPose, t.leftFingers[5], forearm[0]);  // as the bare forearm's elbow half is (kForearmLeft)
        if (auto it = g_learned.find(kBareForearms); t.rightGlove == skin::kRightOnGrip && it != g_learned.end() && it->second.have[12]) {
            Compose(t.rightGunPose, it->second.rel[12], forearm[1]);  // its middle bone, as placed below
            haveForearm[1] = true;
        } else if (t.rightGlove == skin::kRightOwn) {
            // The right glove by its own pose: as the left's, its forearm's elbow half (kForearmRight: 44, the twin of the left's 5 and 4).
            Compose(t.rightGlovePose, t.rightFingers[44 - handpose::kRightFirst], forearm[1]);
            haveForearm[1] = true;
        }
        for (int side = 0; side < 2; ++side)
            if (haveForearm[side])
                for (int i = 0; i < 3; ++i)
                    fold[side][i] = forearm[side][i * 4] * kForearmMiddle[side][0] +
                                    forearm[side][i * 4 + 1] * kForearmMiddle[side][1] +
                                    forearm[side][i * 4 + 2] * kForearmMiddle[side][2] + forearm[side][i * 4 + 3];
        // A forearm solved from its hand: the same point, the middle of the forearm as it's drawn now
        // (placed with its elbow end).
        for (int side = 0; side < 2; ++side)
            if (t.arm[side])
                std::memcpy(fold[side], t.fold[side], sizeof(fold[side]));
        // The body and legs fold onto the line between the two arms' fold points, so every triangle
        // mixing them ('c', 'l' and 'r': 15 on the collar) stays without area (the review, 2026-10-31).
        // (The right glove folded away, skin::kRightHidden, isn't an arm's fold point: it isn't seen.)
        const bool rightHere = t.rightGlove != skin::kRightGame && t.rightGlove != skin::kRightHidden;
        float mid[3];
        for (int i = 0; i < 3; ++i)
            mid[i] = t.left && rightHere ? (fold[0][i] + fold[1][i]) / 2 : t.left ? fold[0][i] : fold[1][i];
        // Each arm solved: whole, up to its shoulder, sleeve and all, the chest and legs folding between the shoulders (a triangle mixing them goes up to there, behind the eye).
        if (t.haveChest && (t.arm[0] || t.arm[1]))
            std::memcpy(mid, t.chest, sizeof(mid));
        // A hand that isn't seen and is folded away (the right glove kRightHidden; the game's left hand over the rifle lying or put away, the left not seen) folds
        // where the body does: its grip point then is none (the eye), and a triangle mixing it with the body and a seen arm would run through the eye.
        if (t.rightGlove == skin::kRightHidden)
            std::memcpy(fold[1], mid, sizeof(fold[1]));
        if (!t.left && (t.gunLying || t.stowed))
            std::memcpy(fold[0], mid, sizeof(fold[0]));
        for (size_t k = 0; draw.roles[k] && k < kPaletteBones; ++k) {
            float* bone = bones + k * 12;
            const int armPart = draw.forearm && k < draw.forearmCount ? draw.forearm[k] : -1;
            switch (draw.roles[k]) {
            case 'L':
                // The whole hand placed as one, each bone bent as its finger is held. (Blending the
                // matrices towards the game's own grip and past it folded the hand flat: user,
                // 2026-10-30.) With its forearm solved (build\arms.txt "on"), the forearm's bones and
                // the glove's cuff on that instead, and the bare forearm's wrist end with the glove's
                // bones it shares vertices with.
                if (t.left) {
                    const int f = t.arm[0] && draw.forearm && k < draw.forearmCount ? draw.forearm[k] : -1;
                    if (f >= 0 && f <= 4) {
                        std::memcpy(bone, t.forearm[0][f], sizeof(t.forearm[0][f]));
                    } else if (f >= 10 && f < 10 + handpose::kBones) {
                        Compose(t.leftPose, t.leftFingers[f - 10], bone);
                    } else {
                        int g = !draw.leftBones ? static_cast<int>(k) : k < draw.leftBoneCount ? draw.leftBones[k] : -1;
                        Compose(t.leftPose, t.leftFingers[g >= 0 && g < handpose::kBones ? g : 0], bone);
                    }
                } else if (t.gunLying || t.stowed) {
                    Collapse(bone, fold[0]);  // no game left hand at its hold over the rifle lying on the ground, or put away (drawn nowhere)
                }
                break;
            case 'R':
                // The right glove (skin::RightGlove).
                switch (t.rightGlove) {
                case skin::kRightOnGrip:
                    // On the gun's grip as the game holds it, each pass by its own learned offsets (its colour and depth always agree). The right forearm,
                    // solved from the hand: placed by that, the hand and fingers on the gun as the game holds them.
                    if (t.arm[1] && armPart >= 0 && armPart <= 4) {
                        std::memcpy(bone, t.forearm[1][armPart], sizeof(t.forearm[1][armPart]));
                        placed = rightPlaced = true;
                    } else if (learned.have[k]) {
                        Compose(t.rightGunPose, learned.rel[k], bone);
                        placed = rightPlaced = true;
                    }
                    break;
                case skin::kRightOwn:
                    // By its own pose, each bone bent as its fingers are held (the left's way: its bone in this pass, RightGloveBone); the forearm solved from the hand
                    // as on the grip.
                    if (t.arm[1] && armPart >= 0 && armPart <= 4)
                        std::memcpy(bone, t.forearm[1][armPart], sizeof(t.forearm[1][armPart]));
                    else
                        Compose(t.rightGlovePose, t.rightFingers[RightGloveBone(draw, k) - handpose::kRightFirst], bone);
                    placed = true;
                    break;
                case skin::kRightHidden:
                    Collapse(bone, fold[1]);  // not seen while the rifle is drawn elsewhere: folded away
                    break;
                default:  // kRightGame: as the game sent it
                    break;
                }
                break;
            case 'G':  // a part of the gun: where its learned offset from the gun's body puts it
                if (t.gun || t.gunDrawn) {  // in a hand, or with none on it (flying into one, lying)
                    // Placed as another draw is (the scope's parts as its body): that draw's offsets,
                    // learned every frame. The see-through parts are only drawn in a later pass, after
                    // the gun's body: they never learned their own and stayed where the game put them,
                    // floating in the sky (the scope trace, 2026-10-31 02:30).
                    const Learned* rel = &learned;
                    if (draw.placedLike.indexCount)
                        if (auto it = g_learned.find(draw.placedLike); it != g_learned.end() && it->second.have[k])
                            rel = &it->second;
                    if (rel->have[k]) {
                        if (k == 0 && draw.key == kScopeReticle && g_reticleFar.load(std::memory_order_relaxed)) {
                            PlaceReticle(t, rel->rel[k], bone);
                        } else if (const int part = M320PartOf(draw.key, k)) {
                            // The grenade launcher's tube and round: swung open about the hinge, the round slid out or gone (its reload by hand). The round as the game has it
                            // kept for the watch of its own reload (its own mesh's draw first: the depth pass's shares it).
                            if (part == 2 && (draw.key == DrawKey{1440, 7848, 0} || GetTickCount64() - g_m320RoundSeenAt > 100)) {
                                std::memcpy(g_m320RoundSeen, rel->rel[k], sizeof(g_m320RoundSeen));
                                g_m320RoundSeenAt = GetTickCount64();
                            }
                            if (float now[12]; M320PartNow(t, part, rel->rel[k], now))
                                Compose(t.gunPose, now, bone);
                            else
                                std::memset(bone, 0, 12 * sizeof(float));  // (all its vertices on one point)
                        } else {
                            Compose(t.gunPose, rel->rel[k], bone);
                        }
                        placed = true;
                    }
                }
                break;
            case 'l':
                if (t.arm[0] && armPart >= 0 && armPart <= 4)
                    std::memcpy(bone, t.forearm[0][armPart], sizeof(t.forearm[0][armPart]));
                else if (t.left || t.gunLying || t.stowed)
                    Collapse(bone, fold[0]);
                break;
            case 'r':
                if (t.arm[1] && armPart >= 0 && armPart <= 4)
                    std::memcpy(bone, t.forearm[1][armPart], sizeof(t.forearm[1][armPart]));
                else if (t.rightGlove != skin::kRightGame)
                    Collapse(bone, fold[1]);
                break;
            case 'c':
                Collapse(bone, mid);
                break;
            }
        }
        if (const int nudge = g_armNudge.load(std::memory_order_relaxed); nudge >= 0 && nudge < static_cast<int>(kPaletteBones) && static_cast<int>(draw.key.indexCount) == g_armNudgeCount.load(std::memory_order_relaxed))
            bones[nudge * 12 + 7] += 0.3f;
        if (static ULONGLONG s_bonesAt = 0; g_armBonesLog.load(std::memory_order_relaxed) && draw.key == kBareForearms && GetTickCount64() - s_bonesAt >= 2000) {
            s_bonesAt = GetTickCount64();

            for (size_t k = 0; draw.roles[k] && k < kPaletteBones; ++k) {
                const float* b = bones + k * 12;
                const float* mid3 = kForearmMiddle[k < 10 ? 0 : 1];
                float at[3];
                TransformPoint(b, mid3, at);
                const float* raw = reinterpret_cast<const float*>(u.bytes.data()) + k * 12;
                Log("Arm bones: %u: role %c map %d: the forearm's middle through it at (%.3f %.3f %.3f); its row lengths %.3f %.3f %.3f; as sent, moved (%.3f %.3f %.3f)", static_cast<unsigned>(k), draw.roles[k],
                    draw.forearm && k < draw.forearmCount ? draw.forearm[k] : -1, at[0], at[1], at[2], Length3(b[0], b[1], b[2]), Length3(b[4], b[5], b[6]), Length3(b[8], b[9], b[10]), raw[3], raw[7], raw[11]);
            }
        }
        // The trigger finger pulled with the controller's trigger (the user, 2026-10-31): the game's
        // own grip curls it further, alike in the gloves' colour and both depth passes (a pass that
        // didn't bend it would show the finger unbent).
        if (const float pull = input::RightTrigger(); t.rightGlove == skin::kRightOnGrip && rightPlaced && pull > 0.01f) {
            auto* palette = reinterpret_cast<float(*)[12]>(bones);
            const float degrees = pull * kTriggerBend;
            if (&draw == &kArmDraws[0])
                handpose::BendRightIndex(palette, degrees);
            else if (&draw == &kArmDraws[1])
                handpose::BendRightIndex(palette, degrees, handpose::kGloveDepthRight, std::size(handpose::kGloveDepthRight));
            else if (&draw == &kArmDraws[4])
                handpose::BendRightIndex(palette, degrees, handpose::kBodyDepthRight, std::size(handpose::kBodyDepthRight));
        }
        // The ACOG on the rifle in a hand this frame, for recoil's sight at the eye (skin::Sight: its glass from the learned body, as the lens below).
        if (t.gun && placed && draw.key == kScopeBody) {
            g_acogFrame = g_frameSerial.load(std::memory_order_relaxed);
            g_haveAcogFrame = true;
        }
        // The rifle's scope magnifies, only inside it: the game's own glass disc shows the world seen through it nearer (scopezoom.h).
        if (const float power = g_scopePower.load(std::memory_order_relaxed);
            power > 1.0f && t.gun && placed && draw.placedLike == kScopeBody && draw.key.startIndex == kScopeGlassStart && DrawnLately(g_rifleDrawn, g_rifleFrame) && scopezoom::InScene(ctx)) {
            if (const auto it = g_learned.find(kScopeBody); it != g_learned.end() && it->second.have[0]) {
                float at[3], onGun[3];
                TransformPoint(it->second.rel[0], g_acogEyepiece, onGun);
                TransformPoint(t.gunPose, onGun, at);
                zoomWanted = ScopeLens(ctx, t, t.gunPose, at, kSightsFall, g_acogEyepieceRadius, power, zoomLens);
                zoomLens.marks = &scopemarks::kAcog;
                if (g_glassDump.load(std::memory_order_relaxed)) {
                    float toGun[12], toScope[12], eyeOnGun[3], eyeInScope[3];
                    if (Invert(t.gunPose, toGun) && Invert(it->second.rel[0], toScope)) {
                        TransformPoint(toGun, t.eyeInArms, eyeOnGun);
                        TransformPoint(toScope, eyeOnGun, eyeInScope);
                        Log("Glass: the eye on the gun (%.3f %.3f %.3f), in the scope's own model (%.3f %.3f %.3f); the scope's z on the gun (%.3f %.3f %.3f)", eyeOnGun[0], eyeOnGun[1], eyeOnGun[2], eyeInScope[0], eyeInScope[1], eyeInScope[2],
                            it->second.rel[0][2], it->second.rel[0][6], it->second.rel[0][10]);
                    }
                }
            }
        }
        if (placed && draw.placedLike == kScopeBody && scopezoom::InScene(ctx))
            NoteGlass(ctx, draw.key);
        // ... and its body with a window in it where its line goes, in both its passes.
        if (const float power = g_scopePower.load(std::memory_order_relaxed);
            power > 1.0f && t.gun && placed && (draw.key == kScopeBody || draw.key == DrawKey{9273, 9594, 0} || draw.key.startIndex == 9357 || draw.key.startIndex == 9486) && DrawnLately(g_rifleDrawn, g_rifleFrame)) {  // (and its red fibre on top: not seen through the window)
            if (const auto it = g_learned.find(kScopeBody); it != g_learned.end() && it->second.have[0]) {
                float at[3], onGun[3], vp[16];
                int eye = -1;
                TransformPoint(it->second.rel[0], g_acogEyepiece, onGun);
                TransformPoint(t.gunPose, onGun, at);
                windowWanted = ViewOfDraw(ctx, vp, eye) && !(scopeview::PictureNow() && power > 1.5f && (eye == 0 || eye == 1)) &&
                               ScopeWindow(ctx, vp, t.eyeInArms, t.gunPose, at, kSightsFall, g_acogEyepieceRadius, power, window);
                if (windowWanted)
                    NoteLensHole(eye, window);
            }
        }
    }
    bool lensMarked = false;
    if (zoomWanted && scopezoom::Begin(ctx, zoomLens, &lensMarked)) {
        t_noHole = true;
        DrawWithPalette(which, ctx, count, start, base, u.resource, t_arms.data(), kGloveUploadSize);
        t_noHole = false;
        scopezoom::End(ctx);
        if (lensMarked)  // its chevron is the lens's only once the map of its marks is made
            g_acogLensFrame = g_frameSerial.load(std::memory_order_relaxed);
        if (!g_scopeGlassOver.load(std::memory_order_relaxed))
            return true;
    }
    // Its chevron is among the lens's own marks then, at their scale. The lens comes first in each eye's picture.
    if (draw.key == kScopeReticle && g_acogLensFrame == g_frameSerial.load(std::memory_order_relaxed) && scopezoom::InScene(ctx))
        return true;
    if (windowWanted && DrawWithWindow(which, ctx, count, start, base, u.resource, t_arms.data(), window)) {
        ++g_handsMovedDraws;
        return true;
    }
    DumpArmPalette(draw.key, t_arms.data(), t_arms.size());
    bool drawn = DrawWithPalette(which, ctx, count, start, base, u.resource, t_arms.data(), kGloveUploadSize,
                                 &draw == &kArmDraws[0],  // the gloves: their cracks closed
                                 draw.placedLike.indexCount && placed ? ScopeBoost(count) : 0);
    if (drawn)
        ++g_handsMovedDraws;
    else
        NoteUnmoved(draw.key, 2);
    drawRounds();
    trace(!drawn    ? "left in place: BF3's buffer not bound"
          : !placed ? "drawn, but no offset from the gun known: where the game put it"
                    : "moved");
    return drawn;
}

// weapon.txt "watch": the scope is a rigid model (no bones: the mesh dump of its 9273 + 12 + 72 +
// 129 + 108 indices has no weights), placed by a matrix in some constant buffer. Each thread's
// latest small uploads are kept, and at the scope's draw those still bound to the vertex shader
// are searched for transforms, to find which one places it.
constexpr DrawKey kScope{9273, 12, 0};
struct RecentUpload {
    ID3D11Resource* resource = nullptr;
    UINT size = 0;
    uint32_t serial = 0;  // t_uploadSerial when it was made
    uint8_t bytes[4096];
};
thread_local RecentUpload t_recent[8];
thread_local int t_recentNext = 0;

void NoteRecentUpload(ID3D11Resource* resource, const uint8_t* data, UINT size) {
    RecentUpload& r = t_recent[t_recentNext];
    t_recentNext = (t_recentNext + 1) % 8;
    r.resource = resource;
    r.serial = t_uploadSerial;
    r.size = std::min(size, static_cast<UINT>(sizeof(r.bytes)));
    std::memcpy(r.bytes, data, r.size);
}

// weapon.txt "scopetrace": how each of the scope's parts is drawn, once per part a trace: blending,
// its pixel shader's constants (when among this thread's latest uploads: "watch" keeps them) and
// textures. The reticle and the red cable looked faint in the headset (user, 2026-10-31).
std::mutex g_materialMutex;
std::vector<DrawKey> g_materialLogged;

void ResetScopeMaterials() {
    std::lock_guard lock(g_materialMutex);
    g_materialLogged.clear();
}

void LogScopeMaterial(ID3D11DeviceContext* ctx, const DrawKey& key) {
    {
        std::lock_guard lock(g_materialMutex);
        if (std::find(g_materialLogged.begin(), g_materialLogged.end(), key) != g_materialLogged.end())
            return;
        g_materialLogged.push_back(key);
    }
    ID3D11BlendState* blend = nullptr;
    FLOAT factor[4] = {};
    UINT mask = 0;
    ctx->OMGetBlendState(&blend, factor, &mask);
    D3D11_BLEND_DESC bd{};
    if (blend) {
        blend->GetDesc(&bd);
        blend->Release();
    }
    const D3D11_RENDER_TARGET_BLEND_DESC& rt = bd.RenderTarget[0];
    ID3D11PixelShader* ps = nullptr;
    ctx->PSGetShader(&ps, nullptr, nullptr);
    if (ps)
        ps->Release();
    Log("Scope material %u/%u: PS %p; blending %s (colour %d %d op %d, alpha %d %d op %d, writes %x), factor %.2f %.2f "
        "%.2f %.2f",
        key.indexCount, key.startIndex, static_cast<void*>(ps), blend && rt.BlendEnable ? "on" : "off",
        rt.SrcBlend, rt.DestBlend, rt.BlendOp, rt.SrcBlendAlpha, rt.DestBlendAlpha, rt.BlendOpAlpha,
        rt.RenderTargetWriteMask, factor[0], factor[1], factor[2], factor[3]);
    ID3D11Buffer* buffers[4] = {};
    ctx->PSGetConstantBuffers(0, 4, buffers);
    for (int slot = 0; slot < 4; ++slot) {
        if (!buffers[slot])
            continue;
        D3D11_BUFFER_DESC desc{};
        buffers[slot]->GetDesc(&desc);
        const RecentUpload* found = nullptr;
        for (int k = 1; k <= 8 && !found; ++k) {  // newest first
            const RecentUpload& r = t_recent[(t_recentNext - k + 8) % 8];
            if (r.resource == buffers[slot] && r.size)
                found = &r;
        }
        char line[1100] = " not among this thread's latest uploads";
        if (found) {
            const auto* f = reinterpret_cast<const float*>(found->bytes);
            int used = 0;
            for (UINT i = 0; i < std::min<UINT>(found->size / 4, 64) && used < 1050; ++i)
                used += snprintf(line + used, sizeof(line) - used, " %g", f[i]);
        }
        Log("Scope material %u/%u: PS constants %d (%u bytes):%s", key.indexCount, key.startIndex, slot,
            desc.ByteWidth, line);
        buffers[slot]->Release();
    }
    ID3D11ShaderResourceView* views[6] = {};
    ctx->PSGetShaderResources(0, 6, views);
    char textures[400] = "";
    int used = 0;
    for (int i = 0; i < 6; ++i) {
        if (!views[i])
            continue;
        D3D11_SHADER_RESOURCE_VIEW_DESC vd{};
        views[i]->GetDesc(&vd);
        ID3D11Resource* resource = nullptr;
        views[i]->GetResource(&resource);
        UINT w = 0, h = 0;
        if (resource) {
            D3D11_RESOURCE_DIMENSION dim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
            resource->GetType(&dim);
            if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D) {
                D3D11_TEXTURE2D_DESC td{};
                static_cast<ID3D11Texture2D*>(resource)->GetDesc(&td);
                w = td.Width, h = td.Height;
            }
            resource->Release();
        }
        if (used < 360)
            used += snprintf(textures + used, sizeof(textures) - used, " t%d %ux%u format %d;", i, w, h, vd.Format);
        views[i]->Release();
    }
    Log("Scope material %u/%u: textures%s", key.indexCount, key.startIndex, used ? textures : " none");
}

void ProbeScope(ID3D11DeviceContext* ctx) {
    static std::atomic<ULONGLONG> lastLog{0};
    ULONGLONG now = GetTickCount64();
    if (now - lastLog.load() < 2000)
        return;
    lastLog = now;
    ID3D11Buffer* bound[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = {};
    ctx->VSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, bound);
    for (UINT slot = 0; slot < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++slot) {
        if (!bound[slot])
            continue;
        D3D11_BUFFER_DESC desc{};
        bound[slot]->GetDesc(&desc);
        bool recent = false;
        for (const RecentUpload& r : t_recent)
            recent |= r.resource == bound[slot];
        Log("Scope probe: vertex shader constant buffer slot %u: %u bytes, %s", slot, desc.ByteWidth,
            recent ? "uploaded just before" : "not among this thread's latest uploads");
        for (const RecentUpload& r : t_recent) {
            if (r.resource != bound[slot] || !r.size)
                continue;
            const auto* f = reinterpret_cast<const float*>(r.bytes);
            int found = 0;
            for (UINT i = 0; i + 12 <= r.size / 4 && found < 6; i += 4) {
                const float* m = f + i;
                // Three rows with unit-length directions and a translation within 6 m: a placement.
                if (std::fabs(Length3(m[0], m[1], m[2]) - 1) > 0.05f || std::fabs(Length3(m[4], m[5], m[6]) - 1) > 0.05f ||
                    std::fabs(Length3(m[8], m[9], m[10]) - 1) > 0.05f || Length3(m[3], m[7], m[11]) > 6.0f)
                    continue;
                ++found;
                float w[4] = {};  // a fourth row, if there is one (4x4 with the translation there)
                if (i + 16 <= r.size / 4)
                    std::memcpy(w, m + 12, sizeof(w));
                Log("Scope probe: slot %u, %u-byte upload, +%u: (%.3f %.3f %.3f %.3f) (%.3f %.3f %.3f %.3f) (%.3f %.3f %.3f "
                    "%.3f) (%.3f %.3f %.3f %.3f)",
                    slot, r.size, i * 4, m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], m[9], m[10], m[11], w[0],
                    w[1], w[2], w[3]);
            }
        }
    }
    for (ID3D11Buffer* b : bound)
        if (b)
            b->Release();
    std::lock_guard lock(g_handsMutex);
    const float* g = g_gunBodyOrig;
    Log("Scope probe: the gun's body for comparison: (%.3f %.3f %.3f %.3f) (%.3f %.3f %.3f %.3f) (%.3f %.3f %.3f %.3f)",
        g[0], g[1], g[2], g[3], g[4], g[5], g[6], g[7], g[8], g[9], g[10], g[11]);
}

// Which of the gun's passes a draw is, or kWeaponPassCount.
size_t WeaponPass(const DrawKey& key) {
    for (size_t i = 0; i < kWeaponPassCount; ++i)
        if (key == kWeaponPasses[i])
            return i;
    return kWeaponPassCount;
}

void WatchDraw(size_t pass) {
    const Upload& u = t_upload;
    if (!u.resource || u.bytes.size() < kPaletteBones * 48 || !PlayersOwn(u.bytes))
        return;
    const auto* m = reinterpret_cast<const float*>(u.bytes.data());
    const float* r = m + (pass == 0 ? kReceiverBone : 0) * 12;
    std::lock_guard lock(g_watchMutex);
    WatchPass& w = g_watch[pass];
    ++w.draws;
    for (UINT k = 0; k < kPaletteBones; ++k) {
        const float* b = m + k * 12;
        float d[3] = {b[3] - r[3], b[7] - r[7], b[11] - r[11]};
        for (int j = 0; j < 3; ++j) {
            float rel = r[j] * d[0] + r[4 + j] * d[1] + r[8 + j] * d[2];  // in the reference's axes
            w.lo[k][j] = w.any ? std::min(w.lo[k][j], rel) : rel;
            w.hi[k][j] = w.any ? std::max(w.hi[k][j], rel) : rel;
        }
    }
    w.any = true;
}

void CaptureMesh(ID3D11DeviceContext* ctx, const DrawKey& key) {
    // A draw asked for by name ("dump 32847 8385") is taken wherever its bones are: The Great
    // Destroyer's bare arms were never dumped, their bones not where the player's own are elsewhere
    // (2026-10-31). Where its first bone sits is logged.
    const bool named = !(key == kWeapon);
    if (((!named || g_dumpOwn.load(std::memory_order_relaxed)) && !PlayersOwn(t_upload.bytes)) || !g_dumpWanted.exchange(false))
        return;
    if (t_upload.bytes.size() >= 48) {
        const auto* first = reinterpret_cast<const float*>(t_upload.bytes.data());
        Log("Dump: draw %u %u, its first bone at (%.3f %.3f %.3f), %zu bytes of bones", key.indexCount, key.startIndex, first[3],
            first[7], first[11], t_upload.bytes.size());
    }
    auto* d = new MeshDump;
    ctx->IAGetVertexBuffers(0, 4, d->vb, d->stride, d->offset);
    ctx->IAGetIndexBuffer(&d->ib, &d->ibFormat, &d->ibOffset);
    ctx->IAGetPrimitiveTopology(&d->topology);
    d->key = key;
    d->bones = t_upload.bytes;
    std::lock_guard lock(g_dumpMutex);
    std::swap(g_dump, d);
    if (d) {  // an earlier one not written yet
        for (ID3D11Buffer* b : d->vb)
            if (b)
                b->Release();
        if (d->ib)
            d->ib->Release();
        delete d;
    }
}

// weapon.txt "pslog 56df872a53641fae": the next three frames' draws with a pixel shader of that code
// (its checksum: the same from one launch to the next, unlike its pointer), each logged with its
// index count and start (which change from launch to launch in The Great Destroyer), its stencil
// reference, its first constant buffer's size, whether the last 4096-byte bone upload is that buffer
// (dumps took another draw's bones when it wasn't) and where its first bone sits: to find the bare
// arms among the draws of the skin shader, which heads use too (2026-10-31).
void LogByShader(ID3D11DeviceContext* ctx, UINT count, UINT start, INT base, UINT instances = 1) {
    ID3D11PixelShader* ps = nullptr;
    ctx->PSGetShader(&ps, nullptr, nullptr);
    if (!ps)
        return;
    // The pointer as this launch's logs show it does too ("pslog 1C3FE7DC").
    const uint64_t sum = shaders::ChecksumOf(ps), wanted = g_psLogSum.load(std::memory_order_relaxed);
    const bool match = sum == wanted || reinterpret_cast<uintptr_t>(ps) == wanted;
    ps->Release();
    if (!match)
        return;
    static uint64_t s_named = 0;
    if (s_named != wanted) {
        s_named = wanted;
        Log("PS log: pixel shader %p has the code %016llx", static_cast<void*>(ps), sum);
    }
    ID3D11Buffer* bones = nullptr;
    ctx->VSGetConstantBuffers(0, 1, &bones);
    UINT size = 0;
    if (bones) {
        D3D11_BUFFER_DESC desc;
        bones->GetDesc(&desc);
        size = desc.ByteWidth;
    }
    const bool ours = bones && t_upload.resource == bones && t_upload.bytes.size() >= 48;
    if (bones)
        bones->Release();
    ID3D11DepthStencilState* state = nullptr;
    UINT ref = 0;
    ctx->OMGetDepthStencilState(&state, &ref);
    if (state)
        state->Release();
    float first[3] = {};
    if (ours) {
        const auto* b = reinterpret_cast<const float*>(t_upload.bytes.data());
        first[0] = b[3], first[1] = b[7], first[2] = b[11];
    }
    Log("PS log: DrawIndexed %u from %u base %d x%u; stencil ref %u; VS buffer 0 %u bytes, %s; first bone (%.3f %.3f "
        "%.3f)",
        count, start, base, instances, ref, size, ours ? "the last bone upload" : "not the last bone upload", first[0],
        first[1], first[2]);
}

// weapon.txt "fpunknown": logs this draw if a first-person view's constants are bound to it (and
// it's the first of its kind since the command).
void ProbeFirstPerson(ID3D11DeviceContext* ctx, UINT count, UINT start, INT base) {
    ID3D11Buffer* buffers[8] = {};
    ctx->VSGetConstantBuffers(0, 8, buffers);
    int slot = -1;
    UINT sizes[8] = {};
    {
        std::lock_guard lock(g_fpMutex);
        for (int i = 0; i < 8; ++i)
            if (buffers[i] && slot < 0 &&
                std::find(std::begin(g_armsViewBuffers), std::end(g_armsViewBuffers), buffers[i]) != std::end(g_armsViewBuffers))
                slot = i;
    }
    for (int i = 0; i < 8; ++i)
        if (buffers[i]) {
            D3D11_BUFFER_DESC desc;
            buffers[i]->GetDesc(&desc);
            sizes[i] = desc.ByteWidth;
            buffers[i]->Release();
        }
    if (slot < 0)
        return;
    const uint64_t id = (static_cast<uint64_t>(count) << 32) ^ (static_cast<uint64_t>(start) << 4) ^
                        static_cast<uint32_t>(base);
    {
        std::lock_guard lock(g_fpProbeMutex);
        if (!g_fpProbeSeen.insert(id).second)
            return;
    }
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ctx->VSGetShader(&vs, nullptr, nullptr);
    ctx->PSGetShader(&ps, nullptr, nullptr);
    if (vs)
        vs->Release();
    if (ps)
        ps->Release();
    ID3D11DepthStencilState* depth = nullptr;
    UINT ref = 0;
    ctx->OMGetDepthStencilState(&depth, &ref);
    if (depth)
        depth->Release();
    ID3D11BlendState* blend = nullptr;
    FLOAT factor[4];
    UINT mask = 0;
    ctx->OMGetBlendState(&blend, factor, &mask);
    bool blended = false;
    if (blend) {
        D3D11_BLEND_DESC desc;
        blend->GetDesc(&desc);
        blended = desc.RenderTarget[0].BlendEnable != FALSE;
        blend->Release();
    }
    ID3D11Buffer* vb = nullptr;
    UINT stride = 0, vbOffset = 0;
    ctx->IAGetVertexBuffers(0, 1, &vb, &stride, &vbOffset);
    if (vb)
        vb->Release();
    Log("First-person probe: DrawIndexed %u from %u base %d: view constants in slot %d; VS buffers %u %u %u %u %u %u; "
        "stride %u; VS %p PS %p; %s; stencil ref %u",
        count, start, base, slot, sizes[0], sizes[1], sizes[2], sizes[3], sizes[4], sizes[5], stride, vs, ps,
        blended ? "blended" : "opaque", ref);
}

// The launcher of Rock and a Hard Place, the SMAW (docs\research\smaw.md), weapon.txt "launcher 1". Its frame bone goes where
// the right controller holds it, and every bone of it, the gloves and the forearms moves by that one move, so the game's own
// hold and reload stay as they are: its left hand holds it all through the reload, its right fetches the rocket. The chest,
// sleeves and legs fold away as with the M416. Its draws' keys are shared with other models: part A is told by its bones.
constexpr DrawKey kLauncherA{3414, 0, 0}, kLauncherB{18222, 3414, 0}, kLauncherC{120, 21636, 0}, kLauncherDepth{21756, 21756, 0};
constexpr float kLauncherMuzzle = 0.6f;  // its front end on the frame's z, from pictures
constexpr float kLauncherRearFromFrame = 0.4983f;  // bones 0 and 2 of part A apart (m): part B's bone 0 is its bone 2
float g_launcherRear[12] = {};  // part A's bone 2 on its bone 0. Under g_handsMutex, as are these:
bool g_haveLauncherRear = false;
float g_launcherCaseRest[12] = {};  // its case, part A's bone 1, at home on the frame
// Each hand's bones on the launcher's frame, learned only from a frame drawn in the same view (the count of view uploads then), as the M416's
// are from its body: moved by a frame from another view, the right glove jumped up and down as the soldier walked (the arms' space moves a
// frame's walk, a whole metre when its origin jumps) and one eye could see it elsewhere than the other (the user, 2026-11-04). A draw that comes
// before the frame in its view is kept until the frame is drawn.
//
// What's seen in a frame only counts from its end, so a frame's depth and colour draws all go by the same frame's bones (a frame apart, the
// forearm's black holes while the game's arms moved: film of the switch, 2026-11-04). And once the game's arms have rested on the launcher that
// rest is kept for good: raising it and putting it away the game swings its arms about it (and at the very end parts its bones), the drawn
// launcher stays in the hand, so the hands stay on it from the first frame to the last. Only its own reload is followed (launcherGameHands), and
// its first raise, nothing being known before it.
struct RigSet {
    std::unordered_map<DrawKey, Learned, DrawKeyHash> bones;
    float fold[2][3] = {};  // the middle of each forearm on the frame
    bool haveFold = false;
    float left[handpose::kBones][12] = {};  // the game's left glove on the frame (LauncherLeftGrip)
    bool haveLeft = false;
};
RigSet g_rigNext, g_rigNow, g_rigRest;
bool g_rigFrozen = false;  // g_rigRest is the game's arms at rest
uint32_t g_launcherFrameView = 0;
std::vector<PendingArms> g_pendingRig;
// How the rest is known: both palms on their usual hold on the frame (measured 2026-11-04, the same to a millimetre at each rest), the middle of
// each forearm within 5 mm of one place, for this long.
constexpr float kRigRightPalm[3] = {-0.026f, -0.141f, 0.061f}, kRigLeftPalm[3] = {0.039f, -0.073f, 0.315f};
constexpr float kRigPalmTolerance = 0.01f, kRigFoldSteady = 0.005f;
constexpr ULONGLONG kRigRestMs = 700, kRigRefreshMs = 1500;  // first taken after so long, then kept up to date each so long at rest
bool g_rigPalmsHome = false, g_rigHaveSteady = false;
float g_rigSteadyFold[2][3] = {}, g_rigLastFold[2][3] = {};
ULONGLONG g_rigSteadySince = 0, g_rigSeenAt = 0, g_rigStillSince = 0;
std::atomic<bool> g_rigThisFrame{false};  // the launcher in hand as this frame began: all of a frame's arm draws go one way (Update)
bool RigThisFrame() {
    return g_rigThisFrame.load(std::memory_order_relaxed);
}

const RigSet& RigInUse() {
    return g_rigFrozen && !g_hands.launcherGameHands ? g_rigRest : g_rigNow;
}
// The game's left hand's frame (the gloves' bone 1, the right's 32 mirrored) on the gun's frame as a draw with rig in use places it (DrawLauncherRig's placed): the
// rig's own, else as seen now; null while neither has it. Under g_handsMutex.
const float* RigLeftHand(const RigSet& rig) {
    constexpr size_t kLeftHandFrame = 1;
    const DrawKey& gloves = kArmDraws[0].key;
    const RigSet* const sets[] = {&rig, &g_rigNow};
    for (const RigSet* set : sets)
        if (const auto it = set->bones.find(gloves); it != set->bones.end() && it->second.have[kLeftHandFrame])
            return it->second.rel[kLeftHandFrame];
    return nullptr;
}
void RigFor(uint32_t kind);  // each gun keeps its own rest

// Any other gun: where the game's palms rest on it isn't known beforehand. Taken when they've stayed within a centimetre of one place on it for a while, the first time after a second; its
// own moves, a reload above all, take them off it, and till they're back its hands are followed (HeldGameMoving). The rule is heldpalms.h's (tools\magseattest.cpp runs it as it is).
// Under g_handsMutex; its moving also in g_heldMoving, for the readers off that lock.
heldpalms::State g_heldPalms;
std::atomic<bool> g_heldMoving{false};

void NoteHeldPalms(const float* right, const float* left) {
    const bool taken = heldpalms::Note(g_heldPalms, right, left, GetTickCount64(), input::Sprinting());
    g_heldMoving.store(g_heldPalms.moving);
    g_rigPalmsHome = g_heldPalms.home;
    if (taken)
        Log("Held gun: the game's palms rest on it at right (%.3f %.3f %.3f), left (%.3f %.3f %.3f)", right[0], right[1], right[2], left[0], left[1], left[2]);
}

void LearnRig(const ArmDraw& draw, const float* bones, const float* frameInverse) {
    if (&draw == &kArmDraws[0]) {
        float at[3], right[3], left[3];
        TransformPoint(bones + draw.rightRoot * 12, kRightPalm, at);
        TransformPoint(frameInverse, at, right);
        TransformPoint(bones + 1 * 12, kLeftPalm, at);
        TransformPoint(frameInverse, at, left);
        const float dr = Length3(right[0] - kRigRightPalm[0], right[1] - kRigRightPalm[1], right[2] - kRigRightPalm[2]);
        const float dl = Length3(left[0] - kRigLeftPalm[0], left[1] - kRigLeftPalm[1], left[2] - kRigLeftPalm[2]);
        if (g_heldKind.load(std::memory_order_relaxed))
            NoteHeldPalms(right, left);
        else
            g_rigPalmsHome = dr < kRigPalmTolerance && dl < kRigPalmTolerance;
        if (static ULONGLONG s_said = 0; g_launcherLog.load(std::memory_order_relaxed)) {
            if (const ULONGLONG now = GetTickCount64(); now - s_said >= 1000) {
                s_said = now;
                Log("Launcher: the game's palms on its frame: right (%.3f %.3f %.3f) %.0f mm from the usual, left (%.3f %.3f %.3f) %.0f mm", right[0],
                    right[1], right[2], dr * 1000.0f, left[0], left[1], left[2], dl * 1000.0f);
            }
        }
    }
    Learned& learned = g_rigNext.bones[draw.key];
    for (size_t k = 0; draw.roles[k] && k < kPaletteBones; ++k) {
        if (draw.roles[k] != 'L' && draw.roles[k] != 'R')
            continue;
        Learn(learned, k, frameInverse, bones + k * 12);
        if (draw.roles[k] == 'L' && &draw == &kArmDraws[0] && k < handpose::kBones) {
            std::memcpy(g_rigNext.left[k], learned.rel[k], sizeof(g_rigNext.left[k]));
            if (k + 1 == handpose::kBones)
                g_rigNext.haveLeft = true;
        }
    }
    if (draw.key == kBareForearms) {  // the middle of each forearm, as the bare forearm's two middle bones hold it
        float at[3];
        TransformPoint(bones, kForearmMiddle[0], at);
        TransformPoint(frameInverse, at, g_rigNext.fold[0]);
        TransformPoint(bones + 12 * 12, kForearmMiddle[1], at);
        TransformPoint(frameInverse, at, g_rigNext.fold[1]);
        g_rigNext.haveFold = true;
        bool steady = g_rigHaveSteady && g_rigPalmsHome;
        for (int side = 0; side < 2; ++side) {
            const float* a = g_rigNext.fold[side];
            const float* b = g_rigSteadyFold[side];
            steady = steady && Length3(a[0] - b[0], a[1] - b[1], a[2] - b[2]) < kRigFoldSteady;
        }
        if (!steady) {
            std::memcpy(g_rigSteadyFold, g_rigNext.fold, sizeof(g_rigSteadyFold));
            g_rigHaveSteady = true;
            g_rigSteadySince = GetTickCount64();
        }
    }
}

// The frame's end: what was seen in it counts now, and it may be the rest the launcher's kept with.
void TakeFrameOfRig() {
    // The game's right hand half a metre off in one frame, as it's put away (following its own reload the arms were drawn up in the sky for a frame as the rifle came back: film,
    // 2026-11-04): that frame isn't taken. Its reload moves it a centimetre or two a frame. Staying there, it counts.
    {
        constexpr size_t kRightRoot = 37;
        static int s_refused = 0;
        const DrawKey& gloves = kArmDraws[0].key;
        const auto next = g_rigNext.bones.find(gloves);
        const auto last = g_rigNow.bones.find(gloves);
        if (next != g_rigNext.bones.end() && last != g_rigNow.bones.end() && next->second.have[kRightRoot] && last->second.have[kRightRoot]) {
            const float* a = next->second.rel[kRightRoot];
            const float* b = last->second.rel[kRightRoot];
            if (Length3(a[3] - b[3], a[7] - b[7], a[11] - b[11]) > 0.15f && ++s_refused <= 20) {
                for (auto& [key, kept] : g_rigNext.bones)
                    std::fill(std::begin(kept.have), std::end(kept.have), false);
                g_rigNext.haveFold = g_rigNext.haveLeft = false;
                g_rigHaveSteady = false;
                return;
            }
            s_refused = 0;
        }
    }
    const bool freshFold = g_rigNext.haveFold;
    for (auto& [key, next] : g_rigNext.bones) {
        Learned& now = g_rigNow.bones[key];
        for (size_t k = 0; k < kPaletteBones; ++k)
            if (next.have[k]) {
                std::memcpy(now.rel[k], next.rel[k], sizeof(now.rel[k]));
                now.have[k] = true;
                next.have[k] = false;
            }
    }
    if (g_rigNext.haveFold) {
        std::memcpy(g_rigNow.fold, g_rigNext.fold, sizeof(g_rigNow.fold));
        g_rigNow.haveFold = true;
        g_rigNext.haveFold = false;
    }
    if (g_rigNext.haveLeft) {
        std::memcpy(g_rigNow.left, g_rigNext.left, sizeof(g_rigNow.left));
        g_rigNow.haveLeft = true;
        g_rigNext.haveLeft = false;
    }
    // A paused game is still drawn, bit for bit the same, and a frame with nothing new says nothing (a controller lost, the hands off): neither is a rest. Nor a sprint.
    const ULONGLONG now = GetTickCount64();
    if (freshFold) {
        const bool same = std::memcmp(g_rigLastFold, g_rigNow.fold, sizeof(g_rigLastFold)) == 0;
        std::memcpy(g_rigLastFold, g_rigNow.fold, sizeof(g_rigLastFold));
        g_rigStillSince = !same ? 0 : g_rigStillSince ? g_rigStillSince : now;
    }
    if (!freshFold || (g_rigStillSince && now - g_rigStillSince > 250) || input::Sprinting() || g_hands.launcherGameHands) {
        g_rigHaveSteady = false;
        return;
    }
    // Kept up to date while it rests: taken wrong once, it isn't wrong for good.
    if (g_rigHaveSteady && g_rigPalmsHome && g_rigNow.haveLeft && now - g_rigSteadySince > (g_rigFrozen ? kRigRefreshMs : kRigRestMs)) {
        g_rigRest = g_rigNow;
        g_rigSteadySince = now;
        if (!g_rigFrozen)
            Log("Launcher: the game's arms at rest on it: kept, the hands stay on it whatever the game's arms do");
        g_rigFrozen = true;
    }
}

void KeepRig(const ArmDraw& draw, const float* bones, uint32_t view) {
    auto it = std::find_if(g_pendingRig.begin(), g_pendingRig.end(), [&](const PendingArms& p) { return p.draw == &draw; });
    if (it == g_pendingRig.end())
        it = g_pendingRig.insert(g_pendingRig.end(), PendingArms{&draw});
    size_t used = 0;
    while (used < kPaletteBones && draw.roles[used])
        ++used;
    std::memcpy(it->bones, bones, used * sizeof(it->bones[0]));
    it->view = view;
    it->waiting = true;
}

// The frame was just drawn: the draws kept from this view learn now.
void LearnRigKept(uint32_t view, const float* frameInverse) {
    for (PendingArms& p : g_pendingRig) {
        if (!p.waiting)
            continue;
        p.waiting = false;
        if (p.view == view)
            LearnRig(*p.draw, &p.bones[0][0], frameInverse);
    }
}

// The SMAW's own FiringFunctionData's shot at place (its ShotConfigData: the firing data starts at4shot::kShotAt before it), read now: still the
// firing data the scan took for the SMAW's own there (its class's, with who's numbers: g_launcherShotWho's), its start as its data has it or as last
// (null: the data's alone). Never the AT4's, the RPG-7's or the enemies' RPG-7's, whose shot is the SMAW's to the bit (the scan never takes them).
bool LauncherShotThere(uintptr_t place, const at4shot::Launcher& who, const float* last) {
    uint8_t firing[at4shot::kFiringBytes];
    SIZE_T got = 0;
    return place > at4shot::kShotAt && ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(place - at4shot::kShotAt), firing, sizeof(firing), &got) &&
           got == sizeof(firing) && at4shot::FiringIs(firing, who, last);
}

// The FiringFunctionData a weapon of the soldier's own points at (at4shot.h: its firing state's WeaponFiringData, that data's PrimaryFire), by its
// slot; 0 none read. The scan's thread: ammo::FiringOf reads only the game's memory, through guarded reads (as the held guns' draws call it from the
// draw threads), the two pointers after it through ReadProcessMemory (the soldier's weapons and their data: plain memory, never the GPU's).
uintptr_t SlotFiringData(int slot) {
    const uintptr_t state = ammo::FiringOf(slot);
    uint32_t data = 0, fire = 0;
    SIZE_T got = 0;
    if (!state || !ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(state + at4shot::kStateData), &data, sizeof(data), &got) ||
        got != sizeof(data) || !data)
        return 0;
    got = 0;
    return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(static_cast<uintptr_t>(data) + at4shot::kPrimaryFire), &fire, sizeof(fire), &got) &&
                   got == sizeof(fire)
               ? fire
               : 0;
}

// The SMAW's own starts a scan took, each with the numbers it's held to, added to those known (under g_handsMutex: a place, its numbers and its last
// written start go together; a new one's last none yet). How many are known.
size_t AddLauncherShots(const std::vector<std::pair<uintptr_t, at4shot::Launcher>>& taken) {
    std::lock_guard lock(g_handsMutex);
    for (const auto& [place, who] : taken) {
        bool have = false;
        for (const auto& k : g_launcherShot)
            have = have || k.load(std::memory_order_relaxed) == place;
        if (have)
            continue;
        for (size_t n = 0; n < std::size(g_launcherShot); ++n)
            if (!g_launcherShot[n].load(std::memory_order_relaxed)) {
                g_launcherShotWho[n] = who;
                std::memset(g_launcherShotLast[n], 0, sizeof(g_launcherShotLast[n]));
                g_launcherShot[n].store(place, std::memory_order_relaxed);
                break;
            }
    }
    size_t count = 0;
    for (const auto& k : g_launcherShot)
        count += k.load(std::memory_order_relaxed) != 0;
    return count;
}

// The frame (g_frameSerial) the SMAW's rocket's start was last written in (its writer in DrawLauncher, under g_handsMutex); 0 none since put back.
std::atomic<uint32_t> g_launcherShotFrame{0};

// The SMAW out of the hands for half a second (Update, each frame): its rocket's start back as its data has it, wherever it was written, as the AT4's
// (RestoreAt4Shot): nothing of the mod's left in the game's data while no hand holds it, so a rocket the game fires then (the mod not drawing it: free
// hands asleep, the headset off) starts where the game's own does. Back in a hand, it's the tube's end again from its first frame.
void RestoreLauncherShot() {
    const uint32_t frame = g_launcherShotFrame.load(std::memory_order_relaxed);
    if (!frame || static_cast<int32_t>(g_frameSerial.load(std::memory_order_relaxed) - frame) < 36)
        return;  // (a signed difference: a frame stored ahead of this one is no half second out of the hands)
    std::lock_guard lock(g_handsMutex);
    if (g_launcherShotFrame.load(std::memory_order_relaxed) != frame)
        return;  // (written again meanwhile)
    static constexpr float kStart[3] = {at4shot::kStart[0], at4shot::kStart[1], at4shot::kStart[2]};
    static constexpr float kNever[3] = {};
    int back = 0;
    for (size_t n = 0; n < std::size(g_launcherShot); ++n) {
        const uintptr_t place = g_launcherShot[n].load(std::memory_order_relaxed);
        if (!place || std::memcmp(g_launcherShotLast[n], kStart, sizeof(kStart)) == 0 || std::memcmp(g_launcherShotLast[n], kNever, sizeof(kNever)) == 0)
            continue;  // (as its data has it, or never written)
        if (!LauncherShotThere(place, g_launcherShotWho[n], g_launcherShotLast[n])) {
            g_launcherShot[n] = 0;  // (not its data any more: a load moved it)
            continue;
        }
        if (WriteProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(place), kStart, sizeof(kStart), nullptr)) {
            std::memcpy(g_launcherShotLast[n], kStart, sizeof(kStart));
            ++back;
        }
    }
    g_launcherShotFrame = 0;
    if (back)
        Log("Launcher: out of the hands: its rocket's start put back as its data has it (%d place%s)", back, back == 1 ? "" : "s");
}

// The launchers' spread taken out of the game's data, so a rocket goes where the tube points without zooming (the user:
// "free aim"; zoomed it did go true, 2026-11-02). 40mm_GunSway, which the M320, the SMAW and the Stinger share, has for each stance
// a dispersion of min angle, max angle, 1.8 a shot, 15 a second. Read through ReadProcessMemory: the heap changes under a scan.
DWORD WINAPI ClearLauncherSpread(LPVOID) {
    const ScanThread scanning;  // (StartScan's: the lowest priority, one at a time)
    const ULONGLONG began = GetTickCount64();
    const HANDLE self = GetCurrentProcess();
    std::vector<float> chunk(256 * 1024);
    const SIZE_T chunkBytes = chunk.size() * sizeof(float);
    std::vector<uintptr_t> places, shots;
    places.reserve(64);
    shots.reserve(16);
    // Only its own numbers: the first try took anything ending 1.8, 15, wrote over the game's memory and crashed it (2026-11-02).
    auto isOurs = [](const float* f) {
        return std::fabs(f[1] - 5.5f) < 1e-3f && f[0] >= 1.0f && f[0] <= 8.0f && std::fabs(f[0] * 2.0f - std::round(f[0] * 2.0f)) < 1e-3f &&
               std::fabs(f[2] - 1.8f) < 1e-4f && std::fabs(f[3] - 15.0f) < 1e-4f;
    };
    MEMORY_BASIC_INFORMATION info;
    for (uintptr_t at = 0x10000; VirtualQuery(reinterpret_cast<LPCVOID>(at), &info, sizeof(info)) == sizeof(info);) {
        const uintptr_t end = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
        if (info.State == MEM_COMMIT && info.Protect == PAGE_READWRITE) {
            for (uintptr_t from = reinterpret_cast<uintptr_t>(info.BaseAddress); from + 16 <= end;) {
                const SIZE_T want = std::min<SIZE_T>(chunkBytes, end - from);
                SIZE_T got = 0;
                if (!ReadProcessMemory(self, reinterpret_cast<LPCVOID>(from), chunk.data(), want, &got) || got < 16)
                    break;
                const size_t floats = got / sizeof(float);
                for (size_t i = 0; i + 4 <= floats; ++i) {
                    // Its ShotConfigData: InitialPosition, InitialDirection and InitialSpeed, each padded to 16 bytes.
                    if (i + 12 <= floats && std::fabs(chunk[i] + 0.3f) < 1e-4f && std::fabs(chunk[i + 1] + 0.1f) < 1e-4f &&
                        std::fabs(chunk[i + 2] - 2.0f) < 1e-4f && chunk[i + 4] == 0.0f && std::fabs(chunk[i + 5] - 0.01f) < 1e-4f &&
                        std::fabs(chunk[i + 6] - 1.0f) < 1e-4f && chunk[i + 8] == 0.0f && chunk[i + 9] == 0.0f &&
                        std::fabs(chunk[i + 10] - 40.0f) < 1e-3f) {
                        const uintptr_t place = from + i * sizeof(float);
                        const auto own = reinterpret_cast<uintptr_t>(chunk.data());
                        if (place < own || place >= own + chunkBytes) {
                            shots.push_back(place);
                            Log("Launcher: a rocket's start like the SMAW's found at %p", reinterpret_cast<void*>(place));
                        }
                    }
                    if (chunk[i + 3] == chunk[i + 3] && isOurs(&chunk[i])) {
                        const uintptr_t place = from + i * sizeof(float);
                        const auto own = reinterpret_cast<uintptr_t>(chunk.data());
                        if ((place < own || place >= own + chunkBytes) && places.size() < places.capacity())
                            places.push_back(place);
                    }
                }
                if (got < want || want < chunkBytes)
                    break;
                from += want - 12;  // a dispersion across the seam is seen whole
            }
        }
        if (end <= at)
            break;
        at = end;
    }
    // Its direction stays the data's, 0.57 degrees up: the game's allowance for the drop (made straight, rockets fell short of
    // a BMP aimed at through the sights: the user, 2026-11-02). Added to those known, as the rifle's. Only the SMAW's own: the start, the direction
    // and the speed are the AT4's, the RPG-7's and the enemies' RPG-7's to the bit (at4shot.h), and the enemies' rockets started from the player's
    // tube (Operation Swordbreaker's roof had the AT4's and RPG7_AI's). Its own is told by its FiringFunctionData (at4shot::SmawOwn): the one a weapon
    // of the soldier's own points at (Rock and a Hard Place: slot 5's), whatever its numbers; else Venice-EBX's numbers for it (350 and 5.8, never read
    // in this game); else, neither telling it, each one whose firing data is no known launcher's, as the scan before the AT4 took them all, so the
    // SMAW never starts from the game's eye where it started from its tube before. Each one taken is held to its own numbers as read now.
    {
        std::vector<at4shot::Match> matches(shots.size());
        for (size_t i = 0; i < shots.size(); ++i) {
            at4shot::Match& m = matches[i];
            m.place = shots[i];
            SIZE_T got = 0;
            m.read = m.place > at4shot::kShotAt &&
                     ReadProcessMemory(self, reinterpret_cast<LPCVOID>(m.place - at4shot::kShotAt), m.firing, sizeof(m.firing), &got) && got == sizeof(m.firing);
        }
        uintptr_t fires[8] = {};  // (each slot ammo::FiringOf reads)
        for (size_t s = 0; s < std::size(fires) && !matches.empty(); ++s)
            fires[s] = SlotFiringData(static_cast<int>(s));
        const at4shot::SmawBy by = at4shot::SmawOwn(matches.data(), matches.size(), fires, std::size(fires));
        std::vector<std::pair<uintptr_t, at4shot::Launcher>> taken;
        for (const at4shot::Match& m : matches) {
            void* const at = reinterpret_cast<void*>(m.place);
            if (m.own) {
                const at4shot::Launcher who = at4shot::NumbersOf(m.firing, at4shot::kSmaw.name);
                taken.emplace_back(m.place, who);
                const bool venice = who.rate == at4shot::kSmaw.rate && who.reload == at4shot::kSmaw.reload;
                if (by == at4shot::SmawBy::kSlot)
                    Log("Launcher: its rocket's start at %p is the SMAW's own (the firing data of the soldier's weapon in slot %d: RateOfFire %g, ReloadTime %g%s)", at,
                        at4shot::SmawSlotOf(m, fires, std::size(fires)), who.rate, who.reload, venice ? ", Venice-EBX's" : ", not Venice-EBX's 350 and 5.8");
                else if (by == at4shot::SmawBy::kNumbers)
                    Log("Launcher: its rocket's start at %p is the SMAW's own (its firing data's RateOfFire 350, ReloadTime 5.8)", at);
                else
                    Log("Launcher: its rocket's start at %p taken for the SMAW's (its firing data no known launcher's: RateOfFire %g, ReloadTime %g)", at, who.rate,
                        who.reload);
                continue;
            }
            const char* whose = m.read ? at4shot::WhoseFiring(m.firing, nullptr) : nullptr;
            const bool anotherSmaw = whose && !std::strcmp(whose, at4shot::kSmaw.name);  // (the SMAW's numbers, not the soldier's own weapon's)
            const char* what = anotherSmaw                     ? "firing data with the SMAW's numbers that no weapon of the soldier's has"
                               : whose                           ? whose
                               : !m.read                         ? "in firing data that can't be read"
                               : by == at4shot::SmawBy::kSlot    ? "another's (no weapon of the soldier's has its firing data)"
                                                                 : "another gun's (its firing data not the SMAW's)";
            Log("Launcher: a rocket's start like the SMAW's at %p is %s: left alone", at, what);
        }
        if (by == at4shot::SmawBy::kNoOther)
            Log("Launcher: none of the %zu rocket starts like the SMAW's is its own by a weapon of the soldier's or by its numbers (350, 5.8): the %zu whose "
                "firing data is no known launcher's taken for it, as before the AT4",
                matches.size(), taken.size());
        const size_t known = AddLauncherShots(taken);
        if (!taken.empty())
            Log("Launcher: its rocket's start found in %zu more places, %zu known", taken.size(), known);
        else if (!matches.empty() && !known)
            Log("Launcher: none of the %zu rocket starts like the SMAW's is its own: nothing written (a SMAW's rockets start from the game's eye)",
                matches.size());
    }
    if (places.empty() || places.size() > 40) {
        Log("Launcher: %zu places like its spread found in %llu ms: left alone", places.size(), GetTickCount64() - began);
        return 0;
    }
    int cleared = 0;
    for (const uintptr_t place : places) {
        float now[4];
        SIZE_T got = 0;
        if (!ReadProcessMemory(self, reinterpret_cast<LPCVOID>(place), now, sizeof(now), &got) || got != sizeof(now) || !isOurs(now))
            continue;
        // Its least and its growth go; its most stays, so min < max as the game expects.
        const float zero = 0.0f;
        if (WriteProcessMemory(self, reinterpret_cast<void*>(place), &zero, sizeof(zero), nullptr) &&
            WriteProcessMemory(self, reinterpret_cast<void*>(place + 8), &zero, sizeof(zero), nullptr))
            ++cleared;
        Log("Launcher: spread from %.2f degrees (most %.2f) taken out at %p", now[0], now[1], reinterpret_cast<void*>(place));
    }
    Log("Launcher: %d of %zu spreads taken out of its data, %llu ms", cleared, places.size(), GetTickCount64() - began);
    return 0;
}

// The Stinger's data (Venice-EBX, Weapons/FIM92A/FIM92A_SP_Valley), as soon as it's in the hand. Its LockingControllerData: LockTime 2, ReleaseTime 0.25, ReleaseOnNewTargetTime 0.5, SampleRate 0.06, HoldStillThreshold 10,
// RayLength 3000, AcceptanceAngle 5, behind the pointer to its ZoomLevelLock array {OutlineTaggedDistance, LockType}: no lock unzoomed (5, LockTypeCount), heat zoomed (2). The user won't have a zoom, so unzoomed takes zoomed's. Its
// ShotConfigData: InitialPosition, InitialDirection, InitialSpeed, each padded to 16 bytes, from the game's eye: made the tube's end each frame, as the SMAW's rocket. Exact numbers only, read through ReadProcessMemory.
std::atomic<uintptr_t> g_stingerShot[8] = {};
float g_stingerShotLast[std::size(g_stingerShot)][3] = {};
constexpr size_t kStingerGun = 1000;  // HeldKind's spreadGun: this is no rifle; its scatter is the launchers' (ClearLauncherSpread)

DWORD WINAPI FindStinger(LPVOID) {
    const ScanThread scanning;  // (StartScan's: the lowest priority, one at a time)
    const ULONGLONG began = GetTickCount64();
    const HANDLE self = GetCurrentProcess();
    std::vector<float> chunk(256 * 1024);
    const SIZE_T chunkBytes = chunk.size() * sizeof(float);
    const auto own = reinterpret_cast<uintptr_t>(chunk.data());
    std::vector<uintptr_t> shots;
    std::vector<uint32_t> locks;
    MEMORY_BASIC_INFORMATION info;
    for (uintptr_t at = 0x10000; VirtualQuery(reinterpret_cast<LPCVOID>(at), &info, sizeof(info)) == sizeof(info);) {
        const uintptr_t end = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
        if (info.State == MEM_COMMIT && info.Protect == PAGE_READWRITE) {
            for (uintptr_t from = reinterpret_cast<uintptr_t>(info.BaseAddress); from + 64 <= end;) {
                const SIZE_T want = std::min<SIZE_T>(chunkBytes, end - from);
                SIZE_T got = 0;
                if (!ReadProcessMemory(self, reinterpret_cast<LPCVOID>(from), chunk.data(), want, &got) || got < 64)
                    break;
                const size_t floats = got / sizeof(float);
                for (size_t i = 1; i + 12 <= floats; ++i) {
                    const uintptr_t place = from + i * sizeof(float);
                    if (place >= own && place < own + chunkBytes)
                        continue;
                    if (std::fabs(chunk[i] + 0.2f) < 1e-4f && std::fabs(chunk[i + 1] + 0.065f) < 1e-4f && std::fabs(chunk[i + 2] - 0.35f) < 1e-4f && chunk[i + 4] == 0.0f && std::fabs(chunk[i + 5] - 0.12f) < 1e-4f &&
                        chunk[i + 6] == 1.0f && chunk[i + 8] == 0.0f && chunk[i + 9] == 0.0f && chunk[i + 10] == 16.0f)
                        shots.push_back(place);
                    if (chunk[i] == 2.0f && chunk[i + 1] == 0.25f && chunk[i + 2] == 0.5f && std::fabs(chunk[i + 3] - 0.06f) < 1e-4f && chunk[i + 4] == 10.0f && chunk[i + 5] == 3000.0f && chunk[i + 6] == 5.0f) {
                        uint32_t array = 0;
                        std::memcpy(&array, &chunk[i - 1], sizeof(array));
                        locks.push_back(array);
                    }
                }
                if (got < want || want < chunkBytes)
                    break;
                from += want - 64;  // across the seam
            }
        }
        if (end <= at)
            break;
        at = end;
    }
    if (const size_t known = AddPlaces(g_stingerShot, shots); !shots.empty())
        Log("Stinger: its missile's start found in %zu places, %zu known", shots.size(), known);
    int unlocked = 0, already = 0;
    for (const uint32_t array : locks) {
        // The array's count stands before it: exactly two, as its data has them.
        struct {
            uint32_t count;
            float d0;
            uint32_t type0;
            float d1;
            uint32_t type1;
        } now{};
        SIZE_T got = 0;
        if (array < 0x10000 || !ReadProcessMemory(self, reinterpret_cast<LPCVOID>(static_cast<uintptr_t>(array) - 4), &now, sizeof(now), &got) || got != sizeof(now) || now.count != 2 || now.d0 != 0.0f || now.d1 != 0.0f)
            continue;
        if (now.type1 != 2)
            continue;
        if (now.type0 == 2) {
            ++already;
            continue;
        }
        if (now.type0 != 5)
            continue;
        const uint32_t heat = 2;
        if (WriteProcessMemory(self, reinterpret_cast<void*>(static_cast<uintptr_t>(array) + 4), &heat, sizeof(heat), nullptr))
            ++unlocked;
    }
    Log("Stinger: its lock works unzoomed in %d of %zu places (%d already), %llu ms", unlocked, locks.size(), already, GetTickCount64() - began);
    return 0;
}

// The AT4's data (Venice-EBX, Weapons/M136/M136; Operation Swordbreaker's hotel roof), as soon as it's in the hand. Its ShotConfigData is the SMAW's,
// the RPG-7's and the enemies' RPG-7's to the bit: InitialPosition (-0.3 -0.1 2.0), InitialDirection (0 0.01 1), InitialSpeed (0 0 40), each padded
// to 16 bytes, from the game's eye (ForceSpawnToCamera): made the tube's end each frame while the AT4 is in a hand, in its own FiringFunctionData alone
// (at4shot.h: its vtable, RateOfFire 350 and ReloadTime 4.2 with the shot; the roof's run found two exact matches, the other the RPG soldiers'), and
// put back as its data has it once the AT4 has been out of the hands for half a second (RestoreAt4Shot). Its GunSwayData is its own: standing
// and crouched unzoomed its least 1 degree (most 5) still and 3 moving, zoomed 0 and 2 (most 2 and 3); prone 0 either way. The unzoomed rows of each
// stance are made that stance's zoomed ones (the user's free aim never zooms), as ClearRifleSpread does a rifle's, through the same rows (one hand,
// "riflespread 0"). Its blocks repeat each other's numbers (the crouched rows are the standing's, the prone's its sprint's), so they're looked for one
// after the other: the zoomed after the standing block, the crouched after the zoomed, the prone after the crouched; all there, or it isn't its data.
// Exact numbers only, read through ReadProcessMemory (plain read-write memory only: never what the game mapped for writing, AMD's slow reads).
std::atomic<uintptr_t> g_at4Shot[8] = {};
float g_at4ShotLast[std::size(g_at4Shot)][3] = {};  // what was last written at each (under g_at4ShotMutex)
std::mutex g_at4ShotMutex;                           // the writer (a draw's) and the putting back (Update's): one at a time
std::atomic<uint32_t> g_at4ShotFrame{0};             // the frame (g_frameSerial) its start was last written in; 0 none since put back
constexpr size_t kAt4Gun = 1001;  // HeldKind's spreadGun: this is no rifle; its rocket's start and its scatter are FindAt4's
constexpr float kAt4ShotStart[3] = {at4shot::kStart[0], at4shot::kStart[1], at4shot::kStart[2]};  // its data's (the SMAW's too)

// Its own FiringFunctionData's shot at place (its ShotConfigData: the firing data starts at4shot::kShotAt before it), read now: the AT4's own, its
// start as its data has it or as last (null: the data's alone).
bool At4ShotThere(uintptr_t place, const float* last) {
    uint8_t firing[at4shot::kFiringBytes];
    SIZE_T got = 0;
    return place > at4shot::kShotAt && ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(place - at4shot::kShotAt), firing, sizeof(firing), &got) &&
           got == sizeof(firing) && at4shot::FiringOurs(firing, last);
}

// The AT4 out of the hands for half a second (Update, each frame): its rocket's start back as its data has it, wherever it was written (nothing of
// the mod's left in the game's data: a scene that fires it without the mod drawing it fires it as the game would).
void RestoreAt4Shot() {
    const uint32_t frame = g_at4ShotFrame.load(std::memory_order_relaxed);
    if (!frame || g_frameSerial.load(std::memory_order_relaxed) - frame < 36)
        return;
    std::lock_guard lock(g_at4ShotMutex);
    int back = 0;
    for (size_t n = 0; n < std::size(g_at4Shot); ++n) {
        const uintptr_t place = g_at4Shot[n].load(std::memory_order_relaxed);
        if (!place || std::memcmp(g_at4ShotLast[n], kAt4ShotStart, sizeof(kAt4ShotStart)) == 0)
            continue;
        if (!At4ShotThere(place, g_at4ShotLast[n])) {
            g_at4Shot[n] = 0;
            continue;
        }
        if (WriteProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(place), kAt4ShotStart, sizeof(kAt4ShotStart), nullptr)) {
            std::memcpy(g_at4ShotLast[n], kAt4ShotStart, sizeof(kAt4ShotStart));
            ++back;
        }
    }
    g_at4ShotFrame = 0;
    if (back)
        Log("AT4: out of the hands: its rocket's start put back as its data has it (%d place%s)", back, back == 1 ? "" : "s");
}

DWORD WINAPI FindAt4(LPVOID) {
    const ScanThread scanning;  // (StartScan's: the lowest priority, one at a time)
    const ULONGLONG began = GetTickCount64();
    const HANDLE self = GetCurrentProcess();
    // Venice-EBX's M136 GunSwayData: Stand.NoZoom's BaseValue, Moving, Jumping, Sprinting, VaultingSmallObject, VaultingMediumObject (min, max, increase
    // a shot, decrease a second each) and its Recoil (most, up a shot, sideways least and most, sideways most, decrease); Stand.Zoom's BaseValue and Moving;
    // Crouch.NoZoom's (the standing's numbers); Prone.NoZoom's (its sprint's); Crouch.Zoom's (Stand.Zoom's), what the crouched rows become.
    static constexpr float kStand[30] = {1, 5, 0, 3, 3, 5, 0, 3, 7, 7, 0, 3, 0, 5.5f, 1.8f, 15, 0, 5.5f, 1.8f, 15, 0, 5.5f, 1.8f, 15, 10, 4, -1, 1, 10, 4};
    static constexpr float kZoom[8] = {0, 2, 0, 3, 2, 3, 0, 3}, kCrouch[8] = {1, 5, 0, 3, 3, 5, 0, 3}, kProne[8] = {0, 5.5f, 1.8f, 15, 0, 5.5f, 1.8f, 15};
    constexpr size_t kStandFloats = 30, kSwayFloats = 512;
    auto same = [](const float* at, const float* wanted, size_t n) {
        for (size_t k = 0; k < n; ++k)
            if (!(std::fabs(at[k] - wanted[k]) < 1e-4f))
                return false;
        return true;
    };
    // Its kick's three a shot (25-27) its own or 0: taken out of its data by recoil (design 6) while it's held, as ClearRifleSpread's standSame.
    auto standSame = [&](const float* at) {
        if (!same(at, kStand, 25) || !same(at + 28, kStand + 28, 2))
            return false;
        for (size_t k = 25; k < 28; ++k)
            if (at[k] != 0.0f && !same(at + k, kStand + k, 1))
                return false;
        return true;
    };
    std::vector<float> chunk(256 * 1024);
    const SIZE_T chunkBytes = chunk.size() * sizeof(float);
    const auto own = reinterpret_cast<uintptr_t>(chunk.data());
    std::vector<uintptr_t> shots;
    std::vector<SpreadRow> rows;
    int found = 0;
    MEMORY_BASIC_INFORMATION info;
    for (uintptr_t at = 0x10000; VirtualQuery(reinterpret_cast<LPCVOID>(at), &info, sizeof(info)) == sizeof(info);) {
        const uintptr_t end = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
        if (info.State == MEM_COMMIT && info.Protect == PAGE_READWRITE) {
            for (uintptr_t from = reinterpret_cast<uintptr_t>(info.BaseAddress); from + 128 <= end;) {
                const SIZE_T want = std::min<SIZE_T>(chunkBytes, end - from);
                SIZE_T got = 0;
                if (!ReadProcessMemory(self, reinterpret_cast<LPCVOID>(from), chunk.data(), want, &got) || got < 128)
                    break;
                const size_t floats = got / sizeof(float);
                for (size_t i = 0; i + kStandFloats <= floats; ++i) {
                    const uintptr_t place = from + i * sizeof(float);
                    if (place >= own && place < own + chunkBytes)
                        continue;
                    // (Its whole FiringFunctionData in this chunk: from 0x60 before the shot to its ReloadTime; one across a seam is seen whole in
                    // the chunk before, whose last kSwayFloats are this one's first.)
                    constexpr size_t kBefore = at4shot::kShotAt / sizeof(float), kFiringFloats = at4shot::kFiringBytes / sizeof(float);
                    if (i >= kBefore && i - kBefore + kFiringFloats <= floats && std::fabs(chunk[i] - kAt4ShotStart[0]) < 1e-4f &&
                        std::fabs(chunk[i + 1] - kAt4ShotStart[1]) < 1e-4f && std::fabs(chunk[i + 2] - kAt4ShotStart[2]) < 1e-4f &&
                        at4shot::FiringOurs(reinterpret_cast<const uint8_t*>(&chunk[i - kBefore]), nullptr) && shots.size() < 16 &&
                        std::find(shots.begin(), shots.end(), place) == shots.end())
                        shots.push_back(place);
                    if (chunk[i] != kStand[0] || !standSame(&chunk[i]))
                        continue;
                    size_t zoom = 0, crouch = 0, prone = 0;
                    for (size_t k = kStandFloats; k + 8 <= kSwayFloats && i + k + 8 <= floats; ++k) {
                        if (!zoom) {
                            if (same(&chunk[i + k], kZoom, 8))
                                zoom = k;
                        } else if (!crouch) {
                            if (k >= zoom + 8 && same(&chunk[i + k], kCrouch, 8))
                                crouch = k;
                        } else if (k >= crouch + 8 && same(&chunk[i + k], kProne, 8)) {
                            prone = k;
                            break;
                        }
                    }
                    if (!zoom || !crouch || !prone)
                        continue;
                    ++found;
                    // Each stance's unzoomed rows its own zoomed rows: the standing's after the standing block, the crouched's the same numbers. Prone
                    // is the same zoomed or not: left as it is.
                    for (const size_t k : {size_t{0}, crouch})
                        for (size_t row = 0; row < 2; ++row) {
                            SpreadRow r{place + (k + row * 4) * sizeof(float)};
                            std::memcpy(r.game, &chunk[i + k + row * 4], sizeof(r.game));
                            std::memcpy(r.wanted, &kZoom[row * 4], sizeof(r.wanted));
                            r.kind = kAt4Gun;
                            rows.push_back(r);
                        }
                }
                if (got < want || want < chunkBytes)
                    break;
                from += want - kSwayFloats * sizeof(float);  // its data across the seam is seen whole
            }
        }
        if (end <= at)
            break;
        at = end;
    }
    if (const size_t known = AddPlaces(g_at4Shot, shots); !shots.empty())
        Log("AT4: its rocket's start found in %zu places, %zu known", shots.size(), known);
    else
        Log("AT4: its rocket's start not found: the rocket starts as the game has it, from its eye (-0.3 -0.1 2.0)");
    if (found > 4) {
        Log("AT4: %d places like its scatter's, %llu ms: left alone", found, GetTickCount64() - began);
        return 0;
    }
    if (found) {
        {
            std::lock_guard lock(g_rifleSpreadMutex);
            for (const SpreadRow& r : rows)
                if (std::none_of(g_rifleSpreadRows.begin(), g_rifleSpreadRows.end(), [&](const SpreadRow& o) { return o.place == r.place; }))
                    g_rifleSpreadRows.push_back(r);
        }
        Log("AT4: its scatter's numbers found in %d places, %llu ms", found, GetTickCount64() - began);
        if (g_rifleSpread.load(std::memory_order_relaxed))
            ApplyRifleSpread(true);
    } else {
        Log("AT4: no scatter's numbers of its own left to change, %llu ms", GetTickCount64() - began);
    }
    return 0;
}

// A held kind's data its last scan found, still where it was (the audit of 2026-11-19: each swap between the M4A1, the Mk11 and the M249 scanned all of
// the game's memory again, 0.6-1.1 s of a core, to find again what was known): every row of its scatter reads as the game's numbers or as written, and
// for the AT4 a place of its rocket's start is still its own (At4ShotThere, as its writer reads it). Read with ReadProcessMemory (never a pointer the
// game maps for writing: AMD-safe). False: none of it known, or a row gone (a load put other things there): its scan again.
bool KindDataThere(size_t kind) {
    const HANDLE self = GetCurrentProcess();
    if (kind == kAt4Gun) {
        std::lock_guard lock(g_at4ShotMutex);
        for (size_t n = 0; n < std::size(g_at4Shot); ++n)
            if (const uintptr_t place = g_at4Shot[n].load(std::memory_order_relaxed); place && At4ShotThere(place, g_at4ShotLast[n]))
                return true;
        return false;
    }
    std::lock_guard lock(g_rifleSpreadMutex);
    int rows = 0;
    for (const SpreadRow& row : g_rifleSpreadRows) {
        if (row.kind != kind)
            continue;
        float now[4];
        SIZE_T got = 0;
        if (!ReadProcessMemory(self, reinterpret_cast<LPCVOID>(row.place), now, sizeof(now), &got) || got != sizeof(now) ||
            (std::memcmp(now, row.game, sizeof(now)) != 0 && std::memcmp(now, row.wanted, sizeof(now)) != 0))
            return false;
        ++rows;
    }
    return rows > 0;
}

bool IsLauncher(const float* b) {
    const float apart = Length3(b[2 * 12 + 3] - b[3], b[2 * 12 + 7] - b[7], b[2 * 12 + 11] - b[11]);
    return std::fabs(apart - kLauncherRearFromFrame) < 0.01f;
}

bool DrawLauncher(int which, ID3D11DeviceContext* ctx, UINT count, UINT start, INT base, const DrawKey& key) {
    const Upload& u = t_upload;
    if (!u.resource || u.bytes.size() != kGloveUploadSize || !PlayersOwn(u.bytes))
        return false;
    const auto* game = reinterpret_cast<const float*>(u.bytes.data());
    const ULONGLONG now = GetTickCount64();
    // Parts A and the depth pass come with bones 0 and 2. At the very end of putting it away the game parts them: taken for something else, part A
    // was left unmoved with its depth moved, the tube a big black shape for a frame (film, 2026-11-04). The tube, bone 2, is what's placed then, as
    // parts B and C are, and always once the arms' rest is known: the launcher in the hand, whatever the game does with it.
    const bool withRoots = key == kLauncherA || key == kLauncherDepth;
    const bool rootsTogether = withRoots && IsLauncher(game);
    const ULONGLONG lastDrawn = g_launcherAt.load();
    // Either says it's drawn: the depth pass comes first in a frame, and left where the game had it in the first frame of a raise it was a black shape in the left eye.
    if (rootsTogether) {
        g_launcherAt = now;
        g_launcherFrame = g_frameSerial.load(std::memory_order_relaxed);
    } else if (!DrawnLately(g_launcherAt, g_launcherFrame)) {
        return false;
    }
    if (!g_launcherOn.load(std::memory_order_relaxed))
        return false;
    // Once for each level's data: its addresses are new after a load, which a long gap without it may have been (as the rifle's), or as soon as they're seen not to be its.
    // (One scan at a time, StartScan: wanted while another is under way, it's kept and started the first frame after.)
    static bool s_clearPending = false;
    if (static std::atomic<ULONGLONG> s_cleared{0}; s_clearPending || !s_cleared.load() || now - s_cleared.load() > 600000 ||
                                                    (now - lastDrawn > 15000 && now - s_cleared.load() > 15000) ||
                                                    (g_launcherScanWanted.load() && now - s_cleared.load() > 5000)) {
        s_clearPending = ScanRunning();
        if (!s_clearPending) {
            s_cleared = now;
            g_launcherScanWanted = false;
            StartScan(ClearLauncherSpread, nullptr);
        }
    }
    thread_local std::vector<uint8_t> t_rig;
    t_rig = u.bytes;
    auto* bones = reinterpret_cast<float*>(t_rig.data());
    WorldPlacing placing;
    bool twoSided = false;  // lying, drawn with both faces (freehands.txt "twosided 1")
    // Its cases (and rockets) lying or flying (loose): each a copy of this draw with only its case's bone, where it is, every other bone on one point.
    auto drawLooseCases = [&] {
        if (!withRoots || !placing.have)
            return;
        thread_local std::vector<uint8_t> t_loose;
        std::lock_guard lock(g_looseMutex);
        for (int p = 0; p < g_loosePoseCount; ++p) {
            const loose::Pose& pose = g_loosePoses[p];
            if (pose.kind != kLooseSmawCase)
                continue;
            t_loose.assign(kGloveUploadSize, 0);
            WorldToDrawn(placing, pose.bone, g_looseOrigin, reinterpret_cast<float*>(t_loose.data()) + 1 * 12);
            if (DrawWithPalette(which, ctx, count, start, base, u.resource, t_loose.data(), kGloveUploadSize))
                g_looseDraws.fetch_add(1, std::memory_order_relaxed);
        }
    };
    {
        std::unique_lock lock(g_handsMutex);
        if (!g_handsOn)
            return false;
        skin::HandTargets t = g_hands;
        // Put away, or its hand not seen at a place (stowed): drawn nowhere, never where the game holds it (the stamps above still say it's the game's weapon, and
        // the rest its arms keep is its own). Only what isn't the launcher, on part A and its depth pass: a case or a rocket in a hand (launcherCase 2: bone 1
        // alone, where the hand has it) and the cases lying.
        if (t.stowed) {
            if (rootsTogether)
                RigFor(0);
            float inHand[12] = {};
            bool caseInHand = false;
            if (withRoots && ToArms(t, ctx)) {
                placing.Take(t);
                if ((caseInHand = t.launcherCase == 2))
                    std::memcpy(inHand, t.launcherCasePose, sizeof(inHand));
            }
            lock.unlock();
            g_stowedSkips[kStowedLauncher].fetch_add(1, std::memory_order_relaxed);
            if (caseInHand) {
                thread_local std::vector<uint8_t> t_inHand;
                t_inHand.assign(kGloveUploadSize, 0);  // every other bone on one point
                std::memcpy(reinterpret_cast<float*>(t_inHand.data()) + 1 * 12, inHand, sizeof(inHand));
                DrawWithPalette(which, ctx, count, start, base, u.resource, t_inHand.data(), kGloveUploadSize);
            }
            drawLooseCases();
            return true;
        }
        // In a hand (gun), or with none on it (launcherDrawn: free hands, flying into a hand or lying where it fell).
        if (!ToArms(t, ctx) || !(t.gun || t.launcherDrawn))
            return false;
        // With no hand on it, only if the targets came for it: not for a held gun lying (the first frame of a switch the game makes meanwhile, drawn as the
        // game has it, as any switch's first frames are).
        if (!t.gun && g_handsKind != 0 && t.arriveKind != gunprofile::kSmawId)
            return false;
        placing.Take(t);
        twoSided = !t.gun && t.launcherLying && t.lyingTwoSided;
        float frame[12], inverse[12], delta[12];
        std::memcpy(frame, game, sizeof(frame));
        if (rootsTogether)
            RigFor(0);  // its own rest back, after another gun (before it's asked about: the other gun's frozen rest left it never placed)
        if (!rootsTogether || g_rigFrozen) {
            if (!g_haveLauncherRear)
                return false;
            float back[12];
            if (!Invert(g_launcherRear, back))
                return false;
            Compose(withRoots ? game + 2 * 12 : game, back, frame);
        }
        if (!Invert(frame, inverse))
            return false;
        if (withRoots) {
            // Its colour and its depth pass both, as the rifle's: the arms' depth draws are in another view than the colour's, and with only the colour to learn from they
            // never learned, following the game's arms while the colour stayed on the rest: dark patches on the forearm through a switch (the review, 2026-11-04).
            if (rootsTogether && !g_rigFrozen) {
                Compose(inverse, game + 2 * 12, g_launcherRear);
                Compose(inverse, game + 1 * 12, g_launcherCaseRest);
                g_haveLauncherRear = true;
            }
            std::memcpy(g_launcherFrameOrig, frame, sizeof(g_launcherFrameOrig));
            g_launcherDeltaTime = now;
            g_launcherFrameView = g_viewUploads.load();
            LearnRigKept(g_launcherFrameView, inverse);
        }
        // What goes with it only while a hand holds it (gun): the game's effects moved with it, its rocket's start, its logs. Lying or flying, none (nothing fires).
        if (key == kLauncherA && t.gun) {
            g_launcherEffect.Note(t, t.launcherSeen, inverse);
            // Its rocket starts from the game's eye plus this, along the game's aim (x left, y up, z forward): made the drawn
            // muzzle, so it comes out of the tube.
            if (g_launcherSpawn.load(std::memory_order_relaxed)) {
                const float muzzle[3] = {0.0f, 0.0f, kLauncherMuzzle};
                {
                    if (float start[3]; ShotStart(t, t.launcherSeen, muzzle, start)) {
                        if (Length3(start[0], start[1], start[2]) < 3.0f) {
                            for (size_t n = 0; n < std::size(g_launcherShot); ++n) {
                                const uintptr_t place = g_launcherShot[n].load(std::memory_order_relaxed);
                                if (!place)
                                    continue;
                                // Only over its own data, as it was left: a checkpoint or a load can move it, and this memory is then something else's.
                                // Its own firing data with it, as the scan took it (LauncherShotThere: its class's, its numbers as read then), never the AT4's
                                // or an RPG-7's.
                                const bool ours = LauncherShotThere(place, g_launcherShotWho[n], g_launcherShotLast[n]);
                                if (!ours) {
                                    g_launcherShot[n] = 0;
                                    g_launcherScanWanted = true;
                                    Log("Launcher: its rocket's start at %p isn't there any more: not written, looking again", reinterpret_cast<void*>(place));
                                    continue;
                                }
                                if (WriteProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(place), start, sizeof(start), nullptr)) {
                                    std::memcpy(g_launcherShotLast[n], start, sizeof(start));
                                    // (This frame's own number, never one past it: a draw on another thread between Update's count and its putting back
                                    // would have it ahead, and the difference there wrap. 0 is "none".)
                                    const uint32_t serial = g_frameSerial.load(std::memory_order_relaxed);
                                    g_launcherShotFrame.store(serial ? serial : 1u, std::memory_order_relaxed);
                                }
                            }
                            if (static ULONGLONG s_said = 0; g_launcherLog.load(std::memory_order_relaxed) && now - s_said >= 1000) {
                                s_said = now;
                                Log("Launcher: the rocket starts at (%.2f %.2f %.2f) from the game's eye (left, up, forward)", start[0], start[1],
                                    start[2]);
                            }
                        }
                    }
                }
            }
            if (static ULONGLONG s_at = 0; g_launcherLog.load(std::memory_order_relaxed) && now - s_at >= 1000) {
                s_at = now;
                float view[12], rel[12];
                {
                    std::lock_guard fp(g_fpMutex);
                    std::memcpy(view, g_viewToArms, sizeof(view));
                }
                Compose(inverse, view, rel);
                // The game's aim, as written before any turn of the mod's: back = (sin yaw cos pitch, -sin pitch, -cos yaw cos pitch).
                float yaw = 0.0f, pitch = 0.0f, aim[3] = {};
                if (camera::GameWeaponAngles(yaw, pitch)) {
                    const float f[3] = {-std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch)};
                    float along[3];
                    for (int c = 0; c < 3; ++c)
                        along[c] = frame[c] * f[0] + frame[4 + c] * f[1] + frame[8 + c] * f[2];
                    std::memcpy(aim, along, sizeof(aim));
                }
                Log("Launcher: the view on its frame: eye (%.3f %.3f %.3f), forward (%.4f %.4f %.4f), the aim on it (%.4f %.4f %.4f), FOV "
                    "source %.1f",
                    rel[3], rel[7], rel[11], -rel[2], -rel[6], -rel[10], aim[0], aim[1], aim[2], camera::SourceFov() * 57.29578f);
            }
            static bool s_logged = false;
            if (!s_logged) {
                s_logged = true;
                Log("Launcher: first drawn in the right hand, its frame as the game has it at (%.3f %.3f %.3f)", frame[3], frame[7],
                    frame[11]);
            }
        }
        // Lying, it's placed from the world: not drawn by a draw without the world's way into it (as the rifle lying, DrawGun).
        if (!t.gun && t.launcherLying && !t.haveDrawnFromWorld)
            return true;
        Compose(t.launcherPose, inverse, delta);
        for (UINT k = 0; k < kPaletteBones; ++k)
            MoveBy(delta, bones + k * 12);
        // The parts that aren't the tube, unless the game's own reload is playing (with no hand on it, never: its reload isn't played on the ground): at rest
        // with it.
        if (withRoots && g_rigFrozen && (!t.launcherGameHands || !t.gun)) {
            std::memcpy(bones, t.launcherPose, 12 * sizeof(float));
            Compose(t.launcherPose, g_launcherCaseRest, bones + 1 * 12);
        }
        // Its case, bone 1 of part A and of their depth pass, loaded by hand (rocket.cpp): nowhere (all on one point, as a magazine out
        // of a gun), where the hand or its fall has it, or on it where it lies (its rel on the frame, with the launcher as drawn).
        if (withRoots && t.launcherCase) {
            float* onCase = bones + 1 * 12;
            if (t.launcherCase == 1)
                std::memset(onCase, 0, 12 * sizeof(float));
            else if (t.launcherCase == 3)
                Compose(t.launcherPose, t.launcherCaseRel, onCase);
            else
                std::memcpy(onCase, t.launcherCasePose, 12 * sizeof(float));
        }
    }
    {
        NoCull bothFaces(ctx, twoSided);
        if (!DrawWithPalette(which, ctx, count, start, base, u.resource, t_rig.data(), kGloveUploadSize))
            return false;
    }
    ++g_handsMovedDraws;
    drawLooseCases();
    return true;
}

// The launcher is what's in hand: drawn lately, and no rifle drawn since. (By the time alone, the arms stayed on the launcher's grips for 0.3 s
// with the rifle already up, floating in front of them: the user, 2026-11-04.)
bool LauncherDrawnLast() {
    const ULONGLONG at = g_launcherAt.load();
    return DrawnLately(g_launcherAt, g_launcherFrame) && g_rifleDrawn.load() <= at && g_heldAt.load() <= at;
}

// Any other gun is what's in hand: drawn lately, and neither the rifle nor the launcher drawn since.
bool HeldDrawnLast() {
    const ULONGLONG at = g_heldAt.load();
    return g_heldOn.load(std::memory_order_relaxed) && DrawnLately(g_heldAt, g_heldFrame) && g_rifleDrawn.load() <= at && g_launcherAt.load() <= at;
}

// The gloves, forearms, body and legs with the launcher in hand: moved as it is.
bool DrawLauncherRig(int which, ID3D11DeviceContext* ctx, UINT count, UINT start, INT base, const ArmDraw& draw) {
    const Upload& u = t_upload;
    // (Parts of the rifle, in the frame it comes back: they're the rifle's, drawn as the game had them they were long black shapes across the view.)
    if (!g_launcherOn.load(std::memory_order_relaxed) || !g_rigThisFrame.load(std::memory_order_relaxed) || draw.roles[0] == 'G' || !u.resource ||
        u.bytes.size() != kGloveUploadSize || !PlayersOwn(u.bytes))
        return false;
    thread_local std::vector<uint8_t> t_rig;
    t_rig = u.bytes;
    auto* bones = reinterpret_cast<float*>(t_rig.data());
    {
        std::lock_guard lock(g_handsMutex);
        const ULONGLONG now = GetTickCount64();
        if (!g_handsOn || (!DrawnLately(g_launcherAt, g_launcherFrame) && !DrawnLately(g_heldAt, g_heldFrame)))
            return false;
        skin::HandTargets t = g_hands;
        // In a hand (gun), or with none on it (launcherDrawn: free hands, flying into a hand or lying where it fell), or put away (stowed: drawn nowhere, the hands
        // free); without any, the game's own arms.
        if (!ToArms(t, ctx) || !(t.gun || t.launcherDrawn || t.stowed))
            return false;
        if (!PlaceOnRig(t, draw, bones, now))
            return false;
    }
    DumpArmPalette(draw.key, t_rig.data(), t_rig.size());
    if (!DrawWithPalette(which, ctx, count, start, base, u.resource, t_rig.data(), kGloveUploadSize, &draw == &kArmDraws[0]))
        return false;
    ++g_handsMovedDraws;
    return true;
}

// A launcher-pose gun in hand as this frame began, and drawn lately: the arms' draws go by its rig.
bool RigDrawnLately() {
    return g_launcherOn.load(std::memory_order_relaxed) && g_rigThisFrame.load(std::memory_order_relaxed) &&
           (DrawnLately(g_launcherAt, g_launcherFrame) || DrawnLately(g_heldAt, g_heldFrame));
}

// An arms' draw's palette (bones, as the game sent it, changed in place) placed on the gun in hand: DrawLauncherRig's own placing, under g_handsMutex, t already
// in this draw's space (ToArms). Also the bare arms' with a gun in hand, in the gloves' numbers (DrawBareArms, barehold.h): then learn says whether this pass is one
// to learn the game's hold from, forearmsDraw is that palette as the bare forearms' draw has its own (the rig's folds are learned from that draw), and placedOut
// takes which hands are drawn and where the others fold. False: nothing placed (no frame to place by).
bool PlaceOnRig(const skin::HandTargets& t, const ArmDraw& draw, float* bones, ULONGLONG now, bool learn, const float* forearmsDraw, RigPlaced* placedOut) {
    {
        float inverse[12], delta[12], deltaRight[12];
        if (!Invert(g_launcherFrameOrig, inverse)) {
            // (Put away, nothing is placed on its frame, the hands go by their own poses: a gun put away before any was drawn in a hand has none yet.)
            if (!t.stowed)
                return false;
            constexpr float kNoTurn[12] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
            std::memcpy(inverse, kNoTurn, sizeof(inverse));
        }
        Compose(t.launcherPose, inverse, delta);
        Compose(t.rightLauncherPose, inverse, deltaRight);  // (the right glove on the gun it's drawn on: the same while the right holds it)
        // Just back to it: whether its arms rest starts over.
        if (now - g_rigSeenAt > 250)
            g_rigHaveSteady = false;
        g_rigSeenAt = now;
        // On the frame, from this view's own frame or as soon as it's drawn; placed by what's known (RigSet), whichever view that was: the launcher
        // where the hand has it and each bone where the game's arms rest on it.
        if (!learn) {
        } else if (const uint32_t view = g_viewUploads.load(); g_launcherFrameView == view) {
            LearnRig(draw, bones, inverse);
            if (forearmsDraw)
                LearnRig(kArmDraws[2], forearmsDraw, inverse);
        } else {
            KeepRig(draw, bones, view);
            if (forearmsDraw)
                KeepRig(kArmDraws[2], forearmsDraw, view);
        }
        const RigSet& rig = RigInUse();
        const auto inUse = rig.bones.find(draw.key);
        const auto seenNow = g_rigNow.bones.find(draw.key);  // a draw not seen before the rest was taken
        auto placedOn = [&](const float* pose, const float* by, size_t k, float* bone) {
            if (inUse != rig.bones.end() && inUse->second.have[k])
                Compose(pose, inUse->second.rel[k], bone);
            else if (seenNow != g_rigNow.bones.end() && seenNow->second.have[k])
                Compose(pose, seenNow->second.rel[k], bone);
            else
                MoveBy(by, bone);
        };
        auto placed = [&](size_t k, float* bone) { placedOn(t.launcherPose, delta, k, bone); };
        // The left hand on its own (hands.cpp, loaded by hand, or holding the pistol grip; the gun lying; the gun put away, any left seen): placed as with the M416,
        // its arm folding where the hand is. The game's own left hand is on the gun only while the right holds its pistol grip (free hands dormant, or the right the
        // trigger hand): held by the right on its front grip alone, flying into a hand or lying, the left not the player's, no left hand is on it; put away, none.
        const bool leftFree = (t.launcherLeftFree || t.stowed) && t.left;
        const bool gameLeft = !leftFree && t.gun && t.rightOnPistol && !t.stowed;
        const bool rightOnGrip = t.rightGlove == skin::kRightOnGrip;
        // The game's left hand's own arm (hands.cpp solved it from the hand's frame on the gun, rigLeftHand): drawn with that hand placed here. With another there
        // now (a switch: the new gun's rest, taken at its first draw, in the middle of this frame) the hand placed here is moved back onto the one the arm was solved
        // from (handMove), so every draw of the frame, either eye's and each pass's, has the hand and its whole arm in the one place; on the new gun's rest from the
        // next frame. (Folded away from that draw on, the sleeve and upper arm would be in one eye's picture and not the other's for that frame.) With no hand
        // known there (a gun whose rest hasn't been taken yet) the arm folds away for the frame, as it always did.
        const float* const leftHandNow = gameLeft && t.rigLeftArm ? RigLeftHand(rig) : nullptr;
        bool gameArm = leftHandNow && std::memcmp(leftHandNow, t.rigLeftHand, sizeof(t.rigLeftHand)) == 0;
        float handMove[12] = {};
        bool handMoved = false;
        if (leftHandNow && !gameArm) {
            float solvedFrom[12], placedNow[12], back[12];
            Compose(t.launcherPose, t.rigLeftHand, solvedFrom);
            Compose(t.launcherPose, leftHandNow, placedNow);
            if (Invert(placedNow, back)) {
                Compose(solvedFrom, back, handMove);
                gameArm = handMoved = std::all_of(std::begin(handMove), std::end(handMove), [](float f) { return std::isfinite(f); });
            }
            // (Said only at a switch, the rig another gun's than the hands came for, and at most twice a second: for the films, that frame is one a burst of
            // pictures can miss. Not for a rest taken again, a millimetre off; nor with the headset idle, the hands still as they last came: it would be a line
            // twice a second all the while.)
            float palmWas[3], palmNow[3];
            TransformPoint(solvedFrom, kLeftPalm, palmWas);
            TransformPoint(placedNow, kLeftPalm, palmNow);
            const float apart = Length3(palmNow[0] - palmWas[0], palmNow[1] - palmWas[1], palmNow[2] - palmWas[2]);
            const bool switched = g_heldKind.load(std::memory_order_relaxed) != g_handsKind;
            if (static ULONGLONG s_saidAt = 0; handMoved && switched && apart > 0.005f && now - s_saidAt >= 500) {
                s_saidAt = now;
                Log("Arms: a switch's frame: the game's left hand kept on its arm, its palm %.0f mm from where the new gun's rest has it", apart * 1000.0f);
            }
        }
        float fold[2][3], mid[3];
        for (int side = 0; side < 2; ++side) {
            if (rig.haveFold)
                TransformPoint(side == 1 && rightOnGrip ? t.rightLauncherPose : t.launcherPose, rig.fold[side], fold[side]);
            else
                std::memcpy(fold[side], side ? t.rightWrist : t.leftWrist, sizeof(fold[side]));
        }
        if (leftFree) {
            float forearm[12];
            Compose(t.leftPose, t.leftFingers[5], forearm);  // as the bare forearm's elbow half is (kForearmLeft)
            TransformPoint(forearm, kForearmMiddle[0], fold[0]);
            if (t.arm[0])
                std::memcpy(fold[0], t.fold[0], sizeof(fold[0]));
        }
        if (gameArm)
            std::memcpy(fold[0], t.rigLeftFold, sizeof(fold[0]));
        // The right glove by its own pose: as the left's, its forearm's elbow half (kForearmRight: 44, the twin of the left's 5 and 4), as DrawArms has it.
        if (t.rightGlove == skin::kRightOwn) {
            float forearm[12];
            Compose(t.rightGlovePose, t.rightFingers[44 - handpose::kRightFirst], forearm);
            TransformPoint(forearm, kForearmMiddle[1], fold[1]);
        }
        if (t.arm[1])
            std::memcpy(fold[1], t.fold[1], sizeof(fold[1]));
        // The body and legs fold between the two arms' fold points with both hands drawn, else onto the one that is (DrawArms' rule). (The game's own right
        // hand only ever with a hand on the gun.)
        const bool leftHere = leftFree || gameLeft, rightHere = t.rightGlove != skin::kRightHidden && (t.gun || t.rightGlove != skin::kRightGame);
        for (int i = 0; i < 3; ++i)
            mid[i] = leftHere && rightHere ? (fold[0][i] + fold[1][i]) / 2 : leftHere ? fold[0][i] : fold[1][i];
        // As with the rifle: each arm solved is whole. The left placed by the game's rest, or by the game's own reload, too: its arm is solved from where the hand is
        // drawn on the gun (gameArm), as the right's on the grip is. (Folded away, its bare forearm ended in an open cut in mid-air: the films' review, 2026-11-11.)
        const bool wholeLeft = (leftFree && t.arm[0]) || gameArm;
        const float (*const leftArm)[12] = leftFree ? t.forearm[0] : t.rigLeftForearm;
        if (t.haveChest && (wholeLeft || t.arm[1]))
            std::memcpy(mid, t.chest, sizeof(mid));
        // A hand that isn't drawn folds where the body does (its grip point is none: a triangle mixing it with the body and a seen arm would run through
        // the eye).
        if (!rightHere)
            std::memcpy(fold[1], mid, sizeof(fold[1]));
        if (!leftHere)
            std::memcpy(fold[0], mid, sizeof(fold[0]));
        for (size_t k = 0; draw.roles[k] && k < kPaletteBones; ++k) {
            float* bone = bones + k * 12;
            const int armPart = draw.forearm && k < draw.forearmCount ? draw.forearm[k] : -1;
            switch (draw.roles[k]) {
            case 'L':
                if (leftFree) {
                    const int f = t.arm[0] && draw.forearm && k < draw.forearmCount ? draw.forearm[k] : -1;
                    if (f >= 0 && f <= 4) {
                        std::memcpy(bone, t.forearm[0][f], sizeof(t.forearm[0][f]));
                    } else if (f >= 10 && f < 10 + handpose::kBones) {
                        Compose(t.leftPose, t.leftFingers[f - 10], bone);
                    } else {
                        const int g = !draw.leftBones ? static_cast<int>(k) : k < draw.leftBoneCount ? draw.leftBones[k] : -1;
                        Compose(t.leftPose, t.leftFingers[g >= 0 && g < handpose::kBones ? g : 0], bone);
                    }
                } else if (gameLeft) {
                    // The hand and its fingers where the game's rest on the gun; its forearm's bones (the glove's cuff with them) by its solved arm, as the right's.
                    if (const int f = gameArm && draw.forearm && k < draw.forearmCount ? draw.forearm[k] : -1; f >= 0 && f <= 4) {
                        std::memcpy(bone, t.rigLeftForearm[f], sizeof(t.rigLeftForearm[f]));
                    } else {
                        placed(k, bone);
                        if (handMoved)
                            MoveBy(handMove, bone);  // (a switch's frame: back onto the hand its arm was solved from)
                    }
                } else {
                    Collapse(bone, fold[0]);  // no game left hand hanging over a gun lying, nor on the front grip the right glove holds
                }
                break;
            case 'R':
                // The right glove (skin::RightGlove): on the gun it's drawn on, by its own pose, or folded away.
                if (!rightHere) {
                    Collapse(bone, fold[1]);
                } else if (const int f = t.arm[1] && draw.forearm && k < draw.forearmCount ? draw.forearm[k] : -1; f >= 0 && f <= 4) {
                    std::memcpy(bone, t.forearm[1][f], sizeof(t.forearm[1][f]));
                } else if (rightOnGrip) {
                    placedOn(t.rightLauncherPose, deltaRight, k, bone);
                } else if (t.rightGlove == skin::kRightOwn) {
                    Compose(t.rightGlovePose, t.rightFingers[RightGloveBone(draw, k) - handpose::kRightFirst], bone);
                } else {
                    placed(k, bone);  // (kRightGame: only with a hand on it, as before)
                }
                break;
            case 'l':
                if (wholeLeft && armPart >= 0 && armPart <= 4)
                    std::memcpy(bone, leftArm[armPart], sizeof(leftArm[armPart]));
                else
                    Collapse(bone, fold[0]);
                break;
            case 'r':
                if (t.arm[1] && armPart >= 0 && armPart <= 4)
                    std::memcpy(bone, t.forearm[1][armPart], sizeof(t.forearm[1][armPart]));
                else
                    Collapse(bone, fold[1]);
                break;
            case 'c':
                Collapse(bone, mid);
                break;
            }
        }
        // The trigger finger pulled with the right trigger, on the grip as the game holds it (the free glove's index comes with its own fingers).
        if (const float pull = input::RightTrigger(); rightOnGrip && pull > 0.01f) {
            auto* palette = reinterpret_cast<float(*)[12]>(bones);
            const float degrees = pull * kTriggerBend;
            if (&draw == &kArmDraws[0])
                handpose::BendRightIndex(palette, degrees);
            else if (&draw == &kArmDraws[1])
                handpose::BendRightIndex(palette, degrees, handpose::kGloveDepthRight, std::size(handpose::kGloveDepthRight));
            else if (&draw == &kArmDraws[4])
                handpose::BendRightIndex(palette, degrees, handpose::kBodyDepthRight, std::size(handpose::kBodyDepthRight));
        }
        if (placedOut) {
            placedOut->here[0] = leftHere, placedOut->here[1] = rightHere;
            std::memcpy(placedOut->fold, fold, sizeof(placedOut->fold));
        }
    }
    return true;
}

// ---- Any other gun: what a dead enemy dropped, and every gun the mod has no model of. The user, 2026-11-05: "study the way they're held, and study the way that they reload the gun on the
// original game... use that for a base to convert to VR." So, as the launcher: its frame bone goes where the right controller holds it, every bone of it and of the arms moves by that one
// move, and the game's own hold and reload stay as they are, whichever gun it is. Its draws aren't known beforehand: what's drawn in the first-person pass (stencil 32) with the player's own
// bones, not the arms and no gun the mod knows. Its body is the biggest of them at the start of its buffer, in colour; its depth pass the one drawn from as many indices in as it has.
// With no magazine button and measured (handmag.h), its magazine's bones: in the colour pass, its rounds' draw, the depth pass, and the game's spare copy of it, parked a metre behind
// the gun until its reload, which is never to be seen with the gun out in the hand; its lever's.
struct HeldMagBones {
    const handmag::Gun* gun;
    int colour, spareColour;
    UINT roundsCount, roundsStart;
    int rounds;
    int depth[2], spareDepth;
    int lever;
    float boxMin[3], boxMax[3];  // the magazine in its own vertices' space
    UINT partCount = 0, partStart = 0;  // another piece of it, drawn apart, on its own palette's bone 0 (the AK-74M's)
    int leverDepth = -1;                // its lever in the depth pass, if that pass's bones are in another order (the M4A1's, the M240's)
    UINT beltCount = 0, beltStart = 0;  // a belt out of it: gone with it and back with the next (the M240's), first this many bones in the depth pass
    int beltDepthBones = 0;
    // Its ejection port's cover, an M16's or an M4's (the user, 2026-11-05: "everything that M416 has"): the mesh has it hanging open under the port, on a hinge along z here on the frame (x, y); shut it's swung up over the port (kPortCoverShut).
    // On a bone of its own, in each pass (the M4A1's); or a run of the body's own indices, on the frame bone, in the colour pass and, as far in, in the depth pass (the M16A4's). The game only ever shows it hanging open, so the mesh has only
    // the one side, that faces out then: shut, turned over, that side faces into the gun, its back isn't drawn, and it's gone but for its edges (the user: "once it closes, it disappears"). So it's drawn twice, each with its own side out.
    int cover = -1, coverDepth = -1;
    float coverHinge[2] = {};
    UINT coverCount = 0, coverStart = 0, coverDepthStart = 0;
    int lid = -1, lidDepth = -1;  // a belt fed gun's lid, in the colour and depth passes
    int partBone = 0;             // the magazine's bone in that other piece's own palette, when the piece has more on another bone (the Mk11's: its bone 0 is the frame's)
};
struct HeldKind {
    UINT bodyIndices;   // its colour pass's index count
    int frameBone;      // rigid, the model's axes (x left, y up, z forward), the same bone in the depth pass: the recordings (reloadrec "allrec")
    float up;           // degrees its sights' line lies below z: it's raised that much, so they look along the aim
    float sights[3];    // that line at its muzzle's distance: where its bullets start
    float muzzle[3];    // its muzzle, for the flash
    float port[3];      // its ejection port, for the casings
    float rightPalm[3], leftPalm[3];  // where the game's palms rest on it (measured)
    const HeldMagBones* mag;
    UINT depthIndices;  // its depth pass's count, from that start too
    size_t spreadGun;   // its place in ClearRifleSpread's table
    const char* shape;  // its collision shape: guns\<this>.bin (tools/gunshape.exe)
    UINT bodyStart = 0; // where its colour pass starts in the buffer, if something else comes first (the M16A4's rounds)
    int depthFrame = -1; // its frame bone in the depth pass, if that pass's bones are in another order
    UINT depthStart = 0; // where its depth pass starts, if not as many indices in as it has
    float right = 0.0f;  // degrees its sights' line lies right of z (toward -x): turned that much too, so they look along the aim (only the AT4's isn't 0;
                         // with 0 every place that turns a gun by its up is as before, bit for bit: hands.cpp's SightsTurn)
};
// The AN-94 (reloadrec and the mesh, 2026-11-05): the seated magazine on its frame x +-1.3 cm, y -16.2 to 3.3 cm, z 34.8 to 47.0 cm, curving forward; its lever's knob on the right.
constexpr handmag::Gun kAn94Mag{{0.0f, 0.033f, 0.386f}, {0.0f, -0.075f, 0.410f}, {0.0f, -0.012f, 0.340f}, {0.0f, 0.005f, 0.424f}, -0.05f, {-0.034f, 0.068f, 0.495f}, 0.113f, 25.0f * 3.14159265f / 180.0f, {0.0f, -0.008f, 0.010f}, magposes::kAn94Hold, magposes::kAn94Fingers, false, false, {{0.023f, 0.348f}, {-0.065f, 0.365f}, {-0.152f, 0.404f}}, magposes::kAn94LeverHold, magposes::kAn94LeverFingers, false, {},0.0f, {}, 0.0f, nullptr, magposes::kAn94LeverUnderHold, magposes::kAn94LeverUnderFingers};
constexpr HeldMagBones kAn94Bones{&kAn94Mag, 4, 5, 312, 16278, 0, {4, 6}, 5, 2, {-0.013f, -0.266f, 0.070f}, {0.013f, -0.071f, 0.192f}};
// The RPK-74M: its body is bone 3, bone 0 its lever; the long magazine on its frame x +-1.8 cm, y -21.7 to 1.3 cm, z 37.1 to 58.2 cm, curving forward. No spare copy: its own reload pulls the one off
// and brings it back.
constexpr handmag::Gun kRpkMag{{0.0f, 0.013f, 0.414f}, {0.0f, -0.10f, 0.46f}, {0.0f, -0.03f, 0.36f}, {0.0f, -0.01f, 0.45f}, -0.05f, {-0.05f, 0.04f, 0.48f}, 0.10f, 25.0f * 3.14159265f / 180.0f, {0.0f, -0.015f, 0.06f}, magposes::kRpkHold, magposes::kRpkFingers, false, false, {{0.001f, 0.378f}, {-0.102f, 0.414f}, {-0.205f, 0.524f}}, magposes::kRpkLeverOverHold, magposes::kRpkLeverOverFingers, false, {},0.0f, {}, 0.08f, nullptr, magposes::kRpkLeverUnderHold, magposes::kRpkLeverUnderFingers};
constexpr HeldMagBones kRpkBones{&kRpkMag, 1, -1, 312, 24492, 0, {1, 4}, -1, 0, {-0.018f, -0.307f, 0.061f}, {0.018f, -0.077f, 0.272f}};
// The AEK-971 (reloadrec and the mesh): knocks its magazine out in its own reload, as the AN-94: bone 3 is the spare that flies off, parked behind the gun till then. Its lever's knob on the right.
constexpr handmag::Gun kAek971Mag{{0.0f, 0.014f, 0.415f}, {0.0f, -0.088f, 0.436f}, {0.0f, -0.031f, 0.370f}, {0.0f, -0.014f, 0.453f}, -0.05f, {-0.031f, 0.054f, 0.451f}, 0.092f, 25.0f * 3.14159265f / 180.0f, {0.0f, 0.0f, 0.0f}, magposes::kAek971Hold, magposes::kAek971Fingers, false, false, {{0.005f, 0.378f}, {-0.082f, 0.395f}, {-0.169f, 0.433f}}, magposes::kAek971LeverHold, magposes::kAek971LeverFingers, false, {},0.0f, {}, 0.0f, nullptr, magposes::kAek971LeverUnderHold, magposes::kAek971LeverUnderFingers};
constexpr HeldMagBones kAek971Bones{&kAek971Mag, 0, 3, 318, 21615, 0, {0, 5}, 3, 2, {-0.013f, -0.282f, 0.068f}, {0.013f, -0.089f, 0.189f}};
// The AK-74M: its magazine in two draws, the rounds a third; pulled off and brought back in its own reload, no spare. The long bar along its right side is its lever, the knob at its front.
constexpr handmag::Gun kAk74Mag{{0.0f, -0.002f, 0.417f}, {0.0f, -0.102f, 0.438f}, {0.0f, -0.047f, 0.372f}, {0.0f, -0.030f, 0.455f}, -0.05f, {-0.037f, 0.036f, 0.497f}, 0.099f, 25.0f * 3.14159265f / 180.0f, {0.0f, 0.0f, 0.0f}, magposes::kAk74Hold, magposes::kAk74Fingers, false, false, {{-0.012f, 0.380f}, {-0.099f, 0.396f}, {-0.187f, 0.435f}}, magposes::kAk74LeverHold, magposes::kAk74LeverFingers, false, {},0.0f, {}, 0.0f, nullptr, magposes::kAk74LeverUnderHold, magposes::kAk74LeverUnderFingers};
constexpr HeldMagBones kAk74Bones{&kAk74Mag, 3, -1, 312, 21723, 0, {3, 4}, -1, 1, {-0.013f, -0.287f, 0.070f}, {0.013f, -0.092f, 0.191f}, 894, 22035};
// The G3A3: a straight wide magazine, held as the M416's (the fit starts from that hold), rocked in only a little. Its cocking handle is on its LEFT, far forward over the handguard: 17 cm of travel in its reload.
constexpr handmag::Gun kG3Mag{{0.0f, 0.033f, 0.481f}, {0.0f, -0.055f, 0.450f}, {0.0f, -0.012f, 0.429f}, {0.0f, 0.005f, 0.525f}, -0.05f, {0.022f, 0.078f, 0.770f}, 0.17f, 10.0f * 3.14159265f / 180.0f, {0.0f, 0.0f, 0.0f}, magposes::kG3a3Hold, magposes::kG3a3Fingers, false, false, {{0.026f, 0.438f}, {-0.045f, 0.441f}, {-0.115f, 0.460f}}, magposes::kG3a3LeverHold, magposes::kG3a3LeverFingers, false, {},53.0f * 3.14159265f / 180.0f, {0.001f, 0.738f}};
constexpr HeldMagBones kG3Bones{&kG3Mag, 1, -1, 0, 0, 0, {1, -1}, -1, 3, {-0.016f, -0.240f, 0.058f}, {0.015f, -0.084f, 0.149f}};
// The KH2002: a bullpup with a magazine button: its magazine behind its grip, its charging handle a hook on top, under the carrying handle.
constexpr handmag::Gun kKh2002Mag{{0.0f, -0.010f, 0.159f}, {0.0f, -0.060f, 0.150f}, {0.0f, -0.055f, 0.123f}, {0.0f, -0.038f, 0.187f}, -0.05f, {0.0f, 0.058f, 0.392f}, 0.085f, 0.0f, {0.0f, 0.0f, 0.0f}, magposes::kKh2002Hold, magposes::kKh2002Fingers, true, false, {}, magposes::kKh2002LeverHold, magposes::kKh2002LeverFingers};
constexpr HeldMagBones kKh2002Bones{&kKh2002Mag, 1, -1, 312, 15237, 0, {1, 4}, -1, 2, {-0.013f, -0.189f, 0.068f}, {0.013f, -0.088f, 0.123f}};
// The M16A4 and the M4A1: the M416's family: magazine buttons, straight in; the T handle at the back of the receiver is the lever, pulled with its two fingers; a bolt that locks back, slapped home on the left side; the port's cover. Their
// rounds come first in the buffer. Flat tops: no iron sights of their own.
constexpr float kPortCoverShut = 195.0f * 3.14159265f / 180.0f;  // from hanging straight down to lying on the receiver's side, which leans in about 15 degrees there (the mesh)
constexpr handmag::Gun kM16Mag{{0.0f, 0.016f, 0.451f}, {0.0f, -0.136f, 0.464f}, {0.0f, -0.029f, 0.407f}, {0.0f, -0.012f, 0.487f}, -0.075f, {0.0f, 0.05f, 0.292f}, 0.08f, 0.0f, {0.0f, 0.0f, 0.0f}, magposes::kM16a4Hold, magposes::kM16a4Fingers, true, false, {}, magposes::kM16a4LeverHold, magposes::kM16a4LeverFingers, true, {0.03f, -0.015f, 0.406f}};
constexpr HeldMagBones kM16Bones{&kM16Mag, 2, -1, 312, 0, 0, {0, 1}, -1, 0, {-0.017f, -0.322f, 0.076f}, {0.017f, -0.093f, 0.173f}, 0, 0, 2, 0, 0, 0, -1, -1, {-0.0175f, 0.0193f}, 204, 2949, 24393};
constexpr handmag::Gun kM4Mag{{0.0f, 0.016f, 0.451f}, {0.0f, -0.133f, 0.461f}, {0.0f, -0.029f, 0.407f}, {0.0f, -0.012f, 0.487f}, -0.077f, {0.0f, 0.05f, 0.292f}, 0.08f, 0.0f, {0.0f, 0.0f, 0.0f}, magposes::kM4a1Hold, magposes::kM4a1Fingers, true, false, {}, magposes::kM4a1LeverHold, magposes::kM4a1LeverFingers, true, {0.03f, -0.015f, 0.406f}};
constexpr HeldMagBones kM4Bones{&kM4Mag, 3, -1, 312, 0, 0, {0, 4}, -1, 0, {-0.013f, -0.313f, 0.076f}, {0.013f, -0.093f, 0.172f}, 0, 0, 1, 0, 0, 0, 1, 2, {-0.0175f, 0.0177f}};
// The M240: belt fed. Its ammunition box by hand as a magazine, no button, nothing to knock it out with: the box pulled off, a fresh one up into its place; the belt goes and comes with the box; its lever on the right, 13 cm.
constexpr handmag::BeltGun kM240Belt{{0.068f, 0.568f}, 103.0f * 3.14159265f / 180.0f, {0.048f, 0.087f, 0.335f}, magposes::kM240LidHold, magposes::kM240LidFingers, magposes::kM240BeltHanging, magposes::kM240BeltLaid, {0.4884f, 1.0245f, 0.2538f}, magposes::kBeltPinchFingers, {0.012f, 0.0535f, 0.519f}, {0.085f, -0.035f, 0.52f},
    {{-0.001f, -0.0445f, 0.025f, -0.0445f}, {0.0f, -0.0625f, 0.027f, -0.0625f}, {0.0f, -0.0765f, 0.024f, -0.0765f}, {0.005f, -0.079f, 0.005f, -0.105f}, {-0.003f, -0.088f, -0.003f, -0.143f}}, 0.308f,
    {0.197f, -0.05f, 0.514f}};  // (heldtest 7's pinch, as filmed)
constexpr handmag::Gun kM240Mag{{0.0f, -0.022f, 0.508f}, {0.095f, -0.09f, 0.50f}, {0.0f, -0.067f, 0.438f}, {0.0f, -0.05f, 0.57f}, -0.05f, {-0.062f, -0.002f, 0.442f}, 0.129f, 0.0f, {0.0f, 0.0f, 0.0f}, magposes::kM240Hold, magposes::kM240Fingers, false, true, {}, magposes::kM240LeverOverHold, magposes::kM240LeverOverFingers, false, {},0.0f, {}, 0.0f, &kM240Belt, magposes::kM240LeverUnderHold, magposes::kM240LeverUnderFingers};
constexpr HeldMagBones kM240Bones{&kM240Mag, 4, -1, 60, 2304, 0, {5, -1}, -1, 2, {-0.124f, -0.247f, 0.023f}, {0.09f, -0.098f, 0.147f}, 0, 0, 8, 2304, 0, 5, -1, -1, {}, 0, 0, 0, 5, 10};
// The AKS-74u (Semper Fidelis, measured 2026-11-18; docs/research/aks74u.md): the AK-74M's short sister. Its magazine in two draws (bone 3 of the body, and 894 more on that bone), the rounds a
// third; rocked off forward and brought back in its own reload, no spare. Its bolt and the bolt's handle on its right are bone 0, which the game moves as it fires (9.4 cm); its own empty reload
// reaches over the top and draws it 6.4 to 6.8 cm.
constexpr handmag::Gun kAks74uMag{{0.0f, 0.011f, 0.410f}, {0.0f, -0.092f, 0.430f}, {0.0f, -0.034f, 0.365f}, {0.0f, -0.017f, 0.447f}, -0.05f, {-0.037f, 0.036f, 0.4715f}, 0.094f, 25.0f * 3.14159265f / 180.0f, {0.0f, 0.0f, 0.0f}, magposes::kAks74uHold, magposes::kAks74uFingers, false, false, {{0.001f, 0.378f}, {-0.084f, 0.390f}, {-0.169f, 0.428f}}, magposes::kAks74uLeverHold, magposes::kAks74uLeverFingers, false, {},0.0f, {}, 0.0f, nullptr, magposes::kAks74uLeverUnderHold, magposes::kAks74uLeverUnderFingers};
constexpr HeldMagBones kAks74uBones{&kAks74uMag, 3, -1, 312, 18957, 0, {3, 4}, -1, 0, {-0.0144f, -0.278f, 0.077f}, {0.0146f, -0.088f, 0.196f}, 894, 18063};
// The UMP-45 (Semper Fidelis, measured 2026-11-18; docs/research/ump45.md): a straight box magazine in a deep well (5.4 cm of it inside), no button: drawn straight down by the hand, as its own
// reload does (22 cm down, then away). Its cocking handle on its LEFT over the handguard, the HK way: its own empty reload draws it 8.3 cm back, turns it 52 degrees up into its notch (about the
// gun's z through x 0, y 0.0564; it rests 6.6 cm back there) while the magazine is changed, then knocks it down. Its rounds come first in the buffer; its depth pass orders its bones its own way
// (0 the rounds, 1 the magazine, 2 the handle, 3 the frame). No knock: a fresh magazine doesn't strike this one out (its own reload never does, the well is deep and straight, and the
// user's list of guns for that has no UMP: asked in the catalogue's decisions). Its back edge, were it wanted: {y, z} (0.017 0.417), (-0.093 0.436), (-0.2025 0.4565).
constexpr handmag::Gun kUmp45Mag{{0.0f, 0.027f, 0.436f}, {0.0f, -0.120f, 0.461f}, {0.0f, -0.018f, 0.408f}, {0.0f, -0.001f, 0.456f}, -0.054f, {0.026f, 0.061f, 0.5715f}, 0.083f, 0.0f, {0.0f, 0.0f, 0.0f}, magposes::kUmp45Hold, magposes::kUmp45Fingers, false, true, {}, magposes::kUmp45LeverHold, magposes::kUmp45LeverFingers, false, {},0.0f, {}, 0.0f, nullptr, nullptr, nullptr, true, 51.8f * 3.14159265f / 180.0f, {0.0f, 0.0564f}, 0.0655f};
constexpr HeldMagBones kUmp45Bones{&kUmp45Mag, 0, -1, 234, 0, 0, {1, 0}, -1, 1, {-0.019f, -0.309f, 0.074f}, {0.019f, -0.067f, 0.127f}, 0, 0, 2};
// The Mk11 Mod 0 (Operation Swordbreaker, measured 2026-11-18; docs/research/mk11.md): an AR-10, so the M16 family's block: a magazine button, the magazine straight down and up (its own
// reload: 0.4 s down, the fresh one seated 1.2 s after), the T handle at the back of the receiver (its ears at y 0.046 to 0.0565, z 0.269 to 0.278), a bolt that locks back on an empty
// magazine and is sent home at the catch on the receiver's left (the game's own empty reload: the left hand under it 1.9 to 2.3 s, its thumb's tip 1.3 cm from the paddle; the paddle on the mesh out to x 0.024,
// y -0.010 to 0.020, z 0.397 to 0.406). Its magazine is short (10 rounds): 9.6 cm, of which 3.2 cm in the well (the receiver's underside there y -0.058). Its buffer begins with a draw of two bones (1902/0:
// bone 0 the frame's, a plate at the stock's neck; bone 1 the magazine's, the sleeve round its bottom), then its rounds (312/1902, on the magazine), then its body. Its depth pass has them all
// (14670 = 1902 + 312 + 12456): bone 0 the frame, 1 and 2 the magazine, 3 the trigger, 4 the port's cover.
// Not as the M4A1's: no bone for its T handle (it's the frame's, so no lever bone here: the hand draws it, and it's drawn moving with the hand apart from the body, its runs
// in handleruns.h), and its port's cover is drawn shut by the game, on a bone that never moves, so it's left as it is (no cover fields).
constexpr handmag::Gun kMk11Mag{{0.0f, -0.026f, 0.450f}, {0.0f, -0.090f, 0.450f}, {0.0f, -0.071f, 0.400f}, {0.0f, -0.054f, 0.492f}, -0.032f, {0.0f, 0.051f, 0.274f}, 0.08f, 0.0f, {0.0f, 0.0f, 0.0f}, magposes::kMk11Hold, magposes::kMk11Fingers, true, false, {}, magposes::kMk11LeverHold, magposes::kMk11LeverFingers, true, {0.024f, 0.005f, 0.401f}};
constexpr HeldMagBones kMk11Bones{.gun = &kMk11Mag, .colour = 3, .spareColour = -1, .roundsCount = 312, .roundsStart = 1902, .rounds = 0, .depth = {1, 2}, .spareDepth = -1, .lever = -1,
                                  .boxMin = {-0.015f, -0.189f, 0.023f}, .boxMax = {0.013f, -0.093f, 0.107f}, .partCount = 1902, .partStart = 0, .partBone = 1};
// The M249 (Operation Swordbreaker, measured 2026-11-18; docs/research/m249.md): belt fed, the M240's block on its own numbers, its rows in m249rows.h (which
// tools/m249test.cpp holds to its dumps and recordings). Its own empty reload: the right hand draws the handle 12.9 cm (0.75-0.88 s) and pushes it home
// (1.13-1.25); the left takes the cover's back end (2.25) and lifts it 102.6 degrees about x through (y 0.0575, z 0.4751) (2.44-2.75); the box slid 8 cm to
// the gun's left at its seat's height (3.14-3.19), then left and down (to 3.44), the same box back in from the left at the seat's height, turned 5.5
// degrees, settling (4.19-4.47); the belt hanging out of it taken and laid in the tray (4.63-5.19); the cover slapped shut (5.69-5.81). By hand as the
// M240's: the cover up, the box off along its own way (only with the cover up) and taken anywhere along it, a fresh one home by itself along its way on,
// the belt pinched and laid, the cover shut, the handle over or under; nothing knocks the box out. Its handle's knob on the right (x -0.076 to -0.069, y
// 0.015 to 0.027, z 0.462 to 0.481).
constexpr HeldMagBones kM249Bones{.gun = &m249rows::kMag, .colour = -1, .spareColour = -1, .roundsCount = 0, .roundsStart = 0, .rounds = 0, .depth = {m249rows::kDepthBox, -1},
                                  .spareDepth = -1, .lever = m249rows::kHandleBone,
                                  .boxMin = {m249rows::kBoxMin[0], m249rows::kBoxMin[1], m249rows::kBoxMin[2]}, .boxMax = {m249rows::kBoxMax[0], m249rows::kBoxMax[1], m249rows::kBoxMax[2]},
                                  .partCount = m249rows::kBoxCount, .partStart = m249rows::kBoxStart, .leverDepth = m249rows::kDepthHandle, .beltCount = m249rows::kBeltCount,
                                  .beltStart = m249rows::kBeltStart, .beltDepthBones = m249rows::kDepthBeltBones, .lid = m249rows::kCoverBone, .lidDepth = m249rows::kDepthCover,
                                  .partBone = m249rows::kBoxBone};
constexpr HeldKind kHeldKinds[] = {
    // Zoomed, the view on its frame is at (0 0.086 0.126) looking straight along z.
    {24492, 3, 0.0f, {0.0f, 0.086f, 1.12f}, {0.0f, 0.06f, 1.12f}, {-0.015f, 0.05f, 0.30f}, {-0.032f, -0.080f, 0.192f}, {-0.001f, -0.029f, 0.585f}, &kRpkBones, 24840, 2, "rpk"},
    // Zoomed, the view on its frame is at (0 0.092 0.127) looking straight along z: its sights' line.
    {16188, 0, 0.0f, {0.0f, 0.092f, 0.95f}, {0.0f, 0.072f, 0.95f}, {-0.017f, 0.06f, 0.33f}, {-0.023f, -0.059f, 0.192f}, {0.002f, -0.018f, 0.561f}, &kAn94Bones, 16590, 1, "an94"},
    // Zoomed, the view on its frame is at (0 0.095 0.127) looking 0.21 degrees down.
    {21585, 4, 0.21f, {0.0f, 0.092f, 0.99f}, {0.0f, 0.033f, 0.99f}, {-0.017f, 0.05f, 0.36f}, {-0.024f, -0.079f, 0.208f}, {0.004f, -0.031f, 0.551f}, &kAek971Bones, 21933, 3, "aek971"},
    // At (0 0.070 0.127), looking straight along z.
    {21723, 2, 0.03f, {0.0f, 0.0695f, 0.936f}, {0.0f, 0.023f, 0.936f}, {-0.017f, 0.045f, 0.40f}, {-0.032f, -0.080f, 0.193f}, {0.001f, -0.031f, 0.548f}, &kAk74Bones, 22995, 4, "ak74"},
    // At (0 0.109 0.126), looking 0.48 degrees down.
    {23064, 0, 0.48f, {0.0f, 0.1015f, 1.03f}, {0.0f, 0.047f, 1.03f}, {-0.02f, 0.07f, 0.42f}, {-0.034f, -0.082f, 0.241f}, {0.020f, -0.007f, 0.616f}, &kG3Bones, 23064, 5, "g3a3"},
    // At (0 0.083 0.122), looking 0.47 degrees up. Its port is far back, at the shooter's cheek.
    {15237, 3, -0.47f, {0.0f, 0.088f, 0.711f}, {0.0f, 0.005f, 0.711f}, {-0.027f, 0.012f, 0.15f}, {-0.025f, -0.099f, 0.234f}, {0.016f, -0.059f, 0.447f}, &kKh2002Bones, 15549, 6, "kh2002"},
    // At (0 0.097 0.134) and (0 0.096 0.134), looking straight along z.
    {17013, 4, 0.0f, {0.0f, 0.097f, 1.046f}, {0.0f, 0.028f, 1.046f}, {-0.018f, 0.02f, 0.43f}, {-0.054f, -0.123f, 0.256f}, {0.014f, -0.041f, 0.636f}, &kM16Bones, 18675, 8, "m16a4", 312},
    {22194, 2, 0.0f, {0.0f, 0.096f, 0.909f}, {0.0f, 0.027f, 0.909f}, {-0.018f, 0.02f, 0.43f}, {-0.054f, -0.114f, 0.255f}, {0.012f, -0.041f, 0.612f}, &kM4Bones, 22506, 9, "m4a1", 312, 3},
    // At (0 0.087 0.124), looking 0.22 degrees down.
    // (Its casings and links come out of the far end of the feed's hole, on its right: "It's kind of like shooting out under it", the user, 2026-11-05.)
    {27588, 3, 0.22f, {0.0f, 0.083f, 1.197f}, {0.0f, 0.03f, 1.197f}, {-0.035f, 0.055f, 0.505f}, {-0.023f, -0.115f, 0.196f}, {0.018f, -0.065f, 0.678f}, &kM240Bones, 29952, 7, "m240", 2364, 9},
    // The Stinger: the tube along z, its grip under it at z 0.6, its sight on bone 1, its aerial's panels on 3 to 6. Reloaded by the game: nothing comes off it. Zoomed it's another model, which isn't used here.
    {20298, 0, 0.0f, {0.0f, 0.055f, 1.174f}, {0.0f, 0.055f, 1.174f}, {0.0f, 0.055f, 0.5f}, {-0.046f, -0.132f, 0.593f}, {0.025f, -0.146f, 0.993f}, nullptr, 20298, kStingerGun, "fim92", 354, -1, 20652},
    // The AKS-74u: at (0 0.074 0.127), looking 0.42 degrees up (its front post stands 1.7 mm higher than its rear notch, 23 cm on). Its port where its shell effect is in its data
    // (FX_ShellEjection_M_Russian at (-0.014 0.039 0.411)); its palms from the train's bare arms as the gloves' bones (tools/baregloves.ps1, holdpic): the right the AK-74M's own to a millimetre.
    {17991, 2, -0.42f, {0.0f, 0.0783f, 0.710f}, {0.0f, 0.018f, 0.710f}, {-0.014f, 0.039f, 0.411f}, {-0.031f, -0.080f, 0.193f}, {0.003f, -0.035f, 0.517f}, &kAks74uBones, 19269, 10, "aks74u"},
    // The UMP-45: at (0 0.097 0.080), looking 0.05 degrees down. Its port where its shell effect is in its data (FX_ShellEjection_45cal at (-0.02 0.034 0.42), the hole in its right side);
    // its palms from the train's bare arms.
    {19314, 2, 0.05f, {0.0f, 0.0965f, 0.648f}, {0.0f, 0.033f, 0.648f}, {-0.020f, 0.034f, 0.420f}, {-0.053f, -0.066f, 0.223f}, {0.004f, -0.026f, 0.538f}, &kUmp45Bones, 19638, 11, "ump45", 234, 3},
    // The M1014 (Semper Fidelis, measured 2026-11-18: build\survey\M1014_*, mesh_13377_0_*): a tube shotgun, no magazine: its shells go in by hand (kHeldTubes,
    // shellload.h), so no HeldMagBones. Frame bone 3 in both passes; bone 0 its bolt and handle, 1 its trigger, 2 its fore-end (still). Zoomed (FOV 40) the view
    // on its frame is at (0 0.139 0.198); its sight's three dots (324/13377, on the frame) lie on y 0.139 from z 0.348 to 0.970: the line straight along z
    // (the one zoomed sample looked 0.78 degrees up: taken as sway). Its muzzle's middle (0 0.109 1.019); its ejection port on its right, z 0.44 to 0.525.
    {13377, 3, 0.0f, {0.0f, 0.139f, 1.019f}, {0.0f, 0.109f, 1.019f}, {-0.017f, 0.110f, 0.48f}, {-0.038f, -0.030f, 0.217f}, {-0.017f, 0.020f, 0.660f}, nullptr, 14151, 12, "m1014"},
    // The Mk11 Mod 0 (Operation Swordbreaker, measured 2026-11-18: build\survey\MK11*, mesh_12456_2214_*): a flat top with the rifle scope (kHeldSights). Its body starts 2214
    // indices in (after the piece of two bones and the rounds), frame bone 0 in both passes. Zoomed (FOV 8.7) the view is in the scope's bone at (0.0002 0.0369 -0.2437) looking
    // along z (0.12 degrees off each way in the one sample: its sway; the reticle's own middle is at (0 0.0370)), and the scope's bone 0 is at (0 0.0552 0.4417) on the frame in
    // every frame of its recordings, its axes the frame's: the line at y 0.0922, the eye at z 0.198. Its muzzle's middle (0 0.026 1.004); its port where its shell effect is in its
    // data (FX_ShellEjection_762 at (-0.020 0.023 0.446): at the cover, x -0.016 to -0.024, y 0.018 to 0.042, z 0.400 to 0.493); its palms the M4A1's to a millimetre.
    {12456, 0, 0.0f, {0.0f, 0.0922f, 1.004f}, {0.0f, 0.026f, 1.004f}, {-0.020f, 0.023f, 0.446f}, {-0.054f, -0.114f, 0.254f}, {0.012f, -0.041f, 0.612f}, &kMk11Bones, 14670, 13, "mk11", 2214},
    // The M249 (Operation Swordbreaker, measured 2026-11-18: build\survey\M249*, mesh_24291_2808_*, 2808_0, 987_27099; its numbers in m249rows.h): its body 2808
    // indices in (after its belt), frame bone 3, its depth pass's frame bone 8. Its iron sights: the rear aperture on the feed cover (its hole's middle (0 0.0927),
    // 8 mm across, from z 0.206 to 0.2203), the front post in its hood (its top y 0.0845 at z 0.813): the line through them lies 0.78 degrees below z and meets the
    // muzzle's plane at y 0.083. (The game's one zoomed sample, FOV 40: the eye at (0 0.093 0.088) looking 0.68 degrees down, on the post's top and 1.1 mm low in
    // the hole: the same point at the muzzle.) Its muzzle's middle (0 0.026 0.935); its port where its shell effect is in its data (FX_ShellEjection_M at
    // (-0.0304 0.0419 0.4217), the M416's own effect; its links' FX_BeltLinkEjection_M at (-0.035 0.042 0.42), left to the game); its palms at rest from its
    // recordings.
    {m249rows::kBody, m249rows::kFrameBone, m249rows::kUp, {m249rows::kSights[0], m249rows::kSights[1], m249rows::kSights[2]},
     {m249rows::kMuzzle[0], m249rows::kMuzzle[1], m249rows::kMuzzle[2]}, {m249rows::kPort[0], m249rows::kPort[1], m249rows::kPort[2]},
     {m249rows::kRightPalm[0], m249rows::kRightPalm[1], m249rows::kRightPalm[2]}, {m249rows::kLeftPalm[0], m249rows::kLeftPalm[1], m249rows::kLeftPalm[2]}, &kM249Bones,
     m249rows::kDepth, m249rows::kSpreadRow, m249rows::kShape, m249rows::kBodyStart, m249rows::kDepthFrame},
    // The AT4 (Operation Swordbreaker's M136, the scene's kit PlayerAT4 puts it in slot 0; measured 2026-11-18: build\mesh_15987_0_*, survey\AT4*; at4test
    // holds this row to them): one draw and its depth pass, the same mesh on the same four bones (15987/0 and 15987/15987): frame bone 0 the tube, 1 the
    // cocking lever (on top at its right, z 0.436 to 0.468), 2 the safety (its right, z 0.545 to 0.558), 3 the folding front grip (turned down under the
    // tube, z 0.871 to 0.885). The tube's axis at (-0.001 0.053), its front ring at z 0.995: the rocket's start. Its sights stand 6 cm left of the tube:
    // the rear aperture a ring of 4.6 mm in the leaf at z 0.614-0.616 round (0.0601 0.1279); the front sight a plate topped by three posts, the middle
    // one's tip (0.0560 0.1250 0.965). Their line lies 0.47 degrees under z and 0.68 right of it (meeting the tube's line 5.2 m out across, 9.2 m down),
    // and the game's own zoomed camera (the draw scan's: eye (0.061 0.129 0.491)) is on it within 0.6 mm, looking along it. Its port is nothing's (it
    // ejects nothing): its venturi's middle. The palms are the game's at rest in the morning's swap (AT4_fire.bin: the right hand on the firing mechanism,
    // the left on the front grip), confirmed on the roof from its real pickup (build\designs\at4_measured.md 4.1-4.2: the right within 1 mm in its rest,
    // the left on the grip to the mm; zoomed the right sits 1.1 cm higher, unused here; the depth pass's bones the colour's in every frame, its 4 and up
    // stale). Reloaded only by the game (X held, spent): nothing comes off it.
    {15987, 0, 0.47f, {0.0556f, 0.1248f, 0.995f}, {-0.001f, 0.053f, 0.995f}, {0.0f, 0.053f, 0.004f}, {-0.091f, 0.023f, 0.409f}, {0.037f, -0.047f, 0.883f}, nullptr, 15987, kAt4Gun, "at4", 0, -1, 0, 0.68f},
};
// Each handle drawn apart (handleruns.h) a held gun's of the table above, one with no lever bone (nothing else moves it), on that gun's own two passes'
// draws, its runs and its stem's top inside them (handleruns::Gaps takes them).
constexpr bool HandleRowsFit() {
    for (const handleruns::Handle& h : handleruns::kHandles) {
        bool fits = false;
        for (const HeldKind& k : kHeldKinds)
            if (k.bodyIndices == h.kind)
                fits = k.mag && k.mag->lever < 0 && h.colour.drawStart == k.bodyStart && h.colour.drawCount == k.bodyIndices &&
                       h.depth.drawStart == (k.depthStart ? k.depthStart : k.depthIndices) && h.depth.drawCount == k.depthIndices;
        for (const handleruns::Pass* pass : {&h.colour, &h.depth}) {
            handleruns::Run gaps[handleruns::kMaxRuns + 2];
            fits = fits && pass->runCount >= 1 && pass->runCount <= handleruns::kMaxRuns &&
                   handleruns::Gaps(pass->drawStart, pass->drawCount, pass->runs, pass->runCount, gaps) >= 0;
            bool inRun = !pass->shaftTop.count;
            for (int i = 0; i < pass->runCount; ++i)
                inRun = inRun || (pass->shaftTop.start >= pass->runs[i].start && pass->shaftTop.start + pass->shaftTop.count <= pass->runs[i].start + pass->runs[i].count);
            fits = fits && inRun && pass->shaftTop.count % 3 == 0;
        }
        if (!fits)
            return false;
    }
    return true;
}
static_assert(HandleRowsFit(), "a handle drawn apart (handleruns.h) is a held gun's with no lever bone, on its own passes' draws, its runs and its stem's top inside them");
// A tube shotgun's own parts, by its colour draw's index count (shellload.h has its places; hands.cpp works them): its bolt group's bone in the colour and
// the depth pass and where that bone is on the frame with the bolt home (its turn the frame's own: the recordings); its shell's draw (the shell on that
// draw's bone 0) and the shell's bone in the depth pass, which draws the shell with the body (the M1014's 14151 = 13377 + 324 + 450: bones 0 to 3 the
// body's, 4 the shell's).
struct HeldTube {
    UINT kind;
    const shellload::Gun* gun;
    int bolt, boltDepth;
    float boltRest[3];
    UINT shellCount, shellStart;
    int shellDepth;
};
constexpr HeldTube kHeldTubes[] = {
    {13377, &shellload::kM1014, 0, 0, {0.001f, -0.042f, 0.301f}, 450, 13701, 4},
};
const HeldTube* TubeOf(const HeldKind& kind) {
    for (const HeldTube& tube : kHeldTubes)
        if (kind.bodyIndices && tube.kind == kind.bodyIndices)
            return &tube;
    return nullptr;
}
// Only those measured. Anything else with the first-person pass's stencil, a knife, a grenade, a Stinger, a gun from another level, stays as the game draws it until it's been measured in its turn: its frame bone isn't
// known (the RPK's bone 0 is its lever).
constexpr HeldKind kHeldUnknown{0, 0, 0.0f, {0.0f, 0.09f, 0.75f}, {0.0f, 0.07f, 0.75f}, {-0.017f, 0.05f, 0.25f}, {-0.023f, -0.059f, 0.192f}, {0.002f, -0.018f, 0.561f}, nullptr, 0, 0, nullptr};
// The enemies' magnified sights on a gun held this way: rigid draws of their own with their bone 0 on the gun (found by putting each on the test gun, 2026-11-05): the body's two passes, the glass, and in
// the sight's own model the glass's middle and radius; the power as the game's own zoom has it, its FOV against the unzoomed 55.
struct HeldSight {
    DrawKey body, depth, glass;
    float eyepiece[3];
    float radius, power;
    const scopemarks::Marks* marks;  // every line and number it has zoomed
    DrawKey ownMarks;                // what the game itself draws of them unzoomed, at the sight, near: with the lens on it's the lens's, far off and at its true size
};
constexpr HeldSight kHeldSights[] = {
    {{10599, 72, 0}, {10599, 10671, 0}, {72, 0, 0}, {0.0f, 0.033f, -0.104f}, 0.014f, 3.44f, &scopemarks::kPso1, {}},                 // the PSO-1: FOV 17.2
    {{9084, 57, 0}, {9084, 9141, 0}, {57, 0, 0}, {0.0f, 0.035f, -0.126f}, 0.014f, 6.0f, &scopemarks::kPks07, {}},                    // the PKS-07: FOV 9.9
    {{3723, 48, 0}, {3723, 3771, 0}, {48, 0, 0}, {0.0f, 0.029f, -0.047f}, 0.016f, 2.95f, &scopemarks::kPka, {12, 210, 0}},           // the PK-A: FOV 20
    {{7884, 60, 0}, {7884, 7944, 0}, {60, 0, 0}, {0.0f, 0.0435f, -0.056f}, 0.0135f, 1.82f, &scopemarks::kPkAs, {3360, 0, 0}},        // the PK-AS, a closed tube unzoomed, so with a window and the game's own zoom, FOV 32: its ring and dot in its glass
    {{6492, 210, 0}, {6492, 6702, 0}, {210, 0, 0}, {0.0f, 0.0405f, 0.064f}, 0.0125f, 1.0f, &scopemarks::kKobra, {18, 0, 0}},         // the Kobra, likewise
    // The EoTech (the M4A1's on Operation Swordbreaker, measured 2026-11-13). The game draws its reticle only zoomed, as a part of its own (324/0), so
    // unzoomed its glass is two plain windows and every mark here is the mod's. Its eyepiece is the rear window's middle, where it bulges nearest the
    // eye (recoil keeps the eye off it), on the reticle's own line; the radius its half diagonal. The game zooms it to FOV 32, but it's drawn at 1x, as
    // the Kobra: a holographic sight doesn't magnify.
    {{9384, 0, 0}, {9384, 9534, 0}, {150, 9384, 0}, {0.0f, 0.0338f, -0.0406f}, 0.020f, 1.0f, &scopemarks::kEotech, {}},
    // The rifle scope (the Mk11's on Operation Swordbreaker, the game's "8x", measured 2026-11-18: build\mesh_12642_156_*, mesh_156_0_*): a closed tube. Its glass's draw is both
    // lenses, the eyepiece at z -0.1296 (27 vertices, 1.9 cm in radius) and the objective at z 0.155 to 0.165 (2.5 cm); the lens is the eyepiece, on the reticle's own line, (0 0.0370)
    // in its bone (the zoomed view in its bone 0 was at (0.0002 0.0369 -0.2437): on that line to 0.2 mm, 11.4 cm behind the glass). The game zooms it to FOV 8.7:
    // tan(27.5) / tan(4.35) = 6.84 times the unzoomed 55, the power here (the user's rule: the game's own magnification, whatever its name says). Zoomed the game swaps in its
    // reticle (480/0 dark lines, 558/480 the mil dots, on the same sheet as every other sight's): unzoomed it draws none, so every mark is the mod's (scopemarks::kRifleScope).
    {{12642, 156, 0}, {12642, 12798, 0}, {156, 0, 0}, {0.0f, 0.0370f, -0.1296f}, 0.019f, 6.84f, &scopemarks::kRifleScope, {}},
};
constexpr const char* kHeldSightNames[] = {"PSO-1", "PKS-07", "PK-A", "PK-AS", "Kobra", "EoTech", "rifle scope"};  // kHeldSights' rows, for the log
static_assert(std::size(kHeldSightNames) == std::size(kHeldSights));
// The guns' other parts drawn apart that no table above names, each with the gun it comes with: the gun's own from its first draw, as its rounds and belt
// are (HeldOwnDraw), not only once seen still on it for half a second. Found in each gun's survey frame (build\survey): the runs of its own index buffer
// that no other draw names, ahead of its depth pass (the buffer's second half, by the mesh dumps' sizes); and the AN-94's part drawn with it from a
// buffer of its own (holdpic's "parts drawn apart"). By count and start, as the magazines' draws are.
struct HeldPart {
    UINT kind;          // the gun's colour draw's index count (HeldKind::bodyIndices)
    UINT count, start;  // the part's draw
};
constexpr HeldPart kHeldParts[] = {
    {16188, 90, 16188},    // the AN-94's: after its body
    {16188, 4689, 0},      // ... rigid on its own bone 0, in a buffer of its own
    {16188, 4689, 4689},   // ... and that one's depth pass
    {24492, 36, 24804},    // the RPK-74M's: after its rounds
    {21585, 30, 21585},    // the AEK-971's: after its body
    {21723, 66, 22929},    // the AK-74M's: after its magazine's other piece
    {17013, 1350, 17325},  // the M16A4's: after its body
    {20298, 354, 0},       // the Stinger's: before its body (its depth pass hasn't it)
    {17991, 72, 17991},    // the AKS-74u's: after its body, on its frame: a strip along its sights' line
    {19314, 90, 19548},    // the UMP-45's: after its body, on its frame: the same kind of strip
    {13377, 324, 13377},   // the M1014's: its sight's three dots, on the frame (its shell's draw, 450/13701, is the tube's: kHeldTubes)
};
uint32_t g_heldSightFrame[std::size(kHeldSights)] = {};  // the frame its body was last drawn in: its glass's key alone may be something else's
uint32_t g_heldLensFrame[std::size(kHeldSights)] = {};   // the frame its lens was last drawn in
// recoil's sight at the eye (skin::Sight): each of those sights drawn on the held gun in a hand, its glass's middle on the gun's frame as the game draws
// them (whatever the magnifying's switch says), and the frame its body was last drawn in. Forgotten at a change of gun (RigFor). Under g_handsMutex.
struct GlassSeen {
    float onFrame[3] = {};
    bool have = false, sameView = false;  // (as HeldKept's)
    uint32_t relFrame = 0;                // onFrame's frame
    uint32_t frame = 0;                   // the frame its body was last drawn in
};
GlassSeen g_glassSeen[std::size(kHeldSights)];
// The sight of the table on the held gun in a hand lately (drawn within 2 frames), as recoil's Sight takes it: the first; -1 none. Under g_handsMutex.
int HeldGlass(uint32_t frameNow) {
    for (size_t n = 0; n < std::size(g_glassSeen); ++n)
        if (g_glassSeen[n].have && frameNow - g_glassSeen[n].frame <= 2)
            return static_cast<int>(n);
    return -1;
}
// The game's zoomed eye on each held gun's frame along z (kHeldKinds' notes: the view on its frame, zoomed), and the nearest of the gun itself on its
// sights' line toward the eye (skin::Sight's rearZ, with no sight of the table above on it): from its collision shape (build\guns\<shape>.bin), where its
// surface first comes within 1 cm of the line (the near plane's distance) going forward from the eye. A flat top has nothing there: the rear of its rail
// then, where whatever sits on the line stands. By its colour draw's index count.
struct HeldEye {
    UINT bodyIndices;
    float eyeZ, rearZ;
};
constexpr HeldEye kHeldEyes[] = {
    {24492, 0.126f, 0.470f},  // the RPK-74M: its rear sight
    {16188, 0.127f, 0.271f},  // the AN-94: its rear sight
    {21585, 0.127f, 0.477f},  // the AEK-971: its rear sight
    {21723, 0.127f, 0.257f},  // the AK-74M: its receiver's cover, 9 mm under the line from there to its rear sight
    {23064, 0.126f, 0.259f},  // the G3A3: its drum sight
    {15237, 0.122f, 0.210f},  // the KH2002: its carrying handle
    {17013, 0.134f, 0.288f},  // the M16A4: a flat top, nothing of it within 3.7 cm of the line
    {22194, 0.134f, 0.285f},  // the M4A1: likewise
    {27588, 0.124f, 0.230f},  // the M240: its rear sight
    {17991, 0.127f, 0.386f},  // the AKS-74u: its rear sight, on the receiver cover's hinge (the cover itself lies 15 mm under the line)
    {19314, 0.080f, 0.271f},  // the UMP-45: its rear sight
    {13377, 0.198f, 0.333f},  // the M1014: its ghost ring's housing (from its mesh: the first of it within 1 cm of the line)
    {12456, 0.198f, 0.307f},  // the Mk11: a flat top, the rear of its rail (its scope's eyepiece, the glass at z 0.312, is what stands on the line: Sight() takes that)
    {m249rows::kBody, m249rows::kEyeZ, m249rows::kRearZ},  // the M249: its rear aperture's ring on the feed cover (from its mesh: the first of it within 1 cm of the line)
};
// The gun in hand's magazine's bones on the frame, seated (as kMagazineBones: colour, rounds, depth, depth), and each gun's as RigFor kept them at a change
// of gun (seatrest.h's Seats, Kept, Change). Under g_handsMutex
seatrest::Seats g_heldSeats;
seatrest::Kept g_heldSeatsKept;
static_assert(kMagBones == seatrest::kSeats, "a held gun's seats as kMagazineBones has them");
// A belt fed gun's feed cover's bone on its frame as its body's draw last had it (the game's, or as a hand last held it), that frame and that gun's
// kind: what a draw riding the cover is known by (lidride.h). Under g_handsMutex
float g_heldLidOnFrame[12] = {};
uint32_t g_heldLidFrame = 0;
UINT g_heldLidKind = 0;
// With bare arms (no gloves' draw, so the game's palms on the gun are never known): each seat draw's magazine bone at rest on the frame (seatrest.h). The body's and the depth
// pass's by their seat; a part drawn apart (the rounds, the AK's other piece), placed by the last body's frame, keeps its own. Under g_handsMutex.
seatrest::Watch g_heldSeatRest[2 * kMagBones];
// A magazine in a hand whose gun's draw had no seat to draw it by (drawn nowhere, the one in the gun hidden as out), or not drawn while the game's own
// moves on the gun are followed (Followed): frames counted since the 10 s line (Update), said at once at most every 2 s with what's known, each reason
// its own. The owner's Operation Swordbreaker (2026-11-19) showed it only by a line missing.
std::atomic<uint32_t> g_heldMagMissed{0}, g_heldMagFollowed{0};
uint32_t g_heldMagMissFrame[2] = {};  // [0] no seat, [1] followed: under g_handsMutex
ULONGLONG g_heldMagMissSaid[2] = {};  // under g_handsMutex
constexpr UINT kHeldBodyLeast = 2000;
uint32_t g_rigKind = 0;  // whose rest the rig holds: 0 the launcher's. Under g_handsMutex.
struct RigSaved {
    RigSet rest;
    bool frozen = false;
};
std::unordered_map<uint32_t, RigSaved> g_rigSaved;
// Free hands, every gun: each of the held gun's draws as a hand last held it, under g_handsMutex, forgotten at a change of gun (RigFor). Its bones on the
// frame then: lying or flying into a hand it's drawn so, not as the game's animation has it now (its reload would play on the ground: X held with no
// pickup near is the game's reload). And whether its bone 0 stays put on the frame while held (a part of the gun drawn apart, a handle, a sight: drawn
// with it as it lies; a draw that moves on it, the arms of a model the mod doesn't know, isn't drawn then: it isn't the gun's).
struct HeldKept {
    float game[kPaletteBones][12] = {};   // its bones as the game drew them then, and its frame's inverse
    float inverse[12] = {};
    bool have = false, sameView = false;  // kept; with a frame drawn in the same view (else a frame's bob off: replaced by the first that is)
    uint32_t frame = 0;                   // ... in this frame (one of another view is taken once that's 2 frames old)
    float bone0[12] = {};                 // bone 0 on the frame as first seen held in a frame's own view
    bool haveBone0 = false;
    uint32_t steady = 0;                  // draws seen held there since
    bool moved = false;                   // it left it once: not the gun's
    bool said = false;                    // its "isn't the gun's" line logged
};
std::unordered_map<DrawKey, HeldKept, DrawKeyHash> g_heldKept;
constexpr uint32_t kRigidDraws = 60;     // seen still on the frame this many times (about half a second, each eye's passes): the gun's own
constexpr float kRigidMoved = 0.01f;     // m, bone 0 off where it was on the frame: it moves on the gun
constexpr float kRigidTurned = 0.05f;    // ... or its axes this far off (about 3 degrees)
// The attachments drawn with the held gun (gunbodies::kAttach's: a sight, a handle), each one's bone 0 on its frame and the frame it was last drawn in
// (skin::AttachmentsOnGun). Under g_handsMutex.
struct AttachSeen {
    float rel0[12] = {};
    bool have = false, sameView = false;  // (as HeldKept's)
    uint32_t relFrame = 0;                // rel0's frame
    uint32_t frame = 0;                   // the frame it was last drawn in
};
AttachSeen g_attachSeen[std::size(gunbodies::kAttach)];

const HeldKind& HeldKindNow() {
    const UINT indices = g_heldKind.load(std::memory_order_relaxed);
    for (const HeldKind& kind : kHeldKinds)
        if (kind.bodyIndices == indices)
            return kind;
    return kHeldUnknown;
}

// A draw of the gun in hand with a magazine in a hand not drawn, under g_handsMutex: no seat for its magazine's bone (seat), or the game's own moves on the
// gun followed (followed: its reload, or its palms off their rest; the magazine's part of the draw isn't run then). Each counted once a frame, said at once
// at most every 2 s (which gun, whether its palms are known, what the stance says), and every 10 s as the frames there were (Update).
void NoteMagNotDrawn(uint32_t frameNow, ULONGLONG now, int seat, bool followed) {
    const int why = followed ? 1 : 0;
    if (g_heldMagMissFrame[why] == frameNow)
        return;
    g_heldMagMissFrame[why] = frameNow;
    (followed ? g_heldMagFollowed : g_heldMagMissed).fetch_add(1, std::memory_order_relaxed);
    if (now - g_heldMagMissSaid[why] < 2000)
        return;
    g_heldMagMissSaid[why] = now;
    static const char* const kStances[] = {"standing", "crouched", "prone"};
    const int stance = input::StanceIndex();
    if (followed)
        Log("Magazine: in a hand but not drawn: the game's own moves on the %s followed (its reload, or its palms 6 cm off their rest; the mod's stance %s)",
            gunprofile::NameOf(HeldKindNow().shape), stance >= 0 && stance <= 2 ? kStances[stance] : "?");
    else
        Log("Magazine: in a hand but not drawn: no seat known for it on the %s (seat %d; the game's palms on it %s; the mod's stance %s%s)", gunprofile::NameOf(HeldKindNow().shape), seat,
            g_heldPalms.known ? "known" : "not seen at rest yet", stance >= 0 && stance <= 2 ? kStances[stance] : "?", input::Sprinting() ? ", the game's hands taken as moving" : "");
}

// Another gun in hand: each keeps its own rest. Under g_handsMutex.
void RigFor(uint32_t kind) {
    if (kind == g_rigKind)
        return;
    g_rigSaved[g_rigKind] = {g_rigRest, g_rigFrozen};
    const auto saved = g_rigSaved.find(kind);
    g_rigRest = saved != g_rigSaved.end() ? saved->second.rest : RigSet{};
    g_rigFrozen = saved != g_rigSaved.end() && saved->second.frozen;
    g_rigNow = RigSet{};
    g_rigNext = RigSet{};
    g_rigHaveSteady = false;
    seatrest::Change(g_heldSeatsKept, g_heldSeats, g_rigKind, kind);  // (the magazine's seats: each gun's its own)
    g_rigKind = kind;
    if (kind) {
        heldpalms::Forget(g_heldPalms);
        g_heldMoving = false;
    }
    g_heldKind = kind;
    std::fill(std::begin(g_heldSeatRest), std::end(g_heldSeatRest), seatrest::Watch{});
    g_heldKept.clear();  // (the gun before's draws as last held, and which of them were its own)
    std::fill(std::begin(g_attachSeen), std::end(g_attachSeen), AttachSeen{});
    std::fill(std::begin(g_glassSeen), std::end(g_glassSeen), GlassSeen{});
}

// A held gun's draw seen held, by a frame drawn in its own view: whether its bone 0 stays put on the frame (HeldKept). Under g_handsMutex.
void NoteHeldRigid(HeldKept& kept, const float* inverse, const float* bone0) {
    float on[12];
    Compose(inverse, bone0, on);
    if (!kept.haveBone0) {
        std::memcpy(kept.bone0, on, sizeof(kept.bone0));
        kept.haveBone0 = true;
        return;
    }
    const float* was = kept.bone0;
    float turned = 0.0f;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            turned = std::max(turned, std::fabs(on[i * 4 + j] - was[i * 4 + j]));
    if (!(Length3(on[3] - was[3], on[7] - was[7], on[11] - was[11]) <= kRigidMoved && turned <= kRigidTurned))  // (not finite: moved too)
        kept.moved = true;
    else if (kept.steady < kRigidDraws)
        ++kept.steady;
}

// With no hand on the held gun (free hands: flying into a hand, lying where it fell), whether this draw is the gun's own and is drawn with it: its body and
// depth pass, its magazine's other draws and its belt, its other parts the mod knows (kHeldParts), a sight the mod knows (its glass and marks with it), an
// attachment, a draw that stayed put on it while held. Anything else (another model's arms the mod doesn't know) would lie on the ground with it. Under
// g_handsMutex.
bool HeldOwnDraw(const DrawKey& key, bool body, const HeldKind& kind, const HeldKept& kept, uint32_t frameNow) {
    if (body || gunbodies::AttachFor(key.indexCount, key.startIndex, key.baseVertex) || (kept.haveBone0 && !kept.moved && kept.steady >= kRigidDraws))
        return true;
    for (const HeldPart& part : kHeldParts)
        if (kind.bodyIndices && part.kind == kind.bodyIndices && key.indexCount == part.count && key.startIndex == part.start)
            return true;
    const HeldMagBones* mag = kind.mag;
    if (mag && ((mag->roundsCount && key.indexCount == mag->roundsCount && key.startIndex == mag->roundsStart) ||
                (mag->partCount && key.indexCount == mag->partCount && key.startIndex == mag->partStart) ||
                (mag->beltCount && key.indexCount == mag->beltCount && key.startIndex == mag->beltStart)))
        return true;
    bool own = false;
    for (size_t n = 0; n < std::size(kHeldSights); ++n) {
        const HeldSight& sight = kHeldSights[n];
        if (key == sight.body || key == sight.depth) {
            g_heldSightFrame[n] = frameNow;  // (its glass and its marks come after it)
            own = true;
        } else if ((key == sight.glass || (sight.ownMarks.indexCount && key == sight.ownMarks)) && frameNow - g_heldSightFrame[n] <= 2) {
            own = true;
        }
    }
    return own;
}

// Which of a held gun's draw's bones are its magazine's (seatBone, -1 none) and which seat each is (seatIndex: the colour pass's 0, the rounds' 1, the depth
// pass's 2 and 3), by the draw; its colour draw sets the magazine's box too (the dropped one's).
void HeldSeats(const HeldMagBones& mag, bool colour, bool depth, UINT count, UINT start, int* seatBone, int* seatIndex) {
    if (colour) {
        g_magBoxMin = mag.boxMin, g_magBoxMax = mag.boxMax;
        seatBone[0] = mag.colour, seatIndex[0] = 0;
    } else if (depth) {
        seatBone[0] = mag.depth[0], seatIndex[0] = 2;
        seatBone[1] = mag.depth[1], seatIndex[1] = 3;
    } else if (count == mag.roundsCount && start == mag.roundsStart) {
        seatBone[0] = mag.rounds, seatIndex[0] = 1;
    } else if (mag.partCount && count == mag.partCount && start == mag.partStart) {
        seatBone[0] = mag.partBone, seatIndex[0] = 0;  // the same bone as the magazine's in the colour pass
    }
}

// A held gun's colour pass last drawn (lying, put away and all): its data's scan goes by the gap since. Its own: the depth pass comes first in each view, so a long
// gap never showed in the time they share. Under g_handsMutex.
ULONGLONG g_heldColourAt = 0;

// A magazine's own draw (g_partCopy's): its shaders and every slot holding its palette, said once (again only after another such draw was said).
void SayPartDraw(ID3D11DeviceContext* ctx, ID3D11Resource* palette, UINT count, UINT start) {
    static std::atomic<uint64_t> s_said{0};
    const uint64_t key = static_cast<uint64_t>(count) << 32 | start;
    if (s_said.exchange(key, std::memory_order_relaxed) == key)
        return;
    unsigned vs = 0, ps = 0;
    PaletteSlots(ctx, palette, vs, ps);
    const auto slots = [](unsigned bits, char* out, size_t size) {
        int used = std::snprintf(out, size, "%s", bits ? "" : "none");
        for (UINT i = 0; i < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT && used >= 0 && static_cast<size_t>(used) < size; ++i)
            if ((bits >> i) & 1u)
                used += std::snprintf(out + used, size - used, "%s%u", used ? " " : "", i);
    };
    char vsText[48], psText[48];
    slots(vs, vsText, sizeof(vsText));
    slots(ps, psText, sizeof(psText));
    ID3D11VertexShader* vertex = nullptr;
    ID3D11PixelShader* pixel = nullptr;
    ctx->VSGetShader(&vertex, nullptr, nullptr);
    ctx->PSGetShader(&pixel, nullptr, nullptr);
    Log("Held gun: a magazine's own draw %u/%u: vertex shader %016llx, pixel shader %016llx (build\\ps); the game's palette in vertex shader slot %s, pixel "
        "shader slot %s; the mod's put in %s",
        count, start, static_cast<unsigned long long>(shaders::ChecksumOfVs(vertex)), static_cast<unsigned long long>(shaders::ChecksumOf(pixel)), vsText,
        psText, g_partCopy.load(std::memory_order_relaxed) == 1 ? "every one of them (partcopy 1, a test)" : "the last vertex shader slot alone, as before");
    if (vertex)
        vertex->Release();
    if (pixel)
        pixel->Release();
}

// A side lever, a charging handle, drawn as the hand has it (handmag.cpp, by HandTargets::heldLeverNotch, heldLeverSwing and heldLeverBack): turned up into
// its notch, swung out, then drawn back along the gun. bone: its bone as drawn (or the frame's, for a handle with no bone of its own: handleruns.h); pose: the
// gun's frame as drawn (HandTargets::launcherPose).
void PullLever(float* bone, const handmag::Gun& gun, float notch, float swing, float back, const float* pose) {
    // Turned up into its notch (the UMP-45's): about the gun's z, through its tube's middle.
    if (notch != 0.0f) {
        const float c = std::cos(notch), s = std::sin(notch), ax = gun.leverNotchAxis[0], ay = gun.leverNotchAxis[1];
        const float turn[12] = {c, -s, 0.0f, ax - (c * ax - s * ay), s, c, 0.0f, ay - (s * ax + c * ay), 0.0f, 0.0f, 1.0f, 0.0f};
        float toFrame[12], about[12], move[12];
        if (Invert(pose, toFrame)) {
            Compose(pose, turn, about);
            Compose(about, toFrame, move);
            MoveBy(move, bone);
        }
    }
    if (swing != 0.0f) {
        const float c = std::cos(swing), s = std::sin(swing), px = gun.leverPivot[0], pz = gun.leverPivot[1];
        const float turn[12] = {c, 0.0f, s, px - (c * px + s * pz), 0.0f, 1.0f, 0.0f, 0.0f, -s, 0.0f, c, pz - (-s * px + c * pz)};
        float toFrame[12], about[12], move[12];
        if (Invert(pose, toFrame)) {
            Compose(pose, turn, about);
            Compose(about, toFrame, move);
            MoveBy(move, bone);
        }
    }
    for (int i = 0; i < 3; ++i)
        bone[i * 4 + 3] -= back * pose[i * 4 + 2];
}

// A handle with no bone of its own drawn apart (handleruns.h): a line once a pull, at its first draw (none for half a second before), with the pull before's
// draws. DrawHeldPart's draws come on the deferred contexts' threads too (DrawIndexed's [1]), outside g_handsMutex: its counts under their own lock, a draw
// of an older frame than the last never starting a pull, and the line written once the lock is let go.
std::mutex g_handleNoteMutex;
void NoteHandleDrawn(const handleruns::Handle& handle, bool depth, float back, bool shaft, uint32_t frameNow) {
    static uint32_t s_frame = 0, s_draws[2] = {};
    static float s_farthest = 0.0f;
    static bool s_any = false;
    bool line = false, first = false;
    uint32_t colourDraws = 0, depthDraws = 0;
    float farthest = 0.0f;
    {
        std::lock_guard hold(g_handleNoteMutex);
        const int32_t since = static_cast<int32_t>(frameNow - s_frame);  // below 0: an older frame's draw, come late from another thread
        if (!s_any || since > 36) {
            line = true, first = !s_any;
            colourDraws = s_draws[0], depthDraws = s_draws[1], farthest = s_farthest;
            s_any = true;
            s_draws[0] = s_draws[1] = 0;
            s_farthest = 0.0f;
        }
        if (line || since > 0)
            s_frame = frameNow;
        ++s_draws[depth ? 1 : 0];
        s_farthest = std::max(s_farthest, back);
    }
    if (!line)
        return;
    if (first)
        Log("Held gun: the %s's handle drawn with the hand as it's pulled%s (its colour and depth passes each drawn round it, its runs moved: handleruns.h)", handle.name,
            shaft ? ", its shaft out of the receiver behind it" : "");
    else
        Log("Held gun: the %s's handle drawn with the hand as it's pulled%s; the pull before: %u colour and %u depth draws, back as far as %.0f mm", handle.name,
            shaft ? ", its shaft out of the receiver behind it" : "", colourDraws, depthDraws, farthest * 1000.0f);
}

bool DrawHeldPart(int which, ID3D11DeviceContext* ctx, UINT count, UINT start, INT base) {
    const Upload& u = t_upload;
    if (!g_heldOn.load(std::memory_order_relaxed) || !u.resource || u.bytes.size() != kGloveUploadSize || !PlayersOwn(u.bytes))
        return false;
    // The guns the mod knows, drawn just now, or the train's pistol.
    const uint32_t frameNow = g_frameSerial.load(std::memory_order_relaxed);
    if (frameNow - g_rifleFrame.load(std::memory_order_relaxed) <= 2 || frameNow - g_launcherFrame.load(std::memory_order_relaxed) <= 2 || skin::TrainPistolInHand())
        return false;
    ID3D11DepthStencilState* state = nullptr;
    UINT stencil = 0;
    ctx->OMGetDepthStencilState(&state, &stencil);
    if (state)
        state->Release();
    // (The measured sights' glass comes with it too. With 2 come nearby soldiers' own parts: moved with the gun, a teammate would jump with it.)
    if (stencil != kFirstPersonStencil)
        return false;
    const ULONGLONG now = GetTickCount64();
    const auto* game = reinterpret_cast<const float*>(u.bytes.data());
    // Its body and its depth pass, by their counts; what comes with them, its sights and the rest, goes where they go.
    UINT bodyOfKind = 0;
    bool colour = false, depth = false;
    for (const HeldKind& held : kHeldKinds) {
        if (start == held.bodyStart && count == held.bodyIndices)
            colour = true, bodyOfKind = held.bodyIndices;
        else if (count == held.depthIndices && start == (held.depthStart ? held.depthStart : held.depthIndices))
            depth = true, bodyOfKind = held.bodyIndices;
    }
    if (colour || depth) {
        g_heldAtKind = bodyOfKind;  // (before the stamp: once that says a gun is drawn, this says which)
        g_heldAt = now;
        g_heldFrame = frameNow;
    } else if (!DrawnLately(g_heldAt, g_heldFrame) || !g_heldKind.load(std::memory_order_relaxed)) {
        return false;
    }
    thread_local std::vector<uint8_t> t_rig, t_heldMag, t_fallen;
    t_rig = u.bytes;
    auto* bones = reinterpret_cast<float*>(t_rig.data());
    bool magHide = false, magDraw = false, spareHide = false, zoomWanted = false, windowWanted = false, coverMoved = false;
    float coverMove[12], coverOther[12], coverOtherBone[12];  // its turn; the turn of its other side; on its own bone, that bone for the other side
    UINT coverRun = 0, coverRunCount = 0;  // the cover, if it's a run of this draw's indices, on this bone
    int coverFrame = 0, coverBoneIndex = -1;
    // A handle with no bone of its own, pulled (handleruns.h): its row and this pass's part of it, the frame's bone in this pass, how it's pulled (as PullLever takes it).
    const handleruns::Handle* handle = nullptr;
    const handleruns::Pass* handlePass = nullptr;
    int handleFrame = 0;
    const handmag::Gun* handleGun = nullptr;
    float handleNotch = 0.0f, handleSwing = 0.0f, handleBack = 0.0f, handlePose[12] = {};
    int lensSight = -1;
    int seatBone[2] = {-1, -1}, seatIndex[2] = {-1, -1};
    bool partDraw = false;  // a magazine's own draw (HeldMagBones::partCount: the M249's pouch, the AK-74M's piece, the Mk11's 1902/0): weapon.txt "partcopy"'s
    // (Its in-place draw and its copies with the mod's palette in every slot holding the game's: partcopy 1, a test; else as before.)
    const auto everySlot = [&] { return partDraw && g_partCopy.load(std::memory_order_relaxed) == 1; };
    LONG window[4] = {};
    scopezoom::Lens zoomLens;
    WorldPlacing placing;  // this draw's, for the magazine kept in the world
    bool twoSided = false;  // lying, drawn with both faces (freehands.txt "twosided 1")
    // A tube shotgun's shells (kHeldTubes): this draw is its shell's own (the shell on its bone 0) or its depth pass (the shell on a bone of that). The
    // shell's mesh is drawn again, every other bone on one point, at each place a shell is: on the gun or in a hand (the hands' targets, taken under the
    // lock below), lying or falling (loose); after the draw itself, or in its stead. Never cut by a scope's window (it isn't the gun's).
    const HeldTube* shellTube = nullptr;
    int shellBone = -1;
    float shellPoses[3][12];
    int shellPoseCount = 0;
    auto drawShells = [&] {
        if (!shellTube || shellBone < 0)
            return;
        thread_local std::vector<uint8_t> t_shell;
        t_noHole = true;
        for (int i = 0; i < shellPoseCount; ++i) {
            t_shell.assign(kGloveUploadSize, 0);
            std::memcpy(reinterpret_cast<float*>(t_shell.data()) + shellBone * 12, shellPoses[i], sizeof(shellPoses[i]));
            DrawWithPalette(which, ctx, count, start, base, u.resource, t_shell.data(), kGloveUploadSize);
        }
        if (placing.have) {
            // (Loose has the shell in its own space, its base's middle at 0, along +z: the shell's bone's vertices less its base's place there.)
            const float* b = shellTube->gun->baseOnBone;
            const float toCase[12] = {1, 0, 0, -b[0], 0, 1, 0, -b[1], 0, 0, 1, -b[2]};
            std::lock_guard lock(g_looseMutex);
            for (int p = 0; p < g_loosePoseCount; ++p) {
                const loose::Pose& pose = g_loosePoses[p];
                if (pose.kind != kLooseShell)
                    continue;
                float world[12];
                Compose(pose.bone, toCase, world);
                t_shell.assign(kGloveUploadSize, 0);
                WorldToDrawn(placing, world, g_looseOrigin, reinterpret_cast<float*>(t_shell.data()) + shellBone * 12);
                if (DrawWithPalette(which, ctx, count, start, base, u.resource, t_shell.data(), kGloveUploadSize))
                    g_looseDraws.fetch_add(1, std::memory_order_relaxed);
            }
        }
        t_noHole = false;
    };
    // Where this draw's magazine is (seatBone, seatIndex: HeldSeats), once its bones are final: where it flies from if it's knocked out (not while it's hidden, nor
    // with the gun drawn nowhere: stowed), its bones on one point while hidden, where one in a hand falls from if it's let go of, and the old fall's bones for this
    // draw. Whether that one is drawn.
    auto keepDrop = [&](bool stowed) {
        const bool shown = g_dropShown.load(std::memory_order_relaxed) && seatBone[0] >= 0;
        if (seatBone[0] >= 0) {
            std::lock_guard lock(g_magMutex);
            Drop& d = g_drop;
            const double nowSeconds = Seconds();
            for (int s = 0; s < 2; ++s) {
                if (seatBone[s] < 0)
                    continue;
                const int i = seatIndex[s];
                if (stowed) {
                    d.seen[i] = false;  // drawn nowhere: none knocked out of it from where the game holds it
                } else if (!magHide) {  // where it flies from if it's knocked out
                    DrawnToWorld(placing, bones + seatBone[s] * 12, d.last[i], d.lastOrigin[i]);
                    d.seen[i] = true;
                }
                if (magHide)
                    std::memset(bones + seatBone[s] * 12, 0, 48);  // all on one point
                if (magDraw) {  // where it falls from if it's let go of
                    DrawnToWorld(placing, reinterpret_cast<const float*>(t_heldMag.data() + seatBone[s] * 48), d.held[i], d.heldOrigin[i]);
                    d.heldSeen[i] = true;
                    d.heldAt[i] = nowSeconds;
                }
            }
            if (shown) {
                t_fallen.assign(kGloveUploadSize, 0);
                for (int s = 0; s < 2; ++s)
                    if (seatBone[s] >= 0 && d.started[seatIndex[s]]) {
                        float world[12];
                        MoveBone(d.start[seatIndex[s]], d, world);
                        WorldToDrawn(placing, world, d.origin0, reinterpret_cast<float*>(t_fallen.data() + seatBone[s] * 48));
                    }
            }
        }
        return shown;
    };
    // What rides this draw and isn't the gun, after it: the old fall's magazine, one in a hand, and this gun's magazines lying or flying (loose), each its own
    // copy, its bones (this draw's seat bones) where it is.
    auto drawCopies = [&](bool shown) {
        if (shown)
            DrawWithPalette(which, ctx, count, start, base, u.resource, t_fallen.data(), kGloveUploadSize, false, 0, everySlot());
        if (magDraw && DrawWithPalette(which, ctx, count, start, base, u.resource, t_heldMag.data(), kGloveUploadSize, false, 0, everySlot()))
            ++g_heldMagDraws;
        if (seatBone[0] >= 0 && placing.have) {
            thread_local std::vector<uint8_t> t_looseHeld;
            std::lock_guard lock(g_looseMutex);
            for (int p = 0; p < g_loosePoseCount; ++p) {
                const loose::Pose& pose = g_loosePoses[p];
                if (pose.kind != kLooseHeldMag)
                    continue;
                for (const LooseMag& mag : g_looseMags) {
                    if (mag.id != pose.id)
                        continue;
                    t_looseHeld.assign(kGloveUploadSize, 0);
                    bool any = false;
                    for (int s = 0; s < 2; ++s) {
                        if (seatBone[s] < 0 || !mag.have[seatIndex[s]])
                            continue;
                        float world[12];
                        Compose(pose.bone, mag.seat[seatIndex[s]], world);
                        WorldToDrawn(placing, world, g_looseOrigin, reinterpret_cast<float*>(t_looseHeld.data()) + seatBone[s] * 12);
                        any = true;
                    }
                    if (any && DrawWithPalette(which, ctx, count, start, base, u.resource, t_looseHeld.data(), kGloveUploadSize, false, 0, everySlot()))
                        g_looseDraws.fetch_add(1, std::memory_order_relaxed);
                    break;
                }
            }
        }
    };
    {
        std::unique_lock lock(g_handsMutex);
        if (!g_handsOn)
            return false;
        if (colour || depth)
            RigFor(bodyOfKind);
        skin::HandTargets t = g_hands;
        // Put away, or its hand not seen at a place (stowed): drawn nowhere, never where the game holds it, whichever gun the game draws (the stamps and RigFor above
        // still say which it is). Only what rides this draw and isn't the gun: its magazines lying (loose, and the old fall's), one in a hand.
        if (t.stowed) {
            if (colour && now - g_heldColourAt <= 15000)
                g_heldColourAt = now;  // (as lying: taken up again after 15 s away it's no new gun, its data not looked for again; a gap of 15 s within it, a load's, still is)
            if (const HeldMagBones* mag = HeldKindNow().mag; mag && ToArms(t, ctx)) {
                placing.Take(t);
                HeldSeats(*mag, colour, depth, count, start, seatBone, seatIndex);
                partDraw = mag->partCount && count == mag->partCount && start == mag->partStart;
                for (int s = 0; s < 2; ++s)
                    if (seatBone[s] >= 0 && t.mag && g_heldSeats.have[seatIndex[s]]) {
                        if (!magDraw) {
                            t_heldMag.assign(kGloveUploadSize, 0);  // every other bone of it on one point
                            magDraw = true;
                        }
                        Compose(t.magPose, g_heldSeats.seat[seatIndex[s]], reinterpret_cast<float*>(t_heldMag.data() + seatBone[s] * 48));
                    } else if (seatBone[s] >= 0 && t.mag) {
                        NoteMagNotDrawn(frameNow, now, seatIndex[s], false);
                    }
            }
            // (A tube shotgun put away: its shells lying on the ground are still drawn with its shell's draws.)
            if (const HeldTube* tube = TubeOf(HeldKindNow()); tube && (depth || (!colour && count == tube->shellCount && start == tube->shellStart)) && ToArms(t, ctx)) {
                placing.Take(t);
                shellTube = tube;
                shellBone = depth ? tube->shellDepth : 0;
            }
            lock.unlock();
            g_stowedSkips[kStowedHeld].fetch_add(1, std::memory_order_relaxed);
            drawCopies(keepDrop(true));
            drawShells();
            return true;
        }
        // In a hand (gun), or with none on it (launcherDrawn: free hands, flying into a hand or lying where it fell).
        if (!ToArms(t, ctx) || !(t.gun || t.launcherDrawn))
            return false;
        // With no hand on it, only the gun the targets came for: in the first frame of a switch the game makes while one lies (its own pickup, a script),
        // the new gun's draws aren't put where the one before lies. Drawn as the game has it, as any switch's first frames are.
        // (... nor the gun due in a hand off the ground, HandTargets::arriveKind: its first frame is drawn where it lay.)
        if (!t.gun && g_heldKind.load(std::memory_order_relaxed) != g_handsKind && g_heldKind.load(std::memory_order_relaxed) != t.arriveKind)
            return false;
        placing.Take(t);
        twoSided = !t.gun && t.launcherLying && t.lyingTwoSided;
        const HeldKind& kind = HeldKindNow();
        const HeldMagBones* mag = kind.mag;
        // A tube shotgun's shell: the game's own on the gun (its shell's draw, and that bone of its depth pass) as its animation has it only in a hand, and
        // with the reload by hand on only while the game's own reload plays; the hands' shells and the loose ones drawn with either (drawShells).
        const HeldTube* tube = TubeOf(kind);
        const bool shellDraw = tube && !colour && !depth && count == tube->shellCount && start == tube->shellStart;
        const bool shellOwn = t.gun && (!t.heldTube || t.heldShellGame);
        if (tube && (shellDraw || depth)) {
            shellTube = tube;
            shellBone = shellDraw ? 0 : tube->shellDepth;
            const bool gunPlaced = t.gun || !t.launcherLying || t.haveDrawnFromWorld;
            for (int i = 0; t.heldTube && i < t.heldShells && i < static_cast<int>(std::size(t.heldShellPose)) && shellPoseCount < static_cast<int>(std::size(shellPoses)); ++i) {
                if (!t.heldShellOnGun[i])
                    std::memcpy(shellPoses[shellPoseCount++], t.heldShellPose[i], sizeof(shellPoses[0]));
                else if (gunPlaced)
                    Compose(t.launcherPose, t.heldShellPose[i], shellPoses[shellPoseCount++]);
            }
        }
        if (shellDraw && !shellOwn) {
            lock.unlock();
            drawShells();
            return true;
        }
        float inverse[12], delta[12];
        if (colour || depth) {
            const float* frame = game + (depth && kind.depthFrame >= 0 ? kind.depthFrame : kind.frameBone) * 12;
            if (!Invert(frame, inverse))
                return false;
            std::memcpy(g_launcherFrameOrig, frame, sizeof(g_launcherFrameOrig));
            g_launcherDeltaTime = now;
            g_launcherFrameView = g_viewUploads.load();
            LearnRigKept(g_launcherFrameView, inverse);
            if (colour) {
                // What goes with it only while a hand holds it (gun): the game's effects moved with it, the scan of its data, its bullets' or missile's start,
                // the log of its numbers. Lying or flying, none of them (nothing fires then).
                if (t.gun)
                    g_launcherEffect.Note(t, t.launcherSeen, inverse);
                // Once for each level's data, as the rifle's: its scatter as it is zoomed, and where its bullets start.
                static size_t s_spreadGun = std::size_t(-1);  // another gun straight after this one has numbers of its own
                const ULONGLONG before = g_heldColourAt;
                g_heldColourAt = now;  // (lying too: taken up again after 15 s on the ground it's no new gun, its data not looked for again)
                // Another gun of the same kind taken off the ground (a new weapon object: the hands' slot's firing state new, held 30 frames) looked for
                // again too, at most once in 15 s (a scan reads all of the game's memory, 0.6 s of a thread): its bullets' start is written only where it
                // was found, so data of its own would leave its rounds from the game's eye. Whether a take brings any: the scan's "Rifle: its bullets'
                // start found in" line. Not the Stinger nor the AT4 (their data are found their own ways); weapon.txt "heldshot 0": not at all.
                static uintptr_t s_firingSeen = 0, s_firingScanned[2] = {};  // (each of the hands' two slots its own: a key switch back is no take)
                static uint32_t s_firingFrame = 0;
                static int s_firingFrames = 0, s_firingSlot = 0;
                static bool s_adoptNext = true;  // the next firing state to hold 30 frames is the one the last scan was for
                bool taken = false;
                if (t.gun && kind.spreadGun != kStingerGun && kind.spreadGun != kAt4Gun && g_heldShotOn.load(std::memory_order_relaxed) &&
                    frameNow != s_firingFrame) {
                    s_firingFrame = frameNow;
                    const int slot = ammo::HandSlot() == 1 ? 1 : 0;
                    const uintptr_t firing = ammo::FiringOf(slot);
                    s_firingFrames = firing && firing == s_firingSeen ? std::min(s_firingFrames + 1, 1000) : 0;
                    s_firingSeen = firing, s_firingSlot = slot;
                    if (s_firingFrames >= 30) {
                        if (s_adoptNext)
                            s_firingScanned[slot] = firing, s_adoptNext = false;
                        taken = firing != s_firingScanned[slot];
                    }
                }
                // Per kind (the audit of 2026-11-19: 17 whole-memory scans in the owner's 24 minutes, one at every swap between the M4A1, the Mk11 and
                // the M249, each finding again what was known): a swap back to a kind scanned since the last load (the 15 s gap) whose data is still
                // where it was (KindDataThere) starts no scan, only the swap's own bookkeeping, said once a swap. A kind not scanned since, or whose data
                // is gone, a load, ten minutes, a take of its kind: scanned as before. One scan at a time (StartScan): wanted while another runs, kept
                // and started the first frame after.
                static std::atomic<ULONGLONG> s_spread{0};
                struct ScannedKind {
                    size_t kind;
                    ULONGLONG at;
                };
                static ScannedKind s_scanned[8] = {};
                static int s_scannedCount = 0;
                static bool s_scanPending = false;
                bool scanNow = false;
                if (t.gun) {
                    const ULONGLONG last = s_spread.load();
                    const bool fresh = !last || now - last > 600000 || (now - before > 15000 && now - last > 15000);
                    if (fresh)
                        s_scannedCount = 0;  // (a load, or ten minutes: what was found may be gone, every kind looked for again)
                    bool want = s_scanPending || fresh || (taken && now - last > 15000);
                    if (!want && s_spreadGun != kind.spreadGun) {
                        const ScannedKind* seen = nullptr;
                        for (int k = 0; k < s_scannedCount; ++k)
                            if (s_scanned[k].kind == kind.spreadGun)
                                seen = &s_scanned[k];
                        if (seen && KindDataThere(kind.spreadGun)) {
                            s_spreadGun = kind.spreadGun;
                            s_adoptNext = true, s_firingFrames = 0;  // (as its scan's: the state in the hands from now is the one found)
                            Log("Held gun: %u indices in hand (its data, found %.0f s ago, still there: not looked for again)", count,
                                static_cast<double>(now - seen->at) / 1000.0);
                        } else {
                            want = true;
                        }
                    }
                    s_scanPending = want && ScanRunning();
                    scanNow = want && !s_scanPending;
                }
                if (scanNow) {
                    {
                        int k = 0;
                        while (k < s_scannedCount && s_scanned[k].kind != kind.spreadGun)
                            ++k;
                        if (k == s_scannedCount && s_scannedCount < static_cast<int>(std::size(s_scanned)))
                            ++s_scannedCount;
                        else if (k == s_scannedCount)
                            k = 0;  // (more kinds than kept: the oldest place given up)
                        s_scanned[k] = ScannedKind{kind.spreadGun, now};
                    }
                    s_spread = now;
                    // A take's scan is for the state in the hands now. Any other is for the next to hold 30 frames from now: at a switch to another
                    // kind the hands' slot names the gun before for a frame yet (ammo.cpp works it out at each Present, by the kind drawn then), and
                    // taken as this scan's, the new gun's own state counted as a take 15 s on.
                    taken = taken && s_spreadGun == kind.spreadGun;  // (with another kind drawn it's that kind's scan)
                    if (taken)
                        s_firingScanned[s_firingSlot] = s_firingSeen;
                    else
                        s_adoptNext = true, s_firingFrames = 0;
                    s_spreadGun = kind.spreadGun;
                    if (kind.spreadGun == kStingerGun) {
                        StartScan(FindStinger, nullptr);
                        StartScan(ClearLauncherSpread, nullptr);
                    } else if (kind.spreadGun == kAt4Gun) {
                        // (Its own scan alone: not ClearLauncherSpread, whose 40mm_GunSway match would take the AT4's prone transitions, and on a
                        // level without the SMAW finds nothing else of its own.)
                        StartScan(FindAt4, nullptr);
                    } else {
                        StartScan(ClearRifleSpread, reinterpret_cast<LPVOID>(kind.spreadGun));
                    }
                    Log("Held gun: %u indices in hand%s", count, taken ? " (another of its kind taken: its data looked for again)" : "");
                }
                // Where its bullets start: its sights' line at its muzzle, or the line through a sight of the table on it, along its sights' line
                // (aimrules::HeldShotLine, recoil's sight at the eye alike; weapon.txt "heldshot 0": the sights' line whatever is on it). On the irons' line,
                // the AN-94's and RPK-74M's rounds went 3.1 to 4.6 cm under the reticle of each optic they carry, at every range.
                float shotLine[3];
                const int glassSight = g_heldShotOn.load(std::memory_order_relaxed) ? HeldGlass(frameNow) : -1;
                aimrules::HeldShotLine(kind.sights, kind.up, glassSight >= 0 ? g_glassSeen[glassSight].onFrame : nullptr, shotLine);
                static uint32_t s_bulletFrame = 0;
                // Its missile starts from the game's eye plus its data's numbers: made the tube's end, over its own data only, as the SMAW's.
                if (t.gun && kind.spreadGun == kStingerGun) {
                    if (float start[3]; s_bulletFrame != frameNow && g_launcherSpawn.load(std::memory_order_relaxed) && ShotStart(t, t.launcherSeen, kind.muzzle, start) && Length3(start[0], start[1], start[2]) < 3.0f) {
                        s_bulletFrame = frameNow;
                        for (size_t n = 0; n < std::size(g_stingerShot); ++n) {
                            const uintptr_t place = g_stingerShot[n].load(std::memory_order_relaxed);
                            if (!place)
                                continue;
                            float there[11];
                            SIZE_T got = 0;
                            const bool ours = ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(place), there, sizeof(there), &got) && got == sizeof(there) && there[4] == 0.0f && std::fabs(there[5] - 0.12f) < 1e-4f &&
                                              there[6] == 1.0f && there[8] == 0.0f && there[9] == 0.0f && there[10] == 16.0f &&
                                              ((std::fabs(there[0] + 0.2f) < 1e-4f && std::fabs(there[1] + 0.065f) < 1e-4f && std::fabs(there[2] - 0.35f) < 1e-4f) || std::memcmp(there, g_stingerShotLast[n], sizeof(g_stingerShotLast[n])) == 0);
                            if (!ours) {
                                g_stingerShot[n] = 0;
                                continue;
                            }
                            if (WriteProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(place), start, sizeof(start), nullptr))
                                std::memcpy(g_stingerShotLast[n], start, sizeof(start));
                        }
                    }
                } else if (t.gun && kind.spreadGun == kAt4Gun) {
                    // The AT4's rocket from the tube's end (its front ring), as the SMAW's and the Stinger's: over its own FiringFunctionData only, as read
                    // again each time (At4ShotThere: its vtable, rate and reload, its shot's direction and speed, its start the data's or the last written).
                    if (float start[3]; s_bulletFrame != frameNow && g_launcherSpawn.load(std::memory_order_relaxed) && ShotStart(t, t.launcherSeen, kind.muzzle, start) && Length3(start[0], start[1], start[2]) < 3.0f) {
                        s_bulletFrame = frameNow;
                        std::lock_guard lock(g_at4ShotMutex);
                        for (size_t n = 0; n < std::size(g_at4Shot); ++n) {
                            const uintptr_t place = g_at4Shot[n].load(std::memory_order_relaxed);
                            if (!place)
                                continue;
                            if (!At4ShotThere(place, g_at4ShotLast[n])) {
                                g_at4Shot[n] = 0;
                                Log("AT4: its rocket's start at %p isn't there any more: not written", reinterpret_cast<void*>(place));
                                continue;
                            }
                            if (WriteProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(place), start, sizeof(start), nullptr)) {
                                std::memcpy(g_at4ShotLast[n], start, sizeof(start));
                                g_at4ShotFrame = frameNow ? frameNow : 1;
                            }
                        }
                        if (static ULONGLONG s_said = 0; g_launcherLog.load(std::memory_order_relaxed) && now - s_said >= 1000) {
                            s_said = now;
                            Log("AT4: the rocket starts at (%.2f %.2f %.2f) from the game's eye (left, up, forward)", start[0], start[1], start[2]);
                        }
                    }
                } else if (float bullet[3]; t.gun && s_bulletFrame != frameNow && g_rifleShotOn.load(std::memory_order_relaxed) && ShotStart(t, t.launcherSeen, shotLine, bullet) &&
                    Length3(bullet[0], bullet[1], bullet[2]) < 3.0f) {
                    s_bulletFrame = frameNow;
                    for (size_t n = 0; n < std::size(g_rifleShot); ++n) {
                        const uintptr_t place = g_rifleShot[n].load(std::memory_order_relaxed);
                        if (!place)
                            continue;
                        // Only over its own data, as it was left (DrawGun).
                        float there[11];
                        SIZE_T got = 0;
                        const bool ours = ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(place), there, sizeof(there), &got) && got == sizeof(there) && there[4] == 0.0f && there[5] == 0.0f &&
                                          there[6] == 1.0f && there[8] == 0.0f && there[9] == 0.0f && IsBulletSpeed(there[10]) &&
                                          ((there[0] == 0.0f && there[1] == 0.0f && there[2] == 0.0f) || std::memcmp(there, g_rifleShotLast[n], sizeof(g_rifleShotLast[n])) == 0);
                        if (!ours) {
                            g_rifleShot[n] = 0;
                            continue;
                        }
                        if (WriteProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(place), bullet, sizeof(bullet), nullptr))
                            std::memcpy(g_rifleShotLast[n], bullet, sizeof(bullet));
                    }
                    if (static ULONGLONG s_said = 0; g_rifleLog.load(std::memory_order_relaxed) && now - s_said >= 1000) {
                        s_said = now;
                        Log("Held gun: its bullets start at (%.3f %.3f %.3f) from the game's eye (left, up, forward), on %s%s%s line", bullet[0], bullet[1], bullet[2],
                            glassSight >= 0 ? "the " : "its own sights'", glassSight >= 0 ? kHeldSightNames[glassSight] : "", glassSight >= 0 ? "'s" : "");
                    }
                }
                if (static ULONGLONG s_at = 0; t.gun && g_heldLog.load(std::memory_order_relaxed) && now - s_at >= 1000) {
                    s_at = now;
                    float view[12], rel[12];
                    {
                        std::lock_guard fp(g_fpMutex);
                        std::memcpy(view, g_viewToArms, sizeof(view));
                    }
                    Compose(inverse, view, rel);
                    float yaw = 0.0f, pitch = 0.0f, aim[3] = {};
                    if (camera::GameWeaponAngles(yaw, pitch)) {
                        const float f[3] = {-std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch)};
                        for (int c = 0; c < 3; ++c)
                            aim[c] = frame[c] * f[0] + frame[4 + c] * f[1] + frame[8 + c] * f[2];
                    }
                    Log("Held gun: %u: the view on its frame: eye (%.3f %.3f %.3f), forward (%.4f %.4f %.4f), the aim on it (%.4f %.4f %.4f), FOV source %.1f; palms %s, moving %d", count, rel[3], rel[7],
                        rel[11], -rel[2], -rel[6], -rel[10], aim[0], aim[1], aim[2], camera::SourceFov() * 57.29578f, g_heldPalms.known ? "known" : "not yet", g_heldMoving.load() ? 1 : 0);
                    for (int k = 0; k < 6; ++k) {
                        float on[12];
                        Compose(inverse, game + k * 12, on);
                        Log("Held gun: bone %d on its frame: at (%.3f %.3f %.3f), its z (%.3f %.3f %.3f)", k, on[3], on[7], on[11], on[2], on[6], on[10]);
                    }
                    if (float toGun[12], eyeOn[3]; Invert(t.launcherPose, toGun)) {
                        TransformPoint(toGun, t.eyeInArms, eyeOn);
                        Log("Held gun: the eye on the gun in hand (%.3f %.3f %.3f)", eyeOn[0], eyeOn[1], eyeOn[2]);
                    }
                }
            }
        } else if (!Invert(g_launcherFrameOrig, inverse)) {
            return false;
        }
        // Lying, it's placed from the world: not drawn by a draw without the world's way into it (as the rifle lying, DrawGun).
        if (!t.gun && t.launcherLying && !t.haveDrawnFromWorld)
            return true;
        // Free hands, every gun: each draw as a hand last held the gun, and whether it's the gun's own (HeldKept). Its frame is this draw's own, or the
        // body's drawn in this same view (from another, a frame's bob off while walking).
        const DrawKey thisDraw{count, start, base};
        const bool exact = colour || depth || g_launcherFrameView == g_viewUploads.load();
        HeldKept& kept = g_heldKept[thisDraw];
        if (!t.gun && !HeldOwnDraw(thisDraw, colour || depth, kind, kept, frameNow)) {
            if (!kept.said) {
                kept.said = true;
                Log("Held gun: draw %u %u isn't the gun's: not drawn while it lies", count, start);
            }
            return true;
        }
        Compose(t.launcherPose, inverse, delta);
        for (UINT k = 0; k < kPaletteBones; ++k)
            MoveBy(delta, bones + k * 12);
        const float* drawnFrom = game;  // the bones it's drawn from: the game's now, or as a hand last held it
        if (t.gun) {
            // (Only with the game's arms at rest on it: let go of in the middle of the game's own reload, it lies as it rests, not with its magazine or
            // spare frozen where the game's animation had them.)
            if (!t.launcherGameHands && (exact || !kept.sameView || frameNow - kept.frame > 2)) {
                std::memcpy(kept.game, game, sizeof(kept.game));
                std::memcpy(kept.inverse, inverse, sizeof(kept.inverse));
                kept.have = true;
                kept.sameView = exact;
                kept.frame = frameNow;
            }
            if (exact && !colour && !depth && !kept.moved)
                NoteHeldRigid(kept, inverse, game);
        } else if (kept.have) {
            // No hand on it: every part as it was on it then, moved with it (the blocks below still set what hands.cpp drives: its seated magazine, lever,
            // lid, belt, cover and aerial).
            float lyingDelta[12];
            Compose(t.launcherPose, kept.inverse, lyingDelta);
            for (UINT k = 0; k < kPaletteBones; ++k)
                Compose(lyingDelta, kept.game[k], bones + k * 12);
            drawnFrom = &kept.game[0][0];
        }
        // An attachment drawn with it (gunbodies::kAttach): its bone 0 on the frame, for its points in the gun's body (skin::AttachmentsOnGun).
        if (const gunbodies::Attach* a = gunbodies::AttachFor(count, start, base)) {
            AttachSeen& seen = g_attachSeen[a - gunbodies::kAttach];
            if (float rel0[12]; t.gun && (exact || !seen.sameView || frameNow - seen.relFrame > 2)) {
                Compose(inverse, game, rel0);
                if (std::all_of(rel0, rel0 + 12, [](float f) { return std::isfinite(f); })) {
                    std::memcpy(seen.rel0, rel0, sizeof(seen.rel0));
                    seen.have = true;
                    seen.sameView = exact;
                    seen.relFrame = frameNow;
                }
            }
            seen.frame = frameNow;
        }
        // A sight of the table drawn on it in a hand, for recoil's sight at the eye (skin::Sight): its glass on the frame as the game draws them, from a
        // draw in the frame's own view (as an attachment's, AttachSeen).
        if (t.gun && !colour && !depth) {
            const DrawKey here{count, start, base};
            for (size_t n = 0; n < std::size(kHeldSights); ++n) {
                if (!(here == kHeldSights[n].body))
                    continue;
                GlassSeen& seen = g_glassSeen[n];
                seen.frame = frameNow;
                if (exact || !seen.sameView || frameNow - seen.relFrame > 2) {
                    float lensAt[3], inGame[3], onFrame[3];
                    for (int i = 0; i < 3; ++i)
                        lensAt[i] = kHeldSights[n].eyepiece[i] + g_sightLensOff[n][i];
                    TransformPoint(game, lensAt, inGame);
                    TransformPoint(inverse, inGame, onFrame);
                    if (std::isfinite(onFrame[0]) && std::isfinite(onFrame[1]) && std::isfinite(onFrame[2])) {
                        std::memcpy(seen.onFrame, onFrame, sizeof(seen.onFrame));
                        seen.have = true;
                        seen.sameView = exact;
                        seen.relFrame = frameNow;
                    }
                }
            }
        }
        // A magnified sight on it: as the rifle's scope (DrawArms). Its glass isn't drawn, its body has a window in it. (Only in a hand: lying or flying,
        // its body, glass and marks are drawn moved with it as any part.)
        if (const float scopeOn = g_scopePower.load(std::memory_order_relaxed); t.gun && scopeOn > 1.0f && !colour && !depth) {
            const DrawKey here{count, start, base};
            for (size_t n = 0; n < std::size(kHeldSights); ++n) {
                const HeldSight& sight = kHeldSights[n];
                const bool body = here == sight.body || here == sight.depth;
                if (body)
                    g_heldSightFrame[n] = frameNow;
                const bool glass = here == sight.glass && frameNow - g_heldSightFrame[n] <= 2 && scopezoom::InScene(ctx);
                // Its own reticle, with its lens showing the real one.
                if (sight.ownMarks.indexCount && here == sight.ownMarks && frameNow - g_heldSightFrame[n] <= 2 && frameNow - g_heldLensFrame[n] <= 1)
                    return true;
                if (!glass && !body)
                    continue;
                if (glass)
                    lensSight = static_cast<int>(n);
                float at[3], vp[16], lensAt[3];
                int eye = -1;
                const float* lensOff = g_sightLensOff[n];
                const float radius = sight.radius + lensOff[3];
                for (int i = 0; i < 3; ++i)
                    lensAt[i] = sight.eyepiece[i] + lensOff[i];
                TransformPoint(bones, lensAt, at);
                if (glass) {
                    // Its glass comes in the lit picture's own pass, as the ACOG's: what's seen through it is already there.
                    zoomWanted = ScopeLens(ctx, t, t.launcherPose, at, 0.0f, radius, sight.power, zoomLens);
                    zoomLens.marks = sight.marks;
                    NoteGlass(ctx, here);
                } else if (sight.power > 1.5f) {
                    windowWanted = ViewOfDraw(ctx, vp, eye) && !(scopeview::PictureNow() && sight.power > 1.5f && (eye == 0 || eye == 1)) &&
                                   ScopeWindow(ctx, vp, t.eyeInArms, t.launcherPose, at, 0.0f, radius, sight.power, window);
                    if (windowWanted)
                        NoteLensHole(eye, window);
                }
            }
        }
        // The Stinger's aerial, folded or on its way up in the hand (flap.cpp).
        if ((colour || depth) && kind.spreadGun == kStingerGun && t.heldPanels)
            for (int b = 0; b < 4; ++b)
                Compose(t.launcherPose, t.heldPanelBones[b], bones + (3 + b) * 12);
        // A tube shotgun's bolt group drawn back (held open, by its handle, on its way home: shellload.h): at that place on the frame, whatever the
        // game's own animation has it at (the game's count keeps its hidden round, so its own bolt went home). In its depth pass the game's own shell
        // goes with its shell's draw (above).
        if (tube && (colour || depth)) {
            if (t.heldTube && t.heldBoltBack > 0.0f) {
                const float on[12] = {1, 0, 0, tube->boltRest[0], 0, 1, 0, tube->boltRest[1], 0, 0, 1, tube->boltRest[2] - t.heldBoltBack};
                Compose(t.launcherPose, on, bones + (depth ? tube->boltDepth : tube->bolt) * 12);
            }
            if (depth && !shellOwn)
                std::memset(bones + tube->shellDepth * 12, 0, 48);  // all on one point
        }
        // The port's cover, turned up from where the mesh has it hanging by as much as it's shut. (Under the game's own reload too, which knows nothing of it.)
        if (mag && (colour || depth) && (mag->cover >= 0 || mag->coverCount) && t.heldCover < 1.0f) {
            const float angle = (t.heldCover - 1.0f) * kPortCoverShut, c = std::cos(angle), s = std::sin(angle), hx = mag->coverHinge[0], hy = mag->coverHinge[1];
            const float turn[12] = {c, -s, 0.0f, hx - (c * hx - s * hy), s, c, 0.0f, hy - (s * hx + c * hy), 0.0f, 0.0f, 1.0f, 0.0f};
            // The other side: the same plate mirrored across its hinge, facing the other way, its thickness between them.
            const float mirror[12] = {-1.0f, 0.0f, 0.0f, 2.0f * hx, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
            float toFrame[12], about[12], turned[12];
            if (Invert(t.launcherPose, toFrame)) {
                Compose(t.launcherPose, turn, about);
                Compose(about, toFrame, coverMove);
                Compose(turn, mirror, turned);
                Compose(t.launcherPose, turned, about);
                Compose(about, toFrame, coverOther);
                coverMoved = true;
                if (mag->cover >= 0) {
                    coverBoneIndex = depth && mag->coverDepth >= 0 ? mag->coverDepth : mag->cover;
                    std::memcpy(coverOtherBone, bones + coverBoneIndex * 12, sizeof(coverOtherBone));
                    MoveBy(coverOther, coverOtherBone);
                    MoveBy(coverMove, bones + coverBoneIndex * 12);
                } else {
                    coverRun = colour ? mag->coverStart : mag->coverDepthStart;
                    coverRunCount = mag->coverCount;
                    coverFrame = depth && kind.depthFrame >= 0 ? kind.depthFrame : kind.frameBone;
                }
            }
        }
        // Its magazine by hand (handmag.cpp), unless the game's own reload is playing, which moves them all itself. (With no hand on it, never: its parts are
        // drawn as a hand last held it, its spare hidden, and what hands.cpp drives, as the launcher's are: DrawLauncher.)
        if (mag && (!t.launcherGameHands || !t.gun)) {
            HeldSeats(*mag, colour, depth, count, start, seatBone, seatIndex);
            partDraw = mag->partCount && count == mag->partCount && start == mag->partStart;
            // The game's "hidden" is a scale of 1e-4, not a zero.
            const auto squashedIn = [](const float* from, int b) {
                const float* m = from + b * 12;
                return m[0] * m[0] + m[4] * m[4] + m[8] * m[8] < 0.25f;
            };
            const auto squashed = [&](int b) { return squashedIn(game, b); };
            const int spare = colour ? mag->spareColour : depth ? mag->spareDepth : -1;
            // Only while the game still shows the magazine itself: in the first moments of its own reload the spare is what's seated, before its hands have moved far enough to say so.
            // (As it's drawn: with no hand on it, as a hand last held it.)
            if (spare >= 0 && !squashedIn(drawnFrom, seatBone[0]))
                std::memset(bones + spare * 12, 0, 48);  // all on one point
            spareHide = spare >= 0;
            const bool leverPulled = t.heldLeverBack > 0.0f || t.heldLeverSwing != 0.0f || t.heldLeverNotch != 0.0f;
            if ((colour || depth) && mag->lever >= 0 && leverPulled)
                PullLever(bones + (depth && mag->leverDepth >= 0 ? mag->leverDepth : mag->lever) * 12, *mag->gun, t.heldLeverNotch, t.heldLeverSwing, t.heldLeverBack,
                          t.launcherPose);
            // A handle with no bone of its own (handleruns.h: the Mk11's T handle, a part of its frame), on this pass's own draw of its body: drawn apart from the
            // body below, its runs with the frame's bone pulled as a lever's bone is. (weapon.txt "handleruns 0": left home, as before.)
            if ((colour || depth) && mag->lever < 0 && leverPulled && g_handleRuns.load(std::memory_order_relaxed) > 0) {
                if (const handleruns::Handle* h = handleruns::For(kind.bodyIndices)) {
                    const handleruns::Pass& pass = colour ? h->colour : h->depth;
                    if (start == pass.drawStart && count == pass.drawCount) {
                        handle = h, handlePass = &pass;
                        handleFrame = depth && kind.depthFrame >= 0 ? kind.depthFrame : kind.frameBone;
                        handleGun = mag->gun;
                        handleNotch = t.heldLeverNotch, handleSwing = t.heldLeverSwing, handleBack = t.heldLeverBack;
                        std::memcpy(handlePose, t.launcherPose, sizeof(handlePose));
                    }
                }
            }
            const bool out = g_magOut.load(std::memory_order_relaxed);
            magHide = out || t.heldMagHidden;
            if (mag->beltCount && out) {  // (with the box out: not with the hand only closed on it)
                if (count == mag->beltCount && start == mag->beltStart)
                    return true;
                if (depth)
                    std::memset(bones, 0, static_cast<size_t>(mag->beltDepthBones) * 48);
            } else if (mag->beltCount && mag->gun->belt && ((count == mag->beltCount && start == mag->beltStart) || depth)) {
                for (int b = 0; b < mag->beltDepthBones && b < 5; ++b) {
                    float* bone = bones + b * 12;
                    if (t.heldBeltDrawn) {  // hanging out of a new box, or in the hand
                        Compose(t.launcherPose, t.heldBeltBones[b], bone);
                    } else if (b >= t.heldBeltPieces) {  // gone through the feed: it shortens as it fires
                        std::memset(bone, 0, 48);
                    } else if (bone[0] * bone[0] + bone[4] * bone[4] + bone[8] * bone[8] < 0.25f) {
                        // Lying in the feed the game moves it as it fires, but it squashes these pieces by its own count, which doesn't follow a reload by hand (the user, 2026-11-05: "sometimes I see it, sometimes I don't"): by the true count.
                        Compose(t.launcherPose, mag->gun->belt->laid[b], bone);
                    }
                }
            }
            // Its lid, up.
            if ((colour || depth) && mag->lid >= 0 && t.heldLid > 0.0f && mag->gun->belt) {  // (the Stinger's aerial: below, it has no mag)
                float turn[12];
                lidride::Turn(*mag->gun->belt, t.heldLid, turn);
                float toFrame[12], about[12], move[12];
                if (Invert(t.launcherPose, toFrame)) {
                    Compose(t.launcherPose, turn, about);
                    Compose(about, toFrame, move);
                    MoveBy(move, bones + (depth ? mag->lidDepth : mag->lid) * 12);
                }
            }
            // What rides its lid (lidride.h: a draw of its own the game skins to the cover's bone, as a sight on the cover's rail may be: the bridge M249's ACOG),
            // up with it: each bone of such a draw that is the cover's bone as the body's draw has it this frame, turned as that bone is. The cover's bone noted
            // from the body's draw (as it's drawn from: the game's own, or as a hand last held it); the gun's own other draws (its belt, its box, its rounds) are
            // its own, above, and never ride it. Only a gun with a lid that carries what rides it (the M249's; handmag::BeltGun::carries), with the lid up by
            // hand: every other gun and draw as before, bit for bit (the M240's too, as it was approved).
            if (mag->lid >= 0 && mag->gun->belt && mag->gun->belt->carries) {
                const float* frameInverse = drawnFrom == game ? inverse : kept.inverse;
                if (colour) {
                    Compose(frameInverse, drawnFrom + mag->lid * 12, g_heldLidOnFrame);
                    g_heldLidFrame = frameNow;
                    g_heldLidKind = kind.bodyIndices;
                } else if (!depth && t.heldLid > 0.0f && (exact || drawnFrom != game) && g_heldLidKind == kind.bodyIndices && frameNow - g_heldLidFrame <= 2 &&
                           !(mag->beltCount && count == mag->beltCount && start == mag->beltStart) && !(mag->partCount && count == mag->partCount && start == mag->partStart) &&
                           !(mag->roundsCount && count == mag->roundsCount && start == mag->roundsStart)) {
                    float turn[12], toFrame[12], about[12], move[12];
                    lidride::Turn(*mag->gun->belt, t.heldLid, turn);
                    if (Invert(t.launcherPose, toFrame)) {
                        Compose(t.launcherPose, turn, about);
                        Compose(about, toFrame, move);
                        for (UINT k = 0; k < kPaletteBones; ++k) {
                            float on[12];
                            Compose(frameInverse, drawnFrom + k * 12, on);
                            if (lidride::Rides(g_heldLidOnFrame, on))
                                MoveBy(move, bones + k * 12);
                        }
                    }
                }
            }
            // A fresh one pressed against it: it gives a little, about its hook, and comes back.
            if (t.heldOldRocked && !magHide) {
                float toFrame[12], about[12], move[12];
                if (Invert(t.launcherPose, toFrame)) {
                    Compose(t.launcherPose, t.heldOldRock, about);
                    Compose(about, toFrame, move);
                    for (int s = 0; s < 2; ++s)
                        if (seatBone[s] >= 0)
                            MoveBy(move, bones + seatBone[s] * 12);
                }
            }
            for (int s = 0; s < 2; ++s) {
                if (seatBone[s] < 0)
                    continue;
                const int i = seatIndex[s];
                // From the game's own bones, whether or not the mod's hiding the magazine: each gun keeps its own (RigFor), and it comes back with one out.
                // With gloves: while the game's palms rest on the gun (NoteHeldPalms). With bare arms (Semper Fidelis's: no gloves' draw, the palms never known) the magazine
                // says it itself: unchanged on the frame for half a second it's seated, as in the game's own reload it never is (seatrest.h). So too with gloves while
                // the palms aren't known and no seat is (a gun drawn the first time prone or sprinting, or in its first second), and while the stance or a sprint
                // moves the game's hands: their moving isn't told then, and the game's own reload's magazine on its way was taken as the seat (kept through a
                // change of gun or a death, centimetres off: the review of 2026-11-19).
                // (seatrest.h's Take: tools\magseattest.cpp and seatresttest.cpp run it as it is.)
                if (!squashed(seatBone[s])) {
                    const bool bare = now - g_bareSeenAt.load(std::memory_order_relaxed) < 250, had = g_heldSeats.have[i], stanceMoves = input::Sprinting();
                    float on[12];
                    Compose(inverse, game + seatBone[s] * 12, on);
                    if (seatrest::Take(g_heldSeats, i, g_heldSeatRest[colour || depth ? i : kMagBones + i], on, g_heldPalms.known, g_heldMoving.load(std::memory_order_relaxed),
                                       stanceMoves, bare, now) == seatrest::Took::kAtRest &&
                        !had) {
                        if (bare)
                            Log("Held gun: bare arms: its magazine's seat %d taken from the magazine at rest on its frame", i);
                        else
                            Log("Held gun: the %s's magazine's seat %d taken from the magazine at rest on its frame (the game's palms on it %s%s)", gunprofile::NameOf(kind.shape), i,
                                g_heldPalms.known ? "known" : "not seen at rest yet", stanceMoves ? ", the stance or a sprint moving them" : "");
                    }
                }
                if (t.mag && g_heldSeats.have[i]) {
                    if (!magDraw) {
                        t_heldMag.assign(kGloveUploadSize, 0);  // every other bone of it on one point
                        magDraw = true;
                    }
                    Compose(t.magPose, g_heldSeats.seat[i], reinterpret_cast<float*>(t_heldMag.data() + seatBone[s] * 48));
                } else if (t.mag) {
                    NoteMagNotDrawn(frameNow, now, i, false);
                }
            }
        } else if (mag && t.mag && colour) {
            // The game's own moves on it followed (its reload, or its palms off their rest): a magazine in a hand isn't drawn meanwhile. Counted, with its own
            // reason, so a log tells it from a seat not known (the review of 2026-11-19). (Nothing else of the block: seatBone stays -1, as before.)
            NoteMagNotDrawn(frameNow, now, 0, true);
        }
    }
    const bool shown = keepDrop(false);
    if (partDraw)
        SayPartDraw(ctx, u.resource, count, start);  // (once; out of the hands' lock)
    NoNearCut nearEye(ctx, !g_gunNearCut.load(std::memory_order_relaxed));
    bool lensMarked = false;
    if (zoomWanted && scopezoom::Begin(ctx, zoomLens, &lensMarked)) {
        t_noHole = true;
        DrawWithPalette(which, ctx, count, start, base, u.resource, t_rig.data(), kGloveUploadSize);
        t_noHole = false;
        scopezoom::End(ctx);
        if (lensSight >= 0 && lensMarked)
            g_heldLensFrame[lensSight] = frameNow;
        if (!g_scopeGlassOver.load(std::memory_order_relaxed))
            return true;
    }
    if (windowWanted && DrawWithWindow(which, ctx, count, start, base, u.resource, t_rig.data(), window)) {
        ++g_handsMovedDraws;
        return true;
    }
    {
        NoCull bothFaces(ctx, twoSided);
        const bool coverIsRun = coverMoved && coverRunCount && coverRun >= start && coverRun + coverRunCount <= start + count;
        thread_local std::vector<uint8_t> t_coverRig;
        // A cover that's part of the body, after the body round it: on its own, turned, and its other side.
        const auto drawCoverRun = [&] {
            t_coverRig = t_rig;
            MoveBy(coverMove, reinterpret_cast<float*>(t_coverRig.data()) + coverFrame * 12);
            DrawWithPalette(which, ctx, coverRunCount, coverRun, base, u.resource, t_coverRig.data(), kGloveUploadSize);
            // And its other side.
            t_coverRig = t_rig;
            MoveBy(coverOther, reinterpret_cast<float*>(t_coverRig.data()) + coverFrame * 12);
            MirroredFaces mirrored(ctx);
            DrawWithPalette(which, ctx, coverRunCount, coverRun, base, u.resource, t_coverRig.data(), kGloveUploadSize);
        };
        // A handle with no bone of its own, pulled (handleruns.h): this pass's draw less its runs (and less a cover that's a run of it), then its runs with the
        // frame's bone pulled as a lever's, its shaft out of the receiver behind it, the cover. Runs that don't fit the draw: drawn whole, as before.
        handleruns::Run cuts[handleruns::kMaxRuns + 1], gaps[handleruns::kMaxRuns + 2];
        int gapCount = -1;
        if (handlePass && handleGun) {
            int cutCount = 0;
            for (int i = 0; i < handlePass->runCount && i < handleruns::kMaxRuns; ++i)
                cuts[cutCount++] = handlePass->runs[i];
            if (coverIsRun)
                cuts[cutCount++] = {coverRun, coverRunCount};
            gapCount = handleruns::Gaps(start, count, cuts, cutCount, gaps);
        }
        if (gapCount >= 0) {
            if (!DrawRangesWithPalette(which, ctx, gaps, gapCount, base, u.resource, t_rig.data(), kGloveUploadSize))
                return false;
            thread_local std::vector<uint8_t> t_handleRig;
            t_handleRig = t_rig;
            PullLever(reinterpret_cast<float*>(t_handleRig.data()) + handleFrame * 12, *handleGun, handleNotch, handleSwing, handleBack, handlePose);
            DrawRangesWithPalette(which, ctx, handlePass->runs, handlePass->runCount, base, u.resource, t_handleRig.data(), kGloveUploadSize);
            // Its shaft, a handle drawn straight back whose mesh has none: each face the stem's flat top taken onto it, after the frame's bone as drawn.
            const bool shaft = handlePass->shaftTop.count && handleSwing == 0.0f && handleNotch == 0.0f && g_handleRuns.load(std::memory_order_relaxed) >= 2;
            if (shaft) {
                const float* frame = reinterpret_cast<const float*>(t_rig.data()) + handleFrame * 12;
                for (int face = 0; face < handleruns::kShaftFaces; ++face) {
                    float onFrame[12];
                    if (!handleruns::ShaftFace(*handle, face, handleBack, onFrame))
                        break;
                    t_handleRig = t_rig;
                    Compose(frame, onFrame, reinterpret_cast<float*>(t_handleRig.data()) + handleFrame * 12);
                    DrawRangesWithPalette(which, ctx, &handlePass->shaftTop, 1, base, u.resource, t_handleRig.data(), kGloveUploadSize);
                }
            }
            NoteHandleDrawn(*handle, depth, handleBack, shaft, frameNow);
            if (coverIsRun)
                drawCoverRun();
        } else if (coverIsRun) {
            // A cover that's part of the body: the body round it, then it on its own, turned.
            if (!DrawWithPalette(which, ctx, coverRun - start, start, base, u.resource, t_rig.data(), kGloveUploadSize))
                return false;
            DrawWithPalette(which, ctx, start + count - coverRun - coverRunCount, coverRun + coverRunCount, base, u.resource, t_rig.data(), kGloveUploadSize);
            drawCoverRun();
        } else if (!DrawWithPalette(which, ctx, count, start, base, u.resource, t_rig.data(), kGloveUploadSize, false, 0, everySlot())) {
            return false;
        }
        ++g_handsMovedDraws;
        // A cover on its own bone: its other side, all the rest on one point.
        if (coverMoved && coverBoneIndex >= 0) {
            thread_local std::vector<uint8_t> t_coverOther;
            t_coverOther.assign(kGloveUploadSize, 0);
            std::memcpy(t_coverOther.data() + coverBoneIndex * 48, coverOtherBone, sizeof(coverOtherBone));
            MirroredFaces mirrored(ctx);
            DrawWithPalette(which, ctx, count, start, base, u.resource, t_coverOther.data(), kGloveUploadSize);
        }
    }
    drawCopies(shown);
    drawShells();
    return true;
}

// weapon.txt "drawscan 60": for that many frames, a line for each kind of draw using a bone palette of the player's
// own (near the arms' origin): how a new gun's draws are found. With "reloadrec" running, each such draw is recorded
// too, its index count as the kind.
void ScanOwnDraw(ID3D11DeviceContext* ctx, UINT count, UINT start, INT base) {
    const Upload& u = t_upload;
    if (!u.resource || u.bytes.size() != kGloveUploadSize || !PlayersOwn(u.bytes) || BoundSlot(ctx, u.resource) < 0)
        return;
    if (g_drawScanRec.load(std::memory_order_relaxed) && g_recUntil.load(std::memory_order_relaxed))
        Record(count, -1, static_cast<int>(start), u.bytes, kPaletteBones);
    if (g_drawScan.load(std::memory_order_relaxed) <= 0)
        return;
    // With "glassdump" too: how each small part of the first person's models is drawn, a scope's zoomed one's above all.
    if (count < 4000 && g_glassDump.load(std::memory_order_relaxed)) {
        ID3D11DepthStencilState* state = nullptr;
        UINT stencil = 0;
        ctx->OMGetDepthStencilState(&state, &stencil);
        if (state)
            state->Release();
        if (stencil == 32 && scopezoom::InScene(ctx) && NoteGlass(ctx, DrawKey{count, start, base})) {
            // Where the view is, in this part's bone 0: how far its marks are from the zoomed eye, and where that eye's line crosses them.
            float view[12], inverse[12], rel[12];
            {
                std::lock_guard fp(g_fpMutex);
                std::memcpy(view, g_viewToArms, sizeof(view));
            }
            if (Invert(reinterpret_cast<const float*>(u.bytes.data()), inverse)) {
                Compose(inverse, view, rel);
                Log("Glass %u/%u: the view in its bone 0: eye (%.4f %.4f %.4f), forward (%.4f %.4f %.4f), the game's FOV %.2f", count, start, rel[3], rel[7], rel[11], -rel[2], -rel[6], -rel[10], camera::SourceFovRaw() * 57.29578f);
            }
        }
    }
    static std::mutex s_mutex;
    static std::unordered_set<uint64_t> s_seen;
    {
        std::lock_guard lock(s_mutex);
        if (!s_seen.insert((static_cast<uint64_t>(count) << 32) ^ (static_cast<uint64_t>(start) << 4) ^
                           static_cast<uint32_t>(base)).second)
            return;
    }
    const auto* b = reinterpret_cast<const float*>(u.bytes.data());
    int used = 0;
    for (UINT k = 0; k < kPaletteBones; ++k)
        if (b[k * 12] != 0.0f || b[k * 12 + 5] != 0.0f)
            ++used;
    ID3D11Buffer* vb = nullptr;
    UINT stride = 0, vbOffset = 0;
    ctx->IAGetVertexBuffers(0, 1, &vb, &stride, &vbOffset);
    if (vb)
        vb->Release();
    ID3D11DepthStencilState* depth = nullptr;
    UINT ref = 0;
    ctx->OMGetDepthStencilState(&depth, &ref);
    if (depth)
        depth->Release();
    Log("Draw scan: %u %u %d stride %u stencil %u: %d bones used, bone 0 at (%.3f %.3f %.3f), bone 1 (%.3f %.3f %.3f)", count,
        start, base, stride, ref, used, b[3], b[7], b[11], b[12 + 3], b[12 + 7], b[12 + 11]);
    // A gun's body: where the view is on each of its first bones. Zoomed, that's its sights' line on whichever is its frame.
    if (ref == kFirstPersonStencil && stride == 48 && count >= kHeldBodyLeast) {
        float view[12];
        {
            std::lock_guard fp(g_fpMutex);
            std::memcpy(view, g_viewToArms, sizeof(view));
        }
        for (int k = 0; k < 6; ++k) {
            float inverse[12], rel[12];
            if (!Invert(b + k * 12, inverse))
                continue;
            Compose(inverse, view, rel);
            Log("Draw scan: %u: the view in its bone %d: eye (%.3f %.3f %.3f), forward (%.4f %.4f %.4f), the game's FOV %.2f", count, k, rel[3], rel[7], rel[11], -rel[2], -rel[6], -rel[10], camera::SourceFovRaw() * 57.29578f);
        }
    }
}

void DrawIndexed(int which, ID3D11DeviceContext* ctx, UINT count, UINT start, INT base) {
    postpin::Pin pin(ctx, count);  // the vignette's constants held as at 100 degrees for this draw (postpin.h)
    ++t_drawSerial;
    if (g_drawScan.load(std::memory_order_relaxed) > 0 ||
        (g_drawScanRec.load(std::memory_order_relaxed) && g_recUntil.load(std::memory_order_relaxed)))
        ScanOwnDraw(ctx, count, start, base);
    switch (effects::NoteDrawIndexed(ctx, count, start, base)) {
    case effects::kHide:
        return;  // weapon.txt "hidefx" / "hideps"

    case effects::kQuads:  // a batch of particles, the flash's among them
        if (effects::DrawQuads(ctx, count, start, base, [&](INT moved) { g_origDrawIndexed[which](ctx, count, start, moved); }))
            return;
        break;

    default:
        break;
    }
    if (scopeview::Drawing()) {  // the view through a scope: nothing of the hands in it
        g_origDrawIndexed[which](ctx, count, start, base);
        return;
    }
    if (!g_research.load(std::memory_order_relaxed)) {
        // Only the hands' and the gun's own draws matter: a couple of comparisons, no lock.
        if (g_handShift.load(std::memory_order_relaxed)) {
            DrawKey key{count, start, base};
            for (const HandDraw& h : kHandDraws)
                if (h.key == key && DrawShifted(which, ctx, count, start, base, h.bones))
                    return;
        }
        DrawKey key{count, start, base};
        if (g_dumpWanted.load(std::memory_order_acquire) && key == g_dumpKey)
            CaptureMesh(ctx, key);
        // A held gun with no collision shape yet: its mesh is taken once, to make the shape from on this PC (shapemake.h).
        // Only with the player's own palette, the whole upload, bound for this very draw (as ScanOwnDraw has it: a dump took another draw's bones otherwise).
        if (shapemake::Wants(count, start)) {
            const bool own = t_upload.resource && t_upload.bytes.size() == kGloveUploadSize && PlayersOwn(t_upload.bytes) && BoundSlot(ctx, t_upload.resource) >= 0;
            shapemake::NoteDraw(ctx, count, start, base, own ? t_upload.bytes.data() : nullptr, own ? t_upload.bytes.size() : 0);
        }
        // What's in the hands, its stock too, is drawn right up to the eye: cut off by the game's near plane, parts of it go invisible aiming down the sights (the user, 2026-11-05). In each of its passes, or the colour pass finds no depth to match.
        const bool uncut = !g_gunNearCut.load(std::memory_order_relaxed);
        if (key == kBarePistol) {
            NoNearCut nearEye(ctx, uncut);
            if (DrawBarePistol(which, ctx, count, start, base))
                return;
        }
        if (key == kLauncherA || key == kLauncherB || key == kLauncherC || key == kLauncherDepth) {
            NoNearCut nearEye(ctx, uncut);
            if (DrawLauncher(which, ctx, count, start, base, key))
                return;
        }
        // The bare arms and the body with them, handcuffed or with the cuffs off (barestate.h: the same arm mesh from another start, the body's draws moved).
        const barestate::Kind bare = barestate::KindOf(count, start, base);
        if (bare == barestate::kArms || bare == barestate::kCuff) {
            if (bare == barestate::kArms && t_upload.resource && t_upload.bytes.size() == kGloveUploadSize && PlayersOwn(t_upload.bytes)) {
                g_bareSeenAt.store(GetTickCount64(), std::memory_order_relaxed);
                if (const int state = barestate::ArmsState(count, start, base); state >= 0 && g_bareArmsState.exchange(state, std::memory_order_relaxed) != state)
                    Log("Bare arms: drawn %s (%u indices from %u)", barestate::kStates[state].name, count, start);
            }
            NoNearCut nearEye(ctx, uncut);
            if (DrawBareArms(which, ctx, count, start, base, bare == barestate::kCuff))
                return;
        }
        if (bare == barestate::kBody) {
            // The torso with its sleeves and the strip over the shoulders: on the arms as they were just drawn. Everything else of the body (and those two without an
            // arm solved whole) hidden while the arms are on the controllers, as before.
            if (const barestate::Part part = barestate::PartOf(count, start, base); part == barestate::kTorso || part == barestate::kStripPart) {
                NoNearCut nearEye(ctx, uncut);
                if (DrawBareSleeves(which, ctx, count, start, base, part))
                    return;
            }
            if (HideBareBody())
                return;
        }
        if (key == kScope && g_weaponWatch.load(std::memory_order_relaxed))
            ProbeScope(ctx);
        // The grenade launcher's tube with the player's bones, the mod's hands on or not: it's on this rifle (skin::RifleM320Seen).
        if (key == DrawKey{7848, 0, 0} && t_upload.resource && t_upload.bytes.size() == kGloveUploadSize && PlayersOwn(t_upload.bytes)) {
            g_m320MetAt.store(GetTickCount64(), std::memory_order_relaxed);
            g_m320Met.store(true, std::memory_order_relaxed);
        }
        size_t pass = WeaponPass(key);
        if (pass < kWeaponPassCount) {
            if (g_weaponWatch.load(std::memory_order_relaxed))
                WatchDraw(pass);
            if (g_weaponShift.load(std::memory_order_relaxed) && g_weaponSelected[pass].any &&
                DrawShifted(which, ctx, count, start, base, kPaletteBones, &g_weaponSelected[pass]))
                return;
            NoNearCut nearEye(ctx, uncut);
            if (DrawGun(which, ctx, count, start, base, pass))
                return;
        } else if (g_handsActive.load(std::memory_order_relaxed)) {
            if (const ArmDraw* arms = FindArmDraw(key)) {
                bool rig = false, moved = false;
                {
                    NoNearCut nearEye(ctx, uncut);
                    rig = DrawLauncherRig(which, ctx, count, start, base, *arms);
                    moved = rig || DrawArms(which, ctx, count, start, base, *arms);
                }
                if (g_armTrace.load(std::memory_order_relaxed) > 0) {
                    const auto* b = t_upload.bytes.size() >= 48 ? reinterpret_cast<const float*>(t_upload.bytes.data()) : nullptr;
                    Log("Arm trace: frame %u view %u: %u %u: %s; upload %u bytes, own %d, bone 0 (%.2f %.2f %.2f), rig %d, rifle %llu ms ago, launcher %llu ms ago",
                        g_frameSerial.load(), g_viewUploads.load(), count, start, rig ? "on the launcher" : moved ? "on the rifle" : "NOT MOVED",
                        static_cast<unsigned>(t_upload.bytes.size()), PlayersOwn(t_upload.bytes) ? 1 : 0, b ? b[3] : 0.0f, b ? b[7] : 0.0f, b ? b[11] : 0.0f,
                        g_rigThisFrame.load() ? 1 : 0, static_cast<unsigned long long>(GetTickCount64() - g_rifleDrawn.load()),
                        static_cast<unsigned long long>(GetTickCount64() - g_launcherAt.load()));
                }
                if (moved)
                    return;
            }
            else if (DrawHeldPart(which, ctx, count, start, base))
                return;
        }
        if (g_fpProbeFrames.load(std::memory_order_relaxed) > 0)
            ProbeFirstPerson(ctx, count, start, base);
        if (g_psLogFrames.load(std::memory_order_relaxed) > 0)
            LogByShader(ctx, count, start, base);
        if (overlay::Draw(ctx, count, [&] { g_origDrawIndexed[which](ctx, count, start, base); }, true, start, base))
            return;
        g_origDrawIndexed[which](ctx, count, start, base);
        return;
    }
    bool skinned = t_drawsSinceBones < kSkinnedWithin;
    ++t_drawsSinceBones;
    if (g_huntFirstPerson.load(std::memory_order_relaxed)) {
        ID3D11DepthStencilState* state = nullptr;
        UINT ref = 0;
        ctx->OMGetDepthStencilState(&state, &ref);
        if (state)
            state->Release();
        if (ref & kFirstPersonStencil) {
            DrawKey key{count, start, base};
            {
                std::lock_guard lock(g_drawMutex);
                ++g_firstPerson[key];
                if (g_haveFinal && key == g_final)
                    g_finalSeen = true;
            }
            const DrawSet* hidden = g_hidden.load(std::memory_order_acquire);
            if (hidden && hidden->count(key))
                return;
        }
    }
    if (skinned) {
        DrawKey key{count, start, base};
        UINT handBones = 0;
        for (const HandDraw& h : kHandDraws)
            if (h.key == key)
                handBones = h.bones;
        {
            std::lock_guard lock(g_drawMutex);
            ++g_skinned[key];
            if (g_haveFinal && key == g_final) {
                g_finalBones = t_bones;
                g_finalSeen = true;
            }
        }
        const DrawSet* hidden = g_hidden.load(std::memory_order_acquire);
        if (hidden && hidden->count(key))
            return;
        if (handBones && g_handShift.load(std::memory_order_relaxed) &&
            DrawShifted(which, ctx, count, start, base, handBones))
            return;
        size_t pass = WeaponPass(key);
        if (pass < kWeaponPassCount && g_weaponShift.load(std::memory_order_relaxed) &&
            g_weaponSelected[pass].any &&
            DrawShifted(which, ctx, count, start, base, kPaletteBones, &g_weaponSelected[pass]))
            return;
    } else if (g_handShift.load(std::memory_order_relaxed) && DrawKey{count, start, base} == kGloves) {
        ++g_notSkinned;  // no bone upload just before it: this pass reuses an earlier one
    }
    g_origDrawIndexed[which](ctx, count, start, base);
}

void STDMETHODCALLTYPE DrawIndexed0(ID3D11DeviceContext* c, UINT count, UINT start, INT base) {
    DrawIndexed(0, c, count, start, base);
}
void STDMETHODCALLTYPE DrawIndexed1(ID3D11DeviceContext* c, UINT count, UINT start, INT base) {
    DrawIndexed(1, c, count, start, base);
}

HRESULT STDMETHODCALLTYPE Map0(ID3D11DeviceContext* c, ID3D11Resource* r, UINT s, D3D11_MAP t, UINT f,
                               D3D11_MAPPED_SUBRESOURCE* m) {
    return Map(0, c, r, s, t, f, m);
}
HRESULT STDMETHODCALLTYPE Map1(ID3D11DeviceContext* c, ID3D11Resource* r, UINT s, D3D11_MAP t, UINT f,
                               D3D11_MAPPED_SUBRESOURCE* m) {
    return Map(1, c, r, s, t, f, m);
}
void STDMETHODCALLTYPE Unmap0(ID3D11DeviceContext* c, ID3D11Resource* r, UINT s) {
    Unmap(0, c, r, s);
}
void STDMETHODCALLTYPE Unmap1(ID3D11DeviceContext* c, ID3D11Resource* r, UINT s) {
    Unmap(1, c, r, s);
}

// The camera-relative kinds of block, and where world-space blocks within 3 m sat relative to the
// eyes, since the last press.
void LogSeen() {
    std::vector<std::pair<Signature, Seen>> rows;
    UINT total = 0;
    for (const auto& [s, seen] : g_seen) {
        total += seen.uploads;
        if (seen.viewUploads)
            rows.push_back({s, seen});
    }
    std::sort(rows.begin(), rows.end(),
              [](const auto& a, const auto& b) { return a.second.viewUploads > b.second.viewUploads; });
    Log("Skin: %zu kinds of bone block, %u uploads; %zu kinds camera-relative (within %.1f m of the origin)",
        g_seen.size(), total, rows.size(), kViewModel);
    for (size_t i = 0; i < rows.size() && i < 15; ++i)
        Log("Skin:   %u matrices (%s) at +%u in %u-byte uploads: %u of %u uploads camera-relative (%u appended "
            "uploads), nearest %.2f m from origin",
            rows[i].first.count, rows[i].first.layout == 0 ? "3x4" : "4x4", rows[i].first.start, rows[i].first.size,
            rows[i].second.viewUploads, rows[i].second.uploads, rows[i].second.appended,
            rows[i].second.nearestOrigin);

    std::vector<std::pair<Cell, UINT>> cells(g_cells.begin(), g_cells.end());
    std::sort(cells.begin(), cells.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
    Log("Skin: world-space blocks within 3 m, by where they sat relative to the eyes (%zu cells of %.1f m):",
        cells.size(), kBucket);
    for (size_t i = 0; i < cells.size() && i < 12; ++i)
        Log("Skin:   right %+.1f..%+.1f  up %+.1f..%+.1f  forward %+.1f..%+.1f  (%s, %s): %u uploads",
            cells[i].first.right * kBucket, (cells[i].first.right + 1) * kBucket, cells[i].first.up * kBucket,
            (cells[i].first.up + 1) * kBucket, cells[i].first.forward * kBucket,
            (cells[i].first.forward + 1) * kBucket, cells[i].first.layout == 0 ? "3x4" : "4x4",
            cells[i].first.appended ? "appended" : "fresh", cells[i].second);
    g_seen.clear();
    g_cells.clear();
}

// The last second's watch, one line per pass: the bones that moved most against the reference.
void LogWatch() {
    std::lock_guard lock(g_watchMutex);
    for (size_t p = 0; p < kWeaponPassCount; ++p) {
        WatchPass& w = g_watch[p];
        std::vector<std::pair<float, UINT>> moved;
        for (UINT k = 0; w.any && k < kPaletteBones; ++k) {
            float range = Length3(w.hi[k][0] - w.lo[k][0], w.hi[k][1] - w.lo[k][1], w.hi[k][2] - w.lo[k][2]);
            if (range > 0.005f)
                moved.push_back({range, k});
        }
        std::sort(moved.begin(), moved.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
        char text[512] = {};
        int used = 0;
        for (size_t i = 0; i < moved.size() && i < 12 && used < 460; ++i) {
            UINT k = moved[i].second;
            used += snprintf(text + used, sizeof(text) - used, " %u: %.1f cm (now %+.2f %+.2f %+.2f);", k,
                             moved[i].first * 100.0f, w.hi[k][0], w.hi[k][1], w.hi[k][2]);
        }
        Log("Weapon watch: draw %u/%u, %u draws; bones moving against bone %u:%s", kWeaponPasses[p].indexCount,
            kWeaponPasses[p].startIndex, w.draws, p == 0 ? kReceiverBone : 0u, moved.empty() ? " none" : text);
        w = WatchPass{};
    }
    if (!g_tracing.load()) {
        std::lock_guard lock(g_traceMutex);
        for (size_t i = 0; i < g_trace.size(); ++i) {
            const TraceRow& r = g_trace[i];
            Log("Trace %3zu: %.4f s, thread %5lu, view %u: eye (%.3f %.3f %.3f) gun (%.3f %.3f %.3f) gap (%.3f %.3f %.3f)", i,
                r.time - g_trace[0].time, r.thread, r.viewUploads, r.eye[0], r.eye[1], r.eye[2], r.gun[0], r.gun[1],
                r.gun[2], r.gun[0] - r.eye[0], r.gun[1] - r.eye[1], r.gun[2] - r.eye[2]);
        }
        g_trace.clear();
    }
    if (g_handsActive.load()) {
        Log("Hands: last second's gun and arm draws: %u moved, %u drawn unmoved", g_handsMovedDraws.exchange(0),
            g_handsUnmoved.exchange(0));
        Log("Hands: placed with the view bound to the draw %u times (it differed from the latest upload %u "
            "times), with the latest %u times",
            g_viewBound.exchange(0), g_viewDiffered.exchange(0), g_viewLatest.exchange(0));
        std::lock_guard lock(g_unmovedMutex);
        static const char* kWhy[] = {"no whole bone buffer copied", "someone else's bones", "BF3's buffer not bound"};
        for (const auto& [key, n] : g_unmovedWhy)
            Log("Hands:   unmoved %u/%u: %u (%s)", std::get<0>(key), std::get<1>(key), n, kWhy[std::get<2>(key)]);
        g_unmovedWhy.clear();
    }
    std::lock_guard projLock(g_projMutex);
    for (const SeenProjection& s : g_seenProjections) {
        const float* m = s.m;
        Log("Weapon watch: matrix with the game's FOV, %u-byte upload at +%u (%s): (%.3f %.3f %.3f %.3f) (%.3f %.3f "
            "%.3f %.3f) (%.3f %.3f %.3f %.3f) (%.3f %.3f %.3f %.3f)",
            s.size, s.offset, s.columns ? "scale down the columns" : "scale along the rows", m[0], m[1], m[2], m[3],
            m[4], m[5], m[6], m[7], m[8], m[9], m[10], m[11], m[12], m[13], m[14], m[15]);
    }
    g_seenProjections.clear();
}

std::wstring BesideDll(const wchar_t* name) {
    wchar_t path[MAX_PATH];
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&BesideDll), &self);
    GetModuleFileNameW(self, path, MAX_PATH);
    if (wchar_t* slash = wcsrchr(path, L'\\'))
        slash[1] = 0;
    return std::wstring(path) + name;
}

// Copies a buffer the graphics card holds back to memory and writes it to a file. On the thread
// that presents, which owns the immediate context.
bool SaveBuffer(ID3D11Device* device, ID3D11DeviceContext* imm, ID3D11Buffer* buffer, const wchar_t* name,
                UINT& size) {
    D3D11_BUFFER_DESC desc;
    buffer->GetDesc(&desc);
    size = desc.ByteWidth;
    D3D11_BUFFER_DESC sd{desc.ByteWidth, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ, 0, 0};
    ID3D11Buffer* staging = nullptr;
    if (FAILED(device->CreateBuffer(&sd, nullptr, &staging)))
        return false;
    imm->CopyResource(staging, buffer);
    D3D11_MAPPED_SUBRESOURCE mapped;
    bool ok = false;
    if (SUCCEEDED(g_origMap[0](imm, staging, 0, D3D11_MAP_READ, 0, &mapped))) {
        FILE* f = nullptr;
        if (_wfopen_s(&f, BesideDll(name).c_str(), L"wb") == 0 && f) {
            ok = fwrite(mapped.pData, 1, desc.ByteWidth, f) == desc.ByteWidth;
            fclose(f);
        }
        g_origUnmap[0](imm, staging, 0);
    }
    staging->Release();
    return ok;
}

void WriteDump() {
    MeshDump* d = nullptr;
    {
        std::lock_guard lock(g_dumpMutex);
        std::swap(d, g_dump);
    }
    if (!d)
        return;
    ID3D11Device* device = nullptr;
    if (d->ib)
        d->ib->GetDevice(&device);
    ID3D11DeviceContext* imm = nullptr;
    if (device)
        device->GetImmediateContext(&imm);
    FILE* f = nullptr;
    wchar_t prefix[48];  // mesh_<index count>_<start>_
    swprintf_s(prefix, L"mesh_%u_%u_", d->key.indexCount, d->key.startIndex);
    auto name = [&](const wchar_t* what) { return std::wstring(prefix) + what; };
    if (imm && _wfopen_s(&f, BesideDll(name(L"dump.txt").c_str()).c_str(), L"w") == 0 && f) {
        fprintf(f, "draw %u %u %d topology %d\n", d->key.indexCount, d->key.startIndex, d->key.baseVertex,
                static_cast<int>(d->topology));
        UINT size = 0;
        bool ok = SaveBuffer(device, imm, d->ib, name(L"ib.bin").c_str(), size);
        fprintf(f, "ib format %d offset %u size %u %s\n", static_cast<int>(d->ibFormat), d->ibOffset, size,
                ok ? "saved" : "FAILED");
        for (int i = 0; i < 4; ++i) {
            if (!d->vb[i])
                continue;
            wchar_t vb[16];
            swprintf_s(vb, L"vb%d.bin", i);
            ok = SaveBuffer(device, imm, d->vb[i], name(vb).c_str(), size);
            fprintf(f, "vb %d stride %u offset %u size %u %s\n", i, d->stride[i], d->offset[i], size,
                    ok ? "saved" : "FAILED");
        }
        fclose(f);
        if (_wfopen_s(&f, BesideDll(name(L"bones.bin").c_str()).c_str(), L"wb") == 0 && f) {
            fwrite(d->bones.data(), 1, d->bones.size(), f);
            fclose(f);
        }
        Log("Weapon: draw %u/%u's mesh is written to mesh_%u_%u_*.bin", d->key.indexCount, d->key.startIndex,
            d->key.indexCount, d->key.startIndex);
    }
    if (imm)
        imm->Release();
    if (device)
        device->Release();
    for (ID3D11Buffer* b : d->vb)
        if (b)
            b->Release();
    if (d->ib)
        d->ib->Release();
    delete d;
}

}

namespace skin {

float BareTwist(int side) {
    return g_bareTwist[side & 1].load(std::memory_order_relaxed);
}

void BareShares(float* share) {
    for (int i = 0; i < 3; ++i)
        share[i] = g_bareShare[i].load(std::memory_order_relaxed);
}

bool PistolTest() {
    return g_pistolTest.load(std::memory_order_relaxed);
}

bool PistolMagazineSeat(float* seat) {
    std::lock_guard lock(g_handsMutex);
    std::memcpy(seat, PistolSeat(), sizeof(g_pistolSeat));
    return g_havePistolSeat;
}

bool DropPistolMagazine(PistolDrop from, const float* velocityView) {
    float toClear = 0.0f;
    if (!PistolHeld() || !StartPistolDrop(from, velocityView, toClear))
        return false;
    if (from == PistolDrop::kHand)
        Log("Pistol reload: the magazine let go of: it falls from the hand");
    else if (toClear > 0.0f)
        Log("Pistol reload: the magazine drops out of the grip along its line (%.1f cm to go%s), then to the floor",
            toClear * 100.0f, from == PistolDrop::kWell ? ", from part way in where it's drawn" : "");
    else
        Log("Pistol reload: the magazine drops from where it's drawn, already out of the grip");
    return true;
}

bool BarePistolHand(float* hold) {
    if (!g_haveBarePistolHand.load(std::memory_order_acquire) || GetTickCount64() - g_barePistolAt.load() > 300)
        return false;
    std::lock_guard lock(g_handsMutex);
    std::memcpy(hold, g_barePistolHand, sizeof(g_barePistolHand));
    return true;
}

bool TrainPistolInHand() {
    return g_haveBarePistolHand.load(std::memory_order_acquire) && GetTickCount64() - g_barePistolAt.load() <= 300;
}

bool BarePistolLeft(float* hold, float (*fingers)[12]) {
    if (!g_haveBarePistolLeft.load(std::memory_order_acquire) || GetTickCount64() - g_barePistolAt.load() > 300)
        return false;
    std::lock_guard lock(g_handsMutex);
    std::memcpy(hold, g_barePistolLeft, sizeof(g_barePistolLeft));
    std::memcpy(fingers, g_barePistolLeftFingers, sizeof(g_barePistolLeftFingers));
    return true;
}

bool BareWrist() {
    return g_bareWrist.load(std::memory_order_relaxed);
}

bool BareArmsLately() {
    return GetTickCount64() - g_bareSeenAt.load(std::memory_order_relaxed) < 250;
}

bool BareHold() {
    return g_bareHold.load(std::memory_order_relaxed);
}

bool BareOnRig() {
    return g_bareHold.load(std::memory_order_relaxed) && BareArmsLately() && RigDrawnLately() && !PistolHeld();
}

float BareElbowReach() {
    return g_bareElbowReach.load(std::memory_order_relaxed);
}

int BareCap() {
    return g_bareCap.load(std::memory_order_relaxed);
}

bool BareSleeves() {
    return g_bareSleeves.load(std::memory_order_relaxed);
}

void SetBareHandsAllowed(bool allowed) {
    if (g_bareAllowed.exchange(allowed) != allowed)
        Log("Bare hands: %s", allowed ? "allowed (no cutscene or scripted turn for a while, the aim found)"
                                       : "off (a cutscene, a scripted turn, or the aim not found)");
    static ULONGLONG s_last = 0;
    const ULONGLONG now = GetTickCount64();
    if (now - s_last >= 10000) {
        s_last = now;
        if (const uint32_t n = g_bareDraws.exchange(0))
            // Placed with the view bound to the draw (its own eye), or the latest one uploaded (maybe
            // the other eye's: a jump between the eyes, the user swaying, 14:55).
            Log("Bare hands: %u draws moved to the controllers in the last 10 s; placed with the draw's own view "
                "%u times, the latest %u times",
                n, g_viewBound.exchange(0), g_viewLatest.exchange(0));
    }
}

void LogShaderDraw(ID3D11DeviceContext* ctx, unsigned count, unsigned start, int base, unsigned instances) {
    if (g_psLogFrames.load(std::memory_order_relaxed) > 0)
        LogByShader(ctx, count, start, base, instances);
}

namespace {

// The ID3D11DeviceContext table's places of the three (d3d11_hook.cpp names the same numbers).
constexpr int kTableDrawIndexed = 12, kTableMap = 14, kTableUnmap = 15;

// The second way, in the game's own contexts' tables, waiting for the first picture (its swap chain gives the game's device).
std::atomic<bool> g_tablePending{false};

// hooks.txt beside the DLL, a line "skin table": the second way from the start (a test of it on a card where the first way takes).
bool TableWanted() {
    HMODULE self = nullptr;
    wchar_t path[MAX_PATH];
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(&TableWanted), &self) ||
        !GetModuleFileNameW(self, path, MAX_PATH))
        return false;
    std::wstring file(path);
    file = file.substr(0, file.find_last_of(L"\\/") + 1) + L"hooks.txt";
    FILE* f = _wfsopen(file.c_str(), L"r", _SH_DENYNO);
    if (!f)
        return false;
    bool wanted = false;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        std::string s(line);
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ' || s.back() == '\t'))
            s.pop_back();
        for (char& c : s)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (s == "skin table")
            wanted = true;
    }
    fclose(f);
    return wanted;
}

// A function's first bytes, for the log when MinHook can't take it (what's there: an overlay's jump, a short body). No C++
// object here: __try needs none to unwind.
void ReadFirstBytes(const void* at, char* text, size_t size) {
    int used = 0;
    text[0] = 0;
    __try {
        for (int i = 0; i < 12; ++i)
            used += sprintf_s(text + used, size - used, "%02X ", static_cast<const unsigned char*>(at)[i]);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        sprintf_s(text + used, size - used, "(unreadable)");
    }
}

std::string FirstBytes(const void* at) {
    char text[64];
    ReadFirstBytes(at, text, sizeof(text));
    return text;
}

// The pointer-sized words of a module's read-only sections that start a context table holding these Map and DrawIndexed (two
// of the three, against chance), up to `most`. No C++ object here: __try needs none to unwind.
int FindTables(HMODULE mod, void* const (&map)[2], void* const (&drawIndexed)[2], void*** out, int most) {
    int found = 0;
    __try {
        const auto base = reinterpret_cast<const unsigned char*>(mod);
        const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        const IMAGE_SECTION_HEADER* section = IMAGE_FIRST_SECTION(nt);
        for (unsigned s = 0; s < nt->FileHeader.NumberOfSections && found < most; ++s, ++section) {
            const DWORD c = section->Characteristics;
            if (!(c & IMAGE_SCN_MEM_READ) || (c & IMAGE_SCN_MEM_EXECUTE) || (c & IMAGE_SCN_MEM_WRITE))
                continue;  // tables of functions live in the read-only data
            void** p = reinterpret_cast<void**>(const_cast<unsigned char*>(base) + section->VirtualAddress);
            const size_t words = section->Misc.VirtualSize / sizeof(void*);
            for (size_t i = 0; i + kTableUnmap < words && found < most; ++i) {
                void* const m = p[i + kTableMap];
                void* const d = p[i + kTableDrawIndexed];
                if ((m == map[0] || m == map[1]) && (d == drawIndexed[0] || d == drawIndexed[1]))
                    out[found++] = p + i;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    return found;
}

// One entry of a context's table swapped for a hook, the old entry kept as the function to call on.
bool PutInTable(void** table, int slot, void* hook, void** orig) {
    DWORD protect = 0;
    if (!table || !VirtualProtect(&table[slot], sizeof(void*), PAGE_EXECUTE_READWRITE, &protect))
        return false;
    *orig = table[slot];
    InterlockedExchangePointer(&table[slot], hook);
    VirtualProtect(&table[slot], sizeof(void*), protect, &protect);
    return true;
}

}  // namespace

// Map, Unmap and DrawIndexed, for the immediate context ([0]) and the deferred ones ([1]): the first way is MinHook at each
// function, each on its own (the deferred contexts may share any of the three with the immediate one: one hook then covers
// both). If any can't be taken, none of the first way is kept (a half-hooked set is worse than none: a player's AMD card,
// 2026-11-14) and the second way takes all three at the first picture: the entries in the game's own contexts' tables.
bool InstallHooks(void* const (&map)[2], void* const (&unmap)[2], void* const (&drawIndexed)[2]) {
    if (TableWanted()) {
        Log("Skin: hooks.txt says \"skin table\": Map, Unmap and DrawIndexed go in the game's contexts' tables at the first "
            "picture (the second way, tested)");
        g_secondWay = true;
        g_tablePending = true;
        return true;
    }
    struct One {
        const char* name;
        void* const* target;
        void* hook[2];
        void** orig[2];
    } const ones[3] = {
        {"Map", map, {reinterpret_cast<void*>(&Map0), reinterpret_cast<void*>(&Map1)},
         {reinterpret_cast<void**>(&g_origMap[0]), reinterpret_cast<void**>(&g_origMap[1])}},
        {"Unmap", unmap, {reinterpret_cast<void*>(&Unmap0), reinterpret_cast<void*>(&Unmap1)},
         {reinterpret_cast<void**>(&g_origUnmap[0]), reinterpret_cast<void**>(&g_origUnmap[1])}},
        {"DrawIndexed", drawIndexed, {reinterpret_cast<void*>(&DrawIndexed0), reinterpret_cast<void*>(&DrawIndexed1)},
         {reinterpret_cast<void**>(&g_origDrawIndexed[0]), reinterpret_cast<void**>(&g_origDrawIndexed[1])}},
    };
    void* made[6] = {};
    int madeCount = 0;
    std::string shared;
    for (const One& one : ones) {
        for (int i = 0; i < 2; ++i) {
            if (i == 1 && one.target[1] == one.target[0]) {
                *one.orig[1] = *one.orig[0];
                shared += shared.empty() ? one.name : std::string(", ") + one.name;
                break;
            }
            const MH_STATUS s = MH_CreateHook(one.target[i], one.hook[i], one.orig[i]);
            if (s == MH_OK) {
                made[madeCount++] = one.target[i];
                continue;
            }
            Log("Skin: %s of the %s can't be hooked at its function (%s); its first bytes: %s", one.name,
                i == 0 ? "main (immediate) context" : "other (deferred) contexts", MH_StatusToString(s), FirstBytes(one.target[i]).c_str());
            for (int m = 0; m < madeCount; ++m)
                MH_RemoveHook(made[m]);
            Log("Skin: none of the three kept at their functions; all three go in the game's contexts' tables at the first picture "
                "(the second way)");
            g_secondWay = true;
            g_tablePending = true;
            return true;
        }
    }
    Log("Skin: Map, Unmap and DrawIndexed hooked at their functions%s%s", shared.empty() ? "" : " (the other contexts share: ",
        shared.empty() ? "" : (shared + ")").c_str());
    return true;
}

// The second way, once, at the first picture: the three entries swapped in the game's immediate context's table and in its
// deferred contexts' table (a deferred context made for the purpose, its table read and the context let go). All or none.
void InstallTableHooks(IDXGISwapChain* swap) {
    if (!g_tablePending.load(std::memory_order_relaxed) || !g_tablePending.exchange(false))
        return;
    ID3D11Device* device = nullptr;
    if (!swap || FAILED(swap->GetDevice(IID_PPV_ARGS(&device))) || !device) {
        Log("Skin: the game's device wasn't found for the second way: Map, Unmap and DrawIndexed NOT hooked");
        launchreport::Warn("the mod can't follow the game's drawing on this graphics card (no device for its second way): the "
                           "hands, guns and per-eye views won't work. Please send this log");
        return;
    }
    ID3D11DeviceContext* immediate = nullptr;
    device->GetImmediateContext(&immediate);
    ID3D11DeviceContext* deferred = nullptr;
    device->CreateDeferredContext(0, &deferred);
    void** tables[2] = {immediate ? *reinterpret_cast<void***>(immediate) : nullptr,
                        deferred ? *reinterpret_cast<void***>(deferred) : nullptr};
    if (!tables[1])
        tables[1] = tables[0];
    struct Slot {
        int slot;
        void* hook[2];
        void** orig[2];
    } const slots[3] = {
        {kTableMap, {reinterpret_cast<void*>(&Map0), reinterpret_cast<void*>(&Map1)},
         {reinterpret_cast<void**>(&g_origMap[0]), reinterpret_cast<void**>(&g_origMap[1])}},
        {kTableUnmap, {reinterpret_cast<void*>(&Unmap0), reinterpret_cast<void*>(&Unmap1)},
         {reinterpret_cast<void**>(&g_origUnmap[0]), reinterpret_cast<void**>(&g_origUnmap[1])}},
        {kTableDrawIndexed, {reinterpret_cast<void*>(&DrawIndexed0), reinterpret_cast<void*>(&DrawIndexed1)},
         {reinterpret_cast<void**>(&g_origDrawIndexed[0]), reinterpret_cast<void**>(&g_origDrawIndexed[1])}},
    };
    // Undone in reverse if one can't be put in: never a half-hooked set.
    struct Done {
        void** table;
        int slot;
        void* old;
    } done[6] = {};
    int doneCount = 0;
    bool ok = tables[0] != nullptr;
    for (const Slot& s : slots) {
        for (int i = 0; ok && i < 2; ++i) {
            if (i == 1 && tables[1] == tables[0]) {
                *s.orig[1] = *s.orig[0];
                break;
            }
            void* old = nullptr;
            if (!PutInTable(tables[i], s.slot, s.hook[i], &old)) {
                ok = false;
                break;
            }
            *s.orig[i] = old;
            done[doneCount++] = {tables[i], s.slot, old};
        }
    }
    if (!ok) {
        for (int d = doneCount - 1; d >= 0; --d) {
            void* ours = nullptr;
            PutInTable(done[d].table, done[d].slot, done[d].old, &ours);
        }
    }
    // Every other table holding the same functions: another kind of context the game draws through (the first-person arms are
    // drawn through one: 2026-11-15's test on an NVIDIA card, the two tables alone placed no hands). Found in the module that
    // holds the main table; each of the three entries swapped where it holds one of the original functions.
    int more = 0;
    wchar_t moduleName[MAX_PATH] = L"?";
    HMODULE mod = nullptr;
    if (ok && GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                 reinterpret_cast<LPCWSTR>(tables[0]), &mod)) {
        GetModuleFileNameW(mod, moduleName, MAX_PATH);
        void* const originalMap[2] = {reinterpret_cast<void*>(g_origMap[0]), reinterpret_cast<void*>(g_origMap[1])};
        void* const originalDraw[2] = {reinterpret_cast<void*>(g_origDrawIndexed[0]), reinterpret_cast<void*>(g_origDrawIndexed[1])};
        void* const originalUnmap[2] = {reinterpret_cast<void*>(g_origUnmap[0]), reinterpret_cast<void*>(g_origUnmap[1])};
        void** found[32] = {};
        const int count = FindTables(mod, originalMap, originalDraw, found, 32);
        for (int f = 0; f < count; ++f) {
            void** t = found[f];
            if (t == tables[0] || t == tables[1])
                continue;
            bool swapped = false;
            for (const Slot& s : slots) {
                void* const* originals = s.slot == kTableMap ? originalMap : s.slot == kTableUnmap ? originalUnmap : originalDraw;
                for (int i = 0; i < 2; ++i)
                    if (t[s.slot] == originals[i]) {
                        void* old = nullptr;
                        if (PutInTable(t, s.slot, s.hook[i], &old))
                            swapped = true;
                        break;
                    }
            }
            more += swapped;
        }
    }
    const wchar_t* moduleLeaf = wcsrchr(moduleName, L'\\');
    Log("Skin: Map, Unmap and DrawIndexed %s in the game's contexts' tables (the second way; the main context's table at %p, the "
        "other contexts' %s; %d more tables of the same functions in %ls)",
        ok ? "hooked" : "NOT hooked", static_cast<void*>(tables[0]), tables[1] == tables[0] ? "the same" : "their own", more,
        moduleLeaf ? moduleLeaf + 1 : moduleName);
    if (ok)
        launchreport::Line("this graphics card needed the mod's second way of following the game's drawing (Map, Unmap and "
                           "DrawIndexed in the contexts' tables): it took");
    else
        launchreport::Warn("the mod can't follow the game's drawing on this graphics card (Map, Unmap and DrawIndexed): the hands, "
                           "guns and per-eye views won't work. Please send this log");
    if (deferred)
        deferred->Release();
    if (immediate)
        immediate->Release();
    device->Release();
}

void HuntStart() {
    // hunt.txt next to the DLL saying "first person": hunt among the first-person draws instead.
    {
        wchar_t path[MAX_PATH];
        HMODULE self = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(&HuntStart), &self);
        GetModuleFileNameW(self, path, MAX_PATH);
        if (wchar_t* slash = wcsrchr(path, L'\\'))
            wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path), L"hunt.txt");
        char text[64] = {};
        FILE* f = nullptr;
        if (_wfopen_s(&f, path, L"r") == 0 && f) {
            fread(text, 1, sizeof(text) - 1, f);
            fclose(f);
        }
        g_huntFirstPerson = std::strstr(text, "first person") != nullptr;
        g_huntOnly = -1;
        if (const char* s = std::strstr(text, "only"))
            sscanf_s(s + 4, "%d", &g_huntOnly);
    }
    // The hunt picks from the skinned draws counted while research is on.
    if (!g_research.exchange(true)) {
        Log("Draw hunt: research turned on to count skinned draws; press F15 again in a few seconds");
        return;
    }
    std::lock_guard lock(g_drawMutex);
    // Pressed during a hunt: stop it and show everything again, for a clean reference picture.
    if (g_hidden.load()) {
        g_hidden = nullptr;
        g_huntRange.clear();
        g_haveFinal = false;
        Log("Draw hunt: stopped; nothing hidden");
        return;
    }
    const auto& pool = g_huntFirstPerson.load() ? g_firstPerson : g_skinned;
    std::vector<std::pair<DrawKey, UINT>> seen(pool.begin(), pool.end());
    std::sort(seen.begin(), seen.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
    g_huntRange.clear();
    for (const auto& [key, count] : seen)
        g_huntRange.push_back(key);
    g_haveFinal = false;
    g_finalSeen = false;
    if (g_huntRange.empty()) {
        g_hidden = nullptr;
        Log("Draw hunt: no skinned draws seen yet");
        return;
    }
    // hunt.txt "first person only N": hide just the N-th (from 0, most drawn first), to map each.
    if (g_huntFirstPerson.load() && g_huntOnly >= 0) {
        for (size_t i = 0; i < g_huntRange.size(); ++i)
            Log("Draw hunt: first-person draw %zu: index count %u, start %u, base %d", i, g_huntRange[i].indexCount,
                g_huntRange[i].startIndex, g_huntRange[i].baseVertex);
        size_t n = std::min(static_cast<size_t>(g_huntOnly), g_huntRange.size() - 1);
        PublishHidden(g_huntRange.begin() + n, g_huntRange.begin() + n + 1);
        Log("Draw hunt: hiding only first-person draw %zu", n);
        return;
    }
    PublishHidden(g_huntRange.begin(), g_huntRange.begin() + (g_huntRange.size() + 1) / 2);
    Log("Draw hunt: %zu %s draw kinds; hiding the first %zu", g_huntRange.size(),
        g_huntFirstPerson.load() ? "first-person" : "skinned", (g_huntRange.size() + 1) / 2);
}

void HuntAnswer(bool vanished) {
    std::lock_guard lock(g_drawMutex);
    if (g_huntRange.size() <= 1) {
        Log("Draw hunt: nothing to narrow (start it with F15)");
        return;
    }
    size_t half = (g_huntRange.size() + 1) / 2;
    if (vanished)
        g_huntRange.resize(half);
    else
        g_huntRange.erase(g_huntRange.begin(), g_huntRange.begin() + half);
    if (g_huntRange.size() == 1) {
        g_final = g_huntRange[0];
        g_haveFinal = true;
        g_finalSeen = false;
        PublishHidden(g_huntRange.begin(), g_huntRange.end());  // keep it hidden, to confirm
        Log("Draw hunt: found it: index count %u, start index %u, base vertex %d (hidden; bone data follows)",
            g_final.indexCount, g_final.startIndex, g_final.baseVertex);
        return;
    }
    PublishHidden(g_huntRange.begin(), g_huntRange.begin() + (g_huntRange.size() + 1) / 2);
    Log("Draw hunt: arms %s; %zu candidates left, hiding %zu", vanished ? "vanished" : "still there",
        g_huntRange.size(), (g_huntRange.size() + 1) / 2);
}

void Probe() {
    shapemake::OnFrame(reinterpret_cast<void*>(g_origMap[0]), reinterpret_cast<void*>(g_origUnmap[0]));  // a gun's mesh being read back for its shape: nothing, but then
    static ULONGLONG lastLog = 0, lastRead = 0, lastFullScan = 0;
    ULONGLONG now = GetTickCount64();
    if (now - lastRead >= 300) {
        lastRead = now;
        ReadSelection();
        ReadWeaponSelection();
    }
    // Every upload size is scanned for one frame (this call to the next) every two seconds.
    bool scanAll = now - lastFullScan >= 2000;
    if (scanAll)
        lastFullScan = now;
    g_scanAll.store(scanAll, std::memory_order_relaxed);
    if (now - lastLog < 1000)
        return;
    lastLog = now;
    if (uint32_t left = g_drawsByEye[1].exchange(0), right = g_drawsByEye[2].exchange(0),
        unknown = g_drawsByEye[0].exchange(0);
        views::Active())
        Log("Hands: draws placed for the left eye %u, the right eye %u, eye not known %u (first-person views a "
            "frame: %u)",
            left, right, unknown, g_armsUploadsBefore);
    if (g_wideScale.load() > 0.0f) {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        Log("First-person FOV: matrices rescaled from the game's FOV in the last second: %u in small buffers, %u in "
            "whole ring buffers, %u appended to ring buffers; scanning took %.1f ms",
            g_rescaled[kSmall].exchange(0), g_rescaled[kRingWhole].exchange(0), g_rescaled[kRingAppend].exchange(0),
            1000.0 * static_cast<double>(g_rescaleTicks.exchange(0)) / static_cast<double>(frequency.QuadPart));
        // A size that held a match once was scanned for good: sizes that matched only during a
        // cutscene or a menu went on costing over a millisecond a frame. Those idle 10 s are dropped;
        // the every-two-seconds full scan finds them again if they come back.
        static ULONGLONG lastMatch[kSizeBuckets] = {};
        char sizes[512] = {};
        int used = 0;
        for (int b = 0; b < kSizeBuckets; ++b) {
            uint32_t scanned = g_scannedBySize[b].exchange(0), matched = g_matchedBySize[b].exchange(0);
            if (matched)
                lastMatch[b] = now;
            else if (g_sizeMatched[b].load() && now - lastMatch[b] > 10000)
                g_sizeMatched[b] = false;
            if ((scanned >= 100 || matched) && used < 480)
                used += snprintf(sizes + used, sizeof(sizes) - used, " %u-%uB: %u scanned %u matched;", b * 256,
                                 b * 256 + 255, scanned, matched);
        }
        Log("First-person FOV: uploads by size:%s", sizes);
    }
    LogBodyLock();
    if (g_weaponShift.load())
        Log("Weapon: last second's gun draws: %u shifted, %u left alone (another soldier's)",
            g_weaponShifted.exchange(0), g_weaponFar.exchange(0));
    // The weapon camera test: where the camera is (as moved), where the game's own gun body was
    // drawn and the first-person eye, both in the world (the arms' space plus its origin).
    if (g_weaponTesting.load()) {
        float cam[3], gun[3] = {}, eye[3] = {};
        {
            std::lock_guard lock(g_mutex);
            std::memcpy(cam, g_camera, sizeof(cam));
        }
        {
            std::lock_guard lock(g_handsMutex);
            if (g_haveGunBodyOrig)
                gun[0] = g_gunBodyOrig[3], gun[1] = g_gunBodyOrig[7], gun[2] = g_gunBodyOrig[11];
        }
        {
            std::lock_guard lock(g_fpMutex);
            eye[0] = g_viewToArms[3], eye[1] = g_viewToArms[7], eye[2] = g_viewToArms[11];
            for (int i = 0; i < 3; ++i)
                gun[i] += g_fpOrigin[i], eye[i] += g_fpOrigin[i];
        }
        Log("Weapon test: camera at (%.3f %.3f %.3f), the game's gun body at (%.3f %.3f %.3f), the first-person "
            "eye at (%.3f %.3f %.3f)",
            cam[0], cam[1], cam[2], gun[0], gun[1], gun[2], eye[0], eye[1], eye[2]);
    }
    if (g_weaponWatch.load())
        LogWatch();
    WriteDump();
    if (g_handShift.load())
        Log("Hands: last second's gloves draws: %u shifted, %u without a fresh copy, %u with the buffer not bound, "
            "%u with no bone upload just before",
            g_shifted.exchange(0), g_noCopy.exchange(0), g_noSlot.exchange(0), g_notSkinned.exchange(0));
    BoneInfo b;
    {
        std::lock_guard lock(g_drawMutex);
        if (!g_haveFinal || !g_finalSeen)
            return;
        b = g_finalBones;
    }
    float rel[3] = {b.mean[0] - g_camera[0], b.mean[1] - g_camera[1], b.mean[2] - g_camera[2]};
    Log("Draw hunt: arms' bones: %u matrices (%s) in %u-byte %s upload; mean translation (%.2f, %.2f, %.2f), "
        "%.2f m from camera (right %.2f, up %.2f, forward %.2f), %.2f m from origin",
        b.count, b.layout == 0 ? "3x4" : "4x4", b.size, b.fresh ? "fresh" : "appended", b.mean[0], b.mean[1],
        b.mean[2], b.dCamera, Dot3(rel, g_axes[0]), Dot3(rel, g_axes[1]), Dot3(rel, g_axes[2]), b.dOrigin);
    Log("Draw hunt:   first matrix rows (%.3f %.3f %.3f %.3f) (%.3f %.3f %.3f %.3f) (%.3f %.3f %.3f %.3f)",
        b.first[0], b.first[1], b.first[2], b.first[3], b.first[4], b.first[5], b.first[6], b.first[7],
        b.first[8], b.first[9], b.first[10], b.first[11]);
}

void SetProjectionFix(float gameFov, float wideFov) {
    bool on = gameFov > 0.0f && wideFov > 0.0f;
    g_gameScale = on ? 1.0f / std::tan(gameFov / 2) : 0.0f;
    g_wideScale = on ? 1.0f / std::tan(wideFov / 2) : 0.0f;
}

void ToggleHandShift() {
    bool on = !g_handShift.load();
    g_handShift = on;
    Log("Hands: shift %s", on ? "on: gloves drawn 20 cm higher" : "off");
    BeepAsync(on ? 1200 : 440, 200);
}

void SetCamera(const float* position, const float* right, const float* up, const float* forward) {
    std::lock_guard lock(g_mutex);
    for (int i = 0; i < 3; ++i) {
        g_camera[i] = position[i];
        g_axes[0][i] = right[i];
        g_axes[1][i] = up[i];
        g_axes[2][i] = forward[i];
    }
}

void ToggleResearch() {
    bool on = !g_research.load();
    g_research = on;
    Log("Skin: research hooks %s", on ? "on (bone searches, ring-buffer comparisons, draw counting)" : "off");
}

bool EjectMagazine() {
    if (g_magOut.load())
        return false;
    const bool held = !RifleInHand();
    if (StartLooseMagazine(false, nullptr, kSpin, held)) {
        g_magOut = true;
        Log("Magazine: ejected, out of its well and falling (loose)");
        return true;
    }
    if (!StartDrop())
        return false;
    g_magOut = true;
    Log("Magazine: ejected, falling to the floor at %.2f m", g_floor.load());
    return true;
}

void InsertMagazine() {
    if (g_magOut.exchange(false))
        Log("Magazine: a new one is in the gun");
}

bool MagazineOut() {
    return g_magOut.load();
}

void SetMagazineOut(bool out) {
    if (g_magOut.exchange(out) != out)
        Log("Magazine: %s, as the gun now in hand was left", out ? "out" : "in");
}

bool DropHeldMagazine(const float* velocityView) {
    if (StartLooseMagazine(true, velocityView, kSpin, !RifleInHand())) {
        Log("Magazine: let go of, falling from the hand (loose)");
        return true;
    }
    if (!StartDrop(true, velocityView))
        return false;
    Log("Magazine: let go of, falling from the hand");
    return true;
}

// By either hand: the left's (or any side but the right's) is the one above, as it always was; the right's goes by its own hand.
bool DropHeldMagazine(const float* velocityView, int side, const loose::HandAt* hand) {
    if (side != 1)
        return DropHeldMagazine(velocityView);
    if (StartLooseMagazine(true, velocityView, kSpin, !RifleInHand(), 1, hand)) {
        Log("Magazine: let go of, falling from the right hand (loose)");
        return true;
    }
    if (!StartDrop(true, velocityView))
        return false;
    Log("Magazine: let go of, falling from the right hand");
    return true;
}

bool MagazineSeatKnown() {
    std::lock_guard lock(g_handsMutex);
    return std::all_of(std::begin(g_haveMagSeat), std::end(g_haveMagSeat), [](bool b) { return b; });
}

bool RifleInHand() {
    return DrawnLately(g_rifleDrawn, g_rifleFrame);
}

bool RightHandOnGun(float* rel, bool& launcher) {
    constexpr size_t kRightHandFrame = 32;  // the gloves' bone: The Great Destroyer's arms' 15, the left's 1 mirrored
    const DrawKey& gloves = kArmDraws[0].key;
    std::lock_guard lock(g_handsMutex);
    // As this frame's draws of the arms go.
    launcher = g_rigThisFrame.load(std::memory_order_relaxed);
    if (launcher) {
        const RigSet* const sets[] = {&RigInUse(), &g_rigNow};
        for (const RigSet* set : sets)
            if (const auto it = set->bones.find(gloves); it != set->bones.end() && it->second.have[kRightHandFrame]) {
                std::memcpy(rel, it->second.rel[kRightHandFrame], sizeof(it->second.rel[kRightHandFrame]));
                return true;
            }
        return false;
    }
    const auto it = g_learned.find(gloves);
    if (it == g_learned.end() || !it->second.have[kRightHandFrame])
        return false;
    std::memcpy(rel, it->second.rel[kRightHandFrame], sizeof(it->second.rel[kRightHandFrame]));
    return true;
}

bool LeftHandOnRig(bool gameHands, float* rel) {
    std::lock_guard lock(g_handsMutex);
    // As this frame's draws of the arms go (DrawLauncherRig's own), by the rig they'll have in use (RigInUse, with the gameHands hands.cpp is about to hand over).
    if (!g_launcherOn.load(std::memory_order_relaxed) || !g_rigThisFrame.load(std::memory_order_relaxed))
        return false;
    const float* hand = RigLeftHand(g_rigFrozen && !gameHands ? g_rigRest : g_rigNow);
    if (!hand || !std::all_of(hand, hand + 12, [](float f) { return std::isfinite(f); }))
        return false;
    std::memcpy(rel, hand, 12 * sizeof(float));
    return true;
}

void Update() {
    UpdateDrop();
    UpdatePistolDrop();
    ++g_frameSerial;
    RestoreAt4Shot();  // (the AT4 half a second out of the hands: its rocket's start back as its data has it)
    RestoreLauncherShot();  // (the SMAW's alike)
    scopezoom::OnFrame();
    scopeview::OnFrame();
    // The next frame's draws all learn the game's hands by this frame's verdict (see g_holdNormal).
    g_learnHold.store(g_holdNormal.load(std::memory_order_relaxed) && !input::Sprinting(), std::memory_order_relaxed);
    {
        std::lock_guard lock(g_handsMutex);
        TakeFrameOfHands();
        TakeFrameOfRig();
    }
    g_rigThisFrame.store(LauncherDrawnLast() || HeldDrawnLast(), std::memory_order_relaxed);
    // recoil's one hand on the gun for its scatter lapses with no call for half a second (recoil switched off, or not stepped): both hands' again.
    if (g_spreadOneHand.load(std::memory_order_relaxed))
        if (const ULONGLONG now = GetTickCount64(); now - g_spreadAskedAt.load() > 500)
            StepHoldSpread(false, now, 0);
    // The grenade launcher's tube met with the rifle is forgotten once neither has been drawn for 10 s (another level's M416 may have none). (The clock
    // read after them: a draw stamped meanwhile is never taken for one long ago.)
    if (g_m320Met.load(std::memory_order_relaxed)) {
        const ULONGLONG rifle = g_rifleDrawn.load(), tube = g_m320MetAt.load(std::memory_order_relaxed), now = GetTickCount64();
        if (now - rifle > kM320Forget && now - tube > kM320Forget)
            g_m320Met.store(false, std::memory_order_relaxed);
    }
    if (g_armTrace.load(std::memory_order_relaxed) > 0)
        --g_armTrace;
    // Every 10 s, how the game's hold was judged: a guard that never runs shows here.
    static ULONGLONG lastHoldLog = 0;
    if (const ULONGLONG now = GetTickCount64(); now - lastHoldLog >= 10000) {
        lastHoldLog = now;
        uint32_t here = 0, later = 0, never = 0;
        {
            std::lock_guard lock(g_handsMutex);
            std::swap(here, g_judgedHere), std::swap(later, g_judgedLater), std::swap(never, g_notJudged);
        }
        if (here || later || never)
            Log("Hands: the last 10 s, the game's hold judged %u times in the gloves' draw, %u at the gun's after them; "
                "%u gloves draws had no gun body in their view",
                here, later, never);
        if (const uint32_t heldDraws = g_heldMagDraws.exchange(0))  // (either hand's: the text kept as the reload check's filter knows it)
            Log("Magazine: drawn in the left hand %u times in the last 10 s", heldDraws);
        if (const uint32_t missed = g_heldMagMissed.exchange(0))
            Log("Magazine: in a hand but not drawn in %u frames of the last 10 s (no seat known for it on the gun in hand)", missed);
        if (const uint32_t followed = g_heldMagFollowed.exchange(0))
            Log("Magazine: in a hand but not drawn in %u frames of the last 10 s (the game's own moves on the gun in hand followed)", followed);
    }
    if (int frames = g_fpProbeFrames.load(std::memory_order_relaxed); frames > 0)
        g_fpProbeFrames.store(frames - 1, std::memory_order_relaxed);
    if (int frames = g_drawScan.load(std::memory_order_relaxed); frames > 0)
        g_drawScan.store(frames - 1, std::memory_order_relaxed);
    if (int frames = g_psLogFrames.load(std::memory_order_relaxed); frames > 0)
        g_psLogFrames.store(frames - 1, std::memory_order_relaxed);
    if (int frames = g_scopeTraceFrames.load(std::memory_order_relaxed); frames > 0)
        g_scopeTraceFrames.store(frames - 1, std::memory_order_relaxed);
}

bool Watching() {
    return g_weaponWatch.load();
}

bool EyePictureWanted() {
    return g_bodyLock.load(std::memory_order_relaxed) && g_bodyStereo.load(std::memory_order_relaxed) &&
           camera::BodyTurnWanted();
}

void NoteEyePicture() {
    std::lock_guard lock(g_bodyMutex);
    BeginBodyFrame(g_bodyLatch);
    ++g_bodyLatch.pictures;
    if (effects::TailRecording()) {
        char note[64];
        std::snprintf(note, sizeof(note), "an eye's picture: %d this frame", g_bodyLatch.pictures);
        effects::TailNote(note);
    }
}

const uint8_t* RecentUploadOf(ID3D11Resource* resource, UINT& size, uint32_t& serial) {
    for (int k = 1; k <= 8; ++k) {  // newest first
        const RecentUpload& r = t_recent[(t_recentNext - k + 8) % 8];
        if (resource && r.resource == resource && r.size) {
            size = r.size;
            serial = r.serial;
            return r.bytes;
        }
    }
    return nullptr;
}

const uint8_t* LatestBlockOf(ID3D11Resource* resource, UINT& size, uint32_t& serial) {
    if (!resource || t_upload.resource != resource || t_upload.bytes.empty())
        return nullptr;
    size = static_cast<UINT>(t_upload.bytes.size());
    serial = t_upload.serial;
    return t_upload.bytes.data();
}

uint32_t UploadSerial() {
    return t_uploadSerial;
}

// What the effects take: whichever gun was drawn moved last. Under g_handsMutex.
const EffectMove* LiveEffect() {
    if (GetTickCount64() - g_launcherDeltaTime <= 100 && g_launcherDeltaTime > g_gunDeltaTime)
        return &g_launcherEffect;
    if (g_pistolDeltaTime > g_gunDeltaTime && GetTickCount64() - g_pistolDeltaTime <= 100)
        return &g_pistolEffect;
    return g_haveGunDelta && GetTickCount64() - g_gunDeltaTime <= 100 ? &g_gunEffect : nullptr;
}
// The arms' space's origin that goes with them. Under g_handsMutex and g_fpMutex.
const float* EffectOrigin() {
    const EffectMove* live = LiveEffect();
    return live && live->haveOrigin ? live->origin : g_fpOrigin;
}
const float* EffectGunBody() {
    if (GetTickCount64() - g_launcherDeltaTime <= 100 && g_launcherDeltaTime > g_gunDeltaTime)
        return g_launcherFrameOrig;
    if (g_pistolDeltaTime > g_gunDeltaTime && GetTickCount64() - g_pistolDeltaTime <= 100)
        return g_pistolFrameOrig;
    return g_haveGunBodyOrig ? g_gunBodyOrig : nullptr;
}
bool GameGunPort(float (&world)[3]) {
    std::lock_guard lock(g_handsMutex);
    const float* body = EffectGunBody();
    if (!body)
        return false;
    // The M416's ejection port on its body bone (the dust cover's middle: bone 3; its casings start within 1 cm of it); the
    // pistol's on its frame bone (from where its casings first show: weapon.txt "flashscan", 2026-11-02).
    const bool pistol = body == g_pistolFrameOrig, held = body == g_launcherFrameOrig && g_heldKind.load(std::memory_order_relaxed);
    const float* heldPort = HeldKindNow().port;
    const float local[3] = {held ? heldPort[0] : pistol ? -0.01f : -0.017f, held ? heldPort[1] : pistol ? 0.04f : 0.043f, held ? heldPort[2] : pistol ? 0.13f : 0.41f};
    std::lock_guard fp(g_fpMutex);
    const float* origin = EffectOrigin();
    for (int r = 0; r < 3; ++r)
        world[r] = body[r * 4] * local[0] + body[r * 4 + 1] * local[1] + body[r * 4 + 2] * local[2] + body[r * 4 + 3] + origin[r];
    return true;
}

bool GameGunMuzzle(float (&world)[3]) {
    std::lock_guard lock(g_handsMutex);
    const float* body = EffectGunBody();
    if (!body)
        return false;
    // On the frame bone (the pistol's flash starts at 0, 5.6 cm above it, and its fireball is 28 cm along it) or the rifle's
    // body bone (its barrel's end, the mesh dump).
    const bool pistol = body == g_pistolFrameOrig, launcher = body == g_launcherFrameOrig, held = launcher && g_heldKind.load(std::memory_order_relaxed);
    const float local[3] = {0.0f, held ? HeldKindNow().muzzle[1] : launcher ? 0.0f : pistol ? 0.056f : 0.04f, held ? HeldKindNow().muzzle[2] : launcher ? kLauncherMuzzle : pistol ? 0.2f : 0.78f};
    std::lock_guard fp(g_fpMutex);
    const float* origin = EffectOrigin();
    for (int r = 0; r < 3; ++r)
        world[r] = body[r * 4] * local[0] + body[r * 4 + 1] * local[1] + body[r * 4 + 2] * local[2] + body[r * 4 + 3] + origin[r];
    return true;
}

void NoteWorldEye(const float* eye) {
    std::lock_guard lock(g_fpMutex);
    NoteWorldEyeLocked(eye);
}

void NoteMainCamera(const float* gameEye, const float* leftEye, const float* rightEye) {
    std::lock_guard lock(g_fpMutex);
    MainCamera& c = g_mainCameras[g_mainCameraNext];
    g_mainCameraNext = (g_mainCameraNext + 1) % kMainCameras;
    c.at = GetTickCount64();
    for (int i = 0; i < 3; ++i) {
        c.gameEye[i] = gameEye[i];
        c.toWorld[i * 4] = leftEye[i], c.toWorld[i * 4 + 1] = leftEye[4 + i], c.toWorld[i * 4 + 2] = leftEye[8 + i];
        c.toWorld[i * 4 + 3] = (leftEye[12 + i] + rightEye[12 + i]) * 0.5f;
    }
}

bool GameGunPlaces(float (&world)[3], float (&arms)[3]) {
    std::lock_guard lock(g_handsMutex);
    const float* body = EffectGunBody();
    if (!body)
        return false;
    std::lock_guard fp(g_fpMutex);
    const float* origin = EffectOrigin();
    for (int i = 0; i < 3; ++i) {
        arms[i] = body[i * 4 + 3];
        world[i] = arms[i] + origin[i];
    }
    return true;
}

bool GunDeltaWorld(float (&out)[16]) {
    std::lock_guard lock(g_handsMutex);
    const EffectMove* live = LiveEffect();
    if (!live || !live->haveOrigin)  // a turn in the world would swing round the world's origin: kilometres away
        return false;
    const float* d = live->delta;
    const float* o = live->origin;
    // The arms' space is the world less its origin: the same turn, the translation moved with it.
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c)
            out[r * 4 + c] = d[r * 4 + c];
        out[r * 4 + 3] = d[r * 4 + 3] + o[r] - (d[r * 4] * o[0] + d[r * 4 + 1] * o[1] + d[r * 4 + 2] * o[2]);
    }
    out[12] = out[13] = out[14] = 0.0f;
    out[15] = 1.0f;
    return true;
}


const uint8_t* LatestViewConstants(ID3D11Resource*& resource) {
    if (!t_viewConstants.resource)
        return nullptr;
    resource = t_viewConstants.resource;
    return t_viewConstants.bytes;
}

bool DrawWithCopy(int which, ID3D11DeviceContext* ctx, ID3D11Resource* original, const uint8_t* bytes, UINT size,
                  const std::function<void()>& draw) {
    thread_local std::pair<UINT, ID3D11Buffer*> t_own[4] = {};
    const int slot = BoundSlot(ctx, original);
    if (slot < 0)
        return false;
    ID3D11Buffer* own = nullptr;
    for (auto& [s, b] : t_own)
        if (b && s == size)
            own = b;
    for (auto& [s, b] : t_own) {
        if (own || b)
            continue;
        ID3D11Device* device = nullptr;
        ctx->GetDevice(&device);
        D3D11_BUFFER_DESC desc{size, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0};
        if (SUCCEEDED(device->CreateBuffer(&desc, nullptr, &b)))
            s = size, own = b;
        device->Release();
    }
    D3D11_MAPPED_SUBRESOURCE mapped;
    // BF3's own Map/Unmap, not the hooked ones: this upload isn't one to inspect.
    if (!own || FAILED(g_origMap[which](ctx, own, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        return false;
    std::memcpy(mapped.pData, bytes, size);
    g_origUnmap[which](ctx, own, 0);
    ctx->VSSetConstantBuffers(slot, 1, &own);
    draw();
    auto buffer = static_cast<ID3D11Buffer*>(original);
    ctx->VSSetConstantBuffers(slot, 1, &buffer);
    return true;
}

bool LatestBlock(ID3D11Resource*& resource, std::vector<uint8_t>& bytes) {
    if (!t_upload.resource)
        return false;
    resource = t_upload.resource;
    bytes = t_upload.bytes;
    return true;
}

bool GameGunBody(float (&out)[12]) {
    std::lock_guard lock(g_handsMutex);
    const float* body = EffectGunBody();
    if (!body)
        return false;
    std::memcpy(out, body, sizeof(out));
    std::lock_guard fp(g_fpMutex);
    const float* origin = EffectOrigin();
    out[3] += origin[0], out[7] += origin[1], out[11] += origin[2];
    return true;
}

int ViewBufferKind(ID3D11Resource* buffer) {
    std::lock_guard lock(g_fpMutex);
    for (const ViewByBuffer& v : g_viewByBuffer)
        if (buffer && v.buffer == buffer)
            return 1;
    return buffer && std::find(std::begin(g_worldViewBuffers), std::end(g_worldViewBuffers), buffer) !=
                         std::end(g_worldViewBuffers)
               ? 2
               : 0;
}

bool FirstPersonViews(float (&arms)[16], float (&world)[16]) {
    std::lock_guard lock(g_fpMutex);
    ULONGLONG now = GetTickCount64();
    if (now - g_fpArmsTime > 500 || now - g_fpWorldTime > 500)  // not seen lately
        return false;
    std::memcpy(arms, g_fpArms, sizeof(arms));
    std::memcpy(world, g_fpWorld, sizeof(world));
    return true;
}

bool LauncherZoomWanted() {
    return g_launcherOn.load(std::memory_order_relaxed) && g_launcherZoom.load(std::memory_order_relaxed) && LauncherInHand();
}

float LauncherUp() {
    return HeldDrawnLast() ? HeldKindNow().up : g_launcherUp.load(std::memory_order_relaxed);
}

bool HeldGunInHand() {
    return HeldDrawnLast();
}

bool HeldGameMoving() {
    return HeldDrawnLast() && g_heldMoving.load(std::memory_order_relaxed);
}

const flap::Aerial* HeldAerial() {
    static constexpr flap::Aerial kFim92Aerial{magposes::kFim92FoldKeys, magposes::kFim92LiftKeys, magposes::kFim92SpringKeys, magposes::kFim92FoldBones, magposes::kFim92LiftHand, magposes::kFim92LiftGlove, &magposes::kFim92LiftFingers[0][0][0], magposes::kFim92LiftBones, magposes::kFim92SpringBones};
    return HeldDrawnLast() && g_launcherHands.load(std::memory_order_relaxed) && HeldKindNow().spreadGun == kStingerGun ? &kFim92Aerial : nullptr;
}

bool At4Held() {
    return HeldDrawnLast() && g_launcherHands.load(std::memory_order_relaxed) && HeldKindNow().spreadGun == kAt4Gun;
}

bool HeldPalms(float* right, float* left) {
    std::lock_guard lock(g_handsMutex);
    // Measured, for those that are, so it isn't a jump when the rest is found; any other's as soon as it is, till then about where most hold a gun.
    const HeldKind& kind = HeldKindNow();
    std::memcpy(right, kind.rightPalm, sizeof(kind.rightPalm));
    std::memcpy(left, kind.leftPalm, sizeof(kind.leftPalm));
    if (kind.bodyIndices)
        return true;
    if (!g_heldPalms.known)
        return false;
    std::memcpy(right, g_heldPalms.palm[0], sizeof(g_heldPalms.palm[0]));
    std::memcpy(left, g_heldPalms.palm[1], sizeof(g_heldPalms.palm[1]));
    return true;
}

const handmag::Gun* HeldMagGun() {
    if (!HeldDrawnLast() || !g_launcherHands.load(std::memory_order_relaxed))
        return nullptr;
    const HeldMagBones* mag = HeldKindNow().mag;
    return mag ? mag->gun : nullptr;
}

bool HeldByHand() {
    return HeldMagGun() != nullptr;
}

const shellload::Gun* HeldTubeGun() {
    if (!HeldDrawnLast() || !g_launcherHands.load(std::memory_order_relaxed))
        return nullptr;
    const HeldTube* tube = TubeOf(HeldKindNow());
    return tube ? tube->gun : nullptr;
}

bool TubeByHand() {
    return g_tubeByHand.load(std::memory_order_relaxed);
}

int ShellTest() {
    return g_shellTest.load(std::memory_order_relaxed);
}

bool ShellLog() {
    return g_shellLog.load(std::memory_order_relaxed);
}

const char* HeldShape() {
    return HeldKindNow().shape;
}

void PullMagazine() {
    if (!g_magOut.exchange(true))
        Log("Magazine: pulled out by hand");
}

void FlickMagazine(const float* velocityView) {
    constexpr float kFlickSpin = 11.0f;  // rad/s: the AN-94's own turns over quickly
    if (g_magOut.exchange(true))
        return;
    if (StartLooseMagazine(false, velocityView, kFlickSpin, !RifleInHand()))
        Log("Magazine: knocked out by the fresh one, flying (loose)");
    else if (!StartDrop(false, velocityView, kFlickSpin))
        Log("Magazine: knocked out, not seen to fall");
    else
        Log("Magazine: knocked out by the fresh one, falling to the floor at %.2f m", g_floor.load());
}

int HeldTest() {
    return g_heldTest.load(std::memory_order_relaxed);
}

float RifleUp() {
    return g_rifleUp.load(std::memory_order_relaxed);
}

bool LauncherHands() {
    return g_launcherHands.load(std::memory_order_relaxed);
}

bool LauncherSteer() {
    return g_launcherSteer.load(std::memory_order_relaxed);
}

int RocketTest() {
    return g_rocketTest.load(std::memory_order_relaxed);
}

bool TakeRocketGameTest() {
    return g_rocketGame.exchange(false);
}

bool LauncherLeftGrip(float (*bones)[12]) {
    std::lock_guard lock(g_handsMutex);
    const RigSet& rig = RigInUse();
    if (!rig.haveLeft)
        return false;
    std::memcpy(bones, rig.left, sizeof(rig.left));
    return true;
}

bool Sight(SightNow& out) {
    out = SightNow{};
    const float scopeOn = g_scopePower.load(std::memory_order_relaxed);  // (1 or less: weapon.txt "scopepower 0", the magnifying off)
    const char* name = nullptr;
    bool say = false;
    {
        std::lock_guard lock(g_handsMutex);
        if (!g_handsOn || !g_hands.gun || g_hands.stowed)  // the game's own arms (no gun drawn by the mod), no hand on the gun (lying, flying into a hand), or drawn nowhere
            return false;
        const uint32_t frameNow = g_frameSerial.load(std::memory_order_relaxed);
        if (HeldDrawnLast()) {
            // Not the Stinger nor the AT4 (their sights are the launchers': recoil keeps no eye off them), nor a gun the mod hasn't measured, nor a
            // switch's frame (its kind still the gun's before).
            const HeldKind& kind = HeldKindNow();
            if (!kind.bodyIndices || kind.spreadGun == kStingerGun || kind.spreadGun == kAt4Gun || g_heldAtKind.load(std::memory_order_relaxed) != kind.bodyIndices)
                return false;
            out.id = kind.bodyIndices;
            std::memcpy(out.line, kind.sights, sizeof(out.line));
            out.up = kind.up;
            out.rearZ = kind.sights[2];  // (one not in kHeldEyes: nothing known on its line short of its muzzle)
            for (const HeldEye& e : kHeldEyes)
                if (e.bodyIndices == kind.bodyIndices)
                    out.eyeZ = e.eyeZ, out.rearZ = e.rearZ;
            // A sight of the table on it: looked through, the line goes through its glass, along the gun's own.
            for (size_t n = 0; n < std::size(kHeldSights); ++n) {
                const GlassSeen& seen = g_glassSeen[n];
                if (!seen.have || frameNow - seen.frame > 2)
                    continue;
                const float* glass = seen.onFrame;
                aimrules::HeldShotLine(kind.sights, out.up, glass, out.line);  // (the line its bullets start on too: DrawHeldPart)
                out.glassZ = out.rearZ = glass[2];
                out.haveGlass = true;
                out.power = scopeOn > 1.0f ? kHeldSights[n].power : 1.0f;
                out.sight = static_cast<int>(n);
                break;
            }
            name = gunprofile::NameOf(kind.shape);
        } else if (RifleInHand() && !LauncherDrawnLast() && !TrainPistolInHand()) {
            out.id = kWeapon.indexCount;
            std::memcpy(out.line, kRifleSightsAtMuzzle, sizeof(out.line));
            out.up = g_rifleUp.load(std::memory_order_relaxed);
            out.eyeZ = kRifleZoomedEyeZ;
            out.rearZ = kRifleRearZ;
            // Its ACOG placed on it lately: its glass as the lens has it (DrawArms), from the scope's body learned on the gun's.
            if (const auto it = g_learned.find(kScopeBody); g_haveAcogFrame && frameNow - g_acogFrame <= 2 && it != g_learned.end() && it->second.have[0]) {
                float onGun[3];
                TransformPoint(it->second.rel[0], g_acogEyepiece, onGun);
                if (std::isfinite(onGun[2])) {
                    out.glassZ = out.rearZ = onGun[2];
                    out.haveGlass = true;
                    out.power = scopeOn > 1.0f ? scopeOn : 1.0f;
                    out.sight = 10;
                }
            }
            name = "M416";
        } else {
            return false;
        }
        const float numbers[] = {out.line[0], out.line[1], out.line[2], out.up, out.eyeZ, out.power, out.glassZ, out.rearZ};
        if (!std::all_of(std::begin(numbers), std::end(numbers), [](float f) { return std::isfinite(f); })) {
            out = SightNow{};
            return false;
        }
        // Once a gun and sight in a run (the first 32 kept), for the films' log, once they've held for 30 asks (a gun's first frames come before its sight's).
        static uint64_t s_said[32] = {}, s_lastKey = 0;
        static size_t s_saidCount = 0;
        static int s_steady = 0;
        const uint64_t key = (static_cast<uint64_t>(out.id) << 8) | static_cast<uint8_t>(out.sight + 1);
        s_steady = key == s_lastKey ? s_steady + 1 : 0;
        s_lastKey = key;
        if (s_steady == 30 && s_saidCount < std::size(s_said) && std::find(s_said, s_said + s_saidCount, key) == s_said + s_saidCount)
            s_said[s_saidCount++] = key, say = true;
    }
    if (say) {
        char tail[96];
        if (out.sight >= 0)
            std::snprintf(tail, sizeof(tail), "the %s (%.2fx), its glass at z %.3f", out.sight == 10 ? "ACOG" : kHeldSightNames[out.sight], out.power, out.glassZ);
        else
            std::snprintf(tail, sizeof(tail), "irons, the nearest of it on the line at z %.3f", out.rearZ);
        Log("Recoil: the sights at the eye on the %s: its line at the muzzle (%.3f %.3f %.3f), %.2f degrees below its z, the game's zoomed eye at z %.3f; %s", name,
            out.line[0], out.line[1], out.line[2], out.up, out.eyeZ, tail);
    }
    return true;
}

void HoldSpread(bool oneHand) {
    const ULONGLONG now = GetTickCount64();
    g_spreadAskedAt = now;
    StepHoldSpread(oneHand, now, kSpreadSettle);
}

// The rig's kept rest only, never its frame of now: through the game's own reload its hands move (the M240's right hand works its lever), and a hand
// mirrored from one moving would leave the grip. Each gun keeps its own rest (RigFor), so one seen before has it at once.
bool RigRestRight(float (*bones)[12], uint32_t& kind, float* palm) {
    std::lock_guard lock(g_handsMutex);
    kind = g_rigKind;
    if (!g_rigFrozen)
        return false;
    const ArmDraw& gloves = kArmDraws[0];
    const auto it = g_rigRest.bones.find(gloves.key);
    if (it == g_rigRest.bones.end())
        return false;
    for (int b = 0; b < handpose::kRightBones; ++b)
        if (!it->second.have[handpose::kRightFirst + b])
            return false;
    float at[3];
    TransformPoint(it->second.rel[gloves.rightRoot], kRightPalm, at);  // (as LearnRig measures the palms on the frame)
    const float* first = it->second.rel[handpose::kRightFirst];
    if (!std::all_of(first, first + handpose::kRightBones * 12, [](float f) { return std::isfinite(f); }) || !std::isfinite(at[0] + at[1] + at[2]))
        return false;
    std::memcpy(bones, first, handpose::kRightBones * sizeof(it->second.rel[0]));
    if (palm)
        std::memcpy(palm, at, sizeof(at));
    return true;
}

bool RigRestLeft(float (*bones)[12], uint32_t& kind) {
    std::lock_guard lock(g_handsMutex);
    kind = g_rigKind;
    if (!g_rigFrozen || !g_rigRest.haveLeft)
        return false;
    const float* first = &g_rigRest.left[0][0];
    if (!std::all_of(first, first + handpose::kBones * 12, [](float f) { return std::isfinite(f); }))
        return false;
    std::memcpy(bones, g_rigRest.left, sizeof(g_rigRest.left));
    return true;
}

void HeldNow(HeldNowOut& out) {
    out = HeldNowOut{};
    std::lock_guard lock(g_handsMutex);  // (RigFor changes the gun in hand only under it, in a draw: these two are the same gun)
    const UINT indices = g_heldKind.load(std::memory_order_relaxed);
    const HeldKind& kind = HeldKindNow();
    out.held = HeldDrawnLast();
    out.id = out.held ? indices : 0;
    // Its palms as HeldPalms gives them: measured, for those that are; any other's once known.
    std::memcpy(out.rightPalm, kind.rightPalm, sizeof(out.rightPalm));
    std::memcpy(out.leftPalm, kind.leftPalm, sizeof(out.leftPalm));
    out.palms = kind.bodyIndices != 0;
    if (!out.palms && g_heldPalms.known) {
        std::memcpy(out.rightPalm, g_heldPalms.palm[0], sizeof(out.rightPalm));
        std::memcpy(out.leftPalm, g_heldPalms.palm[1], sizeof(out.leftPalm));
        out.palms = true;
    }
    // But not known before the rig has taken its kind: a gun's first draw says it's drawn a moment before RigFor takes it, and the kind and palms then are
    // the gun's before (after the launcher, none). Read in that moment, it's a switch's frame: no gun in hand yet.
    if (!indices || g_heldAtKind.load(std::memory_order_relaxed) != indices)
        out.palms = false;
    out.up = kind.up;
    out.right = kind.right;
    out.shape = kind.shape;
}

int AttachmentsOnGun(float (*points)[3], int most, uint32_t& set) {
    set = 0;
    if (!points)
        most = 0;
    std::lock_guard lock(g_handsMutex);
    if (!HeldDrawnLast())
        return 0;
    const uint32_t frameNow = g_frameSerial.load(std::memory_order_relaxed);
    int n = 0;
    for (size_t i = 0; i < std::size(gunbodies::kAttach); ++i) {
        const gunbodies::Attach& a = gunbodies::kAttach[i];
        const AttachSeen& seen = g_attachSeen[i];
        if (!seen.have || a.points <= 0 || frameNow - seen.frame > 2)
            continue;
        set = set * 31u + static_cast<uint32_t>(i + 1);
        for (int p = 0; p < a.points && p < static_cast<int>(std::size(a.point)) && n < most; ++p, ++n)
            TransformPoint(seen.rel0, a.point[p], points[n]);
    }
    return n;
}

bool RifleM320Seen() {
    // Sticky while the rifle is out: met once, it's on it (Update forgets it after 10 s with no rifle drawn, and so does this, should Update not have run).
    const ULONGLONG rifle = g_rifleDrawn.load();
    return g_m320Met.load(std::memory_order_relaxed) && rifle && GetTickCount64() - rifle <= kM320Forget;
}

bool LauncherInHand() {
    return LauncherDrawnLast();
}

bool GunGrip(float* point) {
    std::lock_guard lock(g_handsMutex);
    if (!g_haveGrip)
        return false;
    std::memcpy(point, g_gripModel, sizeof(g_gripModel));
    return true;
}

bool GameLeftGrip(float (*bones)[12]) {
    std::lock_guard lock(g_handsMutex);
    if (!g_haveGameLeft)
        return false;
    std::memcpy(bones, g_gameLeft, sizeof(g_gameLeft));
    return true;
}

bool GameLeftGripM320(float (*bones)[12]) {
    std::lock_guard lock(g_handsMutex);
    if (!g_haveGameLeftM320 && !g_m320GripLoadTried)
        LoadM320Grip();
    if (!g_haveGameLeftM320)
        return false;
    std::memcpy(bones, g_gameLeftM320, sizeof(g_gameLeftM320));
    return true;
}

bool GameRightGrip(float (*bones)[12]) {
    std::lock_guard lock(g_handsMutex);
    // The gloves' own draw's 'R' bones as learned from the game's normal hold (TakeFrameOfHands): the same the right glove is drawn with on the grip.
    const auto it = g_learned.find(kArmDraws[0].key);
    if (it == g_learned.end())
        return false;
    for (int b = 0; b < handpose::kRightBones; ++b)
        if (!it->second.have[handpose::kRightFirst + b])
            return false;
    std::memcpy(bones, it->second.rel[handpose::kRightFirst], handpose::kRightBones * sizeof(it->second.rel[0]));
    return true;
}

void SetLoose(const loose::Result& r, const int* origin) {
    std::lock_guard lock(g_looseMutex);
    g_loosePoseCount = std::min(r.poses, static_cast<int>(std::size(g_loosePoses)));
    std::copy(r.pose, r.pose + g_loosePoseCount, g_loosePoses);
    for (int i = 0; i < 3; ++i)
        g_looseOrigin[i] = static_cast<float>(origin[i]);
}

bool LatestSeenFrom(float* seenFrom, float* origin) {
    std::lock_guard lock(g_fpMutex);
    if (!g_haveLastSeenFrom)
        return false;
    std::memcpy(seenFrom, g_lastSeenFrom, sizeof(g_lastSeenFrom));
    std::memcpy(origin, g_lastSeenOrigin, sizeof(g_lastSeenOrigin));
    return true;
}

bool GameEyeWorld(float* out) {
    std::lock_guard lock(g_fpMutex);
    if (!g_haveViewToArms || !g_haveFpOrigin)
        return false;
    for (int i = 0; i < 3; ++i)
        out[i] = g_viewToArms[i * 4 + 3] + g_fpOrigin[i];
    return true;
}

bool LooseOn() {
    return g_looseOn.load(std::memory_order_relaxed);
}

const loose::Spec* M320CaseSpec() {
    return LooseM320CaseSpec();
}

int TakeLooseDraws() {
    return g_looseDraws.exchange(0, std::memory_order_relaxed);
}

int M320Test() {
    return g_m320TestMode.load(std::memory_order_relaxed);
}

bool M320Log() {
    return g_m320Log.load(std::memory_order_relaxed);
}

bool RifleSeated() {
    return GunSeated();
}

bool M320RoundOnGun(float* rel) {
    std::lock_guard lock(g_handsMutex);
    if (!g_m320RoundSeenAt || GetTickCount64() - g_m320RoundSeenAt > 200)
        return false;
    std::memcpy(rel, g_m320RoundSeen, sizeof(g_m320RoundSeen));
    return true;
}

void SetM320CaseDropped(bool dropped) {
    g_m320CaseDropped = dropped;
}

void M320CaseOnRound(float* out) {
    // kM320RoundToCase takes the round bone's space into the case's; this is the other way round (a pure shift).
    float inverse[12];
    if (!Invert(kM320RoundToCase, inverse))
        std::memcpy(inverse, kM320RoundToCase, sizeof(inverse));
    std::memcpy(out, inverse, sizeof(inverse));
}

void SetHands(const HandTargets* targets) {
    std::lock_guard lock(g_handsMutex);
    g_handsOn = targets != nullptr;
    if (targets) {
        g_hands = *targets;
        g_handsKind = g_heldKind.load(std::memory_order_relaxed);
        GuardFreeHands(g_hands);
        // A gun drawn with no hand on it (free hands: let go of, lying or flying into a hand) has no lens (DrawArms, DrawHeldPart: t.gun), so nothing of the
        // hands is cut for one (LensHole): let go of at the eye, the last frame's hole left a square of the rifle, its scope and the gloves see-through for a
        // frame. (Never while dormant or held: there gunDrawn and launcherDrawn only come with gun. Gun false and neither drawn, nothing's touched.) Nor one
        // drawn nowhere (stowed: no lens drawn).
        if (g_hands.stowed || (!g_hands.gun && (g_hands.gunDrawn || g_hands.launcherDrawn)))
            g_lensHoleFrame = g_frameSerial.load(std::memory_order_relaxed) - 2;
    } else {
        g_haveGunDelta = false;
    }
    g_handsActive = g_handsOn;
    NoteStowed(g_handsOn && g_hands.stowed);
}

void ToggleLift() {
    bool lift = !g_lift.load();
    g_lift = lift;
    if (lift)
        g_research = true;  // the lift happens in the bone search
    {
        std::lock_guard lock(g_mutex);
        LogSeen();
    }
    Log("Skin: lift %s", lift ? "on: camera-relative pieces, 15 cm up" : "off");
    BeepAsync(lift ? 1200 : 440, 200);
}

}

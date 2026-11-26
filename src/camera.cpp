// Camera access. Reading follows the pointer chains from bf3vr.ini; writing uses hardware
// breakpoints, because BF3 rewrites the camera every frame and anything written at the wrong
// moment is lost. A breakpoint fires right after the game writes a camera's height, and the
// handler shifts X and Z along that camera's right vector by the eye offset before anything
// renders from it. The camera has no roll, so the right vector has no height component and the
// handler never writes the watched float itself (which would trigger it again).
// With a controller aiming, the render cameras are also turned to the head, and the weapon
// camera the opposite way so the gun stays with the hand. Aiming itself: see aim.cpp.

#include "camera.h"

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>

#include "headtest.h"
#include "log.h"
#include "views.h"
#include "viewturn.h"

namespace {

constexpr float kPi = 3.14159265f;
constexpr uintptr_t kWatchOffset = 0x34;  // translation.y
constexpr int kSlots = 4;                 // x86 debug registers Dr0..Dr3
constexpr int kMaxWriters = 8;

// The last debug register is for the weapon camera (the copy the gun is drawn from) while the
// view is turned; the others hold render cameras, which get the eye shift.
constexpr int kWeaponSlot = kSlots - 1;

// Slot 2 holds the cull camera (the copy BF3 decides what to draw from), or while hunting for
// it, the candidate being tried. It gets the same head turn as the render cameras in slots 0-1.
constexpr int kCullSlot = 2;

// Transform start per debug register, 0 = unused, and where in its block the breakpoint sits
// (relative to the transform). The exception handler reads these.
std::atomic<uintptr_t> g_slots[kSlots];
std::atomic<uintptr_t> g_slotOffsets[kSlots];
std::atomic<float> g_eyeOffset{0.0f};
std::atomic<bool> g_viewsMode{false};  // views.cpp makes the render and cull cameras itself
std::atomic<uintptr_t> g_weaponAddress{0};
// The weapon camera's yaw as the game last wrote it (atan2 of its back row's x and -z, the way the
// look angles store theirs), before the mod turns it, and when (GetTickCount64).
std::atomic<float> g_weaponYaw{0.0f}, g_weaponPitch{0.0f};  // pitch: -asin of the back row's y, as stored
std::atomic<ULONGLONG> g_weaponYawAt{0};
std::atomic<bool> g_gunFlip{false};
// weapon.txt "wcam x y z" (SetWeaponTest): the camera the gun is placed from moved along its own
// axes, to see what follows it: the gun, its muzzle flash, the casings, the eye.
std::atomic<bool> g_weaponTest{false};
std::atomic<float> g_weaponTestOffset[3];
std::atomic<uint32_t> g_hits{0};
std::atomic<uintptr_t> g_writers[kMaxWriters];
int g_writersLogged = 0;

// ---- Render FOV ----
// BF3's render view (the object the render cameras live in: transform at +0x30, FOV in radians
// at +0x78 and +0x7C, near and far at +0x80, aspect at +0x88, projection matrix at +0x2F0) is
// rebuilt every frame. Its FOV comes from the game camera's view at bf3.exe+1F8B480 (same
// layout), itself rewritten every frame: a write test found that changing bf3.exe+1F8B4F8 changes
// the render view's FOV. The campaign renders 55 degrees whatever the menu says.
constexpr uintptr_t kFovSource = 0x1F8B4F8;    // bf3.exe offset
constexpr uintptr_t kFovFromTransform = 0x48;  // a view's FOV pair, relative to its transform
constexpr uintptr_t kProjectionFromTransform = 0x2C0;
std::atomic<float> g_renderFov{0.0f};          // radians, 0 = the game's own
uintptr_t g_fovSource = 0;                     // absolute, set by StartWatch
// A breakpoint on the source never fired (its writer may be a thread hidden from debug
// registers), and the source code is wrapped on disk. The render and cull views, though, are
// filled by one block copy each frame, transform first, FOV after. With the wider view on, their
// breakpoints watch the near plane just past the FOV instead of the height, so they fire once
// both are in place: the turn, eye shift and FOV are all written after the copy.
constexpr uintptr_t kNearFromTransform = 0x50;
std::atomic<uint32_t> g_slotHits[kSlots];
// Main render camera writes since xr_runtime last asked: one per game frame, if every frame
// shown is built exactly once between two Presents (which the alternate-eye scheme assumes).
std::atomic<uint32_t> g_cameraBuilds{0};
// The render cameras' object holds three such views: the main one (+0x0, its transform is render
// camera 1 at +0x30), the cull view (+0x480) and the first-person view (+0x900), which draws the
// gun and arms. Widening only the main view left the gun at 55 degrees: shown life-size, about
// twice too big. Its FOV pair is written from every camera breakpoint, so whichever fires after
// the game has filled that view in makes it stick. That changed the view's own matrices but not
// the gun, which is drawn from a 55-degree projection made elsewhere; skin.cpp rescales that one
// where it is uploaded to the graphics card.
constexpr uintptr_t kMainTransform = 0x30;
constexpr uintptr_t kFirstPersonView = 0x900;
// The block holding a render view's transform, relative to the transform.
constexpr uintptr_t kViewFromTransform = 0x30;

// Where in a slot's block the breakpoint sits, relative to the transform.
uintptr_t WatchOffset(int slot, bool fov) {
    return fov && (slot == 0 || slot == kCullSlot) ? kNearFromTransform : kWatchOffset;
}

bool LooksLikeFov(float v) {
    return v > 0.2f && v < 2.8f;  // radians; also rejects NaN
}

// A view's FOV pair, only where it looks like one.
void WriteFov(uintptr_t address, float fov) {
    auto f = reinterpret_cast<float*>(address);
    if (LooksLikeFov(f[0]) && LooksLikeFov(f[1]))
        f[0] = f[1] = fov;
}

// View turn for the handler: written into the inactive buffer, then published by flipping the index.
struct ViewTurnState {
    bool active;
    camera::ViewTurn turn;
};
ViewTurnState g_view[2] = {};
std::atomic<int> g_viewIndex{0};
std::atomic<ULONGLONG> g_turnLogUntil{0};  // weapon.txt "viewlog N" (TurnToHead)
std::atomic<bool> g_mirrorFix{true};       // weapon.txt "mirrorfix 0": a mirrored camera turned as any (TurnToHead)
// weapon.txt "cutsceneturn 0": a scripted camera (its rows' w lanes filled: viewturn.h) refused as before, the
// picture held still in the room (TurnToHead).
std::atomic<bool> g_cutsceneTurn{true};
// Cameras turned to the head since TakeTurned, and in the high half how many of them were scripted ones: one
// word, so a picture's two counts are taken together.
std::atomic<uint64_t> g_turned{0};
constexpr uint64_t kScriptedTurned = 1ull << 32;
std::atomic<uint32_t> g_refusedInactive{0}, g_refusedNotCamera{0};  // TurnToHead's refusals (TakeTurned logs them)
float g_refusedMatrix[16] = {};
// views.cpp's cameras of one frame (both eyes and their cull cameras) turned with one published turn (BeginViews,
// the view builder's thread), and what the first of them showed (ShownTurn): its yaw as built (low half) and the
// yaw taken out of it (high half), two floats in one word so the render thread reads them together.
ViewTurnState g_frameView = {};
bool g_frameLatched = false, g_frameFirst = false;
std::atomic<uint64_t> g_shown{0};
std::atomic<ULONGLONG> g_shownAt{0};  // when it was (GetTickCount64; 0 never): older than kShownFresh, stale
constexpr ULONGLONG kShownFresh = 250;
// A camera the mod had already turned, offered again in a frame's views (TurnToHead): its yaw the last frame's
// shown yaw within kRepeatSame after a jump of more than kRepeatJump, its pitch the last shown pitch within
// kRepeatPitch (the head's, baked in: a game camera whose yaw lands there by chance is still turned). The last
// frame's first camera as built and as shown (radians), and the same latched for this frame by BeginViews, so
// both eyes' copies are judged alike. The view builder's thread only.
constexpr float kRepeatSame = 0.1f / 57.29578f, kRepeatJump = 5.0f / 57.29578f, kRepeatPitch = 0.5f / 57.29578f;
float g_lastCameraYaw = 0.0f, g_lastShownYaw = 0.0f, g_lastShownPitch = 0.0f;
float g_refCameraYaw = 0.0f, g_refShownYaw = 0.0f, g_refShownPitch = 0.0f;
bool g_haveLast = false, g_haveRef = false;
uint32_t g_repeats = 0;  // left as they were since the last log
// A scripted camera's level yaw goes on from the last frame's (viewturn::MakeLevel): from the one BeginViews latched
// (g_levelRef), so all of a frame's cameras are made level alike, and the frame's first scripted camera gives the
// one the next frame goes on from (g_level). Gone on from as each came, the eyes and their cull cameras would each
// have been taken a share further to their own yaw. The view builder's thread only.
viewturn::Level g_level, g_levelRef;
bool g_levelTaken = false;
// A scripted scene's straight ahead (sceneview.h): taken from the head its first scripted camera is turned with (TurnToHead), kept for every one
// of that scene (the view builder's thread only); published for xr_runtime.cpp (SceneFacing: the scene's number in the high half, the yaw's bits
// in the low, one word, 0 none). weapon.txt "scenefacing 0": none taken, the scene's forward at the room's straight ahead as before.
std::atomic<bool> g_sceneFacing{true};
sceneview::Latch g_sceneLatch;
std::atomic<uint64_t> g_sceneShown{0};
// What one view build's cameras (BeginViews to BeginViews) came as: turned as the player's kind, turned as scripted
// ones, refused as not a camera. A build with a scripted camera and another kind as well is counted for TakeTurned's
// log, with its three counts (a byte each, in that order from the low one).
int g_buildKinds[3] = {};
std::atomic<uint32_t> g_unalikeBuilds{0}, g_unalikeKinds{0};
// The body lock (skin.cpp): each view build's turn of its main camera (BodyTurns), the last kBodyBuilds of them, the
// newest at g_bodyNext - 1; an entry not valid for a build that turned no scripted main camera (the player's kind, a
// refused one, the look angles written). Written once a build (its first main camera: g_bodyNoted, the view builder's
// thread only), read by the threads that upload, under g_bodyMutex; whether the newest is valid, and when it was made,
// in atomics too, for every upload to ask without the lock.
constexpr int kBodyBuilds = 4;
struct BodyEntry {
    bool valid = false;
    camera::BodyTurn turn;
};
std::mutex g_bodyMutex;
BodyEntry g_bodyEntries[kBodyBuilds];
int g_bodyNext = 0;
uint32_t g_bodyBuild = 0;
bool g_bodyNoted = false;
std::atomic<bool> g_bodyNewestValid{false};
std::atomic<ULONGLONG> g_bodyNewestAt{0};
// This build's body (FrameBody; headmove.h): the view builder's thread only.
bool g_frameBodyHave = false, g_frameBodyScripted = false;
float g_frameBodyRight[3] = {}, g_frameBodyBack[3] = {};
// Each view build's main camera rows and the pose it was turned by (camera.h NoteDrawnView), the last kBuildPoses of them
// by their serial; the build the frame being drawn comes from, once its main view has named it, and the last shown.
struct BuildPose {
    uint32_t serial = 0, poseId = 0;
    float rows[9] = {};
    float head[4] = {0.0f, 0.0f, 0.0f, 1.0f}, place[3] = {};  // the head's as that pose has it
    // Only with detail asked for (SetDrawnDetail: the head test's meters), and then this serial's own while detail == serial: its main camera as the game made it (rows and place), the look
    // yaw and the hand's yaw of its turn, whether the look was written, and how far the head's place moved it (NoteBodyMove).
    uint32_t detail = 0;
    float made[9] = {}, madePlace[3] = {}, move[3] = {}, lookYaw = 0.0f, handYaw = 0.0f;
    bool look = false;
};
std::atomic<bool> g_drawnDetail{false};
constexpr uint32_t kBuildPoses = 8;
std::mutex g_buildPoseMutex;
BuildPose g_buildPoses[kBuildPoses];
uint32_t g_buildPoseSerial = 0, g_drawnSerial = 0, g_lastDrawnSerial = 0;
std::atomic<bool> g_drawnNamed{false};
bool g_buildPoseNoted = false;  // this build's: the view builder's thread only
std::atomic<bool> g_poseByDraw{true};
std::atomic<bool> g_pureRows{true};  // weapon.txt "purerows 0": the picture's turn taken from the game's camera (viewturn::Turn alone)
// SetPoseTries. The head's turn smoothed (the user's three rounds of versions, 2026-11-04: M, O and Q, all with it, the steadiest; N, the place alone, not), its place a little, the turn carried
// on by about what the filter and the picture's age take from it, or the picture swings further from the head and its edge shows; the edge filled.
// (The filter that opens with the head's speed is on: with each picture named by the cameras it was drawn with, the user found it steady and with the least black, 2026-11-12.)
std::atomic<float> g_poseTries[camera::kPoseTryCount] = {6.0f, 12.0f, 0.0f, 0.0f, 80.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
std::atomic<uint32_t> g_handsPose{0};  // SetHandsPose
std::atomic<float> g_waveDegrees{0.0f}, g_waveSeconds{2.0f};
// weapon.txt "nameby" (camera.h NoteDrawnView): which of a picture's main-view uploads name the view build it was drawn from. 0: its first, as ever.
std::atomic<int> g_nameBy{2};  // by the build most of the picture's main views carried: the headset is then told the pose the world was drawn with (0: by the first upload, as it was)
// With "nameby" 1 or 2 a picture's name is only known once it is drawn, and its gun and arms are drawn before that. They are placed for the build as far from the
// one this frame's first upload fits as the last picture's name was from its own first upload's (g_lastOffset, in builds: 0 where the rule names as the first
// upload does, and then this is "nameby 0" to the letter; 1 where the picture begins with the build before and is named the newest), fixed at the frame's first
// such draw (g_handsSerial; 0: not yet), so every draw of them in one picture is placed for one build. (Counted back from the newest build instead, a rule that
// named as the first upload does had the gun a head's step off at both changes of state where "nameby 0" has it off at one.) Under g_buildPoseMutex.
uint32_t g_handsSerial = 0;
int g_lastOffset = 0;
// This frame's per-view uploads (camera.h NoteDrawnView). Under g_buildPoseMutex.
struct FrameViews {
    // The main-view uploads by build and by where in the frame they came, twice over. naming: those that tell one build (or builds alike of one pose) and no
    // other, for the count ("Picture age"): what it says of which cameras a pass was drawn with is sure. all: every one of them, by the newest build its rows fit:
    // what "nameby" 1 and 2 and the world's turn go by. A picture's views carry the newest build or the one before and nothing older, so where rows fit several
    // builds (the head standing still, or come back to within a hundredth of a degree of where it was two builds ago, as at the turning point of a shake) the
    // newest of them is the one. Left out of the name, such a picture was named by its few views of the build before alone, a step of the head off.
    headtest::Naming naming, all;
    uint32_t unclear = 0, stray = 0;  // main views that fit builds of more than one pose (the head still: nothing told), and views in the world that fit none
    // The camera rows as uploaded: of the frame's first main view, of its last, and of the last before the first eye's picture went onto the window; and the
    // build each of them fits (the newest of them where builds are alike).
    bool haveFirst = false, haveLast = false, haveEyes = false;
    float firstRows[9] = {}, lastRows[9] = {}, eyesRows[9] = {};
    uint32_t firstBuild = 0, lastBuild = 0, eyesBuild = 0;
    // And of the last upload counted for each build in all (at the build's place in the ring, while carriedBuild there is that build): the world's turn holds the
    // pose told against rows the game really uploaded, not against the build's own record of them.
    uint32_t carriedBuild[kBuildPoses] = {};
    float carriedRows[kBuildPoses][9] = {};
    // One picture's uploads in the order they came, for the line a second (kept only while counting): what each was (kOrder...), the build it carried, its eye along
    // its own right row (which eye's place: only uploads with the same rows compare), the thread it came on and that thread's count of small uploads as it came
    // (0: not given): the uploads between one view and the next are the per-draw constants of what is drawn with it, near enough.
    struct Entry {
        uint32_t build = 0;
        float along = 0.0f;
        uint8_t kind = 0;
        DWORD thread = 0;
        uint32_t at = 0;
    };
    static constexpr int kMost = 112;
    Entry order[kMost];
    int count = 0, cut = 0;  // cut: how many came after the room ran out
};
static_assert(headtest::Naming::kBuilds == static_cast<int>(kBuildPoses), "a picture's uploads are counted for as many builds as are kept");
enum : uint8_t { kOrderMain, kOrderUnclear, kOrderStray, kOrderFirstPerson, kOrderFirstPersonFits, kOrderEyeFirst, kOrderEyeSecond };
FrameViews g_frameViews;
// The picture TakeDrawnPose last closed, as DrawnWorldView hands it out.
camera::DrawnWorld g_drawnWorld;
bool g_haveDrawnWorld = false;
// Over the pictures since the last line: the uploads and the pictures by build (0 the newest as the picture is handed over, 1 the one before, 2 older).
struct ViewCountSums {
    uint32_t pictures = 0, none = 0, unclear = 0, stray = 0;
    uint32_t uploads[3] = {}, firstBy[3] = {}, lastBy[3] = {}, namedFirst[3] = {}, named[3] = {};
    uint32_t stage[3][3] = {};                          // the uploads by where they came (before the first eye's picture, between the two, after the second) and by build
    uint32_t turned = 0, allBefore = 0, allNewest = 0;  // pictures that began with the build before and ended with the newest; with the build before all through; the newest all through
    uint32_t mixed = 0, handsOff = 0;                   // pictures whose uploads carried more than one build; whose gun and arms were placed for another build than they were named by
    // In the pictures that began with the build before (named a build behind by their first upload): their uploads by where they came and by build; and the build of
    // their last upload before the first eye's picture went out (lateEyes), with those that had none before it and those whose first eye's picture was not marked.
    uint32_t lateStage[3][3] = {}, lateEyes[3] = {}, lateEyesNone = 0, lateUnmarked = 0;
    // What each kind of per-view upload carries (Followed): the small uploads that came after a main view of the newest build, of the one before, of an older one,
    // after a first-person view, after any other; in all pictures, and in those that began with the build before.
    uint32_t after[5] = {}, lateAfter[5] = {};
    char order[FrameViews::kMost * 7 + 40] = "";  // the last such picture's uploads, a letter each with what it carries (OrderLine)
};
ViewCountSums g_viewSums;
ULONGLONG g_viewSumsLogged = 0;

bool CountingViews() {
    return g_drawnDetail.load(std::memory_order_relaxed) || GetTickCount64() < g_turnLogUntil.load(std::memory_order_relaxed);
}

// A per-view upload's eye is in the world: past the arms' space's reach of the origin on some axis. The game's first-person views (the gun's and arms' own) come in the
// arms' space, the world less whole metres, their eye a metre or two from its origin.
bool InWorld(const double* eye) {
    return !(std::fabs(eye[0]) < viewturn::kBodyReach && std::fabs(eye[1]) < viewturn::kBodyReach && std::fabs(eye[2]) < viewturn::kBodyReach);
}

// How many small uploads came after one of a picture's per-view uploads and before the next (on the same thread): the per-draw constants of what is drawn with that
// view, near enough. -1: not known (no count given with it, the picture's last one, or the next came on another thread).
int Followed(const FrameViews& v, int i) {
    const FrameViews::Entry& e = v.order[i];
    if (!e.at || e.kind == kOrderEyeFirst || e.kind == kOrderEyeSecond)
        return -1;
    for (int j = i + 1; j < v.count; ++j) {
        const FrameViews::Entry& next = v.order[j];
        if (next.kind == kOrderEyeFirst || next.kind == kOrderEyeSecond)
            continue;
        const uint32_t between = next.at - e.at;  // (the count may wrap: only the difference is used)
        if (!next.at || next.thread != e.thread || between == 0 || between > 100000)
            return -1;
        return static_cast<int>(between - 1);
    }
    return -1;
}

// One picture's uploads in the order they came, a letter each. N: a main view carrying the newest build's rows; B: the build before; O: an older one; capital, the
// left eye's place (or the only place that build's uploads came from), small, the right eye's. ?: a main view that fits more than one build. .: a view in the world
// that fits none. f: a first-person view (the arms' space); F: one whose rows fit a build. |: the first eye's picture going onto the window; /: the second's. A
// letter followed by ' came on another thread than the one that hands the picture over. The number after a letter: the small uploads that came after it before the
// next per-view upload (Followed), where known. Under g_buildPoseMutex.
void OrderLine(const FrameViews& v, uint32_t newest, char* out, size_t size) {
    // Which eye's place: against the build's own camera where its place was kept (a head test's detail), else against the middle of that build's uploads' places.
    float low[kBuildPoses], high[kBuildPoses];
    bool any[kBuildPoses] = {};
    for (int i = 0; i < v.count; ++i) {
        const FrameViews::Entry& e = v.order[i];
        if (e.kind != kOrderMain)
            continue;
        const uint32_t at = e.build % kBuildPoses;
        low[at] = any[at] ? std::min(low[at], e.along) : e.along;
        high[at] = any[at] ? std::max(high[at], e.along) : e.along;
        any[at] = true;
    }
    const DWORD here = GetCurrentThreadId();
    size_t n = 0;
    auto put = [&](char c) {
        if (n + 1 < size)
            out[n++] = c;
    };
    for (int i = 0; i < v.count; ++i) {
        const FrameViews::Entry& e = v.order[i];
        char c = '.';
        if (e.kind == kOrderMain) {
            const uint32_t behind = newest >= e.build ? newest - e.build : 0;
            c = behind == 0 ? 'N' : behind == 1 ? 'B' : 'O';
            const uint32_t at = e.build % kBuildPoses;
            const BuildPose& b = g_buildPoses[at];
            bool right = false;
            if (b.serial == e.build && b.detail == b.serial) {
                const double middle = static_cast<double>(b.madePlace[0] + b.move[0]) * b.rows[0] + static_cast<double>(b.madePlace[1] + b.move[1]) * b.rows[1] +
                                      static_cast<double>(b.madePlace[2] + b.move[2]) * b.rows[2];
                right = e.along - middle > 0.005;
            } else if (high[at] - low[at] > 0.02f) {
                right = e.along > 0.5f * (low[at] + high[at]);
            }
            if (right)
                c = static_cast<char>(c + ('a' - 'A'));
        } else if (e.kind == kOrderUnclear) {
            c = '?';
        } else if (e.kind == kOrderFirstPerson) {
            c = 'f';
        } else if (e.kind == kOrderFirstPersonFits) {
            c = 'F';
        } else if (e.kind == kOrderEyeFirst) {
            c = '|';
        } else if (e.kind == kOrderEyeSecond) {
            c = '/';
        }
        put(c);
        if (e.thread != here && e.kind != kOrderEyeFirst && e.kind != kOrderEyeSecond)
            put('\'');
        if (const int followed = Followed(v, i); followed >= 0 && n + 8 < size)
            n += std::snprintf(out + n, size - n, "%d", followed);
    }
    if (v.cut > 0 && n + 16 < size)
        n += std::snprintf(out + n, size - n, " +%d more", v.cut);
    out[std::min(n, size - 1)] = 0;
}

// The picture being handed over: what its uploads say of it kept for DrawnWorldView, their counts into the sums, and the lines once a second. newest: the newest view
// build there is now; byFirst: the build its first upload named (0 none); named: the build it is named by. Under g_buildPoseMutex.
void CloseFrameViews(uint32_t newest, uint32_t byFirst, uint32_t named) {
    const FrameViews& c = g_frameViews;
    const headtest::Naming& u = c.naming;
    // The world's turn's part: the rows as uploaded, and with each the build they fit (its own rows and the head it was turned with: the body those cameras were
    // drawn with).
    g_haveDrawnWorld = c.haveLast;
    g_drawnWorld = camera::DrawnWorld{};
    if (c.haveLast) {
        auto fitted = [](uint32_t serial, camera::DrawnWorld::Build& out) {
            const BuildPose& b = g_buildPoses[serial % kBuildPoses];
            out.have = serial != 0 && b.serial == serial;
            if (out.have) {
                std::memcpy(out.rows, b.rows, sizeof(out.rows));
                std::memcpy(out.head, b.head, sizeof(out.head));
            }
        };
        g_drawnWorld.marked = c.haveEyes;
        std::memcpy(g_drawnWorld.rows, c.haveEyes ? c.eyesRows : c.lastRows, sizeof(g_drawnWorld.rows));
        fitted(c.haveEyes ? c.eyesBuild : c.lastBuild, g_drawnWorld.build);
        g_drawnWorld.haveFirst = c.haveFirst;
        std::memcpy(g_drawnWorld.firstRows, c.firstRows, sizeof(g_drawnWorld.firstRows));
        fitted(c.firstBuild, g_drawnWorld.firstBuild);
        if (const uint32_t most = c.all.Most(); most && c.carriedBuild[most % kBuildPoses] == most) {
            g_drawnWorld.haveMost = true;
            std::memcpy(g_drawnWorld.mostRows, c.carriedRows[most % kBuildPoses], sizeof(g_drawnWorld.mostRows));  // (as uploaded)
            fitted(most, g_drawnWorld.mostBuild);
        }
        g_drawnWorld.mixed = c.all.Builds() > 1;
        g_drawnWorld.handsOff = g_nameBy.load(std::memory_order_relaxed) != 0 && g_handsSerial != 0 && named != 0 && g_handsSerial != named;
    }
    if (!CountingViews()) {
        g_viewSums = ViewCountSums{};
        g_frameViews = FrameViews{};
        return;
    }
    auto by = [newest](uint32_t serial) { return newest - serial >= 2 ? 2u : newest - serial; };
    ViewCountSums& s = g_viewSums;
    ++s.pictures;
    s.unclear += c.unclear;
    s.stray += c.stray;
    uint32_t got[3] = {}, staged[3][3] = {};
    for (int i = 0; i < headtest::Naming::kBuilds; ++i)
        if (const uint32_t b = u.build[i]; b && b <= newest)
            for (int k = 0; k < 3; ++k) {
                got[by(b)] += u.count[i][k];
                staged[k][by(b)] += u.count[i][k];
            }
    for (int i = 0; i < 3; ++i) {
        s.uploads[i] += got[i];
        for (int k = 0; k < 3; ++k)
            s.stage[k][i] += staged[k][i];
    }
    if (byFirst && byFirst <= newest)
        ++s.namedFirst[by(byFirst)];
    if (named && named <= newest)
        ++s.named[by(named)];
    s.mixed += u.Builds() > 1;
    s.handsOff += g_drawnWorld.handsOff;
    // What each kind of per-view upload carries: the small uploads that came after it before the next.
    uint32_t after[5] = {};
    for (int i = 0; i < c.count; ++i) {
        const int followed = Followed(c, i);
        if (followed < 0)
            continue;
        const FrameViews::Entry& e = c.order[i];
        const uint32_t kind = e.kind == kOrderMain ? (e.build && e.build <= newest ? by(e.build) : 4u) : e.kind == kOrderFirstPerson || e.kind == kOrderFirstPersonFits ? 3u : 4u;
        after[kind] += static_cast<uint32_t>(followed);
    }
    for (int i = 0; i < 5; ++i)
        s.after[i] += after[i];
    if (!u.first || u.first > newest || u.last > newest) {
        ++s.none;
    } else {
        ++s.firstBy[by(u.first)];
        ++s.lastBy[by(u.last)];
        s.turned += by(u.first) == 1 && by(u.last) == 0;
        s.allBefore += got[1] > 0 && got[0] == 0 && got[2] == 0;
        s.allNewest += got[0] > 0 && got[1] == 0 && got[2] == 0;
        if (by(u.first) == 1) {
            for (int i = 0; i < 3; ++i)
                for (int k = 0; k < 3; ++k)
                    s.lateStage[k][i] += staged[k][i];
            if (u.eyes == 0)
                ++s.lateUnmarked;
            else if (!u.lastBeforeEyes || u.lastBeforeEyes > newest)
                ++s.lateEyesNone;
            else
                ++s.lateEyes[by(u.lastBeforeEyes)];
            for (int i = 0; i < 5; ++i)
                s.lateAfter[i] += after[i];
            OrderLine(c, newest, s.order, sizeof(s.order));  // (the second's last such picture is the one shown)
        }
    }
    const ULONGLONG now = GetTickCount64();
    if (now - g_viewSumsLogged >= 1000) {
        g_viewSumsLogged = now;
        const uint32_t late = s.firstBy[1];
        Log("Picture age: %u pictures; their first main-view upload named the newest view build %u times, the one before %u, an older one %u; all their main-view uploads (352 bytes) that "
            "fit one build alone: %u carried the newest, %u the one before, %u an older one (%u fit more than one: the head still); a picture's first carried the newest %u times, the one "
            "before %u, older %u; its last of the frame carried the newest %u times, the one before %u, older %u; pictures that began with the one before and ended with the newest %u, "
            "with the one before all through %u, with the newest all through %u; with no such upload %u; in the pictures that began with the one before, %u uploads carried the newest and "
            "%u the one before; of those %u pictures the last upload before the first eye's picture went onto the window carried the newest in %u, the one before in %u, an older one in %u "
            "(%u had none before it, in %u that moment was not marked); their uploads before the first eye's picture: %u the newest, %u the one before; between the two eyes' pictures: %u "
            "and %u; after the second: %u and %u; all pictures' uploads the same way: %u and %u, %u and %u, %u and %u; %u views in the world fit no build; %u pictures had uploads of "
            "more than one build; named (nameby %d) by the newest %u times, the one before %u, older %u; the gun and arms placed for another build than the picture's in %u; what "
            "each kind of per-view upload carries, the small uploads that came after it before the next, in the pictures that began with the one before: %u after a main view of "
            "the newest build, %u after one of the build before, %u after an older one's, %u after a first-person view, %u after any other; in all pictures: %u, %u, %u, %u and %u",
            s.pictures, s.namedFirst[0], s.namedFirst[1], s.namedFirst[2], s.uploads[0], s.uploads[1], s.uploads[2], s.unclear, s.firstBy[0], s.firstBy[1], s.firstBy[2], s.lastBy[0],
            s.lastBy[1], s.lastBy[2], s.turned, s.allBefore, s.allNewest, s.none, s.lateStage[0][0] + s.lateStage[1][0] + s.lateStage[2][0],
            s.lateStage[0][1] + s.lateStage[1][1] + s.lateStage[2][1], late, s.lateEyes[0], s.lateEyes[1], s.lateEyes[2], s.lateEyesNone, s.lateUnmarked, s.lateStage[0][0],
            s.lateStage[0][1], s.lateStage[1][0], s.lateStage[1][1], s.lateStage[2][0], s.lateStage[2][1], s.stage[0][0], s.stage[0][1], s.stage[1][0], s.stage[1][1], s.stage[2][0],
            s.stage[2][1], s.stray, s.mixed, g_nameBy.load(std::memory_order_relaxed), s.named[0], s.named[1], s.named[2], s.handsOff, s.lateAfter[0], s.lateAfter[1],
            s.lateAfter[2], s.lateAfter[3], s.lateAfter[4], s.after[0], s.after[1], s.after[2], s.after[3], s.after[4]);
        // One picture's uploads as they came: the second's last picture that began with the build before, or with none such this very one.
        char order[sizeof(s.order)];
        const bool began = s.order[0] != 0;
        if (began)
            std::memcpy(order, s.order, sizeof(order));
        else
            OrderLine(c, newest, order, sizeof(order));
        if (order[0])
            Log("Picture order: a picture's per-view uploads as they came (%s): %s  (N a main view with the newest build's cameras, B the build before's, O an older one's; capital at "
                "the left eye's place, small at the right's; ? fits more than one build; . in the world, fits none; f a first-person view, F one that fits a build; | the first eye's "
                "picture going onto the window, / the second's; ' on another thread; the number after a letter: the small uploads that came after it before the next per-view "
                "upload, the draws it carries, near enough)",
                began ? "the second's last that began with the build before" : "this one: none began with the build before", order);
        g_viewSums = ViewCountSums{};
    }
    g_frameViews = FrameViews{};
}

// made: the camera as the game made it, before the turn (all 16 floats), for the head test's detail.
void NoteBuildPose(const float* turned, const camera::ViewTurn& turn, const float* made) {
    if (g_buildPoseNoted)
        return;
    g_buildPoseNoted = true;
    std::lock_guard lock(g_buildPoseMutex);
    const uint32_t serial = ++g_buildPoseSerial;
    BuildPose& e = g_buildPoses[serial % kBuildPoses];
    e.serial = serial;
    e.poseId = turn.poseId;
    std::memcpy(e.head, turn.head, sizeof(e.head));
    std::memcpy(e.place, turn.headPlace, sizeof(e.place));
    for (int r = 0; r < 3; ++r)
        for (int i = 0; i < 3; ++i)
            e.rows[3 * r + i] = turned[4 * r + i];
    if (g_drawnDetail.load(std::memory_order_relaxed)) {
        e.detail = serial;
        for (int r = 0; r < 3; ++r)
            for (int i = 0; i < 3; ++i)
                e.made[3 * r + i] = made[4 * r + i];
        for (int i = 0; i < 3; ++i) {
            e.madePlace[i] = made[12 + i];
            e.move[i] = 0.0f;
        }
        e.look = turn.haveLook;
        e.lookYaw = turn.lookYaw;
        e.handYaw = turn.currentYaw;
    }
}

// A build's entry as the head test reads it (under g_buildPoseMutex).
void FillDrawn(const BuildPose& e, camera::DrawnCamera& out) {
    out.build = e.serial;
    out.poseId = e.poseId;
    std::memcpy(out.rows, e.rows, sizeof(out.rows));
    std::memcpy(out.gameRows, e.made, sizeof(out.gameRows));
    std::memcpy(out.gamePlace, e.madePlace, sizeof(out.gamePlace));
    std::memcpy(out.move, e.move, sizeof(out.move));
    out.look = e.look;
    out.lookYaw = e.lookYaw;
    out.handYaw = e.handYaw;
}

// The body from a main camera turned to the head: the head's turn taken back off it leaves the game's camera less the
// hand's yaw and the game's pitch (viewturn::ViewTurnLocal), or a scripted camera as made level. Made level again, in case
// of a roll the turn doesn't know of. Once a build: both eyes' main cameras are turned alike.
void NoteFrameBody(const float* turned, const camera::ViewTurn& turn, bool mirrored, bool scripted) {
    if (g_frameBodyHave)
        return;
    float inverse[4] = {-turn.head[0], -turn.head[1], -turn.head[2], turn.head[3]};
    if (mirrored) {  // as viewturn::Turn mirrored it
        inverse[1] = -inverse[1];
        inverse[2] = -inverse[2];
    }
    float rows[16];
    std::memcpy(rows, turned, sizeof(rows));
    viewturn::TurnLocal(rows, inverse);
    const float backLength = std::sqrt(rows[8] * rows[8] + rows[10] * rows[10]);
    const float rightLength = std::sqrt(rows[0] * rows[0] + rows[2] * rows[2]);
    if (!(backLength > 0.1f) || !(rightLength > 0.1f))
        return;  // never for a camera with the game's pitch taken out
    g_frameBodyRight[0] = rows[0] / rightLength, g_frameBodyRight[1] = 0.0f, g_frameBodyRight[2] = rows[2] / rightLength;
    g_frameBodyBack[0] = rows[8] / backLength, g_frameBodyBack[1] = 0.0f, g_frameBodyBack[2] = rows[10] / backLength;
    g_frameBodyScripted = scripted;
    g_frameBodyHave = true;
}

// This view build's main camera noted for the body lock (once a build): turned as a scripted one with no look angles
// written (scripted), from before (as the game made it) to after, its first-person camera body (or nullptr) as the
// game made it (bodyMade) and as turned; or not (a valid entry no more: the body's views are left as they are).
void NoteBodyTurn(bool scripted, const float* before, const float* after, const float* bodyMade, const float* body) {
    if (g_bodyNoted)
        return;
    g_bodyNoted = true;
    BodyEntry entry;
    entry.turn.build = g_bodyBuild;
    entry.turn.at = GetTickCount64();
    entry.valid = scripted && viewturn::OwnFrameTurn(before, after, entry.turn.turn);
    if (entry.valid) {
        for (int r = 0; r < 3; ++r)
            for (int i = 0; i < 3; ++i) {
                entry.turn.before[3 * r + i] = before[4 * r + i];
                entry.turn.after[3 * r + i] = after[4 * r + i];
                if (body && viewturn::Classify(bodyMade, true) != viewturn::kNone) {
                    entry.turn.firstPersonMade[3 * r + i] = bodyMade[4 * r + i];
                    entry.turn.firstPerson[3 * r + i] = body[4 * r + i];
                }
            }
        for (int i = 0; i < 3; ++i)
            entry.turn.place[i] = before[12 + i];
    }
    {
        std::lock_guard lock(g_bodyMutex);
        g_bodyEntries[g_bodyNext] = entry;
        g_bodyNext = (g_bodyNext + 1) % kBodyBuilds;
    }
    g_bodyNewestAt.store(entry.turn.at, std::memory_order_relaxed);
    g_bodyNewestValid.store(entry.valid, std::memory_order_release);
}

// Camera copies the finder (F10) saw this session, which the F6 hunt tries one at a time.
std::mutex g_testMutex;
std::vector<uintptr_t> g_testCopies;
std::atomic<int> g_huntIndex{-1};  // -1 = not hunting
ULONGLONG g_huntStepTime = 0;
constexpr ULONGLONG kHuntStepMs = 3000;

// Beep without holding up the render thread it's called from.
void BeepAsync(DWORD frequency, DWORD duration) {
    DWORD packed = (frequency << 16) | duration;
    if (HANDLE t = CreateThread(nullptr, 0, [](LPVOID p) -> DWORD {
            DWORD v = static_cast<DWORD>(reinterpret_cast<uintptr_t>(p));
            Beep(v >> 16, v & 0xFFFF);
            return 0;
        }, reinterpret_cast<LPVOID>(static_cast<uintptr_t>(packed)), 0, nullptr))
        CloseHandle(t);
}

uintptr_t DebugRegister(const CONTEXT* c, int i) {
    switch (i) {
    case 0: return c->Dr0;
    case 1: return c->Dr1;
    case 2: return c->Dr2;
    default: return c->Dr3;
    }
}

void SetDebugRegister(CONTEXT* c, int i, uintptr_t value) {
    switch (i) {
    case 0: c->Dr0 = value; break;
    case 1: c->Dr1 = value; break;
    case 2: c->Dr2 = value; break;
    default: c->Dr3 = value; break;
    }
}

// Dr7 bits for breakpoint i: local enable, and its RW/LEN field.
constexpr uintptr_t EnableBits(int i) {
    return (static_cast<uintptr_t>(3) << (2 * i)) | (static_cast<uintptr_t>(0xF) << (16 + 4 * i));
}

void RecordWriter(uintptr_t ip) {
    for (auto& slot : g_writers) {
        uintptr_t current = slot.load(std::memory_order_relaxed);
        if (current == ip)
            return;
        if (current == 0 && slot.compare_exchange_strong(current, ip))
            return;
    }
}

// The camera check and the turn's maths (viewturn.h, so tools/viewturntest.cpp runs them too): IsCamera, the
// player's camera's kind, is the check everything here keeps to; only TurnToHead takes scripted ones as well.
using viewturn::IsCamera;
using viewturn::TurnLocal;
using viewturn::ViewTurnLocal;

// The handler wrote a watched float itself (a camera's translation y, where the breakpoint sits):
// the breakpoint fires again inside it. Writing it unguarded recursed until the stack overflowed
// (the weapon camera test, 2026-10-30), so that write is let through.
thread_local bool t_inHandler = false;

// Runs on the game thread that just wrote a camera. Must stay tiny: no locks, no logging.
LONG CALLBACK OnException(EXCEPTION_POINTERS* info) {
    if (info->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP)
        return EXCEPTION_CONTINUE_SEARCH;
    CONTEXT* c = info->ContextRecord;
    if (!(c->Dr6 & 0xF))
        return EXCEPTION_CONTINUE_SEARCH;
    if (t_inHandler) {
        c->Dr6 = 0;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    t_inHandler = true;

    float offset = g_eyeOffset.load(std::memory_order_relaxed);
    for (int i = 0; i < kSlots; ++i) {
        if (!(c->Dr6 & (1u << i)))
            continue;
        uintptr_t watched = g_slots[i].load(std::memory_order_relaxed);
        uintptr_t watchOffset = g_slotOffsets[i].load(std::memory_order_relaxed);
        if (!watched || DebugRegister(c, i) != watched + watchOffset) {
            // Left over from a camera address that is no longer current: disarm it in this thread.
            c->Dr7 &= ~EnableBits(i);
            continue;
        }
#ifdef _WIN64
        RecordWriter(c->Rip);
#else
        RecordWriter(c->Eip);
#endif
        g_hits.fetch_add(1, std::memory_order_relaxed);
        g_slotHits[i].fetch_add(1, std::memory_order_relaxed);
        auto m = reinterpret_cast<float*>(watched);
        // Only touch memory that holds a camera right now: BF3 reuses these blocks for other
        // data, and shifting anything else corrupts it.
        if (!IsCamera(m))
            continue;
        if (i == 0)
            g_cameraBuilds.fetch_add(1, std::memory_order_relaxed);
        if (i != kWeaponSlot && g_viewsMode.load(std::memory_order_relaxed))
            continue;  // already turned, shifted and widened where the views were built
        // Wider FOV: the render view's and the cull view's own pair, once the copy has put the
        // game's there (the breakpoint then sits past it, on the near plane), and the
        // first-person view's.
        if (float fov = g_renderFov.load(std::memory_order_relaxed); fov > 0.0f && i != kWeaponSlot) {
            if (watchOffset == kNearFromTransform)
                WriteFov(watched + kFovFromTransform, fov);
            if (uintptr_t main = g_slots[0].load(std::memory_order_relaxed)) {
                uintptr_t firstPerson = main - kMainTransform + kFirstPersonView + kViewFromTransform;
                if (IsCamera(reinterpret_cast<const float*>(firstPerson)))
                    WriteFov(firstPerson + kFovFromTransform, fov);
            }
        }
        const ViewTurnState& view = g_view[g_viewIndex.load(std::memory_order_acquire)];
        float local[4];
        if (view.active)
            ViewTurnLocal(m, view.turn, local);
        if (watched == g_weaponAddress.load(std::memory_order_relaxed)) {
            // As the game wrote it, built from the real look angles (aim.cpp tells them by it).
            g_weaponYaw.store(std::atan2(m[8], -m[10]), std::memory_order_relaxed);
            g_weaponPitch.store(-std::asin(std::clamp(m[9], -1.0f, 1.0f)), std::memory_order_relaxed);
            g_weaponYawAt.store(GetTickCount64(), std::memory_order_relaxed);
            // BF3 places the gun from the weapon camera relative to the picture's camera, so
            // turning the weapon camera the opposite way keeps the gun with the hand. Which way
            // is "opposite" is confirmed in game (F1 flips it).
            if (view.active) {
                if (!g_gunFlip.load(std::memory_order_relaxed))
                    local[0] = -local[0], local[1] = -local[1], local[2] = -local[2];
                TurnLocal(m, local);
            }
            if (g_weaponTest.load(std::memory_order_relaxed)) {
                const float o[3] = {g_weaponTestOffset[0].load(std::memory_order_relaxed),
                                    g_weaponTestOffset[1].load(std::memory_order_relaxed),
                                    g_weaponTestOffset[2].load(std::memory_order_relaxed)};
                for (int k = 0; k < 3; ++k)
                    m[12 + k] += m[k] * o[0] + m[4 + k] * o[1] + m[8 + k] * o[2];
            }
            continue;  // the weapon camera never gets the eye shift
        }
        if (view.active)
            TurnLocal(m, local);
        if (offset != 0.0f) {
            m[12] += m[0] * offset;
            m[14] += m[2] * offset;
        }
    }
    t_inHandler = false;
    c->Dr6 = 0;
    return EXCEPTION_CONTINUE_EXECUTION;
}

// Data breakpoints after any write to the 4 bytes at each target plus its offset (0 = slot unused).
bool ArmThread(DWORD threadId, const uintptr_t (&targets)[kSlots], const uintptr_t (&offsets)[kSlots]) {
    HANDLE thread = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME, FALSE, threadId);
    if (!thread)
        return false;
    bool ok = false;
    if (SuspendThread(thread) != static_cast<DWORD>(-1)) {
        CONTEXT c{};
        c.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        if (GetThreadContext(thread, &c)) {
            for (int i = 0; i < kSlots; ++i) {
                c.Dr7 &= ~EnableBits(i);
                SetDebugRegister(&c, i, targets[i] ? targets[i] + offsets[i] : 0);
                if (targets[i])  // L_i, RW_i = write, LEN_i = 4 bytes
                    c.Dr7 |= (static_cast<uintptr_t>(1) << (2 * i)) | (static_cast<uintptr_t>(1) << (16 + 4 * i)) |
                             (static_cast<uintptr_t>(3) << (18 + 4 * i));
            }
            ok = SetThreadContext(thread, &c) != 0;
        }
        ResumeThread(thread);
    }
    CloseHandle(thread);
    return ok;
}

// While BF3 rebuilds objects (level loads, video settings) a chain can briefly land somewhere
// else, even on a misaligned address. Only watch, and so only write to, a real camera transform.
bool LooksValid(uintptr_t address) {
    if (address & 0xF)
        return false;
    float m[16];
    return camera::SafeRead(address, m, sizeof(m)) && IsCamera(m);
}

// What should be watched right now: render cameras, the cull camera (or hunt candidate), and
// the weapon camera while the view is turned.
void DesiredSlots(uintptr_t (&out)[kSlots]) {
    std::fill(std::begin(out), std::end(out), 0);
    bool weapon = g_view[g_viewIndex.load()].active || g_weaponTest.load();

    uintptr_t cull = 0;
    int hunt = g_huntIndex.load();
    if (hunt >= 0) {
        std::lock_guard lock(g_testMutex);
        if (hunt < static_cast<int>(g_testCopies.size()))
            cull = g_testCopies[hunt];
    } else if (!camera::Resolve(config::GetCull(), cull)) {
        cull = 0;
    }
    if (cull && LooksValid(cull))
        out[kCullSlot] = cull;

    int renderSlots = out[kCullSlot] ? kCullSlot : weapon ? kWeaponSlot : kSlots;
    std::vector<config::CameraPointer> cameras = config::GetCameras();
    for (size_t i = 0; i < cameras.size() && i < static_cast<size_t>(renderSlots); ++i) {
        uintptr_t address = 0;
        if (camera::Resolve(cameras[i], address) && LooksValid(address))
            out[i] = address;
    }
    uintptr_t weaponAddress = 0;
    if (weapon && camera::Resolve(config::GetAim().gameCamera, weaponAddress) && LooksValid(weaponAddress))
        out[kWeaponSlot] = weaponAddress;
    g_weaponAddress = out[kWeaponSlot] ? weaponAddress : 0;
}

// The game's own threads. (A Toolhelp snapshot lists every thread on the PC: 11 ms with SteamVR
// and browsers running, and this thread took one about every 50 ms.)
std::vector<DWORD> GameThreads() {
    using NextThread = LONG(NTAPI*)(HANDLE, HANDLE, ACCESS_MASK, ULONG, ULONG, PHANDLE);
    static const auto next =
        reinterpret_cast<NextThread>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtGetNextThread"));
    std::vector<DWORD> ids;
    if (!next)
        return ids;
    HANDLE thread = nullptr, following = nullptr;
    while (next(GetCurrentProcess(), thread, THREAD_QUERY_LIMITED_INFORMATION, 0, 0, &following) >= 0) {
        if (thread)
            CloseHandle(thread);
        thread = following;
        ids.push_back(GetThreadId(thread));
    }
    if (thread)
        CloseHandle(thread);
    return ids;
}

// Every 50 ms: re-resolve what to watch, since BF3 moves the render camera to new memory every
// few seconds and each moment unwatched is a frame without the eye shift. Game threads are
// re-armed straight away when it moves, and new threads are picked up twice a second.
DWORD WINAPI WatchThread(LPVOID) {
    std::vector<DWORD> armed;
    uintptr_t current[kSlots] = {}, currentOffsets[kSlots] = {};
    int missing[kSlots] = {};
    for (int tick = 0;; ++tick) {
        Sleep(50);
        uintptr_t desired[kSlots], offsets[kSlots];
        DesiredSlots(desired);
        bool fov = g_renderFov.load() > 0.0f;
        for (int i = 0; i < kSlots; ++i)
            offsets[i] = WatchOffset(i, fov);
        // A camera block often fails validation for a moment while BF3 rewrites it. Keep the
        // breakpoint on it for up to a second rather than re-arming every thread each flicker;
        // the handler checks the block again before every write anyway.
        // The render and cull cameras' chain goes through memory BF3 reuses every frame: it
        // resolves to something else about one read in eight (sampled 2026-10-30), and once,
        // after the opening ride, for good, which lost the wide view (the game drew 55 degrees).
        // The cameras themselves stay put for minutes, so those keep their breakpoint for as long
        // as the block still holds a camera transform.
        for (int i = 0; i < kSlots; ++i) {
            if (desired[i] || !current[i]) {
                missing[i] = 0;
            } else if (i != kWeaponSlot && LooksValid(current[i])) {
                desired[i] = current[i];
            } else if (++missing[i] < 20) {
                desired[i] = current[i];
            } else if (missing[i] == 20) {
                // Dropped once (logged once): which part of the camera check its block failed (The Great
                // Destroyer's train checkpoint drops the weapon slot for its 7.4 s opening sequence).
                float m[16] = {};
                const bool read = camera::SafeRead(current[i], m, sizeof(m));
                auto row = [&m](int r) {
                    return std::sqrt(m[r * 4] * m[r * 4] + m[r * 4 + 1] * m[r * 4 + 1] + m[r * 4 + 2] * m[r * 4 + 2]);
                };
                Log("Camera watch: slot %d dropped (%08llX %s): rows %.3f %.3f %.3f, w %.3f %.3f %.3f", i,
                    static_cast<unsigned long long>(current[i]), read ? "readable" : "unreadable", row(0), row(1), row(2),
                    m[3], m[7], m[11]);
            }
        }
        bool moved = !std::equal(std::begin(desired), std::end(desired), std::begin(current)) ||
                     !std::equal(std::begin(offsets), std::end(offsets), std::begin(currentOffsets));
        if (moved) {
            armed.clear();
            std::copy(std::begin(desired), std::end(desired), std::begin(current));
            std::copy(std::begin(offsets), std::end(offsets), std::begin(currentOffsets));
            // Address first and offset second could briefly pair a new address with an old
            // breakpoint; the handler then just disarms that thread until it is re-armed below.
            for (int i = 0; i < kSlots; ++i) {
                g_slots[i] = desired[i];
                g_slotOffsets[i] = offsets[i];
            }
            Log("Camera watch: watching %08X %08X %08X %08X%s", static_cast<unsigned>(desired[0]),
                static_cast<unsigned>(desired[1]), static_cast<unsigned>(desired[2]),
                static_cast<unsigned>(desired[3]), fov ? " (render and cull views past their FOV)" : "");
        }
        if (!moved && tick % 10 != 0)
            continue;

        DWORD self = GetCurrentThreadId();
        for (DWORD id : GameThreads()) {
            if (id == self || std::find(armed.begin(), armed.end(), id) != armed.end())
                continue;
            if (ArmThread(id, current, currentOffsets))
                armed.push_back(id);
        }
    }
}

void LogWatch() {
    bool any = false;
    for (auto& slot : g_slots)
        any |= slot.load() != 0;
    if (!any)
        return;
    uint32_t hits = g_hits.exchange(0);
    Log("Camera watch: %u writes in the last second, eye offset %.3f m", hits, g_eyeOffset.load());
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    for (; g_writersLogged < kMaxWriters; ++g_writersLogged) {
        uintptr_t ip = g_writers[g_writersLogged].load();
        if (!ip)
            break;
        Log("Camera watch: written by the instruction before bf3.exe+%X", static_cast<unsigned>(ip - base));
    }

    Log("Camera watch: per slot %u %u %u %u", g_slotHits[0].exchange(0), g_slotHits[1].exchange(0),
        g_slotHits[2].exchange(0), g_slotHits[3].exchange(0));
    // What the game actually renders with: the render view's FOV and the one its projection
    // matrix was built from (scale = 1 / tan(FOV / 2)).
    constexpr float kDeg = 57.29578f;
    auto fromScale = [](float scale) { return scale > 0.0f ? 2.0f * std::atan(1.0f / scale) * kDeg : 0.0f; };
    float source = 0.0f, view[2] = {}, cull[2] = {}, scale = 0.0f, firstPerson[2] = {}, firstScale = 0.0f;
    camera::SafeRead(g_fovSource, &source, sizeof(source));
    if (uintptr_t render = g_slots[0].load()) {
        camera::SafeRead(render + kFovFromTransform, view, sizeof(view));
        camera::SafeRead(render + kProjectionFromTransform, &scale, sizeof(scale));
        uintptr_t fp = render - kMainTransform + kFirstPersonView + kViewFromTransform;
        camera::SafeRead(fp + kFovFromTransform, firstPerson, sizeof(firstPerson));
        camera::SafeRead(fp + kProjectionFromTransform, &firstScale, sizeof(firstScale));
    }
    if (uintptr_t cullView = g_slots[kCullSlot].load())
        camera::SafeRead(cullView + kFovFromTransform, cull, sizeof(cull));
    Log("FOV: source %.1f, render view %.1f (its projection %.1f), cull view %.1f, first person %.1f (its "
        "projection %.1f); wanted %.1f degrees; the two eyes built with %.1f",
        source * kDeg, view[0] * kDeg, fromScale(scale), cull[0] * kDeg, firstPerson[0] * kDeg, fromScale(firstScale),
        g_renderFov.load() * kDeg, views::RenderedFov() * kDeg);
}

}

namespace camera {

// Pointer chains go stale between levels, so every read must survive bad memory.
bool SafeRead(uintptr_t address, void* out, size_t size) {
    __try {
        std::memcpy(out, reinterpret_cast<const void*>(address), size);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool Resolve(const config::CameraPointer& pointer, uintptr_t& address) {
    if (!pointer.base)
        return false;
    address = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)) + pointer.base;
    for (uintptr_t offset : pointer.offsets) {
        uintptr_t next = 0;
        if (!SafeRead(address, &next, sizeof(next)) || !next)
            return false;
        address = next + offset;
    }
    return true;
}

bool Read(Transform& out) {
    uintptr_t address = 0;
    return Resolve(config::GetCamera(), address) && SafeRead(address, &out, sizeof(out));
}

void Probe() {
    static ULONGLONG lastLog = 0;
    ULONGLONG now = GetTickCount64();
    if (now - lastLog < 1000)
        return;
    lastLog = now;
    LogWatch();
    if (!config::GetCamera().base)
        return;

    Transform t;
    if (!Read(t)) {
        Log("Camera probe: pointer chain did not resolve");
        return;
    }
    Log("Camera probe: pos (%.2f, %.2f, %.2f) forward (%.2f, %.2f, %.2f)", t.trans[0], t.trans[1], t.trans[2],
        t.forward[0], t.forward[1], t.forward[2]);
}

void StartWatch() {
    g_fovSource = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)) + kFovSource;
    AddVectoredExceptionHandler(1, OnException);
    if (HANDLE t = CreateThread(nullptr, 0, WatchThread, nullptr, 0, nullptr))
        CloseHandle(t);
}

void SetEyeOffset(float metres) {
    g_eyeOffset = metres;
}

void SetViewTurn(const ViewTurn* turn) {
    int next = 1 - g_viewIndex.load(std::memory_order_relaxed);
    g_view[next].active = turn != nullptr;
    if (turn)
        g_view[next].turn = *turn;
    g_viewIndex.store(next, std::memory_order_release);
}

void SetWeaponTest(const float* offset) {
    if (offset)
        for (int k = 0; k < 3; ++k)
            g_weaponTestOffset[k] = offset[k];
    if (g_weaponTest.exchange(offset != nullptr) != (offset != nullptr) || offset)
        Log("Camera: the weapon camera test is %s (%.2f %.2f %.2f along its right, up and back)",
            offset ? "on" : "off", offset ? offset[0] : 0.0f, offset ? offset[1] : 0.0f, offset ? offset[2] : 0.0f);
}

void SetGunFlip(bool flip) {
    g_gunFlip = flip;
}

void SetRenderFov(float radians) {
    g_renderFov = radians;
}

// weapon.txt "renderfov" (a test): degrees, 0 none. Said when it changes.
std::atomic<float> g_testFov{0.0f};

void SetTestFov(float degrees) {
    const float kept = std::isfinite(degrees) && degrees >= 40.0f && degrees <= 140.0f ? degrees : 0.0f;
    if (g_testFov.exchange(kept, std::memory_order_relaxed) != kept) {
        if (kept > 0.0f)
            Log("Render FOV: %.2f degrees while weapon.txt says \"renderfov\" (a test: the picture's size in pixels stays)", kept);
        else
            Log("Render FOV: as bf3vr.ini has it again (weapon.txt \"renderfov\" gone)");
    }
}

float TestFov() {
    return g_testFov.load(std::memory_order_relaxed);
}

float WantedFov() {
    return g_renderFov.load(std::memory_order_relaxed);
}

void SetViewsMode(bool on) {
    g_viewsMode = on;
}

bool TurnToHead(float* transform, float* body) {
    // The frame's turn (BeginViews): both eyes and their cull cameras turned alike.
    const ViewTurnState& view = g_frameLatched ? g_frameView : g_view[g_viewIndex.load(std::memory_order_acquire)];
    // Why a camera wasn't turned, counted for TakeTurned's log: after a load the game's own arms showed
    // 6.5 s with no camera turned at all (the user: "it could be quicker", 16:28).
    if (!view.active) {
        g_refusedInactive.fetch_add(1, std::memory_order_relaxed);
        g_haveLast = false;  // nothing turned: no last picture to tell a repeated camera by
        if (body)
            NoteBodyTurn(false, nullptr, nullptr, nullptr, nullptr);
        return false;
    }
    // A scripted camera (a cutscene's or a ride's, its rows' w lanes filled: viewturn.h) is turned as well, unless
    // weapon.txt says "cutsceneturn 0": made level about its yaw (viewturn::MakeLevel) and the head put on it, so the
    // game draws where the head looks and there are no edges to see (the opening drive of Comrades, 2026-11-01). The
    // player's kind of camera (IsCamera) goes as it always went; anything else is refused. With a scripted camera the
    // view's first-person camera (body) is turned by the same turn, in its own frame (viewturn::TurnWith), and the
    // turn goes to the body lock (NoteBodyTurn; skin.cpp): the player's own arms, gun and legs are drawn with views the
    // game uploads, which it doesn't make from that camera, and left as the game made them they moved with the head
    // ("lock the body while I move my head", the user, 2026-11-01; still so with that camera turned, 09:24).
    const viewturn::Kind kind = viewturn::Classify(transform, g_cutsceneTurn.load(std::memory_order_relaxed));
    if (kind == viewturn::kNone) {
        if (g_refusedNotCamera.fetch_add(1, std::memory_order_relaxed) == 0)
            std::memcpy(g_refusedMatrix, transform, sizeof(g_refusedMatrix));
        ++g_buildKinds[2];
        if (body)
            NoteBodyTurn(false, nullptr, nullptr, nullptr, nullptr);
        return false;
    }
    const bool scripted = kind == viewturn::kScripted;
    ++g_buildKinds[scripted ? 1 : 0];
    float before[16];
    std::memcpy(before, transform, sizeof(before));
    // A cutscene's camera can come mirrored (the user, The Great Destroyer 2026-10-31: as Kovac walks
    // in, turning the head left turned the view right; the look angles weren't even known then, so
    // nothing fought the game): its rows the other handedness from the player's own camera, learned
    // while the look angles are written (that camera follows them; never from a scripted camera, which
    // the player's never is, though its first pictures can come with the look still written). The turn
    // is then mirrored the same way, across its right row: yaw and roll the other way, pitch as it is.
    static std::atomic<int> s_normalHand{0};
    static int s_lastHand = 0;
    const float* r0 = transform;
    const float* r1 = transform + 4;
    const float* r2 = transform + 8;
    const float det = r0[0] * (r1[1] * r2[2] - r1[2] * r2[1]) - r0[1] * (r1[0] * r2[2] - r1[2] * r2[0]) +
                      r0[2] * (r1[0] * r2[1] - r1[1] * r2[0]);
    const int hand = det < 0.0f ? -1 : 1;
    if (view.turn.haveLook && !scripted && s_normalHand.exchange(hand) != hand)
        Log("Camera: the player's camera has %s rows", hand > 0 ? "right-handed" : "left-handed");
    const int normal = s_normalHand.load(std::memory_order_relaxed);
    const bool mirrored = normal != 0 && hand != normal && g_mirrorFix.load(std::memory_order_relaxed);
    static ULONGLONG s_handLogged = 0;
    if (hand != s_lastHand && GetTickCount64() - s_handLogged >= 1000) {
        s_lastHand = hand;
        s_handLogged = GetTickCount64();
        Log("Camera: the camera turned to the head has %s rows (det %.3f)%s", hand > 0 ? "right-handed" : "left-handed",
            det, mirrored ? ": mirrored from the player's own, the head's turn mirrored to match" : "");
    }
    // weapon.txt "viewlog N": for N seconds, each frame's first camera turned: the hand's yaw, the
    // look yaw written, the game camera's yaw and place, the head's yaw and the yaw shown. The user
    // saw the view move with the gun though the look yaw checked out (2026-10-30): this shows
    // whether it turns or the game's camera slides. Marked " | nudge" (mouse aim holding its corrections for the
    // search's nudge), " | again" (a camera already turned, left as it was: below), and " | scripted" with the
    // pitch and roll the game gave a scripted camera, left out.
    static ULONGLONG lastLog = 0;
    auto logView = [&](const char* mark) {
        const ULONGLONG now = GetTickCount64();
        if (now >= g_turnLogUntil.load(std::memory_order_relaxed) || now - lastLog < 10)
            return;
        lastLog = now;
        const float* q = view.turn.head;
        const float forward[3] = {-2 * (q[0] * q[2] + q[3] * q[1]), -2 * (q[1] * q[2] - q[3] * q[0]),
                                  -(1 - 2 * (q[0] * q[0] + q[1] * q[1]))};
        const float deg = 57.29578f;
        char how[64] = "";
        if (scripted) {
            float pitch = 0.0f, roll = 0.0f;
            viewturn::PitchRoll(before, pitch, roll);
            std::snprintf(how, sizeof(how), " | scripted, pitch %.1f roll %.1f left out", pitch * deg,
                          hand * roll * deg);
        }
        Log("View: hand %.2f written %.2f | camera %.2f at %.3f %.3f %.3f | head %.2f shown %.2f%s%s | pitch head %.2f shown %.2f",
            view.turn.currentYaw * deg, view.turn.lookYaw * deg, std::atan2(before[8], -before[10]) * deg, before[12],
            before[13], before[14], std::atan2(-forward[0], -forward[2]) * deg,
            std::atan2(transform[8], -transform[10]) * deg, how, mark, std::asin(std::clamp(forward[1], -1.0f, 1.0f)) * deg,
            std::asin(std::clamp(-transform[9], -1.0f, 1.0f)) * deg);
    };
    // A camera the mod had already turned, offered again in this frame's views: its yaw the last frame's shown yaw
    // within 0.04 degrees after a jump of 5 to 75 degrees (19 frames in The Great Destroyer, 18:30-18:44; at the
    // game's own place, no eye shift: the weapon camera as OnException turned it). With a look yaw the turn reads
    // the camera's own yaw and comes out the same; without one the yaw was taken out of it a second time and the
    // picture flicked for a frame by the hand's angle from the head (-75 degrees at 18:42:40.994, 18:42:45.660,
    // 18:43:03.694 and 18:44:27.259, standing still on the train). It's left as it was turned.
    const float cameraYaw = std::atan2(before[8], -before[10]);
    if (!view.turn.haveLook && !mirrored && g_haveRef &&
        std::fabs(std::remainder(cameraYaw - g_refShownYaw, 2 * kPi)) < kRepeatSame &&
        std::fabs(std::remainder(cameraYaw - g_refCameraYaw, 2 * kPi)) > kRepeatJump &&
        std::fabs(std::asin(std::clamp(-before[9], -1.0f, 1.0f)) - g_refShownPitch) < kRepeatPitch) {
        // Turned (before): the picture isn't an unturned one.
        g_turned.fetch_add(scripted ? 1 + kScriptedTurned : 1, std::memory_order_relaxed);
        static ULONGLONG s_repeatLogged = 0;
        ++g_repeats;
        if (GetTickCount64() - s_repeatLogged >= 1000) {
            s_repeatLogged = GetTickCount64();
            Log("Camera: %u camera%s the mod had turned came again (the last picture's yaw, %.1f degrees from the game's): "
                "left as turned",
                g_repeats, g_repeats == 1 ? "" : "s", std::fabs(std::remainder(cameraYaw - g_refCameraYaw, 2 * kPi)) * 57.29578f);
            g_repeats = 0;
        }
        logView(" | again");
        if (body) {
            NoteFrameBody(transform, view.turn, mirrored, scripted);
            NoteBuildPose(transform, view.turn, before);
        }
        return true;
    }
    const bool first = g_frameFirst;
    g_frameFirst = false;
    // A scripted camera's level yaw goes on from the last frame's only within one run of them (a quarter of a second
    // without one ends it): the run's first takes its own yaw (viewturn::MakeLevel), and is logged with what was left
    // out of it.
    static ULONGLONG s_scriptedAt = 0;
    static bool s_bodyLogged = false;  // the run's first-person camera logged (below)
    if (scripted) {
        const ULONGLONG now = GetTickCount64();
        if (now - s_scriptedAt > kShownFresh)
            g_levelRef.have = false;  // for the rest of this frame's cameras too
        if (now - s_scriptedAt > 1000) {
            float pitch = 0.0f, roll = 0.0f;
            viewturn::PitchRoll(before, pitch, roll);
            Log("Camera: a scripted camera (its rows' w lanes %.3f %.3f %.3f) turned to the head about its own yaw "
                "%.1f, its pitch %.1f and roll %.1f degrees left out",
                before[3], before[7], before[11], cameraYaw * 57.29578f, pitch * 57.29578f, hand * roll * 57.29578f);
            s_bodyLogged = false;
        }
        s_scriptedAt = now;
    }
    // The run's first first-person camera turned with a scripted one is logged as the game made it, against the main
    // camera: how far its rows are turned from the main camera's (TurnWith keeps it on the scene either way), and its
    // place from the main camera's less whole metres, the whole metres logged on their own. The game's first-person
    // views come in the arms' space too, the world less whole metres (skin.cpp): a camera in that space at the main
    // camera's place is that origin turned round from it, the rest 0. It and the main camera's turn go to the body
    // lock (NoteBodyTurn), once a build: the main camera of each eye comes with it, both turned alike.
    const bool logBody = body && scripted && !view.turn.haveLook && !s_bodyLogged;
    float bodyBefore[16] = {};
    if (body)
        std::memcpy(bodyBefore, body, sizeof(bodyBefore));
    viewturn::Level level = g_levelRef;
    // A scripted scene's straight ahead (sceneview.h): its first scripted camera takes it from the head it's turned with (the owner's breach,
    // 2026-11-26: the scene shown 85 degrees off his face, at the room's straight ahead), every camera after it in the scene the same, so the
    // scene's forward is where the head faced as it began, from its first picture.
    float sceneYaw = 0.0f;
    if (scripted && g_sceneFacing.load(std::memory_order_relaxed)) {
        bool fresh = false;
        sceneYaw = sceneview::LatchYaw(g_sceneLatch, view.turn.sceneNumber, view.turn.head, fresh);
        if (fresh) {
            uint32_t bits = 0;
            std::memcpy(&bits, &sceneYaw, sizeof(bits));
            g_sceneShown.store(static_cast<uint64_t>(view.turn.sceneNumber) << 32 | bits, std::memory_order_release);
            const float degrees = sceneYaw * 57.29578f;
            Log("Camera: a scripted scene begins (scene %u): its straight ahead put where the head faces, %.0f degrees %s of the room's (the "
                "scene's own yaw %.1f); the head looks about it from there, and play goes on from it after (weapon.txt scenefacing 0: at the room's, "
                "as before)",
                view.turn.sceneNumber, std::fabs(degrees), degrees >= 0.0f ? "left" : "right", cameraYaw * 57.29578f);
        }
    }
    const float taken = viewturn::Turn(transform, view.turn, scripted, hand, mirrored, level, body, sceneYaw);
    if (!scripted && !mirrored && hand > 0 && view.turn.haveLook && view.turn.lookSign < 0.0f && g_pureRows.load(std::memory_order_relaxed))
        viewturn::PureRows(transform, view.turn);  // nothing of the game's camera in the picture's turn
    if (scripted && !g_levelTaken) {
        g_level = level;
        g_levelTaken = true;
    }
    if (body) {
        NoteBodyTurn(scripted && !view.turn.haveLook, before, transform, bodyBefore, body);
        NoteFrameBody(transform, view.turn, mirrored, scripted);
        NoteBuildPose(transform, view.turn, before);
    }
    if (logBody) {
        s_bodyLogged = true;
        auto apart = [](const float* p, const float* q) {  // degrees between two rows
            const float cross[3] = {p[1] * q[2] - p[2] * q[1], p[2] * q[0] - p[0] * q[2], p[0] * q[1] - p[1] * q[0]};
            return std::atan2(std::sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]),
                              p[0] * q[0] + p[1] * q[1] + p[2] * q[2]) * 57.29578f;
        };
        float whole[3], rest[3];
        for (int i = 0; i < 3; ++i) {
            const float d = bodyBefore[12 + i] - before[12 + i];
            whole[i] = std::round(d);
            rest[i] = d - whole[i];
        }
        Log("Camera: the view's first-person camera (+0x930; the body's own views are turned as they're uploaded) %s; "
            "as the game made it, its forward %.1f and its up %.1f degrees from the scripted camera's, its place %.3f m "
            "from it less whole metres (%.0f %.0f %.0f of them apart: none in the world, the arms' space's origin "
            "turned round in that space), its rows' w lanes %.3f %.3f %.3f",
            viewturn::Classify(bodyBefore, true) != viewturn::kNone ? "turned with it, about its own place"
                                                                    : "left as the game made it: not a camera",
            apart(bodyBefore + 8, before + 8), apart(bodyBefore + 4, before + 4),
            std::sqrt(rest[0] * rest[0] + rest[1] * rest[1] + rest[2] * rest[2]), whole[0], whole[1], whole[2],
            bodyBefore[3], bodyBefore[7], bodyBefore[11]);
    }
    g_turned.fetch_add(scripted ? 1 + kScriptedTurned : 1, std::memory_order_relaxed);
    // What the frame's first camera showed, for xr_runtime.cpp's picture body (not a mirrored one: its turn is
    // the other way round), and for the next frame's check above. A scripted camera's body is its yaw as made
    // level, with nothing taken out.
    if (first && !mirrored) {
        const float shownCamera = scripted ? std::atan2(level.back[0], -level.back[1]) : cameraYaw;
        uint32_t low = 0, high = 0;
        std::memcpy(&low, &shownCamera, sizeof(low));
        std::memcpy(&high, &taken, sizeof(high));
        g_shown.store(static_cast<uint64_t>(high) << 32 | low, std::memory_order_release);
        g_shownAt.store(GetTickCount64(), std::memory_order_release);
        g_lastCameraYaw = cameraYaw;
        g_lastShownYaw = std::atan2(transform[8], -transform[10]);
        g_lastShownPitch = std::asin(std::clamp(-transform[9], -1.0f, 1.0f));
        g_haveLast = true;
    }
    logView(view.turn.nudging ? " | nudge" : "");
    return true;
}

void BeginViews() {
    // The last build's cameras, if a scripted one came with another kind (TakeTurned logs it).
    if (g_buildKinds[1] > 0 && (g_buildKinds[0] > 0 || g_buildKinds[2] > 0)) {
        auto byte = [](int n) { return static_cast<uint32_t>(std::min(n, 255)); };
        g_unalikeKinds.store(byte(g_buildKinds[0]) | (byte(g_buildKinds[1]) << 8) | (byte(g_buildKinds[2]) << 16),
                             std::memory_order_relaxed);
        g_unalikeBuilds.fetch_add(1, std::memory_order_relaxed);
    }
    g_buildKinds[0] = g_buildKinds[1] = g_buildKinds[2] = 0;
    g_frameView = g_view[g_viewIndex.load(std::memory_order_acquire)];
    g_frameLatched = true;
    g_frameFirst = true;
    g_refCameraYaw = g_lastCameraYaw;
    g_refShownYaw = g_lastShownYaw;
    g_refShownPitch = g_lastShownPitch;
    g_haveRef = g_haveLast;
    g_levelRef = g_level;
    g_levelTaken = false;
    ++g_bodyBuild;
    g_bodyNoted = false;  // the build's main camera goes to the body lock (NoteBodyTurn)
    g_frameBodyHave = false;
    g_buildPoseNoted = false;
}

void NoteDrawnView(const float* rows, const double* eye, uint32_t uploads) {
    std::lock_guard lock(g_buildPoseMutex);
    if (!g_buildPoseSerial)
        return;
    const uint32_t newest = g_buildPoseSerial;
    const uint32_t oldest = newest >= kBuildPoses ? newest - kBuildPoses + 1 : 1;
    // The frame's first upload that fits a build, as it ever was ("nameby 0" names the picture by it): the oldest not shown yet that it fits (a still head's builds
    // all fit: pictures go in order); else a build shown already, drawn again.
    if (!g_drawnSerial) {
        for (uint32_t s = std::max(oldest, g_lastDrawnSerial + 1); s <= newest && !g_drawnSerial; ++s)
            if (const BuildPose& e = g_buildPoses[s % kBuildPoses]; e.serial == s && viewturn::RowsAlike(rows, e.rows))
                g_drawnSerial = s;
        for (uint32_t s = newest; s >= oldest && !g_drawnSerial; --s)
            if (const BuildPose& e = g_buildPoses[s % kBuildPoses]; e.serial == s && viewturn::RowsAlike(rows, e.rows))
                g_drawnSerial = s;
        if (g_drawnSerial)
            g_drawnNamed.store(true, std::memory_order_relaxed);
    }
    // Every upload by the builds whose rows it carries: one build's alone tells which cameras that pass was drawn with. So do builds alike that were all turned
    // with one pose (the game building its views twice between two Presents, or going on building them while no new pose is published): whichever of them the pass
    // was drawn with, the pose is the same, and it counts for the newest of them. Left out as telling nothing, the few uploads of an older build were all such a
    // picture had to be named by.
    uint32_t fits = 0, which = 0, pose = 0;
    bool onePose = true;
    for (uint32_t s = newest; s >= oldest; --s)
        if (const BuildPose& e = g_buildPoses[s % kBuildPoses]; e.serial == s && viewturn::RowsAlike(rows, e.rows)) {
            if (!fits) {
                which = s;
                pose = e.poseId;
            } else if (e.poseId != pose) {
                onePose = false;
            }
            ++fits;
        }
    const bool tells = fits == 1 || (fits > 1 && onePose);
    FrameViews& v = g_frameViews;
    uint8_t kind = kOrderStray;
    if (!InWorld(eye)) {
        kind = fits ? kOrderFirstPersonFits : kOrderFirstPerson;  // (the gun's and arms' own views: never the world's cameras, whatever their rows fit)
    } else if (!fits) {
        ++v.stray;  // (another view in the world: a reflection's, a light's, or a camera no build of the mod's turned)
    } else {
        if (!v.haveFirst) {
            v.haveFirst = true;
            std::memcpy(v.firstRows, rows, sizeof(v.firstRows));
            v.firstBuild = which;
        }
        v.haveLast = true;
        std::memcpy(v.lastRows, rows, sizeof(v.lastRows));
        v.lastBuild = which;
        v.all.Add(which);
        v.carriedBuild[which % kBuildPoses] = which;
        std::memcpy(v.carriedRows[which % kBuildPoses], rows, sizeof(v.carriedRows[0]));
        if (!tells) {
            ++v.unclear;  // (builds of several poses alike: the head stands still, and the count is told nothing)
            kind = kOrderUnclear;
        } else {
            v.naming.Add(which);
            kind = kOrderMain;
        }
    }
    if (!CountingViews())
        return;
    if (v.count < FrameViews::kMost) {
        FrameViews::Entry& e = v.order[v.count++];
        e.build = tells ? which : 0;
        e.along = static_cast<float>(eye[0] * rows[0] + eye[1] * rows[1] + eye[2] * rows[2]);
        e.kind = kind;
        e.thread = GetCurrentThreadId();
        e.at = uploads;
    } else {
        ++v.cut;
    }
}

bool DrawnNamed() {
    return g_drawnNamed.load(std::memory_order_relaxed);
}

void NoteEyePicture(int picture) {
    std::lock_guard lock(g_buildPoseMutex);
    FrameViews& v = g_frameViews;
    if (picture != v.naming.eyes || picture < 0 || picture > 1)
        return;  // (each of the two once a frame, in their order)
    v.naming.EyePicture();
    v.all.EyePicture();
    if (picture == 0 && v.haveLast) {
        v.haveEyes = true;
        std::memcpy(v.eyesRows, v.lastRows, sizeof(v.eyesRows));
        v.eyesBuild = v.lastBuild;
    }
    if (CountingViews() && v.count < FrameViews::kMost) {
        FrameViews::Entry& e = v.order[v.count++];
        e.kind = picture == 0 ? kOrderEyeFirst : kOrderEyeSecond;
        e.thread = GetCurrentThreadId();
    }
}

void SetNameBy(int rule) {
    rule = std::clamp(rule, 0, 2);
    static const char* const kRules[3] = {"its first main-view upload, as ever", "its last main-view upload before the first eye's picture goes out",
                                          "the view build most of its main-view uploads carry"};
    if (g_nameBy.exchange(rule) != rule)
        Log("Head poses: each picture named by rule %d: %s", rule, kRules[rule]);
}

int NameBy() {
    return g_nameBy.load(std::memory_order_relaxed);
}

bool DrawnWorldView(DrawnWorld& out) {
    std::lock_guard lock(g_buildPoseMutex);
    if (!g_haveDrawnWorld)
        return false;
    out = g_drawnWorld;
    return true;
}

bool TakeDrawnPose(uint32_t& poseId) {
    std::lock_guard lock(g_buildPoseMutex);
    const uint32_t byFirst = g_drawnSerial;
    // The picture's name: by its first upload, or ("nameby" 1, 2) by what all its main-view uploads carried (headtest.h Naming), each counted for the newest build
    // its rows fit; with no main-view upload at all, by the first upload's rule again.
    const int rule = g_nameBy.load(std::memory_order_relaxed);
    const uint32_t s = rule != 0 ? g_frameViews.all.Named(rule, byFirst) : byFirst;
    CloseFrameViews(g_buildPoseSerial, byFirst, s);
    g_drawnSerial = 0;
    g_handsSerial = 0;
    g_drawnNamed.store(false, std::memory_order_relaxed);
    if (!s || g_buildPoses[s % kBuildPoses].serial != s)
        return false;
    poseId = g_buildPoses[s % kBuildPoses].poseId;
    g_lastDrawnSerial = s;
    g_lastOffset = byFirst ? std::clamp(static_cast<int>(s) - static_cast<int>(byFirst), -3, 3) : 0;
    return poseId != 0;
}

bool NextDrawnPose(uint32_t& poseId) {
    std::lock_guard lock(g_buildPoseMutex);
    if (!g_lastDrawnSerial || g_buildPoseSerial <= g_lastDrawnSerial)
        return false;
    // (Far behind, nothing having been named for a while: the newest.)
    const uint32_t s = g_buildPoseSerial - g_lastDrawnSerial > 3 ? g_buildPoseSerial : g_lastDrawnSerial + 1;
    const BuildPose& e = g_buildPoses[s % kBuildPoses];
    if (e.serial != s || !e.poseId)
        return false;
    poseId = e.poseId;
    return true;
}

void SetPoseByDraw(bool on) {
    if (g_poseByDraw.exchange(on) != on)
        Log("Head poses: each picture handed over with %s", on ? "the pose it was drawn with" : "pose_lag's guess");
}

bool PoseByDraw() {
    return g_poseByDraw.load(std::memory_order_relaxed);
}

void SetPureRows(bool on) {
    if (g_pureRows.exchange(on) != on)
        Log("Camera: the picture's turn %s", on ? "made from the body's yaw and the head alone" : "taken from the game's camera");
}

void SetPoseTries(int which, float value) {
    static const char* const kNames[kPoseTryCount] = {"the head's turn low-passed at (Hz)", "the head's place low-passed at (Hz)", "the head and hands located ahead by (ms)",
                                                      "the layer's pose turned every other second by (degrees)", "the head's filtered turn carried on by (ms)", "the last picture held",
                                                      "the picture's edge carried outwards", "the filled image asked for", "the head's filter opened by its own speed",
                                                      "the head's turn carried by its own rate"};
    if (which < 0 || which >= kPoseTryCount)
        return;
    if (g_poseTries[which].exchange(value) != value)
        Log("Head poses: %s %.1f", kNames[which], value);
}

float PoseTry(int which) {
    return which >= 0 && which < kPoseTryCount ? g_poseTries[which].load(std::memory_order_relaxed) : 0.0f;
}

void SetHandsPose(uint32_t poseId) {
    g_handsPose.store(poseId, std::memory_order_relaxed);
}

bool HandsOffDrawn(float* out) {
    const uint32_t handsPose = g_handsPose.load(std::memory_order_relaxed);
    if (!handsPose || !g_drawnNamed.load(std::memory_order_relaxed))
        return false;
    std::lock_guard lock(g_buildPoseMutex);
    uint32_t s = g_drawnSerial;
    if (s && g_nameBy.load(std::memory_order_relaxed) != 0) {
        // "nameby" 1 or 2: the picture is named only once it is drawn. Its gun and arms go by the build as far from the one its first upload fits as the last
        // picture's name was from its own (no further than the newest there is), taken at their first draw of the frame and kept for the rest of it (two builds
        // in one picture's hands would part its depth from its colour).
        if (!g_handsSerial)
            g_handsSerial = static_cast<uint32_t>(std::clamp(static_cast<int>(s) + g_lastOffset, 1, static_cast<int>(g_buildPoseSerial)));
        s = g_handsSerial;
    }
    if (!s || g_buildPoses[s % kBuildPoses].serial != s)
        return false;
    const BuildPose& drawn = g_buildPoses[s % kBuildPoses];
    if (drawn.poseId == handsPose)
        return false;
    const BuildPose* assumed = nullptr;
    for (const BuildPose& e : g_buildPoses)
        if (e.serial && e.poseId == handsPose)
            assumed = &e;
    if (!assumed)
        return false;
    // inverse(drawn) x assumed: a place measured from the head as assumed, measured from the head as drawn.
    const float n[4] = {-drawn.head[0], -drawn.head[1], -drawn.head[2], drawn.head[3]};
    float q[4];
    viewturn::QuatMul(n, assumed->head, q);
    const float x = q[0], y = q[1], z = q[2], w = q[3];
    const float m[3][3] = {{1 - 2 * (y * y + z * z), 2 * (x * y - w * z), 2 * (x * z + w * y)},
                           {2 * (x * y + w * z), 1 - 2 * (x * x + z * z), 2 * (y * z - w * x)},
                           {2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y)}};
    const float between[3] = {assumed->place[0] - drawn.place[0], assumed->place[1] - drawn.place[1], assumed->place[2] - drawn.place[2]};
    float place[3];
    viewturn::QuatRotate(n, between, place);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            out[i * 4 + j] = m[i][j];
        out[i * 4 + 3] = place[i];
    }
    return true;
}

void SetHeadWave(float degrees, float seconds) {
    g_waveDegrees = degrees;
    g_waveSeconds = seconds > 0.1f ? seconds : 2.0f;
}

float HeadWaveYaw() {
    const float degrees = g_waveDegrees.load(std::memory_order_relaxed);
    if (degrees == 0.0f)
        return 0.0f;
    static LARGE_INTEGER s_frequency{};
    if (!s_frequency.QuadPart)
        QueryPerformanceFrequency(&s_frequency);
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    const double t = static_cast<double>(now.QuadPart) / static_cast<double>(s_frequency.QuadPart);
    return degrees * (kPi / 180.0f) * static_cast<float>(std::sin(2.0 * 3.14159265358979 * t / g_waveSeconds.load(std::memory_order_relaxed)));
}

bool FrameBody(float* right, float* back, bool& scripted, ViewTurn& turn) {
    if (!g_frameBodyHave)
        return false;
    for (int i = 0; i < 3; ++i) {
        right[i] = g_frameBodyRight[i];
        back[i] = g_frameBodyBack[i];
    }
    scripted = g_frameBodyScripted;
    turn = g_frameView.turn;
    return true;
}

void NoteBodyMove(const float* move) {
    {
        std::lock_guard lock(g_bodyMutex);
        BodyEntry& e = g_bodyEntries[(g_bodyNext - 1 + kBodyBuilds) % kBodyBuilds];
        for (int i = 0; i < 3; ++i)
            e.turn.move[i] = move[i];
    }
    // The head test's meters: the same for this build's own entry (DrawnCameraNamed), where its cameras really went.
    if (g_buildPoseNoted && g_drawnDetail.load(std::memory_order_relaxed)) {
        std::lock_guard lock(g_buildPoseMutex);
        BuildPose& p = g_buildPoses[g_buildPoseSerial % kBuildPoses];
        if (p.detail == p.serial)
            for (int i = 0; i < 3; ++i)
                p.move[i] = move[i];
    }
}

void SetDrawnDetail(bool on) {
    g_drawnDetail.store(on, std::memory_order_relaxed);
}

bool DrawnCameraNamed(DrawnCamera& out) {
    std::lock_guard lock(g_buildPoseMutex);
    const uint32_t s = g_lastDrawnSerial;
    const BuildPose& e = g_buildPoses[s % kBuildPoses];
    if (!s || e.serial != s || e.detail != s)
        return false;
    FillDrawn(e, out);
    return true;
}

bool DrawnCameraOfPose(uint32_t poseId, DrawnCamera& out) {
    if (!poseId)
        return false;
    std::lock_guard lock(g_buildPoseMutex);
    const BuildPose* newest = nullptr;
    for (const BuildPose& e : g_buildPoses)
        if (e.serial && e.detail == e.serial && e.poseId == poseId && (!newest || e.serial > newest->serial))
            newest = &e;
    if (!newest)
        return false;
    FillDrawn(*newest, out);
    return true;
}

int BodyTurns(BodyTurn* out, int most) {
    if (!BodyTurnWanted())
        return 0;
    std::lock_guard lock(g_bodyMutex);
    const ULONGLONG now = GetTickCount64();
    int n = 0;
    for (int k = 1; k <= kBodyBuilds && n < most; ++k) {
        const BodyEntry& e = g_bodyEntries[(g_bodyNext - k + kBodyBuilds) % kBodyBuilds];
        if (!e.valid || now - e.turn.at > kShownFresh)
            break;
        out[n++] = e.turn;
    }
    return n;
}

bool BodyTurnWanted() {
    return g_bodyNewestValid.load(std::memory_order_acquire) &&
           GetTickCount64() - g_bodyNewestAt.load(std::memory_order_relaxed) <= kShownFresh;
}

bool ShownTurn(float& cameraYaw, float& takenYaw) {
    const ULONGLONG at = g_shownAt.load(std::memory_order_acquire);
    if (!at || GetTickCount64() - at > kShownFresh)
        return false;
    const uint64_t v = g_shown.load(std::memory_order_acquire);
    const uint32_t low = static_cast<uint32_t>(v), high = static_cast<uint32_t>(v >> 32);
    std::memcpy(&cameraYaw, &low, sizeof(low));
    std::memcpy(&takenYaw, &high, sizeof(high));
    return std::isfinite(cameraYaw) && std::isfinite(takenYaw);
}

int TakeTurned(int* scripted) {
    const uint64_t both = g_turned.exchange(0, std::memory_order_relaxed);
    const int turned = static_cast<int>(both % kScriptedTurned);
    const int scriptedTurned = static_cast<int>(both / kScriptedTurned);
    if (scripted)
        *scripted = scriptedTurned;
    // While none are turned, once a second: why (the view turn off, or the matrix not a camera: then its
    // translation and its rows' lengths, the first refused one), and whether the views were built at all.
    static ULONGLONG s_logged = 0;
    const ULONGLONG now = GetTickCount64();
    const uint32_t inactive = g_refusedInactive.exchange(0), notCamera = g_refusedNotCamera.exchange(0);
    const float* m = g_refusedMatrix;
    auto row = [m](int r) { return std::sqrt(m[r * 4] * m[r * 4] + m[r * 4 + 1] * m[r * 4 + 1] + m[r * 4 + 2] * m[r * 4 + 2]); };
    // Its up (row 1) and true forward (-row 2) too: a cutscene's camera upside down shows up y below 0 (the
    // interrogation's QTE fight, "completely upside down", 2026-10-31 20:50).
    auto logRefused = [&] {
        Log("Camera: the first refused: rows %.3f %.3f %.3f, w %.3f %.3f %.3f, at (%.2f %.2f %.2f), up (%.2f %.2f %.2f) "
            "forward (%.2f %.2f %.2f)",
            row(0), row(1), row(2), m[3], m[7], m[11], m[12], m[13], m[14], m[4], m[5], m[6], -m[8], -m[9], -m[10]);
    };
    if (turned == 0 && now - s_logged >= 1000) {
        s_logged = now;
        Log("Camera: no camera turned this picture: %u refused with the view turn off, %u not a camera%s", inactive,
            notCamera, notCamera ? "" : " (none offered: the views not built?)");
        if (notCamera)
            logRefused();
    } else if (now - s_logged >= 1000 && g_unalikeBuilds.load(std::memory_order_relaxed) > 0) {
        // View builds with a scripted camera and another kind as well, once a second: the cameras BF3 drew and culled
        // a picture with weren't all turned alike (each is turned as it comes). Told by each build (BeginViews), not
        // by these counts: a picture's can take in builds on both sides of a hand-over from the player's camera.
        s_logged = now;
        const uint32_t builds = g_unalikeBuilds.exchange(0, std::memory_order_relaxed);
        const uint32_t kinds = g_unalikeKinds.load(std::memory_order_relaxed);
        Log("Camera: %u view build%s turned their cameras unalike (the last: %u scripted, %u the player's kind, %u "
            "refused, not a camera)",
            builds, builds == 1 ? "" : "s", (kinds >> 8) & 0xFF, kinds & 0xFF, (kinds >> 16) & 0xFF);
        if (notCamera)
            logRefused();
    }
    return turned;
}

void SetCutsceneTurn(bool on) {
    g_cutsceneTurn = on;
    Log("Camera: a scripted camera (a cutscene's or a ride's, its rows' w lanes filled) is %s (weapon.txt "
        "cutsceneturn %d)",
        on ? "turned to the head about its own yaw, the horizon level"
           : "left as the game made it: the picture held still in the room",
        on ? 1 : 0);
}

void SetMirrorFix(bool on) {
    if (g_mirrorFix.exchange(on) != on)
        Log("Camera: a mirrored camera's turn is %s", on ? "mirrored to match" : "left as any other");
}

void SetSceneFacing(bool on) {
    if (g_sceneFacing.exchange(on) != on)
        Log("Camera: a scripted scene's forward is shown %s (weapon.txt scenefacing %d)",
            on ? "where the head faced as the scene began" : "at the room's straight ahead, wherever the head is", on ? 1 : 0);
}

bool SceneFacing(unsigned number, float& yaw) {
    if (!g_sceneFacing.load(std::memory_order_relaxed))
        return false;
    const uint64_t v = g_sceneShown.load(std::memory_order_acquire);
    if (!v || static_cast<unsigned>(v >> 32) != number)
        return false;
    const uint32_t bits = static_cast<uint32_t>(v);
    std::memcpy(&yaw, &bits, sizeof(yaw));
    return std::isfinite(yaw);
}

void LogTurns(int seconds) {
    g_turnLogUntil.store(GetTickCount64() + 1000ull * seconds, std::memory_order_relaxed);
    Log("View: logging the view's turn each frame for %d s", seconds);
}

bool GameWeaponAngles(float& yaw, float& pitch) {
    if (GetTickCount64() - g_weaponYawAt.load(std::memory_order_relaxed) > 200)
        return false;
    yaw = g_weaponYaw.load(std::memory_order_relaxed);
    pitch = g_weaponPitch.load(std::memory_order_relaxed);
    return std::isfinite(yaw) && std::isfinite(pitch);
}

uint32_t TakeCameraBuilds() {
    return g_cameraBuilds.exchange(0);
}

bool ProjectionDepth(float& nearPlane, float& depthScale) {
    // Seen in game: rows (s 0 0 0) (0 s 0 0) (0 0 -1.00001 -1) (0 0 -0.06 0): depth = 1.00001 - 0.06/distance.
    uintptr_t render = g_slots[0].load();
    float m[16];
    if (!render || !SafeRead(render + kProjectionFromTransform, m, sizeof(m)))
        return false;
    float scale = -m[10], n = -m[14];
    if (!(scale > 0.9f && scale < 1.1f && n > 0.001f && n < 1.0f))
        return false;
    nearPlane = n;
    depthScale = scale;
    return true;
}

float SourceFov() {
    float fov = 0.0f;
    return g_fovSource && SafeRead(g_fovSource, &fov, sizeof(fov)) && LooksLikeFov(fov) ? fov : 0.0f;
}

float SourceFovRaw() {
    float fov = 0.0f;
    return g_fovSource && SafeRead(g_fovSource, &fov, sizeof(fov)) ? fov : 0.0f;
}

float RenderedFov() {
    // The render view's projection says what the last frame was drawn with. With the render view
    // not watched, nothing widens it: the game draws its own FOV.
    if (uintptr_t render = g_slots[0].load()) {
        float scale = 0.0f;
        if (SafeRead(render + kProjectionFromTransform, &scale, sizeof(scale)) && scale > 0.05f && scale < 20.0f)
            return 2.0f * std::atan(1.0f / scale);
    }
    return SourceFov();
}

void SetTestCopies(const std::vector<uintptr_t>& copies) {
    std::lock_guard lock(g_testMutex);
    g_testCopies = copies;
    g_huntIndex = -1;
}

void ToggleHunt() {
    if (g_huntIndex >= 0) {
        g_huntIndex = -1;
        Log("Cull hunt: stopped without saving");
        BeepAsync(440, 300);
        return;
    }
    size_t count;
    {
        std::lock_guard lock(g_testMutex);
        count = g_testCopies.size();
    }
    if (!count) {
        Log("Cull hunt: no camera copies yet. Press F10 first to run the camera finder.");
        BeepAsync(300, 700);
        return;
    }
    g_huntIndex = 0;
    g_huntStepTime = GetTickCount64();
    Log("Cull hunt: trying copy 1 of %zu", count);
    BeepAsync(1000, 80);
}

void UpdateHunt() {
    int index = g_huntIndex.load();
    if (index < 0 || GetTickCount64() - g_huntStepTime < kHuntStepMs)
        return;
    size_t count;
    {
        std::lock_guard lock(g_testMutex);
        count = g_testCopies.size();
    }
    g_huntStepTime = GetTickCount64();
    if (++index >= static_cast<int>(count)) {
        g_huntIndex = -1;
        Log("Cull hunt: tried all %zu copies without a save", count);
        BeepAsync(300, 700);
        return;
    }
    g_huntIndex = index;
    Log("Cull hunt: trying copy %d of %zu", index + 1, count);
    BeepAsync(1000, 80);
}

uintptr_t HuntCandidate() {
    int index = g_huntIndex.load();
    std::lock_guard lock(g_testMutex);
    return index >= 0 && index < static_cast<int>(g_testCopies.size()) ? g_testCopies[index] : 0;
}

void StopHunt() {
    g_huntIndex = -1;
}

}

#include "views.h"

#include <windows.h>
#include <MinHook.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>

#include "camera.h"
#include "headmove.h"
#include "skin.h"
#include "input.h"
#include "log.h"

namespace {

// The function that builds the frame's view descriptions (bf3.exe 1.6, not relocated). It takes
// the game's camera state, and fills a count and an array of descriptions 0x1100 bytes each: a
// name pointer at +4, the viewport at +0x10, then three camera blocks of 0x480 bytes, each starting
// with its transform: the main view +0x30, the one BF3 culls with +0x4B0, the first person +0x930.
// Within a camera block: FOV pair +0x48, aspect +0x58. The first-person views the player's own arms,
// gun and legs are drawn with aren't made from that block (both eyes' come from one camera, the
// weapon camera's; turned, the body still moved with the head): skin.cpp turns those as they're uploaded.
constexpr uintptr_t kBuildViews = 0x01018E70;
constexpr uint8_t kBuildViewsStart[] = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0, 0x81, 0xEC, 0x74, 0x0E, 0x00, 0x00};
constexpr size_t kViewSize = 0x1100;
constexpr size_t kMainCamera = 0x30, kCullCamera = 0x4B0, kFirstPersonCamera = 0x930, kCameraSize = 0x480;
constexpr size_t kFovPair = 0x48, kAspect = 0x58;
constexpr size_t kViewport = 0x10;  // x, y, width, height (int32 each)

// GameRenderSettings, as the view builder reads it: [[bf3.exe+1F84D78]+0x3C], recognised by its
// vtable; SplitScreenTestViewCount at +0x40 (BF3's type tables).
constexpr uintptr_t kSettingsOwner = 0x02384D78;
constexpr uintptr_t kSettingsVtable = 0x02138E50;
constexpr size_t kViewCount = 0x40;

using BuildViewsFn = void(__fastcall*)(void* self, void* edx, void* source, uint32_t* count, uint8_t* views);
BuildViewsFn g_buildViews = nullptr;
bool g_hooked = false;
std::atomic<bool> g_want{false}, g_active{false};
std::atomic<float> g_offset[2] = {0.0f, 0.0f};
std::atomic<float> g_fov{0.0f};  // what the eyes were last built with
std::atomic<int32_t> g_rightAt{0};  // where the game put the second view (x), before MakeEyes moved it
std::atomic<float> g_nearPlane{0.0f};  // weapon.txt "nearplane": metres, for a test; 0 the game's own (6 cm), which is what the eyes have
// weapon.txt "offcentre": how far each eye's window is moved outward and down (shares of its own width and height: two floats kept as one number, so both change together); 0 0 not
// at all.
std::atomic<uint64_t> g_offCentre{0};
// Each eye's window as last built with the mod's move in it (to the right and up: the game's own numbers), for what the headset is told; whether the mod moved it at all.
std::atomic<float> g_window[2][2] = {};
std::atomic<bool> g_windowMoved{false};

// The last build's cameras for the HUD's markers (views::LastCameras): written once a build, read once a frame.
SRWLOCK g_camerasLock = SRWLOCK_INIT;
views::BuiltCameras g_cameras;
bool g_haveCameras = false;
uint32_t g_builds = 0;

bool LooksLikeFov(float v) { return v > 0.2f && v < 2.8f; }

// weapon.txt "lodfov": the angle the eyes' level of detail is picked by, radians: the second number of a camera block's pair, which the scope's view also sets apart from its
// field of view. 0: the angle the picture is drawn with, as ever.
std::atomic<float> g_detailFov{0.0f};
// bf3vr.ini [stereo] detail_fov: the same, when weapon.txt has no "lodfov" line; radians, 0 the angle drawn.
std::atomic<float> g_detailBase{0.0f};

void WriteFov(uint8_t* camera, float fov) {
    auto f = reinterpret_cast<float*>(camera + kFovPair);
    if (fov > 0.0f && LooksLikeFov(f[0]) && LooksLikeFov(f[1])) {
        const float test = g_detailFov.load(std::memory_order_relaxed);
        const float detail = test > 0.0f ? test : g_detailBase.load(std::memory_order_relaxed);
        f[0] = fov;
        f[1] = detail > 0.0f ? detail : fov;
    }
}

// The window a camera block's projection is made through, within the block: its offset at +0x6C (two floats: shares of the window's own width and height, to the right and up; 0 0 by
// default) and its scale at +0x74 (1 1). The game's one projection builder (0x006BB7D0; for a perspective camera, kind 0 at +0x40) makes the frustum from them: with h = near x
// tan(FOV / 2) and w = aspect x h, left = -w + 2 w x offset.x, right = left + 2 w x scale.x, top = h + 2 h x offset.y, bottom = top - 2 h x scale.y. So the offset moves the window in
// its own plane and leaves its size alone: what is straight ahead comes out that share of the picture to the left of and below its middle. That holds while the byte at +0x44 is set (1
// by default: the picture's depth runs along the camera's back row; with it clear the same numbers would move the window the other way). The renderer's own copies of a view's main and
// cull cameras take the main camera's offset and a scale of 1 (0x017B9DD7), the cull slices the cull camera's own (0x01780F30), the views the gun and arms are drawn with the main
// camera's (0x010181B0). The word at +0x80 says what has to be worked out again: the game's own setter of the offset (0x006BBD60) sets the bits worth 1 and 8 in it.
constexpr size_t kKind = 0x40, kDepthAlongBack = 0x44, kWindowOffset = 0x6C, kStale = 0x80;
constexpr uint32_t kWindowStale = 9;

// Whether a camera block is one whose window the offset moves as read above: a perspective camera, depth along its back row; and the game's own offset for it a number inside the picture.
bool WindowMovable(const uint8_t* camera, const float* own) {
    uint32_t kind = 1;
    std::memcpy(&kind, camera + kKind, sizeof(kind));
    return kind == 0 && camera[kDepthAlongBack] != 0 && std::fabs(own[0]) < 0.5f && std::fabs(own[1]) < 0.5f;  // (also false for NaN)
}

// A camera block's window put at these shares of its width (to the right) and height (up), and the block marked as the game's own setter marks it.
void PlaceWindow(uint8_t* camera, float right, float up) {
    auto* offset = reinterpret_cast<float*>(camera + kWindowOffset);
    offset[0] = right;
    offset[1] = up;
    uint32_t stale = 0;
    std::memcpy(&stale, camera + kStale, sizeof(stale));
    stale |= kWindowStale;
    std::memcpy(camera + kStale, &stale, sizeof(stale));
}

// How far a camera block's picture reaches from its own forward, in degrees: to the left, to the right, up and down.
void WindowAngles(const uint8_t* camera, float out[4]) {
    const float h = std::tan(reinterpret_cast<const float*>(camera + kFovPair)[0] * 0.5f);
    const float w = h * reinterpret_cast<const float*>(camera + kAspect)[0];
    const auto* offset = reinterpret_cast<const float*>(camera + kWindowOffset);
    constexpr float kDegrees = 57.29578f;
    out[0] = std::atan(w * (1.0f - 2.0f * offset[0])) * kDegrees;
    out[1] = std::atan(w * (1.0f + 2.0f * offset[0])) * kDegrees;
    out[2] = std::atan(h * (1.0f + 2.0f * offset[1])) * kDegrees;
    out[3] = std::atan(h * (1.0f - 2.0f * offset[1])) * kDegrees;
}

// weapon.txt "offcentre" (a test): each eye's window moved in its own plane, outward and down, the game doing the rest: in the main and the cull camera of each eye (the renderer and
// the gun's views go by the main one, the cull slices by the cull one), and in the first-person camera too when it is of the same kind, to keep it in step. The left eye is the first
// view, shown in the picture's left half: its window goes left, the right eye's right, both down. The eyes' places, turns, angle and picture size stay as they are. With 0 0 (the
// default) nothing here is read or written. What the headset must then be told of each eye's picture is kept (OffCentre), and said in the log whenever it changes: a picture handed
// over as centred would show the world that share of the picture away from where it is.
// The move goes on top of the game's own offset (0 0 wherever its code was read, but it has setters of its own). That is what a camera of the first view holds as built, unless it
// still holds, to the bit, what the mod left there the build before: a view can come with the mod's own last numbers in it (its rows do, now and then), and added to again each build
// the window would run away. Then the game's own is the one last seen. The second view's cameras are copies of the first's as built, so they go by the same. When the line goes, or
// the cameras turn out not to be movable, a camera still holding the mod's numbers gets the game's own back.
void MoveWindows(uint8_t* const eye[2]) {
    struct Kept {
        float game[2] = {};  // the game's own offset, as last seen in this camera of the first view
        float left[2] = {};  // what the mod left there
        bool have = false;
    };
    static Kept s_kept[3];
    static constexpr size_t kCameras[3] = {kMainCamera, kCullCamera, kFirstPersonCamera};
    static int s_state = 0;  // as last said: 0 centred, 1 moved, 2 asked for but these cameras' windows can't be moved
    static float s_said[4] = {};
    static bool s_saidFirstPerson = false;
    float asked[2];
    const uint64_t packed = g_offCentre.load(std::memory_order_relaxed);
    std::memcpy(asked, &packed, sizeof(asked));
    const float outward = asked[0], down = asked[1];
    const bool wanted = outward != 0.0f || down != 0.0f;
    bool leftover[3] = {}, movable[3] = {};
    for (int k = 0; k < 3; ++k) {
        Kept& kept = s_kept[k];
        if (!wanted && !kept.have)
            continue;  // (the default: nothing looked at, nothing remembered)
        const auto* now = reinterpret_cast<const float*>(eye[0] + kCameras[k] + kWindowOffset);
        leftover[k] = kept.have && now[0] == kept.left[0] && now[1] == kept.left[1] && (kept.left[0] != kept.game[0] || kept.left[1] != kept.game[1]);
        if (!leftover[k])
            kept.game[0] = now[0], kept.game[1] = now[1];
        movable[k] = WindowMovable(eye[0] + kCameras[k], kept.game) && WindowMovable(eye[1] + kCameras[k], kept.game);
    }
    // The main and cull cameras must be of the kind read; the first-person ones are moved only if they are.
    const bool place = wanted && movable[0] && movable[1];
    for (int k = 0; k < 3; ++k) {
        Kept& kept = s_kept[k];
        if (place && movable[k]) {
            for (int e = 0; e < 2; ++e)
                PlaceWindow(eye[e] + kCameras[k], kept.game[0] + (e == 0 ? -outward : outward), kept.game[1] - down);
            const auto* left = reinterpret_cast<const float*>(eye[0] + kCameras[k] + kWindowOffset);
            kept.left[0] = left[0], kept.left[1] = left[1];
            kept.have = true;
        } else {
            if (leftover[k] && std::isfinite(kept.game[0]) && std::isfinite(kept.game[1]))
                for (int e = 0; e < 2; ++e)  // the mod's numbers are still in it and not wanted: the game's own back
                    PlaceWindow(eye[e] + kCameras[k], kept.game[0], kept.game[1]);
            kept.have = false;
        }
    }
    g_windowMoved.store(place, std::memory_order_relaxed);
    if (!wanted) {
        if (s_state != 0)
            Log("Views: each eye's picture drawn centred again (weapon.txt offcentre 0 0): nothing written to the cameras' windows");
        s_state = 0;
        return;
    }
    if (!place) {
        if (s_state != 2) {
            const int k = movable[0] ? 1 : 0;
            uint32_t kind = 0;
            std::memcpy(&kind, eye[0] + kCameras[k] + kKind, sizeof(kind));
            Log("Views: the eyes' pictures left centred (weapon.txt offcentre %.6f %.6f asked): the %s camera is not one whose window is known to move by its offset (kind %u, depth "
                "along its back row %u, the game's own offset %.4f %.4f)",
                outward, down, k == 0 ? "main" : "cull", kind, static_cast<unsigned>(eye[0][kCameras[k] + kDepthAlongBack]), s_kept[k].game[0], s_kept[k].game[1]);
        }
        s_state = 2;
        return;
    }
    for (int e = 0; e < 2; ++e) {
        const auto* now = reinterpret_cast<const float*>(eye[e] + kMainCamera + kWindowOffset);
        g_window[e][0].store(now[0], std::memory_order_relaxed);
        g_window[e][1].store(now[1], std::memory_order_relaxed);
    }
    const float* gameOwn = s_kept[0].game;
    const bool firstPerson = movable[2];
    if (s_state != 1 || s_said[0] != outward || s_said[1] != down || s_said[2] != gameOwn[0] || s_said[3] != gameOwn[1] || s_saidFirstPerson != firstPerson) {
        float left[4], right[4];
        WindowAngles(eye[0] + kMainCamera, left);
        WindowAngles(eye[1] + kMainCamera, right);
        const auto* main = reinterpret_cast<const float*>(eye[0] + kMainCamera);
        Log("Views: each eye's picture drawn off-centre (weapon.txt offcentre %.6f %.6f): its window moved that share of its width outward and of its height down, on the game's own "
            "offset of %.4f %.4f, in its main and cull cameras%s; the left eye's picture reaches %.2f degrees left, %.2f right, %.2f up, %.2f down, the right eye's %.2f, %.2f, %.2f, "
            "%.2f (from an angle of %.2f degrees, aspect %.4f); the headset is told each eye's own",
            outward, down, gameOwn[0], gameOwn[1], firstPerson ? " and its first-person camera" : " (not its first-person camera: another kind)", left[0], left[1], left[2], left[3],
            right[0], right[1], right[2], right[3], main[kFovPair / 4] * 57.29578f, main[kAspect / 4]);
        s_said[0] = outward, s_said[1] = down, s_said[2] = gameOwn[0], s_said[3] = gameOwn[1];
        s_saidFirstPerson = firstPerson;
    }
    s_state = 1;
}

// The two views made into the eyes: both start from the first view's cameras (the game's own),
// are turned to the head, and are shifted half the eye distance each way along the head's right.
// The first-person camera is turned only with a scripted main camera (a cutscene's or a ride's), by
// the same turn (camera.h TurnToHead), which also goes to skin.cpp: the views the player's own arms,
// gun and legs are drawn with are turned by it as they're uploaded (the user, 2026-11-01: "Only the
// head moves"). The first-person camera isn't what they're made from, but it's kept in step with them,
// as the cull camera is with the main one. In ordinary play the mod places the gun and arms itself.
// The second is also put at the window's left edge, where the first is (overlay.cpp moves its
// finished picture into the right half): placed at the right, its screen effects worked from that
// place against pictures only an eye wide, and while suppressed the whole right eye was smeared
// into streaks (the user's video; caught in both-eye pictures, 2026-10-30).
void MakeEyes(uint8_t* views) {
    uint8_t* eye[2] = {views, views + kViewSize};
    auto* place = reinterpret_cast<int32_t*>(eye[1] + kViewport);
    if (place[0] != 0) {
        g_rightAt.store(place[0], std::memory_order_relaxed);
        place[0] = 0;
    }
    const float* game = reinterpret_cast<const float*>(eye[0] + kMainCamera);
    input::NoteTurnStep(0, std::atan2(game[8], game[10]));  // the stick turn's evenness, as built
    // The game's own camera as built, for the HUD's markers (LastCameras): before it is turned and its angle written.
    views::BuiltCameras built;
    for (int r = 0; r < 3; ++r)
        for (int i = 0; i < 3; ++i)
            built.gameRows[3 * r + i] = game[4 * r + i];
    for (int i = 0; i < 3; ++i)
        built.gameEye[i] = game[12 + i];
    built.gameFov = game[kFovPair / 4];
    built.gameAspect = game[kAspect / 4];
    // The second view is the split-screen test's: the first one's camera turned round.
    std::memcpy(eye[1] + kMainCamera, eye[0] + kMainCamera, kCameraSize);
    std::memcpy(eye[1] + kCullCamera, eye[0] + kCullCamera, kCameraSize);
    std::memcpy(eye[1] + kFirstPersonCamera, eye[0] + kFirstPersonCamera, kCameraSize);
    const float fov = camera::WantedFov();
    const float gameEye[3] = {game[12], game[13], game[14]};
    const float gameBack[3] = {game[8], game[9], game[10]};  // (TurnToHead rewrites them)
    // Whether the rows as built are the game's own of this build: a build can come with the rows the mod left in this camera at the last one, the eye's place this build's all the same (now
    // and then with the headset on, every build in some launches without it). Such rows tell nothing of the yaw the game's eye stands on: headmove.h is told (the eye's arm).
    static float s_leftRows[9] = {};
    static bool s_haveLeft = false;
    float builtRows[9];
    for (int r = 0; r < 3; ++r)
        for (int i = 0; i < 3; ++i)
            builtRows[3 * r + i] = game[4 * r + i];
    const bool rowsStale = s_haveLeft && headmove::ArmRowsStale(builtRows, s_leftRows);
    float headMove[3] = {};
    bool headMoved = false;
    camera::BeginViews();  // one turn for both eyes and their cull cameras
    for (int e = 0; e < 2; ++e) {
        auto main = reinterpret_cast<float*>(eye[e] + kMainCamera);
        auto cull = reinterpret_cast<float*>(eye[e] + kCullCamera);
        auto firstPerson = reinterpret_cast<float*>(eye[e] + kFirstPersonCamera);
        camera::TurnToHead(main, firstPerson);
        camera::TurnToHead(cull);
        // The head's place in the room moves all of them alike, once a build (headmove.h): the eyes' turns are the same.
        if (e == 0)
            headMoved = headmove::Build(gameEye, gameBack, rowsStale, headMove);
        if (headMoved)
            for (float* m : {main, cull, firstPerson})
                for (int i = 0; i < 3; ++i)
                    m[12 + i] += headMove[i];
        // The eye's place: along the head's right (the turned main camera's first row).
        const float offset = g_offset[e].load(std::memory_order_relaxed);
        for (float* m : {main, cull, firstPerson})
            for (int i = 0; i < 3; ++i)
                m[12 + i] += main[i] * offset;
        for (float* m : {main, cull, firstPerson})
            WriteFov(reinterpret_cast<uint8_t*>(m), fov);
        // The first-person camera keeps the whole picture's aspect; its half is the main view's.
        reinterpret_cast<float*>(eye[e] + kFirstPersonCamera + kAspect)[0] =
            reinterpret_cast<float*>(eye[e] + kMainCamera + kAspect)[0];
    }
    // weapon.txt "offcentre": each eye's window moved outward and down (MoveWindows); without it nothing is written.
    MoveWindows(eye);
    // The head's camera as the eyes are built: their turn (the same in both), their midpoint, their angle.
    {
        const auto* left = reinterpret_cast<const float*>(eye[0] + kMainCamera);
        const auto* right = reinterpret_cast<const float*>(eye[1] + kMainCamera);
        for (int r = 0; r < 3; ++r)
            for (int i = 0; i < 3; ++i)
                built.headRows[3 * r + i] = left[4 * r + i];
        for (int i = 0; i < 3; ++i)
            built.headEye[i] = 0.5f * (left[12 + i] + right[12 + i]);
        built.eyeFov = left[kFovPair / 4];
        built.eyeAspect = left[kAspect / 4];
        built.nearPlane = left[0x50 / 4];
        built.farPlane = left[0x54 / 4];
        built.offCentre = g_windowMoved.load(std::memory_order_relaxed);
        built.at = GetTickCount64();
        AcquireSRWLockExclusive(&g_camerasLock);
        built.build = ++g_builds;
        g_cameras = built;
        g_haveCameras = true;
        ReleaseSRWLockExclusive(&g_camerasLock);
    }
    // The left eye's main rows as the mod leaves them: the next build's are told from these (above).
    for (int r = 0; r < 3; ++r)
        for (int i = 0; i < 3; ++i)
            s_leftRows[3 * r + i] = game[4 * r + i];
    s_haveLeft = true;
    // What the near and far planes are, once a level: everything nearer than the near plane has one depth.
    if (static bool s_said = false; !s_said) {
        s_said = true;
        const auto said = [&](const char* name, size_t camera) {
            const auto* plane = reinterpret_cast<const float*>(eye[0] + camera + 0x50);
            Log("Views: the %s camera's near plane %.3f, far %.1f", name, plane[0], plane[1]);
        };
        said("main", kMainCamera);
        said("cull", kCullCamera);
        said("first-person", kFirstPersonCamera);
    }
    // The game's own near plane (6 cm) unless weapon.txt "nearplane" moves it for a test. A nearer one was used for the eye
    // pressed up to a sight (everything nearer than the plane has one depth, drawn over or under by the order it's drawn in),
    // but the depth buffer has 24 bits: at 1 cm one step of it is 6 m at a kilometre, against 1 m at the game's 6 cm, and the
    // sun's glare then flashed on and off and far mountains flickered as the head moved. When it is moved: in all three
    // cameras, the game working everything out from them; skin.cpp puts the gun's own projection, made elsewhere, on the same one.
    if (const float wanted = g_nearPlane.load(std::memory_order_relaxed); wanted > 0.0f)
        for (const uint8_t* e : {eye[0], eye[1]})
            for (const size_t camera : {kMainCamera, kCullCamera, kFirstPersonCamera}) {
                auto* plane = reinterpret_cast<float*>(const_cast<uint8_t*>(e) + camera + 0x50);
                if (plane[0] > wanted && plane[0] < 1.0f)
                    plane[0] = wanted;
            }
    // Where the arms' space is, by the eye its views have: the game's, with the head's move only in a scripted picture
    // (skin.cpp LockBody moves them there). And the camera the world's picture is drawn from, for what's in the world and
    // goes with the gun in the hand.
    {
        float firstPersonEye[3];
        const bool carried = headMoved && camera::BodyTurnWanted();
        for (int i = 0; i < 3; ++i)
            firstPersonEye[i] = gameEye[i] + (carried ? headMove[i] : 0.0f);
        skin::NoteWorldEye(firstPersonEye);
        skin::NoteMainCamera(gameEye, reinterpret_cast<const float*>(eye[0] + kMainCamera),
                             reinterpret_cast<const float*>(eye[1] + kMainCamera));
    }
    // Read back: with two views the pointer chain to the render camera leads nowhere, and the
    // headset was told the game's own 55 degrees for a 100-degree picture (a small square, user).
    float drawn = reinterpret_cast<float*>(eye[0] + kMainCamera + kFovPair)[0];
    g_fov.store(LooksLikeFov(drawn) ? drawn : 0.0f, std::memory_order_relaxed);
}

// weapon.txt "viewdiff N": for N seconds, the two views' descriptions compared outside their
// camera blocks (which MakeEyes makes alike), logged when what differs changes (at most 4 times a
// second). The suppression blur stayed in the right eye with the HUD in both (2026-10-30): if the
// game keeps the screen's effects in a view's description, they differ here while suppressed.
std::atomic<ULONGLONG> g_diffUntil{0};

void LogDifferences(const uint8_t* views) {
    if (GetTickCount64() > g_diffUntil.load(std::memory_order_relaxed))
        return;
    static ULONGLONG lastLog = 0;
    static uint32_t last[kViewSize / 4][2] = {};
    static bool lastDiffers[kViewSize / 4] = {};
    if (GetTickCount64() - lastLog < 250)
        return;
    const auto* a = reinterpret_cast<const uint32_t*>(views);
    const auto* b = reinterpret_cast<const uint32_t*>(views + kViewSize);
    char line[900];
    int used = 0;
    bool changed = false;
    for (size_t i = 0; i < kViewSize / 4; ++i) {
        const size_t at = i * 4;
        if (at >= kMainCamera && at < kFirstPersonCamera + kCameraSize)
            continue;
        const bool differs = a[i] != b[i];
        if (differs != lastDiffers[i] || (differs && (a[i] != last[i][0] || b[i] != last[i][1])))
            changed = true;
        lastDiffers[i] = differs, last[i][0] = a[i], last[i][1] = b[i];
        if (differs && used < 820) {
            float fa, fb;
            std::memcpy(&fa, &a[i], 4), std::memcpy(&fb, &b[i], 4);
            used += snprintf(line + used, sizeof(line) - used, " +%03zX %08X|%08X (%g|%g)", at, a[i], b[i], fa, fb);
        }
    }
    if (!changed)
        return;
    lastLog = GetTickCount64();
    Log("Views: the eyes' descriptions differ at%s", used ? line : " nothing");
}

void __fastcall BuildViewsHook(void* self, void* edx, void* source, uint32_t* count, uint8_t* views) {
    g_buildViews(self, edx, source, count, views);
    bool eyes = g_want.load(std::memory_order_relaxed) && count && *count == 2 && views;
    if (eyes) {
        LogDifferences(views);
        MakeEyes(views);
    }
    // The renderer copies these views next; the camera breakpoints there leave them alone now.
    camera::SetViewsMode(eyes);
    g_active.store(eyes, std::memory_order_relaxed);
}

// The view builder hooked once its code is there (bf3.exe decrypts itself at start-up).
void Hook() {
    uint8_t start[sizeof(kBuildViewsStart)];
    if (!camera::SafeRead(kBuildViews, start, sizeof(start)) ||
        std::memcmp(start, kBuildViewsStart, sizeof(start)) != 0)
        return;
    void* target = reinterpret_cast<void*>(kBuildViews);
    if (MH_CreateHook(target, reinterpret_cast<void*>(&BuildViewsHook), reinterpret_cast<void**>(&g_buildViews)) !=
            MH_OK ||
        MH_EnableHook(target) != MH_OK) {
        Log("Views: could not hook the view builder");
        g_hooked = true;  // don't retry every frame
        g_buildViews = nullptr;
        return;
    }
    g_hooked = true;
    Log("Views: view builder hooked");
}

// Sets GameRenderSettings.SplitScreenTestViewCount, only where that object really is.
void SetViewCount(uint32_t count) {
    uintptr_t owner = 0, settings = 0, vtable = 0;
    if (!camera::SafeRead(kSettingsOwner, &owner, sizeof(owner)) || !owner ||
        !camera::SafeRead(owner + 0x3C, &settings, sizeof(settings)) || !settings ||
        !camera::SafeRead(settings, &vtable, sizeof(vtable)) || vtable != kSettingsVtable)
        return;
    auto field = reinterpret_cast<volatile uint32_t*>(settings + kViewCount);
    if (*field != count) {
        *field = count;
        Log("Views: %u view%s (GameRenderSettings at %08X)", count, count == 1 ? "" : "s",
            static_cast<unsigned>(settings));
    }
}

}

namespace views {

void Update(bool want) {
    if (!g_hooked)
        Hook();
    want = want && g_buildViews;
    g_want.store(want, std::memory_order_relaxed);
    static ULONGLONG lastSet = 0;
    static bool lastWant = false;
    ULONGLONG now = GetTickCount64();
    if (want != lastWant || now - lastSet >= 1000) {  // settings are rebuilt on level loads
        lastSet = now;
        lastWant = want;
        SetViewCount(want ? 2 : 1);
    }
    if (!want) {
        g_active.store(false, std::memory_order_relaxed);
        camera::SetViewsMode(false);
    }
}

bool Active() { return g_active.load(std::memory_order_relaxed); }

void SetEyeOffsets(float left, float right) {
    g_offset[0].store(left, std::memory_order_relaxed);
    g_offset[1].store(right, std::memory_order_relaxed);
}

float EyeOffset(int eye) { return eye == 0 || eye == 1 ? g_offset[eye].load(std::memory_order_relaxed) : 0.0f; }

float RenderedFov() { return Active() ? g_fov.load(std::memory_order_relaxed) : 0.0f; }

bool RightEyeAtLeft() { return Active() && g_rightAt.load(std::memory_order_relaxed) != 0; }

void SetNearPlane(float metres) { g_nearPlane.store(metres, std::memory_order_relaxed); }
float NearPlane() { return g_nearPlane.load(std::memory_order_relaxed); }

void SetDetailFov(float degrees) {
    const float kept = std::isfinite(degrees) && degrees >= 20.0f && degrees <= 150.0f ? degrees * 0.01745329f : 0.0f;
    if (g_detailFov.exchange(kept, std::memory_order_relaxed) != kept) {
        if (kept > 0.0f)
            Log("Views: the eyes' level of detail picked as for a picture %.2f degrees wide, however wide it is drawn (weapon.txt lodfov)", degrees);
        else
            Log("Views: the eyes' level of detail picked by the angle the picture is drawn with again (weapon.txt lodfov gone)");
    }
}

void SetDetailBase(float degrees) {
    const float kept = std::isfinite(degrees) && degrees >= 20.0f && degrees <= 150.0f ? degrees * 0.01745329f : 0.0f;
    if (g_detailBase.exchange(kept, std::memory_order_relaxed) != kept && kept > 0.0f)
        Log("Views: the eyes' level of detail picked as for a picture %.2f degrees wide, however wide it is drawn ([stereo] detail_fov)", degrees);
}

void SetOffCentre(float outward, float down) {
    const auto kept = [](float v) { return std::isfinite(v) ? std::clamp(v, -0.45f, 0.45f) : 0.0f; };
    const float both[2] = {kept(outward), kept(down)};
    uint64_t packed = 0;
    std::memcpy(&packed, both, sizeof(both));
    g_offCentre.store(packed, std::memory_order_relaxed);
}

bool OffCentre(int eye, float& right, float& up) {
    if ((eye != 0 && eye != 1) || !Active() || !g_windowMoved.load(std::memory_order_relaxed))
        return false;
    right = g_window[eye][0].load(std::memory_order_relaxed);
    up = g_window[eye][1].load(std::memory_order_relaxed);
    return true;
}

void WatchDifferences(int seconds) {
    g_diffUntil.store(GetTickCount64() + 1000ull * seconds, std::memory_order_relaxed);
    Log("Views: comparing the eyes' descriptions for %d s", seconds);
}

bool LastCameras(BuiltCameras& out) {
    AcquireSRWLockShared(&g_camerasLock);
    const bool have = g_haveCameras;
    if (have)
        out = g_cameras;
    ReleaseSRWLockShared(&g_camerasLock);
    return have && Active();
}

}

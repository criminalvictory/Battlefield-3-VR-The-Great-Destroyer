#include "reload.h"

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>

#include "ammo.h"
#include "effects.h"
#include "log.h"
#include "skin.h"
#include "sounds.h"

namespace {

// Where on the gun the left hand works it, in the gun's model space (x left, y up, z towards the
// muzzle; metres), from the mesh dump as the game poses it: the charging handle is bone 0 (the user
// saw it wiggle), its T at the back of the receiver's top (z 0.26, y 0.06); the magazine well is at
// z 0.38-0.47, and the bolt release sits on the left side just behind it.
constexpr float kHandle[3] = {0.0f, 0.08f, 0.27f};
// Where the palm point (where the arm folds into the glove) is with the glove's fingers on the
// handle: 13 cm behind it and 6 cm above (the user's twelve tries, 2026-10-30, all within 7 cm).
constexpr float kHandleGrip[3] = {0.035f, 0.14f, 0.135f};
constexpr float kHandleReach = 0.09f;    // the palm within this of the grip point grabs the handle
constexpr float kHandleReachGrip = 0.07f, kHandleLost = 0.25f;  // as near for the grip, which is also the shoulder's and the handguard's; gone this far off it, let go of: the glove isn't left glued there
constexpr float kBoltRelease[3] = {0.03f, 0.01f, 0.37f};
constexpr float kSlapReach = 0.10f;
constexpr float kSlapSpeed = 0.7f;       // m/s: slower is a touch, not a slap
// With the gun's shape: the glove touching the receiver's left side (x > 5 mm), this near the release, coming on at more than this.
constexpr float kSlapSide = 0.005f, kSlapOnRelease = 0.07f;
constexpr float kTouchSlapSpeed = 0.05f;  // m/s
constexpr float kPullTravel = 0.07f;     // as far back as the handle goes
constexpr float kPullToCharge = 0.045f;  // pulled at least this far and let go: a round chambered
constexpr double kSpringHome = 0.06;     // s: the handle let go, back home
// The ejection port's cover springs open on its hinge with each shot and snaps shut before the next
// (the user: it opens and shuts, it doesn't slide; then: faster, between shots). Shots come
// ~75 ms apart; open within a frame, held about a frame, shut within ~35 ms. It stays open while
// the bolt is locked back. With the charging handle it stays in step with the handle: as far open
// as the handle has come back, all the way at kCoverFullAt, so a slow pull opens it slowly (user,
// 2026-10-30: it popped open and shut "too quick"), and let go, it shuts as the spring takes the
// handle home (the user: then it should close at once).
constexpr float kCoverOpen = 2.8f;       // radians (160 degrees): out and down against the receiver
constexpr double kCoverSwing = 0.012;    // s to spring open
constexpr double kCoverHold = 0.018;     // s open after the bolt last moved
constexpr double kCoverShut = 0.035;     // s to swing shut
constexpr float kCoverFullAt = 0.04f;    // m of the handle's pull: the cover all the way open

bool g_boltBack = false;
bool g_grabbed = false;
float g_grabZ = 0.0f, g_pull = 0.0f, g_maxPull = 0.0f, g_lastTrigger = 0.0f, g_lastGrip = 0.0f;
double g_releasedAt = -1.0;
float g_lastPalm[3] = {};
bool g_havePalm = false;
bool g_wasOnRelease = false;  // the glove on the release's part of the gun last frame
double g_lastTime = 0.0;
int g_lastLoaded = -1;
reload::Cover g_rifleCover;
std::atomic<float> g_testPull{-1.0f};  // weapon.txt "pull 30": the handle drawn this far (m), a test
std::atomic<float> g_handleBack{0.0f}, g_coverAngle{0.0f};  // what DrawGun draws
bool g_chargeSounded = false;  // this pull of the handle has made its sound

double Now() {
    static LARGE_INTEGER frequency = [] {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        return f;
    }();
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return static_cast<double>(now.QuadPart) / static_cast<double>(frequency.QuadPart);
}

// A point in the view's space into the gun's model space (the pose's rows: rotation | translation).
void ToGun(const float* pose, const float* p, float* out) {
    const float d[3] = {p[0] - pose[3], p[1] - pose[7], p[2] - pose[11]};
    for (int i = 0; i < 3; ++i)
        out[i] = pose[i] * d[0] + pose[4 + i] * d[1] + pose[8 + i] * d[2];
}

float Distance(const float* a, const float* b) {
    const float d[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    return std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
}

// True when a round went in and the bolt is home.
bool Chamber(const char* how) {
    if (!g_boltBack)
        return false;
    // With the magazine out or empty there's nothing to chamber: it stays back for a full one (the
    // one round the gun keeps never fires, so one round left is empty).
    int loaded = 0, spare = 0;
    if (skin::MagazineOut() || !ammo::Get(loaded, spare) || loaded <= 1) {
        Log("Reload: %s with no rounds to chamber", how);
        return false;
    }
    g_boltBack = false;
    Log("Reload: %s: a round is chambered", how);
    return true;
}

}

namespace reload {

float StepCover(Cover& cover, float dt, bool boltBack, float handle, bool quick) {
    // Snapped open by a shot or the bolt locking back, then shut; with the handle, as far open as the
    // handle is back: pulled, eased forward by hand, or sprung home.
    const double now = Now();
    const ULONGLONG shot = effects::LastShot();
    const bool newShot = shot != cover.shotSeen;
    cover.shotSeen = shot;
    if (newShot || boltBack)
        cover.boltMovedAt = now;
    // Quick: a cover that swings a long way (the M16's and the M4's, half a turn and more): open for the frame of the shot, shut again by two frames on. At the M416's pace, on the M16's 14 shots a second, it hung open four frames in five and looked as if it never moved (the user, 2026-11-05: "the dust cover is supposed to open back and forth").
    const double hold = quick ? 0.008 : kCoverHold, shutIn = quick ? 0.024 : kCoverShut;
    const bool snapped = cover.boltMovedAt >= 0.0 && (now - cover.boltMovedAt < hold || (quick && boltBack));
    const float drawn = std::clamp(handle / kCoverFullAt, 0.0f, 1.0f);
    if (snapped) {
        cover.open = std::min(cover.open + dt / static_cast<float>(kCoverSwing), 1.0f);
        cover.byHandle = false;
    } else if (drawn > 0.0f || cover.byHandle) {
        cover.open = drawn;
        cover.byHandle = drawn > 0.0f;
    } else {
        cover.open = std::max(cover.open - dt / static_cast<float>(shutIn), 0.0f);
    }
    // Snapped by a shot: fast at first, easing into place. With the handle: the angle as the pull.
    return cover.byHandle ? cover.open : cover.open * (2.0f - cover.open);
}

bool Slapped(const float* palm, const float* lastPalm, float dt, const float* touch, bool& wasOnRelease, const float* release, bool hasShape) {
    // The glove really on the release: its place on the receiver's left side, near enough. Not a hand going by (the user, 2026-11-05: "it activates when I'm hovering it... activate it with me actually pressing the bolt reset"); and not only a touch that begins there from off the gun: a hand coming along the magazine and up onto it counts as it arrives.
    const bool onRelease = touch && touch[0] > kSlapSide && Distance(touch, release) < kSlapOnRelease;
    const bool fresh = onRelease && !wasOnRelease;
    wasOnRelease = onRelease;
    if (dt <= 0.0f)
        return false;
    const float speed = Distance(palm, lastPalm) / dt;
    if (hasShape)
        return fresh && speed > kTouchSlapSpeed;
    // A gun whose shape isn't there: a quick hand right by it.
    return speed > kSlapSpeed && Distance(palm, release) < kSlapReach;
}

bool NearHandle(const float* gunPose, const float* leftPalm, bool byGrip) {
    if (!gunPose || !leftPalm)
        return false;
    float palm[3];
    ToGun(gunPose, leftPalm, palm);
    return Distance(palm, kHandleGrip) < (byGrip ? kHandleReachGrip : kHandleReach);
}

void Update(const float* gunPose, const float* leftPalm, float grip, float trigger, const float* touch, bool hasShape) {
    const double now = Now();
    const float dt = g_lastTime > 0.0 ? static_cast<float>(now - g_lastTime) : 0.0f;
    g_lastTime = now;

    // Run dry: the last round fired, the bolt locks back. (The gun keeps one round the game never
    // fires, or it would reload by itself: one round left is empty.)
    int loaded = 0, spare = 0;
    if (skin::RifleInHand() && ammo::Get(loaded, spare)) {
        if (g_lastLoaded > 1 && loaded <= 1 && !skin::MagazineOut() && !g_boltBack) {
            g_boltBack = true;
            Log("Reload: the magazine ran dry; the bolt is locked back");
        }
        g_lastLoaded = loaded;
    }

    float palm[3] = {};
    const bool have = gunPose && leftPalm;
    if (have)
        ToGun(gunPose, leftPalm, palm);

    // The charging handle: the left trigger pressed near it grabs it (the user's choice, 2026-10-30;
    // not a trigger already held as the hand comes along), or the grip nearer it (the user, 2026-11-05: "embed both"); each press counts by itself, a grip already held takes nothing from the trigger. Back with the hand, both let go.
    const bool closingTrigger = trigger > 0.6f && g_lastTrigger <= 0.6f, closingGrip = grip > 0.6f && g_lastGrip <= 0.6f;
    if ((closingTrigger || closingGrip) && have)
        Log("Reload: left %s pressed, the palm %.0f mm from the charging handle's grip point (%.3f %.3f %.3f "
            "on the gun)",
            closingTrigger ? "trigger" : "grip", Distance(palm, kHandleGrip) * 1000.0f, palm[0], palm[1], palm[2]);
    if (have && !g_grabbed && ((closingTrigger && Distance(palm, kHandleGrip) < kHandleReach) || (closingGrip && Distance(palm, kHandleGrip) < kHandleReachGrip))) {
        g_grabbed = true;
        g_grabZ = palm[2];
        g_pull = g_maxPull = 0.0f;
        g_releasedAt = -1.0;
        g_chargeSounded = false;
        Log("Reload: charging handle grabbed");
    }
    if (g_grabbed) {
        const float handleHere[3] = {kHandleGrip[0], kHandleGrip[1], g_grabZ - g_pull};
        if (!have || std::max(grip, trigger) < 0.4f || Distance(palm, handleHere) > kHandleLost) {
            g_grabbed = false;
            g_releasedAt = now;
            if (g_maxPull >= kPullToCharge) {
                Chamber("charging handle pulled");
                if (!g_boltBack)  // the bolt goes home (empty, it stays caught back)
                    sounds::Play("bolt");
            }
            Log("Reload: charging handle let go, pulled %.0f mm", g_maxPull * 1000.0f);
        } else {
            g_pull = std::clamp(g_grabZ - palm[2], 0.0f, kPullTravel);
            g_maxPull = std::max(g_maxPull, g_pull);
            if (g_pull > 0.02f && !g_chargeSounded) {
                g_chargeSounded = true;
                sounds::Play("lever");  // (none yet: the game's M416 never pulls its handle, and its data has no sound of it)
            }
        }
    }
    float handle = g_grabbed ? g_pull : 0.0f;
    if (!g_grabbed && g_releasedAt >= 0.0) {
        const double t = (now - g_releasedAt) / kSpringHome;
        if (t < 1.0)
            handle = g_pull * static_cast<float>(1.0 - t);
        else
            g_releasedAt = -1.0, g_pull = 0.0f;
    }
    if (const float test = g_testPull.load(std::memory_order_relaxed); test >= 0.0f)
        handle = std::min(test, kPullTravel);
    g_handleBack = handle;

    g_coverAngle = kCoverOpen * StepCover(g_rifleCover, dt, g_boltBack, handle);

    // The bolt release: a slap at the left side of the receiver, behind the magazine well. The glove
    // touching the gun there as the hand comes in (the gun's shape, gunshape.cpp: the user saw the
    // hand go through the gun and the slap not always count), or, without the shape, a quick hand
    // near it.
    {
        // (Kept up to date all the time, or a hand already resting there as the bolt locks back would count as coming to it.)
        const bool slapped = Slapped(palm, g_lastPalm, have && g_havePalm ? dt : 0.0f, touch, g_wasOnRelease, kBoltRelease, hasShape);
        if (slapped && !g_grabbed && g_boltBack && Chamber(touch ? "bolt release slapped (the glove touched it)" : "bolt release slapped"))
            sounds::Play("bolt");
    }
    if (have)
        std::memcpy(g_lastPalm, palm, sizeof(palm));
    g_havePalm = have;
    g_lastTrigger = trigger;
    g_lastGrip = grip;
}

bool BoltBack() {
    return g_boltBack;
}

bool HandleHeld() {
    return g_grabbed;
}

void ReleaseBolt() {
    if (g_boltBack)
        Log("Reload: the game's own reload chambers a round");
    g_boltBack = false;
}

float HandleBack() {
    return g_handleBack.load(std::memory_order_relaxed);
}

float CoverAngle() {
    return g_coverAngle.load(std::memory_order_relaxed);
}

void TestPull(float metres) {
    if (g_testPull.exchange(metres) != metres)
        Log("Reload: %s", metres >= 0.0f ? "the charging handle held drawn (weapon.txt pull)" : "no test pull");
}

}

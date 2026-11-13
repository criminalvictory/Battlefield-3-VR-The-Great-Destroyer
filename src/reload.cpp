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

// Where on the gun a hand works it, in the gun's model space (x left, y up, z towards the
// muzzle; metres), from the mesh dump as the game poses it: the charging handle is bone 0 (the user
// saw it wiggle), its T at the back of the receiver's top (z 0.26, y 0.06); the magazine well is at
// z 0.38-0.47, and the bolt release sits on the left side just behind it (reload.h's kBoltReleaseOnGun:
// the gun is never mirrored, so it's there on its left for either hand).
constexpr float kHandle[3] = {0.0f, 0.08f, 0.27f};
// Where the palm point (where the arm folds into the glove) is with the glove's fingers on the
// handle: 13 cm behind it and 6 cm above (the user's twelve tries, 2026-10-30, all within 7 cm), is
// reload.h's kHandleGripOnGun: the left's, and the right's mirrored about the T on x 0.
constexpr float kHandleReach = 0.09f;    // the palm within this of the grip point grabs the handle
constexpr float kHandleReachGrip = 0.07f, kHandleLost = 0.25f;  // as near for the grip, which is also the shoulder's and the handguard's; gone this far off it, let go of: the glove isn't left glued there
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
const char* const kSideName[2] = {"left", "right"};  // each hand in the log, by side
// A slap's chamber in the log, by side: without the gun's shape, and with the glove touching the release.
const char* const kSlappedHow[2][2] = {{"bolt release slapped", "bolt release slapped (the glove touched it)"},
                                       {"bolt release slapped by the right hand", "bolt release slapped by the right hand (the glove touched it)"}};
const reload::Hand kNoHands[2] = {};

// The gun's half, one of each: the bolt, the cover, the handle and its spring home.
bool g_boltBack = false;
int g_grabbedBy = -1;  // the hand that has the charging handle: 0 the left, 1 the right, -1 none
float g_grabZ = 0.0f, g_pull = 0.0f, g_maxPull = 0.0f;
double g_releasedAt = -1.0;
double g_lastTime = 0.0;
int g_lastLoaded = -1;
reload::Cover g_rifleCover;
std::atomic<float> g_testPull{-1.0f};  // weapon.txt "pull 30": the handle drawn this far (m), a test
std::atomic<float> g_handleBack{0.0f}, g_coverAngle{0.0f};  // what DrawGun draws
bool g_chargeSounded = false;  // this pull of the handle has made its sound
std::atomic<double> g_testClock{-1.0};  // tools/reloadtest.cpp's clock (s): Now() reads it while it's 0 or more
// Each hand's half, by side ([0] the left, [1] the right): its presses, its palm and its slap.
float g_lastTriggerBy[2] = {}, g_lastGripBy[2] = {};
float g_lastPalmBy[2][3] = {};
bool g_havePalmBy[2] = {};
bool g_wasOnReleaseBy[2] = {};  // the glove on the release's part of the gun last frame

double Now() {
    // A test's clock while it gives one (tools/reloadtest.cpp, at 72 Hz).
    if (const double test = g_testClock.load(std::memory_order_relaxed); test >= 0.0)
        return test;
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

bool Finite3(const float* p) {
    return std::isfinite(p[0]) && std::isfinite(p[1]) && std::isfinite(p[2]);
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

bool NearHandle(const float* gunPose, const float* palm, bool byGrip, int side) {
    if (!gunPose || !palm)
        return false;
    float onGun[3];
    ToGun(gunPose, palm, onGun);
    return Distance(onGun, kHandleGripOnGun[side == 1 ? 1 : 0]) < (byGrip ? kHandleReachGrip : kHandleReach);
}

void UpdateHands(const float* gunPose, const Hand hands[2], bool hasShape) {
    const double now = Now();
    const float dt = g_lastTime > 0.0 ? static_cast<float>(now - g_lastTime) : 0.0f;
    g_lastTime = now;

    // Run dry: the last round fired, the bolt locks back. (The gun keeps one round the game never
    // fires, or it would reload by itself: one round left is empty.) Not with a held gun drawn since: the rifle is still "in hand" 30 frames after its
    // last draw, and the count in hand is then the held gun's (a gun taken up from the ground has a slot of its own).
    int loaded = 0, spare = 0;
    if (skin::RifleInHand() && !skin::HeldGunInHand() && ammo::Get(loaded, spare)) {
        if (g_lastLoaded > 1 && loaded <= 1 && !skin::MagazineOut() && !g_boltBack) {
            g_boltBack = true;
            Log("Reload: the magazine ran dry; the bolt is locked back");
        }
        g_lastLoaded = loaded;
    }

    // Each hand's palm in the gun's space (one that isn't finite there is taken as not given), its own controller's presses (a
    // number not finite: not pressed) and where its glove touches the gun (not finite: nowhere).
    if (!hands)
        hands = kNoHands;
    float palm[2][3] = {};
    bool have[2] = {};
    float grip[2], trigger[2];
    const float* touch[2];
    for (int s = 0; s < 2; ++s) {
        if (gunPose && hands[s].palm) {
            ToGun(gunPose, hands[s].palm, palm[s]);
            have[s] = Finite3(palm[s]);
            if (!have[s])
                palm[s][0] = palm[s][1] = palm[s][2] = 0.0f;
        }
        grip[s] = std::isfinite(hands[s].grip) ? hands[s].grip : 0.0f;
        trigger[s] = std::isfinite(hands[s].trigger) ? hands[s].trigger : 0.0f;
        touch[s] = hands[s].touch && Finite3(hands[s].touch) ? hands[s].touch : nullptr;
    }

    // The charging handle: a hand's trigger pressed near it grabs it (the user's choice, 2026-10-30;
    // not a trigger already held as the hand comes along), or its grip nearer it (the user, 2026-11-05: "embed both"); each press counts by itself, a grip already held takes nothing from the trigger. Back with the hand, both let go.
    // Either hand, each by its own presses and its own grip point on the T (the right's the left's mirrored); one hand has it at a time, the left asked first.
    bool closingTrigger[2], closingGrip[2];
    for (int s = 0; s < 2; ++s) {
        closingTrigger[s] = trigger[s] > 0.6f && g_lastTriggerBy[s] <= 0.6f;
        closingGrip[s] = grip[s] > 0.6f && g_lastGripBy[s] <= 0.6f;
        if ((closingTrigger[s] || closingGrip[s]) && have[s])
            Log("Reload: %s %s pressed, the palm %.0f mm from the charging handle's grip point (%.3f %.3f %.3f "
                "on the gun)",
                kSideName[s], closingTrigger[s] ? "trigger" : "grip", Distance(palm[s], kHandleGripOnGun[s]) * 1000.0f, palm[s][0], palm[s][1], palm[s][2]);
    }
    for (int s = 0; s < 2 && g_grabbedBy < 0; ++s) {
        if (have[s] && ((closingTrigger[s] && Distance(palm[s], kHandleGripOnGun[s]) < kHandleReach) || (closingGrip[s] && Distance(palm[s], kHandleGripOnGun[s]) < kHandleReachGrip))) {
            g_grabbedBy = s;
            g_grabZ = palm[s][2];
            g_pull = g_maxPull = 0.0f;
            g_releasedAt = -1.0;
            g_chargeSounded = false;
            Log("Reload: charging handle grabbed%s", s == 0 ? "" : " by the right hand");
        }
    }
    if (g_grabbedBy >= 0) {
        // The hand that has it, by its own palm and presses: its palm withdrawn (not tracked, busy, or no longer the hand at work) lets go too.
        const int g = g_grabbedBy;
        const float handleHere[3] = {kHandleGripOnGun[g][0], kHandleGripOnGun[g][1], g_grabZ - g_pull};
        if (!have[g] || std::max(grip[g], trigger[g]) < 0.4f || Distance(palm[g], handleHere) > kHandleLost) {
            g_grabbedBy = -1;
            g_releasedAt = now;
            if (g_maxPull >= kPullToCharge) {
                Chamber(g == 0 ? "charging handle pulled" : "charging handle pulled by the right hand");
                if (!g_boltBack)  // the bolt goes home (empty, it stays caught back)
                    sounds::Play("bolt");
            }
            Log("Reload: charging handle let go%s, pulled %.0f mm", g == 0 ? "" : " by the right hand", g_maxPull * 1000.0f);
        } else {
            g_pull = std::clamp(g_grabZ - palm[g][2], 0.0f, kPullTravel);
            g_maxPull = std::max(g_maxPull, g_pull);
            if (g_pull > 0.02f && !g_chargeSounded) {
                g_chargeSounded = true;
                sounds::Play("lever");  // (none yet: the game's M416 never pulls its handle, and its data has no sound of it)
            }
        }
    }
    float handle = g_grabbedBy >= 0 ? g_pull : 0.0f;
    if (g_grabbedBy < 0 && g_releasedAt >= 0.0) {
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
    // near it. Either hand's, each with its own palm and touch, the left's first: once one has
    // chambered a round, the other finds the bolt home.
    for (int s = 0; s < 2; ++s) {
        // (Kept up to date all the time, or a hand already resting there as the bolt locks back would count as coming to it.)
        const bool slapped = Slapped(palm[s], g_lastPalmBy[s], have[s] && g_havePalmBy[s] ? dt : 0.0f, touch[s], g_wasOnReleaseBy[s], kBoltReleaseOnGun, hasShape);
        if (slapped && g_grabbedBy < 0 && g_boltBack && Chamber(kSlappedHow[s][touch[s] ? 1 : 0]))
            sounds::Play("bolt");
    }
    for (int s = 0; s < 2; ++s) {
        if (have[s])
            std::memcpy(g_lastPalmBy[s], palm[s], sizeof(palm[s]));
        g_havePalmBy[s] = have[s];
        g_lastTriggerBy[s] = trigger[s];
        g_lastGripBy[s] = grip[s];
    }
}

void Update(const float* gunPose, const float* leftPalm, float grip, float trigger, const float* touch, bool hasShape) {
    // The left alone, as before either hand could.
    const Hand hands[2] = {{leftPalm, grip, trigger, touch}, {}};
    UpdateHands(gunPose, hands, hasShape);
}

bool BoltBack() {
    return g_boltBack;
}

bool HandleHeld() {
    return g_grabbedBy >= 0;
}

int HandleSide() {
    return g_grabbedBy;
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

void TestLockBack() {
    // As if run dry with a magazine in: a slap or a pull chambers the next round.
    if (g_boltBack)
        return;
    g_boltBack = true;
    Log("Reload: the bolt locked back (a test: a free hands film round)");
}

void TestClock(double seconds) {
    g_testClock.store(std::isfinite(seconds) ? seconds : -1.0, std::memory_order_relaxed);
}

}

// Motion controllers through OpenXR actions. The right controller's aim pose steers the
// player's look angles (see aim.cpp); its buttons and sticks are injected into BF3 as the
// keyboard and mouse input the game already understands.

#include "input.h"
#include "launchreport.h"

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "ammo.h"
#include "config.h"
#include "ejectkey.h"
#include "freetune.h"
#include "god.h"
#include "hands.h"
#include "inputrules.h"
#include "m320.h"
#include "headmove.h"
#include "log.h"
#include "pad.h"
#include "pickups.h"
#include "pouch.h"
#include "qte.h"
#include "reload.h"
#include "skin.h"
#include "sounds.h"
#include "tracking.h"
#include "window.h"

namespace {

XrInstance g_instance = XR_NULL_HANDLE;
XrActionSet g_set = XR_NULL_HANDLE;
XrAction g_aim = XR_NULL_HANDLE;
XrAction g_fire = XR_NULL_HANDLE;
XrAction g_ads = XR_NULL_HANDLE;
// The game's grenade (G). Not bound to anything for now: it was on the right grip, which threw a
// grenade when squeezed while holding the gun; the right grip is for holding the gun (user,
// 2026-10-31). A grenade gets its own gesture later.
XrAction g_grenade = XR_NULL_HANDLE;
XrAction g_move = XR_NULL_HANDLE;
XrAction g_turn = XR_NULL_HANDLE;
XrAction g_jump = XR_NULL_HANDLE;
XrAction g_crouch = XR_NULL_HANDLE;
XrAction g_reload = XR_NULL_HANDLE;
XrAction g_selectLauncher = XR_NULL_HANDLE;  // key 4: the launcher (slot 5) on the rifle mission
XrAction g_selectRifle = XR_NULL_HANDLE;     // key 1: the primary
XrAction g_selectSwap = XR_NULL_HANDLE;      // Touch has one free button: it goes between the two
// The Frame's left D-pad right: X with the gun in the left hand by its pistol grip (ejectkey.h). Bound only on the Frame (a try that
// takes it: g_ejectBound); anywhere else it reads false.
XrAction g_ejectLeft = XR_NULL_HANDLE;
bool g_ejectBound = false;
XrAction g_use = XR_NULL_HANDLE;
XrAction g_sprint = XR_NULL_HANDLE;
XrSpace g_aimSpace = XR_NULL_HANDLE;
// Both hands' grip poses (left 0, right 1), for drawing the hands where the controllers are.
XrAction g_hand = XR_NULL_HANDLE;
// The left grip, squeezed: the left glove's middle, ring and little fingers curl (its trigger, aim
// down sights, curls the index finger). The Steam Frame's left View button: lines the left glove
// up with the hand.
XrAction g_squeeze = XR_NULL_HANDLE;
XrAction g_calibrate = XR_NULL_HANDLE;
// The left thumb resting on the stick: the glove's thumb folds in.
XrAction g_thumbTouch = XR_NULL_HANDLE;
// Free hands: the right grip, squeezed, holds the gun (let go, the gun stays only in a hand on its handguard, else it falls), and with
// the right thumb on its stick they move the free right glove's fingers, as the left's do the left's. Actions of their own: g_squeeze
// stays the left's alone (one action on both hands, read without a hand, would give the left the right's squeeze). Null if a runtime
// wouldn't make them: then nothing's bound to them and the gun never leaves the right hand.
XrAction g_squeezeRight = XR_NULL_HANDLE;
XrAction g_thumbTouchRight = XR_NULL_HANDLE;
// The buzz in each hand (Buzz: recoil's pulse a shot): an output action for each, bound to that hand's haptic in every profile that takes
// it (Suggest tries each profile with them first, then as it always was without). Null if the runtime wouldn't make them: nothing buzzes.
// The session they go through (Init); the profiles whose bindings took them; whether each hand's profile in use (Sync) is one of them;
// and whether the last sync worked (the session focused: no buzz otherwise).
XrAction g_hapticLeft = XR_NULL_HANDLE, g_hapticRight = XR_NULL_HANDLE;
XrSession g_session = XR_NULL_HANDLE;
constexpr int kBuzzProfilesMost = 8;
XrPath g_buzzProfiles[kBuzzProfilesMost] = {};
int g_buzzProfileCount = 0;
bool g_buzzBound[2] = {};
bool g_focused = false;
// The right stick's click held kRecenterHold recenters the view, as F9 does (the user, 2026-11-01: "if we click it
// down and hold it ... 2 or 3 seconds, that should recenter the frame"). The click does nothing else. 2.5 s felt "way too long" (the user, 2026-11-04: "maybe 2 seconds"); a moment's
// slip of the thumb off the click no longer starts the wait again (kRecenterSlip).
XrAction g_recenter = XR_NULL_HANDLE;
constexpr ULONGLONG kRecenterHold = 1500, kRecenterSlip = 200;  // ms
ULONGLONG g_recenterSince = 0, g_recenterHeldAt = 0;
bool g_recenterDone = false;
std::atomic<bool> g_recenterWanted{false};
// A short click of the same is a jump (the user, 2026-11-05: "let's also make the right controller analog stick when you click it down. Let that be the jump button"; their
// design that morning: a click let go quickly jumps, held it recenters with no jump, and a click with the stick pushed over isn't one, or pushing it down to crouch or go
// prone could jump). Let go within kJumpClick, the stick near its middle all the while: the jump as it's let go, once it's stayed let go kJumpSettle (a thumb's slip in a
// recenter's hold isn't one). The game's jump is held a moment through g_jump's own mapping (Space), A's as before (it jumps too).
constexpr ULONGLONG kJumpClick = 300, kJumpSettle = 50, kJumpPress = 150;  // ms
constexpr float kJumpStickMost = 0.5f;  // the stick pushed further than this during the click: a crouch, prone or a turn, no jump
ULONGLONG g_stickJumpUntil = 0;
float g_rightStickLength = 0.0f;        // the right stick's push, as last read (0-1)
float g_leftSqueeze = 0.0f, g_leftTrigger = 0.0f;
bool g_leftThumb = false;
// The right's alike (free hands), read before the focus check as the left's are: its grip, its thumb on the stick, and its trigger's
// pull (g_fire's; g_rightTrigger below is only refreshed with the game in front). Whether each grip was read this frame ([0] left,
// [1] right): the sync worked and its action is active (bound, the controller awake). Its action inactive, its value is 0; the sync
// failed (the headset off), the last one read stays: either way unread, so never a let-go.
float g_rightSqueeze = 0.0f, g_rightTriggerNow = 0.0f;
bool g_rightThumb = false;
bool g_squeezeRead[2] = {};
bool g_calibrateDown = false, g_calibratePressed = false;
XrPath g_handPaths[2] = {};
XrSpace g_handSpaces[2] = {};
XrPosef g_handPoses[2] = {};
bool g_handTracked[2] = {};
// Each controller's own speed and turn as the runtime gives them (from its motion sensors as well as the cameras: XrSpaceVelocity), in the room: m/s, rad/s. Whether each was given.
XrVector3f g_handLinear[2] = {}, g_handAngular[2] = {};
bool g_handLinearValid[2] = {}, g_handAngularValid[2] = {};
// The runtime's own flags of each grip this frame (0 when it wasn't located), and the place it gave when its position is valid, both raw (before
// SteadyHand's hold of a hand lost for a moment): the controllers' tracking record (tracking.h, NoteTracking), which the reach test's lines say.
XrSpaceLocationFlags g_handFlags[2] = {};
XrVector3f g_handRaw[2] = {};
tracking::Record g_track[2];

bool g_ready = false;
bool g_aimTracked = false;
XrPosef g_aimPose{};

// The stand-in hand (weapon.txt "standin 1"): while the right controller isn't tracked (nobody in
// the headset, the controllers asleep), a right hand holds the rifle out in front, aiming where the
// head looks, so the drawn gun, its effects and the aim can be tested without the user. Its grip
// pose sits from its aim pose as the real controller's do: learned whenever the right controller
// is tracked and kept in build\standin.txt for the next run.
std::atomic<bool> g_standIn{false};
std::atomic<bool> g_forceHand{false};  // weapon.txt "forcehand 1": the stand-in holds the gun with the real controller awake too: a film with the controllers lying about
std::atomic<float> g_standInPitch{0.0f};  // radians: the headless head tilted up (to fire at the sky)
std::atomic<float> g_standInUp{0.0f};     // radians: the gun raised from the head (weapon.txt "raisehand 15")
std::atomic<float> g_standInYaw{0.0f};    // radians, left positive: the gun turned from the head (to see its side)
bool g_standingIn = false;  // holding the gun this frame
XrPosef g_standInLeft{};     // its left hand under the handguard, drawn only (StandInLeftPose)
bool g_haveStandInLeft = false;
XrPosef g_aimToGrip{{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}};  // the grip in the aim pose's frame
bool g_haveAimToGrip = false;
ULONGLONG g_aimToGripSavedAt = 0;
// Where it aims from in the head's frame (x right, y up, z back; metres): low and out, the rifle at
// the hip. (Shouldered, 10 cm down and 30 cm out, its stock came a few cm in front of the right eye,
// which then saw it end-on while the left saw it from the side: it looked like a per-eye bug.)
constexpr XrVector3f kStandInAim{0.08f, -0.25f, -0.45f};
// "standin 1 45 0 -0.25 -0.15 0": the hold moved that far (right, up, back; metres), so the drawn
// gun is clearly apart from where the game holds its own (to tell what follows which).
std::atomic<float> g_standInShift[3] = {0.0f, 0.0f, 0.0f};

XrQuaternionf Mul(const XrQuaternionf& a, const XrQuaternionf& b) {
    return {a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y, a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w, a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}

XrQuaternionf Conjugate(const XrQuaternionf& q) {
    return {-q.x, -q.y, -q.z, q.w};
}

XrVector3f Rotate(const XrQuaternionf& q, const XrVector3f& v) {
    const XrQuaternionf r = Mul(Mul(q, {v.x, v.y, v.z, 0.0f}), Conjugate(q));
    return {r.x, r.y, r.z};
}

std::wstring StandInPath() {
    wchar_t path[MAX_PATH];
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&StandInPath), &self);
    GetModuleFileNameW(self, path, MAX_PATH);
    if (wchar_t* slash = wcsrchr(path, L'\\'))
        wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path), L"standin.txt");
    return path;
}

// A tracking jump (the log's "Hands: the gun jumped 30 to 75 degrees and 10 to 35 cm in a frame; the right controller itself turned as much", with the eye close behind a scope, the controller close under the headset): a real hand can't
// go from what it was doing to that in one frame. Such a frame isn't believed: the controller's reading from then on is taken with what it jumped by taken off, fading out over a moment, so the gun goes on from where it was and comes to
// the new place gently; a jump straight back cancels. A hand really moving that fast was already moving nearly that fast the frame before, and isn't touched; nothing's added to any ordinary frame. Off: weapon.txt "steadyhands 0".
std::atomic<bool> g_steadyHands{true};
constexpr float kJumpTurn = 900.0f * 3.14159265f / 180.0f, kJumpMove = 4.5f;  // radians and metres a second: more than this in one frame...
constexpr float kJumpOverLast = 3.0f;                                         // ...and this many times what it did in the frames before
constexpr float kJumpFade = 0.25f;                                            // s: what's left of it each this long, a little over a third
constexpr float kHoldLost = 0.5f;                                             // s with no reading it's held
constexpr float kGapTurn = 8.0f * 3.14159265f / 180.0f, kGapMove = 0.04f;     // back after a gap: further off than this, eased; nearer, simply there
// A hand really moving that fast says so itself: its controller's own motion sensors (the runtime's speed and turn) read at least this share of the step. A hard throw (the user,
// 2026-11-05: the magazine "sometimes locks itself behind my shoulder") read 37-49 degrees and 13-16 cm in its last frame before the let-go: taken out as a jump, the glove and
// the magazine were held back where the hand had been, and it left from behind the shoulder with next to no speed. A tracking jump has no such speed behind it.
constexpr float kJumpBelieved = 0.4f;
// ... and the sensors' reading trails the places by about a frame: a throw's first fast frame was still taken out (its sensors at a third of its step), the next believed, and
// the glove and the magazine held back by that first step through the let-go (the user's next throws, the same day: "not perfect"). Within this many frames after a jump, the
// sensors reading the jump's own speed after all make it a real move: what was taken out is put straight back.
constexpr int kJumpConfirmFrames = 3;
struct SteadyState {
    bool have = false, gap = false;
    XrPosef lastRaw{}, lastOut{}, lastAim{};
    XrTime lastTime = 0;
    float usualTurn = 0.0f, usualMove = 0.0f;  // radians, metres a second, of its last frames that were believed
    XrQuaternionf offTurn{0.0f, 0.0f, 0.0f, 1.0f};
    XrVector3f offMove{0.0f, 0.0f, 0.0f};
    bool off = false;
    int sinceJump = 1000;                    // frames since the last jump taken out
    float jumpMove = 0.0f, jumpTurn = 0.0f;  // its speed then (m/s, rad/s)
    XrVector3f jumpDir{0.0f, 0.0f, 0.0f};    // its step's way (unit, the room)
    // What was taken out before this chain of jumps began (faded as the offset is): a confirmed jump puts back only its own part, not an earlier glitch's.
    XrQuaternionf preTurn{0.0f, 0.0f, 0.0f, 1.0f};
    XrVector3f preMove{0.0f, 0.0f, 0.0f};
    // The aim pose on the grip, as the runtime gives both when the aim is really tracked (the same controller: fixed), to carry a held aim on the steadied grip.
    XrQuaternionf aimTurnOnGrip{0.0f, 0.0f, 0.0f, 1.0f};
    XrVector3f aimAtOnGrip{0.0f, 0.0f, 0.0f};
    bool haveAimOnGrip = false;
};
SteadyState g_steady[2];

float TurnBetween(const XrQuaternionf& a, const XrQuaternionf& b) {
    return 2.0f * std::acos(std::fmin(std::fabs(a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w), 1.0f));
}

void SteadyHand(int side, bool tracked, XrTime time) {
    SteadyState& s = g_steady[side];
    if (!g_steadyHands.load(std::memory_order_relaxed)) {
        s = SteadyState{};
        return;
    }
    if (!tracked) {
        // A moment with no reading (the controller close under the headset, out of its cameras' sight): it stays where it was, as a hand does; it isn't let go of. Longer, it's gone.
        if (s.have && static_cast<float>(time - s.lastTime) * 1e-9f < kHoldLost) {
            g_handPoses[side] = s.lastOut;
            g_handTracked[side] = true;
            if (side == 1) {
                g_aimPose = s.lastAim;
                g_aimTracked = true;
            }
            s.gap = true;
            return;
        }
        s.have = false;  // back after a long gap it starts afresh: where it is is where it is
        s.off = false;
        s.gap = false;
        return;
    }
    XrPosef& grip = g_handPoses[side];
    const XrPosef raw = grip;
    const float dt = s.have ? static_cast<float>(time - s.lastTime) * 1e-9f : 0.0f;
    // The aim pose on the grip, learned whenever the runtime gives the aim itself; a moment it doesn't (its bit off, the grip still read), the aim rides the steadied grip
    // by that, rather than last frame's aim (which compounded with an offset, froze while the grip moved and snapped back after: the review, 2026-11-05).
    const bool aimReal = side == 1 && g_aimTracked;
    if (aimReal) {
        const XrQuaternionf inverse = Conjugate(raw.orientation);
        s.aimTurnOnGrip = Mul(inverse, g_aimPose.orientation);
        s.aimAtOnGrip = Rotate(inverse, {g_aimPose.position.x - raw.position.x, g_aimPose.position.y - raw.position.y, g_aimPose.position.z - raw.position.z});
        s.haveAimOnGrip = true;
    }
    const bool aimHeld = side == 1 && !g_aimTracked && s.have && dt < kHoldLost && s.haveAimOnGrip;
    auto placeAim = [&] {
        const XrVector3f there = Rotate(grip.orientation, s.aimAtOnGrip);
        g_aimPose.orientation = Mul(grip.orientation, s.aimTurnOnGrip);
        g_aimPose.position = {grip.position.x + there.x, grip.position.y + there.y, grip.position.z + there.z};
    };
    if (aimHeld)
        g_aimTracked = true;
    // The same moment asked for again: nothing new and no step to judge, but what's taken out still is (it handed out the raw, jumped pose for that call: the review).
    const bool again = s.have && dt <= 0.0005f && !s.gap;
    if (!again && s.have && (s.gap || dt < 0.1f)) {
        const float frameTime = std::fmax(dt, 0.0005f);
        const float turned = TurnBetween(s.lastRaw.orientation, raw.orientation) / frameTime;
        const XrVector3f d{raw.position.x - s.lastRaw.position.x, raw.position.y - s.lastRaw.position.y, raw.position.z - s.lastRaw.position.z};
        const float step = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z), moved = step / frameTime;
        const XrVector3f dir = step > 1e-6f ? XrVector3f{d.x / step, d.y / step, d.z / step} : XrVector3f{0.0f, 0.0f, 0.0f};
        // Back after a gap, it's taken from where it was held, however far it's come: that's the one jump there's no telling from a real move by its speed.
        // A move believed by the sensors' speed ALONG the step (any speed at all once counted, whichever way the hand went: a sudden real move with a sideways glitch in it
        // passed as real, the review); a turn by the sensors' speed of turning.
        const XrVector3f& lv = g_handLinear[side];
        const XrVector3f& av = g_handAngular[side];
        const float sensedAlong = lv.x * dir.x + lv.y * dir.y + lv.z * dir.z, sensedTurn = std::sqrt(av.x * av.x + av.y * av.y + av.z * av.z);
        const bool moveJump = moved > kJumpMove && moved > s.usualMove * kJumpOverLast, turnJump = turned > kJumpTurn && turned > s.usualTurn * kJumpOverLast;
        const bool moveReal = g_handLinearValid[side] && sensedAlong >= kJumpBelieved * moved, turnReal = g_handAngularValid[side] && sensedTurn >= kJumpBelieved * turned;
        const bool jumped = s.gap ? TurnBetween(s.lastRaw.orientation, raw.orientation) > kGapTurn || step > kGapMove
                                  : (turnJump && !turnReal) || (moveJump && !moveReal);
        if (!s.gap && (moveJump || turnJump) && !jumped) {
            static ULONGLONG s_saidReal = 0;
            if (GetTickCount64() - s_saidReal > 250) {
                s_saidReal = GetTickCount64();
                Log("Hands: the %s controller moved %.0f degrees and %.0f cm in a frame, its own sensors %.1f m/s along it and %.0f degrees a second: a real move, kept",
                    side ? "right" : "left", turned * dt * 57.29578f, moved * dt * 100.0f, sensedAlong, sensedTurn * 57.29578f);
            }
        }
        // A jump taken out a frame or two ago, read by the controller's own sensors after all (its move's speed along the jump's own step, its turn's speed): a real move
        // (a throw's first fast frame). Each part put back on its own, to what was taken out before this jump (an earlier glitch's part stays, fading).
        if (!jumped && s.off && s.sinceJump < kJumpConfirmFrames) {
            const bool moveBack = s.jumpMove > 0.0f && g_handLinearValid[side] && lv.x * s.jumpDir.x + lv.y * s.jumpDir.y + lv.z * s.jumpDir.z >= kJumpBelieved * s.jumpMove;
            const bool turnBack = s.jumpTurn > 0.0f && g_handAngularValid[side] && sensedTurn >= kJumpBelieved * s.jumpTurn;
            if (moveBack) {
                s.offMove = s.preMove;
                s.jumpMove = 0.0f;
            }
            if (turnBack) {
                s.offTurn = s.preTurn;
                s.jumpTurn = 0.0f;
            }
            if (moveBack || turnBack) {
                static ULONGLONG s_saidBack = 0;
                if (GetTickCount64() - s_saidBack > 250) {
                    s_saidBack = GetTickCount64();
                    Log("Hands: the %s controller's jump %d frame%s ago read by its own sensors after all (%.1f m/s along it, %.0f degrees a second): a real move, its %s put back",
                        side ? "right" : "left", s.sinceJump + 1, s.sinceJump ? "s" : "", lv.x * s.jumpDir.x + lv.y * s.jumpDir.y + lv.z * s.jumpDir.z, sensedTurn * 57.29578f,
                        moveBack && turnBack ? "move and turn" : moveBack ? "move" : "turn");
                }
            }
        }
        ++s.sinceJump;
        s.gap = false;
        if (jumped) {
            if (s.sinceJump > kJumpConfirmFrames) {  // a new chain: what was taken out before it, kept to put back to
                s.preTurn = s.off ? s.offTurn : XrQuaternionf{0.0f, 0.0f, 0.0f, 1.0f};
                s.preMove = s.off ? s.offMove : XrVector3f{0.0f, 0.0f, 0.0f};
            }
            s.offTurn = Mul(s.lastOut.orientation, Conjugate(raw.orientation));
            s.offMove = {s.lastOut.position.x - raw.position.x, s.lastOut.position.y - raw.position.y, s.lastOut.position.z - raw.position.z};
            s.off = true;
            s.sinceJump = 0;
            s.jumpMove = moveJump ? moved : 0.0f;
            s.jumpTurn = turnJump ? turned : 0.0f;
            s.jumpDir = dir;
            static ULONGLONG s_said = 0;
            if (GetTickCount64() - s_said > 250) {
                s_said = GetTickCount64();
                Log("Hands: the %s controller jumped %.0f degrees and %.0f cm in a frame (its own sensors %s%.1f m/s along it and %.0f degrees a second): taken out",
                    side ? "right" : "left", turned * dt * 57.29578f, moved * dt * 100.0f, g_handLinearValid[side] ? "" : "not given, ", sensedAlong, sensedTurn * 57.29578f);
            }
        } else {
            s.usualTurn += (turned - s.usualTurn) * 0.5f;
            s.usualMove += (moved - s.usualMove) * 0.5f;
        }
        // Fading (what's kept to put back to as well).
        if (s.off && !jumped) {
            const float keep = std::exp(-dt / kJumpFade);
            auto fadeTurn = [keep](XrQuaternionf& q) {
                const float angle = TurnBetween(q, {0.0f, 0.0f, 0.0f, 1.0f});
                if (angle > 1e-5f) {  // towards none, the short way, by what's kept of the angle
                    const float sign = q.w < 0.0f ? -1.0f : 1.0f;
                    const float half = angle * 0.5f, scale = std::sin(half * keep) / std::sin(half);
                    q = {q.x * sign * scale, q.y * sign * scale, q.z * sign * scale, std::cos(half * keep)};
                }
            };
            s.offMove.x *= keep, s.offMove.y *= keep, s.offMove.z *= keep;
            s.preMove.x *= keep, s.preMove.y *= keep, s.preMove.z *= keep;
            fadeTurn(s.preTurn);
            const float angle = TurnBetween(s.offTurn, {0.0f, 0.0f, 0.0f, 1.0f});
            if (angle < 0.002f && std::fabs(s.offMove.x) + std::fabs(s.offMove.y) + std::fabs(s.offMove.z) < 0.0005f)
                s.off = false;
            else
                fadeTurn(s.offTurn);
        }
    }
    if (s.off) {
        grip.orientation = Mul(s.offTurn, raw.orientation);
        grip.position = {raw.position.x + s.offMove.x, raw.position.y + s.offMove.y, raw.position.z + s.offMove.z};
    }
    // The right controller's aim pose is the same controller: carried on the grip as it is on it (held, or taken out with the grip).
    if (aimHeld || (aimReal && s.off))
        placeAim();
    if (again)
        return;  // (nothing new to keep: the last frame's stays the one judged against)
    s.lastRaw = raw;
    s.lastOut = grip;
    if (side == 1)
        s.lastAim = g_aimPose;
    s.lastTime = time;
    s.have = true;
}

// The real controller's grip seen from its aim pose, each frame both are tracked; saved once a
// minute (and the first time).
void LearnAimToGrip() {
    if (!g_aimTracked || !g_handTracked[1])
        return;
    const XrQuaternionf inverse = Conjugate(g_aimPose.orientation);
    const XrPosef& grip = g_handPoses[1];
    g_aimToGrip.orientation = Mul(inverse, grip.orientation);
    g_aimToGrip.position = Rotate(inverse, {grip.position.x - g_aimPose.position.x,
                                            grip.position.y - g_aimPose.position.y,
                                            grip.position.z - g_aimPose.position.z});
    g_haveAimToGrip = true;
    const ULONGLONG now = GetTickCount64();
    if (g_aimToGripSavedAt && now - g_aimToGripSavedAt < 60000)
        return;
    g_aimToGripSavedAt = now;
    FILE* f = nullptr;
    if (_wfopen_s(&f, StandInPath().c_str(), L"w") == 0 && f) {
        const XrPosef& p = g_aimToGrip;
        fprintf(f, "%f %f %f %f %f %f %f\n", p.orientation.x, p.orientation.y, p.orientation.z, p.orientation.w,
                p.position.x, p.position.y, p.position.z);
        fclose(f);
    }
}

void LoadAimToGrip() {
    FILE* f = nullptr;
    if (_wfopen_s(&f, StandInPath().c_str(), L"r") != 0 || !f)
        return;
    XrPosef p{};
    if (fscanf_s(f, "%f %f %f %f %f %f %f", &p.orientation.x, &p.orientation.y, &p.orientation.z, &p.orientation.w,
                 &p.position.x, &p.position.y, &p.position.z) == 7) {
        g_aimToGrip = p;
        g_haveAimToGrip = true;
        Log("Stand-in hand: the controller's grip from its aim as last seen (build\\standin.txt)");
    }
    fclose(f);
}

// The smooth turn. Through the mouse it stuttered: BF3 reads the mouse on a rhythm of its own, and
// one move a frame landed on both sides of a read about 6 times a second, so a frame barely turned
// and the next turned double (logged 2026-10-30). In stereo the stick's turn now goes into the look
// yaw the mod writes each frame (aim.cpp), which the picture follows, so it's drawn in step with
// the frames. Otherwise (theater mode, no [aim] yaw) a thread moves the mouse a little every
// millisecond. The stick (or F17 test), the mouse thread's speed (counts a second), the direct
// turn's stick and when it was last taken, and when the turn started and was last asked for (ms,
// QueryPerformanceCounter).
float g_stickTurn = 0.0f;
float g_moveTurn = 0.0f;  // the head's angle from the game's facing (SetMoveTurn)
int g_turnTestFrames = 0;
std::atomic<float> g_turnRate{0.0f};
bool g_turnThread = false;
float g_directTurn = 0.0f;
std::atomic<float> g_turnBy{0.0f};  // weapon.txt "turnby": degrees still to turn at once, right positive
double g_lastTake = 0.0;
std::atomic<double> g_turnSince{0.0}, g_turnLast{0.0};

// DirectInput scan codes, which BF3 reads.
constexpr WORD kScanW = 0x11, kScanA = 0x1E, kScanS = 0x1F, kScanD = 0x20;
constexpr WORD kScanSpace = 0x39, kScanCtrl = 0x1D, kScanR = 0x13, kScanE = 0x12;
constexpr WORD kScanShift = 0x2A, kScanG = 0x22, kScan1 = 0x02, kScan2 = 0x03, kScan4 = 0x05;
constexpr WORD kScanF = 0x21;  // (with Z, kScanZ below, the game's quick-time events' other keys: qte.h)

enum class Output { Key, LeftMouse, RightMouse };

// One controller input mapped to one key or mouse button, sent only when it changes.
struct Mapping {
    XrAction* action;
    bool analog;  // float action pressed past a threshold, rather than a button
    Output output;
    WORD scan;
    bool down;
};

Mapping g_buttons[] = {
    {&g_fire, true, Output::LeftMouse, 0, false},  {&g_ads, true, Output::RightMouse, 0, false},
    {&g_grenade, true, Output::Key, kScanG, false}, {&g_jump, false, Output::Key, kScanSpace, false},
    {&g_crouch, false, Output::Key, kScanCtrl, false}, {&g_use, false, Output::Key, kScanE, false},
    {&g_sprint, false, Output::Key, kScanShift, false},
    {&g_selectLauncher, false, Output::Key, kScan4, false}, {&g_selectRifle, false, Output::Key, kScan1, false},
    {&g_selectSwap, false, Output::Key, kScan4, false},
};

// X (g_reload): a tap ejects the magazine, which drops to the floor, or with it out puts a new
// one in. Held for 0.6 s it's the game's own reload (R), as is a tap with a gun the mod doesn't
// know the magazine of.
constexpr ULONGLONG kHoldForReload = 600;
bool g_xDown = false, g_xHeld = false;
ULONGLONG g_xSince = 0;
// The D-pad's right as X (freehands.txt "dpadeject"): whether its press counts, judged as it goes down; when it went down (its log).
ejectkey::State g_eject;
ULONGLONG g_ejectSince = 0;
// A test round's D-pad press under way with nobody in the headset (Sync's not-focused branch works X for it, as FireAlone the trigger).
bool g_scriptX = false;
bool g_rDown = false;
ULONGLONG g_rUntil = 0;  // a tapped R is let go then
bool g_rForPickup = false;  // R held by X held while nobody holds the rifle: the game's own pickup, none of the reload's side effects

// Left stick to W/A/S/D, each pressed past half deflection.
struct StickKey {
    WORD scan;
    bool down;
};
StickKey g_moveKeys[4] = {{kScanW, false}, {kScanS, false}, {kScanA, false}, {kScanD, false}};

bool Check(XrResult result, const char* what) {
    if (XR_SUCCEEDED(result))
        return true;
    char name[XR_MAX_RESULT_STRING_SIZE] = "?";
    xrResultToString(g_instance, result, name);
    Log("Controllers: %s failed: %s", what, name);
    return false;
}

XrPath Path(const char* text) {
    XrPath path = XR_NULL_PATH;
    xrStringToPath(g_instance, text, &path);
    return path;
}

bool MakeAction(XrActionType type, const char* name, const char* label, XrAction& out) {
    XrActionCreateInfo ci{XR_TYPE_ACTION_CREATE_INFO};
    ci.actionType = type;
    strcpy_s(ci.actionName, name);
    strcpy_s(ci.localizedActionName, label);
    return Check(xrCreateAction(g_set, &ci, &out), name);
}

// The right grip's two actions were made (free hands).
bool RightGripMade() {
    return g_squeezeRight != XR_NULL_HANDLE && g_thumbTouchRight != XR_NULL_HANDLE;
}

// Both buzzes were made (Init).
bool HapticsMade() {
    return g_hapticLeft != XR_NULL_HANDLE && g_hapticRight != XR_NULL_HANDLE;
}

// A profile's bindings taken with the buzzes or without them (a profile keeps the last suggestion it took).
void NoteBuzzTaken(XrPath profile, bool took) {
    int kept = 0;
    for (int i = 0; i < g_buzzProfileCount; ++i)
        if (g_buzzProfiles[i] != profile)
            g_buzzProfiles[kept++] = g_buzzProfiles[i];
    g_buzzProfileCount = kept;
    if (took && g_buzzProfileCount < kBuzzProfilesMost)
        g_buzzProfiles[g_buzzProfileCount++] = profile;
}

bool BuzzTaken(XrPath profile) {
    for (int i = 0; i < g_buzzProfileCount; ++i)
        if (profile != XR_NULL_PATH && g_buzzProfiles[i] == profile)
            return true;
    return false;
}

// One suggestion of a profile's bindings (Suggest's tries). A runtime may not know every profile; that profile is skipped and the
// others still apply. Every profile also gets both hands' grip poses, and all but the simple one the left grip's squeeze.
// rightGrip (free hands): the right grip's squeeze and the right thumb's touch as well, tried first. One path a runtime doesn't
// know refuses the whole suggestion, so refused, the same is tried again without them: the left's are never lost for the right's
// (without them the gun simply never leaves the right hand). withoutRight: this is such a retry (the Frame's tries), said so.
// haptics: the bindings end with the two buzzes (Suggest put them there). Taken, the line says so after the count it always gave
// (the two are in the count in all); refused, it's said as a try, since Suggest then makes the same suggestion again without them.
bool SuggestOnce(const char* profile, const std::vector<std::pair<XrAction, const char*>>& bindings, bool leftSqueeze, bool rightGrip,
                 bool withoutRight, bool haptics) {
    std::vector<XrActionSuggestedBinding> suggested;
    for (const auto& b : bindings)
        suggested.push_back({b.first, Path(b.second)});
    suggested.push_back({g_hand, Path("/user/hand/left/input/grip/pose")});
    suggested.push_back({g_hand, Path("/user/hand/right/input/grip/pose")});
    if (leftSqueeze) {
        suggested.push_back({g_squeeze, Path("/user/hand/left/input/squeeze/value")});
        suggested.push_back({g_thumbTouch, Path("/user/hand/left/input/thumbstick/touch")});
    }
    XrInteractionProfileSuggestedBinding info{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
    info.interactionProfile = Path(profile);
    auto accepted = [&](size_t count, bool withoutTheRight) {
        NoteBuzzTaken(info.interactionProfile, haptics);
        const char* without = withoutTheRight ? " without the right grip" : "";
        if (haptics)
            Log("Controllers: bindings for %s accepted%s (%zu) with the haptics (%zu in all)", profile, without, count - 2, count);
        else
            Log("Controllers: bindings for %s accepted%s (%zu)", profile, without, count);
    };
    if (leftSqueeze && rightGrip) {
        if (RightGripMade()) {
            std::vector<XrActionSuggestedBinding> withRight = suggested;
            withRight.push_back({g_squeezeRight, Path("/user/hand/right/input/squeeze/value")});
            withRight.push_back({g_thumbTouchRight, Path("/user/hand/right/input/thumbstick/touch")});
            info.countSuggestedBindings = static_cast<uint32_t>(withRight.size());
            info.suggestedBindings = withRight.data();
            if (XR_SUCCEEDED(xrSuggestInteractionProfileBindings(g_instance, &info))) {
                accepted(withRight.size(), false);
                return true;
            }
        }
        withoutRight = true;
    }
    info.countSuggestedBindings = static_cast<uint32_t>(suggested.size());
    info.suggestedBindings = suggested.data();
    if (XR_SUCCEEDED(xrSuggestInteractionProfileBindings(g_instance, &info))) {
        accepted(suggested.size(), withoutRight);
        return true;
    }
    if (haptics)
        Log("Controllers: %s refused the bindings with its haptics added (%zu tried): tried again without them", profile, suggested.size());
    else
        Log("Controllers: no bindings for %s on this runtime (%zu tried)", profile, suggested.size());
    return false;
}

// Each profile's bindings with the buzz in both hands added first; refused (one path it doesn't know refuses the whole suggestion), the
// same as they always were: nothing is ever lost for the haptics. The Frame's tries come through here too, each with them first.
bool Suggest(const char* profile, const std::vector<std::pair<XrAction, const char*>>& bindings,
             bool leftSqueeze = true, bool rightGrip = false, bool withoutRight = false) {
    if (HapticsMade()) {
        auto withHaptics = bindings;
        withHaptics.push_back({g_hapticLeft, "/user/hand/left/output/haptic"});
        withHaptics.push_back({g_hapticRight, "/user/hand/right/output/haptic"});
        if (SuggestOnce(profile, withHaptics, leftSqueeze, rightGrip, withoutRight, true))
            return true;
    }
    return SuggestOnce(profile, bindings, leftSqueeze, rightGrip, withoutRight, false);
}

void SuggestBindings() {
    // Valve Index: A/B on both hands.
    Suggest("/interaction_profiles/valve/index_controller",
            {{g_aim, "/user/hand/right/input/aim/pose"},
             {g_fire, "/user/hand/right/input/trigger/value"},
             {g_ads, "/user/hand/left/input/trigger/value"},
             {g_move, "/user/hand/left/input/thumbstick"},
             {g_turn, "/user/hand/right/input/thumbstick"},
             {g_jump, "/user/hand/right/input/a/click"},
             {g_crouch, "/user/hand/right/input/b/click"},
             {g_reload, "/user/hand/left/input/a/click"},
             {g_use, "/user/hand/left/input/b/click"},
             {g_selectLauncher, "/user/hand/left/input/trackpad/force"},
             {g_selectRifle, "/user/hand/right/input/trackpad/force"},
             {g_sprint, "/user/hand/left/input/thumbstick/click"},
             {g_recenter, "/user/hand/right/input/thumbstick/click"}},
            true, true);
    // Quest / Touch style: A/B right, X/Y left. SteamVR also uses this for many other controllers.
    Suggest("/interaction_profiles/oculus/touch_controller",
            {{g_aim, "/user/hand/right/input/aim/pose"},
             {g_fire, "/user/hand/right/input/trigger/value"},
             {g_ads, "/user/hand/left/input/trigger/value"},
             {g_move, "/user/hand/left/input/thumbstick"},
             {g_turn, "/user/hand/right/input/thumbstick"},
             {g_jump, "/user/hand/right/input/a/click"},
             {g_crouch, "/user/hand/right/input/b/click"},
             {g_reload, "/user/hand/left/input/x/click"},
             {g_use, "/user/hand/left/input/y/click"},
             {g_selectSwap, "/user/hand/left/input/menu/click"},
             {g_sprint, "/user/hand/left/input/thumbstick/click"},
             {g_recenter, "/user/hand/right/input/thumbstick/click"}},
            true, true);
    // Steam Frame (when the runtime offers its extension): A/B/X/Y all on the right hand, a D-pad
    // on the left. X is the magazine. Its grip's name isn't documented: squeeze, else grip.
    std::vector<std::pair<XrAction, const char*>> frame = {
        {g_aim, "/user/hand/right/input/aim/pose"},
        {g_fire, "/user/hand/right/input/trigger/value"},
        {g_ads, "/user/hand/left/input/trigger/value"},
        {g_move, "/user/hand/left/input/thumbstick"},
        {g_turn, "/user/hand/right/input/thumbstick"},
        {g_jump, "/user/hand/right/input/a/click"},
        {g_crouch, "/user/hand/right/input/b/click"},
        {g_reload, "/user/hand/right/input/x/click"},
        {g_use, "/user/hand/right/input/y/click"},
        {g_sprint, "/user/hand/left/input/thumbstick/click"},
        {g_recenter, "/user/hand/right/input/thumbstick/click"}};
    const char* kFrame = "/interaction_profiles/valve/frame_controller";
    // Its grips are squeeze or grip; the View button may not be offered: each try leaves out more.
    // The D-pad picks the weapon: up the launcher (key 4), down the rifle (key 1).
    // right (free hands): the right grip by the same name as the left's, and with the View button the right thumb's touch too.
    // eject: with the D-pad, its right as X for the gun in the left hand (g_ejectLeft).
    auto with = [&](bool squeeze, bool grip, bool view, bool dpad = false, bool right = false, bool eject = false) {
        auto b = frame;
        if (dpad) {
            b.push_back({g_selectLauncher, "/user/hand/left/input/dpad_up/click"});
            b.push_back({g_selectRifle, "/user/hand/left/input/dpad_down/click"});
            if (eject)
                b.push_back({g_ejectLeft, "/user/hand/left/input/dpad_right/click"});
        }
        if (squeeze) {
            b.push_back({g_squeeze, "/user/hand/left/input/squeeze/value"});
            if (right)
                b.push_back({g_squeezeRight, "/user/hand/right/input/squeeze/value"});
        } else if (grip) {
            b.push_back({g_squeeze, "/user/hand/left/input/grip/value"});
            if (right)
                b.push_back({g_squeezeRight, "/user/hand/right/input/grip/value"});
        }
        if (view) {
            b.push_back({g_calibrate, "/user/hand/left/input/view/click"});
            b.push_back({g_thumbTouch, "/user/hand/left/input/thumbstick/touch"});
            if (right)
                b.push_back({g_thumbTouchRight, "/user/hand/right/input/thumbstick/touch"});
        }
        return b;
    };
    // Each try first with the right's added; refused (one path it doesn't know refuses the whole suggestion), the same try as it was,
    // and only then the next, which leaves out more: whichever try the runtime takes, the left's grip, D-pad and View are what they
    // were, with the right's added when it takes them too.
    auto tryWithRight = [&](bool squeeze, bool grip, bool view, bool dpad = false, bool eject = false) {
        return (RightGripMade() && Suggest(kFrame, with(squeeze, grip, view, dpad, true, eject), false)) ||
               Suggest(kFrame, with(squeeze, grip, view, dpad, false, eject), false, false, true);
    };
    // The D-pad's right first, as two tries of their own before all the others (its path not known refuses the whole try): refused, the
    // tries below are exactly as they were.
    g_ejectBound = g_ejectLeft != XR_NULL_HANDLE && (tryWithRight(true, false, true, true, true) || tryWithRight(false, true, true, true, true));
    if (g_ejectBound)
        Log("Controllers: the Frame's D-pad right bound: X's job with the gun in the left hand");
    else if (g_ejectLeft != XR_NULL_HANDLE)
        Log("Controllers: the Frame's D-pad right not taken: X alone ejects (the bindings as before)");
    if (!g_ejectBound && !tryWithRight(true, false, true, true) && !tryWithRight(false, true, true, true) &&
        !tryWithRight(true, false, true) && !tryWithRight(false, true, true) &&
        !tryWithRight(true, false, false) && !tryWithRight(false, true, false))
        Suggest(kFrame, with(false, false, false), false);
    // Anything else: at least aim and fire.
    Suggest("/interaction_profiles/khr/simple_controller",
            {{g_aim, "/user/hand/right/input/aim/pose"}, {g_fire, "/user/hand/right/input/select/click"}});
}

void SendKey(WORD scan, bool down) {
    INPUT in{};
    in.type = INPUT_KEYBOARD;
    in.ki.wScan = scan;
    in.ki.dwFlags = KEYEVENTF_SCANCODE | (down ? 0 : KEYEVENTF_KEYUP);
    SendInput(1, &in, sizeof(in));
}

// A click reaches the game and also the window under the pointer, so a press only goes with the
// pointer on BF3's window (window.cpp): on the taskbar's "Show desktop" corner, every shot
// minimised the game (2026-10-30). A release always goes, so a button never sticks down.
std::atomic<bool> g_holdZoom{false};  // weapon.txt "zoom 1": the game's zoom (right mouse) held
std::atomic<ULONGLONG> g_sprintAt{0};  // when sprint (the left stick's click) was last held
std::atomic<float> g_rightTrigger{0.0f};  // how far the right trigger is pulled (0-1)
bool g_mouseDown[2] = {};  // left, right: as the game was last sent them
// The mod's own last release of each button refused by Windows (SendInput took nothing: a window of higher rights or the secure desktop in front), not
// sent since: Windows then has the button down as well, from the mod's press (KeepButtonsUp sends a lone one, and reads that hold as the mod's own).
bool g_upRefused[2] = {};

bool GameHasFocus();

// The fire button's sends said in the log with whether bf3 had the focus (inputrules.h's StepSay: a few a second at most, the rest counted into the next
// line): a release that went while another window was in front may never reach the game, which then fires by itself (KeepButtonsUp). went: SendInput
// took it.
inputrules::SayState g_fireSay;
void SayFireSend(bool down, bool went) {
    const bool front = GameHasFocus();
    const inputrules::SayOut say = inputrules::StepSay(g_fireSay, GetTickCount64(), front);
    if (!say.say)
        return;
    char where[112] = "bf3 in front";
    if (!front) {
        char name[64] = "";
        const HWND window = GetForegroundWindow();
        if (!window || !GetClassNameA(window, name, sizeof(name)))
            strcpy_s(name, "none");
        snprintf(where, sizeof(where), "bf3 not in front: the window in front of class %s; the game may not see it", name);
    }
    char quiet[96] = "";
    if (say.quiet > 0)
        snprintf(quiet, sizeof(quiet), "; %d sends not said before it, %d of them with bf3 not in front", say.quiet, say.quietAway);
    Log("Input: the fire button %s %s (%s%s)", down ? "down" : "up", went ? "sent" : "refused by Windows (SendInput took nothing)", where, quiet);
}

void SendMouseButton(bool left, bool down) {
    bool& sent = g_mouseDown[left ? 0 : 1];
    if (down == sent)
        return;
    if (!window::PointerOnGame() && down) {
        static ULONGLONG lastLog = 0;
        if (GetTickCount64() - lastLog > 1000) {
            lastLog = GetTickCount64();
            Log("Input: a %s click held back; the mouse pointer couldn't be put on the game's window",
                left ? "fire" : "right");
        }
        return;
    }
    INPUT in{};
    in.type = INPUT_MOUSE;
    in.mi.dwFlags = left ? (down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP)
                         : (down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP);
    const bool went = SendInput(1, &in, sizeof(in)) == 1;
    sent = down;
    g_upRefused[left ? 0 : 1] = !down && !went;  // (a press of its own after it: its release to come)
    if (left)
        SayFireSend(down, went);
}

// One button's release alone, past SendMouseButton's own record (it's up there already: KeepButtonsUp sends this only while the mod holds the button
// nowhere), so a release the game never saw reaches it. g_mouseDown is left as it is. False when SendInput took nothing.
bool SendLoneRelease(bool left) {
    INPUT in{};
    in.type = INPUT_MOUSE;
    in.mi.dwFlags = left ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_RIGHTUP;
    return SendInput(1, &in, sizeof(in)) == 1;
}

void SendMouseMove(int dx) {
    INPUT in{};
    in.type = INPUT_MOUSE;
    in.mi.dx = dx;
    in.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, &in, sizeof(in));
}

double NowMs() {
    static LARGE_INTEGER frequency = [] {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        return f;
    }();
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return 1000.0 * static_cast<double>(now.QuadPart) / static_cast<double>(frequency.QuadPart);
}

// Injected input goes to whichever window has focus; never type into anything but BF3.
bool GameHasFocus() {
    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    return pid == GetCurrentProcessId();
}

// Moves the mouse at g_turnRate, carrying the fractions over. It stops on its own when the frames
// stop asking (a loading screen, a stalled game) or BF3 loses focus.
DWORD WINAPI TurnThread(LPVOID) {
    HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    double carry = 0.0, last = NowMs();
    for (;;) {
        LARGE_INTEGER due;
        due.QuadPart = -10000;  // 1 ms
        if (timer && SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE))
            WaitForSingleObject(timer, INFINITE);
        else
            Sleep(1);
        const double now = NowMs(), elapsed = std::min(now - last, 20.0);
        last = now;
        const float rate = g_turnRate.load();
        if (rate == 0.0f || now - g_turnLast.load() > 50.0 || !GameHasFocus()) {
            carry = 0.0;
            continue;
        }
        carry += rate * elapsed / 1000.0;
        if (int dx = static_cast<int>(carry)) {  // toward zero, the rest carried
            carry -= dx;
            SendMouseMove(dx);
        }
    }
}

// How far the stick at x (-1 to 1) is past its dead zone (0 to 1).
float TurnAmount(float x) {
    return std::fabs(x) > 0.2f ? (std::fabs(x) - 0.2f) / 0.8f : 0.0f;
}

void NoteTurning() {
    const double now = NowMs();
    if (now - g_turnLast.load() > 40.0)
        g_turnSince.store(now);
    g_turnLast.store(now);
}

// The mouse thread's speed for the stick at x (inside the dead zone: none). turn_speed is the
// mouse counts of a 72nd of a second at full deflection.
void SetMouseTurn(float x) {
    const float amount = TurnAmount(x);
    if (amount == 0.0f) {
        g_turnRate.store(0.0f);
        return;
    }
    if (!g_turnThread) {
        g_turnThread = true;
        if (HANDLE t = CreateThread(nullptr, 0, TurnThread, nullptr, 0, nullptr))
            CloseHandle(t);
    }
    NoteTurning();
    g_turnRate.store(std::copysign(amount * config::Get().turnSpeed * 72.0f, x));
}

// The stick turn seen from frame to frame while it turns steadily: 0 = the game's own camera as
// built (views.cpp), 1 = what is shown (the look yaw written this frame against the controller's,
// xr_runtime.cpp). Even steps are a smooth turn; a frame without a step and then a double one is a
// stutter. Logged once a second while turning.
struct TurnSteps {
    const char* name;
    float lastYaw = 0.0f;
    bool haveLast = false;
    std::vector<float> steps;  // degrees
    float net = 0.0f;          // the steps with their signs (degrees): which way it turned
    ULONGLONG lastLog = 0;
};
TurnSteps g_turnSteps[2] = {{"the game's camera"}, {"shown"}};

bool TurningSteadily() {
    const double now = NowMs();
    return now - g_turnLast.load() < 40.0 && now - g_turnSince.load() > 100.0;
}

void Press(const Mapping& m, bool down) {
    if (m.output == Output::Key)
        SendKey(m.scan, down);
    else
        SendMouseButton(m.output == Output::LeftMouse, down);
}

ULONGLONG g_rLetGoAt = 0;  // when R was last let go here (0 never): the game's reload just done (GameReloadSince)
// X held with a tube shotgun whose shells go in by hand: when R is to go down (0: not waiting), a moment after ammo::TubeAsk.
ULONGLONG g_tubeRAt = 0;
constexpr ULONGLONG kTubeRDelay = 60;

void PressR(bool down) {
    if (down != g_rDown) {
        SendKey(kScanR, down);
        g_rDown = down;
        if (!down)
            g_rLetGoAt = GetTickCount64();
    }
    if (!down)
        g_rForPickup = false;
}

// Y (use) held 0.7 s goes between the launcher and the rifle, for when the left controller isn't there (its D-pad is the
// plain way; the user's left controller slept through the first try, 2026-11-02). Each pick is a key held 0.3 s: a tap of
// 0.1 s through the game's keyboard read didn't always take.
constexpr ULONGLONG kHoldToSwap = 700, kSelectPress = 300;
ULONGLONG g_useSince = 0, g_selectUntil = 0;
bool g_useSwapped = false, g_toLauncher = true;
WORD g_selectScan = 0;

void HoldSelect(WORD scan) {
    if (g_selectScan && g_selectScan != scan)
        SendKey(g_selectScan, false);
    if (g_selectScan != scan)
        SendKey(scan, true);
    g_selectScan = scan;
    g_selectUntil = GetTickCount64() + kSelectPress;
    ammo::Picked(scan == kScan1 ? 0 : scan == kScan2 ? 1 : -1);  // (the slot told to come up: two guns of one kind told apart by it)
}

// A slot's key: 0 key 1 (the rifle), 1 key 2 (the Stinger, or whatever the game keeps in slot 1), 5 key 4 (the launcher); 0 for a slot no key reaches.
int KeyOfSlot(int slot) {
    return slot == 0 ? 1 : slot == 1 ? 2 : slot == 5 ? 4 : 0;
}

WORD ScanOfSlot(int slot) {
    return slot == 0 ? kScan1 : slot == 1 ? kScan2 : slot == 5 ? kScan4 : WORD{0};
}

// The user's last pick (LastPickAt: holsters put the gun the game gives after it into the right hand): when (GetTickCount64, 0 none), its slot (-1 X
// held's pickup), and how many in all (two frames can share one tick). Never holsters' own keys (SelectSlot). Sync's own, the render thread.
ULONGLONG g_pickAt = 0;
int g_pickSlot = -1;
unsigned g_picks = 0;

void NotePick(int slot) {
    g_pickAt = GetTickCount64();
    g_pickSlot = slot;
    ++g_picks;
}

// The gun put away and its place, for input's lines: "the M416 at the right shoulder" (hands::StowedWhere), else "the gun put away".
void StowedWords(char* out, size_t size) {
    const char* where = hands::StowedWhere();
    snprintf(out, size, "%s", where && where[0] ? where : "the gun put away");
}

// A pick of the user's (the D-pad, Touch's menu button, Y held: lead "Y held: "): recorded (LastPickAt), then its key held (HoldSelect). One that would
// raise the gun put away at its place (hands::PickIsOwnSlot: the game's weapon already, and the game ignores its own weapon's key) sends no key: holsters
// draw that gun into the right hand at the next hands::Update. Another slot's, with the gun put away, goes as ever: the game's switch, its gun into
// the right hand (holster.h's Arrive goes by the pick's time).
void Pick(int slot, const char* lead) {
    NotePick(slot);
    const char* name = slot == 1 ? "Stinger" : slot == 5 ? "launcher" : "rifle";
    const int key = KeyOfSlot(slot);
    char where[128];
    if (hands::PickIsOwnSlot(slot)) {
        StowedWords(where, sizeof(where));
        Log("Input: %sthe %s picked (key %d not sent: %s is the game's weapon already): holsters are to draw it from its place into the right hand", lead,
            name, key, where);
        return;
    }
    const bool stowed = hands::GunStowed();
    HoldSelect(ScanOfSlot(slot));
    if (stowed) {
        StowedWords(where, sizeof(where));
        Log("Input: %sthe %s (key %d) picked with nothing in the hands (%s): the game's switch, into the right hand", lead, name, key, where);
    } else {
        Log("Input: %sthe %s (key %d) picked", lead, name, key);
    }
}

// The grenade launcher under the rifle, with a hand on its grip (hands::M320HeldBy, either hand): the rifle stays in the game's hand and the trigger of the hand on the
// rifle's pistol grip fires it (the user, 2026-11-05: "I want to be able to shoot at the same time ... the right trigger to work. That has to work"); a pull of the
// trigger of the hand on the launcher's grip is a grenade: the launcher picked by the game's own "switch to the grenade launcher" (the pad's D-pad left: its profile has
// it there only), fired, and the same toggle again back to the rifle. Both switches instant (ammo.cpp: AltDeployTime 0). Key 1 doesn't do for the way back: after a
// shot, empty, the game ignored it (six presses in 2.5 s), and pressed as it fired it kept the game from reloading it. Empty, the game's own reload instead (picked, R
// held a moment, back once its round's in): the user wants "the original reload that the game has" kept; X held with a hand on its grip does it too. (Its reload by
// hand: m320.h.)
enum class M320Shot { kIdle, kSelecting, kFiring, kReloading, kBack };
M320Shot g_m320Shot = M320Shot::kIdle;
ULONGLONG g_m320ShotAt = 0, g_m320IdleUpSince = 0, g_m320BackPressAt = 0, g_m320PickPressAt = 0;
int g_m320LoadedAtShot = 0;
bool g_m320JustReload = false;
// Each trigger's press for the launcher, [0] the left, [1] the right (past 0.6, let go under 0.4), followed every call whichever hand is on its grip; and the hand on
// its grip as last logged (-1 none).
bool g_m320Pulled[2] = {};
int g_m320GripWas = -1;
// R for its reload, held kM320ReloadPress, with the game in front (let go by UpdateM320 or on losing it: ReleaseAll).
WORD g_m320Key = 0;
ULONGLONG g_m320KeyUntil = 0;
constexpr ULONGLONG kM320ReloadPress = 300, kM320Toggle = 200, kM320ToggleAgain = 450;
constexpr unsigned short kPadLauncher = 0x0004;  // the D-pad's left

// weapon.txt "m320shot": as a pull of the trigger of the hand on its grip (a test, nobody in the headset: arms.txt "test m320" for the hand)
std::atomic<bool> g_m320TestShot{false};
// One more each time its round left (kFiring's "fired"): recoil's kick for it (M320Fired).
std::atomic<unsigned> g_m320Fired{0};

// A film round's triggers (SetFilmTrigger: build\freehands.txt "test N", hands.cpp's scripted hands), [0] left, [1] right: the pull last given,
// and whether it was given since the last Sync (not given: let go).
std::atomic<float> g_filmTrigger[2] = {0.0f, 0.0f};
std::atomic<bool> g_filmTriggerGiven[2] = {false, false};
// ... and whether each was given since the launcher's trigger last looked, and when it last found one (GetTickCount64, 0 never). The launcher's
// trigger (UpdateM320Shot, once a frame before Sync; the scripted hands give theirs later in the frame) goes by a hand's film pull while it keeps
// coming, for kFilmPullFor after the last it found, else by that hand's controller: a scripted hand on its grip works it as a real hand does. Timed
// from its own look, not the giving: a slow frame between the two (a still written, a hitch) never hands a held pull to the sleeping controller for
// a call, which let go of it and made the next frame's a second press (a dry pull).
std::atomic<bool> g_filmTriggerNew[2] = {false, false};
ULONGLONG g_filmTriggerSeenAt[2] = {0, 0};
constexpr ULONGLONG kFilmPullFor = 250;  // ms

// weapon.txt "filmtrigger" (StartFilmPull): a film's pulls of one trigger, given at the top of Sync as a scripted hand's are (SetFilmTrigger), so they go
// through FireAlone and FireButton as any pull does (FireBlocked, the kept-back round, the trigger's stop). The hand (-1 none under way), each pull's
// hold, how many, when the first began; the pull last said (its number twice, plus 1 held), and whether it's been said held off. Sync's own but for
// the settings, set by the reader before the hand.
std::atomic<int> g_filmPullHand{-1};
std::atomic<int> g_filmPullCount{0};
std::atomic<ULONGLONG> g_filmPullHold{0}, g_filmPullFrom{0}, g_filmPullAsked{0};
std::atomic<int> g_filmPullSaid{-1};
bool g_filmPullHeldSaid = false;
constexpr ULONGLONG kFilmPullGap = 600;            // ms let go between two pulls (past the trigger's own let-go, below 0.4)
constexpr ULONGLONG kFilmPullWaitMost = 10000;     // ms held off since it was asked for: dropped (a film gone before its pulls)
constexpr float kFilmPullPitch = -45.0f * 3.14159265f / 180.0f;  // the stand-in's head at least this far down: real rounds into the ground

void FilmPullStep() {
    const int hand = g_filmPullHand.load(std::memory_order_acquire);
    if (hand < 0)
        return;
    const int pulls = g_filmPullCount.load(std::memory_order_relaxed);
    const ULONGLONG hold = g_filmPullHold.load(std::memory_order_relaxed), period = hold + kFilmPullGap;
    const ULONGLONG now = GetTickCount64(), since = now - g_filmPullFrom.load(std::memory_order_relaxed);
    const char* name = hand == 0 ? "left" : "right";
    // Real rounds: only into the ground in front, the stand-in standing in (as every film's real round). Held off, it waits (said once), giving no
    // trigger (a frame without one is let go; one given would stand in for the launcher's trigger too: UpdateM320Shot). Dropped once its pulls had
    // begun, or held off kFilmPullWaitMost since it was asked for: no pulls left over for a later stand-in.
    if (!g_standIn.load(std::memory_order_relaxed) || g_standInPitch.load(std::memory_order_relaxed) > kFilmPullPitch) {
        const bool begun = g_filmPullSaid.load(std::memory_order_relaxed) >= 0;
        if (begun || now - g_filmPullAsked.load(std::memory_order_relaxed) > kFilmPullWaitMost) {
            int was = hand;
            if (g_filmPullHand.compare_exchange_strong(was, -1)) {
                g_filmPullSaid = -1;
                g_filmPullHeldSaid = false;
                Log("Input: a film's pull of the %s trigger dropped: %s", name,
                    begun ? "the stand-in went off, or its head came up, during its pulls" : "held off 10 s since it was asked for");
            }
            return;
        }
        if (!g_filmPullHeldSaid)
            Log("Input: a film's pull of the %s trigger held off: the stand-in isn't on with its head 45 degrees down or more (weapon.txt standin 1 -70)",
                name);
        g_filmPullHeldSaid = true;
        g_filmPullFrom.store(now, std::memory_order_relaxed);  // (its pulls from when it may)
        return;
    }
    g_filmPullHeldSaid = false;
    const int pull = static_cast<int>(since / period);
    if (pull >= pulls) {
        input::SetFilmTrigger(hand, 0.0f);  // (a last frame let go: FireAlone says it)
        g_filmPullHand.store(-1, std::memory_order_relaxed);
        g_filmPullSaid = -1;
        int loaded = 0, spare = 0;
        ammo::Get(loaded, spare);
        Log("Input: a film's pull of the %s trigger: done, %d pull%s (%d rounds loaded, %d spare)", name, pulls, pulls == 1 ? "" : "s", loaded, spare);
        return;
    }
    const bool held = since % period < hold;
    input::SetFilmTrigger(hand, held ? 1.0f : 0.0f);
    if (const int said = pull * 2 + (held ? 1 : 0); said != g_filmPullSaid) {
        g_filmPullSaid = said;
        int loaded = 0, spare = 0;
        ammo::Get(loaded, spare);
        if (held)
            Log("Input: a film's pull of the %s trigger, %d of %d, held %.1f s (%d rounds loaded, %d spare)", name, pull + 1, pulls, hold / 1000.0, loaded, spare);
        else
            Log("Input: a film's pull of the %s trigger, %d of %d, let go (%d rounds loaded, %d spare)", name, pull + 1, pulls, loaded, spare);
    }
}

void M320Back(const char* why) {
    if (g_m320Shot == M320Shot::kFiring)
        SendMouseButton(true, false);
    const bool now = ammo::M320Up() && ammo::M320State() != 5;  // (firing: kBack toggles as soon as it's done)
    if (now)
        pad::PressButtons(kPadLauncher, kM320Toggle);
    g_m320Shot = M320Shot::kBack;
    g_m320ShotAt = GetTickCount64();
    g_m320BackPressAt = now ? g_m320ShotAt : 0;
    Log("Input: the grenade launcher %s: back to the rifle (its own toggle); %d loaded, %d spare", why, ammo::M320Loaded(), ammo::M320Spare());
}

void StartM320(bool reloadOnly) {
    g_m320JustReload = reloadOnly;
    g_m320PickPressAt = 0;
    if (!ammo::M320Up()) {
        pad::PressButtons(kPadLauncher, kM320Toggle);
        g_m320PickPressAt = GetTickCount64();
    }
    g_m320Shot = M320Shot::kSelecting;
    g_m320ShotAt = GetTickCount64();
    Log("Input: the grenade launcher picked to %s (%d loaded, %d spare)", reloadOnly ? "be reloaded by the game" : "fire", ammo::M320Loaded(), ammo::M320Spare());
}

void UpdateM320Shot() {
    const ULONGLONG now = GetTickCount64();
    if (g_m320Key && now >= g_m320KeyUntil) {
        SendKey(g_m320Key, false);
        g_m320Key = 0;
    }
    // Each hand's own trigger, each with its own press: a film round's pull for a hand it moves (one found within kFilmPullFor), else that controller's.
    // Both followed every call, so a trigger already held as its hand takes the grip doesn't fire.
    const float controller[2] = {g_leftTrigger, g_rightTriggerNow};
    bool fresh[2] = {};
    for (int side = 0; side < 2; ++side) {
        if (g_filmTriggerNew[side].exchange(false, std::memory_order_relaxed))
            g_filmTriggerSeenAt[side] = now;
        const ULONGLONG seenAt = g_filmTriggerSeenAt[side];
        const float pull = seenAt && now < seenAt + kFilmPullFor ? g_filmTrigger[side].load(std::memory_order_relaxed) : controller[side];
        const bool pulled = pull > (g_m320Pulled[side] ? 0.4f : 0.6f);
        fresh[side] = pulled && !g_m320Pulled[side];
        g_m320Pulled[side] = pulled;
    }
    // The hand on its grip (hands::M320HeldBy, a frame old as every hands:: flag here), read once: a fresh pull of its trigger is the launcher's. The test's
    // pull (weapon.txt m320shot) is taken in a call without one; with no hand on its grip, in a call without a fresh pull of the left's, as before (neither
    // is used then).
    const int heldBy = hands::M320HeldBy();
    const int onGrip = heldBy == 0 || heldBy == 1 ? heldBy : -1;
    if (onGrip != g_m320GripWas) {
        g_m320GripWas = onGrip;
        if (onGrip >= 0)
            Log("Input: the grenade launcher's trigger is the %s hand's now (on its grip)", onGrip == 0 ? "left" : "right");
        else
            Log("Input: no hand on the grenade launcher's grip now");
    }
    const bool freshPull = fresh[onGrip >= 0 ? onGrip : 0] || g_m320TestShot.exchange(false);
    switch (g_m320Shot) {
    case M320Shot::kIdle:
        if (freshPull && onGrip >= 0 && ammo::M320Carried()) {
            // Its reload by hand (m320.h): fired, open or swinging shut, the pull is the index on the barrel's release instead (a dry pull, open); ready, a shot as before.
            // Off ("m320byhand 0") or not in use, FireReady is true: as before, the game's own reload on an empty pull.
            if (m320::ByHand() && !m320::FireReady())
                m320::PressRelease();
            else if (ammo::M320Loaded() >= 1) {
                // Free hands: no hand on the rifle's pistol grip (held by its handguard alone), nothing fires, the grenade neither.
                if (hands::FireHand() >= 0)
                    StartM320(false);
                else
                    Log("Input: no hand on the rifle's pistol grip: the grenade doesn't fire");
            } else if (ammo::M320Spare() >= 1)
                StartM320(true);
            else
                Log("Input: the grenade launcher's empty, no rounds left");
        }
        // In the game's hand with nothing asked of it (a pick cut short): the rifle back after 3 s.
        if (g_m320Shot == M320Shot::kIdle && ammo::M320Up()) {
            if (!g_m320IdleUpSince)
                g_m320IdleUpSince = now;
            else if (now - g_m320IdleUpSince > 3000) {
                g_m320IdleUpSince = 0;
                M320Back("in hand with nothing asked of it");
            }
        } else {
            g_m320IdleUpSince = 0;
        }
        break;
    case M320Shot::kSelecting:
        if (ammo::M320Up()) {
            g_m320ShotAt = now;
            if (g_m320JustReload) {
                // Its count left at 0 for the game (it's kept at 1 otherwise: ammo.cpp), and R held a moment.
                if (!ammo::M320GameReload()) {
                    M320Back("couldn't be left empty for the game's reload");
                    break;
                }
                g_m320Shot = M320Shot::kReloading;
                if (!g_m320Key) {
                    SendKey(kScanR, true);
                    g_m320Key = kScanR;
                    g_m320KeyUntil = now + kM320ReloadPress;
                }
            } else {
                g_m320LoadedAtShot = ammo::M320Loaded();
                g_m320Shot = M320Shot::kFiring;
                SendMouseButton(true, true);  // (the fire mapping holds it too while the headset's on: the same button, sent once)
            }
        } else if (now - g_m320ShotAt > 1500) {
            M320Back("didn't come into hand in 1.5 s");
        } else if (g_m320PickPressAt && now - g_m320PickPressAt > kM320ToggleAgain && (ammo::M320State() == 0 || ammo::M320State() == 3)) {
            // Its pick dropped: pulled just as the game was still switching back to the rifle after its own reload (0.13 s after, 2026-11-05: "didn't come into hand"),
            // the toggle went nowhere. Pressed again, as the way back is.
            pad::PressButtons(kPadLauncher, kM320Toggle);
            g_m320PickPressAt = now;
            Log("Input: the grenade launcher's pick pressed again (it hadn't come into hand: its state %d)", ammo::M320State());
        }
        break;
    case M320Shot::kFiring:
        if (ammo::M320Loaded() < g_m320LoadedAtShot) {
            g_m320Fired.fetch_add(1, std::memory_order_relaxed);
            M320Back("fired");
        } else if (now - g_m320ShotAt > 600)
            M320Back("didn't fire in 0.6 s");
        break;
    case M320Shot::kReloading: {
        // Back once it's ready (state 2): a toggle as its reload ends (11, 12) was dropped, and the next took it round again (2026-11-05).
        static ULONGLONG s_loadedAt = 0;
        if (ammo::M320Loaded() >= 1) {
            if (!s_loadedAt)
                s_loadedAt = now;
            if (ammo::M320State() == 2 || now - s_loadedAt > 800) {
                s_loadedAt = 0;
                M320Back("reloaded by the game");
            }
        } else if (now - g_m320ShotAt > 4500) {
            s_loadedAt = 0;
            M320Back("not reloaded in 4.5 s");
        }
        break;
    }
    case M320Shot::kBack:
        // The toggle again while it's still in hand and not firing (it fires for a frame or two: a toggle then may be dropped), every 0.45 s, for up to 2.5 s. (Toggled
        // away while it reloads, its reload's called off: rlWeaponSwitchCancelsUnfinishedReload.)
        if (!ammo::M320Up()) {
            g_m320Shot = M320Shot::kIdle;
        } else if (now - g_m320ShotAt > 2500) {
            g_m320Shot = M320Shot::kIdle;
            Log("Input: the grenade launcher still in hand 2.5 s after it was toggled back to the rifle (its state %d)", ammo::M320State());
        } else if (ammo::M320State() != 5 && now - g_m320BackPressAt > kM320ToggleAgain) {
            pad::PressButtons(kPadLauncher, kM320Toggle);
            g_m320BackPressAt = now;
            Log("Input: toggled back to the rifle again: the grenade launcher's state %d", ammo::M320State());
        }
        break;
    }
}

// Crouch and prone on the right stick (the user, 2026-11-05; B's crouch taken off): "the right analog stick flicked down" crouches, "flicked downwards and held" goes prone, both
// stay ("they're also toggled"), and "to get up from crouching or proning ... flick the analog stick up". Only a push near straight down or up counts, never a turn (its sideways
// part under about a third of it): every controller has the stick, Quest's too. The game's own keys for them (its profile): X its crouch toggle, Z its prone toggle, each held a
// little (a tap of 0.1 s through its keyboard read didn't always take). The stance as the mod last put it: a sprint or a jump stands the soldier up by itself.
enum class Stance { kStand, kCrouch, kProne };
Stance g_stance = Stance::kStand;
std::atomic<bool> g_stanceProne{false};
std::atomic<ULONGLONG> g_stanceChangedAt{0};
void SetStance(Stance s) {
    if (s != g_stance)
        g_stanceChangedAt.store(GetTickCount64(), std::memory_order_relaxed);
    g_stance = s;
    g_stanceProne.store(s == Stance::kProne, std::memory_order_relaxed);
}
ULONGLONG g_stickDownSince = 0, g_stickUpSince = 0;
bool g_stickDownUsed = false;
// The game's two toggles as the mod last put them (seen 2026-11-05: X crouches and stays so after it's let go, Z goes prone and stays; out of prone by Z the soldier's back to what
// X last left). A key at a time, held 0.3 s (as the weapon picks); a second one waits for the first to be let go.
bool g_crouchOn = false;
WORD g_stanceKey = 0, g_stanceNext = 0;
ULONGLONG g_stanceKeyUntil = 0, g_stanceNextAt = 0;
constexpr WORD kScanX = 0x2D, kScanZ = 0x2C;
constexpr ULONGLONG kStanceHold = 450, kStancePress = 300, kStanceGap = 150;
const char* StanceName(Stance s) {
    return s == Stance::kStand ? "standing" : s == Stance::kCrouch ? "crouched" : "prone";
}

void TapStance(WORD scan, WORD then = 0) {
    if (g_stanceKey)
        SendKey(g_stanceKey, false);
    SendKey(scan, true);
    g_stanceKey = scan;
    g_stanceKeyUntil = GetTickCount64() + kStancePress;
    g_stanceNext = then;
    g_stanceNextAt = g_stanceKeyUntil + kStanceGap;
}

void UpdateStance(XrVector2f stick) {
    const ULONGLONG now = GetTickCount64();
    if (g_stanceKey && now >= g_stanceKeyUntil) {
        SendKey(g_stanceKey, false);
        g_stanceKey = 0;
    }
    if (!g_stanceKey && g_stanceNext && now >= g_stanceNextAt) {
        const WORD next = g_stanceNext;
        TapStance(next);
    }
    // A sprint stands the soldier up, both toggles off.
    if (now - g_sprintAt.load(std::memory_order_relaxed) < 200) {
        g_crouchOn = false;
        SetStance(Stance::kStand);
    }
    const bool straight = std::fabs(stick.x) < 0.35f * std::fabs(stick.y);
    const bool down = stick.y < -0.8f && straight, up = stick.y > 0.8f && straight;
    if (down && !g_stickDownSince) {
        g_stickDownSince = now;
        g_stickDownUsed = false;
    }
    // Held down: prone.
    if (g_stickDownSince && !g_stickDownUsed && down && now - g_stickDownSince >= kStanceHold) {
        g_stickDownUsed = true;
        if (g_stance != Stance::kProne) {
            TapStance(kScanZ);
            Log("Input: the right stick held down: prone (Z), from %s", StanceName(g_stance));
            SetStance(Stance::kProne);
        }
    }
    // Flicked down (let go before the hold): crouched, and it stays so (the user, 2026-11-05: "they're also toggled": only a flick up gets up). From prone: down to a crouch.
    if (g_stickDownSince && stick.y > -0.5f) {
        if (!g_stickDownUsed && g_stance != Stance::kCrouch) {
            const Stance was = g_stance;
            if (g_stance == Stance::kProne)
                TapStance(kScanZ, g_crouchOn ? 0 : kScanX);
            else
                TapStance(kScanX);
            g_crouchOn = true;
            SetStance(Stance::kCrouch);
            Log("Input: the right stick flicked down: crouched, from %s", StanceName(was));
        }
        g_stickDownSince = 0;
    }
    // Flicked up: standing, from either, at once (once a push: it's ready again once the stick's back under half way).
    if (up && !g_stickUpSince) {
        g_stickUpSince = now;
        if (g_stance != Stance::kStand) {
            const Stance was = g_stance;
            if (g_stance == Stance::kProne)
                TapStance(kScanZ, g_crouchOn ? kScanX : 0);
            else
                TapStance(kScanX);
            g_crouchOn = false;
            SetStance(Stance::kStand);
            Log("Input: the right stick flicked up: standing, from %s", StanceName(was));
        }
    }
    if (g_stickUpSince && stick.y < 0.5f)
        g_stickUpSince = 0;
}

// The game's fighting quick-time events on the grips (qte.h; the user, 2026-11-18): in a scripted moment (xr_runtime.cpp tells it each frame: a
// cutscene's camera, the game turning the look itself) a fresh squeeze of either grip holds down every input the game's prompts name, E, Space and
// the two mouse buttons (and Left Ctrl) through their own mappings in Sync (QteButton), F and Z here (QteKeys), until it's let go or the moment is over. Outside
// one, nothing. weapon.txt "qte 0" switches it off.
std::atomic<bool> g_qteOn{true}, g_qteCutscene{false}, g_qteTurn{false}, g_qteTurnArmed{false};
qte::State g_qte;
bool g_qteKeysDown = false;  // F and Z as the game was last sent them
// weapon.txt "qtetest 1.5": a test's squeeze of the left grip, let go 0.3 s and then held that long, by the same rules (only in a scripted moment),
// with nobody in the headset too (no buttons are read then: QteAlone sends all six itself, with the stand-in on and the game in front).
std::atomic<ULONGLONG> g_qteTestFrom{0}, g_qteTestUntil{0};
bool g_qteAloneDown = false;  // E, Space and the mouse buttons as QteAlone last sent them

void QteKeys(bool down) {
    if (down == g_qteKeysDown)
        return;
    SendKey(kScanF, down);
    SendKey(kScanZ, down);
    g_qteKeysDown = down;
}

void QteSay(const qte::Out& o) {
    if (o.ignored)
        Log("QTE: a grip squeezed while the game turns the look itself, which began gun in hand: not taken for a quick-time event (nothing pressed)");
    if (o.began)
        Log("QTE: the %s grip squeezed in a scripted moment (%s): E, F, Space, Z, Left Ctrl and both mouse buttons held down", o.hand == 0 ? "left" : "right",
            o.cutscene ? "a cutscene's camera" : "the game turning the look itself");
    if (o.ended)
        Log("QTE: all of them let go after %.2f s (%s)", static_cast<double>(o.heldMs) / 1000.0, qte::Word(o.why));
}

bool QteTesting(ULONGLONG now) {
    return now < g_qteTestUntil.load(std::memory_order_relaxed);
}

// Sync's, with the game in front, each frame before the buttons: whether the six are to be down.
bool QteStep() {
    qte::In in;
    in.nowMs = GetTickCount64();
    in.on = g_qteOn.load(std::memory_order_relaxed);
    in.front = true;
    in.cutscene = g_qteCutscene.load(std::memory_order_relaxed);
    in.turn = g_qteTurn.load(std::memory_order_relaxed);
    in.turnArmed = g_qteTurnArmed.load(std::memory_order_relaxed);
    in.read[0] = g_squeezeRead[0], in.read[1] = g_squeezeRead[1];
    in.squeeze[0] = g_leftSqueeze, in.squeeze[1] = g_rightSqueeze;
    if (QteTesting(in.nowMs)) {  // (the test's squeeze as the left grip's)
        in.read[0] = true;
        in.squeeze[0] = in.nowMs >= g_qteTestFrom.load(std::memory_order_relaxed) && in.nowMs + 200 < g_qteTestUntil.load(std::memory_order_relaxed) ? 1.0f : 0.0f;  // (let go 0.2 s at its end)
    }
    const qte::Out o = qte::Step(g_qte, in);
    QteSay(o);
    return o.held;
}

// The mappings whose key or button a prompt may name: E (use), Space (jump), the left and the right mouse button, and Left Ctrl (crouch held: the game's
// own list, bf3.exe's EIAQuicktime actions, has a "CrouchDuck" beside InteractDrag, Fire, Block, FastMelee and JumpClimb, and which key it is isn't seen yet).
bool QteButton(const Mapping& m) {
    return m.action == &g_use || m.action == &g_jump || m.action == &g_fire || m.action == &g_ads || m.action == &g_crouch;
}

// The test's squeeze with nobody in the headset (Sync's not-focused branch, after its ReleaseAll, which leaves it be meanwhile): all six sent here.
bool QteAloneOn() {
    return QteTesting(GetTickCount64()) && g_standIn.load(std::memory_order_relaxed) && GameHasFocus();
}

void QteAloneSend(bool down) {
    QteKeys(down);
    if (down == g_qteAloneDown)
        return;
    SendKey(kScanE, down);
    SendKey(kScanSpace, down);
    SendKey(kScanCtrl, down);
    SendMouseButton(true, down);
    SendMouseButton(false, down);
    g_qteAloneDown = down;
}

void QteAlone() {
    if (QteAloneOn())
        QteAloneSend(QteStep());
}

// keepFire: the fire button left as it is, for a film round's trigger with nobody in the headset (FireAlone works it then). keepX: X's
// press, R and the D-pad's right left as they are, for a test round's D-pad press then (Sync works them).
void ReleaseAll(bool keepCrouch = false, bool keepFire = false, bool keepX = false) {
    // A pick keeps its time with the game in front (the headset off, this runs every frame: the grenade launcher's way back, a frame long, was never taken).
    if (g_selectScan && (!GameHasFocus() || GetTickCount64() >= g_selectUntil)) {
        SendKey(g_selectScan, false);
        g_selectScan = 0;
    }
    if (g_m320Key && (!GameHasFocus() || GetTickCount64() >= g_m320KeyUntil)) {
        SendKey(g_m320Key, false);
        g_m320Key = 0;
    }
    if (g_stanceKey) {
        SendKey(g_stanceKey, false);
        g_stanceKey = 0;
    }
    g_stanceNext = 0;
    g_stickDownSince = g_stickUpSince = 0;
    g_useSince = 0;
    // A quick-time event's press (qte.h): over with everything else (its four mapped ones go with the mappings below; with keepFire, a film's, none is down).
    const bool qteWas = g_qte.held && !g_qteAloneDown;  // (the fire button down is its own then, not a film's: let go whatever keepFire says)
    if (!QteAloneOn()) {
        QteSay(qte::Stop(g_qte, GetTickCount64(), qte::End::kFront));
        QteAloneSend(false);
    }
    for (Mapping& m : g_buttons)
        if (m.down && !(keepCrouch && m.action == &g_crouch) && !(keepFire && m.action == &g_fire && !qteWas))
            Press(m, false), m.down = false;
    for (StickKey& k : g_moveKeys)
        if (k.down)
            SendKey(k.scan, false), k.down = false;
    if (!keepX) {
        PressR(false);
        g_xDown = g_xHeld = false;
        g_eject = ejectkey::State{};  // (as X: still held when it comes back, a new press, judged afresh)
    }
    g_stickTurn = 0.0f;
}

// The mod holds that mouse button down: sent down (g_mouseDown), a mapping pressing it (a trigger's, a film round's: FireAlone; the zoom's), the grenade
// launcher's shot; the right also while weapon.txt "zoom 1" or the launcher's zoom test holds it.
bool ModHolds(bool left) {
    if (g_mouseDown[left ? 0 : 1])
        return true;
    for (const Mapping& m : g_buttons)
        if (m.down && m.output == (left ? Output::LeftMouse : Output::RightMouse))
            return true;
    if (left)
        return g_m320Shot == M320Shot::kFiring;
    return g_holdZoom.load(std::memory_order_relaxed) || skin::LauncherZoomWanted();
}

// A lone release of each mouse button owed to the game (inputrules.h's StepRelease): at every new soldier (god::SoldierSerial), whenever bf3 comes back
// to the front, and after a release of the mod's own that Windows refused; sent once bf3 is in front with the mod's own button up and no mouse button
// held by anyone else, and again 0.25, 0.5, 1, 2 and 4 s after it while that holds; never the game brought to the front for it. Sync's first step each
// frame, and LetGo's while Sync doesn't run. Its first and its last said, at most one line a second (bf3 going in and out of the front could repeat
// them), the rest counted into the next; the sends between not said. One Windows refused is tried again a second later.
inputrules::ReleaseState g_release;
ULONGLONG g_releaseSaidAt = 0;
int g_releaseUnsaid = 0;
int g_releaseSends[2] = {};          // each button's sends since its first
ULONGLONG g_refusedAgainAt[2] = {};  // a lone release Windows refused: tried again from then
void KeepButtonsUp() {
    const ULONGLONG now = GetTickCount64();
    inputrules::ReleaseIn in;
    in.nowMs = now;
    in.soldier = god::SoldierSerial();
    in.front = GameHasFocus();
    in.modHolds[inputrules::kLeft] = ModHolds(true);
    in.modHolds[inputrules::kRight] = ModHolds(false);
    // (A button Windows has down from a press of the mod's whose release it refused is the mod's own, not a real one held: its release goes all the same.)
    const bool down[2] = {(GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0, (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0};
    for (int b = 0; b < 2; ++b) {
        in.refused[b] = g_upRefused[b] && now >= g_refusedAgainAt[b];
        in.otherHeld = in.otherHeld || (down[b] && !in.modHolds[b] && !g_upRefused[b]);
    }
    const inputrules::ReleaseOut out = inputrules::StepRelease(g_release, in);
    // (This frame's sends by what they say: [0] a first, [1] the last of the sends after it, [2] one Windows refused; each a line naming its buttons.)
    bool kind[3][2] = {};
    for (int b = 0; b < 2; ++b) {
        if (!out.send[b])
            continue;
        const bool went = SendLoneRelease(b == inputrules::kLeft);
        g_upRefused[b] = !went;
        if (!went)
            g_refusedAgainAt[b] = now + 1000;
        g_releaseSends[b] = out.again[b] ? g_releaseSends[b] + 1 : 1;
        if (!went)
            kind[2][b] = true;
        else if (!out.again[b])
            kind[0][b] = true;
        else if (out.last[b])
            kind[1][b] = true;  // (the sends between the first and the last: not said)
    }
    for (int k = 0; k < 3; ++k) {
        if (!kind[k][0] && !kind[k][1])
            continue;
        if (g_releaseSaidAt && now - g_releaseSaidAt < 1000) {
            ++g_releaseUnsaid;
            continue;
        }
        const int b = kind[k][0] ? inputrules::kLeft : inputrules::kRight;
        const char* buttons = kind[k][0] && kind[k][1] ? "the fire and the right buttons" : kind[k][0] ? "the fire button" : "the right button";
        const char* why = out.why[b] == inputrules::Why::kSoldier   ? "a new soldier"
                          : out.why[b] == inputrules::Why::kRefused ? "the mod's own release refused by Windows"
                                                                     : "bf3 back in front";
        char more[64] = "";
        if (g_releaseUnsaid > 0)
            snprintf(more, sizeof(more), "; %d more since the last said", g_releaseUnsaid);
        g_releaseSaidAt = now;
        g_releaseUnsaid = 0;
        if (k == 0)
            Log("Input: a lone release of %s sent to the game (%s; the mod's own up), and again 0.25 to 4 s after it while that holds%s", buttons, why, more);
        else if (k == 1)
            Log("Input: the lone releases of %s done: %d sent in %.1f s (%s)%s", buttons, g_releaseSends[b],
                static_cast<double>(now - g_release.firstAt[b]) / 1000.0, why, more);
        else
            Log("Input: a lone release of %s refused by Windows (SendInput took nothing; %s): tried again in a second%s", buttons, why, more);
    }
}

// The gun doesn't fire with its magazine out, nor its last round: run dry, the game would reload
// by itself. (Guns loaded a round at a time, like the SMAW, aren't stopped.) Says why, or null.
const char* FireBlocked() {
    // The launcher loaded by hand (rocket.cpp): no rocket in it. The game's count never runs out, so it wouldn't stop it.
    if (hands::LauncherEmpty())
        return "the launcher is empty";
    // The Stinger reloaded by hand (flap.cpp): no missile ready until its aerial's been lifted.
    if (hands::StingerEmpty())
        return "the Stinger: lift its aerial";
    // A tube shotgun with its shells by hand (shellload.h): its bolt handle held back (out of battery), or nothing to fire (no live shell chambered, or
    // its bolt held open). The game's count keeps its hidden round, which must never fire.
    if (hands::TubeBoltHeld())
        return "the shotgun's bolt handle is held back";
    if (hands::TubeDry())
        return "the shotgun has nothing to fire: a shell in, and the bolt home";
    // A gun with no magazine button: as the rifle, with its own empty chamber.
    if (skin::HeldByHand()) {
        if (skin::MagazineOut())
            return "the magazine is out";
        if (hands::HeldChamberEmpty())
            return "the chamber is empty: pull the lever";
        int loaded = 0, spare = 0;
        if (ammo::Get(loaded, spare) && loaded <= 1)
            return "the magazine is empty";
        return nullptr;
    }
    if (!skin::RifleInHand())
        return nullptr;
    if (skin::MagazineOut())
        return "the magazine is out";
    if (reload::BoltBack())  // charge it first
        return "the bolt is locked back";
    int loaded = 0, spare = 0;
    if (ammo::MagazineSize() >= 5 && ammo::Get(loaded, spare) && loaded <= 1)
        return "the magazine is empty";
    return nullptr;
}
bool g_fireToldWhy = false;  // logged once a pull
// The train pistol's slide held back by the left hand (hands::PistolSlideHeld, build\pistol.txt "slide 1"): the pistol
// is out of battery, so a pull then does nothing, and one held on as the slide's let go still doesn't fire until it's
// let go (below 0.4) and pulled again (a real pistol's trigger has to reset). Its magazine out of the grip in the
// rehearsal (hands::PistolMagazineOut, "mag 1" with "rounds 0": the rounds don't change) holds it off the same way; with
// its rounds by hand an empty chamber does (hands::PistolDry: that pull sends the slide home off its stop, or does
// nothing; the game's count still has its hidden round, which must never fire). Only ever set with the train pistol in
// hand.
bool g_fireHeldOff = false;
// Free hands: whose trigger fired the rifle last frame (hands::FireHand: 0 left, 1 right, -1 none), so a change holds it off the same
// way until the new trigger's let go; and the pull with no hand on the pistol grip, said once.
int g_fireHandWas = 1;
bool g_noFireHandTold = false;

// The game's fire button (g_fire's mapping, m): rightDown as that mapping's own read of the right trigger has it (its hysteresis), leftPull and
// rightPull how far each trigger is pulled (0-1); whether the button's to be down. Sync's buttons give it the controllers' triggers; FireAlone a
// film round's, with nobody in the headset.
bool FireButton(const Mapping& m, bool rightDown, float leftPull, float rightPull) {
    if (g_m320Shot != M320Shot::kIdle) {
        // A grenade on its way (UpdateM320Shot): the game's fire button is the launcher's for that moment, the right trigger waits (its pull fires the rifle as soon as
        // the rifle's back).
        return g_m320Shot == M320Shot::kFiring;
    }
    bool down = rightDown;
    // Free hands (hands::FireHand, a frame old as every hands:: flag here): the trigger of the hand on the rifle's pistol grip
    // fires it, the left's with the same hysteresis and still the game's left mouse button; with no hand on the pistol grip
    // (the rifle on the ground, flying into a hand, held by its handguard alone) no trigger does. A trigger held as its hand
    // becomes the one that fires waits to be let go, as below. Dormant it's the right: all of this then does nothing.
    const int fireHand = hands::FireHand();
    if (fireHand == 0)
        down = leftPull > (m.down ? 0.4f : 0.6f);
    const float pull = fireHand == 0 ? leftPull : rightPull;
    if (fireHand < 0 && down) {
        if (!g_noFireHandTold)
            Log("Input: no hand on the rifle's pistol grip: the trigger doesn't fire");
        g_noFireHandTold = true;
        down = false;
    } else if (pull < 0.4f) {
        g_noFireHandTold = false;
    }
    if (fireHand != g_fireHandWas) {
        if (pull >= 0.4f)
            g_fireHeldOff = true;
        g_fireHandWas = fireHand;
        if (fireHand < 0)
            Log("Input: no trigger fires the rifle now: no hand on its pistol grip");
        else
            Log("Input: the %s trigger fires the rifle now", fireHand == 0 ? "left" : "right");
    }
    // Held off until the trigger's really let go, by its raw pull: while it's held off the button isn't down,
    // so down is the press past 0.6, and a trigger resting near 0.6 would end it with a dip and fire as it
    // came back over (the shot right after a rack).
    if (pull < 0.4f)
        g_fireHeldOff = false;
    if (down && !g_fireHeldOff && hands::PistolSlideHeld()) {
        g_fireHeldOff = true;
        Log("Input: the trigger didn't fire: the pistol's slide is held back by the left hand");
    } else if (down && !g_fireHeldOff && hands::PistolMagazineOut()) {
        g_fireHeldOff = true;
        Log("Input: the trigger didn't fire: the pistol's magazine is out (pistol.txt mag 1: a rehearsal, no "
            "rounds change)");
    } else if (down && !g_fireHeldOff && hands::PistolDry()) {
        g_fireHeldOff = true;
        hands::PistolDryPull();
        Log("Input: the trigger didn't fire: nothing in the pistol's chamber (its rounds by hand)");
    } else if (down && !g_fireHeldOff && hands::TubeInHand() && ammo::TubeHiddenOut() && ammo::TubeCount() <= 1 && !hands::TubeDry()) {
        // The shotgun's last shell with its hidden round out of the game's count (its own reload asked for with X): fired so, the game's count hit 0
        // and it began its own reload before the mod's next look. The pull ends the asking and the shot waits, a frame or two, until that round is
        // back in (not held off: it fires as soon as it is, the trigger still pulled).
        static ULONGLONG s_saidAt = 0;
        ammo::TubeEndAsk();
        if (const ULONGLONG at = GetTickCount64(); at - s_saidAt > 1000) {
            s_saidAt = at;
            Log("Input: the trigger waits a frame: the shotgun's last shell with its hidden round out (the game's own reload asked for)");
        }
        down = false;
    } else if (down && !g_fireHeldOff && hands::TubeBoltHeld()) {
        // (Held off until the trigger's let go, as the pistol's slide: a trigger held as the handle's let go doesn't fire.)
        g_fireHeldOff = true;
        Log("Input: the trigger didn't fire: the shotgun's bolt handle is held back");
    } else if (down && !g_fireHeldOff && hands::TubeDry()) {
        g_fireHeldOff = true;
        hands::TubeDryPull();
        Log("Input: the trigger didn't fire: nothing to fire in the shotgun (its shells by hand)");
    } else if (down && !g_fireHeldOff && hands::LauncherEmpty()) {
        // Held off until it's let go: held through the reload, the new rocket left as it locked, with no new pull.
        g_fireHeldOff = true;
    }
    if (g_fireHeldOff)
        down = false;
    const char* why = down ? FireBlocked() : nullptr;
    if (why && !g_fireToldWhy) {
        int loaded = 0, spare = 0;
        ammo::Get(loaded, spare);
        Log("Input: the trigger didn't fire: %s (%d rounds loaded, %d spare)", why, loaded, spare);
    }
    g_fireToldWhy = why != nullptr;
    if (why)
        down = false;
    return down;
}

XrVector2f Stick(XrSession session, XrAction action) {
    XrActionStateGetInfo get{XR_TYPE_ACTION_STATE_GET_INFO};
    get.action = action;
    XrActionStateVector2f state{XR_TYPE_ACTION_STATE_VECTOR2F};
    if (XR_FAILED(xrGetActionStateVector2f(session, &get, &state)) || !state.isActive)
        return {0.0f, 0.0f};
    return state.currentState;
}

// The buzz (Buzz). One pulse as it's sent: amplitude 0-1, seconds, hertz (0 the runtime's own).
struct Pulse {
    float amplitude = 0.0f, seconds = 0.0f, hertz = 0.0f;
};
constexpr float kBuzzLongest = 1.0f;     // seconds: a number gone wrong never leaves a hand buzzing
constexpr float kBuzzHighest = 1000.0f;  // hertz
// The call's cost (up to 30 a second in automatic fire, on two hands), timed on the render thread for its first kBuzzTimed calls: their
// median over kBuzzSlowMs, or a later call over kBuzzHitchMs kBuzzHitches times, and the pulses go out from a worker from then on.
constexpr int kBuzzTimed = 8, kBuzzHitches = 3;
constexpr double kBuzzSlowMs = 0.1, kBuzzHitchMs = 1.0;
double g_buzzTimes[kBuzzTimed] = {};
int g_buzzTimedCount = 0, g_buzzHitchCount = 0;
bool g_buzzFromWorker = false, g_buzzWorkerFailed = false;
// The worker's mailbox: the latest pulse asked for each hand (a new one replaces one not yet sent, as the runtime's own rule does one
// still going) and the event that wakes it; and the results already said (each kind once). All under the one lock.
SRWLOCK g_buzzLock = SRWLOCK_INIT;
Pulse g_buzzNext[2];
bool g_buzzWaiting[2] = {};
HANDLE g_buzzWake = nullptr;
constexpr int kBuzzSaidMost = 8;
XrResult g_buzzSaid[kBuzzSaidMost] = {};
int g_buzzSaidCount = 0;
// Each hand's pulses asked for (Buzz) and sent on (to the runtime, or to the worker's mailbox), said once a second while any were asked (BuzzCount):
// with nobody in the headset none goes out, and the count shows what a gun asks for all the same. Why the last one asked wasn't sent (-1 all were):
// 0 the controllers not ready, 1 the session not focused, 2 that hand's profile without the haptics, 3 the haptics' actions not made.
std::atomic<unsigned> g_buzzAsked[2] = {}, g_buzzSent[2] = {};
std::atomic<int> g_buzzWhyNot{-1};
std::atomic<ULONGLONG> g_buzzCountFrom{0};

void BuzzCount() {
    const ULONGLONG now = GetTickCount64();
    ULONGLONG from = g_buzzCountFrom.load(std::memory_order_relaxed);
    if (!from) {
        g_buzzCountFrom.compare_exchange_strong(from, now);
        return;
    }
    if (now - from < 1000 || !g_buzzCountFrom.compare_exchange_strong(from, now))
        return;
    unsigned asked[2], sent[2];
    for (int side = 0; side < 2; ++side) {
        asked[side] = g_buzzAsked[side].exchange(0, std::memory_order_relaxed);
        sent[side] = g_buzzSent[side].exchange(0, std::memory_order_relaxed);
    }
    const int why = g_buzzWhyNot.exchange(-1, std::memory_order_relaxed);
    if (asked[0] + asked[1] == 0)
        return;
    static const char* const kWhyNot[] = {"the controllers not ready", "the session not focused: nobody in the headset", "that hand's profile without the haptics",
                                          "the haptics' actions not made"};
    Log("Controllers: buzz pulses in the last %.1f s: the left %u asked for, %u sent; the right %u asked for, %u sent%s%s%s", (now - from) / 1000.0, asked[0], sent[0],
        asked[1], sent[1], why >= 0 && why < 4 ? " (not sent: " : "", why >= 0 && why < 4 ? kWhyNot[why] : "", why >= 0 && why < 4 ? ")" : "");
}

// One pulse into that hand's controller (the render thread's Buzz, or the worker): every kind of result but plain success said the first time.
XrResult SendBuzz(int side, const Pulse& p) {
    XrHapticActionInfo info{XR_TYPE_HAPTIC_ACTION_INFO};
    info.action = side ? g_hapticRight : g_hapticLeft;
    info.subactionPath = XR_NULL_PATH;
    XrHapticVibration vibration{XR_TYPE_HAPTIC_VIBRATION};
    vibration.duration = static_cast<XrDuration>(std::llround(static_cast<double>(p.seconds) * 1e9));  // nanoseconds
    vibration.frequency = p.hertz > 0.0f ? p.hertz : XR_FREQUENCY_UNSPECIFIED;
    vibration.amplitude = p.amplitude;
    const XrResult result = xrApplyHapticFeedback(g_session, &info, reinterpret_cast<const XrHapticBaseHeader*>(&vibration));
    if (result != XR_SUCCESS) {
        bool say = true;
        AcquireSRWLockExclusive(&g_buzzLock);
        for (int i = 0; i < g_buzzSaidCount; ++i)
            if (g_buzzSaid[i] == result)
                say = false;
        if (say && g_buzzSaidCount < kBuzzSaidMost)
            g_buzzSaid[g_buzzSaidCount++] = result;
        else
            say = false;
        ReleaseSRWLockExclusive(&g_buzzLock);
        if (say) {
            char name[XR_MAX_RESULT_STRING_SIZE] = "?";
            xrResultToString(g_instance, result, name);
            Log("Controllers: a buzz in the %s hand gave %s (said once)", side ? "right" : "left", name);
        }
    }
    return result;
}

DWORD WINAPI BuzzThread(LPVOID) {
    for (;;) {
        WaitForSingleObject(g_buzzWake, INFINITE);
        for (int side = 0; side < 2; ++side) {
            AcquireSRWLockExclusive(&g_buzzLock);
            const bool waiting = g_buzzWaiting[side];
            const Pulse p = g_buzzNext[side];
            g_buzzWaiting[side] = false;
            ReleaseSRWLockExclusive(&g_buzzLock);
            if (waiting)
                SendBuzz(side, p);
        }
    }
}

// The pulses moved to the worker (the call found slow; why: how it was found so). Once a run; if the thread can't be started they stay.
void BuzzFromWorker(double ms, const char* why) {
    if (g_buzzFromWorker || g_buzzWorkerFailed)
        return;
    g_buzzWake = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    HANDLE thread = g_buzzWake ? CreateThread(nullptr, 0, BuzzThread, nullptr, 0, nullptr) : nullptr;
    if (!thread) {
        if (g_buzzWake)
            CloseHandle(g_buzzWake);
        g_buzzWake = nullptr;
        g_buzzWorkerFailed = true;
        Log("Controllers: a buzz takes %.3f ms to send (%s), and no thread could be started for them: sent as they're asked for", ms, why);
        return;
    }
    SetThreadPriority(thread, THREAD_PRIORITY_ABOVE_NORMAL);  // (a pulse a time slice late would trail the kick it goes with)
    CloseHandle(thread);
    g_buzzFromWorker = true;
    Log("Controllers: a buzz takes %.3f ms to send (%s): sent from a thread of their own from now on", ms, why);
}

// A hand's profile in use (Sync, as it changes): whether its buzz can work, said with what the runtime bound it to (a binding set by hand
// in SteamVR may leave it out; that's no fault of the game's).
void NoteBuzzProfile(int side, XrPath profile) {
    g_buzzBound[side] = BuzzTaken(profile);
    const XrAction action = side ? g_hapticRight : g_hapticLeft;
    if (profile == XR_NULL_PATH || action == XR_NULL_HANDLE)
        return;
    const char* hand = side ? "right" : "left";
    if (!g_buzzBound[side]) {
        Log("Controllers: no buzz in the %s hand: its profile's bindings didn't take the haptics", hand);
        return;
    }
    XrBoundSourcesForActionEnumerateInfo bound{XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO};
    bound.action = action;
    XrPath sources[8] = {};
    uint32_t count = 0;
    const XrResult result = xrEnumerateBoundSourcesForAction(g_session, &bound, 8, &count, sources);
    if (XR_FAILED(result)) {
        char name[XR_MAX_RESULT_STRING_SIZE] = "?";
        xrResultToString(g_instance, result, name);
        Log("Controllers: the %s hand's buzz: what it's bound to couldn't be read (%s)", hand, name);
    } else if (count == 0) {
        Log("Controllers: the %s hand's buzz is bound to nothing (a binding set in SteamVR without it?)", hand);
        static bool s_said = false;  // (the launch report says it once: the fix is the same for both hands)
        if (!s_said) {
            s_said = true;
            if (launchreport::IsVdxr())
                launchreport::Line("the controllers won't vibrate (no buzz on a shot, at a holster or a reload): Virtual Desktop's runtime (VDXR) "
                                   "gave the vibration no controller. Send this log; Start BF3 VR.bat (SteamVR) is the way meanwhile");
            else
                launchreport::Line("the controllers won't vibrate (no buzz on a shot, at a holster or a reload): SteamVR's controller binding for this "
                                   "game has no vibration in it, often one saved for another mod of this game. In SteamVR's settings, Controllers, "
                                   "Manage Controller Bindings: pick this game and choose its default binding (this mod's own), then start the game again");
        }
    } else {
        char name[XR_MAX_PATH_LENGTH] = "?";
        uint32_t length = 0;
        xrPathToString(g_instance, sources[0], sizeof(name), &length, name);
        Log("Controllers: the %s hand's buzz is bound to %s%s", hand, name, count > 1 ? " and more" : "");
    }
}

// The left hand's profile in use (Sync, as it changes): on the Frame, with the D-pad's right taken, what the runtime bound it to (a binding
// set by hand in SteamVR may leave it out).
void NoteEjectProfile(XrPath profile) {
    static const XrPath frame = Path("/interaction_profiles/valve/frame_controller");
    if (profile == XR_NULL_PATH || profile != frame || !g_ejectBound || g_ejectLeft == XR_NULL_HANDLE)
        return;
    XrBoundSourcesForActionEnumerateInfo bound{XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO};
    bound.action = g_ejectLeft;
    XrPath sources[8] = {};
    uint32_t count = 0;
    const XrResult result = xrEnumerateBoundSourcesForAction(g_session, &bound, 8, &count, sources);
    if (XR_FAILED(result)) {
        char name[XR_MAX_RESULT_STRING_SIZE] = "?";
        xrResultToString(g_instance, result, name);
        Log("Controllers: the D-pad's right: what it's bound to couldn't be read (%s)", name);
    } else if (count == 0) {
        Log("Controllers: the D-pad's right is bound to nothing (a binding set in SteamVR without it?): X alone ejects");
    } else {
        char name[XR_MAX_PATH_LENGTH] = "?";
        uint32_t length = 0;
        xrPathToString(g_instance, sources[0], sizeof(name), &length, name);
        Log("Controllers: the D-pad's right is bound to %s%s", name, count > 1 ? " and more" : "");
    }
}

// ---- The reach test's lines (tracking.h): "Tracking:", each controller's state as the runtime gives it, its changes and its grip's squeezes, a place
// said from the neck point in the body's axes (pouch::BodyFrame, as of the last frame's hands). Sync's own. ----
constexpr const char* kTrackHand[2] = {"left", "right"};

const char* StateWord(tracking::State s) {
    return s == tracking::State::kSeen      ? "seen"
           : s == tracking::State::kGuessed ? "not seen (the runtime guesses its place)"
           : s == tracking::State::kNone    ? "gives no place"
                                            : "not watched (the headset off or out of focus)";
}

// A place in the room as the lines say it: "9 cm right, 2 cm up, 11 cm behind the neck point (at the back of the neck)"; "(the body not placed yet)"
// before the pouch has placed it. False when a number isn't finite: the line isn't said.
bool PlaceText(char* out, size_t size, const float* room) {
    if (!tracking::Finite3(room))
        return false;
    float neck[3] = {}, yaw = 0.0f;
    if (!pouch::BodyFrame(neck, yaw) || !tracking::Finite3(neck) || !std::isfinite(yaw)) {
        snprintf(out, size, "(the body not placed yet)");
        return true;
    }
    float p[3];
    tracking::FromNeck(room, neck, yaw, p);
    if (!tracking::Finite3(p))
        return false;
    snprintf(out, size, "%.0f cm %s, %.0f cm %s, %.0f cm %s the neck point (%s)", std::fabs(p[0]) * 100.0f, p[0] < 0.0f ? "left" : "right",
             std::fabs(p[1]) * 100.0f, p[1] < 0.0f ? "down" : "up", std::fabs(p[2]) * 100.0f, p[2] < 0.0f ? "behind" : "in front of", tracking::Word(p));
    return true;
}

// Its speed when it was last seen: "moving 0.62 m/s (0.10 right, 0.30 up, 0.53 back)", from its own sensors or by its last two places (said so);
// "at a speed not known". False when a number isn't finite.
bool SpeedText(char* out, size_t size, const tracking::Record& r) {
    if (!r.speedKnown) {
        snprintf(out, size, "at a speed not known");
        return true;
    }
    const float* v = r.seenSpeed;
    const float speed = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (!tracking::Finite3(v) || !std::isfinite(speed))
        return false;
    const char* by = r.speedSensed ? "" : ", by its last two places";
    float neck[3] = {}, yaw = 0.0f;
    if (!pouch::BodyFrame(neck, yaw) || !std::isfinite(yaw)) {
        snprintf(out, size, "moving %.2f m/s%s", speed, by);
        return true;
    }
    float b[3];
    tracking::InBody(v, yaw, b);
    snprintf(out, size, "moving %.2f m/s (%.2f %s, %.2f %s, %.2f %s%s)", speed, std::fabs(b[0]), b[0] < 0.0f ? "left" : "right", std::fabs(b[1]),
             b[1] < 0.0f ? "down" : "up", std::fabs(b[2]), b[2] < 0.0f ? "back" : "forward", by);
    return true;
}

// Lines 2-5 and 7 of one controller this frame (tracking::Step's out).
void SayTracking(int side, const tracking::Out& o) {
    const tracking::Record& r = g_track[side];
    const char* hand = kTrackHand[side];
    char at[192], speed[128];
    switch (o.change) {
    case tracking::Change::kLost:
        if (PlaceText(at, sizeof(at), r.seenAt) && SpeedText(speed, sizeof(speed), r))
            Log("Tracking: the %s controller not seen now (%s): last seen %s, %s", hand, o.guessed ? "the runtime guesses its place" : "the runtime gives no place",
                at, speed);
        break;
    case tracking::Change::kGuessStopped:
        if (!std::isfinite(o.after) || !std::isfinite(o.guessOff))
            break;
        if (r.haveSeen) {
            if (PlaceText(at, sizeof(at), r.seenAt))
                Log("Tracking: the %s controller gives no place now (its guess stopped after %.2f s): last seen %s; its last guess %.0f cm from there", hand,
                    o.after, at, o.guessOff * 100.0f);
        } else if (PlaceText(at, sizeof(at), r.guessAt)) {
            Log("Tracking: the %s controller gives no place now (its guess stopped after %.2f s): not seen since it was watched again; its last guess at %s", hand,
                o.after, at);
        }
        break;
    case tracking::Change::kGuessedAgain:
        if (std::isfinite(o.after) && PlaceText(at, sizeof(at), r.guessAt))
            Log("Tracking: the %s controller's place guessed again after %.2f s with none, at %s", hand, o.after, at);
        break;
    case tracking::Change::kSeenAgain:
        if (!std::isfinite(o.after) || !std::isfinite(o.guessOff) || !PlaceText(at, sizeof(at), r.seenAt))
            break;
        if (o.guessBefore)
            Log("Tracking: the %s controller seen again after %.2f s, at %s; the runtime's guess there was %.0f cm off", hand, o.after, at, o.guessOff * 100.0f);
        else
            Log("Tracking: the %s controller seen again after %.2f s, at %s; no guess just before", hand, o.after, at);
        break;
    default:
        break;
    }
    if (o.summary && std::isfinite(o.summarySpan) && PlaceText(at, sizeof(at), o.summaryAt))
        Log("Tracking: the %s controller changed %d times more in %.1f s without a line, about %s; now %s", hand, o.summaryCount, o.summarySpan, at,
            o.summaryUnwatched ? StateWord(tracking::State::kNotWatched) : StateWord(r.state));
}

// Both controllers' records this frame (watched: the controllers read; not: no controllers, the headset off or out of focus, no line but a count
// owed): the raw flags and places of this frame's locate, before SteadyHand holds a hand lost for a moment. Line 1 once for both.
void NoteTracking(double t, bool watched) {
    tracking::Out out[2];
    for (int i = 0; i < 2; ++i) {
        tracking::Frame f;
        f.t = t;
        f.watched = watched;
        if (watched) {
            const XrSpaceLocationFlags flags = g_handFlags[i];
            f.positionValid = (flags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0;
            f.positionTracked = (flags & XR_SPACE_LOCATION_POSITION_TRACKED_BIT) != 0;
            f.orientationValid = (flags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0;
            f.place[0] = g_handRaw[i].x, f.place[1] = g_handRaw[i].y, f.place[2] = g_handRaw[i].z;
            f.sensed = g_handLinearValid[i];
            f.speed[0] = g_handLinear[i].x, f.speed[1] = g_handLinear[i].y, f.speed[2] = g_handLinear[i].z;
        }
        out[i] = tracking::Step(g_track[i], f);
    }
    // (Line 1 at most once in 2 s: a session going in and out of focus every frame would fill the log. The rest counted into the next.)
    static double s_watchSaidAt = -1e30;
    static int s_watchUnsaid = 0;
    if (out[0].watchedNow || out[1].watchedNow) {
        if (t - s_watchSaidAt >= 2.0 || t < s_watchSaidAt) {
            char more[64] = "";
            if (s_watchUnsaid > 0)
                snprintf(more, sizeof(more), " (%d more times since the last said)", s_watchUnsaid);
            Log("Tracking: watching the controllers%s: the left %s, the right %s%s", out[0].firstWatch || out[1].firstWatch ? "" : " again",
                StateWord(g_track[0].state), StateWord(g_track[1].state), more);
            s_watchSaidAt = t;
            s_watchUnsaid = 0;
        } else {
            ++s_watchUnsaid;
        }
    }
    for (int i = 0; i < 2; ++i)
        SayTracking(i, out[i]);
}

// A hand's grip this frame for the reach test (after its read, before the focus check): line 6 for a squeeze of a hand not seen, or seen within 0.55 m
// of the neck point (so a handguard's and a magazine well's squeezes don't fill the log); at most four a second a hand.
void NoteReachSqueeze(int side, double t) {
    float neck[3] = {}, yaw = 0.0f;
    const bool body = pouch::BodyFrame(neck, yaw);
    tracking::Record& r = g_track[side];
    const tracking::SqueezeOut s = tracking::Squeeze(r, t, g_squeezeRead[side], side == 1 ? g_rightSqueeze : g_leftSqueeze, body, neck);
    if (!s.say)
        return;
    const char* hand = kTrackHand[side];
    char more[64] = "";
    if (s.unsaidBefore > 0)
        snprintf(more, sizeof(more), " (%d more squeezes not said before it)", s.unsaidBefore);
    char at[192];
    if (r.state == tracking::State::kSeen) {
        if (PlaceText(at, sizeof(at), r.seenAt))
            Log("Tracking: the %s grip squeezed at %s: seen%s", hand, at, more);
        return;
    }
    const double since = t - (r.haveSeen ? r.seenTime : r.watchSince);
    if (!std::isfinite(since))
        return;
    char last[352] = "not since it was watched again";
    if (r.haveSeen) {
        char where[192], speed[128];
        if (!PlaceText(where, sizeof(where), r.seenAt) || !SpeedText(speed, sizeof(speed), r))
            return;
        snprintf(last, sizeof(last), "%s, %s", where, speed);
    }
    if (r.state == tracking::State::kGuessed) {
        if (PlaceText(at, sizeof(at), r.guessAt))
            Log("Tracking: the %s grip squeezed at %s: not seen for %.2f s, the runtime's guess; last seen %s%s", hand, at, since, last, more);
    } else {
        Log("Tracking: the %s grip squeezed: no place, not seen for %.2f s; last seen %s%s", hand, since, last, more);
    }
}

// The reach test's path of a hand: from a squeeze made within kPathNear of the neck point (or with the hand not seen) until the grip opens again,
// its place every kPathStep, said in one line when the grip opens (and every kPathMost samples of a long hold): where a reach went, not only where it
// squeezed (a hand on its way over a shoulder to the back of the neck meets the shoulder first). Only while no hand holds the gun, so holding the
// rifle by its pistol grip at the cheek never fills the log; at most two lines a second a hand.
struct ReachPath {
    bool on = false;
    double next = 0.0;     // the next sample's time (s)
    double from = 0.0;     // this line's first sample's
    int n = 0;
    short at[40][3] = {};  // cm right, up, forward of the neck point
    char state[40] = {};   // 's' seen, 'g' the runtime's guess, 'n' no place
    double said[2] = {-1e30, -1e30};
    int saidNext = 0;
    int unsaid = 0;
};
ReachPath g_reachPath[2];
constexpr float kPathNear = 0.5f;   // m from the neck point
constexpr double kPathStep = 0.05;  // s between two samples
constexpr int kPathMost = 40;       // samples a line (2 s)

void SayReachPath(int side, ReachPath& p, double t, bool open) {
    if (p.n == 0)
        return;
    const double last = p.said[p.saidNext];
    if (t >= last && t - last < 1.0) {  // (the third line within a second: counted into the next said)
        ++p.unsaid;
        p.n = 0;
        return;
    }
    char line[40 * 20 + 16];
    size_t used = 0;
    line[0] = '\0';
    for (int i = 0; i < p.n && used + 24 < sizeof(line); ++i) {
        const int wrote = p.state[i] == 'n' ? snprintf(line + used, sizeof(line) - used, " (none)")
                                            : snprintf(line + used, sizeof(line) - used, " (%d %d %d%s)", p.at[i][0], p.at[i][1], p.at[i][2],
                                                       p.state[i] == 'g' ? " g" : "");
        if (wrote < 0)
            break;
        used += static_cast<size_t>(wrote);
    }
    char more[64] = "";
    if (p.unsaid > 0)
        snprintf(more, sizeof(more), " (%d more not said before it)", p.unsaid);
    Log("Tracking: the %s hand's path while squeezed%s, every 0.05 s from %.2f s ago, in cm right, up and forward of the neck point (g: the runtime's "
        "guess):%s%s",
        kTrackHand[side], open ? "" : " (still squeezed)", t - p.from, line, more);
    p.said[p.saidNext] = t;
    p.saidNext = (p.saidNext + 1) % 2;
    p.unsaid = 0;
    p.n = 0;
}

void NoteReachPath(int side, double t) {
    ReachPath& p = g_reachPath[side];
    const tracking::Record& r = g_track[side];
    const float grip = side == 1 ? g_rightSqueeze : g_leftSqueeze;
    const bool read = g_squeezeRead[side] && std::isfinite(grip);
    const bool watched = r.state == tracking::State::kSeen || r.state == tracking::State::kGuessed || r.state == tracking::State::kNone;
    float neck[3] = {}, yaw = 0.0f;
    const bool body = pouch::BodyFrame(neck, yaw) && tracking::Finite3(neck) && std::isfinite(yaw);
    const float* at = r.state == tracking::State::kSeen ? r.seenAt : r.state == tracking::State::kGuessed ? r.guessAt : nullptr;
    float rel[3] = {};
    bool placed = false;
    if (body && at && tracking::Finite3(at)) {
        tracking::FromNeck(at, neck, yaw, rel);
        placed = tracking::Finite3(rel);
    }
    const bool close = placed && std::sqrt(rel[0] * rel[0] + rel[1] * rel[1] + rel[2] * rel[2]) <= kPathNear;  // ("near" is a Windows macro)
    if (!p.on) {
        if (!read || !watched || !body || !(grip > tracking::kSqueeze) || hands::HoldsGun() ||
            !(close || r.state == tracking::State::kNone))
            return;
        p.on = true;
        p.n = 0;
        p.next = t;
    }
    if (!read || !watched || !(grip >= tracking::kArm) || hands::HoldsGun()) {  // (the grip open again, the headset off, the gun taken)
        SayReachPath(side, p, t, true);
        p.on = false;
        return;
    }
    if (t < p.next)
        return;
    p.next = t - p.next < kPathStep ? p.next + kPathStep : t + kPathStep;  // (steady steps; after a stall, from now)
    if (p.n == 0)
        p.from = t;
    if (placed) {
        for (int k = 0; k < 3; ++k)
            p.at[p.n][k] = static_cast<short>(std::lround(std::clamp(rel[k] * 100.0f, -999.0f, 999.0f)));
        p.state[p.n] = r.state == tracking::State::kSeen ? 's' : 'g';
    } else {
        p.state[p.n] = 'n';
    }
    if (++p.n >= kPathMost)
        SayReachPath(side, p, t, false);  // (a long hold: said in pieces, followed on)
}

}

namespace input {

bool Init(XrInstance instance, XrSession session) {
    g_instance = instance;
    g_session = session;
    XrActionSetCreateInfo si{XR_TYPE_ACTION_SET_CREATE_INFO};
    strcpy_s(si.actionSetName, "gameplay");
    strcpy_s(si.localizedActionSetName, "Gameplay");
    if (!Check(xrCreateActionSet(instance, &si, &g_set), "xrCreateActionSet"))
        return false;

    g_handPaths[0] = Path("/user/hand/left");
    g_handPaths[1] = Path("/user/hand/right");
    XrActionCreateInfo hand{XR_TYPE_ACTION_CREATE_INFO};
    hand.actionType = XR_ACTION_TYPE_POSE_INPUT;
    strcpy_s(hand.actionName, "hand");
    strcpy_s(hand.localizedActionName, "Hands");
    hand.countSubactionPaths = 2;
    hand.subactionPaths = g_handPaths;
    if (!Check(xrCreateAction(g_set, &hand, &g_hand), "hand"))
        return false;
    if (!MakeAction(XR_ACTION_TYPE_FLOAT_INPUT, "left_squeeze", "Close the left hand", g_squeeze) ||
        !MakeAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "calibrate_left", "Line up the left hand", g_calibrate) ||
        !MakeAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "left_thumb_touch", "Left thumb on the stick", g_thumbTouch) ||
        !MakeAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "recenter", "Recenter the view (hold the right stick's click)",
                    g_recenter))
        return false;
    // Free hands' right grip and thumb: nothing else needs them, so one the runtime won't make leaves the rest working (unbound, the
    // gun stays in the right hand).
    if (!MakeAction(XR_ACTION_TYPE_FLOAT_INPUT, "right_squeeze", "Close the right hand (holds the gun)", g_squeezeRight) ||
        !MakeAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "right_thumb_touch", "Right thumb on the stick", g_thumbTouchRight))
        g_squeezeRight = g_thumbTouchRight = XR_NULL_HANDLE;
    // The D-pad's right as X: the same, one not made leaves X alone as the magazine.
    if (!MakeAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "eject_left", "Magazine with the gun in the left hand (D-pad right)", g_ejectLeft))
        g_ejectLeft = XR_NULL_HANDLE;
    bool ok = MakeAction(XR_ACTION_TYPE_POSE_INPUT, "aim", "Aim (right hand)", g_aim) &&
              MakeAction(XR_ACTION_TYPE_FLOAT_INPUT, "fire", "Fire", g_fire) &&
              MakeAction(XR_ACTION_TYPE_FLOAT_INPUT, "aim_down_sights", "Aim down sights", g_ads) &&
              MakeAction(XR_ACTION_TYPE_FLOAT_INPUT, "grenade", "Grenade", g_grenade) &&
              MakeAction(XR_ACTION_TYPE_VECTOR2F_INPUT, "move", "Move", g_move) &&
              MakeAction(XR_ACTION_TYPE_VECTOR2F_INPUT, "turn", "Turn", g_turn) &&
              MakeAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "jump", "Jump", g_jump) &&
              MakeAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "crouch", "Crouch", g_crouch) &&
              MakeAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "reload", "Magazine (tap: eject / insert, hold: reload)",
                         g_reload) &&
              MakeAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "use", "Use", g_use) &&
              MakeAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "select_launcher", "Select the launcher", g_selectLauncher) &&
              MakeAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "select_rifle", "Select the rifle", g_selectRifle) &&
              MakeAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "select_swap", "Launcher or rifle", g_selectSwap) &&
              MakeAction(XR_ACTION_TYPE_BOOLEAN_INPUT, "sprint", "Sprint", g_sprint);
    if (!ok)
        return false;
    // The buzz in each hand (recoil): nothing else needs them, so ones the runtime won't make leave everything else as it was, and
    // nothing buzzes (both or neither).
    {
        XrAction left = XR_NULL_HANDLE, right = XR_NULL_HANDLE;
        const bool leftMade = MakeAction(XR_ACTION_TYPE_VIBRATION_OUTPUT, "haptic_left", "Left hand buzz", left);
        if (leftMade && MakeAction(XR_ACTION_TYPE_VIBRATION_OUTPUT, "haptic_right", "Right hand buzz", right)) {
            g_hapticLeft = left;
            g_hapticRight = right;
        } else if (leftMade) {
            xrDestroyAction(left);
        }
    }
    SuggestBindings();

    XrSessionActionSetsAttachInfo attach{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
    attach.countActionSets = 1;
    attach.actionSets = &g_set;
    if (!Check(xrAttachSessionActionSets(session, &attach), "xrAttachSessionActionSets"))
        return false;

    XrActionSpaceCreateInfo space{XR_TYPE_ACTION_SPACE_CREATE_INFO};
    space.action = g_aim;
    space.poseInActionSpace.orientation.w = 1.0f;
    if (!Check(xrCreateActionSpace(session, &space, &g_aimSpace), "xrCreateActionSpace"))
        return false;
    for (int i = 0; i < 2; ++i) {
        XrActionSpaceCreateInfo hs{XR_TYPE_ACTION_SPACE_CREATE_INFO};
        hs.action = g_hand;
        hs.subactionPath = g_handPaths[i];
        hs.poseInActionSpace.orientation.w = 1.0f;
        if (!Check(xrCreateActionSpace(session, &hs, &g_handSpaces[i]), "xrCreateActionSpace (hand)"))
            return false;
    }

    g_ready = true;
    Log("Controllers: ready");
    return true;
}

// With the headset off or out of focus no controllers are read: the body's own walk and crouch (headmove.h) still go in
// while the stand-in stands for the player (weapon.txt "standin 1"), for testing with nobody in the headset. The crouch only
// under weapon.txt's "headtest": otherwise the head's last place is all there is, and it would be held for as long as the game's
// in front, Left Ctrl down with it.
void HeadMoveAlone() {
    headmove::NoteStick(false);
    if (!g_standIn.load(std::memory_order_relaxed) || !GameHasFocus())
        return;
    for (Mapping& m : g_buttons) {
        if (m.action != &g_crouch)
            continue;
        const bool want = headmove::CrouchWanted() && headmove::TestOn();
        if (want != m.down) {
            Press(m, want);
            m.down = want;
        }
    }
    float x = 0.0f, y = 0.0f;
    headmove::FollowStick(x, y);
    pad::SetMove(x, y);
}

// So too a film round's triggers (SetFilmTrigger; pull: each given since the last Sync, -1 if not): they alone work the game's fire button, by
// the rules a controller's pull goes by (FireButton: the hand on the rifle's pistol grip fires it), the stand-in on and the game in front (Sync
// checks). Each press and let-go said with the rounds loaded, so a shot between them shows as fewer. A free hands round's pull fires real rounds only
// when they're asked for (freehands.txt "fire 1", or the pickup round's pickups.txt "fire 1"), with two hands on the gun and the head 45 degrees down
// (inputrules.h's RoundFireRefused); otherwise it presses nothing, said once a pull. weapon.txt "filmtrigger"'s pulls (its own word and its own
// check of the head) and the grenade launcher's shot go as before.
bool g_roundRefusedSaid = false;
void FireAlone(const float pull[2]) {
    for (Mapping& m : g_buttons) {
        if (m.action != &g_fire)
            continue;
        const float left = std::max(pull[0], 0.0f), right = std::max(pull[1], 0.0f);
        bool down = FireButton(m, right > (m.down ? 0.4f : 0.6f), left, right);
        if (down && g_m320Shot == M320Shot::kIdle && g_filmPullHand.load(std::memory_order_relaxed) < 0) {
            const bool asked = freetune::RoundsFire() || (freetune::Test() == 10 && pickups::Tuning().testFire);
            if (const char* why = inputrules::RoundFireRefused(asked, hands::TwoHands(), g_standInPitch.load(std::memory_order_relaxed))) {
                if (!g_roundRefusedSaid) {
                    int loaded = 0, spare = 0;
                    ammo::Get(loaded, spare);
                    Log("Input: a film round's %s trigger: no real round (%s; %d rounds loaded)", hands::FireHand() == 0 ? "left" : "right", why, loaded);
                }
                g_roundRefusedSaid = true;
                down = false;
            }
        }
        if (left < 0.4f && right < 0.4f)
            g_roundRefusedSaid = false;  // (let go: the next pull said again)
        if (down == m.down)
            continue;
        Press(m, down);
        m.down = down;
        if (g_m320Shot != M320Shot::kIdle)
            continue;  // (the grenade launcher's shot: its own lines)
        int loaded = 0, spare = 0;
        ammo::Get(loaded, spare);
        if (down)
            Log("Input: a film round's %s trigger: the fire button down (%d rounds loaded)", hands::FireHand() == 0 ? "left" : "right", loaded);
        else
            Log("Input: a film round's trigger let go: the fire button up (%d rounds loaded)", loaded);
    }
}

// The Frame's left D-pad right held this frame (dpad: the controller's, or a test round's, hands::ScriptEject), X as read (x): X itself
// while it counts (ejectkey.h), so everything in XButton is X's own, tap, hold and let-go alike; pressed with X, the two are one press.
// It counts with the gun in the left hand by its pistol grip as it goes down ("dpadeject 1", the default), whoever holds it ("dpadeject
// 2"), or never ("dpadeject 0"). Gives X as XButton takes it.
bool DpadAsX(bool dpad, bool x, ULONGLONG now) {
    const int mode = freetune::DpadEject();
    const int gripHand = hands::GripHand();
    const ejectkey::Out e = ejectkey::Step(g_eject, dpad, mode, gripHand);
    if (e.edge) {
        g_ejectSince = now;
        if (e.counts)
            Log("Input: D-pad right: X's job (%s)", mode == ejectkey::kAnyHold ? "dpadeject 2: whoever holds the gun" : "the gun in the left hand by its pistol grip");
        else
            Log("Input: D-pad right: nothing (%s)", mode == ejectkey::kOff ? "dpadeject 0"
                                                    : gripHand == 1 ? "the gun is in the right hand"
                                                                    : "nobody holds the gun by its pistol grip");
    }
    if (e.letGo && e.counts)  // (as XButton takes it: its hold made or not; X itself still down, the press goes on)
        Log("Input: D-pad right let go after %llu ms: %s", now - g_ejectSince, x ? "X still held: one press" : g_xHeld ? "held (X's hold)" : "a tap");
    return x || e.press;
}

// X held this frame (x, with the D-pad's right in it): its tap, hold and let-go (Sync; and its not-focused branch, for a test round's D-pad press).
void XButton(bool x, ULONGLONG now) {
    if (x && !g_xDown) {
        g_xSince = now;
        g_xHeld = false;
    }
    // Free hands: nobody holds the rifle (on the ground, or flying into a hand): a tap of X does nothing to it, said once a press. R still
    // down for a reload (X held as it left the hand) is let go, and that press counts as held, so letting it go taps nothing either. X held
    // afresh is the game's own pickup alone (R held, its "HOLD [R]": a gun within 2.5 m is taken into the hand and the rifle goes into its
    // place), with none of the reload's side effects (the user, 2026-11-06: an enemy's gun off the floor after throwing their own; with no
    // gun near, the game reloads the rifle where it lies).
    // Holsters: the gun put away at its place (hands::GunStowed) has its own words, so "the rifle is on the ground" still means a lying rifle; and X
    // held sends R only when the game's own target is a dropped gun (GameWouldTake 1): with that not known (-1), R could be the game's reload of a
    // gun drawn nowhere. The pickup is a pick of the user's (LastPickAt, at the end): the gun it brings comes into the right hand.
    if (!hands::HoldsGun()) {
        const bool stowed = hands::GunStowed();
        char where[128] = "";
        if (x && !g_xDown) {
            if (stowed) {
                StowedWords(where, sizeof(where));
                Log("Input: X: nothing in the hands (%s): a tap does nothing; held, the game's own pickup (R) of a gun near", where);
            } else {
                Log("Input: X: the rifle is on the ground: a tap does nothing; held, the game's own pickup (R)");
            }
        }
        if (g_rDown && !g_rForPickup) {
            PressR(false);
            if (x)
                g_xHeld = true;
        }
        if (x && !g_xHeld && now - g_xSince >= kHoldForReload) {
            g_xHeld = true;
            const int take = pickups::GameWouldTake();  // 1: the game's own target is a dropped gun; -1 not known (pickups off): as before, unless put away
            if (stowed ? take == 1 : take != 0) {
                PressR(true);  // until X is let go
                g_rForPickup = true;
                if (stowed) {
                    StowedWords(where, sizeof(where));
                    Log("Input: X held with nothing in the hands (%s): R held, the game's own pickup of a gun near", where);
                } else {
                    Log("Input: X held with the rifle on the ground: R held, the game's own pickup of a gun near");
                }
            } else if (stowed) {
                StowedWords(where, sizeof(where));
                Log("Input: X held with nothing in the hands (%s): %s: nothing sent (R would be the game's reload of the gun put away)", where,
                    take < 0 ? "what the game would take isn't known (pickups off, not hooked, or its tick stale)" : "nothing the game would take");
            } else {
                Log("Input: X held with the rifle on the ground: nothing the game would take: nothing sent (R would reload the rifle where it lies)");
            }
        }
    }
    if (x && !g_xHeld && now - g_xSince >= kHoldForReload && hands::M320Held() && ammo::M320Carried()) {
        // With a hand on the grenade launcher's grip (either: hands::M320Held), X held is its own game reload (the rifle's magazine stays as it is).
        g_xHeld = true;
        if (g_m320Shot == M320Shot::kIdle && ammo::M320Loaded() < 1 && ammo::M320Spare() >= 1)
            StartM320(true);
        else
            Log("Input: X held on the grenade launcher's grip: %s", ammo::M320Loaded() >= 1 ? "it's loaded" : "no rounds left");
    } else if (x && !g_xHeld && now - g_xSince >= kHoldForReload && hands::TubeInHand()) {
        // A tube shotgun with its shells by hand: the game's own reload asked for (ammo.cpp takes its hidden round out of the game's count at its next
        // look), and R a moment later, held until X is let go: sent at once, a gun one shell short of full still looked full to the game.
        g_xHeld = true;
        ammo::TubeAsk(true);
        g_tubeRAt = now + kTubeRDelay;
        Log("Input: X held with the shotgun: the game's own reload asked for (R in %llu ms, once its hidden round is out of the game's count)",
            static_cast<unsigned long long>(kTubeRDelay));
    } else if (x && !g_xHeld && now - g_xSince >= kHoldForReload) {
        g_xHeld = true;
        PressR(true);  // until X is let go
        // The magazine put in, the rifle's bolt home and a held gun's chamber wait for the game's count of that gun to rise (ammo::HeldPress): nothing of
        // the reload is drawn at the hold itself, as the game may refuse it (Operation Swordbreaker's opening: a seated magazine drawn in a gun that held
        // one round).
        ammo::HeldPress();
        // The train pistol's too, with its magazine by hand (build\pistol.txt "mag 1"): in the rehearsal ("rounds 0") its rounds never left the game's
        // count, so its magazine back in the grip shows nothing the game doesn't hold; with its rounds by hand it goes back in once the game's count
        // rises (ammo.cpp's pistol keeps a reload asked for).
        hands::PistolGameReload();
        // The launcher's: the game's own reload, if it's empty (rocket.cpp); its rocket is drawn by the game's own count.
        hands::LauncherGameReload();
    }
    if (g_tubeRAt && x && g_xHeld && now >= g_tubeRAt) {
        g_tubeRAt = 0;
        PressR(true);  // (the shotgun's: until X is let go)
    }
    if (!x && g_xDown) {
        if (g_xHeld) {
            PressR(false);
            if (g_tubeRAt || ammo::TubeLive())
                ammo::TubeAsk(false);
            g_tubeRAt = 0;
        } else {
            input::MagazineTap();
        }
    }
    g_xDown = x;
    if (g_rDown && !g_xHeld && now >= g_rUntil)
        PressR(false);
    // X held's R for the game's own pickup: a pick of the user's on every frame it's held (LastPickAt, slot -1), so the gun it brings, however long
    // the game takes, comes into the right hand with the gun put away.
    if (g_rDown && g_rForPickup)
        NotePick(-1);
}

void Sync(XrSession session, XrSpace space, XrTime time) {
    g_aimTracked = false;
    KeepButtonsUp();  // (a release the game never saw: a lone one at a new soldier and as bf3 comes back to the front)
    FilmPullStep();  // (weapon.txt "filmtrigger": given as a film round's, below)
    BuzzCount();     // (the pulses' line once a second, whether or not one comes)
    const double seconds = static_cast<double>(time) * 1e-9;  // (the controllers' tracking record's clock)
    // A film round's triggers given since the last Sync (FireAlone), -1 for none.
    float filmPull[2];
    for (int i = 0; i < 2; ++i)
        filmPull[i] = g_filmTriggerGiven[i].exchange(false, std::memory_order_relaxed) ? g_filmTrigger[i].load(std::memory_order_relaxed) : -1.0f;
    if (!g_ready) {
        // No controllers (Init failed): nothing tracked this frame; the stand-in may hold the gun.
        g_handTracked[0] = g_handTracked[1] = false;
        g_squeezeRead[0] = g_squeezeRead[1] = false;
        g_standingIn = false;
        NoteTracking(seconds, false);  // (not watched: no line)
        return;
    }
    XrActiveActionSet active{g_set, XR_NULL_PATH};
    XrActionsSyncInfo sync{XR_TYPE_ACTIONS_SYNC_INFO};
    sync.countActiveActionSets = 1;
    sync.activeActionSets = &active;
    if (xrSyncActions(session, &sync) != XR_SUCCESS) {  // also XR_SESSION_NOT_FOCUSED
        g_focused = false;  // (no buzz meanwhile)
        // (Nobody in the headset, as in every film: the buttons below never run, so a film round's trigger works the fire button here.)
        const bool fireAlone =
            (filmPull[0] >= 0.0f || filmPull[1] >= 0.0f) && g_standIn.load(std::memory_order_relaxed) && GameHasFocus();
        // So too a test round's D-pad press (hands::ScriptEject), from its first frame to the one it's let go on: X's state kept from the
        // frame after it began (its first starts from nothing, whatever a controller held before the session went out of focus).
        const bool scriptX = g_standIn.load(std::memory_order_relaxed) && GameHasFocus() && (hands::ScriptEject() || g_scriptX);
        ReleaseAll(g_standIn.load(std::memory_order_relaxed) && GameHasFocus(), fireAlone, scriptX && g_scriptX);
        g_handTracked[0] = g_handTracked[1] = false;  // not seen this frame (the stand-in may hold the gun)
        g_squeezeRead[0] = g_squeezeRead[1] = false;  // nor either grip read: free hands take nothing for a let-go meanwhile
        g_standingIn = false;
        NoteTracking(seconds, false);  // (not watched: the headset off or out of focus is no loss, no line)
        HeadMoveAlone();
        QteAlone();  // (weapon.txt "qtetest": a quick-time event's press with nobody in the headset)
        if (fireAlone)
            FireAlone(filmPull);
        g_scriptX = false;
        if (scriptX) {
            const ULONGLONG now = GetTickCount64();
            XButton(DpadAsX(hands::ScriptEject(), false, now), now);
            g_scriptX = g_eject.down || g_xDown;  // (let go: the next frame lets go of everything as before)
        }
        return;
    }
    g_focused = true;
    g_scriptX = false;  // (a press under way goes on below as the D-pad's own)
    // The left hand's too: without it the D-pad, stick and left trigger do nothing.
    {
        static XrPath leftHand = Path("/user/hand/left"), leftProfile = XR_NULL_PATH;
        static bool logged = false;
        XrInteractionProfileState left{XR_TYPE_INTERACTION_PROFILE_STATE};
        if (XR_SUCCEEDED(xrGetCurrentInteractionProfile(session, leftHand, &left)) &&
            (!logged || left.interactionProfile != leftProfile)) {
            logged = true;
            leftProfile = left.interactionProfile;
            char name[XR_MAX_PATH_LENGTH] = "(none: asleep or not connected)";
            uint32_t length = 0;
            if (leftProfile)
                xrPathToString(g_instance, leftProfile, sizeof(name), &length, name);
            Log("Controllers: the left hand's profile is %s", name);
            NoteBuzzProfile(0, leftProfile);
            NoteEjectProfile(leftProfile);
        }
    }
    // Which controller profile the runtime picked (it decides what each button does).
    static XrPath rightHand = Path("/user/hand/right"), profile = XR_NULL_PATH;
    XrInteractionProfileState state{XR_TYPE_INTERACTION_PROFILE_STATE};
    if (XR_SUCCEEDED(xrGetCurrentInteractionProfile(session, rightHand, &state)) &&
        state.interactionProfile != profile) {
        profile = state.interactionProfile;
        char name[XR_MAX_PATH_LENGTH] = "(none)";
        uint32_t length = 0;
        if (profile)
            xrPathToString(g_instance, profile, sizeof(name), &length, name);
        Log("Controllers: the right hand's profile is %s", name);
        NoteBuzzProfile(1, profile);
    }

    XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
    if (XR_SUCCEEDED(xrLocateSpace(g_aimSpace, space, time, &location)) &&
        (location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT)) {
        g_aimPose = location.pose;
        g_aimTracked = true;
    }
    for (int i = 0; i < 2; ++i) {
        XrSpaceVelocity velocity{XR_TYPE_SPACE_VELOCITY};
        XrSpaceLocation hand{XR_TYPE_SPACE_LOCATION};
        hand.next = &velocity;
        const bool located = XR_SUCCEEDED(xrLocateSpace(g_handSpaces[i], space, time, &hand));
        // (The runtime's own flags and place, raw, for the tracking record alone: g_handTracked and the poses keep their meaning.)
        g_handFlags[i] = located ? hand.locationFlags : 0;
        if (g_handFlags[i] & XR_SPACE_LOCATION_POSITION_VALID_BIT)
            g_handRaw[i] = hand.pose.position;
        g_handTracked[i] = located && (hand.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) &&
                           (hand.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT);
        if (g_handTracked[i])
            g_handPoses[i] = hand.pose;
        g_handLinearValid[i] = g_handTracked[i] && (velocity.velocityFlags & XR_SPACE_VELOCITY_LINEAR_VALID_BIT) && std::isfinite(velocity.linearVelocity.x) &&
                               std::isfinite(velocity.linearVelocity.y) && std::isfinite(velocity.linearVelocity.z);
        g_handAngularValid[i] = g_handTracked[i] && (velocity.velocityFlags & XR_SPACE_VELOCITY_ANGULAR_VALID_BIT) && std::isfinite(velocity.angularVelocity.x) &&
                                std::isfinite(velocity.angularVelocity.y) && std::isfinite(velocity.angularVelocity.z);
        g_handLinear[i] = g_handLinearValid[i] ? velocity.linearVelocity : XrVector3f{};
        g_handAngular[i] = g_handAngularValid[i] ? velocity.angularVelocity : XrVector3f{};
    }
    // The tracking record of each controller (seen, guessed, no place) and the reach test's lines, from the raw reading, before SteadyHand holds a hand
    // lost for a moment where it was.
    NoteTracking(seconds, true);
    // Tracking's jumps taken out (the right controller close under the headset: the user's "the gun sort of kicks away", 2026-11-05).
    for (int i = 0; i < 2; ++i)
        SteadyHand(i, g_handTracked[i], time);
    LearnAimToGrip();
    g_standingIn = false;
    {
        XrActionStateGetInfo get{XR_TYPE_ACTION_STATE_GET_INFO};
        get.action = g_squeeze;
        XrActionStateFloat squeeze{XR_TYPE_ACTION_STATE_FLOAT};
        const bool leftSqueezeRead = XR_SUCCEEDED(xrGetActionStateFloat(session, &get, &squeeze)) && squeeze.isActive;
        g_leftSqueeze = leftSqueezeRead ? squeeze.currentState : 0.0f;
        g_squeezeRead[0] = leftSqueezeRead && std::isfinite(g_leftSqueeze);
        get.action = g_calibrate;
        XrActionStateBoolean view{XR_TYPE_ACTION_STATE_BOOLEAN};
        bool down = XR_SUCCEEDED(xrGetActionStateBoolean(session, &get, &view)) && view.isActive && view.currentState;
        if (down && !g_calibrateDown)
            g_calibratePressed = true;
        g_calibrateDown = down;
        get.action = g_thumbTouch;
        XrActionStateBoolean thumb{XR_TYPE_ACTION_STATE_BOOLEAN};
        g_leftThumb = XR_SUCCEEDED(xrGetActionStateBoolean(session, &get, &thumb)) && thumb.isActive && thumb.currentState;
        // Recentering by the right stick's click, held: once a hold, whether or not BF3 has focus.
        get.action = g_recenter;
        XrActionStateBoolean click{XR_TYPE_ACTION_STATE_BOOLEAN};
        const bool held =
            XR_SUCCEEDED(xrGetActionStateBoolean(session, &get, &click)) && click.isActive && click.currentState;
        const ULONGLONG now = GetTickCount64();
        if (held)
            g_recenterHeldAt = now;
        if (!held && now - g_recenterHeldAt > kRecenterSlip) {
            g_recenterSince = 0;
            g_recenterDone = false;
        } else if (held && !g_recenterSince) {
            g_recenterSince = now;
        } else if (held && g_recenterSince && !g_recenterDone && now - g_recenterSince >= kRecenterHold) {
            g_recenterDone = true;
            g_recenterWanted.store(true, std::memory_order_relaxed);
            Log("Input: the right stick's click held %.1f s: the view recentered", kRecenterHold / 1000.0);
        }
        // ... a short click of it: a jump.
        {
            static ULONGLONG s_clickAt = 0, s_letGoAt = 0;
            static float s_clickMost = 0.0f;
            static bool s_wasHeld = false, s_jumpWaiting = false;
            if (held && !s_wasHeld) {
                s_clickAt = now;
                s_clickMost = g_rightStickLength;
                s_jumpWaiting = false;
            } else if (held) {
                s_clickMost = std::fmax(s_clickMost, g_rightStickLength);
            } else if (s_wasHeld) {
                s_letGoAt = now;
                s_jumpWaiting = now - s_clickAt < kJumpClick && s_clickMost < kJumpStickMost && !g_recenterDone;
                if (now - s_clickAt < kJumpClick && s_clickMost >= kJumpStickMost)
                    Log("Input: the right stick clicked while pushed %.0f%% over: no jump", s_clickMost * 100.0f);
            }
            if (s_jumpWaiting && !held && now - s_letGoAt >= kJumpSettle) {
                s_jumpWaiting = false;
                g_stickJumpUntil = now + kJumpPress;
                Log("Input: the right stick clicked: a jump");
            }
            s_wasHeld = held;
        }
        get.action = g_ads;  // the left trigger
        XrActionStateFloat trigger{XR_TYPE_ACTION_STATE_FLOAT};
        g_leftTrigger = XR_SUCCEEDED(xrGetActionStateFloat(session, &get, &trigger)) && trigger.isActive
                            ? trigger.currentState
                            : 0.0f;
        // Free hands: the right grip (it holds the gun), the right thumb on its stick and the right trigger's pull, whether or not BF3
        // has focus, as the left's are (the fire mapping below reads the same pull again, from this same sync).
        if (RightGripMade()) {
            get.action = g_squeezeRight;
            XrActionStateFloat rightSqueeze{XR_TYPE_ACTION_STATE_FLOAT};
            g_squeezeRead[1] = XR_SUCCEEDED(xrGetActionStateFloat(session, &get, &rightSqueeze)) && rightSqueeze.isActive &&
                               std::isfinite(rightSqueeze.currentState);
            g_rightSqueeze = g_squeezeRead[1] ? rightSqueeze.currentState : 0.0f;
            get.action = g_thumbTouchRight;
            XrActionStateBoolean rightThumb{XR_TYPE_ACTION_STATE_BOOLEAN};
            g_rightThumb = XR_SUCCEEDED(xrGetActionStateBoolean(session, &get, &rightThumb)) && rightThumb.isActive && rightThumb.currentState;
        } else {
            g_squeezeRead[1] = false;
            g_rightSqueeze = 0.0f;
            g_rightThumb = false;
        }
        get.action = g_fire;
        XrActionStateFloat rightTrigger{XR_TYPE_ACTION_STATE_FLOAT};
        g_rightTriggerNow = XR_SUCCEEDED(xrGetActionStateFloat(session, &get, &rightTrigger)) && rightTrigger.isActive &&
                                    std::isfinite(rightTrigger.currentState)
                                ? rightTrigger.currentState
                                : 0.0f;
        // The reach test's squeezes (line 6) and paths, each grip as just read.
        NoteReachSqueeze(0, seconds);
        NoteReachSqueeze(1, seconds);
        NoteReachPath(0, seconds);
        NoteReachPath(1, seconds);
    }

    if (!GameHasFocus()) {
        ReleaseAll();
        headmove::NoteStick(false);
        return;
    }

    // A quick-time event's press (qte.h): in a scripted moment a fresh squeeze of either grip holds every prompt's key and button down.
    const bool qteHeld = QteStep();
    QteKeys(qteHeld);
    for (Mapping& m : g_buttons) {
        XrActionStateGetInfo get{XR_TYPE_ACTION_STATE_GET_INFO};
        get.action = *m.action;
        bool down = false;
        if (m.analog) {
            // A little hysteresis so a trigger resting near the threshold doesn't chatter.
            XrActionStateFloat state{XR_TYPE_ACTION_STATE_FLOAT};
            const bool active = XR_SUCCEEDED(xrGetActionStateFloat(session, &get, &state)) && state.isActive;
            if (active)
                down = state.currentState > (m.down ? 0.4f : 0.6f);
            if (m.action == &g_fire)  // the glove's trigger finger follows it (skin.cpp)
                g_rightTrigger.store(active ? state.currentState : 0.0f, std::memory_order_relaxed);
        } else {
            XrActionStateBoolean state{XR_TYPE_ACTION_STATE_BOOLEAN};
            down = XR_SUCCEEDED(xrGetActionStateBoolean(session, &get, &state)) && state.isActive &&
                   state.currentState;
        }
        if (m.action == &g_fire)  // (the grenade launcher's shot, the hand on the rifle's pistol grip, what holds it off or stops it)
            down = FireButton(m, down, g_leftTrigger, g_rightTrigger.load(std::memory_order_relaxed));
        // The left trigger is aim down sights, unless the grip is held with it: a fist. With free
        // hands the scope is looked through by holding the gun up to the eye, and the trigger
        // works the charging handle (reload.cpp) and curls the glove's index finger (user,
        // 2026-10-30).
        if (down && m.action == &g_ads && (hands::On() || g_leftSqueeze > 0.5f))
            down = false;
        // weapon.txt "zoom 1": the game's zoom held (the Stinger locks only zoomed; a test before
        // raising the tube to the eye does it, 2026-10-31).
        if (m.action == &g_ads && (g_holdZoom.load(std::memory_order_relaxed) || skin::LauncherZoomWanted()))
            down = true;
        if (m.action == &g_sprint && down)
            g_sprintAt = GetTickCount64();
        // B crouches no more (the user, 2026-11-05: the right stick does, UpdateStance); a real crouch still holds the game's crouch (headmove.h).
        if (m.action == &g_crouch)
            down = headmove::CrouchWanted() && g_stance == Stance::kStand;  // (not over the stick's: a bow when prone would bring the soldier up to a crouch)
        if (m.action == &g_jump && GetTickCount64() < g_stickJumpUntil)
            down = true;  // (the right stick's short click)
        if (m.action == &g_jump && down && g_stance != Stance::kStand) {
            g_crouchOn = false;
            SetStance(Stance::kStand);  // (a jump stands the soldier up)
        }
        // Free hands: the rifle on the ground (or flying into a hand) is drawn only while it's the game's weapon, so no pick till a hand
        // has it (the left's too: a pick then works as ever, and the rifle comes back to the right hand). Put away at its place (holsters:
        // hands::GunStowed) it may be picked: a pick then goes into the right hand (Pick).
        if ((m.action == &g_selectLauncher || m.action == &g_selectRifle || m.action == &g_selectSwap) && down && !m.down &&
            !hands::HoldsGun() && !hands::GunStowed()) {
            Log("Input: no weapon picked: the rifle is on the ground");
            m.down = down;
            continue;
        }
        // The weapon's pick: a press long enough to take, and the way back is known for Y and Touch's button.
        if (m.action == &g_selectLauncher || m.action == &g_selectRifle || m.action == &g_selectSwap) {
            if (down && !m.down) {
                const bool launcher = m.action == &g_selectSwap ? g_toLauncher : m.action == &g_selectLauncher;
                g_toLauncher = !launcher;
                // Once the Stinger is taken up (slot 1, key 2) it's that, the launcher's job being done by then.
                const bool stinger = launcher && ammo::SecondaryCarried();
                Pick(stinger ? 1 : launcher ? 5 : 0, "");
            }
            m.down = down;
            continue;
        }
        if (m.action == &g_use) {
            const ULONGLONG since = GetTickCount64();
            if (!down) {
                g_useSince = 0;
                g_useSwapped = false;
            } else if (!g_useSince) {
                g_useSince = since;
            } else if (!g_useSwapped && since - g_useSince >= kHoldToSwap && !hands::HoldsGun() && !hands::GunStowed()) {
                g_useSwapped = true;  // (once a hold: the rifle on the ground, as the picks above)
                Log("Input: no weapon picked: the rifle is on the ground");
            } else if (!g_useSwapped && since - g_useSince >= kHoldToSwap) {
                g_useSwapped = true;
                const bool launcher = g_toLauncher;
                g_toLauncher = !launcher;
                const bool stinger = launcher && ammo::SecondaryCarried();
                Pick(stinger ? 1 : launcher ? 5 : 0, "Y held: ");
            }
        }
        if (qteHeld && QteButton(m))  // (past every rule of play above: the moment is the game's, and it's let go the frame the press ends)
            down = true;
        if (down != m.down) {
            Press(m, down);
            m.down = down;
        }
    }

    if (g_selectScan && GetTickCount64() >= g_selectUntil) {
        SendKey(g_selectScan, false);
        g_selectScan = 0;
    }

    XrActionStateGetInfo getX{XR_TYPE_ACTION_STATE_GET_INFO};
    getX.action = g_reload;
    XrActionStateBoolean xState{XR_TYPE_ACTION_STATE_BOOLEAN};
    bool x = XR_SUCCEEDED(xrGetActionStateBoolean(session, &getX, &xState)) && xState.isActive &&
             xState.currentState;
    ULONGLONG now = GetTickCount64();
    // The Frame's left D-pad right (or a test round's): X itself while it counts (DpadAsX), then X's own tap, hold and let-go (XButton).
    {
        bool dpad = false;
        if (g_ejectLeft != XR_NULL_HANDLE) {
            XrActionStateGetInfo getE{XR_TYPE_ACTION_STATE_GET_INFO};
            getE.action = g_ejectLeft;
            XrActionStateBoolean eState{XR_TYPE_ACTION_STATE_BOOLEAN};
            dpad = XR_SUCCEEDED(xrGetActionStateBoolean(session, &getE, &eState)) && eState.isActive && eState.currentState;
        }
        x = DpadAsX(dpad || hands::ScriptEject(), x, now);
    }
    XButton(x, now);

    // The left stick walks where the head faces (user, 2026-10-30: not where the gun points). The
    // game walks along its own facing, which follows the gun, so the stick is turned by the head's
    // angle from it; then it goes in as a controller's left stick (pad.cpp), or as W/A/S/D.
    XrVector2f move = Stick(session, g_move);
    const float c = std::cos(g_moveTurn), s = std::sin(g_moveTurn);
    float mx = move.x * c - move.y * s, my = move.x * s + move.y * c;
    const float length = std::sqrt(mx * mx + my * my);
    constexpr float kDeadZone = 0.15f;
    const float scale = length > kDeadZone ? (length - kDeadZone) / (1.0f - kDeadZone) / length : 0.0f;
    // The player's own stick wins; left alone, it may walk the soldier under the head (headmove.h).
    const bool playerStick = length > kDeadZone;
    headmove::NoteStick(playerStick);
    float gameX = mx * scale, gameY = my * scale;
    if (float followX = 0.0f, followY = 0.0f; !playerStick && headmove::FollowStick(followX, followY))
        gameX = followX, gameY = followY;
    // The keys when the game isn't reading the stick (pad.cpp); none with another window in front, which they'd
    // type into (any held one is let go).
    const bool focus = GameHasFocus();
    const bool analog = config::Get().analogMove && focus && pad::SetMove(gameX, gameY);
    const bool keys = focus && !analog;
    bool want[4] = {keys && my > 0.5f, keys && my < -0.5f, keys && mx < -0.5f, keys && mx > 0.5f};
    for (int i = 0; i < 4; ++i) {
        if (want[i] != g_moveKeys[i].down) {
            SendKey(g_moveKeys[i].scan, want[i]);
            g_moveKeys[i].down = want[i];
        }
    }

    const XrVector2f rightStick = Stick(session, g_turn);
    g_stickTurn = rightStick.x;  // turned in UpdateTurn
    g_rightStickLength = std::sqrt(rightStick.x * rightStick.x + rightStick.y * rightStick.y);  // (a click pushed over isn't a jump)
    UpdateStance(rightStick);
}

void SetScripted(bool cutscene, bool turn, bool turnArmed) {
    g_qteCutscene.store(cutscene, std::memory_order_relaxed);
    g_qteTurn.store(turn, std::memory_order_relaxed);
    g_qteTurnArmed.store(turnArmed, std::memory_order_relaxed);
}

void SetQte(bool on) {
    if (g_qteOn.exchange(on) != on)
        Log("QTE: %s", on ? "in a scripted moment a fresh squeeze of either grip answers the game's quick-time events (weapon.txt qte 1)"
                          : "the grips answer no quick-time events (weapon.txt qte 0)");
}

void StartQteTest(float seconds) {
    if (!(seconds > 0.0f)) {
        g_qteTestUntil.store(0, std::memory_order_relaxed);
        return;
    }
    const ULONGLONG now = GetTickCount64();
    const ULONGLONG held = static_cast<ULONGLONG>(std::fmin(seconds, 30.0f) * 1000.0f);
    g_qteTestFrom.store(now + 300, std::memory_order_relaxed);
    g_qteTestUntil.store(now + 300 + held + 200, std::memory_order_relaxed);
    Log("QTE test: the left grip as let go for 0.3 s, then squeezed %.1f s (it presses only in a scripted moment)", static_cast<double>(held) / 1000.0);
}

void LetGo() {
    ReleaseAll();
    g_focused = false;  // (Sync isn't running: no buzz)
    KeepButtonsUp();    // (as Sync would: its lone releases go on while it doesn't run)
}

void StartTurnTest() {
    g_turnTestFrames = 216;  // 3 s at 72 Hz
    Log("Turn test: 3 s of the stick held right");
}

void UpdateTurn(bool direct) {
    float x = GameHasFocus() ? g_stickTurn : 0.0f;
    if (g_turnTestFrames > 0 && GameHasFocus()) {
        --g_turnTestFrames;
        x = 1.0f;
    }
    SetMouseTurn(direct ? 0.0f : x);
    g_directTurn = direct ? x : 0.0f;
    if (direct && TurnAmount(x) > 0.0f)
        NoteTurning();
}

void SetMoveTurn(float radians) {
    g_moveTurn = radians;
}

bool TakeRecenter() {
    return g_recenterWanted.exchange(false, std::memory_order_relaxed);
}

float TakeTurn() {
    const double now = NowMs();
    double elapsed = now - g_lastTake;
    g_lastTake = now;
    if (elapsed < 0.0 || elapsed > 50.0)
        elapsed = 1000.0 / 72.0;  // after a pause, one frame's worth
    const float once = -g_turnBy.exchange(0.0f) * 0.01745329f;  // weapon.txt "turnby", on top of the stick's
    const float amount = TurnAmount(g_directTurn);
    if (amount == 0.0f)
        return once;
    // Left is positive; the stick to the right turns right.
    const float radians = amount * config::Get().turnDegrees * 0.01745329f * static_cast<float>(elapsed / 1000.0);
    return (g_directTurn > 0.0f ? -radians : radians) + once;
}

void TurnBy(float degrees) {
    g_turnBy.store(degrees);
    Log("Turn test: the view turned %.0f degrees %s at once", std::fabs(degrees), degrees < 0.0f ? "left" : "right");
}

void NoteTurnStep(int which, float yaw) {
    constexpr float kPi = 3.14159265f;
    if (which < 0 || which > 1)
        return;
    TurnSteps& t = g_turnSteps[which];
    if (t.haveLast && TurningSteadily()) {
        float step = yaw - t.lastYaw;
        if (step > kPi)
            step -= 2.0f * kPi;
        else if (step < -kPi)
            step += 2.0f * kPi;
        t.steps.push_back(std::fabs(step) * 57.29578f);
        t.net += step * 57.29578f;
    }
    t.lastYaw = yaw;
    t.haveLast = true;
    ULONGLONG now = GetTickCount64();
    if (now - t.lastLog < 1000)
        return;
    t.lastLog = now;
    if (t.steps.empty())
        return;
    std::vector<float> sorted = t.steps;
    std::sort(sorted.begin(), sorted.end());
    const float median = sorted[sorted.size() / 2];
    int none = 0, doubled = 0;
    for (float s : t.steps) {
        none += s < 0.25f * median;
        doubled += s > 1.75f * median;
    }
    Log("Turn (%s): %zu frames turning; the yaw moved %.3f degrees a frame (median), %.3f to %.3f; %d frames "
        "without a step, %d with a double one; net %+.1f degrees",
        t.name, t.steps.size(), median, sorted.front(), sorted.back(), none, doubled, t.net);
    if (none || doubled || sorted.back() > 1.25f * median || sorted.front() < 0.75f * median) {
        char text[512];
        int used = 0;
        for (size_t i = 0; i < t.steps.size() && i < 48 && used < 480; ++i)
            used += snprintf(text + used, sizeof(text) - used, " %.2f", t.steps[i]);
        Log("Turn (%s): steps%s", t.name, text);
    }
    t.steps.clear();
    t.net = 0.0f;
}

bool AimPose(XrPosef& out) {
    if (!g_aimTracked)
        return false;
    out = g_aimPose;
    return true;
}

float LeftSqueeze() {
    return g_leftSqueeze;
}

float LeftTrigger() {
    return g_leftTrigger;
}

bool LeftThumbOnStick() {
    return g_leftThumb;
}

float Squeeze(int hand) {
    return hand == 0 ? g_leftSqueeze : hand == 1 ? g_rightSqueeze : 0.0f;
}

bool SqueezeRead(int hand) {
    return hand >= 0 && hand < 2 && g_squeezeRead[hand];
}

float Trigger(int hand) {
    return hand == 0 ? g_leftTrigger : hand == 1 ? g_rightTriggerNow : 0.0f;
}

bool ThumbOnStick(int hand) {
    return hand == 0 ? g_leftThumb : hand == 1 && g_rightThumb;
}

bool AimFromGrip(int hand, const XrPosef& grip, XrPosef& aim) {
    if (hand < 0 || hand > 1 || !g_haveAimToGrip)
        return false;
    // The grip in the aim's frame, the right controller's as learned; the left's its mirror image across x (q (x, -y, -z, w),
    // p (-x, y, z)), exact for mirror-image controllers. Then aim = grip with that taken back off.
    XrPosef a = g_aimToGrip;
    if (hand == 0) {
        a.orientation.y = -a.orientation.y, a.orientation.z = -a.orientation.z;
        a.position.x = -a.position.x;
    }
    XrQuaternionf q = Mul(grip.orientation, Conjugate(a.orientation));
    const float length = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (!std::isfinite(length) || length < 1e-6f)
        return false;
    q = {q.x / length, q.y / length, q.z / length, q.w / length};
    const XrVector3f back = Rotate(q, a.position);
    const XrVector3f p{grip.position.x - back.x, grip.position.y - back.y, grip.position.z - back.z};
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
        return false;
    aim.orientation = q;
    aim.position = p;
    return true;
}

unsigned long long GameReloadSince() {
    if (g_rDown)
        return 0;
    return g_rLetGoAt ? GetTickCount64() - g_rLetGoAt : ~0ULL;
}

bool TakeCalibratePress() {
    bool pressed = g_calibratePressed;
    g_calibratePressed = false;
    return pressed;
}

bool HandSettled(int hand) {
    if (hand < 0 || hand > 1 || !g_handTracked[hand])
        return false;
    if (!g_steadyHands.load(std::memory_order_relaxed))
        return true;
    const SteadyState& s = g_steady[hand];
    // (A jump taken out counts for its first kJumpConfirmFrames, while it may still be put back as a real move; its fade after, which takes over a
    // second to end from 10 cm, follows the hand's own reading the whole way and doesn't.)
    return s.have && !s.gap && !(s.off && s.sinceJump < kJumpConfirmFrames);
}

bool HandPose(int hand, XrPosef& out) {
    if (hand < 0 || hand > 1 || !g_handTracked[hand])
        return false;
    out = g_handPoses[hand];
    return true;
}

bool TrackingOf(int hand, tracking::Record& out) {
    if (hand < 0 || hand > 1)
        return false;
    out = g_track[hand];
    return true;
}

// Holsters' key (hands.cpp, a draw from another slot's place and its key again): held as a pick's is (HoldSelect: 0.3 s, ammo::Picked told), let go
// by Sync (or ReleaseAll, UpdateM320) as a pick's is. Not a pick of the user's: LastPickAt doesn't move. Never into another window than bf3's (the
// keys go to whichever is in front), nor during the grenade launcher's shot (its own toggles; holster.h asks none then either): false, said at most
// once a second. Y held and Touch's menu button go between the two guns, so the other one is theirs next, as after a pick of this slot.
bool SelectSlot(int slot) {
    const WORD scan = ScanOfSlot(slot);
    if (!scan)
        return false;
    const char* why = !GameHasFocus() ? "bf3 isn't in front" : g_m320Shot != M320Shot::kIdle ? "the grenade launcher's shot is under way" : nullptr;
    if (why) {
        static ULONGLONG s_saidAt = 0;
        const ULONGLONG now = GetTickCount64();
        if (!s_saidAt || now - s_saidAt >= 1000) {
            s_saidAt = now;
            Log("Input: holsters' key %d (slot %d) not sent: %s", KeyOfSlot(slot), slot, why);
        }
        return false;
    }
    HoldSelect(scan);
    g_toLauncher = slot == 0;
    Log("Input: key %d held for holsters (slot %d: the game's own switch)", KeyOfSlot(slot), slot);
    return true;
}

unsigned long long LastPickAt(int* slot, unsigned* picks) {
    if (slot)
        *slot = g_pickSlot;
    if (picks)
        *picks = g_picks;
    return g_pickAt;
}

bool M320GameReloading() {
    return g_m320Shot == M320Shot::kReloading || (g_m320Shot == M320Shot::kSelecting && g_m320JustReload);
}

bool M320ShotUnderway() {
    return g_m320Shot != M320Shot::kIdle;
}

unsigned M320Fired() {
    return g_m320Fired.load(std::memory_order_relaxed);
}

bool HandVelocity(int hand, float* linear, float* angular) {
    if (hand < 0 || hand > 1 || !g_handTracked[hand] || !g_handLinearValid[hand])
        return false;
    linear[0] = g_handLinear[hand].x, linear[1] = g_handLinear[hand].y, linear[2] = g_handLinear[hand].z;
    angular[0] = g_handAngular[hand].x, angular[1] = g_handAngular[hand].y, angular[2] = g_handAngular[hand].z;  // (0 if not given)
    return true;
}

void Buzz(int side, float amplitude, float seconds, float hertz) {
    if (side < 0 || side > 1 || !std::isfinite(amplitude) || !std::isfinite(seconds) || amplitude <= 0.0f || seconds <= 0.0f)
        return;
    g_buzzAsked[side].fetch_add(1, std::memory_order_relaxed);
    BuzzCount();
    if (!g_ready || !g_focused || !g_buzzBound[side] || !HapticsMade()) {
        g_buzzWhyNot.store(!g_ready ? 0 : !g_focused ? 1 : !g_buzzBound[side] ? 2 : 3, std::memory_order_relaxed);
        return;
    }
    g_buzzSent[side].fetch_add(1, std::memory_order_relaxed);
    Pulse p;
    p.amplitude = std::fmin(amplitude, 1.0f);
    p.seconds = std::fmin(seconds, kBuzzLongest);
    p.hertz = std::isfinite(hertz) && hertz > 0.0f ? std::fmin(hertz, kBuzzHighest) : 0.0f;
    if (g_buzzFromWorker) {
        AcquireSRWLockExclusive(&g_buzzLock);
        g_buzzNext[side] = p;
        g_buzzWaiting[side] = true;
        ReleaseSRWLockExclusive(&g_buzzLock);
        SetEvent(g_buzzWake);
        return;
    }
    const double start = NowMs();
    SendBuzz(side, p);
    const double took = NowMs() - start;
    if (g_buzzTimedCount == 0)
        Log("Controllers: the first buzz: the %s hand, %.2f for %.0f ms at %s: %.3f ms to send", side ? "right" : "left", p.amplitude,
            p.seconds * 1000.0f, p.hertz > 0.0f ? "the hertz asked for" : "the runtime's own hertz", took);
    if (g_buzzTimedCount < kBuzzTimed) {
        g_buzzTimes[g_buzzTimedCount++] = took;
        if (g_buzzTimedCount == kBuzzTimed) {
            double sorted[kBuzzTimed];
            std::copy(g_buzzTimes, g_buzzTimes + kBuzzTimed, sorted);
            std::sort(sorted, sorted + kBuzzTimed);
            const double median = 0.5 * (sorted[kBuzzTimed / 2 - 1] + sorted[kBuzzTimed / 2]);
            char why[64];
            snprintf(why, sizeof(why), "the median of its first %d, the slowest %.3f", kBuzzTimed, sorted[kBuzzTimed - 1]);
            if (median > kBuzzSlowMs)
                BuzzFromWorker(median, why);
            else
                Log("Controllers: a buzz takes %.3f ms to send (%s): sent as they're asked for", median, why);
        }
    } else if (took > kBuzzHitchMs && ++g_buzzHitchCount >= kBuzzHitches) {
        char why[64];
        snprintf(why, sizeof(why), "%d calls over %.1f ms", kBuzzHitches, kBuzzHitchMs);
        BuzzFromWorker(took, why);
    }
}

void SetStandIn(bool on, float pitchDegrees, float yawDegrees, const float* shift) {
    for (int i = 0; i < 3; ++i)
        g_standInShift[i].store(shift ? shift[i] : 0.0f, std::memory_order_relaxed);
    if (g_standIn.exchange(on) != on)
        Log("Stand-in hand: %s", on ? "on (holds the rifle while the right controller isn't tracked)" : "off");
    const float pitch = pitchDegrees * 3.14159265f / 180.0f;
    if (g_standInPitch.exchange(pitch) != pitch)
        Log("Stand-in hand: the headless head looks %.0f degrees up", pitchDegrees);
    const float yaw = -yawDegrees * 3.14159265f / 180.0f;  // given right positive
    if (g_standInYaw.exchange(yaw) != yaw)
        Log("Stand-in hand: aiming %.0f degrees right of the head", yawDegrees);
}

std::atomic<float> g_triggerTest{0.0f};  // weapon.txt "fingertest 1": the finger as if pulled, not firing

float RightTrigger() {
    return std::max(g_rightTrigger.load(std::memory_order_relaxed), g_triggerTest.load(std::memory_order_relaxed));
}

void SetTriggerTest(float pull) {
    if (g_triggerTest.exchange(pull) != pull && pull > 0.0f)
        Log("Input: the glove's trigger finger drawn %.0f%% pulled (weapon.txt fingertest)", pull * 100.0f);
}

void SetFilmTrigger(int hand, float pull) {
    if (hand < 0 || hand > 1)
        return;
    g_filmTrigger[hand].store(std::isfinite(pull) ? std::clamp(pull, 0.0f, 1.0f) : 0.0f, std::memory_order_relaxed);
    g_filmTriggerGiven[hand].store(true, std::memory_order_relaxed);
    g_filmTriggerNew[hand].store(true, std::memory_order_relaxed);  // (the launcher's trigger goes by it a while: UpdateM320Shot)
}

void StartFilmPull(float seconds, int hand, int pulls) {
    if (!(seconds > 0.0f) || !std::isfinite(seconds)) {
        if (g_filmPullHand.exchange(-1) >= 0)
            Log("Input: a film's pull stopped (weapon.txt filmtrigger 0)");
        return;
    }
    hand = hand == 0 ? 0 : 1;
    pulls = std::clamp(pulls, 1, 100);
    g_filmPullHand.store(-1, std::memory_order_relaxed);
    g_filmPullSaid.store(-1, std::memory_order_relaxed);
    g_filmPullCount.store(pulls, std::memory_order_relaxed);
    g_filmPullHold.store(static_cast<ULONGLONG>(std::fmin(seconds, 60.0f) * 1000.0f), std::memory_order_relaxed);
    g_filmPullFrom.store(GetTickCount64(), std::memory_order_relaxed);
    g_filmPullAsked.store(GetTickCount64(), std::memory_order_relaxed);
    g_filmPullHand.store(hand, std::memory_order_release);
    Log("Input: a film's pull of the %s trigger asked for: %d pull%s of %.1f s, %.1f s apart (weapon.txt filmtrigger; real rounds)", hand == 0 ? "left" : "right",
        pulls, pulls == 1 ? "" : "s", std::fmin(seconds, 60.0f), kFilmPullGap / 1000.0);
}

int StanceIndex() {
    return g_stance == Stance::kProne ? 2 : g_stance == Stance::kCrouch || headmove::CrouchWanted() ? 1 : 0;
}

float StanceEyeHeight() {
    // The game's own (its soldier physics data: the eye 1.6 m above the feet standing, 1.0 crouched, 0.3 prone), as the mod last put the stance; a real crouch holds its crouch.
    if (g_stance == Stance::kProne)
        return 0.3f;
    return g_stance == Stance::kCrouch || headmove::CrouchWanted() ? 1.0f : 1.6f;
}

void UpdateM320() {
    if (!GameHasFocus())
        return;  // (its keys would go to whatever's in front)
    if (g_selectScan && GetTickCount64() >= g_selectUntil) {  // (as Update lets go of a pick: Update doesn't run with the headset off)
        SendKey(g_selectScan, false);
        g_selectScan = 0;
    }
    UpdateM320Shot();
}

void TestM320Shot() {
    g_m320TestShot = true;
    Log("Input: a left trigger pull for the grenade launcher (weapon.txt m320shot)");
}

bool Sprinting() {
    // A second after letting go too: the game's sprint takes that long to blend back. Prone, and a second and a half after the stick changes the stance, the game's hands move on its
    // gun just as much (its prone and its transitions, 0.3-1.05 s in its data): nothing's learned from them either (the stance study, 2026-11-05).
    const ULONGLONG now = GetTickCount64();
    return now - g_sprintAt.load(std::memory_order_relaxed) < 1000 || g_stanceProne.load(std::memory_order_relaxed) ||
           now - g_stanceChangedAt.load(std::memory_order_relaxed) < 1500;
}

void SetSteadyHands(bool on) {
    g_steadyHands = on;
}

void SetForceHand(bool on) {
    g_forceHand = on;
}

void SetZoomHold(bool on) {
    if (g_holdZoom.exchange(on) != on)
        Log("Input: the game's zoom %s (weapon.txt zoom)", on ? "held" : "let go");
}

void SetStandInUp(float degrees) {
    g_standInUp = degrees * 3.14159265f / 180.0f;
}

float StandInPitch() {
    return g_standInPitch.load(std::memory_order_relaxed);
}

void StandIn(const XrPosef& head) {
    static bool loaded = false;
    if (!loaded) {
        loaded = true;
        LoadAimToGrip();
    }
    g_haveStandInLeft = false;
    if (!g_standIn.load(std::memory_order_relaxed) || (!g_forceHand.load(std::memory_order_relaxed) && (g_aimTracked || g_handTracked[1]))) {
        g_standingIn = false;
        return;
    }
    // Aiming where the head looks, level (no roll), from the shouldered rifle's place.
    float yaw = 0.0f, pitch = 0.0f;
    {
        const XrQuaternionf& q = head.orientation;
        const float fx = -2.0f * (q.x * q.z + q.w * q.y);
        const float fy = -2.0f * (q.y * q.z - q.w * q.x);
        const float fz = -(1.0f - 2.0f * (q.x * q.x + q.y * q.y));
        yaw = std::atan2(-fx, -fz);
        pitch = std::asin(std::clamp(fy, -1.0f, 1.0f));
    }
    yaw += g_standInYaw.load(std::memory_order_relaxed);
    pitch += g_standInUp.load(std::memory_order_relaxed);
    const XrQuaternionf turn{0.0f, std::sin(yaw / 2), 0.0f, std::cos(yaw / 2)};
    const XrQuaternionf tilt{std::sin(pitch / 2), 0.0f, 0.0f, std::cos(pitch / 2)};
    const XrQuaternionf level = Mul(turn, tilt);
    const XrVector3f hold{kStandInAim.x + g_standInShift[0].load(std::memory_order_relaxed),
                          kStandInAim.y + g_standInShift[1].load(std::memory_order_relaxed),
                          kStandInAim.z + g_standInShift[2].load(std::memory_order_relaxed)};
    const XrVector3f at = Rotate(level, hold);
    g_aimPose.orientation = level;
    g_aimPose.position = {head.position.x + at.x, head.position.y + at.y, head.position.z + at.z};
    g_aimTracked = true;
    const XrVector3f grip = Rotate(level, g_aimToGrip.position);
    g_handPoses[1].orientation = Mul(level, g_aimToGrip.orientation);
    g_handPoses[1].position = {g_aimPose.position.x + grip.x, g_aimPose.position.y + grip.y,
                               g_aimPose.position.z + grip.z};
    g_handTracked[1] = true;
    // A left hand too while that controller sleeps: under the handguard, palm up (a quarter turn
    // about the forward axis), to see the left glove and forearm with nobody in the headset. Only
    // drawn (StandInLeftPose): holds, reloads and touches never see it (the review, 2026-10-31: it
    // took holds and chambered rounds with the controllers' last buttons while the headset was off).
    if (!g_handTracked[0]) {
        const XrVector3f under = Rotate(level, {-0.04f, -0.07f, -0.30f});
        const float half = -0.7853982f;
        g_standInLeft.orientation = Mul(level, XrQuaternionf{0.0f, 0.0f, std::sin(half), std::cos(half)});
        g_standInLeft.position = {g_aimPose.position.x + under.x, g_aimPose.position.y + under.y,
                                  g_aimPose.position.z + under.z};
        g_haveStandInLeft = true;
    }
    if (!g_standingIn) {
        static ULONGLONG told = 0;
        if (GetTickCount64() - told > 10000) {
            told = GetTickCount64();
            Log("Stand-in hand: holding the rifle (the right controller isn't tracked)%s",
                g_haveAimToGrip ? "" : "; its grip is placed at its aim, never having seen the controller");
        }
    }
    g_standingIn = true;
}

bool StandingIn() {
    return g_standingIn;
}

bool StandInLeftPose(XrPosef& out) {
    if (!g_haveStandInLeft)
        return false;
    out = g_standInLeft;
    return true;
}

bool StandInOn() {
    return g_standIn.load(std::memory_order_relaxed);
}

std::atomic<bool> g_tapInsert{false};  // the user, 2026-11-05: off, a magazine comes from over the shoulder by hand or the game's own reload

void SetTapInsert(bool on) {
    g_tapInsert = on;
}

void MagazineTap(bool fromScript) {
    // Free hands: nobody holds the rifle (on the ground, or flying into a hand): no magazine out or in, nor the game's reload (F16, the
    // pouch's reach; X's own press stops before it gets here). Put away at its place (holsters) the same, in words of its own.
    if (!hands::HoldsGun()) {
        if (hands::GunStowed()) {
            char where[128];
            StowedWords(where, sizeof(where));
            Log("Input: a magazine tap%s: nothing in the hands (%s): take it from its place first", fromScript ? " (a script's: F16 or a test)" : "", where);
        } else {
            Log("Input: a magazine tap%s: the rifle is on the ground: take it up first", fromScript ? " (a script's: F16 or a test)" : "");
        }
        return;
    }
    // No button on this gun (the user, 2026-11-05): the hand takes its magazine out. X held is still the game's own reload.
    if (skin::HeldByHand()) {
        // One with a button drops it, as the M416's.
        if (const handmag::Gun* gun = skin::HeldMagGun(); gun && gun->button) {
            if (skin::MagazineOut())
                Log("Input: X with the magazine out: take one at the belly");
            else if (skin::EjectMagazine()) {
                ammo::Eject();
                hands::PlayGunsOwn("mag_out", "mag_out");
            }
            return;
        }
        Log("Input: X: this gun has no magazine button: take it out by hand, or knock it out with a fresh one");
        return;
    }
    // A tube shotgun with its shells by hand: nothing to drop. (A tap sent R before: the game's own reload, with its hidden round still in.)
    if (hands::TubeInHand()) {
        Log("Input: X: the shotgun has no magazine: its shells from the belly, up the loading port; held, the game's own reload");
        return;
    }
    if (!skin::RifleInHand()) {
        // The train pistol with its magazine by hand (build\pistol.txt "mag 1"): X is its magazine release (hands.cpp).
        if (hands::PistolMagazineTap(fromScript))
            return;
        PressR(true);
        g_rUntil = GetTickCount64() + 150;  // the game reads the keys once a frame
        return;
    }
    if (skin::MagazineOut()) {
        if (!fromScript && !g_tapInsert.load(std::memory_order_relaxed)) {
            Log("Input: X with the magazine out: take one at the belly (pouch.txt xinsert 1 brings "
                "the tap back)");
            return;
        }
        ammo::Insert();
        skin::InsertMagazine();
        sounds::Play("mag_in");
    } else if (skin::EjectMagazine()) {
        ammo::Eject();
        sounds::Play("mag_out");
    }
}

}

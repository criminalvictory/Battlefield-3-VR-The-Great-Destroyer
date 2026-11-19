#pragma once
// Recoil for every gun (design 6): the drawn gun's own kick, worked out on its own so tools/recoiltest.cpp can run it without the game.
// Three layers, in the gun's own model space (x left, y up, z forward to the muzzle, metres):
//   A, the drift: the game's own per-shot climb and sideways pull (its GunSwayData, read from the gun's own data), piled up per shot, coming back
//      after a string, drawn through the gun's own camera spring (CameraRecoilData);
//   B, the kick: the game's own first-person kick in the hands (measured: 1.5-1.8 cm back by 30 ms, ~2 degrees up by 50 ms, a roll to a new side
//      each round): damped springs each shot strikes, back, muzzle up, sideways, roll, a launcher's lift; and held fire's slower push back (the
//      game's own: 2.4-3.0 cm back on average while the trigger is held, home 125-200 ms after the last round);
//   C, the vibration: a pulse a shot from the gun's own rumble.
// One hand on a long gun: B five times higher, random and slow, A twice; a pistol keeps its own in one hand; the sight at the eye turns it all
// about the eye, scaled by the sight's own ZoomedKickbackFactor, and the kick never brings the glass within 1.5 cm of the eye, whatever the hold.
// Out: one rigid move in the gun's model space (the drawn pose = the held pose times it) and this frame's pulses.
//
// Every number blended between one hand and two moves the springs' constants and caps, never their state: a cap that falls below the state holds
// it where it is and the spring brings it down by itself, so taking hold mid-kick never yanks the gun. The turn about the eye eases as the eye
// does, so the support hand letting go at the sight never jumps the gun from the eye to the grip. A clear eases out over kClearEase while the
// gun stays in the hand (a long frame, a scripted moment) and is at once when it leaves it (a drop, a throw, a switch).

#include <cmath>
#include <cstdint>

namespace recoil {

constexpr float kDegree = 3.14159265f / 180.0f;
constexpr float kStep = 0.001f;            // seconds: every spring stepped 1 ms at a time (72, 60 and 36 fps draw the same curve)
constexpr float kGap = 0.25f;              // seconds: a frame longer than this clears everything (a load, a pause, the headset off)
constexpr float kMostStep = 0.1f;          // seconds: a longer frame is stepped as this long (the clear goes by the real gap)
constexpr float kStringGap = 1.25f;        // a string goes on while the next round comes within this many of the gun's own gaps: nothing comes back (P2)
constexpr float kFiringGaps = 2.0f;        // the drift comes back at the gun's firing rate while a round left within this many gaps (by rounds, not the button)
constexpr bool kDriftLinear = true;        // the recovery in degrees a second (false: a share a second); P2
constexpr float kFirstAfter = 0.05f;       // seconds past 1.5 gaps without a round: the next is a first shot again
constexpr float kShareLeast = 0.9f;        // each round's share of its up: uniform in [0.9, 1.0] (the web's tables; the logs: 93%)
constexpr int kMostShots = 3;              // rounds taken in one frame
constexpr float kEyeIn = 0.10f, kEyeOut = 0.20f;   // seconds the sight's being at the eye eases in, out
constexpr float kEyeSlideShare = 0.4f, kEyeSlideMost = 0.006f, kEyeGlassLeast = 0.015f;
constexpr float kReboundMost = 0.005f;     // metres forward of where the hands hold it
constexpr float kTotalTurnMost = 30.0f;    // degrees, drift and kick together: for comfort (the aim's own clamp to the head is recoilrun's to log)
constexpr float kDriftDampingLeast = 0.35f;   // the drift's camera spring is never drawn lighter than this (unless Tune::driftDamping says)
constexpr float kHeldStepShare = 0.33f;    // held fire's push back: each later round of a string adds this share of one round's back (the game's: 0.4-0.8 cm)
constexpr float kHeldMost = 0.03f;         // metres: held fire's push back at most (two hands; one hand has the slow spring's own instead)
constexpr float kHeldHome = 0.08f;         // seconds after the string's end for held fire's push back to come home (the game's: 125-200 ms after the round)
constexpr float kFlipShare = 1.5f, kFlipDamping = 0.18f;   // the flip after held fire (Tune::release): its peak against one round's rise; its damping
constexpr float kClearEase = 0.1f;         // seconds a clear eases out over while the gun stays in the hand
constexpr float kQuiet = 1e-5f;            // metres or degrees: a spring this close to rest is at rest
constexpr float kSpreadSettle = 0.2f;      // seconds a change of hold must last before the game's hip spread follows it (Output::hipSpread)
constexpr float kLauncherRumble = 1.0f;    // a launcher's rumble when its data has none (the AT4's): the SMAW's data's LowRumble and HighRumble
constexpr int kAheadMost = 28;             // the slack's look ahead in at most this many steps (36 fps's): a longer frame in longer ones

enum class Kind : uint8_t { kNone, kPistol, kSmg, kRifle, kMg, kShotgun, kMarksman, kBolt, kM320, kRocket, kMissile, kAt4 };
inline bool Long(Kind k) { return k != Kind::kPistol && k != Kind::kNone; }
inline bool Launcher(Kind k) { return k == Kind::kM320 || k == Kind::kRocket || k == Kind::kMissile || k == Kind::kAt4; }

struct Drift {                    // A, as the gun's own data has it for this stance: [0] the hip (NoZoom), [1] at the eye (Zoom); degrees, seconds
    float up[2] = {}, sideMin[2] = {}, sideMax[2] = {}, decrease[2] = {};
    float capUp = 10.0f, capSide = 10.0f, firstShot = 1.0f, shootingScale = 1.0f;
    float spring = 250.0f, damping = 0.35f;   // its CameraRecoilData; spring 0: none (drawn through 250 at damping 1: never a snap)
};
struct Spring { float peak = 0.0f, at = 0.05f, damping = 0.75f; };   // one shot's peak from rest, seconds to it, damping ratio (< 1)
struct Kick {                     // B, two hands at the hip
    Spring back{0.015f, 0.030f, 0.75f};       // metres
    Spring rise{1.8f, 0.050f, 0.75f};         // degrees
    Spring lift{0.0f, 0.100f, 0.70f};         // metres (launchers)
    float roll = 0.9f;            // its peak, as a share of the rise (a pistol's are the holds' own: 0.5)
    float yaw = 0.25f;            // its peak at most, as a share of the rise, either way at random (a pistol's: the holds', 0.1 and 0.15)
    float jitter = 0.1f;          // each round's size: 1 -+ this, two hands (one hand on a long gun 5 times it, a pistol in one hand twice)
    float burstFirst = 1.0f;      // a burst pair's first round's share (the AN-94: 0.3; the second gets 2 - it)
    float heldShare = 1.8f;       // held fire's mean push back against one round's back (the game's own: 2.4-3.0 cm against 1.5), 0 none
};
struct Gun {
    Kind kind = Kind::kNone;
    Drift drift;
    Kick kick;
    float rate = 600.0f;          // rounds a minute
    float pairGap = 0.0f;         // seconds: two rounds within it are a burst pair (the AN-94: 0.075), 0 none
    int8_t burst = -1;            // with a pair gap: 1 its burst mode in force, 0 not (automatic or single: every round strikes whole, at once),
                                  // -1 not known (a pull's first two rounds within the pair gap show a burst; further apart, not)
    int8_t automatic = -1;        // its buzz never longer than 0.8 of its gap: 1 yes, 0 no (a pump, a semi-automatic), -1 by its kind (SMG, rifle, MG)
    float eyeFactor = 1.0f;       // the sight in use's ZoomedKickbackFactor
    float rumble[3] = {};         // LowRumble, HighRumble, RumbleDuration (s)
};
struct Hold {
    bool held = false;            // held by a pistol grip, not scripted: else everything clears
    float hands = 0.0f;           // 0-1, two hands (the support glove's blend onto the gun)
    bool eye = false;             // the sight at the eye this frame (eased in State)
    bool leftTrigger = false;     // the left hand fires it: the sideways pull, the yaw and the roll mirrored
    bool frontGrip = false;       // the support hand on a front grip (the M320's, a launcher's): its buzz 0.8, a handguard's 0.6
    bool stillDrawn = false;      // not held but still drawn in the hand (a scripted moment): the clear eases out instead of at once
};
struct Tune {                     // build\recoil.txt
    float drift = 1.0f, kick = 1.0f, buzz = 1.0f;   // each layer's strength, 0 off
    float oneRise = 5.0f, oneBack = 2.5f;           // one hand on a long gun
    float pistolOne = 1.5f;                         // a pistol in one hand against two
    bool eyePivot = true, mirror = true;
    float sideSign = 1.0f;                          // +1: the data's negative sideways is left (P2 confirms)
    float liftMost = 0.04f;                         // metres
    float gun = 1.0f;                               // this gun's own scale (A and B)
    float buzzHz = 0.0f;                            // 0: the runtime's own (recoilrun's, for input::Buzz)
    bool release = false;                           // the game's flip after held fire (up, then below the line, settling at the camera spring's 2.5 Hz)
    bool oneHandSpread = true;                      // one hand on a long gun: the game's own hip spread back (Output::hipSpread)
    float driftDamping = 0.0f;                      // the drift's camera spring damping ratio instead of the data's (0: the data's, at least 0.35; at most 1)
};
struct Frame {
    float dt = 0.0f;              // seconds since the last frame (stepped at most kMostStep)
    float gap = -1.0f;            // the real seconds since the last frame, never capped, for the clear (negative: dt is the real one)
    int shots = 0;                // rounds out this frame (kMostShots at most)
    bool trigger = false;         // the fire button down
    bool newGun = false;          // another gun, a new life, the first frame: everything clears
    float pivot[3] = {};          // the trigger hand's anchor on the gun (model)
    float eyeAt[3] = {};          // the eye nearer the sights' line (model): when Hold::eye, or any frame with haveEye
    bool haveEye = false;         // eyeAt given this frame though Hold::eye is false (without it, the last given is kept while the eye eases out)
    float glassZ = 0.0f;          // the sight's rear glass along z (model), when haveGlass
    bool haveGlass = false;
    float rearZ = 0.0f;           // the gun's nearest point to the eye on the sights' line along z (model; skin::SightNow::rearZ), when haveRear
    bool haveRear = false;
    uint32_t seed = 0;            // non-zero: restarts the state's random numbers (the tests; recoilrun once a run)
};
struct Osc { float x = 0.0f, v = 0.0f; };
struct State {
    float upTarget = 0.0f, sideTarget = 0.0f;       // A piled up, degrees
    Osc up, side;                                   // A as drawn (the camera spring)
    Osc back, pitch, yaw, roll, lift;               // B
    float heldTarget = 0.0f;                        // held fire's push back piled up (metres)
    Osc held;                                       // ... as drawn
    Osc flip;                                       // the flip after held fire (degrees; Tune::release)
    float since = 1e9f;                             // seconds since the last round
    bool released = true;                           // the fire button up since it
    float rollSign = 1.0f;
    int string = 0;                                 // rounds in this string (0: none going on)
    int8_t pull = -1;                               // this pull's burst mode as its first two rounds showed it (-1 not yet)
    int pullRounds = 0;                             // rounds in this pull
    float heldBack = 0.0f, heldBackIn = -1.0f;      // a burst pair's held-back share and the seconds till it strikes (the AN-94; < 0 none)
    float heldV[5] = {};                            // ... its strike on back, pitch, yaw, roll, lift
    float struck[5] = {};                           // the last round's own peaks from rest (back m, pitch, yaw, roll deg, lift m; signed, before its share)
    float share = 0.0f;                             // ... the share of it struck at once
    float eye = 0.0f;                               // Hold::eye eased (0-1)
    float eyePivot = 0.0f;                          // the turn about the eye's weight as drawn: eye x two hands, eased as the eye is (in at
                                                    // kEyeIn, out at kEyeOut), so a support hand letting go never jumps the pivot
    float eyeAt[3] = {};                            // the eye, and the glass or the gun's rear (its z), as last given
    bool haveEye = false;
    float guardZ = 0.0f;
    bool haveGuard = false;
    bool live = false;                              // anything kicking (springs, targets): else all of them exactly at rest
    float fade = 0.0f;                              // a clear easing out: the share of the last move still drawn
    float fadeQ[4] = {0, 0, 0, 1}, fadeT[3] = {}, fadePivot[3] = {};
    uint32_t rng = 0x9E3779B9u;
    float lastQ[4] = {0, 0, 0, 1};                  // last frame's turn and the one before (the slack)
    float lastQ2[4] = {0, 0, 0, 1};
    float lastT[3] = {}, lastPivot[3] = {};         // last frame's move: its translation and its pivot (model)
    bool spread = false;                            // the game's hip spread as settled (Output::hipSpread; kept through a clear)
    bool spreadHeld = false;                        // the gun held last frame (else the next hold is one taken afresh)
    float spreadFor = 0.0f;                         // seconds the hold has wanted the other one
};
struct Output {
    float k[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};   // the drawn pose = the held pose x k (rows: rotation | translation)
    float q[4] = {0, 0, 0, 1};    // k's turn (x, y, z, w), the gun's model space
    float pitch = 0.0f, yaw = 0.0f, roll = 0.0f;     // degrees, drift and kick together (up, right, its left side up)
    float drift[2] = {};          // A as drawn: up, right (degrees)
    float back = 0.0f, lift = 0.0f;                  // metres
    float slack = 0.0f;           // radians the turn may change by before the next frame (the scope picture's margin)
    float pulse[2] = {}, pulseFor[2] = {};           // to send now: [0] the trigger hand, [1] the support hand (amplitude 0-1, seconds)
    float lateIn = 0.0f, late = 0.0f, lateFor = 0.0f;  // a launcher's lift pulse: seconds from now, amplitude, length (0 none)
    bool active = false;          // anything off the hands' own line
    bool hipSpread = false;       // the game's own hip spread wanted (Tune::oneHandSpread): one hand on a long gun, not a launcher, settled here (a hold
                                  // taken afresh at once, a change of hold once it has lasted kSpreadSettle; nothing holding it, as it was last held)
};

// A struck spring: v0 = peak x Omega / Gain puts its peak at `at`.
inline float Omega(float at, float damping) { const float s = std::sqrt(1.0f - damping * damping); return std::atan2(s, damping) / (s * at); }
inline float Gain(float damping) { const float s = std::sqrt(1.0f - damping * damping); return std::exp(-(damping / s) * std::atan2(s, damping)); }

// A number from outside: within [lo, hi], or the fallback when it isn't finite.
inline float Safe(float v, float lo, float hi, float fallback) { return std::isfinite(v) ? std::fmin(std::fmax(v, lo), hi) : fallback; }
inline bool Finite3(const float* v) { return std::isfinite(v[0]) && std::isfinite(v[1]) && std::isfinite(v[2]); }

// Hamilton products and turns of quaternions (x, y, z, w), as hands.cpp's QuatMul and QuatToMatrix.
inline void QMul(const float* a, const float* b, float* o) {
    const float x = a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1];
    const float y = a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0];
    const float z = a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3];
    const float w = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];
    o[0] = x, o[1] = y, o[2] = z, o[3] = w;
}
inline void QMatrix(const float* q, float* m) {   // row-major 3x3
    const float x = q[0], y = q[1], z = q[2], w = q[3];
    m[0] = 1 - 2 * (y * y + z * z), m[1] = 2 * (x * y - z * w), m[2] = 2 * (x * z + y * w);
    m[3] = 2 * (x * y + z * w), m[4] = 1 - 2 * (x * x + z * z), m[5] = 2 * (y * z - x * w);
    m[6] = 2 * (x * z - y * w), m[7] = 2 * (y * z + x * w), m[8] = 1 - 2 * (x * x + y * y);
}
// Radians between two turns (precise near zero).
inline float QAngle(const float* a, const float* b) {
    const float inv[4] = {-a[0], -a[1], -a[2], a[3]};
    float r[4];
    QMul(inv, b, r);
    return 2.0f * std::atan2(std::sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2]), std::fabs(r[3]));
}
// The turn a share u (0-1) of the way from none to q.
inline void QPart(const float* q, float u, float* out) {
    const float w = std::fmin(std::fabs(q[3]), 1.0f), sign = q[3] < 0.0f ? -1.0f : 1.0f;
    const float half = std::acos(w), s = std::sin(half);
    if (s < 1e-6f) {
        out[0] = out[1] = out[2] = 0.0f, out[3] = 1.0f;
        return;
    }
    const float k = std::sin(half * u) / s * sign;
    out[0] = q[0] * k, out[1] = q[1] * k, out[2] = q[2] * k, out[3] = std::cos(half * u);
}

// The kick's turn q (model space) on an aim pose's turn (x right, y up, z back), for a gun raised upDegrees from the aim (5.1): the aim that draws
// the gun as kicked. out = aim . F(Rx(-up) . q . Rx(+up)), F the half turn about y between the aim and the model (x and z flipped), normalized.
// (hands.cpp's raise[9] is Rx(-up): the drawn turn is toView . aim . F . Rx(-up) . q.)
inline void Fold(const float aim[4], const float q[4], float upDegrees, float rightDegrees, float out[4]) {
    const float half = 0.5f * (std::isfinite(upDegrees) ? upDegrees : 0.0f) * kDegree;
    float minus[4] = {-std::sin(half), 0.0f, 0.0f, std::cos(half)}, plus[4] = {std::sin(half), 0.0f, 0.0f, std::cos(half)};
    // A gun whose sights' line also lies right of its z (the AT4's): the raise with its turn about y (hands.cpp's SightsTurn), the same conjugation.
    if (rightDegrees != 0.0f && std::isfinite(rightDegrees)) {
        const float halfRight = 0.5f * rightDegrees * kDegree;
        const float turn[4] = {0.0f, std::sin(halfRight), 0.0f, std::cos(halfRight)}, back[4] = {0.0f, -std::sin(halfRight), 0.0f, std::cos(halfRight)};
        float m[4], p[4];
        QMul(minus, turn, m);
        QMul(back, plus, p);
        for (int i = 0; i < 4; ++i)
            minus[i] = m[i], plus[i] = p[i];
    }
    float t[4], inAim[4];
    QMul(minus, q, t);
    QMul(t, plus, inAim);
    const float flipped[4] = {-inAim[0], inAim[1], -inAim[2], inAim[3]};
    float r[4];
    QMul(aim, flipped, r);
    const float n = std::sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2] + r[3] * r[3]);
    const bool ok = n > 1e-6f && std::isfinite(n);
    for (int i = 0; i < 4; ++i)
        out[i] = ok ? r[i] / n : aim[i];
}
inline void Fold(const float aim[4], const float q[4], float upDegrees, float out[4]) {
    Fold(aim, q, upDegrees, 0.0f, out);
}

// ---- Inside Step ----

inline uint32_t Next(uint32_t& r) {   // xorshift32: only ever drawn at a shot
    r ^= r << 13, r ^= r >> 17, r ^= r << 5;
    return r ? r : (r = 0x9E3779B9u);
}
inline float Uniform(uint32_t& r, float lo, float hi) { return lo + (hi - lo) * static_cast<float>(Next(r) >> 8) * (1.0f / 16777216.0f); }

// The turn about the eye's weight the hold wants now (3.5): the eye at the sight with two hands on a long gun (Step eases State::eyePivot to it).
inline float EyePivotWanted(const Gun& gun, const Hold& hold, const Tune& tune, const State& s) {
    return Long(gun.kind) && tune.eyePivot && s.haveEye ? s.eye * Safe(hold.hands, 0.0f, 1.0f, 0.0f) : 0.0f;
}

// The game's hip spread for this frame (Output::hipSpread): a hold taken afresh takes its own at once; while held, a change counts once it has
// lasted kSpreadSettle (the support hand's blend flickering past a half never thrashes the game's rows); while nothing holds the gun no round can
// leave it, so the rows stay as it was last held (a quick re-take finds them as it left them).
inline bool SpreadFor(State& s, bool held, bool wanted, float seconds) {
    if (!held) {
        s.spreadHeld = false, s.spreadFor = 0.0f;
        return s.spread;
    }
    if (!s.spreadHeld || wanted == s.spread) {
        s.spread = wanted, s.spreadHeld = true, s.spreadFor = 0.0f;
        return s.spread;
    }
    s.spreadFor += std::isfinite(seconds) ? std::fmin(std::fmax(seconds, 0.0f), kGap) : 0.0f;
    if (s.spreadFor >= kSpreadSettle - 1e-5f)
        s.spread = wanted, s.spreadFor = 0.0f;
    return s.spread;
}

// This frame's numbers: the gun's own as the hold and the sight at the eye make them (3.4, 3.5).
struct Params {
    float w = 0.0f;                                  // the two-hand weight
    float gap = 0.1f;                                // seconds between the gun's rounds
    float up = 0.0f, sideMin = 0.0f, sideMax = 0.0f, decrease = 0.0f;   // A's row in use
    float capUp = 10.0f, capSide = 10.0f, firstShot = 1.0f, shootingScale = 1.0f;
    float driftW = 15.8f, driftZ = 0.35f;            // A's camera spring: its frequency (the root of its stiffness), its damping ratio
    float um = 1.0f, sm = 1.0f, toss = 0.0f, cm = 1.0f, dm = 1.0f, fm = 1.0f, driftScale = 1.0f, side = 1.0f;
    float peak[5] = {};                              // B: one round's peak from rest (back m, rise deg, -, -, lift m)
    float wn[5] = {}, z[5] = {}, gain[5] = {};       // B: each spring's natural frequency, damping ratio, gain (yaw and roll the rise's)
    float most[5] = {};                              // B: each spring's cap (lift: Tune::liftMost)
    float yawShare = 0.25f, rollShare = 0.9f, jitter = 0.1f;
    bool alternate = true;
    float heldMost = 0.0f, heldStep = 0.0f, heldW = 0.0f;   // held fire's push back: at most, each later round, its spring's frequency
    float flipV = 0.0f, flipW = 15.8f;               // the flip after held fire: its strike, its frequency
    bool release = false;
    float E = 0.0f, eyeScale = 1.0f, slide = 1.0f;   // the eye pivot's weight (two hands at the sight), the sight's factor, the slide's share
    float guard = 0.0f;                              // the eye at the sight, any hold (0-1)
    bool guardHard = false;                          // the eye given: the glass (or the gun's rear) kept 1.5 cm from it
};

inline Params Blend(const Gun& gun, const Hold& hold, const Tune& tune, const State& s, bool eyeNow) {
    Params p;
    const bool pistol = gun.kind == Kind::kPistol;
    const float w = Safe(hold.hands, 0.0f, 1.0f, 0.0f);
    auto mix = [w](float one, float two) { return one + (two - one) * w; };
    p.w = w;
    p.gap = 60.0f / (std::isfinite(gun.rate) && gun.rate >= 10.0f ? std::fmin(gun.rate, 3000.0f) : 600.0f);
    // A: the row in use (the eye's past half: the game's own aimed row), the hold's multipliers
    const Drift& d = gun.drift;
    const int row = s.eye > 0.5f ? 1 : 0;
    p.up = Safe(d.up[row], 0.0f, 45.0f, 0.0f);
    p.sideMin = Safe(d.sideMin[row], -45.0f, 45.0f, 0.0f), p.sideMax = Safe(d.sideMax[row], -45.0f, 45.0f, 0.0f);
    if (p.sideMax < p.sideMin) {
        const float t = p.sideMin;
        p.sideMin = p.sideMax, p.sideMax = t;
    }
    p.decrease = Safe(d.decrease[row], 0.0f, 1000.0f, 0.0f);
    p.capUp = Safe(d.capUp, 0.0f, 90.0f, 10.0f), p.capSide = Safe(d.capSide, 0.0f, 90.0f, 10.0f);
    p.firstShot = Safe(d.firstShot, 0.0f, 10.0f, 1.0f), p.shootingScale = Safe(d.shootingScale, 0.0f, 10.0f, 1.0f);
    const float spring = Safe(d.spring, 0.0f, 5000.0f, 250.0f);
    float zeta = spring >= 1.0f ? std::fmax(Safe(d.damping, 0.0f, 2.0f, kDriftDampingLeast), kDriftDampingLeast) : 1.0f;
    if (tune.driftDamping > 0.0f)
        zeta = Safe(tune.driftDamping, 0.05f, 2.0f, zeta);
    p.driftW = std::sqrt(spring >= 1.0f ? spring : 250.0f);
    p.driftZ = zeta;
    p.driftScale = Safe(tune.drift, 0.0f, 10.0f, 1.0f) * Safe(tune.gun, 0.0f, 10.0f, 1.0f);
    p.side = (std::isfinite(tune.sideSign) && tune.sideSign < 0.0f ? -1.0f : 1.0f) * (hold.leftTrigger && tune.mirror ? -1.0f : 1.0f);
    // B: the gun's own two-hand kick, and each hold's
    const Kick& kick = gun.kick;
    const float kickScale = Safe(tune.kick, 0.0f, 10.0f, 1.0f) * Safe(tune.gun, 0.0f, 10.0f, 1.0f);
    const float jitter = Safe(kick.jitter, 0.0f, 0.9f, 0.1f);
    float backAt = Safe(kick.back.at, 0.005f, 1.0f, 0.03f), backZ = Safe(kick.back.damping, 0.05f, 0.95f, 0.75f);
    float riseAt = Safe(kick.rise.at, 0.005f, 1.0f, 0.05f), riseZ = Safe(kick.rise.damping, 0.05f, 0.95f, 0.75f);
    float backMul, riseMul, liftMul = 1.0f;
    if (pistol) {
        p.um = mix(1.0f, 0.8f), p.sm = mix(1.0f, 0.8f);
        backMul = riseMul = mix(Safe(tune.pistolOne, 0.0f, 10.0f, 1.5f), 1.0f);
        p.jitter = jitter * mix(2.0f, 1.0f), p.yawShare = mix(0.15f, 0.1f), p.rollShare = 0.5f;
        p.most[0] = mix(0.025f, 0.02f), p.most[1] = mix(10.0f, 7.0f), p.most[2] = mix(3.0f, 2.0f), p.most[3] = mix(6.0f, 4.0f);
    } else {
        p.um = mix(2.0f, 1.0f), p.sm = mix(3.0f, 1.0f), p.toss = mix(1.0f, 0.0f), p.cm = mix(1.5f, 1.0f), p.dm = mix(0.5f, 1.0f), p.fm = mix(1.5f, 1.0f);
        backMul = mix(Safe(tune.oneBack, 0.0f, 20.0f, 2.5f), 1.0f), riseMul = mix(Safe(tune.oneRise, 0.0f, 50.0f, 5.0f), 1.0f), liftMul = mix(1.5f, 1.0f);
        p.jitter = jitter * mix(5.0f, 1.0f), p.yawShare = mix(0.8f, Safe(kick.yaw, 0.0f, 2.0f, 0.25f)), p.rollShare = mix(0.6f, Safe(kick.roll, 0.0f, 2.0f, 0.9f));
        backAt = mix(0.045f, backAt), backZ = mix(0.6f, backZ), riseAt = mix(0.07f, riseAt), riseZ = mix(0.4f, riseZ);
        p.most[0] = mix(0.06f, 0.04f), p.most[1] = mix(20.0f, 6.0f), p.most[2] = mix(10.0f, 3.0f), p.most[3] = mix(15.0f, 5.0f);
    }
    p.jitter = std::fmin(p.jitter, 0.95f);
    p.alternate = w > 0.5f;
    const float liftAt = Safe(kick.lift.at, 0.005f, 1.0f, 0.1f), liftZ = Safe(kick.lift.damping, 0.05f, 0.95f, 0.7f);
    p.most[4] = Safe(tune.liftMost, 0.0f, 0.2f, 0.04f);
    p.peak[0] = Safe(kick.back.peak, 0.0f, 0.2f, 0.0f) * backMul * kickScale;
    p.peak[1] = Safe(kick.rise.peak, 0.0f, 45.0f, 0.0f) * riseMul * kickScale;
    p.peak[4] = Safe(kick.lift.peak, 0.0f, 0.2f, 0.0f) * liftMul * kickScale;
    const float at[5] = {backAt, riseAt, riseAt, riseAt, liftAt}, damping[5] = {backZ, riseZ, riseZ, riseZ, liftZ};
    for (int i = 0; i < 5; ++i)
        p.z[i] = damping[i], p.wn[i] = Omega(at[i], damping[i]), p.gain[i] = Gain(damping[i]);
    // Held fire's push back, two hands: toward the game's own mean (heldShare x one round's back) less what the back spring already holds at this
    // rate (one round's peak / (gain x frequency x gap)), within the hold's cap; with one hand the slow spring piles up by itself.
    const float springMean = p.peak[0] / (p.gain[0] * p.wn[0] * p.gap);
    const float wanted = std::fmin(Safe(kick.heldShare, 0.0f, 5.0f, 1.8f) * p.peak[0], p.most[0]) - springMean;
    p.heldMost = w * std::fmin(std::fmax(wanted, 0.0f), kHeldMost);
    p.heldStep = kHeldStepShare * p.peak[0];
    p.heldW = 4.74f / kHeldHome;   // critically damped: within 5% of where it's going in kHeldHome
    // The flip after held fire (offered: Tune::release), at the camera spring's own frequency.
    p.release = tune.release;
    p.flipW = p.driftW;
    p.flipV = std::fmin(kFlipShare * p.peak[1], p.most[1]) * p.flipW / Gain(kFlipDamping);   // (a struck spring peaks at v0 x Gain / frequency)
    // At the eye (3.5, the critic's 2 and 13): the pivot and the sight's factor need two hands on a long gun (a cheek on the stock; Step eases its
    // weight, EyePivotWanted); the guard is the eye at the sight with any hold.
    const float factor = Safe(gun.eyeFactor, 0.0f, 2.0f, 1.0f);
    p.E = Safe(s.eyePivot, 0.0f, 1.0f, 0.0f);
    p.eyeScale = 1.0f + (factor - 1.0f) * p.E;
    p.slide = 1.0f - p.E + p.E * kEyeSlideShare * factor;
    p.guard = s.haveEye ? s.eye : 0.0f;
    p.guardHard = s.haveEye && s.haveGuard && (eyeNow || s.eye > 0.0f);
    return p;
}

// A damped spring's own motion over a step h (x'' = -wn^2 x - 2 z wn x'), exactly: x' = xx x + xv v, v' = vx x + vv v. Worked out once a frame;
// each strike then peaks where and as high as it should whatever the step (72, 60 and 36 fps draw the same curve). A damping of 1 or more is
// taken as just under it.
struct Propagator { float xx = 1.0f, xv = 0.0f, vx = 0.0f, vv = 1.0f; };
inline Propagator Over(float wn, float z, float h) {
    Propagator m;
    if (!(h > 0.0f) || !(wn > 0.0f) || !std::isfinite(wn))
        return m;
    z = std::fmin(std::fmax(z, 0.0f), 0.999f);
    const float a = z * wn, wd = wn * std::sqrt(1.0f - z * z), e = std::exp(-a * h), c = std::cos(wd * h), sd = std::sin(wd * h) / wd;
    m.xx = e * (c + a * sd), m.xv = e * sd, m.vx = -e * wn * wn * sd, m.vv = e * (c - a * sd);
    return m;
}
struct Steps { float h = 0.0f; Propagator kick[5], drift, held, flip; };   // this frame's step and each spring's motion over it
inline Steps StepsOf(const Params& p, float h) {
    Steps m;
    m.h = h;
    for (int i = 0; i < 5; ++i)
        m.kick[i] = Over(p.wn[i], p.z[i], h);
    m.drift = Over(p.driftW, p.driftZ, h);
    m.held = Over(p.heldW, 1.0f, h);
    m.flip = Over(p.flipW, kFlipDamping, h);
    return m;
}

// One step of a spring at rest at 0, with its cap: a cap below where it already is holds it there (it never pulls it in), and a spring that reaches
// its cap stops there with no speed, so it never bounces off one.
inline void Swing(Osc& o, const Propagator& m, float most) {
    const float before = std::fabs(o.x), x = o.x;
    o.x = m.xx * x + m.xv * o.v, o.v = m.vx * x + m.vv * o.v;
    const float cap = std::fmax(most, before);
    if (o.x > cap)
        o.x = cap, o.v = 0.0f;
    else if (o.x < -cap)
        o.x = -cap, o.v = 0.0f;
}
// One step of a spring following a target (A drawn, held fire drawn).
inline void Follow(Osc& o, float target, const Propagator& m) {
    const float y = o.x - target;
    o.x = target + m.xx * y + m.xv * o.v, o.v = m.vx * y + m.vv * o.v;
}
inline void Toward0(float& x, float by) { x = x > 0.0f ? std::fmax(x - by, 0.0f) : std::fmin(x + by, 0.0f); }

inline Osc& Kicks(State& s, int i) {   // B's springs in order: back, pitch, yaw, roll, lift
    switch (i) {
        case 0: return s.back;
        case 1: return s.pitch;
        case 2: return s.yaw;
        case 3: return s.roll;
        default: return s.lift;
    }
}

// One step (about 1 ms) of everything between rounds.
inline void Advance(State& s, const Params& p, const Steps& m) {
    const float h = m.h;
    if (s.since >= kStringGap * p.gap) {
        // After a string: the drift comes back, at the gun's firing rate while a round left within kFiringGaps of its gaps, then at its own.
        const float rate = p.decrease * p.dm * (s.since < kFiringGaps * p.gap ? p.shootingScale : 1.0f);
        if (kDriftLinear) {
            Toward0(s.upTarget, rate * h);
            Toward0(s.sideTarget, rate * h);
        } else {
            const float keep = std::fmax(1.0f - rate * h, 0.0f);
            s.upTarget *= keep, s.sideTarget *= keep;
        }
        // Held fire's push back comes home; the flip after held fire, once.
        if (s.string > 0) {
            if (p.release && s.string >= 2)
                s.flip.v += p.flipV;
            s.string = 0;
        }
        s.heldTarget = 0.0f;
    }
    if (s.heldTarget > p.heldMost)   // more than the hold allows now (the support hand let go): eased down to it
        s.heldTarget = p.heldMost + (s.heldTarget - p.heldMost) * std::fmax(1.0f - 3.0f * h / kHeldHome, 0.0f);
    if (s.heldBackIn >= 0.0f) {      // a burst pair's held-back share comes in
        s.heldBackIn -= h;
        if (s.heldBackIn <= 0.0f) {
            for (int i = 0; i < 5; ++i)
                Kicks(s, i).v += s.heldV[i];
            s.heldBackIn = -1.0f, s.heldBack = 0.0f;
        }
    }
    Follow(s.up, s.upTarget, m.drift);
    Follow(s.side, s.sideTarget, m.drift);
    for (int i = 0; i < 5; ++i)
        Swing(Kicks(s, i), m.kick[i], p.most[i]);
    if (s.back.x < -kReboundMost)
        s.back.x = -kReboundMost, s.back.v = 0.0f;
    Follow(s.held, s.heldTarget, m.held);
    Swing(s.flip, m.flip, p.most[1]);
    s.since += h;
}

// One round leaving: A piles up, B is struck, C's pulse.
inline void Strike(const Gun& gun, const Hold& hold, const Tune& tune, const Params& p, State& s, Output& out) {
    const bool first = s.released || s.since > 1.5f * p.gap + kFirstAfter;
    s.string = s.since < kStringGap * p.gap ? s.string + 1 : 1;
    if (first)
        s.pull = -1, s.pullRounds = 0;
    // A (3.2): up by its share of the row's up (x the first shot's), sideways a pick in its range (+ one hand's toss); a cap below where it is holds
    const float share = Uniform(s.rng, kShareLeast, 1.0f);
    const float sidePick = Uniform(s.rng, p.sideMin, p.sideMax), toss = Uniform(s.rng, -1.0f, 1.0f);
    const float up = p.up * share * (first ? p.firstShot * p.fm : 1.0f) * p.um * p.driftScale;
    s.upTarget = std::fmin(s.upTarget + up, std::fmax(p.capUp * p.cm, s.upTarget));
    const float sideMost = std::fmax(p.capSide * p.cm, std::fabs(s.sideTarget));
    const float side = s.sideTarget + p.side * (sidePick * p.sm + p.toss * toss * p.up) * p.driftScale;
    s.sideTarget = std::fmin(std::fmax(side, -sideMost), sideMost);
    // B (3.3): each spring struck for its peak from rest x this round's size; the AN-94's burst pair splits it.
    const float size = 1.0f + p.jitter * Uniform(s.rng, -1.0f, 1.0f);
    const float yawPick = Uniform(s.rng, -p.yawShare, p.yawShare);
    if (!p.alternate)
        s.rollSign = Uniform(s.rng, -1.0f, 1.0f) < 0.0f ? -1.0f : 1.0f;
    else if (s.string == 1)
        s.rollSign = Uniform(s.rng, -1.0f, 1.0f) < 0.0f ? -1.0f : 1.0f;
    else
        s.rollSign = -s.rollSign;
    float now = 1.0f;
    const float pairGap = Safe(gun.pairGap, 0.0f, 0.5f, 0.0f);
    if (pairGap > 0.0f) {
        const float burstFirst = Safe(gun.kick.burstFirst, 0.0f, 1.0f, 1.0f);
        const int mode = gun.burst >= 0 ? gun.burst : s.pull;
        if (s.heldBackIn >= 0.0f) {             // the second of a pair: the rest of the pair's kick, the first's held-back part cancelled
            now = 2.0f - burstFirst;
            s.heldBackIn = -1.0f, s.heldBack = 0.0f;
            if (gun.burst < 0)
                s.pull = 1;
        } else if (mode == 0 || (mode < 0 && s.pullRounds >= 1)) {   // automatic: whole, at once
            if (gun.burst < 0)
                s.pull = 0;
        } else {                                // a burst's first (or a pull's first, not known yet): its share now, the rest held back
            now = burstFirst;
            s.heldBack = 1.0f - burstFirst, s.heldBackIn = s.heldBack > 0.0f ? pairGap : -1.0f;
        }
    }
    const float roll = s.rollSign * (hold.leftTrigger ? -1.0f : 1.0f), yawSide = hold.leftTrigger && tune.mirror ? -1.0f : 1.0f;
    const float peaks[5] = {p.peak[0] * size, p.peak[1] * size, p.peak[1] * size * yawPick * yawSide, p.peak[1] * size * p.rollShare * roll,
                            p.peak[4] * size};
    for (int i = 0; i < 5; ++i) {
        const float v = peaks[i] * p.wn[i] / p.gain[i];
        Kicks(s, i).v += v * now;
        if (s.heldBackIn >= 0.0f && now < 1.0f)
            s.heldV[i] = v * s.heldBack;
        s.struck[i] = peaks[i];
    }
    s.share = now;
    // Held fire's push back: each later round of a string, toward what the hold allows.
    if (s.string >= 2)
        s.heldTarget = std::fmin(s.heldTarget + p.heldStep, std::fmax(p.heldMost, s.heldTarget));
    // C (3.7): the trigger hand's thump, the support hand's share with two hands, a launcher's lift.
    float low = Safe(gun.rumble[0], 0.0f, 1.0f, 0.0f), high = Safe(gun.rumble[1], 0.0f, 1.0f, 0.0f);
    const float length = Safe(gun.rumble[2], 0.0f, 2.0f, 0.0f);
    // A rocket's or a missile's launcher whose data has no rumble at all (the AT4's 0 and 0; every other launcher's data has 1 and 1): a launch's blast
    // all the same, at the SMAW's (every gun has its buzz: the user's rule).
    if ((gun.kind == Kind::kRocket || gun.kind == Kind::kMissile || gun.kind == Kind::kAt4) && low == 0.0f && high == 0.0f)
        low = high = kLauncherRumble;
    const float buzz = Safe(tune.buzz, 0.0f, 10.0f, 1.0f);
    const float amp = std::fmin((0.45f + 0.55f * std::fmax(low, high)) * buzz, 1.0f);
    const bool automatic = gun.automatic >= 0 ? gun.automatic > 0 : gun.kind == Kind::kSmg || gun.kind == Kind::kRifle || gun.kind == Kind::kMg;
    float lasts = 0.015f + 0.2f * length * (0.5f + 0.5f * low);
    if (automatic)
        lasts = std::fmin(lasts, 0.8f * p.gap);
    if (amp > 0.0f) {
        out.pulse[0] = std::fmax(out.pulse[0], amp), out.pulseFor[0] = std::fmax(out.pulseFor[0], lasts);
        if (p.w > 0.5f) {
            const float support = amp * (hold.frontGrip ? 0.8f : gun.kind == Kind::kPistol ? 0.7f : 0.6f);
            out.pulse[1] = std::fmax(out.pulse[1], support), out.pulseFor[1] = std::fmax(out.pulseFor[1], lasts);
        }
        if (gun.kick.lift.peak > 0.0f)
            out.lateIn = 0.03f, out.late = std::fmin(0.6f * buzz, 1.0f), out.lateFor = 0.12f;
    }
    s.since = 0.0f;
    s.released = false;
    ++s.pullRounds;
}

// The move the state draws (3.1): q = qy(-yaw) . qx(-pitch) . qz(roll) about the pivot, slid by s = (0, lift, -back); the eye's guard; then a clear's
// remainder on top. Fills out's move and angles; t and pivot the move's translation and its pivot.
inline void Make(const State& s, const Params& p, const float* gripPivot, float fade, Output& out, float* t, float* pivot) {
    float pitch = s.up.x + (s.pitch.x + s.flip.x) * p.eyeScale, yaw = s.side.x + s.yaw.x * p.eyeScale;
    const float roll = s.roll.x * p.eyeScale;
    const float turn = std::sqrt(pitch * pitch + yaw * yaw);
    if (turn > kTotalTurnMost)
        pitch *= kTotalTurnMost / turn, yaw *= kTotalTurnMost / turn;
    // The slide toward the eye: a share of the back with two hands at the sight; with the sight at the eye (any hold) at most 6 mm, and not past 1.5 cm
    // from the glass.
    float back = (s.back.x + s.held.x) * p.slide;
    if (p.guard > 0.0f && back > 0.0f) {
        float most = kEyeSlideMost;
        if (s.haveGuard)
            most = std::fmin(most, std::fmax(s.guardZ - s.eyeAt[2] - kEyeGlassLeast, 0.0f));
        back += (std::fmin(back, most) - back) * p.guard;
    }
    const float hy = -0.5f * yaw * kDegree, hp = -0.5f * pitch * kDegree, hr = 0.5f * roll * kDegree;
    const float qy[4] = {0.0f, std::sin(hy), 0.0f, std::cos(hy)}, qx[4] = {std::sin(hp), 0.0f, 0.0f, std::cos(hp)}, qz[4] = {0.0f, 0.0f, std::sin(hr), std::cos(hr)};
    float qa[4], q[4], m[9];
    QMul(qy, qx, qa);
    QMul(qa, qz, q);
    QMatrix(q, m);
    for (int i = 0; i < 3; ++i)
        pivot[i] = gripPivot[i] + (s.eyeAt[i] - gripPivot[i]) * p.E;
    float shift[3] = {0.0f, s.lift.x, -back};
    auto translation = [&]() {
        for (int i = 0; i < 3; ++i)
            t[i] = pivot[i] - (m[i * 3] * pivot[0] + m[i * 3 + 1] * pivot[1] + m[i * 3 + 2] * pivot[2]) +
                   (m[i * 3] * shift[0] + m[i * 3 + 1] * shift[1] + m[i * 3 + 2] * shift[2]);
    };
    translation();
    // The eye at the sight (any hold): taken into the kicked gun's space, it never comes within 1.5 cm of the glass (or the gun's rear) because of the
    // kick, nor nearer than it is as the hands hold it: the gun goes forward along its own z until it doesn't (the critic's 2).
    if (p.guardHard) {
        const float d[3] = {s.eyeAt[0] - t[0], s.eyeAt[1] - t[1], s.eyeAt[2] - t[2]};
        const float local = m[2] * d[0] + m[5] * d[1] + m[8] * d[2];
        const float limit = std::fmax(s.guardZ - kEyeGlassLeast, s.eyeAt[2]);
        if (local > limit) {
            shift[2] += local - limit;
            translation();
        }
    }
    out.pitch = pitch, out.yaw = yaw, out.roll = roll;
    out.drift[0] = s.up.x, out.drift[1] = s.side.x;
    out.back = -shift[2], out.lift = shift[1];
    // A clear's remainder (eased out): the last move a share `fade` of the way, about its own pivot, on top.
    if (fade > 0.0f) {
        float qf[4], mf[9], fullM[9];
        QPart(s.fadeQ, fade, qf);
        QMatrix(qf, mf);
        QMatrix(s.fadeQ, fullM);
        float tf[3];
        for (int i = 0; i < 3; ++i) {
            const float* r = mf + i * 3;
            const float* full = fullM + i * 3;
            tf[i] = s.fadePivot[i] - (r[0] * s.fadePivot[0] + r[1] * s.fadePivot[1] + r[2] * s.fadePivot[2]) +
                    fade * (s.fadeT[i] - s.fadePivot[i] + full[0] * s.fadePivot[0] + full[1] * s.fadePivot[1] + full[2] * s.fadePivot[2]);
        }
        float mm[9], tt[3];
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j)
                mm[i * 3 + j] = mf[i * 3] * m[j] + mf[i * 3 + 1] * m[3 + j] + mf[i * 3 + 2] * m[6 + j];
            tt[i] = mf[i * 3] * t[0] + mf[i * 3 + 1] * t[1] + mf[i * 3 + 2] * t[2] + tf[i];
        }
        float qq[4];
        QMul(qf, q, qq);
        for (int i = 0; i < 9; ++i)
            m[i] = mm[i];
        for (int i = 0; i < 3; ++i)
            t[i] = tt[i];
        for (int i = 0; i < 4; ++i)
            q[i] = qq[i];
    }
    for (int i = 0; i < 3; ++i) {
        out.k[i * 4] = m[i * 3], out.k[i * 4 + 1] = m[i * 3 + 1], out.k[i * 4 + 2] = m[i * 3 + 2], out.k[i * 4 + 3] = t[i];
    }
    for (int i = 0; i < 4; ++i)
        out.q[i] = q[i];
}

inline bool Quiet(const State& s) {
    const Osc* all[9] = {&s.up, &s.side, &s.back, &s.pitch, &s.yaw, &s.roll, &s.lift, &s.held, &s.flip};
    for (const Osc* o : all)
        if (std::fabs(o->x) > kQuiet || std::fabs(o->v) > 1e-3f)
            return false;
    return std::fabs(s.upTarget) <= 1e-4f && std::fabs(s.sideTarget) <= 1e-4f && s.heldTarget <= kQuiet && s.heldBackIn < 0.0f;
}

// Everything back to its default but the random numbers and the game's hip spread as settled (the next hold takes its own afresh); with ease, the
// last move kept to fade out (a gun still in the hand).
inline void Clear(State& s, bool ease) {
    State next;
    next.rng = s.rng;
    next.spread = s.spread;
    if (ease && (s.live || s.fade > 0.0f)) {
        next.fade = 1.0f;
        for (int i = 0; i < 4; ++i)
            next.fadeQ[i] = s.lastQ[i], next.lastQ[i] = s.lastQ[i], next.lastQ2[i] = s.lastQ2[i];
        for (int i = 0; i < 3; ++i)
            next.fadeT[i] = s.lastT[i], next.fadePivot[i] = s.lastPivot[i], next.lastT[i] = s.lastT[i], next.lastPivot[i] = s.lastPivot[i];
    }
    s = next;
}

// A clear's remainder alone, as this frame's move.
inline Output Remainder(State& s) {
    Output out;
    if (s.fade <= 0.0f)
        return out;
    Params none;
    State rest;
    for (int i = 0; i < 4; ++i)
        rest.fadeQ[i] = s.fadeQ[i];
    for (int i = 0; i < 3; ++i)
        rest.fadeT[i] = s.fadeT[i], rest.fadePivot[i] = s.fadePivot[i];
    float t[3], pivot[3];
    const float zero[3] = {};
    Make(rest, none, zero, s.fade, out, t, pivot);
    out.pitch = out.yaw = out.roll = out.back = out.lift = 0.0f;
    out.active = true;
    out.slack = QAngle(out.q, s.lastQ);
    for (int i = 0; i < 4; ++i)
        s.lastQ2[i] = s.lastQ[i], s.lastQ[i] = out.q[i];
    for (int i = 0; i < 3; ++i)
        s.lastT[i] = t[i];
    return out;
}

// This frame's move for the gun in hand (10.1 with the critic's fixes). Allocates nothing; a few microseconds a frame.
inline Output Step(const Gun& gun, const Hold& hold, const Frame& f, const Tune& tune, State& s) {
    if (f.seed)
        s.rng = f.seed;
    const float dt = std::isfinite(f.dt) ? std::fmin(std::fmax(f.dt, 0.0f), kMostStep) : 0.0f;
    const float real = std::isfinite(f.gap) && f.gap >= 0.0f ? f.gap : f.dt;
    // The game's hip spread back with one hand on a long gun (not a launcher), as the hold wants it now (SpreadFor settles it).
    const bool spreadWanted = tune.oneHandSpread && Long(gun.kind) && !Launcher(gun.kind) && Safe(hold.hands, 0.0f, 1.0f, 0.0f) < 0.5f;
    // 1. Clears: the gun leaving the hand (or another gun) at once; a long frame, or a scripted moment with it still drawn, easing out.
    if (f.newGun || gun.kind == Kind::kNone || (!hold.held && !hold.stillDrawn)) {
        Clear(s, false);
        Output none;
        none.hipSpread = SpreadFor(s, hold.held, spreadWanted, real);   // (another gun still held: its hold's at once)
        return none;
    }
    if (!hold.held) {
        if (s.live)
            Clear(s, true);
        else
            s.fade = std::fmax(s.fade - dt / kClearEase, 0.0f);
        Output rest = Remainder(s);
        rest.hipSpread = SpreadFor(s, false, false, real);
        return rest;
    }
    if (!std::isfinite(real) || real > kGap) {
        Clear(s, true);
        Output rest = Remainder(s);
        rest.hipSpread = SpreadFor(s, true, spreadWanted, real);   // (cleared: the hold's at once)
        return rest;
    }
    if (s.fade > 0.0f)
        s.fade = std::fmax(s.fade - dt / kClearEase, 0.0f);
    // 2. The eye: eased in and out; where it is, and the glass or the gun's rear, kept from the last frame they were given.
    const bool eyeNow = (hold.eye || f.haveEye) && Finite3(f.eyeAt);
    if (eyeNow) {
        for (int i = 0; i < 3; ++i)
            s.eyeAt[i] = f.eyeAt[i];
        s.haveEye = true;
    }
    if (f.haveGlass && std::isfinite(f.glassZ))
        s.guardZ = f.glassZ, s.haveGuard = true;
    else if (f.haveRear && std::isfinite(f.rearZ))
        s.guardZ = f.rearZ, s.haveGuard = true;
    const bool atEye = hold.eye && eyeNow;
    s.eye = atEye ? std::fmin(s.eye + dt / kEyeIn, 1.0f) : std::fmax(s.eye - dt / kEyeOut, 0.0f);
    // The turn about the eye's weight eased toward what the hold wants as the eye itself is: with two hands held it's the eye's own, and the support
    // hand's blend dropping at once (letting go at the sight) slides the pivot from the eye to the grip over kEyeOut instead of in a frame.
    {
        const float wanted = EyePivotWanted(gun, hold, tune, s);
        s.eyePivot = wanted > s.eyePivot ? std::fmin(s.eyePivot + dt / kEyeIn, wanted) : std::fmax(s.eyePivot - dt / kEyeOut, wanted);
    }
    // 3. Every number for this hold (and the eye).
    const Params p = Blend(gun, hold, tune, s, eyeNow);
    const float pivot0[3] = {Finite3(f.pivot) ? f.pivot[0] : 0.0f, Finite3(f.pivot) ? f.pivot[1] : 0.0f, Finite3(f.pivot) ? f.pivot[2] : 0.0f};
    // 4-6. The rounds, dated evenly over the frame from its start, and every spring stepped 1 ms at a time.
    Output out;
    const int shots = f.shots > kMostShots ? kMostShots : f.shots < 0 ? 0 : f.shots;
    const int steps = static_cast<int>(std::ceil(dt / kStep - 1e-4f));
    const Steps m = StepsOf(p, steps > 0 ? dt / static_cast<float>(steps) : kStep);   // (with no time, kStep's for the slack's look ahead)
    int fired = 0;
    for (int i = 0; i < steps || fired < shots; ++i) {
        while (fired < shots && (steps == 0 || fired * steps <= i * shots)) {
            Strike(gun, hold, tune, p, s, out);
            ++fired;
        }
        if (i < steps)
            Advance(s, p, m);
    }
    if (!f.trigger)
        s.released = true;
    // 8. The move.
    float t[3], pivot[3];
    Make(s, p, pivot0, s.fade, out, t, pivot);
    s.live = !Quiet(s);
    if (!s.live) {                       // at rest: exactly
        const Osc zero;
        s.up = s.side = s.back = s.pitch = s.yaw = s.roll = s.lift = s.held = s.flip = zero;
        s.upTarget = s.sideTarget = s.heldTarget = 0.0f;
    }
    out.active = s.live || s.fade > 0.0f;
    if (!out.active) {
        const Output none;
        for (int i = 0; i < 12; ++i)
            out.k[i] = none.k[i];
        for (int i = 0; i < 4; ++i)
            out.q[i] = none.q[i];
        for (int i = 0; i < 3; ++i)
            t[i] = 0.0f;
    }
    // 9. The scope picture's slack (the critic's 5), one frame's turn (scopeview doubles it for a picture two frames behind): the largest of this
    // frame's, the frame before's and the next with no round; plus, whenever the fire button is down (at any rate) once a round has left since the
    // gun came into the hand, a round's first frame from rest at its biggest size. Before the first round it's 0: the picture as it always was.
    // The next frame is looked at in this one's steps, or for a frame longer than kAheadMost of them (a stutter, under 36 fps) in as many longer
    // ones: the same frame ahead at under two thirds of the cost.
    {
        State next = s;
        const bool coarse = steps > kAheadMost;
        const int ahead = coarse ? kAheadMost : steps > 0 ? steps : 1;
        const Steps longer = coarse ? StepsOf(p, dt / static_cast<float>(kAheadMost)) : Steps{};
        const Steps& by = coarse ? longer : m;
        const float frame = steps > 0 ? dt : 1.0f / 72.0f;
        for (int i = 0; i < ahead; ++i)
            Advance(next, p, by);
        Output predicted;
        float tp[3], pp[3];
        Make(next, p, pivot0, std::fmax(s.fade - frame / kClearEase, 0.0f), predicted, tp, pp);
        float slack = std::fmax(std::fmax(QAngle(out.q, s.lastQ), QAngle(s.lastQ, s.lastQ2)), QAngle(predicted.q, out.q));
        if (f.trigger && s.since < 1e8f) {
            const float wn = p.wn[1], z = p.z[1], wd = wn * std::sqrt(1.0f - z * z);
            const float v0 = p.peak[1] * (1.0f + p.jitter) * p.eyeScale * wn / p.gain[1];
            const float upTo = std::fmin(frame, std::atan2(std::sqrt(1.0f - z * z), z) / wd);   // (no further than its peak)
            slack += v0 / wd * std::exp(-z * wn * upTo) * std::sin(wd * upTo) * std::sqrt(1.0f + p.yawShare * p.yawShare) * kDegree;
        }
        out.slack = std::isfinite(slack) ? slack : 0.0f;
    }
    for (int i = 0; i < 4; ++i)
        s.lastQ2[i] = s.lastQ[i], s.lastQ[i] = out.q[i];
    for (int i = 0; i < 3; ++i)
        s.lastT[i] = t[i], s.lastPivot[i] = pivot[i];
    // The game's hip spread wanted back, settled (skin::HoldSpread writes it as it comes).
    out.hipSpread = SpreadFor(s, true, spreadWanted, real);
    return out;
}

}  // namespace recoil

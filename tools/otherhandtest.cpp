// The owner's rule of 2026-11-19 (src/workhand.h; the M249 on Operation Swordbreaker's bridge, held by its front with the left, the right on its charging
// handle after laying the belt, and nothing took the handle): while one hand holds a gun, by its pistol grip or its front alone, the other works every
// part it has, either hand in either role. Run without the game on scripted frames at 72 Hz, through the very code hands.cpp runs: grasp.h (who holds
// the gun), workhand.h (the hand at work, the hand a module is fed, the chamber), handmag.cpp (every held gun's magazine, box, lever, lid and belt by
// either hand) and shellload.cpp (the M1014's handle).
//   h1  each held gun's lever by the right, over the top by its trigger and from below by its grip: taken, drawn as far as it goes, a round chambered;
//       the left's fitted hand mirrored on a handle on the gun's middle, a fist with the real wrist on one off it (the knob swung out with the G3's, the
//       UMP-45's turned up into its notch); the left's as before.
//   h2  the right's work the left's mirror image, exactly, on every part across the gun's middle: the AN-94's magazine out, the Afghan flick and a
//       fresh one home; the M4A1's and the Mk11's T handle; the M240's and the M249's lid; frame for frame, the gloves and the magazines mirrored.
//   h3  the M240's and the M249's whole belt reload by the right alone: lid up, box out, a fresh one home, the belt into the feed, lid shut, the handle.
//   h4  the hand changed with something in the last one (the gun back in the other hand): let go of, that hand's events, nothing taken by the new one.
//   h5  Claims (the regrip's guard) and Update agree for the right, as they do for the left: random squeezes round every part.
//   h6  the owner's case end to end, the M249 and the M240: the left lays the belt and shuts the lid with the right on the pistol grip, takes the
//       front, the right lets go (held by the left alone, nothing fires), pulls the handle (claimed: no regrip; the gun where the left has it), takes
//       its pistol grip back and fires; and mirrored for a lefty: the left on the pistol grip, the right lays the belt, takes the front, the left lets go
//       and pulls the handle, takes its grip back and fires.
//   h7  every held gun's lever with the gun held by its front alone, either hand, the trigger only for the hand on the pistol grip after; the
//       KH2002's magazine by the right 9.6 cm from its pistol grip claimed, never a regrip; the M1014's handle racked by the right with the left holding
//       its front (shellload.cpp's gunHand from the grasp).
//   h8  workhand.h's rules: the hand at work, the side fed, a hand no longer at work, the chamber, the trigger.
// Prints PASS or FAIL per check; the exit code is the failures.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "../src/grasp.h"
#include "../src/handmag.h"
#include "../src/handpose.h"
#include "../src/m249rows.h"
#include "../src/magposes.h"
#include "../src/shellload.h"
#include "../src/workhand.h"

namespace {

int g_checks = 0, g_failed = 0;
void Check(bool ok, const char* what) {
    ++g_checks;
    g_failed += !ok;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
}

constexpr float kPi = 3.14159265f;
constexpr float kDt = 1.0f / 72.0f;

// ---- skin.cpp's rows (copied, as heldmagtest's are; the M249's are its own header's) ----
constexpr handmag::Gun kAn94{{0.0f, 0.033f, 0.386f}, {0.0f, -0.075f, 0.410f}, {0.0f, -0.012f, 0.340f}, {0.0f, 0.005f, 0.424f}, -0.05f, {-0.034f, 0.068f, 0.495f}, 0.113f, 25.0f * kPi / 180.0f, {0.0f, -0.008f, 0.010f}, magposes::kAn94Hold, magposes::kAn94Fingers, false, false, {{0.023f, 0.348f}, {-0.065f, 0.365f}, {-0.152f, 0.404f}}, magposes::kAn94LeverHold, magposes::kAn94LeverFingers, false, {}, 0.0f, {}, 0.0f, nullptr, magposes::kAn94LeverUnderHold, magposes::kAn94LeverUnderFingers};
constexpr handmag::Gun kAk74{{0.0f, -0.002f, 0.417f}, {0.0f, -0.102f, 0.438f}, {0.0f, -0.047f, 0.372f}, {0.0f, -0.030f, 0.455f}, -0.05f, {-0.037f, 0.036f, 0.497f}, 0.099f, 25.0f * kPi / 180.0f, {0.0f, 0.0f, 0.0f}, magposes::kAk74Hold, magposes::kAk74Fingers, false, false, {{-0.012f, 0.380f}, {-0.099f, 0.396f}, {-0.187f, 0.435f}}, magposes::kAk74LeverHold, magposes::kAk74LeverFingers, false, {}, 0.0f, {}, 0.0f, nullptr, magposes::kAk74LeverUnderHold, magposes::kAk74LeverUnderFingers};
constexpr handmag::Gun kG3{{0.0f, 0.033f, 0.481f}, {0.0f, -0.055f, 0.450f}, {0.0f, -0.012f, 0.429f}, {0.0f, 0.005f, 0.525f}, -0.05f, {0.022f, 0.078f, 0.770f}, 0.17f, 10.0f * kPi / 180.0f, {0.0f, 0.0f, 0.0f}, magposes::kG3a3Hold, magposes::kG3a3Fingers, false, false, {{0.026f, 0.438f}, {-0.045f, 0.441f}, {-0.115f, 0.460f}}, magposes::kG3a3LeverHold, magposes::kG3a3LeverFingers, false, {}, 53.0f * kPi / 180.0f, {0.001f, 0.738f}};
constexpr handmag::Gun kKh2002{{0.0f, -0.010f, 0.159f}, {0.0f, -0.060f, 0.150f}, {0.0f, -0.055f, 0.123f}, {0.0f, -0.038f, 0.187f}, -0.05f, {0.0f, 0.058f, 0.392f}, 0.085f, 0.0f, {0.0f, 0.0f, 0.0f}, magposes::kKh2002Hold, magposes::kKh2002Fingers, true, false, {}, magposes::kKh2002LeverHold, magposes::kKh2002LeverFingers};
constexpr handmag::Gun kM16{{0.0f, 0.016f, 0.451f}, {0.0f, -0.136f, 0.464f}, {0.0f, -0.029f, 0.407f}, {0.0f, -0.012f, 0.487f}, -0.075f, {0.0f, 0.05f, 0.292f}, 0.08f, 0.0f, {0.0f, 0.0f, 0.0f}, magposes::kM16a4Hold, magposes::kM16a4Fingers, true, false, {}, magposes::kM16a4LeverHold, magposes::kM16a4LeverFingers, true, {0.03f, -0.015f, 0.406f}};
constexpr handmag::Gun kM4{{0.0f, 0.016f, 0.451f}, {0.0f, -0.133f, 0.461f}, {0.0f, -0.029f, 0.407f}, {0.0f, -0.012f, 0.487f}, -0.077f, {0.0f, 0.05f, 0.292f}, 0.08f, 0.0f, {0.0f, 0.0f, 0.0f}, magposes::kM4a1Hold, magposes::kM4a1Fingers, true, false, {}, magposes::kM4a1LeverHold, magposes::kM4a1LeverFingers, true, {0.03f, -0.015f, 0.406f}};
constexpr handmag::BeltGun kM240Belt{{0.068f, 0.568f}, 103.0f * kPi / 180.0f, {0.048f, 0.087f, 0.335f}, magposes::kM240LidHold, magposes::kM240LidFingers, magposes::kM240BeltHanging, magposes::kM240BeltLaid, {0.4884f, 1.0245f, 0.2538f}, magposes::kBeltPinchFingers, {0.012f, 0.0535f, 0.519f}, {0.085f, -0.035f, 0.52f},
    {{-0.001f, -0.0445f, 0.025f, -0.0445f}, {0.0f, -0.0625f, 0.027f, -0.0625f}, {0.0f, -0.0765f, 0.024f, -0.0765f}, {0.005f, -0.079f, 0.005f, -0.105f}, {-0.003f, -0.088f, -0.003f, -0.143f}}, 0.308f,
    {0.197f, -0.05f, 0.514f}};
constexpr handmag::Gun kM240{{0.0f, -0.022f, 0.508f}, {0.095f, -0.09f, 0.50f}, {0.0f, -0.067f, 0.438f}, {0.0f, -0.05f, 0.57f}, -0.05f, {-0.062f, -0.002f, 0.442f}, 0.129f, 0.0f, {0.0f, 0.0f, 0.0f}, magposes::kM240Hold, magposes::kM240Fingers, false, true, {}, magposes::kM240LeverOverHold, magposes::kM240LeverOverFingers, false, {}, 0.0f, {}, 0.0f, &kM240Belt, magposes::kM240LeverUnderHold, magposes::kM240LeverUnderFingers};
constexpr handmag::Gun kAks74u{{0.0f, 0.011f, 0.410f}, {0.0f, -0.092f, 0.430f}, {0.0f, -0.034f, 0.365f}, {0.0f, -0.017f, 0.447f}, -0.05f, {-0.037f, 0.036f, 0.4715f}, 0.094f, 25.0f * kPi / 180.0f, {0.0f, 0.0f, 0.0f}, magposes::kAks74uHold, magposes::kAks74uFingers, false, false, {{0.001f, 0.378f}, {-0.084f, 0.390f}, {-0.169f, 0.428f}}, magposes::kAks74uLeverHold, magposes::kAks74uLeverFingers, false, {}, 0.0f, {}, 0.0f, nullptr, magposes::kAks74uLeverUnderHold, magposes::kAks74uLeverUnderFingers};
constexpr handmag::Gun kUmp45{{0.0f, 0.027f, 0.436f}, {0.0f, -0.120f, 0.461f}, {0.0f, -0.018f, 0.408f}, {0.0f, -0.001f, 0.456f}, -0.054f, {0.026f, 0.061f, 0.5715f}, 0.083f, 0.0f, {0.0f, 0.0f, 0.0f}, magposes::kUmp45Hold, magposes::kUmp45Fingers, false, true, {}, magposes::kUmp45LeverHold, magposes::kUmp45LeverFingers, false, {}, 0.0f, {}, 0.0f, nullptr, nullptr, nullptr, true, 51.8f * kPi / 180.0f, {0.0f, 0.0564f}, 0.0655f};
constexpr handmag::Gun kMk11{{0.0f, -0.026f, 0.450f}, {0.0f, -0.090f, 0.450f}, {0.0f, -0.071f, 0.400f}, {0.0f, -0.054f, 0.492f}, -0.032f, {0.0f, 0.051f, 0.274f}, 0.08f, 0.0f, {0.0f, 0.0f, 0.0f}, magposes::kMk11Hold, magposes::kMk11Fingers, true, false, {}, magposes::kMk11LeverHold, magposes::kMk11LeverFingers, true, {0.024f, 0.005f, 0.401f}};

// Each gun: its rows and where the game's palms rest on it (skin.cpp's kHeldKinds: the right's on the pistol grip, the left's on the front).
struct Rig {
    const char* name;
    const handmag::Gun* gun;
    float right[3], left[3];
};
const Rig kRigs[] = {
    {"AN-94", &kAn94, {-0.023f, -0.059f, 0.192f}, {0.002f, -0.018f, 0.561f}},
    {"AK-74M", &kAk74, {-0.032f, -0.080f, 0.193f}, {0.001f, -0.031f, 0.548f}},
    {"G3A3", &kG3, {-0.034f, -0.082f, 0.241f}, {0.020f, -0.007f, 0.616f}},
    {"KH2002", &kKh2002, {-0.025f, -0.099f, 0.234f}, {0.016f, -0.059f, 0.447f}},
    {"M16A4", &kM16, {-0.054f, -0.123f, 0.256f}, {0.014f, -0.041f, 0.636f}},
    {"M4A1", &kM4, {-0.054f, -0.114f, 0.255f}, {0.012f, -0.041f, 0.612f}},
    {"M240", &kM240, {-0.023f, -0.115f, 0.196f}, {0.018f, -0.065f, 0.678f}},
    {"AKS-74u", &kAks74u, {-0.031f, -0.080f, 0.193f}, {0.003f, -0.035f, 0.517f}},
    {"UMP-45", &kUmp45, {-0.053f, -0.066f, 0.223f}, {0.004f, -0.026f, 0.538f}},
    {"Mk11", &kMk11, {-0.054f, -0.114f, 0.254f}, {0.012f, -0.041f, 0.612f}},
    {"M249", &m249rows::kMag, {m249rows::kRightPalm[0], m249rows::kRightPalm[1], m249rows::kRightPalm[2]},
     {m249rows::kLeftPalm[0], m249rows::kLeftPalm[1], m249rows::kLeftPalm[2]}},
};
const Rig& RigOf(const handmag::Gun* gun) {
    for (const Rig& r : kRigs)
        if (r.gun == gun)
            return r;
    return kRigs[0];
}

// ---- Poses: 3x4, three rows rotation | translation ----
using M = std::array<float, 12>;

M Id() { return M{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0}; }
M Mul(const M& a, const M& b) {
    M o{};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 4; ++j)
            o[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j] + (j == 3 ? a[i * 4 + 3] : 0.0f);
    return o;
}
M Inv(const M& m) {  // rigid
    M o{};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            o[i * 4 + j] = m[j * 4 + i];
    for (int i = 0; i < 3; ++i)
        o[i * 4 + 3] = -(o[i * 4] * m[3] + o[i * 4 + 1] * m[7] + o[i * 4 + 2] * m[11]);
    return o;
}
M InvAny(const M& m) {  // any invertible (the line-up's turn is written to 5 decimals)
    const float a = m[0], b = m[1], c = m[2], d = m[4], e = m[5], f = m[6], g = m[8], h = m[9], k = m[10];
    const float c0 = e * k - f * h, c1 = f * g - d * k, c2 = d * h - e * g, s = 1.0f / (a * c0 + b * c1 + c * c2);
    M o{c0 * s, (c * h - b * k) * s, (b * f - c * e) * s, 0.0f, c1 * s, (a * k - c * g) * s, (c * d - a * f) * s, 0.0f, c2 * s, (b * g - a * h) * s, (a * e - b * d) * s, 0.0f};
    for (int i = 0; i < 3; ++i)
        o[i * 4 + 3] = -(o[i * 4] * m[3] + o[i * 4 + 1] * m[7] + o[i * 4 + 2] * m[11]);
    return o;
}
std::array<float, 3> Apply(const M& m, const float* p) {
    return {m[0] * p[0] + m[1] * p[1] + m[2] * p[2] + m[3], m[4] * p[0] + m[5] * p[1] + m[6] * p[2] + m[7], m[8] * p[0] + m[9] * p[1] + m[10] * p[2] + m[11]};
}
M Load(const float* p) {
    M m;
    std::memcpy(m.data(), p, sizeof(float) * 12);
    return m;
}
M Moved(M m, const float* by) {
    m[3] += by[0], m[7] += by[1], m[11] += by[2];
    return m;
}
float Dist(const float* a, const float* b) {
    const float d[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    return std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
}
float MaxDiff(const M& a, const float* b) {
    float d = 0.0f;
    for (int i = 0; i < 12; ++i)
        d = std::max(d, std::fabs(a[i] - b[i]));
    return d;
}
float MaxDiff(const float* a, const float* b) {
    float d = 0.0f;
    for (int i = 0; i < 12; ++i)
        d = std::max(d, std::fabs(a[i] - b[i]));
    return d;
}
const M kS{-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};  // the mirror across x = 0 (a gun's middle plane; the gloves' bind spaces)

// The gun's frame in the view's space (anywhere, turned: heldmagtest's).
const M kGunPose{0.9848f, 0.0f, 0.1736f, 0.05f, 0.0f, 1.0f, 0.0f, -0.2f, -0.1736f, 0.0f, 0.9848f, -0.6f};

// Each glove's own points in its bind space (the right's the left's x negated): its hand's middle, its fist's, its palm point (hands.cpp's g_leftAnchor).
constexpr float kHandIn[2][3] = {{0.5363f, 1.0976f, 0.2092f}, {-0.5363f, 1.0976f, 0.2092f}};
constexpr float kFistIn[2][3] = {{0.5268f, 1.0601f, 0.2214f}, {-0.5268f, 1.0601f, 0.2214f}};
constexpr float kAnchorIn[2][3] = {{0.502f, 1.146f, 0.144f}, {-0.502f, 1.146f, 0.144f}};
// The left glove's line-up on its controller (hands.cpp's g_leftOffset, built in); the right's its mirror (hands.cpp's for the free right glove).
const M kLineUpL{-0.65114f, -0.68371f, -0.32949f, 1.15898f, -0.54118f, 0.72265f, -0.43004f, -0.39106f, 0.53211f, -0.10169f, -0.84054f, 0.02741f};
M LineUp(int side) { return side == 1 ? Mul(kS, Mul(kLineUpL, kS)) : kLineUpL; }

// A glove's turn (its bind space into the gun's frame) with its fingers along `fingers` and its palm facing `palm` on the gun (made square), for that side.
M Turn(int side, const float* fingersIn, const float* palmIn) {
    float bAlong[3] = {side == 1 ? -0.437f : 0.437f, -0.544f, 0.716f}, bPalm[3] = {side == 1 ? 0.639f : -0.639f, -0.748f, -0.179f};
    auto norm = [](float* v) {
        const float n = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        for (int i = 0; i < 3; ++i)
            v[i] /= n;
    };
    auto cross = [](const float* a, const float* b, float* o) { o[0] = a[1] * b[2] - a[2] * b[1], o[1] = a[2] * b[0] - a[0] * b[2], o[2] = a[0] * b[1] - a[1] * b[0]; };
    auto square = [&](float* a, float* p, float* c) {  // a kept, p made at right angles to it, c = a x p
        norm(a);
        const float d = a[0] * p[0] + a[1] * p[1] + a[2] * p[2];
        for (int i = 0; i < 3; ++i)
            p[i] -= d * a[i];
        norm(p);
        cross(a, p, c);
    };
    float bAcross[3], along[3] = {fingersIn[0], fingersIn[1], fingersIn[2]}, palm[3] = {palmIn[0], palmIn[1], palmIn[2]}, across[3];
    square(bAlong, bPalm, bAcross);
    square(along, palm, across);
    M m{};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            m[i * 4 + j] = along[i] * bAlong[j] + palm[i] * bPalm[j] + across[i] * bAcross[j];
    return m;
}
// That turn on the gun with the glove's point `bind` at `place` on its frame, into the view's space through the gun's pose.
M GloveOn(const M& gun, const M& turn, const float* bind, const float* place) {
    M local = turn;
    local[3] = local[7] = local[11] = 0.0f;  // (the turn alone: any translation it has is replaced)
    const std::array<float, 3> at = Apply(local, bind);
    local[3] = place[0] - at[0], local[7] = place[1] - at[1], local[11] = place[2] - at[2];
    return Mul(gun, local);
}
// The glove holding a table's way (hold: the part's frame into its bind space) with the part at `part` on the gun's frame.
M GloveHolding(const M& gun, const float* hold, const M& part) { return Mul(Mul(gun, part), Inv(Load(hold))); }
// The world seen in the gun's middle plane: a left glove's mirror image is the right glove W L S (W = G S G^-1), and back.
M MirrorGlove(const M& gun, const M& glove) { return Mul(Mul(Mul(gun, Mul(kS, Inv(gun))), glove), kS); }
// A part's pose (its frame into the view's) mirrored likewise.
M MirrorPart(const M& gun, const float* pose) { return Mul(Mul(Mul(gun, Mul(kS, Inv(gun))), Load(pose)), kS); }

// Fingers along -x (the left reaching across to the gun's right) or +x (the right to its left), a little forward; the palm down (over) or up (under).
M LeverTurn(int side, bool under) {
    const float fingers[3] = {side == 1 ? 0.954f : -0.954f, 0.0f, 0.3f}, palm[3] = {0.0f, under ? 1.0f : -1.0f, 0.0f};
    return Turn(side, fingers, palm);
}
// The right's front grip place on the gun: the game's left palm mirrored (gunprofile's support[1]).
std::array<float, 3> FrontOf(const Rig& rig, int side) { return {side == 1 ? -rig.left[0] : rig.left[0], rig.left[1], rig.left[2]}; }

// ---- The module alone ----
handmag::Frame Frame(const handmag::Gun& gun, int side, const M& gunPose = kGunPose) {
    handmag::Frame f;
    f.dt = kDt;
    f.gun = &gun;
    f.gunTracked = true;
    std::memcpy(f.gunPose, gunPose.data(), sizeof(f.gunPose));
    const std::array<float, 3> front = FrontOf(RigOf(&gun), side);
    std::memcpy(f.frontGrip, front.data(), sizeof(f.frontGrip));
    f.side = side;
    f.left = true;
    f.spare = true;
    f.handFree = true;
    return f;
}
void SetGlove(handmag::Frame& f, const M& glove) { std::memcpy(f.glove, glove.data(), sizeof(f.glove)); }
// Nothing carries over: everything let go of, the gun gone a frame, the lid shut and the belt laid, that side fed with its grip open.
void Settle(const handmag::Gun& gun, int side) {
    handmag::Clear();
    handmag::Frame f = Frame(gun, side);
    f.gun = nullptr;
    handmag::Update(f);
    handmag::BeltReset();
    f = Frame(gun, side);
    SetGlove(f, GloveOn(kGunPose, LeverTurn(side, false), kHandIn[side], gun.top));
    handmag::Update(f);
}

// The lever over the top by the trigger or from below by the grip, as far as it goes and let go.
struct LeverRun {
    bool taken = false, fist = false, under = false, tableAsWanted = false, onKnob = true, back = false, let = false, charged = false, notched = false;
    int sideSeen = -1;
    float most = 0.0f, offKnob = 0.0f;
};
LeverRun PullLever(const handmag::Gun& gun, int side, bool under, bool byTrigger, const M& gunPose = kGunPose) {
    LeverRun run;
    Settle(gun, side);
    const M turn = LeverTurn(side, under);
    const float lift = under ? -0.005f : 0.005f;
    const float knob[3] = {gun.lever[0], gun.lever[1] + lift, gun.lever[2]};
    handmag::Frame f = Frame(gun, side, gunPose);
    SetGlove(f, GloveOn(gunPose, turn, kFistIn[side], knob));
    handmag::Update(f);
    (byTrigger ? f.trigger : f.squeeze) = 1.0f;
    handmag::Result r = handmag::Update(f);
    run.taken = r.leverTaken;
    run.sideSeen = r.side;
    // The table the module should have: the left's own; the right's mirrored on a handle on the middle; none for the right off it.
    const bool fromBelow = under && gun.leverUnderHold;
    const float* left = fromBelow ? gun.leverUnderHold : gun.leverHold;
    const float(*leftFingers)[12] = fromBelow ? gun.leverUnderFingers : gun.leverFingers;
    const bool table = side == 0 || !handmag::OffMiddle(gun.lever[0]);
    float want[12];
    if (side == 1)
        handmag::MirrorHold(left, 0.0f, want);
    else
        std::memcpy(want, left, sizeof(want));
    const float travel = gun.leverTravel + 0.01f;
    const int frames = 36;
    for (int i = 1; i <= frames && run.taken; ++i) {
        const float back = travel * i / frames;
        const float at[3] = {knob[0], knob[1], knob[2] - back};
        SetGlove(f, GloveOn(gunPose, turn, kFistIn[side], at));
        r = handmag::Update(f);
        run.fist = run.fist || r.fist;
        run.most = std::max(run.most, r.leverBack);
        if (i > 6 && r.gloveBlend >= 1.0f) {
            if (table) {
                // On the table's place, drawn back with the lever (and swung out, a handle that swings).
                const float c = std::cos(r.leverSwing), s = std::sin(r.leverSwing), px = gun.leverPivot[0], pz = gun.leverPivot[1];
                const M backM{c, 0, s, px - (c * px + s * pz), 0, 1, 0, 0, -s, 0, c, pz - (-s * px + c * pz) - r.leverBack};
                const M expect = Mul(Mul(gunPose, backM), Inv(Load(want)));
                run.offKnob = std::max(run.offKnob, MaxDiff(expect, r.glove));
                run.tableAsWanted = r.leverFingers == leftFingers;
            } else {
                // A fist on the knob where the handle has gone, the turn the real hand's.
                const float c = std::cos(r.leverSwing), s = std::sin(r.leverSwing), px = gun.leverPivot[0], pz = gun.leverPivot[1];
                const M backM{c, 0, s, px - (c * px + s * pz), 0, 1, 0, 0, -s, 0, c, pz - (-s * px + c * pz) - r.leverBack};
                const std::array<float, 3> knobNow = Apply(Mul(gunPose, backM), gun.lever);
                const std::array<float, 3> fistAt = Apply(Load(r.glove), kFistIn[side]);
                run.offKnob = std::max(run.offKnob, Dist(knobNow.data(), fistAt.data()));
                const M real = GloveOn(gunPose, turn, kFistIn[side], at);
                float turnOff = 0.0f;
                for (int k = 0; k < 3; ++k)
                    for (int j = 0; j < 3; ++j)
                        turnOff = std::max(turnOff, std::fabs(real[k * 4 + j] - r.glove[k * 4 + j]));
                run.offKnob = std::max(run.offKnob, turnOff);
                run.tableAsWanted = r.leverFingers == nullptr;
            }
        }
        if (r.notched) {
            run.notched = true;
            break;
        }
    }
    run.back = run.most >= gun.leverTravel - 0.002f || run.notched;
    (byTrigger ? f.trigger : f.squeeze) = 0.0f;
    r = handmag::Update(f);
    run.let = r.leverLet || run.notched;
    run.charged = r.charged;
    return run;
}

// The seated magazine taken by that side's fitted hold (the right's mirrored about the part's middle) and drawn down, rocked off its hook or slid out.
bool PullMagazine(const handmag::Gun& gun, int side, handmag::Frame& f) {
    float hold[12];
    handmag::HoldFor(gun, hold, side);
    f.squeeze = 0.0f;
    SetGlove(f, GloveHolding(kGunPose, hold, Id()));
    handmag::Update(f);
    f.squeeze = 1.0f;
    if (!handmag::Update(f).grabbed)
        return false;
    for (int i = 1; i <= 40; ++i) {
        const float down[3] = {0.0f, -0.004f * i, 0.0f};
        SetGlove(f, GloveHolding(kGunPose, hold, Moved(Id(), down)));
        if (handmag::Update(f).pulledOut)
            return true;
    }
    return false;
}

// ---- The hands and the grasp together (h6, h7): as hands.cpp runs them each frame ----
struct Sim {
    const Rig& rig;
    const handmag::Gun& gun;
    grasp::State gs;
    grasp::Result gr;
    M gunRoom;                 // the gun's frame in the room (= the view's space here), as drawn this frame
    M hand[2];                 // each controller's grip pose
    float squeeze[2] = {}, trigger[2] = {};
    bool onFront[2] = {};      // LauncherSupport's hold on the front (hands.cpp, after handmag)
    bool frontFresh[2] = {true, true};
    bool busyWas[2] = {};      // last frame: that hand had something of the module's
    bool spent[2] = {};
    bool onGrip[2] = {};
    bool shoulder[2] = {};     // a fresh squeeze at the belly this frame (the pouch's reach, scripted)
    M onBarrel = Id();
    bool magOut = false, dry = true, beltBlocked = false, notchBlocked = false;
    handmag::Result hr;
    int fed = 0, work = 0;
    bool claimed[2] = {};
    // What happened, for the checks.
    int regrips = 0, letGos = 0, toBarrel = -1, gunJumps = 0;
    bool firedOnce[2] = {};
    std::vector<std::string> events;
    explicit Sim(const Rig& r) : rig(r), gun(*r.gun) {
        gunRoom = Moved(Id(), std::array<float, 3>{0.1f, 1.2f, -0.4f}.data());
        for (int s = 0; s < 2; ++s)
            hand[s] = Moved(Id(), std::array<float, 3>{s == 1 ? 0.3f : -0.3f, 0.9f, -0.2f}.data());
        hand[1] = PistolGripControllerAt(1);
    }
    std::array<float, 3> GripPoint(int side) const { return {side == 1 ? rig.right[0] : -rig.right[0], rig.right[1], rig.right[2]}; }
    // A controller holding the gun as drawn now by that side's pistol grip: at its grip point, turned as the gun (the gun in that hand: hand [I | -grip]).
    M PistolGripControllerAt(int side) const {
        const std::array<float, 3> g = GripPoint(side);
        return Mul(gunRoom, Moved(Id(), g.data()));
    }
    M GloveOf(int side) const { return Mul(hand[side], LineUp(side)); }
    std::array<float, 3> PalmOf(int side) const { return Apply(GloveOf(side), kAnchorIn[side]); }
    void PlaceGlove(int side, const M& glove) { hand[side] = Mul(glove, InvAny(LineUp(side))); }
    // The gun where the holding hand alone has it.
    M GunFromHands() const {
        if ((gr.holder == grasp::Holder::kHeld || gr.holder == grasp::Holder::kGlide) && (gr.trigger == 0 || gr.trigger == 1)) {
            const std::array<float, 3> g = GripPoint(gr.trigger);
            const float back[3] = {-g[0], -g[1], -g[2]};
            return Mul(hand[gr.trigger], Moved(Id(), back));
        }
        if (gr.holder == grasp::Holder::kBarrel && (gr.barrel == 0 || gr.barrel == 1))
            return Mul(hand[gr.barrel], onBarrel);
        return gunRoom;
    }
    void Step(int frames = 1) {
        for (int n = 0; n < frames; ++n)
            StepOne();
    }
    void StepOne() {
        // The grasp (FreeHands): each hand as seen and read, the gun as drawn last frame, the claim of the hand at work while it's held by its front alone.
        grasp::Frame f;
        f.dt = kDt;
        f.enabled = true;
        for (int s = 0; s < 2; ++s) {
            grasp::HandIn& h = f.hand[s];
            h.tracked = h.own = h.read = true;
            std::memcpy(h.pose, hand[s].data(), sizeof(h.pose));
            h.squeeze = squeeze[s];
            h.trigger = trigger[s];
            h.busy = busyWas[s];
            h.onSupport = onFront[s];
            claimed[s] = false;
        }
        if (gs.holder == grasp::Holder::kBarrel && (gs.barrel == 0 || gs.barrel == 1)) {
            const int g = 1 - gs.barrel;
            if (!onFront[g]) {
                const M glove = GloveOf(g);
                const std::array<float, 3> palm = PalmOf(g);
                claimed[g] = f.hand[g].claimed = handmag::Claims(gun, gunRoom.data(), glove.data(), palm.data(), g);
            }
        }
        f.haveGun = true;
        std::memcpy(f.gunRoom, gunRoom.data(), sizeof(f.gunRoom));
        for (int s = 0; s < 2; ++s) {
            const std::array<float, 3> g = GripPoint(s);
            std::memcpy(f.gripPoint[s], g.data(), sizeof(f.gripPoint[s]));
        }
        f.boxMin[0] = -0.05f, f.boxMin[1] = -0.3f, f.boxMin[2] = -0.1f, f.boxMax[0] = 0.05f, f.boxMax[1] = 0.12f, f.boxMax[2] = 1.1f;
        const grasp::Holder was = gs.holder;
        gr = grasp::Update(f, gs);
        if (gr.toBarrel >= 0) {
            onBarrel = Mul(Inv(hand[gr.toBarrel]), gunRoom);
            toBarrel = gr.toBarrel;
            events.push_back(std::string("held by the ") + (gr.toBarrel ? "right" : "left") + "'s front alone");
        }
        if (gr.regrip >= 0)
            ++regrips, events.push_back(std::string("regrip by the ") + (gr.regrip ? "right" : "left"));
        if (gr.released >= 0)
            ++letGos;
        (void)was;
        for (int s = 0; s < 2; ++s) {
            spent[s] = gr.spent[s];
            onGrip[s] = (gr.holder == grasp::Holder::kHeld || gr.holder == grasp::Holder::kGlide) && gr.trigger == s;
        }
        const M before = gunRoom;
        gunRoom = GunFromHands();
        if (gr.holder != grasp::Holder::kGlide && MaxDiff(before, gunRoom.data()) > 0.02f)
            ++gunJumps;
        // The hand at work and the hand the module is fed (workhand.h, as hands.cpp has them).
        work = workhand::WorkHand(true, gr.holder, gr.trigger, gr.barrel);
        const int busy = handmag::BusySide();
        fed = workhand::FedSide(busy, work, true, false);
        handmag::Frame hf;
        hf.dt = kDt;
        hf.gun = &gun;
        hf.gunTracked = true;
        std::memcpy(hf.gunPose, gunRoom.data(), sizeof(hf.gunPose));
        hf.side = fed;
        hf.left = true;
        const M glove = GloveOf(fed);
        std::memcpy(hf.glove, glove.data(), sizeof(hf.glove));
        hf.squeeze = squeeze[fed];
        hf.trigger = trigger[fed];
        const std::array<float, 3> front = FrontOf(rig, fed);
        std::memcpy(hf.frontGrip, front.data(), sizeof(hf.frontGrip));
        hf.shoulder = shoulder[fed];
        hf.wellEmpty = magOut;
        hf.spare = true;
        hf.handFree = !onFront[fed] && !onGrip[fed] && !spent[fed];
        const bool gunLeftHands = gr.holder == grasp::Holder::kLoose || gr.holder == grasp::Holder::kStowed || (gr.holder == grasp::Holder::kGlide && gr.glideFromWorld);
        hf.dropHeld = gunLeftHands || workhand::NoLongerAtWork(busy, work, true, gr.holder == grasp::Holder::kLoose);
        hr = handmag::Update(hf);
        const char* by = hr.side == 1 ? " (right)" : " (left)";
        if (hr.spentSqueeze)
            frontFresh[hr.side] = false;
        if (hr.pulledOut || hr.flicked)
            magOut = true, events.push_back(std::string("magazine out") + by);
        if (hr.pulledOut && gun.belt)
            dry = true;
        if (hr.seated)
            magOut = false, events.push_back(std::string("magazine home") + by);
        if (hr.took)
            events.push_back(std::string("a fresh one at the belly") + by);
        if (hr.lidUp)
            events.push_back("lid up");
        if (hr.lidShut)
            events.push_back("lid shut");
        if (hr.beltLaid)
            events.push_back(std::string("belt laid") + by);
        if (hr.leverTaken)
            events.push_back(std::string("lever taken") + by);
        if (hr.leverLet) {
            events.push_back(std::string("lever let go") + by);
            if (workhand::LeverChambers(hr.charged, dry, magOut, hr.beltReady, true, 100))
                dry = false, events.push_back("a round chambered");
        }
        if (hr.knocked && dry && !magOut)
            dry = false, events.push_back("knocked out of its notch: a round chambered");
        beltBlocked = gun.belt && !hr.beltReady;
        notchBlocked = hr.leverHeld;
        // LauncherSupport (after handmag): the hand that may hold the front (the one not on the pistol grip, or the one holding it by its front alone)
        // takes it on a fresh squeeze within 25 cm, lets go once its grip is open; not with the module's work in it.
        const int supportSide = gr.holder == grasp::Holder::kHeld ? 1 - gr.trigger : gr.holder == grasp::Holder::kBarrel ? gr.barrel : -1;
        const bool workBy[2] = {hr.side == 0 && (hr.holding || hr.fist || hr.took || hr.grabbed), hr.side == 1 && (hr.holding || hr.fist || hr.took || hr.grabbed)};
        for (int s = 0; s < 2; ++s) {
            if (squeeze[s] < 0.4f)
                frontFresh[s] = true;
            const std::array<float, 3> palm = PalmOf(s), place = FrontOf(rig, s);
            const std::array<float, 3> palmOnGun = Apply(Inv(gunRoom), palm.data());
            if (s == supportSide && !onFront[s] && squeeze[s] > 0.6f && frontFresh[s] && !workBy[s] && Dist(palmOnGun.data(), place.data()) < 0.25f)
                onFront[s] = true, frontFresh[s] = false;
            else if (onFront[s] && (squeeze[s] < 0.4f || s != supportSide))
                onFront[s] = false;
            busyWas[s] = workBy[s];
        }
        // The trigger: the game fires only for the hand whose trigger the grasp says fires it, the gun ready.
        for (int s = 0; s < 2; ++s)
            if (trigger[s] > 0.6f && gr.fireHand == s && workhand::HeldMayFire(dry, beltBlocked, notchBlocked))
                firedOnce[s] = true;
    }
    // That hand's glove to this one over these seconds (its middle along a line, its turn the target's), grip and trigger as given.
    void Glide(int side, const M& to, float seconds, float sq, float tr) {
        const M from = GloveOf(side);
        const int frames = std::max(1, static_cast<int>(std::lround(seconds * 72.0f)));
        for (int i = 1; i <= frames; ++i) {
            M g = to;
            const float u = static_cast<float>(i) / frames;
            for (int k = 0; k < 3; ++k)
                g[k * 4 + 3] = from[k * 4 + 3] + (to[k * 4 + 3] - from[k * 4 + 3]) * u;
            PlaceGlove(side, g);
            squeeze[side] = sq, trigger[side] = tr;
            StepOne();
        }
    }
    bool Saw(const char* what) const {
        for (const std::string& e : events)
            if (e == what)
                return true;
        return false;
    }
};

// A glove on the gun (as drawn now) by that side's way: its point `bind` at `place` on the gun's frame.
M GloveAtGun(const Sim& sim, const M& turn, const float* bind, const float* place) { return GloveOn(sim.gunRoom, turn, bind, place); }
// The palm-up hold on a front (heldmagtest's game hold on a handguard), that side's.
M FrontTurn(int side) {
    const float fingers[3] = {side == 1 ? 0.45f : -0.45f, 0.33f, 0.83f}, palm[3] = {side == 1 ? 0.17f : -0.17f, 0.88f, -0.44f};
    return Turn(side, fingers, palm);
}
// The front taken by that hand: to its place, its palm point `below` under the game's own (a support hand under the handguard), and squeezed.
void TakeFront(Sim& sim, int side, float below = 0.03f) {
    std::array<float, 3> place = FrontOf(sim.rig, side);
    place[1] -= below;
    sim.Glide(side, GloveAtGun(sim, FrontTurn(side), kAnchorIn[side], place.data()), 0.3f, 0.0f, 0.0f);
    sim.Glide(side, sim.GloveOf(side), 0.1f, 1.0f, 0.0f);
}
// The pistol grip's hand lets go (it's been squeezed since it took it: armed).
void LetGoOfGrip(Sim& sim, int side) {
    sim.squeeze[side] = 1.0f;
    sim.Step(3);
    sim.squeeze[side] = 0.0f;
    sim.Step(8);
}
// The hand at work takes its pistol grip back: to it (its controller at its grip point, turned as the gun) and a fresh squeeze.
void TakeGrip(Sim& sim, int side) {
    sim.squeeze[side] = 0.0f, sim.trigger[side] = 0.0f;
    sim.Step(4);
    const M at = sim.PistolGripControllerAt(side);
    for (int i = 0; i < 20; ++i) {
        sim.hand[side] = at;
        sim.Step();
    }
    sim.squeeze[side] = 1.0f;
    sim.Step(20);
}
// The lever over the top by the hand at work's trigger, drawn back all the way and let go; the hand then away from it.
void LeverBy(Sim& sim, int side, bool under, bool byTrigger) {
    const handmag::Gun& g = sim.gun;
    const M turn = LeverTurn(side, under);
    const float knob[3] = {g.lever[0], g.lever[1] + (under ? -0.005f : 0.005f), g.lever[2]};
    const float sq = byTrigger ? 0.0f : 1.0f, tr = byTrigger ? 1.0f : 0.0f;
    sim.Glide(side, GloveAtGun(sim, turn, kFistIn[side], knob), 0.4f, 0.0f, 0.0f);
    sim.Glide(side, sim.GloveOf(side), 0.1f, sq, tr);
    const float back[3] = {knob[0], knob[1], knob[2] - g.leverTravel - 0.01f};
    sim.Glide(side, GloveAtGun(sim, turn, kFistIn[side], back), 0.6f, sq, tr);
    sim.Glide(side, sim.GloveOf(side), 0.1f, 0.0f, 0.0f);
    const float away[3] = {knob[0] + (side == 1 ? -0.25f : 0.25f), knob[1] - 0.15f, knob[2] - 0.2f};
    sim.Glide(side, GloveAtGun(sim, turn, kFistIn[side], away), 0.3f, 0.0f, 0.0f);
}
// The trigger of that hand pulled and let go.
void Fire(Sim& sim, int side) {
    sim.trigger[side] = 0.0f;
    sim.Step(3);
    sim.trigger[side] = 1.0f;
    sim.Step(6);
    sim.trigger[side] = 0.0f;
    sim.Step(3);
}

// The belt reload by the hand at work (side), the other holding the gun: the lid up, the box out, a fresh one home, the belt into the feed, the lid shut.
// What happened, in words; false where a step took nothing.
struct BeltRun {
    bool lidUp = false, boxOut = false, fresh = false, home = false, beltTaken = false, laid = false, lidShut = false, ready = false;
};
float LidAngleAt(const handmag::BeltGun& b, float angle, int side, float* out) {
    const float c = std::cos(angle), s = std::sin(angle), y = b.lidGrab[1] - b.lidHinge[0], z = b.lidGrab[2] - b.lidHinge[1];
    out[0] = side == 1 ? -b.lidGrab[0] : b.lidGrab[0];
    out[1] = b.lidHinge[0] + c * y - s * z;
    out[2] = b.lidHinge[1] + s * y + c * z;
    return angle;
}
BeltRun BeltReload(Sim& sim, int side) {
    BeltRun run;
    const handmag::Gun& g = sim.gun;
    const handmag::BeltGun& b = *g.belt;
    float lidHold[12];
    if (side == 1)
        handmag::MirrorHold(b.lidHold, 0.0f, lidHold);
    else
        std::memcpy(lidHold, b.lidHold, sizeof(lidHold));
    const M lidTurn = Inv(Load(lidHold));
    auto lidGlove = [&](float angle) {
        float at[3];
        LidAngleAt(b, angle, side, at);
        // (The fitted hold's hand's middle at the lid's back end, turned up with it about the hinge's x.)
        const float c = std::cos(angle), s = std::sin(angle), hy = b.lidHinge[0], hz = b.lidHinge[1];
        const M turnUp{1, 0, 0, 0, 0, c, -s, hy - (c * hy - s * hz), 0, s, c, hz - (s * hy + c * hz)};
        M local = Mul(turnUp, lidTurn);
        const std::array<float, 3> mid = Apply(local, kHandIn[side]);
        local[3] += at[0] - mid[0], local[7] += at[1] - mid[1], local[11] += at[2] - mid[2];
        return Mul(sim.gunRoom, local);
    };
    // The lid up.
    sim.Glide(side, lidGlove(0.0f), 0.4f, 0.0f, 0.0f);
    sim.Glide(side, sim.GloveOf(side), 0.1f, 1.0f, 0.0f);
    for (int i = 1; i <= 40 && !sim.Saw("lid up"); ++i)
        sim.Glide(side, lidGlove(std::min(0.025f * i, 0.9f)), kDt, 1.0f, 0.0f);
    sim.Glide(side, sim.GloveOf(side), 0.2f, 0.0f, 0.0f);
    sim.Step(20);
    run.lidUp = sim.Saw("lid up");
    // The box: by that side's fitted hold where its own place is (the right's mirrored about the box's middle), taken, drawn off: down and out, or along its
    // way; let go of (it falls).
    float hold[12];
    handmag::HoldFor(g, hold, side);
    const float own[3] = {side == 1 ? 2.0f * handmag::PartMiddle(g) - g.grab[0] : g.grab[0], g.grab[1], g.grab[2]};
    M boxTurn = Inv(Load(hold));
    // (A box off the gun's middle taken by the right is held as the hand is: any turn; its palm sideways under the box.)
    auto boxGlove = [&](const float* move) {
        const float at[3] = {own[0] + move[0], own[1] + move[1], own[2] + move[2]};
        return GloveAtGun(sim, boxTurn, kHandIn[side], at);
    };
    const float none[3] = {0, 0, 0};
    sim.Glide(side, boxGlove(none), 0.4f, 0.0f, 0.0f);
    sim.Glide(side, sim.GloveOf(side), 0.1f, 1.0f, 0.0f);
    for (int i = 1; i <= 80 && !sim.magOut; ++i) {
        float move[3] = {0.0f, -0.003f * i, 0.0f};
        if (g.way) {
            // Along its own way off: its grab point's path.
            const float along = 0.002f * i;
            int k = 0;
            while (k + 2 < g.way->offCount && g.way->off[k + 1][0] < along)
                ++k;
            const M a = Load(g.way->off[k] + 1), c = Load(g.way->off[k + 1] + 1);
            const std::array<float, 3> pa = Apply(a, g.grab), pc = Apply(c, g.grab);
            const float span = g.way->off[k + 1][0] - g.way->off[k][0], u = span > 1e-6f ? (along - g.way->off[k][0]) / span : 1.0f;
            for (int j = 0; j < 3; ++j)
                move[j] = pa[j] + (pc[j] - pa[j]) * u - g.grab[j];
        }
        sim.Glide(side, boxGlove(move), kDt, 1.0f, 0.0f);
    }
    run.boxOut = sim.magOut;
    sim.Glide(side, sim.GloveOf(side), 0.1f, 0.0f, 0.0f);
    sim.Step(10);
    // A fresh one at the belly (the pouch's reach, its squeeze), brought to the well: home by itself.
    sim.shoulder[side] = true;
    sim.Glide(side, sim.GloveOf(side), kDt, 1.0f, 0.0f);
    sim.shoulder[side] = false;
    run.fresh = sim.Saw(side == 1 ? "a fresh one at the belly (right)" : "a fresh one at the belly (left)");
    M seatNear = Moved(Id(), std::array<float, 3>{0.0f, g.mouth - 0.04f, 0.0f}.data());
    if (g.way) {
        seatNear = Load(g.way->on[0] + 1);
        seatNear[3] += side == 1 ? -0.03f : 0.03f, seatNear[7] -= 0.04f;
    }
    const M freshGlove = GloveHolding(sim.gunRoom, hold, seatNear);
    for (int i = 0; i < 90 && sim.magOut; ++i)
        sim.Glide(side, freshGlove, kDt, 1.0f, 0.0f);
    run.home = !sim.magOut;
    sim.Glide(side, sim.GloveOf(side), 0.1f, 0.0f, 0.0f);
    sim.Step(10);
    // The belt: pinched at its hanging tip (that side's pinch), brought to the feed (from that hand's side), taken by it.
    float tip[3];
    handmag::HangingTip(b, tip);
    const float pinch[3] = {side == 1 ? -b.pinch[0] : b.pinch[0], b.pinch[1], b.pinch[2]};
    const float palmUp[3] = {0.0f, side == 1 ? -0.2f : -0.2f, 1.0f}, fingersIn[3] = {side == 1 ? 1.0f : -1.0f, 0.0f, 0.0f};
    const M pinchTurn = Turn(side, fingersIn, palmUp);
    sim.Glide(side, GloveAtGun(sim, pinchTurn, pinch, tip), 0.4f, 0.0f, 0.0f);
    sim.Glide(side, sim.GloveOf(side), 0.1f, 1.0f, 0.0f);
    run.beltTaken = sim.hr.leverFingers == b.pinchFingers || sim.busyWas[side];
    const float feed[3] = {b.feed[0] + (side == 1 ? -0.06f : 0.06f), b.feed[1] + 0.02f, b.feed[2]};
    for (int i = 0; i < 60 && !sim.Saw(side == 1 ? "belt laid (right)" : "belt laid (left)"); ++i)
        sim.Glide(side, GloveAtGun(sim, pinchTurn, pinch, feed), i < 30 ? 0.02f : kDt, 1.0f, 0.0f);
    run.laid = sim.Saw(side == 1 ? "belt laid (right)" : "belt laid (left)");
    sim.Glide(side, sim.GloveOf(side), 0.1f, 0.0f, 0.0f);
    sim.Step(10);
    // The lid shut: taken up there and brought down.
    sim.Glide(side, lidGlove(b.lidOpen), 0.4f, 0.0f, 0.0f);
    sim.Glide(side, sim.GloveOf(side), 0.1f, 1.0f, 0.0f);
    for (int i = 1; i <= 60 && !sim.Saw("lid shut"); ++i)
        sim.Glide(side, lidGlove(std::max(b.lidOpen - 0.03f * i, 0.0f)), kDt, 1.0f, 0.0f);
    sim.Glide(side, sim.GloveOf(side), 0.2f, 0.0f, 0.0f);
    sim.Step(20);
    run.lidShut = sim.Saw("lid shut");
    run.ready = sim.hr.beltReady;
    return run;
}

std::string Words(const std::vector<std::string>& events) {
    std::string s;
    for (const std::string& e : events)
        s += (s.empty() ? "" : "; ") + e;
    return s;
}

}  // namespace

int main() {
    char what[2000];

    // ---- h1: each held gun's lever by either hand, over by its trigger and under by its grip ----
    for (const Rig& rig : kRigs) {
        for (int side = 0; side < 2; ++side) {
            for (int way = 0; way < 2; ++way) {
                const bool under = way == 1;
                const LeverRun run = PullLever(*rig.gun, side, under, !under);
                const bool table = side == 0 || !handmag::OffMiddle(rig.gun->lever[0]);
                const bool notch = rig.gun->leverNotch > 0.0f;
                const bool ok = run.taken && run.sideSeen == side && run.fist && run.back && run.let && (run.charged || notch) && run.tableAsWanted && run.offKnob < 0.002f;
                std::snprintf(what, sizeof(what), "h1 the %s's lever by the %s, %s by its %s: taken %d (the %s hand's), drawn %.1f cm of %.1f, let go %d, chambered %d%s; %s %d, the glove %.1f mm off it at most",
                              rig.name, side ? "right" : "left", under ? "from below" : "over the top", under ? "grip" : "trigger", run.taken, run.sideSeen == 1 ? "right" : run.sideSeen == 0 ? "left" : "no",
                              run.most * 100.0f, rig.gun->leverTravel * 100.0f, run.let, run.charged, notch ? (run.notched ? " (turned up into its notch: the bolt held open)" : "") : "",
                              table ? (side == 1 ? "the left's fitted hand mirrored on it (a handle on the gun's middle)" : "the left's fitted hand on it") : "a fist with the real wrist on the knob",
                              run.tableAsWanted, run.offKnob * 1000.0f);
                Check(ok, what);
            }
        }
    }

    // ---- h2: the right's work the left's mirror image, frame for frame, on the parts across the gun's middle ----
    {
        // One run of a script for a side: each frame's glove (the left's; the right's its mirror image) and grip, the gun the same; its results.
        auto run = [&](const handmag::Gun& gun, int side, const std::vector<std::pair<M, float>>& script, bool wellEmptyFrom, int fromFrame, bool shoulderAt, int shoulderFrame,
                       std::vector<handmag::Result>& out) {
            Settle(gun, side);
            out.clear();
            bool wellEmpty = false;
            for (size_t i = 0; i < script.size(); ++i) {
                handmag::Frame f = Frame(gun, side);
                const M glove = side == 1 ? MirrorGlove(kGunPose, script[i].first) : script[i].first;
                SetGlove(f, glove);
                f.squeeze = script[i].second;
                f.wellEmpty = wellEmpty || (wellEmptyFrom && static_cast<int>(i) >= fromFrame);
                f.shoulder = shoulderAt && static_cast<int>(i) == shoulderFrame;
                const handmag::Result r = handmag::Update(f);
                if (r.pulledOut || r.flicked)
                    wellEmpty = true;
                if (r.seated)
                    wellEmpty = false;
                out.push_back(r);
            }
        };
        auto compare = [&](const handmag::Gun& gun, const char* name, const std::vector<std::pair<M, float>>& script, bool wellEmptyFrom, int fromFrame, bool shoulderAt,
                           int shoulderFrame, const char* events) {
            std::vector<handmag::Result> left, right;
            run(gun, 0, script, wellEmptyFrom, fromFrame, shoulderAt, shoulderFrame, left);
            run(gun, 1, script, wellEmptyFrom, fromFrame, shoulderAt, shoulderFrame, right);
            int eventsApart = 0, sides = 0;
            float gloveOff = 0.0f, magOff = 0.0f;
            int happened = 0;
            std::string firstApart;
            for (size_t i = 0; i < left.size(); ++i) {
                const handmag::Result &a = left[i], &b = right[i];
                const bool ea[] = {a.took, a.grabbed, a.toGrip, a.pulledOut, a.touched, a.flicked, a.caught, a.seated, a.letGo, a.leverTaken, a.leverLet, a.charged, a.lidTaken, a.lidUp, a.lidShut, a.mag, a.holding, a.fist};
                const bool eb[] = {b.took, b.grabbed, b.toGrip, b.pulledOut, b.touched, b.flicked, b.caught, b.seated, b.letGo, b.leverTaken, b.leverLet, b.charged, b.lidTaken, b.lidUp, b.lidShut, b.mag, b.holding, b.fist};
                const int apartBefore = eventsApart;
                for (size_t k = 0; k < std::size(ea); ++k)
                    eventsApart += ea[k] != eb[k], happened += ea[k] && k < 15;
                sides += a.side != 0 || b.side != 1;
                if (a.gloveBlend > 0.0f || b.gloveBlend > 0.0f) {
                    gloveOff = std::max(gloveOff, std::fabs(a.gloveBlend - b.gloveBlend));
                    gloveOff = std::max(gloveOff, MaxDiff(MirrorGlove(kGunPose, Load(a.glove)), b.glove));
                }
                if (a.mag && b.mag)
                    magOff = std::max(magOff, MaxDiff(MirrorPart(kGunPose, a.magPose), b.magPose));
                if (a.leverFingers != b.leverFingers)
                    ++eventsApart;
                if (std::fabs(a.leverBack - b.leverBack) > 2e-4f || std::fabs(a.lid - b.lid) > 2e-4f)  // (the mirrored glove's own float noise, as the gloves')
                    ++eventsApart;
                if (eventsApart != apartBefore && firstApart.empty()) {
                    char at[300];
                    std::string bools;
                    for (size_t k = 0; k < std::size(ea); ++k)
                        bools += ea[k] != eb[k] ? std::to_string(k) + "(" + std::to_string(ea[k]) + "/" + std::to_string(eb[k]) + ") " : "";
                    std::snprintf(at, sizeof(at), "; first apart at frame %zu: lid %.4f/%.4f, back %.4f/%.4f, fist %d/%d, fingers %d/%d, flags %s", i, a.lid, b.lid, a.leverBack, b.leverBack, a.fist, b.fist,
                                  a.leverFingers != nullptr, b.leverFingers != nullptr, bools.c_str());
                    firstApart = at;
                }
            }
            std::snprintf(what, sizeof(what), "h2 the %s, %s: the right's the left's mirror image over %zu frames: %d events, all at the same frames (%d apart), each its own hand's (%d not), the gloves %.4f and the magazines %.4f apart at most%s",
                          name, events, left.size(), happened, eventsApart, sides, gloveOff, magOff, firstApart.c_str());
            Check(happened > 0 && eventsApart == 0 && sides == 0 && gloveOff < 2e-4f && magOff < 2e-4f, what);
        };
        // The AN-94: the seated magazine taken and pulled down (rocked off its hook), let go of; a fresh one from the belly brought under the well: home.
        {
            std::vector<std::pair<M, float>> s;
            float hold[12];
            handmag::HoldFor(kAn94, hold, 0);
            for (int i = 0; i < 4; ++i)
                s.push_back({GloveHolding(kGunPose, hold, Id()), 0.0f});
            for (int i = 0; i < 40; ++i)
                s.push_back({GloveHolding(kGunPose, hold, Moved(Id(), std::array<float, 3>{0.0f, -0.0025f * i, 0.003f * (i > 20 ? i - 20 : 0)}.data())), 1.0f});
            for (int i = 0; i < 4; ++i)
                s.push_back({s.back().first, 0.0f});
            const size_t belly = s.size() + 2;
            for (int i = 0; i < 4; ++i)
                s.push_back({GloveHolding(kGunPose, hold, Moved(Id(), std::array<float, 3>{0.0f, -0.25f, 0.1f}.data())), i >= 2 ? 1.0f : 0.0f});
            for (int i = 0; i < 40; ++i) {
                const float u = std::min(i / 25.0f, 1.0f);
                s.push_back({GloveHolding(kGunPose, hold, Moved(Id(), std::array<float, 3>{0.0f, -0.25f + 0.21f * u, 0.1f - 0.1f * u}.data())), 1.0f});
            }
            for (int i = 0; i < 6; ++i)
                s.push_back({s.back().first, 0.0f});
            compare(kAn94, "AN-94", s, false, 0, true, static_cast<int>(belly), "its magazine pulled out, let go of, a fresh one taken at the belly and home by itself");
        }
        // The AN-94: the Afghan reload, a fresh one swept forward fast into the old one: knocked out, the fresh one home.
        {
            std::vector<std::pair<M, float>> s;
            float hold[12];
            handmag::HoldFor(kAn94, hold, 0);
            const auto at = [&](float y, float z, float tilt) {
                const float c = std::cos(tilt), sn = std::sin(tilt);
                M m{1, 0, 0, 0, 0, c, sn, 0, 0, -sn, c, 0};
                const std::array<float, 3> top = Apply(m, kAn94.top);
                m[3] = 0.0f, m[7] = kAn94.top[1] + y - top[1], m[11] = kAn94.top[2] + z - top[2];
                return GloveHolding(kGunPose, hold, m);
            };
            for (int i = 0; i < 4; ++i)
                s.push_back({at(-0.25f, 0.0f, 0.0f), i >= 2 ? 1.0f : 0.0f});
            for (int i = 0; i < 30; ++i)
                s.push_back({at(-0.25f + 0.18f * i / 29.0f, -0.16f, -0.1f), 1.0f});
            for (int i = 0; i < 10; ++i)
                s.push_back({at(-0.07f + 0.04f * i / 9.0f, -0.16f + 0.18f * i / 9.0f, -0.1f - 0.2f * i / 9.0f), 1.0f});
            for (int i = 0; i < 40; ++i)
                s.push_back({at(-0.03f, 0.02f, -0.3f), 1.0f});
            compare(kAn94, "AN-94", s, false, 0, true, 2, "the Afghan reload: a fresh one knocks the old one out and goes home");
        }
        // The M4A1's and the Mk11's T handle (on the gun's middle): taken by the grip, drawn, let go.
        for (const handmag::Gun* gun : {&kM4, &kMk11}) {
            std::vector<std::pair<M, float>> s;
            const M turn = LeverTurn(0, false);
            for (int i = 0; i < 40; ++i) {
                const float back = std::min(std::max(i - 4, 0) * 0.004f, gun->leverTravel + 0.005f);
                const float at[3] = {gun->lever[0], gun->lever[1] + 0.005f, gun->lever[2] - back};
                s.push_back({GloveOn(kGunPose, turn, kFistIn[0], at), i >= 2 && i < 36 ? 1.0f : 0.0f});
            }
            compare(*gun, gun == &kM4 ? "M4A1" : "Mk11", s, false, 0, false, 0, "its T handle taken by the grip, drawn back and let go");
        }
        // The M240's and the M249's lid (the receiver's own, across it): taken at its back end, lifted till it goes on by itself.
        for (const handmag::Gun* gun : {&kM240, &m249rows::kMag}) {
            const handmag::BeltGun& b = *gun->belt;
            std::vector<std::pair<M, float>> s;
            const M lidTurn = Inv(Load(b.lidHold));
            for (int i = 0; i < 40; ++i) {
                float at[3];
                const float angle = LidAngleAt(b, std::min(std::max(i - 4, 0) * 0.03f, 0.8f), 0, at);
                const float c = std::cos(angle), sn = std::sin(angle), hy = b.lidHinge[0], hz = b.lidHinge[1];
                const M up{1, 0, 0, 0, 0, c, -sn, hy - (c * hy - sn * hz), 0, sn, c, hz - (sn * hy + c * hz)};
                M local = Mul(up, lidTurn);
                const std::array<float, 3> mid = Apply(local, kHandIn[0]);
                local[3] += at[0] - mid[0], local[7] += at[1] - mid[1], local[11] += at[2] - mid[2];
                s.push_back({Mul(kGunPose, local), i >= 2 && i < 38 ? 1.0f : 0.0f});
            }
            compare(*gun, gun == &kM240 ? "M240" : "M249", s, false, 0, false, 0, "its lid taken at its back end and lifted: up by itself");
        }
    }

    // ---- h3: the M240's and the M249's whole belt reload by the right alone (the left holding the gun by its front), then the handle ----
    for (const handmag::Gun* gun : {&kM240, &m249rows::kMag}) {
        const Rig& rig = RigOf(gun);
        Sim sim(rig);
        handmag::Clear();
        handmag::BeltReset();
        sim.squeeze[1] = 1.0f;
        sim.Step(5);
        TakeFront(sim, 0);
        LetGoOfGrip(sim, 1);
        const bool byFront = sim.gr.holder == grasp::Holder::kBarrel && sim.gr.barrel == 0 && sim.work == 1;
        const BeltRun run = BeltReload(sim, 1);
        LeverBy(sim, 1, false, true);
        const bool chambered = sim.Saw("a round chambered");
        std::snprintf(what, sizeof(what), "h3 the %s held by the left's front alone %d: the right lifts the lid %d, takes the box out %d, a fresh one at the belly %d, home %d, the belt laid %d, the lid shut %d (ready %d), the handle %d, a round chambered %d; the left on its front all along %d, no regrip %d [%s]",
                      rig.name, byFront, run.lidUp, run.boxOut, run.fresh, run.home, run.laid, run.lidShut, run.ready, sim.Saw("lever let go (right)"), chambered,
                      sim.gr.holder == grasp::Holder::kBarrel && sim.gr.barrel == 0, sim.regrips == 0, Words(sim.events).c_str());
        Check(byFront && run.lidUp && run.boxOut && run.fresh && run.home && run.laid && run.lidShut && run.ready && chambered && sim.regrips == 0 &&
                  sim.gr.holder == grasp::Holder::kBarrel,
              what);
    }

    // ---- h4: the hand changed with something in the last one: let go of as the gun's leaving the hands would, that hand's events, nothing taken by the new one ----
    {
        // The right holds a fresh AN-94 magazine; the module is fed the left (the gun back in the right hand as at a new life), the left's grip held.
        Settle(kAn94, 1);
        handmag::Frame f = Frame(kAn94, 1);
        float hold[12];
        handmag::HoldFor(kAn94, hold, 1);
        SetGlove(f, GloveHolding(kGunPose, hold, Moved(Id(), std::array<float, 3>{0.0f, -0.3f, 0.0f}.data())));
        handmag::Update(f);
        f.squeeze = 1.0f;
        f.shoulder = true;
        const bool took = handmag::Update(f).took;
        f.shoulder = false;
        for (int i = 0; i < 6; ++i)
            handmag::Update(f);
        const int busy = handmag::BusySide();
        handmag::Frame g = Frame(kAn94, 0);
        float leftHold[12];
        handmag::HoldFor(kAn94, leftHold, 0);
        SetGlove(g, GloveHolding(kGunPose, leftHold, Id()));  // (the left right on the seated magazine, its grip held: no take)
        g.squeeze = 1.0f;
        const handmag::Result r = handmag::Update(g);
        const bool let = r.letGo && r.side == 1 && !r.grabbed && !r.took && handmag::BusySide() == -1;
        bool none = true;
        for (int i = 0; i < 20; ++i) {
            const handmag::Result q = handmag::Update(g);
            none = none && !q.grabbed && !q.took && !q.leverTaken && q.side == 0 && q.gloveBlend == 0.0f;
        }
        g.squeeze = 0.0f;
        handmag::Update(g);
        g.squeeze = 1.0f;
        const bool fresh = handmag::Update(g).grabbed;
        std::snprintf(what, sizeof(what), "h4 a fresh magazine in the right %d (busy %d), the left fed with its grip held on the seated one: the right's let go of %d (its event, the right's), the left takes nothing while its grip stays held %d, a fresh squeeze of it takes the seated one %d",
                      took, busy, let, none, fresh);
        Check(took && busy == 1 && let && none && fresh, what);
    }
    {
        // The right on the M240's lever when the left is fed: it springs home, the right's event; then the left's own squeeze takes it.
        Settle(kM240, 1);
        handmag::Frame f = Frame(kM240, 1);
        const float knob[3] = {kM240.lever[0], kM240.lever[1] + 0.005f, kM240.lever[2]};
        SetGlove(f, GloveOn(kGunPose, LeverTurn(1, false), kFistIn[1], knob));
        handmag::Update(f);
        f.trigger = 1.0f;
        const bool taken = handmag::Update(f).leverTaken;
        const float back[3] = {knob[0], knob[1], knob[2] - 0.05f};
        SetGlove(f, GloveOn(kGunPose, LeverTurn(1, false), kFistIn[1], back));
        for (int i = 0; i < 6; ++i)
            handmag::Update(f);
        handmag::Frame g = Frame(kM240, 0);
        SetGlove(g, GloveOn(kGunPose, LeverTurn(0, false), kFistIn[0], knob));
        g.trigger = 1.0f;
        const handmag::Result r = handmag::Update(g);
        const bool let = r.leverLet && r.side == 1 && !r.leverTaken;
        g.trigger = 0.0f;
        handmag::Update(g);
        g.trigger = 1.0f;
        const handmag::Result q = handmag::Update(g);
        std::snprintf(what, sizeof(what), "h4 the right on the M240's lever %d, the left fed: let go of, springs home, the right's event %d; the left's held trigger takes nothing then %d, its fresh pull takes it %d (the left's %d)",
                      taken, let, !r.leverTaken, q.leverTaken, q.side == 0);
        Check(taken && let && !r.leverTaken && q.leverTaken && q.side == 0, what);
    }

    // ---- h5: Claims and Update agree for the right, as for the left (the regrip's guard is the take itself) ----
    {
        std::mt19937 rng(1119);
        std::uniform_real_distribution<float> u(-1.0f, 1.0f);
        for (const Rig& rig : kRigs) {
            const handmag::Gun& gun = *rig.gun;
            int tried = 0, took = 0, apart = 0;
            for (int n = 0; n < 240; ++n) {
                Settle(gun, 1);
                // Round its parts: the lever's knob, the seated magazine's middle, the right's place on the lid's back end; turned any way.
                const int part = n % 3;
                float at[3] = {gun.lever[0], gun.lever[1], gun.lever[2]};
                if (part == 1)
                    std::memcpy(at, gun.grab, sizeof(at));
                if (part == 2 && gun.belt)
                    at[0] = -gun.belt->lidGrab[0], at[1] = gun.belt->lidGrab[1], at[2] = gun.belt->lidGrab[2];
                for (float& v : at)
                    v += 0.07f * u(rng);
                const float fingers[3] = {u(rng), u(rng), u(rng)}, palm[3] = {u(rng), u(rng), u(rng)};
                const M glove = GloveOn(kGunPose, Turn(1, fingers, palm), kHandIn[1], at);
                handmag::Frame f = Frame(gun, 1);
                SetGlove(f, glove);
                handmag::Update(f);
                const std::array<float, 3> anchor = Apply(glove, kAnchorIn[1]);
                const bool claims = handmag::Claims(gun, kGunPose.data(), glove.data(), anchor.data(), 1);
                f.squeeze = 1.0f;
                const handmag::Result r = handmag::Update(f);
                const bool takes = r.leverTaken || r.lidTaken || r.beltTaken || r.grabbed || r.knocked;
                ++tried, took += takes, apart += claims != takes;
            }
            std::snprintf(what, sizeof(what), "h5 the %s, the right's squeezes round its handle, magazine and lid: %d, %d taking something; Claims and Update apart on %d", rig.name, tried, took, apart);
            Check(apart == 0 && took > 0, what);
        }
        // The module at work with one hand: the other's squeeze isn't its (one hand at a time).
        Settle(kAn94, 1);
        handmag::Frame f = Frame(kAn94, 1);
        const float knob[3] = {kAn94.lever[0], kAn94.lever[1] + 0.005f, kAn94.lever[2]};
        const M right = GloveOn(kGunPose, LeverTurn(1, false), kFistIn[1], knob);
        SetGlove(f, right);
        handmag::Update(f);
        f.trigger = 1.0f;
        handmag::Update(f);
        const M left = GloveOn(kGunPose, LeverTurn(0, false), kFistIn[0], knob);
        const std::array<float, 3> leftPalm = Apply(left, kAnchorIn[0]), rightPalm = Apply(right, kAnchorIn[1]);
        const bool mine = handmag::Claims(kAn94, kGunPose.data(), right.data(), rightPalm.data(), 1);
        const bool theirs = handmag::Claims(kAn94, kGunPose.data(), left.data(), leftPalm.data(), 0);
        Check(mine && !theirs, "h5 the right on the AN-94's lever: the right's squeeze is the module's, the left's at the same knob isn't (one hand at a time)");
    }

    // ---- h6: the owner's case end to end, the M249 and the M240; and mirrored for a lefty ----
    for (const handmag::Gun* gun : {&m249rows::kMag, &kM240}) {
        const Rig& rig = RigOf(gun);
        {
            Sim sim(rig);
            handmag::Clear();
            handmag::BeltReset();
            sim.squeeze[1] = 1.0f;  // the right on the pistol grip, squeezed (armed)
            sim.Step(5);
            const bool usual = sim.gr.holder == grasp::Holder::kHeld && sim.gr.trigger == 1 && sim.work == 0 && sim.fed == 0;
            // Run dry: the left (the hand at work) lays a fresh box and its belt, shuts the lid.
            const BeltRun belt = BeltReload(sim, 0);
            const bool leftDid = belt.lidUp && belt.boxOut && belt.home && belt.laid && belt.lidShut && belt.ready;
            // The left takes the front; the right lets go: held by the left's front alone. Nothing fires.
            TakeFront(sim, 0);
            const M gunBefore = sim.gunRoom;
            LetGoOfGrip(sim, 1);
            const bool byFront = sim.gr.holder == grasp::Holder::kBarrel && sim.gr.barrel == 0 && sim.work == 1;
            Fire(sim, 1);
            Fire(sim, 0);
            const bool noFire = !sim.firedOnce[0] && !sim.firedOnce[1] && sim.gr.fireHand == -1;
            // The right pulls the handle over the top by its trigger: claimed, no regrip; the gun where the left has it.
            const int regripsBefore = sim.regrips, jumpsBefore = sim.gunJumps;
            const M gunHeld = sim.gunRoom;
            LeverBy(sim, 1, false, true);
            const bool handle = sim.Saw("lever taken (right)") && sim.Saw("lever let go (right)") && sim.Saw("a round chambered");
            const bool still = MaxDiff(gunHeld, sim.gunRoom.data()) < 1e-5f && sim.gunJumps == jumpsBefore && sim.regrips == regripsBefore;
            const bool stillFront = sim.gr.holder == grasp::Holder::kBarrel && sim.onFront[0];
            // The right takes its pistol grip back and fires.
            TakeGrip(sim, 1);
            const bool regrip = sim.gr.holder == grasp::Holder::kHeld && sim.gr.trigger == 1 && sim.regrips == regripsBefore + 1;
            Fire(sim, 1);
            std::snprintf(what, sizeof(what), "h6 the owner's %s: in the right hand as usual %d; the left lays the belt and shuts the lid %d; held by the left's front alone %d (it moved %.4f), the triggers fire nothing then %d; the right pulls the handle %d, the gun still on the left %d, the left on its front %d; the right takes its grip back %d and fires %d [%s]",
                          rig.name, usual, leftDid, byFront, MaxDiff(gunBefore, sim.gunRoom.data()), noFire, handle, still, stillFront, regrip, sim.firedOnce[1], Words(sim.events).c_str());
            Check(usual && leftDid && byFront && noFire && handle && still && stillFront && regrip && sim.firedOnce[1] && !sim.firedOnce[0], what);
        }
        {
            // A lefty: the left on the pistol grip; the right (at work) lays the belt and shuts the lid, takes the front; the left lets go (held by the
            // right's front alone) and pulls the handle, takes its grip back and fires.
            Sim sim(rig);
            handmag::Clear();
            handmag::BeltReset();
            sim.gs = grasp::State{};
            sim.gs.trigger = 0;
            sim.gs.fireReset[0] = true, sim.gs.fireReset[1] = false;
            sim.gr.holder = grasp::Holder::kHeld, sim.gr.trigger = 0;
            sim.hand[0] = sim.PistolGripControllerAt(0);
            sim.hand[1] = Moved(Id(), std::array<float, 3>{0.4f, 0.9f, -0.2f}.data());
            sim.squeeze[0] = 1.0f;
            sim.Step(5);
            const bool lefty = sim.gr.holder == grasp::Holder::kHeld && sim.gr.trigger == 0 && sim.work == 1 && sim.fed == 1;
            const BeltRun belt = BeltReload(sim, 1);
            const bool rightDid = belt.lidUp && belt.boxOut && belt.home && belt.laid && belt.lidShut && belt.ready;
            TakeFront(sim, 1);
            LetGoOfGrip(sim, 0);
            const bool byFront = sim.gr.holder == grasp::Holder::kBarrel && sim.gr.barrel == 1 && sim.work == 0;
            const int regripsBefore = sim.regrips;
            const M gunHeld = sim.gunRoom;
            LeverBy(sim, 0, true, false);
            const bool handle = sim.Saw("lever taken (left)") && sim.Saw("lever let go (left)") && sim.Saw("a round chambered");
            const bool still = MaxDiff(gunHeld, sim.gunRoom.data()) < 1e-5f && sim.regrips == regripsBefore;
            TakeGrip(sim, 0);
            const bool regrip = sim.gr.holder == grasp::Holder::kHeld && sim.gr.trigger == 0;
            Fire(sim, 0);
            Fire(sim, 1);
            std::snprintf(what, sizeof(what), "h6 a lefty's %s: in the left hand, the right at work %d; the right lays the belt and shuts the lid %d; held by the right's front alone %d; the left pulls the handle from below by its grip %d, the gun still on the right %d; the left takes its grip back %d and fires %d, the right's trigger nothing %d [%s]",
                          rig.name, lefty, rightDid, byFront, handle, still, regrip, sim.firedOnce[0], !sim.firedOnce[1], Words(sim.events).c_str());
            Check(lefty && rightDid && byFront && handle && still && regrip && sim.firedOnce[0] && !sim.firedOnce[1], what);
        }
    }

    // ---- h7: every held gun's lever with the gun held by its front alone, either hand; the trigger only for the hand on the pistol grip after ----
    for (const Rig& rig : kRigs) {
        for (int holder = 0; holder < 2; ++holder) {
            const int worker = 1 - holder;
            Sim sim(rig);
            handmag::Clear();
            handmag::BeltReset();
            // The worker on the pistol grip (as a pick-up leaves it), the holder takes the front, the worker lets go.
            sim.gs = grasp::State{};
            sim.gs.trigger = worker;
            sim.gs.fireReset[worker] = true, sim.gs.fireReset[holder] = false;
            sim.gr.holder = grasp::Holder::kHeld, sim.gr.trigger = worker;
            sim.hand[worker] = sim.PistolGripControllerAt(worker);
            sim.squeeze[worker] = 1.0f;
            sim.Step(5);
            TakeFront(sim, holder);
            LetGoOfGrip(sim, worker);
            const bool byFront = sim.gr.holder == grasp::Holder::kBarrel && sim.gr.barrel == holder && sim.work == worker;
            Fire(sim, worker);
            const bool noFire = !sim.firedOnce[worker] && !sim.firedOnce[holder];
            const M gunHeld = sim.gunRoom;
            const bool under = worker == 1;  // (the right from below by its grip, the left over the top by its trigger)
            LeverBy(sim, worker, under, !under);
            const std::string by = worker == 1 ? " (right)" : " (left)";
            const bool handle = sim.Saw(("lever taken" + by).c_str()) && sim.Saw(("lever let go" + by).c_str());
            const bool chambered = sim.Saw("a round chambered") || sim.Saw("knocked out of its notch: a round chambered");
            const bool still = MaxDiff(gunHeld, sim.gunRoom.data()) < 1e-5f && sim.regrips == 0 && sim.gr.holder == grasp::Holder::kBarrel;
            // (A handle resting in its notch: the hand knocks it down first, a squeeze by it.)
            if (rig.gun->leverNotch > 0.0f && sim.notchBlocked) {
                const M turn = LeverTurn(worker, false);
                const float c = std::cos(rig.gun->leverNotch), s = std::sin(rig.gun->leverNotch);
                const float dx = rig.gun->lever[0] - rig.gun->leverNotchAxis[0], dy = rig.gun->lever[1] - rig.gun->leverNotchAxis[1];
                const float knob[3] = {rig.gun->leverNotchAxis[0] + c * dx - s * dy, rig.gun->leverNotchAxis[1] + s * dx + c * dy, rig.gun->lever[2] - rig.gun->leverNotchBack};
                sim.Glide(worker, GloveAtGun(sim, turn, kHandIn[worker], knob), 0.4f, 0.0f, 0.0f);
                sim.Glide(worker, sim.GloveOf(worker), 0.1f, 1.0f, 0.0f);
                sim.Glide(worker, sim.GloveOf(worker), 0.1f, 0.0f, 0.0f);
            }
            TakeGrip(sim, worker);
            Fire(sim, worker);
            std::snprintf(what, sizeof(what), "h7 the %s held by the %s's front alone %d, nothing fires then %d: the %s's lever %s %d, a round chambered %d, the gun still and no regrip %d; the %s takes its grip back and fires %d, the %s's trigger never %d [%s]",
                          rig.name, holder ? "right" : "left", byFront, noFire, worker ? "right" : "left", under ? "from below by the grip" : "over the top by the trigger", handle, chambered, still,
                          worker ? "right" : "left", sim.firedOnce[worker], holder ? "right" : "left", !sim.firedOnce[holder], Words(sim.events).c_str());
            Check(byFront && noFire && handle && chambered && still && sim.firedOnce[worker] && !sim.firedOnce[holder], what);
        }
    }
    {
        // The UMP-45's handle sits over its handguard on its left (the HK way). Its front taken with the game's own left hold there, the hand middle's nearer
        // its front than its handle by the left (as before: 8.8 cm against 9.4) and nearer the handle by the right (its fingers up the gun's left side,
        // 7.1 against 8.8): the nearer is taken, one rule for both hands, so a lefty's right squeezing there has the handle; 3 cm under, the front.
        int took[2][2] = {};
        for (int side = 0; side < 2; ++side)
            for (int low = 0; low < 2; ++low) {
                const Rig& rig = RigOf(&kUmp45);
                Sim sim(rig);
                handmag::Clear();
                sim.gs = grasp::State{};
                sim.gs.trigger = 1 - side;
                sim.gs.fireReset[1 - side] = true, sim.gs.fireReset[side] = false;
                sim.gr.holder = grasp::Holder::kHeld, sim.gr.trigger = 1 - side;
                sim.hand[1 - side] = sim.PistolGripControllerAt(1 - side);
                sim.squeeze[1 - side] = 1.0f;
                sim.Step(5);
                TakeFront(sim, side, low ? 0.03f : 0.0f);
                took[side][low] = sim.onFront[side] ? 1 : sim.Saw(side ? "lever taken (right)" : "lever taken (left)") ? 2 : 0;
            }
        std::snprintf(what, sizeof(what), "h7 the UMP-45's front at the game's own hold: the left takes %s, the right %s; 3 cm under it: the left %s, the right %s",
                      took[0][0] == 1 ? "the front" : took[0][0] == 2 ? "the handle" : "nothing", took[1][0] == 1 ? "the front" : took[1][0] == 2 ? "the handle" : "nothing",
                      took[0][1] == 1 ? "the front" : "not the front", took[1][1] == 1 ? "the front" : "not the front");
        Check(took[0][0] == 1 && took[1][0] == 2 && took[0][1] == 1 && took[1][1] == 1, what);
    }
    {
        // The KH2002's magazine, 9.6 cm from the right's pistol grip point: the right's squeeze on it with the left holding the front is the module's (claimed),
        // never a regrip; drawn out by the right.
        const Rig& rig = RigOf(&kKh2002);
        Sim sim(rig);
        handmag::Clear();
        sim.squeeze[1] = 1.0f;
        sim.Step(5);
        TakeFront(sim, 0);
        LetGoOfGrip(sim, 1);
        float hold[12];
        handmag::HoldFor(kKh2002, hold, 1);
        const M onIt = GloveHolding(sim.gunRoom, hold, Id());
        const std::array<float, 3> gripAt = Apply(Mul(onIt, InvAny(LineUp(1))), std::array<float, 3>{0, 0, 0}.data());
        const std::array<float, 3> gripOnGun = Apply(Inv(sim.gunRoom), gripAt.data());
        const std::array<float, 3> pistol = sim.GripPoint(1);
        const float fromGrip = Dist(gripOnGun.data(), pistol.data());
        sim.Glide(1, onIt, 0.4f, 0.0f, 0.0f);
        sim.Glide(1, onIt, 0.1f, 1.0f, 0.0f);
        const bool claimed = sim.claimed[1] || sim.busyWas[1];
        for (int i = 1; i <= 30 && !sim.magOut; ++i)
            sim.Glide(1, GloveHolding(sim.gunRoom, hold, Moved(Id(), std::array<float, 3>{0.0f, -0.004f * i, 0.0f}.data())), kDt, 1.0f, 0.0f);
        std::snprintf(what, sizeof(what), "h7 the KH2002 held by the left's front alone: the right's squeeze on its magazine (its grip %.1f cm from its pistol grip's point, within the regrip's 15) claimed %d, no regrip %d, the magazine out by the right %d",
                      fromGrip * 100.0f, claimed, sim.regrips == 0, sim.magOut);
        Check(fromGrip < 0.15f && claimed && sim.regrips == 0 && sim.magOut, what);
    }
    {
        // The M1014's bolt handle racked by the right while the left holds its front alone: shellload.cpp's hand holding the gun from the grasp (its front's
        // hand), the right at work.
        shellload::Clear();
        grasp::State gs;
        grasp::Frame gf;
        gf.dt = kDt;
        gf.enabled = true;
        gs.holder = grasp::Holder::kBarrel, gs.trigger = -1, gs.barrel = 0;
        const int gunHand = gs.barrel;  // (hands.cpp's g_tubeGunHand: the hand holding it, by its front alone here)
        const int work = workhand::WorkHand(true, gs.holder, gs.trigger, gs.barrel);
        const shellload::Gun& g = shellload::kM1014;
        const M turn = LeverTurn(1, false);
        auto frame = [&](float back, float squeeze) {
            shellload::Frame f;
            f.dt = kDt;
            f.active = true;
            f.gun = &g;
            f.gunTracked = true;
            std::memcpy(f.gunPose, kGunPose.data(), sizeof(f.gunPose));
            const M head = Id();
            std::memcpy(f.headPose, head.data(), sizeof(f.headPose));
            f.gunHand = gunHand;
            f.hand[0].tracked = true;
            f.hand[0].busy = true;  // (on the front)
            const float knob[3] = {g.handle[0], g.handle[1], g.handle[2] - back};
            f.hand[1].tracked = true;
            // (shellload measures the right's hand's middle with its own mirrored point: that point on the knob.)
            const float rightMiddle[3] = {-shellload::kHandInGlove[0], shellload::kHandInGlove[1], shellload::kHandInGlove[2]};
            const M placed = GloveOn(kGunPose, turn, rightMiddle, knob);
            std::memcpy(f.hand[1].glove, placed.data(), sizeof(f.hand[1].glove));
            f.hand[1].squeeze = squeeze;
            f.canLoad = true;
            return shellload::Update(f);
        };
        frame(0.0f, 0.0f);
        bool taken = false, racked = false;
        int side = -1;
        for (int i = 0; i < 50; ++i) {
            const float back = std::min(std::max(i - 3, 0) * 0.004f, g.travel);
            const shellload::Result r = frame(back, i < 45 ? 1.0f : 0.0f);
            taken = taken || r.handleTaken;
            racked = racked || r.racked;
            if (r.handleTaken)
                side = r.side;
        }
        std::snprintf(what, sizeof(what), "h7 the M1014 held by the left's front alone: the hand at work the right %d; its bolt handle taken by the right %d (the %s hand's), racked %d",
                      work == 1, taken, side == 1 ? "right" : side == 0 ? "left" : "no", racked);
        Check(work == 1 && taken && side == 1 && racked, what);
    }

    // ---- h8: workhand.h's rules ----
    {
        using grasp::Holder;
        const bool work = workhand::WorkHand(false, Holder::kBarrel, -1, 0) == 0 && workhand::WorkHand(true, Holder::kHeld, 1, -1) == 0 &&
                          workhand::WorkHand(true, Holder::kHeld, 0, -1) == 1 && workhand::WorkHand(true, Holder::kGlide, 1, 0) == 0 &&
                          workhand::WorkHand(true, Holder::kBarrel, -1, 0) == 1 && workhand::WorkHand(true, Holder::kBarrel, -1, 1) == 0 &&
                          workhand::WorkHand(true, Holder::kLoose, -1, -1) == -1 && workhand::WorkHand(true, Holder::kStowed, -1, -1) == -1;
        Check(work, "h8 the hand at work: dormant the left; the hand not on the pistol grip; the hand not holding the front alone; nobody while it lies or is put away");
        const bool fed = workhand::FedSide(-1, 1, true, false) == 1 && workhand::FedSide(-1, 1, false, false) == 0 && workhand::FedSide(1, 0, true, false) == 1 &&
                         workhand::FedSide(0, 1, true, false) == 0 && workhand::FedSide(-1, -1, true, false) == 0 && workhand::FedSide(1, 1, true, true) == 0;
        Check(fed, "h8 the side fed: the hand at work (the right only with its own pose), the hand with something in it until it's done, the left while nobody works it and for heldtest");
        const bool gone = workhand::NoLongerAtWork(1, 0, true, false) && !workhand::NoLongerAtWork(1, 1, true, false) && !workhand::NoLongerAtWork(1, 0, false, false) &&
                          !workhand::NoLongerAtWork(1, -1, true, true) && !workhand::NoLongerAtWork(-1, 0, true, false) && workhand::NoLongerAtWork(0, 1, true, false);
        Check(gone, "h8 a hand no longer at work lets go: only while free hands are awake, never on the frame the gun's let go of, never with nothing in it");
        const bool chamber = workhand::LeverChambers(true, true, false, true, true, 2) && !workhand::LeverChambers(false, true, false, true, true, 2) &&
                             !workhand::LeverChambers(true, false, false, true, true, 2) && !workhand::LeverChambers(true, true, true, true, true, 2) &&
                             !workhand::LeverChambers(true, true, false, false, true, 2) && !workhand::LeverChambers(true, true, false, true, false, 2) &&
                             !workhand::LeverChambers(true, true, false, true, true, 1);
        Check(chamber, "h8 a round chambered by the lever only drawn far enough, the chamber empty, a magazine in, the belt laid and the lid shut, a round more than the one kept");
        const bool fire = workhand::HeldMayFire(false, false, false) && !workhand::HeldMayFire(true, false, false) && !workhand::HeldMayFire(false, true, false) &&
                          !workhand::HeldMayFire(false, false, true);
        Check(fire, "h8 a held gun fires with a round chambered, its belt laid and lid shut, no handle in its notch (hands.cpp's HeldChamberEmpty)");
        // The mirror of a hold across a part's own middle: twice is the hold again; across the gun's middle it's pouch.cpp's elements negated.
        float once[12], twice[12];
        handmag::MirrorHold(magposes::kM240Hold, 0.095f, once);
        handmag::MirrorHold(once, 0.095f, twice);
        float s[12];
        handmag::MirrorHold(magposes::kAn94Hold, 0.0f, s);
        bool pouchWay = true;
        for (int i = 0; i < 12; ++i)
            pouchWay = pouchWay && s[i] == ((i == 1 || i == 2 || i == 3 || i == 4 || i == 8) ? -magposes::kAn94Hold[i] : magposes::kAn94Hold[i]);
        float det = once[0] * (once[5] * once[10] - once[6] * once[9]) - once[1] * (once[4] * once[10] - once[6] * once[8]) + once[2] * (once[4] * once[9] - once[5] * once[8]);
        std::snprintf(what, sizeof(what), "h8 a hold mirrored about a part's middle: twice is itself (%.6f), a turn still (det %.4f); about the gun's middle the pouch's mirror %d", MaxDiff(twice, magposes::kM240Hold), det, pouchWay);
        Check(MaxDiff(twice, magposes::kM240Hold) < 1e-5f && std::fabs(det - 1.0f) < 0.01f && pouchWay, what);
    }

    std::printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed;
}

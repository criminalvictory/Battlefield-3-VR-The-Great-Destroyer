// The fingers on a lever or a charging handle, every gun with one, over the top and from under it, either hand, without the game: weapon.txt "heldtest 3"
// and "heldtest 4"'s scripted hand (src/heldtest.h, its path and its wrist, as hands.cpp has them) through src/handmag.cpp as hands.cpp feeds it, the
// glove drawn as hands.cpp draws the left (eased onto the fitted hand about its palm point) and as RightGlove draws the right (the left's mirror image,
// the left's tables flipped onto it: handpose::LeftToRight), the fingers posed by src/handpose.cpp and eased onto the gun's own fitted fingers about each
// bone's joint (hands.cpp's BlendAbout, copied; the table and its share by handmag::EaseFingers, as hands.cpp has them), or closed in a fist on a lever
// with no table for that hand (hands.cpp's g_heldFist, copied). At 72 Hz, the whole pull, the gun held still and turned as the films' stand-in held it.
// (What's copied here, BlendAbout, the fist and skin.cpp's rows of the guns, is a change to both when it changes.)
// The proof of 2026-11-25 (Operation Swordbreaker's Mk11, heldtest 4, build\survey\p114b_mk11_under_zoom.png frames 7 and 8): the left glove's fingers
// drawn as long striped rods: heldtest 4's wrist was a shear (its palm 65 degrees from its fingers), every finger bone drawn on it 0.89 to 1.19 of its
// length. Each check fails with that wrist (heldtest.h's LeverWrist as it was) and passes with the wrist a turn.
// The right's runs: hands.cpp never scripts the right hand (workhand.h's FedSide: heldtest's hand is the left's), and a controller's glove is a turn
// whatever way it's held, so no right hand was ever drawn on heldtest's wrist. Here the right's wrist is the left's mirror image (it fails with the old one
// as the left's does), and the right's f1 and f2 check what a real right hand has on a lever: its own tables (the left's mirrored onto a handle on the
// middle), its fist on a lever off the middle, the glove's ease onto its fitted hand; its f5 its let-go, as hands.cpp has it.
//   f1  every frame, either hand, over and under, every gun: the glove as drawn is a turn (each of its axes 1 to 0.1%)
//   f2  ... every finger's three bones as drawn (the glove times the bone): each its own length, joint to joint (to 2%), and none stretched or squeezed
//       past what the glove is made with (each axis 0.86 to 1.02: the fingers are slimmed to 0.88 across on the left glove; the right's own 1)
//   f3  ... the pull itself happens: the lever taken by the trigger as the hand gets to it, drawn back as far as the hand goes (10 cm, or all its
//       travel), let go of, nothing else of the gun taken
//   f4  the wrist (LeverWrist): a turn over, under and from a handle's own side; under, the palm up and square to the fingers, over it down
//   f5  ... the fingers ease out from the table they were on: every frame, each fingertip (in the glove: the fingers alone) within 3 mm of where the
//       table of the frame before puts it at this frame's share. The review of 84654d7: at a lever's let-go the fingers went over to the gun's magazine
//       table at once and eased out from there (hands.cpp 9037-9040 and 9050-9053 as they were): a tip moved up to 12 cm in that one frame (the G3A3's
//       middle; 6 eased from the lever's own table), 4 to 10 cm of it the other table's; the left on every gun, the right on a handle on the middle
//       (its fist on a side lever has no table), real hands too. The eases themselves, 35% of the way a frame at 72 Hz, move a tip up to 7.5 cm in a
//       frame (the right's fist closing on a side lever): the most a tip moves in a frame is said, not held to a length.
//   f6  handmag::EaseFingers on its own: the table kept as the hand lets go of the lever, the belt's pinch, a magazine and the Stinger's aerial; at once
//       onto a magazine taken; the other hand's magazine never this hand's table
// Usage: leverfingerstest [old | oldletgo] (prints PASS or FAIL per check; the exit code is the failures). old: the wrist as it was (f1 and f2 from
// under fail); oldletgo: the let-go as it was, the magazine's table whenever the gun has one fitted (f5 and f6 fail).

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "../src/handmag.h"
#include "../src/handpose.h"
#include "../src/heldtest.h"
#include "../src/m249rows.h"
#include "../src/magposes.h"

namespace {

int g_checks = 0, g_failed = 0;
void Check(bool ok, const std::string& what) {
    ++g_checks;
    g_failed += !ok;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", what.c_str());
}

constexpr float kPi = 3.14159265f;
constexpr float kDt = 1.0f / 72.0f;

// ---- skin.cpp's rows (copied, as heldmagtest's and otherhandtest's are; the M249's its own header's) ----
constexpr handmag::Gun kAn94{{0.0f, 0.033f, 0.386f}, {0.0f, -0.075f, 0.410f}, {0.0f, -0.012f, 0.340f}, {0.0f, 0.005f, 0.424f}, -0.05f, {-0.034f, 0.068f, 0.495f}, 0.113f, 25.0f * kPi / 180.0f, {0.0f, -0.008f, 0.010f}, magposes::kAn94Hold, magposes::kAn94Fingers, false, false, {{0.023f, 0.348f}, {-0.065f, 0.365f}, {-0.152f, 0.404f}}, magposes::kAn94LeverHold, magposes::kAn94LeverFingers, false, {}, 0.0f, {}, 0.0f, nullptr, magposes::kAn94LeverUnderHold, magposes::kAn94LeverUnderFingers};
constexpr handmag::Gun kRpk{{0.0f, 0.013f, 0.414f}, {0.0f, -0.10f, 0.46f}, {0.0f, -0.03f, 0.36f}, {0.0f, -0.01f, 0.45f}, -0.05f, {-0.05f, 0.04f, 0.48f}, 0.10f, 25.0f * kPi / 180.0f, {0.0f, -0.015f, 0.06f}, magposes::kRpkHold, magposes::kRpkFingers, false, false, {{0.001f, 0.378f}, {-0.102f, 0.414f}, {-0.205f, 0.524f}}, magposes::kRpkLeverOverHold, magposes::kRpkLeverOverFingers, false, {}, 0.0f, {}, 0.08f, nullptr, magposes::kRpkLeverUnderHold, magposes::kRpkLeverUnderFingers};
constexpr handmag::Gun kAek971{{0.0f, 0.014f, 0.415f}, {0.0f, -0.088f, 0.436f}, {0.0f, -0.031f, 0.370f}, {0.0f, -0.014f, 0.453f}, -0.05f, {-0.031f, 0.054f, 0.451f}, 0.092f, 25.0f * kPi / 180.0f, {0.0f, 0.0f, 0.0f}, magposes::kAek971Hold, magposes::kAek971Fingers, false, false, {{0.005f, 0.378f}, {-0.082f, 0.395f}, {-0.169f, 0.433f}}, magposes::kAek971LeverHold, magposes::kAek971LeverFingers, false, {}, 0.0f, {}, 0.0f, nullptr, magposes::kAek971LeverUnderHold, magposes::kAek971LeverUnderFingers};
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

// Each gun with a lever: its rows and where the game's left palm rests on its front (skin.cpp's kHeldKinds: a squeeze nearer that is the front's).
struct Rig {
    const char* name;
    const handmag::Gun* gun;
    float front[3];
};
const Rig kRigs[] = {
    {"AN-94", &kAn94, {0.002f, -0.018f, 0.561f}},
    {"RPK-74M", &kRpk, {-0.001f, -0.029f, 0.585f}},
    {"AEK-971", &kAek971, {0.004f, -0.031f, 0.551f}},
    {"AK-74M", &kAk74, {0.001f, -0.031f, 0.548f}},
    {"AKS-74u", &kAks74u, {0.003f, -0.035f, 0.517f}},
    {"G3A3", &kG3, {0.020f, -0.007f, 0.616f}},
    {"UMP-45", &kUmp45, {0.004f, -0.026f, 0.538f}},
    {"KH2002", &kKh2002, {0.016f, -0.059f, 0.447f}},
    {"M16A4", &kM16, {0.014f, -0.041f, 0.636f}},
    {"M4A1", &kM4, {0.012f, -0.041f, 0.612f}},
    {"Mk11 Mod 0", &kMk11, {0.012f, -0.041f, 0.612f}},
    {"M240", &kM240, {0.018f, -0.065f, 0.678f}},
    {"M249", &m249rows::kMag, {m249rows::kLeftPalm[0], m249rows::kLeftPalm[1], m249rows::kLeftPalm[2]}},
};

// ---- 3x4 poses (rows: rotation | translation) ----
using M = std::array<float, 12>;
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
std::array<float, 3> Apply(const float* m, const float* p) {
    return {m[0] * p[0] + m[1] * p[1] + m[2] * p[2] + m[3], m[4] * p[0] + m[5] * p[1] + m[6] * p[2] + m[7], m[8] * p[0] + m[9] * p[1] + m[10] * p[2] + m[11]};
}
float Distance(const std::array<float, 3>& a, const std::array<float, 3>& b) {
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}
M Turn(float yaw, float pitch, float roll, float x, float y, float z) {
    const float cy = std::cos(yaw), sy = std::sin(yaw), cp = std::cos(pitch), sp = std::sin(pitch), cr = std::cos(roll), sr = std::sin(roll);
    const float ry[9] = {cy, 0, sy, 0, 1, 0, -sy, 0, cy}, rx[9] = {1, 0, 0, 0, cp, -sp, 0, sp, cp}, rz[9] = {cr, -sr, 0, sr, cr, 0, 0, 0, 1};
    float a[9], b[9];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            a[i * 3 + j] = 0.0f;
            for (int k = 0; k < 3; ++k)
                a[i * 3 + j] += ry[i * 3 + k] * rx[k * 3 + j];
        }
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            b[i * 3 + j] = 0.0f;
            for (int k = 0; k < 3; ++k)
                b[i * 3 + j] += a[i * 3 + k] * rz[k * 3 + j];
        }
    return {b[0], b[1], b[2], x, b[3], b[4], b[5], y, b[6], b[7], b[8], z};
}

// The singular values of a pose's 3x3, smallest first (the eigenvalues of its square, by Jacobi's turns).
std::array<float, 3> Axes(const float* m) {
    double a[3][3] = {};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            for (int k = 0; k < 3; ++k)
                a[i][j] += static_cast<double>(m[k * 4 + i]) * m[k * 4 + j];
    for (int sweep = 0; sweep < 30; ++sweep)
        for (int p = 0; p < 2; ++p)
            for (int q = p + 1; q < 3; ++q) {
                if (std::fabs(a[p][q]) < 1e-15)
                    continue;
                const double th = 0.5 * std::atan2(2.0 * a[p][q], a[q][q] - a[p][p]), c = std::cos(th), s = std::sin(th);
                for (int k = 0; k < 3; ++k) {
                    const double kp = a[k][p], kq = a[k][q];
                    a[k][p] = c * kp - s * kq, a[k][q] = s * kp + c * kq;
                }
                for (int k = 0; k < 3; ++k) {
                    const double pk = a[p][k], qk = a[q][k];
                    a[p][k] = c * pk - s * qk, a[q][k] = s * pk + c * qk;
                }
            }
    double e[3] = {a[0][0], a[1][1], a[2][2]};
    std::sort(e, e + 3);
    return {static_cast<float>(std::sqrt(std::max(e[0], 0.0))), static_cast<float>(std::sqrt(std::max(e[1], 0.0))), static_cast<float>(std::sqrt(std::max(e[2], 0.0)))};
}

// ---- hands.cpp's BlendAbout (copied): a and b blended about a point p of the bind space, p straight from where a puts it to where b does, the turn
// between them blended, the rest (the palm's widening, the fingers' slimming) as it is. ----
struct Quat {
    float x, y, z, w;
};
Quat MatrixToQuat(const float* m) {
    Quat q;
    const float trace = m[0] + m[4] + m[8];
    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        q = {(m[7] - m[5]) / s, (m[2] - m[6]) / s, (m[3] - m[1]) / s, 0.25f * s};
    } else if (m[0] > m[4] && m[0] > m[8]) {
        const float s = std::sqrt(1.0f + m[0] - m[4] - m[8]) * 2.0f;
        q = {0.25f * s, (m[1] + m[3]) / s, (m[2] + m[6]) / s, (m[7] - m[5]) / s};
    } else if (m[4] > m[8]) {
        const float s = std::sqrt(1.0f + m[4] - m[0] - m[8]) * 2.0f;
        q = {(m[1] + m[3]) / s, 0.25f * s, (m[5] + m[7]) / s, (m[2] - m[6]) / s};
    } else {
        const float s = std::sqrt(1.0f + m[8] - m[0] - m[4]) * 2.0f;
        q = {(m[2] + m[6]) / s, (m[5] + m[7]) / s, 0.25f * s, (m[3] - m[1]) / s};
    }
    return q;
}
void QuatToMatrix(const Quat& q, float* m) {
    const float x = q.x, y = q.y, z = q.z, w = q.w;
    m[0] = 1 - 2 * (y * y + z * z), m[1] = 2 * (x * y - z * w), m[2] = 2 * (x * z + y * w);
    m[3] = 2 * (x * y + z * w), m[4] = 1 - 2 * (x * x + z * z), m[5] = 2 * (y * z - x * w);
    m[6] = 2 * (x * z - y * w), m[7] = 2 * (y * z + x * w), m[8] = 1 - 2 * (x * x + y * y);
}
void Mul3(const float* a, const float* b, float* out) {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            out[i * 3 + j] = a[i * 3] * b[j] + a[i * 3 + 1] * b[3 + j] + a[i * 3 + 2] * b[6 + j];
}
void Transpose3(const float* a, float* out) {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            out[i * 3 + j] = a[j * 3 + i];
}
void BlendAbout(const float* a, const float* b, float s, const float* p, float* out) {
    float ma[9], mb[9];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            ma[i * 3 + j] = a[i * 4 + j], mb[i * 3 + j] = b[i * 4 + j];
    auto unit = [](Quat q) {
        const float n = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
        return n > 0.0f ? Quat{q.x / n, q.y / n, q.z / n, q.w / n} : Quat{0, 0, 0, 1};
    };
    Quat qa = unit(MatrixToQuat(ma)), qb = unit(MatrixToQuat(mb));
    if (qa.x * qb.x + qa.y * qb.y + qa.z * qb.z + qa.w * qb.w < 0)
        qb = {-qb.x, -qb.y, -qb.z, -qb.w};
    const Quat q = unit({qa.x + (qb.x - qa.x) * s, qa.y + (qb.y - qa.y) * s, qa.z + (qb.z - qa.z) * s, qa.w + (qb.w - qa.w) * s});
    float turnA[9], turnB[9], turn[9], back[9], restA[9], restB[9], rest[9], m[9];
    QuatToMatrix(qa, turnA), QuatToMatrix(qb, turnB), QuatToMatrix(q, turn);
    Transpose3(turnA, back), Mul3(back, ma, restA);
    Transpose3(turnB, back), Mul3(back, mb, restB);
    for (int i = 0; i < 9; ++i)
        rest[i] = restA[i] + (restB[i] - restA[i]) * s;
    Mul3(turn, rest, m);
    const std::array<float, 3> pa = Apply(a, p), pb = Apply(b, p);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            out[i * 4 + j] = m[i * 3 + j];
        out[i * 4 + 3] = pa[i] + (pb[i] - pa[i]) * s - (m[i * 3] * p[0] + m[i * 3 + 1] * p[1] + m[i * 3 + 2] * p[2]);
    }
}

// ---- the hands as hands.cpp has them ----
constexpr float kLeftAnchor[3] = {0.502f, 1.146f, 0.144f};  // hands.cpp's g_leftAnchor: the left palm's point (the right's: x negated)
constexpr float kHandMiddle[2][3] = {{0.5363f, 1.0976f, 0.2092f}, {-0.5363f, 1.0976f, 0.2092f}};  // the hand's middle (handmag.cpp's kHandInGlove)
constexpr float kLeftWidth = 1.15f;                                  // hands.cpp's g_leftWidth; the right glove's own (freetune's rightwidth) 1
constexpr float kSpread = 0.7f;                                      // hands.cpp's g_spread
constexpr float kRest[3] = {0.14f, -0.22f, -0.10f};                  // the scripted hand's rest from where it holds the seated magazine (the right's: x negated)
// Each finger's bones from the palm out (thumb, index ... little), the segments between handpose's joints (tools/handview.cpp's kSegmentBone).
constexpr int kSegmentBone[5][3] = {{18, 17, 2}, {9, 19, 15}, {7, 21, 22}, {11, 20, 10}, {13, 16, 3}};
const char* const kFingerName[5] = {"thumb", "index", "middle", "ring", "little"};

struct RunResult {
    float gloveLeast = 9.0f, gloveMost = 0.0f;  // the glove's axes
    float lengthLeast = 9.0f, lengthMost = 0.0f;  // the finger bones' lengths, joint to joint, of their own
    float boneLeast = 9.0f, boneMost = 0.0f;      // the finger bones' axes
    std::string worstWhere;                        // where the worst of them was
    float worstOff = 0.0f;
    bool taken = false, let = false;
    float pulled = 0.0f;
    bool stuck = false;  // something taken that isn't the lever (the magazine, the lid, the belt)
    float tipMost = 0.0f;   // the most a fingertip moved in a frame, in the glove (the fingers alone)
    std::string tipWhere;
    float jumpMost = 0.0f;  // the most a fingertip was off where the table of the frame before puts it at this frame's share
    std::string jumpWhere;
};

// The table the fingers are eased onto, this frame, after Update's r: handmag::EaseFingers as hands.cpp has it; oldLetGo, as it was before the review of
// 84654d7 (the gun's magazine table whenever it has one fitted, the lever's, the lid's or the belt's while the hand is on them: the check that the checks
// catch it).
void Ease(handmag::FittedFingers& h, const handmag::Result& r, bool mine, const float (*magazine)[12], const float (*other)[12], bool oldLetGo) {
    if (oldLetGo && magazine)
        h.table = magazine;
    handmag::EaseFingers(h, r, mine, magazine, other);
}

// One pull: side 0 the left, 1 the right; over (heldtest 3) or under (heldtest 4). oldWrist: the wrist as it was before 2026-11-25; oldLetGo: the let-go's
// table as it was before the review of 84654d7 (the checks that the checks catch them).
RunResult Pull(const Rig& rig, const M& gunPose, int side, bool overTheTop, bool oldWrist, bool oldLetGo) {
    const handmag::Gun& gun = *rig.gun;
    RunResult result;
    handmag::Clear();
    handmag::BeltReset();
    // A frame with nothing held and this hand fed: what the last run's hand had is let go of, its grip and trigger armed afresh.
    {
        handmag::Frame idle;
        idle.dt = kDt;
        idle.gun = &gun;
        std::memcpy(idle.gunPose, gunPose.data(), sizeof(idle.gunPose));
        idle.side = side;
        idle.left = true;
        std::memcpy(idle.glove, gunPose.data(), sizeof(idle.glove));
        for (int i = 0; i < 3; ++i)
            idle.frontGrip[i] = side == 1 && i == 0 ? -rig.front[0] : rig.front[i];
        idle.spare = true;
        for (int k = 0; k < 3; ++k)
            handmag::Update(idle);
        handmag::Clear();
    }
    const float sx = side == 1 ? -1.0f : 1.0f;
    float joints[5][4][3];
    handpose::Joints(joints);
    // Where this hand holds the seated magazine (handmag's own hold, the right's the left's mirrored about the magazine's middle), the knob and the rest from
    // there; the path (heldtest.h).
    float seated[12];
    handmag::GloveFor(gun, gunPose.data(), seated, side);
    const M toGun = Inv(gunPose);
    const std::array<float, 3> middleSeated = Apply(seated, kHandMiddle[side]);
    const std::array<float, 3> handOnMag = Apply(toGun.data(), middleSeated.data());
    float toLever[3], rest[3];
    for (int i = 0; i < 3; ++i) {
        toLever[i] = gun.lever[i] - handOnMag[i];
        rest[i] = (i == 0 ? sx : 1.0f) * kRest[i];
    }
    heldtest::Key keys[heldtest::kLeverKeys];
    heldtest::LeverKeys(overTheTop, toLever, rest, keys);
    // The wrist: heldtest.h's (or as it was), on the gun; the right's the left's mirror image (S w S, S = diag(-1, 1, 1): a turn stays a turn).
    float wrist[9];
    if (oldWrist) {
        constexpr float kBindAlong[3] = {0.437f, -0.544f, 0.716f}, kBindPalm[3] = {-0.639f, -0.748f, -0.179f}, kBindAcross[3] = {0.633f, -0.380f, -0.675f};
        const bool onItsLeft = gun.lever[0] > 0.01f, three = overTheTop;
        const float along[3] = {onItsLeft ? 0.2f : three ? -0.954f : -0.976f, onItsLeft ? 0.0f : three ? 0.0f : 0.217f, onItsLeft ? 0.98f : three ? 0.3f : 0.0f};
        const float palm[3] = {onItsLeft ? -0.98f : three ? 0.0f : -0.217f, onItsLeft ? 0.0f : three ? -1.0f : 0.976f, onItsLeft ? 0.2f : 0.0f};
        const float across[3] = {along[1] * palm[2] - along[2] * palm[1], along[2] * palm[0] - along[0] * palm[2], along[0] * palm[1] - along[1] * palm[0]};
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                wrist[i * 3 + j] = along[i] * kBindAlong[j] + palm[i] * kBindPalm[j] + across[i] * kBindAcross[j];
    } else {
        heldtest::LeverWrist(gun.lever[0] > 0.01f, !overTheTop, wrist);
    }
    if (side == 1)
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                wrist[i * 3 + j] *= (i == 0 ? -1.0f : 1.0f) * (j == 0 ? -1.0f : 1.0f);
    const M onGunTurn = {wrist[0], wrist[1], wrist[2], 0.0f, wrist[3], wrist[4], wrist[5], 0.0f, wrist[6], wrist[7], wrist[8], 0.0f};
    // The fingers: the hand's own (relaxed, a little apart), eased onto the gun's fitted fingers and into a fist as hands.cpp eases them.
    handmag::FittedFingers held;
    float fist = 0.0f;
    const float anchor[3] = {sx * kLeftAnchor[0], kLeftAnchor[1], kLeftAnchor[2]};
    std::array<float, 3> tipsBefore[5] = {};
    for (int frame = 0; frame <= static_cast<int>(3.9f / kDt); ++frame) {
        const float t = frame * kDt;
        float off[3];
        heldtest::KeyAt(keys, heldtest::kLeverKeys, t, off);
        // The scripted glove: the wrist on the gun, moved so the hand's middle is where the path has it.
        M glove = Mul(gunPose, onGunTurn);
        const float onGun[3] = {handOnMag[0] + off[0], handOnMag[1] + off[1], handOnMag[2] + off[2]};
        const std::array<float, 3> want = Apply(gunPose.data(), onGun), middle = Apply(glove.data(), kHandMiddle[side]);
        for (int i = 0; i < 3; ++i)
            glove[i * 4 + 3] += want[i] - middle[i];
        handmag::Frame f;
        f.dt = kDt;
        f.gun = &gun;
        f.gunTracked = true;
        std::memcpy(f.gunPose, gunPose.data(), sizeof(f.gunPose));
        f.side = side;
        f.left = true;
        std::memcpy(f.glove, glove.data(), sizeof(f.glove));
        for (int i = 0; i < 3; ++i) {
            f.gripRoom[i] = want[i];
            f.frontGrip[i] = side == 1 && i == 0 ? -rig.front[0] : rig.front[i];  // (the right's place on the front: the left's mirrored)
        }
        f.havePalm = false;  // (the front by the hand's middle, as the scripted left always had it)
        f.trigger = heldtest::LeverTrigger(t) ? 1.0f : 0.0f;
        f.squeeze = 0.0f;
        f.spare = true;
        f.handFree = true;
        const handmag::Result r = handmag::Update(f);
        const bool mine = r.side == side;
        result.taken = result.taken || (mine && r.leverTaken);
        if (mine && r.leverLet) {
            result.let = true;
            result.pulled = r.leverPulled;
        }
        result.stuck = result.stuck || (mine && (r.grabbed || r.took || r.lidTaken || r.beltTaken));
        // The glove as drawn: eased onto the fitted hand (or the fist's place) about the palm's point.
        M drawn = glove;
        if (mine && r.gloveBlend > 0.0f)
            BlendAbout(glove.data(), r.glove, r.gloveBlend, anchor, drawn.data());
        // The fingers as drawn: a fist on a lever with no table for this hand, eased in; the gun's own fitted fingers (its magazine's, or the lever's while
        // the hand is on it) by their share, eased; the left's posed in the left's layout, the right's flipped onto the right glove.
        fist += ((mine && r.fist && !r.leverFingers ? 1.0f : 0.0f) - fist) * 0.35f;
        handpose::Fingers fingers;
        fingers.spread = kSpread;
        if (side == 1)
            fingers.slim = 1.0f;  // (the free right glove's own: freetune's rightwidth 1 1)
        for (int i = 0; i < 4; ++i)
            fingers.curl[i] += (0.88f - fingers.curl[i]) * fist;
        fingers.thumb += (0.8f - fingers.thumb) * fist;
        const handmag::FittedFingers before = held;
        Ease(held, r, mine, gun.fingers, nullptr, oldLetGo);
        // The fingers posed and eased onto a table by a share, in the left's layout: the left's own, or the right's flipped back (each bone's twin).
        auto pose = [&](const float (*table)[12], float share, float (*bones)[12]) {
            if (side == 0) {
                handpose::Pose(fingers, kLeftWidth, bones);
                if (table && share > 0.001f)
                    for (int b = 0; b < handpose::kBones; ++b) {
                        float pivot[3], blended[12];
                        handpose::Pivot(b, pivot);
                        BlendAbout(bones[b], table[b], share, pivot, blended);
                        std::memcpy(bones[b], blended, sizeof(blended));
                    }
            } else {
                float right[handpose::kRightBones][12];
                handpose::PoseRight(fingers, 1.0f, right);
                if (table && share > 0.001f) {
                    float flipped[handpose::kRightBones][12];
                    handpose::LeftToRight(table, flipped);
                    for (int r2 = 0; r2 < handpose::kRightBones; ++r2) {
                        float pivot[3], blended[12];
                        handpose::PivotRight(handpose::kRightFirst + r2, pivot);
                        BlendAbout(right[r2], flipped[r2], share, pivot, blended);
                        std::memcpy(right[r2], blended, sizeof(blended));
                    }
                }
                for (int b = 0; b < handpose::kBones; ++b)
                    std::memcpy(bones[b], right[handpose::kRightOfLeft[b] - handpose::kRightFirst], sizeof(bones[b]));
            }
        };
        // Each finger's tip in the glove (its last bone's far joint; the right's joints the left's mirrored, as its bones are the left's twins mirrored).
        auto tipsOf = [&](const float (*bones)[12], std::array<float, 3> tips[5]) {
            for (int fi = 0; fi < 5; ++fi) {
                float tip[3];
                for (int i = 0; i < 3; ++i)
                    tip[i] = joints[fi][3][i] * (i == 0 ? sx : 1.0f);
                tips[fi] = Apply(bones[kSegmentBone[fi][2]], tip);
            }
        };
        float bones[handpose::kBones][12];
        pose(held.table, held.share, bones);
        // f5: the tips' move this frame; and, the table changed while the fingers were still on the one before, how far each tip is from where that one
        // puts it at this frame's share (none changed: the same, by the same sums).
        std::array<float, 3> tips[5];
        tipsOf(bones, tips);
        for (int fi = 0; fi < 5 && frame > 0; ++fi) {
            const float moved = Distance(tips[fi], tipsBefore[fi]);
            if (moved > result.tipMost) {
                result.tipMost = moved;
                char where[96];
                std::snprintf(where, sizeof(where), "%s at %.2f s", kFingerName[fi], t);
                result.tipWhere = where;
            }
        }
        if (before.table && before.share > 0.001f && before.table != held.table) {
            float kept[handpose::kBones][12];
            pose(before.table, held.share, kept);
            std::array<float, 3> keptTips[5];
            tipsOf(kept, keptTips);
            for (int fi = 0; fi < 5; ++fi) {
                const float apart = Distance(tips[fi], keptTips[fi]);
                if (apart > result.jumpMost) {
                    result.jumpMost = apart;
                    char where[96];
                    std::snprintf(where, sizeof(where), "%s at %.2f s", kFingerName[fi], t);
                    result.jumpWhere = where;
                }
            }
        }
        for (int fi = 0; fi < 5; ++fi)
            tipsBefore[fi] = tips[fi];
        // The measures, every frame.
        const std::array<float, 3> g = Axes(drawn.data());
        result.gloveLeast = std::min(result.gloveLeast, g[0]);
        result.gloveMost = std::max(result.gloveMost, g[2]);
        for (int fi = 0; fi < 5; ++fi)
            for (int s = 0; s < 3; ++s) {
                const int b = kSegmentBone[fi][s];
                const M bone = Mul(drawn, M{bones[b][0], bones[b][1], bones[b][2], bones[b][3], bones[b][4], bones[b][5], bones[b][6], bones[b][7], bones[b][8], bones[b][9], bones[b][10], bones[b][11]});
                // (The right's joints: the left's mirrored, as its bones are the left's twins mirrored.)
                float from[3], to[3];
                for (int i = 0; i < 3; ++i)
                    from[i] = joints[fi][s][i] * (i == 0 ? sx : 1.0f), to[i] = joints[fi][s + 1][i] * (i == 0 ? sx : 1.0f);
                const float bind = Distance({from[0], from[1], from[2]}, {to[0], to[1], to[2]});
                const float length = Distance(Apply(bone.data(), from), Apply(bone.data(), to)) / bind;
                const std::array<float, 3> a = Axes(bone.data());
                result.lengthLeast = std::min(result.lengthLeast, length);
                result.lengthMost = std::max(result.lengthMost, length);
                result.boneLeast = std::min(result.boneLeast, a[0]);
                result.boneMost = std::max(result.boneMost, a[2]);
                const float worst = std::max({std::fabs(length - 1.0f), a[2] - 1.0f, 0.88f - a[0]});
                if (worst > result.worstOff) {
                    result.worstOff = worst;
                    char where[96];
                    std::snprintf(where, sizeof(where), "%s's bone %d at %.2f s", kFingerName[fi], s + 1, t);
                    result.worstWhere = where;
                }
            }
    }
    return result;
}

}  // namespace

int main(int argc, char** argv) {
    // "old": the wrist as it was before 2026-11-25 (heldtest 4's palm 65 degrees from its fingers): the checks fail with it. "oldletgo": the let-go's
    // table as it was before the review of 84654d7 (the gun's magazine table whenever it has one fitted): f5 and f6 fail with it.
    const bool oldWrist = argc > 1 && !std::strcmp(argv[1], "old");
    const bool oldLetGo = argc > 1 && !std::strcmp(argv[1], "oldletgo");
    if (oldWrist)
        std::printf("(the wrist as it was before 2026-11-25: these checks are to fail)\n");
    if (oldLetGo)
        std::printf("(the let-go's table as it was before the review of 84654d7: these checks are to fail)\n");
    // f4: the wrist itself.
    {
        float w[9];
        bool turns = true;
        float worst = 0.0f;
        for (int left = 0; left < 2; ++left)
            for (int below = 0; below < 2; ++below) {
                heldtest::LeverWrist(left == 1, below == 1, w);
                const M m = {w[0], w[1], w[2], 0, w[3], w[4], w[5], 0, w[6], w[7], w[8], 0};
                const std::array<float, 3> a = Axes(m.data());
                const float det = w[0] * (w[4] * w[8] - w[5] * w[7]) - w[1] * (w[3] * w[8] - w[5] * w[6]) + w[2] * (w[3] * w[7] - w[4] * w[6]);
                worst = std::max({worst, std::fabs(a[0] - 1.0f), std::fabs(a[2] - 1.0f), std::fabs(det - 1.0f)});
                turns = turns && worst < 1e-4f;
            }
        char what[200];
        std::snprintf(what, sizeof(what), "f4 the wrist a turn over, under and from a handle's own side (each axis and its determinant within %.6f of 1)", worst);
        Check(turns, what);
        // Under: the palm up and square to the fingers; over: the palm down. (The gloves' own axes through the wrist.)
        constexpr float kAlong[3] = {0.437f, -0.544f, 0.716f}, kPalm[3] = {-0.639f, -0.748f, -0.179f};
        for (int below = 0; below < 2; ++below) {
            heldtest::LeverWrist(false, below == 1, w);
            float along[3], palm[3];
            for (int i = 0; i < 3; ++i) {
                along[i] = w[i * 3] * kAlong[0] + w[i * 3 + 1] * kAlong[1] + w[i * 3 + 2] * kAlong[2];
                palm[i] = w[i * 3] * kPalm[0] + w[i * 3 + 1] * kPalm[1] + w[i * 3 + 2] * kPalm[2];
            }
            const float la = std::sqrt(along[0] * along[0] + along[1] * along[1] + along[2] * along[2]), lp = std::sqrt(palm[0] * palm[0] + palm[1] * palm[1] + palm[2] * palm[2]);
            const float square = std::acos(std::clamp((along[0] * palm[0] + along[1] * palm[1] + along[2] * palm[2]) / (la * lp), -1.0f, 1.0f)) * 180.0f / kPi;
            std::snprintf(what, sizeof(what), "f4 %s: the palm %s (%.3f up), %.1f degrees from the fingers (square), the fingers to the gun's right (%.3f)",
                          below ? "under" : "over", below ? "up" : "down", palm[1] / lp, square, -along[0] / la);
            Check((below ? palm[1] / lp > 0.9f : palm[1] / lp < -0.9f) && std::fabs(square - 90.0f) < 0.5f && -along[0] / la > 0.9f, what);
        }
    }
    // f1-f3: every gun, either hand, over and under, the gun held still as it is in its own space and turned as the films' stand-in held it.
    const M poses[2] = {Turn(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), Turn(-70.0f * kPi / 180.0f, -12.0f * kPi / 180.0f, 8.0f * kPi / 180.0f, 0.12f, -0.21f, -0.38f)};
    int runs = 0;
    for (const Rig& rig : kRigs)
        for (int side = 0; side < 2; ++side)
            for (int over = 1; over >= 0; --over) {
                RunResult all;
                bool pulls = true;
                std::string pullWhy;
                for (int p = 0; p < 2; ++p) {
                    const RunResult r = Pull(rig, poses[p], side, over == 1, oldWrist, oldLetGo);
                    ++runs;
                    all.gloveLeast = std::min(all.gloveLeast, r.gloveLeast);
                    all.gloveMost = std::max(all.gloveMost, r.gloveMost);
                    all.lengthLeast = std::min(all.lengthLeast, r.lengthLeast);
                    all.lengthMost = std::max(all.lengthMost, r.lengthMost);
                    all.boneLeast = std::min(all.boneLeast, r.boneLeast);
                    all.boneMost = std::max(all.boneMost, r.boneMost);
                    if (r.worstOff > all.worstOff) {
                        all.worstOff = r.worstOff;
                        all.worstWhere = r.worstWhere + (p ? " (turned)" : "");
                    }
                    if (r.tipMost > all.tipMost) {
                        all.tipMost = r.tipMost;
                        all.tipWhere = r.tipWhere + (p ? " (turned)" : "");
                    }
                    if (r.jumpMost > all.jumpMost) {
                        all.jumpMost = r.jumpMost;
                        all.jumpWhere = r.jumpWhere + (p ? " (turned)" : "");
                    }
                    // (The path draws the hand 10 cm back: the lever as far, or all its travel if that's less: the G3's is 17 cm.)
                    const float expected = std::min(rig.gun->leverTravel, 0.10f);
                    const bool ok = r.taken && r.let && r.pulled >= 0.9f * expected && !r.stuck;
                    if (!ok && pullWhy.empty()) {
                        char why[160];
                        std::snprintf(why, sizeof(why), " (pose %d: %s, %s, drawn %.0f mm of %.0f%s)", p, r.taken ? "taken" : "NOT taken", r.let ? "let go of" : "NOT let go of", r.pulled * 1000.0f,
                                      expected * 1000.0f, r.stuck ? ", something else taken" : "");
                        pullWhy = why;
                    }
                    pulls = pulls && ok;
                }
                const char* hand = side == 1 ? "right" : "left";
                const char* way = over ? "over the top" : "from under it";
                char what[400];
                std::snprintf(what, sizeof(what), "f1 %s, the %s hand %s: the glove drawn a turn every frame (its axes %.4f to %.4f)", rig.name, hand, way, all.gloveLeast, all.gloveMost);
                Check(all.gloveLeast > 0.999f && all.gloveMost < 1.001f, what);
                std::snprintf(what, sizeof(what), "f2 %s, the %s hand %s: every finger bone its own length (%.3f to %.3f) and not stretched (its axes %.3f to %.3f; worst %s)", rig.name, hand, way,
                              all.lengthLeast, all.lengthMost, all.boneLeast, all.boneMost, all.worstWhere.empty() ? "none" : all.worstWhere.c_str());
                Check(all.lengthLeast > 0.98f && all.lengthMost < 1.02f && all.boneLeast > 0.86f && all.boneMost < 1.02f, what);
                std::snprintf(what, sizeof(what), "f3 %s, the %s hand %s: the lever taken by the trigger, drawn, let go of%s", rig.name, hand, way, pullWhy.c_str());
                Check(pulls, what);
                std::snprintf(what, sizeof(what), "f5 %s, the %s hand %s: the fingers ease out from the table they were on (each tip within 3 mm of it: %.1f mm at most%s%s; the most "
                              "a tip moved in a frame %.0f mm, the %s)", rig.name, hand, way, all.jumpMost * 1000.0f, all.jumpWhere.empty() ? "" : ", the ", all.jumpWhere.c_str(),
                              all.tipMost * 1000.0f, all.tipWhere.empty() ? "none" : all.tipWhere.c_str());
                Check(all.jumpMost <= 0.003f, what);
            }
    // f6: handmag::EaseFingers on its own, as hands.cpp calls it each frame for each hand (the tables told apart by where they are: the Mk11's magazine
    // and lever, the belt's pinch, the Stinger's aerial).
    {
        const float (*const mag)[12] = magposes::kMk11Fingers;
        const float (*const lever)[12] = magposes::kMk11LeverFingers;
        const float (*const pinch)[12] = magposes::kBeltPinchFingers;
        const float (*const aerial)[12] = magposes::kFim92LiftFingers[0];
        handmag::Result none, onLever, onBelt, took, holding;
        onLever.leverFingers = lever, onLever.fist = true;
        onBelt.leverFingers = pinch, onBelt.holding = true;  // (handmag.cpp's kBelt: the belt in the hand is held, the pinch its table)
        took.took = true, took.holding = true;
        holding.holding = true;
        auto frames = [&](handmag::FittedFingers& h, const handmag::Result& r, bool mine, const float (*other)[12], int n) {
            for (int i = 0; i < n; ++i)
                Ease(h, r, mine, mag, other, oldLetGo);
        };
        // A let-go: the hand on expect's table (its share near 1), then 12 frames, the first of what Update says then (first), the rest after (mine: they're
        // this hand's): the table still expect each frame, the share down from the frame before.
        auto keptThrough = [&](handmag::FittedFingers h, const float (*expect)[12], const handmag::Result& first, const handmag::Result& after, bool mine,
                               std::string& said) {
            const bool on = h.table == expect && h.share > 0.9f;
            float share = h.share;
            int frame = 0;
            bool kept = on;
            while (kept && frame < 12) {
                Ease(h, frame == 0 ? first : after, mine, mag, nullptr, oldLetGo);
                ++frame;
                kept = h.table == expect && h.share < share;
                share = h.share;
            }
            char line[120];
            if (!on)
                std::snprintf(line, sizeof(line), "NOT on its table before the let-go (the share %.3f)", share);
            else if (kept)
                std::snprintf(line, sizeof(line), "kept 12 frames, the share down to %.3f", share);
            else
                std::snprintf(line, sizeof(line), "NOT kept: %s at the let-go's frame %d (the share %.3f)", h.table == expect ? "the share up" : "another table", frame, share);
            said = line;
            return kept;
        };
        std::string said;
        handmag::FittedFingers h;
        frames(h, onLever, true, nullptr, 20);
        bool ok = keptThrough(h, lever, none, none, true, said);
        Check(ok, "f6 a lever let go of: its fingers ease out from the lever's table, not the gun's magazine's (" + said + ")");
        h = {};
        frames(h, onBelt, true, nullptr, 20);
        ok = keptThrough(h, pinch, none, none, true, said);
        Check(ok, "f6 the belt let go of (the M240's, the M249's): from its pinch's table (" + said + ")");
        h = {};
        Ease(h, took, true, mag, nullptr, oldLetGo);
        const bool atOnce = h.table == mag && h.share == 1.0f;
        frames(h, holding, true, nullptr, 10);
        ok = keptThrough(h, mag, none, none, true, said) && atOnce;
        Check(ok, std::string("f6 a magazine taken: on its table at once (") + (atOnce ? "the share 1" : "NOT at once") + "), let go of: eased out from it (" + said + ")");
        h = {};
        frames(h, none, true, aerial, 20);
        ok = keptThrough(h, aerial, none, none, true, said);
        Check(ok, "f6 the Stinger's aerial let go of, the gun with a fitted magazine: from the aerial's table (" + said + ")");
        // This hand lets go of the lever as the other takes a magazine (handmag serves one hand: its Result the other's now): this one's eases out still.
        h = {};
        frames(h, onLever, true, nullptr, 20);
        ok = keptThrough(h, lever, took, holding, false, said);
        Check(ok, "f6 the other hand takes a magazine as this one lets go of a lever: this one's fingers ease out from the lever's, not the magazine's (" + said + ")");
    }
    std::printf("%d checks, %d failed (%d pulls of %d guns, either hand, over and under, two places of the gun; 72 Hz)\n", g_checks, g_failed, runs,
                static_cast<int>(std::size(kRigs)));
    return g_failed;
}

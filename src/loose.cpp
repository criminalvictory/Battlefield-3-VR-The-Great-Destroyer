// Loose things (loose.h): a small rigid-body step, the study's of 2026-11-05 (its reference model ran the numbers first). Changed since, by the offline
// test (tools/loosetest.cpp): a well is a slide along its line on the gun, its walls gripping by what presses it on them, still and sliding (a barrel's
// case held until gravity beats them; the gun's own shove felt over four frames, so its tracking's shake doesn't creep anything out); the hand's speed
// is the middle of its last three steps, not their mean (a tracking jump as it's let go of isn't a throw); round things are rings; casings are many
// and cheap; each thing keeps the ground it was given, and only a firm one moves one lying. The player's rifle (gunbody.h, 2026-11-06): kept (never
// pushed out, never gone for lying), touching the ground by its own hull's points, let go of by either hand (ReleaseBy: each hand's own last frames).
#include "loose.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <iterator>

namespace loose {
namespace {

// ---------------------------------------------------------------- numbers
constexpr float kGravity = 9.81f;
constexpr float kSubstep = 1.0f / 288.0f;  // 4 a frame at 72 Hz
constexpr int kMostSubsteps = 8;
constexpr int kIterations = 6, kCasingIterations = 4;
constexpr float kSkin = 0.001f;            // m: a point this near a plane is in contact
constexpr float kSlop = 0.0005f;           // m: left in it, so contacts stay from substep to substep
constexpr float kBounceFrom = 0.5f;        // m/s: slower into the ground, no bounce (rest, not jitter)
constexpr float kAir = 0.6f;               // 0.5 rho Cd (rho 1.2, Cd 1): drag = kAir * area * v^2
constexpr float kSleepSpeed = 0.04f, kSleepTurn = 0.35f, kSleepFor = 0.30f, kSleepForEdge = 1.0f;
// A gun come to a stop with its top still up on a base narrower than kPropWide (the M1014 on its grip's bottom and its magazine tube's end, 3.4 cm
// wide: 8 of 195 near-upright drops): a real one so balanced goes over at a breath. It's rolled about its own length, toward the
// side it leans to, a little harder each time; after kPropMost it lies as it is (against something). Only as it would otherwise go to rest so: a
// gun that falls over by itself is never touched (every other narrow gun: bodytest's rows are as they were).
constexpr float kPropUp = 0.7f, kPropWide = 0.04f, kPropTurn = 2.0f;
constexpr int kPropMost = 6;
constexpr float kWake = 0.002f;            // m: one lying this far into a firm ground, or this far above all of them: it settles again
constexpr float kAskResting = 0.25f;       // s: the ground under one at rest asked again this often
constexpr float kHeard = 0.35f;            // m/s into the ground: heard
constexpr float kSoundApart = 0.08f, kSoundGate = 0.30f;  // s (the game's impact player: an event gate of 0.3 s)
constexpr float kTipEngaged = 0.012f;      // m of it left in the well when the walls stop holding its angle
constexpr float kTipMost = 6.0f;           // rad/s
constexpr float kMinPush = 0.35f;          // m/s: the least a magazine leaves its well at, whatever way the gun points
constexpr float kGripMoving = 0.8f;        // a barrel's walls' grip once it slides, of their grip on it still
constexpr float kMostShove = 30.0f;        // m/s^2: the most of the gun's own shove felt in a well (more is a tracking jump)
constexpr float kMostSpeed = 8.0f, kMostTurn = 30.0f;  // a throw: m/s, rad/s (handmag's 3 m/s cap was for a drop)
constexpr float kUnseenFor = 0.5f;         // s out of view before it may go
constexpr float kSeenSizes = 150.0f, kSeenNearest = 3.0f, kSeenFarthest = 40.0f;  // seen within 150 times its size (half a degree across)
constexpr float kNoGroundFor = 5.0f;       // s falling with no ground ever given: gone (fallen out of the world)
constexpr float kFreeFor = 30.0f;          // s free and never at rest (down a slope with no end): gone once unseen
constexpr float kJumpPlace = 0.5f, kJumpTurn = 1.5f;  // m, rad in a frame: the hand or the gun is another (a gun switch, tracking found again)
constexpr float kPi = 3.14159265f;

float g_push = 0.8f;  // m/s out of the well (skin.cpp's kEjectSpeed)
float g_wellFriction = 0.25f;
float g_lie = kLieSeconds;
float g_bounceScale = 1.0f;
int g_mostBig = kMost, g_mostSmall = kCasings;

// Each ground: its bounce (a loaded metal magazine on it), its friction, and how rough it is to roll on: its bumps' height (m), what a round end
// rolling over them is held back by (rolling resistance is that over its radius: the launcher's 4.45 cm case on dirt 0.09, as the study had it,
// a casing's 4.8 mm on tarmac 0.21; with the study's one number for every size, a casing rolled 1.5 m over tarmac for 5 s: loosetest). The game's
// index (loose.h's Surface).
struct Feel {
    float bounce, friction, rough;
};
constexpr Feel kFeel[kSurfaces] = {
    {0.10f, 0.70f, 0.0040f},  // dirt
    {0.25f, 0.55f, 0.0010f},  // tarmac
    {0.08f, 0.60f, 0.0055f},  // grass
    {0.06f, 0.65f, 0.0070f},  // tall grass
    {0.12f, 0.65f, 0.0030f},  // gravel
    {0.30f, 0.35f, 0.0007f},  // metal
    {0.30f, 0.35f, 0.0007f},  // a vehicle's metal
    {0.05f, 0.75f, 0.0070f},  // sand
    {0.00f, 0.90f, 0.0130f},  // water (it lies on the bottom: nothing deeper is known)
    {0.25f, 0.45f, 0.0013f},  // wood
    {0.08f, 0.65f, 0.0055f},  // forest floor
    {0.15f, 0.40f, 0.0013f},  // wet tarmac
    {0.25f, 0.45f, 0.0013f},  // wood (indoors)
};

// ---------------------------------------------------------------- small maths
struct V3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
};
V3 operator+(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
V3 operator-(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
V3 operator*(V3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
float Dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
V3 Cross(V3 a, V3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
float Len(V3 a) { return std::sqrt(Dot(a, a)); }
V3 Unit(V3 a) {
    const float l = Len(a);
    return l > 1e-12f ? a * (1.0f / l) : V3{};
}
V3 Load(const float* p) { return {p[0], p[1], p[2]}; }
void Store(V3 a, float* p) { p[0] = a.x, p[1] = a.y, p[2] = a.z; }
bool Finite(const float* p, int n) {
    for (int i = 0; i < n; ++i)
        if (!std::isfinite(p[i]))
            return false;
    return true;
}
bool Finite(V3 a) { return std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z); }
float Median3(float a, float b, float c) { return std::max(std::min(a, b), std::min(std::max(a, b), c)); }
V3 Median(const V3 (&s)[3]) { return {Median3(s[0].x, s[1].x, s[2].x), Median3(s[0].y, s[1].y, s[2].y), Median3(s[0].z, s[1].z, s[2].z)}; }

struct M3 {  // row-major
    float m[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
};
V3 operator*(const M3& a, V3 v) {
    return {a.m[0] * v.x + a.m[1] * v.y + a.m[2] * v.z, a.m[3] * v.x + a.m[4] * v.y + a.m[5] * v.z, a.m[6] * v.x + a.m[7] * v.y + a.m[8] * v.z};
}
M3 operator*(const M3& a, const M3& b) {
    M3 o;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            o.m[i * 3 + j] = a.m[i * 3] * b.m[j] + a.m[i * 3 + 1] * b.m[3 + j] + a.m[i * 3 + 2] * b.m[6 + j];
    return o;
}
M3 Transposed(const M3& a) {
    M3 o;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            o.m[i * 3 + j] = a.m[j * 3 + i];
    return o;
}
V3 TMul(const M3& a, V3 v) { return Transposed(a) * v; }
M3 Inverse(const M3& a) {
    const float* m = a.m;
    const float c0 = m[4] * m[8] - m[5] * m[7], c1 = m[5] * m[6] - m[3] * m[8], c2 = m[3] * m[7] - m[4] * m[6];
    const float det = m[0] * c0 + m[1] * c1 + m[2] * c2;
    M3 o;
    if (std::fabs(det) < 1e-30f)
        return o;
    const float k = 1.0f / det;
    o.m[0] = c0 * k, o.m[1] = (m[2] * m[7] - m[1] * m[8]) * k, o.m[2] = (m[1] * m[5] - m[2] * m[4]) * k;
    o.m[3] = c1 * k, o.m[4] = (m[0] * m[8] - m[2] * m[6]) * k, o.m[5] = (m[2] * m[3] - m[0] * m[5]) * k;
    o.m[6] = c2 * k, o.m[7] = (m[1] * m[6] - m[0] * m[7]) * k, o.m[8] = (m[0] * m[4] - m[1] * m[3]) * k;
    return o;
}
M3 RotationOf(const float* rows) {  // a 3x4 row-major matrix's turn
    M3 o;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            o.m[i * 3 + j] = rows[i * 4 + j];
    return o;
}
V3 PlaceOf(const float* rows) { return {rows[3], rows[7], rows[11]}; }

struct Q {
    float w = 1.0f, x = 0.0f, y = 0.0f, z = 0.0f;
};
Q Mul(Q a, Q b) {
    return {a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z, a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y, a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w};
}
Q Normal(Q q) {
    const float l = std::sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    return l > 1e-12f ? Q{q.w / l, q.x / l, q.y / l, q.z / l} : Q{};
}
bool Finite(Q q) { return std::isfinite(q.w) && std::isfinite(q.x) && std::isfinite(q.y) && std::isfinite(q.z); }
M3 ToM3(Q q) {
    M3 r;
    r.m[0] = 1 - 2 * (q.y * q.y + q.z * q.z), r.m[1] = 2 * (q.x * q.y - q.w * q.z), r.m[2] = 2 * (q.x * q.z + q.w * q.y);
    r.m[3] = 2 * (q.x * q.y + q.w * q.z), r.m[4] = 1 - 2 * (q.x * q.x + q.z * q.z), r.m[5] = 2 * (q.y * q.z - q.w * q.x);
    r.m[6] = 2 * (q.x * q.z - q.w * q.y), r.m[7] = 2 * (q.y * q.z + q.w * q.x), r.m[8] = 1 - 2 * (q.x * q.x + q.y * q.y);
    return r;
}
Q FromM3(const M3& a) {
    const float* m = a.m;
    const float trace = m[0] + m[4] + m[8];
    Q q;
    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        q = {0.25f * s, (m[7] - m[5]) / s, (m[2] - m[6]) / s, (m[3] - m[1]) / s};
    } else if (m[0] > m[4] && m[0] > m[8]) {
        const float s = std::sqrt(1.0f + m[0] - m[4] - m[8]) * 2.0f;
        q = {(m[7] - m[5]) / s, 0.25f * s, (m[1] + m[3]) / s, (m[2] + m[6]) / s};
    } else if (m[4] > m[8]) {
        const float s = std::sqrt(1.0f + m[4] - m[0] - m[8]) * 2.0f;
        q = {(m[2] - m[6]) / s, (m[1] + m[3]) / s, 0.25f * s, (m[5] + m[7]) / s};
    } else {
        const float s = std::sqrt(1.0f + m[8] - m[0] - m[4]) * 2.0f;
        q = {(m[3] - m[1]) / s, (m[2] + m[6]) / s, (m[5] + m[7]) / s, 0.25f * s};
    }
    return Normal(q);
}
M3 Orthonormal(const M3& a) { return ToM3(FromM3(a)); }  // a bone's turn with any scale or skew taken out
Q Turned(Q q, V3 w, float h) {  // turned by the world's angular velocity w over h
    const float speed = Len(w), a = speed * h;
    if (a < 1e-9f)
        return q;
    const V3 n = w * (1.0f / speed);
    const float s = std::sin(a * 0.5f);
    return Normal(Mul({std::cos(a * 0.5f), n.x * s, n.y * s, n.z * s}, q));
}
// The turn from a to b, as a rotation vector (axis times angle).
V3 TurnBetween(const M3& a, const M3& b) {
    const M3 d = b * Transposed(a);
    const V3 axis{d.m[7] - d.m[5], d.m[2] - d.m[6], d.m[3] - d.m[1]};  // twice the sine of the angle, along the axis
    const float s = Len(axis);
    const float angle = std::atan2(0.5f * s, 0.5f * (d.m[0] + d.m[4] + d.m[8] - 1.0f));
    return s > 1e-12f ? axis * (angle / s) : V3{};
}

// ---------------------------------------------------------------- shapes
constexpr int kPoints = 22, kDiscs = 4;
struct Shape {
    V3 com;                    // its middle in its own vertices' space
    M3 inertia;                // about its middle, per kg, on its own axes
    int points = 0;
    V3 point[kPoints];         // its corners (and its back's curve), from its middle
    int discs = 0;
    V3 discCentre[kDiscs], discAxis[kDiscs];  // its rings, from its middle
    float discRadius[kDiscs] = {};
    float area = 0.0f;         // its biggest face, for the air
    float size = 0.0f;         // its biggest size, for how far it's seen
};

// A magazine: its top end and its bottom end, each its depth front to back, the bottom moved forward by the sweep, the back bowed by the sag. Its
// corners and, bowed, its back's curve at eighths of its length (where the shape's hull is: the front's curve is hollow), so lying on its curved
// back it's never more than a millimetre in (the curve's sag over 64 between two: the RPK's 3.7 cm bow, 0.6 mm; at sixths it lay 1.3 mm in).
void BuildBox(const Spec& s, Shape& out) {
    const float half = (s.boxMax[0] - s.boxMin[0]) * 0.5f, cx = (s.boxMax[0] + s.boxMin[0]) * 0.5f;
    const float top = s.boxMax[1], bottom = s.boxMin[1], length = top - bottom;
    const float zSpan = s.boxMax[2] - s.boxMin[2];
    const float depth = std::max(zSpan - std::fabs(s.sweep), 0.002f);
    const float backTop = s.sweep >= 0.0f ? s.boxMin[2] : s.boxMin[2] - s.sweep;
    auto back = [&](float t) { return backTop + s.sweep * t - s.sag * 4.0f * t * (1.0f - t); };  // t 0 its top, 1 its bottom
    constexpr int kX = 4, kY = 12, kZ = 6;
    V3 samples[kX * kY * kZ];
    V3 sum;
    int n = 0;
    for (int iy = 0; iy < kY; ++iy) {
        const float t = (iy + 0.5f) / kY, y = top - length * t, b = back(t);
        for (int iz = 0; iz < kZ; ++iz)
            for (int ix = 0; ix < kX; ++ix) {
                samples[n] = {cx - half + 2.0f * half * (ix + 0.5f) / kX, y, b + depth * (iz + 0.5f) / kZ};
                sum = sum + samples[n++];
            }
    }
    out.com = sum * (1.0f / static_cast<float>(n));
    const float sx = 2.0f * half / kX, sy = length / kY, sz = depth / kZ;  // each sample a little box of its own
    float I[9] = {};
    for (int i = 0; i < n; ++i) {
        const V3 r = samples[i] - out.com;
        I[0] += r.y * r.y + r.z * r.z + (sy * sy + sz * sz) / 12.0f;
        I[4] += r.x * r.x + r.z * r.z + (sx * sx + sz * sz) / 12.0f;
        I[8] += r.x * r.x + r.y * r.y + (sx * sx + sy * sy) / 12.0f;
        I[1] -= r.x * r.y, I[2] -= r.x * r.z, I[5] -= r.y * r.z;
    }
    I[3] = I[1], I[6] = I[2], I[7] = I[5];
    for (int i = 0; i < 9; ++i)
        out.inertia.m[i] = I[i] / static_cast<float>(n);
    for (float side : {-1.0f, 1.0f}) {
        const float x = cx + side * half;
        const V3 corners[4] = {{x, top, back(0.0f)}, {x, top, back(0.0f) + depth}, {x, bottom, back(1.0f)}, {x, bottom, back(1.0f) + depth}};
        for (const V3& c : corners)
            out.point[out.points++] = c - out.com;
        if (s.sag > 0.003f)
            for (int k = 1; k <= 7; ++k) {
                const float t = k / 8.0f;
                out.point[out.points++] = V3{x, top - length * t, back(t)} - out.com;
            }
    }
    float sizes[3] = {2.0f * half, length, zSpan};
    std::sort(sizes, sizes + 3);
    out.size = sizes[2];
    out.area = sizes[2] * sizes[1];
}

// A round thing: its wall along it (by its girth there: a cone between rings), and a solid head at its back a tenth of its length thick.
void BuildRound(const Spec& s, Shape& out) {
    const int axis = std::clamp(s.round, 0, 2), u = (axis + 1) % 3, w = (axis + 2) % 3;
    const int rings = std::clamp(s.rings, 2, kDiscs);
    const float back = s.ringAt[0], front = s.ringAt[rings - 1];
    const float length = std::max(front - back, 0.001f);
    auto place = [&](float along, float radius, float angle) {
        float c[3];
        c[axis] = along, c[u] = s.roundAt[0] + radius * std::cos(angle), c[w] = s.roundAt[1] + radius * std::sin(angle);
        return V3{c[0], c[1], c[2]};
    };
    auto radiusAt = [&](float along) {
        for (int i = 0; i + 1 < rings; ++i)
            if (along <= s.ringAt[i + 1] || i + 2 == rings) {
                const float span = s.ringAt[i + 1] - s.ringAt[i];
                const float k = span > 1e-6f ? std::clamp((along - s.ringAt[i]) / span, 0.0f, 1.0f) : 0.0f;
                return s.ringRadius[i] + (s.ringRadius[i + 1] - s.ringRadius[i]) * k;
            }
        return s.ringRadius[0];
    };
    constexpr int kAlong = 24, kRound = 12, kHead = 3;
    V3 samples[kAlong * kRound + kHead * kRound];
    float weights[std::size(samples)] = {};
    int n = 0;
    const float head = std::clamp(s.head, 0.0f, 1.0f);
    float girth = 0.0f;
    for (int i = 0; i < kAlong; ++i)
        girth += radiusAt(back + length * (i + 0.5f) / kAlong);
    for (int i = 0; i < kAlong; ++i) {
        const float along = back + length * (i + 0.5f) / kAlong, r = radiusAt(along);
        for (int k = 0; k < kRound; ++k) {
            samples[n] = place(along, r, 2.0f * kPi * k / kRound);
            weights[n++] = girth > 0.0f ? (1.0f - head) * r / (girth * kRound) : 0.0f;
        }
    }
    if (head > 0.0f) {
        float share = 0.0f;  // each ring of the head by its share of the disc's area
        for (int j = 0; j < kHead; ++j)
            share += j + 0.5f;
        for (int j = 0; j < kHead; ++j)
            for (int k = 0; k < kRound; ++k) {
                samples[n] = place(back + 0.05f * length, s.ringRadius[0] * (j + 0.5f) / kHead, 2.0f * kPi * (k + 0.5f) / kRound);
                weights[n++] = head * (j + 0.5f) / (share * kRound);
            }
    }
    float total = 0.0f;
    V3 sum;
    for (int i = 0; i < n; ++i)
        sum = sum + samples[i] * weights[i], total += weights[i];
    out.com = total > 0.0f ? sum * (1.0f / total) : place(0.5f * (back + front), 0.0f, 0.0f);
    float I[9] = {};
    for (int i = 0; i < n; ++i) {
        const V3 r = samples[i] - out.com;
        const float m = weights[i];
        I[0] += m * (r.y * r.y + r.z * r.z), I[4] += m * (r.x * r.x + r.z * r.z), I[8] += m * (r.x * r.x + r.y * r.y);
        I[1] -= m * r.x * r.y, I[2] -= m * r.x * r.z, I[5] -= m * r.y * r.z;
    }
    I[3] = I[1], I[6] = I[2], I[7] = I[5];
    // A thin wall's the least turn about its own axis it can have; a hair more about the others keeps the inverse sane for a short fat one.
    for (int i = 0; i < 9; ++i)
        out.inertia.m[i] = total > 0.0f ? I[i] / total : 0.0f;
    for (int i = 0; i < 3; ++i)
        out.inertia.m[i * 4] += 1e-7f;
    const V3 along{axis == 0 ? 1.0f : 0.0f, axis == 1 ? 1.0f : 0.0f, axis == 2 ? 1.0f : 0.0f};
    float widest = 0.0f;
    for (int i = 0; i < rings; ++i) {
        out.discCentre[i] = place(s.ringAt[i], 0.0f, 0.0f) - out.com;
        out.discAxis[i] = along;
        out.discRadius[i] = std::max(s.ringRadius[i], 0.0005f);
        widest = std::max(widest, out.discRadius[i]);
    }
    out.discs = rings;
    out.size = std::max(length, 2.0f * widest);
    out.area = length * 2.0f * widest;
}

// A hull's points as given, at most kPoints (none for a round thing).
int HullPoints(const Spec& s) { return s.round < 0 && s.hull ? std::clamp(s.hullPoints, 0, kPoints) : 0; }

void Build(const Spec& s, Shape& out) {
    out = Shape{};
    if (s.round >= 0)
        BuildRound(s, out);
    else
        BuildBox(s, out);
    // A hull (a gun): where it touches is the hull's points, from its middle; its weight, turn, area and size stay the box's. Broken points are left
    // out; none left, the box's corners stay.
    V3 hull[kPoints];
    int points = 0;
    for (int i = 0; i < HullPoints(s); ++i)
        if (Finite(s.hull[i], 3))
            hull[points++] = Load(s.hull[i]) - out.com;
    if (points > 0) {
        out.points = points;
        std::copy(hull, hull + points, out.point);
    }
}

// The furthest it reaches along a way in its own axes, from its middle.
float Support(const Shape& s, V3 way) {
    float most = -1e9f;
    for (int i = 0; i < s.points; ++i)
        most = std::max(most, Dot(s.point[i], way));
    for (int i = 0; i < s.discs; ++i) {
        const V3 across = way - s.discAxis[i] * Dot(way, s.discAxis[i]);
        most = std::max(most, Dot(s.discCentre[i], way) + s.discRadius[i] * Len(across));
    }
    return most > -1e8f ? most : 0.0f;
}

// The table: boxes READ from skin.cpp (kMagMin/kMagMax, HeldMagBones::boxMin/boxMax, kPistolMagMin/Max), the launcher's case from rocket.cpp's
// numbers (its axis at x 0, y -0.0935 from z -0.481 to 0.160, the flange to -0.447); sweep and sag from handmag.h's back edges (the AK family, the
// G3, the RPK), the STANAGs' their box's depth less 6.6 cm; the M320's case 46 mm long (a real one's: the round's mesh is 11.9 cm with its nose)
// and 5.1 cm across (the mesh's); casings and masses real-world (empty, a round).
const Spec kSpecs[] = {
    {"m416", {-0.011f, -0.304f, 0.038f}, {0.011f, -0.090f, 0.131f}, 0.027f, 0.0f, 0.13f, 0.0123f, 30, 1.0f},
    {"m16a4", {-0.017f, -0.322f, 0.076f}, {0.017f, -0.093f, 0.173f}, 0.031f, 0.0f, 0.12f, 0.0123f, 30, 1.0f},
    {"m4a1", {-0.013f, -0.313f, 0.076f}, {0.013f, -0.093f, 0.172f}, 0.030f, 0.0f, 0.12f, 0.0123f, 30, 1.0f},
    {"kh2002", {-0.013f, -0.189f, 0.068f}, {0.013f, -0.088f, 0.123f}, 0.0f, 0.0f, 0.12f, 0.0123f, 30, 1.0f},
    {"an94", {-0.013f, -0.266f, 0.070f}, {0.013f, -0.071f, 0.192f}, 0.056f, 0.011f, 0.23f, 0.0102f, 30, 1.15f},
    {"aek971", {-0.013f, -0.282f, 0.068f}, {0.013f, -0.089f, 0.189f}, 0.055f, 0.0105f, 0.23f, 0.0102f, 30, 1.15f},
    {"ak74", {-0.013f, -0.287f, 0.070f}, {0.013f, -0.092f, 0.191f}, 0.055f, 0.0113f, 0.23f, 0.0102f, 30, 1.15f},
    {"rpk", {-0.018f, -0.307f, 0.061f}, {0.018f, -0.077f, 0.272f}, 0.146f, 0.037f, 0.29f, 0.0102f, 45, 1.15f},
    {"g3a3", {-0.016f, -0.240f, 0.058f}, {0.015f, -0.084f, 0.149f}, 0.022f, 0.008f, 0.25f, 0.024f, 20, 1.0f},
    {"m240", {-0.124f, -0.247f, 0.023f}, {0.090f, -0.098f, 0.147f}, 0.0f, 0.0f, 0.40f, 0.027f, 100, 0.6f},
    {"m9", {-0.0132f, -0.2041f, 0.0743f}, {0.0130f, -0.0926f, 0.1298f}, -0.030f, 0.0f, 0.09f, 0.012f, 15, 1.0f},
    {.name = "smaw", .massEmpty = 2.0f, .bounce = 1.2f, .round = 2, .roundAt = {0.0f, -0.0935f}, .rings = 3, .ringAt = {-0.481f, -0.447f, 0.160f},
     .ringRadius = {0.068f, 0.068f, 0.0445f}},
    {.name = "m320", .massEmpty = 0.045f, .bounce = 1.3f, .round = 2, .rings = 2, .ringAt = {0.0f, 0.046f}, .ringRadius = {0.0255f, 0.0255f},
     .head = 0.4f},
    // A 12-gauge shell, live (the M1014's, as its own mesh draws it: 6.4 cm long, 2.1 cm across; shellload.h): let go of by the hand or thrown out by the
    // bolt, never a fired hull (those stay the game's own effect). About 45 g with its load; its brass head and powder at its back.
    {.name = "12g", .massEmpty = 0.045f, .bounce = 1.2f, .round = 2, .rings = 2, .ringAt = {0.0f, 0.064f}, .ringRadius = {0.0105f, 0.0105f},
     .head = 0.5f},
    // Brass: 5.56 x 45 (44.7 mm, 9.6 mm at its base, its shoulder 9.0 mm at 36 mm, its neck 6.4 mm), 5.45 x 39, 7.62 x 51, 9 x 19.
    {.name = "556", .massEmpty = 0.006f, .bounce = 1.6f, .round = 2, .rings = 3, .ringAt = {0.0f, 0.036f, 0.0447f}, .ringRadius = {0.0048f, 0.0045f, 0.0032f},
     .head = 0.35f, .casing = true},
    {.name = "545", .massEmpty = 0.0055f, .bounce = 1.6f, .round = 2, .rings = 3, .ringAt = {0.0f, 0.031f, 0.0398f}, .ringRadius = {0.0050f, 0.0045f, 0.0031f},
     .head = 0.35f, .casing = true},
    {.name = "762", .massEmpty = 0.012f, .bounce = 1.6f, .round = 2, .rings = 3, .ringAt = {0.0f, 0.040f, 0.0512f}, .ringRadius = {0.0060f, 0.0057f, 0.0044f},
     .head = 0.35f, .casing = true},
    {.name = "9mm", .massEmpty = 0.004f, .bounce = 1.6f, .round = 2, .rings = 2, .ringAt = {0.0f, 0.0192f}, .ringRadius = {0.0050f, 0.0049f},
     .head = 0.4f, .casing = true},
};

// Shapes built once for each spec (the table's, or a caller's own: told apart by what's in them, not where they are). A hull's points are kept as
// they were (a caller's own may be gone by the next start): its kept pointer is only ever asked whether there was one.
struct Built {
    bool have = false;
    Spec spec{};
    float hull[kPoints][3] = {};
    Shape shape;
};
bool Same(const Built& built, const Spec& b) {
    const Spec& a = built.spec;
    if (a.name != b.name && (!a.name || !b.name || std::strcmp(a.name, b.name) != 0))
        return false;
    for (int i = 0; i < 3; ++i)
        if (a.boxMin[i] != b.boxMin[i] || a.boxMax[i] != b.boxMax[i])
            return false;
    for (int i = 0; i < 4; ++i)
        if (a.ringAt[i] != b.ringAt[i] || a.ringRadius[i] != b.ringRadius[i])
            return false;
    if (a.keep != b.keep || a.hullPoints != b.hullPoints || (a.hull != nullptr) != (b.hull != nullptr))
        return false;
    for (int i = 0; i < HullPoints(b); ++i)
        for (int k = 0; k < 3; ++k)
            if (built.hull[i][k] != b.hull[i][k])
                return false;
    return a.sweep == b.sweep && a.sag == b.sag && a.massEmpty == b.massEmpty && a.massRound == b.massRound && a.capacity == b.capacity &&
           a.bounce == b.bounce && a.round == b.round && a.roundAt[0] == b.roundAt[0] && a.roundAt[1] == b.roundAt[1] && a.rings == b.rings &&
           a.head == b.head && a.casing == b.casing;
}
Built g_built[24];
int g_builtNext = 0;
const Shape& ShapeOf(const Spec& spec) {
    for (const Built& b : g_built)
        if (b.have && Same(b, spec))
            return b.shape;
    Built& b = g_built[g_builtNext];
    g_builtNext = (g_builtNext + 1) % static_cast<int>(std::size(g_built));
    b.have = true, b.spec = spec;
    for (int i = 0; i < HullPoints(spec); ++i)
        std::memcpy(b.hull[i], spec.hull[i], sizeof(b.hull[i]));
    Build(spec, b.shape);
    return b.shape;
}

// ---------------------------------------------------------------- the hands' and the gun's last frames, in the room
constexpr int kHistory = 6;
struct History {
    V3 at[kHistory];
    M3 turn[kHistory];
    float dt[kHistory] = {};
    int next = 0, count = 0;
    // A frame's pose (rows: rotation | translation, in the room). One that jumps from the last is another (a gun switch, tracking found again):
    // the frames before it are forgotten.
    void Add(const float* rows, float step) {
        const V3 place = PlaceOf(rows);
        const M3 r = Orthonormal(RotationOf(rows));
        if (count && (Len(place - At(0)) > kJumpPlace || Len(TurnBetween(Turn(0), r)) > kJumpTurn))
            count = 0;
        at[next] = place, turn[next] = r, dt[next] = step;
        next = (next + 1) % kHistory, count = std::min(count + 1, kHistory);
    }
    void Forget() { count = 0; }
    int Index(int back) const { return (next - 1 - back + 2 * kHistory) % kHistory; }  // 0 the newest
    const V3& At(int back) const { return at[Index(back)]; }
    const M3& Turn(int back) const { return turn[Index(back)]; }
    float Dt(int back) const { return dt[Index(back)]; }
    // Its speed now (m/s), each of its axes the middle of its last three steps': a steady throw as handmag's LetGoSpeed reads it (the mean of the
    // three), but a frame's tracking jump left out (the study's model: a 10 cm jump on the frame it's let go of read 8.3 m/s for 6; this, 5.6).
    V3 Speed() const {
        if (count < 4)
            return {};
        V3 steps[3];
        for (int k = 0; k < 3; ++k)
            steps[k] = Dt(k) > 0.0f ? (At(k) - At(k + 1)) * (1.0f / Dt(k)) : V3{};
        return Median(steps);
    }
    // Its turn now (rad/s, the room's axes), alike.
    V3 Turning() const {
        if (count < 4)
            return {};
        V3 steps[3];
        for (int k = 0; k < 3; ++k)
            steps[k] = Dt(k) > 0.0f ? TurnBetween(Turn(k + 1), Turn(k)) * (1.0f / Dt(k)) : V3{};
        return Median(steps);
    }
};
History g_hand, g_gun;
History g_by[2];  // each hand's (Frame::hands: [0] the left, as g_hand; [1] the right), for ReleaseBy

// The gun's frame at a substep: between last frame's and this one's (blend 0 last frame's, 1 this one's).
void GunBetween(float blend, M3& turn, V3& at) {
    turn = g_gun.Turn(0), at = g_gun.At(0);
    if (g_gun.count < 2 || blend >= 1.0f)
        return;
    const M3& before = g_gun.Turn(1);
    at = g_gun.At(1) + (at - g_gun.At(1)) * blend;
    turn = ToM3(Turned(FromM3(before), TurnBetween(before, g_gun.Turn(0)), blend));
}

// ---------------------------------------------------------------- the things
enum class Phase { kNone, kWell, kFree, kRest };
struct Thing {
    Phase phase = Phase::kNone;
    int id = 0, kind = 0;
    bool fresh = false;        // started since the last Step: drawn where it started
    bool small = false;        // a casing
    bool keep = false;         // a gun: lies until it's taken
    Shape shape;
    int anchor[3] = {};        // the arms' space's origin when it started: all below is in that space (but what's the room's)
    float mass = 0.0f;
    M3 inertiaBody, inverseBody;
    float bounce = 1.0f, airPerMass = 0.0f, seenFar = 15.0f;
    V3 p, v, L;                // its middle, its speed, its angular momentum
    Q q;                       // its turn: its vertices' axes into the arms'
    // In its well or barrel: its seat on the gun's frame bone (in the room), its middle there, its line out; how far out it is and how fast; how far
    // to go; the walls' grip still and sliding; pushed (a magazine: never slower than kMinPush) or not (a case: never back in past its seat).
    M3 seatTurn;
    V3 seatAt, seatMiddle, axisOut;
    float along = 0.0f, alongSpeed = 0.0f, well = 0.0f, gripStill = 0.0f, gripMoving = 0.0f;
    bool pushed = false;
    V3 roomP, roomV;           // its middle in the room while in the well, and its speed there
    M3 gunTurnNow;             // the gun's turn at its last substep in the well
    // The ground it knows, in its own space.
    Plane ground[kPlanes];
    int grounds = 0;
    float asked = 0.0f;        // s since the ground was last asked, at rest
    // Lying.
    float quiet = 0.0f, rested = 0.0f, unseen = 0.0f, clock = 0.0f, freeFor = 0.0f, noGround = 0.0f;
    float lastSound = -10.0f, lastGainDb = -100.0f;
    bool heard = false;
    uint64_t touching = 0;     // its contact points (and round ends) touching in the last substep
    int props = 0;             // a gun stopped propped upright on a narrow base: the times it was tipped (kPropMost)
};
Thing g_big[kMost], g_small[kCasings];
int g_nextId = 1;
Result g_result;
bool g_resultGiven = false;  // the caller has it: what's told next starts it afresh

M3 InertiaWorld(const Thing& t, const M3& r) { return r * t.inertiaBody * Transposed(r); }
M3 InverseWorld(const Thing& t, const M3& r) { return r * t.inverseBody * Transposed(r); }

// A gun stopped with its top up on a base under kPropWide across (its touching hull points, across its own x): tipped over, about its length.
bool TipPropped(Thing& t) {
    if (!t.keep || t.shape.discs > 0 || t.shape.points < 1 || t.props >= kPropMost)
        return false;
    const M3 r = ToM3(t.q);
    const V3 top = r * V3{0.0f, 1.0f, 0.0f};
    if (top.y <= kPropUp)
        return false;
    float least = 1e9f, most = -1e9f;
    for (int i = 0; i < t.shape.points; ++i)
        if (t.touching & (uint64_t{1} << (i & 63)))
            least = std::min(least, t.shape.point[i].x), most = std::max(most, t.shape.point[i].x);
    if (most < least || most - least >= kPropWide)
        return false;
    const V3 side = r * V3{1.0f, 0.0f, 0.0f}, along = r * V3{0.0f, 0.0f, 1.0f};
    // (About its own z a positive turn lifts its +x side: the side already lower goes down. Level: by its number.)
    const float way = side.y < -1e-4f ? -1.0f : side.y > 1e-4f ? 1.0f : (t.id & 1) ? 1.0f : -1.0f;
    ++t.props;
    t.L = t.L + InertiaWorld(t, r) * (along * (way * kPropTurn * static_cast<float>(t.props)));
    return true;
}

void Afresh() {
    if (g_resultGiven) {
        g_result.poses = g_result.hits = g_result.gone = 0;
        g_resultGiven = false;
    }
}
void TellGone(int id) {
    Afresh();
    if (g_result.gone < kAll)
        g_result.goneId[g_result.gone++] = id;
}

float DbOf(float speed, float distance) {
    // The game's impact player (sound/maoism/material_interaction_impact__scrape_player): amplitude 0.5 to 2.5 over 0 to 6 (ScaleClamp), here 0 dB at 2.5;
    // and its cone output's fall-off: 0 dB at 0.667 m, -10.3 at 4.39, -34.9 at 19, -50.6 at 28.2, -77.4 at 36.6 (a spline there, straight lines here).
    const float a = (0.5f + 2.0f * std::min(speed / 6.0f, 1.0f)) / 2.5f;
    constexpr float kD[5] = {0.667f, 4.3863f, 19.0f, 28.2f, 36.6f}, kG[5] = {0.0f, -10.3054f, -34.88f, -50.56f, -77.44f};
    float fall = 0.0f;
    if (distance >= kD[4])
        fall = kG[4];
    else
        for (int i = 0; i < 4; ++i)
            if (distance >= kD[i] && distance < kD[i + 1])
                fall = kG[i] + (kG[i + 1] - kG[i]) * (distance - kD[i]) / (kD[i + 1] - kD[i]);
    return 20.0f * std::log10(a) + fall;
}

Thing* Slot(bool small) {
    Thing* pool = small ? g_small : g_big;
    const int most = small ? g_mostSmall : g_mostBig;
    for (int i = 0; i < most; ++i)
        if (pool[i].phase == Phase::kNone)
            return &pool[i];
    // Full: the one at rest longest that nobody sees goes; all seen, the one at rest longest (the drawing can't take more: the one way one goes in view).
    // Never a gun (keep): with only those and moving ones there's no room (the caller keeps the gun in the hand).
    Thing* oldest = nullptr;
    for (int pass = 0; pass < 2 && !oldest; ++pass)
        for (int i = 0; i < most; ++i) {
            Thing& t = pool[i];
            if (t.phase == Phase::kRest && !t.keep && (pass == 1 || t.unseen > 0.0f) && (!oldest || t.rested > oldest->rested))
                oldest = &t;
        }
    if (!oldest)
        return nullptr;
    TellGone(oldest->id);
    oldest->phase = Phase::kNone;
    return oldest;
}

void Begin(Thing& t, const Spec& spec, int kind, int rounds, const Frame& f) {
    t = Thing{};
    t.id = g_nextId++;
    if (g_nextId <= 0)
        g_nextId = 1;
    t.kind = kind;
    t.fresh = true;
    t.small = spec.casing;
    t.keep = spec.keep;
    t.shape = ShapeOf(spec);
    std::memcpy(t.anchor, f.origin, sizeof(t.anchor));
    t.mass = std::max(spec.massEmpty + static_cast<float>(std::clamp(rounds, 0, std::max(spec.capacity, 0))) * spec.massRound, 0.001f);
    for (int i = 0; i < 9; ++i)
        t.inertiaBody.m[i] = t.shape.inertia.m[i] * t.mass;
    t.inverseBody = Inverse(t.inertiaBody);
    t.bounce = spec.bounce * (rounds > 0 ? 0.85f : 1.0f);
    t.airPerMass = kAir * t.shape.area / t.mass;
    t.seenFar = std::clamp(kSeenSizes * t.shape.size, kSeenNearest, kSeenFarthest);
}

V3 Shift(const Thing& t, const Frame& f) {  // this thing's space -> the arms' space now
    return {static_cast<float>(t.anchor[0] - f.origin[0]), static_cast<float>(t.anchor[1] - f.origin[1]), static_cast<float>(t.anchor[2] - f.origin[2])};
}
// A place in the room into this thing's own space (the arms' space at its anchor).
V3 RoomToThing(const Thing& t, const Frame& f, V3 room) { return RotationOf(f.roomToArms) * room + PlaceOf(f.roomToArms) - Shift(t, f); }

// Its place from its bone (its vertices' space into the arms' space): its middle and turn.
void PlaceFromBone(Thing& t, const float* bone) {
    const M3 r = RotationOf(bone);
    t.q = FromM3(r);
    t.p = ToM3(t.q) * t.shape.com + PlaceOf(bone);
}

// Seated on the gun: its seat on the gun's frame bone, its middle there, its line out; its place now.
void Seat(Thing& t, const float* onGun, const float* axisOut, const Frame& f) {
    t.seatTurn = Orthonormal(RotationOf(onGun));
    t.seatAt = PlaceOf(onGun);
    t.seatMiddle = t.seatTurn * t.shape.com + t.seatAt;
    t.axisOut = Unit(Load(axisOut));
    const M3 gunTurn = Orthonormal(RotationOf(f.gunRoom));
    t.roomP = gunTurn * t.seatMiddle + PlaceOf(f.gunRoom);
    t.roomV = {};
    t.gunTurnNow = gunTurn;
    t.p = RoomToThing(t, f, t.roomP);
    t.q = FromM3(RotationOf(f.roomToArms) * gunTurn * t.seatTurn);
}

// How the gun's own motion shoves on what's in its well, m/s^2 in the room: its seat's place over four frames, two by two (one frame's shake of the
// tracking, a millimetre, comes to under 1 m/s^2 this way; frame by frame it would be some 10, and would walk a case out of its barrel).
V3 Shove(const Thing& t) {
    if (g_gun.count < 5)
        return {};
    auto at = [&](int k) { return g_gun.Turn(k) * t.seatMiddle + g_gun.At(k); };
    const float nearer = g_gun.Dt(0) + g_gun.Dt(1), further = g_gun.Dt(2) + g_gun.Dt(3);
    if (nearer <= 0.0f || further <= 0.0f)
        return {};
    const V3 a = ((at(0) - at(2)) * (1.0f / nearer) - (at(2) - at(4)) * (1.0f / further)) * (2.0f / (nearer + further));
    const float l = Len(a);
    return l > kMostShove ? a * (kMostShove / l) : a;
}

// One substep along its line in the room, with the gun between last frame's place and this one's: the speed it has, gravity and the gun's shove
// along the line (pull: per kg), and the walls' grip by what presses it on them. Out past its mouth: true.
bool WellStep(Thing& t, const Frame& f, float h, float blend, V3 pull) {
    M3 turn;
    V3 at;
    GunBetween(blend, turn, at);
    const V3 a = turn * t.axisOut;
    const float down = Dot(pull, a);
    const float pressed = Len(pull - a * down);
    float speed = t.alongSpeed;
    if (speed != 0.0f || std::fabs(down) > t.gripStill * pressed) {
        const float way = speed > 0.0f ? 1.0f : speed < 0.0f ? -1.0f : down > 0.0f ? 1.0f : -1.0f;
        const float next = speed + (down - t.gripMoving * pressed * way) * h;
        speed = speed != 0.0f && next * speed < 0.0f ? 0.0f : next;  // the grip stops it; it never turns it round
    }
    if (t.pushed)
        speed = std::max(speed, kMinPush);
    float along = t.along + speed * h;
    if (along < 0.0f)
        along = 0.0f, speed = std::max(speed, 0.0f);  // back against its seat
    t.along = along, t.alongSpeed = speed;
    const V3 room = turn * t.seatMiddle + at + a * along;
    t.roomV = (room - t.roomP) * (1.0f / h);
    t.roomP = room;
    t.gunTurnNow = turn;
    t.p = RoomToThing(t, f, room);
    t.q = FromM3(RotationOf(f.roomToArms) * turn * t.seatTurn);
    return along >= t.well;
}

// Out of the well: free, at its speed in the room (the gun's there and its own along the line) and the walking, turning as the gun turns, and
// tipped about the mouth's lower lip as its last leaves.
void LeaveWell(Thing& t, const Frame& f) {
    const M3 toArms = RotationOf(f.roomToArms);
    t.v = toArms * t.roomV + Load(f.walk);
    const M3 r = ToM3(t.q);
    t.L = InertiaWorld(t, r) * (toArms * g_gun.Turning());
    const V3 a = toArms * (t.gunTurnNow * t.axisOut);
    const V3 down{0.0f, -1.0f, 0.0f};
    V3 k = Cross(a, down);
    const float sinTilt = Len(k);
    if (sinTilt > 1e-3f) {
        k = k * (1.0f / sinTilt);
        const V3 side = Unit(down - a * Dot(down, a));
        const float behind = Support(t.shape, TMul(r, a * -1.0f)), across = Support(t.shape, TMul(r, side));
        const V3 lip = t.p - a * behind + side * across;
        const V3 arm = t.p - lip;
        const V3 armAcross = arm - k * Dot(arm, k);
        const float pivot = Dot(k, InertiaWorld(t, r) * k) + t.mass * Dot(armAcross, armAcross);
        const float torque = Dot(Cross(arm, V3{0.0f, -kGravity * t.mass, 0.0f}), k);
        const float leaving = kTipEngaged / std::max(t.alongSpeed, kMinPush);
        const float tip = pivot > 0.0f ? std::clamp(torque / pivot * leaving, 0.0f, kTipMost) : 0.0f;
        t.L = t.L + InertiaWorld(t, r) * (k * tip);
        t.v = t.v + Cross(k * tip, arm) * 0.5f;
    }
    t.phase = Phase::kFree;
}

// Out of the well with no gun to slide out of (it went, or another came): it goes on as it was going.
void LoseGun(Thing& t, const Frame& f) {
    t.v = RotationOf(f.roomToArms) * t.roomV + Load(f.walk);
    t.L = {};
    t.phase = Phase::kFree;
}

struct Contact {
    V3 r, n, tangent, discAxis;
    float depth, approach, bounce, friction, rough, normal;
    int plane;
    bool disc;
};
constexpr int kContacts = 48;

// One substep free: gravity, the air, its turn, its contacts with the ground it knows. Back: the fastest new contact into the ground, and its surface.
float FreeStep(Thing& t, float h, int iterations, int& surface, int& contacts) {
    t.v.y -= kGravity * h;
    const float speed = Len(t.v);
    if (speed > 0.0f)
        t.v = t.v * std::max(0.0f, 1.0f - t.airPerMass * speed * h);
    // Turned by its angular velocity half a step on (its momentum kept, its turn's speed follows how it's turned): a spin about its biggest axis
    // stays one. The turn of the step's start alone walked a magazine thrown end over end into tumbling in 4 s (11 flips: loosetest).
    M3 r = ToM3(t.q);
    M3 inv = InverseWorld(t, r);
    t.p = t.p + t.v * h;
    const Q half = Turned(t.q, inv * t.L, 0.5f * h);
    t.q = Turned(t.q, InverseWorld(t, ToM3(half)) * t.L, h);
    r = ToM3(t.q);
    inv = InverseWorld(t, r);
    V3 w = inv * t.L;
    Contact c[kContacts];
    int n = 0;
    float fastest = 0.0f;
    uint64_t touching = 0;
    for (int pl = 0; pl < t.grounds; ++pl) {
        const Plane& plane = t.ground[pl];
        const V3 normal = Load(plane.normal), on = Load(plane.point);
        const Feel& feel = kFeel[std::clamp(plane.surface, 0, kSurfaces - 1)];
        auto add = [&](V3 world, int key, bool disc, V3 axis) {
            const float depth = Dot(on - world, normal);
            if (depth < -kSkin || n >= kContacts)
                return;
            Contact& k = c[n++];
            k.r = world - t.p, k.n = normal, k.depth = depth, k.plane = pl;
            k.approach = -Dot(t.v + Cross(w, k.r), normal);
            k.bounce = std::min(0.5f, feel.bounce * t.bounce * g_bounceScale), k.friction = feel.friction, k.rough = feel.rough;
            k.disc = disc, k.discAxis = axis, k.normal = 0.0f, k.tangent = {};
            const uint64_t bit = uint64_t{1} << (key & 63);
            if (!(t.touching & bit) && k.approach > fastest)
                fastest = k.approach, surface = plane.surface;
            touching |= bit;
        };
        for (int i = 0; i < t.shape.points; ++i)
            add(t.p + r * t.shape.point[i], i, false, {});
        for (int i = 0; i < t.shape.discs; ++i) {
            const V3 centre = t.p + r * t.shape.discCentre[i], axis = r * t.shape.discAxis[i];
            const V3 across = normal - axis * Dot(normal, axis);
            const float l = Len(across);
            const int key = kPoints + 3 * i;
            if (l > 1e-4f) {
                add(centre - across * (t.shape.discRadius[i] / l), key, true, axis);
            } else {  // flat on the plane: three points of its rim
                const V3 u = Unit(Cross(axis, std::fabs(axis.x) < 0.9f ? V3{1, 0, 0} : V3{0, 1, 0})), v2 = Cross(axis, u);
                for (int j = 0; j < 3; ++j) {
                    const float a = 2.0944f * static_cast<float>(j);
                    add(centre + (u * std::cos(a) + v2 * std::sin(a)) * t.shape.discRadius[i], key + j, true, axis);
                }
            }
        }
    }
    t.touching = touching;
    contacts = n;
    for (int it = 0; it < iterations; ++it)
        for (int i = 0; i < n; ++i) {
            Contact& k = c[i];
            w = inv * t.L;
            V3 u = t.v + Cross(w, k.r);
            const float target = k.approach > kBounceFrom ? k.bounce * k.approach : 0.0f;
            const float effective = 1.0f / t.mass + Dot(k.n, Cross(inv * Cross(k.r, k.n), k.r));
            float push = (target - Dot(u, k.n)) / effective;
            const float total = std::max(k.normal + push, 0.0f);
            push = total - k.normal, k.normal = total;
            const V3 J = k.n * push;
            t.v = t.v + J * (1.0f / t.mass);
            t.L = t.L + Cross(k.r, J);
            // Friction, within its cone.
            w = inv * t.L;
            u = t.v + Cross(w, k.r);
            const V3 slide = u - k.n * Dot(u, k.n);
            const float s = Len(slide);
            if (s < 1e-9f)
                continue;
            const V3 dir = slide * (1.0f / s);
            const float effT = 1.0f / t.mass + Dot(dir, Cross(inv * Cross(k.r, dir), k.r));
            V3 want = k.tangent + dir * (-s / effT);
            const float most = k.friction * k.normal, l = Len(want);
            if (l > most && l > 0.0f)
                want = want * (most / l);
            const V3 Jt = want - k.tangent;
            k.tangent = want;
            t.v = t.v + Jt * (1.0f / t.mass);
            t.L = t.L + Cross(k.r, Jt);
        }
    // A round end rolling: dry rolling resistance, its load times the ground's bumps (c_rr, the bumps over its radius, times its load and radius).
    for (int i = 0; i < n; ++i) {
        const Contact& k = c[i];
        if (!k.disc || k.normal <= 0.0f)
            continue;
        w = inv * t.L;
        const float roll = Dot(w, k.discAxis);
        const float own = Dot(k.discAxis, InertiaWorld(t, r) * (k.discAxis * roll));
        const float take = std::min(std::fabs(own), k.rough * k.normal);
        t.L = t.L - k.discAxis * std::copysign(take, roll);
    }
    // Out of each plane by its deepest point (it never goes into the ground).
    for (int pl = 0; pl < t.grounds; ++pl) {
        float deepest = 0.0f;
        for (int i = 0; i < n; ++i)
            if (c[i].plane == pl)
                deepest = std::max(deepest, c[i].depth);
        if (deepest > kSlop)
            t.p = t.p + Load(t.ground[pl].normal) * (deepest - kSlop);
    }
    return fastest;
}

// How far its lowest point is into a plane (its own space): kSlop when it lies on it, below 0 clear of it.
float Deepest(const Thing& t, const Plane& plane) {
    const M3 r = ToM3(t.q);
    const V3 normal = Load(plane.normal), on = Load(plane.point);
    return Dot(on - t.p, normal) + Support(t.shape, TMul(r, normal * -1.0f));
}

// The ground near it this frame, in its own space: kept while none's known (0), forgotten when there's none (-1). One lying moves only for a firm
// ground that has it in it, or under it no longer.
void AskGround(Thing& t, const Frame& f) {
    if (!f.floor)
        return;
    const V3 shift = Shift(t, f);
    float at[3];
    Store(t.p + shift, at);
    Plane got[kPlanes] = {};
    const int n = f.floor(f.floorUser, at, got, kPlanes);
    if (n < 0) {
        t.grounds = 0;
        if (t.phase == Phase::kRest)
            t.phase = Phase::kFree, t.quiet = 0.0f;
        return;
    }
    Plane mine[kPlanes] = {};
    int count = 0;
    bool firm = false;
    for (int i = 0; i < std::min(n, kPlanes); ++i) {
        const V3 normal = Load(got[i].normal);
        const float l = Len(normal);
        if (!Finite(got[i].point, 3) || !Finite(normal) || l < 0.5f)
            continue;  // broken: left out
        Plane& m = mine[count++];
        m = got[i];
        Store(normal * (1.0f / l), m.normal);
        Store(Load(got[i].point) - shift, m.point);
        firm |= m.firm;
    }
    if (count == 0)
        return;
    if (t.phase == Phase::kRest) {
        if (!firm)
            return;  // a guess never moves one lying
        bool sunk = false, held = false;
        for (int i = 0; i < count; ++i)
            if (mine[i].firm) {
                const float deepest = Deepest(t, mine[i]);
                sunk |= deepest > kSlop + kWake;
                held |= deepest >= kSlop - kWake;
            }
        if (sunk || !held)
            t.phase = Phase::kFree, t.quiet = 0.0f;
    }
    std::memcpy(t.ground, mine, sizeof(mine));
    t.grounds = count;
}

bool Seen(const Thing& t, const Frame& f) {
    const V3 toIt = t.p + Shift(t, f) - Load(f.eye);
    const float distance = Len(toIt);
    if (distance > t.seenFar)
        return false;
    return distance < 0.3f || Dot(toIt, Unit(Load(f.look))) > f.seenCos * distance;
}

void ToPose(const Thing& t, const Frame& f, Pose& out) {
    const M3 r = ToM3(t.q);
    const V3 place = t.p - r * t.shape.com + Shift(t, f);
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            out.bone[i * 4 + j] = r.m[i * 3 + j];
    out.bone[3] = place.x, out.bone[7] = place.y, out.bone[11] = place.z;
    out.id = t.id, out.kind = t.kind, out.resting = t.phase == Phase::kRest, out.inGun = t.phase == Phase::kWell;
}

void Hear(Result& r, const Hit& hit) {
    if (r.hits < kHits) {
        r.hit[r.hits++] = hit;
        return;
    }
    int quietest = 0;  // full: the quietest makes way
    for (int i = 1; i < kHits; ++i)
        if (r.hit[i].gainDb < r.hit[quietest].gainDb)
            quietest = i;
    if (hit.gainDb > r.hit[quietest].gainDb)
        r.hit[quietest] = hit;
}

struct Timing {
    float dt;
    int substeps;
    float h;
    V3 gravityRoom;
};

void StepThing(Thing& t, const Frame& f, const Timing& time, Result& r) {
    if (t.fresh) {  // started since the last Step: where it was given, and the ground it'll fall on
        t.fresh = false;
        if (t.phase == Phase::kWell && g_gun.count >= 1) {  // seated on the gun as this frame has it (a start made with last frame's Frame would trail the gun a frame)
            t.roomP = g_gun.Turn(0) * t.seatMiddle + g_gun.At(0);
            t.gunTurnNow = g_gun.Turn(0);
            t.p = RoomToThing(t, f, t.roomP);
            t.q = FromM3(RotationOf(f.roomToArms) * g_gun.Turn(0) * t.seatTurn);
        }
        AskGround(t, f);
        if (r.poses < kAll)
            ToPose(t, f, r.pose[r.poses++]);
        return;
    }
    const float dt = time.dt;
    t.clock += dt;
    if (t.phase != Phase::kRest || (t.asked += dt) >= kAskResting) {
        t.asked = 0.0f;
        AskGround(t, f);
    }
    float fastest = 0.0f;
    int surface = kDirt;
    const bool wasResting = t.phase == Phase::kRest;  // (its minute counts from the frame after it came to rest)
    const V3 pull = t.phase == Phase::kWell ? time.gravityRoom - Shove(t) : V3{};
    for (int s = 0; s < time.substeps && dt > 0.0f; ++s) {
        if (t.phase == Phase::kWell) {
            if (g_gun.count < 2)
                LoseGun(t, f);
            else if (WellStep(t, f, time.h, static_cast<float>(s + 1) / static_cast<float>(time.substeps), pull))
                LeaveWell(t, f);
            continue;
        }
        if (t.phase != Phase::kFree)
            break;
        int contacts = 0, hitOn = kDirt;
        const float hit = FreeStep(t, time.h, t.small ? kCasingIterations : kIterations, hitOn, contacts);
        if (hit > fastest)
            fastest = hit, surface = hitOn;
        const M3 inv = InverseWorld(t, ToM3(t.q));
        const bool still = contacts > 0 && Len(t.v) < kSleepSpeed && Len(inv * t.L) < kSleepTurn;
        t.quiet = still ? t.quiet + time.h : 0.0f;
        const bool steady = contacts >= 3 || (t.shape.discs > 0 && contacts >= 2);  // flat, or a round one on its side; else on an edge
        if (t.quiet >= (steady ? kSleepFor : kSleepForEdge) && TipPropped(t)) {
            t.quiet = 0.0f;  // (not at rest so: over it goes)
        } else if (t.quiet >= (steady ? kSleepFor : kSleepForEdge)) {
            t.phase = Phase::kRest;
            t.v = {}, t.L = {};
            t.quiet = 0.0f;
            t.asked = kAskResting * static_cast<float>(t.id % 16) / 16.0f;  // the resting ones' asks spread over frames
        }
    }
    if (!Finite(t.p) || !Finite(t.v) || !Finite(t.L) || !Finite(t.q)) {  // broken numbers: never drawn
        TellGone(t.id);
        t.phase = Phase::kNone;
        return;
    }
    const bool seen = Seen(t, f);
    t.unseen = seen ? 0.0f : t.unseen + dt;
    if (t.phase == Phase::kFree) {
        t.freeFor += dt;
        t.noGround = t.grounds ? 0.0f : t.noGround + dt;
        if (t.noGround >= kNoGroundFor || (!t.keep && t.freeFor >= kFreeFor && t.unseen >= kUnseenFor)) {
            TellGone(t.id);
            t.phase = Phase::kNone;
            return;
        }
    }
    // At rest it lies its time; then it goes once nobody has seen it for a moment. A gun lies until it's taken.
    if (t.phase == Phase::kRest) {
        t.freeFor = 0.0f;
        if (wasResting)
            t.rested += dt;
        if (!t.keep && t.rested >= g_lie && t.unseen >= kUnseenFor) {
            TellGone(t.id);
            t.phase = Phase::kNone;
            return;
        }
    }
    // Heard: kept apart, and within the gate only if as loud as the last (the slap flat after a corner's touch).
    if (fastest > kHeard) {
        const float distance = Len(t.p + Shift(t, f) - Load(f.eye));
        const float db = DbOf(fastest, distance);
        const float since = t.clock - t.lastSound;
        if (since >= kSoundApart && (since >= kSoundGate || db >= t.lastGainDb)) {
            Hit hit;
            hit.id = t.id, hit.kind = t.kind, hit.speed = fastest, hit.gainDb = db, hit.surface = surface, hit.first = !t.heard;
            Store(t.p + Shift(t, f), hit.at);
            Hear(r, hit);
            t.heard = true, t.lastSound = t.clock, t.lastGainDb = db;
        }
    }
    if (r.poses < kAll)
        ToPose(t, f, r.pose[r.poses++]);
}

// Let go of by a hand (Release's left, or either of Frame::hands, ReleaseBy): tracked, its grip in the room, its sensors' speed and turn when
// sensed, and its last frames (steps).
int ReleaseFrom(const Spec& spec, int kind, const float* bone, int rounds, const Frame& f, bool hand, const float* handRoom, bool handSensed,
                const float* handVelocity, const float* handSpin, const History& steps) {
    if (!bone || !Finite(bone, 12) || !Finite(f.roomToArms, 12) || !Finite(f.walk, 3))
        return 0;
    Thing* t = Slot(spec.casing);
    if (!t)
        return 0;
    Begin(*t, spec, kind, rounds, f);
    PlaceFromBone(*t, bone);
    const M3 toArms = RotationOf(f.roomToArms);
    V3 v{}, w{};
    const bool sensed = hand && handSensed && Finite(handVelocity, 3) && Finite(handSpin, 3) && Finite(handRoom, 12);
    if (sensed || (hand && steps.count >= 4)) {
        // The hand's grip point's speed and its turn, in the room; the middle of the magazine moves with both. As its controller's own sensors read them when they're
        // given (the steps between its places lag a throw and can't tell it from a tracking jump), else as its last steps' middle.
        const V3 middleRoom = Transposed(toArms) * (t->p - PlaceOf(f.roomToArms));
        w = sensed ? Load(handSpin) : steps.Turning();
        const float turn = Len(w);
        if (turn > kMostTurn)
            w = w * (kMostTurn / turn);
        // (The lever arm from the hand as given with the start: the caller may give this frame's hand, a step on from the steps' newest.)
        const V3 handAt = hand && Finite(handRoom, 12) ? PlaceOf(handRoom) : steps.At(0);
        v = sensed ? Load(handVelocity) + Cross(w, middleRoom - handAt) : steps.Speed() + Cross(w, middleRoom - handAt);
        const float s = Len(v);
        if (s > kMostSpeed)
            v = v * (kMostSpeed / s);
    }
    t->v = toArms * v + Load(f.walk);
    t->L = InertiaWorld(*t, ToM3(t->q)) * (toArms * w);
    t->phase = Phase::kFree;
    return t->id;
}

template <typename Each>
void ForEach(Each each) {
    for (Thing& t : g_big)
        each(t);
    for (Thing& t : g_small)
        each(t);
}

}  // namespace

const Spec* SpecFor(const char* name) {
    if (!name)
        return nullptr;
    for (const Spec& s : kSpecs)
        if (std::strcmp(s.name, name) == 0)
            return &s;
    return nullptr;
}

int Eject(const Spec& spec, int kind, const float* onGun, const float* axisOut, float well, float push, int rounds, const Frame& f) {
    if (!f.gun || !onGun || !axisOut || !Finite(onGun, 12) || !Finite(axisOut, 3) || !Finite(f.gunRoom, 12) || !Finite(f.roomToArms, 12) ||
        Len(Load(axisOut)) < 0.5f)
        return 0;
    Thing* t = Slot(spec.casing);
    if (!t)
        return 0;
    Begin(*t, spec, kind, rounds, f);
    Seat(*t, onGun, axisOut, f);
    t->well = std::max(well, 0.0f);
    t->alongSpeed = push > 0.0f ? push : g_push;
    t->pushed = true;
    t->gripStill = t->gripMoving = g_wellFriction;
    t->phase = Phase::kWell;
    return t->id;
}

int TipOut(const Spec& spec, int kind, const float* onGun, const float* axisOut, float length, float grip, const Frame& f) {
    if (!f.gun || !onGun || !axisOut || !Finite(onGun, 12) || !Finite(axisOut, 3) || !Finite(f.gunRoom, 12) || !Finite(f.roomToArms, 12) ||
        Len(Load(axisOut)) < 0.5f || !std::isfinite(grip))
        return 0;
    Thing* t = Slot(spec.casing);
    if (!t)
        return 0;
    Begin(*t, spec, kind, 0, f);
    Seat(*t, onGun, axisOut, f);
    t->well = std::max(length, 0.0f);
    t->alongSpeed = 0.0f;
    t->pushed = false;
    t->gripStill = std::max(grip, 0.0f), t->gripMoving = kGripMoving * t->gripStill;
    t->phase = Phase::kWell;
    return t->id;
}

int Release(const Spec& spec, int kind, const float* bone, int rounds, const Frame& f) {
    return ReleaseFrom(spec, kind, bone, rounds, f, f.hand, f.handRoom, f.handSensed, f.handVelocity, f.handSpin, g_hand);
}

int ReleaseBy(const Spec& spec, int kind, const float* bone, int rounds, const Frame& f, int side) {
    if (side != 0 && side != 1)
        return 0;
    const HandAt& h = f.hands[side];
    return ReleaseFrom(spec, kind, bone, rounds, f, h.tracked, h.room, h.sensed, h.velocity, h.spin, g_by[side]);
}

int Flick(const Spec& spec, int kind, const float* bone, int rounds, const float* velocity, const float* axis, float spin, const Frame& f) {
    if (!bone || !velocity || !axis || !Finite(bone, 12) || !Finite(velocity, 3) || !Finite(axis, 3) || !std::isfinite(spin) ||
        !Finite(f.roomToArms, 12) || !Finite(f.walk, 3))
        return 0;
    Thing* t = Slot(spec.casing);
    if (!t)
        return 0;
    Begin(*t, spec, kind, rounds, f);
    PlaceFromBone(*t, bone);
    const M3 toArms = RotationOf(f.roomToArms);
    V3 gunV{}, gunW{};
    if (f.gun && g_gun.count >= 4) {
        const V3 middleRoom = Transposed(toArms) * (t->p - PlaceOf(f.roomToArms));
        gunW = g_gun.Turning();
        gunV = g_gun.Speed() + Cross(gunW, middleRoom - g_gun.At(0));
    }
    t->v = toArms * gunV + Load(f.walk) + Load(velocity);
    t->L = InertiaWorld(*t, ToM3(t->q)) * (toArms * gunW + Unit(Load(axis)) * spin);
    t->phase = Phase::kFree;
    return t->id;
}

const Result& Step(const Frame& f) {
    Afresh();
    Result& r = g_result;
    r.poses = r.hits = 0;
    const bool placed = Finite(f.roomToArms, 12);
    Timing time;
    time.dt = std::isfinite(f.dt) ? std::clamp(f.dt, 0.0f, 0.1f) : 0.0f;
    if (f.gun && placed && Finite(f.gunRoom, 12))
        g_gun.Add(f.gunRoom, time.dt);
    else
        g_gun.Forget();
    if (f.hand && placed && Finite(f.handRoom, 12))
        g_hand.Add(f.handRoom, time.dt);
    else
        g_hand.Forget();
    for (int s = 0; s < 2; ++s)
        if (f.hands[s].tracked && placed && Finite(f.hands[s].room, 12))
            g_by[s].Add(f.hands[s].room, time.dt);
        else
            g_by[s].Forget();
    time.substeps = std::clamp(static_cast<int>(std::ceil(time.dt / kSubstep)), 1, kMostSubsteps);
    time.h = time.dt / static_cast<float>(time.substeps);
    time.gravityRoom = placed ? TMul(RotationOf(f.roomToArms), V3{0.0f, -kGravity, 0.0f}) : V3{0.0f, -kGravity, 0.0f};
    auto fewer = [](Thing* pool, int all, int most) {  // fewer allowed now (SetMost); a gun stays where it is
        for (int i = most; i < all; ++i)
            if (pool[i].phase != Phase::kNone && !pool[i].keep) {
                TellGone(pool[i].id);
                pool[i].phase = Phase::kNone;
            }
    };
    fewer(g_big, kMost, g_mostBig);
    fewer(g_small, kCasings, g_mostSmall);
    ForEach([&](Thing& t) {
        if (t.phase == Phase::kNone)
            return;
        if (placed)
            StepThing(t, f, time, r);
        else if (r.poses < kAll)  // no room this frame: where it was
            ToPose(t, f, r.pose[r.poses++]);
    });
    g_resultGiven = true;
    return r;
}

bool Remove(int id) {
    bool found = false;
    ForEach([&](Thing& t) {
        if (t.phase != Phase::kNone && t.id == id)
            t.phase = Phase::kNone, found = true;
    });
    return found;
}

bool Has(int id) {
    bool found = false;
    ForEach([&](const Thing& t) { found |= t.phase != Phase::kNone && t.id == id; });
    return found;
}

void Clear() {
    ForEach([](Thing& t) { t.phase = Phase::kNone; });
    g_result.poses = g_result.hits = g_result.gone = 0;
    g_resultGiven = false;
    g_hand.Forget(), g_gun.Forget();
    g_by[0].Forget(), g_by[1].Forget();
}

int Count() {
    int n = 0;
    ForEach([&](const Thing& t) { n += t.phase != Phase::kNone; });
    return n;
}

int Resting() {
    int n = 0;
    ForEach([&](const Thing& t) { n += t.phase == Phase::kRest; });
    return n;
}

void SetPush(float metresPerSecond) { g_push = std::clamp(metresPerSecond, kMinPush, 3.0f); }
void SetWellFriction(float mu) { g_wellFriction = std::clamp(mu, 0.0f, 1.0f); }
void SetLie(float seconds) { g_lie = std::clamp(seconds, 1.0f, 600.0f); }
void SetBounce(float scale) { g_bounceScale = std::clamp(scale, 0.0f, 2.0f); }
void SetMost(int big, int casings) {
    g_mostBig = std::clamp(big, 1, kMost);
    g_mostSmall = std::clamp(casings, 1, kCasings);
}

}  // namespace loose

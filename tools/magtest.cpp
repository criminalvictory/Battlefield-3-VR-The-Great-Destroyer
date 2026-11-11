// The magazine taken at the belly and pushed into the M416's well by the left hand
// (src/pouch.cpp, as hands.cpp runs it), on scripted frames at 72 Hz, without the game: the squeeze
// that takes one (the gun's state, the hand, the handguard), the zone and the body's heading under
// it, the funnel below the well's mouth (what's caught and what isn't), the ride up the line and the
// seat, tracking jumps, letting go, the hand lost, pulled off the line.
// The room is OpenXR LOCAL (y up, metres): the head level at (0 1.6 0) faces -z, and turns about the
// neck point the pouch goes by (7.5 cm below and 8 cm behind the eyes), as a head does. The view is
// the head's own space (x right, y up, z back); the gun is held still in it, turned (170 degrees
// about the view's up, the muzzle 8 degrees up, canted 12) so the module's matrices are tried off
// their axes. The gun's model space is x left, y up, z to the muzzle.
// The left hand is the magazine it holds: the glove holding it there comes from pouch::GloveFor,
// and the grip point (the fist's middle, what the zone goes by) is the hand's middle in the judge's
// fit, (0.045 -0.140 0.425) in the gun's space with the magazine seated, put into the room.
// The magazine's top middle in the gun's space gives s, its place along the well's line from home
// (0 seated, -8.7 cm at the mouth), h = -0.087 - s its depth below the mouth, lateral its distance
// from the line; angle is its whole turn from seated. "Drawn" is Result::magPose, the magazine as
// the mod draws it; its move is the most its top or its bottom's corners move in a frame.
// The module's state is its own (no reset): each check starts it afresh (Clear, the squeeze open,
// the head turned half round and back so the body's heading snaps to it).
// 1-9 are the checks asked for; 10-13 more of the design (the judge's synthesis, sections 4-8 and
// 11), counted too; those marked info are shown, not counted.
// Either hand (free hands, 2026-11-06): 16 the seated magazine taken by the left hand and drawn down (the old
// paths, with only the old inputs), then m1-m10: m1 runs 1-16 again as their mirror image (the right hand,
// Frame::side 1: the head, the gun, the glove and the grip point mirrored across x = 0) and wants the same events
// in the same frames, every pose the left's mirrored, every check passing (counted apart, not printed); m2-m10,
// each hand on the gun as it is (the right holding the magazine on its other face), the right's own numbers, a
// change of hand, a hand that can't keep its magazine (Frame::dropHeld), HoldingSide, GloveFor and OnSeated.
// Usage: magtest [-v ID] [-trace FILE]   (-v prints that check frame by frame, e.g. -v 4a; -trace writes every
// frame's result, to compare two builds of pouch.cpp: the left's paths come first, the mirrored run's lines
// are marked 1)
// The exit code is the number of failed checks.

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

#include "../src/pouch.h"

namespace {

constexpr float kPi = 3.14159265f;
constexpr float kDegree = kPi / 180.0f;
constexpr float kDt = 1.0f / 72.0f;  // a frame at 72 Hz (13.9 ms)

// pouch.cpp's numbers (the design's): the seated magazine's top middle in the gun's space, s at the
// well's mouth, the funnel's depth below it, home and the most it moves a frame on the line.
constexpr float kTop[3] = {0.0f, 0.0413f, 0.415f};
constexpr float kMouth = -0.087f;
constexpr float kFunnelDepth = 0.08f;
constexpr float kHome = 0.005f;
constexpr float kMaxStep = 0.03f;
// Where a magazine is taken (ammozone.h: at the belly since 2026-11-05, over the left shoulder before):
// an egg, its middle from the neck and its half sizes (right, up, forward from the neck), 1 its edge
// each way by its own size; the hand goes to its middle to take one. The neck from the eyes in the
// head's axes; the neck with the eyes at (0 1.6 0), level.
constexpr float kBelly[3] = {0.0f, -0.43f, 0.13f};
constexpr float kBellyRadii[3] = {0.18f, 0.16f, 0.12f};
constexpr float kNeck[3] = {0.0f, -0.075f, 0.08f};
constexpr float kNeck0[3] = {0.0f, 1.525f, 0.08f};
// The hand's middle in the magazine's space (the gun's, with it seated; the judge's fit).
constexpr float kHandMiddle[3] = {0.045f, -0.140f, 0.425f};
// ... by side: the left's, or the right's (on the magazine's other face: x negated).
const float* HandMiddle(int side) {
    static constexpr float kRight[3] = {-kHandMiddle[0], kHandMiddle[1], kHandMiddle[2]};
    return side == 1 ? kRight : kHandMiddle;
}
// The magazine's top middle and its bottom's front and back corners: its moves are theirs.
constexpr float kMagPoints[3][3] = {{0.0f, 0.0413f, 0.415f}, {0.0f, -0.173f, 0.474f}, {0.0f, -0.173f, 0.381f}};
constexpr float kEps = 1e-5f;

std::string g_verbose;  // -v: the check printed frame by frame
// m1: the module fed every frame as its mirror image (the right hand), its results put back into the left's world.
bool g_mirror = false;
std::FILE* g_trace = nullptr;  // -trace: every frame's result written out
// Every frame's result and what the module was given, by run (0 as it is, 1 mirrored), for m1.
std::vector<pouch::Result> g_results[2];
std::vector<int> g_fedSides[2];

float Clamp01(float v) {
    return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v;
}
float Ease(float u) {  // 0 to 1, eased in and out
    u = Clamp01(u);
    return u * u * (3.0f - 2.0f * u);
}
int Frames(float seconds) {
    return static_cast<int>(std::lround(seconds / kDt));
}
float Distance(const float* a, const float* b) {
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}
float Wrap(float degrees) {
    return std::remainder(degrees, 360.0f);
}

// ---- Rigid poses (3x4, rows: rotation | translation) ----

struct Pose {
    float m[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
};

Pose FromArray(const float* a) {
    Pose p;
    std::memcpy(p.m, a, sizeof(p.m));
    return p;
}

Pose Mul(const Pose& a, const Pose& b) {
    Pose out;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j)
            out.m[i * 4 + j] = a.m[i * 4] * b.m[j] + a.m[i * 4 + 1] * b.m[4 + j] + a.m[i * 4 + 2] * b.m[8 + j];
        out.m[i * 4 + 3] += a.m[i * 4 + 3];
    }
    return out;
}

Pose Inverse(const Pose& a) {
    Pose out;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            out.m[i * 4 + j] = a.m[j * 4 + i];
    for (int i = 0; i < 3; ++i)
        out.m[i * 4 + 3] = -(out.m[i * 4] * a.m[3] + out.m[i * 4 + 1] * a.m[7] + out.m[i * 4 + 2] * a.m[11]);
    return out;
}

void Apply(const Pose& a, const float* p, float* out) {
    for (int i = 0; i < 3; ++i)
        out[i] = a.m[i * 4] * p[0] + a.m[i * 4 + 1] * p[1] + a.m[i * 4 + 2] * p[2] + a.m[i * 4 + 3];
}

Pose Move(float x, float y, float z) {
    Pose p;
    p.m[3] = x, p.m[7] = y, p.m[11] = z;
    return p;
}

// A turn of degrees about the axis (x y z) through the origin (right-handed).
Pose Turn(float x, float y, float z, float degrees) {
    const float n = std::sqrt(x * x + y * y + z * z);
    x /= n, y /= n, z /= n;
    const float c = std::cos(degrees * kDegree), s = std::sin(degrees * kDegree), t = 1.0f - c;
    const float r[9] = {t * x * x + c,     t * x * y - s * z, t * x * z + s * y, t * x * y + s * z, t * y * y + c,
                        t * y * z - s * x, t * x * z - s * y, t * y * z + s * x, t * z * z + c};
    Pose p;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            p.m[i * 4 + j] = r[i * 3 + j];
    return p;
}

// ... about that axis through a point.
Pose TurnAbout(float x, float y, float z, float degrees, const float* point) {
    return Mul(Mul(Move(point[0], point[1], point[2]), Turn(x, y, z, degrees)), Move(-point[0], -point[1], -point[2]));
}

// How far a pose turns (degrees, 0-180).
float Angle(const Pose& a) {
    return std::acos(std::clamp((a.m[0] + a.m[5] + a.m[10] - 1.0f) * 0.5f, -1.0f, 1.0f)) / kDegree;
}

// The most any of the magazine's points is apart between two poses (metres).
float Apart(const Pose& a, const Pose& b) {
    float most = 0.0f;
    for (const float* p : kMagPoints) {
        float pa[3], pb[3];
        Apply(a, p, pa);
        Apply(b, p, pb);
        most = std::max(most, Distance(pa, pb));
    }
    return most;
}

struct Quat {
    float x = 0, y = 0, z = 0, w = 1;
};

Quat AxisQuat(float x, float y, float z, float degrees) {
    const float n = std::sqrt(x * x + y * y + z * z), h = degrees * kDegree * 0.5f, s = std::sin(h) / n;
    return {x * s, y * s, z * s, std::cos(h)};
}

Quat QMul(const Quat& a, const Quat& b) {
    return {a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y, a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w, a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}

void SetTurn(Pose& p, Quat q) {
    const float n = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    const float x = q.x / n, y = q.y / n, z = q.z / n, w = q.w / n;
    const float r[9] = {1 - 2 * (y * y + z * z), 2 * (x * y - z * w),     2 * (x * z + y * w),
                        2 * (x * y + z * w),     1 - 2 * (x * x + z * z), 2 * (y * z - x * w),
                        2 * (x * z - y * w),     2 * (y * z + x * w),     1 - 2 * (x * x + y * y)};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            p.m[i * 4 + j] = r[i * 3 + j];
}

Quat ToQuat(const Pose& p) {
    const float* m = p.m;
    const float trace = m[0] + m[5] + m[10];
    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        return {(m[9] - m[6]) / s, (m[2] - m[8]) / s, (m[4] - m[1]) / s, 0.25f * s};
    }
    if (m[0] > m[5] && m[0] > m[10]) {
        const float s = std::sqrt(1.0f + m[0] - m[5] - m[10]) * 2.0f;
        return {0.25f * s, (m[1] + m[4]) / s, (m[2] + m[8]) / s, (m[9] - m[6]) / s};
    }
    if (m[5] > m[10]) {
        const float s = std::sqrt(1.0f + m[5] - m[0] - m[10]) * 2.0f;
        return {(m[1] + m[4]) / s, 0.25f * s, (m[6] + m[9]) / s, (m[2] - m[8]) / s};
    }
    const float s = std::sqrt(1.0f + m[10] - m[0] - m[5]) * 2.0f;
    return {(m[2] + m[8]) / s, (m[6] + m[9]) / s, 0.25f * s, (m[4] - m[1]) / s};
}

Quat Slerp(const Quat& a, Quat b, float u) {
    float d = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    if (d < 0.0f)
        b = {-b.x, -b.y, -b.z, -b.w}, d = -d;
    float wa = 1.0f - u, wb = u;
    if (d < 0.9995f) {
        const float th = std::acos(d), s = std::sin(th);
        wa = std::sin((1.0f - u) * th) / s, wb = std::sin(u * th) / s;
    }
    return {a.x * wa + b.x * wb, a.y * wa + b.y * wb, a.z * wa + b.z * wb, a.w * wa + b.w * wb};
}

// The magazine's pose between two (u 0-1): turned along the way, the hand's middle on a line.
Pose Between(const Pose& a, const Pose& b, float u) {
    Pose p;
    SetTurn(p, Slerp(ToQuat(a), ToQuat(b), u));
    float ha[3], hb[3], turned[3];
    Apply(a, kHandMiddle, ha);
    Apply(b, kHandMiddle, hb);
    Apply(p, kHandMiddle, turned);  // its translation still 0
    for (int i = 0; i < 3; ++i)
        p.m[i * 4 + 3] = ha[i] + (hb[i] - ha[i]) * u - turned[i];
    return p;
}

// ---- The head, the neck and the pouch ----

// The head turned about the neck point the pouch goes by: yaw left, pitch up, roll (degrees).
XrPosef HeadAt(const float* neck, float yaw, float pitch, float roll = 0.0f) {
    const Quat q = QMul(QMul(AxisQuat(0, 1, 0, yaw), AxisQuat(1, 0, 0, pitch)), AxisQuat(0, 0, 1, roll));
    Pose turn;
    SetTurn(turn, q);
    float off[3];
    Apply(turn, kNeck, off);
    XrPosef p{};
    p.orientation = {q.x, q.y, q.z, q.w};
    p.position = {neck[0] - off[0], neck[1] - off[1], neck[2] - off[2]};
    return p;
}

Pose FromXr(const XrPosef& x) {
    Pose p;
    SetTurn(p, {x.orientation.x, x.orientation.y, x.orientation.z, x.orientation.w});
    p.m[3] = x.position.x, p.m[7] = x.position.y, p.m[11] = x.position.z;
    return p;
}

// A point in the body's axes (right, up, forward from the neck; metres), into the room, the body's heading yaw (degrees, left of -z).
void PouchPoint(const float* neck, float yaw, float right, float up, float forward, float* out) {
    const float a = yaw * kDegree;
    const float r[3] = {std::cos(a), 0.0f, -std::sin(a)}, f[3] = {-std::sin(a), 0.0f, -std::cos(a)};
    for (int i = 0; i < 3; ++i)
        out[i] = neck[i] + right * r[i] + forward * f[i];
    out[1] += up;
}

// Where the hand goes to take one: the zone's middle at the belly, with the body's heading yaw.
void MiddleFor(const float* neck, float yaw, float* out) {
    PouchPoint(neck, yaw, kBelly[0], kBelly[1], kBelly[2], out);
}

// How far out of the zone a room point is (1 its edge, each way by its own size), the body's heading yaw.
float SizeAt(const float* neck, float yaw, const float* p) {
    const float a = yaw * kDegree;
    const float d[3] = {p[0] - neck[0], p[1] - neck[1], p[2] - neck[2]};
    const float body[3] = {d[0] * std::cos(a) - d[2] * std::sin(a), d[1], -d[0] * std::sin(a) - d[2] * std::cos(a)};
    float sum = 0.0f;
    for (int i = 0; i < 3; ++i) {
        const float u = (body[i] - kBelly[i]) / kBellyRadii[i];
        sum += u * u;
    }
    return std::sqrt(sum);
}

// The body's heading the module went by (degrees), from a squeeze's Result::fromMiddle (the grip from
// the neck, in the body's axes) with the grip at `grip`.
float HeadingFrom(const float* neck, const float* grip, const float* fromMiddle) {
    const float alpha = std::atan2(-(grip[0] - neck[0]), -(grip[2] - neck[2]));
    return Wrap((alpha + std::atan2(fromMiddle[0], fromMiddle[2])) / kDegree);
}

// ---- The magazine's place by the well ----

struct Place {
    float top[3];
    float s, h, lateral, angle;
};

Place PlaceOf(const Pose& inGun) {
    Place p;
    Apply(inGun, kTop, p.top);
    p.s = p.top[1] - kTop[1];
    p.h = kMouth - p.s;
    p.lateral = std::sqrt(p.top[0] * p.top[0] + (p.top[2] - kTop[2]) * (p.top[2] - kTop[2]));
    p.angle = Angle(inGun);
    return p;
}

// The gun as the right hand holds it still in the view (its model space into the view's).
Pose GunPose() {
    return Mul(Mul(Mul(Move(0.10f, -0.28f, -0.20f), Turn(0, 1, 0, 170.0f)), Turn(1, 0, 0, -8.0f)), Turn(0, 0, 1, 12.0f));
}

// The magazine upright in the room, its front forward (-z), its left to the left.
Pose Upright() {
    return Turn(0, 1, 0, 180.0f);
}

Pose g_gloveOfSeated;      // the glove holding a magazine at the gun's own pose (GloveFor: hold^-1)
float g_handInGlove[3] = {};  // the hand's middle in the gloves' bind space

// ---- Into the module: as it is, or as its mirror image ----

// A pose's mirror image across x = 0 (S m S, S = diag(-1, 1, 1): elements 1, 2, 3, 4 and 8 negated): the view's and the room's, the gun's
// about its middle plane, the gloves' bind spaces each other's.
void MirrorRows(float* m) {
    for (const int i : {1, 2, 3, 4, 8})
        m[i] = -m[i];
}
Pose Mirrored(const Pose& p) {
    Pose m = p;
    MirrorRows(m.m);
    return m;
}

// One frame's result written out (-trace): every field, the numbers exactly (hex).
void Trace(const pouch::Result& r) {
    if (!g_trace)
        return;
    static long s_line[2] = {};
    const int run = g_mirror ? 1 : 0;
    std::fprintf(g_trace, "%d %ld m%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d", run, s_line[run]++, r.mag, r.hideSeated, r.holding, r.atPouch,
                 r.reached, r.took, r.grabbed, r.toGrip, r.pulledOut, r.caught, r.offLine, r.seated, r.letGo, r.putBack, r.cancelled,
                 r.squeezed, pouch::Holding());
    for (const float v : r.magPose)
        std::fprintf(g_trace, " %a", v);
    for (const float v : r.glove)
        std::fprintf(g_trace, " %a", v);
    std::fprintf(g_trace, " | %a | %a %a %a | %a %a %a | %a | %s\n", r.gloveBlend, r.dropVelocity[0], r.dropVelocity[1], r.dropVelocity[2],
                 r.fromMiddle[0], r.fromMiddle[1], r.fromMiddle[2], r.size, r.note);
}

// pouch::Update, as it is; or with g_mirror (m1) the other hand's: the other side and every input mirrored across x = 0 (the head, the
// room's grip point, the view's gun and glove), the result put back into the left's world so every check reads it as before.
// fed: what the module was given.
pouch::Result Feed(const pouch::Frame& f, pouch::Frame* fed = nullptr) {
    pouch::Frame in = f;
    if (g_mirror) {
        in.side = 1 - in.side;
        MirrorRows(in.gunPose);
        MirrorRows(in.glove);
        in.leftGrip.x = -in.leftGrip.x;
        in.head.position.x = -in.head.position.x;
        in.head.orientation.y = -in.head.orientation.y;
        in.head.orientation.z = -in.head.orientation.z;
    }
    pouch::Result r = pouch::Update(in);
    if (fed)
        *fed = in;
    if (g_mirror) {
        MirrorRows(r.magPose);
        MirrorRows(r.glove);
        r.dropVelocity[0] = -r.dropVelocity[0];
        r.fromMiddle[0] = -r.fromMiddle[0];
    }
    Trace(r);
    const int run = g_mirror ? 1 : 0;
    g_results[run].push_back(r);
    g_fedSides[run].push_back(in.side);
    return r;
}

// ---- The scripted frames ----

struct Sim {
    std::string id;
    bool verbose = false;
    // This frame's inputs, but dt, the glove, the gun's pose and (unless given) the grip; f.side is the hand holding mag (its glove
    // from GloveFor of that side, its grip point its hand's middle on the magazine's face on that side).
    pouch::Frame f;
    Pose head;        // the room <- the view (f.head)
    Pose gun;         // the view <- the gun's model space
    Pose mag;         // the magazine as the hand holds it, in the view
    bool gripGiven = false;
    bool gloveGiven = false;  // f.glove as set (the seated magazine's paths), not the one holding mag
    bool zeroGun = false;
    pouch::Frame fed;  // what the module was given this frame (mirrored with g_mirror)
    int frame = 0;
    float t = 0.0f;
    pouch::Result r;
    bool line = false;  // on the well's line: caught, and not off it, seated, let go or gone since
    bool drawnBefore = false;
    float before[3][3] = {};
    float move = 0.0f;  // the drawn magazine's this frame (0 unless drawn this frame and the last)
    int took = 0, caught = 0, seated = 0, offLine = 0, letGo = 0, putBack = 0, cancelled = 0;

    void Head(const XrPosef& p) {
        f.head = p;
        head = FromXr(p);
    }
    void InGun(const Pose& e) { mag = Mul(gun, e); }
    // The hand's middle at p in the room, the magazine turned so in the room.
    void AtGrip(const float* p, const Pose& turn) {
        Pose room = turn;
        float o[3];
        Apply(turn, HandMiddle(f.side), o);
        room.m[3] = p[0] - o[0], room.m[7] = p[1] - o[1], room.m[11] = p[2] - o[2];
        mag = Mul(Inverse(head), room);
    }
    Pose HandInGun() const { return Mul(Inverse(gun), mag); }
    bool Drawn(Pose& e) const {
        if (!r.mag)
            return false;
        e = Mul(Inverse(gun), FromArray(r.magPose));
        return true;
    }

    const pouch::Result& Step() {
        f.dt = kDt;
        if (!gloveGiven)
            pouch::GloveFor(mag.m, f.glove, f.side);
        std::memcpy(f.gunPose, gun.m, sizeof(f.gunPose));
        if (zeroGun)
            std::memset(f.gunPose, 0, sizeof(f.gunPose));  // hands.cpp: t.gunPose is all zeros without t.gun
        if (!gripGiven) {
            float v[3], p[3];
            Apply(mag, HandMiddle(f.side), v);
            Apply(head, v, p);
            f.leftGrip = {p[0], p[1], p[2]};
        }
        r = Feed(f, &fed);
        ++frame;
        t += kDt;
        took += r.took, caught += r.caught, seated += r.seated, offLine += r.offLine;
        letGo += r.letGo, putBack += r.putBack, cancelled += r.cancelled;
        if (r.caught)
            line = true;
        if (r.offLine || r.seated || r.letGo || r.putBack || r.cancelled)
            line = false;
        move = 0.0f;
        Pose e;
        if (Drawn(e)) {
            float now[3][3];
            for (int k = 0; k < 3; ++k)
                Apply(e, kMagPoints[k], now[k]);
            if (drawnBefore)
                for (int k = 0; k < 3; ++k)
                    move = std::max(move, Distance(now[k], before[k]));
            std::memcpy(before, now, sizeof(now));
            drawnBefore = true;
        } else {
            drawnBefore = false;
        }
        if (verbose)
            Print();
        return r;
    }

    void Print() const {
        const Place p = PlaceOf(HandInGun());
        std::printf("%-4s %4d %6.3f sq %.2f %s | hand s %6.1f h %6.1f lat %5.1f ang %5.1f", id.c_str(), frame, t,
                    f.squeeze, f.left ? (f.side == 1 ? "R" : "L") : "-", p.s * 1000, p.h * 1000, p.lateral * 1000, p.angle);
        Pose e;
        if (Drawn(e)) {
            const Place d = PlaceOf(e);
            std::printf(" | drawn s %6.1f lat %5.1f ang %5.1f move %5.1f", d.s * 1000, d.lateral * 1000, d.angle,
                        move * 1000);
        } else {
            std::printf(" | not drawn                                    ");
        }
        std::printf(" | blend %.2f%s%s%s%s%s%s%s%s%s%s %s\n", r.gloveBlend, r.holding ? " holding" : "",
                    r.reached ? " REACHED" : "", r.took ? " TOOK" : "", r.caught ? " CAUGHT" : "",
                    r.offLine ? " OFFLINE" : "", r.seated ? " SEATED" : "", r.letGo ? " LETGO" : "",
                    r.putBack ? " PUTBACK" : "", r.cancelled ? " CANCELLED" : "", line ? " (line)" : "",
                    *r.note ? r.note : "");
    }
};

// The module afresh: nothing held, the squeeze open (armed, nothing crossed), the glove's blend gone,
// the gun's magazine counted in, the body's heading snapped to a level head at (0 1.6 0) (a turn of
// more than 45 degrees in a frame is a recentre). Then the script's defaults: the gun hot (its
// magazine out), the hand free, spare rounds, the palm far from the handguard. side: the hand fed
// (m1-m10; the mirrored run's marked ' when printed).
void Fresh(Sim& sim, const char* id, int side = 0) {
    pouch::Clear();
    pouch::SetZone(nullptr, true);  // on, as hands.cpp turns it (its place is ammozone.h's: no numbers)
    pouch::Frame f;
    f.dt = kDt;
    f.left = true;
    f.side = side;
    f.leftGrip = {3.0f, 0.0f, 3.0f};
    f.head = HeadAt(kNeck0, 180.0f, 0.0f);
    Feed(f);
    f.head = HeadAt(kNeck0, 0.0f, 0.0f);
    for (int i = 0; i < 4; ++i)
        Feed(f);
    sim = Sim{};
    sim.id = g_mirror ? std::string(id) + "'" : std::string(id);
    sim.verbose = !g_verbose.empty() && g_verbose == id;
    sim.gun = GunPose();
    sim.Head(HeadAt(kNeck0, 0.0f, 0.0f));
    sim.f.side = side;
    sim.f.left = true;
    sim.f.gun = true;
    sim.f.wellEmpty = true;
    sim.f.dry = false;
    sim.f.spare = true;
    sim.f.handFree = true;
    sim.f.guardDistance = 1.0f;
    sim.InGun(Move(0.0f, -0.20f, 0.0f));
}

void Still(Sim& sim, float seconds, const std::function<void()>& each = {}) {
    for (int i = 0, n = Frames(seconds); i < n; ++i) {
        sim.Step();
        if (each)
            each();
    }
}

// A magazine taken where the hand is, as pouch.txt "test" does (the pouch counted as reached): the
// squeeze open 3 frames, a fresh squeeze, 3 frames more held still.
bool TakeHere(Sim& sim) {
    sim.f.squeeze = 0.0f;
    sim.f.forceInside = true;
    for (int i = 0; i < 3; ++i)
        sim.Step();
    sim.f.squeeze = 1.0f;
    sim.Step();
    const bool took = sim.r.took;
    sim.f.forceInside = false;
    for (int i = 0; i < 3; ++i)
        sim.Step();
    return took && pouch::Holding();
}

// A magazine taken with the grip at `at` (a fresh squeeze there), held upright facing forward.
bool TakeAt(Sim& sim, const float* at) {
    sim.AtGrip(at, Upright());
    sim.f.squeeze = 0.0f;
    for (int i = 0; i < 3; ++i)
        sim.Step();
    sim.f.squeeze = 1.0f;
    sim.Step();
    const bool took = sim.r.took;
    for (int i = 0; i < 3; ++i)
        sim.Step();
    return took && pouch::Holding();
}

// The hand moves the magazine along the well's line (the gun's +y) from s0 to s1 at v m/s, the rest
// of its pose `shape` (its pose with its top's place at home); each() after every frame.
void Slide(Sim& sim, const Pose& shape, float s0, float s1, float v, const std::function<void()>& each = {}) {
    const float step = v * kDt;
    float s = s0;
    while (s != s1) {
        s = s1 > s0 ? std::min(s + step, s1) : std::max(s - step, s1);
        sim.InGun(Mul(Move(0.0f, s, 0.0f), shape));
        sim.Step();
        if (each)
            each();
    }
}

// What the drawn magazine did over a push into the well.
struct Watch {
    float caughtH = -1.0f, caughtAngle = 0.0f, caughtLateral = 0.0f, caughtAt = -1.0f;
    float seatedAt = -1.0f, offS = 1.0f, offH = 0.0f, offLateral = 0.0f, offAngle = 0.0f, offJump = 0.0f;
    float drawnSMost = -1.0f;  // on the line
    float moveMost = 0.0f;     // any frame it's drawn this frame and the last
    float mouthAngle = -1.0f, mouthLateral = -1.0f;  // on the line, the first frame at the mouth or above
    float wellAngleMost = 0.0f, wellLateralMost = 0.0f;  // on the line, at the mouth or above
    float gloveOff = 0.0f;  // the glove the module draws from the one holding the drawn magazine (at the hand)

    void After(Sim& sim) {
        if (sim.r.seated)
            sim.f.wellEmpty = false;  // the game: the magazine is in (skin::InsertMagazine)
        const Place hand = PlaceOf(sim.HandInGun());
        if (sim.r.caught)
            caughtH = hand.h, caughtAngle = hand.angle, caughtLateral = hand.lateral, caughtAt = sim.t;
        if (sim.r.offLine && offS > 0.5f)
            offS = hand.s, offH = hand.h, offLateral = hand.lateral, offAngle = hand.angle, offJump = sim.move;
        if (sim.r.seated)
            seatedAt = sim.t;
        moveMost = std::max(moveMost, sim.move);
        Pose e;
        if (sim.line && sim.Drawn(e)) {
            const Place d = PlaceOf(e);
            drawnSMost = std::max(drawnSMost, d.s);
            if (d.s >= kMouth - kEps) {
                if (mouthAngle < 0.0f)
                    mouthAngle = d.angle, mouthLateral = d.lateral;
                wellAngleMost = std::max(wellAngleMost, d.angle);
                wellLateralMost = std::max(wellLateralMost, d.lateral);
            }
            if (sim.r.gloveBlend > 0.0f) {
                float g[12], a[3], b[3];
                pouch::GloveFor(sim.r.magPose, g);
                Apply(FromArray(g), g_handInGlove, a);
                Apply(FromArray(sim.r.glove), g_handInGlove, b);
                gloveOff = std::max(gloveOff, Distance(a, b));
            }
        }
    }
};

// ---- Reporting ----

int g_failed = 0, g_failedAsked = 0, g_counted = 0, g_infoOff = 0, g_failedEither = 0;
// m1's mirrored run: its checks counted apart and not printed (m1 names any that failed).
int g_mirrorCounted = 0, g_mirrorFailed = 0;
std::string g_mirrorFailedIds;

void Report(const char* id, const char* what, bool pass, bool info, const char* format, ...) {
    if (g_mirror) {
        if (!info) {
            ++g_mirrorCounted;
            if (!pass)
                ++g_mirrorFailed, g_mirrorFailedIds += std::string(" ") + id;
        }
        return;
    }
    char numbers[1024];
    va_list args;
    va_start(args, format);
    std::vsnprintf(numbers, sizeof(numbers), format, args);
    va_end(args);
    std::printf("%-4s %-5s %s | %s\n", id, info ? (pass ? "info" : "info*") : pass ? "PASS" : "FAIL", what, numbers);
    if (info) {
        g_infoOff += !pass;
        return;
    }
    ++g_counted;
    if (!pass) {
        ++g_failed;
        if (id[0] == 'm')
            ++g_failedEither;
        else if (std::atoi(id) <= 9)
            ++g_failedAsked;
    }
}

const char* YesNo(bool b) {
    return b ? "yes" : "no";
}

// ---- 1. Taking a magazine at the belly ----

void Scenario1() {
    struct Case {
        const char* id;
        const char* what;
        bool empty, dry, free, spare;
        float guard;
        bool gun, take;
        const char* why;
    };
    const Case cases[] = {
        {"1a", "at the belly, the gun hot (its magazine out), the hand free: takes one", true, false, true,
         true, 1.0f, true, true, "took a magazine"},
        {"1b", "dry with its magazine in (the bolt back: hot too): takes one", false, true, true, true, 1.0f, true,
         true, "took a magazine"},
        {"1c", "its magazine in and not dry: nothing (X first)", false, false, true, true, 1.0f, true, false,
         "X first"},
        {"1d", "the hand not free (handFree false): nothing", true, false, false, true, 1.0f, true, false, "busy"},
        {"1e", "no spare rounds: nothing", true, false, true, false, 1.0f, true, false, "no rounds"},
        {"1f", "the palm 8 cm from the handguard's line (guardDistance 0.08): nothing", true, false, true, true, 0.08f,
         true, false, "handguard"},
        {"1g", "no gun the pouch knows (gun false): nothing", true, false, true, true, 1.0f, false, false, "no gun"},
    };
    float middle[3];
    MiddleFor(kNeck0, 0.0f, middle);
    for (const Case& c : cases) {
        Sim sim;
        Fresh(sim, c.id);
        sim.f.wellEmpty = c.empty, sim.f.dry = c.dry, sim.f.handFree = c.free, sim.f.spare = c.spare;
        sim.f.guardDistance = c.guard, sim.f.gun = c.gun;
        sim.AtGrip(middle, Upright());
        sim.f.squeeze = 0.0f;
        Still(sim, 5 * kDt);
        sim.f.squeeze = 1.0f;
        bool reached = false;
        float size = -1.0f;
        std::string note;
        for (int i = 0; i < 12; ++i) {
            sim.Step();
            if (sim.r.reached)
                reached = true, size = sim.r.size, note = sim.r.note;
        }
        const bool pass = reached && sim.took == (c.take ? 1 : 0) && note.find(c.why) != std::string::npos &&
                          pouch::Holding() == c.take;
        Report(c.id, c.what, pass, false,
               "grip (%.3f %.3f %.3f): the squeeze reached it %s (%.2f of its size), took %d, holding %s, \"%s\"",
               sim.f.leftGrip.x, sim.f.leftGrip.y, sim.f.leftGrip.z, YesNo(reached), size, sim.took,
               YesNo(pouch::Holding()), note.c_str());
    }
}

// ---- 2. The squeeze's edge and its 0.4 s ----

void Scenario2() {
    float middle[3];
    MiddleFor(kNeck0, 0.0f, middle);
    {
        Sim sim;
        Fresh(sim, "2a");
        float rest[3];
        PouchPoint(kNeck0, 0.0f, -0.18f, -0.30f, 0.25f, rest);  // in front of the chest
        sim.AtGrip(rest, Upright());
        sim.f.squeeze = 0.0f;
        Still(sim, 0.1f);
        sim.f.squeeze = 1.0f;
        sim.Step();
        const float crossedAt = sim.t;
        Still(sim, 0.5f);
        float enteredAt = -1.0f;
        for (int i = 1, n = Frames(0.3f); i <= n; ++i) {
            float p[3];
            const float u = Ease(static_cast<float>(i) / n);
            for (int k = 0; k < 3; ++k)
                p[k] = rest[k] + (middle[k] - rest[k]) * u;
            sim.AtGrip(p, Upright());
            sim.Step();
            if (enteredAt < 0.0f && SizeAt(kNeck0, 0.0f, p) <= 1.0f)
                enteredAt = sim.t;
        }
        Still(sim, 0.5f);
        Report("2a", "a squeeze already held while the hand sweeps into the belly: nothing", sim.took == 0, false,
               "squeezed %.2f s before the hand came in, took %d", enteredAt - crossedAt, sim.took);
    }
    for (int k = 0; k < 2; ++k) {
        const char* id = k == 0 ? "2b" : "2c";
        const float v = k == 0 ? 0.5f : 0.1f, out = 0.05f;
        // From 5 cm in front of the zone's front, straight back to its middle at the belly.
        const float front = kBelly[2] + kBellyRadii[2], way = front + out - kBelly[2];
        Sim sim;
        Fresh(sim, id);
        float start[3];
        PouchPoint(kNeck0, 0.0f, kBelly[0], kBelly[1], front + out, start);
        sim.AtGrip(start, Upright());
        sim.f.squeeze = 0.0f;
        Still(sim, 0.1f);
        sim.f.squeeze = 1.0f;
        sim.Step();
        int lines = sim.r.squeezed;  // the log lines this squeeze gives (pouch.cpp: one, when it settles)
        const float crossedAt = sim.t, crossedSize = SizeAt(kNeck0, 0.0f, start);
        float enteredAt = -1.0f, tookAt = sim.r.took ? sim.t : -1.0f, d = 0.0f;
        while (d < way) {
            d = std::min(d + v * kDt, way);
            float p[3];
            PouchPoint(kNeck0, 0.0f, kBelly[0], kBelly[1], front + out - d, p);
            sim.AtGrip(p, Upright());
            sim.Step();
            lines += sim.r.squeezed;
            if (enteredAt < 0.0f && SizeAt(kNeck0, 0.0f, p) <= 1.0f)
                enteredAt = sim.t;
            if (sim.r.took && tookAt < 0.0f)
                tookAt = sim.t;
        }
        Still(sim, 0.3f, [&] { lines += sim.r.squeezed; });
        const bool take = k == 0;
        const bool pass =
            sim.took == (take ? 1 : 0) && (!take || std::fabs(tookAt - enteredAt) < 1e-4f) && lines == 1;
        Report(id,
               take ? "squeezed 5 cm in front of the zone, the hand in 0.1 s later (0.5 m/s): takes one"
                    : "squeezed 5 cm in front of the zone, the hand in 0.5 s later (0.1 m/s): nothing",
               pass, false, "squeezed at %.2f of its size, the hand in %.3f s later, took %d%s; %d log line(s)",
               crossedSize, enteredAt - crossedAt, sim.took,
               tookAt >= 0.0f ? (tookAt == enteredAt ? " (as it came in)" : " (later)") : "", lines);
    }
}

// ---- 3. The body's heading ----

void Scenario3() {
    float middle0[3];
    MiddleFor(kNeck0, 0.0f, middle0);
    // Looking down 60 degrees and turning the head 30 left (slowly: 2 s, so the heading settles towards
    // the head) with the hand at the belly; probes (fresh squeezes every other frame; no gun, so
    // nothing's taken) give the heading the module went by, and whether the hand was still in the zone.
    for (int k = 0; k < 2; ++k) {
        const bool reaching = k == 0;
        const char* id = reaching ? "3a" : "3c";
        Sim sim;
        Fresh(sim, id);
        sim.f.gun = false;
        const float rest[3] = {kNeck0[0], kNeck0[1] - 0.30f, kNeck0[2] - 0.35f};  // in front of the chest
        float mostHeading = 0.0f;
        int probes = 0, outside = 0;
        const int reach = Frames(0.6f), turn = Frames(2.0f), hold = Frames(2.0f);
        for (int i = 1; i <= reach + turn + hold; ++i) {
            const float u = reaching ? Ease(static_cast<float>(i) / reach) : 0.0f;
            const float w = Ease(static_cast<float>(i - reach) / turn);
            sim.Head(HeadAt(kNeck0, 30.0f * w, -60.0f * w));
            float p[3];
            for (int j = 0; j < 3; ++j)
                p[j] = rest[j] + (middle0[j] - rest[j]) * u;
            sim.AtGrip(p, Upright());
            sim.f.squeeze = i % 2 == 0 ? 1.0f : 0.0f;
            sim.Step();
            if (!sim.r.squeezed)
                continue;
            const float heading = HeadingFrom(kNeck0, p, sim.r.fromMiddle);
            ++probes;
            if (i > reach && !sim.r.reached)
                ++outside;
            if (std::fabs(heading) > std::fabs(mostHeading))
                mostHeading = heading;
        }
        if (reaching)
            Report("3a", "looking down 60 deg and turning the head 30 deg left while reaching: the hand stays in the zone",
                   outside == 0 && probes > 0, false, "out of the zone in %d of %d probes (the body's heading at most %.1f deg)",
                   outside, probes, mostHeading);
        else
            Report("3c", "the same head turn with the hand in front of the chest: the heading settles after the head",
                   true, true, "the body's heading at most %.1f deg over %d probes", mostHeading, probes);
    }
    // The whole body turned 90 degrees in 1 s (the neck still, the head with it), the hand at the
    // belly of the new heading, probed every other frame.
    {
        Sim sim;
        Fresh(sim, "3b");
        sim.f.gun = false;
        const int before = Frames(0.2f), turn = Frames(1.0f), after = Frames(10.0f);
        float lagMost = 0.0f, lagEnd = -1.0f, lag1 = -1.0f, lag10 = -1.0f, sizeEnd = 0.0f, size1 = 0.0f;
        for (int i = 1; i <= before + turn + after; ++i) {
            const float yaw = 90.0f * Ease(static_cast<float>(i - before) / turn);
            sim.Head(HeadAt(kNeck0, yaw, 0.0f));
            float p[3];
            MiddleFor(kNeck0, yaw, p);
            sim.AtGrip(p, Turn(0, 1, 0, 180.0f + yaw));
            sim.f.squeeze = i % 2 == 0 ? 1.0f : 0.0f;
            sim.Step();
            if (!sim.r.squeezed)
                continue;
            const float heading = HeadingFrom(kNeck0, p, sim.r.fromMiddle);
            const float lag = std::fabs(Wrap(yaw - heading));
            lagMost = std::max(lagMost, lag);
            const float since = (i - before - turn) * kDt;  // after the turn
            float front[3];
            MiddleFor(kNeck0, 90.0f, front);  // where the hand goes for the new heading
            if (since >= 0.0f && lagEnd < 0.0f)
                lagEnd = lag, sizeEnd = SizeAt(kNeck0, heading, front);
            if (since >= 1.0f && lag1 < 0.0f)
                lag1 = lag, size1 = SizeAt(kNeck0, heading, front);
            if (since >= 10.0f - 2 * kDt && lag10 < 0.0f)
                lag10 = lag;
        }
        const bool pass = lagMost <= 36.0f && sizeEnd <= 1.0f && lag10 >= 0.0f && lag10 <= 5.0f;
        Report("3b", "the whole body turned 90 deg over 1 s: the zone follows", pass, false,
               "the heading at most %.1f deg behind; at the turn's end %.1f deg (the hand's new place at %.2f of "
               "the zone's size), 1 s later %.1f deg (%.2f), 10 s later %.1f deg",
               lagMost, lagEnd, sizeEnd, lag1, size1, lag10);
    }
}

// ---- 4. Pushed in upright ----

void Scenario4() {
    for (int k = 0; k < 2; ++k) {
        const char* id = k == 0 ? "4a" : "4b";
        const float v = k == 0 ? 0.3f : 2.0f;
        Sim sim;
        Fresh(sim, id);
        sim.InGun(Move(0.0f, -0.20f, 0.0f));
        const bool took = TakeHere(sim);
        Watch w;
        Slide(sim, Pose(), -0.20f, 0.01f, v, [&] { w.After(sim); });
        Still(sim, 0.3f, [&] { w.After(sim); });
        sim.f.squeeze = 0.0f;
        Still(sim, 0.2f, [&] { w.After(sim); });
        const bool pass = took && sim.caught == 1 && w.caughtH > 0.0f && w.caughtH <= kFunnelDepth + kEps &&
                          sim.seated == 1 && sim.offLine == 0 && sim.letGo == 0 && sim.cancelled == 0 &&
                          w.drawnSMost <= kEps && w.moveMost <= kMaxStep + kEps && w.gloveOff < 0.001f;
        char what[160];
        std::snprintf(what, sizeof(what),
                      "upright from 20 cm below home, pushed up the gun's +y at %.1f m/s: caught, rides, seats once", v);
        Report(id, what, pass, false,
               "took %s; caught %d at h %.1f cm; seated %d, %.2f s after the catch; the drawn s at most %+.2f mm, "
               "moved at most %.1f mm a frame; off the line %d, let go %d, gone %d; the module's glove %.2f mm from "
               "the one holding its magazine",
               YesNo(took), sim.caught, w.caughtH * 100, sim.seated, w.seatedAt - w.caughtAt, w.drawnSMost * 1000,
               w.moveMost * 1000, sim.offLine, sim.letGo, sim.cancelled, w.gloveOff * 1000);
    }
}

// ---- 5. Wrong entries ----

void Scenario5() {
    struct Wrong {
        const char* id;
        const char* what;
        Pose shape;
    };
    const Wrong wrongs[] = {
        {"5a", "backwards (turned 180 deg about the gun's y), pushed up and back: never caught", TurnAbout(0, 1, 0, 180.0f, kTop)},
        {"5b", "sideways (90 deg about y): never caught", TurnAbout(0, 1, 0, 90.0f, kTop)},
        {"5c", "upside down (180 deg about the gun's z): never caught", TurnAbout(0, 0, 1, 180.0f, kTop)},
        {"5d", "tilted 55 deg fore and aft (about x): never caught", TurnAbout(1, 0, 0, 55.0f, kTop)},
        {"5e", "tilted 55 deg sideways (about z): never caught", TurnAbout(0, 0, 1, 55.0f, kTop)},
        {"5f", "upright, 8 cm to the side of the line: never caught", Move(0.08f, 0.0f, 0.0f)},
        {"5g", "upright, 8 cm forward of the line: never caught", Move(0.0f, 0.0f, 0.08f)},
    };
    for (const Wrong& c : wrongs) {
        Sim sim;
        Fresh(sim, c.id);
        sim.InGun(Mul(Move(0.0f, -0.20f, 0.0f), c.shape));
        TakeHere(sim);
        float angleLeast = 999.0f, lateralLeast = 999.0f;
        auto each = [&] {
            const Place p = PlaceOf(sim.HandInGun());
            if (p.h >= 0.0f && p.h <= kFunnelDepth)
                angleLeast = std::min(angleLeast, p.angle), lateralLeast = std::min(lateralLeast, p.lateral);
        };
        Slide(sim, c.shape, -0.20f, 0.0f, 0.3f, each);
        Slide(sim, c.shape, 0.0f, -0.20f, 0.3f, each);
        Report(c.id, c.what, sim.caught == 0, false, "caught %d (in the funnel's depths the hand %.0f deg, %.1f cm off)",
               sim.caught, angleLeast, lateralLeast * 100);
    }
    {
        // The top 3 cm above the mouth (in the gun), down to 5 cm below the mouth (in the funnel, not
        // out of it) and back up.
        Sim sim;
        Fresh(sim, "5h");
        const float s0 = kMouth + 0.03f, s1 = kMouth - 0.05f;
        sim.InGun(Move(0.0f, s0, 0.0f));
        TakeHere(sim);
        float caughtH = -1.0f, sMost = -1.0f;
        bool down = true, caughtDown = false;
        auto each = [&] {
            if (sim.r.caught && caughtH < -0.5f)
                caughtH = PlaceOf(sim.HandInGun()).h, caughtDown = down;
            Pose e;
            if (sim.line && sim.Drawn(e))
                sMost = std::max(sMost, PlaceOf(e).s);
        };
        Slide(sim, Pose(), s0, s1, 0.3f, each);
        down = false;
        Slide(sim, Pose(), s1, s0, 0.3f, each);
        char numbers[256] = "";
        if (sim.caught)
            std::snprintf(numbers, sizeof(numbers), " at h %.1f cm (%s); on the line at the end %s, drawn up to s %.1f cm",
                          caughtH * 100, caughtDown ? "coming down" : "going up", YesNo(sim.line), sMost * 100);
        Report("5h", "down from above through the receiver (the top 3 cm above the mouth, down 8 cm, back up): never caught",
               sim.caught == 0, false, "caught %d%s", sim.caught, numbers);
    }
    {
        // In from the side above the mouth: the top 2 cm into the receiver, from 12 cm to the side.
        Sim sim;
        Fresh(sim, "5i");
        const float s = kMouth + 0.02f;
        sim.InGun(Move(0.12f, s, 0.0f));
        TakeHere(sim);
        for (float x = 0.12f; x > 0.0f;) {
            x = std::max(x - 0.3f * kDt, 0.0f);
            sim.InGun(Move(x, s, 0.0f));
            sim.Step();
        }
        Still(sim, 0.3f);
        Report("5i", "in from the side above the mouth (2 cm into the receiver): never caught", sim.caught == 0, false,
               "caught %d", sim.caught);
    }
}

// ---- 6. A tracking jump on the line ----

void Scenario6() {
    Sim sim;
    Fresh(sim, "6");
    sim.InGun(Move(0.0f, -0.20f, 0.0f));
    TakeHere(sim);
    Slide(sim, Pose(), -0.20f, -0.06f, 0.3f);
    Still(sim, 0.2f);
    const bool onBefore = sim.line;
    sim.InGun(Move(0.0f, -0.06f + 0.20f, 0.0f));  // 20 cm up the line for a frame
    sim.Step();
    const float moveJump = sim.move;
    const bool drawnJump = sim.r.mag, offJump = sim.r.offLine, seatedJump = sim.r.seated;
    sim.InGun(Move(0.0f, -0.06f, 0.0f));
    sim.Step();
    const float moveAfter = sim.move;
    Still(sim, 0.5f);
    Pose e;
    const bool drawn = sim.Drawn(e);
    const Place d = PlaceOf(e);
    const bool pass = onBefore && drawnJump && moveJump <= kMaxStep + kEps && moveAfter <= kMaxStep + kEps &&
                      sim.seated == 0 && !seatedJump;
    Report("6", "on the line 6 cm from home, the hand 20 cm up for one frame: moves <= 3 cm, no seat", pass, false,
           "on the line before %s; the jump frame: the drawn magazine moved %.1f cm (off the line %s), the frame "
           "after %.1f cm; seated %d, off the line %d in all; at the end %s, drawn at s %.1f cm, %.1f cm off the line",
           YesNo(onBefore), moveJump * 100, YesNo(offJump), moveAfter * 100, sim.seated, sim.offLine,
           sim.line ? "on the line" : "in the hand", drawn ? d.s * 100 : 0.0f, drawn ? d.lateral * 100 : 0.0f);
}

// ---- 7. Letting go ----

void Scenario7() {
    float middle[3];
    MiddleFor(kNeck0, 0.0f, middle);
    {
        Sim sim;
        Fresh(sim, "7a");
        const bool took = TakeAt(sim, middle);
        float air[3];
        PouchPoint(kNeck0, 0.0f, -0.40f, -0.35f, 0.29f, air);  // out to the left at the waist
        for (int i = 1, n = Frames(0.5f); i <= n; ++i) {
            float p[3];
            const float u = Ease(static_cast<float>(i) / n);
            for (int k = 0; k < 3; ++k)
                p[k] = middle[k] + (air[k] - middle[k]) * u;
            sim.AtGrip(p, Upright());
            sim.Step();
        }
        Still(sim, 0.1f);
        sim.f.squeeze = 0.3f;
        sim.Step();
        const pouch::Result r = sim.r;
        Report("7a", "let go (squeeze 0.3) in the air: letGo", took && r.letGo && !r.putBack && !pouch::Holding() &&
                                                                    sim.caught == 0,
               false, "took %s; at %.2f of the zone's size: letGo %d, putBack %d, holding %s, caught on the way %d",
               YesNo(took), SizeAt(kNeck0, 0.0f, air), r.letGo, r.putBack, YesNo(pouch::Holding()), sim.caught);
    }
    {
        Sim sim;
        Fresh(sim, "7b");
        const bool took = TakeAt(sim, middle);
        Still(sim, 0.2f);
        sim.f.squeeze = 0.3f;
        sim.Step();
        const pouch::Result r = sim.r;
        Report("7b", "let go at the belly: putBack", took && r.putBack && !r.letGo && !pouch::Holding(), false,
               "took %s; letGo %d, putBack %d, holding %s", YesNo(took), r.letGo, r.putBack, YesNo(pouch::Holding()));
    }
    {
        Sim sim;
        Fresh(sim, "7c");
        TakeHere(sim);
        Slide(sim, Pose(), -0.20f, -0.03f, 0.3f);
        Still(sim, 3 * kDt);
        const bool on = sim.line;
        sim.f.squeeze = 0.3f;
        sim.Step();
        const pouch::Result r = sim.r;
        Report("7c", "let go on the line 3 cm before home: letGo", on && r.letGo && !r.putBack && !r.seated &&
                                                                       sim.seated == 0 && !pouch::Holding(),
               false, "on the line %s; letGo %d, putBack %d, seated %d", YesNo(on), r.letGo, r.putBack, sim.seated);
    }
    {
        Sim sim;
        Fresh(sim, "7d");
        TakeHere(sim);
        float s = -0.20f, firstS = 0.0f;
        bool first = false, second = false;
        pouch::Result r;
        for (int i = 0; i < Frames(2.0f) && !second; ++i) {
            s = std::min(s + 0.3f * kDt, 0.005f);
            sim.InGun(Move(0.0f, s, 0.0f));
            sim.Step();
            Pose e;
            if (first) {
                r = sim.r, second = true;
            } else if (sim.line && sim.Drawn(e) && PlaceOf(e).s >= -kHome) {
                first = true, firstS = PlaceOf(e).s;
                sim.f.squeeze = 0.3f;  // let go on the next frame, the second at home
            }
        }
        Report("7d", "let go on the second frame at home: seated, not letGo", second && r.seated && !r.letGo && !r.putBack,
               false, "the first frame at home at s %.1f mm; the next (squeeze 0.3): seated %d, letGo %d, putBack %d",
               firstS * 1000, r.seated, r.letGo, r.putBack);
    }
    // Let go mid-swing in the air: it falls at the hand's speed (at most 3 m/s), along the swing (the
    // design's test 11.17; 7a's hand is still when it lets go).
    for (int k = 0; k < 3; ++k) {
        const char* id = k == 0 ? "7e" : k == 1 ? "7f" : "7g";
        const float speed = k == 0 ? 1.5f : k == 1 ? 5.0f : 0.6f;
        const float dir[3] = {k == 2 ? 0.0f : -0.8f, k == 2 ? -1.0f : 0.36f, k == 2 ? 0.0f : -0.48f};
        Sim sim;
        Fresh(sim, id);
        const bool took = TakeAt(sim, middle);
        float air[3];
        PouchPoint(kNeck0, 0.0f, -0.40f, -0.35f, 0.29f, air);
        for (int i = 1, n = Frames(0.5f); i <= n; ++i) {
            float p[3];
            const float u = Ease(static_cast<float>(i) / n);
            for (int c = 0; c < 3; ++c)
                p[c] = middle[c] + (air[c] - middle[c]) * u;
            sim.AtGrip(p, Upright());
            sim.Step();
        }
        const float n = std::sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
        float p[3] = {air[0], air[1], air[2]};
        for (int i = 0; i < 6; ++i) {
            for (int c = 0; c < 3; ++c)
                p[c] += dir[c] / n * speed * kDt;
            sim.AtGrip(p, Upright());
            if (i == 5)
                sim.f.squeeze = 0.3f;
            sim.Step();
        }
        const pouch::Result r = sim.r;
        const float want = std::min(speed, 3.0f);
        const float* v = r.dropVelocity;
        const float got = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        float along = 0.0f;
        for (int c = 0; c < 3; ++c)
            along += v[c] * dir[c] / n;
        Report(id, "let go in the air mid-swing: dropped at the hand's speed (design 11.17)",
               took && r.letGo && std::fabs(got - want) <= 0.1f * want && along >= 0.99f * got, false,
               "took %s; letGo %d; swing %.2f m/s, want %.2f; dropVelocity (%.3f %.3f %.3f) = %.3f m/s, %.3f along",
               YesNo(took), r.letGo, speed, want, v[0], v[1], v[2], got, along);
    }
}

// ---- 8. The hand lost, another magazine in ----

void Scenario8() {
    for (int k = 0; k < 2; ++k) {
        const char* id = k == 0 ? "8a" : "8b";
        const float lost = k == 0 ? 0.5f : 1.2f;
        Sim sim;
        Fresh(sim, id);
        TakeHere(sim);  // upright 20 cm below home, in the hand
        Still(sim, 0.1f);
        sim.f.left = false;
        const float lostAt = sim.t;
        bool heldLost = true, drawnLost = false;
        float goneAfter = -1.0f;
        for (int i = 0, n = Frames(lost); i < n; ++i) {
            sim.Step();
            if (sim.r.cancelled && goneAfter < 0.0f)
                goneAfter = sim.t - lostAt;
            if (goneAfter < 0.0f && !sim.r.holding)
                heldLost = false;
            drawnLost |= sim.r.mag;
        }
        sim.f.left = true;
        sim.Step();
        const bool backHolding = sim.r.holding && pouch::Holding(), backDrawn = sim.r.mag;
        Still(sim, 0.3f);
        if (k == 0)
            Report(id, "the left hand lost 0.5 s while held, then back: still held", sim.cancelled == 0 && heldLost &&
                                                                                      backHolding && backDrawn &&
                                                                                      pouch::Holding(),
                   false, "gone %d; held through the loss %s (drawn while lost %s); back: holding %s, drawn %s",
                   sim.cancelled, YesNo(heldLost), YesNo(drawnLost), YesNo(backHolding), YesNo(backDrawn));
        else
            Report(id, "the left hand lost 1.2 s while held: cancelled", sim.cancelled == 1 && goneAfter > 0.95f &&
                                                                             goneAfter < 1.05f && !backHolding &&
                                                                             sim.letGo == 0 && !pouch::Holding(),
                   false, "gone %d, %.3f s after the loss; back: holding %s; letGo %d", sim.cancelled, goneAfter,
                   YesNo(backHolding), sim.letGo);
    }
    {
        Sim sim;
        Fresh(sim, "8c");
        TakeHere(sim);
        Still(sim, 0.1f);
        sim.f.wellEmpty = false;  // X, the game's reload or a new life put one in
        sim.Step();
        const bool now = sim.r.cancelled;
        Still(sim, 0.2f);
        Report("8c", "another magazine going in (wellEmpty true -> false) while held: cancelled",
               now && sim.cancelled == 1 && !pouch::Holding() && sim.letGo == 0 && sim.seated == 0, false,
               "gone on that frame %s, gone %d in all, holding %s, letGo %d, seated %d", YesNo(now), sim.cancelled,
               YesNo(pouch::Holding()), sim.letGo, sim.seated);
    }
    {
        Sim sim;
        Fresh(sim, "8d");
        TakeHere(sim);
        Slide(sim, Pose(), -0.20f, -0.06f, 0.3f);
        Still(sim, 0.1f);
        const bool on = sim.line;
        sim.f.left = false;
        float sLeast = 1.0f, sMost = -1.0f;
        bool allDrawn = true;
        for (int i = 0, n = Frames(0.5f); i < n; ++i) {
            sim.Step();
            Pose e;
            if (sim.Drawn(e))
                sLeast = std::min(sLeast, PlaceOf(e).s), sMost = std::max(sMost, PlaceOf(e).s);
            else
                allDrawn = false;
        }
        sim.f.left = true;
        Still(sim, 0.2f);
        const bool pass = on && sim.cancelled == 0 && allDrawn && std::fabs(sLeast + 0.06f) < 0.001f &&
                          std::fabs(sMost + 0.06f) < 0.001f && sim.line;
        Report("8d", "(more) the hand lost 0.5 s on the line 6 cm from home: kept at its depth, drawn there", pass, false,
               "gone %d; drawn every lost frame %s, at s %.1f to %.1f cm; on the line after %s", sim.cancelled,
               YesNo(allDrawn), sLeast * 100, sMost * 100, YesNo(sim.line));
    }
}

// ---- 9. Pulled off the line ----

void Scenario9() {
    Sim sim;
    Fresh(sim, "9");
    TakeHere(sim);
    const float s = kMouth - 0.04f;  // caught, then held 4 cm below the mouth
    Slide(sim, Pose(), -0.20f, s, 0.3f);
    Still(sim, 0.1f);
    const bool on = sim.line;
    float offAt = -1.0f, jump = 0.0f, offFromHand = 0.0f, x = 0.0f;
    auto check = [&] {
        if (offAt < 0.0f)
            return;
        offFromHand = std::max(offFromHand, sim.r.mag ? Apart(FromArray(sim.r.magPose), sim.mag) : 1.0f);
    };
    while (x < 0.10f) {
        x = std::min(x + 0.5f * kDt, 0.10f);
        sim.InGun(Move(x, s, 0.0f));
        sim.Step();
        if (sim.r.offLine && offAt < 0.0f)
            offAt = x, jump = sim.move;
        check();
    }
    Still(sim, 0.3f, check);
    const bool pass = on && sim.offLine == 1 && offAt > 0.0f && pouch::Holding() && sim.r.holding && sim.r.mag &&
                      offFromHand < 0.001f && sim.caught == 1;
    Report("9", "caught 4 cm below the mouth, the hand moved 10 cm sideways: offLine, back in the hand", pass, false,
           "on the line before %s; off the line %d, with the hand %.1f cm to the side (the drawn magazine jumped "
           "%.1f cm to it); after: holding %s, drawn at the hand within %.2f mm; caught again %d",
           YesNo(on), sim.offLine, offAt * 100, jump * 100, YesNo(pouch::Holding()), offFromHand * 1000,
           sim.caught - 1);
}

// ---- 10. The zone's edges (more) ----

void Scenario10() {
    {
        Sim sim;
        Fresh(sim, "10a");
        sim.f.gun = false;
        struct Face {  // from the zone's middle
            const char* name;
            float right, up, forward;
        };
        const Face faces[] = {{"right", kBellyRadii[0], 0, 0}, {"left", -kBellyRadii[0], 0, 0},
                              {"up", 0, kBellyRadii[1], 0},    {"down", 0, -kBellyRadii[1], 0},
                              {"front", 0, 0, kBellyRadii[2]}, {"back", 0, 0, -kBellyRadii[2]}};
        int wrong = 0;
        std::string misses;
        for (const Face& face : faces)
            for (const float k : {0.95f, 1.05f}) {
                float p[3];
                PouchPoint(kNeck0, 0.0f, kBelly[0] + face.right * k, kBelly[1] + face.up * k,
                           kBelly[2] + face.forward * k, p);
                sim.AtGrip(p, Upright());
                sim.f.squeeze = 0.0f;
                Still(sim, 2 * kDt);
                sim.f.squeeze = 1.0f;
                sim.Step();
                if (sim.r.reached != (k < 1.0f)) {
                    ++wrong;
                    char m[64];
                    std::snprintf(m, sizeof(m), " %s at %.2f", face.name, k);
                    misses += m;
                }
            }
        Report("10a", "(more) each limit of the zone: a squeeze at 0.95 of it reaches it, at 1.05 doesn't",
               wrong == 0, false, "%d of 12 wrong%s", wrong, misses.c_str());
    }
    {
        Sim sim;
        Fresh(sim, "10b");
        struct Spot {
            const char* name;
            float right, up, forward;
        };
        const Spot spots[] = {{"the chest", 0.0f, -0.25f, 0.24f},
                              {"over the left shoulder, where it was until 2026-11-05", -0.18f, 0.02f, -0.10f},
                              {"the pelvis, where the pouch used to be", 0.0f, -0.60f, 0.19f},
                              {"the hip", 0.25f, -0.60f, 0.19f},
                              {"the thigh", 0.0f, -0.85f, 0.24f},
                              {"behind the hip", 0.0f, -0.60f, -0.11f},
                              {"the handguard's place", -0.10f, -0.25f, 0.45f}};
        int taken = 0;
        std::string which;
        for (const Spot& spot : spots) {
            float p[3];
            PouchPoint(kNeck0, 0.0f, spot.right, spot.up, spot.forward, p);
            sim.AtGrip(p, Upright());
            sim.f.squeeze = 0.0f;
            Still(sim, 2 * kDt);
            sim.f.squeeze = 1.0f;
            Still(sim, 3 * kDt);
            if (sim.took > taken) {
                taken = sim.took;
                which += std::string(" ") + spot.name;
                pouch::Clear();
            }
        }
        Report("10b", "(more) squeezes at the chest, over the left shoulder, the pelvis, the hip, the thigh, behind the hip, the handguard's place: nothing", taken == 0,
               false, "took %d%s", taken, which.c_str());
    }
}

// ---- 11. A crooked entry, straightened (more) ----

void Scenario11() {
    struct Tilt {
        const char* id;
        const char* what;
        float x, y, z;
    };
    const Tilt tilts[] = {
        {"11a", "(more) 20 deg fore and aft and 3 cm to the side, caught 5 cm below the mouth: straightened, seats", 1, 0, 0},
        {"11b", "(more) 20 deg sideways and 3 cm to the side, caught 5 cm below the mouth: straightened, seats", 0, 0, 1},
    };
    for (const Tilt& c : tilts) {
        Sim sim;
        Fresh(sim, c.id);
        const Pose shape = Mul(Move(0.03f, 0.0f, 0.0f), TurnAbout(c.x, c.y, c.z, 20.0f, kTop));
        const float s0 = kMouth - 0.10f;
        sim.InGun(Mul(Move(0.0f, s0, 0.0f), shape));
        TakeHere(sim);
        Watch w;
        Slide(sim, shape, s0, 0.01f, 0.3f, [&] { w.After(sim); });
        Still(sim, 0.3f, [&] { w.After(sim); });
        const bool pass = sim.caught == 1 && w.caughtH >= 0.0f && w.caughtH <= kFunnelDepth + kEps && sim.offLine == 0 &&
                          sim.seated == 1 && w.mouthAngle >= 0.0f && w.mouthAngle <= 15.0f + 0.01f &&
                          w.mouthLateral <= 0.03f + kEps;
        char off[160] = "";
        if (sim.offLine)
            std::snprintf(off, sizeof(off), " (the hand's top at h %.1f cm: %s the mouth)", w.offH * 100,
                          w.offH < 0.0f ? "above" : "below");
        Report(c.id, c.what, pass, false,
               "caught %d at h %.1f cm (the hand %.0f deg, %.1f cm off); the drawn magazine at the mouth %.1f deg, "
               "%.1f mm off; off the line %d%s; seated %d; moved at most %.1f mm a frame",
               sim.caught, w.caughtH * 100, w.caughtAngle, w.caughtLateral * 100, w.mouthAngle, w.mouthLateral * 1000,
               sim.offLine, off, sim.seated, w.moveMost * 1000);
    }
}

// ---- 12. The wrist turned inside the well (more) ----

void Scenario12() {
    struct Wrist {
        const char* id;
        const char* what;
        float x, y, z;
    };
    const Wrist wrists[] = {
        {"12a", "(more) on the line 5 cm from home, the wrist rolled 10 deg (about the gun's z): stays on", 0, 0, 1},
        {"12b", "(more) ... the wrist pitched 10 deg (about x): stays on", 1, 0, 0},
        {"12c", "(more) ... the wrist twisted 10 deg (about y): stays on", 0, 1, 0},
    };
    const float s = -0.05f;
    for (const Wrist& c : wrists) {
        Sim sim;
        Fresh(sim, c.id);
        TakeHere(sim);
        Slide(sim, Pose(), -0.20f, s, 0.3f);
        Still(sim, 0.1f);
        const bool on = sim.line;
        float offAt = -1.0f;
        for (int i = 1, n = Frames(0.2f); i <= n; ++i) {
            const float a = 10.0f * Ease(static_cast<float>(i) / n);
            sim.InGun(Mul(Move(0.0f, s, 0.0f), TurnAbout(c.x, c.y, c.z, a, kHandMiddle)));
            sim.Step();
            if (sim.r.offLine && offAt < 0.0f)
                offAt = a;
        }
        Still(sim, 0.3f);
        // What the module measures inside the well (the glove's origin, its bind space's, from the
        // drawn glove's) and the hand's own middle, with the full 10 degrees.
        const Pose hand = Mul(Move(0.0f, s, 0.0f), TurnAbout(c.x, c.y, c.z, 10.0f, kHandMiddle)), drawn = Move(0.0f, s, 0.0f);
        const Pose gloveHand = Mul(hand, g_gloveOfSeated), gloveDrawn = Mul(drawn, g_gloveOfSeated);
        const float origin[3] = {0, 0, 0};
        float a[3], b[3], ha[3], hb[3];
        Apply(gloveHand, origin, a);
        Apply(gloveDrawn, origin, b);
        Apply(hand, kHandMiddle, ha);
        Apply(drawn, kHandMiddle, hb);
        char off[64] = "";
        if (sim.offLine)
            std::snprintf(off, sizeof(off), " at %.1f deg", offAt);
        Report(c.id, c.what, on && sim.offLine == 0 && sim.line, false,
               "off the line %d%s; with 10 deg the glove's origin is %.1f cm from the drawn glove's (what the module "
               "measures inside the well, 12 cm allowed), the hand's middle %.1f cm",
               sim.offLine, off, Distance(a, b) * 100, Distance(ha, hb) * 100);
    }
    {
        Sim sim;
        Fresh(sim, "12d");
        TakeHere(sim);
        Slide(sim, Pose(), -0.20f, s, 0.3f);
        Still(sim, 0.1f);
        float x = 0.0f, offAt = -1.0f;
        while (x < 0.15f) {
            x = std::min(x + 0.5f * kDt, 0.15f);
            sim.InGun(Move(x, s, 0.0f));
            sim.Step();
            if (sim.r.offLine && offAt < 0.0f)
                offAt = x;
        }
        Still(sim, 0.2f);
        Report("12d", "(more) on the line 5 cm from home, the hand pulled 15 cm sideways: off the line", sim.offLine == 1,
               false, "off the line %d (the hand %.1f cm to the side)", sim.offLine, offAt * 100);
    }
}

// ---- 13. The whole reload (more) ----

void Scenario13() {
    Sim sim;
    Fresh(sim, "13");
    float middle[3];
    MiddleFor(kNeck0, 0.0f, middle);
    const bool took = TakeAt(sim, middle);
    Watch w;
    // Out from the belly beside the gun (25 cm to its left, 30 cm below home), then under the well, then pushed up.
    constexpr float kUnder = -0.20f;
    const Pose beside = Mul(sim.gun, Move(0.25f, -0.30f, 0.0f)), to = Mul(sim.gun, Move(0.0f, kUnder, 0.0f));
    Pose from = sim.mag;
    for (int i = 1, n = Frames(0.4f); i <= n; ++i) {
        sim.mag = Between(from, beside, Ease(static_cast<float>(i) / n));
        sim.Step();
        w.After(sim);
    }
    from = sim.mag;
    for (int i = 1, n = Frames(0.3f); i <= n; ++i) {
        sim.mag = Between(from, to, Ease(static_cast<float>(i) / n));
        sim.Step();
        w.After(sim);
    }
    Slide(sim, Pose(), kUnder, 0.005f, 0.5f, [&] { w.After(sim); });
    Still(sim, 0.3f, [&] { w.After(sim); });
    const bool pass = took && sim.caught == 1 && sim.seated == 1 && sim.offLine == 0 && sim.letGo == 0 &&
                      sim.cancelled == 0 && sim.putBack == 0;
    Report("13", "(more) taken at the belly, brought out under the well in 0.7 s, pushed in at 0.5 m/s: seats", pass,
           false, "took %s; caught %d at h %.1f cm, seated %d; off the line %d, let go %d, gone %d; moved at most %.1f "
           "mm a frame", YesNo(took), sim.caught, w.caughtH * 100, sim.seated, sim.offLine, sim.letGo, sim.cancelled,
           w.moveMost * 1000);
}

// ---- 14, 15. Shown, not counted ----

void Scenario14() {
    // The gun's pose missing (gunTracked false, gunPose all zeros, as hands.cpp gives it) for n frames.
    struct Case {
        const char* id;
        const char* what;
        bool onLine;
        int frames;
        bool expectKept;
    } cases[] = {
        {"14a", "the right controller's pose missing 1 frame while held: kept", false, 1, true},
        {"14b", "... 1 frame while on the line: kept, no off-line, then seats", true, 1, true},
        {"14c", "... 10 frames while on the line: kept", true, 10, true},
        {"14d", "... 1.2 s while held: gone (the 1 s rule)", false, Frames(1.2f), false},
    };
    for (const Case& c : cases) {
        Sim sim;
        Fresh(sim, c.id);
        TakeHere(sim);
        if (c.onLine)
            Slide(sim, Pose(), -0.20f, -0.06f, 0.3f);
        Still(sim, 0.1f);
        int drawnLost = 0, offLost = 0;
        for (int i = 0; i < c.frames; ++i) {
            sim.f.gunTracked = false;
            sim.zeroGun = true;
            sim.Step();
            drawnLost += sim.r.mag;
            offLost += sim.r.offLine;
        }
        sim.f.gunTracked = true;
        sim.zeroGun = false;
        const bool lineAfter = sim.line;
        Still(sim, 0.2f);
        const int offAfter = sim.offLine;
        int seated = 0;
        if (c.onLine && pouch::Holding()) {
            Slide(sim, Pose(), -0.06f, 0.005f, 0.3f, [&] {
                if (sim.r.seated)
                    sim.f.wellEmpty = false;
            });
            seated = sim.seated;
        }
        const bool kept = sim.cancelled == 0 && (c.onLine ? seated == 1 && offAfter == 0 : pouch::Holding());
        Report(c.id, c.what, c.expectKept ? kept : sim.cancelled == 1 && !pouch::Holding(), false,
               "gone %d; drawn while lost %d frames (zero gun pose), off the line while lost %d; on the line after %s, "
               "off the line in all %d; seated %d; holding at the end %s",
               sim.cancelled, drawnLost, offLost, YesNo(lineAfter), sim.offLine, seated, YesNo(pouch::Holding()));
    }
    // The weapon itself gone (Frame::gun false, the pose there): still gone at once.
    {
        Sim sim;
        Fresh(sim, "14e");
        TakeHere(sim);
        Still(sim, 0.1f);
        sim.f.gun = false;
        sim.Step();
        const bool goneNow = sim.r.cancelled;
        sim.f.gun = true;
        Still(sim, 0.1f);
        Report("14e", "the weapon switched (Frame::gun false) while held: gone at once", goneNow && !pouch::Holding(),
               false, "gone that frame %s, holding after %s", YesNo(goneNow), YesNo(pouch::Holding()));
    }
    // A fresh squeeze at the pouch while the gun's pose is missing (hands.cpp leaves guardDistance at
    // its default 1 m then: the handguard rule isn't measured).
    {
        Sim sim;
        Fresh(sim, "14f");
        sim.f.guardDistance = 1.0f;
        sim.f.squeeze = 0.0f;
        sim.f.forceInside = true;
        for (int i = 0; i < 3; ++i)
            sim.Step();
        sim.f.gunTracked = false;
        sim.zeroGun = true;
        sim.f.squeeze = 1.0f;
        sim.Step();
        const bool took = sim.r.took;
        const char* note = sim.r.note;
        sim.f.gunTracked = true;
        sim.zeroGun = false;
        sim.f.forceInside = false;
        Still(sim, 0.1f);
        Report("14f", "a squeeze at the pouch with the gun's pose missing (the handguard rule unmeasured): nothing taken",
               !took, true, "took %s (\"%s\"), holding after %s", YesNo(took), note, YesNo(pouch::Holding()));
    }
}

void Scenario15() {
    Sim sim;
    Fresh(sim, "15");
    TakeHere(sim);
    Still(sim, 0.1f);
    sim.f.left = false;
    Still(sim, 5 * kDt);
    sim.f.squeeze = 0.0f;  // let go of while the hand is lost
    sim.Step();
    const bool goneNow = sim.r.cancelled;
    Still(sim, 4 * kDt);
    sim.f.left = true;
    sim.Step();
    const bool letGoBack = sim.r.letGo;
    Report("15", "let go of while the hand is lost: gone at once (the design, section 6)", goneNow, true,
           "gone on letting go %s; when the hand came back: letGo %s, holding %s", YesNo(goneNow), YesNo(letGoBack),
           YesNo(pouch::Holding()));
}

// ---- The seated magazine by hand: the left's paths, with only the old inputs (m1 runs them again as the right's mirror image) ----

// pouch.cpp's numbers: the left hand's middle in the gloves' bind space and the way out of its palm (the right's: x negated), the
// seated magazine's middle below the well, where a hand takes hold of it, and how far the hand draws it before it moves.
constexpr float kHandInGloveL[3] = {0.5363f, 1.0976f, 0.2092f};
constexpr float kPalmInGloveL[3] = {-0.639f, -0.748f, -0.179f};
constexpr float kGrab[3] = {0.0f, -0.11f, 0.43f};
constexpr float kPullDead = 0.006f;

Pose g_fittedTurn;  // the left glove's turn on the gun as it holds the seated magazine (GloveFor of the gun's own pose)

// What the seated paths did, by run (0 the left, 1 its mirror image).
struct SeatedPaths {
    // Taken at the fitted hold, drawn down at 0.25 m/s, let go of 20 cm down.
    bool grabbed = false;
    int pulledOut = 0, letGo = 0, hidden = 0;
    float pulledOutAt = -1.0f;  // how far the hand had come down (m)
    float blends[3] = {};       // the glove's blend on the first three frames drawn down
    float drawnOff = 0.0f;      // the drawn magazine from the hand's while drawn down (most, m)
    float dropVelocity[3] = {};
    // Up instead (the handguard's), let go of before it's out, drawn 3 cm and the hand gone 25 cm aside.
    int upToGrip = 0, awayToGrip = 0;
    bool upHolding = true, earlyIn = false, awayIn = false;
    // The gun lost 5 frames part way, then drawn out; then the hand lost 1.2 s.
    bool lostKept = false;
    int lostPulledOut = 0, lostCancelled = 0;
    float lostCancelledAfter = -1.0f;
    // The palm's way at the grab's middle: up refused; up and 4 cm low taken; as fitted taken; 5 cm forward, 9.5 cm aside refused.
    bool palm[5] = {};
};
SeatedPaths g_seatedPaths[2];
struct SeatTrial {
    pouch::Frame fed;  // what Update was given
    bool grabbed = false;
};
std::vector<SeatTrial> g_seatTrials[2];  // m8's random gloves, by run

struct Random {  // the same numbers every run (xorshift)
    std::uint32_t s = 2026110601u;
    float Next() {  // 0-1
        s ^= s << 13, s ^= s >> 17, s ^= s << 5;
        return static_cast<float>(s >> 8) * (1.0f / 16777216.0f);
    }
};

// The glove (into the view) turned so on the gun (turn: no translation) with the hand's middle (pouch.cpp's) at `at` on the gun; the
// grip point there too.
void GloveAt(Sim& sim, const Pose& turn, const float* at) {
    Pose g = turn;
    float o[3];
    Apply(turn, kHandInGloveL, o);
    g.m[3] = at[0] - o[0], g.m[7] = at[1] - o[1], g.m[11] = at[2] - o[2];
    const Pose glove = Mul(sim.gun, g);
    std::memcpy(sim.f.glove, glove.m, sizeof(sim.f.glove));
    float v[3], p[3];
    Apply(sim.gun, at, v);
    Apply(sim.head, v, p);
    sim.f.leftGrip = {p[0], p[1], p[2]};
    sim.gloveGiven = sim.gripGiven = true;
}

// The fitted turn turned further about the gun's axes, its palm's way to `to` (a unit vector on the gun).
Pose PalmTo(const float* to) {
    float d[3];
    Apply(g_fittedTurn, kPalmInGloveL, d);
    const float n = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    for (float& v : d)
        v /= n;
    const float axis[3] = {d[1] * to[2] - d[2] * to[1], d[2] * to[0] - d[0] * to[2], d[0] * to[1] - d[1] * to[0]};
    const float s = std::sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
    const float c = d[0] * to[0] + d[1] * to[1] + d[2] * to[2];
    if (s < 1e-6f)  // already that way, or the other way round (any axis across it)
        return c > 0.0f ? g_fittedTurn : Mul(std::fabs(d[2]) < 0.9f ? Turn(d[1], -d[0], 0.0f, 180.0f) : Turn(0.0f, d[2], -d[1], 180.0f), g_fittedTurn);
    return Mul(Turn(axis[0], axis[1], axis[2], std::atan2(s, c) / kDegree), g_fittedTurn);
}

// Afresh with the magazine seated (the gun's own in it), the hand (side's) at the fitted hold of it, its squeeze open three frames.
void SeatedFresh(Sim& sim, const char* id, int side = 0) {
    Fresh(sim, id, side);
    sim.f.wellEmpty = false;
    sim.InGun(Pose());
    sim.f.squeeze = 0.0f;
    Still(sim, 3 * kDt);
}

// A fresh squeeze where the hand is: the seated one taken hold of?
bool Grab(Sim& sim) {
    sim.f.squeeze = 1.0f;
    sim.Step();
    return sim.r.grabbed;
}

// The hand from where it holds the seated magazine to (x, y) off it on the gun, along a line at v m/s.
void Draw(Sim& sim, float x0, float y0, float x1, float y1, float v, const std::function<void()>& each = {}) {
    const float length = std::sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
    for (float d = 0.0f; d < length;) {
        d = std::min(d + v * kDt, length);
        const float u = d / length;
        sim.InGun(Move(x0 + (x1 - x0) * u, y0 + (y1 - y0) * u, 0.0f));
        sim.Step();
        if (each)
            each();
    }
}

// ---- 16. The seated magazine by hand (more) ----

void Scenario16() {
    SeatedPaths& p = g_seatedPaths[g_mirror ? 1 : 0];
    p = SeatedPaths{};
    {
        Sim sim;
        SeatedFresh(sim, "s1");
        p.grabbed = Grab(sim);
        int drawn = 0;
        Draw(sim, 0.0f, 0.0f, 0.0f, -0.20f, 0.25f, [&] {
            if (sim.r.hideSeated) {
                if (drawn < 3)
                    p.blends[drawn] = sim.r.gloveBlend;
                ++drawn;
                ++p.hidden;
                if (sim.r.mag)
                    p.drawnOff = std::max(p.drawnOff, Apart(FromArray(sim.r.magPose), sim.mag));
            }
            if (sim.r.pulledOut) {
                ++p.pulledOut;
                p.pulledOutAt = -PlaceOf(sim.HandInGun()).s;
                sim.f.wellEmpty = true;  // the game is told (hands.cpp: ammo::Eject, skin::PullMagazine)
            }
        });
        sim.f.squeeze = 0.3f;  // let go of 20 cm down, still moving
        sim.InGun(Move(0.0f, -0.20f - 0.25f * kDt, 0.0f));
        sim.Step();
        p.letGo = sim.letGo;
        std::memcpy(p.dropVelocity, sim.r.dropVelocity, sizeof(p.dropVelocity));
    }
    {
        Sim sim;
        SeatedFresh(sim, "s2");
        Grab(sim);
        Draw(sim, 0.0f, 0.0f, 0.0f, 0.04f, 0.2f, [&] { p.upToGrip += sim.r.toGrip; });
        p.upHolding = pouch::Holding();
    }
    {
        Sim sim;
        SeatedFresh(sim, "s3");
        const bool took = Grab(sim);
        sim.f.squeeze = 0.3f;
        sim.Step();
        p.earlyIn = took && !sim.r.pulledOut && !sim.r.letGo && !sim.r.hideSeated && !sim.r.mag && !pouch::Holding();
    }
    {
        Sim sim;
        SeatedFresh(sim, "s4");
        const bool took = Grab(sim);
        int out = 0;
        Draw(sim, 0.0f, 0.0f, 0.0f, -0.03f, 0.25f, [&] { out += sim.r.pulledOut; });
        Draw(sim, 0.0f, -0.03f, 0.25f, -0.03f, 0.5f, [&] {
            p.awayToGrip += sim.r.toGrip;
            out += sim.r.pulledOut;
        });
        p.awayIn = took && out == 0 && !sim.r.hideSeated && !pouch::Holding() && sim.letGo == 0;
    }
    {
        Sim sim;
        SeatedFresh(sim, "s5");
        Grab(sim);
        Draw(sim, 0.0f, 0.0f, 0.0f, -0.04f, 0.25f);
        p.lostKept = true;
        sim.f.gunTracked = false;
        sim.zeroGun = true;
        for (int i = 0; i < 5; ++i) {
            sim.Step();
            p.lostKept = p.lostKept && sim.r.holding && pouch::Holding() && !sim.r.cancelled;
        }
        sim.f.gunTracked = true;
        sim.zeroGun = false;
        Draw(sim, 0.0f, -0.04f, 0.0f, -0.12f, 0.25f, [&] {
            if (sim.r.pulledOut) {
                ++p.lostPulledOut;
                sim.f.wellEmpty = true;
            }
        });
        sim.f.left = false;
        const float lostAt = sim.t;
        for (int i = 0, n = Frames(1.2f); i < n; ++i) {
            sim.Step();
            if (sim.r.cancelled && p.lostCancelled++ == 0)
                p.lostCancelledAfter = sim.t - lostAt;
        }
    }
    {
        const float up[3] = {0.0f, 1.0f, 0.0f}, down[3] = {0.0f, -1.0f, 0.0f};
        struct Case {
            const float* to;  // the palm's way (nullptr: as fitted)
            float at[3];
            bool take;
        } const cases[5] = {
            {up, {kGrab[0], kGrab[1], kGrab[2]}, false},
            {up, {kGrab[0], kGrab[1] - 0.04f, kGrab[2]}, true},
            {nullptr, {kGrab[0], kGrab[1], kGrab[2]}, true},
            {down, {kGrab[0], kGrab[1], kGrab[2] + 0.05f}, false},
            {down, {kGrab[0] + 0.095f, kGrab[1], kGrab[2]}, false},
        };
        for (int k = 0; k < 5; ++k) {
            Sim sim;
            Fresh(sim, "s6");
            sim.f.wellEmpty = false;
            GloveAt(sim, cases[k].to ? PalmTo(cases[k].to) : g_fittedTurn, cases[k].at);
            sim.f.squeeze = 0.0f;
            Still(sim, 2 * kDt);
            p.palm[k] = Grab(sim) == cases[k].take;
        }
    }
    {
        // m8's: 2000 gloves turned any way, their hand's middle anywhere within 15 cm of the grab's middle; a fresh squeeze each.
        Sim sim;
        Fresh(sim, "s7");
        sim.f.wellEmpty = false;
        std::vector<SeatTrial>& trials = g_seatTrials[g_mirror ? 1 : 0];
        trials.clear();
        Random random;
        for (int i = 0; i < 2000; ++i) {
            float axis[3], off[3], n = 0.0f, r = 0.0f;
            do {
                for (float& v : axis)
                    v = 2.0f * random.Next() - 1.0f;
                n = std::sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
            } while (n > 1.0f || n < 0.1f);
            const float angle = 180.0f * random.Next();
            do {
                for (float& v : off)
                    v = 0.15f * (2.0f * random.Next() - 1.0f);
                r = std::sqrt(off[0] * off[0] + off[1] * off[1] + off[2] * off[2]);
            } while (r > 0.15f);
            const float at[3] = {kGrab[0] + off[0], kGrab[1] + off[1], kGrab[2] + off[2]};
            GloveAt(sim, Mul(Turn(axis[0], axis[1], axis[2], angle), g_fittedTurn), at);
            sim.f.squeeze = 0.0f;
            sim.Step();
            const bool grabbed = Grab(sim);
            trials.push_back({sim.fed, grabbed});
            pouch::Clear();
        }
    }
    {
        // It falls down the gun's y in the room (the head level, facing -z).
        const Pose room = Mul(FromXr(HeadAt(kNeck0, 0.0f, 0.0f)), GunPose());
        const float down[3] = {-room.m[1], -room.m[5], -room.m[9]};
        const float* v = p.dropVelocity;
        const float speed = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        const float along = v[0] * down[0] + v[1] * down[1] + v[2] * down[2];
        const bool pass = p.grabbed && p.hidden >= 20 && std::fabs(p.blends[0] - 0.70f) < 1e-4f && p.blends[1] == 1.0f &&
                          p.blends[2] == 1.0f && p.drawnOff <= kPullDead + 1e-4f && p.pulledOut == 1 &&
                          p.pulledOutAt >= -kMouth + kPullDead - kEps && p.pulledOutAt <= -kMouth + kPullDead + 0.25f * kDt + kEps &&
                          p.letGo == 1 && speed > 0.20f && speed < 0.26f && along >= 0.99f * speed;
        Report("16a",
               "(more) the left hand takes the seated magazine and draws it down at 0.25 m/s: the gun's own hidden, the one in the hand "
               "drawn on the line 6 mm behind it, the glove eased onto it; free past the mouth; let go 20 cm down, it falls at the hand's speed",
               pass, false,
               "taken %s; hidden %d frames, the glove's blend %.2f %.2f %.2f, the drawn one at most %.1f mm from the hand's; drawn out %d "
               "with the hand %.1f cm down; let go %d, falling at %.3f m/s (%.3f down the gun)",
               YesNo(p.grabbed), p.hidden, p.blends[0], p.blends[1], p.blends[2], p.drawnOff * 1000, p.pulledOut,
               p.pulledOutAt * 100, p.letGo, speed, along);
    }
    Report("16b", "(more) taken, then the hand goes 4 cm up instead: the squeeze is the handguard's (toGrip), the magazine stays in",
           p.upToGrip == 1 && !p.upHolding, false, "toGrip %d, holding after %s", p.upToGrip, YesNo(p.upHolding));
    Report("16c", "(more) taken and let go of at once (0.3): it stays in, nothing drawn or dropped", p.earlyIn, false, "stays in %s",
           YesNo(p.earlyIn));
    Report("16d", "(more) drawn 3 cm, then the hand 25 cm aside: it stays in, the squeeze spent (not the handguard's either)",
           p.awayIn && p.awayToGrip == 0, false, "stays in %s, toGrip %d", YesNo(p.awayIn), p.awayToGrip);
    Report("16e", "(more) the gun's pose lost 5 frames while it's drawn down: kept, then drawn out; the hand lost 1.2 s after: gone after 1 s",
           p.lostKept && p.lostPulledOut == 1 && p.lostCancelled == 1 && std::fabs(p.lostCancelledAfter - 1.0f) < 0.03f, false,
           "kept through the loss %s; drawn out %d; gone %d, %.3f s after the hand was lost", YesNo(p.lostKept), p.lostPulledOut,
           p.lostCancelled, p.lostCancelledAfter);
    Report("16f",
           "(more) the palm's way at the magazine's middle: up there refused (the handguard's); up but 4 cm low, and as fitted, taken; 5 cm "
           "forward of it, or 9.5 cm aside, refused",
           p.palm[0] && p.palm[1] && p.palm[2] && p.palm[3] && p.palm[4], false,
           "as they should be: up %s, up and low %s, fitted %s, forward %s, aside %s", YesNo(p.palm[0]), YesNo(p.palm[1]),
           YesNo(p.palm[2]), YesNo(p.palm[3]), YesNo(p.palm[4]));
    {
        int taken = 0;
        for (const SeatTrial& t : g_seatTrials[g_mirror ? 1 : 0])
            taken += t.grabbed;
        Report("16g", "(more) 2000 gloves turned any way, their hand's middle within 15 cm of the magazine's middle, a fresh squeeze each (m8 checks each against OnSeated)",
               taken > 0 && taken < 2000, true, "%d taken", taken);
    }
}

void RunScenarios() {
    Scenario1();
    Scenario2();
    Scenario3();
    Scenario4();
    Scenario5();
    Scenario6();
    Scenario7();
    Scenario8();
    Scenario9();
    Scenario10();
    Scenario11();
    Scenario12();
    Scenario13();
    Scenario14();
    Scenario15();
    Scenario16();
}

// ---- m1-m10: either hand (free hands, 2026-11-06) ----

// m1: every path above (1-16) again as its mirror image, the right hand's (Feed with g_mirror): the same events in the same frames,
// every pose and number the left's mirrored, every check of them passing (counted apart).
void M1() {
    const size_t n0 = g_results[0].size();
    const int counted0 = g_counted;
    g_mirror = true;
    RunScenarios();
    g_mirror = false;
    const size_t n1 = g_results[1].size();
    int eventsOff = 0, sidesOff = 0, notFinite = 0;
    long firstOff = -1;
    float poseOff = 0.0f, blendOff = 0.0f, otherOff = 0.0f;
    auto most = [&](float& m, float a, float b) {
        if (!std::isfinite(a) || !std::isfinite(b))
            ++notFinite;
        else
            m = std::max(m, std::fabs(a - b));
    };
    for (size_t i = 0; i < std::min(n0, n1); ++i) {
        const pouch::Result &a = g_results[0][i], &b = g_results[1][i];
        const bool same = a.mag == b.mag && a.hideSeated == b.hideSeated && a.holding == b.holding && a.atPouch == b.atPouch &&
                          a.reached == b.reached && a.took == b.took && a.grabbed == b.grabbed && a.toGrip == b.toGrip &&
                          a.pulledOut == b.pulledOut && a.caught == b.caught && a.offLine == b.offLine && a.seated == b.seated &&
                          a.letGo == b.letGo && a.putBack == b.putBack && a.cancelled == b.cancelled && a.squeezed == b.squeezed &&
                          std::strcmp(a.note, b.note) == 0;
        if (!same && eventsOff++ == 0)
            firstOff = static_cast<long>(i);
        sidesOff += a.side != 0 || b.side != 1 || g_fedSides[0][i] != 0 || g_fedSides[1][i] != 1;
        for (int k = 0; k < 12; ++k) {
            most(poseOff, a.magPose[k], b.magPose[k]);
            most(poseOff, a.glove[k], b.glove[k]);
        }
        most(blendOff, a.gloveBlend, b.gloveBlend);
        for (int k = 0; k < 3; ++k) {
            most(otherOff, a.dropVelocity[k], b.dropVelocity[k]);
            most(otherOff, a.fromMiddle[k], b.fromMiddle[k]);
        }
        most(otherOff, a.size, b.size);
    }
    const bool pass = n0 > 0 && n1 == n0 && eventsOff == 0 && sidesOff == 0 && notFinite == 0 && poseOff <= 1e-5f && blendOff <= 1e-6f &&
                      otherOff <= 1e-5f && g_mirrorCounted == counted0 && g_mirrorFailed == 0;
    Report("m1",
           "every path above (1-16) again as the right hand's mirror image (side 1: the head, the gun, the glove and the grip point "
           "mirrored across x = 0): the same events in the same frames, every pose the left's mirrored, every check passing",
           pass, false,
           "%zu frames, the left's %zu; the events differ in %d (first at frame %ld); magPose and glove at most %.1e apart, the glove's "
           "blend %.1e, the drop's speed and the squeeze's place %.1e; a side wrong in %d, a number not finite in %d; the mirrored run's "
           "checks: %d of %d failed (the left's %d)%s",
           n1, n0, eventsOff, firstOff, poseOff, blendOff, otherOff, sidesOff, notFinite, g_mirrorFailed, g_mirrorCounted, counted0,
           g_mirrorFailedIds.c_str());
}

// m2: the right hand's own numbers on the seated magazine (its hand's middle -0.5363 1.0976 0.2092 in the gloves' bind space, its palm's
// way x negated). In the mirrored run (16a, 16f) it takes it as the left does and refuses it palm up at its middle. On the gun as it is:
// each glove holding it (the right's the left's mirrored about the gun's middle plane) takes it as its own side, not as the other (that
// side's hand's middle is 1.07 m off in the bind space); palm up at its middle or 4 cm above it refused (the handguard's), 4 cm low taken;
// fed through Update with its glove from GloveFor of its side, each takes it.
void M2() {
    const SeatedPaths& right = g_seatedPaths[1];
    const bool mirrored = right.grabbed && right.pulledOut == 1 && right.palm[0] && right.palm[1] && right.palm[2] && right.palm[3] &&
                          right.palm[4];
    const Pose gun = GunPose();
    // That side's glove on the gun turned as the left's turn (the right's mirrored about the gun's middle plane), its hand's middle at `at`
    // (the right's at at mirrored).
    auto gloveAt = [&](const Pose& turn, const float* at, int side) {
        Pose g = turn;
        float o[3];
        Apply(turn, kHandInGloveL, o);
        g.m[3] = at[0] - o[0], g.m[7] = at[1] - o[1], g.m[11] = at[2] - o[2];
        return Mul(gun, side == 1 ? Mirrored(g) : g);
    };
    const float up[3] = {0.0f, 1.0f, 0.0f};
    const float above[3] = {kGrab[0], kGrab[1] + 0.04f, kGrab[2]}, low[3] = {kGrab[0], kGrab[1] - 0.04f, kGrab[2]};
    int wrong = 0;
    std::string which;
    for (int s = 0; s < 2; ++s) {
        struct Case {
            const char* name;
            Pose glove;
            int side;
            bool take;
        } const cases[] = {
            {"fitted", gloveAt(g_fittedTurn, kGrab, s), s, true},
            {"fitted, as the other side", gloveAt(g_fittedTurn, kGrab, s), 1 - s, false},
            {"palm up at its middle", gloveAt(PalmTo(up), kGrab, s), s, false},
            {"palm up 4 cm above", gloveAt(PalmTo(up), above, s), s, false},
            {"palm up 4 cm low", gloveAt(PalmTo(up), low, s), s, true},
        };
        for (const Case& c : cases)
            if (pouch::OnSeated(gun.m, c.glove.m, c.side) != c.take) {
                ++wrong;
                which += std::string(s == 1 ? " (right) " : " (left) ") + c.name;
            }
    }
    // The right's own hand's middle where the right glove's put.
    float middleOff = 0.0f;
    {
        const float handInGloveR[3] = {-kHandInGloveL[0], kHandInGloveL[1], kHandInGloveL[2]};
        float o[3];
        Apply(Mul(Inverse(gun), gloveAt(g_fittedTurn, kGrab, 1)), handInGloveR, o);
        middleOff = Distance(o, kGrab);
    }
    bool updateTakes[2] = {};
    for (int s = 0; s < 2; ++s) {
        Sim sim;
        SeatedFresh(sim, "m2", s);
        updateTakes[s] = Grab(sim) && pouch::HoldingSide() == s && sim.r.side == s;
    }
    Report("m2",
           "the right hand's own numbers on the seated magazine (its hand's middle -0.5363 1.0976 0.2092 in the bind space, its palm's way "
           "x negated): taken as the left's is; palm up at its middle refused (the handguard's)",
           mirrored && wrong == 0 && middleOff < 1e-5f && updateTakes[0] && updateTakes[1], false,
           "the mirrored run: taken %s, drawn out %d, the palm's 5 cases as the left's %s; on the gun as it is: OnSeated wrong in %d of "
           "10%s (the right's middle %.1e m from where it's put); through Update, its glove from GloveFor of its side: the left takes it "
           "%s, the right %s",
           YesNo(right.grabbed), right.pulledOut,
           YesNo(right.palm[0] && right.palm[1] && right.palm[2] && right.palm[3] && right.palm[4]), wrong, which.c_str(), middleOff,
           YesNo(updateTakes[0]), YesNo(updateTakes[1]));
}

// m3: a change of hand with nothing held: the new hand's squeeze, already held when it's first fed, isn't fresh until it's been under 0.4
// (nor at 0.5, then 0.7); then a fresh one takes a magazine, that hand's. The last hand's crossing (squeezed 5 cm in front of the belly)
// doesn't reach the belly with the new hand in it the next frame (the same hand would: taken). The old rows ran on side 0 from the first
// frame: g_fedSide starting at 0 changed nothing for them.
void M3() {
    float middle[3], front[3];
    MiddleFor(kNeck0, 0.0f, middle);
    PouchPoint(kNeck0, 0.0f, kBelly[0], kBelly[1], kBelly[2] + kBellyRadii[2] + 0.05f, front);
    int heldTook[2] = {}, partTook[2] = {}, crossedTook[2] = {};
    bool tookAfter[2] = {}, sameTook[2] = {};
    for (int s = 0; s < 2; ++s) {
        const int o = 1 - s;
        {
            Sim sim;
            Fresh(sim, "m3", s);
            sim.AtGrip(middle, Upright());
            sim.f.squeeze = 0.0f;
            Still(sim, 3 * kDt);
            sim.f.side = o;  // the other hand, at the belly, its squeeze already held
            sim.AtGrip(middle, Upright());
            sim.f.squeeze = 1.0f;
            Still(sim, 10 * kDt);
            heldTook[s] = sim.took;
            sim.f.squeeze = 0.5f;
            Still(sim, 3 * kDt);
            sim.f.squeeze = 0.7f;
            Still(sim, 3 * kDt);
            partTook[s] = sim.took - heldTook[s];
            sim.f.squeeze = 0.3f;
            sim.Step();
            sim.f.squeeze = 1.0f;
            sim.Step();
            tookAfter[s] = sim.r.took && sim.took == 1 && sim.r.side == o && pouch::HoldingSide() == o;
        }
        for (int change = 0; change < 2; ++change) {
            Sim sim;
            Fresh(sim, "m3", s);
            sim.AtGrip(front, Upright());
            sim.f.squeeze = 0.0f;
            Still(sim, 0.1f);
            sim.f.squeeze = 1.0f;
            sim.Step();
            if (change)
                sim.f.side = o;
            sim.AtGrip(middle, Upright());
            Still(sim, 0.3f);
            if (change)
                crossedTook[s] = sim.took;
            else
                sameTook[s] = sim.took == 1;
        }
    }
    const bool pass = heldTook[0] == 0 && heldTook[1] == 0 && partTook[0] == 0 && partTook[1] == 0 && tookAfter[0] && tookAfter[1] &&
                      crossedTook[0] == 0 && crossedTook[1] == 0 && sameTook[0] && sameTook[1];
    Report("m3",
           "a change of hand with nothing held: the new hand's squeeze held as it's first fed isn't fresh until it's been under 0.4; the "
           "last hand's crossing doesn't carry over",
           pass, false,
           "left then right / right then left: took with the squeeze held %d / %d, at 0.5 then 0.7 %d / %d, after it opened %s / %s; the "
           "last hand's crossing, the new hand at the belly: took %d / %d (the same hand: %s / %s)",
           heldTook[0], heldTook[1], partTook[0], partTook[1], YesNo(tookAfter[0]), YesNo(tookAfter[1]), crossedTook[0],
           crossedTook[1], YesNo(sameTook[0]), YesNo(sameTook[1]));
}

// The hand from the belly out to its own side at the waist over 0.5 s, then swung along way (its side's: x mirrored for the right) at
// speed m/s for 5 frames; the 6th frame swung is left to the caller (its direction in `dir`, unit).
void SwingOut(Sim& sim, const float* middle, float speed, float* p, float* dir) {
    const int s = sim.f.side;
    float air[3];
    PouchPoint(kNeck0, 0.0f, s == 1 ? 0.40f : -0.40f, -0.35f, 0.29f, air);
    for (int i = 1, n = Frames(0.5f); i <= n; ++i) {
        const float u = Ease(static_cast<float>(i) / n);
        for (int c = 0; c < 3; ++c)
            p[c] = middle[c] + (air[c] - middle[c]) * u;
        sim.AtGrip(p, Upright());
        sim.Step();
    }
    const float way[3] = {s == 1 ? 0.8f : -0.8f, 0.36f, -0.48f};
    const float n = std::sqrt(way[0] * way[0] + way[1] * way[1] + way[2] * way[2]);
    for (int c = 0; c < 3; ++c)
        dir[c] = way[c] / n;
    for (int i = 0; i < 6; ++i) {
        for (int c = 0; c < 3; ++c)
            p[c] += dir[c] * speed * kDt;
        sim.AtGrip(p, Upright());
        if (i < 5)
            sim.Step();
    }
}

// How a let go magazine falls against the hand's swing: its speed (m/s) and how much of it is along dir.
void DropAlong(const pouch::Result& r, const float* dir, float& speed, float& along) {
    const float* v = r.dropVelocity;
    speed = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    along = v[0] * dir[0] + v[1] * dir[1] + v[2] * dir[2];
}

// m4: a hand that can't keep its magazine (Frame::dropHeld: the rifle has left the hands, another hand works it): let go of, never
// cancelled, each hand. a: in the hand mid-swing (1.5, 5 m/s): it falls at the hand's speed over its last three frames (at most 3 m/s);
// b: on the line 3 cm before home: it falls, not seated, the glove easing off the line; c: on the seated one drawn 3 cm: it stays in, the
// glove easing off from where it's drawn; d: in the hand with the hand and the gun lost: it falls (not kept, not gone); e: at the belly: back.
void M4() {
    float middle[3];
    MiddleFor(kNeck0, 0.0f, middle);
    {
        bool ok[2][2] = {};
        float speeds[2][2] = {}, alongs[2][2] = {};
        for (int s = 0; s < 2; ++s)
            for (int k = 0; k < 2; ++k) {
                const float speed = k == 0 ? 1.5f : 5.0f;
                Sim sim;
                Fresh(sim, "m4a", s);
                const bool took = TakeAt(sim, middle);
                float p[3], dir[3];
                SwingOut(sim, middle, speed, p, dir);
                sim.f.dropHeld = true;  // the squeeze still held
                sim.Step();
                const pouch::Result r = sim.r;
                DropAlong(r, dir, speeds[s][k], alongs[s][k]);
                const float want = std::min(speed, 3.0f);
                ok[s][k] = took && r.letGo && !r.putBack && !r.cancelled && !r.holding && !r.mag && !pouch::Holding() && r.side == s &&
                           std::fabs(speeds[s][k] - want) <= 0.01f * want && alongs[s][k] >= 0.999f * speeds[s][k];
            }
        Report("m4a", "dropHeld with the magazine in the hand mid-swing: it falls at the hand's speed over its last three frames (at most 3 m/s), never cancelled",
               ok[0][0] && ok[0][1] && ok[1][0] && ok[1][1], false,
               "left %s %s, right %s %s: swung at 1.5 m/s it falls at %.3f / %.3f m/s, at 5 m/s %.3f / %.3f (want 1.5 and 3, along the swing)",
               YesNo(ok[0][0]), YesNo(ok[0][1]), YesNo(ok[1][0]), YesNo(ok[1][1]), speeds[0][0], speeds[1][0], speeds[0][1], speeds[1][1]);
    }
    {
        bool ok[2] = {};
        float blends[2][2] = {};
        for (int s = 0; s < 2; ++s) {
            Sim sim;
            Fresh(sim, "m4b", s);
            TakeHere(sim);
            Slide(sim, Pose(), -0.20f, -0.03f, 0.3f);
            Still(sim, 3 * kDt);
            const bool on = sim.line;
            sim.f.dropHeld = true;
            sim.Step();
            const pouch::Result r = sim.r;
            sim.f.dropHeld = false;
            sim.Step();
            blends[s][0] = r.gloveBlend, blends[s][1] = sim.r.gloveBlend;
            ok[s] = on && r.letGo && !r.seated && !r.putBack && !r.cancelled && !r.holding && !pouch::Holding() && r.side == s &&
                    sim.seated == 0 && r.gloveBlend == 1.0f && std::fabs(sim.r.gloveBlend - 0.65f) < 1e-5f;
        }
        Report("m4b", "dropHeld on the line 3 cm before home: it falls, not seated; the glove eases off the line from where it's drawn",
               ok[0] && ok[1], false, "left %s, right %s; the glove's blend that frame and the next %.2f %.2f / %.2f %.2f", YesNo(ok[0]),
               YesNo(ok[1]), blends[0][0], blends[0][1], blends[1][0], blends[1][1]);
    }
    {
        bool ok[2] = {};
        float blends[2][2] = {};
        for (int s = 0; s < 2; ++s) {
            Sim sim;
            SeatedFresh(sim, "m4c", s);
            const bool took = Grab(sim);
            Draw(sim, 0.0f, 0.0f, 0.0f, -0.03f, 0.25f);
            const bool drawn = sim.r.hideSeated && sim.r.holding;
            float drawnGlove[12];
            std::memcpy(drawnGlove, sim.r.glove, sizeof(drawnGlove));
            sim.f.dropHeld = true;
            sim.Step();
            const pouch::Result r = sim.r;
            float off = 0.0f;
            for (int k = 0; k < 12; ++k)
                off = std::max(off, std::fabs(r.glove[k] - drawnGlove[k]));
            sim.f.dropHeld = false;
            sim.Step();
            blends[s][0] = r.gloveBlend, blends[s][1] = sim.r.gloveBlend;
            ok[s] = took && drawn && !r.pulledOut && !r.letGo && !r.putBack && !r.cancelled && !r.hideSeated && !r.mag && !r.holding &&
                    !pouch::Holding() && r.side == s && r.gloveBlend == 1.0f && off < 1e-5f && std::fabs(sim.r.gloveBlend - 0.65f) < 1e-5f &&
                    !sim.r.pulledOut && !sim.r.letGo;
        }
        Report("m4c", "dropHeld on the seated one drawn 3 cm: it stays in (not drawn out, not dropped); the glove eases off from where it's drawn",
               ok[0] && ok[1], false, "left %s, right %s; the glove's blend that frame and the next %.2f %.2f / %.2f %.2f", YesNo(ok[0]),
               YesNo(ok[1]), blends[0][0], blends[0][1], blends[1][0], blends[1][1]);
    }
    {
        bool ok[2] = {};
        for (int s = 0; s < 2; ++s) {
            Sim sim;
            Fresh(sim, "m4d", s);
            TakeHere(sim);
            Still(sim, 0.1f);
            sim.f.left = false;
            Still(sim, 5 * kDt);
            const bool kept = pouch::Holding();
            sim.f.gunTracked = false;
            sim.zeroGun = true;
            sim.f.dropHeld = true;
            sim.Step();
            const pouch::Result r = sim.r;
            ok[s] = kept && r.letGo && !r.cancelled && !r.putBack && !pouch::Holding() && r.side == s && sim.cancelled == 0;
        }
        Report("m4d", "dropHeld with the magazine in the hand, the hand lost 5 frames and the gun's pose missing: it falls (not kept, not gone)",
               ok[0] && ok[1], false, "left %s, right %s", YesNo(ok[0]), YesNo(ok[1]));
    }
    {
        bool ok[2] = {};
        for (int s = 0; s < 2; ++s) {
            Sim sim;
            Fresh(sim, "m4e", s);
            const bool took = TakeAt(sim, middle);
            Still(sim, 0.2f);
            sim.f.dropHeld = true;
            sim.Step();
            const pouch::Result r = sim.r;
            ok[s] = took && r.putBack && !r.letGo && !r.cancelled && !pouch::Holding() && r.side == s;
        }
        Report("m4e", "dropHeld with the magazine in the hand at the belly: back", ok[0] && ok[1], false, "left %s, right %s", YesNo(ok[0]),
               YesNo(ok[1]));
    }
}

// m5: GloveFor by side: the right's is magPose . Mirror(inv(hold)), the two-argument one the left's exactly; the right holding a magazine
// (Update fed side 1, its glove from GloveFor side 1) draws it where it is, as the left does; SetHold makes the right's hold again.
void M5() {
    float hold[12];
    pouch::GetHold(hold);
    const Pose rightInverse = Mirrored(Inverse(FromArray(hold)));
    const Pose poses[3] = {Pose(), GunPose(), Mul(GunPose(), Mul(Move(0.02f, -0.15f, 0.01f), Turn(1, 2, 3, 25.0f)))};
    float gloveOff = 0.0f;
    bool twoSame = true;
    for (const Pose& m : poses) {
        float right[12], left[12], left0[12];
        pouch::GloveFor(m.m, right, 1);
        pouch::GloveFor(m.m, left);
        pouch::GloveFor(m.m, left0, 0);
        const Pose want = Mul(m, rightInverse);
        for (int k = 0; k < 12; ++k)
            gloveOff = std::max(gloveOff, std::fabs(right[k] - want.m[k]));
        twoSame = twoSame && std::memcmp(left, left0, sizeof(left)) == 0;
    }
    float backOff[2] = {};
    for (int s = 0; s < 2; ++s) {
        Sim sim;
        Fresh(sim, "m5", s);
        TakeHere(sim);
        const Pose places[3] = {Move(0.0f, -0.20f, 0.0f), Mul(Move(0.05f, -0.30f, 0.02f), Turn(0, 1, 0, 30.0f)),
                                Mul(Move(-0.04f, -0.25f, 0.10f), Turn(1, 1, 0, 50.0f))};
        for (const Pose& e : places) {
            sim.InGun(e);
            for (int i = 0; i < 3; ++i) {
                sim.Step();
                if (!sim.r.mag || sim.r.caught || sim.r.letGo) {
                    backOff[s] = 1.0f;
                    continue;
                }
                for (int k = 0; k < 12; ++k)
                    backOff[s] = std::max(backOff[s], std::fabs(sim.r.magPose[k] - sim.mag.m[k]));
            }
        }
    }
    // SetHold with the hold turned 10 degrees about the magazine's top: the right's made again as its mirror image; then put back.
    float setOff = 0.0f;
    {
        const Pose turned = Mul(FromArray(hold), TurnAbout(0, 1, 0, 10.0f, kTop));
        pouch::SetHold(turned.m);
        float now[12], right[12];
        pouch::GetHold(now);
        pouch::GloveFor(GunPose().m, right, 1);
        const Pose want = Mul(GunPose(), Mirrored(Inverse(FromArray(now))));
        for (int k = 0; k < 12; ++k)
            setOff = std::max(setOff, std::fabs(right[k] - want.m[k]));
        pouch::SetHold(hold);
    }
    float back[12], restored = 0.0f;
    pouch::GetHold(back);
    for (int k = 0; k < 12; ++k)
        restored = std::max(restored, std::fabs(back[k] - hold[k]));
    Report("m5",
           "GloveFor by side: the right's magPose . Mirror(inv(hold)), the two-argument one the left's; a magazine held in either hand "
           "drawn where it is; SetHold makes the right's hold again",
           gloveOff <= 1e-6f && twoSame && backOff[0] <= 1e-6f && backOff[1] <= 1e-6f && setOff <= 1e-6f, false,
           "the right's glove %.1e from magPose . Mirror(inv(hold)); the two-argument one the left's exactly %s; held still at three places "
           "the drawn magazine %.1e (left) / %.1e (right) from the one in the hand; after SetHold %.1e (the hold put back to %.1e)",
           gloveOff, YesNo(twoSame), backOff[0], backOff[1], setOff, restored);
}

// m6: HoldingSide through a take at the belly, the line and the seat, then the seated one taken, drawn out and let go of, each hand: its
// side while holding (Holding()), -1 otherwise; Result::side that hand's throughout.
void M6() {
    bool ok[2] = {};
    int wrongs[2] = {};
    for (int s = 0; s < 2; ++s) {
        Sim sim;
        Fresh(sim, "m6", s);
        int wrong = 0;
        auto check = [&] { wrong += pouch::HoldingSide() != (pouch::Holding() ? s : -1) || sim.r.side != s; };
        const bool none0 = pouch::HoldingSide() == -1;
        sim.f.forceInside = true;
        sim.f.squeeze = 0.0f;
        Still(sim, 3 * kDt, check);
        sim.f.squeeze = 1.0f;
        sim.Step();
        check();
        const bool tookSide = sim.r.took && pouch::HoldingSide() == s;
        sim.f.forceInside = false;
        bool lineSide = false, seatedNone = false;
        Slide(sim, Pose(), -0.20f, 0.01f, 0.3f, [&] {
            check();
            if (sim.line)
                lineSide = pouch::HoldingSide() == s;
            if (sim.r.seated) {
                sim.f.wellEmpty = false;
                seatedNone = pouch::HoldingSide() == -1;
            }
        });
        Still(sim, 0.2f, check);
        sim.InGun(Pose());  // the hand back on the seated one, opened
        sim.f.squeeze = 0.0f;
        Still(sim, 3 * kDt, check);
        const bool grabbedSide = Grab(sim) && pouch::HoldingSide() == s;
        check();
        bool outSide = false;
        Draw(sim, 0.0f, 0.0f, 0.0f, -0.15f, 0.25f, [&] {
            check();
            if (sim.r.pulledOut) {
                sim.f.wellEmpty = true;
                outSide = pouch::HoldingSide() == s;
            }
        });
        sim.f.squeeze = 0.3f;
        sim.Step();
        check();
        const bool letGoNone = sim.r.letGo && pouch::HoldingSide() == -1;
        wrongs[s] = wrong;
        ok[s] = wrong == 0 && none0 && tookSide && lineSide && seatedNone && grabbedSide && outSide && letGoNone;
    }
    Report("m6", "HoldingSide through a take, the line, the seat, the seated one taken, drawn out and let go of: that hand's while held, -1 otherwise",
           ok[0] && ok[1], false, "left %s, right %s; frames wrong %d / %d", YesNo(ok[0]), YesNo(ok[1]), wrongs[0], wrongs[1]);
}

// m7: the other hand fed while one has a magazine (each hand first): a: in the hand mid-swing, it falls at that hand's speed (Result::side
// the holder's that frame), and the new hand's held squeeze takes nothing until it's been open; b: still at the belly, back; c: on the line,
// it falls; d: on the seated one drawn 3 cm, it stays in, the glove straight back at the hand.
void M7() {
    float middle[3];
    MiddleFor(kNeck0, 0.0f, middle);
    {
        bool ok[2] = {};
        float speeds[2] = {}, alongs[2] = {};
        int tookHeld[2] = {};
        for (int s = 0; s < 2; ++s) {
            const int o = 1 - s;
            Sim sim;
            Fresh(sim, "m7a", s);
            const bool took = TakeAt(sim, middle);
            float p[3], dir[3];
            SwingOut(sim, middle, 1.5f, p, dir);
            sim.f.side = o;  // the other hand, at the belly, its squeeze held
            sim.AtGrip(middle, Upright());
            sim.Step();
            const pouch::Result r = sim.r;
            DropAlong(r, dir, speeds[s], alongs[s]);
            Still(sim, 10 * kDt);
            tookHeld[s] = sim.took - 1;
            sim.f.squeeze = 0.3f;
            sim.Step();
            sim.f.squeeze = 1.0f;
            sim.Step();
            ok[s] = took && r.letGo && !r.putBack && !r.cancelled && r.side == s && std::fabs(speeds[s] - 1.5f) <= 0.015f &&
                    alongs[s] >= 0.999f * speeds[s] && tookHeld[s] == 0 && sim.r.took && sim.r.side == o && pouch::HoldingSide() == o;
        }
        Report("m7a", "the other hand fed while one has a magazine mid-swing: it falls at the holder's speed; the new hand's held squeeze takes nothing until it's been open",
               ok[0] && ok[1], false, "left first %s, right first %s; falling at %.3f / %.3f m/s (1.5 swung); taken while the squeeze was held %d / %d",
               YesNo(ok[0]), YesNo(ok[1]), speeds[0], speeds[1], tookHeld[0], tookHeld[1]);
    }
    {
        bool ok[2] = {};
        for (int s = 0; s < 2; ++s) {
            Sim sim;
            Fresh(sim, "m7b", s);
            const bool took = TakeAt(sim, middle);
            Still(sim, 0.2f);
            float air[3];
            PouchPoint(kNeck0, 0.0f, s == 1 ? -0.40f : 0.40f, -0.35f, 0.29f, air);  // the other hand out at its own side
            sim.f.side = 1 - s;
            sim.AtGrip(air, Upright());
            sim.Step();
            ok[s] = took && sim.r.putBack && !sim.r.letGo && !sim.r.cancelled && sim.r.side == s && !pouch::Holding();
        }
        Report("m7b", "... while the holder is still at the belly: back (where it was last seen)", ok[0] && ok[1], false, "left first %s, right first %s",
               YesNo(ok[0]), YesNo(ok[1]));
    }
    {
        bool ok[2] = {};
        for (int s = 0; s < 2; ++s) {
            Sim sim;
            Fresh(sim, "m7c", s);
            TakeHere(sim);
            Slide(sim, Pose(), -0.20f, -0.03f, 0.3f);
            Still(sim, 3 * kDt);
            const bool on = sim.line;
            sim.f.side = 1 - s;
            sim.Step();
            ok[s] = on && sim.r.letGo && !sim.r.seated && !sim.r.cancelled && sim.r.side == s && !pouch::Holding() && sim.r.gloveBlend == 0.0f;
        }
        Report("m7c", "... on the line: it falls, not seated", ok[0] && ok[1], false, "left first %s, right first %s", YesNo(ok[0]),
               YesNo(ok[1]));
    }
    {
        bool ok[2] = {};
        for (int s = 0; s < 2; ++s) {
            Sim sim;
            SeatedFresh(sim, "m7d", s);
            const bool took = Grab(sim);
            Draw(sim, 0.0f, 0.0f, 0.0f, -0.03f, 0.25f);
            const bool drawn = sim.r.hideSeated;
            sim.f.side = 1 - s;
            sim.Step();
            const pouch::Result r = sim.r;
            sim.Step();
            ok[s] = took && drawn && !r.pulledOut && !r.letGo && !r.cancelled && !r.hideSeated && !r.mag && r.gloveBlend == 0.0f &&
                    r.side == s && !pouch::Holding() && sim.r.side == 1 - s && sim.r.gloveBlend == 0.0f && !sim.r.grabbed;
        }
        Report("m7d", "... on the seated one drawn 3 cm: it stays in, the glove straight back at the hand", ok[0] && ok[1], false,
               "left first %s, right first %s", YesNo(ok[0]), YesNo(ok[1]));
    }
    {
        bool ok[2] = {};
        for (int s = 0; s < 2; ++s) {
            Sim sim;
            Fresh(sim, "m7e", s);
            TakeHere(sim);
            Still(sim, 0.1f);
            sim.f.side = 1 - s;
            sim.f.gun = false;  // the weapon switched that frame too
            sim.Step();
            ok[s] = sim.r.cancelled && !sim.r.letGo && !sim.r.putBack && sim.r.side == s && !pouch::Holding();
        }
        Report("m7e", "(more) ... and the gun gone the same frame: gone, as with the same hand (not dropped)", ok[0] && ok[1], false,
               "left first %s, right first %s", YesNo(ok[0]), YesNo(ok[1]));
    }
}

// m8: OnSeated agrees with Update's own grab for each of 16g's 2000 gloves, both runs (the left's, and the right's in the mirrored run);
// a glove and gun mirrored, as side 1, give what the original gives as side 0.
void M8() {
    int disagree[2] = {}, taken[2] = {}, mirrorDisagree = 0;
    for (int run = 0; run < 2; ++run)
        for (const SeatTrial& t : g_seatTrials[run]) {
            taken[run] += t.grabbed;
            disagree[run] += pouch::OnSeated(t.fed.gunPose, t.fed.glove, t.fed.side) != t.grabbed || t.fed.side != run;
        }
    for (const SeatTrial& t : g_seatTrials[0]) {
        float gun[12], glove[12];
        std::memcpy(gun, t.fed.gunPose, sizeof(gun));
        std::memcpy(glove, t.fed.glove, sizeof(glove));
        MirrorRows(gun);
        MirrorRows(glove);
        mirrorDisagree += pouch::OnSeated(gun, glove, 1) != pouch::OnSeated(t.fed.gunPose, t.fed.glove, 0);
    }
    const bool pass = g_seatTrials[0].size() == 2000 && g_seatTrials[1].size() == 2000 && disagree[0] == 0 && disagree[1] == 0 &&
                      mirrorDisagree == 0 && taken[0] == taken[1] && taken[0] > 0 && taken[0] < 2000;
    Report("m8", "OnSeated agrees with Update's own grab for 2000 gloves within 15 cm of the seated magazine, both hands; mirrored, side 1 gives side 0's",
           pass, false, "taken %d (left) / %d (right) of %zu; OnSeated disagreeing %d / %d; mirrored and side 1 against side 0: %d differ",
           taken[0], taken[1], g_seatTrials[0].size(), disagree[0], disagree[1], mirrorDisagree);
}

// m9: the other hand fed while the glove eases back after a seat: no ease (gloveBlend 0, no glove) and Result::side the new hand's from
// that frame; fed the same hand, the ease runs on (0.65, 0.30, 0) as that hand's.
void M9() {
    bool ok[2][2] = {};
    float blends[2][2][3] = {};
    for (int s = 0; s < 2; ++s)
        for (int change = 0; change < 2; ++change) {
            Sim sim;
            Fresh(sim, "m9", s);
            TakeHere(sim);
            float at = -0.20f;
            for (int i = 0; i < 200 && !sim.r.seated; ++i) {
                at = std::min(at + 0.3f * kDt, 0.01f);
                sim.InGun(Move(0.0f, at, 0.0f));
                sim.Step();
            }
            const bool seated = sim.r.seated && sim.r.gloveBlend == 1.0f;
            sim.f.wellEmpty = false;
            const int want = change ? 1 - s : s;
            if (change)
                sim.f.side = want;
            int sideWrong = 0;
            float glove = 0.0f;
            for (int i = 0; i < 3; ++i) {
                sim.Step();
                blends[s][change][i] = sim.r.gloveBlend;
                sideWrong += sim.r.side != want;
                for (const float v : sim.r.glove)
                    glove = std::max(glove, std::fabs(v));
            }
            const float* b = blends[s][change];
            ok[s][change] = seated && sideWrong == 0 &&
                            (change ? b[0] == 0.0f && b[1] == 0.0f && b[2] == 0.0f && glove == 0.0f
                                    : std::fabs(b[0] - 0.65f) < 1e-5f && std::fabs(b[1] - 0.30f) < 1e-5f && b[2] == 0.0f && glove > 0.0f);
        }
    Report("m9", "the other hand fed while the glove eases back after a seat: the ease stops and Result::side is the new hand's; the same hand's eases on",
           ok[0][0] && ok[0][1] && ok[1][0] && ok[1][1], false,
           "left %s / %s, right %s / %s (the same hand / the other); the blends after the seat: left %.2f %.2f %.2f / %.2f %.2f %.2f, right %.2f %.2f %.2f / %.2f %.2f %.2f",
           YesNo(ok[0][0]), YesNo(ok[0][1]), YesNo(ok[1][0]), YesNo(ok[1][1]), blends[0][0][0], blends[0][0][1], blends[0][0][2],
           blends[0][1][0], blends[0][1][1], blends[0][1][2], blends[1][0][0], blends[1][0][1], blends[1][0][2], blends[1][1][0],
           blends[1][1][1], blends[1][1][2]);
}

// m10: dropHeld on the seated one drawn 3 cm with the gun's pose missing (gunTracked false, the pose zeros): it stays in, no glove drawn
// and gloveBlend 0 that frame and the next (straight back at the hand); with the gun tracked it eases off (m4c).
void M10() {
    bool ok[2] = {};
    for (int s = 0; s < 2; ++s) {
        Sim sim;
        SeatedFresh(sim, "m10", s);
        const bool took = Grab(sim);
        Draw(sim, 0.0f, 0.0f, 0.0f, -0.03f, 0.25f);
        const bool drawn = sim.r.hideSeated && sim.r.gloveBlend > 0.0f;
        sim.f.gunTracked = false;
        sim.zeroGun = true;
        sim.f.dropHeld = true;
        sim.Step();
        const pouch::Result r = sim.r;
        float glove = 0.0f;
        for (const float v : r.glove)
            glove = std::max(glove, std::fabs(v));
        sim.f.dropHeld = false;
        sim.f.gunTracked = true;
        sim.zeroGun = false;
        sim.Step();
        ok[s] = took && drawn && r.gloveBlend == 0.0f && glove == 0.0f && !r.pulledOut && !r.letGo && !r.cancelled && !r.hideSeated &&
                !r.mag && !pouch::Holding() && r.side == s && sim.r.gloveBlend == 0.0f && !sim.r.grabbed && !sim.r.pulledOut;
    }
    Report("m10", "dropHeld on the seated one with the gun's pose missing: it stays in, no glove drawn and gloveBlend 0, then and after",
           ok[0] && ok[1], false, "left %s, right %s", YesNo(ok[0]), YesNo(ok[1]));
}

}  // namespace

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "-v") == 0 && i + 1 < argc)
            g_verbose = argv[++i];
        else if (std::strcmp(argv[i], "-trace") == 0 && i + 1 < argc)
            g_trace = std::fopen(argv[++i], "w");
    pouch::SetZone(nullptr, true);
    {
        float glove[12];
        const Pose seated{};
        pouch::GloveFor(seated.m, glove);
        g_gloveOfSeated = FromArray(glove);
        Apply(Inverse(g_gloveOfSeated), kHandMiddle, g_handInGlove);
        g_fittedTurn = g_gloveOfSeated;
        g_fittedTurn.m[3] = g_fittedTurn.m[7] = g_fittedTurn.m[11] = 0.0f;
    }
    float middle[3];
    MiddleFor(kNeck0, 0.0f, middle);
    {
        const Pose gun = GunPose();
        float top[3], view[3];
        Apply(gun, kTop, view);
        const Pose head = FromXr(HeadAt(kNeck0, 0.0f, 0.0f));
        Apply(head, view, top);
        std::printf("magtest: src/pouch.cpp at 72 Hz. The eyes at (0 1.6 0), the neck at (%.3f %.3f %.3f), the hand at "
                    "the belly (the zone's middle) at (%.3f %.3f %.3f); the gun's magazine home at (%.3f %.3f %.3f) in the room; the hand's "
                    "middle in the gloves' bind space (%.4f %.4f %.4f), the glove's origin %.2f m from it.\n",
                    kNeck0[0], kNeck0[1], kNeck0[2], middle[0], middle[1], middle[2], top[0], top[1], top[2],
                    g_handInGlove[0], g_handInGlove[1], g_handInGlove[2],
                    std::sqrt(g_handInGlove[0] * g_handInGlove[0] + g_handInGlove[1] * g_handInGlove[1] +
                              g_handInGlove[2] * g_handInGlove[2]));
    }
    RunScenarios();
    // Either hand: m1 first (it runs all of the above again, mirrored, from the module as they left it).
    M1();
    M2();
    M3();
    M4();
    M5();
    M6();
    M7();
    M8();
    M9();
    M10();
    std::printf("%d of %d checks failed (%d of them in 1-9, the ones asked for; %d in m1-m10, either hand); %d info checks marked *\n",
                g_failed, g_counted, g_failedAsked, g_failedEither, g_infoOff);
    if (g_trace)
        std::fclose(g_trace);
    return g_failed;
}

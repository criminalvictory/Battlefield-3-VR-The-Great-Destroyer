// The forearm placed from its hand (src/armik.h, as hands.cpp runs it for the left glove), without
// the game: the bind pose, the whole arm moved rigidly, the game's own rifle hold from the mesh dumps,
// the twist sweep, the wrist's readings and range, degenerate poses, the time a solve takes, and the
// stand-in's test hooks. The gloves' bind space: x to the left, y up, z forward (metres). Checks 4-8
// pose the left arm in a view's space (x right, y up, z back), as hands.cpp does.
//   1  the bind pose, the pole along the bind elbow's own way: the frames move points near the arm
//      0.5 mm at most (each arm)
//   2  500 random rigid moves of the whole arm (hand, shoulder and body axes): the frames follow
//      within 0.5 mm (each arm)
//   3  the game's own rifle hold (build\mesh_*_bones.bin: the body's and the gloves' depth passes'
//      bones and the sleeves' chest; skipped without them): from the game's shoulder, hand and chest
//      axes, the left forearm within 1 degree of the game's and its twist within 1 degree of the
//      game's -99.2 (the hand against its forearm bone, about the elbow-wrist line); the right
//      forearm within 3 degrees
//   4  the hand rolled 1 degree a frame about its forearm, 400 frames: twist bone B turns 0.7
//      degrees a frame at most, apart from the one flip back after the hold at 150 degrees
//   5  the hand bent about its wrist joint (flexion, then ulnar deviation, -80 to 80 degrees): read
//      within 0.5 degrees of the bend, held at the range's limits (flexion 70, extension 65, ulnar
//      40, radial 25) and only there
//   6  degenerate poses (the wrist on the shoulder, far out of reach): finite frames
//   7  a solve takes under 5 us
//   8  the stand-in's tests (hands.cpp, build\arms.txt "test move -0.25 0.05 0.35", "test neutral",
//      "test bend"): the wrist in reach and bent at the elbow; each bend read as given within 0.5
//      degrees, the forearm staying put (its elbow end within 1 mm)
//   9  the arm never stretches ("Mr. Elastic", the user, 2026-11-04): 20000 random hands, near, out of
//      reach (1.08 m as in their picture) and turned past the wrist's range: the upper arm's far end
//      never further from the elbow than the bone is long, it's where the upper arm's frame puts the
//      bind shoulder whenever it's that far, the forearm's own length kept, the wrist on the hand;
//      in reach and in range, the far end is the shoulder given
//   10 the elbow's reach limit (Params::elbowReach, for the bare forearms, drawn without an upper arm: Semper Fidelis's train, 2026-11-18, the left forearm a thin
//      torn flap: the wrist's range had turned the forearm by the hand alone and its open elbow end sat out in front of the eye):
//      a  off (0, as every gloved solve): 20000 random hands each arm, with the gloves' numbers and the bare arms', the state carried on: every number of the result
//         the very same as the solver before it gave (tools\armtest_old.inc), bit for bit
//      b  on (0.05, the bare arms' numbers): 20000 random hands each arm, the wrist in reach: the elbow end never further than 0.05 from the triangle's own elbow
//         (so never further than the upper arm's length + 0.05 from the shoulder), also where the limit off leaves it inside the upper arm's reach of the shoulder
//         (the review, 2026-11-18: measured from the shoulder alone those were let be); the forearm its own length, the wrist on the hand, the frames rigid, nothing
//         not finite; a hand in range solved as with the limit off
//      c  the log's three bad left wrists (bent -22/+58 at rest, -18/-44 in the reload, -30/-43 at the bolt handle: each "range met"), the hand anywhere in reach
//         and turned up to 90 degrees past the range: with the limit off the elbow end goes more than twice the upper arm's length from the shoulder; with it on,
//         never beyond the limit. (Said with them, not checked: how often the open elbow end is ahead of the face and turned to the eye, with the limit off, on,
//         and for the arm on its triangle. An arm on its own line can show its open end too: that is the cap's, weapon.txt "barecap", not this limit's.)
//      d  the rest picture rebuilt (the stand-in's left hand at its rest place, the head 50 degrees down): the wrist reads as the log's line; with the limit off the
//         elbow end is out ahead of the eye, far off the shoulder. The hand there is beyond the arm's reach: with the limit on the forearm is back by the arm's line
//         (its elbow end within 0.05 of the straight arm's), and its open end (still ahead of the face: there is no upper arm to bring the shoulder along) is capped
//         whole (armik::EndCap, CapFrame: the rim's bones drawn in on the elbow joint)
//      e  the cap is none while the wrist is within the arm's reach of the shoulder, wherever the elbow end is (The Great Destroyer's arms as they were), and whole
//         from 5 cm beyond it
//      f  the review's experiment: 90000 random hands in reach in front of the body, the wrist bent up to 130 degrees any way, the head level, 35 and 50 degrees down:
//         the open elbow end in view where the natural elbow is not, with the limit off and on (a tenth of the off's at most); no hand in reach capped at all
//   11 the bare arm whole, its sleeve on it (the user, 2026-11-18: the sleeves, shoulders and upper body on the bare arms' levels; hands.cpp PlaceBareForearms keeps
//      the solver's upper arm and builds the collar, armik::Collar): 20000 random hands each arm with the bare arms' numbers and the reach limit, near and far out
//      of reach: the upper arm and the forearm keep their lengths and meet at the elbow, every bone rigid; the collar is the chest's axes, whole (never drawn in:
//      the film, 2026-11-18, the shoulder's cloth a pointed tail), its shoulder joint on the upper arm's far end whenever the shoulder is brought along; the sleeve
//      is on only up to 12 cm of that (barehold::SleeveStays: what the cloth takes, tools\baresleevetest.cpp s7), back on within 10, and an arm without it is
//      capped whole; an elbow end nearer the shoulder than the bone is long leaves the bone's end past the shoulder by the reach limit at most
// Usage: armtest [dump folder]   (the mesh dumps' folder; default E:\BF3 VR PROJECT\build)
// The exit code is the number of failed checks.

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <random>
#include <string>
#include <vector>

#include "../src/armik.h"
#include "../src/barehold.h"
#include "armtest_old.inc"

namespace {

constexpr double kPiD = 3.14159265358979;

double Deg(double radians) {
    return radians * 180 / kPiD;
}

// ---- Points and rigid poses in double (3x4, rows: rotation | translation) ----
struct V3 {
    double x = 0, y = 0, z = 0;
    V3() = default;
    V3(double a, double b, double c) : x(a), y(b), z(c) {}
    V3 operator+(const V3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    V3 operator-(const V3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    V3 operator*(double s) const { return {x * s, y * s, z * s}; }
    double operator[](int i) const { return i == 0 ? x : i == 1 ? y : z; }
};
double Dot(const V3& a, const V3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
V3 Cross(const V3& a, const V3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
double Len(const V3& a) {
    return std::sqrt(Dot(a, a));
}
V3 Unit(const V3& a) {
    const double l = Len(a);
    return l > 1e-15 ? a * (1.0 / l) : a;
}
double AngleDeg(const V3& a, const V3& b) {
    return Deg(std::acos(std::clamp(Dot(Unit(a), Unit(b)), -1.0, 1.0)));
}
V3 FromF(const float* f) {
    return {f[0], f[1], f[2]};
}

struct M34 {
    double m[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    V3 Apply(const V3& p) const {
        return {m[0] * p.x + m[1] * p.y + m[2] * p.z + m[3], m[4] * p.x + m[5] * p.y + m[6] * p.z + m[7],
                m[8] * p.x + m[9] * p.y + m[10] * p.z + m[11]};
    }
    V3 Rot(const V3& p) const {
        return {m[0] * p.x + m[1] * p.y + m[2] * p.z, m[4] * p.x + m[5] * p.y + m[6] * p.z,
                m[8] * p.x + m[9] * p.y + m[10] * p.z};
    }
};
M34 Mul(const M34& a, const M34& b) {
    M34 o;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j)
            o.m[i * 4 + j] = a.m[i * 4] * b.m[j] + a.m[i * 4 + 1] * b.m[4 + j] + a.m[i * 4 + 2] * b.m[8 + j];
        o.m[i * 4 + 3] += a.m[i * 4 + 3];
    }
    return o;
}
// A turn of radians about the unit axis through pivot.
M34 RotAbout(const V3& axis, double angle, const V3& pivot) {
    const double c = std::cos(angle), s = std::sin(angle), t = 1 - c, x = axis.x, y = axis.y, z = axis.z;
    const double r[9] = {t * x * x + c,     t * x * y - s * z, t * x * z + s * y, t * x * y + s * z, t * y * y + c,
                         t * y * z - s * x, t * x * z - s * y, t * y * z + s * x, t * z * z + c};
    M34 o;
    for (int i = 0; i < 3; ++i) {
        o.m[i * 4] = r[i * 3], o.m[i * 4 + 1] = r[i * 3 + 1], o.m[i * 4 + 2] = r[i * 3 + 2];
        o.m[i * 4 + 3] = pivot[i] - (r[i * 3] * pivot.x + r[i * 3 + 1] * pivot.y + r[i * 3 + 2] * pivot.z);
    }
    return o;
}
void ToF(const M34& m, float* f) {
    for (int i = 0; i < 12; ++i)
        f[i] = static_cast<float>(m.m[i]);
}
M34 ToM(const float* f) {
    M34 m;
    for (int i = 0; i < 12; ++i)
        m.m[i] = f[i];
    return m;
}
// How far a 3x4's rotation turns from another's (degrees).
double TurnBetween(const M34& a, const M34& b) {
    double trace = 0;  // of a's rotation times b's transposed
    for (int i = 0; i < 3; ++i)
        for (int k = 0; k < 3; ++k)
            trace += a.m[i * 4 + k] * b.m[i * 4 + k];
    return Deg(std::acos(std::clamp((trace - 1) / 2, -1.0, 1.0)));
}

// The most two placements put points near the arm apart (mirrored for the right).
double MaxApart(const M34& a, const M34& b, bool right) {
    const V3 points[4] = {{0.37, 1.26, -0.01}, {0.51, 1.12, 0.16}, {0.44, 1.21, 0.06}, {0.30, 1.30, 0.10}};
    double most = 0;
    for (const V3& p0 : points) {
        const V3 p(right ? -p0.x : p0.x, p0.y, p0.z);
        most = std::max(most, Len(a.Apply(p) - b.Apply(p)));
    }
    return most;
}

V3 Joint(const float* joint, bool right) {
    return {right ? -joint[0] : joint[0], joint[1], joint[2]};
}

bool Finite(const armik::Result& r) {
    for (const auto& frame : r.frame)
        for (float v : frame)
            if (!std::isfinite(v))
                return false;
    for (int i = 0; i < 3; ++i)
        if (!std::isfinite(r.fold[i]) || !std::isfinite(r.elbow[i]))
            return false;
    return true;
}

// A draw's bones as the game drew it, from the mod's mesh dump (weapon.txt "dump"): 12 floats each.
std::vector<M34> LoadBones(const std::string& dir, int count, int start) {
    char name[64];
    std::snprintf(name, sizeof(name), "\\mesh_%d_%d_bones.bin", count, start);
    std::ifstream f(dir + name, std::ios::binary);
    const std::vector<char> bytes{std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
    std::vector<M34> bones(bytes.size() / 48);
    for (size_t k = 0; k < bones.size(); ++k) {
        float m[12];
        std::memcpy(m, &bytes[k * 48], sizeof(m));
        bones[k] = ToM(m);
    }
    return bones;
}

// ---- Reporting ----
int g_failed = 0, g_counted = 0, g_skipped = 0;

void Report(const char* id, const char* what, bool pass, const char* format, ...) {
    char numbers[1024];
    va_list args;
    va_start(args, format);
    std::vsnprintf(numbers, sizeof(numbers), format, args);
    va_end(args);
    std::printf("%-3s %-5s %s | %s\n", id, pass ? "PASS" : "FAIL", what, numbers);
    ++g_counted;
    g_failed += !pass;
}

void Skip(const char* id, const char* what, const char* why) {
    std::printf("%-3s %-5s %s | %s\n", id, "SKIP", what, why);
    ++g_skipped;
}

// ---- 1, 2: the bind pose and rigid moves ----
void BindAndRigid() {
    for (int side = 0; side < 2; ++side) {
        const bool right = side == 1;
        const double sx = right ? -1 : 1;
        const V3 S = Joint(armik::kShoulder, right), E = Joint(armik::kElbow, right), W = Joint(armik::kWrist, right);
        // The pole along the bind elbow's own way off the shoulder-wrist line, in the body's axes (out
        // to this arm's side: the bind space's x for the left; up y; back -z); no rising-hand term.
        const V3 D = Unit(W - S), pe = Unit((E - S) - D * Dot(E - S, D));
        armik::Params q;
        q.poleOut = static_cast<float>(pe.x * sx), q.poleDown = static_cast<float>(-pe.y),
        q.poleBack = static_cast<float>(-pe.z), q.riseOut = 0, q.riseDown = 0;
        const float out[3] = {static_cast<float>(sx), 0, 0}, up[3] = {0, 1, 0}, back[3] = {0, 0, -1};
        const float hand[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
        const float shoulder[3] = {static_cast<float>(S.x), static_cast<float>(S.y), static_cast<float>(S.z)};
        armik::State st;
        armik::Result r;
        armik::Solve(hand, shoulder, out, up, back, right, q, st, r);
        double worst = 0;
        for (const auto& frame : r.frame)
            worst = std::max(worst, MaxApart(ToM(frame), M34{}, right));
        Report(right ? "1R" : "1L", right ? "the right arm's bind pose" : "the left arm's bind pose", worst <= 0.0005,
               "the frames move points near the arm %.3f mm at most (<= 0.5); bend %.2f deg, twist %.2f deg "
               "(the pole: out %.2f down %.2f back %.2f)",
               worst * 1000, Deg(r.bend), Deg(r.twist), q.poleOut, q.poleDown, q.poleBack);
        // The whole arm turned and moved at random: the frames move with it.
        std::mt19937 g(7 + side);
        std::uniform_real_distribution<double> u(-1, 1);
        double moved = 0;
        for (int n = 0; n < 500; ++n) {
            const V3 axis = Unit(V3(u(g), u(g), u(g)));
            M34 shift;
            shift.m[3] = u(g), shift.m[7] = u(g), shift.m[11] = u(g);
            const M34 G = Mul(RotAbout(axis, u(g) * 3.1, V3()), shift);
            float gh[12], gs[3], go[3], gu[3], gb[3];
            ToF(G, gh);
            const V3 s2 = G.Apply(S), o2 = G.Rot(V3(sx, 0, 0)), u2 = G.Rot(V3(0, 1, 0)), b2 = G.Rot(V3(0, 0, -1));
            for (int i = 0; i < 3; ++i)
                gs[i] = static_cast<float>(s2[i]), go[i] = static_cast<float>(o2[i]), gu[i] = static_cast<float>(u2[i]),
                gb[i] = static_cast<float>(b2[i]);
            armik::State s3;
            armik::Result r2;
            armik::Solve(gh, gs, go, gu, gb, right, q, s3, r2);
            for (const auto& frame : r2.frame)
                moved = std::max(moved, MaxApart(ToM(frame), G, right));
        }
        Report(right ? "2R" : "2L", right ? "the right arm moved rigidly" : "the left arm moved rigidly", moved <= 0.0005,
               "500 random turns and shifts: the frames within %.3f mm of the move (<= 0.5)", moved * 1000);
    }
}

// ---- 3: the game's own rifle hold ----
void GameHold(const std::string& dir) {
    const std::vector<M34> chest = LoadBones(dir, 8529, 0), body = LoadBones(dir, 21189, 28806),
                           gloves = LoadBones(dir, 7617, 49995);
    if (chest.empty() || body.size() < 22 || gloves.size() < 19) {
        Skip("3", "the game's own rifle hold", "no mesh dumps (mesh_8529_0, mesh_21189_28806, mesh_7617_49995 bones)");
        return;
    }
    // The body's axes from the chest bone (the sleeves' draw's 'c'): left, up, forward.
    const V3 left = Unit(chest[0].Rot(V3(1, 0, 0))), up = Unit(chest[0].Rot(V3(0, 1, 0))),
             forward = Unit(chest[0].Rot(V3(0, 0, 1)));
    for (int side = 0; side < 2; ++side) {
        const bool right = side == 1;
        // The left arm is in the body's depth pass (upper arm 17, forearm 18, twist bones 19 and 20, the
        // hand 21: the glove's bone 1), the right in the gloves' (14 ... 18).
        const std::vector<M34>& P = right ? gloves : body;
        const int upper = right ? 14 : 17, fore = right ? 15 : 18, A = right ? 16 : 19, B = right ? 17 : 20,
                  hand = right ? 18 : 21;
        const V3 s = P[upper].Apply(Joint(armik::kShoulder, right)), out = right ? left * -1 : left, back = forward * -1;
        float h[12], sh[3], o[3], u[3], b[3];
        ToF(P[hand], h);
        for (int i = 0; i < 3; ++i)
            sh[i] = static_cast<float>(s[i]), o[i] = static_cast<float>(out[i]), u[i] = static_cast<float>(up[i]),
            b[i] = static_cast<float>(back[i]);
        armik::Params q;
        if (right)
            q.poleOut = 0.9f, q.ulnar = 55.0f;
        armik::State st;
        armik::Result r;
        armik::Solve(h, sh, o, u, b, right, q, st, r);
        const V3 f0 = Unit(Joint(armik::kWrist, right) - Joint(armik::kElbow, right));
        const double apart = AngleDeg(P[fore].Rot(f0), ToM(r.frame[0]).Rot(f0));
        const double elbow = Len(P[fore].Apply(Joint(armik::kElbow, right)) - ToM(r.frame[0]).Apply(Joint(armik::kElbow, right)));
        char frames[256];
        std::snprintf(frames, sizeof(frames), "; frames against the game's bones %d/%d/%d: %.1f/%.1f/%.1f mm at most", fore,
                      A, B, MaxApart(ToM(r.frame[0]), P[fore], right) * 1000, MaxApart(ToM(r.frame[1]), P[A], right) * 1000,
                      MaxApart(ToM(r.frame[2]), P[B], right) * 1000);
        if (!right) {
            constexpr double kGameTwist = -99.2;  // the game's left hand against its forearm bone (the dumps)
            const bool pass = apart <= 1.0 && std::fabs(Deg(r.twist) - kGameTwist) <= 1.0;
            Report("3L", "the game's left hold rebuilt", pass,
                   "the forearm %.2f deg from the game's (<= 1), its elbow end %.0f mm off; twist %.1f deg (the game's "
                   "%.1f, within 1); bend %.1f (flexion %+.1f, ulnar %+.1f)%s%s",
                   apart, elbow * 1000, Deg(r.twist), kGameTwist, Deg(r.bend), Deg(r.flexion), Deg(r.deviation),
                   r.clamped ? " clamped" : "", frames);
        } else {
            Report("3R", "the game's right hold rebuilt", apart <= 3.0,
                   "the forearm %.2f deg from the game's (<= 3), its elbow end %.0f mm off; twist %.1f deg (the game's "
                   "44.1); bend %.1f (flexion %+.1f, ulnar %+.1f)%s%s",
                   apart, elbow * 1000, Deg(r.twist), Deg(r.bend), Deg(r.flexion), Deg(r.deviation),
                   r.clamped ? " clamped" : "", frames);
        }
    }
}

// ---- 4-7 on one pose: the left arm in a view's space (x right, y up, z back), the shoulder out to
// the left of the neck, the wrist in front of the chest; the hand turned so its straight wrist lies
// along the solved forearm (worked out here in double, apart from armik::Neutral). ----
struct Pose {
    float out[3] = {-1, 0, 0}, up[3] = {0, 1, 0}, back[3] = {0, 0, 1};
    float shoulder[3] = {-0.164f, 1.42f, 0.08f};
    V3 wrist{-0.08, 1.30, -0.36};
    M34 base;  // the hand
};

Pose MakePose(const armik::Params& p) {
    Pose s;
    // The glove's bind axes (x left, y up, z forward) onto the view's (x right, y up, z back).
    const double turn[12] = {-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0};
    std::memcpy(s.base.m, turn, sizeof(turn));
    const V3 at = s.base.Apply(Joint(armik::kWrist, false));
    s.base.m[3] += s.wrist.x - at.x, s.base.m[7] += s.wrist.y - at.y, s.base.m[11] += s.wrist.z - at.z;
    const V3 f0 = Unit(Joint(armik::kWrist, false) - Joint(armik::kElbow, false));
    for (int round = 0; round < 6; ++round) {
        float h[12];
        ToF(s.base, h);
        armik::State st;
        armik::Result r;
        armik::Solve(h, s.shoulder, s.out, s.up, s.back, false, p, st, r);
        const V3 f1 = Unit(FromF(r.wrist) - FromF(r.elbow)), hf = Unit(s.base.Rot(f0)), axis = Cross(hf, f1);
        if (Len(axis) > 1e-12)
            s.base = Mul(RotAbout(Unit(axis), std::atan2(Len(axis), Dot(hf, f1)), s.wrist), s.base);
    }
    return s;
}

void Bends(const Pose& s, const armik::Params& p) {
    const V3 f0 = Unit(Joint(armik::kWrist, false) - Joint(armik::kElbow, false));
    V3 pa(armik::kPalm[0], armik::kPalm[1], armik::kPalm[2]), ax(armik::kAcross[0], armik::kAcross[1], armik::kAcross[2]);
    ax = Unit(ax - f0 * Dot(ax, f0));
    pa = Unit(pa - f0 * Dot(pa, f0) - ax * Dot(pa, ax));
    struct Kind {
        const char* id;
        const char* name;
        bool flexion;  // read as flexion (else ulnar deviation)
        V3 axis;
        double most, least;  // the range: up to most positive, least negative
    } kinds[] = {{"5a", "flexion", true, Cross(f0, pa), p.flex, p.ext},
                 {"5b", "ulnar deviation", false, Cross(f0, ax), p.ulnar, p.radial}};
    float h[12];
    ToF(s.base, h);
    armik::State st0;
    armik::Result r0;
    armik::Solve(h, s.shoulder, s.out, s.up, s.back, false, p, st0, r0);
    for (const Kind& k : kinds) {
        double worst = 0, worstOther = 0, movedFree = 0;
        int wrongHold = 0;
        std::string readings;
        for (double a : {-80.0, -60.0, -30.0, 0.0, 30.0, 60.0, 80.0}) {
            const M34 H = Mul(RotAbout(Unit(s.base.Rot(k.axis)), a * kPiD / 180, s.wrist), s.base);
            ToF(H, h);
            armik::State st;
            armik::Result r;
            armik::Solve(h, s.shoulder, s.out, s.up, s.back, false, p, st, r);
            const double want = std::clamp(a, -k.least, k.most);
            const bool held = a > k.most || a < -k.least;
            const double read = Deg(k.flexion ? r.flexion : r.deviation), other = Deg(k.flexion ? r.deviation : r.flexion);
            worst = std::max(worst, std::fabs(read - want));
            worstOther = std::max(worstOther, std::fabs(other));
            wrongHold += held != r.clamped;
            if (!held)
                movedFree = std::max(movedFree, Len(FromF(r.elbow) - FromF(r0.elbow)));
            char one[48];
            std::snprintf(one, sizeof(one), " %+.0f->%+.1f%s", a, read, r.clamped ? "h" : "");
            readings += one;
        }
        Report(k.id, k.name, worst <= 0.5 && worstOther <= 0.5 && wrongHold == 0,
               "read within %.2f deg of the bend held to its range (<= 0.5), the other way %.2f; %d held wrongly; the "
               "forearm's elbow end moved %.1f mm unheld; bent->read (h held):%s",
               worst, worstOther, wrongHold, movedFree * 1000, readings.c_str());
    }
}

void TwistSweep(const Pose& s, const armik::Params& p) {
    const V3 f0 = Unit(Joint(armik::kWrist, false) - Joint(armik::kElbow, false));
    armik::State st;
    double mostB = 0, last = 0;
    int heldFrom = -1, flips = 0, flipAt = -1;
    M34 lastB;
    for (int deg = 0; deg <= 400; ++deg) {
        const M34 H = Mul(RotAbout(Unit(s.base.Rot(f0)), deg * kPiD / 180, s.wrist), s.base);
        float h[12];
        ToF(H, h);
        armik::Result r;
        armik::Solve(h, s.shoulder, s.out, s.up, s.back, false, p, st, r);
        const double twist = Deg(r.twist);
        if (r.twistHeld && heldFrom < 0)
            heldFrom = deg;
        const bool flip = deg > 0 && std::fabs(twist - last) > 90;
        if (flip && flipAt < 0)
            flipAt = deg;
        flips += flip;
        const M34 B = ToM(r.frame[2]);
        if (deg > 0 && !flip)
            mostB = std::max(mostB, TurnBetween(B, lastB));
        lastB = B;
        last = twist;
    }
    Report("4", "the hand rolled about its forearm", mostB <= 0.7 && flips == 1 && heldFrom >= 0,
           "twist bone B turns %.3f deg a frame at most (<= 0.7) with the hand's 1; the twist held from %d deg of roll, "
           "%d flip%s back (at %d deg; one wanted)",
           mostB, heldFrom, flips, flips == 1 ? "" : "s", flipAt);
}

void Degenerate(const Pose& s, const armik::Params& p) {
    int bad = 0;
    std::string seen;
    const V3 Wb = Joint(armik::kWrist, false);
    for (const V3& w : {V3(-0.164, 1.42, 0.08), V3(-0.164, 1.425, 0.08), V3(-0.164, 0.6, 0.08), V3(-0.164, 2.3, 0.08),
                        V3(-0.164, 1.42, -1.2), V3(0.6, 1.4, -0.2)}) {
        M34 H = s.base;
        const V3 at = H.Apply(Wb);
        H.m[3] += w.x - at.x, H.m[7] += w.y - at.y, H.m[11] += w.z - at.z;
        float h[12];
        ToF(H, h);
        armik::State st;
        armik::Result r;
        for (int frame = 0; frame < 3; ++frame)
            armik::Solve(h, s.shoulder, s.out, s.up, s.back, false, p, st, r);
        bad += !Finite(r);
        char one[96];
        std::snprintf(one, sizeof(one), " (%.2f %.2f %.2f) %s%s", w.x, w.y, w.z, r.reach ? "in reach" : "out of reach",
                      Finite(r) ? "" : " NOT FINITE");
        seen += one;
    }
    Report("6", "degenerate poses", bad == 0, "%d of 6 not finite; wrists at%s", bad, seen.c_str());
}

void Cost(const Pose& s, const armik::Params& p) {
    LARGE_INTEGER frequency, a, b;
    QueryPerformanceFrequency(&frequency);
    float h[12];
    ToF(s.base, h);
    armik::State st;
    armik::Result r;
    volatile float sink = 0;
    constexpr int kSolves = 200000;
    QueryPerformanceCounter(&a);
    for (int n = 0; n < kSolves; ++n) {
        h[3] = -0.5f + (n % 100) * 0.002f;
        armik::Solve(h, s.shoulder, s.out, s.up, s.back, false, p, st, r);
        sink = sink + r.frame[2][3];
    }
    QueryPerformanceCounter(&b);
    const double us = static_cast<double>(b.QuadPart - a.QuadPart) * 1e6 / static_cast<double>(frequency.QuadPart) / kSolves;
    Report("7", "the time a solve takes", us < 5.0, "%.3f us a solve (< 5)", us);
}

// ---- 8: the stand-in's left hand (input.cpp: under the handguard, palm up, from the stand-in's
// aim 0.08 right, 0.25 down, 0.45 in front of the eyes) with build\grip.txt's line-up, the head
// level at the view's origin, as hands.cpp places it; then arms.txt's test lines. ----
void StandIn(const armik::Params& p) {
    // grip.txt's leftpose (2026-10-31): the glove on the left controller's grip pose.
    const double offset[12] = {-0.78243, -0.59231, -0.19229, 1.12604,  -0.50836, 0.78587,
                               -0.35215, -0.48218, 0.35969,  -0.17776, -0.91598, 0.22423};
    M34 lineUp;
    std::memcpy(lineUp.m, offset, sizeof(offset));
    const double half = -0.7853982;  // a quarter turn about the forward axis: palm up
    M34 controller = RotAbout(V3(0, 0, 1), 2 * half, V3());
    controller.m[3] = 0.08 - 0.04, controller.m[7] = -0.25 - 0.07, controller.m[11] = -0.45 - 0.30;
    M34 glove = Mul(controller, lineUp);
    glove.m[3] += -0.25, glove.m[7] += 0.05, glove.m[11] += 0.35;  // "test move -0.25 0.05 0.35"
    // The shoulder as hands.cpp estimates it: the neck 7.5 cm below and 8 cm behind the eyes, then 16.4
    // cm out, 16 cm down and 2 cm back; the body facing where the head does.
    const float out[3] = {-1, 0, 0}, up[3] = {0, 1, 0}, back[3] = {0, 0, 1};
    const float shoulder[3] = {-0.164f, -0.075f - 0.16f, 0.08f + 0.02f};
    float hand[12];
    ToF(glove, hand);
    armik::Neutral(hand, shoulder, out, up, back, false, p);  // "test neutral"
    armik::State st0;
    armik::Result r0;
    armik::Solve(hand, shoulder, out, up, back, false, p, st0, r0);
    const V3 s(shoulder[0], shoulder[1], shoulder[2]), w = FromF(r0.wrist), e = FromF(r0.elbow);
    const double reach = Len(w - s);
    // The elbow's bend (180 straight): from the two bones' lengths and the reach.
    const double a = armik::kUpper, b = armik::kFore, dc = std::clamp(reach, 0.0406, 0.5611);
    const double elbowBend = 180 - Deg(std::acos(std::clamp((a * a + b * b - dc * dc) / (2 * a * b), -1.0, 1.0)));
    Report("8a", "the stand-in's test hand neutral", r0.reach && elbowBend > 30 && Deg(r0.bend) < 0.5,
           "the wrist %.3f m from the shoulder (reach 0.561), the elbow bent %.0f deg (> 30); the wrist bent %.2f deg "
           "after \"test neutral\" (< 0.5); twist %+.1f",
           reach, elbowBend, Deg(r0.bend), Deg(r0.twist));
    // (The stand-in's hand is palm up: its twist is about +100 degrees at neutral, so a roll of +80 goes
    // past the hold at 150: 8c.)
    struct Test {
        float flex, dev, twist;
    } tests[] = {{30, 0, 0}, {40, 0, 0}, {-40, 0, 0}, {0, 30, 0}, {0, -20, 0}, {0, 0, -80}};
    double worst = 0, movedMost = 0;
    int held = 0;
    std::string readings;
    for (const Test& t : tests) {
        float bent[12];
        std::memcpy(bent, hand, sizeof(bent));
        armik::BendWrist(bent, false, t.flex, t.dev, t.twist);  // "test bend"
        armik::State st;
        armik::Result r;
        armik::Solve(bent, shoulder, out, up, back, false, p, st, r);
        const double twistWant = std::remainder(Deg(r0.twist) + t.twist, 360.0);
        worst = std::max({worst, std::fabs(Deg(r.flexion) - t.flex), std::fabs(Deg(r.deviation) - t.dev),
                          std::fabs(Deg(r.twist) - twistWant)});
        movedMost = std::max(movedMost, Len(FromF(r.elbow) - e));
        held += r.clamped || r.twistHeld;
        char one[96];
        std::snprintf(one, sizeof(one), " (%g %g %g)->(%+.1f %+.1f %+.1f)", t.flex, t.dev, t.twist, Deg(r.flexion),
                      Deg(r.deviation), Deg(r.twist) - Deg(r0.twist));
        readings += one;
    }
    Report("8b", "the stand-in's test bends", worst <= 0.5 && movedMost <= 0.001 && held == 0,
           "read within %.2f deg of the bends (<= 0.5), the forearm's elbow end moved %.2f mm at most (<= 1), %d held; "
           "bent (flexion ulnar twist)->read:%s",
           worst, movedMost * 1000, held, readings.c_str());
    // "test bend 0 0 80" as the game runs it, the state carried on from the neutral hand's frame: the
    // twist unwound past 180 and held at +150.
    {
        float bent[12];
        std::memcpy(bent, hand, sizeof(bent));
        armik::BendWrist(bent, false, 0, 0, 80);
        armik::State st;
        armik::Result r;
        armik::Solve(hand, shoulder, out, up, back, false, p, st, r);
        armik::Solve(bent, shoulder, out, up, back, false, p, st, r);
        const double most = Deg(p.maxTwist);
        Report("8c", "the stand-in's roll past the hold", r.twistHeld && std::fabs(Deg(r.twist) - most) <= 0.5,
               "neutral %+.1f deg rolled 80 more: twist %+.1f%s (held at %+.0f wanted)", Deg(r0.twist), Deg(r.twist),
               r.twistHeld ? " held" : "", most);
    }
}

// ---- 9: no stretching, whatever the hand does ----
void NeverStretches(const armik::Params& p) {
    for (int side = 0; side < 2; ++side) {
        const bool right = side == 1;
        const double sx = right ? 1 : -1;  // in a view's space (x right): out to this arm's side
        const float out[3] = {static_cast<float>(sx), 0, 0}, up[3] = {0, 1, 0}, back[3] = {0, 0, 1};
        const float shoulder[3] = {static_cast<float>(sx * 0.164), -0.235f, 0.10f};
        const V3 Sb = Joint(armik::kShoulder, right), Wb = Joint(armik::kWrist, right);
        std::mt19937 g(11 + side);
        std::uniform_real_distribution<double> u(-1, 1);
        double longest = 0, offFrame = 0, offForearm = 0, offWrist = 0, offGiven = 0;
        int beyond = 0, stretched = 0, bad = 0;
        armik::State st;
        for (int n = 0; n < 20000; ++n) {
            // Anywhere within 1.3 m of the eyes, turned any way: from one frame to the next, so the state's carried on as in play.
            const V3 at(u(g) * 1.3, u(g) * 1.3, u(g) * 1.3);
            M34 H = RotAbout(Unit(V3(u(g), u(g), u(g))), u(g) * 3.14, V3());
            const V3 w = H.Apply(Wb);
            H.m[3] += at.x - w.x, H.m[7] += at.y - w.y, H.m[11] += at.z - w.z;
            float h[12];
            ToF(H, h);
            armik::Result r;
            armik::Solve(h, shoulder, out, up, back, right, p, st, r);
            bad += !Finite(r);
            const V3 e = FromF(r.elbow), far3 = FromF(r.shoulder), given(shoulder[0], shoulder[1], shoulder[2]);
            const double upper = Len(far3 - e);
            longest = std::max(longest, upper - armik::kUpper);
            stretched += upper > armik::kUpper + 0.001;
            offForearm = std::max(offForearm, std::fabs(Len(FromF(r.wrist) - e) - armik::kFore));
            offWrist = std::max(offWrist, Len(ToM(r.frame[2]).Apply(Wb) - at));
            if (Len(given - e) > armik::kUpper + 1e-4) {
                ++beyond;
                offFrame = std::max(offFrame, Len(ToM(r.upper).Apply(Sb) - far3));
            } else {
                offGiven = std::max(offGiven, Len(far3 - given));
            }
        }
        Report(right ? "9R" : "9L", right ? "the right arm never stretches" : "the left arm never stretches",
               stretched == 0 && longest <= 0.001 && offFrame <= 0.001 && offForearm <= 0.001 && offWrist <= 0.001 && offGiven <= 1.5e-4 && bad == 0,  // (the 0.1 mm line above, in float)
               "20000 random hands (%d with the elbow past the bone's length from the shoulder): the bone %+.2f mm from its length at most (<= 1), %d stretched; the frame's end %.2f mm "
               "from the far end (<= 1); the forearm %.2f mm off its length (<= 1), the wrist %.2f mm off the hand (<= 1); within reach the far end %.3f mm from the shoulder given; %d not finite",
               beyond, longest * 1000, stretched, offFrame * 1000, offForearm * 1000, offWrist * 1000, offGiven * 1000, bad);
    }
    // Their picture's pose: the hold 1.08 m from the left shoulder.
    {
        const float out[3] = {-1, 0, 0}, up[3] = {0, 1, 0}, back[3] = {0, 0, 1};
        const float shoulder[3] = {-0.16f, -0.24f, 0.08f};
        const V3 Wb = Joint(armik::kWrist, false), at(0.01, 0.15, -0.91);
        M34 H = RotAbout(V3(0, 1, 0), kPiD, V3());
        const V3 w = H.Apply(Wb);
        H.m[3] += at.x - w.x, H.m[7] += at.y - w.y, H.m[11] += at.z - w.z;
        float h[12];
        ToF(H, h);
        armik::State st;
        armik::Result r;
        armik::Solve(h, shoulder, out, up, back, false, p, st, r);
        const double upper = Len(FromF(r.shoulder) - FromF(r.elbow)), moved = Len(FromF(r.shoulder) - V3(shoulder[0], shoulder[1], shoulder[2]));
        Report("9c", "the arm in the user's picture", upper <= armik::kUpper + 0.001 && moved > 0.4,
               "the hand %.2f m from the shoulder: the upper arm %.3f m (it's %.3f m), the shoulder brought along %.2f m", Len(FromF(r.wrist) - V3(shoulder[0], shoulder[1], shoulder[2])), upper, armik::kUpper, moved);
    }
}

// ---- 10: the elbow's reach limit ----
armik::Params BareParams(bool right) {  // hands.cpp's PlaceBareForearms: the wider wrist, the forearm taking the hand's roll (weapon.txt "bareshare 0.5 0.8 1")
    armik::Params p;
    if (right)
        p.poleOut = 0.9f;
    p.flex = 100.0f, p.ext = 95.0f, p.radial = 45.0f, p.ulnar = 60.0f;
    p.share[0] = 0.5f, p.share[1] = 0.8f, p.share[2] = 1.0f;
    return p;
}

armik_old::Params Old(const armik::Params& p) {
    armik_old::Params o;
    std::memcpy(o.share, p.share, sizeof(o.share));
    o.poleOut = p.poleOut, o.poleDown = p.poleDown, o.poleBack = p.poleBack, o.riseOut = p.riseOut, o.riseDown = p.riseDown;
    o.flex = p.flex, o.ext = p.ext, o.radial = p.radial, o.ulnar = p.ulnar, o.maxTwist = p.maxTwist, o.swivelStep = p.swivelStep;
    return o;
}

// How far a frame's rotation is from rigid: the worst entry of its rows' products against the identity.
double OffRigid(const float* m) {
    double worst = 0;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            double d = 0;
            for (int k = 0; k < 3; ++k)
                d += static_cast<double>(m[i * 4 + k]) * m[j * 4 + k];
            worst = std::max(worst, std::fabs(d - (i == j ? 1.0 : 0.0)));
        }
    return worst;
}

void ReachOff() {
    int differ = 0, solved = 0, clamped = 0;
    for (int set = 0; set < 2; ++set)
        for (int side = 0; side < 2; ++side) {
            const bool right = side == 1;
            armik::Params p = set ? BareParams(right) : armik::Params{};
            if (!set && right)
                p.poleOut = 0.9f, p.ulnar = 55.0f;
            const armik_old::Params po = Old(p);
            const double sx = right ? 1 : -1;
            const float out[3] = {static_cast<float>(sx), 0, 0}, up[3] = {0, 1, 0}, back[3] = {0, 0, 1};
            const float shoulder[3] = {static_cast<float>(sx * 0.164), -0.235f, 0.10f};
            const V3 Wb = Joint(armik::kWrist, right);
            std::mt19937 g(31 + side + set * 2);
            std::uniform_real_distribution<double> u(-1, 1);
            armik::State st;
            armik_old::State sto;
            for (int n = 0; n < 20000; ++n) {
                const V3 at(u(g) * 0.9, u(g) * 0.9, u(g) * 0.9);
                M34 H = RotAbout(Unit(V3(u(g), u(g), u(g))), u(g) * 3.14, V3());
                const V3 w = H.Apply(Wb);
                H.m[3] += at.x - w.x, H.m[7] += at.y - w.y, H.m[11] += at.z - w.z;
                float h[12];
                ToF(H, h);
                armik::Result r;
                armik_old::Result ro;
                armik::Solve(h, shoulder, out, up, back, right, p, st, r);
                armik_old::Solve(h, shoulder, out, up, back, right, po, sto, ro);
                ++solved;
                clamped += r.clamped;
                const bool same = !std::memcmp(r.frame, ro.frame, sizeof(r.frame)) && !std::memcmp(r.upper, ro.upper, sizeof(r.upper)) &&
                                  !std::memcmp(r.shoulder, ro.shoulder, sizeof(r.shoulder)) && !std::memcmp(r.fold, ro.fold, sizeof(r.fold)) &&
                                  !std::memcmp(r.elbow, ro.elbow, sizeof(r.elbow)) && !std::memcmp(r.wrist, ro.wrist, sizeof(r.wrist)) &&
                                  !std::memcmp(&r.bend, &ro.bend, sizeof(float)) && !std::memcmp(&r.flexion, &ro.flexion, sizeof(float)) &&
                                  !std::memcmp(&r.deviation, &ro.deviation, sizeof(float)) && !std::memcmp(&r.twist, &ro.twist, sizeof(float)) &&
                                  r.reach == ro.reach && r.clamped == ro.clamped && r.twistHeld == ro.twistHeld && !r.reachHeld &&
                                  !std::memcmp(st.pole, sto.pole, sizeof(st.pole)) && !std::memcmp(&st.twist, &sto.twist, sizeof(float)) &&
                                  !std::memcmp(&st.azimuth, &sto.azimuth, sizeof(float)) && st.have == sto.have;
                differ += !same;
            }
        }
    Report("10a", "the reach limit off: the solver as it was", differ == 0,
           "%d random hands (both arms, the gloves' numbers and the bare arms'; %d past the wrist's range): %d results not the old solver's bit for bit (0 wanted)", solved,
           clamped, differ);
}

void ReachOn() {
    constexpr float kReach = 0.05f;
    for (int side = 0; side < 2; ++side) {
        const bool right = side == 1;
        armik::Params p = BareParams(right), off = p;
        p.elbowReach = kReach;
        const double sx = right ? 1 : -1;
        const float out[3] = {static_cast<float>(sx), 0, 0}, up[3] = {0, 1, 0}, back[3] = {0, 0, 1};
        const float shoulder[3] = {static_cast<float>(sx * 0.164), -0.235f, 0.10f};
        const V3 Wb = Joint(armik::kWrist, right), S(shoulder[0], shoulder[1], shoulder[2]);
        std::mt19937 g(41 + side);
        std::uniform_real_distribution<double> u(-1, 1);
        double furthest = 0, furthestOff = 0, offForearm = 0, offWrist = 0, rigid = 0, stretch = 0, offNatural = 0, offNaturalOff = 0;
        int bad = 0, held = 0, beyondOff = 0, changedFree = 0, solved = 0, insideOff = 0, insideLeft = 0;
        armik::Params wide = off;
        wide.flex = wide.ext = wide.radial = wide.ulnar = 1.0e5f;  // (no range at all: the triangle's own forearm whatever the hand)
        armik::State st, sto, stw;
        while (solved < 20000) {
            const V3 at = S + V3(u(g), u(g), u(g)) * 0.56;
            if (Len(at - S) > armik::kUpper + armik::kFore - 0.002 || Len(at - S) < 0.06)
                continue;
            ++solved;
            M34 H = RotAbout(Unit(V3(u(g), u(g), u(g))), u(g) * 3.14, V3());
            const V3 w = H.Apply(Wb);
            H.m[3] += at.x - w.x, H.m[7] += at.y - w.y, H.m[11] += at.z - w.z;
            float h[12];
            ToF(H, h);
            armik::Result r, ro, rw;
            sto = stw = st;  // (from the same frame before: a forearm the limit turned back leaves another twist behind)
            armik::Solve(h, shoulder, out, up, back, right, p, st, r);
            armik::Solve(h, shoulder, out, up, back, right, off, sto, ro);
            armik::Solve(h, shoulder, out, up, back, right, wide, stw, rw);  // (the triangle's own: no range to meet)
            bad += !Finite(r);
            held += r.reachHeld;
            const double d = Len(FromF(r.elbow) - S), dOff = Len(FromF(ro.elbow) - S);
            const double apart = Len(FromF(r.elbow) - FromF(rw.elbow)), apartOff = Len(FromF(ro.elbow) - FromF(rw.elbow));
            furthest = std::max(furthest, d), furthestOff = std::max(furthestOff, dOff);
            offNatural = std::max(offNatural, apart), offNaturalOff = std::max(offNaturalOff, apartOff);
            beyondOff += dOff > armik::kUpper + kReach + 1e-4;
            // The limit off, the elbow end inside the upper arm's reach of the shoulder yet off the triangle's elbow: the ones a limit measured from the shoulder let be.
            if (dOff <= armik::kUpper && apartOff > kReach + 1e-4)
                ++insideOff, insideLeft += apart > kReach + 1e-4;
            offForearm = std::max(offForearm, std::fabs(Len(FromF(r.wrist) - FromF(r.elbow)) - armik::kFore));
            for (const auto& frame : r.frame) {
                offWrist = std::max(offWrist, Len(ToM(frame).Apply(Wb) - at));
                rigid = std::max(rigid, OffRigid(frame));
            }
            rigid = std::max(rigid, OffRigid(r.upper));
            stretch = std::max(stretch, Len(FromF(r.shoulder) - FromF(r.elbow)) - armik::kUpper);
            if (!ro.clamped)
                changedFree += std::memcmp(r.frame, ro.frame, sizeof(r.frame)) != 0 || r.reachHeld;
        }
        Report(right ? "10bR" : "10bL", right ? "the right elbow within reach of its shoulder" : "the left elbow within reach of its shoulder",
               furthest <= armik::kUpper + kReach + 1e-4 && offNatural <= kReach + 1e-4 && insideOff > 0 && insideLeft == 0 && offForearm <= 0.001 && offWrist <= 0.001 &&
                   rigid <= 1e-4 && stretch <= 0.001 && bad == 0 && changedFree == 0 && beyondOff > 0,
               "20000 random hands in reach: the elbow end %.3f m from the triangle's elbow at most (<= %.3f; with the limit off %.3f), %.3f m from the shoulder (<= %.3f; off "
               "%.3f, %d beyond); %d of the off's inside the upper arm's reach yet more than %.2f off the triangle's elbow, %d of them left there (0 wanted); %d turned back by "
               "the limit; the forearm "
               "%.3f mm off its length (<= 1), the wrist %.3f mm off the hand (<= 1), the upper arm %+.3f mm (<= 1), the frames %.1e off rigid (<= 1e-4); %d not finite; %d in "
               "range solved otherwise than with the limit off",
               offNatural, kReach, offNaturalOff, furthest, armik::kUpper + kReach, furthestOff, beyondOff, insideOff, kReach, insideLeft, held, offForearm * 1000,
               offWrist * 1000, stretch * 1000, rigid, bad, changedFree);
    }
}

// The log's bad frames (build\logs\bf3vr_1118_uncuffed.log, the left arm): the wrist as read each time the range was met.
void LoggedWrists() {
    constexpr float kReach = 0.05f;
    struct Logged {
        const char* id;
        const char* when;
        double flexion, ulnar;
    } const logged[] = {{"10c1", "at rest (10:32:30: -22, +58)", -22, 58}, {"10c2", "in the reload (10:34: -18, -44)", -18, -44}, {"10c3", "at the bolt handle (10:35:33: -30, -43)", -30, -43}};
    const bool right = false;
    armik::Params off = BareParams(right), on = off, wide = off;
    on.elbowReach = kReach;
    wide.flex = wide.ext = wide.radial = wide.ulnar = 179.0f;
    const float out[3] = {-1, 0, 0}, up[3] = {0, 1, 0}, back[3] = {0, 0, 1};
    const float shoulder[3] = {-0.164f, -0.235f, 0.10f};
    const V3 Wb = Joint(armik::kWrist, right), S(shoulder[0], shoulder[1], shoulder[2]), eye(0, 0, 0);
    const V3 f0 = Unit(Joint(armik::kWrist, right) - Joint(armik::kElbow, right));
    V3 pa(armik::kPalm[0], armik::kPalm[1], armik::kPalm[2]), ax(armik::kAcross[0], armik::kAcross[1], armik::kAcross[2]);
    ax = Unit(ax - f0 * Dot(ax, f0));
    pa = Unit(pa - f0 * Dot(pa, f0) - ax * Dot(pa, ax));
    // The open elbow end ahead of the face (within 45 degrees of straight ahead) and turned to the eye: the eye looks into the tube (within 35 degrees of its line).
    auto flap = [&](const armik::Result& r) {
        const V3 e = FromF(r.elbow), f1 = Unit(FromF(r.wrist) - e);
        return AngleDeg(e - eye, V3(0, 0, -1)) < 45.0 && AngleDeg(e - eye, f1) < 35.0;
    };
    for (const Logged& l : logged) {
        const double az = std::atan2(l.ulnar, l.flexion), range = std::hypot(l.flexion, l.ulnar);
        std::mt19937 g(53);
        std::uniform_real_distribution<double> u(-1, 1);
        double readOff = 0, furthestOff = 0, furthestOn = 0, offForearm = 0, rigid = 0, twistStep = 0;
        int solved = 0, beyondOff = 0, beyondOn = 0, flapsOff = 0, flapsOn = 0, flapsTriangle = 0, notClamped = 0, bad = 0;
        while (solved < 5000) {
            // The wrist in front of the body, in reach; the triangle's forearm there (the elbow's way doesn't depend on how the hand is turned).
            const V3 at = S + V3(u(g) * 0.45, u(g) * 0.45, -0.05 - (u(g) + 1) * 0.25);
            if (Len(at - S) > armik::kUpper + armik::kFore - 0.01 || Len(at - S) < 0.12)
                continue;
            ++solved;
            M34 H;
            {
                const V3 w = H.Apply(Wb);
                H.m[3] += at.x - w.x, H.m[7] += at.y - w.y, H.m[11] += at.z - w.z;
            }
            float h[12];
            ToF(H, h);
            armik::State s0;
            armik::Result r0;
            armik::Solve(h, shoulder, out, up, back, right, wide, s0, r0);
            const V3 ft = Unit(FromF(r0.wrist) - FromF(r0.elbow));
            // The hand turned so that forearm lies, in the hand's frame, the logged way off its straight wrist and 5 to 90 degrees past the range, rolled any way about it.
            const double bend = (range + 5 + (u(g) + 1) * 42.5) * kPiD / 180;
            const V3 v = Unit(f0 * std::cos(bend) + (pa * -std::cos(az) + ax * -std::sin(az)) * std::sin(bend));
            const V3 axis = Cross(v, ft);
            M34 R = Len(axis) > 1e-9 ? RotAbout(Unit(axis), std::atan2(Len(axis), Dot(v, ft)), V3()) : M34{};
            R = Mul(RotAbout(ft, u(g) * kPiD, V3()), R);
            const V3 w = R.Apply(Wb);
            R.m[3] += at.x - w.x, R.m[7] += at.y - w.y, R.m[11] += at.z - w.z;
            ToF(R, h);
            armik::State sa, sb;
            armik::Result ra, rb;
            armik::Solve(h, shoulder, out, up, back, right, off, sa, ra);
            armik::Solve(h, shoulder, out, up, back, right, on, sb, rb);
            notClamped += !ra.clamped;
            readOff = std::max({readOff, std::fabs(Deg(ra.flexion) - l.flexion), std::fabs(Deg(ra.deviation) - l.ulnar)});
            const double dOff = Len(FromF(ra.elbow) - S), dOn = Len(FromF(rb.elbow) - S);
            furthestOff = std::max(furthestOff, dOff), furthestOn = std::max(furthestOn, dOn);
            beyondOff += dOff > armik::kUpper + kReach + 1e-4, beyondOn += dOn > armik::kUpper + kReach + 1e-4;
            flapsOff += flap(ra), flapsOn += flap(rb), flapsTriangle += flap(r0);
            bad += !Finite(rb);
            offForearm = std::max(offForearm, std::fabs(Len(FromF(rb.wrist) - FromF(rb.elbow)) - armik::kFore));
            for (const auto& frame : rb.frame)
                rigid = std::max(rigid, OffRigid(frame));
            // The forearm's three frames next to each other: no more than the twist's shares apart (0.5, 0.8, 1 of at most 150 degrees: 45 between neighbours).
            twistStep = std::max({twistStep, TurnBetween(ToM(rb.frame[0]), ToM(rb.frame[1])), TurnBetween(ToM(rb.frame[1]), ToM(rb.frame[2]))});
        }
        char what[128];
        std::snprintf(what, sizeof(what), "the left wrist %s", l.when);
        Report(l.id, what,
               notClamped == 0 && readOff <= 0.5 && beyondOff > 0 && furthestOff > 2 * armik::kUpper && beyondOn == 0 && offForearm <= 0.001 && rigid <= 1e-4 &&
                   twistStep <= 45.5 && bad == 0,
               "5000 hands in reach, bent that way 5 to 90 degrees past the range (read as the log's within %.2f deg, %d not held). The limit off: the elbow end up to %.2f m from "
               "the shoulder (the bone %.2f), %d beyond the limit. On: %.3f m at most (<= %.3f), %d beyond; the forearm %.3f mm off its length, its frames %.1e off rigid, "
               "neighbours %.1f deg apart at most (<= 45.5); %d not finite. (The open end ahead of the face and to the eye: %d off, %d on, %d on the triangle.)",
               readOff, notClamped, furthestOff, armik::kUpper, beyondOff, furthestOn, armik::kUpper + kReach, beyondOn, offForearm * 1000, rigid, twistStep, bad, flapsOff,
               flapsOn, flapsTriangle);
    }
}

// The rest picture itself (build\survey\unc_01_rest_p50_*.jpg, 10:32:30): the stand-in's left hand at its rest place (input.cpp: 4 cm left, 7 cm under and 30 cm ahead of
// the aim, itself 8 cm right, 25 cm down and 45 cm ahead of the eyes; palm up) with build\grip.txt's line-up as it was that day, the headless head looking 50 degrees
// down (the hand goes with the head; the body's up and back are the room's), the shoulder as hands.cpp estimates it. The view's space: the eye at the origin.
void RestPicture() {
    const double offset[12] = {-0.49005, -0.83220, -0.25941, 1.23704, -0.69263, 0.55246, -0.46376, -0.14398, 0.52923, -0.04759, -0.84714, -0.03144};
    M34 lineUp;
    std::memcpy(lineUp.m, offset, sizeof(offset));
    M34 controller = RotAbout(V3(0, 0, 1), -kPiD / 2, V3());
    controller.m[3] = 0.08 - 0.04, controller.m[7] = -0.25 - 0.07, controller.m[11] = -0.45 - 0.30;
    const M34 glove = Mul(controller, lineUp);
    const double down = 50 * kPiD / 180;
    const float out[3] = {-1, 0, 0}, up[3] = {0, static_cast<float>(std::cos(down)), static_cast<float>(std::sin(down))},
                back[3] = {0, static_cast<float>(-std::sin(down)), static_cast<float>(std::cos(down))};
    float shoulder[3];
    const float neck[3] = {0.0f, -0.075f, 0.08f};
    for (int i = 0; i < 3; ++i)
        shoulder[i] = neck[i] + out[i] * 0.164f - up[i] * 0.16f + back[i] * 0.02f;
    float hand[12];
    ToF(glove, hand);
    armik::Params off = BareParams(false), on = off;
    on.elbowReach = 0.05f;
    armik::State sa, sb;
    armik::Result ra, rb;
    armik::Solve(hand, shoulder, out, up, back, false, off, sa, ra);
    armik::Solve(hand, shoulder, out, up, back, false, on, sb, rb);
    const V3 S(shoulder[0], shoulder[1], shoulder[2]), eye(0, 0, 0);
    auto ahead = [&](const armik::Result& r) { return AngleDeg(FromF(r.elbow) - eye, V3(0, 0, -1)); };
    auto intoEnd = [&](const armik::Result& r) { return AngleDeg(FromF(r.elbow) - eye, Unit(FromF(r.wrist) - FromF(r.elbow))); };
    const double dOff = Len(FromF(ra.elbow) - S), dOn = Len(FromF(rb.elbow) - S), reach = Len(FromF(rb.wrist) - S);
    // The hand is beyond the arm's reach there: the limit takes the forearm back by the arm's line, its elbow end within 0.05 of the straight arm's (the arm straight at
    // the shoulder: as far from it as the hand is, less the forearm). An upper arm would bring the shoulder along; the bare forearm has none, and its open end, ahead
    // of the face, gets its cap.
    const double offNatural = Len(FromF(rb.elbow) - (FromF(rb.wrist) - Unit(FromF(rb.wrist) - S) * armik::kFore));
    const double straight = AngleDeg(FromF(rb.wrist) - FromF(rb.elbow), FromF(rb.wrist) - S), capOff = armik::EndCap(ra, shoulder), capOn = armik::EndCap(rb, shoulder);
    // The cap: the rim's bones drawn in on the elbow joint: points up to 9 cm past the elbow (the mesh's open edges) land within 3 mm of it; no cap, where they were.
    double capFar = 0, capNone = 0;
    {
        float capped[12], same[12];
        armik::CapFrame(rb.frame[0], false, static_cast<float>(capOn), capped);
        armik::CapFrame(rb.frame[0], false, 0.0f, same);
        const V3 E = Joint(armik::kElbow, false), along = Unit(Joint(armik::kElbow, false) - Joint(armik::kWrist, false));
        for (const V3& p : {E, E + along * 0.09, E + V3(0.04, 0, 0), E + V3(0, 0.04, 0.03) + along * 0.05}) {
            capFar = std::max(capFar, Len(ToM(capped).Apply(p) - FromF(rb.elbow)));
            capNone = std::max(capNone, Len(ToM(same).Apply(p) - ToM(rb.frame[0]).Apply(p)));
        }
    }
    // The log's line for this frame: bent -22 and +58, twist +15, the range met. (Within 6 degrees: the head's own place and the line-up are the day's, read back.)
    const bool asLogged = ra.clamped && std::fabs(Deg(ra.flexion) + 22) <= 6 && std::fabs(Deg(ra.deviation) - 58) <= 6 && std::fabs(Deg(ra.twist) - 15) <= 12;
    Report("10d", "the rest picture's left arm",
           asLogged && dOff > armik::kUpper + 0.05 && reach > armik::kUpper + armik::kFore && rb.reachHeld && offNatural <= 0.0501 && straight <= 11.1 &&
               Finite(rb) && capOff == 1.0 && capOn == 1.0 && capFar <= 0.003 && capNone <= 1e-6,
           "the stand-in's hand at rest, the head 50 degrees down: read %+.0f / %+.0f, twist %+.0f%s (the log: -22 / +58, +15, range met); the hand %.2f m from the shoulder, "
           "beyond the arm's %.2f. The limit off: the elbow end (%.2f %.2f %.2f) from the eye, %.2f m from the shoulder (the bone %.2f), %.0f deg off straight ahead, the eye "
           "%.0f deg off the tube's line. On: (%.2f %.2f %.2f), %.3f m from the shoulder, %.3f m from the straight arm's elbow end (<= 0.050), the forearm %.2f deg off the "
           "line to the shoulder (<= 11.1), the eye %.0f deg off its line: the open end capped %.0f%% (100 wanted), its rim within %.1f mm of the joint (<= 3); uncapped the rim moves %.4f mm",
           Deg(ra.flexion), Deg(ra.deviation), Deg(ra.twist), ra.clamped ? ", range met" : "", reach, armik::kUpper + armik::kFore, ra.elbow[0], ra.elbow[1], ra.elbow[2], dOff,
           armik::kUpper, ahead(ra), intoEnd(ra), rb.elbow[0], rb.elbow[1], rb.elbow[2], dOn, offNatural, straight, intoEnd(rb), capOn * 100, capFar * 1000, capNone * 1000);
    // The cap by how far the hand is beyond the arm's reach: none within it, wherever the elbow end is (a wrist past its range, the limit holding it: the review,
    // 2026-11-18, every pose the limit held was drawn capped whole), all of it 5 cm beyond.
    {
        armik::Result r = rb;
        double worstIn = 0;
        int steps = 0;
        float last = -1.0f;
        for (int mm = -100; mm <= 120; mm += 5) {
            float atEnds[2];
            for (int end = 0; end < 2; ++end) {  // the elbow end on the shoulder, and 60 cm off it: the cap is the wrist's reach alone
                for (int i = 0; i < 3; ++i) {
                    r.wrist[i] = shoulder[i] + (i == 1 ? -1.0f : 0.0f) * (armik::kUpper + armik::kFore + mm * 0.001f);
                    r.elbow[i] = shoulder[i] + (i == 2 ? -0.6f * end : 0.0f);
                }
                atEnds[end] = armik::EndCap(r, shoulder);
            }
            const float cap = std::max(atEnds[0], atEnds[1]);
            steps += atEnds[0] != atEnds[1];
            if (mm <= 0)
                worstIn = std::max(worstIn, static_cast<double>(cap));
            steps += cap < last;
            last = cap;
        }
        Report("10e", "the cap only beyond the arm's reach", worstIn == 0.0 && steps == 0 && last == 1.0f,
               "the wrist from 10 cm inside the arm's reach to 12 cm beyond, the elbow end anywhere: capped %.2f at most within it (0 wanted), never less going out, %.2f at "
               "the end (1 wanted)",
               worstIn, last);
    }
}

// The reviewer's experiment (2026-11-18): hands anywhere in reach in front of the body, the wrist bent any way up to 130 degrees off the triangle's forearm and rolled
// any way about it, both arms, the head level and 35 and 50 degrees down (the view's space: the eye at the origin, the body's up and back the room's). Counted: the
// open elbow end in view (within 50 degrees of straight ahead) where the natural elbow, the triangle's, is not: the flap; of those, the ones well in view (within 40
// degrees: not just over the edge the natural elbow is just outside of) and the ones turned to the eye (it looks into the tube, within 35 degrees of its line). And
// what the cap does to hands in reach.
struct InReachCount {
    int hands = 0, clamped = 0, shown = 0, deep = 0, toEye = 0, capFull = 0, capPart = 0, held = 0, bad = 0;
    double furthest = 0, capMost = 0;  // the elbow end from the triangle's elbow (m), the most cap
};
InReachCount InReachHands(float elbowReach, int each) {
    InReachCount c;
    const V3 eye(0, 0, 0);
    auto inView = [&](const V3& e) { return AngleDeg(e - eye, V3(0, 0, -1)) < 50.0; };
    for (const double pitch : {0.0, 35.0, 50.0})
        for (int side = 0; side < 2; ++side) {
            const bool right = side == 1;
            const double down = pitch * kPiD / 180, sx = right ? 1 : -1;
            const float out[3] = {static_cast<float>(sx), 0, 0}, up[3] = {0, static_cast<float>(std::cos(down)), static_cast<float>(std::sin(down))},
                        back[3] = {0, static_cast<float>(-std::sin(down)), static_cast<float>(std::cos(down))};
            float shoulder[3];
            const float neck[3] = {0.0f, -0.075f, 0.08f};
            for (int i = 0; i < 3; ++i)
                shoulder[i] = neck[i] + out[i] * 0.164f - up[i] * 0.16f + back[i] * 0.02f;
            armik::Params p = BareParams(right), wide = p;
            p.elbowReach = elbowReach;
            wide.flex = wide.ext = wide.radial = wide.ulnar = 1.0e5f;  // (no range at all: the triangle's own forearm whatever the hand)
            const V3 Wb = Joint(armik::kWrist, right), S(shoulder[0], shoulder[1], shoulder[2]), B(back[0], back[1], back[2]);
            const V3 f0 = Unit(Joint(armik::kWrist, right) - Joint(armik::kElbow, right));
            V3 pa(armik::kPalm[0] * (right ? -1 : 1), armik::kPalm[1], armik::kPalm[2]), ax(armik::kAcross[0] * (right ? -1 : 1), armik::kAcross[1], armik::kAcross[2]);
            ax = Unit(ax - f0 * Dot(ax, f0));
            pa = Unit(pa - f0 * Dot(pa, f0) - ax * Dot(pa, ax));
            std::mt19937 g(71 + side + static_cast<int>(pitch));
            std::uniform_real_distribution<double> u(-1, 1);
            int solved = 0;
            while (solved < each) {
                const V3 at = S + V3(u(g), u(g), u(g)) * 0.56;
                if (Len(at - S) > armik::kUpper + armik::kFore - 0.01 || Len(at - S) < 0.12 || Dot(at - S, B) > 0)
                    continue;
                ++solved;
                M34 H;
                {
                    const V3 w = H.Apply(Wb);
                    H.m[3] += at.x - w.x, H.m[7] += at.y - w.y, H.m[11] += at.z - w.z;
                }
                float h[12];
                ToF(H, h);
                armik::State s0;
                armik::Result r0;
                armik::Solve(h, shoulder, out, up, back, right, wide, s0, r0);
                const V3 natural = FromF(r0.elbow), ft = Unit(FromF(r0.wrist) - natural);
                const double bend = (u(g) + 1) * 65 * kPiD / 180, az = u(g) * kPiD;
                const V3 v = Unit(f0 * std::cos(bend) + (pa * -std::cos(az) + ax * -std::sin(az)) * std::sin(bend));
                const V3 axis = Cross(v, ft);
                M34 R = Len(axis) > 1e-9 ? RotAbout(Unit(axis), std::atan2(Len(axis), Dot(v, ft)), V3()) : M34{};
                R = Mul(RotAbout(ft, u(g) * kPiD, V3()), R);
                const V3 w = R.Apply(Wb);
                R.m[3] += at.x - w.x, R.m[7] += at.y - w.y, R.m[11] += at.z - w.z;
                ToF(R, h);
                armik::State st;
                armik::Result r;
                armik::Solve(h, shoulder, out, up, back, right, p, st, r);
                ++c.hands;
                c.clamped += r.clamped, c.held += r.reachHeld, c.bad += !Finite(r);
                const V3 e = FromF(r.elbow);
                c.furthest = std::max(c.furthest, Len(e - natural));
                if (inView(e) && !inView(natural)) {
                    ++c.shown;
                    c.deep += AngleDeg(e - eye, V3(0, 0, -1)) < 40.0;
                    c.toEye += AngleDeg(e - eye, Unit(FromF(r.wrist) - e)) < 35.0;
                }
                const float cap = armik::EndCap(r, shoulder);
                c.capFull += cap >= 1.0f, c.capPart += cap > 0.0f && cap < 1.0f;
                c.capMost = std::max(c.capMost, static_cast<double>(cap));
            }
        }
    return c;
}

void InReach() {
    const InReachCount off = InReachHands(0.0f, 15000), on = InReachHands(0.05f, 15000);
    Report("10f", "hands in reach: no flap, no cap",
           on.bad == 0 && off.shown >= 1000 && on.shown * 4 <= off.shown && on.deep * 20 <= off.deep && on.toEye * 20 <= off.toEye && on.furthest <= 0.0501 &&
               on.capFull == 0 && on.capPart == 0 && off.capFull == 0 && off.capPart == 0 && on.hands == off.hands,
           "%d random hands in reach in front of the body (both arms, the head level, 35 and 50 degrees down), the wrist bent up to 130 degrees any way, %d past its range. "
           "The open elbow end in view where the natural elbow is not: %d with the limit off (%d well in view, %d turned to the eye), %d with it on (%d well in view, %d to "
           "the eye; a quarter, a twentieth and a twentieth of the off's at most); with it on the elbow end %.3f m from the triangle's elbow at most (<= 0.050), %d turned "
           "back; capped whole %d, partly %d, and with the limit off %d, %d (0 wanted of each: no cap in reach); %d not finite",
           on.hands, on.clamped, off.shown, off.deep, off.toEye, on.shown, on.deep, on.toEye, on.furthest, on.held, on.capFull, on.capPart, off.capFull, off.capPart, on.bad);
}

// ---- 11: the bare arm whole (its sleeve on it) ----
void WholeBareArm() {
    for (int side = 0; side < 2; ++side) {
        const bool right = side == 1;
        const double sx = right ? 1 : -1;  // in a view's space (x right): out to this arm's side
        const float out[3] = {static_cast<float>(sx), 0, 0}, up[3] = {0, 1, 0}, back[3] = {0, 0, 1}, chestLeft[3] = {-1, 0, 0};
        const float shoulder[3] = {static_cast<float>(sx * 0.164), -0.235f, 0.10f};
        const V3 Sb = Joint(armik::kShoulder, right), Eb = Joint(armik::kElbow, right), Wb = Joint(armik::kWrist, right), given(shoulder[0], shoulder[1], shoulder[2]);
        armik::Params p = BareParams(right);
        p.elbowReach = 0.05f;
        std::mt19937 g(211 + side);
        std::uniform_real_distribution<double> u(-1, 1);
        double offUpper = 0, offFore = 0, offElbow = 0, offCollar = 0, offFar = 0, gap = 0, gapNear = 0, offRigid = 0, offCollarAxes = 0, onUntil = 0, offFrom = 9;
        int beyond = 0, brought = 0, sleeveOff = 0, bad = 0, ruleWrong = 0, offNotCapped = 0;
        bool sleeve = true;  // (carried on from hand to hand, as hands.cpp carries it from frame to frame)
        armik::State st;
        for (int n = 0; n < 20000; ++n) {
            // As row 9: anywhere within 1.3 m of the eyes, turned any way, the state carried on; every other hand within 0.7 m (most in reach).
            const double reach = n % 2 ? 0.7 : 1.3;
            const V3 at(u(g) * reach, u(g) * reach, u(g) * reach);
            M34 H = RotAbout(Unit(V3(u(g), u(g), u(g))), u(g) * 3.14, V3());
            const V3 w = H.Apply(Wb);
            H.m[3] += at.x - w.x, H.m[7] += at.y - w.y, H.m[11] += at.z - w.z;
            float h[12];
            ToF(H, h);
            armik::Result r;
            armik::Solve(h, shoulder, out, up, back, right, p, st, r);
            const V3 far3 = FromF(r.shoulder);
            const double along = Len(far3 - given);
            // The sleeve: on while the shoulder is brought along no further than the cloth takes, back on only 2 cm inside that; its collar whole.
            const bool sleeveWas = sleeve;
            sleeve = barehold::SleeveStays(static_cast<float>(along), sleeveWas);
            const float keep = 1.0f;
            float collar[12];
            armik::Collar(chestLeft, up, back, r.shoulder, right, keep, collar);
            bool finite = Finite(r);
            for (float v : r.upper)
                finite = finite && std::isfinite(v);
            for (float v : collar)
                finite = finite && std::isfinite(v);
            bad += !finite;
            const M34 U = ToM(r.upper), E0 = ToM(r.frame[0]), C = ToM(collar);
            const V3 uS = U.Apply(Sb), uE = U.Apply(Eb), fE = E0.Apply(Eb);
            offUpper = std::max(offUpper, std::fabs(Len(uS - uE) - armik::kUpper));
            offFore = std::max(offFore, std::fabs(Len(ToM(r.frame[2]).Apply(Wb) - fE) - armik::kFore));
            offElbow = std::max(offElbow, Len(uE - fE));
            offCollar = std::max(offCollar, Len(C.Apply(Sb) - far3));
            offRigid = std::max({offRigid, OffRigid(r.upper), OffRigid(r.frame[0]), OffRigid(r.frame[1]), OffRigid(r.frame[2])});
            // The collar: the chest's axes times keep, nothing else.
            for (int i = 0; i < 3; ++i) {
                offCollarAxes = std::max<double>({offCollarAxes, std::fabs(collar[i * 4] - chestLeft[i] * keep), std::fabs(collar[i * 4 + 1] - up[i] * keep),
                                                  std::fabs(collar[i * 4 + 2] + back[i] * keep)});
            }
            beyond += !r.reach;
            if (along > 1e-6) {  // the shoulder brought along: the bone's far end is the collar's joint
                ++brought;
                offFar = std::max(offFar, Len(uS - far3));
            } else if (Len(FromF(r.wrist) - given) >= 0.05) {  // the elbow end nearer the shoulder than the bone is long: the bone's end past it, by the reach limit at most
                gap = std::max(gap, Len(uS - far3));
            } else {  // (the wrist on the shoulder itself: no triangle)
                gapNear = std::max(gapNear, Len(uS - far3));
            }
            sleeveOff += !sleeve;
            ruleWrong += (along > barehold::kSleeveShoulderMost && sleeve) || (along <= barehold::kSleeveShoulderBack && !sleeve) ||
                         (along > barehold::kSleeveShoulderBack && along <= barehold::kSleeveShoulderMost && sleeve != sleeveWas);
            offNotCapped += !sleeve && armik::EndCap(r, shoulder) < 1.0f;
            if (sleeve)
                onUntil = std::max(onUntil, along);
            else
                offFrom = std::min(offFrom, along);
        }
        Report(right ? "11R" : "11L", right ? "the right bare arm whole" : "the left bare arm whole",
               bad == 0 && offUpper <= 1e-4 && offFore <= 1e-4 && offElbow <= 1e-4 && offCollar <= 1e-4 && offFar <= 1e-4 && gap <= 0.0505 && offRigid <= 1e-5 && offCollarAxes == 0 &&
                   ruleWrong == 0 && offNotCapped == 0 && beyond > 1000 && brought > 1000 && sleeveOff > 100 && onUntil <= barehold::kSleeveShoulderMost + 1e-6 &&
                   offFrom > barehold::kSleeveShoulderBack,
               "20000 random hands with the bare arms' numbers and the reach limit (%d beyond reach, %d with the shoulder brought along, %d without the sleeve"
               "): the upper arm %.4f mm and the forearm %.4f mm off their lengths (<= 0.1), the two %.4f mm apart at the elbow (<= 0.1), all four bones %.1e off rigid; the "
               "collar's shoulder joint %.4f mm from the shoulder (<= 0.1) and, brought along, %.4f mm from the upper arm's far end (<= 0.1); where the elbow end sits nearer the "
               "shoulder than the bone is long the bone's end %.1f mm past it at most (the reach limit's 50; %.1f mm with the wrist on the shoulder itself); the collar the chest's "
               "axes, never drawn in; the sleeve on up to %.3f m brought along (the limit %.2f), off from %.3f m (back on within %.2f), %d against the rule, %d arms without it "
               "not capped whole (0 wanted of each); %d not finite",
               beyond, brought, sleeveOff, offUpper * 1000, offFore * 1000, offElbow * 1000, offRigid, offCollar * 1000, offFar * 1000, gap * 1000, gapNear * 1000, onUntil,
               barehold::kSleeveShoulderMost, offFrom, barehold::kSleeveShoulderBack, ruleWrong, offNotCapped, bad);
    }
}

}  // namespace

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "E:\\BF3 VR PROJECT\\build";
    const armik::Params p;  // the left's
    std::printf("armtest: src/armik.h, the forearm from its hand. Upper arm %.4f m, forearm %.4f m; the left's pole "
                "out %.1f down %.1f back %.1f, wrist range flexion %.0f extension %.0f radial %.0f ulnar %.0f, twist "
                "shares %.3f/%.3f, held from %.0f deg.\n",
                armik::kUpper, armik::kFore, p.poleOut, p.poleDown, p.poleBack, p.flex, p.ext, p.radial, p.ulnar,
                p.share[1], p.share[2], Deg(p.maxTwist));
    BindAndRigid();
    GameHold(dir);
    const Pose pose = MakePose(p);
    TwistSweep(pose, p);
    Bends(pose, p);
    Degenerate(pose, p);
    Cost(pose, p);
    StandIn(p);
    NeverStretches(p);
    ReachOff();
    ReachOn();
    LoggedWrists();
    RestPicture();
    InReach();
    WholeBareArm();
    std::printf("%d of %d checks failed%s\n", g_failed, g_counted,
                g_skipped ? " (the game's hold skipped: no mesh dumps)" : "");
    return g_failed;
}

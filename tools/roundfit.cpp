// Fits the left glove round the grenade launcher's 40 mm round carried in the fist, away from the launcher: from the belly to the barrel, or a spent case pulled out.
// The user, 2026-11-05, of the round drawn in the game's own rim pinch (perched on the index finger): "I want it to actually look like he's holding the shell ... I just
// need his thumb and his finger to sort of wrap around the shell." So a fist round it, tools/rocketfit.cpp's way: the palm against the round's body, every joint of the four
// fingers closed round it one way until what it moves touches, the thumb round it the other way until it touches the round or lies on the fingers. The round lies across
// the fist, its base (the rim) at the little finger's side, flush or a little out, its nose out past the thumb and index side, so the hand pushes it nose first into the
// breech.
// The round's mesh (draw 1440 from 7848, build\mesh_1440_7848_*) is measured on its bone at the start: a straight body 5.1 cm across from its base to where its nose
// begins, then the ogive to its tip. The whole glove, every vertex on its 23 bones, is kept out of that solid of revolution but for a glove's give (2.5 mm); the thumb out
// of the fingers as much. The glove's fingers are thick (3.5 cm through, 2.6 across) and the fist covers 10.6 cm of the round's 11.9: with its base flush the nose stands
// little more than a centimetre out past the fist, 3 past the index knuckle line.
//   roundfit <build folder> <out.bmp>                                      the best fit of each turn, drawn from seven sides, one row each
//   roundfit <build folder> <out.bmp> pick <turn> <tilt> <along> <out.h>   that placement only, drawn big from eight sides; writes the tables (src/m320carry.h)
//   roundfit <build folder> <out.bmp> why <turn> <tilt> <along>            that placement's fit, step by step, drawn big
// Options after them: base=<mm> how far the base stands out past the little finger (2; negative tucks it in); apart=<i>,<m>,<r>,<l> the fingers' turn towards the thumb
// at their knuckles (degrees; Pose's spread 0.25 if not given); pad=<mm> how far a last part's pad may go in to bring its tip down (2.5, the most any of the glove may);
// palmin=<mm> how far the open palm is put in (1.5); thumbpast=<n> what the thumb further out towards the nose than the index finger costs a mm (3);
// turns=, tilts=, alongs= <from>:<to>:<step> the search's grid; plain: the glove drawn in one colour, not each finger its own.
// All in the gloves' bind space: the round's axis runs through O along D, D towards its base. turn: degrees it lies off straight across the hand, its base's end towards
// the wrist; tilt: out of the palm's plane; along: metres from the hand's middle towards the fingers.
// Built without the project's CMake (the mod's DLL is held by the running game), from the project's folder:
//   cl /nologo /EHsc /O2 /std:c++17 /DNOMINMAX /DWIN32_LEAN_AND_MEAN /I src /I tools /Febuild\roundfit.exe /Fobuild\ tools\roundfit.cpp src\handpose.cpp
//   build\roundfit build %TEMP%\round.bmp pick 20 5 0.0125 src\m320carry.h base=0   (what made src/m320carry.h)

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "meshdump.h"

using namespace meshdump;

namespace {

constexpr float kMiddle[3] = {0.537f, 1.098f, 0.207f};
constexpr int8_t kDepth[handpose::kBones] = {0, 0, 3, 3, 0, 0, 0, 1, 0, 1, 3, 1, 0, 1, 0, 3, 2, 2, 1, 2, 2, 2, 3};
constexpr float kWidth = 1.15f;     // the user's palm width (hands.cpp g_leftWidth)
constexpr float kAllowed = 0.0025f; // a glove's give: this far into the round at most, anywhere
constexpr float kPi = 3.14159265f;
constexpr float kSpreadApart[4] = {3.0f, 0.25f, -1.25f, -4.25f};  // the fingers' turn at their knuckles: Pose's spread 0.25
// The round's line as the mod has it (src/m320.cpp kRoundBaseOnBone, kRoundNoseOnBone): x 0.0003, y -0.0584 on its bone, its base at z 0.2795, its nose at 0.3984. That line
// is put exactly on the fitted axis; the mesh's own middle line, measured, is half a millimetre off it, and the glove is kept out of the round as drawn.
constexpr float kLine[2] = {0.0003f, -0.0584f};
constexpr float kBaseZ = 0.2795f, kNoseZ = 0.3984f;
// The game's own pinch on the round's rim (src/m320.cpp kBones7350, frame 7350): its palm, bone 1, from the gloves' bind space into the round's bone. As the mod draws it
// (re-anchored on the palm) the round's in the glove at the drawn palm times this one's inverse; the carried round is turned about its length as near that as it goes, so
// going over from the pinch to the carry swings it without spinning it.
constexpr float kPinchPalm[12] = {0.715837f, 0.657457f, -0.235217f, -0.987313f, -0.607637f, 0.752485f, 0.254053f, -0.632581f, 0.344026f, -0.038934f, 0.938152f, -0.097090f};

// ---- The round's shape, measured from its mesh ----
struct Profile {
    float axis[2] = {};          // its middle line on its bone
    std::vector<float> z, r;     // its outline from the base's rim to the tip (the upper hull of every vertex's place along and out from the line)
    float base = 0.0f, nose = 0.0f, body = 0.0f, bodyEnd = 0.0f;  // where its base and tip are, the body's radius, where the nose begins

    float RadiusAt(float zz) const {
        if (zz < base || zz > nose)
            return -1.0f;
        for (size_t i = 0; i + 1 < z.size(); ++i)
            if (zz <= z[i + 1]) {
                const float t = z[i + 1] > z[i] ? (zz - z[i]) / (z[i + 1] - z[i]) : 0.0f;
                return r[i] + (r[i + 1] - r[i]) * t;
            }
        return r.back();
    }
    // Signed: metres out of the round, negative in it.
    float Distance(float zz, float rho) const {
        float best = 1e9f;
        auto edge = [&](float z0, float r0, float z1, float r1) {
            const float ez = z1 - z0, er = r1 - r0, pz = zz - z0, pr = rho - r0;
            const float l2 = ez * ez + er * er;
            const float t = l2 > 0.0f ? std::clamp((pz * ez + pr * er) / l2, 0.0f, 1.0f) : 0.0f;
            best = std::min(best, std::hypot(pz - ez * t, pr - er * t));
        };
        edge(base, 0.0f, z[0], r[0]);
        for (size_t i = 0; i + 1 < z.size(); ++i)
            edge(z[i], r[i], z[i + 1], r[i + 1]);
        edge(z.back(), r.back(), nose, 0.0f);
        const float R = RadiusAt(zz);
        return R >= 0.0f && rho <= R ? -best : best;
    }
};

Profile g_profile;
Mesh g_round;  // its mesh on its bone

bool MeasureRound(const std::string& dir) {
    if (!LoadDraw(dir, 1440, 0, {}, g_round, -1, 0, 7848))
        return false;
    std::vector<char> used(g_round.pos.size() / 3, 0);
    for (int t : g_round.tris)
        used[t] = 1;
    float lo = 1e9f, hi = -1e9f;
    for (size_t v = 0; v < used.size(); ++v)
        if (used[v])
            lo = std::min(lo, g_round.pos[v * 3 + 2]), hi = std::max(hi, g_round.pos[v * 3 + 2]);
    // Its middle line: the middle of the rings off its base's face (the face has the primer's rings, not round the line alone).
    double sx = 0, sy = 0;
    int n = 0;
    for (size_t v = 0; v < used.size(); ++v)
        if (used[v] && g_round.pos[v * 3 + 2] > lo + 0.002f && g_round.pos[v * 3 + 2] < hi - 0.001f)
            sx += g_round.pos[v * 3], sy += g_round.pos[v * 3 + 1], ++n;
    if (!n)
        return false;
    Profile& p = g_profile;
    p.axis[0] = static_cast<float>(sx / n), p.axis[1] = static_cast<float>(sy / n);
    p.base = lo, p.nose = hi;
    std::vector<std::pair<float, float>> pts;
    for (size_t v = 0; v < used.size(); ++v)
        if (used[v])
            pts.push_back({g_round.pos[v * 3 + 2], std::hypot(g_round.pos[v * 3] - p.axis[0], g_round.pos[v * 3 + 1] - p.axis[1])});
    std::sort(pts.begin(), pts.end());
    std::vector<std::pair<float, float>> hull;
    for (const auto& q : pts) {
        while (hull.size() >= 2) {
            const auto& o = hull[hull.size() - 2];
            const auto& a = hull.back();
            const float cross = (a.first - o.first) * (q.second - o.second) - (a.second - o.second) * (q.first - o.first);
            if (cross < 0.0f)
                break;
            hull.pop_back();
        }
        hull.push_back(q);
    }
    while (hull.size() >= 2 && hull[1].first - hull[0].first < 1e-5f)  // the base's face: only its rim
        hull.erase(hull.begin());
    for (const auto& q : hull)
        p.z.push_back(q.first), p.r.push_back(q.second);
    p.body = p.r[0];
    p.bodyEnd = p.z[0];
    for (size_t i = 1; i < p.z.size(); ++i)
        if (p.r[i] >= p.body - 0.0002f)
            p.bodyEnd = p.z[i], p.body = std::max(p.body, p.r[i]);
    return true;
}

// ---- The glove ----
std::vector<GloveVertex> g_glove;
std::vector<int> g_gloveTris;
std::vector<int> g_all;         // every vertex of its drawn triangles: all of it, on the left glove's 23 bones
std::vector<int> g_part[6];     // each finger's by the bone with the most weight (0 the palm with the cuff, 1 the thumb, 2-5 the fingers)
std::vector<int> g_finger[5];   // the finger itself, without its bone in the palm (for the thumb: its two last parts, not the one under the ball of the thumb)
std::vector<int> g_seg[5][4];   // each finger's parts (1-3) without the vertices near the joint before it, which the joint's bend pinches inwards: what touches
std::vector<int> g_tipPart[5];  // the last part's far end
std::vector<int> g_moves[5][4]; // what each joint moves (1 the knuckle or the thumb's base, 2, 3), but not the parts past it, which their own joints close: none of it in
float g_joints[5][4][3];
int g_boneOf[5][4];             // each finger's bone by how many joints out (1-3)
float g_partRadius[5][4];       // each finger part's thickness about its middle line (the thumb lies on them)
float g_padSink = 0.0025f;      // how far a last part's pad may go into the round to bring its tip down. "pad=2.5" (mm)
float g_tipGap = 0.0005f;       // ... until the tip's this near the surface
float g_baseOut = 0.002f;       // how far the base stands out past the little finger. "base=2" (mm)
float g_palmIn = 0.0015f;       // how far the open palm is put into it: less than the give, for the palm's vertices the knuckles move a little. "palmin=1.5" (mm)
float g_thumbPast = 3.0f;       // what the thumb lying further out towards the nose than the index finger costs, per mm. "thumbpast=3"
float g_apart[4] = {kSpreadApart[0], kSpreadApart[1], kSpreadApart[2], kSpreadApart[3]};
bool g_why = false;
bool g_plain = false;           // the glove drawn in one colour, as the game has it, not each finger its own. "plain"

int MainBone(const GloveVertex& v) {
    int best = 0;
    for (int s = 1; s < 4; ++s)
        if (v.weight[s] > v.weight[best])
            best = s;
    return v.bone[best];
}

void PosePoint(const float (*bones)[12], int i, float* out) {
    const GloveVertex& v = g_glove[i];
    out[0] = out[1] = out[2] = 0.0f;
    for (int s = 0; s < 4; ++s) {
        if (v.weight[s] <= 0 || v.bone[s] >= handpose::kBones)
            continue;
        float p[3];
        Apply(bones[v.bone[s]], v.pos, p);
        for (int k = 0; k < 3; ++k)
            out[k] += v.weight[s] * p[k];
    }
}

void JointAt(const float (*bones)[12], int finger, int joint, float* out) {
    Apply(bones[g_boneOf[finger][std::max(joint, 1)]], g_joints[finger][joint], out);
}

float FromSegment(const float* p, const float* a, const float* b) {
    const float ab[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]}, ap[3] = {p[0] - a[0], p[1] - a[1], p[2] - a[2]};
    const float l2 = Dot(ab, ab);
    const float t = l2 > 0.0f ? std::clamp(Dot(ap, ab) / l2, 0.0f, 1.0f) : 0.0f;
    const float d[3] = {ap[0] - ab[0] * t, ap[1] - ab[1] * t, ap[2] - ab[2] * t};
    return std::sqrt(Dot(d, d));
}

void InvertRigid(const float* m, float* out) {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            out[i * 4 + j] = m[j * 4 + i];
    for (int i = 0; i < 3; ++i)
        out[i * 4 + 3] = -(out[i * 4] * m[3] + out[i * 4 + 1] * m[7] + out[i * 4 + 2] * m[11]);
}

// ---- The round in the hand ----
struct Round {
    float place[12] = {}, inverse[12] = {};  // its bone into the glove, and back
    float O[3] = {}, D[3] = {};              // the fitted axis: through O, D along it towards the base (the little finger's side)
    float baseAlong = 0.0f;                  // the base's middle at O + D * this
    float Distance(const float* p) const {
        float q[3];
        Apply(inverse, p, q);
        return g_profile.Distance(q[2], std::hypot(q[0] - g_profile.axis[0], q[1] - g_profile.axis[1]));
    }
    float Along(const float* p) const { return (p[0] - O[0]) * D[0] + (p[1] - O[1]) * D[1] + (p[2] - O[2]) * D[2]; }
    float NoseAlong() const { return baseAlong - (kNoseZ - kBaseZ); }
};

// Its bone's turn in the glove (rows): its length, base to nose, along -D, turned about it as near the game's pinch has it as goes.
void RoundTurn(const float* D, float* R) {
    // The pinch's, as drawn: the drawn palm (Pose's at this width) times the game's palm's inverse; its rows made square again (the palm's widening isn't a turn).
    float palm[handpose::kBones][12];
    handpose::Pose(handpose::Fingers{}, kWidth, palm);
    float inv[12], drawn[12];
    InvertRigid(kPinchPalm, inv);
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            drawn[i * 4 + j] = palm[1][i * 4] * inv[j] + palm[1][i * 4 + 1] * inv[4 + j] + palm[1][i * 4 + 2] * inv[8 + j];
    float zv[3] = {drawn[2], drawn[6], drawn[10]}, xv[3] = {drawn[0], drawn[4], drawn[8]};
    Normalize(zv);
    const float d = Dot(xv, zv);
    for (int k = 0; k < 3; ++k)
        xv[k] -= d * zv[k];
    Normalize(xv);
    float yv[3];
    Cross(zv, xv, yv);
    // The least turn from its length to -D.
    const float zc[3] = {-D[0], -D[1], -D[2]};
    float axis[3];
    Cross(zv, zc, axis);
    const float s = std::sqrt(Dot(axis, axis)), c = Dot(zv, zc);
    float Q[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
    if (s > 1e-7f) {
        for (float& v : axis)
            v /= s;
        const float a = std::atan2(s, c), cs = std::cos(a), sn = std::sin(a), t = 1 - cs;
        const float x = axis[0], y = axis[1], z = axis[2];
        const float q[9] = {t * x * x + cs, t * x * y - sn * z, t * x * z + sn * y, t * x * y + sn * z, t * y * y + cs, t * y * z - sn * x,
                            t * x * z - sn * y, t * y * z + sn * x, t * z * z + cs};
        std::memcpy(Q, q, sizeof(Q));
    } else if (c < 0.0f) {
        const float q[9] = {-1, 0, 0, 0, -1, 0, 0, 0, 1};  // (never: the pinch's length is 66 degrees off)
        std::memcpy(Q, q, sizeof(Q));
    }
    const float cols[3][3] = {{xv[0], xv[1], xv[2]}, {yv[0], yv[1], yv[2]}, {zv[0], zv[1], zv[2]}};
    for (int j = 0; j < 3; ++j) {
        float turned[3];
        for (int i = 0; i < 3; ++i)
            turned[i] = Q[i * 3] * cols[j][0] + Q[i * 3 + 1] * cols[j][1] + Q[i * 3 + 2] * cols[j][2];
        for (int i = 0; i < 3; ++i)
            R[i * 3 + j] = turned[i];
    }
}

void PlaceRound(Round& round, const float* O, const float* D, float baseAlong) {
    float R[9];
    RoundTurn(D, R);
    std::memcpy(round.O, O, sizeof(round.O));
    std::memcpy(round.D, D, sizeof(round.D));
    round.baseAlong = baseAlong;
    const float b[3] = {kLine[0], kLine[1], kBaseZ};
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            round.place[i * 4 + j] = R[i * 3 + j];
        round.place[i * 4 + 3] = O[i] + D[i] * baseAlong - (R[i * 3] * b[0] + R[i * 3 + 1] * b[1] + R[i * 3 + 2] * b[2]);
    }
    InvertRigid(round.place, round.inverse);
}

// What a part closes onto: the round, and for the thumb the fingers too (each of their parts a capsule as thick as its mesh).
struct Capsule {
    float a[3], b[3], radius;
};
struct Target {
    const Round* round = nullptr;
    std::vector<Capsule> fingers;
    float Distance(const float* p) const {
        float d = round->Distance(p);
        for (const Capsule& c : fingers)
            d = std::min(d, FromSegment(p, c.a, c.b) - c.radius);
        return d;
    }
};

void FingerCapsules(const float (*bones)[12], std::vector<Capsule>& out) {
    out.clear();
    for (int f = 1; f < 5; ++f)
        for (int d = 1; d <= 3; ++d) {
            Capsule c;
            JointAt(bones, f, d - 1, c.a);
            JointAt(bones, f, d, c.b);
            c.radius = g_partRadius[f][d];
            out.push_back(c);
        }
}

float GapOf(const Target& t, const float (*bones)[12], const std::vector<int>& set) {
    float gap = 1e9f;
    for (int i : set) {
        float q[3];
        PosePoint(bones, i, q);
        gap = std::min(gap, t.Distance(q));
    }
    return gap;
}

// Closes one joint from low until what it moves touches, as a hand closing does. Bent back, a joint's inside bulges (the glove's skinning): from low it first opens up
// to where nothing it moves is in, if that's further. False: it's never out.
bool Close(const Target& t, handpose::JointPose& pose, float& angle, float low, float high, const std::vector<int>& part) {
    float bones[handpose::kBones][12];
    for (;; low += 2.0f) {
        if (low > high)
            return false;
        angle = low;
        handpose::PoseJoints(pose, kWidth, bones);
        if (GapOf(t, bones, part) >= -kAllowed)
            break;
    }
    float last = low;
    for (float a = low; a <= high; a += 2.0f) {
        angle = a;
        handpose::PoseJoints(pose, kWidth, bones);
        if (GapOf(t, bones, part) < -kAllowed) {
            if (a <= low)
                return false;
            float l = last, h = a;
            for (int i = 0; i < 5; ++i) {
                const float m = (l + h) / 2;
                angle = m;
                handpose::PoseJoints(pose, kWidth, bones);
                (GapOf(t, bones, part) < -kAllowed ? h : l) = m;
            }
            angle = l;
            return true;
        }
        last = a;
    }
    angle = high;
    return true;
}

// A straight last part touches a round thing at one place, its tip standing off: the last joint goes on closing until the tip's on it too, nothing it moves more than
// the pad's give in.
void TipDown(const Target& t, handpose::JointPose& pose, float& angle, float high, int f) {
    float bones[handpose::kBones][12];
    float keep = angle;
    for (float a = angle; a <= high; a += 1.0f) {
        angle = a;
        handpose::PoseJoints(pose, kWidth, bones);
        if (GapOf(t, bones, g_moves[f][3]) < -std::min(g_padSink, kAllowed)) {
            angle = keep;
            return;
        }
        if (GapOf(t, bones, g_tipPart[f]) <= g_tipGap)
            return;
        keep = a;
    }
    angle = keep;
}

struct Lay {
    float gaps[3] = {};  // each part's gap (mm)
    float score = -1e30f;
};

// One finger closed on the round from the palm out: the knuckle as far as it goes, or a little less, so the joints after it bend as a hand's do, each part touching.
bool LayFinger(const Target& t, const float* palmWay, const float* fingersWay, int k, handpose::JointPose& pose, Lay& out) {
    const int f = k + 1;
    pose.finger[k][1] = pose.finger[k][2] = 8.0f;
    if (!Close(t, pose, pose.finger[k][0], -25.0f, 95.0f, g_moves[f][1])) {
        if (g_why)
            std::printf("  %s: its knuckle is never out\n", f == 1 ? "index" : f == 2 ? "middle" : f == 3 ? "ring" : "little");
        return false;
    }
    const float most = pose.finger[k][0];
    handpose::JointPose best = pose;
    float bones[handpose::kBones][12];
    for (float knuckle = most; knuckle >= most - 30.0f; knuckle -= 3.0f) {
        handpose::JointPose p = pose;
        p.finger[k][0] = knuckle;
        p.finger[k][2] = 8.0f;
        if (!Close(t, p, p.finger[k][1], 8.0f, 110.0f, g_moves[f][2])) {
            if (g_why)
                std::printf("  finger %d knuckle %.0f (most %.0f): its middle joint never out\n", f, knuckle, most);
            continue;
        }
        if (!Close(t, p, p.finger[k][2], 8.0f, 85.0f, g_moves[f][3])) {
            if (g_why)
                std::printf("  finger %d knuckle %.0f middle %.0f: its last joint never out\n", f, knuckle, p.finger[k][1]);
            continue;
        }
        TipDown(t, p, p.finger[k][2], 85.0f, f);
        handpose::PoseJoints(p, kWidth, bones);
        float tip[3];
        JointAt(bones, f, 3, tip);
        const float v[3] = {tip[0] - t.round->O[0], tip[1] - t.round->O[1], tip[2] - t.round->O[2]};
        const float reach = std::atan2(Dot(v, fingersWay), Dot(v, palmWay)) * 180.0f / kPi;
        Lay lay;
        float gapSum = 0.0f;
        for (int d = 1; d <= 3; ++d) {
            lay.gaps[d - 1] = GapOf(t, bones, g_seg[f][d]) * 1000.0f;
            gapSum += std::max(lay.gaps[d - 1], 0.0f);
        }
        // Touching first; then round as far as it goes, its middle and last joints bent alike.
        lay.score = reach - gapSum * 8.0f - std::fabs(p.finger[k][1] - p.finger[k][2]) * 0.15f;
        if (g_why)
            std::printf("  finger %d: knuckle %.0f (most %.0f) middle %.0f last %.0f: gaps %.1f %.1f %.1f mm, round %.0f, score %.0f\n", f, knuckle, most, p.finger[k][1],
                        p.finger[k][2], lay.gaps[0], lay.gaps[1], lay.gaps[2], reach, lay.score);
        if (lay.score > out.score) {
            out = lay;
            best = p;
        }
    }
    if (out.score < -1e29f)
        return false;
    pose = best;
    return true;
}

// The thumb the same way, round the other way, its way out of the palm given: onto the round, or onto the fingers where they're between.
bool LayThumb(const Target& t, const float* palmWay, const float* fingersWay, handpose::JointPose& pose, Lay& out, float& reachOut, float& acrossOut) {
    pose.thumbKnuckle = pose.thumbLast = 5.0f;
    if (!Close(t, pose, pose.thumbBase, -25.0f, 70.0f, g_moves[0][1]))
        return false;
    const float most = pose.thumbBase;
    handpose::JointPose best = pose;
    float bones[handpose::kBones][12];
    for (float base = most; base >= most - 15.0f; base -= 5.0f) {
        handpose::JointPose p = pose;
        p.thumbBase = base;
        p.thumbLast = 5.0f;
        if (!Close(t, p, p.thumbKnuckle, 0.0f, 85.0f, g_moves[0][2]))
            continue;
        if (!Close(t, p, p.thumbLast, 5.0f, 85.0f, g_moves[0][3]))
            continue;
        TipDown(t, p, p.thumbLast, 85.0f, 0);
        handpose::PoseJoints(p, kWidth, bones);
        float j1[3], j2[3], tip[3];
        JointAt(bones, 0, 1, j1), JointAt(bones, 0, 2, j2), JointAt(bones, 0, 3, tip);
        const float v[3] = {tip[0] - t.round->O[0], tip[1] - t.round->O[1], tip[2] - t.round->O[2]};
        const float reach = -std::atan2(Dot(v, fingersWay), Dot(v, palmWay)) * 180.0f / kPi;
        const float a[3] = {j2[0] - j1[0], j2[1] - j1[1], j2[2] - j1[2]}, b[3] = {tip[0] - j2[0], tip[1] - j2[1], tip[2] - j2[2]};
        float curl[3];
        Cross(a, b, curl);
        Normalize(curl);
        const float across = std::fabs(Dot(curl, t.round->D));
        Lay lay;
        float gapSum = 0.0f;
        for (int d = 2; d <= 3; ++d) {
            lay.gaps[d - 1] = GapOf(t, bones, g_seg[0][d]) * 1000.0f;
            gapSum += std::max(lay.gaps[d - 1], 0.0f);
        }
        lay.gaps[0] = GapOf(t, bones, g_seg[0][1]) * 1000.0f;
        lay.score = reach + across * 90.0f - gapSum * 8.0f;
        if (lay.score > out.score) {
            out = lay;
            best = p;
            reachOut = reach;
            acrossOut = across;
        }
    }
    if (out.score < -1e29f)
        return false;
    pose = best;
    return true;
}

struct Result {
    handpose::JointPose pose;
    Round round;
    float turn = 0, tilt = 0, along = 0, h = 0;
    float reach[5] = {};       // how far round each tip gets from the palm, degrees (the thumb's the other way, positive too)
    float across = 0;          // how much the thumb's curl lies across the round (1: right across it)
    float noseOut = 0;         // how far the nose stands out past the index finger and the thumb, whichever's further out (m)
    float noseOutIndex = 0, noseOutThumb = 0, noseOutKnuckle = 0, noseOutHand = 0;
    float baseOut = 0;         // how far the base stands out past the little finger (m)
    float baseOutHand = 0;     // ... past all of the hand on that side (the heel of the palm, the cuff)
    float score = -1e30f;
};

// Which way is which round the round: from its axis towards the palm, and the way the fingers go.
void Ways(const Round& round, const float* E, float* palmWay, float* fingersWay) {
    palmWay[0] = -kPalm[0], palmWay[1] = -kPalm[1], palmWay[2] = -kPalm[2];
    const float d = Dot(palmWay, round.D);
    for (int k = 0; k < 3; ++k)
        palmWay[k] -= d * round.D[k];
    Normalize(palmWay);
    Cross(round.D, palmWay, fingersWay);
    if (Dot(fingersWay, E) < 0)
        for (int k = 0; k < 3; ++k)
            fingersWay[k] = -fingersWay[k];
}

float MostAlong(const Round& round, const float (*bones)[12], const std::vector<int>& set, bool most) {
    float m = most ? -1e9f : 1e9f;
    for (int i : set) {
        float q[3];
        PosePoint(bones, i, q);
        const float a = round.Along(q);
        m = most ? std::max(m, a) : std::min(m, a);
    }
    return m;
}

// Where the nose and the base stand out of the fist.
void Ends(Result& r) {
    float bones[handpose::kBones][12];
    handpose::PoseJoints(r.pose, kWidth, bones);
    const float nose = r.round.NoseAlong();
    r.noseOutIndex = MostAlong(r.round, bones, g_finger[1], false) - nose;
    r.noseOutThumb = MostAlong(r.round, bones, g_finger[0], false) - nose;
    r.noseOut = std::min(r.noseOutIndex, r.noseOutThumb);
    r.noseOutHand = MostAlong(r.round, bones, g_all, false) - nose;
    float knuckle[3];
    JointAt(bones, 1, 0, knuckle);
    r.noseOutKnuckle = r.round.Along(knuckle) - nose;
    r.baseOut = r.round.baseAlong - MostAlong(r.round, bones, g_finger[4], true);
    r.baseOutHand = r.round.baseAlong - MostAlong(r.round, bones, g_all, true);
}

// The whole hand at one placement. False: it doesn't go.
bool FitAt(float turnDeg, float tiltDeg, float along, Result& r) {
    const float t = turnDeg * kPi / 180, l = tiltDeg * kPi / 180;
    float D[3], E[3];
    for (int k = 0; k < 3; ++k)
        D[k] = (kAcross[k] * std::cos(t) - kAlong[k] * std::sin(t)) * std::cos(l) + kPalm[k] * std::sin(l);
    Normalize(D);
    for (int k = 0; k < 3; ++k)
        E[k] = kAlong[k] * std::cos(t) + kAcross[k] * std::sin(t);
    handpose::JointPose open;
    std::memcpy(open.fingerApart, g_apart, sizeof(g_apart));
    float bones[handpose::kBones][12];
    handpose::PoseJoints(open, kWidth, bones);
    // The base first where the open hand's little finger ends, then where the closed one's does, until it's still.
    Round round;
    float O[3];
    auto placeO = [&](float h) {
        for (int k = 0; k < 3; ++k)
            O[k] = kMiddle[k] + E[k] * along + kPalm[k] * h;
    };
    placeO(0.04f);
    PlaceRound(round, O, D, 0.0f);
    float baseAlong = MostAlong(round, bones, g_finger[4], true) + g_baseOut;
    handpose::JointPose pose = open;
    float palmWay[3], fingersWay[3], h = 0.0f, fingersScore = 0.0f;
    for (int iteration = 0; iteration < 4; ++iteration) {
        // Brought up out of the palm until the palm meets it.
        pose = open;
        handpose::PoseJoints(pose, kWidth, bones);
        float low = 0.01f, high = 0.10f;
        Target target;
        target.round = &round;
        for (int step = 0; step < 14; ++step) {
            const float mid = (low + high) / 2;
            placeO(mid);
            PlaceRound(round, O, D, baseAlong);
            (-GapOf(target, bones, g_part[0]) > g_palmIn ? low : high) = mid;
        }
        h = high;
        placeO(h);
        PlaceRound(round, O, D, baseAlong);
        Ways(round, E, palmWay, fingersWay);
        if (g_why) {  // what of the palm meets it first
            float deepest = -1e9f, at[3] = {};
            int bone = -1;
            for (int i : g_part[0]) {
                float q[3];
                PosePoint(bones, i, q);
                const float d = -round.Distance(q);
                if (d > deepest) {
                    deepest = d, bone = MainBone(g_glove[i]);
                    const float v[3] = {q[0] - kMiddle[0], q[1] - kMiddle[1], q[2] - kMiddle[2]};
                    at[0] = Dot(v, kAlong), at[1] = Dot(v, kAcross), at[2] = Dot(v, kPalm);
                }
            }
            std::printf("  the palm meets it first at bone %d, %.1f mm in: from the hand's middle %.1f along, %.1f across, %.1f out of the palm (mm)\n", bone, deepest * 1000,
                        at[0] * 1000, at[1] * 1000, at[2] * 1000);
        }
        fingersScore = 0.0f;
        for (int k = 0; k < 4; ++k) {
            Lay lay;
            if (!LayFinger(target, palmWay, fingersWay, k, pose, lay)) {
                if (g_why)
                    std::printf("finger %d wouldn't lie on it; palm at %.3f, base at %.3f\n", k, h, baseAlong);
                return false;
            }
            fingersScore += lay.score * 0.25f;
        }
        handpose::PoseJoints(pose, kWidth, bones);
        const float wanted = MostAlong(round, bones, g_finger[4], true) + g_baseOut;
        if (g_why) {
            std::printf("pass %d: palm at %.4f, base at %.4f, wanted at %.4f; along the round from O towards the base (mm), each finger from .. to:", iteration, h, baseAlong, wanted);
            for (int f = 1; f < 5; ++f)
                std::printf(" %s %.1f .. %.1f", f == 1 ? "index" : f == 2 ? "middle" : f == 3 ? "ring" : "little", MostAlong(round, bones, g_finger[f], false) * 1000,
                            MostAlong(round, bones, g_finger[f], true) * 1000);
            std::printf("; the palm %.1f .. %.1f; the nose at %.1f\n", MostAlong(round, bones, g_part[0], false) * 1000, MostAlong(round, bones, g_part[0], true) * 1000,
                        (baseAlong - (kNoseZ - kBaseZ)) * 1000);
        }
        if (std::fabs(wanted - baseAlong) < 0.0003f)
            break;
        baseAlong = wanted;
    }
    r.round = round, r.turn = turnDeg, r.tilt = tiltDeg, r.along = along, r.h = h;
    for (int k = 0; k < 4; ++k) {
        float tip[3];
        JointAt(bones, k + 1, 3, tip);
        const float v[3] = {tip[0] - round.O[0], tip[1] - round.O[1], tip[2] - round.O[2]};
        r.reach[k + 1] = std::atan2(Dot(v, fingersWay), Dot(v, palmWay)) * 180.0f / kPi;
    }
    if (g_why)
        std::printf("the fingers lie on it, %.0f %.0f %.0f %.0f round\n", r.reach[1], r.reach[2], r.reach[3], r.reach[4]);
    // The thumb: the best way out of the palm that lays it round the other way, across the round, onto it or the fingers.
    Target thumbTarget;
    thumbTarget.round = &r.round;
    FingerCapsules(bones, thumbTarget.fingers);
    Result best = r;
    int laid = 0;
    auto tryThumb = [&](float spread, float out, float roll) {
        handpose::JointPose p = pose;
        p.thumbSpread = spread, p.thumbOut = out, p.thumbRoll = roll;
        Lay lay;
        float reach = 0.0f, across = 0.0f;
        if (!LayThumb(thumbTarget, palmWay, fingersWay, p, lay, reach, across))
            return;
        ++laid;
        // Not over the nose: the thumb no further out that way than the index finger, or the nose can't go in.
        float laidBones[handpose::kBones][12];
        handpose::PoseJoints(p, kWidth, laidBones);
        const float pastIndex = MostAlong(r.round, laidBones, g_finger[1], false) - MostAlong(r.round, laidBones, g_finger[0], false);
        const float score = fingersScore + lay.score - std::fabs(roll) * 0.08f - std::max(spread - 45.0f, 0.0f) * 0.4f - std::max(out - 55.0f, 0.0f) * 0.4f -
                            std::fabs(tiltDeg) * 1.0f - std::fabs(turnDeg - 15.0f) * 0.3f - std::max(pastIndex, 0.0f) * 1000.0f * g_thumbPast;
        if (score > best.score) {
            best.pose = p;
            best.score = score;
            best.reach[0] = reach;
            best.across = across;
        }
    };
    for (float spread = -15.0f; spread <= 60.0f; spread += 15.0f)
        for (float out = 20.0f; out <= 95.0f; out += 15.0f)
            for (float roll = -30.0f; roll <= 90.0f; roll += 30.0f)
                tryThumb(spread, out, roll);
    if (best.score > -1e29f) {
        const float s0 = best.pose.thumbSpread, o0 = best.pose.thumbOut, r0 = best.pose.thumbRoll;
        for (float spread = s0 - 10.0f; spread <= s0 + 10.01f; spread += 5.0f)
            for (float out = o0 - 10.0f; out <= o0 + 10.01f; out += 5.0f)
                for (float roll = r0 - 20.0f; roll <= r0 + 20.01f; roll += 10.0f)
                    tryThumb(spread, out, roll);
    }
    if (g_why)
        std::printf("the thumb laid %d ways\n", laid);
    if (best.score < -1e29f)
        return false;
    r = best;
    Ends(r);
    // The nose out past the thumb and index side by 3 to 5 cm.
    const float out = r.noseOut * 1000.0f;
    r.score -= std::max(30.0f - out, 0.0f) * 3.0f + std::max(out - 50.0f, 0.0f) * 3.0f;
    return true;
}

// ---- Reports ----
const char* const kName[5] = {"thumb", "index", "middle", "ring", "little"};

struct Check {
    float deepest = 0;      // the deepest any of the glove goes into the round (m)
    int deepestBone = -1;
    float gap[5] = {};      // each finger's nearest (m; negative in)
    float segGap[5][4] = {};
    float thumbOnFingers = 0;  // the thumb's nearest to the fingers' capsules (m; negative in)
    float deepestPalm = 0;
    float deepestFaces = 0;    // the deepest against the round's own flat faces (its outline runs through their corners, up to 0.16 mm outside them)
};

// How far p is from the round's own triangles, placed (m; negative in it: by their winding number, 1 inside a closed mesh, 0 outside).
float FacesDistance(const std::vector<float>& at, const float* p) {
    float best = 1e9f;
    double winding = 0.0;
    for (size_t t = 0; t + 2 < g_round.tris.size(); t += 3) {
        const float *a = &at[g_round.tris[t] * 3], *b = &at[g_round.tris[t + 1] * 3], *c = &at[g_round.tris[t + 2] * 3];
        // The nearest point of the triangle (Ericson's, Real-Time Collision Detection 5.1.5).
        const float ab[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]}, ac[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]}, ap[3] = {p[0] - a[0], p[1] - a[1], p[2] - a[2]};
        const float bp[3] = {p[0] - b[0], p[1] - b[1], p[2] - b[2]}, cp[3] = {p[0] - c[0], p[1] - c[1], p[2] - c[2]};
        const float d1 = Dot(ab, ap), d2 = Dot(ac, ap), d3 = Dot(ab, bp), d4 = Dot(ac, bp), d5 = Dot(ab, cp), d6 = Dot(ac, cp);
        const float vc = d1 * d4 - d3 * d2, vb = d5 * d2 - d1 * d6, va = d3 * d6 - d5 * d4;
        float q[3];
        if (d1 <= 0 && d2 <= 0)
            std::memcpy(q, a, sizeof(q));
        else if (d3 >= 0 && d4 <= d3)
            std::memcpy(q, b, sizeof(q));
        else if (d6 >= 0 && d5 <= d6)
            std::memcpy(q, c, sizeof(q));
        else if (vc <= 0 && d1 >= 0 && d3 <= 0)
            for (int k = 0; k < 3; ++k)
                q[k] = a[k] + ab[k] * (d1 / (d1 - d3));
        else if (vb <= 0 && d2 >= 0 && d6 <= 0)
            for (int k = 0; k < 3; ++k)
                q[k] = a[k] + ac[k] * (d2 / (d2 - d6));
        else if (va <= 0 && d4 - d3 >= 0 && d5 - d6 >= 0)
            for (int k = 0; k < 3; ++k)
                q[k] = b[k] + (c[k] - b[k]) * ((d4 - d3) / ((d4 - d3) + (d5 - d6)));
        else
            for (int k = 0; k < 3; ++k)
                q[k] = a[k] + ab[k] * (vb / (va + vb + vc)) + ac[k] * (vc / (va + vb + vc));
        const float d[3] = {p[0] - q[0], p[1] - q[1], p[2] - q[2]};
        best = std::min(best, std::sqrt(Dot(d, d)));
        // The solid angle it makes at p (Van Oosterom and Strackee), over 4 pi.
        const double ra[3] = {-ap[0], -ap[1], -ap[2]}, rb[3] = {-bp[0], -bp[1], -bp[2]}, rc[3] = {-cp[0], -cp[1], -cp[2]};
        const double la = std::sqrt(ra[0] * ra[0] + ra[1] * ra[1] + ra[2] * ra[2]), lb = std::sqrt(rb[0] * rb[0] + rb[1] * rb[1] + rb[2] * rb[2]),
                     lc = std::sqrt(rc[0] * rc[0] + rc[1] * rc[1] + rc[2] * rc[2]);
        const double triple = ra[0] * (rb[1] * rc[2] - rb[2] * rc[1]) - ra[1] * (rb[0] * rc[2] - rb[2] * rc[0]) + ra[2] * (rb[0] * rc[1] - rb[1] * rc[0]);
        const double denom = la * lb * lc + (ra[0] * rb[0] + ra[1] * rb[1] + ra[2] * rb[2]) * lc + (rb[0] * rc[0] + rb[1] * rc[1] + rb[2] * rc[2]) * la +
                             (rc[0] * ra[0] + rc[1] * ra[1] + rc[2] * ra[2]) * lb;
        winding += 2.0 * std::atan2(triple, denom) / (4.0 * 3.14159265358979);
    }
    return std::fabs(winding) > 0.5 ? -best : best;
}

Check Measure(const Result& r) {
    Check c;
    float bones[handpose::kBones][12];
    handpose::PoseJoints(r.pose, kWidth, bones);
    Target t;
    t.round = &r.round;
    std::vector<float> roundAt(g_round.pos.size());
    for (size_t v = 0; v * 3 < g_round.pos.size(); ++v)
        Apply(r.round.place, &g_round.pos[v * 3], &roundAt[v * 3]);
    c.deepest = c.deepestFaces = -1e9f;
    for (int i : g_all) {
        float q[3];
        PosePoint(bones, i, q);
        const float d = -r.round.Distance(q);
        if (d > c.deepest)
            c.deepest = d, c.deepestBone = MainBone(g_glove[i]);
        if (d > -0.003f)  // (near it: the faces can only be nearer than the outline)
            c.deepestFaces = std::max(c.deepestFaces, -FacesDistance(roundAt, q));
    }
    for (int f = 0; f < 5; ++f) {
        c.gap[f] = GapOf(t, bones, g_part[f + 1]);
        for (int d = 1; d <= 3; ++d)
            c.segGap[f][d] = GapOf(t, bones, g_seg[f][d]);
    }
    c.deepestPalm = -GapOf(t, bones, g_part[0]);
    Target fingers;
    Round far;  // (the round out of the way: the fingers alone)
    const float away[3] = {1e3f, 0.0f, 0.0f};
    PlaceRound(far, away, r.round.D, 0.0f);
    fingers.round = &far;
    FingerCapsules(bones, fingers.fingers);
    c.thumbOnFingers = GapOf(fingers, bones, g_part[1]);
    return c;
}

void Print(const Result& r) {
    std::printf("turn %2.0f tilt %3.0f along %6.3f palm %.3f | round: thumb %3.0f, fingers %3.0f %3.0f %3.0f %3.0f | thumb across %.2f | nose out %.1f cm (index %.1f, thumb %.1f, "
                "index knuckle %.1f, all the hand %.1f), base out %.1f mm (all the hand %.1f) | thumb spread %.0f out %.0f roll %.0f base %.0f knuckle %.0f last %.0f | "
                "index %.0f %.0f %.0f middle %.0f %.0f %.0f ring %.0f %.0f %.0f little %.0f %.0f %.0f | score %.0f\n",
                r.turn, r.tilt, r.along, r.h, r.reach[0], r.reach[1], r.reach[2], r.reach[3], r.reach[4], r.across, r.noseOut * 100, r.noseOutIndex * 100, r.noseOutThumb * 100,
                r.noseOutKnuckle * 100, r.noseOutHand * 100, r.baseOut * 1000, r.baseOutHand * 1000, r.pose.thumbSpread, r.pose.thumbOut, r.pose.thumbRoll, r.pose.thumbBase, r.pose.thumbKnuckle, r.pose.thumbLast,
                r.pose.finger[0][0], r.pose.finger[0][1], r.pose.finger[0][2], r.pose.finger[1][0], r.pose.finger[1][1], r.pose.finger[1][2], r.pose.finger[2][0],
                r.pose.finger[2][1], r.pose.finger[2][2], r.pose.finger[3][0], r.pose.finger[3][1], r.pose.finger[3][2], r.score);
}

void PrintGaps(const Result& r) {
    const Check c = Measure(r);
    std::printf("   nearest mm (negative in):");
    for (int f = 0; f < 5; ++f)
        std::printf(" %s %.1f (parts %.1f %.1f %.1f);", kName[f], c.gap[f] * 1000, c.segGap[f][1] * 1000, c.segGap[f][2] * 1000, c.segGap[f][3] * 1000);
    std::printf(" palm in %.1f; the whole glove's deepest %.2f mm (bone %d; %.2f into its faces); the thumb on the fingers %.1f\n", c.deepestPalm * 1000, c.deepest * 1000,
                c.deepestBone, c.deepestFaces * 1000, c.thumbOnFingers * 1000);
}

// ---- Pictures ----
void Draw(const std::vector<Result>& fits, const char* path) {
    constexpr int kViews = 7;
    Image img(kTile * kViews, kTile * static_cast<int>(fits.size()));
    constexpr float kFingerColour[6][3] = {{0.6f, 0.5f, 0.4f}, {0.9f, 0.25f, 0.2f}, {0.95f, 0.55f, 0.15f},
                                           {0.9f, 0.85f, 0.2f}, {0.3f, 0.8f, 0.3f}, {0.3f, 0.5f, 0.95f}};
    constexpr float kBody[3] = {0.55f, 0.56f, 0.6f}, kNose[3] = {0.42f, 0.48f, 0.22f}, kBase[3] = {0.8f, 0.62f, 0.3f};
    const float scale = kTile / 0.20f;
    for (int row = 0; row < static_cast<int>(fits.size()); ++row) {
        const Result& r = fits[row];
        float bones[handpose::kBones][12];
        handpose::PoseJoints(r.pose, kWidth, bones);
        std::vector<float> placed(g_glove.size() * 3);
        for (size_t i = 0; i < g_glove.size(); ++i)
            PosePoint(bones, static_cast<int>(i), &placed[i * 3]);
        std::vector<float> roundAt(g_round.pos.size());
        for (size_t v = 0; v * 3 < g_round.pos.size(); ++v)
            Apply(r.round.place, &g_round.pos[v * 3], &roundAt[v * 3]);
        const float* D = r.round.D;
        float side[3], up[3];
        Cross(D, kPalm, side);
        Normalize(side);
        Cross(side, D, up);
        Normalize(up);
        // Between the hand's middle and the round's.
        const float mid = (r.round.baseAlong + r.round.NoseAlong()) / 2;
        float centre[3];
        for (int k = 0; k < 3; ++k)
            centre[k] = (r.round.O[k] + D[k] * mid + kMiddle[k]) / 2;
        const float* P = kPalm;
        // Down the round from its nose and from its base, the back of the hand, the palm's side, along the palm both ways, and from behind and above, as the player sees it.
        const View views[kViews] = {MakeView(D[0], D[1], D[2], up[0], up[1], up[2]),
                                    MakeView(-D[0], -D[1], -D[2], up[0], up[1], up[2]),
                                    MakeView(P[0], P[1], P[2], D[0], D[1], D[2]),
                                    MakeView(-P[0], -P[1], -P[2], D[0], D[1], D[2]),
                                    MakeView(side[0], side[1], side[2], D[0], D[1], D[2]),
                                    MakeView(-side[0], -side[1], -side[2], D[0], D[1], D[2]),
                                    MakeView(-D[0] + P[0] * 0.5f - side[0] * 0.3f, -D[1] + P[1] * 0.5f - side[1] * 0.3f, -D[2] + P[2] * 0.5f - side[2] * 0.3f, -P[0], -P[1], -P[2])};
        for (int v = 0; v < kViews; ++v) {
            for (size_t t = 0; t + 2 < g_round.tris.size(); t += 3) {
                const int a = g_round.tris[t], b = g_round.tris[t + 1], c = g_round.tris[t + 2];
                const float zMax = std::max({g_round.pos[a * 3 + 2], g_round.pos[b * 3 + 2], g_round.pos[c * 3 + 2]});
                const float* colour = zMax < g_profile.base + 0.0015f ? kBase : zMax > g_profile.bodyEnd + 0.0005f ? kNose : kBody;
                DrawTriangle(img, v * kTile, row * kTile, views[v], centre, scale, &roundAt[a * 3], &roundAt[b * 3], &roundAt[c * 3], colour);
            }
            for (size_t t = 0; t + 2 < g_gloveTris.size(); t += 3)
                DrawTriangle(img, v * kTile, row * kTile, views[v], centre, scale, &placed[g_gloveTris[t] * 3], &placed[g_gloveTris[t + 1] * 3],
                             &placed[g_gloveTris[t + 2] * 3], kFingerColour[g_glove[g_gloveTris[t]].finger + 1]);
        }
    }
    img.Save(path);
    std::printf("saved %s\n", path);
}

// One fit drawn big: eight views in two rows, 560 pixels each, 17 cm across.
struct Canvas {
    int w, h;
    std::vector<float> rgb, depth;
    Canvas(int w, int h) : w(w), h(h), rgb(static_cast<size_t>(w) * h * 3, 0.12f), depth(static_cast<size_t>(w) * h, 1e9f) {}
    void Triangle(int tx, int ty, int tile, const View& view, const float* centre, float scale, const float* a, const float* b, const float* c, const float* colour) {
        const float* p[3] = {a, b, c};
        float sx[3], sy[3], sz[3];
        for (int i = 0; i < 3; ++i) {
            const float d[3] = {p[i][0] - centre[0], p[i][1] - centre[1], p[i][2] - centre[2]};
            sx[i] = tx + tile / 2 + Dot(d, view.right) * scale;
            sy[i] = ty + tile / 2 - Dot(d, view.up) * scale;
            sz[i] = Dot(d, view.forward);
        }
        float e1[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]}, e2[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]}, nrm[3];
        Cross(e1, e2, nrm);
        Normalize(nrm);
        float light[3];
        for (int i = 0; i < 3; ++i)
            light[i] = -view.forward[i] * 0.7f + view.up[i] * 0.6f + view.right[i] * 0.3f;
        Normalize(light);
        const float shade = 0.25f + 0.55f * std::fabs(Dot(nrm, light)) + 0.2f * std::fabs(Dot(nrm, view.forward));
        const int x0 = std::max(tx, static_cast<int>(std::floor(std::min({sx[0], sx[1], sx[2]}))));
        const int x1 = std::min(tx + tile - 1, static_cast<int>(std::ceil(std::max({sx[0], sx[1], sx[2]}))));
        const int y0 = std::max(ty, static_cast<int>(std::floor(std::min({sy[0], sy[1], sy[2]}))));
        const int y1 = std::min(ty + tile - 1, static_cast<int>(std::ceil(std::max({sy[0], sy[1], sy[2]}))));
        const float area = (sx[1] - sx[0]) * (sy[2] - sy[0]) - (sx[2] - sx[0]) * (sy[1] - sy[0]);
        if (std::fabs(area) < 1e-6f)
            return;
        for (int y = y0; y <= y1; ++y)
            for (int x = x0; x <= x1; ++x) {
                const float px = x + 0.5f, py = y + 0.5f;
                const float w0 = ((sx[1] - px) * (sy[2] - py) - (sx[2] - px) * (sy[1] - py)) / area;
                const float w1 = ((sx[2] - px) * (sy[0] - py) - (sx[0] - px) * (sy[2] - py)) / area;
                const float w2 = 1 - w0 - w1;
                if (w0 < 0 || w1 < 0 || w2 < 0)
                    continue;
                const float z = w0 * sz[0] + w1 * sz[1] + w2 * sz[2];
                float& depthAt = depth[static_cast<size_t>(y) * w + x];
                if (z >= depthAt)
                    continue;
                depthAt = z;
                for (int k = 0; k < 3; ++k)
                    rgb[(static_cast<size_t>(y) * w + x) * 3 + k] = colour[k] * shade;
            }
    }
    void Save(const char* path) const {
        Image img(w, h);
        img.rgb = rgb;
        img.Save(path);
    }
};

void DrawBig(const Result& r, const char* path) {
    constexpr int kSide = 560, kColumns = 4, kRows = 2;
    Canvas img(kSide * kColumns, kSide * kRows);
    constexpr float kFingerColour[6][3] = {{0.6f, 0.5f, 0.4f}, {0.9f, 0.25f, 0.2f}, {0.95f, 0.55f, 0.15f},
                                           {0.9f, 0.85f, 0.2f}, {0.3f, 0.8f, 0.3f}, {0.3f, 0.5f, 0.95f}};
    constexpr float kBody[3] = {0.55f, 0.56f, 0.6f}, kNose[3] = {0.42f, 0.48f, 0.22f}, kBase[3] = {0.8f, 0.62f, 0.3f};
    const float scale = kSide / 0.17f;
    float bones[handpose::kBones][12];
    handpose::PoseJoints(r.pose, kWidth, bones);
    std::vector<float> placed(g_glove.size() * 3);
    for (size_t i = 0; i < g_glove.size(); ++i)
        PosePoint(bones, static_cast<int>(i), &placed[i * 3]);
    std::vector<float> roundAt(g_round.pos.size());
    for (size_t v = 0; v * 3 < g_round.pos.size(); ++v)
        Apply(r.round.place, &g_round.pos[v * 3], &roundAt[v * 3]);
    const float* D = r.round.D;
    const float* P = kPalm;
    float E[3];  // along the fingers, square to the round and the palm
    Cross(P, D, E);
    Normalize(E);
    if (Dot(E, kAlong) < 0)
        for (float& v : E)
            v = -v;
    const float mid = (r.round.baseAlong + r.round.NoseAlong()) / 2;
    float centre[3];
    for (int k = 0; k < 3; ++k)
        centre[k] = (r.round.O[k] + D[k] * mid) * 0.6f + kMiddle[k] * 0.4f + E[k] * 0.01f;
    auto mix = [](const float* a, float x, const float* b, float y, const float* c, float z, float* out) {
        for (int k = 0; k < 3; ++k)
            out[k] = a[k] * x + b[k] * y + c[k] * z;
    };
    float f6[3], f7[3];
    mix(D, -0.55f, P, 0.65f, E, -0.5f, f6);  // from above the back of the hand at the wrist, the nose's side: as the player looks down at the hand
    mix(D, 0.5f, P, -0.6f, E, -0.6f, f7);    // from the palm's side, the little finger's end
    const View views[kColumns * kRows] = {
        MakeView(P[0], P[1], P[2], E[0], E[1], E[2]),       // the back of the hand
        MakeView(-P[0], -P[1], -P[2], E[0], E[1], E[2]),    // the palm's side, through the round
        MakeView(D[0], D[1], D[2], E[0], E[1], E[2]),       // from the nose: down the round
        MakeView(-D[0], -D[1], -D[2], E[0], E[1], E[2]),    // from the base
        MakeView(-E[0], -E[1], -E[2], P[0], P[1], P[2]),    // from in front of the fingertips
        MakeView(E[0], E[1], E[2], -P[0], -P[1], -P[2]),    // from the wrist
        MakeView(f6[0], f6[1], f6[2], E[0], E[1], E[2]),
        MakeView(f7[0], f7[1], f7[2], E[0], E[1], E[2]),
    };
    for (int v = 0; v < kColumns * kRows; ++v) {
        const int tx = (v % kColumns) * kSide, ty = (v / kColumns) * kSide;
        for (size_t t = 0; t + 2 < g_round.tris.size(); t += 3) {
            const int a = g_round.tris[t], b = g_round.tris[t + 1], c = g_round.tris[t + 2];
            const float zMax = std::max({g_round.pos[a * 3 + 2], g_round.pos[b * 3 + 2], g_round.pos[c * 3 + 2]});
            const float* colour = zMax < g_profile.base + 0.0015f ? kBase : zMax > g_profile.bodyEnd + 0.0005f ? kNose : kBody;
            img.Triangle(tx, ty, kSide, views[v], centre, scale, &roundAt[a * 3], &roundAt[b * 3], &roundAt[c * 3], colour);
        }
        constexpr float kGlove[3] = {0.52f, 0.45f, 0.34f};
        for (size_t t = 0; t + 2 < g_gloveTris.size(); t += 3)
            img.Triangle(tx, ty, kSide, views[v], centre, scale, &placed[g_gloveTris[t] * 3], &placed[g_gloveTris[t + 1] * 3], &placed[g_gloveTris[t + 2] * 3],
                         g_plain ? kGlove : kFingerColour[g_glove[g_gloveTris[t]].finger + 1]);
    }
    img.Save(path);
    std::printf("saved %s\n", path);
}

// ---- The tables ----
void WriteTables(const Result& r, const char* path, const std::string& command) {
    float bones[handpose::kBones][12];
    handpose::PoseJoints(r.pose, kWidth, bones);
    const Check c = Measure(r);
    // Where along the round each finger lies (its nearest part's place on the round's bone).
    float lieAt[5] = {};
    for (int f = 0; f < 5; ++f) {
        float best = 1e9f;
        for (int i : g_part[f + 1]) {
            float q[3], b[3];
            PosePoint(bones, i, q);
            const float d = r.round.Distance(q);
            if (d < best) {
                best = d;
                Apply(r.round.inverse, q, b);
                lieAt[f] = b[2];
            }
        }
    }
    FILE* f = nullptr;
    if (fopen_s(&f, path, "w") || !f)
        return;
    const Profile& p = g_profile;
    std::fprintf(f, "#pragma once\n\n");
    std::fprintf(f, "// The left hand carrying a 40 mm round in its fist, away from the launcher (from the belly to the barrel, or a spent case pulled out): the user, 2026-11-05,\n");
    std::fprintf(f, "// of the round perched on the game's rim pinch, \"I want it to actually look like he's holding the shell ... I just need his thumb and his finger to sort of\n");
    std::fprintf(f, "// wrap around the shell.\" A power grip fitted on the meshes offline by tools\\roundfit.cpp (tools\\rocketfit.cpp's way): the palm against the round's body, every\n");
    std::fprintf(f, "// joint of the four fingers closed round it one way until what it moves touches, the thumb round it the other way until it touches the round or the fingers,\n");
    std::fprintf(f, "// each tip brought down onto it. The round lies across the fist, its base (the rim) at the little finger's side, its nose out past the thumb and index side, so\n");
    std::fprintf(f, "// the hand pushes it nose first into the breech.\n");
    std::fprintf(f, "// The round's mesh (draw 1440 from 7848) measured on its bone: a straight body of radius %.1f mm (%.1f cm across) about x %.4f, y %.4f from its base at z %.4f\n",
                 p.body * 1000, p.body * 200, p.axis[0], p.axis[1], p.base);
    std::fprintf(f, "// to %.4f, then the ogive nose to its tip at %.4f (its radius", p.z.size() > 1 ? p.z[1] : p.bodyEnd, p.nose);
    for (size_t i = 2; i + 1 < p.z.size(); ++i)
        std::fprintf(f, " %.1f mm at %.4f%s", p.r[i] * 1000, p.z[i], i + 2 < p.z.size() ? "," : ").\n");
    std::fprintf(f, "// The fist closes on the body, a cylinder of that radius; the glove is kept out of the round's whole outline (its base, body and nose) but for a glove's\n");
    std::fprintf(f, "// give of 2.5 mm. The round's line as the mod has it (x 0.0003, y -0.0584: m320.cpp kRoundBaseOnBone, kRoundNoseOnBone) runs exactly along the fitted\n");
    std::fprintf(f, "// axis; the mesh's own middle line is %.1f mm off it, and the numbers here are for the round as drawn.\n",
                 std::hypot(p.axis[0] - kLine[0], p.axis[1] - kLine[1]) * 1000);
    std::fprintf(f, "// Its turn about its own length (free) is the game's pinch's as near as goes (frame 7350), so going over from the pinch to the carry doesn't spin it.\n");
    std::fprintf(f, "// Placement \"pick %g %g %g\": the round %g degrees off straight across the hand (its base's end towards the wrist), tilted %g out of the palm's plane, %.1f cm\n",
                 r.turn, r.tilt, r.along, r.turn, r.tilt, r.along * 100);
    std::fprintf(f, "// from the hand's middle towards the fingers; the palm %.1f mm into it. Round it from the palm: the fingers' tips %.0f %.0f %.0f %.0f degrees, the thumb's %.0f the\n",
                 c.deepestPalm * 1000, r.reach[1], r.reach[2], r.reach[3], r.reach[4], r.reach[0]);
    std::fprintf(f, "// other way. The deepest the glove goes into the round, anywhere (all 23 bones' vertices): %.2f mm (bone %d) into its outline, %.2f into its own flat faces.\n",
                 c.deepest * 1000, c.deepestBone, c.deepestFaces * 1000);
    std::fprintf(f, "// Each finger's nearest (mm; negative in): thumb %.1f, index %.1f, middle %.1f, ring %.1f, little %.1f; by part, from the palm out (each part without\n",
                 c.gap[0] * 1000, c.gap[1] * 1000, c.gap[2] * 1000, c.gap[3] * 1000, c.gap[4] * 1000);
    std::fprintf(f, "// the stretch its joint's bend pinches):");
    for (int k = 0; k < 5; ++k)
        std::fprintf(f, " %s %.1f %.1f %.1f%s", kName[k], c.segGap[k][1] * 1000, c.segGap[k][2] * 1000, c.segGap[k][3] * 1000, k == 4 ? ".\n" : ",");
    std::fprintf(f, "// The thumb's nearest to the fingers %.1f mm. Along the round (z on its bone) the fingers lie at: index %.3f, middle %.3f, ring %.3f, little %.3f, thumb %.3f.\n",
                 c.thumbOnFingers * 1000, lieAt[1], lieAt[2], lieAt[3], lieAt[4], lieAt[0]);
    std::fprintf(f, "// The nose's tip stands %.1f cm out past the index knuckle line (the plane through the index knuckle's middle, square to the round), %.1f cm past the fist on\n",
                 r.noseOutKnuckle * 100, r.noseOut * 100);
    std::fprintf(f, "// the thumb and index side (%.1f past the index finger's outer side, %.1f past the thumb); the base %.1f mm out past the little finger. The fist covers %.1f cm\n",
                 r.noseOutIndex * 100, r.noseOutThumb * 100, r.baseOut * 1000,
                 ((r.round.baseAlong - r.baseOut) - (r.round.NoseAlong() + r.noseOutIndex)) * 100);
    std::fprintf(f, "// of the round's %.1f (the glove's fingers are 2.6 cm wide and 3.5 thick), so with the base flush the nose can't stand further out past it: \"base=-17\" tucks\n",
                 (kNoseZ - kBaseZ) * 100);
    std::fprintf(f, "// the base 1.7 cm in under the little finger, which then curls over its rim, for 2.9 cm.\n");
    std::fprintf(f, "// Thumb: spread %g, out %g, roll %g, base %.1f, knuckle %.1f, last %.1f. Fingers (knuckle, middle, last): index %.1f %.1f %.1f, middle %.1f %.1f %.1f,\n",
                 r.pose.thumbSpread, r.pose.thumbOut, r.pose.thumbRoll, r.pose.thumbBase, r.pose.thumbKnuckle, r.pose.thumbLast, r.pose.finger[0][0], r.pose.finger[0][1],
                 r.pose.finger[0][2], r.pose.finger[1][0], r.pose.finger[1][1], r.pose.finger[1][2]);
    std::fprintf(f, "// ring %.1f %.1f %.1f, little %.1f %.1f %.1f; their turn towards the thumb at the knuckles %g %g %g %g.\n", r.pose.finger[2][0], r.pose.finger[2][1],
                 r.pose.finger[2][2], r.pose.finger[3][0], r.pose.finger[3][1], r.pose.finger[3][2], r.pose.fingerApart[0], r.pose.fingerApart[1], r.pose.fingerApart[2],
                 r.pose.fingerApart[3]);
    std::fprintf(f, "// Bone 1, the palm, is handpose::Pose's at width 1.15 (PoseJoints doesn't move it), so hands.cpp's re-anchoring on the palm leaves the round where it was fitted.\n");
    std::fprintf(f, "// Made again by, from the project's folder in an x86 developer prompt (no CMake target: the running game held the mod's DLL):\n");
    std::fprintf(f, "//   cl /nologo /EHsc /O2 /std:c++17 /DNOMINMAX /DWIN32_LEAN_AND_MEAN /I src /I tools /Febuild\\roundfit.exe /Fobuild\\ tools\\roundfit.cpp src\\handpose.cpp\n");
    std::fprintf(f, "//   %s\n\n", command.c_str());
    std::fprintf(f, "namespace m320carry {\n\n");
    std::fprintf(f, "// The round's bone (draw 1440's bone 0, the space kRoundBaseOnBone and kRoundNoseOnBone are in) into the left glove's bind space (3x4 rows: rotation | translation,\n");
    std::fprintf(f, "// rigid).\n");
    std::fprintf(f, "constexpr float kRoundInGlove[12] = {");
    for (int k = 0; k < 12; ++k)
        std::fprintf(f, "%.6ff%s", r.round.place[k], k == 11 ? "};\n\n" : ", ");
    std::fprintf(f, "// Each left glove bone (0-22) as handpose::PoseJoints gives it at width 1.15 (bind space -> glove space): the fitted carry grip.\n");
    std::fprintf(f, "constexpr float kFingers[23][12] = {\n");
    for (int b = 0; b < handpose::kBones; ++b) {
        std::fprintf(f, "    {");
        for (int k = 0; k < 12; ++k)
            std::fprintf(f, "%.6ff%s", bones[b][k], k == 11 ? "},\n" : ", ");
    }
    std::fprintf(f, "};\n\n}  // namespace m320carry\n");
    fclose(f);
    std::printf("wrote %s\n", path);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: roundfit <build folder> <out.bmp> [pick turn tilt along out.h | why turn tilt along] [base=mm apart=i,m,r,l pad=mm palmin=mm thumbpast=n "
                             "turns=a:b:s tilts=a:b:s alongs=a:b:s]\n");
        return 1;
    }
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    const std::string dir = argv[1];
    std::string options;
    for (int i = 3; i < argc; ++i) {
        if (!std::strncmp(argv[i], "base=", 5))
            g_baseOut = static_cast<float>(std::atof(argv[i] + 5)) * 0.001f, options += std::string(" ") + argv[i];
        if (!std::strcmp(argv[i], "plain"))
            g_plain = true;
        if (!std::strncmp(argv[i], "thumbpast=", 10))
            g_thumbPast = static_cast<float>(std::atof(argv[i] + 10)), options += std::string(" ") + argv[i];
        if (!std::strncmp(argv[i], "palmin=", 7))
            g_palmIn = static_cast<float>(std::atof(argv[i] + 7)) * 0.001f, options += std::string(" ") + argv[i];
        if (!std::strncmp(argv[i], "pad=", 4))
            g_padSink = static_cast<float>(std::atof(argv[i] + 4)) * 0.001f, options += std::string(" ") + argv[i];
        if (!std::strncmp(argv[i], "apart=", 6)) {
            std::sscanf(argv[i] + 6, "%f,%f,%f,%f", &g_apart[0], &g_apart[1], &g_apart[2], &g_apart[3]);
            options += std::string(" ") + argv[i];
        }
    }
    if (!LoadGlove(dir, g_glove, g_gloveTris)) {
        std::fprintf(stderr, "the gloves' mesh dump isn't in %s\n", dir.c_str());
        return 1;
    }
    if (!MeasureRound(dir)) {
        std::fprintf(stderr, "the round's mesh dump (mesh_1440_7848_*) isn't in %s\n", dir.c_str());
        return 1;
    }
    {
        const Profile& p = g_profile;
        std::printf("the round on its bone: its line at x %.4f y %.4f (the mod's 0.0003 -0.0584, %.2f mm off), base %.4f, body radius %.4f to %.4f, tip %.4f; outline:", p.axis[0],
                    p.axis[1], std::hypot(p.axis[0] - kLine[0], p.axis[1] - kLine[1]) * 1000, p.base, p.body, p.bodyEnd, p.nose);
        for (size_t i = 0; i < p.z.size(); ++i)
            std::printf(" (%.4f %.4f)", p.z[i], p.r[i]);
        std::printf("\n");
    }
    handpose::Joints(g_joints);
    for (int b = 0; b < handpose::kBones; ++b)
        if (kFinger[b] >= 0 && kDepth[b] >= 1)
            g_boneOf[kFinger[b]][kDepth[b]] = b;
    std::vector<char> drawn(g_glove.size(), 0);
    for (int i : g_gloveTris)
        drawn[i] = 1;
    for (size_t i = 0; i < g_glove.size(); ++i) {
        if (!drawn[i] || g_glove[i].finger < -1)
            continue;
        const int vi = static_cast<int>(i);
        g_all.push_back(vi);
        g_part[g_glove[i].finger + 1].push_back(vi);
        const int b = MainBone(g_glove[i]);
        if (g_glove[i].finger >= 0 && kDepth[b] >= (g_glove[i].finger == 0 ? 2 : 1))
            g_finger[g_glove[i].finger].push_back(vi);
        // What each joint moves: any weight on a bone of that finger that far out or further, but not a part past it.
        for (int f = 0; f < 5; ++f)
            for (int d = 1; d <= 3; ++d) {
                bool moved = false;
                for (int s = 0; s < 4; ++s)
                    if (g_glove[i].weight[s] > 0 && g_glove[i].bone[s] < handpose::kBones && kFinger[g_glove[i].bone[s]] == f && kDepth[g_glove[i].bone[s]] >= d)
                        moved = true;
                const bool mine = kFinger[b] == f ? kDepth[b] <= d : d == 1;
                if (moved && mine)
                    g_moves[f][d].push_back(vi);
            }
        // Past the joint before it by this much, a part's own: a first part's root sits in the palm, and a joint's bend pinches its inside.
        const int f = g_glove[i].finger;
        if (f >= 0 && kDepth[b] >= 1) {
            const float* j0 = g_joints[f][kDepth[b] - 1];
            const float dx = g_glove[i].pos[0] - j0[0], dy = g_glove[i].pos[1] - j0[1], dz = g_glove[i].pos[2] - j0[2];
            const float* j1 = g_joints[f][kDepth[b]];
            const float ex = j1[0] - j0[0], ey = j1[1] - j0[1], ez = j1[2] - j0[2];
            const float length = std::sqrt(ex * ex + ey * ey + ez * ez);
            const float margin = kDepth[b] >= 2 ? length * 0.5f : f == 0 ? 0.025f : 0.018f;
            if (std::sqrt(dx * dx + dy * dy + dz * dz) > margin) {
                g_seg[f][kDepth[b]].push_back(vi);
                if (kDepth[b] == 3 && std::sqrt(dx * dx + dy * dy + dz * dz) > length * 0.75f)
                    g_tipPart[f].push_back(vi);
            }
        }
    }
    // Each finger part's thickness: how far its own vertices are from its middle line, the hand open (three quarters of them nearer).
    {
        handpose::JointPose open;
        float bones[handpose::kBones][12];
        handpose::PoseJoints(open, kWidth, bones);
        for (int f = 1; f < 5; ++f)
            for (int d = 1; d <= 3; ++d) {
                float a[3], b[3];
                JointAt(bones, f, d - 1, a);
                JointAt(bones, f, d, b);
                std::vector<float> dist;
                for (int i : g_seg[f][d]) {
                    float q[3];
                    PosePoint(bones, i, q);
                    dist.push_back(FromSegment(q, a, b));
                }
                std::sort(dist.begin(), dist.end());
                g_partRadius[f][d] = dist.empty() ? 0.008f : dist[dist.size() * 3 / 4];
            }
        std::printf("the glove: %zu vertices; the fingers' parts' thickness (mm):", g_all.size());
        for (int f = 1; f < 5; ++f)
            std::printf(" %s %.1f %.1f %.1f", kName[f], g_partRadius[f][1] * 1000, g_partRadius[f][2] * 1000, g_partRadius[f][3] * 1000);
        std::printf("\n");
    }

    if (argc > 6 && !std::strcmp(argv[3], "why")) {
        g_why = true;
        Result r;
        const bool ok = FitAt(static_cast<float>(std::atof(argv[4])), static_cast<float>(std::atof(argv[5])), static_cast<float>(std::atof(argv[6])), r);
        std::printf("%s\n", ok ? "fits" : "doesn't");
        if (ok) {
            Print(r);
            PrintGaps(r);
            DrawBig(r, argv[2]);
        }
        return 0;
    }

    if (argc > 7 && !std::strcmp(argv[3], "pick")) {
        Result r;
        if (!FitAt(static_cast<float>(std::atof(argv[4])), static_cast<float>(std::atof(argv[5])), static_cast<float>(std::atof(argv[6])), r)) {
            std::printf("it doesn't go there\n");
            return 1;
        }
        Print(r);
        PrintGaps(r);
        DrawBig(r, argv[2]);
        const std::string command = std::string("build\\roundfit build %TEMP%\\round.bmp pick ") + argv[4] + " " + argv[5] + " " + argv[6] + " src\\m320carry.h" + options;
        WriteTables(r, argv[7], command);
        return 0;
    }

    // Every placement, a thread each in turn; the best of each turn drawn. turns=, tilts=, alongs= <from>:<to>:<step> change the grid.
    float grid[3][3] = {{-20.0f, 40.0f, 10.0f}, {-40.0f, 20.0f, 10.0f}, {-0.02f, 0.03f, 0.01f}};
    for (int i = 3; i < argc; ++i)
        for (int g = 0; g < 3; ++g) {
            const char* name = g == 0 ? "turns=" : g == 1 ? "tilts=" : "alongs=";
            if (!std::strncmp(argv[i], name, std::strlen(name)))
                std::sscanf(argv[i] + std::strlen(name), "%f:%f:%f", &grid[g][0], &grid[g][1], &grid[g][2]);
        }
    std::vector<float> turns, tilts, alongs;
    for (float turn = grid[0][0]; turn <= grid[0][1] + 1e-3f; turn += grid[0][2])
        turns.push_back(turn);
    for (float tilt = grid[1][0]; tilt <= grid[1][1] + 1e-3f; tilt += grid[1][2])
        tilts.push_back(tilt);
    for (float along = grid[2][0]; along <= grid[2][1] + 1e-5f; along += grid[2][2])
        alongs.push_back(along);
    struct Job {
        float turn, tilt, along;
        bool ok = false;
        Result r;
    };
    std::vector<Job> jobs;
    for (float turn : turns)
        for (float tilt : tilts)
            for (float along : alongs)
                jobs.push_back({turn, tilt, along});
    std::atomic<size_t> next{0};
    std::vector<std::thread> workers;
    const unsigned threads = std::max(1u, std::thread::hardware_concurrency());
    for (unsigned w = 0; w < threads; ++w)
        workers.emplace_back([&] {
            for (size_t j; (j = next++) < jobs.size();)
                jobs[j].ok = FitAt(jobs[j].turn, jobs[j].tilt, jobs[j].along, jobs[j].r);
        });
    for (auto& w : workers)
        w.join();
    std::vector<Result> fits;
    for (float turn : turns) {
        Result best;
        for (const Job& j : jobs)
            if (j.turn == turn && j.ok && j.r.score > best.score)
                best = j.r;
        if (best.score > -1e29f) {
            Print(best);
            PrintGaps(best);
            fits.push_back(best);
        }
    }
    for (const Job& j : jobs)
        if (j.ok)
            std::printf("  %2.0f %3.0f %6.3f: score %.0f nose out %.1f cm base out %.1f mm, thumb %.0f round, fingers %.0f %.0f %.0f %.0f\n", j.turn, j.tilt, j.along, j.r.score,
                        j.r.noseOut * 100, j.r.baseOut * 1000, j.r.reach[0], j.r.reach[1], j.r.reach[2], j.r.reach[3], j.r.reach[4]);
        else
            std::printf("  %2.0f %3.0f %6.3f: doesn't go\n", j.turn, j.tilt, j.along);
    if (fits.empty()) {
        std::printf("nothing fitted\n");
        return 1;
    }
    Draw(fits, argv[2]);
    return 0;
}

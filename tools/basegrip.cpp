// Fits the left glove's fingertips on the base end of the grenade launcher's 40 mm round: a pinch, not a fist. The user, 2026-11-05, of the fist round the
// round's body (tools/roundfit.cpp, src/m320carry.h): "you don't need a whole fist ... at most three fingers. And it should be at the end of the shell ... not
// sideways either. He should be holding it from the bottom ... his thumb, his finger, and his middle finger on that circle base. I'm okay with the ring finger
// and pinky folding", with four photographs of a water bottle held that way: only its base end, between the pads of the thumb on one side and the index and
// middle fingers on the other, a centimetre or two from its bottom, the bottle standing straight out from the fingertips away from the palm, nose first, the palm
// clear of it, the ring and little fingers loosely folded and clear.
// So: the round's way in the hand is given (along the fingers, tilted out of the palm's plane by elev, turned towards the little finger by yaw); the index and
// middle fingers are shaped joint by joint; the round is put where both their pads touch it (1 mm in), the index pad's touch zIndex from its base; then the
// thumb, swung out to oppose them, closes at its knuckle and last joint until its pad touches it from the other side. "fit" searches the fingers' and the
// thumb's joints (a pattern search from the variant's start and eleven starts about it) for the most natural hand that does all that with nothing else of the
// glove on the round: no part of the glove more than a glove's give (2.5 mm) into it, the palm, the ring and little fingers clear, each last part touching with
// its pad.
// The glove's thumb (11.4 cm from its base joint, 4.4 cm behind the hand's middle) can't reach across a 5.1 cm round lying in the palm's plane past the
// fingertips: the round lies along the index finger's own line, as the photos have the bottle, 40-85 degrees out of the palm's plane. The four variants:
//   A  photos 2 and 4: straight out along the index's line, the index and middle pads side by side on top of the base end 1 cm up, the thumb's under it.
//   B  A tilted further out of the palm's plane: the base end in the fingertips with its face towards the palm (clear of it), as "holding it from the bottom".
//   C  photo 4, staggered: the index laid along the body (its pad 3 cm up), the middle fingertip curled onto the base's rim, the thumb under between them.
//   D  photos 1 and 3: the index and middle curled over the body, the pads 2 cm up, the round forward and a little down.
//   basegrip <build folder> <out folder> look <key=value ...>          one grip as given, drawn from eight sides (look.bmp) and measured
//   basegrip <build folder> <out folder> fit <A|B|C|D> [key=value ...]  a variant fitted: drawn from eight sides (basegrip_<v>.bmp), its wearer's view and the
//                                                                      photos' views big (_wearer, _photo24, _photo13 .bmp), its tables written in the form of
//                                                                      src/m320carry.h (basegrip_<v>.h) and its numbers in a line (_numbers.txt)
// Keys: elev yaw (degrees) z (the index pad's touch from the base, mm) gap (mm, negative in); iK iM iL iA, mK mM mL mA, rK rM rL rA, lK lM lL lA (each finger's
// knuckle, middle and last joint and its turn towards the thumb, degrees); tS tO tR tB (the thumb's spread, out, roll and base) tShare (its last joint's bend, of
// its knuckle's); plain (the glove in one colour). For fit also: starts=<n> (12 made the headers), nosearch, out=<name> (the files' name for a try), and the
// variant's aims changed for a try: aim.elev aim.yaw aim.elevFree aim.yawFree aim.z aim.zLo aim.zHi aim.midLo aim.midHi (mm) aim.line aim.opLo aim.opHi
// aim.spreadLo aim.spreadHi aim.apart aim.alike aim.thumbOut.
// All in the gloves' bind space. Built without the project's CMake (the mod's DLL is held by the running game), from the project's folder in an x86 prompt:
//   cl /nologo /EHsc /O2 /std:c++17 /DNOMINMAX /DWIN32_LEAN_AND_MEAN /I src /I tools /Febuild\basegrip.exe /Fobuild\ tools\basegrip.cpp src\handpose.cpp
//   build\basegrip build <out folder> fit A starts=12      (and B, C, D: the four headers)

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <random>
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
constexpr float kDeg = kPi / 180.0f;
// The round's line as the mod has it (src/m320.cpp kRoundBaseOnBone, kRoundNoseOnBone): x 0.0003, y -0.0584 on its bone, its base at z 0.2795, its nose at 0.3984.
// (Not constants: "shell12g" among the arguments fits the same pinch on the M1014's 12-gauge shell instead, its own draw 450 from 13701 on its bone 0: its line x -0.0013,
// y -0.1026, its base at z 0.049, its nose at 0.113; the header then goes into src/shellposes.h by hand, its names changed.)
float kLine[2] = {0.0003f, -0.0584f};
float kBaseZ = 0.2795f, kNoseZ = 0.3984f;
bool g_shell12g = false;
// The game's own pinch on the round's rim (src/m320.cpp kBones7350): its palm, bone 1, from the gloves' bind space into the round's bone. The round is turned about
// its own length as near that as goes (tools/roundfit.cpp's RoundTurn), so going over from the game's pinch to this grip swings it without spinning it.
constexpr float kPinchPalm[12] = {0.715837f, 0.657457f, -0.235217f, -0.987313f, -0.607637f, 0.752485f, 0.254053f, -0.632581f, 0.344026f, -0.038934f, 0.938152f, -0.097090f};
const char* const kName[5] = {"thumb", "index", "middle", "ring", "little"};
void HandWays(float* A, float* C, float* P);
struct Grip;
bool SetKey(Grip& g, const std::string& key, float v);

// ---- The round's shape, measured from its mesh (tools/roundfit.cpp's) ----
struct Profile {
    float axis[2] = {};
    std::vector<float> z, r;
    float base = 0.0f, nose = 0.0f, body = 0.0f, bodyEnd = 0.0f;

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
Mesh g_round;
std::vector<int> g_roundUsed;  // the vertices its triangles use (its buffer holds others)

bool MeasureRound(const std::string& dir) {
    if (!(g_shell12g ? LoadDraw(dir, 450, 0, {}, g_round, -1, 0, 13701) : LoadDraw(dir, 1440, 0, {}, g_round, -1, 0, 7848)))
        return false;
    std::vector<char> used(g_round.pos.size() / 3, 0);
    for (int t : g_round.tris)
        used[t] = 1;
    for (size_t v = 0; v < used.size(); ++v)
        if (used[v])
            g_roundUsed.push_back(static_cast<int>(v));
    float lo = 1e9f, hi = -1e9f;
    for (size_t v = 0; v < used.size(); ++v)
        if (used[v])
            lo = std::min(lo, g_round.pos[v * 3 + 2]), hi = std::max(hi, g_round.pos[v * 3 + 2]);
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
    while (hull.size() >= 2 && hull[1].first - hull[0].first < 1e-5f)
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
std::vector<int> g_all;          // every vertex of its drawn triangles on the left glove's 23 bones
std::vector<int> g_part[6];      // by the finger of the bone with the most weight (0 the palm with the cuff, 1 the thumb, 2-5 the fingers)
std::vector<int> g_seg[5][4];    // each finger's parts (1-3) without the vertices near the joint before it, which the joint's bend pinches inwards
std::vector<int> g_pad[5];       // the last part's pad: its palm's side (the thumb's: towards the palm and the fingers), out over the tip
std::vector<int> g_back[5];      // the rest of the last part: its back and sides
float g_padDir[5][3];            // the way each pad faces in the bind space
float g_joints[5][4][3];
int g_boneOf[5][4];
float g_partRadius[5][4];
bool g_plain = false;

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

void PoseAll(const float (*bones)[12], std::vector<float>& out) {
    out.assign(g_glove.size() * 3, 0.0f);
    for (int i : g_all)
        PosePoint(bones, i, &out[i * 3]);
}

void JointAt(const float (*bones)[12], int finger, int joint, float* out) { Apply(bones[g_boneOf[finger][std::max(joint, 1)]], g_joints[finger][joint], out); }

void TurnVector(const float* m, const float* v, float* out) {
    for (int k = 0; k < 3; ++k)
        out[k] = m[k * 4] * v[0] + m[k * 4 + 1] * v[1] + m[k * 4 + 2] * v[2];
    Normalize(out);
}

float FromSegment(const float* p, const float* a, const float* b) {
    const float ab[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]}, ap[3] = {p[0] - a[0], p[1] - a[1], p[2] - a[2]};
    const float l2 = Dot(ab, ab);
    const float t = l2 > 0.0f ? std::clamp(Dot(ap, ab) / l2, 0.0f, 1.0f) : 0.0f;
    const float d[3] = {ap[0] - ab[0] * t, ap[1] - ab[1] * t, ap[2] - ab[2] * t};
    return std::sqrt(Dot(d, d));
}

// The nearest two segments come (m).
float SegmentSegment(const float* p1, const float* q1, const float* p2, const float* q2) {
    // Sampled finely enough for capsules a centimetre thick.
    float best = 1e9f;
    for (int i = 0; i <= 12; ++i) {
        const float t = i / 12.0f;
        const float p[3] = {p1[0] + (q1[0] - p1[0]) * t, p1[1] + (q1[1] - p1[1]) * t, p1[2] + (q1[2] - p1[2]) * t};
        best = std::min(best, FromSegment(p, p2, q2));
    }
    return best;
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
    float B[3] = {}, N[3] = {};              // its base's middle in the glove; the way it points, from its base to its nose
    float Distance(const float* p) const {
        float q[3];
        Apply(inverse, p, q);
        return g_profile.Distance(q[2], std::hypot(q[0] - g_profile.axis[0], q[1] - g_profile.axis[1]));
    }
    // How far along it from its base's face (m), and the turn round it (degrees, about N from the round's bone's own x).
    float FromBase(const float* p) const {
        float q[3];
        Apply(inverse, p, q);
        return q[2] - kBaseZ;
    }
    float Around(const float* p) const {
        float q[3];
        Apply(inverse, p, q);
        return std::atan2(q[1] - kLine[1], q[0] - kLine[0]) / kDeg;
    }
};

// Its bone's turn in the glove (rows): its length, base to nose, along -D, turned about it as near the game's pinch has it as goes (tools/roundfit.cpp's).
void RoundTurn(const float* D, float* R) {
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
        const float q[9] = {-1, 0, 0, 0, -1, 0, 0, 0, 1};
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

void PlaceRound(Round& round, const float* B, const float* N) {
    const float D[3] = {-N[0], -N[1], -N[2]};
    float R[9];
    RoundTurn(D, R);
    std::memcpy(round.B, B, sizeof(round.B));
    std::memcpy(round.N, N, sizeof(round.N));
    const float b[3] = {kLine[0], kLine[1], kBaseZ};
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            round.place[i * 4 + j] = R[i * 3 + j];
        round.place[i * 4 + 3] = B[i] - (R[i * 3] * b[0] + R[i * 3 + 1] * b[1] + R[i * 3 + 2] * b[2]);
    }
    InvertRigid(round.place, round.inverse);
}

// ---- The grip ----
struct Grip {
    handpose::JointPose pose;
    float elev = 0.0f, yaw = 0.0f;  // the round's way (base to nose): degrees from along the fingers out towards where the palm faces, and towards the little finger
    float zIndex = 0.010f;          // the index pad's touch, from the round's base along it (m)
    float gap = -0.0010f;           // how far the three pads go into it (m; negative in)
    float thumbShare = 0.8f;        // the thumb's last joint bends this much of its knuckle's as it closes
    Round round;
};

void Way(const Grip& g, float* N) {
    const float e = g.elev * kDeg, y = g.yaw * kDeg;
    for (int k = 0; k < 3; ++k)
        N[k] = kAlong[k] * std::cos(e) * std::cos(y) + kAcross[k] * std::cos(e) * std::sin(y) + kPalm[k] * std::sin(e);
    Normalize(N);
}

float GapOf(const Round& r, const std::vector<float>& at, const std::vector<int>& set, int* which = nullptr) {
    float gap = 1e9f;
    for (int i : set) {
        const float d = r.Distance(&at[i * 3]);
        if (d < gap) {
            gap = d;
            if (which)
                *which = i;
        }
    }
    return gap;
}

// The round put where the index and middle pads both touch it (gap in), the index pad's touch zIndex from its base. False: no such place near.
bool PlaceOnPads(Grip& g, const std::vector<float>& at, const float (*bones)[12]) {
    float N[3];
    Way(g, N);
    float E1[3], E2[3];
    {
        const float* seed = std::fabs(Dot(N, kAcross)) < 0.9f ? kAcross : kPalm;
        Cross(N, seed, E1);
        Normalize(E1);
        Cross(N, E1, E2);
        Normalize(E2);
    }
    // The pads' middles and the way they face, posed.
    float c[2][3] = {}, nrm[3] = {};
    for (int f = 1; f <= 2; ++f) {
        for (int i : g_pad[f])
            for (int k = 0; k < 3; ++k)
                c[f - 1][k] += at[i * 3 + k] / g_pad[f].size();
        float d[3];
        TurnVector(bones[g_boneOf[f][3]], g_padDir[f], d);
        for (int k = 0; k < 3; ++k)
            nrm[k] += d[k];
    }
    const float dn = Dot(nrm, N);
    for (int k = 0; k < 3; ++k)
        nrm[k] -= dn * N[k];
    Normalize(nrm);
    const float R = g_profile.body;
    float Q[3];
    for (int k = 0; k < 3; ++k)
        Q[k] = (c[0][k] + c[1][k]) / 2 + nrm[k] * (R + 0.004f);
    float B[3];
    const float s0 = Dot(c[0], N) - Dot(Q, N);
    for (int k = 0; k < 3; ++k)
        B[k] = Q[k] + N[k] * (s0 - g.zIndex);
    Round r;
    auto residual = [&](const float* b, float* F, int* contact) {
        PlaceRound(r, b, N);
        int ci = -1;
        F[0] = GapOf(r, at, g_pad[1], &ci) - g.gap;
        F[1] = GapOf(r, at, g_pad[2]) - g.gap;
        if (contact)
            *contact = ci;
        return ci;
    };
    for (int outer = 0; outer < 6; ++outer) {
        // Across the round: Newton on the two gaps.
        for (int it = 0; it < 25; ++it) {
            float F[2];
            residual(B, F, nullptr);
            if (std::fabs(F[0]) < 2e-5f && std::fabs(F[1]) < 2e-5f)
                break;
            const float h = 2e-4f;
            float J[2][2];
            for (int col = 0; col < 2; ++col) {
                const float* e = col == 0 ? E1 : E2;
                float b2[3] = {B[0] + e[0] * h, B[1] + e[1] * h, B[2] + e[2] * h}, F2[2];
                residual(b2, F2, nullptr);
                J[0][col] = (F2[0] - F[0]) / h, J[1][col] = (F2[1] - F[1]) / h;
            }
            const float det = J[0][0] * J[1][1] - J[0][1] * J[1][0];
            float du, dv;
            if (std::fabs(det) > 1e-4f) {
                du = -(J[1][1] * F[0] - J[0][1] * F[1]) / det;
                dv = -(-J[1][0] * F[0] + J[0][0] * F[1]) / det;
            } else {  // (the pads in a line with the round: towards both)
                du = -(J[0][0] * F[0] + J[1][0] * F[1]) * 0.5f;
                dv = -(J[0][1] * F[0] + J[1][1] * F[1]) * 0.5f;
            }
            const float len = std::hypot(du, dv);
            if (len > 0.004f)
                du *= 0.004f / len, dv *= 0.004f / len;
            for (int k = 0; k < 3; ++k)
                B[k] += E1[k] * du + E2[k] * dv;
        }
        // Along it: the index pad's touch at zIndex.
        float F[2];
        int ci = residual(B, F, nullptr);
        if (ci < 0)
            return false;
        const float z = r.FromBase(&at[ci * 3]);
        const float dz = z - g.zIndex;
        for (int k = 0; k < 3; ++k)
            B[k] += N[k] * dz;
        if (std::fabs(dz) < 1e-4f && std::fabs(F[0]) < 1e-4f && std::fabs(F[1]) < 1e-4f)
            break;
    }
    PlaceRound(g.round, B, N);
    const float gi = GapOf(g.round, at, g_pad[1]), gm = GapOf(g.round, at, g_pad[2]);
    return std::fabs(gi - g.gap) < 3e-4f && std::fabs(gm - g.gap) < 3e-4f;
}

// The thumb closed at its knuckle and last joint (the last tShare of the knuckle's) until its pad touches the round. False: it never does.
bool CloseThumb(Grip& g, std::vector<float>& at, float (*bones)[12]) {
    auto gapAt = [&](float knuckle) {
        g.pose.thumbKnuckle = knuckle, g.pose.thumbLast = knuckle * g.thumbShare;
        handpose::PoseJoints(g.pose, kWidth, bones);
        for (int i : g_pad[0])
            PosePoint(bones, i, &at[i * 3]);
        return GapOf(g.round, at, g_pad[0]);
    };
    float lo = -30.0f;
    float glo = gapAt(lo);
    if (glo < g.gap)
        return false;  // (already in, open as it goes)
    for (float k = lo + 3.0f; k <= 100.0f; k += 3.0f) {
        const float gk = gapAt(k);
        if (gk < g.gap) {
            float a = k - 3.0f, b = k;
            for (int i = 0; i < 14; ++i) {
                const float m = (a + b) / 2;
                (gapAt(m) < g.gap ? b : a) = m;
            }
            gapAt(b);
            PoseAll(bones, at);
            return true;
        }
    }
    gapAt(0.0f);
    PoseAll(bones, at);
    return false;
}

// ---- Measures ----
struct Check {
    bool placed = false, thumbClosed = false;
    float deepest = 0;           // the deepest any of the glove goes into the round (m)
    int deepestBone = -1;
    float deepestFaces = 0;      // ... against its own flat faces
    float pad[5] = {}, back[5] = {};  // each last part's pad's gap and its back's (m, negative in)
    float part[6] = {};          // each part's nearest (0 the palm)
    float seg[5][4] = {};        // each finger part's nearest
    float padZ[5] = {};          // where each pad touches along the round, from its base (m)
    float padAround[5] = {};     // ... and round it (degrees)
    float opposed = 0;           // the thumb's touch round the round from the index and middle's (degrees, 180 straight across)
    float palmGap = 0;
    float thumbToFingers = 0;    // the thumb's capsules to the index and middle's, beyond their thickness (m)
    float indexToMiddle = 0;     // the index's last two parts to the middle's, beyond their thickness
    float noseOut = 0;           // how far the nose stands out past the whole hand (m)
    float baseIn = 0;            // how far the base is in past the nearest of the hand behind it, along the round (m: positive, the glove's behind the base)
    float padAt[5][3] = {};      // each pad's touch, in the glove
    float indexLine = 0;         // the round's way against the index's last part's (degrees)
};

// How far p is from the round's own triangles, placed (m; negative in it) (tools/roundfit.cpp's).
float FacesDistance(const std::vector<float>& at, const float* p) {
    float best = 1e9f;
    double winding = 0.0;
    for (size_t t = 0; t + 2 < g_round.tris.size(); t += 3) {
        const float *a = &at[g_round.tris[t] * 3], *b = &at[g_round.tris[t + 1] * 3], *c = &at[g_round.tris[t + 2] * 3];
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

float AngleBetween(float a, float b) {
    float d = std::fmod(std::fabs(a - b), 360.0f);
    return d > 180.0f ? 360.0f - d : d;
}

Check Measure(const Grip& g, bool faces = false) {
    Check c;
    float bones[handpose::kBones][12];
    handpose::PoseJoints(g.pose, kWidth, bones);
    std::vector<float> at;
    PoseAll(bones, at);
    const Round& r = g.round;
    c.deepest = -1e9f;
    std::vector<float> roundAt;
    if (faces) {
        roundAt.resize(g_round.pos.size());
        for (size_t v = 0; v * 3 < g_round.pos.size(); ++v)
            Apply(r.place, &g_round.pos[v * 3], &roundAt[v * 3]);
        c.deepestFaces = -1e9f;
    }
    for (int i : g_all) {
        const float d = -r.Distance(&at[i * 3]);
        if (d > c.deepest)
            c.deepest = d, c.deepestBone = MainBone(g_glove[i]);
        if (faces && d > -0.003f)
            c.deepestFaces = std::max(c.deepestFaces, -FacesDistance(roundAt, &at[i * 3]));
    }
    for (int p = 0; p < 6; ++p)
        c.part[p] = GapOf(r, at, g_part[p]);
    c.palmGap = c.part[0];
    for (int f = 0; f < 5; ++f) {
        for (int d = 1; d <= 3; ++d)
            c.seg[f][d] = GapOf(r, at, g_seg[f][d]);
        int which = -1;
        c.pad[f] = GapOf(r, at, g_pad[f], &which);
        c.back[f] = GapOf(r, at, g_back[f]);
        if (which >= 0) {
            c.padZ[f] = r.FromBase(&at[which * 3]);
            c.padAround[f] = r.Around(&at[which * 3]);
            std::memcpy(c.padAt[f], &at[which * 3], sizeof(c.padAt[f]));
        }
    }
    {
        float a[3], b[3];
        JointAt(bones, 1, 2, a);
        JointAt(bones, 1, 3, b);
        float d[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
        Normalize(d);
        c.indexLine = std::acos(std::clamp(Dot(d, r.N), -1.0f, 1.0f)) / kDeg;
    }
    {
        // The index and middle touches' middle round the round, against the thumb's.
        const float a1 = c.padAround[1] * kDeg, a2 = c.padAround[2] * kDeg;
        const float mean = std::atan2(std::sin(a1) + std::sin(a2), std::cos(a1) + std::cos(a2)) / kDeg;
        c.opposed = AngleBetween(mean, c.padAround[0]);
    }
    // The fingers' capsules against each other.
    float j[5][4][3];
    for (int f = 0; f < 5; ++f)
        for (int k = 0; k < 4; ++k)
            JointAt(bones, f, k, j[f][k]);
    c.thumbToFingers = 1e9f;
    for (int d = 2; d <= 3; ++d)
        for (int f = 1; f <= 2; ++f)
            for (int e = 1; e <= 3; ++e)
                c.thumbToFingers = std::min(c.thumbToFingers, SegmentSegment(j[0][d - 1], j[0][d], j[f][e - 1], j[f][e]) - 0.009f - g_partRadius[f][e] * 0.8f);
    c.indexToMiddle = 1e9f;
    for (int d = 2; d <= 3; ++d)
        for (int e = 2; e <= 3; ++e)
            c.indexToMiddle = std::min(c.indexToMiddle, SegmentSegment(j[1][d - 1], j[1][d], j[2][e - 1], j[2][e]) - (g_partRadius[1][d] + g_partRadius[2][e]) * 0.8f);
    // The nose out past all of the hand, and the hand behind the base.
    float most = -1e9f, least = 1e9f;
    for (int i : g_all) {
        const float z = r.FromBase(&at[i * 3]);
        most = std::max(most, z);
        least = std::min(least, z);
    }
    c.noseOut = (kNoseZ - kBaseZ) - most;
    c.baseIn = -least;
    return c;
}

void PrintCheck(const Grip& g, const Check& c) {
    std::printf("  deepest %.2f mm (bone %d)%s; pads (gap, from the base, round it) thumb %.1f %.1f %.0f, index %.1f %.1f %.0f, middle %.1f %.1f %.0f; opposed %.0f\n",
                c.deepest * 1000, c.deepestBone, "", c.pad[0] * 1000, c.padZ[0] * 1000, c.padAround[0], c.pad[1] * 1000, c.padZ[1] * 1000, c.padAround[1],
                c.pad[2] * 1000, c.padZ[2] * 1000, c.padAround[2], c.opposed);
    std::printf("  backs of the last parts: thumb %.1f index %.1f middle %.1f; parts (palm thumb index middle ring little) %.1f %.1f %.1f %.1f %.1f %.1f mm\n",
                c.back[0] * 1000, c.back[1] * 1000, c.back[2] * 1000, c.part[0] * 1000, c.part[1] * 1000, c.part[2] * 1000, c.part[3] * 1000, c.part[4] * 1000,
                c.part[5] * 1000);
    std::printf("  by part from the palm out:");
    for (int f = 0; f < 5; ++f)
        std::printf(" %s %.1f %.1f %.1f;", kName[f], c.seg[f][1] * 1000, c.seg[f][2] * 1000, c.seg[f][3] * 1000);
    std::printf(" thumb to the fingers %.1f, index to middle %.1f; nose out past the hand %.1f cm, the hand %.1f cm behind the base; %.0f degrees off the index's line;"
                " deepest into its faces %.2f\n",
                c.thumbToFingers * 1000, c.indexToMiddle * 1000, c.noseOut * 100, c.baseIn * 100, c.indexLine, c.deepestFaces * 1000);
    const auto& p = g.pose;
    std::printf("  elev %.1f yaw %.1f z %.1f | thumb spread %.1f out %.1f roll %.1f base %.1f knuckle %.1f last %.1f | index %.1f %.1f %.1f (%.1f) middle %.1f %.1f %.1f (%.1f) ring %.1f %.1f %.1f (%.1f) little %.1f %.1f %.1f (%.1f)\n",
                g.elev, g.yaw, g.zIndex * 1000, p.thumbSpread, p.thumbOut, p.thumbRoll, p.thumbBase, p.thumbKnuckle, p.thumbLast, p.finger[0][0], p.finger[0][1],
                p.finger[0][2], p.fingerApart[0], p.finger[1][0], p.finger[1][1], p.finger[1][2], p.fingerApart[1], p.finger[2][0], p.finger[2][1], p.finger[2][2],
                p.fingerApart[2], p.finger[3][0], p.finger[3][1], p.finger[3][2], p.fingerApart[3]);
}

// Where things are in the hand (cm from its middle: along the fingers, across towards the little finger, out of the palm).
void PrintInHand(const Grip& g) {
    float A[3], C[3], P[3];
    HandWays(A, C, P);
    float bones[handpose::kBones][12];
    handpose::PoseJoints(g.pose, kWidth, bones);
    auto show = [&](const char* name, const float* p) {
        const float d[3] = {p[0] - kMiddle[0], p[1] - kMiddle[1], p[2] - kMiddle[2]};
        std::printf(" %s (%.1f %.1f %.1f)", name, Dot(d, A) * 100, Dot(d, C) * 100, Dot(d, P) * 100);
    };
    std::printf("  in the hand (cm: along, across, out of the palm):");
    for (int f = 0; f < 5; ++f) {
        float t[3];
        JointAt(bones, f, 3, t);
        char name[32];
        std::snprintf(name, sizeof(name), "%s tip", kName[f]);
        show(name, t);
    }
    show("base", g.round.B);
    const float* N = g.round.N;
    std::printf(" way (%.2f %.2f %.2f)\n", Dot(N, A), Dot(N, C), Dot(N, P));
}

// Places the round on the index and middle pads and closes the thumb on it.
bool Solve(Grip& g, Check* out = nullptr, bool faces = false) {
    float bones[handpose::kBones][12];
    handpose::PoseJoints(g.pose, kWidth, bones);
    std::vector<float> at;
    PoseAll(bones, at);
    const bool placed = PlaceOnPads(g, at, bones);
    bool closed = false;
    if (placed)
        closed = CloseThumb(g, at, bones);
    if (out) {
        *out = Measure(g, faces);
        out->placed = placed, out->thumbClosed = closed;
    }
    return placed && closed;
}

// ---- The search ----
// What a variant is after.
struct Aim {
    char letter;
    const char* what;       // what it is, a line
    float elev, yaw;        // the round's way (degrees), and how far the search may move each
    float elevFree, yawFree;
    float zIndex;           // the index pad's touch from the base (m)
    float zLo, zHi;         // where the middle's and the thumb's pads may touch (m from the base)
    float lineMost;         // the round within this of the index's last part's line (degrees; 0: no matter)
    float opLo, opHi;       // the thumb's touch round the round from the fingers' (degrees)
    const char* start;      // the hand it starts from (key=values)
    // (optional) the middle's own range along the round (m; both 0: zLo, zHi), how far round the round the index and middle touches lie apart (degrees; both 0:
    // no matter), the most the index and middle may turn apart at their knuckles (degrees; 0: 12).
    float midLo = 0, midHi = 0, spreadLo = 0, spreadHi = 0, apartMost = 0;
    float alike = 0.004f;   // how much the index and middle bent alike counts
    float thumbOutMost = 95.0f;  // the most the thumb may swing out in front of the palm (degrees)
};

float Sq(float v) { return v * v; }
float Over(float v, float most) { return std::max(0.0f, v - most); }
float Under(float v, float least) { return std::max(0.0f, least - v); }

// What the knobs the search turns are, and how far.
struct Knob {
    float& (*ref)(Grip&);
    float step, lo, hi;
};
const Knob kKnobs[] = {
    {[](Grip& g) -> float& { return g.pose.finger[0][0]; }, 4, -5, 85},
    {[](Grip& g) -> float& { return g.pose.finger[0][1]; }, 4, 0, 100},
    {[](Grip& g) -> float& { return g.pose.finger[0][2]; }, 4, 0, 75},
    {[](Grip& g) -> float& { return g.pose.fingerApart[0]; }, 2, -12, 12},
    {[](Grip& g) -> float& { return g.pose.finger[1][0]; }, 4, -5, 85},
    {[](Grip& g) -> float& { return g.pose.finger[1][1]; }, 4, 0, 100},
    {[](Grip& g) -> float& { return g.pose.finger[1][2]; }, 4, 0, 75},
    {[](Grip& g) -> float& { return g.pose.fingerApart[1]; }, 2, -12, 12},
    {[](Grip& g) -> float& { return g.pose.thumbSpread; }, 4, -30, 60},
    {[](Grip& g) -> float& { return g.pose.thumbOut; }, 4, 0, 95},
    {[](Grip& g) -> float& { return g.pose.thumbRoll; }, 4, -40, 90},
    {[](Grip& g) -> float& { return g.pose.thumbBase; }, 4, -25, 45},
    {[](Grip& g) -> float& { return g.thumbShare; }, 0.1f, 0.3f, 1.3f},
    {[](Grip& g) -> float& { return g.pose.finger[2][0]; }, 4, 15, 90},
    {[](Grip& g) -> float& { return g.pose.finger[2][1]; }, 4, 15, 100},
    {[](Grip& g) -> float& { return g.pose.finger[2][2]; }, 4, 5, 70},
    {[](Grip& g) -> float& { return g.pose.finger[3][0]; }, 4, 15, 90},
    {[](Grip& g) -> float& { return g.pose.finger[3][1]; }, 4, 15, 100},
    {[](Grip& g) -> float& { return g.pose.finger[3][2]; }, 4, 5, 70},
};
float& ElevOf(Grip& g) { return g.elev; }
float& YawOf(Grip& g) { return g.yaw; }

float Cost(Grip& g, const Aim& aim, const Grip& start, Check* out = nullptr) {
    Check c;
    const bool ok = Solve(g, &c);
    if (out)
        *out = c;
    if (!c.placed)
        return 1e7f;
    if (!ok)  // (the thumb never reaches: how far it stays off, to close in on it)
        return 1e5f + std::max(c.pad[0], 0.0f) * 1e5f;
    const auto& p = g.pose;
    float cost = 0.0f;
    // Into the round: a glove's give at most, anywhere.
    cost += 1000.0f * Sq(Over(c.deepest * 1000, 2.5f));
    // Each last part touches with its pad, not its back or side.
    for (int f = 0; f < 3; ++f)
        cost += 20.0f * Sq(Under((c.back[f] - c.pad[f]) * 1000, 0.3f));
    // A fingertip pinch: the three fingers' other parts off it.
    for (int f = 0; f < 3; ++f)
        for (int d = 1; d <= 2; ++d)
            cost += 3.0f * Sq(Under(c.seg[f][d] * 1000, 1.5f));
    // The palm clear, the ring and little fingers clear.
    cost += 0.5f * Sq(Under(c.palmGap * 1000, 15.0f));
    cost += 5.0f * Sq(Under(c.part[4] * 1000, 5.0f)) + 5.0f * Sq(Under(c.part[5] * 1000, 5.0f));
    // The middle's and the thumb's touches where wanted along the round, the thumb across from the fingers.
    for (int f : {0, 2}) {
        const float z = c.padZ[f] * 1000;
        const bool own = f == 2 && (aim.midLo != 0 || aim.midHi != 0);
        const float lo = own ? aim.midLo : aim.zLo, hi = own ? aim.midHi : aim.zHi;
        cost += 0.5f * Sq(Under(z, lo * 1000)) + 0.5f * Sq(Over(z, hi * 1000));
    }
    cost += 0.05f * Sq(Under(c.opposed, aim.opLo)) + 0.05f * Sq(Over(c.opposed, aim.opHi));
    if (aim.spreadLo != 0 || aim.spreadHi != 0) {
        const float spread = AngleBetween(c.padAround[1], c.padAround[2]);
        cost += 0.05f * Sq(Under(spread, aim.spreadLo)) + 0.05f * Sq(Over(spread, aim.spreadHi));
    }
    // The fingers off each other.
    cost += 5.0f * Sq(Under(c.thumbToFingers * 1000, 0.0f)) + 5.0f * Sq(Under(c.indexToMiddle * 1000, -1.0f));
    if (aim.lineMost > 0)
        cost += 0.05f * Sq(Over(c.indexLine, aim.lineMost));
    // Natural joints: the last joint following the middle one, the index and middle alike, the thumb's joints not bent back.
    for (int k = 0; k < 2; ++k)
        cost += 0.004f * Sq(p.finger[k][2] - 0.65f * p.finger[k][1]);
    for (int j = 0; j < 3; ++j)
        cost += aim.alike * Sq(p.finger[0][j] - p.finger[1][j]);
    cost += 0.01f * (Sq(p.fingerApart[0]) + Sq(p.fingerApart[1] - 1.0f));
    cost += 0.05f * Sq(Under(p.thumbKnuckle, 0.0f)) + 0.05f * Sq(Under(p.thumbLast, 0.0f)) + 0.02f * Sq(Over(p.thumbKnuckle, 70.0f)) +
            0.02f * Sq(Over(p.thumbLast, 70.0f));
    for (int k = 2; k < 4; ++k)
        cost += 0.004f * Sq(p.finger[k][2] - 0.6f * p.finger[k][1]);
    // Near the variant's own start, so each keeps its look.
    for (const Knob& k : kKnobs) {
        const float d = (k.ref(g) - k.ref(const_cast<Grip&>(start))) / (k.step * 2.5f);
        cost += 0.03f * d * d;
    }
    cost += 0.02f * (Sq(g.elev - aim.elev) + Sq(g.yaw - aim.yaw)) / 25.0f;
    return cost;
}

// A pattern search: each knob tried a step either way, kept when it's better, the steps halved when nothing is.
Grip Search(const Aim& aim, Grip g, const Grip& start, float& best) {
    std::vector<Knob> knobs(std::begin(kKnobs), std::end(kKnobs));
    if (aim.apartMost > 0)
        for (int k : {3, 7})
            knobs[k].lo = -aim.apartMost, knobs[k].hi = aim.apartMost;
    knobs[9].hi = aim.thumbOutMost;
    g.pose.thumbOut = std::min(g.pose.thumbOut, aim.thumbOutMost);
    if (aim.elevFree > 0)
        knobs.push_back({ElevOf, 3, aim.elev - aim.elevFree, aim.elev + aim.elevFree});
    if (aim.yawFree > 0)
        knobs.push_back({YawOf, 3, aim.yaw - aim.yawFree, aim.yaw + aim.yawFree});
    best = Cost(g, aim, start);
    for (float scale : {1.0f, 0.5f, 0.25f, 0.125f}) {
        for (int it = 0; it < 40; ++it) {
            bool improved = false;
            for (const Knob& k : knobs)
                for (int dir : {1, -1}) {
                    Grip t = g;
                    float& v = k.ref(t);
                    const float was = v;
                    v = std::clamp(v + dir * k.step * scale, k.lo, k.hi);
                    if (v == was)
                        continue;
                    const float c = Cost(t, aim, start);
                    if (c < best - 1e-4f) {
                        g = t, best = c, improved = true;
                        break;
                    }
                }
            if (!improved)
                break;
        }
    }
    return g;
}

void ReadKeyString(Grip& g, const char* text) {
    std::string s = text;
    size_t at = 0;
    while (at < s.size()) {
        size_t end = s.find(' ', at);
        if (end == std::string::npos)
            end = s.size();
        const std::string word = s.substr(at, end - at);
        const size_t eq = word.find('=');
        if (eq != std::string::npos)
            SetKey(g, word.substr(0, eq), static_cast<float>(std::atof(word.c_str() + eq + 1)));
        at = end + 1;
    }
}

// From the variant's start and a few starts about it (a thread each), the best.
Grip Fit(const Aim& aim, const Grip& start, int starts) {
    std::vector<Grip> found(starts);
    std::vector<float> scores(starts, 1e30f);
    std::atomic<int> next{0};
    std::vector<std::thread> workers;
    const unsigned threads = std::max(1u, std::min(static_cast<unsigned>(starts), std::thread::hardware_concurrency()));
    for (unsigned w = 0; w < threads; ++w)
        workers.emplace_back([&] {
            for (int s; (s = next++) < starts;) {
                Grip g = start;
                if (s > 0) {
                    std::mt19937 rng(1234u + s * 7919u);
                    std::uniform_real_distribution<float> jitter(-1.0f, 1.0f);
                    for (const Knob& k : kKnobs) {
                        float& v = k.ref(g);
                        v = std::clamp(v + jitter(rng) * k.step * 2.5f, k.lo, k.hi);
                    }
                }
                found[s] = Search(aim, g, start, scores[s]);
            }
        });
    for (auto& w : workers)
        w.join();
    int best = 0;
    for (int s = 1; s < starts; ++s)
        if (scores[s] < scores[best])
            best = s;
    std::printf("variant %c: the best of %d starts is %d, cost %.2f (the others:", aim.letter, starts, best, scores[best]);
    for (int s = 0; s < starts; ++s)
        std::printf(" %.1f", scores[s]);
    std::printf(")\n");
    return found[best];
}

// ---- Pictures ----
struct Canvas {
    int w, h;
    std::vector<float> rgb, depth;
    Canvas(int w, int h) : w(w), h(h), rgb(static_cast<size_t>(w) * h * 3, 0.13f), depth(static_cast<size_t>(w) * h, 1e9f) {}
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

// The ways round the hand: along the fingers, across them (thumb to little finger), out of the palm.
void HandWays(float* A, float* C, float* P) {
    std::memcpy(A, kAlong, 3 * sizeof(float));
    std::memcpy(P, kPalm, 3 * sizeof(float));
    Normalize(A);
    const float d = Dot(P, A);
    for (int k = 0; k < 3; ++k)
        P[k] -= d * A[k];
    Normalize(P);
    Cross(A, P, C);  // (along x out of the palm is across, thumb to little finger)
    Normalize(C);
}

View ViewFrom(const float* A, const float* C, const float* P, float a, float c, float p, float ua, float uc, float up) {
    const float f[3] = {A[0] * a + C[0] * c + P[0] * p, A[1] * a + C[1] * c + P[1] * p, A[2] * a + C[2] * c + P[2] * p};
    const float u[3] = {A[0] * ua + C[0] * uc + P[0] * up, A[1] * ua + C[1] * uc + P[1] * up, A[2] * ua + C[2] * uc + P[2] * up};
    return MakeView(f[0], f[1], f[2], u[0], u[1], u[2]);
}

constexpr float kFingerColour[6][3] = {{0.6f, 0.5f, 0.4f}, {0.9f, 0.25f, 0.2f}, {0.95f, 0.55f, 0.15f}, {0.9f, 0.85f, 0.2f}, {0.3f, 0.8f, 0.3f}, {0.3f, 0.5f, 0.95f}};
constexpr float kGlove[3] = {0.52f, 0.45f, 0.34f};
constexpr float kBody[3] = {0.55f, 0.56f, 0.6f}, kNose[3] = {0.42f, 0.48f, 0.22f}, kBrass[3] = {0.85f, 0.65f, 0.3f};
// The brass case's band at the base (its rim and the first of the body), drawn brass so the base end shows.
constexpr float kBrassBand = 0.006f;

void DrawGrip(Canvas& img, int tx, int ty, int tile, const View& view, const Grip& g, const float* centre, float scale, bool plain) {
    float bones[handpose::kBones][12];
    handpose::PoseJoints(g.pose, kWidth, bones);
    std::vector<float> at;
    PoseAll(bones, at);
    std::vector<float> roundAt(g_round.pos.size());
    for (size_t v = 0; v * 3 < g_round.pos.size(); ++v)
        Apply(g.round.place, &g_round.pos[v * 3], &roundAt[v * 3]);
    for (size_t t = 0; t + 2 < g_round.tris.size(); t += 3) {
        const int a = g_round.tris[t], b = g_round.tris[t + 1], c = g_round.tris[t + 2];
        const float zMax = std::max({g_round.pos[a * 3 + 2], g_round.pos[b * 3 + 2], g_round.pos[c * 3 + 2]});
        const float* colour = zMax < g_profile.base + kBrassBand ? kBrass : zMax > g_profile.bodyEnd + 0.0005f ? kNose : kBody;
        img.Triangle(tx, ty, tile, view, centre, scale, &roundAt[a * 3], &roundAt[b * 3], &roundAt[c * 3], colour);
    }
    for (size_t t = 0; t + 2 < g_gloveTris.size(); t += 3)
        img.Triangle(tx, ty, tile, view, centre, scale, &at[g_gloveTris[t] * 3], &at[g_gloveTris[t + 1] * 3], &at[g_gloveTris[t + 2] * 3],
                     plain ? kGlove : kFingerColour[g_glove[g_gloveTris[t]].finger + 1]);
}

// Each view's middle and size: the glove's and the round's points, as that view sees them (the picture's middle in the glove, and how many metres across it all is).
void Frame(const Grip& g, const View& view, float* centre, float& across) {
    float bones[handpose::kBones][12];
    handpose::PoseJoints(g.pose, kWidth, bones);
    std::vector<float> at;
    PoseAll(bones, at);
    float lo[2] = {1e9f, 1e9f}, hi[2] = {-1e9f, -1e9f};
    auto take = [&](const float* p) {
        const float x = Dot(p, view.right), y = Dot(p, view.up);
        lo[0] = std::min(lo[0], x), hi[0] = std::max(hi[0], x), lo[1] = std::min(lo[1], y), hi[1] = std::max(hi[1], y);
    };
    for (int i : g_all)
        take(&at[i * 3]);
    for (int v : g_roundUsed) {
        float q[3];
        Apply(g.round.place, &g_round.pos[v * 3], q);
        take(q);
    }
    const float cx = (lo[0] + hi[0]) / 2, cy = (lo[1] + hi[1]) / 2;
    for (int k = 0; k < 3; ++k)
        centre[k] = view.right[k] * cx + view.up[k] * cy;
    across = std::max(hi[0] - lo[0], hi[1] - lo[1]);
}

// The round's own ways from its touches: N its way (base to nose), U from the thumb's touch towards the index and middle's (square to N), S = N x U.
void RoundWays(const Grip& g, float* N, float* U, float* S) {
    Check c = Measure(g);
    std::memcpy(N, g.round.N, 3 * sizeof(float));
    for (int k = 0; k < 3; ++k)
        U[k] = (c.padAt[1][k] + c.padAt[2][k]) / 2 - c.padAt[0][k];
    const float d = Dot(U, N);
    for (int k = 0; k < 3; ++k)
        U[k] -= d * N[k];
    Normalize(U);
    Cross(N, U, S);
    Normalize(S);
}

// The views. 0: as the wearer sees the left hand held out at chest height, the round pointing away (behind it, above the index and middle, from the body's middle:
// the eyes look forward 0.71, left 0.36 and down 0.62 at it). 1: as photos 2 and 4 (the thumb's side, the round across the picture, the fingers on it above,
// the thumb under). 2: as photos 1 and 3 (from behind and above the hand, its base towards the eye). Then down the round from its nose, the palm's side, the
// little finger's side, from in front of the fingertips, above the back of the hand.
struct Views {
    View v[8];
    const char* name[8];
};

Views MakeViews(const Grip& g) {
    float A[3], C[3], P[3];
    HandWays(A, C, P);
    float N[3], U[3], S[3];
    RoundWays(g, N, U, S);
    // S towards the thumb's side of the hand (the wearer's right for the left hand, the round pointing away and the fingers on it above).
    float side = Dot(S, C) < 0.0f ? 1.0f : -1.0f;
    float Sr[3] = {S[0] * side, S[1] * side, S[2] * side};
    Views out;
    float f[3];
    for (int k = 0; k < 3; ++k)
        f[k] = N[k] * 0.71f - Sr[k] * 0.36f - U[k] * 0.62f;
    out.v[0] = MakeView(f[0], f[1], f[2], U[0], U[1], U[2]), out.name[0] = "the wearer's view";
    for (int k = 0; k < 3; ++k)
        f[k] = -Sr[k] * 0.92f - U[k] * 0.3f + N[k] * 0.1f;
    out.v[1] = MakeView(f[0], f[1], f[2], U[0], U[1], U[2]), out.name[1] = "photos 2 and 4";
    for (int k = 0; k < 3; ++k)
        f[k] = N[k] * 0.62f - U[k] * 0.62f - Sr[k] * 0.25f;
    out.v[2] = MakeView(f[0], f[1], f[2], U[0], U[1], U[2]), out.name[2] = "photos 1 and 3";
    out.v[3] = MakeView(-N[0], -N[1], -N[2], U[0], U[1], U[2]), out.name[3] = "down the round from its nose";
    out.v[4] = ViewFrom(A, C, P, 0, 0, -1, 1, 0, 0), out.name[4] = "the palm's side";
    out.v[5] = ViewFrom(A, C, P, 0.1f, -0.85f, 0.35f, 0, 0, -1), out.name[5] = "the little finger's side";
    out.v[6] = ViewFrom(A, C, P, -1, 0, 0.15f, 0, 0, -1), out.name[6] = "from in front of the fingertips";
    out.v[7] = ViewFrom(A, C, P, 0, 0, 1, 1, 0, 0), out.name[7] = "above the back of the hand";
    return out;
}

void DrawSheet(const Grip& g, const char* path, bool plain) {
    constexpr int kSide = 600, kColumns = 4, kRows = 2;
    Canvas img(kSide * kColumns, kSide * kRows);
    const Views views = MakeViews(g);
    float centres[8][3], most = 0.0f;
    for (int v = 0; v < 8; ++v) {
        float across;
        Frame(g, views.v[v], centres[v], across);
        most = std::max(most, across);
    }
    const float scale = kSide * 0.94f / most;
    for (int v = 0; v < kColumns * kRows; ++v)
        DrawGrip(img, (v % kColumns) * kSide, (v / kColumns) * kSide, kSide, views.v[v], g, centres[v], scale, plain);
    img.Save(path);
    std::printf("saved %s\n", path);
}

// One view big (the combined picture's tiles), metres across the picture.
void DrawOne(const Grip& g, const View& view, const char* path, int side, float metres, bool plain) {
    Canvas img(side, side);
    float centre[3], across;
    Frame(g, view, centre, across);
    DrawGrip(img, 0, 0, side, view, g, centre, side / std::max(metres, across * 1.06f), plain);
    img.Save(path);
    std::printf("saved %s\n", path);
}

// ---- Keys ----
bool SetKey(Grip& g, const std::string& key, float v) {
    auto& p = g.pose;
    static const std::map<std::string, int> fingers = {{"i", 0}, {"m", 1}, {"r", 2}, {"l", 3}};
    if (key == "elev")
        g.elev = v;
    else if (key == "yaw")
        g.yaw = v;
    else if (key == "z")
        g.zIndex = v * 0.001f;
    else if (key == "gap")
        g.gap = v * 0.001f;
    else if (key == "tS")
        p.thumbSpread = v;
    else if (key == "tO")
        p.thumbOut = v;
    else if (key == "tR")
        p.thumbRoll = v;
    else if (key == "tB")
        p.thumbBase = v;
    else if (key == "tShare")
        g.thumbShare = v;
    else if (key.size() == 2 && fingers.count(key.substr(0, 1))) {
        const int f = fingers.at(key.substr(0, 1));
        switch (key[1]) {
        case 'K': p.finger[f][0] = v; break;
        case 'M': p.finger[f][1] = v; break;
        case 'L': p.finger[f][2] = v; break;
        case 'A': p.fingerApart[f] = v; break;
        default: return false;
        }
    } else
        return false;
    return true;
}

void ReadKeys(Grip& g, int argc, char** argv, int from) {
    for (int i = from; i < argc; ++i) {
        if (!std::strcmp(argv[i], "plain")) {
            g_plain = true;
            continue;
        }
        const char* eq = std::strchr(argv[i], '=');
        if (!eq)
            continue;
        const std::string key(argv[i], static_cast<size_t>(eq - argv[i]));
        if (key == "starts" || key.rfind("aim.", 0) == 0 || key == "out")
            continue;
        if (!SetKey(g, key, static_cast<float>(std::atof(eq + 1))))
            std::fprintf(stderr, "unknown key %s\n", key.c_str());
    }
}

// ---- The variants ----
const Aim kAims[] = {
    {'A', "as photos 2 and 4: the round straight out along the index finger's own line, the index and middle pads on its base end, the thumb's pad across from them",
     58, 0, 12, 10, 0.010f, 0.0f, 0.020f, 12, 150, 180,
     "iK=55 iM=10 iL=5 mK=55 mM=10 mL=5 tS=20 tO=60 tR=40 tB=0 rK=55 rM=65 rL=35 lK=60 lM=65 lL=35"},
    {'B', "A tilted about 25 degrees further out of the palm's plane: the base end held at the fingertips with its face towards the palm (the palm clear of it), the "
          "round pointing almost straight out of the palm",
     88, 0, 6, 10, 0.010f, 0.0f, 0.020f, 0, 150, 180,
     "iK=50 iM=30 iL=15 mK=50 mM=30 mL=15 tS=10 tO=80 tR=50 tB=10 rK=55 rM=65 rL=35 lK=60 lM=65 lL=35",
     0, 0, 0, 0, 0, 0.004f, 80.0f},
    {'C', "as photo 4, staggered: the index finger laid along the body (its pad 3 cm up from the base), the middle fingertip curled onto the base's rim, the thumb's "
          "pad under, between them",
     58, 0, 12, 10, 0.030f, 0.008f, 0.028f, 0, 150, 180,
     "iK=50 iM=5 iL=5 mK=62 mM=45 mL=30 tS=20 tO=60 tR=40 tB=0 rK=55 rM=65 rL=35 lK=60 lM=65 lL=35",
     0.0f, 0.007f, 0, 0, 0, 0.0f},
    {'D', "as photos 1 and 3: the three pads 2 cm up the body from the base, the index and middle curled over it more, the round pointing forward and a little down",
     40, 0, 10, 10, 0.020f, 0.015f, 0.030f, 0, 150, 180,
     "iK=35 iM=55 iL=35 mK=35 mM=55 mL=35 tS=20 tO=70 tR=50 tB=0 rK=55 rM=65 rL=35 lK=60 lM=65 lL=35"},
};

// ---- The tables ----
// Text as comment lines no longer than width.
void WriteWrapped(FILE* f, const std::string& text, size_t width = 160) {
    std::string line = "//";
    size_t at = 0;
    while (at < text.size()) {
        size_t end = text.find(' ', at);
        if (end == std::string::npos)
            end = text.size();
        const std::string word = text.substr(at, end - at);
        if (line.size() + 1 + word.size() > width) {
            std::fprintf(f, "%s\n", line.c_str());
            line = "//";
        }
        line += " " + word;
        at = end + 1;
    }
    if (line.size() > 2)
        std::fprintf(f, "%s\n", line.c_str());
}

void WriteHeader(const Aim& aim, const Grip& g, const Check& c, const char* path, const std::string& command) {
    float bones[handpose::kBones][12];
    handpose::PoseJoints(g.pose, kWidth, bones);
    float open[handpose::kBones][12];
    handpose::Pose(handpose::Fingers{}, kWidth, open);
    float palmMoved = 0.0f;
    for (int k = 0; k < 12; ++k)
        palmMoved = std::max(palmMoved, std::fabs(bones[1][k] - open[1][k]));
    FILE* f = nullptr;
    if (fopen_s(&f, path, "w") || !f)
        return;
    const auto& p = g.pose;
    std::fprintf(f, "#pragma once\n\n");
    char letter[2] = {aim.letter, 0};
    WriteWrapped(f, std::string("The left hand holding a 40 mm round by its base end in a fingertip pinch, variant ") + letter +
                        " of four (tools\\basegrip.cpp): the user, 2026-11-05, after the fist round its body: \"you don't need a whole fist ... at most three fingers "
                        "... at the end of the shell ... with his finger, middle finger, and thumb on that circle base. I'm okay with the ring finger and pinky "
                        "folding\", with four photographs of a water bottle held so. Variant " +
                        letter + ", " + aim.what + ".");
    std::fprintf(f, "// Fitted on the meshes offline: the index and middle fingers shaped joint by joint, the round put where both their pads touch it, the thumb opposed and closed\n");
    std::fprintf(f, "// at its knuckle and last joint until its pad touches it from the other side; the most natural such hand (the search's) with nothing else on the round.\n");
    std::fprintf(f, "// Into the round: the deepest the glove goes anywhere (all 23 bones' vertices) %.2f mm (bone %d) into its outline, %.2f into its own flat faces (2.5 the most).\n",
                 c.deepest * 1000, c.deepestBone, c.deepestFaces * 1000);
    std::fprintf(f, "// The pads touch it (gap, negative in): thumb %.1f mm, index %.1f, middle %.1f; the rest of their last parts %.1f %.1f %.1f. Along the round from its\n",
                 c.pad[0] * 1000, c.pad[1] * 1000, c.pad[2] * 1000, c.back[0] * 1000, c.back[1] * 1000, c.back[2] * 1000);
    std::fprintf(f, "// base's face the pads touch at: thumb");
    std::fprintf(f, " %.1f mm, index %.1f, middle %.1f; the thumb's touch %.0f degrees round the round from the index and middle's (180: straight across).\n",
                 c.padZ[0] * 1000, c.padZ[1] * 1000, c.padZ[2] * 1000, c.opposed);
    std::fprintf(f, "// Clear of it (mm): the palm %.1f, the ring finger %.1f, the little finger %.1f; the thumb's other parts %.1f %.1f, the index's %.1f %.1f, the middle's %.1f %.1f.\n",
                 c.palmGap * 1000, c.part[4] * 1000, c.part[5] * 1000, c.seg[0][1] * 1000, c.seg[0][2] * 1000, c.seg[1][1] * 1000, c.seg[1][2] * 1000, c.seg[2][1] * 1000,
                 c.seg[2][2] * 1000);
    std::fprintf(f, "// The nose stands %.1f cm out past all of the hand. The round's way: %.1f degrees out of the palm's plane from along the fingers, %.1f towards the little\n",
                 c.noseOut * 100, g.elev, g.yaw);
    std::fprintf(f, "// finger (%.0f degrees off the index's last part's line). Its turn about its own length is the game's pinch's as near as goes (frame 7350).\n", c.indexLine);
    std::fprintf(f, "// Thumb: spread %.1f, out %.1f, roll %.1f, base %.1f, knuckle %.1f, last %.1f. Fingers (knuckle, middle, last; turn towards the thumb): index %.1f %.1f %.1f %.1f,\n",
                 p.thumbSpread, p.thumbOut, p.thumbRoll, p.thumbBase, p.thumbKnuckle, p.thumbLast, p.finger[0][0], p.finger[0][1], p.finger[0][2], p.fingerApart[0]);
    std::fprintf(f, "// middle %.1f %.1f %.1f %.1f, ring %.1f %.1f %.1f, little %.1f %.1f %.1f.\n", p.finger[1][0], p.finger[1][1], p.finger[1][2], p.fingerApart[1],
                 p.finger[2][0], p.finger[2][1], p.finger[2][2], p.finger[3][0], p.finger[3][1], p.finger[3][2]);
    std::fprintf(f, "// Bone 1, the palm, is handpose::Pose's at width 1.15 (PoseJoints doesn't move it: %.0e off), so hands.cpp's re-anchoring on the palm leaves the round where\n",
                 palmMoved);
    std::fprintf(f, "// it was fitted. In the form of src\\m320carry.h (hands.cpp's m320::SetHold(kCarry, ...)): put in its place to try it.\n");
    std::fprintf(f, "// Made again by, from the project's folder in an x86 developer prompt:\n");
    std::fprintf(f, "//   cl /nologo /EHsc /O2 /std:c++17 /DNOMINMAX /DWIN32_LEAN_AND_MEAN /I src /I tools /Febuild\\basegrip.exe /Fobuild\\ tools\\basegrip.cpp src\\handpose.cpp\n");
    std::fprintf(f, "//   %s\n\n", command.c_str());
    std::fprintf(f, "namespace m320carry {\n\n");
    std::fprintf(f, "// The round's bone (draw 1440's bone 0, the space kRoundBaseOnBone and kRoundNoseOnBone are in) into the left glove's bind space (3x4 rows: rotation | translation,\n");
    std::fprintf(f, "// rigid).\n");
    std::fprintf(f, "constexpr float kRoundInGlove[12] = {");
    for (int k = 0; k < 12; ++k)
        std::fprintf(f, "%.6ff%s", g.round.place[k], k == 11 ? "};\n\n" : ", ");
    std::fprintf(f, "// Each left glove bone (0-22) as handpose::PoseJoints gives it at width 1.15 (bind space -> glove space): the fitted pinch.\n");
    std::fprintf(f, "constexpr float kFingers[23][12] = {\n");
    for (int b = 0; b < handpose::kBones; ++b) {
        std::fprintf(f, "    {");
        for (int k = 0; k < 12; ++k)
            std::fprintf(f, "%.6ff%s", bones[b][k], k == 11 ? "},\n" : ", ");
    }
    std::fprintf(f, "};\n\n}  // namespace m320carry\n");
    fclose(f);
    std::printf("wrote %s (the palm %.0e off Pose's)\n", path, palmMoved);
}

// ---- Setting up ----
bool Setup(const std::string& dir) {
    if (!LoadGlove(dir, g_glove, g_gloveTris)) {
        std::fprintf(stderr, "the gloves' mesh dump isn't in %s\n", dir.c_str());
        return false;
    }
    if (!MeasureRound(dir)) {
        std::fprintf(stderr, "the round's mesh dump (mesh_1440_7848_*) isn't in %s\n", dir.c_str());
        return false;
    }
    handpose::Joints(g_joints);
    for (int b = 0; b < handpose::kBones; ++b)
        if (meshdump::kFinger[b] >= 0 && kDepth[b] >= 1)
            g_boneOf[meshdump::kFinger[b]][kDepth[b]] = b;
    // Each pad's way in the bind space: the fingers' out of the palm, the thumb's towards the palm and the fingers (handpose.cpp's), square to the last part.
    for (int f = 0; f < 5; ++f) {
        float u[3] = {g_joints[f][3][0] - g_joints[f][2][0], g_joints[f][3][1] - g_joints[f][2][1], g_joints[f][3][2] - g_joints[f][2][2]};
        Normalize(u);
        float d[3];
        for (int k = 0; k < 3; ++k)
            d[k] = f == 0 ? kPalm[k] + kAcross[k] : kPalm[k];
        const float along = Dot(d, u);
        for (int k = 0; k < 3; ++k)
            d[k] -= along * u[k];
        Normalize(d);
        std::memcpy(g_padDir[f], d, sizeof(d));
    }
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
        const int f = g_glove[i].finger;
        if (f >= 0 && kDepth[b] >= 1) {
            const float* j0 = g_joints[f][kDepth[b] - 1];
            const float* j1 = g_joints[f][kDepth[b]];
            const float w[3] = {g_glove[i].pos[0] - j0[0], g_glove[i].pos[1] - j0[1], g_glove[i].pos[2] - j0[2]};
            const float e[3] = {j1[0] - j0[0], j1[1] - j0[1], j1[2] - j0[2]};
            const float length = std::sqrt(Dot(e, e));
            const float margin = kDepth[b] >= 2 ? length * 0.5f : f == 0 ? 0.025f : 0.018f;
            if (std::sqrt(Dot(w, w)) > margin)
                g_seg[f][kDepth[b]].push_back(vi);
            if (kDepth[b] == 3) {
                // The pad: the palm's side of the last part, from a quarter of the way out to past its tip.
                const float t = Dot(w, e) / (length * length);
                float off[3];
                for (int k = 0; k < 3; ++k)
                    off[k] = w[k] - e[k] * t;
                const float o = std::sqrt(Dot(off, off));
                const float facing = o > 1e-6f ? Dot(off, g_padDir[f]) / o : 0.0f;
                if (t > 0.2f && (facing > 0.35f || (t > 1.0f && facing > -0.1f)))
                    g_pad[f].push_back(vi);
                else
                    g_back[f].push_back(vi);
            }
        }
    }
    // Each finger part's thickness about its middle line, the hand open (three quarters of its vertices nearer).
    {
        handpose::JointPose open;
        float bones[handpose::kBones][12];
        handpose::PoseJoints(open, kWidth, bones);
        for (int f = 0; f < 5; ++f)
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
    }
    std::printf("the glove: %zu vertices; pads (vertices): thumb %zu index %zu middle %zu ring %zu little %zu; the round's body radius %.1f mm, base at %.4f\n", g_all.size(),
                g_pad[0].size(), g_pad[1].size(), g_pad[2].size(), g_pad[3].size(), g_pad[4].size(), g_profile.body * 1000, g_profile.base);
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 4) {
        std::fprintf(stderr, "usage: basegrip <build folder> <out folder> look <key=value ...> | fit <A|B|C|D> [key=value ...]\n");
        return 1;
    }
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    const std::string dir = argv[1], outDir = argv[2];
    for (int i = 3; i < argc; ++i)
        if (!std::strcmp(argv[i], "shell12g")) {
            g_shell12g = true;
            kLine[0] = -0.0013f, kLine[1] = -0.1026f, kBaseZ = 0.049f, kNoseZ = 0.113f;
        }
    if (!Setup(dir))
        return 1;
    if (!std::strcmp(argv[3], "look")) {
        Grip g;
        // A start: the fingers a little bent, the ring and little folded, the thumb out in front of the palm.
        g.pose.finger[0][0] = 30, g.pose.finger[0][1] = 30, g.pose.finger[0][2] = 15;
        g.pose.finger[1][0] = 30, g.pose.finger[1][1] = 30, g.pose.finger[1][2] = 15;
        g.pose.finger[2][0] = 55, g.pose.finger[2][1] = 70, g.pose.finger[2][2] = 40;
        g.pose.finger[3][0] = 60, g.pose.finger[3][1] = 70, g.pose.finger[3][2] = 40;
        g.pose.thumbOut = 40, g.pose.thumbRoll = 30;
        ReadKeys(g, argc, argv, 4);
        Check c;
        const bool ok = Solve(g, &c, true);
        std::printf("%s (placed %d, thumb closed %d)\n", ok ? "fits" : "doesn't", c.placed, c.thumbClosed);
        PrintCheck(g, c);
        PrintInHand(g);
        DrawSheet(g, (outDir + "\\look.bmp").c_str(), g_plain);
        return 0;
    }
    if (!std::strcmp(argv[3], "fit") && argc > 4) {
        const Aim* found = nullptr;
        for (const Aim& a : kAims)
            if (a.letter == argv[4][0])
                found = &a;
        if (!found) {
            std::fprintf(stderr, "no variant %s\n", argv[4]);
            return 1;
        }
        // The aim's own numbers changed for a try: aim.elev= aim.yaw= aim.elevFree= aim.yawFree= aim.z= aim.zLo= aim.zHi= (mm) aim.line= aim.opLo= aim.opHi=.
        Aim tried = *found;
        for (int i = 5; i < argc; ++i) {
            const char* eq = std::strchr(argv[i], '=');
            if (std::strncmp(argv[i], "aim.", 4) || !eq)
                continue;
            const std::string key(argv[i] + 4, static_cast<size_t>(eq - argv[i] - 4));
            const float v = static_cast<float>(std::atof(eq + 1));
            if (key == "elev") tried.elev = v;
            else if (key == "yaw") tried.yaw = v;
            else if (key == "elevFree") tried.elevFree = v;
            else if (key == "yawFree") tried.yawFree = v;
            else if (key == "z") tried.zIndex = v * 0.001f;
            else if (key == "zLo") tried.zLo = v * 0.001f;
            else if (key == "zHi") tried.zHi = v * 0.001f;
            else if (key == "line") tried.lineMost = v;
            else if (key == "opLo") tried.opLo = v;
            else if (key == "opHi") tried.opHi = v;
            else if (key == "midLo") tried.midLo = v * 0.001f;
            else if (key == "midHi") tried.midHi = v * 0.001f;
            else if (key == "spreadLo") tried.spreadLo = v;
            else if (key == "spreadHi") tried.spreadHi = v;
            else if (key == "apart") tried.apartMost = v;
            else if (key == "alike") tried.alike = v;
            else if (key == "thumbOut") tried.thumbOutMost = v;
            else std::fprintf(stderr, "unknown aim key %s\n", key.c_str());
        }
        const Aim* aim = &tried;
        Grip start;
        start.elev = aim->elev, start.yaw = aim->yaw, start.zIndex = aim->zIndex;
        ReadKeyString(start, aim->start);
        ReadKeys(start, argc, argv, 5);
        int starts = 8;
        bool search = true;
        std::string command = std::string("build\\basegrip build <out folder> fit ") + aim->letter;
        for (int i = 5; i < argc; ++i) {
            if (!std::strncmp(argv[i], "starts=", 7))
                starts = std::max(1, std::atoi(argv[i] + 7));
            if (!std::strcmp(argv[i], "nosearch"))
                search = false;
            command += std::string(" ") + argv[i];
        }
        Grip g = search ? Fit(*aim, start, starts) : start;
        Check c;
        Solve(g, &c, true);
        float bestCost = Cost(g, *aim, start);
        std::printf("variant %c (cost %.2f): %s\n", aim->letter, bestCost, c.placed && c.thumbClosed ? "fits" : "DOESN'T");
        PrintCheck(g, c);
        PrintInHand(g);
        std::string name(1, aim->letter);
        for (int i = 5; i < argc; ++i)
            if (!std::strncmp(argv[i], "out=", 4))
                name = argv[i] + 4;
        const std::string base = outDir + "\\basegrip_" + name;
        DrawSheet(g, (base + ".bmp").c_str(), g_plain);
        const Views views = MakeViews(g);
        DrawOne(g, views.v[0], (base + "_wearer.bmp").c_str(), 900, 0.22f, true);
        DrawOne(g, views.v[1], (base + "_photo24.bmp").c_str(), 600, 0.22f, true);
        DrawOne(g, views.v[2], (base + "_photo13.bmp").c_str(), 600, 0.22f, true);
        WriteHeader(*aim, g, c, (base + ".h").c_str(), command);
        // The numbers in a line, for the pictures' captions.
        FILE* f = nullptr;
        if (!fopen_s(&f, (base + "_numbers.txt").c_str(), "w") && f) {
            std::fprintf(f, "deepest into the round %.1f mm | pads touch (gap mm, from the base mm): thumb %.1f at %.0f, index %.1f at %.0f, middle %.1f at %.0f | "
                            "clear: palm %.0f mm, ring %.0f, little %.0f | round %.0f deg out of the palm's plane\n",
                         c.deepest * 1000, c.pad[0] * 1000, c.padZ[0] * 1000, c.pad[1] * 1000, c.padZ[1] * 1000, c.pad[2] * 1000, c.padZ[2] * 1000, c.palmGap * 1000,
                         c.part[4] * 1000, c.part[5] * 1000, g.elev);
            fclose(f);
        }
        return 0;
    }
    std::fprintf(stderr, "unknown mode %s\n", argv[3]);
    return 1;
}

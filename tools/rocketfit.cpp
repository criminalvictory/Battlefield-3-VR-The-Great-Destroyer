// Fits the left glove round the launcher's case, without the game. The user, 2026-11-04: "just imagine a hand holding a cup ... His thumb is going to wrap
// around the other perimeter of the circle of the cylinder." The tube's body is 8.9 cm across (only its flange at the back is 13.6, which is what fooled the
// first two grips into hovering 2 cm off it). So: the palm against the tube, every joint of every finger laid on a circle round its axis, one way round, and
// every joint of the thumb on it the other way. Each finger stands off just as far as keeps its mesh out of the tube, no more.
//   rocketfit <build folder> <out.bmp>                                the best fit of each turn, drawn from seven sides, one row each
//   rocketfit <build folder> <out.bmp> pick <turn> <tilt> <along> <out.h>   that placement only; writes the tables for src/rocketposes.h
// All in the gloves' bind space: the tube's axis runs through O along D. turn: degrees the tube lies off straight across the hand, its little finger's end
// towards the wrist; tilt: out of the palm's plane; along: metres from the hand's middle towards the fingers.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "meshdump.h"

using namespace meshdump;

namespace {

constexpr float kMiddle[3] = {0.537f, 1.098f, 0.207f};
constexpr int8_t kDepth[handpose::kBones] = {0, 0, 3, 3, 0, 0, 0, 1, 0, 1, 3, 1, 0, 1, 0, 3, 2, 2, 1, 2, 2, 2, 3};
constexpr float kRadius = 0.0445f;
constexpr float kWidth = 1.15f;     // the user's palm width (hands.cpp g_leftWidth)
constexpr float kAllowed = 0.0025f; // a glove's soft: this far into the tube at most
constexpr float kPi = 3.14159265f;
constexpr float kSpreadApart[4] = {3.0f, 0.25f, -1.25f, -4.25f};  // the fingers' turn at their knuckles: Pose's spread 0.25

struct Tube {
    float O[3], D[3];
    float FromAxis(const float* p) const {
        const float v[3] = {p[0] - O[0], p[1] - O[1], p[2] - O[2]};
        const float a = Dot(v, D);
        const float r[3] = {v[0] - D[0] * a, v[1] - D[1] * a, v[2] - D[2] * a};
        return std::sqrt(Dot(r, r));
    }
    float Distance(const float* p) const { return FromAxis(p) - kRadius; }
};

std::vector<GloveVertex> g_glove;
std::vector<int> g_gloveTris;
std::vector<int> g_part[6];      // each finger's vertices (0 the palm with the cuff, 1 the thumb, 2-5 the fingers)
std::vector<int> g_palmOnly;     // the palm without the cuff
std::vector<int> g_laid[5];      // each finger's vertices that are kept out of the tube: its moving parts, without their roots at the palm
std::vector<int> g_seg[5][4];    // ... by segment (1-3), without the vertices at the joint before it, which the joint's bend pinches inwards
std::vector<int> g_tipPart[5];   // the last part's far end
float g_joints[5][4][3];
float g_farPart = 0.5f;    // a joint closes until what it moves touches, past this much of the part's length: "far=0.5"
float g_baseCost = 0.0f;   // what the thumb's turn at its root costs, per degree: a big one crumples the glove at the palm. "basecost=0.3"
float g_padSink = 0.006f;  // how far a last part's pad may go into the tube to bring its tip down. "pad=6" (mm)
float g_tipGap = 0.0005f;  // ... until the tip's this near the surface
bool g_why = false;
int g_why1 = 0, g_why2 = 0, g_why3 = 0, g_whySink = 0, g_whyLaid = 0, g_whyReach = 0;
float g_whyDeep = 1e9f;
int g_boneOf[5][4];              // each finger's bone by how many joints out (1-3)

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

float DeepestOf(const Tube& tube, const float (*bones)[12], const std::vector<int>& set) {
    float d = -1.0f;
    for (int i : set) {
        float q[3];
        PosePoint(bones, i, q);
        d = std::max(d, -tube.Distance(q));
    }
    return d;
}

// Where a finger's joint is (0 its base, 1, 2, 3 its tip): each moves with the bone just before it.
void JointAt(const float (*bones)[12], int finger, int joint, float* out) {
    Apply(bones[g_boneOf[finger][std::max(joint, 1)]], g_joints[finger][joint], out);
}

struct Result {
    handpose::JointPose pose;
    Tube tube{};
    float turn = 0, tilt = 0, along = 0, h = 0;
    float standOff[5] = {};    // how far each finger's joints are off the surface (mm)
    float reach[5] = {};       // how far round its tip gets from the palm, degrees (the thumb's the other way, positive too)
    float across = 0;          // how much the thumb's curl lies across the tube (1: right across it, 0: along it)
    float score = -1e30f;
};

// The angle round the tube, from the palm, positive the way the fingers go.
float RoundAngle(const Tube& tube, const float* palmWay, const float* fingersWay, const float* p) {
    const float v[3] = {p[0] - tube.O[0], p[1] - tube.O[1], p[2] - tube.O[2]};
    return std::atan2(Dot(v, fingersWay), Dot(v, palmWay)) * 180.0f / kPi;
}

// How far a part's nearest vertex is off the surface (metres; negative in it).
float GapOf(const Tube& tube, const float (*bones)[12], const std::vector<int>& set) {
    float gap = 1e9f;
    for (int i : set) {
        float q[3];
        PosePoint(bones, i, q);
        gap = std::min(gap, tube.Distance(q));
    }
    return gap;
}

// Closes one joint from low until the part it moves touches the tube, as a hand closing does. False: it's in the tube already at low.
bool Close(const Tube& tube, handpose::JointPose& pose, float& angle, float low, float high, const std::vector<int>& part) {
    float bones[handpose::kBones][12];
    float last = low;
    for (float a = low; a <= high; a += 2.0f) {
        angle = a;
        handpose::PoseJoints(pose, kWidth, bones);
        if (GapOf(tube, bones, part) < -kAllowed) {
            if (a <= low)
                return false;
            float l = last, h = a;
            for (int i = 0; i < 4; ++i) {
                const float m = (l + h) / 2;
                angle = m;
                handpose::PoseJoints(pose, kWidth, bones);
                (GapOf(tube, bones, part) < -kAllowed ? h : l) = m;
            }
            angle = l;
            return true;
        }
        last = a;
    }
    angle = high;
    return true;
}

// A straight last part can only touch a round tube at one place, and its tip then stands a centimetre off: seen as a finger lifting. The last joint goes on closing until
// the tip's on the surface too, the pad going a few millimetres in, as a pad gives.
void TipDown(const Tube& tube, handpose::JointPose& pose, float& angle, float high, int f) {
    float bones[handpose::kBones][12];
    float keep = angle;
    for (float a = angle; a <= high; a += 1.0f) {
        angle = a;
        handpose::PoseJoints(pose, kWidth, bones);
        if (GapOf(tube, bones, g_seg[f][3]) < -g_padSink) {
            angle = keep;
            return;
        }
        if (GapOf(tube, bones, g_tipPart[f]) <= g_tipGap)
            return;
        keep = a;
    }
    angle = keep;
}

struct Lay {
    float gaps[3] = {};  // each segment's gap (mm)
    float score = -1e30f;
};

// One finger closed on the tube from the palm out: the knuckle as far as it goes, or a little less, so the joints after it are bent as a hand's are, each part
// touching. The best of those.
bool LayFinger(const Tube& tube, const float* palmWay, const float* fingersWay, int k, handpose::JointPose& pose, Lay& out) {
    const int f = k + 1;
    pose.finger[k][1] = pose.finger[k][2] = 8.0f;
    if (!Close(tube, pose, pose.finger[k][0], -25.0f, 95.0f, g_seg[f][1]))
        return false;
    const float most = pose.finger[k][0];
    handpose::JointPose best = pose;
    float bones[handpose::kBones][12];
    for (float knuckle = most; knuckle >= most - 24.0f; knuckle -= 3.0f) {
        handpose::JointPose p = pose;
        p.finger[k][0] = knuckle;
        p.finger[k][2] = 8.0f;
        if (!Close(tube, p, p.finger[k][1], 8.0f, 110.0f, g_seg[f][2]))
            continue;
        if (!Close(tube, p, p.finger[k][2], 8.0f, 85.0f, g_seg[f][3]))
            continue;
        TipDown(tube, p, p.finger[k][2], 85.0f, f);
        handpose::PoseJoints(p, kWidth, bones);
        float tip[3];
        JointAt(bones, f, 3, tip);
        const float v[3] = {tip[0] - tube.O[0], tip[1] - tube.O[1], tip[2] - tube.O[2]};
        const float reach = std::atan2(Dot(v, fingersWay), Dot(v, palmWay)) * 180.0f / kPi;
        Lay lay;
        float gapSum = 0.0f;
        for (int d = 1; d <= 3; ++d) {
            lay.gaps[d - 1] = GapOf(tube, bones, g_seg[f][d]) * 1000.0f;
            gapSum += std::max(lay.gaps[d - 1], 0.0f);
        }
        // Touching first; then round as far as it goes, its middle and last joints bent alike.
        lay.score = reach - gapSum * 8.0f - std::fabs(p.finger[k][1] - p.finger[k][2]) * 0.15f;
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

// The thumb the same way, its way out of the palm given.
bool LayThumb(const Tube& tube, const float* palmWay, const float* fingersWay, handpose::JointPose& pose, Lay& out, float& reachOut, float& acrossOut) {
    pose.thumbKnuckle = pose.thumbLast = 5.0f;
    if (!Close(tube, pose, pose.thumbBase, -25.0f, 70.0f, g_seg[0][1]))
        return false;
    const float most = pose.thumbBase;
    handpose::JointPose best = pose;
    float bones[handpose::kBones][12];
    for (float base = most; base >= most - 15.0f; base -= 5.0f) {
        handpose::JointPose p = pose;
        p.thumbBase = base;
        p.thumbLast = 5.0f;
        if (!Close(tube, p, p.thumbKnuckle, 0.0f, 85.0f, g_seg[0][2]))
            continue;
        if (!Close(tube, p, p.thumbLast, 5.0f, 85.0f, g_seg[0][3]))
            continue;
        TipDown(tube, p, p.thumbLast, 85.0f, 0);
        handpose::PoseJoints(p, kWidth, bones);
        float j1[3], j2[3], tip[3];
        JointAt(bones, 0, 1, j1), JointAt(bones, 0, 2, j2), JointAt(bones, 0, 3, tip);
        const float v[3] = {tip[0] - tube.O[0], tip[1] - tube.O[1], tip[2] - tube.O[2]};
        const float reach = -std::atan2(Dot(v, fingersWay), Dot(v, palmWay)) * 180.0f / kPi;
        const float a[3] = {j2[0] - j1[0], j2[1] - j1[1], j2[2] - j1[2]}, b[3] = {tip[0] - j2[0], tip[1] - j2[1], tip[2] - j2[2]};
        float curl[3];
        Cross(a, b, curl);
        Normalize(curl);
        const float across = std::fabs(Dot(curl, tube.D));
        Lay lay;
        float gapSum = 0.0f;
        for (int d = 2; d <= 3; ++d) {
            lay.gaps[d - 1] = GapOf(tube, bones, g_seg[0][d]) * 1000.0f;
            gapSum += std::max(lay.gaps[d - 1], 0.0f);
        }
        lay.gaps[0] = GapOf(tube, bones, g_seg[0][1]) * 1000.0f;
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
// The whole hand at one placement. False: it doesn't go.
bool FitAt(float turnDeg, float tiltDeg, float along, Result& r) {
    const float t = turnDeg * kPi / 180, l = tiltDeg * kPi / 180;
    Tube tube;
    for (int k = 0; k < 3; ++k)
        tube.D[k] = (kAcross[k] * std::cos(t) - kAlong[k] * std::sin(t)) * std::cos(l) + kPalm[k] * std::sin(l);
    Normalize(tube.D);
    float E[3];
    for (int k = 0; k < 3; ++k)
        E[k] = kAlong[k] * std::cos(t) + kAcross[k] * std::sin(t);
    handpose::JointPose pose;
    std::memcpy(pose.fingerApart, kSpreadApart, sizeof(kSpreadApart));
    float bones[handpose::kBones][12];
    handpose::PoseJoints(pose, kWidth, bones);
    // Brought up out of the palm until the palm meets it.
    float low = 0.03f, high = 0.13f;
    auto place = [&](float h) {
        for (int k = 0; k < 3; ++k)
            tube.O[k] = kMiddle[k] + E[k] * along + kPalm[k] * h;
    };
    for (int step = 0; step < 12; ++step) {
        const float mid = (low + high) / 2;
        place(mid);
        (DeepestOf(tube, bones, g_palmOnly) > kAllowed ? low : high) = mid;
    }
    place(high);
    r.tube = tube, r.turn = turnDeg, r.tilt = tiltDeg, r.along = along, r.h = high;
    // Which way is which, round the tube.
    float palmWay[3] = {-kPalm[0], -kPalm[1], -kPalm[2]}, fingersWay[3];
    const float d = Dot(palmWay, tube.D);
    for (int k = 0; k < 3; ++k)
        palmWay[k] -= d * tube.D[k];
    Normalize(palmWay);
    Cross(tube.D, palmWay, fingersWay);
    if (Dot(fingersWay, E) < 0)
        for (float& v : fingersWay)
            v = -v;
    float fingersScore = 0.0f;
    for (int k = 0; k < 4; ++k) {
        Lay lay;
        if (!LayFinger(tube, palmWay, fingersWay, k, pose, lay)) {
            if (g_why)
                std::printf("finger %d wouldn't lie on it; palm at %.3f\n", k, high);
            return false;
        }
        fingersScore += lay.score * 0.25f;
    }
    handpose::PoseJoints(pose, kWidth, bones);
    for (int k = 0; k < 4; ++k) {
        float tip[3];
        JointAt(bones, k + 1, 3, tip);
        r.reach[k + 1] = RoundAngle(tube, palmWay, fingersWay, tip);
    }
    if (g_why)
        std::printf("the fingers lie on it, %.0f %.0f %.0f %.0f round; palm at %.3f\n", r.reach[1], r.reach[2], r.reach[3], r.reach[4], high);
    // The thumb: the best way out of the palm that lays it round the other way, right across the tube.
    Result best = r;
    auto tryThumb = [&](float spread, float out, float roll) {
        handpose::JointPose p = pose;
        p.thumbSpread = spread, p.thumbOut = out, p.thumbRoll = roll;
        Lay lay;
        float reach = 0.0f, across = 0.0f;
        if (!LayThumb(tube, palmWay, fingersWay, p, lay, reach, across)) {
            ++g_why1;
            return;
        }
        ++g_whyLaid;
        const float score = fingersScore + lay.score - std::fabs(roll) * 0.08f - std::max(spread - 45.0f, 0.0f) * 0.4f - std::max(out - 55.0f, 0.0f) * 0.4f - std::fabs(tiltDeg) * 1.0f -
                            std::fabs(turnDeg - 10.0f) * 0.5f - (std::fabs(spread) + std::fabs(out) + std::fabs(roll)) * g_baseCost;
        if (score > best.score) {
            best.pose = p;
            best.score = score;
            best.reach[0] = reach;
            best.across = across;
        }
    };
    for (float spread = 0.0f; spread <= 60.0f; spread += 15.0f)
        for (float out = 20.0f; out <= 80.0f; out += 15.0f)
            for (float roll = -30.0f; roll <= 60.0f; roll += 30.0f)
                tryThumb(spread, out, roll);
    if (best.score > -1e29f) {
        const float s0 = best.pose.thumbSpread, o0 = best.pose.thumbOut, r0 = best.pose.thumbRoll;
        for (float spread = s0 - 10.0f; spread <= s0 + 10.01f; spread += 5.0f)
            for (float out = o0 - 10.0f; out <= o0 + 10.01f; out += 5.0f)
                for (float roll = r0 - 20.0f; roll <= r0 + 20.01f; roll += 10.0f)
                    tryThumb(spread, out, roll);
    }
    if (best.score < -1e29f)
        return false;
    r = best;
    return true;
}

void Print(const Result& r) {
    std::printf("turn %2.0f tilt %3.0f along %6.3f palm %.3f | round: thumb %3.0f, fingers %3.0f %3.0f %3.0f %3.0f | thumb across %.2f | "
                "thumb spread %.0f out %.0f roll %.0f base %.0f knuckle %.0f last %.0f | index %.0f %.0f %.0f middle %.0f %.0f %.0f ring %.0f %.0f %.0f little %.0f %.0f %.0f | score %.0f\n",
                r.turn, r.tilt, r.along, r.h, r.reach[0], r.reach[1], r.reach[2], r.reach[3], r.reach[4], r.across, r.pose.thumbSpread, r.pose.thumbOut, r.pose.thumbRoll, r.pose.thumbBase,
                r.pose.thumbKnuckle, r.pose.thumbLast, r.pose.finger[0][0], r.pose.finger[0][1], r.pose.finger[0][2], r.pose.finger[1][0], r.pose.finger[1][1], r.pose.finger[1][2],
                r.pose.finger[2][0], r.pose.finger[2][1], r.pose.finger[2][2], r.pose.finger[3][0], r.pose.finger[3][1], r.pose.finger[3][2], r.score);
}
// How each finger's parts really lie on the tube, from the mesh: the nearest vertex of each part to the surface (mm: negative in it).
void PrintGaps(const Result& r) {
    float bones[handpose::kBones][12];
    handpose::PoseJoints(r.pose, kWidth, bones);
    std::printf("   gaps mm:");
    for (int f = 0; f < 5; ++f) {
        std::printf(" %s", f == 0 ? "thumb" : f == 1 ? "index" : f == 2 ? "middle" : f == 3 ? "ring" : "little");
        for (int d = 1; d <= 3; ++d) {
            float gap = 1e9f;
            for (int i : g_laid[f]) {
                int best = 0;
                for (int s = 1; s < 4; ++s)
                    if (g_glove[i].weight[s] > g_glove[i].weight[best])
                        best = s;
                if (kDepth[g_glove[i].bone[best]] != d)
                    continue;
                float q[3];
                PosePoint(bones, i, q);
                gap = std::min(gap, r.tube.Distance(q));
            }
            std::printf(" %.1f", gap * 1000.0f);
        }
        std::printf(" tip %.1f;", GapOf(r.tube, bones, g_tipPart[f]) * 1000.0f);
    }
    std::printf(" palm %.1f; deepest of each, all of its moving part:", -DeepestOf(r.tube, bones, g_palmOnly) * 1000.0f);
    for (int f = 0; f < 5; ++f)
        std::printf(" %.1f", DeepestOf(r.tube, bones, g_part[f + 1]) * 1000.0f);
    std::printf("\n");
}

void Draw(const std::vector<Result>& fits, const char* path) {
    constexpr int kViews = 7;
    Image img(kTile * kViews, kTile * static_cast<int>(fits.size()));
    constexpr float kFingerColour[6][3] = {{0.6f, 0.5f, 0.4f}, {0.9f, 0.25f, 0.2f}, {0.95f, 0.55f, 0.15f},
                                           {0.9f, 0.85f, 0.2f}, {0.3f, 0.8f, 0.3f}, {0.3f, 0.5f, 0.95f}};
    constexpr float kTubeColour[3] = {0.5f, 0.5f, 0.55f};
    const float scale = kTile / 0.24f;
    for (int row = 0; row < static_cast<int>(fits.size()); ++row) {
        const Result& r = fits[row];
        float bones[handpose::kBones][12];
        handpose::PoseJoints(r.pose, kWidth, bones);
        std::vector<float> placed(g_glove.size() * 3);
        for (size_t i = 0; i < g_glove.size(); ++i)
            PosePoint(bones, static_cast<int>(i), &placed[i * 3]);
        float side[3], up[3];
        Cross(r.tube.D, kPalm, side);
        Normalize(side);
        Cross(side, r.tube.D, up);
        Normalize(up);
        const float centre[3] = {r.tube.O[0] - kPalm[0] * 0.02f, r.tube.O[1] - kPalm[1] * 0.02f, r.tube.O[2] - kPalm[2] * 0.02f};
        const float* D = r.tube.D;
        const float* P = kPalm;
        // Down the tube both ways, the back of the hand, the far side, along the palm both ways, and from behind the tube a little above, as the player sees it.
        const View views[kViews] = {MakeView(D[0], D[1], D[2], up[0], up[1], up[2]),
                                    MakeView(-D[0], -D[1], -D[2], up[0], up[1], up[2]),
                                    MakeView(P[0], P[1], P[2], D[0], D[1], D[2]),
                                    MakeView(-P[0], -P[1], -P[2], D[0], D[1], D[2]),
                                    MakeView(side[0], side[1], side[2], D[0], D[1], D[2]),
                                    MakeView(-side[0], -side[1], -side[2], D[0], D[1], D[2]),
                                    MakeView(D[0] + P[0] * 0.5f - side[0] * 0.3f, D[1] + P[1] * 0.5f - side[1] * 0.3f, D[2] + P[2] * 0.5f - side[2] * 0.3f, -P[0], -P[1], -P[2])};
        for (int v = 0; v < kViews; ++v) {
            constexpr int kSides = 48;
            if (v < 2) {  // down its length a disc just past the hand, or it hides what's round it
                float past[3];
                for (int k = 0; k < 3; ++k)
                    past[k] = r.tube.O[k] + D[k] * (v == 0 ? 0.12f : -0.12f);
                for (int s = 0; s < kSides; ++s) {
                    const float a0 = 2 * kPi * s / kSides, a1 = 2 * kPi * (s + 1) / kSides;
                    float q[2][3];
                    for (int k = 0; k < 3; ++k) {
                        q[0][k] = past[k] + (side[k] * std::cos(a0) + up[k] * std::sin(a0)) * kRadius;
                        q[1][k] = past[k] + (side[k] * std::cos(a1) + up[k] * std::sin(a1)) * kRadius;
                    }
                    DrawTriangle(img, v * kTile, row * kTile, views[v], centre, scale, past, q[0], q[1], kTubeColour);
                }
            } else {
                for (int s = 0; s < kSides; ++s) {
                    const float a0 = 2 * kPi * s / kSides, a1 = 2 * kPi * (s + 1) / kSides;
                    float q[4][3];
                    for (int k = 0; k < 3; ++k) {
                        const float r0 = (side[k] * std::cos(a0) + up[k] * std::sin(a0)) * kRadius, r1 = (side[k] * std::cos(a1) + up[k] * std::sin(a1)) * kRadius;
                        q[0][k] = r.tube.O[k] + r0 - D[k] * 0.4f, q[1][k] = r.tube.O[k] + r1 - D[k] * 0.4f;
                        q[2][k] = r.tube.O[k] + r1 + D[k] * 0.4f, q[3][k] = r.tube.O[k] + r0 + D[k] * 0.4f;
                    }
                    DrawTriangle(img, v * kTile, row * kTile, views[v], centre, scale, q[0], q[1], q[2], kTubeColour);
                    DrawTriangle(img, v * kTile, row * kTile, views[v], centre, scale, q[0], q[2], q[3], kTubeColour);
                }
            }
            for (size_t t = 0; t + 2 < g_gloveTris.size(); t += 3)
                DrawTriangle(img, v * kTile, row * kTile, views[v], centre, scale, &placed[g_gloveTris[t] * 3], &placed[g_gloveTris[t + 1] * 3],
                             &placed[g_gloveTris[t + 2] * 3], kFingerColour[g_glove[g_gloveTris[t]].finger + 1]);
        }
    }
    img.Save(path);
    std::printf("saved %s\n", path);
}

void WriteTables(const Result& r, const char* path) {
    constexpr float kAxisY = -0.0935f, kHoldZ = -0.07f;
    float y[3] = {kPalm[0], kPalm[1], kPalm[2]}, x[3];
    const float d = Dot(y, r.tube.D);
    for (int k = 0; k < 3; ++k)
        y[k] -= d * r.tube.D[k];
    Normalize(y);
    Cross(y, r.tube.D, x);
    FILE* f = nullptr;
    if (fopen_s(&f, path, "w") || !f)
        return;
    std::fprintf(f, "// Fitted by tools\\rocketfit.cpp: \"pick %.0f %.0f %.3f\". The thumb %.0f degrees round from the palm, the fingers %.0f %.0f %.0f %.0f the other way.\n",
                 r.turn, r.tilt, r.along, r.reach[0], r.reach[1], r.reach[2], r.reach[3], r.reach[4]);
    float hold[12];
    for (int i = 0; i < 3; ++i) {
        hold[i * 4] = x[i], hold[i * 4 + 1] = y[i], hold[i * 4 + 2] = r.tube.D[i];
        hold[i * 4 + 3] = r.tube.O[i] - (y[i] * kAxisY + r.tube.D[i] * kHoldZ);
    }
    std::fprintf(f, "constexpr float kCaseInHand[12] = {");
    for (int k = 0; k < 12; ++k)
        std::fprintf(f, "%.5ff%s", hold[k], k == 11 ? "};\n" : ", ");
    float bones[handpose::kBones][12];
    handpose::PoseJoints(r.pose, kWidth, bones);
    std::fprintf(f, "constexpr float kFingers[handpose::kBones][12] = {\n");
    for (int b = 0; b < handpose::kBones; ++b) {
        std::fprintf(f, "    {");
        for (int k = 0; k < 12; ++k)
            std::fprintf(f, "%.5ff%s", bones[b][k], k == 11 ? "},\n" : ", ");
    }
    std::fprintf(f, "};\n");
    fclose(f);
    std::printf("wrote %s\n", path);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: rocketfit <build folder> <out.bmp> [pick turn tilt along out.h]\n");
        return 1;
    }
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    const std::string dir = argv[1];
    for (int i = 3; i < argc; ++i) {
        if (!std::strncmp(argv[i], "far=", 4))
            g_farPart = static_cast<float>(std::atof(argv[i] + 4));
        if (!std::strncmp(argv[i], "basecost=", 9))
            g_baseCost = static_cast<float>(std::atof(argv[i] + 9));
        if (!std::strncmp(argv[i], "pad=", 4))
            g_padSink = static_cast<float>(std::atof(argv[i] + 4)) * 0.001f;
    }
    if (!LoadGlove(dir, g_glove, g_gloveTris)) {
        std::fprintf(stderr, "the gloves' mesh dump isn't in %s\n", dir.c_str());
        return 1;
    }
    handpose::Joints(g_joints);
    for (int b = 0; b < handpose::kBones; ++b)
        if (kFinger[b] >= 0 && kDepth[b] >= 1)
            g_boneOf[kFinger[b]][kDepth[b]] = b;
    for (size_t i = 0; i < g_glove.size(); ++i) {
        if (g_glove[i].finger < -1)
            continue;
        g_part[g_glove[i].finger + 1].push_back(static_cast<int>(i));
        int best = 0;
        for (int s = 1; s < 4; ++s)
            if (g_glove[i].weight[s] > g_glove[i].weight[best])
                best = s;
        const int b = g_glove[i].bone[best];
        if (g_glove[i].finger == -1 && b != 0 && b != 4 && b != 5)
            g_palmOnly.push_back(static_cast<int>(i));
        // Depth-0 bones are the palm's. Past the joint before it by this much, a part's own: a first segment's root sits in the palm, and a joint's bend pinches its inside.
        const int f = g_glove[i].finger;
        if (f >= 0 && kDepth[b] >= 1) {
            const float* j0 = g_joints[f][kDepth[b] - 1];
            const float dx = g_glove[i].pos[0] - j0[0], dy = g_glove[i].pos[1] - j0[1], dz = g_glove[i].pos[2] - j0[2];
            const float* j1 = g_joints[f][kDepth[b]];
            const float ex = j1[0] - j0[0], ey = j1[1] - j0[1], ez = j1[2] - j0[2];
            const float length = std::sqrt(ex * ex + ey * ey + ez * ez);
            const float margin = kDepth[b] >= 2 ? length * g_farPart : f == 0 ? 0.025f : 0.018f;
            if (std::sqrt(dx * dx + dy * dy + dz * dz) > margin) {
                g_laid[f].push_back(static_cast<int>(i));
                g_seg[f][kDepth[b]].push_back(static_cast<int>(i));
                if (kDepth[b] == 3 && std::sqrt(dx * dx + dy * dy + dz * dz) > length * 0.75f)
                    g_tipPart[f].push_back(static_cast<int>(i));
            }
        }
    }

    if (argc > 6 && !std::strcmp(argv[3], "why")) {
        g_why = true;
        Result r;
        const bool ok = FitAt(static_cast<float>(std::atof(argv[4])), static_cast<float>(std::atof(argv[5])), static_cast<float>(std::atof(argv[6])), r);
        std::printf("%s; the thumb: never closes base %d knuckle %d last %d, into the tube %d (least %.1f mm), laid %d, not round far enough %d\n", ok ? "fits" : "doesn't", g_why1, g_why2, g_why3,
                    g_whySink, g_whyDeep * 1000.0f, g_whyLaid, g_whyReach);
        if (ok) {
            Print(r);
            PrintGaps(r);
        }
        return 0;
    }

    if (argc > 7 && !std::strcmp(argv[3], "pick")) {  // options after them
        Result r;
        if (!FitAt(static_cast<float>(std::atof(argv[4])), static_cast<float>(std::atof(argv[5])), static_cast<float>(std::atof(argv[6])), r)) {
            std::printf("it doesn't go there\n");
            return 1;
        }
        Print(r);
        PrintGaps(r);
        Draw({r}, argv[2]);
        WriteTables(r, argv[7]);
        return 0;
    }

    std::vector<Result> fits;
    for (float turn = 0.0f; turn <= 30.0f; turn += 10.0f) {
        Result best;
        for (float tilt : {0.0f, 6.0f})
            for (float along = 0.0f; along <= 0.0301f; along += 0.01f) {
                Result r;
                if (FitAt(turn, tilt, along, r) && r.score > best.score)
                    best = r;
            }
        if (best.score > -1e29f) {
            Print(best);
            PrintGaps(best);
            fits.push_back(best);
        }
    }
    if (fits.empty()) {
        std::printf("nothing fitted\n");
        return 1;
    }
    Draw(fits, argv[2]);
    return 0;
}

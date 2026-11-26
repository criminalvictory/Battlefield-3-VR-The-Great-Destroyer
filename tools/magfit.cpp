// Fits the left glove round a gun's magazine, without the game: the user, 2026-11-05, on the AN-94: "he's holding the magazine like too low... maybe in the middle of the magazine... the index finger, middle
// finger, ring finger, and pinky seems to be like really just hovering it rather than actually gripping it"; and for every gun: "Make sure that all the finger formations are really pressed against the magazine."
// So: the magazine's own mesh as a distance field. The thumb keeps the formation they liked (the M416's, pouch.cpp and hands.cpp's kOnMag). The hand goes to the middle of the part of the magazine below the
// well, turned to lie along it where it curves, its palm brought up until it meets the magazine; then each finger closes joint by joint until each of its parts is on the surface, as a hand closing does.
//   magfit <build folder> <out.bmp> <colour indices> <frame bone> <magazine bone> <below the well, y> <name> <out.h>
// Writes the tables for src/magposes.h: the magazine in the glove and each of the left glove's bones, and prints each part's gap to the surface in millimetres.
// A magazine drawn apart on a palette of its own (the M249's box, 987@27099: its bone 0 is the box) takes its frame from the gun's own draw: the frame
// bone as <bone>@<count>@<start> (3@24291@2808), the two dumps' palettes taken in the same frame; its mesh and its seat are then on that frame.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "../src/gunshape.h"
#include "meshdump.h"

using namespace meshdump;

namespace {

constexpr float kMiddle[3] = {0.537f, 1.098f, 0.207f};
constexpr int8_t kDepth[handpose::kBones] = {0, 0, 3, 3, 0, 0, 0, 1, 0, 1, 3, 1, 0, 1, 0, 3, 2, 2, 1, 2, 2, 2, 3};
constexpr float kWidth = 1.15f;      // the user's palm width (hands.cpp g_leftWidth)
constexpr float kAllowed = 0.0015f;  // a glove's soft: this far into the magazine at most
constexpr float kPi = 3.14159265f;
constexpr float kDegree = kPi / 180.0f;
// The M416's hold (pouch.cpp's g_hold, written to 3 decimals there and tidied): the magazine with the gun's model space seated -> the gloves' bind space. Its turn is the formation the user likes.
constexpr float kM416Hold[12] = {0.77207f, -0.57184f, 0.27731f, 0.32829f, 0.34822f, 0.01563f, -0.93728f, 1.44740f, 0.53164f, 0.82022f, 0.21119f, 0.21821f};
// The M416's thumb on it (hands.cpp's kOnMag...): kept, its fine turn searched round these.
constexpr float kThumb = 0.905f, kThumbOut = 45.63f, kThumbRoll = 52.72f, kThumbPress = -15.49f;
// The thumb's two outer joints aren't opened further than this past that formation: bent backwards they look broken (the user, 2026-11-05, on the M16's: "his thumb looks really weird... make sure you really fix the thumb").
constexpr float kThumbOpens = -6.0f;

struct Field {
    Solid solid;
    std::vector<float> distance;
    float At(const float* p) const {  // metres to the magazine's surface, negative inside; far off the grid: a lot
        int x, y, z;
        if (!solid.Cell(p, x, y, z))
            return 0.05f;
        return distance[solid.At(x, y, z)];
    }
};
Field g_mag;
std::vector<GloveVertex> g_glove;
std::vector<int> g_gloveTris;
std::vector<int> g_part[6];    // each finger's vertices (0 the palm with the cuff, 1 the thumb, 2-5 the fingers)
std::vector<int> g_palmOnly;   // the palm without the cuff
std::vector<int> g_seg[5][4];  // each finger's vertices by segment (1-3), without those at the joint before it, which the joint's bend pinches inwards
float g_joints[5][4][3];
Mesh g_magMesh;
std::vector<float> g_magPoints;  // y and z of points over its faces
bool g_why = false;

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

// The glove's bind space -> the gun's: the hold's inverse.
void Invert(const float* m, float* out) {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            out[i * 4 + j] = m[j * 4 + i];
    for (int i = 0; i < 3; ++i)
        out[i * 4 + 3] = -(out[i * 4] * m[3] + out[i * 4 + 1] * m[7] + out[i * 4 + 2] * m[11]);
}

void Compose(const float* a, const float* b, float* out) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j)
            out[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        out[i * 4 + 3] += a[i * 4 + 3];
    }
}

float Gap(const float* toGun, const float (*bones)[12], const std::vector<int>& set) {
    float gap = 1.0f;
    for (int i : set) {
        float q[3], g[3];
        PosePoint(bones, i, q);
        Apply(toGun, q, g);
        gap = std::min(gap, g_mag.At(g));
    }
    return gap;
}

// Closes one joint from low until the part it moves touches, as a hand closing does. False: it's in already at low.
bool Close(const float* toGun, handpose::JointPose& pose, float& angle, float low, float high, const std::vector<int>& part) {
    float bones[handpose::kBones][12];
    float last = low;
    for (float a = low; a <= high; a += 2.0f) {
        angle = a;
        handpose::PoseJoints(pose, kWidth, bones);
        if (Gap(toGun, bones, part) < -kAllowed) {
            if (a <= low)
                return false;
            float l = last, h = a;
            for (int i = 0; i < 5; ++i) {
                const float m = (l + h) / 2;
                angle = m;
                handpose::PoseJoints(pose, kWidth, bones);
                (Gap(toGun, bones, part) < -kAllowed ? h : l) = m;
            }
            angle = l;
            return true;
        }
        last = a;
    }
    angle = high;
    return true;
}

// One finger closed from the palm out: its knuckle as far as it goes or a little less, so all its parts are on the surface; the best of those: least gaps, furthest round.
bool LayFinger(const float* toGun, int k, handpose::JointPose& pose, float gaps[3]) {
    const int f = k + 1;
    pose.finger[k][1] = pose.finger[k][2] = 2.0f;
    if (!Close(toGun, pose, pose.finger[k][0], -20.0f, 95.0f, g_seg[f][1]))
        return false;
    const float most = pose.finger[k][0];
    handpose::JointPose best = pose;
    float bestScore = -1e30f;
    float bones[handpose::kBones][12];
    for (float knuckle = most; knuckle >= most - 30.0f; knuckle -= 3.0f) {
        handpose::JointPose p = pose;
        p.finger[k][0] = knuckle;
        p.finger[k][2] = 2.0f;
        if (!Close(toGun, p, p.finger[k][1], 2.0f, 110.0f, g_seg[f][2]))
            continue;
        if (!Close(toGun, p, p.finger[k][2], 2.0f, 90.0f, g_seg[f][3]))
            continue;
        handpose::PoseJoints(p, kWidth, bones);
        float g[3], sum = 0.0f;
        for (int d = 1; d <= 3; ++d) {
            g[d - 1] = Gap(toGun, bones, g_seg[f][d]) * 1000.0f;
            sum += std::max(g[d - 1], 0.0f);
        }
        const float score = -sum - std::fabs(p.finger[k][1] - p.finger[k][2]) * 0.01f;
        if (score > bestScore) {
            bestScore = score;
            best = p;
            std::copy(g, g + 3, gaps);
        }
    }
    if (bestScore < -1e29f)
        return false;
    pose = best;
    return true;
}

// The fitted hand: the thumb's bones from the M416's formation, the fingers' from the fit.
struct Result {
    float hold[12] = {};
    handpose::JointPose pose;
    handpose::Fingers thumb;
    float press = kThumbPress;
    float gaps[5][3] = {};
    float palmGap = 0.0f;
    float y = 0, tilt = 0, yaw = 0, slide = 0;
    float score = -1e30f;
    float bones[handpose::kBones][12] = {};
};

// The thumb's bones (2, 17, 18), as hands.cpp poses the formation they like.
void ThumbBones(const handpose::Fingers& fingers, float press, float (*out)[12]) {
    float posed[handpose::kBones][12];
    handpose::Pose(fingers, kWidth, posed);
    const float bend[5] = {press, 0.0f, 0.0f, 0.0f, 0.0f};
    handpose::BendFurther(posed, bend);
    for (const int b : {2, 17, 18})
        std::memcpy(out[b], posed[b], sizeof(posed[b]));
}

// The hold at this place: the M416's turn, tilted about the gun's x and turned about the magazine's length (y), its hand's middle at (0, y, z) with the palm brought up until it meets the magazine.
bool Place(float y, float tiltDeg, float yawDeg, float slide, float* hold, float (*bones)[12], float& palmGap) {
    float turned[12];
    const float t = tiltDeg * kDegree, w = yawDeg * kDegree;
    const float rx[12] = {1, 0, 0, 0, 0, std::cos(t), std::sin(t), 0, 0, -std::sin(t), std::cos(t), 0};
    const float ry[12] = {std::cos(w), 0, std::sin(w), 0, 0, 1, 0, 0, -std::sin(w), 0, std::cos(w), 0};
    float both[12];
    Compose(ry, rx, both);
    // (The hold takes the gun's space to the glove's: a turn of the gun's space goes on the right, undone.)
    float undone[12];
    Invert(both, undone);
    Compose(kM416Hold, undone, turned);
    float toGun[12];
    // Its palm's way in the gun's space: the glove's palm normal, turned into it.
    Invert(turned, toGun);
    float palmWay[3];
    for (int r = 0; r < 3; ++r)
        palmWay[r] = toGun[r * 4] * kPalm[0] + toGun[r * 4 + 1] * kPalm[1] + toGun[r * 4 + 2] * kPalm[2];
    // The hand's middle on the magazine's line at this height, slid along z; then away from the magazine along the palm's way until the palm no more than touches.
    auto at = [&](float back, float* m) {
        const float target[3] = {-palmWay[0] * back, y - palmWay[1] * back, slide - palmWay[2] * back};
        float centre[3];
        for (int r = 0; r < 3; ++r)
            centre[r] = toGun[r * 4] * kMiddle[0] + toGun[r * 4 + 1] * kMiddle[1] + toGun[r * 4 + 2] * kMiddle[2];
        std::memcpy(m, toGun, 12 * sizeof(float));
        for (int r = 0; r < 3; ++r)
            m[r * 4 + 3] = target[r] - centre[r];
    };
    float m[12];
    float posed[handpose::kBones][12];
    handpose::JointPose open;
    handpose::PoseJoints(open, kWidth, posed);
    float low = 0.0f, high = 0.08f;
    for (int i = 0; i < 12; ++i) {
        const float mid = (low + high) / 2;
        at(mid, m);
        (Gap(m, posed, g_palmOnly) < -kAllowed ? low : high) = mid;
    }
    at(high, m);
    palmGap = Gap(m, posed, g_palmOnly) * 1000.0f;
    Invert(m, hold);
    (void)bones;
    return true;
}

bool FitAt(float y, float tilt, float yaw, float slide, Result& r) {
    float hold[12], toGun[12];
    if (!Place(y, tilt, yaw, slide, hold, nullptr, r.palmGap))
        return false;
    Invert(hold, toGun);
    handpose::JointPose pose;
    for (int k = 0; k < 4; ++k)
        pose.fingerApart[k] = (3.0f - k * 2.3f) * 0.6f;  // nearly together, as a hand round something
    float gaps[5][3] = {};
    float sum = 0.0f;
    if (g_why) {
        float centre[3];
        Apply(toGun, kMiddle, centre);
        std::printf("the hand's middle on the gun at (%.3f %.3f %.3f), the palm %.1f mm off\n", centre[0], centre[1], centre[2], r.palmGap);
    }
    for (int k = 0; k < 4; ++k) {
        if (!LayFinger(toGun, k, pose, gaps[k + 1])) {
            if (g_why) {
                float bones[handpose::kBones][12];
                handpose::JointPose open = pose;
                open.finger[k][0] = -20.0f, open.finger[k][1] = open.finger[k][2] = 2.0f;
                handpose::PoseJoints(open, kWidth, bones);
                std::printf("finger %d won't lie on it: with it open, its parts %.1f %.1f %.1f mm off\n", k, Gap(toGun, bones, g_seg[k + 1][1]) * 1000.0f, Gap(toGun, bones, g_seg[k + 1][2]) * 1000.0f, Gap(toGun, bones, g_seg[k + 1][3]) * 1000.0f);
            }
            return false;
        }
        if (g_why)
            std::printf("finger %d: knuckle %.0f middle %.0f last %.0f: %.1f %.1f %.1f mm off\n", k, pose.finger[k][0], pose.finger[k][1], pose.finger[k][2], gaps[k + 1][0], gaps[k + 1][1], gaps[k + 1][2]);
        for (int d = 0; d < 3; ++d)
            sum += std::max(gaps[k + 1][d], 0.0f) * (d == 0 ? 0.5f : 1.0f);
    }
    // The thumb: the formation they like, its fine turn searched so its pad lies on the far side.
    float bones[handpose::kBones][12];
    handpose::PoseJoints(pose, kWidth, bones);
    handpose::Fingers fingers;
    fingers.spread = 0.314f;
    fingers.thumb = kThumb;
    float bestThumb = -1e30f;
    handpose::Fingers bestFingers = fingers;
    float bestPress = kThumbPress;
    float tb[3] = {};
    for (float out = kThumbOut - 12.0f; out <= kThumbOut + 12.01f; out += 6.0f)
        for (float roll = kThumbRoll - 15.0f; roll <= kThumbRoll + 15.01f; roll += 7.5f)
            for (float thumb = kThumb - 0.15f; thumb <= kThumb + 0.0751f; thumb += 0.075f)
                for (float press = kThumbPress - 20.0f; press <= kThumbPress + 20.01f; press += 10.0f) {
                    handpose::Fingers f = fingers;
                    f.thumb = thumb, f.thumbOut = out, f.thumbRoll = roll;
                    ThumbBones(f, press, bones);
                    const float g2 = Gap(toGun, bones, g_seg[0][2]) * 1000.0f, g3 = Gap(toGun, bones, g_seg[0][3]) * 1000.0f, g1 = Gap(toGun, bones, g_seg[0][1]) * 1000.0f;
                    if (g1 < -kAllowed * 1500.0f || g2 < -kAllowed * 1000.0f || g3 < -kAllowed * 1000.0f)
                        continue;
                    const float s = -std::max(g2, 0.0f) - std::max(g3, 0.0f) * 1.2f - std::fabs(out - kThumbOut) * 0.02f - std::fabs(roll - kThumbRoll) * 0.02f - std::fabs(press - kThumbPress) * 0.01f;
                    if (s > bestThumb) {
                        bestThumb = s;
                        bestFingers = f;
                        bestPress = press;
                        tb[0] = g1, tb[1] = g2, tb[2] = g3;
                    }
                }
    if (bestThumb < -1e29f)
        return false;
    ThumbBones(bestFingers, bestPress, bones);
    std::memcpy(r.bones, bones, sizeof(bones));
    std::memcpy(r.hold, hold, sizeof(hold));
    r.pose = pose;
    r.thumb = bestFingers;
    r.press = bestPress;
    std::memcpy(r.gaps, gaps, sizeof(gaps));
    std::copy(tb, tb + 3, r.gaps[0]);
    r.y = y, r.tilt = tilt, r.yaw = yaw, r.slide = slide;
    r.score = -sum + bestThumb - std::max(r.palmGap, 0.0f) * 2.0f - std::fabs(yaw) * 0.04f;
    return true;
}

void Print(const Result& r) {
    std::printf("y %.3f slide %.3f tilt %.0f yaw %.0f | palm %.1f mm | thumb %.1f %.1f %.1f (out %.0f roll %.0f thumb %.2f press %.0f) | index %.1f %.1f %.1f middle %.1f %.1f %.1f ring %.1f %.1f %.1f little %.1f %.1f %.1f | score %.1f\n", r.y,
                r.slide, r.tilt, r.yaw, r.palmGap, r.gaps[0][0], r.gaps[0][1], r.gaps[0][2], r.thumb.thumbOut, r.thumb.thumbRoll, r.thumb.thumb, r.press, r.gaps[1][0], r.gaps[1][1], r.gaps[1][2], r.gaps[2][0], r.gaps[2][1], r.gaps[2][2], r.gaps[3][0],
                r.gaps[3][1], r.gaps[3][2], r.gaps[4][0], r.gaps[4][1], r.gaps[4][2], r.score);
}

Mesh g_context;           // the rest of the gun, drawn behind the hand (the lever modes)
float g_drawSpan = 0.26f;  // metres across a tile

void Draw(const Result& r, const char* path, float* mid) {
    constexpr int kViews = 4;
    Image img(kTile * kViews, kTile);
    constexpr float kFingerColour[6][3] = {{0.6f, 0.5f, 0.4f}, {0.9f, 0.25f, 0.2f}, {0.95f, 0.55f, 0.15f}, {0.9f, 0.85f, 0.2f}, {0.3f, 0.8f, 0.3f}, {0.3f, 0.5f, 0.95f}};
    constexpr float kMagColour[3] = {0.45f, 0.45f, 0.5f};
    constexpr float kContextColour[3] = {0.28f, 0.3f, 0.36f};
    float toGun[12];
    Invert(r.hold, toGun);
    std::vector<float> placed(g_glove.size() * 3);
    for (size_t i = 0; i < g_glove.size(); ++i) {
        float q[3];
        PosePoint(r.bones, static_cast<int>(i), q);
        Apply(toGun, q, &placed[i * 3]);
    }
    const View views[kViews] = {MakeView(-1, 0, 0, 0, 1, 0), MakeView(1, 0, 0, 0, 1, 0), MakeView(0, 0, 1, 0, 1, 0), MakeView(-0.5f, 0.6f, 0.6f, 0, 1, 0)};
    const float scale = kTile / g_drawSpan;
    for (int v = 0; v < kViews; ++v) {
        for (size_t t = 0; t + 2 < g_context.tris.size(); t += 3)
            DrawTriangle(img, v * kTile, 0, views[v], mid, scale, &g_context.pos[g_context.tris[t] * 3], &g_context.pos[g_context.tris[t + 1] * 3], &g_context.pos[g_context.tris[t + 2] * 3], kContextColour);
        for (size_t t = 0; t + 2 < g_magMesh.tris.size(); t += 3)
            DrawTriangle(img, v * kTile, 0, views[v], mid, scale, &g_magMesh.pos[g_magMesh.tris[t] * 3], &g_magMesh.pos[g_magMesh.tris[t + 1] * 3], &g_magMesh.pos[g_magMesh.tris[t + 2] * 3], kMagColour);
        for (size_t t = 0; t + 2 < g_gloveTris.size(); t += 3)
            DrawTriangle(img, v * kTile, 0, views[v], mid, scale, &placed[g_gloveTris[t] * 3], &placed[g_gloveTris[t + 1] * 3], &placed[g_gloveTris[t + 2] * 3], kFingerColour[g_glove[g_gloveTris[t]].finger + 1]);
    }
    img.Save(path);
    std::printf("saved %s\n", path);
}

void WriteTables(const Result& r, const char* name, const char* path);

// How far a joint of this posed finger is already bent (degrees, positive towards the palm), so it isn't opened past straight.
void JointBends(const float (*bones)[12], int f, float* out) {
    constexpr float kNone[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    const float* at[4] = {};
    for (int b = 0; b < handpose::kBones; ++b)
        if (kFinger[b] == f && kDepth[b] >= 1 && kDepth[b] <= 3)
            at[kDepth[b]] = bones[b];
    for (int d = 1; d <= 3; ++d) {
        out[d - 1] = 0.0f;
        const float* parent = d == 1 ? kNone : at[d - 1];
        if (!at[d] || !parent)
            continue;
        // The turn from the bone before it to it, by what a bend of its own does to that turn's size.
        float inverse[12], rel[12];
        Invert(parent, inverse);
        Compose(inverse, at[d], rel);
        const float trace = rel[0] + rel[5] + rel[10];
        out[d - 1] = std::acos(std::clamp((trace - 1.0f) / 2.0f, -1.0f, 1.0f)) / kDegree;
    }
}

// The most it closes from low with this part no further in than a glove gives, or where it's nearest if it never reaches. False: in already at low.
bool CloseJoint(const float* toGun, const float (*base)[12], int f, float* angles, int joint, float low, float high, const std::vector<int>& part) {
    float bones[handpose::kBones][12];
    auto gapAt = [&](float a) {
        angles[joint] = a;
        std::memcpy(bones, base, sizeof(bones));
        handpose::BendFingerJoints(bones, f, angles);
        return Gap(toGun, bones, part);
    };
    if (gapAt(low) < -kAllowed)
        return false;
    float last = low, nearest = low, nearestGap = 1e9f;
    for (float a = low; a <= high; a += 2.5f) {
        const float g = gapAt(a);
        if (g < -kAllowed) {
            float l = last, h = a;
            for (int i = 0; i < 5; ++i) {
                const float m = (l + h) / 2;
                (gapAt(m) < -kAllowed ? h : l) = m;
            }
            angles[joint] = l;
            return true;
        }
        if (g < nearestGap)
            nearestGap = g, nearest = a;
        last = a;
    }
    angles[joint] = nearest;
    return true;
}

// One finger laid on the magazine from the knuckle out; the knuckle's angle tried across its range, the one with the least air under its parts kept. Its angles are on top of the game's pose.
bool LayOnMag(const float* toGun, const float (*base)[12], int f, float* angles, float* gaps) {
    float bent[3];
    JointBends(base, f, bent);
    const bool thumb = f == 0;
    float low[3];
    for (int j = 0; j < 3; ++j)
        low[j] = thumb ? (j == 0 ? -45.0f : kThumbOpens) : std::max(-45.0f, -bent[j] - 2.0f);
    float bestScore = -1e30f;
    bool any = false;
    float bones[handpose::kBones][12];
    for (float a1 = low[0]; a1 <= 50.0f; a1 += 4.0f) {
        float a[3] = {a1, low[1], low[2]};
        if (!CloseJoint(toGun, base, f, a, 1, low[1], 70.0f, g_seg[f][2]))
            a[1] = low[1];
        if (!CloseJoint(toGun, base, f, a, 2, low[2], 70.0f, g_seg[f][3]))
            a[2] = low[2];
        std::memcpy(bones, base, sizeof(bones));
        handpose::BendFingerJoints(bones, f, a);
        float g[3];
        for (int d = 0; d < 3; ++d)
            g[d] = Gap(toGun, bones, g_seg[f][d + 1]) * 1000.0f;
        // Least air and none of it sunk in (dear: a thick glove on a thin magazine may have no way to be quite out of it), then the nearest to how the game holds it, and no joint bent one way and the next the other: a finger, not a zigzag.
        float sunk = 0.0f;
        for (float gi : g)
            sunk += std::max(-gi - kAllowed * 1000.0f, 0.0f);
        const float score = -sunk * 6.0f - (std::max(g[0], 0.0f) * (thumb ? 0.3f : 0.6f) + std::max(g[1], 0.0f) + std::max(g[2], 0.0f) * 1.2f) - 0.02f * (std::fabs(a[0]) + std::fabs(a[1]) + std::fabs(a[2])) - 0.03f * std::fabs(a[1] - a[2]);
        if (score > bestScore) {
            bestScore = score;
            std::copy(a, a + 3, angles);
            std::copy(g, g + 3, gaps);
            any = true;
        }
    }
    return any;
}

// Where a finger's joint is across the gun (x): joint 0-2 from the palm out, 3 its tip.
float PartX(const float* toGun, const float (*bones)[12], int f, int joint) {
    const float* bone = nullptr;
    const int depth = std::min(joint + 1, 3);
    for (int b = 0; b < handpose::kBones; ++b)
        if (kFinger[b] == f && kDepth[b] == depth)
            bone = bones[b];
    if (!bone)
        return 0.0f;
    float posed[3], onGun[3];
    Apply(bone, g_joints[f][joint], posed);
    Apply(toGun, posed, onGun);
    return onGun[0];
}

// The game's hold moved to the middle of the part below the well, round the magazine's own curve, then shifted about there for the place where every finger lies on it with the least air.
void Grip(const float* vanillaHold, const float (*vanillaFingers)[12], float wellY, const float* lo, float midY, float midZ, const char* image, const char* name, const char* out) {
    // The magazine's curve: a circle through its back edge. (Cut level, its front edge turns at the bottom, where the magazine ends.)
    std::vector<std::pair<double, double>> edge;
    for (float y = lo[1] + 0.012f; y < wellY - 0.004f; y += 0.006f) {
        float back = 1e9f;
        for (size_t p = 0; p + 1 < g_magPoints.size(); p += 2) {
            if (std::fabs(g_magPoints[p] - y) > 0.006f)
                continue;
            back = std::min(back, g_magPoints[p + 1]);
        }
        if (back < 1e8f)
            edge.push_back({y, back});
    }
    // A straight magazine, or too short to tell: far in front of it, so "out" is backward.
    double radius = 5.0, cy = midY, cz = midZ + radius;
    // Some levels catch no vertex of the edge itself, and stand off behind it: only the lower hull of them is the edge.
    {
        std::vector<std::pair<double, double>> hull;
        std::sort(edge.begin(), edge.end());
        for (const auto& p : edge) {
            while (hull.size() >= 2) {
                const auto& a = hull[hull.size() - 2];
                const auto& b = hull.back();
                if ((b.first - a.first) * (p.second - a.second) - (b.second - a.second) * (p.first - a.first) <= 0.0)
                    hull.pop_back();
                else
                    break;
            }
            hull.push_back(p);
        }
        // Either end may be such a level too: the edge only ever runs one way.
        auto slope = [&](size_t i) { return (hull[i + 1].second - hull[i].second) / (hull[i + 1].first - hull[i].first); };
        if (hull.size() >= 4) {
            const double way = slope(hull.size() / 2);
            while (hull.size() >= 4 && slope(hull.size() - 2) * way <= 0.0)
                hull.pop_back();
            while (hull.size() >= 4 && slope(0) * way <= 0.0)
                hull.erase(hull.begin());
        }
        edge.swap(hull);
    }
    // From how much its slope turns along its length: a fit of the circle itself is thrown by the few levels left on a nearly straight magazine.
    if (edge.size() >= 4) {
        auto lineOf = [&](size_t from, size_t to, double& a, double& midY2, double& midZ2) {
            double sy = 0, sz = 0, syy = 0, syz = 0;
            const double n = static_cast<double>(to - from);
            for (size_t i = from; i < to; ++i)
                sy += edge[i].first, sz += edge[i].second, syy += edge[i].first * edge[i].first, syz += edge[i].first * edge[i].second;
            a = (n * syz - sy * sz) / (n * syy - sy * sy);
            midY2 = sy / n, midZ2 = sz / n;
        };
        double a1, a2, y1, y2, z1, z2;
        const size_t half = edge.size() / 2;
        lineOf(0, half + 1, a1, y1, z1);
        lineOf(half, edge.size(), a2, y2, z2);
        const double turnedBy = std::fabs(std::atan(a2) - std::atan(a1));
        const double length = std::hypot(y2 - y1, z2 - z1);
        radius = std::clamp(length / std::max(turnedBy, 1e-4), 0.1, 5.0);
        const double am = std::tan((std::atan(a1) + std::atan(a2)) / 2);
        const double norm = std::sqrt(1.0 + am * am);
        cy = (y1 + y2) / 2 - am / norm * radius;
        cz = (z1 + z2) / 2 + 1.0 / norm * radius;
    }
    std::printf("the magazine's curve: a circle about y %.3f, z %.3f, radius %.3f m, through %zu levels\n", cy, cz, radius, edge.size());
    float toGun0[12];
    Invert(vanillaHold, toGun0);
    float m0[3];
    Apply(toGun0, kMiddle, m0);
    std::printf("the game's hand's middle at (%.3f %.3f %.3f); the magazine's middle below the well y %.3f, z %.3f\n", m0[0], m0[1], m0[2], midY, midZ);
    // Round the curve until the hand's middle is level with the magazine's.
    float turned[12];
    std::memcpy(turned, toGun0, sizeof(turned));
    float turn = 0.0f;
    {
        float best = 1e9f;
        const float reach = std::min(70.0f, static_cast<float>(0.25 / radius) / kDegree);
        for (float t = -reach; t <= reach; t += reach / 400.0f) {
            const float c = std::cos(t * kDegree), s = std::sin(t * kDegree);
            const float y = static_cast<float>(cy + (m0[1] - cy) * c - (m0[2] - cz) * s);
            if (std::fabs(y - midY) < best)
                best = std::fabs(y - midY), turn = t;
        }
        const float c = std::cos(turn * kDegree), s = std::sin(turn * kDegree);
        const float rot[12] = {1, 0, 0, 0, 0, c, -s, static_cast<float>(cy - c * cy + s * cz), 0, s, c, static_cast<float>(cz - s * cy - c * cz)};
        Compose(rot, toGun0, turned);
    }
    float m1[3];
    Apply(turned, kMiddle, m1);
    std::printf("turned %.1f degrees round it: its middle at (%.3f %.3f %.3f)\n", turn, m1[0], m1[1], m1[2]);
    const float vanillaThumbX = PartX(toGun0, vanillaFingers, 0, 3);
    std::printf("in the game's grip the thumb's tip is at x %.3f, the index knuckle at x %.3f\n", vanillaThumbX, PartX(toGun0, vanillaFingers, 1, 0));
    // Out from the curve's middle, and along it.
    float radial[2] = {static_cast<float>(m1[1] - cy), static_cast<float>(m1[2] - cz)};
    const float rn = std::hypot(radial[0], radial[1]);
    radial[0] /= rn, radial[1] /= rn;
    const float along[2] = {-radial[1], radial[0]};
    struct Try {
        float toGun[12];
        float angles[5][3], gaps[5][3], palm;
        float score;
    };
    Try best{};
    best.score = -1e30f;
    int why[7] = {};
    auto attempt = [&](float dx, float dr, float dl, float dt) {
        Try t{};
        // Tilted about the gun's x through the hand's middle, as the knuckles' line may lie better on the edge.
        {
            const float c = std::cos(dt * kDegree), s = std::sin(dt * kDegree);
            const float rot[12] = {1, 0, 0, 0, 0, c, -s, m1[1] - c * m1[1] + s * m1[2], 0, s, c, m1[2] - s * m1[1] - c * m1[2]};
            Compose(rot, turned, t.toGun);
        }
        t.toGun[3] += dx;
        t.toGun[7] += dr * radial[0] + dl * along[0];
        t.toGun[11] += dr * radial[1] + dl * along[1];
        t.palm = Gap(t.toGun, vanillaFingers, g_palmOnly) * 1000.0f;
        if (t.palm < -kAllowed * 1000.0f) {
            ++why[0];
            return;
        }
        float score = -std::max(t.palm, 0.0f) * 0.15f - std::fabs(dl) * 200.0f - std::fabs(dt) * 0.03f;
        for (int f = 0; f < 5; ++f) {
            if (!LayOnMag(t.toGun, vanillaFingers, f, t.angles[f], t.gaps[f])) {
                ++why[1 + f];
                return;
            }
            score -= std::max(t.gaps[f][0], 0.0f) * (f == 0 ? 0.3f : 0.6f) + std::max(t.gaps[f][1], 0.0f) + std::max(t.gaps[f][2], 0.0f) * 1.2f;
            for (float gi : t.gaps[f])
                score -= std::max(-gi - kAllowed * 1000.0f, 0.0f) * 6.0f;
            score -= (f == 0 ? 0.02f : 0.01f) * (std::fabs(t.angles[f][0]) + std::fabs(t.angles[f][1]) + std::fabs(t.angles[f][2])) + 0.02f * std::fabs(t.angles[f][1] - t.angles[f][2]);
        }
        // The same grip as the game's: the thumb on its own face, its tip against that face, and the fingers' knuckles on the other side of the magazine from it.
        {
            float bones[handpose::kBones][12];
            std::memcpy(bones, vanillaFingers, sizeof(bones));
            for (int f = 0; f < 5; ++f)
                handpose::BendFingerJoints(bones, f, t.angles[f]);
            const float thumbX = PartX(t.toGun, bones, 0, 3), knuckleX = PartX(t.toGun, bones, 1, 0);
            if (thumbX * vanillaThumbX <= 0.0f || thumbX * knuckleX >= 0.0f) {
                ++why[6];
                return;
            }
        }
        t.score = score;
        if (t.score > best.score)
            best = t;
    };
    float bdx = 0.0f, bdr = 0.0f, bdt = 0.0f;
    int tried = 0;
    for (float dt = -16.0f; dt <= 16.01f; dt += 8.0f)
        for (float dx = -0.016f; dx <= 0.0241f; dx += 0.004f)
            for (float dr = -0.02f; dr <= 0.0501f; dr += 0.005f) {
                const float before = best.score;
                attempt(dx, dr, 0.0f, dt);
                ++tried;
                if (best.score > before)
                    bdx = dx, bdr = dr, bdt = dt;
            }
    std::printf("%d places tried, the best %.1f (left out: %d the palm in it, fingers that wouldn't lie %d %d %d %d %d, %d on the wrong sides)\n", tried, best.score, why[0], why[1], why[2], why[3], why[4], why[5], why[6]);
    if (best.score < -1e29f) {
        std::printf("nowhere fitted\n");
        return;
    }
    float bdl = 0.0f;
    const float cdx = bdx, cdr = bdr, cdt = bdt;
    for (float dt = cdt - 6.0f; dt <= cdt + 6.01f; dt += 3.0f)
        for (float dx = cdx - 0.004f; dx <= cdx + 0.0041f; dx += 0.002f)
            for (float dr = cdr - 0.005f; dr <= cdr + 0.0051f; dr += 0.0025f)
                for (float dl = 0.0f; dl <= 0.0121f; dl += 0.006f) {  // only ever further up: "he's holding the magazine too low"
                    const float before = best.score;
                    attempt(dx, dr, dl, dt);
                    if (best.score > before)
                        bdx = dx, bdr = dr, bdl = dl, bdt = dt;
                }
    std::printf("shifted %.1f mm across, %.1f out, %.1f along, tilted %.0f degrees; the score %.1f\n", bdx * 1000.0f, bdr * 1000.0f, bdl * 1000.0f, bdt, best.score);
    // Each finger once more at that place, every bend of its three joints tried: the least air, then the smoothest curl, a finger and not a zigzag. And twice for the fingers: the second time near the others', a hand and not four strangers.
    for (int pass = 0; pass < 2; ++pass)
    for (int f = pass; f < 5; ++f) {
        float bent[3];
        JointBends(vanillaFingers, f, bent);
        float low[3];
        for (int j = 0; j < 3; ++j)
            low[j] = f == 0 ? (j == 0 ? -40.0f : kThumbOpens) : std::max(-40.0f, -bent[j] - 2.0f);
        float top = -1e30f;
        float others[3] = {};
        for (int j = 0; j < 3; ++j) {
            float v[4] = {best.angles[1][j], best.angles[2][j], best.angles[3][j], best.angles[4][j]};
            std::sort(v, v + 4);
            others[j] = (v[1] + v[2]) / 2;
        }
        const float together = pass == 1 ? 0.06f : 0.0f;
        float bones[handpose::kBones][12];
        for (float a1 = low[0]; a1 <= 50.0f; a1 += 4.0f)
            for (float a2 = low[1]; a2 <= 60.0f; a2 += 4.0f)
                for (float a3 = low[2]; a3 <= 60.0f; a3 += 4.0f) {
                    const float a[3] = {a1, a2, a3};
                    std::memcpy(bones, vanillaFingers, sizeof(bones));
                    handpose::BendFingerJoints(bones, f, a);
                    const float g1 = Gap(best.toGun, bones, g_seg[f][1]) * 1000.0f;
                    const float g2 = Gap(best.toGun, bones, g_seg[f][2]) * 1000.0f;
                    const float g3 = Gap(best.toGun, bones, g_seg[f][3]) * 1000.0f;
                    const float sunk = std::max(-g1 - kAllowed * 1000.0f, 0.0f) + std::max(-g2 - kAllowed * 1000.0f, 0.0f) + std::max(-g3 - kAllowed * 1000.0f, 0.0f);
                    const float score = -sunk * 6.0f - (std::max(g1, 0.0f) * (f == 0 ? 0.3f : 0.6f) + std::max(g2, 0.0f) + std::max(g3, 0.0f) * 1.2f) - 0.01f * (std::fabs(a1) + std::fabs(a2) + std::fabs(a3)) - 0.03f * std::fabs(a2 - a3) -
                                        together * (std::fabs(a1 - others[0]) + std::fabs(a2 - others[1]) + std::fabs(a3 - others[2]));
                    if (score > top) {
                        top = score;
                        best.angles[f][0] = a1, best.angles[f][1] = a2, best.angles[f][2] = a3;
                    }
                }
    }
    Result r;
    std::memcpy(r.bones, vanillaFingers, sizeof(r.bones));
    for (int f = 0; f < 5; ++f) {
        handpose::BendFingerJoints(r.bones, f, best.angles[f]);
        for (int d = 0; d < 3; ++d)
            r.gaps[f][d] = Gap(best.toGun, r.bones, g_seg[f][d + 1]) * 1000.0f;
        std::printf("%s: bent %.0f %.0f %.0f more; its parts %.1f %.1f %.1f mm off\n", f == 0 ? "thumb" : f == 1 ? "index" : f == 2 ? "middle" : f == 3 ? "ring" : "little", best.angles[f][0], best.angles[f][1], best.angles[f][2], r.gaps[f][0], r.gaps[f][1], r.gaps[f][2]);
    }
    r.palmGap = Gap(best.toGun, r.bones, g_palmOnly) * 1000.0f;
    std::printf("the palm %.1f mm off\n", r.palmGap);
    float m2[3];
    Apply(best.toGun, kMiddle, m2);
    std::printf("the hand's middle at (%.3f %.3f %.3f)\n", m2[0], m2[1], m2[2]);
    Invert(best.toGun, r.hold);
    r.y = m2[1], r.slide = m2[2], r.tilt = turn;
    float middle[3] = {0.0f, midY, midZ};
    Draw(r, image, middle);
    WriteTables(r, name, out);
}

void WriteTables(const Result& r, const char* name, const char* path) {
    FILE* f = nullptr;
    if (fopen_s(&f, path, "w") || !f) {
        std::printf("can't write %s\n", path);
        return;
    }
    std::fprintf(f, "// Fitted by tools\\magfit.cpp: y %.3f, slide %.3f, tilt %.0f, yaw %.0f. Each part's gap to the magazine in mm: palm %.1f; thumb %.1f %.1f %.1f; index %.1f %.1f %.1f; middle %.1f %.1f %.1f; ring %.1f %.1f %.1f; little %.1f %.1f %.1f.\n", r.y, r.slide,
                 r.tilt, r.yaw, r.palmGap, r.gaps[0][0], r.gaps[0][1], r.gaps[0][2], r.gaps[1][0], r.gaps[1][1], r.gaps[1][2], r.gaps[2][0], r.gaps[2][1], r.gaps[2][2], r.gaps[3][0], r.gaps[3][1], r.gaps[3][2], r.gaps[4][0], r.gaps[4][1], r.gaps[4][2]);
    std::fprintf(f, "inline constexpr float k%sHold[12] = {", name);
    for (int k = 0; k < 12; ++k)
        std::fprintf(f, "%.5ff%s", r.hold[k], k == 11 ? "};\n" : ", ");
    std::fprintf(f, "inline constexpr float k%sFingers[handpose::kBones][12] = {\n", name);
    for (int b = 0; b < handpose::kBones; ++b) {
        std::fprintf(f, "    {");
        for (int k = 0; k < 12; ++k)
            std::fprintf(f, "%.5ff%s", r.bones[b][k], k == 11 ? "},\n" : ", ");
    }
    std::fprintf(f, "};\n");
    fclose(f);
    std::printf("wrote %s\n", path);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 9) {
        std::fprintf(stderr, "usage: magfit <build folder> <out.bmp> <colour indices> <frame bone> <magazine bone> <below the well, y> <name> <out.h>\n");
        return 1;
    }
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    const std::string dir = argv[1];
    int count = 0, drawStart = 0;
    ParseDraw(argv[3], count, drawStart);
    const int frame = std::atoi(argv[4]), magBone = std::atoi(argv[5]);
    const float wellY = static_cast<float>(std::atof(argv[6]));
    // A magazine drawn apart (the frame bone given as <bone>@<count>@<start>): its own bone on the gun's frame, from the two dumps' palettes.
    float partOnFrame[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    const bool apart = std::strchr(argv[4], '@') != nullptr;
    if (apart) {
        int gunCount = 0, gunStart = 0;
        ParseDraw(std::strchr(argv[4], '@') + 1, gunCount, gunStart);
        const auto own = ReadAll(dir + "\\mesh_" + std::to_string(count) + "_" + std::to_string(drawStart) + "_bones.bin");
        const auto gun = ReadAll(dir + "\\mesh_" + std::to_string(gunCount) + "_" + std::to_string(gunStart) + "_bones.bin");
        if (own.size() < 48u * (magBone + 1) || gun.size() < 48u * (frame + 1)) {
            std::fprintf(stderr, "the dumps' bones of %d@%d and %d@%d aren't in %s\n", count, drawStart, gunCount, gunStart, dir.c_str());
            return 1;
        }
        float inverse[12];
        Invert(reinterpret_cast<const float*>(gun.data()) + frame * 12, inverse);
        Compose(inverse, reinterpret_cast<const float*>(own.data()) + magBone * 12, partOnFrame);
        std::printf("its bone %d on %d@%d's frame bone %d: at (%.4f %.4f %.4f)\n", magBone, gunCount, gunStart, frame, partOnFrame[3], partOnFrame[7], partOnFrame[11]);
    }
    if (!LoadDraw(dir, count, apart ? magBone : frame, {}, g_magMesh, magBone, 0, drawStart)) {
        std::fprintf(stderr, "draw %d's mesh isn't in %s\n", count, dir.c_str());
        return 1;
    }
    if (apart)
        for (size_t v = 0; v + 2 < g_magMesh.pos.size(); v += 3) {
            float onFrame[3];
            Apply(partOnFrame, &g_magMesh.pos[v], onFrame);
            std::memcpy(&g_magMesh.pos[v], onFrame, sizeof(onFrame));
        }
    float lo[3] = {1e9f, 1e9f, 1e9f}, hi[3] = {-1e9f, -1e9f, -1e9f};
    for (int v : g_magMesh.tris)
        for (int k = 0; k < 3; ++k)
            lo[k] = std::min(lo[k], g_magMesh.pos[v * 3 + k]), hi[k] = std::max(hi[k], g_magMesh.pos[v * 3 + k]);
    std::printf("the magazine: x %.3f..%.3f, y %.3f..%.3f, z %.3f..%.3f\n", lo[0], hi[0], lo[1], hi[1], lo[2], hi[2]);
    // Its shape as a distance field, 1 mm voxels with 6 cm of air round it. (Every triangle: leaveOut -1, no part is that.)
    float from[3], to[3];
    for (int k = 0; k < 3; ++k)
        from[k] = lo[k] - 0.06f, to[k] = hi[k] + 0.06f;
    g_mag.solid.Build(g_magMesh, from, to, 0.001f, -1);
    g_mag.distance = DistanceField(g_mag.solid);
    if (!LoadGlove(dir, g_glove, g_gloveTris)) {
        std::fprintf(stderr, "the gloves' mesh dump isn't in %s\n", dir.c_str());
        return 1;
    }
    handpose::Joints(g_joints);
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
        const int f = g_glove[i].finger;
        if (f >= 0 && kDepth[b] >= 1) {
            const float* j0 = g_joints[f][kDepth[b] - 1];
            const float dx = g_glove[i].pos[0] - j0[0], dy = g_glove[i].pos[1] - j0[1], dz = g_glove[i].pos[2] - j0[2];
            const float* j1 = g_joints[f][kDepth[b]];
            const float ex = j1[0] - j0[0], ey = j1[1] - j0[1], ez = j1[2] - j0[2];
            const float length = std::sqrt(ex * ex + ey * ey + ez * ez);
            const float margin = kDepth[b] >= 2 ? length * 0.35f : f == 0 ? 0.025f : 0.018f;
            if (std::sqrt(dx * dx + dy * dy + dz * dz) > margin)
                g_seg[f][kDepth[b]].push_back(static_cast<int>(i));
        }
    }
    // Points over its faces, not only its corners: a plain box of a magazine has no vertex between its top and its bottom.
    for (size_t t = 0; t + 2 < g_magMesh.tris.size(); t += 3) {
        const float* a = &g_magMesh.pos[g_magMesh.tris[t] * 3];
        const float* b = &g_magMesh.pos[g_magMesh.tris[t + 1] * 3];
        const float* c = &g_magMesh.pos[g_magMesh.tris[t + 2] * 3];
        float span = 0.0f;
        for (int k = 0; k < 3; ++k)
            span = std::max({span, std::fabs(b[k] - a[k]), std::fabs(c[k] - a[k]), std::fabs(c[k] - b[k])});
        const int steps = std::clamp(static_cast<int>(span / 0.004f), 1, 60);
        for (int i = 0; i <= steps; ++i)
            for (int j = 0; i + j <= steps; ++j) {
                const float u = static_cast<float>(i) / steps, w = static_cast<float>(j) / steps;
                g_magPoints.push_back(a[1] + (b[1] - a[1]) * u + (c[1] - a[1]) * w);
                g_magPoints.push_back(a[2] + (b[2] - a[2]) * u + (c[2] - a[2]) * w);
            }
    }
    // The middle of what's below the well, and how the magazine lies there: its front and back edges' slope.
    const float midY = (wellY + lo[1]) / 2;
    const float band = std::min(0.04f, std::max(wellY - midY, 0.03f));
    float zFrontUp = -1e9f, zBackUp = 1e9f, zFrontDown = -1e9f, zBackDown = 1e9f;
    for (size_t p = 0; p + 1 < g_magPoints.size(); p += 2) {
        const float y = g_magPoints[p], z = g_magPoints[p + 1];
        if (y < midY + band && y > midY + band / 2)
            zFrontUp = std::max(zFrontUp, z), zBackUp = std::min(zBackUp, z);
        if (y > midY - band && y < midY - band / 2)
            zFrontDown = std::max(zFrontDown, z), zBackDown = std::min(zBackDown, z);
    }
    // (A lever's too short for those bands: just its middle.)
    const bool bands = zFrontUp > -1e8f && zFrontDown > -1e8f;
    const float slope = bands ? std::atan2((zFrontDown + zBackDown - zFrontUp - zBackUp) / 2, band * 1.5f) / kDegree : 0.0f;
    const float midZ = bands ? (zFrontUp + zBackUp + zFrontDown + zBackDown) / 4 : (lo[2] + hi[2]) / 2;
    std::printf("the middle below the well: y %.3f, z %.3f; it leans %.0f degrees forward there; %.1f cm from back to front\n", midY, midZ, slope, ((zFrontUp - zBackUp + zFrontDown - zBackDown) / 2) * 100.0f);
    // The hand as the game's own reload holds a magazine (vanilla first), each finger closed onto the mesh from there:
    //   ... vanilla <its allrec> <from ms> <to ms> [up, m] [forward, m]
    // Or moved to the middle of the magazine and laid on it finger by finger (the user: "really pressed against the magazine", "not holding the magazine too low"):
    //   ... grip <its allrec> <from ms> <to ms> [the recording's magazine bone, if the reload's fresh one is another] [a moment of the recording with it home, ms]
    //   ... grip <its allrec> <from ms> <to ms> seated   (a magazine drawn apart that the recording never drew: a stretch with the hand on it seated)
    // Or starting from the grip another gun's reload has on its magazine, moved onto this magazine's middle and turned to its lean:
    //   ... gripfrom <that fit's out.h>.start
    if (argc > 10 && !std::strcmp(argv[9], "gripfrom")) {
        float their[3], hold[12], fingers[handpose::kBones][12];
        if (!std::strcmp(argv[10], "m416")) {
            // The M416's own hold, the one the user went by (pouch.cpp's g_hold and hands.cpp's kOnMag): the palm on the magazine's left side, the fingers round its front edge, the thumb round its back.
            Mesh rifle;
            if (!LoadGun(dir, rifle)) {
                std::printf("the M416's dump isn't there\n");
                return 1;
            }
            float rifleLow = 1e9f, rifleTop = -1e9f;
            for (size_t v = 0; v < rifle.part.size(); ++v)
                if (rifle.part[v] == kMagazineBone)
                    rifleLow = std::min(rifleLow, rifle.pos[v * 3 + 1]), rifleTop = std::max(rifleTop, rifle.pos[v * 3 + 1]);
            const float rifleMidY = (rifleTop - 0.045f + rifleLow) / 2;
            float front = -1e9f, back = 1e9f;
            for (size_t v = 0; v < rifle.part.size(); ++v)
                if (rifle.part[v] == kMagazineBone && std::fabs(rifle.pos[v * 3 + 1] - rifleMidY) < 0.05f)
                    front = std::max(front, rifle.pos[v * 3 + 2]), back = std::min(back, rifle.pos[v * 3 + 2]);
            their[0] = rifleMidY, their[1] = (front + back) / 2, their[2] = 11.77f;  // its lean by main's own measure, on this dump at this height
            std::printf("the M416's magazine: y %.3f to %.3f, its middle y %.3f, z %.3f\n", rifleLow, rifleTop, their[0], their[1]);
            const float rifleHold[12] = {0.77207f, -0.57184f, 0.27731f, 0.32829f, 0.34822f, 0.01563f, -0.93728f, 1.44740f, 0.53164f, 0.82022f, 0.21119f, 0.21821f};
            std::memcpy(hold, rifleHold, sizeof(hold));
            // Made exactly orthonormal, as pouch.cpp's Tidy.
            {
                float* r0 = hold;
                float* r1 = hold + 4;
                float* r2 = hold + 8;
                float n = std::sqrt(r0[0] * r0[0] + r0[1] * r0[1] + r0[2] * r0[2]);
                r0[0] /= n, r0[1] /= n, r0[2] /= n;
                const float d = r1[0] * r0[0] + r1[1] * r0[1] + r1[2] * r0[2];
                for (int i = 0; i < 3; ++i)
                    r1[i] -= d * r0[i];
                n = std::sqrt(r1[0] * r1[0] + r1[1] * r1[1] + r1[2] * r1[2]);
                r1[0] /= n, r1[1] /= n, r1[2] /= n;
                r2[0] = r0[1] * r1[2] - r0[2] * r1[1];
                r2[1] = r0[2] * r1[0] - r0[0] * r1[2];
                r2[2] = r0[0] * r1[1] - r0[1] * r1[0];
            }
            handpose::Fingers on{};
            constexpr float kOnMag[4] = {0.306f, 0.281f, 0.219f, 0.156f};
            for (int i = 0; i < 4; ++i)
                on.curl[i] = kOnMag[i];
            on.thumb = 0.905f, on.spread = 0.314f, on.thumbOut = 45.63f, on.thumbRoll = 52.72f;
            handpose::Pose(on, 1.15f, fingers);
            const float press[5] = {-15.49f, 0.0f, 0.0f, 0.0f, 0.0f};
            handpose::BendFurther(fingers, press);
        } else {
            FILE* f = nullptr;
            if (fopen_s(&f, argv[10], "rb") || !f) {
                std::printf("can't read %s\n", argv[10]);
                return 1;
            }
            const bool whole = fread(their, 4, 3, f) == 3 && fread(hold, sizeof(hold), 1, f) == 1 && fread(fingers, sizeof(fingers), 1, f) == 1;
            fclose(f);
            if (!whole)
                return 1;
        }
        // The way down each magazine there, in the gun's y and z.
        const float turn = (their[2] - slope) * kDegree;  // this matrix lowers a magazine's lean by turn
        const float c = std::cos(turn), s = std::sin(turn);
        const float move[12] = {1, 0, 0, 0, 0, c, -s, midY - (c * their[0] - s * their[1]), 0, s, c, midZ - (s * their[0] + c * their[1])};
        float toGun[12], moved[12], newHold[12];
        Invert(hold, toGun);
        Compose(move, toGun, moved);
        Invert(moved, newHold);
        std::printf("starting from %s, turned %.1f degrees\n", argv[10], turn / kDegree);
        // ... exactly that hand, not refitted: its thumb round the magazine as it was (the user, 2026-11-05, on the M16's, refitted: "I want that thumb to be wrapping around the magazine. You had it right with the other guns"). Moved up, forward and across on the magazine (m), the fingers each opened or closed only as far as they need not to go through it.
        if (argc > 14 && !std::strcmp(argv[11], "asis")) {
            float there[12];
            Invert(newHold, there);
            there[7] += static_cast<float>(std::atof(argv[12])), there[11] += static_cast<float>(std::atof(argv[13])), there[3] += static_cast<float>(std::atof(argv[14]));
            Result r;
            Invert(there, r.hold);
            std::memcpy(r.bones, fingers, sizeof(r.bones));
            const auto report = [&](const char* when) {
                std::printf("%s: palm %.1f;", when, Gap(there, r.bones, g_palmOnly) * 1000.0f);
                for (int f = 0; f < 5; ++f)
                    std::printf(" %s %.1f %.1f %.1f;", f == 0 ? "thumb" : f == 1 ? "index" : f == 2 ? "middle" : f == 3 ? "ring" : "little", Gap(there, r.bones, g_seg[f][1]) * 1000.0f, Gap(there, r.bones, g_seg[f][2]) * 1000.0f, Gap(there, r.bones, g_seg[f][3]) * 1000.0f);
                std::printf("\n");
            };
            report("as it is");
            // Each finger bent as a whole onto the magazine, or back out of it; the thumb only back out, and a little: it stays as it was, round the magazine.
            float bends[5] = {};
            for (int f = 0; f < 5; ++f) {
                float best = 0.0f;
                bool any = false;
                for (float bend = f == 0 ? -20.0f : -30.0f; bend <= (f == 0 ? 0.01f : 30.01f); bend += 1.0f) {
                    float tried[handpose::kBones][12];
                    std::memcpy(tried, r.bones, sizeof(tried));
                    float all[5] = {};
                    all[f] = bend;
                    handpose::BendFurther(tried, all);
                    const float least = std::min({Gap(there, tried, g_seg[f][1]), Gap(there, tried, g_seg[f][2]), Gap(there, tried, g_seg[f][3])});
                    if (least < -kAllowed) {
                        if (any)
                            break;
                        continue;
                    }
                    best = bend;
                    any = true;
                }
                bends[f] = best;
            }
            handpose::BendFurther(r.bones, bends);
            std::printf("bent thumb %.0f, index %.0f, middle %.0f, ring %.0f, little %.0f\n", bends[0], bends[1], bends[2], bends[3], bends[4]);
            report("closed on it");
            for (int f = 0; f < 5; ++f)
                for (int d = 0; d < 3; ++d)
                    r.gaps[f][d] = Gap(there, r.bones, g_seg[f][d + 1]) * 1000.0f;
            r.palmGap = Gap(there, r.bones, g_palmOnly) * 1000.0f;
            float m[3];
            Apply(there, kMiddle, m);
            std::printf("the hand's middle at (%.3f %.3f %.3f)\n", m[0], m[1], m[2]);
            r.y = m[1], r.slide = m[2];
            float middle[3] = {0.0f, midY, midZ};
            Draw(r, argv[2], middle);
            WriteTables(r, argv[7], argv[8]);
            return 0;
        }
        Grip(newHold, fingers, wellY, lo, midY, midZ, argv[2], argv[7], argv[8]);
        return 0;
    }
    // The hand from a hold (the game's, or another gun's moved here), each finger closed onto the mesh from there.
    auto closeOn = [&](const float* chosenHold, const float (*chosenFingers)[12], float up, float forward) {
        Result r;
        // Up along the magazine and forward, in the gun's space: its middle, as the user asked.
        float toGun[12];
        Invert(chosenHold, toGun);
        float before[3];
        Apply(toGun, kMiddle, before);
        std::printf("the game's hand's middle on the gun at (%.3f %.3f %.3f)\n", before[0], before[1], before[2]);
        toGun[7] += up, toGun[11] += forward;
        Invert(toGun, r.hold);
        std::memcpy(r.bones, chosenFingers, sizeof(r.bones));
        auto gapsNow = [&](const char* when) {
            std::printf("%s: palm %.1f mm;", when, Gap(toGun, r.bones, g_palmOnly) * 1000.0f);
            for (int f = 0; f < 5; ++f)
                std::printf(" %s %.1f %.1f %.1f;", f == 0 ? "thumb" : f == 1 ? "index" : f == 2 ? "middle" : f == 3 ? "ring" : "little", Gap(toGun, r.bones, g_seg[f][1]) * 1000.0f, Gap(toGun, r.bones, g_seg[f][2]) * 1000.0f, Gap(toGun, r.bones, g_seg[f][3]) * 1000.0f);
            std::printf("\n");
        };
        gapsNow("as the game holds it");
        // Each finger bent onto it: the most it closes with no part further in than a glove gives; a finger already in, opened until it isn't. One that never reaches it stays as it was: a knob's too small for all five.
        float bends[5] = {};
        for (int f = 0; f < 5; ++f) {
            float bestBend = 0.0f, bestLeast = 1e9f;
            bool any = false;
            for (float bend = -40.0f; bend <= 60.01f; bend += 1.0f) {
                float tried[handpose::kBones][12];
                std::memcpy(tried, r.bones, sizeof(tried));
                float all[5] = {};
                all[f] = bend;
                handpose::BendFurther(tried, all);
                const float least = std::min({Gap(toGun, tried, g_seg[f][1]), Gap(toGun, tried, g_seg[f][2]), Gap(toGun, tried, g_seg[f][3])});
                if (least < -kAllowed) {
                    if (any)
                        break;
                    continue;
                }
                bestBend = bend;
                bestLeast = least;
                any = true;
            }
            bends[f] = bestLeast > 0.004f ? 0.0f : bestBend;
        }
        handpose::BendFurther(r.bones, bends);
        std::printf("bent thumb %.0f, index %.0f, middle %.0f, ring %.0f, little %.0f\n", bends[0], bends[1], bends[2], bends[3], bends[4]);
        gapsNow("closed on it");
        for (int f = 0; f < 5; ++f)
            for (int d = 0; d < 3; ++d)
                r.gaps[f][d] = Gap(toGun, r.bones, g_seg[f][d + 1]) * 1000.0f;
        r.palmGap = Gap(toGun, r.bones, g_palmOnly) * 1000.0f;
        r.y = before[1] + up, r.slide = before[2] + forward;
        float middle[3] = {0.0f, midY, midZ};
        Draw(r, argv[2], middle);
        WriteTables(r, argv[7], argv[8]);
        return 0;
    };
    //   ... leverv: the M416's own hand on its charging handle, the index and middle fingers in their V over the T bar (hands.cpp's kOnLever and the fingers with it, the user, 2026-10-31), moved so the bar lies where this gun's does: an M16's or an M4's
    // (the user, 2026-11-05: "that like specific two-finger pull that the M416 does").
    if (argc > 9 && !std::strcmp(argv[9], "leverv")) {
        Mesh rifle;
        if (!LoadGun(dir, rifle)) {
            std::printf("the M416's dump isn't there\n");
            return 1;
        }
        float theirs[2][3] = {{1e9f, 1e9f, 1e9f}, {-1e9f, -1e9f, -1e9f}};
        for (size_t t = 0; t + 2 < rifle.tris.size(); t += 3) {  // (only its draw's triangles: the vertex buffer holds others, posed by garbage)
            if (rifle.part[rifle.tris[t]] != 0 || rifle.part[rifle.tris[t + 1]] != 0 || rifle.part[rifle.tris[t + 2]] != 0)
                continue;
            for (int c = 0; c < 3; ++c)
                for (int k = 0; k < 3; ++k)
                    theirs[0][k] = std::min(theirs[0][k], rifle.pos[rifle.tris[t + c] * 3 + k]), theirs[1][k] = std::max(theirs[1][k], rifle.pos[rifle.tris[t + c] * 3 + k]);
        }
        std::printf("the M416's handle: x %.3f..%.3f, y %.3f..%.3f, z %.3f..%.3f; this one's: x %.3f..%.3f, y %.3f..%.3f, z %.3f..%.3f\n", theirs[0][0], theirs[1][0], theirs[0][1], theirs[1][1], theirs[0][2], theirs[1][2], lo[0], hi[0], lo[1], hi[1], lo[2], hi[2]);
        const float dy = hi[1] - theirs[1][1], dz = lo[2] - theirs[0][2];
        std::printf("moved %.1f mm up, %.1f mm forward\n", dy * 1000.0f, dz * 1000.0f);
        constexpr float kOnLever[12] = {0.445276f, -0.368412f, -0.816090f, 0.358794f, 0.623634f, 0.781615f, -0.012581f, -1.056041f, 0.642503f, -0.503339f, 0.577788f, 0.317216f};
        float toGun[12];
        std::memcpy(toGun, kOnLever, sizeof(toGun));
        toGun[7] += dy, toGun[11] += dz;
        handpose::Fingers on;
        constexpr float kCurl[4] = {0.292f, 0.288f, 0.90f, 0.90f}, kApart[4] = {16.4f, -15.6f, -14.4f, -16.8f};
        for (int i = 0; i < 4; ++i)
            on.curl[i] = kCurl[i], on.apart[i] = kApart[i];
        on.thumb = 0.30f, on.spread = 0.4f;
        Result r;
        handpose::Pose(on, kWidth, r.bones);
        Invert(toGun, r.hold);
        for (int f = 0; f < 5; ++f)
            for (int d = 0; d < 3; ++d)
                r.gaps[f][d] = Gap(toGun, r.bones, g_seg[f][d + 1]) * 1000.0f;
        r.palmGap = Gap(toGun, r.bones, g_palmOnly) * 1000.0f;
        std::printf("palm %.1f; thumb %.1f %.1f %.1f; index %.1f %.1f %.1f; middle %.1f %.1f %.1f; ring %.1f %.1f %.1f; little %.1f %.1f %.1f mm off the handle\n", r.palmGap, r.gaps[0][0], r.gaps[0][1], r.gaps[0][2], r.gaps[1][0], r.gaps[1][1], r.gaps[1][2], r.gaps[2][0], r.gaps[2][1], r.gaps[2][2], r.gaps[3][0], r.gaps[3][1], r.gaps[3][2], r.gaps[4][0], r.gaps[4][1], r.gaps[4][2]);
        LoadDraw(dir, count, frame, {magBone}, g_context, -1, 0, drawStart);
        g_drawSpan = 0.3f;
        float middle[3] = {0.0f, hi[1], lo[2] + 0.02f};
        Draw(r, argv[2], middle);
        WriteTables(r, argv[7], argv[8]);
        return 0;
    }
    //   ... hook <fin's middle x y z>: one finger hooked round a fin (the KH2002's, the user, 2026-11-05: "have the index finger sort of hug around the lever right at the top fin... without clipping... go through the handle as well"): the hand beside the
    // gun, the index through the opening over the receiver, round the front of the fin and back past its far side; the other three in a fist outside, the thumb up by the handle. Searched: the hand's turn and place, then the index joint by joint, on the
    // finger's axis first and then on its whole surface, against the lever and against the rest of the gun.
    //   ... pinch: the hand holding something small between the thumb and the pointer (the M240's belt; the user, 2026-11-05: "grab it with his thumb and pointer"), the other three curled loosely out of the way: no gun needed. Prints the point it holds, in the gloves' bind space.
    if (argc > 9 && !std::strcmp(argv[9], "pinch")) {
        handpose::JointPose p;
        p.finger[0][0] = 40.0f, p.finger[0][1] = 45.0f, p.finger[0][2] = 25.0f;
        p.finger[1][0] = 55.0f, p.finger[1][1] = 70.0f, p.finger[1][2] = 45.0f;
        p.finger[2][0] = 62.0f, p.finger[2][1] = 78.0f, p.finger[2][2] = 50.0f;
        p.finger[3][0] = 68.0f, p.finger[3][1] = 85.0f, p.finger[3][2] = 50.0f;
        auto tipOf = [&](const float (*bones)[12], int f, float* out) {
            for (int b = 0; b < handpose::kBones; ++b)
                if (kFinger[b] == f && kDepth[b] == 3)
                    Apply(bones[b], g_joints[f][3], out);
        };
        float best = 1e9f;
        handpose::JointPose chosen = p;
        for (float out = 0.0f; out <= 70.01f; out += 5.0f)
            for (float spread = -20.0f; spread <= 40.01f; spread += 5.0f)
                for (float base = 0.0f; base <= 40.01f; base += 10.0f)
                    for (float knuckle = 0.0f; knuckle <= 60.01f; knuckle += 10.0f)
                        for (float last = 0.0f; last <= 50.01f; last += 10.0f) {
                            handpose::JointPose q = p;
                            q.thumbOut = out, q.thumbSpread = spread, q.thumbBase = base, q.thumbKnuckle = knuckle, q.thumbLast = last;
                            float bones[handpose::kBones][12], a[3], c[3];
                            handpose::PoseJoints(q, kWidth, bones);
                            tipOf(bones, 0, a);
                            tipOf(bones, 1, c);
                            const float d[3] = {a[0] - c[0], a[1] - c[1], a[2] - c[2]};
                            // The tips about a centimetre apart: what's held is between them.
                            const float score = std::fabs(std::sqrt(Dot(d, d)) - 0.012f) + (knuckle + last) * 0.00002f;
                            if (score < best)
                                best = score, chosen = q;
                        }
        Result r;
        handpose::PoseJoints(chosen, kWidth, r.bones);
        float a[3], c[3];
        tipOf(r.bones, 0, a);
        tipOf(r.bones, 1, c);
        std::printf("thumb out %.0f spread %.0f base %.0f knuckle %.0f last %.0f; the tips %.1f mm apart; the point it holds {%.4ff, %.4ff, %.4ff}\n", chosen.thumbOut, chosen.thumbSpread, chosen.thumbBase, chosen.thumbKnuckle, chosen.thumbLast, std::sqrt((a[0]-c[0])*(a[0]-c[0])+(a[1]-c[1])*(a[1]-c[1])+(a[2]-c[2])*(a[2]-c[2])) * 1000.0f, (a[0]+c[0])/2, (a[1]+c[1])/2, (a[2]+c[2])/2);
        const float none[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
        std::memcpy(r.hold, none, sizeof(none));
        g_magMesh.tris.clear();
        float middle[3] = {(a[0]+c[0])/2, (a[1]+c[1])/2, (a[2]+c[2])/2};
        g_drawSpan = 0.22f;
        Draw(r, argv[2], middle);
        WriteTables(r, argv[7], argv[8]);
        return 0;
    }
    //   ... under <knob's x y z>: a side lever taken from below (the user, 2026-11-05: "if he's doing the underhand... make sure that he's not clipping through the gun and that he's actually holding the lever"): the palm up beside the receiver, the fingers forward and curling up round the front of
    // the knob and over it, the index and middle on it, the other two closed beside them. The same search, about the knob's own axis.
    //   ... <hook|under|over> <x y z> palm <mm>: the same, the palm kept within that of the gun or the knob: each millimetre further costs a place as much
    // as a millimetre of the index off the knob (the M249's from under its knob, the review of 2026-11-18: its palm 23 mm off the gun, where the M240's is 5).
    if (argc > 12 && (!std::strcmp(argv[9], "hook") || !std::strcmp(argv[9], "under") || !std::strcmp(argv[9], "over"))) {
        const bool above = !std::strcmp(argv[9], "over"), under = !std::strcmp(argv[9], "under") || above;  // (over: the same from above, the palm down, the fingers forward and curling down round the front of the knob)
        const float upWay = above ? -1.0f : 1.0f;
        const float fin[3] = {static_cast<float>(std::atof(argv[10])), static_cast<float>(std::atof(argv[11])), static_cast<float>(std::atof(argv[12]))};
        const bool palmKept = argc > 14 && !std::strcmp(argv[13], "palm");
        const float palmMost = palmKept ? static_cast<float>(std::atof(argv[14])) : 0.0f;  // mm
        LoadDraw(dir, count, frame, {magBone}, g_context, -1, 0, drawStart);
        Field gun;
        {
            const float from2[3] = {fin[0] - 0.16f, fin[1] - 0.14f, fin[2] - 0.18f}, to2[3] = {fin[0] + 0.20f, fin[1] + 0.12f, fin[2] + 0.18f};
            gun.solid.Build(g_context, from2, to2, 0.001f, -1);
            gun.distance = DistanceField(gun.solid);
            // A gun's model is hollow, and open in places: what's between its two sides at any height and place along it counts as in it, or a hand can lie inside the receiver with nothing in the way.
            for (int z = 0; z < gun.solid.n[2]; ++z)
                for (int y = 0; y < gun.solid.n[1]; ++y) {
                    int lowest = -1, highest = -1;
                    for (int x = 0; x < gun.solid.n[0]; ++x)
                        if (gun.solid.depth[gun.solid.At(x, y, z)] >= 1) {
                            if (lowest < 0)
                                lowest = x;
                            highest = x;
                        }
                    for (int x = lowest + 1; lowest >= 0 && x < highest; ++x) {
                        float& d = gun.distance[gun.solid.At(x, y, z)];
                        d = std::min(d, -static_cast<float>(std::min(x - lowest, highest - x)) * gun.solid.cell);
                    }
                }
        }
        auto gapGun = [&](const float* toGun, const float (*bones)[12], const std::vector<int>& set) {
            float least = 1.0f;
            for (int i : set) {
                float q[3], g[3];
                PosePoint(bones, i, q);
                Apply(toGun, q, g);
                least = std::min(least, gun.At(g));
            }
            return least;
        };
        auto boneOf = [&](const float (*bones)[12], int f, int depth) -> const float* {
            for (int b = 0; b < handpose::kBones; ++b)
                if (kFinger[b] == f && kDepth[b] == depth)
                    return bones[b];
            return nullptr;
        };
        // The index's thickness, from its surface round its axis in a straight pose.
        float radius[3] = {};
        {
            handpose::JointPose open;
            float bones[handpose::kBones][12];
            handpose::PoseJoints(open, kWidth, bones);
            for (int d = 1; d <= 3; ++d) {
                const float* b = boneOf(bones, 1, d);
                float a[3], c[3];
                Apply(b, g_joints[1][d - 1], a);
                Apply(b, g_joints[1][d], c);
                const float ax[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
                const float ll = Dot(ax, ax);
                float sum = 0.0f;
                int n = 0;
                for (int i : g_seg[1][d]) {
                    float q[3];
                    PosePoint(bones, i, q);
                    const float e[3] = {q[0] - a[0], q[1] - a[1], q[2] - a[2]};
                    const float t = Dot(e, ax) / ll;
                    const float o[3] = {e[0] - ax[0] * t, e[1] - ax[1] * t, e[2] - ax[2] * t};
                    sum += std::sqrt(Dot(o, o));
                    ++n;
                }
                radius[d - 1] = n ? sum / n : 0.008f;
            }
            std::printf("the index's thickness: radius %.1f %.1f %.1f mm; its parts %.1f %.1f %.1f mm long\n", radius[0] * 1000.0f, radius[1] * 1000.0f, radius[2] * 1000.0f,
                        std::sqrt(std::pow(g_joints[1][1][0] - g_joints[1][0][0], 2.0f) + std::pow(g_joints[1][1][1] - g_joints[1][0][1], 2.0f) + std::pow(g_joints[1][1][2] - g_joints[1][0][2], 2.0f)) * 1000.0f,
                        std::sqrt(std::pow(g_joints[1][2][0] - g_joints[1][1][0], 2.0f) + std::pow(g_joints[1][2][1] - g_joints[1][1][1], 2.0f) + std::pow(g_joints[1][2][2] - g_joints[1][1][2], 2.0f)) * 1000.0f,
                        std::sqrt(std::pow(g_joints[1][3][0] - g_joints[1][2][0], 2.0f) + std::pow(g_joints[1][3][1] - g_joints[1][2][1], 2.0f) + std::pow(g_joints[1][3][2] - g_joints[1][2][2], 2.0f)) * 1000.0f);
        }
        struct Try {
            float toGun[12];
            float a[3];
            float score;
        };
        std::vector<Try> tops;
        handpose::JointPose base;
        float baseBones[handpose::kBones][12];
        handpose::PoseJoints(base, kWidth, baseBones);
        float knuckleBind[3];
        Apply(boneOf(baseBones, 1, 1), g_joints[1][0], knuckleBind);
        size_t step = std::max<size_t>(1, g_palmOnly.size() / 60);
        std::vector<int> palmFew;
        for (size_t i = 0; i < g_palmOnly.size(); i += step)
            palmFew.push_back(g_palmOnly[i]);
        long long tried = 0;
        const float yawFrom = under ? -24.0f : 0.0f, yawTo = under ? 24.01f : 70.01f, yawStep = under ? 12.0f : 14.0f;
        const float pitchFrom = under ? -15.0f : -20.0f, pitchTo = under ? 45.01f : 20.01f, pitchStep = under ? 15.0f : 10.0f;
        const float rollFrom = above ? -60.0f : under ? -45.0f : -30.0f, rollTo = above ? 60.01f : 30.01f, rollStep = 15.0f;
        // (Below a knob on the gun's right the index is nearest the gun; on its left, the little finger is, the index out beyond the knob.)
        const float kxFrom = fin[0] + (under ? (fin[0] > 0.0f ? -0.01f : -0.03f) : 0.02f), kxTo = fin[0] + (under ? (fin[0] > 0.0f ? 0.0501f : 0.0201f) : 0.0701f), kxStep = 0.01f;
        const float kyFrom = fin[1] + (above ? 0.0f : under ? -0.07f : -0.012f), kyTo = fin[1] + (above ? 0.0701f : under ? 0.0001f : 0.0121f), kyStep = under ? 0.007f : 0.004f;
        const float kzFrom = fin[2] + (under ? -0.06f : -0.03f), kzTo = fin[2] + (under ? 0.0301f : 0.0501f), kzStep = 0.01f;
        for (float yaw = yawFrom; yaw <= yawTo; yaw += yawStep)
            for (float pitch = pitchFrom; pitch <= pitchTo; pitch += pitchStep)
                for (float roll = rollFrom; roll <= rollTo; roll += rollStep) {
                    const float cy = std::cos(yaw * kDegree), sy = std::sin(yaw * kDegree), cp = std::cos(pitch * kDegree), sp = std::sin(pitch * kDegree);
                    // Over the top, a hook: the fingers across the gun, the palm back. From below: the fingers forward, the palm up.
                    const float along[3] = {under ? -sy * cp : -cy * cp, sp, under ? cy * cp : sy * cp};
                    float palm0[3] = {under ? 0.0f : -sy, under ? upWay : 0.0f, under ? 0.0f : -cy};
                    const float over = Dot(palm0, along);
                    for (int k = 0; k < 3; ++k)
                        palm0[k] -= over * along[k];
                    Normalize(palm0);
                    float side[3];
                    Cross(along, palm0, side);
                    const float cr = std::cos(roll * kDegree), sr = std::sin(roll * kDegree);
                    const float palmWay[3] = {palm0[0] * cr + side[0] * sr, palm0[1] * cr + side[1] * sr, palm0[2] * cr + side[2] * sr};
                    float across[3];
                    Cross(along, palmWay, across);
                    float turn[12] = {};
                    for (int i = 0; i < 3; ++i)
                        for (int j = 0; j < 3; ++j)
                            turn[i * 4 + j] = along[i] * kAlong[j] + palmWay[i] * kPalm[j] + across[i] * kAcross[j];
                    for (float kx = kxFrom; kx <= kxTo; kx += kxStep)
                        for (float ky = kyFrom; ky <= kyTo; ky += kyStep)
                            for (float kz = kzFrom; kz <= kzTo; kz += kzStep) {
                                float toGun[12];
                                std::memcpy(toGun, turn, sizeof(toGun));
                                for (int i = 0; i < 3; ++i)
                                    toGun[i * 4 + 3] = 0.0f;
                                float at[3];
                                Apply(toGun, knuckleBind, at);
                                toGun[3] = kx - at[0], toGun[7] = ky - at[1], toGun[11] = kz - at[2];
                                if (gapGun(toGun, baseBones, palmFew) < -0.002f || Gap(toGun, baseBones, palmFew) < -0.002f)
                                    continue;
                                float best = -1e30f, bestA[3] = {};
                                handpose::JointPose p;
                                for (float a1 = -10.0f; a1 <= 70.01f; a1 += 10.0f)
                                    for (float a2 = 10.0f; a2 <= 100.01f; a2 += 10.0f)
                                        for (float a3 = 0.0f; a3 <= 80.01f; a3 += 10.0f) {
                                            ++tried;
                                            p.finger[0][0] = a1, p.finger[0][1] = a2, p.finger[0][2] = a3;
                                            float bones[handpose::kBones][12];
                                            handpose::PoseJoints(p, kWidth, bones);
                                            float sweep = 0.0f, angleWas = 0.0f, nearest[2] = {1.0f, 1.0f}, away = 0.0f;
                                            bool haveAngle = false, clear = true;
                                            for (int d = 1; d <= 3 && clear; ++d) {
                                                const float* b = boneOf(bones, 1, d);
                                                for (int s = 0; s < 4; ++s) {
                                                    const float t = s / 3.0f;
                                                    const float j[3] = {g_joints[1][d - 1][0] + (g_joints[1][d][0] - g_joints[1][d - 1][0]) * t, g_joints[1][d - 1][1] + (g_joints[1][d][1] - g_joints[1][d - 1][1]) * t, g_joints[1][d - 1][2] + (g_joints[1][d][2] - g_joints[1][d - 1][2]) * t};
                                                    float q[3], g[3];
                                                    Apply(b, j, q);
                                                    Apply(toGun, q, g);
                                                    const float r0 = radius[d - 1];
                                                    const float lever = g_mag.At(g) - r0, rest = gun.At(g) - r0;
                                                    if (lever < -0.003f || rest < -0.003f) {
                                                        clear = false;
                                                        break;
                                                    }
                                                    if (d >= 2) {
                                                        nearest[d - 2] = std::min(nearest[d - 2], lever);
                                                        const float e[3] = {g[0] - fin[0], g[1] - fin[1], g[2] - fin[2]};
                                                        away = std::max(away, std::sqrt(Dot(e, e)));
                                                    }
                                                    // Round the fin, from its near side round its front; round a knob's axis, from under it round its front and over.
                                                    const float angle = under ? std::atan2(g[2] - fin[2], (fin[1] - g[1]) * upWay) : std::atan2(g[2] - fin[2], g[0] - fin[0]);
                                                    if (haveAngle)
                                                        sweep += std::remainder(angle - angleWas, 2.0f * kPi);
                                                    angleWas = angle, haveAngle = true;
                                                }
                                            }
                                            if (!clear)
                                                continue;
                                            const float contact = under ? (std::fabs(nearest[0]) + std::fabs(nearest[1])) * 1000.0f : std::fabs(std::min(nearest[0], nearest[1])) * 1000.0f;
                                            const float wrapped = std::min(sweep / kDegree, under ? 170.0f : 210.0f);
                                            // From below, the last two parts of the finger close round the knob, and the hand as level as it can be: a fist on it, not a hand reaching for it.
                                            const float score = wrapped * 0.12f - contact - std::max(away * 1000.0f - (under ? 30.0f : 48.0f), 0.0f) * (under ? 0.6f : 0.25f) - std::fabs(a2 - a3) * 0.01f - std::fabs(roll) * (under ? 0.04f : 0.02f) - (under ? (std::fabs(yaw) + std::fabs(pitch)) * 0.03f : 0.0f);
                                            if (score > best) {
                                                best = score;
                                                bestA[0] = a1, bestA[1] = a2, bestA[2] = a3;
                                            }
                                        }
                                if (best > -1e29f && palmKept)
                                    best -= std::max(std::min(gapGun(toGun, baseBones, palmFew), Gap(toGun, baseBones, palmFew)) * 1000.0f - palmMost, 0.0f);
                                if (best > -1e29f) {
                                    Try t;
                                    std::memcpy(t.toGun, toGun, sizeof(toGun));
                                    std::copy(bestA, bestA + 3, t.a);
                                    t.score = best;
                                    tops.push_back(t);
                                    std::sort(tops.begin(), tops.end(), [](const Try& a, const Try& b) { return a.score > b.score; });
                                    if (tops.size() > 40)
                                        tops.resize(40);
                                }
                            }
                }
        std::printf("%lld fingers tried, %zu places kept\n", tried, tops.size());
        if (tops.empty())
            return 1;
        // Each kept place once more with the whole surface: the index by small steps, then the fist and the thumb.
        Result bestR;
        float bestToGun[12] = {};
        float bestScore = -1e30f;
        for (const Try& t : tops) {
            handpose::JointPose p;
            float a[3] = {t.a[0], t.a[1], t.a[2]};
            float top = -1e30f;
            for (float d1 = -10.0f; d1 <= 10.01f; d1 += 2.5f)
                for (float d2 = -10.0f; d2 <= 10.01f; d2 += 2.5f)
                    for (float d3 = -10.0f; d3 <= 10.01f; d3 += 2.5f) {
                        p.finger[0][0] = t.a[0] + d1, p.finger[0][1] = t.a[1] + d2, p.finger[0][2] = t.a[2] + d3;
                        if (p.finger[0][1] < 0.0f || p.finger[0][2] < 0.0f)
                            continue;
                        float bones[handpose::kBones][12];
                        handpose::PoseJoints(p, kWidth, bones);
                        float sunk = 0.0f, air = 1e9f;
                        for (int d = 1; d <= 3; ++d) {
                            const float g1 = Gap(t.toGun, bones, g_seg[1][d]) * 1000.0f, g2 = gapGun(t.toGun, bones, g_seg[1][d]) * 1000.0f;
                            sunk += std::max(-g1 - kAllowed * 1000.0f, 0.0f) + std::max(-g2 - kAllowed * 1000.0f, 0.0f);
                            if (d >= 2)
                                air = std::min(air, g1);
                        }
                        const float s = -sunk * 6.0f - std::fabs(air) - std::fabs(p.finger[0][1] - p.finger[0][2]) * 0.01f;
                        if (s > top) {
                            top = s;
                            a[0] = p.finger[0][0], a[1] = p.finger[0][1], a[2] = p.finger[0][2];
                        }
                    }
            p.finger[0][0] = a[0], p.finger[0][1] = a[1], p.finger[0][2] = a[2];
            // The fist: each of the three as little closed as keeps it out of the gun, and no less than a loose fist.
            float sunkFist = 0.0f;
            for (int k = 1; k < 4; ++k) {
                float bestSum = 1e9f;
                float chosen[3] = {80.0f, 95.0f, 65.0f};
                float leastIn = -1e9f;
                float chosenIn[3] = {80.0f, 95.0f, 65.0f};
                // From below, the middle on the knob too: as near the index as keeps it out. The rest a loose fist.
                const bool alongIndex = under && k == 1;
                const float from1 = alongIndex ? a[0] - 20.0f : 35.0f, to1 = alongIndex ? a[0] + 30.01f : 95.01f, from2 = alongIndex ? a[1] - 20.0f : 50.0f, to2 = alongIndex ? a[1] + 30.01f : 105.01f, from3 = alongIndex ? a[2] - 20.0f : 35.0f, to3 = alongIndex ? a[2] + 20.01f : 70.01f;
                for (float a1 = from1; a1 <= to1; a1 += alongIndex ? 5.0f : 10.0f)
                    for (float a2 = from2; a2 <= to2; a2 += alongIndex ? 5.0f : 11.0f)
                        for (float a3 = from3; a3 <= to3; a3 += alongIndex ? 5.0f : 7.0f) {
                            handpose::JointPose q = p;
                            q.finger[k][0] = a1, q.finger[k][1] = a2, q.finger[k][2] = a3;
                            float bones[handpose::kBones][12];
                            handpose::PoseJoints(q, kWidth, bones);
                            float in = 1.0f;
                            for (int d = 1; d <= 3; ++d)
                                in = std::min({in, gapGun(t.toGun, bones, g_seg[k + 1][d]), Gap(t.toGun, bones, g_seg[k + 1][d])});
                            if (in > leastIn)
                                leastIn = in, chosenIn[0] = a1, chosenIn[1] = a2, chosenIn[2] = a3;
                            // (The other two, from below: closed as far as they go, a fist.)
                            const float key = alongIndex ? std::fabs(a1 - a[0]) + std::fabs(a2 - a[1]) + std::fabs(a3 - a[2]) : under ? 1000.0f - (a1 + a2 + a3) : a1 + a2 + a3;
                            if (in >= -kAllowed && key < bestSum)
                                bestSum = key, chosen[0] = a1, chosen[1] = a2, chosen[2] = a3;
                        }
                if (bestSum > 1e8f) {
                    sunkFist += -leastIn * 1000.0f;
                    std::copy(chosenIn, chosenIn + 3, chosen);
                }
                p.finger[k][0] = chosen[0], p.finger[k][1] = chosen[1], p.finger[k][2] = chosen[2];
                p.fingerApart[k] = 0.0f;
            }
            // The thumb: up along the index, out of the gun.
            float thumbBest = -1e30f;
            handpose::JointPose bestThumb = p;
            for (float out = 0.0f; out <= 50.01f; out += 10.0f)
                for (float spread = -10.0f; spread <= 30.01f; spread += 10.0f)
                    for (float knuckle = 5.0f; knuckle <= 45.01f; knuckle += 10.0f)
                        for (float last = 5.0f; last <= 45.01f; last += 10.0f) {
                            handpose::JointPose q = p;
                            q.thumbOut = out, q.thumbSpread = spread, q.thumbKnuckle = knuckle, q.thumbLast = last;
                            float bones[handpose::kBones][12];
                            handpose::PoseJoints(q, kWidth, bones);
                            float in = 1.0f;
                            for (int d = 1; d <= 3; ++d)
                                in = std::min({in, gapGun(t.toGun, bones, g_seg[0][d]), Gap(t.toGun, bones, g_seg[0][d])});
                            const float s = (in < -kAllowed ? in * 1000.0f * 10.0f : 0.0f) - std::fabs(in * 1000.0f - 2.0f) * 0.2f - out * 0.02f - std::fabs(spread) * 0.01f;
                            if (s > thumbBest)
                                thumbBest = s, bestThumb = q;
                        }
            p = bestThumb;
            Result r;
            handpose::PoseJoints(p, kWidth, r.bones);
            float in = 1.0f, sunk = 0.0f;
            for (int f = 0; f < 5; ++f)
                for (int d = 1; d <= 3; ++d) {
                    const float g1 = Gap(t.toGun, r.bones, g_seg[f][d]), g2 = gapGun(t.toGun, r.bones, g_seg[f][d]);
                    r.gaps[f][d - 1] = std::min(g1, g2) * 1000.0f;
                    in = std::min(in, std::min(g1, g2));
                    sunk += std::max(-std::min(g1, g2) * 1000.0f - kAllowed * 1000.0f, 0.0f);
                }
            const float palmIn = std::min(Gap(t.toGun, r.bones, g_palmOnly), gapGun(t.toGun, r.bones, g_palmOnly));
            r.palmGap = palmIn * 1000.0f;
            sunk += std::max(-palmIn * 1000.0f - kAllowed * 1000.0f, 0.0f);
            const float score = t.score + top * 0.5f - sunk * 3.0f;
            std::printf("place score %.1f: the index %.0f %.0f %.0f: %.1f %.1f %.1f mm; sunk %.1f mm; the score %.1f\n", t.score, a[0], a[1], a[2], r.gaps[1][0], r.gaps[1][1], r.gaps[1][2], sunk, score);
            if (score > bestScore) {
                bestScore = score;
                bestR = r;
                std::memcpy(bestToGun, t.toGun, sizeof(bestToGun));
            }
        }
        Invert(bestToGun, bestR.hold);
        float m[3], knuckle[3];
        Apply(bestToGun, kMiddle, m);
        Apply(bestToGun, knuckleBind, knuckle);
        std::printf("the hand's middle at (%.3f %.3f %.3f), the index's knuckle at (%.3f %.3f %.3f); palm %.1f; thumb %.1f %.1f %.1f; index %.1f %.1f %.1f; middle %.1f %.1f %.1f; ring %.1f %.1f %.1f; little %.1f %.1f %.1f mm off the gun\n", m[0], m[1], m[2], knuckle[0], knuckle[1], knuckle[2], bestR.palmGap, bestR.gaps[0][0], bestR.gaps[0][1], bestR.gaps[0][2], bestR.gaps[1][0], bestR.gaps[1][1], bestR.gaps[1][2], bestR.gaps[2][0], bestR.gaps[2][1], bestR.gaps[2][2], bestR.gaps[3][0], bestR.gaps[3][1], bestR.gaps[3][2], bestR.gaps[4][0], bestR.gaps[4][1], bestR.gaps[4][2]);
        g_drawSpan = 0.26f;
        float middle[3] = {fin[0] + 0.03f, fin[1], fin[2]};
        Draw(bestR, argv[2], middle);
        WriteTables(bestR, argv[7], argv[8]);
        return 0;
    }
    //   ... leverfrom <that fit's out.h>.start <dx> <dy> <dz>: another gun's hand on its lever, moved by this to this one's
    if (argc > 13 && !std::strcmp(argv[9], "leverfrom")) {
        FILE* f = nullptr;
        if (fopen_s(&f, argv[10], "rb") || !f) {
            std::printf("can't read %s\n", argv[10]);
            return 1;
        }
        float their[3], hold[12], fingers[handpose::kBones][12];
        const bool whole = fread(their, 4, 3, f) == 3 && fread(hold, sizeof(hold), 1, f) == 1 && fread(fingers, sizeof(fingers), 1, f) == 1;
        fclose(f);
        if (!whole)
            return 1;
        float toGun[12], moved[12];
        Invert(hold, toGun);
        toGun[3] += static_cast<float>(std::atof(argv[11])), toGun[7] += static_cast<float>(std::atof(argv[12])), toGun[11] += static_cast<float>(std::atof(argv[13]));
        Invert(toGun, moved);
        return closeOn(moved, fingers, 0.0f, 0.0f);
    }
    //   ... soft <the game's own hold, a fit's out.h>.start <give mm> <palm mm> [shape <the gun's shape .bin>]: the game's own hand on a soft thing, as its
    // own reload holds it (the M249's cloth pouch; the review of 2026-11-18: "the hand on the box hovers", its palm 25 mm off the pouch's end, the middle
    // fingertip 29 mm, where the M240's box hold has its palm at 7 and every finger within 5.5). The hand moved about the game's place (up to 3.5 cm in or
    // 1 cm out along the gun's x, 2 cm up or down, 1.5 cm along) and its fingers closed on it from the game's own pose, the cloth giving: a part of a
    // finger may press in up to <give> mm, never further (never through it), the palm up to half that; the place where the palm is within <palm> mm of it
    // with the least air under the fingers, the least moved from the game's own place (a centimetre costs as much as a millimetre of a fingertip's air),
    // the thumb kept near the game's own. Each finger first bent as a whole at each place, then joint by joint at the best. With the gun's shape
    // (build\guns\<gun>.bin, what the hands collide with in the game): no part of the hand, the palm's and every finger's every vertex, further into it
    // than a glove gives, 2.5 mm (the review of 2026-11-18: the M249's thumb, closed round the pouch's back, went up into the receiver's left wall, 7.9 mm
    // into its shape, where the game's own lies 7.5 mm clear of it). A magazine drawn apart has its gun's own draw behind the hand in the picture.
    if (argc > 12 && !std::strcmp(argv[9], "soft")) {
        FILE* f = nullptr;
        if (fopen_s(&f, argv[10], "rb") || !f) {
            std::printf("can't read %s\n", argv[10]);
            return 1;
        }
        float their[3], hold[12], fingers[handpose::kBones][12];
        const bool whole = fread(their, 4, 3, f) == 3 && fread(hold, sizeof(hold), 1, f) == 1 && fread(fingers, sizeof(fingers), 1, f) == 1;
        fclose(f);
        if (!whole)
            return 1;
        const float give = static_cast<float>(std::atof(argv[11])), palmMost = static_cast<float>(std::atof(argv[12]));  // mm
        constexpr float kPress = 3.0f;  // mm a part presses the cloth in for nothing; deeper costs a little, deeper than give isn't taken
        float start[12];
        Invert(hold, start);
        float startMiddle[3];
        Apply(start, kMiddle, startMiddle);
        std::printf("the game's own hand's middle on the gun at (%.4f %.4f %.4f): palm %.1f mm off\n", startMiddle[0], startMiddle[1], startMiddle[2],
                    Gap(start, fingers, g_palmOnly) * 1000.0f);
        constexpr float kGunGive = 2.5f;  // mm
        // The thumb kept near the game's own (the review of 2026-11-18: its thumb lies down the pouch's back; fitted freely it was turned up along the pouch's
        // top into the receiver): each degree it's bent from the game's own pose costs this many times what a finger's does.
        constexpr float kThumbKeep = 20.0f;
        const bool haveGun = argc > 14 && !std::strcmp(argv[13], "shape");
        if (haveGun && !gunshape::Load(std::wstring(argv[14], argv[14] + std::strlen(argv[14])).c_str())) {
            std::fprintf(stderr, "can't read the gun's shape %s\n", argv[14]);
            return 1;
        }
        if (apart) {  // (the gun behind the hand in the picture)
            int gunCount = 0, gunStart = 0;
            ParseDraw(std::strchr(argv[4], '@') + 1, gunCount, gunStart);
            LoadDraw(dir, gunCount, frame, {}, g_context, -1, 0, gunStart);
        }
        // The deepest of these vertices into the gun's shape (mm, negative in it; off its grid, clear).
        auto gunGap = [&](const float* toGun, const float (*bones)[12], const std::vector<int>& set) {
            if (!haveGun)
                return 1000.0f;
            float least = 1.0f;
            for (int i : set) {
                float q[3], g[3], d = 0.05f;
                PosePoint(bones, i, q);
                Apply(toGun, q, g);
                least = std::min(least, gunshape::Distance(g, d) ? d : 0.05f);
            }
            return least * 1000.0f;
        };
        if (haveGun)
            std::printf("the game's own hand against the gun's shape: palm %.1f mm; thumb %.1f, index %.1f, middle %.1f, ring %.1f, little %.1f\n",
                        gunGap(start, fingers, g_part[0]), gunGap(start, fingers, g_part[1]), gunGap(start, fingers, g_part[2]), gunGap(start, fingers, g_part[3]),
                        gunGap(start, fingers, g_part[4]), gunGap(start, fingers, g_part[5]));
        auto segGaps = [&](const float* toGun, const float (*bones)[12], int fi, float* g) {
            for (int d = 0; d < 3; ++d)
                g[d] = Gap(toGun, bones, g_seg[fi][d + 1]) * 1000.0f;
        };
        // A finger's worth at these gaps (mm): none past give, none of it into the gun past its give; the least air, the tip's most; a little for pressing
        // in past kPress.
        auto fingerScore = [&](const float* g, float inGun, float bentBy) {
            if (inGun < -kGunGive)
                return -1e30f;
            float score = 0.0f;
            for (int d = 0; d < 3; ++d) {
                if (g[d] < -give)
                    return -1e30f;
                score -= std::max(g[d], 0.0f) * (d == 0 ? 0.6f : d == 1 ? 1.0f : 1.2f) + std::max(-g[d] - kPress, 0.0f) * 0.3f;
            }
            return score - 0.01f * bentBy;
        };
        // One finger bent as a whole (BendFurther), the best bend's worth; or joint by joint (BendFingerJoints), its angles.
        auto wholeFinger = [&](const float* toGun, int fi, float& bestBend) {
            float best = -1e30f;
            for (float bend = -40.0f; bend <= 60.01f; bend += 2.0f) {
                float tried[handpose::kBones][12];
                std::memcpy(tried, fingers, sizeof(tried));
                float all[5] = {};
                all[fi] = bend;
                handpose::BendFurther(tried, all);
                float g[3];
                segGaps(toGun, tried, fi, g);
                const float s = fingerScore(g, gunGap(toGun, tried, g_part[fi + 1]), std::fabs(bend) * (fi == 0 ? kThumbKeep : 1.0f));
                if (s > best)
                    best = s, bestBend = bend;
            }
            return best;
        };
        auto placeAt = [&](float dx, float dy, float dz, float* toGun) {
            std::memcpy(toGun, start, sizeof(start));
            toGun[3] += dx, toGun[7] += dy, toGun[11] += dz;
        };
        auto placeScore = [&](const float* toGun, float moved, float& palm, float* bends) {
            palm = Gap(toGun, fingers, g_palmOnly) * 1000.0f;
            if (palm < -give * 0.5f || gunGap(toGun, fingers, g_part[0]) < -kGunGive)
                return -1e30f;
            float score = -3.0f * std::max(palm - palmMost, 0.0f) - 100.0f * moved;  // (moved in m: a centimetre, 1)
            for (int fi = 0; fi < 5; ++fi) {
                const float s = wholeFinger(toGun, fi, bends[fi]);
                if (s < -1e29f)
                    return -1e30f;
                score += s * (fi == 0 ? 0.5f : 1.0f);
            }
            return score;
        };
        struct Place {
            float d[3];
            float score;
        };
        std::vector<Place> places;
        int tried = 0, palmIn = 0;
        for (float dx = -0.035f; dx <= 0.01001f; dx += 0.0025f)
            for (float dy = -0.02f; dy <= 0.02001f; dy += 0.005f)
                for (float dz = -0.015f; dz <= 0.01501f; dz += 0.005f) {
                    float toGun[12], palm, bends[5];
                    placeAt(dx, dy, dz, toGun);
                    const float s = placeScore(toGun, std::sqrt(dx * dx + dy * dy + dz * dz), palm, bends);
                    ++tried;
                    if (s < -1e29f) {
                        ++palmIn;
                        continue;
                    }
                    places.push_back({{dx, dy, dz}, s});
                }
        std::sort(places.begin(), places.end(), [](const Place& a, const Place& b) { return a.score > b.score; });
        std::printf("%d places tried, %zu kept (%d with the palm or a finger further in than it gives)\n", tried, places.size(), palmIn);
        if (places.empty())
            return 1;
        // The best few once more, each finger joint by joint.
        Result best;
        float bestScore = -1e30f, bestD[3] = {};
        for (size_t p = 0; p < places.size() && p < 6; ++p) {
            float toGun[12];
            const float* d = places[p].d;
            placeAt(d[0], d[1], d[2], toGun);
            Result r;
            std::memcpy(r.bones, fingers, sizeof(r.bones));
            float score = -100.0f * std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
            float bentBy[5][3] = {};
            bool ok = true;
            for (int fi = 0; fi < 5 && ok; ++fi) {
                float bent[3], low[3];
                JointBends(fingers, fi, bent);
                for (int j = 0; j < 3; ++j)
                    low[j] = fi == 0 ? (j == 0 ? -40.0f : kThumbOpens) : std::max(-40.0f, -bent[j] - 2.0f);
                float top = -1e30f, chosen[3] = {};
                for (float a1 = low[0]; a1 <= 50.0f; a1 += 4.0f)
                    for (float a2 = low[1]; a2 <= 60.0f; a2 += 4.0f)
                        for (float a3 = low[2]; a3 <= 60.0f; a3 += 4.0f) {
                            const float a[3] = {a1, a2, a3};
                            float tried2[handpose::kBones][12];
                            std::memcpy(tried2, fingers, sizeof(tried2));
                            handpose::BendFingerJoints(tried2, fi, a);
                            float g[3];
                            segGaps(toGun, tried2, fi, g);
                            const float s = fingerScore(g, gunGap(toGun, tried2, g_part[fi + 1]), 0.0f) -
                                            0.01f * (std::fabs(a1) + std::fabs(a2) + std::fabs(a3)) * (fi == 0 ? kThumbKeep : 1.0f) - 0.03f * std::fabs(a2 - a3);
                            if (s > top)
                                top = s, chosen[0] = a1, chosen[1] = a2, chosen[2] = a3;
                        }
                if (top < -1e29f) {
                    ok = false;
                    break;
                }
                handpose::BendFingerJoints(r.bones, fi, chosen);
                score += top * (fi == 0 ? 0.5f : 1.0f);
                std::memcpy(bentBy[fi], chosen, sizeof(chosen));
            }
            r.palmGap = Gap(toGun, r.bones, g_palmOnly) * 1000.0f;
            if (!ok || r.palmGap < -give * 0.5f || gunGap(toGun, r.bones, g_part[0]) < -kGunGive)
                continue;
            score -= 3.0f * std::max(r.palmGap - palmMost, 0.0f);
            std::printf("place (%+.4f %+.4f %+.4f): whole fingers %.1f, joint by joint %.1f; each finger's joints bent from the game's own (degrees): thumb %.0f %.0f %.0f, "
                        "index %.0f %.0f %.0f, middle %.0f %.0f %.0f, ring %.0f %.0f %.0f, little %.0f %.0f %.0f\n",
                        d[0], d[1], d[2], places[p].score, score, bentBy[0][0], bentBy[0][1], bentBy[0][2], bentBy[1][0], bentBy[1][1], bentBy[1][2], bentBy[2][0], bentBy[2][1],
                        bentBy[2][2], bentBy[3][0], bentBy[3][1], bentBy[3][2], bentBy[4][0], bentBy[4][1], bentBy[4][2]);
            if (score > bestScore) {
                bestScore = score;
                best = r;
                std::memcpy(bestD, d, sizeof(bestD));
                Invert(toGun, best.hold);
            }
        }
        if (bestScore < -1e29f) {
            std::printf("nowhere fitted joint by joint\n");
            return 1;
        }
        float toGun[12], m[3];
        Invert(best.hold, toGun);
        for (int fi = 0; fi < 5; ++fi)
            segGaps(toGun, best.bones, fi, best.gaps[fi]);
        Apply(toGun, kMiddle, m);
        std::printf("moved (%+.4f %+.4f %+.4f) from the game's own; the hand's middle at (%.4f %.4f %.4f); palm %.1f mm;", bestD[0], bestD[1], bestD[2], m[0], m[1], m[2],
                    best.palmGap);
        for (int fi = 0; fi < 5; ++fi)
            std::printf(" %s %.1f %.1f %.1f;", fi == 0 ? "thumb" : fi == 1 ? "index" : fi == 2 ? "middle" : fi == 3 ? "ring" : "little", best.gaps[fi][0], best.gaps[fi][1],
                        best.gaps[fi][2]);
        std::printf("\n");
        if (haveGun)
            std::printf("against the gun's shape: palm %.1f mm; thumb %.1f, index %.1f, middle %.1f, ring %.1f, little %.1f\n", gunGap(toGun, best.bones, g_part[0]),
                        gunGap(toGun, best.bones, g_part[1]), gunGap(toGun, best.bones, g_part[2]), gunGap(toGun, best.bones, g_part[3]), gunGap(toGun, best.bones, g_part[4]),
                        gunGap(toGun, best.bones, g_part[5]));
        best.y = m[1], best.slide = m[2];
        float middle[3] = {0.0f, midY, midZ};
        Draw(best, argv[2], middle);
        WriteTables(best, argv[7], argv[8]);
        return 0;
    }
    const bool gripMode = argc > 12 && !std::strcmp(argv[9], "grip");
    //   ... vanillaright <its allrec> <from ms> <to ms> [up, m] [forward, m]: as vanilla, from a reload the game does with its RIGHT hand (the PKP's: its
    // left stays on the handguard throughout, its right draws the handle, lifts the cover, takes the box off, lays the belt): that hand mirrored across
    // the gun's x = 0 plane onto the left glove (handpose::RightToLeft: the gloves' bind spaces are each other's mirror image), on the gun's frame, as the
    // free left hand works it in VR; then each finger closed onto this gun's own mesh where the mirrored hand lies. (A part drawn apart isn't taken.)
    const bool mirrorRight = argc > 12 && !std::strcmp(argv[9], "vanillaright");
    if (mirrorRight && apart) {
        std::printf("vanillaright takes a part of the gun's own draw (a bone of it), not one drawn apart\n");
        return 1;
    }
    if (argc > 12 && (!std::strcmp(argv[9], "vanilla") || gripMode || mirrorRight)) {
        const float up = !gripMode && argc > 13 ? static_cast<float>(std::atof(argv[13])) : 0.0f, forward = !gripMode && argc > 14 ? static_cast<float>(std::atof(argv[14])) : 0.0f;
        const int from2 = std::atoi(argv[11]), to2 = std::atoi(argv[12]);
        // The magazine seated, on the frame: from the dump's bones.
        auto dumpBones = ReadAll(dir + "\\mesh_" + std::to_string(count) + "_" + std::to_string(drawStart) + "_bones.bin");
        const float* db = reinterpret_cast<const float*>(dumpBones.data());
        float invFrame[12], seat[12];
        if (apart) {  // (a piece drawn apart: its seat on the gun's frame, as its mesh is)
            std::memcpy(seat, partOnFrame, sizeof(seat));
        } else {
            Invert(db + frame * 12, invFrame);
            Compose(invFrame, db + magBone * 12, seat);
        }
        // The fresh magazine in the reload may be a copy on a bone of its own, parked till then: its seat is where it is at this moment of the recording, home.
        // Or, for a magazine drawn apart that the recording never drew (the Saiga 12K's, its socket's mesh not made by the swap that recorded it):
        //   ... grip <its allrec> <from ms> <to ms> seated
        // the stretch is one with the game's hand on it still seated, and the magazine is taken as seated on the gun's own frame in each of its frames.
        const bool seated = gripMode && apart && argc > 13 && !std::strcmp(argv[13], "seated");
        const int recBone = gripMode && argc > 13 && !seated ? std::atoi(argv[13]) : magBone;
        const int seatMs = gripMode && argc > 14 && !seated ? std::atoi(argv[14]) : -1;
        int gunCount = 0, gunStart = 0;
        if (apart)
            ParseDraw(std::strchr(argv[4], '@') + 1, gunCount, gunStart);
        // The recording's frames in that time, with the gloves'.
        // (Each record's frame and its place among that frame's records of its draw: the n-th body and the n-th gloves of a frame are the same eye's,
        // which vanillaright's frame-space mirror needs; vanilla pairs by the nearest moment, as before.)
        struct Palette {
            int ms;
            std::vector<float> bones;
            int serial = 0, nth = 0;
        };
        std::vector<Palette> mags, gloves;
        {
            FILE* f = nullptr;
            if (fopen_s(&f, argv[10], "rb") || !f) {
                std::printf("can't read %s\n", argv[10]);
                return 1;
            }
            int head[8];
            std::vector<float> buffer;
            int serialNow = -1, magNth = 0, gloveNth = 0;
            while (fread(head, 4, 8, f) == 8) {
                const int kind = head[0], pass = head[3], ms = head[6], n = head[7];
                if (n <= 0 || n > 64)
                    break;
                buffer.resize(static_cast<size_t>(n) * 12);
                if (fread(buffer.data(), 48, n, f) != static_cast<size_t>(n))
                    break;
                if (head[1] != serialNow)
                    serialNow = head[1], magNth = gloveNth = 0;
                if (seatMs >= 0 && !apart && kind == count && pass == drawStart && ms >= seatMs && ms < seatMs + 60) {
                    float onFrame[12];
                    Invert(&buffer[frame * 12], onFrame);
                    Compose(onFrame, &buffer[recBone * 12], seat);
                }
                const bool isMag = kind == count && pass == drawStart, isGlove = kind == 13755 && pass == 15051;
                const int nth = isMag ? magNth++ : isGlove ? gloveNth++ : 0;
                if (ms < from2 || ms > to2)
                    continue;
                if (seated && kind == gunCount && pass == gunStart && buffer.size() >= static_cast<size_t>(frame + 1) * 12) {
                    // Seated: the magazine's own bone where it rests on this frame of the gun's (its frame bone composed with the seat). (Never
                    // vanillaright's, which takes no part drawn apart: no pairing by place asked of it.)
                    std::vector<float> own(static_cast<size_t>(std::max(recBone + 1, 1)) * 12, 0.0f);
                    Compose(&buffer[frame * 12], partOnFrame, &own[recBone * 12]);
                    mags.push_back({ms, own, serialNow, 0});
                } else if (!seated && isMag) {
                    mags.push_back({ms, buffer, serialNow, nth});
                } else if (isGlove) {
                    gloves.push_back({ms, buffer, serialNow, nth});
                }
            }
            fclose(f);
        }
        std::printf("%zu frames of it, %zu of the gloves, %d to %d ms\n", mags.size(), gloves.size(), from2, to2);
        if (mags.empty() || gloves.empty())
            return 1;
        // Each frame: the hand on the magazine; the steadiest stretch is it held.
        struct Held {
            int ms;
            float hold[12];
            float fingers[handpose::kBones][12];
        };
        std::vector<Held> helds;
        float restInverse[12];
        Invert(seat, restInverse);
        for (const Palette& m : mags) {
            const Palette* g = &gloves[0];
            for (const Palette& c : gloves)
                if (std::abs(c.ms - m.ms) < std::abs(g->ms - m.ms))
                    g = &c;
            Held h;
            h.ms = m.ms;
            float magPose[12], inverseHand[12];
            Compose(&m.bones[recBone * 12], restInverse, magPose);
            if (mirrorRight) {
                // On the gun's frame: the magazine's move from its seat and the right glove's bones (23-44), its palm 32 (the left's 1's twin); the
                // right hand's hold and bones about its palm, then mirrored onto the left glove. The gloves the same eye's as the body (its frame).
                g = nullptr;
                for (const Palette& c : gloves)
                    if (c.serial == m.serial && c.nth == m.nth)
                        g = &c;
                if (!g || g->bones.size() < static_cast<size_t>(handpose::kRightFirst + handpose::kRightBones) * 12)
                    continue;
                float frameInverse[12], magOnFrame[12], right[handpose::kRightBones][12], rightHold[12], palmInverse[12], rightFingers[handpose::kRightBones][12];
                Invert(&m.bones[frame * 12], frameInverse);
                Compose(frameInverse, magPose, magOnFrame);
                for (int r = 0; r < handpose::kRightBones; ++r)
                    Compose(frameInverse, &g->bones[(handpose::kRightFirst + r) * 12], right[r]);
                Invert(right[handpose::kRightOfLeft[1] - handpose::kRightFirst], palmInverse);
                Compose(palmInverse, magOnFrame, rightHold);
                for (int r = 0; r < handpose::kRightBones; ++r)
                    Compose(palmInverse, right[r], rightFingers[r]);
                handpose::Mirror(rightHold, h.hold);
                handpose::RightToLeft(rightFingers, h.fingers);
                helds.push_back(h);
                continue;
            }
            Invert(&g->bones[1 * 12], inverseHand);
            Compose(inverseHand, magPose, h.hold);
            for (int b = 0; b < handpose::kBones; ++b)
                Compose(inverseHand, &g->bones[b * 12], h.fingers[b]);
            helds.push_back(h);
        }
        size_t bestI = 0;
        float bestD = 1e9f;
        for (size_t i = 3; i + 3 < helds.size(); ++i) {
            float d = 0.0f;
            for (int k = 0; k < 12; ++k)
                d += std::fabs(helds[i - 3].hold[k] - helds[i + 3].hold[k]);
            if (d < bestD)
                bestD = d, bestI = i;
        }
        Held chosen = helds[bestI];
        std::printf("steadiest at %d ms (%.4f)\n", chosen.ms, bestD);
        if (gripMode) {
            // Kept, for a gun whose own reload doesn't hold its magazine this way (gripfrom).
            if (FILE* f = nullptr; !fopen_s(&f, (std::string(argv[8]) + ".start").c_str(), "wb") && f) {
                fwrite(&midY, 4, 1, f), fwrite(&midZ, 4, 1, f), fwrite(&slope, 4, 1, f);
                fwrite(chosen.hold, sizeof(chosen.hold), 1, f), fwrite(chosen.fingers, sizeof(chosen.fingers), 1, f);
                fclose(f);
            }
            Grip(chosen.hold, chosen.fingers, wellY, lo, midY, midZ, argv[2], argv[7], argv[8]);
            return 0;
        }
        // Kept too: a gun whose own reload never has this hand on the lever takes another's (leverfrom).
        if (FILE* f = nullptr; !fopen_s(&f, (std::string(argv[8]) + ".start").c_str(), "wb") && f) {
            fwrite(&midY, 4, 1, f), fwrite(&midZ, 4, 1, f), fwrite(&slope, 4, 1, f);
            fwrite(chosen.hold, sizeof(chosen.hold), 1, f), fwrite(chosen.fingers, sizeof(chosen.fingers), 1, f);
            fclose(f);
        }
        return closeOn(chosen.hold, chosen.fingers, up, forward);
    }
    Result best;
    if (argc > 13 && !std::strcmp(argv[9], "at")) {  // one place only, saying why: at <dy> <tilt from its lean> <yaw> <slide from its middle>
        g_why = true;
        Result r;
        const bool ok = FitAt(midY + static_cast<float>(std::atof(argv[10])), slope + static_cast<float>(std::atof(argv[11])), static_cast<float>(std::atof(argv[12])), midZ + static_cast<float>(std::atof(argv[13])), r);
        std::printf("%s\n", ok ? "fits" : "doesn't");
        if (!ok)
            return 1;
        best = r;
    } else
        for (float dy = -0.015f; dy <= 0.0151f; dy += 0.0075f)
            for (float dtilt = -8.0f; dtilt <= 8.01f; dtilt += 4.0f)
                for (float yaw = -16.0f; yaw <= 16.01f; yaw += 8.0f)
                    for (float dz = -0.02f; dz <= 0.0601f; dz += 0.01f) {
                        Result r;
                        if (FitAt(midY + dy, slope + dtilt, yaw, midZ + dz, r)) {
                            r.score -= std::fabs(dy) * 60.0f;
                            if (r.score > best.score)
                                best = r;
                        }
                    }
    if (best.score < -1e29f) {
        std::printf("nothing fitted\n");
        return 1;
    }
    Print(best);
    float mid[3] = {0.0f, midY, midZ};
    Draw(best, argv[2], mid);
    WriteTables(best, argv[7], argv[8]);
    return 0;
}

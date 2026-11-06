// The right glove as the left's mirror image (src/handpose.cpp's mirror kit, handpose.h), without the
// game: the twin bones, the two tables, the mirror, the right glove's poses and the points its bones
// turn about. Every right pose is the left's own approved one flipped, never fitted again (the user,
// 2026-11-05), so what is checked here is the flip itself. Matrices: three rows, rotation | translation.
//   1 twins       every left bone's twin the same finger with as many joints from the palm (the
//                 wrist's and the palm's bones no finger's on either glove); the left's own table read
//                 back from handpose.cpp through Pivot
//   2 tables      each other's inverse, every right bone the twin of one left bone (the left's 5, the
//                 forearm's own bone, excepted: it takes the cuff's 44)
//   3 mirror      S m S (elements 1, 2, 3, 4 and 8 negated); twice the move itself and in place the
//                 same, bit for bit; the turn's determinant kept; a mirrored point moved where the
//                 mirrored move puts it; a product's mirror the product of the mirrors
//   4 round trip  RightToLeft(LeftToRight(L)) = L (its 5: its 4) and LeftToRight(RightToLeft(R)) = R,
//                 bit for bit; each bone its twin's mirror; the two tables in one buffer the same
//   5 poses       PoseRight relaxed, pointing, a fist and half closed at the game's own proportions
//                 (width 1, slim 1): the left's Pose flipped bit for bit, finite, each bone exactly as
//                 orthonormal as its left twin and within 5e-4 (the left's folded thumb is 1.5e-4 off:
//                 handpose.cpp's kAlong is 0.99978 long), determinant 1; widened as the left's (1.15,
//                 slim 0.88): finite, never inside out; numbers not finite given: each taken as
//                 relaxed, the hand finite
//   6 curl        a right finger posed closer moves its tip the way the game's own rig bends it
//                 (BendRight, within 30 degrees); a finger bent in the left's layout and mirrored goes
//                 the rig's own way (within 20 degrees: the left's axes are 10 off the rig's)
//   7 pivots      PivotRight its twin's Pivot with x negated (another bone: the right hand's middle);
//                 each within 10 mm of the right glove's own joint (RightJoints, from the game's grip)
//   8 mesh        with the gloves' mesh dump in the build folder (mesh_13755_15051_vb0.bin, _ib.bin):
//                 each finger bone's twin's vertices' centre within 10 mm of its left twin's mirrored;
//                 each left bone's twin the right bone sharing the most of its weight at the same
//                 place, mirrored (the cuff's 4 aside: the right glove has next to no weight on its 44,
//                 so skin.cpp's kGlovesForearm decides; the wrist's and palm's centres lie 12-18 mm
//                 apart, the left glove's cuff reaching further up its forearm); skipped without it
//   9 time        PoseRight under 20 microseconds
// Usage: twintest [build folder]   (where the mesh dump is; the folder twintest.exe is in otherwise)
// The exit code is the number of failed checks.

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <random>
#include <string>
#include <vector>

#include "../src/handpose.h"
#include "meshdump.h"

namespace {

using handpose::kBones;
using handpose::kRightBones;
using handpose::kRightFirst;

constexpr float kPi = 3.14159265f, kDegree = kPi / 180.0f;
constexpr int kGloveBones = kRightFirst + kRightBones;  // the gloves' draw's bones, both hands (0-44)
const char* const kFingerName[5] = {"thumb", "index", "middle", "ring", "little"};

// The left glove's bones: each one's finger (0 the thumb ... 4 the little finger; -1 the wrist and
// palm) and how many joints lie between it and the palm, as handpose.cpp has them (read back from it
// through Pivot in 1; a finger's own bone in the palm, its depth 0, only by the mesh in 8).
constexpr int8_t kLeftFinger[kBones] = {-1, -1, 0, 4, -1, -1, 2, 2, 1, 1, 3, 3, 3, 4, 4, 1, 4, 0, 0, 1, 3, 2, 2};
constexpr int8_t kLeftDepth[kBones] = {0, 0, 3, 3, 0, 0, 0, 1, 0, 1, 3, 1, 0, 1, 0, 3, 2, 2, 1, 2, 2, 2, 3};

int g_failed = 0, g_checks = 0;

void Check(bool ok, const char* format, ...) {
    ++g_checks;
    if (!ok)
        ++g_failed;
    char text[600];
    va_list args;
    va_start(args, format);
    std::vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    std::printf("%s %s\n", ok ? "ok  " : "FAIL", text);
}

// ---- Matrices: three rows, rotation | translation ----
void Identity(float* m) {
    const float i[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    std::memcpy(m, i, sizeof(i));
}

void Compose(const float* a, const float* b, float* out) {
    float r[12];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j)
            r[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        r[i * 4 + 3] += a[i * 4 + 3];
    }
    std::memcpy(out, r, sizeof(r));
}

void Apply(const float* m, const float* p, float* out) {
    for (int i = 0; i < 3; ++i)
        out[i] = m[i * 4] * p[0] + m[i * 4 + 1] * p[1] + m[i * 4 + 2] * p[2] + m[i * 4 + 3];
}

float Det(const float* m) {
    return m[0] * (m[5] * m[10] - m[6] * m[9]) - m[1] * (m[4] * m[10] - m[6] * m[8]) + m[2] * (m[4] * m[9] - m[5] * m[8]);
}

// How far a turn is from orthonormal: the largest element of R Rt - I.
float OffOrthonormal(const float* m) {
    float worst = 0.0f;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            const float dot = m[i * 4] * m[j * 4] + m[i * 4 + 1] * m[j * 4 + 1] + m[i * 4 + 2] * m[j * 4 + 2];
            worst = std::max(worst, std::fabs(dot - (i == j ? 1.0f : 0.0f)));
        }
    return worst;
}

bool Finite(const float* v, size_t n) {
    for (size_t i = 0; i < n; ++i)
        if (!std::isfinite(v[i]))
            return false;
    return true;
}

float MostApart(const float* a, const float* b, size_t n) {
    float worst = 0.0f;
    for (size_t i = 0; i < n; ++i)
        worst = std::max(worst, std::fabs(a[i] - b[i]));
    return worst;
}

float Distance(const float* a, const float* b) {
    const float d[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    return std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
}

// The angle between two directions, degrees (180 for one of no length).
float Angle(const float* a, const float* b) {
    const float na = std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]), nb = std::sqrt(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]);
    if (na < 1e-9f || nb < 1e-9f)
        return 180.0f;
    const float c = (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]) / (na * nb);
    return std::acos(std::fmax(-1.0f, std::fmin(1.0f, c))) / kDegree;
}

std::mt19937 g_random(20261106);

float Uniform(float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(g_random); }

// A random turn (a unit quaternion, evenly over all turns) and a translation within a metre.
void RandomRigid(float* m) {
    std::normal_distribution<float> normal(0.0f, 1.0f);
    float q[4], length = 0.0f;
    do {
        for (float& c : q)
            c = normal(g_random);
        length = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    } while (length < 1e-3f);
    const float x = q[0] / length, y = q[1] / length, z = q[2] / length, w = q[3] / length;
    m[0] = 1 - 2 * (y * y + z * z), m[1] = 2 * (x * y - z * w), m[2] = 2 * (x * z + y * w);
    m[4] = 2 * (x * y + z * w), m[5] = 1 - 2 * (x * x + z * z), m[6] = 2 * (y * z - x * w);
    m[8] = 2 * (x * z - y * w), m[9] = 2 * (y * z + x * w), m[10] = 1 - 2 * (x * x + y * y);
    m[3] = Uniform(-1.0f, 1.0f), m[7] = Uniform(-1.0f, 1.0f), m[11] = Uniform(-1.0f, 1.0f);
}

// Any affine move: turns, scales and shears.
void RandomAffine(float* m) {
    for (int i = 0; i < 12; ++i)
        m[i] = Uniform(-2.0f, 2.0f);
}

// The right glove's bone (its number less kRightFirst) of a finger at so many joints from the palm.
int RightBoneOf(int finger, int depth) {
    for (int r = 0; r < kRightBones; ++r)
        if (handpose::kRightFinger[r] == finger && handpose::kRightDepth[r] == depth)
            return r;
    return -1;
}

std::string FolderOfExe() {
    char path[MAX_PATH] = {};
    const DWORD n = GetModuleFileNameA(nullptr, path, MAX_PATH);
    const std::string s(path, n);
    const size_t slash = s.find_last_of("\\/");
    return slash == std::string::npos ? std::string(".") : s.substr(0, slash);
}

// The hand's shapes the free right glove takes (hands.cpp: the index by the trigger, the others by the
// grip, the thumb by the thumb on the stick), at the game's own proportions.
handpose::Fingers Shape(float index, float others, float thumb) {
    handpose::Fingers f{};
    f.curl[0] = index;
    f.curl[1] = f.curl[2] = f.curl[3] = others;
    f.thumb = thumb;
    f.spread = 0.7f;
    f.slim = 1.0f;
    return f;
}

}  // namespace

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::printf("twintest: the right glove as the left's mirror image\n");
    const std::string dir = argc > 1 ? argv[1] : FolderOfExe();

    // ---- 1. The twins: the same finger, as many joints from the palm ----
    {
        float joints[5][4][3], middle[3];
        handpose::Joints(joints);
        handpose::Pivot(-1, middle);
        int readBack = 0;
        for (int b = 0; b < kBones; ++b) {
            float p[3];
            handpose::Pivot(b, p);
            const int f = kLeftFinger[b], d = kLeftDepth[b];
            const float* expect = f >= 0 && d > 0 ? joints[f][d - 1] : middle;
            if (std::memcmp(p, expect, sizeof(p)) == 0)
                ++readBack;
            else
                std::printf("     the left's bone %d turns about (%.4f %.4f %.4f), not the %s's joint %d\n", b, p[0], p[1], p[2],
                            f >= 0 ? kFingerName[f] : "palm", d);
        }
        Check(readBack == kBones, "1 twins: the left's fingers and joints from the palm as handpose.cpp has them (read back through Pivot: %d of %d bones)", readBack,
              kBones);
        int same = 0, fingerBones = 0;
        for (int b = 0; b < kBones; ++b) {
            const int r = handpose::kRightOfLeft[b] - kRightFirst;
            if (r >= 0 && r < kRightBones && handpose::kRightFinger[r] == kLeftFinger[b] && handpose::kRightDepth[r] == kLeftDepth[b])
                ++same;
            else
                std::printf("     the left's bone %d (finger %d, %d from the palm): its twin %d is finger %d, %d from the palm\n", b, kLeftFinger[b], kLeftDepth[b],
                            handpose::kRightOfLeft[b], r >= 0 && r < kRightBones ? handpose::kRightFinger[r] : 99,
                            r >= 0 && r < kRightBones ? handpose::kRightDepth[r] : 99);
            fingerBones += kLeftFinger[b] >= 0;
        }
        Check(same == kBones && fingerBones == 19, "1 twins: every left bone's twin the same finger with as many joints from the palm (%d of %d; %d of them a finger's)", same, kBones,
              fingerBones);
        const int notFingers[4][2] = {{0, 37}, {1, 32}, {4, 44}, {5, 44}};
        bool wrist = true;
        for (const auto& n : notFingers)
            wrist = wrist && handpose::kRightOfLeft[n[0]] == n[1] && kLeftFinger[n[0]] < 0 && handpose::kRightFinger[n[1] - kRightFirst] < 0;
        Check(wrist, "1 twins: the wrist's 0 on 37, the palm's 1 on 32, the cuff's 4 and the forearm's own 5 on 44, none a finger's bone on either glove");
    }

    // ---- 2. The two tables each other's inverse ----
    {
        int back = 0, forth = 0, once = 0;
        int taken[kRightBones] = {};
        for (int r = 0; r < kRightBones; ++r) {
            const int b = handpose::kLeftOfRight[r];
            back += b >= 0 && b < kBones && b != 5 && handpose::kRightOfLeft[b] == r + kRightFirst;
        }
        for (int b = 0; b < kBones; ++b) {
            const int r = handpose::kRightOfLeft[b] - kRightFirst;
            if (r < 0 || r >= kRightBones || b == 5)
                continue;
            forth += handpose::kLeftOfRight[r] == b;
            ++taken[r];
        }
        for (const int t : taken)
            once += t == 1;
        Check(back == kRightBones, "2 tables: kRightOfLeft[kLeftOfRight[r]] the right bone itself for every right bone (%d of %d), none the left's 5", back, kRightBones);
        Check(forth == kBones - 1, "2 tables: kLeftOfRight[kRightOfLeft[b]] the left bone itself for every left bone but 5 (%d of %d)", forth, kBones - 1);
        Check(once == kRightBones && handpose::kRightOfLeft[5] == 44, "2 tables: every right bone the twin of exactly one left bone (%d of %d), the left's 5 on the cuff's 44",
              once, kRightBones);
    }

    // ---- 3. Mirror ----
    {
        constexpr int kTries = 1000;
        int pattern = 0, twice = 0, inPlace = 0;
        float worstDet = 0.0f, worstPoint = 0.0f, worstProduct = 0.0f;
        for (int i = 0; i < kTries; ++i) {
            float a[12], b[12], g[12], h[12];
            RandomRigid(a), RandomRigid(b), RandomAffine(g), RandomAffine(h);
            float m[12];
            handpose::Mirror(a, m);
            bool p = true;
            for (int k = 0; k < 12; ++k) {
                const bool negated = k == 1 || k == 2 || k == 3 || k == 4 || k == 8;
                p = p && m[k] == (negated ? -a[k] : a[k]);
            }
            pattern += p;
            float mm[12], c[12];
            handpose::Mirror(m, mm);
            twice += std::memcmp(mm, a, sizeof(a)) == 0;
            std::memcpy(c, a, sizeof(a));
            handpose::Mirror(c, c);
            inPlace += std::memcmp(c, m, sizeof(m)) == 0;
            worstDet = std::max({worstDet, std::fabs(Det(m) - Det(a)), std::fabs(Det(m) - 1.0f)});
            // A point mirrored, moved by the mirrored move: where the move puts the point, mirrored.
            for (const float* move : {static_cast<const float*>(a), static_cast<const float*>(g)}) {
                float moved[12];
                handpose::Mirror(move, moved);
                const float point[3] = {Uniform(-1.0f, 1.0f), Uniform(-1.0f, 1.0f), Uniform(-1.0f, 1.0f)};
                const float mirrored[3] = {-point[0], point[1], point[2]};
                float q1[3], q2[3];
                Apply(moved, mirrored, q1);
                Apply(move, point, q2);
                q2[0] = -q2[0];
                worstPoint = std::max(worstPoint, Distance(q1, q2));
            }
            // A product's mirror and the product of the mirrors.
            const float* pairs[2][2] = {{a, b}, {g, h}};
            for (const auto& pair : pairs) {
                float ab[12], mab[12], ma[12], mb[12], mamb[12];
                Compose(pair[0], pair[1], ab);
                handpose::Mirror(ab, mab);
                handpose::Mirror(pair[0], ma);
                handpose::Mirror(pair[1], mb);
                Compose(ma, mb, mamb);
                worstProduct = std::max(worstProduct, MostApart(mab, mamb, 12));
            }
        }
        Check(pattern == kTries, "3 mirror: elements 1, 2, 3, 4 and 8 negated, the rest as they were (%d of %d random rigid moves)", pattern, kTries);
        Check(twice == kTries && inPlace == kTries, "3 mirror: mirrored twice the move itself (%d of %d) and in place the same (%d), bit for bit", twice, kTries, inPlace);
        Check(worstDet < 1e-5f, "3 mirror: the turn's determinant kept, +1: no face turned inside out (worst %.1e)", worstDet);
        Check(worstPoint < 1e-5f, "3 mirror: a mirrored point moved where the mirrored move puts it, rigid and any affine move (worst %.1e m)", worstPoint);
        Check(worstProduct < 1e-5f, "3 mirror: a product's mirror the product of the mirrors (worst %.1e)", worstProduct);
        const float nan = std::numeric_limits<float>::quiet_NaN();
        float broken[12] = {nan, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}, out[12];
        handpose::Mirror(broken, out);
        Check(std::isnan(out[0]) && Finite(out + 1, 11), "3 mirror: a number not finite stays where it was, the others mirrored (no new one made)");
    }

    // ---- 4. The round trip, bone by bone ----
    {
        float left[kBones][12], right[kRightBones][12];
        for (auto& m : left)
            RandomRigid(m);
        for (auto& m : right)
            RandomRigid(m);
        float toRight[kRightBones][12], backLeft[kBones][12];
        handpose::LeftToRight(left, toRight);
        handpose::RightToLeft(toRight, backLeft);
        int same = 0;
        for (int b = 0; b < kBones; ++b)
            same += b != 5 && std::memcmp(backLeft[b], left[b], sizeof(left[b])) == 0;
        const bool fifth = std::memcmp(backLeft[5], left[4], sizeof(left[4])) == 0;
        Check(same == kBones - 1 && fifth, "4 round trip: RightToLeft(LeftToRight(L)) = L bit for bit for every left bone but 5 (%d of %d), 5 given its 4's (%s)", same,
              kBones - 1, fifth ? "yes" : "no");
        float toLeft[kBones][12], backRight[kRightBones][12];
        handpose::RightToLeft(right, toLeft);
        handpose::LeftToRight(toLeft, backRight);
        same = 0;
        for (int r = 0; r < kRightBones; ++r)
            same += std::memcmp(backRight[r], right[r], sizeof(right[r])) == 0;
        Check(same == kRightBones, "4 round trip: LeftToRight(RightToLeft(R)) = R bit for bit (%d of %d)", same, kRightBones);
        int twinR = 0, twinL = 0;
        for (int r = 0; r < kRightBones; ++r) {
            float m[12];
            handpose::Mirror(left[handpose::kLeftOfRight[r]], m);
            twinR += std::memcmp(m, toRight[r], sizeof(m)) == 0;
        }
        for (int b = 0; b < kBones; ++b) {
            float m[12];
            handpose::Mirror(right[handpose::kRightOfLeft[b] - kRightFirst], m);
            twinL += std::memcmp(m, toLeft[b], sizeof(m)) == 0;
        }
        Check(twinR == kRightBones && twinL == kBones, "4 tables: each right bone its left twin's mirror (%d of %d), each left bone its right twin's (%d of %d)", twinR, kRightBones,
              twinL, kBones);
        // The two tables in one buffer: read whole before anything is written.
        float shared[kBones][12];
        std::memcpy(shared, left, sizeof(left));
        handpose::LeftToRight(shared, shared);
        const bool sharedRight = std::memcmp(shared, toRight, sizeof(toRight)) == 0;
        std::memcpy(shared, right, sizeof(right));
        handpose::RightToLeft(shared, shared);
        const bool sharedLeft = std::memcmp(shared, toLeft, sizeof(toLeft)) == 0;
        Check(sharedRight && sharedLeft, "4 tables: the same with both tables in one buffer (LeftToRight %s, RightToLeft %s)", sharedRight ? "yes" : "no", sharedLeft ? "yes" : "no");
    }

    // ---- 5. The right glove's poses ----
    {
        struct Named {
            const char* name;
            handpose::Fingers f;
        };
        const Named shapes[] = {{"relaxed", Shape(0.0f, 0.0f, 0.0f)},
                                {"pointing", Shape(0.0f, 1.0f, 1.0f)},
                                {"a fist", Shape(1.0f, 1.0f, 1.0f)},
                                {"half closed", Shape(0.5f, 0.5f, 0.4f)}};
        for (const Named& s : shapes) {
            float right[kRightBones][12], left[kBones][12], flipped[kRightBones][12];
            handpose::PoseRight(s.f, 1.0f, right);
            handpose::Pose(s.f, 1.0f, left);
            handpose::LeftToRight(left, flipped);
            const bool asFlip = std::memcmp(right, flipped, sizeof(right)) == 0;
            // As orthonormal as the left's own, to the bit (the mirror adds nothing): the left's folded
            // thumb turns about handpose.cpp's kAlong, 0.99978 long, so it is 1.5e-4 off in a fist.
            float off = 0.0f, det = 0.0f;
            bool asLeft = true;
            int worstBone = -1;
            for (int r = 0; r < kRightBones; ++r) {
                const float* twin = left[handpose::kLeftOfRight[r]];
                const float o = OffOrthonormal(right[r]), d = std::fabs(Det(right[r]) - 1.0f);
                asLeft = asLeft && o == OffOrthonormal(twin) && d == std::fabs(Det(twin) - 1.0f);
                if (o > off)
                    off = o, worstBone = kRightFirst + r;
                det = std::max(det, d);
            }
            const bool finite = Finite(right[0], kRightBones * 12);
            Check(asFlip, "5 poses: %s: PoseRight the left's Pose flipped, bit for bit", s.name);
            Check(finite && asLeft && off < 5e-4f && det < 5e-4f,
                  "5 poses: %s at the game's own proportions (width 1, slim 1): finite, orthonormal to %.1e (bone %d), determinant 1 to %.1e, each bone exactly as its left twin", s.name,
                  off, worstBone, det);
        }
        // Widened as the left glove is (its width, its fingers slimmed): scaled, never inside out.
        {
            handpose::Fingers f = Shape(0.3f, 0.6f, 0.5f);
            f.slim = 0.88f;
            float right[kRightBones][12];
            handpose::PoseRight(f, 1.15f, right);
            float least = 1e9f;
            for (const auto& m : right)
                least = std::min(least, Det(m));
            Check(Finite(right[0], kRightBones * 12) && least > 0.5f, "5 poses: widened as the left (1.15, slim 0.88): finite, no bone inside out (least determinant %.3f)",
                  least);
        }
        // Numbers not finite given (a controller's glitch): each taken as relaxed, the width the glove's own.
        {
            const float nan = std::numeric_limits<float>::quiet_NaN(), inf = std::numeric_limits<float>::infinity();
            handpose::Fingers bad = Shape(nan, 0.6f, -inf);
            bad.curl[2] = inf, bad.spread = nan, bad.apart[1] = nan, bad.thumbOut = inf, bad.thumbRoll = nan, bad.slim = nan;
            handpose::Fingers clean = Shape(0.0f, 0.6f, 0.0f);
            clean.curl[2] = 0.0f, clean.spread = 0.0f, clean.apart[1] = 0.0f, clean.thumbOut = 0.0f, clean.thumbRoll = 0.0f, clean.slim = 0.0f;
            float right[kRightBones][12], expect[kRightBones][12];
            handpose::PoseRight(bad, nan, right);
            handpose::PoseRight(clean, 1.0f, expect);
            const bool relaxed = std::memcmp(right, expect, sizeof(right)) == 0;
            bool widths = true;
            for (const float width : {inf, -inf, 0.0f, -1.0f}) {
                float w[kRightBones][12];
                handpose::PoseRight(Shape(0.2f, 0.4f, 0.3f), width, w);
                float least = 1e9f;
                for (const auto& m : w)
                    least = std::min(least, Det(m));
                widths = widths && Finite(w[0], kRightBones * 12) && least > 0.5f;
            }
            Check(Finite(right[0], kRightBones * 12) && relaxed && widths,
                  "5 poses: numbers not finite given: finite, each such number taken as relaxed (%s), a width not finite or not positive as the glove's own (%s)",
                  relaxed ? "yes" : "no", widths ? "yes" : "no");
        }
    }

    // ---- 6. The right fingers curl the way the game's own rig bends them ----
    {
        float joints[5][4][3];
        handpose::Joints(joints);
        const handpose::Fingers relaxed = Shape(0.0f, 0.0f, 0.0f);
        float right0[kRightBones][12], left0[kBones][12];
        handpose::PoseRight(relaxed, 1.0f, right0);
        handpose::Pose(relaxed, 1.0f, left0);
        float worstPosed = 0.0f, worstBent = 0.0f, worstTip = 0.0f;
        for (int f = 0; f < 5; ++f) {
            const int r = RightBoneOf(f, 3);
            const float tip[3] = {-joints[f][3][0], joints[f][3][1], joints[f][3][2]};  // the left's tip, mirrored
            float at0[3];
            Apply(right0[r], tip, at0);
            // The rig's own bend of that finger (degrees towards the palm), from the same pose.
            float palette[kGloveBones][12];
            for (int k = 0; k < kRightFirst; ++k)
                Identity(palette[k]);
            std::memcpy(palette[kRightFirst], right0, sizeof(right0));
            float bend[5] = {};
            bend[f] = 20.0f;
            handpose::BendRight(palette, bend);
            float atRig[3];
            Apply(palette[kRightFirst + r], tip, atRig);
            // Posed closer.
            handpose::Fingers closer = relaxed;
            if (f == 0)
                closer.thumb = 0.3f;
            else
                closer.curl[f - 1] = 0.3f;
            float right1[kRightBones][12], atPosed[3];
            handpose::PoseRight(closer, 1.0f, right1);
            Apply(right1[r], tip, atPosed);
            // Bent as much in the left's layout with the left's own bend, then mirrored.
            float left1[kBones][12], mirrored[kRightBones][12], atMirrored[3];
            std::memcpy(left1, left0, sizeof(left0));
            handpose::BendFurther(left1, bend);
            handpose::LeftToRight(left1, mirrored);
            Apply(mirrored[r], tip, atMirrored);
            float rig[3], posed[3], bent[3];
            for (int k = 0; k < 3; ++k)
                rig[k] = atRig[k] - at0[k], posed[k] = atPosed[k] - at0[k], bent[k] = atMirrored[k] - at0[k];
            const float posedOff = Angle(posed, rig), bentOff = Angle(bent, rig), apart = Distance(atMirrored, atRig);
            std::printf("info 6 curl: the %s's tip, the rig's bend of 20 degrees moving it %.1f mm: posed closer %.1f degrees off its way, bent in the left's layout and "
                        "mirrored %.1f off (%.1f mm from where the rig puts it)\n",
                        kFingerName[f], 1000.0f * Distance(atRig, at0), posedOff, bentOff, 1000.0f * apart);
            if (f > 0)  // (the folded thumb turns across the palm too, not only about the rig's one axis)
                worstPosed = std::max(worstPosed, posedOff);
            worstBent = std::max(worstBent, bentOff), worstTip = std::max(worstTip, apart);
        }
        Check(worstPosed < 30.0f, "6 curl: each right finger posed closer moves its tip the way the game's own rig bends it (worst %.1f degrees off)", worstPosed);
        Check(worstBent < 20.0f, "6 curl: a finger bent in the left's layout and mirrored goes the rig's own way, thumb too (worst %.1f degrees off, %.1f mm from the rig's)", worstBent,
              1000.0f * worstTip);
    }

    // ---- 7. The points the right glove's bones turn about ----
    {
        int same = 0;
        for (int r = 0; r < kRightBones; ++r) {
            float p[3], q[3];
            handpose::PivotRight(kRightFirst + r, p);
            handpose::Pivot(handpose::kLeftOfRight[r], q);
            q[0] = -q[0];
            same += std::memcmp(p, q, sizeof(p)) == 0;
        }
        Check(same == kRightBones, "7 pivots: PivotRight every right bone's twin's Pivot with x negated, bit for bit (%d of %d)", same, kRightBones);
        float middle[3];
        handpose::Pivot(-1, middle);
        middle[0] = -middle[0];
        bool others = true;
        for (const int bone : {-1, 0, 5, 22, 45, 60, 1000, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()}) {
            float p[3];
            handpose::PivotRight(bone, p);
            others = others && std::memcmp(p, middle, sizeof(p)) == 0;
        }
        Check(others, "7 pivots: a bone not the right glove's (-1, 0, 5, 22, 45, 60, 1000, the ints' ends): the right hand's middle (%.3f %.3f %.3f), the left's mirrored", middle[0],
              middle[1], middle[2]);
        float rightJoints[5][4][3];
        handpose::RightJoints(rightJoints);
        float worst = 0.0f;
        int worstBone = -1;
        for (int r = 0; r < kRightBones; ++r) {
            const int f = handpose::kRightFinger[r], d = handpose::kRightDepth[r];
            if (f < 0 || d < 1)
                continue;
            float p[3];
            handpose::PivotRight(kRightFirst + r, p);
            const float off = Distance(p, rightJoints[f][d - 1]);
            if (off > worst)
                worst = off, worstBone = kRightFirst + r;
        }
        Check(worst < 0.010f, "7 pivots: each within 10 mm of the right glove's own joint (RightJoints, from the game's grip; worst %.1f mm, bone %d)", 1000.0f * worst, worstBone);
    }

    // ---- 8. The mesh: each twin where its left twin is, mirrored ----
    {
        const auto vb = meshdump::ReadAll(dir + "\\mesh_13755_15051_vb0.bin"), ib = meshdump::ReadAll(dir + "\\mesh_13755_15051_ib.bin");
        if (vb.size() < static_cast<size_t>(meshdump::kGloveOffset) || ib.size() < (meshdump::kGloveFirst + meshdump::kGloveCount) * 2u) {
            std::printf("skip 8 mesh: the gloves' mesh dump isn't in %s\n", dir.c_str());
        } else {
            // Each vertex once, on the hand its heaviest bone is; each bone's vertices' centre, by their
            // weights on it.
            struct Vertex {
                float p[3];
                int bone[4];
                float weight[4];
            };
            std::vector<Vertex> lefts, rights;
            double sum[kGloveBones][3] = {}, weight[kGloveBones] = {};
            std::vector<char> seen(65536, 0);
            int outside = 0;
            for (int j = meshdump::kGloveFirst; j < meshdump::kGloveFirst + meshdump::kGloveCount; ++j) {
                uint16_t v;
                std::memcpy(&v, &ib[2 * static_cast<size_t>(j)], 2);
                if (seen[v])
                    continue;
                seen[v] = 1;
                const size_t at = meshdump::kGloveOffset + v * static_cast<size_t>(meshdump::kStride);
                if (at + 16 > vb.size()) {
                    ++outside;
                    continue;
                }
                const uint8_t* o = &vb[at];
                uint16_t h[3];
                std::memcpy(h, o, 6);
                Vertex x{};
                int heaviest = 0;
                for (int k = 0; k < 3; ++k)
                    x.p[k] = meshdump::Half(h[k]);
                for (int s = 0; s < 4; ++s) {
                    x.bone[s] = o[8 + s], x.weight[s] = o[12 + s] / 255.0f;
                    if (x.weight[s] > x.weight[heaviest])
                        heaviest = s;
                    if (x.weight[s] <= 0.0f || x.bone[s] >= kGloveBones)
                        continue;
                    for (int k = 0; k < 3; ++k)
                        sum[x.bone[s]][k] += x.weight[s] * static_cast<double>(x.p[k]);
                    weight[x.bone[s]] += x.weight[s];
                }
                (x.bone[heaviest] < kBones ? lefts : rights).push_back(x);
            }
            float centre[kGloveBones][3] = {};
            for (int b = 0; b < kGloveBones; ++b)
                for (int k = 0; k < 3; ++k)
                    centre[b][k] = weight[b] > 0.0 ? static_cast<float>(sum[b][k] / weight[b]) : 0.0f;
            auto apart = [&](int b) {
                const int twin = handpose::kRightOfLeft[b];
                const float mirrored[3] = {-centre[b][0], centre[b][1], centre[b][2]};
                return weight[b] > 0.0 && weight[twin] > 0.0 ? Distance(mirrored, centre[twin]) : 1e9f;
            };
            // The fingers: each bone's vertices about one joint, so their centres are where the bone is.
            float worst = 0.0f;
            int worstBone = -1, pairs = 0;
            for (int b = 0; b < kBones; ++b) {
                if (kLeftFinger[b] < 0)
                    continue;
                ++pairs;
                if (apart(b) > worst)
                    worst = apart(b), worstBone = b;
            }
            Check(pairs == 19 && worst < 0.010f, "8 mesh: each finger bone's twin's vertices' centre within 10 mm of its left twin's mirrored (%d pairs; worst %.1f mm, the left's %d and %d)",
                  pairs, 1000.0f * worst, worstBone, worstBone >= 0 ? handpose::kRightOfLeft[worstBone] : -1);
            // Every bone by its weights at the same place: each left vertex with the right one nearest it
            // mirrored (within 3 mm), each pair's weights shared bone by bone.
            std::vector<double> shared(static_cast<size_t>(kBones) * kGloveBones, 0.0);
            int matched = 0;
            for (const Vertex& l : lefts) {
                const float mirrored[3] = {-l.p[0], l.p[1], l.p[2]};
                const Vertex* nearest = nullptr;
                float nearestOff = 0.003f;
                for (const Vertex& r : rights) {
                    const float d = Distance(mirrored, r.p);
                    if (d <= nearestOff)
                        nearestOff = d, nearest = &r;
                }
                if (!nearest)
                    continue;
                ++matched;
                for (int s = 0; s < 4; ++s)
                    for (int t = 0; t < 4; ++t)
                        if (l.weight[s] > 0.0f && nearest->weight[t] > 0.0f && l.bone[s] < kBones && nearest->bone[t] >= kRightFirst && nearest->bone[t] < kGloveBones)
                            shared[static_cast<size_t>(l.bone[s]) * kGloveBones + nearest->bone[t]] += std::min(l.weight[s], nearest->weight[t]);
            }
            int agree = 0, asked = 0;
            for (int b = 0; b < kBones; ++b) {
                if (b == 4 || b == 5)  // the cuff's: the right glove has next to no weight on its 44 (below); kGlovesForearm's twist bones decide
                    continue;
                ++asked;
                int most = -1;
                double total = 0.0;
                for (int r = kRightFirst; r < kGloveBones; ++r) {
                    const double w = shared[static_cast<size_t>(b) * kGloveBones + r];
                    total += w;
                    if (w > 0.0 && (most < 0 || w > shared[static_cast<size_t>(b) * kGloveBones + most]))
                        most = r;
                }
                if (most == handpose::kRightOfLeft[b])
                    ++agree;
                else
                    std::printf("     the left's bone %d shares the most weight with the right's %d (%.0f%%), not its twin %d (%.0f%%)\n", b, most,
                                most >= 0 && total > 0.0 ? 100.0 * shared[static_cast<size_t>(b) * kGloveBones + most] / total : 0.0, handpose::kRightOfLeft[b],
                                total > 0.0 ? 100.0 * shared[static_cast<size_t>(b) * kGloveBones + handpose::kRightOfLeft[b]] / total : 0.0);
            }
            Check(asked == kBones - 2 && agree == asked,
                  "8 mesh: each left bone's twin the right bone sharing the most of its weight at the same place, mirrored (%d of %d; %d of the left's %zu vertices with a right one within "
                  "3 mm)",
                  agree, asked, matched, lefts.size());
            std::printf("info 8 mesh: the wrist's 0 and 37 centres %.1f mm apart, the palm's 1 and 32 %.1f mm, the cuff's 4 and 44 %.1f mm: the left glove's cuff reaches further up "
                        "its forearm (%zu vertices against the right's %zu); the right's 44 carries weight %.1f in all against the left's 4's %.1f (%d vertices past the buffer's end)\n",
                        1000.0f * apart(0), 1000.0f * apart(1), 1000.0f * apart(4), lefts.size(), rights.size(), weight[44], weight[4], outside);
        }
    }

    // ---- 9. The time ----
    {
        handpose::Fingers f = Shape(0.0f, 0.5f, 0.5f);
        float out[kRightBones][12];
        volatile float sink = 0.0f;
        constexpr int kTimes = 20000;
        const auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < kTimes; ++i) {
            f.curl[0] = static_cast<float>(i % 100) / 100.0f;
            handpose::PoseRight(f, 1.0f, out);
            sink = sink + out[8][3];
        }
        const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count() / kTimes;
        float left[kBones][12];
        handpose::Pose(f, 1.0f, left);
        const auto start2 = std::chrono::steady_clock::now();
        for (int i = 0; i < kTimes; ++i) {
            left[0][3] = static_cast<float>(i);
            handpose::LeftToRight(left, out);
            sink = sink + out[14][3];
        }
        const double flip = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start2).count() / kTimes;
        Check(us < 20.0, "9 time: PoseRight %.2f microseconds (LeftToRight alone %.3f)", us, flip);
    }

    std::printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed;
}

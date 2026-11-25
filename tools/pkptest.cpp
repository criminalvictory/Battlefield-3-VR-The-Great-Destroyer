// The PKP Pecheneg (Operation Swordbreaker), held to what was measured, without the game: skin.cpp's own rows for it (src/pkprows.h, which skin.cpp builds its
// rows from: a copy here could drift) against the mesh dumps and the recordings they were read from (build\mesh_23298_2310_*, 2310_0, the gloves' 13755_15051;
// build\survey\PKP_partial.bin, PKP_empty.bin), the tables written from them (src/magposes/pkp*.h and the box's way pkp_way.h, src/recoilguns.h's row,
// src/gunbodies.h's body, src/shaperecipes.h's recipe, its shape build\guns\pkp.bin), and its belt reload worked by the left hand through handmag itself
// (src/handmag.cpp), as the M240's and the M249's are.
//   p1  the tables: one row for it in each (its recoil row: its body draw and shape, a hundred rounds, 560 m/s, 600 a minute, its kick recorded; its box its
//       whole count by its shape, an open bolt (pickrules: 100); its body, 8.2 kg empty and 26 g a round; its shape's recipe; its name; its casings out to its
//       left; its box dropped metal), every number finite
//   p2  the mesh (SKIP without the dumps): its bones (box, flaps, feed arm, cover, handle, trigger, frame); the handle's knob where the row's lever is; the
//       sights' line from the game's zoomed eye through the front post's tip, to the muzzle, and the first of the gun within 1 cm of it; the belt's five
//       pieces as the row has them, numbered from the box; the box: a bone of the body, its box, its rest on the frame, its top and front
//   p3  the recordings (SKIP without them): the depth pass's bones (0-4 the belt, then the body's 0-8) in every frame; the cover's hinge and where it rests
//       open, and its other three bones turned with it; the handle's draw straight back as far as the row's travel; the belt laid and hanging as the tables
//       have them, never hidden by the game's count; the palms at rest; the left hand on the handguard all through the reload (the right does it all); the
//       game's right hand on the cover and on the box, mirrored, where the fitted holds are; the kick of each burst's first round as the recoil row has it;
//       the box's way off and on (pkp_way.h) as the empty reload has it
//   p4  the fitted hands: the box's hold at its left end (its palm within 8 mm, every finger on it, none of it into it past a glove's give) and clear of
//       the gun; the cover's on its top, nothing of it into the gun's shape past a glove's give; the handle's over and under at the knob, on the gun's
//       right, from under it its palm within 8 mm of the gun; none of their tables broken
//   p5  handmag on the PKP's own table: the whole reload by hand (cover up; the box off along its own reload's way, to the gun's right, free 8.2 cm along;
//       straight or steeply down it stays in; a fresh one home by itself along its way on, from the right and below, swung into its seat; its belt hanging,
//       pinched at its tip and laid in the feed, bowing out to the gun's right in the hand; the cover shut; the handle over and under; a round chambered),
//       the belt not ready until all of it is done; Claims and Update agreeing
//   p6  its cover in four bones: the three others turned with it as its own bone is (skin.cpp's lidAlso, the depth pass's the same bones); nothing of the
//       gun's own rides the cover's bone but itself
//   p7  its box dropped: metal; its nine clips in the table of the game's own sounds, cut from its own reload wave
// Usage: pkptest [<folder with the mesh dumps> [<folder with the recordings>]] (build and build\survey by default, from the project's folder). Reads only.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "../src/gunbodies.h"
#include "../src/gunprofile.h"
#include "../src/gunshape.h"
#include "../src/handmag.h"
#include "../src/handpose.h"
#include "../src/lidride.h"
#include "../src/loose.h"
#include "../src/magposes.h"
#include "../src/m249rows.h"
#include "../src/magsound.h"
#include "../src/pickrules.h"
#include "../src/pkprows.h"
#include "../src/recoilguns.h"
#include "../src/shaperecipes.h"
#include "meshdump.h"

namespace {

int g_checks = 0, g_failed = 0, g_skipped = 0;
void Check(bool ok, const char* what) {
    ++g_checks;
    g_failed += !ok;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
}
void Skip(const char* what) {
    ++g_skipped;
    std::printf("SKIP %s\n", what);
}

constexpr float kPi = 3.14159265f;

// ---- skin.cpp's own rows for it (pkprows.h: skin.cpp builds kPkpBones, its kHeldKinds and kHeldEyes rows and handmag's from these very ones) ----
const handmag::BeltGun& kBelt = pkprows::kBelt;
const handmag::Gun& kGun = pkprows::kMag;
const handmag::BoxWay& kWay = pkprows::kWay;
constexpr int kBody[2] = {static_cast<int>(pkprows::kBody), static_cast<int>(pkprows::kBodyStart)}, kDepth[2] = {static_cast<int>(pkprows::kDepth), static_cast<int>(pkprows::kDepth)};
constexpr int kBeltDraw[2] = {static_cast<int>(pkprows::kBeltCount), static_cast<int>(pkprows::kBeltStart)};
constexpr int kFrameBone = pkprows::kFrameBone, kBoxBone = pkprows::kBoxBone, kHandleBone = pkprows::kHandleBone, kCoverBone = pkprows::kCoverBone;
constexpr int kBeltBones = pkprows::kDepthBeltBones;
const float* const kBoxMin = pkprows::kBoxMin;
const float* const kBoxMax = pkprows::kBoxMax;
constexpr float kUp = pkprows::kUp;
const float* const kSights = pkprows::kSights;
const float* const kMuzzle = pkprows::kMuzzle;
const float* const kPort = pkprows::kPort;
const float* const kRightPalm = pkprows::kRightPalm;
const float* const kLeftPalm = pkprows::kLeftPalm;
constexpr float kEyeZ = pkprows::kEyeZ, kRearZ = pkprows::kRearZ;
// What the game said zoomed (build\survey\PKP_zoom.txt): the view in each of its bones 0-5, FOV 40, all six on the frame within 0.6 mm of this eye.
constexpr float kZoomedEye[3] = {0.0f, 0.1148f, 0.1588f}, kZoomedForward[3] = {-0.0012f, -0.0032f, 1.0f};
// Its box's bone 0 at rest on the frame (the dump): moved, unturned.
constexpr float kBoxRest[3] = {0.0f, 0.0761f, 0.4901f};
// The gloves' draw and the palms' points in it (as tools/m249test.cpp has them); the right glove's palm bone (the left's 1's twin) and its hand's middle.
constexpr int kGloves[2] = {13755, 15051};
constexpr int kRightRoot = 37, kLeftPalmBone = 1, kRightPalmBone = handpose::kRightOfLeft[1];
constexpr float kRightPalmPoint[3] = {-0.505f, 1.129f, 0.150f}, kLeftPalmPoint[3] = {0.537f, 1.098f, 0.207f};
constexpr float kHandInGlove[3] = {0.5363f, 1.0976f, 0.2092f};  // handmag.cpp's: the hand's middle
constexpr float kHandInRightGlove[3] = {-0.5363f, 1.0976f, 0.2092f};  // ... the right glove's, its mirror

float Half(uint16_t h) {
    const int s = (h >> 15) & 1, e = (h >> 10) & 0x1F, m = h & 0x3FF;
    const float v = e == 0 ? std::ldexp(m / 1024.0f, -14) : std::ldexp(1 + m / 1024.0f, e - 15);
    return s ? -v : v;
}
std::vector<uint8_t> ReadAll(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
// 3x4, rows.
void Compose(const float* a, const float* b, float* out) {
    float r[12];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            r[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        r[i * 4 + 3] = a[i * 4] * b[3] + a[i * 4 + 1] * b[7] + a[i * 4 + 2] * b[11] + a[i * 4 + 3];
    }
    std::memcpy(out, r, sizeof(r));
}
void InvertRigid(const float* m, float* out) {
    float r[12];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            r[i * 4 + j] = m[j * 4 + i];
    for (int i = 0; i < 3; ++i)
        r[i * 4 + 3] = -(r[i * 4] * m[3] + r[i * 4 + 1] * m[7] + r[i * 4 + 2] * m[11]);
    std::memcpy(out, r, sizeof(r));
}
void Apply(const float* m, const float* p, float* o) {
    float r[3];
    for (int k = 0; k < 3; ++k)
        r[k] = m[k * 4] * p[0] + m[k * 4 + 1] * p[1] + m[k * 4 + 2] * p[2] + m[k * 4 + 3];
    std::memcpy(o, r, sizeof(r));
}
float Dist(const float* a, const float* b) {
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}
float Apart(const float* a, const float* b) {  // two bones' places
    return std::sqrt((a[3] - b[3]) * (a[3] - b[3]) + (a[7] - b[7]) * (a[7] - b[7]) + (a[11] - b[11]) * (a[11] - b[11]));
}
float TurnBetween(const float* a, const float* b) {  // degrees
    float trace = 0.0f;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            trace += a[i * 4 + j] * b[i * 4 + j];
    return std::acos(std::clamp((trace - 1.0f) * 0.5f, -1.0f, 1.0f)) * 180.0f / kPi;
}
bool Squashed(const float* m) {
    return m[0] * m[0] + m[4] * m[4] + m[8] * m[8] < 0.25f;
}
// The mirror across the gun's x = 0 plane of a 3x4 on the gun's frame (handpose::Mirror's).
void Mirror(const float* in, float* out) {
    handpose::Mirror(in, out);
}
// The line the sights' row makes: from its point at the muzzle back along z, rising by kUp the further back.
float LineY(float z) {
    return kSights[1] + (kSights[2] - z) * std::tan(kUp * kPi / 180.0f);
}

// A mesh dump's draw: each vertex once, posed by the dump's own bones into the space of bone `frame` (or left in its own vertices' space: frame -1), with
// the bone it goes with. Stride 48: a half position at 0, four bones at 8, four weights at 12.
struct Vertex {
    float at[3];   // posed
    float own[3];  // as stored
    int bone;
};
struct Dump {
    bool read = false;
    std::vector<uint8_t> bones;
    std::vector<Vertex> vertices;
    const float* Bone(int b) const { return reinterpret_cast<const float*>(bones.data()) + b * 12; }
    int Bones() const { return static_cast<int>(bones.size() / 48); }
};
Dump Load(const std::string& dir, int count, int start, int frame) {
    Dump d;
    const std::string name = dir + "\\mesh_" + std::to_string(count) + "_" + std::to_string(start) + "_";
    const auto vb = ReadAll(name + "vb0.bin"), ib = ReadAll(name + "ib.bin");
    d.bones = ReadAll(name + "bones.bin");
    std::ifstream text(name + "dump.txt");
    std::string word;
    unsigned stride = 0, offset = 0;
    while (text >> word)
        if (word == "vb") {
            int slot = -1;
            std::string a, b;
            unsigned s = 0, o = 0;
            if (text >> slot >> a >> s >> b >> o && slot == 0)
                stride = s, offset = o;
            break;
        }
    if (vb.empty() || ib.size() < static_cast<size_t>(start + count) * 2 || d.bones.size() < 48 || stride != 48)
        return d;
    const int boneCount = d.Bones();
    float inverse[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    if (frame >= 0 && frame < boneCount)
        InvertRigid(d.Bone(frame), inverse);
    std::vector<int> seen(65536, 0);
    for (int i = start; i < start + count; ++i) {
        uint16_t v;
        std::memcpy(&v, &ib[2 * static_cast<size_t>(i)], 2);
        if (seen[v])
            continue;
        seen[v] = 1;
        const size_t o = offset + static_cast<size_t>(v) * stride;
        if (o + stride > vb.size())
            return d;
        uint16_t h[3];
        std::memcpy(h, &vb[o], 6);
        Vertex x{};
        x.own[0] = Half(h[0]), x.own[1] = Half(h[1]), x.own[2] = Half(h[2]);
        int bone = 0, most = -1;
        for (int s = 0; s < 4; ++s)
            if (vb[o + 12 + s] > most)
                most = vb[o + 12 + s], bone = vb[o + 8 + s];
        if (bone >= boneCount)
            return d;
        x.bone = bone;
        if (frame >= 0) {
            float posed[3];
            Apply(d.Bone(bone), x.own, posed);
            Apply(inverse, posed, x.at);
        } else {
            std::memcpy(x.at, x.own, sizeof(x.at));
        }
        d.vertices.push_back(x);
    }
    d.read = true;
    return d;
}
struct Box {
    float lo[3] = {1e9f, 1e9f, 1e9f}, hi[3] = {-1e9f, -1e9f, -1e9f};
    int count = 0;
    void Add(const float* p) {
        for (int k = 0; k < 3; ++k)
            lo[k] = std::min(lo[k], p[k]), hi[k] = std::max(hi[k], p[k]);
        ++count;
    }
};

// A recording (weapon.txt "allrec"): records of 8 ints (the draw's count, the frame's serial, the eye, its start, the rounds loaded, the spare, ms, bones)
// and that many bones, in the view's space of the eye the draw was for.
struct Record {
    int count, start, ms, frame, loaded, eye;
    std::vector<float> bones;
    const float* Bone(int b) const { return &bones[static_cast<size_t>(b) * 12]; }
    int Bones() const { return static_cast<int>(bones.size() / 12); }
};
// Each frame's records together, in the order they were drawn.
template <typename Fn>
bool Frames(const std::string& path, Fn&& each) {
    FILE* f = nullptr;
    if (fopen_s(&f, path.c_str(), "rb") || !f)
        return false;
    int head[8], first = -1;
    std::vector<Record> group;
    while (fread(head, 4, 8, f) == 8) {
        const int n = head[7];
        if (n <= 0 || n > 4096)
            break;
        Record r;
        r.bones.resize(static_cast<size_t>(n) * 12);
        if (fread(r.bones.data(), 48, n, f) != static_cast<size_t>(n))
            break;
        if (first < 0)
            first = head[6];
        r.count = head[0], r.start = head[3], r.ms = head[6] - first, r.frame = head[1], r.loaded = head[4], r.eye = head[2];
        if (!group.empty() && group.front().frame != r.frame) {
            each(group);
            group.clear();
        }
        group.push_back(std::move(r));
    }
    if (!group.empty())
        each(group);
    fclose(f);
    return first >= 0;
}
// The n-th record of a draw in a frame's group (each eye's draws in the order drawn: the n-th of each draw goes with the n-th body), or none.
const Record* Nth(const std::vector<Record>& group, int count, int start, int n) {
    for (const Record& r : group)
        if (r.count == count && r.start == start && n-- == 0)
            return &r;
    return nullptr;
}

// ---- The left glove against a mesh, as tools/magfit.cpp measures its fits (tools/m249test.cpp's) ----
constexpr int8_t kGloveDepth[handpose::kBones] = {0, 0, 3, 3, 0, 0, 0, 1, 0, 1, 3, 1, 0, 1, 0, 3, 2, 2, 1, 2, 2, 2, 3};  // magfit.cpp's kDepth
struct Glove {
    bool read = false;
    std::vector<meshdump::GloveVertex> vertices;
    std::vector<int> palm;
    std::vector<int> part[5][4];  // [finger][1-3]
};
Glove LoadGloveParts(const std::string& dir) {
    Glove g;
    std::vector<int> tris;
    if (!meshdump::LoadGlove(dir, g.vertices, tris))
        return g;
    float joints[5][4][3];
    handpose::Joints(joints);
    for (size_t i = 0; i < g.vertices.size(); ++i) {
        const meshdump::GloveVertex& v = g.vertices[i];
        if (v.finger < -1)
            continue;
        int best = 0;
        for (int s = 1; s < 4; ++s)
            if (v.weight[s] > v.weight[best])
                best = s;
        const int b = v.bone[best];
        if (b >= handpose::kBones)
            continue;
        if (v.finger == -1 && b != 0 && b != 4 && b != 5)
            g.palm.push_back(static_cast<int>(i));
        const int f = v.finger, d = kGloveDepth[b];
        if (f >= 0 && d >= 1) {
            const float* j0 = joints[f][d - 1];
            const float* j1 = joints[f][d];
            const float length = std::sqrt((j1[0] - j0[0]) * (j1[0] - j0[0]) + (j1[1] - j0[1]) * (j1[1] - j0[1]) + (j1[2] - j0[2]) * (j1[2] - j0[2]));
            const float margin = d >= 2 ? length * 0.35f : f == 0 ? 0.025f : 0.018f;
            const float dx = v.pos[0] - j0[0], dy = v.pos[1] - j0[1], dz = v.pos[2] - j0[2];
            if (std::sqrt(dx * dx + dy * dy + dz * dz) > margin)
                g.part[f][d].push_back(static_cast<int>(i));
        }
    }
    g.read = !g.palm.empty();
    return g;
}
void PoseVertex(const meshdump::GloveVertex& v, const float (*bones)[12], const float* toGun, float* out) {
    float sum[3] = {};
    for (int s = 0; s < 4; ++s) {
        if (v.weight[s] <= 0.0f || v.bone[s] >= handpose::kBones)
            continue;
        float p[3];
        Apply(bones[v.bone[s]], v.pos, p);
        for (int k = 0; k < 3; ++k)
            sum[k] += v.weight[s] * p[k];
    }
    Apply(toGun, sum, out);
}
struct Field {
    meshdump::Solid solid;
    std::vector<float> distance;
    float At(const float* p) const {
        int x, y, z;
        return solid.Cell(p, x, y, z) ? distance[solid.At(x, y, z)] : 0.05f;
    }
};
Field FieldOf(const meshdump::Mesh& mesh, const float* lo, const float* hi, float cell) {
    Field f;
    f.solid.Build(mesh, lo, hi, cell, -1);
    f.distance = meshdump::DistanceField(f.solid);
    return f;
}
template <typename Distance>
float GapOf(const Glove& g, const std::vector<int>& set, const float (*bones)[12], const float* toGun, Distance&& distance) {
    float gap = 1.0f;
    for (int i : set) {
        float p[3];
        PoseVertex(g.vertices[static_cast<size_t>(i)], bones, toGun, p);
        gap = std::min(gap, distance(p));
    }
    return gap;
}

// ---- handmag on scripted frames at 72 Hz (as tools/m249test.cpp drives the M249) ----
constexpr float kDt = 1.0f / 72.0f;
const float kGunPose[12] = {0.9848f, 0.0f, 0.1736f, 0.05f, 0.0f, 1.0f, 0.0f, -0.2f, -0.1736f, 0.0f, 0.9848f, -0.6f};
handmag::Frame Base() {
    handmag::Frame f;
    f.dt = kDt;
    f.gun = &kGun;
    f.gunTracked = true;
    std::memcpy(f.gunPose, kGunPose, sizeof(f.gunPose));
    std::memcpy(f.frontGrip, kLeftPalm, sizeof(f.frontGrip));
    f.left = true;
    f.spare = true;
    return f;
}
void Reset() {
    handmag::Clear();
    handmag::BeltReset();
    handmag::Frame f = Base();
    f.gun = nullptr;
    handmag::Update(f);
    f = Base();
    handmag::Update(f);
}
// The glove turned as a table's hold has it (the gloves' bind space into the gun's frame), its hand's middle, or its belt pinch, at this place on the gun.
void GloveBy(const float* hold, const float* point, const float* place, float* glove) {
    float inGun[12];
    InvertRigid(hold, inGun);
    float at[3];
    for (int i = 0; i < 3; ++i)
        at[i] = inGun[i * 4] * point[0] + inGun[i * 4 + 1] * point[1] + inGun[i * 4 + 2] * point[2];
    for (int i = 0; i < 3; ++i)
        inGun[i * 4 + 3] = place[i] - at[i];
    Compose(kGunPose, inGun, glove);
}
// The cover's grab (where the fitted hand takes it) with the cover turned up by this much, on the frame.
void LidAt(float angle, float* out) {
    const float c = std::cos(angle), s = std::sin(angle), y = kBelt.lidGrab[1] - kBelt.lidHinge[0], z = kBelt.lidGrab[2] - kBelt.lidHinge[1];
    out[0] = kBelt.lidGrab[0];
    out[1] = kBelt.lidHinge[0] + c * y - s * z;
    out[2] = kBelt.lidHinge[1] + s * y + c * z;
}
// The hand's middle of a table's hold on the gun's frame.
void MiddleOf(const float* hold, float* out) {
    float inGun[12];
    InvertRigid(hold, inGun);
    Apply(inGun, kHandInGlove, out);
}
// The cover up: taken at its grab and lifted till it goes on by itself, then let go of.
bool CoverUp(handmag::Frame& f) {
    Reset();
    f = Base();
    float at[3];
    LidAt(0.0f, at);
    GloveBy(magposes::kPkpLidHold, kHandInGlove, at, f.glove);
    handmag::Update(f);
    f.squeeze = 1.0f;
    const bool taken = handmag::Update(f).lidTaken;
    bool up = false;
    for (int i = 1; i < 90 && !up; ++i) {
        LidAt(std::min(0.02f * i, 0.9f), at);
        GloveBy(magposes::kPkpLidHold, kHandInGlove, at, f.glove);
        up = handmag::Update(f).lidUp;
    }
    f.squeeze = 0.0f;
    handmag::Update(f);
    return taken && up;
}
// The box's way off: how far this point of it (on the gun, the box seated) has come at this far along it (the rows' measure: its grab point's), from its seat.
void WayPoint(const float* point, float at, float* out) {
    int i = 0;
    while (i + 2 < kWay.offCount && kWay.off[i + 1][0] < at)
        ++i;
    float a[3], b[3];
    Apply(kWay.off[i] + 1, point, a);
    Apply(kWay.off[i + 1] + 1, point, b);
    const float span = kWay.off[i + 1][0] - kWay.off[i][0], s = span > 1e-6f ? (at - kWay.off[i][0]) / span : 1.0f;
    for (int k = 0; k < 3; ++k)
        out[k] = a[k] + (b[k] - a[k]) * s - point[k];
}
// ... its grab point's.
void WayGrab(float at, float* out) {
    WayPoint(kGun.grab, at, out);
}
// Where a fresh box starts in on its way on: its top there, on the frame.
void WayOnTop(float* out) {
    Apply(kWay.on[0] + 1, kGun.top, out);
}
// The box taken at the grab with the cover up, off along its way, let go of, and a fresh one home by itself: its belt hanging out of it.
bool BeltHanging(handmag::Frame& f) {
    if (!CoverUp(f))
        return false;
    GloveBy(magposes::kPkpHold, kHandInGlove, kGun.grab, f.glove);
    handmag::Update(f);
    f.squeeze = 1.0f;
    if (!handmag::Update(f).grabbed)
        return false;
    bool out = false;
    for (float along = 0.002f; along <= 0.15f && !out; along += 0.002f) {
        float move[3];
        WayGrab(along, move);
        const float place[3] = {kGun.grab[0] + move[0], kGun.grab[1] + move[1], kGun.grab[2] + move[2]};
        GloveBy(magposes::kPkpHold, kHandInGlove, place, f.glove);
        out = handmag::Update(f).pulledOut;
    }
    f.wellEmpty = true;
    f.squeeze = 0.0f;
    handmag::Update(f);
    f.squeeze = 1.0f;
    f.shoulder = true;
    handmag::Update(f);
    f.shoulder = false;
    bool seated = false;
    float top[3];
    WayOnTop(top);
    for (int i = 0; i < 80 && !seated; ++i) {
        // (The box's top 3 cm to the right of where its way on starts and 4 cm under it.)
        const float near[3] = {kGun.grab[0] + top[0] - kGun.top[0] - 0.03f, kGun.grab[1] + top[1] - kGun.top[1] - 0.04f, kGun.grab[2] + top[2] - kGun.top[2]};
        GloveBy(magposes::kPkpHold, kHandInGlove, near, f.glove);
        seated = handmag::Update(f).seated;
    }
    f.wellEmpty = false;
    f.squeeze = 0.0f;
    const handmag::Result r = handmag::Update(f);
    return out && seated && r.beltDrawn;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "build", survey = argc > 2 ? argv[2] : "build\\survey";
    char what[1200];

    // ---- p1: the tables ----
    {
        const recoilguns::Row* row = recoilguns::ByName("Weapons/Pecheneg/Pecheneg");
        const bool known = row && row->body == static_cast<uint32_t>(kBody[0]) && row->shape && !std::strcmp(row->shape, "pkp") && recoilguns::ByBody(kBody[0]) == row &&
                           recoilguns::ByShape("pkp") == row;
        std::snprintf(what, sizeof(what), "p1 its recoil row: \"%s\", its body draw %u, its shape %s; found by its blueprint, its body and its shape alike", row ? row->name : "none",
                      row ? row->body : 0, row && row->shape ? row->shape : "none");
        Check(known, what);
        if (row) {
            int same = 0, sameHeld = 0;
            for (const recoilguns::Row& other : recoilguns::kRows) {
                same += other.speed == 560.0f;
                sameHeld += other.speed == 560.0f && other.body != 0;
            }
            std::snprintf(what, sizeof(what), "p1 ... a machine gun, %d in its box (open bolt: no round kept), %.0f m/s (%d of the %zu rows, the M60's and the M82A3's the others; it alone held), %.0f "
                          "a minute; its buzz %.1f %.1f for %.3f s",
                          row->magazine, row->speed, same, std::size(recoilguns::kRows), row->rate, row->rumble[0], row->rumble[1], row->rumble[2]);
            Check(row->kind == recoil::Kind::kMg && row->magazine == 100 && row->speed == 560.0f && same == 3 && sameHeld == 1 && row->rate == 600.0f && row->rumble[0] > 0.0f &&
                      row->rumble[2] > 0.0f,
                  what);
            std::snprintf(what, sizeof(what), "p1 ... its kick its own recording's: %.2f cm back, a rise of %.2f degrees (with the drift's answer its recorded 2.48), held fire's push %.2f of a round's",
                          row->kick.back.peak * 100.0f, row->kick.rise.peak, row->kick.heldShare);
            Check(std::fabs(row->kick.back.peak - 0.0182f) < 1e-5f && row->kick.rise.peak > 1.0f && row->kick.rise.peak < 2.48f && row->kick.heldShare > 0.95f && row->kick.heldShare < 1.15f,
                  what);
            Check(&recoilguns::Default(recoil::Kind::kMg) != row, "p1 ... the machine guns' own row for a gun known only by its kind stays the M240's");
            const pickrules::OpenBolt* bolt = pickrules::OpenBoltOf(pkprows::kShape);
            const int holds = bolt ? pickrules::MagazineHolds(row->magazine, row->magazine, 0, bolt) : 0;
            std::snprintf(what, sizeof(what), "p1 ... an open bolt by its shape (%s): a box holds %d (its row's %d whole), as its own reloads count it (0 and 92 both to 100); the M249's still %d, the "
                          "M240B's still %d",
                          pkprows::kShape, holds, row->magazine, pickrules::MagazineHolds(100, 100, 0, pickrules::OpenBoltOf("m249")),
                          pickrules::MagazineHolds(100, 100, 0, pickrules::OpenBoltOf("m240")));
            Check(bolt && bolt == pickrules::OpenBoltOf(row->shape) && holds == 100 && bolt->box == 100 && pickrules::MagazineHolds(100, 100, 0, pickrules::OpenBoltOf("m249")) == 100 &&
                      pickrules::MagazineHolds(100, 100, 0, pickrules::OpenBoltOf("m240")) == 99,
                  what);
        }
        const gunbodies::Body* body = gunbodies::For("pkp");
        std::snprintf(what, sizeof(what), "p1 its body for free hands and the holster: %s, %.1f kg empty (the Pecheneg's public figure), %d hull points, %d rounds of %.0f g, heavy to the ear",
                      body && body->spec ? body->spec->name : "none", body && body->spec ? body->spec->massEmpty : 0.0f, body && body->spec ? body->spec->hullPoints : 0,
                      body && body->spec ? body->spec->capacity : 0, body && body->spec ? body->spec->massRound * 1000.0f : 0.0f);
        Check(body && body->spec && !std::strcmp(body->spec->name, "pkpgun") && body->spec->keep && body->spec->hullPoints >= 4 && body->spec->capacity == 100 &&
                  body->spec->massEmpty == 8.2f && body->spec->massRound == 0.026f && body->sound == gunbodies::kHeavy,
              what);
        const shaperecipes::Recipe* recipe = shaperecipes::Find(pkprows::kShape);
        const std::string arguments = recipe ? shaperecipes::Arguments(*recipe) : "";
        int turnedRests = 0;
        for (int k = 0; recipe && k < recipe->draws[0].restCount; ++k)
            turnedRests += recipe->draws[0].rest[k].turned;
        std::snprintf(what, sizeof(what), "p1 its collision shape's recipe: gunshape ... 0.003 %s (its body from 2310, its frame bone 7, its box and its charging handle left out; its cover, flaps and "
                      "trigger kept as they rest, unturned: %d rests, %d turned; its belt a draw of its own)",
                      arguments.c_str(), recipe ? recipe->draws[0].restCount : 0, turnedRests);
        Check(!std::strcmp(pkprows::kShape, "pkp") && recipe && arguments == "23298@2310 7 0,4" && recipe->draws[0].vbOffset == 57888 && recipe->drawCount == 1 &&
                  recipe->draws[0].restCount == 6 && turnedRests == 0 && recipe->draws[0].bones == 9,
              what);
        std::snprintf(what, sizeof(what), "p1 its name in the logs: %s (row %d, after the AT4's %d)", gunprofile::NameOf("pkp"), gunprofile::RowOf("pkp"), gunprofile::kAt4Row);
        Check(!std::strcmp(gunprofile::NameOf("pkp"), "PKP") && gunprofile::RowOf("pkp") > gunprofile::kAt4Row, what);
        const loose::Spec* box = loose::SpecFor("pkp");
        float boxOff = 1.0f;
        if (box) {
            boxOff = 0.0f;
            for (int k = 0; k < 3; ++k)
                boxOff = std::max({boxOff, std::fabs(box->boxMin[k] - kBoxMin[k]), std::fabs(box->boxMax[k] - kBoxMax[k])});
        }
        std::snprintf(what, sizeof(what), "p1 its box let go of: loose's \"pkp\", %.1f mm off the row's box, %.2f kg empty and %.0f g a round, %d of them; it lands as metal",
                      boxOff * 1000.0f, box ? box->massEmpty : 0.0f, box ? box->massRound * 1000.0f : 0.0f, box ? box->capacity : 0);
        Check(box && boxOff < 1e-6f && box->capacity == 100 && box->massEmpty == 0.60f && box->massRound == 0.026f && magsound::Of(pkprows::kShape) == magsound::Stuff::kMetal, what);
        // Its casings' emitter (src/casings.cpp's table, read as text: the file is the game side's, not linked here).
        {
            std::ifstream source("src\\casings.cpp");
            const std::string text{std::istreambuf_iterator<char>(source), std::istreambuf_iterator<char>()};
            const std::string key = "{\"pkp\", casings::k762, {";
            const size_t at = text.find(key);
            float port[3] = {}, turn[3] = {}, zoomTurn[3] = {};
            const int got = at == std::string::npos ? 0
                                                    : std::sscanf(text.c_str() + at + key.size(), "%ff, %ff, %ff}, {%ff, %ff, %ff}, {%ff, %ff, %ff}", &port[0], &port[1], &port[2], &turn[0], &turn[1],
                                                                  &turn[2], &zoomTurn[0], &zoomTurn[1], &zoomTurn[2]);
            if (text.empty()) {
                Skip("p1 src\\casings.cpp isn't here (not run from the project's folder): its casings");
            } else {
                std::snprintf(what, sizeof(what), "p1 its casings: 7.62 from (%.3f %.3f %.3f) (the row's port %.1f mm off), turned %.1f %.1f %.1f degrees (the _762's throw out to its left), the same "
                              "zoomed",
                              port[0], port[1], port[2], Dist(port, kPort) * 1000.0f, turn[0], turn[1], turn[2]);
                Check(got == 9 && Dist(port, kPort) < 1e-6f && turn[2] == -155.0f && zoomTurn[2] == -155.0f && turn[0] == zoomTurn[0] && kPort[0] > 0.0f, what);
            }
        }
        const float numbers[] = {kGun.top[1], kGun.top[2], kGun.grab[0], kGun.grab[1], kGun.grab[2], kGun.hook[2], kGun.mouth, kGun.lever[0], kGun.lever[1], kGun.lever[2], kGun.leverTravel,
                                 kBelt.lidHinge[0], kBelt.lidHinge[1], kBelt.lidOpen, kBelt.lidGrab[0], kBelt.lidGrab[1], kBelt.lidGrab[2], kBelt.feed[0], kBelt.feed[1], kBelt.feed[2],
                                 kBelt.anchor[0], kBelt.anchor[1], kBelt.anchor[2], kBelt.pieceZ, kSights[1], kSights[2], kUp, kEyeZ, kRearZ};
        Check(std::all_of(std::begin(numbers), std::end(numbers), [](float f) { return std::isfinite(f); }) && kGun.mouth < 0.0f && kRearZ > kEyeZ && kGun.way == &kWay && !kGun.straight &&
                  kGun.wide && kGun.grabTo <= kBoxMin[0] + 0.001f && kGun.grab[0] > kBoxMax[0] && kGun.noKnock && !kGun.button && !kGun.boltCatch && kGun.rockIn == 0.0f &&
                  kWay.offCount >= 3 && kWay.onCount >= 3 && kWay.free > 0.0f && kWay.free < kWay.off[kWay.offCount - 1][0],
              "p1 its rows' numbers finite; no magazine button, nothing knocks its box out, the box off and on along its own reload's way (not straight, not rocked), "
              "taken anywhere along it from its left end to its bail, no bolt catch");
        bool order = kBelt.reversed && kBelt.carries;
        for (int i = 0; i < 5; ++i)
            order = order && handmag::BeltOrder(kBelt, i) == 4 - i && handmag::BeltOrder(kBelt, handmag::BeltOrder(kBelt, i)) == i;
        order = order && !m249rows::kBelt.reversed && handmag::BeltOrder(m249rows::kBelt, 3) == 3 && handmag::BeltOrder(handmag::BeltGun{}, 1) == 1;
        std::snprintf(what, sizeof(what), "p1 its belt numbered from the box (bone 4 the free end, in the feed; bone 0 into the box): BeltOrder turns round both ways %d; its anchor on the gun's right "
                      "(x %.3f) so in the hand it bows out to the right; the M249's and the M240's numbered from the feed as before",
                      order, kBelt.anchor[0]);
        Check(order && kBelt.anchor[0] < 0.0f && kBelt.feed[0] > kBelt.anchor[0], what);
    }

    // ---- p2: the mesh ----
    const Dump body = Load(dir, kBody[0], kBody[1], kFrameBone);
    if (!body.read) {
        Skip("p2 no dump of its body (build\\mesh_23298_2310_*): its mesh");
    } else {
        Box bones[9];
        int others = 0;
        for (const Vertex& v : body.vertices)
            if (v.bone >= 0 && v.bone < 9)
                bones[v.bone].Add(v.at);
            else
                ++others;
        std::snprintf(what, sizeof(what), "p2 its body: %d vertices on the frame (bone 7), %d the box's (0), %d the cases' flap's (1), %d the feed arm's (2), %d under the cover (3), %d the "
                      "handle's (4), %d the links' flap's (5), %d the trigger's (6), %d the cover's (8), %d on no bone of these; %.3f m long",
                      bones[7].count, bones[0].count, bones[1].count, bones[2].count, bones[3].count, bones[4].count, bones[5].count, bones[6].count, bones[8].count, others,
                      bones[7].hi[2] - bones[7].lo[2]);
        Check(bones[7].count > 4000 && bones[0].count > 1000 && bones[4].count > 100 && bones[8].count > 1500 && bones[1].count > 50 && bones[5].count > 50 && others == 0 &&
                  bones[7].hi[2] - bones[7].lo[2] > 1.19f && bones[7].hi[2] - bones[7].lo[2] < 1.23f,
              what);
        // The handle's knob: what of it stands out on the right furthest (the outer centimetre).
        Box knob;
        for (const Vertex& v : body.vertices)
            if (v.bone == kHandleBone && v.at[0] < -0.045f)
                knob.Add(v.at);
        const float knobMid[3] = {(knob.lo[0] + knob.hi[0]) / 2, (knob.lo[1] + knob.hi[1]) / 2, (knob.lo[2] + knob.hi[2]) / 2};
        std::snprintf(what, sizeof(what), "p2 its handle on the right (x %.3f to %.3f): its knob x %.3f to %.3f, y %.3f to %.3f, z %.3f to %.3f, its middle %.1f mm from the row's lever (%.4f %.4f %.4f)",
                      bones[4].lo[0], bones[4].hi[0], knob.lo[0], knob.hi[0], knob.lo[1], knob.hi[1], knob.lo[2], knob.hi[2], Dist(knobMid, kGun.lever) * 1000.0f, kGun.lever[0], kGun.lever[1],
                      kGun.lever[2]);
        Check(knob.count >= 20 && bones[4].hi[0] < -0.02f && Dist(knobMid, kGun.lever) < 0.002f, what);
        // The feed cover: its hinge within its front end; the hand's grab over its top's left half.
        const Box& cover = bones[8];
        std::snprintf(what, sizeof(what), "p2 its feed cover: x %.3f to %.3f, y %.3f to %.3f, z %.3f to %.3f; the row's hinge at z %.4f within its front end; its grab (%.4f %.4f %.4f) over its top, "
                      "on its left, %.1f cm ahead of its back end",
                      cover.lo[0], cover.hi[0], cover.lo[1], cover.hi[1], cover.lo[2], cover.hi[2], kBelt.lidHinge[1], kBelt.lidGrab[0], kBelt.lidGrab[1], kBelt.lidGrab[2],
                      (kBelt.lidGrab[2] - cover.lo[2]) * 100.0f);
        Check(kBelt.lidHinge[1] > cover.hi[2] - 0.02f && kBelt.lidHinge[1] < cover.hi[2] && kBelt.lidGrab[2] > cover.lo[2] && kBelt.lidGrab[2] < cover.hi[2] - 0.1f && kBelt.lidGrab[0] > 0.0f &&
                  kBelt.lidGrab[0] < cover.hi[0] + 0.01f && kBelt.lidGrab[1] > cover.hi[1],
              what);
        // The iron sights: the front post's tip (round, in its hood) on the row's line, and the game's own zoomed eye on it, looking along it; no rear sight.
        float postTop = -1e9f;
        for (const Vertex& v : body.vertices)
            if (v.bone == kFrameBone && std::fabs(v.at[0]) < 0.0025f && v.at[2] > 1.10f && v.at[2] < 1.15f)
                postTop = std::max(postTop, v.at[1]);
        float postZ = 0.0f;
        int postN = 0;
        for (const Vertex& v : body.vertices)
            if (v.bone == kFrameBone && std::fabs(v.at[0]) < 0.0025f && v.at[2] > 1.10f && v.at[2] < 1.15f && v.at[1] > postTop - 0.0005f)
                postZ += v.at[2], ++postN;
        postZ = postN ? postZ / postN : 0.0f;
        const float lineSlope = std::tan(kUp * kPi / 180.0f);
        const float gameDown = std::atan2(-kZoomedForward[1], kZoomedForward[2]) * 180.0f / kPi, gameSide = std::atan2(-kZoomedForward[0], kZoomedForward[2]) * 180.0f / kPi;
        const float offLine = std::hypot(gameDown - kUp, gameSide);
        std::snprintf(what, sizeof(what), "p2 its sights: the front post's tip y %.4f at z %.3f, %.2f mm off the row's line; the game's zoomed eye (0 %.4f %.4f) %.2f mm off it, looking %.2f degrees down "
                      "and %.2f right (%.2f degrees off the line); the line at the muzzle y %.4f (the row's %.4f), %.2f degrees down",
                      postTop, postZ, std::fabs(LineY(postZ) - postTop) * 1000.0f, kZoomedEye[1], kZoomedEye[2], std::fabs(LineY(kZoomedEye[2]) - kZoomedEye[1]) * 1000.0f, gameDown,
                      gameSide, offLine, LineY(kSights[2]), kSights[1], kUp);
        Check(postN > 2 && std::fabs(LineY(postZ) - postTop) < 0.0006f && std::fabs(LineY(kZoomedEye[2]) - kZoomedEye[1]) < 0.0006f && offLine < 0.1f && kSights[0] == 0.0f &&
                  kSights[2] == kMuzzle[2] && std::fabs(kEyeZ - kZoomedEye[2]) < 0.001f && lineSlope > 0.0f,
              what);
        // The first of the gun (what the shape keeps: not its box, not its handle) within 1 cm of the line going forward from the eye: the row's rear z.
        float firstNear = 1e9f, cover0 = 1e9f;
        for (const Vertex& v : body.vertices) {
            if (v.bone == kBoxBone || v.bone == kHandleBone || v.at[2] < kZoomedEye[2])
                continue;
            const float off = std::hypot(v.at[0], v.at[1] - LineY(v.at[2]));
            if (off < 0.01f)
                firstNear = std::min(firstNear, v.at[2]);
            if (v.bone == kCoverBone)
                cover0 = std::min(cover0, off);
        }
        std::snprintf(what, sizeof(what), "p2 the first of the gun within 1 cm of its sights' line ahead of the eye at z %.4f (the row's rear %.3f): the front sight's hood; the cover's top %.1f mm "
                      "under the line at its nearest (no rear sight stands on it)",
                      firstNear, kRearZ, cover0 * 1000.0f);
        Check(std::fabs(firstNear - kRearZ) < 0.001f && cover0 > 0.015f, what);
        // Its muzzle, and its port where its shell effect is: by the cases' flap on its left.
        Box tip;
        for (const Vertex& v : body.vertices)
            if (v.bone == kFrameBone && v.at[2] > bones[7].hi[2] - 0.001f)
                tip.Add(v.at);
        const float tipMid[3] = {(tip.lo[0] + tip.hi[0]) / 2, (tip.lo[1] + tip.hi[1]) / 2, tip.hi[2]};
        std::snprintf(what, sizeof(what), "p2 its muzzle's middle (%.4f %.4f %.4f), the row's (0 %.4f %.4f); the port (%.3f %.3f %.3f) by the cases' flap on its left (x %.3f to %.3f, y %.3f to %.3f, "
                      "z %.3f to %.3f)",
                      tipMid[0], tipMid[1], tipMid[2], kMuzzle[1], kMuzzle[2], kPort[0], kPort[1], kPort[2], bones[1].lo[0], bones[1].hi[0], bones[1].lo[1], bones[1].hi[1], bones[1].lo[2],
                      bones[1].hi[2]);
        Check(Dist(tipMid, kMuzzle) < 0.002f && std::fabs(kPort[0] - bones[1].lo[0]) < 0.003f && kPort[1] > bones[1].lo[1] && kPort[1] < bones[1].hi[1] && kPort[2] > bones[1].lo[2] &&
                  kPort[2] < bones[1].hi[2],
              what);
        // The belt: its five pieces in their own bones' space as the row has them (by bone: 0 to 3 standing, their tops the ends nearer the feed; 4 across the
        // tray, its left end the one nearer the feed).
        const Dump belt = Load(dir, kBeltDraw[0], kBeltDraw[1], -1);
        if (!belt.read) {
            Skip("p2 no dump of its belt (mesh_2310_0_*)");
        } else {
            Box pieces[5];
            int beltOthers = 0;
            for (const Vertex& v : belt.vertices)
                if (v.bone < 5)
                    pieces[v.bone].Add(v.own);
                else
                    ++beltOthers;
            float worst = 0.0f, zWorst = 0.0f;
            bool shapes = true;
            for (int b = 0; b < 5; ++b) {
                const float* p = kBelt.pieces[b];
                const bool across = p[1] == p[3];
                shapes = shapes && across == (b == 4);
                if (across)  // (its feed end its left: the higher x)
                    worst = std::max({worst, std::fabs(pieces[b].hi[0] - p[0]), std::fabs(pieces[b].lo[0] - p[2]), std::fabs((pieces[b].lo[1] + pieces[b].hi[1]) / 2 - p[1])});
                else
                    worst = std::max({worst, std::fabs(pieces[b].hi[1] - p[1]), std::fabs(pieces[b].lo[1] - p[3]), std::fabs((pieces[b].lo[0] + pieces[b].hi[0]) / 2 - p[0])});
                zWorst = std::max(zWorst, std::fabs((pieces[b].lo[2] + pieces[b].hi[2]) / 2 - kBelt.pieceZ));
            }
            std::snprintf(what, sizeof(what), "p2 its belt (2310/0): five pieces of %d, %d, %d, %d, %d vertices (%d on another bone), 0 to 3 standing and 4 across, each where the row has it to %.2f mm, "
                          "their rounds' middle %.2f mm off the row's z",
                          pieces[0].count, pieces[1].count, pieces[2].count, pieces[3].count, pieces[4].count, beltOthers, worst * 1000.0f, zWorst * 1000.0f);
            Check(beltOthers == 0 && shapes && pieces[4].count > 0 && worst < 0.0006f && zWorst < 0.0015f, what);
            // The belt lying in the feed (the laid table): its free end's piece's middle is the row's feed, its last piece's end in the box the row's anchor.
            float feedAt[3], anchorAt[3];
            const float* p4 = kBelt.pieces[4];
            const float* p0 = kBelt.pieces[0];
            const float feedMid[3] = {(p4[0] + p4[2]) / 2, (p4[1] + p4[3]) / 2, kBelt.pieceZ}, boxEnd[3] = {p0[2], p0[3], kBelt.pieceZ};
            Apply(kBelt.laid[4], feedMid, feedAt);
            Apply(kBelt.laid[0], boxEnd, anchorAt);
            std::snprintf(what, sizeof(what), "p2 laid: its piece across the tray's middle (%.4f %.4f %.4f), %.1f mm from the row's feed; its last piece's end (%.4f %.4f %.4f), %.1f mm from the row's "
                          "anchor, by the box's top on the right",
                          feedAt[0], feedAt[1], feedAt[2], Dist(feedAt, kBelt.feed) * 1000.0f, anchorAt[0], anchorAt[1], anchorAt[2], Dist(anchorAt, kBelt.anchor) * 1000.0f);
            Check(Dist(feedAt, kBelt.feed) < 0.0015f && Dist(anchorAt, kBelt.anchor) < 0.0015f, what);
        }
        // The box: a bone of the body (0): its own vertices' box as the row's; its bone at rest on the frame, unturned; on the frame its top under the gun, its
        // front, its back; the fitted grab beyond its left end, the far end of its line at its bail.
        Box own;
        for (const Vertex& v : body.vertices)
            if (v.bone == kBoxBone)
                own.Add(v.own);
        float boxOff = 0.0f;
        for (int k = 0; k < 3; ++k)
            boxOff = std::max({boxOff, std::fabs(own.lo[k] - kBoxMin[k]), std::fabs(own.hi[k] - kBoxMax[k])});
        float frameInverse[12], rest[12];
        InvertRigid(body.Bone(kFrameBone), frameInverse);
        Compose(frameInverse, body.Bone(kBoxBone), rest);
        const float none12[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
        float topY = -1e9f;
        for (const Vertex& v : body.vertices)
            if (v.bone == kBoxBone && std::fabs(v.at[0]) < 0.03f)
                topY = std::max(topY, v.at[1]);
        const Box& onFrame = bones[0];
        std::snprintf(what, sizeof(what), "p2 its box (bone 0): %d vertices, its own box %.1f mm off the row's; at rest on the frame at (%.4f %.4f %.4f), turned %.3f degrees; on the frame x %.3f to "
                      "%.3f, y %.3f to %.3f, z %.3f to %.3f: its top under the gun y %.4f (the row's %.3f), its front z %.4f (the hook's %.3f), its back z %.4f (the paddle's %.3f); the grab %.1f cm "
                      "beyond its left end, its line to its bail (%.3f)",
                      own.count, boxOff * 1000.0f, rest[3], rest[7], rest[11], TurnBetween(rest, none12), onFrame.lo[0], onFrame.hi[0], onFrame.lo[1], onFrame.hi[1], onFrame.lo[2], onFrame.hi[2],
                      topY, kGun.top[1], onFrame.hi[2], kGun.hook[2], onFrame.lo[2], kGun.paddle[2], (kGun.grab[0] - onFrame.hi[0]) * 100.0f, kGun.grabTo);
        Check(own.count > 1000 && boxOff < 0.001f && std::fabs(rest[3] - kBoxRest[0]) < 0.0005f && std::fabs(rest[7] - kBoxRest[1]) < 0.0005f && std::fabs(rest[11] - kBoxRest[2]) < 0.0005f &&
                  TurnBetween(rest, none12) < 0.05f && std::fabs(topY - kGun.top[1]) < 0.0015f && std::fabs(onFrame.hi[2] - kGun.hook[2]) < 0.002f &&
                  std::fabs(onFrame.lo[2] - kGun.paddle[2]) < 0.004f && kGun.grab[0] > onFrame.hi[0] && kGun.grab[0] < onFrame.hi[0] + 0.04f && std::fabs(kGun.grabTo - onFrame.lo[0]) < 0.001f,
              what);
    }

    // ---- p3: the recordings ----
    const Glove glove = LoadGloveParts(dir);
    const std::wstring shapePath = std::wstring(dir.begin(), dir.end()) + L"\\guns\\pkp.bin";
    const bool haveShape = gunshape::Load(shapePath.c_str());
    auto shapeAt = [](const float* p) {
        float d = 0.05f;
        return gunshape::Distance(p, d) ? d : 0.05f;
    };
    struct WayFrame {
        int ms;
        float moved[12];
    };
    std::vector<WayFrame> wayFrames;
    float openCover[12] = {}, openOthers[2][12] = {}, restCover[12] = {}, restOthers[2][12] = {};  // (the empty reload's cover resting open, and bones 3 and 5 then)
    bool haveOpen = false;
    for (const char* which : {"partial", "empty"}) {
        const std::string path = survey + "\\PKP_" + which + ".bin";
        const bool empty = !std::strcmp(which, "empty");
        long frames = 0, depths = 0, wrongDepth = 0, belts = 0;
        float worstDepth = 0.0f, worstDepthTurn = 0.0f;
        float coverMost = 0.0f, hingeY = 0.0f, hingeZ = 0.0f, hingeOffAxis = 0.0f, hingeSpread = 0.0f, hingeY0 = 0.0f, hingeZ0 = 0.0f;
        int hingeN = 0;
        std::vector<float> openAngles;
        float othersWorst[2] = {}, armMost = 0.0f;  // bones 3 and 5 against the cover's move while it opens (the empty reload); the feed arm (2) its own way
        float handleMost = 0.0f, handleAside = 0.0f, handleTurn = 0.0f;
        float coverRest[12], handleRest[12], otherRest[3][12];
        bool haveRest = false;
        float laidWorst = 0.0f, laidTurn = 0.0f, hangWorst = 0.0f, hangTurn = 0.0f;
        int laidN = 0, hangN = 0, squashed = 0;
        float palms[2][3] = {}, palmWorst = 0.0f;
        int palmN = 0;
        float leftRest[3] = {}, leftMost = 0.0f;
        bool haveLeftRest = false;
        float lidHand = 1e9f, boxHand = 1e9f, boxHandBeyond = -1.0f;
        struct Shot {
            int ms;
            float back, rise;
            bool done;
        };
        std::vector<Shot> shots;
        float lastFrame[12], burstRest[12];
        int lastLoaded = -1, lastRoundMs = -100000;
        bool haveLast = false;
        float boxRestOn[12];
        bool haveBoxRest = false;
        const bool read = Frames(path, [&](const std::vector<Record>& group) {
            std::vector<const Record*> eyes;
            for (const Record& r : group)
                if (r.count == kBody[0] && r.start == kBody[1] && r.Bones() > kCoverBone)
                    eyes.push_back(&r);
            if (eyes.empty())
                return;
            ++frames;
            const Record& first = *eyes.front();
            float inverse[12], coverOn[12], handleOn[12], otherOn[3][12];
            InvertRigid(first.Bone(kFrameBone), inverse);
            Compose(inverse, first.Bone(kCoverBone), coverOn);
            Compose(inverse, first.Bone(kHandleBone), handleOn);
            for (int k = 0; k < 3; ++k)
                Compose(inverse, first.Bone(pkprows::kCoverAlso[k]), otherOn[k]);
            // The depth pass of each eye against the same eye's belt and body: 0-4 the belt's, 5-13 the body's 0-8.
            for (size_t e = 0; e < eyes.size(); ++e) {
                const Record* d = Nth(group, kDepth[0], kDepth[1], static_cast<int>(e));
                const Record* b = Nth(group, kBeltDraw[0], kBeltDraw[1], static_cast<int>(e));
                if (!d || !b || d->Bones() < 14 || b->Bones() < kBeltBones) {
                    wrongDepth += d != nullptr;
                    continue;
                }
                ++depths;
                for (int k = 0; k < 14; ++k) {
                    const float* expect = k < kBeltBones ? b->Bone(k) : eyes[e]->Bone(k - kBeltBones);
                    worstDepth = std::max(worstDepth, Apart(d->Bone(k), expect));
                    worstDepthTurn = std::max(worstDepthTurn, TurnBetween(d->Bone(k), expect));
                }
            }
            // The belt (the first eye's), on the frame.
            if (const Record* b = Nth(group, kBeltDraw[0], kBeltDraw[1], 0); b && b->Bones() >= kBeltBones) {
                ++belts;
                float on[5][12];
                for (int k = 0; k < kBeltBones; ++k) {
                    squashed += Squashed(b->Bone(k));
                    Compose(inverse, b->Bone(k), on[k]);
                }
                const bool still = empty && first.ms >= 4610 && first.ms <= 15985;
                const bool hanging = empty && first.ms >= 3985 && first.ms <= 4032;
                for (int k = 0; k < kBeltBones; ++k) {
                    if (still)
                        laidWorst = std::max(laidWorst, Apart(on[k], kBelt.laid[k])), laidTurn = std::max(laidTurn, TurnBetween(on[k], kBelt.laid[k]));
                    if (hanging)
                        hangWorst = std::max(hangWorst, Apart(on[k], kBelt.hanging[k])), hangTurn = std::max(hangTurn, TurnBetween(on[k], kBelt.hanging[k]));
                }
                laidN += still, hangN += hanging;
            }
            if (!haveRest) {
                std::memcpy(coverRest, coverOn, sizeof(coverRest));
                std::memcpy(handleRest, handleOn, sizeof(handleRest));
                std::memcpy(otherRest, otherOn, sizeof(otherRest));
                haveRest = true;
            }
            // The cover: its turn from rest and the point it turns about; the other three bones of it against its move.
            float moved[12], restInverse[12];
            InvertRigid(coverRest, restInverse);
            Compose(coverOn, restInverse, moved);
            const float angle = std::atan2(moved[9], moved[5]) * 180.0f / kPi;
            coverMost = std::max(coverMost, angle);
            if (angle > 20.0f) {
                const float c = moved[5], s = moved[9], a = 1 - c, det = a * a + s * s;
                const float y = (a * moved[7] - s * moved[11]) / det, z = (s * moved[7] + a * moved[11]) / det;
                if (!hingeN)
                    hingeY0 = y, hingeZ0 = z;
                hingeY += y, hingeZ += z, ++hingeN;
                hingeSpread = std::max({hingeSpread, std::fabs(y - hingeY0), std::fabs(z - hingeZ0)});
                hingeOffAxis = std::max({hingeOffAxis, std::fabs(moved[1]), std::fabs(moved[2]), std::fabs(moved[4]), std::fabs(moved[8]), std::fabs(moved[3])});
            }
            if (angle > 80.0f)
                openAngles.push_back(angle);
            if (empty) {
                for (int k = 0; k < 3; ++k) {
                    float otherMoved[12], otherInverse[12];
                    InvertRigid(otherRest[k], otherInverse);
                    Compose(otherOn[k], otherInverse, otherMoved);
                    if (k < 2)
                        othersWorst[k] = std::max({othersWorst[k], Apart(otherMoved, moved) * 1000.0f, TurnBetween(otherMoved, moved)});
                    else
                        armMost = std::max(armMost, TurnBetween(otherMoved, moved));
                }
                // The cover resting open (93 degrees: 3.0 to 5.06 s), and its other bones then.
                if (!haveOpen && first.ms >= 3500 && std::fabs(angle - kBelt.lidOpen * 180.0f / kPi) < 0.3f) {
                    std::memcpy(openCover, coverOn, sizeof(openCover));
                    std::memcpy(restCover, coverRest, sizeof(restCover));
                    for (int k = 0; k < 2; ++k)
                        std::memcpy(openOthers[k], otherOn[k], sizeof(openOthers[k])), std::memcpy(restOthers[k], otherRest[k], sizeof(restOthers[k]));
                    haveOpen = true;
                }
            }
            // The handle: back along z, nothing aside.
            handleMost = std::max(handleMost, handleRest[11] - handleOn[11]);
            handleAside = std::max({handleAside, std::fabs(handleOn[3] - handleRest[3]), std::fabs(handleOn[7] - handleRest[7])});
            handleTurn = std::max(handleTurn, TurnBetween(handleOn, handleRest));
            // The gloves (the first eye's): the palms at rest; the left hand all through the reload; the game's right hand on the cover and on the box, mirrored.
            if (const Record* g = Nth(group, kGloves[0], kGloves[1], 0); g && g->Bones() > kRightRoot) {
                float on[12], at[2][3], leftMiddle[3];
                Compose(inverse, g->Bone(kRightRoot), on);
                Apply(on, kRightPalmPoint, at[0]);
                Compose(inverse, g->Bone(kLeftPalmBone), on);
                Apply(on, kLeftPalmPoint, at[1]);
                Apply(on, kHandInGlove, leftMiddle);
                if (empty && first.ms >= 6400 && first.ms <= 15000) {
                    for (int s = 0; s < 2; ++s)
                        for (int k = 0; k < 3; ++k)
                            palms[s][k] += at[s][k];
                    ++palmN;
                    palmWorst = std::max({palmWorst, Dist(at[0], kRightPalm), Dist(at[1], kLeftPalm)});
                }
                if (empty && first.ms <= 6000) {
                    if (!haveLeftRest)
                        std::memcpy(leftRest, leftMiddle, sizeof(leftRest)), haveLeftRest = true;
                    leftMost = std::max(leftMost, Dist(leftMiddle, leftRest));
                }
                float rightOn[12], mirrored[12], middle[3];
                Compose(inverse, g->Bone(kRightPalmBone), rightOn);
                Mirror(rightOn, mirrored);
                Apply(mirrored, kHandInGlove, middle);  // (the right glove's own middle, mirrored onto the left's)
                if (empty && first.ms >= 1600 && first.ms <= 1770)
                    lidHand = std::min(lidHand, Dist(middle, kBelt.lidGrab));
                if (empty && first.ms >= 2300 && first.ms <= 2560) {
                    boxHand = std::min(boxHand, Dist(middle, kGun.grab));
                    boxHandBeyond = std::max(boxHandBeyond, middle[0]);
                }
            }
            // The box's way (the empty reload).
            if (empty) {
                float boxOn[12];
                Compose(inverse, first.Bone(kBoxBone), boxOn);
                if (!haveBoxRest)
                    std::memcpy(boxRestOn, boxOn, sizeof(boxRestOn)), haveBoxRest = true;
                if (first.ms >= 2500 && first.ms <= 4100) {
                    WayFrame w;
                    w.ms = first.ms;
                    float boxInverse[12];
                    InvertRigid(boxRestOn, boxInverse);
                    Compose(boxOn, boxInverse, w.moved);
                    wayFrames.push_back(w);
                }
            }
            // The kick: a burst begins with a round counted half a second or more after the last; its first round's push against where the frame rested the
            // frame before, up to and with the frame the next round is counted in.
            if (haveLast) {
                const bool round = first.loaded < lastLoaded;
                if (round && first.ms - lastRoundMs > 500) {
                    std::memcpy(burstRest, lastFrame, sizeof(burstRest));
                    shots.push_back({first.ms, 0.0f, 0.0f, false});
                }
                if (!shots.empty() && !shots.back().done) {
                    float restInv[12], m[12];
                    InvertRigid(burstRest, restInv);
                    Compose(restInv, first.Bone(kFrameBone), m);
                    shots.back().back = std::max(shots.back().back, -m[11]);
                    shots.back().rise = std::max(shots.back().rise, std::atan2(m[6], m[10]) * 180.0f / kPi);
                    if (round && first.ms != shots.back().ms)
                        shots.back().done = true;
                }
                if (round)
                    lastRoundMs = first.ms;
            }
            std::memcpy(lastFrame, first.Bone(kFrameBone), sizeof(lastFrame));
            lastLoaded = first.loaded;
            haveLast = true;
        });
        if (!read || !frames) {
            std::snprintf(what, sizeof(what), "p3 %s: no recording", path.c_str());
            Skip(what);
            continue;
        }
        std::snprintf(what, sizeof(what), "p3 PKP_%s: %ld frames, %ld depth passes (%ld without the eye's belt or body to hold them to), %ld belt draws: the depth pass's 0-4 the belt's, 5-13 the "
                      "body's 0-8, to %.2f mm and %.2f degrees",
                      which, frames, depths, wrongDepth, belts, worstDepth * 1000.0f, worstDepthTurn);
        Check(depths > 1000 && wrongDepth == 0 && worstDepth < 0.0005f && worstDepthTurn < 0.1f && belts > 500, what);
        std::snprintf(what, sizeof(what), "p3 ... its handle drawn %.2f cm straight back at most (%.2f mm aside, turned %.2f degrees; the row's travel %.1f); the belt never hidden by the game's count "
                      "(%d squashed pieces)",
                      handleMost * 100.0f, handleAside * 1000.0f, handleTurn, kGun.leverTravel * 100.0f, squashed);
        Check(std::fabs(handleMost - kGun.leverTravel) < 0.002f && handleAside < 0.001f && handleTurn < 0.2f && squashed == 0, what);
        hingeY /= std::max(hingeN, 1), hingeZ /= std::max(hingeN, 1);
        std::sort(openAngles.begin(), openAngles.end());
        const float resting = openAngles.empty() ? 0.0f : openAngles[openAngles.size() / 2];
        std::snprintf(what, sizeof(what), "p3 ... the cover turns about x alone (%.4f off) through (y %.4f, z %.4f) in %d frames (within %.2f mm), to %.1f degrees at most as it pops open, resting at "
                      "%.2f (the median of %zu frames past 80); the row's hinge (%.4f %.4f), %.1f degrees",
                      hingeOffAxis, hingeY, hingeZ, hingeN, hingeSpread * 1000.0f, coverMost, resting, openAngles.size(), kBelt.lidHinge[0], kBelt.lidHinge[1], kBelt.lidOpen * 180.0f / kPi);
        Check(hingeN > 50 && hingeOffAxis < 0.001f && hingeSpread < 0.0005f && std::fabs(hingeY - kBelt.lidHinge[0]) < 0.0005f && std::fabs(hingeZ - kBelt.lidHinge[1]) < 0.0005f &&
                  std::fabs(resting - kBelt.lidOpen * 180.0f / kPi) < 0.5f && coverMost > resting,
              what);
        if (empty) {
            std::snprintf(what, sizeof(what), "p3 ... its other cover bones with it all through the empty reload: under it (3) to %.2f (mm or degrees), the links' flap (5) to %.2f; the feed arm (2) "
                          "turns its own way too, up to %.1f degrees off it (carried with the cover by hand)",
                          othersWorst[0], othersWorst[1], armMost);
            Check(othersWorst[0] < 0.3f && othersWorst[1] < 1.5f && armMost > 10.0f, what);
            for (int s = 0; s < 2; ++s)
                for (int k = 0; k < 3; ++k)
                    palms[s][k] /= std::max(palmN, 1);
            std::snprintf(what, sizeof(what), "p3 ... the belt laid (%d frames, 4.61 to 15.99 s) as the table has it to %.2f mm and %.2f degrees; hanging out of the fresh box (%d frames, 3.985 to 4.032 s) "
                          "to %.2f mm and %.2f degrees; the palms at rest (%d frames) right (%.3f %.3f %.3f) left (%.3f %.3f %.3f), the row's to %.1f mm",
                          laidN, laidWorst * 1000.0f, laidTurn, hangN, hangWorst * 1000.0f, hangTurn, palmN, palms[0][0], palms[0][1], palms[0][2], palms[1][0], palms[1][1], palms[1][2],
                          palmWorst * 1000.0f);
            Check(laidN > 600 && laidWorst < 0.0005f && laidTurn < 0.25f && hangN >= 4 && hangWorst < 0.002f && hangTurn < 1.5f && palmN > 100 && palmWorst < 0.003f, what);
            std::snprintf(what, sizeof(what), "p3 ... its own reload all its right hand's: the left on the handguard to %.1f mm all through it; the right on the cover's top (1.6 to 1.77 s), mirrored "
                          "onto the left, %.1f cm from the row's grab on the cover; on the box (2.3 to 2.56 s), mirrored, %.1f cm from the fitted grab, out at x %.3f beyond the box's left end",
                          leftMost * 1000.0f, lidHand * 100.0f, boxHand * 100.0f, boxHandBeyond);
            Check(haveLeftRest && leftMost < 0.004f && lidHand < 0.03f && boxHand < 0.045f && boxHandBeyond > kBoxMax[0], what);
        } else {
            const recoilguns::Row* row = recoilguns::ByBody(kBody[0]);
            std::snprintf(what, sizeof(what), "p3 ... its kick: %zu bursts from rest; the first rounds %.2f and %.2f cm back, %.2f and %.2f degrees up (until the next round's count); the row's back %.2f cm",
                          shots.size(), shots.size() > 0 ? shots[0].back * 100.0f : 0.0f, shots.size() > 1 ? shots[1].back * 100.0f : 0.0f, shots.size() > 0 ? shots[0].rise : 0.0f,
                          shots.size() > 1 ? shots[1].rise : 0.0f, row ? row->kick.back.peak * 100.0f : 0.0f);
            Check(shots.size() == 2 && row && std::fabs(shots[0].back - row->kick.back.peak) < 0.0006f && std::fabs(shots[1].back - shots[0].back) < 0.002f && shots[0].rise > 2.2f &&
                      shots[0].rise < 2.7f,
                  what);
        }
    }
    // The box's way off and on (pkp_way.h) is the empty reload's own: each of its rows (the seat aside) one of the recording's frames, to a hundredth of a
    // millimetre; off: its grab point (its left end) slides level to the gun's right while its right end tips down, free 8.2 cm along, then right and down;
    // on: from the right and below, swung up into the seat, the seat itself last.
    if (wayFrames.size() < 50) {
        Skip("p3 the box's way: no empty reload recorded");
    } else {
        auto nearestFrame = [&](const float* pose) {
            float best = 1e9f;
            for (const WayFrame& w : wayFrames) {
                float worst = 0.0f;
                for (int k = 0; k < 12; ++k)
                    worst = std::max(worst, std::fabs(w.moved[k] - pose[k]));
                best = std::min(best, worst);
            }
            return best;
        };
        const float none12[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
        float offWorst = 0.0f, turnAtFree = 0.0f, rightAtFree = 0.0f, riseAtFree = 0.0f, turnEnd = 0.0f;
        for (int i = 1; i < kWay.offCount; ++i) {
            const float* pose = kWay.off[i] + 1;
            offWorst = std::max(offWorst, nearestFrame(pose));
            turnEnd = TurnBetween(pose, none12);
        }
        float freeMove[3];
        WayGrab(kWay.free, freeMove);
        {
            float freePose[12];
            int i = 0;
            while (i + 2 < kWay.offCount && kWay.off[i + 1][0] < kWay.free)
                ++i;
            std::memcpy(freePose, kWay.off[i + 1] + 1, sizeof(freePose));
            turnAtFree = TurnBetween(freePose, none12);
        }
        rightAtFree = -freeMove[0], riseAtFree = freeMove[1];
        std::snprintf(what, sizeof(what), "p3 the box's way off: %d rows, each the recording's own frame to %.3f mm; free %.1f cm along: its grab point %.1f cm to the gun's right and %.1f mm up, the box "
                      "turned %.1f degrees (its right end tipped down); %.1f degrees at its last row",
                      kWay.offCount, offWorst * 1000.0f, kWay.free * 100.0f, rightAtFree * 100.0f, riseAtFree * 1000.0f, turnAtFree, turnEnd);
        Check(offWorst < 0.00002f && std::fabs(kWay.free - 0.0824f) < 0.0001f && rightAtFree > 0.075f && std::fabs(riseAtFree) < 0.01f && turnAtFree > 15.0f && turnAtFree < 25.0f &&
                  turnEnd > 25.0f,
              what);
        const float middle[3] = {0.009f, -0.094f, 0.551f};  // (the box's middle on the frame, seated)
        float onWorst = 0.0f, firstFrom = 0.0f, firstTurn = 0.0f, firstRight = 0.0f, firstBelow = 0.0f;
        for (int i = 0; i < kWay.onCount; ++i) {
            const float* pose = kWay.on[i] + 1;
            if (i + 1 < kWay.onCount)
                onWorst = std::max(onWorst, nearestFrame(pose));
            if (i == 0) {
                float mid[3];
                Apply(pose, middle, mid);
                firstFrom = Dist(mid, middle), firstTurn = TurnBetween(pose, none12), firstRight = middle[0] - mid[0], firstBelow = middle[1] - mid[1];
            }
        }
        const float* last = kWay.on[kWay.onCount - 1] + 1;
        const bool home = last[0] == 1.0f && last[5] == 1.0f && last[10] == 1.0f && last[3] == 0.0f && last[7] == 0.0f && last[11] == 0.0f;
        std::snprintf(what, sizeof(what), "p3 the box's way on: %d rows over %.3f s, each the recording's own frame to %.3f mm; from %.1f cm off its seat (%.1f cm to the gun's right, %.1f cm below), "
                      "turned %.1f degrees, swung up into the seat %d",
                      kWay.onCount, kWay.on[kWay.onCount - 1][0], onWorst * 1000.0f, firstFrom * 100.0f, firstRight * 100.0f, firstBelow * 100.0f, firstTurn, home);
        Check(onWorst < 0.00002f && firstFrom > 0.06f && firstFrom < 0.10f && firstRight > 0.05f && firstBelow > 0.03f && firstTurn > 25.0f && firstTurn < 40.0f && home &&
                  kWay.on[kWay.onCount - 1][0] > 0.05f && kWay.on[kWay.onCount - 1][0] < 0.2f,
              what);
    }

    // ---- p4: the fitted hands ----
    {
        float boxHand[3], lidHand[3], overHand[3], underHand[3];
        MiddleOf(magposes::kPkpHold, boxHand);
        MiddleOf(magposes::kPkpLidHold, lidHand);
        MiddleOf(magposes::kPkpLeverOverHold, overHand);
        MiddleOf(magposes::kPkpLeverUnderHold, underHand);
        std::snprintf(what, sizeof(what), "p4 the fitted hands' middles: on the box (%.3f %.3f %.3f), %.1f mm from the row's grab; on the cover (%.3f %.3f %.3f), %.1f mm from its grab; over the handle "
                      "%.1f cm from its knob, under it %.1f cm, both on the gun's right",
                      boxHand[0], boxHand[1], boxHand[2], Dist(boxHand, kGun.grab) * 1000.0f, lidHand[0], lidHand[1], lidHand[2], Dist(lidHand, kBelt.lidGrab) * 1000.0f,
                      Dist(overHand, kGun.lever) * 100.0f, Dist(underHand, kGun.lever) * 100.0f);
        Check(Dist(boxHand, kGun.grab) < 0.001f && Dist(lidHand, kBelt.lidGrab) < 0.001f && Dist(overHand, kGun.lever) < 0.09f && Dist(underHand, kGun.lever) < 0.09f && overHand[0] < 0.0f &&
                  underHand[0] < 0.0f,
              what);
        bool sound = true;
        for (const float* hold : {&magposes::kPkpHold[0], &magposes::kPkpLidHold[0], &magposes::kPkpLeverOverHold[0], &magposes::kPkpLeverUnderHold[0]})
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j) {
                    const float dot = hold[i * 4] * hold[j * 4] + hold[i * 4 + 1] * hold[j * 4 + 1] + hold[i * 4 + 2] * hold[j * 4 + 2];
                    sound = sound && std::fabs(dot - (i == j ? 1.0f : 0.0f)) < 0.001f;
                }
        for (const auto* fingers : {magposes::kPkpFingers, magposes::kPkpLidFingers, magposes::kPkpLeverOverFingers, magposes::kPkpLeverUnderFingers})
            for (int b = 0; b < handpose::kBones; ++b)
                sound = sound && std::all_of(fingers[b], fingers[b] + 12, [](float f) { return std::isfinite(f); });
        Check(sound, "p4 the four holds rigid turns, their fingers' bones finite");
    }
    {
        meshdump::Mesh boxMesh, gunMesh;
        if (!glove.read || !meshdump::LoadDraw(dir, kBody[0], kFrameBone, {}, boxMesh, kBoxBone, 0, kBody[1]) || !meshdump::LoadDraw(dir, kBody[0], kFrameBone, {}, gunMesh, -1, 0, kBody[1])) {
            Skip("p4 the hands against the meshes: no dumps of the gloves or the body");
        } else {
            // The box's hold on the box (its bone of the body, on the frame), 1 mm cells.
            float lo[3] = {1e9f, 1e9f, 1e9f}, hi[3] = {-1e9f, -1e9f, -1e9f};
            for (int v : boxMesh.tris)
                for (int k = 0; k < 3; ++k)
                    lo[k] = std::min(lo[k], boxMesh.pos[v * 3 + k]), hi[k] = std::max(hi[k], boxMesh.pos[v * 3 + k]);
            for (int k = 0; k < 3; ++k)
                lo[k] -= 0.06f, hi[k] += 0.06f;
            const Field box = FieldOf(boxMesh, lo, hi, 0.001f);
            auto onBox = [&](const float* p) { return box.At(p); };
            float toGun[12];
            InvertRigid(magposes::kPkpHold, toGun);
            const float palm = GapOf(glove, glove.palm, magposes::kPkpFingers, toGun, onBox) * 1000.0f;
            float part[5][3], deepest = palm, farthestFinger = -1e9f;
            for (int fi = 0; fi < 5; ++fi) {
                float nearest = 1e9f;
                for (int d = 1; d <= 3; ++d) {
                    part[fi][d - 1] = GapOf(glove, glove.part[fi][d], magposes::kPkpFingers, toGun, onBox) * 1000.0f;
                    nearest = std::min(nearest, part[fi][d - 1]);
                    deepest = std::min(deepest, part[fi][d - 1]);
                }
                farthestFinger = std::max(farthestFinger, nearest);
            }
            std::snprintf(what, sizeof(what), "p4 the hand on the box's left end: palm %.1f mm; thumb %.1f %.1f %.1f; index %.1f %.1f %.1f; middle %.1f %.1f %.1f; ring %.1f %.1f %.1f; little %.1f %.1f %.1f "
                          "(- in it); each finger's nearest part %.1f mm off at most, the deepest %.1f mm in",
                          palm, part[0][0], part[0][1], part[0][2], part[1][0], part[1][1], part[1][2], part[2][0], part[2][1], part[2][2], part[3][0], part[3][1], part[3][2], part[4][0],
                          part[4][1], part[4][2], farthestFinger, -deepest);
            Check(palm <= 8.0f && palm >= -2.5f && farthestFinger <= 3.0f && deepest >= -3.0f, what);
            // The handle from under it: the body with its handle, round the knob, 2 mm cells.
            const float near0[3] = {kGun.lever[0] - 0.12f, kGun.lever[1] - 0.12f, kGun.lever[2] - 0.14f}, near1[3] = {kGun.lever[0] + 0.10f, kGun.lever[1] + 0.10f, kGun.lever[2] + 0.14f};
            const Field gun = FieldOf(gunMesh, near0, near1, 0.002f);
            auto onGun = [&](const float* p) { return gun.At(p); };
            InvertRigid(magposes::kPkpLeverUnderHold, toGun);
            const float underPalm = GapOf(glove, glove.palm, magposes::kPkpLeverUnderFingers, toGun, onGun) * 1000.0f;
            float underDeepest = underPalm;
            for (int fi = 0; fi < 5; ++fi)
                for (int d = 1; d <= 3; ++d)
                    underDeepest = std::min(underDeepest, GapOf(glove, glove.part[fi][d], magposes::kPkpLeverUnderFingers, toGun, onGun) * 1000.0f);
            std::snprintf(what, sizeof(what), "p4 the hand from under the handle, against the gun and its knob (2 mm cells): palm %.1f mm off, the hand's deepest %.1f mm in", underPalm, -underDeepest);
            Check(underPalm <= 8.0f && underPalm >= -2.0f && underDeepest >= -4.0f, what);
        }
    }
    // The box's and the cover's holds against the gun's own shape (build\guns\pkp.bin, what the hands collide with in the game; the cover is in it, shut): no
    // vertex of either hand further into it than a glove gives (2.5 mm, and a fraction of the shape's 3 mm cells).
    if (!glove.read || !haveShape) {
        Skip("p4 the holds against the gun: no gloves' dump or gun shape (build\\guns\\pkp.bin)");
    } else {
        for (int which = 0; which < 2; ++which) {
            const float* hold = which ? magposes::kPkpLidHold : magposes::kPkpHold;
            const auto* fingers = which ? magposes::kPkpLidFingers : magposes::kPkpFingers;
            float toGun[12];
            InvertRigid(hold, toGun);
            float part[6];
            std::fill(std::begin(part), std::end(part), 1000.0f);
            int past = 0;
            for (const meshdump::GloveVertex& v : glove.vertices) {
                if (v.finger < -1 || v.finger > 4)
                    continue;
                float p[3];
                PoseVertex(v, fingers, toGun, p);
                const float d = shapeAt(p) * 1000.0f;
                part[v.finger + 1] = std::min(part[v.finger + 1], d);
                past += d < -2.7f;
            }
            const float deepest = *std::min_element(std::begin(part), std::end(part));
            std::snprintf(what, sizeof(what), "p4 the hand on the %s against the gun's shape: palm %.1f mm, thumb %.1f, index %.1f, middle %.1f, ring %.1f, little %.1f (- in it); %d vertices past a "
                          "glove's give",
                          which ? "cover" : "box", part[0], part[1], part[2], part[3], part[4], part[5], past);
            Check(deepest >= -2.7f && past == 0 && (which || deepest > 10.0f), what);
        }
    }

    // ---- p5: handmag on the PKP's own table ----
    {
        Reset();
        handmag::Frame f = Base();
        float glove12[12], at[3];
        GloveBy(magposes::kPkpHold, kHandInGlove, kGun.grab, glove12);
        std::memcpy(f.glove, glove12, sizeof(glove12));
        handmag::Update(f);
        f.squeeze = 1.0f;
        const handmag::Result shut = handmag::Update(f);
        f.squeeze = 0.0f;
        handmag::Update(f);
        Check(!shut.grabbed && shut.beltReady, "p5 the cover shut: a squeeze on the box takes nothing, the belt ready to fire");
        LidAt(0.0f, at);
        GloveBy(magposes::kPkpLidHold, kHandInGlove, at, f.glove);
        handmag::Update(f);
        f.squeeze = 1.0f;
        const bool lidTaken = handmag::Update(f).lidTaken;
        bool lidUp = false, readyWhileUp = false;
        for (int i = 1; i < 90 && !lidUp; ++i) {
            LidAt(std::min(0.02f * i, 0.9f), at);
            GloveBy(magposes::kPkpLidHold, kHandInGlove, at, f.glove);
            const handmag::Result r = handmag::Update(f);
            lidUp = r.lidUp;
            readyWhileUp = readyWhileUp || r.beltReady;
        }
        f.squeeze = 0.0f;
        handmag::Result r = handmag::Update(f);
        std::snprintf(what, sizeof(what), "p5 the cover taken on its top %d, lifted till it goes on by itself: up %d (%.1f degrees), the belt not ready meanwhile %d", lidTaken, lidUp,
                      r.lid * 180.0f / kPi, !readyWhileUp);
        Check(lidTaken && lidUp && std::fabs(r.lid - kBelt.lidOpen) < 0.01f && !readyWhileUp && !r.beltReady, what);
        auto takeAt = [&](handmag::Frame& g, const float* place) {
            GloveBy(magposes::kPkpHold, kHandInGlove, place, g.glove);
            handmag::Update(g);
            g.squeeze = 1.0f;
            return handmag::Update(g).grabbed;
        };
        auto boxOnGun = [&](const handmag::Result& res, float* onGun) {
            float gunInverse[12];
            InvertRigid(kGunPose, gunInverse);
            Compose(gunInverse, res.magPose, onGun);
        };
        auto drawnOff = [&](const handmag::Result& res, const float* real) {
            float a[3], b[3];
            Apply(real, kHandInGlove, a);
            if (res.gloveBlend <= 0.0f)
                return 0.0f;
            Apply(res.glove, kHandInGlove, b);
            for (int k = 0; k < 3; ++k)
                b[k] = a[k] + (b[k] - a[k]) * res.gloveBlend;
            return Dist(a, b);
        };
        {
            // Pulled from its left end the ways its own reload doesn't take it, 2 mm a frame to 25 cm (degrees below level, toward the gun's right): straight and
            // steeply down it stays in, let go of once the hand is 8 cm off its way (the glove never further off the hand), the box never leaping along its way;
            // level and gently down it comes free.
            std::string downs, frees;
            bool downOk = true, freeOk = true;
            for (const float degrees : {90.0f, 75.0f, 60.0f, 30.0f, 15.0f, 0.0f}) {
                handmag::Frame g;
                const bool up = CoverUp(g);
                const bool took = takeAt(g, kGun.grab);
                const float c = std::cos(degrees * kPi / 180.0f), s = std::sin(degrees * kPi / 180.0f);
                bool freed = false, held = true, haveWas = false;
                float glovedMost = 0.0f, step = 0.0f, was[3] = {}, leftAt = 0.0f, freeAt = 0.0f;
                for (int i = 1; i <= 125 && held && !freed; ++i) {
                    const float to[3] = {kGun.grab[0] - c * 0.002f * i, kGun.grab[1] - s * 0.002f * i, kGun.grab[2]};
                    GloveBy(magposes::kPkpHold, kHandInGlove, to, g.glove);
                    const handmag::Result res = handmag::Update(g);
                    freed = res.pulledOut;
                    held = res.holding;
                    if (!held)
                        leftAt = 0.002f * i;
                    if (freed)
                        freeAt = 0.002f * i;
                    if (held && !freed) {
                        glovedMost = std::max(glovedMost, drawnOff(res, g.glove));
                        float onGun[12], grabAt[3];
                        boxOnGun(res, onGun);
                        Apply(onGun, kGun.grab, grabAt);
                        if (haveWas)
                            step = std::max(step, Dist(grabAt, was));
                        std::memcpy(was, grabAt, sizeof(was));
                        haveWas = true;
                    }
                }
                char line[200];
                std::snprintf(line, sizeof(line), "%.0f: %s %.1f cm, the glove %.1f cm off at most, the box's step %.1f mm", degrees, freed ? "free after" : !held ? "left in after" : "still held at",
                              (freed ? freeAt : !held ? leftAt : 0.25f) * 100.0f, glovedMost * 100.0f, step * 1000.0f);
                if (degrees >= 60.0f) {
                    downs += (downs.empty() ? "" : "; ") + std::string(line);
                    downOk = downOk && up && took && !freed && !held && leftAt < 0.14f && glovedMost < 0.0805f && step < 0.006f;
                } else {
                    // (Its way off goes level to the right, its left end 6.8 mm up by free: a pull 30 degrees down is 30 off it, the M249's 45 is about as far
                    // off its own; 15 and level are near it.)
                    frees += (frees.empty() ? "" : "; ") + std::string(line);
                    const bool near = degrees < 20.0f;
                    freeOk = freeOk && up && took && freed && freeAt < (near ? 0.105f : 0.125f) && glovedMost < (near ? 0.045f : 0.075f) && step < 0.006f;
                }
            }
            std::snprintf(what, sizeof(what), "p5 the box taken at its left end, the cover up, pulled steeply down toward the gun's right (degrees below level), 2 mm a frame to 25 cm: %s", downs.c_str());
            Check(downOk, what);
            std::snprintf(what, sizeof(what), "p5 ... pulled level or gently down to the gun's right: %s", frees.c_str());
            Check(freeOk, what);
            // weapon.txt "heldtest 7"'s scripted pull (hands.cpp): from the seat in a straight line to where the way's last row has the grab point, 3 cm
            // forward, in 0.7 s eased at both ends, at 72 Hz: free on the way, the glove never far off the hand.
            handmag::Frame g;
            CoverUp(g);
            const bool took3 = takeAt(g, kGun.grab);
            float wayEnd[3];
            Apply(kWay.off[kWay.offCount - 1] + 1, kGun.grab, wayEnd);
            for (int k = 0; k < 3; ++k)
                wayEnd[k] -= kGun.grab[k];
            wayEnd[2] += 0.03f;
            bool scriptedFree = false;
            float scriptedAt = 0.0f, scriptedGlove = 0.0f;
            for (int i = 1; i <= 51 && !scriptedFree; ++i) {
                const float u = std::min(i * kDt / 0.7f, 1.0f), e = u * u * (3.0f - 2.0f * u);
                const float to[3] = {kGun.grab[0] + wayEnd[0] * e, kGun.grab[1] + wayEnd[1] * e, kGun.grab[2] + wayEnd[2] * e};
                GloveBy(magposes::kPkpHold, kHandInGlove, to, g.glove);
                const handmag::Result res = handmag::Update(g);
                scriptedFree = res.pulledOut;
                scriptedAt = e;
                if (!scriptedFree)
                    scriptedGlove = std::max(scriptedGlove, drawnOff(res, g.glove));
            }
            std::snprintf(what, sizeof(what), "p5 heldtest 7's scripted pull, a straight line to the way's last row in 0.7 s: taken %d, free %d %.0f%% of the way along it, the glove %.1f cm off the hand "
                          "at most",
                          took3, scriptedFree, scriptedAt * 100.0f, scriptedGlove * 100.0f);
            Check(took3 && scriptedFree && scriptedGlove < 0.08f, what);
            // Taken anywhere along the 29 cm box: in its middle and by its bail at its right end, held where the hand took it. Drawn as the box turns on its way
            // (where the hand took it goes its own path: by the bail 10 cm down as the left end slides 8 cm): free, the drawn glove on the real hand, the box
            // never leaping. Drawn level to the right instead (as the game's own hand draws its left end): free all the same, the glove drawn down with the box's
            // turning end, no further off the hand than the way takes that point off the grab point's path by the time it's free.
            for (const float x : {0.01f, -0.12f}) {
                const float from[3] = {x, kGun.grab[1], kGun.grab[2]};
                float ownAtFree[3], grabAtFree[3];
                WayPoint(from, kWay.free, ownAtFree);
                WayGrab(kWay.free, grabAtFree);
                const float apart = Dist(ownAtFree, grabAtFree);
                std::string lines;
                bool ok = true;
                for (const bool turning : {true, false}) {
                    handmag::Frame h;
                    CoverUp(h);
                    const bool took = takeAt(h, from);
                    bool freed = false, haveWas = false;
                    float step = 0.0f, was[3] = {}, glovedMost = 0.0f, handMoved = 0.0f;
                    for (int i = 1; i <= 90 && !freed; ++i) {
                        float to[3] = {from[0] - 0.002f * i, from[1], from[2]};
                        if (turning) {  // (2 mm of the way's measure a frame)
                            float move[3];
                            WayPoint(from, 0.002f * i, move);
                            for (int k = 0; k < 3; ++k)
                                to[k] = from[k] + move[k];
                        }
                        GloveBy(magposes::kPkpHold, kHandInGlove, to, h.glove);
                        const handmag::Result res = handmag::Update(h);
                        freed = res.pulledOut;
                        handMoved = Dist(to, from);
                        glovedMost = std::max(glovedMost, drawnOff(res, h.glove));
                        if (res.mag && !freed) {
                            float onGun[12], takenAt[3];
                            boxOnGun(res, onGun);
                            Apply(onGun, from, takenAt);
                            if (haveWas)
                                step = std::max(step, Dist(takenAt, was));
                            std::memcpy(was, takenAt, sizeof(was));
                            haveWas = true;
                        }
                    }
                    char line[240];
                    std::snprintf(line, sizeof(line), "%s: taken %d, free %d after the hand's %.1f cm, the drawn glove %.1f cm off the hand at most, the box's largest step a frame %.1f mm",
                                  turning ? "as the box turns" : "level", took, freed, handMoved * 100.0f, glovedMost * 100.0f, step * 1000.0f);
                    lines += (lines.empty() ? "" : "; ") + std::string(line);
                    ok = ok && took && freed && step < 0.01f && glovedMost < (turning ? 0.015f : apart + 0.01f);
                }
                std::snprintf(what, sizeof(what), "p5 the box taken %s (the hand's middle at x %.2f, its own path %.1f cm off the grab point's by free), drawn %s", x > 0.0f ? "in its middle" : "by its bail", x,
                              apart * 100.0f, lines.c_str());
                Check(ok, what);
            }
        }
        {
            // The handle with the cover up: from under it and over it at their fitted holds, by the trigger and the grip: the handle, not the box.
            auto tryAt = [&](const float* hold, bool trigger) {
                handmag::Frame g;
                CoverUp(g);
                float middle[3];
                MiddleOf(hold, middle);
                GloveBy(hold, kHandInGlove, middle, g.glove);
                handmag::Update(g);
                (trigger ? g.trigger : g.squeeze) = 1.0f;
                const handmag::Result res = handmag::Update(g);
                return res.leverTaken ? 1 : res.grabbed ? 2 : 0;
            };
            const int underTrigger = tryAt(magposes::kPkpLeverUnderHold, true), underGrip = tryAt(magposes::kPkpLeverUnderHold, false);
            const int overTrigger = tryAt(magposes::kPkpLeverOverHold, true), overGrip = tryAt(magposes::kPkpLeverOverHold, false);
            std::snprintf(what, sizeof(what), "p5 the cover up: the handle from under it by the trigger %d, the grip %d; over it by the trigger %d, the grip %d (1 the handle, 2 the box)", underTrigger,
                          underGrip, overTrigger, overGrip);
            Check(underTrigger == 1 && underGrip == 1 && overTrigger == 1 && overGrip == 1, what);
        }
        // The reload: the cover up, the box taken at its left end, drawn along its own way: on the way's own places (as far along as the hand, less its dead 6 mm),
        // turned as the way turns it, free once the way's free length is past.
        CoverUp(f);
        const bool grabbed = takeAt(f, kGun.grab);
        bool out = false;
        float freeAt = 0.0f, offWay = 0.0f;
        for (float along = 0.002f; along <= 0.15f && !out; along += 0.002f) {
            float move[3];
            WayGrab(along, move);
            const float place[3] = {kGun.grab[0] + move[0], kGun.grab[1] + move[1], kGun.grab[2] + move[2]};
            GloveBy(magposes::kPkpHold, kHandInGlove, place, f.glove);
            r = handmag::Update(f);
            out = r.pulledOut;
            freeAt = along;
            if (r.mag && !out) {
                float onGun[12], grabAt[3], want[3];
                boxOnGun(r, onGun);
                Apply(onGun, kGun.grab, grabAt);
                WayGrab(std::max(along - 0.006f, 0.0f), want);
                for (int k = 0; k < 3; ++k)
                    want[k] += kGun.grab[k];
                offWay = std::max(offWay, Dist(grabAt, want));
            }
        }
        std::snprintf(what, sizeof(what), "p5 the box taken at its left end with the cover up %d, drawn along its own way off: free %.1f cm along (the way's %.1f and the hand's dead 0.6), on the way's "
                      "places to %.2f mm",
                      grabbed, freeAt * 100.0f, kWay.free * 100.0f, offWay * 1000.0f);
        Check(grabbed && out && freeAt > kWay.free + 0.004f && freeAt < kWay.free + 0.010f && offWay < 0.001f, what);
        // Let go of, the well empty; a fresh one from the belly brought 3 cm to the right of where its way on starts and 4 cm under it: caught there, and home
        // along the way on, from the right and below, swung up into its seat; its belt hanging.
        f.wellEmpty = true;
        f.squeeze = 0.0f;
        r = handmag::Update(f);
        const bool letGo = r.letGo;
        f.squeeze = 1.0f;
        f.shoulder = true;
        const bool took = handmag::Update(f).took;
        f.shoulder = false;
        bool caught = false, seated = false;
        float onTop[3];
        WayOnTop(onTop);
        float turnMost = 0.0f, fromRight = 0.0f;
        const float boxMiddle[3] = {0.009f, -0.094f, 0.551f};
        const float none12[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
        for (int i = 0; i < 80 && !seated; ++i) {
            const float near[3] = {kGun.grab[0] + onTop[0] - kGun.top[0] - 0.03f, kGun.grab[1] + onTop[1] - kGun.top[1] - 0.04f, kGun.grab[2] + onTop[2] - kGun.top[2]};
            GloveBy(magposes::kPkpHold, kHandInGlove, near, f.glove);
            r = handmag::Update(f);
            caught = caught || r.caught;
            seated = r.seated;
            if (caught && r.mag) {
                float onGun[12], frameOnly[12], gunInverse[12], turn[12], backInverse[12], mid[3], seatMid[3];
                boxOnGun(r, onGun);
                InvertRigid(kGunPose, gunInverse);
                Compose(gunInverse, kGunPose, frameOnly);
                InvertRigid(frameOnly, backInverse);
                Compose(backInverse, onGun, turn);
                turnMost = std::max(turnMost, TurnBetween(turn, none12));
                Apply(onGun, boxMiddle, mid);
                Apply(frameOnly, boxMiddle, seatMid);
                fromRight = std::max(fromRight, seatMid[0] - mid[0]);
            }
        }
        f.wellEmpty = false;
        f.squeeze = 0.0f;
        r = handmag::Update(f);
        std::snprintf(what, sizeof(what), "p5 let go of %d; a fresh one taken at the belly %d, brought by where its way on starts: caught %d, home %d along its way on (turned up to %.1f degrees on its way; "
                      "in from %.1f cm to the gun's right); its belt drawn hanging %d, not ready %d",
                      letGo, took, caught, seated, turnMost, fromRight * 100.0f, r.beltDrawn, !r.beltReady);
        Check(letGo && took && caught && seated && turnMost > 25.0f && turnMost < 45.0f && fromRight > 0.04f && r.beltDrawn && !r.beltReady, what);
        // The belt pinched at its hanging tip and brought toward the box's side above it: in the hand it bows out to the gun's right (the side it comes from);
        // then to the feed from the right: laid.
        float tip[3];
        handmag::HangingTip(kBelt, tip);
        GloveBy(magposes::kPkpHold, kBelt.pinch, tip, f.glove);
        handmag::Update(f);
        f.squeeze = 1.0f;
        const bool beltTaken = handmag::Update(f).beltTaken;
        // Slack: the hand 3 cm over where it comes out of the box (its short belt: 6 cm over it is 8.4 cm from the feed, inside the 10 cm the feed takes
        // it from with the cover up; 3 cm over, 10.6 cm).
        float bow = 0.0f, slackFeed = 0.0f;
        const float slack[3] = {kBelt.anchor[0], kBelt.anchor[1] + 0.03f, kBelt.anchor[2]};
        bool slackHeld = true;
        {
            for (int i = 0; i < 12; ++i) {
                GloveBy(magposes::kPkpHold, kBelt.pinch, slack, f.glove);
                r = handmag::Update(f);
                slackHeld = slackHeld && r.beltDrawn && r.holding && !r.beltLaid;
            }
            slackFeed = Dist(slack, kBelt.feed);
            // The middle piece (bone 2: 2 from either end) against the line from the hand to where it comes out of the box, across the gun.
            const float* p = kBelt.pieces[2];
            const float mid[3] = {(p[0] + p[2]) / 2, (p[1] + p[3]) / 2, kBelt.pieceZ};
            float midAt[3];
            Apply(r.beltBones[2], mid, midAt);
            const float chordX = (slack[0] + kBelt.anchor[0]) / 2;
            bow = midAt[0] - chordX;
        }
        bool laid = false;
        for (int i = 1; i < 120 && !laid; ++i) {
            const float s = std::min(i / 40.0f, 1.0f);
            const float to[3] = {slack[0] + (kBelt.feed[0] - 0.06f - slack[0]) * s, slack[1] + (kBelt.feed[1] + 0.02f - slack[1]) * s, slack[2] + (kBelt.feed[2] - slack[2]) * s};
            GloveBy(magposes::kPkpHold, kBelt.pinch, to, f.glove);
            laid = handmag::Update(f).beltLaid;
        }
        f.squeeze = 0.0f;
        r = handmag::Update(f);
        std::snprintf(what, sizeof(what), "p5 its belt's hanging tip (%.3f %.3f %.3f) pinched %d; held with slack (%.1f cm from the feed) %d, it bows %.1f cm out to the gun's right; brought to the feed from "
                      "the right: laid %d; not ready with the cover up %d",
                      tip[0], tip[1], tip[2], beltTaken, slackFeed * 100.0f, slackHeld, -bow * 100.0f, laid, !r.beltReady);
        Check(kBelt.testTip[0] == 0.0f && beltTaken && slackHeld && bow < -0.003f && laid && !r.beltReady, what);
        LidAt(kBelt.lidOpen, at);
        GloveBy(magposes::kPkpLidHold, kHandInGlove, at, f.glove);
        handmag::Update(f);
        f.squeeze = 1.0f;
        handmag::Update(f);
        bool lidShut = false;
        for (int i = 1; i < 120 && !lidShut; ++i) {
            LidAt(std::max(kBelt.lidOpen - 0.02f * i, 0.3f), at);
            GloveBy(magposes::kPkpLidHold, kHandInGlove, at, f.glove);
            lidShut = handmag::Update(f).lidShut;
        }
        f.squeeze = 0.0f;
        for (int i = 0; i < 20 && !lidShut; ++i)
            lidShut = handmag::Update(f).lidShut;
        r = handmag::Update(f);
        std::snprintf(what, sizeof(what), "p5 the cover brought down: shut %d, the belt laid and the cover down: ready %d", lidShut, r.beltReady);
        Check(lidShut && r.beltReady && r.lid == 0.0f, what);
        for (int way = 0; way < 2; ++way) {
            const float* hold = way ? magposes::kPkpLeverUnderHold : magposes::kPkpLeverOverHold;
            float knobHand[3];
            MiddleOf(hold, knobHand);
            GloveBy(hold, kHandInGlove, knobHand, f.glove);
            handmag::Update(f);
            f.trigger = 1.0f;
            const handmag::Result taken = handmag::Update(f);
            bool fingers = false;
            float back = 0.0f;
            for (int i = 1; i <= 30; ++i) {
                const float to[3] = {knobHand[0], knobHand[1], knobHand[2] - 0.14f * i / 30.0f};
                GloveBy(hold, kHandInGlove, to, f.glove);
                r = handmag::Update(f);
                back = std::max(back, r.leverBack);
                fingers = fingers || r.leverFingers == (way ? magposes::kPkpLeverUnderFingers : magposes::kPkpLeverOverFingers);
            }
            f.trigger = 0.0f;
            r = handmag::Update(f);
            std::snprintf(what, sizeof(what), "p5 its handle %s: taken %d, the fitted hand on it %d, back %.1f cm (of %.1f), let go %d, a round chambered %d", way ? "from under it" : "over the top",
                          taken.leverTaken, fingers, back * 100.0f, kGun.leverTravel * 100.0f, r.leverLet, r.charged);
            Check(taken.leverTaken && fingers && back > 0.12f && r.leverLet && r.charged, what);
            for (int i = 0; i < 10; ++i)
                handmag::Update(f);
        }
        // Claims agrees with Update: a squeeze anywhere near the parts is the PKP's exactly when Update takes something; with the cover shut, up, and a fresh
        // box's belt hanging.
        uint32_t seed = 3u;
        auto next = [&] {
            seed = seed * 1664525u + 1013904223u;
            return static_cast<float>(seed >> 8) * (1.0f / 16777216.0f);
        };
        float tipHanging[3];
        handmag::HangingTip(kBelt, tipHanging);
        const char* const states[3] = {"the cover shut", "the cover up", "a fresh box's belt hanging"};
        for (int state = 0; state < 3; ++state) {
            int samples = 0, disagree = 0, taken = 0, ready = 0;
            for (int n = 0; n < 300; ++n) {
                handmag::Frame g;
                if (state == 0) {
                    Reset();
                    g = Base();
                    handmag::Update(g);
                    ready = 1;
                } else {
                    ready = state == 1 ? CoverUp(g) : BeltHanging(g);
                    if (!ready)
                        break;
                }
                const float centres[4][3] = {{kGun.grab[0], kGun.grab[1], kGun.grab[2]}, {kBelt.lidGrab[0], kBelt.lidGrab[1], kBelt.lidGrab[2]},
                                             {kGun.lever[0], kGun.lever[1], kGun.lever[2]}, {tipHanging[0], tipHanging[1], tipHanging[2]}};
                const float* c = centres[n % 4];
                const float place[3] = {c[0] + (next() - 0.5f) * 0.3f, c[1] + (next() - 0.5f) * 0.3f, c[2] + (next() - 0.5f) * 0.3f};
                GloveBy(n % 2 ? magposes::kPkpHold : magposes::kPkpLidHold, kHandInGlove, place, g.glove);
                g.squeeze = 0.0f;
                handmag::Update(g);
                float palm[3];
                Apply(g.glove, kHandInGlove, palm);
                const bool claims = handmag::Claims(kGun, g.gunPose, g.glove, palm);
                g.squeeze = 1.0f;
                const handmag::Result t = handmag::Update(g);
                const bool takes = t.lidTaken || t.leverTaken || t.grabbed || t.beltTaken;
                ++samples;
                taken += takes;
                disagree += claims != takes;
            }
            std::snprintf(what, sizeof(what), "p5 Claims and Update agree, %s: %d squeezes round the box, the cover, the handle and the belt's tip, %d of them taking something, %d apart", states[state],
                          samples, taken, disagree);
            Check(ready && samples == 300 && taken > 20 && disagree == 0, what);
        }
        Reset();
    }

    // ---- p6: its cover in four bones (skin.cpp's lidAlso): the other three turned with the cover's own as the hand lifts it, as the game's own reload has
    // them (the cover's rest turned by lidride::Turn to the row's 93 degrees against the empty reload's cover resting open, and its bones 3 and 5 likewise; the
    // feed arm, 2, has a turn of its own then and is carried as it rests); the depth pass's three the same bones; nothing of the gun's own rides the cover's
    // bone but itself (so what lidride lifts is a draw of its own on the cover, never the gun's). ----
    {
        bool depthSame = true;
        for (int k = 0; k < 3; ++k)
            depthSame = depthSame && pkprows::kDepthCoverAlso[k] == kBeltBones + pkprows::kCoverAlso[k] && pkprows::kCoverAlso[k] != kCoverBone;
        depthSame = depthSame && pkprows::kDepthCover == kBeltBones + kCoverBone && pkprows::kDepthBox == kBeltBones + kBoxBone && pkprows::kDepthHandle == kBeltBones + kHandleBone &&
                    pkprows::kDepthFrame == kBeltBones + kFrameBone;
        Check(depthSame, "p6 the depth pass's cover, its three other bones, box, handle and frame: the body's own, after the belt's five");
        if (!haveOpen) {
            Skip("p6 no empty reload: the cover's turn");
        } else {
            float turn[12], lifted[12], worstPlace = 0.0f, worstTurn = 0.0f;
            lidride::Turn(kBelt, kBelt.lidOpen, turn);
            Compose(turn, restCover, lifted);
            worstPlace = Apart(lifted, openCover), worstTurn = TurnBetween(lifted, openCover);
            for (int k = 0; k < 2; ++k) {
                Compose(turn, restOthers[k], lifted);
                worstPlace = std::max(worstPlace, Apart(lifted, openOthers[k]));
                worstTurn = std::max(worstTurn, TurnBetween(lifted, openOthers[k]));
            }
            std::snprintf(what, sizeof(what), "p6 the cover's rest and its bones 3 and 5 lifted by lidride::Turn to %.1f degrees: %.2f mm and %.2f degrees from the empty reload's resting open",
                          kBelt.lidOpen * 180.0f / kPi, worstPlace * 1000.0f, worstTurn);
            Check(worstPlace < 0.0008f && worstTurn < 0.3f, what);
        }
        if (!body.read) {
            Skip("p6 no dump of its body: what rides its cover");
        } else {
            float frameInverse[12], cover[12];
            InvertRigid(body.Bone(kFrameBone), frameInverse);
            Compose(frameInverse, body.Bone(kCoverBone), cover);
            int others = 0, riding = 0;
            for (int b = 0; b < 9; ++b) {
                if (b == kCoverBone)
                    continue;
                float on[12];
                Compose(frameInverse, body.Bone(b), on);
                ++others;
                riding += lidride::Rides(cover, on);
            }
            const Dump beltDump = Load(dir, kBeltDraw[0], kBeltDraw[1], -1);
            for (int b = 0; beltDump.read && b < kBeltBones; ++b) {
                float on[12];
                Compose(frameInverse, beltDump.Bone(b), on);
                ++others;
                riding += lidride::Rides(cover, on);
            }
            std::snprintf(what, sizeof(what), "p6 the cover's bone rides itself %d; of the gun's own other %d bones (its body's eight, its belt's five) %d ride it", lidride::Rides(cover, cover), others,
                          riding);
            Check(lidride::Rides(cover, cover) && riding == 0 && others == 8 + (beltDump.read ? kBeltBones : 0), what);
        }
    }

    // ---- p7: its box dropped (metal, magsound.h) and its sounds: nine clips cut from its own reload wave in the table of the game's own sounds
    // (tools/gamesounds.cpp's g_cuts, read as text beside this program's project), each a stretch of its 6.53 s, in the order its reload does them. ----
    {
        using magsound::Of;
        using magsound::Stuff;
        bool others = Of("m249") == Stuff::kCloth && Of("m240") == Stuff::kMetal && Of("ak74") == Stuff::kPlastic;
        std::snprintf(what, sizeof(what), "p7 its box: %s; the M249's pouch still cloth, the M240's box metal, the AK family's plastic: %d", Of("pkp") == Stuff::kMetal ? "metal" : "NOT METAL", others);
        Check(Of("pkp") == Stuff::kMetal && others, what);
        std::ifstream source("tools\\gamesounds.cpp");
        const std::string text{std::istreambuf_iterator<char>(source), std::istreambuf_iterator<char>()};
        if (text.empty()) {
            Skip("p7 tools\\gamesounds.cpp isn't here (not run from the project's folder): its clips");
        } else {
            const bool wave = text.find("static const char* const kPkp = \"pecheneg/pecheneg_reload_wave\";") != std::string::npos;
            int rows = 0;
            bool inOrder = true, inside = true;
            double lastFrom = -1.0;
            for (const char* part : {"lever", "bolt", "lid_latch", "lid_up", "mag_out", "mag_in", "belt", "belt_lay", "lid_shut"}) {
                const std::string key = std::string("{\"") + part + "\", \"pkp\", kPkp, ";
                const size_t at = text.find(key);
                if (at == std::string::npos)
                    continue;
                ++rows;
                double from = 0.0, to = 0.0;
                if (std::sscanf(text.c_str() + at + key.size(), "%lf, %lf", &from, &to) != 2)
                    continue;
                inside = inside && from > 0.0 && to > from && to < 6.533 && to - from < 0.5;
                inOrder = inOrder && from > lastFrom;
                lastFrom = from;
            }
            std::snprintf(what, sizeof(what), "p7 its clips: its wave named %d; %d of the nine parts (lever, bolt, lid_latch, lid_up, mag_out, mag_in, belt, belt_lay, lid_shut) cut from it, each a "
                          "short stretch inside it %d, in its reload's order %d",
                          wave, rows, inside, inOrder);
            Check(wave && rows == 9 && inside && inOrder, what);
        }
    }

    std::printf("%d checks, %d failed, %d skipped\n", g_checks, g_failed, g_skipped);
    return g_failed;
}

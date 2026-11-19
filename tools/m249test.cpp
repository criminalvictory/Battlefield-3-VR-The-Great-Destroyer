// The M249 (Operation Swordbreaker), held to what was measured, without the game: skin.cpp's own rows for it (src/m249rows.h, which skin.cpp builds its
// rows from: the review of 2026-11-18, a copy here could drift) against the mesh dumps and the recordings they were read from (build\mesh_24291_2808_*,
// 2808_0, 987_27099, the gloves' 13755_15051; build\survey\M249_partial.bin, M249_empty.bin), the tables written from them (src/magposes/m249*.h and the
// box's way m249_way.h, src/recoilguns.h's row, src/gunbodies.h's body, src/shaperecipes.h's recipe, its shape build\guns\m249.bin), and its belt reload
// worked by the left hand through handmag itself (src/handmag.cpp), as the M240's is.
//   w1  the tables: one row for it in each (its recoil row: its body draw and shape, a hundred rounds, 620 m/s alone of every row, 800 a minute, its kick
//       recorded; its body; its shape's recipe, its carrying handle in it as it rests, turned; its name), every number finite
//   w2  the mesh (SKIP without the dumps): its bones (handle, cover, carrying handle, frame); the handle's knob where the row's lever is; the sights'
//       line through the rear aperture's hole (from its back face to its front face) and the front post's top, to the muzzle; the belt's five pieces as
//       the row has them; the box, a draw of its own: its box, its rest on the frame (gunbodyfit's), its top and front; the carrying handle's rest
//   w3  the recordings (SKIP without them): the depth pass's bones (0-4 belt, 5 handle, 6 cover, 7 carrying handle, 8 frame, 9 box) in every frame; the
//       cover's hinge and its most open; the handle's draw straight back as far as the row's travel; the belt laid and hanging as the tables have them;
//       the palms at rest; the game's hand on the cover's back end where the lid is taken; the kick of each burst's first round as the recoil row has it;
//       the box's way off and on (m249_way.h) as the empty reload has it, the game's own hand going along with it; the game's own left hand at rest on
//       the handguard against the gun's shape (its palm 2 cm off in every rest frame: the game's own hold, kept as it is)
//   w4  the fitted hands: the box's hold on the pouch (its palm within 7.5 mm, every finger on it, none of it into it past the cloth's 10 mm), near the
//       game's own place on it; the cover's at its grab; the handle's over and under at the knob, the hand from under it with its palm within 7.5 mm of
//       the gun; none of their tables broken
//   w5  handmag on the M249's own table: the whole reload by hand (cover up; the box off along its own reload's way, free 8 cm to the left, not straight
//       down, taken anywhere along the pouch, eased into the hand; a fresh one home by itself along its way on, from the left at the seat's height,
//       turned 5.5 degrees and settling; its belt hanging, pinched at its tip and laid in the feed; the cover shut; the handle over and under; a round
//       chambered), the belt not ready until all of it is done; heldtest 7's pinch on the belt's own tip; Claims and Update agreeing
//   w6  what rides the feed cover (lidride.h): the cover's bone itself, no other bone of the gun; no draw of the M249's or the M240's recordings but
//       their own rides it; a draw on the cover's bone lifted with the lid exactly as the cover's bone
//   w7  its pouch dropped: cloth (magsound.h), the game's own ammo bag landing in the sounds' table; every other gun's magazines as they were
// Usage: m249test [<folder with the mesh dumps> [<folder with the recordings>]] (build and build\survey by default, from the project's folder). Reads only.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../src/gunbodies.h"
#include "../src/gunprofile.h"
#include "../src/gunshape.h"
#include "../src/handmag.h"
#include "../src/handpose.h"
#include "../src/lidride.h"
#include "../src/m249rows.h"
#include "../src/magposes.h"
#include "../src/magsound.h"
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

// ---- skin.cpp's own rows for it (m249rows.h: skin.cpp builds kM249Bones, its kHeldKinds and kHeldEyes rows and handmag's from these very ones) ----
const handmag::BeltGun& kBelt = m249rows::kBelt;
const handmag::Gun& kGun = m249rows::kMag;
const handmag::BoxWay& kWay = m249rows::kWay;
constexpr int kBody[2] = {static_cast<int>(m249rows::kBody), static_cast<int>(m249rows::kBodyStart)}, kDepth[2] = {static_cast<int>(m249rows::kDepth), static_cast<int>(m249rows::kDepth)};
constexpr int kBeltDraw[2] = {static_cast<int>(m249rows::kBeltCount), static_cast<int>(m249rows::kBeltStart)}, kBoxDraw[2] = {static_cast<int>(m249rows::kBoxCount), static_cast<int>(m249rows::kBoxStart)};
constexpr int kFrameBone = m249rows::kFrameBone, kHandleBone = m249rows::kHandleBone, kCoverBone = m249rows::kCoverBone, kCarryBone = m249rows::kCarryBone, kBoxBone = m249rows::kBoxBone;
constexpr int kDepthBox = m249rows::kDepthBox, kDepthHandle = m249rows::kDepthHandle, kDepthCover = m249rows::kDepthCover, kDepthCarry = m249rows::kDepthCarry,
              kDepthFrame = m249rows::kDepthFrame, kBeltBones = m249rows::kDepthBeltBones;
const float* const kBoxMin = m249rows::kBoxMin;
const float* const kBoxMax = m249rows::kBoxMax;
constexpr float kUp = m249rows::kUp;  // degrees the sights' line lies below z
const float* const kSights = m249rows::kSights;
const float* const kMuzzle = m249rows::kMuzzle;
const float* const kPort = m249rows::kPort;
const float* const kRightPalm = m249rows::kRightPalm;
const float* const kLeftPalm = m249rows::kLeftPalm;
constexpr float kEyeZ = m249rows::kEyeZ, kRearZ = m249rows::kRearZ;
// The M240's, for what rides a lid (w6): its body, its depth pass, its belt and its tape (its "rounds"), its frame and lid bones (skin.cpp's kM240Bones).
constexpr int kM240Body[2] = {27588, 2364}, kM240Depth[2] = {29952, 29952}, kM240Belt[2] = {2304, 0}, kM240Tape[2] = {60, 2304}, kM240Frame = 3, kM240Lid = 5;
// What the game said zoomed (build\survey\M249_zoom.txt): the view in its frame bone 3, FOV 40.
constexpr float kZoomedEye[3] = {0.0f, 0.093f, 0.088f}, kZoomedForward[3] = {-0.0004f, -0.0118f, 0.9999f};
// gunbodyfit's table: the box's bone 0 on the frame at rest.
constexpr float kBoxRest[12] = {1.0f, 0.00086f, -0.00044f, -0.00009f, -0.00086f, 1.0f, 0.00296f, 0.06350f, 0.00044f, -0.00295f, 1.0f, 0.33287f};
// The gloves' draw and the palms' points in it (as tools/mk11test.cpp has them).
constexpr int kGloves[2] = {13755, 15051};
constexpr int kRightRoot = 37, kLeftPalmBone = 1;
constexpr float kRightPalmPoint[3] = {-0.505f, 1.129f, 0.150f}, kLeftPalmPoint[3] = {0.537f, 1.098f, 0.207f};
constexpr float kHandInGlove[3] = {0.5363f, 1.0976f, 0.2092f};  // handmag.cpp's: the hand's middle

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
// Bones alike: places within mm and turns within degrees.
bool Alike(const float* a, const float* b, float mm, float degrees) {
    return Apart(a, b) * 1000.0f <= mm && TurnBetween(a, b) <= degrees;
}
bool Squashed(const float* m) {
    return m[0] * m[0] + m[4] * m[4] + m[8] * m[8] < 0.25f;
}
// The line the sights' row makes: from its point at the muzzle back along z, rising by kUp the further back.
float LineY(float z) {
    return kSights[1] + (kSights[2] - z) * std::tan(kUp * kPi / 180.0f);
}

// A mesh dump's draw: each vertex once, posed by the dump's own bones into the space of bone `frame` (or left in its own vertices' space: frame -1),
// with the bone it goes with. Stride 48: a half position at 0, four bones at 8, four weights at 12.
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

// ---- The left glove against a mesh, as tools/magfit.cpp measures its fits: the gloves' mesh dump (13755_15051), the palm without the cuff, each
// finger's three parts (each vertex on its main bone, less those by the joint before it, which a bend pinches in); a mesh as a distance field ----
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
// A glove vertex posed by a hand's bones (from the gloves' bind space), then put on the gun by toGun (the bones' space onto the gun's frame).
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
    float At(const float* p) const {  // metres to the mesh's surface, negative inside; off the grid, a lot
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
// The nearest of these glove vertices to a surface (metres, negative in it), the glove posed by bones and put on the gun by toGun.
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

// ---- handmag on scripted frames at 72 Hz (as tools/heldmagtest.cpp drives the M240) ----
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
// The cover's back end (where it's taken) with the cover turned up by this much, on the frame.
void LidAt(float angle, float* out) {
    const float c = std::cos(angle), s = std::sin(angle), y = kBelt.lidGrab[1] - kBelt.lidHinge[0], z = kBelt.lidGrab[2] - kBelt.lidHinge[1];
    out[0] = kBelt.lidGrab[0];
    out[1] = kBelt.lidHinge[0] + c * y - s * z;
    out[2] = kBelt.lidHinge[1] + s * y + c * z;
}
// The hand's middle of a table's hold on the gun's frame (its glove's bind space's kHandInGlove).
void MiddleOf(const float* hold, float* out) {
    float inGun[12];
    InvertRigid(hold, inGun);
    Apply(inGun, kHandInGlove, out);
}
// The cover up: taken at its back end and lifted till it goes on by itself, then let go of.
bool CoverUp(handmag::Frame& f) {
    Reset();
    f = Base();
    float at[3];
    LidAt(0.0f, at);
    GloveBy(magposes::kM249LidHold, kHandInGlove, at, f.glove);
    handmag::Update(f);
    f.squeeze = 1.0f;
    const bool taken = handmag::Update(f).lidTaken;
    bool up = false;
    for (int i = 1; i < 90 && !up; ++i) {
        LidAt(std::min(0.02f * i, 0.9f), at);
        GloveBy(magposes::kM249LidHold, kHandInGlove, at, f.glove);
        up = handmag::Update(f).lidUp;
    }
    f.squeeze = 0.0f;
    handmag::Update(f);
    return taken && up;
}
// The box's way off (its rows' grab point, as handmag counts the hand's pull along it): where the grab point has come at this far along it, from its seat.
void WayGrab(float at, float* out) {
    int i = 0;
    while (i + 2 < kWay.offCount && kWay.off[i + 1][0] < at)
        ++i;
    float a[3], b[3];
    Apply(kWay.off[i] + 1, kGun.grab, a);
    Apply(kWay.off[i + 1] + 1, kGun.grab, b);
    const float span = kWay.off[i + 1][0] - kWay.off[i][0], s = span > 1e-6f ? (at - kWay.off[i][0]) / span : 1.0f;  // (past the last row: on along it)
    for (int k = 0; k < 3; ++k)
        out[k] = a[k] + (b[k] - a[k]) * s - kGun.grab[k];
}
// Where a fresh box starts in on its way on: its top there, on the frame.
void WayOnTop(float* out) {
    Apply(kWay.on[0] + 1, kGun.top, out);
}
// The hand on the box (as its hold has it) from where it took it, along the way off to this far along it, 2 mm a frame: whether it came free, and how far
// along the hand had come then (0: not).
float PullAlongWay(handmag::Frame& f, const float* takenAt, float most) {
    for (float at = 0.002f; at <= most + 1e-6f; at += 0.002f) {
        float move[3];
        WayGrab(at, move);
        const float place[3] = {takenAt[0] + move[0], takenAt[1] + move[1], takenAt[2] + move[2]};
        GloveBy(magposes::kM249Hold, kHandInGlove, place, f.glove);
        if (handmag::Update(f).pulledOut)
            return at;
    }
    return 0.0f;
}
// ... then the box off by the hand along its way, let go of, and a fresh one home by itself: its belt hanging out of it.
bool BeltHanging(handmag::Frame& f) {
    if (!CoverUp(f))
        return false;
    GloveBy(magposes::kM249Hold, kHandInGlove, kGun.grab, f.glove);
    handmag::Update(f);
    f.squeeze = 1.0f;
    if (!handmag::Update(f).grabbed)
        return false;
    const bool out = PullAlongWay(f, kGun.grab, 0.15f) > 0.0f;
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
        // (The box's top 3 cm to the left of where its way on starts and 4 cm under it.)
        const float near[3] = {kGun.grab[0] + top[0] - kGun.top[0] + 0.03f, kGun.grab[1] + top[1] - kGun.top[1] - 0.04f, kGun.grab[2] + top[2] - kGun.top[2]};
        GloveBy(magposes::kM249Hold, kHandInGlove, near, f.glove);
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
    char what[1000];

    // ---- w1: the tables ----
    {
        const recoilguns::Row* row = recoilguns::ByName("Weapons/M249/M249");
        const bool known = row && row->body == static_cast<uint32_t>(kBody[0]) && row->shape && !std::strcmp(row->shape, "m249") && recoilguns::ByBody(kBody[0]) == row &&
                           recoilguns::ByShape("m249") == row;
        std::snprintf(what, sizeof(what), "w1 its recoil row: \"%s\", its body draw %u, its shape %s; found by its blueprint, its body and its shape alike", row ? row->name : "none",
                      row ? row->body : 0, row && row->shape ? row->shape : "none");
        Check(known, what);
        if (row) {
            int same = 0;
            for (const recoilguns::Row& other : recoilguns::kRows)
                same += other.speed == 620.0f;
            std::snprintf(what, sizeof(what), "w1 ... a machine gun, %d in its box (open bolt: no round kept), %.0f m/s (its alone of the %zu rows: %d), %.0f a minute; its buzz %.1f %.1f for %.2f s",
                          row->magazine, row->speed, std::size(recoilguns::kRows), same, row->rate, row->rumble[0], row->rumble[1], row->rumble[2]);
            Check(row->kind == recoil::Kind::kMg && row->magazine == 100 && row->speed == 620.0f && same == 1 && row->rate == 800.0f && row->rumble[0] > 0.0f && row->rumble[2] > 0.0f, what);
            std::snprintf(what, sizeof(what), "w1 ... its kick its own recording's: %.2f cm back, a rise of %.2f degrees (with the drift's answer its recorded 2.44), held fire's push %.2f of a round's",
                          row->kick.back.peak * 100.0f, row->kick.rise.peak, row->kick.heldShare);
            Check(std::fabs(row->kick.back.peak - 0.0151f) < 1e-5f && row->kick.rise.peak > 1.0f && row->kick.rise.peak < 2.44f && row->kick.heldShare > 0.9f && row->kick.heldShare < 1.2f, what);
            Check(&recoilguns::Default(recoil::Kind::kMg) != row, "w1 ... the machine guns' own row for a gun known only by its kind stays the M240's");
        }
        const gunbodies::Body* body = gunbodies::For("m249");
        std::snprintf(what, sizeof(what), "w1 its body for free hands and the holster: %s, %.1f kg, %d hull points, %d rounds of %.1f g, heavy to the ear", body && body->spec ? body->spec->name : "none",
                      body && body->spec ? body->spec->massEmpty : 0.0f, body && body->spec ? body->spec->hullPoints : 0, body && body->spec ? body->spec->capacity : 0,
                      body && body->spec ? body->spec->massRound * 1000.0f : 0.0f);
        Check(body && body->spec && !std::strcmp(body->spec->name, "m249gun") && body->spec->keep && body->spec->hullPoints >= 4 && body->spec->capacity == 100 && body->sound == gunbodies::kHeavy, what);
        const shaperecipes::Recipe* recipe = shaperecipes::Find("m249");
        const std::string arguments = recipe ? shaperecipes::Arguments(*recipe) : "";
        const shaperecipes::Rest* carry = nullptr;
        for (int k = 0; recipe && k < recipe->draws[0].restCount; ++k)
            if (recipe->draws[0].rest[k].bone == kCarryBone)
                carry = &recipe->draws[0].rest[k];
        std::snprintf(what, sizeof(what), "w1 its collision shape's recipe: gunshape ... 0.003 %s (its body from 2808, its frame bone 3, its charging handle left out; its feed cover kept "
                      "shut, its carrying handle kept as it rests: turned %s on the frame; its box and belt are draws of their own)", arguments.c_str(),
                      carry && carry->turned ? "(its turn in its row)" : "NOT IN IT");
        Check(recipe && arguments == "24291@2808 3 0" && recipe->draws[0].vbOffset == 72624 && recipe->drawCount == 1 && recipe->draws[0].restCount == 2 &&
                  recipe->draws[0].rest[0].bone == kCoverBone && !recipe->draws[0].rest[0].turned && carry && carry->turned,
              what);
        std::snprintf(what, sizeof(what), "w1 its name in the logs: %s (row %d)", gunprofile::NameOf("m249"), gunprofile::RowOf("m249"));
        Check(!std::strcmp(gunprofile::NameOf("m249"), "M249") && gunprofile::RowOf("m249") > gunprofile::kStingerRow, what);
        const float numbers[] = {kGun.top[1], kGun.top[2], kGun.grab[0], kGun.grab[1], kGun.grab[2], kGun.hook[2], kGun.mouth, kGun.lever[0], kGun.lever[1], kGun.lever[2], kGun.leverTravel,
                                 kBelt.lidHinge[0], kBelt.lidHinge[1], kBelt.lidOpen, kBelt.feed[0], kBelt.feed[1], kBelt.feed[2], kBelt.anchor[0], kBelt.anchor[1], kBelt.pieceZ,
                                 kSights[1], kSights[2], kUp, kEyeZ, kRearZ};
        Check(std::all_of(std::begin(numbers), std::end(numbers), [](float f) { return std::isfinite(f); }) && kGun.mouth < 0.0f && kRearZ > kEyeZ && kGun.way == &kWay &&
                  !kGun.straight && kGun.wide && kGun.grabTo <= kBoxMin[0] + 0.001f && kGun.noKnock && !kGun.button && !kGun.boltCatch && kGun.rockIn == 0.0f &&
                  kWay.offCount >= 3 && kWay.onCount >= 3 && kWay.free > 0.0f && kWay.free < kWay.off[kWay.offCount - 1][0],
              "w1 its rows' numbers finite; no magazine button, nothing knocks its box out, the box off and on along its own reload's way (not straight, not rocked), "
              "taken anywhere along it to its right end, no bolt catch");
    }

    // ---- w2: the mesh ----
    const Dump body = Load(dir, kBody[0], kBody[1], kFrameBone);
    if (!body.read) {
        Skip("w2 no dump of its body (build\\mesh_24291_2808_*): its mesh");
    } else {
        Box frame, handle, cover, carry;
        for (const Vertex& v : body.vertices)
            (v.bone == kHandleBone ? handle : v.bone == kCoverBone ? cover : v.bone == kCarryBone ? carry : frame).Add(v.at);
        std::snprintf(what, sizeof(what), "w2 its body: %d vertices on the frame (bone 3), %d the handle's (0), %d the feed cover's (1), %d the carrying handle's (2); %.3f m long", frame.count,
                      handle.count, cover.count, carry.count, frame.hi[2] - frame.lo[2]);
        Check(frame.count > 5000 && handle.count > 100 && cover.count > 2000 && carry.count > 300 && frame.count + handle.count + cover.count + carry.count == static_cast<int>(body.vertices.size()) &&
                  frame.hi[2] - frame.lo[2] > 0.95f && frame.hi[2] - frame.lo[2] < 1.1f,
              what);
        // The handle's knob: what of it stands out on the right furthest.
        Box knob;
        for (const Vertex& v : body.vertices)
            if (v.bone == kHandleBone && v.at[0] < -0.065f)
                knob.Add(v.at);
        const float knobMid[3] = {(knob.lo[0] + knob.hi[0]) / 2, (knob.lo[1] + knob.hi[1]) / 2, (knob.lo[2] + knob.hi[2]) / 2};
        std::snprintf(what, sizeof(what), "w2 its handle on the right (x %.3f to %.3f): its knob x %.3f to %.3f, y %.3f to %.3f, z %.3f to %.3f, its middle %.1f mm from the row's lever (%.4f %.4f %.4f)",
                      handle.lo[0], handle.hi[0], knob.lo[0], knob.hi[0], knob.lo[1], knob.hi[1], knob.lo[2], knob.hi[2], Dist(knobMid, kGun.lever) * 1000.0f, kGun.lever[0], kGun.lever[1],
                      kGun.lever[2]);
        Check(knob.count >= 20 && handle.hi[0] < 0.0f && Dist(knobMid, kGun.lever) < 0.002f, what);
        // The feed cover: from its back end to its hinge's end, over the receiver; its back end where the hand takes it.
        std::snprintf(what, sizeof(what), "w2 its feed cover: x %.3f to %.3f, y %.3f to %.3f, z %.3f to %.3f; the row's hinge at z %.4f within its front end, its grab (%.3f %.3f %.3f) %.1f cm "
                      "ahead of its back end, on its left",
                      cover.lo[0], cover.hi[0], cover.lo[1], cover.hi[1], cover.lo[2], cover.hi[2], kBelt.lidHinge[1], kBelt.lidGrab[0], kBelt.lidGrab[1], kBelt.lidGrab[2],
                      (kBelt.lidGrab[2] - cover.lo[2]) * 100.0f);
        Check(kBelt.lidHinge[1] > cover.hi[2] - 0.02f && kBelt.lidHinge[1] < cover.hi[2] && kBelt.lidGrab[2] - cover.lo[2] > 0.0f && kBelt.lidGrab[2] - cover.lo[2] < 0.05f &&
                  kBelt.lidGrab[0] > 0.0f && kBelt.lidGrab[0] < cover.hi[0] + 0.01f,
              what);
        // The iron sights: the rear aperture's hole on the cover and the front post's top, on the row's line.
        Box post;
        float postTop = -1e9f;
        for (const Vertex& v : body.vertices)
            if (v.bone == kFrameBone && std::fabs(v.at[0]) < 0.0025f && v.at[2] > 0.79f && v.at[2] < 0.83f && v.at[1] > 0.07f && v.at[1] < 0.092f)
                postTop = std::max(postTop, v.at[1]);
        float postZ = 0.0f;
        int postN = 0;
        for (const Vertex& v : body.vertices)
            if (v.bone == kFrameBone && std::fabs(v.at[0]) < 0.0025f && v.at[2] > 0.79f && v.at[2] < 0.83f && v.at[1] > postTop - 0.0005f)
                postZ += v.at[2], ++postN;
        postZ = postN ? postZ / postN : 0.0f;
        // The hole: the ring's inner edge each way from its middle (the cover's vertices round the line at its back end).
        float holeLeft = 1e9f, holeRight = -1e9f, holeUp = 1e9f, holeDown = -1e9f, holeZ = 0.0f;
        int holeN = 0;
        for (const Vertex& v : body.vertices) {
            if (v.bone != kCoverBone || v.at[2] < 0.195f || v.at[2] > 0.24f || std::fabs(v.at[0]) > 0.014f || v.at[1] < 0.078f || v.at[1] > 0.106f)
                continue;
            if (std::fabs(v.at[1] - 0.0917f) < 0.002f) {
                if (v.at[0] > 0.0f)
                    holeLeft = std::min(holeLeft, v.at[0]);
                else
                    holeRight = std::max(holeRight, v.at[0]);
            }
            if (std::fabs(v.at[0]) < 0.002f) {
                if (v.at[1] > 0.0917f)
                    holeUp = std::min(holeUp, v.at[1]);
                else
                    holeDown = std::max(holeDown, v.at[1]);
                holeZ += v.at[2], ++holeN;
            }
        }
        holeZ = holeN ? holeZ / holeN : 0.0f;
        const float holeMid[2] = {(holeLeft + holeRight) / 2, (holeUp + holeDown) / 2};
        // How deep the hole is: its edge's vertices (within 5.5 mm of its middle) from its back face to its front face (the notes had "at z 0.2203": its
        // front face; the vertices' mean, as holeZ is, is 0.212).
        float holeBack = 1e9f, holeFront = -1e9f;
        for (const Vertex& v : body.vertices)
            if (v.bone == kCoverBone && v.at[2] > 0.19f && v.at[2] < 0.25f && std::hypot(v.at[0] - holeMid[0], v.at[1] - holeMid[1]) < 0.0055f)
                holeBack = std::min(holeBack, v.at[2]), holeFront = std::max(holeFront, v.at[2]);
        std::snprintf(what, sizeof(what),
                      "w2 its sights: the rear aperture's hole (x %.4f to %.4f, y %.4f to %.4f; from its back face at z %.4f to its front face at %.4f, its vertices' mean %.3f), the "
                      "row's line %.2f mm from its middle; the front post's top y %.4f at z %.3f, the line %.2f mm from it; the line at the muzzle y %.4f (the row's %.3f), %.2f degrees down",
                      holeRight, holeLeft, holeDown, holeUp, holeBack, holeFront, holeZ, std::fabs(LineY(holeZ) - holeMid[1]) * 1000.0f, postTop, postZ,
                      std::fabs(LineY(postZ) - postTop) * 1000.0f, LineY(kSights[2]), kSights[1], kUp);
        Check(holeN > 2 && postN > 2 && std::fabs(holeMid[0]) < 0.0005f && std::fabs(LineY(holeZ) - holeMid[1]) < 0.0006f && std::fabs(LineY(postZ) - postTop) < 0.0006f &&
                  holeUp - holeDown > 0.006f && holeLeft - holeRight > 0.006f && kSights[0] == 0.0f && std::fabs(holeBack - 0.2060f) < 0.001f &&
                  std::fabs(holeFront - 0.2203f) < 0.0005f && holeZ > holeBack && holeZ < holeFront,
              what);
        // The game's own zoomed eye looks through the same hole: off the row's line by less than the hole's half, at its own z; and the gun's nearest
        // on the line ahead of the eye is the ring (kHeldEyes' rearZ).
        const float eyeOff = kZoomedEye[1] - LineY(kZoomedEye[2]);
        const float gameAtHole = kZoomedEye[1] + kZoomedForward[1] / kZoomedForward[2] * (holeZ - kZoomedEye[2]);
        const float gameAtPost = kZoomedEye[1] + kZoomedForward[1] / kZoomedForward[2] * (postZ - kZoomedEye[2]);
        std::snprintf(what, sizeof(what), "w2 the game's zoomed eye (0 %.3f %.3f): %.1f mm off the row's line there; its own line through the hole %.1f mm from the hole's middle and %.1f mm from the post's "
                      "top; the eye's z the row's (%.3f), the ring's back face ahead of it at the row's rear z %.3f",
                      kZoomedEye[1], kZoomedEye[2], eyeOff * 1000.0f, (gameAtHole - holeMid[1]) * 1000.0f, (gameAtPost - postTop) * 1000.0f, kEyeZ, kRearZ);
        Check(std::fabs(gameAtHole - holeMid[1]) < (holeUp - holeDown) / 2 && std::fabs(gameAtPost - postTop) < 0.001f && kZoomedEye[2] == kEyeZ && kRearZ < holeZ && kRearZ > holeZ - 0.02f, what);
        // Its muzzle, and its port where its shell effect is: by the receiver's right side, at the feed.
        Box tip;
        for (const Vertex& v : body.vertices)
            if (v.bone == kFrameBone && v.at[2] > frame.hi[2] - 0.001f)
                tip.Add(v.at);
        const float tipMid[3] = {(tip.lo[0] + tip.hi[0]) / 2, (tip.lo[1] + tip.hi[1]) / 2, tip.hi[2]};
        float side = 0.0f;
        for (const Vertex& v : body.vertices)
            if (v.bone == kFrameBone && std::fabs(v.at[1] - kPort[1]) < 0.015f && std::fabs(v.at[2] - kPort[2]) < 0.03f)
                side = std::min(side, v.at[0]);
        std::snprintf(what, sizeof(what), "w2 its muzzle's middle (%.3f %.3f %.3f), the row's (0 %.3f %.3f); the port (%.3f %.3f %.3f) %.1f mm off the receiver's right side there (x %.3f)", tipMid[0],
                      tipMid[1], tipMid[2], kMuzzle[1], kMuzzle[2], kPort[0], kPort[1], kPort[2], std::fabs(kPort[0] - side) * 1000.0f, side);
        Check(Dist(tipMid, kMuzzle) < 0.002f && kSights[2] == kMuzzle[2] && std::fabs(kPort[0] - side) < 0.012f, what);
        // The belt: its five pieces in their own bones' space as the row has them.
        const Dump belt = Load(dir, kBeltDraw[0], kBeltDraw[1], -1);
        if (!belt.read) {
            Skip("w2 no dump of its belt (mesh_2808_0_*)");
        } else {
            Box pieces[5];
            int others = 0;
            for (const Vertex& v : belt.vertices)
                if (v.bone < 5)
                    pieces[v.bone].Add(v.own);
                else
                    ++others;
            float worst = 0.0f, zWorst = 0.0f;
            for (int b = 0; b < 5; ++b) {
                const float* p = kBelt.pieces[b];
                const bool across = p[1] == p[3];  // (along x at one height, or down along y)
                if (across) {
                    worst = std::max({worst, std::fabs(pieces[b].lo[0] - p[0]), std::fabs(pieces[b].hi[0] - p[2]), std::fabs((pieces[b].lo[1] + pieces[b].hi[1]) / 2 - p[1])});
                } else {
                    worst = std::max({worst, std::fabs(pieces[b].hi[1] - p[1]), std::fabs(pieces[b].lo[1] - p[3]), std::fabs((pieces[b].lo[0] + pieces[b].hi[0]) / 2 - p[0])});
                }
                zWorst = std::max(zWorst, std::fabs((pieces[b].lo[2] + pieces[b].hi[2]) / 2 - kBelt.pieceZ));
            }
            std::snprintf(what, sizeof(what), "w2 its belt (2808/0): five pieces of %d, %d, %d, %d, %d vertices (%d on another bone), each where the row has it to %.1f mm, their rounds' middle %.1f mm "
                          "off the row's z",
                          pieces[0].count, pieces[1].count, pieces[2].count, pieces[3].count, pieces[4].count, others, worst * 1000.0f, zWorst * 1000.0f);
            Check(others == 0 && pieces[4].count > 0 && worst < 0.0015f && zWorst < 0.001f, what);
        }
        // The box: a draw of its own, alone on its palette's bone 0; its box, its rest on the frame by the two dumps (taken in the same frame).
        const Dump boxDump = Load(dir, kBoxDraw[0], kBoxDraw[1], -1);
        if (!boxDump.read) {
            Skip("w2 no dump of its box (mesh_987_27099_*)");
        } else {
            Box own;
            int others = 0;
            for (const Vertex& v : boxDump.vertices)
                own.Add(v.own), others += v.bone != kBoxBone;
            float boxOff = 0.0f;
            for (int k = 0; k < 3; ++k)
                boxOff = std::max({boxOff, std::fabs(own.lo[k] - kBoxMin[k]), std::fabs(own.hi[k] - kBoxMax[k])});
            float frameInverse[12], rest[12];
            InvertRigid(body.Bone(kFrameBone), frameInverse);
            Compose(frameInverse, boxDump.Bone(kBoxBone), rest);
            Box onFrame;
            for (const Vertex& v : boxDump.vertices) {
                float p[3];
                Apply(rest, v.own, p);
                onFrame.Add(p);
            }
            std::snprintf(what, sizeof(what), "w2 its box (987/27099): %d vertices, all on bone 0 (%d not), its box %.1f mm off the row's; its bone 0 on the frame at (%.4f %.4f %.4f), %.2f mm and %.2f "
                          "degrees off gunbodyfit's table",
                          own.count, others, boxOff * 1000.0f, rest[3], rest[7], rest[11], Apart(rest, kBoxRest) * 1000.0f, TurnBetween(rest, kBoxRest));
            Check(own.count > 300 && others == 0 && boxOff < 0.001f && Apart(rest, kBoxRest) < 0.0005f && TurnBetween(rest, kBoxRest) < 0.05f, what);
            // On the frame: its top, its middle along z, its front face; and the receiver over it, clear of its top once it's come down by the mouth.
            float receiver = 1e9f;
            for (const Vertex& v : body.vertices)
                if (v.bone == kFrameBone && v.at[2] > onFrame.lo[2] && v.at[2] < onFrame.hi[2] && v.at[0] > onFrame.lo[0] && v.at[0] < onFrame.hi[0] && v.at[1] < 0.0f &&
                    v.at[1] > onFrame.hi[1] - 0.06f)
                    receiver = std::min(receiver, v.at[1]);
            std::snprintf(what, sizeof(what), "w2 its box on the frame: x %.3f to %.3f, y %.3f to %.3f, z %.3f to %.3f (the row's top (0 %.3f %.3f), front z %.3f, the line it's taken along "
                          "from x %.3f to %.3f); the receiver over it down to y %.3f: %.1f cm of the box beside it (it slides off to the left: its way, w3)",
                          onFrame.lo[0], onFrame.hi[0], onFrame.lo[1], onFrame.hi[1], onFrame.lo[2], onFrame.hi[2], kGun.top[1], kGun.top[2], kGun.hook[2], kGun.grab[0], kGun.grabTo,
                          receiver, (onFrame.hi[1] - receiver) * 100.0f);
            Check(std::fabs(onFrame.hi[1] - kGun.top[1]) < 0.0015f && std::fabs((onFrame.lo[2] + onFrame.hi[2]) / 2 - kGun.top[2]) < 0.002f && std::fabs(onFrame.hi[2] - kGun.hook[2]) < 0.0015f &&
                      receiver < 1e8f && std::fabs(kGun.grabTo - onFrame.lo[0]) < 0.002f && kGun.grab[0] > onFrame.hi[0],
                  what);
        }
        // The carrying handle, folded over: where the shape's recipe keeps it (its turned rest) is where the dump has it.
        {
            float frameInverse[12], carryOn[12];
            InvertRigid(body.Bone(kFrameBone), frameInverse);
            Compose(frameInverse, body.Bone(kCarryBone), carryOn);
            const shaperecipes::Recipe* recipe = shaperecipes::Find("m249");
            const shaperecipes::Rest* rest = nullptr;
            for (int k = 0; recipe && k < recipe->draws[0].restCount; ++k)
                if (recipe->draws[0].rest[k].bone == kCarryBone)
                    rest = &recipe->draws[0].rest[k];
            float place = 1.0f, axes = 1.0f;
            if (rest) {
                const float d[3] = {carryOn[3] - rest->at[0], carryOn[7] - rest->at[1], carryOn[11] - rest->at[2]};
                place = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
                axes = 0.0f;
                for (int i = 0; i < 3; ++i)
                    for (int j = 0; j < 3; ++j)
                        axes = std::max(axes, std::fabs(carryOn[i * 4 + j] - rest->turn[i * 3 + j]));
            }
            const float none[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
            std::snprintf(what, sizeof(what), "w2 its carrying handle (bone 2) on the frame at (%.4f %.4f %.4f), turned %.1f degrees: the recipe's rest %.3f mm and %.5f off it (its mesh x %.3f to "
                          "%.3f, y %.3f to %.3f, z %.3f to %.3f: where a hand going for the charging handle passes)",
                          carryOn[3], carryOn[7], carryOn[11], TurnBetween(carryOn, none), place * 1000.0f, axes, carry.lo[0], carry.hi[0], carry.lo[1], carry.hi[1], carry.lo[2],
                          carry.hi[2]);
            Check(rest && rest->turned && place < 0.00005f && axes < 0.00005f, what);
        }
    }

    // ---- w3: the recordings ----
    // The gloves' mesh (the game's own left hand here, the fitted hands in w4) and the gun's shape (build\guns\m249.bin: the game's left hand on its
    // handguard against it, as tools/holdpic.cpp measures it).
    const Glove glove = LoadGloveParts(dir);
    const std::wstring shapePath = std::wstring(dir.begin(), dir.end()) + L"\\guns\\m249.bin";
    const bool haveShape = gunshape::Load(shapePath.c_str());
    auto shapeAt = [](const float* p) {
        float d = 0.05f;
        return gunshape::Distance(p, d) ? d : 0.05f;
    };
    // The game's own left hand at rest on the handguard, both recordings: its palm's and each finger's nearest to the gun's shape (mm).
    float supportPalmLeast = 1e9f, supportPalmMost = -1e9f, supportFingerMost = -1e9f, supportDeepest = 1e9f;
    int supportFrames = 0;
    // The box's way in the empty reload (each frame's box on the frame from its seat, 3.0 to 4.6 s), and the game's own hand on it as it slides off.
    struct WayFrame {
        int ms;
        float moved[12];
    };
    std::vector<WayFrame> wayFrames;
    float handOffStart[3] = {}, handOffMost = 0.0f;
    bool haveHandOff = false;
    float gameBoxHand[3] = {};
    int gameBoxHandMs = 1000;  // (how far from 2.86 s the frame it was taken from: 1000, none)
    // What rides the feed cover: a bone of a draw other than the gun's own that is the cover's bone (lidride.h), in every frame of both recordings.
    long lidDraws = 0;
    std::map<std::pair<int, int>, long> lidRiders;  // by draw: its bones that are the cover's, all frames
    for (const char* which : {"partial", "empty"}) {
        const std::string path = survey + "\\M249_" + which + ".bin";
        long frames = 0, depths = 0, wrongDepth = 0, belts = 0, boxes = 0;
        float worstDepth = 0.0f, worstDepthTurn = 0.0f;
        // The cover's turn about x and its fixed point; the handle's draw; the box's way off and back; the carrying handle never moves.
        float coverMost = 0.0f, hingeY = 0.0f, hingeZ = 0.0f, hingeOffAxis = 0.0f, hingeSpread = 0.0f, hingeY0 = 0.0f, hingeZ0 = 0.0f;
        int hingeN = 0;
        float handleMost = 0.0f, handleAside = 0.0f, handleTurn = 0.0f, carryMoved = 0.0f;
        float boxOut = 0.0f, boxTurnOut = 0.0f, boxHomeAfter = 1e9f;
        bool boxComing = false;
        float coverRest[12], handleRest[12], carryRest[12], boxRest[12];
        bool haveRest = false;
        // The belt laid (a still stretch) and hanging (the fresh box seated, before the hand takes it), against the tables; squashed by the count ever.
        float laidWorst = 0.0f, laidTurn = 0.0f, hangWorst = 0.0f, hangTurn = 0.0f;
        int laidN = 0, hangN = 0, squashed = 0;
        // The palms at rest; the game's hand on the cover's back end before it opens.
        float palms[2][3] = {}, palmWorst = 0.0f, lidHand = 1e9f;
        int palmN = 0;
        // The kick: the frame in the view against where it rested before each burst.
        struct Shot {
            int ms;
            float back, rise;
            bool done;
        };
        std::vector<Shot> shots;
        float lastFrame[12], burstRest[12];
        int lastLoaded = -1, lastRoundMs = -100000;
        bool haveLast = false;
        const bool read = Frames(path, [&](const std::vector<Record>& group) {
            // Each eye's body draw first, then its other draws (as the game drew them): the depth pass, the gloves, the body, the belt, the box.
            std::vector<const Record*> eyes;
            for (const Record& r : group)
                if (r.count == kBody[0] && r.start == kBody[1] && r.Bones() > kFrameBone)
                    eyes.push_back(&r);
            if (eyes.empty())
                return;
            ++frames;
            const Record& first = *eyes.front();
            float inverse[12], handleOn[12], coverOn[12], carryOn[12];
            InvertRigid(first.Bone(kFrameBone), inverse);
            Compose(inverse, first.Bone(kHandleBone), handleOn);
            Compose(inverse, first.Bone(kCoverBone), coverOn);
            Compose(inverse, first.Bone(kCarryBone), carryOn);
            // The eye of each other draw: the n-th of each draw goes with the n-th body (the order drawn).
            int nth[4] = {};
            const Record* box = nullptr;
            for (const Record& r : group) {
                const bool isDepth = r.count == kDepth[0] && r.start == kDepth[1], isBelt = r.count == kBeltDraw[0] && r.start == kBeltDraw[1],
                           isBox = r.count == kBoxDraw[0] && r.start == kBoxDraw[1];
                if (!isDepth && !isBelt && !isBox)
                    continue;
                const int kind = isDepth ? 0 : isBelt ? 1 : 2;
                const int e = nth[kind]++;
                if (e >= static_cast<int>(eyes.size()))
                    continue;
                const Record& eyeBody = *eyes[e];
                if (isDepth && r.Bones() > kDepthBox) {
                    ++depths;
                    // Its bones against the same eye's body, belt and box.
                    const Record* eyeBelt = nullptr;
                    const Record* eyeBox = nullptr;
                    int b = 0, x = 0;
                    for (const Record& o : group) {
                        if (o.count == kBeltDraw[0] && o.start == kBeltDraw[1] && b++ == e)
                            eyeBelt = &o;
                        if (o.count == kBoxDraw[0] && o.start == kBoxDraw[1] && x++ == e)
                            eyeBox = &o;
                    }
                    const float* expect[10] = {};
                    for (int k = 0; k < kBeltBones && eyeBelt && eyeBelt->Bones() >= kBeltBones; ++k)
                        expect[k] = eyeBelt->Bone(k);
                    expect[kDepthHandle] = eyeBody.Bone(kHandleBone), expect[kDepthCover] = eyeBody.Bone(kCoverBone), expect[kDepthCarry] = eyeBody.Bone(kCarryBone);
                    expect[kDepthFrame] = eyeBody.Bone(kFrameBone);
                    expect[kDepthBox] = eyeBox ? eyeBox->Bone(kBoxBone) : nullptr;
                    bool all = true;
                    for (int k = 0; k < 10; ++k) {
                        if (!expect[k]) {
                            all = false;
                            continue;
                        }
                        worstDepth = std::max(worstDepth, Apart(r.Bone(k), expect[k]));
                        worstDepthTurn = std::max(worstDepthTurn, TurnBetween(r.Bone(k), expect[k]));
                    }
                    wrongDepth += !all;
                } else if (isBelt && e == 0 && r.Bones() >= kBeltBones) {
                    ++belts;
                    float on[5][12];
                    for (int k = 0; k < kBeltBones; ++k) {
                        squashed += Squashed(r.Bone(k));
                        Compose(inverse, r.Bone(k), on[k]);
                    }
                    const bool still = !std::strcmp(which, "partial") && first.ms >= 9500 && first.ms <= 15900;
                    const bool hanging = !std::strcmp(which, "empty") && first.ms >= 4470 && first.ms <= 4600;
                    for (int k = 0; k < kBeltBones; ++k) {
                        if (still)
                            laidWorst = std::max(laidWorst, Apart(on[k], kBelt.laid[k])), laidTurn = std::max(laidTurn, TurnBetween(on[k], kBelt.laid[k]));
                        if (hanging)
                            hangWorst = std::max(hangWorst, Apart(on[k], kBelt.hanging[k])), hangTurn = std::max(hangTurn, TurnBetween(on[k], kBelt.hanging[k]));
                    }
                    laidN += still, hangN += hanging;
                } else if (isBox && e == 0) {
                    ++boxes;
                    box = &r;
                }
            }
            if (!haveRest) {
                std::memcpy(coverRest, coverOn, sizeof(coverRest));
                std::memcpy(handleRest, handleOn, sizeof(handleRest));
                std::memcpy(carryRest, carryOn, sizeof(carryRest));
                if (box)
                    Compose(inverse, box->Bone(kBoxBone), boxRest);
                haveRest = box != nullptr;
            }
            if (!haveRest)
                return;
            // The cover: its turn from rest (on the frame) and the point it turns about.
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
            // The handle: back along z, nothing aside.
            handleMost = std::max(handleMost, handleRest[11] - handleOn[11]);
            handleAside = std::max({handleAside, std::fabs(handleOn[3] - handleRest[3]), std::fabs(handleOn[7] - handleRest[7])});
            handleTurn = std::max(handleTurn, TurnBetween(handleOn, handleRest));
            carryMoved = std::max(carryMoved, Apart(carryOn, carryRest));
            // The box: off and back; its turn while it goes off (before it starts back).
            if (box) {
                float boxOn[12];
                Compose(inverse, box->Bone(kBoxBone), boxOn);
                const float away = Apart(boxOn, boxRest);
                if (away > boxOut)
                    boxOut = away;
                if (boxOut > 0.3f && away < boxOut - 0.05f)
                    boxComing = true;
                if (!boxComing && away > 0.002f)
                    boxTurnOut = std::max(boxTurnOut, TurnBetween(boxOn, boxRest));
                if (boxComing && first.ms > 5000)
                    boxHomeAfter = std::min(boxHomeAfter, away);
            }
            // The palms at rest, and the game's hand on the cover before it opens.
            for (const Record& r : group) {
                if (r.count != kGloves[0] || r.start != kGloves[1] || r.Bones() <= kRightRoot)
                    continue;
                float on[12], at[2][3];
                Compose(inverse, r.Bone(kRightRoot), on);
                Apply(on, kRightPalmPoint, at[0]);
                Compose(inverse, r.Bone(kLeftPalmBone), on);
                Apply(on, kLeftPalmPoint, at[1]);
                if (!std::strcmp(which, "empty") && first.ms >= 6400 && first.ms <= 10000) {
                    for (int s = 0; s < 2; ++s)
                        for (int k = 0; k < 3; ++k)
                            palms[s][k] += at[s][k];
                    ++palmN;
                    palmWorst = std::max({palmWorst, Dist(at[0], kRightPalm), Dist(at[1], kLeftPalm)});
                }
                if (!std::strcmp(which, "empty") && first.ms >= 2250 && first.ms <= 2400) {
                    float middle[3];
                    Compose(inverse, r.Bone(kLeftPalmBone), on);
                    Apply(on, kHandInGlove, middle);
                    lidHand = std::min(lidHand, Dist(middle, kBelt.lidGrab));
                }
                break;
            }
            // The box's way (the empty reload), and the game's own hand on it as it slides off (3.14 to 3.19 s): from the box's grab point, as it goes.
            if (!std::strcmp(which, "empty") && box && first.ms >= 3000 && first.ms <= 4600) {
                WayFrame w;
                w.ms = first.ms;
                float boxOn[12], restInverse[12];
                Compose(inverse, box->Bone(kBoxBone), boxOn);
                InvertRigid(boxRest, restInverse);
                Compose(boxOn, restInverse, w.moved);
                wayFrames.push_back(w);
                if (first.ms >= 3140 && first.ms <= 3190)
                    for (const Record& r : group) {
                        if (r.count != kGloves[0] || r.start != kGloves[1] || r.Bones() <= kLeftPalmBone)
                            continue;
                        float on[12], middle[3], grab[3], off[3];
                        Compose(inverse, r.Bone(kLeftPalmBone), on);
                        Apply(on, kHandInGlove, middle);
                        Apply(w.moved, kGun.grab, grab);
                        for (int k = 0; k < 3; ++k)
                            off[k] = middle[k] - grab[k];
                        if (!haveHandOff)
                            std::memcpy(handOffStart, off, sizeof(off)), haveHandOff = true;
                        handOffMost = std::max(handOffMost, Dist(off, handOffStart));
                        break;
                    }
            }
            // The game's own hand on the box where its hold is taken from (the empty reload's steadiest stretch, 2.84 to 2.905 s: magfit's "vanilla").
            if (!std::strcmp(which, "empty") && first.ms >= 2840 && first.ms <= 2905 && std::abs(first.ms - 2860) < gameBoxHandMs)
                for (const Record& r : group) {
                    if (r.count != kGloves[0] || r.start != kGloves[1] || r.Bones() <= kLeftPalmBone)
                        continue;
                    float on[12];
                    Compose(inverse, r.Bone(kLeftPalmBone), on);
                    Apply(on, kHandInGlove, gameBoxHand);
                    gameBoxHandMs = std::abs(first.ms - 2860);
                    break;
                }
            // The game's own left hand at rest on the handguard (the empty reload after it, 6.4 to 10 s; the partial one before its handle, to 3 s): the
            // left glove posed by the recording's bones 0-22 onto the frame, against the gun's shape.
            if (haveShape && glove.read && ((!std::strcmp(which, "empty") && first.ms >= 6400 && first.ms <= 10000) || (!std::strcmp(which, "partial") && first.ms <= 3000)))
                for (const Record& r : group) {
                    if (r.count != kGloves[0] || r.start != kGloves[1] || r.Bones() < handpose::kBones || r.eye != first.eye)
                        continue;
                    float bones[handpose::kBones][12];
                    for (int b = 0; b < handpose::kBones; ++b)
                        Compose(inverse, r.Bone(b), bones[b]);
                    const float none[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
                    const float palm = GapOf(glove, glove.palm, bones, none, shapeAt) * 1000.0f;
                    supportPalmLeast = std::min(supportPalmLeast, palm), supportPalmMost = std::max(supportPalmMost, palm);
                    for (int fi = 0; fi < 5; ++fi) {
                        float nearest = 1e9f;
                        for (int d = 1; d <= 3; ++d)
                            nearest = std::min(nearest, GapOf(glove, glove.part[fi][d], bones, none, shapeAt) * 1000.0f);
                        supportFingerMost = std::max(supportFingerMost, nearest);
                        supportDeepest = std::min(supportDeepest, nearest);
                    }
                    ++supportFrames;
                    break;
                }
            // What rides the feed cover: each eye's body's cover bone on its frame, against every bone of every other draw of that eye but the gun's own
            // (its body, depth pass, belt and box) and the gloves.
            for (const Record* eyeBody : eyes) {
                float eyeInverse[12], lidOn[12];
                InvertRigid(eyeBody->Bone(kFrameBone), eyeInverse);
                Compose(eyeInverse, eyeBody->Bone(kCoverBone), lidOn);
                for (const Record& r : group) {
                    if (r.eye != eyeBody->eye || (r.count == kBody[0] && r.start == kBody[1]) || (r.count == kDepth[0] && r.start == kDepth[1]) ||
                        (r.count == kBeltDraw[0] && r.start == kBeltDraw[1]) || (r.count == kBoxDraw[0] && r.start == kBoxDraw[1]) || (r.count == kGloves[0] && r.start == kGloves[1]))
                        continue;
                    ++lidDraws;
                    for (int k = 0; k < std::min(r.Bones(), 85); ++k) {
                        float on[12];
                        Compose(eyeInverse, r.Bone(k), on);
                        if (lidride::Rides(lidOn, on))
                            ++lidRiders[{r.count, r.start}];
                    }
                }
            }
            // The kick: a burst begins with a round counted half a second or more after the last; its first round's push is measured against where the
            // frame rested the frame before, up to and with the frame the next round is counted in (800 a minute: its own peak is cut there).
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
            std::snprintf(what, sizeof(what), "w3 %s: no recording", path.c_str());
            Skip(what);
            continue;
        }
        std::snprintf(what, sizeof(what), "w3 M249_%s: %ld frames, %ld depth passes (%ld without every draw to hold it to), %ld belt and %ld box draws: the depth pass's 0-4 the belt's, 5 the handle, 6 the "
                      "cover, 7 the carrying handle, 8 the frame, 9 the box, to %.2f mm and %.2f degrees",
                      which, frames, depths, wrongDepth, belts, boxes, worstDepth * 1000.0f, worstDepthTurn);
        Check(depths > 1000 && wrongDepth == 0 && worstDepth < 0.0005f && worstDepthTurn < 0.1f && belts > 500 && boxes > 500, what);
        std::snprintf(what, sizeof(what), "w3 ... its handle drawn %.1f cm straight back at most (%.2f mm aside, turned %.2f degrees; the row's travel %.1f); the carrying handle never moves (%.2f mm); the "
                      "belt never hidden by the game's count (%d squashed pieces)",
                      handleMost * 100.0f, handleAside * 1000.0f, handleTurn, kGun.leverTravel * 100.0f, carryMoved * 1000.0f, squashed);
        Check(std::fabs(handleMost - kGun.leverTravel) < 0.002f && handleAside < 0.001f && handleTurn < 0.2f && carryMoved < 0.0005f && squashed == 0, what);
        if (!std::strcmp(which, "empty")) {
            hingeY /= std::max(hingeN, 1), hingeZ /= std::max(hingeN, 1);
            std::snprintf(what, sizeof(what), "w3 ... the cover turns about x alone (%.4f off) through (y %.4f, z %.4f) in %d frames (within %.2f mm), up to %.1f degrees; the row's hinge (%.4f %.4f), %.1f degrees",
                          hingeOffAxis, hingeY, hingeZ, hingeN, hingeSpread * 1000.0f, coverMost, kBelt.lidHinge[0], kBelt.lidHinge[1], kBelt.lidOpen * 180.0f / kPi);
            Check(hingeN > 50 && hingeOffAxis < 0.001f && hingeSpread < 0.0005f && std::fabs(hingeY - kBelt.lidHinge[0]) < 0.0005f && std::fabs(hingeZ - kBelt.lidHinge[1]) < 0.0005f &&
                      std::fabs(coverMost - kBelt.lidOpen * 180.0f / kPi) < 0.5f,
                  what);
            std::snprintf(what, sizeof(what), "w3 ... its box off to %.0f cm and back (home within %.2f mm after), turned %.2f degrees at most on its way off (no turn: its way, below); the game's hand on "
                          "the cover's back end %.1f mm from the row's grab",
                          boxOut * 100.0f, boxHomeAfter * 1000.0f, boxTurnOut, lidHand * 1000.0f);
            Check(boxOut > 0.3f && boxHomeAfter < 0.0005f && boxTurnOut < 1.0f && lidHand < 0.002f, what);
            for (int s = 0; s < 2; ++s)
                for (int k = 0; k < 3; ++k)
                    palms[s][k] /= std::max(palmN, 1);
            std::snprintf(what, sizeof(what), "w3 ... the belt hanging out of the fresh box (%d frames) as the table has it to %.2f mm and %.2f degrees; the palms at rest (%d frames) right (%.3f %.3f %.3f) left "
                          "(%.3f %.3f %.3f), the row's to %.1f mm",
                          hangN, hangWorst * 1000.0f, hangTurn, palmN, palms[0][0], palms[0][1], palms[0][2], palms[1][0], palms[1][1], palms[1][2], palmWorst * 1000.0f);
            Check(hangN > 5 && hangWorst < 0.001f && hangTurn < 0.2f && palmN > 100 && palmWorst < 0.003f, what);
        } else {
            std::snprintf(what, sizeof(what), "w3 ... the belt laid in the feed, the gun still (%d frames), as the table has it to %.2f mm and %.2f degrees", laidN, laidWorst * 1000.0f, laidTurn);
            Check(laidN > 300 && laidWorst < 0.001f && laidTurn < 0.5f, what);
            const recoilguns::Row* row = recoilguns::ByBody(kBody[0]);
            std::snprintf(what, sizeof(what), "w3 ... its kick: %zu bursts from rest; the first rounds %.2f and %.2f cm back, %.2f and %.2f degrees up (until the next round's count); the row's back %.2f cm",
                          shots.size(), shots.size() > 0 ? shots[0].back * 100.0f : 0.0f, shots.size() > 1 ? shots[1].back * 100.0f : 0.0f, shots.size() > 0 ? shots[0].rise : 0.0f,
                          shots.size() > 1 ? shots[1].rise : 0.0f, row ? row->kick.back.peak * 100.0f : 0.0f);
            Check(shots.size() == 2 && row && std::fabs(shots[0].back - row->kick.back.peak) < 0.0006f && std::fabs(shots[1].back - shots[0].back) < 0.002f && shots[0].rise > 2.2f &&
                      shots[0].rise < 2.7f,
                  what);
        }
    }
    // The box's way off and on (m249_way.h) is the empty reload's own: each of its rows (the seat aside) one of the recording's frames, to a hundredth of a
    // millimetre; off: no turn, level for 8 cm to the left (free there, the last within 1 cm of the seat's height), then left and down; on: its middle back at
    // the seat's height 6.6 cm to the left, turned about the gun's length up to 5.5 degrees, settling to the seat.
    if (wayFrames.size() < 50) {
        Skip("w3 the box's way: no empty reload recorded");
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
        const float middle[3] = {(kBoxMin[0] + kBoxMax[0]) / 2, (kBoxMin[1] + kBoxMax[1]) / 2, (kBoxMin[2] + kBoxMax[2]) / 2};  // (its own space)
        float boxMiddle[3];  // on the frame, seated (the box's bone 0's rest: gunbodyfit's)
        Apply(kBoxRest, middle, boxMiddle);
        float offWorst = 0.0f, offTurn = 0.0f, dropAtFree = 0.0f, dropAfter = 0.0f, leftAtFree = 0.0f;
        for (int i = 1; i < kWay.offCount; ++i) {
            const float* pose = kWay.off[i] + 1;
            offWorst = std::max(offWorst, nearestFrame(pose));
            const float none[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
            offTurn = std::max(offTurn, TurnBetween(pose, none));
            float mid[3];
            Apply(pose, boxMiddle, mid);
            if (std::fabs(kWay.off[i][0] - kWay.free) < 1e-5f)
                dropAtFree = boxMiddle[1] - mid[1], leftAtFree = mid[0] - boxMiddle[0];
            if (i + 1 < kWay.offCount && std::fabs(kWay.off[i][0] - kWay.free) < 1e-5f) {
                float next[3];
                Apply(kWay.off[i + 1] + 1, boxMiddle, next);
                dropAfter = boxMiddle[1] - next[1];
            }
        }
        std::snprintf(what, sizeof(what), "w3 the box's way off: %d rows, each the recording's own frame to %.3f mm; turned %.2f degrees at most; free %.1f cm along, %.1f cm to the gun's left, "
                      "dropped %.1f mm there, %.1f mm by the next frame",
                      kWay.offCount, offWorst * 1000.0f, offTurn, kWay.free * 100.0f, leftAtFree * 100.0f, dropAtFree * 1000.0f, dropAfter * 1000.0f);
        Check(offWorst < 0.00002f && offTurn < 0.1f && kWay.free > 0.075f && kWay.free < 0.085f && leftAtFree > 0.075f && dropAtFree < 0.01f && dropAfter >= 0.01f, what);
        float onWorst = 0.0f, onTurnMost = 0.0f, offAxis = 0.0f, firstLeft = 0.0f, firstHigh = 1.0f;
        for (int i = 0; i < kWay.onCount; ++i) {
            const float* pose = kWay.on[i] + 1;
            if (i + 1 < kWay.onCount)
                onWorst = std::max(onWorst, nearestFrame(pose));
            const float none[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
            onTurnMost = std::max(onTurnMost, TurnBetween(pose, none));
            offAxis = std::max({offAxis, std::fabs(pose[2]), std::fabs(pose[6]), std::fabs(pose[8]), std::fabs(pose[9])});  // (about z alone)
            if (i == 0) {
                float mid[3];
                Apply(pose, boxMiddle, mid);
                firstLeft = mid[0] - boxMiddle[0], firstHigh = mid[1] - boxMiddle[1];
            }
        }
        const float* last = kWay.on[kWay.onCount - 1] + 1;
        const bool home = last[0] == 1.0f && last[5] == 1.0f && last[10] == 1.0f && last[3] == 0.0f && last[7] == 0.0f && last[11] == 0.0f;
        std::snprintf(what, sizeof(what), "w3 the box's way on: %d rows over %.2f s, each the recording's own frame to %.3f mm; from %.1f cm to the gun's left, its middle %.1f mm off the seat's "
                      "height, turned about the gun's length alone (%.4f off) up to %.2f degrees, settling to the seat %d",
                      kWay.onCount, kWay.on[kWay.onCount - 1][0], onWorst * 1000.0f, firstLeft * 100.0f, firstHigh * 1000.0f, offAxis, onTurnMost, home);
        Check(onWorst < 0.00002f && firstLeft > 0.05f && firstLeft < 0.08f && std::fabs(firstHigh) < 0.01f && offAxis < 0.002f && onTurnMost > 5.3f && onTurnMost < 5.8f && home &&
                  kWay.on[kWay.onCount - 1][0] > 0.2f && kWay.on[kWay.onCount - 1][0] < 0.35f,
              what);
        std::snprintf(what, sizeof(what), "w3 the game's own hand on the box as it slides off (3.14 to 3.19 s): it goes with it, its middle from the box's grab point changing %.1f mm at most",
                      handOffMost * 1000.0f);
        Check(haveHandOff && handOffMost < 0.01f, what);
    }
    // The game's own left hand at rest on the handguard: its palm 2 cm off the gun's shape (the M240's own: 2.8 mm, tools/holdpic.cpp), its fingers on it,
    // in every rest frame of both recordings: the game's own hold (the review of 2026-11-18: "22 mm off the handguard"), not the mod's fit. Vanilla first:
    // kept, drawn as the game holds it (skin::LauncherLeftGrip: the rig's rest, the game's own hand).
    if (!haveShape || !glove.read || !supportFrames) {
        Skip("w3 the game's own left hand on the handguard: no gun shape (build\\guns\\m249.bin) or gloves' dump");
    } else {
        std::snprintf(what, sizeof(what), "w3 the game's own left hand at rest on the handguard (%d frames, both recordings): its palm (the palm proper, as magfit has it; holdpic's, with the "
                      "knuckles, 22 mm) %.1f to %.1f mm off the gun's shape, steady; each finger's nearest %.1f mm at most (into it %.1f mm at most): the game's own hold, kept",
                      supportFrames, supportPalmLeast, supportPalmMost, supportFingerMost, -supportDeepest);
        Check(supportFrames > 100 && supportPalmLeast > 15.0f && supportPalmMost - supportPalmLeast < 2.0f && supportFingerMost < 4.0f && supportDeepest > -12.0f, what);
    }

    // ---- w4: the fitted hands ----
    {
        float boxHand[3], lidHand[3], overHand[3], underHand[3];
        MiddleOf(magposes::kM249Hold, boxHand);
        MiddleOf(magposes::kM249LidHold, lidHand);
        MiddleOf(magposes::kM249LeverOverHold, overHand);
        MiddleOf(magposes::kM249LeverUnderHold, underHand);
        std::snprintf(what, sizeof(what), "w4 the fitted hands' middles: on the box (%.3f %.3f %.3f), %.1f mm from the row's grab; on the cover (%.3f %.3f %.3f), %.1f mm from its grab; over the handle "
                      "%.1f cm from its knob, under it %.1f cm, both on the gun's right",
                      boxHand[0], boxHand[1], boxHand[2], Dist(boxHand, kGun.grab) * 1000.0f, lidHand[0], lidHand[1], lidHand[2], Dist(lidHand, kBelt.lidGrab) * 1000.0f,
                      Dist(overHand, kGun.lever) * 100.0f, Dist(underHand, kGun.lever) * 100.0f);
        // (Over the top the fingers wrap the knob from behind and above: the hand's middle sits back of it, within the grip's reach of a lever, 10 cm.)
        Check(Dist(boxHand, kGun.grab) < 0.001f && Dist(lidHand, kBelt.lidGrab) < 0.001f && Dist(overHand, kGun.lever) < 0.09f && Dist(underHand, kGun.lever) < 0.09f && overHand[0] < 0.0f &&
                  underHand[0] < 0.0f,
              what);
        // Every table a rigid turn with a move, its fingers' bones finite.
        bool sound = true;
        for (const float* hold : {&magposes::kM249Hold[0], &magposes::kM249LidHold[0], &magposes::kM249LeverOverHold[0], &magposes::kM249LeverUnderHold[0]})
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j) {
                    const float dot = hold[i * 4] * hold[j * 4] + hold[i * 4 + 1] * hold[j * 4 + 1] + hold[i * 4 + 2] * hold[j * 4 + 2];
                    sound = sound && std::fabs(dot - (i == j ? 1.0f : 0.0f)) < 0.001f;
                }
        for (const auto* fingers : {magposes::kM249Fingers, magposes::kM249LidFingers, magposes::kM249LeverOverFingers, magposes::kM249LeverUnderFingers})
            for (int b = 0; b < handpose::kBones; ++b)
                sound = sound && std::all_of(fingers[b], fingers[b] + 12, [](float f) { return std::isfinite(f); });
        Check(sound, "w4 the four holds rigid turns, their fingers' bones finite");
    }
    // The hands against the meshes (as tools/magfit.cpp measures its fits: each part's nearest to the surface, negative in it). The box's hold (the review of
    // 2026-11-18: "the hand on the box hovers", its palm 25 mm off, the middle fingertip 29 mm, where the M240's box hold has its palm at 7 and every finger
    // within 5.5 mm): on the cloth pouch, its palm within 7.5 mm, every finger on it (its nearest part within 3 mm, or pressed in), none of the hand
    // further in than the cloth gives (10 mm, never through it), and near the game's own place on the box (vanilla first: 2.86 s of its reload, moved
    // less than 3 cm). The handle from under it (its palm 23 mm off, the M240's 5): its palm within 7.5 mm of the gun or its knob, nothing of it in the gun.
    {
        const Dump bodyDump = Load(dir, kBody[0], kBody[1], kFrameBone);
        const Dump boxDump = Load(dir, kBoxDraw[0], kBoxDraw[1], -1);
        meshdump::Mesh boxMesh, gunMesh;
        if (!glove.read || !bodyDump.read || !boxDump.read || !meshdump::LoadDraw(dir, kBoxDraw[0], kBoxBone, {}, boxMesh, kBoxBone, 0, kBoxDraw[1]) ||
            !meshdump::LoadDraw(dir, kBody[0], kFrameBone, {}, gunMesh, -1, 0, kBody[1])) {
            Skip("w4 the hands against the meshes: no dumps of the gloves, the body or the box");
        } else {
            // The box on the frame, as the two dumps (taken in the same frame) have it.
            float frameInverse[12], boxOn[12];
            InvertRigid(bodyDump.Bone(kFrameBone), frameInverse);
            Compose(frameInverse, boxDump.Bone(kBoxBone), boxOn);
            float lo[3] = {1e9f, 1e9f, 1e9f}, hi[3] = {-1e9f, -1e9f, -1e9f};
            for (size_t v = 0; v + 2 < boxMesh.pos.size(); v += 3) {
                float p[3];
                Apply(boxOn, &boxMesh.pos[v], p);
                std::memcpy(&boxMesh.pos[v], p, sizeof(p));
                for (int k = 0; k < 3; ++k)
                    lo[k] = std::min(lo[k], p[k]), hi[k] = std::max(hi[k], p[k]);
            }
            for (int k = 0; k < 3; ++k)
                lo[k] -= 0.06f, hi[k] += 0.06f;
            const Field pouch = FieldOf(boxMesh, lo, hi, 0.001f);
            auto onPouch = [&](const float* p) { return pouch.At(p); };
            float toGun[12];
            InvertRigid(magposes::kM249Hold, toGun);
            const float palm = GapOf(glove, glove.palm, magposes::kM249Fingers, toGun, onPouch) * 1000.0f;
            float part[5][3], deepest = palm, farthestFinger = -1e9f;
            for (int fi = 0; fi < 5; ++fi) {
                float nearest = 1e9f;
                for (int d = 1; d <= 3; ++d) {
                    part[fi][d - 1] = GapOf(glove, glove.part[fi][d], magposes::kM249Fingers, toGun, onPouch) * 1000.0f;
                    nearest = std::min(nearest, part[fi][d - 1]);
                    deepest = std::min(deepest, part[fi][d - 1]);
                }
                farthestFinger = std::max(farthestFinger, nearest);
            }
            float boxHand[3];
            MiddleOf(magposes::kM249Hold, boxHand);
            const float fromGame = gameBoxHandMs < 1000 ? Dist(boxHand, gameBoxHand) : 1.0f;
            std::snprintf(what, sizeof(what), "w4 the hand on the box, against the pouch: palm %.1f mm; thumb %.1f %.1f %.1f; index %.1f %.1f %.1f; middle %.1f %.1f %.1f; ring %.1f %.1f %.1f; "
                          "little %.1f %.1f %.1f (- in it); each finger's nearest part %.1f mm off at most, the deepest %.1f mm in; %.1f cm from the game's own hand there",
                          palm, part[0][0], part[0][1], part[0][2], part[1][0], part[1][1], part[1][2], part[2][0], part[2][1], part[2][2], part[3][0], part[3][1], part[3][2], part[4][0],
                          part[4][1], part[4][2], farthestFinger, -deepest, fromGame * 100.0f);
            Check(palm <= 7.5f && palm >= -5.0f && farthestFinger <= 3.0f && deepest >= -10.5f && fromGame < 0.03f, what);
            // The handle from under it: the body with its handle, round the knob, 2 mm cells.
            const float near0[3] = {kGun.lever[0] - 0.12f, kGun.lever[1] - 0.12f, kGun.lever[2] - 0.14f}, near1[3] = {kGun.lever[0] + 0.10f, kGun.lever[1] + 0.10f, kGun.lever[2] + 0.14f};
            const Field gun = FieldOf(gunMesh, near0, near1, 0.002f);
            auto onGun = [&](const float* p) { return gun.At(p); };
            InvertRigid(magposes::kM249LeverUnderHold, toGun);
            const float underPalm = GapOf(glove, glove.palm, magposes::kM249LeverUnderFingers, toGun, onGun) * 1000.0f;
            float underDeepest = underPalm;
            for (int fi = 0; fi < 5; ++fi)
                for (int d = 1; d <= 3; ++d)
                    underDeepest = std::min(underDeepest, GapOf(glove, glove.part[fi][d], magposes::kM249LeverUnderFingers, toGun, onGun) * 1000.0f);
            std::snprintf(what, sizeof(what), "w4 the hand from under the handle, against the gun and its knob (2 mm cells): palm %.1f mm off, the hand's deepest %.1f mm in", underPalm,
                          -underDeepest);
            Check(underPalm <= 8.0f && underPalm >= -2.0f && underDeepest >= -4.0f, what);
        }
    }

    // ---- w5: handmag on the M249's own table ----
    {
        Reset();
        handmag::Frame f = Base();
        float glove[12], at[3];
        // The box with the cover shut: a squeeze on it takes nothing (its belt is held under the cover).
        GloveBy(magposes::kM249Hold, kHandInGlove, kGun.grab, glove);
        std::memcpy(f.glove, glove, sizeof(glove));
        handmag::Update(f);
        f.squeeze = 1.0f;
        const handmag::Result shut = handmag::Update(f);
        f.squeeze = 0.0f;
        handmag::Update(f);
        Check(!shut.grabbed && shut.beltReady, "w5 the cover shut: a squeeze on the box takes nothing, the belt ready to fire");
        // The cover taken at its back end and lifted: on by itself past its share, up.
        LidAt(0.0f, at);
        GloveBy(magposes::kM249LidHold, kHandInGlove, at, f.glove);
        handmag::Update(f);
        f.squeeze = 1.0f;
        const bool lidTaken = handmag::Update(f).lidTaken;
        bool lidUp = false, readyWhileUp = false;
        for (int i = 1; i < 90 && !lidUp; ++i) {
            LidAt(std::min(0.02f * i, 0.9f), at);
            GloveBy(magposes::kM249LidHold, kHandInGlove, at, f.glove);
            const handmag::Result r = handmag::Update(f);
            lidUp = r.lidUp;
            readyWhileUp = readyWhileUp || r.beltReady;
        }
        f.squeeze = 0.0f;
        handmag::Result r = handmag::Update(f);
        std::snprintf(what, sizeof(what), "w5 the cover taken at its back end %d, lifted till it goes on by itself: up %d (%.1f degrees), the belt not ready meanwhile %d", lidTaken, lidUp,
                      r.lid * 180.0f / kPi, !readyWhileUp);
        Check(lidTaken && lidUp && std::fabs(r.lid - kBelt.lidOpen) < 0.01f && !readyWhileUp && !r.beltReady, what);
        // The box: off along its own reload's way (m249_way.h; the review of 2026-11-18: "the box comes off a different way from the game's"). First the ways
        // it doesn't come off and the places it's taken from, each with the cover up anew; then the reload goes on from the cover up.
        const float none12[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
        auto takeAt = [&](handmag::Frame& g, const float* place) {
            GloveBy(magposes::kM249Hold, kHandInGlove, place, g.glove);
            handmag::Update(g);
            g.squeeze = 1.0f;
            return handmag::Update(g).grabbed;
        };
        // On the gun (the test's frame turned 10 degrees: its numbers to 4 places): the box drawn.
        auto boxOnGun = [&](const handmag::Result& res, float* onGun) {
            float gunInverse[12];
            InvertRigid(kGunPose, gunInverse);
            Compose(gunInverse, res.magPose, onGun);
        };
        const float boxMiddle[3] = {(kBoxMin[0] + kBoxMax[0]) / 2, (kBoxMin[1] + kBoxMax[1]) / 2 + 0.0635f, (kBoxMin[2] + kBoxMax[2]) / 2 + 0.3329f};  // on the frame, seated
        {
            // Straight down 16 cm from its end, as it came off before: it stays in (its way is to the left).
            handmag::Frame g;
            const bool up = CoverUp(g);
            const bool took = takeAt(g, kGun.grab);
            bool freed = false;
            for (int i = 1; i <= 80; ++i) {
                const float down[3] = {kGun.grab[0], kGun.grab[1] - 0.002f * i, kGun.grab[2]};
                GloveBy(magposes::kM249Hold, kHandInGlove, down, g.glove);
                freed = freed || handmag::Update(g).pulledOut;
            }
            std::snprintf(what, sizeof(what), "w5 the box taken at its end %d (the cover up %d) and drawn straight down 16 cm: it stays in %d (before: free 5.8 cm down)", took, up, !freed);
            Check(up && took && !freed, what);
            // Straight to the gun's left, level: free, where the way's free point and the hand's dead give put it.
            CoverUp(g);
            const bool took2 = takeAt(g, kGun.grab);
            float freeLeft = 0.0f;
            for (int i = 1; i <= 48 && freeLeft == 0.0f; ++i) {
                const float left[3] = {kGun.grab[0] + 0.002f * i, kGun.grab[1], kGun.grab[2]};
                GloveBy(magposes::kM249Hold, kHandInGlove, left, g.glove);
                if (handmag::Update(g).pulledOut)
                    freeLeft = 0.002f * i;
            }
            std::snprintf(what, sizeof(what), "w5 the box taken at its end %d and drawn level to the gun's left: free %.1f cm along (its way free at %.1f; before: never)", took2, freeLeft * 100.0f,
                          kWay.free * 100.0f);
            Check(took2 && freeLeft > kWay.free && freeLeft < kWay.free + 0.02f, what);
        }
        {
            // Taken anywhere along the 19 cm pouch: in its middle, and by its right end. Pulled level to the left: free; the box eased into the hand that took it
            // there (from its way's free point to the real hand over a few frames, not in one jump).
            for (const float x : {0.0f, -0.09f}) {
                handmag::Frame g;
                CoverUp(g);
                const float from[3] = {x, kGun.grab[1], kGun.grab[2]};
                const bool took = takeAt(g, from);
                bool freed = false;
                float step = 0.0f, was[3] = {}, endTo = 1.0f;
                bool haveWas = false;
                int after = 0;
                for (int i = 1; i <= 60 && after < 8; ++i) {
                    const float left[3] = {from[0] + 0.002f * std::min(i, 48), from[1], from[2]};
                    GloveBy(magposes::kM249Hold, kHandInGlove, left, g.glove);
                    const handmag::Result res = handmag::Update(g);
                    freed = freed || res.pulledOut;
                    if (res.mag) {
                        float onGun[12], grabAt[3];
                        boxOnGun(res, onGun);
                        Apply(onGun, kGun.grab, grabAt);
                        if (haveWas)
                            step = std::max(step, Dist(grabAt, was));
                        std::memcpy(was, grabAt, sizeof(was));
                        haveWas = true;
                        if (freed) {
                            ++after;
                            endTo = Dist(grabAt, left);  // (the box held as its hold has it: its grab point is the hand's middle)
                        }
                    }
                }
                std::snprintf(what, sizeof(what), "w5 the box taken %s (the hand's middle at x %.2f) %d, drawn level to the left: free %d; its largest step a frame %.1f cm, at the hand after %d "
                              "frames to %.1f mm (before: not taken)",
                              x == 0.0f ? "in its middle" : "by its right end", x, took, freed, step * 100.0f, after, endTo * 1000.0f);
                // (Eased with the glove, a third of the way a frame: the hand took it 11 and 20 cm from its hold's place.)
                Check(took && freed && step < 0.4f * std::fabs(kGun.grab[0] - x) + 0.01f && endTo < 0.001f, what);
            }
        }
        // The reload: the cover up, the box taken at its end, drawn along its own way: the box on the way's own places (as far along as the hand, less its
        // dead 6 mm), never turned, free once the way's free length is past.
        CoverUp(f);
        const bool grabbed = takeAt(f, kGun.grab);
        bool out = false;
        float freeAt = 0.0f, turned = 0.0f, offWay = 0.0f;
        for (float along = 0.002f; along <= 0.15f && !out; along += 0.002f) {
            float move[3];
            WayGrab(along, move);
            const float place[3] = {kGun.grab[0] + move[0], kGun.grab[1] + move[1], kGun.grab[2] + move[2]};
            GloveBy(magposes::kM249Hold, kHandInGlove, place, f.glove);
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
                float frameOnly[12], gunInverse[12];
                InvertRigid(kGunPose, gunInverse);
                Compose(gunInverse, kGunPose, frameOnly);
                for (int a = 0; a < 3; ++a)  // (entry by entry: the test's frame's numbers are to 4 places, which an angle from the trace makes 0.6 degrees)
                    for (int b = 0; b < 3; ++b)
                        turned = std::max(turned, std::fabs(onGun[a * 4 + b] - frameOnly[a * 4 + b]) * 180.0f / kPi);
            }
        }
        std::snprintf(what, sizeof(what), "w5 the box taken at its end with the cover up %d, drawn along its own way off: free %.1f cm along (the way's %.1f and the hand's dead 0.6), on the way's "
                      "places to %.2f mm on its way, turned %.2f degrees",
                      grabbed, freeAt * 100.0f, kWay.free * 100.0f, offWay * 1000.0f, turned);
        Check(grabbed && out && freeAt > kWay.free + 0.004f && freeAt < kWay.free + 0.010f && offWay < 0.0005f && turned < 0.1f, what);
        // Let go of (it falls), the well empty; a fresh one from the belly brought 3 cm to the left of where its way on starts and 4 cm under it: caught there,
        // and home along the way on: in from the gun's left at the seat's height, turned about the gun's length up to 5.5 degrees, settling; its belt hanging.
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
        float turnMost = 0.0f, fromLeft = 0.0f, offHeight = 0.0f, notAboutZ = 0.0f;
        bool onWay = false;  // (reached where its way on starts, turned as it comes in: from then on, its way on)
        for (int i = 0; i < 80 && !seated; ++i) {
            const float near[3] = {kGun.grab[0] + onTop[0] - kGun.top[0] + 0.03f, kGun.grab[1] + onTop[1] - kGun.top[1] - 0.04f, kGun.grab[2] + onTop[2] - kGun.top[2]};
            GloveBy(magposes::kM249Hold, kHandInGlove, near, f.glove);
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
                const float degrees = TurnBetween(turn, none12);
                turnMost = std::max(turnMost, degrees);
                Apply(onGun, boxMiddle, mid);
                Apply(frameOnly, boxMiddle, seatMid);
                // On its way on (from where it starts, turned as it comes in): at the seat's height, from the left.
                onWay = onWay || degrees > 5.4f;
                if (onWay) {
                    fromLeft = std::max(fromLeft, mid[0] - seatMid[0]);
                    offHeight = std::max(offHeight, std::fabs(mid[1] - seatMid[1]));
                    notAboutZ = std::max({notAboutZ, std::fabs(turn[2]), std::fabs(turn[6]), std::fabs(turn[8]), std::fabs(turn[9])});
                }
            }
        }
        f.wellEmpty = false;  // (the game told: a new box in)
        f.squeeze = 0.0f;
        r = handmag::Update(f);
        std::snprintf(what, sizeof(what), "w5 let go of %d; a fresh one taken at the belly %d, brought by where its way on starts: caught %d, home %d along its way on (turned up to %.2f degrees about "
                      "the gun's length, %.4f off it; in from %.1f cm to the gun's left, %.1f mm off the seat's height meanwhile; before: straight up, no turn); its belt drawn hanging %d, not "
                      "ready %d",
                      letGo, took, caught, seated, turnMost, notAboutZ, fromLeft * 100.0f, offHeight * 1000.0f, r.beltDrawn, !r.beltReady);
        Check(letGo && took && caught && seated && turnMost > 5.3f && turnMost < 5.8f && notAboutZ < 0.003f && fromLeft > 0.04f && offHeight < 0.012f && r.beltDrawn && !r.beltReady, what);
        // The belt pinched at its hanging tip (heldtest 7's: none of its own, so the belt's own tip) and brought to the feed: laid.
        float tip[3];
        handmag::HangingTip(kBelt, tip);
        GloveBy(magposes::kM249Hold, kBelt.pinch, tip, f.glove);
        handmag::Update(f);
        f.squeeze = 1.0f;
        const bool beltTaken = handmag::Update(f).beltTaken;
        bool laid = false;
        for (int i = 1; i < 120 && !laid; ++i) {
            const float s = std::min(i / 40.0f, 1.0f);
            const float to[3] = {tip[0] + (kBelt.feed[0] + 0.06f - tip[0]) * s, tip[1] + (kBelt.feed[1] + 0.02f - tip[1]) * s, tip[2] + (kBelt.feed[2] - tip[2]) * s};
            GloveBy(magposes::kM249Hold, kBelt.pinch, to, f.glove);
            laid = handmag::Update(f).beltLaid;
        }
        f.squeeze = 0.0f;
        r = handmag::Update(f);
        std::snprintf(what, sizeof(what), "w5 its belt's hanging tip (%.3f %.3f %.3f) pinched %d, brought to the feed: laid %d; not ready with the cover up %d", tip[0], tip[1], tip[2], beltTaken, laid,
                      !r.beltReady);
        Check(kBelt.testTip[0] == 0.0f && beltTaken && laid && !r.beltReady, what);
        // The cover taken up at its back end and brought down: shut by itself past its share; the belt ready.
        LidAt(kBelt.lidOpen, at);
        GloveBy(magposes::kM249LidHold, kHandInGlove, at, f.glove);
        handmag::Update(f);
        f.squeeze = 1.0f;
        handmag::Update(f);
        bool lidShut = false;
        for (int i = 1; i < 120 && !lidShut; ++i) {
            LidAt(std::max(kBelt.lidOpen - 0.02f * i, 0.3f), at);
            GloveBy(magposes::kM249LidHold, kHandInGlove, at, f.glove);
            lidShut = handmag::Update(f).lidShut;
        }
        f.squeeze = 0.0f;
        for (int i = 0; i < 20 && !lidShut; ++i)
            lidShut = handmag::Update(f).lidShut;
        r = handmag::Update(f);
        std::snprintf(what, sizeof(what), "w5 the cover brought down: shut %d, the belt laid and the cover down: ready %d", lidShut, r.beltReady);
        Check(lidShut && r.beltReady && r.lid == 0.0f, what);
        // The handle: the trigger by its knob, drawn back 11 cm, let go of: a round chambered; over the top (the fitted hand), then from under it.
        for (int way = 0; way < 2; ++way) {
            const float* hold = way ? magposes::kM249LeverUnderHold : magposes::kM249LeverOverHold;
            float knobHand[3];
            MiddleOf(hold, knobHand);
            GloveBy(hold, kHandInGlove, knobHand, f.glove);
            handmag::Update(f);
            f.trigger = 1.0f;
            const handmag::Result taken = handmag::Update(f);
            bool fingers = false;
            float back = 0.0f;
            for (int i = 1; i <= 30; ++i) {
                const float to[3] = {knobHand[0], knobHand[1], knobHand[2] - 0.11f * i / 30.0f};
                GloveBy(hold, kHandInGlove, to, f.glove);
                r = handmag::Update(f);
                back = std::max(back, r.leverBack);
                fingers = fingers || r.leverFingers == (way ? magposes::kM249LeverUnderFingers : magposes::kM249LeverOverFingers);
            }
            f.trigger = 0.0f;
            r = handmag::Update(f);
            std::snprintf(what, sizeof(what), "w5 its handle %s: taken %d, the fitted hand on it %d, back %.1f cm (of %.1f), let go %d, a round chambered %d", way ? "from under it" : "over the top",
                          taken.leverTaken, fingers, back * 100.0f, kGun.leverTravel * 100.0f, r.leverLet, r.charged);
            Check(taken.leverTaken && fingers && back > 0.10f && r.leverLet && r.charged, what);
            for (int i = 0; i < 10; ++i)
                handmag::Update(f);
        }
        // Claims agrees with Update: a squeeze of the glove anywhere near the parts (the hand turned as the box's hold or the cover's) is the M249's exactly
        // when Update takes something; with the cover shut, with it up, and with a fresh box's belt hanging.
        uint32_t seed = 249u;
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
                GloveBy(n % 2 ? magposes::kM249Hold : magposes::kM249LidHold, kHandInGlove, place, g.glove);
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
            std::snprintf(what, sizeof(what), "w5 Claims and Update agree, %s: %d squeezes round the box, the cover, the handle and the belt's tip, %d of them taking something, %d apart", states[state],
                          samples, taken, disagree);
            Check(ready && samples == 300 && taken > 20 && disagree == 0, what);
        }
        Reset();
    }

    // ---- w6: what rides the feed cover (lidride.h; the review of 2026-11-18: the bridge M249's ACOG, on the cover's bone in the game, floats when the cover
    // is lifted by hand). The cover's bone rides itself; no other bone of the gun's own draws does (its box's bone 0 lies over the cover, and is no rider);
    // no draw of the M249's or the M240's recordings but their own has a bone that rides it, in any frame (so with the cover lifted every recorded draw is
    // drawn as before, bit for bit); and the cover's turn the lifting uses (lidride::Turn, the body's own lid's) is the game's own: its rest turned 102.6
    // degrees is where its own reload has it at its most open. ----
    {
        const Dump body = Load(dir, kBody[0], kBody[1], kFrameBone);
        const Dump boxDump = Load(dir, kBoxDraw[0], kBoxDraw[1], -1);
        const Dump beltDump = Load(dir, kBeltDraw[0], kBeltDraw[1], -1);
        if (!body.read || !boxDump.read || !beltDump.read) {
            Skip("w6 no dumps of its body, box and belt: what rides its cover");
        } else {
            float frameInverse[12], cover[12];
            InvertRigid(body.Bone(kFrameBone), frameInverse);
            Compose(frameInverse, body.Bone(kCoverBone), cover);
            int others = 0, riding = 0;
            auto test = [&](const float* bone) {
                float on[12];
                Compose(frameInverse, bone, on);
                ++others;
                riding += lidride::Rides(cover, on);
            };
            for (const int b : {kHandleBone, kCarryBone, kFrameBone})
                test(body.Bone(b));
            test(boxDump.Bone(kBoxBone));
            for (int b = 0; b < kBeltBones; ++b)
                test(beltDump.Bone(b));
            float boxOn[12];
            Compose(frameInverse, boxDump.Bone(kBoxBone), boxOn);
            const bool boxOver = boxOn[3] > -0.044f && boxOn[3] < 0.055f && boxOn[7] > 0.034f && boxOn[11] > 0.191f && boxOn[11] < 0.486f;
            std::snprintf(what, sizeof(what), "w6 the cover's bone (0 %.4f %.4f) rides itself %d; of the gun's own other %d bones (its handle, carrying handle, frame, box, belt) %d ride it (its box's "
                          "bone 0 lies over the cover at (%.4f %.4f %.4f) %d)",
                          cover[7], cover[11], lidride::Rides(cover, cover), others, riding, boxOn[3], boxOn[7], boxOn[11], boxOver);
            Check(lidride::Rides(cover, cover) && riding == 0 && boxOver, what);
        }
        // The M240's recordings, as w3 counted the M249's.
        long m240Draws = 0, m240Frames = 0;
        std::map<std::pair<int, int>, long> m240Riders;
        for (const char* which : {"partial", "empty"}) {
            const std::string path = survey + "\\M240_" + which + ".bin";
            Frames(path, [&](const std::vector<Record>& group) {
                bool any = false;
                for (const Record& eyeBody : group) {
                    if (eyeBody.count != kM240Body[0] || eyeBody.start != kM240Body[1] || eyeBody.Bones() <= kM240Lid)
                        continue;
                    any = true;
                    float eyeInverse[12], lidOn[12];
                    InvertRigid(eyeBody.Bone(kM240Frame), eyeInverse);
                    Compose(eyeInverse, eyeBody.Bone(kM240Lid), lidOn);
                    for (const Record& r : group) {
                        if (r.eye != eyeBody.eye || (r.count == kM240Body[0] && r.start == kM240Body[1]) || (r.count == kM240Depth[0] && r.start == kM240Depth[1]) ||
                            (r.count == kM240Belt[0] && r.start == kM240Belt[1]) || (r.count == kM240Tape[0] && r.start == kM240Tape[1]) || (r.count == kGloves[0] && r.start == kGloves[1]))
                            continue;
                        ++m240Draws;
                        for (int k = 0; k < std::min(r.Bones(), 85); ++k) {
                            float on[12];
                            Compose(eyeInverse, r.Bone(k), on);
                            if (lidride::Rides(lidOn, on))
                                ++m240Riders[{r.count, r.start}];
                        }
                    }
                }
                m240Frames += any;
            });
        }
        auto list = [](const std::map<std::pair<int, int>, long>& riders) {
            std::string out;
            for (const auto& [key, n] : riders)
                out += (out.empty() ? "" : ", ") + std::to_string(key.first) + "/" + std::to_string(key.second) + " (" + std::to_string(n) + ")";
            return out.empty() ? std::string("none") : out;
        };
        if (!lidDraws) {
            Skip("w6 no recordings of the M249: what rides its cover in them");
        } else {
            // The M249's: one other draw has the cover's bone among its own: 96/0, drawn with the gun's own bones in its depth pass's order (the cover its bone 6,
            // or 1 in the colour pass's) in its recordings: with the cover lifted by hand, what of it is on the cover goes up with it, as in the game's own
            // reload (a draw of a sight on the cover would be the same). Every other draw: no bone of it rides it, drawn as before.
            // (Besides it, a stray frame or two of another draw's bones: what rides the cover in those frames goes with it too.)
            long others = 0;
            for (const auto& [key, n] : lidRiders)
                if (key != std::make_pair(96, 0))
                    others = std::max(others, n);
            const bool only96 = lidRiders.count({96, 0}) && lidRiders.at({96, 0}) > 1000 && others <= 4;
            std::snprintf(what, sizeof(what), "w6 the M249's recordings, every other draw's every bone (%ld draws): riding its cover %s: with the cover lifted by hand, those bones go up with it, "
                          "every other draw as before; its cover carries what rides it %d",
                          lidDraws, list(lidRiders).c_str(), kBelt.carries);
            Check(only96 && kBelt.carries, what);
        }
        // The M240's: its recordings have such draws too (its lid its colour pass's bone 5 and its depth pass's 10), but it doesn't carry them (a BeltGun's
        // own default): its lid lifted by hand as it was approved, every draw as before, until a film shows its riders.
        const handmag::BeltGun plain{};
        std::snprintf(what, sizeof(what), "w6 the M240's recordings (%ld frames, %ld draws): riding its lid %s; a lid carries nothing unless its row says so (%d): the M240's as approved",
                      m240Frames, m240Draws, list(m240Riders).c_str(), plain.carries);
        Check(!plain.carries, what);
        // The lifting's turn is the game's own: the cover's rest on the frame turned by lidride::Turn to the row's 102.6 degrees, against the empty reload's
        // cover at its most open.
        float restCover[12] = {}, openCover[12] = {}, openAngle = -1.0f;
        bool haveRestCover = false;
        Frames(survey + "\\M249_empty.bin", [&](const std::vector<Record>& group) {
            for (const Record& r : group) {
                if (r.count != kBody[0] || r.start != kBody[1] || r.Bones() <= kFrameBone)
                    continue;
                float inverse[12], on[12];
                InvertRigid(r.Bone(kFrameBone), inverse);
                Compose(inverse, r.Bone(kCoverBone), on);
                if (!haveRestCover)
                    std::memcpy(restCover, on, sizeof(on)), haveRestCover = true;
                float restInverse[12], moved[12];
                InvertRigid(restCover, restInverse);
                Compose(on, restInverse, moved);
                const float angle = std::atan2(moved[9], moved[5]);
                if (angle > openAngle)
                    openAngle = angle, std::memcpy(openCover, on, sizeof(on));
                break;
            }
        });
        if (!haveRestCover) {
            Skip("w6 no empty reload: the cover's turn");
        } else {
            float turn[12], lifted[12];
            lidride::Turn(kBelt, kBelt.lidOpen, turn);
            Compose(turn, restCover, lifted);
            std::snprintf(what, sizeof(what), "w6 the cover's rest lifted by lidride::Turn to %.1f degrees: %.2f mm and %.2f degrees from the empty reload's cover at its most open (%.1f degrees): a rider "
                          "on it goes where the game's own reload takes the cover",
                          kBelt.lidOpen * 180.0f / kPi, Apart(lifted, openCover) * 1000.0f, TurnBetween(lifted, openCover), openAngle * 180.0f / kPi);
            Check(Apart(lifted, openCover) < 0.0005f && TurnBetween(lifted, openCover) < 0.2f, what);
        }
    }

    // ---- w7: its pouch dropped (magsound.h; the review of 2026-11-18: after its first landing it rattled as metal). Cloth: the game's own ammo bag landing
    // (tools/gamesounds.cpp's bag_v0 ... bag_v6, "loose", from Bag_Collision_Wave), at every hit; every other gun's magazines as they were. ----
    {
        using magsound::Of;
        using magsound::Stuff;
        bool others = Of(nullptr) == Stuff::kMetal;
        for (const char* metal : {"m240", "m16a4", "m4a1", "kh2002", "g3a3", "mk11", "m416", "m9"})
            others = others && Of(metal) == Stuff::kMetal;
        for (const char* polymer : {"ak74", "an94", "aek971", "rpk", "aks74u", "ump45"})
            others = others && Of(polymer) == Stuff::kPlastic;
        std::snprintf(what, sizeof(what), "w7 its pouch: %s; the AK family's and the UMP-45's plastic, every other gun's (the M240's box among them) metal: %d",
                      Of("m249") == Stuff::kCloth ? "cloth" : "NOT CLOTH", others);
        Check(Of("m249") == Stuff::kCloth && others && magsound::kBagTakes == 7, what);
        // Its sound in the table of the game's own clips (the source, beside this program's project: tools\gamesounds.cpp).
        std::ifstream source("tools\\gamesounds.cpp");
        const std::string text{std::istreambuf_iterator<char>(source), std::istreambuf_iterator<char>()};
        if (text.empty()) {
            Skip("w7 tools\\gamesounds.cpp isn't here (not run from the project's folder): its bag's clips");
        } else {
            int rows = 0;
            for (int take = 0; take < magsound::kBagTakes; ++take) {
                char row[96];
                std::snprintf(row, sizeof(row), "{\"bag_v%d\", \"loose\", kBagCollision, 0.0, 9.0,", take);
                rows += text.find(row) != std::string::npos;
            }
            const bool wave = text.find("kBagCollision = \"sound/maoism/special_objects/ammo_medic_bag/bag_collision_wave\"") != std::string::npos;
            std::snprintf(what, sizeof(what), "w7 the game's own ammo bag landing in the sounds' table: %d of %d takes (bag_vN_loose), from Bag_Collision_Wave %d", rows, magsound::kBagTakes, wave);
            Check(rows == magsound::kBagTakes && wave, what);
        }
    }

    std::printf("%d checks, %d failed, %d skipped\n", g_checks, g_failed, g_skipped);
    return g_failed;
}

// The M249 (Operation Swordbreaker), held to what was measured, without the game: skin.cpp's rows for it (copied here, as the other tests keep theirs)
// against the mesh dumps and the recordings they were read from (build\mesh_24291_2808_*, 2808_0, 987_27099; build\survey\M249_partial.bin,
// M249_empty.bin), the tables written from them (src/magposes/m249*.h, src/recoilguns.h's row, src/gunbodies.h's body, src/shaperecipes.h's recipe),
// and its belt reload worked by the left hand through handmag itself (src/handmag.cpp), as the M240's is.
//   w1  the tables: one row for it in each (its recoil row: its body draw and shape, a hundred rounds, 620 m/s alone of every row, 800 a minute, its kick
//       recorded; its body; its shape's recipe; its name), every number finite
//   w2  the mesh (SKIP without the dumps): its bones (handle, cover, carrying handle, frame); the handle's knob where the row's lever is; the sights'
//       line through the rear aperture's hole and the front post's top, to the muzzle; the belt's five pieces as the row has them; the box, a draw of
//       its own: its box, its rest on the frame (gunbodyfit's), its top and front, clear of the receiver once down by the row's mouth
//   w3  the recordings (SKIP without them): the depth pass's bones (0-4 belt, 5 handle, 6 cover, 7 carrying handle, 8 frame, 9 box) in every frame; the
//       cover's hinge and its most open; the handle's draw straight back as far as the row's travel; the box off and back without a turn on its way
//       out; the belt laid and hanging as the tables have them; the palms at rest; the game's hand on the cover's back end where the lid is taken;
//       the kick of each burst's first round as the recoil row has it
//   w4  the fitted hands: the box's hold's middle at the row's grab, the cover's at its grab, the handle's over and under at the knob, none of their
//       tables broken
//   w5  handmag on the M249's own table: the whole reload by hand (cover up, box off straight down only with the cover up, a fresh one home by itself,
//       its belt hanging, pinched at its tip and laid in the feed, the cover shut, the handle over and under, a round chambered), the belt not ready
//       until all of it is done; heldtest 7's pinch on the belt's own tip
// Usage: m249test [<folder with the mesh dumps> [<folder with the recordings>]] (build and build\survey by default, from the project's folder). Reads only.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "../src/gunbodies.h"
#include "../src/gunprofile.h"
#include "../src/handmag.h"
#include "../src/handpose.h"
#include "../src/recoilguns.h"
#include "../src/shaperecipes.h"

namespace magposes {
#include "../src/magposes/m240_belt.h"  // (the belt's pinch: the glove's own, the M240's)
#include "../src/magposes/m249.h"
#include "../src/magposes/m249_belt.h"
#include "../src/magposes/m249_lever_over.h"
#include "../src/magposes/m249_lever_under.h"
#include "../src/magposes/m249_lid.h"
}

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

// ---- skin.cpp's rows for it, copied (kM249Belt, kM249Mag, kM249Bones, kHeldKinds, kHeldEyes) ----
const handmag::BeltGun kBelt{{0.0575f, 0.4751f}, 102.6f * kPi / 180.0f, {0.050f, 0.072f, 0.2205f}, magposes::kM249LidHold, magposes::kM249LidFingers, magposes::kM249BeltHanging, magposes::kM249BeltLaid, {0.4884f, 1.0245f, 0.2538f}, magposes::kBeltPinchFingers, {0.006f, 0.0553f, 0.4287f}, {0.071f, -0.050f, 0.428f},
    {{-0.0015f, -0.0445f, 0.025f, -0.0445f}, {0.0f, -0.0625f, 0.027f, -0.0625f}, {0.0f, -0.0768f, 0.024f, -0.0768f}, {0.005f, -0.079f, 0.005f, -0.105f}, {-0.003f, -0.088f, -0.003f, -0.143f}}, 0.307f};
const handmag::Gun kGun{.top = {0.0f, -0.019f, 0.425f}, .grab = {0.128f, -0.072f, 0.411f}, .paddle = {0.0f, -0.030f, 0.377f}, .hook = {0.0f, -0.020f, 0.473f}, .mouth = -0.05f,
                        .lever = {-0.0725f, 0.021f, 0.4715f}, .leverTravel = 0.129f, .rockIn = 0.0f, .handOffset = {0.0f, 0.0f, 0.0f}, .hold = magposes::kM249Hold,
                        .fingers = magposes::kM249Fingers, .button = false, .noKnock = true, .leverHold = magposes::kM249LeverOverHold,
                        .leverFingers = magposes::kM249LeverOverFingers, .belt = &kBelt, .leverUnderHold = magposes::kM249LeverUnderHold,
                        .leverUnderFingers = magposes::kM249LeverUnderFingers, .straight = true};
constexpr int kBody[2] = {24291, 2808}, kDepth[2] = {28086, 28086}, kBeltDraw[2] = {2808, 0}, kBoxDraw[2] = {987, 27099};
constexpr int kFrameBone = 3, kHandleBone = 0, kCoverBone = 1, kCarryBone = 2, kBoxBone = 0;
constexpr int kDepthBox = 9, kDepthHandle = 5, kDepthCover = 6, kDepthCarry = 7, kDepthFrame = 8, kBeltBones = 5;
constexpr float kBoxMin[3] = {-0.101f, -0.2006f, 0.0435f}, kBoxMax[3] = {0.092f, -0.0826f, 0.139f};
constexpr float kUp = 0.78f;  // degrees the sights' line lies below z
constexpr float kSights[3] = {0.0f, 0.083f, 0.935f}, kMuzzle[3] = {0.0f, 0.026f, 0.935f}, kPort[3] = {-0.030f, 0.042f, 0.422f};
constexpr float kRightPalm[3] = {-0.021f, -0.107f, 0.218f}, kLeftPalm[3] = {0.028f, -0.084f, 0.583f};
constexpr float kEyeZ = 0.088f, kRearZ = 0.205f;
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
    int count, start, ms, frame, loaded;
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
        r.count = head[0], r.start = head[3], r.ms = head[6] - first, r.frame = head[1], r.loaded = head[4];
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
// ... then the box off by the hand, let go of, and a fresh one home by itself: its belt hanging out of it.
bool BeltHanging(handmag::Frame& f) {
    if (!CoverUp(f))
        return false;
    GloveBy(magposes::kM249Hold, kHandInGlove, kGun.grab, f.glove);
    handmag::Update(f);
    f.squeeze = 1.0f;
    if (!handmag::Update(f).grabbed)
        return false;
    bool out = false;
    for (int i = 1; i < 60 && !out; ++i) {
        const float down[3] = {kGun.grab[0], kGun.grab[1] - 0.002f * i, kGun.grab[2]};
        GloveBy(magposes::kM249Hold, kHandInGlove, down, f.glove);
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
    for (int i = 0; i < 80 && !seated; ++i) {
        const float near[3] = {kGun.grab[0], kGun.grab[1] - 0.04f, kGun.grab[2]};
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
        std::snprintf(what, sizeof(what), "w1 its collision shape's recipe: gunshape ... 0.003 %s (its body from 2808, its frame bone 3, its handle and its carrying handle left out; its box and belt "
                      "are draws of their own)", arguments.c_str());
        Check(recipe && arguments == "24291@2808 3 0,2" && recipe->draws[0].vbOffset == 72624 && recipe->drawCount == 1 && recipe->draws[0].restCount == 1 &&
                  recipe->draws[0].rest[0].bone == kCoverBone,
              what);
        std::snprintf(what, sizeof(what), "w1 its name in the logs: %s (row %d)", gunprofile::NameOf("m249"), gunprofile::RowOf("m249"));
        Check(!std::strcmp(gunprofile::NameOf("m249"), "M249") && gunprofile::RowOf("m249") > gunprofile::kStingerRow, what);
        const float numbers[] = {kGun.top[1], kGun.top[2], kGun.grab[0], kGun.grab[1], kGun.grab[2], kGun.hook[2], kGun.mouth, kGun.lever[0], kGun.lever[1], kGun.lever[2], kGun.leverTravel,
                                 kBelt.lidHinge[0], kBelt.lidHinge[1], kBelt.lidOpen, kBelt.feed[0], kBelt.feed[1], kBelt.feed[2], kBelt.anchor[0], kBelt.anchor[1], kBelt.pieceZ,
                                 kSights[1], kSights[2], kUp, kEyeZ, kRearZ};
        Check(std::all_of(std::begin(numbers), std::end(numbers), [](float f) { return std::isfinite(f); }) && kGun.mouth < 0.0f && kRearZ > kEyeZ && kGun.straight && kGun.noKnock &&
                  !kGun.button && !kGun.boltCatch && kGun.rockIn == 0.0f,
              "w1 its rows' numbers finite; no magazine button, nothing knocks its box out, the box off and on straight (no rock), no bolt catch");
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
        std::snprintf(what, sizeof(what),
                      "w2 its sights: the rear aperture's hole (x %.4f to %.4f, y %.4f to %.4f at z %.3f), the row's line %.2f mm from its middle; the front post's top y %.4f at z %.3f, the line "
                      "%.2f mm from it; the line at the muzzle y %.4f (the row's %.3f), %.2f degrees down",
                      holeRight, holeLeft, holeDown, holeUp, holeZ, std::fabs(LineY(holeZ) - holeMid[1]) * 1000.0f, postTop, postZ, std::fabs(LineY(postZ) - postTop) * 1000.0f, LineY(kSights[2]),
                      kSights[1], kUp);
        Check(holeN > 2 && postN > 2 && std::fabs(holeMid[0]) < 0.0005f && std::fabs(LineY(holeZ) - holeMid[1]) < 0.0006f && std::fabs(LineY(postZ) - postTop) < 0.0006f &&
                  holeUp - holeDown > 0.006f && holeLeft - holeRight > 0.006f && kSights[0] == 0.0f,
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
            std::snprintf(what, sizeof(what), "w2 its box on the frame: x %.3f to %.3f, y %.3f to %.3f, z %.3f to %.3f (the row's top (0 %.3f %.3f), front z %.3f); the receiver over it down to y %.3f: "
                          "%.1f cm of the box beside it, clear once %.1f cm down (the row's mouth)",
                          onFrame.lo[0], onFrame.hi[0], onFrame.lo[1], onFrame.hi[1], onFrame.lo[2], onFrame.hi[2], kGun.top[1], kGun.top[2], kGun.hook[2], receiver,
                          (onFrame.hi[1] - receiver) * 100.0f, -kGun.mouth * 100.0f);
            Check(std::fabs(onFrame.hi[1] - kGun.top[1]) < 0.0015f && std::fabs((onFrame.lo[2] + onFrame.hi[2]) / 2 - kGun.top[2]) < 0.002f && std::fabs(onFrame.hi[2] - kGun.hook[2]) < 0.0015f &&
                      receiver < 1e8f && onFrame.hi[1] + kGun.mouth < receiver,
                  what);
        }
    }

    // ---- w3: the recordings ----
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
            std::snprintf(what, sizeof(what), "w3 ... its box off to %.0f cm and back (home within %.2f mm after), turned %.2f degrees at most on its way off (the row: straight, no rock); the game's hand on "
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
        // The box: taken by the hand on it, drawn straight down, free 5 cm down, never turned.
        GloveBy(magposes::kM249Hold, kHandInGlove, kGun.grab, f.glove);
        handmag::Update(f);
        f.squeeze = 1.0f;
        const bool grabbed = handmag::Update(f).grabbed;
        bool out = false;
        float freeAt = 0.0f, turned = 0.0f, aside = 0.0f;
        for (int i = 1; i < 60 && !out; ++i) {
            const float down[3] = {kGun.grab[0], kGun.grab[1] - 0.002f * i, kGun.grab[2]};
            GloveBy(magposes::kM249Hold, kHandInGlove, down, f.glove);
            r = handmag::Update(f);
            out = r.pulledOut;
            freeAt = 0.002f * i;
            if (r.mag) {
                // (Against the gun's own frame, the same way: the test's frame is turned 10 degrees, its numbers to 4 places.)
                float gunInverse[12], onGun[12], frameOnly[12];
                InvertRigid(kGunPose, gunInverse);
                Compose(gunInverse, r.magPose, onGun);
                Compose(gunInverse, kGunPose, frameOnly);
                for (int a = 0; a < 3; ++a)
                    for (int b = 0; b < 3; ++b)
                        turned = std::max(turned, std::fabs(onGun[a * 4 + b] - frameOnly[a * 4 + b]) * 180.0f / kPi);
                aside = std::max({aside, std::fabs(onGun[3]), std::fabs(onGun[11])});
            }
        }
        std::snprintf(what, sizeof(what), "w5 the box taken with the cover up %d, drawn straight down: free %.1f cm down (the row's mouth %.1f), turned %.2f degrees, %.2f mm aside on its way", grabbed,
                      freeAt * 100.0f, -kGun.mouth * 100.0f, turned, aside * 1000.0f);
        // (Aside: the row's grab is the fitted hand's middle to a millimetre; drawn on the well the box goes straight down to the bit.)
        Check(grabbed && out && std::fabs(freeAt - (-kGun.mouth + 0.006f)) < 0.004f && turned < 0.01f && aside < 0.0006f, what);
        // Let go of (it falls), the well empty; a fresh one from the belly brought under the seat: home by itself, its belt hanging out of it.
        f.wellEmpty = true;
        f.squeeze = 0.0f;
        r = handmag::Update(f);
        const bool letGo = r.letGo;
        f.squeeze = 1.0f;
        f.shoulder = true;
        const bool took = handmag::Update(f).took;
        f.shoulder = false;
        bool caught = false, seated = false;
        for (int i = 0; i < 80 && !seated; ++i) {
            const float near[3] = {kGun.grab[0], kGun.grab[1] - 0.04f, kGun.grab[2]};
            GloveBy(magposes::kM249Hold, kHandInGlove, near, f.glove);
            r = handmag::Update(f);
            caught = caught || r.caught;
            seated = r.seated;
        }
        f.wellEmpty = false;  // (the game told: a new box in)
        f.squeeze = 0.0f;
        r = handmag::Update(f);
        std::snprintf(what, sizeof(what), "w5 let go of %d; a fresh one taken at the belly %d, 4 cm under the seat: caught %d, home %d; its belt drawn hanging %d, not ready %d", letGo, took, caught, seated,
                      r.beltDrawn, !r.beltReady);
        Check(letGo && took && caught && seated && r.beltDrawn && !r.beltReady, what);
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

    std::printf("%d checks, %d failed, %d skipped\n", g_checks, g_failed, g_skipped);
    return g_failed;
}

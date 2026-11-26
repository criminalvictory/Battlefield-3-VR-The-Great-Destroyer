// The Saiga 12K (Operation Swordbreaker), held to what was measured, without the game: skin.cpp's own rows for it (src/saiga12krows.h, which skin.cpp
// builds its rows from) against its mesh dumps (build\mesh_14970_0_*, the gloves' 13755_15051, and its magazine's three draws mesh_1176_0_*,
// 450_1176_*, 1626_1626_*: a mesh of its own, which the game never drew where the gun was recorded, so made from the game's own files by
// gamesounds --mesh), its recordings (build\survey\SAIGA12K_partial.bin, SAIGA12K_dry.bin, SAIGA12K_zoom.txt) and the tables written from them
// (src/magposes/saiga12k*.h, src/recoilguns.h's row and tools/recoil/kick.tsv's, src/gunbodies.h's body, src/shaperecipes.h's recipe, the sound cuts in
// tools/gamesounds.cpp).
//   s1  the tables: one row for it in each (its recoil row: its body draw and shape, seven rounds, nine pellets, 300 m/s, 220 a minute, its kick recorded;
//       its body with its magazine, 3.5 kg empty and 45 g a shell, six to a magazine; its dropped magazine; its shape's recipe without its bolt; its name
//       after the AT4's; its magazine plastic to the ear), every number finite
//   s2  the meshes (SKIP without the dumps): its body's three bones (the bolt, the trigger, the frame) and the bolt at rest; its magazine's three draws:
//       the box on its bone 0, the shells on theirs, their depth pass on the two; the magazine's own box as the rows have it
//   s3  the seat: the magazine at the rows' seat in the receiver's opening (its edge at y -0.0060 in both meshes): inside it all round within 2 mm, its top
//       3.5 cm up inside the receiver
//   s4  its numbers for the hand (handmag::Gun) on the meshes: the top, the middle below the receiver, the front edge it rocks about, the back edge at its
//       three heights, the paddle behind it; the bolt's knob where the lever is
//   s5  the sights: the rear notch and the front post on the line, level; the muzzle's middle; the game's zoomed eye on the line; the first of the gun on
//       the line ahead of the eye (kHeldEyes' rearZ)
//   s6  the recordings (SKIP without them): the palms at rest; the game's own hand still on the seated magazine before it pulls it (both reloads), near the
//       grab; the bolt's travel firing (9.1 cm) and in the game's own pull (10.0 cm: the lever's travel); the counts: seven, six after an empty reload
//   s7  the kick: each of its seven rounds fired dry, on its frame against where it rested just before: its rise and back, and the first one's as the
//       recoil row (kick.tsv) has it
//   s8  the fitted hands: the magazine's hold against the magazine at its seat (its palm within 3 mm, every finger on it, none of it in it past a glove's
//       2.5 mm), near the game's own hand on it; the handle's over (as the game's own: its last fingers on the knob) and under (its palm within 11 mm, the
//       index on the knob) against the body; none of the tables broken
//   s9  the sound cuts: tools\gamesounds.cpp's four rows for it, and the bones' moments of its own empty reload, put on the wave's clock (the hand leaving
//       the handguard), inside them: the magazine pulled, the bolt drawn, the bolt home
// Usage: saiga12ktest [<folder with the mesh dumps> [<folder with the recordings>]] (build and build\survey by default, from the project's folder). Reads only.

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
#include "../src/handpose.h"
#include "../src/loose.h"
#include "../src/magposes.h"
#include "../src/magsound.h"
#include "../src/recoilguns.h"
#include "../src/saiga12krows.h"
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
namespace rows = saiga12krows;
const handmag::Gun& kGun = rows::kMag;
constexpr int kGloves[2] = {13755, 15051};
constexpr int kRightRoot = 37, kLeftPalmBone = 1;
constexpr float kRightPalmPoint[3] = {-0.505f, 1.129f, 0.150f}, kLeftPalmPoint[3] = {0.537f, 1.098f, 0.207f};
constexpr float kHandInGlove[3] = {0.5363f, 1.0976f, 0.2092f};  // handmag.cpp's: the hand's middle
// The receiver's opening for the magazine, as both meshes have its edge: y -0.0060 on the frame.
constexpr float kOpeningY = -0.0060f;

float Half(uint16_t h) {
    const int s = (h >> 15) & 1, e = (h >> 10) & 0x1F, m = h & 0x3FF;
    const float v = e == 0 ? std::ldexp(m / 1024.0f, -14) : std::ldexp(1 + m / 1024.0f, e - 15);
    return s ? -v : v;
}
std::vector<uint8_t> ReadAll(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
std::string ReadText(const std::string& path) {
    const auto bytes = ReadAll(path);
    return {bytes.begin(), bytes.end()};
}
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
bool Finite(const float* v, int n) {
    for (int i = 0; i < n; ++i)
        if (!std::isfinite(v[i]))
            return false;
    return true;
}

// A mesh dump's draw: each vertex once, as stored (its own bone's space), with the bone it goes with. Stride 48 or 16: a half position at 0, four bones
// at 8, four weights at 12.
struct Vertex {
    float at[3];
    int bone;
};
struct Dump {
    bool read = false;
    unsigned stride = 0;
    std::vector<uint8_t> bones;
    std::vector<Vertex> vertices;
    std::vector<int> tris;  // into vertices
    const float* Bone(int b) const { return reinterpret_cast<const float*>(bones.data()) + b * 12; }
    int Bones() const { return static_cast<int>(bones.size() / 48); }
};
Dump Load(const std::string& dir, int count, int start) {
    Dump d;
    const std::string name = dir + "\\mesh_" + std::to_string(count) + "_" + std::to_string(start) + "_";
    const auto vb = ReadAll(name + "vb0.bin"), ib = ReadAll(name + "ib.bin");
    d.bones = ReadAll(name + "bones.bin");
    std::ifstream text(name + "dump.txt");
    std::string word;
    unsigned offset = 0;
    while (text >> word)
        if (word == "vb") {
            int slot = -1;
            std::string a, b;
            unsigned s = 0, o = 0;
            if (text >> slot >> a >> s >> b >> o && slot == 0)
                d.stride = s, offset = o;
            break;
        }
    if (vb.empty() || ib.size() < static_cast<size_t>(start + count) * 2 || d.bones.size() < 48 || (d.stride != 48 && d.stride != 16))
        return d;
    std::vector<int> index(65536, -1);
    for (int i = start; i < start + count; ++i) {
        uint16_t v;
        std::memcpy(&v, &ib[2 * static_cast<size_t>(i)], 2);
        if (index[v] < 0) {
            const size_t o = offset + static_cast<size_t>(v) * d.stride;
            if (o + 16 > vb.size())
                return d;
            uint16_t h[3];
            std::memcpy(h, &vb[o], 6);
            Vertex x{};
            x.at[0] = Half(h[0]), x.at[1] = Half(h[1]), x.at[2] = Half(h[2]);
            int most = -1;
            for (int s = 0; s < 4; ++s)
                if (vb[o + 12 + s] > most)
                    most = vb[o + 12 + s], x.bone = vb[o + 8 + s];
            index[v] = static_cast<int>(d.vertices.size());
            d.vertices.push_back(x);
        }
        d.tris.push_back(index[v]);
    }
    d.read = true;
    return d;
}
int OnBone(const Dump& d, int bone) {
    return static_cast<int>(std::count_if(d.vertices.begin(), d.vertices.end(), [&](const Vertex& v) { return v.bone == bone; }));
}
// The magazine's triangles at the seat on the frame (as a mesh for the distance field, and as triangles to cut at a height).
void MagazineOnFrame(const Dump& mag, meshdump::Mesh& out) {
    out = {};
    for (const Vertex& v : mag.vertices) {
        for (int k = 0; k < 3; ++k)
            out.pos.push_back(v.at[k] + rows::kSeat[k]);
        out.part.push_back(0);
    }
    out.tris = mag.tris;
}
// Where a mesh's triangles cross the height y: the furthest back and forward (z) and across (x) of the crossings, and how many.
struct Cut {
    float zLo = 1e9f, zHi = -1e9f, xLo = 1e9f, xHi = -1e9f;
    int n = 0;
};
Cut CutAt(const meshdump::Mesh& m, float y, float xNear = 1.0f) {
    Cut c;
    for (size_t t = 0; t + 2 < m.tris.size(); t += 3)
        for (int e = 0; e < 3; ++e) {
            const float* p = &m.pos[static_cast<size_t>(m.tris[t + e]) * 3];
            const float* q = &m.pos[static_cast<size_t>(m.tris[t + (e + 1) % 3]) * 3];
            if ((p[1] - y) * (q[1] - y) >= 0.0f)
                continue;
            const float s = (y - p[1]) / (q[1] - p[1]), x = p[0] + s * (q[0] - p[0]), z = p[2] + s * (q[2] - p[2]);
            if (std::fabs(x - kGun.top[0]) > xNear)
                continue;
            c.zLo = std::min(c.zLo, z), c.zHi = std::max(c.zHi, z), c.xLo = std::min(c.xLo, x), c.xHi = std::max(c.xHi, x);
            ++c.n;
        }
    return c;
}

// A recording (weapon.txt "allrec"): records of 8 ints (the draw's count, the frame's serial, the eye, its start, the rounds loaded, the spare, ms, bones)
// and that many bones; each frame's records together.
struct Record {
    int count, start, ms, frame, loaded;
    std::vector<float> bones;
    const float* Bone(int b) const { return &bones[static_cast<size_t>(b) * 12]; }
    int Bones() const { return static_cast<int>(bones.size() / 12); }
};
struct Frame {
    int ms = 0, loaded = -1;
    bool haveBody = false, haveGloves = false;
    float frame[12] = {}, bolt[12] = {}, left[12] = {}, right[12] = {};  // the body's frame and bolt; the gloves' left palm bone and right root
};
std::vector<Frame> Recording(const std::string& path) {
    std::vector<Frame> out;
    FILE* f = nullptr;
    if (fopen_s(&f, path.c_str(), "rb") || !f)
        return out;
    int head[8], first = -1, serial = -1;
    std::vector<float> bones;
    while (fread(head, 4, 8, f) == 8) {
        const int n = head[7];
        if (n <= 0 || n > 4096)
            break;
        bones.resize(static_cast<size_t>(n) * 12);
        if (fread(bones.data(), 48, n, f) != static_cast<size_t>(n))
            break;
        if (first < 0)
            first = head[6];
        if (out.empty() || head[1] != serial) {
            out.emplace_back();
            out.back().ms = head[6] - first;
            serial = head[1];
        }
        Frame& fr = out.back();
        if (head[0] == static_cast<int>(rows::kBody) && head[3] == static_cast<int>(rows::kBodyStart) && n > rows::kFrameBone) {
            std::memcpy(fr.frame, &bones[rows::kFrameBone * 12], 48);
            std::memcpy(fr.bolt, &bones[rows::kBoltBone * 12], 48);
            fr.loaded = head[4];
            fr.haveBody = true;
        } else if (head[0] == kGloves[0] && head[3] == kGloves[1] && n > kRightRoot) {
            std::memcpy(fr.left, &bones[kLeftPalmBone * 12], 48);
            std::memcpy(fr.right, &bones[kRightRoot * 12], 48);
            fr.haveGloves = true;
        }
    }
    fclose(f);
    // Not first-person? (Only frames with both draws are used.)
    return out;
}
// A frame's place on its own frame bone: its left hand's middle, its palms, its bolt's z.
void OnFrame(const Frame& fr, const float* bindPoint, bool left, float* out) {
    float inverse[12], on[12];
    InvertRigid(fr.frame, inverse);
    Compose(inverse, left ? fr.left : fr.right, on);
    Apply(on, bindPoint, out);
}
float BoltZ(const Frame& fr) {
    float inverse[12], on[12];
    InvertRigid(fr.frame, inverse);
    Compose(inverse, fr.bolt, on);
    return on[11];
}

// ---- The left glove against a mesh, as tools/magfit.cpp measures its fits (tools/m249test.cpp's own) ----
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
float GapOf(const Glove& g, const std::vector<int>& set, const float (*bones)[12], const float* toGun, const Field& field) {
    float gap = 1.0f;
    for (int i : set) {
        float p[3];
        PoseVertex(g.vertices[static_cast<size_t>(i)], bones, toGun, p);
        gap = std::min(gap, field.At(p));
    }
    return gap;
}
// Each part's gap (palm, then [finger][segment]) and the nearest part of each finger, the deepest of all.
struct Gaps {
    float palm = 0.0f, part[5][3] = {}, nearest[5] = {}, deepest = 0.0f, farthestFinger = 0.0f;
};
Gaps GapsOf(const Glove& g, const float* hold, const float (*fingers)[12], const Field& field) {
    Gaps out;
    float toGun[12];
    InvertRigid(hold, toGun);
    out.palm = GapOf(g, g.palm, fingers, toGun, field) * 1000.0f;
    out.deepest = out.palm;
    out.farthestFinger = -1e9f;
    for (int f = 0; f < 5; ++f) {
        out.nearest[f] = 1e9f;
        for (int d = 1; d <= 3; ++d) {
            out.part[f][d - 1] = GapOf(g, g.part[f][d], fingers, toGun, field) * 1000.0f;
            out.nearest[f] = std::min(out.nearest[f], out.part[f][d - 1]);
            out.deepest = std::min(out.deepest, out.part[f][d - 1]);
        }
        out.farthestFinger = std::max(out.farthestFinger, out.nearest[f]);
    }
    return out;
}
// A hold's hand's middle on the gun's frame.
void MiddleOf(const float* hold, float* out) {
    float inGun[12];
    InvertRigid(hold, inGun);
    Apply(inGun, kHandInGlove, out);
}

}  // namespace

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "build", survey = argc > 2 ? argv[2] : "build\\survey";
    char what[1200];

    // ---- s1: the tables ----
    {
        const recoilguns::Row* row = recoilguns::ByName("Weapons/SAIGA20K/Saiga_20K");
        const bool known = row && row->body == rows::kBody && row->shape && !std::strcmp(row->shape, rows::kShape) && recoilguns::ByBody(rows::kBody) == row &&
                           recoilguns::ByShape(rows::kShape) == row;
        std::snprintf(what, sizeof(what), "s1 its recoil row: body %u, shape %s, %d rounds of %d pellets at %.0f m/s, %.0f a minute; its kick %.2f cm back, %.2f deg up (recorded)",
                      row ? row->body : 0u, row && row->shape ? row->shape : "none", row ? row->magazine : 0, row ? row->pellets : 0, row ? row->speed : 0.0f, row ? row->rate : 0.0f,
                      row ? row->kick.back.peak * 100.0f : 0.0f, row ? row->kick.rise.peak : 0.0f);
        Check(known && row->magazine == 7 && row->pellets == 9 && row->speed == 300.0f && row->rate == 220.0f && std::fabs(row->kick.back.peak - 0.0069f) < 1e-5f &&
                  row->kick.rise.peak > 5.5f && row->kick.rise.peak < 7.0f && row->kick.heldShare == 0.0f,
              what);
        const gunbodies::Body* body = gunbodies::For(rows::kShape);
        const loose::Spec* dropped = loose::SpecFor(rows::kShape);
        bool boxSame = dropped != nullptr;
        for (int k = 0; boxSame && k < 3; ++k)
            boxSame = dropped->boxMin[k] == rows::kBoxMin[k] && dropped->boxMax[k] == rows::kBoxMax[k];
        std::snprintf(what, sizeof(what), "s1 its body (%s: %.1f kg empty, %.0f g a shell, %d to a magazine, %d hull points, down to y %.3f: its magazine in it) and its dropped "
                      "magazine (its box the rows', %d shells, %.2f kg empty); its name \"%s\" in row %d, after the AT4's; its magazine %s to the ear",
                      body && body->spec ? body->spec->name : "none", body && body->spec ? body->spec->massEmpty : 0.0f, body && body->spec ? body->spec->massRound * 1000.0f : 0.0f,
                      body && body->spec ? body->spec->capacity : 0, body && body->spec ? body->spec->hullPoints : 0, body ? body->hullMin[1] : 0.0f, dropped ? dropped->capacity : 0,
                      dropped ? dropped->massEmpty : 0.0f, gunprofile::RowOf(rows::kShape) >= 0 ? gunprofile::kNames[gunprofile::RowOf(rows::kShape)][1] : "none",
                      gunprofile::RowOf(rows::kShape), magsound::Of(rows::kShape) == magsound::Stuff::kPlastic ? "plastic" : "NOT PLASTIC");
        Check(body && body->spec && body->spec->massEmpty == 3.5f && body->spec->massRound == 0.045f && body->spec->capacity == 6 && body->hullMin[1] < -0.125f && boxSame &&
                  dropped->capacity == 6 && gunprofile::RowOf(rows::kShape) == gunprofile::kAt4Row + 1 && !std::strcmp(gunprofile::kNames[gunprofile::kAt4Row + 1][1], "Saiga 12K") &&
                  magsound::Of(rows::kShape) == magsound::Stuff::kPlastic,
              what);
        const shaperecipes::Recipe* recipe = shaperecipes::Find(rows::kShape);
        Check(recipe && recipe->frameBone == rows::kFrameBone && !std::strcmp(recipe->leaveOut, "0") && recipe->drawCount == 1 && recipe->draws[0].count == rows::kBody &&
                  recipe->draws[0].start == rows::kBodyStart && recipe->draws[0].ibBytes == 59880 && recipe->draws[0].vbBytes == 282224 && recipe->draws[0].restCount == 1 &&
                  recipe->draws[0].rest[0].bone == rows::kTriggerBone,
              "s1 its shape's recipe: its body 14970/0 (its buffers 59880 and 282224 bytes) on its frame bone without its bolt (0), its trigger kept as it rests");
        bool finite = Finite(kGun.top, 3) && Finite(kGun.grab, 3) && Finite(kGun.paddle, 3) && Finite(kGun.hook, 3) && Finite(kGun.lever, 3) && Finite(&kGun.back[0][0], 6) &&
                      Finite(rows::kSeat, 3) && Finite(rows::kSights, 3) && Finite(rows::kMuzzle, 3) && Finite(rows::kPort, 3) && Finite(rows::kRightPalm, 3) &&
                      Finite(rows::kLeftPalm, 3) && Finite(rows::kBoxMin, 3) && Finite(rows::kBoxMax, 3);
        for (const float* hold : {magposes::kSaiga12kHold, magposes::kSaiga12kLeverHold, magposes::kSaiga12kLeverUnderHold}) {
            finite = finite && Finite(hold, 12);
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j) {
                    float dot = 0.0f;
                    for (int k = 0; k < 3; ++k)
                        dot += hold[i * 4 + k] * hold[j * 4 + k];
                    finite = finite && std::fabs(dot - (i == j ? 1.0f : 0.0f)) < 0.001f;
                }
        }
        for (const auto* fingers : {magposes::kSaiga12kFingers, magposes::kSaiga12kLeverFingers, magposes::kSaiga12kLeverUnderFingers})
            for (int b = 0; b < handpose::kBones; ++b)
                finite = finite && Finite(fingers[b], 12);
        Check(finite && !kGun.button && !kGun.noKnock && !kGun.straight && !kGun.boltCatch && kGun.hold && kGun.leverHold && kGun.leverUnderHold && kGun.mouth < 0.0f &&
                  std::fabs(kGun.rockIn - 25.0f * kPi / 180.0f) < 1e-4f,
              "s1 its numbers finite, its three holds rigid turns; the AK family's block: no button, a fresh one can knock it out, rocked 25 degrees, no hold-open");
    }

    // ---- s2: the meshes ----
    const Dump body = Load(dir, rows::kBody, rows::kBodyStart);
    const Dump mag = Load(dir, rows::kMagCount, rows::kMagStart);
    const Dump shells = Load(dir, rows::kShellsCount, rows::kShellsStart);
    const Dump magDepth = Load(dir, rows::kMagDepthCount, rows::kMagDepthStart);
    meshdump::Mesh frameMesh, magMesh;
    bool meshes = body.read && mag.read;
    if (!meshes) {
        Skip("s2-s5, s8: no dumps of its body (mesh_14970_0_*) or its magazine (mesh_1176_0_*: gamesounds --mesh weapons/saiga20k/saiga_20k_regularmag_1p_mesh "
             "--seat 14970@0 2 0 0.0989 0.376 --out build)");
    } else {
        const int bolt = OnBone(body, rows::kBoltBone), trigger = OnBone(body, rows::kTriggerBone), frame = OnBone(body, rows::kFrameBone);
        float inverse[12], boltRest[12];
        InvertRigid(body.Bone(rows::kFrameBone), inverse);
        Compose(inverse, body.Bone(rows::kBoltBone), boltRest);
        std::snprintf(what, sizeof(what), "s2 its body 14970/0: %d vertices on the bolt (0), %d on the trigger (1), %d on the frame (2), no other bone; the bolt at rest on the frame at (%.3f %.3f %.3f)",
                      bolt, trigger, frame, boltRest[3], boltRest[7], boltRest[11]);
        Check(body.stride == 48 && bolt == 101 && trigger == 70 && frame == 4752 && bolt + trigger + frame == static_cast<int>(body.vertices.size()) && std::fabs(boltRest[3]) < 0.001f &&
                  std::fabs(boltRest[7] + 0.041f) < 0.001f && std::fabs(boltRest[11] - 0.071f) < 0.001f,
              what);
        float lo[3] = {1e9f, 1e9f, 1e9f}, hi[3] = {-1e9f, -1e9f, -1e9f};
        for (const Vertex& v : mag.vertices)
            for (int k = 0; k < 3; ++k)
                lo[k] = std::min(lo[k], v.at[k]), hi[k] = std::max(hi[k], v.at[k]);
        float boxOff = 0.0f;
        for (int k = 0; k < 3; ++k)
            boxOff = std::max({boxOff, std::fabs(lo[k] - rows::kBoxMin[k]), std::fabs(hi[k] - rows::kBoxMax[k])});
        const bool otherDraws = shells.read && magDepth.read;
        std::snprintf(what, sizeof(what), "s2 its magazine, a mesh of its own: the box 1176/0 (stride %u) %d vertices, all on its bone 0; its shells 450/1176 %d, all on their bone 0; their depth pass "
                      "1626/1626 (stride %u) %d on bone 0, %d on bone 1; the box's own bounds the rows' to %.2f mm",
                      mag.stride, static_cast<int>(mag.vertices.size()), otherDraws ? static_cast<int>(shells.vertices.size()) : 0, otherDraws ? magDepth.stride : 0u,
                      otherDraws ? OnBone(magDepth, rows::kDepthMagBone) : 0, otherDraws ? OnBone(magDepth, rows::kDepthShellsBone) : 0, boxOff * 1000.0f);
        Check(otherDraws && mag.stride == 48 && OnBone(mag, rows::kMagBone) == 454 && mag.vertices.size() == 454 && shells.stride == 48 && OnBone(shells, rows::kShellsBone) == 134 &&
                  shells.vertices.size() == 134 && magDepth.stride == 16 && OnBone(magDepth, rows::kDepthMagBone) == 200 && OnBone(magDepth, rows::kDepthShellsBone) == 77 &&
                  boxOff < 0.0001f,
              what);
        // The body on the frame, each vertex by its own bone as the dump's bones rest (the frame's own triangles for the cuts), and the magazine at its seat.
        for (const Vertex& v : body.vertices) {
            float on[12], p[3];
            Compose(inverse, body.Bone(v.bone), on);
            Apply(on, v.at, p);
            for (int k = 0; k < 3; ++k)
                frameMesh.pos.push_back(p[k]);
            frameMesh.part.push_back(v.bone);
        }
        for (size_t t = 0; t + 2 < body.tris.size(); t += 3)
            if (body.vertices[static_cast<size_t>(body.tris[t])].bone == rows::kFrameBone && body.vertices[static_cast<size_t>(body.tris[t + 1])].bone == rows::kFrameBone &&
                body.vertices[static_cast<size_t>(body.tris[t + 2])].bone == rows::kFrameBone)
                for (int e = 0; e < 3; ++e)
                    frameMesh.tris.push_back(body.tris[t + e]);
        MagazineOnFrame(mag, magMesh);
    }

    // ---- s3: the seat ----
    if (meshes) {
        // The receiver's opening: the frame's vertices on its edge's height near the well, and the magazine's own at the same height.
        float oxLo = 1e9f, oxHi = -1e9f, ozLo = 1e9f, ozHi = -1e9f;
        int edge = 0;
        for (size_t v = 0; v + 2 < frameMesh.pos.size(); v += 3) {
            const float* p = &frameMesh.pos[v];
            if (frameMesh.part[v / 3] != rows::kFrameBone || std::fabs(p[1] - kOpeningY) > 0.0002f || std::fabs(p[0] - kGun.top[0]) > 0.03f || p[2] < 0.39f || p[2] > 0.50f)
                continue;
            oxLo = std::min(oxLo, p[0]), oxHi = std::max(oxHi, p[0]), ozLo = std::min(ozLo, p[2]), ozHi = std::max(ozHi, p[2]);
            ++edge;
        }
        float mxLo = 1e9f, mxHi = -1e9f, mzLo = 1e9f;
        int ring = 0;
        for (size_t v = 0; v + 2 < magMesh.pos.size(); v += 3) {
            const float* p = &magMesh.pos[v];
            if (std::fabs(p[1] - kOpeningY) > 0.0003f)
                continue;
            mxLo = std::min(mxLo, p[0]), mxHi = std::max(mxHi, p[0]), mzLo = std::min(mzLo, p[2]);
            ++ring;
        }
        float topY = -1e9f;
        for (size_t v = 0; v + 2 < magMesh.pos.size(); v += 3)
            topY = std::max(topY, magMesh.pos[v + 1]);
        const float left = (oxHi - mxHi) * 1000.0f, right = (mxLo - oxLo) * 1000.0f, back = (mzLo - ozLo) * 1000.0f;
        std::snprintf(what, sizeof(what), "s3 the seat (%.4f %.4f %.4f): the receiver's opening's back corners at y %.4f (%d of the frame's vertices: x %.4f and %.4f, z %.4f) hold the magazine's own "
                      "back corners at that height (%d vertices: x %.4f to %.4f, back z %.4f): %.2f mm clear on its left, %.2f on its right, %.2f at its back; its top %.1f cm up inside the receiver",
                      rows::kSeat[0], rows::kSeat[1], rows::kSeat[2], kOpeningY, edge, oxLo, oxHi, ozLo, ring, mxLo, mxHi, mzLo, left, right, back, (topY - kOpeningY) * 100.0f);
        Check(edge >= 2 && ring >= 4 && ozHi - ozLo < 0.0005f && left >= 0.2f && left <= 2.0f && right >= 0.2f && right <= 2.0f && back >= 0.2f && back <= 2.0f &&
                  topY - kOpeningY > 0.03f && topY - kOpeningY < 0.04f,
              what);
    }

    // ---- s4: its numbers for the hand ----
    if (meshes) {
        float topY = -1e9f, bottomY = 1e9f, topZLo = 1e9f, topZHi = -1e9f;
        for (size_t v = 0; v + 2 < magMesh.pos.size(); v += 3)
            topY = std::max(topY, magMesh.pos[v + 1]), bottomY = std::min(bottomY, magMesh.pos[v + 1]);
        for (const float y : {0.005f, 0.015f}) {
            const Cut c = CutAt(magMesh, y);
            topZLo = std::min(topZLo, c.zLo), topZHi = std::max(topZHi, c.zHi);
        }
        const float middleY = (kOpeningY + bottomY) / 2;
        const Cut atMiddle = CutAt(magMesh, kGun.grab[1]);
        const Cut atHook = CutAt(magMesh, kGun.hook[1]);
        float backOff = 0.0f;
        for (int i = 0; i < 3; ++i)
            backOff = std::max(backOff, std::fabs(CutAt(magMesh, kGun.back[i][0]).zLo - kGun.back[i][1]));
        // The paddle: the frame's vertices hanging behind the well, under the receiver.
        float padZLo = 1e9f, padZHi = -1e9f;
        int pad = 0;
        for (size_t v = 0; v + 2 < frameMesh.pos.size(); v += 3) {
            const float* p = &frameMesh.pos[v];
            if (frameMesh.part[v / 3] != rows::kFrameBone || std::fabs(p[0] - kGun.paddle[0]) > 0.01f || std::fabs(p[1] - kGun.paddle[1]) > 0.006f || p[2] < 0.36f || p[2] > 0.40f)
                continue;
            padZLo = std::min(padZLo, p[2]), padZHi = std::max(padZHi, p[2]);
            ++pad;
        }
        const float backAtPaddle = CutAt(magMesh, kGun.paddle[1]).zLo;
        std::snprintf(what, sizeof(what), "s4 on the magazine at its seat: its top y %.4f (the row's %.4f), z %.4f to %.4f there (the row's %.4f); the middle below the receiver y %.4f (the grab's "
                      "%.4f), its back and front there %.4f and %.4f (the grab's z %.4f); its front edge at the receiver's underside z %.4f (the hook's %.4f); its back edge at the rows' three "
                      "heights within %.2f mm; the paddle behind it (%d vertices, z %.4f to %.4f; the row's %.4f, %.1f mm behind the magazine's back there)",
                      topY, kGun.top[1], topZLo, topZHi, kGun.top[2], middleY, kGun.grab[1], atMiddle.zLo, atMiddle.zHi, kGun.grab[2], atHook.zHi, kGun.hook[2], backOff * 1000.0f, pad,
                      padZLo, padZHi, kGun.paddle[2], (backAtPaddle - kGun.paddle[2]) * 1000.0f);
        Check(std::fabs(topY - kGun.top[1]) < 0.001f && kGun.top[2] > topZLo && kGun.top[2] < topZHi && std::fabs((topZLo + topZHi) / 2 - kGun.top[2]) < 0.003f &&
                  std::fabs(middleY - kGun.grab[1]) < 0.002f && std::fabs((atMiddle.zLo + atMiddle.zHi) / 2 - kGun.grab[2]) < 0.002f && std::fabs(atHook.zHi - kGun.hook[2]) < 0.0015f &&
                  std::fabs(kGun.hook[1] - kOpeningY) < 0.0005f && backOff < 0.0015f && pad >= 4 && kGun.paddle[2] >= padZLo - 0.001f && kGun.paddle[2] <= padZHi + 0.001f &&
                  backAtPaddle - kGun.paddle[2] > 0.01f && backAtPaddle - kGun.paddle[2] < 0.03f,
              what);
        // The handle's knob: the bolt's vertices furthest out to the right, on the frame with the bolt at rest.
        float inverse[12], boltOn[12];
        InvertRigid(body.Bone(rows::kFrameBone), inverse);
        Compose(inverse, body.Bone(rows::kBoltBone), boltOn);
        float outermost = 1e9f;
        for (const Vertex& v : body.vertices)
            if (v.bone == rows::kBoltBone) {
                float p[3];
                Apply(boltOn, v.at, p);
                outermost = std::min(outermost, p[0]);
            }
        float kLo[3] = {1e9f, 1e9f, 1e9f}, kHi[3] = {-1e9f, -1e9f, -1e9f};
        for (const Vertex& v : body.vertices)
            if (v.bone == rows::kBoltBone) {
                float p[3];
                Apply(boltOn, v.at, p);
                if (p[0] > outermost + 0.004f)
                    continue;
                for (int k = 0; k < 3; ++k)
                    kLo[k] = std::min(kLo[k], p[k]), kHi[k] = std::max(kHi[k], p[k]);
            }
        const float knob[3] = {(kLo[0] + kHi[0]) / 2, (kLo[1] + kHi[1]) / 2, (kLo[2] + kHi[2]) / 2};
        std::snprintf(what, sizeof(what), "s4 its handle's knob on the bolt at rest, its outer 4 mm: x %.4f to %.4f, y %.4f to %.4f, z %.4f to %.4f; the lever row (%.3f %.3f %.3f) %.1f mm from its middle",
                      kLo[0], kHi[0], kLo[1], kHi[1], kLo[2], kHi[2], kGun.lever[0], kGun.lever[1], kGun.lever[2], Dist(knob, kGun.lever) * 1000.0f);
        Check(Dist(knob, kGun.lever) < 0.003f && kGun.lever[0] < -0.04f, what);
    }

    // ---- s5: the sights ----
    if (meshes) {
        // The rear notch's floor and the front post's top: the frame's highest within 6 mm of the line's x (the rear sight's ears stand 7.7 mm and 6.6 mm
        // to its sides), at the rear sight's block and at the post.
        float rearTop = -1e9f, postTop = -1e9f;
        for (size_t v = 0; v + 2 < frameMesh.pos.size(); v += 3) {
            const float* p = &frameMesh.pos[v];
            if (frameMesh.part[v / 3] != rows::kFrameBone || std::fabs(p[0] - rows::kSights[0]) > 0.006f)
                continue;
            if (p[2] > 0.490f && p[2] < 0.512f)
                rearTop = std::max(rearTop, p[1]);
            if (p[2] > 0.695f && p[2] < 0.705f)
                postTop = std::max(postTop, p[1]);
        }
        // The muzzle: the frame's frontmost face.
        float front = -1e9f;
        for (size_t v = 0; v + 2 < frameMesh.pos.size(); v += 3)
            front = std::max(front, frameMesh.pos[v + 2]);
        float mLo[2] = {1e9f, 1e9f}, mHi[2] = {-1e9f, -1e9f};
        for (size_t v = 0; v + 2 < frameMesh.pos.size(); v += 3)
            if (frameMesh.pos[v + 2] > front - 0.004f)
                for (int k = 0; k < 2; ++k)
                    mLo[k] = std::min(mLo[k], frameMesh.pos[v + k]), mHi[k] = std::max(mHi[k], frameMesh.pos[v + k]);
        // The first of the frame within 1 cm of the line, going forward from the eye.
        float rear = 1e9f;
        for (size_t v = 0; v + 2 < frameMesh.pos.size(); v += 3) {
            const float* p = &frameMesh.pos[v];
            if (frameMesh.part[v / 3] == rows::kFrameBone && p[2] > rows::kEyeZ + 0.02f && std::hypot(p[0] - rows::kSights[0], p[1] - rows::kSights[1]) < 0.01f)
                rear = std::min(rear, p[2]);
        }
        // The game's zoomed eye (its draw scan: "the view in its bone 2").
        const std::string zoom = ReadText(survey + "\\SAIGA12K_zoom.txt");
        float eye[3] = {}, forward[3] = {};
        const size_t at = zoom.find("the view in its bone 2: eye (");
        const bool haveEye = at != std::string::npos &&
                             std::sscanf(zoom.c_str() + at, "the view in its bone 2: eye (%f %f %f), forward (%f %f %f)", &eye[0], &eye[1], &eye[2], &forward[0], &forward[1], &forward[2]) == 6;
        const float eyeOff = haveEye ? std::hypot(eye[0] - rows::kSights[0], eye[1] - rows::kSights[1]) : 1.0f;
        std::snprintf(what, sizeof(what), "s5 its sights: the rear notch's floor y %.4f, the front post's top y %.4f (the line's %.4f, %s); the muzzle's face z %.4f, its middle (%.4f %.4f) (the row's "
                      "(%.4f %.4f %.4f)); the first of the gun within 1 cm of the line ahead of the eye z %.3f (the eye row's %.3f); the game's zoomed eye %s (%.3f %.3f %.3f), %.1f mm off the line, "
                      "looking %.2f degrees up",
                      rearTop, postTop, rows::kSights[1], rows::kUp == 0.0f ? "level" : "NOT LEVEL", front, (mLo[0] + mHi[0]) / 2, (mLo[1] + mHi[1]) / 2, rows::kMuzzle[0], rows::kMuzzle[1],
                      rows::kMuzzle[2], rear, rows::kRearZ, haveEye ? "at" : "NOT READ at", eye[0], eye[1], eye[2], eyeOff * 1000.0f,
                      std::atan2(forward[1], forward[2]) * 180.0f / kPi);
        Check(std::fabs(rearTop - rows::kSights[1]) < 0.0005f && std::fabs(postTop - rows::kSights[1]) < 0.0005f && rows::kUp == 0.0f && std::fabs(front - rows::kMuzzle[2]) < 0.0005f &&
                  std::fabs((mLo[0] + mHi[0]) / 2 - rows::kMuzzle[0]) < 0.001f && std::fabs((mLo[1] + mHi[1]) / 2 - rows::kMuzzle[1]) < 0.001f && std::fabs(rear - rows::kRearZ) < 0.002f &&
                  rows::kSights[2] == rows::kMuzzle[2] && (!haveEye || (eyeOff < 0.0015f && std::fabs(eye[2] - rows::kEyeZ) < 0.001f)),
              what);
        if (!haveEye)
            Skip("s5 the game's zoomed eye: build\\survey\\SAIGA12K_zoom.txt isn't there");
    }

    // ---- s6, s7, s9: the recordings ----
    const std::vector<Frame> partial = Recording(survey + "\\SAIGA12K_partial.bin"), dry = Recording(survey + "\\SAIGA12K_dry.bin");
    if (partial.empty() || dry.empty()) {
        Skip("s6, s7, s9: no recordings (build\\survey\\SAIGA12K_partial.bin, SAIGA12K_dry.bin)");
    } else {
        // The palms at rest: the partial's 1.5 to 3.0 s (after its two rounds), the dry's 9.5 to 12 s (after its reload).
        float palms[2][3] = {}, worstPalm = 0.0f;
        int rest = 0;
        for (const std::vector<Frame>* rec : {&partial, &dry})
            for (const Frame& fr : *rec) {
                const bool still = rec == &partial ? fr.ms >= 1500 && fr.ms <= 3000 : fr.ms >= 9500 && fr.ms <= 12000;
                if (!still || !fr.haveBody || !fr.haveGloves)
                    continue;
                float right[3], leftPalm[3];
                OnFrame(fr, kRightPalmPoint, false, right);
                OnFrame(fr, kLeftPalmPoint, true, leftPalm);
                for (int k = 0; k < 3; ++k)
                    palms[0][k] += right[k], palms[1][k] += leftPalm[k];
                worstPalm = std::max({worstPalm, Dist(right, rows::kRightPalm), Dist(leftPalm, rows::kLeftPalm)});
                ++rest;
            }
        for (auto& p : palms)
            for (float& c : p)
                c /= std::max(rest, 1);
        std::snprintf(what, sizeof(what), "s6 the palms at rest (%d frames of both recordings): right (%.3f %.3f %.3f), left (%.3f %.3f %.3f); each frame within %.1f mm of the rows'", rest, palms[0][0],
                      palms[0][1], palms[0][2], palms[1][0], palms[1][1], palms[1][2], worstPalm * 1000.0f);
        Check(rest > 100 && worstPalm < 0.003f, what);
        // The game's own hand still on the seated magazine before it pulls it: the partial's 3.35 to 3.46 s, the dry's 6.08 to 6.16 s.
        float gameHand[3] = {}, spread = 0.0f;
        int on = 0;
        float first[3] = {};
        bool haveFirst = false;
        for (const std::vector<Frame>* rec : {&partial, &dry})
            for (const Frame& fr : *rec) {
                const bool held = rec == &partial ? fr.ms >= 3350 && fr.ms <= 3460 : fr.ms >= 6080 && fr.ms <= 6160;
                if (!held || !fr.haveBody || !fr.haveGloves)
                    continue;
                float m[3];
                OnFrame(fr, kHandInGlove, true, m);
                if (!haveFirst)
                    std::memcpy(first, m, sizeof(first)), haveFirst = true;
                spread = std::max(spread, Dist(m, first));
                for (int k = 0; k < 3; ++k)
                    gameHand[k] += m[k];
                ++on;
            }
        for (float& c : gameHand)
            c /= std::max(on, 1);
        float fitted[3];
        MiddleOf(magposes::kSaiga12kHold, fitted);
        std::snprintf(what, sizeof(what), "s6 the game's own hand still on the seated magazine before it pulls it (%d frames of both reloads, within %.1f mm of one another): its middle (%.3f %.3f %.3f), "
                      "%.1f cm from the grab, low on the magazine (%.1f cm above its bottom); the fitted hold's middle (%.3f %.3f %.3f), raised to the middle below the receiver",
                      on, spread * 1000.0f, gameHand[0], gameHand[1], gameHand[2], Dist(gameHand, kGun.grab) * 100.0f, (gameHand[1] - (kGun.back[2][0] - 0.006f)) * 100.0f, fitted[0],
                      fitted[1], fitted[2]);
        Check(on >= 10 && spread < 0.005f && Dist(gameHand, kGun.grab) < 0.075f && std::fabs(fitted[1] - kGun.grab[1]) < 0.005f && std::fabs(fitted[2] - gameHand[2]) < 0.01f, what);
        // The bolt: its travel firing (the partial's two rounds) and in the game's own pull (the dry's empty reload); the counts.
        float firing = 0.0f, pulled = 0.0f;
        for (const Frame& fr : partial)
            if (fr.haveBody && fr.ms < 1500)
                firing = std::max(firing, 0.071f - BoltZ(fr));
        for (const Frame& fr : dry)
            if (fr.haveBody && fr.ms > 8000 && fr.ms < 8800)
                pulled = std::max(pulled, 0.071f - BoltZ(fr));
        int startCount = -1, emptyRefill = -1, partialRefill = -1;
        for (const Frame& fr : dry)
            if (fr.haveBody) {
                if (startCount < 0)
                    startCount = fr.loaded;
                if (fr.ms > 8000 && fr.ms < 9000 && fr.loaded > 0)
                    emptyRefill = std::max(emptyRefill, fr.loaded);
            }
        for (const Frame& fr : partial)
            if (fr.haveBody && fr.ms > 4600 && fr.ms < 6000)
                partialRefill = std::max(partialRefill, fr.loaded);
        std::snprintf(what, sizeof(what), "s6 the bolt %.1f cm back with each round, %.1f cm in the game's own pull (the lever's travel %.1f cm); the gun %d loaded at the start, %d after an empty "
                      "reload (one less: the chamber's), %d after a part-used one",
                      firing * 100.0f, pulled * 100.0f, kGun.leverTravel * 100.0f, startCount, emptyRefill, partialRefill);
        Check(std::fabs(firing - 0.091f) < 0.002f && std::fabs(pulled - kGun.leverTravel) < 0.002f && startCount == 7 && emptyRefill == 6 && partialRefill == 7, what);

        // ---- s7: the kick, each round fired dry ----
        struct Round {
            int ms;
            float rise, riseAt, back, backAt;
        };
        std::vector<Round> rounds;
        std::vector<const Frame*> bodies;
        for (const Frame& fr : dry)
            if (fr.haveBody)
                bodies.push_back(&fr);
        for (size_t i = 1; i < bodies.size(); ++i) {
            if (bodies[i]->loaded != bodies[i - 1]->loaded - 1 || bodies[i]->loaded < 0)
                continue;
            Round r{bodies[i]->ms, 0.0f, 0.0f, 0.0f, 0.0f};
            float inverse[12];
            InvertRigid(bodies[i - 1]->frame, inverse);
            for (size_t j = i - 1; j < bodies.size() && bodies[j]->ms - bodies[i]->ms <= 600; ++j) {
                float d[12];
                Compose(inverse, bodies[j]->frame, d);
                const float back = -d[11] * 100.0f, rise = std::atan2(d[6], d[10]) * 180.0f / kPi, t = static_cast<float>(bodies[j]->ms - bodies[i - 1]->ms);
                if (back > r.back)
                    r.back = back, r.backAt = t;
                if (rise > r.rise)
                    r.rise = rise, r.riseAt = t;
            }
            rounds.push_back(r);
        }
        float riseLo = 1e9f, riseHi = -1e9f, backLo = 1e9f, backHi = -1e9f;
        for (const Round& r : rounds)
            riseLo = std::min(riseLo, r.rise), riseHi = std::max(riseHi, r.rise), backLo = std::min(backLo, r.back), backHi = std::max(backHi, r.back);
        // kick.tsv's row: recorded, back_cm back_ms rise_deg rise_ms.
        const std::string tsv = ReadText("tools\\recoil\\kick.tsv");
        const size_t line = tsv.find("Saiga 12K\tweapons/saiga20k/saiga_20k\tshotgun\t14970\tsaiga12k\trecorded\t");
        float tBack = 0.0f, tBackMs = 0.0f, tRise = 0.0f, tRiseMs = 0.0f;
        const bool haveRow = line != std::string::npos &&
                             std::sscanf(tsv.c_str() + line, "Saiga 12K\tweapons/saiga20k/saiga_20k\tshotgun\t14970\tsaiga12k\trecorded\t%f\t%f\t%f\t%f", &tBack, &tBackMs, &tRise, &tRiseMs) == 4;
        std::snprintf(what, sizeof(what), "s7 its kick: %d rounds fired dry, each turning it up %.2f to %.2f degrees and %.2f to %.2f cm back; the first %.2f degrees at %.0f ms, %.2f cm at %.0f ms; kick.tsv's "
                      "row %s (%.2f cm at %.0f ms, %.2f degrees at %.0f ms)",
                      static_cast<int>(rounds.size()), riseLo, riseHi, backLo, backHi, rounds.empty() ? 0.0f : rounds[0].rise, rounds.empty() ? 0.0f : rounds[0].riseAt,
                      rounds.empty() ? 0.0f : rounds[0].back, rounds.empty() ? 0.0f : rounds[0].backAt, haveRow ? "recorded" : "NOT FOUND", tBack, tBackMs, tRise, tRiseMs);
        Check(rounds.size() == 7 && riseLo > 6.3f && riseHi < 6.9f && backLo > 0.6f && backHi < 0.85f && haveRow && std::fabs(rounds[0].rise - tRise) < 0.05f &&
                  std::fabs(rounds[0].riseAt - tRiseMs) < 20.0f && std::fabs(rounds[0].back - tBack) < 0.05f && std::fabs(rounds[0].backAt - tBackMs) < 20.0f,
              what);

        // ---- s9: the sound cuts against the bones of its own empty reload ----
        // The reload's start: the left hand leaving its rest on the handguard (3 mm); the magazine pulled (the hand leaving its still hold on it); the bolt drawn
        // (half its pull) and home again.
        float handRest[3] = {};
        int restN = 0;
        for (const Frame& fr : dry)
            if (fr.haveBody && fr.haveGloves && fr.ms > 5000 && fr.ms < 5600) {
                float m[3];
                OnFrame(fr, kHandInGlove, true, m);
                for (int k = 0; k < 3; ++k)
                    handRest[k] += m[k];
                ++restN;
            }
        for (float& c : handRest)
            c /= std::max(restN, 1);
        float startMs = -1.0f, pullMs = -1.0f, drawMs = -1.0f, homeMs = -1.0f;
        float onMag[3] = {};
        bool haveOnMag = false;
        for (const Frame& fr : dry) {
            if (!fr.haveBody || !fr.haveGloves || fr.ms < 5600)
                continue;
            float m[3];
            OnFrame(fr, kHandInGlove, true, m);
            if (startMs < 0.0f && Dist(m, handRest) > 0.003f)
                startMs = static_cast<float>(fr.ms);
            if (fr.ms >= 6080 && fr.ms <= 6160 && !haveOnMag)
                std::memcpy(onMag, m, sizeof(onMag)), haveOnMag = true;
            if (haveOnMag && pullMs < 0.0f && fr.ms > 6160 && Dist(m, onMag) > 0.003f)
                pullMs = static_cast<float>(fr.ms);
            const float back = 0.071f - BoltZ(fr);
            if (fr.ms > 8000 && drawMs < 0.0f && back > kGun.leverTravel / 2)
                drawMs = static_cast<float>(fr.ms);
            if (drawMs > 0.0f && homeMs < 0.0f && back < 0.002f)
                homeMs = static_cast<float>(fr.ms);
        }
        const std::string source = ReadText("tools\\gamesounds.cpp");
        struct CutRow {
            const char* part;
            float from = 0.0f, to = 0.0f;
            bool found = false;
        } cuts[] = {{"mag_out"}, {"mag_in"}, {"lever"}, {"bolt"}};
        for (CutRow& c : cuts) {
            const std::string head = std::string("{\"") + c.part + "\", \"saiga12k\", kSaigaLong, ";
            const size_t at = source.find(head);
            c.found = at != std::string::npos && std::sscanf(source.c_str() + at + head.size(), "%f, %f", &c.from, &c.to) == 2;
        }
        const float pull = (pullMs - startMs) / 1000.0f, draw = (drawMs - startMs) / 1000.0f, home = (homeMs - startMs) / 1000.0f;
        const bool allFound = cuts[0].found && cuts[1].found && cuts[2].found && cuts[3].found;
        std::snprintf(what, sizeof(what), "s9 the sound cuts (%s): mag_out %.3f-%.3f, mag_in %.3f-%.3f, lever %.3f-%.3f, bolt %.3f-%.3f s of its empty reload's wave; the bones of its own empty reload "
                      "on the wave's clock (the hand off the handguard at %.3f s): the magazine pulled at %.3f, the bolt drawn half its pull at %.3f, home at %.3f",
                      source.empty() ? "tools\\gamesounds.cpp NOT READ" : allFound ? "four rows" : "A ROW MISSING", cuts[0].from, cuts[0].to, cuts[1].from, cuts[1].to, cuts[2].from,
                      cuts[2].to, cuts[3].from, cuts[3].to, startMs / 1000.0f, pull, draw, home);
        Check(allFound && startMs > 0.0f && pullMs > 0.0f && drawMs > 0.0f && homeMs > 0.0f && pull > cuts[0].from && pull < cuts[0].to && draw > cuts[2].from &&
                  draw < cuts[2].to && home > cuts[3].from && home < cuts[3].to && cuts[0].to < cuts[1].from && cuts[1].to < cuts[2].from && cuts[2].to < cuts[3].from,
              what);
    }

    // ---- s8: the fitted hands ----
    const Glove glove = LoadGloveParts(dir);
    if (!meshes || !glove.read) {
        Skip("s8 the hands against the meshes: no dumps of the gloves, the body or the magazine");
    } else {
        float lo[3] = {1e9f, 1e9f, 1e9f}, hi[3] = {-1e9f, -1e9f, -1e9f};
        for (size_t v = 0; v + 2 < magMesh.pos.size(); v += 3)
            for (int k = 0; k < 3; ++k)
                lo[k] = std::min(lo[k], magMesh.pos[v + k]), hi[k] = std::max(hi[k], magMesh.pos[v + k]);
        for (int k = 0; k < 3; ++k)
            lo[k] -= 0.06f, hi[k] += 0.06f;
        const Field onMag = FieldOf(magMesh, lo, hi, 0.001f);
        const Gaps g = GapsOf(glove, magposes::kSaiga12kHold, magposes::kSaiga12kFingers, onMag);
        float middle[3];
        MiddleOf(magposes::kSaiga12kHold, middle);
        std::snprintf(what, sizeof(what), "s8 the hand on the magazine at its seat: palm %.1f mm; thumb %.1f %.1f %.1f; index %.1f %.1f %.1f; middle %.1f %.1f %.1f; ring %.1f %.1f %.1f; little %.1f %.1f %.1f "
                      "(- in it); each finger's nearest part %.1f mm off at most, the deepest %.1f mm in; its middle (%.3f %.3f %.3f), %.1f cm from the grab",
                      g.palm, g.part[0][0], g.part[0][1], g.part[0][2], g.part[1][0], g.part[1][1], g.part[1][2], g.part[2][0], g.part[2][1], g.part[2][2], g.part[3][0], g.part[3][1],
                      g.part[3][2], g.part[4][0], g.part[4][1], g.part[4][2], g.farthestFinger, -g.deepest, middle[0], middle[1], middle[2], Dist(middle, kGun.grab) * 100.0f);
        Check(g.palm <= 3.0f && g.palm >= -2.5f && g.farthestFinger <= 3.5f && g.deepest >= -2.5f && Dist(middle, kGun.grab) < 0.07f, what);
        // The handle: the body round the knob, the bolt at rest with it, 1 mm cells.
        meshdump::Mesh handleMesh = frameMesh;
        handleMesh.tris.clear();
        for (size_t t = 0; t + 2 < body.tris.size(); t += 3)
            for (int e = 0; e < 3; ++e)
                handleMesh.tris.push_back(body.tris[t + e]);
        const float near0[3] = {kGun.lever[0] - 0.12f, kGun.lever[1] - 0.12f, kGun.lever[2] - 0.14f}, near1[3] = {kGun.lever[0] + 0.12f, kGun.lever[1] + 0.12f, kGun.lever[2] + 0.14f};
        const Field onGun = FieldOf(handleMesh, near0, near1, 0.001f);
        // The bolt and its handle alone (what magfit closed the fingers on), as it rests.
        meshdump::Mesh boltMesh = frameMesh;
        boltMesh.tris.clear();
        for (size_t t = 0; t + 2 < body.tris.size(); t += 3)
            if (body.vertices[static_cast<size_t>(body.tris[t])].bone == rows::kBoltBone && body.vertices[static_cast<size_t>(body.tris[t + 1])].bone == rows::kBoltBone &&
                body.vertices[static_cast<size_t>(body.tris[t + 2])].bone == rows::kBoltBone)
                for (int e = 0; e < 3; ++e)
                    boltMesh.tris.push_back(body.tris[t + e]);
        const Field onBolt = FieldOf(boltMesh, near0, near1, 0.001f);
        const Gaps over = GapsOf(glove, magposes::kSaiga12kLeverHold, magposes::kSaiga12kLeverFingers, onBolt);
        const Gaps overGun = GapsOf(glove, magposes::kSaiga12kLeverHold, magposes::kSaiga12kLeverFingers, onGun);
        const Gaps under = GapsOf(glove, magposes::kSaiga12kLeverUnderHold, magposes::kSaiga12kLeverUnderFingers, onGun);
        std::snprintf(what, sizeof(what), "s8 the hand on the handle: over the top as the game's own (on the bolt: its ring %.2f and little %.2f mm off at their nearest, the deepest %.2f mm in; on "
                      "the whole gun, its palm %.2f mm off, the deepest %.2f mm in: the game's own hand laid on the receiver as it reaches over); from under it, its palm %.2f mm off, its "
                      "thumb %.2f and index %.2f at their nearest, the deepest %.2f mm in",
                      over.nearest[3], over.nearest[4], -over.deepest, overGun.palm, -overGun.deepest, under.palm, under.nearest[0], under.nearest[1], -under.deepest);
        // (Deepest: a glove's 2.5 mm, and half of the field's 1 mm cell, which magfit's own field, on another grid, read 1 mm less: its "-1.5" for the little
        // finger reads -2.5 here.)
        Check(over.nearest[3] <= 3.0f && over.nearest[4] <= 3.0f && over.deepest >= -3.0f && overGun.deepest >= -3.0f && under.palm <= 11.0f && under.nearest[1] <= 1.0f &&
                  under.deepest >= -3.0f,
              what);
    }

    std::printf("%d checks, %d failed, %d skipped\n", g_checks, g_failed, g_skipped);
    return g_failed;
}

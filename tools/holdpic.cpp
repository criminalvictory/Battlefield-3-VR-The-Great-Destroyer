// The game's own hands on each gun at rest, mirrored by the twin tables and drawn on the gun, to look at before any film of free hands on every gun:
// the left glove on a gun's pistol grip is the game's own right hand there mirrored (handpose::RightToLeft), the right glove on its front grip the game's
// own left hold mirrored (LeftToRight), never fitted again. Both come from the first frame of the gun's survey recording (build\survey\<gun>_partial.bin,
// weapon.txt "allrec": the game's bones as it drew them, the gun at rest in its hands), on the gun's frame bone, as skin.cpp's rig keeps its rest.
//   - Drawn on the gun's own mesh (its mesh dump, posed by that frame's bones) from four sides, beside the game's own hold seen in a mirror: where the gun
//     is the same on both sides the two pictures match; where they differ, the gun isn't (a lever, a selector, a sight on one side). The gun's parts
//     drawn apart that lie on it in that frame (a sight, the rounds, the M240's belt, the M416's grenade launcher and sight) are drawn with it.
//   - Each fingertip's and the palm's nearest skin to the gun's collision shape (build\guns\<shape>.bin, tools/gunshape.exe): the game's own hand, its
//     mirror, and the mirror as hands.cpp bends it while it's held (BendWithController's numbers: the grip squeezed, the thumb on the stick, the
//     trigger off and in).
//   - The palms against the profile rows (src/gunprofile.h's Make, from skin.cpp's kHeldKinds palms): the game's right palm at anchor[1], its left at
//     support[0], and the mirrored gloves' palms at anchor[0] and support[1], where the hands' places on the gun are.
//   holdpic <build folder> <out folder> [shape ...]     (an94 rpk aek971 ak74 g3a3 kh2002 m16a4 m4a1 m240 fim92 aks74u ump45 mk11 m249 saiga12k m416; none: every one)
// Writes hold_<shape>.bmp for each gun and holdpic.txt (what's printed) into the out folder. Each picture, top to bottom, four tiles a row: from its
// left, from its right, from below, and from below in front on the hand's outside (the first row's last: from behind and above, as the shooter sees it):
//   1 the whole gun held left-handed: the left glove on its pistol grip, the right glove on its front grip (both mirrored)
//   2 the pistol grip: the left glove                       3 the same grip with the game's own right hand, seen in a mirror
//   4 the front grip: the right glove                       5 the same grip with the game's own left hand, seen in a mirror
// The fingers: thumb red, index orange, middle yellow, ring green, little blue, the palm brown; the gun's frame light grey, its other bones darker, its
// parts drawn apart dark green (none of them in its shape). The marks, each finger's tip and the palm at their nearest skin to the gun: magenta in it
// past a glove's give (2.5 mm), cyan on it, white within 10 mm; a square where that point is seen, a ring where it's hidden (inside the fist, behind the
// grip); the profile's palm point a white cross. The pictures show the tables as they are (the bends only change the numbers). The M416, built and
// approved in the headset, is drawn the same way for comparison, from the recording of its grenade launcher's own reload (its left hand cups the
// launcher, which isn't in its shape).
// The exit code: how many guns couldn't be pictured (a file missing or unreadable); 1 for a usage error.
// Built with the project's CMake (holdpic), or without it while the game holds the DLL, from the project's folder in an x86 prompt:
//   cl /nologo /EHsc /O2 /std:c++17 /DNOMINMAX /DWIN32_LEAN_AND_MEAN /I src /I tools /Febuild\holdpic.exe /Fobuild\ tools\holdpic.cpp src\handpose.cpp

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "../src/gunprofile.h"
#include "../src/handpose.h"
#include "meshdump.h"

using namespace meshdump;

namespace {

using handpose::kBones;
using handpose::kRightBones;
using handpose::kRightFirst;

constexpr int kGloveBones = kRightFirst + kRightBones;          // the gloves' draw's bones, both hands (0-44)
constexpr uint32_t kGlovesCount = 13755, kGlovesStart = 15051;  // the gloves' colour draw (skin.cpp kArmDraws[0])
constexpr int kRightRoot = 37, kLeftPalmBone = 1;
// Where a palm on a gun is measured: skin.cpp's kRightPalm on the right's root, kLeftPalm on the left's palm bone (the kHeldKinds palms are these).
constexpr float kRightPalm[3] = {-0.505f, 1.129f, 0.150f};
constexpr float kLeftPalm[3] = {0.537f, 1.098f, 0.207f};
constexpr float kGive = 0.0025f;  // a glove's give: this far into the gun still looks on it
constexpr float kNear = 0.010f;   // within this of it, near it
constexpr float kOdd = 0.003f;    // a mirrored hand's part this much further in or off than the game's own: the gun differs side to side there
constexpr float kFar = 1.0f;      // no part on the shape's grid (5 cm of air round the gun and more)
// hands.cpp's BendWithController at rest in its targets, degrees (thumb ... little): the grip squeezed all the way (+6 on the three), the thumb
// on the stick (+5); on the pistol grip the index with the trigger off (0) or all the way in (22); on the front grip the index lifted (-12).
constexpr float kPistolHeld[5] = {5.0f, 0.0f, 6.0f, 6.0f, 6.0f};
constexpr float kPistolFiring[5] = {5.0f, 22.0f, 6.0f, 6.0f, 6.0f};
constexpr float kFrontHeld[5] = {5.0f, -12.0f, 6.0f, 6.0f, 6.0f};

// Each gun: its collision shape's name (src/gunprofile.h's word for it), its survey recording, its colour draw and the start of its mesh dump's files,
// its frame bone, and its profile row's palms (skin.cpp's kHeldKinds: the game's own palms at rest, measured from these recordings' first frames).
struct Gun {
    const char* shape;
    const char* recording;  // under build\survey
    uint32_t count, start;  // its colour draw
    const char* files;      // its mesh dump's files' names start so, in the build folder
    int frameBone;
    float rightPalm[3], leftPalm[3];
    bool reference;  // the M416: no row of palms (its anchor is the controller's grip, learned in the game); drawn for comparison
};
constexpr Gun kGuns[] = {
    {"an94", "AN94_partial.bin", 16188, 0, "mesh_16188_0_", 0, {-0.023f, -0.059f, 0.192f}, {0.002f, -0.018f, 0.561f}, false},
    {"rpk", "RPK_partial.bin", 24492, 0, "mesh_24492_0_", 3, {-0.032f, -0.080f, 0.192f}, {-0.001f, -0.029f, 0.585f}, false},
    {"aek971", "AEK971_partial.bin", 21585, 0, "mesh_21585_0_", 4, {-0.024f, -0.079f, 0.208f}, {0.004f, -0.031f, 0.551f}, false},
    {"ak74", "AK74M_partial.bin", 21723, 0, "mesh_21723_0_", 2, {-0.032f, -0.080f, 0.193f}, {0.001f, -0.031f, 0.548f}, false},
    {"g3a3", "G3A3_partial.bin", 23064, 0, "mesh_23064_0_", 0, {-0.034f, -0.082f, 0.241f}, {0.020f, -0.007f, 0.616f}, false},
    {"kh2002", "KH2002_partial.bin", 15237, 0, "mesh_15237_0_", 3, {-0.025f, -0.099f, 0.234f}, {0.016f, -0.059f, 0.447f}, false},
    {"m16a4", "M16A4_partial.bin", 17013, 312, "mesh_17013_312_", 4, {-0.054f, -0.123f, 0.256f}, {0.014f, -0.041f, 0.636f}, false},
    {"m4a1", "M4A1_partial.bin", 22194, 312, "mesh_22194_312_", 2, {-0.054f, -0.114f, 0.255f}, {0.012f, -0.041f, 0.612f}, false},
    {"m240", "M240_partial.bin", 27588, 2364, "mesh_27588_2364_", 3, {-0.023f, -0.115f, 0.196f}, {0.018f, -0.065f, 0.678f}, false},
    {"fim92", "FIM92A_partial.bin", 20298, 354, "mesh_20298_354_", 0, {-0.046f, -0.132f, 0.593f}, {0.025f, -0.146f, 0.993f}, false},
    // Semper Fidelis's two (2026-11-18). The train's arms are bare (18108/16053), so their recordings were written again with the gloves' draw in them
    // (the bare hands' bones under the gloves' numbers, skin.cpp's kBareGlove: toolsaregloves.ps1).
    {"aks74u", "AKS74u_partial_gloves.bin", 17991, 0, "mesh_17991_0_", 2, {-0.031f, -0.080f, 0.193f}, {0.003f, -0.035f, 0.517f}, false},
    {"ump45", "UMP45_partial_gloves.bin", 19314, 234, "mesh_19314_234_", 2, {-0.053f, -0.066f, 0.223f}, {0.004f, -0.026f, 0.538f}, false},
    // Operation Swordbreaker's Mk11 (2026-11-18; a level with gloves).
    {"mk11", "MK11_partial.bin", 12456, 2214, "mesh_12456_2214_", 0, {-0.054f, -0.114f, 0.254f}, {0.012f, -0.041f, 0.612f}, false},
    // Operation Swordbreaker's M249 (2026-11-18; a level with gloves): its palms from its empty recording's rest (its partial one fires first).
    {"m249", "M249_empty.bin", 24291, 2808, "mesh_24291_2808_", 3, {-0.021f, -0.107f, 0.218f}, {0.028f, -0.084f, 0.583f}, false},
    // Operation Swordbreaker's Saiga 12K (2026-11-18; a level with gloves): its body alone (its magazine is a mesh of its own, never drawn in its recordings).
    {"saiga12k", "SAIGA12K_partial.bin", 14970, 0, "mesh_14970_0_", 2, {-0.032f, -0.080f, 0.193f}, {-0.006f, -0.034f, 0.589f}, false},
    {"m416", "M320_vanilla.bin", 12768, 0, "gun_", 2, {}, {}, true},
};

std::string g_report;

// Printed, and kept for holdpic.txt.
void Say(const char* format, ...) {
    char text[2048];
    va_list args;
    va_start(args, format);
    std::vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    std::fputs(text, stdout);
    g_report += text;
}

// ---- Matrices: three rows, rotation | translation ----
void Compose(const float* a, const float* b, float* out) {
    float r[12];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j)
            r[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        r[i * 4 + 3] += a[i * 4 + 3];
    }
    std::memcpy(out, r, sizeof(r));
}

bool Invert(const float* m, float* out) {
    const float a = m[0], b = m[1], c = m[2], d = m[4], e = m[5], f = m[6], g = m[8], h = m[9], k = m[10];
    const float det = a * (e * k - f * h) - b * (d * k - f * g) + c * (d * h - e * g);
    if (!(std::fabs(det) > 1e-8f))
        return false;
    const float r[9] = {(e * k - f * h) / det, (c * h - b * k) / det, (b * f - c * e) / det, (f * g - d * k) / det, (a * k - c * g) / det,
                        (c * d - a * f) / det, (d * h - e * g) / det, (b * g - a * h) / det, (a * e - b * d) / det};
    for (int i = 0; i < 3; ++i) {
        out[i * 4] = r[i * 3], out[i * 4 + 1] = r[i * 3 + 1], out[i * 4 + 2] = r[i * 3 + 2];
        out[i * 4 + 3] = -(r[i * 3] * m[3] + r[i * 3 + 1] * m[7] + r[i * 3 + 2] * m[11]);
    }
    return true;
}

bool Finite(const float* v, size_t n) {
    for (size_t i = 0; i < n; ++i)
        if (!std::isfinite(v[i]))
            return false;
    return true;
}

float Distance(const float* a, const float* b) {
    const float d[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    return std::sqrt(Dot(d, d));
}

// ---- The gun's collision shape (tools/gunshape.cpp's "GSDF" version 2, read as src/gunshape.cpp reads it) ----
struct Shape {
    int n[3] = {};
    float origin[3] = {}, cell = 0.0f;
    std::vector<int16_t> tenths;  // signed tenths of a millimetre, x fastest

    bool Load(const std::string& path) {
        FILE* f = nullptr;
        if (fopen_s(&f, path.c_str(), "rb") || !f)
            return false;
        char magic[4] = {};
        int32_t version = 0;
        bool ok = fread(magic, 1, 4, f) == 4 && !std::memcmp(magic, "GSDF", 4) && fread(&version, 4, 1, f) == 1 && version == 2 &&
                  fread(n, 4, 3, f) == 3 && fread(origin, 4, 3, f) == 3 && fread(&cell, 4, 1, f) == 1 && cell > 0.0005f && Finite(origin, 3);
        for (int k = 0; k < 3; ++k)
            ok = ok && n[k] > 1 && n[k] < 2000;
        ok = ok && static_cast<uint64_t>(n[0]) * n[1] * n[2] <= (64u << 20);  // (a damaged file: not a field of gigabytes)
        if (ok) {
            tenths.resize(static_cast<size_t>(n[0]) * n[1] * n[2]);
            ok = fread(tenths.data(), 2, tenths.size(), f) == tenths.size();
        }
        fclose(f);
        return ok;
    }
    // Metres from p to the gun's surface (negative inside), trilinear; false off the grid.
    bool At(const float* p, float& d) const {
        float f[3];
        int i[3];
        for (int k = 0; k < 3; ++k) {
            const float c = (p[k] - origin[k]) / cell;
            if (!(c >= 0.0f && c < static_cast<float>(n[k] - 1)))
                return false;
            i[k] = static_cast<int>(c);
            f[k] = c - static_cast<float>(i[k]);
        }
        float v = 0.0f;
        for (int dz = 0; dz < 2; ++dz)
            for (int dy = 0; dy < 2; ++dy)
                for (int dx = 0; dx < 2; ++dx) {
                    const float w = (dx ? f[0] : 1 - f[0]) * (dy ? f[1] : 1 - f[1]) * (dz ? f[2] : 1 - f[2]);
                    v += w * tenths[(static_cast<size_t>(i[2] + dz) * n[1] + i[1] + dy) * n[0] + i[0] + dx];
                }
        d = v * 0.0001f;
        return true;
    }
};

// ---- The mesh dumps ----
// A dump's draw as the game had it bound (its _dump.txt): the base vertex, the index format (57: 16 bits, 42: 32), the vertex buffer's stride and the
// offset it was bound at. Without the file (the M416's gun_*.bin): 16 bits, 48 bytes from the start.
struct DumpInfo {
    int base = 0, format = 57;
    unsigned stride = 48, offset = 0;
};

void ReadDumpInfo(const std::string& path, DumpInfo& info) {
    std::ifstream text(path);
    std::string word;
    while (text >> word) {
        if (word == "draw") {
            unsigned count = 0, start = 0;
            int base = 0;
            if (text >> count >> start >> base)
                info.base = base;
        } else if (word == "ib") {
            std::string formatWord;
            int format = 0;
            if (text >> formatWord >> format && formatWord == "format")
                info.format = format;
        } else if (word == "vb") {
            int slot = -1;
            std::string strideWord, offsetWord;
            unsigned stride = 0, offset = 0;
            if (text >> slot >> strideWord >> stride >> offsetWord >> offset && slot == 0)
                info.stride = stride, info.offset = offset;
        }
    }
}

bool IndexAt(const std::vector<uint8_t>& ib, int format, size_t i, uint32_t& v) {
    const size_t size = format == 42 ? 4 : 2;
    if ((i + 1) * size > ib.size())
        return false;
    if (size == 4) {
        std::memcpy(&v, &ib[i * 4], 4);
    } else {
        uint16_t s;
        std::memcpy(&s, &ib[i * 2], 2);
        v = s;
    }
    return true;
}

// A vertex as these meshes have it: its place in half floats at 0, four bones at 8, four weights at 12 (here summing to 1). raw: its weights' bytes'
// sum, 250-260 in every draw these are read from (anything else: a layout this doesn't know).
struct Vertex {
    float p[3];
    int bone[4];
    float weight[4];
    int raw;
};

bool VertexAt(const std::vector<uint8_t>& vb, const DumpInfo& info, uint32_t v, Vertex& out) {
    const size_t at = info.offset + static_cast<size_t>(v) * info.stride;
    if (info.stride < 16 || at + 16 > vb.size())
        return false;
    const uint8_t* o = &vb[at];
    uint16_t h[3];
    std::memcpy(h, o, 6);
    out.raw = 0;
    for (int k = 0; k < 3; ++k)
        out.p[k] = Half(h[k]);
    for (int s = 0; s < 4; ++s) {
        out.bone[s] = o[8 + s];
        out.weight[s] = static_cast<float>(o[12 + s]);
        out.raw += o[12 + s];
    }
    for (float& w : out.weight)
        w = out.raw > 0 ? w / static_cast<float>(out.raw) : 0.0f;
    return Finite(out.p, 3);
}

bool LaidOut(const Vertex& x) { return x.raw >= 250 && x.raw <= 260; }

// The gun as drawn in the recorded frame, on its frame bone (x left, y up, z to the muzzle): its colour draw, less a part parked far from it (the
// AN-94's and the AEK-971's spare magazines, thrown in by their own reloads: more than 30 cm outside the frame's own vertices), and the parts drawn
// apart that lie on it in that frame (a sight, the rounds, the M240's belt, the M416's grenade launcher: their own draws and palettes).
struct GunMesh {
    std::vector<float> pos;  // xyz a vertex
    std::vector<int> bone;   // its bone with the most weight; -1 a part drawn apart
    std::vector<int> tris;
    std::vector<int> parked;
    std::vector<std::string> parts;  // the parts drawn apart, count@start
    float lo[3] = {}, hi[3] = {};    // its colour draw's box (1st to 99th percentile: a stray vertex doesn't move the pictures)
    int refused = 0;                 // triangles of its colour draw that couldn't be read (an index past its buffer), left out
};

// One draw from its mesh dump (<files>vb0.bin, ib.bin, dump.txt), posed by its recorded palette onto the gun's frame (toFrame): its vertices added to
// the gun's (a part's bone -1), its triangles to tris (a triangle with an index past its buffer left out, counted in refused). Its vertices skinned
// (four bones and their weights summing to 255) or, a part's, rigid (a sight's: its place in four half floats, the last 1 or -1, and no weights; all
// of it on its bone 0, as skin.cpp places kHeldSights). False when its dump isn't there, it's a depth pass (a part's: stride under 32), or more than 1%
// of its vertices are laid out neither way; then nothing of it is added.
bool AddDraw(const std::string& dir, const std::string& files, uint32_t count, uint32_t start, const std::vector<float>& palette, const float* toFrame,
             bool part, GunMesh& gun, std::vector<int>& tris, int* refused, std::string& why) {
    const std::string name = dir + "\\" + files;
    const auto vb = ReadAll(name + "vb0.bin"), ib = ReadAll(name + "ib.bin");
    DumpInfo info;
    ReadDumpInfo(name + "dump.txt", info);
    if (vb.empty() || ib.empty()) {
        why = "its mesh dump (" + files + "vb0.bin, ib.bin) isn't in the build folder";
        return false;
    }
    if (info.format != 57 && info.format != 42) {
        why = "its index format " + std::to_string(info.format) + " isn't one this reads";
        return false;
    }
    if (part && info.stride < 32) {
        why = "a depth pass";
        return false;
    }
    const int bones = static_cast<int>(palette.size() / 12);
    const size_t vertices = vb.size() > info.offset && info.stride >= 16 ? (vb.size() - info.offset) / info.stride : 0;
    // Its triangles' corners, each within its buffer.
    std::vector<uint32_t> corners;
    int missed = 0;
    for (size_t i = start; i + 2 < static_cast<size_t>(start) + count; i += 3) {
        uint32_t c[3];
        bool keep = true;
        for (int k = 0; k < 3 && keep; ++k) {
            uint32_t v = 0;
            const int64_t at = IndexAt(ib, info.format, i + k, v) ? static_cast<int64_t>(v) + info.base : -1;
            keep = at >= 0 && static_cast<size_t>(at) < vertices;
            c[k] = static_cast<uint32_t>(at);
        }
        if (keep)
            corners.insert(corners.end(), c, c + 3);
        else
            ++missed;
    }
    // How its vertices are laid out.
    std::vector<char> seen(vertices, 0);
    int used = 0, skinned = 0, rigid = 0;
    for (uint32_t v : corners) {
        if (seen[v])
            continue;
        seen[v] = 1;
        ++used;
        Vertex x;
        if (!VertexAt(vb, info, v, x))
            continue;
        uint16_t w;
        std::memcpy(&w, &vb[info.offset + static_cast<size_t>(v) * info.stride + 6], 2);
        skinned += LaidOut(x) ? 1 : 0;
        rigid += w == 0x3C00 || w == 0xBC00 ? 1 : 0;  // a half float's 1 or -1 (its tangent's side, kept there)
    }
    const bool asSkinned = used && skinned * 100 >= used * 99, asRigid = !asSkinned && part && bones > 0 && used && rigid * 100 >= used * 99;
    if (!asSkinned && !asRigid) {
        why = used ? "its vertices aren't laid out as these meshes' are (" + std::to_string(used - skinned) + " of " + std::to_string(used) +
                         " weights don't sum to 255, " + std::to_string(used - rigid) + " not rigid)"
                   : "no triangle of it could be read";
        return false;
    }
    const size_t firstVertex = gun.bone.size(), firstTri = tris.size();
    std::vector<int> mine(vertices, -1);
    for (size_t t = 0; t + 2 < corners.size(); t += 3) {
        int c[3];
        bool keep = true;
        for (int k = 0; k < 3 && keep; ++k) {
            const uint32_t v = corners[t + k];
            if (mine[v] < 0) {
                Vertex x;
                float sum[3] = {}, weight = 0.0f, most = -1.0f;
                int heaviest = -1;
                if (!VertexAt(vb, info, v, x)) {
                    keep = false;
                    break;
                }
                if (asRigid) {
                    Apply(&palette[0], x.p, sum);
                    weight = 1.0f, heaviest = 0;
                } else {
                    for (int s = 0; s < 4; ++s) {
                        if (x.weight[s] <= 0.0f || x.bone[s] >= bones)
                            continue;
                        float q[3];
                        Apply(&palette[static_cast<size_t>(x.bone[s]) * 12], x.p, q);
                        for (int a = 0; a < 3; ++a)
                            sum[a] += x.weight[s] * q[a];
                        weight += x.weight[s];
                        if (x.weight[s] > most)
                            most = x.weight[s], heaviest = x.bone[s];
                    }
                    for (float& a : sum)
                        a = weight > 0.0f ? a / weight : 0.0f;
                }
                float on[3];
                Apply(toFrame, sum, on);
                if (!(weight > 0.0f) || !Finite(on, 3)) {
                    keep = false;
                    break;
                }
                mine[v] = static_cast<int>(gun.bone.size());
                gun.pos.insert(gun.pos.end(), on, on + 3);
                gun.bone.push_back(part ? -1 : heaviest);
            }
            c[k] = mine[v];
        }
        if (keep)
            tris.insert(tris.end(), c, c + 3);
        else
            ++missed;
    }
    if (refused)
        *refused += missed;
    if (tris.size() == firstTri) {
        why = "no triangle of it could be read";
        gun.pos.resize(firstVertex * 3), gun.bone.resize(firstVertex);
        return false;
    }
    return true;
}

// The recording's frame: the gun's palette, and every other draw of that frame with its own.
struct Recorded {
    struct Draw {
        uint32_t count = 0, start = 0;
        std::vector<float> palette;  // 12 floats a bone
    };
    int serial = -1, ms = 0, glovesMs = 0;
    std::vector<float> gun, gloves;
    std::vector<Draw> draws;
};

// The draws of the soldier's own arms and body (skin.cpp's kArmDraws): never a part of the gun.
constexpr uint32_t kArmDraws[][2] = {{13755, 15051}, {7617, 49995}, {2016, 13035}, {8529, 0}, {21189, 28806}, {4506, 8529}};

bool LoadGunMesh(const std::string& dir, const Gun& g, const Recorded& rec, GunMesh& gun, std::string& why) {
    const int bones = static_cast<int>(rec.gun.size() / 12);
    float toFrame[12];
    if (g.frameBone >= bones || !Invert(&rec.gun[static_cast<size_t>(g.frameBone) * 12], toFrame)) {
        why = "its frame bone isn't in the recorded frame";
        return false;
    }
    std::vector<int> corners;
    if (!AddDraw(dir, g.files, g.count, g.start, rec.gun, toFrame, false, gun, corners, &gun.refused, why))
        return false;
    // Parked parts: each bone's vertices' middle against the frame bone's own box.
    float core[2][3] = {{1e9f, 1e9f, 1e9f}, {-1e9f, -1e9f, -1e9f}};
    std::vector<double> middle(64 * 3, 0.0);
    std::vector<int> each(64, 0);
    for (size_t v = 0; v < gun.bone.size(); ++v) {
        const int b = std::clamp(gun.bone[v], 0, 63);
        ++each[b];
        for (int a = 0; a < 3; ++a) {
            middle[b * 3 + a] += gun.pos[v * 3 + a];
            if (b == g.frameBone)
                core[0][a] = std::min(core[0][a], gun.pos[v * 3 + a]), core[1][a] = std::max(core[1][a], gun.pos[v * 3 + a]);
        }
    }
    if (each[std::clamp(g.frameBone, 0, 63)] > 0)
        for (int b = 0; b < 64; ++b) {
            if (!each[b] || b == g.frameBone)
                continue;
            float out = 0.0f;
            for (int a = 0; a < 3; ++a) {
                const float m = static_cast<float>(middle[b * 3 + a] / each[b]);
                const float e = std::max(std::max(core[0][a] - m, 0.0f), m - core[1][a]);
                out += e * e;
            }
            if (std::sqrt(out) > 0.30f)
                gun.parked.push_back(b);
        }
    for (size_t t = 0; t + 2 < corners.size(); t += 3) {
        bool keep = true;
        for (int k = 0; k < 3; ++k)
            keep = keep && std::find(gun.parked.begin(), gun.parked.end(), gun.bone[corners[t + k]]) == gun.parked.end();
        if (keep)
            gun.tris.insert(gun.tris.end(), corners.begin() + t, corners.begin() + t + 3);
    }
    if (gun.tris.empty()) {
        why = "nothing of its colour draw is left to draw";
        return false;
    }
    std::vector<float> axis;
    for (int a = 0; a < 3; ++a) {
        axis.clear();
        for (int v : gun.tris)
            axis.push_back(gun.pos[static_cast<size_t>(v) * 3 + a]);
        std::sort(axis.begin(), axis.end());
        gun.lo[a] = axis[axis.size() / 100];
        gun.hi[a] = axis[axis.size() - 1 - axis.size() / 100];
    }
    // The parts drawn apart: any other draw of the frame with a mesh dump, not the arms' or the body's, not a depth pass, its middle within 5 cm of the
    // gun's box.
    for (const Recorded::Draw& d : rec.draws) {
        if ((d.count == g.count && d.start == g.start) ||
            std::any_of(std::begin(kArmDraws), std::end(kArmDraws), [&](const uint32_t* k) { return k[0] == d.count && k[1] == d.start; }))
            continue;
        const std::string files = "mesh_" + std::to_string(d.count) + "_" + std::to_string(d.start) + "_";
        std::error_code missing;
        if (!std::filesystem::exists(dir + "\\" + files + "dump.txt", missing))
            continue;
        const size_t firstVertex = gun.bone.size();
        std::vector<int> partTris;
        std::string partWhy;
        if (!AddDraw(dir, files, d.count, d.start, d.palette, toFrame, true, gun, partTris, nullptr, partWhy))
            continue;
        double sum[3] = {};
        for (size_t v = firstVertex; v < gun.bone.size(); ++v)
            for (int a = 0; a < 3; ++a)
                sum[a] += gun.pos[v * 3 + a];
        bool on = true;
        for (int a = 0; a < 3; ++a) {
            const float m = static_cast<float>(sum[a] / static_cast<double>(gun.bone.size() - firstVertex));
            on = on && m > gun.lo[a] - 0.05f && m < gun.hi[a] + 0.05f;
        }
        if (!on) {
            gun.pos.resize(firstVertex * 3), gun.bone.resize(firstVertex);
            continue;
        }
        gun.tris.insert(gun.tris.end(), partTris.begin(), partTris.end());
        gun.parts.push_back(std::to_string(d.count) + "@" + std::to_string(d.start));
    }
    return true;
}

// One glove of the gloves' colour draw: its vertices in the gloves' bind space with their weights, each one's bone with the most weight, its
// triangles. The left's bones 0-22, the right's 23-44; a triangle with weight on both hands (or on neither) is in neither.
struct Hand {
    bool right = false;
    std::vector<Vertex> vertices;
    std::vector<int> heaviest;
    std::vector<int> tris;
};

bool LoadGloves(const std::string& dir, Hand& left, Hand& right, std::string& why) {
    const std::string name = dir + "\\mesh_" + std::to_string(kGlovesCount) + "_" + std::to_string(kGlovesStart) + "_";
    const auto vb = ReadAll(name + "vb0.bin"), ib = ReadAll(name + "ib.bin");
    DumpInfo info;
    ReadDumpInfo(name + "dump.txt", info);
    if (vb.empty() || ib.empty()) {
        why = "the gloves' mesh dump (mesh_13755_15051_vb0.bin, ib.bin) isn't in the build folder";
        return false;
    }
    right.right = true;
    std::vector<int> mine[2];
    mine[0].assign(65536 * 4, -1), mine[1].assign(65536 * 4, -1);
    int odd = 0;
    for (size_t i = kGlovesStart; i + 2 < static_cast<size_t>(kGlovesStart) + kGlovesCount; i += 3) {
        Vertex x[3];
        uint32_t v[3];
        bool ok = true, onLeft = true, onRight = true;
        for (int k = 0; k < 3 && ok; ++k) {
            ok = IndexAt(ib, info.format, i + k, v[k]) && (v[k] += info.base) < mine[0].size() && VertexAt(vb, info, v[k], x[k]);
            odd += ok && !LaidOut(x[k]) ? 1 : 0;
            for (int s = 0; s < 4 && ok; ++s) {
                if (x[k].weight[s] <= 0.0f)
                    continue;
                onLeft = onLeft && x[k].bone[s] < kRightFirst;
                onRight = onRight && x[k].bone[s] >= kRightFirst && x[k].bone[s] < kGloveBones;
            }
        }
        if (!ok || onLeft == onRight)
            continue;
        Hand& h = onLeft ? left : right;
        std::vector<int>& map = mine[onLeft ? 0 : 1];
        for (int k = 0; k < 3; ++k) {
            if (map[v[k]] < 0) {
                map[v[k]] = static_cast<int>(h.vertices.size());
                h.vertices.push_back(x[k]);
                int best = 0;
                for (int s = 1; s < 4; ++s)
                    if (x[k].weight[s] > x[k].weight[best])
                        best = s;
                h.heaviest.push_back(x[k].bone[best]);
            }
            h.tris.push_back(map[v[k]]);
        }
    }
    if (left.tris.empty() || right.tris.empty()) {
        why = "the gloves' mesh dump has no whole glove in it";
        return false;
    }
    if (odd * 100 > static_cast<int>(left.tris.size() + right.tris.size())) {
        why = "the gloves' vertices aren't laid out as these meshes' are (their weights don't sum to 255)";
        return false;
    }
    return true;
}

// Which part of the hand a glove bone is: 0-4 a finger's last part (the thumb ... the little finger), 5 the palm, 6 the wrist, 7 the rest of a finger.
// The left's by its twin on the right glove (the two gloves' fingers agree bone for bone: handpose.cpp's TwinsAgree).
enum Part { kPalm = 5, kWrist = 6, kFingerRest = 7 };
const char* const kPartName[8] = {"thumb", "index", "middle", "ring", "little", "palm", "wrist", "finger"};

int FingerOf(int bone) {  // 0 the thumb ... 4 the little finger, -1 the palm and the wrist
    const int r = bone >= kRightFirst ? bone - kRightFirst : bone >= 0 && bone < kBones ? handpose::kRightOfLeft[bone] - kRightFirst : -1;
    return r >= 0 && r < kRightBones ? handpose::kRightFinger[r] : -1;
}

int PartOf(int bone) {
    const int r = bone >= kRightFirst ? bone - kRightFirst : bone >= 0 && bone < kBones ? handpose::kRightOfLeft[bone] - kRightFirst : -1;
    if (r < 0 || r >= kRightBones)
        return kWrist;
    const int finger = handpose::kRightFinger[r], depth = handpose::kRightDepth[r];
    const bool wrist = bone >= kRightFirst ? bone == 37 || bone == 44 : bone == 0 || bone == 4 || bone == 5;
    if (finger < 0)
        return wrist ? kWrist : kPalm;
    return depth == 0 ? kPalm : depth == 3 ? finger : kFingerRest;
}

// A hand's vertices placed by its table (the left's bones 0-22 by table[b], the right's 23-44 by table[b - 23]: each from the gloves' bind space onto the
// gun's frame), as the glove is drawn.
std::vector<float> Place(const Hand& h, const float (*table)[12]) {
    const int first = h.right ? kRightFirst : 0, last = h.right ? kGloveBones : kBones;
    std::vector<float> placed(h.vertices.size() * 3, 0.0f);
    for (size_t i = 0; i < h.vertices.size(); ++i) {
        const Vertex& x = h.vertices[i];
        for (int s = 0; s < 4; ++s) {
            if (x.weight[s] <= 0.0f || x.bone[s] < first || x.bone[s] >= last)
                continue;
            float q[3];
            Apply(table[x.bone[s] - first], x.p, q);
            for (int k = 0; k < 3; ++k)
                placed[i * 3 + k] += x.weight[s] * q[k];
        }
    }
    return placed;
}

// Each fingertip's (its last part's) and the palm's nearest skin to the gun's shape, and the hand's deepest point anywhere (metres, negative in it;
// kFar with no part on the shape's grid).
struct Contact {
    float part[6] = {kFar, kFar, kFar, kFar, kFar, kFar};
    int at[6] = {-1, -1, -1, -1, -1, -1};
    float deepest = kFar;
    int deepestAt = -1, deepestPart = kPalm;
};

Contact Measure(const Hand& h, const std::vector<float>& placed, const Shape& shape) {
    Contact c;
    for (size_t i = 0; i < h.vertices.size(); ++i) {
        float d;
        if (!shape.At(&placed[i * 3], d))
            continue;
        const int part = PartOf(h.heaviest[i]);
        if (part <= kPalm && d < c.part[part])
            c.part[part] = d, c.at[part] = static_cast<int>(i);
        if (d < c.deepest)
            c.deepest = d, c.deepestAt = static_cast<int>(i), c.deepestPart = part;
    }
    return c;
}

// ---- The recording (weapon.txt "allrec"): records of 8 ints (the draw's count, the frame, the eye, its start, the rounds, a spare, the ms, the
// bones), each followed by its bones (3x4 floats) ----
// The gun's first colour draw, every other draw of that frame (the first of each), and the gloves' in the same frame (else the latest within 100 ms
// before it).
bool FirstFrame(const std::string& path, uint32_t count, uint32_t start, Recorded& out, std::string& why) {
    FILE* f = nullptr;
    if (fopen_s(&f, path.c_str(), "rb") || !f) {
        why = "its recording isn't there";
        return false;
    }
    int32_t head[8];
    std::vector<float> bones, lastGloves;
    std::vector<Recorded::Draw> frame;  // the draws of the frame being read
    int frameSerial = -1, lastGlovesSerial = -1, lastGlovesMs = 0;
    while (fread(head, sizeof(head), 1, f) == 1) {
        const int n = head[7];
        if (n <= 0 || n > 64)
            break;
        bones.resize(static_cast<size_t>(n) * 12);
        if (fread(bones.data(), 48, n, f) != static_cast<size_t>(n))
            break;
        const uint32_t kind = static_cast<uint32_t>(head[0]), pass = static_cast<uint32_t>(head[3]);
        const int serial = head[1], ms = head[6];
        if (serial != frameSerial) {
            if (out.serial >= 0)
                break;  // the gun's frame is over
            frameSerial = serial;
            frame.clear();
        }
        if (kind == kGlovesCount && pass == kGlovesStart && n >= kGloveBones) {
            lastGloves = bones, lastGlovesSerial = serial, lastGlovesMs = ms;
            if (out.serial >= 0 && out.gloves.empty())
                out.gloves = bones, out.glovesMs = ms;
        } else if (kind == count && pass == start && out.serial < 0) {
            out.gun = bones, out.serial = serial, out.ms = ms;
            if (lastGlovesSerial == serial)
                out.gloves = lastGloves, out.glovesMs = lastGlovesMs;
        } else if (kind >= 12 && std::none_of(frame.begin(), frame.end(), [&](const Recorded::Draw& d) { return d.count == kind && d.start == pass; })) {
            frame.push_back({kind, pass, bones});
        }
    }
    fclose(f);
    if (out.serial < 0) {
        why = "its recording has no draw of it";
        return false;
    }
    out.draws = std::move(frame);
    if (out.gloves.empty() && !lastGloves.empty() && out.ms - lastGlovesMs <= 100)
        out.gloves = lastGloves, out.glovesMs = lastGlovesMs;
    if (out.gloves.empty()) {
        why = "its recording has no gloves near its first frame";
        return false;
    }
    if (!Finite(out.gun.data(), out.gun.size()) || !Finite(out.gloves.data(), out.gloves.size())) {
        why = "its recorded bones aren't finite";
        return false;
    }
    for (Recorded::Draw& d : out.draws)
        if (!Finite(d.palette.data(), d.palette.size()))
            d.palette.clear();  // (drawn with no bones: nothing of it added)
    return true;
}

// ---- Pictures ----
constexpr float kFingerColour[6][3] = {{0.6f, 0.5f, 0.4f}, {0.9f, 0.25f, 0.2f}, {0.95f, 0.55f, 0.15f}, {0.9f, 0.85f, 0.2f}, {0.3f, 0.8f, 0.3f}, {0.3f, 0.5f, 0.95f}};
constexpr float kFrameColour[3] = {0.62f, 0.62f, 0.66f}, kPartColour[3] = {0.42f, 0.44f, 0.5f}, kApartColour[3] = {0.36f, 0.42f, 0.36f};
constexpr float kInColour[3] = {1.0f, 0.1f, 0.9f}, kOnColour[3] = {0.1f, 1.0f, 1.0f}, kNearColour[3] = {1.0f, 1.0f, 1.0f};
constexpr float kRim[3] = {0.05f, 0.05f, 0.05f};

// The same view of the scene's mirror image across the gun's x = 0 plane, its picture not flipped: the game's own hold drawn so, it reads as the
// mirrored hold does (where the gun is the same on both sides, the two pictures are the same).
View Mirrored(const View& v) {
    View m = v;
    m.forward[0] = -m.forward[0], m.up[0] = -m.up[0], m.right[0] = -m.right[0];
    return m;
}

void DrawGun(Image& img, int tx, int ty, const View& view, const float* centre, float scale, const GunMesh& gun, int frameBone) {
    for (size_t t = 0; t + 2 < gun.tris.size(); t += 3) {
        const int a = gun.tris[t], b = gun.tris[t + 1], c = gun.tris[t + 2];
        DrawTriangle(img, tx, ty, view, centre, scale, &gun.pos[static_cast<size_t>(a) * 3], &gun.pos[static_cast<size_t>(b) * 3],
                     &gun.pos[static_cast<size_t>(c) * 3], gun.bone[a] == frameBone ? kFrameColour : gun.bone[a] < 0 ? kApartColour : kPartColour);
    }
}

void DrawHand(Image& img, int tx, int ty, const View& view, const float* centre, float scale, const Hand& h, const std::vector<float>& placed) {
    for (size_t t = 0; t + 2 < h.tris.size(); t += 3) {
        const int a = h.tris[t], b = h.tris[t + 1], c = h.tris[t + 2];
        DrawTriangle(img, tx, ty, view, centre, scale, &placed[static_cast<size_t>(a) * 3], &placed[static_cast<size_t>(b) * 3],
                     &placed[static_cast<size_t>(c) * 3], kFingerColour[FingerOf(h.heaviest[a]) + 1]);
    }
}

// A mark at p, placed as DrawTriangle places points, over everything: a square where p is seen (nothing in front of it by more than 6 mm), a ring where
// it's hidden (inside the fist, behind the grip), or a cross; each in a dark rim.
enum class Look { kSeen, kHidden, kCross };

void Mark(Image& img, int tx, int ty, const View& view, const float* centre, float scale, const float* p, const float* colour, Look look) {
    const float d[3] = {p[0] - centre[0], p[1] - centre[1], p[2] - centre[2]};
    const float sx = tx + kTile / 2 + Dot(d, view.right) * scale, sy = ty + kTile / 2 - Dot(d, view.up) * scale, sz = Dot(d, view.forward);
    if (!(sx >= tx && sx < tx + kTile && sy >= ty && sy < ty + kTile))
        return;
    const int cx = static_cast<int>(sx), cy = static_cast<int>(sy);
    if (look != Look::kCross)
        look = img.depth[static_cast<size_t>(cy) * img.w + cx] < sz - 0.006f ? Look::kHidden : Look::kSeen;
    for (int y = cy - 6; y <= cy + 6; ++y)
        for (int x = cx - 6; x <= cx + 6; ++x) {
            if (x < tx || x >= tx + kTile || y < ty || y >= ty + kTile)
                continue;
            const int ax = std::abs(x - cx), ay = std::abs(y - cy), edge = std::max(ax, ay), arm = std::min(ax, ay);
            const float* c = look == Look::kSeen     ? (edge <= 3 ? colour : edge <= 5 ? kRim : nullptr)
                             : look == Look::kHidden ? (edge == 4 || edge == 5 ? colour : edge == 3 || edge == 6 ? kRim : nullptr)
                                                     : (arm <= 1 ? colour : arm == 2 ? kRim : nullptr);
            if (!c)
                continue;
            for (int k = 0; k < 3; ++k)
                img.rgb[(static_cast<size_t>(y) * img.w + x) * 3 + k] = c[k];
        }
}

void MarkContacts(Image& img, int tx, int ty, const View& view, const float* centre, float scale, const Contact& c, const std::vector<float>& placed) {
    for (int part = 0; part <= kPalm; ++part) {
        if (c.at[part] < 0 || c.part[part] > kNear)
            continue;
        const float* colour = c.part[part] < -kGive ? kInColour : c.part[part] <= kOdd ? kOnColour : kNearColour;
        Mark(img, tx, ty, view, centre, scale, &placed[static_cast<size_t>(c.at[part]) * 3], colour, Look::kSeen);
    }
}

// ---- The report ----
// A distance in millimetres, or "far" (nothing on the shape's grid).
std::string Mm(float metres) {
    char text[32];
    if (metres >= kFar)
        return "far";
    std::snprintf(text, sizeof(text), "%.1f mm", 1000.0f * metres);
    return text;
}

void Row(const char* grip, const char* who, const Contact& c) {
    char text[512];
    int at = std::snprintf(text, sizeof(text), "  %-12s %-40s", grip, who);
    for (int part = 0; part <= kPalm; ++part)
        at += c.part[part] >= kFar ? std::snprintf(text + at, sizeof(text) - at, "    far") : std::snprintf(text + at, sizeof(text) - at, " %6.1f", 1000.0f * c.part[part]);
    if (c.deepest >= kFar)
        std::snprintf(text + at, sizeof(text) - at, "     far\n");
    else
        std::snprintf(text + at, sizeof(text) - at, "  %6.1f %s\n", 1000.0f * c.deepest, kPartName[c.deepestPart]);
    Say("%s", text);
}

// The mirrored hand against the game's own, part by part: where it lies further in or further off by more than kOdd (the gun isn't the same on both
// sides there), and anything of it in the gun past a glove's give that the game's own isn't. The count of such findings.
int Compare(const char* glove, const char* game, const Contact& mirrored, const Contact& own) {
    int found = 0;
    for (int part = 0; part <= kPalm; ++part) {
        const float a = mirrored.part[part], b = own.part[part];
        if (a >= kFar || b >= kFar || (a > kNear && b > kNear) || std::fabs(a - b) <= kOdd)
            continue;
        Say("    %s: its %s %.1f mm %s than %s's (%.1f against %.1f mm)\n", glove, kPartName[part], 1000.0f * std::fabs(a - b), a < b ? "further in" : "further off",
            game, 1000.0f * a, 1000.0f * b);
        ++found;
    }
    if (mirrored.deepest < -kGive && !(own.deepest < mirrored.deepest + kOdd)) {
        Say("    %s: its %s %.1f mm in the gun, past a glove's give (%s's deepest %s)\n", glove, kPartName[mirrored.deepestPart], -1000.0f * mirrored.deepest, game,
            Mm(own.deepest).c_str());
        ++found;
    }
    return found;
}

struct Summary {
    const char* shape;
    float pistolDeepest, pistolOwnDeepest, frontDeepest, frontOwnDeepest;
    int findings;
};
std::vector<Summary> g_summary;

bool Picture(const std::string& dir, const std::string& outDir, const Gun& g, const Hand& left, const Hand& right) {
    const char* name = g.reference ? "M416" : gunprofile::NameOf(g.shape);
    std::string why;
    Recorded rec;
    if (!FirstFrame(dir + "\\survey\\" + g.recording, g.count, g.start, rec, why)) {
        Say("%s (%s): not pictured: %s (survey\\%s)\n\n", name, g.shape, why.c_str(), g.recording);
        return false;
    }
    GunMesh gun;
    if (!LoadGunMesh(dir, g, rec, gun, why)) {
        Say("%s (%s): not pictured: %s\n\n", name, g.shape, why.c_str());
        return false;
    }
    Shape shape;
    if (!shape.Load(dir + "\\guns\\" + g.shape + ".bin")) {
        Say("%s (%s): not pictured: its shape guns\\%s.bin isn't there or can't be read (tools/gunshape.exe makes it)\n\n", name, g.shape, g.shape);
        return false;
    }
    // The gloves on the gun's frame, as skin.cpp's rig keeps its rest: each bone of the gloves' draw composed with the frame bone's inverse.
    float toFrame[12], rel[kGloveBones][12];
    if (!Invert(&rec.gun[static_cast<size_t>(g.frameBone) * 12], toFrame)) {
        Say("%s (%s): not pictured: its frame bone can't be inverted\n\n", name, g.shape);
        return false;
    }
    for (int b = 0; b < kGloveBones; ++b)
        Compose(toFrame, &rec.gloves[static_cast<size_t>(b) * 12], rel[b]);
    if (!Finite(&rel[0][0], sizeof(rel) / sizeof(float))) {
        Say("%s (%s): not pictured: the gloves on its frame aren't finite\n\n", name, g.shape);
        return false;
    }
    // The four tables: the game's own right and left, and each mirrored onto the other glove by the twin tables (the left's bone 5 takes the cuff's 44).
    float gameRight[kRightBones][12], gameLeft[kBones][12], leftOnPistol[kBones][12], rightOnFront[kRightBones][12];
    std::memcpy(gameRight, rel[kRightFirst], sizeof(gameRight));
    std::memcpy(gameLeft, rel[0], sizeof(gameLeft));
    handpose::RightToLeft(gameRight, leftOnPistol);
    handpose::LeftToRight(gameLeft, rightOnFront);
    // As hands.cpp bends them while they're held.
    float leftHeld[kBones][12], leftFiring[kBones][12], frontBent[kBones][12], rightHeld[kRightBones][12];
    std::memcpy(leftHeld, leftOnPistol, sizeof(leftHeld));
    std::memcpy(leftFiring, leftOnPistol, sizeof(leftFiring));
    std::memcpy(frontBent, gameLeft, sizeof(frontBent));
    handpose::BendFurther(leftHeld, kPistolHeld);
    handpose::BendFurther(leftFiring, kPistolFiring);
    handpose::BendFurther(frontBent, kFrontHeld);
    handpose::LeftToRight(frontBent, rightHeld);

    // The palms: the game's, measured as the kHeldKinds rows were; the mirrored gloves' at the twins of those bones, on the mirrored points.
    const float mirroredRightPalm[3] = {-kRightPalm[0], kRightPalm[1], kRightPalm[2]}, mirroredLeftPalm[3] = {-kLeftPalm[0], kLeftPalm[1], kLeftPalm[2]};
    float rightPalm[3], leftPalm[3], leftGlovePalm[3], rightGlovePalm[3];
    Apply(rel[kRightRoot], kRightPalm, rightPalm);
    Apply(rel[kLeftPalmBone], kLeftPalm, leftPalm);
    Apply(leftOnPistol[handpose::kLeftOfRight[kRightRoot - kRightFirst]], mirroredRightPalm, leftGlovePalm);
    Apply(rightOnFront[handpose::kRightOfLeft[kLeftPalmBone] - kRightFirst], mirroredLeftPalm, rightGlovePalm);

    // The profile as hands.cpp makes it for this gun (a stand-in body: Make needs one, and the gun's box).
    static const loose::Spec kNoBody{};
    gunprofile::In in;
    in.held = true, in.heldId = g.count, in.heldShape = g.shape, in.body = &kNoBody;
    std::memcpy(in.launcherGrip, g.rightPalm, sizeof(in.launcherGrip));
    std::memcpy(in.launcherFront, g.leftPalm, sizeof(in.launcherFront));
    std::memcpy(in.bodyMin, gun.lo, sizeof(in.bodyMin));
    std::memcpy(in.bodyMax, gun.hi, sizeof(in.bodyMax));
    const gunprofile::Profile p = g.reference ? gunprofile::Profile{} : gunprofile::Make(in);
    const char* front = g.reference ? "handguard" : p.pose == gunprofile::Pose::kNone ? "front grip" : p.front;

    Say("%s (%s): colour draw %u@%u, frame bone %d; survey\\%s, frame %d at %d ms (the gloves at %d ms)\n", name, g.shape, g.count, g.start, g.frameBone,
        g.recording, rec.serial, rec.ms, rec.glovesMs);
    if (!gun.parked.empty()) {
        Say("  left out of the pictures, parked far from the gun: bone");
        for (int b : gun.parked)
            Say(" %d", b);
        Say("\n");
    }
    if (gun.refused)
        Say("  %d of its %u triangles couldn't be read (an index past its buffer): left out\n", gun.refused, g.count / 3);
    if (!gun.parts.empty()) {
        Say("  drawn with it, its parts drawn apart (darker green; none of them in its shape):");
        for (const std::string& part : gun.parts)
            Say(" %s", part.c_str());
        Say("\n");
    }
    // The shape against the gun's own mesh: its frame's vertices lie on its surface (else the shape isn't on this frame, and no distance below holds).
    {
        std::vector<float> off;
        for (size_t v = 0; v < gun.bone.size(); ++v) {
            float d;
            if (gun.bone[v] == g.frameBone && shape.At(&gun.pos[v * 3], d))
                off.push_back(std::fabs(d));
        }
        std::sort(off.begin(), off.end());
        if (off.empty())
            Say("  its shape: none of its frame's vertices on the shape's grid: the shape isn't on this frame\n");
        else
            Say("  its shape against its frame's own vertices: half within %.1f mm, 90%% within %.1f mm (%zu vertices)\n", 1000.0f * off[off.size() / 2],
                1000.0f * off[off.size() * 9 / 10], off.size());
    }
    if (g.reference) {
        Say("  the game's palms on it: right (%.3f %.3f %.3f), left (%.3f %.3f %.3f) (the M416 has no row of palms: its anchor is the controller's grip)\n",
            rightPalm[0], rightPalm[1], rightPalm[2], leftPalm[0], leftPalm[1], leftPalm[2]);
    } else if (p.pose == gunprofile::Pose::kNone) {
        Say("  the profile is dormant for it (gunprofile::Make gave kNone): free hands would leave it in the right hand\n");
    } else {
        Say("  the game's palms on it: right (%.3f %.3f %.3f), %.1f mm from the profile's anchor[1]; left (%.3f %.3f %.3f), %.1f mm from its support[0]\n",
            rightPalm[0], rightPalm[1], rightPalm[2], 1000.0f * Distance(rightPalm, p.anchor[1]), leftPalm[0], leftPalm[1], leftPalm[2],
            1000.0f * Distance(leftPalm, p.support[0]));
        Say("  the left glove's palm on its pistol grip (%.3f %.3f %.3f), %.1f mm from anchor[0]; the right glove's on its %s (%.3f %.3f %.3f), %.1f mm from "
            "support[1]\n",
            leftGlovePalm[0], leftGlovePalm[1], leftGlovePalm[2], 1000.0f * Distance(leftGlovePalm, p.anchor[0]), front, rightGlovePalm[0], rightGlovePalm[1],
            rightGlovePalm[2], 1000.0f * Distance(rightGlovePalm, p.support[1]));
    }
    {
        const float mirroredRight[3] = {-rightPalm[0], rightPalm[1], rightPalm[2]}, mirroredLeft[3] = {-leftPalm[0], leftPalm[1], leftPalm[2]};
        const float twinOff = std::max(Distance(leftGlovePalm, mirroredRight), Distance(rightGlovePalm, mirroredLeft));
        if (twinOff > 1e-4f)
            Say("  the twin tables don't mirror the palms: %.2f mm off the game's palms mirrored\n", 1000.0f * twinOff);
    }

    // The hands placed on the gun, and how near each comes to its shape.
    const auto ownRight = Place(right, gameRight), ownLeft = Place(left, gameLeft);
    const auto leftPlaced = Place(left, leftOnPistol), rightPlaced = Place(right, rightOnFront);
    const Contact cOwnRight = Measure(right, ownRight, shape), cLeft = Measure(left, leftPlaced, shape);
    const Contact cLeftHeld = Measure(left, Place(left, leftHeld), shape), cLeftFiring = Measure(left, Place(left, leftFiring), shape);
    const Contact cOwnLeft = Measure(left, ownLeft, shape), cRight = Measure(right, rightPlaced, shape);
    const Contact cRightHeld = Measure(right, Place(right, rightHeld), shape);
    Say("  the nearest skin to its shape, mm (- in it):                    thumb  index middle   ring little   palm  deepest\n");
    Row("pistol grip", "the game's right hand", cOwnRight);
    Row("", "the left glove (its mirror)", cLeft);
    Row("", "  held (grip squeezed, thumb on the stick)", cLeftHeld);
    Row("", "  firing (the trigger in)", cLeftFiring);
    Row(front, "the game's left hand", cOwnLeft);
    Row("", "the right glove (its mirror)", cRight);
    Row("", "  held (grip squeezed, thumb on the stick)", cRightHeld);
    if (cOwnRight.deepest > kNear)
        Say("    the game's right hand is nowhere near its shape: it holds a part the shape hasn't (drawn apart), so its pistol grip's numbers say nothing\n");
    if (cOwnLeft.deepest > kNear)
        Say("    the game's left hand is nowhere near its shape: it holds a part the shape hasn't (drawn apart: the M416's grenade launcher), so its %s's "
            "numbers say nothing\n",
            front);
    int findings = Compare("the left glove", "the game's right hand", cLeft, cOwnRight);
    findings += Compare("the right glove", "the game's left hand", cRight, cOwnLeft);
    if (!findings)
        Say("    the mirrored hands lie on the gun as the game's own do (every part within %.0f mm of the game's, nothing past a glove's give)\n", 1000.0f * kOdd);
    g_summary.push_back({g.shape, cLeft.deepest, cOwnRight.deepest, cRight.deepest, cOwnLeft.deepest, findings});

    // The picture: five rows of four tiles.
    constexpr int kColumns = 4, kRows = 5;
    Image img(kTile * kColumns, kTile * kRows);
    const View whole[kColumns] = {MakeView(-1, 0, 0, 0, 1, 0), MakeView(1, 0, 0, 0, 1, 0), MakeView(0, 1, 0, 0, 0, 1), MakeView(-0.25f, -0.45f, 0.86f, 0, 1, 0)};
    const View pistol[kColumns] = {MakeView(-1, 0, 0, 0, 1, 0), MakeView(1, 0, 0, 0, 1, 0), MakeView(0, 1, 0, 0, 0, 1), MakeView(-0.5f, 0.55f, -0.65f, 0, 1, 0)};
    const View frontViews[kColumns] = {MakeView(-1, 0, 0, 0, 1, 0), MakeView(1, 0, 0, 0, 1, 0), MakeView(0, 1, 0, 0, 0, 1), MakeView(0.5f, 0.55f, -0.65f, 0, 1, 0)};
    const float wholeCentre[3] = {0.0f, (gun.lo[1] + gun.hi[1]) * 0.5f, (gun.lo[2] + gun.hi[2]) * 0.5f};
    const float wholeScale = kTile / std::clamp(1.1f * std::max(gun.hi[2] - gun.lo[2], gun.hi[1] - gun.lo[1]), 0.4f, 2.5f);
    const float pistolCentre[3] = {0.0f, rightPalm[1], rightPalm[2]}, frontCentre[3] = {0.0f, leftPalm[1], leftPalm[2]};
    const float closeScale = kTile / 0.24f;
    // The profile's palm points (the M416's: the game's own palms and their mirrors).
    const float* anchor0 = g.reference || p.pose == gunprofile::Pose::kNone ? leftGlovePalm : p.anchor[0];
    const float* anchor1 = g.reference || p.pose == gunprofile::Pose::kNone ? rightPalm : p.anchor[1];
    const float* support1 = g.reference || p.pose == gunprofile::Pose::kNone ? rightGlovePalm : p.support[1];
    const float* support0 = g.reference || p.pose == gunprofile::Pose::kNone ? leftPalm : p.support[0];
    for (int c = 0; c < kColumns; ++c) {
        const int x = c * kTile;
        DrawGun(img, x, 0, whole[c], wholeCentre, wholeScale, gun, g.frameBone);
        DrawHand(img, x, 0, whole[c], wholeCentre, wholeScale, left, leftPlaced);
        DrawHand(img, x, 0, whole[c], wholeCentre, wholeScale, right, rightPlaced);

        DrawGun(img, x, kTile, pistol[c], pistolCentre, closeScale, gun, g.frameBone);
        DrawHand(img, x, kTile, pistol[c], pistolCentre, closeScale, left, leftPlaced);
        MarkContacts(img, x, kTile, pistol[c], pistolCentre, closeScale, cLeft, leftPlaced);
        Mark(img, x, kTile, pistol[c], pistolCentre, closeScale, anchor0, kNearColour, Look::kCross);

        const View pistolMirror = Mirrored(pistol[c]);
        DrawGun(img, x, 2 * kTile, pistolMirror, pistolCentre, closeScale, gun, g.frameBone);
        DrawHand(img, x, 2 * kTile, pistolMirror, pistolCentre, closeScale, right, ownRight);
        MarkContacts(img, x, 2 * kTile, pistolMirror, pistolCentre, closeScale, cOwnRight, ownRight);
        Mark(img, x, 2 * kTile, pistolMirror, pistolCentre, closeScale, anchor1, kNearColour, Look::kCross);

        DrawGun(img, x, 3 * kTile, frontViews[c], frontCentre, closeScale, gun, g.frameBone);
        DrawHand(img, x, 3 * kTile, frontViews[c], frontCentre, closeScale, right, rightPlaced);
        MarkContacts(img, x, 3 * kTile, frontViews[c], frontCentre, closeScale, cRight, rightPlaced);
        Mark(img, x, 3 * kTile, frontViews[c], frontCentre, closeScale, support1, kNearColour, Look::kCross);

        const View frontMirror = Mirrored(frontViews[c]);
        DrawGun(img, x, 4 * kTile, frontMirror, frontCentre, closeScale, gun, g.frameBone);
        DrawHand(img, x, 4 * kTile, frontMirror, frontCentre, closeScale, left, ownLeft);
        MarkContacts(img, x, 4 * kTile, frontMirror, frontCentre, closeScale, cOwnLeft, ownLeft);
        Mark(img, x, 4 * kTile, frontMirror, frontCentre, closeScale, support0, kNearColour, Look::kCross);
    }
    // The tiles' edges.
    for (int y = 0; y < img.h; ++y)
        for (int x = 0; x < img.w; ++x)
            if ((x % kTile == 0 && x) || (y % kTile == 0 && y))
                for (int k = 0; k < 3; ++k)
                    img.rgb[(static_cast<size_t>(y) * img.w + x) * 3 + k] = 0.35f;
    const std::string file = outDir + "\\hold_" + g.shape + ".bmp";
    img.Save(file.c_str());
    Say("  %s\n\n", file.c_str());
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: holdpic <build folder> <out folder> [shape ...]   (an94 rpk aek971 ak74 g3a3 kh2002 m16a4 m4a1 m240 fim92 aks74u ump45 mk11 m249 saiga12k m416)\n");
        return 1;
    }
    const std::string dir = argv[1], outDir = argv[2];
    std::vector<std::string> only(argv + 3, argv + argc);
    for (const std::string& s : only)
        if (std::none_of(std::begin(kGuns), std::end(kGuns), [&](const Gun& g) { return s == g.shape; })) {
            std::fprintf(stderr, "no gun %s here (an94 rpk aek971 ak74 g3a3 kh2002 m16a4 m4a1 m240 fim92 aks74u ump45 mk11 m249 m416)\n", s.c_str());
            return 1;
        }
    std::error_code made;
    std::filesystem::create_directories(outDir, made);
    Hand left, right;
    std::string why;
    if (!LoadGloves(dir, left, right, why)) {
        std::fprintf(stderr, "%s\n", why.c_str());
        return 1;
    }
    Say("Each gun's mirrored holds (tools/holdpic.cpp): the left glove on its pistol grip is the game's own right hand there mirrored, the right glove on "
        "its front grip the game's own left hold mirrored, from the first frame of its survey recording.\n");
    Say("hold_<gun>.bmp, top to bottom, each from its left, its right, below, and below in front on the hand's outside (the first row's last: as the "
        "shooter sees it):\n");
    Say("  1 the whole gun held left-handed (both gloves mirrored)\n");
    Say("  2 the pistol grip, the left glove       3 the game's own right hand there, seen in a mirror (where the gun is the same on both sides, 2 and 3 "
        "match)\n");
    Say("  4 the front grip, the right glove       5 the game's own left hand there, seen in a mirror\n");
    Say("Fingers: thumb red, index orange, middle yellow, ring green, little blue, palm brown. The gun's frame light grey, its other bones darker, its parts "
        "drawn apart dark green.\n");
    Say("Marks at each fingertip's and the palm's nearest skin to the gun's collision shape: magenta in it past %.1f mm, cyan on it, white within %.0f mm; "
        "a square where seen, a ring where hidden. The profile's palm point: a white cross.\n\n",
        1000.0f * kGive, 1000.0f * kNear);
    Say("The gloves: the left %zu vertices and %zu triangles, the right %zu and %zu (mesh_13755_15051)\n\n", left.vertices.size(), left.tris.size() / 3,
        right.vertices.size(), right.tris.size() / 3);
    int missing = 0;
    for (const Gun& g : kGuns)
        if (only.empty() || std::find(only.begin(), only.end(), g.shape) != only.end())
            missing += Picture(dir, outDir, g, left, right) ? 0 : 1;
    if (!g_summary.empty()) {
        Say("Deepest in the gun, mm (- in it): the mirrored glove (the game's own hand there)\n");
        Say("  %-8s %-24s %-24s %s\n", "gun", "pistol grip", "front grip", "findings");
        const auto cell = [](float metres) {
            char text[16];
            if (metres >= kFar)
                return std::string("   far");
            std::snprintf(text, sizeof(text), "%6.1f", 1000.0f * metres);
            return std::string(text);
        };
        const auto both = [&](float mirrored, float own) { return cell(mirrored) + " (" + cell(own) + ")"; };
        for (const Summary& s : g_summary)
            Say("  %-8s %-24s %-24s %d\n", s.shape, both(s.pistolDeepest, s.pistolOwnDeepest).c_str(), both(s.frontDeepest, s.frontOwnDeepest).c_str(), s.findings);
    }
    if (missing)
        Say("%d gun%s not pictured\n", missing, missing == 1 ? "" : "s");
    if (FILE* f = nullptr; !fopen_s(&f, (outDir + "\\holdpic.txt").c_str(), "wb") && f) {
        fwrite(g_report.data(), 1, g_report.size(), f);
        fclose(f);
    }
    return missing;
}

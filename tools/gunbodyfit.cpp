// Every gun's body when it's let go of (free hands; loose.h), fitted from the game's own first-person meshes into src/gunbodies.h.
// Usage: gunbodyfit <build folder> <out gunbodies.h> [--check] [--partial] [--pictures <folder>]
//   - Reads only the mesh dumps already beside the DLL (weapon.txt "dump N S": mesh_<count>_<start>_{vb0,ib,bones}.bin and _dump.txt): each gun's
//     body draw posed by its own bones into its frame bone's space; the parts drawn apart (the M240's belt and its box's tape, the M16A4's rounds;
//     the other guns' rounds and the AK-74M's other magazine piece once they're dumped), each on its own bones at rest on the frame; the SMAW from
//     its depth pass (its three parts on one palette), with its case and without; the sights drawn apart (the enemies' PSO-1 and the rest): a few
//     extreme points each, for the gun they come on.
//   - Writes the header and <build folder>\bodytest_points.bin: up to 512 of each body's outermost kept vertices, with the hull box and the middle
//     of the mesh they came from (the header's), what tools/bodytest.cpp holds against the ground (a body's hull points only touch it by construction).
//   - A gun or a sight it can't fit, a part it can't add (one not dumped yet aside), or a frame or core bone not the table's: it writes nothing,
//     the header and the points stay as they were, and the exit code counts them. --partial writes them without what's missing (the header says so).
//   - --check writes nothing: it fits again and compares with the header on disk; the exit code counts its numbers off by more than 1 mm. It also
//     fits the M416's own dump (gun_*.bin) and prints its box beside gunbody.h's (made by hand, tried in the headset: never refitted).
//   - --pictures <folder>: each body from its left, its right, below and behind, its hull points marked by kind, its keel drawn, its lowest vertex
//     and its middle marked (body_<shape>.bmp). Look at each one.
//   - Trials, never for the header: --points <file> (bodytest's points elsewhere), --band (the side band and the top's points first), --rest
//     <weight> <degrees> (the rest ways' weight and cone), --faces <points> (a wide bottom's rest faces' points).
// The fit is src/bodyfit.h's (shared with the learner of guns never seen). Its own vertex reader: the stride and the offset from dump.txt (meshdump.h's
// LoadDraw takes 48 and is shared by other tools); half position at 0, four bone bytes at 8, four weight bytes at 12 (stride 48 and the depth passes'
// 16), or a rigid draw's half position alone (stride 32: the sights, on their palette's bone 0). Weights must sum to 250-260: more than 1% of the
// vertices off refuses the draw (a layout it doesn't know).

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "../src/bodyfit.h"
#include "../src/gunbody.h"
#include "../src/magposes.h"
#include "meshdump.h"

namespace {

// ---------------------------------------------------------------- the guns

enum Sound : int { kMetal = 0, kPolymer = 1, kHeavy = 2 };  // gunbodies::Sound's numbers
constexpr const char* kSoundNames[] = {"kMetal", "kPolymer", "kHeavy"};

// A draw of its own at rest on the gun: its bone ownBone is the body draw's bone gunBone (its rounds, its tape or the AK-74M's other piece go
// with the magazine, as skin.cpp's HeldMagBones has them); or, with onFrame, its bones at rest on the frame from a table (the M240's belt lying in
// the feed, as handmag draws it). awaited: not dumped yet, so while its dump isn't there it's named to be dumped, not a failure (once it's in the
// header it isn't awaited: missing then, the body would lose it). file: its dump's name, when another gun's draw has the same numbers (the dump
// names both mesh_<count>_<start>_: the M4A1's rounds and the M16A4's).
struct Part {
    int count = 0, start = 0;
    int ownBone = 0, gunBone = -1;
    const float (*onFrame)[12] = nullptr;
    int onFrameBones = 0;
    const char* what = "";
    bool awaited = false;
    const char* file = nullptr;
};
struct Gun {
    const char* shape;     // skin.cpp's HeldShape(), or "smaw" / "smaw_bare"
    const char* name;      // for the report
    const char* ident;     // its names in the header: k<ident>, k<ident>Hull
    int count, start;      // its body draw (the SMAW's: its depth pass)
    int frame, expectCore; // the frame bone (the table's: the space its points go into) and the bone most of its vertices should go with
    unsigned moving;       // its bones that move (its magazine, its lever, its lid, its aerial): left out only to hold its box against the research's
    float mass, round;     // kg: public figures with a typical sight (mass hardly matters: an impulse-only body on static ground moves the same)
    int capacity;
    float bounce;
    int sound;
    bool bare;             // the SMAW without its case (part A's bone 1 left out)
    Part parts[3];         // its parts drawn apart, dumped or awaited
    float researchMin[3], researchMax[3];  // the research's box (the GSDF scan, its magazine and moving bones out), or all 0
    float merge = 0.0f;    // the fit's merge distance for this gun, 0 the fit's own (1.5 cm): the UMP-45's wide magazine's bottom corner on its left lies 1.37 cm from its
                           // keel's point there, so it was merged away and, lying on that side, the corner went 13 mm into the ground (bodytest's b1-b3)
};

const Part kTape{60, 2304, 0, 4, nullptr, 0, "its box's tape"};
const Part kBelt{2304, 0, 0, -1, magposes::kM240BeltLaid, 5, "its belt lying in the feed"};
const Part kM16Rounds{312, 0, 0, 2, nullptr, 0, "its rounds"};
// Not dumped yet: each one's bone 0 on its magazine's bone in the body draw (HeldMagBones' rounds bone on its colour bone; the AK-74M's other
// piece on its palette's bone 0, as DrawHeldPart seats it).
const Part kAn94Rounds{312, 16278, 0, 4, nullptr, 0, "its rounds", true};
const Part kRpkRounds{312, 24492, 0, 1, nullptr, 0, "its rounds", true};
const Part kAek971Rounds{318, 21615, 0, 0, nullptr, 0, "its rounds", true};
const Part kAk74Piece{894, 22035, 0, 3, nullptr, 0, "its magazine's other piece", true};
const Part kAk74Rounds{312, 21723, 0, 3, nullptr, 0, "its rounds", true};
const Part kKh2002Rounds{312, 15237, 0, 1, nullptr, 0, "its rounds", true};
const Part kM4Rounds{312, 0, 0, 3, nullptr, 0, "its rounds", true, "mesh_312_0_m4a1_"};
// Semper Fidelis's two (dumped 2026-11-18): the AKS-74u's as the AK-74M's; the UMP-45's rounds first in its buffer, on its magazine's bone 0.
const Part kAks74uPiece{894, 18063, 0, 3, nullptr, 0, "its magazine's other piece"};
const Part kAks74uRounds{312, 18957, 0, 3, nullptr, 0, "its rounds"};
const Part kUmp45Rounds{234, 0, 0, 0, nullptr, 0, "its rounds"};
// The Mk11 Mod 0 (Operation Swordbreaker, dumped 2026-11-18): its buffer begins with a draw of two bones (1902/0: bone 0 the frame, a plate at the
// stock's neck; bone 1 the magazine, the sleeve round its bottom), then its rounds (312/1902, on the magazine), then its body (12456/2214).
const Part kMk11Piece{1902, 0, 0, 0, nullptr, 0, "its stock's plate and its magazine's sleeve"};
const Part kMk11Rounds{312, 1902, 0, 3, nullptr, 0, "its rounds"};
// The M249 (Operation Swordbreaker, dumped 2026-11-18): its box, a cloth pouch, is a draw of its own (987/27099) alone on its palette's bone 0, which no
// bone of the body draw is: placed by its bone 0 on the frame at rest, from the two dumps (taken in the same frame; held to them by tools/m249test.cpp's
// k2); and its belt lying in the feed, as handmag draws it.
constexpr float kM249BoxRest[1][12] = {{1.0f, 0.00086f, -0.00044f, -0.00009f, -0.00086f, 1.0f, 0.00296f, 0.06350f, 0.00044f, -0.00295f, 1.0f, 0.33287f}};
const Part kM249Box{987, 27099, 0, -1, kM249BoxRest, 1, "its box"};
const Part kM249Belt{2808, 0, 0, -1, magposes::kM249BeltLaid, 5, "its belt lying in the feed"};

const Gun kGuns[] = {
    {"an94", "AN-94", "An94", 16188, 0, 0, 0, 1u << 4 | 1u << 2, 3.85f, 0.0102f, 30, 1.0f, kMetal, false, {kAn94Rounds},
     {-0.026f, -0.087f, -0.004f}, {0.019f, 0.105f, 0.950f}},
    {"rpk", "RPK-74M", "Rpk", 24492, 0, 3, 3, 1u << 1 | 1u << 0, 4.8f, 0.0102f, 45, 1.0f, kMetal, false, {kRpkRounds},
     {-0.028f, -0.136f, -0.034f}, {0.026f, 0.092f, 1.127f}},
    {"aek971", "AEK-971", "Aek971", 21585, 0, 4, 4, 1u << 0 | 1u << 2, 3.3f, 0.0102f, 30, 1.0f, kMetal, false, {kAek971Rounds},
     {-0.025f, -0.117f, -0.066f}, {0.026f, 0.099f, 0.993f}},
    {"ak74", "AK-74M", "Ak74", 21723, 0, 2, 2, 1u << 3 | 1u << 1, 3.4f, 0.0102f, 30, 1.0f, kMetal, false, {kAk74Piece, kAk74Rounds},
     {-0.024f, -0.115f, -0.007f}, {0.024f, 0.074f, 0.938f}},
    {"g3a3", "G3A3", "G3a3", 23064, 0, 0, 0, 1u << 1 | 1u << 3, 4.4f, 0.024f, 20, 1.0f, kMetal, false, {},
     {-0.032f, -0.116f, -0.031f}, {0.034f, 0.115f, 1.031f}},
    {"kh2002", "KH2002", "Kh2002", 15237, 0, 3, 3, 1u << 1 | 1u << 2, 3.7f, 0.0123f, 30, 1.0f, kMetal, false, {kKh2002Rounds},
     {-0.029f, -0.108f, -0.025f}, {0.028f, 0.090f, 0.713f}},
    {"m16a4", "M16A4", "M16a4", 17013, 312, 4, 4, 1u << 2 | 1u << 0, 3.4f, 0.0123f, 30, 1.0f, kMetal, false, {kM16Rounds},
     {-0.039f, -0.140f, -0.020f}, {0.030f, 0.061f, 1.048f}},
    {"m4a1", "M4A1", "M4a1", 22194, 312, 2, 2, 1u << 3 | 1u << 0 | 1u << 1, 2.9f, 0.0123f, 30, 1.0f, kMetal, false, {kM4Rounds},
     {-0.039f, -0.140f, 0.022f}, {0.030f, 0.061f, 0.910f}},
    {"m240", "M240", "M240", 27588, 2364, 3, 3, 1u << 4 | 1u << 2 | 1u << 5, 12.5f, 0.027f, 100, 0.8f, kHeavy, false, {kTape, kBelt},
     {-0.087f, -0.141f, -0.029f}, {0.054f, 0.099f, 1.201f}},
    {"fim92", "Stinger", "Fim92", 20298, 354, 0, 0, 1u << 3 | 1u << 4 | 1u << 5 | 1u << 6, 15.2f, 0.0f, 1, 0.8f, kHeavy, false, {},
     {-0.071f, -0.178f, -0.347f}, {0.148f, 0.128f, 1.174f}},
    // (No research box for these two: measured after the scan. 2.7 kg and 2.5 kg empty, public figures.)
    {"aks74u", "AKS-74u", "Aks74u", 17991, 0, 2, 2, 1u << 3 | 1u << 0, 2.7f, 0.0102f, 30, 1.0f, kMetal, false, {kAks74uPiece, kAks74uRounds}, {}, {}},
    {"ump45", "UMP-45", "Ump45", 19314, 234, 2, 2, 1u << 0 | 1u << 1, 2.5f, 0.021f, 25, 1.0f, kMetal, false, {kUmp45Rounds}, {}, {}, 0.012f},
    // The M1014 (Semper Fidelis, 2026-11-18): its bolt and handle are bone 0; its sight's dots (324/13377) lie inside its box and its shells are in
    // its tube, so no part drawn apart is added. 3.8 kg (the Benelli M4's public figure); a 12-gauge shell about 45 g; 5 as the game counts it (4 in
    // the tube and the chambered one). No research box (it was never in the GSDF scan).
    // (Its grip's bottom is 3.4 cm wide and its tube's end, 10 cm higher, 4 cm: dropped near upright it can come to rest standing on the two, its muzzle
    // down 12 degrees, as a real one can: bodytest counts that pose apart.)
    {"m1014", "M1014", "M1014", 13377, 0, 3, 3, 1u << 0, 3.8f, 0.045f, 5, 1.0f, kMetal, false, {}, {}, {}},
    // The Mk11 Mod 0 (Operation Swordbreaker, 2026-11-18): frame bone 0, its magazine bone 3 (its trigger 1 and its port's cover 2 stay); its scope
    // is a sight drawn apart (kSights). 4.9 kg (the SR-25's public figure); a 7.62 round 24 g; 10 as the game counts its magazine. No research box.
    {"mk11", "Mk11 Mod 0", "Mk11", 12456, 2214, 0, 0, 1u << 3, 4.9f, 0.024f, 10, 1.0f, kMetal, false, {kMk11Piece, kMk11Rounds}, {}, {}},
    // The M249 (Operation Swordbreaker, 2026-11-18): frame bone 3, its handle 0 and its feed cover 1 move (its carrying handle 2 stays); its box and its belt
    // drawn apart. 7.5 kg (its public figure, empty); a 5.56 round with its link 13.3 g; 100 as the game counts its box: 8.8 kg with them (the free hands'
    // line says both: loose::WeightWords; its pouch's own 0.2 kg is its dropped box's, as no gun's magazine is its body's). As heavy as the M240 to the ear.
    // No research box.
    {"m249", "M249", "M249", 24291, 2808, 3, 3, 1u << 0 | 1u << 1, 7.5f, 0.0133f, 100, 0.8f, kHeavy, false, {kM249Box, kM249Belt}, {}, {}},
    // The AT4 (Operation Swordbreaker, dumped 2026-11-18): one draw on four bones, frame bone 0 the tube; its small moving parts (1 the cocking lever, 2
    // the safety, 3 the folding front grip, which the game keeps turned down under the tube) left out of the hull. Thrown away spent: 5.0 kg empty
    // (the M136's public 6.7 kg less its 1.7 kg rocket), one rocket. A fibreglass tube: its knocks the hollow plastic's. No research box.
    {"at4", "AT4", "At4", 15987, 0, 0, 0, 1u << 1 | 1u << 2 | 1u << 3, 5.0f, 1.7f, 1, 0.8f, kPolymer, false, {}, {}, {}},
    {"smaw", "SMAW (its case on)", "Smaw", 21756, 21756, 0, 2, 0u, 13.6f, 0.0f, 1, 0.8f, kHeavy, false, {}, {}, {}},
    {"smaw_bare", "SMAW (bare)", "SmawBare", 21756, 21756, 0, 2, 0u, 7.7f, 0.0f, 1, 0.8f, kHeavy, true, {},
     {-0.112f, -0.179f, -0.193f}, {0.083f, 0.181f, 0.641f}},
};
// The SMAW's case at home on the frame (docs/research/smaw.md: z -0.726 to -0.085, round x 0, y 0.0915): part A's bone 1 there is the case.
constexpr float kCaseHome[3] = {0.0f, 0.0915f, -0.4f};
constexpr int kCaseBone = 1;

// The sights drawn apart (skin.cpp's kHeldSights' bodies): rigid, on their palette's bone 0.
struct Sight {
    int count, start, base;
    const char* name;
};
const Sight kSights[] = {
    {10599, 72, 0, "the PSO-1"}, {9084, 57, 0, "the PKS-07"}, {3723, 48, 0, "the PK-A"}, {7884, 60, 0, "the PK-AS"}, {6492, 210, 0, "the Kobra"},
    {9384, 0, 0, "the EoTech"}, {12642, 156, 0, "the rifle scope"},
    {9273, 12, 0, "the ACOG"},  // (the M416's own, on a held gun: the bridge M249's; src/acog.h. The M416's own body, gunbody.h's, has it already.)
};
constexpr int kAttachPoints = 8;

// The M416 without its M320: gunbody::kM416Hull less the M320's tube (its rows 14-17) and its grip's bottom (21); 3.9 kg (the M416 3.5, the ACOG 0.4).
constexpr int kM416BareDrop[] = {14, 15, 16, 17, 21};
constexpr float kM416BareMass = 3.9f;

bodyfit::Options g_options;  // the fit's (the defaults; "--band" for a trial with the side band and the top first)

// ---------------------------------------------------------------- reading a dump

std::vector<uint8_t> ReadAll(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
std::string Format(const char* format, ...) {
    char s[1024];
    va_list args;
    va_start(args, format);
    std::vsnprintf(s, sizeof(s), format, args);
    va_end(args);
    return s;
}

struct Dump {
    std::string name;  // mesh_<count>_<start>_
    int count = 0, start = 0, base = 0;
    int indexFormat = 0;
    unsigned indexOffset = 0, stride = 0, offset = 0;
    std::vector<uint8_t> vb, ib, bones;
    int Bones() const { return static_cast<int>(bones.size() / 48); }
    const float* Bone(int b) const { return reinterpret_cast<const float*>(bones.data()) + b * 12; }
};

// The files' name: mesh_<count>_<start>_ as the dump writes them, or file (one saved apart: another gun's draw has the same numbers).
std::string DumpName(int count, int start, const char* file = nullptr) {
    return file ? std::string(file) : "mesh_" + std::to_string(count) + "_" + std::to_string(start) + "_";
}
bool Read(const std::string& dir, int count, int start, Dump& d, std::string& why, const char* file = nullptr) {
    d = Dump{};
    d.count = count, d.start = start;
    d.name = DumpName(count, start, file);
    std::ifstream text(dir + "\\" + d.name + "dump.txt");
    if (!text) {
        why = d.name + "dump.txt isn't there";
        return false;
    }
    std::string line;
    bool haveDraw = false, haveIb = false, haveVb = false;
    while (std::getline(text, line)) {
        std::istringstream in(line);
        std::string word;
        in >> word;
        if (word == "draw") {
            int c = 0, s = 0, b = 0;
            if (in >> c >> s >> b)
                haveDraw = c == count && s == start, d.base = b;
        } else if (word == "ib") {
            std::string formatWord, offsetWord;
            if (in >> formatWord >> d.indexFormat >> offsetWord >> d.indexOffset)
                haveIb = true;
        } else if (word == "vb") {
            int slot = -1;
            std::string strideWord, offsetWord;
            unsigned stride = 0, offset = 0;
            if (in >> slot >> strideWord >> stride >> offsetWord >> offset && slot == 0)
                d.stride = stride, d.offset = offset, haveVb = true;
        }
    }
    if (!haveDraw || !haveIb || !haveVb) {
        why = d.name + "dump.txt doesn't say its draw, its indices and its vertex buffer 0";
        return false;
    }
    d.vb = ReadAll(dir + "\\" + d.name + "vb0.bin");
    d.ib = ReadAll(dir + "\\" + d.name + "ib.bin");
    d.bones = ReadAll(dir + "\\" + d.name + "bones.bin");
    const size_t width = d.indexFormat == 57 ? 2 : d.indexFormat == 42 ? 4 : 0;
    if (!width) {
        why = d.name + ": its indices are neither 16 nor 32 bits (format " + std::to_string(d.indexFormat) + ")";
        return false;
    }
    if (d.ib.size() < d.indexOffset + width * (static_cast<size_t>(start) + count) || d.vb.size() <= d.offset || d.stride < 8 || d.bones.size() < 48) {
        why = d.name + ": its buffers are shorter than its draw";
        return false;
    }
    return true;
}
int IndexAt(const Dump& d, int i) {
    if (d.indexFormat == 57) {
        uint16_t v;
        std::memcpy(&v, &d.ib[d.indexOffset + 2 * static_cast<size_t>(i)], 2);
        return v + d.base;
    }
    uint32_t v;
    std::memcpy(&v, &d.ib[d.indexOffset + 4 * static_cast<size_t>(i)], 4);
    return static_cast<int>(v) + d.base;
}
int VerticesIn(const Dump& d) { return static_cast<int>((d.vb.size() - d.offset) / d.stride); }

// ---------------------------------------------------------------- matrices (3 rows: rotation | translation)

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
    for (int k = 0; k < 3; ++k)
        out[k] = m[k * 4] * p[0] + m[k * 4 + 1] * p[1] + m[k * 4 + 2] * p[2] + m[k * 4 + 3];
}
// A turn with a move: its rows unit and square to each other within 1%.
bool Rigid(const float* m) {
    for (int i = 0; i < 12; ++i)
        if (!std::isfinite(m[i]))
            return false;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            const float dot = m[i * 4] * m[j * 4] + m[i * 4 + 1] * m[j * 4 + 1] + m[i * 4 + 2] * m[j * 4 + 2];
            if (std::fabs(dot - (i == j ? 1.0f : 0.0f)) > 0.01f)
                return false;
        }
    return true;
}
void InvertRigid(const float* m, float* inv) {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            inv[i * 4 + j] = m[j * 4 + i];
    for (int i = 0; i < 3; ++i)
        inv[i * 4 + 3] = -(inv[i * 4] * m[3] + inv[i * 4 + 1] * m[7] + inv[i * 4 + 2] * m[11]);
}

// ---------------------------------------------------------------- posing

struct Mesh {
    std::vector<float> pos;  // xyz a vertex, in the gun's frame bone's space
    std::vector<int> bone;   // each vertex's bone of most weight (a part's: kPartBone + its number)
    std::vector<int> tris;   // 3 a triangle
};
constexpr int kPartBone = 48;  // a part's vertices go with bone kPartBone + its number (bodyfit's bones are 0..63)

// A skinned draw's vertices posed by palette (12 floats a bone: its bind space into the frame's), every vertex its draw's indices reach; bone: which
// bone each takes (-1: its own bone of most weight). False with why: a layout it doesn't know, a bone past its palette, numbers that aren't.
bool AddSkinned(const Dump& d, const std::vector<float>& palette, int bone, Mesh& m, std::string& why) {
    if (d.stride != 48 && d.stride != 16) {
        why = d.name + ": stride " + std::to_string(d.stride) + " isn't a skinned layout it knows (48, 16)";
        return false;
    }
    const int n = VerticesIn(d), bones = static_cast<int>(palette.size() / 12);
    std::vector<int> index(static_cast<size_t>(n), -1);
    int off = 0, used = 0, broken = 0;
    std::vector<int> tris;
    for (int i = d.start; i + 2 < d.start + d.count; i += 3) {
        int corner[3];
        bool keep = true;
        for (int c = 0; c < 3; ++c) {
            const int v = IndexAt(d, i + c);
            if (v < 0 || v >= n) {
                keep = false;
                continue;
            }
            if (index[v] == -1) {
                const uint8_t* o = &d.vb[d.offset + static_cast<size_t>(v) * d.stride];
                uint16_t h[3];
                std::memcpy(h, o, 6);
                const float p[3] = {meshdump::Half(h[0]), meshdump::Half(h[1]), meshdump::Half(h[2])};
                float sum[3] = {};
                int weights = 0, main = -1, mainWeight = -1;
                bool ok = std::isfinite(p[0]) && std::isfinite(p[1]) && std::isfinite(p[2]);
                for (int s = 0; s < 4 && ok; ++s) {
                    const int b = o[8 + s], w = o[12 + s];
                    weights += w;
                    if (!w)
                        continue;
                    if (b >= bones) {
                        ok = false;
                        break;
                    }
                    float q[3];
                    Apply(&palette[static_cast<size_t>(b) * 12], p, q);
                    for (int k = 0; k < 3; ++k)
                        sum[k] += q[k] * static_cast<float>(w);
                    if (w > mainWeight)
                        mainWeight = w, main = b;
                }
                ++used;
                if (weights < 250 || weights > 260)
                    ++off;
                if (!ok || weights <= 0 || !std::isfinite(sum[0]) || !std::isfinite(sum[1]) || !std::isfinite(sum[2])) {
                    index[v] = -2, ++broken;
                } else {
                    index[v] = static_cast<int>(m.bone.size());
                    for (int k = 0; k < 3; ++k)
                        m.pos.push_back(sum[k] / static_cast<float>(weights));
                    m.bone.push_back(bone >= 0 ? bone : main);
                }
            }
            if (index[v] < 0)
                keep = false;
            corner[c] = index[v];
        }
        if (keep)
            tris.insert(tris.end(), corner, corner + 3);
    }
    if (used == 0 || off * 100 > used) {
        why = d.name + ": " + std::to_string(off) + " of " + std::to_string(used) + " vertices' weights don't sum to 250-260 (a layout it doesn't know)";
        return false;
    }
    if (broken * 100 > used) {
        why = d.name + ": " + std::to_string(broken) + " of " + std::to_string(used) + " vertices on a bone past its palette or not numbers";
        return false;
    }
    m.tris.insert(m.tris.end(), tris.begin(), tris.end());
    return true;
}

// A rigid draw's vertices (the sights' stride 32: a half position and a fourth half of 1 or -1, no bones), in its own bone 0's space.
bool AddRigid(const Dump& d, Mesh& m, std::string& why) {
    if (d.stride != 32) {
        why = d.name + ": stride " + std::to_string(d.stride) + " isn't the rigid layout (32)";
        return false;
    }
    const int n = VerticesIn(d);
    std::vector<int> index(static_cast<size_t>(n), -1);
    int used = 0, off = 0;
    for (int i = d.start; i + 2 < d.start + d.count; i += 3) {
        int corner[3];
        bool keep = true;
        for (int c = 0; c < 3; ++c) {
            const int v = IndexAt(d, i + c);
            if (v < 0 || v >= n) {
                keep = false;
                continue;
            }
            if (index[v] == -1) {
                const uint8_t* o = &d.vb[d.offset + static_cast<size_t>(v) * d.stride];
                uint16_t h[4];
                std::memcpy(h, o, 8);
                const float p[3] = {meshdump::Half(h[0]), meshdump::Half(h[1]), meshdump::Half(h[2])};
                ++used;
                off += h[3] != 0x3C00 && h[3] != 0xBC00;  // its fourth: 1 or -1 (a sign, not the place's)
                if (!std::isfinite(p[0]) || !std::isfinite(p[1]) || !std::isfinite(p[2]) || std::fabs(p[0]) > 1.0f || std::fabs(p[1]) > 1.0f ||
                    std::fabs(p[2]) > 1.0f) {
                    index[v] = -2;
                } else {
                    index[v] = static_cast<int>(m.bone.size());
                    m.pos.insert(m.pos.end(), p, p + 3);
                    m.bone.push_back(0);
                }
            }
            if (index[v] < 0)
                keep = false;
            corner[c] = index[v];
        }
        if (keep)
            m.tris.insert(m.tris.end(), corner, corner + 3);
    }
    if (used == 0 || off * 100 > used) {
        why = d.name + ": " + std::to_string(off) + " of " + std::to_string(used) + " vertices without a fourth half of 1 or -1 (a layout it doesn't know)";
        return false;
    }
    return true;
}

// The body draw's palette into its frame bone's space.
bool FramePalette(const Dump& d, int frame, std::vector<float>& palette, std::string& why) {
    if (frame < 0 || frame >= d.Bones() || !Rigid(d.Bone(frame))) {
        why = d.name + ": its frame bone " + std::to_string(frame) + " isn't a turn with a move";
        return false;
    }
    float toFrame[12];
    InvertRigid(d.Bone(frame), toFrame);
    palette.assign(static_cast<size_t>(d.Bones()) * 12, 0.0f);
    for (int b = 0; b < d.Bones(); ++b)
        Compose(toFrame, d.Bone(b), &palette[static_cast<size_t>(b) * 12]);
    return true;
}

// ---------------------------------------------------------------- one gun

struct Fitted {
    const Gun* gun = nullptr;
    bool ok = false;
    std::string why;
    bodyfit::Result r;
    int derivedFrame = -1;       // the body draw's own bone of most weight (the frame bone, checked)
    int vertices = 0;            // the body draw's vertices
    std::vector<std::string> notes;
    int partsFailed = 0;         // its parts that aren't awaited, or are dumped, and weren't added (its body would lose them)
    std::vector<std::string> awaited;  // its parts not dumped yet, how to dump each (the report's last line)
    Mesh mesh;                   // what was fitted (kept or not)
    float lowest[3] = {};        // its lowest kept vertex
    std::vector<float> outer;    // up to 512 of its outermost kept vertices (bodytest's)
    float researchOff = -1.0f;   // the largest difference from the research's box, what moves left out (mm); -1 none to compare
    // How far its mesh reaches past its hull, the worst way (lying with that way down: how deep the mesh goes into the ground), and the worst way
    // not within 45 degrees of standing upright (where the keel narrows its bottom on purpose).
    float worst = 0.0f, worstWay[3] = {}, lyingWorst = 0.0f, lyingWay[3] = {};
};
void Gaps(const Mesh& m, const std::vector<unsigned char>& kept, const bodyfit::Result& r, Fitted& out) {
    constexpr int kWays = 2048;
    for (int i = 0; i < kWays; ++i) {
        const float y = 1.0f - 2.0f * (i + 0.5f) / kWays, s = std::sqrt(std::max(0.0f, 1.0f - y * y)), a = 2.39996323f * static_cast<float>(i);
        const float way[3] = {s * std::cos(a), y, s * std::sin(a)};
        float mesh = -1e30f, hull = -1e30f;
        for (size_t v = 0; v < kept.size(); ++v)
            if (kept[v])
                mesh = std::max(mesh, m.pos[v * 3] * way[0] + m.pos[v * 3 + 1] * way[1] + m.pos[v * 3 + 2] * way[2]);
        for (int p = 0; p < r.hullPoints; ++p)
            hull = std::max(hull, r.hull[p][0] * way[0] + r.hull[p][1] * way[1] + r.hull[p][2] * way[2]);
        const float gap = mesh - hull;
        if (gap > out.worst)
            out.worst = gap, std::memcpy(out.worstWay, way, sizeof(way));
        if (way[1] > -0.7071f && gap > out.lyingWorst)
            out.lyingWorst = gap, std::memcpy(out.lyingWay, way, sizeof(way));
    }
}

// Up to most of the kept vertices furthest out, each along one of many ways spread over the sphere from the middle: the ones that can touch a plane.
std::vector<float> Outermost(const Mesh& m, const std::vector<unsigned char>& kept, const float* middle, int most) {
    constexpr int kWays = 2048;
    std::vector<int> chosen;
    for (int i = 0; i < kWays; ++i) {
        const float y = 1.0f - 2.0f * (i + 0.5f) / kWays, r = std::sqrt(std::max(0.0f, 1.0f - y * y)), a = 2.39996323f * static_cast<float>(i);
        const float way[3] = {r * std::cos(a), y, r * std::sin(a)};
        int best = -1;
        float bestAlong = -1e30f;
        for (size_t v = 0; v < m.bone.size(); ++v) {
            if (!kept[v])
                continue;
            const float along = (m.pos[v * 3] - middle[0]) * way[0] + (m.pos[v * 3 + 1] - middle[1]) * way[1] + (m.pos[v * 3 + 2] - middle[2]) * way[2];
            if (along > bestAlong)
                bestAlong = along, best = static_cast<int>(v);
        }
        if (best >= 0 && std::find(chosen.begin(), chosen.end(), best) == chosen.end())
            chosen.push_back(best);
    }
    std::vector<float> out;
    const size_t step = chosen.size() > static_cast<size_t>(most) ? (chosen.size() + most - 1) / most : 1;
    for (size_t i = 0; i < chosen.size() && out.size() / 3 < static_cast<size_t>(most); i += step)
        out.insert(out.end(), &m.pos[static_cast<size_t>(chosen[i]) * 3], &m.pos[static_cast<size_t>(chosen[i]) * 3] + 3);
    return out;
}

Fitted FitGun(const std::string& dir, const Gun& g) {
    Fitted out;
    out.gun = &g;
    Dump body;
    std::string why;
    std::vector<float> palette;
    if (!Read(dir, g.count, g.start, body, why) || !FramePalette(body, g.frame, palette, why) || !AddSkinned(body, palette, -1, out.mesh, why)) {
        out.why = why;
        return out;
    }
    out.vertices = static_cast<int>(out.mesh.bone.size());
    out.derivedFrame = bodyfit::MostWeighted(out.mesh.bone.data(), out.vertices);
    // Its parts drawn apart, on their own bones at rest on the frame: only a dump of the same buffer is this gun's (another gun's draw can
    // have the same numbers: the M4A1's rounds and the M16A4's). One not dumped yet is named; any other not added fails the gun.
    std::string notDumped;
    for (int partNumber = 0; partNumber < static_cast<int>(std::size(g.parts)); ++partNumber) {
        const Part& part = g.parts[partNumber];
        if (!part.count)
            continue;
        if (part.awaited && !std::filesystem::exists(dir + "\\" + DumpName(part.count, part.start, part.file) + "dump.txt")) {
            const std::string numbers = Format("%d@%d%s", part.count, part.start, part.file ? " (in its own buffer)" : "");
            notDumped += (notDumped.empty() ? "" : ", ") + Format("%s %s", part.what, numbers.c_str());
            std::string how = Format("%s %s", std::strncmp(part.what, "its ", 4) == 0 ? part.what + 4 : part.what, numbers.c_str());  // (after "the AN-94's")
            if (part.file)
                how += Format(": the dump names its files %s*, as another gun's: move those aside first, rename the new ones %s*, then put the others back",
                              DumpName(part.count, part.start).c_str(), part.file);
            out.awaited.push_back(how);
            continue;
        }
        Dump d;
        std::vector<float> partPalette;
        bool ok = Read(dir, part.count, part.start, d, why, part.file);
        if (ok && d.vb.size() != body.vb.size()) {
            why = d.name + " is another buffer's (" + std::to_string(d.vb.size()) + " bytes, its gun's " + std::to_string(body.vb.size()) + ")";
            ok = false;
        }
        if (ok && part.onFrame) {
            partPalette.assign(static_cast<size_t>(part.onFrameBones) * 12, 0.0f);
            for (int b = 0; b < part.onFrameBones; ++b)
                std::memcpy(&partPalette[static_cast<size_t>(b) * 12], part.onFrame[b], 48);
        } else if (ok) {
            // Its own palette carried onto the frame by the bone it shares with the body: frame <- body's gunBone <- its ownBone.
            if (part.ownBone >= d.Bones() || part.gunBone < 0 || part.gunBone >= body.Bones() || !Rigid(d.Bone(part.ownBone))) {
                why = d.name + ": its bone " + std::to_string(part.ownBone) + " can't be carried onto the gun's " + std::to_string(part.gunBone);
                ok = false;
            } else {
                float fromOwn[12], onFrame[12];
                InvertRigid(d.Bone(part.ownBone), fromOwn);
                Compose(&palette[static_cast<size_t>(part.gunBone) * 12], fromOwn, onFrame);
                partPalette.assign(static_cast<size_t>(d.Bones()) * 12, 0.0f);
                for (int b = 0; b < d.Bones(); ++b)
                    Compose(onFrame, d.Bone(b), &partPalette[static_cast<size_t>(b) * 12]);
            }
        }
        if (ok)
            ok = AddSkinned(d, partPalette, kPartBone + partNumber, out.mesh, why);
        out.partsFailed += !ok;
        out.notes.push_back(ok ? Format("added %s (%d@%d)", part.what, part.count, part.start)
                               : Format("NOT added: %s (%d@%d): %s", part.what, part.count, part.start, why.c_str()));
    }
    if (!notDumped.empty())
        out.notes.push_back("not dumped yet (weapon.txt \"dump N S\", then fit again): " + notDumped);
    // The SMAW: part A's bone 1 is its case while it sits at home; else the case is its spec's rings placed there (loose's "smaw"), for the body
    // with its case. Bare, everything on bone 1 is left out.
    std::vector<int> tris;
    if (std::strncmp(g.shape, "smaw", 4) == 0) {
        float lo[3] = {1e9f, 1e9f, 1e9f}, hi[3] = {-1e9f, -1e9f, -1e9f};
        int caseVertices = 0;
        for (size_t v = 0; v < out.mesh.bone.size(); ++v)
            if (out.mesh.bone[v] == kCaseBone) {
                ++caseVertices;
                for (int k = 0; k < 3; ++k)
                    lo[k] = std::min(lo[k], out.mesh.pos[v * 3 + k]), hi[k] = std::max(hi[k], out.mesh.pos[v * 3 + k]);
            }
        float off = 1e9f;
        if (caseVertices) {
            const float middle[3] = {0.5f * (lo[0] + hi[0]), 0.5f * (lo[1] + hi[1]), 0.5f * (lo[2] + hi[2])};
            off = std::sqrt(bodyfit::detail::Distance2(middle, kCaseHome));
        }
        const bool atHome = caseVertices > 0 && off <= 0.02f;
        for (size_t t = 0; t + 2 < out.mesh.tris.size(); t += 3) {
            bool onCase = false;
            for (int c = 0; c < 3; ++c)
                onCase |= out.mesh.bone[out.mesh.tris[t + c]] == kCaseBone;
            if (g.bare ? !onCase : (!onCase || atHome))
                tris.insert(tris.end(), &out.mesh.tris[t], &out.mesh.tris[t] + 3);
        }
        char line[200];
        if (g.bare) {
            std::snprintf(line, sizeof(line), "its case (bone %d, %d vertices) left out", kCaseBone, caseVertices);
        } else if (atHome) {
            std::snprintf(line, sizeof(line), "its case: bone %d's %d vertices, at home (their middle %.1f mm from it)", kCaseBone, caseVertices, off * 1000.0f);
        } else {
            // Eight points on its rings at home: z -0.726 radius 0.068 (the flange), z -0.085 radius 0.0445; each a triangle of itself (no area).
            const float rings[2][2] = {{-0.726f, 0.068f}, {-0.085f, 0.0445f}};
            for (const auto& ring : rings)
                for (int k = 0; k < 4; ++k) {
                    const float a = 1.5707963f * static_cast<float>(k);
                    const float p[3] = {kCaseHome[0] + ring[1] * std::cos(a), kCaseHome[1] + ring[1] * std::sin(a), ring[0]};
                    const int v = static_cast<int>(out.mesh.bone.size());
                    out.mesh.pos.insert(out.mesh.pos.end(), p, p + 3);
                    out.mesh.bone.push_back(kCaseBone);
                    tris.insert(tris.end(), {v, v, v});
                }
            std::snprintf(line, sizeof(line), "its case: bone %d isn't at home (%d vertices, %.0f mm off): its spec's rings put there instead", kCaseBone,
                          caseVertices, off * 1000.0f);
        }
        out.notes.push_back(line);
    } else {
        tris = out.mesh.tris;
    }
    bodyfit::Options options = g_options;
    if (g.merge > 0.0f)
        options.merge = g.merge;
    out.r = bodyfit::Fit(out.mesh.pos.data(), out.mesh.bone.data(), static_cast<int>(out.mesh.bone.size()), tris.data(), static_cast<int>(tris.size() / 3), options);
    if (!out.r.ok) {
        out.why = std::string("the fit refused it: ") + out.r.why;
        return out;
    }
    out.mesh.tris = tris;
    // What the fit kept (for the pictures and bodytest): every corner of a triangle with none on a bone left out.
    std::vector<unsigned char> kept(out.mesh.bone.size(), 0);
    auto leftOut = [&](int b) {
        for (int i = 0; i < out.r.leftOutCount; ++i)
            if (out.r.leftOut[i] == b)
                return true;
        return false;
    };
    for (size_t t = 0; t + 2 < tris.size(); t += 3) {
        bool keep = true;
        for (int c = 0; c < 3; ++c)
            keep &= !leftOut(out.mesh.bone[tris[t + c]]);
        if (keep)
            for (int c = 0; c < 3; ++c)
                kept[tris[t + c]] = 1;
    }
    float low = 1e9f;
    for (size_t v = 0; v < kept.size(); ++v)
        if (kept[v] && out.mesh.pos[v * 3 + 1] < low)
            low = out.mesh.pos[v * 3 + 1], std::memcpy(out.lowest, &out.mesh.pos[v * 3], sizeof(out.lowest));
    out.outer = Outermost(out.mesh, kept, out.r.centre, 512);
    Gaps(out.mesh, kept, out.r, out);
    // Against the research's box (its magazine and moving parts out, from the shapes' cells within 3 mm: about 2 mm), the kept mesh less what
    // moves and its parts.
    if (g.researchMax[2] > g.researchMin[2]) {
        float lo[3] = {1e9f, 1e9f, 1e9f}, hi[3] = {-1e9f, -1e9f, -1e9f};
        for (size_t v = 0; v < kept.size(); ++v) {
            const int b = out.mesh.bone[v];
            if (kept[v] && b < kPartBone && !(b < 32 && (g.moving >> b & 1u)))
                for (int k = 0; k < 3; ++k)
                    lo[k] = std::min(lo[k], out.mesh.pos[v * 3 + k]), hi[k] = std::max(hi[k], out.mesh.pos[v * 3 + k]);
        }
        out.researchOff = 0.0f;
        for (int k = 0; k < 3; ++k)
            out.researchOff = std::max({out.researchOff, std::fabs(lo[k] - g.researchMin[k]) * 1000.0f, std::fabs(hi[k] - g.researchMax[k]) * 1000.0f});
    }
    out.ok = true;
    return out;
}

// ---------------------------------------------------------------- the sights

struct FittedSight {
    const Sight* sight = nullptr;
    bool ok = false;
    std::string why;
    int points = 0;
    float point[kAttachPoints][3] = {};
    float lo[3] = {}, hi[3] = {};
};
FittedSight FitSight(const std::string& dir, const Sight& s) {
    FittedSight out;
    out.sight = &s;
    Dump d;
    Mesh m;
    std::string why;
    if (!Read(dir, s.count, s.start, d, why) || d.base != s.base || !AddRigid(d, m, why)) {
        out.why = why.empty() ? "its dump's base isn't the table's" : why;
        return out;
    }
    std::vector<unsigned char> used(m.bone.size(), 0);
    for (int v : m.tris)
        used[v] = 1;
    for (int k = 0; k < 3; ++k)
        out.lo[k] = 1e9f, out.hi[k] = -1e9f;
    for (size_t v = 0; v < used.size(); ++v)
        if (used[v])
            for (int k = 0; k < 3; ++k)
                out.lo[k] = std::min(out.lo[k], m.pos[v * 3 + k]), out.hi[k] = std::max(out.hi[k], m.pos[v * 3 + k]);
    // The furthest along each of the box's eight corners, in its own proportions; one within 1.5 cm of one kept is the same.
    float mid[3], half[3];
    for (int k = 0; k < 3; ++k)
        mid[k] = 0.5f * (out.lo[k] + out.hi[k]), half[k] = std::max(0.5f * (out.hi[k] - out.lo[k]), 1e-4f);
    for (int corner = 0; corner < 8; ++corner) {
        const float way[3] = {corner & 1 ? 1.0f : -1.0f, corner & 2 ? 1.0f : -1.0f, corner & 4 ? 1.0f : -1.0f};
        int best = -1;
        float bestAlong = -1e30f;
        for (size_t v = 0; v < used.size(); ++v) {
            if (!used[v])
                continue;
            float along = 0.0f;
            for (int k = 0; k < 3; ++k)
                along += (m.pos[v * 3 + k] - mid[k]) / half[k] * way[k];
            if (along > bestAlong)
                bestAlong = along, best = static_cast<int>(v);
        }
        if (best >= 0)
            bodyfit::detail::Add(out.point, nullptr, out.points, &m.pos[static_cast<size_t>(best) * 3], bodyfit::kExtra, 0.015f);
        if (out.points > kAttachPoints)
            out.points = kAttachPoints;
    }
    out.ok = out.points > 0;
    if (!out.ok)
        out.why = "no vertex";
    return out;
}

// ---------------------------------------------------------------- the header

// A number for a comment: none of -0.000.
float Tidy(float v) { return std::fabs(v) < 0.0005f ? 0.0f : v; }
std::string Num(float v) {
    if (std::fabs(v) < 0.00005f)
        v = 0.0f;
    char s[32];
    std::snprintf(s, sizeof(s), "%.4ff", v);
    return s;
}
std::string Point(const float* p) { return "{" + Num(p[0]) + ", " + Num(p[1]) + ", " + Num(p[2]) + "}"; }

std::string Header(const std::vector<Fitted>& guns, const std::vector<FittedSight>& sights) {
    std::string h;
    h += "#pragma once\n";
    h += "// Every gun's body when it's let go of (free hands; loose.h), fitted by tools/gunbodyfit.exe from the game's own first-person meshes (build\\mesh_*.bin).\n";
    h += "// Written by it, never by hand: run \"gunbodyfit build src\\gunbodies.h\" again. The M416 keeps gunbody.h's (made by hand, tried in the headset).\n";
    h += "// In each gun's frame bone's space (x to its left, y up, z to the muzzle; metres). Its middle is its kept triangles' area-weighted middle (its box\n";
    h += "// for loose is its core bone's size about it); its hull points are its mesh's own vertices, furthest out every way (src/bodyfit.h). Under a\n";
    h += "// narrow bottom those along it no wider than its core are on one line along z (its keel) with its middle beside it, so stood upright it tips\n";
    h += "// over, and the same vertices 5 mm up (its guards) hold it on its side; a bottom 4 cm wide or more has none (it may stand on it). Its points\n";
    h += "// before restFrom matter most: an attachment's go before the rest (bodyfit::WithExtra).\n";
    std::string missing;  // (written only with --partial: without it, nothing is written while anything's missing)
    for (const Fitted& f : guns)
        if (!f.ok)
            missing += (missing.empty() ? "" : ", ") + std::string(f.gun->shape);
    for (const FittedSight& s : sights)
        if (!s.ok)
            missing += (missing.empty() ? "" : ", ") + std::string(s.sight->name);
    if (!missing.empty())
        h += "// PARTIAL (gunbodyfit --partial): without " + missing + ", not fitted.\n";
    h += "#include <cstdint>\n#include <cstring>\n#include \"gunbody.h\"\nnamespace gunbodies {\n";
    h += "enum Sound : int { kMetal = 0, kPolymer = 1, kHeavy = 2 };\n";
    h += "struct Body {\n";
    h += "    const char* shape;          // skin.cpp's HeldShape(), or \"smaw\" (its case on) / \"smaw_bare\"\n";
    h += "    const loose::Spec* spec;\n";
    h += "    float hullMin[3], hullMax[3];\n";
    h += "    int sound;\n";
    h += "    int restFrom;               // spec->hull from here on is the fit's \"the rest\" (the fit's step 8, bodyfit.h): an attachment's points go before it\n";
    h += "};\n";
    h += "// An attachment drawn apart from the gun (a sight, a handle, an underbarrel): its draw, and at most 8 extreme points in its own bone 0's space.\n";
    h += "struct Attach {\n    uint32_t count, start;\n    int base;\n    int points;\n    float point[8][3];\n};\n";
    for (const Fitted& f : guns) {
        if (!f.ok)
            continue;
        const Gun& g = *f.gun;
        const bodyfit::Result& r = f.r;
        std::string out;
        for (int i = 0; i < r.leftOutCount; ++i)
            out += Format("%s%d", i ? ", " : "", r.leftOut[i]);
        h += "\n// " + std::string(g.name) + ": " + Format("%d@%d", g.count, g.start) + Format(", frame bone %d, core bone %d (%d of %d vertices)", g.frame, r.coreBone,
                                                                                               r.coreVertices, f.vertices);
        h += r.leftOutCount ? ", left out: bone " + out : std::string(", nothing left out");
        const std::string keel = r.footprint < g_options.standWidth ? Format("its keel at x %.3f", Tidy(r.keelX)) : std::string("no keel (it may stand on it)");
        h += Format(";\n// kept %d vertices; its middle (%.3f %.3f %.3f), its footprint %.1f cm wide, %s; %d hull points, the rest from %d.\n", r.keptVertices,
                    Tidy(r.centre[0]), Tidy(r.centre[1]), Tidy(r.centre[2]), r.footprint * 100.0f, keel.c_str(), r.hullPoints, r.restFrom);
        for (const std::string& n : f.notes)
            h += "// " + n + ".\n";
        h += Format("inline constexpr float k%sHull[%d][3] = {\n", g.ident, r.hullPoints);
        for (int i = 0; i < r.hullPoints; ++i)
            h += "    " + Point(r.hull[i]) + ",  // " + bodyfit::WhatName(r.what[i]) + "\n";
        h += "};\n";
        h += Format("inline constexpr loose::Spec k%s{.name = \"%sgun\", .boxMin = %s, .boxMax = %s, .sweep = 0.0f, .sag = 0.0f,\n", g.ident,
                    std::strncmp(g.shape, "smaw", 4) == 0 ? "smaw" : g.shape, Point(r.boxMin).c_str(), Point(r.boxMax).c_str());
        h += Format("    .massEmpty = %s, .massRound = %s, .capacity = %d, .bounce = %s, .keep = true, .hull = k%sHull, .hullPoints = %d};\n", Num(g.mass).c_str(),
                    Num(g.round).c_str(), g.capacity, Num(g.bounce).c_str(), g.ident, r.hullPoints);
    }
    h += "\ninline constexpr Body kBodies[] = {\n";
    for (const Fitted& f : guns)
        if (f.ok)
            h += Format("    {\"%s\", &k%s, %s, %s, %s, %d},\n", f.gun->shape, f.gun->ident, Point(f.r.hullMin).c_str(), Point(f.r.hullMax).c_str(),
                        kSoundNames[f.gun->sound], f.r.restFrom);
    h += "};\n";
    h += "\n// The sights drawn apart (skin.cpp's kHeldSights), each with the gun it came on: the furthest of its vertices toward its box's eight corners.\n";
    h += "inline constexpr Attach kAttach[] = {\n";
    int attached = 0;
    for (const FittedSight& s : sights) {
        if (!s.ok)
            continue;
        h += Format("    {%d, %d, %d, %d,  // %s\n     {", s.sight->count, s.sight->start, s.sight->base, s.points, s.sight->name);
        for (int i = 0; i < s.points; ++i)
            h += (i == 0 ? "" : i % 4 == 0 ? ",\n      " : ", ") + Point(s.point[i]);
        h += "}},\n";
        ++attached;
    }
    if (!attached)
        h += "    {0, 0, 0, 0, {}},\n";
    h += "};\n";
    h += "\n// The M416 without its M320 (a later level's): gunbody::kM416 less the M320's five hull points (its tube's four, its grip's bottom), 3.9 kg (the\n";
    h += "// M416 3.5, the ACOG 0.4); the same boxes. Named \"m416gun\" too, so its lines are the M416's.\n";
    constexpr int kBarePoints = 22 - static_cast<int>(std::size(kM416BareDrop));
    h += Format("inline constexpr float kM416BareHull[%d][3] = {\n", kBarePoints);
    for (int i = 0; i < 22; ++i)
        if (std::find(std::begin(kM416BareDrop), std::end(kM416BareDrop), i) == std::end(kM416BareDrop))
            h += Format("    {gunbody::kM416Hull[%d][0], gunbody::kM416Hull[%d][1], gunbody::kM416Hull[%d][2]},\n", i, i, i);
    h += "};\n";
    h += "inline constexpr loose::Spec kM416Bare{.name = gunbody::kM416.name,\n";
    h += "                                       .boxMin = {gunbody::kM416.boxMin[0], gunbody::kM416.boxMin[1], gunbody::kM416.boxMin[2]},\n";
    h += "                                       .boxMax = {gunbody::kM416.boxMax[0], gunbody::kM416.boxMax[1], gunbody::kM416.boxMax[2]},\n";
    h += "                                       .sweep = gunbody::kM416.sweep, .sag = gunbody::kM416.sag, .massEmpty = " + Num(kM416BareMass) + ",\n";
    h += "                                       .massRound = gunbody::kM416.massRound, .capacity = gunbody::kM416.capacity, .bounce = gunbody::kM416.bounce,\n";
    h += Format("                                       .keep = true, .hull = kM416BareHull, .hullPoints = %d};\n", kBarePoints);
    h += "\ninline const Body* For(const char* shape) {\n    if (!shape)\n        return nullptr;\n    for (const Body& b : kBodies)\n";
    h += "        if (b.spec && std::strcmp(b.shape, shape) == 0)\n            return &b;\n    return nullptr;\n}\n";
    h += "inline const Attach* AttachFor(uint32_t count, uint32_t start, int base) {\n    for (const Attach& a : kAttach)\n";
    h += "        if (a.points > 0 && a.count == count && a.start == start && a.base == base)\n            return &a;\n    return nullptr;\n}\n";
    h += "}\n";
    return h;
}

// The numbers in a header's code (its comments left out), in order.
std::vector<double> Numbers(const std::string& text) {
    std::vector<double> out;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        const size_t comment = line.find("//");
        if (comment != std::string::npos)
            line.erase(comment);
        for (size_t i = 0; i < line.size();) {
            const char c = line[i];
            const bool start = std::isdigit(static_cast<unsigned char>(c)) || ((c == '-' || c == '.') && i + 1 < line.size() && (std::isdigit(static_cast<unsigned char>(line[i + 1])) || line[i + 1] == '.'));
            const bool inName = i > 0 && (std::isalnum(static_cast<unsigned char>(line[i - 1])) || line[i - 1] == '_');
            if (start && !inName) {
                char* end = nullptr;
                const double v = std::strtod(line.c_str() + i, &end);
                const size_t used = static_cast<size_t>(end - (line.c_str() + i));
                if (used > 0) {
                    out.push_back(v);
                    i += used;
                    continue;
                }
            }
            ++i;
        }
    }
    return out;
}

// ---------------------------------------------------------------- bodytest's points

// "BTP3", the bodies' count, then each body's shape (16 bytes), its points' count, its footprint, the hull box and the middle it was fitted with
// (the header's hullMin, hullMax and its Spec's box's middle: its kept mesh's, which bodytest ties the points to the header by), and its points (xyz).
bool WritePoints(const std::string& path, const std::vector<Fitted>& guns) {
    FILE* f = nullptr;
    if (fopen_s(&f, path.c_str(), "wb") || !f)
        return false;
    int bodies = 0;
    for (const Fitted& g : guns)
        bodies += g.ok;
    std::fwrite("BTP3", 1, 4, f);
    std::fwrite(&bodies, sizeof(int), 1, f);
    for (const Fitted& g : guns) {
        if (!g.ok)
            continue;
        char shape[16] = {};
        std::strncpy(shape, g.gun->shape, sizeof(shape) - 1);
        const int points = static_cast<int>(g.outer.size() / 3);
        std::fwrite(shape, 1, sizeof(shape), f);
        std::fwrite(&points, sizeof(int), 1, f);
        std::fwrite(&g.r.footprint, sizeof(float), 1, f);
        std::fwrite(g.r.hullMin, sizeof(float), 3, f);
        std::fwrite(g.r.hullMax, sizeof(float), 3, f);
        std::fwrite(g.r.centre, sizeof(float), 3, f);
        std::fwrite(g.outer.data(), sizeof(float), g.outer.size(), f);
    }
    const bool written = !std::ferror(f);
    return std::fclose(f) == 0 && written;
}

// ---------------------------------------------------------------- pictures

void Mark(meshdump::Image& img, int x, int y, int size, const float* colour, int tx) {
    for (int j = -size; j <= size; ++j)
        for (int i = -size; i <= size; ++i) {
            const int px = x + i, py = y + j;
            if (px < tx || px >= tx + meshdump::kTile || py < 0 || py >= img.h)
                continue;
            for (int k = 0; k < 3; ++k)
                img.rgb[(static_cast<size_t>(py) * img.w + px) * 3 + k] = colour[k];
        }
}
void Picture(const std::string& path, const Fitted& f) {
    using namespace meshdump;
    constexpr int kViews = 4;
    Image img(kTile * kViews, kTile);
    const View views[kViews] = {MakeView(-1, 0, 0, 0, 1, 0), MakeView(1, 0, 0, 0, 1, 0), MakeView(0, 1, 0, -1, 0, 0), MakeView(0, 0, 1, 0, 1, 0)};
    const bodyfit::Result& r = f.r;
    const float mid[3] = {0.5f * (r.hullMin[0] + r.hullMax[0]), 0.5f * (r.hullMin[1] + r.hullMax[1]), 0.5f * (r.hullMin[2] + r.hullMax[2])};
    constexpr float kMesh[3] = {0.55f, 0.6f, 0.68f}, kOut[3] = {0.35f, 0.25f, 0.25f};
    // By bodyfit::What: the keel's red, the ends yellow, the bottom's wide points orange, the side band green, the top blue, the gap points cyan,
    // the guards (a bottom vertex held above the keel) pink, a face it rests on green.
    const float kinds[][3] = {{1.0f, 0.15f, 0.1f}, {1.0f, 0.15f, 0.1f}, {1.0f, 0.15f, 0.1f}, {1.0f, 0.9f, 0.1f}, {1.0f, 0.9f, 0.1f},
                              {1.0f, 0.55f, 0.0f}, {0.2f, 0.95f, 0.2f}, {0.2f, 0.5f, 1.0f}, {0.1f, 0.9f, 0.9f}, {1.0f, 0.55f, 0.75f},
                              {0.9f, 0.3f, 0.9f}, {0.2f, 0.95f, 0.2f}};
    constexpr float kLowest[3] = {1.0f, 0.2f, 1.0f}, kMiddle[3] = {1.0f, 1.0f, 1.0f}, kKeel[3] = {1.0f, 0.15f, 0.1f};
    // The keel: its points (those at its x) one after another along z.
    std::vector<int> keel;
    for (int i = 0; i < r.hullPoints; ++i)
        if (r.hull[i][0] == r.keelX)
            keel.push_back(i);
    std::sort(keel.begin(), keel.end(), [&](int a, int b) { return r.hull[a][2] < r.hull[b][2]; });
    auto leftOut = [&](int b) {
        for (int i = 0; i < r.leftOutCount; ++i)
            if (r.leftOut[i] == b)
                return true;
        return false;
    };
    for (int v = 0; v < kViews; ++v) {
        // The scale that fits this side's extent in the tile.
        float extent = 0.0f;
        for (int c = 0; c < 8; ++c) {
            const float p[3] = {c & 1 ? r.hullMax[0] : r.hullMin[0], c & 2 ? r.hullMax[1] : r.hullMin[1], c & 4 ? r.hullMax[2] : r.hullMin[2]};
            const float d[3] = {p[0] - mid[0], p[1] - mid[1], p[2] - mid[2]};
            extent = std::max({extent, std::fabs(Dot(d, views[v].right)), std::fabs(Dot(d, views[v].up))});
        }
        const float scale = 180.0f / std::max(extent, 0.01f);
        for (size_t t = 0; t + 2 < f.mesh.tris.size(); t += 3) {
            const int a = f.mesh.tris[t], b = f.mesh.tris[t + 1], c = f.mesh.tris[t + 2];
            const bool out = leftOut(f.mesh.bone[a]) || leftOut(f.mesh.bone[b]) || leftOut(f.mesh.bone[c]);
            if (out && v != 0)
                continue;
            DrawTriangle(img, v * kTile, 0, views[v], mid, scale, &f.mesh.pos[static_cast<size_t>(a) * 3], &f.mesh.pos[static_cast<size_t>(b) * 3],
                         &f.mesh.pos[static_cast<size_t>(c) * 3], out ? kOut : kMesh);
        }
        auto at = [&](const float* p, int& x, int& y) {
            const float d[3] = {p[0] - mid[0], p[1] - mid[1], p[2] - mid[2]};
            x = v * kTile + kTile / 2 + static_cast<int>(std::lround(Dot(d, views[v].right) * scale));
            y = kTile / 2 - static_cast<int>(std::lround(Dot(d, views[v].up) * scale));
        };
        for (size_t i = 0; i + 1 < keel.size(); ++i) {
            const float* back = r.hull[keel[i]];
            const float* front = r.hull[keel[i + 1]];
            for (int s = 0; s <= 200; ++s) {
                const float k = s / 200.0f, p[3] = {back[0] + (front[0] - back[0]) * k, back[1] + (front[1] - back[1]) * k, back[2] + (front[2] - back[2]) * k};
                int x, y;
                at(p, x, y);
                Mark(img, x, y, 0, kKeel, v * kTile);
            }
        }
        for (int i = 0; i < r.hullPoints; ++i) {
            int x, y;
            at(r.hull[i], x, y);
            Mark(img, x, y, 3, kinds[std::clamp(r.what[i], 0, 11)], v * kTile);
        }
        int x, y;
        at(f.lowest, x, y);
        Mark(img, x, y, 2, kLowest, v * kTile);
        at(r.centre, x, y);
        Mark(img, x - 5, y, 0, kMiddle, v * kTile), Mark(img, x + 5, y, 0, kMiddle, v * kTile);
        for (int s = -5; s <= 5; ++s)
            Mark(img, x + s, y, 0, kMiddle, v * kTile), Mark(img, x, y + s, 0, kMiddle, v * kTile);
    }
    img.Save(path.c_str());
}

// ---------------------------------------------------------------- the report

void Report(const Fitted& f) {
    const Gun& g = *f.gun;
    if (!f.ok) {
        std::printf("%s (%s): NOT FITTED: %s\n", g.shape, g.name, f.why.c_str());
        return;
    }
    const bodyfit::Result& r = f.r;
    std::printf("%s (%s): %d@%d, frame bone %d (derived %d: %s), core bone %d (%d of %d vertices; expected %d: %s)", g.shape, g.name, g.count, g.start,
                g.frame, f.derivedFrame, f.derivedFrame == g.frame || std::strncmp(g.shape, "smaw", 4) == 0 ? "PASS" : "FAIL", r.coreBone, r.coreVertices,
                f.vertices, g.expectCore, r.coreBone == g.expectCore ? "PASS" : "FAIL");
    if (r.leftOutCount) {
        std::printf(", left out: bone");
        for (int i = 0; i < r.leftOutCount; ++i)
            std::printf(" %d", r.leftOut[i]);
    }
    std::printf("\n");
    for (const std::string& n : f.notes)
        std::printf("  %s\n", n.c_str());
    std::printf("  core box (%.3f %.3f %.3f)..(%.3f %.3f %.3f); hull box (%.3f %.3f %.3f)..(%.3f %.3f %.3f)\n", r.coreMin[0], r.coreMin[1], r.coreMin[2],
                r.coreMax[0], r.coreMax[1], r.coreMax[2], r.hullMin[0], r.hullMin[1], r.hullMin[2], r.hullMax[0], r.hullMax[1], r.hullMax[2]);
    if (f.researchOff >= 0.0f)
        std::printf("  against the research's box (what moves and its parts drawn apart out): at most %.1f mm off (%s)\n", f.researchOff, f.researchOff <= 5.0f ? "within 5 mm" : "MORE than 5 mm");
    std::printf("  middle (%.3f %.3f %.3f), keel x %.3f (the middle %.1f mm beside it), footprint %.1f cm wide, lowest vertex (%.3f %.3f %.3f); %d kept vertices\n",
                r.centre[0], r.centre[1], r.centre[2], r.keelX, std::fabs(r.centre[0] - r.keelX) * 1000.0f, r.footprint * 100.0f, f.lowest[0], f.lowest[1],
                f.lowest[2], r.keptVertices);
    std::printf("  the fit's own gap: %.1f mm; it comes to rest %d ways:", r.gap * 1000.0f, r.rests);
    for (int i = 0; i < r.rests; ++i)
        std::printf(" (%.2f %.2f %.2f)", r.rest[i][0], r.rest[i][1], r.rest[i][2]);
    std::printf("\n");
    std::printf("  its mesh past its hull: at most %.1f mm (lying with (%.2f %.2f %.2f) down), %.1f mm not near upright (with (%.2f %.2f %.2f) down)\n",
                f.worst * 1000.0f, f.worstWay[0], f.worstWay[1], f.worstWay[2], f.lyingWorst * 1000.0f, f.lyingWay[0], f.lyingWay[1], f.lyingWay[2]);
    std::printf("  %d hull points (the rest from %d), %zu outermost vertices for bodytest:\n", r.hullPoints, r.restFrom, f.outer.size() / 3);
    for (int i = 0; i < r.hullPoints; ++i)
        std::printf("    %2d (%7.4f %7.4f %7.4f) %s\n", i, r.hull[i][0], r.hull[i][1], r.hull[i][2], bodyfit::WhatName(r.what[i]));
}

// The M416's own dump against gunbody.h's hand-made body (information only: it's never refitted).
void M416Check(const std::string& dir) {
    meshdump::Mesh gun;
    if (!meshdump::LoadGun(dir, gun)) {
        std::printf("the M416: its dump (gun_*.bin) isn't there\n");
        return;
    }
    std::vector<int> bone(gun.part.begin(), gun.part.end());
    const bodyfit::Result r = bodyfit::Fit(gun.pos.data(), bone.data(), static_cast<int>(bone.size()), gun.tris.data(), static_cast<int>(gun.tris.size() / 3));
    if (!r.ok) {
        std::printf("the M416: the fit refused its dump: %s\n", r.why);
        return;
    }
    const loose::Spec& m = gunbody::kM416;
    std::printf("the M416 (information only, gunbody.h is kept): its dump's core box (%.3f %.3f %.3f)..(%.3f %.3f %.3f) and kept box (%.3f %.3f %.3f)..(%.3f %.3f %.3f);\n"
                "  gunbody.h's box (%.3f %.3f %.3f)..(%.3f %.3f %.3f), its hull's (%.3f %.3f %.3f)..(%.3f %.3f %.3f); the dump's middle (%.3f %.3f %.3f)\n",
                r.coreMin[0], r.coreMin[1], r.coreMin[2], r.coreMax[0], r.coreMax[1], r.coreMax[2], r.hullMin[0], r.hullMin[1], r.hullMin[2], r.hullMax[0],
                r.hullMax[1], r.hullMax[2], m.boxMin[0], m.boxMin[1], m.boxMin[2], m.boxMax[0], m.boxMax[1], m.boxMax[2], gunbody::kM416HullMin[0],
                gunbody::kM416HullMin[1], gunbody::kM416HullMin[2], gunbody::kM416HullMax[0], gunbody::kM416HullMax[1], gunbody::kM416HullMax[2], r.centre[0],
                r.centre[1], r.centre[2]);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: gunbodyfit <build folder> <out gunbodies.h> [--check] [--partial] [--pictures <folder>]\n");
        return 1000;
    }
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    const std::string dir = argv[1], header = argv[2];
    bool check = false, partial = false;
    std::string pictures, points = dir + "\\bodytest_points.bin";
    for (int i = 3; i < argc; ++i) {
        if (std::strcmp(argv[i], "--check") == 0)
            check = true;
        else if (std::strcmp(argv[i], "--partial") == 0)  // (the header and the points without what's missing, the header says so)
            partial = true;
        else if (std::strcmp(argv[i], "--pictures") == 0 && i + 1 < argc)
            pictures = argv[++i];
        else if (std::strcmp(argv[i], "--points") == 0 && i + 1 < argc)  // (bodytest's points elsewhere: a trial)
            points = argv[++i];
        else if (std::strcmp(argv[i], "--band") == 0)
            g_options.band = true;
        else if (std::strcmp(argv[i], "--rest") == 0 && i + 2 < argc)  // (a trial: its rest ways' weight and cone)
            g_options.restWeight = static_cast<float>(std::atof(argv[i + 1])), g_options.restCone = static_cast<float>(std::atof(argv[i + 2])), i += 2;
        else if (std::strcmp(argv[i], "--faces") == 0 && i + 1 < argc)  // (a trial: how many points a wide bottom's rest faces get)
            g_options.restFacePoints = std::atoi(argv[++i]);
    }
    std::vector<Fitted> guns;
    int failed = 0;
    for (const Gun& g : kGuns) {
        guns.push_back(FitGun(dir, g));
        Report(guns.back());
        const Fitted& f = guns.back();
        failed += !f.ok || f.partsFailed > 0 || f.r.coreBone != g.expectCore || (f.derivedFrame != g.frame && std::strncmp(g.shape, "smaw", 4) != 0);
    }
    std::vector<FittedSight> sights;
    for (const Sight& s : kSights) {
        sights.push_back(FitSight(dir, s));
        const FittedSight& f = sights.back();
        if (!f.ok) {
            std::printf("%s (%d@%d): NOT FITTED: %s\n", s.name, s.count, s.start, f.why.c_str());
            ++failed;
            continue;
        }
        std::printf("%s (%d@%d): its box (%.3f %.3f %.3f)..(%.3f %.3f %.3f), %d points:", s.name, s.count, s.start, f.lo[0], f.lo[1], f.lo[2], f.hi[0], f.hi[1],
                    f.hi[2], f.points);
        for (int i = 0; i < f.points; ++i)
            std::printf(" (%.3f %.3f %.3f)", f.point[i][0], f.point[i][1], f.point[i][2]);
        std::printf("\n");
    }
    // What's still to dump: the parts awaited (each has its row, added once its dump is here), and the carry handle (no row yet: its draw's numbers
    // aren't known).
    std::string awaited;
    for (const Fitted& f : guns)
        for (const std::string& a : f.awaited)
            awaited += std::string(awaited.empty() ? "" : "; ") + "the " + f.gun->name + "'s " + a;
    std::printf("not dumped yet (weapon.txt \"dump N S\" holding the gun, then fit again): %s%sthe M16A4's and M4A1's carry handle (m16_ironsight_1p_Mesh: a "
                "sight drawn apart; its draw's numbers aren't known yet: it needs a row in kSights as well)\n",
                awaited.c_str(), awaited.empty() ? "" : "; ");
    if (failed)
        std::printf("%d of the guns and sights not fitted, a part of theirs not added, or their frame or core bone not the table's\n", failed);
    const std::string text = Header(guns, sights);
    if (check) {
        M416Check(dir);
        std::ifstream in(header, std::ios::binary);
        if (!in) {
            std::printf("--check: %s isn't there\n", header.c_str());
            return 1000;
        }
        const std::string disk{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
        const std::vector<double> want = Numbers(text), have = Numbers(disk);
        int off = static_cast<int>(want.size() > have.size() ? want.size() - have.size() : have.size() - want.size());
        for (size_t i = 0; i < std::min(want.size(), have.size()); ++i)
            off += std::fabs(want[i] - have[i]) > 0.001;
        off += failed;
        std::printf("--check: %s against the meshes: %d numbers (of %zu) off by more than 1 mm\n", header.c_str(), off, want.size());
        return off;
    }
    // Anything missing: nothing written (a header without a gun leaves that gun without a body in the game), unless --partial asks for it.
    if (failed && !partial) {
        std::printf("nothing written: %s and %s are as they were (--partial writes them without what's missing)\n", header.c_str(), points.c_str());
    } else {
        {
            std::ofstream outFile(header, std::ios::binary);
            outFile << text;
            if (!outFile) {
                std::printf("couldn't write %s\n", header.c_str());
                return 1000;
            }
        }
        std::printf("wrote %s%s\n", header.c_str(), failed ? " (partial: without what's missing)" : "");
        if (!WritePoints(points, guns)) {
            std::printf("couldn't write %s\n", points.c_str());
            return 1000;
        }
        std::printf("wrote %s\n", points.c_str());
    }
    if (!pictures.empty()) {
        std::error_code made;
        std::filesystem::create_directories(pictures, made);
        for (const Fitted& f : guns)
            if (f.ok) {
                const std::string path = pictures + "\\body_" + f.gun->shape + ".bmp";
                Picture(path, f);
                std::printf(std::filesystem::exists(path) ? "pictured %s\n" : "couldn't picture %s\n", path.c_str());
            }
    }
    return failed;
}

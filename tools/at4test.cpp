// The AT4 (Operation Swordbreaker's M136) without the game: skin.cpp's row for it (copied here, as the other tests keep theirs) against its mesh dump
// and its recording (build\mesh_15987_0_*, build\survey\AT4_fire.bin), the tables written for it, the sights' turn, a hand's use of a scripted
// prompt (src/usepoint.h), the quick-time guard, its holster's place, the game's use's code against build\bf3_mem.exe, the SMAW's rocket's start
// told from the AT4's and the RPG-7s', and the take's side effects on the held magazine.
//   t1  the tables: its row in each (recoilguns.h: its body draw, shape and kind, its blueprint's name, the SMAW's buzz when its data has none;
//       gunbodies.h: its body, a polymer knock; shaperecipes.h: frame bone 0, its lever, safety and front grip left out; gunprofile.h: row 15, its
//       front grip, its sights' right carried); skin.cpp's row is this one, word for word
//   t2  the mesh (SKIP without it): its four bones (the frame; the lever on top at its right, z 0.436-0.468; the safety at its right, z 0.545-0.558;
//       the front grip turned down under the tube, z 0.871-0.885), nothing of it past its muzzle; the tube's front ring's middle the row's muzzle
//   t3  its sights (SKIP without the mesh): the rear aperture a ring in the leaf (its radius 4-6 mm, every point of it within 0.3 mm of it), the
//       front post's tip; their line's up and right the row's within 0.02 degrees; the row's sights point on that line at the muzzle within 0.5 mm;
//       the game's own zoomed eye (the draw scan's) on it within 1 mm and looking along it within 0.15 degrees (its one sample's sway)
//   t4  the recording (SKIP without it): the game's palms at rest the row's within 1 mm (the right at the firing mechanism, the left on the front grip)
//   t5  the sights' turn (gunprofile::SightsTurn, as hands.cpp places a gun): the measured line turned onto the aim within 1e-4 rad; right 0 the raise
//       as built before it (a frozen copy, out of line), bit for bit, for every up from -1 to 1 degree and the guns' own
//   t6  a hand's use of a prompt (usepoint.h): which prompts a hand uses (the AT4's and the doors', never the pose prompts, holds, 300 m, jumps,
//       flags); the reach (the disc, under and over, widening up, the capsule, the AT4's own 1 m disc); the tick once on entry, not again at its
//       edge; a fresh squeeze held 0.25 s asks once, never again until let go; one carried into reach never; out of reach the hold starts again; a
//       gun in the other hand no bar at a door; the AT4's prompt (it gives a gun) with a gun in a hand: no tick, refused with the double click, the
//       squeeze still the use's, the tick once the gun is let go of, the bar as the hold ends, a refusal made outside Update (X held's); a squeeze
//       something else wants at its start never the use's; one that takes something else first given up; not targeted, asleep, not tracked, a
//       hand holding something: nothing; both hands at once
//   t7  the quick-time guard (qtewatch.h's Bar, one rule for the live triggers and a fist fight's prompt): Fire 0x07 awaited in a scripted moment
//       with a launcher in a hand: barred, never awaited; the quick-time Fire alike; E still answered; no launcher in a hand: as before, bit for bit;
//       a fight's prompt for the quick-time Fire with a launcher: the grips' press barred, its answer kept, so the fights' guard lets the trigger's
//       left button through and holds up every other key
//   t8  its holster (holster.h): the AT4 in slot 0 with the Mk11 in slot 1 (the scene's kit on Swordbreaker): behind the neck, the Mk11 at the left
//       shoulder; with no secondary: the left shoulder
//   t9  the game's use (SKIP without build\bf3_mem.exe): every row pickups.cpp checks it by (its code's FNV-1a 64, its bytes, its words) as the image
//       has them, and each row word for word in src/pickups.cpp
//   t10 its rocket's start (src/at4shot.h, SKIP without build\survey\AT4R_R10_scan.txt): the roof's dump of its FiringFunctionData is its own; the
//       same with the SMAW's, the RPG-7's or the enemies' RPG-7's numbers, or another class, never; its start as last written there, never another's
//   t10b the SMAW's alike (skin.cpp's ClearLauncherSpread, its writer and its putting back): the roof's two exact matches the AT4's and the
//       enemies' RPG-7's by their GUIDs, both taken by the scan before by their shot alone, never the SMAW's by its numbers; the SMAW's own numbers
//       are, its start as last written, never another's; which matches are the SMAW's own (at4shot::SmawOwn, ten cases: by a weapon of the
//       soldier's whatever its numbers, else Venice-EBX's numbers, else none of the known others'; never the AT4's or an RPG-7's, one unread or of
//       another class), each held to its own numbers after; skin.cpp's lines word for word (the frame stored never ahead)
//   t11 the take's side effects (src/pickrules.h's OtherGun and SeeDry, as hands.cpp's held magazine runs them): the roof's take frame by frame
//       (the AT4's one round under the M4A1 never its run dry; the kit's fresh M4A1 a new gun that fires); each swap a frame ahead of the slot's
//       name (waited for, never a new M4A1 with one round), a name never read with it (a second, then as before); the same gun as before; a gun
//       of its kind taken up with one round; OtherGun's cases; hands.cpp's lines word for word (recoilrun's check of the slot's gun by its name is
//       recoilruntest's 22)
//   t12 merged with holsters by hand, the owner's flow (b) (src/pickrules.h's LyingTaken and LyingBack, as hands.cpp's OnGunChanged runs them): the
//       lying M4A1 kept when the roof's gift takes its slot (never with no gift, nothing lying, a pickup's take, the same gun, another slot); the
//       kit's M4A1 back lies there again, through a death on its level too (the AT4, a new life's kit of it, or a gun between: waits; a take, a
//       pick, another slot, too far or not known, a new life's own kit of it, another level's kit, too long: ends, each said why); hands.cpp's and
//       input.cpp's lines word for word: the record, the use barred with a gun in a hand, X held's and X's tap's R refused so (holstertest HT1-HT8
//       run the flows whole with holsters)
// Usage: at4test [<folder with the mesh dumps and bf3_mem.exe> [<folder with the recordings>]] (build and build\survey by default, from the project's
// folder). Reads only.

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <string>
#include <vector>

#include "../src/at4shot.h"
#include "../src/gunbodies.h"
#include "../src/gunprofile.h"
#include "../src/holster.h"
#include "../src/pickrules.h"
#include "../src/qtefight.h"
#include "../src/qtelist.h"
#include "../src/qtewatch.h"
#include "../src/recoil.h"
#include "../src/recoilguns.h"
#include "../src/shaperecipes.h"
#include "../src/usepoint.h"
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

// ---- skin.cpp's row for it, copied (kHeldKinds' at4) ----
constexpr int kBody = 15987, kFrameBone = 0;
constexpr float kUp = 0.47f, kRight = 0.68f;
constexpr float kSights[3] = {0.0556f, 0.1248f, 0.995f}, kMuzzle[3] = {-0.001f, 0.053f, 0.995f};
constexpr float kRightPalm[3] = {-0.091f, 0.023f, 0.409f}, kLeftPalm[3] = {0.037f, -0.047f, 0.883f};
constexpr const char* kSkinRow = "{15987, 0, 0.47f, {0.0556f, 0.1248f, 0.995f}, {-0.001f, 0.053f, 0.995f}, {0.0f, 0.053f, 0.004f}, {-0.091f, 0.023f, 0.409f}, "
                                 "{0.037f, -0.047f, 0.883f}, nullptr, 15987, kAt4Gun, \"at4\", 0, -1, 0, 0.68f},";
// What the game said zoomed (build\survey\AT4_zoom.txt's draw scan): the view in its bone 0.
constexpr float kZoomedEye[3] = {0.061f, 0.129f, 0.491f}, kZoomedForward[3] = {-0.0121f, -0.0081f, 0.9999f};
// The gloves' draw and the palms' points in it (skin.cpp's kArmDraws[0]: kRightPalm on the right's root bone 37, kLeftPalm on the left's palm bone 1).
constexpr int kGloves[2] = {13755, 15051};
constexpr int kRightRoot = 37, kLeftPalmBone = 1;
constexpr float kRightPalmPoint[3] = {-0.505f, 1.129f, 0.150f}, kLeftPalmPoint[3] = {0.537f, 1.098f, 0.207f};

constexpr float kPi = 3.14159265f;

std::vector<uint8_t> ReadAll(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
std::string ReadText(const std::string& path) {
    const std::vector<uint8_t> b = ReadAll(path);
    return std::string(b.begin(), b.end());
}
float Dist(const float* a, const float* b) {
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}
void Apply(const float* m, const float* p, float* o) {
    for (int r = 0; r < 3; ++r)
        o[r] = m[r * 4] * p[0] + m[r * 4 + 1] * p[1] + m[r * 4 + 2] * p[2] + m[r * 4 + 3];
}
void InvertRigid(const float* m, float* o) {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            o[i * 4 + j] = m[j * 4 + i];
    for (int i = 0; i < 3; ++i)
        o[i * 4 + 3] = -(o[i * 4] * m[3] + o[i * 4 + 1] * m[7] + o[i * 4 + 2] * m[11]);
}
void Compose(const float* a, const float* b, float* o) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j)
            o[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        o[i * 4 + 3] += a[i * 4 + 3];
    }
}
uint64_t Fnv(const uint8_t* bytes, size_t size) {
    uint64_t h = 14695981039346656037ull;
    for (size_t i = 0; i < size; ++i)
        h ^= bytes[i], h *= 1099511628211ull;
    return h;
}

// A recording (weapon.txt "allrec"): records of 8 ints (the draw's count, the frame's serial, .., its start, .., .., ms, bones) and that many bones,
// in the view's space of the eye the draw was for (mk11test's reader).
struct Record {
    int count = 0, start = 0, frame = 0;
    std::vector<float> bones;
};
std::vector<Record> Recording(const std::string& path) {
    std::vector<Record> out;
    FILE* f = nullptr;
    if (fopen_s(&f, path.c_str(), "rb") || !f)
        return out;
    int head[8];
    while (fread(head, 4, 8, f) == 8) {
        const int n = head[7];
        if (n <= 0 || n > 4096)
            break;
        Record r;
        r.bones.resize(static_cast<size_t>(n) * 12);
        if (fread(r.bones.data(), 48, n, f) != static_cast<size_t>(n))
            break;
        r.count = head[0], r.start = head[3], r.frame = head[1];
        out.push_back(std::move(r));
    }
    fclose(f);
    return out;
}

// The image's bytes at a VA (bf3_mem.exe: loaded at 0x400000, not relocated), through its section headers; false outside them.
struct Image {
    std::vector<uint8_t> file;
    struct Section {
        uint32_t va, size, raw, rawSize;
    };
    std::vector<Section> sections;
    bool Load(const std::string& path) {
        file = ReadAll(path);
        if (file.size() < 0x400 || file[0] != 'M' || file[1] != 'Z')
            return false;
        uint32_t nt = 0;
        std::memcpy(&nt, &file[0x3C], 4);
        if (nt + 0x18 + 0xE0 > file.size() || std::memcmp(&file[nt], "PE\0\0", 4) != 0)
            return false;
        uint16_t count = 0, optional = 0;
        std::memcpy(&count, &file[nt + 6], 2);
        std::memcpy(&optional, &file[nt + 0x14], 2);
        const size_t table = nt + 0x18 + optional;
        for (int i = 0; i < count && table + (i + 1) * 40 <= file.size(); ++i) {
            const uint8_t* s = &file[table + i * 40];
            Section sec{};
            uint32_t virtualSize = 0;
            std::memcpy(&virtualSize, s + 8, 4);
            std::memcpy(&sec.va, s + 12, 4);
            std::memcpy(&sec.rawSize, s + 16, 4);
            std::memcpy(&sec.raw, s + 20, 4);
            sec.size = std::max(virtualSize, sec.rawSize);
            sections.push_back(sec);
        }
        return !sections.empty();
    }
    bool Read(uint32_t va, void* out, size_t size) const {
        const uint32_t rva = va - 0x400000u;
        for (const Section& s : sections)
            if (rva >= s.va && rva + size <= s.va + s.rawSize && rva + size <= s.va + s.size) {
                const size_t at = s.raw + (rva - s.va);
                if (at + size > file.size())
                    return false;
                std::memcpy(out, &file[at], size);
                return true;
            }
        return false;
    }
};

// ---- pickups.cpp's rows for the game's use (kUseCode, kUseBytes, kUseWords), copied ----
struct CodeRow {
    uint32_t at, size;
    uint64_t fnv;
    const char* text;  // the row as pickups.cpp writes it
};
const CodeRow kUseCode[] = {
    {0x008A9440, 631, 0xCECA9804C85D0826ull, "{0x008A9440, 631, 0xCECA9804C85D0826ull, "},
    {0x008A8630, 272, 0x638BADA8C36B571Bull, "{0x008A8630, 272, 0x638BADA8C36B571Bull, "},
    {0x008A1A90, 830, 0xBF4CF0439EA83E8Cull, "{0x008A1A90, 830, 0xBF4CF0439EA83E8Cull, "},
    {0x008A6CF0, 64, 0x8C811FB410477102ull, "{0x008A6CF0, 64, 0x8C811FB410477102ull, "},
    {0x011953E0, 68, 0x3930E7A150F5838Full, "{0x011953E0, 68, 0x3930E7A150F5838Full, "},
    {0x0118D130, 85, 0x8A8A26F1D3889C46ull, "{0x0118D130, 85, 0x8A8A26F1D3889C46ull, "},
    {0x00A64ED0, 91, 0x20509B1DC743BD00ull, "{0x00A64ED0, 91, 0x20509B1DC743BD00ull, "},
    {0x00545C90, 24, 0x2640302D80421EEEull, "{0x00545C90, 24, 0x2640302D80421EEEull, "},
    {0x007E3590, 43, 0x1214EB2E7AE082FBull, "{0x007E3590, 43, 0x1214EB2E7AE082FBull, "},
    {0x00832980, 156, 0x7F5E0058A9646484ull, "{0x00832980, 156, 0x7F5E0058A9646484ull, "},
    {0x0082FE50, 72, 0x520A42D75FE702D6ull, "{0x0082FE50, 72, 0x520A42D75FE702D6ull, "},
    {0x008912E0, 10, 0x04748D5ADEA66225ull, "{0x008912E0, 10, 0x04748D5ADEA66225ull, "},
    {0x00891390, 10, 0x69EAA6AEC920023Dull, "{0x00891390, 10, 0x69EAA6AEC920023Dull, "},
};
const uint8_t kUseTypeTest[] = {0x8B, 0x17, 0x8B, 0x02, 0x8B, 0xCF, 0xFF, 0xD0, 0x3D, 0xA8, 0x97, 0x3D, 0x02, 0x0F, 0x85, 0x5E, 0x02, 0x00, 0x00};
const uint8_t kUseKeyPath[] = {0x8B, 0xCF, 0xE8, 0x76, 0x20, 0xFE, 0xFF, 0xD9, 0xEE, 0xD9, 0xC9, 0x8B, 0xCF, 0xDF, 0xE9, 0xDD, 0xD8, 0x9F, 0xF6, 0xC4,
                               0x44, 0x7A, 0x18, 0xE8, 0x11, 0x82, 0xFD, 0xFF, 0x8B, 0x4D, 0x08, 0x50, 0xE8, 0x98, 0x22, 0x90, 0x00, 0x84, 0xC0, 0x74,
                               0x23, 0x8B, 0x56, 0x10, 0x52, 0xEB, 0x16, 0xE8, 0xF9, 0x81, 0xFD, 0xFF, 0x8B, 0x4D, 0x08, 0x50, 0xE8, 0x60, 0x22, 0x90,
                               0x00, 0x84, 0xC0, 0x74, 0x0B, 0x8B, 0x46, 0x10, 0x50, 0x8B, 0xCF, 0xE8, 0x41, 0x03, 0xFF, 0xFF};
const uint8_t kUseGetter[] = {0xB8, 0xA8, 0x97, 0x3D, 0x02, 0xC3};
const uint8_t kUseEnabledTest[] = {0xF6, 0x46, 0x0A, 0x01, 0x75, 0x2B};
struct BytesRow {
    uint32_t at;
    const uint8_t* bytes;
    uint32_t size;
};
const BytesRow kUseBytes[] = {{0x008B8EB3, kUseTypeTest, sizeof(kUseTypeTest)}, {0x008B90B3, kUseKeyPath, sizeof(kUseKeyPath)},
                              {0x00891220, kUseGetter, sizeof(kUseGetter)}, {0x008A944B, kUseEnabledTest, sizeof(kUseEnabledTest)}};
struct WordRow {
    uint32_t at, value, size;
    const char* text;
};
const WordRow kUseWords[] = {
    {0x020BC9F8 + 0x00, 0x00891220, 4, "{0x020BC9F8 + 0x00, 0x00891220, 4, "}, {0x020BC9F8 + 0x24, 0x01191130, 4, "{0x020BC9F8 + 0x24, 0x01191130, 4, "},
    {0x020BC9F8 + 0x78, 0x008A1A90, 4, "{0x020BC9F8 + 0x78, 0x008A1A90, 4, "}, {0x023D97A8 + 0x14, 0x02EB, 2, "{0x023D97A8 + 0x14, 0x02EB, 2, "},
    {0x023D97A8 + 0x16, 0x02EB, 2, "{0x023D97A8 + 0x16, 0x02EB, 2, "},       {0x021C6DD4, 0x01FE5550, 4, "{0x021C6DD4, 0x01FE5550, 4, "},
};

// The raise every place that turns a gun by its sights built before SightsTurn (hands.cpp 6e49f8c, three lines), frozen here and kept out of line so
// the comparison can't share the optimizer's sines and cosines with the code it checks.
__declspec(noinline) void ParentRaise(float upDegrees, float* out) {
    const float up = upDegrees * 3.14159265f / 180.0f;
    const float raise[9] = {1, 0, 0, 0, std::cos(up), std::sin(up), 0, -std::sin(up), std::cos(up)};
    std::memcpy(out, raise, sizeof(raise));
}

// ---- The roof's dump of the AT4's FiringFunctionData (build\survey\AT4R_R10_scan.txt's "== E8860980" rows: 64 dwords each, from 19:29:40) ----
// The dwords of the rows from base, as the scan wrote them ("== XXXXXXXX" then "XXXXXXXX: dword dword ..."), consecutive rows only; empty if not there.
std::vector<uint32_t> DumpFrom(const std::string& text, uint32_t base) {
    std::vector<uint32_t> out;
    uint32_t next = base;
    for (size_t at = 0; (at = text.find('\n', at)) != std::string::npos;) {
        ++at;
        unsigned row = 0;
        if (std::sscanf(text.c_str() + at, "%8X:", &row) != 1 || row != next)
            continue;
        const char* colon = std::strchr(text.c_str() + at, ':');
        if (!colon)
            break;
        const std::string line(colon + 1, std::strcspn(colon + 1, "\r\n"));  // (this row alone)
        for (const char* p = line.c_str();;) {
            unsigned word = 0;
            int used = 0;
            if (std::sscanf(p, " %8X%n", &word, &used) != 1)
                break;
            out.push_back(word);
            p += used;
        }
        next = base + static_cast<uint32_t>(out.size() * 4);
    }
    return out;
}

// ---- usepoint.h's frames ----
usepoint::Frame UseFrameAt(const float* point, const float* left, const float* right, float leftSqueeze, float rightSqueeze) {
    usepoint::Frame f;
    f.dt = 1.0f / 72.0f;
    f.point.on = true;
    std::memcpy(f.point.at, point, sizeof(f.point.at));
    const float* at[2] = {left, right};
    const float squeeze[2] = {leftSqueeze, rightSqueeze};
    for (int s = 0; s < 2; ++s) {
        f.hand[s].tracked = true;
        f.hand[s].empty = true;
        std::memcpy(f.hand[s].at, at[s], sizeof(f.hand[s].at));
        f.hand[s].squeeze = squeeze[s];
    }
    return f;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "build", survey = argc > 2 ? argv[2] : "build\\survey";
    char what[900];

    // ---- t1: the tables ----
    {
        const recoilguns::Row* row = recoilguns::ByName("Weapons/M136/M136");
        const bool known = row && row->body == static_cast<uint32_t>(kBody) && row->shape && !std::strcmp(row->shape, "at4") && row->kind == recoil::Kind::kAt4 &&
                           recoilguns::ByBody(kBody) == row && recoilguns::ByShape("at4") == row;
        std::snprintf(what, sizeof(what), "t1 its recoil row: \"%s\", its body draw %u, its shape %s, a launcher of its own kind; found by its blueprint, its body and its shape alike",
                      row ? row->name : "none", row ? row->body : 0, row && row->shape ? row->shape : "none");
        Check(known, what);
        if (row) {
            std::snprintf(what, sizeof(what), "t1 ... one rocket at %.0f m/s; its data's rumble %.1f %.1f for %.1f s (none: recoil.h's launcher's rumble, the SMAW's, %.1f)",
                          row->speed, row->rumble[0], row->rumble[1], row->rumble[2], recoil::kLauncherRumble);
            Check(row->magazine == 1 && row->speed == 40.0f && row->rumble[0] == 0.0f && row->rumble[1] == 0.0f && recoil::kLauncherRumble == 1.0f, what);
            // Its buzz once struck: the SMAW's (1.0 for 55 ms, its support hand 0.8 on its front grip), a lift's late pulse.
            recoil::Gun g;
            g.kind = row->kind, g.rate = row->rate;
            for (int i = 0; i < 3; ++i)
                g.rumble[i] = row->rumble[i];
            g.kick = row->kick;
            recoil::State s;
            recoil::Hold hold;
            hold.held = true, hold.hands = 1.0f, hold.frontGrip = true;
            recoil::Frame f;
            f.dt = 1.0f / 72.0f, f.shots = 1, f.trigger = true, f.seed = 7;
            const recoil::Output o = recoil::Step(g, hold, f, recoil::Tune{}, s);
            std::snprintf(what, sizeof(what), "t1 ... its blast's buzz: %.2f for %.0f ms, the front grip's hand %.2f, a late %.2f (the SMAW's: 1.00, 55, 0.80, 0.60)", o.pulse[0],
                          o.pulseFor[0] * 1000.0f, o.pulse[1], o.late);
            Check(std::fabs(o.pulse[0] - 1.0f) < 1e-4f && std::fabs(o.pulseFor[0] - 0.055f) < 1e-4f && std::fabs(o.pulse[1] - 0.8f) < 1e-4f && std::fabs(o.late - 0.6f) < 1e-4f, what);
        }
        const gunbodies::Body* b = gunbodies::For("at4");
        std::snprintf(what, sizeof(what), "t1 its body (gunbodies.h): %s, %d hull points, %.1f kg empty and %.1f a rocket, a polymer knock", b && b->spec ? b->spec->name : "none",
                      b && b->spec ? b->spec->hullPoints : 0, b && b->spec ? b->spec->massEmpty : 0.0f, b && b->spec ? b->spec->massRound : 0.0f);
        Check(b && b->spec && !std::strcmp(b->spec->name, "at4gun") && b->spec->hullPoints >= 4 && b->spec->keep && b->sound == gunbodies::kPolymer && b->spec->capacity == 1, what);
        const shaperecipes::Recipe* r = shaperecipes::Find("at4");
        std::snprintf(what, sizeof(what), "t1 its collision shape's recipe: frame bone %d, \"%s\" left out, %u indices from %u, %d palette bones", r ? r->frameBone : -1, r ? r->leaveOut : "",
                      r ? r->draws[0].count : 0, r ? r->draws[0].start : 0, r ? r->draws[0].bones : 0);
        Check(r && r->frameBone == kFrameBone && !std::strcmp(r->leaveOut, "1,2,3") && r->draws[0].count == static_cast<uint32_t>(kBody) && r->draws[0].start == 0 &&
                  r->draws[0].bones == 4 && r->drawCount == 1,
              what);
        gunprofile::In in;
        in.held = true, in.heldId = kBody, in.heldShape = "at4";
        std::memcpy(in.launcherGrip, kRightPalm, sizeof(kRightPalm));
        std::memcpy(in.launcherFront, kLeftPalm, sizeof(kLeftPalm));
        in.launcherUp = kUp, in.launcherRight = kRight;
        in.body = b ? b->spec : nullptr;
        if (b)
            std::memcpy(in.bodyMin, b->hullMin, sizeof(in.bodyMin)), std::memcpy(in.bodyMax, b->hullMax, sizeof(in.bodyMax));
        in.bodySound = gunbodies::kPolymer;
        const gunprofile::Profile p = gunprofile::Make(in);
        std::snprintf(what, sizeof(what), "t1 its profile: row %d, \"%s\", by its %s, up %.2f right %.2f, slot %d", gunprofile::RowOf("at4"), p.name, p.front, p.up, p.right, p.slot);
        Check(gunprofile::RowOf("at4") == gunprofile::kAt4Row && p.pose == gunprofile::Pose::kLauncher && !std::strcmp(p.name, "AT4") && !std::strcmp(p.front, "front grip") &&
                  p.up == kUp && p.right == kRight && p.sound == gunprofile::Sound::kPolymer,
              what);
        const std::string skin = ReadText("src\\skin.cpp");
        if (skin.empty()) {
            Skip("t1 skin.cpp's row (src\\skin.cpp not there: run from the project's folder)");
        } else {
            Check(skin.find(kSkinRow) != std::string::npos && skin.find("constexpr size_t kAt4Gun = 1001;") != std::string::npos,
                  "t1 skin.cpp's kHeldKinds row for it is this one, word for word (its marker kAt4Gun 1001)");
        }
    }

    // ---- t2, t3: the mesh, its sights ----
    meshdump::Mesh mesh;
    const bool haveMesh = meshdump::LoadDraw(dir, kBody, kFrameBone, {}, mesh);
    if (!haveMesh) {
        Skip("t2 its mesh (build\\mesh_15987_0_* not there)");
        Skip("t3 its sights (its mesh not there)");
    } else {
        std::vector<char> used(mesh.part.size(), 0);
        for (int v : mesh.tris)
            used[static_cast<size_t>(v)] = 1;
        auto at = [&](size_t v) { return &mesh.pos[v * 3]; };
        float lo[4][3], hi[4][3];
        int counts[4] = {};
        bool other = false;
        for (int b = 0; b < 4; ++b)
            for (int k = 0; k < 3; ++k)
                lo[b][k] = 9.0f, hi[b][k] = -9.0f;
        for (size_t v = 0; v < mesh.part.size(); ++v) {
            if (!used[v])
                continue;
            const int b = mesh.part[v];
            if (b < 0 || b > 3) {
                other = true;
                continue;
            }
            ++counts[b];
            for (int k = 0; k < 3; ++k)
                lo[b][k] = std::min(lo[b][k], at(v)[k]), hi[b][k] = std::max(hi[b][k], at(v)[k]);
        }
        std::snprintf(what, sizeof(what), "t2 its bones: the frame %d vertices z %.3f to %.3f; the lever %d at z %.3f-%.3f, x %.3f-%.3f; the safety %d at z %.3f-%.3f; the front grip %d at z %.3f-%.3f, y %.3f to %.3f; no other bone",
                      counts[0], lo[0][2], hi[0][2], counts[1], lo[1][2], hi[1][2], lo[1][0], hi[1][0], counts[2], lo[2][2], hi[2][2], counts[3], lo[3][2], hi[3][2], lo[3][1], hi[3][1]);
        Check(!other && counts[0] > 5000 && counts[1] > 100 && counts[2] > 30 && counts[3] > 30 && lo[1][2] > 0.43f && hi[1][2] < 0.47f && hi[1][0] < 0.0f &&
                  lo[2][2] > 0.54f && hi[2][2] < 0.56f && lo[3][2] > 0.87f && hi[3][2] < 0.886f && lo[3][1] < -0.09f && hi[0][2] < kMuzzle[2] + 0.005f &&
                  lo[0][2] > -0.01f,
              what);
        float fx0 = 9, fx1 = -9, fy0 = 9, fy1 = -9;
        for (size_t v = 0; v < mesh.part.size(); ++v)
            if (used[v] && mesh.part[v] == 0 && at(v)[2] >= 0.985f && at(v)[1] < 0.115f)
                fx0 = std::min(fx0, at(v)[0]), fx1 = std::max(fx1, at(v)[0]), fy0 = std::min(fy0, at(v)[1]), fy1 = std::max(fy1, at(v)[1]);
        const float ring[3] = {0.5f * (fx0 + fx1), 0.5f * (fy0 + fy1), kMuzzle[2]};
        std::snprintf(what, sizeof(what), "t2 the tube's front ring's middle (%.4f %.4f), across %.1f cm: the row's muzzle (%.3f %.3f %.3f) within 2 mm (%.2f mm)", ring[0], ring[1],
                      (fx1 - fx0) * 100.0f, kMuzzle[0], kMuzzle[1], kMuzzle[2], Dist(ring, kMuzzle) * 1000.0f);
        Check(Dist(ring, kMuzzle) < 0.002f && fx1 - fx0 > 0.09f && fx1 - fx0 < 0.13f, what);

        // The rear aperture: the leaf's ring (the leaf's plane z 0.612-0.6165), its points 3.5-6 mm from where it's looked for, under the upper ring (y 0.1333).
        std::vector<const float*> ringPoints;
        const float lookFor[2] = {0.0601f, 0.1279f};
        for (size_t v = 0; v < mesh.part.size(); ++v) {
            if (!used[v] || mesh.part[v] != 0)
                continue;
            const float* p = at(v);
            const float d = std::sqrt((p[0] - lookFor[0]) * (p[0] - lookFor[0]) + (p[1] - lookFor[1]) * (p[1] - lookFor[1]));
            if (p[2] >= 0.612f && p[2] <= 0.6165f && d >= 0.0035f && d <= 0.006f && p[1] <= 0.1333f)
                ringPoints.push_back(p);
        }
        // A circle through them (least squares: x^2 + y^2 + D x + E y + F = 0).
        double m[3][4] = {};
        for (const float* p : ringPoints) {
            const double row[3] = {p[0], p[1], 1.0}, rhs = -(static_cast<double>(p[0]) * p[0] + static_cast<double>(p[1]) * p[1]);
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j)
                    m[i][j] += row[i] * row[j];
                m[i][3] += row[i] * rhs;
            }
        }
        for (int c = 0; c < 3; ++c) {  // (Gauss-Jordan)
            int pivot = c;
            for (int r = c + 1; r < 3; ++r)
                if (std::fabs(m[r][c]) > std::fabs(m[pivot][c]))
                    pivot = r;
            for (int k = 0; k < 4; ++k)
                std::swap(m[c][k], m[pivot][k]);
            const double d = m[c][c] != 0.0 ? m[c][c] : 1e-30;
            for (int k = 0; k < 4; ++k)
                m[c][k] /= d;
            for (int r = 0; r < 3; ++r)
                if (r != c)
                    for (int k = 3; k >= 0; --k)
                        m[r][k] -= m[r][c] * m[c][k];
        }
        const double cx = -0.5 * m[0][3], cy = -0.5 * m[1][3], radius = std::sqrt(std::max(0.0, cx * cx + cy * cy - m[2][3]));
        double worstOff = 0.0, zSum = 0.0;
        for (const float* p : ringPoints) {
            worstOff = std::max(worstOff, std::fabs(std::sqrt((p[0] - cx) * (p[0] - cx) + (p[1] - cy) * (p[1] - cy)) - radius));
            zSum += p[2];
        }
        const float rear[3] = {static_cast<float>(cx), static_cast<float>(cy), ringPoints.empty() ? 0.0f : static_cast<float>(zSum / ringPoints.size())};
        std::snprintf(what, sizeof(what), "t3 the rear aperture: a ring of %zu points round (%.4f %.4f) at z %.4f, radius %.2f mm, every point within %.2f mm of it (0.3)", ringPoints.size(),
                      rear[0], rear[1], rear[2], radius * 1000.0, worstOff * 1000.0);
        Check(ringPoints.size() >= 16 && radius > 0.004 && radius < 0.006 && worstOff < 0.0003 && std::fabs(cx - lookFor[0]) < 0.0005 && std::fabs(cy - lookFor[1]) < 0.0005, what);
        // The front sight: a plate over the tube (z 0.955-0.975, x 0.03-0.08) whose top is three posts side by side; the aiming post is the middle one, its
        // tip the middle of its top across x, the plate's middle along z.
        float topY = -1.0f;
        for (size_t v = 0; v < mesh.part.size(); ++v)
            if (used[v] && mesh.part[v] == 0 && at(v)[2] > 0.955f && at(v)[2] < 0.975f && at(v)[0] > 0.03f && at(v)[0] < 0.08f && at(v)[1] > 0.10f)
                topY = std::max(topY, at(v)[1]);
        std::vector<const float*> top;
        for (size_t v = 0; v < mesh.part.size(); ++v)
            if (used[v] && mesh.part[v] == 0 && at(v)[2] > 0.955f && at(v)[2] < 0.975f && at(v)[0] > 0.03f && at(v)[0] < 0.08f && at(v)[1] >= topY - 0.0002f)
                top.push_back(at(v));
        std::sort(top.begin(), top.end(), [](const float* a, const float* b) { return a[0] < b[0]; });
        std::vector<std::pair<size_t, size_t>> posts;  // (first, last) in top, split where x jumps 2 mm
        for (size_t i = 0; i < top.size(); ++i)
            if (posts.empty() || top[i][0] - top[i - 1][0] > 0.002f)
                posts.push_back({i, i});
            else
                posts.back().second = i;
        float tipStore[3] = {};
        const float* tip = nullptr;
        if (posts.size() == 3) {
            float zSum = 0.0f;
            for (const float* p : top)
                zSum += p[2];
            tipStore[0] = 0.5f * (top[posts[1].first][0] + top[posts[1].second][0]), tipStore[1] = topY, tipStore[2] = zSum / static_cast<float>(top.size());
            tip = tipStore;
        }
        std::snprintf(what, sizeof(what), "t3 the front sight: %zu posts on its top (3) at y %.4f, the middle one %.4f to %.4f across", posts.size(), topY,
                      posts.size() == 3 ? top[posts[1].first][0] : 0.0f, posts.size() == 3 ? top[posts[1].second][0] : 0.0f);
        Check(posts.size() == 3, what);
        if (!tip) {
            Check(false, "t3 the front post: not found");
        } else {
            const float line[3] = {tip[0] - rear[0], tip[1] - rear[1], tip[2] - rear[2]};
            const float up = std::atan2(-line[1], line[2]) * 180.0f / kPi, right = std::atan2(-line[0], line[2]) * 180.0f / kPi;
            std::snprintf(what, sizeof(what), "t3 the middle post's tip (%.5f %.5f %.5f); the line from the aperture's middle (%.5f %.5f %.5f) %.3f degrees under z and %.3f right of it: the row's %.2f and %.2f within 0.02",
                          tip[0], tip[1], tip[2], rear[0], rear[1], rear[2], up, right, kUp, kRight);
            Check(std::fabs(up - kUp) < 0.02f && std::fabs(right - kRight) < 0.02f, what);
            const float u = (kSights[2] - rear[2]) / line[2];
            const float atMuzzle[3] = {rear[0] + u * line[0], rear[1] + u * line[1], kSights[2]};
            std::snprintf(what, sizeof(what), "t3 the row's sights (%.4f %.4f %.4f): that line at the muzzle (%.4f %.4f) within 0.5 mm (%.2f mm)", kSights[0], kSights[1], kSights[2],
                          atMuzzle[0], atMuzzle[1], Dist(atMuzzle, kSights) * 1000.0f);
            Check(Dist(atMuzzle, kSights) < 0.0005f, what);
            // The game's own zoomed eye: on that line, looking along it.
            const float ue = (kZoomedEye[2] - rear[2]) / line[2];
            const float onLine[3] = {rear[0] + ue * line[0], rear[1] + ue * line[1], kZoomedEye[2]};
            const float n = std::sqrt(line[0] * line[0] + line[1] * line[1] + line[2] * line[2]);
            const float cosA = (line[0] * kZoomedForward[0] + line[1] * kZoomedForward[1] + line[2] * kZoomedForward[2]) /
                               (n * std::sqrt(kZoomedForward[0] * kZoomedForward[0] + kZoomedForward[1] * kZoomedForward[1] + kZoomedForward[2] * kZoomedForward[2]));
            const float apart = std::acos(std::min(1.0f, cosA)) * 180.0f / kPi;
            std::snprintf(what, sizeof(what), "t3 the game's zoomed eye (%.3f %.3f %.3f) %.2f mm off that line, looking %.3f degrees off it (within 1 mm and 0.15: its one sample's sway)",
                          kZoomedEye[0], kZoomedEye[1], kZoomedEye[2], Dist(onLine, kZoomedEye) * 1000.0f, apart);
            Check(Dist(onLine, kZoomedEye) < 0.001f && apart < 0.15f, what);
        }
    }

    // ---- t4: the recording ----
    {
        const std::vector<Record> recs = Recording(survey + "\\AT4_fire.bin");
        if (recs.empty()) {
            Skip("t4 its recording (build\\survey\\AT4_fire.bin not there)");
        } else {
            // Each gloves' draw against each of the gun's frames of the same frame serial: the eye whose frame puts the right palm nearest the row's.
            float best = 1e9f, palms[2][3] = {};
            int pairs = 0;
            for (const Record& g : recs) {
                if (g.count != kGloves[0] || g.start != kGloves[1] || g.bones.size() < (kRightRoot + 1) * 12)
                    continue;
                for (const Record& gun : recs) {
                    if (gun.count != kBody || gun.start != 0 || gun.frame != g.frame || gun.bones.size() < 12)
                        continue;
                    float inv[12], on[12], at[2][3];
                    InvertRigid(&gun.bones[kFrameBone * 12], inv);
                    Compose(inv, &g.bones[kRightRoot * 12], on);
                    Apply(on, kRightPalmPoint, at[0]);
                    Compose(inv, &g.bones[kLeftPalmBone * 12], on);
                    Apply(on, kLeftPalmPoint, at[1]);
                    ++pairs;
                    if (const float d = Dist(at[0], kRightPalm); d < best)
                        best = d, std::memcpy(palms, at, sizeof(palms));
                }
            }
            std::snprintf(what, sizeof(what), "t4 the game's palms at rest (%d pairs): right (%.3f %.3f %.3f) %.2f mm from the row's, left (%.3f %.3f %.3f) %.2f mm (within 1 mm)", pairs,
                          palms[0][0], palms[0][1], palms[0][2], Dist(palms[0], kRightPalm) * 1000.0f, palms[1][0], palms[1][1], palms[1][2], Dist(palms[1], kLeftPalm) * 1000.0f);
            Check(pairs > 0 && Dist(palms[0], kRightPalm) < 0.001f && Dist(palms[1], kLeftPalm) < 0.001f, what);
        }
    }

    // ---- t5: the sights' turn ----
    {
        // The measured line (the aperture's middle to the middle post's tip: t3's numbers), turned by its sights' turn, onto z.
        const float line[3] = {0.05598f - 0.06010f, 0.12500f - 0.12785f, 0.96536f - 0.61572f};
        float turn[9];
        gunprofile::SightsTurn(kUp, kRight, turn);
        const float n = std::sqrt(line[0] * line[0] + line[1] * line[1] + line[2] * line[2]);
        float onto[3];
        for (int r = 0; r < 3; ++r)
            onto[r] = (turn[r * 3] * line[0] + turn[r * 3 + 1] * line[1] + turn[r * 3 + 2] * line[2]) / n;
        const float off = std::acos(std::min(1.0f, onto[2]));
        std::snprintf(what, sizeof(what), "t5 its sights' line turned by its up and right lies along the aim: %.2e rad off (the row's rounding: within 4e-4)", off);
        Check(off < 4e-4f, what);
        bool same = true;
        for (int i = -100; i <= 100 && same; ++i) {
            const float up = static_cast<float>(i) * 0.01f;
            float raise[9], got[9];
            ParentRaise(up, raise);
            gunprofile::SightsTurn(up, 0.0f, got);
            same = std::memcmp(got, raise, sizeof(raise)) == 0;
        }
        for (const float up : {0.29f, 0.47f, -0.7f, 3.0f, 0.0f, -0.0f}) {  // (the rifle's, the AT4's, a launcher's, and either zero)
            float raise[9], got[9];
            ParentRaise(up, raise);
            gunprofile::SightsTurn(up, 0.0f, got);
            same = same && std::memcmp(got, raise, sizeof(raise)) == 0;
        }
        Check(same, "t5 with no right (every gun but the AT4) the sights' turn is the raise as built before it (a frozen copy, out of line), bit for bit, "
                    "for every up from -1 to 1 degree and the guns' own");
    }

    // ---- t6: a hand's use of a prompt ----
    {
        // Which prompts a hand uses (the campaign's, Venice-EBX): the AT4's (the reload key, a press, 2 m), the doors' (interact, a press, 0.75-3 m);
        // never the roof's pose prompts (EIAProne 14, EIAChangePose 13 held 1 s, 100 m), a hold, the jet's 300 m, a jump, a flag, a hold not read.
        struct ByHandRow {
            int action, kind;
            float hold, radius;
            bool want;
        } byHand[] = {{15, 0, 0.0f, 2.0f, true},    {35, 0, 0.0f, 0.75f, true},          {35, 0, 0.0f, 3.0f, true},   {35, 0, 0.0f, 3.01f, false},
                      {14, 0, 0.0f, 100.0f, false}, {13, 0, 1.0f, 100.0f, false},        {35, 0, 1.0f, 2.0f, false},  {35, 0, 0.0f, 300.0f, false},
                      {12, 0, 0.0f, 2.0f, false},   {15, 3, 0.0f, 2.0f, false},          {15, 0, -1.0f, 2.0f, false}, {35, 0, 0.0f, std::nanf(""), false},
                      {35, 0, 0.0f, 0.0f, false},   {7, 0, 0.0f, 1.5f, false}};
        int byHandRight = 0;
        for (const ByHandRow& r : byHand)
            byHandRight += usepoint::ByHand(r.action, r.kind, r.hold, r.radius) == r.want;
        std::snprintf(what, sizeof(what), "t6 the prompts a hand uses: %d of %zu rows as they should be (the AT4's and the doors' yes; pose prompts, holds, 300 m, a jump, a flag, a fire, no)",
                      byHandRight, std::size(byHand));
        Check(byHandRight == static_cast<int>(std::size(byHand)), what);

        const float point[3] = {1.0f, 0.0f, 2.0f};
        const float away[3] = {3.0f, 1.0f, 3.0f};
        usepoint::Point p;
        p.on = true;
        std::memcpy(p.at, point, sizeof(p.at));
        const float in1[3] = {1.3f, 0.1f, 2.2f}, out1[3] = {1.6f, 0.0f, 2.0f}, under[3] = {1.0f, -0.35f, 2.0f}, over[3] = {1.0f, 1.25f, 2.0f};
        const float wideUp[3] = {1.65f, 1.0f, 2.0f}, wideLow[3] = {1.55f, 0.05f, 2.0f};
        std::snprintf(what, sizeof(what), "t6 the reach: a hand 36 cm across and 10 up in (%d), 60 cm across out (%d), 35 under out (%d), 1.25 over out (%d), 65 cm across 1 m up in (%d), 55 across 5 up out (%d)",
                      usepoint::Reach(p, in1), usepoint::Reach(p, out1), usepoint::Reach(p, under), usepoint::Reach(p, over), usepoint::Reach(p, wideUp), usepoint::Reach(p, wideLow));
        Check(usepoint::Reach(p, in1) && !usepoint::Reach(p, out1) && !usepoint::Reach(p, under) && !usepoint::Reach(p, over) && usepoint::Reach(p, wideUp) &&
                  !usepoint::Reach(p, wideLow),
              what);
        usepoint::Point c = p;
        c.capsule = true, c.radius = 0.05f;
        const float ca[3] = {0.5f, 0.0f, 2.0f}, cb[3] = {1.5f, 0.0f, 2.0f};
        std::memcpy(c.a, ca, sizeof(c.a)), std::memcpy(c.b, cb, sizeof(c.b));
        const float cIn[3] = {1.4f, 0.19f, 2.0f}, cOut[3] = {1.4f, 0.21f, 2.0f}, cEnd[3] = {1.69f, 0.0f, 2.0f}, cPast[3] = {1.71f, 0.0f, 2.0f};
        const float nan[3] = {std::nanf(""), 0.0f, 0.0f};
        std::snprintf(what, sizeof(what), "t6 ... a measured capsule (its radius 5 cm, 15 more): 19 cm off it in (%d), 21 out (%d), past its end by 19 in (%d), by 21 out (%d); a point not a number never (%d)",
                      usepoint::Reach(c, cIn), usepoint::Reach(c, cOut), usepoint::Reach(c, cEnd), usepoint::Reach(c, cPast), usepoint::Reach(p, nan));
        Check(usepoint::Reach(c, cIn) && !usepoint::Reach(c, cOut) && usepoint::Reach(c, cEnd) && !usepoint::Reach(c, cPast) && !usepoint::Reach(p, nan), what);
        // A prompt's own disc (the AT4's 1.0 m: its tube lies 0.65 m off the point): a hand on the tube in, past the disc out.
        usepoint::Point wide = p;
        wide.disc = 1.0f;
        const float onTube[3] = {1.65f, 0.05f, 2.0f}, pastWide[3] = {2.05f, 0.0f, 2.0f};
        std::snprintf(what, sizeof(what), "t6 ... the AT4's own disc (1.0 m): a hand on its tube 65 cm off the point in (%d; out with 0.5 m: %d), 1.05 m across out (%d)",
                      usepoint::Reach(wide, onTube), usepoint::Reach(p, onTube), usepoint::Reach(wide, pastWide));
        Check(usepoint::Reach(wide, onTube) && !usepoint::Reach(p, onTube) && !usepoint::Reach(wide, pastWide), what);
        // Its edge: in at 49 cm (the tick), still in at 53 (no tick again on coming back), out past 55, back in at 49: a tick again.
        {
            usepoint::State st;
            usepoint::Out o;
            int ticks = 0, tickFrames[4] = {-1, -1, -1, -1};
            const float edgeIn[3] = {1.49f, 0.0f, 2.0f}, edgeNear[3] = {1.53f, 0.0f, 2.0f}, edgeOut[3] = {1.57f, 0.0f, 2.0f}, distant[3] = {3.0f, 1.0f, 3.0f};
            const float* path[] = {distant, edgeIn, edgeNear, edgeIn, edgeNear, edgeOut, edgeIn};
            for (int i = 0; i < 70; ++i) {
                usepoint::Update(UseFrameAt(point, path[i / 10], distant, 0.0f, 0.0f), st, o);
                if (o.entered[0] && ticks < 4)
                    tickFrames[ticks++] = i;
            }
            std::snprintf(what, sizeof(what), "t6 the edge of its reach: %d ticks (2: at frames %d and %d; 10 and 60), none on coming back from 4 cm out", ticks, tickFrames[0], tickFrames[1]);
            Check(ticks == 2 && tickFrames[0] == 10 && tickFrames[1] == 60, what);
        }

        // The left hand comes into reach (a tick), squeezes, holds 0.25 s: one use; held on: no other; let go and squeezed again: another.
        usepoint::State st;
        usepoint::Out o;
        int uses = 0, ticks = 0, firstUseFrame = -1, spentFrames = 0;
        for (int i = 0; i < 200; ++i) {
            const float* left = i < 10 ? away : in1;
            const float squeeze = (i >= 20 && i < 80) || i >= 100 ? 1.0f : 0.0f;
            usepoint::Update(UseFrameAt(point, left, away, squeeze, 0.0f), st, o);
            uses += o.use == 0;
            ticks += o.entered[0];
            spentFrames += o.spent[0];
            if (o.use == 0 && firstUseFrame < 0)
                firstUseFrame = i;
            if (o.use == 1 || o.given[0] || o.spent[1])
                uses += 100;  // (never the right, never given up)
        }
        std::snprintf(what, sizeof(what), "t6 a fresh squeeze in reach, held: %d uses in two squeezes (2), the first %d frames on (18: 0.25 s at 72 Hz), %d ticks (1), its squeeze the use's %d frames (160)",
                      uses, firstUseFrame - 20, ticks, spentFrames);
        Check(uses == 2 && firstUseFrame - 20 == 17 && ticks == 1 && spentFrames == 160, what);

        // A squeeze carried into reach: no use; out of reach mid-hold: the hold starts again.
        usepoint::Clear(st);
        uses = 0;
        int spentCarried = 0;
        for (int i = 0; i < 60; ++i) {
            usepoint::Update(UseFrameAt(point, i < 10 ? away : in1, away, 1.0f, 0.0f), st, o);
            uses += o.use >= 0;
            spentCarried += o.spent[0];
        }
        usepoint::Clear(st);
        int usedAt = -1;
        for (int i = 0; i < 80; ++i) {
            const float* left = i >= 10 && i < 14 ? out1 : in1;  // (out of reach 4 frames, 0.14 s into the hold)
            usepoint::Update(UseFrameAt(point, left, away, 1.0f, 0.0f), st, o);
            if (o.use == 0 && usedAt < 0)
                usedAt = i;
        }
        std::snprintf(what, sizeof(what), "t6 a squeeze carried into reach: %d uses, never the use's (%d frames); a hold left 4 frames: used at frame %d (18 after it came back: 31)",
                      uses, spentCarried, usedAt);
        Check(uses == 0 && spentCarried == 0 && usedAt == 31, what);

        // A door (a prompt that gives no gun) with a gun in the other hand: no bar (the rifle in the right hand, the left opens it): the free left's
        // squeeze held 0.25 s is the use, once; the right's own squeeze (it holds the gun) never anything.
        usepoint::Clear(st);
        int gunUses = 0, gunRight = 0, gunAt = -1, spentGun = 0;
        for (int i = 0; i < 60; ++i) {
            usepoint::Frame f = UseFrameAt(point, in1, in1, i >= 5 && i < 40 ? 1.0f : 0.0f, i >= 5 ? 1.0f : 0.0f);
            f.hand[1].empty = false;  // (the right holds the M4A1, in reach too)
            f.gunHeld = true;
            usepoint::Update(f, st, o);
            gunUses += o.use == 0;
            gunRight += o.use == 1 || o.spent[1] || o.entered[1] || o.given[1] || o.refused[1];
            if (o.use == 0 && gunAt < 0)
                gunAt = i;
            spentGun += o.spent[0];
        }
        std::snprintf(what, sizeof(what), "t6 a door with a gun in the other hand: the free hand's use (%d, frame %d: 22), the gun's hand never anything (%d frames), the squeeze the use's %d frames (35)",
                      gunUses, gunAt, gunRight, spentGun);
        Check(gunUses == 1 && gunAt == 22 && gunRight == 0 && spentGun == 35, what);

        // The AT4's prompt (it gives a gun) with a gun in the other hand (the owner, 2026-11-19: "Rifle dropped on the floor first", "Better to physically
        // drop the rifle and pick up the AT"): no tick, no use; the squeeze held 0.25 s refused once (frame 22), the double click (its first pulse then,
        // its second 60 ms on: frame 27), the squeeze still the use's till let go (35 frames: no other thing of the hands' comes of it); the right never
        // anything. The rifle let go of at frame 45 with the left still in reach: the tick then; a fresh squeeze at 50: the use at 67.
        usepoint::Clear(st);
        int barUses = 0, barTicks = 0, barRefused = 0, barAt = -1, barRight = 0, barSpent = 0, tickAt = -1, barUsedAt = -1;
        int pulseFrames[3] = {-1, -1, -1}, pulses = 0;
        for (int i = 0; i < 100; ++i) {
            const float squeeze = (i >= 5 && i < 40) || i >= 50 ? 1.0f : 0.0f;
            usepoint::Frame f = UseFrameAt(point, in1, in1, squeeze, i < 45 ? 1.0f : 0.0f);
            f.point.gives = true;
            f.hand[1].empty = i >= 45;  // (the right holds the M4A1 till frame 45)
            f.gunHeld = i < 45;
            usepoint::Update(f, st, o);
            barUses += o.use == 0 && i < 45;
            barTicks += o.entered[0] && i < 45;
            barRefused += o.refused[0];
            if (o.refused[0] && barAt < 0)
                barAt = i;
            barRight += (o.use == 1 || o.spent[1] || o.entered[1] || o.given[1] || o.refused[1]) && i < 45;
            barSpent += o.spent[0] && i < 45;
            if (o.pulse[0].seconds > 0.0f && o.pulse[0].amplitude == usepoint::kDoubleAmp && pulses < 3)
                pulseFrames[pulses++] = i;
            if (o.entered[0] && i >= 45 && tickAt < 0)
                tickAt = i;
            if (o.use == 0 && i >= 45 && barUsedAt < 0)
                barUsedAt = i;
        }
        std::snprintf(what, sizeof(what), "t6 the AT4's prompt with a gun in the other hand: no use (%d), no tick (%d), refused %d time(s) at frame %d (22), the double click's pulses at frames %d and %d (22, 27; %d in all), the squeeze the use's %d frames (35), the gun's hand never anything (%d); the gun let go of: the tick at frame %d (45), the use at %d (67)",
                      barUses, barTicks, barRefused, barAt, pulseFrames[0], pulseFrames[1], pulses, barSpent, barRight, tickAt, barUsedAt);
        Check(barUses == 0 && barTicks == 0 && barRefused == 1 && barAt == 22 && pulses == 2 && pulseFrames[0] == 22 && pulseFrames[1] == 27 && barSpent == 35 &&
                  barRight == 0 && tickAt == 45 && barUsedAt == 67,
              what);

        // The bar as the hold ends: the gun let go of within the hold (frame 15, the squeeze from 5): the use at its end (22); a gun taken into a hand within
        // it (from frame 15): refused at 22. The same prompt with no gun anywhere: the use, as a door's.
        int endUse[2] = {-2, -2}, endRefused[2] = {};
        for (int comes = 0; comes < 2; ++comes) {
            usepoint::Clear(st);
            for (int i = 0; i < 40; ++i) {
                usepoint::Frame f = UseFrameAt(point, in1, away, i >= 5 ? 1.0f : 0.0f, 0.0f);
                f.point.gives = true;
                f.gunHeld = comes ? i >= 15 : i < 15;
                usepoint::Update(f, st, o);
                if (o.use == 0 && endUse[comes] < 0)
                    endUse[comes] = i;
                endRefused[comes] += o.refused[0];
            }
        }
        usepoint::Clear(st);
        int freeAt = -1;
        for (int i = 0; i < 40; ++i) {
            usepoint::Frame f = UseFrameAt(point, in1, away, i >= 5 ? 1.0f : 0.0f, 0.0f);
            f.point.gives = true;
            usepoint::Update(f, st, o);
            if (o.use == 0 && freeAt < 0)
                freeAt = i;
        }
        std::snprintf(what, sizeof(what), "t6 ... the gun let go of within the hold: used at frame %d (22, refused %d); taken up within it: refused %d (1), used %d (never); no gun anywhere: used at %d (22)",
                      endUse[0], endRefused[0], endRefused[1], endUse[1], freeAt);
        Check(endUse[0] == 22 && endRefused[0] == 0 && endRefused[1] == 1 && endUse[1] == -2 && freeAt == 22, what);

        // A refusal made outside Update (hands.cpp's for X held's R, usepoint::Refuse): its first pulse returned, its second sent by Update 60 ms on, once;
        // a new life (Clear) before it's due drops it.
        usepoint::Clear(st);
        const usepoint::Pulse first = usepoint::Refuse(st, 1);
        int secondAt = -1, seconds = 0;
        for (int i = 1; i <= 20; ++i) {
            usepoint::Update(UseFrameAt(point, away, away, 0.0f, 0.0f), st, o);
            if (o.pulse[1].seconds > 0.0f) {
                ++seconds;
                if (secondAt < 0)
                    secondAt = i;
            }
        }
        usepoint::Clear(st);
        usepoint::Refuse(st, 0);
        usepoint::Clear(st);
        int dropped = 0;
        for (int i = 0; i < 20; ++i) {
            usepoint::Update(UseFrameAt(point, away, away, 0.0f, 0.0f), st, o);
            dropped += o.pulse[0].seconds > 0.0f;
        }
        const usepoint::Pulse none = usepoint::Refuse(st, 2);
        std::snprintf(what, sizeof(what), "t6 a refusal outside Update: its first pulse %.2f for %.0f ms, its second %d frame(s) on (5), %d in all (1); cleared before it's due: %d (0); a side out of range: %.2f",
                      first.amplitude, first.seconds * 1000.0f, secondAt, seconds, dropped, none.amplitude);
        Check(first.amplitude == usepoint::kDoubleAmp && first.seconds == usepoint::kDoubleSeconds && secondAt == 5 && seconds == 1 && dropped == 0 &&
                  none.seconds == 0.0f,
              what);

        // Something else's squeeze (yield: a body's place, near a gun in a hand, a gun lying under the hand) at its start: never the use's, no tick; the
        // same squeeze held on as the hand stops yielding: still not; let go and squeezed again there: the use.
        usepoint::Clear(st);
        int yieldUses = 0, yieldSpent = 0, yieldTicks = 0, usedAgainAt = -1;
        for (int i = 0; i < 90; ++i) {
            usepoint::Frame f = UseFrameAt(point, in1, away, (i >= 5 && i < 50) || i >= 60 ? 1.0f : 0.0f, 0.0f);
            f.hand[0].yield = i < 20;
            usepoint::Update(f, st, o);
            yieldUses += o.use == 0 && i < 60;
            yieldSpent += o.spent[0] && i < 60;
            yieldTicks += o.entered[0] && i < 20;
            if (o.use == 0 && i >= 60 && usedAgainAt < 0)
                usedAgainAt = i;
        }
        std::snprintf(what, sizeof(what), "t6 a squeeze something else wants at its start: no use (%d), not the use's (%d frames), no tick (%d); squeezed again once free: used at frame %d (77)",
                      yieldUses, yieldSpent, yieldTicks, usedAgainAt);
        Check(yieldUses == 0 && yieldSpent == 0 && yieldTicks == 0 && usedAgainAt == 77, what);

        // The use's squeeze that takes something else first (the hand not empty 5 frames in: a magazine, a grenade, the gun's front): given up once, no
        // use, still the use's till let go.
        usepoint::Clear(st);
        int givenUp = 0, givenUses = 0, givenSpent = 0;
        for (int i = 0; i < 60; ++i) {
            usepoint::Frame f = UseFrameAt(point, in1, away, i >= 5 && i < 40 ? 1.0f : 0.0f, 0.0f);
            f.hand[0].empty = !(i >= 10 && i < 40);
            usepoint::Update(f, st, o);
            givenUp += o.given[0];
            givenUses += o.use >= 0;
            givenSpent += o.spent[0];
        }
        std::snprintf(what, sizeof(what), "t6 the use's squeeze taking something else first: given up %d time(s) (1), no use (%d), the use's %d frames (35)", givenUp,
                      givenUses, givenSpent);
        Check(givenUp == 1 && givenUses == 0 && givenSpent == 35, what);

        // Nothing: not targeted, asleep, not tracked, a hand holding something.
        int nothing = 0;
        for (int which = 0; which < 4; ++which) {
            usepoint::Clear(st);
            for (int i = 0; i < 60; ++i) {
                usepoint::Frame f = UseFrameAt(point, in1, away, i >= 5 ? 1.0f : 0.0f, 0.0f);
                if (which == 0)
                    f.point.on = false;
                else if (which == 1)
                    f.asleep = true;
                else if (which == 2)
                    f.hand[0].tracked = false;
                else
                    f.hand[0].empty = false;
                usepoint::Update(f, st, o);
                nothing += o.use >= 0 || o.given[0] || o.spent[0] || o.entered[0] || o.pulse[0].seconds > 0.0f;
            }
        }
        std::snprintf(what, sizeof(what), "t6 not targeted, free hands asleep, the hand not tracked, the hand holding something: nothing at all (%d frames said anything)", nothing);
        Check(nothing == 0, what);

        // Both hands squeezing in reach together: one use, the first in order (the left), the other's squeeze still the use's.
        usepoint::Clear(st);
        int left = 0, right = 0, bothSpent = 0;
        for (int i = 0; i < 40; ++i) {
            usepoint::Update(UseFrameAt(point, in1, in1, i >= 5 ? 1.0f : 0.0f, i >= 5 ? 1.0f : 0.0f), st, o);
            left += o.use == 0, right += o.use == 1;
            bothSpent += o.spent[0] && o.spent[1];
        }
        std::snprintf(what, sizeof(what), "t6 both hands squeezed in reach at once: the left's use (%d), the right's never in the same frame (%d after it), both squeezes the use's (%d frames)", left, right,
                      bothSpent);
        Check(left == 1 && right <= 1 && bothSpent == 35, what);
    }

    // ---- t7: the quick-time guard ----
    {
        qtelist::List list;
        list.status = qtelist::Status::kOk;
        list.count = 3;
        list.at[0].action = 0x07, list.at[0].entity = 0x1000, list.at[0].bus = 0x9000;  // Fire (the roof's fire trigger)
        list.at[1].action = 0x3D, list.at[1].entity = 0x2000, list.at[1].bus = 0x9000;  // the quick-time Fire
        list.at[2].action = 0x3C, list.at[2].entity = 0x3000, list.at[2].bus = 0x9000;  // InteractDrag (E)
        const auto frameOf = [&](bool scripted, unsigned barred) {
            const qtelist::Keys keys = qtelist::Awaited(list, scripted);
            qtewatch::Frame f;
            f.await = keys.await, f.hold = keys.hold;
            qtewatch::Bar(f, barred);
            return f;
        };
        const qtewatch::Frame free = frameOf(true, 0), barred = frameOf(true, qte::kKeyLeftMouse), play = frameOf(false, qte::kKeyLeftMouse);
        std::snprintf(what, sizeof(what), "t7 Fire 0x07 and the quick-time Fire awaited in a scripted moment: answered with no launcher (await %02X), never with one (await %02X, barred %02X); E answered either way",
                      free.await, barred.await, barred.barred);
        Check((free.await & qte::kKeyLeftMouse) && !(barred.await & qte::kKeyLeftMouse) && (barred.barred & qte::kKeyLeftMouse) && (free.await & qte::kKeyE) &&
                  (barred.await & qte::kKeyE) && (play.await & qte::kKeyLeftMouse) == 0 && free.barred == 0,
              what);

        // A fist fight's open prompt awaiting the quick-time Fire, with a launcher in a hand: the one rule bars the grips' press of it (nothing awaited),
        // and the fights' guard agrees: the prompt's answer stays its own, so the trigger's own left button still goes through it (as it fires the
        // launcher), and every other quick-time key is held up as in any prompt. With no launcher: the grips press it, as before.
        qtewatch::Frame fight;
        fight.prompt = true, fight.answer = qte::kKeyLeftMouse, fight.await = qte::kKeyLeftMouse;
        qtewatch::Frame fightFree = fight;
        qtewatch::Bar(fight, qte::kKeyLeftMouse);
        qtewatch::Bar(fightFree, 0);
        qtefight::Guard guard;
        qtefight::GuardFrame(guard, false, fight.prompt, fight.answer, 1000);
        const qtefight::Pass trigger = qtefight::GuardKey(guard, qte::kKeyLeftMouse, true, 1010), use = qtefight::GuardKey(guard, qte::kKeyE, true, 1020);
        std::snprintf(what, sizeof(what), "t7 a fist fight's prompt for the quick-time Fire with a launcher in a hand: the grips' press barred (await %02X, barred %02X), its answer kept (%02X): the trigger's left button through the guard (%s), E held up (%s); with no launcher the grips' (await %02X)",
                      fight.await, fight.barred, fight.answer, trigger.drop ? "held up" : "through", use.drop ? "held up" : "through", fightFree.await);
        Check(fight.await == 0 && fight.barred == qte::kKeyLeftMouse && fight.answer == qte::kKeyLeftMouse && fight.prompt && !trigger.drop && use.drop &&
                  fightFree.await == qte::kKeyLeftMouse && fightFree.barred == 0 && fightFree.answer == qte::kKeyLeftMouse,
              what);

        // Nothing barred: the frame as it came, bit for bit (every key, held or tapped).
        bool same = true;
        for (unsigned keys = 0; keys <= qte::kKeysV2; ++keys) {
            qtewatch::Frame f;
            f.await = keys, f.hold = keys & 0x15, f.answer = keys & 1u, f.prompt = (keys & 2u) != 0;
            const qtewatch::Frame before = f;
            qtewatch::Bar(f, 0);
            same = same && f.await == before.await && f.hold == before.hold && f.answer == before.answer && f.prompt == before.prompt && f.barred == 0;
            qtewatch::Frame g = before;
            qtewatch::Bar(g, qte::kKeyLeftMouse);
            same = same && g.await == (before.await & ~2u) && g.hold == (before.hold & ~2u) && g.barred == (before.await & 2u) && g.answer == before.answer;
        }
        Check(same, "t7 nothing barred: every frame as it came, bit for bit; the left mouse button barred: only it taken out of the await and the hold, the answer kept");
    }

    // ---- t8: its holster ----
    {
        holster::Settings s;  // (every default: launchers at the secondary's shoulder, behind the neck while a secondary is there)
        holster::SlotIn slots[holster::kSlots];
        slots[0].carried = true, slots[0].launcher = true, slots[0].known = true;  // the AT4 (the kit's, in slot 0)
        slots[1].carried = true, slots[1].known = true;                            // the Mk11
        holster::Place places[holster::kSlots];
        holster::PlacesOf(slots, s, places);
        const holster::Place withMk11 = places[0], mk11 = places[1];
        slots[1] = holster::SlotIn{};
        holster::PlacesOf(slots, s, places);
        std::snprintf(what, sizeof(what), "t8 the AT4 in slot 0 with the Mk11 in slot 1: %s (behind the neck), the Mk11 %s (the left shoulder); with no secondary: %s (the left shoulder)",
                      holster::Name(withMk11), holster::Name(mk11), holster::Name(places[0]));
        Check(withMk11 == holster::Place::kNeck && mk11 == holster::Place::kLeftShoulder && places[0] == holster::Place::kLeftShoulder, what);
        // The Mk11 drawn from the left shoulder while the AT4 is out: the scene's restriction (ChangeWeapon False) keeps the game on slot 0, so key 2 is
        // pressed once again at 0.6 s and given up at 1.5 s (its line: holster.h's kNoSwitch), never landed.
        holster::KeyIn k;
        k.target = 1, k.gameSlot = 0, k.readNow = true;
        const auto step = [&](float t, int presses) {
            holster::KeyIn q = k;
            q.since = t, q.presses = presses;
            return holster::KeyAgain(q, s);
        };
        std::snprintf(what, sizeof(what), "t8 the Mk11's key refused by the scene while the AT4 is out: waits (%d), pressed again at 0.6 s (%d), waits (%d), given up at 1.5 s (%d), never landed",
                      static_cast<int>(step(0.3f, 1)), static_cast<int>(step(0.6f, 1)), static_cast<int>(step(1.2f, 2)), static_cast<int>(step(1.5f, 2)));
        Check(step(0.3f, 1) == holster::KeyStep::kWait && step(0.6f, 1) == holster::KeyStep::kPressAgain && step(1.2f, 2) == holster::KeyStep::kWait &&
                  step(1.5f, 2) == holster::KeyStep::kGiveUp,
              what);
    }

    // ---- t9: the game's use against the image ----
    {
        Image image;
        if (!image.Load(dir + "\\bf3_mem.exe")) {
            Skip("t9 the game's use (bf3_mem.exe not there)");
        } else {
            const std::string pickups = ReadText("src\\pickups.cpp");
            int code = 0, bytes = 0, words = 0, written = 0;
            for (const CodeRow& r : kUseCode) {
                std::vector<uint8_t> b(r.size);
                code += image.Read(r.at, b.data(), b.size()) && Fnv(b.data(), b.size()) == r.fnv;
                written += !pickups.empty() && pickups.find(r.text) != std::string::npos;
            }
            for (const BytesRow& r : kUseBytes) {
                std::vector<uint8_t> b(r.size);
                bytes += image.Read(r.at, b.data(), b.size()) && std::memcmp(b.data(), r.bytes, r.size) == 0;
            }
            for (const WordRow& r : kUseWords) {
                uint32_t v = 0;
                words += image.Read(r.at, &v, r.size) && v == r.value;
                written += !pickups.empty() && pickups.find(r.text) != std::string::npos;
            }
            const int codes = static_cast<int>(std::size(kUseCode)), byteRows = static_cast<int>(std::size(kUseBytes)), wordRows = static_cast<int>(std::size(kUseWords));
            std::snprintf(what, sizeof(what), "t9 the game's use as bf3_mem.exe has it: %d of %d pieces of code by their FNV-1a 64, %d of %d runs of bytes, %d of %d words; %d of %d rows word for word in src\\pickups.cpp",
                          code, codes, bytes, byteRows, words, wordRows, written, codes + wordRows);
            Check(code == codes && bytes == byteRows && words == wordRows && written == codes + wordRows, what);
        }
    }

    // ---- t10: its rocket's start written in its own firing data alone (at4shot.h), against the roof's dump ----
    {
        const std::vector<uint32_t> dump = DumpFrom(ReadText(survey + "\\AT4R_R10_scan.txt"), 0xE8860980u);
        constexpr size_t kFfd = 0xE88609C0u - 0xE8860980u;  // its FiringFunctionData in that dump (its vtable 021C2A68 there, its shot at E8860A20)
        if (dump.size() * 4 < kFfd + at4shot::kFiringBytes) {
            Skip("t10 its firing data (build\\survey\\AT4R_R10_scan.txt not there, or short)");
        } else {
            std::vector<uint8_t> bytes(dump.size() * 4);
            std::memcpy(bytes.data(), dump.data(), bytes.size());
            const std::vector<uint8_t> ffd(bytes.begin() + kFfd, bytes.begin() + kFfd + at4shot::kFiringBytes);
            // The same data with another gun's numbers (Venice-EBX): the SMAW's (350, 5.8), the RPG-7's (100, 4.8), its soldiers' RPG7_AI (18, 4.8); another
            // class's vtable (the sway data's, 021C2A24); its start somewhere else, as last written there or not.
            auto as = [&](float rate, float reload, uint32_t vtable, const float* start, const float* last) {
                std::vector<uint8_t> b = ffd;
                std::memcpy(&b[at4shot::kRateAt], &rate, sizeof(rate));
                std::memcpy(&b[at4shot::kReloadAt], &reload, sizeof(reload));
                std::memcpy(&b[0], &vtable, sizeof(vtable));
                if (start)
                    std::memcpy(&b[at4shot::kShotAt], start, 3 * sizeof(float));
                return at4shot::FiringOurs(b.data(), last);
            };
            const uint32_t vt = at4shot::kFiringVtable;
            const float ring[3] = {0.056f, -0.32f, 0.88f}, elsewhere[3] = {0.1f, -0.3f, 0.9f};
            const bool own = at4shot::FiringOurs(ffd.data(), nullptr), smaw = as(350.0f, 5.8f, vt, nullptr, nullptr), rpg = as(100.0f, 4.8f, vt, nullptr, nullptr),
                       ai = as(18.0f, 4.8f, vt, nullptr, nullptr), other = as(350.0f, 4.2f, 0x021C2A24u, nullptr, nullptr),
                       written = as(350.0f, 4.2f, vt, ring, ring), notWritten = as(350.0f, 4.2f, vt, ring, elsewhere) || as(350.0f, 4.2f, vt, ring, nullptr);
            float rate = 0.0f, reload = 0.0f;
            std::memcpy(&rate, &ffd[at4shot::kRateAt], sizeof(rate));
            std::memcpy(&reload, &ffd[at4shot::kReloadAt], sizeof(reload));
            std::snprintf(what, sizeof(what), "t10 the roof's dump of its firing data (rate %.0f, reload %.2f): its own (%d); never the SMAW's (%d), the RPG-7's (%d), its soldiers' (%d), another class (%d); its start as last written (%d), never another's (%d)",
                          rate, reload, own, smaw, rpg, ai, other, written, notWritten);
            Check(own && !smaw && !rpg && !ai && !other && written && !notWritten, what);

            // ---- t10b: the SMAW's rocket's start alike (skin.cpp's ClearLauncherSpread and its writer, at4shot::FiringIs with kSmaw): the roof's
            // two exact matches are the AT4's and the enemies' RPG-7's (each its instance's GUID 16 bytes before its vtable: Venice-EBX's M136
            // 3385F7CF-15C2-4D2D-B571-1CB94130ABF4 and RPG7_AI 4A2ADB00-B529-4C70-925B-AD77F312AC44), and the SMAW's scan took both by their shot alone ----
            auto is = [&](const at4shot::Launcher& who, float rate, float reload, uint32_t vtable, const float* start, const float* last) {
                std::vector<uint8_t> b = ffd;
                std::memcpy(&b[at4shot::kRateAt], &rate, sizeof(rate));
                std::memcpy(&b[at4shot::kReloadAt], &reload, sizeof(reload));
                std::memcpy(&b[0], &vtable, sizeof(vtable));
                if (start)
                    std::memcpy(&b[at4shot::kShotAt], start, 3 * sizeof(float));
                return at4shot::FiringIs(b.data(), who, last);
            };
            float shot[11];
            std::memcpy(shot, &ffd[at4shot::kShotAt], sizeof(shot));
            const bool oldTook = at4shot::ShotOurs(shot, nullptr);  // (the scan before: the shot's numbers alone)
            const bool smawOwn = is(at4shot::kSmaw, 350.0f, 5.8f, vt, nullptr, nullptr), smawAt4 = at4shot::FiringIs(ffd.data(), at4shot::kSmaw, nullptr),
                       smawAi = is(at4shot::kSmaw, 18.0f, 4.8f, vt, nullptr, nullptr), smawRpg = is(at4shot::kSmaw, 100.0f, 4.8f, vt, nullptr, nullptr),
                       smawOther = is(at4shot::kSmaw, 350.0f, 5.8f, 0x021C2A24u, nullptr, nullptr), smawWritten = is(at4shot::kSmaw, 350.0f, 5.8f, vt, ring, ring),
                       smawNot = is(at4shot::kSmaw, 350.0f, 5.8f, vt, ring, elsewhere) || is(at4shot::kSmaw, 350.0f, 5.8f, vt, ring, nullptr);
            const std::vector<uint32_t> second = DumpFrom(ReadText(survey + "\\AT4R_R10_scan.txt"), 0xE8891200u);
            const bool guids = dump.size() > 0x10 && dump[0x0C] == 0x3385F7CFu && dump[0x0D] == 0x4D2D15C2u && dump[0x0E] == 0xB91C71B5u &&
                               dump[0x0F] == 0xF4AB3041u && dump[0x10] == vt && second.size() >= 0x30 && second[0x08] == 0x4A2ADB00u &&
                               second[0x09] == 0x4C70B529u && second[0x0A] == 0x77AD5B92u && second[0x0B] == 0x44AC12F3u && second[0x0C] == vt;
            float secondShot[11] = {};
            if (second.size() >= 0x24 + 11)
                std::memcpy(secondShot, &second[0x24], sizeof(secondShot));  // (its shot: E8891230 + 0x60)
            const bool secondLike = second.size() >= 0x24 + 11 && at4shot::ShotOurs(secondShot, nullptr);
            std::vector<uint8_t> aiBytes = ffd;
            const float aiRate = 18.0f, aiReload = 4.8f;
            std::memcpy(&aiBytes[at4shot::kRateAt], &aiRate, sizeof(aiRate));
            std::memcpy(&aiBytes[at4shot::kReloadAt], &aiReload, sizeof(aiReload));
            const char* whoAi = at4shot::WhoseFiring(aiBytes.data(), nullptr);
            const char* whoAt4 = at4shot::WhoseFiring(ffd.data(), nullptr);
            std::snprintf(what, sizeof(what), "t10b the roof's two exact matches the AT4's and the enemies' RPG-7's by their GUIDs (%d), the second's shot the SMAW's to the bit too (%d), the scan before took them by their shot alone (%d): never the SMAW's by its numbers (the AT4's %d, the enemies' %d: \"%s\", the RPG-7's %d, another class %d); the SMAW's own numbers (350, 5.8) are (%d), its start as last written (%d), never another's (%d); the AT4's named \"%s\"",
                          guids, secondLike, oldTook, smawAt4, smawAi, whoAi ? whoAi : "none", smawRpg, smawOther, smawOwn, smawWritten, smawNot, whoAt4 ? whoAt4 : "none");
            Check(guids && secondLike && oldTook && !smawAt4 && !smawAi && !smawRpg && !smawOther && smawOwn && smawWritten && !smawNot && whoAi &&
                      !std::strcmp(whoAi, "the enemies' RPG-7") && whoAt4 && !std::strcmp(whoAt4, "the AT4"),
                  what);

            // ---- t10b: which of a scan's exact matches are the SMAW's own (at4shot::SmawOwn, skin.cpp's ClearLauncherSpread): by a weapon of the
            // soldier's (its slot's firing data, whatever its numbers: the SMAW's were never read in this game), else Venice-EBX's numbers, else each
            // one that is no known launcher's (the scan before the AT4 took every match); never the AT4's or either RPG-7's. The matches are the roof's
            // dump of the M136's firing data with each one's numbers (and the class) put in; their places a Rock run's two (bf3vr_1105_1535.log). ----
            auto bytesOf = [&](float rate, float reload, uint32_t vtable) {
                std::vector<uint8_t> b = ffd;
                std::memcpy(&b[at4shot::kRateAt], &rate, sizeof(rate));
                std::memcpy(&b[at4shot::kReloadAt], &reload, sizeof(reload));
                std::memcpy(&b[0], &vtable, sizeof(vtable));
                return b;
            };
            auto matchOf = [&](uintptr_t place, const std::vector<uint8_t>& b, bool read) {
                at4shot::Match m;
                m.place = place;
                m.read = read;
                std::memcpy(m.firing, b.data(), sizeof(m.firing));
                return m;
            };
            constexpr uintptr_t kSmawAt = 0xEBCD8FE0u, kAiAt = 0xEBCF7090u, kSecondAt = 0xEBD00000u;  // (shot places; their firing data 0x60 before)
            const std::vector<uint8_t> venice = bytesOf(350.0f, 5.8f, vt), notVenice = bytesOf(350.0f, 5.5f, vt), aiData = bytesOf(18.0f, 4.8f, vt),
                                       otherClass = bytesOf(350.0f, 5.8f, 0x021C2A24u);
            struct Case {
                const char* name;
                std::vector<at4shot::Match> matches;
                std::vector<uintptr_t> fires;
                at4shot::SmawBy by;
                std::vector<bool> own;
            };
            const uintptr_t slot5Smaw[8] = {0x11110000u, 0, 0x22220000u, 0, 0, kSmawAt - at4shot::kShotAt, 0, 0};
            std::vector<Case> cases = {
                {"the roof (the AT4 in slot 0, the Mk11 in 1): neither the AT4's nor RPG7_AI's",
                 {matchOf(0xE8860A20u, ffd, true), matchOf(0xE8891290u, aiData, true)}, {0xE88609C0u, 0x33330000u, 0, 0, 0, 0, 0, 0}, at4shot::SmawBy::kNone, {false, false}},
                {"Rock as Venice-EBX has it, by slot 5", {matchOf(kSmawAt, venice, true), matchOf(kAiAt, aiData, true)},
                 {std::begin(slot5Smaw), std::end(slot5Smaw)}, at4shot::SmawBy::kSlot, {true, false}},
                {"Rock with the SMAW's numbers not Venice-EBX's (350, 5.5), by slot 5", {matchOf(kSmawAt, notVenice, true), matchOf(kAiAt, aiData, true)},
                 {std::begin(slot5Smaw), std::end(slot5Smaw)}, at4shot::SmawBy::kSlot, {true, false}},
                {"Rock, no slot read, Venice-EBX's numbers", {matchOf(kSmawAt, venice, true), matchOf(kAiAt, aiData, true)}, std::vector<uintptr_t>(8, 0),
                 at4shot::SmawBy::kNumbers, {true, false}},
                {"Rock, no slot read, its numbers not Venice-EBX's: no known launcher's", {matchOf(kSmawAt, notVenice, true), matchOf(kAiAt, aiData, true)},
                 std::vector<uintptr_t>(8, 0), at4shot::SmawBy::kNoOther, {true, false}},
                {"two with the SMAW's numbers, slot 5's the second's: that one alone",
                 {matchOf(kSmawAt, venice, true), matchOf(kSecondAt, venice, true)}, {0, 0, 0, 0, 0, kSecondAt - at4shot::kShotAt, 0, 0}, at4shot::SmawBy::kSlot,
                 {false, true}},
                {"a slot holding RPG7_AI's firing data: never by it (then by the numbers)", {matchOf(kSmawAt, venice, true), matchOf(kAiAt, aiData, true)},
                 {kAiAt - at4shot::kShotAt, 0, 0, 0, 0, 0, 0, 0}, at4shot::SmawBy::kNumbers, {true, false}},
                {"its firing data not read: never, even with its slot's", {matchOf(kSmawAt, venice, false)}, {std::begin(slot5Smaw), std::end(slot5Smaw)},
                 at4shot::SmawBy::kNone, {false}},
                {"another class with the SMAW's numbers: never, even with its slot's", {matchOf(kSmawAt, otherClass, true)}, {std::begin(slot5Smaw), std::end(slot5Smaw)},
                 at4shot::SmawBy::kNone, {false}},
                {"the AT4's and both RPG-7s', no slot read: none (the scan before took all three)",
                 {matchOf(0xE8860A20u, ffd, true), matchOf(kAiAt, aiData, true), matchOf(kSecondAt, bytesOf(100.0f, 4.8f, vt), true)}, std::vector<uintptr_t>(8, 0),
                 at4shot::SmawBy::kNone, {false, false, false}},
            };
            int right = 0;
            std::string wrong;
            for (Case& c : cases) {
                const at4shot::SmawBy by = at4shot::SmawOwn(c.matches.data(), c.matches.size(), c.fires.data(), c.fires.size());
                bool same = by == c.by;
                for (size_t i = 0; i < c.matches.size(); ++i)
                    same = same && c.matches[i].own == c.own[i];
                right += same;
                if (!same)
                    wrong += std::string(wrong.empty() ? "" : "; ") + c.name + " (rule " + std::to_string(static_cast<int>(by)) + ")";
            }
            // The rule before this round (Venice-EBX's numbers alone) on a SMAW whose numbers aren't those: nothing taken (its rockets from the eye).
            const bool beforeLost = !at4shot::FiringIs(notVenice.data(), at4shot::kSmaw, nullptr);
            // Each one taken is held to its own numbers as read (LauncherShotThere: FiringIs with at4shot::NumbersOf): the same data's, its start as
            // last written; never RPG7_AI's numbers, nor another start.
            const at4shot::Launcher held = at4shot::NumbersOf(notVenice.data(), at4shot::kSmaw.name);
            std::vector<uint8_t> writtenThere = notVenice;
            std::memcpy(&writtenThere[at4shot::kShotAt], ring, sizeof(ring));
            const bool holds = at4shot::FiringIs(notVenice.data(), held, nullptr) && !at4shot::FiringIs(aiData.data(), held, nullptr) &&
                               at4shot::FiringIs(writtenThere.data(), held, ring) && !at4shot::FiringIs(writtenThere.data(), held, elsewhere) &&
                               held.rate == 350.0f && held.reload == 5.5f;
            std::snprintf(what, sizeof(what), "t10b the SMAW's own among a scan's matches: %d of %zu cases right%s%s; the rule before (Venice-EBX's numbers alone) took none of a SMAW not numbered so (%d); each taken held to its own numbers, its start as last written, never another's (%d)",
                          right, cases.size(), wrong.empty() ? "" : ": wrong ", wrong.c_str(), beforeLost, holds);
            Check(right == static_cast<int>(cases.size()) && beforeLost && holds, what);
        }
    }
    {
        // skin.cpp: the SMAW's scan tells its own by SmawOwn with each slot's firing data, its writer and its putting back hold each place to the numbers
        // it was taken with (word for word), every place comes through AddLauncherShots, the frame stored is never one ahead, and Update puts it back.
        const std::string skin = ReadText("src\\skin.cpp");
        if (skin.empty()) {
            Skip("t10b skin.cpp's SMAW lines (src\\skin.cpp not there: run from the project's folder)");
        } else {
            size_t thereUses = 0;
            for (size_t at = 0; (at = skin.find("LauncherShotThere(place, g_launcherShotWho[n], g_launcherShotLast[n])", at)) != std::string::npos; ++at)
                ++thereUses;
            // (The putting back's test, signed: a frame stored one ahead of the count read wraps an unsigned difference past 36.)
            const uint32_t count = 100, ahead = 101;
            const bool wraps = count - ahead >= 36u && static_cast<int32_t>(count - ahead) < 36;
            Check(skin.find("const at4shot::SmawBy by = at4shot::SmawOwn(matches.data(), matches.size(), fires, std::size(fires));") != std::string::npos &&
                      skin.find("fires[s] = SlotFiringData(static_cast<int>(s));") != std::string::npos &&
                      skin.find("reinterpret_cast<LPCVOID>(state + at4shot::kStateData)") != std::string::npos &&
                      skin.find("static_cast<uintptr_t>(data) + at4shot::kPrimaryFire") != std::string::npos &&
                      skin.find("got == sizeof(firing) && at4shot::FiringIs(firing, who, last);") != std::string::npos && thereUses == 2 &&
                      skin.find("g_launcherShotWho[n] = who;") != std::string::npos && skin.find("const size_t known = AddLauncherShots(taken);") != std::string::npos &&
                      skin.find("AddPlaces(g_launcherShot,") == std::string::npos &&
                      skin.find("g_launcherShotFrame.store(serial ? serial : 1u, std::memory_order_relaxed);") != std::string::npos &&
                      skin.find("static_cast<int32_t>(g_frameSerial.load(std::memory_order_relaxed) - frame) < 36") != std::string::npos && wraps &&
                      skin.find("    RestoreLauncherShot();  // (the SMAW's alike)") != std::string::npos,
                  "t10b skin.cpp: the SMAW's scan tells its own by SmawOwn with each slot's firing data, its writer and its putting back hold each place to its numbers as taken, every place added with them, the frame stored never ahead, Update puts it back");
        }
    }

    // ---- t11: the take's side effects on main's DLL (at4_measured.md section 11; pickrules.h's OtherGun and SeeDry, as hands.cpp's held magazine
    // runs them each frame) ----
    {
        const recoilguns::Row* m4a1 = recoilguns::ByShape("m4a1");
        const recoilguns::Row* at4 = recoilguns::ByName("Weapons/M136/M136");
        // hands.cpp's loop: its count and firing state kept from frame to frame (the firing state before kept while waiting for the slot's name read
        // with the new one, a second at most), its chamber run dry or not.
        struct Held {
            int was = -1;
            uint32_t firingWas = 0;
            bool dry = false;
            int runDry = 0, newGun = 0, notDrawn = 0, waited = 0, waits = 0;
        };
        const auto frame = [](Held& h, const recoilguns::Row* drawn, const recoilguns::Row* inSlot, uint32_t firing, int count, bool nameWithIt = true) {
            pickrules::DryIn d;
            d.was = h.was, d.firingWas = h.firingWas, d.now = count, d.firing = firing, d.otherGun = pickrules::OtherGun(drawn, inSlot);
            d.nameWithIt = nameWithIt || h.waits >= 72;
            const pickrules::DryOut o = pickrules::SeeDry(d);
            h.waits = o.what == pickrules::DrySeen::kWaiting ? h.waits + 1 : 0;
            if (o.what == pickrules::DrySeen::kRunDry)
                h.dry = true, ++h.runDry;
            else if (o.what == pickrules::DrySeen::kReloaded)
                h.dry = false;
            else if (o.what == pickrules::DrySeen::kNewGun)
                h.dry = o.dry, ++h.newGun;
            else if (o.what == pickrules::DrySeen::kNotDrawn)
                ++h.notDrawn;
            else if (o.what == pickrules::DrySeen::kWaiting)
                ++h.waited;
            h.was = o.was;
            if (!o.keepFiring)
                h.firingWas = firing;
        };
        // The roof's take at 72 frames a second (at4_measured.md section 3): the M4A1 drawn with 31 in slot 0 (firing A); the kit's AT4 there (firing B,
        // 1 loaded) 0.1 s after the prompt while the M4A1 is drawn 0.3 s more; the AT4 drawn (no held magazine: not run); after the shot the kit's fresh
        // M4A1 (firing C, 31) under the AT4's tube half a second, then drawn; the trigger pulled: a round fired (30).
        Held h;
        for (int f = 0; f < 72; ++f)
            frame(h, m4a1, m4a1, 0xA000u, 31);
        for (int f = 0; f < 22; ++f)
            frame(h, m4a1, at4, 0xB000u, 1);
        const bool dryAtTake = h.dry;
        for (int f = 0; f < 72; ++f)
            frame(h, m4a1, m4a1, 0xC000u, 31);
        frame(h, m4a1, m4a1, 0xC000u, 30);
        std::snprintf(what, sizeof(what), "t11 the held magazine through the roof's take: the AT4's one round under the M4A1 never read as it run dry (%d run dry, %d frames of another gun in its slot), the kit's fresh M4A1 a new gun (%d) with its chamber as its count is: it fires (dry %d at the take, %d after)",
                      h.runDry, h.notDrawn, h.newGun, dryAtTake, h.dry);
        Check(h.runDry == 0 && h.notDrawn == 22 && h.newGun == 1 && !dryAtTake && !h.dry, what);
        // The same take with each swap landing between ammo's read of the slots and the hands' (their names a frame behind the firing states: the kit's
        // AT4 under the M4A1's name, then the kit's M4A1 under the AT4's): one frame waited each time, then as above. Read as the rule before this round
        // (the name taken with whatever firing state), that first frame was a new M4A1 with one round: its chamber "empty" (the review's line).
        Held r;
        for (int f = 0; f < 72; ++f)
            frame(r, m4a1, m4a1, 0xA000u, 31);
        frame(r, m4a1, m4a1, 0xB000u, 1, false);
        const bool dryWaiting = r.dry;
        for (int f = 0; f < 21; ++f)
            frame(r, m4a1, at4, 0xB000u, 1);
        const bool dryRaceTake = r.dry;
        frame(r, m4a1, at4, 0xC000u, 31, false);
        for (int f = 0; f < 71; ++f)
            frame(r, m4a1, m4a1, 0xC000u, 31);
        frame(r, m4a1, m4a1, 0xC000u, 30);
        Held before;
        for (int f = 0; f < 72; ++f)
            frame(before, m4a1, m4a1, 0xA000u, 31);
        frame(before, m4a1, m4a1, 0xB000u, 1);  // (its name a frame behind, taken as the firing state's)
        // A name never read with the firing state: decided a second on as before (hands.cpp's 72 frames); the count before kept meanwhile.
        Held s;
        frame(s, m4a1, m4a1, 0xA000u, 20);
        for (int f = 0; f < 73; ++f)
            frame(s, m4a1, m4a1, 0xD000u, 1, false);
        std::snprintf(what, sizeof(what), "t11 each swap a frame ahead of the slot's name: %d frames waited, %d run dry, %d of another gun, %d new gun (dry %d waiting, %d at the take, %d after); the rule before read it as a new M4A1 with one round, its chamber empty (%d, %d); a name never read with it: %d frames waited, then as before (new %d, dry %d)",
                      r.waited, r.runDry, r.notDrawn, r.newGun, dryWaiting, dryRaceTake, r.dry, before.newGun, before.dry, s.waited, s.newGun, s.dry);
        Check(r.waited == 2 && r.runDry == 0 && r.notDrawn == 21 && r.newGun == 1 && !dryWaiting && !dryRaceTake && !r.dry && before.newGun == 1 && before.dry &&
                  s.waited == 72 && s.newGun == 1 && s.dry,
              what);
        // The same gun as before: fired to its last round, run dry (once); a magazine in by the game's own reload (+30): its chamber filled; its slot's
        // name not known: as before.
        Held g;
        for (int n = 31; n >= 1; --n)
            frame(g, m4a1, m4a1, 0xA000u, n);
        const bool dryAfter = g.dry;
        frame(g, m4a1, m4a1, 0xA000u, 31);
        Held u;
        frame(u, m4a1, nullptr, 0xA000u, 5);
        frame(u, m4a1, nullptr, 0xA000u, 1);
        std::snprintf(what, sizeof(what), "t11 the same gun as before: fired to its last round run dry (%d, once: %d), the game's reload fills its chamber (dry %d); its slot's name not known: as before (%d)",
                      dryAfter, g.runDry, g.dry, u.dry);
        Check(dryAfter && g.runDry == 1 && !g.dry && u.dry && u.runDry == 1, what);
        // A new gun of its kind taken into the slot (another firing state) with one round: its chamber empty as its count is, no round fired.
        Held t;
        frame(t, m4a1, m4a1, 0xA000u, 20);
        frame(t, m4a1, m4a1, 0xD000u, 1);
        Check(t.dry && t.newGun == 1 && t.runDry == 0, "t11 a gun of its kind taken up with one round (another firing state): its chamber empty as its count is, no round fired");
        // OtherGun: two known rows, one row, either unknown.
        Check(pickrules::OtherGun(m4a1, at4) && !pickrules::OtherGun(m4a1, m4a1) && !pickrules::OtherGun(m4a1, static_cast<const recoilguns::Row*>(nullptr)) &&
                  !pickrules::OtherGun(static_cast<const recoilguns::Row*>(nullptr), at4) && m4a1 && at4 && m4a1 != at4,
              "t11 OtherGun: the M4A1 drawn with the AT4 in its slot two guns; the same row one; either not known one (as before)");
        // hands.cpp runs it as this loop does (word for word: no test compiles hands.cpp), and ammo.cpp's read of the slot is what the name came with.
        std::string hands = ReadText("src\\hands.cpp");
        const std::string ammo = ReadText("src\\ammo.cpp");
        hands.erase(std::remove(hands.begin(), hands.end(), '\r'), hands.end());  // (a checkout's line ends either way)
        if (hands.empty() || ammo.empty()) {
            Skip("t11 hands.cpp's held magazine lines (src\\hands.cpp or src\\ammo.cpp not there: run from the project's folder)");
        } else {
            Check(hands.find("const uint32_t dryFiring = magGun ? static_cast<uint32_t>(ammo::FiringOf(dryHandSlot)) : 0u;") != std::string::npos &&
                      hands.find("const recoilguns::Row* slotRow = ammo::RowInSlot(dryHandSlot);") != std::string::npos &&
                      hands.find("d.was = g_heldLoadedWas, d.firingWas = g_heldFiringWas, d.now = loadedNow, d.firing = dryFiring;") != std::string::npos &&
                      hands.find("d.otherGun = pickrules::OtherGun(drawnRow, slotRow);") != std::string::npos &&
                      hands.find("d.nameWithIt = ammo::SlotFiringRead(dryHandSlot) == dryFiring || s_nameWaits >= 72;") != std::string::npos &&
                      hands.find("s_nameWaits = dry.what == pickrules::DrySeen::kWaiting ? s_nameWaits + 1 : 0;") != std::string::npos &&
                      hands.find("g_heldLoadedWas = dry.was;\n            if (!dry.keepFiring)\n                g_heldFiringWas = dryFiring;") != std::string::npos &&
                      ammo.find("return slot >= 0 && slot < kSlots ? g_slots[slot].firing : 0;") != std::string::npos,
                  "t11 hands.cpp's held magazine runs SeeDry as the loop above (its firing state live, the slot's name with ammo's read of the slot, the firing state before kept while waiting, a second at most)");
        }
    }

    // ---- t12: merged with holsters by hand (2026-11-19), the owner's flow (b) on the roof: the M4A1 let go of on the floor, the AT4 taken (the kit
    // takes the lying rifle's slot and its game gun with it), fired, thrown, the kit's M4A1 back: it lies again where it lay (pickrules::LyingTaken at
    // the gift, LyingBack at the give-back, as hands.cpp's OnGunChanged runs them), never into a hand by itself; holstertest's HT rows run the flows
    // whole with holsters. ----
    {
        constexpr uint32_t kM4 = 1101, kAt4 = 1001, kMk11 = 1201;  // (profiles' ids: any three)
        pickrules::LyingTakenIn roof;
        roof.gift = true, roof.lies = true, roof.lyingId = kM4, roof.newId = kAt4, roof.lyingSlot = 0, roof.newSlot = 0;
        const auto with = [&roof](void (*change)(pickrules::LyingTakenIn&)) {
            pickrules::LyingTakenIn in = roof;
            change(in);
            return pickrules::LyingTaken(in);
        };
        Check(pickrules::LyingTaken(roof) && !with([](pickrules::LyingTakenIn& i) { i.gift = false; }) &&
                  !with([](pickrules::LyingTakenIn& i) { i.lies = false; }) && !with([](pickrules::LyingTakenIn& i) { i.retired = true; }) &&
                  !with([](pickrules::LyingTakenIn& i) { i.newId = i.lyingId; }) && !with([](pickrules::LyingTakenIn& i) { i.newSlot = 1; }) &&
                  !with([](pickrules::LyingTakenIn& i) { i.lyingId = 0; }) && !with([](pickrules::LyingTakenIn& i) { i.newId = 0; }) &&
                  !with([](pickrules::LyingTakenIn& i) { i.lyingSlot = -1, i.newSlot = -1; }),
              "t12 LyingTaken: the roof's gift (a use, or X held's R at the prompt) taking the slot of the M4A1 lying: kept; never with no gift, nothing "
              "lying, a pickup's take (the rifle into that pickup: the game's way), the same gun, another slot, a gun not known, no slot");
        pickrules::LyingBackIn back;
        back.on = true, back.id = kM4, back.slot = 0, back.at = 50000, back.newId = kM4, back.newSlot = 0, back.now = 50000 + 16000, back.distance = 0.9f;
        const auto backWith = [&back](void (*change)(pickrules::LyingBackIn&)) {
            pickrules::LyingBackIn in = back;
            change(in);
            return pickrules::LyingBack(in);
        };
        using V = pickrules::LyingBackVerdict;
        const pickrules::LyingBackOut lie = pickrules::LyingBack(back);
        const pickrules::LyingBackOut none = backWith([](pickrules::LyingBackIn& i) { i.on = false; });
        const pickrules::LyingBackOut at4 = backWith([](pickrules::LyingBackIn& i) { i.newId = kAt4; });
        const pickrules::LyingBackOut mk11 = backWith([](pickrules::LyingBackIn& i) { i.newId = kMk11, i.newSlot = 1; });
        // Through a death on its level (the review of e403947: the "RPG" checkpoint after a miss): the new life's kit of the AT4 leaves it waiting (its
        // first frame too); the kit's M4A1 back 16 s into that life (the record's time from it): laid there again.
        const pickrules::LyingBackOut lifeAt4 = backWith([](pickrules::LyingBackIn& i) { i.newId = kAt4, i.newLife = true, i.lifeAt = i.now; });
        const pickrules::LyingBackOut afterDeath = backWith([](pickrules::LyingBackIn& i) { i.lifeAt = i.at; });
        Check(lie.verdict == V::kLie && std::strcmp(lie.why, "the scene gave it back") == 0 && none.verdict == V::kNone && at4.verdict == V::kNone &&
                  mk11.verdict == V::kNone && lifeAt4.verdict == V::kNone && afterDeath.verdict == V::kLie,
              "t12 LyingBack: the kit's M4A1 back in slot 0 16 s on, 0.9 m from the eye: it lies there again (16 s into a new life on its level too); no "
              "record, the AT4 itself (a new life's kit of it too), the Mk11 drawn between: the record waits");
        struct Drop {
            void (*change)(pickrules::LyingBackIn&);
            const char* why;
        } drops[] = {
            {[](pickrules::LyingBackIn& i) { i.take = true; }, "it came back by a take, into the hand that took it"},
            {[](pickrules::LyingBackIn& i) { i.picked = true; }, "a pick of yours brought it, into the right hand"},
            {[](pickrules::LyingBackIn& i) { i.newSlot = 1; }, "the game gave it back into another slot"},
            {[](pickrules::LyingBackIn& i) { i.distance = 6.5f; }, "where it lay is too far now"},
            {[](pickrules::LyingBackIn& i) { i.distance = -1.0f; }, "where it lay isn't known now"},
            {[](pickrules::LyingBackIn& i) { i.newLife = true; }, "a new life's kit gave it, as at a spawn"},
            {[](pickrules::LyingBackIn& i) { i.lifeAt = i.now - 4000; }, "a new life's kit gave it, as at a spawn"},
            {[](pickrules::LyingBackIn& i) { i.lifeAt = i.at, i.otherLevel = true, i.newId = kAt4; }, "another level's kit: the world it lay in is gone"},
            {[](pickrules::LyingBackIn& i) { i.now = i.at + pickrules::kLyingKeepsMs + 1; }, "the scene took it too long ago"},
            {[](pickrules::LyingBackIn& i) { i.now = i.at - 1; }, "the scene took it too long ago"},
        };
        int dropped = 0;
        for (const Drop& d : drops) {
            const pickrules::LyingBackOut o = backWith(d.change);
            dropped += o.verdict == V::kDrop && std::strcmp(o.why, d.why) == 0;
        }
        const pickrules::LyingBackOut edge = backWith([](pickrules::LyingBackIn& i) { i.distance = pickrules::kLyingNear, i.now = i.at + pickrules::kLyingKeepsMs; });
        const pickrules::LyingBackOut kitEdge = backWith([](pickrules::LyingBackIn& i) { i.lifeAt = i.now - pickrules::kLyingKitMs; });
        Check(dropped == static_cast<int>(std::size(drops)) && edge.verdict == V::kLie && kitEdge.verdict == V::kLie,
              "t12 LyingBack ends the record, each said why: its gun back by a take, by a pick of the player's, into another slot, lying too far (6 m) or "
              "where not known, a new life's own kit (its first frame, or within 10 s of its start: a load of an earlier save), another level's kit (any "
              "gun), too long (120 s), a clock gone back; at 6 m and 120 s, and 10 s into a new life, still laid there");
        // hands.cpp and input.cpp run them so (word for word: no test compiles them), the record kept before the gift's own block and the give-back
        // decided before the take's (a take of its kind ends it), laid down still by StepLoose (loose::Place, loosetest g10) with the grasp kLoose and
        // no Arrive; a change of gun under way told holsters (holster.h's Frame::drawnOther, holstertest HT1, HT1b, HT5).
        std::string hands = ReadText("src\\hands.cpp"), input = ReadText("src\\input.cpp");
        hands.erase(std::remove(hands.begin(), hands.end(), '\r'), hands.end());
        input.erase(std::remove(input.begin(), input.end(), '\r'), input.end());
        if (hands.empty() || input.empty()) {
            Skip("t12 hands.cpp's and input.cpp's lines (run from the project's folder)");
        } else {
            const size_t record = hands.find("        if (pickrules::LyingTaken(lt)) {"), gift = hands.find("    if (g_useMade.side >= 0 && p.id && p.id != g_useMade.wasId && g_pick.landedSide < 0) {");
            const size_t giveBack = hands.find("        const pickrules::LyingBackOut back = pickrules::LyingBack(lb);");
            const size_t take = hands.find("    if (g_pick.landedSide >= 0 && !g_pick.sameKind && PickArrives(p.id)) {");
            // (The reviews of e403947: X held with a gun in a free hand at the prompt sends nothing, before the grenade launcher's and the shotgun's
            // branches, which press R too; with nothing in the hands, a gun coming into one bars it; X's tap's R alike.)
            const size_t refuseHeld = input.find("    if (x && !g_xHeld && now - g_xSince >= kHoldForReload && pickups::GameWouldTake() == 2 && hands::GunBarsGift(nullptr, 0)) {");
            const size_t m320 = input.find("    } else if (x && !g_xHeld && now - g_xSince >= kHoldForReload && hands::M320Held() && ammo::M320Carried()) {");
            Check(record != std::string::npos && gift != std::string::npos && record < gift && giveBack != std::string::npos && take != std::string::npos &&
                      gift < giveBack && giveBack < take &&
                      hands.find("        lt.gift = (g_useMade.side >= 0 && g_useMade.gift && nowMs - g_useMade.at <= kUseGiftMs) || input::PromptRSince() <= kUseGiftMs;") != std::string::npos &&
                      hands.find("        lb.picked = pickAt && pickAt > g_lyingGone.at && pickSlot == g_lyingGone.slot && lb.now - pickAt <= pickrules::kLyingPickMs;") != std::string::npos &&
                      hands.find("        lb.newLife = g_newLife.load(std::memory_order_relaxed);\n        lb.lifeAt = g_lyingGone.lifeAt;") != std::string::npos &&
                      hands.find("            g_lyingGone.at = g_lyingGone.lifeAt = GetTickCount64();") != std::string::npos &&
                      hands.find("            g_grasp.holder = grasp::Holder::kLoose;\n            g_grasp.trigger = -1;") != std::string::npos &&
                      hands.find("            g_holster.lyingSlot = -1;") != std::string::npos &&
                      hands.find("            g_gunRelease.want = g_gunRelease.still = true;") != std::string::npos &&
                      hands.find("                       : g_gunRelease.still        ? loose::Place(*body, kLooseGun, bone, g_gunRelease.rounds, f)") != std::string::npos &&
                      hands.find("        const bool other = p.id != 0 && slotGun != 0 && slotGun != p.id;") != std::string::npos &&
                      hands.find("        hf.drawnOther = other && now - s_sameAt <= 2000;") != std::string::npos &&
                      hands.find("                g_holsterTaken |= 1u << g_pick.landedSlot;") != std::string::npos &&
                      hands.find("    uf.point.gives = uf.point.on && up.action == pickups::kUseReload;\n    uf.gunHeld = GunInAHand(nullptr, 0);") != std::string::npos &&
                      hands.find("    return g_on && FreshFreeHands() && g_freeOn && !g_scripted.load(std::memory_order_relaxed) && GunInAHand(words, size);") != std::string::npos &&
                      hands.find("            f.arriveSide = a, f.arriveMagnet = g_pick.magnet, f.arriveUnarmed = g_pick.byUse;") != std::string::npos &&
                      refuseHeld != std::string::npos && m320 != std::string::npos && refuseHeld < m320 &&
                      input.find("            } else if (take == 2 && hands::GunBarsGift(nullptr, 0)) {\n") != std::string::npos &&
                      input.find("        if (pickups::GameWouldTake() == 2 && hands::GunBarsGift(nullptr, 0)) {\n            hands::RefuseGift(") != std::string::npos &&
                      input.find("                if (take == 2) {\n                    g_promptRAt = now;") != std::string::npos &&
                      input.find("        if (pickups::GameWouldTake() == 2) {\n            g_promptRAt = now;") != std::string::npos &&
                      input.find("            if (take == 1 && HolsteredSwap()) {") != std::string::npos &&
                      input.find("            } else if (stowed ? take == 1 || take == 2 : take != 0) {") != std::string::npos,
                  "t12 hands.cpp's lines: the record before the gift's block, the give-back before the take's (a pick only of its own slot just now, kept "
                  "through a new life), the lay-down (kLoose, holsters' lying gun read afresh, still, loose::Place), the change of gun under way for "
                  "holsters, the gift's take mark, the use barred with a gun in a hand (UseFrame, GunBarsGift), the gift unarmed in an open hand; "
                  "input.cpp's: X held with a gun in a free hand at the prompt sends nothing (before the grenade launcher's and the shotgun's R), with "
                  "nothing in the hands a gun coming into one bars it, X's tap alike; X held's R at the prompt timed both ways, its take check with "
                  "holsters' HolsteredSwap and the prompt's 2");
        }
    }

    std::printf("%d checks, %d failed, %d skipped\n", g_checks, g_failed, g_skipped);
    return g_failed;
}

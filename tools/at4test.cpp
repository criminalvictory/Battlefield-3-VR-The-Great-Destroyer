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
//       run the flows whole with holsters); the give-back's lay-down kept (LaidBack) through the "RPG" checkpoint's load frame by frame: the soldier
//       unread (the kit's seconds wait), the start kit's M4A1 a moment before the AT4 (made again), its own kind for good (over at the kit's 10 s)
//   t13 one measure for the use and the grasp (a gun lying within the grasp's reach goes first only when nearer; with no target distance GunFirst
//       within pickReach alone) and the body's places (the belly and the chests the hands', and wherever the bigger holsters keep every place out)
//   t14 the use's claim: spent from the squeeze's first frame, claimed (the other modules barred) only once its use is asked or refused, till let
//       go; a module's take before that (the handguard 0.19 s on) gives the use up, never claimed
//   t15 the integration review of d32a39b, findings 2 and 7 (usepoint.h's LyingFirst and BodyFirst, as hands.cpp's UseYields runs them; grasp.h's
//       Update with the use frame by frame): the AT4's disc unmeasured, the M4A1 lying beside the tube: a hand on the tube 4-15 cm from it (anywhere
//       along it 4-8 cm from it), at its end beyond it, 35-45 cm over it reaching down asks the AT4, only a hand on the rifle (under 3 cm) takes it; the
//       tube's capsule measured: the nearer, as before; the belly with both hands empty: the AT4's (with a gun in the other hand, and at a door, the
//       belly's); a squeeze reading exactly 0.6 the use's from that frame (the grasp takes at 0.6); hands.cpp's lines word for word
//   (t6, t14 and t15 are 0.1.14's rule, pickups.txt "usereach 1": Frame::anywhere false, their rows unchanged)
//   t16 the default, a free hand's click anywhere while the prompt shows (the owner, 2026-11-25: "I want to be able to just hold the grip button
//       instead"; 2026-11-26: "even a click will open whatever"): a hand 2.4 m off squeezed: asked the frame after (its first frame the hands'
//       modules'), its tick then; the prompt's place not known: the same; asleep (a fight's run, a new life): nothing; a grip held from before: the
//       prompt's the frame it shows, asked the next; held from a squeeze begun at something else's place or with the hand holding something: never, till
//       let go; the gun's hand and a squeeze something else's never; a click whose ask's frame falls in the game's rescan's gap: asked as it's back, and
//       one made in a gap the prompt's then; another prompt at the ask's frame: its use; a moment's failure asked again 0.5 s on, twice at most, none let
//       go, refused or "usereach 1"; one use a squeeze; the AT4 with a gun in a hand refused at the tube (the double click), away from it never the
//       prompt's, the use once both are empty; a module's take in the squeeze's first frame gives it up, one later comes after the ask; the body's places
//       only with a gun in a hand; a moment's failure by pickups.cpp's words; why a grip doesn't answer; both grips one use; a grip unread at its ask's
//       frame asked once read; "usereach 1" off reach never; hands.cpp's lines; the use test's hand a free one
//   t17 the review of babecfd, with the click: a squeeze on a lying gun by its next frame taken by the grasp (the use gives up; the grasp never told
//       busy before its ask), carried on later the click's (the door asked, the grasp told busy); carried to a module's place (the belly, the gun, a
//       holster's place) at its ask's frame: given up, never claimed, later the click's; the AT4 with a gun in a hand away from the tube never the
//       prompt's, at the tube refused at once; the grasp's busy (once asked, in the prompt's own reach from the first frame, "usereach 1" bit for bit);
//       two hands one use whatever the lag, the right held from before too, the other hand's squeeze not the prompt's while the first's is held;
//       cutscenes: no grip held from before, none held through one, a fresh squeeze yes; a shoulder's place at the ask's frame gives it up; why a grip
//       doesn't answer (both on the gun, barred, carried on, used); ByHand by the rule; hands.cpp's, input.cpp's and README.md's lines
//   t18 the grip as the interact button (the owner, 2026-11-26): the bank's breach's clicks replayed (the first one uses it, at its next frame), clicks
//       of a frame or two; the hand on the gun's front let go and squeezed again in place (its use at once, its squeeze the hands' still: never claimed
//       nor busy), not after 0.5 s, not 20 cm off, never the pistol-grip hand's, nor at the AT4's prompt with the gun in the hands, again for a second
//       click; with no prompt showing, a free hand's click taps the use key the frame after (once a click, a click of a frame, both hands one tap), never
//       one something of the hands' takes in its first frame or has by its next, one begun at a place or by a hand holding something, any bar, a
//       prompt's gap, asleep, "usereach 1" or an unread hand; TapBarOf's bars and their order; the belly with a gun in a hand only in its own zone (the
//       owner's nine squeezes at the cellar's door the door's now, the pouch's two takes the belly's); hands.cpp's, input.cpp's and pickups.cpp's lines
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
#include "../src/grasp.h"
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

// ---- usepoint.h's frames: t6, t14 and t15 are 0.1.14's rows, its reach rule ("usereach 1": Frame::anywhere false), bit for bit; t16 the default,
// a free hand's grip anywhere while the prompt shows (anywhere true) ----
usepoint::Frame UseFrameAt(const float* point, const float* left, const float* right, float leftSqueeze, float rightSqueeze, bool anywhere = false) {
    usepoint::Frame f;
    f.dt = 1.0f / 72.0f;
    f.anywhere = anywhere;
    f.point.on = true;
    std::memcpy(f.point.at, point, sizeof(f.point.at));
    const float* at[2] = {left, right};
    const float squeeze[2] = {leftSqueeze, rightSqueeze};
    for (int s = 0; s < 2; ++s) {
        f.hand[s].tracked = true;
        f.hand[s].empty = true;
        f.hand[s].clear = anywhere;  // (held anywhere: a free hand away from every module's place, as hands.cpp's UseClear has it)
        std::memcpy(f.hand[s].at, at[s], sizeof(f.hand[s].at));
        f.hand[s].squeeze = squeeze[s];
    }
    return f;
}

// ---- hands.cpp's UseYields for a gun lying on the floor (its `first`, as it runs: the grasp's own measures, judged by usepoint::LyingFirst), and one
// squeeze by the right hand at a prompt with usepoint::Update and grasp::Update together frame by frame, as FreeHands runs them (the use's spent squeeze
// is the grasp's busy hand; the hand holds what the grasp gave it). The magnet on (the game's default), the hand settled, the gun at rest. ----
constexpr float kPickReach = 0.25f, kSphere = 0.35f, kMagnetDown = 1.2f, kMagnetUp = 0.7f, kMagnetWiden = 0.2f;
bool LyingYields(const usepoint::Point& point, const float at[3], const float room[12], const float lo[3], const float hi[3], bool reachDown, float* box = nullptr,
                 float* column = nullptr) {
    float m[3];
    grasp::ToModel(room, at, m);
    const float target = usepoint::TargetDistance(point, at);
    const float d = grasp::BoxDistance(m, lo, hi);
    const bool within = d < kPickReach || d < kSphere;
    float rank = -1.0f, depth = 0.0f, off = 0.0f;
    if (!within && reachDown && !grasp::MagnetReach(at, room, lo, hi, kSphere, kMagnetDown, kMagnetUp, kMagnetWiden, rank, depth, off))
        rank = -1.0f;
    if (box)
        *box = d;
    if (column)
        *column = rank;
    return usepoint::LyingFirst(point, target, d, within, rank, kPickReach);
}
struct SqueezeRun {
    bool asked = false, taken = false, ticked = false, refused = false;
    int spentFrom = -1, takenAt = -1;
};
// The right hand at `at` squeezes at frame 5 (ramp's readings first, then 1) and holds a second; the lying gun at `room` with its box (none: lying 0).
// anywhere: usepoint.h's Frame::anywhere (false: "usereach 1", the rows as 0.1.14 had them).
SqueezeRun SqueezeAt(const usepoint::Point& point, const float at[3], bool yield, const float room[12], const float lo[3], const float hi[3], bool reachDown,
                     bool lying = true, std::initializer_list<float> ramp = {}, bool anywhere = false) {
    SqueezeRun o;
    usepoint::State us;
    grasp::State gs;
    gs.holder = grasp::Holder::kLoose, gs.trigger = -1, gs.barrel = -1;
    const float dt = 1.0f / 72.0f, away[3] = {-3.0f, 3.0f, 3.0f};
    for (int frame = 0; frame < 77; ++frame) {
        float squeeze = frame < 5 ? 0.0f : 1.0f;
        if (frame >= 5 && static_cast<size_t>(frame - 5) < ramp.size())
            squeeze = ramp.begin()[frame - 5];
        usepoint::Frame uf;
        uf.dt = dt;
        uf.anywhere = anywhere;
        uf.point = point;
        uf.gunHeld = o.taken;  // (the gun lies till a hand takes it)
        std::memcpy(uf.hand[0].at, away, sizeof(uf.hand[0].at));
        uf.hand[0].tracked = uf.hand[0].empty = true;
        usepoint::Hand& uh = uf.hand[1];
        uh.tracked = true, uh.empty = !o.taken, uh.yield = yield, uh.squeeze = squeeze;
        std::memcpy(uh.at, at, sizeof(uh.at));
        usepoint::Out uo;
        usepoint::Update(uf, us, uo);
        o.asked = o.asked || uo.use == 1;
        o.ticked = o.ticked || uo.entered[1];
        o.refused = o.refused || uo.refused[1];
        if (uo.spent[1] && o.spentFrom < 0)
            o.spentFrom = frame;
        grasp::Frame gf;
        gf.dt = dt;
        gf.enabled = true;
        gf.magnet = true;
        gf.lying = lying && !o.taken;
        gf.lyingResting = gf.lying;
        std::memcpy(gf.lyingRoom, room, sizeof(gf.lyingRoom));
        std::memcpy(gf.boxMin, lo, sizeof(gf.boxMin));
        std::memcpy(gf.boxMax, hi, sizeof(gf.boxMax));
        for (int s = 0; s < 2; ++s) {
            grasp::HandIn& h = gf.hand[s];
            h.tracked = h.own = h.read = h.settled = true;
            const float* p = s == 1 ? at : away;
            const float pose[12] = {1, 0, 0, p[0], 0, 1, 0, p[1], 0, 0, 1, p[2]};
            std::memcpy(h.pose, pose, sizeof(h.pose));
        }
        gf.hand[1].squeeze = squeeze;
        gf.hand[1].reachDown = reachDown;
        gf.hand[1].busy = uo.busy[1];  // (FreeHands: usepoint's busy is the grasp's busy hand; "usereach 1": its spent, 0.1.14's)
        const grasp::Result r = grasp::Update(gf, gs);
        if (r.pickedUp == 1 && !o.taken)
            o.taken = true, o.takenAt = frame;
    }
    return o;
}

// The review of babecfd, finding 1, with the real grasp: the right hand squeezes at frame 5 at `from` (free: no lying gun within the grasp's reaches)
// and is at `to` from frame 5 + k (over the lying gun), the use held anywhere (Frame::anywhere); its yield each frame as hands.cpp's UseYields gives it
// for a lying gun (LyingYields), the grasp's busy hand usepoint's busy (FreeHands). What came of it: taken by the grasp, the use asked, given up.
struct CarryOut {
    bool taken = false, asked = false, given = false, yielded = false, claimedBefore = false, busyBefore = false;
    int takenAt = -1;
};
CarryOut CarryTo(const usepoint::Point& point, const float from[3], const float to[3], int k, const float room[12], const float lo[3], const float hi[3]) {
    CarryOut o;
    usepoint::State us;
    grasp::State gs;
    gs.holder = grasp::Holder::kLoose, gs.trigger = -1, gs.barrel = -1;
    const float dt = 1.0f / 72.0f, away[3] = {-3.0f, 3.0f, 3.0f};
    for (int frame = 0; frame < 90; ++frame) {
        const float* at = frame < 5 + k ? from : to;
        const float squeeze = frame < 5 ? 0.0f : 1.0f;
        usepoint::Frame uf;
        uf.dt = dt;
        uf.anywhere = true;
        uf.point = point;
        uf.gunHeld = o.taken;
        std::memcpy(uf.hand[0].at, away, sizeof(uf.hand[0].at));
        uf.hand[0].tracked = uf.hand[0].empty = true;
        usepoint::Hand& uh = uf.hand[1];
        uh.tracked = true, uh.empty = !o.taken, uh.squeeze = squeeze;
        uh.clear = at == from;  // (hands.cpp's UseClear: 1 m over the gun, clear of it; on it, not)
        std::memcpy(uh.at, at, sizeof(uh.at));
        uh.yield = !o.taken && LyingYields(point, at, room, lo, hi, false);
        usepoint::Out uo;
        usepoint::Update(uf, us, uo);
        o.asked = o.asked || uo.use == 1;
        o.given = o.given || uo.given[1];
        o.yielded = o.yielded || uo.yielded[1];
        if (frame < 5 + k) {
            o.claimedBefore = o.claimedBefore || uo.claimed[1];
            o.busyBefore = o.busyBefore || uo.busy[1];
        }
        grasp::Frame gf;
        gf.dt = dt;
        gf.enabled = true;
        gf.magnet = true;
        gf.lying = !o.taken;
        gf.lyingResting = gf.lying;
        std::memcpy(gf.lyingRoom, room, sizeof(gf.lyingRoom));
        std::memcpy(gf.boxMin, lo, sizeof(gf.boxMin));
        std::memcpy(gf.boxMax, hi, sizeof(gf.boxMax));
        for (int s = 0; s < 2; ++s) {
            grasp::HandIn& h = gf.hand[s];
            h.tracked = h.own = h.read = h.settled = true;
            const float* p = s == 1 ? at : away;
            const float pose[12] = {1, 0, 0, p[0], 0, 1, 0, p[1], 0, 0, 1, p[2]};
            std::memcpy(h.pose, pose, sizeof(h.pose));
        }
        gf.hand[1].squeeze = squeeze;
        gf.hand[1].busy = uo.busy[1];
        const grasp::Result r = grasp::Update(gf, gs);
        if (r.pickedUp == 1 && !o.taken)
            o.taken = true, o.takenAt = frame;
    }
    return o;
}

// Two hands at a prompt that keeps showing after its use (maxUses 0: the kick doors, a breach door in its own scene, the seats, the rappel), with
// hands.cpp's one use at a time (g_useAsked, cleared by NoteUse the frame after the game's thread served it, serve frames on): the uses made. The left
// squeezes at frame 5 + leftAt, the right at frame 5 + rightAt (from frame 2 with heldRight: held from before the prompt, shown from frame 10).
int UsesOfTwo(int rightLag, int serve, bool heldRight = false, int leftAt = 0) {
    const float point[3] = {1.0f, 0.0f, 2.0f}, farL[3] = {-0.5f, 1.2f, 0.5f}, farR[3] = {3.0f, 1.0f, 3.0f};
    usepoint::State st;
    usepoint::Out o;
    int askedAt = -1, uses = 0;
    for (int i = 0; i < 240; ++i) {
        if (askedAt >= 0 && i - askedAt >= serve)
            ++uses, askedAt = -1;  // (made: the prompt still shows)
        usepoint::Frame f;
        f.dt = 1.0f / 72.0f;
        f.anywhere = true;
        f.point.on = heldRight ? i >= 10 : true;
        f.point.id = 7;
        std::memcpy(f.point.at, point, sizeof(f.point.at));
        const float* at[2] = {farL, farR};
        for (int s = 0; s < 2; ++s) {
            f.hand[s].tracked = f.hand[s].empty = true;
            std::memcpy(f.hand[s].at, at[s], sizeof(f.hand[s].at));
        }
        f.hand[0].squeeze = i >= 5 + leftAt ? 1.0f : 0.0f;
        f.hand[1].squeeze = (heldRight ? i >= 2 : i >= 5 + rightLag) ? 1.0f : 0.0f;
        usepoint::Update(f, st, o);
        if (o.use >= 0 && askedAt < 0)
            askedAt = i;  // (RequestUse; else "not asked: the other hand's use waits")
    }
    return uses + (askedAt >= 0);
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
        } byHand[] = {{15, 0, 0.0f, 2.0f, true},    {35, 0, 0.0f, 0.75f, true},          {35, 0, 0.0f, 3.0f, true},   {35, 0, 0.0f, 3.01f, true},
                      {35, 0, 0.0f, 5.0f, true},    {35, 0, 0.0f, 5.01f, false},         {35, 0, 0.0f, 50.0f, false},
                      {14, 0, 0.0f, 100.0f, false}, {13, 0, 1.0f, 100.0f, false},        {35, 0, 1.0f, 2.0f, false},  {35, 0, 0.0f, 300.0f, false},
                      {12, 0, 0.0f, 2.0f, false},   {15, 3, 0.0f, 2.0f, false},          {15, 0, -1.0f, 2.0f, false}, {35, 0, 0.0f, std::nanf(""), false},
                      {35, 0, 0.0f, 0.0f, false},   {7, 0, 0.0f, 1.5f, false}};
        int byHandRight = 0;
        for (const ByHandRow& r : byHand)
            byHandRight += usepoint::ByHand(r.action, r.kind, r.hold, r.radius) == r.want;
        std::snprintf(what, sizeof(what), "t6 the prompts a hand uses: %d of %zu rows as they should be (the AT4's, the doors' and Kaffarov's HALO start at 5 m yes; "
                                          "past 5 m, the jet's 50 and 300 m, pose prompts, holds, a jump, a flag, a fire, no)",
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
        // The lay-down kept (pickrules::LaidBack; the integration review of d32a39b, finding 11): laid down after a late hit at 70 s, the death at the
        // window's end 8 s on, the "RPG" checkpoint's kit (the AT4 in slot 0) read 0.3 s into the new life: the record made again. In the life it was laid
        // down in: kept, never made again (that's LyingBack's alone). The new life's kit not read yet, another gun in its slot, or its own kind (the start
        // kit's M4A1 a moment before the AT4: the review of 075277d), within 10 s: waits. Nothing carried (the soldier unread, or no kit yet): waits, the
        // kit's seconds held at now (out.lifeAt).
        pickrules::LaidBackIn laid;
        laid.on = true, laid.id = kM4, laid.giftId = kAt4, laid.slot = 0, laid.at = 70000, laid.lifeAt = 78000, laid.slotId = kAt4, laid.now = 78300;
        const auto laidWith = [&laid](void (*change)(pickrules::LaidBackIn&)) {
            pickrules::LaidBackIn in = laid;
            change(in);
            return pickrules::LaidBack(in);
        };
        using L = pickrules::LaidBackVerdict;
        const pickrules::LaidBackOut again = pickrules::LaidBack(laid);
        const pickrules::LaidBackOut sameLife = laidWith([](pickrules::LaidBackIn& i) { i.lifeAt = 0, i.now = i.at + 5000; });
        const pickrules::LaidBackOut sameLifeAt4 = laidWith([](pickrules::LaidBackIn& i) { i.lifeAt = 0; });  // (the AT4 in its slot: still that life)
        const pickrules::LaidBackOut notRead = laidWith([](pickrules::LaidBackIn& i) { i.slotId = 0; });
        const pickrules::LaidBackOut otherGun = laidWith([](pickrules::LaidBackIn& i) { i.slotId = kMk11, i.now = i.lifeAt + 9000; });
        const pickrules::LaidBackOut ownMoment = laidWith([](pickrules::LaidBackIn& i) { i.slotId = kM4; });  // (0.3 s in: the start kit's, a moment)
        const pickrules::LaidBackOut noKit = laidWith([](pickrules::LaidBackIn& i) { i.slotId = 0, i.carriesNone = true, i.now = i.lifeAt + 9000; });
        const pickrules::LaidBackOut offNone = laidWith([](pickrules::LaidBackIn& i) { i.on = false; });
        std::snprintf(what, sizeof(what), "t12 LaidBack: the give-back's lay-down after a late hit, a death, the \"RPG\" checkpoint's kit with the AT4 in slot 0: the record "
                                          "made again; in the life it was laid down in kept, never made again; a new life's kit not read yet, another gun in its slot or "
                                          "its own kind within 10 s: waits (its own kind 0.3 s in: %d); nothing carried 9 s in: waits (%d), its seconds from now (%llu, "
                                          "%llu); none kept: nothing",
                      static_cast<int>(ownMoment.verdict), static_cast<int>(noKit.verdict), noKit.lifeAt, laid.lifeAt + 9000);
        Check(again.verdict == L::kAgain && std::strcmp(again.why, "the new life's kit gives the scene's gun again in its slot") == 0 && again.lifeAt == laid.lifeAt &&
                  sameLife.verdict == L::kNone && sameLife.lifeAt == 0 && sameLifeAt4.verdict == L::kNone && notRead.verdict == L::kNone &&
                  notRead.lifeAt == laid.lifeAt && otherGun.verdict == L::kNone && ownMoment.verdict == L::kNone && noKit.verdict == L::kNone &&
                  noKit.lifeAt == laid.lifeAt + 9000 && offNone.verdict == L::kNone,
              what);
        struct LaidDrop {
            void (*change)(pickrules::LaidBackIn&);
            const char* why;
        } laidDrops[] = {
            {[](pickrules::LaidBackIn& i) { i.taken = true; }, "a take since: it, or another gun, was taken off the ground"},
            {[](pickrules::LaidBackIn& i) { i.taken = true, i.lifeAt = 0; }, "a take since: it, or another gun, was taken off the ground"},
            {[](pickrules::LaidBackIn& i) { i.picked = true, i.lifeAt = 0; }, "a pick of yours since"},
            {[](pickrules::LaidBackIn& i) { i.otherLevel = true; }, "another level's kit: the world it lay in is gone"},
            {[](pickrules::LaidBackIn& i) { i.record = true; }, "another gun that lay is recorded now"},
            {[](pickrules::LaidBackIn& i) { i.slotId = kM4, i.now = i.lifeAt + pickrules::kLyingKitMs; }, "the new life's kit gave it, as at a spawn (a later checkpoint)"},
            {[](pickrules::LaidBackIn& i) { i.slotId = 0, i.now = i.lifeAt + pickrules::kLyingKitMs; }, "the new life's kit gave the scene's gun no more in its slot"},
            {[](pickrules::LaidBackIn& i) { i.giftId = 0, i.now = i.lifeAt + pickrules::kLyingKitMs; }, "the new life's kit gave the scene's gun no more in its slot"},
            {[](pickrules::LaidBackIn& i) { i.now = i.at + pickrules::kLyingKeepsMs + 1; }, "it was laid down too long ago"},
            {[](pickrules::LaidBackIn& i) { i.slotId = 0, i.carriesNone = true, i.now = i.at + pickrules::kLyingKeepsMs + 1; }, "it was laid down too long ago"},
            {[](pickrules::LaidBackIn& i) { i.now = i.at - 1; }, "it was laid down too long ago"},
        };
        int laidDropped = 0;
        std::string laidWrong;
        for (const LaidDrop& d : laidDrops) {
            const pickrules::LaidBackOut o = laidWith(d.change);
            const bool ok = o.verdict == L::kDrop && std::strcmp(o.why, d.why) == 0;
            laidDropped += ok;
            if (!ok)
                laidWrong += std::string("; not: ") + d.why + " (" + std::to_string(static_cast<int>(o.verdict)) + ", " + o.why + ")";
        }
        const pickrules::LaidBackOut keepsEdge = laidWith([](pickrules::LaidBackIn& i) { i.now = i.at + pickrules::kLyingKeepsMs; });
        std::snprintf(what, sizeof(what), "t12 LaidBack ends it, each said why: a take since (that life or the new), a pick since, another level's kit, another record now, the new "
                                          "life's kit of its own kind through its 10 s (a later checkpoint), no scene's gun within 10 s (or its kind not known), too long "
                                          "(120 s; nothing carried too), a clock gone back; at 120 s still made again (%d of %d%s; edge %d)",
                      laidDropped, static_cast<int>(std::size(laidDrops)), laidWrong.c_str(), static_cast<int>(keepsEdge.verdict));
        Check(laidDropped == static_cast<int>(std::size(laidDrops)) && keepsEdge.verdict == L::kAgain, what);
        // Through the "RPG" checkpoint's load frame by frame, as hands.cpp asks LaidBack each frame (the review of 075277d): laid down at 70 s, the given-back
        // M4A1 in slot 0 till the death at 80 s (that life: waits); the soldier unread a while, nothing in any slot (ammo.cpp's new life once it's unread
        // over 1 s, and again at the new soldier: hands.cpp's lifeAt then; out.lifeAt kept every frame); the new soldier's slot 0 the start kit's M4A1 a
        // moment, then the AT4 (at about one load in five in the owner's logs: 19-66 ms at retries, 292-485 ms at a launch's first); 72 frames a second.
        // Made again, each, as the AT4 comes; its own kind for good (a later checkpoint): over 10 s after the new soldier, said so; a soldier never back:
        // kept till 120 s after the lay-down. (It ended at the M4A1's first frame, and with the soldier unread past 11 s the kit's seconds were over
        // before the new soldier came: the retry had nothing on the floor and the second give-back went into a hand.)
        {
            constexpr unsigned long long kForGood = ~0ull, kNever = ~0ull, kLaidAt = 70000, kDeathAt = 80000, kFrame = 14;
            constexpr unsigned long long kKit = pickrules::kLyingKitMs, kKeeps = pickrules::kLyingKeepsMs;
            struct Load {
                const char* what;
                unsigned long long gone;      // ms the soldier unread after the death (0: the new soldier at once; kNever: never back)
                unsigned long long m4;        // ms the new soldier's slot 0 holds the start kit's M4A1, then the AT4 (kForGood: the M4A1 for good)
                L want;
                unsigned long long from, to;  // its verdict this many ms after the new soldier came (one never back: after the lay-down)
                const char* why;              // ... a drop's
            };
            const char* const spawn = "the new life's kit gave it, as at a spawn (a later checkpoint)";
            const Load loads[] = {
                {"the new soldier at once, its kit read straight as the AT4", 0, 0, L::kAgain, 0, kFrame, ""},
                {"at once, the start kit's M4A1 60 ms, then the AT4", 0, 60, L::kAgain, 60, 60 + kFrame, ""},
                {"unread 9.4 s (the owner's loads), the M4A1 30 ms, then the AT4", 9400, 30, L::kAgain, 30, 30 + kFrame, ""},
                {"unread 9.4 s, the M4A1 485 ms (a launch's first load), then the AT4", 9400, 485, L::kAgain, 485, 485 + kFrame, ""},
                {"unread 12 s (a slower load), its kit read straight as the AT4", 12000, 0, L::kAgain, 0, kFrame, ""},
                {"unread 12 s, the M4A1 485 ms, then the AT4", 12000, 485, L::kAgain, 485, 485 + kFrame, ""},
                {"a later checkpoint at once: the M4A1 for good", 0, kForGood, L::kDrop, kKit, kKit + 2 * kFrame, spawn},
                {"a later checkpoint, unread 9.4 s: the M4A1 for good", 9400, kForGood, L::kDrop, kKit, kKit + 2 * kFrame, spawn},
                {"the soldier never back, nothing carried", kNever, 0, L::kDrop, kKeeps, kKeeps + kFrame, "it was laid down too long ago"},
            };
            int loadsRight = 0;
            std::string loadsWrong;
            for (const Load& l : loads) {
                pickrules::LaidBackIn in;
                in.on = true, in.id = kM4, in.giftId = kAt4, in.slot = 0, in.at = kLaidAt;
                const bool back = l.gone != kNever;
                const unsigned long long born = back ? kDeathAt + l.gone : 0;
                bool unreadLife = false, bornLife = false;
                pickrules::LaidBackOut o;
                long long at = -1;  // (ms after the new soldier, or the lay-down; under 0 before it)
                for (unsigned long long now = kLaidAt; now <= kLaidAt + 2 * kKeeps; now += kFrame) {
                    const bool alive = now < kDeathAt, soldier = back && now >= born;
                    if (!alive && !soldier && !unreadLife && now - kDeathAt > 1000)
                        unreadLife = true, in.lifeAt = now;  // (unread over 1 s: a new life)
                    if (soldier && !bornLife)
                        bornLife = true, in.lifeAt = now;  // (the new soldier: a new life)
                    in.carriesNone = !alive && !soldier;
                    in.slotId = alive ? kM4 : !soldier ? 0 : l.m4 == kForGood || now - born < l.m4 ? kM4 : kAt4;
                    in.now = now;
                    o = pickrules::LaidBack(in);
                    in.lifeAt = o.lifeAt;  // (hands.cpp's g_laidBack.lifeAt)
                    if (o.verdict != L::kNone) {
                        at = static_cast<long long>(now) - static_cast<long long>(back ? born : kLaidAt);
                        break;
                    }
                }
                const bool ok = o.verdict == l.want && (l.want != L::kDrop || std::strcmp(o.why, l.why) == 0) && at >= static_cast<long long>(l.from) &&
                                at <= static_cast<long long>(l.to);
                loadsRight += ok;
                if (!ok)
                    loadsWrong += std::string("; ") + l.what + ": " + std::to_string(static_cast<int>(o.verdict)) + " (" + o.why + ") at " + std::to_string(at) + " ms";
            }
            std::snprintf(what, sizeof(what), "t12 LaidBack through the \"RPG\" checkpoint's load frame by frame: the start kit's M4A1 60, 30 or 485 ms before the AT4, the "
                                              "soldier unread 9.4 or 12 s first: made again as the AT4 comes; the M4A1 for good: over 10 s after the new soldier (a later "
                                              "checkpoint); a soldier never back: kept till 120 s after the lay-down (%d of %d%s)",
                          loadsRight, static_cast<int>(std::size(loads)), loadsWrong.c_str());
            Check(loadsRight == static_cast<int>(std::size(loads)), what);
        }
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

    // ---- t13: one measure for the use and the grasp (the audit of 2026-11-19): flow (b) on the roof, the M4A1 dropped, then a hand reached for the AT4.
    // The use's target by its capsule (the tube where it lies, here as a census would give it) or its point; a gun lying within the grasp's reach goes
    // first only when nearer (usepoint::TargetDistance, GunFirst); a target whose place isn't measured (the AT4's own disc today) only within pickReach,
    // as before. The belly and the chests yield whatever holsters say (holster::AtBodyTake). ----
    {
        char what[640];
        usepoint::Point tube;
        tube.on = true;
        const float pointAt[3] = {0.0f, 0.0f, 0.0f};
        std::memcpy(tube.at, pointAt, sizeof(tube.at));
        tube.disc = 1.0f;
        tube.capsule = true, tube.radius = 0.055f;
        const float ta[3] = {0.65f, 0.05f, -0.45f}, tb[3] = {0.65f, 0.05f, 0.55f};  // (the tube along z, 0.65 m east of the point, on the floor)
        std::memcpy(tube.a, ta, sizeof(tube.a)), std::memcpy(tube.b, tb, sizeof(tube.b));
        // The rifle lying 15 cm east of the tube, parallel (its box's nearest face 15 cm from the tube's surface).
        const float handOnTube[3] = {0.65f, 0.10f, 0.0f}, handOnRifle[3] = {0.86f, 0.10f, 0.0f};
        const float onTubeTarget = usepoint::TargetDistance(tube, handOnTube), onRifleTarget = usepoint::TargetDistance(tube, handOnRifle);
        const float rifleFromTubeHand = 0.86f - 0.65f - 0.06f, rifleFromRifleHand = 0.0f;  // (the rifle's box from each hand: 15 cm, 0)
        const bool tubeAsks = !usepoint::GunFirst(rifleFromTubeHand, onTubeTarget, 0.25f), rifleTakes = usepoint::GunFirst(rifleFromRifleHand, onRifleTarget, 0.25f);
        std::snprintf(what, sizeof(what), "t13 the rifle 15 cm from the tube: a hand on the tube asks the AT4 (the tube %.3f m, the rifle %.2f: %d), a hand on the "
                                          "rifle takes the rifle (it %.2f m, the tube %.3f: %d)",
                      onTubeTarget, rifleFromTubeHand, tubeAsks, rifleFromRifleHand, onRifleTarget, rifleTakes);
        Check(tubeAsks && rifleTakes && onTubeTarget == 0.0f, what);
        // Within the magnet's sphere (30 cm, past pickReach) and nearer than the tube: the rifle (the magnet no longer blocked by the use); a tie: the prompt.
        const bool sphere = usepoint::GunFirst(0.30f, 0.45f, 0.25f) && !usepoint::GunFirst(0.30f, 0.20f, 0.25f) && !usepoint::GunFirst(0.20f, 0.20f, 0.25f);
        std::snprintf(what, sizeof(what), "t13 ... past pickReach in the magnet's reach: nearer than the tube the rifle's, farther the prompt's, a tie the prompt's (%d)", sphere);
        Check(sphere, what);
        // Not measured (the AT4's own disc, no capsule): no target distance; GunFirst then within pickReach alone (any prompt but one that gives a gun:
        // that one goes by LyingFirst, t15, a hand practically on the gun).
        usepoint::Point disc = tube;
        disc.capsule = false;
        const float unknown = usepoint::TargetDistance(disc, handOnTube);
        const bool asBefore = unknown < 0.0f && usepoint::GunFirst(0.20f, unknown, 0.25f) && !usepoint::GunFirst(0.30f, unknown, 0.25f);
        std::snprintf(what, sizeof(what), "t13 ... the AT4's own disc unmeasured: no target distance (%.0f); GunFirst with none, within pickReach alone (%d; a prompt "
                                          "that gives a gun goes by LyingFirst: t15)", unknown, asBefore);
        Check(asBefore, what);
        // A door's point (the plain disc): across from the point.
        usepoint::Point door;
        door.on = true;
        std::memcpy(door.at, pointAt, sizeof(door.at));
        const float nearDoor[3] = {0.30f, 0.9f, 0.40f};
        const float doorTarget = usepoint::TargetDistance(door, nearDoor);
        const float nan3[3] = {std::nanf(""), 0.0f, 0.0f};
        std::snprintf(what, sizeof(what), "t13 ... a door's point: across from it (%.3f, 0.5); a hand not a number: none (%.0f); a gun not a number never first (%d)", doorTarget,
                      usepoint::TargetDistance(door, nan3), usepoint::GunFirst(std::nanf(""), 0.4f, 0.25f));
        Check(std::fabs(doorTarget - 0.5f) < 1e-4f && usepoint::TargetDistance(door, nan3) < 0.0f && !usepoint::GunFirst(std::nanf(""), 0.4f, 0.25f), what);
        // The body's places whatever holsters say: the belly's zone (grown 5 cm) and the chests', yield; a shoulder, the hand out in front, not this test's.
        const float belly[3] = {0.0f, -0.43f, 0.13f}, bellyEdge[3] = {0.0f, -0.43f, 0.29f}, chestL[3] = {-0.08f, -0.18f, 0.12f}, chestR[3] = {0.08f, -0.18f, 0.12f};
        const float shoulder[3] = {0.20f, -0.025f, -0.03f}, front[3] = {0.0f, -0.30f, 0.45f};
        const float nanP[3] = {std::nanf(""), 0.0f, 0.0f};
        std::snprintf(what, sizeof(what), "t13 the body test: the belly (%d), its edge grown 5 cm (16 cm out: %d), the left chest (%d), the right (%d) yield; a shoulder "
                                          "(%d, holsters' own), the hand 45 cm out in front (%d), not a number (%d) don't",
                      holster::AtBodyTake(belly), holster::AtBodyTake(bellyEdge), holster::AtBodyTake(chestL), holster::AtBodyTake(chestR), holster::AtBodyTake(shoulder),
                      holster::AtBodyTake(front), holster::AtBodyTake(nanP));
        Check(holster::AtBodyTake(belly) && holster::AtBodyTake(bellyEdge) && holster::AtBodyTake(chestL) && holster::AtBodyTake(chestR) &&
                  !holster::AtBodyTake(shoulder) && !holster::AtBodyTake(front) && !holster::AtBodyTake(nanP),
              what);
        // With the holsters' settings (the bigger holsters' KeptOut, merged 2026-11-25): wherever they keep every place out too, the belly's 7 cm past
        // its zone at the default keepOut: 6 cm out in front of it the plain test says no, the holsters' says yes; a shoulder and the front still no.
        const holster::Settings hs;
        const float bellyFar[3] = {0.0f, -0.43f, 0.31f};
        std::snprintf(what, sizeof(what), "t13 ... with the holsters' settings: 6 cm past the belly's zone in front (plain %d, the holsters' KeptOut %d, together %d); a "
                                          "shoulder (%d), the hand 45 cm out (%d), not a number (%d) still no",
                      holster::AtBodyTake(bellyFar), holster::KeptOut(bellyFar, hs), holster::AtBodyTake(bellyFar, hs), holster::AtBodyTake(shoulder, hs),
                      holster::AtBodyTake(front, hs), holster::AtBodyTake(nanP, hs));
        Check(!holster::AtBodyTake(bellyFar) && holster::KeptOut(bellyFar, hs) && holster::AtBodyTake(bellyFar, hs) && holster::AtBodyTake(belly, hs) &&
                  !holster::AtBodyTake(shoulder, hs) && !holster::AtBodyTake(front, hs) && !holster::AtBodyTake(nanP, hs),
              what);
    }

    // ---- t14: the use's claim (Out::claimed, hands.cpp's g_useOwnsBy; the review of 2026-11-25, finding 1). A squeeze begun in a prompt's reach is the
    // use's (spent) from its first frame, but the hands' other modules are barred from it only once its use is asked or refused (claimed): before that a
    // module's take wins and the use gives up, as usepoint.h has always said (the handguard squeezed 35 cm out and brought to it within 0.2 s, the belly's
    // 0.4 s window). Claimed till let go. ----
    {
        const float point[3] = {1.0f, 0.0f, 2.0f}, in1[3] = {1.3f, 0.1f, 2.2f}, away[3] = {3.0f, 1.0f, 3.0f};
        usepoint::State st;
        usepoint::Out o;
        // The hand on its way to the handguard: squeezed in reach at frame 5, the handguard taken at frame 19 (0.19 s on: not empty any more).
        int spentFrom = -1, claimedEver = 0, givenAt = -1, usedAt = -1;
        for (int i = 0; i < 40; ++i) {
            usepoint::Frame f = UseFrameAt(point, in1, away, i >= 5 ? 1.0f : 0.0f, 0.0f);
            f.hand[0].empty = i < 19;
            usepoint::Update(f, st, o);
            if (o.spent[0] && spentFrom < 0)
                spentFrom = i;
            claimedEver += o.claimed[0];
            if (o.given[0] && givenAt < 0)
                givenAt = i;
            if (o.use == 0 && usedAt < 0)
                usedAt = i;
        }
        std::snprintf(what, sizeof(what), "t14 squeezed in reach, the handguard taken 0.19 s on: spent from frame %d (5), never claimed (%d frames), given up at %d (19), "
                                          "never used (%d)",
                      spentFrom, claimedEver, givenAt, usedAt);
        Check(spentFrom == 5 && claimedEver == 0 && givenAt == 19 && usedAt == -1, what);
        // Held in reach, empty: used at frame 22 (0.25 s), claimed from that frame through the squeeze, out of reach too, none once let go.
        usepoint::Clear(st);
        int claimedFrom = -1, claimedFrames = 0, used = -1, claimedAfter = 0;
        for (int i = 0; i < 70; ++i) {
            const float* at = i < 40 ? in1 : away;
            usepoint::Update(UseFrameAt(point, at, away, i >= 5 && i < 60 ? 1.0f : 0.0f, 0.0f), st, o);
            if (o.claimed[0] && claimedFrom < 0)
                claimedFrom = i;
            claimedFrames += o.claimed[0] && i < 60;
            claimedAfter += o.claimed[0] && i >= 60;
            if (o.use == 0 && used < 0)
                used = i;
        }
        // A prompt that gives a gun, a gun in a hand: refused at 22, claimed from then (the refusal's squeeze takes nothing else either).
        usepoint::Clear(st);
        int refusedAt = -1, refusedClaimedFrom = -1;
        for (int i = 0; i < 40; ++i) {
            usepoint::Frame f = UseFrameAt(point, in1, away, i >= 5 ? 1.0f : 0.0f, 0.0f);
            f.point.gives = true;
            f.gunHeld = true;
            usepoint::Update(f, st, o);
            if (o.refused[0] && refusedAt < 0)
                refusedAt = i;
            if (o.claimed[0] && refusedClaimedFrom < 0)
                refusedClaimedFrom = i;
        }
        std::snprintf(what, sizeof(what), "t14 ... held in reach, empty: used at %d (22), claimed from %d (22) for %d frames (38, out of reach too), after the let-go %d (0); "
                                          "refused at %d (22), claimed from %d (22)",
                      used, claimedFrom, claimedFrames, claimedAfter, refusedAt, refusedClaimedFrom);
        Check(used == 22 && claimedFrom == 22 && claimedFrames == 38 && claimedAfter == 0 && refusedAt == 22 && refusedClaimedFrom == 22, what);
    }

    // ---- t15: the integration review of d32a39b, findings 2 and 7 (usepoint.h's LyingFirst and BodyFirst, as hands.cpp's UseYields runs them) and its
    // squeeze at exactly 0.6. Flow (b) on the roof with the AT4's own 1 m disc unmeasured (no capsule: TargetDistance -1): the M4A1 let go of lying
    // parallel to the tube on the floor (the review's probe: the tube along z 0.65 m east of the point, as t13 lays it); one squeeze by the right hand, the
    // use and the grasp together. A hand on the tube, the rifle 4-15 cm beside it: the AT4 (it was the rifle, by pickReach; with 8 cm, the review of
    // 075277d, still the rifle at 1-8 cm); anywhere along the tube with the rifle 4-8 cm beside it: the AT4; at the tube's end beyond the rifle's: the
    // AT4; 35 cm over the rifle and 10 cm off it, and 45 cm straight over it, reaching down: the AT4 (the magnet's column took the rifle); on the rifle
    // (its box within 3 cm: dropped 1 cm beside the tube, 2 cm off its side) or in it: the rifle; 70 cm off it: the AT4. With the tube's capsule
    // measured: the nearer, the column too, as before.
    // The body's places (finding 7), standing 1 m from the point, the right hand at the belly: with both hands empty the AT4 (no tick and no use before);
    // with a gun in the other hand the belly keeps the squeeze (the barred prompt never refuses a magazine's squeeze); a door's prompt: the belly's. A
    // squeeze reading exactly 0.6 a frame (the grasp takes at 0.6): the use's from that frame, the rifle 25 cm off not taken (the magnet's sphere took it). ----
    {
        usepoint::Point disc;
        disc.on = true, disc.gives = true, disc.disc = 1.0f;
        disc.at[0] = 0.0f, disc.at[1] = 0.05f, disc.at[2] = 0.0f;  // (the AT4's use point on the floor, the room's y up)
        // The M4A1's box (x -0.06..0.06 round its middle, the floor y 0..0.10, z -0.40..0.45), lying turned as the room, its middle at x.
        const float lo[3] = {-0.06f, 0.0f, -0.40f}, hi[3] = {0.06f, 0.10f, 0.45f};
        const auto rifleAt = [](float x, float room[12]) {
            const float m[12] = {1, 0, 0, x, 0, 1, 0, 0, 0, 0, 1, 0};
            std::memcpy(room, m, sizeof(m));
        };
        struct Case {
            const char* what;
            float rifle;   // the rifle's middle's x (the tube at 0.65)
            float at[3];   // the hand
            bool down;     // reaching down
            bool rifles;   // the rifle's squeeze (else the AT4's)
        };
        const Case cases[] = {
            {"on the tube, the rifle 9 cm beside it", 0.65f + 0.09f + 0.06f, {0.65f, 0.10f, 0.0f}, false, false},
            {"on the tube, the rifle 10 cm beside it", 0.65f + 0.10f + 0.06f, {0.65f, 0.10f, 0.0f}, false, false},
            {"on the tube, the rifle 15 cm beside it", 0.86f, {0.65f, 0.10f, 0.0f}, false, false},
            {"at the tube's end beyond the rifle's (18 cm)", 0.86f, {0.65f, 0.10f, 0.55f}, false, false},
            {"35 cm over the rifle, 10 cm off its side, reaching down", 0.86f, {0.70f, 0.45f, 0.0f}, true, false},
            {"45 cm straight over the rifle, reaching down", 0.86f, {0.86f, 0.55f, 0.0f}, true, false},
            {"on the tube, the rifle 7 cm beside it", 0.65f + 0.07f + 0.06f, {0.65f, 0.10f, 0.0f}, false, false},
            {"on the tube, the rifle 5 cm beside it", 0.65f + 0.05f + 0.06f, {0.65f, 0.10f, 0.0f}, false, false},
            {"on the tube, the rifle 4 cm beside it", 0.65f + 0.04f + 0.06f, {0.65f, 0.10f, 0.0f}, false, false},
            {"on the tube, the rifle dropped 1 cm beside it (on the rifle: still the rifle)", 0.65f + 0.01f + 0.06f, {0.65f, 0.10f, 0.0f}, false, true},
            {"on the rifle's side (2 cm from its box)", 0.86f, {0.78f, 0.10f, 0.0f}, false, true},
            {"on the rifle", 0.86f, {0.86f, 0.08f, 0.0f}, false, true},
            {"70 cm off the rifle, in the disc", 0.86f, {-0.30f, 0.10f, 0.0f}, false, false},
        };
        int right = 0;
        std::string wrong;
        for (const Case& c : cases) {
            float room[12], box = 0.0f, column = 0.0f;
            rifleAt(c.rifle, room);
            const bool yield = LyingYields(disc, c.at, room, lo, hi, c.down, &box, &column);
            const SqueezeRun r = SqueezeAt(disc, c.at, yield, room, lo, hi, c.down);
            const bool ok = usepoint::Reach(disc, c.at) && (c.rifles ? r.taken && !r.asked : r.asked && !r.taken);
            right += ok;
            if (!ok) {
                char line[200];
                std::snprintf(line, sizeof(line), "; %s (box %.3f m, column %.3f): yield %d, the AT4 asked %d, the rifle taken %d", c.what, box, column, yield, r.asked,
                              r.taken);
                wrong += line;
            }
        }
        std::snprintf(what, sizeof(what), "t15 the AT4's disc unmeasured, the M4A1 lying beside the tube: on the tube 4-15 cm from it, at its end beyond it, 35 and 45 cm "
                                          "over it reaching down: the AT4; 1 or 2 cm from it, or on it: the rifle; 70 cm off: the AT4 (%d of %d%s)",
                      right, static_cast<int>(std::size(cases)), wrong.c_str());
        Check(right == static_cast<int>(std::size(cases)), what);
        // Anywhere along the tube (21 grips, z -0.45 to 0.55 by 5 cm) with the rifle 4-8 cm beside it: the AT4 at every one (the review of 075277d: with
        // 8 cm, 18-20 of the 21 took the rifle 1-8 cm beside the tube, and a rifle let go of at the tube lands by the hand).
        int sweepAsked = 0, sweepAll = 0;
        std::string sweepLine;
        for (int cm = 4; cm <= 8; ++cm) {
            float room[12];
            rifleAt(0.65f + 0.01f * static_cast<float>(cm) + 0.06f, room);
            int asked = 0;
            for (int k = 0; k <= 20; ++k) {
                const float at[3] = {0.65f, 0.10f, -0.45f + 0.05f * static_cast<float>(k)};
                const bool yield = LyingYields(disc, at, room, lo, hi, false);
                const SqueezeRun r = SqueezeAt(disc, at, yield, room, lo, hi, false);
                asked += usepoint::Reach(disc, at) && r.asked && !r.taken;
            }
            sweepAsked += asked, sweepAll += 21;
            sweepLine += " " + std::to_string(cm) + " cm " + std::to_string(asked);
        }
        std::snprintf(what, sizeof(what), "t15 ... the rifle 4-8 cm beside the tube, a hand anywhere along it (21 grips each): the AT4 at every one (%d of %d:%s)",
                      sweepAsked, sweepAll, sweepLine.c_str());
        Check(sweepAsked == sweepAll, what);
        // Measured (the tube's capsule, as a census would give it): the nearer goes first, as before (the hand on the tube: the AT4; 45 cm over the rifle,
        // the tube 49 cm off: the rifle by the column).
        usepoint::Point measured = disc;
        measured.capsule = true, measured.radius = 0.055f;
        const float ta[3] = {0.65f, 0.05f, -0.45f}, tb[3] = {0.65f, 0.05f, 0.55f};
        std::memcpy(measured.a, ta, sizeof(measured.a)), std::memcpy(measured.b, tb, sizeof(measured.b));
        float room[12];
        rifleAt(0.86f, room);
        const float onTube[3] = {0.65f, 0.10f, 0.0f}, overRifle[3] = {0.86f, 0.55f, 0.0f};
        const bool tubeYield = LyingYields(measured, onTube, room, lo, hi, false), overYield = LyingYields(measured, overRifle, room, lo, hi, true);
        const SqueezeRun tubeRun = SqueezeAt(measured, onTube, tubeYield, room, lo, hi, false), overRun = SqueezeAt(measured, overRifle, overYield, room, lo, hi, true);
        std::snprintf(what, sizeof(what), "t15 ... the tube's capsule measured: on the tube the AT4 (asked %d, taken %d); 45 cm over the rifle, the tube 49 cm off: the "
                                          "rifle by the column (yield %d, taken %d)",
                      tubeRun.asked, tubeRun.taken, overYield, overRun.taken);
        Check(!tubeYield && tubeRun.asked && !tubeRun.taken && overYield && overRun.taken && !overRun.asked, what);
        // The body's places (finding 7): standing (the eye 1.60 m over the roof, the neck point 1.525), facing the point 1.0 m ahead (-z) on the floor; the
        // right hand at the belly's middle (0, -0.43, 0.13 from the neck point), in the AT4's reach. hands.cpp's yield: BodyFirst && AtBodyTake.
        usepoint::Point ahead = disc;
        ahead.at[0] = 0.0f, ahead.at[1] = 0.0086f, ahead.at[2] = -1.0f;
        const float neck[3] = {0.0f, 1.525f, 0.0f}, belly[3] = {0.0f, 1.095f, -0.13f};
        float fromNeck[3] = {};
        const holster::Settings hs;
        const bool atBody = grasp::FromNeck(belly, neck, 0.0f, fromNeck) && holster::AtBodyTake(fromNeck, hs);
        usepoint::Point door = ahead;
        door.gives = false;
        const bool emptyYield = usepoint::BodyFirst(ahead, false) && atBody, heldYield = usepoint::BodyFirst(ahead, true) && atBody,
                   doorYield = usepoint::BodyFirst(door, false) && atBody;
        const SqueezeRun empty = SqueezeAt(ahead, belly, emptyYield, room, lo, hi, false, false);
        usepoint::State hs2;
        usepoint::Out ho;
        bool heldRefused = false, heldSpent = false;
        for (int i = 0; i < 60; ++i) {  // (a gun in the other hand: the prompt barred; the right's squeeze at the belly)
            usepoint::Frame f = UseFrameAt(ahead.at, belly, belly, 0.0f, i >= 5 ? 1.0f : 0.0f);
            f.point = ahead;
            f.gunHeld = true;
            f.hand[0].tracked = false;
            f.hand[1].yield = heldYield;
            usepoint::Update(f, hs2, ho);
            heldRefused = heldRefused || ho.refused[1];
            heldSpent = heldSpent || ho.spent[1];
        }
        const SqueezeRun doorRun = SqueezeAt(door, belly, doorYield, room, lo, hi, false, false);
        std::snprintf(what, sizeof(what), "t15 ... the right hand at the belly (in the body's places %d, in the AT4's reach %d), standing 1 m from it: both hands empty, "
                                          "the AT4 (yield %d, tick %d, asked %d); a gun in the other hand, the belly's (yield %d, spent %d, refused %d); a door's "
                                          "prompt, the belly's (yield %d, asked %d)",
                      atBody, usepoint::Reach(ahead, belly), emptyYield, empty.ticked, empty.asked, heldYield, heldSpent, heldRefused, doorYield, doorRun.asked);
        Check(atBody && usepoint::Reach(ahead, belly) && !emptyYield && empty.ticked && empty.asked && heldYield && !heldSpent && !heldRefused && doorYield &&
                  !doorRun.asked,
              what);
        // The squeeze reading exactly 0.6 a frame (grasp.h takes at 0.6 or more, usepoint.h counted it down only past 0.6): the hand on the tube's far side,
        // the rifle's box 25 cm off (in the magnet's sphere, not first): the use's from the 0.6 frame, the AT4 asked, the rifle not taken; readings that
        // never are 0.6 (0.25, 0.5, 0.75): the same.
        rifleAt(0.86f, room);
        const float farSide[3] = {0.55f, 0.10f, 0.0f};
        float farBox = 0.0f;
        const bool farYield = LyingYields(disc, farSide, room, lo, hi, false, &farBox);
        const SqueezeRun exact = SqueezeAt(disc, farSide, farYield, room, lo, hi, false, true, {0.3f, 0.6f, 0.6f, 0.6f, 0.6f});
        const SqueezeRun steps = SqueezeAt(disc, farSide, farYield, room, lo, hi, false, true, {0.25f, 0.5f, 0.75f});
        std::snprintf(what, sizeof(what), "t15 ... a squeeze reading exactly 0.6 (the rifle's box %.2f m off, yield %d): the use's from frame %d (6), the AT4 asked %d, the "
                                          "rifle taken %d (at %d); 0.25, 0.5, 0.75: from %d (7), asked %d, taken %d",
                      farBox, farYield, exact.spentFrom, exact.asked, exact.taken, exact.takenAt, steps.spentFrom, steps.asked, steps.taken);
        Check(!farYield && exact.spentFrom == 6 && exact.asked && !exact.taken && steps.spentFrom == 7 && steps.asked && !steps.taken, what);
        // hands.cpp's UseYields and UseFrame as the rows above run them, word for word (no test compiles hands.cpp).
        std::string hands = ReadText("src\\hands.cpp");
        hands.erase(std::remove(hands.begin(), hands.end(), '\r'), hands.end());
        if (hands.empty()) {
            Skip("t15 hands.cpp's lines (run from the project's folder)");
        } else {
            const char* const lines[] = {
                "bool UseYields(int side, const grasp::Frame& f, const grasp::HandIn& hand, const holster::Result& hr, const usepoint::Point& point, bool gunHeld,\n"
                "               bool anywhere, const char** why = nullptr) {",
                "    if (usepoint::BodyKeeps(point, gunHeld, anywhere)) {\n        float neck[3], yaw = 0.0f, fromNeck[3];\n        if (pouch::BodyFrame(neck, yaw) && "
                "grasp::FromNeck(at, neck, yaw, fromNeck) &&\n            (anywhere ? usepoint::BellyTakes(fromNeck) : holster::AtBodyTake(fromNeck, g_holsterSet)))\n"
                "            return yields(",
                "        const bool within = d < reach || (f.magnet && hand.settled && atRest && d < sphere);\n        float rank = -1.0f, depth = 0.0f, off = 0.0f;\n"
                "        if (!within && f.magnet && hand.reachDown && hand.settled && atRest &&\n            !grasp::MagnetReach(",
                "            rank = -1.0f;\n        return usepoint::LyingFirst(point, target, d, within, rank, reach);\n    };",
                "        const bool yieldWanted = uf.point.on || (!was.down && in.squeeze >= usepoint::kSqueezeOn) || (was.owned && !was.asked) || was.tap;\n"
                "        u.yield = yieldWanted && UseYields(s, f, in, hr, uf.point, uf.gunHeld, anywhere, &yieldWhy[s]);",
            };
            int found = 0;
            for (const char* l : lines)
                found += hands.find(l) != std::string::npos;
            std::snprintf(what, sizeof(what), "t15 hands.cpp's lines: UseYields given the gun in a hand, its body test by BodyFirst, each lying gun by LyingFirst (%d of %d)",
                          found, static_cast<int>(std::size(lines)));
            Check(found == static_cast<int>(std::size(lines)), what);
        }
    }

    // ---- t16: a free hand's grip anywhere while the prompt shows (usepoint.h's Frame::anywhere, the default; the owner, 2026-11-25: "holding grip down if
    // you needed to open a door and it said hold R or E to open. I want to be able to just hold the grip button instead"). At 72 Hz. ----
    {
        const float point[3] = {1.0f, 0.0f, 2.0f}, far1[3] = {-0.5f, 1.2f, 0.5f}, away[3] = {3.0f, 1.0f, 3.0f};
        const float nanAt[3] = {std::nanf(""), std::nanf(""), std::nanf("")};
        usepoint::State st;
        usepoint::Out o;
        // A run: the left hand at `left` (the right away, open), its grip by frame from `grip`, the frame changed by `edit`; what came of it.
        struct Run {
            int tookAt = -1, askedAt = -1, asks = 0, tickAt = -1, ticks = 0, other = 0, spentFrames = 0, claimedFrames = 0, givenAt = -1, refusedAt = -1;
            int doubles = 0, secondAt = -1;
            bool heldInto = false;
            float tickAmp = 0.0f;
            std::vector<int> askFrames;
        };
        const auto run = [&](int frames, const float* left, const auto& grip, const auto& edit, const auto& after) {
            Run r;
            usepoint::Clear(st);
            for (int i = 0; i < frames; ++i) {
                usepoint::Frame f = UseFrameAt(point, left, away, grip(i), 0.0f, true);
                edit(i, f);
                usepoint::Update(f, st, o);
                if (o.took[0] && r.tookAt < 0)
                    r.tookAt = i, r.heldInto = o.heldInto[0];
                if (o.use == 0) {
                    ++r.asks;
                    r.askFrames.push_back(i);
                    if (r.askedAt < 0)
                        r.askedAt = i;
                }
                if (o.entered[0]) {
                    ++r.ticks;
                    if (r.tickAt < 0)
                        r.tickAt = i, r.tickAmp = o.pulse[0].amplitude;
                }
                if (o.given[0] && r.givenAt < 0)
                    r.givenAt = i;
                if (o.refused[0] && r.refusedAt < 0)
                    r.refusedAt = i;
                if (o.pulse[0].amplitude == usepoint::kDoubleAmp && o.pulse[0].seconds > 0.0f) {
                    ++r.doubles;
                    if (r.doubles == 2)
                        r.secondAt = i;
                }
                r.spentFrames += o.spent[0];
                r.claimedFrames += o.claimed[0];
                r.other += o.use == 1 || o.spent[1] || o.entered[1] || o.took[1] || o.refused[1];
                after(i);
            }
            return r;
        };
        const auto none = [](int, usepoint::Frame&) {};
        const auto nothing = [](int) {};
        const auto from5 = [](int i) { return i >= 5 ? 1.0f : 0.0f; };
        // a. A free hand 2.4 m from the point, out of any reach and clear of every module's place, still, squeezed at frame 5 and held: the prompt's from
        // that frame (fresh), its use asked once it has been held kStillFor still and clear (frame 12, 0.1 s; till 2026-11-26 at 37, 0.45 s on; 01a5132's
        // the frame after, 6) with a tick on that hand then (0.25 for 20 ms) and none before, once a squeeze; the other hand never anything.
        const Run a = run(80, far1, from5, none, nothing);
        std::snprintf(what, sizeof(what), "t16 a free hand 2.4 m from the point squeezed while the prompt shows: the prompt's at frame %d (5, fresh %d), asked at %d (12), %d "
                                          "time(s) (1); its tick at %d (12, %.2f), %d in all (1); the other hand nothing (%d)",
                      a.tookAt, !a.heldInto, a.askedAt, a.asks, a.tickAt, static_cast<double>(a.tickAmp), a.ticks, a.other);
        Check(a.tookAt == 5 && !a.heldInto && a.askedAt == 12 && a.asks == 1 && a.tickAt == 12 && a.tickAmp == usepoint::kTickAmp && a.ticks == 1 && a.other == 0,
              what);
        // b. The prompt's place not known (not a number): the same.
        const Run b = run(80, far1, from5, [&](int, usepoint::Frame& f) { std::memcpy(f.point.at, nanAt, sizeof(f.point.at)); }, nothing);
        // ... and in a scripted moment or with free hands asleep (no bar now: Frame::asleep is only a new life's first frame, a switch's bridging frames,
        // a fist fight's run): asleep throughout, nothing.
        const Run bAsleep = run(80, far1, from5, [](int, usepoint::Frame& f) { f.asleep = true; }, nothing);
        std::snprintf(what, sizeof(what), "t16 ... its place not known: asked at %d (12); asleep (a fight's run, a new life's first frame): asked %d, spent %d frames (none)",
                      b.askedAt, bAsleep.asks, bAsleep.spentFrames);
        Check(b.askedAt == 12 && b.asks == 1 && bAsleep.asks == 0 && bAsleep.spentFrames == 0, what);
        // c. A grip held from before (squeezed from frame 2, free), the prompt showing from frame 10: the prompt's once it has shown kHeldIntoShown (20,
        // 0.15 s: a door's edge walked past with a fist held never opens it; till 2026-11-26 0.4 s, at 38; 01a5132's the frame it showed), asked then (its
        // hand still and clear since frame 2); nothing before.
        const auto from2 = [](int i) { return i >= 2 ? 1.0f : 0.0f; };
        const Run c = run(90, far1, from2, [](int i, usepoint::Frame& f) { f.point.on = i >= 10; }, nothing);
        int spentBefore = 0;
        {
            usepoint::Clear(st);
            for (int i = 0; i < 20; ++i) {
                usepoint::Frame f = UseFrameAt(point, far1, away, from2(i), 0.0f, true);
                f.point.on = i >= 10;
                usepoint::Update(f, st, o);
                spentBefore += o.spent[0];
            }
        }
        std::snprintf(what, sizeof(what), "t16 a grip held from before, the prompt shown from frame 10: the prompt's at %d (20, held from before %d), asked at %d (20), its "
                                          "tick %d; spent before %d frames (0)",
                      c.tookAt, c.heldInto, c.askedAt, c.tickAt, spentBefore);
        Check(c.tookAt == 20 && c.heldInto && c.askedAt == 20 && c.tickAt == 20 && c.asks == 1 && spentBefore == 0, what);
        // d. Held from a squeeze begun (frame 2) where something else had it (yield at its first frame: a shoulder, the belly with a gun in a hand, near
        // the gun), the yield over at frame 5, the prompt from frame 10: never the prompt's while held (to 100; c, begun free, at 20); let go 100-104 and
        // squeezed at 105: asked at 112. e. ... begun with the hand holding something (a magazine, the gun's front), empty from frame 5: the same.
        int never[2] = {}, againAt[2] = {-1, -1};
        for (int k = 0; k < 2; ++k) {
            const Run r = run(140, far1, [](int i) { return i >= 2 && (i < 100 || i >= 105) ? 1.0f : 0.0f; },
                              [k](int i, usepoint::Frame& f) {
                                  f.point.on = i >= 10;
                                  if (k == 0)
                                      f.hand[0].yield = i < 5;
                                  else
                                      f.hand[0].empty = i >= 5;
                              },
                              nothing);
            never[k] = r.tookAt >= 0 && r.tookAt < 100 ? 1 : 0;
            againAt[k] = r.askedAt;
        }
        std::snprintf(what, sizeof(what), "t16 held from a squeeze begun at something else's place (%d) or with the hand holding something (%d): never the prompt's while held "
                                          "(0, 0); let go and squeezed again: asked at %d and %d (112)",
                      never[0], never[1], againAt[0], againAt[1]);
        Check(!never[0] && !never[1] && againAt[0] == 112 && againAt[1] == 112, what);
        // f. The gun's hand (it holds the gun: not empty) never: a fresh squeeze, or one held from before; nor a hand whose squeeze is something else's.
        const Run fFresh = run(80, far1, from5, [](int, usepoint::Frame& f) { f.hand[0].empty = false; }, nothing);
        const Run fHeld = run(80, far1, [](int) { return 1.0f; }, [](int i, usepoint::Frame& f) {
            f.hand[0].empty = false;
            f.point.on = i >= 10;
        }, nothing);
        const Run fYield = run(80, far1, from5, [](int, usepoint::Frame& f) { f.hand[0].yield = true; }, nothing);
        std::snprintf(what, sizeof(what), "t16 the gun's hand: fresh %d, held %d asks, spent %d frames (none); a squeeze something else's: %d asks, spent %d (none)", fFresh.asks,
                      fHeld.asks, fFresh.spentFrames + fHeld.spentFrames, fYield.asks, fYield.spentFrames);
        Check(fFresh.asks == 0 && fHeld.asks == 0 && fFresh.spentFrames + fHeld.spentFrames == 0 && fYield.asks == 0 && fYield.spentFrames == 0, what);
        // g. The game's target rescanned: a click at 5 whose ask's time (12) falls in a gap of 14 frames (0.19 s, 6-19): kept, asked as the prompt shows
        // again (20), spent throughout (75); in a gap of 25 frames (0.35 s, past 0.3: the showing over, 6-30): still the use's, asked as it shows again
        // (31). A click made in a gap (12, the prompt unseen 10-23): the prompt's at once (12: the review of 01a5132, finding 5), asked as it shows again
        // (24).
        const Run gShort = run(80, far1, from5, [](int i, usepoint::Frame& f) { f.point.on = !(i >= 6 && i < 20); }, nothing);
        const Run gLong = run(90, far1, from5, [](int i, usepoint::Frame& f) { f.point.on = !(i >= 6 && i < 31); }, nothing);
        const Run gIn = run(80, far1, [](int i) { return i >= 12 ? 1.0f : 0.0f; }, [](int i, usepoint::Frame& f) { f.point.on = !(i >= 10 && i < 24); }, nothing);
        std::snprintf(what, sizeof(what), "t16 a click whose ask's time is in a gap of 0.19 s: asked at %d (20), spent %d frames (75); of 0.35 s: asked at %d (31), spent %d "
                                          "(85); a click made in a gap: the prompt's at %d (12), asked at %d (24)",
                      gShort.askedAt, gShort.spentFrames, gLong.askedAt, gLong.spentFrames, gIn.tookAt, gIn.askedAt);
        Check(gShort.askedAt == 20 && gShort.spentFrames == 75 && gShort.asks == 1 && gLong.askedAt == 31 && gLong.spentFrames == 85 && gLong.asks == 1 &&
                  gIn.tookAt == 12 && gIn.askedAt == 24 && gIn.asks == 1,
              what);
        // h. Another prompt (the game's entity) before the ask's time: the use asked for the one showing then (12), once, the squeeze still the use's;
        // another prompt at frame 30 while the squeeze that used the first is held: nothing more (one use a squeeze).
        const Run h = run(80, far1, from5, [](int i, usepoint::Frame& f) { f.point.id = i < 6 ? 7u : 9u; }, nothing);
        const Run h2 = run(80, far1, from5, [](int i, usepoint::Frame& f) { f.point.id = i < 30 ? 7u : 9u; }, nothing);
        std::snprintf(what, sizeof(what), "t16 another prompt shown before the ask's time: asked at %d (12), once (%d), spent %d frames (75); another at frame 30 with the "
                                          "squeeze held: asks %d (1)",
                      h.askedAt, h.asks, h.spentFrames, h2.asks);
        Check(h.askedAt == 12 && h.asks == 1 && h.spentFrames == 75 && h2.askedAt == 12 && h2.asks == 1, what);
        // i. A use not made for a moment (Transient), Rearm: asked again 0.5 s on (frame 48), the squeeze claimed meanwhile, no tick again; again (84); a
        // third Rearm refused (twice a squeeze); a click let go owed (Rearm: asked again 0.5 s on while it shows, the hand not squeezed again), none for a
        // refused squeeze, none with "usereach 1".
        int rearms[3] = {}, claimedGap = 0;
        const Run i1 = run(140, far1, [](int i) { return i >= 5 ? 1.0f : 0.0f; }, none, [&](int i) {
            if (o.use == 0) {
                const int k = static_cast<int>(st.hand[0].retries);
                if (k < 3)
                    rearms[k] = usepoint::Rearm(st, 0) ? 1 : 0;
            }
            if (i > 12 && i < 48)
                claimedGap += o.claimed[0];
        });
        const bool third = i1.askFrames.size() == 3 && rearms[0] && rearms[1] && !rearms[2];
        usepoint::Clear(st);
        bool letGo = true, refusedNo = true, reachNo = true;
        {
            for (int i = 0; i < 30; ++i)
                usepoint::Update(UseFrameAt(point, far1, away, i >= 5 && i < 25 ? 1.0f : 0.0f, 0.0f, true), st, o);
            letGo = usepoint::Rearm(st, 0) && st.hand[0].due;  // (its use asked at 12 and the click let go at 25: owed)
            usepoint::Clear(st);
            const float atPoint[3] = {1.1f, 0.2f, 2.1f};  // (in the prompt's own reach: a prompt that gives a gun refuses there alone)
            for (int i = 0; i < 50; ++i) {
                usepoint::Frame f = UseFrameAt(point, atPoint, away, i >= 5 ? 1.0f : 0.0f, 0.0f, true);
                f.point.gives = f.gunHeld = true;
                usepoint::Update(f, st, o);
            }
            refusedNo = st.hand[0].refused && !usepoint::Rearm(st, 0);
            usepoint::Clear(st);
            const float inReach[3] = {1.3f, 0.1f, 2.2f};  // (t6's in1: 36 cm across, 10 up)
            for (int i = 0; i < 30; ++i)
                usepoint::Update(UseFrameAt(point, inReach, away, i >= 5 ? 1.0f : 0.0f, 0.0f, false), st, o);
            reachNo = !usepoint::Rearm(st, 0);
        }
        std::snprintf(what, sizeof(what), "t16 asked again after a moment's failure: asks at %d, %d, %d (12, 48, 84), a third Rearm refused %d, claimed between %d frames (35), "
                                          "ticks %d (1); a click let go owed %d, refused none %d, \"usereach 1\" none %d",
                      i1.askFrames.size() > 0 ? i1.askFrames[0] : -1, i1.askFrames.size() > 1 ? i1.askFrames[1] : -1, i1.askFrames.size() > 2 ? i1.askFrames[2] : -1,
                      !rearms[2], claimedGap, i1.ticks, letGo, refusedNo, reachNo);
        Check(third && i1.askFrames[0] == 12 && i1.askFrames[1] == 48 && i1.askFrames[2] == 84 && claimedGap == 35 && i1.ticks == 1 && letGo && refusedNo && reachNo,
              what);
        // j. One use a squeeze: held on after its use, none again (to 80); let go 80-84 and squeezed at 85: asked at 92.
        const Run j = run(130, far1, [](int i) { return i >= 5 && (i < 80 || i >= 85) ? 1.0f : 0.0f; }, none, nothing);
        std::snprintf(what, sizeof(what), "t16 one use a squeeze: asks at %d and %d (12, 92), %d in all (2)", j.askFrames.empty() ? -1 : j.askFrames[0],
                      j.askFrames.size() > 1 ? j.askFrames[1] : -1, j.asks);
        Check(j.asks == 2 && j.askFrames[0] == 12 && j.askFrames[1] == 92, what);
        // k. The AT4's prompt (it gives a gun) with a gun in a hand, held anywhere, a squeeze at the tube itself (in its own reach): refused at its time (12),
        // the double click (12, 17), no tick, no use; the gun let go of at 60 with the grip still held: nothing (one a squeeze); let go 70-74, squeezed at 75:
        // the use at 82, its tick then. kAway: the same 2.4 m from it: never the prompt's (no refusal, no double click, never spent: its squeeze the hands');
        // the gun let go of at 40, the grip held from before (begun free, held 0.5 s, the prompt up long since) is the prompt's at once and asked (40).
        const float atTube[3] = {1.1f, 0.2f, 2.1f};
        const Run k = run(130, atTube, [](int i) { return i >= 5 && (i < 70 || i >= 75) ? 1.0f : 0.0f; }, [](int i, usepoint::Frame& f) {
            f.point.gives = true;
            f.gunHeld = i < 60;
        }, nothing);
        int awaySpentBefore = 0;
        const Run kAway = run(100, far1, from5, [](int i, usepoint::Frame& f) {
            f.point.gives = true;
            f.gunHeld = i < 40;
        }, [&](int i) {
            if (i < 40)
                awaySpentBefore += o.spent[0] || o.claimed[0] || o.busy[0];
        });
        std::snprintf(what, sizeof(what), "t16 the AT4's prompt with a gun in a hand, at the tube: refused at %d (12), double click %d pulses, the second at %d (17), the use at "
                                          "%d (82), its tick at %d (82), %d tick(s) (1); 2.4 m from it: refused %d, pulses %d, spent while barred %d (none), then the "
                                          "grip held from before the prompt's at %d (40, held from before %d), used at %d (40)",
                      k.refusedAt, k.doubles, k.secondAt, k.askedAt, k.tickAt, k.ticks, kAway.refusedAt, kAway.doubles, awaySpentBefore, kAway.tookAt,
                      kAway.heldInto, kAway.askedAt);
        Check(k.refusedAt == 12 && k.doubles == 2 && k.secondAt == 17 && k.askedAt == 82 && k.asks == 1 && k.tickAt == 82 && k.ticks == 1 && kAway.refusedAt < 0 &&
                  kAway.doubles == 0 && awaySpentBefore == 0 && kAway.tookAt == 40 && kAway.heldInto && kAway.askedAt == 40 && kAway.asks == 1,
              what);
        // l. A module's take in the squeeze's first frame (the hands' modules run after the use in it: the hand not empty from frame 6): given up at 6, never
        // asked, never claimed; a take later (from 15) comes after the ask (12): the squeeze claimed by then, no module's to take (g_useOwnsBy).
        const Run l = run(60, far1, from5, [](int i, usepoint::Frame& f) { f.hand[0].empty = i < 6; }, nothing);
        const Run lLate = run(60, far1, from5, [](int i, usepoint::Frame& f) { f.hand[0].empty = i < 15; }, nothing);
        std::snprintf(what, sizeof(what), "t16 a module's take in the squeeze's first frame: given up at %d (6), asked %d, claimed %d frames (0); from 15: asked at %d (12), "
                                          "given up %d (never), claimed from 12 on %d frames (48)",
                      l.givenAt, l.asks, l.claimedFrames, lLate.askedAt, lLate.givenAt, lLate.claimedFrames);
        Check(l.givenAt == 6 && l.asks == 0 && l.claimedFrames == 0 && lLate.askedAt == 12 && lLate.givenAt < 0 && lLate.claimedFrames == 48, what);
        // m. The body's places (BodyKeeps): held anywhere only with a gun in a hand, any prompt; "usereach 1" BodyFirst's (0.1.14).
        usepoint::Point door, at4;
        at4.gives = true;
        const bool body = usepoint::BodyKeeps(door, true, true) && !usepoint::BodyKeeps(door, false, true) && usepoint::BodyKeeps(at4, true, true) &&
                          !usepoint::BodyKeeps(at4, false, true) && usepoint::BodyKeeps(door, false, false) == usepoint::BodyFirst(door, false) &&
                          usepoint::BodyKeeps(at4, false, false) == usepoint::BodyFirst(at4, false) && usepoint::BodyKeeps(at4, true, false) == usepoint::BodyFirst(at4, true);
        Check(body, "t16 the body's places held anywhere: a door's or the AT4's squeeze at the belly the body's only with a gun in a hand; \"usereach 1\": BodyFirst");
        // n. Transient: the four words a moment's failure, pickups.cpp's own word for word; never used up, not on, the game's own checks, another use, none.
        const char* const moments[] = {"the game doesn't target it now", "too late", "the soldier isn't awake", "not answered in 1 s"};
        const char* const notMoments[] = {"used up", "it isn't on", "the game's own checks refused it (a delay, a team, already a user)", "another use waits",
                                          "a later use was served", "not answered (another use asked since)", "the game's use faulted (C0000005)", ""};
        int momentsRight = 0;
        for (const char* m : moments)
            momentsRight += usepoint::Transient(m);
        for (const char* m : notMoments)
            momentsRight += !usepoint::Transient(m);
        momentsRight += !usepoint::Transient(nullptr);
        std::string pickupsCpp = ReadText("src\\pickups.cpp");
        int inPickups = 0;
        for (const char* m : moments)
            inPickups += pickupsCpp.find(std::string("\"") + m + "\"") != std::string::npos;
        if (pickupsCpp.empty())
            Skip("t16 pickups.cpp's words (run from the project's folder)");
        std::snprintf(what, sizeof(what), "t16 a moment's failure: %d of %d words told right; pickups.cpp gives each of the four word for word (%d)", momentsRight,
                      static_cast<int>(std::size(moments) + std::size(notMoments) + 1), inPickups);
        Check(momentsRight == static_cast<int>(std::size(moments) + std::size(notMoments) + 1) && (pickupsCpp.empty() || inPickups == 4), what);
        // o. Why a grip doesn't answer (GripOf, the "Use grips:" lines): let go, the prompt not showing, held from before (the prompt's once it has shown
        // 0.15 s), asked, a fresh squeeze not yet asked (holding: by its time), something else's, held from one begun elsewhere, holding something, refused.
        usepoint::Clear(st);
        usepoint::Grip seen[6] = {};
        for (int i = 0; i < 70; ++i) {
            usepoint::Frame f = UseFrameAt(point, far1, away, i >= 2 ? 1.0f : 0.0f, 0.0f, true);
            f.point.on = i >= 10;
            usepoint::Update(f, st, o);
            const usepoint::Grip g = usepoint::GripOf(f, st, 0);
            if (i == 0)
                seen[0] = g;  // (let go)
            else if (i == 5)
                seen[1] = g;  // (squeezed, the prompt not showing)
            else if (i == 10)
                seen[2] = g;  // (held from before, the prompt just shown: settling)
            else if (i == 20)
                seen[3] = g;  // (the prompt's at 20 and asked)
            else if (i == 60)
                seen[4] = g;  // (asked, held on)
        }
        usepoint::Clear(st);
        for (int i = 0; i < 9; ++i)  // (a fresh squeeze at 5 of a hand not clear of the hands' places: the prompt's, its use not asked yet at 8)
            usepoint::Update([&] {
                usepoint::Frame f = UseFrameAt(point, far1, away, i >= 5 ? 1.0f : 0.0f, 0.0f, true);
                f.hand[0].clear = false;
                return f;
            }(), st, o);
        seen[5] = usepoint::GripOf(UseFrameAt(point, far1, away, 1.0f, 0.0f, true), st, 0);
        usepoint::Clear(st);
        usepoint::Frame fy = UseFrameAt(point, far1, away, 0.0f, 0.0f, true);
        usepoint::Update(fy, st, o);  // (let go, read)
        fy.hand[0].squeeze = 1.0f;
        fy.hand[0].yield = true;
        usepoint::Update(fy, st, o);  // (squeezed where something else has it)
        const usepoint::Grip yieldGrip = usepoint::GripOf(fy, st, 0);
        fy.hand[0].yield = false;
        usepoint::Update(fy, st, o);
        const usepoint::Grip beganElse = usepoint::GripOf(fy, st, 0);
        fy.hand[0].empty = false;
        const usepoint::Grip holds = usepoint::GripOf(fy, st, 0);
        using G = usepoint::Grip;
        const bool grips = seen[0] == G::kOpen && seen[1] == G::kNotShowing && seen[2] == G::kSettling && seen[3] == G::kAsked && seen[4] == G::kAsked &&
                           seen[5] == G::kHolding && yieldGrip == G::kElse && beganElse == G::kBeganElse && holds == G::kHolds &&
                           std::strlen(usepoint::GripWord(G::kRefused)) > 0;
        std::snprintf(what, sizeof(what), "t16 why a grip doesn't answer (GripOf): let go %d, the prompt not showing %d, held from before settling %d, asked %d and %d, "
                                          "holding %d, something else's %d, begun elsewhere %d, holding something %d",
                      static_cast<int>(seen[0]), static_cast<int>(seen[1]), static_cast<int>(seen[2]), static_cast<int>(seen[3]), static_cast<int>(seen[4]),
                      static_cast<int>(seen[5]), static_cast<int>(yieldGrip), static_cast<int>(beganElse), static_cast<int>(holds));
        Check(grips, what);
        // p. Both hands squeezed together, far apart: the left's use (12), and the right's squeeze that use's too (asked and claimed with it: one use, the
        // review of babecfd, finding 3); both squeezes the use's.
        usepoint::Clear(st);
        int leftAt = -1, rightAt = -1, bothSpent = 0, rightClaimedAt = -1;
        for (int i = 0; i < 60; ++i) {
            usepoint::Update(UseFrameAt(point, far1, away, i >= 5 ? 1.0f : 0.0f, i >= 5 ? 1.0f : 0.0f, true), st, o);
            if (o.use == 0 && leftAt < 0)
                leftAt = i;
            if (o.use == 1 && rightAt < 0)
                rightAt = i;
            if (o.claimed[1] && rightClaimedAt < 0)
                rightClaimedAt = i;
            bothSpent += o.spent[0] && o.spent[1];
        }
        std::snprintf(what, sizeof(what), "t16 both grips squeezed at once: the left asked at %d (12), the right never (%d), its squeeze claimed with it at %d (12), both the "
                                          "use's %d frames (55)",
                      leftAt, rightAt, rightClaimedAt, bothSpent);
        Check(leftAt == 12 && rightAt < 0 && rightClaimedAt == 12 && bothSpent == 55, what);
        // t. A grip not read a moment (its controller unseen, the headset off: Hand::tracked false) before its ask: nothing known of it, never a let-go: the
        // squeeze kept (frames 6-15 unread), the use's throughout, asked held kFreshAsk of its read time (47: 32 frames read from its first, 5, past the
        // 10 unread, as f1552ad counted it; where it went meanwhile isn't known, so no still hand's ask);
        // a grip read again squeezed (squeezed while unread) is no fresh squeeze: never the prompt's till let go; nor one held as the state begins (after a
        // new life's Clear).
        const Run tBlip = run(80, far1, from5, [](int i, usepoint::Frame& f) { f.hand[0].tracked = !(i >= 6 && i < 16); }, nothing);
        const Run tBack = run(80, far1, [](int i) { return i >= 12 ? 1.0f : 0.0f; }, [](int i, usepoint::Frame& f) { f.hand[0].tracked = !(i >= 10 && i < 20); },
                              nothing);
        const Run tStart = run(80, far1, [](int) { return 1.0f; }, none, nothing);
        std::snprintf(what, sizeof(what), "t16 a grip unread 10 frames before its ask: asked at %d (47), spent %d frames (75); squeezed while unread: asks %d (0); held as "
                                          "the state begins: asks %d (0)",
                      tBlip.askedAt, tBlip.spentFrames, tBack.asks, tStart.asks);
        Check(tBlip.askedAt == 47 && tBlip.spentFrames == 75 && tBack.asks == 0 && tStart.asks == 0, what);
        // q. The rule changed under way, "usereach 1" (anywhere false): a hand 2.4 m off never; in reach as 0.1.14 (t6's rows).
        usepoint::Clear(st);
        int reachAsks = 0;
        for (int i = 0; i < 60; ++i) {
            usepoint::Update(UseFrameAt(point, far1, away, i >= 5 ? 1.0f : 0.0f, 0.0f, false), st, o);
            reachAsks += o.use >= 0;
        }
        std::snprintf(what, sizeof(what), "t16 \"usereach 1\": a hand 2.4 m off asks %d time(s) (never; in reach: t6's rows)", reachAsks);
        Check(reachAsks == 0, what);
        // r. hands.cpp's lines (no test compiles hands.cpp): the rule's frame (asleep only a new life, a switch's bridging, a fight's run; the point on
        // without its place), the right hand the game's gun's with free hands asleep, the left the train pistol's on it, the yield every frame, the gift's
        // start only within the prompt's own reach, a moment's failure asked again, the use's squeeze no bar to a key once its prompt shows no more, the
        // empty hand for the quick-time step; the test hand's place.
        std::string handsCpp = ReadText("src\\hands.cpp");
        handsCpp.erase(std::remove(handsCpp.begin(), handsCpp.end(), '\r'), handsCpp.end());
        if (handsCpp.empty()) {
            Skip("t16 hands.cpp's lines (run from the project's folder)");
        } else {
            const char* const lines[] = {
                "    const bool anywhere = !pickups::Tuning().useReach;",
                "    uf.asleep = anywhere ? f.newLife || g_bridging || input::QteFightLive() : !enabled || f.frozen || f.newLife;",
                "    uf.point.on = shown && (anywhere || placed);",
                "(holder == Holder::kBarrel && g_grasp.barrel == s) ||\n                              (s == 1 && !enabled);",
                "        const bool onPistol = s == 0 && (g_pistolSupport || g_pistolBlend > 0.0f || SlideHeld() || pistolmag::Holding());\n"
                "        u.empty = !holdsGun && !in.busy && !in.onSupport && !onPistol;",
                "    const bool testing = UseTestHand(f, hr, uf, anywhere, now, &yieldWhy[0]);\n    usepoint::Update(uf, g_usePoint, out);",
                "        const bool atPrompt = usepoint::Reach(uf.point, uf.hand[out.use].at) && RoomPointToWorld(uf.hand[out.use].at, handWorld);",
                "        } else if (anywhere && usepoint::Transient(why) && usepoint::Rearm(g_usePoint, out.use)) {",
                "    } else if (usepoint::Transient(why) && usepoint::Rearm(g_usePoint, side)) {",
                "        g_useDoneBy[side] = g_useAsked.entity;",
                "        c.use = g_useSpentBy[s] && !UseDoneGone(s, GetTickCount64());",
                "        g_emptyNow[s].store(qte::Empty(c) && !(s == 1 && !g_freeOn), std::memory_order_relaxed);",
                "        if (!uo.spent[s])\n            g_useDoneBy[s] = 0;",
                "constexpr float kUseTestAt[3] = {-0.35f, -0.15f, 0.40f};",
            };
            int found = 0;
            std::string missing;
            for (const char* l : lines) {
                const bool here = handsCpp.find(l) != std::string::npos;
                found += here;
                if (!here && missing.size() < 200)
                    missing += std::string(" [") + std::string(l).substr(0, 60) + "]";
            }
            std::snprintf(what, sizeof(what), "t16 hands.cpp's lines for the rule held anywhere, word for word (%d of %d%s)", found, static_cast<int>(std::size(lines)),
                          missing.c_str());
            Check(found == static_cast<int>(std::size(lines)), what);
        }
        // s. The use test's hand (weapon.txt usetest) is a free hand: from the neck point (-0.35, -0.15, 0.40) out of the belly and the chests, out of every
        // holster place, and more than 25 cm from the stand-in's rifle (its aim 8 cm right, 25 under and 45 ahead of the eyes, the neck point 7.5 cm under and
        // 8 cm behind them: the rifle a box 5 cm round its line, 30 cm back and 50 ahead of its grip).
        {
            const float testAt[3] = {-0.35f, -0.15f, 0.40f};
            const holster::Settings hs;
            const bool outBody = !holster::AtBodyTake(testAt, hs) && holster::PlaceAt(testAt, hs) == holster::Place::kNone;
            // (In the eyes' axes, right, up, forward: the neck point (0, -0.075, -0.08); the rifle's line x 0.08, y -0.25, forward 0.15 to 0.95.)
            const float p[3] = {testAt[0], -0.075f + testAt[1], -0.08f + testAt[2]};
            const float q[3] = {0.08f, -0.25f, std::fmin(std::fmax(p[2], 0.15f), 0.95f)};
            const float apart = std::sqrt((p[0] - q[0]) * (p[0] - q[0]) + (p[1] - q[1]) * (p[1] - q[1]) + (p[2] - q[2]) * (p[2] - q[2])) - 0.05f;
            std::snprintf(what, sizeof(what), "t16 the use test's hand: out of the body's places and every holster place (%d), %.2f m from the stand-in's rifle (over 0.25)",
                          outBody, static_cast<double>(apart));
            Check(outBody && apart > 0.25f, what);
        }
    }

    // ---- t17: the review of babecfd (findings 1-4, 7-9), held anywhere (usepoint.h's Frame::anywhere). At 72 Hz. ----
    {
        const float point[3] = {1.0f, 0.0f, 2.0f}, far1[3] = {-0.5f, 1.2f, 0.5f}, away[3] = {3.0f, 1.0f, 3.0f};
        const float atTube[3] = {1.1f, 0.2f, 2.1f};  // (in the prompt's own reach)
        usepoint::State st;
        usepoint::Out o;
        // a. Finding 1, the grasp's own take, and the click: a door 3 m off, the M4A1 lying on the floor (t15's box); the right squeezed 1 m over it (no lying
        // gun within any reach, clear of it: the use's) and on it k frames on. On it within kStillFor (k 0-7: the squeeze's first frame, or before its
        // still hand's ask): the grasp's, the use given up (on the gun, nearer than the door), never asked, the grasp never told busy before. Held still
        // 0.1 s away from everything first (k 8-28): the click's, the door asked then (12), the grasp told busy from then, the gun not taken (once a prompt
        // shows, a still squeeze of a free hand away from everything is the interact button's: the owner's "even a click"; a hand moving to the gun is
        // never still: t19b).
        {
            usepoint::Point door;
            door.on = true;
            door.at[0] = 3.0f, door.at[1] = 0.0f, door.at[2] = 0.0f;
            const float lo[3] = {-0.06f, 0.0f, -0.40f}, hi[3] = {0.06f, 0.10f, 0.45f};
            const float room[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
            const float from[3] = {0.6f, 1.0f, 0.0f}, to[3] = {0.0f, 0.12f, 0.0f};
            int taken = 0, asked = 0, given = 0, busyBefore = 0, claimedBefore = 0, lateTaken = 0, lateAsked = 0, lateBusyBefore = 0;
            for (int k = 0; k <= 28; ++k) {
                const CarryOut c = CarryTo(door, from, to, k, room, lo, hi);
                if (k <= 7) {
                    taken += c.taken && c.takenAt == 5 + k;
                    asked += c.asked;
                    given += c.yielded || c.given;  // (on the gun it's the gun's: given up that frame, Out::yielded; or the hand not empty after, Out::given)
                    busyBefore += c.busyBefore;
                    claimedBefore += c.claimedBefore;
                } else {
                    lateTaken += c.taken;
                    lateAsked += c.asked;
                    lateBusyBefore += c.busyBefore;  // (busy before it got to the gun: from the ask, frame 12)
                }
            }
            std::snprintf(what, sizeof(what), "t17a a squeeze on a lying gun within 0.1 s: taken by the grasp %d of 8, the door asked %d (0), given up %d (7: on it at its "
                                              "crossing it was never the use's), busy before %d, claimed before %d (0, 0); held still away 0.1 s first (8-28 frames "
                                              "on): the door asked %d of 21, the gun taken %d (0), the grasp told busy before it got there %d (21)",
                          taken, asked, given, busyBefore, claimedBefore, lateAsked, lateTaken, lateBusyBefore);
            Check(taken == 8 && asked == 0 && given == 7 && busyBefore == 0 && claimedBefore == 0 && lateAsked == 21 && lateTaken == 0 && lateBusyBefore == 21, what);
        }
        // b. Finding 1, the modules, and the click: a squeeze begun free and still 2.4 m from the point, clear of everything, then at a module's place (the
        // pouch at the belly, the handguard and the gun's parts within 25 cm, a holster's place, a round, a rocket) k frames on (Hand::yield from then, as
        // UseYields gives it): within kStillFor (k 1-7): given up there (Out::yielded), never claimed, never asked: the module's take free; held still 0.1 s
        // first (k 8-28): the click's, asked at 12, claimed from then (g_useOwnsBy: no module takes it); a hand on its way there moves, never still: t19.
        // A door; and the AT4's prompt with a gun in a hand (the roof, the player fighting within its 2 m: the hand away from the tube is never the
        // prompt's at all: no refusal, no double click). At the tube (in its own reach) with a gun in a hand: refused at its time (12, the double click),
        // claimed from then (carried to the belly at 0.3 s: no magazine with it).
        {
            int claimed[2] = {}, yieldedAt[2] = {}, asks[2] = {}, refusals[2] = {}, pulses[2] = {}, spent[2] = {}, lateAsks = 0, lateClaimed = 0;
            for (int pass = 0; pass < 2; ++pass) {
                for (int k = 1; k <= 28; ++k) {
                    usepoint::Clear(st);
                    for (int i = 0; i < 80; ++i) {
                        usepoint::Frame f = UseFrameAt(point, far1, away, i >= 5 ? 1.0f : 0.0f, 0.0f, true);
                        f.point.gives = f.gunHeld = pass == 1;
                        f.hand[0].yield = i >= 5 + k;
                        usepoint::Update(f, st, o);
                        if (pass == 0 && k >= 8) {  // (the click's: asked at 12, claimed from then)
                            lateAsks += o.use == 0 && i == 12;
                            lateClaimed += o.claimed[0] && i >= 12;
                            continue;
                        }
                        claimed[pass] += o.claimed[0];
                        yieldedAt[pass] += o.yielded[0] && i == 5 + k;
                        asks[pass] += o.use == 0;
                        refusals[pass] += o.refused[0];
                        pulses[pass] += o.pulse[0].seconds > 0.0f;
                        spent[pass] += o.spent[0];
                    }
                }
            }
            usepoint::Clear(st);
            int tubeYieldAt = -1, tubeRefusedAt = -1, tubePulses = 0, tubeClaimed = 0;
            for (int i = 0; i < 80; ++i) {
                usepoint::Frame f = UseFrameAt(point, atTube, away, i >= 5 ? 1.0f : 0.0f, 0.0f, true);
                f.point.gives = f.gunHeld = true;
                f.hand[0].yield = i >= 27;  // (at the belly with the gun in the other hand: a magazine's)
                usepoint::Update(f, st, o);
                if (o.yielded[0] && tubeYieldAt < 0)
                    tubeYieldAt = i;
                if (o.refused[0] && tubeRefusedAt < 0)
                    tubeRefusedAt = i;
                tubePulses += o.pulse[0].seconds > 0.0f;
                tubeClaimed += o.claimed[0];
            }
            std::snprintf(what, sizeof(what), "t17b a squeeze begun free carried to a module's place within 0.1 s: a door: claimed %d frames (0), given up there %d of 7, "
                                              "asked %d (0); held still 0.1 s first (8-28 frames on): asked at 12 %d of 21, claimed from 12 %d frames (1428); the AT4 "
                                              "with a gun in a hand, away from the tube: spent %d (0), refused %d, pulses %d (0, 0); at the tube: refused at %d (12), "
                                              "pulses %d (2), given up %d (never), claimed %d frames (68)",
                          claimed[0], yieldedAt[0], asks[0], lateAsks, lateClaimed, spent[1], refusals[1], pulses[1], tubeRefusedAt, tubePulses, tubeYieldAt,
                          tubeClaimed);
            Check(claimed[0] == 0 && yieldedAt[0] == 7 && asks[0] == 0 && lateAsks == 21 && lateClaimed == 21 * 68 && spent[1] == 0 && refusals[1] == 0 &&
                      pulses[1] == 0 && claimed[1] == 0 && tubeRefusedAt == 12 && tubePulses == 2 && tubeYieldAt < 0 && tubeClaimed == 68,
                  what);
        }
        // c. Finding 1, the grasp's busy (Out::busy): a squeeze away from the prompt's reach: not busy before its use is asked (5-11), busy from it (12 on);
        // in the prompt's own reach (the AT4's tube: never the rifle lying beside it): busy from its first frame; given up (something else's before its
        // ask, 6): busy no more from then; "usereach 1" in reach: busy every frame spent is, bit for bit (0.1.14's).
        {
            int awayBefore = 0, awayAfter = 0, reachFrom = -1, givenBusy = 0, reachDiff = 0, reachSpent = 0;
            usepoint::Clear(st);
            for (int i = 0; i < 60; ++i) {
                usepoint::Update(UseFrameAt(point, far1, away, i >= 5 ? 1.0f : 0.0f, 0.0f, true), st, o);
                awayBefore += o.busy[0] && i < 12;
                awayAfter += o.busy[0] && i >= 12;
            }
            usepoint::Clear(st);
            for (int i = 0; i < 30; ++i) {
                usepoint::Update(UseFrameAt(point, atTube, away, i >= 5 ? 1.0f : 0.0f, 0.0f, true), st, o);
                if (o.busy[0] && reachFrom < 0)
                    reachFrom = i;
            }
            usepoint::Clear(st);
            for (int i = 0; i < 40; ++i) {
                usepoint::Frame f = UseFrameAt(point, atTube, away, i >= 5 ? 1.0f : 0.0f, 0.0f, true);
                f.hand[0].yield = i >= 6;
                usepoint::Update(f, st, o);
                givenBusy += o.busy[0] && i >= 6;
            }
            usepoint::Clear(st);
            const float in1[3] = {1.3f, 0.1f, 2.2f};
            for (int i = 0; i < 60; ++i) {
                usepoint::Frame f = UseFrameAt(point, i < 40 ? in1 : away, away, i >= 5 && i < 50 ? 1.0f : 0.0f, 0.0f, false);
                f.hand[0].empty = i < 45;
                usepoint::Update(f, st, o);
                reachDiff += o.busy[0] != o.spent[0];
                reachSpent += o.spent[0];
            }
            std::snprintf(what, sizeof(what), "t17c the grasp's busy: away from the reach before the ask %d frames (0), from it %d (48); in the prompt's own reach from frame %d "
                                              "(5); given up at 6, busy after %d (0); \"usereach 1\" busy unlike spent %d frames of %d (0)",
                          awayBefore, awayAfter, reachFrom, givenBusy, reachDiff, reachSpent);
            Check(awayBefore == 0 && awayAfter == 48 && reachFrom == 5 && givenBusy == 0 && reachDiff == 0 && reachSpent > 0, what);
        }
        // d. Finding 3, two hands at a prompt still showing after its use (maxUses 0), hands.cpp asking one use at a time, the game serving it 1-3 frames
        // on: the right squeezed 0-60 frames after the left, held: one use every time (babecfd: two for most); the right held from before the prompt, the
        // left's fresh squeeze: one; the left at frame 5 (its use made at 12), the right squeezed at 40 while it's held: not the prompt's (the other's use),
        // the left let go at 60, the right at 70, squeezed again at 80: its use at 87 (two in all).
        {
            int wrong = 0, runs = 0;
            std::string line;
            for (int serve = 1; serve <= 3; ++serve)
                for (int lag : {0, 1, 2, 4, 8, 15, 30, 31, 32, 33, 40, 60}) {
                    const int uses = UsesOfTwo(lag, serve);
                    ++runs;
                    if (uses != 1) {
                        ++wrong;
                        if (line.size() < 120)
                            line += " [serve " + std::to_string(serve) + " lag " + std::to_string(lag) + ": " + std::to_string(uses) + "]";
                    }
                }
            const int held1 = UsesOfTwo(0, 1, true, 35), held3 = UsesOfTwo(0, 3, true, 35);
            usepoint::Clear(st);
            int asksBoth = 0, rightTookBefore80 = 0, rightAt = -1;
            usepoint::Grip rightGrip = usepoint::Grip::kOpen;
            for (int i = 0; i < 140; ++i) {
                usepoint::Frame f = UseFrameAt(point, far1, away, i >= 5 && i < 60 ? 1.0f : 0.0f, (i >= 40 && i < 70) || i >= 80 ? 1.0f : 0.0f, true);
                usepoint::Update(f, st, o);
                asksBoth += o.use >= 0;
                rightTookBefore80 += o.took[1] && i < 80;
                if (o.use == 1 && rightAt < 0)
                    rightAt = i;
                if (i == 50)
                    rightGrip = usepoint::GripOf(f, st, 1);
            }
            std::snprintf(what, sizeof(what), "t17d two hands, the prompt still showing: one use in %d of %d runs (%s); the right held from before: %d and %d use(s) (1, 1); the "
                                              "right squeezed while the left's use is held: the prompt's %d (0, why %d: the other's), squeezed again after both let go: "
                                              "asked at %d (87), %d asks in all (2)",
                          runs - wrong, runs, wrong ? line.c_str() : "all", held1, held3, rightTookBefore80, static_cast<int>(rightGrip), rightAt, asksBoth);
            Check(wrong == 0 && held1 == 1 && held3 == 1 && rightTookBefore80 == 0 && rightGrip == usepoint::Grip::kOtherUsed && rightAt == 87 && asksBoth == 2, what);
        }
        // e. Finding 2, cutscenes (Frame::cutscene): a grip held from before (frame 2), the prompt from 10, in a cutscene throughout: never the prompt's
        // (GripOf: a cutscene's); a cutscene to frame 29 with the grip held through it, the prompt from 40: never (held through it); without the cutscene:
        // the prompt's once it has shown 0.15 s, asked then (50); a fresh squeeze in a cutscene with the prompt up: asked at 12 (hands.cpp makes it by its
        // key).
        {
            const auto held = [&](bool cutAll, int cutTo, int promptFrom, usepoint::Grip* grip) {
                usepoint::Clear(st);
                int askAt = -1;
                for (int i = 0; i < 120; ++i) {
                    usepoint::Frame f = UseFrameAt(point, far1, away, i >= 2 ? 1.0f : 0.0f, 0.0f, true);
                    f.point.on = i >= promptFrom;
                    f.cutscene = cutAll || i < cutTo;
                    usepoint::Update(f, st, o);
                    if (o.use == 0 && askAt < 0)
                        askAt = i;
                    if (grip && i == 100)
                        *grip = usepoint::GripOf(f, st, 0);
                }
                return askAt;
            };
            usepoint::Grip inCut = usepoint::Grip::kOpen, through = usepoint::Grip::kOpen;
            const int cutAll = held(true, 0, 10, &inCut), cutThrough = held(false, 30, 40, &through), none = held(false, 0, 40, nullptr);
            usepoint::Clear(st);
            int freshAt = -1;
            for (int i = 0; i < 60; ++i) {
                usepoint::Frame f = UseFrameAt(point, far1, away, i >= 5 ? 1.0f : 0.0f, 0.0f, true);
                f.cutscene = true;
                usepoint::Update(f, st, o);
                if (o.use == 0 && freshAt < 0)
                    freshAt = i;
            }
            std::snprintf(what, sizeof(what), "t17e cutscenes: held from before in one: asked %d (never, why %d: the cutscene's); held through one, the prompt after it: asked %d "
                                              "(never, why %d); no cutscene: asked at %d (50); a fresh squeeze in one: asked at %d (12)",
                          cutAll, static_cast<int>(inCut), cutThrough, static_cast<int>(through), none, freshAt);
            Check(cutAll < 0 && inCut == usepoint::Grip::kCutscene && cutThrough < 0 && through == usepoint::Grip::kCutscene && none == 50 && freshAt == 12, what);
        }
        // f. Finding 4, a holster's place: a squeeze begun free just outside a shoulder (the use's at frame 5; not clear there: UseClear keeps every place
        // kUseClearMore wider) and in its place k frames on (UseYields' first rule: hr.place, a draw there, a draw that waits): within kFreshAsk (k 1-31):
        // given up there, never asked, never claimed (holsters draw with it: their going-on window is 0.4 s); held there 0.45 s first (k 33): the use's,
        // asked at 37, claimed (holsters take it as busy: HolsterHand's g_useOwnsBy).
        {
            const auto shoulder = [&](int from, int* yieldAt, int* asks, int* claimedAny) {
                usepoint::Clear(st);
                *yieldAt = -1, *asks = 0, *claimedAny = 0;
                for (int i = 0; i < 80; ++i) {
                    usepoint::Frame f = UseFrameAt(point, far1, away, i >= 5 ? 1.0f : 0.0f, 0.0f, true);
                    f.hand[0].clear = false;
                    f.hand[0].yield = i >= from;
                    usepoint::Update(f, st, o);
                    if (o.yielded[0] && *yieldAt < 0)
                        *yieldAt = i;
                    *asks += o.use >= 0;
                    *claimedAny += o.claimed[0];
                }
            };
            int yields = 0, asksIn = 0, claimedIn = 0, lateYield = 0, lateAsks = 0, lateClaimed = 0;
            for (int k = 1; k <= 31; ++k) {
                int yieldAt = 0, asks = 0, claimedAny = 0;
                shoulder(5 + k, &yieldAt, &asks, &claimedAny);
                yields += yieldAt == 5 + k;
                asksIn += asks;
                claimedIn += claimedAny;
            }
            shoulder(5 + 33, &lateYield, &lateAsks, &lateClaimed);
            std::snprintf(what, sizeof(what), "t17f a squeeze the use's by a shoulder, in its place within 0.45 s: given up there %d of 31, asked %d, claimed %d (0, 0); there "
                                              "after 0.45 s: given up %d (never), asked %d (1), claimed %d frames (43)",
                          yields, asksIn, claimedIn, lateYield, lateAsks, lateClaimed);
            Check(yields == 31 && asksIn == 0 && claimedIn == 0 && lateYield < 0 && lateAsks == 1 && lateClaimed == 43, what);
        }
        // g. Findings 8 and 9, and why a grip doesn't answer: both hands on the gun, squeezed: both "holds something" (hands.cpp's "Use grips:" lines count
        // that now); the AT4 with a gun in a hand away from the tube: barred; given up on its way to something else: yielded; held from before after this
        // prompt's use: used; every word said. ByHand: "usereach 1" 3 m at most (0.1.14's), held anywhere 5 m (Kaffarov's HALO start).
        {
            usepoint::Clear(st);
            usepoint::Frame both = UseFrameAt(point, far1, away, 1.0f, 1.0f, true);
            both.hand[0].empty = both.hand[1].empty = false;
            for (int i = 0; i < 10; ++i)
                usepoint::Update(both, st, o);
            const bool holds = usepoint::GripOf(both, st, 0) == usepoint::Grip::kHolds && usepoint::GripOf(both, st, 1) == usepoint::Grip::kHolds;
            usepoint::Clear(st);
            usepoint::Frame barredF = UseFrameAt(point, far1, away, 0.0f, 0.0f, true);
            barredF.point.gives = barredF.gunHeld = true;
            usepoint::Update(barredF, st, o);
            barredF.hand[0].squeeze = 1.0f;
            usepoint::Update(barredF, st, o);
            const usepoint::Grip barred = usepoint::GripOf(barredF, st, 0);
            usepoint::Clear(st);
            usepoint::Grip yielded = usepoint::Grip::kOpen, usedHeld = usepoint::Grip::kOpen;
            for (int i = 0; i < 20; ++i) {
                usepoint::Frame f = UseFrameAt(point, far1, away, i >= 5 ? 1.0f : 0.0f, 0.0f, true);
                f.hand[0].yield = i >= 6;
                usepoint::Update(f, st, o);
                yielded = usepoint::GripOf(f, st, 0);
            }
            usepoint::Clear(st);
            for (int i = 0; i < 80; ++i) {  // (the left's squeeze at 5 used at 12, let go at 45; the right squeezed at 30 while it's held: held on, from before)
                usepoint::Frame f = UseFrameAt(point, far1, away, i >= 5 && i < 45 ? 1.0f : 0.0f, i >= 30 ? 1.0f : 0.0f, true);
                usepoint::Update(f, st, o);
                if (i == 70)
                    usedHeld = usepoint::GripOf(f, st, 1);
            }
            bool words = true;
            for (int g = 0; g <= static_cast<int>(usepoint::Grip::kFrontLost); ++g)
                words = words && std::strlen(usepoint::GripWord(static_cast<usepoint::Grip>(g))) > 0;
            const bool byHand = !usepoint::ByHand(35, 0, 0.0f, 5.0f, false) && usepoint::ByHand(35, 0, 0.0f, 3.0f, false) &&
                                !usepoint::ByHand(35, 0, 0.0f, 3.01f, false) && usepoint::ByHand(35, 0, 0.0f, 5.0f, true) && usepoint::ByHand(35, 0, 0.0f, 5.0f) &&
                                !usepoint::ByHand(35, 0, 0.0f, 5.01f, true);
            std::snprintf(what, sizeof(what), "t17g why a grip doesn't answer: both on the gun %d, barred away from the AT4 %d (%d), carried on %d (%d), held after the prompt's "
                                              "use %d (%d); every word %d; ByHand \"usereach 1\" 3 m, anywhere 5 m %d",
                          holds, barred == usepoint::Grip::kBarred, static_cast<int>(barred), yielded == usepoint::Grip::kYielded, static_cast<int>(yielded),
                          usedHeld == usepoint::Grip::kUsedHeld, static_cast<int>(usedHeld), words, byHand);
            Check(holds && barred == usepoint::Grip::kBarred && yielded == usepoint::Grip::kYielded && usedHeld == usepoint::Grip::kUsedHeld && words && byHand, what);
        }
        // h. hands.cpp's and input.cpp's lines for these, word for word (no test compiles them): the scripted moment's use by the prompt's key, a cutscene's
        // frame, the yield every frame till the ask, the grasp's busy from usepoint's busy, holsters' busy with the use's claim, both hands on the gun in
        // the "Use grips:" lines, ByHand by the rule, the done prompt's 0.3 s; README.md without a control character (finding 7: a backspace in a path).
        {
            std::string handsCpp = ReadText("src\\hands.cpp"), inputCpp = ReadText("src\\input.cpp"), readme = ReadText("README.md");
            handsCpp.erase(std::remove(handsCpp.begin(), handsCpp.end(), '\r'), handsCpp.end());
            inputCpp.erase(std::remove(inputCpp.begin(), inputCpp.end(), '\r'), inputCpp.end());
            if (handsCpp.empty() || inputCpp.empty() || readme.empty()) {
                Skip("t17h hands.cpp's, input.cpp's and README.md's lines (run from the project's folder)");
            } else {
                const char* const lines[] = {
                    "    const bool scripted = anywhere && f.frozen && !uf.asleep;\n    uf.cutscene = anywhere && input::InCutscene();",
                    "    const bool byHand = targeted && usepoint::ByHand(up.action, up.kind, up.hold, up.radius, anywhere);",
                    "        } else if (scripted) {\n",
                    "            if (input::TapPromptKey(up.action, \"a scripted moment: its own key, the game's input deciding\")) {\n"
                    "                MadeUse(out.use, up, up.action, atPrompt ? handWorld : nullptr);\n                g_useDoneBy[out.use] = up.entity;",
                    "        h.busy = h.busy || hr.busy[s] || uo.busy[s] || giftWay;",
                    "    h.busy = g.busy || g_useOwnsBy[side];",
                    "        unanswered = unanswered || (grip[0] == G::kHolds && grip[1] == G::kHolds);",
                    "constexpr ULONGLONG kUseGoneMs = 300;",
                    "    return now - s_shownAt[side] > kUseGoneMs;",
                };
                int found = 0;
                std::string missing;
                for (const char* l : lines) {
                    const bool here = handsCpp.find(l) != std::string::npos;
                    found += here;
                    if (!here && missing.size() < 200)
                        missing += std::string(" [") + std::string(l).substr(0, 60) + "]";
                }
                const bool input = inputCpp.find("bool InCutscene() {\n    return g_qteCutscene.load(std::memory_order_relaxed);\n}") != std::string::npos &&
                                   inputCpp.find("        in.heldYoung = qte::KeyKeepsGun(s_heldEAge, (in.await & qte::kKeyE) != 0 && qte::InPlay(in), in.nowMs);") !=
                                       std::string::npos;
                int control = 0;
                for (const char c : readme)
                    control += static_cast<unsigned char>(c) < 0x20 && c != '\r' && c != '\n' && c != '\t';
                std::snprintf(what, sizeof(what), "t17h hands.cpp's lines (%d of %d%s), input.cpp's (%d), README.md's control characters %d (0)", found,
                              static_cast<int>(std::size(lines)), missing.c_str(), input, control);
                Check(found == static_cast<int>(std::size(lines)) && input && control == 0, what);
            }
        }
    }

    // ---- t18: the grip as the interact button (the owner, 2026-11-26, in the headset: "the door's open with grip, but it takes a while, and it's not
    // very responsive ... make those grip prompts very responsive, like even a click will open whatever. That's like the interactive button"): a click
    // at once, the click of the hand on the gun's front, the use key tapped with no prompt the mod sees, the belly with a gun in a hand. At 72 Hz. ----
    {
        const float point[3] = {1.0f, 0.0f, 2.0f}, far1[3] = {-0.5f, 1.2f, 0.5f}, away[3] = {3.0f, 1.0f, 3.0f};
        usepoint::State st;
        usepoint::Out o;
        using G = usepoint::Grip;
        using B = usepoint::TapBar;
        // a. The bank's breach replayed (03:25:01-03): the right's clicks, 0.14 s and 0.11 s long, 0.74 s and 0.42 s apart, then a squeeze held; a free
        // hand 2.4 m off, still and clear; the prompt shows till its use is made (served 3 frames after the ask: a door's one use). The first click uses it
        // while it's held, 0.1 s on (12, Ask::kStill; that night three clicks did nothing and the fourth, held 0.44 s, did it), once. The same hand resting
        // by the belly with the rifle in the other (not clear): its first click used as it's let go (15, Ask::kLetGo). A click of one frame (let go at
        // 6): asked as it's let go (6, its squeeze's age as it was let go: one frame, Out::age); of two: at 7 (two frames).
        {
            const auto bank = [&](bool clear, int* firstAsk, usepoint::Ask* how) {
                usepoint::Clear(st);
                int asks = 0, servedAt = -1;
                *firstAsk = -1;
                for (int i = 0; i < 140; ++i) {
                    const bool down = (i >= 5 && i < 15) || (i >= 58 && i < 66) || i >= 88;
                    usepoint::Frame f = UseFrameAt(point, away, far1, 0.0f, down ? 1.0f : 0.0f, true);
                    f.hand[1].clear = clear;
                    f.point.on = servedAt < 0 || i < servedAt;
                    usepoint::Update(f, st, o);
                    if (o.use == 1) {
                        ++asks;
                        if (*firstAsk < 0)
                            *firstAsk = i, *how = o.how[1], servedAt = i + 3;
                    }
                }
                return asks;
            };
            int firstAsk = -1, restAsk = -1;
            usepoint::Ask how = usepoint::Ask::kNone, restHow = usepoint::Ask::kNone;
            const int asks = bank(true, &firstAsk, &how), restAsks = bank(false, &restAsk, &restHow);
            const auto clickOf = [&](int frames, float* age) {
                usepoint::Clear(st);
                int at = -1, n = 0;
                for (int i = 0; i < 30; ++i) {
                    usepoint::Update(UseFrameAt(point, far1, away, i >= 5 && i < 5 + frames ? 1.0f : 0.0f, 0.0f, true), st, o);
                    if (o.use == 0) {
                        ++n;
                        if (at < 0)
                            at = i, *age = o.how[0] == usepoint::Ask::kLetGo ? o.age[0] : -1.0f;
                    }
                }
                return n == 1 ? at : -100 - n;
            };
            float age1 = -1.0f, age2 = -1.0f;
            const int one = clickOf(1, &age1), two = clickOf(2, &age2);
            const float dt = 1.0f / 72.0f;
            std::snprintf(what, sizeof(what), "t18a the bank's breach replayed, clicks of 0.14 s and 0.11 s: the first used at frame %d (12, held, still %d), %d use(s) (1); "
                                              "resting by the belly: at %d (15, let go %d), %d use(s) (1); a click of one frame: asked at %d (6, as it's let go, %.1f ms "
                                              "held); of two: at %d (7, %.1f ms)",
                          firstAsk, how == usepoint::Ask::kStill, asks, restAsk, restHow == usepoint::Ask::kLetGo, restAsks, one,
                          static_cast<double>(age1 * 1000.0f), two, static_cast<double>(age2 * 1000.0f));
            Check(firstAsk == 12 && how == usepoint::Ask::kStill && asks == 1 && restAsk == 15 && restHow == usepoint::Ask::kLetGo && restAsks == 1 && one == 6 &&
                      std::fabs(age1 - dt) < 1e-4f && two == 7 && std::fabs(age2 - 2.0f * dt) < 1e-4f,
                  what);
        }
        // b. The hand on the gun's front (Hand::front: the left on the handguard, the right on the pistol grip), both on the rifle at a door's prompt (the
        // cellar's start prompt, 03:25:36-38: "a free hand's grip answers" was all there was). The left lets go at 20 (open 20-29, 0.14 s) and squeezes again
        // at 30 in place: a click (Out::support at 30: the hands take its hold on the front again with it that frame), its use asked the frame after, once
        // its hold is (31, Ask::kFront: the review of 01a5132, finding 2), a tick; its squeeze spent every frame from 30, never claimed nor busy (the
        // handguard's again); GripOf: waiting for the front at 30, asked at 40. Let go 0.5 s (20-55): no click (the squeeze by the gun the gun's):
        // nothing. Back 20 cm off where it let go: nothing. The pistol-grip hand let go and squeezed (never front): nothing. The AT4's prompt (it gives a
        // gun) with the gun in the hands: nothing, no refusal. Clicked again (let go 45-49, squeezed at 50): a use again at 51 (one a squeeze). The gun
        // gone from the other hand meanwhile (dropped at 25: Hand::otherGrip false), the front's let-go and squeeze: nothing (a catch's, the gun's).
        {
            struct FrontRun {
                int askedAt = -1, asks = 0, supportAt = -1, claimed = 0, busy = 0, spent = 0, ticks = 0, refused = 0, secondAt = -1;
                usepoint::Ask how = usepoint::Ask::kNone;
                G grip = G::kOpen, gripAt30 = G::kOpen;
            };
            const float onFront[3] = {0.0f, 1.0f, 0.4f}, moved[3] = {0.2f, 1.0f, 0.4f};
            // side: the hand that lets go (0 the front's, 1 the pistol grip's); open: its let-go frames [from, to); again: a second let-go [from, to) or none;
            // droppedAt: the gun gone from the pistol-grip hand from that frame (-1 never).
            const auto run = [&](int side, int openFrom, int openTo, const float* backAt, bool gives, int againFrom = -1, int againTo = -1, int droppedAt = -1) {
                FrontRun r;
                usepoint::Clear(st);
                for (int i = 0; i < 90; ++i) {
                    const bool open = (i >= openFrom && i < openTo) || (i >= againFrom && i < againTo);
                    const bool back = i >= openTo;
                    usepoint::Frame f = UseFrameAt(point, back ? backAt : onFront, away, 1.0f, 1.0f, true);
                    f.point.gives = gives;
                    f.gunHeld = true;
                    for (int s = 0; s < 2; ++s) {
                        usepoint::Hand& u = f.hand[s];
                        const bool lets = s == side;
                        u.squeeze = lets && open ? 0.0f : 1.0f;
                        u.yield = true;  // (by the gun: within 25 cm of it, UseYields)
                        u.clear = false;
                        // (The front hand: on it till its let-go, again from the frame after its squeeze (hands.cpp's Support takes it with that squeeze);
                        // the pistol-grip hand: on the gun till its let-go, the gun on the front alone then.)
                        const bool holding = !lets || !(open || i == openTo || i == againTo);
                        u.otherGrip = s == 0 && !(side == 1 && i >= openFrom) && !(droppedAt >= 0 && i >= droppedAt);
                        u.front = u.otherGrip && holding;
                        u.empty = !holding;
                    }
                    usepoint::Update(f, st, o);
                    const int k = side;
                    if (o.use == k) {
                        ++r.asks;
                        if (r.askedAt < 0)
                            r.askedAt = i, r.how = o.how[k];
                        if (r.asks == 2)
                            r.secondAt = i;
                    }
                    if (o.support[k] && r.supportAt < 0)
                        r.supportAt = i;
                    r.claimed += o.claimed[k];
                    r.busy += o.busy[k];
                    r.spent += o.spent[k];
                    r.ticks += o.entered[k];
                    r.refused += o.refused[k];
                    if (i == 30)
                        r.gripAt30 = usepoint::GripOf(f, st, k);
                    if (i == 40)
                        r.grip = usepoint::GripOf(f, st, k);
                }
                return r;
            };
            const FrontRun click = run(0, 20, 30, onFront, false), slow = run(0, 20, 56, onFront, false), off = run(0, 20, 30, moved, false),
                           grip = run(1, 20, 30, onFront, false), at4 = run(0, 20, 30, onFront, true), twice = run(0, 20, 30, onFront, false, 45, 50),
                           dropped = run(0, 20, 30, onFront, false, -1, -1, 25);
            std::snprintf(what, sizeof(what), "t18b the hand on the gun's front let go 0.14 s and squeezed again in place: a click at %d (30), asked at %d (31, once its hold is "
                                              "taken again %d), a tick %d, spent %d frames (60), claimed %d, busy %d (0, 0), GripOf %d then %d (waiting, asked); let go 0.5 "
                                              "s: asks %d; back 20 cm off: asks %d; the pistol-grip hand: asks %d; the AT4's prompt: asks %d, refused %d (0, 0); clicked "
                                              "again: asks %d, the second at %d (51); the gun gone from the other hand: asks %d (0)",
                          click.supportAt, click.askedAt, click.how == usepoint::Ask::kFront, click.ticks, click.spent, click.claimed, click.busy,
                          static_cast<int>(click.gripAt30), static_cast<int>(click.grip), slow.asks, off.asks, grip.asks, at4.asks, at4.refused, twice.asks,
                          twice.secondAt, dropped.asks);
            Check(click.askedAt == 31 && click.how == usepoint::Ask::kFront && click.asks == 1 && click.supportAt == 30 && click.ticks == 1 && click.spent == 60 &&
                      click.claimed == 0 && click.busy == 0 && click.gripAt30 == G::kFrontWait && click.grip == G::kAsked && slow.asks == 0 && off.asks == 0 &&
                      grip.asks == 0 && at4.asks == 0 && at4.refused == 0 && twice.asks == 2 && twice.secondAt == 51 && dropped.asks == 0,
                  what);
        }
        // c. No prompt showing (Frame::tap kNone): a free hand's click at 5, still and clear, taps the use key by a prompt's times (12, held 0.1 s), once a
        // click (held on: no more); let go 40-49 and squeezed at 50: at 57; a click of one frame: at 6, as it's let go; both hands at once: one tap (the
        // other's Out::tapTwice). Something of the hands' takes it in its first frame (not empty at 6): no tap, Out::tapGone; its squeeze something
        // else's by 6 (yield: a shoulder's place, the gun): no tap, tapGone; at a holster's place from its crossing (yield at 5): nothing at all; the hand
        // holding something: nothing. Each bar: no tap, Out::tapBarred. A prompt in its gap (gone at 4, the click at 5): no tap (the prompt's: back at 10,
        // asked at 12); past its gap (gone at 4, the click at 40): at 47. Asleep: nothing, not even tapBarred. "usereach 1": never. The hand unread before
        // its tap: none. Held, not clear (by the belly with a gun in a hand): at 37, held 0.45 s.
        {
            struct TapRun {
                int tapAt = -1, taps = 0, gone = 0, barred = 0, twice = 0, asks = 0, askAt = -1;
                std::vector<int> at;
            };
            const auto tapRun = [&](int frames, const auto& grip, const auto& edit) {
                TapRun r;
                usepoint::Clear(st);
                for (int i = 0; i < frames; ++i) {
                    usepoint::Frame f = UseFrameAt(point, far1, away, grip(i), 0.0f, true);
                    f.point.on = false;
                    f.tap = B::kNone;
                    edit(i, f);
                    usepoint::Update(f, st, o);
                    if (o.tap >= 0) {
                        ++r.taps;
                        r.at.push_back(i);
                        if (r.tapAt < 0)
                            r.tapAt = i;
                    }
                    r.gone += o.tapGone[0] + o.tapGone[1];
                    r.barred += o.tapBarred[0] + o.tapBarred[1];
                    r.twice += o.tapTwice[0] + o.tapTwice[1];
                    if (o.use >= 0) {
                        ++r.asks;
                        if (r.askAt < 0)
                            r.askAt = i;
                    }
                }
                return r;
            };
            const auto none = [](int, usepoint::Frame&) {};
            const auto from5 = [](int i) { return i >= 5 ? 1.0f : 0.0f; };
            const TapRun t1 = tapRun(80, [](int i) { return i >= 5 && (i < 40 || i >= 50) ? 1.0f : 0.0f; }, none);
            const TapRun tOne = tapRun(30, [](int i) { return i == 5 ? 1.0f : 0.0f; }, none);
            const TapRun tBoth = tapRun(30, from5, [](int i, usepoint::Frame& f) { f.hand[1].squeeze = i >= 5 ? 1.0f : 0.0f; });
            const TapRun tTook = tapRun(30, from5, [](int i, usepoint::Frame& f) { f.hand[0].empty = i < 6; });
            const TapRun tYield = tapRun(30, from5, [](int i, usepoint::Frame& f) { f.hand[0].yield = i >= 6; });
            const TapRun tPlace = tapRun(30, from5, [](int i, usepoint::Frame& f) { f.hand[0].yield = i >= 5; });
            const TapRun tHolds = tapRun(30, from5, [](int, usepoint::Frame& f) { f.hand[0].empty = false; });
            int barredRight = 0, barredRuns = 0;
            for (const B b : {B::kOff, B::kFight, B::kAsleep, B::kQuickTime, B::kAfterUse, B::kScripted, B::kSeat, B::kPoseUnknown, B::kInteractPoint}) {
                const TapRun r = tapRun(30, from5, [b](int, usepoint::Frame& f) { f.tap = b; });
                ++barredRuns;
                barredRight += r.taps == 0 && r.barred == 1;
            }
            const TapRun tGap = tapRun(30, from5, [](int i, usepoint::Frame& f) { f.point.on = i < 4 || i >= 10; });
            const TapRun tPast = tapRun(60, [](int i) { return i >= 40 ? 1.0f : 0.0f; }, [](int i, usepoint::Frame& f) { f.point.on = i < 4; });
            const TapRun tAsleep = tapRun(30, from5, [](int, usepoint::Frame& f) { f.asleep = true; });
            const TapRun tReach = tapRun(30, from5, [](int, usepoint::Frame& f) { f.anywhere = false; });
            const TapRun tUnread = tapRun(30, from5, [](int i, usepoint::Frame& f) { f.hand[0].tracked = i != 8; });
            const TapRun tHeld = tapRun(60, from5, [](int, usepoint::Frame& f) { f.hand[0].clear = false; });
            std::snprintf(what, sizeof(what), "t18c no prompt showing: a free hand's click tapped at %d (12), %d tap(s) (2: again at %d, 57); a click of a frame at %d (6); both "
                                              "hands: %d tap, %d twice (1, 1); taken in its first frame: %d taps, gone %d (0, 1); something else's by its next: %d, %d (0, 1); "
                                              "at a place from its crossing: %d, %d (0, 0); holding something: %d, %d (0, 0); each bar: %d of %d barred (9); a prompt's gap: "
                                              "%d taps, asked at %d (0, 12); past it: at %d (47); asleep %d, %d; \"usereach 1\" %d; unread %d (all 0); held, not clear: at "
                                              "%d (37)",
                          t1.tapAt, t1.taps, t1.at.size() > 1 ? t1.at[1] : -1, tOne.tapAt, tBoth.taps, tBoth.twice, tTook.taps, tTook.gone, tYield.taps,
                          tYield.gone, tPlace.taps, tPlace.gone, tHolds.taps, tHolds.gone, barredRight, barredRuns, tGap.taps, tGap.askAt, tPast.tapAt,
                          tAsleep.taps, tAsleep.barred, tReach.taps, tUnread.taps, tHeld.tapAt);
            Check(t1.tapAt == 12 && t1.taps == 2 && t1.at[1] == 57 && tOne.tapAt == 6 && tOne.taps == 1 && tBoth.taps == 1 && tBoth.twice == 1 && tTook.taps == 0 &&
                      tTook.gone == 1 && tYield.taps == 0 && tYield.gone == 1 && tPlace.taps == 0 && tPlace.gone == 0 && tHolds.taps == 0 && tHolds.gone == 0 &&
                      barredRight == barredRuns && tGap.taps == 0 && tGap.askAt == 12 && tPast.tapAt == 47 && tPast.taps == 1 && tAsleep.taps == 0 &&
                      tAsleep.barred == 0 && tReach.taps == 0 && tUnread.taps == 0 && tHeld.tapAt == 37 && tHeld.taps == 1,
                  what);
        }
        // d. TapBarOf: off first, then a fight, free hands asleep, a quick-time trigger live or key awaited, a prompt's use a moment ago, a scripted
        // moment, a seat, the pose not known, a use point on the interact key not a hand's; all clear: the use key's (kNone); every bar has its words.
        {
            usepoint::TapIn clear;
            clear.on = clear.awake = true;
            clear.onFoot = 1;
            const auto with = [&](const auto& edit) {
                usepoint::TapIn in = clear;
                edit(in);
                return usepoint::TapBarOf(in);
            };
            const bool bars = usepoint::TapBarOf(clear) == B::kNone && with([](usepoint::TapIn& in) { in.on = false, in.fight = true; }) == B::kOff &&
                              with([](usepoint::TapIn& in) { in.fight = true, in.awake = false; }) == B::kFight &&
                              with([](usepoint::TapIn& in) { in.awake = false, in.quickTime = true; }) == B::kAsleep &&
                              with([](usepoint::TapIn& in) { in.quickTime = true, in.afterUse = true; }) == B::kQuickTime &&
                              with([](usepoint::TapIn& in) { in.afterUse = true, in.scripted = true; }) == B::kAfterUse &&
                              with([](usepoint::TapIn& in) { in.scripted = true, in.onFoot = 0; }) == B::kScripted &&
                              with([](usepoint::TapIn& in) { in.onFoot = 0, in.interactPoint = true; }) == B::kSeat &&
                              with([](usepoint::TapIn& in) { in.onFoot = -1; }) == B::kPoseUnknown &&
                              with([](usepoint::TapIn& in) { in.onFoot = 2; }) == B::kPoseUnknown &&
                              with([](usepoint::TapIn& in) { in.interactPoint = true; }) == B::kInteractPoint && usepoint::TapBarOf(usepoint::TapIn{}) == B::kOff;
            bool words = true;
            for (int b = 0; b <= static_cast<int>(B::kInteractPoint); ++b)
                words = words && std::strlen(usepoint::TapWord(static_cast<B>(b))) > 0;
            words = words && std::strstr(usepoint::TapWord(B::kOff), "faulted") == nullptr;  // (the tap is the player's own key: the game's use faulted bars it not)
            std::snprintf(what, sizeof(what), "t18d TapBarOf: each bar alone and its order %d; every bar's words %d", bars, words);
            Check(bars && words, what);
        }
        // e. The belly held anywhere with a gun in a hand (BellyTakes, UseYields' body rule): the owner's nine squeezes by the belly at the cellar's door
        // (03:25:39-46, the log's "Pouch:" lines: cm right, up and forward of the neck point, 1.13-1.57 of the zone's size, "outside it" every time):
        // none the belly's now (the door's), at least eight of them the old rule's (holster::AtBodyTake, the holsters' settings: kept from the door, taken
        // by nothing); the pouch's own takes that night (03:28:41.663 at 0.39 of its size, 03:28:43.989 at 0.98): the belly's; its middle: the belly's;
        // not a number: not.
        {
            const float owner[9][3] = {{7, -34, 27}, {9, -37, 25}, {7, -37, 25}, {7, -38, 25}, {2, -38, 27}, {1, -44, 31}, {2, -44, 30}, {1, -44, 30}, {3, -36, 31}};
            const float takes[2][3] = {{3, -45, 17}, {3, -28, 15}};
            const holster::Settings hs;
            int belly = 0, old = 0, taken = 0;
            for (const auto& c : owner) {
                const float p[3] = {c[0] / 100.0f, c[1] / 100.0f, c[2] / 100.0f};
                belly += usepoint::BellyTakes(p);
                old += holster::AtBodyTake(p, hs);
            }
            for (const auto& c : takes) {
                const float p[3] = {c[0] / 100.0f, c[1] / 100.0f, c[2] / 100.0f};
                taken += usepoint::BellyTakes(p) && ammozone::Inside(p);
            }
            const float middle[3] = {ammozone::kBelly[0], ammozone::kBelly[1], ammozone::kBelly[2]}, nan3[3] = {std::nanf(""), -0.43f, 0.13f};
            std::snprintf(what, sizeof(what), "t18e the belly with a gun in a hand: the owner's nine squeezes the belly's %d (0), the old rule's %d (8 or 9); the pouch's two "
                                              "takes %d (2); its middle %d, not a number %d (1, 0)",
                          belly, old, taken, usepoint::BellyTakes(middle), usepoint::BellyTakes(nan3));
            Check(belly == 0 && old >= 8 && taken == 2 && usepoint::BellyTakes(middle) && !usepoint::BellyTakes(nan3), what);
        }
        // f. hands.cpp's, input.cpp's and pickups.cpp's lines for these, word for word (no test compiles them): the gun's front for the click, the tap's
        // bars from the game's moment, its hold taken again with the click's squeeze, a hand whose take of a gun off the ground waits no free one, the
        // use key tapped by input's own rule, the soldier on foot read from the pose (a seat 0), pickups.txt "interact".
        {
            std::string handsCpp = ReadText("src\\hands.cpp"), inputCpp = ReadText("src\\input.cpp"), pickupsCpp = ReadText("src\\pickups.cpp");
            for (std::string* t : {&handsCpp, &inputCpp, &pickupsCpp})
                t->erase(std::remove(t->begin(), t->end(), '\r'), t->end());
            if (handsCpp.empty() || inputCpp.empty() || pickupsCpp.empty()) {
                Skip("t18f hands.cpp's, input.cpp's and pickups.cpp's lines (run from the project's folder)");
            } else {
                const char* const lines[] = {
                    "        u.otherGrip = enabled ? (holder == Holder::kHeld && g_grasp.trigger == 1 - s) : s == 0;\n"
                    "        u.front = u.otherGrip && ((in.onSupport && !g_onM320By[s]) || (!enabled && s == 0 && g_pistolSupport));",
                    "    tapIn.on = anywhere && pickups::UseByHandOn() && pickups::Tuning().interactTap;\n    tapIn.fight = input::QteFightLive();\n"
                    "    tapIn.awake = enabled && !f.newLife && !g_bridging;\n    tapIn.quickTime = input::QteAnyLive();\n"
                    "    tapIn.afterUse = g_useAsked.serial != 0 || (g_useLastAt && now >= g_useLastAt && now - g_useLastAt < kTapAfterUseMs);\n"
                    "    tapIn.scripted = f.frozen || uf.cutscene;\n"
                    "    tapIn.onFoot = input::OnFoot();\n    tapIn.interactPoint = targeted && up.action == pickups::kUseInteract && !byHand;\n"
                    "    uf.tap = usepoint::TapBarOf(tapIn);",
                    "        if (out.support[s]) {",
                    "            g_freshBy[s] = true;",
                    "        u.empty = u.empty && (!anywhere || g_grasp.pendingSide != s);",
                    "        } else if (input::TapInteract(\"a free hand's click, no prompt the mod sees (the grip as the use key)\", &whyNot, key, sizeof(key))) {",
                    "        if (testing && out.tap == 0) {",
                };
                int found = 0;
                std::string missing;
                for (const char* l : lines) {
                    const bool here = handsCpp.find(l) != std::string::npos;
                    found += here;
                    if (!here && missing.size() < 200)
                        missing += std::string(" [") + std::string(l).substr(0, 60) + "]";
                }
                const char* const inputLines[] = {
                    "    case poseread::Miss::kSoldierType: return 0;",
                    "    WatchPoseFrame();\n    g_onFootNow.store(OnFootOf(), std::memory_order_relaxed);",
                    "    const inputrules::UseTapNo no = inputrules::UseTapRefused(in);",
                    "    g_useTapUntil = now + inputrules::kUseTapMs;\n    g_useTapByClick = true;",
                    "    in.seated = mountseat::Seated();\n    in.haveKey = k.dik != 0;",
                };
                int inputFound = 0;
                for (const char* l : inputLines)
                    inputFound += inputCpp.find(l) != std::string::npos;
                const bool pickups = pickupsCpp.find("        else if (std::strncmp(line, \"interact\", 8) == 0 && sscanf_s(line + 8, \"%d\", &n) == 1)\n"
                                                     "            v.tune.interactTap = n != 0;") != std::string::npos;
                std::snprintf(what, sizeof(what), "t18f hands.cpp's lines (%d of %d%s), input.cpp's (%d of %d), pickups.cpp's \"interact\" %d", found,
                              static_cast<int>(std::size(lines)), missing.c_str(), inputFound, static_cast<int>(std::size(inputLines)), pickups);
                Check(found == static_cast<int>(std::size(lines)) && inputFound == static_cast<int>(std::size(inputLines)) && pickups, what);
            }
        }
    }

    // ---- t19: the review of 01a5132 (findings 1-7, held anywhere). At 72 Hz. ----
    {
        const float point[3] = {1.0f, 0.0f, 2.0f}, far1[3] = {-0.5f, 1.2f, 0.5f}, away[3] = {3.0f, 1.0f, 3.0f};
        const float dt = 1.0f / 72.0f;
        usepoint::State st;
        usepoint::Out o;
        using G = usepoint::Grip;
        using B = usepoint::TapBar;
        // a. Finding 1, the belly: a free left hand reaching for a magazine at the belly with the rifle in the right while a door's prompt shows (and with
        // none: the use key's tap), the squeeze begun at Size `from` of the zone before it gets there (the pouch's 0.4 s reach window from the crossing),
        // straight on or from rest (minimum jerk); each frame its place, its yield as UseYields gives it (BellyTakes), clear as UseClear (the belly grown
        // by its put-back and kUseClearMore, 0.15 m) and by the body as UseNearBody (holster::AtBodyTake). The pouch takes inside its zone within 0.4 s of
        // the crossing unless the use has claimed it, the hand holding the magazine the frame after. The review's reaches (from 2.0, 30-380 ms; 01a5132:
        // the door every time) and from 3.0 and 4.0: the magazine every time, never the door nor the use key; the slow ones past the pouch's window (0.6 s
        // from 3.0 and 4.0, 0.5 s from 3.0): neither the door nor the use key, as f1552ad. The owner's nine squeezes at the cellar's door (03:25:39-46:
        // his right resting by the belly, the rifle in the left), clicks of 0.2 s: all the door's, as they're let go (f1552ad's: none; 01a5132's: all at
        // their next frame).
        {
            const holster::Settings hs;
            const auto reach = [&](float from, float T, bool minJerk, bool prompt) {
                usepoint::Clear(st);
                const auto place = [&](float t, float* p) {
                    float u = std::fmin(t / T, 1.0f);
                    if (minJerk)
                        u = u * u * u * (10.0f - 15.0f * u + 6.0f * u * u);
                    const float size = from - (from - 0.5f) * u;
                    p[0] = ammozone::kBelly[0], p[1] = ammozone::kBelly[1], p[2] = ammozone::kBelly[2] + ammozone::kBellyRadii[2] * size;
                };
                int who = 0;  // (0 nobody, 1 the magazine, 2 the door or the use key)
                bool pouchHas = false;
                for (int i = 0; i < 90; ++i) {
                    const float t = (i - 5) * dt;
                    float p[3];
                    place(t > 0.0f ? t : 0.0f, p);
                    usepoint::Frame f = UseFrameAt(point, p, away, i >= 5 ? 1.0f : 0.0f, 0.0f, true);
                    f.point.on = prompt;
                    f.tap = B::kNone;
                    f.gunHeld = true;
                    f.hand[0].yield = usepoint::BellyTakes(p);
                    f.hand[0].clear = !ammozone::Inside(p, ammozone::kPutBackMore + 0.10f);
                    f.hand[0].nearBody = holster::AtBodyTake(p, hs);
                    f.hand[0].empty = !pouchHas;
                    f.hand[1].empty = false;  // (the rifle)
                    usepoint::Update(f, st, o);
                    if ((o.use == 0 || o.tap == 0) && !who)
                        who = 2;
                    if (i >= 5 && !pouchHas && !o.claimed[0] && ammozone::Inside(p) && t <= 0.4f + 1e-4f) {
                        pouchHas = true;
                        if (!who)
                            who = 1;
                    }
                }
                return who;
            };
            int review = 0, more = 0, slowOther = 0;
            for (const float T : {0.03f, 0.06f, 0.10f, 0.15f, 0.20f, 0.30f, 0.38f})
                for (int pass = 0; pass < 4; ++pass)
                    review += reach(2.0f, T, (pass & 1) != 0, pass < 2) == 1;
            for (const float from : {3.0f, 4.0f})
                for (const float T : {0.03f, 0.10f, 0.20f, 0.30f, 0.38f})
                    for (int pass = 0; pass < 4; ++pass)
                        more += reach(from, T, (pass & 1) != 0, pass < 2) == 1;
            for (const float from : {3.0f, 4.0f})
                for (const float T : {0.5f, 0.6f})
                    for (int pass = 0; pass < 4; ++pass)
                        slowOther += reach(from, T, (pass & 1) != 0, pass < 2) == 2;
            const float owner[9][3] = {{7, -34, 27}, {9, -37, 25}, {7, -37, 25}, {7, -38, 25}, {2, -38, 27}, {1, -44, 31}, {2, -44, 30}, {1, -44, 30}, {3, -36, 31}};
            int ownerUsed = 0;
            for (const auto& c : owner) {
                const float p[3] = {c[0] / 100.0f, c[1] / 100.0f, c[2] / 100.0f};
                usepoint::Clear(st);
                int at = -1;
                for (int i = 0; i < 40; ++i) {
                    usepoint::Frame f = UseFrameAt(point, p, away, i >= 5 && i < 19 ? 1.0f : 0.0f, 0.0f, true);
                    f.gunHeld = true;
                    f.hand[0].yield = usepoint::BellyTakes(p);
                    f.hand[0].clear = !ammozone::Inside(p, ammozone::kPutBackMore + 0.10f);
                    f.hand[0].nearBody = holster::AtBodyTake(p, hs);
                    f.hand[1].empty = false;
                    usepoint::Update(f, st, o);
                    if (o.use == 0 && at < 0)
                        at = i;
                }
                ownerUsed += at == 19 && !usepoint::BellyTakes(p);
            }
            std::snprintf(what, sizeof(what), "t19a a reach for a magazine at the belly with a prompt or none: the review's (from 2.0, 30-380 ms, straight on and from rest) "
                                              "the magazine %d of 28; from 3.0 and 4.0 %d of 40; the slow ones the door or the use key %d of 16 (0); the owner's nine "
                                              "resting squeezes the door's as they're let go %d of 9",
                          review, more, slowOther, ownerUsed);
            Check(review == 28 && more == 40 && slowOther == 0 && ownerUsed == 9, what);
        }
        // b. Finding 1, a shoulder's draw and the handguard: a free hand moving to them, the squeeze begun on its way (at 0.5-1.5 m/s, 15-40 cm off), not
        // clear within kUseClearMore of them (UseClear), their place from its arrival (Hand::yield): never asked before it gets there, given up there
        // (the holster's or the handguard's), with a prompt showing; with none, never the use key: every reach that gets there within their 0.4 s reach
        // windows (18 runs). The slower ones (0.5-0.8 s: their windows over before it gets there, so the place takes nothing even with no prompt) asked
        // held kFreshAsk (37) on their way, as f1552ad (6 runs). A hand held still first, clear, 0.1 s away: the click's (t17a, t17b).
        {
            int given = 0, runs = 0, asked = 0, taps = 0, slowRuns = 0, slowAt37 = 0;
            for (const float speed : {0.5f, 0.8f, 1.2f, 1.5f})
                for (const float off : {0.15f, 0.25f, 0.40f})
                    for (int pass = 0; pass < 2; ++pass) {
                        usepoint::Clear(st);
                        const bool slow = off / speed > 0.4f + 1e-4f;
                        if (slow) {
                            ++slowRuns;
                            int at = -1;
                            for (int i = 0; i < 90; ++i) {
                                const float t = (i - 5) * dt;
                                const float left = std::fmax(off - speed * (t > 0.0f ? t : 0.0f), 0.0f);
                                const float at3[3] = {far1[0] + left, far1[1], far1[2]};
                                usepoint::Frame f = UseFrameAt(point, at3, away, i >= 5 ? 1.0f : 0.0f, 0.0f, true);
                                f.point.on = pass == 0;
                                f.tap = B::kNone;
                                f.hand[0].yield = left <= 0.0f;
                                f.hand[0].clear = left > 0.10f;
                                usepoint::Update(f, st, o);
                                if ((o.use == 0 || o.tap == 0) && at < 0)
                                    at = i;
                            }
                            slowAt37 += at == 37;
                            continue;
                        }
                        ++runs;
                        bool gave = false;
                        for (int i = 0; i < 90; ++i) {
                            const float t = (i - 5) * dt;
                            const float left = std::fmax(off - speed * (t > 0.0f ? t : 0.0f), 0.0f);  // (m from the place)
                            const float at[3] = {far1[0] + left, far1[1], far1[2]};
                            usepoint::Frame f = UseFrameAt(point, at, away, i >= 5 ? 1.0f : 0.0f, 0.0f, true);
                            f.point.on = pass == 0;
                            f.tap = B::kNone;
                            f.hand[0].yield = left <= 0.0f;
                            f.hand[0].clear = left > 0.10f;
                            usepoint::Update(f, st, o);
                            asked += o.use == 0;
                            taps += o.tap == 0;
                            gave = gave || o.yielded[0] || o.tapGone[0];
                        }
                        given += gave;
                    }
            std::snprintf(what, sizeof(what), "t19b a reach to a shoulder's place or the handguard, squeezed on its way, there within 0.4 s: given up there %d of %d (18), the "
                                              "door asked %d, the use key tapped %d (0, 0); slower: asked or tapped held at 37 %d of %d (6, as f1552ad)",
                          given, runs, asked, taps, slowAt37, slowRuns);
            Check(runs == 18 && given == runs && asked == 0 && taps == 0 && slowRuns == 6 && slowAt37 == slowRuns, what);
        }
        // c. Finding 2, the click on the gun's front asked only once its hold on the front is taken again: the left lets go of the handguard at 20 and
        // squeezes again at 23 10 cm toward the magazine well, where it takes a magazine instead (not empty from 24, no front): given up (kTook), never
        // asked; nothing takes its hold again (empty, no front: the hand floats): given up after kFrontWait (30), never asked, GripOf "not taken again";
        // the gun gone from the right hand the frame after the squeeze (dropped): given up; let go again before its hold was taken: nothing.
        {
            const float onFront[3] = {0.0f, 1.0f, 0.4f}, toWell[3] = {0.0f, 1.0f, 0.3f};
            const auto run = [&](int kind, int* asks, int* gaveAt, G* grip) {
                usepoint::Clear(st);
                *asks = 0, *gaveAt = -1;
                for (int i = 0; i < 70; ++i) {
                    const bool open = i >= 20 && i < 23;
                    usepoint::Frame f = UseFrameAt(point, i >= 23 ? toWell : onFront, away, 1.0f, 1.0f, true);
                    f.gunHeld = true;
                    for (int s = 0; s < 2; ++s)
                        f.hand[s].yield = true, f.hand[s].clear = false;
                    usepoint::Hand& u = f.hand[0];
                    u.squeeze = open || (kind == 3 && i >= 25 && i < 30) ? 0.0f : 1.0f;
                    u.otherGrip = !(kind == 2 && i >= 24);
                    u.front = u.otherGrip && i < 20;
                    u.empty = i >= 20 && !(kind == 0 && i >= 24);
                    f.hand[1].empty = !(kind == 2 && i >= 24) ? false : true;
                    usepoint::Update(f, st, o);
                    *asks += o.use == 0;
                    if ((o.given[0] || o.yielded[0]) && *gaveAt < 0)
                        *gaveAt = i;
                    if (i == 40 && grip)
                        *grip = usepoint::GripOf(f, st, 0);
                }
            };
            int magAsks = 0, magGave = 0, floatAsks = 0, floatGave = 0, dropAsks = 0, dropGave = 0, letAsks = 0, letGave = 0;
            G floatGrip = G::kOpen;
            run(0, &magAsks, &magGave, nullptr);
            run(1, &floatAsks, &floatGave, &floatGrip);
            run(2, &dropAsks, &dropGave, nullptr);
            run(3, &letAsks, &letGave, nullptr);
            std::snprintf(what, sizeof(what), "t19c a click on the gun's front: its squeeze a magazine's: asked %d, given up at %d (0, 24); the hold not taken again: asked %d, "
                                              "given up at %d (0, 30), GripOf %d; the gun dropped: asked %d, given up at %d (0, 24); let go before its hold: asked %d (0)",
                          magAsks, magGave, floatAsks, floatGave, static_cast<int>(floatGrip), dropAsks, dropGave, letAsks);
            Check(magAsks == 0 && magGave == 24 && floatAsks == 0 && floatGave == 30 && floatGrip == G::kFrontLost && dropAsks == 0 && dropGave == 24 && letAsks == 0,
                  what);
        }
        // d. Finding 3, a use asked again only for the prompt it was for: a free hand's use (12) not made for a moment (Rearm), the squeeze held on, the
        // prompt gone 5 s, another shows: never its (Out::gone as the showing ended, never asked again); another prompt showing before the retry's time:
        // never its either; the same prompt: asked again 0.5 s on (48). A click (5-14, asked at 12 held still, let go at 15) whose use fails: owed (Rearm), asked
        // again 0.5 s on (48) while the same prompt shows, the hand not squeezed again, no tick; once more after another failure (84), never a third; never
        // for another prompt, nor once the showing is over, nor after a new squeeze (its own click, 20-22: asked as it's let go, 23, and asked again for
        // its own failures, 59 and 95; the first click's owed 48 never).
        {
            const auto held = [&](int prompt2From, uint32_t id2, int* askAgain, int* gone) {
                usepoint::Clear(st);
                *askAgain = -1, *gone = 0;
                int asks = 0;
                for (int i = 0; i < 520; ++i) {
                    usepoint::Frame f = UseFrameAt(point, far1, away, i >= 5 ? 1.0f : 0.0f, 0.0f, true);
                    f.point.id = i < prompt2From ? 7u : id2;
                    f.point.on = i < 20 || (prompt2From < 20 ? true : i >= prompt2From);
                    usepoint::Update(f, st, o);
                    if (o.use == 0) {
                        if (++asks == 1)
                            usepoint::Rearm(st, 0);
                        else if (*askAgain < 0)
                            *askAgain = i;
                    }
                    *gone += o.gone[0];
                }
            };
            int againOther = 0, goneOther = 0, againSoon = 0, goneSoon = 0, againSame = 0, goneSame = 0;
            held(380, 9u, &againOther, &goneOther);   // (prompt 1 to 19, nothing 20-379, prompt 2 from 380)
            held(14, 9u, &againSoon, &goneSoon);      // (prompt 2 from 14, before the retry's time)
            held(100000, 7u, &againSame, &goneSame);  // (prompt 1 throughout? no: gone at 20, the same id back never)
            usepoint::Clear(st);
            int sameAgain = -1;
            {
                int asks = 0;
                for (int i = 0; i < 120; ++i) {
                    usepoint::Update(UseFrameAt(point, far1, away, i >= 5 ? 1.0f : 0.0f, 0.0f, true), st, o);
                    if (o.use == 0 && ++asks == 1)
                        usepoint::Rearm(st, 0);
                    else if (o.use == 0 && sameAgain < 0)
                        sameAgain = i;
                }
            }
            // A click let go (5-14, asked at 12 while held, let go at 15), its use failing twice.
            const auto letGo = [&](int newSqueezeAt, uint32_t idFrom60, int showUntil, std::vector<int>* at, int* ticks) {
                usepoint::Clear(st);
                at->clear(), *ticks = 0;
                for (int i = 0; i < 200; ++i) {
                    const bool down = (i >= 5 && i < 15) || (newSqueezeAt >= 0 && i >= newSqueezeAt && i < newSqueezeAt + 3);
                    usepoint::Frame f = UseFrameAt(point, far1, away, down ? 1.0f : 0.0f, 0.0f, true);
                    f.point.id = i < 60 ? 7u : idFrom60;
                    f.point.on = i < showUntil;
                    usepoint::Update(f, st, o);
                    *ticks += o.entered[0];
                    if (o.use == 0) {
                        at->push_back(i);
                        if (at->size() <= 3)
                            usepoint::Rearm(st, 0);  // (each use not made for a moment)
                    }
                }
            };
            std::vector<int> owed, owedOther, owedOver, owedNew;
            int ticks = 0, ticksOther = 0, ticksOver = 0, ticksNew = 0;
            letGo(-1, 7u, 1000, &owed, &ticks);
            letGo(-1, 9u, 1000, &owedOther, &ticksOther);
            letGo(-1, 7u, 30, &owedOver, &ticksOver);
            letGo(20, 7u, 1000, &owedNew, &ticksNew);
            std::snprintf(what, sizeof(what), "t19d a use asked again only for its prompt: held on, another prompt after the showing: asked again at %d (never), given up %d "
                                              "(1); another before the retry: %d (never), %d; the same prompt: at %d (48); a click let go owed: asked at %s (12, 48, 84), "
                                              "ticks %d (1); another prompt from 60: %d use(s) (2: 12, 48); the showing over at 30: %d (1); squeezed again at 20: %s "
                                              "(12, 23, 59, 95: the first's owed 48 dropped, the new click's own and its retries)",
                          againOther, goneOther, againSoon, goneSoon, sameAgain,
                          (std::to_string(owed.size() > 0 ? owed[0] : -1) + ", " + std::to_string(owed.size() > 1 ? owed[1] : -1) + ", " +
                           std::to_string(owed.size() > 2 ? owed[2] : -1))
                              .c_str(),
                          ticks, static_cast<int>(owedOther.size()), static_cast<int>(owedOver.size()),
                          [&] {
                              std::string t;
                              for (const int a : owedNew)
                                  t += (t.empty() ? "" : ", ") + std::to_string(a);
                              return t;
                          }()
                              .c_str());
            const std::vector<int> newExpected = {12, 23, 59, 95};
            Check(againOther < 0 && goneOther == 1 && againSoon < 0 && goneSoon == 1 && againSame < 0 && sameAgain == 48 && owed.size() == 3 && owed[0] == 12 &&
                      owed[1] == 48 && owed[2] == 84 && ticks == 1 && owedOther.size() == 2 && owedOver.size() == 1 && owedNew == newExpected,
                  what);
        }
        // e. Finding 4, the truck with the rifle in both hands: a click on the gun's front with no prompt the mod sees taps the use key once its hold on the
        // front is taken again (the frame after, Ask::kFront), never before; not taken again: no tap (Out::tapGone); a bar: Out::tapBarred.
        {
            const float onFront[3] = {0.0f, 1.0f, 0.4f};
            const auto run = [&](bool again, B bar, int* tapAt, int* gone, int* barred) {
                usepoint::Clear(st);
                *tapAt = -1, *gone = 0, *barred = 0;
                for (int i = 0; i < 60; ++i) {
                    usepoint::Frame f = UseFrameAt(point, onFront, away, 1.0f, 1.0f, true);
                    f.point.on = false;
                    f.tap = bar;
                    f.gunHeld = true;
                    for (int s = 0; s < 2; ++s)
                        f.hand[s].yield = true, f.hand[s].clear = false;
                    usepoint::Hand& u = f.hand[0];
                    u.squeeze = i >= 20 && i < 23 ? 0.0f : 1.0f;
                    u.otherGrip = true;
                    u.front = i < 20 || (again && i >= 24);
                    u.empty = !u.front;
                    f.hand[1].empty = false;
                    usepoint::Update(f, st, o);
                    if (o.tap == 0 && *tapAt < 0)
                        *tapAt = i;
                    *gone += o.tapGone[0];
                    *barred += o.tapBarred[0];
                }
            };
            int at = -1, gone = 0, barred = 0, lostAt = -1, lostGone = 0, lostBarred = 0, barAt = -1, barGone = 0, barBarred = 0;
            run(true, B::kNone, &at, &gone, &barred);
            run(false, B::kNone, &lostAt, &lostGone, &lostBarred);
            run(true, B::kSeat, &barAt, &barGone, &barBarred);
            std::snprintf(what, sizeof(what), "t19e a click on the gun's front, no prompt: tapped at %d (24); its hold not taken again: tapped %d, gone %d (never, 1); a seat: "
                                              "tapped %d, barred %d (never, 1)",
                          at, lostAt, lostGone, barAt, barBarred);
            Check(at == 24 && gone == 0 && lostAt < 0 && lostGone == 1 && barAt < 0 && barBarred == 1, what);
        }
        // f. Finding 5, a rescan's gap: a click on the gun's front whose squeeze again falls in a gap of 1 and 3 frames: asked as the prompt is back (and its
        // hold on the front taken: 24, 26); a free click of one frame made in a gap: the prompt's, asked as it's let go with the prompt back (6); let go
        // in a gap: owed, asked as it's back (7).
        {
            const auto front = [&](int gap) {
                usepoint::Clear(st);
                int at = -1;
                for (int i = 0; i < 60; ++i) {
                    usepoint::Frame f = UseFrameAt(point, far1, away, 1.0f, 1.0f, true);
                    f.point.on = !(i >= 23 && i < 23 + gap);
                    f.gunHeld = true;
                    for (int s = 0; s < 2; ++s)
                        f.hand[s].yield = true, f.hand[s].clear = false;
                    usepoint::Hand& u = f.hand[0];
                    u.squeeze = i >= 20 && i < 23 ? 0.0f : 1.0f;
                    u.otherGrip = true;
                    u.front = i < 20 || i >= 24;
                    u.empty = !u.front;
                    f.hand[1].empty = false;
                    usepoint::Update(f, st, o);
                    if (o.use == 0 && at < 0)
                        at = i;
                }
                return at;
            };
            const auto oneFrame = [&](int gapAt) {
                usepoint::Clear(st);
                int at = -1;
                for (int i = 0; i < 30; ++i) {
                    usepoint::Frame f = UseFrameAt(point, far1, away, i == 5 ? 1.0f : 0.0f, 0.0f, true);
                    f.point.on = i != gapAt;
                    usepoint::Update(f, st, o);
                    if (o.use == 0 && at < 0)
                        at = i;
                }
                return at;
            };
            const int g1 = front(1), g3 = front(3), inGap = oneFrame(5), letInGap = oneFrame(6);
            std::snprintf(what, sizeof(what), "t19f a rescan's gap: a click on the gun's front in a gap of 1 frame asked at %d (24), of 3 at %d (26); a free click of a frame made "
                                              "in a gap asked at %d (6), let go in one at %d (7, owed)",
                          g1, g3, inGap, letInGap);
            Check(g1 == 24 && g3 == 26 && inGap == 6 && letInGap == 7, what);
        }
        // g. Finding 6, a door walked past with a free fist held from before (begun free 30 frames before): targeted 3 frames (its 2 m edge crossed): never
        // used; 10 frames: never (under kHeldIntoShown); 12: used at its 10th frame (0.15 s shown).
        {
            const auto walk = [&](int frames) {
                usepoint::Clear(st);
                int at = -1;
                for (int i = 0; i < 80; ++i) {
                    usepoint::Frame f = UseFrameAt(point, far1, away, i >= 5 ? 1.0f : 0.0f, 0.0f, true);
                    f.point.on = i >= 35 && i < 35 + frames;
                    usepoint::Update(f, st, o);
                    if (o.use == 0 && at < 0)
                        at = i - 35;
                }
                return at;
            };
            const int w3 = walk(3), w10 = walk(10), w12 = walk(12);
            std::snprintf(what, sizeof(what), "t19g a door's edge walked past with a fist held: targeted 3 frames: used at %d (never); 10: %d (never); 12: at %d (10)", w3, w10, w12);
            Check(w3 < 0 && w10 < 0 && w12 == 10, what);
        }
        // h. Finding 7, the tap beside the fights and a prompt's use: TapBarOf's quick-time bar (input::QteAnyLive: a fist fight's six triggers between its
        // prompts, a key awaited however long, a frame not read) and its after-use bar (kTapAfterUse, 1 s); hands.cpp's inputs for them, input.cpp's
        // store of the uncapped flag with the fights' triggers counted (qtefight's Frame::live, qtewatch's), word for word (no test compiles them).
        {
            usepoint::TapIn in;
            in.on = in.awake = true;
            in.onFoot = 1;
            const bool none = usepoint::TapBarOf(in) == B::kNone;
            in.quickTime = true;
            const bool quick = usepoint::TapBarOf(in) == B::kQuickTime;
            in.quickTime = false, in.afterUse = true;
            const bool after = usepoint::TapBarOf(in) == B::kAfterUse && usepoint::kTapAfterUse >= 1.0f;
            std::string handsCpp = ReadText("src\\hands.cpp"), inputCpp = ReadText("src\\input.cpp"), fightH = ReadText("src\\qtefight.h"),
                        watchCpp = ReadText("src\\qtewatch.cpp");
            for (std::string* t : {&handsCpp, &inputCpp, &fightH, &watchCpp})
                t->erase(std::remove(t->begin(), t->end(), '\r'), t->end());
            if (handsCpp.empty() || inputCpp.empty() || fightH.empty() || watchCpp.empty()) {
                Skip("t19h hands.cpp's, input.cpp's, qtefight.h's and qtewatch.cpp's lines (run from the project's folder)");
                Check(none && quick && after, "t19h TapBarOf's quick-time and after-use bars");
            } else {
                const bool lines = handsCpp.find("    tapIn.quickTime = input::QteAnyLive();") != std::string::npos &&
                                   handsCpp.find("            g_useLastAt = now;") != std::string::npos &&
                                   handsCpp.find("    g_useMade.at = g_useLastAt = GetTickCount64();") != std::string::npos &&
                                   inputCpp.find("    g_qteAnyLiveNow.store(fight || (awaited && (awaited->live > 0 || awaited->await || awaited->barred || awaited->prompt || "
                                                 "awaited->unknown)),") != std::string::npos &&
                                   inputCpp.find("bool QteAnyLive() {\n    return g_qteAnyLiveNow.load(std::memory_order_relaxed);\n}") != std::string::npos &&
                                   fightH.find("    out.live = groupCount;") != std::string::npos && watchCpp.find("    frame.live = f.live;") != std::string::npos;
                std::snprintf(what, sizeof(what), "t19h the tap beside the fights: TapBarOf's none %d, quick-time %d, after a use %d; the lines word for word %d", none, quick,
                              after, lines);
                Check(none && quick && after && lines, what);
            }
        }
        // i. The nits: a held-from-before squeeze let go on the frame the prompt is its is its own (Out::askedHeld), and a grip held from before never
        // taps the use key; hands.cpp's "Interact:" lines from Out (how, age), the use test's hand never taps, a squeeze moved by the belly the body's
        // (Hand::nearBody from UseNearBody), clear from UseClear.
        {
            usepoint::Clear(st);
            int taps = 0;
            for (int i = 0; i < 80; ++i) {
                usepoint::Frame f = UseFrameAt(point, far1, away, 1.0f, 0.0f, true);
                f.point.on = false;
                f.tap = B::kNone;
                if (i < 3)
                    f.hand[0].tracked = false;  // (squeezed before the state was read, read squeezed from 3: a grip held from before, as "usetest N held")
                usepoint::Update(f, st, o);
                taps += o.tap >= 0;
            }
            usepoint::Clear(st);
            bool heldOwn = false;
            for (int i = 0; i < 40; ++i) {
                usepoint::Frame f = UseFrameAt(point, far1, away, i >= 2 && i < 20 ? 1.0f : 0.0f, 0.0f, true);
                f.point.on = i >= 9;
                usepoint::Update(f, st, o);
                if (o.use == 0)
                    heldOwn = o.askedHeld[0] && o.how[0] != usepoint::Ask::kLetGo;
            }
            std::string handsCpp = ReadText("src\\hands.cpp");
            handsCpp.erase(std::remove(handsCpp.begin(), handsCpp.end(), '\r'), handsCpp.end());
            const bool lines = handsCpp.empty() ||
                               (handsCpp.find("        u.clear = placesWanted && UseClear(s, f, in, hr, uf.gunHeld);\n        u.nearBody = placesWanted && UseNearBody(in, uf.gunHeld);") !=
                                    std::string::npos &&
                                handsCpp.find("            if (out.askedHeld[out.use])") != std::string::npos &&
                                handsCpp.find("    const auto howWords = [&](int s, char* words, size_t size) {") != std::string::npos &&
                                handsCpp.find("    u.clear = anywhere && UseClear(0, f, hand, hr, uf.gunHeld);\n    u.nearBody = anywhere && UseNearBody(hand, uf.gunHeld);") !=
                                    std::string::npos);
            if (handsCpp.empty())
                Skip("t19i hands.cpp's lines (run from the project's folder)");
            std::snprintf(what, sizeof(what), "t19i a grip held from before taps %d (0); one held into the prompt, asked as held from before %d; hands.cpp's lines %d", taps,
                          heldOwn, lines);
            Check(taps == 0 && heldOwn && lines, what);
        }
    }

    std::printf("%d checks, %d failed, %d skipped\n", g_checks, g_failed, g_skipped);
    return g_failed;
}

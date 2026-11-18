// Blackburn's arms whole up to the shoulders on the bare arms' levels (Semper Fidelis, The Great Destroyer; the user, 2026-11-18: the camo sleeves, the shoulders and
// the upper body as the levels with gloves have them, not forearms cut off past the elbow) without the game: src/barestate.h's parts, src/barehold.h's sleeve tables
// and placing, src/armik.h's solver and collar, as skin.cpp's DrawBareArms / DrawBareSleeves and hands.cpp's PlaceBareForearms run them. Each check prints PASS, FAIL
// or SKIP, then the totals; the exit code is the number failed.
//   s1  the draws: the torso is 5604 from 0 handcuffed and 5604 from 7545 with the cuffs off, the legs 7545 from 5604 and 7545 from 0 (the names were the wrong way
//       round until now), the strip 222 from 13149 in both, the depth-only copies and the chain as they were; every one of them still the body (KindOf as before);
//       another base vertex is none
//   s2  the tables: the torso's 13 entries and the strip's 7, each arm with its collar, its upper arm's two bones (and in the torso its hem on the elbow end), the
//       right's the left's mirrored; PlaceForearmBones with the sleeve on puts 47 / 48 and 21 / 22 on the upper arm and 51 / 25 on the elbow end, nothing capped
//       whatever the cap asked; with it off the very numbers as before (the rim on the elbow end, drawn in by the cap)
//   s3  nothing on the levels with gloves: of the draws recorded there (tools/barestatetest_draws.inc, 33 recordings) and the mod's other tables' none is the torso,
//       the strip or any part of this body but the chain's 3492 from 0 (a gun's piece there, as before); PartOf says a part exactly where KindOf says the body
//   s4  a sleeve only on an arm solved whole (FromRig): a forearm gone whole with its hand has none whatever was asked; one folded away has none; PlaceSleeves with
//       a side off folds that side's every bone and the chest onto the chest's point and leaves the other side's as with both on; with both off everything is on
//       the point (nothing drawn)
//   s5  the mesh (build\mesh_18108_16053_vb0.bin / _ib.bin, the handcuffed buffer whole; SKIP without it): every 'l' entry's vertices lie on the left (x > 0), every
//       'r' entry's on the right (this found the design's table wrong: the torso's 8, taken for a left bone, is the right collar), the hems' round the elbow
//       joint (the rolled sleeve's wide end: within 15 cm), the collars' and upper arms' reach the shoulder joint; in the bind pose the arms' own skin ends 3 cm
//       and more up inside the sleeve's rim, along the upper arm, all the way round it
//   s6  the recordings (build\survey: AKS74u, UMP45, M1014 handcuffed, aks_test_idle with the cuffs off; SKIP without them), the game's own frames:
//       a  the same skeleton: in every recorded frame the torso's bones 1, 0, 4, 9, 10, 12 are the arms' 47, 48, 50, 21, 22, 24 bit for bit, and the strip's 0, 1,
//          3, 4, 5, 6 the torso's 0, 1, 3, 8, 9, 10; armik.h's joints through the game's own bones: collar and upper arm meet at the shoulder, upper arm, hem and
//          the arms' elbow end at the elbow (within 0.2 mm), the upper arm 0.3004 m and the forearm 0.2608 m (within 0.5 mm)
//       b  posed from each frame: each arm solved (the bare arms' numbers, the reach limit on) from the game's own palm as the hand and the game's own shoulder,
//          then again with the shoulders 30 cm and 60 cm further back (hands beyond the arm's reach, the shoulder brought along up to 26 cm, the collar drawn in
//          to 3%); the torso's, the strip's and the arms' palettes made as the mod draws
//          them. The sleeve joins the forearm at the elbow: the upper arm's, the hem's, the arms' 47 / 21 and 50 / 24 elbow joints in one place (within 0.1 mm).
//          No bone stretched: the upper arm its 0.3004 m and the forearm its 0.2608 m (within 0.1 mm), every arm bone rigid, the collar the chest's axes whole or
//          drawn in (3% at least), its shoulder joint on the upper arm's far end wherever the shoulder was brought along, and never further from the upper arm's own
//          than the reach limit (5 cm) where it wasn't; the wrist on the hand. Both cuff states.
//       c  with the mesh: skinned with those palettes, every vertex of the torso and strip wholly on the chest's bones is on the chest's point; no edge of the
//          cloth between two vertices wholly on one arm's bones is longer than 1.25 times its bind length, nor grown more than the game's own frames grow one (5
//          mm); the arms' own skin round the elbow (under the sleeve: its upper arm's bones now on the upper arm, where they were rigid on the forearm's end) bends
//          as the game's elbow does: its edges grown no more than 1.25 times what the game's own frames grow them with the game's shoulders, twice with the hands
//          beyond reach (the forearm's elbow end takes half the hand's roll, weapon.txt "bareshare": that turns it on the upper arm, as the game's own does less);
//          the arms' open end stays up inside the sleeve (the rim 3 cm and more below it along the upper arm, all the way round), in reach and beyond: no cap
//          is needed with the sleeve on
// Usage: baresleevetest [<folder with the recordings> [<folder with the mesh dump>]] (build\survey and build by default, from the project's folder). Reads only.

#include "../src/barehold.h"
#include "../src/barestate.h"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace {

using barehold::detail::Point;
using Key = std::pair<uint32_t, uint32_t>;  // index count, start

int g_checks = 0, g_failed = 0, g_skipped = 0;
void Check(bool ok, const char* id, const char* format, ...) {
    char text[1536];
    va_list args;
    va_start(args, format);
    std::vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    ++g_checks;
    g_failed += !ok;
    std::printf("%s %-4s %s\n", ok ? "PASS" : "FAIL", id, text);
}
void Skip(const char* id, const char* what) {
    ++g_skipped;
    std::printf("SKIP %-4s %s\n", id, what);
}

float Dist(const float* a, const float* b) {
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}
bool SameBone(const float* a, const float* b) {  // bit for bit
    return std::memcmp(a, b, 12 * sizeof(float)) == 0;
}
// How far a bone's rotation is from a rigid one scaled by s: the worst entry of its rows' products against s*s times the identity.
float OffRigid(const float* m, float s = 1.0f) {
    float worst = 0;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            const float d = m[i * 4] * m[j * 4] + m[i * 4 + 1] * m[j * 4 + 1] + m[i * 4 + 2] * m[j * 4 + 2];
            worst = std::max(worst, std::fabs(d - (i == j ? s * s : 0.0f)));
        }
    return worst;
}
void Joint(const float* joint, int side, float* out) {  // armik.h's joint on that side (0 left, 1 right)
    out[0] = side == 0 ? joint[0] : -joint[0], out[1] = joint[1], out[2] = joint[2];
}

// ---- s1 ----
void Draws() {
    using namespace barestate;
    const bool torso = PartOf(5604, 0, 0) == kTorso && PartOf(5604, 7545, 0) == kTorso && TorsoState(5604, 0, 0) == 0 && TorsoState(5604, 7545, 0) == 1 &&
                       TorsoState(7545, 0, 0) == -1 && TorsoState(7545, 5604, 0) == -1 && TorsoState(5604, 0, 1) == -1;
    const bool legs = PartOf(7545, 5604, 0) == kLegs && PartOf(7545, 0, 0) == kLegs;
    const bool strip = PartOf(222, 13149, 0) == kStripPart;
    const bool depth = PartOf(26346, 34161, 0) == kBodyDepth && PartOf(23664, 31479, 0) == kBodyDepth && PartOf(7815, 60507, 0) == kHandDepth && PartOf(7815, 55143, 0) == kHandDepth;
    const bool other = PartOf(3492, 3492, 0) == kPartOther && PartOf(3492, 0, 0) == kPartOther && PartOf(11970, 11970, 0) == kPartOther;
    const bool none = PartOf(18108, 16053, 0) == kPartNone && PartOf(18108, 13371, 0) == kPartNone && PartOf(2682, 13371, 0) == kPartNone && PartOf(5604, 0, 5) == kPartNone &&
                      PartOf(222, 13149, -1) == kPartNone && PartOf(5604, 5604, 0) == kPartNone && PartOf(0, 0, 0) == kPartNone;
    constexpr Key kBodyKeys[] = {{5604, 0}, {7545, 5604}, {26346, 34161}, {7815, 60507}, {7545, 0}, {5604, 7545}, {23664, 31479}, {7815, 55143}, {222, 13149}, {3492, 3492}, {3492, 0}, {11970, 11970}};
    bool body = true;
    for (const Key& k : kBodyKeys)
        body = body && KindOf(k.first, k.second, 0) == kBody && KindOf(k.first, k.second, 1) == kNone;
    Check(torso && legs && strip && depth && other && none && body, "s1",
          "the draws: the torso 5604 from 0 and from 7545 %s, the legs 7545 from 5604 and from 0 %s, the strip %s, the depth-only copies %s, the chain and the pistol's copy %s; "
          "the arms, the cuff, another base vertex no part %s; all twelve still the body %s",
          torso ? "ok" : "WRONG", legs ? "ok" : "WRONG", strip ? "ok" : "WRONG", depth ? "ok" : "WRONG", other ? "ok" : "WRONG", none ? "ok" : "WRONG", body ? "ok" : "WRONG");
}

// ---- s2 ----
void Tables() {
    using namespace barehold;
    int wrong = 0;
    // Each arm of each table: one collar, two upper arm bones, (the torso) one hem; the right's the left's in the same order.
    auto count = [&](const char* roles, const int8_t* map, int n, char side, int part) {
        int c = 0;
        for (int k = 0; k < n; ++k)
            c += roles[k] == side && map[k] == part;
        return c;
    };
    for (char side : {'l', 'r'}) {
        wrong += count(kSleeveTorsoRoles, kSleeveTorso, kSleeveTorsoBones, side, kFrameCollar) != 1 || count(kSleeveTorsoRoles, kSleeveTorso, kSleeveTorsoBones, side, kFrameUpper) != 2 ||
                 count(kSleeveTorsoRoles, kSleeveTorso, kSleeveTorsoBones, side, 0) != 1;
        wrong += count(kSleeveStripRoles, kSleeveStrip, kSleeveStripBones, side, kFrameCollar) != 1 || count(kSleeveStripRoles, kSleeveStrip, kSleeveStripBones, side, kFrameUpper) != 2;
    }
    for (int k = 0; k < kSleeveTorsoBones; ++k)
        wrong += (kSleeveTorsoRoles[k] == 'c') != (kSleeveTorso[k] < 0) || (kSleeveTorsoRoles[k] != 'c' && kSleeveTorsoRoles[k] != 'l' && kSleeveTorsoRoles[k] != 'r');
    for (int k = 0; k < kSleeveStripBones; ++k)
        wrong += (kSleeveStripRoles[k] == 'c') != (kSleeveStrip[k] < 0);
    wrong += kSleeveTorsoBones != 13 || kSleeveStripBones != 7 || kSleeveTorsoRoles[2] != 'c' || kSleeveStripRoles[2] != 'c';
    // PlaceForearmBones.
    float frames[2][kArmFrames][12];
    for (int s = 0; s < 2; ++s)
        for (int f = 0; f < kArmFrames; ++f)
            for (int i = 0; i < 12; ++i)
                frames[s][f][i] = 100.0f * s + 10.0f * f + i + 0.5f;
    int onWrong = 0, offWrong = 0, others = 0;
    for (int side = 0; side < 2; ++side)
        for (float cap : {0.0f, 0.4f, 1.0f}) {
            float bare[kBareBones * 12], was[kBareBones * 12];
            for (int i = 0; i < kBareBones * 12; ++i)
                bare[i] = was[i] = -3.0f - i;
            PlaceForearmBones(side, frames[side], cap, true, bare);
            const int wristEnd = side == 0 ? 46 : 20, a = side == 0 ? 49 : 23, end = side == 0 ? 50 : 24, up1 = side == 0 ? 47 : 21, up2 = side == 0 ? 48 : 22, helper = side == 0 ? 51 : 25;
            onWrong += !SameBone(bare + wristEnd * 12, frames[side][2]) || !SameBone(bare + a * 12, frames[side][1]) || !SameBone(bare + end * 12, frames[side][0]) ||
                       !SameBone(bare + up1 * 12, frames[side][kFrameUpper]) || !SameBone(bare + up2 * 12, frames[side][kFrameUpper]) || !SameBone(bare + helper * 12, frames[side][0]);
            for (int k = 0; k < kBareBones; ++k)
                if (k != wristEnd && k != a && k != end && k != up1 && k != up2 && k != helper)
                    others += !SameBone(bare + k * 12, was + k * 12);
            // Off: as DrawBareArms had it until now (the rim on the elbow end's frame, drawn in by the cap when there is one).
            std::memcpy(bare, was, sizeof(bare));
            PlaceForearmBones(side, frames[side], cap, false, bare);
            float capped[12];
            armik::CapFrame(frames[side][0], side == 1, cap, capped);
            const float* rim = cap > 0.0f ? capped : frames[side][0];
            offWrong += !SameBone(bare + wristEnd * 12, frames[side][2]) || !SameBone(bare + a * 12, frames[side][1]) || !SameBone(bare + end * 12, frames[side][0]) ||
                        !SameBone(bare + up1 * 12, rim) || !SameBone(bare + up2 * 12, rim) || !SameBone(bare + helper * 12, rim);
        }
    Check(wrong == 0 && onWrong == 0 && offWrong == 0 && others == 0, "s2",
          "the tables: %d entries wrong (the torso's 13 \"%s\", the strip's 7 \"%s\": a collar, two upper arm bones and in the torso a hem each side); the forearm's bones with the "
          "sleeve on: %d cases wrong of 6 (47 / 48, 21 / 22 on the upper arm, 51 / 25 on the elbow end, no cap); with it off %d wrong of 6 (as before, bit for bit); %d other bones touched",
          wrong, kSleeveTorsoRoles, kSleeveStripRoles, onWrong, offWrong, others);
}

// ---- s3 ----
constexpr Key kGloved[] = {
#include "barestatetest_draws.inc"
};
// The mod's own tables' draws on other levels (skin.cpp; barestatetest's list).
constexpr Key kOthers[] = {{13755, 15051}, {7617, 49995}, {8529, 0},     {21189, 28806}, {4506, 8529},   {2016, 13035}, {12768, 0},
                           {312, 12768},   {13080, 13080}, {7848, 0},    {1440, 7848},   {9288, 9288},   {9273, 12},    {9273, 9594},
                           {3414, 0},      {18222, 3414},  {120, 21636}, {21756, 21756}, {11970, 0},     {16188, 4689}, {20298, 354}};
void Gloved() {
    int posed = 0, parts = 0, chain = 0, differ = 0;
    auto look = [&](const Key& k) {
        const barestate::Part p = barestate::PartOf(k.first, k.second, 0);
        posed += p == barestate::kTorso || p == barestate::kStripPart;
        chain += p == barestate::kPartOther && k == Key{3492, 0};
        parts += p != barestate::kPartNone && !(k == Key{3492, 0});
        differ += (p != barestate::kPartNone) != (barestate::KindOf(k.first, k.second, 0) == barestate::kBody);
    };
    for (const Key& k : kGloved)
        look(k);
    for (const Key& k : kOthers)
        look(k);
    Check(posed == 0 && parts == 0 && differ == 0, "s3",
          "the levels with gloves: of %zu recorded draws and the mod's %zu others, %d the torso or the strip (0 wanted), %d any other part of this body but the chain's 3492 from 0 "
          "(%d times: a gun's piece there, hidden only with bare arms drawn moved, as before); a part and the body said apart %d times",
          std::size(kGloved), std::size(kOthers), posed, parts, chain, differ);
}

// ---- s4 ----
void OnlySolved() {
    using namespace barehold;
    float frames[2][kArmFrames][12] = {};
    for (int s = 0; s < 2; ++s)
        for (int f = 0; f < kArmFrames; ++f) {
            float* m = frames[s][f];
            m[0] = m[5] = m[10] = 1.0f;
            m[3] = 0.1f * f + (s ? -0.3f : 0.3f), m[7] = 0.2f, m[11] = -0.4f;
        }
    // FromRig: the gloves' palette the identity (each hand where the bind pose has it), the forearm solved 1 cm from the hand's wrist, then 5 cm.
    float gloves[kGloveBones * 12] = {}, bare[kBareBones * 12] = {};
    for (int k = 0; k < kGloveBones; ++k)
        gloves[k * 12] = gloves[k * 12 + 5] = gloves[k * 12 + 10] = 1.0f;
    const float fold[2][3] = {{1, 2, 3}, {4, 5, 6}};
    int wrong = 0;
    for (int trial = 0; trial < 3; ++trial) {
        for (int k = 0; k < kBareBones; ++k) {
            std::memset(bare + k * 12, 0, 12 * sizeof(float));
            bare[k * 12] = bare[k * 12 + 5] = bare[k * 12 + 10] = 1.0f;
        }
        float nearby[2][kArmFrames][12];
        std::memcpy(nearby, frames, sizeof(nearby));
        for (int s = 0; s < 2; ++s)
            for (int f = 0; f < kArmFrames; ++f)
                nearby[s][f][3] = nearby[s][f][7] = 0.0f, nearby[s][f][11] = trial == 1 ? 0.05f : 0.01f;  // the forearm's wrist that far from the hand's
        Arm arms[2];
        for (int s = 0; s < 2; ++s)
            arms[s].have = true, arms[s].frame = nearby[s], arms[s].sleeve = true;
        const bool here[2] = {true, trial != 2};
        const Placed placed = FromRig(gloves, here, fold, arms, bare);
        if (trial == 0)
            wrong += !(placed.solved[0] && placed.solved[1] && placed.sleeve[0] && placed.sleeve[1]) || !SameBone(bare + 47 * 12, nearby[0][kFrameUpper]) || !SameBone(bare + 21 * 12, nearby[1][kFrameUpper]);
        if (trial == 1)  // whole with the hand: no sleeve, the upper arm's bones the game's own moved with the palm (here: where they were)
            wrong += placed.solved[0] || placed.solved[1] || placed.sleeve[0] || placed.sleeve[1] || SameBone(bare + 47 * 12, nearby[0][kFrameUpper]);
        if (trial == 2)  // the right folded away
            wrong += !placed.sleeve[0] || placed.sleeve[1] || placed.solved[1] || bare[21 * 12 + 3] != 4.0f;
    }
    // PlaceSleeves.
    const float chest[3] = {0.5f, -0.25f, 0.125f};
    const float (*const both[2])[12] = {frames[0], frames[1]};
    int foldWrong = 0, keepWrong = 0;
    struct Table {
        const char* roles;
        const int8_t* map;
        int n;
    } const tables[] = {{kSleeveTorsoRoles, kSleeveTorso, kSleeveTorsoBones}, {kSleeveStripRoles, kSleeveStrip, kSleeveStripBones}};
    for (const Table& t : tables) {
        float all[16 * 12], some[16 * 12];
        const bool on[2] = {true, true};
        for (int i = 0; i < 16 * 12; ++i)
            all[i] = 9.0f;
        PlaceSleeves(t.roles, t.map, t.n, on, both, chest, all);
        for (int k = 0; k < t.n; ++k) {
            const float* b = all + k * 12;
            const bool folded = b[0] == 0 && b[1] == 0 && b[2] == 0 && b[4] == 0 && b[5] == 0 && b[6] == 0 && b[8] == 0 && b[9] == 0 && b[10] == 0 && b[3] == chest[0] && b[7] == chest[1] && b[11] == chest[2];
            if (t.roles[k] == 'c')
                foldWrong += !folded;
            else
                keepWrong += !SameBone(b, frames[t.roles[k] == 'r'][t.map[k]]);
        }
        for (int off = 0; off < 3; ++off) {  // the left off, the right off, both off
            const bool o[2] = {off == 1, off == 0};
            for (int i = 0; i < 16 * 12; ++i)
                some[i] = 9.0f;
            PlaceSleeves(t.roles, t.map, t.n, o, both, chest, some);
            for (int k = 0; k < t.n; ++k) {
                const float* b = some + k * 12;
                const int side = t.roles[k] == 'l' ? 0 : t.roles[k] == 'r' ? 1 : -1;
                const float at[3] = {0.3f, 0.7f, -0.2f};
                float p[3];
                Point(b, at, p);
                if (side >= 0 && o[side])
                    keepWrong += !SameBone(b, all + k * 12);
                else
                    foldWrong += Dist(p, chest) != 0.0f || OffRigid(b, 0.0f) != 0.0f;
            }
        }
    }
    Check(wrong == 0 && foldWrong == 0 && keepWrong == 0, "s4",
          "a sleeve only on an arm solved whole: FromRig %d cases wrong of 3 (solved 1 cm from the hand: sleeves on; 5 cm: whole with the hand, none; folded away: none); PlaceSleeves, "
          "the torso's and the strip's with both, one and no side on: %d entries not folded onto the chest's point that should be, %d not on their arm's frame",
          wrong, foldWrong, keepWrong);
}

// ---- The mesh ----
struct Vert {
    float p[3];
    uint8_t bone[4], weight[4];
};
struct Part {
    std::vector<Vert> verts;
    std::vector<uint16_t> index;
};
struct Mesh {
    Part torso, strip, arms;
};
float Half(uint16_t h) {
    const int e = (h >> 10) & 31, m = h & 1023;
    const float s = h & 0x8000 ? -1.0f : 1.0f;
    if (e == 0)
        return s * std::ldexp(static_cast<float>(m), -24);
    if (e == 31)
        return s * 1e30f;
    return s * std::ldexp(static_cast<float>(m + 1024), e - 25);
}
bool ReadFile(const std::string& path, std::vector<uint8_t>& bytes) {
    FILE* f = nullptr;
    if (fopen_s(&f, path.c_str(), "rb") != 0 || !f)
        return false;
    _fseeki64(f, 0, SEEK_END);
    const long long size = _ftelli64(f);
    _fseeki64(f, 0, SEEK_SET);
    bytes.resize(static_cast<size_t>(std::max(size, 0ll)));
    const bool read = size > 0 && fread(bytes.data(), 1, bytes.size(), f) == bytes.size();
    fclose(f);
    return read;
}
// One draw's part of the handcuffed buffer: count indices from start, its vertices from byte offset at, 48 bytes each (the place in halves, 4 bone numbers at 8, 4
// weights at 12).
bool TakePart(const std::vector<uint8_t>& vb, const std::vector<uint8_t>& ib, uint32_t start, uint32_t count, size_t at, int bones, Part& part) {
    if ((static_cast<size_t>(start) + count) * 2 > ib.size())
        return false;
    part.index.resize(count);
    std::memcpy(part.index.data(), ib.data() + start * 2, count * 2);
    const int n = *std::max_element(part.index.begin(), part.index.end()) + 1;
    if (at + static_cast<size_t>(n) * 48 > vb.size())
        return false;
    part.verts.resize(n);
    for (int v = 0; v < n; ++v) {
        const uint8_t* b = vb.data() + at + static_cast<size_t>(v) * 48;
        uint16_t h[3];
        std::memcpy(h, b, 6);
        Vert& out = part.verts[v];
        for (int i = 0; i < 3; ++i)
            out.p[i] = Half(h[i]);
        std::memcpy(out.bone, b + 8, 4);
        std::memcpy(out.weight, b + 12, 4);
        int sum = 0;
        for (int i = 0; i < 4; ++i) {
            sum += out.weight[i];
            if (out.weight[i] && out.bone[i] >= bones)
                return false;
        }
        if (sum != 255)
            return false;
    }
    return true;
}
bool LoadMesh(const std::string& dir, Mesh& mesh) {
    std::vector<uint8_t> vb, ib;
    if (!ReadFile(dir + "\\mesh_18108_16053_vb0.bin", vb) || !ReadFile(dir + "\\mesh_18108_16053_ib.bin", ib))
        return false;
    return TakePart(vb, ib, 0, 5604, 0, barehold::kSleeveTorsoBones, mesh.torso) && TakePart(vb, ib, 13149, 222, 143616, barehold::kSleeveStripBones, mesh.strip) &&
           TakePart(vb, ib, 16053, 18108, 180096, barehold::kBareBones, mesh.arms);
}
void Skin(const Vert& v, const float* palette, float* out) {
    out[0] = out[1] = out[2] = 0.0f;
    for (int i = 0; i < 4; ++i) {
        if (!v.weight[i])
            continue;
        float p[3];
        Point(palette + v.bone[i] * 12, v.p, p);
        const float w = v.weight[i] / 255.0f;
        out[0] += p[0] * w, out[1] += p[1] * w, out[2] += p[2] * w;
    }
}

// What a vertex of a sleeves' draw is: wholly the chest's (2), wholly one arm's (0 left, 1 right), or mixed (-1).
int Owner(const Vert& v, const char* roles) {
    int owner = -2;
    for (int i = 0; i < 4; ++i) {
        if (!v.weight[i])
            continue;
        const char r = roles[v.bone[i]];
        const int o = r == 'c' ? 2 : r == 'l' ? 0 : 1;
        if (owner == -2)
            owner = o;
        else if (owner != o)
            return -1;
    }
    return owner;
}
int ArmsOwner(const Vert& v) {  // the arms' draw: 0 left (26-51), 1 right (0-25), -1 both
    int owner = -2;
    for (int i = 0; i < 4; ++i) {
        if (!v.weight[i])
            continue;
        const int o = v.bone[i] >= barehold::kBareSide[0][0] ? 0 : 1;
        if (owner == -2)
            owner = o;
        else if (owner != o)
            return -1;
    }
    return owner;
}

// The arms' open end under the sleeve, all the way round. The arms' own skin stops some way up the upper arm (where the forearms were cut off without the sleeve); the
// sleeve's rolled rim hangs lower, a little above the elbow. Each side, in each twelfth of the way round the upper arm's line (shoulder joint to elbow joint): the
// sleeve's rim vertex there (the furthest down the arm of the torso's vertices wholly on that arm's bones) and the arms' end vertex there (the furthest up it). The rim
// further down than the end, by how much along the arm: the end is under the cloth there.
constexpr int kRound = 12;
struct Cover {
    int rim[2][kRound], end[2][kRound];  // vertices of the torso's draw and the arms'; -1: no pair in that twelfth (too few vertices there)
    float bind[2] = {};                  // the least of them in the bind pose
    int used[2] = {};
};
float Along(const float* p, const float* from, const float* axis) {
    return (p[0] - from[0]) * axis[0] + (p[1] - from[1]) * axis[1] + (p[2] - from[2]) * axis[2];
}
Cover MakeCover(const Mesh& mesh) {
    Cover c;
    for (int side = 0; side < 2; ++side) {
        float s[3], e[3];
        Joint(armik::kShoulder, side, s), Joint(armik::kElbow, side, e);
        float axis[3] = {e[0] - s[0], e[1] - s[1], e[2] - s[2]};
        const float length = armik::detail::Normalize(axis);
        float a[3] = {0, 0, 1}, b[3];  // across the arm
        armik::detail::Across(a, axis), armik::detail::Normalize(a);
        armik::detail::Cross(axis, a, b);
        auto twelfth = [&](const float* p) {
            const float d[3] = {p[0] - s[0], p[1] - s[1], p[2] - s[2]};
            const float turn = std::atan2(armik::detail::Dot(d, b), armik::detail::Dot(d, a)) / (2 * armik::kPi) + 0.5f;
            return std::clamp(static_cast<int>(turn * kRound), 0, kRound - 1);
        };
        float rimAt[kRound], endAt[kRound];
        for (int k = 0; k < kRound; ++k)
            c.rim[side][k] = c.end[side][k] = -1, rimAt[k] = -9.0f, endAt[k] = 9.0f;
        for (size_t v = 0; v < mesh.torso.verts.size(); ++v)
            if (const Vert& vert = mesh.torso.verts[v]; Owner(vert, barehold::kSleeveTorsoRoles) == side)
                if (const int k = twelfth(vert.p); Along(vert.p, s, axis) > rimAt[k])
                    rimAt[k] = Along(vert.p, s, axis), c.rim[side][k] = static_cast<int>(v);
        for (size_t v = 0; v < mesh.arms.verts.size(); ++v)
            if (const Vert& vert = mesh.arms.verts[v]; ArmsOwner(vert) == side && Along(vert.p, s, axis) < length + 0.02f)  // (the upper arm's part of it)
                if (const int k = twelfth(vert.p); Along(vert.p, s, axis) < endAt[k])
                    endAt[k] = Along(vert.p, s, axis), c.end[side][k] = static_cast<int>(v);
        c.bind[side] = 9.0f;
        for (int k = 0; k < kRound; ++k) {
            if (c.rim[side][k] < 0 || c.end[side][k] < 0 || rimAt[k] - endAt[k] < 0.02f) {  // (a twelfth with no vertex of the arms' end in it)
                c.rim[side][k] = c.end[side][k] = -1;
                continue;
            }
            ++c.used[side];
            c.bind[side] = std::min(c.bind[side], rimAt[k] - endAt[k]);
        }
    }
    return c;
}

// ---- s5 ----
void MeshTables(const Mesh& mesh, const Cover& cover) {
    using namespace barehold;
    int wrongSide = 0, farHem = 0, noShoulder = 0;
    float hemFurthest = 0;
    struct Table {
        const Part* part;
        const char* roles;
        const int8_t* map;
        int n;
    } const tables[] = {{&mesh.torso, kSleeveTorsoRoles, kSleeveTorso, kSleeveTorsoBones}, {&mesh.strip, kSleeveStripRoles, kSleeveStrip, kSleeveStripBones}};
    for (const Table& t : tables)
        for (int k = 0; k < t.n; ++k) {
            const int side = t.roles[k] == 'l' ? 0 : t.roles[k] == 'r' ? 1 : -1;
            if (side < 0)
                continue;
            float elbow[3], shoulder[3], nearestShoulder = 9.0f;
            Joint(armik::kElbow, side, elbow);
            Joint(armik::kShoulder, side, shoulder);
            int used = 0;
            for (const Vert& v : t.part->verts)
                for (int i = 0; i < 4; ++i)
                    if (v.weight[i] && v.bone[i] == k) {
                        ++used;
                        // (A collar's lightest vertices cross the spine by a centimetre or two: by its main ones.)
                        if (v.weight[i] >= 64)
                            wrongSide += side == 0 ? v.p[0] < 0.0f : v.p[0] > 0.0f;
                        if (t.map[k] == 0) {
                            hemFurthest = std::max(hemFurthest, Dist(v.p, elbow));
                            farHem += Dist(v.p, elbow) > 0.15f;
                        }
                        nearestShoulder = std::min(nearestShoulder, Dist(v.p, shoulder));
                    }
            if (t.map[k] != 0 && t.part == &mesh.torso)
                noShoulder += !(used > 0 && nearestShoulder < 0.10f);
        }
    // The arms' own skin under the sleeve, all the way round the upper arm (Cover).
    Check(wrongSide == 0 && farHem == 0 && noShoulder == 0 && cover.used[0] >= 10 && cover.used[1] >= 10 && cover.bind[0] >= 0.03f && cover.bind[1] >= 0.03f, "s5",
          "the mesh (%zu torso, %zu strip, %zu arms vertices): %d vertices of an arm's entry on the other side, %d of a hem's far from the elbow joint (the furthest %.1f "
          "cm; 15 allowed: the rolled sleeve's wide end), %d of the torso's collars and upper arms not reaching the shoulder joint; in the bind pose, along the upper arm, the "
          "arms' skin ends %.1f / %.1f cm up inside the sleeve's rim at the least (left / right, in %d / %d twelfths of the way round; 3 wanted): the open end the forearms "
          "were cut at is under the cloth",
          mesh.torso.verts.size(), mesh.strip.verts.size(), mesh.arms.verts.size(), wrongSide, farHem, hemFurthest * 100, noShoulder, cover.bind[0] * 100, cover.bind[1] * 100,
          cover.used[0], cover.used[1]);
}

// ---- The recordings ----
struct Record {
    int32_t head[8];  // the draw's index count, the frame, the eye, its start, the rounds, the spare, the ms, the bones' count
    const float* bones;
};
bool Load(const std::string& path, std::vector<uint8_t>& bytes, std::vector<Record>& records) {
    if (!ReadFile(path, bytes))
        return false;
    size_t at = 0;
    while (at + 32 <= bytes.size()) {
        Record r;
        std::memcpy(r.head, bytes.data() + at, 32);
        if (r.head[7] <= 0 || r.head[7] > 64 || at + 32 + static_cast<size_t>(r.head[7]) * 48 > bytes.size())
            break;
        r.bones = reinterpret_cast<const float*>(bytes.data() + at + 32);
        records.push_back(r);
        at += 32 + static_cast<size_t>(r.head[7]) * 48;
    }
    return !records.empty();
}

armik::Params BareParams(bool right) {  // hands.cpp's PlaceBareForearms: the wider wrist, the forearm taking the hand's roll, the elbow's reach limit
    armik::Params p;
    if (right)
        p.poleOut = 0.9f;
    p.flex = 100.0f, p.ext = 95.0f, p.radial = 45.0f, p.ulnar = 60.0f;
    p.share[0] = 0.5f, p.share[1] = 0.8f, p.share[2] = 1.0f;
    p.elbowReach = 0.05f;
    return p;
}

// One arm as hands.cpp's PlaceBareForearms makes it: solved from the hand and the shoulder given, its five frames.
struct ArmPose {
    float frames[barehold::kArmFrames][12];
    armik::Result r;
    float brought = 0, keep = 1, cap = 0;
    bool finite = true;
};
void SolveArm(const float* hand, const float* shoulder, const float* chestLeft, const float* up, const float* chestBack, int side, armik::State& state, ArmPose& pose) {
    float out[3];
    for (int i = 0; i < 3; ++i)
        out[i] = side == 0 ? chestLeft[i] : -chestLeft[i];
    armik::Solve(hand, shoulder, out, up, chestBack, side == 1, BareParams(side == 1), state, pose.r);
    std::memcpy(pose.frames, pose.r.frame, sizeof(pose.r.frame));
    std::memcpy(pose.frames[barehold::kFrameUpper], pose.r.upper, sizeof(pose.r.upper));
    pose.brought = Dist(pose.r.shoulder, shoulder);
    pose.keep = armik::CollarKeep(pose.brought);
    armik::Collar(chestLeft, up, chestBack, pose.r.shoulder, side == 1, pose.keep, pose.frames[barehold::kFrameCollar]);
    pose.cap = armik::EndCap(pose.r, shoulder);
    pose.finite = true;
    for (int f = 0; f < barehold::kArmFrames; ++f)
        for (int i = 0; i < 12; ++i)
            pose.finite = pose.finite && std::isfinite(pose.frames[f][i]);
}

// The worst of them: how many times its bind length an edge is (ratio), and how much longer in metres (grown).
struct Stretched {
    float ratio = 0, grown = 0;
};
struct Totals {
    // a
    int frames = 0, pairs = 0, notSame = 0, stripNotSame = 0;
    float gameShoulder = 0, gameElbow = 0, gameUpper = 0, gameFore = 0;
    // b
    int poses = 0, beyond = 0, broughtFar = 0, held = 0, notFinite = 0, capped = 0;
    float elbow = 0, upper = 0, fore = 0, wrist = 0, rigid = 0, collarRigid = 0, collarOnShoulder = 0, shoulderGap = 0, keepLeast = 1, broughtMost = 0;
    // c
    long long chestVerts = 0;
    float chestOff = 0, hemBelow = 9.0f, upperOff = 0;
    // The torso's and the strip's edges within an arm, the arms' own round the elbow (those with the game's own shoulders apart): as posed, and in the game's own frames.
    Stretched sleeveEdges, elbowEdges, elbowEdgesOwn, gameSleeveEdges, gameElbowEdges;
};

// The edges of a part between two vertices wholly on one arm's bones (never the chest's): each one's longest stretch, posed against the bind pose.
struct Edges {
    std::vector<std::pair<uint16_t, uint16_t>> list;
    std::vector<float> rest;
};
template <typename OwnerOf>
Edges ArmEdges(const Part& part, OwnerOf owner) {
    Edges e;
    for (size_t t = 0; t + 2 < part.index.size(); t += 3)
        for (int k = 0; k < 3; ++k) {
            const uint16_t a = part.index[t + k], b = part.index[t + (k + 1) % 3];
            const int oa = owner(part.verts[a]), ob = owner(part.verts[b]);
            const float rest = Dist(part.verts[a].p, part.verts[b].p);
            if (a < b && (oa == 0 || oa == 1) && oa == ob && rest > 1e-4f) {
                e.list.push_back({a, b});
                e.rest.push_back(rest);
            }
        }
    return e;
}
void Stretch(const Edges& e, const std::vector<float>& skinned, Stretched& worst) {
    for (size_t i = 0; i < e.list.size(); ++i) {
        const float now = Dist(&skinned[e.list[i].first * 3], &skinned[e.list[i].second * 3]);
        worst.ratio = std::max(worst.ratio, now / e.rest[i]);
        worst.grown = std::max(worst.grown, now - e.rest[i]);
    }
}
void SkinAll(const Part& part, const float* palette, std::vector<float>& out) {
    out.resize(part.verts.size() * 3);
    for (size_t v = 0; v < part.verts.size(); ++v)
        Skin(part.verts[v], palette, &out[v * 3]);
}

// The shoulders the arms are solved from: the game's own, then that much further back (metres): hands beyond the arm's reach, the shoulder brought along, the collar
// drawn in.
constexpr int kVariants = 3;
constexpr float kFurtherBack[kVariants] = {0.0f, 0.30f, 0.60f};

struct MeshWork {
    const Mesh* mesh = nullptr;
    Edges torso, strip, arms;
    Cover cover;
};

void Recording(const std::string& dir, const char* file, const MeshWork& work, Totals& all, int& read) {
    using namespace barehold;
    std::vector<uint8_t> bytes;
    std::vector<Record> records;
    if (!Load(dir + "\\" + file, bytes, records))
        return;
    ++read;
    Totals t;
    // Each frame's arms, torso and strip, in the order drawn (arms, torso, strip: once an eye).
    std::map<int, std::vector<const Record*>> frames;
    int state = -1;
    for (const Record& r : records) {
        const uint32_t count = static_cast<uint32_t>(r.head[0]), start = static_cast<uint32_t>(r.head[3]);
        const int arms = barestate::ArmsState(count, start, 0);
        const barestate::Part part = barestate::PartOf(count, start, 0);
        if (arms >= 0 && r.head[7] >= kBareBones) {
            state = arms;
            frames[r.head[1]].push_back(&r);
        } else if ((part == barestate::kTorso && r.head[7] >= kSleeveTorsoBones) || (part == barestate::kStripPart && r.head[7] >= kSleeveStripBones)) {
            frames[r.head[1]].push_back(&r);
        }
    }
    armik::State states[kVariants][4][2];  // the shoulders as the game's / further back; the frame's first, second ... arms; left, right
    std::vector<float> skinned, armsSkinned;
    int torsoStates[2] = {};
    for (const auto& [frame, list] : frames) {
        ++t.frames;
        int nth = 0;
        for (size_t i = 0; i < list.size(); ++i) {
            const Record& arms = *list[i];
            if (barestate::ArmsState(static_cast<uint32_t>(arms.head[0]), static_cast<uint32_t>(arms.head[3]), 0) < 0)
                continue;
            // Its torso and strip: the next ones drawn in the frame.
            const Record *torso = nullptr, *strip = nullptr;
            for (size_t j = i + 1; j < list.size() && barestate::ArmsState(static_cast<uint32_t>(list[j]->head[0]), static_cast<uint32_t>(list[j]->head[3]), 0) < 0; ++j) {
                const barestate::Part part = barestate::PartOf(static_cast<uint32_t>(list[j]->head[0]), static_cast<uint32_t>(list[j]->head[3]), 0);
                if (part == barestate::kTorso && !torso)
                    torso = list[j];
                if (part == barestate::kStripPart && !strip)
                    strip = list[j];
            }
            if (!torso || !strip)
                continue;
            ++t.pairs;
            const int torsoState = barestate::TorsoState(static_cast<uint32_t>(torso->head[0]), static_cast<uint32_t>(torso->head[3]), 0);
            if (torsoState >= 0)
                ++torsoStates[torsoState];
            const float *A = arms.bones, *T = torso->bones, *S = strip->bones;
            // a: the same skeleton.
            constexpr int kTorsoArms[6][2] = {{1, 47}, {0, 48}, {4, 50}, {9, 21}, {10, 22}, {12, 24}};
            constexpr int kStripTorso[6][2] = {{0, 0}, {1, 1}, {3, 3}, {4, 8}, {5, 9}, {6, 10}};
            bool same = torsoState == state, stripSame = true;
            for (const auto& p : kTorsoArms)
                same = same && SameBone(T + p[0] * 12, A + p[1] * 12);
            for (const auto& p : kStripTorso)
                stripSame = stripSame && SameBone(S + p[0] * 12, T + p[1] * 12);
            t.notSame += !same, t.stripNotSame += !stripSame;
            float gameShoulder[2][3];
            for (int side = 0; side < 2; ++side) {
                float sb[3], eb[3], wb[3], a[3], b[3], c[3], d[3];
                Joint(armik::kShoulder, side, sb), Joint(armik::kElbow, side, eb), Joint(armik::kWrist, side, wb);
                const int collar = side == 0 ? 3 : 8, upper = side == 0 ? 1 : 9, upper2 = side == 0 ? 0 : 10, hem = side == 0 ? 4 : 12, end = side == 0 ? 50 : 24;
                Point(T + collar * 12, sb, a), Point(T + upper * 12, sb, b);
                t.gameShoulder = std::max(t.gameShoulder, Dist(a, b));
                std::memcpy(gameShoulder[side], b, sizeof(b));
                Point(T + upper * 12, eb, c);
                t.gameUpper = std::max(t.gameUpper, std::fabs(Dist(b, c) - armik::kUpper));
                Point(T + upper2 * 12, eb, a), t.gameElbow = std::max(t.gameElbow, Dist(a, c));
                Point(T + hem * 12, eb, a), t.gameElbow = std::max(t.gameElbow, Dist(a, c));
                Point(A + end * 12, eb, a), t.gameElbow = std::max(t.gameElbow, Dist(a, c));
                Point(A + kBarePalm[side] * 12, wb, d);
                t.gameFore = std::max(t.gameFore, std::fabs(Dist(a, d) - armik::kFore));
            }
            // b: posed. The body's axes the game's own chest bone's (the bind pose's x its left, y up, z its front).
            const float* chestBone = T + 2 * 12;
            float left[3] = {chestBone[0], chestBone[4], chestBone[8]}, up[3] = {chestBone[1], chestBone[5], chestBone[9]}, back[3] = {-chestBone[2], -chestBone[6], -chestBone[10]};
            armik::detail::Normalize(left), armik::detail::Normalize(up), armik::detail::Normalize(back);
            for (int variant = 0; variant < kVariants; ++variant) {
                ArmPose pose[2];
                float shoulders[2][3], chest[3];
                bool on[2] = {true, true};
                for (int side = 0; side < 2; ++side) {
                    float shoulder[3], wb[3], eb[3], sb[3];
                    Joint(armik::kWrist, side, wb), Joint(armik::kElbow, side, eb), Joint(armik::kShoulder, side, sb);
                    for (int k = 0; k < 3; ++k)
                        shoulder[k] = gameShoulder[side][k] + back[k] * kFurtherBack[variant];
                    const float* hand = A + kBarePalm[side] * 12;  // the game's palm bone: the hand as drawn
                    ArmPose& p = pose[side];
                    SolveArm(hand, shoulder, left, up, back, side, states[variant][std::min(nth, 3)][side], p);
                    std::memcpy(shoulders[side], p.r.shoulder, sizeof(shoulders[side]));
                    ++t.poses;
                    t.notFinite += !p.finite;
                    t.beyond += !p.r.reach;
                    t.held += p.r.reachHeld;
                    t.broughtFar += p.brought > armik::kCollarDrawsIn;
                    t.broughtMost = std::max(t.broughtMost, p.brought);
                    t.keepLeast = std::min(t.keepLeast, p.keep);
                    t.capped += p.cap > 0.0f;
                    // The joints through the frames.
                    float e0[3], eU[3], sU[3], sC[3], w2[3], wHand[3];
                    Point(p.frames[0], eb, e0), Point(p.frames[kFrameUpper], eb, eU), Point(p.frames[kFrameUpper], sb, sU), Point(p.frames[kFrameCollar], sb, sC);
                    Point(p.frames[2], wb, w2), Point(hand, wb, wHand);
                    t.elbow = std::max(t.elbow, Dist(e0, eU));
                    t.upper = std::max(t.upper, std::fabs(Dist(sU, eU) - armik::kUpper));
                    t.fore = std::max(t.fore, std::fabs(Dist(e0, w2) - armik::kFore));
                    t.wrist = std::max(t.wrist, Dist(w2, wHand));
                    for (int f = 0; f <= kFrameUpper; ++f)
                        t.rigid = std::max(t.rigid, OffRigid(p.frames[f]));
                    t.collarRigid = std::max(t.collarRigid, OffRigid(p.frames[kFrameCollar], p.keep));
                    t.collarOnShoulder = std::max(t.collarOnShoulder, Dist(sC, p.r.shoulder));
                    // The collar's shoulder joint against the upper arm's own: one place wherever the shoulder was brought along; an elbow end nearer the shoulder than
                    // the bone is long (the wrist past its range, within the reach limit) leaves the bone's end that much past it.
                    if (p.brought > 0.0f)
                        t.collarOnShoulder = std::max(t.collarOnShoulder, Dist(sC, sU));
                    else
                        t.shoulderGap = std::max(t.shoulderGap, Dist(sC, sU));
                }
                for (int k = 0; k < 3; ++k)
                    chest[k] = (shoulders[0][k] + shoulders[1][k]) / 2;
                // The palettes as drawn.
                float armsPalette[kBareBones * 12], torsoPalette[kSleeveTorsoBones * 12], stripPalette[kSleeveStripBones * 12];
                std::memcpy(armsPalette, A, sizeof(armsPalette));
                std::memcpy(torsoPalette, T, sizeof(torsoPalette));
                std::memcpy(stripPalette, S, sizeof(stripPalette));
                const float (*const frames2[2])[12] = {pose[0].frames, pose[1].frames};
                for (int side = 0; side < 2; ++side)
                    PlaceForearmBones(side, pose[side].frames, pose[side].cap, true, armsPalette);
                PlaceSleeves(kSleeveTorsoRoles, kSleeveTorso, kSleeveTorsoBones, on, frames2, chest, torsoPalette);
                PlaceSleeves(kSleeveStripRoles, kSleeveStrip, kSleeveStripBones, on, frames2, chest, stripPalette);
                // The elbow through the palettes: the sleeve's upper arm bones and hem, the arms' upper arm bones and elbow end, one joint.
                for (int side = 0; side < 2; ++side) {
                    float eb[3], at[3], other[3];
                    Joint(armik::kElbow, side, eb);
                    Point(armsPalette + (side == 0 ? 50 : 24) * 12, eb, at);
                    const int torsoBones[3] = {side == 0 ? 1 : 9, side == 0 ? 0 : 10, side == 0 ? 4 : 12}, armsBones[3] = {side == 0 ? 47 : 21, side == 0 ? 48 : 22, side == 0 ? 51 : 25};
                    for (int b : torsoBones)
                        Point(torsoPalette + b * 12, eb, other), t.elbow = std::max(t.elbow, Dist(at, other));
                    for (int b : armsBones)
                        Point(armsPalette + b * 12, eb, other), t.elbow = std::max(t.elbow, Dist(at, other));
                }
                // c: the mesh under those palettes.
                if (work.mesh) {
                    const Mesh& mesh = *work.mesh;
                    SkinAll(mesh.torso, torsoPalette, skinned);
                    for (size_t v = 0; v < mesh.torso.verts.size(); ++v)
                        if (Owner(mesh.torso.verts[v], kSleeveTorsoRoles) == 2)
                            ++t.chestVerts, t.chestOff = std::max(t.chestOff, Dist(&skinned[v * 3], chest));
                    Stretch(work.torso, skinned, t.sleeveEdges);
                    SkinAll(mesh.arms, armsPalette, armsSkinned);
                    Stretch(work.arms, armsSkinned, t.elbowEdges);
                    if (variant == 0)
                        Stretch(work.arms, armsSkinned, t.elbowEdgesOwn);
                    // The arms' open end inside the sleeve: along the upper arm from the shoulder joint, all the way round, the sleeve's rim against the arms' end.
                    for (int side = 0; side < 2; ++side) {
                        float sb[3], eb[3], s[3], e[3];
                        Joint(armik::kShoulder, side, sb), Joint(armik::kElbow, side, eb);
                        Point(pose[side].frames[kFrameUpper], sb, s), Point(pose[side].frames[kFrameUpper], eb, e);
                        float axis[3] = {e[0] - s[0], e[1] - s[1], e[2] - s[2]};
                        armik::detail::Normalize(axis);
                        for (int k = 0; k < kRound; ++k)
                            if (work.cover.rim[side][k] >= 0)
                                t.hemBelow = std::min(t.hemBelow, Along(&skinned[work.cover.rim[side][k] * 3], s, axis) - Along(&armsSkinned[work.cover.end[side][k] * 3], s, axis));
                        // A vertex wholly on the upper arm's two bones, in the sleeve or the arms: where the solved upper arm puts it.
                        for (size_t v = 0; v < mesh.arms.verts.size(); ++v) {
                            const Vert& vert = mesh.arms.verts[v];
                            bool upperOnly = true;
                            for (int k = 0; k < 4; ++k)
                                upperOnly = upperOnly && (!vert.weight[k] || vert.bone[k] == (side == 0 ? 47 : 21) || vert.bone[k] == (side == 0 ? 48 : 22));
                            if (upperOnly) {
                                float want[3];
                                Point(pose[side].frames[kFrameUpper], vert.p, want);
                                t.upperOff = std::max(t.upperOff, Dist(want, &armsSkinned[v * 3]));
                            }
                        }
                    }
                    SkinAll(mesh.strip, stripPalette, skinned);
                    for (size_t v = 0; v < mesh.strip.verts.size(); ++v)
                        if (Owner(mesh.strip.verts[v], kSleeveStripRoles) == 2)
                            ++t.chestVerts, t.chestOff = std::max(t.chestOff, Dist(&skinned[v * 3], chest));
                    Stretch(work.strip, skinned, t.sleeveEdges);
                    // The game's own frame, for what its animation stretches the same edges by.
                    if (variant == 0) {
                        SkinAll(mesh.torso, T, skinned);
                        Stretch(work.torso, skinned, t.gameSleeveEdges);
                        SkinAll(mesh.arms, A, armsSkinned);
                        Stretch(work.arms, armsSkinned, t.gameElbowEdges);
                        SkinAll(mesh.strip, S, skinned);
                        Stretch(work.strip, skinned, t.gameSleeveEdges);
                    }
                }
            }
            ++nth;
        }
    }
    const char* cuffs = state == 1 ? "the cuffs off" : "handcuffed";
    char id[16];
    std::snprintf(id, sizeof(id), "s6a");
    Check(t.pairs > 0 && t.notSame == 0 && t.stripNotSame == 0 && torsoStates[1 - std::max(state, 0)] == 0 && t.gameShoulder <= 2e-4f && t.gameElbow <= 2e-4f && t.gameUpper <= 5e-4f && t.gameFore <= 5e-4f, id,
          "%s (%s: the torso %d from %d): %d frames, %d draws of the arms with their torso and strip; the torso's arm bones not the arms' own bit for bit %d times, the strip's not the "
          "torso's %d; the game's own bones: collar and upper arm %.3f mm apart at the shoulder at most, upper arm, hem and elbow end %.3f mm at the elbow (0.2), the upper arm %.3f mm and "
          "the forearm %.3f mm off armik's lengths (0.5)",
          file, cuffs, 5604, state == 1 ? 7545 : 0, t.frames, t.pairs, t.notSame, t.stripNotSame, t.gameShoulder * 1000, t.gameElbow * 1000, t.gameUpper * 1000, t.gameFore * 1000);
    std::snprintf(id, sizeof(id), "s6b");
    Check(t.poses > 0 && t.notFinite == 0 && t.elbow <= 1e-4f && t.upper <= 1e-4f && t.fore <= 1e-4f && t.wrist <= 1e-4f && t.rigid <= 1e-4f && t.collarRigid <= 1e-4f &&
              t.collarOnShoulder <= 1e-4f && t.shoulderGap <= 0.0505f && t.keepLeast >= armik::kCollarKeep && t.beyond > 0 && t.beyond < t.poses,
          id,
          "%s posed (%s): %d arms solved from the game's own hands (%d beyond the arm's reach, %d with the shoulder brought along over 12 cm, %.2f m at most, the collar drawn in to "
          "%.0f%% at the least; %d with the elbow kept by the reach limit): the sleeve's, the hem's and the arms' elbow joints %.4f mm apart at most (0.1); the upper arm %.4f mm and the "
          "forearm %.4f mm off their lengths (0.1), the wrist %.4f mm off the hand; the arm's bones %.1e off rigid, the collar %.1e off the chest's axes scaled; the collar's shoulder "
          "joint %.4f mm from the upper arm's far end (0.1), and where the elbow sits nearer than the bone is long the bone's end %.1f mm past it at most (the reach limit's 50); %d not "
          "finite",
          file, cuffs, t.poses, t.beyond, t.broughtFar, t.broughtMost, t.keepLeast * 100, t.held, t.elbow * 1000, t.upper * 1000, t.fore * 1000, t.wrist * 1000, t.rigid, t.collarRigid,
          t.collarOnShoulder * 1000, t.shoulderGap * 1000, t.notFinite);
    if (work.mesh) {
        std::snprintf(id, sizeof(id), "s6c");
        Check(t.chestVerts > 0 && t.chestOff <= 1e-5f && t.sleeveEdges.ratio <= 1.25f && t.sleeveEdges.grown <= std::max(0.005f, t.gameSleeveEdges.grown) &&
                  t.elbowEdges.grown <= 2.0f * t.gameElbowEdges.grown && t.elbowEdgesOwn.grown <= 1.25f * t.gameElbowEdges.grown && t.hemBelow >= 0.03f && t.upperOff <= 1e-4f,
              id,
              "%s on the mesh: %lld chest vertices %.4f mm from the chest's point at most; an edge of the cloth within one arm's sleeve %.2f times its length and %.1f mm longer at "
              "most (1.25 and 5 mm allowed; the game's own frames: %.2f, %.1f mm); the arms' skin round the elbow, under the sleeve, bends as the game's own elbow does: an edge "
              "%.1f mm longer at most with the game's shoulders (the game's own frames: %.1f mm; 1.25 times that allowed), %.1f mm with the hands beyond reach (twice allowed); the "
              "arms' open end %.1f cm up inside the sleeve's rim at the least, all the way round (3 wanted; %d of the arms would have been capped without the sleeve); the upper arm's vertices %.4f mm "
              "from where the solved upper arm puts them",
              file, t.chestVerts, t.chestOff * 1000, t.sleeveEdges.ratio, t.sleeveEdges.grown * 1000, t.gameSleeveEdges.ratio, t.gameSleeveEdges.grown * 1000, t.elbowEdgesOwn.grown * 1000,
              t.gameElbowEdges.grown * 1000, t.elbowEdges.grown * 1000, t.hemBelow * 100, t.capped, t.upperOff * 1000);
    }
    all.poses += t.poses, all.pairs += t.pairs, all.frames += t.frames;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "build\\survey", meshDir = argc > 2 ? argv[2] : "build";
    std::printf("baresleevetest: Blackburn's arms whole up to the shoulders (src/barestate.h, barehold.h, armik.h). Recordings: %s; the mesh dump: %s\n", dir.c_str(), meshDir.c_str());
    Draws();
    Tables();
    Gloved();
    OnlySolved();
    Mesh mesh;
    MeshWork work;
    if (LoadMesh(meshDir, mesh)) {
        work.cover = MakeCover(mesh);
        MeshTables(mesh, work.cover);
        work.mesh = &mesh;
        work.torso = ArmEdges(mesh.torso, [](const Vert& v) { return Owner(v, barehold::kSleeveTorsoRoles); });
        work.strip = ArmEdges(mesh.strip, [](const Vert& v) { return Owner(v, barehold::kSleeveStripRoles); });
        work.arms = ArmEdges(mesh.arms, [](const Vert& v) {
            // (Only the elbow's: an edge with an end on the upper arm's bones or the elbow end's helper. The hands' edges are the game's own fingers.)
            for (int k = 0; k < 4; ++k)
                if (v.weight[k] && (v.bone[k] == 47 || v.bone[k] == 48 || v.bone[k] == 51 || v.bone[k] == 21 || v.bone[k] == 22 || v.bone[k] == 25))
                    return ArmsOwner(v);
            return -1;
        });
    } else {
        Skip("s5", "no mesh dump (mesh_18108_16053_vb0.bin, _ib.bin): the tables against the mesh, and s6c");
    }
    Totals all;
    int read = 0;
    for (const char* file : {"AKS74u_empty.bin", "AKS74u_partial.bin", "UMP45_empty.bin", "UMP45_partial.bin", "M1014_empty.bin", "M1014_partial.bin", "aks_test_idle.bin"})
        Recording(dir, file, work, all, read);
    if (!read)
        Skip("s6", "no recordings (AKS74u_empty.bin ... aks_test_idle.bin): the game's own frames");
    else
        std::printf("     %d recordings: %d frames, %d draws of the arms with their torso and strip, %d arms posed\n", read, all.frames, all.pairs, all.poses);
    std::printf("%d of %d checks failed%s\n", g_failed, g_checks, g_skipped ? " (some skipped: no recordings or no mesh dump)" : "");
    return g_failed;
}

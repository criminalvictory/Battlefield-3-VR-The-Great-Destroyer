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
//          then again with the shoulders 30 cm and 60 cm further back (hands beyond the arm's reach, the shoulder brought along; past 12 cm of that the sleeve
//          off, barehold::SleeveStays, and that arm capped whole); the torso's, the strip's and the arms' palettes made as the mod draws
//          them. The sleeve joins the forearm at the elbow: the upper arm's, the hem's, the arms' 47 / 21 and 50 / 24 elbow joints in one place (within 0.1 mm).
//          No bone stretched: the upper arm its 0.3004 m and the forearm its 0.2608 m (within 0.1 mm), every arm bone rigid, the collar the chest's axes whole
//          (never drawn in), its shoulder joint on the upper arm's far end wherever the shoulder was brought along, and never further from the upper arm's own
//          than the reach limit (5 cm) where it wasn't; the wrist on the hand. Both cuff states.
//       c  with the mesh: skinned with those palettes, every vertex of the torso and strip wholly on the chest's bones is on the chest's point; no edge of the
//          cloth between two vertices wholly on one arm's bones is longer than 1.25 times its bind length, nor grown more than the game's own frames grow one (5
//          mm); the arms' own skin round the elbow (under the sleeve: its upper arm's bones now on the upper arm, where they were rigid on the forearm's end) bends
//          as the game's elbow does: its edges grown no more than 1.25 times what the game's own frames grow them with the game's shoulders, twice with the hands
//          beyond reach (the forearm's elbow end takes half the hand's roll, weapon.txt "bareshare": that turns it on the upper arm, as the game's own does less);
//          the arms' open end stays up inside the sleeve (the rim 3 cm and more below it along the upper arm, all the way round), in reach and beyond: no cap
//          is needed with the sleeve on
//   s7  the cloth tied to the chest (the film, 2026-11-18: the gun held 20 cm and more forward, an arm raised high, the sleeves off the body, the shoulder's
//       cloth a pointed tail and shards: 685 of the torso's and strip's vertices lean on the chest's bones and an arm's, and s6c's edges, wholly one arm's, have
//       none of them; SKIP without the bare body's mesh, the gloved chest's (build\mesh_8529_0_vb0.bin / _ib.bin) or the recordings). Every edge with an end
//       leaning on the chest's bones and an end leaning on an arm's, over the recordings' draws (every fourth) in the game's hold, the hold as filmed, the film's
//       reaches (20, 35, 50 cm forward) and raises (30, 50, 70 cm), and hands anywhere:
//         - what bringing a shoulder along changes in an edge's length (against the same arms at the body's own shoulders) is within the bar: the same of the
//           gloved levels' chest and sleeves (8529) on their own rule, wherever that rule shows its collars whole (the shoulders 12 cm along at most)
//         - an edge is no longer than bound by more than the gloved chest's are while its hands hold on (the fold itself does this, under a raised arm)
//         - a sleeve is on only with its shoulder brought along 12 cm at most, its collar whole; the chest's point no further than that from the body's; an arm
//           without its sleeve is capped whole; the rule's own numbers (back on within 10 cm, no sleeve on a number not finite)
//         - the two holds keep both sleeves, and every pose with both on is the torso as before the limit, to the bit (the hold as filmed: good)
//         - as the sleeves were first built (no limit, the collar drawn in from 12 cm as the gloved levels') each of the film's six kinds of pose is over the bar:
//           the check fails on that code
//       "sweep" prints the table the limit was found from: each limit tried, each kind of pose.
// Usage: baresleevetest [<folder with the recordings> [<folder with the mesh dump> [sweep]]] (build\survey and build by default, from the project's folder; sweep:
//        s7's table of the cloth's growth for each limit tried: how the limit was found). Reads only.

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
    Part gloved;  // the levels with gloves' chest and sleeves (8529 from 0, mesh_8529_0: s7's bar), when its dump is there
    bool haveGloved = false;
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
    if (!(TakePart(vb, ib, 0, 5604, 0, barehold::kSleeveTorsoBones, mesh.torso) && TakePart(vb, ib, 13149, 222, 143616, barehold::kSleeveStripBones, mesh.strip) &&
          TakePart(vb, ib, 16053, 18108, 180096, barehold::kBareBones, mesh.arms)))
        return false;
    std::vector<uint8_t> gvb, gib;
    mesh.haveGloved = ReadFile(dir + "\\mesh_8529_0_vb0.bin", gvb) && ReadFile(dir + "\\mesh_8529_0_ib.bin", gib) && TakePart(gvb, gib, 0, 8529, 0, 14, mesh.gloved);
    return true;
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
    bool on = true;  // its sleeve on it (barehold::SleeveStays)
};
// before: as the sleeves were first built (the shoulder brought along without limit, the collar drawn in from 12 cm as the gloved levels' is, a sleeve always on).
// most: the limit tried (s7's sweep); the mod's own by default.
void SolveArm(const float* hand, const float* shoulder, const float* chestLeft, const float* up, const float* chestBack, int side, armik::State& state, ArmPose& pose,
              bool before = false, float most = barehold::kSleeveShoulderMost) {
    float out[3];
    for (int i = 0; i < 3; ++i)
        out[i] = side == 0 ? chestLeft[i] : -chestLeft[i];
    armik::Solve(hand, shoulder, out, up, chestBack, side == 1, BareParams(side == 1), state, pose.r);
    std::memcpy(pose.frames, pose.r.frame, sizeof(pose.r.frame));
    std::memcpy(pose.frames[barehold::kFrameUpper], pose.r.upper, sizeof(pose.r.upper));
    pose.brought = Dist(pose.r.shoulder, shoulder);
    pose.keep = before ? armik::CollarKeep(pose.brought) : 1.0f;
    pose.on = before || barehold::SleeveStays(pose.brought, true, most);
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
    int poses = 0, beyond = 0, broughtFar = 0, held = 0, notFinite = 0, capped = 0, off = 0, offNotCapped = 0;
    float elbow = 0, upper = 0, fore = 0, wrist = 0, rigid = 0, collarRigid = 0, collarOnShoulder = 0, shoulderGap = 0, keepLeast = 1, broughtMost = 0, broughtOn = 0;
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
    std::vector<int8_t> side;  // ArmEdges': the arm it is on (0 left, 1 right)
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
                e.side.push_back(static_cast<int8_t>(oa));
            }
        }
    return e;
}
// (on: only the edges of the arms that are; ArmEdges' lists.)
void Stretch(const Edges& e, const std::vector<float>& skinned, Stretched& worst, const bool* on = nullptr) {
    for (size_t i = 0; i < e.list.size(); ++i) {
        if (on && !on[e.side[i]])
            continue;
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

// The shoulders the arms are solved from: the game's own, then that much further back (metres): hands beyond the arm's reach, the shoulder brought along, and past
// what the cloth takes (barehold::SleeveStays) the sleeve folded away, the arm as it was without one.
constexpr int kVariants = 3;
constexpr float kFurtherBack[kVariants] = {0.0f, 0.30f, 0.60f};

// The cloth tied to the chest: every edge of a part with an end leaning on the chest's bones (wholly or in part) and an end leaning on an arm's (the collar's, the
// upper arm's). The chest's bones fold onto the point between the shoulders, so these are the edges that stretch when an arm's bones leave it: 602 of the torso's
// vertices are such blends, and ArmEdges (wholly one arm's) never looked at one of them.
Edges FoldEdges(const Part& part, const char* roles) {
    auto chestShare = [&](const Vert& v) {
        int share = 0;
        for (int i = 0; i < 4; ++i)
            if (v.weight[i] && roles[v.bone[i]] != 'l' && roles[v.bone[i]] != 'r')
                share += v.weight[i];
        return share;
    };
    std::map<std::pair<uint16_t, uint16_t>, float> found;
    for (size_t t = 0; t + 2 < part.index.size(); t += 3)
        for (int k = 0; k < 3; ++k) {
            const uint16_t a = std::min(part.index[t + k], part.index[t + (k + 1) % 3]), b = std::max(part.index[t + k], part.index[t + (k + 1) % 3]);
            const int ca = chestShare(part.verts[a]), cb = chestShare(part.verts[b]);
            const float rest = Dist(part.verts[a].p, part.verts[b].p);
            if (a != b && (ca > 0 || cb > 0) && (ca < 255 || cb < 255) && rest > 1e-4f)
                found[{a, b}] = rest;
        }
    Edges e;
    for (const auto& [ends, rest] : found)
        e.list.push_back(ends), e.rest.push_back(rest);
    return e;
}
int Blended(const Part& part, const char* roles) {  // its vertices leaning on the chest's bones and an arm's
    int n = 0;
    for (const Vert& v : part.verts) {
        bool chest = false, arm = false;
        for (int i = 0; i < 4; ++i)
            if (v.weight[i])
                (roles[v.bone[i]] == 'l' || roles[v.bone[i]] == 'r' ? arm : chest) = true;
        n += chest && arm;
    }
    return n;
}

// The hands, shoulders and chest's axes of one recorded draw of the arms (the game's own), for s7's poses.
struct Base {
    float hand[2][12], shoulder[2][3], left[3], up[3], back[3];
};

struct MeshWork {
    const Mesh* mesh = nullptr;
    Edges torso, strip, arms;
    Edges foldTorso, foldStrip, foldGloved;
    Cover cover;
    std::vector<Base>* bases = nullptr;  // every kBaseEvery-th draw of the recordings, gathered for s7
};
constexpr int kBaseEvery = 4;

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
            if (work.bases && t.pairs % kBaseEvery == 1) {
                Base base;
                for (int side = 0; side < 2; ++side) {
                    std::memcpy(base.hand[side], A + kBarePalm[side] * 12, sizeof(base.hand[side]));
                    std::memcpy(base.shoulder[side], gameShoulder[side], sizeof(base.shoulder[side]));
                }
                std::memcpy(base.left, left, sizeof(left)), std::memcpy(base.up, up, sizeof(up)), std::memcpy(base.back, back, sizeof(back));
                work.bases->push_back(base);
            }
            for (int variant = 0; variant < kVariants; ++variant) {
                ArmPose pose[2];
                float shoulders[2][3], chest[3];
                bool on[2] = {};
                for (int side = 0; side < 2; ++side) {
                    float shoulder[3], wb[3], eb[3], sb[3];
                    Joint(armik::kWrist, side, wb), Joint(armik::kElbow, side, eb), Joint(armik::kShoulder, side, sb);
                    for (int k = 0; k < 3; ++k)
                        shoulder[k] = gameShoulder[side][k] + back[k] * kFurtherBack[variant];
                    const float* hand = A + kBarePalm[side] * 12;  // the game's palm bone: the hand as drawn
                    ArmPose& p = pose[side];
                    SolveArm(hand, shoulder, left, up, back, side, states[variant][std::min(nth, 3)][side], p);
                    on[side] = p.on && p.finite;  // (its sleeve: the shoulder brought along no further than the cloth takes)
                    std::memcpy(shoulders[side], on[side] ? p.r.shoulder : shoulder, sizeof(shoulders[side]));  // (without one: the shoulder as given, hands.cpp)
                    t.off += !on[side];
                    t.offNotCapped += !on[side] && p.cap < 1.0f;
                    if (on[side])
                        t.broughtOn = std::max(t.broughtOn, p.brought);
                    ++t.poses;
                    t.notFinite += !p.finite;
                    t.beyond += !p.r.reach;
                    t.held += p.r.reachHeld;
                    t.broughtFar += p.brought > kSleeveShoulderMost;
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
                    PlaceForearmBones(side, pose[side].frames, pose[side].cap, on[side], armsPalette);
                PlaceSleeves(kSleeveTorsoRoles, kSleeveTorso, kSleeveTorsoBones, on, frames2, chest, torsoPalette);
                PlaceSleeves(kSleeveStripRoles, kSleeveStrip, kSleeveStripBones, on, frames2, chest, stripPalette);
                // The elbow through the palettes: the sleeve's upper arm bones and hem, the arms' upper arm bones and elbow end, one joint.
                for (int side = 0; side < 2; ++side) {
                    float eb[3], at[3], other[3];
                    Joint(armik::kElbow, side, eb);
                    Point(armsPalette + (side == 0 ? 50 : 24) * 12, eb, at);
                    const int torsoBones[3] = {side == 0 ? 1 : 9, side == 0 ? 0 : 10, side == 0 ? 4 : 12}, armsBones[3] = {side == 0 ? 47 : 21, side == 0 ? 48 : 22, side == 0 ? 51 : 25};
                    for (int b : torsoBones)
                        if (on[side])  // (a sleeve folded away is on the chest's point)
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
                    Stretch(work.arms, armsSkinned, t.elbowEdges, on);  // (an arm without its sleeve has its end's rim drawn in, as it was before the sleeves)
                    if (variant == 0)
                        Stretch(work.arms, armsSkinned, t.elbowEdgesOwn, on);
                    // The arms' open end inside the sleeve: along the upper arm from the shoulder joint, all the way round, the sleeve's rim against the arms' end.
                    for (int side = 0; side < 2; ++side) {
                        if (!on[side])
                            continue;
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
              t.collarOnShoulder <= 1e-4f && t.shoulderGap <= 0.0505f && t.keepLeast == 1.0f && t.beyond > 0 && t.beyond < t.poses && t.off == t.broughtFar && t.off > 0 &&
              t.off < t.poses && t.offNotCapped == 0 && t.broughtOn <= kSleeveShoulderMost,
          id,
          "%s posed (%s): %d arms solved from the game's own hands (%d beyond the arm's reach; %d with the shoulder brought along over %.0f cm, %.2f m at most: those %d without "
          "their sleeve, %d of them not capped whole (0 wanted); a sleeve on a shoulder %.3f m along at most, its collar whole, %.0f%% at the least; %d with the elbow kept by the "
          "reach limit): the sleeve's, the hem's and the arms' elbow joints %.4f mm apart at most (0.1); the upper arm %.4f mm and the forearm %.4f mm off their lengths (0.1), the "
          "wrist %.4f mm off the hand; the arm's bones %.1e off rigid, the collar %.1e off the chest's axes; the collar's shoulder joint %.4f mm from the upper arm's far end "
          "(0.1), and where the elbow sits nearer than the bone is long the bone's end %.1f mm past it at most (the reach limit's 50); %d not finite",
          file, cuffs, t.poses, t.beyond, t.broughtFar, kSleeveShoulderMost * 100, t.broughtMost, t.off, t.offNotCapped, t.broughtOn, t.keepLeast * 100, t.held, t.elbow * 1000,
          t.upper * 1000, t.fore * 1000, t.wrist * 1000, t.rigid, t.collarRigid, t.collarOnShoulder * 1000, t.shoulderGap * 1000, t.notFinite);
    if (work.mesh) {
        std::snprintf(id, sizeof(id), "s6c");
        Check(t.chestVerts > 0 && t.chestOff <= 1e-5f && t.sleeveEdges.ratio <= 1.25f && t.sleeveEdges.grown <= std::max(0.005f, t.gameSleeveEdges.grown) &&
                  t.elbowEdges.grown <= 2.0f * t.gameElbowEdges.grown && t.elbowEdgesOwn.grown <= 1.25f * t.gameElbowEdges.grown && t.hemBelow >= 0.03f && t.upperOff <= 1e-4f,
              id,
              "%s on the mesh: %lld chest vertices %.4f mm from the chest's point at most; an edge of the cloth within one arm's sleeve %.2f times its length and %.1f mm longer at "
              "most (1.25 and 5 mm allowed; the game's own frames: %.2f, %.1f mm); the arms' skin round the elbow, under the sleeve, bends as the game's own elbow does: an edge "
              "%.1f mm longer at most with the game's shoulders (the game's own frames: %.1f mm; 1.25 times that allowed), %.1f mm with the hands beyond reach (twice allowed); the "
              "arms' open end %.1f cm up inside the sleeve's rim at the least, all the way round, wherever the sleeve is on (3 wanted; %d of the arms would have been capped without "
              "one); the upper arm's vertices %.4f mm from where the solved upper arm puts them",
              file, t.chestVerts, t.chestOff * 1000, t.sleeveEdges.ratio, t.sleeveEdges.grown * 1000, t.gameSleeveEdges.ratio, t.gameSleeveEdges.grown * 1000, t.elbowEdgesOwn.grown * 1000,
              t.gameElbowEdges.grown * 1000, t.elbowEdges.grown * 1000, t.hemBelow * 100, t.capped, t.upperOff * 1000);
    }
    all.poses += t.poses, all.pairs += t.pairs, all.frames += t.frames;
}

// ---- s7: the cloth tied to the chest ----
// The poses: the recordings' own hold (the game's: both hands well in reach); the hold as filmed in the headset's place (2026-11-18, the stand-in's: the left hand on
// the handguard 8 cm beyond its arm's reach, the right in reach: here the same hands with the shoulders as far further back as brings the left shoulder 8 cm along);
// the film's poses from that hold (the gun held 20, 35 and 50 cm further forward: the shoulders that far further back, the left 0.26, 0.40 and 0.54 m along as built,
// the film's 0.25, 0.39, 0.55; the arms raised 30, 50 and 70 cm: the shoulders that far down and 25 cm back, as the film's raises had the gun out ahead, the left
// shoulder 0.26 to 0.37 m along as built, the film's 0.30 to 0.49);
// and hands anywhere (each wrist 0.15 to 0.95 m from its shoulder, any way but behind the back, the hand turned up to 120 degrees any way).
struct Family {
    const char* name;
    bool filmed;  // from the hold as filmed
    float back, down;
};
constexpr Family kFamilies[] = {{"the game's hold", false, 0.0f, 0.0f}, {"hold as filmed", true, 0.0f, 0.0f}, {"20 cm forward", true, 0.20f, 0.0f},
                                {"35 cm forward", true, 0.35f, 0.0f},   {"50 cm forward", true, 0.50f, 0.0f}, {"raised 30 cm", true, 0.25f, 0.30f},
                                {"raised 50 cm", true, 0.25f, 0.50f},   {"raised 70 cm", true, 0.25f, 0.70f},  {"hands anywhere", false, 0.0f, 0.0f}};
constexpr int kFamilyCount = static_cast<int>(std::size(kFamilies)), kRandomFamily = kFamilyCount - 1, kRandomEach = 6;
constexpr int kHolds = 2, kFilmFrom = 2;  // the first two are holds; the film's reaches and raises from the third on
constexpr float kFilmedHoldBrought = 0.08f;
// The gloved levels' chest and sleeves (skin.cpp's kArmDraws: 8529 from 0, "cllllll-rrrrrr", kSleevesForearm): the same frames, the chest folded as here.
inline constexpr char kGlovedRoles[] = "cllllll-rrrrrr";
inline constexpr int8_t kGlovedMap[] = {-1, 3, 3, 4, 0, 1, 0, -1, 4, 3, 0, 1, 3, 0};

// What a pose's cloth is measured by.
//   grown:  how much longer than in the bind pose an edge is. The fold itself does most of this, on either mesh, with every hand in reach (an edge from a vertex
//           half the chest's to one mostly the arm's spans half the way to the chest's point: 9 to 14 cm, the most under a raised arm, in the armpit).
//   change: how much an edge's length differs from the same arms' at the body's own shoulders (the arms' frames moved back by what the shoulder was brought
//           along, the collar whole, the chest's point between the body's shoulders): what bringing the shoulder along, and drawing the collar in, does to the
//           cloth, longer or shorter. This is what the limit holds: none with nothing brought along.
struct ClothStats {
    Stretched cloth;        // grown: the bare torso's and strip's edges tied to the chest
    float change = 0;       // change: the same edges
    Stretched gloved;       // grown: the gloved chest's, on the gloved levels' own rule (the shoulder without limit, the collar drawn in from 12 cm), in every pose
    float glovedChange = 0; // change: the gloved chest's, in the poses where that rule shows its collars whole (both shoulders 12 cm along at most): s7's bar
    float glovedReach = 0;  // grown: the gloved chest's, with both shoulders no further along than its own hold lets go at (0.24 m: 0.80 m from the shoulder)
    int whole = 0;          // the poses the bar is of
    int arms = 0, off = 0;  // arms posed; of them without their sleeve
    int drawnIn = 0;        // sleeves on with the collar drawn in at all
    int partCap = 0;        // arms without their sleeve whose open end isn't capped whole
    float broughtOn = 0;    // the furthest a shoulder was brought along with its sleeve on
    float brought = 0;      // ... at all
    float chestOut = 0;     // the furthest the chest's point was from the point between the shoulders given (the body's)
};
struct Rng {
    uint32_t state = 0x1234567u;
    float Next() {  // 0 to 1
        state = state * 1664525u + 1013904223u;
        return static_cast<float>(state >> 8) / 16777216.0f;
    }
};
void Changed(const Edges& e, const std::vector<float>& posed, const std::vector<float>& atBody, float& worst) {
    for (size_t i = 0; i < e.list.size(); ++i)
        worst = std::max(worst, std::fabs(Dist(&posed[e.list[i].first * 3], &posed[e.list[i].second * 3]) - Dist(&atBody[e.list[i].first * 3], &atBody[e.list[i].second * 3])));
}
// Two arms' frames at the body's own shoulders: each moved back by what its shoulder was brought along, its collar whole on the shoulder given.
void AtBody(const Base& base, const ArmPose pose[2], const float shoulders[2][3], float frames[2][barehold::kArmFrames][12]) {
    for (int side = 0; side < 2; ++side) {
        std::memcpy(frames[side], pose[side].frames, sizeof(pose[side].frames));
        for (int f = 0; f <= barehold::kFrameUpper; ++f)
            for (int k = 0; k < 3; ++k)
                frames[side][f][k * 4 + 3] -= pose[side].r.shoulder[k] - shoulders[side][k];
        armik::Collar(base.left, base.up, base.back, shoulders[side], side == 1, 1.0f, frames[side][barehold::kFrameCollar]);
    }
}
constexpr float kGlovedLetsGo = 0.24f;  // hands.cpp's kHoldOutOfReach (0.80 m from the shoulder) less the arm's length
// One pose: both arms solved from the hands and shoulders given (afresh), the torso's and strip's palettes as the mod makes them, the cloth measured.
void PoseCloth(const Base& base, const float hands[2][12], const float shoulders[2][3], bool before, float most, const MeshWork& work, ClothStats& stats, bool gloved,
               std::vector<float>* torsoOut = nullptr) {
    using namespace barehold;
    ArmPose pose[2];
    float at[2][3], chest[3], body[3], bodyFrames[2][kArmFrames][12];
    bool on[2];
    for (int side = 0; side < 2; ++side) {
        armik::State fresh;
        SolveArm(hands[side], shoulders[side], base.left, base.up, base.back, side, fresh, pose[side], before, most);
        on[side] = pose[side].on && pose[side].finite;
        std::memcpy(at[side], on[side] ? pose[side].r.shoulder : shoulders[side], sizeof(at[side]));  // (an arm without a sleeve: its shoulder as given, hands.cpp)
        ++stats.arms;
        stats.off += !on[side];
        stats.drawnIn += on[side] && pose[side].keep < 1.0f;
        stats.partCap += !on[side] && pose[side].cap < 1.0f;
        stats.brought = std::max(stats.brought, pose[side].brought);
        if (on[side])
            stats.broughtOn = std::max(stats.broughtOn, pose[side].brought);
    }
    for (int k = 0; k < 3; ++k)
        chest[k] = (at[0][k] + at[1][k]) / 2, body[k] = (shoulders[0][k] + shoulders[1][k]) / 2;
    stats.chestOut = std::max(stats.chestOut, Dist(chest, body));
    AtBody(base, pose, shoulders, bodyFrames);
    const float (*const frames[2])[12] = {pose[0].frames, pose[1].frames};
    const float (*const atBody[2])[12] = {bodyFrames[0], bodyFrames[1]};
    static std::vector<float> skinned, skinnedBody;
    float palette[16 * 12];
    struct Table {
        const Part* part;
        const Edges* edges;
        const char* roles;
        const int8_t* map;
        int n;
    } const tables[] = {{&work.mesh->torso, &work.foldTorso, kSleeveTorsoRoles, kSleeveTorso, kSleeveTorsoBones},
                        {&work.mesh->strip, &work.foldStrip, kSleeveStripRoles, kSleeveStrip, kSleeveStripBones}};
    for (const Table& t : tables) {
        PlaceSleeves(t.roles, t.map, t.n, on, frames, chest, palette);
        SkinAll(*t.part, palette, skinned);
        Stretch(*t.edges, skinned, stats.cloth);
        PlaceSleeves(t.roles, t.map, t.n, on, atBody, body, palette);
        SkinAll(*t.part, palette, skinnedBody);
        Changed(*t.edges, skinned, skinnedBody, stats.change);
        if (torsoOut && t.part == &work.mesh->torso)
            *torsoOut = skinned;
    }
    if (gloved && work.mesh->haveGloved) {
        // The gloved chest on the gloved levels' rule (PlaceForearms: the collar at the solver's shoulder, drawn in from 12 cm brought along, never off).
        ArmPose g[2];
        for (int side = 0; side < 2; ++side) {
            armik::State fresh;
            SolveArm(hands[side], shoulders[side], base.left, base.up, base.back, side, fresh, g[side], true);
        }
        float glovedChest[3];
        for (int k = 0; k < 3; ++k)
            glovedChest[k] = (g[0].r.shoulder[k] + g[1].r.shoulder[k]) / 2;
        const bool both[2] = {true, true};
        const float (*const gframes[2])[12] = {g[0].frames, g[1].frames};
        PlaceSleeves(kGlovedRoles, kGlovedMap, static_cast<int>(std::size(kGlovedMap)), both, gframes, glovedChest, palette);
        SkinAll(work.mesh->gloved, palette, skinned);
        Stretch(work.foldGloved, skinned, stats.gloved);
        const float along = std::max(g[0].brought, g[1].brought);
        if (along <= kGlovedLetsGo) {
            Stretched s;
            Stretch(work.foldGloved, skinned, s);
            stats.glovedReach = std::max(stats.glovedReach, s.grown);
        }
        if (g[0].keep >= 1.0f && g[1].keep >= 1.0f) {
            ++stats.whole;
            AtBody(base, g, shoulders, bodyFrames);
            PlaceSleeves(kGlovedRoles, kGlovedMap, static_cast<int>(std::size(kGlovedMap)), both, atBody, body, palette);
            SkinAll(work.mesh->gloved, palette, skinnedBody);
            Changed(work.foldGloved, skinned, skinnedBody, stats.glovedChange);
        }
    }
}
// How far back the shoulders of a recorded draw go for the hold as filmed: the left shoulder brought kFilmedHoldBrought along, the right hand still in reach (its
// shoulder brought along 2 cm at most, by its wrist's range). Negative: this draw has no such hold (a reload's frame with the left hand down by the body or the
// right hand out ahead): the film's poses aren't made from it.
float FilmedHold(const Base& base) {
    auto brought = [&](float back, int side) {
        float shoulder[3];
        for (int k = 0; k < 3; ++k)
            shoulder[k] = base.shoulder[side][k] + base.back[k] * back;
        armik::State fresh;
        ArmPose pose;
        SolveArm(base.hand[side], shoulder, base.left, base.up, base.back, side, fresh, pose);
        return pose.brought;
    };
    float lo = 0.0f, hi = 0.6f;
    if (brought(lo, 0) >= kFilmedHoldBrought || brought(hi, 0) < kFilmedHoldBrought)
        return -1.0f;
    for (int i = 0; i < 24; ++i) {
        const float mid = (lo + hi) / 2;
        (brought(mid, 0) < kFilmedHoldBrought ? lo : hi) = mid;
    }
    return brought(hi, 1) <= 0.02f ? hi : -1.0f;
}
// A family's pose of one recorded draw: its hands and shoulders.
void FamilyPose(const Base& base, float filmedHold, int family, Rng& rng, float hands[2][12], float shoulders[2][3]) {
    using namespace armik::detail;
    const float back = kFamilies[family].back + (kFamilies[family].filmed ? filmedHold : 0.0f);
    for (int side = 0; side < 2; ++side) {
        std::memcpy(hands[side], base.hand[side], 12 * sizeof(float));
        for (int k = 0; k < 3; ++k)
            shoulders[side][k] = base.shoulder[side][k] + base.back[k] * back - base.up[k] * kFamilies[family].down;
        if (family != kRandomFamily)
            continue;
        // The hand turned any way about its wrist joint, then its wrist put somewhere round the shoulder.
        float wb[3], wrist[3], axis[3], way[3];
        Joint(armik::kWrist, side, wb);
        Point(hands[side], wb, wrist);
        do {
            for (float& a : axis)
                a = rng.Next() * 2 - 1;
        } while (Dot(axis, axis) > 1.0f || Normalize(axis) < 1e-3f);
        TurnPlacement(hands[side], axis, rng.Next() * 120.0f * armik::kDeg, wrist);
        do {
            for (float& w : way)
                w = rng.Next() * 2 - 1;
        } while (Dot(way, way) > 1.0f || Normalize(way) < 1e-3f || Dot(way, base.back) > 0.3f);
        const float far = 0.15f + 0.80f * rng.Next();
        for (int k = 0; k < 3; ++k)
            hands[side][k * 4 + 3] += shoulders[side][k] + way[k] * far - wrist[k];
    }
}

void Cloth(const MeshWork& work, const std::vector<Base>& bases, bool sweep) {
    using namespace barehold;
    if (!work.mesh || !work.mesh->haveGloved || bases.empty()) {
        Skip("s7", "no mesh dump of the bare body or of the gloved chest (mesh_8529_0_vb0.bin, _ib.bin), or no recordings: the cloth tied to the chest");
        return;
    }
    std::vector<float> holds(bases.size());
    int filmed = 0;
    float holdLeast = 9.0f, holdMost = 0.0f;
    for (size_t i = 0; i < bases.size(); ++i) {
        holds[i] = FilmedHold(bases[i]);
        if (holds[i] >= 0.0f)
            ++filmed, holdLeast = std::min(holdLeast, holds[i]), holdMost = std::max(holdMost, holds[i]);
    }
    // Every pose of every family, in one order whatever is measured in it.
    auto each = [&](auto&& pose) {
        Rng rng;
        for (size_t i = 0; i < bases.size(); ++i)
            for (int family = 0; family < kFamilyCount; ++family) {
                if (kFamilies[family].filmed && holds[i] < 0.0f)
                    continue;
                for (int n = 0; n < (family == kRandomFamily ? kRandomEach : 1); ++n) {
                    float hands[2][12], shoulders[2][3];
                    FamilyPose(bases[i], holds[i], family, rng, hands, shoulders);
                    pose(bases[i], family, hands, shoulders);
                }
            }
    };
    ClothStats now[kFamilyCount], was[kFamilyCount];
    int changed = 0, same = 0;  // poses whose every sleeve stays on: the torso as it was before the limit, to the bit
    std::vector<float> torsoNow, torsoWas;
    each([&](const Base& base, int family, const float hands[2][12], const float shoulders[2][3]) {
        const int offBefore = now[family].off;
        PoseCloth(base, hands, shoulders, false, kSleeveShoulderMost, work, now[family], true, &torsoNow);
        PoseCloth(base, hands, shoulders, true, 0.0f, work, was[family], false, &torsoWas);
        if (now[family].off == offBefore) {
            ++same;
            changed += torsoNow.size() != torsoWas.size() || std::memcmp(torsoNow.data(), torsoWas.data(), torsoNow.size() * sizeof(float)) != 0;
        }
    });
    // The bars, both the gloved levels' own chest and sleeves (8529) on their own rule in these very poses. What bringing a shoulder along changes: wherever that
    // rule shows its collars whole (both shoulders 12 cm along at most; further it draws the collar in, to hide what the cloth does). How much longer than in the
    // bind pose: wherever its hands hold on at all (0.24 m along at most).
    float bar = 0, grownBar = 0;
    int barPoses = 0;
    for (const ClothStats& s : now)
        bar = std::max(bar, s.glovedChange), grownBar = std::max(grownBar, s.glovedReach), barPoses += s.whole;
    if (sweep) {
        // How the limit was found: what bringing the shoulder along changes, each family, for each limit tried (the collar whole), then as the sleeves were first built.
        std::printf("     s7 sweep: %zu recorded draws, %d of them with a hold as filmed (the shoulders %.2f to %.2f m further back)\n", bases.size(), filmed, holdLeast, holdMost);
        std::printf("     %-10s", "limit (m)");
        for (const Family& f : kFamilies)
            std::printf(" %21s", f.name);
        std::printf("   (the cloth tied to the chest: mm changed by the shoulder brought along / mm longer than bound, at most / sleeves off of 100)\n");
        for (float most : {0.0f, 0.02f, 0.04f, 0.06f, 0.08f, 0.10f, 0.11f, 0.12f, 0.13f, 0.14f, 0.16f, 0.18f, 0.20f, 0.25f, 0.30f, 0.40f, 0.60f, 9.0f}) {
            ClothStats tried[kFamilyCount];
            each([&](const Base& base, int family, const float hands[2][12], const float shoulders[2][3]) {
                PoseCloth(base, hands, shoulders, false, most, work, tried[family], false);
            });
            std::printf("     %-10.2f", most);
            float all = 0;
            for (const ClothStats& s : tried) {
                all = std::max(all, s.change);
                std::printf(" %6.1f / %5.1f / %3.0f%%", s.change * 1000, s.cloth.grown * 1000, 100.0f * s.off / std::max(s.arms, 1));
            }
            std::printf("   worst %.1f%s\n", all * 1000, all <= bar ? "" : "  OVER the bar");
        }
        std::printf("     %-10s", "as built");
        for (const ClothStats& s : was)
            std::printf(" %6.1f / %5.1f / %4.2f", s.change * 1000, s.cloth.grown * 1000, s.brought);
        std::printf("   (mm changed / mm longer / m brought along at most; no limit, the collar drawn in from 12 cm)\n");
        std::printf("     %-10s", "gloved");
        for (const ClothStats& s : now)
            std::printf(" %6.1f / %5.1f / %4d", s.glovedChange * 1000, s.gloved.grown * 1000, s.whole);
        std::printf("   (the gloved chest on its own rule: mm changed with its collars whole / mm longer in every pose / poses with its collars whole)\n");
        std::printf("     the bars: %.1f mm changed (the gloved collars whole), %.1f mm longer (the gloved hands holding on, %.2f m along at most)\n", bar * 1000, grownBar * 1000, kGlovedLetsGo);
    }
    ClothStats all, allWas;
    for (int family = 0; family < kFamilyCount; ++family) {
        const ClothStats* from[2] = {&now[family], &was[family]};
        ClothStats* to[2] = {&all, &allWas};
        for (int k = 0; k < 2; ++k) {
            to[k]->cloth.grown = std::max(to[k]->cloth.grown, from[k]->cloth.grown), to[k]->cloth.ratio = std::max(to[k]->cloth.ratio, from[k]->cloth.ratio);
            to[k]->change = std::max(to[k]->change, from[k]->change);
            to[k]->arms += from[k]->arms, to[k]->off += from[k]->off, to[k]->drawnIn += from[k]->drawnIn, to[k]->partCap += from[k]->partCap;
            to[k]->broughtOn = std::max(to[k]->broughtOn, from[k]->broughtOn), to[k]->brought = std::max(to[k]->brought, from[k]->brought);
            to[k]->chestOut = std::max(to[k]->chestOut, from[k]->chestOut);
        }
    }
    // The rule itself: off beyond the limit, back on only 2 cm inside it, never on a number not finite; back on no nearer than the elbow's reach limit and the cap's
    // 5 cm together (an arm without its sleeve is capped whole), the limit no more than where the gloved levels' collar begins to draw in.
    const bool rule = SleeveStays(kSleeveShoulderMost, true) && !SleeveStays(kSleeveShoulderMost + 1e-4f, true) && !SleeveStays(kSleeveShoulderMost, false) &&
                      SleeveStays(kSleeveShoulderBack, false) && !SleeveStays(kSleeveShoulderBack + 1e-4f, false) && SleeveStays(0.0f, false) &&
                      !SleeveStays(std::nanf(""), true) && !SleeveStays(std::nanf(""), false) && kSleeveShoulderBack < kSleeveShoulderMost &&
                      kSleeveShoulderBack >= 0.05f + 0.05f - 1e-6f && kSleeveShoulderMost <= armik::kCollarDrawsIn + 1e-6f;
    const int blended = Blended(work.mesh->torso, kSleeveTorsoRoles) + Blended(work.mesh->strip, kSleeveStripRoles);
    // The film's own poses told from a good one: as first built, each of its six kinds changed the cloth by more than the bar; and the two holds (the game's, the
    // film's) keep both sleeves, as filmed.
    int toldApart = 0, holdsOff = 0, filmOff = 0;
    float filmWas = 9.0f;
    for (int family = kFilmFrom; family < kRandomFamily; ++family)
        toldApart += was[family].change > bar, filmOff += now[family].off, filmWas = std::min(filmWas, was[family].change);
    for (int family = 0; family < kHolds; ++family)
        holdsOff += now[family].off;
    Check(rule && bar > 0.0f && barPoses > 0 && filmed > 0 && all.change <= bar && all.cloth.grown <= grownBar && all.broughtOn <= kSleeveShoulderMost + 1e-6f &&
              all.chestOut <= kSleeveShoulderMost + 1e-6f && all.drawnIn == 0 && all.partCap == 0 && holdsOff == 0 && filmOff > 0 && changed == 0 && same > 0 &&
              toldApart == kRandomFamily - kFilmFrom && allWas.change > bar && allWas.drawnIn > 0,
          "s7",
          "the cloth tied to the chest (%d vertices of the torso and strip leaning on the chest's bones and an arm's, %zu + %zu edges; %zu recorded draws, each in the game's hold "
          "and with %d hands anywhere, %d of them in the hold as filmed and from it 20, 35 and 50 cm forward and raised 30, 50 and 70 cm: %d arms). Bringing a shoulder along "
          "changes an edge's length by %.1f mm at most; the bar, the gloved levels' own chest (8529) on its own rule wherever that shows its collars whole (%d of these poses): "
          "%.1f mm. An edge %.1f mm longer than bound at most (x%.2f: the fold itself, under a raised arm; the gloved chest's while its hands hold on: %.1f mm). A sleeve on its "
          "arm with the shoulder brought along %.3f m at most (the limit %.2f, back on within %.2f: %s), the chest's point %.3f m from the body's at most, the collar drawn in on "
          "%d (0 wanted); %d arms without their sleeve (%d in the two holds: 0 wanted; %d in the film's reaches and raises), %d of them not capped whole (0 wanted); of %d poses "
          "with both sleeves on, %d not the torso as before the limit to the bit (0 wanted: the holds as filmed). As first built (no limit, the collar drawn in from 12 cm): "
          "changed by %.1f mm at most, in the film's six kinds of pose by %.1f mm at the least, %d of the six over the bar (6 wanted: this check fails on that code); a sleeve "
          "on a shoulder %.2f m along, %d collars drawn in",
          blended, work.foldTorso.list.size(), work.foldStrip.list.size(), bases.size(), kRandomEach, filmed, all.arms, all.change * 1000, barPoses, bar * 1000, all.cloth.grown * 1000,
          all.cloth.ratio, grownBar * 1000, all.broughtOn, kSleeveShoulderMost, kSleeveShoulderBack, rule ? "ok" : "WRONG", all.chestOut, all.drawnIn, all.off, holdsOff, filmOff,
          all.partCap, same, changed, allWas.change * 1000, filmWas * 1000, toldApart, allWas.broughtOn, allWas.drawnIn);
}

}  // namespace

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "build\\survey", meshDir = argc > 2 ? argv[2] : "build";
    std::printf("baresleevetest: Blackburn's arms whole up to the shoulders (src/barestate.h, barehold.h, armik.h). Recordings: %s; the mesh dump: %s\n", dir.c_str(), meshDir.c_str());
    Draws();
    Tables();
    Gloved();
    OnlySolved();
    const bool sweep = argc > 3 && std::string(argv[3]) == "sweep";
    Mesh mesh;
    MeshWork work;
    std::vector<Base> bases;
    work.bases = &bases;
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
        work.foldTorso = FoldEdges(mesh.torso, barehold::kSleeveTorsoRoles);
        work.foldStrip = FoldEdges(mesh.strip, barehold::kSleeveStripRoles);
        if (mesh.haveGloved)
            work.foldGloved = FoldEdges(mesh.gloved, kGlovedRoles);
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
    Cloth(work, bases, sweep);
    std::printf("%d of %d checks failed%s\n", g_failed, g_checks, g_skipped ? " (some skipped: no recordings or no mesh dump)" : "");
    return g_failed;
}

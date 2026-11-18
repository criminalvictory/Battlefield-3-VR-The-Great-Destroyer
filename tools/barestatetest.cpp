// Blackburn's bare arms by their draws, handcuffed and with the cuffs off (src/barestate.h, header only, as skin.cpp's draw hook asks it) without the
// game. Each check prints PASS, FAIL or SKIP, then the totals; the exit code is the number failed.
//   b1  handcuffed (the train: the draws of build\survey\AKS74u_empty.bin, UMP45_empty.bin and M1014_empty.bin, written here): the arms are taken
//       (state 0), the cuff as the cuff, the body's four and the piece of uniform as the body; the guns' own draws are nobody's
//   b2  the cuffs off (after the grapple: every draw of build\survey\aks_test_idle.bin, written here): the same arm mesh from 13371 is taken (state
//       1), no draw is a cuff, the body's four moved draws and the piece of uniform are the body, the AKS-74u's five draws are nobody's
//   b3  nothing else changed: over every index count and start named anywhere (both states, the old table, the draws below, each one off either way
//       too) the answer is the old one (the table as skin.cpp had it until 2026-11-18, written here again) but for the five draws of the cuffs off
//   b4  no other level's draw is taken: of the draws recorded on the levels with gloves (tools/barestatetest_draws.inc, 33 recordings) and the mod's
//       own tables' (the gloves, their body, Dima's and Kaffarov's bare forearms, the M416, the launchers, the scopes) none is the arms or a cuff,
//       and none is the body that wasn't before
//   b5  another base vertex is never taken; a state without a cuff takes no draw of no indices; the two states' draws are all different
//   b6  the recordings themselves (weapon.txt "reloadrec" with "drawscan"; SKIP where there are none): in each with the gloves' draw nothing is the
//       arms or a cuff and none of the cuffs-off draws is there; in each with bare arms the arms' draw is taken in one state only, its body's draws
//       with it; aks_test_idle.bin is the cuffs off
// Usage: barestatetest [<folder with the recordings>] (build\survey by default, from the project's folder). Reads only.

#include "../src/barestate.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <windows.h>

namespace {

using barestate::Kind;
using barestate::KindOf;
using Key = std::pair<uint32_t, uint32_t>;  // index count, start

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

// The table as skin.cpp had it until 2026-11-18 (kBareArms, kBareCuff, kBareBody), the handcuffed state alone.
Kind Old(uint32_t count, uint32_t start, int32_t base) {
    if (base != 0)
        return barestate::kNone;
    if (count == 18108 && start == 16053)
        return barestate::kArms;
    if (count == 2682 && start == 13371)
        return barestate::kCuff;
    static constexpr Key kBody[] = {{5604, 0}, {7545, 5604}, {26346, 34161}, {7815, 60507}, {3492, 3492}, {222, 13149}, {3492, 0}, {11970, 11970}};
    for (const Key& k : kBody)
        if (k.first == count && k.second == start)
            return barestate::kBody;
    return barestate::kNone;
}

// Handcuffed, on the train: every draw of 200 indices and more with the player's own palette in AKS74u_empty.bin, UMP45_empty.bin and M1014_empty.bin.
constexpr Key kCuffedArms{18108, 16053}, kCuffedCuff{2682, 13371};
constexpr Key kCuffedBody[] = {{5604, 0}, {7545, 5604}, {26346, 34161}, {7815, 60507}, {222, 13149}};
constexpr Key kCuffedGuns[] = {{17991, 0}, {312, 18957}, {894, 18063}, {19269, 19269},  // the AKS-74u
                               {234, 0},   {19314, 234}, {19638, 19638}};               // the UMP-45
// The cuffs off: every draw of aks_test_idle.bin (15), the AKS-74u in hand.
constexpr Key kFreeArms{18108, 13371};
constexpr Key kFreeBody[] = {{7545, 0}, {5604, 7545}, {23664, 31479}, {7815, 55143}, {222, 13149}};
constexpr Key kFreeGun[] = {{17991, 0}, {72, 17991}, {312, 18957}, {894, 18063}, {19269, 19269}, {282, 0}, {288, 0}, {294, 0}, {300, 0}};
// What the cuffs' going changed: these five and nothing else.
constexpr Key kNew[] = {{18108, 13371}, {7545, 0}, {5604, 7545}, {23664, 31479}, {7815, 55143}};

// The levels with gloves, as recorded.
constexpr Key kGloved[] = {
#include "barestatetest_draws.inc"
};
// The mod's own tables' draws on other levels (skin.cpp): the gloves and their depth-only copy, the chest and sleeves, the body's depth-only copy,
// the legs, Dima's and Kaffarov's bare forearms, the M416's three, the M320's, the scope's, the launchers', the train pistol.
constexpr Key kOthers[] = {{13755, 15051}, {7617, 49995}, {8529, 0},     {21189, 28806}, {4506, 8529},   {2016, 13035}, {12768, 0},
                           {312, 12768},   {13080, 13080}, {7848, 0},    {1440, 7848},   {9288, 9288},   {9273, 12},    {9273, 9594},
                           {3414, 0},      {18222, 3414},  {120, 21636}, {21756, 21756}, {11970, 0},     {16188, 4689}, {20298, 354}};

bool IsNew(uint32_t count, uint32_t start) {
    for (const Key& k : kNew)
        if (k.first == count && k.second == start)
            return true;
    return false;
}

// A recording's draws: 8 ints a record (the draw's index count, the frame, the eye, its start, the rounds, the spare, the ms, the bones' count),
// then 48 bytes a bone. False when it isn't one.
bool ReadDraws(const std::string& path, std::set<Key>& draws) {
    FILE* f = nullptr;
    if (fopen_s(&f, path.c_str(), "rb") != 0 || !f)
        return false;
    setvbuf(f, nullptr, _IOFBF, 1 << 20);
    int records = 0;
    bool whole = true;
    for (;;) {
        int32_t head[8];
        if (fread(head, sizeof(head), 1, f) != 1)
            break;
        if (head[7] <= 0 || head[7] > 64 || head[0] <= 0) {
            whole = false;
            break;
        }
        if (_fseeki64(f, static_cast<long long>(head[7]) * 48, SEEK_CUR) != 0)
            break;
        draws.insert({static_cast<uint32_t>(head[0]), static_cast<uint32_t>(head[3])});
        ++records;
    }
    fclose(f);
    return whole && records > 0;
}

}  // namespace

int main(int argc, char** argv) {
    // b1
    {
        bool ok = barestate::ArmsState(kCuffedArms.first, kCuffedArms.second, 0) == 0 && KindOf(kCuffedArms.first, kCuffedArms.second, 0) == barestate::kArms;
        Check(ok, "b1 handcuffed: 18108 from 16053 is the bare arms, state 0");
        Check(KindOf(kCuffedCuff.first, kCuffedCuff.second, 0) == barestate::kCuff && barestate::ArmsState(kCuffedCuff.first, kCuffedCuff.second, 0) == -1,
              "b1 handcuffed: 2682 from 13371 is the cuff, not the arms");
        ok = true;
        for (const Key& k : kCuffedBody)
            ok = ok && KindOf(k.first, k.second, 0) == barestate::kBody;
        Check(ok, "b1 handcuffed: the body's five draws are the body");
        ok = true;
        for (const Key& k : kCuffedGuns)
            ok = ok && KindOf(k.first, k.second, 0) == barestate::kNone;
        Check(ok, "b1 handcuffed: the AKS-74u's and the UMP-45's draws are nobody's");
    }
    // b2
    {
        Check(barestate::ArmsState(kFreeArms.first, kFreeArms.second, 0) == 1 && KindOf(kFreeArms.first, kFreeArms.second, 0) == barestate::kArms,
              "b2 the cuffs off: 18108 from 13371 is the bare arms, state 1");
        bool ok = true, cuff = false;
        for (const Key& k : kFreeBody) {
            ok = ok && KindOf(k.first, k.second, 0) == barestate::kBody;
            cuff = cuff || KindOf(k.first, k.second, 0) == barestate::kCuff;
        }
        Check(ok, "b2 the cuffs off: the body's five draws (7545/0, 5604/7545, 23664/31479, 7815/55143, 222/13149) are the body");
        ok = true;
        for (const Key& k : kFreeGun) {
            ok = ok && KindOf(k.first, k.second, 0) == barestate::kNone;
            cuff = cuff || KindOf(k.first, k.second, 0) == barestate::kCuff;
        }
        Check(ok, "b2 the cuffs off: the AKS-74u's draws are nobody's");
        Check(!cuff && barestate::kStates[1].cuff.count == 0, "b2 the cuffs off: no draw of it is a cuff");
    }
    // b3
    {
        std::set<uint32_t> counts, starts;
        auto name = [&](uint32_t count, uint32_t start) {
            for (int d = -1; d <= 1; ++d) {
                counts.insert(count + d);
                starts.insert(start + d);
            }
        };
        for (const barestate::State& s : barestate::kStates) {
            name(s.arms.count, s.arms.start);
            name(s.cuff.count, s.cuff.start);
            for (const barestate::Draw& b : barestate::BodyOf(s))
                name(b.count, b.start);
        }
        for (const barestate::Draw& b : barestate::kBodyEither)
            name(b.count, b.start);
        for (const Key& k : kCuffedGuns)
            name(k.first, k.second);
        for (const Key& k : kFreeGun)
            name(k.first, k.second);
        for (const Key& k : kOthers)
            name(k.first, k.second);
        for (const Key& k : kGloved)
            name(k.first, k.second);
        long long pairs = 0, differ = 0, wrong = 0;
        for (uint32_t count : counts)
            for (uint32_t start : starts) {
                ++pairs;
                const Kind now = KindOf(count, start, 0), was = Old(count, start, 0);
                if (now == was)
                    continue;
                ++differ;
                if (!IsNew(count, start) || was != barestate::kNone)
                    ++wrong;
            }
        std::printf("     %lld pairs of a count and a start: %lld answered otherwise than before, %lld of them not the cuffs-off draws\n", pairs, differ, wrong);
        Check(wrong == 0 && differ == static_cast<long long>(std::size(kNew)), "b3 the answer is the old table's everywhere but for the five draws of the cuffs off");
        bool ok = true;
        for (const Key& k : kNew)
            ok = ok && Old(k.first, k.second, 0) == barestate::kNone && KindOf(k.first, k.second, 0) != barestate::kNone;
        Check(ok, "b3 each of the five was nobody's before and is taken now");
    }
    // b4
    {
        int arms = 0, newBody = 0, body = 0;
        auto look = [&](const Key& k) {
            const Kind now = KindOf(k.first, k.second, 0);
            arms += now == barestate::kArms || now == barestate::kCuff;
            body += now == barestate::kBody;
            newBody += now != Old(k.first, k.second, 0);
        };
        for (const Key& k : kGloved)
            look(k);
        std::printf("     %zu draws of the levels with gloves: %d the arms or a cuff, %d the body (as before: the chain's 3492 from 0 is a gun's piece there), %d new\n",
                    std::size(kGloved), arms, body, newBody);
        Check(arms == 0, "b4 no draw recorded on a level with gloves is the bare arms or a cuff");
        Check(newBody == 0, "b4 none of them is answered otherwise than before");
        arms = newBody = body = 0;
        for (const Key& k : kOthers)
            look(k);
        Check(arms == 0 && body == 0 && newBody == 0, "b4 the mod's other tables' draws (gloves, their body, Dima's and Kaffarov's forearms, guns, launchers, the pistol) are nobody's");
        bool ok = true;
        for (const Key& k : kGloved)
            ok = ok && !IsNew(k.first, k.second);
        for (const Key& k : kOthers)
            ok = ok && !IsNew(k.first, k.second);
        Check(ok, "b4 none of the cuffs-off draws is among them");
    }
    // b5
    {
        bool ok = true;
        for (const barestate::State& s : barestate::kStates)
            for (int32_t base : {1, -1, 4096})
                ok = ok && KindOf(s.arms.count, s.arms.start, base) == barestate::kNone && barestate::ArmsState(s.arms.count, s.arms.start, base) == -1 &&
                     KindOf(s.torso.count, s.torso.start, base) == barestate::kNone;
        Check(ok, "b5 another base vertex is never taken");
        Check(KindOf(0, 0, 0) == barestate::kNone && KindOf(0, 13371, 0) == barestate::kNone, "b5 a draw of no indices is nobody's (the cuffs-off state has no cuff)");
        std::set<Key> all;
        size_t n = 0;
        for (const barestate::State& s : barestate::kStates) {
            all.insert({s.arms.count, s.arms.start}), ++n;
            if (s.cuff.count)
                all.insert({s.cuff.count, s.cuff.start}), ++n;
            for (const barestate::Draw& b : barestate::BodyOf(s))
                all.insert({b.count, b.start}), ++n;
        }
        for (const barestate::Draw& b : barestate::kBodyEither)
            all.insert({b.count, b.start}), ++n;
        Check(all.size() == n, "b5 no draw is in the tables twice");
    }
    // b6
    {
        const std::string folder = argc > 1 ? argv[1] : "build\\survey";
        WIN32_FIND_DATAA found;
        const HANDLE h = FindFirstFileA((folder + "\\*.bin").c_str(), &found);
        int gloved = 0, bare = 0, badGloved = 0, badBare = 0;
        bool idleSeen = false, idleFree = false;
        if (h != INVALID_HANDLE_VALUE) {
            do {
                std::set<Key> draws;
                if (!ReadDraws(folder + "\\" + found.cFileName, draws))
                    continue;
                int states[barestate::kStateCount] = {};
                int cuffs = 0, fresh = 0;
                for (const Key& k : draws) {
                    if (const int s = barestate::ArmsState(k.first, k.second, 0); s >= 0)
                        ++states[s];
                    cuffs += KindOf(k.first, k.second, 0) == barestate::kCuff;
                    fresh += IsNew(k.first, k.second);
                }
                const bool hasArms = states[0] + states[1] > 0;
                // tools\baregloves.ps1's copies (<name>_gloves.bin) have both: the bare arms' and the gloves' made from them.
                if (!hasArms && draws.count({13755, 15051})) {
                    ++gloved;
                    if (cuffs || fresh) {
                        ++badGloved;
                        std::printf("     %s (gloves): %d cuff draws, %d of the cuffs-off draws\n", found.cFileName, cuffs, fresh);
                    }
                } else if (hasArms) {
                    ++bare;
                    const int state = states[1] ? 1 : 0;
                    bool whole = states[0] + states[1] == 1 && (state == 0) == (cuffs == 1);
                    for (const barestate::Draw& b : barestate::BodyOf(barestate::kStates[state]))
                        whole = whole && draws.count({b.count, b.start}) == 1;
                    for (const barestate::Draw& b : barestate::BodyOf(barestate::kStates[1 - state]))
                        whole = whole && draws.count({b.count, b.start}) == 0;
                    if (!whole) {
                        ++badBare;
                        std::printf("     %s (bare arms): not one state's draws alone\n", found.cFileName);
                    }
                    if (_stricmp(found.cFileName, "aks_test_idle.bin") == 0) {
                        idleSeen = true;
                        idleFree = state == 1 && whole;
                    }
                }
            } while (FindNextFileA(h, &found));
            FindClose(h);
        }
        if (gloved + bare == 0) {
            Skip(("b6 no recordings in " + folder).c_str());
        } else {
            std::printf("     %s: %d recordings with gloves, %d with bare arms\n", folder.c_str(), gloved, bare);
            if (gloved)
                Check(badGloved == 0, "b6 the recordings with gloves: nothing is the arms or a cuff, none of the cuffs-off draws is there");
            else
                Skip("b6 no recording with gloves");
            if (bare)
                Check(badBare == 0, "b6 the recordings with bare arms: the arms taken in one state only, that state's body with them and none of the other's");
            else
                Skip("b6 no recording with bare arms");
            if (idleSeen)
                Check(idleFree, "b6 aks_test_idle.bin (after the grapple) is the cuffs off: its arms taken, its body's draws with them");
            else
                Skip("b6 no aks_test_idle.bin");
        }
    }
    std::printf("%d checks, %d failed, %d skipped\n", g_checks, g_failed, g_skipped);
    return g_failed;
}

// The look angles' row swept from the copies written over (src/aimrules.h: SlotWords, SweepWanted, SweepTakes, SweepChoice, as aim.cpp's Search runs
// them), without the game: a made-up piece of memory with the row's slots in it, a stand-in for the game (30 ticks a second: the slot it reads copied
// over the others, its camera built from it) and for aim.cpp's Proves (the same steps: the nudge only over the value read, written over after 50 ms,
// kept, followed, undone, come back). aim.cpp's Search itself can't run here (the game's memory, its camera, mouse aim's hold): the search below is its
// first pass and the sweep after it written out again around the same aimrules calls.
//   1  SlotWords: the row's shape (a pointer, two zeros, the yaw, two zeros, the pitch), and each way of not having it
//   2  SweepTakes: the shape at the camera's angles; 0.0099 off passes, 0.0101 not; a zero pitch or yaw never, whatever the camera; not a number never
//   3  SweepWanted: only with find, nothing found, not crowded, a slot-shaped candidate written over
//   4  the launch that never found them (three copies reached, the slot the game reads 0x300 to 0x880 past them, above and below): without the sweep
//      nothing in five searches, as it was; with it the first search proves the read slot; every write a nudge on a slot at the camera's angles, each
//      undone; the null row before it, a slot at another look and a stray float never written
//   5  the launch that worked (the read slot among the candidates): found in the first pass, the row never looked through, nothing read for it
//   6  more copies the walk didn't reach, nearer than the read slot: four proved a search (written over), the rest and the read slot in the next
//   7  the read slot out of the sweep's reach (0x940): never found, at most four nudges a search, tried again from the first after all were tried
//   8  SweepChoice alone: nearest first, above before below, each place once though three copies reach it, a candidate and a place swept before left
//      out and counted, the count past the four it gives
//   9  broken inputs: a camera not a number, no copies, a copy at the bottom or the top of memory, memory that can't be read: nothing taken; the time
// Usage: aimsweeptest
// The exit code is the number of failed checks.

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <unordered_set>
#include <vector>

#include "../src/aimrules.h"

namespace {

int g_failed = 0, g_counted = 0;

void Report(const char* id, const char* what, bool pass, const char* format, ...) {
    char numbers[1536];
    va_list args;
    va_start(args, format);
    std::vsnprintf(numbers, sizeof(numbers), format, args);
    va_end(args);
    std::printf("%-4s %-5s %s | %s\n", id, pass ? "PASS" : "FAIL", what, numbers);
    ++g_counted;
    g_failed += !pass;
}

constexpr float kNaN = std::numeric_limits<float>::quiet_NaN();
constexpr float kNudge = 0.03f;  // aim.cpp's
constexpr size_t kMostPairs = 16;  // aim.cpp's

// The made-up memory, and the game on it.
struct World {
    static constexpr uintptr_t kBase = 0x20000000, kSize = 0x8000;
    std::vector<uint32_t> words = std::vector<uint32_t>(kSize / 4, 0);
    uintptr_t readSlot = 0;          // the slot the game reads
    std::vector<uintptr_t> copies;   // those it writes over from it at each tick
    float cameraYaw = 0.0f, cameraPitch = 0.0f;
    std::vector<uintptr_t> written;  // every word the mod wrote (the nudges and their undoing)
    int reads = 0;                   // the sweep's reads

    bool Read(uintptr_t at, void* out, size_t size) const {
        if (at < kBase || at + size > kBase + kSize || (at & 3))
            return false;
        std::memcpy(out, &words[(at - kBase) / 4], size);
        return true;
    }
    float Float(uintptr_t at) const {
        float v = 0.0f;
        Read(at, &v, 4);
        return v;
    }
    void Set(uintptr_t at, float v) { std::memcpy(&words[(at - kBase) / 4], &v, 4); }
    void SetWord(uintptr_t at, uint32_t v) { words[(at - kBase) / 4] = v; }
    void MakeSlot(uintptr_t at, float yaw, float pitch) {
        SetWord(at - 0xC, 0x30001000);
        SetWord(at - 8, 0), SetWord(at - 4, 0), SetWord(at + 4, 0), SetWord(at + 8, 0);
        Set(at, yaw), Set(at + 0xC, pitch);
    }
    // aim.cpp's SwapIfUnchanged.
    bool Swap(uintptr_t at, float expected, float value) {
        uint32_t was = 0, now = 0;
        std::memcpy(&was, &expected, 4);
        std::memcpy(&now, &value, 4);
        if (at < kBase || at + 4 > kBase + kSize || words[(at - kBase) / 4] != was)
            return false;
        words[(at - kBase) / 4] = now;
        written.push_back(at);
        return true;
    }
    void Tick(int ticks = 1) {
        for (int t = 0; t < ticks; ++t) {
            const float yaw = Float(readSlot), pitch = Float(readSlot + 0xC);
            for (const uintptr_t copy : copies)
                Set(copy, yaw), Set(copy + 0xC, pitch);
            cameraYaw = yaw, cameraPitch = pitch;
        }
    }
};

enum class Proof { kNo, kKept, kYes, kBusy, kOverwritten };

void Undo(World& w, uintptr_t at, float nudged, float nudge) {
    const float now = w.Float(at);
    if (std::fabs(now - nudged) < kNudge * 0.3f)
        w.Swap(at, now, now - nudge);
}

// aim.cpp's Proves(at, false, true), its sleeps as the game's ticks (33 ms each).
Proof Proves(World& w, uintptr_t at) {
    float yaw = 0.0f, pitch = 0.0f;
    if (!w.Read(at, &yaw, 4) || !w.Read(at + 0xC, &pitch, 4) || aimrules::YawApart(yaw, w.cameraYaw) > 0.02f)
        return Proof::kNo;
    if (std::fabs(yaw) < 0.02f || std::fabs(pitch) < 0.02f || !aimrules::LooksLikeAngle(yaw, false) || !aimrules::LooksLikeAngle(pitch, true))
        return Proof::kNo;
    const float still[2] = {w.cameraYaw, w.cameraPitch};
    w.Tick(4);
    yaw = w.Float(at), pitch = w.Float(at + 0xC);
    if (aimrules::YawApart(yaw, w.cameraYaw) > 0.02f || std::fabs(yaw) < 0.02f || std::fabs(pitch) < 0.02f)
        return Proof::kNo;
    const float pitchNudge = pitch > 0.0f ? -kNudge : kNudge;
    if (!w.Swap(at, yaw, yaw + kNudge))
        return Proof::kNo;
    if (!w.Swap(at + 0xC, pitch, pitch + pitchNudge)) {
        Undo(w, at, yaw + kNudge, kNudge);
        return Proof::kNo;
    }
    w.Tick(2);
    if (!(std::fabs(w.Float(at) - (yaw + kNudge)) < kNudge * 0.3f && std::fabs(w.Float(at + 0xC) - (pitch + pitchNudge)) < kNudge * 0.3f)) {
        Undo(w, at, yaw + kNudge, kNudge);
        Undo(w, at + 0xC, pitch + pitchNudge, pitchNudge);
        return Proof::kOverwritten;
    }
    w.Tick(3);
    const bool kept = std::fabs(w.Float(at) - (yaw + kNudge)) < kNudge * 0.3f && std::fabs(w.Float(at + 0xC) - (pitch + pitchNudge)) < kNudge * 0.3f;
    const bool followed = std::fabs((w.cameraYaw - still[0]) - kNudge) < kNudge * 0.3f && std::fabs((w.cameraPitch - still[1]) - pitchNudge) < kNudge * 0.3f;
    Undo(w, at, yaw + kNudge, kNudge);
    Undo(w, at + 0xC, pitch + pitchNudge, pitchNudge);
    if (!kept)
        return Proof::kOverwritten;
    if (!followed)
        return Proof::kKept;
    w.Tick(5);
    return aimrules::YawApart(w.cameraYaw, still[0]) < kNudge * 0.3f && std::fabs(w.cameraPitch - still[1]) < kNudge * 0.3f ? Proof::kYes : Proof::kKept;
}

struct Outcome {
    uintptr_t found = 0;
    bool firstPass = false;  // found by the first pass
    bool swept = false;      // the row was looked through
    int candidates = 0, shaped = 0, writtenOver = 0;
    int taken = 0, proved = 0, before = 0;
    aimrules::Swept at[aimrules::kSweepMost] = {};
    Proof answer[aimrules::kSweepMost] = {};
};

// aim.cpp's Search with find from its first pass on: reached is what the walk and Matching hand over (the pairs near the soldier), the slot-shaped
// first; then the sweep (sweep 0: as it was before it). tried: the soldier's places swept before (aim.cpp's g_swept).
Outcome Search(World& w, const std::vector<uintptr_t>& reached, std::unordered_set<uintptr_t>& tried, bool sweep) {
    Outcome o;
    w.Tick(2);  // (the hold: the camera settled)
    if (!(std::fabs(w.cameraPitch) > 0.02f && std::fabs(w.cameraYaw) > 0.02f))
        return o;
    std::vector<uintptr_t> pairs, rest;
    for (const uintptr_t at : reached) {
        uint32_t words[7] = {};
        float pair[4] = {}, y = 0.0f, p = 0.0f;
        if (!w.Read(at, pair, sizeof(pair)) || !aimrules::StrictPair(pair[0], pair[3], w.cameraYaw, w.cameraPitch, false))
            continue;
        (w.Read(at - 0xC, words, sizeof(words)) && aimrules::SlotWords(words, y, p) ? pairs : rest).push_back(at);
    }
    o.shaped = static_cast<int>(pairs.size());
    pairs.insert(pairs.end(), rest.begin(), rest.end());
    o.candidates = static_cast<int>(pairs.size());
    const bool crowded = pairs.size() > kMostPairs;
    uintptr_t copies[kMostPairs] = {};
    for (size_t k = 0; k < pairs.size() && !crowded && !o.found; ++k) {
        const Proof answer = Proves(w, pairs[k]);
        if (answer == Proof::kYes)
            o.found = pairs[k], o.firstPass = true;
        if (answer == Proof::kOverwritten && k < static_cast<size_t>(o.shaped) && o.writtenOver < static_cast<int>(kMostPairs))
            copies[o.writtenOver++] = pairs[k];
    }
    if (sweep && aimrules::SweepWanted(true, o.found != 0, crowded, o.writtenOver) && std::fabs(w.cameraPitch) > 0.02f && std::fabs(w.cameraYaw) > 0.02f) {
        o.swept = true;
        o.taken = aimrules::SweepChoice(
            copies, o.writtenOver, w.cameraYaw, w.cameraPitch,
            [&](uintptr_t at, uint32_t* words) {
                ++w.reads;
                return w.Read(at - 0xC, words, 7 * sizeof(uint32_t));
            },
            [&](uintptr_t at) { return std::find(pairs.begin(), pairs.end(), at) != pairs.end() || tried.count(at) != 0; }, o.at, aimrules::kSweepMost, &o.before);
        if (!o.taken && o.before)
            tried.clear();
        for (int n = 0; n < o.taken && n < aimrules::kSweepMost && !o.found; ++n) {
            o.answer[n] = Proves(w, o.at[n].at);
            o.proved = n + 1;
            if (o.answer[n] == Proof::kYes)
                o.found = o.at[n].at;
            else if (o.answer[n] != Proof::kBusy)
                tried.insert(o.at[n].at);
        }
    }
    return o;
}

constexpr uintptr_t kRow = World::kBase + 0x4000;  // the first copy the walk reaches
constexpr float kYaw = 2.4871f, kPitch = -0.3127f;

// The failing launch's memory: three copies side by side at kRow, the read slot at readAt, and around them what must never be written: the pointer
// followed by nulls just before the row, a slot of another look, a stray float pair at the camera's angles without the slot's shape.
World Launch(uintptr_t readAt, std::vector<uintptr_t>* reached) {
    World w;
    for (int n = 0; n < 3; ++n) {
        w.MakeSlot(kRow + n * 0x40, kYaw, kPitch);
        w.copies.push_back(kRow + n * 0x40);
    }
    w.MakeSlot(readAt, kYaw, kPitch);
    w.readSlot = readAt;
    w.SetWord(kRow - 0x40 - 0xC, 0x30002000);                // the null row: a pointer, then zeros
    w.MakeSlot(kRow + 0x100, kYaw + 0.5f, kPitch + 0.2f);    // a slot of another look
    w.Set(kRow - 0x200, kYaw), w.Set(kRow - 0x200 + 0xC, kPitch);  // a stray pair: no pointer before it
    w.SetWord(kRow - 0x200 - 0xC, 7);
    w.Tick();
    if (reached)
        *reached = w.copies;
    return w;
}

// Every word the mod wrote lies on a slot (its yaw or pitch) that is the read slot or one of the game's copies.
bool WritesOnSlotsOnly(const World& w) {
    for (const uintptr_t at : w.written) {
        bool on = at == w.readSlot || at == w.readSlot + 0xC;
        for (const uintptr_t copy : w.copies)
            on = on || at == copy || at == copy + 0xC;
        if (!on)
            return false;
    }
    return true;
}

}  // namespace

int main() {
    // 1
    {
        uint32_t good[7] = {0x30001000, 0, 0, 0, 0, 0, 0};
        std::memcpy(&good[3], &kYaw, 4), std::memcpy(&good[6], &kPitch, 4);
        float y = 0.0f, p = 0.0f;
        const bool shape = aimrules::SlotWords(good, y, p) && y == kYaw && p == kPitch;
        int refused = 0;
        const auto with = [&](int word, uint32_t value) {
            uint32_t w[7];
            std::memcpy(w, good, sizeof(w));
            w[word] = value;
            float a = 0.0f, b = 0.0f;
            refused += !aimrules::SlotWords(w, a, b);
        };
        with(0, 0), with(0, 0x8000), with(0, 0x30001001), with(0, 0xFFFF0004u), with(1, 1), with(2, 1), with(4, 1), with(5, 1);
        const float steep = 2.0f;
        uint32_t bits = 0;
        std::memcpy(&bits, &steep, 4);
        with(6, bits), with(3, 0x7FC00000u), with(6, 0x7FC00000u), with(3, 0x30001000);
        Report("1", "SlotWords", shape && refused == 12, "the row's shape %d; refused %d of 12 without it", shape, refused);
    }
    // 2
    {
        const auto takes = [](float yaw, float pitch, float cy, float cp) {
            uint32_t w[7] = {0x30001000, 0, 0, 0, 0, 0, 0};
            std::memcpy(&w[3], &yaw, 4), std::memcpy(&w[6], &pitch, 4);
            return aimrules::SweepTakes(w, cy, cp);
        };
        const bool at = takes(kYaw, kPitch, kYaw, kPitch), close = takes(kYaw + 0.0099f, kPitch - 0.0099f, kYaw, kPitch);
        const bool apart = takes(kYaw + 0.0101f, kPitch, kYaw, kPitch) || takes(kYaw, kPitch + 0.0101f, kYaw, kPitch);
        const bool zero = takes(kYaw, 0.0f, kYaw, 0.0f) || takes(kYaw, 0.015f, kYaw, 0.015f) || takes(0.0f, kPitch, 0.0f, kPitch) || takes(0.015f, kPitch, 0.015f, kPitch) ||
                          takes(6.2831853f, kPitch, 0.0f, kPitch);
        const bool nan = takes(kNaN, kPitch, kYaw, kPitch) || takes(kYaw, kNaN, kYaw, kPitch) || takes(kYaw, kPitch, kNaN, kPitch) || takes(kYaw, kPitch, kYaw, kNaN);
        uint32_t noShape[7] = {7, 0, 0, 0, 0, 0, 0};
        std::memcpy(&noShape[3], &kYaw, 4), std::memcpy(&noShape[6], &kPitch, 4);
        const bool stray = aimrules::SweepTakes(noShape, kYaw, kPitch);
        Report("2", "SweepTakes", at && close && !apart && !zero && !nan && !stray, "at the camera's %d, 0.0099 off %d, 0.0101 off %d, a zero %d, not a number %d, no shape %d", at,
               close, apart, zero, nan, stray);
    }
    // 3
    {
        const bool wanted = aimrules::SweepWanted(true, false, false, 1) && aimrules::SweepWanted(true, false, false, 3);
        const bool never = aimrules::SweepWanted(false, false, false, 3) || aimrules::SweepWanted(true, true, false, 3) || aimrules::SweepWanted(true, false, true, 3) ||
                           aimrules::SweepWanted(true, false, false, 0);
        Report("3", "SweepWanted", wanted && !never, "wanted with a written-over copy %d; without find, found, crowded or none written over %d", wanted, never);
    }
    // 4
    {
        int cases = 0, oldFound = 0, found = 0, firstSearch = 0, writesOk = 0, undone = 0, others = 0, most = 0;
        long nearest = 0, farthest = 0;
        for (int side = 0; side < 2; ++side)
            for (uintptr_t d = 0x300; d <= 0x880; d += 0x40) {
                // (Above: past the last copy; below: before the first.)
                const uintptr_t readAt = side ? kRow - d : kRow + 0x80 + d;
                ++cases;
                std::vector<uintptr_t> reached;
                {
                    World w = Launch(readAt, &reached);
                    std::unordered_set<uintptr_t> tried;
                    for (int s = 0; s < 5; ++s)
                        oldFound += Search(w, reached, tried, false).found != 0;
                }
                World w = Launch(readAt, &reached);
                const std::vector<uint32_t> before = w.words;
                std::unordered_set<uintptr_t> tried;
                const Outcome o = Search(w, reached, tried, true);
                found += o.found == readAt && !o.firstPass && o.swept && o.candidates == 3 && o.shaped == 3 && o.writtenOver == 3;
                firstSearch += o.proved == 1 && o.answer[0] == Proof::kYes && o.at[0].at == readAt;
                writesOk += WritesOnSlotsOnly(w) && !w.written.empty();
                most = std::max(most, o.proved);
                const long distance = o.at[0].Distance();
                if (cases == 1 || std::labs(distance) < std::labs(nearest))
                    nearest = distance;
                if (cases == 1 || std::labs(distance) > std::labs(farthest))
                    farthest = distance;
                // Every nudge undone: the angles as they were to a float's last bits (a nudge added and taken off), every other word bit for bit.
                bool same = true, rest = true;
                for (size_t k = 0; k < w.words.size(); ++k) {
                    const uintptr_t at = World::kBase + k * 4;
                    bool angle = at == readAt || at == readAt + 0xC;
                    for (const uintptr_t copy : w.copies)
                        angle = angle || at == copy || at == copy + 0xC;
                    if (angle) {
                        float a = 0.0f, b = 0.0f;
                        std::memcpy(&a, &w.words[k], 4), std::memcpy(&b, &before[k], 4);
                        same = same && std::fabs(a - b) < 1e-6f;
                    } else {
                        rest = rest && w.words[k] == before[k];
                    }
                }
                undone += same;
                others += rest;
            }
        Report("4", "the launch that never found them: the row swept", oldFound == 0 && found == cases && firstSearch == cases && writesOk == cases && undone == cases &&
                                                                         others == cases && most == 1,
               "%d rows (the read slot 0x300 to 0x880 above and below the three copies): without the sweep found in %d of %d searches; with it the read slot proved in "
               "%d, by the first place swept in %d (0x%lX to 0x%lX from its copy); writes on the look's slots only %d, every nudge undone %d, every other word "
               "untouched %d",
               cases, oldFound, cases * 5, found, firstSearch, static_cast<unsigned long>(std::labs(nearest)), static_cast<unsigned long>(std::labs(farthest)), writesOk, undone, others);
    }
    // 5
    {
        std::vector<uintptr_t> reached;
        World w = Launch(kRow + 0x80 + 0x300, &reached);
        reached.push_back(w.readSlot);  // the fourth slot-shaped candidate, the one that proves
        std::unordered_set<uintptr_t> tried;
        const Outcome o = Search(w, reached, tried, true);
        const bool pass = o.found == w.readSlot && o.firstPass && !o.swept && o.shaped == 4 && w.reads == 0 && o.taken == 0 && tried.empty() && WritesOnSlotsOnly(w);
        Report("5", "the launch that worked: the first pass, the row never swept", pass,
               "found by the first pass %d among %d slot-shaped candidates, swept %d, the sweep's reads %d, places taken %d", o.firstPass, o.shaped, o.swept, w.reads, o.taken);
    }
    // 6
    {
        std::vector<uintptr_t> reached;
        World w = Launch(kRow + 0x80 + 0x600, &reached);
        for (int n = 0; n < 5; ++n) {  // five more copies the walk didn't reach: 0xC0, 0x140, 0x1C0 above the last reached and 0x80, 0xC0 below the first
            const uintptr_t at = n < 3 ? kRow + 0x80 + 0xC0 + n * 0x80 : kRow - 0x40 * (n - 2) - 0x40;
            w.MakeSlot(at, kYaw, kPitch);
            w.copies.push_back(at);
        }
        w.Tick();
        std::unordered_set<uintptr_t> tried;
        const Outcome first = Search(w, reached, tried, true);
        int writtenOver = 0;
        for (int n = 0; n < first.proved; ++n)
            writtenOver += first.answer[n] == Proof::kOverwritten;
        const Outcome second = Search(w, reached, tried, true);
        const bool pass = !first.found && first.taken == 6 && first.proved == 4 && writtenOver == 4 && tried.size() >= 4 && second.found == w.readSlot && !second.firstPass &&
                          second.before == 4 && second.taken == 2 && second.proved == 2 && second.answer[0] == Proof::kOverwritten && second.answer[1] == Proof::kYes &&
                          WritesOnSlotsOnly(w);
        Report("6", "more copies unreached: four a search, the next goes on", pass,
               "the first search took %d places, proved %d (%d written over), found %d; the second left out %d tried before, proved %d, found the read slot %d", first.taken,
               first.proved, writtenOver, first.found != 0, second.before, second.proved, second.found == w.readSlot);
    }
    // 7
    {
        std::vector<uintptr_t> reached;
        World w = Launch(kRow + 0x80 + 0x940, &reached);
        const uintptr_t copy = kRow + 0x80 + 0x200;  // one more copy within reach, to be tried
        w.MakeSlot(copy, kYaw, kPitch);
        w.copies.push_back(copy);
        w.Tick();
        std::unordered_set<uintptr_t> tried;
        int found = 0, most = 0, proved[4] = {};
        for (int s = 0; s < 4; ++s) {
            const size_t writes = w.written.size();
            const Outcome o = Search(w, reached, tried, true);
            found += o.found != 0;
            proved[s] = o.proved;
            // (Two words a nudge, two its undoing where it's still there: the three candidates' and the swept places'.)
            most = std::max(most, static_cast<int>((w.written.size() - writes) / 2) - 3);
        }
        const bool pass = !found && proved[0] == 1 && proved[1] == 0 && proved[2] == 1 && proved[3] == 0 && most <= aimrules::kSweepMost && WritesOnSlotsOnly(w);
        Report("7", "the read slot out of reach: never found, bounded, tried again", pass,
               "found in %d of 4 searches; places proved a search %d %d %d %d; at most %d nudged by the sweep in one; writes on the look's slots only %d", found, proved[0],
               proved[1], proved[2], proved[3], most, WritesOnSlotsOnly(w));
    }
    // 8
    {
        World w;
        const uintptr_t copies[3] = {kRow, kRow + 0x40, kRow + 0x80};
        for (const uintptr_t at : copies)
            w.MakeSlot(at, kYaw, kPitch);
        const uintptr_t above[4] = {kRow + 0xC0, kRow + 0x200, kRow + 0x400, kRow + 0x80 + 0x900}, below[3] = {kRow - 0x40, kRow - 0x300, kRow - 0x900};
        for (const uintptr_t at : above)
            w.MakeSlot(at, kYaw, kPitch);
        for (const uintptr_t at : below)
            w.MakeSlot(at, kYaw, kPitch);
        w.MakeSlot(kRow + 0x80 + 0x940, kYaw, kPitch);  // past the reach
        w.MakeSlot(kRow + 0x220, kYaw, kPitch);          // off the row's step: never looked at
        const auto read = [&](uintptr_t at, uint32_t* words) { return w.Read(at - 0xC, words, 28); };
        aimrules::Swept out[8] = {};
        int skipped = -1;
        const int all = aimrules::SweepChoice(copies, 3, kYaw, kPitch, read, [](uintptr_t) { return false; }, out, 8, &skipped);
        const bool order = all == 7 && out[0].at == kRow + 0xC0 && out[0].copy == kRow + 0x80 && out[1].at == kRow - 0x40 && out[1].copy == kRow && out[2].at == kRow + 0x200 &&
                           out[3].at == kRow - 0x300 && out[4].at == kRow + 0x400 && out[5].at == kRow + 0x80 + 0x900 && out[5].Distance() == 0x900 &&
                           out[6].at == kRow - 0x900 && out[6].Distance() == -0x900 && skipped == 0;
        aimrules::Swept four[4] = {};
        const int capped = aimrules::SweepChoice(
            copies, 3, kYaw, kPitch, read, [&](uintptr_t at) { return at == kRow + 0xC0 || at == kRow - 0x300; }, four, 4, &skipped);
        const bool cap = capped == 5 && skipped == 2 && four[0].at == kRow - 0x40 && four[1].at == kRow + 0x200 && four[2].at == kRow + 0x400 &&
                         four[3].at == kRow + 0x80 + 0x900;
        Report("8", "SweepChoice: the order, each once, left out, the count", order && cap,
               "seven places nearest first, above before below, each once, +0x900 and -0x900 the farthest %d (%d taken); two left out and counted, four given of five %d (%d "
               "taken, %d left out)",
               order, all, cap, capped, skipped);
    }
    // 9
    {
        World w = Launch(kRow + 0x80 + 0x300, nullptr);
        const auto read = [&](uintptr_t at, uint32_t* words) { return w.Read(at - 0xC, words, 28); };
        const auto none = [](uintptr_t) { return false; };
        aimrules::Swept out[4] = {};
        const uintptr_t copies[3] = {kRow, kRow + 0x40, kRow + 0x80};
        int taken = 0;
        taken += aimrules::SweepChoice(copies, 3, kNaN, kPitch, read, none, out, 4);
        taken += aimrules::SweepChoice(copies, 3, kYaw, kNaN, read, none, out, 4);
        taken += aimrules::SweepChoice(copies, 0, kYaw, kPitch, read, none, out, 4);
        const uintptr_t edges[3] = {0, 0x10040, UINTPTR_MAX - 0x40};
        int edgeReads = 0;
        taken += aimrules::SweepChoice(
            edges, 3, kYaw, kPitch,
            [&](uintptr_t, uint32_t*) {
                ++edgeReads;
                return false;
            },
            none, out, 4);
        taken += aimrules::SweepChoice(copies, 3, kYaw, kPitch, [](uintptr_t, uint32_t*) { return false; }, none, out, 4);
        const bool works = aimrules::SweepChoice(copies, 3, kYaw, kPitch, read, none, out, 4) == 1 && out[0].at == w.readSlot;
        LARGE_INTEGER f, t0, t1;
        QueryPerformanceFrequency(&f);
        double best = 1e9;
        int sink = 0;
        for (int round = 0; round < 5; ++round) {
            QueryPerformanceCounter(&t0);
            for (int k = 0; k < 2000; ++k)
                sink += aimrules::SweepChoice(copies, 3, kYaw, kPitch, read, none, out, 4);
            QueryPerformanceCounter(&t1);
            best = std::min(best, static_cast<double>(t1.QuadPart - t0.QuadPart) / f.QuadPart * 1e6 / 2000);
        }
        Report("9", "broken inputs, the time", taken == 0 && edgeReads == 0 && works && sink == 10000 && best < 200.0,
               "taken from a camera not a number, no copies, copies at memory's ends, memory not readable: %d (reads at the ends %d); the same row read rightly %d; a sweep "
               "of three copies %.1f us",
               taken, edgeReads, works, best);
    }

    std::printf("%d of %d checks failed\n", g_failed, g_counted);
    return g_failed;
}

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
//   7  the read slot out of the sweep's reach (0x940): never found, at most four nudges a search, tried again from the first only after five
//      searches that had nothing left to take (no nudge of the sweep's in those)
//   8  SweepChoice alone: nearest first, above before below, each place once though three copies reach it, a place swept before left out and
//      counted, a candidate of the pass left out and not counted, the count past the four it gives
//   9  broken inputs: a camera not a number, no copies, a copy at the bottom or the top of memory, memory that can't be read: nothing taken; the time
//  10  the hold's time: SweepGoesOn (the first always, the next only under 600 ms); four places that keep their nudge (0.3 s each): three proved in
//      the first search, the fourth left for the next and not counted as tried
//  11  SweepCopies: the real logs' candidates (three slot-shaped written over, a fourth not a slot that kept its nudge): three copies; a slot-shaped
//      one not written over none; the order; the most
//  12  a candidate of the pass on the row (slot-shaped, kept its nudge) and one not a slot, the read slot out of reach: nothing taken, 0 tried
//      before in every search, nothing forgotten for it
//  13  SweepTried: what an answer leaves out (not a proof, not a moving camera), the round again after five searches with nothing to take and not
//      before, a search that took something starts the count again, a new soldier and a find forget
// The reach (src/aimrules.h: ReachKind, ReachScan, ReachNext, ReachReads, ReachSpans, ReachChoice, ReachWanted, as aim.cpp's ReachLook and Search run them),
// on a sparse piece of memory megabytes across with VirtualQuery's regions (Heap), the game and Proves as above:
//  14  ReachKind: 2 the row's shape, 1 a pointer alone, never without a pointer, off by 0.0101, a zero, not a number; ReachScan in 64 KB pieces: every
//      16-byte line about the seams once, none off a line; ReachNext; ReachReads: only committed, private, exactly read-write memory
//  15  the M249's checkpoint of 2026-11-18 22:03 (bf3vr_1118_m249ingame.log): the walk reaching three copies, the look 0x1580 below them: as it was,
//      nothing in 43 searches; with the reach the first search, nothing written but nudges on the look's slots, each undone, nothing but plain memory read
//  16  the user's M4A1 of 2026-11-19 (bf3vr_1119_acog.log): the look 0x1D3880 and 0x340580 from the copies, in one arena and across two: as it was nothing
//      in 20 searches each; with the reach the first search, 64 MB read at most
//  17  a new life whose walk reaches nothing: found about the last life's look (0x276BC0 from it); the launch's first life: no anchor, nothing read
//  18  a look without the row's shape (a pointer, then words not zero), two copies with it off the row's step nearer, strays without a pointer or off a
//      line: the copies first, then the look, in the first search's hold; the strays never nudged
//  19  crowded: 19 with the row's shape at the camera's angles none nudged; 17 with a pointer alone left out, the look with the row's shape proved
//  20  AMD's write-combined memory (0x404), uncached, a guard page, a mapped file, a module, reserved: never read, the look in one never found; past a
//      region that can't be read midway, found; the reach's own allocation never read
//  21  ReachSpans: joined, two arenas (64 MB in all), the most never passed, the second cut about its anchor, memory's ends, nothing for bad anchors
//  22  the time: 32 MB of real memory looked through in 64 KB pieces (the four looks found, under 100 ms)
//  23  ReachChoice alone: the row's shape first, nearest an anchor first, above before below, each once, a candidate uncounted, tried before counted,
//      the count past the four it gives, broken inputs; the tried places forgotten at the fifth search with nothing to take
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
#include <random>
#include <unordered_map>
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
    uint64_t ms = 0;                 // the game's time: 33 a tick
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
            ms += 33;
        }
    }
};

// A sparse piece of 32-bit memory for the reach, megabytes across: its 4 KB pages made as they're written, regions with VirtualQuery's state, type and
// protection (outside them memory is free), and the game on it as World's. Reads of a region the reach mustn't read (write-combined, uncached, a guard
// page, a mapped file, a module) are counted.
struct Heap {
    struct Region {
        uintptr_t from = 0, to = 0;
        uint32_t state = aimrules::kMemCommit, type = aimrules::kMemPrivate, protect = aimrules::kPageReadWrite;
    };
    std::vector<Region> regions;  // disjoint
    std::unordered_map<uintptr_t, std::vector<uint32_t>> pages;
    uintptr_t readSlot = 0;
    std::vector<uintptr_t> copies;
    float cameraYaw = 0.0f, cameraPitch = 0.0f;
    std::vector<uintptr_t> written;
    uint64_t ms = 0;
    size_t forbiddenReads = 0, bytesRead = 0;

    void Add(uintptr_t from, uintptr_t to, uint32_t protect = aimrules::kPageReadWrite, uint32_t type = aimrules::kMemPrivate,
             uint32_t state = aimrules::kMemCommit) {
        regions.push_back({from, to, state, type, protect});
    }
    const Region* At(uintptr_t at) const {
        for (const Region& r : regions)
            if (at >= r.from && at < r.to)
                return &r;
        return nullptr;
    }
    // VirtualQuery: the region about at, or the free gap up to the next region.
    bool Query(uintptr_t at, uintptr_t& base, uintptr_t& size, uint32_t& state, uint32_t& type, uint32_t& protect) const {
        if (const Region* r = At(at)) {
            base = r->from, size = r->to - r->from, state = r->state, type = r->type, protect = r->protect;
            return true;
        }
        uintptr_t next = 0xFFFF0000u;
        for (const Region& r : regions)
            if (r.from > at && r.from < next)
                next = r.from;
        base = at & ~static_cast<uintptr_t>(0xFFF), size = next - base, state = 0x10000 /* MEM_FREE */, type = 0, protect = 0x01 /* PAGE_NOACCESS */;
        return at < 0xFFFF0000u;
    }
    uint32_t& Word(uintptr_t at) {
        std::vector<uint32_t>& page = pages[at & ~static_cast<uintptr_t>(0xFFF)];
        if (page.empty())
            page.assign(1024, 0);
        return page[(at & 0xFFF) / 4];
    }
    uint32_t WordAt(uintptr_t at) const {
        const auto p = pages.find(at & ~static_cast<uintptr_t>(0xFFF));
        return p == pages.end() ? 0 : p->second[(at & 0xFFF) / 4];
    }
    // Every byte in committed memory a read can reach (any protection but no access and a guard page's first touch, as the game's own reads), else
    // false; reads of what the reach mustn't read are counted.
    std::vector<std::pair<uintptr_t, uintptr_t>> poisoned;  // ranges whose reads fail (a region freed while it's read)
    bool Read(uintptr_t at, void* out, size_t size) {
        if ((at & 3) || (size & 3) || !size)
            return false;
        for (const auto& [from, to] : poisoned)
            if (at < to && at + size > from)
                return false;
        // One region at a time, page by page.
        for (uintptr_t a = at; a < at + size;) {
            const Region* r = At(a);
            if (!r || r->state != aimrules::kMemCommit || r->protect == 0x01)
                return false;
            a = r->to;
        }
        for (uintptr_t a = at; a < at + size;) {
            const Region* r = At(a);
            const uintptr_t regionEnd = std::min<uintptr_t>(r->to, at + size);
            const bool forbidden = !aimrules::ReachReads(r->state, r->type, r->protect);
            while (a < regionEnd) {
                const uintptr_t pageEnd = std::min<uintptr_t>((a & ~static_cast<uintptr_t>(0xFFF)) + 0x1000, regionEnd);
                const auto p = pages.find(a & ~static_cast<uintptr_t>(0xFFF));
                uint8_t* to = static_cast<uint8_t*>(out) + (a - at);
                if (p == pages.end())
                    std::memset(to, 0, pageEnd - a);
                else
                    std::memcpy(to, &p->second[(a & 0xFFF) / 4], pageEnd - a);
                if (forbidden)
                    forbiddenReads += (pageEnd - a) / 4;
                a = pageEnd;
            }
        }
        bytesRead += size;
        return true;
    }
    float Float(uintptr_t at) const {
        float v = 0.0f;
        const uint32_t w = WordAt(at);
        std::memcpy(&v, &w, 4);
        return v;
    }
    void Set(uintptr_t at, float v) { std::memcpy(&Word(at), &v, 4); }
    void SetWord(uintptr_t at, uint32_t v) { Word(at) = v; }
    void MakeSlot(uintptr_t at, float yaw, float pitch, uint32_t pointer = 0x30001000) {
        SetWord(at - 0xC, pointer);
        SetWord(at - 8, 0), SetWord(at - 4, 0), SetWord(at + 4, 0), SetWord(at + 8, 0);
        Set(at, yaw), Set(at + 0xC, pitch);
    }
    bool Swap(uintptr_t at, float expected, float value) {
        uint32_t was = 0, now = 0;
        std::memcpy(&was, &expected, 4);
        std::memcpy(&now, &value, 4);
        const Region* r = At(at);
        if (!r || r->state != aimrules::kMemCommit || WordAt(at) != was)
            return false;
        Word(at) = now;
        written.push_back(at);
        return true;
    }
    void Tick(int ticks = 1) {
        for (int t = 0; t < ticks; ++t) {
            const float yaw = Float(readSlot), pitch = Float(readSlot + 0xC);
            for (const uintptr_t copy : copies)
                Set(copy, yaw), Set(copy + 0xC, pitch);
            cameraYaw = yaw, cameraPitch = pitch;
            ms += 33;
        }
    }
};

enum class Proof { kNo, kKept, kYes, kBusy, kOverwritten };

template <class W>
void Undo(W& w, uintptr_t at, float nudged, float nudge) {
    const float now = w.Float(at);
    if (std::fabs(now - nudged) < kNudge * 0.3f)
        w.Swap(at, now, now - nudge);
}

// aim.cpp's Proves(at, false, true), its sleeps as the game's ticks (33 ms each).
template <class W>
Proof Proves(W& w, uintptr_t at) {
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

// aim.cpp's AnswerOf.
aimrules::Answer AnswerOf(Proof answer) {
    switch (answer) {
    case Proof::kYes:
        return aimrules::Answer::kYes;
    case Proof::kKept:
        return aimrules::Answer::kKept;
    case Proof::kBusy:
        return aimrules::Answer::kBusy;
    case Proof::kOverwritten:
        return aimrules::Answer::kOverwritten;
    default:
        return aimrules::Answer::kNo;
    }
}

struct Outcome {
    uintptr_t found = 0;
    bool firstPass = false;  // found by the first pass
    bool swept = false;      // the row was looked through
    int candidates = 0, shaped = 0, writtenOver = 0;
    int taken = 0, proved = 0, before = 0;
    bool again = false;  // the round started again: the places tried forgotten
    aimrules::Swept at[aimrules::kSweepMost] = {};
    Proof answer[aimrules::kSweepMost] = {};
};

// aim.cpp's Search with find from its first pass on: reached is what the walk and Matching hand over (the pairs near the soldier), the slot-shaped
// first; then the sweep (sweep 0: as it was before it). tried: the soldier's places swept before (aim.cpp's g_swept).
Outcome Search(World& w, const std::vector<uintptr_t>& reached, aimrules::SweepTried& tried, bool sweep, uintptr_t soldier = 0x1000) {
    Outcome o;
    tried.For(soldier);
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
    std::vector<aimrules::Candidate> answered;
    for (size_t k = 0; k < pairs.size() && !crowded && !o.found; ++k) {
        const Proof answer = Proves(w, pairs[k]);
        if (answer == Proof::kYes)
            o.found = pairs[k], o.firstPass = true;
        answered.push_back({pairs[k], k < static_cast<size_t>(o.shaped), answer == Proof::kOverwritten});
    }
    o.writtenOver = aimrules::SweepCopies(answered.data(), static_cast<int>(answered.size()), copies, static_cast<int>(kMostPairs));
    if (sweep && aimrules::SweepWanted(true, o.found != 0, crowded, o.writtenOver) && std::fabs(w.cameraPitch) > 0.02f && std::fabs(w.cameraYaw) > 0.02f) {
        o.swept = true;
        o.taken = aimrules::SweepChoice(
            copies, o.writtenOver, w.cameraYaw, w.cameraPitch,
            [&](uintptr_t at, uint32_t* words) {
                ++w.reads;
                return w.Read(at - 0xC, words, 7 * sizeof(uint32_t));
            },
            [&](uintptr_t at) { return std::find(pairs.begin(), pairs.end(), at) != pairs.end(); }, [&](uintptr_t at) { return tried.Has(at); }, o.at,
            aimrules::kSweepMost, &o.before);
        o.again = tried.Chosen(o.taken, o.before);
        const uint64_t sweepStart = w.ms;
        for (int n = 0; n < o.taken && n < aimrules::kSweepMost && !o.found && aimrules::SweepGoesOn(w.ms - sweepStart, n); ++n) {
            o.answer[n] = Proves(w, o.at[n].at);
            o.proved = n + 1;
            tried.Answered(o.at[n].at, AnswerOf(o.answer[n]));
            if (o.answer[n] == Proof::kYes)
                o.found = o.at[n].at;
        }
    }
    if (o.found)
        tried.Found();
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

// ---- The reach on a Heap (aim.cpp's ReachLook and Search's reach, written out again around the same aimrules calls) ----

struct ReachRun {
    std::vector<aimrules::Reached> places;
    int spans = 0;
    size_t bytes = 0;
};
// aim.cpp's ReachLook: each span's regions as Query gives them, only aimrules::ReachReads ones, never the own allocation (ownFrom..ownTo: its piece,
// its stack), in pieces of 64 KB and six words, each line once.
ReachRun ReachLook(Heap& h, const uintptr_t* anchors, int count, float cameraYaw, float cameraPitch, uintptr_t ownFrom = 0, uintptr_t ownTo = 0) {
    ReachRun run;
    constexpr int kSpansMost = 32;
    aimrules::Span spans[kSpansMost];
    run.spans = aimrules::ReachSpans(anchors, count, spans, kSpansMost, aimrules::kReachBytesMost);
    constexpr size_t kPieceWords = 0x4000 + 6;
    std::vector<uint32_t> piece(kPieceWords);
    for (int s = 0; s < run.spans; ++s)
        for (uintptr_t at = spans[s].from; at < spans[s].to;) {
            uintptr_t base = 0, size = 0;
            uint32_t state = 0, type = 0, protect = 0;
            if (!h.Query(at, base, size, state, type, protect))
                break;
            const uintptr_t regionEnd = base + size, end = std::min<uintptr_t>(regionEnd, spans[s].to);
            const bool own = ownTo && base < ownTo && regionEnd > ownFrom;
            if (!own && aimrules::ReachReads(state, type, protect))
                for (uintptr_t from = at; from < end && end - from >= 28;) {
                    const size_t words = std::min<size_t>(kPieceWords, (end - from) / 4);
                    if (!h.Read(from, piece.data(), words * 4))
                        break;
                    run.bytes += words * 4;
                    aimrules::ReachScan(piece.data(), words, from, cameraYaw, cameraPitch, [&](uintptr_t place, int kind) {
                        if (run.places.size() < 256)
                            run.places.push_back({place, kind, 0});
                    });
                    const uintptr_t next = aimrules::ReachNext(from, words);
                    if (next <= from)
                        break;
                    from = next;
                }
            if (regionEnd <= at)
                break;
            at = regionEnd;
        }
    return run;
}

struct ReachOutcome {
    uintptr_t found = 0;
    bool firstPass = false, bySweep = false, byReach = false, swept = false, reached = false;
    int candidates = 0, shaped = 0;
    int sweptTaken = 0, sweptProved = 0;
    int anchors = 0, taken = 0, proved = 0, before = 0, crowd = 0, kind2 = 0, kind1 = 0, spans = 0;
    size_t bytes = 0;
    aimrules::Reached at[aimrules::kSweepMost] = {};
    Proof answer[aimrules::kSweepMost] = {};
};
// The launch's looks proved, newest first (aim.cpp's g_provenEver).
struct Ever {
    uintptr_t at[8] = {};
    void Note(uintptr_t a) {
        int from = 7;
        for (int k = 0; k < 8; ++k)
            if (at[k] == a) {
                from = k;
                break;
            }
        for (int k = from; k > 0; --k)
            at[k] = at[k - 1];
        at[0] = a;
    }
};
// aim.cpp's Search with find from its first pass on, on a Heap: reached is what the walk hands over; the sweep; then the reach (reach 0: as it was before
// it), its anchors this search's candidates and swept places written over and the launch's looks proved (ever).
ReachOutcome ReachSearch(Heap& h, const std::vector<uintptr_t>& reached, aimrules::SweepTried& swept, aimrules::SweepTried& reachTried, Ever& ever, bool reach,
                         uintptr_t soldier = 0x1000, uintptr_t ownFrom = 0, uintptr_t ownTo = 0) {
    ReachOutcome o;
    swept.For(soldier);
    reachTried.For(soldier);
    h.Tick(2);  // (the hold: the camera settled)
    if (!(std::fabs(h.cameraPitch) > 0.02f && std::fabs(h.cameraYaw) > 0.02f))
        return o;
    std::vector<uintptr_t> pairs, rest;
    for (const uintptr_t at : reached) {
        uint32_t words[7] = {};
        float pair[4] = {}, y = 0.0f, p = 0.0f;
        if (!h.Read(at, pair, sizeof(pair)) || !aimrules::StrictPair(pair[0], pair[3], h.cameraYaw, h.cameraPitch, false))
            continue;
        (h.Read(at - 0xC, words, sizeof(words)) && aimrules::SlotWords(words, y, p) ? pairs : rest).push_back(at);
    }
    o.shaped = static_cast<int>(pairs.size());
    pairs.insert(pairs.end(), rest.begin(), rest.end());
    o.candidates = static_cast<int>(pairs.size());
    const bool crowded = pairs.size() > kMostPairs;
    std::vector<aimrules::Candidate> answered;
    std::vector<Proof> answers;
    for (size_t k = 0; k < pairs.size() && !crowded && !o.found; ++k) {
        const Proof answer = Proves(h, pairs[k]);
        if (answer == Proof::kYes)
            o.found = pairs[k], o.firstPass = true;
        answered.push_back({pairs[k], k < static_cast<size_t>(o.shaped), answer == Proof::kOverwritten});
        answers.push_back(answer);
    }
    uintptr_t copies[kMostPairs] = {};
    const int copyCount = aimrules::SweepCopies(answered.data(), static_cast<int>(answered.size()), copies, static_cast<int>(kMostPairs));
    aimrules::Swept sweptAt[aimrules::kSweepMost] = {};
    Proof sweptAnswer[aimrules::kSweepMost] = {};
    uint64_t extraStart = 0;
    bool extra = false;
    if (aimrules::SweepWanted(true, o.found != 0, crowded, copyCount)) {
        o.swept = true;
        int before = 0;
        o.sweptTaken = aimrules::SweepChoice(
            copies, copyCount, h.cameraYaw, h.cameraPitch, [&](uintptr_t at, uint32_t* words) { return h.Read(at - 0xC, words, 7 * sizeof(uint32_t)); },
            [&](uintptr_t at) { return std::find(pairs.begin(), pairs.end(), at) != pairs.end(); }, [&](uintptr_t at) { return swept.Has(at); }, sweptAt,
            aimrules::kSweepMost, &before);
        swept.Chosen(o.sweptTaken, before);
        const uint64_t sweepStart = h.ms;
        for (int n = 0; n < o.sweptTaken && n < aimrules::kSweepMost && !o.found && aimrules::SweepGoesOn(h.ms - sweepStart, n); ++n) {
            if (!n)
                extraStart = sweepStart, extra = true;
            sweptAnswer[n] = Proves(h, sweptAt[n].at);
            o.sweptProved = n + 1;
            swept.Answered(sweptAt[n].at, AnswerOf(sweptAnswer[n]));
            if (sweptAnswer[n] == Proof::kYes)
                o.found = sweptAt[n].at, o.bySweep = true;
        }
    }
    if (reach && !o.found && !crowded) {
        uintptr_t anchors[aimrules::kReachAnchors] = {};
        const auto anchor = [&](uintptr_t at) {
            if (at && o.anchors < aimrules::kReachAnchors && std::find(anchors, anchors + o.anchors, at) == anchors + o.anchors)
                anchors[o.anchors++] = at;
        };
        for (size_t k = 0; k < answers.size(); ++k)
            if (answers[k] == Proof::kOverwritten)
                anchor(pairs[k]);
        for (int n = 0; n < o.sweptProved; ++n)
            if (sweptAnswer[n] == Proof::kOverwritten)
                anchor(sweptAt[n].at);
        for (const uintptr_t at : ever.at)
            anchor(at);
        if (aimrules::ReachWanted(true, false, crowded, o.anchors) && std::fabs(h.cameraPitch) > 0.02f && std::fabs(h.cameraYaw) > 0.02f) {
            o.reached = true;
            const ReachRun run = ReachLook(h, anchors, o.anchors, h.cameraYaw, h.cameraPitch, ownFrom, ownTo);
            o.spans = run.spans, o.bytes = run.bytes;
            for (const aimrules::Reached& r : run.places)
                ++(r.kind == 2 ? o.kind2 : o.kind1);
            const auto sweptHere = [&](uintptr_t at) {
                for (int n = 0; n < o.sweptProved; ++n)
                    if (sweptAt[n].at == at)
                        return true;
                return false;
            };
            o.taken = aimrules::ReachChoice(
                run.places.data(), static_cast<int>(run.places.size()), anchors, o.anchors,
                [&](uintptr_t at) { return std::find(pairs.begin(), pairs.end(), at) != pairs.end() || sweptHere(at); },
                [&](uintptr_t at) { return reachTried.Has(at) || swept.Has(at); }, o.at, aimrules::kSweepMost, &o.before, &o.crowd);
            reachTried.Chosen(o.taken, o.before);
            const uint64_t reachStart = extra ? extraStart : h.ms;
            for (int n = 0; n < o.taken && n < aimrules::kSweepMost && !o.found && aimrules::SweepGoesOn(h.ms - reachStart, o.sweptProved + n); ++n) {
                o.answer[n] = Proves(h, o.at[n].at);
                o.proved = n + 1;
                reachTried.Answered(o.at[n].at, AnswerOf(o.answer[n]));
                if (o.answer[n] == Proof::kYes)
                    o.found = o.at[n].at, o.byReach = true;
            }
        }
    }
    if (o.found) {
        swept.Found();
        reachTried.Found();
        ever.Note(o.found);
    }
    return o;
}

// Every word the mod wrote on a Heap lies on the read slot's or a copy's yaw or pitch.
bool WritesOnSlotsOnly(const Heap& h) {
    for (const uintptr_t at : h.written) {
        bool on = at == h.readSlot || at == h.readSlot + 0xC;
        for (const uintptr_t copy : h.copies)
            on = on || at == copy || at == copy + 0xC;
        if (!on)
            return false;
    }
    return true;
}
// Every word as it was (the angles to a float's last bits: a nudge added and taken off), against a copy of the pages before.
bool AllAsBefore(const Heap& h, const std::unordered_map<uintptr_t, std::vector<uint32_t>>& before) {
    for (const auto& [page, words] : h.pages) {
        const auto was = before.find(page);
        for (size_t k = 0; k < words.size(); ++k) {
            const uint32_t old = was == before.end() ? 0u : was->second[k];
            if (words[k] == old)
                continue;
            float a = 0.0f, b = 0.0f;
            std::memcpy(&a, &words[k], 4), std::memcpy(&b, &old, 4);
            if (!(std::fabs(a - b) < 1e-6f))
                return false;
        }
    }
    return true;
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
                    aimrules::SweepTried tried;
                    for (int s = 0; s < 5; ++s)
                        oldFound += Search(w, reached, tried, false).found != 0;
                }
                World w = Launch(readAt, &reached);
                const std::vector<uint32_t> before = w.words;
                aimrules::SweepTried tried;
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
        aimrules::SweepTried tried;
        const Outcome o = Search(w, reached, tried, true);
        const bool pass = o.found == w.readSlot && o.firstPass && !o.swept && o.shaped == 4 && w.reads == 0 && o.taken == 0 && tried.places.empty() && WritesOnSlotsOnly(w);
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
        aimrules::SweepTried tried;
        const Outcome first = Search(w, reached, tried, true);
        int writtenOver = 0;
        for (int n = 0; n < first.proved; ++n)
            writtenOver += first.answer[n] == Proof::kOverwritten;
        const size_t leftOut = tried.places.size();
        const Outcome second = Search(w, reached, tried, true);
        const bool pass = !first.found && first.taken == 6 && first.proved == 4 && writtenOver == 4 && leftOut == 4 && tried.places.empty() && second.found == w.readSlot && !second.firstPass &&
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
        aimrules::SweepTried tried;
        constexpr int kSearches = 14;
        int found = 0, most = 0, proved[kSearches] = {}, again[kSearches] = {}, resting = 0;
        for (int s = 0; s < kSearches; ++s) {
            const size_t writes = w.written.size();
            const Outcome o = Search(w, reached, tried, true);
            found += o.found != 0;
            proved[s] = o.proved, again[s] = o.again;
            // (Two words a nudge, two its undoing where it's still there: the three candidates' and the swept places'.)
            const int nudged = static_cast<int>((w.written.size() - writes) / 2) - 3;
            most = std::max(most, nudged);
            resting += !o.proved && nudged > 0;  // a search with nothing to take that nudged for the sweep all the same
        }
        // One place proved, then five searches with nothing to take (the fifth forgets the places), then the round again: searches 0, 6 and 12.
        bool rhythm = true;
        char said[64] = {};
        for (int s = 0; s < kSearches; ++s) {
            rhythm = rhythm && proved[s] == (s % 6 == 0 ? 1 : 0) && again[s] == (s % 6 == 5 ? 1 : 0);
            said[s] = static_cast<char>('0' + proved[s]);
        }
        const bool pass = !found && rhythm && resting == 0 && most <= aimrules::kSweepMost && WritesOnSlotsOnly(w);
        Report("7", "the read slot out of reach: never found, bounded, the round again after five searches", pass,
               "found in %d of %d searches; places proved a search %s (one in six: %d); the sweep's nudges in the searches between %d; at most %d nudged by the sweep in "
               "one; writes on the look's slots only %d",
               found, kSearches, said, rhythm, resting, most, WritesOnSlotsOnly(w));
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
        const auto none = [](uintptr_t) { return false; };
        const int all = aimrules::SweepChoice(copies, 3, kYaw, kPitch, read, none, none, out, 8, &skipped);
        const bool order = all == 7 && out[0].at == kRow + 0xC0 && out[0].copy == kRow + 0x80 && out[1].at == kRow - 0x40 && out[1].copy == kRow && out[2].at == kRow + 0x200 &&
                           out[3].at == kRow - 0x300 && out[4].at == kRow + 0x400 && out[5].at == kRow + 0x80 + 0x900 && out[5].Distance() == 0x900 &&
                           out[6].at == kRow - 0x900 && out[6].Distance() == -0x900 && skipped == 0;
        aimrules::Swept four[4] = {};
        const int capped = aimrules::SweepChoice(
            copies, 3, kYaw, kPitch, read, none, [&](uintptr_t at) { return at == kRow + 0xC0 || at == kRow - 0x300; }, four, 4, &skipped);
        const bool cap = capped == 5 && skipped == 2 && four[0].at == kRow - 0x40 && four[1].at == kRow + 0x200 && four[2].at == kRow + 0x400 &&
                         four[3].at == kRow + 0x80 + 0x900;
        // The same two as candidates of the pass: left out, not counted; with one more swept before, that one alone counted.
        aimrules::Swept rest[8] = {};
        int counted = -1;
        const int asCandidates = aimrules::SweepChoice(
            copies, 3, kYaw, kPitch, read, [&](uintptr_t at) { return at == kRow + 0xC0 || at == kRow - 0x300; }, [&](uintptr_t at) { return at == kRow + 0x400; }, rest, 8,
            &counted);
        const bool candidates = asCandidates == 4 && counted == 1 && rest[0].at == kRow - 0x40 && rest[1].at == kRow + 0x200 && rest[2].at == kRow + 0x80 + 0x900 &&
                                rest[3].at == kRow - 0x900;
        Report("8", "SweepChoice: the order, each once, left out, the count", order && cap && candidates,
               "seven places nearest first, above before below, each once, +0x900 and -0x900 the farthest %d (%d taken); two left out and counted, four given of five %d (%d "
               "taken, %d left out); the two as the pass's candidates left out and not counted, one swept before counted %d (%d taken, %d counted)",
               order, all, cap, capped, skipped, candidates, asCandidates, counted);
    }
    // 9
    {
        World w = Launch(kRow + 0x80 + 0x300, nullptr);
        const auto read = [&](uintptr_t at, uint32_t* words) { return w.Read(at - 0xC, words, 28); };
        const auto none = [](uintptr_t) { return false; };
        aimrules::Swept out[4] = {};
        const uintptr_t copies[3] = {kRow, kRow + 0x40, kRow + 0x80};
        int taken = 0;
        taken += aimrules::SweepChoice(copies, 3, kNaN, kPitch, read, none, none, out, 4);
        taken += aimrules::SweepChoice(copies, 3, kYaw, kNaN, read, none, none, out, 4);
        taken += aimrules::SweepChoice(copies, 0, kYaw, kPitch, read, none, none, out, 4);
        const uintptr_t edges[3] = {0, 0x10040, UINTPTR_MAX - 0x40};
        int edgeReads = 0;
        taken += aimrules::SweepChoice(
            edges, 3, kYaw, kPitch,
            [&](uintptr_t, uint32_t*) {
                ++edgeReads;
                return false;
            },
            none, none, out, 4);
        taken += aimrules::SweepChoice(copies, 3, kYaw, kPitch, [](uintptr_t, uint32_t*) { return false; }, none, none, out, 4);
        const bool works = aimrules::SweepChoice(copies, 3, kYaw, kPitch, read, none, none, out, 4) == 1 && out[0].at == w.readSlot;
        LARGE_INTEGER f, t0, t1;
        QueryPerformanceFrequency(&f);
        double best = 1e9;
        int sink = 0;
        for (int round = 0; round < 5; ++round) {
            QueryPerformanceCounter(&t0);
            for (int k = 0; k < 2000; ++k)
                sink += aimrules::SweepChoice(copies, 3, kYaw, kPitch, read, none, none, out, 4);
            QueryPerformanceCounter(&t1);
            best = std::min(best, static_cast<double>(t1.QuadPart - t0.QuadPart) / f.QuadPart * 1e6 / 2000);
        }
        Report("9", "broken inputs, the time", taken == 0 && edgeReads == 0 && works && sink == 10000 && best < 200.0,
               "taken from a camera not a number, no copies, copies at memory's ends, memory not readable: %d (reads at the ends %d); the same row read rightly %d; a sweep "
               "of three copies %.1f us",
               taken, edgeReads, works, best);
    }
    // 10
    {
        const bool rule = aimrules::SweepGoesOn(0, 0) && aimrules::SweepGoesOn(100000, 0) && aimrules::SweepGoesOn(599, 1) && !aimrules::SweepGoesOn(600, 1) &&
                          !aimrules::SweepGoesOn(601, 3);
        std::vector<uintptr_t> reached;
        World w = Launch(kRow + 0x80 + 0x940, &reached);  // the read slot out of reach
        // Four places of the row's shape at the camera's angles that nothing writes over and nothing reads: each keeps its nudge, the camera not
        // following (9 ticks, 297 ms).
        const uintptr_t still[4] = {kRow + 0x80 + 0xC0, kRow - 0x80, kRow + 0x80 + 0x180, kRow - 0x180};
        for (const uintptr_t at : still)
            w.MakeSlot(at, kYaw, kPitch);
        aimrules::SweepTried tried;
        const uint64_t t0 = w.ms;
        const Outcome first = Search(w, reached, tried, true);
        const uint64_t firstMs = w.ms - t0;
        int kept = 0;
        for (int n = 0; n < first.proved; ++n)
            kept += first.answer[n] == Proof::kKept;
        const bool fourthLeft = !tried.Has(first.at[3].at) && tried.places.size() == 3;
        const Outcome second = Search(w, reached, tried, true);
        const Outcome third = Search(w, reached, tried, true);
        // (Undone: the four as they were.)
        bool undone = true;
        for (const uintptr_t at : still)
            undone = undone && std::fabs(w.Float(at) - kYaw) < 1e-6f && std::fabs(w.Float(at + 0xC) - kPitch) < 1e-6f;
        const bool pass = rule && !first.found && first.taken == 4 && first.proved == 3 && kept == 3 && fourthLeft && second.before == 3 && second.taken == 1 &&
                          second.proved == 1 && second.at[0].at == first.at[3].at && third.taken == 0 && third.before == 4 && third.proved == 0 && !third.again && undone;
        Report("10", "the hold's time: no proof begun past 600 ms, the rest in the next search", pass,
               "the rule %d; four places that keep the nudge: the first search took %d, proved %d (%d kept) in %llu ms with the first pass, the fourth left and not counted "
               "as tried %d; the second left out %d, proved %d (the fourth); the third nothing (%d tried before); every nudge undone %d",
               rule, first.taken, first.proved, kept, static_cast<unsigned long long>(firstMs), fourthLeft, second.before, second.proved, third.before, undone);
    }
    // 11
    {
        // (2026-11-18's log: three slot-shaped written over, the fourth not a slot and keeping its nudge.)
        const aimrules::Candidate log[4] = {{0x21000040, true, true}, {0x21000080, true, true}, {0x210000C0, true, true}, {0x21004000, false, false}};
        uintptr_t out[16] = {};
        const int three = aimrules::SweepCopies(log, 4, out, 16);
        const bool real = three == 3 && out[0] == 0x21000040 && out[1] == 0x21000080 && out[2] == 0x210000C0 && out[3] == 0;
        const aimrules::Candidate mixed[5] = {{0x100, true, false}, {0x140, false, true}, {0x180, true, true}, {0x1C0, false, false}, {0x200, true, true}};
        uintptr_t two[16] = {};
        const bool only = aimrules::SweepCopies(mixed, 5, two, 16) == 2 && two[0] == 0x180 && two[1] == 0x200;
        uintptr_t one[2] = {0, 0x5555};
        const bool most = aimrules::SweepCopies(log, 4, one, 1) == 1 && one[0] == 0x21000040 && one[1] == 0x5555;
        const bool none = aimrules::SweepCopies(log, 0, out, 16) == 0 && aimrules::SweepCopies(&log[3], 1, out, 16) == 0 && aimrules::SweepCopies(log, 4, out, 0) == 0;
        Report("11", "SweepCopies: the slot-shaped written over, nothing else", real && only && most && none,
               "the real logs' four candidates give three copies %d; not a slot or not written over is none, in the pass's order %d; the most kept %d; none from none %d", real,
               only, most, none);
    }
    // 12
    {
        std::vector<uintptr_t> reached;
        World w = Launch(kRow + 0x80 + 0x940, &reached);  // the read slot out of reach
        const uintptr_t fourth = kRow + 0x80 + 0x140;      // on the row, the slot's shape, at the camera's angles, nothing writing over it: keeps its nudge
        w.MakeSlot(fourth, kYaw, kPitch);
        reached.push_back(fourth);
        reached.push_back(kRow - 0x200);  // the stray pair at the camera's angles: not a slot
        aimrules::SweepTried tried;
        int swept = 0, taken = 0, before = 0, again = 0, shape = 0;
        for (int s = 0; s < 8; ++s) {
            const Outcome o = Search(w, reached, tried, true);
            swept += o.swept, taken += o.taken, before += o.before, again += o.again;
            shape += o.candidates == 5 && o.shaped == 4 && o.writtenOver == 3 && !o.found;
        }
        const bool pass = swept == 8 && shape == 8 && taken == 0 && before == 0 && again == 0 && tried.places.empty() && tried.rested == 0 && WritesOnSlotsOnly(w) == false;
        // (The stray pair is a candidate of the first pass, as in the game: nudged and undone there, never by the sweep.)
        Report("12", "a candidate of the pass on the row: not taken, not counted as tried before", pass,
               "eight searches, five candidates (four slot-shaped, three written over) in %d, swept in %d: taken %d, tried before %d, rounds started again %d, places "
               "remembered %zu",
               shape, swept, taken, before, again, tried.places.size());
    }
    // 13
    {
        aimrules::SweepTried t;
        t.For(0x1000);
        t.Answered(0x40, aimrules::Answer::kOverwritten), t.Answered(0x80, aimrules::Answer::kKept), t.Answered(0xC0, aimrules::Answer::kNo);
        t.Answered(0x100, aimrules::Answer::kBusy), t.Answered(0x140, aimrules::Answer::kYes), t.Answered(0x180, aimrules::Answer::kNone);
        const bool answers = t.Has(0x40) && t.Has(0x80) && t.Has(0xC0) && !t.Has(0x100) && !t.Has(0x140) && !t.Has(0x180) && t.places.size() == 3;
        // Nothing to take, three tried before: four searches keep them, the fifth forgets.
        int early = 0;
        for (int s = 0; s < aimrules::kSweepRest - 1; ++s)
            early += t.Chosen(0, 3) || t.places.size() != 3;
        const bool fifth = t.Chosen(0, 3) && t.places.empty() && t.rested == 0;
        // Nothing taken and nothing tried before (the slot out of reach), or something taken: never forgets, and the count starts again.
        t.Answered(0x40, aimrules::Answer::kOverwritten);
        int wrong = 0;
        for (int s = 0; s < 3; ++s)
            wrong += t.Chosen(0, 1);
        wrong += t.Chosen(2, 1) || t.rested != 0;
        for (int s = 0; s < aimrules::kSweepRest - 1; ++s)
            wrong += t.Chosen(0, 1);
        wrong += t.Chosen(0, 0) || t.rested != 0;
        for (int s = 0; s < 20; ++s)
            wrong += t.Chosen(0, 0) || t.Chosen(1, 0);
        const bool keeps = !wrong && t.Has(0x40);
        // A new soldier forgets (the places and the count); the same one doesn't; a find forgets, the soldier kept.
        t.Chosen(0, 1), t.Chosen(0, 1);
        t.For(0x1000);
        const bool same = t.Has(0x40) && t.rested == 2;
        t.For(0x2000);
        const bool soldier = t.places.empty() && t.rested == 0 && t.soldier == 0x2000;
        t.Answered(0x40, aimrules::Answer::kKept), t.Chosen(0, 1);
        t.Found();
        const bool find = t.places.empty() && t.rested == 0 && t.soldier == 0x2000;
        Report("13", "SweepTried: the answers, the round again, a new soldier, a find", answers && !early && fifth && keeps && same && soldier && find,
               "left out after written over, kept, refused and not after a proof, a moving camera, no try %d; kept through %d searches with nothing to take (%d wrong), "
               "forgotten at the fifth %d; never forgotten with something taken or nothing tried before %d; the same soldier keeps %d, a new one forgets %d, a find forgets %d",
               answers, aimrules::kSweepRest - 1, early, fifth, keeps, same, soldier, find);
    }
    // 14
    {
        const auto kindOf = [](uint32_t head, uint32_t between, float yaw, float pitch, float cy, float cp) {
            uint32_t w[7] = {head, between, between, 0, between, between, 0};
            std::memcpy(&w[3], &yaw, 4), std::memcpy(&w[6], &pitch, 4);
            return aimrules::ReachKind(w, cy, cp);
        };
        const uint32_t head = 0x30001000;
        const bool two = kindOf(head, 0, kYaw, kPitch, kYaw, kPitch) == 2 && kindOf(head, 0, kYaw + 0.0099f, kPitch - 0.0099f, kYaw, kPitch) == 2 &&
                         kindOf(head, 0, 6.2831853f - 0.5f, kPitch, -0.5f, kPitch) == 2;
        const bool one = kindOf(head, 0x3F800000, kYaw, kPitch, kYaw, kPitch) == 1 && kindOf(0x02237778, 0x12345678, kYaw, kPitch, kYaw, kPitch) == 1;
        int refused = 0;
        refused += !kindOf(7, 0, kYaw, kPitch, kYaw, kPitch) + !kindOf(0, 0, kYaw, kPitch, kYaw, kPitch) + !kindOf(0xFFFF0004u, 0, kYaw, kPitch, kYaw, kPitch) +
                   !kindOf(0x30001002, 0, kYaw, kPitch, kYaw, kPitch);
        refused += !kindOf(head, 0, kYaw + 0.0101f, kPitch, kYaw, kPitch) + !kindOf(head, 0, kYaw, kPitch + 0.0101f, kYaw, kPitch);
        refused += !kindOf(head, 0, kYaw, 0.0f, kYaw, 0.0f) + !kindOf(head, 0, kYaw, 0.015f, kYaw, 0.015f) + !kindOf(head, 0, 0.015f, kPitch, 0.015f, kPitch) +
                   !kindOf(head, 0, 6.2831853f, kPitch, 0.0f, kPitch) + !kindOf(head, 0x3F800000, 0.0f, kPitch, 0.0f, kPitch);
        refused += !kindOf(head, 0, kNaN, kPitch, kYaw, kPitch) + !kindOf(head, 0, kYaw, kNaN, kYaw, kPitch) + !kindOf(head, 0, kYaw, kPitch, kNaN, kPitch) +
                   !kindOf(head, 0, kYaw, 2.0f, kYaw, 2.0f);
        // The seams: three pieces' worth of words with a place on the four lines about each seam and the first and last, and two more 4 and 8 bytes off a
        // line; looked through in pieces of 64 KB and six words as aim.cpp's ReachLook does.
        constexpr uintptr_t base = 0x40000000;
        std::vector<uint32_t> mem(3 * 0x4000 + 40, 0);
        const auto put = [&](size_t word, uint32_t first) {
            mem[word] = first;
            std::memcpy(&mem[word + 3], &kYaw, 4), std::memcpy(&mem[word + 6], &kPitch, 4);
        };
        std::vector<uintptr_t> lines;
        for (const size_t seam : {size_t{0x4000}, size_t{0x8000}})
            for (const size_t w : {seam - 8, seam - 4, seam, seam + 4})
                put(w, head), lines.push_back(base + w * 4 + 0xC);
        put(0, head), lines.push_back(base + 0xC);
        put(mem.size() - 8, head), lines.push_back(base + (mem.size() - 8) * 4 + 0xC);
        put(0x2001, head), put(0x6002, head);  // (4 and 8 bytes past a line)
        std::vector<uintptr_t> seen;
        for (uintptr_t from = base; from < base + mem.size() * 4 && base + mem.size() * 4 - from >= 28;) {
            const size_t words = std::min<size_t>(0x4006, (base + mem.size() * 4 - from) / 4);
            aimrules::ReachScan(&mem[(from - base) / 4], words, from, kYaw, kPitch, [&](uintptr_t at, int) { seen.push_back(at); });
            from = aimrules::ReachNext(from, words);
        }
        std::sort(seen.begin(), seen.end());
        std::sort(lines.begin(), lines.end());
        const bool seams = seen == lines;
        const bool next = aimrules::ReachNext(base, 0x4006) == base + 0x10000 && aimrules::ReachNext(base, 7) == base + 16 && aimrules::ReachNext(base, 6) == base &&
                          aimrules::ReachNext(base, 10) == base + 16 && aimrules::ReachNext(base, 11) == base + 32;
        const bool broken = aimrules::ReachScan(nullptr, 100, base, kYaw, kPitch, [](uintptr_t, int) {}) == 0 &&
                            aimrules::ReachScan(mem.data(), 100, base + 4, kYaw, kPitch, [](uintptr_t, int) {}) == 0 &&
                            aimrules::ReachScan(mem.data(), 6, base, kYaw, kPitch, [](uintptr_t, int) {}) == 0 &&
                            aimrules::ReachScan(mem.data(), 100, base, kNaN, kPitch, [](uintptr_t, int) {}) == 0;
        const bool reads = aimrules::ReachReads(aimrules::kMemCommit, aimrules::kMemPrivate, aimrules::kPageReadWrite) &&
                           !aimrules::ReachReads(aimrules::kMemCommit, aimrules::kMemPrivate, 0x404) && !aimrules::ReachReads(aimrules::kMemCommit, aimrules::kMemPrivate, 0x204) &&
                           !aimrules::ReachReads(aimrules::kMemCommit, aimrules::kMemPrivate, 0x104) && !aimrules::ReachReads(aimrules::kMemCommit, aimrules::kMemPrivate, 0x02) &&
                           !aimrules::ReachReads(aimrules::kMemCommit, 0x40000, aimrules::kPageReadWrite) &&
                           !aimrules::ReachReads(aimrules::kMemCommit, 0x1000000, aimrules::kPageReadWrite) &&
                           !aimrules::ReachReads(0x2000, aimrules::kMemPrivate, aimrules::kPageReadWrite) && !aimrules::ReachReads(0x10000, 0, 0x01);
        Report("14", "ReachKind, ReachScan, ReachNext, ReachReads", two && one && refused == 15 && seams && next && broken && reads,
               "the row's shape 2 %d; a pointer alone 1 %d; refused %d of 15 (no pointer, off by 0.0101, a zero, not a number, a pitch past 1.6); every line about the "
               "seams once, none off a line %d (%zu of %zu); ReachNext %d; broken inputs nothing %d; only committed private read-write memory read %d",
               two, one, refused, seams, seen.size(), lines.size(), next, broken, reads);
    }
    // 15
    {
        // The M249's checkpoint (bf3vr_1118_m249ingame.log): the soldier new at 22:03:39, the camera held at yaw 2.3725 and pitch -0.0354 by mouse aim, the
        // walk reaching E7712CCC, E7712D0C and E742FE0C in each of 43 searches, each written over, the sweep finding nothing; the look found at E771174C at
        // 22:09:45 when the walk reached it. The last life's look (E7152E8C, 21:58:59) left as it was.
        constexpr float kY = 2.3725f, kP = -0.0354f;
        const auto launch = [&](Heap& h) {
            h.Add(0xE7000000, 0xE7800000);
            h.copies = {0xE7712CCC, 0xE7712D0C, 0xE742FE0C};
            for (const uintptr_t c : h.copies)
                h.MakeSlot(c, kY, kP);
            h.readSlot = 0xE771174C;
            h.MakeSlot(h.readSlot, kY, kP);
            h.MakeSlot(0xE7152E8C, 2.8969f, 0.1375f);
            h.SetWord(0xE7712CCC - 0x40 - 0xC, 0x30002000);  // the null row
            h.MakeSlot(0xE7712CCC + 0x100, kY + 0.5f, kP + 0.2f);  // a slot of another look
            h.SetWord(0xE7700000, 7), h.Set(0xE770000C, kY), h.Set(0xE7700018, kP);  // a stray pair at the camera's angles, no pointer before it
            h.Tick();
        };
        Heap old;
        launch(old);
        Ever oldEver;
        oldEver.Note(0xE7152E8C);
        aimrules::SweepTried s0, r0;
        int oldFound = 0, oldSwept = 0;
        for (int s = 0; s < 43; ++s) {
            const ReachOutcome o = ReachSearch(old, old.copies, s0, r0, oldEver, false);
            oldFound += o.found != 0, oldSwept += o.swept && o.sweptTaken == 0;
        }
        Heap h;
        launch(h);
        const auto before = h.pages;
        Ever ever;
        ever.Note(0xE7152E8C);
        aimrules::SweepTried s1, r1;
        const ReachOutcome o = ReachSearch(h, h.copies, s1, r1, ever, true);
        const aimrules::Reached last = o.proved ? o.at[o.proved - 1] : aimrules::Reached{};
        const bool pass = oldFound == 0 && oldSwept == 43 && o.found == h.readSlot && o.byReach && !o.firstPass && o.swept && o.sweptTaken == 0 && o.anchors == 4 &&
                          o.proved == 1 && last.Distance() == -0x1580 && last.anchor == 0xE7712CCC && last.kind == 2 && WritesOnSlotsOnly(h) && AllAsBefore(h, before) &&
                          h.forbiddenReads == 0 && ever.at[0] == h.readSlot;
        Report("15", "the M249's checkpoint of 2026-11-18: its look 0x1580 from the copies", pass,
               "as it was: found in %d of 43 searches, the sweep nothing in %d; with the reach the first search: found %08X by the reach %d (%d anchors, %d spans, %.1f MB "
               "read, %d places with the row's shape, %d a pointer alone), %c0x%lX from %08X; writes on the look's slots only %d, every nudge undone and every other "
               "word as it was %d, nothing read but plain memory %d",
               oldFound, oldSwept, static_cast<unsigned>(o.found), o.byReach, o.anchors, o.spans, o.bytes / 1048576.0, o.kind2, o.kind1, last.Distance() < 0 ? '-' : '+',
               static_cast<unsigned long>(std::labs(last.Distance())), static_cast<unsigned>(last.anchor), WritesOnSlotsOnly(h), AllAsBefore(h, before), h.forbiddenReads == 0);
    }
    // 16
    {
        // The user's play of 2026-11-19 (bf3vr_1119_acog.log): (a) the switch back to the M4A1 at 01:14:49: the walk reaching E77199CC and E7719CCC (the
        // Mk11's row) for a minute, the look at E754614C (0x1D3880 below); (b) 01:20 to 01:47: copies in two arenas (E7719CCC, DF49F43C, DF3CFF9C), the look
        // at DF7DF9BC (0x340580 above DF49F43C), 292 s of frames.
        constexpr float kY = 1.4023f, kP = 0.2634f;
        int oldFound = 0, firstSearch = 0, safe = 0;
        long distances[2] = {};
        for (int c = 0; c < 2; ++c) {
            const auto launch = [&](Heap& h) {
                h.Add(0xE7400000, 0xE7900000);
                if (c == 0) {
                    h.copies = {0xE77199CC, 0xE7719CCC};
                    h.readSlot = 0xE754614C;
                } else {
                    h.Add(0xDF000000, 0xDFA00000);
                    h.copies = {0xE7719CCC, 0xDF49F43C, 0xDF3CFF9C};
                    h.readSlot = 0xDF7DF9BC;
                }
                for (const uintptr_t at : h.copies)
                    h.MakeSlot(at, kY, kP);
                h.MakeSlot(h.readSlot, kY, kP);
                h.Tick();
            };
            Heap old;
            launch(old);
            Ever e0;
            aimrules::SweepTried s0, r0;
            for (int s = 0; s < 20; ++s)
                oldFound += ReachSearch(old, old.copies, s0, r0, e0, false).found != 0;
            Heap h;
            launch(h);
            const auto before = h.pages;
            Ever ever;
            aimrules::SweepTried s1, r1;
            const ReachOutcome o = ReachSearch(h, h.copies, s1, r1, ever, true);
            firstSearch += o.found == h.readSlot && o.byReach;
            distances[c] = o.proved ? o.at[o.proved - 1].Distance() : 0;
            safe += WritesOnSlotsOnly(h) && AllAsBefore(h, before) && h.forbiddenReads == 0 && o.bytes <= aimrules::kReachBytesMost;
        }
        Report("16", "the user's M4A1 of 2026-11-19: 1.9 and 3.4 MB from the copies", oldFound == 0 && firstSearch == 2 && distances[0] == -0x1D3880 && distances[1] == 0x340580 && safe == 2,
               "as it was: found in %d of 40 searches; with the reach in the first search %d of 2 (%c0x%lX, %c0x%lX from the nearest copy); writes on the look's slots "
               "only, every nudge undone, plain memory alone, 64 MB at most %d of 2",
               oldFound, firstSearch, distances[0] < 0 ? '-' : '+', static_cast<unsigned long>(std::labs(distances[0])), distances[1] < 0 ? '-' : '+',
               static_cast<unsigned long>(std::labs(distances[1])), safe);
    }
    // 17
    {
        // A new life whose walk reaches nothing at the camera's angles: the reach looks about the launch's looks proved before (the last life's M4A1 at
        // E77BCD0C, 00:59:52; the new life's at E754614C, 0x276BC0 below it). The launch's first life has none: nothing read, as before.
        constexpr float kY = -1.3890f, kP = -0.0645f;
        Heap h;
        h.Add(0xE7400000, 0xE7900000);
        h.copies = {};
        h.readSlot = 0xE754614C;
        h.MakeSlot(h.readSlot, kY, kP);
        h.MakeSlot(0xE77BCD0C, 0.4490f, -0.3006f);  // the last life's look, as it was left
        h.Tick();
        const auto before = h.pages;
        Ever ever;
        ever.Note(0xE77BCD0C);
        aimrules::SweepTried s1, r1;
        const ReachOutcome o = ReachSearch(h, {}, s1, r1, ever, true);
        Heap first;
        first.Add(0xE7400000, 0xE7900000);
        first.readSlot = 0xE754614C;
        first.MakeSlot(first.readSlot, kY, kP);
        first.Tick();
        Ever none;
        aimrules::SweepTried s2, r2;
        const ReachOutcome f = ReachSearch(first, {}, s2, r2, none, true);
        const bool pass = o.found == h.readSlot && o.byReach && o.anchors == 1 && o.proved == 1 && o.at[0].Distance() == -0x276BC0 && WritesOnSlotsOnly(h) &&
                          AllAsBefore(h, before) && !f.found && !f.reached && first.bytesRead == 0;
        Report("17", "a new life, nothing reached: about the looks proved before", pass,
               "found %08X by the reach %d about %d anchor (%c0x%lX from the last life's look); the launch's first life: reached %d, %zu bytes read",
               static_cast<unsigned>(o.found), o.byReach, o.anchors, o.at[0].Distance() < 0 ? '-' : '+', static_cast<unsigned long>(std::labs(o.at[0].Distance())),
               f.reached, first.bytesRead);
    }
    // 18
    {
        // The look without the row's shape (a pointer, then words that aren't zero: 46 of 128 finds came with no slot-shaped pair at that instant), 0x9000
        // from the copies reached; two copies with the row's shape nearer but off the row's step (out of the sweep's sight); a stray pair at the camera's
        // angles with no pointer before it, one 4 bytes off a line and one 8: the copies first, then the look, in the first search's hold.
        Heap h;
        h.Add(0x30000000, 0x30400000);
        constexpr uintptr_t kCopy = 0x30100A4C;
        h.copies = {kCopy, kCopy + 0x40, kCopy + 0x5010, kCopy - 0x6010};
        for (const uintptr_t at : h.copies)
            h.MakeSlot(at, kYaw, kPitch);
        h.readSlot = kCopy + 0x9000;
        h.MakeSlot(h.readSlot, kYaw, kPitch, 0x02237778);
        h.SetWord(h.readSlot - 8, 0x3F800000), h.SetWord(h.readSlot + 4, 0x3DCCCCCD);  // (1.0 and 0.1: not the row's shape)
        h.SetWord(kCopy - 0xC + 0x2000, 7), h.Set(kCopy + 0x2000, kYaw), h.Set(kCopy + 0x200C, kPitch);  // on a line, no pointer
        h.MakeSlot(kCopy + 0x3004, kYaw, kPitch), h.MakeSlot(kCopy + 0x4008, kYaw, kPitch);          // off a line by 4 and 8
        h.Tick();
        const auto before = h.pages;
        Ever ever;
        aimrules::SweepTried s1, r1;
        const std::vector<uintptr_t> reached = {kCopy, kCopy + 0x40};
        const ReachOutcome o = ReachSearch(h, reached, s1, r1, ever, true);
        const bool order = o.proved == 3 && o.at[0].kind == 2 && o.at[1].kind == 2 && o.answer[0] == Proof::kOverwritten && o.answer[1] == Proof::kOverwritten &&
                           o.at[2].kind == 1 && o.at[2].at == h.readSlot && o.answer[2] == Proof::kYes && o.at[0].at == kCopy + 0x5010 && o.at[1].at == kCopy - 0x6010;
        const bool pass = o.found == h.readSlot && order && o.kind1 == 1 && WritesOnSlotsOnly(h) && AllAsBefore(h, before);
        Report("18", "a look without the row's shape: after the copies with it, in the same hold", pass,
               "found %08X by the reach %d: proved %d (%s, %s, then the look %s); places with a pointer alone %d (the strays: no pointer, off a line, never taken); "
               "writes on the look's slots only %d, every nudge undone %d",
               static_cast<unsigned>(o.found), o.byReach, o.proved, o.proved > 0 ? (o.at[0].kind == 2 ? "the row's shape" : "a pointer") : "-",
               o.proved > 1 ? (o.at[1].kind == 2 ? "the row's shape" : "a pointer") : "-", o.proved > 2 && o.at[2].kind == 1 ? "with a pointer alone" : "?", o.kind1,
               WritesOnSlotsOnly(h), AllAsBefore(h, before));
    }
    // 19
    {
        // Crowded: more than kReachMost places of a kind at the camera's angles. Seventeen still slots of the row's shape and the look: nothing nudged by
        // the reach. Seventeen with a pointer alone and the look with the row's shape: those seventeen left out, the look proved.
        const auto launch = [&](Heap& h, bool rowShape) {
            h.Add(0x30000000, 0x30400000);
            h.copies = {0x30100A4C};
            h.MakeSlot(h.copies[0], kYaw, kPitch);
            h.readSlot = 0x30150A4C;
            h.MakeSlot(h.readSlot, kYaw, kPitch);
            for (int k = 0; k < 17; ++k) {
                const uintptr_t at = 0x30200A4C + k * 0x1000;
                h.MakeSlot(at, kYaw, kPitch);
                if (!rowShape)
                    h.SetWord(at - 4, 0x3F800000);
            }
            h.Tick();
        };
        Heap a;
        launch(a, true);
        Ever e1;
        aimrules::SweepTried s1, r1;
        const ReachOutcome oa = ReachSearch(a, a.copies, s1, r1, e1, true);
        // (19 with the row's shape: the copy, the look and the 17. The first pass's candidate, written over, is the only one nudged.)
        const bool none = !oa.found && oa.reached && oa.crowd == 2 && oa.taken == 0 && oa.proved == 0 && oa.kind2 == 19 && WritesOnSlotsOnly(a);
        int stillNudged = 0;
        for (const uintptr_t at : a.written)
            stillNudged += at >= 0x30200A4C;
        Heap b;
        launch(b, false);
        Ever e2;
        aimrules::SweepTried s2, r2;
        const ReachOutcome ob = ReachSearch(b, b.copies, s2, r2, e2, true);
        int pointerNudged = 0;
        for (const uintptr_t at : b.written)
            pointerNudged += at >= 0x30200A4C;
        const bool left = ob.found == b.readSlot && ob.crowd == 1 && ob.kind1 == 17 && pointerNudged == 0;
        Report("19", "crowded: more than 16 of a kind, that kind never nudged", none && stillNudged == 0 && left,
               "%d with the row's shape: crowded %d, taken %d, nudged among them %d, found %d; 17 with a pointer alone and the look: crowded %d, the 17 nudged %d, the "
               "look proved %d",
               oa.kind2, oa.crowd, oa.taken, stillNudged, oa.found != 0, ob.crowd, pointerNudged, ob.found == b.readSlot);
    }
    // 20
    {
        // AMD and the rest of what's never read: the look in write-combined memory (0x404, as an RX 7900 XT's mapped buffers), uncached (0x204), behind a
        // guard page (0x104), in a mapped file, in a module, reserved only: never read, never found. In plain memory past a region that can't be read
        // midway (freed while it's read): found. A copy of the look's words in the reach's own allocation: never read.
        struct Case {
            uint32_t protect, type, state;
        };
        const Case cases[6] = {{0x404, aimrules::kMemPrivate, aimrules::kMemCommit}, {0x204, aimrules::kMemPrivate, aimrules::kMemCommit},
                               {0x104, aimrules::kMemPrivate, aimrules::kMemCommit}, {aimrules::kPageReadWrite, 0x40000, aimrules::kMemCommit},
                               {aimrules::kPageReadWrite, 0x1000000, aimrules::kMemCommit}, {aimrules::kPageReadWrite, aimrules::kMemPrivate, 0x2000}};
        int untouched = 0;
        for (const Case& c : cases) {
            Heap h;
            h.Add(0x30000000, 0x30200000);
            h.Add(0x30200000, 0x30210000, c.protect, c.type, c.state);
            h.Add(0x30210000, 0x30400000);
            h.copies = {0x30100A4C};
            h.MakeSlot(h.copies[0], kYaw, kPitch);
            h.readSlot = 0x30200A4C;
            h.MakeSlot(h.readSlot, kYaw, kPitch);
            h.Tick();
            Ever ever;
            aimrules::SweepTried s1, r1;
            const ReachOutcome o = ReachSearch(h, h.copies, s1, r1, ever, true);
            untouched += !o.found && h.forbiddenReads == 0 && o.reached;
        }
        Heap h;
        h.Add(0x30000000, 0x30200000);
        h.Add(0x30200000, 0x30300000);
        h.Add(0x30300000, 0x30400000);
        h.poisoned.push_back({0x30280000, 0x30290000});
        h.copies = {0x30100A4C};
        h.MakeSlot(h.copies[0], kYaw, kPitch);
        h.readSlot = 0x30350A4C;
        h.MakeSlot(h.readSlot, kYaw, kPitch);
        h.MakeSlot(0x30380A4C, kYaw, kPitch);  // a copy of its words in the reach's own allocation
        h.Add(0x30380000, 0x30390000);
        h.regions[2].to = 0x30380000;
        h.Add(0x30390000, 0x30400000);
        h.Tick();
        Ever ever;
        aimrules::SweepTried s1, r1;
        const ReachOutcome o = ReachSearch(h, h.copies, s1, r1, ever, true, 0x1000, 0x30380000, 0x30390000);
        int ownTaken = 0;
        for (int n = 0; n < o.proved; ++n)
            ownTaken += o.at[n].at == 0x30380A4C;
        // (2 with the row's shape: the copy and the look; the copy in its own allocation never seen.)
        const bool past = o.found == h.readSlot && ownTaken == 0 && o.kind2 == 2 && h.forbiddenReads == 0;
        Report("20", "AMD's write-combined memory and the rest never read", untouched == 6 && past,
               "write-combined, uncached, a guard page, a mapped file, a module, reserved: untouched and not found %d of 6; past a region that can't be read midway "
               "found %d, its own allocation never read %d",
               untouched, o.found == h.readSlot, ownTaken == 0 && o.kind2 == 2);
    }
    // 21
    {
        // ReachSpans: two anchors 1 MB apart one span; two arenas two spans of 32 MB (64 MB in all); a third anchor past the most nothing; memory's ends;
        // 16-byte lines; each byte once; lowest first; 0 skipped.
        aimrules::Span s[8] = {};
        const uintptr_t joined[2] = {0xE7712CCC, 0xE7612CCC};
        const int nJoined = aimrules::ReachSpans(joined, 2, s, 8, aimrules::kReachBytesMost);
        const bool one = nJoined == 1 && s[0].from == ((0xE7612CCC - aimrules::kReachEachWay) & ~0xFu) && s[0].to == ((0xE7712CCC + aimrules::kReachEachWay) & ~0xFu);
        const uintptr_t arenas[4] = {0xE7719CCC, 0, 0xDF49F43C, 0xC0000000};
        const int nArenas = aimrules::ReachSpans(arenas, 4, s, 8, aimrules::kReachBytesMost);
        size_t total = 0;
        bool lines = true, sorted = true;
        for (int k = 0; k < nArenas; ++k) {
            total += s[k].to - s[k].from;
            lines = lines && !(s[k].from & 0xF) && !(s[k].to & 0xF);
            sorted = sorted && (k == 0 || s[k].from > s[k - 1].to);
        }
        const bool two = nArenas == 2 && total <= aimrules::kReachBytesMost && total >= aimrules::kReachBytesMost - 64 && s[0].from < 0xDF49F43C && s[0].to > 0xDF49F43C &&
                         s[1].from < 0xE7719CCC && s[1].to > 0xE7719CCC && lines && sorted;
        const uintptr_t ends[2] = {0x20000, 0xFFFE0000u};
        const int nEnds = aimrules::ReachSpans(ends, 2, s, 8, aimrules::kReachBytesMost);
        const bool edge = nEnds == 2 && s[0].from == 0x10000 && s[1].to == 0xFFFF0000u;
        const uintptr_t bad[3] = {0, 0x8000, 0xFFFF8000u};
        const bool nothing = aimrules::ReachSpans(bad, 3, s, 8, aimrules::kReachBytesMost) == 0 && aimrules::ReachSpans(nullptr, 3, s, 8, aimrules::kReachBytesMost) == 0;
        // The cut: 40 MB left for a second anchor's window gives it 20 MB each way... here 20 MB in all, 10 each way about it.
        const uintptr_t cut[2] = {0x80000000, 0x90000000};
        const int nCut = aimrules::ReachSpans(cut, 2, s, 8, 0x2000000 + 0x1400000);
        const bool cutOk = nCut == 2 && s[0].to - s[0].from == 0x2000000 && s[1].to - s[1].from == 0x1400000 && s[1].from < 0x90000000 && s[1].to > 0x90000000;
        Report("21", "ReachSpans: joined, two arenas, the most, the ends", one && two && edge && nothing && cutOk,
               "1 MB apart one span %d; two arenas %d spans %.1f MB (lines %d, lowest first %d); memory's ends %d; 0, under 0x10000, past 0xFFFF0000 nothing %d; "
               "the second cut about it to what's left %d",
               one, nArenas, total / 1048576.0, lines, sorted, edge, nothing, cutOk);
    }
    // 22
    {
        // The time: 32 MB of real memory (random words, four looks at the camera's angles among them) looked through in 64 KB pieces as ReachLook does.
        constexpr size_t kBytes = 32u << 20;
        uint32_t* memory = static_cast<uint32_t*>(VirtualAlloc(nullptr, kBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        int found = 0;
        double best = 1e9;
        if (memory) {
            std::mt19937 random(1118);
            for (size_t k = 0; k < kBytes / 4; ++k)
                memory[k] = random();
            const size_t looks[4] = {0x1000, 0x123450, 0x3FFFFC, 0x7FFFF0};  // (word indexes on 16-byte lines; the third across a piece's seam)
            for (const size_t k : looks) {
                memory[k] = 0x30001000, memory[k + 1] = memory[k + 2] = memory[k + 4] = memory[k + 5] = 0;
                std::memcpy(&memory[k + 3], &kYaw, 4), std::memcpy(&memory[k + 6], &kPitch, 4);
            }
            const auto base = reinterpret_cast<uintptr_t>(memory);
            std::vector<uint32_t> piece(0x4006);
            for (int round = 0; round < 3; ++round) {
                LARGE_INTEGER f, t0, t1;
                QueryPerformanceFrequency(&f);
                QueryPerformanceCounter(&t0);
                int seenNow = 0;
                for (uintptr_t from = base; from < base + kBytes && base + kBytes - from >= 28;) {
                    const size_t words = std::min<size_t>(0x4006, (base + kBytes - from) / 4);
                    std::memcpy(piece.data(), reinterpret_cast<const void*>(from), words * 4);
                    seenNow += aimrules::ReachScan(piece.data(), words, from, kYaw, kPitch, [](uintptr_t, int) {});
                    from = aimrules::ReachNext(from, words);
                }
                QueryPerformanceCounter(&t1);
                best = std::min(best, static_cast<double>(t1.QuadPart - t0.QuadPart) / f.QuadPart * 1000.0);
                found = seenNow;
            }
            VirtualFree(memory, 0, MEM_RELEASE);
        }
        Report("22", "the time: 32 MB looked through", memory && found == 4 && best < 100.0, "the four looks found %d; best of three %.1f ms (%.0f MB a second)", found, best,
               best > 0 ? 32.0 / (best / 1000.0) : 0.0);
    }
    // 23
    {
        // ReachChoice alone: the row's shape before a pointer alone, nearest an anchor first, above before below at one distance, each place once, a candidate
        // left out uncounted, one tried before left out and counted, the count past the four it gives; and the round again as SweepTried's.
        const uintptr_t anchors[2] = {0x30100000, 0x31000000};
        const aimrules::Reached places[9] = {{0x30100100, 1, 0}, {0x30100200, 2, 0}, {0x300FFE00, 2, 0}, {0x30100200, 2, 0}, {0x31000040, 2, 0},
                                             {0x30FFFFC0, 2, 0}, {0x30100040, 2, 0}, {0x30200000, 2, 0}, {0x30100080, 0, 0}};
        aimrules::Reached out[8] = {};
        int skipped = -1, crowd = -1;
        const auto none = [](uintptr_t) { return false; };
        const int all = aimrules::ReachChoice(places, 9, anchors, 2, none, none, out, 8, &skipped, &crowd);
        const bool order = all == 7 && out[0].at == 0x30100040 && out[1].at == 0x31000040 && out[2].at == 0x30FFFFC0 && out[3].at == 0x30100200 &&
                           out[4].at == 0x300FFE00 && out[5].at == 0x30200000 && out[6].at == 0x30100100 && out[6].kind == 1 && out[1].anchor == 0x31000000 &&
                           out[4].Distance() == -0x200 && skipped == 0 && crowd == 0;
        aimrules::Reached four[4] = {};
        const int capped = aimrules::ReachChoice(
            places, 9, anchors, 2, [](uintptr_t at) { return at == 0x31000040; }, [](uintptr_t at) { return at == 0x30FFFFC0 || at == 0x30200000; }, four, 4, &skipped,
            &crowd);
        const bool cap = capped == 4 && skipped == 2 && four[0].at == 0x30100040 && four[1].at == 0x30100200 && four[2].at == 0x300FFE00 && four[3].at == 0x30100100;
        const bool broken = aimrules::ReachChoice(nullptr, 5, anchors, 2, none, none, out, 8) == 0 && aimrules::ReachChoice(places, 9, nullptr, 0, none, none, out, 8) == 7;
        aimrules::SweepTried t;
        t.For(0x1000);
        t.Answered(0x40, aimrules::Answer::kOverwritten);
        int early = 0;
        for (int s = 0; s < aimrules::kSweepRest - 1; ++s)
            early += t.Chosen(0, 1);
        const bool round = !early && t.Chosen(0, 1) && t.places.empty();
        Report("23", "ReachChoice: the order, each once, left out, the count; the round again", order && cap && broken && round,
               "seven places: the row's shape nearest first, above before below, then the pointer's alone, each once %d (%d taken); a candidate uncounted, two tried "
               "before counted, four given %d (%d taken, %d left out); broken inputs %d; tried places forgotten at the fifth search with nothing to take %d",
               order, all, cap, capped, skipped, broken, round);
    }

    std::printf("%d of %d checks failed\n", g_failed, g_counted);
    return g_failed;
}

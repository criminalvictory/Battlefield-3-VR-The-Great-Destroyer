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
// The reach (src/aimrules.h: ReachKind, ReachScan, ReachNext, ReachReads, ReachSpans, ReachThrough, ReachChoice, ReachWanted, ReachPointerAlone, as
// aim.cpp's ReachLook and Search run them; src/aimmemory.h, the game's memory as aim.cpp reads it, run on this program's own), on a sparse piece of
// memory megabytes across with VirtualQuery's regions and allocations, a working set and a clock (Heap), the game and Proves as above:
//  14  ReachKind: 2 the row's shape, 1 a pointer alone, never without a pointer, off by 0.0101, a zero, not a number; ReachScan in 64 KB pieces: every
//      16-byte line about the seams once, none off a line, a pointer alone only when asked (ReachPointerAlone: copies, none with the row's shape);
//      ReachNext; ReachReads: only committed, private, exactly read-write memory
//  15  the M249's checkpoint of 2026-11-18 22:03 (bf3vr_1118_m249ingame.log): the walk reaching three copies, the look 0x1580 below them: as it was,
//      nothing in 43 searches; with the reach the first search, nothing written but nudges on the look's slots, each undone, nothing but plain memory read
//  16  the user's play of 2026-11-19 (bf3vr_1119_acog.log): the M4A1's look 0x1D3880 from the copies after the switch from the Mk11 (one arena), the
//      M249's 0x340580 (across two): as it was nothing in 20 searches each; with the reach the first search, 64 MB read at most
//  17  a new life whose walk reaches nothing: found about a look proved three lives before (0x276BC0 from it); the launch's first life: no anchor,
//      nothing read
//  18  a look without the row's shape: with copies without it too (the .50 cal run of 2026-11-18), found by the pointer before it alone in the first
//      search, 0xD9C40 below them (the walk took 678.8 s); with copies that have it, a pointer alone never looked for and the look never nudged, only the
//      row's shape (two copies off the row's step); strays (no pointer, off a line by 4 and 8) never
//  19  crowded: 19 with the row's shape at the camera's angles none nudged; 18 with a pointer alone left out, the look with the row's shape proved
//  20  AMD's write-combined memory (0x404), uncached, a guard page, a mapped file, a module, reserved: never read, the look in one never found; past a
//      region that can't be read midway, found; the reach's own allocation never read; a thread's stack (a guard page in its allocation) never read,
//      one committed whole never read where the threads' list names it
//  21  ReachSpans: rings nearest first (each anchor's 64 KB, then 1 MB, 5 MB, 16 MB each way), each byte once, on lines: one anchor; two 1 MB apart;
//      three far apart, each one's 5 MB looked through before any 16 MB (as it was the third had none); sixteen, each one's 1 MB; the most never
//      passed, a second anchor cut about it; memory's ends; nothing for bad anchors
//  22  the time on this program's own memory (aimmemory.h: VirtualQuery, the working set, QueryPerformanceCounter): 32 MB looked through, the four
//      looks found, under 100 ms; cut at 3 ms, the look nearest its anchor found all the same
//  23  ReachChoice alone: the row's shape first, nearest an anchor first, above before below, each once, a candidate uncounted, tried before counted,
//      the count past the four it gives, broken inputs, a crowd counted past what's kept; the tried places forgotten at the fifth search with nothing
//      to take
//  24  the time on a PC five times slower (1.5 us a KB): stopped at 25 ms, within a piece; the M4A1's look 1.9 MB above its copy found, nearest first,
//      where a look through in memory's order from 16 MB below (as it was) is cut before it
//  25  memory read slowly though called plain read-write (as AMD's write-combined reads: 180 us a KB): a piece read twice, its allocation left for the
//      search, a look past it found in the same search; at the second search left for the launch, never read again; a plain piece slow once (a thread
//      switched out) read again, nothing struck
//  26  the working set: a page out of it never read (the look in one not taken, the reach's own nor Proves' reads), the look between two such pages
//      found, a block ending where a page out of it begins found; the working set not known: every page read, as before
//  27  this program's own pages and threads (aimmemory.h): a page put out of the working set (VirtualUnlock) never read and still out after, no page
//      out of it brought in; a look on a thread's stack (a default one, its guard page; one committed whole, none) never taken with the threads' list
//      (AddThreadStacks names both), the one committed whole taken without the list
//  28  the cap per kind: 300 with a pointer alone and 3 with the row's shape (the look among them): all 3 kept, the look found, the pointer's crowded;
//      250 with a pointer alone and 20 with the row's shape: the row's shape crowded, none nudged
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

#include "../src/aimmemory.h"
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

// A sparse piece of 32-bit memory for the reach, megabytes across: its 4 KB pages made as they're written, regions with VirtualQuery's state, type,
// protection and allocation (outside them memory is free), pages out of the working set, a clock that reads move (each region at its own speed), and
// the game on it as World's. Reads of a region the reach mustn't read (write-combined, uncached, a guard page, a mapped file, a module) and of a page out
// of the working set are counted.
struct Heap {
    struct Region {
        uintptr_t from = 0, to = 0;
        uint32_t state = aimrules::kMemCommit, type = aimrules::kMemPrivate, protect = aimrules::kPageReadWrite;
        uintptr_t allocation = 0;  // its allocation's base (0: its own from)
        double microsPerKB = 0.3;  // how long its reads take (plain memory: 64 KB in 19 us, as on the user's PC)
    };
    std::vector<Region> regions;  // disjoint
    std::unordered_map<uintptr_t, std::vector<uint32_t>> pages;
    uintptr_t readSlot = 0;
    std::vector<uintptr_t> copies;
    float cameraYaw = 0.0f, cameraPitch = 0.0f;
    std::vector<uintptr_t> written;
    uint64_t ms = 0;
    size_t forbiddenReads = 0, bytesRead = 0;
    std::unordered_set<uintptr_t> out;  // pages out of the working set
    bool workingSetKnown = true;
    size_t outReads = 0;                // words read from them
    std::vector<uintptr_t> own;         // allocations the reach never reads (its own piece, the stacks the threads' list names)
    double clock = 0.0;                 // microseconds
    int preemptBig = 0;                 // reads of 4 KB or more still to be made 15 ms longer (a thread switched out in the middle)
    std::unordered_map<uintptr_t, size_t> regionBytes;  // bytes read from each region (by its from)

    void Add(uintptr_t from, uintptr_t to, uint32_t protect = aimrules::kPageReadWrite, uint32_t type = aimrules::kMemPrivate,
             uint32_t state = aimrules::kMemCommit, uintptr_t allocation = 0, double microsPerKB = 0.3) {
        regions.push_back({from, to, state, type, protect, allocation, microsPerKB});
    }
    const Region* At(uintptr_t at) const {
        for (const Region& r : regions)
            if (at >= r.from && at < r.to)
                return &r;
        return nullptr;
    }
    // VirtualQuery: the region about at, or the free gap up to the next region.
    bool Query(uintptr_t at, aimrules::Region& q) const {
        if (const Region* r = At(at)) {
            q.base = r->from, q.end = r->to, q.allocation = r->allocation ? r->allocation : r->from;
            q.state = r->state, q.type = r->type, q.protect = r->protect;
            return true;
        }
        uintptr_t next = 0xFFFF0000u;
        for (const Region& r : regions)
            if (r.from > at && r.from < next)
                next = r.from;
        q.base = at & ~static_cast<uintptr_t>(0xFFF), q.end = next, q.allocation = 0, q.state = 0x10000 /* MEM_FREE */, q.type = 0, q.protect = 0x01;
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
    // false; reads of what the reach mustn't read are counted, and the clock moves by each region's speed.
    std::vector<std::pair<uintptr_t, uintptr_t>> poisoned;  // ranges whose reads fail (a region freed while it's read)
    bool Read(uintptr_t at, void* dest, size_t size) {
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
            clock += static_cast<double>(regionEnd - a) / 1024.0 * r->microsPerKB;
            regionBytes[r->from] += regionEnd - a;
            while (a < regionEnd) {
                const uintptr_t page = a & ~static_cast<uintptr_t>(0xFFF);
                const uintptr_t pageEnd = std::min<uintptr_t>(page + 0x1000, regionEnd);
                const auto p = pages.find(page);
                uint8_t* to = static_cast<uint8_t*>(dest) + (a - at);
                if (p == pages.end())
                    std::memset(to, 0, pageEnd - a);
                else
                    std::memcpy(to, &p->second[(a & 0xFFF) / 4], pageEnd - a);
                if (forbidden)
                    forbiddenReads += (pageEnd - a) / 4;
                if (out.count(page))
                    outReads += (pageEnd - a) / 4;
                a = pageEnd;
            }
        }
        if (preemptBig > 0 && size >= 0x1000)
            --preemptBig, clock += 15000.0;
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
    // A place with a pointer before it alone: the pointer, then words that aren't zero (1.0 and 0.1), the angles where the row's are.
    void MakeLone(uintptr_t at, float yaw, float pitch, uint32_t pointer = 0x02237778) {
        MakeSlot(at, yaw, pitch, pointer);
        SetWord(at - 8, 0x3F800000), SetWord(at + 4, 0x3DCCCCCD);
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

// A Heap as aimrules::ReachThrough's memory (aim.cpp's is aimmemory::GameMemory).
struct HeapMemory {
    Heap& h;
    std::vector<uint32_t> piece = std::vector<uint32_t>(aimrules::kReachPieceWords);
    explicit HeapMemory(Heap& heap) : h(heap) {}
    uint32_t* Piece() { return piece.data(); }
    bool Query(uintptr_t at, aimrules::Region& r) const { return h.Query(at, r); }
    bool Own(uintptr_t allocation) const { return std::find(h.own.begin(), h.own.end(), allocation) != h.own.end(); }
    bool Resident(uintptr_t page, int count, uint8_t* valid) const {
        if (!h.workingSetKnown)
            return false;
        for (int k = 0; k < count; ++k)
            valid[k] = h.out.count(page + static_cast<uintptr_t>(k) * 0x1000) ? 0 : 1;
        return true;
    }
    bool Read(uintptr_t from, uint32_t* to, size_t words) { return h.Read(from, to, words * 4); }
    uint64_t Micros() const { return static_cast<uint64_t>(h.clock); }
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
    aimrules::ReachTotals totals;
};
// aim.cpp's ReachLook: the spans in ReachSpans' order, ReachThrough on the Heap (its own allocations and the threads' stacks in h.own), the slow
// allocations in memo.
ReachRun ReachLook(Heap& h, const uintptr_t* anchors, int count, float cameraYaw, float cameraPitch, bool pointerAlone, aimrules::ReachMemo& memo) {
    ReachRun run;
    constexpr int kSpansMost = 256;
    aimrules::Span spans[kSpansMost];
    run.spans = aimrules::ReachSpans(anchors, count, spans, kSpansMost, aimrules::kReachBytesMost);
    HeapMemory memory(h);
    run.totals = aimrules::ReachThrough(spans, run.spans, memory, memo, cameraYaw, cameraPitch, pointerAlone, run.places);
    return run;
}

struct ReachOutcome {
    uintptr_t found = 0;
    bool firstPass = false, bySweep = false, byReach = false, swept = false, reached = false, pointerAlone = false;
    int candidates = 0, shaped = 0, copies = 0, shapedCopies = 0;
    int sweptTaken = 0, sweptProved = 0;
    int anchors = 0, taken = 0, proved = 0, before = 0, crowd = 0, kind2 = 0, kind1 = 0, spans = 0;
    size_t bytes = 0;
    aimrules::ReachTotals totals;
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
// it), its anchors this search's candidates and swept places written over and the launch's looks proved (ever), a pointer alone looked for as
// ReachPointerAlone says of the copies (the candidates written over, and their shape at one instant; the sweep's all have the row's). memo: the
// launch's slow allocations (none given: a fresh one).
ReachOutcome ReachSearch(Heap& h, const std::vector<uintptr_t>& reached, aimrules::SweepTried& swept, aimrules::SweepTried& reachTried, Ever& ever, bool reach,
                         uintptr_t soldier = 0x1000, aimrules::ReachMemo* memo = nullptr) {
    aimrules::ReachMemo fresh;
    aimrules::ReachMemo& slow = memo ? *memo : fresh;
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
            if (answers[k] == Proof::kOverwritten) {
                anchor(pairs[k]);
                ++o.copies, o.shapedCopies += k < static_cast<size_t>(o.shaped);
            }
        for (int n = 0; n < o.sweptProved; ++n)
            if (sweptAnswer[n] == Proof::kOverwritten) {
                anchor(sweptAt[n].at);
                ++o.copies, ++o.shapedCopies;
            }
        for (const uintptr_t at : ever.at)
            anchor(at);
        o.pointerAlone = aimrules::ReachPointerAlone(o.copies, o.shapedCopies);
        if (aimrules::ReachWanted(true, false, crowded, o.anchors) && std::fabs(h.cameraPitch) > 0.02f && std::fabs(h.cameraYaw) > 0.02f) {
            o.reached = true;
            const ReachRun run = ReachLook(h, anchors, o.anchors, h.cameraYaw, h.cameraPitch, o.pointerAlone, slow);
            o.spans = run.spans, o.bytes = run.totals.bytes, o.totals = run.totals;
            o.kind2 = run.totals.counts[2], o.kind1 = run.totals.counts[1];
            const auto sweptHere = [&](uintptr_t at) {
                for (int n = 0; n < o.sweptProved; ++n)
                    if (sweptAt[n].at == at)
                        return true;
                return false;
            };
            o.taken = aimrules::ReachChoice(
                run.places.data(), static_cast<int>(run.places.size()), anchors, o.anchors,
                [&](uintptr_t at) { return std::find(pairs.begin(), pairs.end(), at) != pairs.end() || sweptHere(at); },
                [&](uintptr_t at) { return reachTried.Has(at) || swept.Has(at); }, o.at, aimrules::kSweepMost, &o.before, &o.crowd, run.totals.counts);
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

// Row 27's threads: a look with the row's shape on the thread's own stack, held there (its frame live) until told.
struct StackHold {
    float yaw = 0.0f, pitch = 0.0f;
    uintptr_t at = 0;  // the look's line on the stack
    HANDLE ready = nullptr, done = nullptr;
};
DWORD WINAPI HoldOnStack(LPVOID param) {
    StackHold& s = *static_cast<StackHold*>(param);
    alignas(16) volatile uint32_t frame[64] = {};
    volatile uint32_t* line = &frame[16];
    line[0] = 0x30001000, line[1] = 0, line[2] = 0, line[4] = 0, line[5] = 0;
    uint32_t y = 0, p = 0;
    std::memcpy(&y, &s.yaw, 4), std::memcpy(&p, &s.pitch, 4);
    line[3] = y, line[6] = p;
    s.at = reinterpret_cast<uintptr_t>(line);
    SetEvent(s.ready);
    WaitForSingleObject(s.done, 30000);
    return frame[16];  // (the frame live until here)
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
        // line; looked through in pieces of 64 KB and six words as ReachThrough does.
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
            const size_t words = std::min<size_t>(aimrules::kReachPieceWords, (base + mem.size() * 4 - from) / 4);
            aimrules::ReachScan(&mem[(from - base) / 4], words, from, kYaw, kPitch, true, [&](uintptr_t at, int) { seen.push_back(at); });
            from = aimrules::ReachNext(from, words);
        }
        std::sort(seen.begin(), seen.end());
        std::sort(lines.begin(), lines.end());
        const bool seams = seen == lines;
        const bool next = aimrules::ReachNext(base, 0x4006) == base + 0x10000 && aimrules::ReachNext(base, 7) == base + 16 && aimrules::ReachNext(base, 6) == base &&
                          aimrules::ReachNext(base, 10) == base + 16 && aimrules::ReachNext(base, 11) == base + 32;
        const bool broken = aimrules::ReachScan(nullptr, 100, base, kYaw, kPitch, true, [](uintptr_t, int) {}) == 0 &&
                            aimrules::ReachScan(mem.data(), 100, base + 4, kYaw, kPitch, true, [](uintptr_t, int) {}) == 0 &&
                            aimrules::ReachScan(mem.data(), 6, base, kYaw, kPitch, true, [](uintptr_t, int) {}) == 0 &&
                            aimrules::ReachScan(mem.data(), 100, base, kNaN, kPitch, true, [](uintptr_t, int) {}) == 0;
        // A pointer alone only when asked; asked only with copies written over and none of them with the row's shape.
        uint32_t lone[7] = {head, 0x3F800000, 0, 0, 0x3DCCCCCD, 0, 0};
        std::memcpy(&lone[3], &kYaw, 4), std::memcpy(&lone[6], &kPitch, 4);
        int asked = 0, notAsked = 0;
        aimrules::ReachScan(lone, 7, base, kYaw, kPitch, true, [&](uintptr_t, int kind) { asked += kind == 1; });
        aimrules::ReachScan(lone, 7, base, kYaw, kPitch, false, [&](uintptr_t, int) { ++notAsked; });
        const bool alone = asked == 1 && notAsked == 0 && aimrules::ReachPointerAlone(2, 0) && !aimrules::ReachPointerAlone(2, 1) &&
                           !aimrules::ReachPointerAlone(3, 3) && !aimrules::ReachPointerAlone(0, 0);
        const bool reads = aimrules::ReachReads(aimrules::kMemCommit, aimrules::kMemPrivate, aimrules::kPageReadWrite) &&
                           !aimrules::ReachReads(aimrules::kMemCommit, aimrules::kMemPrivate, 0x404) && !aimrules::ReachReads(aimrules::kMemCommit, aimrules::kMemPrivate, 0x204) &&
                           !aimrules::ReachReads(aimrules::kMemCommit, aimrules::kMemPrivate, 0x104) && !aimrules::ReachReads(aimrules::kMemCommit, aimrules::kMemPrivate, 0x02) &&
                           !aimrules::ReachReads(aimrules::kMemCommit, 0x40000, aimrules::kPageReadWrite) &&
                           !aimrules::ReachReads(aimrules::kMemCommit, 0x1000000, aimrules::kPageReadWrite) &&
                           !aimrules::ReachReads(0x2000, aimrules::kMemPrivate, aimrules::kPageReadWrite) && !aimrules::ReachReads(0x10000, 0, 0x01);
        Report("14", "ReachKind, ReachScan, ReachNext, ReachReads, ReachPointerAlone", two && one && refused == 15 && seams && next && broken && reads && alone,
               "the row's shape 2 %d; a pointer alone 1 %d; refused %d of 15 (no pointer, off by 0.0101, a zero, not a number, a pitch past 1.6); every line about the "
               "seams once, none off a line %d (%zu of %zu); ReachNext %d; broken inputs nothing %d; only committed private read-write memory read %d; a pointer "
               "alone only when asked, asked only with copies none of which has the row's shape %d",
               two, one, refused, seams, seen.size(), lines.size(), next, broken, reads, alone);
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
                          h.forbiddenReads == 0 && ever.at[0] == h.readSlot && !o.pointerAlone && !o.totals.cut;
        Report("15", "the M249's checkpoint of 2026-11-18: its look 0x1580 from the copies", pass,
               "as it was: found in %d of 43 searches, the sweep nothing in %d; with the reach the first search: found %08X by the reach %d (%d anchors, %d spans, %.1f MB "
               "read in %llu us of a PC as fast as the user's, cut %d; %d places with the row's shape, a pointer alone looked for %d), %c0x%lX from %08X; writes on the "
               "look's slots only %d, every nudge undone and every other word as it was %d, nothing read but plain memory %d",
               oldFound, oldSwept, static_cast<unsigned>(o.found), o.byReach, o.anchors, o.spans, o.bytes / 1048576.0, static_cast<unsigned long long>(o.totals.micros),
               o.totals.cut, o.kind2, o.pointerAlone, last.Distance() < 0 ? '-' : '+', static_cast<unsigned long>(std::labs(last.Distance())),
               static_cast<unsigned>(last.anchor), WritesOnSlotsOnly(h), AllAsBefore(h, before), h.forbiddenReads == 0);
    }
    // 16
    {
        // The user's play of 2026-11-19 (bf3vr_1119_acog.log): (a) the switch from the Mk11 to the M4A1 at 01:14:49: the walk reaching E77199CC and E7719CCC
        // (the Mk11's row) for a minute, the M4A1's look at E754614C (0x1D3880 below); (b) 01:20 to 01:47, the M249 taken at 01:20:22: copies in two arenas
        // (E7719CCC, DF49F43C, DF3CFF9C), its look at DF7DF9BC (0x340580 above DF49F43C), 292 s of frames.
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
            safe += WritesOnSlotsOnly(h) && AllAsBefore(h, before) && h.forbiddenReads == 0 && o.bytes <= aimrules::kReachBytesMost && !o.totals.cut;
        }
        Report("16", "the user's play of 2026-11-19: the M4A1's look 1.9 MB and the M249's 3.4 MB from the copies",
               oldFound == 0 && firstSearch == 2 && distances[0] == -0x1D3880 && distances[1] == 0x340580 && safe == 2,
               "as it was: found in %d of 40 searches; with the reach in the first search %d of 2 (%c0x%lX, %c0x%lX from the nearest copy); writes on the look's slots "
               "only, every nudge undone, plain memory alone, 64 MB at most, within its time %d of 2",
               oldFound, firstSearch, distances[0] < 0 ? '-' : '+', static_cast<unsigned long>(std::labs(distances[0])), distances[1] < 0 ? '-' : '+',
               static_cast<unsigned long>(std::labs(distances[1])), safe);
    }
    // 17
    {
        // A new life whose walk reaches nothing at the camera's angles: the reach looks about the launch's looks proved before (the user's M4A1 of 2026-11-19
        // at E77BCD0C, 00:59:52, three lives before; the new life's at E754614C, 01:15:49, 0x276BC0 below it). The launch's first life has none: nothing
        // read, as before.
        constexpr float kY = -1.3890f, kP = -0.0645f;
        Heap h;
        h.Add(0xE7400000, 0xE7900000);
        h.copies = {};
        h.readSlot = 0xE754614C;
        h.MakeSlot(h.readSlot, kY, kP);
        h.MakeSlot(0xE77BCD0C, 0.4490f, -0.3006f);  // the earlier life's look, as it was left
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
                          AllAsBefore(h, before) && !o.pointerAlone && !f.found && !f.reached && first.bytesRead == 0;
        Report("17", "a new life, nothing reached: about the looks proved before", pass,
               "found %08X by the reach %d about %d anchor (%c0x%lX from the look three lives before), no pointer alone looked for with no copies %d; the launch's "
               "first life: reached %d, %zu bytes read",
               static_cast<unsigned>(o.found), o.byReach, o.anchors, o.at[0].Distance() < 0 ? '-' : '+', static_cast<unsigned long>(std::labs(o.at[0].Distance())),
               !o.pointerAlone, f.reached, first.bytesRead);
    }
    // 18
    {
        // A look without the row's shape (a pointer, then words that aren't zero). (a) The .50 cal run of 2026-11-18 (bf3vr_1118_sb50cal.log, to 17:20:35):
        // the walk reached E71722CC and E717230C, "not a slot, written over"; the look the game read, E709868C, 0xD9C40 below, without the row's shape
        // too: found by the pointer before it alone in the first search (the walk took 678.8 s). Strays at the camera's angles never nudged: no pointer
        // before them, off a line by 4 and 8. (b) The same look, the copies with the row's shape (and two more off the row's step, out of the sweep's
        // sight): a pointer alone never looked for, the look never nudged; only the row's shape taken (the two off the step, written over).
        constexpr uintptr_t kCopy = 0xE71722CC, kLook = 0xE709868C;
        const auto launch = [&](Heap& h, bool shapedCopies) {
            h.Add(0xE7000000, 0xE7400000);
            h.copies = {kCopy, kCopy + 0x40};
            if (shapedCopies)
                h.copies.push_back(kCopy + 0x5010), h.copies.push_back(kCopy - 0x6010);
            for (const uintptr_t at : h.copies) {
                if (shapedCopies)
                    h.MakeSlot(at, kYaw, kPitch);
                else
                    h.MakeLone(at, kYaw, kPitch);
            }
            h.readSlot = kLook;
            h.MakeLone(h.readSlot, kYaw, kPitch);
            h.SetWord(kCopy - 0xC + 0x2000, 7), h.Set(kCopy + 0x2000, kYaw), h.Set(kCopy + 0x200C, kPitch);  // on a line, no pointer
            h.MakeLone(kCopy + 0x3004, kYaw, kPitch), h.MakeLone(kCopy + 0x4008, kYaw, kPitch);          // off a line by 4 and 8
            h.Tick();
        };
        const std::vector<uintptr_t> reached = {kCopy, kCopy + 0x40};
        Heap a;
        launch(a, false);
        const auto beforeA = a.pages;
        Ever ea;
        aimrules::SweepTried sa, ra;
        const ReachOutcome oa = ReachSearch(a, reached, sa, ra, ea, true);
        const bool foundA = oa.found == kLook && oa.byReach && oa.pointerAlone && oa.copies == 2 && oa.shapedCopies == 0 && oa.proved == 1 && oa.at[0].kind == 1 &&
                            oa.at[0].Distance() == -0xD9C40 && WritesOnSlotsOnly(a) && AllAsBefore(a, beforeA);
        Heap b;
        launch(b, true);
        const auto beforeB = b.pages;
        Ever eb;
        aimrules::SweepTried sb, rb;
        const ReachOutcome ob = ReachSearch(b, reached, sb, rb, eb, true);
        int lookWritten = 0;
        for (const uintptr_t at : b.written)
            lookWritten += at == kLook || at == kLook + 0xC;
        bool onlyShaped = true;
        for (int n = 0; n < ob.proved; ++n)
            onlyShaped = onlyShaped && ob.at[n].kind == 2 && ob.answer[n] == Proof::kOverwritten;
        const bool notB = !ob.found && ob.reached && !ob.pointerAlone && ob.copies == 2 && ob.shapedCopies == 2 && ob.kind1 == 0 && ob.proved == 2 && onlyShaped &&
                          lookWritten == 0 && WritesOnSlotsOnly(b) && AllAsBefore(b, beforeB);
        Report("18", "a look without the row's shape: taken only where the copies lack it too", foundA && notB,
               "the .50 cal run's copies without the row's shape: found %08X by a pointer before it alone %d (%c0x%lX from %08X), a pointer alone looked for %d, writes "
               "on the look's slots only and undone %d; the copies with the row's shape: a pointer alone looked for %d, places with it counted %d, proved %d (the row's "
               "shape, written over, %d), the look nudged %d times, found %d",
               static_cast<unsigned>(oa.found), oa.byReach && oa.proved == 1 && oa.at[0].kind == 1, oa.proved ? (oa.at[0].Distance() < 0 ? '-' : '+') : '?',
               oa.proved ? static_cast<unsigned long>(std::labs(oa.at[0].Distance())) : 0ul, oa.proved ? static_cast<unsigned>(oa.at[0].anchor) : 0u, oa.pointerAlone,
               WritesOnSlotsOnly(a) && AllAsBefore(a, beforeA), ob.pointerAlone, ob.kind1, ob.proved, onlyShaped, lookWritten, ob.found != 0);
    }
    // 19
    {
        // Crowded: more than kReachMost places of a kind at the camera's angles. (a) The copy, the look and 17 more with the row's shape: none nudged by the
        // reach. (b) The copy without the row's shape (so a pointer alone is looked for) and 17 more with a pointer alone, the look with the row's shape:
        // those 18 left out, the look proved.
        const auto launch = [&](Heap& h, bool rowShape) {
            h.Add(0x30000000, 0x30400000);
            h.copies = {0x30100A4C};
            if (rowShape)
                h.MakeSlot(h.copies[0], kYaw, kPitch);
            else
                h.MakeLone(h.copies[0], kYaw, kPitch);
            h.readSlot = 0x30150A4C;
            h.MakeSlot(h.readSlot, kYaw, kPitch);
            for (int k = 0; k < 17; ++k) {
                const uintptr_t at = 0x30200A4C + k * 0x1000;
                if (rowShape)
                    h.MakeSlot(at, kYaw, kPitch);
                else
                    h.MakeLone(at, kYaw, kPitch);
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
        const bool left = ob.found == b.readSlot && ob.pointerAlone && ob.crowd == 1 && ob.kind1 == 18 && pointerNudged == 0;
        Report("19", "crowded: more than 16 of a kind, that kind never nudged", none && stillNudged == 0 && left,
               "%d with the row's shape: crowded %d, taken %d, nudged among them %d, found %d; 18 with a pointer alone and the look: crowded %d, the 17 nudged %d, the "
               "look proved %d",
               oa.kind2, oa.crowd, oa.taken, stillNudged, oa.found != 0, ob.crowd, pointerNudged, ob.found == b.readSlot);
    }
    // 20
    {
        // AMD and the rest of what's never read: the look in write-combined memory (0x404, as an RX 7900 XT's mapped buffers), uncached (0x204), behind a
        // guard page (0x104), in a mapped file, in a module, reserved only: never read, never found. In plain memory past a region that can't be read
        // midway (freed while it's read): found. A copy of the look's words in the reach's own allocation: never read. A thread's stack ([reserved][guard]
        // [committed], the look in its committed part): never read. One committed whole (no guard page): read where nothing names it, never where the
        // threads' list does.
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
        h.Add(0x30300000, 0x30380000);
        h.Add(0x30380000, 0x30390000);  // the reach's own allocation
        h.Add(0x30390000, 0x30400000);
        h.own.push_back(0x30380000);
        h.poisoned.push_back({0x30280000, 0x30290000});
        h.copies = {0x30100A4C};
        h.MakeSlot(h.copies[0], kYaw, kPitch);
        h.readSlot = 0x30350A4C;
        h.MakeSlot(h.readSlot, kYaw, kPitch);
        h.MakeSlot(0x30380A4C, kYaw, kPitch);  // a copy of its words in the reach's own allocation
        h.Tick();
        Ever ever;
        aimrules::SweepTried s1, r1;
        const ReachOutcome o = ReachSearch(h, h.copies, s1, r1, ever, true);
        int ownTaken = 0;
        for (int n = 0; n < o.proved; ++n)
            ownTaken += o.at[n].at == 0x30380A4C;
        // (2 with the row's shape: the copy and the look; the copy in its own allocation never seen.)
        const bool past = o.found == h.readSlot && ownTaken == 0 && o.kind2 == 2 && h.forbiddenReads == 0 && h.regionBytes[0x30380000] == 0;
        // A thread's stack, its guard page in its allocation.
        size_t stackRead = 1;
        bool stackFound = true;
        int stacks = 0;
        {
            Heap s;
            s.Add(0x30000000, 0x30200000);
            s.Add(0x30200000, 0x302F0000, 0, aimrules::kMemPrivate, 0x2000 /* MEM_RESERVE */, 0x30200000);
            s.Add(0x302F0000, 0x302F1000, 0x104, aimrules::kMemPrivate, aimrules::kMemCommit, 0x30200000);
            s.Add(0x302F1000, 0x30300000, aimrules::kPageReadWrite, aimrules::kMemPrivate, aimrules::kMemCommit, 0x30200000);
            s.copies = {0x30100A4C};
            s.MakeSlot(s.copies[0], kYaw, kPitch);
            s.readSlot = 0x302F8A4C;
            s.MakeSlot(s.readSlot, kYaw, kPitch);
            s.Tick();
            Ever e;
            aimrules::SweepTried t1, t2;
            const ReachOutcome so = ReachSearch(s, s.copies, t1, t2, e, true);
            stackRead = s.regionBytes[0x302F1000];
            stackFound = so.found != 0;
            stacks = so.totals.stacks;
        }
        // A stack committed whole: no guard page; named by the threads' list or not.
        bool wholeFound = false, wholeListed = false;
        for (int listed = 0; listed < 2; ++listed) {
            Heap s;
            s.Add(0x30000000, 0x30200000);
            s.Add(0x30200000, 0x30300000);
            if (listed)
                s.own.push_back(0x30200000);
            s.copies = {0x30100A4C};
            s.MakeSlot(s.copies[0], kYaw, kPitch);
            s.readSlot = 0x302F8A4C;
            s.MakeSlot(s.readSlot, kYaw, kPitch);
            s.Tick();
            Ever e;
            aimrules::SweepTried t1, t2;
            const ReachOutcome so = ReachSearch(s, s.copies, t1, t2, e, true);
            if (listed)
                wholeListed = !so.found && s.regionBytes[0x30200000] == 0;
            else
                wholeFound = so.found == s.readSlot;
        }
        const bool stacked = stackRead == 0 && !stackFound && stacks == 1 && wholeFound && wholeListed;
        Report("20", "AMD's write-combined memory, threads' stacks and the rest never read", untouched == 6 && past && stacked,
               "write-combined, uncached, a guard page, a mapped file, a module, reserved: untouched and not found %d of 6; past a region that can't be read midway "
               "found %d, its own allocation never read %d; a stack with its guard page: %zu bytes read, found %d, stacks told %d; one committed whole: found where "
               "nothing names it %d, never read where the threads' list does %d",
               untouched, o.found == h.readSlot, ownTaken == 0 && o.kind2 == 2 && h.regionBytes[0x30380000] == 0, stackRead, stackFound, stacks, wholeFound, wholeListed);
    }
    // 21
    {
        // ReachSpans: the order, rings nearest first; each span on lines, none overlapping another; the bytes; memory's ends; bad anchors.
        static aimrules::Span s[256];
        const auto sound = [](const aimrules::Span* sp, int n, size_t& total) {
            total = 0;
            for (int k = 0; k < n; ++k) {
                if ((sp[k].from & 0xF) || (sp[k].to & 0xF) || sp[k].to <= sp[k].from || sp[k].from < 0x10000 || sp[k].to > 0xFFFF0000u)
                    return false;
                for (int j = 0; j < k; ++j)
                    if (sp[k].from < sp[j].to && sp[j].from < sp[k].to)
                        return false;
                total += sp[k].to - sp[k].from;
            }
            return true;
        };
        const auto covered = [](const aimrules::Span* sp, int n, uintptr_t from, uintptr_t to) {
            uintptr_t sum = 0;
            for (int k = 0; k < n; ++k) {
                const uintptr_t a = std::max(sp[k].from, from), b = std::min(sp[k].to, to);
                if (b > a)
                    sum += b - a;
            }
            return sum == to - from;
        };
        // (How far along the order a byte is first looked at: the bytes of the spans before its own; past them all when none has it.)
        const auto orderOf = [](const aimrules::Span* sp, int n, uintptr_t at) {
            size_t before = 0;
            for (int k = 0; k < n; ++k) {
                if (at >= sp[k].from && at < sp[k].to)
                    return before + (at - sp[k].from);
                before += sp[k].to - sp[k].from;
            }
            return ~static_cast<size_t>(0);
        };
        const auto gapOf = [](const aimrules::Span& sp, uintptr_t a) -> uintptr_t { return a < sp.from ? sp.from - a : a >= sp.to ? a - sp.to + 1 : 0; };
        // a: one anchor: 64 KB about it first, then each ring's two sides; 32 MB; each span no nearer it than the one before (but for the edges' 16-byte
        // lines: a ring's side below begins its line a few bytes farther than the side above).
        const uintptr_t one[1] = {0xE7712CCC};
        const int n1 = aimrules::ReachSpans(one, 1, s, 256, aimrules::kReachBytesMost);
        size_t t1 = 0;
        bool outward = sound(s, n1, t1) && n1 == 7 && gapOf(s[0], one[0]) == 0 && t1 == 2 * aimrules::kReachEachWay &&
                       covered(s, n1, (one[0] - aimrules::kReachEachWay) & ~0xFu, (one[0] + aimrules::kReachEachWay) & ~0xFu);
        for (int k = 1; k < n1; ++k)
            outward = outward && gapOf(s[k], one[0]) + 0x20 >= gapOf(s[k - 1], one[0]);
        // b: two anchors 1 MB apart: each's 64 KB first, every byte of the two windows once (33 MB).
        const uintptr_t pair[2] = {0xE7712CCC, 0xE7612CCC};
        const int n2 = aimrules::ReachSpans(pair, 2, s, 256, aimrules::kReachBytesMost);
        size_t t2 = 0;
        const bool twoOk = sound(s, n2, t2) && gapOf(s[0], pair[0]) == 0 && gapOf(s[1], pair[1]) == 0 && t2 == 0x2100000 &&
                           covered(s, n2, (pair[1] - aimrules::kReachEachWay) & ~0xFu, (pair[0] + aimrules::kReachEachWay) & ~0xFu);
        // c: three far apart: each one's 5 MB before any one's 16 MB ring; the third's 0x40FA20 (the farthest look of the logs) looked through.
        const uintptr_t three[3] = {0x40000A4C, 0x60000A4C, 0x80000A4C};
        const int n3 = aimrules::ReachSpans(three, 3, s, 256, aimrules::kReachBytesMost);
        size_t t3 = 0;
        size_t fiveLast = 0, sixteenFirst = ~static_cast<size_t>(0);
        for (const uintptr_t a : three) {
            fiveLast = std::max({fiveLast, orderOf(s, n3, a + 0x4FFFF0), orderOf(s, n3, a - 0x4FFFF0)});
            sixteenFirst = std::min({sixteenFirst, orderOf(s, n3, a + 0x500010), orderOf(s, n3, a - 0x500010)});
        }
        const bool threeOk = sound(s, n3, t3) && t3 <= aimrules::kReachBytesMost && fiveLast < sixteenFirst && covered(s, n3, three[2] - 0x40FA20, three[2] + 0x40FA20);
        // d: sixteen far apart: each one's 1 MB, 64 MB at most.
        uintptr_t sixteen[16];
        for (int k = 0; k < 16; ++k)
            sixteen[k] = 0x10000A4C + static_cast<uintptr_t>(k) * 0x8000000;
        const int n16 = aimrules::ReachSpans(sixteen, 16, s, 256, aimrules::kReachBytesMost);
        size_t t16 = 0;
        bool each = sound(s, n16, t16) && t16 <= aimrules::kReachBytesMost;
        for (const uintptr_t a : sixteen)
            each = each && covered(s, n16, (a - 0x100000) & ~0xFu, (a + 0x100000) & ~0xFu);
        // e: memory's ends; f: nothing for bad anchors; g: the most: a second anchor cut about it to what's left.
        const uintptr_t ends[2] = {0x20000, 0xFFFE0000u};
        const int nEnds = aimrules::ReachSpans(ends, 2, s, 256, aimrules::kReachBytesMost);
        size_t tEnds = 0;
        bool low = false, high = false;
        for (int k = 0; k < nEnds; ++k)
            low = low || s[k].from == 0x10000, high = high || s[k].to == 0xFFFF0000u;
        const bool edge = sound(s, nEnds, tEnds) && low && high;
        const uintptr_t bad[3] = {0, 0x8000, 0xFFFF8000u};
        const bool nothing = aimrules::ReachSpans(bad, 3, s, 256, aimrules::kReachBytesMost) == 0 && aimrules::ReachSpans(nullptr, 3, s, 256, aimrules::kReachBytesMost) == 0;
        const uintptr_t cut[2] = {0x80000000, 0x90000000};
        const size_t cutMost = 0x2000000 + 0x1400000;
        const int nCut = aimrules::ReachSpans(cut, 2, s, 256, cutMost);
        size_t tCut = 0;
        const bool cutOk = sound(s, nCut, tCut) && tCut <= cutMost && covered(s, nCut, cut[0] - aimrules::kReachEachWay, cut[0] + aimrules::kReachEachWay) &&
                           covered(s, nCut, cut[1] - 0x500000, cut[1] + 0x500000);
        Report("21", "ReachSpans: rings nearest first, each byte once, the most", outward && twoOk && threeOk && each && edge && nothing && cutOk,
               "one anchor: %d spans, %.1f MB, outward %d; two 1 MB apart %d (%.1f MB); three far apart: each one's 5 MB before any 16 MB, the third's 4 MB looked "
               "through %d (%.1f MB); sixteen: each one's 1 MB %d (%.1f MB); memory's ends %d; bad anchors nothing %d; a second anchor cut about it %d (%.1f MB)",
               n1, t1 / 1048576.0, outward, twoOk, t2 / 1048576.0, threeOk, t3 / 1048576.0, each, t16 / 1048576.0, edge, nothing, cutOk, tCut / 1048576.0);
    }
    // 22
    {
        // The time on this program's own memory: 32 MB (random words, four looks at the camera's angles, one 0x40 above the anchor) looked through by
        // ReachThrough with aimmemory.h's GameMemory, as aim.cpp's ReachLook does (VirtualQuery, the working set, QueryPerformanceCounter): best of three.
        // Then with 3 ms to do it in: cut, the look nearest the anchor found all the same (up to three tries: this program runs at Idle beside others).
        constexpr size_t kBytes = 32u << 20;
        constexpr float kY = 1.7317f, kP = -0.4127f;  // (angles no other memory of this program holds)
        uint32_t* memory = static_cast<uint32_t*>(VirtualAlloc(nullptr, kBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        uint32_t* piece = static_cast<uint32_t*>(VirtualAlloc(nullptr, aimrules::kReachPieceWords * 4, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        int found = 0;
        double best = 1e9;
        size_t bytes = 0, cutBytes = 0;
        bool cut = false, nearFound = false;
        uint64_t cutMicros = 0;
        if (memory && piece) {
            std::mt19937 random(1118);
            for (size_t k = 0; k < kBytes / 4; ++k)
                memory[k] = random();
            const size_t looks[4] = {0x1000, 0x123450, 0x400010, 0x7FFFF0};  // (word indexes on 16-byte lines; the third 0x40 above the anchor's line)
            for (const size_t k : looks) {
                memory[k] = 0x30001000, memory[k + 1] = memory[k + 2] = memory[k + 4] = memory[k + 5] = 0;
                std::memcpy(&memory[k + 3], &kY, 4), std::memcpy(&memory[k + 6], &kP, 4);
            }
            const auto base = reinterpret_cast<uintptr_t>(memory);
            const uintptr_t anchor = base + kBytes / 2 + 0xC, nearest = base + 0x400010 * 4 + 0xC;
            static aimrules::Span spans[256];
            const int n = aimrules::ReachSpans(&anchor, 1, spans, 256, aimrules::kReachBytesMost);
            aimmemory::GameMemory m;
            m.Start();
            m.piece = piece;
            MEMORY_BASIC_INFORMATION mine{};
            VirtualQuery(piece, &mine, sizeof(mine));
            m.own.push_back(reinterpret_cast<uintptr_t>(mine.AllocationBase));
            for (int round = 0; round < 3; ++round) {
                aimrules::ReachMemo memo;
                std::vector<aimrules::Reached> places;
                const aimrules::ReachTotals t = aimrules::ReachThrough(spans, n, m, memo, kY, kP, false, places);
                best = std::min(best, t.micros / 1000.0);
                found = t.counts[2], bytes = t.bytes;
            }
            for (int tries = 0; tries < 3 && !(cut && nearFound); ++tries) {
                aimrules::ReachMemo memo;
                std::vector<aimrules::Reached> places;
                const aimrules::ReachTotals t = aimrules::ReachThrough(spans, n, m, memo, kY, kP, false, places, 3000);
                cut = t.cut, cutBytes = t.bytes, cutMicros = t.micros, nearFound = false;
                for (const aimrules::Reached& p : places)
                    nearFound = nearFound || p.at == nearest;
            }
        }
        if (memory)
            VirtualFree(memory, 0, MEM_RELEASE);
        if (piece)
            VirtualFree(piece, 0, MEM_RELEASE);
        // (Each piece's read, and each span's, goes 24 bytes past its end where the memory goes on, so the bytes read are a little over the 32 MB.)
        Report("22", "the time: 32 MB of this program's own memory through aimmemory.h", memory && piece && found == 4 && bytes >= kBytes &&
                                                                                              bytes <= kBytes + (kBytes / 0x10000 + 16) * 24 && best < 100.0 && cut &&
                                                                                              cutBytes < kBytes && nearFound,
               "the four looks found %d, %zu bytes read of %zu; best of three %.1f ms (%.0f MB a second); with 3 ms: cut %d after %.1f MB in %llu us, the look "
               "nearest the anchor found %d",
               found, bytes, kBytes, best, best > 0 ? 32.0 / (best / 1000.0) : 0.0, cut, cutBytes / 1048576.0, static_cast<unsigned long long>(cutMicros), nearFound);
    }
    // 23
    {
        // ReachChoice alone: the row's shape before a pointer alone, nearest an anchor first, above before below at one distance, each place once, a candidate
        // left out uncounted, one tried before left out and counted, the count past the four it gives; a crowd counted past what's kept; and the round again
        // as SweepTried's.
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
        // Six with the row's shape kept, twenty counted (ReachThrough keeps 17 of a kind): crowded, none taken; one with a pointer alone, 30 counted: those
        // left out, the row's shape taken.
        const int countsShaped[3] = {0, 1, 20}, countsLone[3] = {0, 30, 6};
        int crowdShaped = -1, crowdLone = -1;
        const bool counted = aimrules::ReachChoice(places, 9, anchors, 2, none, none, out, 8, nullptr, &crowdShaped, countsShaped) == 0 && crowdShaped == 2 &&
                             aimrules::ReachChoice(places, 9, anchors, 2, none, none, out, 8, nullptr, &crowdLone, countsLone) == 6 && crowdLone == 1;
        aimrules::SweepTried t;
        t.For(0x1000);
        t.Answered(0x40, aimrules::Answer::kOverwritten);
        int early = 0;
        for (int s = 0; s < aimrules::kSweepRest - 1; ++s)
            early += t.Chosen(0, 1);
        const bool round = !early && t.Chosen(0, 1) && t.places.empty();
        Report("23", "ReachChoice: the order, each once, left out, the count, a crowd counted; the round again", order && cap && broken && counted && round,
               "seven places: the row's shape nearest first, above before below, then the pointer's alone, each once %d (%d taken); a candidate uncounted, two tried "
               "before counted, four given %d (%d taken, %d left out); broken inputs %d; a crowd counted past what's kept %d; tried places forgotten at the fifth "
               "search with nothing to take %d",
               order, all, cap, capped, skipped, broken, counted, round);
    }
    // 24
    {
        // A PC five times slower than the user's (1.5 us a KB): the reach stops at 25 ms (checked before each 64 KB piece: within one piece of it). The
        // M4A1's look of 2026-11-19 (0x1D3880 from the copy the walk reached), here above it: found, nearest first (it lies in its anchor's third ring),
        // where a look through in memory's order from 16 MB below (as it was) is cut 1.9 MB short of it.
        constexpr uintptr_t kCopy = 0x42000A4C, kLook = kCopy + 0x1D3880;
        const auto launch = [&](Heap& h) {
            h.Add(0x40000000, 0x44000000, aimrules::kPageReadWrite, aimrules::kMemPrivate, aimrules::kMemCommit, 0, 1.5);
            h.copies = {kCopy};
            h.MakeSlot(kCopy, kYaw, kPitch);
            h.readSlot = kLook;
            h.MakeSlot(kLook, kYaw, kPitch);
            h.Tick();
        };
        Heap h;
        launch(h);
        Ever ever;
        aimrules::SweepTried s1, r1;
        const ReachOutcome o = ReachSearch(h, h.copies, s1, r1, ever, true);
        const bool nearest = o.found == kLook && o.byReach && o.totals.cut && o.totals.micros >= aimrules::kReachMicrosMost &&
                             o.totals.micros < aimrules::kReachMicrosMost + 120;
        Heap old;
        launch(old);
        HeapMemory memory(old);
        aimrules::ReachMemo memo;
        std::vector<aimrules::Reached> places;
        const aimrules::Span inOrder{(kCopy - aimrules::kReachEachWay) & ~static_cast<uintptr_t>(0xF), (kCopy + aimrules::kReachEachWay) & ~static_cast<uintptr_t>(0xF)};
        const aimrules::ReachTotals t = aimrules::ReachThrough(&inOrder, 1, memory, memo, old.cameraYaw, old.cameraPitch, false, places);
        bool lookSeen = false;
        for (const aimrules::Reached& p : places)
            lookSeen = lookSeen || p.at == kLook;
        Report("24", "the time: 25 ms at most, nearest first", nearest && t.cut && !lookSeen,
               "five times slower: found %08X by the reach %d, its time up %d after %.1f MB in %llu us (25000 at most, within a piece); in memory's order from 16 MB "
               "below: its time up %d after %.1f MB, the look seen %d",
               static_cast<unsigned>(o.found), o.byReach, o.totals.cut, o.totals.bytes / 1048576.0, static_cast<unsigned long long>(o.totals.micros), t.cut,
               t.bytes / 1048576.0, lookSeen);
    }
    // 25
    {
        // Memory called plain read-write that reads as AMD's write-combined memory does (180 us a KB: 11.5 ms a 64 KB piece), between the copy and the
        // look. (a) Its first piece read twice, slow both times: its allocation left for the rest of the search and struck once; the look past it found in
        // the same search. (b) Searches with no look in reach: at the second the allocation is left for the launch, and the third never reads it. (c) Plain
        // memory with one read 15 ms long (a thread switched out in the middle): read again, nothing struck, the look found.
        const auto launch = [&](Heap& h, bool look) {
            h.Add(0x30000000, 0x30200000);
            h.Add(0x30200000, 0x30300000, aimrules::kPageReadWrite, aimrules::kMemPrivate, aimrules::kMemCommit, 0, 180.0);
            h.Add(0x30300000, 0x30400000);
            h.Add(0x38000000, 0x38100000);  // (where the look lies when none is in reach: 126 MB off)
            h.copies = {0x30180A4C};
            h.MakeSlot(h.copies[0], kYaw, kPitch);
            h.readSlot = look ? 0x30310A4C : 0x38000A4C;
            h.MakeSlot(h.readSlot, kYaw, kPitch);
            h.Tick();
        };
        Heap a;
        launch(a, true);
        aimrules::ReachMemo memoA;
        Ever ea;
        aimrules::SweepTried sa, ra;
        const ReachOutcome oa = ReachSearch(a, a.copies, sa, ra, ea, true, 0x1000, &memoA);
        const bool pastA = oa.found == a.readSlot && oa.byReach && oa.totals.slowNow == 1 && oa.totals.slowAt == 0x30200000 && !memoA.Slow(0x30200000) &&
                           a.regionBytes[0x30200000] == 2 * aimrules::kReachPieceWords * 4 && !oa.totals.cut;
        Heap b;
        launch(b, false);
        aimrules::ReachMemo memoB;
        Ever eb;
        aimrules::SweepTried sb, rb;
        const ReachOutcome b1 = ReachSearch(b, b.copies, sb, rb, eb, true, 0x1000, &memoB);
        const size_t afterFirst = b.regionBytes[0x30200000];
        const bool struckOnce = !memoB.Slow(0x30200000);
        const ReachOutcome b2 = ReachSearch(b, b.copies, sb, rb, eb, true, 0x1000, &memoB);
        const size_t afterSecond = b.regionBytes[0x30200000];
        const ReachOutcome b3 = ReachSearch(b, b.copies, sb, rb, eb, true, 0x1000, &memoB);
        const bool launchLeft = b1.totals.slowNow == 1 && struckOnce && b2.totals.slowNow == 1 && memoB.Slow(0x30200000) && b3.totals.slowNow == 0 &&
                                b3.totals.slowLeft >= 1 && b.regionBytes[0x30200000] == afterSecond && afterSecond == 2 * afterFirst && b3.totals.micros < 2000 &&
                                !b1.found && !b3.found;
        Heap c;
        c.Add(0x30000000, 0x30400000);
        c.copies = {0x30100A4C};
        c.MakeSlot(c.copies[0], kYaw, kPitch);
        c.readSlot = 0x30180A4C;
        c.MakeSlot(c.readSlot, kYaw, kPitch);
        c.Tick();
        c.preemptBig = 1;
        aimrules::ReachMemo memoC;
        Ever ec;
        aimrules::SweepTried sc, rc;
        const ReachOutcome oc = ReachSearch(c, c.copies, sc, rc, ec, true, 0x1000, &memoC);
        const bool once = oc.found == c.readSlot && oc.totals.slowNow == 0 && memoC.slow.empty() && c.preemptBig == 0;
        Report("25", "memory slower than RAM: left for the search, then the launch; slow once is no strike", pastA && launchLeft && once,
               "a piece read twice slowly, its allocation left and the look past it found in the same search %d (%llu us); searches with none in reach: left for the "
               "launch at the second %d, the third never reads it %d (%llu us); a piece slow once read again, nothing struck, the look found %d",
               pastA, static_cast<unsigned long long>(oa.totals.micros), b2.totals.slowNow == 1 && memoB.Slow(0x30200000),
               b.regionBytes[0x30200000] == afterSecond && b3.totals.slowLeft >= 1, static_cast<unsigned long long>(b3.totals.micros), once);
    }
    // 26
    {
        // The working set: (a) the look's page out of it: never read (not by the reach, not by Proves), not taken; (b) the pages on both sides of the look's
        // out of it: the look found, neither read; (c) a block ending where a page out of it begins: found; (d) the working set not known: every page read,
        // the look in a page out of it found, as before.
        const auto launch = [&](Heap& h, uintptr_t look) {
            h.Add(0x30000000, 0x30400000);
            h.copies = {0x30100A4C};
            h.MakeSlot(h.copies[0], kYaw, kPitch);
            h.readSlot = look;
            h.MakeSlot(look, kYaw, kPitch);
            h.Tick();
        };
        const auto search = [&](Heap& h) {
            Ever e;
            aimrules::SweepTried s1, r1;
            return ReachSearch(h, h.copies, s1, r1, e, true);
        };
        Heap a;
        launch(a, 0x30120A4C);
        a.out.insert(0x30120000);
        const ReachOutcome oa = search(a);
        const bool outA = !oa.found && oa.reached && a.outReads == 0 && oa.totals.notResident >= 0x1000;
        Heap b;
        launch(b, 0x30120A4C);
        b.out.insert(0x3011F000), b.out.insert(0x30121000);
        const ReachOutcome ob = search(b);
        const bool betweenB = ob.found == b.readSlot && b.outReads == 0 && ob.totals.notResident >= 0x2000;
        Heap c;
        launch(c, 0x30121FEC);  // (its seven words from 0x30121FE0 to the page's last)
        c.out.insert(0x30122000);
        const ReachOutcome oc = search(c);
        const bool edgeC = oc.found == c.readSlot && c.outReads == 0;
        Heap d;
        launch(d, 0x30120A4C);
        d.out.insert(0x30120000);
        d.workingSetKnown = false;
        const ReachOutcome od = search(d);
        const bool asBefore = od.found == d.readSlot && d.outReads > 0 && od.totals.notResident == 0;
        Report("26", "the working set: a page out of it never read", outA && betweenB && edgeC && asBefore,
               "the look's page out: found %d, words read from it %zu, %.1f KB left; the pages beside it out: found %d, read %zu; a block ending at a page out: found %d; "
               "the working set not known: found %d, every page read %d",
               oa.found != 0, a.outReads, oa.totals.notResident / 1024.0, ob.found == b.readSlot, b.outReads, edgeC, od.found == d.readSlot, d.outReads > 0);
    }
    // 27
    {
        // This program's own pages and threads, through aimmemory.h as aim.cpp reads the game's. (a) 1 MB: the first 32 pages written, a look on page 10
        // and one on page 20, page 20 put out of the working set (VirtualUnlock on a page not locked), the rest never touched: the look on page 10 found,
        // page 20's not, and no page out of the working set before is in it after (none paged back in, none made). (b) Two threads, each holding a look on its
        // own stack: one with the default stack (its guard page), one committed whole (asked for 1 MB, the reserve's size: no guard page): with the
        // threads' list (AddThreadStacks) neither taken, both named; without the list, the guard page still keeps the first out and the one committed whole
        // is taken (the list is what keeps it out).
        constexpr float kY = 2.0911f, kP = 0.3517f;  // (angles no other memory of this program holds)
        constexpr size_t kSize = 1 << 20;
        uint8_t* block = static_cast<uint8_t*>(VirtualAlloc(nullptr, kSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        uint32_t* piece = static_cast<uint32_t*>(VirtualAlloc(nullptr, aimrules::kReachPieceWords * 4, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        bool outBefore = false, page10 = false, page20 = false, noneIn = false;
        size_t notResident = 0;
        uintptr_t ownPiece = 0;
        if (piece) {
            MEMORY_BASIC_INFORMATION mine{};
            VirtualQuery(piece, &mine, sizeof(mine));
            ownPiece = reinterpret_cast<uintptr_t>(mine.AllocationBase);
        }
        const auto putLook = [&](uintptr_t line) {
            uint32_t* w = reinterpret_cast<uint32_t*>(line);
            w[0] = 0x30001000, w[1] = w[2] = w[4] = w[5] = 0;
            std::memcpy(&w[3], &kY, 4), std::memcpy(&w[6], &kP, 4);
        };
        if (block && piece) {
            const auto at = reinterpret_cast<uintptr_t>(block);
            for (size_t k = 0; k < 32 * 0x1000; k += 4)
                *reinterpret_cast<volatile uint32_t*>(block + k) = 0x11111111;
            putLook(at + 10 * 0x1000 + 0x100), putLook(at + 20 * 0x1000 + 0x100);
            VirtualUnlock(block + 20 * 0x1000, 0x1000);  // (not locked: it leaves the working set)
            bool in[256] = {};
            for (int k = 0; k < 256; ++k)
                in[k] = aimmemory::InWorkingSet(at + static_cast<uintptr_t>(k) * 0x1000);
            outBefore = !in[20];
            aimmemory::GameMemory m;
            m.Start();
            m.piece = piece;
            m.own.push_back(ownPiece);
            const aimrules::Span span{at, at + kSize};
            aimrules::ReachMemo memo;
            std::vector<aimrules::Reached> places;
            const aimrules::ReachTotals t = aimrules::ReachThrough(&span, 1, m, memo, kY, kP, false, places);
            for (const aimrules::Reached& p : places)
                page10 = page10 || p.at == at + 10 * 0x1000 + 0x10C, page20 = page20 || p.at == at + 20 * 0x1000 + 0x10C;
            noneIn = true;
            for (int k = 0; k < 256; ++k)
                noneIn = noneIn && (in[k] || !aimmemory::InWorkingSet(at + static_cast<uintptr_t>(k) * 0x1000));
            notResident = t.notResident;
        }
        // (b)
        StackHold holds[2];
        HANDLE threads[2] = {};
        uintptr_t allocation[2] = {}, allocationEnd[2] = {};
        bool guard[2] = {}, listed[2] = {}, takenWith[2] = {true, true}, takenWithout[2] = {};
        int stacksTold = 0, listedCount = 0;
        uint64_t listMicros = 0;
        if (piece) {
            for (int k = 0; k < 2; ++k) {
                holds[k].yaw = kY, holds[k].pitch = kP;
                holds[k].ready = CreateEventW(nullptr, TRUE, FALSE, nullptr);
                holds[k].done = CreateEventW(nullptr, TRUE, FALSE, nullptr);
                threads[k] = CreateThread(nullptr, k ? 0x100000 : 0, HoldOnStack, &holds[k], 0, nullptr);
            }
            for (int k = 0; k < 2; ++k)
                if (threads[k])
                    WaitForSingleObject(holds[k].ready, 10000);
            for (int k = 0; k < 2; ++k) {
                MEMORY_BASIC_INFORMATION m{};
                if (!holds[k].at || VirtualQuery(reinterpret_cast<LPCVOID>(holds[k].at), &m, sizeof(m)) != sizeof(m))
                    continue;
                allocation[k] = reinterpret_cast<uintptr_t>(m.AllocationBase);
                for (uintptr_t q = allocation[k]; VirtualQuery(reinterpret_cast<LPCVOID>(q), &m, sizeof(m)) == sizeof(m) &&
                                                  reinterpret_cast<uintptr_t>(m.AllocationBase) == allocation[k];
                     q = reinterpret_cast<uintptr_t>(m.BaseAddress) + m.RegionSize) {
                    guard[k] = guard[k] || (m.Protect & PAGE_GUARD) != 0;
                    allocationEnd[k] = reinterpret_cast<uintptr_t>(m.BaseAddress) + m.RegionSize;
                }
            }
            const aimrules::Span stacks[2] = {{allocation[0], allocationEnd[0]}, {allocation[1], allocationEnd[1]}};
            for (int withList = 1; withList >= 0; --withList) {
                aimmemory::GameMemory m;
                m.Start();
                m.piece = piece;
                m.own.push_back(ownPiece);
                if (withList) {
                    const uint64_t from = m.Micros();
                    listedCount = aimmemory::AddThreadStacks(m.own);
                    listMicros = m.Micros() - from;
                }
                aimrules::ReachMemo memo;
                std::vector<aimrules::Reached> places;
                const aimrules::ReachTotals t = aimrules::ReachThrough(stacks, allocation[0] && allocation[1] ? 2 : 0, m, memo, kY, kP, false, places);
                for (int k = 0; k < 2; ++k) {
                    bool taken = false;
                    for (const aimrules::Reached& p : places)
                        taken = taken || p.at == holds[k].at + 0xC;
                    (withList ? takenWith : takenWithout)[k] = taken;
                    if (withList)
                        listed[k] = m.Own(allocation[k]);
                }
                if (!withList)
                    stacksTold = t.stacks;
            }
            for (int k = 0; k < 2; ++k) {
                if (holds[k].done)
                    SetEvent(holds[k].done);
                if (threads[k]) {
                    WaitForSingleObject(threads[k], 10000);
                    CloseHandle(threads[k]);
                }
                if (holds[k].ready)
                    CloseHandle(holds[k].ready);
                if (holds[k].done)
                    CloseHandle(holds[k].done);
            }
        }
        if (block)
            VirtualFree(block, 0, MEM_RELEASE);
        if (piece)
            VirtualFree(piece, 0, MEM_RELEASE);
        const bool pages = block && piece && outBefore && page10 && !page20 && noneIn && notResident >= 224 * 0x1000;
        const bool stacksOk = allocation[0] && allocation[1] && listed[0] && listed[1] && !takenWith[0] && !takenWith[1] && guard[0] && !takenWithout[0] &&
                              (guard[1] || takenWithout[1]);
        Report("27", "this program's own pages and threads through aimmemory.h", pages && stacksOk,
               "page 20 out of the working set before %d; the look on page 10 found %d, on page 20 %d; no page out before in after %d; %.1f KB left out of the set; "
               "stacks: guard pages %d %d, named by the threads' list %d %d (%d stacks listed in %llu us), taken with it %d %d, without it %d %d (stacks told by "
               "their guard page %d)",
               outBefore, page10, page20, noneIn, notResident / 1024.0, guard[0], guard[1], listed[0], listed[1], listedCount,
               static_cast<unsigned long long>(listMicros), takenWith[0], takenWith[1], takenWithout[0], takenWithout[1], stacksTold);
    }
    // 28
    {
        // The cap per kind (ReachThrough keeps 17 of a kind, counts every one): (a) the copy without the row's shape (a pointer alone looked for), 300 more
        // with a pointer alone at the camera's angles, and three with the row's shape: two copies off the row's step and the look: all three kept, proved
        // nearest first, the look found; the pointer's crowded, none of them nudged. (b) 250 with a pointer alone and 20 with the row's shape that would
        // keep a nudge: both kinds crowded, none of them nudged.
        constexpr uintptr_t kCopy = 0x30400A4C;
        Heap a;
        a.Add(0x30000000, 0x30800000);
        a.copies = {kCopy, kCopy + 0x2010, kCopy - 0x3010};
        a.MakeLone(kCopy, kYaw, kPitch);
        a.MakeSlot(kCopy + 0x2010, kYaw, kPitch), a.MakeSlot(kCopy - 0x3010, kYaw, kPitch);
        for (int k = 0; k < 300; ++k)
            a.MakeLone(0x30500A4C + static_cast<uintptr_t>(k) * 0x100, kYaw, kPitch);
        a.readSlot = 0x30600A4C;
        a.MakeSlot(a.readSlot, kYaw, kPitch);
        a.Tick();
        Ever ea;
        aimrules::SweepTried sa, ra;
        const ReachOutcome oa = ReachSearch(a, {kCopy}, sa, ra, ea, true);
        int loneNudged = 0;
        for (const uintptr_t at : a.written)
            loneNudged += at >= 0x30500A4C && at < 0x30500A4C + 300 * 0x100;
        const bool keptA = oa.found == a.readSlot && oa.pointerAlone && oa.kind1 == 301 && oa.kind2 == 3 && oa.crowd == 1 && oa.proved == 3 && loneNudged == 0;
        Heap b;
        b.Add(0x30000000, 0x30800000);
        b.Add(0x38000000, 0x38100000);
        b.copies = {kCopy};
        b.MakeLone(kCopy, kYaw, kPitch);
        for (int k = 0; k < 250; ++k)
            b.MakeLone(0x30500A4C + static_cast<uintptr_t>(k) * 0x100, kYaw, kPitch);
        for (int k = 0; k < 20; ++k)
            b.MakeSlot(0x30600A4C + static_cast<uintptr_t>(k) * 0x100, kYaw, kPitch);
        b.readSlot = 0x38000A4C;  // (the look out of reach)
        b.MakeSlot(b.readSlot, kYaw, kPitch);
        b.Tick();
        Ever eb;
        aimrules::SweepTried sb, rb;
        const ReachOutcome ob = ReachSearch(b, {kCopy}, sb, rb, eb, true);
        int decoys = 0;
        for (const uintptr_t at : b.written)
            decoys += at >= 0x30500A4C && at < 0x30700000;
        const bool crowdB = !ob.found && ob.crowd == 3 && ob.taken == 0 && decoys == 0 && ob.kind1 == 251 && ob.kind2 == 20;
        Report("28", "the cap per kind: every one counted, 17 of a kind kept", keptA && crowdB,
               "301 with a pointer alone and 3 with the row's shape: counted %d and %d, crowded %d, proved %d, the look found %d, a pointer alone nudged %d; 251 and 20: "
               "counted %d and %d, crowded %d, taken %d, nudged %d",
               oa.kind1, oa.kind2, oa.crowd, oa.proved, oa.found == a.readSlot, loneNudged, ob.kind1, ob.kind2, ob.crowd, ob.taken, decoys);
    }

    std::printf("%d of %d checks failed\n", g_failed, g_counted);
    return g_failed;
}

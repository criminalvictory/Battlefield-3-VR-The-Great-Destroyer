#pragma once

// The look angles' search rules (aim.cpp runs them on the game's memory) and a held gun's bullets' line (skin.cpp), header only: no game memory and
// no Windows calls, so tools/aimrulestest.cpp runs them as they are. Each rule "with find" is weapon.txt "aimfind" on (the default); off, every rule
// gives what aim.cpp did before it ("aimfind 0").

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <unordered_set>
#include <vector>

namespace aimrules {

constexpr float kPi = 3.14159265f;

// A float that could be a look angle in radians: finite, the pitch within a quarter turn, the yaw
// within a thousand (it may carry whole turns), and not a pointer's bits (those read as huge, or
// as tiny numbers no angle has).
inline bool LooksLikeAngle(float v, bool pitch) {
    if (!std::isfinite(v))
        return false;
    const float size = std::fabs(v);
    return (size == 0.0f || size >= 1e-6f) && size <= (pitch ? 1.6f : 1000.0f);
}

inline float YawApart(float a, float b) {
    float d = std::fmod(a - b, 2.0f * kPi);
    if (d > kPi)
        d -= 2.0f * kPi;
    if (d < -kPi)
        d += 2.0f * kPi;
    return std::fabs(d);
}

// As the angles are found, less than this between the camera and the look is the two read a hair apart, not a vehicle: none, exactly. Taken as it came (the last route's "0 degrees off the
// camera's yaw"), it was never quite none, so the follower ran on foot, and what it followed went into the picture but not the shots: the whole world shook as the gun moved, and the
// sights lay off the bullets by up to a degree or two, on or off by how the angles had been found that run (the user's jitter and "sometimes it works", 2026-11-04; the 18:21 run, found by the
// whole search, had neither).
constexpr float kNoVehicle = 1.5f / 57.2957795f;
inline float VehicleTurn(float off) {
    return std::fabs(off) < kNoVehicle ? 0.0f : off;
}

// ---- The walk (aim.cpp's Pairs) ----

constexpr float kStrict = 0.01f;     // radians: a pair and the camera read at one moment
constexpr float kNear = 0.05f;       // the walk's, the camera read again at each window: what the look turns in a game tick (33 ms) at 87 degrees a second
constexpr float kNotZero = 0.02f;    // a zero: a null word as often as an angle (Proves never nudges one)
constexpr size_t kMostNear = 4096;   // the walk's near list at most (it stops adding there)

// The walk's own test, as it stood before this round, and the one a pair passes again at one instant (aim.cpp's Matching): its yaw a plausible angle
// within kStrict of the camera's (with anyYaw: any yaw from 0.02 to 7), its pitch within kStrict of the camera's.
inline bool StrictPair(float yaw, float pitch, float cameraYaw, float cameraPitch, bool anyYaw) {
    return LooksLikeAngle(yaw, false) && (anyYaw ? std::fabs(yaw) >= 0.02f && std::fabs(yaw) < 7.0f : YawApart(yaw, cameraYaw) < 0.01f) &&
           std::fabs(pitch - cameraPitch) < 0.01f;
}

// The walk's test with the camera read again just after each window (with find): a pair and the camera a moment apart differ by what the look turns
// between the game's update of the slot and its next camera, a tick at most, so kNear; both angles plausible and neither a zero (null words would swell
// the list, and Proves never nudges a zero). With anyYaw: the pitch alone, any yaw from 0.02 to 7; the pass by the pitch alone asks within kStrict (its
// one window, the pitch's, is 0.1 wide at kNear with any yaw beside it, and a full list would cut its walk short).
inline bool NearPair(float yaw, float pitch, float cameraYaw, float cameraPitch, bool anyYaw, float within = kNear) {
    if (!LooksLikeAngle(yaw, false) || !LooksLikeAngle(pitch, true) || std::fabs(yaw) < kNotZero || std::fabs(pitch) < kNotZero ||
        !(std::fabs(pitch - cameraPitch) < within))
        return false;
    return anyYaw ? std::fabs(yaw) < 7.0f : YawApart(yaw, cameraYaw) < within;
}

// ---- The pass by the pitch alone ----

// Slot-shaped pairs at the camera's own yaw, at one instant, that say no vehicle turns the look: the game copies the slot it reads over the others at
// each of its ticks (the five slots), so two or more of them there are the look's copies, at the camera's yaw. On a moving vehicle (The Great Destroyer's
// train) the copies hold the player's own yaw, not the camera's: none there.
constexpr int kCopiesSeen = 2;
// Whether the pass by the pitch alone runs at all (the camera's pitch clear of level is aim.cpp's own test): never crowded, as before; with find, not
// when the first pass saw the look's copies at the camera's yaw (shapedAtCamera, -1 when it didn't look).
inline bool PitchPassWanted(int shapedAtCamera, bool crowded, bool find) {
    if (crowded)
        return false;
    return !find || shapedAtCamera < kCopiesSeen;
}
// How long it may hold mouse aim in one search (with find): it went through all 16 of its candidates in 50 of the 86 searches of the user's try of
// 2026-11-13, holding the game's view still 0.5 to 5 s each time (143 s in all). It goes on from the next candidate in the next search.
constexpr uint64_t kPitchHoldMost = 600;  // ms
inline bool PitchPassGoesOn(uint64_t heldMs, int triedThisPass) {
    return triedThisPass == 0 || heldMs < kPitchHoldMost;
}

// ---- The row swept from the copies written over (aim.cpp's Search, with find) ----

// The angles come in a row of slots kSlotStep apart (aim.cpp: g_slots); they aren't always side by side: on two of three loads scanned, the copies were
// 0x300 to 0x880 from the slot the game read.
constexpr uintptr_t kSlotStep = 0x40;
constexpr int kSlotsEachWay = 36;

// A slot of the row by its seven words, from three words before its yaw: a pointer's shape, two zeros, the yaw, two zeros, the pitch (2026-11-04, in
// memory), two plausible angles.
inline bool SlotWords(const uint32_t* words, float& yaw, float& pitch) {
    std::memcpy(&yaw, &words[3], 4);
    std::memcpy(&pitch, &words[6], 4);
    return words[0] >= 0x10000 && words[0] < 0xFFFF0000u && !(words[0] & 3) && !words[1] && !words[2] && !words[4] && !words[5] && LooksLikeAngle(yaw, false) &&
           LooksLikeAngle(pitch, true);
}

// In about 2 of 7 launches at one checkpoint the walk reached three of the row's slots and not the one the game read (2026-11-18: "3 of those at one
// instant slot-shaped", each written over within 50 ms, the same line every 3 s for the whole launch; the launches that found it saw 4, the 4th
// proving): the shots then went where the head looked. So when the first pass ends with nothing found and a slot-shaped candidate of it was written
// over, the row is looked through from each such copy, kSlotsEachWay each way, and what's taken there is proved as any candidate is. Only with find,
// never crowded.
inline bool SweepWanted(bool find, bool found, bool crowded, int shapedWrittenOver) {
    return find && !found && !crowded && shapedWrittenOver > 0;
}
// What the sweep takes, by a place's seven words (SlotWords) and the camera read at that instant: the row's shape, the walk's own test against the
// camera (StrictPair), and neither angle a zero (as FindSlots: just before the row there's a pointer followed by nulls, and a float over a pointer
// crashed the game once).
inline bool SweepTakes(const uint32_t* words, float cameraYaw, float cameraPitch) {
    float yaw = 0.0f, pitch = 0.0f;
    return SlotWords(words, yaw, pitch) && StrictPair(yaw, pitch, cameraYaw, cameraPitch, false) && !(std::fabs(pitch) < kNotZero) &&
           !(YawApart(yaw, 0.0f) < kNotZero);
}
constexpr int kSweepMost = 4;  // proved in one search at most (each holds mouse aim 0.2 to 0.5 s); the rest in the next
// How long the sweep's proofs may hold mouse aim in one search, on top of the first pass's: as the pass by the pitch alone (kPitchHoldMost: the first
// always, the next only while the proofs before it took less); those left are taken again by the next search.
inline bool SweepGoesOn(uint64_t heldMs, int provedThisSearch) {
    return PitchPassGoesOn(heldMs, provedThisSearch);
}
// A candidate of the first pass after its answer: the slot's shape or not, written over or not.
struct Candidate {
    uintptr_t at = 0;
    bool slot = false;
    bool writtenOver = false;
};
// The copies the row is swept from: the first pass's candidates with the slot's shape whose nudge was written over (the game rebuilding each from the
// slot it reads), in the pass's order, most at most. One without the shape (the fourth of the real logs: "not a slot, kept the nudge"), or a
// slot-shaped one not written over (kept, refused, the camera moving) is none.
inline int SweepCopies(const Candidate* tried, int count, uintptr_t* out, int most) {
    int copies = 0;
    for (int k = 0; k < count && copies < most; ++k)
        if (tried[k].slot && tried[k].writtenOver)
            out[copies++] = tried[k].at;
    return copies;
}
struct Swept {
    uintptr_t at = 0;    // the place taken
    uintptr_t copy = 0;  // the written-over copy it was reached from (the nearest of them)
    long Distance() const { return static_cast<long>(static_cast<intptr_t>(at) - static_cast<intptr_t>(copy)); }
};
// The places to prove, nearest their copy first (above it before below), each once: read(at, words) gives the seven words from 0xC before at (false:
// not readable), candidate(at) says a candidate of this pass (it had its answer there: never taken, never counted), before(at) a place swept before
// for this soldier. Up to most into out; returns how many it took in all (those past most are left for the next search), skipped: how many before
// refused that the sweep would have taken (so none taken and none skipped is the slot out of the sweep's reach, whatever the pass's candidates).
template <class Read, class IsCandidate, class Before>
int SweepChoice(const uintptr_t* copies, int count, float cameraYaw, float cameraPitch, Read&& read, IsCandidate&& candidate, Before&& before, Swept* out, int most,
                int* skipped = nullptr) {
    constexpr int kSeenMost = 64;  // (a row has a handful; past this nothing more is counted)
    uintptr_t seen[kSeenMost] = {};
    int seenCount = 0, taken = 0, refused = 0;
    if (!std::isfinite(cameraYaw) || !std::isfinite(cameraPitch))
        count = 0;
    for (int k = 1; k <= kSlotsEachWay; ++k)
        for (int sign = 1; sign >= -1; sign -= 2)
            for (int c = 0; c < count && seenCount < kSeenMost; ++c) {
                if (copies[c] < 0x10000 + kSlotsEachWay * kSlotStep || copies[c] > UINTPTR_MAX - kSlotsEachWay * kSlotStep - 0x40)
                    continue;
                const uintptr_t at = copies[c] + static_cast<uintptr_t>(static_cast<intptr_t>(sign * k) * static_cast<intptr_t>(kSlotStep));
                bool again = false;  // a copy itself, or a place already looked at from a nearer copy
                for (int n = 0; n < count; ++n)
                    again = again || copies[n] == at;
                for (int n = 0; n < seenCount; ++n)
                    again = again || seen[n] == at;
                uint32_t words[7] = {};
                if (again || candidate(at) || !read(at, words) || !SweepTakes(words, cameraYaw, cameraPitch))
                    continue;
                seen[seenCount++] = at;
                if (before(at)) {
                    ++refused;
                    continue;
                }
                if (taken < most)
                    out[taken] = Swept{at, copies[c]};
                ++taken;
            }
    if (skipped)
        *skipped = refused;
    return taken;
}

// The vehicle's turn as a proof measures it (with find): the camera, held still by mouse aim, less the pair it's built from, read at the same moment.
// The pass by the pitch alone took the camera's yaw at its start and the pair's at its end, 0.3 to 4.4 s apart, and a gun turning meanwhile gave a false
// turn of 14 and 12 degrees on foot (the user's try, 10:39:06 and 10:40:54). Not finite: none.
inline float ProofTurn(float cameraYaw, float pairYaw) {
    const float off = std::remainder(cameraYaw - pairYaw, 2.0f * kPi);
    return std::isfinite(off) ? VehicleTurn(off) : 0.0f;
}

// ---- After a loss: what proved itself, tried again first (Search's quick path) ----

// The addresses that proved themselves for the player's soldier, newest first, each with the take it came after (pickups' newest swap when its search
// began), the gun its proof counts for (ProvedWith: the game's own current weapon's object, ammo::SlotIdentity: each of the soldier's guns reads a look
// of its own; 0 not read) and its strikes in a row. A new soldier forgets them. Search's own thread.
constexpr int kProvenMost = 4;
// The gun a proof counts for: the gun in hand as the proof began and as it ended (each as the render thread last read it), the same: that gun. Not the
// gun as the search began: in the user's play of 2026-11-19 a search began at 01:09:11.87 with the M4A1 in hand, the switch to the Mk11 came at
// 01:09:12.123 and the Mk11's look (E6F81F4C, a copy written over while the M4A1 was in hand) was proved at 01:09:12.679: kept as the M4A1's, a switch
// back to the M4A1 would have tried it first and waited for it (up to 15 s). Changed between the two (a switch inside the proof): kWeaponUnsure, which
// no gun read waits for (Proven::OtherWeapon) and each tries after its own. No gun read at either: 0, as before.
constexpr uintptr_t kWeaponUnsure = 1;  // (no gun's object lies at 1)
inline uintptr_t ProvedWith(uintptr_t atStart, uintptr_t atEnd) {
    return atStart == atEnd ? atStart : kWeaponUnsure;
}
struct Proven {
    struct Entry {
        uintptr_t address = 0;
        uint32_t swap = 0;
        int strikes = 0;
        uintptr_t weapon = 0;
    };
    uintptr_t soldier = 0;
    Entry entry[kProvenMost] = {};  // newest first; address 0 past the last

    void For(uintptr_t s) {
        if (s != soldier) {
            *this = Proven{};
            soldier = s;
        }
    }
    // Proved just now: to the front (taken from where it was, else the oldest dropped), its strikes none, with the gun its proof counts for (ProvedWith).
    void Remember(uintptr_t s, uintptr_t address, uint32_t swap, uintptr_t weapon = 0) {
        For(s);
        if (!address)
            return;
        int at = kProvenMost - 1;
        for (int k = 0; k < kProvenMost; ++k)
            if (entry[k].address == address) {
                at = k;
                break;
            }
        for (int k = at; k > 0; --k)
            entry[k] = entry[k - 1];
        entry[0] = Entry{address, swap, 0, weapon};
    }
    // The order a quick try goes through them: those proved with the gun now in hand first (the newest first), then the others as they stand. A gun not
    // read (0), now or at the proof, counts as the same one; one proved across a switch (kWeaponUnsure) as another gun. Returns how many it put in order
    // (Count()).
    int Order(uintptr_t weapon, int* order) const {
        int n = 0;
        const int count = Count();
        for (int pass = 0; pass < 2; ++pass)
            for (int k = 0; k < count; ++k)
                if (OtherWeapon(entry[k].weapon, weapon) == (pass == 1))
                    order[n++] = k;
        return n;
    }
    static bool OtherWeapon(uintptr_t provedWith, uintptr_t now) { return provedWith && now && provedWith != now; }
    int Count() const {
        int n = 0;
        while (n < kProvenMost && entry[n].address)
            ++n;
        return n;
    }
};

// What a quick try came to (aim.cpp turns its Proof into this).
enum class Answer {
    kNone,             // not tried (its pitch not near the camera's, or not readable)
    kYes,              // proved
    kKept,             // kept the nudge, the camera didn't follow it (or didn't come back)
    kBusy,             // the camera moving
    kOverwritten,      // the nudge written over: a copy the game rebuilds, or a slot it no longer reads
    kNo,               // refused (a zero, a stray, the value changed under it)
    kNearNotMatching,  // after the hold its pitch within kLikelyPitch of the camera's but not within kStrict: not what the camera is built from
};
constexpr int kStrikes = 5;            // in a row: about 2 s of tries every 400 ms
constexpr float kLikelyPitch = 0.05f;  // a pair this near the camera's pitch is worth waiting for (the route's test before this round)

// A try's strikes after its answer: a proof or a busy camera starts them again; written over, or near but not matching, is one more; kept and not
// followed is one more for the route's (a known address the game reads keeps a nudge whose camera recoil or the game's own turn moved meanwhile); a
// refusal (a level look, a stray) or no try changes nothing.
inline int StrikesAfter(int strikes, Answer answer, bool route) {
    switch (answer) {
    case Answer::kYes:
    case Answer::kBusy:
        return 0;
    case Answer::kOverwritten:
    case Answer::kNearNotMatching:
        return strikes + 1;
    case Answer::kKept:
        return route ? strikes + 1 : strikes;
    default:
        return strikes;
    }
}
// Whether a known address holds the whole search back (every 400 ms for 15 s, as before this round): the newest as before whatever its pitch, an older
// one only with its pitch near the camera's; never one proved before a take since (the game reads the taken gun's slot: the user's try retried the
// old address 15 s after each take, the whole search waiting), nor one proved with another gun in hand (otherWeapon: Proven::OtherWeapon; each gun
// reads a look of its own, and a switch with key 1 or 2 is no take: in the user's play of 2026-11-19 the Mk11's address held the search 15 s after
// the switch to the AT4 picked up into slot 0, 01:10:42-01:10:57, its nudge kept and never followed), nor one that struck out.
inline bool KnownLikely(bool newest, bool takenSince, int strikes, float pitchApart, bool otherWeapon = false) {
    if (takenSince || otherWeapon || strikes >= kStrikes)
        return false;
    return newest || pitchApart < kLikelyPitch;
}
// The route's: with its pitch near the camera's (as before), for a soldier it wasn't proven on (on that one the known address is the same place, or
// the route led past a take to a slot the game no longer reads), not struck out.
inline bool RouteLikely(bool provenOnThisSoldier, int strikes, float pitchApart) {
    return !provenOnThisSoldier && strikes < kStrikes && pitchApart < kLikelyPitch;
}

// ---- The row swept: what was tried (aim.cpp's g_swept) ----

// The row's places the sweep proved for a soldier and got no proof from (SweepChoice's before: the next search goes on to the others). All of them
// tried and nothing proved, the slot isn't within reach of these copies: the round starts again only after kSweepRest searches that found it so
// (before this, every other search proved them all again, holding mouse aim each time for as long as the slot wasn't there). A new soldier and a find
// forget them. Search's own thread.
constexpr int kSweepRest = 5;
struct SweepTried {
    uintptr_t soldier = 0;
    std::unordered_set<uintptr_t> places;
    int rested = 0;  // searches in a row with nothing to take and places tried before

    void For(uintptr_t s) {
        if (s != soldier) {
            places.clear();
            rested = 0;
            soldier = s;
        }
    }
    bool Has(uintptr_t at) const { return places.count(at) != 0; }
    // After a search's choice (SweepChoice's taken and skipped): true when the round starts again (the places forgotten, from the first next time).
    bool Chosen(int taken, int before) {
        if (taken || !before) {
            rested = 0;
            return false;
        }
        if (++rested < kSweepRest)
            return false;
        places.clear();
        rested = 0;
        return true;
    }
    // After a place's answer: one that gave no proof is left out from here on; a proof isn't (Found follows), nor a camera moving (tried again in
    // the next search), nor one not tried.
    void Answered(uintptr_t at, Answer answer) {
        if (answer != Answer::kYes && answer != Answer::kBusy && answer != Answer::kNone)
            places.insert(at);
    }
    // The look angles found, by whatever way: what comes after a loss starts from the first.
    void Found() {
        places.clear();
        rested = 0;
    }
};

// ---- The reach: the memory about the look's copies looked through (aim.cpp's Search, with find) ----

// The look the game reads isn't always where the walk reaches, nor within the sweep's 0x900 of its copies. After the M249's checkpoint of 2026-11-18
// 22:03 the walk reached the same three copies in each of 43 searches (122 s of frames over 5.7 minutes), the slot the game read 0x1580 below them; in
// the user's play of 2026-11-19 the M4A1's look after the switch from the Mk11 lay 0x1D3880 from the copies reached (01:14:49 to 01:15:49) and the
// M249's 0x340580 (01:20 to 01:47, 292 s of frames); a .50 cal run of 2026-11-18 found the M4A1's only after 678.8 s (17:20:35, when the walk reached it,
// 0xD9C40 below its copy E71722CC). Every look proved in the logs kept (287 places, 2026-11-05 to 11-19) began on a 16-byte line with its yaw 0xC into it.
// So when the first pass and the sweep find nothing, the memory within kReachEachWay of each anchor (this search's candidates and swept places written
// over: the look's copies; the looks proved earlier in this launch) is looked through at each 16-byte line, nearest first, and what's taken there is
// proved as any candidate is. Only the game's own plain memory is read: committed, private and exactly read-write (never write-combined or uncached, as
// AMD's mapped video memory is, nor a guard page, nor a mapped file or a module), never a thread's stack, never a page out of the game's working set.
constexpr uintptr_t kReachEachWay = 0x1000000;  // 16 MB each way: the farthest seen, 0x340580
constexpr size_t kReachBytesMost = 0x4000000;   // 64 MB in one search at most
constexpr int kReachAnchors = 16;
constexpr int kReachMost = 16;  // more places of a kind than this at the camera's angles: none of that kind nudged (the walk's kMostPairs)
// Nearest first: each anchor's memory in rings, every anchor's nearest ring before any anchor's next (the farthest look of the logs, 0x40FA20 from its
// anchor, lies in each anchor's third), so a look through cut short by its time (kReachMicrosMost) has looked where the looks were.
constexpr uintptr_t kReachRings[4] = {0x10000, 0x100000, 0x500000, kReachEachWay};  // each way: 64 KB, 1 MB, 5 MB, 16 MB
// How long one search's look through memory may take (QueryPerformanceCounter's microseconds), mouse aim held all the while: 64 MB of the game's plain
// memory took 19 ms on the user's PC (twice 32 MB's 9.6, best of three, the game running), and memory that isn't plain RAM, read as AMD's write-combined
// video memory reads (0.7 us a word: 11 ms for 64 KB), would hold it for seconds. Checked before each 64 KB piece: past it, the rest is left.
constexpr uint64_t kReachMicrosMost = 25000;
// A 64 KB piece read slower than this, and slower again read at once once more (a thread switched out in the middle of a read is slow once), is memory
// that isn't plain RAM whatever the system calls it: its allocation left for the rest of the search, and for the launch at the second search so.
constexpr uint64_t kReachPieceSlow = 2000;  // us: plain memory reads 64 KB in about 20
constexpr int kReachSlowStrikes = 2;
constexpr size_t kReachPieceWords = 0x4000 + 6;  // a piece: 64 KB and the next line's six words

// VirtualQuery's state, type and protection as numbers (no Windows header here): MEM_COMMIT, MEM_PRIVATE, PAGE_READWRITE and no modifier, PAGE_GUARD.
constexpr uint32_t kMemCommit = 0x1000, kMemPrivate = 0x20000, kPageReadWrite = 0x04, kPageGuard = 0x100;
inline bool ReachReads(uint32_t state, uint32_t type, uint32_t protect) {
    return state == kMemCommit && type == kMemPrivate && protect == kPageReadWrite;
}

// Whether the reach takes places with a pointer before them alone (ReachKind's 1): only when this search has copies of the look (its candidates
// written over) and none of them has the row's shape: the .50 cal run of 2026-11-18 17:18-17:20, "0 of those at one instant slot-shaped", its look
// found at 17:20:35 without the row's shape too. The game copies the look it reads over the others, so a look with the row's shape has copies with it,
// and the pointer's alone would only widen where a nudge can land: a pointer before two words that read as the camera's angles is also a run of
// pointers wherever the game's memory lies at 0x3C000000 to 0x40C90FDB (floats of 0.008 to 6.28: 27 of the logs' copies lay at 0x40xxxxxx), and a
// pointer nudged is the crash of 2026-10-31 (bf3.exe+179AE2). No copies (the walk reached nothing): the row's shape alone.
inline bool ReachPointerAlone(int copies, int shapedCopies) {
    return copies > 0 && shapedCopies == 0;
}

// What the reach takes on a 16-byte line, by its seven words from the line (the yaw the fourth, the pitch the seventh) and the camera read at that
// instant: both angles the camera's within kStrict and neither a zero, and a pointer's shape in the first word; 2 with the row's whole shape too
// (SweepTakes), 1 with the pointer's alone (46 of the 128 finds of 2026-11-18/19 came with no slot-shaped pair at that instant; taken only as
// ReachPointerAlone says), 0 none.
inline int ReachKind(const uint32_t* words, float cameraYaw, float cameraPitch) {
    if (SweepTakes(words, cameraYaw, cameraPitch))
        return 2;
    float yaw = 0.0f, pitch = 0.0f;
    std::memcpy(&yaw, &words[3], 4);
    std::memcpy(&pitch, &words[6], 4);
    const bool pointer = words[0] >= 0x10000 && words[0] < 0xFFFF0000u && !(words[0] & 3);
    return pointer && StrictPair(yaw, pitch, cameraYaw, cameraPitch, false) && LooksLikeAngle(pitch, true) && !(std::fabs(pitch) < kNotZero) &&
                   !(YawApart(yaw, 0.0f) < kNotZero)
               ? 1
               : 0;
}

// A chunk of memory looked through: count words read from base (a 16-byte line); take(at, kind) for each place, at its yaw's address (the line + 0xC),
// one with a pointer alone only with pointerAlone (ReachPointerAlone). A place begins only where its seven words are all in the chunk: the next chunk
// begins at ReachNext's line, so each line is looked at once across the seams. Returns how many it took.
template <class Take>
inline int ReachScan(const uint32_t* words, size_t count, uintptr_t base, float cameraYaw, float cameraPitch, bool pointerAlone, Take&& take) {
    if (!words || count < 7 || (base & 0xF) || !std::isfinite(cameraYaw) || !std::isfinite(cameraPitch))
        return 0;
    int taken = 0;
    for (size_t i = 0; i + 7 <= count; i += 4) {
        float pitch = 0.0f;
        std::memcpy(&pitch, &words[i + 6], 4);
        if (!(std::fabs(pitch - cameraPitch) < kStrict))  // (the cheap test first: nearly every line goes no further)
            continue;
        if (const int kind = ReachKind(&words[i], cameraYaw, cameraPitch); kind == 2 || (kind == 1 && pointerAlone)) {
            ++taken;
            take(base + i * 4 + 0xC, kind);
        }
    }
    return taken;
}
// The line the chunk after one of count words from base begins at: the first line ReachScan didn't begin a place on (base itself for too few).
inline uintptr_t ReachNext(uintptr_t base, size_t count) {
    return count < 7 ? base : base + ((count - 7) / 4 + 1) * 16;
}

struct Span {
    uintptr_t from = 0, to = 0;  // [from, to): the 16-byte lines a place may begin on
};
// The memory a search looks through, in the order to look: ring by ring (kReachRings), each ring about every anchor in the anchors' order (the newest
// first) before the next ring, the window less what's taken already (each byte once: a ring's pieces below and above the one inside it), its edges on
// 16-byte lines, nothing under 0x10000 or from 0xFFFF0000 up; mostBytes in all at most (the window that would pass it cut about its anchor, then none).
// Returns how many spans (at most most: those past it are left).
inline int ReachSpans(const uintptr_t* anchors, int count, Span* out, int most, size_t mostBytes) {
    constexpr uintptr_t kLow = 0x10000, kHigh = 0xFFFF0000u;
    std::vector<Span> chosen;  // what's taken, disjoint, lowest first
    size_t total = 0;
    int n = 0;
    for (const uintptr_t ring : kReachRings)
        for (int k = 0; k < count && anchors && out && n < most; ++k) {
            const uintptr_t a = anchors[k];
            if (a < kLow || a >= kHigh)
                continue;
            const uintptr_t half = std::min<uintptr_t>(ring, static_cast<uintptr_t>((mostBytes - std::min(total, mostBytes)) / 2) & ~static_cast<uintptr_t>(0xF));
            if (half < 0x100)
                return n;
            // (Both edges down to a line: half a whole number of lines, the window is 2 * half at most, so the most is never passed.)
            const Span w{(a - kLow > half ? a - half : kLow) & ~static_cast<uintptr_t>(0xF), kHigh - a > half ? (a + half) & ~static_cast<uintptr_t>(0xF) : kHigh};
            std::vector<Span> pieces;  // the window less what's chosen, lowest first
            uintptr_t at = w.from;
            for (const Span& c : chosen) {
                if (c.to <= at || c.from >= w.to)
                    continue;
                if (c.from > at)
                    pieces.push_back({at, c.from});
                at = std::max(at, c.to);
            }
            if (at < w.to)
                pieces.push_back({at, w.to});
            for (const Span& p : pieces) {
                if (n >= most)
                    break;
                out[n++] = p;
                total += p.to - p.from;
                chosen.push_back(p);
            }
            std::sort(chosen.begin(), chosen.end(), [](const Span& x, const Span& y) { return x.from < y.from; });
        }
    return n;
}

struct Reached {
    uintptr_t at = 0;      // its yaw's address
    int kind = 0;          // ReachKind's
    uintptr_t anchor = 0;  // the anchor nearest it
    long Distance() const { return static_cast<long>(static_cast<intptr_t>(at) - static_cast<intptr_t>(anchor)); }
};

// What VirtualQuery says of the region about an address (no Windows header here).
struct Region {
    uintptr_t base = 0, end = 0;  // [base, end)
    uintptr_t allocation = 0;     // its allocation's base
    uint32_t state = 0, type = 0, protect = 0;
};
// What the reach keeps from one search to the next in a launch: the allocations a piece of was read slowly, with their strikes (kReachPieceSlow,
// kReachSlowStrikes). Search's own thread.
struct ReachMemo {
    struct Strikes {
        uintptr_t allocation = 0;
        int count = 0;
    };
    std::vector<Strikes> slow;
    bool Slow(uintptr_t allocation) const {
        for (const Strikes& s : slow)
            if (s.allocation == allocation)
                return s.count >= kReachSlowStrikes;
        return false;
    }
    // One more strike for it: true when it's left for the launch from here on.
    bool Strike(uintptr_t allocation) {
        for (Strikes& s : slow)
            if (s.allocation == allocation)
                return ++s.count >= kReachSlowStrikes;
        if (slow.size() < 256)
            slow.push_back({allocation, 1});
        return kReachSlowStrikes <= 1;
    }
};
// What one look through memory came to.
struct ReachTotals {
    size_t bytes = 0;          // read and looked through
    size_t notResident = 0;    // left: pages out of the game's working set (never read, so never paged in nor made)
    int stacks = 0;            // allocations left as a thread's stack (a guard page among their first regions)
    int slowNow = 0;           // pieces read slowly twice in a row in this search (their allocations left for the rest of it)
    uintptr_t slowAt = 0;      // ... the last one's allocation
    int slowLeft = 0;          // regions left: their allocation read slowly before (ReachMemo)
    bool cut = false;          // stopped at its time (kReachMicrosMost)
    uint64_t micros = 0;       // how long it took
    int counts[3] = {0, 0, 0};  // the places at the camera's angles of each kind, every one (places keeps kReachMost + 1 of each)
};
// The reach's look through memory (aim.cpp's ReachLook runs it on the game's memory through aimmemory.h, tools/aimsweeptest.cpp on made-up memory and
// on its own): each span in its order (ReachSpans), its regions as memory.Query gives them; only the game's plain memory (ReachReads), never an
// allocation memory.Own names (the reach's own piece, every thread's stack the threads name), nor one with a guard page among its first eight regions
// (a thread's stack: [reserved][guard][committed]), nor one the memo holds slow; in pieces of kReachPieceWords (64 KB and six words), and of those only
// the pages memory.Resident says are in the game's working set (none read that isn't: nothing paged in, nothing made); each piece read by memory.Read
// (false: freed meanwhile, the rest of that region left) and timed by memory.Micros (slow twice in a row: its allocation left for the rest of the
// search and struck in the memo: kReachPieceSlow, kReachSlowStrikes). Every 16-byte line of a span is begun once (ReachScan, ReachNext: the read goes six
// words past a span's end where its region goes on). Stops at microsMost, checked before each region and each piece (the nearest memory looked through
// first: ReachSpans' order). Places go into places, kReachMost + 1 of each kind at most (past that the kind is crowded: ReachChoice), every one
// counted. memory: Piece() (kReachPieceWords words), Query(at, Region&), Own(allocation), Resident(page, count, uint8_t* valid) (false: not known, all
// read), Read(from, uint32_t*, words), Micros().
template <class Memory>
ReachTotals ReachThrough(const Span* spans, int count, Memory& memory, ReachMemo& memo, float cameraYaw, float cameraPitch, bool pointerAlone,
                         std::vector<Reached>& places, uint64_t microsMost = kReachMicrosMost) {
    ReachTotals t;
    const uint64_t start = memory.Micros();
    uint32_t* piece = memory.Piece();
    if (!piece || !spans || !std::isfinite(cameraYaw) || !std::isfinite(cameraPitch))
        return t;
    struct Seen {
        uintptr_t allocation;
        bool stack;
    };
    std::vector<Seen> seen;          // this search's allocations, a stack or not
    std::vector<uintptr_t> struck;   // ... and those read slowly twice in a row in it
    const auto stack = [&](uintptr_t allocation) {
        for (const Seen& s : seen)
            if (s.allocation == allocation)
                return s.stack;
        bool guard = false;
        Region q;
        uintptr_t at = allocation;
        for (int k = 0; k < 8 && memory.Query(at, q) && q.allocation == allocation && q.end > at; ++k, at = q.end)
            guard = guard || (q.protect & kPageGuard) != 0;
        seen.push_back({allocation, guard});
        t.stacks += guard;
        return guard;
    };
    int kept[3] = {0, 0, 0};
    constexpr int kPagesMost = static_cast<int>((kReachPieceWords * 4 + 0xFFF) / 0x1000) + 1;
    for (int s = 0; s < count && !t.cut; ++s) {
        for (uintptr_t at = spans[s].from; at < spans[s].to && !t.cut;) {
            if (memory.Micros() - start >= microsMost) {
                t.cut = true;
                break;
            }
            Region r;
            if (!memory.Query(at, r) || r.end <= at)
                break;
            const uintptr_t end = std::min<uintptr_t>(r.end, spans[s].to);           // lines begun below this
            const uintptr_t readEnd = std::min<uintptr_t>(r.end, spans[s].to + 24);  // read up to this
            bool read = ReachReads(r.state, r.type, r.protect) && !memory.Own(r.allocation) &&
                        std::find(struck.begin(), struck.end(), r.allocation) == struck.end();
            if (read && memo.Slow(r.allocation))
                ++t.slowLeft, read = false;
            if (read && stack(r.allocation))
                read = false;
            for (uintptr_t from = at; read && from < end && readEnd - from >= 28;) {
                if (memory.Micros() - start >= microsMost) {
                    t.cut = true;
                    break;
                }
                size_t words = std::min<size_t>(kReachPieceWords, (readEnd - from) / 4);
                // The working set: up to its first page out of it, or past every page out of it from here on.
                const uintptr_t first = from & ~static_cast<uintptr_t>(0xFFF);
                const int pages = static_cast<int>((((from + words * 4 - 1) & ~static_cast<uintptr_t>(0xFFF)) - first) / 0x1000) + 1;
                uint8_t valid[kPagesMost] = {};
                if (pages <= kPagesMost && memory.Resident(first, pages, valid)) {
                    int k = 0;
                    while (k < pages && valid[k])
                        ++k;
                    if (k < pages) {
                        const uintptr_t gap = first + static_cast<uintptr_t>(k) * 0x1000;
                        if (gap < from + 28) {  // (nothing to begin before it)
                            int j = k;
                            while (j < pages && !valid[j])
                                ++j;
                            const uintptr_t past = first + static_cast<uintptr_t>(j) * 0x1000;
                            t.notResident += std::min(past, readEnd) - std::max(gap, from);
                            from = past;
                            continue;
                        }
                        words = (gap - from) / 4;
                    }
                }
                uint64_t was = memory.Micros();
                if (!memory.Read(from, piece, words))
                    break;
                if (memory.Micros() - was > kReachPieceSlow) {
                    was = memory.Micros();
                    if (!memory.Read(from, piece, words))
                        break;
                    if (memory.Micros() - was > kReachPieceSlow) {
                        ++t.slowNow;
                        t.slowAt = r.allocation;
                        struck.push_back(r.allocation);
                        memo.Strike(r.allocation);
                        break;
                    }
                }
                t.bytes += words * 4;
                ReachScan(piece, words, from, cameraYaw, cameraPitch, pointerAlone, [&](uintptr_t place, int kind) {
                    ++t.counts[kind];
                    if (kept[kind] <= kReachMost) {
                        ++kept[kind];
                        places.push_back({place, kind, 0});
                    }
                });
                const uintptr_t next = ReachNext(from, words);
                if (next <= from)
                    break;
                from = next;
            }
            at = r.end;
        }
    }
    t.micros = memory.Micros() - start;
    return t;
}

// The places to prove of those the scan took (places, any order): the row's whole shape first, then the pointer's alone; each kind nearest an anchor
// first (above it before below at one distance), each place once; never a candidate of this search (candidate(at): it had its answer), and one tried
// before for this soldier (before(at)) left out and counted (skipped: none taken and none skipped is nothing in reach). A kind with more than kReachMost
// places at the camera's angles is left out whole, as the walk's kMostPairs nudges none (crowded: bit 1 the pointer's alone, bit 2 the row's shape; with
// the row's shape crowded nothing at all is taken), counted as counts gives them where it's given (ReachThrough's: every one found, where places keeps
// kReachMost + 1 of a kind: a crowd past what's kept is still a crowd). Up to most into out; returns how many it took in all (those past most are left
// for the next search).
template <class IsCandidate, class Before>
inline int ReachChoice(const Reached* places, int count, const uintptr_t* anchors, int anchorCount, IsCandidate&& candidate, Before&& before, Reached* out,
                       int most, int* skipped = nullptr, int* crowded = nullptr, const int* counts = nullptr) {
    std::vector<Reached> all;
    for (int k = 0; k < count && places; ++k) {
        if (!places[k].kind)
            continue;
        bool again = false;
        for (const Reached& r : all)
            again = again || r.at == places[k].at;
        if (again)
            continue;
        Reached r = places[k];
        r.anchor = 0;
        for (int a = 0; a < anchorCount && anchors; ++a) {
            if (!anchors[a])
                continue;
            // (Not "far": windows.h makes that a word of nothing.)
            const uintptr_t gap = r.at > anchors[a] ? r.at - anchors[a] : anchors[a] - r.at;
            const uintptr_t nearest = r.anchor ? (r.at > r.anchor ? r.at - r.anchor : r.anchor - r.at) : ~static_cast<uintptr_t>(0);
            if (gap < nearest)
                r.anchor = anchors[a];
        }
        all.push_back(r);
    }
    int kinds[3] = {0, 0, 0};
    for (const Reached& r : all)
        ++kinds[r.kind == 2 ? 2 : 1];
    if (counts)
        kinds[1] = std::max(kinds[1], counts[1]), kinds[2] = std::max(kinds[2], counts[2]);
    const int crowd = (kinds[1] > kReachMost ? 1 : 0) | (kinds[2] > kReachMost ? 2 : 0);
    if (crowded)
        *crowded = crowd;
    std::sort(all.begin(), all.end(), [](const Reached& x, const Reached& y) {
        if (x.kind != y.kind)
            return x.kind > y.kind;
        const long dx = std::labs(x.Distance()), dy = std::labs(y.Distance());
        if (dx != dy)
            return dx < dy;
        if ((x.Distance() > 0) != (y.Distance() > 0))
            return x.Distance() > 0;
        return x.at < y.at;
    });
    int taken = 0, refused = 0;
    for (const Reached& r : all) {
        if ((crowd & 2) || (r.kind == 1 && (crowd & 1)) || candidate(r.at))
            continue;
        if (before(r.at)) {
            ++refused;
            continue;
        }
        if (taken < most && out)
            out[taken] = r;
        ++taken;
    }
    if (skipped)
        *skipped = refused;
    return taken;
}
// Whether the reach runs (with find): nothing found, not crowded, and something to look about.
inline bool ReachWanted(bool find, bool found, bool crowded, int anchors) {
    return find && !found && !crowded && anchors > 0;
}

// ---- How soon the next search ----

// A search that found nothing is followed by the next kSearchSoon after its start within kSoonFor of the look's loss (a new soldier, a load, a cutscene
// over, the look gone from the camera), kSearchLater after (as before, with find: 3 s). After a load or a cutscene the first walk often meets the game's
// view still swinging to the gun (mouse aim from the picture's turn: "0 of those at one instant"), and the next, 3 s later, found it (2026-11-18 06:51:20
// to 06:51:24, the train's checkpoint: the same at each of its loads).
constexpr uint64_t kSearchSoon = 2000, kSearchLater = 3000, kSoonFor = 20000;  // ms
inline uint64_t SearchAgainMs(uint64_t sinceLoss) {
    return sinceLoss < kSoonFor ? kSearchSoon : kSearchLater;
}

// ---- Mouse aim kept off the angles the search can't use (MouseAim, with find) ----

// A look within kNotZero of level or of straight ahead reads as memory's zeros (Proves never nudges a zero, and the first pass doesn't run at all: "exactly
// level or straight ahead"), and with yaw and pitch both small memory is full of pairs like it ("too many to nudge safely"). In the logs of 2026-11-18/19,
// of 708 searches with the camera's yaw and pitch both within 0.07 of zero 593 were too many to nudge (27 pairs on average); with the yaw within 0.07 and
// the pitch 0.12 to 0.2, none of 6 (3 pairs); both past 0.07, 1 of 499. Runs facing straight ahead weren't found for minutes: the M1014's 185 s (55
// searches "straight ahead"), the shell fix's 358 s (97), the AKS-74u's 144 s and the uncuffed run's 173 s (128 and 155 "too many").
// While mouse aim steers the game's view (the look not found) its aim is held off them: a yaw within kLevelOff of straight ahead kLevelOff to its right;
// then, with the yaw within kSmallBox of straight ahead and the pitch within kSmallBox of level, the pitch kSmallBox off level on its own side (under it
// at level: 7 degrees off the gun at the most); else a pitch within kLevelOff of level kLevelOff under it (as before). The picture never shows the game's
// view's own yaw and pitch (viewturn.h takes the camera's own out, and its yaw against the aim's: LookYaw stays the aim's); the rounds fired meanwhile go
// that far off the gun's line, the walk off it by the yaw's hold (4 degrees at most, from just left of straight ahead).
constexpr float kLevelOff = 0.035f;  // about 2 degrees: the search's zero is 0.02
constexpr float kSmallBox = 0.12f;   // about 7 degrees
inline void HoldOffZeros(float& yaw, float& pitch) {
    if (!std::isfinite(yaw) || !std::isfinite(pitch))
        return;
    if (YawApart(yaw, 0.0f) < kLevelOff)
        yaw = kLevelOff;
    if (YawApart(yaw, 0.0f) < kSmallBox && std::fabs(pitch) < kSmallBox)
        pitch = pitch > 0.0f ? kSmallBox : -kSmallBox;
    else if (std::fabs(pitch) < kLevelOff)
        pitch = -kLevelOff;
}

// ---- Mouse aim (MouseAim) ----

// Its last step (with find): half the gap a frame, cut toward zero, sends nothing under 2 counts (0.5 degrees at the measured 0.246 a count), which
// is left standing. Past kLastStep of a count with no count sent for kQuietFrames frames (the game has taken those on their way), one count toward the
// aim; never straight back the other way after one (a count larger than taken would swing it to and fro): only a cut step frees that again.
constexpr float kLastStep = 0.6f;
constexpr int kQuietFrames = 4;
struct MouseAxis {
    int quiet = 0;     // frames since a count went on this axis
    int lastStep = 0;  // the last step's sign, 0 after a cut step
};
// The counts to send on one axis for a gap (radians, the aim less the camera's, the sense the counts turn it); countsPerRadian as aim.cpp takes it.
// Without the last step: exactly the cut step of before. A gap that isn't finite: none.
inline long MouseStep(float gap, float countsPerRadian, MouseAxis& axis, bool lastStep) {
    if (!std::isfinite(gap) || !std::isfinite(countsPerRadian)) {
        axis.quiet = std::min(axis.quiet + 1, 1000);
        return 0;
    }
    const long cut = static_cast<long>(std::clamp(gap * countsPerRadian * 0.5f, -150.0f, 150.0f));
    if (cut) {
        axis.quiet = 0;
        axis.lastStep = 0;
        return cut;
    }
    if (lastStep && axis.quiet >= kQuietFrames) {
        const float counts = gap * countsPerRadian;
        const int sign = counts > kLastStep ? 1 : counts < -kLastStep ? -1 : 0;
        if (sign && sign != -axis.lastStep) {
            axis.quiet = 0;
            axis.lastStep = sign;
            return sign;
        }
    }
    axis.quiet = std::min(axis.quiet + 1, 1000);
    return 0;
}

// ---- The learned pitch limits (Update) ----

constexpr float kSteep = 0.9f;          // about 52 degrees: only writes past this teach the limits
constexpr float kPitchDefault = 1.5f;   // the limits before anything is learned (and after they're forgotten)
// Whether the game pulling the last write back teaches a limit: +1 the upper, -1 the lower, 0 none. A steep write pulled back toward level, narrower
// than the limit known. With find, only in a frame where the game left the yaw alone too: a game writing over the slot from another slot moves the yaw
// as well (a film of 2026-11-13: 30 frames a second of 0.73 degrees, and "above -11.6" learned from it, which then held every write there).
inline int PullTeaches(float written, float read, bool yawLeftAlone, bool find, float pitchMax, float pitchMin) {
    if (!(std::fabs(read - written) > 0.002f) || (find && !yawLeftAlone))
        return 0;
    if (written > kSteep && read < written && read < pitchMax)
        return 1;
    if (written < -kSteep && read > written && read > pitchMin)
        return -1;
    return 0;
}
// A learned limit tried again (with find): while the pitch wanted lies past it, every kProbeEvery the write goes past it for up to kProbeMs. Pulled back
// in that time, it stands; held all that time with the game leaving the yaw alone, there was none and it's dropped (a stand-in film's "-45" stayed the
// whole launch, the gun aiming 15 degrees off it at -60). +1 the upper, -1 the lower, 0 none to try now.
constexpr uint64_t kProbeEvery = 2000, kProbeMs = 100;  // ms: three of the game's ticks
inline int ProbeWanted(float wanted, float pitchMax, float pitchMin, uint64_t sinceLast, bool find) {
    if (!find || !std::isfinite(wanted) || sinceLast < kProbeEvery)
        return 0;
    if (pitchMax < kPitchDefault && wanted > pitchMax)
        return 1;
    if (pitchMin > -kPitchDefault && wanted < pitchMin)
        return -1;
    return 0;
}
// A probe's frame read back: pulled back (the limit stands) when the read is off what was written.
inline bool ProbePulled(float written, float read) {
    return !(std::fabs(read - written) <= 0.002f);
}
// The tries of one launch (Update's, the render thread): Past each frame as the pitch is written, Read each frame with what the frame before wrote.
struct LimitProbe {
    int side = 0;         // the side being tried: +1 the upper, -1 the lower, 0 none
    uint64_t since = 0;   // when this try began (ms)
    uint64_t last = 0;    // when the last began
    bool clean = false;   // the game left the yaw alone all the while
    // While a try goes on: the side to drop (+1, -1: held all of kProbeMs, the yaw left alone), else 0 (going on, or over: pulled back, it stands).
    int Read(float written, float read, bool yawLeftAlone, uint64_t now) {
        if (!side)
            return 0;
        clean = clean && yawLeftAlone;
        if (ProbePulled(written, read)) {
            side = 0;
            return 0;
        }
        if (now - since < kProbeMs)
            return 0;
        const int drop = clean ? side : 0;
        side = 0;
        return drop;
    }
    // Whether this frame's write goes past a learned limit: a try begun when due (wanted: the pitch asked for; may: with find, and a write of this look
    // read back already), or one going on while the pitch wanted still lies past it.
    bool Past(float wanted, float pitchMax, float pitchMin, bool may, uint64_t now) {
        if (!side && may)
            if (const int s = ProbeWanted(wanted, pitchMax, pitchMin, now - last, true))
                side = s, since = last = now, clean = true;
        if (side && may && (side > 0 ? wanted > pitchMax : wanted < pitchMin))
            return true;
        side = 0;
        return false;
    }
};

// ---- A held gun's bullets ----

// Where a held gun's bullets start on its frame (skin.cpp's DrawHeldPart; recoil's sight at the eye, skin::Sight, the same line): its sights' line at
// its muzzle's distance; with a sight of the table on it (its glass's middle on the frame), the line through that glass along its sights' line, at the
// same distance. Without, its rounds left from the irons' line, 3.1 to 4.6 cm under the reticle of the AN-94's and RPK-74M's optics at every range.
// A glass not finite: the sights.
inline void HeldShotLine(const float* sights, float upDegrees, const float* glass, float* out) {
    if (!glass || !std::isfinite(glass[0]) || !std::isfinite(glass[1]) || !std::isfinite(glass[2])) {
        out[0] = sights[0], out[1] = sights[1], out[2] = sights[2];
        return;
    }
    const float z = sights[2];
    out[0] = glass[0];
    out[1] = glass[1] - (z - glass[2]) * std::tan(upDegrees * 3.14159265f / 180.0f);
    out[2] = z;
}

}  // namespace aimrules

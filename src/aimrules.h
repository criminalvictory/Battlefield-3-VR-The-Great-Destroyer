#pragma once

// The look angles' search rules (aim.cpp runs them on the game's memory) and a held gun's bullets' line (skin.cpp), header only: no game memory and
// no Windows calls, so tools/aimrulestest.cpp runs them as they are. Each rule "with find" is weapon.txt "aimfind" on (the default); off, every rule
// gives what aim.cpp did before it ("aimfind 0").

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

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
struct Swept {
    uintptr_t at = 0;    // the place taken
    uintptr_t copy = 0;  // the written-over copy it was reached from (the nearest of them)
    long Distance() const { return static_cast<long>(static_cast<intptr_t>(at) - static_cast<intptr_t>(copy)); }
};
// The places to prove, nearest their copy first (above it before below), each once: read(at, words) gives the seven words from 0xC before at (false:
// not readable), skip(at) says a place not to take (a candidate of the pass already, or swept before for this soldier). Up to most into out; returns
// how many it took in all (those past most are left for the next search), skipped: how many skip refused that the sweep would have taken.
template <class Read, class Skip>
int SweepChoice(const uintptr_t* copies, int count, float cameraYaw, float cameraPitch, Read&& read, Skip&& skip, Swept* out, int most, int* skipped = nullptr) {
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
                if (again || !read(at, words) || !SweepTakes(words, cameraYaw, cameraPitch))
                    continue;
                seen[seenCount++] = at;
                if (skip(at)) {
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
// began) and its strikes in a row. A new soldier forgets them. Search's own thread.
constexpr int kProvenMost = 4;
struct Proven {
    struct Entry {
        uintptr_t address = 0;
        uint32_t swap = 0;
        int strikes = 0;
    };
    uintptr_t soldier = 0;
    Entry entry[kProvenMost] = {};  // newest first; address 0 past the last

    void For(uintptr_t s) {
        if (s != soldier) {
            *this = Proven{};
            soldier = s;
        }
    }
    // Proved just now: to the front (taken from where it was, else the oldest dropped), its strikes none.
    void Remember(uintptr_t s, uintptr_t address, uint32_t swap) {
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
        entry[0] = Entry{address, swap, 0};
    }
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
// old address 15 s after each take, the whole search waiting), nor one that struck out.
inline bool KnownLikely(bool newest, bool takenSince, int strikes, float pitchApart) {
    if (takenSince || strikes >= kStrikes)
        return false;
    return newest || pitchApart < kLikelyPitch;
}
// The route's: with its pitch near the camera's (as before), for a soldier it wasn't proven on (on that one the known address is the same place, or
// the route led past a take to a slot the game no longer reads), not struck out.
inline bool RouteLikely(bool provenOnThisSoldier, int strikes, float pitchApart) {
    return !provenOnThisSoldier && strikes < kStrikes && pitchApart < kLikelyPitch;
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

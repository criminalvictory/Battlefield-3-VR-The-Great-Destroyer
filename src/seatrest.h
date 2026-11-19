// A held gun's magazine's seat on its frame, on a level whose arms are bare (Semper Fidelis, The Great Destroyer: one draw, 18108/16053 handcuffed and 18108/13371 with the cuffs off (barestate.h), no gloves).
// With gloves the seat is taken from the game's own bones while the game's palms rest on the gun (skin.cpp's NoteHeldPalms, from the gloves' draw): its own
// reload takes them off it, and the magazine with them. Bare arms have no gloves' draw, so the palms are never known there, and a magazine in the hand
// had no seat to be drawn by. There the magazine says it itself: seated, it is rigid with the frame; in the game's own reload it moves on it. Its place
// on the frame unchanged for kRestMs is its seat. (The game's own reloads of the AKS-74u and the UMP-45, recorded on the train, replayed by
// tools\seatresttest.cpp: no seat is taken from a magazine on its way.)
// With gloves and the palms known, nothing of this is asked: Learn's first two arguments decide alone, as before. With gloves and the palms not known
// (a gun's first draw, its first second; prone, a sprint or a change of stance, when the game's hands move on its gun and nothing of them is learned) the
// magazine at rest on the frame gives the seat too, but only while none is known: a magazine taken out then was drawn nowhere in the hand (the owner's
// Operation Swordbreaker, 2026-11-19: tools\magseattest.cpp).
// Each gun keeps its own seats (Kept, Change), as it keeps its own rest of the game's arms: one drawn again, after another gun or a death, has them from
// its first frame. Forgotten at every change of gun, a magazine taken out before the game's palms were seen at rest again had no seat to be drawn by.
#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>

namespace seatrest {

constexpr float kSame = 0.0005f;     // m: the same place on the frame
constexpr float kSameTurn = 0.002f;  // and the same turn (each of its matrix's nine)
constexpr uint64_t kRestMs = 500;    // unchanged this long: seated
constexpr uint64_t kGapMs = 250;     // not drawn this long (put away, another gun's turn): it starts over

struct Watch {
    float at[12] = {};   // where the stretch began (not the last frame's: a slow drift isn't a rest)
    uint64_t since = 0;  // when
    uint64_t seen = 0;   // its last draw
    bool have = false;
};

// One draw's magazine bone on the frame (a 3x4, rows): true once it's been where it is for kRestMs.
inline bool Rests(Watch& w, const float* on, uint64_t now) {
    bool same = w.have && now - w.seen <= kGapMs;
    for (int i = 0; i < 12; ++i) {
        if (!std::isfinite(on[i])) {
            w.have = false;
            return false;
        }
        same = same && std::fabs(on[i] - w.at[i]) <= (i % 4 == 3 ? kSame : kSameTurn);
    }
    w.seen = now;
    if (!same) {
        std::memcpy(w.at, on, sizeof(w.at));
        w.since = now;
        w.have = true;
        return false;
    }
    return now - w.since >= kRestMs;
}

// Whether this draw's magazine bone is its seat now. With the game's palms known (gloves): while they rest on the gun, as it always was. Without them:
// the magazine at rest on the frame; with gloves only while no seat is known for it (seatKnown), so one known is never replaced but by the palms.
inline bool Learn(bool palmsKnown, bool gameMoving, bool bareArms, bool rests, bool seatKnown) {
    return palmsKnown ? !gameMoving : rests && (bareArms || !seatKnown);
}

// ---- The gun in hand's seats (skin.cpp's g_heldSeats, as kMagazineBones: the colour pass's, the rounds', the depth pass's two) ----

constexpr int kSeats = 4;
struct Seats {
    float seat[kSeats][12] = {};  // the magazine's bones on the frame, seated
    bool have[kSeats] = {};
};

// How a draw's seat was taken: not, by the game's palms at rest on the gun, by the magazine at rest on the frame.
enum class Took : uint8_t { kNone = 0, kByPalms = 1, kAtRest = 2 };
// One draw of the gun in hand with a magazine bone of seat i (skin.cpp's DrawHeldPart): that bone on the gun's frame as the game drew it (on; never one
// the game hides, its scale 1e-4: the caller leaves those out), the game's palms (known, and moving: its own reload), whether bare arms are drawn, the
// watch of this draw's bone (Rests) and the time. The seat i is taken from it, or left as it is.
inline Took Take(Seats& s, int i, Watch& watch, const float* on, bool palmsKnown, bool gameMoving, bool bareArms, uint64_t now) {
    if (palmsKnown) {
        if (gameMoving)
            return Took::kNone;
        std::memcpy(s.seat[i], on, sizeof(s.seat[i]));
        s.have[i] = true;
        return Took::kByPalms;
    }
    const bool rests = Rests(watch, on, now);
    if (!Learn(false, false, bareArms, rests, s.have[i]))
        return Took::kNone;
    std::memcpy(s.seat[i], on, sizeof(s.seat[i]));
    s.have[i] = true;
    return Took::kAtRest;
}

constexpr int kKinds = 32;  // guns kept (the mod knows fewer: skin.cpp's kHeldKinds)
struct Kept {
    uint32_t kind[kKinds] = {};
    Seats seats[kKinds];
    int count = 0;
    int next = 0;  // the one replaced when all are taken
};

// A change of gun from one kind (its colour pass's index count; 0 none, the launcher's) to another (skin.cpp's RigFor): the seats known of the gun
// before are kept as its own (each one known; one not known this time keeps what it had), and the gun now in hand's come back (none for one never
// seen). A seat on its frame is the gun's own model's, the same in every life and every level. Kept for kKinds guns, the oldest replaced.
inline void Change(Kept& kept, Seats& now, uint32_t from, uint32_t to) {
    if (from) {
        int at = -1;
        for (int k = 0; k < kept.count; ++k)
            if (kept.kind[k] == from)
                at = k;
        bool any = false;
        for (int i = 0; i < kSeats; ++i)
            any = any || now.have[i];
        if (at < 0 && any) {
            if (kept.count < kKinds) {
                at = kept.count++;
            } else {
                if (kept.kind[kept.next] == to)  // (never the one coming back)
                    kept.next = (kept.next + 1) % kKinds;
                at = kept.next;
                kept.next = (kept.next + 1) % kKinds;
            }
            kept.kind[at] = from;
            kept.seats[at] = Seats{};
        }
        if (at >= 0)
            for (int i = 0; i < kSeats; ++i)
                if (now.have[i]) {
                    std::memcpy(kept.seats[at].seat[i], now.seat[i], sizeof(now.seat[i]));
                    kept.seats[at].have[i] = true;
                }
    }
    now = Seats{};
    if (to)
        for (int k = 0; k < kept.count; ++k)
            if (kept.kind[k] == to)
                now = kept.seats[k];
}

}  // namespace seatrest

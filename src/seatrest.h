// A held gun's magazine's seat on its frame, on a level whose arms are bare (Semper Fidelis, The Great Destroyer: one draw, 18108/16053, no gloves).
// With gloves the seat is taken from the game's own bones while the game's palms rest on the gun (skin.cpp's NoteHeldPalms, from the gloves' draw): its own
// reload takes them off it, and the magazine with them. Bare arms have no gloves' draw, so the palms are never known there, and a magazine in the hand
// had no seat to be drawn by. There the magazine says it itself: seated, it is rigid with the frame; in the game's own reload it moves on it. Its place
// on the frame unchanged for kRestMs is its seat. (The game's own reloads of the AKS-74u and the UMP-45, recorded on the train, replayed by
// tools\seatresttest.cpp: no seat is taken from a magazine on its way.)
// With gloves nothing of this is asked: Learn's first two arguments decide alone, as before.
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
// only with bare arms drawn, and the magazine at rest on the frame.
inline bool Learn(bool palmsKnown, bool gameMoving, bool bareArms, bool rests) {
    return palmsKnown ? !gameMoving : bareArms && rests;
}

}  // namespace seatrest

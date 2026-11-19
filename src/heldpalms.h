// Where the game's palms rest on a held gun (skin.cpp's NoteHeldPalms, from the gloves' draw each frame, on the gun's frame), and whether the game's own
// hands have gone off that rest (its reload above all: skin::HeldGameMoving). Header only, no game memory and no Windows calls, so the tools run it as
// skin.cpp does (tools\magseattest.cpp).
//
// Taken once both palms have stayed within a centimetre of one place on the gun for a while, the first time after a second, then each 4 s; never while
// the game's hands move on its gun by the stance or a sprint (input::Sprinting: a sprint and a second after it, prone, 1.5 s after a stance change),
// never at the rest already known. Known, both palms 6 cm off it for 60 ms (not by a sprint) are the game's own moves; both back within 2 cm, not.
// A new gun in hand (RigFor) knows none of it: Forget.
// A held gun's magazine's seat is taken from the game's own bones only while these palms are known and not moving (seatrest.h's Learn).
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace heldpalms {

constexpr float kAway = 0.06f, kHome = 0.02f, kTryWithin = 0.01f;    // m
constexpr uint64_t kFirstRest = 1000, kNewRest = 4000, kAwayFor = 60;  // ms

struct State {
    float palm[2][3] = {};     // the rest, right and left, on the gun's frame (known)
    float tryAt[2][3] = {};    // where they've stayed since trySince
    bool known = false;        // a rest taken for the gun in hand
    bool moving = false;       // the game's hands off it: its own moves
    bool home = false;         // both at the rest known this frame (never without one)
    uint64_t trySince = 0;     // ms
    uint64_t awaySince = 0;    // ms
};

inline float Apart(const float* a, const float* b) {
    const float x = a[0] - b[0], y = a[1] - b[1], z = a[2] - b[2];
    return std::sqrt(x * x + y * y + z * z);
}

// This frame's palms (each a point on the gun's frame), now, and whether the stance or a sprint moves the game's hands (input::Sprinting). True when
// the rest is taken now.
inline bool Note(State& s, const float* right, const float* left, uint64_t now, bool sprinting) {
    float dr = 1.0f, dl = 1.0f;
    if (s.known) {
        dr = Apart(right, s.palm[0]);
        dl = Apart(left, s.palm[1]);
        // Not by sprinting: that moves the game's hands on the gun too, and the real hands aren't to follow it, as the rifle's and the launcher's don't (the review, 2026-11-05).
        if ((dr > kAway || dl > kAway) && !sprinting) {
            if (!s.awaySince)
                s.awaySince = now;
            if (now - s.awaySince >= kAwayFor)
                s.moving = true;
        } else if (dr < kHome && dl < kHome) {
            s.awaySince = 0;
            s.moving = false;
        }
    }
    s.home = dr < kHome && dl < kHome;
    const float tried = std::max(Apart(right, s.tryAt[0]), Apart(left, s.tryAt[1]));
    if (tried > kTryWithin || !s.trySince) {
        std::memcpy(s.tryAt[0], right, sizeof(s.tryAt[0]));
        std::memcpy(s.tryAt[1], left, sizeof(s.tryAt[1]));
        s.trySince = now;
    } else if (!sprinting && now - s.trySince > (s.known ? kNewRest : kFirstRest) && !s.home) {
        std::memcpy(s.palm, s.tryAt, sizeof(s.palm));
        s.known = true;
        s.awaySince = 0;
        s.moving = false;
        return true;
    }
    return false;
}

// Another gun in hand: nothing known of where the game's palms rest on it (where they last stayed is kept: the next Note starts a stay afresh).
inline void Forget(State& s) {
    s.known = false;
    s.moving = false;
    s.trySince = 0;
    s.awaySince = 0;
}

}  // namespace heldpalms

// What rides a belt fed gun's feed cover: a draw of its own the game skins to the cover's bone, turned up with the cover when the hand lifts it (skin.cpp
// DrawHeldPart), as the cover's own bone in the body's draw is. The bridge M249's ACOG may be one (the review of 2026-11-18: "the ACOG's draw isn't
// turned with the lid ... so it floats when the cover is opened by hand"; the catalogue's data: every optic on these guns sits on the bone that is the
// feed cover). The game has drawn a sight piece so before: the Stinger's 354/0, its bone 0 the body's bone 1. A sight on a mount of its own (a bone of
// its own off the cover's) isn't this; its draws are to be recorded in the game first (docs/research/m249.md's in-game checks). Pure, no game.

#pragma once

#include <cmath>

#include "handmag.h"

namespace lidride {

// The same bone: places within this, each axis' parts within that (about 0.1 degrees).
constexpr float kSamePlace = 0.0002f, kSameAxis = 0.002f;

// Whether a bone of a draw, on the gun's frame as the game draws it, is the feed cover's bone there (the body's draw's, the same moment): its very
// place and turn. (The game never lifts the cover itself while the hand does: both stay where it rests.)
inline bool Rides(const float* lidOnFrame, const float* boneOnFrame) {
    for (int i = 0; i < 3; ++i) {
        if (!(std::fabs(boneOnFrame[i * 4 + 3] - lidOnFrame[i * 4 + 3]) <= kSamePlace))  // (not a number: not the same)
            return false;
        for (int j = 0; j < 3; ++j)
            if (!(std::fabs(boneOnFrame[i * 4 + j] - lidOnFrame[i * 4 + j]) <= kSameAxis))
                return false;
    }
    return true;
}

// The cover turned up by angle (radians) about x through its hinge (y, z), on the frame: what lifts the cover's bone in the body's draw.
inline void Turn(const handmag::BeltGun& b, float angle, float* out) {
    const float c = std::cos(angle), s = std::sin(angle), hy = b.lidHinge[0], hz = b.lidHinge[1];
    const float turn[12] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, c, -s, hy - (c * hy - s * hz), 0.0f, s, c, hz - (s * hy + c * hz)};
    for (int i = 0; i < 12; ++i)
        out[i] = turn[i];
}

}  // namespace lidride

// What rides a belt fed gun's feed cover: a draw of its own the game skins to the cover's bone, turned up with the cover when the hand lifts it (skin.cpp
// DrawHeldPart), as the cover's own bone in the body's draw is (Rides); and a sight on the cover's own mount, its bone the cover's moved by a fixed
// offset along the cover's axes (OnMount: the bridge M249's ACOG, recorded 2026-11-19 through the game's own reload, every one of its six draws so in
// every frame: m249rows::kSightMount). The game has drawn a sight piece on a body's bone before: the Stinger's 354/0, its bone 0 the body's bone 1.
// Pure, no game.

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

// Whether a bone of a draw, on the gun's frame as the game draws it, is on the cover's own mount (BeltGun::mount; none: never): the cover's bone there
// (the body's draw's, the same moment) moved by the mount along the cover's own axes, its axes the cover's. As Rides, within the same.
inline bool OnMount(const handmag::BeltGun& b, const float* lidOnFrame, const float* boneOnFrame) {
    if (!b.mount)
        return false;
    for (int i = 0; i < 3; ++i) {
        const float at = lidOnFrame[i * 4] * b.mount[0] + lidOnFrame[i * 4 + 1] * b.mount[1] + lidOnFrame[i * 4 + 2] * b.mount[2] + lidOnFrame[i * 4 + 3];
        if (!(std::fabs(boneOnFrame[i * 4 + 3] - at) <= kSamePlace))  // (not a number: not there)
            return false;
        for (int j = 0; j < 3; ++j)
            if (!(std::fabs(boneOnFrame[i * 4 + j] - lidOnFrame[i * 4 + j]) <= kSameAxis))
                return false;
    }
    return true;
}

// Whether a draw (not the gun's body) is tried against the cover lifted by hand at all, before Rides and OnMount look at each of its bones: framed in the
// view the gun's body was drawn in (sameView: skin.cpp's exact, its bones and the cover's from one picture), or drawn as a hand last held the gun (asKept:
// lying, flying), or a held sight's own draw after its body (sightsOwn: the ACOG's glass, chevron and fibre). Those the game draws in its see-through
// pass, which comes with a first-person view of its own (the session's picture order of 2026-11-19: three a frame for each eye, that pass the last, after
// the shadow views), so never in the body's view; yet framed by the same eye's body they're on the mount as their body is (the game's own reload,
// tools/m249acogtest.cpp a7). Any other draw from another view may be a frame's bob off the body's: not tried. (With sameView alone the ACOG's glass,
// chevron and fibre stayed shut over the tray while its body went up with the cover: the review of dd042df.)
inline bool Tried(bool sameView, bool asKept, bool sightsOwn) {
    return sameView || asKept || sightsOwn;
}

// The cover turned up by angle (radians) about x through its hinge (y, z), on the frame: what lifts the cover's bone in the body's draw.
inline void Turn(const handmag::BeltGun& b, float angle, float* out) {
    const float c = std::cos(angle), s = std::sin(angle), hy = b.lidHinge[0], hz = b.lidHinge[1];
    const float turn[12] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, c, -s, hy - (c * hy - s * hz), 0.0f, s, c, hz - (s * hy + c * hz)};
    for (int i = 0; i < 12; ++i)
        out[i] = turn[i];
}

}  // namespace lidride

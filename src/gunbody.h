#pragma once

// The player's own rifle as a loose body (loose.h), let go of by the hands (free hands): the M416 with its M320 and ACOG, as
// Rock and a Hard Place gives it. In the gun's model space (its frame bone's): x to its left, y up, z to the muzzle; metres. Every number an
// estimate, to refine from the films (and from the mesh dumps with "gunpartlog 1"):
//   - its box, for its weight and its turn: build\guns\m416.bin's grid less its 5 cm of air (x -0.040 to 0.028, y -0.122 to 0.081, z -0.018 to
//     0.776), and back to the stock's butt (the stock is its own mesh, 7848's bone 0 at z -0.142 by skin.cpp's kPartRests; its butt about -0.23).
//     Its middle at x -0.006 tips one standing upright onto its right side;
//   - its weight: the M416 3.5 kg, the M320 1.5, the ACOG 0.4; a round 12.3 g, 30 in the magazine;
//   - its bounce 1.0 (on dirt 0.10, a dull landing: the game's own pickup's restitution is 0.1);
//   - where it touches the ground (kM416Hull): the stock's butt, the top rail, the ACOG, the handguard (hands.cpp's kGuardFrom/To, y 0.045 from
//     z 0.46 to 0.68), the muzzle, the M320's tube, the brass deflector on its right (the grid's widest: x -0.042 at y 0.044, z 0.265), and along
//     the bottom in the middle (x 0) the butt's heel, the pistol grip, the magazine's bottom (pouch.cpp's well, its top at (0 0.041 0.415)) and the
//     M320's grip (its middle at x 0.0004: m320.cpp's kGripPlane). Lying on a side it rests on the side's points; dropped upright it stands on
//     the bottom's points alone, all at x 0, so it always tips over (a box's corners would let it sleep standing on its magazine; the butt's heel
//     at x +-0.03, as first estimated, made a foot with the magazine that held its middle, 6 mm right of x 0: about one drop in fifty stood on
//     it, loosetest's sweep).
#include "loose.h"

namespace gunbody {

inline constexpr float kM416Hull[22][3] = {
    {0.030f, 0.045f, -0.230f},  {-0.030f, 0.045f, -0.230f},   // the stock's butt, its top
    {0.0f, -0.075f, -0.230f},                                 // ... its heel, in the middle
    {-0.042f, 0.044f, 0.265f},                                // the brass deflector, on its right
    {0.030f, 0.081f, 0.000f},   {-0.030f, 0.081f, 0.000f},    // the top rail at the receiver's back
    {0.022f, 0.150f, 0.100f},   {-0.022f, 0.150f, 0.100f},    // the ACOG's top, its back
    {0.022f, 0.150f, 0.300f},   {-0.022f, 0.150f, 0.300f},    // ... and its front
    {0.028f, 0.080f, 0.680f},   {-0.028f, 0.080f, 0.680f},    // the handguard's front, its top
    {0.014f, 0.045f, 0.776f},   {-0.014f, 0.045f, 0.776f},    // the muzzle
    {0.025f, -0.060f, 0.350f},  {-0.025f, -0.060f, 0.350f},   // the M320's tube, its back
    {0.025f, -0.060f, 0.620f},  {-0.025f, -0.060f, 0.620f},   // ... and its front
    {0.0f, -0.122f, 0.210f},                                  // the pistol grip's bottom
    {0.0f, -0.175f, 0.400f},    {0.0f, -0.180f, 0.460f},      // the magazine's bottom, its back and its front
    {0.0f, -0.180f, 0.490f},                                  // the M320's grip's bottom
};
// The hull's box: how near a hand must be to take it up is measured to it (grasp.h's pickReach).
inline constexpr float kM416HullMin[3] = {-0.042f, -0.180f, -0.230f}, kM416HullMax[3] = {0.030f, 0.150f, 0.776f};

inline constexpr loose::Spec kM416{.name = "m416gun", .boxMin = {-0.040f, -0.122f, -0.230f}, .boxMax = {0.028f, 0.081f, 0.776f}, .sweep = 0.0f, .sag = 0.0f,
                                   .massEmpty = 5.4f, .massRound = 0.0123f, .capacity = 30, .bounce = 1.0f, .keep = true, .hull = kM416Hull, .hullPoints = 22};

}  // namespace gunbody

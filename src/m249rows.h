// The M249's rows (Operation Swordbreaker, measured 2026-11-18; docs/research/m249.md), in one place: skin.cpp draws and works the gun by them (its
// kM249Bones, its kHeldKinds and kHeldEyes rows; handmag's belt, box and way below) and tools/m249test.cpp holds these very rows to the mesh dumps and
// the recordings they were read from, so an edit here is an edit the test sees (the review of 2026-11-18: a copy in the test could drift).
//
// Places are on the gun's frame bone (3): x to its left, y up, z to the muzzle, metres. Its buffer: the belt 2808/0 (five bones, the M240's links to half
// a millimetre), the body 24291/2808 (0 the charging handle, 1 the feed cover, 2 the carrying handle, 3 the frame), the box 987/27099 (a cloth pouch under
// the gun, alone on its own palette's bone 0: the body has no bone of it); its depth pass 28086/28086 has them all: 0-4 the belt, 5 the handle, 6 the
// cover, 7 the carrying handle, 8 the frame, 9 the box.

#pragma once

#include <cstddef>
#include <iterator>

#include "handmag.h"
#include "magposes.h"

namespace m249rows {

// Its draws (index count, start) and their bones.
inline constexpr unsigned kBody = 24291, kBodyStart = 2808, kDepth = 28086;
inline constexpr unsigned kBeltCount = 2808, kBeltStart = 0, kBoxCount = 987, kBoxStart = 27099;
inline constexpr int kFrameBone = 3, kHandleBone = 0, kCoverBone = 1, kCarryBone = 2, kBoxBone = 0;
inline constexpr int kDepthBeltBones = 5, kDepthHandle = 5, kDepthCover = 6, kDepthCarry = 7, kDepthFrame = 8, kDepthBox = 9;
// Its row in ClearRifleSpread's table (its own GunSwayData, inline in its blueprint), and its collision shape's name (shaperecipes.h).
inline constexpr size_t kSpreadRow = 14;
inline constexpr const char* kShape = "m249";

// Its iron sights: the rear aperture on the feed cover (its hole's middle (0 0.0927), the hole 8 mm across and 1.4 cm deep, from its back face at z
// 0.206 to its front face at 0.2203), the front post in its hood (its top y 0.0845 at z 0.813): the line through them lies 0.78 degrees below z and
// meets the muzzle's plane at y 0.083. (The game's one zoomed sample, FOV 40: the eye at (0 0.093 0.088) looking 0.68 degrees down, on the post's top
// and 1.1 mm low in the hole: the same point at the muzzle.)
inline constexpr float kUp = 0.78f;
inline constexpr float kSights[3] = {0.0f, 0.083f, 0.935f};
inline constexpr float kMuzzle[3] = {0.0f, 0.026f, 0.935f};
// Its port where its shell effect is in its data (FX_ShellEjection_M at (-0.0304 0.0419 0.4217), the M416's own effect; its links' FX_BeltLinkEjection_M
// at (-0.035 0.042 0.42), left to the game).
inline constexpr float kPort[3] = {-0.030f, 0.042f, 0.422f};
// The game's palms at rest on it (its recordings).
inline constexpr float kRightPalm[3] = {-0.021f, -0.107f, 0.218f}, kLeftPalm[3] = {0.028f, -0.084f, 0.583f};
// Its eye row (kHeldEyes): the game's zoomed eye's z on the frame, and the aperture ring's back face ahead of it, the gun's nearest on the line.
inline constexpr float kEyeZ = 0.088f, kRearZ = 0.205f;
// The box in its own vertices' space (for the box dropped: its bounds).
inline constexpr float kBoxMin[3] = {-0.101f, -0.2006f, 0.0435f}, kBoxMax[3] = {0.092f, -0.0826f, 0.139f};

// Its belt by hand, as the M240's: the feed cover's hinge (y 0.0575, z 0.4751) and 102.6 degrees, its back end where the game's hand takes it, the belt
// hanging and laid as its own reload has them, the feed, where the belt goes into the box, the pieces: the M240's links to half a millimetre.
inline constexpr handmag::BeltGun kBelt{{0.0575f, 0.4751f}, 102.6f * 3.14159265f / 180.0f, {0.050f, 0.072f, 0.2205f}, magposes::kM249LidHold, magposes::kM249LidFingers,
                                        magposes::kM249BeltHanging, magposes::kM249BeltLaid, {0.4884f, 1.0245f, 0.2538f}, magposes::kBeltPinchFingers, {0.006f, 0.0553f, 0.4287f},
                                        {0.071f, -0.050f, 0.428f},
                                        {{-0.0015f, -0.0445f, 0.025f, -0.0445f}, {0.0f, -0.0625f, 0.027f, -0.0625f}, {0.0f, -0.0768f, 0.024f, -0.0768f},
                                         {0.005f, -0.079f, 0.005f, -0.105f}, {-0.003f, -0.088f, -0.003f, -0.143f}},
                                        0.307f, {}, true};  // (heldtest 7's pinch: its own hanging tip; what rides its cover goes up with it)
// Its box's way off and on, its own reload's (src/magposes/m249_way.h, tools/boxway.cpp): off 8 cm to the gun's left at the seat's height and free there,
// then left and down; on from the left at the seat's height, turned 5.5 degrees about the gun's length, settling.
inline constexpr handmag::BoxWay kWay{magposes::kM249WayOff, static_cast<int>(std::size(magposes::kM249WayOff)), magposes::kM249WayFree, magposes::kM249WayOn,
                                      static_cast<int>(std::size(magposes::kM249WayOn))};
// Its box by hand: no button, nothing knocks it out; taken anywhere along it (19 cm across the gun: from the game's own hold at its left end, grab, to
// its right end, grabTo), off and on along its way (way: not straight down, not rocked: mouth is left unused by it); the handle's knob on the right,
// 12.9 cm. (top: the seated box's top's middle; hook: its front face's top; paddle: behind it.)
inline constexpr handmag::Gun kMag{.top = {0.0f, -0.019f, 0.425f}, .grab = {0.111f, -0.062f, 0.421f}, .paddle = {0.0f, -0.030f, 0.377f}, .hook = {0.0f, -0.020f, 0.473f},
                                   .mouth = -0.05f, .lever = {-0.0725f, 0.021f, 0.4715f}, .leverTravel = 0.129f, .rockIn = 0.0f, .handOffset = {0.0f, 0.0f, 0.0f},
                                   .hold = magposes::kM249Hold, .fingers = magposes::kM249Fingers, .button = false, .noKnock = true,
                                   .leverHold = magposes::kM249LeverOverHold, .leverFingers = magposes::kM249LeverOverFingers, .belt = &kBelt,
                                   .leverUnderHold = magposes::kM249LeverUnderHold, .leverUnderFingers = magposes::kM249LeverUnderFingers, .way = &kWay, .wide = true,
                                   .grabTo = -0.101f};

}  // namespace m249rows

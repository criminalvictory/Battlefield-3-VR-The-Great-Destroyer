// The PKP Pecheneg's rows (Operation Swordbreaker, measured 2026-11-18; docs/research/pkp.md), in one place: skin.cpp draws and works the gun by them (its
// kPkpBones, its kHeldKinds and kHeldEyes rows; handmag's belt, box and way below) and tools/pkptest.cpp holds these very rows to the mesh dumps and the
// recordings they were read from, so an edit here is an edit the test sees (as m249rows.h: a copy in the test could drift).
//
// Places are on the gun's frame bone (7): x to its left, y up, z to the muzzle, metres. Its buffer: the belt 2310/0 (five bones, numbered from the box: 4
// lies across the feed tray, 3 to 0 go down the receiver's right side into the box), the body 23298/2310 (0 the box, a steel box 29 cm across the gun
// under the receiver, a bail on its right end; 1 the cases' port flap on the left; 2 the feed arm on the cover's right; 3 the part under the cover; 4 the
// charging handle on the right; 5 the links' port flap on the cover's left; 6 the trigger; 7 the frame; 8 the feed cover); its depth pass 25608/25608 has
// them all: 0-4 the belt, then the body's bones 0-8 as 5-13. The cover is four bones (8, 3, 5, 2) that the game turns together about the same hinge (the
// feed arm, 2, with a turn of its own besides: 42 degrees off the cover resting open, up to 39 as the handle is drawn; by hand it goes with the cover).

#pragma once

#include <cstddef>
#include <iterator>

#include "handmag.h"
#include "magposes.h"

namespace pkprows {

// Its draws (index count, start) and their bones.
inline constexpr unsigned kBody = 23298, kBodyStart = 2310, kDepth = 25608;
inline constexpr unsigned kBeltCount = 2310, kBeltStart = 0;
inline constexpr int kFrameBone = 7, kBoxBone = 0, kCaseFlapBone = 1, kHandleBone = 4, kTriggerBone = 6, kCoverBone = 8;
inline constexpr int kCoverAlso[3] = {3, 5, 2};  // the cover's other bones: under it, the links' flap, the feed arm
inline constexpr int kDepthBeltBones = 5, kDepthBox = 5, kDepthHandle = 9, kDepthFrame = 12, kDepthCover = 13;
inline constexpr int kDepthCoverAlso[3] = {8, 10, 7};
// Its row in ClearRifleSpread's table (its own GunSwayData, Venice-EBX 95D41E17), and its collision shape's name (shaperecipes.h).
inline constexpr size_t kSpreadRow = 15;
inline constexpr const char* kShape = "pkp";

// Its iron sights: the front post's tip (0 0.1124 1.125), a round post in a hood whose ears stand to y 0.1167; no rear sight stands on its line (the cover's
// top lies 1.7 cm under it, the carrying handle 1.35 cm off it). The game's zoomed eye (FOV 40, the one sample: the view in each of its bones, all six
// agreeing to 0.6 mm) is at (0 0.1148 0.1588) looking 0.18 degrees down, on the post's tip to 0.5 mm: the line from that eye through the tip lies 0.14
// degrees below z and meets the muzzle's plane at y 0.1122.
inline constexpr float kUp = 0.14f;
inline constexpr float kSights[3] = {0.0f, 0.1122f, 1.2139f};
inline constexpr float kMuzzle[3] = {0.0f, 0.0466f, 1.2139f};
// Its port where its shell effect is in its data (FX_ShellEjection_762 at (0.0204 0.0094 0.4796), turned (10.6 -1.2 -155): the cases out to its LEFT, at
// its flap (bone 1); its links' FX_BeltLinkEjection_M at (0.0225 0.0691 0.5582), the links' flap on the cover's left (bone 5), left to the game).
inline constexpr float kPort[3] = {0.020f, 0.009f, 0.480f};
// The game's palms at rest on it (its recordings: the empty reload after it, 620 frames).
inline constexpr float kRightPalm[3] = {-0.054f, -0.112f, 0.251f}, kLeftPalm[3] = {0.015f, -0.073f, 0.715f};
// Its eye row (kHeldEyes): the game's zoomed eye's z on the frame, and the first of the gun within 1 cm of the line ahead of it: the front sight's hood (by its
// mesh; nothing before it comes within 1.05 cm).
inline constexpr float kEyeZ = 0.159f, kRearZ = 1.118f;
// The box in its own vertices' space (for the box dropped: its bounds; its bone 0 rests on the frame moved by (0 0.0761 0.4901), unturned).
inline constexpr float kBoxMin[3] = {-0.136f, -0.262f, 0.013f}, kBoxMax[3] = {0.154f, -0.0782f, 0.1094f};

// Its belt by hand, as the M240's and the M249's but from the gun's right: the feed cover's hinge (y 0.0708, z 0.6155) and 93 degrees (where it rests open in
// the game's own reload, 3.0 to 5.1 s; it swings to 98.5 as it pops open, then settles), the hand on its top where the game's own (mirrored onto the left)
// lifts it, the belt hanging and laid as its own reload has them, the feed (the piece across the tray, its middle), where the belt goes into the box (its
// last piece's lower end), the pieces in their own bones' space (by bone: 0 to 3 standing, their tops nearer the feed; 4 across the tray, its left end the
// one nearer the feed), its rounds' middle along z.
inline constexpr handmag::BeltGun kBelt{{0.0708f, 0.6155f}, 93.0f * 3.14159265f / 180.0f, {0.0237f, 0.1088f, 0.3506f}, magposes::kPkpLidHold, magposes::kPkpLidFingers,
                                        magposes::kPkpBeltHanging, magposes::kPkpBeltLaid, {0.4884f, 1.0245f, 0.2538f}, magposes::kBeltPinchFingers, {-0.0200f, 0.0576f, 0.5557f},
                                        {-0.080f, -0.0606f, 0.554f},
                                        {{-0.0050f, -0.0887f, -0.0050f, -0.1041f}, {-0.0057f, -0.0795f, -0.0057f, -0.1110f}, {-0.0035f, -0.0681f, -0.0035f, -0.1006f},
                                         {-0.0044f, -0.0579f, -0.0044f, -0.0909f}, {0.0295f, -0.0516f, -0.0305f, -0.0516f}},
                                        0.307f, {}, true, true};  // (heldtest 7's pinch: its own hanging tip; what rides its cover goes up with it; numbered from the box)
// Its box's way off and on, its own reload's (src/magposes/pkp_way.h, tools/boxway.cpp): off to the gun's right, its left end sliding level while its right end
// tips down (turned 19.5 degrees by 8 cm), free 8.2 cm along, then right and down, turned 27.5; on from the right and below, swung up into its seat.
inline constexpr handmag::BoxWay kWay{magposes::kPkpWayOff, static_cast<int>(std::size(magposes::kPkpWayOff)), magposes::kPkpWayFree, magposes::kPkpWayOn,
                                      static_cast<int>(std::size(magposes::kPkpWayOn))};
// Its box by hand: no button, nothing knocks it out; taken anywhere along it (29 cm across the gun: from the fitted hold at its left end, grab (the fitted
// hand's middle there), to its bail on its right end, grabTo; within 9 cm of grab the hand goes onto it as the fitted hold has it, further along it's held
// where the hand took it: handmag), off and on along its way; the handle's knob on the right, 14.5 cm. (top: the box's top's middle under the gun; hook: its
// front face's top; paddle: its back face's.)
inline constexpr handmag::Gun kMag{.top = {0.0f, -0.0197f, 0.551f}, .grab = {0.1740f, -0.0817f, 0.5594f}, .paddle = {0.0f, -0.030f, 0.500f}, .hook = {0.0f, -0.020f, 0.600f},
                                   .mouth = -0.05f, .lever = {-0.0509f, 0.0180f, 0.4814f}, .leverTravel = 0.145f, .rockIn = 0.0f, .handOffset = {0.0f, 0.0f, 0.0f},
                                   .hold = magposes::kPkpHold, .fingers = magposes::kPkpFingers, .button = false, .noKnock = true,
                                   .leverHold = magposes::kPkpLeverOverHold, .leverFingers = magposes::kPkpLeverOverFingers, .belt = &kBelt,
                                   .leverUnderHold = magposes::kPkpLeverUnderHold, .leverUnderFingers = magposes::kPkpLeverUnderFingers, .way = &kWay, .wide = true,
                                   .grabTo = -0.136f};

}  // namespace pkprows

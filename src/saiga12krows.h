// The Saiga 12K's rows (Operation Swordbreaker, measured 2026-11-18; docs/research/saiga12k.md), in one place: skin.cpp draws and works the gun by them
// (its kSaiga12kBones, its kHeldKinds and kHeldEyes rows) and tools/saiga12ktest.cpp holds these very rows to the meshes and the recordings they were
// read from, so an edit here is an edit the test sees.
//
// Places are on the gun's frame bone (2): x to its left, y up, z to the muzzle, metres. Its body 14970/0 (0 the bolt with its handle on the right, 1 the
// trigger, 2 the frame) and its depth pass 14970/14970 (the same bones, in the same order) have no magazine: the magazine is a mesh of its own
// (weapons/saiga20k/saiga_20k_regularmag_1p_Mesh), a skinned socket object the game makes only when the gun carries one of the five primary-accessory
// unlocks of its blueprint (NoPrimaryAccessory, Flashlight, Targetpointer, Flashsuppressor, Silencer; the ExtendedMag's socket makes the long one,
// saiga_20k_extendedmag_1p_Mesh, instead). The enemies' Saigas carry NoPrimaryAccessory (PLR_Extremist_SHG's kit), Kaffarov's and The Great Destroyer's
// a flashlight or a laser: every real one has its magazine. The morning's swap (the M4A1's blueprint made the Saiga's, its unlocks the M4A1's) carried
// none of them: no magazine was made, so none was drawn and the reload's hand closed on nothing. Its three draws, each on a palette of its own of the
// gun's skeleton's bones 7 (the magazine) and 8 (its shells): the box 1176/0 (on its bone 0, bone 7), its shells 450/1176 (on its bone 0, bone 8), and
// their depth pass 1626/1626 (bone 0 the box, 1 the shells), from its own buffers.

#pragma once

#include <cstddef>

#include "handmag.h"
#include "magposes.h"

namespace saiga12krows {

// Its draws (index count, start) and their bones.
inline constexpr unsigned kBody = 14970, kBodyStart = 0, kDepth = 14970;
inline constexpr int kFrameBone = 2, kBoltBone = 0, kTriggerBone = 1;
// The magazine's own draws and their bones in their own palettes.
inline constexpr unsigned kMagCount = 1176, kMagStart = 0, kShellsCount = 450, kShellsStart = 1176, kMagDepthCount = 1626, kMagDepthStart = 1626;
inline constexpr int kMagBone = 0, kShellsBone = 0, kDepthMagBone = 0, kDepthShellsBone = 1;
// Its row in ClearRifleSpread's table (its own GunSwayData, Weapons/SAIGA20K/SAIGA20K_GunSway), and its collision shape's name (shaperecipes.h).
inline constexpr size_t kSpreadRow = 15;
inline constexpr const char* kShape = "saiga12k";

// The magazine's bone on the frame, seated: no turn, only moved. The game's far LOD of the gun (saiga_20k_3p_Mesh's LOD4, every part on the frame's
// bone) has the magazine in it as a box: its bottom's back and front corners (y -0.1305, z 0.4353) and (y -0.1083, z 0.5117) are the magazine's own far
// box's (its LOD3 on its bone: (y -0.2296, z 0.0594) and (y -0.2069, z 0.1356)) moved by (0.0991 0.3759) and (0.0986 0.3761); and the receiver's
// opening (its edge at y -0.0060, x -0.0172 to 0.0123, back z 0.4126) holds the magazine's own edge at the same height (x -0.0163 to 0.0117, back z
// 0.4136). The game itself is what places it in play (the mod learns each seat from the game's own draws at rest: seatrest.h); this is the fitted
// hands' and the tables' magazine.
inline constexpr float kSeat[3] = {0.0f, 0.0989f, 0.3760f};

// Its iron sights: the rear notch on the rail's back block (its floor y 0.0842 at z 0.49 to 0.51), the front post at the rail's front (its top y 0.0843 at
// z 0.7007): the line level, at x -0.0023 (the bore's, as the muzzle's face has it: the gun lies 2.3 mm right of its frame's x 0). The game's one zoomed
// sample (FOV 40): the eye at (-0.002 0.084 0.127) looking 0.05 degrees up: on the line.
inline constexpr float kUp = 0.0f;
inline constexpr float kSights[3] = {-0.0023f, 0.0843f, 0.8374f};
inline constexpr float kMuzzle[3] = {-0.0023f, 0.026f, 0.8374f};
// Its port where its shell effect is in its data (FX_ShellEjection_Shotgun at (-0.01 0.041 0.44), turned 20 degrees about x and y; the hulls stay the
// game's own effect, as the M1014's: the mod has no 12-gauge casing).
inline constexpr float kPort[3] = {-0.010f, 0.041f, 0.440f};
// The game's palms at rest on it (its recordings).
inline constexpr float kRightPalm[3] = {-0.032f, -0.080f, 0.193f}, kLeftPalm[3] = {-0.006f, -0.034f, 0.589f};
// Its eye row (kHeldEyes): the game's zoomed eye's z on the frame, and the nearest of the gun on the line ahead of it: the rear sight's ears.
inline constexpr float kEyeZ = 0.127f, kRearZ = 0.490f;
// The magazine in its own vertices' space (its bone 7): the dropped one's bounds.
inline constexpr float kBoxMin[3] = {-0.0176f, -0.2301f, 0.0266f}, kBoxMax[3] = {0.0131f, -0.0697f, 0.1361f};

// Its magazine by hand, the AK family's (the user's list of paddle guns: no button, X does nothing): taken and rocked forward off its front edge at the
// receiver's underside, a fresh one from the belly hooked there and rocked home (25 degrees, the AK family's: the game's own reload rocks it out and in
// too, its angle unrecorded), a fresh one swept into the old one's back knocking it out. top: its top's middle (the feed lips stand 3.5 cm up inside
// the receiver, to y 0.029); grab: the middle of what's below the receiver (y -0.006 to -0.131): the fitted hand's middle; paddle: the release hanging
// behind the well (z 0.372 to 0.386, y -0.006 to -0.04), 2 cm behind the magazine's back; hook: its front edge at the receiver's underside; back: its
// back edge at the underside, its middle and its bottom. Its handle: the bolt's knob on the right (its end x -0.0447, y 0.032 to 0.041, z 0.473 to
// 0.479), the bolt 9.1 cm back with each shot and 10.0 cm in the game's own pull (its empty reload, over the top): no hold-open, racked after every
// empty magazine.
inline constexpr handmag::Gun kMag{.top = {-0.002f, 0.029f, 0.445f}, .grab = {-0.002f, -0.069f, 0.461f}, .paddle = {-0.002f, -0.025f, 0.386f},
                                   .hook = {-0.002f, -0.006f, 0.485f}, .mouth = -0.05f, .lever = {-0.044f, 0.036f, 0.476f}, .leverTravel = 0.100f,
                                   .rockIn = 25.0f * 3.14159265f / 180.0f, .handOffset = {0.0f, 0.0f, 0.0f}, .hold = magposes::kSaiga12kHold,
                                   .fingers = magposes::kSaiga12kFingers, .button = false, .noKnock = false,
                                   .back = {{-0.006f, 0.4073f}, {-0.069f, 0.4204f}, {-0.125f, 0.4331f}}, .leverHold = magposes::kSaiga12kLeverHold,
                                   .leverFingers = magposes::kSaiga12kLeverFingers, .leverUnderHold = magposes::kSaiga12kLeverUnderHold,
                                   .leverUnderFingers = magposes::kSaiga12kLeverUnderFingers};

}  // namespace saiga12krows

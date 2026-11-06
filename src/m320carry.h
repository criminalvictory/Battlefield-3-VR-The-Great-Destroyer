#pragma once

// The left hand holding a 40 mm round by its base end in a fingertip pinch, variant A of four (tools\basegrip.cpp): the user, 2026-11-05, after the fist round
// its body: "you don't need a whole fist ... at most three fingers ... at the end of the shell ... with his finger, middle finger, and thumb on that circle
// base. I'm okay with the ring finger and pinky folding", with four photographs of a water bottle held so. Variant A, as photos 2 and 4: the round straight out
// along the index finger's own line, the index and middle pads on its base end, the thumb's pad across from them.
// Fitted on the meshes offline: the index and middle fingers shaped joint by joint, the round put where both their pads touch it, the thumb opposed and closed
// at its knuckle and last joint until its pad touches it from the other side; the most natural such hand (the search's) with nothing else on the round.
// Into the round: the deepest the glove goes anywhere (all 23 bones' vertices) 1.00 mm (bone 2) into its outline, 0.95 into its own flat faces (2.5 the most).
// The pads touch it (gap, negative in): thumb -1.0 mm, index -1.0, middle -1.0; the rest of their last parts 1.4 6.5 8.2. Along the round from its
// base's face the pads touch at: thumb 9.2 mm, index 10.0, middle 15.2; the thumb's touch 150 degrees round the round from the index and middle's (180: straight across).
// Clear of it (mm): the palm 55.7, the ring finger 7.6, the little finger 31.5; the thumb's other parts 48.3 28.6, the index's 30.9 10.0, the middle's 24.6 4.6.
// The nose stands 8.6 cm out past all of the hand. The round's way: 58.8 degrees out of the palm's plane from along the fingers, 0.8 towards the little
// finger (12 degrees off the index's last part's line). Its turn about its own length is the game's pinch's as near as goes (frame 7350).
// Thumb: spread 20.1, out 59.7, roll 39.6, base -0.2, knuckle 34.2, last 27.6. Fingers (knuckle, middle, last; turn towards the thumb): index 53.4 7.9 5.2 0.1,
// middle 53.3 8.3 5.5 0.9, ring 55.1 62.2 37.0, little 59.9 62.6 37.3.
// Bone 1, the palm, is handpose::Pose's at width 1.15 (PoseJoints doesn't move it: 0e+00 off), so hands.cpp's re-anchoring on the palm leaves the round where
// it was fitted. In the form of src\m320carry.h (hands.cpp's m320::SetHold(kCarry, ...)): put in its place to try it.
// Made again by, from the project's folder in an x86 developer prompt:
//   cl /nologo /EHsc /O2 /std:c++17 /DNOMINMAX /DWIN32_LEAN_AND_MEAN /I src /I tools /Febuild\basegrip.exe /Fobuild\ tools\basegrip.cpp src\handpose.cpp
//   build\basegrip build <out folder> fit A starts=12

namespace m320carry {

// The round's bone (draw 1440's bone 0, the space kRoundBaseOnBone and kRoundNoseOnBone are in) into the left glove's bind space (3x4 rows: rotation | translation,
// rigid).
constexpr float kRoundInGlove[12] = {0.735869f, -0.599178f, -0.315408f, 0.550244f, -0.095991f, 0.368791f, -0.924542f, 1.311504f, 0.670285f, 0.710619f, 0.213866f, 0.211693f};

// Each left glove bone (0-22) as handpose::PoseJoints gives it at width 1.15 (bind space -> glove space): the fitted pinch.
constexpr float kFingers[23][12] = {
    {1.030052f, -0.018041f, -0.032046f, 0.010304f, -0.018041f, 1.010830f, 0.019238f, -0.006186f, -0.032046f, 0.019238f, 1.034172f, -0.010988f},
    {1.060103f, -0.036081f, -0.064091f, 0.020608f, -0.036081f, 1.021660f, 0.038475f, -0.012372f, -0.064091f, 0.038475f, 1.068344f, -0.021976f},
    {-0.017011f, -0.999231f, 0.035026f, 1.554716f, 0.243437f, -0.038116f, -0.969157f, 1.241072f, 0.969757f, -0.007960f, 0.243901f, -0.310270f},
    {-0.233240f, -0.177192f, -0.909225f, 1.076480f, -0.653519f, -0.694213f, 0.246270f, 2.082279f, -0.656506f, 0.675528f, -0.070040f, -0.112483f},
    {1.030052f, -0.018041f, -0.032046f, 0.010304f, -0.018041f, 1.010830f, 0.019238f, -0.006186f, -0.032046f, 0.019238f, 1.034172f, -0.010988f},
    {1.030052f, -0.018041f, -0.032046f, 0.010304f, -0.018041f, 1.010830f, 0.019238f, -0.006186f, -0.032046f, 0.019238f, 1.034172f, -0.010988f},
    {1.060103f, -0.036081f, -0.064091f, 0.020608f, -0.036081f, 1.021660f, 0.038475f, -0.012372f, -0.064091f, 0.038475f, 1.068344f, -0.021976f},
    {0.698011f, 0.508643f, -0.411006f, -0.287901f, -0.632898f, 0.635661f, -0.408329f, 0.846506f, 0.170457f, 0.556724f, 0.741068f, -0.638447f},
    {1.060103f, -0.036081f, -0.064091f, 0.020608f, -0.036081f, 1.021660f, 0.038475f, -0.012372f, -0.064091f, 0.038475f, 1.068344f, -0.021976f},
    {0.693187f, 0.542420f, -0.384105f, -0.335517f, -0.650507f, 0.625996f, -0.401622f, 0.863579f, 0.137179f, 0.538871f, 0.752732f, -0.600199f},
    {-0.199527f, -0.119661f, -0.925754f, 1.019169f, -0.713025f, -0.653797f, 0.181216f, 2.080835f, -0.602440f, 0.725896f, -0.068540f, -0.168633f},
    {0.693058f, 0.488869f, -0.437980f, -0.257041f, -0.633066f, 0.614392f, -0.436400f, 0.863628f, 0.173342f, 0.593484f, 0.715741f, -0.676898f},
    {1.060103f, -0.036081f, -0.064091f, 0.020608f, -0.036081f, 1.021660f, 0.038475f, -0.012372f, -0.064091f, 0.038475f, 1.068344f, -0.021976f},
    {0.648588f, 0.511409f, -0.479877f, -0.252503f, -0.684091f, 0.552764f, -0.442875f, 0.950269f, 0.154405f, 0.634492f, 0.682513f, -0.709545f},
    {1.060103f, -0.036081f, -0.064091f, 0.020608f, -0.036081f, 1.021660f, 0.038475f, -0.012372f, -0.064091f, 0.038475f, 1.068344f, -0.021976f},
    {0.563239f, 0.589738f, -0.507183f, -0.278047f, -0.773001f, 0.448747f, -0.421219f, 1.126658f, 0.087846f, 0.653687f, 0.663914f, -0.669581f},
    {0.012412f, 0.259128f, -0.919340f, 0.478927f, -0.906531f, -0.346841f, -0.166042f, 1.965913f, -0.300891f, 0.884433f, 0.139599f, -0.589249f},
    {0.082860f, -0.905210f, -0.416784f, 1.522910f, 0.291063f, 0.421969f, -0.858606f, 0.683848f, 0.953098f, -0.050167f, 0.298441f, -0.270341f},
    {0.219029f, -0.516155f, -0.828004f, 1.114036f, 0.277566f, 0.846509f, -0.454267f, 0.124964f, 0.935395f, -0.130329f, 0.328680f, -0.178895f},
    {0.617227f, 0.575474f, -0.458394f, -0.308007f, -0.727504f, 0.522396f, -0.417337f, 1.021609f, 0.110697f, 0.610249f, 0.700819f, -0.647424f},
    {0.068650f, 0.294646f, -0.905329f, 0.424323f, -0.920476f, -0.272771f, -0.216829f, 1.911045f, -0.243231f, 0.898596f, 0.167097f, -0.622158f},
    {0.621348f, 0.537235f, -0.490069f, -0.253251f, -0.714034f, 0.529498f, -0.425612f, 1.010085f, 0.144627f, 0.635401f, 0.680828f, -0.691001f},
    {0.566535f, 0.547971f, -0.541875f, -0.217869f, -0.762346f, 0.453423f, -0.429645f, 1.119060f, 0.121250f, 0.683271f, 0.637665f, -0.715281f},
};

}  // namespace m320carry

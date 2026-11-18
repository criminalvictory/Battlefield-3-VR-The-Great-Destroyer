#pragma once

// The left hand holding a 12-gauge shell (the M1014's: its own draw 450 from 13701, 6.4 cm long, 2.1 cm across) by its base end in a fingertip pinch: the grip the
// user chose for the grenade launcher's round (m320carry.h, variant A of tools\basegrip.cpp: "at most three fingers ... at the end of the shell ... with his
// finger, middle finger, and thumb on that circle base"), fitted again on the shell itself: straight out along the index finger's own line, the index and middle
// pads on its base end, the thumb's pad across from them, the ring and little fingers folded and clear. Used for a shell carried from the belly and pushed up
// the loading port or laid into the ejection port (shellload.h); the right hand's is this flipped (handpose::Mirror, LeftToRight).
// Fitted on the meshes offline, not from the game's own hand (the level's bare arms have another skeleton): look at build\shellgrip\basegrip_A.bmp.
// Into the shell: the deepest the glove goes anywhere (all 23 bones' vertices) 1.00 mm (bone 15) into its outline, 0.09 into its own flat faces (2.5 the most).
// The pads touch it (gap, negative in): thumb -1.0 mm, index -1.0, middle -1.0; the rest of their last parts 0.3 5.4 7.9. Along the shell from its
// base's face the pads touch at: thumb 2.6 mm, index 10.0, middle 14.4; the thumb's touch 170 degrees round the shell from the index and middle's (180: straight across).
// Clear of it (mm): the palm 53.7, the ring finger 22.4, the little finger 46.0; the thumb's other parts 50.7 34.4, the index's 33.1 10.4, the middle's 28.0 6.4.
// The nose stands 3.2 cm out past all of the hand. The shell's way: 63.2 degrees out of the palm's plane from along the fingers, 0.0 towards the little
// finger (12 degrees off the index's last part's line).
// Thumb: spread 20.5, out 57.3, roll 43.9, base 7.5, knuckle 39.0, last 31.6. Fingers (knuckle, middle, last; turn towards the thumb): index 64.0 4.5 2.6 0.0,
// middle 63.9 5.0 2.8 1.0, ring 55.5 65.6 39.2, little 59.9 62.2 37.0.
// Bone 1, the palm, is handpose::Pose's at width 1.15 (PoseJoints doesn't move it), so a re-anchoring on the palm leaves the shell where it was fitted.
// Made again by, from the project's folder in an x86 developer prompt (the mesh dumps mesh_450_13701_* and the gloves' in build\):
//   cl /nologo /EHsc /O2 /std:c++17 /DNOMINMAX /DWIN32_LEAN_AND_MEAN /I src /I tools /Febuild\basegrip.exe /Fobuild\ tools\basegrip.cpp src\handpose.cpp
//   build\basegrip build <out folder> fit A shell12g starts=12        (then its two tables copied here under these names)

namespace shellposes {

// The shell's bone (draw 450's bone 0: the shell's line at x -0.0013, y -0.1026 on it, its base at z 0.049, its nose at 0.113) into the left glove's bind space
// (3x4 rows: rotation | translation, rigid).
constexpr float kShellInGlove[12] = {0.685538f, -0.624611f, -0.374031f, 0.456120f, -0.154214f, 0.377513f, -0.913073f, 1.117380f, 0.711517f, 0.683627f, 0.162476f, 0.297689f};

// Each left glove bone (0-22) as handpose::PoseJoints gives it at width 1.15 (bind space -> glove space): the fitted pinch.
constexpr float kFingers[23][12] = {
    {1.030052f, -0.018041f, -0.032046f, 0.010304f, -0.018041f, 1.010830f, 0.019238f, -0.006186f, -0.032046f, 0.019238f, 1.034172f, -0.010988f},
    {1.060103f, -0.036081f, -0.064091f, 0.020608f, -0.036081f, 1.021660f, 0.038475f, -0.012372f, -0.064091f, 0.038475f, 1.068344f, -0.021976f},
    {-0.088978f, -0.943866f, 0.318083f, 1.466184f, 0.277666f, -0.330195f, -0.902137f, 1.526422f, 0.956537f, 0.008051f, 0.291463f, -0.330438f},
    {-0.230415f, -0.167622f, -0.911756f, 1.065431f, -0.660981f, -0.690218f, 0.237426f, 2.084455f, -0.650001f, 0.682033f, -0.067629f, -0.123594f},
    {1.030052f, -0.018041f, -0.032046f, 0.010304f, -0.018041f, 1.010830f, 0.019238f, -0.006186f, -0.032046f, 0.019238f, 1.034172f, -0.010988f},
    {1.030052f, -0.018041f, -0.032046f, 0.010304f, -0.018041f, 1.010830f, 0.019238f, -0.006186f, -0.032046f, 0.019238f, 1.034172f, -0.010988f},
    {1.060103f, -0.036081f, -0.064091f, 0.020608f, -0.036081f, 1.021660f, 0.038475f, -0.012372f, -0.064091f, 0.038475f, 1.068344f, -0.021976f},
    {0.598258f, 0.541911f, -0.512905f, -0.244271f, -0.734969f, 0.498530f, -0.427198f, 1.057061f, 0.136904f, 0.656144f, 0.662775f, -0.709290f},
    {1.060103f, -0.036081f, -0.064091f, 0.020608f, -0.036081f, 1.021660f, 0.038475f, -0.012372f, -0.064091f, 0.038475f, 1.068344f, -0.021976f},
    {0.590399f, 0.584155f, -0.482361f, -0.301162f, -0.751198f, 0.484840f, -0.420835f, 1.077549f, 0.098059f, 0.632586f, 0.682399f, -0.664217f},
    {-0.224231f, -0.195826f, -0.906930f, 1.107747f, -0.655916f, -0.688897f, 0.252765f, 2.066234f, -0.656235f, 0.675106f, -0.090246f, -0.079366f},
    {0.689485f, 0.490277f, -0.442027f, -0.255675f, -0.637229f, 0.609316f, -0.437455f, 0.871657f, 0.172332f, 0.597542f, 0.712602f, -0.680020f},
    {1.060103f, -0.036081f, -0.064091f, 0.020608f, -0.036081f, 1.021660f, 0.038475f, -0.012372f, -0.064091f, 0.038475f, 1.068344f, -0.021976f},
    {0.648422f, 0.511456f, -0.480051f, -0.252425f, -0.684261f, 0.552530f, -0.442905f, 0.950622f, 0.154346f, 0.634658f, 0.682371f, -0.709663f},
    {1.060103f, -0.036081f, -0.064091f, 0.020608f, -0.036081f, 1.021660f, 0.038475f, -0.012372f, -0.064091f, 0.038475f, 1.068344f, -0.021976f},
    {0.515356f, 0.597667f, -0.547433f, -0.255029f, -0.807995f, 0.382438f, -0.421136f, 1.218615f, 0.063543f, 0.687756f, 0.631193f, -0.689136f},
    {0.016448f, 0.263514f, -0.918029f, 0.471670f, -0.907905f, -0.341133f, -0.170309f, 1.961637f, -0.296524f, 0.885357f, 0.143044f, -0.593521f},
    {0.006990f, -0.977011f, -0.213021f, 1.596825f, 0.358722f, 0.201298f, -0.911469f, 0.906421f, 0.933407f, -0.070045f, 0.351886f, -0.249553f},
    {0.161890f, -0.632281f, -0.757621f, 1.258348f, 0.371347f, 0.750348f, -0.546861f, 0.204544f, 0.914260f, -0.192811f, 0.356274f, -0.103967f},
    {0.543234f, 0.594077f, -0.523880f, -0.274016f, -0.788324f, 0.420480f, -0.422151f, 1.167422f, 0.076986f, 0.668388f, 0.650216f, -0.681864f},
    {0.033303f, 0.258729f, -0.918204f, 0.485601f, -0.911823f, -0.322991f, -0.181522f, 1.950281f, -0.280392f, 0.892990f, 0.136039f, -0.587286f},
    {0.548127f, 0.549229f, -0.559068f, -0.211027f, -0.776746f, 0.428931f, -0.428828f, 1.155182f, 0.114269f, 0.697919f, 0.623209f, -0.730068f},
    {0.519263f, 0.550895f, -0.584407f, -0.188770f, -0.798255f, 0.388809f, -0.427581f, 1.209385f, 0.099943f, 0.719768f, 0.600402f, -0.738139f},
};

}  // namespace shellposes

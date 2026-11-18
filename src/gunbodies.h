#pragma once
// Every gun's body when it's let go of (free hands; loose.h), fitted by tools/gunbodyfit.exe from the game's own first-person meshes (build\mesh_*.bin).
// Written by it, never by hand: run "gunbodyfit build src\gunbodies.h" again. The M416 keeps gunbody.h's (made by hand, tried in the headset).
// In each gun's frame bone's space (x to its left, y up, z to the muzzle; metres). Its middle is its kept triangles' area-weighted middle (its box
// for loose is its core bone's size about it); its hull points are its mesh's own vertices, furthest out every way (src/bodyfit.h). Under a
// narrow bottom those along it no wider than its core are on one line along z (its keel) with its middle beside it, so stood upright it tips
// over, and the same vertices 5 mm up (its guards) hold it on its side; a bottom 4 cm wide or more has none (it may stand on it). Its points
// before restFrom matter most: an attachment's go before the rest (bodyfit::WithExtra).
#include <cstdint>
#include <cstring>
#include "gunbody.h"
namespace gunbodies {
enum Sound : int { kMetal = 0, kPolymer = 1, kHeavy = 2 };
struct Body {
    const char* shape;          // skin.cpp's HeldShape(), or "smaw" (its case on) / "smaw_bare"
    const loose::Spec* spec;
    float hullMin[3], hullMax[3];
    int sound;
    int restFrom;               // spec->hull from here on is the fit's "the rest" (the fit's step 8, bodyfit.h): an attachment's points go before it
};
// An attachment drawn apart from the gun (a sight, a handle, an underbarrel): its draw, and at most 8 extreme points in its own bone 0's space.
struct Attach {
    uint32_t count, start;
    int base;
    int points;
    float point[8][3];
};

// AN-94: 16188@0, frame bone 0, core bone 0 (3955 of 6012 vertices), left out: bone 5;
// kept 5554 vertices; its middle (0.000 0.022 0.434), its footprint 2.5 cm wide, its keel at x 0.006; 20 hull points, the rest from 12.
// not dumped yet (weapon.txt "dump N S", then fit again): its rounds 312@16278.
inline constexpr float kAn94Hull[20][3] = {
    {0.0056f, -0.0522f, 0.0002f},  // keel back
    {0.0056f, -0.1618f, 0.4098f},  // keel middle
    {0.0056f, 0.0496f, 0.9482f},  // front end
    {-0.0055f, 0.0949f, 0.3069f},  // gap
    {0.0016f, 0.0461f, 0.0001f},  // gap
    {-0.0049f, 0.1046f, 0.8373f},  // gap
    {-0.0346f, 0.0683f, 0.4969f},  // gap
    {-0.0126f, -0.1539f, 0.4141f},  // guard
    {-0.0112f, -0.0327f, 0.0044f},  // gap
    {0.0051f, 0.0764f, 0.9277f},  // gap
    {-0.0081f, 0.0539f, 0.9321f},  // gap
    {0.0181f, 0.0570f, 0.3020f},  // gap
    {0.0095f, 0.0990f, 0.8276f},  // gap
    {0.0109f, 0.0247f, 0.0045f},  // gap
    {-0.0045f, 0.0610f, 0.9482f},  // gap
    {-0.0118f, 0.0247f, 0.0045f},  // gap
    {0.0138f, 0.0718f, 0.2671f},  // gap
    {0.0104f, -0.0327f, 0.0044f},  // gap
    {-0.0176f, 0.0834f, 0.3123f},  // gap
    {0.0108f, 0.0538f, 0.8349f},  // gap
};
inline constexpr loose::Spec kAn94{.name = "an94gun", .boxMin = {-0.0214f, -0.0677f, 0.0225f}, .boxMax = {0.0206f, 0.1120f, 0.8450f}, .sweep = 0.0f, .sag = 0.0f,
    .massEmpty = 3.8500f, .massRound = 0.0102f, .capacity = 30, .bounce = 1.0000f, .keep = true, .hull = kAn94Hull, .hullPoints = 20};

// RPK-74M: 24492@0, frame bone 3, core bone 3 (7491 of 8605 vertices), nothing left out;
// kept 8605 vertices; its middle (0.000 -0.002 0.437), its footprint 3.7 cm wide, its keel at x 0.006; 22 hull points, the rest from 14.
// not dumped yet (weapon.txt "dump N S", then fit again): its rounds 312@24492.
inline constexpr float kRpkHull[22][3] = {
    {0.0058f, -0.1268f, -0.0288f},  // keel back
    {0.0058f, -0.2170f, 0.5478f},  // keel middle
    {0.0001f, -0.0100f, -0.0325f},  // back end
    {0.0021f, 0.0264f, 1.1240f},  // front end
    {-0.0173f, 0.0834f, 0.4951f},  // gap
    {-0.0066f, 0.0898f, 1.0977f},  // gap
    {-0.0525f, 0.0382f, 0.4824f},  // gap
    {0.0223f, 0.0464f, 0.2423f},  // gap
    {-0.0210f, -0.0854f, -0.0234f},  // gap
    {-0.0183f, -0.2085f, 0.5495f},  // guard
    {0.0218f, -0.0665f, -0.0236f},  // gap
    {-0.0075f, 0.0684f, 0.2510f},  // gap
    {0.0086f, 0.0865f, 1.1006f},  // gap
    {0.0184f, -0.2085f, 0.5495f},  // guard
    {-0.0124f, -0.1180f, -0.0282f},  // gap
    {-0.0153f, -0.0229f, -0.0272f},  // gap
    {0.0120f, 0.0861f, 0.4792f},  // gap
    {0.0186f, -0.1031f, -0.0238f},  // gap
    {0.0177f, -0.0304f, -0.0264f},  // gap
    {0.0117f, 0.0330f, 1.1045f},  // gap
    {0.0178f, 0.0831f, 0.4951f},  // gap
    {-0.0207f, 0.0490f, 0.2351f},  // gap
};
inline constexpr loose::Spec kRpk{.name = "rpkgun", .boxMin = {-0.0260f, -0.1140f, -0.1416f}, .boxMax = {0.0257f, 0.1100f, 1.0149f}, .sweep = 0.0f, .sag = 0.0f,
    .massEmpty = 4.8000f, .massRound = 0.0102f, .capacity = 45, .bounce = 1.0000f, .keep = true, .hull = kRpkHull, .hullPoints = 22};

// AEK-971: 21585@0, frame bone 4, core bone 4 (5851 of 7232 vertices), left out: bone 3;
// kept 6733 vertices; its middle (0.000 0.016 0.442), its footprint 2.5 cm wide, its keel at x 0.006; 20 hull points, the rest from 15.
// not dumped yet (weapon.txt "dump N S", then fit again): its rounds 318@21615.
inline constexpr float kAek971Hull[20][3] = {
    {0.0065f, -0.0878f, -0.0627f},  // keel back
    {0.0065f, -0.1788f, 0.4391f},  // keel middle
    {0.0000f, 0.0260f, 0.9897f},  // front end
    {-0.0080f, 0.0741f, 0.2612f},  // gap
    {0.0041f, 0.0238f, -0.0604f},  // gap
    {0.0017f, 0.0983f, 0.8848f},  // gap
    {-0.0331f, 0.0538f, 0.4509f},  // gap
    {-0.0126f, -0.1712f, 0.4434f},  // guard
    {-0.0116f, -0.0485f, -0.0591f},  // gap
    {0.0126f, 0.0387f, 0.9839f},  // gap
    {-0.0137f, 0.0335f, 0.9839f},  // gap
    {0.0157f, 0.0648f, 0.2529f},  // gap
    {0.0094f, 0.0964f, 0.5415f},  // gap
    {-0.0092f, 0.0164f, -0.0604f},  // gap
    {-0.0094f, -0.0804f, -0.0627f},  // gap
    {0.0145f, -0.0323f, -0.0577f},  // gap
    {0.0189f, 0.0745f, 0.6709f},  // gap
    {0.0000f, 0.0471f, 0.9839f},  // gap
    {-0.0095f, 0.0964f, 0.5415f},  // gap
    {0.0219f, 0.0305f, 0.2913f},  // gap
};
inline constexpr loose::Spec kAek971{.name = "aek971gun", .boxMin = {-0.0232f, -0.0900f, -0.0846f}, .boxMax = {0.0242f, 0.1228f, 0.9695f}, .sweep = 0.0f, .sag = 0.0f,
    .massEmpty = 3.3000f, .massRound = 0.0102f, .capacity = 30, .bounce = 1.0000f, .keep = true, .hull = kAek971Hull, .hullPoints = 20};

// AK-74M: 21723@0, frame bone 2, core bone 2 (6776 of 7515 vertices), nothing left out;
// kept 7515 vertices; its middle (0.000 -0.001 0.417), its footprint 2.5 cm wide, its keel at x 0.006; 19 hull points, the rest from 14.
// not dumped yet (weapon.txt "dump N S", then fit again): its magazine's other piece 894@22035, its rounds 312@21723.
inline constexpr float kAk74Hull[19][3] = {
    {0.0058f, -0.1967f, 0.4407f},  // keel back
    {0.0058f, -0.0623f, -0.0052f},  // back end
    {0.0024f, 0.0168f, 0.9355f},  // front end
    {-0.0003f, 0.0370f, 0.0040f},  // gap
    {-0.0047f, 0.0720f, 0.8545f},  // gap
    {-0.0404f, 0.0363f, 0.4980f},  // gap
    {-0.0162f, -0.0455f, 0.0128f},  // gap
    {-0.0126f, -0.1890f, 0.4451f},  // guard
    {0.0153f, 0.0680f, 0.5093f},  // gap
    {0.0231f, 0.0144f, 0.6665f},  // gap
    {-0.0107f, 0.0708f, 0.4985f},  // gap
    {-0.0104f, 0.0225f, 0.9277f},  // gap
    {0.0150f, 0.0152f, 0.0080f},  // gap
    {-0.0135f, 0.0288f, 0.0085f},  // gap
    {0.0156f, -0.0455f, 0.0128f},  // gap
    {-0.0117f, -0.0557f, -0.0050f},  // gap
    {0.0074f, 0.0299f, 0.9277f},  // gap
    {-0.0003f, 0.0598f, 0.2639f},  // gap
    {-0.0142f, 0.0146f, 0.0032f},  // gap
};
inline constexpr loose::Spec kAk74{.name = "ak74gun", .boxMin = {-0.0226f, -0.0933f, -0.0535f}, .boxMax = {0.0222f, 0.0912f, 0.8873f}, .sweep = 0.0f, .sag = 0.0f,
    .massEmpty = 3.4000f, .massRound = 0.0102f, .capacity = 30, .bounce = 1.0000f, .keep = true, .hull = kAk74Hull, .hullPoints = 19};

// G3A3: 23064@0, frame bone 0, core bone 0 (6885 of 7794 vertices), nothing left out;
// kept 7794 vertices; its middle (0.000 0.030 0.496), its footprint 2.9 cm wide, its keel at x 0.006; 22 hull points, the rest from 15.
inline constexpr float kG3a3Hull[22][3] = {
    {0.0062f, -0.0593f, -0.0207f},  // keel back
    {0.0062f, -0.1225f, 0.4496f},  // keel middle
    {0.0062f, -0.1126f, 0.5269f},  // keel front
    {0.0117f, 0.0556f, -0.0294f},  // back end
    {0.0087f, 0.0437f, 1.0303f},  // front end
    {-0.0043f, 0.1134f, 0.8921f},  // gap
    {-0.0107f, 0.1091f, 0.2734f},  // gap
    {-0.0177f, 0.0471f, -0.0238f},  // gap
    {-0.0296f, 0.0341f, 0.5151f},  // gap
    {-0.0109f, 0.0473f, 1.0303f},  // gap
    {-0.0152f, -0.0508f, -0.0211f},  // gap
    {0.0233f, 0.0843f, 0.7893f},  // gap
    {0.0111f, 0.1088f, 0.2732f},  // gap
    {-0.0142f, -0.1064f, 0.5268f},  // guard
    {-0.0083f, 0.0620f, -0.0259f},  // gap
    {0.0107f, 0.1072f, 0.8940f},  // gap
    {0.0177f, -0.0426f, -0.0200f},  // gap
    {0.0062f, -0.1137f, 0.2930f},  // gap
    {-0.0126f, -0.1087f, 0.2930f},  // guard
    {0.0001f, 0.0583f, 1.0303f},  // gap
    {-0.0132f, -0.1175f, 0.4496f},  // guard
    {0.0312f, 0.0422f, 0.6050f},  // gap
};
inline constexpr loose::Spec kG3a3{.name = "g3a3gun", .boxMin = {-0.0303f, -0.0842f, -0.0337f}, .boxMax = {0.0306f, 0.1442f, 1.0260f}, .sweep = 0.0f, .sag = 0.0f,
    .massEmpty = 4.4000f, .massRound = 0.0240f, .capacity = 20, .bounce = 1.0000f, .keep = true, .hull = kG3a3Hull, .hullPoints = 22};

// KH2002: 15237@0, frame bone 3, core bone 3 (4920 of 5691 vertices), nothing left out;
// kept 5691 vertices; its middle (0.000 -0.001 0.294), its footprint 2.7 cm wide, its keel at x 0.006; 22 hull points, the rest from 16.
// not dumped yet (weapon.txt "dump N S", then fit again): its rounds 312@15237.
inline constexpr float kKh2002Hull[22][3] = {
    {0.0059f, -0.0750f, -0.0021f},  // keel back
    {0.0059f, -0.1112f, 0.1331f},  // keel middle
    {0.0059f, -0.0051f, 0.7104f},  // keel front
    {0.0000f, 0.0310f, -0.0233f},  // back end
    {-0.0085f, 0.0887f, 0.6064f},  // gap
    {0.0006f, 0.0864f, 0.2273f},  // gap
    {0.0059f, -0.1063f, 0.3333f},  // gap
    {-0.0265f, -0.0278f, -0.0140f},  // gap
    {0.0271f, -0.0102f, -0.0161f},  // gap
    {-0.0136f, -0.0996f, 0.3333f},  // guard
    {0.0085f, 0.0881f, 0.6099f},  // gap
    {-0.0096f, 0.0045f, 0.7104f},  // gap
    {0.0179f, 0.0294f, -0.0186f},  // gap
    {0.0257f, 0.0039f, 0.5044f},  // gap
    {-0.0129f, -0.1057f, 0.1331f},  // guard
    {-0.0245f, 0.0131f, -0.0182f},  // gap
    {0.0206f, -0.0569f, -0.0074f},  // guard
    {-0.0263f, 0.0020f, 0.5005f},  // gap
    {0.0048f, 0.0129f, 0.7104f},  // gap
    {-0.0206f, -0.0569f, -0.0074f},  // guard
    {-0.0160f, 0.0286f, -0.0228f},  // gap
    {0.0000f, -0.0469f, -0.0151f},  // gap
};
inline constexpr loose::Spec kKh2002{.name = "kh2002gun", .boxMin = {-0.0272f, -0.0988f, -0.0728f}, .boxMax = {0.0269f, 0.0962f, 0.6614f}, .sweep = 0.0f, .sag = 0.0f,
    .massEmpty = 3.7000f, .massRound = 0.0123f, .capacity = 30, .bounce = 1.0000f, .keep = true, .hull = kKh2002Hull, .hullPoints = 22};

// M16A4: 17013@312, frame bone 4, core bone 4 (5090 of 6365 vertices), nothing left out;
// kept 6470 vertices; its middle (-0.003 -0.011 0.458), its footprint 2.6 cm wide, its keel at x 0.003; 22 hull points, the rest from 15.
// added its rounds (312@0).
inline constexpr float kM16a4Hull[22][3] = {
    {0.0029f, -0.1025f, -0.0182f},  // keel back
    {0.0029f, -0.2134f, 0.4740f},  // keel middle
    {0.0029f, 0.0182f, 1.0459f},  // front end
    {-0.0004f, 0.0455f, -0.0181f},  // gap
    {-0.0281f, 0.0477f, 0.8013f},  // gap
    {0.0309f, 0.0338f, 0.8008f},  // gap
    {-0.0058f, 0.0384f, 1.0459f},  // gap
    {-0.0198f, 0.0003f, -0.0181f},  // gap
    {0.0190f, 0.0003f, -0.0181f},  // gap
    {0.0234f, 0.0548f, 0.2890f},  // gap
    {-0.0369f, 0.0259f, 0.2869f},  // gap
    {0.0071f, 0.0591f, 0.7988f},  // gap
    {-0.0146f, -0.1962f, 0.4704f},  // guard
    {0.0101f, 0.0341f, 1.0459f},  // gap
    {-0.0175f, -0.0726f, -0.0181f},  // gap
    {-0.0234f, 0.0548f, 0.2888f},  // gap
    {0.0143f, 0.0349f, -0.0181f},  // gap
    {0.0167f, -0.0726f, -0.0181f},  // gap
    {-0.0152f, 0.0349f, -0.0181f},  // gap
    {-0.0364f, 0.0255f, 0.8022f},  // gap
    {0.0142f, -0.1959f, 0.4703f},  // guard
    {-0.0080f, 0.0591f, 0.7988f},  // gap
};
inline constexpr loose::Spec kM16a4{.name = "m16a4gun", .boxMin = {-0.0369f, -0.1093f, -0.0735f}, .boxMax = {0.0308f, 0.0877f, 0.9905f}, .sweep = 0.0f, .sag = 0.0f,
    .massEmpty = 3.4000f, .massRound = 0.0123f, .capacity = 30, .bounce = 1.0000f, .keep = true, .hull = kM16a4Hull, .hullPoints = 22};

// M4A1: 22194@312, frame bone 2, core bone 2 (6836 of 8261 vertices), nothing left out;
// kept 8261 vertices; its middle (0.000 -0.015 0.403), its footprint 2.5 cm wide, its keel at x 0.006; 22 hull points, the rest from 17.
// not dumped yet (weapon.txt "dump N S", then fit again): its rounds 312@0 (in its own buffer).
inline constexpr float kM4a1Hull[22][3] = {
    {0.0055f, -0.0917f, 0.0354f},  // keel back
    {0.0055f, -0.2048f, 0.4831f},  // keel middle
    {0.0055f, -0.1994f, 0.5028f},  // keel front
    {0.0000f, 0.0502f, 0.0239f},  // back end
    {-0.0096f, 0.0267f, 0.9087f},  // front end
    {0.0310f, 0.0321f, 0.7031f},  // gap
    {-0.0369f, 0.0243f, 0.2886f},  // gap
    {-0.0210f, -0.0290f, 0.0302f},  // gap
    {0.0117f, 0.0267f, 0.9082f},  // gap
    {-0.0079f, 0.0573f, 0.7031f},  // gap
    {0.0214f, 0.0210f, 0.0262f},  // gap
    {-0.0128f, -0.1922f, 0.5095f},  // guard
    {-0.0210f, 0.0275f, 0.0257f},  // gap
    {-0.0143f, -0.0786f, 0.0343f},  // gap
    {0.0000f, 0.0384f, 0.9082f},  // gap
    {0.0238f, 0.0528f, 0.2907f},  // gap
    {-0.0318f, 0.0321f, 0.7031f},  // gap
    {0.0168f, 0.0415f, 0.0246f},  // gap
    {-0.0120f, -0.1988f, 0.4774f},  // guard
    {-0.0238f, 0.0528f, 0.2905f},  // gap
    {0.0143f, -0.0786f, 0.0343f},  // gap
    {0.0072f, 0.0573f, 0.7031f},  // gap
};
inline constexpr loose::Spec kM4a1{.name = "m4a1gun", .boxMin = {-0.0344f, -0.1138f, -0.0391f}, .boxMax = {0.0335f, 0.0844f, 0.8457f}, .sweep = 0.0f, .sag = 0.0f,
    .massEmpty = 2.9000f, .massRound = 0.0123f, .capacity = 30, .bounce = 1.0000f, .keep = true, .hull = kM4a1Hull, .hullPoints = 22};

// M240: 27588@2364, frame bone 3, core bone 3 (8829 of 10685 vertices), nothing left out;
// kept 11870 vertices; its middle (0.003 -0.016 0.560), its footprint 21.0 cm wide, no keel (it may stand on it); 22 hull points, the rest from 19.
// added its box's tape (60@2304).
// added its belt lying in the feed (2304@0).
inline constexpr float kM240Hull[22][3] = {
    {0.0852f, -0.1706f, 0.5641f},  // rest face
    {0.0852f, -0.1681f, 0.4480f},  // rest face
    {0.0951f, -0.0291f, 0.5591f},  // rest face
    {0.0507f, 0.0693f, 0.5556f},  // rest face
    {0.0483f, 0.0717f, 0.4768f},  // rest face
    {-0.0837f, 0.0854f, 0.6510f},  // rest face
    {0.0113f, 0.0261f, 1.1973f},  // gap
    {-0.0068f, 0.0677f, -0.0275f},  // gap
    {-0.1152f, -0.1623f, 0.4647f},  // gap
    {0.0001f, -0.0440f, -0.0275f},  // gap
    {0.0080f, 0.0851f, 1.1104f},  // gap
    {-0.1105f, -0.1669f, 0.5520f},  // gap
    {-0.0113f, 0.0350f, 1.1973f},  // gap
    {0.0185f, 0.0520f, -0.0275f},  // gap
    {-0.1186f, -0.0907f, 0.4647f},  // gap
    {0.0109f, 0.0900f, 0.2421f},  // gap
    {-0.0088f, 0.0845f, 1.1104f},  // gap
    {-0.0018f, 0.0184f, 1.1973f},  // gap
    {-0.0183f, 0.0520f, -0.0275f},  // gap
    {-0.0085f, -0.1388f, 0.2445f},  // gap
    {-0.0707f, 0.0965f, 0.6449f},  // gap
    {-0.1186f, -0.1541f, 0.5520f},  // gap
};
inline constexpr loose::Spec kM240{.name = "m240gun", .boxMin = {-0.0404f, -0.1307f, -0.0523f}, .boxMax = {0.0461f, 0.0992f, 1.1725f}, .sweep = 0.0f, .sag = 0.0f,
    .massEmpty = 12.5000f, .massRound = 0.0270f, .capacity = 100, .bounce = 0.8000f, .keep = true, .hull = kM240Hull, .hullPoints = 22};

// Stinger: 20298@354, frame bone 0, core bone 0 (3947 of 6922 vertices), nothing left out;
// kept 6922 vertices; its middle (-0.011 0.038 0.647), its footprint 7.4 cm wide, no keel (it may stand on it); 22 hull points, the rest from 22.
inline constexpr float kFim92Hull[22][3] = {
    {0.0064f, -0.1736f, 0.8677f},  // rest face
    {0.0000f, -0.1763f, 0.6250f},  // rest face
    {-0.0307f, -0.1736f, 0.8311f},  // rest face
    {-0.1767f, 0.0673f, 0.8111f},  // rest face
    {0.1456f, 0.0755f, 0.6832f},  // rest face
    {-0.0459f, 0.1996f, 0.8139f},  // rest face
    {0.0051f, 0.1013f, -0.3455f},  // gap
    {0.0117f, 0.0696f, 1.1738f},  // gap
    {-0.1741f, 0.1983f, 1.0789f},  // gap
    {-0.0453f, -0.0200f, -0.3455f},  // gap
    {0.0634f, 0.0059f, -0.3455f},  // gap
    {-0.1767f, 0.0673f, 1.0816f},  // gap
    {-0.0476f, 0.1984f, 1.0785f},  // gap
    {0.0206f, -0.1119f, 1.0830f},  // gap
    {0.1183f, 0.1034f, 0.9298f},  // gap
    {-0.0641f, 0.0589f, -0.3455f},  // gap
    {0.0436f, -0.1736f, 0.8311f},  // gap
    {-0.1760f, 0.1996f, 0.8113f},  // gap
    {0.0585f, 0.0684f, -0.3455f},  // gap
    {0.0457f, 0.0213f, 1.1631f},  // gap
    {0.1310f, 0.1150f, 0.6576f},  // gap
    {-0.0244f, -0.0006f, 1.1729f},  // gap
};
inline constexpr loose::Spec kFim92{.name = "fim92gun", .boxMin = {-0.0801f, -0.1007f, -0.1127f}, .boxMax = {0.0576f, 0.1768f, 1.4066f}, .sweep = 0.0f, .sag = 0.0f,
    .massEmpty = 15.2000f, .massRound = 0.0000f, .capacity = 1, .bounce = 0.8000f, .keep = true, .hull = kFim92Hull, .hullPoints = 22};

// AKS-74u: 17991@0, frame bone 2, core bone 2 (5550 of 6305 vertices), nothing left out;
// kept 6591 vertices; its middle (-0.001 -0.008 0.364), its footprint 2.5 cm wide, its keel at x 0.005; 14 hull points, the rest from 12.
// added its magazine's other piece (894@18063).
// added its rounds (312@18957).
inline constexpr float kAks74uHull[14][3] = {
    {0.0053f, -0.0699f, 0.0069f},  // keel back
    {0.0053f, -0.1794f, 0.4330f},  // keel middle
    {-0.0033f, 0.0199f, 0.0053f},  // back end
    {0.0075f, 0.0212f, 0.7100f},  // front end
    {-0.0039f, 0.0784f, 0.6299f},  // gap
    {-0.0395f, 0.0326f, 0.4710f},  // gap
    {-0.0095f, 0.0138f, 0.7090f},  // gap
    {-0.0136f, -0.1717f, 0.4373f},  // guard
    {0.0168f, 0.0399f, 0.2512f},  // gap
    {-0.0116f, -0.0618f, 0.0080f},  // gap
    {-0.0058f, 0.0765f, 0.3972f},  // gap
    {0.0214f, 0.0102f, 0.6001f},  // gap
    {0.0209f, 0.0340f, 0.6001f},  // gap
    {0.0032f, 0.0580f, 0.2566f},  // gap
};
inline constexpr loose::Spec kAks74u{.name = "aks74ugun", .boxMin = {-0.0221f, -0.1028f, 0.0115f}, .boxMax = {0.0206f, 0.0869f, 0.7162f}, .sweep = 0.0f, .sag = 0.0f,
    .massEmpty = 2.7000f, .massRound = 0.0102f, .capacity = 30, .bounce = 1.0000f, .keep = true, .hull = kAks74uHull, .hullPoints = 14};

// UMP-45: 19314@234, frame bone 2, core bone 2 (6674 of 7301 vertices), nothing left out;
// kept 7377 vertices; its middle (0.000 0.011 0.393), its footprint 3.8 cm wide, its keel at x 0.006; 22 hull points, the rest from 15.
// added its rounds (234@0).
inline constexpr float kUmp45Hull[22][3] = {
    {0.0060f, -0.0498f, 0.0127f},  // keel back
    {0.0060f, -0.2125f, 0.4600f},  // keel middle
    {0.0060f, -0.2054f, 0.4987f},  // keel front
    {-0.0105f, 0.0388f, 0.0111f},  // back end
    {-0.0025f, 0.0265f, 0.6479f},  // front end
    {0.0001f, 0.1024f, 0.6011f},  // gap
    {0.0140f, 0.0940f, 0.2810f},  // gap
    {0.0304f, 0.0646f, 0.5763f},  // gap
    {-0.0187f, -0.2070f, 0.4599f},  // guard
    {0.0107f, 0.0482f, 0.0111f},  // gap
    {-0.0260f, 0.0416f, 0.6143f},  // gap
    {-0.0272f, 0.0738f, 0.2428f},  // gap
    {-0.0101f, -0.0448f, 0.0127f},  // guard
    {0.0266f, 0.0259f, 0.6143f},  // gap
    {0.0188f, -0.2070f, 0.4599f},  // guard
    {-0.0084f, 0.0516f, 0.0130f},  // gap
    {-0.0177f, -0.2002f, 0.4966f},  // guard
    {-0.0137f, 0.0940f, 0.2810f},  // gap
    {0.0177f, -0.2001f, 0.4968f},  // guard
    {-0.0236f, 0.0008f, 0.6108f},  // gap
    {0.0284f, 0.0486f, 0.2542f},  // gap
    {0.0049f, 0.0377f, 0.6479f},  // gap
};
inline constexpr loose::Spec kUmp45{.name = "ump45gun", .boxMin = {-0.0279f, -0.0931f, 0.0750f}, .boxMax = {0.0280f, 0.1147f, 0.7118f}, .sweep = 0.0f, .sag = 0.0f,
    .massEmpty = 2.5000f, .massRound = 0.0210f, .capacity = 25, .bounce = 1.0000f, .keep = true, .hull = kUmp45Hull, .hullPoints = 22};

// M1014: 13377@0, frame bone 3, core bone 3 (4658 of 5076 vertices), nothing left out;
// kept 5076 vertices; its middle (-0.001 0.076 0.398), its footprint 3.4 cm wide, its keel at x 0.005; 22 hull points, the rest from 12.
inline constexpr float kM1014Hull[22][3] = {
    {0.0053f, -0.0547f, -0.0316f},  // keel back
    {0.0053f, -0.0562f, 0.2993f},  // keel middle
    {-0.0008f, 0.0698f, -0.0417f},  // back end
    {-0.0038f, 0.1168f, 1.0186f},  // front end
    {0.0102f, 0.1460f, 0.3486f},  // gap
    {-0.0319f, 0.1094f, 0.4920f},  // gap
    {0.0043f, 0.1451f, 0.9731f},  // gap
    {0.0053f, 0.0991f, 1.0186f},  // gap
    {-0.0178f, -0.0492f, 0.2891f},  // guard
    {-0.0234f, 0.0334f, 0.0366f},  // gap
    {-0.0171f, 0.0073f, -0.0327f},  // gap
    {0.0181f, 0.0695f, 0.0149f},  // gap
    {0.0156f, 0.0073f, -0.0327f},  // gap
    {0.0196f, 0.0469f, 0.7710f},  // gap
    {-0.0099f, -0.0415f, -0.0281f},  // guard
    {-0.0124f, 0.1456f, 0.3486f},  // gap
    {0.0163f, -0.0492f, 0.2891f},  // guard
    {-0.0191f, 0.0695f, 0.0149f},  // gap
    {-0.0184f, 0.0442f, 0.7710f},  // gap
    {-0.0117f, 0.0561f, -0.0365f},  // gap
    {0.0135f, 0.0420f, -0.0354f},  // gap
    {-0.0054f, 0.1409f, 0.9858f},  // gap
};
inline constexpr loose::Spec kM1014{.name = "m1014gun", .boxMin = {-0.0245f, -0.0256f, -0.1317f}, .boxMax = {0.0230f, 0.1767f, 0.9286f}, .sweep = 0.0f, .sag = 0.0f,
    .massEmpty = 3.8000f, .massRound = 0.0450f, .capacity = 5, .bounce = 1.0000f, .keep = true, .hull = kM1014Hull, .hullPoints = 22};

// Mk11 Mod 0: 12456@2214, frame bone 0, core bone 0 (4642 of 5098 vertices), nothing left out;
// kept 5599 vertices; its middle (-0.001 0.001 0.451), its footprint 3.0 cm wide, its keel at x 0.005; 22 hull points, the rest from 20.
// added its stock's plate and its magazine's sleeve (1902@0).
// added its rounds (312@1902).
inline constexpr float kMk11Hull[22][3] = {
    {0.0053f, -0.0929f, -0.0040f},  // keel back
    {0.0053f, -0.1370f, 0.3091f},  // keel middle
    {0.0053f, -0.1123f, 0.4917f},  // keel front
    {-0.0008f, 0.0468f, -0.0174f},  // back end
    {-0.0009f, 0.0326f, 1.0039f},  // front end
    {0.0275f, 0.0349f, 0.7031f},  // gap
    {-0.0264f, 0.0477f, 0.3870f},  // gap
    {0.0053f, -0.1049f, 0.4596f},  // gap
    {-0.0199f, -0.0999f, 0.4596f},  // guard
    {0.0107f, 0.0563f, 0.8511f},  // gap
    {-0.0187f, -0.0621f, 0.0026f},  // gap
    {0.0053f, 0.0156f, 1.0039f},  // gap
    {0.0194f, -0.0169f, -0.0026f},  // gap
    {-0.0124f, 0.0563f, 0.8511f},  // gap
    {0.0179f, 0.0565f, 0.2732f},  // gap
    {-0.0182f, 0.0283f, -0.0069f},  // gap
    {0.0053f, -0.1332f, 0.2932f},  // gap
    {-0.0152f, -0.1282f, 0.2932f},  // guard
    {-0.0258f, 0.0341f, 0.8315f},  // gap
    {0.0139f, 0.0404f, -0.0077f},  // gap
    {0.0269f, 0.0184f, 0.8315f},  // gap
    {-0.0194f, 0.0565f, 0.2732f},  // gap
};
inline constexpr loose::Spec kMk11{.name = "mk11gun", .boxMin = {-0.0308f, -0.0966f, -0.0595f}, .boxMax = {0.0294f, 0.0984f, 0.9618f}, .sweep = 0.0f, .sag = 0.0f,
    .massEmpty = 4.9000f, .massRound = 0.0240f, .capacity = 10, .bounce = 1.0000f, .keep = true, .hull = kMk11Hull, .hullPoints = 22};

// SMAW (its case on): 21756@21756, frame bone 0, core bone 2 (3886 of 4163 vertices), nothing left out;
// kept 4163 vertices; its middle (-0.009 0.071 0.036), its footprint 3.9 cm wide, its keel at x 0.000; 22 hull points, the rest from 21.
// its case: bone 1's 257 vertices, at home (their middle 5.4 mm from it).
inline constexpr float kSmawHull[22][3] = {
    {0.0000f, 0.0283f, -0.7259f},  // keel back
    {0.0000f, -0.1771f, 0.0964f},  // keel middle
    {0.0000f, -0.1333f, 0.3411f},  // keel front
    {0.0000f, 0.0540f, 0.6399f},  // front end
    {0.0376f, 0.1791f, 0.0603f},  // gap
    {-0.0235f, 0.1551f, -0.7259f},  // gap
    {0.0332f, 0.1705f, 0.6223f},  // gap
    {-0.1031f, 0.0966f, 0.6096f},  // gap
    {0.0640f, 0.1158f, -0.7259f},  // gap
    {-0.0628f, 0.0677f, -0.7259f},  // gap
    {0.0832f, 0.0872f, 0.0798f},  // gap
    {0.0453f, 0.0830f, 0.6399f},  // gap
    {0.0624f, 0.0640f, -0.7259f},  // gap
    {-0.1099f, 0.0981f, -0.0786f},  // gap
    {0.0000f, -0.1771f, 0.1360f},  // gap
    {0.0202f, -0.1721f, 0.1360f},  // guard
    {0.0284f, 0.1536f, -0.7259f},  // gap
    {-0.0612f, 0.1196f, -0.7259f},  // gap
    {-0.0191f, -0.1721f, 0.0999f},  // guard
    {0.0789f, 0.1413f, 0.1082f},  // gap
    {-0.0316f, 0.1241f, 0.6399f},  // gap
    {-0.0753f, 0.1395f, 0.1858f},  // gap
};
inline constexpr loose::Spec kSmaw{.name = "smawgun", .boxMin = {-0.1055f, -0.1071f, -0.3792f}, .boxMax = {0.0876f, 0.2492f, 0.4518f}, .sweep = 0.0f, .sag = 0.0f,
    .massEmpty = 13.6000f, .massRound = 0.0000f, .capacity = 1, .bounce = 0.8000f, .keep = true, .hull = kSmawHull, .hullPoints = 22};

// SMAW (bare): 21756@21756, frame bone 0, core bone 2 (3886 of 4163 vertices), nothing left out;
// kept 3906 vertices; its middle (-0.013 0.063 0.202), its footprint 3.9 cm wide, its keel at x 0.000; 22 hull points, the rest from 20.
// its case (bone 1, 257 vertices) left out.
inline constexpr float kSmawBareHull[22][3] = {
    {0.0000f, -0.1771f, 0.0955f},  // keel back
    {0.0000f, -0.1333f, 0.3411f},  // keel front
    {-0.0076f, 0.0375f, -0.1912f},  // back end
    {0.0000f, 0.0540f, 0.6399f},  // front end
    {0.0372f, 0.1792f, 0.0603f},  // gap
    {0.0332f, 0.1705f, 0.6223f},  // gap
    {-0.1099f, 0.0981f, -0.0786f},  // gap
    {-0.1031f, 0.0966f, 0.6096f},  // gap
    {0.0831f, 0.0845f, 0.0798f},  // gap
    {0.0376f, 0.1289f, -0.1389f},  // gap
    {0.0453f, 0.0830f, 0.6399f},  // gap
    {-0.0195f, 0.1402f, -0.1389f},  // gap
    {0.0490f, 0.0718f, -0.1389f},  // gap
    {0.0000f, -0.1771f, 0.1360f},  // gap
    {0.0202f, -0.1721f, 0.1360f},  // guard
    {-0.0753f, 0.1395f, 0.1858f},  // gap
    {0.0787f, 0.1413f, 0.0994f},  // gap
    {-0.0191f, -0.1721f, 0.0999f},  // guard
    {-0.0554f, 0.1419f, -0.0964f},  // gap
    {-0.0316f, 0.1241f, 0.6399f},  // gap
    {0.0108f, 0.1432f, -0.1389f},  // gap
    {0.0202f, -0.1721f, 0.0999f},  // guard
};
inline constexpr loose::Spec kSmawBare{.name = "smawgun", .boxMin = {-0.1091f, -0.1149f, -0.2135f}, .boxMax = {0.0840f, 0.2414f, 0.6175f}, .sweep = 0.0f, .sag = 0.0f,
    .massEmpty = 7.7000f, .massRound = 0.0000f, .capacity = 1, .bounce = 0.8000f, .keep = true, .hull = kSmawBareHull, .hullPoints = 22};

inline constexpr Body kBodies[] = {
    {"an94", &kAn94, {-0.0346f, -0.1618f, -0.0017f}, {0.0181f, 0.1046f, 0.9482f}, kMetal, 12},
    {"rpk", &kRpk, {-0.0525f, -0.2170f, -0.0325f}, {0.0258f, 0.0898f, 1.1240f}, kMetal, 14},
    {"aek971", &kAek971, {-0.0331f, -0.1788f, -0.0643f}, {0.0244f, 0.0983f, 0.9897f}, kMetal, 15},
    {"ak74", &kAk74, {-0.0404f, -0.1967f, -0.0052f}, {0.0231f, 0.0720f, 0.9355f}, kMetal, 14},
    {"g3a3", &kG3a3, {-0.0296f, -0.1225f, -0.0294f}, {0.0313f, 0.1143f, 1.0303f}, kMetal, 15},
    {"kh2002", &kKh2002, {-0.0271f, -0.1112f, -0.0233f}, {0.0271f, 0.0887f, 0.7109f}, kMetal, 16},
    {"m16a4", &kM16a4, {-0.0369f, -0.2134f, -0.0182f}, {0.0309f, 0.0591f, 1.0459f}, kMetal, 15},
    {"m4a1", &kM4a1, {-0.0369f, -0.2048f, 0.0239f}, {0.0310f, 0.0573f, 0.9087f}, kMetal, 17},
    {"m240", &kM240, {-0.1186f, -0.1706f, -0.0275f}, {0.0977f, 0.0968f, 1.1973f}, kHeavy, 19},
    {"fim92", &kFim92, {-0.1767f, -0.1763f, -0.3455f}, {0.1465f, 0.1996f, 1.1738f}, kHeavy, 22},
    {"aks74u", &kAks74u, {-0.0395f, -0.1794f, 0.0053f}, {0.0214f, 0.0784f, 0.7100f}, kMetal, 12},
    {"ump45", &kUmp45, {-0.0275f, -0.2125f, 0.0111f}, {0.0306f, 0.1024f, 0.6479f}, kMetal, 15},
    {"m1014", &kM1014, {-0.0319f, -0.0562f, -0.0417f}, {0.0232f, 0.1460f, 1.0186f}, kMetal, 12},
    {"mk11", &kMk11, {-0.0310f, -0.1370f, -0.0174f}, {0.0292f, 0.0581f, 1.0039f}, kMetal, 20},
    {"smaw", &kSmaw, {-0.1099f, -0.1771f, -0.7259f}, {0.0832f, 0.1792f, 0.6399f}, kHeavy, 21},
    {"smaw_bare", &kSmawBare, {-0.1099f, -0.1771f, -0.1912f}, {0.0832f, 0.1792f, 0.6399f}, kHeavy, 20},
};

// The sights drawn apart (skin.cpp's kHeldSights), each with the gun it came on: the furthest of its vertices toward its box's eight corners.
inline constexpr Attach kAttach[] = {
    {10599, 72, 0, 8,  // the PSO-1
     {{-0.0158f, 0.0173f, -0.1088f}, {0.0158f, 0.0173f, -0.1079f}, {-0.0186f, 0.0455f, -0.1085f}, {0.0159f, 0.0489f, -0.1093f},
      {-0.0125f, -0.0026f, 0.0770f}, {0.0127f, -0.0046f, 0.0770f}, {-0.0114f, 0.0421f, 0.1190f}, {0.0114f, 0.0421f, 0.1190f}}},
    {9084, 57, 0, 8,  // the PKS-07
     {{-0.0192f, 0.0255f, -0.1310f}, {0.0192f, 0.0255f, -0.1310f}, {-0.0192f, 0.0451f, -0.1312f}, {0.0192f, 0.0451f, -0.1312f},
      {-0.0117f, -0.0034f, 0.0779f}, {0.0117f, -0.0034f, 0.0779f}, {-0.0153f, 0.0652f, 0.1248f}, {0.0153f, 0.0652f, 0.1248f}}},
    {3723, 48, 0, 8,  // the PK-A
     {{-0.0121f, -0.0026f, -0.0377f}, {0.0116f, -0.0039f, -0.0377f}, {-0.0152f, 0.0397f, -0.0507f}, {0.0144f, 0.0397f, -0.0507f},
      {-0.0121f, -0.0026f, 0.0410f}, {0.0116f, -0.0039f, 0.0410f}, {-0.0127f, 0.0379f, 0.0805f}, {0.0119f, 0.0379f, 0.0805f}}},
    {7884, 60, 0, 8,  // the PK-AS
     {{-0.0109f, 0.0032f, -0.0420f}, {0.0177f, 0.0051f, -0.0331f}, {-0.0126f, 0.0526f, -0.0623f}, {0.0124f, 0.0526f, -0.0623f},
      {-0.0111f, -0.0025f, 0.0522f}, {0.0112f, -0.0025f, 0.0522f}, {-0.0228f, 0.0698f, 0.0515f}, {0.0232f, 0.0703f, 0.0510f}}},
    {6492, 210, 0, 8,  // the Kobra
     {{-0.0111f, -0.0030f, -0.0514f}, {0.0157f, -0.0015f, -0.0481f}, {-0.0175f, 0.0257f, -0.0525f}, {0.0139f, 0.0251f, -0.0593f},
      {-0.0141f, 0.0075f, 0.0695f}, {0.0139f, 0.0075f, 0.0695f}, {-0.0117f, 0.0479f, 0.0688f}, {0.0116f, 0.0479f, 0.0688f}}},
    {9384, 0, 0, 8,  // the EoTech
     {{-0.0197f, -0.0027f, -0.0557f}, {0.0197f, -0.0027f, -0.0557f}, {-0.0166f, 0.0449f, -0.0461f}, {0.0166f, 0.0449f, -0.0461f},
      {-0.0169f, -0.0010f, 0.0476f}, {0.0169f, -0.0010f, 0.0476f}, {-0.0158f, 0.0177f, 0.0535f}, {0.0158f, 0.0177f, 0.0535f}}},
    {12642, 156, 0, 8,  // the rifle scope
     {{-0.0168f, 0.0210f, -0.1368f}, {0.0194f, 0.0234f, -0.1368f}, {-0.0267f, 0.0583f, -0.1171f}, {0.0178f, 0.0522f, -0.1368f},
      {-0.0214f, 0.0182f, 0.1936f}, {0.0206f, 0.0182f, 0.1937f}, {-0.0214f, 0.0555f, 0.1936f}, {0.0206f, 0.0555f, 0.1937f}}},
};

// The M416 without its M320 (a later level's): gunbody::kM416 less the M320's five hull points (its tube's four, its grip's bottom), 3.9 kg (the
// M416 3.5, the ACOG 0.4); the same boxes. Named "m416gun" too, so its lines are the M416's.
inline constexpr float kM416BareHull[17][3] = {
    {gunbody::kM416Hull[0][0], gunbody::kM416Hull[0][1], gunbody::kM416Hull[0][2]},
    {gunbody::kM416Hull[1][0], gunbody::kM416Hull[1][1], gunbody::kM416Hull[1][2]},
    {gunbody::kM416Hull[2][0], gunbody::kM416Hull[2][1], gunbody::kM416Hull[2][2]},
    {gunbody::kM416Hull[3][0], gunbody::kM416Hull[3][1], gunbody::kM416Hull[3][2]},
    {gunbody::kM416Hull[4][0], gunbody::kM416Hull[4][1], gunbody::kM416Hull[4][2]},
    {gunbody::kM416Hull[5][0], gunbody::kM416Hull[5][1], gunbody::kM416Hull[5][2]},
    {gunbody::kM416Hull[6][0], gunbody::kM416Hull[6][1], gunbody::kM416Hull[6][2]},
    {gunbody::kM416Hull[7][0], gunbody::kM416Hull[7][1], gunbody::kM416Hull[7][2]},
    {gunbody::kM416Hull[8][0], gunbody::kM416Hull[8][1], gunbody::kM416Hull[8][2]},
    {gunbody::kM416Hull[9][0], gunbody::kM416Hull[9][1], gunbody::kM416Hull[9][2]},
    {gunbody::kM416Hull[10][0], gunbody::kM416Hull[10][1], gunbody::kM416Hull[10][2]},
    {gunbody::kM416Hull[11][0], gunbody::kM416Hull[11][1], gunbody::kM416Hull[11][2]},
    {gunbody::kM416Hull[12][0], gunbody::kM416Hull[12][1], gunbody::kM416Hull[12][2]},
    {gunbody::kM416Hull[13][0], gunbody::kM416Hull[13][1], gunbody::kM416Hull[13][2]},
    {gunbody::kM416Hull[18][0], gunbody::kM416Hull[18][1], gunbody::kM416Hull[18][2]},
    {gunbody::kM416Hull[19][0], gunbody::kM416Hull[19][1], gunbody::kM416Hull[19][2]},
    {gunbody::kM416Hull[20][0], gunbody::kM416Hull[20][1], gunbody::kM416Hull[20][2]},
};
inline constexpr loose::Spec kM416Bare{.name = gunbody::kM416.name,
                                       .boxMin = {gunbody::kM416.boxMin[0], gunbody::kM416.boxMin[1], gunbody::kM416.boxMin[2]},
                                       .boxMax = {gunbody::kM416.boxMax[0], gunbody::kM416.boxMax[1], gunbody::kM416.boxMax[2]},
                                       .sweep = gunbody::kM416.sweep, .sag = gunbody::kM416.sag, .massEmpty = 3.9000f,
                                       .massRound = gunbody::kM416.massRound, .capacity = gunbody::kM416.capacity, .bounce = gunbody::kM416.bounce,
                                       .keep = true, .hull = kM416BareHull, .hullPoints = 17};

inline const Body* For(const char* shape) {
    if (!shape)
        return nullptr;
    for (const Body& b : kBodies)
        if (b.spec && std::strcmp(b.shape, shape) == 0)
            return &b;
    return nullptr;
}
inline const Attach* AttachFor(uint32_t count, uint32_t start, int base) {
    for (const Attach& a : kAttach)
        if (a.points > 0 && a.count == count && a.start == start && a.base == base)
            return &a;
    return nullptr;
}
}

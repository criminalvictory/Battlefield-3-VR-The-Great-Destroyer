#pragma once

#include "handpose.h"

namespace rocketposes {

// The left hand on the launcher's case, as a hand holds a cup (the user, 2026-11-04: "His thumb is going to wrap around the other perimeter of the circle of
// the cylinder"): the palm against it, the four fingers closed on it one way round, the thumb the other way. Fitted on the meshes offline by
// tools\rocketfit.cpp, every part of every finger closed from the palm out until it touches. Where along the case the hand is and how far it's turned on it are
// the game's own (in its reload it's the RIGHT hand that changes the rocket, the left staying on the front grip: tools\rocketgrip.cpp). Its fingers aren't: made
// for its one camera, they stand off the case, and from behind it the thumb lies on the same side as the fingers.
// The case's body is 8.9 cm across, z -0.44 to 0.06 on its bone (radius 43.7 to 44.7 mm); only its flange at the back, z -0.48 to -0.447, is 13.6.
// kCaseInHand: case -> the gloves' bind space. kFingers: each of the left glove's bones in the bind space, as handpose::Pose gives them (palm width 1.15).
// Fitted by tools\rocketfit.cpp: "pick 20 6 0.020". The thumb 144 degrees round from the palm, the fingers 89 103 107 84 the other way.
constexpr float kCaseInHand[12] = {0.62688f, -0.68238f, 0.37600f, 0.46966f, -0.64138f, -0.72597f, -0.24819f, 0.95029f, 0.44232f, -0.08557f, -0.89276f, 0.13347f};
constexpr float kFingers[handpose::kBones][12] = {
    {1.03005f, -0.01804f, -0.03205f, 0.01030f, -0.01804f, 1.01083f, 0.01924f, -0.00619f, -0.03205f, 0.01924f, 1.03417f, -0.01099f},
    {1.06010f, -0.03608f, -0.06409f, 0.02061f, -0.03608f, 1.02166f, 0.03847f, -0.01237f, -0.06409f, 0.03847f, 1.06834f, -0.02198f},
    {0.51113f, -0.75072f, -0.41845f, 1.13146f, 0.13995f, 0.55306f, -0.82127f, 0.61535f, 0.84800f, 0.36122f, 0.38776f, -0.67957f},
    {0.37972f, 0.54365f, -0.69451f, -0.06435f, -0.87479f, 0.12589f, -0.44172f, 1.50718f, -0.05520f, 0.81134f, 0.46344f, -0.71140f},
    {1.03005f, -0.01804f, -0.03205f, 0.01030f, -0.01804f, 1.01083f, 0.01924f, -0.00619f, -0.03205f, 0.01924f, 1.03417f, -0.01099f},
    {1.03005f, -0.01804f, -0.03205f, 0.01030f, -0.01804f, 1.01083f, 0.01924f, -0.00619f, -0.03205f, 0.01924f, 1.03417f, -0.01099f},
    {1.06010f, -0.03608f, -0.06409f, 0.02061f, -0.03608f, 1.02166f, 0.03847f, -0.01237f, -0.06409f, 0.03847f, 1.06834f, -0.02198f},
    {0.84226f, 0.39440f, -0.22654f, -0.28787f, -0.41973f, 0.82940f, -0.32903f, 0.49810f, 0.17680f, 0.35955f, 0.85162f, -0.45377f},
    {1.06010f, -0.03608f, -0.06409f, 0.02061f, -0.03608f, 1.02166f, 0.03847f, -0.01237f, -0.06409f, 0.03847f, 1.06834f, -0.02198f},
    {0.89623f, 0.30092f, -0.14789f, -0.23952f, -0.28883f, 0.91904f, -0.20941f, 0.29877f, 0.18928f, 0.20318f, 0.89984f, -0.29707f},
    {0.36050f, 0.49517f, -0.73400f, 0.01632f, -0.88371f, 0.12729f, -0.41660f, 1.51417f, -0.01580f, 0.84101f, 0.42690f, -0.74341f},
    {0.80973f, 0.42268f, -0.28213f, -0.28498f, -0.47572f, 0.76878f, -0.39162f, 0.59939f, 0.17082f, 0.44612f, 0.81338f, -0.53795f},
    {1.06010f, -0.03608f, -0.06409f, 0.02061f, -0.03608f, 1.02166f, 0.03847f, -0.01237f, -0.06409f, 0.03847f, 1.06834f, -0.02198f},
    {0.82243f, 0.43489f, -0.23785f, -0.31584f, -0.47110f, 0.76254f, -0.41571f, 0.59964f, 0.11899f, 0.44618f, 0.81416f, -0.51323f},
    {1.06010f, -0.03608f, -0.06409f, 0.02061f, -0.03608f, 1.02166f, 0.03847f, -0.01237f, -0.06409f, 0.03847f, 1.06834f, -0.02198f},
    {0.40139f, 0.57020f, -0.65530f, -0.10179f, -0.87101f, 0.25947f, -0.38202f, 1.37549f, 0.05212f, 0.76421f, 0.54781f, -0.71456f},
    {0.73507f, 0.50614f, -0.35437f, -0.31458f, -0.60068f, 0.63280f, -0.46364f, 0.82199f, 0.10649f, 0.55951f, 0.74276f, -0.60964f},
    {0.58557f, -0.51547f, -0.62557f, 0.89214f, 0.12947f, 0.82130f, -0.55555f, 0.25566f, 0.80018f, 0.24433f, 0.54769f, -0.57052f},
    {0.64774f, -0.22784f, -0.72695f, 0.56309f, 0.08887f, 0.97029f, -0.22491f, 0.03459f, 0.75662f, 0.08108f, 0.64877f, -0.38952f},
    {0.72927f, 0.49983f, -0.36605f, -0.29889f, -0.59323f, 0.70259f, -0.35547f, 0.74021f, 0.19679f, 0.48271f, 0.78428f, -0.56824f},
    {0.62563f, 0.51870f, -0.50345f, -0.22508f, -0.70937f, 0.50413f, -0.46189f, 1.03094f, 0.12861f, 0.66746f, 0.65402f, -0.70755f},
    {0.61373f, 0.54405f, -0.49358f, -0.24450f, -0.72328f, 0.51198f, -0.43252f, 1.03588f, 0.13052f, 0.64390f, 0.67390f, -0.68131f},
    {0.41832f, 0.54990f, -0.66249f, -0.08888f, -0.86075f, 0.23998f, -0.41696f, 1.39647f, 0.03086f, 0.78278f, 0.52163f, -0.72553f},
};

}

#pragma once
// The places free hands' film rounds 4-9 put a scripted hand (freetest.h's Where kinds from kHandle on, and
// any step moved from one place to another), for either side: the right's are the left's tables mirrored.
// Pure: hands.cpp's RunFreeTest gives it this frame's state in a Scene, and tools/grasptest.cpp runs the
// same rounds on the same places without the game.

#include "freetest.h"

namespace freeplaces {

struct Scene {   // hands.cpp's state this frame, everything the places go by (rows: rotation | translation)
    float headPose[12] = {};          // the view's space into the room's
    bool haveGun = false;
    float gunRoom[12] = {};           // the rifle as drawn last frame: its model space into the room's (g_lastGunRoom)
    float standInHead[12] = {};       // the stand-in's right grip in the head's space (the left's turn: its handpose::Mirror)
    float lineUp[12] = {};            // the left glove's line-up, the gloves' bind space into the left grip's frame (g_leftOffset; the right's: its Mirror)
    float leftAnchor[3] = {};         // the left glove's palm point in the bind space (g_leftAnchor; the right's: x negated)
    bool haveBody = false;
    float neck[3] = {};               // pouch::BodyFrame: the neck point (room) and the body's heading (radians)
    float yaw = 0.0f;
    bool haveM320Grip = false;
    float m320Grip[12] = {};          // skin::GameLeftGripM320's bone 0: the game's left glove on the launcher's grip (the bind space into the rifle's model space)
    bool haveRound[2] = {};
    float roundInGlove[2][12] = {};   // each side's round in its glove as the launcher module last had it (g_lastRoundInGloveBy)
};

bool Handles(freetest::Where where);  // kHandle and every kind after it (kFront and before: RunFreeTest's own, unless a step has a from)

// The controller's grip pose (the room) for side's hand at step's place (where at along, its shift, from at fromAlong by blend). False: not now (no
// rifle drawn, no body frame, the launcher's grip not learned, no round in that glove, a number not finite): the hand stays where it was.
bool Grip(const Scene& scene, const freetest::Step& step, int side, float grip[12]);

}  // namespace freeplaces

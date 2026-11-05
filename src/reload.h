#pragma once

namespace reload {

// The rifle's bolt and its two manual releases (the user's wish, 2026-10-30). Run dry, the bolt
// locks back and the gun won't fire. With a magazine in, it goes forward and chambers a round when
// the charging handle (the lever at the back of the receiver's top) is pulled back with the left
// hand, left trigger held, and let go, or when the bolt release on the left side of the receiver, just
// behind the magazine well, is slapped (the flat-screen game's empty reload).
//
// Once a frame on the render thread, with free hands (hands.cpp): the gun's pose (rows: rotation |
// translation, its model space to the view's) and the left palm in the view's space (nullptr when
// not tracked), how far the left trigger is pulled (0-1), and where the left glove touches the
// gun's surface, in its model space (nullptr when it doesn't: gunshape.cpp).
void Update(const float* gunPose, const float* leftPalm, float grip, float trigger, const float* touch, bool hasShape);

// The palm's near enough the charging handle that a press of this kind takes it (the pouch leaves it alone then: over the shoulder with the gun at the eye is right by it).
bool NearHandle(const float* gunPose, const float* leftPalm, bool byGrip);

// The bolt is back: the gun mustn't fire.
bool BoltBack();

// The left hand has hold of the charging handle (the glove is drawn on it, hooking it).
bool HandleHeld();

// The game's own reload (X held), or a new life: a round in the chamber.
void ReleaseBolt();

// How far back the charging handle is drawn, in metres along the barrel.
float HandleBack();

// How far the ejection port's cover has swung open on its hinge, in radians (0 = shut).
float CoverAngle();

// Any gun's port cover (the M416's in Update; the M16's and the M4's in hands.cpp, the user, 2026-11-05: "everything that M416 has"). A frame's step, with how long the frame was, whether its bolt is locked back
// and how far back its handle is: how far open it is, 0-1.
struct Cover {
    float open = 0.0f;       // 0 shut, 1 open
    bool byHandle = false;   // opened by the handle (follows it home) rather than by a shot
    double boltMovedAt = -1.0;
    unsigned long long shotSeen = 0;  // the last shot it opened for
};
float StepCover(Cover& cover, float dt, bool boltBack, float handle, bool quick = false);

// A slap on a bolt's release, readily but only with the hand really on it (the user, 2026-11-05: "reactive as possible", then "it activates when I'm hovering it... actually pressing the bolt reset"): the palm on the gun now and a frame ago, the frame's length;
// where the glove touches the gun (nullptr: it doesn't); whether it was on the release a frame ago, updated here; the release; whether this gun's shape is known, so a touch can be felt at all. The glove arriving on the release from anywhere else, on or off the gun, hardly moving at all. Without a shape, a quick hand right by it.
bool Slapped(const float* palm, const float* lastPalm, float dt, const float* touch, bool& wasOnRelease, const float* release, bool hasShape);

// weapon.txt "pull 30": the charging handle held drawn this far (metres; below 0 no test), to see
// the cover open with it without anyone in the headset.
void TestPull(float metres);

}

#pragma once

namespace reload {

// The rifle's bolt and its two manual releases (the user's wish, 2026-10-30), by either hand. Run
// dry, the bolt locks back and the gun won't fire. With a magazine in, it goes forward and chambers
// a round when the charging handle (the lever at the back of the receiver's top) is pulled back with
// either hand, its trigger or grip held, and let go, or when the bolt release on the left side of the
// receiver, just behind the magazine well, is slapped by either hand (the flat-screen game's empty
// reload).
//
// Once a frame on the render thread, with free hands (hands.cpp): the gun's pose (rows: rotation |
// translation, its model space to the view's) and the left palm in the view's space (nullptr when
// not tracked), how far the left trigger is pulled (0-1), and where the left glove touches the
// gun's surface, in its model space (nullptr when it doesn't: gunshape.cpp).
void Update(const float* gunPose, const float* leftPalm, float grip, float trigger, const float* touch, bool hasShape);

inline constexpr float kBoltReleaseOnGun[3] = {0.03f, 0.01f, 0.37f};  // the bolt release on the receiver's left side (the rifle's model space): reload.cpp's kBoltRelease
// The palm point with the glove's V on the charging handle's T, by side ([0] the left: reload.cpp's kHandleGrip; [1] the right: x negated, the T on x 0).
inline constexpr float kHandleGripOnGun[2][3] = {{0.035f, 0.14f, 0.135f}, {-0.035f, 0.14f, 0.135f}};
// The left glove's V on the handle (hands.cpp's kOnLever, moved here unchanged: the gloves' bind space into the rifle's model space); the right's is its
// handpose::Mirror.
inline constexpr float kOnLever[12] = {0.445276f, -0.368412f, -0.816090f, 0.358794f,  0.623634f, 0.781615f,
                                       -0.012581f, -1.056041f, 0.642503f,  -0.503339f, 0.577788f, 0.317216f};
struct Hand {                           // one hand this frame, as hands.cpp has it ([0] the left, [1] the right)
    const float* palm = nullptr;        // its palm point in the view's space; nullptr: not tracked, busy, or not the hand at work (takes nothing, lets go of the handle)
    float grip = 0.0f, trigger = 0.0f;  // its own controller's, 0-1
    const float* touch = nullptr;       // where its glove touches the gun's surface (the gun's model space); nullptr: it doesn't
};
// The charging handle and the bolt release by either hand, once a frame. One hand holds the handle at a time (side 0 checked first); each hand's presses,
// palm and slap are its own. Update(gunPose, leftPalm, grip, trigger, touch, hasShape) is this with {leftPalm, grip, trigger, touch} and {}.
void UpdateHands(const float* gunPose, const Hand hands[2], bool hasShape);

// The palm's near enough the charging handle that a press of this kind takes it (the pouch leaves it alone then: over the shoulder with the gun at the eye is right by it).
bool NearHandle(const float* gunPose, const float* palm, bool byGrip, int side = 0);  // that side's grip point (kHandleGripOnGun[side])

// The bolt is back: the gun mustn't fire.
bool BoltBack();

// A hand has hold of the charging handle (its glove is drawn on it, hooking it): HandleSide() >= 0.
bool HandleHeld();
int HandleSide();                 // 0 the left, 1 the right, -1 none; HandleHeld() is HandleSide() >= 0

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

void TestLockBack();              // free hands' film rounds: with the bolt home, locked back as if run dry with the magazine in (logged once)
void TestClock(double seconds);   // tools/reloadtest.cpp: Now() reads this from now on; below 0 the real clock again

}

#pragma once

// The Stinger reloaded by the left hand (the user, 2026-11-05: "implement the manual reloading for that stinger... he turns a flap for the stinger. I want that implemented as well"). What the game's own reload does, measured
// (docs/research/other-guns.md): nothing comes off the tube; a little after the shot the aerial's four panels fold down flat by themselves, the left hand goes round to the tube's right, takes the folded aerial and lifts it, and it
// springs open, and that's the next missile ready. So here: fired, the aerial folds; it won't fire again until the hand (grip or trigger) has taken the folded aerial on the tube's right and lifted it.
// Pure: no game calls and no logs. hands.cpp tells the game.

namespace flap {

// The tables (src/magposes/fim92_flap.h), on the gun's frame: the panels folding, as a hand lifts them with the hand's middle, the glove and its bones (23 x 12 each), and springing open.
struct Aerial {
    int foldKeys, liftKeys, springKeys;
    const float (*foldBones)[4][12];
    const float (*liftHand)[3];
    const float (*liftGlove)[12];
    const float* liftFingers;
    const float (*liftBones)[4][12];
    const float (*springBones)[4][12];
};

struct Frame {
    float dt = 0.0f;
    const Aerial* aerial = nullptr;  // the Stinger in hand
    bool gunTracked = true;
    float gunPose[12] = {};  // its frame bone into the view's space
    bool left = false;
    float glove[12] = {};    // the left glove as its controller holds it
    float squeeze = 0.0f, trigger = 0.0f;
    bool handFree = true;    // not on the front grip
    bool shot = false;       // a missile's just gone
    bool spare = true;       // another to come
    bool held = true;        // free hands: a hand holds the Stinger (false: a lift lets go and stays folded; nothing is taken)
};

struct Result {
    bool panels = false;       // the panels not as the game has them: these, on the frame
    float bones[4][12] = {};
    float glove[12] = {};      // the hand on the aerial ...
    float gloveBlend = 0.0f;   // ... by this much
    const float (*fingers)[12] = nullptr;  // ... these
    bool holding = false;      // the hand's busy with it
    bool spentSqueeze = false;
    bool empty = false;        // it won't fire: no missile ready
    bool folded = false;       // it's just finished folding
    bool taken = false;        // the hand's just taken it
    bool opened = false;       // it's just sprung open: the next missile ready
};

Result Update(const Frame& f);
// The game's own reload or a new life: ready, the aerial open.
void Reset();

}

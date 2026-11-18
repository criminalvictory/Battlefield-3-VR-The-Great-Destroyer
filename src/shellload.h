#pragma once

// A tube shotgun's shells by hand: what the hands do (docs\research\shotgun-shells.md, section 2; the M1014 of Semper Fidelis, measured 2026-11-18 in
// build\survey\M1014_*). The counting is shotgunammo.h's (ammo.cpp runs it on the game's count); this is where the hands are and what they did, pure: no
// game calls and no logs (tools\shellloadtest.cpp runs it on scripted frames). Each frame hands.cpp gives a Frame and turns the Result into the count
// (one ammo::TubeLoadShell for `loaded`, TubePortLoad for `portDrop`, TubeBoltHome for `released` and `tugged`, TubeRack for `racked`), sounds, buzz and
// the drawing.
//   - A shell from the belly, like all the ammo: a fresh squeeze (grip or trigger) of the hand at work inside the belly's egg gives one, if the count
//     says one may be taken (a shell left there, and room for it: the tube, or the open bolt's port). Carried by its base end in the fingertip pinch
//     (shellposes.h). Let go of at the belly it goes back; anywhere else it falls (a loose shell: it costs nothing, it was never counted).
//   - The loading port, under the receiver: the shell's nose brought to its mouth, nose up or forward, goes onto the gun's own path (the game's own
//     reload's: up past the lifter, tilting forward, then along the tube: Gun::keyBase, keyTilt) and from then on moves only as the hand pushes it, the
//     hand riding it; no magnet. Its last centimetre and a half goes by itself (a finish): the latch's click, counted. It goes onto the way up to 3 cm
//     short of the mouth: there it is where the hand has it down the port's line. Pulled back 4.5 cm past the mouth it comes out into the hand, and goes
//     in again only after the hand has taken it 6 cm away. A full tube: it stops at the lifter with a knock and stays in the hand.
//   - The ejection port, on the gun's right side, only with the bolt held open on an empty chamber: the shell brought to it, nose forward, drops onto
//     the carrier and lies there (counted when the bolt takes it).
//   - The bolt release, the button under the port: a hand arriving on it, or poking it, with the bolt held open sends the bolt home (not for 0.3 s
//     after that hand dropped a shell into the port right above it).
//   - The bolt handle, on the bolt, on the right: taken by a fresh squeeze near its knob and drawn back with the hand. From home, drawn most of its
//     way and let go: racked (what is chambered is thrown out, the next fed). Held open already: a tug and let go sends it home. While it is held back
//     the gun is out of battery (no shot).
//   - Either hand: the hand at work is whichever doesn't hold the gun (Frame::gunHand); the right's hold is the left's flipped. The ports and the
//     button are where they are on the gun for both.
//
// Spaces. Matrices are three rows: rotation | translation. The gun's: its frame bone: x to the gun's left, y up, z to the muzzle, metres. The view's:
// the eye at the origin, x right, y up, z back. The room's: y up. A glove's matrix takes the gloves' bind space into the view's. The shell's is its own
// draw's bone (the M1014's 450 from 13701: Gun::baseOnBone and noseOnBone are its base's and nose's middles there).
namespace shellload {

// A gun's places, on its frame.
struct Gun {
    float baseOnBone[3], noseOnBone[3];  // the shell's base's and nose's middles on the shell's own bone
    // The way in through the loading port, as the game's own reload moves the shell: the base's middle at each key, and how far its nose is tilted up
    // from the gun's z there (degrees, about x). The first: its nose at the mouth. The last: in the tube (drawn no more).
    int keys;
    float keyBase[8][3];
    float keyTilt[8];
    float latch;        // m of the way short of its end at which the latch has it: from there it goes home by itself
    float portBase[3];  // a shell lying in the ejection port on the carrier: its base's middle (its nose along +z)
    float portAt[3];    // where a shell in the hand is brought to for it: between the carrier and the port's opening
    float portReach;    // ... within this of it (the shell's middle), m
    float release[3];   // the bolt release button
    float releaseReach;
    float handle[3];    // the bolt handle's knob with the bolt home
    float handleReach;
    float travel;       // the bolt's way back, m
};
extern const Gun kM1014;

// One hand as hands.cpp has it this frame.
struct HandIn {
    bool tracked = false;                  // its controller this frame (or a test's hand); a number not finite is taken as not
    float glove[12] = {};                  // the glove as its controller holds it: the gloves' bind space into the view's
    float gripRoom[3] = {};                // its controller's grip point in the room: where the belly is measured from, as pouch.cpp does
    float squeeze = 0.0f, trigger = 0.0f;  // its grip and trigger, 0-1: either takes hold
    bool busy = false;                     // holding something else (a front grip, another module's thing): it takes nothing here
    int touches = 0;                       // the glove's points that press the release, the view's space (none: the hand's middle)
    float touch[4][3] = {};
    bool forceBelly = false;               // tests: the belly counted as reached wherever the hand is
};

enum class Take { kYes = 0, kNoShells = 1, kFull = 2 };  // shotgunammo::Take's

struct Frame {
    float dt = 0.0f;
    bool active = false;         // the gun in hand, its shells by hand on. Out of use for under 0.2 s everything is kept; longer, what the hand has falls
    const Gun* gun = nullptr;
    bool gunTracked = true;      // gunPose is this frame's
    float gunPose[12] = {};      // the gun's frame into the view's
    float headPose[12] = {};     // the view's space into the room's
    HandIn hand[2];              // [0] the left, [1] the right
    int gunHand = 1;             // the hand holding the gun (its pistol grip, or its front alone): never the one at work
    float neck[3] = {};          // the body as pouch::BodyFrame places it, in the room
    float yaw = 0.0f;
    bool haveBody = false;
    // The gun as the count has it (ammo.cpp, shotgunammo.h).
    Take take = Take::kYes;      // may a hand take a shell from the belly now
    bool canLoad = false;        // a shell pushed up the tube now would go in (room, and a shell behind it)
    bool tubeFull = false;       // ... the tube is full
    bool portOpen = false;       // a shell laid into the ejection port now would lie there (the bolt held open, the chamber empty, none there)
    bool locked = false;         // the bolt held open
    bool gameReload = false;     // the game's own reload is asked for (X held): nothing goes onto the loading path meanwhile
};

struct Result {
    bool active = false;              // the module has the gun's bolt and shells this frame (false: as the game draws them)
    // ---- Drawn ----
    float boltBack = 0.0f;            // the bolt group drawn this far back from home (m): held open, by the handle, or on its way home
    bool lineShell = false;           // a shell on the loading path: its bone on the gun's frame
    float shellOnGun[12] = {};
    bool handShell = false;           // a shell in the hand at work: its bone in the glove (drawn at the glove as finally drawn times this) and in the view
    float shellInGlove[12] = {};
    float shellView[12] = {};
    // ---- The hand at work ----
    int side = 0;
    float gloveOnGun[12] = {};        // its glove on the gun (riding the shell up the port, or on the handle) ...
    float gloveBlend = 0.0f;          // ... blended in by this much from where the hand is
    const float (*fingers)[12] = nullptr;  // the pinch (the left glove's layout, bones 0-22, for either side), in by fingersShare
    float fingersShare = 0.0f;
    bool fist = false;                // on the bolt handle: a fist
    bool holding = false;             // the hand at work is busy here
    bool spentSqueeze = false;        // this frame's squeeze was used here
    bool handleHeld = false;          // the handle is held back: out of battery, no shot
    // ---- Events ----
    bool took = false;                // a shell from the belly
    bool putBack = false;             // let go of at the belly: back
    bool letGo = false;               // let go of anywhere else, or lost: it falls (drop*)
    float dropView[12] = {};          // its bone in the view's space as it goes
    float dropVelocityRoom[3] = {};   // its speed then (the room, m/s, at most 3)
    bool dropFromHand = false;        // from the hand (else from the loading port, where it was)
    bool entered = false;             // its nose into the loading port: on the path, pushed by the hand
    bool pulledOut = false;           // pulled back out before the latch (or the tube filled meanwhile)
    bool caught = false;              // past the latch: home by itself
    bool loaded = false;              // home in the tube: ammo::TubeLoadShell
    bool fullKnock = false;           // at the mouth with the tube full: it stops at the lifter
    bool portDrop = false;            // laid into the ejection port: ammo::TubePortLoad
    bool released = false;            // the release pressed with the bolt held open: ammo::TubeBoltHome
    bool handleTaken = false;
    bool handleLet = false;           // the handle let go of, drawn handlePulled m at its most
    float handlePulled = 0.0f;
    bool racked = false;              // ... from home, far enough: ammo::TubeRack
    bool tugged = false;              // ... held open, tugged: ammo::TubeBoltHome
    // A fresh squeeze at the belly that took nothing (for the log): where the grip point was from the neck (right, up, forward) and why.
    bool squeezed = false;
    float fromNeck[3] = {};
    const char* note = "";
    const char* state = "";
};

Result Update(const Frame& f);
// Nothing in the hands, the bolt as the count has it (a new life, the hands turned on).
void Clear();
// The hand at work is busy with it.
bool Holding();
// The fitted hold (shellposes.h): the shell's bone in the left glove's bind space, and its fingers. Before the first Update.
void SetHold(const float* shellInGlove, const float (*fingers)[12]);

// The shape, for tests and the drawing.
float PathLength(const Gun& gun);
// The shell's bone on the gun's frame u metres along the way in (clamped to it).
void ShellOnPath(const Gun& gun, float u, float* out);
// ... lying in the ejection port.
void ShellInPort(const Gun& gun, float* out);
// The glove of this side that holds a shell whose bone is at shellView (the view's space), as the hold has it.
void GloveFor(int side, const float* shellView, float* glove);
extern const float kHandInGlove[3];  // the hand's middle in the left glove's bind space (pouch.cpp's): the handle is reached for with it

}  // namespace shellload

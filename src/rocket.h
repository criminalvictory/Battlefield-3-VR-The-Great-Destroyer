#pragma once

#include <openxr/openxr.h>

namespace rocket {

// The launcher loaded by the left hand (the user, 2026-11-04: "I want the player to physically pull the rocket out of the launcher and then put
// a new one in"; docs\research\smaw.md). What's loaded is the long case on the launcher's bone 1, out behind the launcher. Fired, the spent
// case stays on. A squeeze of the left grip on it takes it; a roll of the wrist (or the start of a pull) turns it open, as the game's reload does,
// and it pulls off along the launcher's line; let go, it falls. A squeeze over either shoulder then gives a new one, held whichever way round
// suits the hand. Anywhere near the launcher's back it's caught, whichever way it's held: it goes onto the line, on by a push or by
// itself, and a roll of the wrist (or a moment's wait) turns it shut. Only then is the game told (hands.cpp: the rounds, the sound). Pure: no
// game calls and no logs (rockettest.exe runs it on scripted frames).
//
// Matrices: three rows, rotation | translation. The case's: its bone's space into the view's, as the bone is drawn. The view's space: the eye at
// the origin, x right, y up, z back. The launcher's frame: x left, y up, z to its muzzle.

struct Frame {
    float dt = 0.0f;  // seconds since the last frame
    bool launcher = false;  // the launcher is in the right hand this frame
    float launcherPose[12] = {};  // its frame into the view's space
    float headPose[12] = {};      // the view's space into the room's (y up, metres)
    bool left = false;      // the left hand this frame: the controller, or a test's
    float glove[12] = {};   // the left glove as its controller holds it, the gloves' bind space into the view's
    float squeeze = 0.0f;   // the left grip, 0-1
    // The body as pouch::BodyFrame places it, in the room: the neck point, and its heading (radians: forward is (-sin, 0, -cos),
    // right (cos, 0, -sin)).
    float neck[3] = {};
    float yaw = 0.0f;
    bool haveBody = false;
    bool loaded = false;  // the game's count, as the mod has it: a rocket in the launcher
    bool spare = false;   // rockets left to take
    bool gameReload = false;  // the game's own reload is under way (X held): its animation has the case
    bool forceTake = false;   // tests: the shoulder counted as reached wherever the hand is
    bool handFree = true;  // free hands: the left isn't on the launcher's pistol grip (it takes nothing then)
    bool held = true;      // free hands: a hand holds the launcher (false: it lies or flies into a hand; nothing caught at its back or taken off it)
    bool stowed = false;   // holsters: the launcher put away at its place (held false too): drawn nowhere, so no new one is taken at the belly either
};

enum class Show {
    kGame,  // where the game's animation has it
    kNone,  // drawn nowhere
    kAt,    // at pose
};

struct Result {
    Show show = Show::kGame;
    float pose[12] = {};   // the case's bone into the view's space, for kAt
    // kAt on the launcher (on its line): the same on its frame, for a launcher turned after this (the left hand steering it).
    bool onFrame = false;
    float rel[12] = {};
    float glove[12] = {};  // the glove where it holds the case on the launcher ...
    float gloveBlend = 0.0f;  // ... blended in by this much from where the hand is
    bool holding = false;     // a case in the left hand: the hand is busy
    bool empty = false;       // no live rocket in the launcher: the trigger mustn't fire
    // Events this frame.
    bool spent = false;     // fired: the spent case waits for the hand
    bool grabbed = false;   // the hand took hold of the spent case: this squeeze is spent (not the front grip's)
    bool unlocked = false;  // it's turned open
    bool freed = false;     // pulled clear of the launcher, in the hand
    bool took = false;      // a new one taken from over the shoulder: this squeeze is spent
    bool caught = false;    // caught on the launcher's line
    bool offLine = false;   // pulled off the line: back in the hand
    bool homing = false;    // it starts its last slide on
    bool seated = false;    // turned shut: the game is told
    bool letGo = false;     // let go of: it falls
    // It starts falling this frame (let go of, or slid off the back): where it is (its bone into the view's space) and its speed (the room, m/s). With
    // SetFallOwn(false) its fall is the caller's (loose.h: the real ground, a minute there): falling, it's drawn nowhere here.
    bool fell = false;
    bool fellFromHand = false;
    float fellPose[12] = {};
    float fellVelocity[3] = {};
    bool putBack = false;   // let go of at the shoulder: gone back
    bool cancelled = false; // gone without a fall (the launcher went, the hand was lost a second, the game reloaded)
    // A fresh squeeze the launcher could use, for the log: where the grip was from the neck (right, up, forward; metres) and why
    // nothing was taken if it wasn't.
    bool squeezed = false;
    float fromNeck[3] = {};
    const char* note = "";
    // The state's name (the log, the tests).
    const char* state = "";
    bool caseOn = false;   // the case on the launcher's back (loaded, spent, turning open, coming off, settling): a launcher let go of now lies with it
};

Result Update(const Frame& f);

// Everything as for a loaded launcher, nothing in the hand (a new life, free hands off).
void Clear();

// Whose the case's fall is: true its own (down to a floor a guess below the eyes, gone 1.3 s after), false the caller's (Result::fell starts it).
void SetFallOwn(bool own);

// The zone over the shoulder: taken above this far below the neck (m), behind this far in front of it, within this of it.
void SetZone(float below, float forward, float reach);
// How near the launcher's back a new one is caught (m: the hand, the case's own line), how near the spent case the hand takes it (m), how long
// on the line before it goes on by itself (s; below 0 only pushed), and how long at the end before it turns shut by itself (s; below 0 only
// by the wrist).
void SetMagnet(float catchHand, float catchCase, float grab, float dwell, float lockWait);

// Tests: the case's bone on the launcher's frame, s metres back along the line from its place at rest and twisted this far open (radians);
// the launcher's back on its frame; the case's line on its own bone and the place on the frame the spent case is held by.
void CaseOnLine(float s, float* rel, float twist = 0.0f);
extern const float kMouthOnFrame[3];
extern const float kFrontOnCase[3];
extern const float kCatchLine;
extern const float kTwist;
extern const float kForward;

}  // namespace rocket

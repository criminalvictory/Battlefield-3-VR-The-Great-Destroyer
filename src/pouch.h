#pragma once

#include <openxr/openxr.h>

namespace pouch {

// Magazines taken from over the left shoulder (ammozone.h: all ammo, every gun, the whole game; at the
// pelvis until 2026-11-04) and pushed into the gun by hand (the M416, with the left hand; the user,
// 2026-10-31; the design: three designs and a judge, the same night). The name "pouch" stayed.
// With the gun "hot" (its magazine out, or its bolt locked back on an empty one), a fresh squeeze of
// the hand's grip over the shoulder puts a magazine in its glove: the gun's own magazine,
// drawn on its own (skin.cpp). Brought top first up under the magazine well, it's caught in a funnel
// below the well's mouth that straightens it as it's pushed, slides only along the well and seats when
// pushed home: only then is the game told (hands.cpp: the rounds, the gun's magazine, its sound).
// Let go of before that, it falls, or goes back if let go at the shoulder. Pure: no game calls and
// no logs (Result::note says why a squeeze took nothing), so magtest.exe runs it on scripted paths.
// Either hand's (free hands, 2026-11-06: "pick up the magazine with my left hand, my right hand"):
// Frame::side says whose hand it's fed. The right's hand's middle, palm's way and the magazine's hold
// are the left's mirrored (the gloves' bind spaces are each other's mirror image across x = 0, as the
// gun's model is about its middle plane). One magazine at a time: there is one well.

struct Frame {
    float dt = 0.0f;  // seconds since the last frame
    // In the room (the recentred space, y up, metres): the head, and the hand's controller's grip point
    // (the middle of the fist; the hand is side's, as below).
    XrPosef head{};
    bool left = false;      // that hand this frame: the controller, or a test's (the name stayed)
    XrVector3f leftGrip{};
    float squeeze = 0.0f;   // its grip, 0-1
    // In the view's space (hands.cpp's): the gun's body (its model space into the view's) and that
    // hand's glove as its controller holds it (the gloves' bind space into the view's).
    bool gun = false;  // the gun the pouch knows (the M416), its own firing state, its magazine's seat known
    bool gunTracked = true;  // gunPose is this frame's (the controller holding it seen): without it the magazine's
                             // kept a second, as with a lost hand
    float gunPose[12] = {};
    float glove[12] = {};
    float guardDistance = 1.0f;  // that hand's palm from the handguard's line (metres)
    bool handFree = true;        // not on the handguard or the charging handle, not lining the glove up
    bool wellEmpty = false;      // the gun's magazine is out
    bool dry = false;
    bool atHandle = false;  // the palm's on the charging handle: a press there is that's, not one over the shoulder            // its bolt is locked back
    bool spare = false;          // rounds left for another magazine
    bool forceInside = false;    // tests: the pouch counted as reached wherever the hand is
    // The belly is the grenade launcher's now (m320.h's bellyForM320: its barrel open, opened after the rifle's magazine came out): no magazine from it (the user,
    // 2026-11-05: "I don't want it to confuse ... the 40 millimeter or the magazine"; "If the M320 is open, then it wants to").
    bool bellyTaken = false;
    // Whose hand left/leftGrip/squeeze/glove/guardDistance/handFree/atHandle are: 0 the left, 1 the right (its hand's middle, palm's way and the
    // magazine's hold the left's mirrored). A new side fed: a magazine held is let go of, and its squeeze counts once it's been under 0.4.
    int side = 0;
    // The hand with a magazine can't keep it (the rifle has left the hands, another hand works it, free hands off for the right): let go of (it
    // falls, or goes back at the belly), never cancelled; a hand on the seated one leaves it in.
    bool dropHeld = false;
};

struct Result {
    bool mag = false;      // a magazine to draw: magPose, where the gun's body would be with it seated
    float magPose[12] = {};
    bool hideSeated = false;  // the gun's own not drawn: the hand is drawing it down the well
    float glove[12] = {};  // the glove where it holds the magazine on the well's line ...
    float gloveBlend = 0;  // ... blended in by this much from where the hand is
    bool holding = false;  // a magazine in the hand (the hand is busy: no handguard, no bolt)
    // The hand where a let go puts a magazine back (5 cm more than the zone), this frame: the train
    // pistol's magazine goes by it too (pistolmag.cpp, hands.cpp), its zone the same as the M416's.
    bool atPouch = false;
    // Events this frame.
    bool reached = false;  // a fresh squeeze reached the zone (another gun: its own reload)
    bool took = false;     // a magazine taken: this squeeze is spent (not the handguard's)
    bool grabbed = false;  // the seated one taken hold of: this squeeze is spent ...
    bool toGrip = false;   // ... and the hand went on to the gun instead: it's the handguard's after all
    bool pulledOut = false;  // ... and drawn down out of the well: it's in the hand, the gun is told
    bool caught = false;   // caught in the funnel under the well
    bool offLine = false;  // pulled off the well's line: back in the hand as it's held
    bool seated = false;   // pushed home: the game is told
    bool letGo = false;    // let go of: it falls from where it was drawn last ...
    float dropVelocity[3] = {};  // ... at the hand's speed then (room, m/s, at most 3)
    bool putBack = false;  // let go of at the shoulder: gone back
    bool cancelled = false;  // gone without a fall (another magazine went in, the gun went, the hand or the
                             // gun lost a second, or let go of while lost)
    // A fresh squeeze within 80 cm of the neck, for the log: where the grip point was from the neck in
    // the body's axes (right, up, forward; metres), how far out of the zone (1 its edge), and why
    // nothing was taken if it wasn't.
    bool squeezed = false;
    float fromMiddle[3] = {};
    float size = 0.0f;
    const char* note = "";
    int side = 0;  // whose glove the magazine, glove/gloveBlend and the events are: the holding side (also while its glove eases back), else Frame::side
};

Result Update(const Frame& f);
// A magazine is in either hand, or on its way into the well.
bool Holding();
int HoldingSide();  // the hand with a magazine (or drawing the seated one down): 0, 1; -1 none
// The body as the pouch places it, as of the last Update (hands.cpp's forearms go by it too): the
// neck point (7.5 cm below and 8 cm behind the eyes in the head's axes) in the room, and the body's
// heading (radians about the room's up: forward is (-sin, 0, -cos), right (cos, 0, -sin); it follows
// the head beyond 35 degrees, settles towards it slowly and jumps with a recentre: pouch.cpp). False
// before the first Update.
bool BodyFrame(float neck[3], float& yaw);
// Gone without a fall (free hands off).
void Clear();

// build\pouch.txt (hands.cpp reads it; grip.txt is rewritten whole when the glove is lined up):
// "zone below forward reach": how far below the neck, in front of it and from it a magazine is taken
// (metres; ammozone.h's unless changed); "pouch off" turns it off.
void SetZone(const float* zone, bool on);
// "hold" and 12 numbers: the magazine's place in the left glove (rows, rotation | translation, from
// the gun's model space with the magazine seated into the gloves' bind space); the right glove's is
// its mirror image, made again with it.
void SetHold(const float* hold);
// That place as it is now (the guns with no button start from it: handmag.cpp): the left glove's.
void GetHold(float* hold);
// Where the left glove is to hold a magazine at magPose (tests).
void GloveFor(const float* magPose, float* glove);
void GloveFor(const float* magPose, float* glove, int side);        // that side's glove holding a magazine at magPose (side 1: the hold mirrored)
bool OnSeated(const float* gunPose, const float* glove, int side);  // that side's glove where a fresh squeeze takes hold of the seated magazine (Update's own test)

}

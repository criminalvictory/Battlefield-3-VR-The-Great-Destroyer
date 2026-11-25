#pragma once

namespace handmag {

// The guns with no magazine button: a paddle behind the well (the AK family, the SVD, the SKS, the G3, the SG553, the AS Val...; the user,
// 2026-11-05: "any gun that doesn't have an eject button... needs to have the magazine of the gun removed by hand"). X does nothing to them. Two ways:
//   - Casual: "the player can grab the magazine in the gun and pull it out". A fresh squeeze of the left grip with the hand on the seated magazine takes it; it rocks forward about its
//     front hook as the hand pulls it down and comes free in the hand. Let go of, it falls, or goes back if let go at the shoulder.
//   - Afghan: "grabbing a magazine and flicking out the magazine that's inserted already into the gun". A fresh magazine taken over the left shoulder with the old one still in, its front corner
//     knocked into the old one's back as the hand sweeps forward: the old one flies off forward, tumbling (the AN-94's own reload, measured: docs/research), and the fresh one goes in. The magazines have to
//     touch and it takes force (the user, 2026-11-05: "I want those magazines to touch", "more force into it"): gently, the old one gives a little and the fresh one stops against it.
// A magazine with the well empty goes home by itself as soon as it's near, whichever way it comes, gun up at the eye or not (the user, 2026-11-05: "a lot more magnetic and snappier", and for reloading while
// aiming: "I clip through. So try to make that magnetic as well").
// The side lever (the user's video, 2026-11-05: "either like this or like this", a fist reaching over the top with the back of the hand showing, or from below with the fingers showing):
// the left trigger or grip near its knob closes the fist on it, with the wrist as the real hand has it, so either grip is simply how the hand comes in; or, for a lever that hardly stands off the gun, the hand as the game's
// own reload has it on the lever (leverHold). Back with the hand, let go, it springs home and chambers a round if the magazine has any.
// Everything here is taken and held by either the left grip or the left trigger (the user, 2026-11-05: "embed both").
// Pure: no game calls and no logs, as pouch.cpp. hands.cpp tells the game.

// A belt fed gun (the M240; the user, 2026-11-05: "Open up the flap, take out the box, and then insert a new box... I don't want a bullet rail. I need to drag that bullet rail into its position. And then when it's there, close the
// flap, and then bolt"): its lid up by the hand on its back end; its box out by the hand, with the lid up; a new box goes home by itself near its seat, its belt hanging out of it; the belt taken by the hand and
// brought to the feed, which takes it near there; the lid shut by the hand; then the lever. As the game's own reload has them (docs/research/other-guns.md).
struct BeltGun {
    float lidHinge[2];   // y, z on the frame: the lid turns about x through here
    float lidOpen;       // radians all the way up
    float lidGrab[3];    // where the hand takes it, with it shut
    const float* lidHold;  // the hand on it (tools/magfit.cpp), as a lever's
    const float (*lidFingers)[12];
    // The belt's five bones on the frame, hanging out of a new box and lying in the feed, as the game has them (src/magposes/m240_belt.h). In the hand, between the thumb and the pointer ("I want him actually holding the bullet rail"),
    // its end is where the hand has it and the rest a curve from there back to the box.
    const float (*hanging)[12];
    const float (*laid)[12];
    float pinch[3];        // what the hand holds, in the gloves' bind space
    const float (*pinchFingers)[12];
    float feed[3];         // the belt's end lying in the feed, on the frame
    float anchor[3];       // where it comes out of the box
    float pieces[5][4];    // each piece of it in its own bone's space: x, y of its end nearer the feed, x, y of its end nearer the box
    float pieceZ;          // the middle of its rounds along z, in those spaces
    // Where weapon.txt "heldtest 7"'s scripted hand pinches it, hanging out of a new box, on the frame (the M240's as filmed); none: its first piece's
    // free end as it hangs (HangingTip).
    float testTip[3] = {};
    // What rides its lid (lidride.h: a draw of its own the game skins to the lid's bone, a sight on the cover's rail) turned up with it as the hand lifts
    // it (skin.cpp DrawHeldPart): the M249's. Not the M240's yet: as it was approved, bit for bit, until a film shows its riders (its recordings have
    // draws with its lid's bone among theirs: tools/m249test.cpp w6).
    bool carries = false;
    // A sight on the cover's own mount, which rides it as its own bone does (lidride::OnMount, with carries): that bone's place in the cover's bone,
    // its axes the cover's (the bridge M249's ACOG, measured 2026-11-19: m249rows::kSightMount). None: only a draw skinned to the cover's very bone.
    const float* mount = nullptr;
};
// The belt's free end hanging out of a new box, on the frame: its first piece's end nearer the feed, as hanging has it.
inline void HangingTip(const BeltGun& b, float* out) {
    const float p[3] = {b.pieces[0][0], b.pieces[0][1], b.pieceZ};
    const float* m = b.hanging[0];
    for (int i = 0; i < 3; ++i)
        out[i] = m[i * 4] * p[0] + m[i * 4 + 1] * p[1] + m[i * 4 + 2] * p[2] + m[i * 4 + 3];
}

// A box that comes off and goes on the way its own reload moves it (the M249's pouch, the review of 2026-11-18: its own reload slides it 8 cm to the
// gun's left with under 1 cm of drop, then takes it left and down; a fresh one comes back in from the left at the seat's height, turned 5.5 degrees
// about the gun's length, and settles). The owner's rule: the game's own animation first. Rows (src/magposes/<gun>_way.h, tools/boxway.cpp from the
// gun's own empty reload): {how far along, then the box on the gun's frame from its seat (3x4)}.
//   off: from the seat (the first row), by metres its grab point has come along the way: the hand's pull is followed along it (the nearest point of the
//        way to how far the hand has come since it took the box, looked for from where the box is, the box going along it no faster than the hand moves:
//        never leaping to another part of it), the box drawn there, and free once it's come `free` along; the hand 8 cm off the way first (straight down,
//        or steeply down) leaves it in, whatever the pull. The rows go on past `free` as the game's own way goes on (left and down), never past the last.
//   on: from where a fresh one starts in (by its seat, at its height, turned as the game's comes in) to the seat (the last row), by seconds of the
//        game's own. The magnet takes a fresh one whose top comes near where it is in the first row, and brings it there, then home along the rest.
// None (the M240's, every other gun's): off straight down or rocked off a hook, home up into the well, as before.
struct BoxWay {
    const float (*off)[13];
    int offCount;
    float free;
    const float (*on)[13];
    int onCount;
};

// In the gun's frame bone's space (x left, y up, z to the muzzle), with its magazine seated.
struct Gun {
    float top[3];        // the seated magazine's top middle
    float grab[3];       // its middle: where the hand takes it
    float paddle[3];     // the release behind the well
    float hook[3];       // its front hook, the top's front corner: what it rocks about, and what touches an old one
    float mouth;         // the top's place along the well's line (y, from home) at the receiver's underside
    float lever[3];      // the side lever's knob
    float leverTravel;   // as far back as it goes
    float rockIn;        // radians it's turned, bottom forward, as it comes into the well
    float handOffset[3]; // the magazine's place from the M416's where the hand holds it (pouch.cpp's hold), for a gun not fitted yet
    // The hand fitted round this gun's own magazine (magposes.h, tools/magfit.cpp): the magazine in the glove, from the gun's frame with it seated to the gloves' bind space, and the glove's bones.
    const float* hold = nullptr;
    const float (*fingers)[12] = nullptr;
    // A magazine button (the M16, the M4, the KH2002...): X drops the magazine, as the M416's; the hand can take it too, straight down the well (the user, 2026-11-05: "let's also implement being able to take out the
    // magazine with our hand"); a fresh one can't knock it out. A fresh one still comes from over the left shoulder, and the lever is its charging handle.
    bool button = false;
    // Nothing a fresh one could knock out (the M240's box).
    bool noKnock = false;
    // The seated magazine's back edge, for a gun that can have it knocked out: {y, z} on the frame at its top, its middle, its bottom.
    float back[3][2] = {};
    // The hand on the side lever as the game's own reload has it (tools/magfit.cpp): from the gun's frame with the lever at rest to the gloves' bind space, and the glove's bones. Null: a fist with the real wrist.
    const float* leverHold = nullptr;
    const float (*leverFingers)[12] = nullptr;
    // A bolt that locks back on an empty magazine, as the M416's (the M16, the M4): the glove on the release on the receiver's left side, or the handle, sends it home (hands.cpp, reload.cpp's Slapped).
    bool boltCatch = false;
    float release[3] = {};
    // A handle that swings out to be drawn (the G3's, far forward on its left): this many radians about the gun's y, through this place (x, z on the frame), then back along the gun; the hand's table is on it swung out.
    float leverSwing = 0.0f;
    float leverPivot[2] = {};
    // A lever the game doesn't move as the gun fires (the RPK's: the user, 2026-11-05: "I didn't see the lever like open and close"): this far back and home with each shot.
    float kickLever = 0.0f;
    const BeltGun* belt = nullptr;
    // The hand on the lever from below, fitted (tools/magfit.cpp "under"), a fist round it with the palm up: the user, 2026-11-05: "let's have an under and over lever pull"; which, by which way the palm faces as it's taken. Null: leverHold whichever way; with no
    // leverHold either, a fist with the real wrist, which is both already.
    const float* leverUnderHold = nullptr;
    const float (*leverUnderFingers)[12] = nullptr;
    // A magazine with no button in a deep, straight well (the UMP-45's; its own reload draws it straight down 22 cm): the hand slides it down the well until its top is out of it (mouth), as a
    // button gun's goes by hand, not rocked off a hook. X still does nothing: there's no button to press.
    bool straight = false;
    // A handle that's turned up into a notch at the back of its travel and stays there, the bolt held open, until it's knocked down (the UMP-45's, the HK way: its own empty reload draws it back
    // 8.3 cm, turns it 52 degrees up and leaves it there while the magazine is changed, then knocks it down): this many radians about the gun's z, through this place (x, y on the frame); in the
    // notch it rests this far back. By hand: drawn back most of the way and the hand lifted, it goes into the notch and out of the hand; a hand coming down on it there, or taking it again, knocks
    // it out and the bolt goes home. Let go of without the lift it springs home, as any lever. 0: no notch.
    float leverNotch = 0.0f;
    float leverNotchAxis[2] = {};
    float leverNotchBack = 0.0f;
    // A box off and on along its own reload's way (BoxWay above). None: as every gun before it.
    const BoxWay* way = nullptr;
    // A wide box taken anywhere along it (the M249's pouch, 19 cm across the gun; the review of 2026-11-18: "a hand on the middle or right of the pouch
    // gets nothing"): the hand's middle measured from the nearest point of the line from grab, along the gun's x, to grabTo (its far end), not from grab
    // alone. Taken within kGrabReach of grab, the hand goes on the box as the table holds it there (the game's own place on it); further along, it's held
    // where the hand took it, as the hand is (the glove carried with it along its way, in the hand after as it was taken: the second review of 2026-11-18,
    // the glove had been drawn on the game's own place 20 to 30 cm from the real hand). The pull is the hand's own move from where it took it. By its far
    // end, under a side lever's knob, the nearer of the two is taken, the box only by a hand below its top whose palm isn't up as a hand going for the
    // lever from under it has it.
    bool wide = false;
    float grabTo = 0.0f;
};

struct Frame {
    float dt = 0.0f;
    const Gun* gun = nullptr;  // such a gun in hand
    bool gunTracked = true;    // gunPose is this frame's
    float gunPose[12] = {};    // the gun's frame bone into the view's space
    bool left = false;         // a left hand this frame
    float glove[12] = {};      // the left glove as its controller holds it (the gloves' bind space into the view's)
    float gripRoom[3] = {};    // its grip point in the room: the hand's speed
    float squeeze = 0.0f, trigger = 0.0f;  // the left grip and trigger, 0-1: either takes hold
    float frontGrip[3] = {};   // where the left hand holds this gun, on its frame: a squeeze nearer that than the lever is the front grip's
    bool shoulder = false;     // a fresh squeeze reached the zone over the left shoulder (pouch::Result::reached)
    bool atShoulder = false;   // the hand where a let go puts a magazine back
    bool wellEmpty = false;    // its magazine is out
    bool spare = false;        // rounds left for another
    bool handFree = true;      // not on the front grip, not lining the glove up
    // Free hands: the gun has left the hands (loose has it, or it flies into a hand from the ground): a magazine in the hand is let go of (it falls, or
    // goes back at the shoulder), never cancelled; the seated one, the lever, the lid and the belt stay on the gun as they are.
    bool dropHeld = false;
};

struct Result {
    bool mag = false;           // a magazine to draw: magPose, where the gun's frame would be with it seated
    float magPose[12] = {};
    bool hideSeated = false;    // the gun's own not drawn: the hand is rocking it out
    float glove[12] = {};       // the glove on the magazine or the lever ...
    float gloveBlend = 0.0f;    // ... blended in by this much from where the hand is
    bool holding = false;       // a magazine in the hand: busy
    bool fist = false;          // the fingers closed on the lever
    const float (*leverFingers)[12] = nullptr;  // ... these: the hand as the game has it on the lever
    float leverBack = 0.0f;     // how far back the lever is drawn (m)
    float leverSwing = 0.0f;    // how far a handle that swings out is swung out (radians)
    bool spentSqueeze = false;  // this squeeze is used: not the front grip's as well
    bool oldRocked = false;     // the seated one gives a little under a fresh one: drawn at oldRock from its seat, in the gun's space
    float oldRock[12] = {};
    // Events this frame.
    bool took = false;        // a fresh magazine from over the shoulder
    bool grabbed = false;     // the seated one taken hold of
    bool toGrip = false;      // ... and the hand went on to the gun instead: the squeeze is the front grip's after all
    bool pulledOut = false;   // ... and pulled free: it's in the hand, the gun is told
    bool touched = false;     // a fresh one against the old one, too gently to knock it out: at hitSpeed
    bool flicked = false;     // knocked out by the fresh one, at hitSpeed: it flies at flickVelocity (the view's space)
    float hitSpeed = 0.0f;    // m/s along the gun
    float flickVelocity[3] = {};
    bool caught = false;      // near the well with it empty: on its way home by itself
    bool seated = false;      // home: the game is told
    bool letGo = false;       // let go of: it falls from where it was drawn last ...
    float dropVelocity[3] = {};  // ... at the hand's speed then (the room, m/s, at most 3)
    bool putBack = false;     // let go of at the shoulder
    bool cancelled = false;   // gone without a fall (the gun went, the hand was lost a second, another went in)
    bool leverTaken = false;  // the fist closed on the lever
    bool leverMoving = false; // ... it's come back far enough to be heard
    bool charged = false;     // ... drawn far enough and let go: a round chambered
    float leverPulled = 0.0f; // with leverLet: how far it was drawn
    bool leverLet = false;    // let go of
    // A handle with a notch (Gun::leverNotch).
    float leverNotch = 0.0f;  // how far it's turned up into it (radians)
    bool leverHeld = false;   // it rests in its notch: the bolt held open, nothing fires
    bool notched = false;     // it's gone into its notch (and out of the hand)
    bool knocked = false;     // it's been knocked down out of it: the bolt goes home, a round chambered if the magazine has any
    const char* note = "";    // why a squeeze took nothing
    // A fresh squeeze near the seated magazine left for the front grip: where the hand was from its middle, and how far up the palm faced (for tuning).
    bool leftForGrip = false;
    float squeezeFrom[3] = {};
    float palmUp = 0.0f;
    // A belt fed gun.
    float lid = 0.0f;           // how far up its lid is (radians)
    bool beltDrawn = false;     // its belt's not lying as the game has it: these, on the frame
    float beltBones[5][12] = {};
    bool beltReady = true;      // it can feed: the belt laid, the lid shut, a box in
    bool lidTaken = false;      // the hand on the lid
    bool lidUp = false;         // it's reached the top
    bool lidShut = false;       // it's come down
    bool beltTaken = false;     // the hand on the belt
    bool beltLaid = false;      // in the feed
    bool beltLet = false;       // let go of before it's in
};

Result Update(const Frame& f);
// A fresh squeeze of this glove now would be this gun's (its seated magazine, its lever over or under, its lid, its belt), as Update would take it: the
// regrip leaves it alone (FreeHands' claim). Pure: nothing changes.
bool Claims(const Gun& gun, const float* gunPose, const float* glove, const float* palm);
// A magazine is in the hand, on its way into the well, or being rocked out.
bool Holding();
// Gone without a fall.
void Clear();
// A belt fed gun as it's left by the game's own reload or at a new life: the belt laid, the lid shut. A handle in its notch likewise: home.
void BeltReset();
// The magazine in the left glove (pouch.cpp's, moved by the gun's handOffset): from the gun's frame with it seated to the gloves' bind space.
void HoldFor(const Gun& gun, float* hold);
// Where the glove is to hold a magazine at magPose (tests).
void GloveFor(const Gun& gun, const float* magPose, float* glove);
// weapon.txt, to tune them with the user in the headset: how fast a fresh one has to hit the old one to knock it out (m/s along the gun), how near the well a magazine goes home by itself (m, its top from the well's
// mouth), how long it takes (s).
void SetKnockSpeed(float metresPerSecond);
void SetMagnetReach(float metres);
void SetFlyTime(float seconds);

}

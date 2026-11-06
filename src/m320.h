#pragma once

#include "handpose.h"

// The grenade launcher under the rifle (the M416's M320) reloaded by hand. The user, 2026-11-05: "I want the original reload that the game has to work, and I also
// want to be able to expel ... the spent case. I want it to also fall out via gravity. So if I aim the gun upwards, it should be able to fall out ... And then of
// course you take a new .40 millimeter and you shove it in." As the real one works (its army manual, TM 3-22.31; H&K's patent): the index finger of the hand on the
// launcher's grip pushes the barrel release up, and two springs swing the barrel's back end out to the gun's LEFT about a pin at its front, 15 degrees, as the
// game's own reload swings it (build\survey\M320_vanilla.bin). Nothing pulls the spent case: it sits loose in the barrel, so it slides out by itself once the
// muzzle is up enough ("falls out ... by centrifugal action, or manually", the patent), or a jab of the gun throws it, or the hand takes it by the rim and pulls
// it all the way out along the barrel (free only once its nose is out). A new round from the belly, like all the ammo (the user, 2026-11-05: the 40mm from the belly
// "as long as it knows whether I want the 40 millimeter or I want a regular magazine ... I don't want it to confuse"): the belly gives what's open, the launcher's
// barrel open a 40mm, the rifle's magazine out a magazine, both open whichever was opened last. Carried, the hand holds it by its base end, the thumb
// underneath, the index and middle on top (the user: "at most three fingers ... on that circle base"), and keeps that grip as it pushes it in. Put in by hand, not by a magnet (the user, 2026-11-05: "too snappy ... too magnetic. I feel like it did it on its own"): a
// live round's nose brought into the open barrel's back end, in line, goes onto the barrel's line and from then on moves only as the hand pushes it, the hand on
// its rim; pushed to its last 2 cm it goes home with a click. Then a push of the hand on the barrel's left side swings it shut, snapping the last of the way in
// with a click, as the game's own does. The release (a pull of its trigger, fired) opens it; the quick roll of the wrist that also opened it is off (the user,
// 2026-11-06: "I would flick my gun and it would open ... get rid of the flick to open"; SetFlickOpens), and the roll the other way still shuts it. The game's own reload
// stays on X held with the hand on the launcher's grip, and the right trigger always fires the rifle. Only the grip, the trigger and the hand's movement are
// used, so it works on Quest 2 and 3 as on the Frame.
//
// Each frame hands.cpp gives a Frame and turns the Result into the game's count (ammo.cpp: one M320LoadByHand per round seated, one M320Unload per live round
// out), sounds and drawing. Pure: no game calls and no logs (m320test.exe runs it on scripted frames).
//
// Spaces. Matrices are three rows: rotation | translation. The gun's space is the M416's body (draw 12768's bone 2): x to the gun's left, y up, z to the muzzle,
// metres. The view's: the eye at the origin, x right, y up, z back. The room's: y up, metres. A glove's matrix takes the gloves' bind space into the view's.
// Both hands are kept apart by side (0 the left, 1 the right) for left-handers later: the hand at work is whichever isn't on the rifle's grip (gunHand); the
// barrel opens to the gun's left for either, as the real one only does. The glove's own holds are the game's left hand's; the right's are mirrored stand-ins
// (each hand reaches for the rim where its own glove is drawn, the right on the round's right; the right's on the barrel lies on the gun's right, 19 cm from
// where it's taken: a hold of its own still to fit).
namespace m320 {

// One hand as hands.cpp has it this frame.
struct HandIn {
    bool tracked = false;             // its controller this frame (or a test's hand); a glove or grip point with a number not finite is taken as not
    float glove[12] = {};             // the glove as its controller holds it: the gloves' bind space into the view's (hands.cpp's "tracked")
    float gripRoom[3] = {};           // its controller's grip point in the room: where the belly is measured from, exactly as pouch.cpp does
    float squeeze = 0.0f, trigger = 0.0f;  // its grip and trigger, 0-1: either takes hold (the user, 2026-11-05: "embed both")
    bool onGripB = false;             // on the launcher's own grip (hands.cpp: g_supporting && g_onM320 && !g_holdOutOfReach)
    // Holding something else: a magazine, the charging handle, the handguard, or the launcher's grip with the arm out of reach (any hold of the handguard's
    // code that isn't onGripB: a pull of the trigger there mustn't take a round). Not this module's own.
    bool busy = false;
    // The glove's points that push the barrel shut, as drawn last frame (its palm's middle, the index, middle and ring finger tips; the view's space).
    // None given: the palm's middle and the straight fingers' tips, through glove.
    int touches = 0;
    float touch[4][3] = {};
    bool forceBelly = false;          // tests: the belly counted as reached wherever the hand is
};

struct Frame {
    float dt = 0.0f;                  // s since the last frame
    // In use: the M416 in the right hand, its launcher on it, the free hands on, not seated in the car (the module adds its own m320byhand). Out of use for
    // under 0.2 s, everything is kept (a state read failed for a frame); longer, whatever the hand has goes.
    bool active = false;
    bool gunTracked = true;           // gunPose is this frame's (the right controller seen); a pose with a number not finite is taken as not
    float gunPose[12] = {};           // the gun's space into the view's, this frame, as the right hand alone holds it (before the left hand's steer)
    bool haveGunRoom = false;         // gunRoom is the last frame's
    // The gun as drawn last frame, in the room (headPose times the steered gunPose): its turn and speed tip, flick and jab it. A frame late, so the module
    // takes each difference over the frame it spans (the last Update's dt), and reads nothing across a frame of 0.1 s or more.
    float gunRoom[12] = {};
    float headPose[12] = {};          // the view's space into the room's
    HandIn hand[2];
    int gunHand = 1;                  // the hand on the rifle's grip: never read as the launcher's
    // The body as pouch::BodyFrame places it, in the room: the neck point and its heading (forward (-sin, 0, -cos), right (cos, 0, -sin)).
    float neck[3] = {};
    float yaw = 0.0f;
    bool haveBody = false;
    bool loaded = false;              // a live round in it as the mod counts (ammo::M320Loaded() >= 1)
    bool spare = false;               // rounds left to take (ammo::M320Spare() >= 1)
    // The game has it in hand (ammo::M320Up). Asked by nothing now: it reads in hand long after a put-away (a pull eaten, a flick refused, every time).
    bool inGameHand = false;
    // input.cpp's own shot or game reload of it under way (input::M320ShotUnderway: picked, firing, reloading, the rifle coming back): the flick doesn't open it.
    bool shotUnderway = false;
    bool gameReload = false;          // the game's own reload (X held on its grip): it has the launcher until its round is in
    bool releasePress = false;        // the release pressed (TakeReleasePress: input.cpp's pull of the trigger on its grip)
    bool rifleOpen = false;           // the rifle wants a magazine (skin::MagazineOut() || reload::BoltBack()): the belly's other claim
    int test = 0;                     // weapon.txt "m320test": 6 a scripted roll fed to the flick (open, then shut a second later, every 3 s)
    // The left glove's palm bone as hands.cpp draws it (handpose bone 1 at its palm width). hands.cpp re-anchors the game's hand tables on it (the glove at a hold
    // drawn as gloveOnGun times rebase's inverse, its fingers as rebase times the table, rebase = this palm times the table's palm's inverse), so the module takes
    // the round in the hand, and the hand's middle at each hold, through the same: everything it reaches for or catches is where it's drawn. Not given (tests):
    // the tables as they are.
    float palm[12] = {};
    bool havePalm = false;
};

// The round in the barrel as drawn.
enum class Round {
    kGame,    // where the game has it, swung with the barrel and slid roundOut back along it (skin.cpp's g_m320RoundIn true, g_m320RoundOut)
    kHidden,  // none in it (g_m320RoundIn false)
    kOnGun,   // at roundOnGun: on its way home by itself, off the barrel's line
};
// The glove's own hold on the launcher (the game's left hand in its reload, build\survey\M320_vanilla.bin).
enum class Hold {
    kNone,
    kPinch,   // thumb over, index under the rim (frame 7350): the case pulled out, a round pushed in
    kPush,    // the thumb on the round's base (frame 7433): the end of the round's way home
    kBarrel,  // the fingers on the barrel's left side (frame 7469): the barrel swung by the hand
    kCarry,   // the round held by its base end, thumb under, index and middle on top, straight out along the index (SetHold: m320carry.h, the user's grip A); unset, the pinch
};
enum class Barrel { kShut, kSpringing, kOpen, kHeld, kPushed, kSnapping };
enum class Content { kNone, kSeated, kSliding, kPinched, kFlying };
enum class HandState { kFree, kPinch, kRound, kBarrel, kRiding };

struct Result {
    // ---- The launcher drawn ----
    bool drawn = false;               // the module has its parts (false: as the game has them: not in use, or the game's own reload; a round in the hand still is)
    float barrel = 0.0f;              // swung open (radians: skin.cpp's g_m320Barrel; below 0 a little past shut, its snap)
    float hinge[12] = {};             // that turn in the gun's space (skin.cpp's M320Hinge)
    float tubeOnGun[12] = {};         // the tube's bone (7848's bone 1) on the gun: hinge times its place at rest
    Round round = Round::kGame;
    float roundOut = 0.0f;            // kGame: slid this far back out along the open barrel (m: g_m320RoundOut)
    float roundOnGun[12] = {};        // the round's bone (1440's bone 0) on the gun, whatever round says (but kHidden)
    bool roundLive = true;            // a live round, or a spent case
    // A round in the hand at work: a new one, or the old one pulled out.
    bool handRound = false;
    int handSide = 0;
    bool handRoundLive = true;
    float roundInGlove[12] = {};      // its bone in the gloves' bind space: drawn at the glove as finally drawn times this
    float handRoundView[12] = {};     // ... the same in the view's space from this frame's glove (the hand lost: kept where it was in the room)
    float trigger = 0.0f;             // the launcher's trigger blade pulled, 0-1: the finger on it, on its grip
    // ---- The hand at work ----
    int side = 0;
    float gloveOnGun[12] = {};        // its glove on the gun (the gloves' bind space into the gun's space) ...
    float gloveBlend = 0.0f;          // ... blended in by this much from where the hand is (hands.cpp: BlendAbout, after the steer)
    Hold hold = Hold::kNone;
    // Its fingers as the game's hand has them there (each bone in the gloves' bind space, relative to the glove), blended from fingers to fingersAlt by fingersMix,
    // then in by fingersShare from the controller's own fingers. Null: the controller's own (the right glove has none yet).
    const float (*fingers)[12] = nullptr;
    const float (*fingersAlt)[12] = nullptr;
    float fingersMix = 0.0f;
    float fingersShare = 0.0f;
    float indexOnRelease = 0.0f;      // on the launcher's grip, the index finger off the trigger onto the barrel release (0-1) ...
    float releaseLift = 0.0f;         // ... pushing it up this far (m, 0-4 mm with the trigger)
    bool holding = false;             // the hand at work is busy here: not the pouch's, not the handguard's, not the glove's collisions
    bool spentSqueeze = false;        // this frame's squeeze was used here: not the handguard's or the pouch's as well
    // ---- The launcher's state ----
    bool fireReady = true;            // shut, latched and a live round in it as the mod counts: the left trigger fires (FireReady); true while not in use
    bool latched = true;
    bool bellyForM320 = false;        // its barrel open and opened after the rifle's magazine came out: the belly gives a 40mm (or nothing)
    bool wantsRound = false;          // ... and it would take one now (rounds left, none live in it, the hand free)
    float m320OpenedAt = -1.0f;       // when its barrel was last unlatched (s on the module's clock; -1 never)
    Barrel barrelState = Barrel::kShut;
    Content content = Content::kSeated;
    HandState handState = HandState::kFree;
    float roundS = 0.0f, roundV = 0.0f;  // the round in the barrel: m back out along it, m/s
    // ---- Dropped: it leaves the module (skin.cpp's world-kept drop, 60 s on the floor). One a frame: a round slid out of the barrel the same frame the hand
    // lets go of one waits that frame at the barrel's back end, drawn, and drops the next. ----
    bool drop = false;
    bool dropLive = false;            // a live round (back with the spare ones already, or never counted)
    bool dropFromBarrel = false;      // slid out of the barrel (else let go of from the hand)
    float dropView[12] = {};          // its bone in the view's space as it goes
    float dropVelocityRoom[3] = {};   // its speed then (the room, m/s, at most 3): the slide plus the gun's own at the breech, or the hand's
    float dropSlideRoom[3] = {};      // ... the slide's part alone (for a drop that adds the gun's own speed itself)
    // ---- Events this frame ----
    bool released = false;            // unlatched (the release, the wrist's flick, or the hand on the breech): open_latch
    bool openStop = false;            // at its 15-degree stop: open_stop
    bool contentMoving = false;       // the round in it starts out (gravity, a jab, the hand): case_out
    bool unload = false;              // a live round out of it: ammo::M320Unload (back with the spare ones)
    bool took = false;                // a new round from the belly (not counted until it's in)
    bool putBack = false;             // a live round let go of at the belly: back
    bool entered = false;             // a live round's nose into the open barrel, in line: on its line, the hand on its rim, pushed in by the hand
    bool caught = false;              // pushed in to its last 2 cm: home with a click (round_in), the hand riding it
    bool seated = false;              // a live round home: ammo::M320LoadByHand
    bool caseSeated = false;          // a spent case pushed back in (nothing counted)
    bool shut = false;                // the barrel hits shut: close (its hit 10 ms in, the latch 66 ms in)
    bool latchedNow = false;          // ... latched after its little bounce: the trigger fires again
    bool dryPull = false;             // a pull of the trigger with it open: dry
    bool flickOpen = false, flickShut = false;  // the wrist's flick did it
    bool barrelTaken = false, barrelLet = false;  // the hand on the barrel's back end, and off it
    bool sprungBack = false;          // let go of short of the snap: its springs push it back open
    bool slapped = false;             // a slap of the hand at its left side: snapping shut
    bool blocked = false;             // shutting it stopped at 10 degrees: the round half out is in the way
    bool pinched = false, freed = false;  // the hand on the round's rim, and the round pulled free into the hand
    bool jabbed = false;              // a jab of the gun threw the round back
    bool letGo = false;               // a round let go of from the hand: it falls (drop)
    bool cancelled = false;           // a round in the hand gone without a fall (the hand lost a second, the launcher gone, the game's reload)
    bool fired = false;               // the count fell: the spent case stays in
    bool reloaded = false;            // the count came up without the hand (the game's reload, a checkpoint): shut and loaded
    // A fresh squeeze the launcher looked at and didn't take (for the log): where the grip point was from the neck (right, up, forward; m) and why.
    bool squeezed = false;
    float fromNeck[3] = {};
    const char* note = "";
    const char* state = "";
};

Result Update(const Frame& f);

// Shut, latched, a live round in it, nothing in the hands (a new life, the free hands turned on); the first frame after takes the round's state from the count.
void Clear();
// The hand at work is busy with the launcher (the pouch's and the handguard's gate).
bool Holding();

// For input.cpp (any thread).
// Shut, latched and loaded: a pull of the trigger on its grip fires it. True whenever the module isn't in use (the game's own reload too), so the firing
// never waits on it.
bool FireReady();
// weapon.txt "m320byhand 0" turns the reload by hand off: the game's own reload on an empty pull again.
bool ByHand();
void SetByHand(bool on);
// The release pushed by a pull of the trigger on its grip when it isn't ready to fire; Update takes it through Frame::releasePress (TakeReleasePress).
void PressRelease();
bool TakeReleasePress();

// weapon.txt, to tune with the user in the headset: the muzzle-up angle at which the round in the open barrel starts to slide and the friction it slides
// with ("m320tip 30 0.3"); the wrist's flick, rad/s about the gun's length (0 off, below 0 the other way round: "m320flick 9"); how much of the way shut it
// snaps the rest of the way ("m320snap 0.6"); the jab along the gun that throws the round back, m/s/s (0 off: "m320jab 25"); putting a round in
// ("m320magnet 0.025 0.08"): how near the barrel's line its nose must come at the back end to go onto it (m, 0.01-0.06), and how long its last 2 cm take (s).
void SetTip(float degrees, float friction);
void SetFlick(float radiansPerSecond);
// Whether the flick OPENS it too (default false: the user, 2026-11-06, "get rid of the flick to open"; the release opens it). The flick shutting it stays.
void SetFlickOpens(bool opens);
void SetSnapShut(float fraction);
void SetJab(float metresPerSecondSquared);
void SetMagnet(float metres, float seconds);
// A hold fitted by hand later (tools\magfit.cpp, tools\roundfit.cpp): the object (the round's bone for kPinch, kPush and kCarry, the tube's for kBarrel) in
// the left glove's bind space, and its fingers (null: the game's own as recorded). Before the first Update.
void SetHold(Hold hold, const float* objectInGlove, const float (*fingers)[12]);
// The glove that holds the object at objectView as the game's hand does (tests, and a scripted hand in the game: as pouch::GloveFor).
void GloveFor(Hold hold, int side, const float* objectView, float* glove);
// weapon.txt "m320test 7" (hands.cpp's scripted left hand, filmed with nobody in the headset): at the start of each round of its script, the barrel emptied (a live
// round in it given back to the spare ones) and sprung open, the hand free. Next Update.
void TestOpenEmpty();
// A round in the hand whose blend from the rim's pinch to the carry came out not finite since the last call (it was handed on as the pinch's): its two ends
// (the round in the glove, rows rotation | translation) and the mix, for the log. False when none.
bool TakeBadRound(float* pinched, float* carry, float& mix);

// The launcher's shape, for tests and the drawing: the turn about the hinge, the round's bone on the gun on the barrel's line slid s back, the barrel's
// back end (its mouth) on the gun, all at this angle (radians).
void HingeAt(float angle, float* out);
void RoundOnLine(float angle, float s, float* out);
void MouthAt(float angle, float* out);
extern const float kOpenMost;         // radians, the stop
extern const float kClear;            // m back along the barrel: the round's nose out of it
extern const float kRoundNoseOnBone[3], kRoundBaseOnBone[3];
extern const float kHandInGlove[3];   // the hand's middle in the left glove's bind space (pouch.cpp's): what every reach is measured from
// The hand's middle as the game's left hand pinches the round (on the round's bone: SetHold's fitted pinch moves it), as it holds the launcher's grip (on
// the gun; a right hand's is mirrored about the grip's middle, x 0.0004); where the hand takes the barrel's back end (on the tube as shut).
void PinchTarget(float* onRound);
void GripMiddle(float* onGun);
extern const float kBarrelGrab[3];

}  // namespace m320

#pragma once
// The free hands' film rounds (build\freehands.txt "test N", with nobody in the headset): what each hand
// does at a time into the round, [0] left and [1] right. hands.cpp turns each Where into a grip pose and
// drives the hands with it (the right while the stand-in stands in, the left while the real left isn't
// tracked); tools/grasptest.cpp runs the same rounds through the hold machine (grasp.h). Pure, header only.
// Rounds 4-9's places, round 13's on the body, and a step moved from one place to another, are freeplaces.cpp's (freeplaces.h).
//   test 1 (26 s) every hand-over: the right lets go (it falls), the left picks it up and fires, the right
//          takes the handguard, the left lets go (held by the right alone), the left takes the pistol grip
//          back, lets go (it falls), the right picks it up, the left takes the handguard, the right lets go
//          (held by the left alone), the right takes the pistol grip back.
//   test 2 (50 s) the throw: the right lets go moving forward and up with a spin; it lies past the 30 s a
//          magazine lies; the right picks it up.
//   test 3 (12 s) the free right glove relaxed, pointing, a fist and half closed, the left beside it doing
//          the same (its mirror twin), then the right picks the rifle up.
// Rounds 4-9: each hand mechanic by the left with the rifle in the right hand, then the hand-over (the right
// lets go, it falls, the left picks it up), the right doing the same with the left on the pistol grip, and
// the rifle back in the right hand.
//   test 4 (22 s) the charging handle: taken by the trigger and drawn 7 cm back, let go; the bolt locked
//          back; taken by the grip and drawn back again (a round chambered).
//   test 5 (32 s) the seated magazine taken, drawn down out of the well and let go (it falls); a fresh one
//          taken at the belly, brought up under the well, caught and pushed home.
//   test 6 (20 s) the bolt locked back and its release slapped by the free hand.
//   test 7 (26 s) a spent case put in the launcher's open barrel: its rim pinched, the case drawn 10 cm and
//          pushed back home, pinched again, pulled out and let go (it falls); then the right alone slaps the
//          barrel shut, takes the launcher's grip, pulls its trigger (the release: it opens), lets go of it,
//          takes a round at the belly, turns it in front of the eyes, pushes it in and slaps the barrel shut.
//   test 8 (16 s) the tactical reload: the right lets go with the left on the handguard (held by it alone),
//          takes the seated magazine 2 cm back and 1 cm down (the magazine's squeeze, not a regrip), draws it
//          out and lets it fall, pushes a fresh one from the belly home, pulls the handle by its trigger, and
//          takes the pistol grip back.
//   test 9 (14 s) X ejects the magazine; the right lets go, the left picks it up; the right takes a fresh
//          magazine at the belly and holds it; the left lets go (it falls) and the magazine falls from the
//          right a frame later; the right picks the rifle up; the left pushes a fresh one home.
// Round 10: a dropped gun of the game's (pickups.h) taken by hand, both ways.
//   test 10 (60 s) the right throws the rifle to its right; the left takes the dropped gun (the game swaps, it
//          flies in), lifts it in front of the chest and fires it, the right on its front (its trigger fires
//          nothing); the left lets go (it falls); the right takes its own gun back from the pickup that holds
//          it. Then the same the other way: the right throws it, takes the dropped gun and fires it, the left
//          on its front; the right lets go; the left takes the M416 back and lets go; the right picks the
//          lying M416 up (the one forced take: a dropped gun's take is never forced).
// Round 11: the magnet's take off the floor (grasp.h), nothing forced.
//   test 11 (35 s) the right lets go (it falls); lowered beside the knee (kLow) it takes it off the floor; lets go again; squeezes at the
//          belly and in front of the chest (nothing off the floor); the left takes it from beside its knee and lets go there; the right
//          squeezes far from anything and lowers that squeeze held (nothing), opens, squeezes afresh (the take), and keeps it.
// Round 12: the Frame's left D-pad right as X with the rifle in the left hand (input.cpp, ejectkey.h).
//   test 12 (18 s) the right lets go (it falls), the left picks it up and holds it by its pistol grip; the D-pad's right tapped
//          (ejected), tapped again with the magazine out, held 0.8 s (the game's reload), tapped, held; the left lets go (it falls), the
//          right picks it up, and the D-pad's right tapped with the rifle in the right hand (nothing).
// Round 13: holsters (holster.h; hands.cpp, build\holsters.txt) with the M416, its M320 and the SMAW (Rock and a Hard Place's kit). No trigger
// is pulled. kBodyAt is a place from the neck point, kLauncherAt one from the launcher's place (behind the neck, or the left rib cage with
// "launcher rib"); a lost hand gives no place, an unseen one keeps its place but isn't seen (freeplaces.cpp, hands.cpp).
//   test 13 (44 s) the right puts the M416 away at the right shoulder, draws it there and puts it away again; the left draws it there and
//          opens behind the neck (it goes to the right shoulder); the right draws the SMAW behind the neck (key 4), the left takes its front
//          grip and lets go, the right puts it away there and draws the M416 at the shoulder (key 1); the left squeezes behind the neck (the
//          right holds the gun: nothing); the right lets go at the shoulder at 2.5 m/s (a throw: it falls); the left squeezes behind the neck
//          (refused: the M416 lies); the right takes it up; the left pulls a spent case out of the M320 and holds it while the right opens at
//          the shoulder (not put away: it falls); the left lets the case fall, the right takes the M416 up, the left slaps the M320 shut; the
//          right lets go 6 cm outside the shoulder (it falls), takes it up, goes behind the neck with it not seen for its last 0.3 s and opens
//          (put away); draws it at the shoulder; goes behind the neck with it, lost 0.1 s before it's there, and opens 2 s later (put away);
//          sweeps through the shoulder at 0.8 m/s closing its grip at its back edge (nothing); leaves the shoulder for the neck at 0.6 m/s,
//          lost at once, squeezes 0.3 s later and is found in front (the SMAW, never the M416); puts the SMAW away behind the neck and draws
//          the M416 at the shoulder (key 1).
// Round 14: a held gun's lever by the hand at work while the other holds the gun by its front alone (the owner, 2026-11-19: "hold a gun by any barrel
// instead of the trigger grip ... and still pull whatever lever with whichever hand"), and the same mirrored for a lefty; any held gun with a lever of
// handmag's (the M249, the M240, the AKs, the M4A1, the Mk11 ...: freeplaces' Scene::lever; with none, the lever's places aren't made and the hand
// stays where it was). For a belt fed gun's whole reload first, weapon.txt "heldtest 7" (the left's, with the right on the pistol grip).
//   test 14 (24 s) the left takes the front, the right lets go (held by the left's front alone: nothing fires), takes the lever over the top by its
//          trigger, draws it as far as it goes and lets go, then from below by its grip, the same; takes its pistol grip back (a fresh squeeze, not
//          forced: the squeezes on the lever never regrip), fires with the left still on the front (a film's real round needs two hands: inputrules.h),
//          the left lets go of the front; the right lets go (it falls), the left picks it up, the right takes the front, the left lets go (held by the
//          right's front alone), works the lever the same, takes its grip back, fires with the right on the front, the right lets go of the front, the
//          left lets go (it falls), the right picks it up. (weapon.txt "heldtest 0" first: the left has no step at times, and a heldtest's scripted
//          left would start there.)
// The times things are expected (grasptest checks them within 2 frames at 72 Hz; the let-gos 3 frames
// after the grip opens):
//   1: 1.04 released by the right; 5.0 picked up by the left; 5.12 arrived; 9.04 the left lets go, held by
//      the right on the handguard; 11.0 the left regrips; 11.12 arrived; 14.04 released by the left; 16.0
//      picked up by the right; 16.12 arrived; 21.04 the right lets go, held by the left; 23.0 the right
//      regrips; 23.12 arrived. (hands.cpp's Support takes and lets go of the handguard at 7.4 and 13.0 by
//      the right, at 19.0 and 24.0 by the left.)
//   2: 1.04 released by the right; 45.0 picked up by the right; 45.12 arrived.
//   3: 0.54 released by the right; 9.0 picked up by the right; 9.12 arrived.
//   4: 9.04 released by the right; 9.5 picked up by the left; 9.62 arrived; 20.04 released by the left; 20.5
//      picked up by the right; 20.62 arrived. The handle: 2.0 the left's trigger takes it; 2.9 let go, drawn
//      70 mm; 3.8 the bolt locked back; 4.5 the left's grip takes it; 5.4 a round chambered, let go; the same
//      by the right at 11.5, 12.4, 13.3, 14.0 and 14.9.
//   5: 13.04 released by the right; 13.5 picked up by the left; 13.62 arrived; 30.04 released by the left;
//      30.5 picked up by the right; 30.62 arrived. The magazine: 2.0 the left has the seated one; ~2.70
//      drawn out; 3.5 let go (it falls); 4.5 a fresh one at the belly; ~5.83 caught under the well; ~7.48
//      pushed home; the same by the right at 15.5, ~16.20, 17.0, 18.0, ~19.33 and ~20.98.
//   6: 9.04 released by the right; 9.5 picked up by the left; 9.62 arrived; 18.04 released by the left; 18.5
//      picked up by the right; 18.62 arrived. The bolt: 0.4 locked back; ~2.3 the left slaps its release: a
//      round chambered; 10.0 locked back; ~11.3 the right slaps it.
//   7: 4.04 released by the right; 4.5 picked up by the left; 4.62 arrived; 22.54 released by the left; 23.0
//      picked up by the right; 23.12 arrived. The launcher: 0.8 a spent case in, open; 1.4 the left pinches
//      its rim; ~2.28 pushed past half way: home; 2.9 pinched again; ~3.77 pulled free; 3.9 let go (it
//      falls); the same by the right at 5.0, 5.6, ~6.48, 7.1, ~7.97 and 8.1; ~9.3 slapped shut; 9.8 the right
//      takes its grip; 10.8 its trigger: unlatched; 11.6 lets go of the grip; 12.3 a round from the belly;
//      ~15.94 its nose in; ~16.33 home; ~17.4 slapped shut, latched.
//   8: 2.04 the right lets go, held by the left on the handguard; 12.0 the right regrips; 12.12 arrived; no
//      regrip at 3.2. The rest: 1.0 the left takes the handguard; 3.2 the right has the seated magazine;
//      ~3.90 drawn out; 4.6 let go (it falls); 5.4 a fresh one at the belly; ~6.73 caught; ~8.38 pushed
//      home; 9.8 the right's trigger takes the handle; 10.7 let go; 14.0 the left lets go of the handguard.
//   9: 1.04 released by the right; 1.5 picked up by the left; 1.62 arrived; 4.54 released by the left; 5.5
//      picked up by the right; 5.62 arrived. The rest: 0.5 the magazine ejected; 2.5 the right takes a fresh
//      one at the belly; 4.55-4.58 it falls from the right hand; 6.5 the left takes a fresh one; ~7.83
//      caught; ~9.48 pushed home.
//   10 (a game that swaps 30 ms after the ask and has the new gun in hand 0.3 s after the swap): 1.04 released
//      by the right; 5.0 the left reaches (a take waits); 5.33 the game gave the left's; 5.45 arrived; the
//      left's trigger fires it from then to 14.04, also through the right's pull at 11.0; 14.04 released by
//      the left; 21.0 the right reaches; 21.33 the game gave the right's; 21.45 arrived. 31.04 released by
//      the right; 35.0 the right reaches; 35.33 the game gave the right's; 35.45 arrived; the right's trigger
//      fires it from then to 44.04, also through the left's pull at 41.0; 44.04 released by the right; 51.0
//      the left reaches; 51.33 the game gave the left's; 51.45 arrived; 55.04 released by the left; 57.0
//      picked up by the right; 57.12 arrived. (hands.cpp's Support takes and lets go of the front at 8.0 and
//      12.0 by the right, at 38.0 and 42.0 by the left.) In the game its trigger steps fire nothing unless a
//      film asks (hands.cpp).
//   11 (the gun lying at rest): 1.04 released by the right; 5.0 picked up by the right (the magnet), arrived
//      within its glide (0.12-0.25 s); 9.04 released by the right; nothing 12.0-16.0; 17.0 picked up by the
//      left, arrived within its glide; 22.04 released by the left; nothing 23.0-27.0; 27.0 picked up by the
//      right, arrived within its glide.
//   12: 1.04 released by the right; 1.5 picked up by the left; 1.62 arrived; 14.54 released by the left; 15.0 picked up by the
//      right; 15.12 arrived. The left's D-pad right held 3.0-3.2 (a tap: ejected), 4.5-4.7 (a tap, the magazine out: "take one at
//      the belly"), 6.0-6.8 (held: the game's reload from 6.6, a magazine in), 9.5-9.7 (ejected), 10.0-10.8 (held: in again), and
//      16.5-16.7 with the rifle in the right hand (nothing). Taps 0.2 s and holds 0.8 s, each 0.4 s clear of X's 0.6 s; the left
//      lets go 3.7 s after the last hold's R, past the game's reload's 2.5 s (hands.cpp's gameReload: no hand lets go then).
//   13 (holsters.txt's defaults; the game switching guns within the 1.5 s a hand keeps the coming gun): 0.94 put away by the right at the right
//      shoulder (the ACOG's place filmed to 1.7); 1.8 drawn there into the right; 2.34 put away; 3.6 drawn there into the left; 4.84 put away
//      by the left behind the neck, at the right shoulder; 5.9 key 4 by the right behind the neck, the SMAW in its hand by 7.4; 8.0 the left
//      takes its front grip, 9.0 lets go; 10.74 put away behind the neck; 11.8 key 1 by the right at the shoulder, the M416 in its hand by
//      13.3; 13.5 the left behind the neck: nothing, not felt; 14.94 let go at the shoulder at 2.5 m/s: thrown; 15.8 the left behind the
//      neck: refused at 16.2 (it lies; once the take's 0.4 s are over), two pulses; 16.6 taken up by the right; 17.0 a spent case in the
//      M320's barrel, 17.6 its rim pinched by the left, about 18.47 out; 20.34 the right opens at the shoulder: not put away (the left
//      holds the case), it falls, two pulses; 21.04 the case let go of; 21.6 taken up; 22.4-22.6 the M320 slapped shut by the left; 24.34
//      let go 6 cm outside the shoulder: it falls; 25.5 taken up; 27.0 not seen, 27.64 put away behind the neck by the hand not seen (at the
//      right shoulder), seen again 27.8; 28.8 drawn at the shoulder; 30.0 lost, 32.04 put away by its opening (2 s lost), found 32.3; 33.35
//      the grip closed at the shoulder's back edge going 0.8 m/s: nothing, nor held on to behind the neck; 35.45 lost leaving the shoulder,
//      35.75 squeezed: key 4, 36.35 found in front: the SMAW in the right once it has come (by 37.25); 38.94 put away behind the neck; 40.0
//      key 1 at the shoulder, the M416 in the right by 41.5.
//   14: 1.5 the left takes the front (hands.cpp's LauncherSupport); 2.04 the right lets go, held by the left's front alone; 3.2 the right's trigger
//      takes the lever, 4.4 let go, drawn all its way; 5.3 its grip takes it from below, 6.5 let go; 7.5 the right takes its pistol grip back (not
//      forced), 7.62 arrived; 9.0 the right's trigger fires (two hands on the gun); 9.5 the left lets go of the front; 10.04 the right lets go (it
//      falls); 10.5 picked up by the left, 10.62 arrived; 11.5 the right takes the front; 12.04 the left lets go, held by the right's front alone;
//      13.2-16.5 the left's lever as the right's; 17.5 the left takes its pistol grip back, 17.62 arrived; 19.0 the left fires (two hands); 19.5
//      the right lets go of the front; 20.04 the left lets go (it falls); 21.0 picked up by the right, 21.12 arrived.

#include <cmath>
#include <cstdint>

namespace freetest {

enum class Where : uint8_t {
    kNone,     // not moved by the round
    kStandIn,  // the stand-in's right grip
    kMirror,   // that grip mirrored about the head (head space p (-x, y, z), q (x, -y, -z, w))
    kGuard,    // the glove at the game's hold on the handguard of the gun as drawn (its side's), the controller
               // pose behind it through the line-up offset; on any gun but the M416, the hand where the game's
               // rests on its front, the palm's middle on the profile's support for that side
    kGrip,     // the grip point at its side's pistol-grip point of the gun as drawn, keeping its last turn
    kFront,    // 30 cm out, 12 cm to its side, 15 cm down (head space), the stand-in grip's turn, mirrored for the left
    // Rounds 4-9 (freeplaces.cpp places these for either side, the left's tables mirrored for the right):
    kHandle,    // the glove's V on the charging handle (reload::kOnLever; the right's handpose::Mirror of it), drawn back along the rifle by along (m)
    kMagazine,  // the glove holding the seated magazine (pouch::GloveFor of the rifle as drawn, its side's), drawn down the well by along (m)
    kWell,      // the glove holding a magazine on the well's line, its top along (m, below 0) from home
    kBelly,     // the grip point at the belly (ammozone::kBelly from the body frame), turned as its side's stand-in grip
    kRelease,   // the glove (its side's stand-in turn; the right's rolled half a turn about the rifle's length), its palm point from (0.10, -0.03, 0) to
                // (-0.02, -0.03, 0) off the bolt release by along 0-1
    kM320Grip,  // the glove at the game's own hold on the launcher's grip (Scene::m320Grip; the right's mirrored)
    kFace,      // the glove's palm point in front of the eyes (head space (-0.04 left, +0.04 right, -0.06, -0.42)), its side's stand-in turn rolled
                // about the view's forward by along degrees (the right's the other way)
    kMouth,     // the glove holding its round (Scene::roundInGlove[side]) on the open barrel's line, along (m) back from home
    kSlap,      // the glove (kRelease's turn), its slap point (m320::kSlapPointInGlove, its side's) from (0.135, -0.036, 0.60) to (-0.065, -0.036, 0.60) on the rifle by along 0-1
    kRim,       // the glove as drawn holding the round in the open barrel by its rim (m320::DrawnGloveFor kPinch, its side's), along (m) back along the line
    kPickup,    // the grip point at the middle of the nearest dropped gun's box (pickups.h: this frame's first candidate), turned as it last was; worked
                // out at the step's first frame, then held relative to the head; none near: where it was (hands.cpp's RunFreeTest, never freeplaces)
    kLow,       // the grip point lowered beside the knee, as a seated player reaches for the floor: 0.68 m under the neck point, 0.10 m to its side
                // and 0.20 m forward (the body frame), turned as its side's stand-in grip (freeplaces.cpp)
    // Round 13 (freeplaces.cpp):
    kBodyAt,     // the grip point at body (m) from the neck point in the body's axes (right, up, forward: pouch::BodyFrame), turned as its side's
                 // stand-in grip; the same place for either hand (not mirrored). A from place of this kind or the next is at fromBody
    kLauncherAt, // the same from the middle of the launcher's holster place (freeplaces' Scene: behind the neck, or the left rib cage with
                 // holsters.txt "launcher rib"), so the round's launcher steps go where holsters.txt puts launchers
    // Round 14 (freeplaces.cpp): a held gun's lever (handmag.h; freeplaces' Scene::lever, none: not now), for either hand.
    kLeverOver,  // the glove's fist point (handmag's, its side's) on the lever's knob, drawn back along the gun by along of its travel (0-1, past 1
                 // as far as it goes); the palm down, the fingers across the gun's top to its other side, a little forward: over the top
    kLeverUnder, // the same from below it: the palm up
};
// kGuard and kGrip are worked out at a step's first frame and then held head-relative.

struct Step {
    Where where = Where::kNone;
    float squeeze = 0.0f, trigger = 0.0f, thumb = 0.0f;  // 0-1; thumb 1: on the stick
    bool forceTake = false;                               // taken whatever the squeeze's history or the distance
    bool sensed = false;
    float velocity[3] = {}, spin[3] = {};  // head axes, m/s and rad/s (the throw)
    // Rounds 4-9 (freeplaces.cpp):
    float along = 0.0f;               // the place's own measure (above)
    float shift[3] = {};              // the gun-relative places (where and from) moved this far on the rifle (its model space, m)
    Where from = Where::kNone;        // moved from this place (at fromAlong) to where by blend (0-1, eased): kFront or a kind after it
    float fromAlong = 0.0f, blend = 1.0f;
    bool lockBack = false;            // at this step's first frame: reload::TestLockBack
    bool m320Open = false;            // ...: m320::TestOpenEmpty
    bool m320Case = false;            // ...: m320::TestOpenWithCase
    bool magTap = false;              // ...: input::MagazineTap(true)
    bool dpadRight = false;           // the left controller's D-pad right held, every frame of the step (the left's step only): hands::ScriptEject
    // Round 13:
    float body[3] = {}, fromBody[3] = {};  // kBodyAt, kLauncherAt: where (m, the body's axes), and the from place's when it's one of them
    bool lost = false;                // the hand's controller gives no place: hands.cpp's HandPoseOf answers false (no 0.5 s hold), holster.h reads it as
                                      // not seen (Seen::kNone); its grip and trigger are still read
    bool unseen = false;              // its place kept but not seen: HandPoseOf answers with the step's place, holster.h reads it as the runtime's own
                                      // guess (Seen::kGuessed, that place the guess)
};

constexpr float kRound[15] = {0.0f, 26.0f, 50.0f, 12.0f, 22.0f, 32.0f, 20.0f, 26.0f, 16.0f, 14.0f, 60.0f, 35.0f, 18.0f, 44.0f, 24.0f};  // each test's round (seconds); 0: no such test
// The launcher's round slid this far back along its barrel has its nose out (m320.cpp's kClear; grasptest checks they agree).
constexpr float kRoundClear = 0.134f;

// Round 13's places (m, the body's axes: right, up, forward) and its ways in, each inside or outside holster.h's default places as it says:
// freeplaces.cpp checks them against holster.h's own when it builds.
namespace round13 {
// From the neck point (kBodyAt):
constexpr float kShoulder[3] = {0.20f, -0.025f, -0.03f};        // the right shoulder's middle
constexpr float kOutside[3] = {0.33f, -0.025f, -0.03f};         // 6 cm out to the right past the right shoulder's edge
constexpr float kLeftRest[3] = {-0.25f, -0.20f, 0.25f};         // the left hand's rest, in front of the left chest: in no place, over the magnet's gate
constexpr float kFrontOfShoulder[3] = {0.20f, -0.025f, 0.12f};  // the sweep through the right shoulder: from in front of it ...
constexpr float kBehindShoulder[3] = {0.20f, -0.025f, -0.18f};  // ... to behind it, at kSweep m/s, ...
constexpr float kSweep = 0.8f;
constexpr float kSweepFrom = 33.1f;
constexpr float kSweepCloses = -0.08f;                           // ... its grip closing as it passes this forward (the shoulder's back edge is at -0.10)
// From the launcher's place's middle (kLauncherAt):
constexpr float kLauncher[3] = {0.0f, 0.0f, 0.0f};              // its middle
constexpr float kOverShoulder[3] = {0.20f, 0.05f, 0.115f};      // over the right shoulder: the way in (24 cm)
constexpr float kInShoulder[3] = {0.18f, -0.085f, 0.115f};      // inside the right shoulder when it's the back of the neck's (23 cm)
// The ways in at an even speed, and when the hand stops being seen on them (s into the round):
constexpr float kUnseenIn[3] = {26.9f, 27.3f, 27.0f};           // over the shoulder to the middle, unseen from (step 15's put-away)
constexpr float kLostIn[3] = {29.7f, 30.1f, 30.0f};             // the same, lost from (the put-away by a lost hand)
constexpr float kLeaveIn[4] = {35.4f, 35.78f, 35.45f, 35.75f};  // from inside the shoulder to the middle, lost from, squeezed at (the lost take)
}  // namespace round13

inline void At(int test, float t, Step out[2]);  // t: s into the round (hands.cpp takes fmod)

inline void At(int test, float t, Step out[2]) {
    out[0] = Step{};
    out[1] = Step{};
    if (!(t >= 0.0f))  // before the round or not a number: nothing moves
        return;
    Step& l = out[0];
    Step& r = out[1];
    const auto in = [t](float from, float to) { return t >= from && t < to; };
    const auto set = [](Step& s, Where where, float squeeze, bool forceTake = false) {
        s.where = where;
        s.squeeze = squeeze;
        s.forceTake = forceTake;
    };
    // Rounds 4-9: a place with its own measure, the grip and the trigger; and moved there from another place over [a, b).
    const auto put = [](Step& s, Where where, float along, float squeeze = 0.0f, float trigger = 0.0f) {
        s.where = where;
        s.along = along;
        s.squeeze = squeeze;
        s.trigger = trigger;
    };
    const auto glide = [t](Step& s, Where from, float fromAlong, float a, float b) {
        s.from = from;
        s.fromAlong = fromAlong;
        const float u = (t - a) / (b - a);
        s.blend = u > 0.0f ? (u < 1.0f ? u : 1.0f) : 0.0f;
    };
    if (test == 1) {
        if (in(0.0f, 1.0f))
            set(r, Where::kStandIn, 1.0f);
        else if (in(1.0f, 7.0f))
            set(r, Where::kStandIn, 0.0f);  // lets go: it falls
        else if (in(7.0f, 7.4f))
            set(r, Where::kGuard, 0.0f);
        else if (in(7.4f, 13.0f))
            set(r, Where::kGuard, 1.0f);  // on the handguard of the rifle in the left hand
        else if (in(13.0f, 16.0f))
            set(r, Where::kStandIn, 0.0f);
        else if (in(16.0f, 16.2f))
            set(r, Where::kStandIn, 1.0f, true);  // picks it up
        else if (in(16.2f, 21.0f))
            set(r, Where::kStandIn, 1.0f);
        else if (in(21.0f, 23.0f))
            set(r, Where::kStandIn, 0.0f);  // lets go: held by the left on the handguard
        else if (in(23.0f, 23.2f))
            set(r, Where::kGrip, 1.0f, true);  // takes the pistol grip back
        else if (in(23.2f, 26.0f))
            set(r, Where::kGrip, 1.0f);
        if (in(5.0f, 5.2f))
            set(l, Where::kMirror, 1.0f, true);  // picks it up
        else if (in(5.2f, 9.0f)) {
            set(l, Where::kMirror, 1.0f);
            if (in(5.5f, 6.5f))
                l.trigger = 1.0f - 2.0f * std::fabs(t - 6.0f);  // the left trigger pulled and let go: it fires
        } else if (in(9.0f, 11.0f))
            set(l, Where::kMirror, 0.0f);  // lets go: held by the right on the handguard
        else if (in(11.0f, 11.2f))
            set(l, Where::kGrip, 1.0f, true);  // takes the pistol grip back
        else if (in(11.2f, 14.0f))
            set(l, Where::kGrip, 1.0f);
        else if (in(14.0f, 16.0f))
            set(l, Where::kGrip, 0.0f);  // lets go: it falls
        else if (in(18.0f, 19.0f))
            set(l, Where::kGuard, 0.0f);
        else if (in(19.0f, 24.0f))
            set(l, Where::kGuard, 1.0f);  // on the handguard of the rifle in the right hand
    } else if (test == 2) {
        if (in(0.0f, 1.0f))
            set(r, Where::kStandIn, 1.0f);
        else if (in(1.0f, 1.15f)) {
            set(r, Where::kStandIn, 0.0f);  // lets go, thrown forward and up with a spin
            r.sensed = true;
            r.velocity[0] = 0.3f, r.velocity[1] = 1.5f, r.velocity[2] = -3.0f;
            r.spin[0] = 6.0f;
        } else if (in(1.15f, 45.0f))
            set(r, Where::kStandIn, 0.0f);
        else if (in(45.0f, 45.2f))
            set(r, Where::kStandIn, 1.0f, true);  // picks it up
        else if (in(45.2f, 50.0f))
            set(r, Where::kStandIn, 1.0f);
    } else if (test == 3) {
        if (in(0.0f, 0.5f))
            set(r, Where::kStandIn, 1.0f);
        else if (in(0.5f, 1.0f))
            set(r, Where::kStandIn, 0.0f);  // lets go: it falls
        else if (in(1.0f, 3.0f))
            set(r, Where::kFront, 0.0f);  // relaxed
        else if (in(3.0f, 5.0f)) {
            set(r, Where::kFront, 1.0f);  // a point
            r.thumb = 1.0f;
        } else if (in(5.0f, 7.0f)) {
            set(r, Where::kFront, 1.0f);  // a fist
            r.trigger = 1.0f;
            r.thumb = 1.0f;
        } else if (in(7.0f, 9.0f)) {
            set(r, Where::kFront, 0.5f);  // half closed
            r.trigger = 0.5f;
        } else if (in(9.0f, 9.2f))
            set(r, Where::kStandIn, 1.0f, true);  // picks it up
        else if (in(9.2f, 12.0f))
            set(r, Where::kStandIn, 1.0f);
        if (in(1.0f, 9.0f)) {  // the left beside it with the right's fingers
            l.where = Where::kFront;
            l.squeeze = r.squeeze;
            l.trigger = r.trigger;
            l.thumb = r.thumb;
        }
    } else if (test == 4) {
        // The charging handle. The right holds the rifle while the left works the handle, lets go (the left picks it up), works it the same, picks it up.
        if (in(0.0f, 9.0f))
            set(r, Where::kStandIn, 1.0f);
        else if (in(9.0f, 9.5f))
            set(r, Where::kStandIn, 0.0f);  // lets go: it falls
        else if (in(9.5f, 11.0f))
            put(r, Where::kFront, 0.0f);
        else if (in(11.0f, 11.5f)) {
            put(r, Where::kHandle, 0.0f);
            glide(r, Where::kFront, 0.0f, 11.0f, 11.5f);
        } else if (in(11.5f, 11.7f))
            put(r, Where::kHandle, 0.0f, 0.0f, 1.0f);  // its trigger takes the handle
        else if (in(11.7f, 12.4f))
            put(r, Where::kHandle, 0.1f * (t - 11.7f), 0.0f, 1.0f);  // drawn 7 cm back
        else if (in(12.4f, 12.6f))
            put(r, Where::kHandle, 0.07f);  // let go: it springs home
        else if (in(12.6f, 13.3f)) {
            put(r, Where::kFront, 0.0f);
            glide(r, Where::kHandle, 0.07f, 12.6f, 13.3f);
        } else if (in(13.3f, 13.4f)) {
            put(r, Where::kFront, 0.0f);
            r.lockBack = true;  // the bolt locked back
        } else if (in(13.4f, 14.0f)) {
            put(r, Where::kHandle, 0.0f);
            glide(r, Where::kFront, 0.0f, 13.4f, 14.0f);
        } else if (in(14.0f, 14.2f))
            put(r, Where::kHandle, 0.0f, 1.0f);  // its grip takes it
        else if (in(14.2f, 14.9f))
            put(r, Where::kHandle, 0.1f * (t - 14.2f), 1.0f);  // drawn back: a round chambered as it's let go
        else if (in(14.9f, 15.1f))
            put(r, Where::kHandle, 0.07f);
        else if (in(15.1f, 20.5f)) {
            put(r, Where::kFront, 0.0f);
            glide(r, Where::kHandle, 0.07f, 15.1f, 15.6f);
        } else if (in(20.5f, 20.7f))
            set(r, Where::kStandIn, 1.0f, true);  // picks it up
        else if (in(20.7f, 22.0f))
            set(r, Where::kStandIn, 1.0f);
        if (in(0.5f, 1.5f))
            put(l, Where::kFront, 0.0f);
        else if (in(1.5f, 2.0f)) {
            put(l, Where::kHandle, 0.0f);
            glide(l, Where::kFront, 0.0f, 1.5f, 2.0f);
        } else if (in(2.0f, 2.2f))
            put(l, Where::kHandle, 0.0f, 0.0f, 1.0f);
        else if (in(2.2f, 2.9f))
            put(l, Where::kHandle, 0.1f * (t - 2.2f), 0.0f, 1.0f);
        else if (in(2.9f, 3.1f))
            put(l, Where::kHandle, 0.07f);
        else if (in(3.1f, 3.8f)) {
            put(l, Where::kFront, 0.0f);
            glide(l, Where::kHandle, 0.07f, 3.1f, 3.8f);
        } else if (in(3.8f, 3.9f)) {
            put(l, Where::kFront, 0.0f);
            l.lockBack = true;
        } else if (in(3.9f, 4.5f)) {
            put(l, Where::kHandle, 0.0f);
            glide(l, Where::kFront, 0.0f, 3.9f, 4.5f);
        } else if (in(4.5f, 4.7f))
            put(l, Where::kHandle, 0.0f, 1.0f);
        else if (in(4.7f, 5.4f))
            put(l, Where::kHandle, 0.1f * (t - 4.7f), 1.0f);
        else if (in(5.4f, 5.6f))
            put(l, Where::kHandle, 0.07f);
        else if (in(5.6f, 8.0f)) {
            put(l, Where::kFront, 0.0f);
            glide(l, Where::kHandle, 0.07f, 5.6f, 8.0f);
        } else if (in(9.5f, 9.7f))
            set(l, Where::kMirror, 1.0f, true);  // picks it up
        else if (in(9.7f, 20.0f))
            set(l, Where::kMirror, 1.0f);
        else if (in(20.0f, 20.5f))
            set(l, Where::kMirror, 0.0f);  // lets go: it falls
    } else if (test == 5) {
        // The magazine out by hand, a fresh one at the belly, caught at the well and pushed home: the left's, then the right's.
        if (in(0.0f, 13.0f))
            set(r, Where::kStandIn, 1.0f);
        else if (in(13.0f, 13.5f))
            set(r, Where::kStandIn, 0.0f);  // lets go: it falls
        else if (in(13.5f, 15.0f))
            put(r, Where::kFront, 0.0f);
        else if (in(15.0f, 15.5f)) {
            put(r, Where::kMagazine, 0.0f);
            glide(r, Where::kFront, 0.0f, 15.0f, 15.5f);
        } else if (in(15.5f, 15.7f))
            put(r, Where::kMagazine, 0.0f, 1.0f);  // takes hold of the seated magazine
        else if (in(15.7f, 16.5f))
            put(r, Where::kMagazine, 0.1875f * (t - 15.7f), 1.0f);  // draws it 15 cm down: out at the mouth
        else if (in(16.5f, 17.0f)) {
            put(r, Where::kFront, 0.0f, 1.0f);
            glide(r, Where::kMagazine, 0.15f, 16.5f, 17.0f);
        } else if (in(17.0f, 17.2f))
            put(r, Where::kFront, 0.0f);  // lets go of it: it falls
        else if (in(17.2f, 18.0f)) {
            put(r, Where::kBelly, 0.0f);
            glide(r, Where::kFront, 0.0f, 17.2f, 18.0f);
        } else if (in(18.0f, 18.2f))
            put(r, Where::kBelly, 0.0f, 1.0f);  // a fresh one
        else if (in(18.2f, 19.0f)) {
            put(r, Where::kWell, -0.20f, 1.0f);
            glide(r, Where::kBelly, 0.0f, 18.2f, 19.0f);
        } else if (in(19.0f, 21.0f))
            put(r, Where::kWell, -0.20f + 0.1f * (t - 19.0f), 1.0f);  // up the well: caught, pushed home
        else if (in(21.0f, 21.2f))
            put(r, Where::kWell, 0.0f);
        else if (in(21.2f, 30.5f)) {
            put(r, Where::kFront, 0.0f);
            glide(r, Where::kWell, 0.0f, 21.2f, 21.7f);
        } else if (in(30.5f, 30.7f))
            set(r, Where::kStandIn, 1.0f, true);  // picks it up
        else if (in(30.7f, 32.0f))
            set(r, Where::kStandIn, 1.0f);
        if (in(0.5f, 1.5f))
            put(l, Where::kFront, 0.0f);
        else if (in(1.5f, 2.0f)) {
            put(l, Where::kMagazine, 0.0f);
            glide(l, Where::kFront, 0.0f, 1.5f, 2.0f);
        } else if (in(2.0f, 2.2f))
            put(l, Where::kMagazine, 0.0f, 1.0f);
        else if (in(2.2f, 3.0f))
            put(l, Where::kMagazine, 0.1875f * (t - 2.2f), 1.0f);
        else if (in(3.0f, 3.5f)) {
            put(l, Where::kFront, 0.0f, 1.0f);
            glide(l, Where::kMagazine, 0.15f, 3.0f, 3.5f);
        } else if (in(3.5f, 3.7f))
            put(l, Where::kFront, 0.0f);
        else if (in(3.7f, 4.5f)) {
            put(l, Where::kBelly, 0.0f);
            glide(l, Where::kFront, 0.0f, 3.7f, 4.5f);
        } else if (in(4.5f, 4.7f))
            put(l, Where::kBelly, 0.0f, 1.0f);
        else if (in(4.7f, 5.5f)) {
            put(l, Where::kWell, -0.20f, 1.0f);
            glide(l, Where::kBelly, 0.0f, 4.7f, 5.5f);
        } else if (in(5.5f, 7.5f))
            put(l, Where::kWell, -0.20f + 0.1f * (t - 5.5f), 1.0f);
        else if (in(7.5f, 7.7f))
            put(l, Where::kWell, 0.0f);
        else if (in(7.7f, 12.0f)) {
            put(l, Where::kFront, 0.0f);
            glide(l, Where::kWell, 0.0f, 7.7f, 8.2f);
        } else if (in(13.5f, 13.7f))
            set(l, Where::kMirror, 1.0f, true);  // picks it up
        else if (in(13.7f, 30.0f))
            set(l, Where::kMirror, 1.0f);
        else if (in(30.0f, 30.5f))
            set(l, Where::kMirror, 0.0f);  // lets go: it falls
    } else if (test == 6) {
        // The bolt locked back and its release slapped: by the left, then by the right. The right goes round the rifle in the left hand, over its
        // top (its kFront is on the rifle's right, the release on its left, and its glove can't go through the gun): up on its own side, across
        // 30 cm above the release's line (25 cm brushed the rail), down the left side; and back the same way.
        constexpr float kSide = -0.27f, kUp = 0.30f;  // kRelease's shift (the rifle's model space): to its right side, and up
        if (in(0.0f, 9.0f))
            set(r, Where::kStandIn, 1.0f);
        else if (in(9.0f, 9.5f))
            set(r, Where::kStandIn, 0.0f);  // lets go: it falls
        else if (in(9.5f, 10.0f))
            put(r, Where::kFront, 0.0f);
        else if (in(10.0f, 10.1f)) {
            put(r, Where::kFront, 0.0f);
            r.lockBack = true;
        } else if (in(10.1f, 10.4f)) {
            put(r, Where::kRelease, 0.0f);  // up on its own side
            glide(r, Where::kFront, 0.0f, 10.1f, 10.4f);
            r.shift[0] = kSide, r.shift[1] = kUp;
        } else if (in(10.4f, 10.7f)) {
            put(r, Where::kRelease, 0.0f);  // across over the rifle
            r.shift[0] = kSide * (10.7f - t) / 0.3f, r.shift[1] = kUp;
        } else if (in(10.7f, 11.0f)) {
            put(r, Where::kRelease, 0.0f);  // down its left side
            r.shift[1] = kUp * (11.0f - t) / 0.3f;
        } else if (in(11.0f, 11.6f))
            put(r, Where::kRelease, (t - 11.0f) / 0.6f);  // onto the release: slapped
        else if (in(11.6f, 12.5f))
            put(r, Where::kRelease, 1.0f);
        else if (in(12.5f, 12.8f))
            put(r, Where::kRelease, 1.0f - (t - 12.5f) / 0.3f);  // off it
        else if (in(12.8f, 13.1f)) {
            put(r, Where::kRelease, 0.0f);  // up its left side
            r.shift[1] = kUp * (t - 12.8f) / 0.3f;
        } else if (in(13.1f, 13.4f)) {
            put(r, Where::kRelease, 0.0f);  // back across
            r.shift[0] = kSide * (t - 13.1f) / 0.3f, r.shift[1] = kUp;
        } else if (in(13.4f, 18.5f)) {
            put(r, Where::kFront, 0.0f);  // down to the front on its own side
            glide(r, Where::kRelease, 0.0f, 13.4f, 13.9f);
            r.shift[0] = kSide, r.shift[1] = kUp;
        } else if (in(18.5f, 18.7f))
            set(r, Where::kStandIn, 1.0f, true);  // picks it up
        else if (in(18.7f, 20.0f))
            set(r, Where::kStandIn, 1.0f);
        if (in(0.4f, 0.5f)) {
            put(l, Where::kFront, 0.0f);
            l.lockBack = true;
        } else if (in(0.5f, 1.6f))
            put(l, Where::kFront, 0.0f);
        else if (in(1.6f, 2.0f)) {
            put(l, Where::kRelease, 0.0f);
            glide(l, Where::kFront, 0.0f, 1.6f, 2.0f);
        } else if (in(2.0f, 2.6f))
            put(l, Where::kRelease, (t - 2.0f) / 0.6f);
        else if (in(2.6f, 3.5f))
            put(l, Where::kRelease, 1.0f);
        else if (in(3.5f, 8.0f)) {
            put(l, Where::kFront, 0.0f);
            glide(l, Where::kRelease, 1.0f, 3.5f, 8.0f);
        } else if (in(9.5f, 9.7f))
            set(l, Where::kMirror, 1.0f, true);  // picks it up
        else if (in(9.7f, 18.0f))
            set(l, Where::kMirror, 1.0f);
        else if (in(18.0f, 18.5f))
            set(l, Where::kMirror, 0.0f);  // lets go: it falls
    } else if (test == 7) {
        // The launcher by hand: the left's rim twin, then the right's whole reload with the left on the pistol grip.
        if (in(0.0f, 4.0f))
            set(r, Where::kStandIn, 1.0f);
        else if (in(4.0f, 4.5f))
            set(r, Where::kStandIn, 0.0f);  // lets go: it falls
        else if (in(4.5f, 5.0f))
            put(r, Where::kFront, 0.0f);
        else if (in(5.0f, 5.1f)) {
            put(r, Where::kFront, 0.0f);
            r.m320Case = true;  // a spent case in its open barrel
        } else if (in(5.1f, 5.6f)) {
            put(r, Where::kRim, 0.0f);
            glide(r, Where::kFront, 0.0f, 5.1f, 5.6f);
        } else if (in(5.6f, 5.8f))
            put(r, Where::kRim, 0.0f, 1.0f);  // pinches the rim
        else if (in(5.8f, 6.3f))
            put(r, Where::kRim, 0.2f * (t - 5.8f), 1.0f);  // out to 10 cm
        else if (in(6.3f, 6.6f))
            put(r, Where::kRim, 0.10f - 0.2f * (t - 6.3f), 1.0f);  // pushed back: the push hold, home
        else if (in(6.6f, 6.8f))
            put(r, Where::kRim, 0.04f);
        else if (in(6.8f, 7.1f)) {
            put(r, Where::kRim, 0.0f);
            glide(r, Where::kRim, 0.04f, 6.8f, 7.1f);
        } else if (in(7.1f, 7.3f))
            put(r, Where::kRim, 0.0f, 1.0f);  // pinches it again
        else if (in(7.3f, 8.1f))
            put(r, Where::kRim, 0.2f * (t - 7.3f), 1.0f);  // pulled out
        else if (in(8.1f, 8.3f))
            put(r, Where::kRim, 0.16f);  // let go: it falls by the right's own motion
        else if (in(8.3f, 8.8f)) {
            put(r, Where::kFront, 0.0f);
            glide(r, Where::kRim, 0.16f, 8.3f, 8.8f);
        } else if (in(8.8f, 9.2f)) {
            put(r, Where::kSlap, 0.0f);
            glide(r, Where::kFront, 0.0f, 8.8f, 9.2f);
        } else if (in(9.2f, 9.4f))
            put(r, Where::kSlap, (t - 9.2f) / 0.2f);  // slapped shut, empty
        else if (in(9.4f, 9.8f)) {
            put(r, Where::kM320Grip, 0.0f);
            glide(r, Where::kSlap, 1.0f, 9.4f, 9.8f);
        } else if (in(9.8f, 10.8f))
            put(r, Where::kM320Grip, 0.0f, 1.0f);  // takes its grip
        else if (in(10.8f, 11.0f))
            put(r, Where::kM320Grip, 0.0f, 1.0f, 1.0f);  // its trigger: the release
        else if (in(11.0f, 11.6f))
            put(r, Where::kM320Grip, 0.0f, 1.0f);
        else if (in(11.6f, 11.8f))
            put(r, Where::kM320Grip, 0.0f);  // lets go of the grip
        else if (in(11.8f, 12.3f)) {
            put(r, Where::kBelly, 0.0f);
            glide(r, Where::kM320Grip, 0.0f, 11.8f, 12.3f);
        } else if (in(12.3f, 12.9f))
            put(r, Where::kBelly, 0.0f, 1.0f);  // a round
        else if (in(12.9f, 13.4f)) {
            put(r, Where::kFace, 0.0f, 1.0f);
            glide(r, Where::kBelly, 0.0f, 12.9f, 13.4f);
        } else if (in(13.4f, 15.4f))
            put(r, Where::kFace, 50.0f * std::sin(3.14159265f * (t - 13.4f)), 1.0f);  // the round turned in front of the eyes
        else if (in(15.4f, 15.9f)) {
            put(r, Where::kMouth, kRoundClear + 0.04f, 1.0f);
            glide(r, Where::kFace, 0.0f, 15.4f, 15.9f);
        } else if (in(15.9f, 16.9f))
            put(r, Where::kMouth, kRoundClear + 0.04f - 0.25f * (t - 15.9f), 1.0f);  // its nose in, pushed home
        else if (in(16.9f, 17.3f)) {
            put(r, Where::kSlap, 0.0f);
            glide(r, Where::kMouth, kRoundClear - 0.21f, 16.9f, 17.3f);
        } else if (in(17.3f, 17.5f))
            put(r, Where::kSlap, (t - 17.3f) / 0.2f);  // slapped shut
        else if (in(17.5f, 23.0f)) {
            put(r, Where::kFront, 0.0f);
            glide(r, Where::kSlap, 1.0f, 17.5f, 23.0f);
        } else if (in(23.0f, 23.2f))
            set(r, Where::kStandIn, 1.0f, true);  // picks it up
        else if (in(23.2f, 26.0f))
            set(r, Where::kStandIn, 1.0f);
        if (in(0.3f, 0.8f))
            put(l, Where::kFront, 0.0f);
        else if (in(0.8f, 0.9f)) {
            put(l, Where::kFront, 0.0f);
            l.m320Case = true;
        } else if (in(0.9f, 1.4f)) {
            put(l, Where::kRim, 0.0f);
            glide(l, Where::kFront, 0.0f, 0.9f, 1.4f);
        } else if (in(1.4f, 1.6f))
            put(l, Where::kRim, 0.0f, 1.0f);
        else if (in(1.6f, 2.1f))
            put(l, Where::kRim, 0.2f * (t - 1.6f), 1.0f);
        else if (in(2.1f, 2.4f))
            put(l, Where::kRim, 0.10f - 0.2f * (t - 2.1f), 1.0f);
        else if (in(2.4f, 2.6f))
            put(l, Where::kRim, 0.04f);
        else if (in(2.6f, 2.9f)) {
            put(l, Where::kRim, 0.0f);
            glide(l, Where::kRim, 0.04f, 2.6f, 2.9f);
        } else if (in(2.9f, 3.1f))
            put(l, Where::kRim, 0.0f, 1.0f);
        else if (in(3.1f, 3.9f))
            put(l, Where::kRim, 0.2f * (t - 3.1f), 1.0f);
        else if (in(3.9f, 4.1f))
            put(l, Where::kRim, 0.16f);
        else if (in(4.1f, 4.5f)) {
            put(l, Where::kFront, 0.0f);
            glide(l, Where::kRim, 0.16f, 4.1f, 4.5f);
        } else if (in(4.5f, 4.7f))
            set(l, Where::kMirror, 1.0f, true);  // picks it up
        else if (in(4.7f, 22.5f))
            set(l, Where::kMirror, 1.0f);
        else if (in(22.5f, 23.0f))
            set(l, Where::kMirror, 0.0f);  // lets go: it falls
    } else if (test == 8) {
        // The tactical reload: the right works the rifle while the left holds it by its handguard alone.
        const float back[3] = {0.0f, -0.01f, -0.02f};  // the seated magazine taken 2 cm back and 1 cm down (where a regrip would take it)
        if (in(0.0f, 2.0f))
            set(r, Where::kStandIn, 1.0f);
        else if (in(2.0f, 2.5f))
            set(r, Where::kStandIn, 0.0f);  // lets go: held by the left alone
        else if (in(2.5f, 2.8f))
            put(r, Where::kFront, 0.0f);
        else if (in(2.8f, 3.2f)) {
            put(r, Where::kMagazine, 0.0f);
            glide(r, Where::kFront, 0.0f, 2.8f, 3.2f);
        } else if (in(3.2f, 3.4f))
            put(r, Where::kMagazine, 0.0f, 1.0f);  // the magazine's squeeze: no regrip
        else if (in(3.4f, 4.2f))
            put(r, Where::kMagazine, 0.1875f * (t - 3.4f), 1.0f);
        else if (in(4.2f, 4.6f)) {
            put(r, Where::kFront, 0.0f, 1.0f);
            glide(r, Where::kMagazine, 0.15f, 4.2f, 4.6f);
        } else if (in(4.6f, 4.8f))
            put(r, Where::kFront, 0.0f);  // lets go of it: it falls
        else if (in(4.8f, 5.4f)) {
            put(r, Where::kBelly, 0.0f);
            glide(r, Where::kFront, 0.0f, 4.8f, 5.4f);
        } else if (in(5.4f, 5.6f))
            put(r, Where::kBelly, 0.0f, 1.0f);  // a fresh one
        else if (in(5.6f, 6.4f)) {
            put(r, Where::kWell, -0.20f, 1.0f);
            glide(r, Where::kBelly, 0.0f, 5.6f, 6.4f);
        } else if (in(6.4f, 8.4f))
            put(r, Where::kWell, -0.20f + 0.1f * (t - 6.4f), 1.0f);  // caught, pushed home
        else if (in(8.4f, 8.6f))
            put(r, Where::kWell, 0.0f);
        else if (in(8.6f, 9.4f)) {
            put(r, Where::kFront, 0.0f);
            glide(r, Where::kWell, 0.0f, 8.6f, 9.4f);
        } else if (in(9.4f, 9.8f)) {
            put(r, Where::kHandle, 0.0f);
            glide(r, Where::kFront, 0.0f, 9.4f, 9.8f);
        } else if (in(9.8f, 10.0f))
            put(r, Where::kHandle, 0.0f, 0.0f, 1.0f);  // its trigger takes the handle
        else if (in(10.0f, 10.7f))
            put(r, Where::kHandle, 0.1f * (t - 10.0f), 0.0f, 1.0f);
        else if (in(10.7f, 10.9f))
            put(r, Where::kHandle, 0.07f);
        else if (in(10.9f, 11.8f)) {
            put(r, Where::kFront, 0.0f);
            glide(r, Where::kHandle, 0.07f, 10.9f, 11.8f);
        } else if (in(11.8f, 12.0f))
            set(r, Where::kGrip, 0.0f);
        else if (in(12.0f, 12.2f))
            set(r, Where::kGrip, 1.0f, true);  // takes the pistol grip back
        else if (in(12.2f, 16.0f))
            set(r, Where::kGrip, 1.0f);
        if (r.where == Where::kMagazine || r.from == Where::kMagazine)
            for (int i = 0; i < 3; ++i)
                r.shift[i] = back[i];
        if (in(0.5f, 1.0f))
            set(l, Where::kGuard, 0.0f);
        else if (in(1.0f, 14.0f))
            set(l, Where::kGuard, 1.0f);  // on the handguard: it holds the rifle alone from 2.04
        else if (in(14.0f, 14.5f))
            set(l, Where::kGuard, 0.0f);
    } else if (test == 9) {
        // A magazine in the hand falls with the rifle; the left puts one in.
        if (in(0.0f, 0.5f))
            set(r, Where::kStandIn, 1.0f);
        else if (in(0.5f, 0.6f)) {
            set(r, Where::kStandIn, 1.0f);
            r.magTap = true;  // X: the magazine ejected
        } else if (in(0.6f, 1.0f))
            set(r, Where::kStandIn, 1.0f);
        else if (in(1.0f, 1.5f))
            set(r, Where::kStandIn, 0.0f);  // lets go: it falls
        else if (in(1.5f, 2.0f))
            put(r, Where::kFront, 0.0f);
        else if (in(2.0f, 2.5f)) {
            put(r, Where::kBelly, 0.0f);
            glide(r, Where::kFront, 0.0f, 2.0f, 2.5f);
        } else if (in(2.5f, 2.7f))
            put(r, Where::kBelly, 0.0f, 1.0f);  // a fresh magazine
        else if (in(2.7f, 3.2f)) {
            put(r, Where::kFront, 0.0f, 1.0f);
            glide(r, Where::kBelly, 0.0f, 2.7f, 3.2f);
        } else if (in(3.2f, 5.0f))
            put(r, Where::kFront, 0.0f, 1.0f);  // held: it falls with the rifle at 4.54
        else if (in(5.0f, 5.5f))
            put(r, Where::kFront, 0.0f);
        else if (in(5.5f, 5.7f))
            set(r, Where::kStandIn, 1.0f, true);  // picks it up
        else if (in(5.7f, 14.0f))
            set(r, Where::kStandIn, 1.0f);
        if (in(1.5f, 1.7f))
            set(l, Where::kMirror, 1.0f, true);  // picks it up
        else if (in(1.7f, 4.5f))
            set(l, Where::kMirror, 1.0f);
        else if (in(4.5f, 5.0f))
            set(l, Where::kMirror, 0.0f);  // lets go: it falls
        else if (in(6.0f, 6.5f))
            put(l, Where::kBelly, 0.0f);
        else if (in(6.5f, 6.7f))
            put(l, Where::kBelly, 0.0f, 1.0f);  // a fresh one
        else if (in(6.7f, 7.5f)) {
            put(l, Where::kWell, -0.20f, 1.0f);
            glide(l, Where::kBelly, 0.0f, 6.7f, 7.5f);
        } else if (in(7.5f, 9.5f))
            put(l, Where::kWell, -0.20f + 0.1f * (t - 7.5f), 1.0f);  // caught, pushed home
        else if (in(9.5f, 9.7f))
            put(l, Where::kWell, 0.0f);
        else if (in(9.7f, 12.0f)) {
            put(l, Where::kFront, 0.0f);
            glide(l, Where::kWell, 0.0f, 9.7f, 12.0f);
        }
    } else if (test == 10) {
        // A dropped gun of the game's taken by hand, both ways: the left takes it and the right takes its own gun back, then the right takes it
        // and the left takes the M416 back; the round ends with the M416 picked up off the ground by the right (the one forced take).
        if (in(0.0f, 1.0f))
            set(r, Where::kStandIn, 1.0f);
        else if (in(1.0f, 1.15f)) {
            set(r, Where::kStandIn, 0.0f);  // lets go, thrown to its right
            r.sensed = true;
            r.velocity[0] = 1.6f, r.velocity[1] = 0.8f, r.velocity[2] = -0.3f;
            r.spin[0] = 4.0f;
        } else if (in(1.15f, 7.5f))
            set(r, Where::kStandIn, 0.0f);
        else if (in(7.5f, 8.0f))
            set(r, Where::kGuard, 0.0f);  // at the taken gun's front
        else if (in(8.0f, 12.0f)) {
            set(r, Where::kGuard, 1.0f);  // on its front: two hands
            if (in(11.0f, 11.25f))
                r.trigger = 1.0f;  // nothing fires
        } else if (in(12.0f, 20.0f))
            set(r, Where::kStandIn, 0.0f);
        else if (in(20.0f, 21.0f))
            set(r, Where::kPickup, 0.0f);  // at the pickup that holds the gun it threw
        else if (in(21.0f, 22.5f))
            set(r, Where::kPickup, 1.0f);  // the take: its own gun back
        else if (in(22.5f, 31.0f))
            set(r, Where::kStandIn, 1.0f);
        else if (in(31.0f, 31.15f)) {
            set(r, Where::kStandIn, 0.0f);  // lets go, thrown to its right, as at 1.0
            r.sensed = true;
            r.velocity[0] = 1.6f, r.velocity[1] = 0.8f, r.velocity[2] = -0.3f;
            r.spin[0] = 4.0f;
        } else if (in(31.15f, 34.0f))
            set(r, Where::kStandIn, 0.0f);
        else if (in(34.0f, 35.0f))
            set(r, Where::kPickup, 0.0f);  // at the dropped gun
        else if (in(35.0f, 36.5f))
            set(r, Where::kPickup, 1.0f);  // the take, by the right
        else if (in(36.5f, 44.0f)) {
            set(r, Where::kStandIn, 1.0f);  // lifted, held by its pistol grip
            if (in(40.0f, 40.25f))
                r.trigger = 1.0f;  // it fires the gun
        } else if (in(44.0f, 57.0f))
            set(r, Where::kStandIn, 0.0f);  // lets go: it falls
        else if (in(57.0f, 57.2f))
            set(r, Where::kStandIn, 1.0f, true);  // picks the lying M416 up
        else if (in(57.2f, 60.0f))
            set(r, Where::kStandIn, 1.0f);
        if (in(4.0f, 5.0f))
            set(l, Where::kPickup, 0.0f);  // at the dropped gun
        else if (in(5.0f, 6.5f))
            set(l, Where::kPickup, 1.0f);  // the take: the game swaps, the gun flies in
        else if (in(6.5f, 14.0f)) {
            set(l, Where::kMirror, 1.0f);  // lifted in front of the chest, held by its pistol grip
            if (in(10.0f, 10.25f))
                l.trigger = 1.0f;  // it fires the gun
        } else if (in(14.0f, 16.0f))
            set(l, Where::kMirror, 0.0f);  // lets go: it falls
        else if (in(37.5f, 38.0f))
            set(l, Where::kGuard, 0.0f);  // at its front
        else if (in(38.0f, 42.0f)) {
            set(l, Where::kGuard, 1.0f);  // on its front: two hands
            if (in(41.0f, 41.25f))
                l.trigger = 1.0f;  // nothing fires
        } else if (in(50.0f, 51.0f))
            set(l, Where::kPickup, 0.0f);  // at the pickup that holds the M416
        else if (in(51.0f, 52.5f))
            set(l, Where::kPickup, 1.0f);  // the take: the M416, by the left
        else if (in(52.5f, 55.0f))
            set(l, Where::kMirror, 1.0f);  // holds the M416 by its pistol grip
        else if (in(55.0f, 57.0f))
            set(l, Where::kMirror, 0.0f);  // lets go: it falls
    } else if (test == 11) {
        if (in(0.0f, 1.0f))
            set(r, Where::kStandIn, 1.0f);
        else if (in(1.0f, 4.0f))
            set(r, Where::kStandIn, 0.0f);  // lets go: it falls
        else if (in(4.0f, 5.0f))
            set(r, Where::kLow, 0.0f);  // lowered beside the knee, open
        else if (in(5.0f, 7.0f))
            set(r, Where::kLow, 1.0f);  // the take from the floor: the magnet
        else if (in(7.0f, 9.0f))
            set(r, Where::kStandIn, 1.0f);
        else if (in(9.0f, 12.0f))
            set(r, Where::kStandIn, 0.0f);  // lets go: it falls
        else if (in(12.0f, 13.0f))
            set(r, Where::kBelly, 0.0f);
        else if (in(13.0f, 13.5f))
            set(r, Where::kBelly, 1.0f);  // the belly's squeeze: nothing off the floor
        else if (in(13.5f, 15.0f))
            set(r, Where::kFront, 0.0f);
        else if (in(15.0f, 15.5f))
            set(r, Where::kFront, 1.0f);  // above the gate: nothing
        else if (in(15.5f, 16.0f))
            set(r, Where::kFront, 0.0f);
        else if (in(23.0f, 24.0f))
            set(r, Where::kStandIn, 1.0f);  // a squeeze far from anything ...
        else if (in(24.0f, 26.0f))
            set(r, Where::kLow, 1.0f);  // ... held while lowered: nothing
        else if (in(26.0f, 27.0f))
            set(r, Where::kLow, 0.0f);
        else if (in(27.0f, 29.0f))
            set(r, Where::kLow, 1.0f);  // a fresh squeeze: the take
        else if (in(29.0f, 35.0f))
            set(r, Where::kStandIn, 1.0f);  // the rifle back in the right hand for the next round
        if (in(16.0f, 17.0f))
            set(l, Where::kLow, 0.0f);
        else if (in(17.0f, 22.0f))
            set(l, Where::kLow, 1.0f);  // the left's take from the floor
        else if (in(22.0f, 23.0f))
            set(l, Where::kLow, 0.0f);  // lets go there: it falls under the left
    } else if (test == 12) {
        // The hand-overs are test 9's (1.0 / 1.5, the right's forced take back).
        if (in(0.0f, 1.0f))
            set(r, Where::kStandIn, 1.0f);
        else if (in(1.0f, 1.5f))
            set(r, Where::kStandIn, 0.0f);  // lets go: it falls
        else if (in(1.5f, 14.5f))
            set(r, Where::kFront, 0.0f);
        else if (in(14.5f, 15.0f))
            set(r, Where::kStandIn, 0.0f);
        else if (in(15.0f, 15.2f))
            set(r, Where::kStandIn, 1.0f, true);  // picks it up
        else if (in(15.2f, 18.0f))
            set(r, Where::kStandIn, 1.0f);
        if (in(1.5f, 1.7f))
            set(l, Where::kMirror, 1.0f, true);  // picks it up
        else if (in(1.7f, 14.5f))
            set(l, Where::kMirror, 1.0f);  // holds it by its pistol grip
        else if (in(14.5f, 15.0f))
            set(l, Where::kMirror, 0.0f);  // lets go: it falls
        // The left's D-pad right (a button: pressed whatever the left's place, also at 16.5 with no step of its own): taps of 0.2 s, holds of 0.8 s.
        l.dpadRight = in(3.0f, 3.2f) || in(4.5f, 4.7f) || in(6.0f, 6.8f) || in(9.5f, 9.7f) || in(10.0f, 10.8f) || in(16.5f, 16.7f);
    } else if (test == 13) {
        // Holsters. The right holds the M416 at the stand-in's place at the start and at the end.
        using namespace round13;
        constexpr Where kAt = Where::kBodyAt, kLa = Where::kLauncherAt;
        // At a body place with the grip; moved there from a body place over [a, b) (eased, as glide); and along a straight line between two places
        // of one kind at an even speed over [a, b).
        const auto place = [](Step& s, Where where, const float* p, float squeeze) {
            s.where = where;
            s.squeeze = squeeze;
            for (int i = 0; i < 3; ++i)
                s.body[i] = p[i];
        };
        const auto glideFrom = [&glide](Step& s, Where from, const float* p, float a, float b) {
            glide(s, from, 0.0f, a, b);
            for (int i = 0; i < 3; ++i)
                s.fromBody[i] = p[i];
        };
        const auto straight = [t, &place](Step& s, Where where, const float* p, const float* q, float a, float b, float squeeze) {
            const float u = (t - a) / (b - a), k = u > 0.0f ? (u < 1.0f ? u : 1.0f) : 0.0f;
            const float at[3] = {p[0] + (q[0] - p[0]) * k, p[1] + (q[1] - p[1]) * k, p[2] + (q[2] - p[2]) * k};
            place(s, where, at, squeeze);
        };
        const float sweepTo = kSweepFrom + (kFrontOfShoulder[2] - kBehindShoulder[2]) / kSweep;  // (33.475)
        const float sweepCloses = kSweepFrom + (kFrontOfShoulder[2] - kSweepCloses) / kSweep;   // (33.35)
        if (in(0.0f, 0.6f)) {
            place(r, kAt, kShoulder, 1.0f);  // 1: the M416 to the right shoulder ...
            glide(r, Where::kStandIn, 0.0f, 0.0f, 0.6f);
        } else if (in(0.6f, 0.9f))
            place(r, kAt, kShoulder, 1.0f);
        else if (in(0.9f, 1.8f))
            place(r, kAt, kShoulder, 0.0f);  // ... opened there: put away
        else if (in(1.8f, 2.3f))
            place(r, kAt, kShoulder, 1.0f);  // 2: squeezed there: drawn, no key
        else if (in(2.3f, 2.6f))
            place(r, kAt, kShoulder, 0.0f);  // 3: put away again
        else if (in(2.6f, 3.2f)) {
            set(r, Where::kStandIn, 0.0f);
            glideFrom(r, kAt, kShoulder, 2.6f, 3.2f);
        } else if (in(3.2f, 5.0f))
            set(r, Where::kStandIn, 0.0f);
        else if (in(5.0f, 5.6f)) {
            place(r, kLa, kLauncher, 0.0f);  // 5: behind the neck ...
            glide(r, Where::kStandIn, 0.0f, 5.0f, 5.6f);
        } else if (in(5.6f, 5.9f))
            place(r, kLa, kLauncher, 0.0f);
        else if (in(5.9f, 6.2f))
            place(r, kLa, kLauncher, 1.0f);  // ... squeezed: the SMAW asked for (key 4), the hand keeping the coming gun ...
        else if (in(6.2f, 6.8f)) {
            set(r, Where::kStandIn, 1.0f);
            glideFrom(r, kLa, kLauncher, 6.2f, 6.8f);
        } else if (in(6.8f, 9.6f))
            set(r, Where::kStandIn, 1.0f);  // ... in it from its first drawn frame
        else if (in(9.6f, 10.4f)) {
            place(r, kLa, kLauncher, 1.0f);  // 7: behind the neck with it ...
            glide(r, Where::kStandIn, 0.0f, 9.6f, 10.4f);
        } else if (in(10.4f, 10.7f))
            place(r, kLa, kLauncher, 1.0f);
        else if (in(10.7f, 11.0f))
            place(r, kLa, kLauncher, 0.0f);  // ... opened: put away
        else if (in(11.0f, 11.5f)) {
            place(r, kAt, kShoulder, 0.0f);  // 8: to the right shoulder ...
            glideFrom(r, kLa, kLauncher, 11.0f, 11.5f);
        } else if (in(11.5f, 11.8f))
            place(r, kAt, kShoulder, 0.0f);
        else if (in(11.8f, 12.1f))
            place(r, kAt, kShoulder, 1.0f);  // ... squeezed: the M416 asked for (key 1)
        else if (in(12.1f, 12.7f)) {
            set(r, Where::kStandIn, 1.0f);
            glideFrom(r, kAt, kShoulder, 12.1f, 12.7f);
        } else if (in(12.7f, 14.0f))
            set(r, Where::kStandIn, 1.0f);
        else if (in(14.0f, 14.6f)) {
            place(r, kAt, kShoulder, 1.0f);  // 10: to the shoulder with it ...
            glide(r, Where::kStandIn, 0.0f, 14.0f, 14.6f);
        } else if (in(14.6f, 14.9f))
            place(r, kAt, kShoulder, 1.0f);
        else if (in(14.9f, 15.05f)) {
            place(r, kAt, kShoulder, 0.0f);  // ... let go there at 2.5 m/s: thrown, not put away
            r.sensed = true;
            r.velocity[0] = 0.9f, r.velocity[1] = 1.2f, r.velocity[2] = -2.0f;
            r.spin[0] = 4.0f;
        } else if (in(15.05f, 15.6f)) {
            set(r, Where::kStandIn, 0.0f);
            glideFrom(r, kAt, kShoulder, 15.05f, 15.6f);
        } else if (in(15.6f, 16.6f))
            set(r, Where::kStandIn, 0.0f);
        else if (in(16.6f, 16.8f))
            set(r, Where::kStandIn, 1.0f, true);  // 12: takes it up
        else if (in(16.8f, 19.4f))
            set(r, Where::kStandIn, 1.0f);  // (still while the left pulls the M320's case out)
        else if (in(19.4f, 20.0f)) {
            place(r, kAt, kShoulder, 1.0f);  // 12b: to the shoulder with the left holding the case ...
            glide(r, Where::kStandIn, 0.0f, 19.4f, 20.0f);
        } else if (in(20.0f, 20.3f))
            place(r, kAt, kShoulder, 1.0f);
        else if (in(20.3f, 20.8f))
            place(r, kAt, kShoulder, 0.0f);  // ... opened: not put away, it falls
        else if (in(20.8f, 21.4f)) {
            set(r, Where::kStandIn, 0.0f);
            glideFrom(r, kAt, kShoulder, 20.8f, 21.4f);
        } else if (in(21.4f, 21.6f))
            set(r, Where::kStandIn, 0.0f);
        else if (in(21.6f, 21.8f))
            set(r, Where::kStandIn, 1.0f, true);  // takes it up
        else if (in(21.8f, 23.4f))
            set(r, Where::kStandIn, 1.0f);  // (still while the left slaps the M320 shut)
        else if (in(23.4f, 24.0f)) {
            place(r, kAt, kOutside, 1.0f);  // 13: 6 cm outside the shoulder ...
            glide(r, Where::kStandIn, 0.0f, 23.4f, 24.0f);
        } else if (in(24.0f, 24.3f))
            place(r, kAt, kOutside, 1.0f);
        else if (in(24.3f, 24.7f))
            place(r, kAt, kOutside, 0.0f);  // ... opened: it falls
        else if (in(24.7f, 25.3f)) {
            set(r, Where::kStandIn, 0.0f);
            glideFrom(r, kAt, kOutside, 24.7f, 25.3f);
        } else if (in(25.3f, 25.5f))
            set(r, Where::kStandIn, 0.0f);
        else if (in(25.5f, 25.7f))
            set(r, Where::kStandIn, 1.0f, true);  // 14: takes it up
        else if (in(25.7f, 26.2f))
            set(r, Where::kStandIn, 1.0f);
        else if (in(26.2f, 26.8f)) {
            place(r, kLa, kOverShoulder, 1.0f);  // 15: over the shoulder with it ...
            glide(r, Where::kStandIn, 0.0f, 26.2f, 26.8f);
        } else if (in(26.8f, kUnseenIn[0]))
            place(r, kLa, kOverShoulder, 1.0f);
        else if (in(kUnseenIn[0], kUnseenIn[1])) {
            straight(r, kLa, kOverShoulder, kLauncher, kUnseenIn[0], kUnseenIn[1], 1.0f);  // ... in behind the neck, not seen for its last 0.3 s ...
            r.unseen = t >= kUnseenIn[2];
        } else if (in(kUnseenIn[1], 27.6f)) {
            place(r, kLa, kLauncher, 1.0f);
            r.unseen = true;
        } else if (in(27.6f, 27.8f)) {
            place(r, kLa, kLauncher, 0.0f);  // ... opened there not seen: put away
            r.unseen = true;
        } else if (in(27.8f, 28.0f))
            place(r, kLa, kLauncher, 0.0f);  // seen again
        else if (in(28.0f, 28.5f)) {
            place(r, kAt, kShoulder, 0.0f);
            glideFrom(r, kLa, kLauncher, 28.0f, 28.5f);
        } else if (in(28.5f, 28.8f))
            place(r, kAt, kShoulder, 0.0f);
        else if (in(28.8f, 29.1f))
            place(r, kAt, kShoulder, 1.0f);  // drawn at the shoulder again
        else if (in(29.1f, 29.6f)) {
            place(r, kLa, kOverShoulder, 1.0f);  // 15b: over the shoulder with it ...
            glideFrom(r, kAt, kShoulder, 29.1f, 29.6f);
        } else if (in(29.6f, kLostIn[0]))
            place(r, kLa, kOverShoulder, 1.0f);
        else if (in(kLostIn[0], kLostIn[1])) {
            straight(r, kLa, kOverShoulder, kLauncher, kLostIn[0], kLostIn[1], 1.0f);  // ... in behind the neck, lost 0.1 s before it's there ...
            r.lost = t >= kLostIn[2];
        } else if (in(kLostIn[1], 32.0f)) {
            place(r, kLa, kLauncher, 1.0f);
            r.lost = true;
        } else if (in(32.0f, 32.3f)) {
            place(r, kLa, kLauncher, 0.0f);  // ... opened 2 s after it was lost: put away
            r.lost = true;
        } else if (in(32.3f, 32.5f))
            place(r, kLa, kLauncher, 0.0f);  // found again there
        else if (in(32.5f, 33.0f)) {
            place(r, kAt, kFrontOfShoulder, 0.0f);  // 15c: in front of the shoulder ...
            glideFrom(r, kLa, kLauncher, 32.5f, 33.0f);
        } else if (in(33.0f, kSweepFrom))
            place(r, kAt, kFrontOfShoulder, 0.0f);
        else if (in(kSweepFrom, sweepTo))  // ... through it at 0.8 m/s, the grip closing at its back edge: nothing drawn from it ...
            straight(r, kAt, kFrontOfShoulder, kBehindShoulder, kSweepFrom, sweepTo, t >= sweepCloses ? 1.0f : 0.0f);
        else if (in(sweepTo, 34.0f)) {
            place(r, kLa, kLauncher, 1.0f);  // ... nor behind the neck, the grip held
            glideFrom(r, kAt, kBehindShoulder, sweepTo, 34.0f);
        } else if (in(34.0f, 34.3f))
            place(r, kLa, kLauncher, 1.0f);
        else if (in(34.3f, 34.6f))
            place(r, kLa, kLauncher, 0.0f);
        else if (in(34.6f, 35.1f)) {
            place(r, kLa, kInShoulder, 0.0f);  // 16: into the shoulder ...
            glideFrom(r, kLa, kLauncher, 34.6f, 35.1f);
        } else if (in(35.1f, kLeaveIn[0]))
            place(r, kLa, kInShoulder, 0.0f);
        else if (in(kLeaveIn[0], kLeaveIn[1])) {
            // ... out of it for the neck at 0.6 m/s, lost at once, squeezed 0.3 s later ...
            straight(r, kLa, kInShoulder, kLauncher, kLeaveIn[0], kLeaveIn[1], t >= kLeaveIn[3] ? 1.0f : 0.0f);
            r.lost = t >= kLeaveIn[2];
        } else if (in(kLeaveIn[1], 36.35f)) {
            place(r, kLa, kLauncher, 1.0f);
            r.lost = true;
        } else if (in(36.35f, 38.0f))
            set(r, Where::kStandIn, 1.0f);  // ... found in front 0.6 s after: the SMAW in the hand, never the M416
        else if (in(38.0f, 38.6f)) {
            place(r, kLa, kLauncher, 1.0f);  // 17: behind the neck with it ...
            glide(r, Where::kStandIn, 0.0f, 38.0f, 38.6f);
        } else if (in(38.6f, 38.9f))
            place(r, kLa, kLauncher, 1.0f);
        else if (in(38.9f, 39.2f))
            place(r, kLa, kLauncher, 0.0f);  // ... put away
        else if (in(39.2f, 39.7f)) {
            place(r, kAt, kShoulder, 0.0f);
            glideFrom(r, kLa, kLauncher, 39.2f, 39.7f);
        } else if (in(39.7f, 40.0f))
            place(r, kAt, kShoulder, 0.0f);
        else if (in(40.0f, 40.3f))
            place(r, kAt, kShoulder, 1.0f);  // the M416 asked for (key 1)
        else if (in(40.3f, 40.9f)) {
            set(r, Where::kStandIn, 1.0f);
            glideFrom(r, kAt, kShoulder, 40.3f, 40.9f);
        } else if (in(40.9f, 44.0f))
            set(r, Where::kStandIn, 1.0f);  // as at the start
        // The left, from 2.6 s to 23.1 s.
        if (in(2.6f, 3.3f)) {
            place(l, kAt, kShoulder, 0.0f);  // 3: to the right shoulder ...
            glideFrom(l, kAt, kLeftRest, 2.6f, 3.3f);
        } else if (in(3.3f, 3.6f))
            place(l, kAt, kShoulder, 0.0f);
        else if (in(3.6f, 3.9f))
            place(l, kAt, kShoulder, 1.0f);  // ... squeezed: drawn into the left, its trigger hand
        else if (in(3.9f, 4.5f)) {
            place(l, kLa, kLauncher, 1.0f);  // 4: behind the neck with it ...
            glideFrom(l, kAt, kShoulder, 3.9f, 4.5f);
        } else if (in(4.5f, 4.8f))
            place(l, kLa, kLauncher, 1.0f);
        else if (in(4.8f, 5.2f))
            place(l, kLa, kLauncher, 0.0f);  // ... opened: put away at its own place, the right shoulder
        else if (in(5.2f, 5.8f)) {
            place(l, kAt, kLeftRest, 0.0f);
            glideFrom(l, kLa, kLauncher, 5.2f, 5.8f);
        } else if (in(5.8f, 7.6f))
            place(l, kAt, kLeftRest, 0.0f);
        else if (in(7.6f, 8.0f))
            set(l, Where::kGuard, 0.0f);  // 6: at the SMAW's front grip ...
        else if (in(8.0f, 9.0f))
            set(l, Where::kGuard, 1.0f);  // ... takes it ...
        else if (in(9.0f, 9.3f))
            set(l, Where::kGuard, 0.0f);  // ... and lets go
        else if (in(9.3f, 12.7f))
            place(l, kAt, kLeftRest, 0.0f);
        else if (in(12.7f, 13.3f)) {
            place(l, kLa, kLauncher, 0.0f);  // 9: behind the neck ...
            glideFrom(l, kAt, kLeftRest, 12.7f, 13.3f);
        } else if (in(13.3f, 13.5f))
            place(l, kLa, kLauncher, 0.0f);
        else if (in(13.5f, 14.0f))
            place(l, kLa, kLauncher, 1.0f);  // ... squeezed with the right holding the M416: nothing, not felt
        else if (in(14.0f, 14.6f)) {
            place(l, kAt, kLeftRest, 0.0f);
            glideFrom(l, kLa, kLauncher, 14.0f, 14.6f);
        } else if (in(14.6f, 15.0f))
            place(l, kAt, kLeftRest, 0.0f);
        else if (in(15.0f, 15.6f)) {
            place(l, kLa, kLauncher, 0.0f);  // 11: behind the neck ...
            glideFrom(l, kAt, kLeftRest, 15.0f, 15.6f);
        } else if (in(15.6f, 15.8f))
            place(l, kLa, kLauncher, 0.0f);
        else if (in(15.8f, 16.4f))
            place(l, kLa, kLauncher, 1.0f);  // ... squeezed with the M416 on the ground: refused once the take's 0.4 s are over
        else if (in(16.4f, 17.0f)) {
            place(l, kAt, kLeftRest, 0.0f);
            glideFrom(l, kLa, kLauncher, 16.4f, 17.0f);
        } else if (in(17.0f, 17.1f)) {
            place(l, kAt, kLeftRest, 0.0f);
            l.m320Case = true;  // 12b: a spent case in the M320's open barrel ...
        } else if (in(17.1f, 17.6f)) {
            put(l, Where::kRim, 0.0f);
            glideFrom(l, kAt, kLeftRest, 17.1f, 17.6f);
        } else if (in(17.6f, 17.8f))
            put(l, Where::kRim, 0.0f, 1.0f);  // ... its rim pinched ...
        else if (in(17.8f, 18.6f))
            put(l, Where::kRim, 0.2f * (t - 17.8f), 1.0f);  // ... pulled out ...
        else if (in(18.6f, 18.8f))
            put(l, Where::kRim, 0.16f, 1.0f);
        else if (in(18.8f, 19.4f)) {
            place(l, kAt, kLeftRest, 1.0f);  // ... and held while the right opens at the shoulder
            glide(l, Where::kRim, 0.16f, 18.8f, 19.4f);
        } else if (in(19.4f, 21.0f))
            place(l, kAt, kLeftRest, 1.0f);
        else if (in(21.0f, 22.0f))
            place(l, kAt, kLeftRest, 0.0f);  // let go of: it falls
        else if (in(22.0f, 22.4f)) {
            put(l, Where::kSlap, 0.0f);
            glideFrom(l, kAt, kLeftRest, 22.0f, 22.4f);
        } else if (in(22.4f, 22.6f))
            put(l, Where::kSlap, (t - 22.4f) / 0.2f);  // the M320 slapped shut
        else if (in(22.6f, 23.1f)) {
            place(l, kAt, kLeftRest, 0.0f);
            glide(l, Where::kSlap, 1.0f, 22.6f, 23.1f);
        }
    } else if (test == 14) {
        // A held gun's lever by the hand at work with the gun held by its front alone (the owner, 2026-11-19), then the mirror for a lefty. The hand at
        // work's script, either side: from where it is onto the knob over the top, its trigger takes it, drawn back as far as it goes and let go; onto it
        // from below, its grip takes it, drawn and let go; then to its pistol grip, a fresh squeeze takes it back.
        const auto lever = [&](Step& s, Where from, float t0) {
            const float u = t - t0;
            if (u < 0.7f) {
                put(s, Where::kLeverOver, 0.0f);
                glide(s, from, 0.0f, t0, t0 + 0.7f);
            } else if (u < 0.9f) {
                put(s, Where::kLeverOver, 0.0f, 0.0f, 1.0f);  // its trigger takes it
            } else if (u < 1.9f) {
                put(s, Where::kLeverOver, 1.05f * (u - 0.9f), 0.0f, 1.0f);  // drawn back as far as it goes
            } else if (u < 2.1f) {
                put(s, Where::kLeverOver, 1.05f);  // let go: it springs home
            } else if (u < 2.8f) {
                put(s, Where::kLeverUnder, 0.0f);
                glide(s, Where::kLeverOver, 1.05f, t0 + 2.1f, t0 + 2.8f);
            } else if (u < 3.0f) {
                put(s, Where::kLeverUnder, 0.0f, 1.0f);  // its grip takes it from below
            } else if (u < 4.0f) {
                put(s, Where::kLeverUnder, 1.05f * (u - 3.0f), 1.0f);
            } else if (u < 4.2f) {
                put(s, Where::kLeverUnder, 1.05f);  // let go
            } else if (u < 5.0f) {
                set(s, Where::kGrip, 0.0f);  // to its pistol grip
            } else {
                set(s, Where::kGrip, 1.0f);  // a fresh squeeze: it takes it back (no forced take)
            }
        };
        // The owner's case: the right on the pistol grip, the left takes the front, the right lets go (held by the left's front alone), works the lever,
        // takes its grip back and fires, the left still on the front; then the left lets go of it.
        if (in(0.0f, 2.0f))
            set(r, Where::kStandIn, 1.0f);
        else if (in(2.0f, 2.5f))
            set(r, Where::kStandIn, 0.0f);  // lets go: held by the left's front alone
        else if (in(2.5f, 8.5f))
            lever(r, Where::kStandIn, 2.5f);
        else if (in(8.5f, 10.0f)) {
            set(r, Where::kGrip, 1.0f);
            if (in(9.0f, 9.2f))
                r.trigger = 1.0f;  // fires: the hand on the pistol grip
        } else if (in(10.0f, 10.5f))
            set(r, Where::kGrip, 0.0f);  // lets go: it falls
        else if (in(11.0f, 11.5f))
            set(r, Where::kGuard, 0.0f);
        else if (in(11.5f, 19.5f))
            set(r, Where::kGuard, 1.0f);  // on the front of the gun in the left hand, through the left's shot (a film's real round: two hands) ...
        else if (in(19.5f, 20.0f))
            set(r, Where::kGuard, 0.0f);  // ... and off it
        else if (in(21.0f, 21.2f))
            set(r, Where::kStandIn, 1.0f, true);  // picks it up: as at the round's start
        else if (in(21.2f, 24.0f))
            set(r, Where::kStandIn, 1.0f);
        if (in(0.5f, 1.5f))
            set(l, Where::kGuard, 0.0f);
        else if (in(1.5f, 9.5f))
            set(l, Where::kGuard, 1.0f);  // the front: the gun held by it alone from 2.04 to the right's regrip, then on through the right's shot
        else if (in(9.5f, 10.0f))
            set(l, Where::kGuard, 0.0f);  // let go of
        else if (in(10.5f, 10.7f))
            set(l, Where::kMirror, 1.0f, true);  // picks it up: by its pistol grip
        else if (in(10.7f, 12.0f))
            set(l, Where::kMirror, 1.0f);
        else if (in(12.0f, 12.5f))
            set(l, Where::kMirror, 0.0f);  // lets go: held by the right's front alone
        else if (in(12.5f, 18.5f))
            lever(l, Where::kMirror, 12.5f);
        else if (in(18.5f, 20.0f)) {
            set(l, Where::kGrip, 1.0f);
            if (in(19.0f, 19.2f))
                l.trigger = 1.0f;  // fires
        } else if (in(20.0f, 20.5f))
            set(l, Where::kGrip, 0.0f);  // lets go: it falls
    }
}

}  // namespace freetest

#pragma once
// A hand's use of a scripted prompt: a point of the world the game shows a prompt at (pickups.h's UsePoint, the game's ServerInteractionEntity:
// Operation Swordbreaker's "pick up the RPG" on the hotel roof, where Matkovic lets the AT4 fall; a door to breach; the cellar's wire), used by a
// click of the grip. Pure, header only: hands.cpp runs Update once a frame and makes the use (the game's own, else the prompt's key); tools/at4test.cpp
// checks it without the game. Not the game's own pickups (a ServerPickupEntity: the train's M1014, a dead soldier's gun): those are pickups.h's takes.
// And the grip as the interact button where the mod sees no prompt (Frame::tap, Out::tap: a truck's "press E to enter").
//   - Only a prompt a hand uses (ByHand): on the interact key (EIAInteract: doors, breaches, climbing out, a rappel, a mortar, a seat, the HALO jump's
//     start) or the reload key (EIAReload: the game's own pickup key, the AT4's), a press (HoldToInteractTime 0: one call of the game's use finishes
//     it, as one press of its key does; a hold's user idles out after 0.4 s with its use counted), never a flag, the game's own reach for it kMostRadius
//     at most. The campaign's 41 use points on E and R (Venice-EBX, 2026-11-25): every one a press; its pose prompts (EIAProne, EIAChangePose, EIAClutch,
//     holds of 1 s: the AT4's roof has two, 100 m round), jumps, fires, weapon picks and the jet's and the tank's 50-300 m prompts are never a hand's
//     (their own buttons, or the quick-time events').
//   - Frame::anywhere (the default; the owner, 2026-11-25: "holding grip down if you needed to open a door and it said hold R or E to open. I want to
//     be able to just hold the grip button instead"; 2026-11-26, the door open by grip: "it takes a while, and it's not very responsive ... make those
//     grip prompts very responsive, like even a click will open whatever. That's like the interactive button"): while the game shows the prompt (it
//     targets the point: the soldier within its radius and angle; the point on, not used up: Point::on), a click of the grip of a free hand uses it,
//     wherever the hand is. Nothing in a new life's first frame, a weapon switch's bridging frames or a fist fight's run (Frame::asleep); free hands
//     asleep (another gun, the train pistol), no bar; a scripted moment (a cutscene's camera, the game turning the look) no bar either, but there
//     hands.cpp makes the use by the prompt's own key, as Y's press, so the game's input decides whether the moment takes it (never the game's use
//     called past it: the review of babecfd, finding 2), and in a cutscene (Frame::cutscene) no squeeze held from before becomes the prompt's, nor ever
//     one held through it.
//     A prompt that goes unseen kGapMost at most (the game's target rescanned, a frame asleep) is still the one showing: a squeeze made or owned in its
//     gap is the prompt's, asked as it's back, and a click let go in it is owed (HandState::due) and asked as it's back.
//     A grip not read (its controller unseen a moment, the headset off) keeps its state and counts nothing, never a let-go; read again squeezed (or
//     squeezed as the state begins, after a new life) it is no fresh squeeze: let go and squeeze again.
//       - The hand free: holding nothing (no gun by its grip or its front, no magazine, round or case, no module's: Hand::empty) and its squeeze nothing
//         else's of the hands' (Hand::yield, hands.cpp's UseYields: at a holster's place, a draw or a put-away; with a gun in a hand, in the belly's own
//         zone where its takes are (BodyKeeps, BellyTakes: a magazine, a round, a rocket); near a gun in a hand, its front, its magazine or its handle;
//         on a gun lying nearer than the prompt's target (LyingFirst)). One owner per squeeze.
//       - Its squeeze the prompt's: a fresh one (reaching kSqueezeOn) while the prompt shows (or in its gap); or one held from before, begun free (not at
//         a body's place or near the gun: HandState::beganFree) and free now, once the prompt has shown kHeldIntoShown (walking past a door's edge with a
//         fist held never opens it: the review of 01a5132, finding 6). Its use is asked (Out::how), with a tick on that hand (Out::entered: holster.h's
//         0.25 for 20 ms; the line hands.cpp says with it is the word for a player whose buzz is off), at the first of:
//           . let go (a click: Ask::kLetGo, as the squeeze ends; the hand free then);
//           . held kStillFor with the hand still (under kStillSpeed, within kStillMove of where it squeezed) and clear of every place a module of the
//             hands' takes a squeeze carried there (Hand::clear: hands.cpp's UseClear, each place kUseClearMore wider): Ask::kStill;
//           . held kFreshAsk (Ask::kHeld): past every module's reach window from the squeeze's crossing (grasp.h's, pouch.cpp's, rocket.cpp's,
//             m320.cpp's and holster.h's going-on kReachWindow, 0.4 s each).
//         So a squeeze begun free and carried on to a magazine, a round, a rocket, a shoulder's gun, the handguard or a lying gun inside their windows
//         is theirs (it moves, or starts by their places: never asked before it gets there), as the review of babecfd's finding 1 had it, and a click
//         is the interact button's. (Till 2026-11-26 it was asked kFreshAsk from the squeeze, held 0.25 s, and one held from before once the prompt had
//         shown 0.4 s: at the bank's breach the owner's three clicks of the right grip did nothing, a fourth held 0.44 s opened it. The build of
//         01a5132 asked it the frame after the squeeze: the review of that, finding 1, a reach to the belly for a magazine at a door opened the door
//         every time, and so a draw at a shoulder.) One use a squeeze: again only after a let-go.
//       - Till its use is asked the squeeze is only owned, never claimed: a module that takes it (the grasp's take of a lying gun, the magnet, a regrip,
//         a catch; the pouch, the handguard, a round, a rocket) has it, and the use gives up (Out::given: the hand isn't empty the frame after), and so
//         it does if its squeeze becomes something else's before its ask (Hand::yield: Out::yielded).
//       - A click of the hand on the gun's front (Hand::front: the handguard or a front grip, the other hand on the pistol grip, so its let-go keeps the
//         gun in that hand): let go and squeezed again within kClickGap, in place (kClickMove), while the prompt shows. Its squeeze stays the hands'
//         (Out::support: never claimed nor busy) and the hands take its hold on the front again with it (hands.cpp's Support); its use is asked once
//         they have (Hand::front again, the frame after: Ask::kFront), never before, and given up if they don't within kFrontWait, or the hand takes
//         something else (a magazine, the charging handle, the launcher's grip, the pistol's slide), or the gun leaves the other hand (the review of
//         01a5132, finding 2: the use asked on the squeeze's own frame with the hand on its way to the magazine well). The owner at the cellar's start
//         prompt, 2026-11-26 03:25:36-38, both hands on the rifle: "a free hand's grip answers" was all there was. The pistol-grip hand never answers
//         so (its let-go is the gun's), nor any click at a prompt that gives a gun with a gun in a hand.
//       - Two hands: the use asked by one hand's squeeze, the other's squeeze owned and not yet asked is the use's too (asked, claimed: one use); and while
//         the squeeze that made it is held, no squeeze of the other hand becomes the same prompt's, nor for the rest of the showing one held from before
//         (State::used: a door kicked open and shut again, a breach door used twice in its own scene, by the review of babecfd, finding 3).
//       - A use not made for a moment (Transient: the game didn't target it at its next tick, too late, the soldier not awake, not answered in time)
//         is asked again kRetryAfter on (Rearm: hands.cpp's NoteUse), kMostRetries a squeeze at most, only for the prompt it was asked for while that
//         one's showing lasts (HandState::askedId: the review of 01a5132, finding 3, a squeeze held on through a failed use used the next prompt that
//         showed, unasked): while the squeeze is held, or for a click let go meanwhile, owed (HandState::due) while the hand isn't squeezed again.
//   - The grip as the interact button with no prompt the mod sees (Frame::anywhere; Out::tap): the game's other press-E prompts aren't use points (a
//     vehicle's own entry: Swordbreaker's technical truck, "press E to enter" within 5 m of its gunner's entry, docs/research/mounted_gun.md 2.1; a
//     mounted gun's), and nothing the mod reads says one is up (the soldier's interaction target is a pickup or a use point, never a vehicle's entry;
//     seatread.h's rows are unmeasured). So a squeeze of a free hand begun with no prompt showing nor within its kGapMost, that nothing of the hands'
//     takes, taps the player's own use key once (hands.cpp's Interact: input.cpp's TapInteract, as Y presses it), by the same times as a prompt's use
//     (let go, still and clear kStillFor, held kFreshAsk: a reach to the belly for a magazine is the magazine's, never E on its way), and so does a
//     click of the hand on the gun's front once its hold is taken again (the review of 01a5132, finding 4: the rifle in both hands at the truck);
//     unless Frame::tap bars it (TapBarOf: free hands asleep, a fist fight, a quick-time trigger live or a key awaited however long, a use asked or made
//     within kTapAfterUse, a scripted moment or a cutscene, the soldier not on foot (a seat: the use key would get off) or not known to be, a use point
//     on the interact key that isn't a hand's). On foot in play the use key with nothing to use does nothing; the owner at the truck, 2026-11-26
//     03:30:32-45, his rifle on the ground, squeezed his free grips five times (four the right's, by the belly) and nothing came of them. Never a grip
//     held from before (nothing says when such a prompt comes up): let go and squeeze again.
//   - pickups.txt "usereach 1" (Frame::anywhere false): 0.1.14's rule, bit for bit: only while free hands are awake and no scripted moment; the hand
//     within the point's reach (Reach: its disc, Point::disc, kDisc unless the prompt has its own: the AT4's, its tube lying 0.65 m off the point;
//     across, from kBelow under it to kAbove over it, kWiden wider each metre up; once where the thing lies is measured, the capsule round it,
//     kCapsuleGrow wider); a tick on entering it, once until that hand has left it by kLeaveMore past it; a fresh squeeze in reach (never one carried
//     into it) held kHold in reach; the game's own reach for it kReachMostRadius at most (ByHand).
//   - A prompt that gives a gun (Point::gives: the reload key's, the AT4's) only with no gun in either hand (Frame::gunHeld: by its grip or its front,
//     flying into one, a take of one under way; free hands asleep: the game's gun in the right hand). The owner, 2026-11-19: "Rifle dropped on the floor
//     first", then "Better to physically drop the rifle and pick up the AT": the rifle dropped or put on a shoulder first, never swallowed from a hand by
//     the scene's kit. With a gun in a hand: no tick, and a squeeze in the prompt's own reach (Reach: at the tube) held kHold (anywhere: by the times a
//     use is asked, Out::how) is refused (Out::refused: the double click, holster.h's, and hands.cpp's line), never asked; held anywhere, a squeeze away
//     from its reach is never the prompt's then (the roof, the player fighting within its 2 m: every magazine and handguard squeeze would have been
//     refused and claimed: the review of babecfd, finding 1), and one owned before a gun came into a hand gives up there (Out::yielded). A door gives no
//     gun: a gun in the other hand is no bar there (the free hand opens it with the rifle in the right hand).
//   - The squeeze that is the prompt's is the use's (spent) till it's let go. The grasp is told the hand is busy (Out::busy: a lying gun's take, the
//     magnet's or a regrip never comes of it, nor of a refused one): "usereach 1", from its first frame (0.1.14's); held anywhere, once its use is asked
//     or refused, and from its first frame for one begun in the prompt's own reach (HandState::kept: a squeeze on the AT4's tube is never the rifle's
//     lying 4-15 cm beside it), till it gives up. If the hand takes something else before its use is asked (it isn't empty any more: a magazine, a
//     grenade, the gun's front), its use is given up (Out::given). Once its use is asked or refused with it (Out::claimed), the hands' other modules take
//     nothing with it either (hands.cpp's g_useOwnsBy: the handguard, the pouch, a magazine, the handle, a holster's draw...: one owner per squeeze, the
//     audit of 2026-11-19); before that (held anywhere) a module's take still wins and the use gives up. A click on the gun's front is the one squeeze
//     two own: its use's (spent) and its hold's again (never claimed nor busy).
#include <cmath>
#include <cstdint>
#include <cstring>

#include "ammozone.h"

namespace usepoint {

constexpr float kDisc = 0.5f;         // m across from the point
constexpr float kBelow = 0.3f;        // m under it (a point a little above the floor, a hand on the floor)
constexpr float kAbove = 1.2f;        // m over it (standing, a hand lowered to the knees)
constexpr float kWiden = 0.2f;        // m wider for each metre up
constexpr float kCapsuleGrow = 0.15f; // m round a measured capsule
constexpr float kLeaveMore = 0.05f;   // m past the reach before a hand in it has left it (its tick isn't said again and again at the edge)
constexpr float kSqueezeOn = 0.6f, kSqueezeOff = 0.4f;  // the grip: squeezed at or past, let go under (the grasp's own: grasp.h takes at kTake or more)
constexpr float kOnLying = 0.03f;     // m: a hand this near a lying gun's box is on it (LyingFirst: a prompt that gives a gun, its target unmeasured)
constexpr float kHold = 0.25f;        // s the squeeze is held in reach ("usereach 1", 0.1.14's rule; held anywhere: the times below)
// Held anywhere, the times a squeeze of a free hand is the prompt's use (or, with no prompt showing, the use key's tap): let go (a click), still and
// clear of the hands' places kStillFor, or held kFreshAsk (the review of 01a5132, finding 1). Each from the squeeze's first frame (HandState::age), which
// is the hands' modules' alone (they run after the use in hands.cpp's FreeHands: a take there leaves the hand not empty the frame after, Out::given).
constexpr float kStillFor = 0.1f;     // s held with the hand still (kStillSpeed, kStillMove) and clear of every module's place (Hand::clear): Ask::kStill
constexpr float kStillSpeed = 0.15f;  // m/s: still, over its last two frames (a reach from rest is past this within 0.05 s, its place 1-2 cm on) ...
constexpr float kStillMove = 0.03f;   // m: ... and within this of where it squeezed
constexpr float kFreshAsk = 0.45f;    // s held, whatever the hand does: past every module's reach window from the squeeze's crossing (grasp.h's, pouch.cpp's,
                                      // rocket.cpp's, m320.cpp's and holster.h's going-on kReachWindow, 0.4 s each): Ask::kHeld
constexpr float kHeldIntoShown = 0.15f;  // s a prompt has shown before a squeeze held from before (begun free) is its (the review of 01a5132, finding 6)
constexpr float kClickGap = 0.4f;     // s: the hand on the gun's front let go of it and squeezed again within this: a click (Out::support)
constexpr float kClickMove = 0.15f;   // m: ... the hand within this of where it let go (a click in place, never a reach on to something else)
constexpr float kFrontWait = 0.1f;    // s: ... its use asked once its hold on the front is taken again (Hand::front), within this; else given up
constexpr float kGapMost = 0.3f;      // s a prompt may go unseen and still be the one showing (the game rescans its target every 0.17-0.20 s)
constexpr float kRetryAfter = 0.5f;   // s before a use not made for a moment is asked again (Rearm) ...
constexpr int kMostRetries = 2;       // ... at most this many times a squeeze
constexpr float kTickAmp = 0.25f, kTickSeconds = 0.02f;  // holster.h's tick
constexpr float kDoubleAmp = 0.5f, kDoubleSeconds = 0.015f, kDoubleGap = 0.06f;  // holster.h's double click (a refusal): the second 60 ms on
// The prompts a hand uses (ByHand): the game's own enum (EntryInputActionEnum, read in the image: pickups.h's kUseReload, kUseInteract), its flag kind.
constexpr int kActionReload = 15, kActionInteract = 35;
constexpr int kFlagKind = 3;          // IET_CTFFlag: its own path (a ray test, no key)
constexpr float kMostRadius = 5.0f;   // m: the game's own UseWithinRadius for it at most (the campaign's E and R prompts' 0.75-3, Kaffarov's HALO start
                                      // 5; the jet's 50-300 and the tank's laser 300 never)
constexpr float kReachMostRadius = 3.0f;  // m: ... by "usereach 1" (0.1.14's, bit for bit: Kaffarov's HALO start not a hand's there)

// A prompt a hand may use: its key's action, its kind, its HoldToInteractTime (s) and its UseWithinRadius (m), as pickups.h's UsePoint has them;
// anywhere: the rule in use (Frame::anywhere: kMostRadius; "usereach 1": kReachMostRadius). pickups.cpp's own checks of an ask take the wider.
inline bool ByHand(int action, int kind, float hold, float radius, bool anywhere = true) {
    return (action == kActionReload || action == kActionInteract) && kind != kFlagKind && hold == 0.0f && std::isfinite(radius) && radius > 0.0f &&
           radius <= (anywhere ? kMostRadius : kReachMostRadius);
}

struct Point {
    bool on = false;           // the game targets it, it's on, not used up, a prompt a hand uses (ByHand); "usereach 1": and its place known
    uint32_t id = 0;           // which (the game's entity: another one shows anew; 0 not known: one prompt)
    float at[3] = {};          // where it is (the room: y up; not a number: not known, Frame::anywhere only)
    float disc = kDisc;        // m across: its own (the AT4's 1.0), else kDisc
    bool capsule = false;      // where the thing lies is measured: the segment a..b of radius r (the room)
    float a[3] = {}, b[3] = {};
    float radius = 0.0f;
    bool gives = false;        // it gives a gun (the reload key's, the game's pickup key: the AT4's): used only with no gun in either hand
};
struct Hand {
    bool tracked = false;      // seen (its pose read this frame)
    float at[3] = {};          // its grip point (the room)
    float squeeze = 0.0f;      // 0-1
    bool empty = false;        // holding nothing (no gun by its grip or front, nothing carried, no module's)
    bool yield = false;        // its squeeze is something else's of the hands' (hands.cpp: a body's place, a gun in a hand near, a lying gun in reach)
    bool clear = false;        // anywhere: clear of every place a module of the hands' takes a squeeze carried to it (hands.cpp's UseClear: a holster's
                               // place, the belly with a gun in a hand, the gun in a hand, a lying gun; each wider than UseYields has it): a still hand's
                               // squeeze there is a click at once (kStillFor); not clear (or not known), its use waits for its let-go or kFreshAsk
    bool nearBody = false;     // anywhere: with a gun in a hand, by the belly or a chest as the holsters keep them (holster::AtBodyTake, the belly grown by
                               // its put-back: 0.1.14's and f1552ad's yield there): a squeeze that has moved since its first frame and comes here is the
                               // body's (a magazine, a round, a rocket: given up, Out::yielded); one of a hand resting here is a click (the owner's nine
                               // squeezes at the cellar's door, 2026-11-26 03:25:39-46, his right hand resting by the belly: BellyTakes for the rest)
    bool front = false;        // anywhere: it holds the gun's front (the handguard, a front grip; the train pistol's left) with the other hand on its pistol
                               // grip, so a let-go of it keeps the gun in that hand: a let-go and a squeeze again soon after is a click (Out::support) ...
    bool otherGrip = false;    // ... while the other hand still holds the gun by its pistol grip (never once the gun has gone from it: dropped, put away)
};
// Whether a click of a free hand with no prompt showing may tap the use key now (Frame::tap, hands.cpp's TapBarOf): kNone it does; else why not.
enum class TapBar : unsigned char {
    kNone,
    kOff,           // not on: pickups.txt "interact 0" or "use 0", "usereach 1", pickups below level 2
    kFight,         // a fist fight runs (both grips the fight's)
    kAsleep,        // free hands asleep (another gun, the train pistol, a seat's gun with none drawn), a new life's first frame, a switch's bridging
    kQuickTime,     // a quick-time trigger is live or a key awaited, however long (a fist fight's six between its prompts too; the grips answer it)
    kAfterUse,      // a prompt's use asked or made within kTapAfterUse (its scene starting: a fight's triggers live, its camera not yet the game's)
    kScripted,      // a scripted moment (the game draws from its own camera, or turns the look itself): its own keys there
    kSeat,          // the player's soldier isn't on foot (its object isn't a ClientSoldierEntity: a vehicle's or a gun's seat, where the key gets off)
    kPoseUnknown,   // whether the soldier is on foot isn't known (its pose isn't read this run, or no soldier)
    kInteractPoint, // the game targets a use point on the interact key that a hand doesn't use (a hold, past 5 m): the key is that one's
};
constexpr float kTapAfterUse = 1.0f;  // s after a prompt's use is asked or made before a click taps the use key (TapBar::kAfterUse)
struct Frame {
    float dt = 0.0f;
    Point point;
    Hand hand[2];
    bool asleep = false;       // nothing now (anywhere: a new life's first frame, a switch's bridging frames, a fist fight's run; "usereach 1": free
                               // hands asleep or a scripted moment too)
    bool gunHeld = false;      // a gun in a hand (hands.cpp: by its grip or its front alone, flying into one, a take or a draw of one under way)
    bool anywhere = true;      // the grip of a free hand anywhere while the prompt shows; false: "usereach 1", 0.1.14's reach rule
    bool cutscene = false;     // anywhere: the game draws from its own camera (xr_runtime.cpp's g_cutscene): no squeeze held from before becomes the
                               // prompt's, and one held through it never does (let go and squeeze again)
    TapBar tap = TapBar::kOff; // anywhere, no prompt showing: a free hand's click taps the use key (kNone), or why not
};
// Why a squeeze the use owned gave up before its use was asked (HandState::gaveUp, GripOf).
enum class GaveUp : unsigned char {
    kNone,
    kTook,       // the hand took something else (Out::given)
    kElse,       // its squeeze became something else's (Hand::yield: Out::yielded)
    kBarred,     // a gun in a hand away from the reach of a prompt that gives one (Out::yielded)
    kFront,      // a click on the gun's front whose hold on the front wasn't taken again (kFrontWait), or the gun left the other hand (Out::yielded)
    kGone,       // its use to be asked again (Rearm) and the prompt it was for no longer shows: another prompt, or the showing over (Out::yielded)
};
// How a hand's use was asked, or its tap made, this frame (Out::how; hands.cpp's "Interact:" lines).
enum class Ask : unsigned char {
    kNone,
    kLetGo,      // as its squeeze was let go (a click), or owed so and asked as the prompt was back (HandState::due)
    kStill,      // held kStillFor with the hand still and clear of the hands' places
    kHeld,       // held kFreshAsk (the hand moving, or by the belly, a shoulder, the gun or a gun lying there)
    kFront,      // a click on the gun's front, its hold taken again
    kAgain,      // asked again after a moment's failure (Rearm)
};
struct HandState {
    bool inReach = false;      // in reach last frame (its tick said; "usereach 1")
    bool down = false;         // the squeeze past kSqueezeOn (till under kSqueezeOff)
    bool owned = false;        // that squeeze is the use's (begun in reach; anywhere: fresh while it shows or in its gap, or held into it): spent till
                               // let go
    bool asked = false;        // ... and its use asked, given up or refused already
    bool claimed = false;      // ... asked or refused with it (not given up): the use's for good till let go (Out::claimed); never a click's on the
                               // gun's front (click)
    bool refused = false;      // ... refused (a gun in a hand at a prompt that gives one): never asked again with it
    bool useAsked = false;     // anywhere: ... its use really asked (Out::use), never given up nor refused (a click let go then may be asked again: Rearm)
    bool click = false;        // anywhere: ... a click of the hand on the gun's front (Out::support): asked once its hold on the front is taken again
    float frontWait = 0.0f;    // anywhere: ... s it has waited for that (kFrontWait)
    bool heldInto = false;     // anywhere: ... held from before into the prompt (Out::heldInto), not a fresh squeeze
    int frames = 0;            // anywhere: frames the squeeze has been down before this one (a let-go asks only after its first, the modules')
    float frontAt = -1.0f;     // anywhere: when the hand last held the gun's front (Hand::front; State::clock, under 0 never) ...
    float frontWhere[3] = {};  // ... and where it was then
    bool tap = false;          // anywhere: the squeeze is the use key's (begun free with no prompt showing, or a click on the gun's front with none): its
                               // tap by the times a prompt's use is asked, once a squeeze
    float startAt[3] = {};     // anywhere: where the hand was at the squeeze's first frame
    bool moved = true;         // anywhere: the squeeze's hand moved (kStillSpeed, kStillMove) or went unread since its first frame (Hand::nearBody) ...
    bool restless = true;      // ... or wasn't clear (Hand::clear): no Ask::kStill with it
    float at1[3] = {}, at2[3] = {};  // anywhere: the hand's place the last two frames read (its speed) ...
    float dt1 = 0.0f;          // ... the last frame's dt
    int track = 0;             // ... frames in a row its place was read (2: its speed known)
    float held = 0.0f;         // s it has been held (in reach: "usereach 1")
    float secondAt = -1.0f;    // a refusal's second pulse due then (State::clock; under 0 none)
    bool beganFree = false;    // anywhere: the squeeze's first frame was a free hand's (tracked, empty, its squeeze nothing else's)
    bool seen = false;         // anywhere: the grip read last frame (a squeeze is fresh only from a let-go read so: never one unread, nor at the start)
    int retries = 0;           // anywhere: its use asked again so far (Rearm)
    float retryAt = 0.0f;      // ... not before then (State::clock)
    uint32_t askedId = 0;      // anywhere: the prompt its use was last asked for (Rearm: again only for that one, while its showing lasts)
    int again = -1;            // anywhere: a click let go whose use was asked (not answered yet): its retries so far (Rearm owes it again); -1 none
    // A click let go with its use owed: let go in the prompt's gap (asked as it's back), or asked and not made for a moment (Rearm, after kRetryAfter).
    // Asked while the same prompt's showing lasts, the hand not squeezed again (a new squeeze is its own click).
    bool due = false;
    uint32_t dueId = 0;        // ... for that prompt (State::id)
    float dueAt = 0.0f;        // ... not before then (State::clock)
    int dueRetries = 0;        // ... its retries (Rearm's count)
    float dueAge = 0.0f;       // ... how long it was held
    float age = 0.0f;          // anywhere: s the squeeze has been down, read (from its first frame)
    bool kept = false;         // anywhere: owned in the prompt's own reach (Reach): the grasp told busy from its first frame (Out::busy), as 0.1.14
    GaveUp gaveUp = GaveUp::kNone;  // anywhere: owned and given up before its use was asked: why
    bool cutHeld = false;      // anywhere: held through a cutscene (Frame::cutscene): never the prompt's held from before
};
struct State {
    HandState hand[2];
    float clock = 0.0f;
    bool anywhere = true;      // the rule last frame's Update went by (Rearm acts only on Frame::anywhere's)
    bool showing = false;      // anywhere: a prompt shows (the game targets it now, or did within kGapMost)
    uint32_t id = 0;           // ... which (Point::id)
    float shownFor = 0.0f;     // ... s it has shown (its gaps not counted)
    float gap = 0.0f;          // ... s since it last showed (0 while it shows)
    bool used = false;         // anywhere: a use asked while it shows (this showing, till it goes past kGapMost) ...
    uint32_t usedId = 0;       // ... of which prompt (Point::id): no squeeze held from before becomes that one's again this showing
    int usedBy = -1;           // ... by that hand's squeeze, while it's held: no squeeze of the other hand becomes that one's meanwhile
};
struct Pulse {
    float amplitude = 0.0f, seconds = 0.0f;
};
struct Out {
    int use = -1;              // the hand whose use is asked now (-1 none)
    bool entered[2] = {};      // that hand's tick this frame: its use asked (anywhere: the hand that does it, the first ask of its squeeze); "usereach
                               // 1": come into reach
    bool spent[2] = {};        // that hand's squeeze is the use's this frame (the quick-time step takes no key in play with it)
    bool busy[2] = {};         // ... the grasp is told the hand is busy (no lying gun's take, magnet or regrip with it): "usereach 1", spent's
                               // (0.1.14's); anywhere, once claimed, or from its first frame in the prompt's own reach (HandState::kept), till it gives up
    bool claimed[2] = {};      // ... and its use asked or refused with it: no other module of the hands' takes anything with it (spent alone: a
                               // module's take still wins, and the use gives up)
    bool given[2] = {};        // that hand's squeeze, the use's, took something else before its use was asked: given up this frame
    bool yielded[2] = {};      // anywhere: ... became something else's (Hand::yield) before its use was asked, or a gun came into a hand with it away
                               // from the reach of a prompt that gives one, or a click on the gun's front its hold not taken again, or the prompt its
                               // use was to be asked again for gone: given up this frame (hands.cpp says which: HandState::gaveUp)
    bool gone[2] = {};         // anywhere: that hand's use, to be asked again after a moment's failure (Rearm), not asked: the prompt it was for shows no
                               // more (another shows, or none): the squeeze still the use's till let go
    bool refused[2] = {};      // that hand's squeeze held kHold of a prompt that gives a gun with a gun in a hand: refused this frame (the double
                               // click's first pulse in pulse; hands.cpp says why)
    bool took[2] = {};         // anywhere: that hand's squeeze became the prompt's this frame ...
    bool heldInto[2] = {};     // ... held from before (begun free), not a fresh squeeze
    bool support[2] = {};      // anywhere: a click of the hand on the gun's front this frame (Hand::front: let go and squeezed again within kClickGap, in
                               // place): its squeeze still the hands' (spent while the prompt's; never claimed nor busy: its hold on the front taken again
                               // with it), its use asked (or, with no prompt showing, the use key tapped) once that hold is
    Ask how[2] = {};           // anywhere: how that hand's use was asked (Out::use) or refused, or its tap made (Out::tap), this frame ...
    float age[2] = {};         // ... s its squeeze had been held then (one let go: as it was let go)
    bool askedHeld[2] = {};    // ... a squeeze held from before into the prompt
    int tap = -1;              // anywhere, no prompt showing: the hand whose click taps the use key now (Frame::tap kNone; one tap for both hands)
    bool tapGone[2] = {};      // ... that hand's click was something else's of the hands' by its tap's time (a module took it, a body's place, the
                               // gun near, a gun lying there; a click on the gun's front its hold not taken again): nothing tapped
    bool tapBarred[2] = {};    // ... that hand's click, free, taps nothing now: Frame::tap says why (hands.cpp's line)
    bool tapTwice[2] = {};     // ... that hand's click came with the other hand's, whose tap is the one
    Pulse pulse[2];            // to send now
};

inline float Dot3(const float* a, const float* b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

// A hand at `at` (the room) is in the point's reach, more metres wider all round (kLeaveMore: still in it). Not finite: never.
inline bool Reach(const Point& p, const float at[3], float more = 0.0f) {
    for (int i = 0; i < 3; ++i)
        if (!std::isfinite(at[i]) || !std::isfinite(p.at[i]))
            return false;
    if (!std::isfinite(more) || more < 0.0f)
        more = 0.0f;
    if (p.capsule) {
        float ab[3], ap[3];
        for (int i = 0; i < 3; ++i)
            ab[i] = p.b[i] - p.a[i], ap[i] = at[i] - p.a[i];
        const float length = Dot3(ab, ab);
        float u = length > 1e-8f ? Dot3(ap, ab) / length : 0.0f;
        u = u < 0.0f ? 0.0f : u > 1.0f ? 1.0f : u;
        float d = 0.0f;
        for (int i = 0; i < 3; ++i) {
            const float c = ap[i] - u * ab[i];
            d += c * c;
        }
        return std::isfinite(p.radius) && std::sqrt(d) <= p.radius + kCapsuleGrow + more;
    }
    const float up = at[1] - p.at[1];
    if (up < -kBelow - more || up > kAbove + more)
        return false;
    const float disc = std::isfinite(p.disc) && p.disc > 0.0f ? p.disc : kDisc;
    const float dx = at[0] - p.at[0], dz = at[2] - p.at[2];
    return std::sqrt(dx * dx + dz * dz) <= disc + kWiden * (up > 0.0f ? up : 0.0f) + more;
}

inline void Clear(State& s) { s = State{}; }

// How near the use's target is to a hand at `at` (the room, m), for the hands' choice between it and a gun lying within the grasp's reach (hands.cpp's
// UseYields; the audit of 2026-11-19: the prompt was judged by the grasp's 25 cm while the grasp reached 35 and the column, a few centimetres deciding
// who took the squeeze). The measured capsule: to its surface (0 inside it). A prompt whose point is where its thing is (its reach the plain kDisc): across
// from the point, its reach's own measure. A prompt whose thing lies off its point unmeasured (its own disc: the AT4's until a census at the prompt fills
// its capsule): -1, no distance known. A number not finite: -1.
inline float TargetDistance(const Point& p, const float at[3]) {
    for (int i = 0; i < 3; ++i)
        if (!std::isfinite(at[i]) || !std::isfinite(p.at[i]))
            return -1.0f;
    if (p.capsule) {
        float ab[3], ap[3];
        for (int i = 0; i < 3; ++i)
            ab[i] = p.b[i] - p.a[i], ap[i] = at[i] - p.a[i];
        const float length = Dot3(ab, ab);
        float u = length > 1e-8f ? Dot3(ap, ab) / length : 0.0f;
        u = u < 0.0f ? 0.0f : u > 1.0f ? 1.0f : u;
        float d = 0.0f;
        for (int i = 0; i < 3; ++i) {
            const float c = ap[i] - u * ab[i];
            d += c * c;
        }
        const float off = std::sqrt(d) - (std::isfinite(p.radius) && p.radius > 0.0f ? p.radius : 0.0f);
        return std::isfinite(off) ? (off > 0.0f ? off : 0.0f) : -1.0f;
    }
    if (p.disc != kDisc)
        return -1.0f;
    const float dx = at[0] - p.at[0], dz = at[2] - p.at[2];
    return std::sqrt(dx * dx + dz * dz);
}
// Whether a gun lying within the grasp's own reach of a hand (gun: its distance by the grasp's measure, m) goes before the use's target for that hand's
// squeeze: only when it is nearer (a tie is the prompt's). With no target distance known (-1): only within pickReach, as before it was measured.
inline bool GunFirst(float gun, float target, float pickReach) {
    if (!(gun >= 0.0f))
        return false;
    if (!(target >= 0.0f))
        return gun < pickReach;
    return gun < target;
}
// One gun lying within the grasp's own reach of a hand against the use's target for that hand's squeeze (hands.cpp's UseYields, each lying gun): box,
// its box's distance from the hand by the grasp's measure (m); within, within the grasp's reach of it (pickReach, or the magnet's sphere for a settled
// hand and a gun at rest); rank, the magnet's column's rank for a hand reaching down over it (under 0: not in its column). The nearer goes first (GunFirst, a
// tie the prompt's). A prompt that gives a gun whose thing lies off its point unmeasured (the AT4's own 1 m disc: TargetDistance -1): the lying gun
// only for a hand on it (its box within kOnLying, 3 cm), never by the column (the integration review of d32a39b, finding 2: the M4A1 let go of 5-15 cm
// beside the tube took a squeeze made on the tube, and a hand 35-60 cm over the rifle on its way down took the rifle where 0.1.14 asked for the AT4; a
// rifle dropped at the tube lands by the hand, so the owner's flow (b) could go round and round; with 8 cm, the review of 075277d, a rifle 1-8 cm
// beside the tube still took 18-20 of 21 grips along it). A rifle dropped onto the tube, or within 3 cm of the hand on it, is still the rifle's till
// the tube's capsule is measured (a census at the prompt: then the nearer goes first). Any other prompt with no target known (its place not known,
// Frame::anywhere): within pickReach, as before.
inline bool LyingFirst(const Point& p, float target, float box, bool within, float rank, float pickReach) {
    if (p.gives && !(target >= 0.0f))
        return within && box >= 0.0f && box < kOnLying;
    if (within)
        return GunFirst(box, target, pickReach);
    if (rank >= 0.0f)
        return GunFirst(rank, target, pickReach);
    return false;
}
// Whether the body's places (the belly and the chests: holster::AtBodyTake) keep a hand's squeeze from this prompt (hands.cpp's UseYields): theirs are
// the pouch's, handmag's, the grenade launcher's, the rocket's, a grenade's. Not for a prompt that gives a gun with no gun in either hand (the
// integration review of d32a39b, finding 7): every one of those wants a gun or a launcher in the hands then, and the chests answer nothing yet, so a
// hand there in the prompt's reach (resting on the lap, standing at the roof's AT4) had no tick and no use, and its squeeze took nothing. With a gun in
// a hand that prompt is barred yet owns a fresh squeeze in its reach (Update: its refusal claims at kHold, inside the pouch's 0.4 s window): the body
// keeps it then, so a squeeze at the belly for a magazine is the magazine's, never the double click.
inline bool BodyFirst(const Point& p, bool gunHeld) { return !(p.gives && !gunHeld); }
// ... as the rule in use has it (Frame::anywhere): any prompt, held anywhere, by finding 7's reason: with no gun in either hand nothing at the body takes a
// squeeze (the pouch's magazines, the launchers' rounds and rockets want a gun or a launcher in the hands; the chests answer nothing yet), so a hand
// resting at the belly at a door is the door's; with a gun in a hand the body keeps it (a magazine, a round). "usereach 1": BodyFirst, as 0.1.14.
inline bool BodyKeeps(const Point& p, bool gunHeld, bool anywhere) { return anywhere ? gunHeld : BodyFirst(p, gunHeld); }
// Where a squeeze at the body is the belly's (held anywhere, with a gun in a hand: BodyKeeps; fromNeck from the neck point in the body's axes, right, up,
// forward): inside the belly's own zone, where its takes are (pouch.cpp's magazine, m320.cpp's round, rocket.cpp's rocket, shellload.cpp's shell: each
// takes at ammozone::Inside, ungrown, within its 0.4 s). Not the zone grown by its put-back (where a let-go goes back, never where one is taken), nor
// the holsters' keep-out round it, nor the chests (nothing takes a squeeze there yet): holster::AtBodyTake's, which "usereach 1" keeps (0.1.14's). The
// owner at the cellar's door, 2026-11-26 03:25:39-46, the rifle in his left, squeezed his free right grip nine times where it rested, 1.13 to 1.57 of
// the belly's size from its middle: eight were kept from the door by AtBodyTake's grown belly at their first frame, the ninth given up 12 ms after it
// became the door's, and the pouch took none ("outside it"). Not finite: false.
inline bool BellyTakes(const float fromNeck[3]) {
    return std::isfinite(fromNeck[0]) && std::isfinite(fromNeck[1]) && std::isfinite(fromNeck[2]) && ammozone::Inside(fromNeck);
}

// What bars a free hand's click from tapping the use key now (Frame::tap), from the game's moment as hands.cpp reads it; the first that holds.
struct TapIn {
    bool on = false;            // the interact tap on: held anywhere, pickups.txt "interact" and "use" not 0, pickups at level 2
    bool fight = false;         // a fist fight runs (input::QteFightLive)
    bool awake = false;         // free hands awake, not a new life's first frame, nor a switch's bridging frames
    bool quickTime = false;     // a quick-time trigger live or a key awaited, however long (input::QteAnyLive: a fist fight's six triggers between its
                                // prompts, a frame they couldn't be read; the review of 01a5132, finding 7: the cellar's fight's triggers were live
                                // 0.35 s before its scripted camera, and QteKeyInPlay lets go after 10 s of a key)
    bool afterUse = false;      // a prompt's use asked and not answered, or asked or made within kTapAfterUse (hands.cpp's g_useAsked, g_useLastAt)
    bool scripted = false;      // the game draws from its own camera or turns the look itself (a cutscene, a scripted turn)
    int onFoot = -1;            // the player's soldier: 1 on foot (a ClientSoldierEntity), 0 not (another object: a seat), -1 not known (input::OnFoot)
    bool interactPoint = false; // the game targets a use point on the interact key that a hand doesn't use (a hold, past 5 m, a flag)
};
inline TapBar TapBarOf(const TapIn& in) {
    if (!in.on)
        return TapBar::kOff;
    if (in.fight)
        return TapBar::kFight;
    if (!in.awake)
        return TapBar::kAsleep;
    if (in.quickTime)
        return TapBar::kQuickTime;
    if (in.afterUse)
        return TapBar::kAfterUse;
    if (in.scripted)
        return TapBar::kScripted;
    if (in.onFoot == 0)
        return TapBar::kSeat;
    if (in.onFoot != 1)
        return TapBar::kPoseUnknown;
    if (in.interactPoint)
        return TapBar::kInteractPoint;
    return TapBar::kNone;
}
inline const char* TapWord(TapBar b) {
    switch (b) {
    case TapBar::kNone: return "the use key tapped";
    case TapBar::kOff: return "the grip's use key is off (pickups.txt interact 0 or use 0, or usereach 1)";
    case TapBar::kFight: return "a fist fight runs (both grips are the fight's)";
    case TapBar::kAsleep: return "free hands asleep (another gun, the train's pistol, a seat), a new life's first frame or a weapon switch";
    case TapBar::kQuickTime: return "a quick-time event is live (its triggers, or a key awaited: the grips answer it)";
    case TapBar::kAfterUse: return "a prompt was used a moment ago (its scene starting)";
    case TapBar::kScripted: return "a scripted moment (the game's own camera or turn: its own keys)";
    case TapBar::kSeat: return "the soldier isn't on foot (a seat: the use key would get off)";
    case TapBar::kPoseUnknown: return "whether the soldier is on foot isn't known (its pose isn't read)";
    case TapBar::kInteractPoint: return "the game shows a prompt on the use key a hand doesn't use (a hold, or far off): press Y";
    }
    return "";
}
// Whether p is within metres of q (both the room). Not finite: false.
inline bool Within(const float p[3], const float q[3], float metres) {
    float d = 0.0f;
    for (int i = 0; i < 3; ++i) {
        if (!std::isfinite(p[i]) || !std::isfinite(q[i]))
            return false;
        d += (p[i] - q[i]) * (p[i] - q[i]);
    }
    return std::sqrt(d) <= metres;
}

// A refusal's double click for that hand (holster.h's: two short pulses kDoubleGap apart): the first now (returned), the second sent by Update when
// it's due. Also for one refused outside Update (hands.cpp's: X held's R at a prompt that gives a gun, with a gun in a hand).
inline Pulse Refuse(State& s, int side) {
    if (side < 0 || side > 1)
        return Pulse{};
    s.hand[side].secondAt = s.clock + kDoubleGap;
    return Pulse{kDoubleAmp, kDoubleSeconds};
}

// Whether a use not made failed for a moment, worth asking again while its squeeze is held (Rearm), by the words pickups.cpp gives (RequestUse's,
// ServeUse's, UseOutcome's): the game didn't target it at its next tick, it came too late or wasn't taken in a second (a stall: never made), the soldier
// wasn't awake. Never used up, not on, the game's own checks refusing it, its use not as read or faulted, off, its outcome not heard (perhaps made),
// another use asked or waiting (a second use could kick a door twice).
inline bool Transient(const char* why) {
    static const char* const kTransient[] = {"the game doesn't target it now", "too late", "the soldier isn't awake", "not answered in 1 s"};
    if (!why)
        return false;
    for (const char* t : kTransient)
        if (std::strcmp(why, t) == 0)
            return true;
    return false;
}

// That hand's use, asked and not made for a moment (Transient: hands.cpp's NoteUse), asked again kRetryAfter on, kMostRetries times a squeeze at most,
// only for the prompt it was asked for while that one's showing lasts (HandState::askedId: the review of 01a5132, finding 3, a squeeze held on through a
// failed use used the next prompt that showed, unasked): while its squeeze is still held (claimed meanwhile: nothing else of the hands' takes it; a click
// on the gun's front stays the hands'), or for a click let go before its answer came, owed (HandState::due: asked while the hand isn't squeezed again).
// Frame::anywhere's rule only. False: none to ask again (never asked, refused or given up, the retries spent, the prompt gone or another showing,
// "usereach 1").
inline bool Rearm(State& s, int side) {
    if (side < 0 || side > 1 || !s.anywhere)
        return false;
    HandState& h = s.hand[side];
    if (!s.showing || s.id != h.askedId)
        return false;
    if (h.down) {
        if (!h.owned || !h.asked || !h.useAsked || !(h.claimed || h.click) || h.refused || h.retries >= kMostRetries)
            return false;
        h.asked = false;
        ++h.retries;
        h.retryAt = s.clock + kRetryAfter;
        return true;
    }
    if (h.again < 0 || h.again >= kMostRetries || h.due)
        return false;
    h.due = true;
    h.dueId = h.askedId;
    h.dueAt = s.clock + kRetryAfter;
    h.dueRetries = h.again + 1;
    h.again = -1;
    return true;
}

namespace detail {

// "usereach 1": 0.1.14's step, bit for bit (the tests' t6 rows).
inline void ReachStep(const Frame& f, State& s, Out& out, float dt) {
    // A prompt that gives a gun with a gun in a hand: no hand uses it (the owner, 2026-11-19: "Better to physically drop the rifle and pick up the AT").
    const bool barred = f.point.gives && f.gunHeld;
    for (int side = 0; side < 2; ++side) {
        HandState& h = s.hand[side];
        const Hand& in = f.hand[side];
        if (h.secondAt >= 0.0f && s.clock >= h.secondAt - 1e-4f) {  // (a refusal's second pulse, whatever else this frame)
            out.pulse[side] = Pulse{kDoubleAmp, kDoubleSeconds};
            h.secondAt = -1.0f;
        }
        const float squeeze = in.tracked && std::isfinite(in.squeeze) ? in.squeeze : 0.0f;
        const bool wasDown = h.down;
        // (Down at kSqueezeOn, as the grasp takes at its kTake: with "past" alone a frame reading exactly 0.6 let the grasp take a lying gun before the
        // use owned the squeeze; the integration review of d32a39b.)
        h.down = h.down ? squeeze >= kSqueezeOff : squeeze >= kSqueezeOn;
        const bool fresh = h.down && !wasDown;
        if (!h.down)
            h.owned = h.asked = h.claimed = false, h.held = 0.0f;
        const bool live = !f.asleep && f.point.on && in.tracked;
        const bool reach = live && Reach(f.point, in.at);
        // A hand free for it: holding nothing, its squeeze nothing else's. The tick: into reach (a hand holding something, or at something else of the
        // hands', isn't reaching for it; nor one while the prompt is barred: the tick says "hold the grip here"), out of it only kLeaveMore past it; the
        // bar lifted with the hand still in reach (the rifle let go of, or put away): the tick then.
        const bool free = in.empty && !in.yield;
        const bool usable = free && !barred;
        if (reach && !h.inReach && usable && out.pulse[side].seconds <= 0.0f) {  // (a refusal's second pulse now: the tick the frame after)
            out.entered[side] = true;
            out.pulse[side] = Pulse{kTickAmp, kTickSeconds};
        }
        h.inReach = h.inReach ? usable && live && Reach(f.point, in.at, kLeaveMore) : out.entered[side];
        // A fresh squeeze in reach by a free hand is the use's till it's let go (barred too: its refusal, never another thing's).
        if (fresh && reach && free)
            h.owned = true;
        if (!h.owned)
            continue;
        out.spent[side] = out.busy[side] = true;  // (the grasp told busy from its first frame, as 0.1.14 did)
        if (h.asked) {
            out.claimed[side] = h.claimed;
            continue;
        }
        if (!in.empty) {  // (it took something else first: a magazine, a grenade, the gun's front, a gun given to it)
            h.asked = true;
            out.given[side] = true;
            continue;
        }
        h.held = reach ? h.held + dt : 0.0f;  // (out of reach: the hold starts again once it's back, the squeeze still the use's)
        if (h.held >= kHold && out.use < 0) {
            h.asked = true;
            h.claimed = true;
            out.claimed[side] = true;
            if (barred) {  // (as the hold ends: a gun let go of, or put away, within it still lets the use be asked)
                out.refused[side] = true;
                out.pulse[side] = Refuse(s, side);
            } else {
                out.use = side;
            }
        }
    }
}

// Frame::anywhere's step: a click of the grip of a free hand while the prompt shows; with none showing, a free hand's click taps the use key.
inline void AnywhereStep(const Frame& f, State& s, Out& out, float dt) {
    // The prompt showing: the game targets it now, nothing asleep. Gone kGapMost at most, it's still the one showing (the squeezes it owns kept, none
    // asked meanwhile: asked as it's back); another prompt shows anew (the squeezes the use's stay so). Gone past kGapMost: the showing over, and what
    // was used in it forgotten.
    const bool shows = f.point.on && !f.asleep;
    if (shows) {
        if (s.showing && f.point.id != s.id)
            s.shownFor = 0.0f;
        s.showing = true;
        s.id = f.point.id;
        s.gap = 0.0f;
        s.shownFor += dt;
    } else if (s.showing) {
        s.gap += dt;
        if (s.gap > kGapMost + 1e-4f) {
            s.showing = false;
            s.id = 0;
            s.shownFor = s.gap = 0.0f;
            s.used = false, s.usedId = 0, s.usedBy = -1;
        }
    }
    // The prompt showing used already this showing (State::used): no squeeze held from before becomes its again, nor the other hand's squeeze while the
    // one that used it is held (the review of babecfd, finding 3: two grips, or one held from before, kicked a door open and shut again).
    const bool usedThis = s.used && s.showing && s.usedId == s.id;
    // A prompt that gives a gun with a gun in a hand: no hand uses it (the owner, 2026-11-19: "Better to physically drop the rifle and pick up the AT").
    const bool barred = f.point.gives && f.gunHeld;
    // The grasp told that hand is busy (Out::busy): once its use is asked or refused, and from its first frame for a squeeze begun in the prompt's own
    // reach, till it gives up; never a click on the gun's front (never claimed: its hold's again).
    const auto busy = [](const HandState& x) { return x.claimed || (x.kept && !x.asked); };
    // That hand's use asked now, how (one a frame: the other hand's waits), or refused (a prompt that gives a gun, a gun in a hand, a squeeze in its own
    // reach): the double click. The tick on the hand that does it, at its first ask (a refusal's second pulse due now: none). A click on the gun's front
    // stays the hands' (never claimed).
    const auto ask = [&](int side, HandState& h, Ask how) {
        h.asked = true;
        h.claimed = !h.click;
        h.askedId = s.id;
        out.claimed[side] = h.claimed;
        out.busy[side] = busy(h);
        out.how[side] = how;
        out.age[side] = h.age;
        out.askedHeld[side] = h.heldInto;
        if (barred) {
            h.refused = true;
            out.refused[side] = true;
            out.pulse[side] = Refuse(s, side);
            return;
        }
        h.useAsked = true;
        out.use = side;
        if (!h.retries && out.pulse[side].seconds <= 0.0f) {
            out.entered[side] = true;
            out.pulse[side] = Pulse{kTickAmp, kTickSeconds};
        }
    };
    // Given up before its use was asked, why (Out::given, Out::yielded); one claimed already (its use to be asked again) only stays the use's.
    const auto giveUp = [&](int side, HandState& h, GaveUp why) {
        h.asked = true;
        if (!h.claimed) {
            h.gaveUp = why;
            if (why == GaveUp::kTook)
                out.given[side] = true;
            else
                out.yielded[side] = true;
        } else if (why == GaveUp::kGone) {
            out.gone[side] = true;
        }
        out.busy[side] = busy(h);
    };
    // That hand's squeeze taps the use key now, how (no prompt showing): the first hand's this frame; the other's then one with it (Out::tapTwice); a bar
    // of the moment's, nothing (Out::tapBarred).
    const auto tapNow = [&](int side, HandState& h, Ask how) {
        h.tap = false;
        out.how[side] = how;
        out.age[side] = h.age;
        if (out.tap >= 0)
            out.tapTwice[side] = true;
        else if (f.tap != TapBar::kNone)
            out.tapBarred[side] = true;
        else
            out.tap = side;
    };
    for (int side = 0; side < 2; ++side) {
        HandState& h = s.hand[side];
        const Hand& in = f.hand[side];
        if (h.secondAt >= 0.0f && s.clock >= h.secondAt - 1e-4f) {  // (a refusal's second pulse, whatever else this frame)
            out.pulse[side] = Pulse{kDoubleAmp, kDoubleSeconds};
            h.secondAt = -1.0f;
        }
        h.inReach = false;
        // Not read (its controller unseen a moment, the headset off, the stand-in's): nothing is known of the grip: its state kept, nothing counted,
        // never a let-go; read again, a squeeze then is none fresh (as one at the state's first frame, after a new life: held from before, begun no
        // one knows where). Its tap: none (a grip not read never taps the use key); still: no more (where it went meanwhile not known).
        if (!in.tracked || !std::isfinite(in.squeeze)) {
            h.seen = false;
            h.tap = false;
            h.moved = h.restless = true;
            h.track = 0;
            if (h.owned)
                out.spent[side] = true, out.claimed[side] = h.claimed, out.busy[side] = busy(h);
            continue;
        }
        // Its place, and its speed over its last two frames (kStillSpeed): not known till its place has been read two frames in a row.
        const bool placed = std::isfinite(in.at[0]) && std::isfinite(in.at[1]) && std::isfinite(in.at[2]);
        float speed = -1.0f;
        if (placed) {
            if (h.track >= 2 && dt + h.dt1 > 1e-5f) {
                float d = 0.0f;
                for (int i = 0; i < 3; ++i)
                    d += (in.at[i] - h.at2[i]) * (in.at[i] - h.at2[i]);
                speed = std::sqrt(d) / (dt + h.dt1);
            }
            std::memcpy(h.at2, h.at1, sizeof(h.at2));
            std::memcpy(h.at1, in.at, sizeof(h.at1));
            h.dt1 = dt;
            h.track = h.track < 2 ? h.track + 1 : 2;
        } else {
            h.track = 0;
        }
        if (in.front) {  // (the gun's front in that hand: when and where, for its click)
            h.frontAt = s.clock;
            std::memcpy(h.frontWhere, in.at, sizeof(h.frontWhere));
        }
        const float squeeze = in.squeeze;
        const bool wasDown = h.down;
        h.down = h.down ? squeeze >= kSqueezeOff : squeeze >= kSqueezeOn;  // (down at kSqueezeOn, as the grasp takes at its kTake)
        const bool fresh = h.down && !wasDown && h.seen;
        const bool letGo = wasDown && !h.down;
        h.seen = true;
        // A hand free for it: seen, holding nothing, its squeeze nothing else's.
        const bool free = in.empty && !in.yield;
        const bool reachNow = Reach(f.point, in.at);  // (the prompt's own reach: its place known)
        // Let go. The prompt's squeeze not asked yet: a click, its use asked as it's let go (past its first frame, the modules'; the hand free then:
        // nothing of the hands' had it), or owed as the same prompt is back (let go in its gap, or before a moment's failure's time: HandState::due);
        // the other hand's use this frame is this one's too. The use key's squeeze: a click, tapped so. A click on the gun's front let go before its
        // hold was taken again: nothing.
        if (letGo) {
            const bool freeHere = free && !(h.moved && in.nearBody);  // (carried by the belly or a chest with a gun in a hand: the body's)
            if (h.owned && !h.asked && !h.click && h.frames >= 1 && freeHere && !(barred && !reachNow) && out.use < 0) {
                if (shows && s.clock >= h.retryAt - 1e-4f)
                    ask(side, h, h.retries ? Ask::kAgain : Ask::kLetGo);
                else if (s.showing && !barred)
                    h.due = true, h.dueId = s.id, h.dueAt = h.retryAt, h.dueRetries = h.retries, h.dueAge = h.age;
            }
            if (h.tap && !h.click) {
                if (!freeHere)
                    h.tap = false, out.tapGone[side] = true;
                else if (h.frames >= 1 && !s.showing)
                    tapNow(side, h, Ask::kLetGo);
            }
            h.again = h.useAsked && !h.refused ? h.retries : -1;  // (its use asked: owed again if it fails for a moment, Rearm)
        }
        if (!h.down) {
            h.owned = h.asked = h.claimed = h.refused = h.useAsked = h.beganFree = h.kept = h.cutHeld = h.click = h.heldInto = h.tap = false;
            h.held = h.age = h.frontWait = 0.0f;
            h.retries = 0, h.frames = 0;
            h.retryAt = 0.0f;
            h.moved = h.restless = true;
            h.gaveUp = GaveUp::kNone;
            if (s.usedBy == side)
                s.usedBy = -1;  // (the squeeze that used it let go: the other hand's may use it again)
        } else {
            h.age += dt;  // (from its first frame)
        }
        // The squeeze's first frame: where it began, and whether it began free (its squeeze may become the prompt's held from before; never one held in
        // or through a cutscene: a fresh squeeze in one may, hands.cpp makes its use by the prompt's key). A click owed: a new squeeze is its own.
        if (fresh) {
            h.beganFree = free;
            h.moved = h.restless = false;
            std::memcpy(h.startAt, in.at, sizeof(h.startAt));
            h.due = false;
            h.again = -1;
        }
        if (f.cutscene && h.down && !h.owned)
            h.beganFree = false, h.cutHeld = true;
        // Still since its first frame: under kStillSpeed over its last two frames, within kStillMove of where it squeezed (else it moved: by the belly
        // with a gun in a hand, the body's, Hand::nearBody); and clear of the places a module of the hands' takes a squeeze carried there (Hand::clear):
        // Ask::kStill. Once not, never for this squeeze.
        if (h.down) {
            const bool still = placed && speed >= 0.0f && speed < kStillSpeed && Within(in.at, h.startAt, kStillMove);
            if (!still)
                h.moved = true;
            if (!still || !in.clear)
                h.restless = true;
        }
        const bool atBody = h.moved && in.nearBody;  // (a squeeze carried by the belly or a chest with a gun in a hand: the body's)
        // The prompt's: a fresh squeeze of a free hand while it shows or in its gap; one held from before, begun free and free now, once it has shown
        // kHeldIntoShown (no cutscene, not a prompt used this showing); a click of the hand on the gun's front (let go of it within kClickGap and squeezed
        // again in place, holding nothing else; never at a prompt that gives a gun with a gun in a hand). Never the other hand's while the squeeze that
        // used this prompt is held; with a prompt that gives a gun and a gun in a hand, only in its own reach (its refusal there, the double click;
        // elsewhere every squeeze is the hands'). With no prompt showing (nor within its gap), nothing asleep: a fresh squeeze of a free hand, or a click
        // on the gun's front, is the use key's (HandState::tap); a prompt shown meanwhile has it, held into it (begun free).
        if (h.tap && s.showing)
            h.tap = h.click = false;
        const bool otherUsing = usedThis && s.usedBy >= 0 && s.usedBy != side;
        const bool frontClick = fresh && !in.front && in.empty && in.otherGrip && h.frontAt >= 0.0f && s.clock - h.frontAt <= kClickGap + 1e-4f &&
                                Within(in.at, h.frontWhere, kClickMove);
        const bool heldInto = !fresh && h.beganFree && !f.cutscene && !usedThis && shows && s.shownFor >= kHeldIntoShown - 1e-4f;
        if (!h.owned && !h.tap && h.down) {
            if (frontClick && s.showing && !otherUsing && !barred) {
                h.owned = h.click = true;
                h.kept = false;
                h.frontWait = 0.0f;
                out.took[side] = out.support[side] = true;
            } else if (frontClick && !s.showing && !f.asleep) {
                h.tap = h.click = true;
                h.frontWait = 0.0f;
                out.support[side] = true;
            } else if (free && !atBody && !otherUsing && !(barred && !reachNow) && ((fresh && s.showing) || heldInto)) {
                h.owned = true;
                h.kept = reachNow;
                h.heldInto = !fresh;
                out.took[side] = true;
                out.heldInto[side] = !fresh;
            } else if (fresh && free && !atBody && !s.showing && !f.asleep) {
                h.tap = true;
            }
        }
        if (h.owned) {
            out.spent[side] = true;
            out.claimed[side] = h.claimed;  // (asked again after a moment's failure: still claimed meanwhile)
            out.busy[side] = busy(h);
        }
        // Not asked yet: given up if its squeeze is something else's now (it took something else first: a magazine, a grenade, the gun's front, a gun given
        // to it; a holster's place, a draw or a put-away there, the belly with a gun in a hand, near the gun, a lying gun nearer; a gun come into a hand
        // with it away from the reach of a prompt that gives one), or its use was to be asked again and the prompt it was for shows no more. Else asked by
        // its time, the prompt showing (in a gap: kept, asked as it's back): held kFreshAsk, or kStillFor still and clear (or let go: above). A click on the
        // gun's front once its hold on the front is taken again (the hands' Support with this squeeze, last frame), the gun still in the other hand by its
        // pistol grip; given up if the hand takes something else, or that hold isn't taken within kFrontWait.
        if (h.owned && !h.asked) {
            if (h.retries && (!s.showing || s.id != h.askedId)) {
                giveUp(side, h, GaveUp::kGone);
            } else if (h.click) {
                if (!in.otherGrip) {
                    giveUp(side, h, GaveUp::kFront);
                } else if (in.front) {
                    if (shows && out.use < 0 && s.clock >= h.retryAt - 1e-4f)
                        ask(side, h, h.retries ? Ask::kAgain : Ask::kFront);
                } else if (!in.empty) {
                    giveUp(side, h, GaveUp::kTook);
                } else if ((h.frontWait += dt) > kFrontWait + 1e-4f) {
                    giveUp(side, h, GaveUp::kFront);
                }
            } else if (!in.empty) {
                giveUp(side, h, GaveUp::kTook);
            } else if (!h.claimed && (in.yield || atBody || (barred && !reachNow))) {
                giveUp(side, h, in.yield || atBody ? GaveUp::kElse : GaveUp::kBarred);
            } else if (shows && out.use < 0 && s.clock >= h.retryAt - 1e-4f) {
                if (h.retries)
                    ask(side, h, Ask::kAgain);
                else if (h.age >= kFreshAsk - 1e-4f)
                    ask(side, h, Ask::kHeld);
                else if (!h.restless && h.age >= kStillFor - 1e-4f)
                    ask(side, h, Ask::kStill);
            }
        }
        // The use key's squeeze: tapped by a prompt's times (held kFreshAsk, kStillFor still and clear; let go: above), a click on the gun's front once its
        // hold is taken again; nothing if something of the hands' takes it first (Out::tapGone).
        if (h.tap) {
            if (h.click) {
                if (!in.otherGrip || (!in.front && !in.empty))
                    h.tap = false, out.tapGone[side] = true;
                else if (in.front)
                    tapNow(side, h, Ask::kFront);
                else if ((h.frontWait += dt) > kFrontWait + 1e-4f)
                    h.tap = false, out.tapGone[side] = true;
            } else if (!free || atBody) {
                h.tap = false, out.tapGone[side] = true;
            } else if (h.age >= kFreshAsk - 1e-4f) {
                tapNow(side, h, Ask::kHeld);
            } else if (!h.restless && h.age >= kStillFor - 1e-4f) {
                tapNow(side, h, Ask::kStill);
            }
        }
        // A click owed (let go in the prompt's gap, or its use not made for a moment): asked as the same prompt shows, its time come, the hand holding
        // nothing; dropped once that showing is over, another prompt shows, the hand holds something, or a gun in a hand bars the prompt.
        if (h.due && !h.down) {
            if (!s.showing || s.id != h.dueId || !in.empty || barred) {
                h.due = false;
            } else if (shows && out.use < 0 && s.clock >= h.dueAt - 1e-4f) {
                h.due = false;
                h.askedId = s.id;
                h.again = h.dueRetries;
                out.how[side] = h.dueRetries ? Ask::kAgain : Ask::kLetGo;
                out.age[side] = h.dueAge;
                out.use = side;
                if (!h.dueRetries && out.pulse[side].seconds <= 0.0f) {
                    out.entered[side] = true;
                    out.pulse[side] = Pulse{kTickAmp, kTickSeconds};
                }
            }
        }
        if (h.down)
            ++h.frames;
    }
    // A use asked: the other hand's squeeze owned and not asked yet is that use's too (asked and claimed, a click on the front not: one use, both
    // squeezes its own till let go; a click of it owed, dropped), and this showing's prompt used (State::used) by the squeeze that asked it, while it's
    // held.
    if (out.use >= 0) {
        const int other = 1 - out.use;
        HandState& o = s.hand[other];
        if (o.owned && !o.asked) {
            o.asked = true;
            o.claimed = !o.click;
            o.askedId = s.id;
            out.spent[other] = true;
            out.claimed[other] = o.claimed;
            out.busy[other] = busy(o);
        }
        if (o.due && o.dueId == s.id)
            o.due = false;
        s.used = true, s.usedId = s.id, s.usedBy = s.hand[out.use].down ? out.use : -1;
    }
}

}  // namespace detail

// Once a frame (hands.cpp's FreeHands, before the grasp: its busy hands, Out::busy, are told the grasp so).
inline void Update(const Frame& f, State& s, Out& out) {
    out = Out{};
    const float dt = std::isfinite(f.dt) && f.dt > 0.0f && f.dt < 0.25f ? f.dt : 0.0f;
    s.clock += dt;
    s.anywhere = f.anywhere;
    if (f.anywhere)
        detail::AnywhereStep(f, s, out, dt);
    else
        detail::ReachStep(f, s, out, dt);
}

// Why a hand doesn't answer the prompt showing now, after Update (hands.cpp's "Use grips:" lines: each grip's state and, if it isn't answering, why).
enum class Grip : unsigned char {
    kUnseen,        // not tracked or not read (its controller asleep, the stand-in's, the headset off)
    kOpen,          // not squeezed (under kSqueezeOn)
    kHolds,         // it holds something (the gun by its grip or front, a magazine, a round, the handle): a free hand's click answers, or the hand on the
                    // gun's front let go and squeezed again (Out::support)
    kElse,          // its squeeze is something else's (a holster's place, the belly with a gun in a hand, near the gun, a lying gun nearer)
    kBeganElse,     // held from before, begun where something else had it (a body's place, near the gun) or with the hand not free: let go, squeeze again
    kSettling,      // held from before, begun free: the prompt's once it has shown kHeldIntoShown
    kHolding,       // the prompt's, its use asked by its time (still and clear kStillFor, let go, or held kFreshAsk)
    kWaitRetry,     // the prompt's, its use to be asked again (a moment's failure)
    kAsked,         // its use asked (or made): one a squeeze
    kRefused,       // refused: a gun in a hand at a prompt that gives one
    kGiven,         // it took something else first: given up
    kNotShowing,    // the prompt doesn't show (a gap, asleep)
    kYielded,       // the prompt's, then something else's before its use was asked (a holster's place, the belly with a gun in a hand, the gun): given up
    kBarred,        // a gun in a hand and a prompt that gives one: away from its own reach no squeeze is its (at it, refused)
    kOtherUsed,     // the other hand's squeeze used this prompt and is held: no squeeze of this one is its meanwhile
    kUsedHeld,      // held from before, this prompt used while it shows: let go and squeeze again
    kCutscene,      // held from before in a cutscene, or held through one: let go and squeeze again
    kFrontWait,     // a click on the gun's front, the prompt's: its use once the hand holds the front again (kFrontWait)
    kFrontLost,     // a click on the gun's front whose hold on the front wasn't taken again, or the gun left the other hand: given up
};
inline Grip GripOf(const Frame& f, const State& s, int side) {
    if (side < 0 || side > 1)
        return Grip::kUnseen;
    const Hand& in = f.hand[side];
    const HandState& h = s.hand[side];
    if (!in.tracked || !std::isfinite(in.squeeze))
        return Grip::kUnseen;
    if (!h.down)
        return Grip::kOpen;
    if (h.owned) {
        if (h.refused)
            return Grip::kRefused;
        if (h.asked)
            return h.claimed || h.useAsked                 ? Grip::kAsked
                   : h.gaveUp == GaveUp::kElse             ? Grip::kYielded
                   : h.gaveUp == GaveUp::kBarred           ? Grip::kBarred
                   : h.gaveUp == GaveUp::kFront            ? Grip::kFrontLost
                   : h.click && h.gaveUp == GaveUp::kGone  ? Grip::kAsked
                                                           : Grip::kGiven;
        if (s.clock < h.retryAt - 1e-4f)
            return Grip::kWaitRetry;
        return h.click ? Grip::kFrontWait : Grip::kHolding;
    }
    if (!in.empty)
        return Grip::kHolds;
    if (in.yield)
        return Grip::kElse;
    if (!f.point.on || f.asleep)
        return Grip::kNotShowing;
    if (!f.anywhere)
        return h.beganFree ? Grip::kSettling : Grip::kBeganElse;
    const bool usedThis = s.used && s.showing && s.usedId == s.id;
    if (usedThis && s.usedBy >= 0 && s.usedBy != side)
        return Grip::kOtherUsed;
    if (f.point.gives && f.gunHeld && !Reach(f.point, in.at))
        return Grip::kBarred;
    if (h.cutHeld || f.cutscene)
        return Grip::kCutscene;
    if (!h.beganFree)
        return Grip::kBeganElse;
    return usedThis ? Grip::kUsedHeld : Grip::kSettling;
}
inline const char* GripWord(Grip g) {
    switch (g) {
    case Grip::kUnseen: return "not seen (its controller asleep, the stand-in's, or the headset off)";
    case Grip::kOpen: return "let go";
    case Grip::kHolds: return "its hand holds something (the gun by its grip or its front, a magazine, a round, the handle): a free hand's click answers, "
                              "or the hand on the gun's front let go and squeezed again";
    case Grip::kElse: return "its squeeze is something else's";
    case Grip::kBeganElse: return "held from a squeeze begun where something else had it, or with the hand holding something: let go and squeeze again";
    case Grip::kSettling: return "held from before: the prompt's once it has shown 0.15 s";
    case Grip::kHolding: return "the prompt's: its use asked once the hand is still away from the belly, the shoulders and the gun (0.1 s), as it's let go, "
                                "or 0.45 s on";
    case Grip::kWaitRetry: return "the prompt's, its use asked again in a moment";
    case Grip::kAsked: return "its use asked (one a squeeze: let go and squeeze again for another)";
    case Grip::kRefused: return "refused: a gun in a hand (drop it or put it on a shoulder first)";
    case Grip::kGiven: return "it took something else first (let go and squeeze again)";
    case Grip::kNotShowing: return "squeezed, the prompt not showing this moment";
    case Grip::kYielded: return "the prompt's, then something else's before its use (a holster's place, the gun, the belly with a gun in a hand, a gun on "
                                "the ground): let go and squeeze again away from them";
    case Grip::kBarred: return "a gun in a hand: this prompt gives one, so drop it or put it on a shoulder first (a squeeze at the prompt itself is refused, "
                               "with a double click)";
    case Grip::kOtherUsed: return "the other hand's squeeze used this prompt: let go of both and squeeze again for another";
    case Grip::kUsedHeld: return "held from before, and this prompt was used while it shows: let go and squeeze again for another";
    case Grip::kCutscene: return "held in or through a cutscene: let go and squeeze again";
    case Grip::kFrontWait: return "let go of the gun's front and squeezed again: its use once the hand holds the front again";
    case Grip::kFrontLost: return "let go of the gun's front and squeezed again, but the hand didn't take the front again (or the gun left the other hand): "
                                  "let go and squeeze again";
    default: return "";
    }
}

}  // namespace usepoint

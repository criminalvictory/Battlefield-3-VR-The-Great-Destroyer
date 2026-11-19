#pragma once
#include <openxr/openxr.h>

#include "tracking.h"

namespace input {

// SteamVR's extension for the Steam Frame's controllers; enabled when the runtime offers it.
constexpr const char* kFrameControllerExtension = "XR_VALVE_frame_controller_interaction";

// Creates the controller actions and suggests bindings for the common controller profiles
// (SteamVR maps them onto any controller, Steam Frame included). Call once after the session is
// created. Returns false if controllers can't be used; the mod still works without them.
bool Init(XrInstance instance, XrSession session);

// Once per frame: reads the controllers, locates the right-hand aim pose in space, and turns
// buttons and sticks into keyboard/mouse input for BF3 (only while its window has focus).
void Sync(XrSession session, XrSpace space, XrTime time);
// Lets go of everything held down for the game: for when Sync won't be running (SteamVR gone).
void LetGo();
// Each frame (xr_runtime.cpp): the game's scripted moment as the mod sees it, for the quick-time events on the grips (qte.h): cutscene, the game
// draws from its own camera; turn, it turns the look yaw itself (aim::Scripted); turnArmed, that turn began with a gun in hand (aim::ScriptedArmed).
// In one (a cutscene's camera, or a turn begun with no gun in hand) a fresh squeeze of either grip holds E, F, Space, Z and both mouse buttons
// down till it's let go or the moment is over; Sync takes it the next frame.
void SetScripted(bool cutscene, bool turn, bool turnArmed);
// weapon.txt "qte 0/1/2": the grips' answer to quick-time events: 0 none; 1 version 1, the above; 2 (the default) version 2: a fresh squeeze of
// either grip presses exactly the keys the game's live input triggers await (qtewatch.cpp), tapped or held as each trigger reads its input.
void SetQte(int version);
// weapon.txt "qtemash 0/1": version 2's tapped keys again every 130 ms while the grip stays squeezed (1, the default) or once a squeeze (0).
void SetQteMash(bool on);
// weapon.txt "qtefight 0/1": a fist fight's open prompt read from its prefab's gates (1, the default: qtefight.h), its one key pressed alone and every
// other quick-time key the mod would send held up while it's open; 0: a fight's six quick-time triggers press nothing (never all six at once).
void SetQteFight(bool on);
// weapon.txt "qtetest 1.5": a test: the left grip as let go for 0.3 s, then squeezed that many seconds, by the same rules (it presses only in a
// scripted moment); with nobody in the headset too, with the stand-in on and the game in front. 0 stops it. With version 2: for that many
// seconds the grip is let go, and squeezed from 0.3 s after the game awaits a key until it awaits none.
void StartQteTest(float seconds);

// The right controller's aim pose from the last Sync, if it's being tracked.
bool AimPose(XrPosef& out);

// A hand's grip pose (0 left, 1 right) from the last Sync, in the same space, if it's tracked.
bool HandPose(int hand, XrPosef& out);
// Its pose follows its own reading as the runtime gave it: not held where it was through a moment out of sight, nor within 3 frames of a jump taken
// out (weapon.txt "steadyhands"; the jump's fade after it still runs); with the steadying off, whenever it's tracked.
bool HandSettled(int hand);
// Its controller's own speed (m/s) and turn (rad/s, 0 if not given), in that space, as the runtime gives them from its motion sensors: false if it doesn't.
bool HandVelocity(int hand, float* linear, float* angular);
// Its controller's tracking record as the last Sync left it (tracking.h): seen, guessed by the runtime or no place, read raw (not HandPose's 0.5 s hold
// of a hand lost for a moment); where and when it was last seen and its speed then; the runtime's last guess; "not watched" with the headset off or
// out of focus. Also the reach test's "Tracking:" lines. A copy; false for another hand. Render thread.
bool TrackingOf(int hand, tracking::Record& out);
// A buzz in that hand's controller (0 left, 1 right): amplitude 0-1, seconds, hertz (0: the runtime's own). Render thread (the session's).
// Nothing when the haptics weren't bound (a profile that refused them) or the session isn't focused. A new one replaces one still going.
void Buzz(int side, float amplitude, float seconds, float hertz = 0.0f);
// Holsters' draw (hands.cpp): the game's own switch to that slot by its key, held as a pick's (0 key 1, 1 key 2, 5 key 4; ammo::Picked told); false: no key (another
// slot, bf3 not in front, the grenade launcher's shot under way). Not a pick (LastPickAt stays); Y held's toggle then gives the other gun. Render thread.
bool SelectSlot(int slot);
// The user's last pick, never SelectSlot's (GetTickCount64 ms, 0 none): the D-pad, Touch's menu, Y held, X held's R for a pickup each frame; *slot 0, 1, 5, -1 X held's; *picks counts.
unsigned long long LastPickAt(int* slot = nullptr, unsigned* picks = nullptr);

// The stand-in hand (weapon.txt "standin 1"): each frame after Sync, with the head's pose. While
// it's on and the right controller isn't tracked, AimPose and HandPose(1) give a right hand holding
// the rifle out in front, aimed where the head looks, to test the gun with nobody in the headset.
// pitchDegrees ("standin 1 45"): with the headset asleep (headless frames), the head looks that far
// up, so test shots go into the sky.
// yawDegrees ("standin 1 45 25"): the gun aims that far right of the head (to see its side).
// shift ("standin 1 45 0 -0.25 -0.15 0"): the hold moved (right, up, back; metres).
void SetStandIn(bool on, float pitchDegrees = 0.0f, float yawDegrees = 0.0f, const float* shift = nullptr);
void SetStandInUp(float degrees);
float StandInPitch();  // radians
// weapon.txt "zoom 1": the game's zoom (right mouse) held while it's set.
void SetZoomHold(bool on);
// weapon.txt "steadyhands 0": the controllers' tracking jumps left in.
void SetSteadyHands(bool on);
// weapon.txt "forcehand 1": a test: the stand-in hand holds the gun even with the right controller awake.
void SetForceHand(bool on);
// Sprint is held (the left stick's click), or was let go of less than a second ago: the game's
// hands hold its gun differently then (skin.cpp doesn't copy them). Prone too, and just after the stick changes the stance.
bool Sprinting();
// Each frame, headset on or off: the grenade launcher under the rifle's shot (its pick, fire and the rifle back), with the game in front.
void UpdateM320();
// The game's eye above the soldier's feet in the stance the mod last put it in (1.6 m standing, 1.0 crouched, 0.3 prone): where a dropped thing's floor is till the real one's known.
float StanceEyeHeight();
// That stance: 0 standing, 1 crouched, 2 prone.
int StanceIndex();
// weapon.txt "m320shot": as a pull of the left trigger with the hand on the launcher's grip (a test).
void TestM320Shot();
// The game's own reload of the grenade launcher under way (picked to be reloaded, R held): the reload by hand stands aside (m320.h Frame::gameReload).
bool M320GameReloading();
// Its shot or game reload under way from the pull (picked, firing, reloading, the rifle coming back): the wrist's flick doesn't open it then (m320.h Frame::shotUnderway).
bool M320ShotUnderway();
// One more each time the grenade launcher's round left (UpdateM320Shot's "fired"). Any thread.
unsigned M320Fired();
// How far the right trigger is pulled (0-1): the glove's trigger finger follows it.
float RightTrigger();
// weapon.txt "fingertest 1": the trigger finger drawn as if pulled that far (nothing fires).
void SetTriggerTest(float pull);
// A film round's trigger (build\freehands.txt "test N", hands.cpp's scripted hands) for a hand it moves (0 left, 1 right), each frame it
// moves it: how far it's pulled (0-1). With nobody in the headset no buttons are read, so this alone then works the game's fire button, by
// the rules a controller's pull goes by (the hand on the rifle's pistol grip fires it: hands::FireHand); a frame without it is that trigger
// let go. Only with the stand-in on (weapon.txt "standin 1") and the game in front; with the headset's session focused it does nothing.
void SetFilmTrigger(int hand, float pull);
// weapon.txt "filmtrigger 2 1 3": a film's pulls of a trigger (0 left, 1 right; the right by default), held 2 s each, 3 of them (1 by default),
// 0.6 s let go between, given as SetFilmTrigger's each frame: the same fire button by the same rules (the trigger's stop at an empty magazine
// included). Only with the stand-in's head looking 45 degrees down or more (real rounds); held off without it, dropped after 10 s of that or
// once its pulls have begun; "filmtrigger 0" stops it. Each pull said.
void StartFilmPull(float seconds, int hand, int pulls);
void StandIn(const XrPosef& head);
bool StandingIn();
// The stand-in's left hand this frame (the left controller asleep too): a pose like HandPose(0)'s,
// under the handguard, only to be drawn; HandPose(0) doesn't give it.
bool StandInLeftPose(XrPosef& out);
// The stand-in is switched on (weapon.txt "standin 1"), whether or not it holds the gun now.
bool StandInOn();

// The left glove's fingers: how far the grip is squeezed (0-1, the middle, ring and little
// fingers), how far the trigger is pulled (0-1, the index finger), and the thumb resting on the
// stick. All three: a fist.
float LeftSqueeze();
float LeftTrigger();
bool LeftThumbOnStick();
// The same by hand (0 left, 1 right), for free hands: each hand's own grip and trigger hold or let go of the gun.
// How far that hand's grip is squeezed (0-1); Squeeze(0) == LeftSqueeze().
float Squeeze(int hand);
// Its grip was read this frame: the headset's session focused (the sync worked) and its action active (bound,
// the controller awake), whether or not BF3 has focus. False: nothing is known of it, so it must never count as a let-go.
bool SqueezeRead(int hand);
// How far its trigger is pulled (0-1), read before the focus check.
float Trigger(int hand);
// Its thumb resting on the stick; ThumbOnStick(0) == LeftThumbOnStick().
bool ThumbOnStick(int hand);
// The aim pose that hand's controller would give for this grip pose: 1 the learned aim-to-grip (build\standin.txt),
// 0 the same mirrored across x (exact for mirror-image controllers). False before it's known.
bool AimFromGrip(int hand, const XrPosef& grip, XrPosef& aim);
// The game's own reload (R, held for X held): 0 while R is held, else the ms since it was let go; ~0ULL never.
unsigned long long GameReloadSince();
// The left View button (Steam Frame) was pressed since the last call: line up the left glove.
bool TakeCalibratePress();

// X tapped (F16 from tools/drawhunt.ps1 does the same): the magazine out, dropping to the floor,
// or with it out a new one in. With a gun the mod doesn't know the magazine of, the game's reload.
// fromScript (F16): with the magazine out it always puts one in, whatever pouch.txt "xinsert" says.
void MagazineTap(bool fromScript = false);
// A tap with the magazine out puts none in (it's taken over the left shoulder and pushed in by hand:
// hands.cpp; pouch.txt "xinsert 1" brings the tap back); holding X is still the game's reload.
void SetTapInsert(bool on);

// F17 (from tools/drawhunt.ps1): for 3 s, turns the way the right stick held all the way does,
// to measure how even the turn is without the controller.
void StartTurnTest();
// Once per frame, after Sync: the smooth turn from the right stick (or the F17 test), only while
// BF3 has focus. direct: the frame's turn is taken with TakeTurn and written into the look yaw
// with the head's (aim.cpp), which the picture follows; otherwise a thread moves the mouse.
void UpdateTurn(bool direct);
// The direct turn since the last call: radians, left positive ([controllers] turn_degrees).
float TakeTurn();
// The right stick's click held 2.5 s since the last call: the view is to be recentered (as F9).
bool TakeRecenter();
// weapon.txt "turnby 30": the view turned that many degrees at once, right positive (to find the
// sun for the lens flare with nobody in the headset).
void TurnBy(float degrees);
// Each frame: how far the head is turned from the way the game faces (radians, left positive).
// The left stick walks where the head faces, so it's turned by this into the game's frame.
void SetMoveTurn(float radians);
// The turn's evenness from frame to frame, logged once a second while the stick turns: which 0 =
// the yaw of the game's own camera as built, 1 = the yaw shown. Radians.
void NoteTurnStep(int which, float yaw);

}

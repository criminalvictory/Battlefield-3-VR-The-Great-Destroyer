#pragma once

namespace aim {

// Head or controller tracking by way of the player's look angles: each stereo frame, the change
// in yaw since the last frame is added (the mouse turns the body too), and pitch is set outright.
// BF3 then turns, culls, renders and aims from them itself.
// headYaw: radians, turning left is positive. headPitch: radians, looking up is positive.
// turn: the right stick's smooth turn for this frame (radians, left positive), added with it.
// False while the angles aren't known: they're looked for from the player's soldier (each new
// life or checkpoint) in the background, and nothing is written until then.
// body: the picture's body yaw, the game yaw the room's straight ahead shows (radians, the look yaw's sense;
// xr_runtime.cpp), or nullptr. Wherever the look yaw starts again (the angles found, a scripted moment over,
// mouse aim after a pause or a cutscene) it starts from there, so the view goes on and the game's camera turns
// to the gun.
bool Update(float headYaw, float headPitch, float turn = 0.0f, const float* body = nullptr);

// Forget the last head angles (mode switch, recenter), so the next Update doesn't jump: it starts
// again from the game's own yaw (a recenter turns the hand and the head together).
void Reset();

// Drops the look angles found, so they're looked for again: after the game drew from its own camera (a
// cutscene, the jump onto the train in The Great Destroyer; unturned, or a scripted camera turned to the
// head), the place they were found can hold something else, and writing there crashed the game 2 s after
// the jump (2026-10-31).
void Forget();

// weapon.txt "mouseaim 0/1": the view steered with the mouse while the look angles aren't found (on by default).
void SetMouseAim(bool on);

// weapon.txt "aimfind 0/1" (aim.cpp, src/aimrules.h): the search matching the camera at each window of its walk, one hold of mouse aim a pass, a pair
// proved as it is after its wait, what proved itself kept per soldier with the take it came after, the pass by the pitch alone held at most 0.6 s a
// search; mouse aim's last step; a level view not turned while mouse aim steers; the pitch limits learned only where the game left the yaw alone,
// forgotten at each find and tried again. On by default; 0: all as before. Logged when it changes.
void SetFind(bool on);

// True while this frame's mouse aim held its corrections for the search's nudge of a candidate: the game's
// facing then keeps the stick's turn but not the hand's (xr_runtime.cpp turns the walk from where it was).
bool Nudging();

// Whether the game is turning the look yaw itself (a scripted moment): nothing written meanwhile.
bool Scripted();
// ... and that moment began with a gun in hand: perhaps none at all, but the game reading a look slot nothing writes (input.cpp's quick-time
// events take no squeeze for a prompt's answer then).
bool ScriptedArmed();

// weapon.txt "scripted 0/1": whether a scripted moment (the game turning the look yaw itself) stops the
// writes until it's over (on by default; aim.cpp).
void SetScriptedFollow(bool on);

// weapon.txt "aimtest 20": for 2 s from 1 s after, TestTurn() is that many degrees (radians), added
// to the hand's yaw, to check the view's turn without moving the controllers.
void StartTest(float degrees);
float TestTurn();

// The player's look yaw as BF3 stores it (radians, atan2(x, -z) of the camera's backwards row),
// and which way it moves for a controller turn to the left (+1 or -1). False if unreadable.
bool LookYaw(float& radians, float& sign);

// The look angles as Update last wrote them (radians as the game stores them: yaw 0 to 2 pi, growing turning right; pitch up positive). False
// before any write, and while none is written (mouse aim, a scripted moment, the angles not found). pitchHeld: how far the game's own pitch
// limits (as learned) held that pitch back from the one asked for (radians, 0 none). Render thread.
bool LastWritten(float& yaw, float& pitch, float* pitchHeld = nullptr);
// The weapon camera as the game last built it (camera::GameWeaponAngles) less the look it was built from: of the looks in the slot over the last
// few frames (each one written, and each one found there before the write), the one nearest the camera. So the game's build falling between two
// of the mod's writes is never read as a deviation: with nothing of its own added, the camera is exactly one of them. Radians, camera minus look
// (the yaw wrapped, a vehicle's turn taken out). False with no look written lately, or no camera built since the last call. Render thread.
bool WeaponDeviation(float& yaw, float& pitch);
// The look yaws Update wrote lately, as the game's camera has them (a vehicle's turn added: what LookYaw gave after each write), the newest first, up to most of them (8 are kept): one a frame,
// in the order written, so one that no view build ever took is among them. The game's eye stands on one of them, most often not the newest: the one its last tick read (headmove.h, the eye's
// arm). counts: each one's number among all the writes so far (one more with each write), or nullptr. 0 while none is written (mouse aim, a scripted moment, a cutscene, the angles not found).
// Any thread.
int WrittenYaws(float* yaws, int most, unsigned* counts = nullptr);

// Logs about once a second where the yaw/pitch pointers lead and how many head moves were written.
void Probe();

// F4 / F2: flip which way a head turn / tilt moves the view, saved to bf3vr.ini.
void FlipYaw();
void FlipPitch();

}

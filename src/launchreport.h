#pragma once

struct ID3D11Texture2D;

// The launch report (the user, 2026-11-14, after a player on a Quest 3 wrote "The game launches to the menu but freezes in
// the game state"): it says in plain words, step by step, whether the VR launch worked, on any PC and headset (it only reads
// what the game and the VR runtime report). Every line starts with LAUNCH; they go into build\bf3vr.log and, alone, into
// build\launch.log (the short file to send). Each step is written the moment it happens; a step that doesn't come in time
// gets a PROBLEM line with what to try, so a launch that hangs shows the last step it reached and the one that never came.
// Step 8 is the level: the player's soldier there (the game's own state), the headset getting pictures, and the picture not
// black (a small brightness reading of the game's picture once a second, 9 patches of 8x8 pixels, read back a second later).
// LAUNCH WATCH lines say when the game stops handing the mod new pictures (a long load, or a freeze) and where the drawing
// thread was then. Its own thread, waking once a second.
namespace launchreport {

enum Step { kModIn, kGameDraws, kRuntime, kSession, kRunning, kFirstFrame, kFocused, kInLevel, kStepCount };

// Where the game's drawing thread is, as the mod's frame work goes (for the watch's lines).
enum Where { kInGame, kWaitingForHeadset, kHandingOver };

void Begin();                                    // once, from the mod's start thread: the clock, launch.log and the thread
// Once, from the mod's start thread after bf3vr.ini is read: other programs loading themselves from the game's folder (conflicts.h: ReShade, DXVK,
// a proxy dxgi.dll...) named, a LAUNCH PROBLEM line each with what to do, or a line saying none are. on false ([general] folder_report=0): the
// folder not looked through, said.
void GameFolder(bool on);
void Ok(Step step, const char* fmt, ...);        // the step reached (the first call counts, the rest are ignored)
void Problem(Step step, const char* fmt, ...);   // the step failed outright (told once)
void Note(Step step, const char* fmt, ...);      // why the step isn't there yet: told with its PROBLEM line if it's late
void GameDrew();                                 // each new picture the game hands the mod (Present)
void HeadsetFrame();                             // each frame handed to the headset with a picture in it
void Inside(Where where);                        // the drawing thread's place, around the waits on the headset
void Warn(const char* fmt, ...);                 // a "LAUNCH PROBLEM: ..." line of no step (what the player should know and send)
void Line(const char* fmt, ...);                 // a "LAUNCH: ..." line of no step (worth knowing in a report, not a problem)
void Vdxr();                                     // the game runs on Virtual Desktop's own runtime (VDXR): the hints name it, not SteamVR
bool IsVdxr();
// Each VR frame, on the render thread: whether the player is in a level (the player's soldier is there), and the game's
// picture (its back buffer) for the brightness reading, taken at most once a second.
void Picture(ID3D11Texture2D* back, bool inLevel);

}  // namespace launchreport

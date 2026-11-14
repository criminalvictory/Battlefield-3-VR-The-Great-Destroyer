#pragma once

// The launch report (the user, 2026-11-14, after a player on a Quest 3 wrote "The game launches to the menu but freezes in
// the game state"): build\bf3vr.log says in plain words, step by step, whether the VR launch worked. Every line of it starts
// with LAUNCH, so a player can find them. Each step is written the moment it happens, and a step that doesn't come in time
// gets a PROBLEM line with what to try: a launch that hangs shows the last step it reached and the one that never came.
// From the game's first picture on, LAUNCH WATCH lines say when the game stops handing the mod new pictures for a while
// (a long load, or a freeze) and where the drawing thread was then: waiting for the headset, handing a picture over, or
// in the game itself. Its own thread, waking once a second; nothing here touches the game or OpenXR.
namespace launchreport {

enum Step { kModIn, kGameDraws, kRuntime, kSession, kRunning, kFirstFrame, kFocused, kStepCount };

// Where the game's drawing thread is, as the mod's frame work goes (for the watch's lines).
enum Where { kInGame, kWaitingForHeadset, kHandingOver };

void Begin();                                    // once, from the mod's start thread: the clock and the report's thread
void Ok(Step step, const char* fmt, ...);        // the step reached (the first call counts, the rest are ignored)
void Problem(Step step, const char* fmt, ...);   // the step failed outright (told once)
void Note(Step step, const char* fmt, ...);      // why the step isn't there yet: told with its PROBLEM line if it's late
void GameDrew();                                 // each new picture the game hands the mod (Present)
void HeadsetFrame();                             // each frame handed to the headset with a picture in it
void Inside(Where where);                        // the drawing thread's place, around the waits on the headset

}  // namespace launchreport

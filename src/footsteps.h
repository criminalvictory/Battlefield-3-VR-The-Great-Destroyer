#pragma once

// The player's footsteps (the user, 2026-11-05: "the sounds of walking, sprinting"): the game plays its own, from its first-person walk and run animations' step tags
// (Sound/Character/Soldier_Movement_Footsteps_SP, read from its data). Whether they still play in VR, and how often, before anything's changed: weapon.txt "steplog 1"
// counts, once a second, the step tags its animation enters and the step events that reach a sound, and the footsteps' start and stop. Nothing's hooked till it's asked for.
namespace footsteps {

void SetLog(bool on);
// Once a frame (render thread): hooks once asked, and the once-a-second line.
void OnFrame();

}

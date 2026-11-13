#pragma once
#include <cstdint>
// A dropped gun's attachments drawn where it lies (the user, 2026-11-13: "Show it on the ground"): the game draws a dead soldier's gun on the ground
// with its bare third-person body, whatever its unlocks; this adds each attachment's own third-person mesh, made with the game's own mesh-instance
// calls on the game's thread inside the game's own builder of that body, placed on the body's pose by the gun's own socket data, and freed with the
// game's own pair when the body goes. Nothing of the game's is written: no unlock, no attachment, no body. build\pickups.txt "parts N": 0 or no
// line off (nothing hooked, nothing read, nothing called), 1 each drop's unlocks and parts named in the log and the three hooks counting only, 2 one
// sight on one pickup, 3 the sights on every pickup, 4 every part with a picture but magazines. The pickups level 0 (off) means parts 0.
namespace pickparts {

// Render thread, each frame (pickups.cpp's OnFrame): the level in force (pickups' own, 0 off), pickups.txt's parts (-1 none), whether pickups'
// own checks of the game's code passed. The checks of the code it goes through, the hooks, the census in the log, the counts in the log.
void OnFrame(unsigned long long now, int level, int parts, bool pickupsVerified);
// The game's thread (pickups.cpp's tick of the player's soldier): the pass that reaches pickups built before the hooks, and the drain when the
// switch goes down, only on the thread the game builds its pickups on.
void GameTick(unsigned long long now);
// The parts live now (the self-test's and pickups' log).
int Live();

}  // namespace pickparts

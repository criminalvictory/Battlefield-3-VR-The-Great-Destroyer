#pragma once
// build\freehands.txt (beside the DLL): free hands' switch and its test hooks, re-read whole when it
// changes (checked every 300 ms), so deleting it puts every line back to its default: nothing sticks.
//   "off"                free hands off: the right hand holds the rifle as before
//   "test N"             a test round with nobody in the headset (freetest.h: 1, 2 or 3); none by default
//   "rightpose 0"        the free right glove is the game's grip carried at the right controller
//   "lefthold dx dy dz"  the rifle in the left hand shifted (metres, in the left grip's axes)
//   "lefthold lineup"    the rifle in the left hand put where the left glove already is
//   "rightwidth w s"     the free right glove's palm width and how much of their width its fingers keep (1 1)
//   "glide s"            seconds a pickup or a regrip takes into the hand (0.12)
//   "reach p r"          metres a hand reaches: to the rifle's box to pick it up, to its pistol grip to regrip (0.25 0.15)

namespace freetune {
void Read();                          // hands::Update, each frame (the file's time checked every 300 ms; deleted: all defaults)
bool On();                            // no "off" line
int Test();                           // "test N", 0 none
bool RightPose();                     // no "rightpose 0"
bool LeftHold(float shift[3]);        // "lefthold dx dy dz" (m, the left grip's axes); true for "lefthold lineup"
void RightWidth(float& width, float& slim);  // "rightwidth w s", default 1 1
float Glide();                        // "glide s", default 0.12
void Reach(float& pick, float& regrip);      // "reach p r", default 0.25 0.15
}

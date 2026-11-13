#pragma once
// build\freehands.txt (beside the DLL): free hands' switch and its test hooks, re-read whole when it
// changes (checked every 300 ms), so deleting it puts every line back to its default: nothing sticks.
// Each works on whatever gun is in hand.
//   "off"                free hands off: the right hand holds the gun as before
//   "test N"             a test round with nobody in the headset (freetest.h: 1 to 12); none by default
//   "rightpose 0"        the free right glove is the game's grip carried at the right controller
//   "lefthold dx dy dz"  the gun in the left hand shifted (metres, in the left grip's axes)
//   "lefthold lineup"    the gun in the left hand put where the left glove already is
//   "rightwidth w s"     the free right glove's palm width and how much of their width its fingers keep (1 1)
//   "glide s"            seconds a pickup or a regrip takes into the hand (0.12)
//   "reach p r"          metres a hand reaches: to the gun's box to pick it up, to its pistol grip to regrip (0.25 0.15)
//   "guns m416"          free hands on the M416 alone: every other gun held by the right hand as before ("guns all": every gun, the default)
//   "twosided 1"         a gun lying drawn with both faces of every triangle (a first-person mesh may lack the sides first person never shows)
//   "magnet s d w g u"   a gun on the ground drawn to a fresh squeeze within s m of its box, or, for a hand at least g m under the neck point
//                        (outside the belly), from up to d m below it or u m above it, w m wider around a metre down (0.35 1.2 0.20 0.50 0.70);
//                        "magnet 0": off, a pickup within "reach" alone as before; "magnet 1": on with its defaults (on with no line)
//   "snap p m s"         a magnet take's glide: p s more a metre past "reach", m s at most; s its sound: 0 none, 1 the player's own guns,
//                        2 also the game's dropped guns (0.10 0.25 1)
//   "lowsqueeze 1"       the magnet's column only for a squeeze closed with the hand already lowered (by default one closed on the way down,
//                        within its 0.4 s, takes too)
//   "dpadeject N"        the Steam Frame's left D-pad right: 1 X's job with the gun in the left hand by its pistol grip (the default,
//                        also any number but 0 or 2), 0 nothing, 2 X's job whoever holds the gun; X itself works as before in every case

namespace freetune {
void Read();                          // hands::Update, each frame (the file's time checked every 300 ms; deleted: all defaults)
bool On();                            // no "off" line
int Test();                           // "test N", 0 none
bool RightPose();                     // no "rightpose 0"
bool LeftHold(float shift[3]);        // "lefthold dx dy dz" (m, the left grip's axes); true for "lefthold lineup"
void RightWidth(float& width, float& slim);  // "rightwidth w s", default 1 1
float Glide();                        // "glide s", default 0.12
void Reach(float& pick, float& regrip);      // "reach p r", default 0.25 0.15
bool EveryGun();                      // no "guns m416" line
bool TwoSided();                      // "twosided 1"
// "magnet s d w g u": on (no "magnet 0"), the sphere, the column's reach down and up, its widening, the gate (metres)
void Magnet(bool& on, float& sphere, float& down, float& up, float& widen, float& gate);
void Snap(float& perMetre, float& most);  // "snap p m s": a magnet take's glide (0.10 0.25)
int SnapSound();                      // ... s: 0 none, 1 the player's own guns (the default), 2 also the game's dropped guns
bool LowSqueeze();                    // "lowsqueeze 1"
int DpadEject();                      // "dpadeject N": 0, 1 (the default) or 2 (ejectkey.h); safe from any thread
}

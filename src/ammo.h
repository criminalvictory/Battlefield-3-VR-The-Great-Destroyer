#pragma once

#include <cstdint>

namespace pickups {
struct Swap;
}

namespace ammo {

// The rounds in the gun in hand and the spare ones, as the game's own server counts them: that
// count decides whether the gun fires and when it reloads, and the HUD's copy follows it. The gun
// in hand is the one in HandSlot's slot. Render thread. False when the soldier or the gun can't be found.
bool Get(int& loaded, int& spare);

// The soldier's slot (0 or 1) the gun in the hands is in, as Update last worked it out (pickrules::HandSlot): the first for the M416, the
// Stinger, the SMAW and the train pistol; a gun taken up from the ground in whichever of the two the game put it. Every count of the gun in
// hand (Get, Eject, Insert, the hidden round, the trigger's stop) goes to it, and recoil counts its rounds in it. Any thread.
int HandSlot();
// The game told to raise slot 0 or 1 (input's pick, key 1 or 2): two guns of one kind, or one the guns' table doesn't know, are counted, after
// the game's words, in the slot last picked or taken into, whichever came later (HandSlot). Anything else is no pick. Any thread.
void Picked(int slot);

// The rounds in the gun in hand as the game counts them, to tell shots by, on any gun: the train's pistol isn't on
// Get's path. Only ever reads. Render thread.
bool RoundsInHand(int& loaded);
// Something in the soldier's slot 1, the second key's: on Rock and a Hard Place, the Stinger once it's been taken up.
bool SecondaryCarried();

// The magazine came out: one round stays in the chamber, the rest go back to the spare rounds.
void Eject();

// A new magazine went in: as full as the spare rounds allow, plus the chambered round.
void Insert();

// Every frame: notes how full the gun gets (a magazine plus a chambered round), which gives the
// magazine's size for Insert.
void Update();

// That size: 1 for a gun loaded a round at a time (the SMAW, the grenade launcher).
int MagazineSize();

// The rifle the mod knows is in hand with its own firing state, not the grenade launcher under it
// (which holds a round at a time): a rifle magazine may go in. Render thread.
bool RifleFiringInHand();

// The launcher's own count (the SMAW, loaded by the left hand: rocket.cpp): whatever's in hand, as the mod counts it (without its
// hidden round: Update gives it one while it's in hand and hands.cpp loads it by hand, so the game never finds it empty and
// never reloads it by itself). False when its firing state can't be found. Render thread.
bool LauncherGet(int& loaded, int& spare);
// A rocket went in by hand, from the spare ones.
void LauncherInsert();
// The game's own reload for it (X held with it empty): its hidden round out, so the game finds it empty and reloads it itself.
// Update gives it its hidden round again once that's done.
void LauncherGameReload();
// Its count at none and no hidden round in it: left so, the game reloads it by itself.
bool LauncherAtNone();
// Whether Update may give the launcher its hidden round (hands.cpp, each frame: it's in hand and loaded by hand).
void SetLauncherByHand(bool on);

// The Stinger reloaded by the left hand (flap.cpp): its count as the mod has it, and which gun it is (a new life, another one). A missile in from the spare ones; or, none left, its true count, for the game to go back to the
// rifle by itself. False when it can't be found. Render thread.
bool StingerGet(int& loaded, int& spare, unsigned& which);
void StingerLoad();
void StingerOut();

// The grenade launcher under the rifle (the M320, slot 2), as Update last read it: in the game's hand (its firing state's +0xA8: 0 never been, 3 put away, in hand anything else:
// 1 coming up, 2 ready, 5 firing, 10 and 11 reloading; read 2026-11-05 on Rock and a Hard Place), and its rounds. Any thread.
bool M320Up();
int M320State();  // (-1 none found)
bool M320Carried();
// In hand, or put away within the last 1.5 s (the game's hands still on their way back).
bool M320UpOrRecent();
// Its rounds are the mod's count (M320Loaded): the game's is kept at 1, or it won't let it be put away empty. Its own reload: the game's count left at 0 till it has reloaded
// it (false: not as read, or no spare). A round put in by hand (the reload by hand): one from the spare.
bool M320GameReload();
void M320LoadByHand();
// A live round out of it again (tipped out of the open barrel, pulled out): back to the spare ones; the game's count stays at 1.
void M320Unload();
int M320Loaded();
int M320Spare();

// For recoil (recoilrun.cpp), only ever read: the firing state of the weapon in that slot of the player's soldier (0-7), 0 none (its +0x94 is the
// gun's own WeaponFiringData); that slot's rounds count as the game keeps it (the mod's hidden round in it too). Render thread.
uintptr_t FiringOf(int slot);
bool RoundsInSlot(int slot, int& loaded);
// One more for every rounds count the mod writes (Eject, Insert, the hidden rounds, the launchers', the grenade launcher's, the Stinger's, Refill,
// Empty, the spare rounds kept up): a count that went down while this moved isn't shots. Any thread.
unsigned CountWrites();
// The count a weapon of the player's should carry into a pickup the game swaps it into, as the mod counts it: the game's count less the mod's hidden
// round in it (the rifle's, a held gun's, the launchers'); the grenade launcher under the rifle the mod's own count (the game's is kept at 1). False
// if that slot has no weapon with a firing state. Render thread.
bool HandOverCount(int slot, uint32_t& firing, int& loaded);
// A swap the game made (pickups.h; pickups::OnFrame calls it before Update meets the new gun). Every firing state in it is forgotten, those that went
// and those that came (their objects are new; one at an old address is another gun). The gun in hand is forgotten only if its slot (HandSlot's) is
// among the swap's or its firing state went, the grenade launcher only if its slot (2) is or its firing state went (pickrules::Forgets): the new gun's
// path then runs once, and a swap that leaves a slot alone leaves its gun as it was. An outgoing slot-0 or slot-1 gun's magazine-out state is kept by
// the pickup it went into and its name, and given back to it when it comes out of that pickup again; the slot the take put its gun into is kept for
// HandSlot. Said once a swap. Render thread.
void Swapped(const pickups::Swap& swap);

// Testing (weapon.txt "refill"): the gun in hand gets 300 spare rounds. Render thread.
void Refill();

// Testing (weapon.txt "empty"): no rounds left in the gun in hand, so the game's own reload (R)
// is its empty one, bolt release and all, to record its sounds from. Render thread.
void Empty();

}

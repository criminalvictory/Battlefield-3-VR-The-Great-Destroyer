#pragma once

namespace ammo {

// The rounds in the gun in hand and the spare ones, as the game's own server counts them: that
// count decides whether the gun fires and when it reloads, and the HUD's copy follows it.
// Render thread. False when the soldier or the gun can't be found.
bool Get(int& loaded, int& spare);

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

// Testing (weapon.txt "refill"): the gun in hand gets 300 spare rounds. Render thread.
void Refill();

// Testing (weapon.txt "empty"): no rounds left in the gun in hand, so the game's own reload (R)
// is its empty one, bolt release and all, to record its sounds from. Render thread.
void Empty();

}

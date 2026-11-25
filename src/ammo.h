#pragma once

#include <cstdint>

#include "pistolammo.h"
#include "shotgunammo.h"

namespace pickups {
struct Swap;
}
namespace recoilguns {
struct Row;
}

namespace ammo {

// The rounds in the gun in hand and the spare ones, as the game's own server counts them: that
// count decides whether the gun fires and when it reloads, and the HUD's copy follows it. The gun
// in hand is the one in HandSlot's slot. Render thread. False when the soldier or the gun can't be found.
// The HUD shows one round more than the gun holds (31 of a 30-round magazine, 101 of the M249's box):
// the mod's hidden round (ammo.cpp's Gun::hidden) is in the game's count so the game never finds the
// gun empty and never reloads it by itself on top of the hand's; the mod's own count stops the trigger.
// Not corrected (2026-11-25, looked at offline): the HUD is the game's client side, and the client's
// copy of the count (the objects that follow the server's, ammo.cpp's head) is set again from the
// server's at a round fired, not every frame (the M249's belt pieces, shortened by that copy, stay
// squashed after a magazine written in by hand until a shot: docs/research/other-guns.md). Written one
// lower there it would come back high at every round, and at the gun's last real round that copy would
// read empty: the count the game's own firing logic goes by, the self-reload the hidden round is there
// to stop. Changing only what the HUD draws means finding the game's UI code that formats the number,
// which can't be found or proven without the game running. So the HUD keeps the game's count.
bool Get(int& loaded, int& spare);

// The soldier's slot (0 or 1) the gun in the hands is in, as Update last worked it out (pickrules::HandSlot): the first for the M416, the
// Stinger, the SMAW and the train pistol; a gun taken up from the ground in whichever of the two the game put it. Every count of the gun in
// hand (Get, Eject, Insert, the hidden round, the trigger's stop) goes to it, and recoil counts its rounds in it. Any thread.
int HandSlot();
// The game told to raise slot 0 or 1 (input's pick, key 1 or 2): two guns of one kind, or one the guns' table doesn't know, are counted, after
// the game's words, in the slot last picked or taken into, whichever came later (HandSlot). Anything else is no pick. Any thread.
void Picked(int slot);
// The soldier's ten slots (0-9) for holsters (hands.cpp's holster::Frame), as Update read them this frame, before the hands' frame; through a take's
// window (pickups::TakeOpen, a second at most) as last read. HandSlot still goes by the first two alone.
// The game's own current weapon: the slot in its soldier's hands (Update's "the game's own current weapon" line), while that slot holds a weapon with
// a firing state. -1 none read: no soldier, the object its weapons keep it in not found or not read, a take's window open, or a slot holding no such
// weapon (holster.h keeps its last read a moment, then decides nothing on it). Any thread.
int GameSlot();
// A weapon with a firing state in that slot of the player's soldier (SecondaryCarried, for any slot). Any thread.
bool SlotCarried(int slot);
// That slot's gun is a launcher by its kind, its row in the guns' table (recoilguns.h) by its name: kRocket (the SMAW), kMissile (the Stinger, the
// Javelin) or kAt4; never the M320 (kM320), which goes with its rifle or by its slot. Holsters put it at the launcher's place whatever its slot.
// Any thread.
bool LauncherInSlot(int slot);
// That slot's weapon's blueprint name ("Weapons/AN94/AN94"); "" for none, or a slot out of 0-9. Render thread.
const char* NameInSlot(int slot);
// Its row in the guns' table (recoilguns::ByName of that name, looked up when the slot's gun changes and again once a second while it finds none, not
// every frame); nullptr for none, a slot out of 0-9, or a gun the table doesn't know. Render thread.
const recoilguns::Row* RowInSlot(int slot);
// The firing state that slot's name above was read with (Update's read of the slots this frame; through a take's window as last read); 0 none, or a
// slot out of 0-9. Against the firing state read later in the frame (FiringOf): another means the game changed the slot's gun since, and the name is
// a frame behind (hands.cpp's held magazine waits for it). Render thread.
uintptr_t SlotFiringRead(int slot);
// Which gun that slot holds, as this frame read it (holsters tell a new gun in a slot by it): its weapon object and its blueprint (the game makes new
// weapon objects at every swap and every new life); false, both 0, for none or a slot out of 0-9. Render thread.
bool SlotIdentity(int slot, uint32_t& weapon, uint32_t& blueprint);

// The rounds in the gun in hand as the game counts them, to tell shots by, on any gun: the train's pistol isn't on
// Get's path. Only ever reads. Render thread.
bool RoundsInHand(int& loaded);
// Something in the soldier's slot 1, the second key's: on Rock and a Hard Place, the Stinger once it's been taken up.
bool SecondaryCarried();

// The magazine came out: one round stays in the chamber, the rest go back to the spare rounds.
void Eject();

// A new magazine went in: as full as the spare rounds allow, plus the chambered round.
void Insert();

// X held with a gun in a hand (input.cpp's XButton): the game's own reload asked for (R held). Nothing of it is drawn at the hold: the magazine put in,
// and the M416's bolt home or a held gun's chamber filled, come when the game's count of that gun rises, a rise of any size within 8 s (Update,
// inputrules.h's StepPress); no rise, another gun, or a magazine pushed home by hand first, and nothing changes (the game's reload landing on the hand's
// magazine after that is undone: inputrules.h's StillAsked). Said. The M416 and the held guns only (the train pistol, the launchers and the Stinger
// have their own: input.cpp). Render thread.
void HeldPress();

// Every frame: notes how full the gun gets (a magazine plus a chambered round), which gives the
// magazine's size for Insert.
void Update();

// That size: 1 for a gun loaded a round at a time (the SMAW, the grenade launcher).
int MagazineSize();

// The train pistol (the M9 of Semper Fidelis and The Great Destroyer) with its magazine by hand and its rounds the mod's (pistolammo.h;
// build\pistol.txt "reload on", "mag 1", not "rounds 0"). hands.cpp says each frame whether that is so (false: its rounds are the game's alone,
// its own reload as before; one the mod counted is given back, its hidden round out). So, with the pistol in the hand, Update finds it (the
// list's first slot) and counts it from then on: a round in the chamber or none, the rounds in the magazine in the grip, the slide on its stop
// or home. The game's count is kept one above (a hidden round), so the game never reloads it by itself; a reload of the game's not asked for
// (X held asks) is undone. Render thread.
void SetPistolByHand(bool on);
// Counted by the mod just now (the answers below mean something). Any thread.
bool PistolLive();
// Nothing in its chamber: the trigger fires nothing (input.cpp). Any thread.
bool PistolDry();
// Its slide held open by its stop (the last round fired with a magazine in): drawn there (hands.cpp). Any thread.
bool PistolLocked();
// A magazine in its grip, as the mod counts it (true while it isn't counted). Any thread.
bool PistolMagIn();
// Its spare rounds and the rounds in it (the chamber's and the magazine's), as last looked at. Any thread.
int PistolSpare();
int PistolLoaded();
// What the hands did (hands.cpp), each said; false, or nothing done: not counted by the mod. Render thread.
// Its magazine out of the grip: the rounds in it back to the spare ones, the chambered round stays.
bool PistolEject();
// A fresh magazine seated: as many rounds as it holds, or as the spare ones have, from the spare ones; it chambers nothing.
bool PistolInsert();
// Its slide let go after a rack by hand or a tug off its stop (byTrigger: sent home or cycled by the trigger, PistolPull): a chambered round is
// thrown out (lost), the next is fed from the magazine; an empty magazine in, the stop holds it open; none in, it goes home empty.
pistolammo::Racked PistolRack(bool byTrigger);
// The right trigger pulled with nothing in its chamber (input.cpp held it off): the slide on its stop with rounds in the magazine goes home and
// chambers one (kSendsHome); home on an empty chamber with rounds in the magazine and the slide not worked by hand, it cycles and chambers one
// (kRacks); otherwise nothing (kClick). kFires: a round is chambered after all, or it isn't counted.
pistolammo::Pull PistolPull(bool slideByHand);

// A tube shotgun (the M1014) with its shells by hand and its count the mod's (shotgunammo.h; docs\research\shotgun-shells.md). hands.cpp says each frame
// whether that is so (false: its shells are the game's alone, its own reload as before; one the mod counted is given back, its hidden round out). With
// such a gun drawn in the hands, Update finds it in the hands' slot and counts it from then on: the shells in its tube (MagazineCapacity less one: 4, or 6
// with the longer tube), its chamber, its bolt held open or home, a shell lying in its ejection port. The game's count is kept one above (a hidden
// round), so the game never reloads it by itself; a reload of the game's not asked for is given back. Asked for (X held, TubeAsk; the keyboard's R too:
// nothing else, never a swap of the game's), the hidden round is taken out while it runs and each shell of it is kept; a shot ends the asking. Render
// thread.
void SetTubeByHand(bool on);
// Counted by the mod just now (the answers below mean something). Any thread.
bool TubeLive();
// No shell to fire: nothing live in its chamber, or its bolt held open: the trigger fires nothing (input.cpp). Any thread.
bool TubeDry();
// Its bolt held open; a live shell chambered; a shell lying in its ejection port. Any thread.
bool TubeLocked();
bool TubeChambered();
bool TubePort();
// The shells in its tube and how many it holds; the shells in it (the tube's and the chambered one); its spare shells as last looked at; those a hand
// can still take from the belly (the spare ones less what the gun holds that the game's count hasn't been told of, less the port's). Any thread.
int TubeShells();
int TubeCapacity();
int TubeCount();
int TubeSpare();
int TubeBelly();
// May a hand take a shell from the belly now (inHand: the shells the hands hold already). kYes while it isn't counted. Render thread.
shotgunammo::Take TubeCanTake(int inHand);
// The game's own reload of it is asked for just now (X held, or shells of it still landing): the hands stand aside. Any thread.
bool TubeGameReload();
// How many times a shot has left its bolt held open (the last shell fired), for the hands' buzz. Any thread.
unsigned TubeLockedShots();
// X held with it in the hands (input.cpp, before it sends R: the hidden round is out of the game's count by the next look, so a gun one short of full
// doesn't look full to the game), and X let go. Any thread.
void TubeAsk(bool held);
// Its hidden round is out of the game's count just now (its own reload asked for), as the last look left it. Any thread.
bool TubeHiddenOut();
// The trigger pulled on its last shell with the hidden round out (input.cpp, which holds that shot until TubeHiddenOut is false): the asking ends
// here, as a shot ends it, and the next look puts the hidden round back; so the shot never leaves the game's count at 0. Any thread.
void TubeEndAsk();
// What the hands did (hands.cpp), each a look, the move, a look, and said. Not counted by the mod: nothing done (kNoShell; a Bolt that didn't move;
// kFires). Render thread.
// A shell pushed up the loading port to the latch: one more in the tube, from the spare ones.
shotgunammo::Load TubeLoadShell();
// A shell laid into the ejection port with the bolt held open: it lies there, counted when the bolt takes it.
shotgunammo::Load TubePortLoad();
// The bolt sent home off its hold (the release, a tug of the handle): the port's shell chambered, else one from the tube, else home empty.
shotgunammo::Bolt TubeBoltHome();
// The handle drawn back from home and let go: what is chambered thrown out (a live shell lost), the next fed, or the bolt left open.
shotgunammo::Bolt TubeRack();
// The trigger pulled with nothing to fire (input.cpp held it off): a click with the bolt home on an empty chamber, once; else nothing.
shotgunammo::Pull TubePull(bool handleHeld);

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

// The AT4 (Operation Swordbreaker's, kHeldKinds' at4), found by its name among the slots as this frame read them (the scene's kit puts it in slot 0
// with the Mk11 left in slot 1), never by a fixed slot. Its count as the mod has it (without its hidden round: Update gives it one while it's in
// hand, so the game never reloads it by itself) and which one it is (its firing state: another life, its checkpoint loaded again). Render thread.
bool At4Get(int& loaded, int& spare, unsigned& which);
// Its slot, as this frame read the slots; -1 none. (No reload of it by the mod: one rocket, then thrown away.)
int At4Slot();

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

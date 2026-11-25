#pragma once
#include <openxr/openxr.h>

namespace hands {

// Free hands: the gun (with the right hand on it) is drawn at the right controller and the left
// glove at the left controller; the rest of the first-person arms is folded away. Works out the
// target poses every frame, in the space BF3 draws the first-person arms in, and hands them to
// skin.cpp, which redraws the arms and gun with moved bones.
//
// head: the eyes' centre and orientation in the recentered space, the controllers' in the same.
// Render thread, after input::Sync, in stereo mode.
void Update(const XrPosef& head);

// Insert: free hands on / off (on by default).
void Toggle();

// Free hands are on (the left trigger then works the charging handle instead of zooming).
bool On();

}
namespace loose {
struct Frame;
}
namespace hands {
// The loose things' frame as this frame's Update made it (the room into the world, the gun and the left hand, the ground), for their starts (skin.cpp); null before the
// first. Render thread.
const loose::Frame* LooseFrame();

// True while the left hand steers the gun (steer.h: holding the handguard, eased in and out, letting
// go too): the right controller's aim orientation turned as the gun is drawn, for the game to aim
// and fire along, in the recentered space; otherwise false (the gun goes by the right hand alone).
// After Update, same thread.
bool AimOrientation(XrQuaternionf& out);

// weapon.txt "steer 35 65": the left hand steers the gun fully while the hands' line is within the
// first angle of where the right hand points it, not at all past the second (degrees; steer.h).
void SetSteerCone(float fullDegrees, float noneDegrees);

// The train pistol's slide held back by the left hand (build\pistol.txt "reload on" and "slide 1", the
// pistol in the right hand): it's out of battery, so the right trigger mustn't fire (input.cpp). False
// otherwise, and once Update hasn't seen it held for 0.2 s (free hands off, the headset not rendering).
// Render thread.
bool PistolSlideHeld();

// The train pistol's magazine by hand (build\pistol.txt "reload on" and "mag 1", the pistol in the right hand; its
// rounds go with it unless "rounds 0", the rehearsal: ammo.h's pistol). X tapped (input.cpp MagazineTap): its magazine
// release. One part way into the grip drops out; otherwise the pistol's own drops out of the grip; with the grip empty
// nothing (fromScript, F16: one goes in). False when it isn't the pistol's: X as before (the game's reload). Render
// thread.
bool PistolMagazineTap(bool fromScript);

// The held gun's own sound, cut from its own reload wave in the game's data (build\sounds\<kind>_<its shape>.wav, tools/gamesounds.cpp); one with none of its own, the general one.
void PlayGunsOwn(const char* kind, const char* otherwise);

// A hand holding the grenade launcher's own grip under the rifle (its second hold): input.cpp puts the launcher in hand and the trigger of the hand
// on its grip fires it. M320HeldBy() >= 0.
bool M320Held();
// The hand on the grenade launcher's own grip: 0 the left, 1 the right, -1 none; M320Held() is M320HeldBy() >= 0.
int M320HeldBy();
// X held (the game's own reload, R), alike: the pistol's magazine is in again (one part way in drops out first, one in
// the left hand is gone). Nothing otherwise. Render thread.
void PistolGameReload();
// Its magazine out of the grip in the rehearsal ("rounds 0", or the pistol's rounds not counted by the mod): the right
// trigger mustn't fire (input.cpp). False otherwise (with its rounds by hand the chambered round still fires: PistolDry),
// and once Update hasn't seen it so with the pistol in hand for 0.2 s. Any thread.
bool PistolMagazineOut();
// The train pistol with its rounds by hand (ammo.h's pistol) and nothing in its chamber: the right trigger fires nothing
// (input.cpp holds it off, and says so with PistolDryPull once a pull: the next hands' frame sends the slide home off
// its stop with a fresh magazine in, or does nothing). A state, not a time. Any thread.
bool PistolDry();
void PistolDryPull();

// A tube shotgun with its shells by hand (the M1014; shellload.h, ammo.h's tube; weapon.txt "tubebyhand 0" turns it off), as the last Update had it:
// in a hand (X then asks for the game's own reload through ammo::TubeAsk, and a tap of it does nothing: it has no magazine); nothing to fire, a live
// shell not chambered or its bolt held open: the trigger fires nothing (input.cpp holds it off, and says so with TubeDryPull once a pull: the next
// hands' frame clicks, once, with the bolt home on an empty chamber); its bolt handle held back by a hand (out of battery: no shot). States, not times.
// Any thread.
bool TubeInHand();
bool TubeDry();
bool TubeBoltHeld();
void TubeDryPull();

// The launcher loaded by the left hand (rocket.cpp, weapon.txt "launcherhands 0" turns it off): no rocket in it, so the right
// trigger mustn't fire (input.cpp). False otherwise, and once Update hasn't seen it so for 0.2 s. Render thread.
bool LauncherEmpty();
// The Stinger with no missile ready, its aerial to be lifted (flap.cpp). Render thread.
bool StingerEmpty();
// The AT4 in hand with its rocket gone (its own count at none: ammo::At4Get): the trigger mustn't fire; let go of, the spent tube is thrown away.
// Render thread.
bool At4Spent();
// X held with it empty: the game's own reload and animation instead. Nothing otherwise. Render thread.
void LauncherGameReload();
// A gun with no magazine button has run dry: nothing fires until a magazine's in and its lever's been pulled.
bool HeldChamberEmpty();
// The game's own reload (X held) or a new life (newLife: its lid and belt as they come too): a round in a held gun's chamber.
void HeldChambered(bool newLife);
// The tube shotgun the mod counted is forgotten (ammo.cpp: a new life, a checkpoint, another level, a swap of the game's, out of the soldier's slots,
// its shells the game's own again): nothing of it stays in a hand, no handle held, its bolt drawn as the next count has it (shellload::Clear).
// Render thread.
void TubeForgotten();

// Free hands on the rifle (either hand holds it, lets go of it, takes it up; build\freehands.txt). All of them are one frame old
// for input.cpp, like every other flag here.
// Whose trigger fires the rifle: 0 the left, 1 the right, -1 none (nobody on its pistol grip, or a newly taking hand's trigger
// not let go once yet). 1 whenever free hands are dormant (another gun, seated, off): today's right trigger.
int FireHand();
// The gun in two hands as recoil counts them: held by its pistol grip, the other hand's glove on its front (handguard, front grip, the grenade
// launcher's grip) at least half way. False with free hands off. A film round's trigger fires real rounds only so (input.cpp).
bool TwoHands();
// A hand has the rifle (by its pistol grip or its handguard alone); false while it lies on the ground, flies into a hand or is put away at its
// place (GunStowed). True whenever free hands are dormant.
bool HoldsGun();
// The hand on the gun's pistol grip: 0 the left, 1 the right, -1 nobody (lying, gliding into a hand, held by its handguard alone).
// Unlike FireHand, a hand that has just taken it is on it at once. 1 whenever free hands are dormant.
int GripHand();
// Holsters (src/holster.h): the game's weapon is put away at its place on the body, in no hand and lying nowhere (HoldsGun() false); false while dormant or off.
bool GunStowed();
// One owner per squeeze (the audit of 2026-11-19): that hand (0 left, 1 right) carries something as the last Update left it: a magazine, a round, a
// case, the lever fist, the aerial, the charging handle, the train pistol's slide or magazine, a prompt's use under way, the gun in a hand not seen at a
// holster's place (qte::HoldsForKey). input.cpp's quick-time step never takes such a hand's squeeze for a key in play. The gun's own holds (its pistol
// grip, its front, the handguard, a launcher's front grip, the pistol in the hand) don't count: either grip answers, the gun kept in the hand meanwhile
// (the review of 2026-11-25). False with the mod's hands off. Any thread.
bool HoldsSomething(int hand);
// Holsters: a pick (the D-pad, Touch's menu button, Y held) of this slot would raise the put-away gun's own: no key is sent, the next Update draws it into the right hand
// (not into a right hand holding a magazine, a round or a case: it stays at its place, said; a right hand not its own controller seen has it as at a spawn).
bool PickIsOwnSlot(int slot);
// Holsters: the gun put away and its place, for input's lines ("the M416 at the right shoulder"); "" while none is. Static storage, render thread.
const char* StowedWhere();
// Holsters: the game's own pickup (R held: X held's) would take a holstered gun out of its slot and it couldn't be given back (the pickup destroys
// the gun it replaces, takes every gun, or has entries not read): true, with words for input's line; false otherwise (no target, holsters off,
// dormant, or a take that puts it into the pickup: that one goes ahead, the gun staying on its shoulder, drawn back from there by the game's take of
// that pickup). A holstered gun stays till a hand takes it out and drops it (the user, 2026-11-18). Render thread.
bool TakeWouldSwapHolstered(char* words, size_t size);
// A scripted prompt that gives a gun (the reload key's: Operation Swordbreaker's AT4) is used only with no gun in either hand (the owner, 2026-11-19:
// "Better to physically drop the rifle and pick up the AT"): true while free hands are awake with a gun in a hand (by its grip or its front alone,
// flying into one, a take of one off the ground or a draw from a shoulder under way), with words for input's line ("the right hand holds the M4A1");
// false otherwise (free hands dormant or off, or a scripted moment with the hands frozen: the game's own key is the way then). Render thread, before
// this frame's Update (input.cpp's X).
bool GunBarsGift(char* words, size_t size);
// ... refused by input.cpp (by: "X held", "X"): its line, and the double click on the hand that holds the gun (both when none does by its grip).
void RefuseGift(const char* by);
// A test round's left D-pad right held this frame (freehands.txt "test 12"): input.cpp reads it as the controller's.
bool ScriptEject();
// The game's aim from free hands, in the recentered space: along the rifle held by the left or by its handguard alone, gliding
// into a hand, or easing to the head while it lies. False: the old way (the right controller's aim, AimOrientation while steered).
bool Aim(XrQuaternionf& out);
// xr_runtime, each frame: a cutscene or a scripted moment (nothing changes hands meanwhile).
void SetScripted(bool scripted);

}

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

// The train pistol's magazine by hand (build\pistol.txt "reload on" and "mag 1", the pistol in the right hand; a
// rehearsal: no rounds change). X tapped (input.cpp MagazineTap): its magazine release. One part way into the grip
// drops out; otherwise the pistol's own drops out of the grip; with the grip empty nothing (fromScript, F16: one goes
// in). False when it isn't the pistol's: X as before (the game's reload). Render thread.
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
// Its magazine out of the grip: the right trigger mustn't fire (input.cpp). False otherwise, and once Update hasn't
// seen it so with the pistol in hand for 0.2 s. Render thread.
bool PistolMagazineOut();

// The launcher loaded by the left hand (rocket.cpp, weapon.txt "launcherhands 0" turns it off): no rocket in it, so the right
// trigger mustn't fire (input.cpp). False otherwise, and once Update hasn't seen it so for 0.2 s. Render thread.
bool LauncherEmpty();
// The Stinger with no missile ready, its aerial to be lifted (flap.cpp). Render thread.
bool StingerEmpty();
// X held with it empty: the game's own reload and animation instead. Nothing otherwise. Render thread.
void LauncherGameReload();
// A gun with no magazine button has run dry: nothing fires until a magazine's in and its lever's been pulled.
bool HeldChamberEmpty();
// The game's own reload (X held) or a new life (newLife: its lid and belt as they come too): a round in a held gun's chamber.
void HeldChambered(bool newLife);

// Free hands on the rifle (either hand holds it, lets go of it, takes it up; build\freehands.txt). All four are one frame old
// for input.cpp, like every other flag here.
// Whose trigger fires the rifle: 0 the left, 1 the right, -1 none (nobody on its pistol grip, or a newly taking hand's trigger
// not let go once yet). 1 whenever free hands are dormant (another gun, seated, off): today's right trigger.
int FireHand();
// A hand has the rifle (by its pistol grip or its handguard alone); false while it lies on the ground or flies into a hand.
// True whenever free hands are dormant.
bool HoldsGun();
// The game's aim from free hands, in the recentered space: along the rifle held by the left or by its handguard alone, gliding
// into a hand, or easing to the head while it lies. False: the old way (the right controller's aim, AimOrientation while steered).
bool Aim(XrQuaternionf& out);
// xr_runtime, each frame: a cutscene or a scripted moment (nothing changes hands meanwhile).
void SetScripted(bool scripted);

}

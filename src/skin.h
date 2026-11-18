#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include "flap.h"
#include "handmag.h"
#include "handpose.h"
#include "shellload.h"

struct ID3D11Resource;
struct ID3D11DeviceContext;
struct IDXGISwapChain;

namespace skin {

// The bare arms of The Great Destroyer follow the controllers only while this allows it
// (xr_runtime.cpp: no cutscene picture or scripted turn for a few seconds, the aim found).
void SetBareHandsAllowed(bool allowed);
// weapon.txt "barewrist 0/1": the bare forearms solved from their hands (hands.cpp; on by default).
bool BareWrist();
// weapon.txt "baretwist L R": degrees taken off each bare forearm's roll before it's solved (the solver's
// no-roll may sit off by a fixed amount per arm: the candy-wrapper wrist).
float BareTwist(int side);
// weapon.txt "bareshare E A B": the bare forearms' shares of the hand's roll (hands.cpp PlaceBareForearms),
// the elbow end, the middle and the wrist end.
void BareShares(float* share);
// While the train pistol is in hand: the right palm from the pistol's frame as the game holds it (learned
// from an arm draw and a pistol draw next to each other in one eye). False until learned, or with no pistol.
bool BarePistolHand(float* hold);
// The train pistol in hand, as BarePistolHand finds it.
bool TrainPistolInHand();
// The game's own left hand on it, learned with the right's (the game's two-handed M9 hold): the left palm
// from the pistol's frame, and its bones as the left glove's (handpose.cpp's 0-22) from the bind space
// relative to that palm. False until learned (the left hand at the grip), or with no pistol.
bool BarePistolLeft(float* hold, float (*fingers)[12]);
// weapon.txt "pistoltest 1": the stand-in left hand holds the pistol in both hands (a test with nobody in the
// headset: hands.cpp PistolSupport).
bool PistolTest();
// The train pistol's magazine seat: its bone 0 on the frame bone (3) as the game draws it home (rows: rotation |
// translation), its own y the grip's line. Taken from the game's own draw while the manual reload is on
// (HandTargets::pistolReload) and it's within 2 mm of the recordings'; the recordings' until then (false).
bool PistolMagazineSeat(float* seat);
// Where the train pistol's magazine falls from (DropPistolMagazine).
enum class PistolDrop {
    kGrip,  // its own, out of the grip along its line (X)
    kWell,  // one held part way into the grip (HandTargets::pistolMag): out along its line from where it's drawn
    kHand,  // let go of: from where it was drawn in the left hand, at the hand's speed
};
// The train pistol's magazine falls from where it was drawn this last tenth of a second: out of the grip it first
// slides along its line (the well guides it), then falls, tumbles and vanishes as it lands. velocityView: kHand's
// speed as it let go (m/s in the view's space; none: from rest). False if it hasn't been drawn there lately, or
// the manual reload isn't on with the pistol in hand. Render thread.
bool DropPistolMagazine(PistolDrop from, const float* velocityView = nullptr);

// weapon.txt "pslog": the instanced draws (effects.cpp) logged like the indexed ones.
void LogShaderDraw(ID3D11DeviceContext* ctx, unsigned count, unsigned start, int base, unsigned instances);

// Research into BF3's skinning: the bone matrices it uploads to the graphics card to bend
// meshes such as the first-person arms. Hooks ID3D11DeviceContext::Map/Unmap/DrawIndexed
// (immediate and deferred: index 0 and 1, which may be the same function). Call between
// MH_Initialize and MH_EnableHook.
bool InstallHooks(void* const (&map)[2], void* const (&unmap)[2], void* const (&drawIndexed)[2]);
// The second way of hooking Map, Unmap and DrawIndexed (in the game's own contexts' tables), when the first can't take them on
// this graphics driver or hooks.txt asks for it: done once, at the first picture. Every Present; nothing to do but a check after.
void InstallTableHooks(IDXGISwapChain* swap);

// Every frame: where the camera is in the world and its axes (right, up, true forward), so the
// log can show where blocks sit relative to the eyes.
void SetCamera(const float* position, const float* right, const float* up, const float* forward);

// Pause: research hooks on/off (bone searches in every upload, ring-buffer comparisons, draw
// counting). Off by default: they cost frame rate. The hunt and the lift turn them on.
void ToggleResearch();

// Lift the camera-relative pieces (within a metre of the origin) by 15 cm, or stop.
// Logs what's been seen since the last press.
void ToggleLift();

// Insert: draw the gloves (the draw the hunt found) 20 cm higher, or normally again. The first
// test of moving the hands.
void ToggleHandShift();

// Every frame: the game's own FOV and the wider one the mod renders with (radians; either 0 =
// off). Matrices uploaded with the game's FOV, such as the first-person gun and arms', are
// rescaled to the wide one.
void SetProjectionFix(float gameFov, float wideFov);

// Draw hunt, driven by tools/drawhunt.ps1 through keys no real keyboard has: F15 starts it
// (hiding half of all skinned draws seen so far), then F13 = "the arms vanished" or F14 = "still
// there" halves the candidates again, until the arms' draw is found. Its bone data is logged.
void HuntStart();
void HuntAnswer(bool vanished);

// Every frame: logs the found draw's bone data about once a second.
void Probe();

// The magazine of the gun the mod knows (the campaign's M16-type rifle): out of the gun, falling
// to the floor and lying there a while (false if the gun hasn't been drawn yet, or it's already
// out), or back in. Render thread.
bool EjectMagazine();
void InsertMagazine();
bool MagazineOut();
void SetMagazineOut(bool out);  // as the gun now in hand was left
// A magazine held in either hand (HandTargets::mag) let go of: it falls from where it was last
// drawn in the hand (this last tenth of a second), starting at velocityView (m/s in the view's space;
// none: from rest). This one goes by the left hand's motion; the right's is the overload with its
// side and hand (below, after loose's names). False if it hasn't been drawn there lately.
bool DropHeldMagazine(const float* velocityView = nullptr);
// Whether the magazine's seat in the gun (each of its bones' offset from the gun's body, as the game
// draws it in the well) is known: until then a magazine can't be drawn in the hand.
bool MagazineSeatKnown();
// Whether that gun is the one in hand (drawn in the last half second).
bool RifleInHand();

// Every frame on the render thread: moves the dropped magazine.
void Update();

// In a scripted picture with the body moved to each eye's place (weapon.txt "bodystereo 1"): an eye's picture is
// wanted, and one was drawn (effects.cpp's draw hooks: six vertices, unblended, onto a target the window's size and
// twice as wide as high), which ends that eye's pass: the body's views after it are the other eye's.
bool EyePictureWanted();
void NoteEyePicture();

// weapon.txt says "watch": research logs are wanted (bones, the gun's matrices, poses).
bool Watching();

// The first-person view-projection BF3 uploaded last (rows as uploaded, before the wide-view
// rescale): in the space the arms and gun are drawn in, and in the world. Both carry the eye's
// position and the view's axes; their difference is where that space sits in the world.
bool FirstPersonViews(float (&arms)[16], float (&world)[16]);
// Whether a constant buffer is one those view-projections went into: 1 in the arms' space, 2 in
// the world, 0 neither.
int ViewBufferKind(ID3D11Resource* buffer);

// This thread's latest 4096-byte upload (BF3's per-draw block: bones, or instances' placements)
// and its buffer; whole only while something needs it (weapon.txt "watch", free hands).
bool LatestBlock(ID3D11Resource*& resource, std::vector<uint8_t>& bytes);
// This thread's latest small upload into that buffer, if it's among the last eight (weapon.txt
// "watch" keeps them), and the latest 4096-byte block if it went into that buffer: valid until this
// thread's next upload. serial: UploadSerial() when it was made.
const uint8_t* RecentUploadOf(ID3D11Resource* resource, unsigned int& size, uint32_t& serial);
const uint8_t* LatestBlockOf(ID3D11Resource* resource, unsigned int& size, uint32_t& serial);
// Small fresh uploads made on this thread so far: data a draw uses is the draw's own only if it
// was uploaded after the draw before it.
uint32_t UploadSerial();
// Where the game's own gun body was last drawn, in the world and in the arms' space.
bool GameGunPlaces(float (&world)[3], float (&arms)[3]);
// The end of the barrel and the ejection port of the game's own gun, in the world.
bool GameGunMuzzle(float (&world)[3]);
bool GameGunPort(float (&world)[3]);
// An eye's place in the world, from the camera its view is built with: says where the arms' space is (views.cpp).
void NoteWorldEye(const float* eye);
// A view build's main cameras, turned to the head and moved with it, both eyes': what the world's picture is drawn from
// (views.cpp), and the game's own eye they were made from.
void NoteMainCamera(const float* gameEye, const float* leftEye, const float* rightEye);
// The gun's move from where the game holds it to the right hand, to where the gun is seen from the middle of the eyes in the
// world's picture, in the world: a 4x4 (rows; column vectors). False while the gun isn't moved.
bool GunDeltaWorld(float (&out)[16]);
// This thread's latest per-view constants (352 bytes: the view-projection at +176) as they went to
// the graphics card, and their buffer.
const uint8_t* LatestViewConstants(ID3D11Resource*& resource);
// Draws (draw) with bytes bound in place of the constant buffer original, in the vertex shader
// slot it's in, then puts BF3's back. which: the immediate (0) or deferred (1) context's functions.
bool DrawWithCopy(int which, ID3D11DeviceContext* ctx, ID3D11Resource* original, const uint8_t* bytes,
                  unsigned int size, const std::function<void()>& draw);
// The game's own gun body as last drawn (bone 2 of its passes), in the world: rows rotation |
// translation.
bool GameGunBody(float (&out)[12]);

// How the right glove (the gloves' bones 23-44, the 'R' entries) is drawn (HandTargets::rightGlove):
//   kRightGame    as the game sent it (the rifle drawn as the game's own, free hands off);
//   kRightOnGrip  on the rifle's pistol grip as the game holds it: rightGunPose times each pass's own learned bone, the index
//                 bent by the right trigger (today's right hand);
//   kRightOwn     its own pose: rightGlovePose times rightFingers (free, on the handguard, or blending onto or off the grip);
//   kRightHidden  folded away: not seen while the mod draws the rifle somewhere else.
enum RightGlove : int { kRightGame = 0, kRightOnGrip = 1, kRightOwn = 2, kRightHidden = 3 };

// Free hands (hands.cpp): where to draw the gun and the left hand. Matrices are three rows,
// rotation | translation, from the gun's model space / the gloves' bind space into the view's
// own space: the eye at the origin, x right, y up, z back, metres. Each draw places that in the
// arms' space with the eye and axes of the view-projection uploaded for it, so the hands stay put
// relative to the picture however the arms' space moves.
struct HandTargets {
    bool gun = false, left = false;  // which of these is tracked
    float gunPose[12] = {};
    // The rifle drawn (gunDrawn) though no hand holds it (gun false): flying into a hand, or lying (gunLying). Lying, it's
    // placed from the world: gunWorld, its model space into the arms' space with origin gunWorldOrigin (whole metres, as
    // the loose things are), so it stays put on the ground however the head moves (ToArms takes it into each draw's).
    bool gunDrawn = false, gunLying = false;
    float gunWorld[12] = {};
    float gunWorldOrigin[3] = {};
    // The right glove (RightGlove above): kRightOnGrip drawn on rightGunPose (the gun's model space into the view's, as
    // gunPose: the same while the right holds it), kRightOwn at rightGlovePose (the gloves' bind space into the view's)
    // with each of its bones (23-44, [bone - 23]) bent by rightFingers in the bind space, as leftPose and leftFingers.
    int rightGlove = kRightGame;
    float rightGunPose[12] = {};
    float rightGlovePose[12] = {};
    float rightFingers[handpose::kRightBones][12] = {};
    // The gun and the train pistol where the world's picture shows them, in the arms' space: ToArms fills them. What the game
    // draws in the world and goes with the gun is placed by these (GunDeltaWorld), not by the poses above: those go through
    // the first-person view, with each eye's fake shift, and that view has neither the head's turn nor its move.
    float gunSeen[12] = {}, pistolSeen[12] = {};
    // This draw's own eye, in the arms' space (ToArms fills it).
    float eyeInArms[3] = {};
    // The arms' space's origin in the world, for the view of this draw (it jumps a metre as the player walks across one).
    float seenOrigin[3] = {};
    float seenEye[3] = {};  // the game's own eye in the world, of that same build (the camera its rockets start from)
    bool seenHaveOrigin = false;
    // This draw's way from the world (the arms' space with seenOrigin, as the world's picture has it) into what it draws: this eye's first-person placing (ToArms). A thing lying in the
    // world drawn through the first-person passes goes through it, as the gun does through gunPose: its own eye's place and the head's turn, not the aim camera's (the dropped
    // magazine that looked "like some other dimension": the user, 2026-11-05).
    float drawnFromWorld[12] = {};
    bool haveDrawnFromWorld = false;

    float leftPose[12] = {};
    // The right hand's as the left glove's is, mirrored (its controller with the left glove's line-up
    // mirrored across x): The Great Destroyer's bare right arm is placed with it (skin.cpp).
    bool right = false;
    float rightPose[12] = {};
    // The train pistol (The Great Destroyer) as the right controller holds it: its model space into the
    // view's, its grip at the right hand's grip point, aimed as the M416 is (gunPose).
    float pistolPose[12] = {};
    // The launcher (the SMAW, weapon.txt "launcher 1") alike: its frame bone, its rear grip at the right hand's grip point,
    // and where the world's picture shows it (ToArms, as gunSeen is).
    float launcherPose[12] = {}, launcherSeen[12] = {};
    // With it in hand and loaded by the left hand (rocket.cpp, set by hands.cpp): the left glove placed by leftPose and leftFingers
    // like any free hand, not moved with the launcher (hands.cpp blends them onto the game's own front grip while the hand holds
    // it: LauncherLeftGrip); and its case: 0 where the game's animation has it, 1 drawn nowhere, 2 at launcherCasePose (its bone
    // into the view's space), 3 at launcherCaseRel on its frame (it lies).
    bool heldMagHidden = false;  // any other gun: its seated magazine not drawn, the hand is rocking it out (t.mag, t.magPose: handmag.cpp)
    float heldLeverBack = 0.0f;  // ... its side lever drawn back this far along it
    float heldLeverSwing = 0.0f; // ... swung out this much first, a handle that swings out (radians)
    float heldLeverNotch = 0.0f; // ... turned up this much into its notch, a handle that has one (radians about the gun's z)
    bool heldPanels = false;     // ... the Stinger's aerial not as the game has it: these, its bones 3 to 6, on its frame
    float heldPanelBones[4][12] = {};
    float heldLid = 0.0f;        // ... a belt fed gun's lid up this much (radians)
    bool heldBeltDrawn = false;  // ... its belt not lying as the game has it: these, on its frame
    float heldBeltBones[5][12] = {};
    int heldBeltPieces = 5;      // ... as many pieces of the belt as the rounds left make
    float heldCover = 1.0f;      // ... how far open its port's cover is, one that has one (0 shut, 1 open as its mesh has it)
    bool heldOldRocked = false;  // ... its seated magazine giving a little under a fresh one: this, from its seat, in the gun's space
    float heldOldRock[12] = {};
    // A tube shotgun's shells by hand (shellload.h; set by hands.cpp only with such a gun held and its reload by hand on: heldTube; the M1014). Its bolt
    // group drawn heldBoltBack back from home on the frame whatever the game's animation has it at (above 0: held open, drawn by the handle, on its way
    // home; 0: as the game moves it, so its own stroke at each shot shows). The shell the game itself draws on the gun (parked on the lifter, in its own
    // reload pushed up by its hand) is drawn only while the game's own reload plays (heldShellGame); the shell's mesh is drawn again at each of
    // heldShells places, the bone of its own draw there: on the gun's frame (heldShellOnGun: on the loading port's way, lying in the ejection port)
    // or in the view's space (in a hand; ToArms takes those on into the draw's). Loose shells lying or falling are drawn with them (loose).
    bool heldTube = false;
    float heldBoltBack = 0.0f;
    bool heldShellGame = false;
    int heldShells = 0;
    bool heldShellOnGun[3] = {};
    float heldShellPose[3][12] = {};
    bool launcherLeftFree = false;
    bool launcherGameHands = false;  // the game's own reload of it plays (or none of the mod's): its hands are followed on it, wherever they go
    int launcherCase = 0;
    float launcherCasePose[12] = {};
    // The grenade launcher under the rifle reloaded by hand (m320.h, set by hands.cpp), when the module has its parts (m320Own; else weapon.txt's test lines or the game's own):
    // its barrel swung open (radians), its round (0 on the barrel's line slid m320RoundOut back, 1 none, 2 at m320RoundOnGun: its bone on the gun's frame bone), and a round
    // in the hand (its bone into the view's space; ToArms takes it on into the draw's).
    bool m320Own = false;
    float m320Barrel = 0.0f;
    int m320Round = 0;
    float m320RoundOut = 0.0f;
    float m320RoundOnGun[12] = {};
    bool m320HandRound = false;
    float m320HandRoundPose[12] = {};
    float rightWrist[3] = {}, leftWrist[3] = {};  // each hand's grip point
    // Each of the left glove's bones bent as its fingers are held, in the gloves' bind space
    // (handpose.cpp): the glove is drawn with leftPose composed with these.
    float leftFingers[handpose::kBones][12] = {};
    // A magazine in either hand or on its way into the well (pouch.cpp): where the gun's body
    // would be with that magazine seated in it, so its bones are drawn at magPose times their seat.
    bool mag = false;
    float magPose[12] = {};
    // Each forearm solved from its hand and a shoulder (armik.h; hands.cpp PlaceForearms), left
    // then right: its elbow end and twist bones A and B, from the gloves' bind space into the view's,
    // and where its folded arm goes inside it. Without (arm false: arms.txt "off") the forearm moves whole with its hand.
    // Then the upper arm and the collar bone (4th and 5th), for the sleeves: each arm whole up to its shoulder. The chest and legs fold onto chest, between the shoulders, with an arm solved.
    bool arm[2] = {};
    float forearm[2][5][12] = {};
    float fold[2][3] = {};
    bool haveChest = false;
    float chest[3] = {};
    // The game's own left hand on a launcher-pose gun while the left isn't the player's (DrawLauncherRig): its arm solved the same way from where the hand is drawn on the
    // gun (hands.cpp, by skin::LeftHandOnRig), so it's whole up to its shoulder and never a forearm ending in an open cut. As forearm[0] and fold[0] are, but its own: the
    // M416's draws never see them. rigLeftHand: the hand's frame on the gun's that it was solved from; a draw that finds another there (another gun's rest, taken since: a
    // switch's frame) moves the hand back onto this one for that frame, so the arm is never drawn from one hand to another, nor in one eye's picture and not the other's.
    bool rigLeftArm = false;
    float rigLeftForearm[5][12] = {};
    float rigLeftFold[3] = {};
    float rigLeftHand[12] = {};
    // The bare arms' (The Great Destroyer) forearms solved from each hand the same way, left then right
    // (hands.cpp PlaceBareForearms): the hand bends at the wrist, the forearm stays on the arm's line.
    bool bareArm[2] = {};
    float bareForearm[2][3][12] = {};
    // The bare left hand's fingers while the right holds the train pistol (hands.cpp): each of the left
    // glove's bones (handpose.cpp's 0-22) from the bind space relative to its palm (1), as the controller
    // holds them, blended into the game's own two-handed hold while the left hand holds the pistol. Off:
    // the game's own fingers.
    bool bareLeftOn = false;
    // How far the left hand has taken the train pistol (0 one hand, 1 both: hands.cpp PistolSupport's blend), for
    // the right hand's fingers on its grip (skin.cpp: pistolfingers / pistolfingers2).
    float pistolTwoHanded = 0.0f;
    float bareLeftFingers[handpose::kBones][12] = {};
    // The train pistol's manual reload (docs\research\pistol-manual-reload.md), set by hands.cpp only with the
    // pistol in the right hand and build\pistol.txt "reload on" (pistolReload): otherwise none of these is, and the
    // pistol is drawn as before. DrawGun, the M416's, never reads them.
    bool pistolReload = false;
    // A magazine in the left hand or on its way into the grip: where the pistol's frame bone would be with it
    // seated, so its bone 0 is drawn at pistolMagPose times the seat (as magPose is for the M416's).
    bool pistolMag = false;
    float pistolMagPose[12] = {};
    // The pistol's own magazine out of the grip (drawn nowhere), and its slide drawn this far back along its own
    // axis (m; below 0 as the game's own animation has it).
    bool pistolMagOut = false;
    float pistolSlideBack = -1.0f;
    // Free hands for every gun: a launcher-pose gun (DrawHeldPart's, the SMAW: launcherPose) drawn though no hand holds it (launcherDrawn, gun false): flying
    // into a hand (launcherPose, the view's space), or lying (launcherLying: launcherWorld, its frame into the arms' space with origin launcherWorldOrigin), as
    // gunDrawn, gunLying and gunWorld are for the M416.
    bool launcherDrawn = false, launcherLying = false;
    float launcherWorld[12] = {};
    float launcherWorldOrigin[3] = {};
    // The right glove on a launcher-pose gun's grip (kRightOnGrip): the gun it's drawn on, as rightGunPose is for the M416 (the same as launcherPose while the
    // right holds it; at the right hand's own place while the gun flies into it).
    float rightLauncherPose[12] = {};
    // The right hand holds the gun's pistol grip (free hands dormant, or the right the trigger hand holding it or taking it): only then is the game's own left
    // hand drawn on a launcher-pose gun whose left hand isn't the player's (DrawLauncherRig).
    bool rightOnPistol = true;
    // The launcher's case on its frame while the launcher lies (launcherCase 3): drawn with the lying launcher as it's drawn, not through the hands' view.
    float launcherCaseRel[12] = {};
    // freehands.txt "twosided 1": a lying gun's faces drawn both ways (the first-person meshes may lack the sides first person never shows).
    bool lyingTwoSided = false;
    // A gun taken off the ground by hand is due in a hand (pickups.h): its kind (the profile's id: the M416's 12768, the launcher's 3414, else a held
    // gun's colour draw's index count; 0 none). With no hand on a gun, that gun's draws never go as the game holds it. arriveHidden false: they are
    // placed by the targets, which have it lying where it lay (not only the draws of the gun the targets came for). arriveHidden true: they aren't
    // drawn at all until it flies in (the targets of its pose are another gun's, lying).
    uint32_t arriveKind = 0;
    bool arriveHidden = false;
    // The gun drawn nowhere (hands.cpp's holsters): put away at its place, a draw waiting for the game's switch, or held by a hand that isn't seen and is taken to be at
    // a place. Whichever gun the game draws then (the rifle and its parts, the ACOG and the M320; a launcher; any other gun) is left out whole, never drawn where the game
    // holds it, whatever gun, gunDrawn and launcherDrawn say; its draws' stamps still say which gun is the game's (RifleInHand, LauncherInHand, HeldGunInHand), and its
    // scope's own picture goes off by itself (no lens is drawn). What rides those draws and isn't the gun is drawn as before: the magazines and cases lying (loose, and
    // the old fall's), a magazine in a hand (mag), the grenade launcher's round in a hand (m320HandRound), the launcher's case or rocket in a hand (launcherCase 2: drawn
    // alone). Those three are drawn wherever their poses say, so only for what a hand holds: a case still on the launcher's frame is 1 then, a magazine on the gun's
    // line none. The right glove only by its own pose (kRightOwn; kRightGame and kRightOnGrip are taken as kRightHidden), the left by leftPose while left; a hand not
    // drawn folds away, its forearm with it, as over a gun lying. Only with targets given: SetHands(nullptr) is the game's own arms and gun. While the holsters have it
    // away, set on every frame of it, a take off the ground's too until the taken gun is the game's: a frame without it, and with no gun, gunDrawn or launcherDrawn, has
    // the game's own draws, at its hold.
    bool stowed = false;
};
void SetHands(const HandTargets* targets);  // nullptr = off: the game's own arms

// Where the game's own right hand holds the gun, in the gun's model space (from the hand's and the
// gun's bones as the game animates them). False until both have been drawn.
bool GunGrip(float* point);
// The launcher of Rock and a Hard Place drawn just now (told by its bones): the M416's handguard isn't there to hold.
bool LauncherInHand();
// Any other gun in hand (skin.cpp DrawHeldPart): one the mod has no model of, an enemy's dropped gun, with the game's own hold and reload moved whole to the right controller.
bool HeldGunInHand();
// The game's own move playing on it, a reload above all: its palms are off where they rest on it, and its hands are followed.
bool HeldGameMoving();
// Where the game's palms rest on it, in its frame's space. False until they've been seen to rest.
bool HeldPalms(float* right, float* left);
// It's a gun with no magazine button that the mod has measured (handmag.h): its numbers; null otherwise.
const handmag::Gun* HeldMagGun();
// The Stinger in hand, reloaded by the left hand: its aerial's tables (flap.cpp); null otherwise.
const flap::Aerial* HeldAerial();
bool HeldByHand();
// It's a tube shotgun whose shells go in by hand (the M1014; shellload.h): its places on its frame; null otherwise (or with launcherhands 0).
const shellload::Gun* HeldTubeGun();
// ... weapon.txt "tubebyhand 0" turns its reload by hand off (the game's own reload, as before); "shelltest N" a scripted hand (hands.cpp), "shelllog 1" its state
// each frame.
bool TubeByHand();
int ShellTest();
bool ShellLog();
// Its collision shape's name (gunshape::Use); null if it hasn't one.
const char* HeldShape();
// Its seated magazine has come free in the hand (none falls), or been knocked out by a fresh one: it flies at this speed (the view's space), tumbling.
void PullMagazine();
void FlickMagazine(const float* velocityView);
// weapon.txt "heldtest 1": a scripted left hand on it, with nobody in the headset: 1 pulled out by hand, a new one rocked in, the lever from over the top; 2 the Afghan reload; 3 the lever
// from over the top alone; 4 from below.
int HeldTest();
// weapon.txt "launcherhands 0": the launcher all as the game's animation has it, the left hand on it and the game's reload; on by
// default: the left hand free, onto the front grip with a squeeze, the rockets loaded by hand (rocket.cpp).
bool LauncherHands();
// "launchersteer 0": the left hand on its front grip doesn't point it.
bool LauncherSteer();
// The right hand's frame (the gloves' bone 32) as the game holds the gun that's in hand this frame: on its body (the rifle) or on its frame (launcher set: the launcher). False: not known yet.
bool RightHandOnGun(float* rel, bool& launcher);
// The game's own left hand's frame (the gloves' bone 1, the right's 32 mirrored) on the frame of the launcher-pose gun in hand, as this frame's draws of the arms place it
// while the left isn't the player's (DrawLauncherRig): where its arms rest on the gun, or as the game's own moves have it now (gameHands: HandTargets::launcherGameHands
// as this frame's is set). False: no launcher-pose gun's arms this frame, or the hand on it isn't known yet.
bool LeftHandOnRig(bool gameHands, float* rel);
// "rockettest 1": a scripted left hand takes a new case and holds it, for pictures with nobody in the headset; "rockettest 3": on the front grip;
// "rockettest 4": the whole reload, with the wrist; "rockettest 5": the same, a pull and a wait alone.
int RocketTest();
// "rocketgame": as X held with the launcher empty, the game's own reload (once).
bool TakeRocketGameTest();
// The game's own left glove on the launcher as its animation has it just now: each of its bones (handpose.cpp's 0-22) from the gloves'
// bind space into the launcher's frame. False until drawn with the launcher in hand.
bool LauncherLeftGrip(float (*bones)[12]);
// The gun in hand's sights as its own row has them, for recoil's sight at the eye (design 6): the M416's, a held gun's (kHeldKinds). False for
// none (the SMAW, the Stinger, the train pistol, a gun not drawn by the mod, a gun no hand holds). Render thread, from the last drawn frames (under
// g_handsMutex); logged once a gun and sight.
struct SightNow {
    float line[3] = {};      // its sights' line at its muzzle (model): where its bullets start (kRifleSightsAtMuzzle, HeldKind::sights); on a held gun
                             // with a sight of kHeldSights drawn on it, that sight's line instead (through its glass, along the gun's)
    float up = 0.0f;         // degrees the line lies below z (RifleUp, HeldKind::up)
    float eyeZ = 0.13f;      // the game's zoomed eye along z (the M416 0.148; the held guns 0.122-0.134)
    float power = 1.0f;      // the magnified sight drawn on it last frame (1: irons or a reflex, or the magnifying switched off)
    int sight = -1;          // which: kHeldSights' row (0 PSO-1, 1 PKS-07, 2 PK-A, 3 PK-AS, 4 Kobra, 5 EoTech), 10 the M416's ACOG, -1 none (irons)
    float glassZ = 0.0f;     // its rear glass along z (model), when haveGlass: any sight with glass drawn on it, magnifying or not (the Kobra's too)
    bool haveGlass = false;
    float rearZ = 0.0f;      // the gun's nearest point on that line toward the eye, along z (model): its rear glass; with none, where the gun itself
                             // first comes within 1 cm of the line (its rear sight; a flat top's, the rear of its rail)
    uint32_t id = 0;         // which gun: the M416's colour draw's index count (12768), a held gun's (kHeldKinds' bodyIndices)
};
bool Sight(SightNow& out);
// recoil's hold on the gun in hand, for its scatter (design 6), each frame: oneHand, a long gun held by its pistol grip alone, as recoil.h has it this
// frame (Output::hipSpread: its support hand's blend under a half; false for two hands, a pistol, a launcher, nothing held, recoil off or its one-hand
// scatter off). One hand brings back the game's own unzoomed scatter on the guns whose scatter ClearRifleSpread took down to the zoomed one (even a
// first round strays); two hands keep the zoomed one. recoil.h has settled it already (a change of hold lasting 0.2 s); this only lets a change through
// once it has been asked for without a break for 50 ms (a frame recoil clears in never writes); with no call for half a second it's two hands again.
// weapon.txt "riflespread 0" wins. Render thread.
void HoldSpread(bool oneHand);
// Degrees its frame's z lies above the iron sights' line (measured zoomed: 0.7; weapon.txt "launcherup" tunes it): the tube is
// raised by this, so the sights look along the aim and the rocket goes where they point.
float LauncherUp();
// The M416's alike (weapon.txt "rifleup"; zoomed, the game's aim on its body is 0.29 degrees below its z).
float RifleUp();
// The game's zoom held with it (weapon.txt "launcherzoom 1", a test: the user wants to free aim, so the spread is taken out
// of its data instead: ClearLauncherSpread).
bool LauncherZoomWanted();

// The game's own left glove as its animation last held the gun: each of its bones (handpose.cpp's
// 0-22) from the gloves' bind space into the gun's model space. Made by the game's animators for
// this gun, launcher and all, so it doesn't clip; hands.cpp puts it on the free gun while the left
// hand holds the handguard. False until learned once.
bool GameLeftGrip(float (*bones)[12]);
// Its left glove on the grenade launcher's own pistol grip under the rifle, as the game holds it with the launcher in hand (the hand's second hold). False until learned once.
bool GameLeftGripM320(float (*bones)[12]);
// The game's own right glove on the rifle's pistol grip, as the gloves' draw last learned it: each of its bones (23-44,
// bones[handpose::kRightBones], [bone - 23]) from the gloves' bind space into the rifle's model space. The left glove is
// drawn on the grip with it mirrored (hands.cpp). False until every one of them has been learned.
bool GameRightGrip(float (*bones)[12]);
// The game's own right glove (bones 23-44, [bone - 23]) / left glove (0-22) on the launcher's or the held gun's frame as its arms rest on it (RigSet's kept
// rest: the hold, never a frame of its reload), from the gloves' bind space into the frame's. kind: whose rest it is (0 the launcher's, else the held gun's
// colour draw's index count), so the gun before's isn't taken for this one's; palm: the right palm's point on the frame, as kHeldKinds' rightPalm was
// measured. False until a rest is known.
bool RigRestRight(float (*bones)[12], uint32_t& kind, float* palm);
bool RigRestLeft(float (*bones)[12], uint32_t& kind);
// The held gun in hand (DrawHeldPart's), read once under the hands' lock: whether one is drawn last (HeldGunInHand), its kind (its colour draw's index
// count, 0 none), where the game's palms rest on it (as HeldPalms writes them: palms is its return, but false from a new gun's first draw till the rig
// has taken it, while its kind is still the gun's before, or none), its sights' up (LauncherUp's, degrees), its shape.
struct HeldNowOut {
    bool held = false;
    uint32_t id = 0;
    bool palms = false;
    float rightPalm[3] = {}, leftPalm[3] = {};
    float up = 0.0f;
    const char* shape = nullptr;
};
void HeldNow(HeldNowOut& out);
// The attachments drawn with the held gun in hand in the last 2 frames (gunbodies::kAttach's draws), each one's points on the gun's frame, at most
// "most" in all; set changes when the set of attachments does. How many points.
int AttachmentsOnGun(float (*points)[3], int most, uint32_t& set);
// The M416's grenade launcher has been met with the rifle on this level (its tube's draw, M320PartOf's part 1). Sticky while the rifle is out; forgotten
// after 10 s with no rifle drawn. False: free hands give the M416 its body without the M320.
bool RifleM320Seen();

// The loose things (loose.h) as hands.cpp's step left them this frame, in the arms' space with origin (whole metres): each draw places each one (magazines in the guns' draws,
// the launcher's case in its parts'). The latest draw's head -> the arms' space as the world's picture has it, with that space's origin (false till a draw's been placed). The
// game's own eye in the world (the soldier's: its feet are the stance's eye height below). weapon.txt "loose 0" turns them off (the old straight fall).
bool LatestSeenFrom(float* seenFrom, float* origin);
bool GameEyeWorld(float* out);
bool LooseOn();

}

namespace loose {
struct Result;
struct Spec;
struct HandAt;
}
namespace skin {
void SetLoose(const loose::Result& r, const int* origin);
// A magazine held in either hand let go of: side 0 is DropHeldMagazine(velocityView) exactly (hand ignored); side 1 that hand now (its grip in the room,
// its controller's own speed and turn: hands.cpp's), carried on by its move since, released by loose::ReleaseBy.
bool DropHeldMagazine(const float* velocityView, int side, const loose::HandAt* hand);
// The grenade launcher's round as loose has it, drawn with the round's own mesh (kind 3): its spec, and the case's own space (its base's middle at 0, along +z) on the
// round's bone (1440's bone 0, as m320.h's): the case's pose = the round bone's pose times this.
const loose::Spec* M320CaseSpec();
void M320CaseOnRound(float* out);
// The loose copies drawn (every pass and eye) since the last ask, for the log.
int TakeLooseDraws();
// The grenade launcher's round as the game's own animation has it now: its bone (1440's bone 0) on the gun's frame bone (HandTargets::gunPose's), false if not drawn in the last
// 0.2 s. And the old case of the game's own reload let go of (loose has it): the round drawn nowhere until it's set back.
bool M320RoundOnGun(float* rel);
void SetM320CaseDropped(bool dropped);
// weapon.txt "m320test N" (m320.h's scripted tests) and "m320log 1" (its state each frame); the rifle held seated in the car (its parts pinned: no reload by hand there).
int M320Test();
bool M320Log();
bool RifleSeated();
}

#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include "handpose.h"

struct ID3D11Resource;
struct ID3D11DeviceContext;

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
// A magazine held in the left hand (HandTargets::mag) let go of: it falls from where it was last
// drawn in the hand (this last tenth of a second), starting at velocityView (m/s in the view's space;
// none: from rest). False if it hasn't been drawn there lately.
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

// Free hands (hands.cpp): where to draw the gun and the left hand. Matrices are three rows,
// rotation | translation, from the gun's model space / the gloves' bind space into the view's
// own space: the eye at the origin, x right, y up, z back, metres. Each draw places that in the
// arms' space with the eye and axes of the view-projection uploaded for it, so the hands stay put
// relative to the picture however the arms' space moves.
struct HandTargets {
    bool gun = false, left = false;  // which of these is tracked
    float gunPose[12] = {};
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
    // into the view's space).
    bool launcherLeftFree = false;
    bool launcherGameHands = false;  // the game's own reload of it plays (or none of the mod's): its hands are followed on it, wherever they go
    int launcherCase = 0;
    float launcherCasePose[12] = {};
    float rightWrist[3] = {}, leftWrist[3] = {};  // each hand's grip point
    // Each of the left glove's bones bent as its fingers are held, in the gloves' bind space
    // (handpose.cpp): the glove is drawn with leftPose composed with these.
    float leftFingers[handpose::kBones][12] = {};
    // A magazine in the left hand or on its way into the well (pouch.cpp): where the gun's body
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
};
void SetHands(const HandTargets* targets);  // nullptr = off: the game's own arms

// Where the game's own right hand holds the gun, in the gun's model space (from the hand's and the
// gun's bones as the game animates them). False until both have been drawn.
bool GunGrip(float* point);
// The launcher of Rock and a Hard Place drawn just now (told by its bones): the M416's handguard isn't there to hold.
bool LauncherInHand();
// weapon.txt "launcherhands 0": the launcher all as the game's animation has it, the left hand on it and the game's reload; on by
// default: the left hand free, onto the front grip with a squeeze, the rockets loaded by hand (rocket.cpp).
bool LauncherHands();
// "launchersteer 0": the left hand on its front grip doesn't point it.
bool LauncherSteer();
// The right hand's frame (the gloves' bone 32) as the game holds the gun that's in hand this frame: on its body (the rifle) or on its frame (launcher set: the launcher). False: not known yet.
bool RightHandOnGun(float* rel, bool& launcher);
// "rockettest 1": a scripted left hand takes a new case and holds it, for pictures with nobody in the headset; "rockettest 3": on the front grip;
// "rockettest 4": the whole reload, with the wrist; "rockettest 5": the same, a pull and a wait alone.
int RocketTest();
// "rocketgame": as X held with the launcher empty, the game's own reload (once).
bool TakeRocketGameTest();
// The game's own left glove on the launcher as its animation has it just now: each of its bones (handpose.cpp's 0-22) from the gloves'
// bind space into the launcher's frame. False until drawn with the launcher in hand.
bool LauncherLeftGrip(float (*bones)[12]);
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

}

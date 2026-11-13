#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

#include "config.h"

namespace camera {

// Frostbite LinearTransform: four rows of float4 (right, up, forward, translation).
struct Transform {
    float right[4];
    float up[4];
    float forward[4];
    float trans[4];
};

// memcpy that returns false instead of crashing on unmapped or protected memory.
bool SafeRead(uintptr_t address, void* out, size_t size);

// Follows a pointer chain to the address it ends at.
bool Resolve(const config::CameraPointer& pointer, uintptr_t& address);

// Follows the first pointer chain from bf3vr.ini. Returns false if it isn't set or doesn't resolve right now.
bool Read(Transform& out);

// Logs the camera transform and the write watch about once a second.
void Probe();

// Puts hardware write breakpoints on up to four camera copies in every game thread. Each time
// the game writes one, the current eye offset is added straight after, before anything renders it.
void StartWatch();

// Sideways shift along the camera's right vector, in metres (negative = left).
void SetEyeOffset(float metres);

// With a controller aiming, the game camera points where the hand points. This turns the render
// cameras so the picture shows where the head looks instead. Both in the recentered OpenXR space.
struct ViewTurn {
    float head[4];        // head orientation quaternion x, y, z, w
    // Without a look yaw: the yaw taken out of each camera as it is (radians, turning left positive), so the
    // picture follows the game's camera (xr_runtime.cpp holds it at the yaw the last picture took out).
    float controllerYaw;
    // Without lag guessing: the controller yaw and BF3's look yaw written this frame, and which
    // way the look yaw moves for a controller turn left. Each camera then reads the look yaw it
    // was built from out of its own matrix, so the hand's turn it includes is known exactly: the
    // picture shows lookYaw + currentYaw - the head (lookSign -1), whatever the game's camera did.
    bool haveLook;
    float currentYaw;
    float lookYaw;
    float lookSign;
    // Mouse aim holding its corrections for the search's nudge this frame (aim::Nudging): weapon.txt "viewlog"
    // marks those frames.
    bool nudging;
    // For the head's place (headmove.h), published with its turn so a frame never mixes the two: the eye centre in the
    // room (metres), and whether the headset knows it this frame; the way the game walks, from the room's straight
    // ahead (radians, turning left positive); a cutscene or the game turning the view itself.
    float headPlace[3];
    bool placeTracked;
    float facingYaw;
    bool scriptedHint;
    // Which of xr_runtime.cpp's located head poses this turn is (TakeDrawnPose).
    uint32_t poseId = 0;
    // Where the head faces, for what must not take the picture's pose's every step (x, y, z, w): the picture's own pose (head, above) as ever, or beside a pose candidate
    // ("poseadapt", "poserate") the located turn through two low-passes with no lead, as xr_runtime.cpp's aim limit and walk take it. The rows are always turned by head.
    float smoothHead[4] = {0.0f, 0.0f, 0.0f, 1.0f};
};
// nullptr = off (render cameras keep the game's own direction).
void SetViewTurn(const ViewTurn* turn);

// While the view is turned, the weapon camera ([aim] game_camera, which the gun model is placed
// from) is turned the opposite way so the gun stays with the hand. If the gun swings away from
// the hand instead, the direction is the other way round: flip = true.
void SetGunFlip(bool flip);

// Research (weapon.txt "wcam x y z"): the weapon camera moved by offset along its own right, up and
// back axes (metres) each time the game writes it, watched in every mode; nullptr = off.
void SetWeaponTest(const float* offset);

// The field of view to make the game render with, in radians (0 = the game's own). Written into
// the render and cull views and their source each time a camera breakpoint fires.
void SetRenderFov(float radians);
float WantedFov();
// weapon.txt "renderfov N" (a test): the eyes' pictures drawn N degrees wide while the line is there, on top of bf3vr.ini's wider view; 0 none.
void SetTestFov(float degrees);
float TestFov();

// Per-eye views (views.cpp) turn and shift the render and cull cameras where the game builds them:
// the breakpoints then leave those alone (still watching them, for RenderedFov) and only turn the
// weapon camera.
void SetViewsMode(bool on);
// A camera transform turned to the head, as the breakpoints turn the render cameras. False (and
// unchanged) while no view turn is set, or if it isn't a camera. A scripted camera (a cutscene's or a
// ride's, its rows' w lanes filled: viewturn.h) is turned too, made level about its yaw with the head
// put on it, its w lanes left as they are; weapon.txt "cutsceneturn 0" refuses it as before. body: the
// view's first-person camera (+0x930) when transform is the view's main camera (views.cpp), or nullptr.
// With a scripted camera and no look angles written, it's turned by the same turn about its own place
// (the turn as the main camera's own frame has it, put on it in its own: viewturn.h TurnWith); with the
// player's kind it's left as it is (the mod places the gun and arms itself), and with a refused camera too.
// The body isn't drawn from it: the main camera's turn goes to skin.cpp as well (BodyTurns), which turns
// the views the body is drawn with as they're uploaded.
bool TurnToHead(float* transform, float* body = nullptr);
// A view build's turn of its main camera, for the body lock (skin.cpp): build, BeginViews' count of view
// builds; at, when (GetTickCount64); turn, the main camera's turn as its own frame has it (viewturn.h
// OwnFrameTurn: turn[3 * row + k], the turned row's part along the game's row k); before and after, its
// rows (right, up, back) as the game made them and as turned; place, where the game put it (the world);
// firstPersonMade and firstPerson, the view's first-person camera's rows as the game made them and as
// turned with it (zeros if it isn't a camera).
struct BodyTurn {
    uint32_t build = 0;
    uint64_t at = 0;
    float turn[9] = {};
    float before[9] = {}, after[9] = {};
    float place[3] = {};
    float firstPersonMade[9] = {}, firstPerson[9] = {};
    // How far the head's place moved that build's cameras (headmove.h; metres, the world's axes): the body's views go
    // along, or the body would be carried with the head.
    float move[3] = {};
};
// The last view builds' turns, the newest first, up to most of them: only while the newest build turned a
// scripted main camera (a cutscene's or a ride's) with no look angles written, within the last quarter of
// a second, and then as many before it as did too, in a row. 0 in a normal picture, an unturned one, with
// "cutsceneturn 0", or with the look angles written (the mod places the gun and arms itself then). Any
// thread.
int BodyTurns(BodyTurn* out, int most);
// Whether BodyTurns has any: two atomics read, for every upload to ask cheaply. Any thread.
bool BodyTurnWanted();
// views.cpp, once a frame before its TurnToHead calls (on the same thread): the turn published now is the one
// they all use, so a turn published in between can't give the two eyes and their cull cameras different ones.
void BeginViews();
// Which head pose each picture was really drawn with. The view builder runs on its own thread, so whether a build gets
// the pose published at the last Present or the one before it depends on where that thread is when the render thread
// comes out of xrWaitFrame: a fixed number of frames back ([stereo] pose_lag) was right while the game took the whole
// frame and waited for nothing, and wrong some frames and right others once it had 5 ms to spare (the user, 2026-11-04:
// the view jittered with every turn of the head). So each build notes its main camera's rows and the pose it was turned
// by, and the frame's own main views, as they're uploaded (skin.cpp), name the build it comes from.
//
// Every 352-byte per-view upload read as view constants comes here, as the game made it: its camera's rows (right, up, back) and its eye. Any thread. A main view is
// one in the world (its eye past the arms' space's reach of the origin, where the game's first-person views are) whose rows fit a build kept. A picture's main-view
// uploads do not all carry one build: with the head turning, in every picture whose first one carried the build before the newest, exactly eight did and all the
// others (14 to 34) carried the newest (2026-11-12, a replayed head). Three things are made of them:
//  - The picture's name, and with it the pose the headset is told, the gun and arms' correction (HandsOffDrawn) and the age the log says: by weapon.txt "nameby".
//    0, the default: the build the frame's FIRST upload fits (the oldest not shown yet where the builds are alike), exactly as it has been. 1: the build its last
//    main-view upload before the first eye's picture went out carried (the left eye's passes end there). 2: the build most of its main-view uploads carried. For 1
//    and 2 an upload whose rows fit several builds counts for the newest of them (a head standing still, or come back to where it was two builds ago: a picture's
//    views carry the newest build or the one before, never an older one). A picture
//    whose passes really are drawn with two builds' cameras cannot be told right whatever it is named: the headset turns the whole picture by one angle. It is named
//    best by the cameras its world was drawn with, which is what 2 is after. By the game's code as read, the passes of an eye's world take their view where the
//    render thread sets the frame's passes up, from the cameras the newest build left in the renderer, so they should carry the newest; which draws the eight feed,
//    and where in the frame they stand, had not been seen when this was written ("Picture order" shows it).
//  - The count, while a head test runs or weapon.txt "viewlog" is on, once a second: "Picture age: ..." (how many uploads carried which build, before the first
//    eye's picture, between the two and after; of the pictures named by a first upload of the build before, in how many the last upload before the first eye's
//    picture carried the newest) and "Picture order: ..." (one picture's uploads in the order they came, a letter each). The head has to be turning: builds of a
//    still head are alike, and their uploads are counted as fitting more than one (builds alike that were all turned with one pose, the game having built its
//    views twice between two Presents, count as the newest of them: the pose is the same whichever a pass was drawn with). uploads: the calling thread's count of
//    small uploads so far, this one among them (0: not counted). Those that come between one per-view upload and the next are the per-draw constants of what is drawn with that view, near enough: both lines
//    say how many each kind carried, which tells a view the world is drawn with from one that little or nothing is.
//  - The world's turn (DrawnWorldView), always: the rows themselves, for xr_runtime.cpp to hold against the pose it tells the headset.
void NoteDrawnView(const float* rows, const double* eye, uint32_t uploads = 0);
bool DrawnNamed();  // the frame has had an upload that fits a build
// An eye's finished picture is going onto the window (overlay.cpp: 0 the first eye's, the left; 1 the second's): the main-view uploads before the first are the left
// eye's passes', and the last of them is what "nameby 1" and the world's turn go by. Any thread.
void NoteEyePicture(int picture);
void SetNameBy(int rule);
int NameBy();
// The picture TakeDrawnPose last closed, by its own main-view uploads: the camera rows, as the game uploaded them, of the last of them before the first eye's
// picture went out (marked; else that moment was not marked, or nothing came before it, and they are the frame's last), of its first, and of the last that carried
// the build most of them carried; whether they carried more than one build; whether its gun and arms were placed for another build than it was named by. With each
// of the three goes the view build its rows fit (the newest of them where builds are alike): that build's own rows and the head's turn (x, y, z, w) it was turned
// with. A build's rows are its body's turn times its head's, so the camera any pose stands for with that body is those rows with their own head taken off and the
// pose put on (headtest.h CameraOfPose): the pose told is held against the cameras drawn by the body they were drawn with, and what is left is the head's turn
// between the two poses alone, however the stick or a scripted camera turned the body between two builds. False: it had no main-view upload.
struct DrawnWorld {
    struct Build {
        bool have = false;  // (false: that build is kept no more)
        float rows[9] = {}, head[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    };
    float rows[9] = {};
    bool marked = false;
    bool haveFirst = false, haveMost = false;
    float firstRows[9] = {}, mostRows[9] = {};
    Build build, firstBuild, mostBuild;
    bool mixed = false, handsOff = false;
};
bool DrawnWorldView(DrawnWorld& out);
// xr_runtime.cpp, once a Present: the pose the picture about to be shown was drawn with, by the build it is named by ("nameby"). False if nothing named it.
bool TakeDrawnPose(uint32_t& poseId);
// The pose the next picture's views were built with, if they have been (after TakeDrawnPose): what the hands' places
// must be measured from, to be right in that picture. False: not built yet, it will take the pose published next.
bool NextDrawnPose(uint32_t& poseId);
// weapon.txt "posebydraw 0": the old fixed guess, pose_lag, to compare.
void SetPoseByDraw(bool on);
bool PoseByDraw();
void SetPureRows(bool on);  // weapon.txt "purerows": viewturn::PureRows, on by default
// The head as located, and tries on it (xr_runtime.cpp), weapon.txt: "posefilter hz" and "placefilter hz" its turn and its place low-passed (0 off), the picture and the layer alike; "poselead ms" the
// filtered turn carried that far on by its smoothed speed; "poseahead ms" the head and hands located that much later than the frame's display time; "posetest degrees" the layer told a pose turned
// that much every other second, the picture not (2026-11-04: the world jumped, so the headset goes by the layer's pose); "freezeview 1" the last picture held, nothing new shown (a head shaken
// under it: does the headset hold a still picture still?); "edgefill 0" black round the picture (edgefill.h); "fillshot n" with the headset idle, when n changes, the filled image to eye_fill.bmp.
// "poseadapt 1": the turn's low-pass opens with the head's own speed (a still head as steady as "posefilter" makes it, a fast one not held back) and the turn is carried on for the picture's
// measured age in place of "poselead"; a number above 1 is that many ms carried on instead of the measured age. 0: the fixed filter and lead.
// "poserate 1": the turn carried from frame to frame by the head's own turn rate as the runtime gives it, and only pulled towards the located turn at "posefilter", then carried on by
// that rate for the pictures' age: no delay to make up and no more of the located head's unevenness passed on than two low-passes let through (in place of the fixed filter and of
// "poseadapt"; a frame without a rate keeps the filter beside it). "poserate 2": the same, the rate carried on by its own change for as late as it is given. A negative number: the
// rate taken as the head itself has it, not as the room has it. 0: off. With "poseadapt" or "poserate" on, the aim's limit and the walk's direction go by the head's turn through the
// fixed filter's two low-passes without any lead, not by the picture's pose.
enum PoseTries { kPoseFilter, kPlaceFilter, kPoseAhead, kPoseTest, kPoseLead, kFreezeView, kEdgeFill, kFillShot, kPoseAdapt, kPoseRate, kPoseTryCount };
void SetPoseTries(int which, float value);
float PoseTry(int which);
// The pose the hands' places were measured from (xr_runtime.cpp, as they're worked out), and the rigid move, in the head's space (rows, 3 x 4), that puts them right for the picture being drawn
// if that's another pose after all: the game draws the newest build or the one before it, flipping up to 9 times a second, and each time the gun and arms were a head's step off for a picture
// (1.7 degrees at 120 degrees a second). False: the same pose, or not known yet. The picture being drawn goes by the build its first upload named ("nameby 0"); with
// "nameby" 1 or 2, where the name comes only once the picture is drawn, by the build as far from the one its first upload fits as the last picture's name was
// from its own first upload's.
void SetHandsPose(uint32_t poseId);
bool HandsOffDrawn(float* rows);
// The head test's meters (xr_runtime.cpp, headtest.h). With detail asked for, each view build also keeps its main camera as the game made it (its rows and its place), the look yaw and the hand's
// yaw it was turned with, and how far the head's place moved it (NoteBodyMove): the camera a picture was really drawn from, read back beside the pose it was drawn with. Off (as it is unless a head
// test runs) nothing more is kept, and nothing more is done than one flag looked at.
void SetDrawnDetail(bool on);
struct DrawnCamera {
    uint32_t build = 0, poseId = 0;  // the view build's serial, and the head pose it was turned by
    float rows[9] = {};              // its main camera's rows as drawn (right, up, back): the head's turn is in them
    float gameRows[9] = {};          // and as the game made them
    float gamePlace[3] = {};         // where the game put it (the world, metres)
    float move[3] = {};              // how far the head's place and the eye's arm moved it (headmove.h): both eyes are drawn from gamePlace + move, each half the eye distance out along its right
    bool look = false;               // turned from the look yaw written (ViewTurn::haveLook)
    float lookYaw = 0.0f, handYaw = 0.0f;  // ViewTurn's lookYaw and currentYaw (radians)
};
// The camera of the picture TakeDrawnPose last named: the very picture about to be shown. False: its build kept no detail (or none was named: the caller knows).
bool DrawnCameraNamed(DrawnCamera& out);
// The newest build turned by that pose: for a picture nothing named, which xr_runtime.cpp takes to be as old as the last one. False: none kept.
bool DrawnCameraOfPose(uint32_t poseId, DrawnCamera& out);
// weapon.txt "headwave 30 2": with nobody in the headset, the head turned 30 degrees each way every 2 s (radians now).
void SetHeadWave(float degrees, float seconds);
float HeadWaveYaw();
// This view build's body, from its main camera turned to the head (headmove.h, on the view builder's thread, after
// that): the level frame the room's axes stand in, in the world (its right and back along the ground: the game camera
// less the hand's yaw and the game's pitch, or a scripted camera as made level), whether that camera is a scripted one,
// and the turn it was given. False if no main camera was turned this build.
bool FrameBody(float* right, float* back, bool& scripted, ViewTurn& turn);
// The head's place moved this build's main camera so far (headmove.h): for the body lock's views (BodyTurn::move).
void NoteBodyMove(const float* move);
// The latest frame's first camera turned (the left eye's): its yaw as the game built it and the yaw the turn
// took out of it (radians; shown = camera + taken - head, in the look yaw's sense); for a scripted camera (a
// cutscene's or a ride's), its yaw as made level and nothing taken out. False before any, and when none was
// turned for a quarter of a second (a cutscene drawn unturned, the view turn off, theater mode): it's stale then.
// A scripted camera's picture, turned to the head, keeps it fresh.
bool ShownTurn(float& cameraYaw, float& takenYaw);
// How many cameras TurnToHead turned since the last call (xr_runtime.cpp, once a picture: none means
// the game drew it facing its own way, as in the cutscenes of The Great Destroyer). scripted: how many
// of them were scripted cameras (a cutscene's or a ride's), turned about their own yaw.
int TakeTurned(int* scripted = nullptr);
// weapon.txt "viewlog N": for N seconds, log each frame's view turn (hand, written look, game
// camera's yaw and place, head, shown).
void LogTurns(int seconds);

// weapon.txt "mirrorfix 0/1": a camera whose rows are mirrored from the player's own (a cutscene's)
// gets the head's turn mirrored to match (on by default).
void SetMirrorFix(bool on);

// weapon.txt "cutsceneturn 0/1": a scripted camera (a cutscene's or a ride's, its rows' w lanes filled)
// turned to the head about its own yaw (on by default), or refused, the picture held still in the room as
// before. Logged each time it's read.
void SetCutsceneTurn(bool on);

// The game's own field of view this frame (radians; 0 if unreadable), from bf3.exe+1F8B4F8.
float SourceFov();
float SourceFovRaw();  // whatever's there, a long scope's narrow one too

// The FOV (radians) the game really drew the last frame with: the wider one while the render view
// is watched and widened, the game's own while it isn't (BF3 moves the render camera in memory,
// and after the opening ride the saved pointer chain stopped finding it). 0 if unknown.
float RenderedFov();

// The look angles the weapon camera ([aim] game_camera) was last written with by the game, before the
// mod turns it, as the player's look angles store them: yaw atan2 of its back row's x and -z, pitch
// -asin of its y (radians). False if it hasn't been written in the last 200 ms. Any thread.
bool GameWeaponAngles(float& yaw, float& pitch);

// How many times the game built its main render camera since the last call.
uint32_t TakeCameraBuilds();

// From the render view's projection matrix: depth = depthScale - nearPlane / distance (so
// distance = nearPlane / (depthScale - depth)). False, leaving them as they are, if unreadable.
bool ProjectionDepth(float& nearPlane, float& depthScale);
// Cull hunt: BF3 keeps dozens of identical camera copies, and decides what to draw from one that
// isn't turned with the head, so scenery goes missing when the hand aims away. The finder (F10)
// hands over every copy it saw; F6 then turns them with the head one at a time, a few seconds
// each, and F5 saves the one that makes the missing scenery come back.
void SetTestCopies(const std::vector<uintptr_t>& copies);
// F6: start the hunt, or stop it without saving.
void ToggleHunt();
// Every frame: moves the hunt on to the next copy when its time is up.
void UpdateHunt();
// The copy being tried right now; 0 when not hunting.
uintptr_t HuntCandidate();
void StopHunt();

}

#pragma once

#include <cstdint>

// Per-eye views. BF3 keeps a developer setting, GameRenderSettings.SplitScreenTestViewCount, that
// draws a second complete view beside the first every frame, from the same game state (found
// 2026-10-30: the frame's views are built by one function, 0x01018E70 in bf3.exe 1.6, which names
// them "Root" and "SecondaryRoot" and gives each half of the picture; the second one's camera is
// the first turned round by a test angle). Hooked here, the two become the eyes: the left eye in
// the left half, the right eye in the right half, each its own camera, as FrostbiteVR renders
// Bad Company 2 (no picture made up from depth, so no halo).
namespace views {

// Every frame, from the render thread. want: draw the two eye views (the per-eye stereo mode) or
// the game's single view. Hooks the view builder once bf3.exe's code is in place (it's decrypted
// at start-up) and sets the view count.
void Update(bool want);

// Whether the last frame was built as two eye views.
bool Active();

// Each eye's offset along the head's right, in metres (left negative), for the next frames.
void SetEyeOffsets(float left, float right);

// The offset (metres along the head's right) of eye 0 or 1, as last set.
float EyeOffset(int eye);

// The vertical field of view (radians) the eyes were last built with; 0 when not active.
float RenderedFov();

// weapon.txt "nearplane": the eyes' near plane in metres, 0 the game's own (6 cm).
void SetNearPlane(float metres);
float NearPlane();

// weapon.txt "offcentre X Y" (a test): each eye's picture drawn off-centre, by the game's own means. Its window is moved outward (the left eye's to the left, the right eye's to the
// right) by X of its own width and down by Y of its height; its size and the angle it is made from stay as they are. The headset's lens shows more outward and downward than a centred
// picture reaches. 0 0: centred, nothing written (the default). Each is kept within 0.45.
void SetOffCentre(float outward, float down);

// weapon.txt "lodfov N" (a test): the eyes' level of detail picked as for a picture N degrees wide, however wide it is drawn; 0 by the angle drawn (the default).
void SetDetailFov(float degrees);
// bf3vr.ini [stereo] detail_fov: the same for good; weapon.txt's line wins while it is there. 0: by the angle drawn.
void SetDetailBase(float degrees);

// How far eye 0 or 1's window was moved when the eyes were last built, as the game's projection takes it: shares of the window's own width and height, to the right and up. False: the
// mod moved nothing, the picture is as centred as the game makes it. What the headset is told of that eye's picture is made from these.
bool OffCentre(int eye, float& right, float& up);

// Whether the right eye's view is drawn at the window's left edge (like the left eye's), its picture
// to be moved into the right half as it goes onto the window (overlay.cpp).
bool RightEyeAtLeft();

// weapon.txt "viewdiff N": for N seconds, log where the two views' descriptions differ outside
// their cameras, whenever that changes (to find the screen effects only the right eye gets).
void WatchDifferences(int seconds);

// The last build's cameras, for the HUD's world markers and its heading (markers.cpp): the game's own main camera as the game built it, before the mod
// turned it (in VR the gun's aim, at the game's own angle), and the head's (the eyes' midpoint, their turn, their angle). Rows: right, up, back (the camera
// looks along -back). False before the first build of two views. Any thread.
struct BuiltCameras {
    float gameRows[9] = {}, gameEye[3] = {};
    float gameFov = 0.0f, gameAspect = 0.0f;  // radians top to bottom; width over height
    // The game camera's rows of this build were the mod's own left in it at the build before (headmove.h ArmRowsStale: now and then with the headset on,
    // every build in some launches without it): the head's, not the game camera's. Nothing is to be learned of the game camera from them (the review of
    // 2e323f2); its eye is this build's all the same.
    bool rowsStale = false;
    float headRows[9] = {}, headEye[3] = {};
    float eyeFov = 0.0f, eyeAspect = 0.0f;
    float nearPlane = 0.0f, farPlane = 0.0f;
    bool offCentre = false;  // the eyes' pictures off-centre (weapon.txt "offcentre")
    uint32_t build = 0;
    uint64_t at = 0;  // GetTickCount64 of the build
};
bool LastCameras(BuiltCameras& out);

// The size of each eye's picture in pixels, as the views' viewports were last built (the first view's: the eyes' are alike, each a square half of the
// window); false (0 0) while the eyes aren't drawn. What the game draws an eye's own scene with: skyrays.cpp takes the sky only into a picture that size.
bool EyeSize(int& width, int& height);

}

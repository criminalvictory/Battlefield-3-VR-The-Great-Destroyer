#pragma once
#include <windows.h>

#include <cstdint>
#include <vector>

namespace config {

enum class Mode { Theater, Stereo };

struct Settings {
    Mode mode = Mode::Theater;

    // Theater screen placement, in metres.
    float screenDistance = 2.5f;
    float screenWidth = 3.2f;

    // Vertical FOV the game renders with (BF3's FOV slider), used to tell the
    // compositor what each eye image covers.
    float gameVerticalFovDeg = 55.0f;

    // Field of view the mod makes the game render with, in degrees, top to bottom and across
    // (the window is square); 0 = BF3's own (55 in the campaign, whatever the menu says). While
    // set, the headset image uses it instead of gameVerticalFovDeg (Delete toggles, PgUp/PgDn tune).
    float renderFovDeg = 0.0f;
    float renderFovLastDeg = 90.0f;  // what Delete turns it back on at

    // The angle the eyes' level of detail is picked by, in degrees ([stereo] detail_fov). Drawn wider, the game picks coarser models and terrain for the same
    // thing at the same place; held at the angle it was at (100) it picks as it did. 0 = the angle the picture is drawn with.
    float detailFovDeg = 0.0f;

    // The picture's sharpness held whatever its size ([stereo] pixels_per_tangent): pixels for each unit of the tangent of the angle from the picture's middle. Above 0,
    // renderFovDeg is made from [window] size at the start (2 atan(size / 2 / this)): a bigger picture is then a wider one, and its middle is the smaller picture pixel for
    // pixel. 2160 pixels over 100 degrees is 906.2271. 0 = renderFovDeg as bf3vr.ini has it.
    float pixelsPerTangent = 0.0f;

    // The vignette and the suppression blur's clear middle held as at 100 degrees when the picture is drawn wider ([stereo] pin_effects, postpin.h): the same
    // darkening at the same angle from the middle. On by default only when renderFovDeg is wider than 100.
    bool pinEffects = false;

    // Stereo mode: show only the left eye in the desktop window, so it doesn't flicker
    // between the eyes. The headset is unaffected.
    bool mirrorOneEye = true;

    // Per-eye stereo: the desktop window, and OBS's game capture with it, shows one view (the left eye's) instead
    // of both eyes side by side ([stereo] stream_view; Home in game), stream_fov degrees across, drawn for a
    // stream_aspect shape squeezed into the window (16:9: OBS's "stretch to screen" shows it in proportion; 0 = the
    // window's own), steadied with the horizon level (stream_steady), looking stream_tilt degrees below where the
    // head looks (streamview.cpp).
    bool streamView = true;
    float streamFovDeg = 85.0f;
    float streamAspect = 16.0f / 9.0f;
    bool streamFit = false;  // stream_fit: the whole eye's picture fitted in, black at its sides
    bool streamSteady = true;
    float streamTiltDeg = 0.0f;

    // Stereo mode head tracking: head turns and tilts are added to the player's look angles,
    // so the game looks (and aims) where the head looks. The mouse still turns the body.
    bool headTracking = true;
    // The head's place in the room moves the view (headmove.h, [head] position, walk, crouch): lean, duck, step, crouch
    // for real. lean, lean_most: metres along the ground moved one to one, and at most (inside the soldier's collision
    // body, so not through a wall). follow_start: how far the neck goes from the soldier before it's walked after it.
    // stick_full: metres a second a whole stick walks.
    bool headMove = true, headWalk = true, headCrouch = true;
    float headLean = 0.25f, headLeanMost = 0.32f, headFollowStart = 0.25f, headStickFull = 3.0f;

    // Stereo mode: aim and fire with the right controller instead of the head; the head then
    // only turns the picture.
    bool controllerAim = true;
    // Right stick turning at full deflection: in stereo, degrees a second (written into the look
    // yaw); in theater mode, mouse counts per 72nd of a second. 266 degrees is what 15 counts made
    // with the game's mouse sensitivity on the test setup.
    float turnDegrees = 266.0f;
    float turnSpeed = 15.0f;
    // Walking by the left stick as an Xbox controller's (any direction, any speed), where the head
    // faces; 0 = W/A/S/D (eight directions, one speed).
    bool analogMove = true;
    // Which way the gun model is swung to follow the hand (F1 flips it).
    bool gunFlip = false;
    // How far the controller's aim may stray from where the head looks, in degrees. BF3 only
    // draws what's in front of its aim, so further means more missing scenery.
    float maxAimOffsetDeg = 30.0f;
    // Frames by which the view turn lags the hand, to match when BF3 builds its camera. Only a
    // fallback now: camera.cpp reads the look yaw each camera was built from.
    int viewLag = 0;
    // Stereo: how many frames old the head pose each eye image is submitted with (F11, 0-3).
    int poseLag = 1;
    // Hand frames to SteamVR on a thread of their own, so the game doesn't wait about 7 ms a
    // frame for it ([stereo] async_handover).
    bool asyncHandover = false;
    // SteamVR's 32-bit OpenXR runtime for the game whenever the PC has it and the active one is another
    // ([general] prefer_steamvr; xrpick.h). false: the OpenXR loader's own choice.
    bool preferSteamVr = true;

    // Windowed mode: keep BF3's window this many pixels square (0 = leave it alone).
    int windowSize = 1600;
    // The window fitted to the monitor, borderless, in the shape of the picture it shows, the game told it's
    // still windowSize big (window.cpp); false = the window as big as the game draws ([window] fit).
    bool windowFit = true;
    // The desktop window waits for the monitor's refresh, as the game asks ([window] vsync).
    bool desktopVsync = false;
    // The game's constant-buffer uploads written into a copy of the mod's own, read there and copied to the card at Unmap ([render]
    // upload_copy: auto, on, off). auto: only where the card's memory reads slowly (AMD's write-combined video memory: skin.cpp).
    int uploadCopy = 0;  // 0 auto, 1 off, 2 on

    // Campaign: seconds a dead soldier's body stays before the game removes it (0 = the game's own,
    // 15 s for most soldiers).
    float corpseSeconds = 0.0f;
    // Campaign, the game's dropped guns (pickups.h). How much of it is on: 2 = a hand can take one off the ground; 1 = the game's own pickup alone
    // (X held), with the mod's care of it (your own gun kept where it lies, the true rounds carried over, X held sending R only when there is
    // something to take); 0 = off, nothing of the game's hooked, X held as before. Seconds a dropped gun stays (0 = the game's own TimeToLive, 180 s
    // for a soldier's; -1 = never, the nearest 48 kept); whether one at the end of its time goes only while nobody looks at it; and the same count
    // for the player's own gun once the game has swapped it into a dropped gun's place (-1 = kept until it's taken, 0 = as any other, N seconds).
    int pickups = 0;
    float pickupSeconds = 0.0f;
    bool pickupUnseen = false;
    float pickupOwnSeconds = -1.0f;

    // The game's memory for one frame's draws, in MB ([world] frame_memory_mb, framemem.h): both eyes and a scope's view
    // draw out of it, and the game stops when a frame runs out of it (it ships with 16, sized for one view).
    // 0 = the game's own 16, nothing hooked and nothing measured.
    int frameMemoryMb = 64;

    // The eyes' sun shadow maps' size ([world] shadowmap_resolution, scopeview.h): 0 = the game's own (2048 at its highest setting). With a picture drawn wider than
    // 100 degrees each map is spread over more ground; raised by as much, the shadows' edges stay as they were.
    int shadowMapSize = 0;

    // The eyes' motion blur passes ([world] motion_blur_passes): 1 = as the game has them, 0 = switched off. Only for a game whose motion blur amount is 0 (the passes
    // then draw nothing that shows, and cost about 4.5 points of the graphics card's load).
    int motionBlurPasses = 1;
};

// Cheat Engine style pointer chain: address = bf3.exe + base, then for each offset:
// address = [address] + offset. base == 0 means not set.
struct CameraPointer {
    uintptr_t base = 0;
    std::vector<uintptr_t> offsets;
};

// The player's look angles, found by the look-angle finder (F3).
struct Aim {
    // The game's own camera (not the render copies): the finder reads its forward vector to
    // recognise the angles. Defaults to the one found for bf3.exe 1.6.0.0.
    CameraPointer gameCamera;
    CameraPointer yaw;
    CameraPointer pitch;
    bool yawDegrees = false;
    bool pitchDegrees = false;
    // Which way a head turn moves each angle; F4 / F2 flip them in game.
    bool invertYaw = false;  // false: the hand's yaw goes in negated, BF3's way (aim.cpp)
    bool invertPitch = false;
};

// BF3's FOV setting, found by the FOV finder (Home), so the mod can set it past the menu's 90.
struct Fov {
    // How a value is stored: 0 = float degrees, 1 = float radians, 2 = float half-angle degrees,
    // 3 = float half-angle radians, 4 = int degrees.
    struct Target {
        CameraPointer pointer;
        int encoding = 0;
    };
    std::vector<Target> targets;
    // The FOV to set, in the same units as BF3's menu slider. 0 = leave BF3's own value.
    float value = 110.0f;
};

// God mode (campaign only): god.cpp finds the player's health itself.
struct God {
    bool enabled = true;
    float value = 100000.0f;  // what health and maximum health are kept at
};

// Reads bf3vr.ini next to the mod DLL, writing a default one if it doesn't exist.
void Load(HMODULE self);
const Settings& Get();
Settings& Mutable();
// Writes the current gameVerticalFovDeg to bf3vr.ini (tuned in game with Page Up / Page Down), and to bf3vr.player.ini beside it: the
// player's own view, never in a release's zip, read after bf3vr.ini and over it (an update unzipped over the folder keeps it).
void SaveFov();
// Writes the current renderFovDeg and renderFovLastDeg to bf3vr.ini and bf3vr.player.ini (Delete, PgUp/PgDn in game).
void SaveRenderFov();
// Writes the current gunFlip to bf3vr.ini (F1 in game).
void SaveGunFlip();
// Writes the current viewLag to bf3vr.ini.
void SaveViewLag();
// Writes the current poseLag to bf3vr.ini (F11 in game).
void SavePoseLag();
// Writes the current streamView to bf3vr.ini (Home in game).
void SaveStreamView();
// bf3vr.ini [general] dev_keys=1: the developer's keys (F1, F5-F7, F10-F12, Delete, Pause, Ctrl+F2/F3/F4, Ctrl+Scroll Lock)
// work (Page Up / Page Down are the player's again: they only scale the picture). Read from the file at each call (no restart); off when the line isn't there.
bool DevKeys();

// Up to four camera pointers: BF3 keeps several copies of the view (renderer, weapon, ...) and
// the eye shift goes to all of them. The first is the one the camera probe logs.
// They can be replaced at any time by the finder threads, so they're kept separately.
constexpr size_t kMaxCameras = 4;
std::vector<CameraPointer> GetCameras();
CameraPointer GetCamera();
// Replaces the camera pointers and writes them to bf3vr.ini.
void SetCameras(const std::vector<CameraPointer>& pointers);
void SetCamera(const CameraPointer& pointer);

// The camera copy BF3 decides what to draw from (found with the F6 hunt). It gets the same head
// turn as the render cameras, so scenery isn't skipped when the hand aims away from the gaze.
CameraPointer GetCull();
void SetCull(const CameraPointer& pointer);

// Look angles: read and replace (written to bf3vr.ini).
Aim GetAim();
void SetAim(const Aim& aim);

// God mode: read and replace (written to bf3vr.ini).
God GetGod();
void SetGod(const God& god);

// FOV override: read and replace (written to bf3vr.ini).
constexpr size_t kMaxFovTargets = 8;
Fov GetFov();
void SetFov(const Fov& fov);

}

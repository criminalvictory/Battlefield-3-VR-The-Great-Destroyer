#pragma once
// The other controllers' OpenXR profiles, beside the Quest's Touch, the Valve Index's and the Steam Frame's (input.cpp's own): the HP Reverb G2,
// Windows Mixed Reality's motion controllers, HTC Vive's wands, the Vive Cosmos, the Vive Focus 3 and the Pico 4. Each is mapped as the Touch is
// (the buttons table in the README): the right trigger fires, the left aims down sights or works a handle, the left stick walks and its click
// sprints, the right stick turns (crouch and prone by its flicks) and its click jumps (held: recenter), the magazine and use buttons where X and Y
// are, the grips hold. Where a controller has no X and Y (Windows Mixed Reality, the Vive's wands) its trackpads' clicks and its menu buttons take
// their place; the Vive's wands walk on their left trackpad and turn on their right one, clicked (PadTurn). A grip that's a button (squeeze/click)
// is bound to the same float action as a squeezed one: the runtime gives 0 or 1 (OpenXR's boolean to float), past the grips' 0.6 and under their
// 0.4 alike. Every path below is one the OpenXR 1.0 specification (or its controller extension) lists for that profile; tools/xrprofilestest.cpp
// checks each against those lists. A profile's extension is enabled only where the runtime offers it (xr_runtime.cpp); a runtime that doesn't know
// a profile refuses it and the others stand. bf3vr.ini [controllers] other_controllers=0: none of them. Pure, header only.

#include <cstdint>
#include <cstring>

namespace xrprofiles {

// The mod's actions a profile binds (input.cpp's, by name; the grip poses and the haptics every profile gets anyway).
enum Action : uint8_t {
    kAim,         // the right hand's aim pose
    kFire,        // the right trigger
    kAds,         // the left trigger
    kMove,        // the walking stick (a 2D input)
    kTurn,        // the turning stick (2D): its flicks crouch and stand
    kJump,        // A
    kCrouch,      // B (bound as on the Touch; the right stick crouches now)
    kReload,      // the magazine button (X on the Touch): tap drops, hold reloads
    kUse,         // Y on the Touch: the game's use; held, switches weapons
    kSelectSwap,  // the Touch's left menu button: rifle or launcher
    kSprint,      // the walking stick's click
    kRecenter,    // the turning stick's click: short a jump, held a recenter
    kActionCount
};

struct Binding {
    Action action;
    const char* path;
};

// The grips: each hand's squeeze (a float action, a button's 0 or 1 too) and the thumb resting on its stick (nullptr: the controller can't tell).
struct Grips {
    const char* leftSqueeze;
    const char* rightSqueeze;
    const char* leftTouch;
    const char* rightTouch;
};

constexpr int kMostBindings = 14;
constexpr int kGripTries = 2;

struct Profile {
    const char* path;      // the interaction profile
    const char* name;      // for the log and the README
    const char* layout;    // its buttons against the Touch's, for the log
    int extension;         // its index in kExtensions; -1: OpenXR 1.0's own
    Binding binding[kMostBindings];
    int count;
    Grips grips[kGripTries];  // tried in order: the next only when the runtime refuses the one before (leftSqueeze nullptr: no more)
    bool padTurn = false;     // it turns on a trackpad (kTurn's path a trackpad's, kRecenter its click): read as PadTurn says
};

// The extensions the profiles need, enabled where the runtime offers them (xr_runtime.cpp), as bits by their index.
constexpr int kExtensionCount = 4;
constexpr const char* kExtensions[kExtensionCount] = {
    "XR_EXT_hp_mixed_reality_controller",
    "XR_HTC_vive_cosmos_controller_interaction",
    "XR_HTC_vive_focus3_controller_interaction",
    "XR_BD_controller_interaction",
};

// The Touch's layout on a controller with A/B on the right and X/Y on the left (and a left menu button).
#define XRPROFILES_TOUCH_LAYOUT                                                                                                                       \
    {kAim, "/user/hand/right/input/aim/pose"}, {kFire, "/user/hand/right/input/trigger/value"}, {kAds, "/user/hand/left/input/trigger/value"},      \
        {kMove, "/user/hand/left/input/thumbstick"}, {kTurn, "/user/hand/right/input/thumbstick"}, {kJump, "/user/hand/right/input/a/click"},       \
        {kCrouch, "/user/hand/right/input/b/click"}, {kReload, "/user/hand/left/input/x/click"}, {kUse, "/user/hand/left/input/y/click"},          \
        {kSelectSwap, "/user/hand/left/input/menu/click"}, {kSprint, "/user/hand/left/input/thumbstick/click"},                                    \
        {kRecenter, "/user/hand/right/input/thumbstick/click"}

constexpr Profile kProfiles[] = {
    // HP Reverb G2: the Touch's layout; its grips are squeezed (a value), its sticks sense no thumb.
    {"/interaction_profiles/hp/mixed_reality_controller",
     "HP Reverb G2",
     "as the Touch's",
     0,
     {XRPROFILES_TOUCH_LAYOUT},
     12,
     {{"/user/hand/left/input/squeeze/value", "/user/hand/right/input/squeeze/value", nullptr, nullptr}, {}}},
    // Windows Mixed Reality's motion controllers: no A, B, X or Y. The left trackpad's click is the magazine (X), the left menu button use (Y; held,
    // a switch of weapon), the right trackpad's click a jump (A), the right menu button rifle or launcher. Their grips are buttons.
    {"/interaction_profiles/microsoft/motion_controller",
     "Windows Mixed Reality",
     "no A, B, X or Y: the left trackpad's click the magazine, the left menu button use, the right trackpad's click a jump, the right menu button "
     "rifle or launcher",
     -1,
     {{kAim, "/user/hand/right/input/aim/pose"},
      {kFire, "/user/hand/right/input/trigger/value"},
      {kAds, "/user/hand/left/input/trigger/value"},
      {kMove, "/user/hand/left/input/thumbstick"},
      {kTurn, "/user/hand/right/input/thumbstick"},
      {kJump, "/user/hand/right/input/trackpad/click"},
      {kReload, "/user/hand/left/input/trackpad/click"},
      {kUse, "/user/hand/left/input/menu/click"},
      {kSelectSwap, "/user/hand/right/input/menu/click"},
      {kSprint, "/user/hand/left/input/thumbstick/click"},
      {kRecenter, "/user/hand/right/input/thumbstick/click"}},
     11,
     {{"/user/hand/left/input/squeeze/click", "/user/hand/right/input/squeeze/click", nullptr, nullptr}, {}}},
    // HTC Vive's wands: no sticks. The left trackpad walks where the thumb is on it (its click sprints); the right one is pressed as four buttons
    // round a fifth (PadTurn): clicked at its left or right edge it turns, at its bottom crouches (held there: prone), at its top stands, in its
    // middle jumps (held 1.5 s: a recenter); a thumb resting on it does nothing. The left menu button is the magazine (X), the right one use (Y;
    // held, a switch of weapon). Their grips are buttons; a thumb on a trackpad is a thumb on its stick.
    {"/interaction_profiles/htc/vive_controller",
     "HTC Vive",
     "no sticks: the left trackpad walks (its click sprints), the right one is clicked at its edges to turn, crouch (held: prone) or stand, in its "
     "middle to jump (held: recenter); the left menu button the magazine, the right one use",
     -1,
     {{kAim, "/user/hand/right/input/aim/pose"},
      {kFire, "/user/hand/right/input/trigger/value"},
      {kAds, "/user/hand/left/input/trigger/value"},
      {kMove, "/user/hand/left/input/trackpad"},
      {kTurn, "/user/hand/right/input/trackpad"},
      {kReload, "/user/hand/left/input/menu/click"},
      {kUse, "/user/hand/right/input/menu/click"},
      {kSprint, "/user/hand/left/input/trackpad/click"},
      {kRecenter, "/user/hand/right/input/trackpad/click"}},
     9,
     {{"/user/hand/left/input/squeeze/click", "/user/hand/right/input/squeeze/click", "/user/hand/left/input/trackpad/touch",
       "/user/hand/right/input/trackpad/touch"},
      {}},
     true},
    // HTC Vive Cosmos: the Touch's layout; its grips are buttons.
    {"/interaction_profiles/htc/vive_cosmos_controller",
     "HTC Vive Cosmos",
     "as the Touch's",
     1,
     {XRPROFILES_TOUCH_LAYOUT},
     12,
     {{"/user/hand/left/input/squeeze/click", "/user/hand/right/input/squeeze/click", "/user/hand/left/input/thumbstick/touch",
       "/user/hand/right/input/thumbstick/touch"},
      {}}},
    // HTC Vive Focus 3: the Touch's layout; its grips squeezed where the runtime gives the value (the extension's second revision), else buttons.
    {"/interaction_profiles/htc/vive_focus3_controller",
     "HTC Vive Focus 3",
     "as the Touch's",
     2,
     {XRPROFILES_TOUCH_LAYOUT},
     12,
     {{"/user/hand/left/input/squeeze/value", "/user/hand/right/input/squeeze/value", "/user/hand/left/input/thumbstick/touch",
       "/user/hand/right/input/thumbstick/touch"},
      {"/user/hand/left/input/squeeze/click", "/user/hand/right/input/squeeze/click", "/user/hand/left/input/thumbstick/touch",
       "/user/hand/right/input/thumbstick/touch"}}},
    // Pico 4: the Touch's layout; its grips squeezed (a value), else buttons.
    {"/interaction_profiles/bytedance/pico4_controller",
     "Pico 4",
     "as the Touch's",
     3,
     {XRPROFILES_TOUCH_LAYOUT},
     12,
     {{"/user/hand/left/input/squeeze/value", "/user/hand/right/input/squeeze/value", "/user/hand/left/input/thumbstick/touch",
       "/user/hand/right/input/thumbstick/touch"},
      {"/user/hand/left/input/squeeze/click", "/user/hand/right/input/squeeze/click", "/user/hand/left/input/thumbstick/touch",
       "/user/hand/right/input/thumbstick/touch"}}},
};
#undef XRPROFILES_TOUCH_LAYOUT

constexpr int kProfileCount = static_cast<int>(sizeof(kProfiles) / sizeof(kProfiles[0]));

// A profile's extension enabled (bits by kExtensions' index), or none needed.
inline bool Usable(const Profile& p, unsigned enabled) {
    return p.extension < 0 || (p.extension < kExtensionCount && (enabled & (1u << p.extension)) != 0);
}

// The aim pose's path of each hand (the grips' place from it where a runtime gives the aim alone: input.cpp), every profile's.
constexpr const char* kAimPose[2] = {"/user/hand/left/input/aim/pose", "/user/hand/right/input/aim/pose"};

// A turning trackpad's place as input.cpp takes the turning stick's (Profile::padTurn: the Vive's wands). A trackpad says where a thumb rests on it,
// pressed or not, and taken as a stick's (the review of 2026-11-25) a thumb resting off its middle turned, near its bottom edge crouched and held
// there lay down, and a jump's click near an edge crouched too. So its place counts only while it's clicked away from its middle, middle being the
// right stick's kJumpStickMost (a click with the stick pushed further is never a jump): clicked at an edge it turns, crouches (held: prone) or
// stands as the stick pushed there would; clicked in its middle it's (0, 0), the click alone (a jump; held, a recenter); untouched or only touched,
// (0, 0).
struct Pad {
    float x = 0.0f, y = 0.0f;
};
inline Pad PadTurn(float x, float y, bool clicked, float middle) {
    Pad p;
    if (clicked && x * x + y * y >= middle * middle)
        p.x = x, p.y = y;
    return p;
}
// The right hand's held click counts towards a recenter (input.cpp: held 1.5 s) only in a turning trackpad's middle: a turn held at its edge
// never recenters. pushed: the trackpad's place as last read through PadTurn (its length). On a stick (padTurn false), every held click, as ever.
inline bool HeldForRecenter(bool held, bool padTurn, float pushed, float middle) {
    return held && !(padTurn && pushed >= middle);
}
// The runtime's current profile for the right hand (its path as text) one of these that turns on a trackpad.
inline bool TurnsOnPad(const char* path) {
    for (const Profile& p : kProfiles)
        if (p.padTurn && path && std::strcmp(p.path, path) == 0)
            return true;
    return false;
}

}  // namespace xrprofiles

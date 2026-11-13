#pragma once
// Recoil for every gun in the game (design 6), the game's side: build\recoil.txt's switches; the gun in hand's own numbers read from its data and
// its per-shot kick taken out of there (and put back as it was); its shots; this frame's kick (recoil.h); its pulses (input::Buzz); the scope
// picture's slack (scopeview::SetKickSlack); the check that the game's own kick really is out; the logs. Render thread (hands::Update).

#include <cstdint>

#include "recoil.h"

namespace recoilrun {

constexpr uint32_t kTrainPistol = 1;  // the train pistol's id (no gun's id is 1)

// hands::Update, first: build\recoil.txt looked at every 300 ms and read again whole when it changes; deleted, every line is back to its default.
void Read();

struct In {
    float dt = 0.0f;              // seconds since the last frame, as hands.cpp steps them (at most 0.1)
    float gap = 0.0f;             // the real seconds since the last frame, never capped (0: not given); the longer of it and recoilrun's own clock clears
    int pose = 0;                 // 0 nothing drawn, 1 the M416 (gunPose), 2 a launcher-pose gun (launcherPose), 3 the train pistol (pistolPose)
    uint32_t id = 0;              // which gun: the drawn gun's id (the M416's, the SMAW's, a held gun's), kTrainPistol
    const char* shape = nullptr;  // skin::HeldShape() (a held gun), for the table when its data has no gun sway
    int slot = -1;                // its slot in the soldier's weapons (the M416 and the held rifles ammo::HandSlot's, 0 or 1; 1 the Stinger, 5 the SMAW;
                                  // the train pistol 0)
    bool held = false;            // held by its pistol grip, drawn, no scripted moment: else the kick clears
    bool leftTrigger = false;     // the left hand on its pistol grip fires it
    float hands = 0.0f;           // 0-1: the support hand's blend onto its front (two hands)
    bool frontGrip = false;       // that hand on a front grip (the M320's, a launcher's), not a handguard
    int support = -1;             // which hand holds the front (0 left, 1 right), -1 none
    float pivot[3] = {};          // the trigger hand's anchor on the gun (model)
    float eyes[2][3] = {};        // both eyes in the gun's model space (the pose as held)
    int extraShots = 0;           // rounds hands.cpp saw leave: the SMAW's, the Stinger's
    int stance = 0;               // input::StanceIndex(): 0 standing, 1 crouched, 2 prone
    const float* heldPose = nullptr;  // the pose as held (12 floats, rows: the gun's model space into the view's), for the aim's clamp log; null: none
};
// This frame's kick for the gun in hand: the drawn pose = the pose as held times out.k (the identity with recoil off, nothing held, before the
// first shot, or another gun drawn while its change waits out its frames). Also sends its pulses and the scope's slack. The reference stays good
// until the next call.
const recoil::Output& Step(const In& in);
bool On();     // no "off" line in build\recoil.txt
void Clear();  // a new life (hands::HeldChambered(true)), free hands toggled (hands::Toggle): the kick gone at once
// Each Present, whatever is drawn (ammo::Update): a gun whose kick is out of its data gets it back once Step hasn't run for a second (free hands
// off, the headset not rendering), so the game's own kick is never missing where the mod draws none.
void Watch();

}

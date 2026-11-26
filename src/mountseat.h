#pragma once
// The mounted gun in the game (docs/research/mounted_gun.md; the law in src/mountgun.h, the seat's reader in src/seatread.h): the seat read each frame
// once its reader is measured and borne out, the gun told by its rows, the seat's one state that every gate reads.
//
// Today: OFF. The reader's places are unmeasured (the runbook's R3), every gun's rows are unmeasured (R1 to R6), and step 1's two parts that need the game
// (the seat's own view: the body fixed to the vehicle with the eye at the gunner's place; the steering's outputs: the pad's right stick or the mouse) are
// not built. So Seated() is false everywhere, every gate passes everything as before, and the log says once a run why (the first row missing, and from
// which step it must come).

#include "mountgun.h"

namespace mountseat {

// Each frame on the render thread (xr_runtime.cpp, beside input::WatchPose), the headset on or not.
void Frame();

// The seat's state now. Any thread.
mountgun::State State();
// In a gun's seat (its climb-in or the gun): the single state the gates read. False whenever the seat isn't proved: on foot, in any other seat, the
// reader not measured or not borne out, the gun's rows not all measured, step 1's runtime not built.
bool Seated();
// What the seat holds quiet now (mountgun::Quiet on this state). Any thread.
bool Quiet(mountgun::What w);

// The right stick's turn this frame while seated (input::TakeTurn's: radians, left positive; xr_runtime.cpp): the body's turn about the ring's axis
// (mountgun::OrbitTurn) for the seat's view. Never the mouse's (mountgun::StickDirect).
void NoteStickTurn(float radians);

// The fire while seated (input.cpp's fire mapping): either trigger pulled, its grip squeezed (step 1's hold of a handle); mountgun::FireStep. Render thread.
bool FireDown(const float trigger[2], const float squeeze[2]);

}  // namespace mountseat

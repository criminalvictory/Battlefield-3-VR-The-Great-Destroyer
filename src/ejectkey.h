#pragma once
// The Steam Frame's left D-pad right as X (input.cpp), for the rifle held by the left hand on its pistol grip: the magazine
// button under the thumb of the hand on the trigger (the user, 2026-11-12: "the best way to eject would probably be right
// d-pad"). Whether a press counts is judged on the frame it goes down and kept until it's let go, so the gun changing hands
// during a press never starts or cuts one. Pure, header only (tools/grasptest.cpp checks it).

namespace ejectkey {

// build\freehands.txt "dpadeject N": 0 the D-pad's right does nothing; 1 (the default, and any other number) X's job with the
// gun in the left hand by its pistol grip; 2 X's job whoever holds the gun (also with it lying: X's pickup hold).
constexpr int kOff = 0, kLeftGrip = 1, kAnyHold = 2;

struct State {
    bool down = false;    // the D-pad's right held last frame
    bool counts = false;  // ... and that press counts as X
};

struct Out {
    bool press = false;   // held this frame and counting: OR-ed into X
    bool edge = false;    // went down this frame (judged now)
    bool counts = false;  // the press under way (or just let go) counts
    bool letGo = false;   // let go this frame (counts: whether the press that ended counted)
};

// dpad: the D-pad's right held this frame (the controller's, or a test round's); mode: "dpadeject"; gripHand: the hand on the
// gun's pistol grip (hands::GripHand: 0 the left, 1 the right, -1 nobody: lying, gliding into a hand, held by a handguard alone).
inline Out Step(State& s, bool dpad, int mode, int gripHand) {
    Out o;
    if (dpad && !s.down) {
        o.edge = true;
        s.counts = mode == kAnyHold || (mode != kOff && gripHand == 0);
    }
    o.letGo = !dpad && s.down;
    o.counts = s.counts;
    o.press = dpad && s.counts;
    if (!dpad)
        s.counts = false;
    s.down = dpad;
    return o;
}

}  // namespace ejectkey

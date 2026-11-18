#pragma once
// What the game's quick-time events await, read from the game each frame (qtelist.h: its list of live input triggers) for the grips' version 2
// (qte.h, input.cpp).

namespace qtewatch {

struct Frame {
    unsigned await = 0;     // the keys awaited now (qte::Key bits); 0: nothing, or the list couldn't be found or read
    unsigned hold = 0;      // those of them held rather than tapped
    unsigned seen = 0;      // the keys the game's own input state shows down
    bool seenRead = false;  // ... that state was read
    bool read = false;      // the list was read (a manager's, or none: between levels)
};

// Each frame with weapon.txt "qte 2" (input.cpp, the render thread). scripted: version 1's scripted moment (a cutscene's camera, or the game
// turning the look with no gun in hand as that began): play's own Fire, Jump and Interact triggers count only then. The manager's pointer is
// found by its signature in the game's code (the first calls; a part a frame if it isn't where 1.6.0.0 has it) and trusted by the vtables alone;
// every read is guarded. Not found or not readable: said once in the log, and nothing awaited. A trigger appearing or leaving is logged.
Frame Update(bool scripted);

}  // namespace qtewatch

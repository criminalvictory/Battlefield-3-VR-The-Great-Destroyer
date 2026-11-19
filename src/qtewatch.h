#pragma once
// What the game's quick-time events await, read from the game each frame (qtelist.h: its list of live input triggers; qtefight.h: a fist fight's
// open prompt, from its prefab's gates) for the grips' version 2 (qte.h, input.cpp).

namespace qtewatch {

struct Frame {
    unsigned await = 0;     // the keys awaited now (qte::Key bits); 0: nothing, or the list couldn't be found or read
    unsigned hold = 0;      // those of them held rather than tapped
    unsigned seen = 0;      // the keys the game's own input state shows down
    bool seenRead = false;  // ... that state was read
    bool read = false;      // the list was read (a manager's, or none: between levels)
    // A fist fight (qtefight.h): while one's prompt is up, await is its one key (or nothing) and every other quick-time key the mod would send
    // is held up (input.cpp's guard).
    bool prompt = false;    // a fight's prompt is up (its right or wrong gate open, the fight running it: its fail gate open, not standing a minute)
    unsigned answer = 0;    // ... the one key it awaits (0: none safe to press: two prompts, a wrong gate)
    bool mash = false;      // ... a mash (pressed again every 130 ms while the grip stays squeezed), not a press (one tap a squeeze)
    bool unknown = false;   // the fights couldn't be read this frame (a fight not as attached, the list not read): nothing pressed, and the guard
                            // left as it was (a prompt may be up; it lapses by itself)
};

// Each frame with weapon.txt "qte 2" (input.cpp, the render thread). scripted: version 1's scripted moment (a cutscene's camera, or the game
// turning the look with no gun in hand as that began): play's own Fire, Jump and Interact triggers count only then. fights: weapon.txt
// "qtefight 1" (the default): a fight's open prompt read from its gates; 0: a fight's six quick-time triggers press nothing (as also when the
// reader's constants aren't borne out by the game's code). The manager's pointer is found by its signature in the game's code (the first
// calls; a part a frame if it isn't where 1.6.0.0 has it) and trusted by the vtables alone; every read is guarded. Not found or not readable:
// said once in the log, and nothing awaited. A trigger appearing or leaving is logged, and a fight's attach, prompts and presses.
Frame Update(bool scripted, bool fights);

}  // namespace qtewatch

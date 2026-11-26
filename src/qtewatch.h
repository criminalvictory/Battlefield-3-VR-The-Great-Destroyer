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
    unsigned answer = 0;    // ... the one key it awaits (0: none safe to press: two prompts, a wrong gate); never barred (Bar)
    bool mash = false;      // ... a mash (pressed again every 130 ms while the grip stays squeezed), not a press (one tap a squeeze)
    bool unknown = false;   // the fights couldn't be read this frame (a fight not as attached, the list not read): nothing pressed, and the guard
                            // left as it was (a prompt may be up; it lapses by itself)
    unsigned barred = 0;    // the keys awaited but barred (Update's barred, by Bar): never in await or hold
    unsigned quick = 0;     // no fight's prompt up: of await, the keys a quick-time action proper awaits (0x3C-0x41; not play's own Fire, Jump,
                            // Interact), none while a fight's triggers are live: those a grip squeezed already may press in a scripted moment (qte.h)
    unsigned proper = 0;    // no fight's prompt up: of await, the keys a quick-time action proper awaits, a fight's triggers live or not
                            // (qtelist::ProperKeys): E and Space go down as the quick-time actions' own keys, the rest as play's (input.cpp's QteLogical)
    unsigned gates = 0;     // a fight's prompt up: the keys of all its gates read open, right or wrong (unclear: whether another action's is open)
    int fights = 0;         // fist fights read this frame (attached, their gates read): qte::FightStep holds a fight's run between its prompts by it
};

// The keys the grips never press now (input.cpp's QteStep: the left mouse button while a launcher is in a hand, the SMAW, the Stinger, the AT4: a
// squeeze would fire it, and Operation Swordbreaker's hotel roof awaits Fire 0x07 with the AT4 out; its trigger fires it) taken out of what the
// frame awaits, whoever awaits them: a live trigger or a fist fight's open prompt. The one rule for both, and with the fights' guard: answer stays
// the prompt's own, so the guard (input.cpp's SendKey and SendMouseButton) still lets it through from whatever sends it, the trigger's own fire
// among them, and holds up every other quick-time key; only the grips' press of it is barred. Pure (tools/at4test.cpp t7).
inline void Bar(Frame& f, unsigned barred) {
    f.barred = f.await & barred;
    f.await &= ~barred;
    f.hold &= ~barred;
}

// Each frame with weapon.txt "qte 2" (input.cpp, the render thread). scripted: version 1's scripted moment (a cutscene's camera, or the game
// turning the look with no gun in hand as that began): play's own Fire, Jump and Interact triggers count only then. fights: weapon.txt
// "qtefight 1" (the default): a fight's open prompt read from its gates; 0: a fight's six quick-time triggers press nothing (as also when the
// reader's constants aren't borne out by the game's code). barred: keys never pressed now (Bar), said once when the game comes to await one.
// The manager's pointer is found by its signature in the game's code (the first calls; a part a frame if it isn't where 1.6.0.0 has it) and
// trusted by the vtables alone; every read is guarded. Not found or not readable: said once in the log, and nothing awaited. A trigger appearing
// or leaving is logged, and a fight's attach, prompts and presses.
Frame Update(bool scripted, bool fights, unsigned barred = 0);

}  // namespace qtewatch

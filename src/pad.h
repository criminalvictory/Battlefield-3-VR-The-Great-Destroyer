#pragma once

namespace pad {

// Walking by an Xbox controller's left stick, which BF3 reads through XInput: any direction, as
// fast as the stick is pushed (W/A/S/D only give eight directions at one speed). The mod's stick,
// turned from where the head faces into where the game faces, is put in as controller 0's left
// stick; nothing else of a controller is (no buttons, no right stick). Hooks XInputGetState in the
// game's own XInput the first time it's asked to.
//
// As the mod loads: the game finds its controllers as it starts, and controller 0 must be there by then.
void Install();

// Each frame: the move in the game's frame (x right, y forward, -1..1, length up to 1). False when
// XInput couldn't be hooked, or while the game isn't reading controller 0 (the keys are used then):
// some runs it stops asking for it (the user, 2026-11-01: "i cannot move ... after fighting in the
// interrogation room"; the same in two runs on 2026-10-31).
bool SetMove(float x, float y);

// Controller 0's buttons (XINPUT_GAMEPAD_*) held for a while: the game's actions only its pad has a binding for.
void PressButtons(unsigned short mask, unsigned milliseconds);

// Each frame (xr_runtime.cpp's OnPresent, headset or not; it looks once a second): the game's pad read with its window in the background too
// (globalpad.h: its ClientSettings.UseGlobalGamePadInput found, its constants borne out once, then made 1 where it's 0). bf3vr.ini [controllers]
// background_pad=0: left alone. Render thread.
void KeepGlobalInput();
// The last look found the setting on: the game reads the pad whatever window is in front (the stick may walk then; keys and clicks still wait for
// the game's window). Any thread.
bool GlobalInputOn();

// A controller the game reads through XInput (any of the four) with a button down or a trigger pulled within the last 0.2 s, as XInput itself gave
// it, before the mod's own buttons went in: for the fist fights' lines (a press from a real controller, or a pad another program makes, never passes
// the mod's guard, and a wrong quick-time key fails a fight). Read only.
struct Real {
    bool known = false;  // XInput is hooked: what its controllers give is seen
    bool any = false;    // ... one has a button down or a trigger pulled
    int index = -1;
    unsigned short buttons = 0;         // its XINPUT_GAMEPAD_* buttons
    unsigned char left = 0, right = 0;  // its triggers (0-255)
};
Real RealPressed();

}

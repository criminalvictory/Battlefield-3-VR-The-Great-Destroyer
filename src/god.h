#pragma once

#include <cstdint>

namespace god {

// Every frame: keeps the player's health and maximum health at [god] value in bf3vr.ini, in the
// game's own server (which decides deaths) and in the client's copy (which draws the screen).
// Campaign only. Scripted deaths, like leaving the area of operation, still happen.
void Apply();

// End: god mode on or off (saved).
void Toggle();

// The player's soldier in the game's own server, where the campaign's real state is (health,
// ammo), or 0. Render thread.
uintptr_t PlayerServerSoldier();

// The client's copy of the player's soldier (what the player sees and aims with), or 0.
uintptr_t PlayerClientSoldier();

// One more each time the player's soldier in the game's server is found anew (the log's "God mode: the server's soldier is at", god mode on or off):
// a new life, a checkpoint's load, a level. 0 before the first. Any thread.
unsigned SoldierSerial();

}

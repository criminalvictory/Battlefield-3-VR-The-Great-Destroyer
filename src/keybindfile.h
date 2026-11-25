#pragma once
// The player's key bindings as the mod uses them (keybind.cpp): the game's settings file read on a thread of the mod's own, once as it starts
// and again whenever the file changes. keybind.h has the rules.

#include "keybind.h"

namespace keybind {

// Once, from the mod's start thread: the first read (its line in the log) and the watch. bf3vr.ini [controllers] follow_bindings=0: nothing read,
// today's keys.
void Start();

// The keys now: today's until the file's been read (never null). Any thread; a table once handed out never changes.
const Table* Current();

}  // namespace keybind

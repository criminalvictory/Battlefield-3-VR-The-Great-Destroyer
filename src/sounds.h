#pragma once

namespace sounds {

// The game's own reload sounds, cut from a recording of its reload (tools/capture.cpp) into
// build\sounds\<name>.wav, played when the hands do the same by hand: "mag_out", "mag_in",
// "bolt" (the bolt going home), "charge" (the charging handle drawn back). pan: -1 left to 1
// right; the reloads play in the middle (user, 2026-10-30: not off to the gun's side). A file
// that isn't there plays nothing; a changed one is read again. Any thread.
void Play(const char* name, float pan = 0.0f);
// Whether that clip's there. Any thread.
bool Have(const char* name);
// At the mod's start: the sounds' thread starts, finds where the game's own sound plays (the
// output with this process's sound on it) and starts there, so the mod's sounds are heard where
// the game's are (a Quest player's video, 2026-11-17: none of the mod's sounds); `say` gets the
// launch report's line naming that output. They follow the game's sound to another output.
void Begin(void (*say)(const char*));
// How many clips build\sounds holds.
int Count();
// At the mod's start: any clip the mod plays that isn't in build\sounds (none at all: the game
// started without Start BF3 VR.bat, which makes them; or a newer version's: the mod updated in
// place, a new gun's clips beside the older set) is made now from the game's own files (gameDir,
// or bf3.exe's folder) by build\gamesounds.exe, hidden, at idle priority, its words in
// build\gamesounds.log. The list is gamesounds.exe's own (--list, some milliseconds) and only
// the missing are made (--missing: a few seconds to half a minute); a clip that's there is never
// made again. All on a thread of its own, so the start waits for none of it; a clip plays from
// the moment its file is there. `line` gets the launch report's lines (what's there, what is
// being made, what was made), `warn` its problems (they can't be made).
void MakeMissing(void (*line)(const char*), void (*warn)(const char*), const wchar_t* gameDir = nullptr);

}

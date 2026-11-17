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
// No clips (the game started without Start BF3 VR.bat, which makes them on the first start):
// build\gamesounds.exe makes them now from the game's own files (gameDir, or bf3.exe's folder),
// hidden, at idle priority, its words in build\gamesounds.log. 1 started, -1 gamesounds.exe
// isn't there, -2 it couldn't start.
int Make(const wchar_t* gameDir = nullptr);

}

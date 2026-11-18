#pragma once

namespace gunshape {

// Each gun's collision shape (tools/gunshape.cpp writes them from its mesh dump, to build\guns\<name>.bin next to the DLL; on a player's PC shapemake.h has it made the first time the gun is held; the user,
// 2026-11-05: "Give them all collision"): from a point in the gun's model
// space (x left, y up, z to the muzzle; metres), the distance to the gun's surface, negative inside, and the way out (a unit vector, the shape's gradient). False without the file, or off its grid (5 cm of
// air round the gun and more: nothing there touches it). Any thread once loaded; the first call loads it.
bool Distance(const float* p, float& distance, float* normal);
// The distance alone (a seventh of the work: no way out).
bool Distance(const float* p, float& distance);

// Which gun's shape Distance reads from now on. Loaded the first time it's asked for. False when it has none: Distance then says so, and nothing's kept out.
bool Use(const char* name);

// A shape made since it was found missing (shapemake.h): its file is loaded now, on the caller's thread, in place of the "none" kept for its name,
// so the next Use(name) has it. A shape already loaded is never replaced (Distance may be reading it). True when the name has a shape now.
bool Reload(const char* name);

// For tools (tools/shapemaketest.cpp): the folder the shapes are in (with its last backslash) instead of guns\ next to the module.
void SetFolder(const wchar_t* folder);

// For tools (tools/collidetest.cpp): the shape from the file at path instead of next to the module. True when it loaded.
bool Load(const wchar_t* path);

}

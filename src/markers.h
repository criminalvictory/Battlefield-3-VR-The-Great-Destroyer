#pragma once
// The HUD's world markers and its heading, for each eye (2026-11-25, build\designs\per_eye_effects.md section 6 and 5.3).
//
// The game places every 3D icon of the HUD (squadmates' names, objectives, a pickup's icon) from one view-projection handed to the icons' per-frame update
// (bf3.exe 1.6.0.0: 0x0094AB90, __thiscall, its first argument the matrix, RET 0Ch), and works out the player's heading for them by a helper reading the
// game camera (0x0093C230: atan2 of its back row, in st0). That helper is the icons' code's alone: its three callers are all in it (markermath.h
// kHeadingCallers, checked in bf3_mem.exe by tools/eyefxtest.cpp c2), so no minimap or compass of its own turns by it. In VR the game camera is the gun's
// aim at the game's own angle: a marker sits on its target only while the gun points where the head looks.
//
// weapon.txt "worldicons N": 0 (the default) nothing hooked; 1 the icons' matrix read and said every 10 s (which camera it is, its layout, its angle: from
// the matrix itself, markermath.h), nothing changed; 2 that and, once the matrix has been read as the game camera's (its axes within 3 degrees of the game
// camera's rows, five times), the head's own in its place, built the same way (its layout, its depth mapping) with the eyes' angle: each marker on its target
// in direction, at the HUD's depth (overlay.h huddepth). Never with the pictures off-centre ("offcentre"), never from a matrix not read whole.
// "hudheading N": 0 (the default) nothing hooked; 1 the game's heading compared with the same worked out from the game camera's rows, said every 10 s; 2
// that and, once one way of working it out has matched five times, the head's heading worked out that way in its place. Both learn only from builds whose
// game camera rows are the game's own (views.h rowsStale: a stale build neither counts nor breaks a run), and "worldicons 2" is meant with "hudheading 2"
// (the heading feeds the same icons' code: a line says so when it isn't).
namespace markers {

// From the frame loop (xr_runtime.cpp, after views::Update): the hooks made once their code is there (bf3.exe decrypts itself at start-up), when asked for.
void Update();

void SetIcons(int mode);
void SetHeading(int mode);

}  // namespace markers

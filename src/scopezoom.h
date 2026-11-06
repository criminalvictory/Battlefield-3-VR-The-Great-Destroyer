#pragma once

#include <d3d11.h>

#include "scopemarks.h"

namespace scopezoom {

// Magnified only inside the scope (the user, 2026-11-05: "actually magnify them please whether it's 2x or 6x.. get it to magnify ONLY in the scope"). When the scope's first see-through part is about to be
// drawn, the picture so far, the world and all, is what the lens looks at. It's copied, and a disc at the eyepiece reads that copy zoomed about the scope's line: for each pixel, which way the eye looks through
// the eyepiece, that way's angle from the line divided by the scope's power, where that lands in the picture. So what's seen through it is the same picture, nearer, for each eye from where that eye
// really is, and the rest of the view isn't touched. The game's own reticle and lens tint are drawn on top as usual.
struct Lens {
    float scope[12];   // the scope's axes (x left, y up, z along its line) into the arms' space (rows: rotation | translation)
    float vp[16];      // this draw's view-projection of the arms' space (rows)
    float eye[3];      // this eye in the arms' space
    float middle[3];   // the eyepiece's middle, in the scope's axes
    float radius;      // its radius, metres
    float power;       // 4 for 4x
    // Every line, number and range mark the sight has when the game zooms it (scopemarks.h): the game only draws them zoomed, on the sheet it keeps for them. Put in the view by the slope from its line, as
    // a real reticle: wherever the eye is behind the glass, a mark stays on what's under it.
    const scopemarks::Marks* marks = nullptr;
    // This draw's view (rows: right, up, back, place, into the arms' space) and eye, for a picture drawn along the sight's own line (scopeview.h).
    float viewToArms[12] = {};
    int eyeIndex = -1;
    bool haveView = false;
};

// weapon.txt "scopesharpen 0.35": how much the view through a lens has its edges steepened; 0: only its edge-following filter.
void SetSharpen(float amount);
// weapon.txt "scopeviewgain": the light of a picture drawn along the sight's line (scopeview.h) scaled, against the eyes'.
void SetOwnGain(float gain);
// The picture being drawn is the eye's scene, in its HDR target. The HUD's draws come later, into another, and some share the scopes' draws' numbers: a lens drawn then floated in the view.
bool InScene(ID3D11DeviceContext* ctx);
// The view through the scope goes on the game's own glass disc, drawn with its own vertex shader and its own depth test: so it's exactly the disc's shape, in the disc's place, and whatever of the scope is
// nearer the eye, its rim, covers it (the user: "embedded inside the scope", "I still want to see like the perimeter of the scope"). Begin copies the picture and puts the lens's pixel shader on; the caller
// draws the disc; End puts all back. False from Begin: nothing to do (too small to see into, no target it can read, or its shaders didn't build), and the glass is as the game draws it.
// marked: whether the sight's marks are in it yet (their map is made on the frame's own thread, a frame later): until they are, the game's own reticle isn't to be taken away.
bool Begin(ID3D11DeviceContext* ctx, const Lens& lens, bool* marked = nullptr);
void End(ID3D11DeviceContext* ctx);
// weapon.txt "glassdump": how this draw is drawn, into the log, and its textures into build\glass\ as they are, for reading the scope's lines and numbers.
void NoteMaterial(ID3D11DeviceContext* ctx, const char* name);
// Once a frame on the frame's own thread, where textures can be read and whole drawings run with the game's state kept: those saves, and each sight's markings drawn the first time they're wanted.
void OnFrame();

}

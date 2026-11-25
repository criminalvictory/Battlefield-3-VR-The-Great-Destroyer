#pragma once
// The sky's view rays from each eye's own camera (2026-11-25, build\designs\per_eye_effects.md section 7).
//
// The game draws the sky (with its fog) as one six-vertex quad per view, each corner carrying a view ray its pixel shader normalises and looks the sky's
// gradient, the sun, the clouds and the far backdrop up by. Corner rays normalised one by one bend the direction between them across a picture whose window is
// off its middle (the corners then differ in length): the backdrop and the sun slide against the ground, differently in each eye. Both eyes' pictures are
// centred today, where it changes nothing; it matters with weapon.txt "offcentre" (the edge-coverage work saved for the end). Here the sky's draw (its pixel
// shader known as the sky's by its constants and inputs: eyefx.h IsSkyShader) is given the mod's own vertex shader (eyefx.h kSkyRaysVs), which makes each
// corner's ray from the draw's own camera: its turn from the parts of the game's own inverse view-projection that carry no place (exact at any distance from
// the world's middle), the picture's angle and its window's move from the mod, which wrote them into that camera (views.cpp). Exact between the corners
// however the window is moved. Nothing is read back: the game's constant buffer is bound to the mod's shader as well, and the eye's three numbers are
// written into one of the mod's own (16 bytes, written only).
//
// weapon.txt "skyrays N": 0 (the default) the game's own rays; 1 the mod's while the eyes' pictures are off-centre; 2 always (a test: with centred pictures
// the sky must come out as the game's, to a level or so). Never with the depth test on for the draw (the mod's corners have a depth of their own).

#include <d3d11.h>

#include <functional>

namespace skyrays {

void SetMode(int mode);

// From the Draw hooks (effects.cpp), before anything else draws it: true when it has drawn it (draw(count, start): the game's own Draw, unhooked).
bool Draw(ID3D11DeviceContext* ctx, UINT count, const std::function<void(UINT, UINT)>& draw);

}  // namespace skyrays

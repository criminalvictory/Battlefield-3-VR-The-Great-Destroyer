#pragma once

#include <d3d11.h>

#include <cstdint>
#include <functional>

namespace overlay {

// The eyes side by side on the window, each drawn as the game draws a split screen's first view.
// BF3 draws each eye's finished picture onto the window, then that eye's own screen effects (the
// screen's blood, the damage tint), then the HUD once, for the second view. The right eye's view is
// placed at the window's left edge like the left's (views.cpp: placed at the right, its screen
// effects read the scene past its edge while suppressed and smeared the whole eye into streaks,
// 2026-10-30), so all of that comes out at the left edge; here it goes where it belongs: the right
// eye's picture and its own effects into the window's right half, the HUD into both halves.
// From the Draw and DrawIndexed hooks with the vertex or index count; true when it has drawn it.
// (start and base: a DrawIndexed's, for weapon.txt "tail" to read its vertices.)
bool Draw(ID3D11DeviceContext* ctx, UINT count, const std::function<void()>& draw, bool indexed = false,
          UINT start = 0, INT base = 0);

// From the Present hook, before NewFrame: the frame's HUD is done; the enemies' orange markers are
// taken off it (unless SetKeepOrange).
void FinishFrame(IDXGISwapChain* swap);
// weapon.txt "orangemarkers 1": the enemies' orange markers left on.
void SetKeepOrange(bool keep);
// weapon.txt "crosshair 1": the game's own crosshair (four ticks round the view's middle) left on.
void SetKeepCrosshair(bool keep);

// From the Present hook: a new frame's pictures follow.
void NewFrame();

// weapon.txt "tail": log the next frame's choices. "hudfix N": a HUD experiment (0 none).
void LogNextFrame();
void SetHudFix(int mode);
// weapon.txt "hudskip 10-19 40": the HUD's draws with those numbers in each frame (0-255, in the
// order the game draws them) left out, to tell which is which.
void SetHudSkip(const uint64_t (&bits)[4]);
// weapon.txt "hudskiptex 64x32": the HUD's draws whose first texture is that size (width << 16 |
// height, up to four; the first 0: none) left out.
void SetHudSkipTextures(const uint32_t (&sizes)[4]);

// How many eyes' pictures have gone onto the window this frame so far (0: the left eye's scene is being drawn, 1: the right eye's, 2: the HUD's time).
int EyePicturesSoFar();

// Each eye's own passes (2026-11-25, build\designs\per_eye_effects.md; eyefx.h):
// weapon.txt "dofroute 0": the depth of field's two composites routed by their viewport alone, as before (on by default: by their pixel shaders, each
// view's own over its own eye only).
void SetDofRoute(bool on);
// "fxcensus 0": no screen effects' census lines (on by default; nothing changes by them).
void SetCensus(bool on);
// "fxreaders 1": an effect on the window with an eye's viewport that reads its view's picture drawn as that view's own pass (off by default).
void SetReaders(bool on);
// "fxhold 1": an effect only the first view draws (three frames without a match) held for the right eye too (off by default).
void SetHold(bool on);
// "huddepth M": the HUD on a flat plane M metres ahead, each eye's copy moved toward the other (0 by default: at infinity, as before; the depth is the
// owner's to choose).
void SetHudDepth(float metres);
// "hudscale K": the HUD drawn K times its size about each eye's middle (1 by default; at least 0.5).
void SetHudScale(float k);

}

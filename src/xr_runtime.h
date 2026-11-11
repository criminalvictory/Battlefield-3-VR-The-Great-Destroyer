#pragma once
#include <dxgi.h>

namespace vr {
// Called from the game's render thread, just before the real Present.
// First call brings up OpenXR on the game's own D3D11 device.
void OnPresent(IDXGISwapChain* swap);
// The game is about to resize its back buffers; rebuild our swapchain next frame.
void OnResize();
// weapon.txt "halfeye 1": a test: the left eye every other frame only.
void SetHalfEye(bool on);
// weapon.txt "forceheadless 1": a test with nobody in the headset: with the stand-in on, every frame is worked headless (the eyes as last
// seen, turned by "standin"'s head pitch) even while the headset asks for frames, and nothing is handed to it. A headset left awake on a
// desk asks for them all along, so without this the films looked wherever it lay.
void SetForceHeadless(bool on);
}

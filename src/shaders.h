#pragma once
// The game's pixel shaders told apart from one start of the game to the next: their pointers change
// every time, their bytecode doesn't, so each one's bytecode checksum is kept as the game creates it
// (d3d11_hook.cpp hooks ID3D11Device::CreatePixelShader before the game makes its device).

#include <cstdint>

struct ID3D11PixelShader;
struct ID3D11VertexShader;
struct ID3D11Texture2D;

namespace shaders {

// The hook on the device's CreatePixelShader (MinHook; enabled with the others).
bool InstallHook(void* createPixelShader);
// And on its CreateVertexShader.
void InstallVertexHook(void* createVertexShader);
// And on its CreateTexture2D, for the game's sheet of scope markings: every sight's lines, numbers and chevrons, bound only while one's zoomed, so it's taken as the game makes it.
void InstallTextureHook(void* createTexture2D);
// That sheet, a reference for the caller; null till it's made.
ID3D11Texture2D* ScopeSheet();

// The checksum of a shader's bytecode, 0 if its creation wasn't seen.
uint64_t ChecksumOf(ID3D11PixelShader* ps);
uint64_t ChecksumOfVs(ID3D11VertexShader* vs);
// One of the sky's pixel shaders (eyefx.h IsSkyShader, read from its bytecode as the game made it): skyrays.h.
bool IsSky(ID3D11PixelShader* ps);

// The sun's lens flare ghost: an aperture-shaped ring near the middle of the view, one of the quads the
// game draws for the flare after the first-person pass, each with a pixel shader of its own (found
// 2026-10-31 with the sun in view, by hiding them in turn). In a headset it floats in front of the eyes
// like a crosshair (the user's "crosshair when I'm hit": it came with the sun while being shot at). The
// dirty lens's soft blobs from the same flare stay (the user likes them). Taken off unless weapon.txt
// says "lensflare 1"; "flareps <checksum> ..." adds checksums to the built-in one (as "tail" logs them).
bool IsLensFlare(ID3D11PixelShader* ps);
void SetLensFlare(bool keep, const uint64_t* extra, int extraCount);

}  // namespace shaders

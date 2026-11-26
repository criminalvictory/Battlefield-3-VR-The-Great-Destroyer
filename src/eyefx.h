#pragma once
// What each eye gets of the game's screen-wide passes (header only, as overlay.cpp, shaders.cpp and skyrays.cpp run it; tools/eyefxtest.cpp tests it
// without the game). The owner, 2026-11-25: "i like the depth of field ON, and blur.. so make sure they work in BOTH eyes.. screen effects too, and yeah i
// think its time we get the hud and markers working for each eye too, and sure a sky light ray fix is good too". The study: build\designs\per_eye_effects.md.
//
//  - The depth of field's two composites are known by their pixel shaders, not by their viewport: each view's own, drawn over its own eye only.
//  - The HUD, drawn once for both eyes, gets a depth: each eye's copy moved toward the other by half the disparity of a flat plane that far away, so it
//    no longer sits at infinity drawn over a gun 40 cm off. And, the owner's call, it can be drawn smaller about each eye's middle.
//  - The sky's pixel shaders are known by their constants and inputs, for the sky's view rays made from the view's own camera (skyrays.cpp).

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace eyefx {

// ---- The depth of field's composites (dumped and read 2026-11-25) ----
// 94a5f6c1e17c947d: the depth of field by distance (cutscenes, scripted moments): dofBlurTexture and depthTexture, alpha from the depth's distance to the
// focus. 2b2ca40a51aa1b4d: the suppression blur's clear middle: the blur texture as it is, alpha growing from the picture's middle. Each view draws its own,
// from its own blur chain on its own pictures: the first view's belongs to the left eye, the second's to the right, whatever viewport it is drawn with.
constexpr uint64_t kDofByDistance = 0x94a5f6c1e17c947dull;
constexpr uint64_t kClearMiddle = 0x2b2ca40a51aa1b4dull;
inline bool IsDofComposite(uint64_t checksum) { return checksum == kDofByDistance || checksum == kClearMiddle; }

// ---- The HUD's depth ----
// A flat plane D metres ahead, square to the eyes, is seen by the left eye that far to the right of where it is at infinity, by the right eye to the left:
// together IPD x f / D pixels, f the picture's pixels for each unit of the tangent from its middle (half its width over the tangent of half its angle). The
// same at every place of a flat picture, so the whole HUD moved by it lies on that plane. 0: no depth asked (at infinity, as the game's split screen draws it),
// or a number out of reason.
constexpr float kNearestHud = 0.5f, kFarthestHud = 100.0f;
inline float HudDisparity(float ipd, float eyePixels, float fovAcross, float depth) {
    if (!(depth > 0.0f) || !(ipd > 0.0f && ipd < 0.2f) || !(eyePixels >= 64.0f && eyePixels <= 16384.0f) || !(fovAcross > 0.2f && fovAcross < 3.0f))
        return 0.0f;
    const float d = std::clamp(depth, kNearestHud, kFarthestHud);
    const float f = 0.5f * eyePixels / std::tan(0.5f * fovAcross);
    return ipd * f / d;
}
// Each eye's share in whole pixels (the HUD's pixels kept on the pixel grid, so its text stays as sharp): the left eye's draw moved right by it, the right
// eye's copy left by it.
constexpr int kMostShift = 256;
inline int HudShift(float disparity) {
    if (!(disparity > 0.0f))
        return 0;
    return std::min(static_cast<int>(std::lround(0.5f * disparity)), kMostShift);
}
// The same, held where it was (kept: the last frame's, -1 none) while the share stays within 0.6 px of it: on the owner's picture 2 m comes to 28.999 px,
// 14.4996 a side, and an eye distance read a hundredth of a millimetre either way would turn the HUD 14 and 15 px out frame by frame.
inline int HudShiftKept(float disparity, int kept) {
    const int now = HudShift(disparity);
    if (kept < 0 || now == 0)
        return now;
    return std::fabs(0.5f * disparity - static_cast<float>(kept)) <= 0.6f ? kept : now;
}
// The HUD's size about each eye's middle (the owner's call: its corner panels sit about 51 degrees out at 106): 1 as the game lays it out; kept within
// [0.5, 1]. 0 (or less) is 1, as 0 is "as before" for every other new word ("hudscale 0" had given a half-size HUD: the review of 2e323f2).
inline float HudScale(float k) {
    if (!std::isfinite(k) || k >= 1.0f || k <= 0.0f)
        return 1.0f;
    return std::max(k, 0.5f);
}

struct Viewport {
    float x, y, w, h;
};
struct Rect {
    long left, top, right, bottom;
};

// A HUD draw's viewport as the game sets it (at the window's left edge, the HUD laid out in an eye's square: its middle at (eye / 2, eye / 2)), drawn k times
// as big about that middle and moved dx to the right.
inline Viewport Place(const Viewport& v, float eye, float k, float dx) {
    const float c = 0.5f * eye;
    return {c + k * (v.x - c) + dx, c + k * (v.y - c), k * v.w, k * v.h};
}
// A scissor rectangle of the game's, placed alike: outward to whole pixels.
inline Rect PlaceRect(const Rect& r, float eye, float k, float dx) {
    const float c = 0.5f * eye;
    return {static_cast<long>(std::floor(c + k * (static_cast<float>(r.left) - c) + dx)), static_cast<long>(std::floor(c + k * (static_cast<float>(r.top) - c))),
            static_cast<long>(std::ceil(c + k * (static_cast<float>(r.right) - c) + dx)), static_cast<long>(std::ceil(c + k * (static_cast<float>(r.bottom) - c)))};
}
// What two rectangles share; nothing (right <= left) draws nothing.
inline Rect Intersect(const Rect& a, const Rect& b) {
    Rect r{std::max(a.left, b.left), std::max(a.top, b.top), std::min(a.right, b.right), std::min(a.bottom, b.bottom)};
    if (r.right < r.left)
        r.right = r.left;
    if (r.bottom < r.top)
        r.bottom = r.top;
    return r;
}

// ---- The sky ----
// The sky's pixel shaders (111 of them in the 4251 dumped, 2026-11-25: with and without clouds, the panoramic backdrop, fog) share one set of constants:
// $Globals with g_invViewProjMatrix at 304 (its rows as each is dotted with (x, y, depth, 1) of the picture: the world's point back from the picture) and
// g_camPos at 368, and one input: SV_Position, then the view ray in TEXCOORD0's x, y and z (register 1), which the shader normalises and looks the sky's
// gradient, the sun, the clouds and the far backdrop up by. A shader is taken as the sky's from its own bytecode as the game makes it: its resource
// definitions (RDEF: a texture g_skyGradientTexture, those two at those places) and its input signature (ISGN). Every length is checked before it is read.
namespace detail {
inline bool Read32(const uint8_t* p, size_t size, size_t at, uint32_t& out) {
    if (at > size || size - at < 4)
        return false;
    std::memcpy(&out, p + at, 4);
    return true;
}
// A name at an offset in a chunk, compared without reading past it.
inline bool NameIs(const uint8_t* chunk, size_t size, uint32_t at, const char* name) {
    const size_t n = std::strlen(name);
    if (at > size || size - at < n + 1)
        return false;
    return std::memcmp(chunk + at, name, n) == 0 && chunk[at + n] == 0;
}
// The chunk with that four-letter code: its data and size.
inline bool FindChunk(const uint8_t* p, size_t size, const char* code, const uint8_t*& data, size_t& length) {
    uint32_t total = 0, count = 0;
    if (size < 32 || std::memcmp(p, "DXBC", 4) != 0 || !Read32(p, size, 24, total) || total > size || !Read32(p, size, 28, count) || count > 64)
        return false;
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t at = 0, len = 0;
        if (!Read32(p, size, 32 + 4 * i, at) || at > size || size - at < 8 || std::memcmp(p + at, code, 4) != 0 || !Read32(p, size, at + 4, len) ||
            size - at - 8 < len)
            continue;
        data = p + at + 8;
        length = len;
        return true;
    }
    return false;
}
}  // namespace detail

inline bool IsSkyShader(const void* code, size_t size) {
    using namespace detail;
    const auto* p = static_cast<const uint8_t*>(code);
    if (!p || size < 32)
        return false;
    const uint8_t* rdef = nullptr;
    size_t rdefSize = 0;
    if (!FindChunk(p, size, "RDEF", rdef, rdefSize))
        return false;
    uint32_t cbCount = 0, cbAt = 0, bindCount = 0, bindAt = 0;
    if (!Read32(rdef, rdefSize, 0, cbCount) || !Read32(rdef, rdefSize, 4, cbAt) || !Read32(rdef, rdefSize, 8, bindCount) || !Read32(rdef, rdefSize, 12, bindAt) ||
        cbCount > 64 || bindCount > 256)
        return false;
    bool gradient = false;
    for (uint32_t i = 0; i < bindCount && !gradient; ++i) {
        uint32_t name = 0, type = 0;
        if (!Read32(rdef, rdefSize, bindAt + 32 * i, name) || !Read32(rdef, rdefSize, bindAt + 32 * i + 4, type))
            return false;
        gradient = type == 2 /* D3D_SIT_TEXTURE */ && NameIs(rdef, rdefSize, name, "g_skyGradientTexture");
    }
    if (!gradient)
        return false;
    // Variable records: 40 bytes in shader model 5's definitions (RD11 after the 28-byte header), 24 before.
    const size_t varStride = rdefSize >= 32 && std::memcmp(rdef + 28, "RD11", 4) == 0 ? 40 : 24;
    bool matrix = false, camera = false;
    for (uint32_t i = 0; i < cbCount; ++i) {
        uint32_t name = 0, vars = 0, varAt = 0;
        if (!Read32(rdef, rdefSize, cbAt + 24 * i, name) || !Read32(rdef, rdefSize, cbAt + 24 * i + 4, vars) || !Read32(rdef, rdefSize, cbAt + 24 * i + 8, varAt) ||
            vars > 256)
            return false;
        if (!NameIs(rdef, rdefSize, name, "$Globals"))
            continue;
        for (uint32_t k = 0; k < vars; ++k) {
            uint32_t vn = 0, start = 0, bytes = 0;
            if (!Read32(rdef, rdefSize, varAt + varStride * k, vn) || !Read32(rdef, rdefSize, varAt + varStride * k + 4, start) ||
                !Read32(rdef, rdefSize, varAt + varStride * k + 8, bytes))
                return false;
            if (NameIs(rdef, rdefSize, vn, "g_invViewProjMatrix"))
                matrix = start == 304 && bytes == 64;
            else if (NameIs(rdef, rdefSize, vn, "g_camPos"))
                camera = start == 368 && bytes >= 12;
        }
    }
    if (!matrix || !camera)
        return false;
    const uint8_t* isgn = nullptr;
    size_t isgnSize = 0;
    uint32_t elements = 0;
    if (!FindChunk(p, size, "ISGN", isgn, isgnSize) || !Read32(isgn, isgnSize, 0, elements) || elements > 32)
        return false;
    bool position = false, ray = false;
    for (uint32_t i = 0; i < elements; ++i) {
        const size_t at = 8 + 24 * static_cast<size_t>(i);
        uint32_t name = 0, index = 0, reg = 0;
        if (!Read32(isgn, isgnSize, at, name) || !Read32(isgn, isgnSize, at + 4, index) || !Read32(isgn, isgnSize, at + 16, reg) || isgnSize - at < 21)
            return false;
        const uint8_t mask = isgn[at + 20];
        if (NameIs(isgn, isgnSize, name, "SV_Position"))
            position = reg == 0;
        else if (NameIs(isgn, isgnSize, name, "TEXCOORD") && index == 0)
            ray = reg == 1 && (mask & 7) == 7;
        else
            return false;  // anything more in: not the shape the mod's rays are made for
    }
    return position && ray;
}

// A sky draw into an eye's own picture: its one viewport the eye's whole picture at (0, 0), into a target of that size (the 11-16 tail: the eye's sky at
// 0,0 2400 x 2400 into a 2400 x 2400 target). The sky of an environment map's face, a reflection, or any picture of another size is another camera's, with
// an angle and a window of its own: never given an eye's rays (the review of 2e323f2: the game has a sky environment map, SkyEnvmap, and 12 of the 111 sky
// shaders take no depth). eyeWidth, eyeHeight: the eyes' pictures as the views were built (views.h EyeSize); 0: not known, nothing taken.
inline bool EyesOwnPicture(const Viewport& v, uint32_t targetWidth, uint32_t targetHeight, int eyeWidth, int eyeHeight) {
    if (eyeWidth <= 0 || eyeHeight <= 0)
        return false;
    return v.x == 0.0f && v.y == 0.0f && v.w == static_cast<float>(eyeWidth) && v.h == static_cast<float>(eyeHeight) &&
           targetWidth == static_cast<uint32_t>(eyeWidth) && targetHeight == static_cast<uint32_t>(eyeHeight);
}

// The mod's own vertex shader for the sky's draw (skyrays.cpp; compiled once with D3DCompile): the same six corners of the picture, and at each the view ray
// of the view's own camera. A ray taken back whole through g_invViewProjMatrix less g_camPos is only as good as the float numbers of a matrix that carries the
// camera's place in it: at 3 km from the world's middle it was off by a third of a degree (tools/eyefxtest.cpp s3, the first try). So only what has no place
// in it is taken from the matrix: its columns for the picture's x and y (the camera's right and up over the picture's half-tangents across and up, exact:
// the place only enters its other two columns), with ahead square to both (its side taken from the whole matrix, which is good enough for a side). The
// picture's own angle and its window's move come from the mod, which wrote them into the camera (views.cpp; skyrays.cpp binds them in b1). Between the corners
// the rays then come out along the very line of each pixel however far off-centre the window is moved; the game's shader normalises them as before. Nothing
// of the game's is read on the CPU: its constant buffer is bound to this shader as well.
constexpr char kSkyRaysVs[] = R"(
cbuffer Globals : register(b0) {
    float4 g_skip[19];
    float4 g_invViewProj[4];  // offset 304: the rows the game's sky shader dots (x, y, depth, 1) with
    float4 g_camPos;          // offset 368
};
cbuffer Eye : register(b1) {
    float4 g_eye;  // x: the tangent of half the picture's angle top to bottom; y, z: its window's move, shares of its width to the right and of its height up
};
void VS(uint id : SV_VertexID, out float4 pos : SV_Position, out float3 ray : TEXCOORD0) {
    // Two triangles over the whole picture, clockwise on the screen: (-1, 1) (1, 1) (-1, -1), (-1, -1) (1, 1) (1, -1).
    const uint i = id % 6;
    const float2 c = float2(i == 1 || i == 4 || i == 5 ? 1.0 : -1.0, i == 0 || i == 1 || i == 4 ? 1.0 : -1.0);
    pos = float4(c, 0.0, 1.0);
    const float4 middle = float4(0.0, 0.0, 0.999, 1.0);
    const float w = dot(g_invViewProj[3], middle);
    const float s = w < 0.0 ? -1.0 : 1.0;  // (a matrix scaled by -1 as a whole is the same camera)
    const float3 across = float3(g_invViewProj[0].x, g_invViewProj[1].x, g_invViewProj[2].x) * s;
    const float3 upward = float3(g_invViewProj[0].y, g_invViewProj[1].y, g_invViewProj[2].y) * s;
    const float up = g_eye.x, side = up * length(across) / length(upward);
    float3 ahead = normalize(cross(upward, across));
    const float3 rough = float3(dot(g_invViewProj[0], middle), dot(g_invViewProj[1], middle), dot(g_invViewProj[2], middle)) / w - g_camPos.xyz;
    if (dot(ahead, rough) < 0.0)
        ahead = -ahead;
    ray = ahead + (c.x + 2.0 * g_eye.y) * side * normalize(across) + (c.y + 2.0 * g_eye.z) * up * normalize(upward);
}
)";

// The same on the CPU, for the test: the view ray at the picture's point (x, y) (-1 to 1, y up), from the inverse view-projection's rows as the shader has
// them, the camera's place and the eye's numbers as b1 has them.
inline void SkyRay(const float invRows[4][4], const float cam[3], const float eye[4], float x, float y, float out[3]) {
    const float middle[4] = {0.0f, 0.0f, 0.999f, 1.0f};
    float w = 0.0f;
    for (int k = 0; k < 4; ++k)
        w += invRows[3][k] * middle[k];
    const float s = w < 0.0f ? -1.0f : 1.0f;
    float across[3], upward[3], rough[3];
    for (int r = 0; r < 3; ++r) {
        across[r] = invRows[r][0] * s;
        upward[r] = invRows[r][1] * s;
        float v = 0.0f;
        for (int k = 0; k < 4; ++k)
            v += invRows[r][k] * middle[k];
        rough[r] = v / w - cam[r];
    }
    const auto length = [](const float* v) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); };
    const float la = length(across), lu = length(upward), up = eye[0], side = up * la / lu;
    float ahead[3] = {upward[1] * across[2] - upward[2] * across[1], upward[2] * across[0] - upward[0] * across[2], upward[0] * across[1] - upward[1] * across[0]};
    const float lh = length(ahead);
    const float sign = ahead[0] * rough[0] + ahead[1] * rough[1] + ahead[2] * rough[2] < 0.0f ? -1.0f : 1.0f;
    for (int r = 0; r < 3; ++r)
        out[r] = sign * ahead[r] / lh + (x + 2.0f * eye[1]) * side * across[r] / la + (y + 2.0f * eye[2]) * up * upward[r] / lu;
}

}  // namespace eyefx

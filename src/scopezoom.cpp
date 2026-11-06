#include "scopezoom.h"

#include <windows.h>
#include <d3dcompiler.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>

#include "log.h"
#include "scopeview.h"
#include "shaders.h"

namespace {

const char kShader[] = R"(
cbuffer Lens : register(b0) {
    float4x4 vp;          // the arms' space to clip
    float4x4 inverseVp;
    float4 scopeX, scopeY, scopeZ;  // its axes in the arms' space (xyz)
    float4 eye;           // this eye, in the arms' space; w: the power
    float4 middle;        // the eyepiece's middle in the arms' space; w: its radius
    float4 view;          // the viewport: x, y, width, height
    float4 size;          // the copy's width, height; z 1: dark glass
    float4 marksAt;       // x: the sheet of this sight's markings per unit of slope; y: 1 with any
    float4 sharpen;       // x: how much its edges are steepened
    float4 secondX, secondY, secondW;  // with its own picture: a way in the arms' space into that picture (scopeview.h)
};
Texture2D picture : register(t0);
Texture2D marks : register(t1);
SamplerState linearClamp : register(s0);
SamplerState marksSampler : register(s1);

// The picture is in light as it is, the sun many times brighter than anything: filters that overshoot are worked in light squeezed into 0-1, or the sun leaves a dark ring round it.
float3 Squeeze(float3 c) {
    c = max(c, 0.0);
    return c / (1.0 + max(max(c.r, c.g), c.b));
}
float3 Unsqueeze(float3 c) {
    return c / max(1.0 - max(max(c.r, c.g), c.b), 0.001);
}
float3 Read(float2 uv) {
    return Squeeze(picture.SampleLevel(linearClamp, uv, 0).rgb);
}

// The picture a good way nearer: each of its pixels is several of the lens's. A smooth cubic through them looks blurred (the user, 2026-11-05: "way too blurry"), a sharp one shows them as blocks. So an edge-following filter, AMD's FSR 1
// EASU (MIT licence: Copyright (c) 2021 Advanced Micro Devices, Inc.; this is its float form, in squeezed light): across an edge its kernel's narrow, along it long, so a staircase of pixels comes out a clean line. Twelve texels, by Gather.
void TapF(inout float3 aC, inout float aW, float2 off, float2 dir, float2 len, float lob, float clp, float3 c) {
    float2 v = float2(off.x * dir.x + off.y * dir.y, off.x * -dir.y + off.y * dir.x) * len;
    const float d2 = min(dot(v, v), clp);
    float wB = 2.0 / 5.0 * d2 - 1.0;
    float wA = lob * d2 - 1.0;
    wB *= wB;
    wA *= wA;
    wB = 25.0 / 16.0 * wB - (25.0 / 16.0 - 1.0);
    const float w = wB * wA;
    aC += c * w;
    aW += w;
}
void SetF(inout float2 dir, inout float len, float w, float lA, float lB, float lC, float lD, float lE) {
    const float dc = lD - lC, cb = lC - lB;
    float lenX = 1.0 / max(max(abs(dc), abs(cb)), 1e-5);
    const float dirX = lD - lB;
    dir.x += dirX * w;
    lenX = saturate(abs(dirX) * lenX);
    len += lenX * lenX * w;
    const float ec = lE - lC, ca = lC - lA;
    float lenY = 1.0 / max(max(abs(ec), abs(ca)), 1e-5);
    const float dirY = lE - lA;
    dir.y += dirY * w;
    lenY = saturate(abs(dirY) * lenY);
    len += lenY * lenY * w;
}
float3 Texel(float r, float g, float b) {
    return Squeeze(float3(r, g, b));
}
float3 Nearer(float2 uv) {
    const float2 texel = 1.0 / size.xy;
    float2 pp = uv * size.xy - 0.5;
    const float2 fp = floor(pp);
    pp -= fp;
    //    b c
    //  e f g h
    //  i j k l
    //    n o
    const float2 p0 = (fp + float2(1.0, -1.0)) * texel, p1 = (fp + float2(0.0, 1.0)) * texel, p2 = (fp + float2(2.0, 1.0)) * texel, p3 = (fp + float2(1.0, 3.0)) * texel;
    const float4 r0 = picture.GatherRed(linearClamp, p0), g0 = picture.GatherGreen(linearClamp, p0), b0 = picture.GatherBlue(linearClamp, p0);
    const float4 r1 = picture.GatherRed(linearClamp, p1), g1 = picture.GatherGreen(linearClamp, p1), b1 = picture.GatherBlue(linearClamp, p1);
    const float4 r2 = picture.GatherRed(linearClamp, p2), g2 = picture.GatherGreen(linearClamp, p2), b2 = picture.GatherBlue(linearClamp, p2);
    const float4 r3 = picture.GatherRed(linearClamp, p3), g3 = picture.GatherGreen(linearClamp, p3), b3 = picture.GatherBlue(linearClamp, p3);
    const float3 b_ = Texel(r0.x, g0.x, b0.x), c_ = Texel(r0.y, g0.y, b0.y);
    const float3 i_ = Texel(r1.x, g1.x, b1.x), j_ = Texel(r1.y, g1.y, b1.y), f_ = Texel(r1.z, g1.z, b1.z), e_ = Texel(r1.w, g1.w, b1.w);
    const float3 k_ = Texel(r2.x, g2.x, b2.x), l_ = Texel(r2.y, g2.y, b2.y), h_ = Texel(r2.z, g2.z, b2.z), g_ = Texel(r2.w, g2.w, b2.w);
    const float3 o_ = Texel(r3.z, g3.z, b3.z), n_ = Texel(r3.w, g3.w, b3.w);
    const float3 toLuma = float3(0.5, 1.0, 0.5);
    const float bL = dot(b_, toLuma), cL = dot(c_, toLuma), iL = dot(i_, toLuma), jL = dot(j_, toLuma), fL = dot(f_, toLuma), eL = dot(e_, toLuma), kL = dot(k_, toLuma), lL = dot(l_, toLuma), hL = dot(h_, toLuma), gL = dot(g_, toLuma), oL = dot(o_, toLuma), nL = dot(n_, toLuma);
    float2 dir = float2(0.0, 0.0);
    float len = 0.0;
    SetF(dir, len, (1.0 - pp.x) * (1.0 - pp.y), bL, eL, fL, gL, jL);
    SetF(dir, len, pp.x * (1.0 - pp.y), cL, fL, gL, hL, kL);
    SetF(dir, len, (1.0 - pp.x) * pp.y, fL, iL, jL, kL, nL);
    SetF(dir, len, pp.x * pp.y, gL, jL, kL, lL, oL);
    const float dirR = dot(dir, dir);
    if (dirR < 1.0 / 32768.0)
        dir = float2(1.0, 0.0);
    else
        dir /= sqrt(dirR);
    len = len * 0.5;
    len *= len;
    const float stretch = dot(dir, dir) / max(abs(dir.x), abs(dir.y));
    const float2 len2 = float2(1.0 + (stretch - 1.0) * len, 1.0 - 0.5 * len);
    const float lob = 0.5 + ((1.0 / 4.0 - 0.04) - 0.5) * len;
    const float clp = 1.0 / lob;
    const float3 min4 = min(min(f_, g_), min(j_, k_)), max4 = max(max(f_, g_), max(j_, k_));
    float3 aC = float3(0.0, 0.0, 0.0);
    float aW = 0.0;
    TapF(aC, aW, float2(0.0, -1.0) - pp, dir, len2, lob, clp, b_);
    TapF(aC, aW, float2(1.0, -1.0) - pp, dir, len2, lob, clp, c_);
    TapF(aC, aW, float2(-1.0, 1.0) - pp, dir, len2, lob, clp, i_);
    TapF(aC, aW, float2(0.0, 1.0) - pp, dir, len2, lob, clp, j_);
    TapF(aC, aW, float2(0.0, 0.0) - pp, dir, len2, lob, clp, f_);
    TapF(aC, aW, float2(-1.0, 0.0) - pp, dir, len2, lob, clp, e_);
    TapF(aC, aW, float2(1.0, 1.0) - pp, dir, len2, lob, clp, k_);
    TapF(aC, aW, float2(2.0, 1.0) - pp, dir, len2, lob, clp, l_);
    TapF(aC, aW, float2(2.0, 0.0) - pp, dir, len2, lob, clp, h_);
    TapF(aC, aW, float2(1.0, 0.0) - pp, dir, len2, lob, clp, g_);
    TapF(aC, aW, float2(1.0, 2.0) - pp, dir, len2, lob, clp, o_);
    TapF(aC, aW, float2(0.0, 2.0) - pp, dir, len2, lob, clp, n_);
    float3 color = clamp(aC / max(aW, 1e-5), min4, max4);
    // Then steepened a little, by how much it stands out from a picture's pixel round it; less near black or white. No further than what's really next to it, or it rings.
    if (sharpen.x > 0.0) {
        const float3 n0 = Read(uv + float2(0.0, -texel.y)), n1 = Read(uv + float2(-texel.x, 0.0)), n2 = Read(uv + float2(texel.x, 0.0)), n3 = Read(uv + float2(0.0, texel.y));
        const float3 lowest = min(min(min(n0, n1), min(n2, n3)), color), highest = max(max(max(n0, n1), max(n2, n3)), color);
        const float3 around = (n0 + n1 + n2 + n3) * 0.25;
        const float3 room = saturate(min(lowest, 1.0 - highest) / max(highest, 0.001));
        color = clamp(color + (color - around) * sqrt(room) * sharpen.x, lowest, highest);
    }
    return Unsqueeze(saturate(color));
}

float4 PS(float4 pos : SV_Position) : SV_Target {
    const float2 ndc = float2((pos.x - view.x) / view.z * 2.0 - 1.0, 1.0 - (pos.y - view.y) / view.w * 2.0);
    float4 far4 = mul(inverseVp, float4(ndc, 0.5, 1.0));
    const float3 ray = normalize(far4.xyz / far4.w - eye.xyz);
    // The disc's own mesh gives its shape. From the wrong side nothing is seen through it.
    const float along = dot(ray, scopeZ.xyz);
    if (along < 0.2)
        discard;
    const float3 toPlane = ray * dot(middle.xyz - eye.xyz, scopeZ.xyz) / along + eye.xyz - middle.xyz;
    const float off = length(toPlane);
    // The same way in, with its angle from the scope's line divided by the power: what's seen there.
    const float2 slope = float2(dot(ray, scopeX.xyz), dot(ray, scopeY.xyz)) / along / eye.w;
    const float3 way = scopeZ.xyz + scopeX.xyz * slope.x + scopeY.xyz * slope.y;
#ifdef OWN_PICTURE
    // A picture really drawn along the sight's line, for this frame: what's looked at where it is. The edge of it dark, past what it was drawn for.
    const float3 c2 = float3(dot(secondX.xyz, way), dot(secondY.xyz, way), dot(secondW.xyz, way));
    const float2 uv = c2.z > 0.0 ? float2(c2.x / c2.z * 0.5 + 0.5, 0.5 - c2.y / c2.z * 0.5) : float2(-1.0, -1.0);
    float3 color = Nearer(uv) * size.w;  // (its light against the eyes')
    color *= 1.0 - smoothstep(0.96, 1.0, max(abs(uv.x - 0.5), abs(uv.y - 0.5)) * 2.0);
#else
    const float4 clip = mul(vp, float4(eye.xyz + way * 1000.0, 1.0));
    const float2 at = float2(clip.x / clip.w * 0.5 + 0.5, 0.5 - clip.y / clip.w * 0.5);
    const float2 uv = (view.xy + at * view.zw) / size.xy;
    float3 color = size.z > 0.5 ? float3(0.0, 0.0, 0.0) : Nearer(uv);
#endif
    // Its lines, numbers and aim points, by the same slope: they stay on what's under them as the eye moves.
    if (marksAt.y > 0.5) {
        const float2 m = float2(0.5 + slope.x * marksAt.x, 0.5 - slope.y * marksAt.x);
        const float4 mark = marks.Sample(marksSampler, m);
        if (m.x >= 0.0 && m.x <= 1.0 && m.y >= 0.0 && m.y <= 1.0)
            color = color * (1.0 - mark.a) + mark.rgb * 5.0;  // bright ones as the game's: five times their colour
    }
    // A scope only shows so wide a view: with the eye well off its line, or looking in from far to one side, what's beyond that is black, as through a real one. Not a reflex sight, which has no such thing.
    if (eye.w > 1.2)
        color *= 1.0 - smoothstep(0.30, 0.38, length(slope) * eye.w);
    // The glass's edge a little dark, as the view's edge through a scope is.
    return float4(color * lerp(1.0, eye.w > 1.2 ? 0.25 : 1.0, smoothstep(0.82, 1.0, off / middle.w)), 1.0);
}

// A sight's markings onto a sheet of their own, once: from the game's sheet, each piece where its slope is.
cbuffer Map : register(b0) {
    float4 mapScale;  // x: to the sheet's -1..1
    float4 mapColor;
};
Texture2D gameSheet : register(t0);
SamplerState sheetSampler : register(s0);

void MarkVS(float2 at : POSITION, float2 st : TEXCOORD0, out float4 pos : SV_Position, out float2 uv : TEXCOORD0) {
    pos = float4(at * mapScale.x, 0.0, 1.0);
    uv = st;
}
float4 DarkPS(float4 pos : SV_Position, float2 uv : TEXCOORD0) : SV_Target {
    const float t = gameSheet.Sample(sheetSampler, uv).r;
    return float4(0.0, 0.0, 0.0, t);
}
float4 BrightPS(float4 pos : SV_Position, float2 uv : TEXCOORD0) : SV_Target {
    const float t = gameSheet.Sample(sheetSampler, uv).r;
    return float4(mapColor.rgb * t, t);
}
)";

constexpr float kLeastPixels = 4.0f;  // a lens this small in the picture isn't worth a copy
constexpr UINT kMapSize = 1024;       // a sight's markings: its reach of slope across half of this

struct Constants {
    float vp[16];
    float inverseVp[16];
    float x[4], y[4], z[4];
    float eye[4];
    float middle[4];
    float view[4];
    float size[4];
    float marksAt[4];
    float sharpen[4];
    float second[12];
};

std::atomic<float> g_sharpen{0.35f};
std::atomic<float> g_ownGain{1.0f};  // weapon.txt "scopeviewgain": the picture of its own's light scaled

struct MapConstants {
    float scale[4];
    float color[4];
};

struct Resources {
    bool tried = false, ok = false;
    ID3D11PixelShader* ps = nullptr;
    ID3D11PixelShader* ownPs = nullptr;  // reading a picture of its own (scopeview.h)
    ID3D11Buffer* constants = nullptr;
    ID3D11SamplerState* sampler = nullptr;
    ID3D11SamplerState* marksSampler = nullptr;
    ID3D11BlendState* blend = nullptr;
    ID3D11Texture2D* copy = nullptr;
    ID3D11ShaderResourceView* copyView = nullptr;
    UINT width = 0, height = 0;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
} g;

template <class T>
void SafeRelease(T*& p) {
    if (p)
        p->Release();
    p = nullptr;
}

ID3DBlob* Compile(const char* entry, const char* profile, const D3D_SHADER_MACRO* macros = nullptr) {
    ID3DBlob* code = nullptr;
    ID3DBlob* errors = nullptr;
    if (FAILED(D3DCompile(kShader, sizeof(kShader) - 1, "scopezoom", macros, nullptr, entry, profile, D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &code, &errors))) {
        Log("Scope: shader %s failed: %s", entry, errors ? static_cast<const char*>(errors->GetBufferPointer()) : "no message");
        SafeRelease(code);
    }
    SafeRelease(errors);
    return code;
}

bool CreateFixed(ID3D11Device* device) {
    if (g.tried)
        return g.ok;
    g.tried = true;
    ID3DBlob* ps = Compile("PS", "ps_5_0");
    bool ok = ps && SUCCEEDED(device->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &g.ps));
    SafeRelease(ps);
    const D3D_SHADER_MACRO own[] = {{"OWN_PICTURE", "1"}, {nullptr, nullptr}};
    ID3DBlob* ownPs = Compile("PS", "ps_5_0", own);
    ok = ok && ownPs && SUCCEEDED(device->CreatePixelShader(ownPs->GetBufferPointer(), ownPs->GetBufferSize(), nullptr, &g.ownPs));
    SafeRelease(ownPs);
    D3D11_BUFFER_DESC cb{sizeof(Constants), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0};
    D3D11_SAMPLER_DESC sd{};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    D3D11_SAMPLER_DESC md = sd;
    md.Filter = D3D11_FILTER_ANISOTROPIC;
    md.MaxAnisotropy = 8;
    D3D11_BLEND_DESC bd{};
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    ok = ok && SUCCEEDED(device->CreateBuffer(&cb, nullptr, &g.constants)) && SUCCEEDED(device->CreateSamplerState(&sd, &g.sampler)) && SUCCEEDED(device->CreateSamplerState(&md, &g.marksSampler)) &&
         SUCCEEDED(device->CreateBlendState(&bd, &g.blend));
    g.ok = ok;
    Log("Scope: %s", ok ? "its zoom's shaders are ready" : "its zoom's shaders could not be made: the scopes stay as the game draws them");
    return ok;
}

bool PrepareCopy(ID3D11Device* device, const D3D11_TEXTURE2D_DESC& target, DXGI_FORMAT format) {
    if (g.copy && g.width == target.Width && g.height == target.Height && g.format == format)
        return true;
    SafeRelease(g.copyView);
    SafeRelease(g.copy);
    D3D11_TEXTURE2D_DESC d{};
    d.Width = target.Width;
    d.Height = target.Height;
    d.MipLevels = 1;
    d.ArraySize = 1;
    d.Format = format;
    d.SampleDesc.Count = 1;
    d.Usage = D3D11_USAGE_DEFAULT;
    d.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(device->CreateTexture2D(&d, nullptr, &g.copy)) || FAILED(device->CreateShaderResourceView(g.copy, nullptr, &g.copyView))) {
        SafeRelease(g.copyView);
        SafeRelease(g.copy);
        return false;
    }
    g.width = target.Width, g.height = target.Height, g.format = format;
    Log("Scope: reads a %ux%u copy of the picture, format %d, %u samples in the target", target.Width, target.Height, static_cast<int>(format), target.SampleDesc.Count);
    return true;
}

// A 4x4's inverse, rows; false if it hasn't one.
bool Invert4(const float* m, float* out) {
    float inv[16];
    inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
    inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
    inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
    inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
    inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
    inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
    inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
    inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
    inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
    inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
    inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
    inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];
    const float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
    if (std::fabs(det) < 1e-12f)
        return false;
    for (int i = 0; i < 16; ++i)
        out[i] = inv[i] / det;
    return true;
}

// The pixel side's state, between Begin and End.
struct Kept {
    ID3D11PixelShader* ps = nullptr;
    ID3D11ShaderResourceView* srv[2] = {};
    ID3D11SamplerState* sampler[2] = {};
    ID3D11Buffer* cb = nullptr;
    ID3D11BlendState* blend = nullptr;
    FLOAT factor[4] = {};
    UINT mask = 0;
};
thread_local Kept t_kept;

// weapon.txt "glassdump": textures waiting for the frame's own thread to read them.
struct Pending {
    ID3D11Texture2D* texture = nullptr;
    char name[64] = {};
};
std::mutex g_pendingMutex;
std::vector<Pending> g_pending;

// Each sight's markings on a sheet of their own.
struct Map {
    const scopemarks::Marks* marks = nullptr;
    ID3D11ShaderResourceView* view = nullptr;  // null till it's drawn
    bool failed = false;
};
std::vector<Map> g_maps;  // under g_pendingMutex

ID3D11ShaderResourceView* MapFor(const scopemarks::Marks* marks) {
    std::lock_guard lock(g_pendingMutex);
    for (const Map& m : g_maps)
        if (m.marks == marks)
            return m.view;
    Map m;
    m.marks = marks;
    g_maps.push_back(m);
    return nullptr;
}

// One sight's markings drawn, on a context of its own, with the game's state as it was afterwards.
ID3D11ShaderResourceView* DrawMap(ID3D11Device* device, ID3D11DeviceContext* immediate, ID3D11Texture2D* sheet, const scopemarks::Marks& marks) {
    ID3DBlob* vsCode = Compile("MarkVS", "vs_5_0");
    ID3DBlob* darkCode = Compile("DarkPS", "ps_5_0");
    ID3DBlob* brightCode = Compile("BrightPS", "ps_5_0");
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* darkPs = nullptr;
    ID3D11PixelShader* brightPs = nullptr;
    ID3D11InputLayout* layout = nullptr;
    ID3D11Buffer* vertices = nullptr;
    ID3D11Buffer* constants = nullptr;
    ID3D11BlendState* blend = nullptr;
    ID3D11SamplerState* sampler = nullptr;
    ID3D11RasterizerState* raster = nullptr;
    ID3D11DepthStencilState* depth = nullptr;
    ID3D11Texture2D* map = nullptr;
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11ShaderResourceView* view = nullptr;
    ID3D11ShaderResourceView* sheetView = nullptr;
    ID3D11DeviceContext* deferred = nullptr;
    ID3D11CommandList* list = nullptr;
    bool ok = vsCode && darkCode && brightCode && SUCCEEDED(device->CreateVertexShader(vsCode->GetBufferPointer(), vsCode->GetBufferSize(), nullptr, &vs)) &&
              SUCCEEDED(device->CreatePixelShader(darkCode->GetBufferPointer(), darkCode->GetBufferSize(), nullptr, &darkPs)) &&
              SUCCEEDED(device->CreatePixelShader(brightCode->GetBufferPointer(), brightCode->GetBufferSize(), nullptr, &brightPs));
    if (ok) {
        const D3D11_INPUT_ELEMENT_DESC elements[] = {{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}, {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0}};
        ok = SUCCEEDED(device->CreateInputLayout(elements, 2, vsCode->GetBufferPointer(), vsCode->GetBufferSize(), &layout));
    }
    std::vector<scopemarks::Vertex> all;
    if (marks.dark)
        all.insert(all.end(), marks.dark, marks.dark + marks.darkCount);
    if (marks.bright)
        all.insert(all.end(), marks.bright, marks.bright + marks.brightCount);
    if (ok && !all.empty()) {
        D3D11_BUFFER_DESC bd{static_cast<UINT>(all.size() * sizeof(all[0])), D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0};
        D3D11_SUBRESOURCE_DATA init{all.data(), 0, 0};
        ok = SUCCEEDED(device->CreateBuffer(&bd, &init, &vertices));
    }
    const float reach = marks.reach > 1e-5f ? marks.reach * 1.02f : 1.0f;
    const MapConstants mc{{1.0f / reach, 0.0f, 0.0f, 0.0f}, {marks.color[0], marks.color[1], marks.color[2], 1.0f}};
    if (ok) {
        D3D11_BUFFER_DESC bd{sizeof(MapConstants), D3D11_USAGE_IMMUTABLE, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0};
        D3D11_SUBRESOURCE_DATA init{&mc, 0, 0};
        ok = SUCCEEDED(device->CreateBuffer(&bd, &init, &constants));
    }
    if (ok) {
        // Each over what's already there, as the game's own draws them.
        D3D11_BLEND_DESC bd{};
        D3D11_RENDER_TARGET_BLEND_DESC& rt = bd.RenderTarget[0];
        rt.BlendEnable = TRUE;
        rt.SrcBlend = rt.SrcBlendAlpha = D3D11_BLEND_ONE;
        rt.DestBlend = rt.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        rt.BlendOp = rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        D3D11_SAMPLER_DESC sd{};
        sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.MaxLOD = D3D11_FLOAT32_MAX;
        D3D11_RASTERIZER_DESC rd{};
        rd.FillMode = D3D11_FILL_SOLID;
        rd.CullMode = D3D11_CULL_NONE;
        D3D11_DEPTH_STENCIL_DESC dd{};
        D3D11_TEXTURE2D_DESC td{};
        td.Width = td.Height = kMapSize;
        td.MipLevels = 0;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        td.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
        D3D11_TEXTURE2D_DESC sheetDesc{};
        sheet->GetDesc(&sheetDesc);
        D3D11_SHADER_RESOURCE_VIEW_DESC sheetViewDesc{};
        sheetViewDesc.Format = sheetDesc.Format == DXGI_FORMAT_BC1_TYPELESS ? DXGI_FORMAT_BC1_UNORM : sheetDesc.Format;
        sheetViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        sheetViewDesc.Texture2D.MipLevels = 1;
        ok = SUCCEEDED(device->CreateBlendState(&bd, &blend)) && SUCCEEDED(device->CreateSamplerState(&sd, &sampler)) && SUCCEEDED(device->CreateRasterizerState(&rd, &raster)) &&
             SUCCEEDED(device->CreateDepthStencilState(&dd, &depth)) && SUCCEEDED(device->CreateTexture2D(&td, nullptr, &map)) && SUCCEEDED(device->CreateRenderTargetView(map, nullptr, &rtv)) &&
             SUCCEEDED(device->CreateShaderResourceView(map, nullptr, &view)) && SUCCEEDED(device->CreateShaderResourceView(sheet, &sheetViewDesc, &sheetView)) &&
             SUCCEEDED(device->CreateDeferredContext(0, &deferred));
    }
    if (ok) {
        const FLOAT clear[4] = {};
        deferred->ClearRenderTargetView(rtv, clear);
        deferred->OMSetRenderTargets(1, &rtv, nullptr);
        const D3D11_VIEWPORT viewport{0.0f, 0.0f, static_cast<float>(kMapSize), static_cast<float>(kMapSize), 0.0f, 1.0f};
        deferred->RSSetViewports(1, &viewport);
        deferred->RSSetState(raster);
        deferred->OMSetDepthStencilState(depth, 0);
        const FLOAT none[4] = {};
        deferred->OMSetBlendState(blend, none, 0xFFFFFFFF);
        deferred->IASetInputLayout(layout);
        deferred->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        const UINT stride = sizeof(scopemarks::Vertex), offset = 0;
        deferred->IASetVertexBuffers(0, 1, &vertices, &stride, &offset);
        deferred->VSSetShader(vs, nullptr, 0);
        deferred->VSSetConstantBuffers(0, 1, &constants);
        deferred->PSSetConstantBuffers(0, 1, &constants);
        deferred->PSSetShaderResources(0, 1, &sheetView);
        deferred->PSSetSamplers(0, 1, &sampler);
        if (marks.darkCount > 0) {
            deferred->PSSetShader(darkPs, nullptr, 0);
            deferred->Draw(marks.darkCount, 0);
        }
        if (marks.brightCount > 0) {
            deferred->PSSetShader(brightPs, nullptr, 0);
            deferred->Draw(marks.brightCount, marks.darkCount);
        }
        ID3D11RenderTargetView* noTarget = nullptr;
        deferred->OMSetRenderTargets(1, &noTarget, nullptr);
        deferred->GenerateMips(view);
        ok = SUCCEEDED(deferred->FinishCommandList(FALSE, &list));
    }
    if (ok)
        immediate->ExecuteCommandList(list, TRUE);  // TRUE: the game's state as it was
    SafeRelease(list);
    SafeRelease(deferred);
    SafeRelease(sheetView);
    SafeRelease(rtv);
    SafeRelease(map);
    SafeRelease(depth);
    SafeRelease(raster);
    SafeRelease(sampler);
    SafeRelease(blend);
    SafeRelease(constants);
    SafeRelease(vertices);
    SafeRelease(layout);
    SafeRelease(brightPs);
    SafeRelease(darkPs);
    SafeRelease(vs);
    SafeRelease(brightCode);
    SafeRelease(darkCode);
    SafeRelease(vsCode);
    if (!ok)
        SafeRelease(view);
    return view;
}

}

namespace scopezoom {

void SetSharpen(float amount) {
    g_sharpen = amount;
}

void SetOwnGain(float gain) {
    g_ownGain = gain;
}

bool InScene(ID3D11DeviceContext* c) {
    ID3D11RenderTargetView* rtv = nullptr;
    c->OMGetRenderTargets(1, &rtv, nullptr);
    if (!rtv)
        return false;
    D3D11_RENDER_TARGET_VIEW_DESC desc{};
    rtv->GetDesc(&desc);
    rtv->Release();
    return desc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT;
}

bool Begin(ID3D11DeviceContext* c, const Lens& lens, bool* marked) {
    if (marked)
        *marked = false;
    ID3D11Device* device = nullptr;
    c->GetDevice(&device);
    if (!device)
        return false;
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11DepthStencilView* dsv = nullptr;
    c->OMGetRenderTargets(1, &rtv, &dsv);
    ID3D11Resource* resource = nullptr;
    ID3D11Texture2D* target = nullptr;
    bool begun = false;
    if (rtv) {
        rtv->GetResource(&resource);
        if (resource)
            resource->QueryInterface(IID_PPV_ARGS(&target));
    }
    D3D11_VIEWPORT viewport{};
    UINT viewportCount = 1;
    c->RSGetViewports(&viewportCount, &viewport);
    if (target && viewportCount == 1 && CreateFixed(device)) {
        D3D11_TEXTURE2D_DESC desc{};
        target->GetDesc(&desc);
        D3D11_RENDER_TARGET_VIEW_DESC viewDesc{};
        rtv->GetDesc(&viewDesc);
        // Only what's needed, for the frame rate: the picture round where the scope's line goes, and the lens's own pixels. Nothing at all once it's too small to see into.
        float lensAt[3], edgeAt[3], distant[3], atLens[2], atEdge[2], atFar[2];
        for (int i = 0; i < 3; ++i) {
            lensAt[i] = lens.scope[i * 4] * lens.middle[0] + lens.scope[i * 4 + 1] * lens.middle[1] + lens.scope[i * 4 + 2] * lens.middle[2] + lens.scope[i * 4 + 3];
            edgeAt[i] = lensAt[i] + lens.radius * lens.scope[i * 4 + 1];
            distant[i] = lens.eye[i] + 1000.0f * lens.scope[i * 4 + 2];
        }
        auto toPixels = [&](const float* p, float* out) {
            const float* v = lens.vp;
            const float x = v[0] * p[0] + v[1] * p[1] + v[2] * p[2] + v[3], y = v[4] * p[0] + v[5] * p[1] + v[6] * p[2] + v[7], w = v[12] * p[0] + v[13] * p[1] + v[14] * p[2] + v[15];
            if (w < 0.01f)
                return false;
            out[0] = viewport.TopLeftX + (x / w * 0.5f + 0.5f) * viewport.Width;
            out[1] = viewport.TopLeftY + (0.5f - y / w * 0.5f) * viewport.Height;
            return true;
        };
        const bool lensSeen = toPixels(lensAt, atLens) && toPixels(edgeAt, atEdge);
        const float lensPixels = lensSeen ? std::hypot(atEdge[0] - atLens[0], atEdge[1] - atLens[1]) : 0.0f;
        const bool farSeen = toPixels(distant, atFar);
        // The sight being looked through is what the picture of its own is drawn along.
        if (lensPixels >= kLeastPixels && farSeen && lens.haveView) {
            const float focal = 0.5f * viewport.Height * std::sqrt(lens.vp[4] * lens.vp[4] + lens.vp[5] * lens.vp[5] + lens.vp[6] * lens.vp[6]);
            scopeview::NoteLens(lens.eyeIndex, lens.viewToArms, lens.scope, lensPixels, std::hypot(atFar[0] - atLens[0], atFar[1] - atLens[1]), lens.power, focal);
        }
        float secondRows[12];
        ID3D11ShaderResourceView* const ownPicture = lens.haveView && lens.power > 1.5f && scopeview::PictureFrom(lens.eyeIndex, lens.viewToArms, secondRows) ? scopeview::Picture() : nullptr;
        if (lensPixels >= kLeastPixels && (ownPicture || PrepareCopy(device, desc, viewDesc.Format))) {
            D3D11_BOX box{0, 0, 0, 0, 0, 1};
            if (farSeen && !ownPicture) {
                const float half = (std::hypot(atFar[0] - atLens[0], atFar[1] - atLens[1]) + lensPixels) * 1.3f / lens.power + 8.0f;
                box.left = static_cast<UINT>(std::fmax(atFar[0] - half, 0.0f));
                box.top = static_cast<UINT>(std::fmax(atFar[1] - half, 0.0f));
                box.right = static_cast<UINT>(std::fmin(std::fmax(atFar[0] + half, 0.0f), static_cast<float>(desc.Width)));
                box.bottom = static_cast<UINT>(std::fmin(std::fmax(atFar[1] + half, 0.0f), static_cast<float>(desc.Height)));
            }
            const bool anything = box.right > box.left && box.bottom > box.top;
            if (ownPicture)
                ;
            else if (desc.SampleDesc.Count > 1)
                c->ResolveSubresource(g.copy, 0, target, 0, viewDesc.Format);
            else if (anything)
                c->CopySubresourceRegion(g.copy, 0, box.left, box.top, 0, target, 0, &box);
            Constants k{};
            k.sharpen[0] = g_sharpen.load(std::memory_order_relaxed);
            k.size[2] = anything || ownPicture ? 0.0f : 1.0f;  // its line's nowhere in the picture: dark glass
            if (ownPicture)
                std::memcpy(k.second, secondRows, sizeof(k.second));
            k.size[3] = g_ownGain.load(std::memory_order_relaxed);
            ID3D11ShaderResourceView* marks = lens.marks ? MapFor(lens.marks) : nullptr;
            if (marked)
                *marked = marks != nullptr;
            if (marks) {
                k.marksAt[0] = 0.5f / (lens.marks->reach * 1.02f);
                k.marksAt[1] = 1.0f;
            }
            // HLSL's default packing reads a float4x4 by columns: its rows are given as columns here (transposed).
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                    k.vp[i * 4 + j] = lens.vp[j * 4 + i];
            float inverse[16];
            if (Invert4(lens.vp, inverse)) {
                for (int i = 0; i < 4; ++i)
                    for (int j = 0; j < 4; ++j)
                        k.inverseVp[i * 4 + j] = inverse[j * 4 + i];
                // The eye exactly as this draw's view has it, its centre of projection, where everything it draws converges: the third column of its inverse. (The eye the hands know is a little off that,
                // and a few millimetres at the lens is a good part of what a pixel of it looks at: a reflex sight's view through it didn't line up with the world round it.)
                const float homogeneous = inverse[14];
                for (int i = 0; i < 3; ++i) {
                    k.x[i] = lens.scope[i * 4];
                    k.y[i] = lens.scope[i * 4 + 1];
                    k.z[i] = lens.scope[i * 4 + 2];
                    k.eye[i] = std::fabs(homogeneous) > 1e-9f ? inverse[i * 4 + 2] / homogeneous : lens.eye[i];
                    k.middle[i] = lens.scope[i * 4] * lens.middle[0] + lens.scope[i * 4 + 1] * lens.middle[1] + lens.scope[i * 4 + 2] * lens.middle[2] + lens.scope[i * 4 + 3];
                }
                k.eye[3] = lens.power;
                k.middle[3] = lens.radius;
                k.view[0] = viewport.TopLeftX, k.view[1] = viewport.TopLeftY, k.view[2] = viewport.Width, k.view[3] = viewport.Height;
                k.size[0] = static_cast<float>(ownPicture ? scopeview::PictureSize() : desc.Width), k.size[1] = static_cast<float>(ownPicture ? scopeview::PictureSize() : desc.Height);
                c->UpdateSubresource(g.constants, 0, nullptr, &k, 0, 0);
                // The game's state, kept till End: only what the pixel side needs. The disc's own vertex shader, input and depth test stay as they are.
                Kept& kept = t_kept;
                c->PSGetShader(&kept.ps, nullptr, nullptr);
                c->PSGetShaderResources(0, 2, kept.srv);
                c->PSGetSamplers(0, 2, kept.sampler);
                c->PSGetConstantBuffers(0, 1, &kept.cb);
                c->OMGetBlendState(&kept.blend, kept.factor, &kept.mask);

                c->PSSetShader(ownPicture ? g.ownPs : g.ps, nullptr, 0);
                ID3D11ShaderResourceView* views[2] = {ownPicture ? ownPicture : g.copyView, marks};
                c->PSSetShaderResources(0, 2, views);
                ID3D11SamplerState* samplers[2] = {g.sampler, g.marksSampler};
                c->PSSetSamplers(0, 2, samplers);
                c->PSSetConstantBuffers(0, 1, &g.constants);
                const FLOAT none[4] = {};
                c->OMSetBlendState(g.blend, none, 0xFFFFFFFF);
                begun = true;
            }
        }
    }
    SafeRelease(target);
    SafeRelease(resource);
    SafeRelease(rtv);
    SafeRelease(dsv);
    device->Release();
    return begun;
}

void End(ID3D11DeviceContext* c) {
    Kept& kept = t_kept;
    c->PSSetShader(kept.ps, nullptr, 0);
    c->PSSetShaderResources(0, 2, kept.srv);
    c->PSSetSamplers(0, 2, kept.sampler);
    c->PSSetConstantBuffers(0, 1, &kept.cb);
    c->OMSetBlendState(kept.blend, kept.factor, kept.mask);
    SafeRelease(kept.ps);
    SafeRelease(kept.srv[0]);
    SafeRelease(kept.srv[1]);
    SafeRelease(kept.sampler[0]);
    SafeRelease(kept.sampler[1]);
    SafeRelease(kept.cb);
    SafeRelease(kept.blend);
}

void NoteMaterial(ID3D11DeviceContext* c, const char* name) {
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    c->VSGetShader(&vs, nullptr, nullptr);
    c->PSGetShader(&ps, nullptr, nullptr);
    Log("Glass %s: vertex shader %016llx, pixel shader %016llx, context %s", name, static_cast<unsigned long long>(shaders::ChecksumOfVs(vs)), static_cast<unsigned long long>(shaders::ChecksumOf(ps)),
        c->GetType() == D3D11_DEVICE_CONTEXT_IMMEDIATE ? "immediate" : "deferred");
    SafeRelease(vs);
    SafeRelease(ps);
    ID3D11DepthStencilState* depth = nullptr;
    UINT ref = 0;
    c->OMGetDepthStencilState(&depth, &ref);
    D3D11_DEPTH_STENCIL_DESC dd{};
    if (depth)
        depth->GetDesc(&dd);
    Log("Glass %s: depth %s, test %d, writes %d; stencil %d, ref %u", name, depth ? "state" : "default", dd.DepthEnable ? static_cast<int>(dd.DepthFunc) : 0, static_cast<int>(dd.DepthWriteMask), dd.StencilEnable ? 1 : 0, ref);
    SafeRelease(depth);
    ID3D11RasterizerState* raster = nullptr;
    c->RSGetState(&raster);
    D3D11_RASTERIZER_DESC rd{};
    if (raster)
        raster->GetDesc(&rd);
    Log("Glass %s: cull %d, front counter-clockwise %d, scissor %d, depth clip %d", name, static_cast<int>(rd.CullMode), rd.FrontCounterClockwise ? 1 : 0, rd.ScissorEnable ? 1 : 0, rd.DepthClipEnable ? 1 : 0);
    SafeRelease(raster);
    ID3D11Buffer* vb = nullptr;
    UINT stride = 0, offset = 0;
    c->IAGetVertexBuffers(0, 1, &vb, &stride, &offset);
    Log("Glass %s: vertices from byte %u, stride %u", name, offset, stride);
    SafeRelease(vb);
    // What its pixel shader is given: read as it is now, in the immediate context only.
    if (c->GetType() == D3D11_DEVICE_CONTEXT_IMMEDIATE) {
        ID3D11Device* device = nullptr;
        c->GetDevice(&device);
        ID3D11Buffer* buffers[4] = {};
        c->PSGetConstantBuffers(0, 4, buffers);
        for (int slot = 0; slot < 4; ++slot) {
            if (!buffers[slot])
                continue;
            D3D11_BUFFER_DESC bd{};
            buffers[slot]->GetDesc(&bd);
            D3D11_BUFFER_DESC sd{};
            sd.ByteWidth = bd.ByteWidth;
            sd.Usage = D3D11_USAGE_STAGING;
            sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            ID3D11Buffer* staging = nullptr;
            if (device && SUCCEEDED(device->CreateBuffer(&sd, nullptr, &staging))) {
                c->CopyResource(staging, buffers[slot]);
                D3D11_MAPPED_SUBRESOURCE mapped{};
                if (SUCCEEDED(c->Map(staging, 0, D3D11_MAP_READ, 0, &mapped))) {
                    const auto* f = static_cast<const float*>(mapped.pData);
                    // The view's: its projection (projMatrix at +112, the shaders' reflection), what a model's size on the screen goes by.
                    if (slot == 2 && bd.ByteWidth >= 116 && f[28] > 0.01f)
                        Log("Glass %s: the view's projection %.3f: a FOV of %.2f", name, f[28], 2.0f * std::atan(1.0f / f[28]) * 57.29578f);
                    const UINT count = std::min<UINT>(bd.ByteWidth / 4, slot == 1 ? 16 : 0);
                    for (UINT i = 0; i < count; i += 8) {
                        char line[200];
                        int used = 0;
                        for (UINT k = i; k < i + 8 && k < count && used < 180; ++k)
                            used += std::snprintf(line + used, sizeof(line) - used, " %.4g", f[k]);
                        Log("Glass %s: its pixel shader's constants %d, c%u:%s", name, slot, i / 4, line);
                    }
                    c->Unmap(staging, 0);
                }
                staging->Release();
            }
            buffers[slot]->Release();
        }
        SafeRelease(device);
    }
    ID3D11ShaderResourceView* views[8] = {};
    c->PSGetShaderResources(0, 8, views);
    for (int i = 0; i < 8; ++i) {
        if (!views[i])
            continue;
        ID3D11Resource* resource = nullptr;
        views[i]->GetResource(&resource);
        D3D11_RESOURCE_DIMENSION dim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
        if (resource)
            resource->GetType(&dim);
        if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D) {
            auto* texture = static_cast<ID3D11Texture2D*>(resource);
            D3D11_TEXTURE2D_DESC td{};
            texture->GetDesc(&td);
            Log("Glass %s: texture %d: %ux%u, format %d, %u mips, array %u, %s", name, i, td.Width, td.Height, static_cast<int>(td.Format), td.MipLevels, td.ArraySize, td.Width * td.Height > 4096 * 2048 ? "too big to keep" : "saved");
            if (td.Width * td.Height <= 4096 * 2048 && td.SampleDesc.Count == 1) {
                std::lock_guard lock(g_pendingMutex);
                Pending p;
                texture->AddRef();
                p.texture = texture;
                std::snprintf(p.name, sizeof(p.name), "%s_t%d", name, i);
                g_pending.push_back(p);
            }
        } else {
            Log("Glass %s: texture %d: not 2D (%d)", name, i, static_cast<int>(dim));
        }
        SafeRelease(resource);
        views[i]->Release();
    }
}

void OnFrame() {
    // A sight's markings wanted for the first time.
    const scopemarks::Marks* wanted = nullptr;
    {
        std::lock_guard lock(g_pendingMutex);
        for (const Map& m : g_maps)
            if (!m.view && !m.failed) {
                wanted = m.marks;
                break;
            }
    }
    if (wanted) {
        if (ID3D11Texture2D* sheet = shaders::ScopeSheet()) {
            ID3D11Device* device = nullptr;
            sheet->GetDevice(&device);
            ID3D11DeviceContext* immediate = nullptr;
            if (device)
                device->GetImmediateContext(&immediate);
            ID3D11ShaderResourceView* view = immediate ? DrawMap(device, immediate, sheet, *wanted) : nullptr;
            Log("Scope: a sight's markings %s: %d dark, %d bright, reaching %.4f", view ? "drawn" : "could not be drawn", wanted->darkCount, wanted->brightCount, wanted->reach);
            {
                std::lock_guard lock(g_pendingMutex);
                for (Map& m : g_maps)
                    if (m.marks == wanted) {
                        m.view = view;
                        m.failed = !view;
                    }
            }
            SafeRelease(immediate);
            SafeRelease(device);
            sheet->Release();
        } else {
            static bool s_said = false;
            if (!s_said)
                Log("Scope: the game's sheet of markings hasn't been made: no lines in the glass");
            s_said = true;
        }
    }
    std::vector<Pending> list;
    {
        std::lock_guard lock(g_pendingMutex);
        list.swap(g_pending);
    }
    for (Pending& p : list) {
        ID3D11Device* device = nullptr;
        p.texture->GetDevice(&device);
        ID3D11DeviceContext* immediate = nullptr;
        if (device)
            device->GetImmediateContext(&immediate);
        D3D11_TEXTURE2D_DESC d{};
        p.texture->GetDesc(&d);
        const UINT from = D3D11CalcSubresource(0, 0, d.MipLevels);
        d.MipLevels = 1;
        d.ArraySize = 1;
        d.Usage = D3D11_USAGE_STAGING;
        d.BindFlags = 0;
        d.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        d.MiscFlags = 0;
        ID3D11Texture2D* staging = nullptr;
        if (immediate && SUCCEEDED(device->CreateTexture2D(&d, nullptr, &staging))) {
            immediate->CopySubresourceRegion(staging, 0, 0, 0, 0, p.texture, from, nullptr);
            D3D11_MAPPED_SUBRESOURCE mapped{};
            if (SUCCEEDED(immediate->Map(staging, 0, D3D11_MAP_READ, 0, &mapped))) {
                char path[MAX_PATH] = {};
                HMODULE self = nullptr;
                GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(&OnFrame), &self);
                GetModuleFileNameA(self, path, MAX_PATH);
                if (char* slash = std::strrchr(path, '\\'))
                    slash[1] = 0;
                strcat_s(path, "glass\\");
                CreateDirectoryA(path, nullptr);
                char file[MAX_PATH];
                std::snprintf(file, sizeof(file), "%s%s_%ux%u_f%d_p%u.bin", path, p.name, d.Width, d.Height, static_cast<int>(d.Format), mapped.RowPitch);
                FILE* f = nullptr;
                if (fopen_s(&f, file, "wb") == 0 && f) {
                    // Blocks of 4 rows in the compressed formats; the pitch says each row's bytes.
                    const bool blocks = d.Format >= DXGI_FORMAT_BC1_TYPELESS && d.Format <= DXGI_FORMAT_BC5_SNORM;
                    const UINT rows = blocks ? (d.Height + 3) / 4 : d.Height;
                    fwrite(mapped.pData, mapped.RowPitch, rows, f);
                    fclose(f);
                    Log("Glass: saved %s", file);
                }
                immediate->Unmap(staging, 0);
            }
            staging->Release();
        }
        SafeRelease(immediate);
        SafeRelease(device);
        p.texture->Release();
    }
}

}

#include "overlay.h"

#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "camera.h"
#include "eyefx.h"
#include "gputime.h"
#include "log.h"
#include "scopeview.h"
#include "shaders.h"
#include "views.h"

namespace {

// ---- Each eye's own passes, the HUD's depth (2026-11-25: build\designs\per_eye_effects.md) ----
// weapon.txt "dofroute 0": the depth of field's two composites (eyefx.h) routed by their viewport alone, as before 2026-11-25. On: by what they are, each
// view's own drawn over its own eye only, whatever viewport it comes with (the suppression blur's clear middle came with the window's, which was routed
// right already; the depth of field by distance had never been seen: with an eye's viewport the left eye would have had the right eye's blurred picture).
std::atomic<bool> g_dofRoute{true};
// "fxcensus 0": no screen effects' census. On: once a second at most, a line when an effect drawn by the first view found no match after the second's
// picture, when something after it read an eye's picture with an eye's viewport, and what was held (nothing changes by it).
std::atomic<bool> g_census{true};
// "fxreaders 1": an effect on the window with an eye's viewport that reads its view's own picture taken as that view's own pass (the first view's drawn
// into the left eye, the second's into the right half only), as the depth of field is. Off (the default till a film shows one): as before.
std::atomic<bool> g_readers{false};
// "fxhold 1": an effect the first view draws with no match after the second's picture three frames running (the census's "alone") drawn into the left eye
// as the game draws it and again into the right half just after the right eye's picture: the right eye gets it too. Off (the default): skipped, as before.
std::atomic<bool> g_hold{false};
// "huddepth 2": the HUD on a flat plane 2 m ahead (each eye's copy moved toward the other by half the disparity: eyefx.h); 0 (the default): at infinity,
// the same pixels in both eyes, as before. Which depth is the owner's to choose (1.5, 2, 3 m side by side): at infinity the HUD doubles over the gun in the
// hands, at 2 m over what's far (and the world markers with it, till each gets its own depth), so nothing is moved until he has.
std::atomic<float> g_hudDepth{0.0f};
// "hudscale 0.8": the HUD drawn that size about each eye's middle (the owner's call); 1: as the game lays it out.
std::atomic<float> g_hudScale{1.0f};
// A re-issued held effect passes through the hooks untouched.
thread_local bool t_reissuing = false;

// ---- The enemies' orange markers, taken off ----
// BF3's HUD marks enemies with orange diamonds (squadmates and objectives blue); in VR they're drawn
// at the screen's distance, not the enemy's, and the user wants them gone (2026-10-31: they were the
// "crosshairs"). Their draws can't be told from the blue ones (the same icon sheet and shaders, the
// colour put on inside the game's own shader), so the window's picture is copied just before the
// HUD is drawn, and at Present, wherever the HUD made a pixel more orange, it goes back to what it
// was. Blue markers, white text and red damage marks are left. weapon.txt "orangemarkers 1": keep
// them.
std::atomic<bool> g_keepOrange{false};
std::atomic<bool> g_keepCrosshair{false};  // weapon.txt "crosshair 1": the game's crosshair left on (Draw)

// Two passes: where the HUD made a pixel more orange (a mask, its mips made by the hardware), then
// everything the HUD drew near that put back as it was: the markers' white rim and dark shadow
// aren't orange themselves and stayed as a ghost ring when only the orange was taken off.
constexpr char kKeyShader[] = R"(
Texture2D<float4> before : register(t0);
Texture2D<float4> after : register(t1);
Texture2D<float> mask : register(t2);
SamplerState linearClamp : register(s0);
void VS(uint id : SV_VertexID, out float4 pos : SV_Position, out float2 uv : TEXCOORD0) {
    uv = float2((id << 1) & 2, id & 2);
    pos = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
}
// How orange a colour is: green between red and blue, as far from both as it can be.
float Orange(float3 c) { return min(c.r - c.g, c.g - c.b); }
float Mark(float4 pos : SV_Position, float2 uv : TEXCOORD0) : SV_Target {
    int3 p = int3(pos.xy, 0);
    float4 b = before.Load(p), a = after.Load(p);
    // The HUD's own pixel orange too: a white or grey icon over the teal scene is "more orange" than the scene
    // and was cut out as a square (the blur study, 2026-10-31).
    return Orange(a.rgb) > 0.05 && Orange(a.rgb) - Orange(b.rgb) > 0.06 && a.r > b.r + 0.03 ? 1 : 0;
}
float4 PutBack(float4 pos : SV_Position, float2 uv : TEXCOORD0) : SV_Target {
    int3 p = int3(pos.xy, 0);
    // A 32-pixel mip, sampled between texels: any orange within about 40 pixels.
    return mask.SampleLevel(linearClamp, uv, 5) > 0.0005 ? before.Load(p) : after.Load(p);
}
)";

struct KeyResources {
    ID3D11Texture2D* before = nullptr;
    ID3D11Texture2D* after = nullptr;
    ID3D11Texture2D* mask = nullptr;            // where the HUD put orange, with mips
    ID3D11ShaderResourceView* views[3] = {};    // before, after, mask
    ID3D11RenderTargetView* maskTarget = nullptr;
    UINT width = 0, height = 0;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* mark = nullptr;
    ID3D11PixelShader* putBack = nullptr;
    ID3D11SamplerState* sampler = nullptr;
    ID3D11BlendState* blend = nullptr;
    ID3D11DepthStencilState* depth = nullptr;
    ID3D11RasterizerState* raster = nullptr;
    bool failed = false;
} g_key;
// This frame: the window's target as the HUD was drawn into it (held), its picture before the HUD.
ID3D11RenderTargetView* g_keyTarget = nullptr;
ID3D11Texture2D* g_keyTexture = nullptr;
std::atomic<uint32_t> g_keyFrames{0};

template <class T> void Drop(T*& p) {
    if (p)
        p->Release();
    p = nullptr;
}

// The shaders and states once, the two copies of the window's size and format as it is.
bool EnsureKey(ID3D11Device* device, const D3D11_TEXTURE2D_DESC& target) {
    if (g_key.failed)
        return false;
    if (!g_key.mark) {
        auto compile = [&](const char* entry, const char* profile, ID3DBlob** out) {
            ID3DBlob* errors = nullptr;
            const bool ok = SUCCEEDED(D3DCompile(kKeyShader, sizeof(kKeyShader) - 1, "orange", nullptr, nullptr, entry,
                                                 profile, 0, 0, out, &errors));
            if (errors) {
                Log("Overlay: orange markers' shader %s: %s", entry, static_cast<const char*>(errors->GetBufferPointer()));
                errors->Release();
            }
            return ok;
        };
        ID3DBlob* vs = nullptr;
        ID3DBlob* mark = nullptr;
        ID3DBlob* putBack = nullptr;
        const bool ok =
            compile("VS", "vs_4_0", &vs) && compile("Mark", "ps_4_0", &mark) && compile("PutBack", "ps_4_0", &putBack) &&
            SUCCEEDED(device->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &g_key.vs)) &&
            SUCCEEDED(device->CreatePixelShader(mark->GetBufferPointer(), mark->GetBufferSize(), nullptr, &g_key.mark)) &&
            SUCCEEDED(device->CreatePixelShader(putBack->GetBufferPointer(), putBack->GetBufferSize(), nullptr,
                                                &g_key.putBack));
        for (ID3DBlob* b : {vs, mark, putBack})
            if (b)
                b->Release();
        D3D11_BLEND_DESC bd{};
        bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        D3D11_DEPTH_STENCIL_DESC dd{};
        D3D11_RASTERIZER_DESC rd{};
        rd.FillMode = D3D11_FILL_SOLID;
        rd.CullMode = D3D11_CULL_NONE;
        rd.DepthClipEnable = TRUE;
        D3D11_SAMPLER_DESC sd{};
        sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.MaxLOD = D3D11_FLOAT32_MAX;
        if (!ok || FAILED(device->CreateBlendState(&bd, &g_key.blend)) ||
            FAILED(device->CreateDepthStencilState(&dd, &g_key.depth)) ||
            FAILED(device->CreateRasterizerState(&rd, &g_key.raster)) || FAILED(device->CreateSamplerState(&sd, &g_key.sampler))) {
            g_key.failed = true;
            Log("Overlay: the orange markers can't be taken off (no shader)");
            return false;
        }
    }
    if (g_key.before && g_key.width == target.Width && g_key.height == target.Height && g_key.format == target.Format)
        return true;
    for (ID3D11ShaderResourceView*& v : g_key.views)
        Drop(v);
    Drop(g_key.maskTarget);
    Drop(g_key.before);
    Drop(g_key.after);
    Drop(g_key.mask);
    D3D11_TEXTURE2D_DESC d = target;
    d.MipLevels = 1;
    d.ArraySize = 1;
    d.Usage = D3D11_USAGE_DEFAULT;
    d.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    d.CPUAccessFlags = 0;
    d.MiscFlags = 0;
    D3D11_TEXTURE2D_DESC m = d;
    m.Format = DXGI_FORMAT_R16_FLOAT;  // a lone orange pixel survives five mips (in 8 bits it rounds away)
    m.MipLevels = 0;  // all of them
    m.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    m.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
    D3D11_RENDER_TARGET_VIEW_DESC mv{};
    mv.Format = DXGI_FORMAT_R16_FLOAT;
    mv.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    if (FAILED(device->CreateTexture2D(&d, nullptr, &g_key.before)) || FAILED(device->CreateTexture2D(&d, nullptr, &g_key.after)) ||
        FAILED(device->CreateTexture2D(&m, nullptr, &g_key.mask)) ||
        FAILED(device->CreateShaderResourceView(g_key.before, nullptr, &g_key.views[0])) ||
        FAILED(device->CreateShaderResourceView(g_key.after, nullptr, &g_key.views[1])) ||
        FAILED(device->CreateShaderResourceView(g_key.mask, nullptr, &g_key.views[2])) ||
        FAILED(device->CreateRenderTargetView(g_key.mask, &mv, &g_key.maskTarget))) {
        for (ID3D11ShaderResourceView*& v : g_key.views)
            Drop(v);
        Drop(g_key.maskTarget);
        Drop(g_key.before);
        Drop(g_key.after);
        Drop(g_key.mask);
        g_key.failed = true;
        Log("Overlay: the orange markers can't be taken off (no copy of the window, format %d)", target.Format);
        return false;
    }
    g_key.width = target.Width, g_key.height = target.Height, g_key.format = target.Format;
    return true;
}

// Just before the frame's first HUD draw: the window's picture as it is.
void TakeBefore(ID3D11DeviceContext* ctx) {
    if (g_keepOrange.load(std::memory_order_relaxed) || g_keyTarget || g_key.failed)
        return;
    ID3D11RenderTargetView* target = nullptr;
    ctx->OMGetRenderTargets(1, &target, nullptr);
    if (!target)
        return;
    ID3D11Resource* resource = nullptr;
    target->GetResource(&resource);
    ID3D11Texture2D* texture = nullptr;
    if (resource && SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&texture)))) {
        D3D11_TEXTURE2D_DESC d;
        texture->GetDesc(&d);
        ID3D11Device* device = nullptr;
        ctx->GetDevice(&device);
        if (device && d.SampleDesc.Count == 1 && d.MipLevels == 1 && d.ArraySize == 1 && EnsureKey(device, d)) {
            ctx->CopyResource(g_key.before, texture);
            g_keyTarget = target;  // held until Present
            target = nullptr;
            g_keyTexture = texture;
            texture = nullptr;
        }
        if (device)
            device->Release();
    }
    Drop(texture);
    Drop(resource);
    Drop(target);
}

// At Present: the HUD's orange put back to what was under it.
void TakeOffOrange(ID3D11DeviceContext* ctx, IDXGISwapChain* swap) {
    if (!g_keyTarget)
        return;
    ctx->CopyResource(g_key.after, g_keyTexture);
    // The pipeline state this changes, put back afterwards (this runs inside the game's Present).
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11DepthStencilView* dsv = nullptr;
    ctx->OMGetRenderTargets(1, &rtv, &dsv);
    D3D11_VIEWPORT viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    UINT viewportCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    ctx->RSGetViewports(&viewportCount, viewports);
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11GeometryShader* gs = nullptr;
    ctx->VSGetShader(&vs, nullptr, nullptr);
    ctx->PSGetShader(&ps, nullptr, nullptr);
    ctx->GSGetShader(&gs, nullptr, nullptr);
    ID3D11InputLayout* layout = nullptr;
    ctx->IAGetInputLayout(&layout);
    D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ctx->IAGetPrimitiveTopology(&topology);
    ID3D11ShaderResourceView* srvs[3] = {};
    ctx->PSGetShaderResources(0, 3, srvs);
    ID3D11SamplerState* sampler = nullptr;
    ctx->PSGetSamplers(0, 1, &sampler);
    ID3D11BlendState* blend = nullptr;
    FLOAT factor[4] = {};
    UINT mask = 0;
    ctx->OMGetBlendState(&blend, factor, &mask);
    ID3D11DepthStencilState* depth = nullptr;
    UINT ref = 0;
    ctx->OMGetDepthStencilState(&depth, &ref);
    ID3D11RasterizerState* raster = nullptr;
    ctx->RSGetState(&raster);

    const D3D11_VIEWPORT full{0.0f, 0.0f, static_cast<float>(g_key.width), static_cast<float>(g_key.height), 0.0f, 1.0f};
    ctx->RSSetViewports(1, &full);
    ctx->VSSetShader(g_key.vs, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->OMSetBlendState(g_key.blend, nullptr, 0xFFFFFFFF);
    ctx->OMSetDepthStencilState(g_key.depth, 0);
    ctx->RSSetState(g_key.raster);
    ctx->PSSetSamplers(0, 1, &g_key.sampler);
    // Where the HUD put orange, spread by the mask's mips...
    ID3D11ShaderResourceView* none[3] = {};
    ctx->PSSetShaderResources(0, 3, none);
    ctx->OMSetRenderTargets(1, &g_key.maskTarget, nullptr);
    ctx->PSSetShaderResources(0, 2, g_key.views);
    ctx->PSSetShader(g_key.mark, nullptr, 0);
    ctx->Draw(3, 0);
    ctx->OMSetRenderTargets(0, nullptr, nullptr);
    ctx->GenerateMips(g_key.views[2]);
    // ...and everything the HUD drew near it put back as it was.
    ctx->OMSetRenderTargets(1, &g_keyTarget, nullptr);
    ctx->PSSetShaderResources(0, 3, g_key.views);
    ctx->PSSetShader(g_key.putBack, nullptr, 0);
    ctx->Draw(3, 0);

    ctx->PSSetShaderResources(0, 3, none);
    ctx->OMSetRenderTargets(1, &rtv, dsv);
    ctx->RSSetViewports(viewportCount, viewports);
    ctx->VSSetShader(vs, nullptr, 0);
    ctx->PSSetShader(ps, nullptr, 0);
    ctx->GSSetShader(gs, nullptr, 0);
    ctx->IASetInputLayout(layout);
    ctx->IASetPrimitiveTopology(topology);
    ctx->PSSetShaderResources(0, 3, srvs);
    ctx->PSSetSamplers(0, 1, &sampler);
    ctx->OMSetBlendState(blend, factor, mask);
    ctx->OMSetDepthStencilState(depth, ref);
    ctx->RSSetState(raster);
    Drop(rtv);
    Drop(dsv);
    Drop(vs);
    Drop(ps);
    Drop(gs);
    Drop(layout);
    for (ID3D11ShaderResourceView*& v : srvs)
        Drop(v);
    Drop(sampler);
    Drop(blend);
    Drop(depth);
    Drop(raster);

    if (g_keyFrames.fetch_add(1, std::memory_order_relaxed) == 0) {
        ID3D11Texture2D* back = nullptr;
        const bool same = swap && SUCCEEDED(swap->GetBuffer(0, IID_PPV_ARGS(&back))) && back == g_keyTexture;
        Drop(back);
        Log("Overlay: the enemies' orange markers are taken off (the HUD drawn %s)",
            same ? "into the window's own picture" : "into a picture of its own: CHECK it reaches the window");
    }
    Drop(g_keyTarget);
    Drop(g_keyTexture);
}

// The window's pictures and overlays are small draws: a quad or two each (6 to 36 vertices or
// indices in the frames logged with weapon.txt "tail", 2026-10-30).
constexpr UINT kMostVertices = 600;

// This frame so far: how many eyes' pictures have gone onto the window (the left eye's first).
std::atomic<int> g_eyePictures{0};

// Each eye's own effects on the window this frame (the damage tint, drawn right after each eye's
// picture), as the left eye's came: by pixel shader and vertex count. The right eye's are the same
// draws after its picture; anything else then is the HUD. (By shader alone, the screen's blood
// spatter, drawn once for the HUD with the tint's shader, went to the right eye only.)
struct EyeEffect {
    ID3D11PixelShader* ps;
    UINT count;
    uint64_t sum = 0;  // its checksum (shaders.h), for the census
};
std::mutex g_shadersMutex;
std::vector<EyeEffect> g_eyeEffects;
// This frame's own passes of the first view drawn into the left eye between the pictures (the depth of field, "fxreaders"): the second view's like them go
// into the right half only. Under g_shadersMutex.
std::vector<EyeEffect> g_ownPasses;
// This frame's checksums of the first view's effects that found their match after the second's picture (the census, "fxhold"). Under g_shadersMutex.
std::vector<uint64_t> g_matchedSums;

// The census: what it found since the last line, by kind and checksum. Under g_shadersMutex.
enum CensusKind { kAlone, kEyeReader, kOwnPass, kHeldKind, kDofFirstAlone, kDofSecondAlone, kCensusKinds };
const char* const kCensusNames[kCensusKinds] = {"drawn by the first view with no match after the second's picture",
                                                "read an eye's picture with an eye's viewport after the right eye's picture (drawn at the left and into the right half)",
                                                "a view's own pass (the left eye's between the pictures, the right half only after)",
                                                "held for the right eye (fxhold)",
                                                "a depth of field composite drawn by the first view alone (the left eye's own, none in the right)",
                                                "a depth of field composite drawn by the second view alone (as before: an eye's viewport into both eyes, the window's into the "
                                                "right half)"};
struct CensusRow {
    uint64_t sum;
    uint8_t kind;
    UINT vertices;
    uint32_t count;      // since the last line
    ULONGLONG logged;    // when it was last in a line (0 never)
};
constexpr size_t kMostCensusRows = 32;
std::vector<CensusRow> g_censusRows;
// The first view's effects that went without a match, frames running, by checksum ("fxhold" holds one at 3). Under g_shadersMutex.
struct Streak {
    uint64_t sum;
    int frames;
};
std::vector<Streak> g_streaks;
constexpr int kHoldAfter = 3;

void CensusAdd(uint64_t sum, CensusKind kind, UINT vertices) {
    if (!g_census.load(std::memory_order_relaxed))
        return;
    for (CensusRow& r : g_censusRows)
        if (r.sum == sum && r.kind == kind && r.vertices == vertices) {
            ++r.count;
            return;
        }
    if (g_censusRows.size() < kMostCensusRows)
        g_censusRows.push_back({sum, static_cast<uint8_t>(kind), vertices, 1, 0});
}

int StreakOf(uint64_t sum) {
    for (const Streak& s : g_streaks)
        if (s.sum == sum)
            return s.frames;
    return 0;
}
// Pixel shaders of the HUD that read a picture of an eye's scene (an eye's size, a half or a
// quarter of it; the HUD's own pictures are 1024 square): true for those, asked once each.
std::unordered_map<ID3D11PixelShader*, bool> g_readsScene;

std::atomic<int> g_logged{0};
// weapon.txt "tail": the next frame's choices logged (1 asked, 2 this frame). "hudfix N": an
// experiment for the ammo counter that came out in the right eye only (2026-10-30): 1 = the left
// eye's copy drawn without the depth-stencil view.
std::atomic<int> g_logFrame{0};
std::atomic<int> g_hudFix{0};
// HUD draws after the right eye's picture, and the right eye's copies of them, counted for a log
// line once a second (tools/selftest.ps1 checks they match: the HUD in both eyes).
std::atomic<uint32_t> g_hudDraws{0}, g_hudCopies{0};
// weapon.txt "hudskip 10-19 40": the HUD's draws with those numbers in the frame (from 0, in the
// order the game draws them) left out, to tell which is which (the crosshair, 2026-10-31).
constexpr int kSkippable = 256;
std::atomic<uint64_t> g_hudSkip[kSkippable / 64] = {};
std::atomic<int> g_hudIndex{0};
// weapon.txt "hudskiptex 64x32 4096x128": the HUD's draws whose first texture is that size left out
// (sizes stay from run to run; the pointers don't).
std::atomic<uint32_t> g_hudSkipTex[4] = {};  // width << 16 | height, 0 unused

// The size of the pixel shader's first texture (width << 16 | height), 0 none.
uint32_t FirstTextureSize(ID3D11DeviceContext* ctx, DXGI_FORMAT* format = nullptr) {
    if (format)
        *format = DXGI_FORMAT_UNKNOWN;
    ID3D11ShaderResourceView* view = nullptr;
    ctx->PSGetShaderResources(0, 1, &view);
    if (!view)
        return 0;
    uint32_t size = 0;
    ID3D11Resource* resource = nullptr;
    view->GetResource(&resource);
    ID3D11Texture2D* texture = nullptr;
    if (resource && SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&texture)))) {
        D3D11_TEXTURE2D_DESC td;
        texture->GetDesc(&td);
        size = td.Width << 16 | (td.Height & 0xFFFF);
        if (format)
            *format = td.Format;
        texture->Release();
    }
    if (resource)
        resource->Release();
    view->Release();
    return size;
}

// A small RGBA texture's first mip read back and written as a 32-bit .bmp beside the DLL
// (hudtex_<draw>.bmp), with its average colour where it isn't see-through logged.
void SaveSmallTexture(ID3D11DeviceContext* ctx, ID3D11Texture2D* texture, const D3D11_TEXTURE2D_DESC& td, int index) {
    ID3D11Device* device = nullptr;
    texture->GetDevice(&device);
    D3D11_TEXTURE2D_DESC sd = td;
    sd.MipLevels = 1;
    sd.ArraySize = 1;
    sd.Usage = D3D11_USAGE_STAGING;
    sd.BindFlags = 0;
    sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    sd.MiscFlags = 0;
    ID3D11Texture2D* copy = nullptr;
    if (device && SUCCEEDED(device->CreateTexture2D(&sd, nullptr, &copy))) {
        ctx->CopySubresourceRegion(copy, 0, 0, 0, 0, texture, 0, nullptr);
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(ctx->Map(copy, 0, D3D11_MAP_READ, 0, &mapped))) {
            std::vector<uint8_t> pixels(td.Width * td.Height * 4);
            double sum[3] = {}, weight = 0;
            for (UINT y = 0; y < td.Height; ++y)
                for (UINT x = 0; x < td.Width; ++x) {
                    const uint8_t* p = static_cast<const uint8_t*>(mapped.pData) + y * mapped.RowPitch + x * 4;
                    uint8_t* q = &pixels[((td.Height - 1 - y) * td.Width + x) * 4];  // bottom-up, BGRA
                    q[0] = p[2], q[1] = p[1], q[2] = p[0], q[3] = p[3];
                    for (int c = 0; c < 3; ++c)
                        sum[c] += p[c] * (p[3] / 255.0);
                    weight += p[3] / 255.0;
                }
            ctx->Unmap(copy, 0);
            wchar_t path[MAX_PATH];
            HMODULE self = nullptr;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCWSTR>(&SaveSmallTexture), &self);
            GetModuleFileNameW(self, path, MAX_PATH);
            if (wchar_t* slash = wcsrchr(path, L'\\'))
                swprintf_s(slash + 1, MAX_PATH - (slash + 1 - path), L"hudtex_%d.bmp", index);
            FILE* f = nullptr;
            if (_wfopen_s(&f, path, L"wb") == 0 && f) {
                BITMAPFILEHEADER fh{};
                BITMAPINFOHEADER ih{};
                ih.biSize = sizeof(ih);
                ih.biWidth = static_cast<LONG>(td.Width);
                ih.biHeight = static_cast<LONG>(td.Height);
                ih.biPlanes = 1;
                ih.biBitCount = 32;
                fh.bfType = 0x4D42;
                fh.bfOffBits = sizeof(fh) + sizeof(ih);
                fh.bfSize = fh.bfOffBits + static_cast<DWORD>(pixels.size());
                fwrite(&fh, sizeof(fh), 1, f);
                fwrite(&ih, sizeof(ih), 1, f);
                fwrite(pixels.data(), 1, pixels.size(), f);
                fclose(f);
            }
            Log("Overlay: HUD draw %d texture saved (hudtex_%d.bmp); its colour where drawn: %.0f %.0f %.0f", index,
                index, weight > 0 ? sum[0] / weight : 0.0, weight > 0 ? sum[1] / weight : 0.0,
                weight > 0 ? sum[2] / weight : 0.0);
        }
        copy->Release();
    }
    if (device)
        device->Release();
}

// A buffer's bytes [offset, offset + size) read back through a staging copy.
bool ReadBuffer(ID3D11DeviceContext* ctx, ID3D11Buffer* buffer, UINT offset, UINT size, void* out) {
    D3D11_BUFFER_DESC desc;
    buffer->GetDesc(&desc);
    if (!size || offset + size > desc.ByteWidth)
        return false;
    ID3D11Device* device = nullptr;
    buffer->GetDevice(&device);
    D3D11_BUFFER_DESC sd{};
    sd.ByteWidth = (size + 15) & ~15u;
    sd.Usage = D3D11_USAGE_STAGING;
    sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Buffer* copy = nullptr;
    bool ok = false;
    if (device && SUCCEEDED(device->CreateBuffer(&sd, nullptr, &copy))) {
        const D3D11_BOX box{offset, 0, 0, offset + size, 1, 1};
        ctx->CopySubresourceRegion(copy, 0, 0, 0, 0, buffer, 0, &box);
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(ctx->Map(copy, 0, D3D11_MAP_READ, 0, &mapped))) {
            std::memcpy(out, mapped.pData, size);
            ctx->Unmap(copy, 0);
            ok = true;
        }
        copy->Release();
    }
    if (device)
        device->Release();
    return ok;
}

// weapon.txt "tail": a small HUD draw's vertices, raw (a vertex colour would tell the markers apart).
void LogHudVertices(ID3D11DeviceContext* ctx, int index, UINT count, UINT start, INT base) {
    ID3D11Buffer* ib = nullptr;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    UINT ibOffset = 0;
    ctx->IAGetIndexBuffer(&ib, &format, &ibOffset);
    ID3D11Buffer* vbs[4] = {};
    UINT strides[4] = {}, offsets[4] = {};
    ctx->IAGetVertexBuffers(0, 4, vbs, strides, offsets);
    const UINT indexSize = format == DXGI_FORMAT_R32_UINT ? 4 : 2;
    uint8_t raw[48] = {};
    if (ib && ReadBuffer(ctx, ib, ibOffset + start * indexSize, count * indexSize, raw)) {
        UINT first = indexSize == 4 ? *reinterpret_cast<uint32_t*>(raw) : *reinterpret_cast<uint16_t*>(raw);
        for (int s = 0; s < 4; ++s) {  // each stream the draw reads
            const UINT stride = strides[s];
            uint8_t v[64] = {};
            if (!vbs[s] || !stride || stride > 64 || !ReadBuffer(ctx, vbs[s], offsets[s] + (first + base) * stride, stride, v))
                continue;
            char line[400];
            int used = 0;
            for (UINT i = 0; i < stride / 4 && used < 360; ++i) {
                float f;
                std::memcpy(&f, v + i * 4, 4);
                used += snprintf(line + used, sizeof(line) - used, " %08X(%.3g)", *reinterpret_cast<uint32_t*>(v + i * 4), f);
            }
            Log("Overlay: HUD draw %d first vertex, stream %d (stride %u):%s", index, s, stride, line);
        }
    }
    if (ib)
        ib->Release();
    for (ID3D11Buffer* vb : vbs)
        if (vb)
            vb->Release();
}

// weapon.txt "tail": a HUD draw's constants, read back through a staging copy (a stall: the one
// frame logged only): the first floats of the pixel and vertex shaders' first two buffers, and the
// first texture's size. To tell the HUD's pieces apart by their colour (the orange enemy markers,
// 2026-10-31).
void LogHudConstants(ID3D11DeviceContext* ctx, int index) {
    // Direct3D 11.1 binds a part of a constant buffer (the HUD's draws: the start of their 12800-byte
    // buffer held the same post-processing numbers for every one of them): where each slot's starts.
    UINT psFirst[2] = {}, vsFirst[2] = {};
    ID3D11DeviceContext1* ctx1 = nullptr;
    if (SUCCEEDED(ctx->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&ctx1)))) {
        ID3D11Buffer* b[2] = {};
        UINT count[2] = {};
        ctx1->PSGetConstantBuffers1(0, 2, b, psFirst, count);
        for (ID3D11Buffer* x : b)
            if (x)
                x->Release();
        ctx1->VSGetConstantBuffers1(0, 2, b, vsFirst, count);
        for (ID3D11Buffer* x : b)
            if (x)
                x->Release();
        ctx1->Release();
    }
    auto dump = [&](ID3D11Buffer* buffer, const char* stage, int slot) {
        const UINT first = (stage[0] == 'P' ? psFirst : vsFirst)[slot] * 16;  // bytes
        if (!buffer)
            return;
        D3D11_BUFFER_DESC desc;
        buffer->GetDesc(&desc);
        ID3D11Device* device = nullptr;
        buffer->GetDevice(&device);
        D3D11_BUFFER_DESC staging = desc;
        staging.Usage = D3D11_USAGE_STAGING;
        staging.BindFlags = 0;
        staging.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        staging.MiscFlags = 0;
        ID3D11Buffer* copy = nullptr;
        if (device && SUCCEEDED(device->CreateBuffer(&staging, nullptr, &copy))) {
            ctx->CopyResource(copy, buffer);
            D3D11_MAPPED_SUBRESOURCE mapped{};
            if (SUCCEEDED(ctx->Map(copy, 0, D3D11_MAP_READ, 0, &mapped))) {
                const UINT at = first < desc.ByteWidth ? first : 0;
                const auto* f = reinterpret_cast<const float*>(static_cast<const uint8_t*>(mapped.pData) + at);
                char line[400];
                int used = 0;
                for (UINT i = 0; i < std::min<UINT>((desc.ByteWidth - at) / 4, 24) && used < 360; ++i)
                    used += snprintf(line + used, sizeof(line) - used, " %.3g", f[i]);
                Log("Overlay: HUD draw %d %s constants %d (%u bytes, from %u):%s", index, stage, slot, desc.ByteWidth, at,
                    line);
                ctx->Unmap(copy, 0);
            }
            copy->Release();
        }
        if (device)
            device->Release();
    };
    ID3D11Buffer* ps[2] = {};
    ID3D11Buffer* vs[2] = {};
    ctx->PSGetConstantBuffers(0, 2, ps);
    ctx->VSGetConstantBuffers(0, 2, vs);
    for (int i = 0; i < 2; ++i) {
        dump(ps[i], "PS", i);
        dump(vs[i], "VS", i);
        if (ps[i])
            ps[i]->Release();
        if (vs[i])
            vs[i]->Release();
    }
    ID3D11ShaderResourceView* view = nullptr;
    ctx->PSGetShaderResources(0, 1, &view);
    if (view) {
        ID3D11Resource* resource = nullptr;
        view->GetResource(&resource);
        D3D11_TEXTURE2D_DESC td{};
        ID3D11Texture2D* texture = nullptr;
        if (resource && SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&texture)))) {
            texture->GetDesc(&td);
            // A small uncompressed picture (the markers' are 128x64) is saved beside the DLL as
            // hudtex_<draw>.bmp, to see which marker it is, and its average colour logged.
            if (td.Format == DXGI_FORMAT_R8G8B8A8_UNORM && td.Width <= 256 && td.Height <= 256)
                SaveSmallTexture(ctx, texture, td, index);
            texture->Release();
        }
        if (resource)
            resource->Release();
        view->Release();
        Log("Overlay: HUD draw %d texture %p %ux%u format %d", index, static_cast<void*>(resource), td.Width, td.Height,
            td.Format);
    }
}

// The draw with no depth-stencil view bound (no depth or stencil test).
void DrawWithoutDepth(ID3D11DeviceContext* ctx, const std::function<void()>& draw) {
    ID3D11RenderTargetView* target = nullptr;
    ID3D11DepthStencilView* depth = nullptr;
    ctx->OMGetRenderTargets(1, &target, &depth);
    if (!depth) {
        draw();
    } else {
        ctx->OMSetRenderTargets(1, &target, nullptr);
        draw();
        ctx->OMSetRenderTargets(1, &target, depth);
        depth->Release();
    }
    if (target)
        target->Release();
}

bool Blended(ID3D11DeviceContext* ctx) {
    ID3D11BlendState* blend = nullptr;
    FLOAT factor[4];
    UINT mask = 0;
    ctx->OMGetBlendState(&blend, factor, &mask);
    bool blended = false;
    if (blend) {
        D3D11_BLEND_DESC desc;
        blend->GetDesc(&desc);
        blended = desc.RenderTarget[0].BlendEnable != FALSE;
        blend->Release();
    }
    return blended;
}

// The render target's size: the window is twice as wide as high.
bool OnWindow(ID3D11DeviceContext* ctx) {
    ID3D11RenderTargetView* target = nullptr;
    ctx->OMGetRenderTargets(1, &target, nullptr);
    if (!target)
        return false;
    bool window = false;
    ID3D11Resource* resource = nullptr;
    target->GetResource(&resource);
    ID3D11Texture2D* texture = nullptr;
    if (resource && SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&texture)))) {
        D3D11_TEXTURE2D_DESC desc;
        texture->GetDesc(&desc);
        window = desc.Width == 2 * desc.Height;
        texture->Release();
    }
    if (resource)
        resource->Release();
    target->Release();
    return window;
}

ID3D11PixelShader* PixelShader(ID3D11DeviceContext* ctx) {
    ID3D11PixelShader* ps = nullptr;
    ctx->PSGetShader(&ps, nullptr, nullptr);
    if (ps)
        ps->Release();  // only its address is kept, to compare
    return ps;
}

bool ReadsScene(ID3D11DeviceContext* ctx, UINT eye) {
    ID3D11PixelShader* ps = PixelShader(ctx);
    if (!ps)
        return false;
    {
        std::lock_guard lock(g_shadersMutex);
        auto it = g_readsScene.find(ps);
        if (it != g_readsScene.end())
            return it->second;
    }
    bool scene = false;
    ID3D11ShaderResourceView* views[8] = {};
    ctx->PSGetShaderResources(0, 8, views);
    for (ID3D11ShaderResourceView* v : views) {
        if (!v)
            continue;
        ID3D11Resource* resource = nullptr;
        v->GetResource(&resource);
        ID3D11Texture2D* texture = nullptr;
        if (resource && SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&texture)))) {
            D3D11_TEXTURE2D_DESC desc;
            texture->GetDesc(&desc);
            for (UINT part = 1; part <= 4; part *= 2)
                scene = scene || (desc.Width == eye / part && desc.Height == eye / part);
            texture->Release();
        }
        if (resource)
            resource->Release();
        v->Release();
    }
    std::lock_guard lock(g_shadersMutex);
    g_readsScene[ps] = scene;
    return scene;
}

// A picture an eye wide to draw the right eye's things in, as they come out at the window's left
// edge, before they go into its right half.
ID3D11Texture2D* g_half = nullptr;
ID3D11RenderTargetView* g_halfView = nullptr;
UINT g_halfWidth = 0, g_halfHeight = 0;
DXGI_FORMAT g_halfFormat = DXGI_FORMAT_UNKNOWN;

bool HalfTarget(ID3D11Device* device, const D3D11_TEXTURE2D_DESC& window) {
    const UINT width = window.Width / 2;
    if (g_half && g_halfWidth == width && g_halfHeight == window.Height && g_halfFormat == window.Format)
        return true;
    if (g_halfView)
        g_halfView->Release(), g_halfView = nullptr;
    if (g_half)
        g_half->Release(), g_half = nullptr;
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = width;
    desc.Height = window.Height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = window.Format;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    if (FAILED(device->CreateTexture2D(&desc, nullptr, &g_half)) ||
        FAILED(device->CreateRenderTargetView(g_half, nullptr, &g_halfView))) {
        if (g_half)
            g_half->Release(), g_half = nullptr;
        Log("Overlay: no %ux%u picture for the right eye", width, window.Height);
        return false;
    }
    g_halfWidth = width, g_halfHeight = window.Height, g_halfFormat = window.Format;
    return true;
}

// ---- The HUD placed: its viewport scaled about the eye's middle and moved, clipped to its eye ----
// The game's rasterizer states with the scissor test on (a HUD draw moved toward the other eye must not spill into it): made once for each of the game's,
// which is held while it's in the table, so its pointer can't come back as another state's.
struct ClipClone {
    ID3D11RasterizerState* from;  // the game's (null: the default state), held
    ID3D11RasterizerState* clone;
};
std::mutex g_cloneMutex;
std::vector<ClipClone> g_clipClones;
constexpr size_t kMostClipClones = 64;

ID3D11RasterizerState* ScissorClone(ID3D11DeviceContext* ctx, ID3D11RasterizerState* from) {
    std::lock_guard lock(g_cloneMutex);
    for (const ClipClone& c : g_clipClones)
        if (c.from == from)
            return c.clone;
    if (g_clipClones.size() >= kMostClipClones)
        return nullptr;
    D3D11_RASTERIZER_DESC d{};
    if (from) {
        from->GetDesc(&d);
    } else {
        d.FillMode = D3D11_FILL_SOLID;
        d.CullMode = D3D11_CULL_BACK;
        d.DepthClipEnable = TRUE;
    }
    d.ScissorEnable = TRUE;
    ID3D11Device* device = nullptr;
    ctx->GetDevice(&device);
    ID3D11RasterizerState* clone = nullptr;
    if (device) {
        device->CreateRasterizerState(&d, &clone);
        device->Release();
    }
    if (!clone)
        return nullptr;
    if (from)
        from->AddRef();
    g_clipClones.push_back({from, clone});
    return clone;
}

// The draw with its viewport k times as big about the eye's middle and moved dx across, its scissor rectangle alike, clipped to [clipLeft, clipRight) across.
// False (nothing drawn) when no state with the scissor test on can be had: the caller draws as before.
bool DrawPlaced(ID3D11DeviceContext* ctx, const std::function<void()>& draw, const D3D11_VIEWPORT& viewport, float eye, float k, float dx, LONG clipLeft,
                LONG clipRight) {
    ID3D11RasterizerState* rs = nullptr;
    ctx->RSGetState(&rs);
    bool scissorOn = false;
    if (rs) {
        D3D11_RASTERIZER_DESC d;
        rs->GetDesc(&d);
        scissorOn = d.ScissorEnable != FALSE;
    }
    ID3D11RasterizerState* clipping = scissorOn ? rs : ScissorClone(ctx, rs);
    if (!clipping) {
        if (rs)
            rs->Release();
        return false;
    }
    D3D11_RECT saved[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    UINT savedCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    ctx->RSGetScissorRects(&savedCount, saved);
    // Down to the eye's own rows (the HUD is laid out in its square): a rectangle reaching past D3D11's viewport bounds (+-32768) is one a driver isn't
    // promised to clamp (the review of 2e323f2: +-2^20 before, which WARP took).
    const eyefx::Rect clip{clipLeft, 0, clipRight, static_cast<LONG>(eye)};
    eyefx::Rect r = clip;
    if (scissorOn) {
        // The game's own rectangle placed with what it cuts (none set: nothing drawn, as the game's own draw).
        const eyefx::Rect game = savedCount ? eyefx::Rect{saved[0].left, saved[0].top, saved[0].right, saved[0].bottom} : eyefx::Rect{0, 0, 0, 0};
        r = eyefx::Intersect(eyefx::PlaceRect(game, eye, k, dx), clip);
    }
    const D3D11_RECT rect{r.left, r.top, r.right, r.bottom};
    const eyefx::Viewport p = eyefx::Place({viewport.TopLeftX, viewport.TopLeftY, viewport.Width, viewport.Height}, eye, k, dx);
    const D3D11_VIEWPORT placed{p.x, p.y, p.w, p.h, viewport.MinDepth, viewport.MaxDepth};
    ctx->RSSetViewports(1, &placed);
    if (clipping != rs)
        ctx->RSSetState(clipping);
    ctx->RSSetScissorRects(1, &rect);
    draw();
    ctx->RSSetViewports(1, &viewport);
    if (clipping != rs)
        ctx->RSSetState(rs);
    ctx->RSSetScissorRects(savedCount, savedCount ? saved : nullptr);
    if (rs)
        rs->Release();
    return true;
}

// The HUD's place for this frame's draws: each eye's share of the disparity (whole pixels) and the scale. Both 0 and 1: as before, nothing placed.
struct HudPlace {
    int shift = 0;
    float k = 1.0f;
    bool Any() const { return shift != 0 || k != 1.0f; }
};
HudPlace HudPlaceFor(float eyePixels) {
    HudPlace h;
    h.k = eyefx::HudScale(g_hudScale.load(std::memory_order_relaxed));
    const float depth = g_hudDepth.load(std::memory_order_relaxed);
    if (depth > 0.0f) {
        const float ipd = views::EyeOffset(1) - views::EyeOffset(0);
        const float fov = views::RenderedFov();
        static std::atomic<int> s_kept{-1};  // (the render thread's: held against an eye distance read a hair either way: eyefx.h)
        h.shift = eyefx::HudShiftKept(eyefx::HudDisparity(ipd, eyePixels, fov, depth), s_kept.load(std::memory_order_relaxed));
        s_kept.store(h.shift, std::memory_order_relaxed);
        // Said whenever what it comes to changes (an eye distance or an angle the headset gives: not every frame).
        static std::atomic<int> s_said{-1};
        static std::atomic<uint32_t> s_saidSize{0};
        const uint32_t size = static_cast<uint32_t>(eyePixels);
        if (s_said.exchange(h.shift, std::memory_order_relaxed) != h.shift || s_saidSize.exchange(size, std::memory_order_relaxed) != size)
            Log("Overlay: the HUD at %.2f m: each eye's copy moved %d px toward the other (eyes %.1f mm apart, the picture %u px over %.2f degrees)%s", depth,
                h.shift, ipd * 1000.0f, size, fov * 57.2957795f, h.shift ? "" : ": nothing to move (no eye distance or angle yet)");
    }
    return h;
}

// Draws into the window's right half what the draw puts at its left edge: that half into the half
// picture, the draw there (blending onto it as onto the window), and back. Things that read the
// scene by where on the window they are read it right only at the left edge. False when it can't
// (a depth or stencil test wants the window's own depth). place: the HUD's place in the right eye
// (moved left by its shift, scaled), or none.
bool IntoRightHalf(ID3D11DeviceContext* ctx, const std::function<void()>& draw, const HudPlace* place = nullptr,
                   const D3D11_VIEWPORT* viewport = nullptr) {
    ID3D11DepthStencilState* depthState = nullptr;
    UINT ref = 0;
    ctx->OMGetDepthStencilState(&depthState, &ref);
    bool needsDepth = false;
    if (depthState) {
        D3D11_DEPTH_STENCIL_DESC d;
        depthState->GetDesc(&d);
        needsDepth = d.DepthEnable || d.StencilEnable;
        depthState->Release();
    }
    ID3D11RenderTargetView* target = nullptr;
    ID3D11DepthStencilView* depth = nullptr;
    ctx->OMGetRenderTargets(1, &target, &depth);
    ID3D11Resource* window = nullptr;
    ID3D11Texture2D* windowTexture = nullptr;
    ID3D11Device* device = nullptr;
    bool done = false;
    if (target && !(needsDepth && depth)) {
        target->GetResource(&window);
        if (window && SUCCEEDED(window->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&windowTexture)))) {
            D3D11_TEXTURE2D_DESC desc;
            windowTexture->GetDesc(&desc);
            ctx->GetDevice(&device);
            if (device && desc.SampleDesc.Count == 1 && desc.Width == 2 * desc.Height && HalfTarget(device, desc)) {
                const UINT half = desc.Width / 2;
                const D3D11_BOX right{half, 0, 0, desc.Width, desc.Height, 1};
                ctx->CopySubresourceRegion(g_half, 0, 0, 0, 0, windowTexture, 0, &right);
                ctx->OMSetRenderTargets(1, &g_halfView, nullptr);
                const float eye = static_cast<float>(desc.Height);
                if (!place || !viewport || !place->Any() ||
                    !DrawPlaced(ctx, draw, *viewport, eye, place->k, -static_cast<float>(place->shift), 0, static_cast<LONG>(half)))
                    draw();
                ctx->OMSetRenderTargets(1, &target, depth);
                const D3D11_BOX back{0, 0, 0, half, desc.Height, 1};
                ctx->CopySubresourceRegion(windowTexture, 0, half, 0, 0, g_half, 0, &back);
                done = true;
            }
        }
    }
    if (device)
        device->Release();
    if (windowTexture)
        windowTexture->Release();
    if (window)
        window->Release();
    if (depth)
        depth->Release();
    if (target)
        target->Release();
    static std::atomic<bool> s_told{false};
    if (!done && !s_told.exchange(true))
        Log("Overlay: something for the right eye couldn't be drawn into its half (%s)",
            needsDepth ? "it tests depth or stencil" : "no window picture");
    return done;
}

// The draw again a half to the right (viewport and scissor), as a copy for the right eye.
void DrawMovedRight(ID3D11DeviceContext* ctx, const std::function<void()>& draw, const D3D11_VIEWPORT& viewport) {
    const float half = viewport.Height;  // an eye is as wide as the window is high
    D3D11_VIEWPORT moved = viewport;
    moved.TopLeftX += half;
    D3D11_RECT scissor[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE], shifted[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    UINT scissors = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    ctx->RSGetScissorRects(&scissors, scissor);
    for (UINT i = 0; i < scissors; ++i) {
        shifted[i] = scissor[i];
        shifted[i].left += static_cast<LONG>(half);
        shifted[i].right += static_cast<LONG>(half);
    }
    ctx->RSSetViewports(1, &moved);
    if (scissors)
        ctx->RSSetScissorRects(scissors, shifted);
    draw();
    ctx->RSSetViewports(1, &viewport);
    if (scissors)
        ctx->RSSetScissorRects(scissors, scissor);
}

// ---- "fxhold": an effect only the first view draws, held for the right eye ----
// Everything a screen effect's draw is made of, each piece held (Get*), to draw it again later as it was. Shaders past the vertex and pixel shaders aren't
// a screen effect's: a draw with any is never held.
struct PipelineState {
    ID3D11InputLayout* layout = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    static constexpr int kStreams = 4, kBuffers = 4, kViews = 8, kSamplers = 4;
    ID3D11Buffer* vb[kStreams] = {};
    UINT stride[kStreams] = {}, offset[kStreams] = {};
    ID3D11Buffer* ib = nullptr;
    DXGI_FORMAT ibFormat = DXGI_FORMAT_UNKNOWN;
    UINT ibOffset = 0;
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11GeometryShader* gs = nullptr;
    ID3D11HullShader* hs = nullptr;
    ID3D11DomainShader* ds = nullptr;
    ID3D11Buffer* vsCb[kBuffers] = {};
    UINT vsFirst[kBuffers] = {}, vsNum[kBuffers] = {};
    ID3D11Buffer* psCb[kBuffers] = {};
    UINT psFirst[kBuffers] = {}, psNum[kBuffers] = {};
    ID3D11ShaderResourceView* srv[kViews] = {};
    ID3D11SamplerState* samplers[kSamplers] = {};
    ID3D11BlendState* blend = nullptr;
    FLOAT factor[4] = {};
    UINT sampleMask = 0xFFFFFFFF;
    ID3D11DepthStencilState* depth = nullptr;
    UINT ref = 0;
    ID3D11RasterizerState* rs = nullptr;
    UINT viewports = 0;
    D3D11_VIEWPORT viewport[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    UINT scissors = 0;
    D3D11_RECT scissor[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11DepthStencilView* dsv = nullptr;

    void Capture(ID3D11DeviceContext* ctx) {
        ctx->IAGetInputLayout(&layout);
        ctx->IAGetPrimitiveTopology(&topology);
        ctx->IAGetVertexBuffers(0, kStreams, vb, stride, offset);
        ctx->IAGetIndexBuffer(&ib, &ibFormat, &ibOffset);
        ctx->VSGetShader(&vs, nullptr, nullptr);
        ctx->PSGetShader(&ps, nullptr, nullptr);
        ctx->GSGetShader(&gs, nullptr, nullptr);
        ctx->HSGetShader(&hs, nullptr, nullptr);
        ctx->DSGetShader(&ds, nullptr, nullptr);
        ID3D11DeviceContext1* ctx1 = nullptr;
        if (SUCCEEDED(ctx->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&ctx1))) && ctx1) {
            ctx1->VSGetConstantBuffers1(0, kBuffers, vsCb, vsFirst, vsNum);
            ctx1->PSGetConstantBuffers1(0, kBuffers, psCb, psFirst, psNum);
            ctx1->Release();
        } else {
            ctx->VSGetConstantBuffers(0, kBuffers, vsCb);
            ctx->PSGetConstantBuffers(0, kBuffers, psCb);
        }
        ctx->PSGetShaderResources(0, kViews, srv);
        ctx->PSGetSamplers(0, kSamplers, samplers);
        ctx->OMGetBlendState(&blend, factor, &sampleMask);
        ctx->OMGetDepthStencilState(&depth, &ref);
        ctx->RSGetState(&rs);
        viewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        ctx->RSGetViewports(&viewports, viewport);
        scissors = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        ctx->RSGetScissorRects(&scissors, scissor);
        ctx->OMGetRenderTargets(1, &rtv, &dsv);
    }

    // A constant buffer bound back as it was: whole unless a part of it was (D3D11.1's ranges).
    static void SetBuffers(ID3D11DeviceContext* ctx, ID3D11DeviceContext1* ctx1, bool pixel, ID3D11Buffer* const* b, const UINT* first, const UINT* num) {
        for (UINT i = 0; i < kBuffers; ++i) {
            bool part = false;
            if (ctx1 && b[i]) {
                D3D11_BUFFER_DESC d;
                b[i]->GetDesc(&d);
                part = first[i] != 0 || (num[i] != 0 && num[i] * 16 < d.ByteWidth);
            }
            if (part)
                pixel ? ctx1->PSSetConstantBuffers1(i, 1, &b[i], &first[i], &num[i]) : ctx1->VSSetConstantBuffers1(i, 1, &b[i], &first[i], &num[i]);
            else
                pixel ? ctx->PSSetConstantBuffers(i, 1, &b[i]) : ctx->VSSetConstantBuffers(i, 1, &b[i]);
        }
    }

    void Apply(ID3D11DeviceContext* ctx) const {
        ctx->IASetInputLayout(layout);
        ctx->IASetPrimitiveTopology(topology);
        ctx->IASetVertexBuffers(0, kStreams, vb, stride, offset);
        ctx->IASetIndexBuffer(ib, ibFormat, ibOffset);
        ctx->VSSetShader(vs, nullptr, 0);
        ctx->PSSetShader(ps, nullptr, 0);
        ctx->GSSetShader(gs, nullptr, 0);
        ctx->HSSetShader(hs, nullptr, 0);
        ctx->DSSetShader(ds, nullptr, 0);
        ID3D11DeviceContext1* ctx1 = nullptr;
        if (FAILED(ctx->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&ctx1))))
            ctx1 = nullptr;
        SetBuffers(ctx, ctx1, false, vsCb, vsFirst, vsNum);
        SetBuffers(ctx, ctx1, true, psCb, psFirst, psNum);
        if (ctx1)
            ctx1->Release();
        ctx->PSSetShaderResources(0, kViews, srv);
        ctx->PSSetSamplers(0, kSamplers, samplers);
        ctx->OMSetBlendState(blend, factor, sampleMask);
        ctx->OMSetDepthStencilState(depth, ref);
        ctx->RSSetState(rs);
        ctx->RSSetViewports(viewports, viewports ? viewport : nullptr);
        ctx->RSSetScissorRects(scissors, scissors ? scissor : nullptr);
        ctx->OMSetRenderTargets(1, &rtv, dsv);
    }

    template <class T> static void Let(T*& p) {
        if (p)
            p->Release();
        p = nullptr;
    }
    void Release() {
        Let(layout);
        for (auto& b : vb)
            Let(b);
        Let(ib);
        Let(vs);
        Let(ps);
        Let(gs);
        Let(hs);
        Let(ds);
        for (auto& b : vsCb)
            Let(b);
        for (auto& b : psCb)
            Let(b);
        for (auto& v : srv)
            Let(v);
        for (auto& s : samplers)
            Let(s);
        Let(blend);
        Let(depth);
        Let(rs);
        Let(rtv);
        Let(dsv);
    }
};

// A held draw: its pieces, its buffers' contents as they were at that moment (copied on the card: the game fills its buffers again for the next draws,
// and nothing is read back), and how it was drawn.
struct HeldDraw {
    PipelineState state;
    bool indexed = false;
    UINT count = 0, start = 0;
    INT base = 0;
    ID3D11PixelShader* psKey = nullptr;  // (for the second view's like it: never drawn again then)
    uint64_t sum = 0;
};
constexpr int kMostHeld = 4;
HeldDraw g_held[kMostHeld];
int g_heldCount = 0;  // this frame's; under g_shadersMutex
// The copies' own buffers, kept from frame to frame: [held][piece] (4 streams, the index buffer, 4 vertex and 4 pixel constant buffers).
constexpr int kCopyPieces = PipelineState::kStreams + 1 + 2 * PipelineState::kBuffers;
ID3D11Buffer* g_heldCopies[kMostHeld][kCopyPieces] = {};

// The buffer's contents now into the slot's own copy, which takes its place in the held state; false if it can't be made.
bool CopyInto(ID3D11DeviceContext* ctx, ID3D11Buffer*& held, ID3D11Buffer*& own) {
    if (!held)
        return true;
    D3D11_BUFFER_DESC d;
    held->GetDesc(&d);
    if (d.MiscFlags != 0)
        return false;  // (a structured or indirect buffer: not a screen effect's)
    D3D11_BUFFER_DESC o{};
    if (own)
        own->GetDesc(&o);
    if (!own || o.ByteWidth != d.ByteWidth || o.BindFlags != d.BindFlags) {
        if (own)
            own->Release(), own = nullptr;
        D3D11_BUFFER_DESC n{};
        n.ByteWidth = d.ByteWidth;
        n.Usage = D3D11_USAGE_DEFAULT;
        n.BindFlags = d.BindFlags;
        ID3D11Device* device = nullptr;
        ctx->GetDevice(&device);
        if (device) {
            device->CreateBuffer(&n, nullptr, &own);
            device->Release();
        }
        if (!own)
            return false;
    }
    ctx->CopyResource(own, held);
    held->Release();
    held = own;
    own->AddRef();
    return true;
}

// Takes the draw as it is now for the right eye. False: it can't be held (it tests depth or stencil, has shaders past the pixel shader, or a buffer that
// can't be copied); the caller does as before.
bool Hold(ID3D11DeviceContext* ctx, bool indexed, UINT count, UINT start, INT base, uint64_t sum) {
    if (g_heldCount >= kMostHeld)
        return false;
    HeldDraw& h = g_held[g_heldCount];
    h.state.Release();
    h.state.Capture(ctx);
    bool ok = !h.state.gs && !h.state.hs && !h.state.ds;
    if (ok && h.state.depth && h.state.dsv) {
        D3D11_DEPTH_STENCIL_DESC d;
        h.state.depth->GetDesc(&d);
        ok = !d.DepthEnable && !d.StencilEnable;
    }
    ID3D11Buffer** own = g_heldCopies[g_heldCount];
    int piece = 0;
    for (int i = 0; ok && i < PipelineState::kStreams; ++i)
        ok = CopyInto(ctx, h.state.vb[i], own[piece++]);
    if (ok)
        ok = CopyInto(ctx, h.state.ib, own[piece++]);
    for (int i = 0; ok && i < PipelineState::kBuffers; ++i)
        ok = CopyInto(ctx, h.state.vsCb[i], own[piece++]) && CopyInto(ctx, h.state.psCb[i], own[piece++]);
    if (!ok) {
        h.state.Release();
        return false;
    }
    PipelineState::Let(h.state.dsv);  // drawn again with no depth view: it tests neither
    h.indexed = indexed, h.count = count, h.start = start, h.base = base;
    h.psKey = h.state.ps;
    h.sum = sum;
    ++g_heldCount;
    return true;
}

// The held draws again, a half to the right, onto the window as it is now (the right eye's picture just gone into its half): through the context as the
// game draws, the mod's hooks letting them pass.
void Reissue(ID3D11DeviceContext* ctx) {
    std::lock_guard lock(g_shadersMutex);
    if (!g_heldCount)
        return;
    PipelineState now;
    now.Capture(ctx);
    for (int i = 0; i < g_heldCount; ++i) {
        PipelineState placed = g_held[i].state;  // (no references of its own: applied, never released)
        placed.rtv = now.rtv;
        placed.dsv = nullptr;
        for (UINT v = 0; v < placed.viewports; ++v)
            placed.viewport[v].TopLeftX += placed.viewport[v].Height;  // an eye is as wide as the window is high
        for (UINT s = 0; s < placed.scissors; ++s) {
            const LONG half = static_cast<LONG>(placed.viewport[0].Height);
            placed.scissor[s].left += half;
            placed.scissor[s].right += half;
        }
        placed.Apply(ctx);
        t_reissuing = true;
        if (g_held[i].indexed)
            ctx->DrawIndexed(g_held[i].count, g_held[i].start, g_held[i].base);
        else
            ctx->Draw(g_held[i].count, g_held[i].start);
        t_reissuing = false;
        CensusAdd(g_held[i].sum, kHeldKind, g_held[i].count);
    }
    now.Apply(ctx);
    now.Release();
}

void ReleaseHeld() {
    for (int i = 0; i < g_heldCount; ++i)
        g_held[i].state.Release();
    g_heldCount = 0;
}

// A depth of field composite's first sight, by checksum, phase, viewport (eye or window) and, for the second view's, whether the first view drew its twin
// this frame: said once each.
std::atomic<uint32_t> g_dofSeen{0};
void NoteDof(uint64_t sum, int phase, bool window, bool routed, bool twin = true) {
    const uint32_t bit = 1u << ((sum == eyefx::kDofByDistance ? 0 : 8) + (phase == 1 ? 0 : 4) + (window ? 2 : 0) + (twin ? 0 : 1));
    if (g_dofSeen.fetch_or(bit, std::memory_order_relaxed) & bit)
        return;
    Log("Overlay: %s (%016llx) drawn by the %s view with %s viewport: %s", sum == eyefx::kDofByDistance ? "the depth of field by distance" : "the suppression blur's clear middle",
        static_cast<unsigned long long>(sum), phase == 1 ? "first" : "second", window ? "the window's" : "an eye's",
        !routed      ? (window ? "routed by its viewport (dofroute 0): as drawn" : "routed by its viewport (dofroute 0): as an eye effect")
        : phase == 1 ? "drawn into the left eye as the game draws it (its own)"
        : twin       ? "drawn into the right half only (its own)"
        : window     ? "the first view drew none this frame: into the right half, as before 2026-11-25"
                     : "the first view drew none this frame: into both eyes, as before 2026-11-25");
}

}

namespace overlay {

void FinishFrame(IDXGISwapChain* swap) {
    if (!g_keyTarget)
        return;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    g_keyTarget->GetDevice(&device);
    if (device)
        device->GetImmediateContext(&ctx);
    if (ctx)
        TakeOffOrange(ctx, swap);
    else
        Drop(g_keyTarget), Drop(g_keyTexture);
    Drop(ctx);
    Drop(device);
}

void SetKeepOrange(bool keep) {
    if (g_keepOrange.exchange(keep) != keep)
        Log("Overlay: the enemies' orange markers %s", keep ? "shown (orangemarkers 1)" : "taken off");
}

void SetKeepCrosshair(bool keep) {
    if (g_keepCrosshair.exchange(keep) != keep)
        Log("Overlay: the game's crosshair %s", keep ? "shown (crosshair 1)" : "taken off");
}

namespace {

// The first view's effects that went without a match this frame (and those held, unless the second view drew theirs after all): their streaks go on, all
// others' end. Under g_shadersMutex.
void UpdateStreaks() {
    uint64_t alone[16];
    int count = 0;
    const auto matched = [](uint64_t sum) { return std::find(g_matchedSums.begin(), g_matchedSums.end(), sum) != g_matchedSums.end(); };
    const auto add = [&](uint64_t sum) {
        if (!sum || matched(sum) || count >= 16 || std::find(alone, alone + count, sum) != alone + count)
            return;
        alone[count++] = sum;
    };
    for (const EyeEffect& e : g_eyeEffects)
        add(e.sum);
    for (int i = 0; i < g_heldCount; ++i)
        add(g_held[i].sum);
    for (Streak& s : g_streaks)
        if (std::find(alone, alone + count, s.sum) == alone + count)
            s.frames = 0;
    for (int i = 0; i < count; ++i) {
        auto it = std::find_if(g_streaks.begin(), g_streaks.end(), [&](const Streak& s) { return s.sum == alone[i]; });
        if (it != g_streaks.end())
            ++it->frames;
        else if (g_streaks.size() < 32)
            g_streaks.push_back({alone[i], 1});
    }
}

// Once a second: what the census found since, each row at most once a minute. Under g_shadersMutex.
void CensusLine(ULONGLONG now) {
    char line[1600];
    int used = 0;
    for (int kind = 0; kind < kCensusKinds; ++kind) {
        bool first = true;
        for (CensusRow& r : g_censusRows) {
            if (r.kind != kind || !r.count || (r.logged && now - r.logged < 60000))
                continue;
            if (used < static_cast<int>(sizeof(line)) - 320)  // (room for a kind's name and a row: snprintf never past the end)
                used += std::snprintf(line + used, sizeof(line) - used, "%s%s: PS %016llx (%u vertices) x%u", first ? (used ? "; " : "") : ", ",
                                      first ? kCensusNames[kind] : "", static_cast<unsigned long long>(r.sum), r.vertices, r.count);
            first = false;
            r.logged = now;
        }
    }
    for (CensusRow& r : g_censusRows)
        r.count = 0;
    if (used)
        Log("Overlay: screen effects in the last second: %s", line);
}

}  // namespace

void NewFrame() {
    g_eyePictures.store(0, std::memory_order_relaxed);
    g_hudIndex.store(0, std::memory_order_relaxed);
    static ULONGLONG lastLog = 0;
    const ULONGLONG now = GetTickCount64();
    const bool second = now - lastLog >= 1000;
    if (second) {
        lastLog = now;
        if (const uint32_t draws = g_hudDraws.exchange(0), copies = g_hudCopies.exchange(0); draws || copies)
            Log("Overlay: last second %u HUD draws in the left eye, %u copied into the right", draws, copies);
    }
    if (int asked = 1; !g_logFrame.compare_exchange_strong(asked, 2))
        g_logFrame.store(0);
    std::lock_guard lock(g_shadersMutex);
    for (const EyeEffect& e : g_eyeEffects)
        CensusAdd(e.sum, kAlone, e.count);
    for (const EyeEffect& e : g_ownPasses)  // (what the second view didn't match: a depth of field composite only the first view drew)
        if (eyefx::IsDofComposite(e.sum))
            CensusAdd(e.sum, kDofFirstAlone, e.count);
    UpdateStreaks();
    if (second)
        CensusLine(now);
    g_eyeEffects.clear();
    g_ownPasses.clear();
    g_matchedSums.clear();
    ReleaseHeld();
}

int EyePicturesSoFar() {
    return g_eyePictures.load(std::memory_order_relaxed);
}

void SetDofRoute(bool on) {
    if (g_dofRoute.exchange(on) != on)
        Log("Overlay: the depth of field's composites %s", on ? "each drawn over its own view's eye, whatever their viewport (dofroute 1)"
                                                              : "routed by their viewport alone, as before 2026-11-25 (dofroute 0)");
}

void SetCensus(bool on) {
    if (g_census.exchange(on) != on)
        Log("Overlay: the screen effects' census %s", on ? "on (fxcensus 1)" : "off (fxcensus 0)");
}

void SetReaders(bool on) {
    if (g_readers.exchange(on) != on)
        Log("Overlay: an effect with an eye's viewport reading its view's picture %s", on ? "drawn as that view's own pass: the left eye's between the pictures, the right half only after (fxreaders 1)"
                                                                                        : "routed as before (fxreaders 0)");
}

void SetHold(bool on) {
    if (g_hold.exchange(on) != on)
        Log("Overlay: an effect only the first view draws %s", on ? "held for the right eye after three frames without a match (fxhold 1)" : "left out, as before (fxhold 0)");
}

void SetHudDepth(float metres) {
    const float kept = std::isfinite(metres) && metres > 0.0f ? std::clamp(metres, eyefx::kNearestHud, eyefx::kFarthestHud) : 0.0f;
    if (g_hudDepth.exchange(kept) != kept)
        Log("Overlay: the HUD %s", kept > 0.0f ? "given a depth (huddepth): its place said with its first draw" : "at infinity, the same pixels in both eyes (huddepth 0)");
}

void SetHudScale(float k) {
    const float kept = eyefx::HudScale(k);
    if (g_hudScale.exchange(kept) != kept)
        Log("Overlay: the HUD drawn %.2f times its size about each eye's middle (hudscale)", kept);
}

void LogNextFrame() {
    g_logFrame = 1;
}

void SetHudFix(int mode) {
    if (g_hudFix.exchange(mode) != mode)
        Log("Overlay: HUD experiment %d", mode);
}

void SetHudSkipTextures(const uint32_t (&sizes)[4]) {
    bool changed = false;
    for (int i = 0; i < 4; ++i)
        changed |= g_hudSkipTex[i].exchange(sizes[i]) != sizes[i];
    if (changed)
        Log("Overlay: HUD draws with textures %ux%u %ux%u left out (hudskiptex)", sizes[0] >> 16, sizes[0] & 0xFFFF,
            sizes[1] >> 16, sizes[1] & 0xFFFF);
}

void SetHudSkip(const uint64_t (&bits)[4]) {
    bool changed = false;
    for (int i = 0; i < kSkippable / 64; ++i)
        changed |= g_hudSkip[i].exchange(bits[i]) != bits[i];
    if (changed)
        Log("Overlay: HUD draws left out: %s", bits[0] | bits[1] | bits[2] | bits[3] ? "some (hudskip)" : "none");
}

bool Draw(ID3D11DeviceContext* ctx, UINT count, const std::function<void()>& draw, bool indexed, UINT start,
          INT base) {
    if (t_reissuing)
        return false;  // a held effect drawn again for the right eye (Reissue): as it is
    // Only with the eyes side by side, the right eye's view placed at the window's left edge like
    // the left eye's (views.cpp), and only small draws at that edge: an eye's frame there, or the
    // whole window's.
    if (count > kMostVertices || !views::RightEyeAtLeft() || scopeview::Drawing())
        return false;
    D3D11_VIEWPORT viewport;
    UINT viewports = 1;
    ctx->RSGetViewports(&viewports, &viewport);
    if (viewports != 1 || viewport.TopLeftX != 0.0f || viewport.TopLeftY != 0.0f || viewport.Height < 1000.0f)
        return false;
    const bool window = viewport.Width == 2.0f * viewport.Height, eye = viewport.Width == viewport.Height;
    if (!window && !eye)
        return false;
    if (!Blended(ctx)) {
        // An eye's picture onto the window (unblended, over the whole window): the left eye's comes
        // out where it belongs; the right eye's comes out at the left edge too and goes into the
        // right half. (Placed at the right by the game, its screen effects worked from that place
        // against pictures only an eye wide: while suppressed, the whole right eye was smeared into
        // streaks, the user's right eye blinded: their video, 2026-10-30.)
        if (!window)
            return false;
        const int picture = g_eyePictures.fetch_add(1, std::memory_order_relaxed);
        if (picture < 2)
            gputime::Stamp(picture == 0 ? gputime::kLeftEye : gputime::kRightEye, ctx);
        if (picture < 2)
            camera::NoteEyePicture(picture);  // (which of the frame's main-view uploads came before an eye's picture went out: what the picture is named by, camera.h)
        if (picture != 1)
            return false;
        const bool done = IntoRightHalf(ctx, draw);
        if (done)
            Reissue(ctx);  // "fxhold": what only the first view drew, into the right eye too, over its picture
        return done;
    }
    const int pictures = g_eyePictures.load(std::memory_order_relaxed);
    if (pictures != 1 && pictures != 2)
        return false;
    // Between the pictures the second view's own scene is drawn too: only what goes onto the window is looked at further.
    if (pictures == 1 && !OnWindow(ctx))
        return false;
    const UINT eyeSize = static_cast<UINT>(viewport.Height);
    ID3D11PixelShader* const ps = PixelShader(ctx);
    const uint64_t sum = ps ? shaders::ChecksumOf(ps) : 0;
    const bool dofRoute = g_dofRoute.load(std::memory_order_relaxed);
    // A view's own depth of field composite (eyefx.h), by what it is: the first view's drawn into the left eye between the pictures, the second view's into
    // the right half only, whatever viewport it comes with ("dofroute 0": by the viewport alone, as before). Onto the window only.
    const bool dof = eyefx::IsDofComposite(sum) && (pictures == 1 || OnWindow(ctx));
    if (pictures == 1) {
        if (dof)
            NoteDof(sum, 1, window, dofRoute);
        if (dof && dofRoute) {
            std::lock_guard lock(g_shadersMutex);
            g_ownPasses.push_back({ps, count, sum});
            return false;  // as the game draws it: the left eye's own
        }
        // Between the eyes' pictures: the first view's own effects onto the window (its damage
        // effect pass). The game keeps the player's damage for the second view, the right eye's
        // (the screen's blood came in its pass, not the first's): the first's is left out, the
        // second's goes into both eyes. Its shaders are noted to draw the second's alike.
        if (eye) {
            // "fxreaders": one that reads its view's picture is that view's own, as the depth of field is.
            const bool reads = (g_readers.load(std::memory_order_relaxed) || g_hold.load(std::memory_order_relaxed)) && ReadsScene(ctx, eyeSize);
            if (reads && g_readers.load(std::memory_order_relaxed)) {
                std::lock_guard lock(g_shadersMutex);
                g_ownPasses.push_back({ps, count, sum});
                CensusAdd(sum, kOwnPass, count);
                return false;  // as the game draws it: the left eye's own
            }
            std::lock_guard lock(g_shadersMutex);
            // "fxhold": one the second view hasn't matched three frames running, drawn into the left eye as it is and held for the right (not one that reads
            // its view's picture: the right half would get the left eye's).
            if (g_hold.load(std::memory_order_relaxed) && ps && sum && !reads && StreakOf(sum) >= kHoldAfter &&
                Hold(ctx, indexed, count, start, base, sum))
                return false;
            if (ps)
                g_eyeEffects.push_back({ps, count, sum});
            return true;
        }
        return false;
    }
    // After the right eye's picture, all onto the window, the player's damage effects and the HUD,
    // into both eyes: as the game draws it (at the left, with the right eye's view placed there),
    // and for the right eye, a copy a half to the right, or for what reads the scene by where on the
    // window it is (the damage effect pass, a part of the HUD reading an eye's scene), into the
    // right half as it comes out at the left.
    // A depth of field composite of the second view's is the right eye's own (into the right half only) when the first view drew its twin into the left eye
    // this frame (g_ownPasses, by checksum, one for one); drawn by the second view alone it goes as before 2026-11-25, by its viewport (an eye's into both
    // eyes, the window's into the right half), so no eye goes without what the views drew (the review of 2e323f2: the right eye's only, the left eye's none).
    bool ownPass = false, eyeEffect = false, heldTwin = false;
    if (dof && dofRoute) {
        std::lock_guard lock(g_shadersMutex);
        for (auto it = g_ownPasses.begin(); it != g_ownPasses.end(); ++it)
            if (it->sum == sum) {
                ownPass = true;
                g_ownPasses.erase(it);
                break;
            }
        if (!ownPass)
            CensusAdd(sum, kDofSecondAlone, count);
    }
    if (dof)
        NoteDof(sum, 2, window, dofRoute, ownPass || !dofRoute);
    if (eye && ps) {
        std::lock_guard lock(g_shadersMutex);
        if (!ownPass && g_readers.load(std::memory_order_relaxed))
            for (auto it = g_ownPasses.begin(); it != g_ownPasses.end(); ++it)
                if (it->ps == ps && it->count == count) {
                    ownPass = true;
                    g_ownPasses.erase(it);
                    break;
                }
        if (!ownPass)
            for (int i = 0; i < g_heldCount && !heldTwin; ++i)
                if (g_held[i].psKey == ps && g_held[i].count == count) {
                    heldTwin = true;  // the second view drew it after all: the held one serves both eyes this frame, and none is held from the next
                    g_matchedSums.push_back(g_held[i].sum);
                }
        if (!ownPass && !heldTwin)
            for (auto it = g_eyeEffects.begin(); it != g_eyeEffects.end(); ++it)
                if (it->ps == ps && it->count == count) {
                    eyeEffect = true;
                    g_matchedSums.push_back(it->sum);
                    g_eyeEffects.erase(it);
                    break;
                }
    }
    const int index = g_hudIndex.fetch_add(1, std::memory_order_relaxed);
    if (heldTwin)
        return true;
    if (index < kSkippable && (g_hudSkip[index / 64].load(std::memory_order_relaxed) >> (index % 64) & 1)) {
        if (g_logFrame.load(std::memory_order_relaxed) == 2)
            Log("Overlay: HUD draw %d (%u) left out (hudskip)", index, count);
        return true;
    }
    if (g_hudSkipTex[0].load(std::memory_order_relaxed)) {
        const uint32_t size = FirstTextureSize(ctx);
        for (const auto& skip : g_hudSkipTex)
            if (size && skip.load(std::memory_order_relaxed) == size)
                return true;
    }
    // The game's own crosshair: four ticks round the middle of the view, each a 6-index quad on its
    // 128x64 compressed sheet (found 2026-10-31 in the headset's own pictures while the user was shot
    // at: faint against the sky, so hard to see in the mirror; the user's "crosshair when I take
    // damage"). In 30 frames nothing else of the HUD drew with a texture that size and kind.
    if (count == 6 && !g_keepCrosshair.load(std::memory_order_relaxed)) {
        DXGI_FORMAT format;
        if (FirstTextureSize(ctx, &format) == (128u << 16 | 64u) && format == DXGI_FORMAT_BC3_UNORM)
            return true;
    }
    const bool reads = ReadsScene(ctx, eyeSize);
    // The second view's own pass over its whole picture: the depth of field, the suppression blur (a
    // window-wide quad reading its eye's blurred scene, PS checksum 2b2ca40a51aa1b4d). The first view drew its
    // own over the left eye between the pictures (the blur study: with all of this phase left out the left eye
    // is blurred, fine detail 0.02-0.12 against the sharp right eye's 1.7-33). Drawn at the left too, under fire
    // the left eye showed the right eye's blurred picture (correlation 0.98-1.00 at no shift: no depth): only
    // into the right half. (By what it is too: ownPass.)
    const bool viewPass = (window && reads && !eyeEffect) || ownPass;
    // The census: something reading an eye's picture with an eye's viewport after the right eye's picture, that is neither a matched effect nor a view's own
    // pass (a part of the HUD, or the second view's effect the first view had none of): drawn at the left and into the right half, as before. (A depth of
    // field composite drawn by the second view alone has its own row.)
    if (eye && reads && !eyeEffect && !ownPass && !dof) {
        std::lock_guard lock(g_shadersMutex);
        CensusAdd(sum, kEyeReader, count);
    }
    // The picture before the HUD, once a frame (the orange markers), after the blur and the damage passes:
    // taken before them, the blur's spreading of orange sparks and warm light counted as the HUD's orange
    // and the sharp picture went back round it, square patches in the blur (the user, 17:20).
    if (!eyeEffect && !viewPass) {
        gputime::Stamp(gputime::kHudStart, ctx);
        TakeBefore(ctx);
    }
    // The HUD's place ("huddepth", "hudscale"): its own draws only, never the eye effects (a screen-locked tint reads as a visor), a view's own passes, nor
    // a depth of field composite drawn by the second view alone (as before, unmoved).
    const bool hud = !eyeEffect && !viewPass && !dof;
    const HudPlace place = hud ? HudPlaceFor(viewport.Height) : HudPlace{};
    const float eyePixels = viewport.Height;
    // "hudfix": 1 the left eye's copy without the depth-stencil view, 2 no left copy, 3 no right copy.
    const int fix = g_hudFix.load(std::memory_order_relaxed);
    if (fix == 1)
        DrawWithoutDepth(ctx, draw);
    else if (fix != 2 && !viewPass) {
        // The left eye's: moved right by its share, kept out of the right eye's half.
        if (!place.Any() || !DrawPlaced(ctx, draw, viewport, eyePixels, place.k, static_cast<float>(place.shift), 0, static_cast<LONG>(eyePixels)))
            draw();
    }
    const bool intoHalf = fix != 3 && (eyeEffect || reads || ownPass) && IntoRightHalf(ctx, draw, hud ? &place : nullptr, &viewport);
    if (!intoHalf && fix != 3) {
        // The right eye's: a half to the right less its share, kept out of the left eye's half.
        if (!place.Any() || !DrawPlaced(ctx, draw, viewport, eyePixels, place.k, eyePixels - static_cast<float>(place.shift), static_cast<LONG>(eyePixels),
                                        static_cast<LONG>(2.0f * eyePixels)))
            DrawMovedRight(ctx, draw, viewport);
    }
    if (fix != 2)
        ++g_hudDraws;
    if (fix != 3)
        ++g_hudCopies;
    if (g_logFrame.load(std::memory_order_relaxed) == 2) {
        Log("Overlay: HUD draw %d: %u, %s viewport, %s%s, right eye's copy %s; PS %p (%016llx)%s", index, count,
            window ? "window" : "eye", ownPass ? "a view's own pass" : eyeEffect ? "the eye effect" : "HUD", reads ? " reading the scene" : "",
            intoHalf ? "drawn into its half" : "moved right", static_cast<void*>(ps), static_cast<unsigned long long>(sum),
            place.Any() ? " (placed: each eye's share of the HUD's depth and its scale)" : "");
        LogHudConstants(ctx, index);
        if (indexed && count <= 12)
            LogHudVertices(ctx, index, count, start, base);
    }
    if (g_logged.fetch_add(1, std::memory_order_relaxed) == 0)
        Log("Overlay: the right eye's view is drawn as the left eye's and moved into its half; the HUD in both eyes");
    return true;
}

}

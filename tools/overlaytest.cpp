// The overlay's routing (src/overlay.cpp, compiled into this test with the game's side stubbed: the views, the scope's picture, the camera's naming, the
// card's timing, the shaders' checksums, the log) on a software graphics card (WARP), without the game: a made-up frame of a 2000 x 1000 window (each eye
// 1000 square), drawn through overlay::Draw as the draw hooks do (a draw it doesn't take is drawn by the test, as the hook draws it), read back each frame.
//   o0  the defaults as built, nothing set: each eye its own depth of field, the HUD the same in both eyes (at infinity), the rest as before
//   o1  as before (huddepth 0, dofroute 0): the eyes' pictures in their halves; the first view's damage effect left out, the second's in both eyes; a HUD
//       piece at the same place in both; the suppression blur's composite (the window's viewport) each eye its own view's
//   o2  the depth of field by distance with an eye's viewport: with dofroute 0 (as before 2026-11-25) the left eye got the right eye's; with dofroute 1
//       each eye its own; the clear middle as before either way
//   o3  the HUD at a depth (shift 20 px): the left eye's copy moved right, the right eye's left; nothing spilled across the middle from either edge
//   o4  the HUD scaled about each eye's middle (hudscale 0.5), with and without a depth
//   o5  a HUD piece reading an eye's picture: into the right half, placed there too
//   o6  the census: an effect the first view drew alone said, by its checksum; a frame with everything matched says nothing new
//   o7  fxreaders: an effect reading its view's picture with an eye's viewport: the left eye's own between the pictures, the right half only after
//   o8  fxhold: an effect only the first view draws: left out for three frames, then in both eyes, its constants as they were at its draw (the buffer
//       filled again since); its twin from the second view ends the hold
//   o9  nothing taken with one view, or while the scope's own picture is drawn
//   o10 the time
// Usage: overlaytest   (the exit code is the number of failed checks)

#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "../src/gputime.h"
#include "../src/overlay.h"
#include "../src/scopeview.h"
#include "../src/shaders.h"
#include "../src/views.h"

// ---- The game's side, stubbed ----
namespace test {
bool g_rightAtLeft = true;
float g_offsets[2] = {-0.032f, 0.032f};
float g_fov = 0.0f;  // set in main: 1250 px per unit tangent on a 1000 px eye
std::map<ID3D11PixelShader*, uint64_t> g_sums;
std::vector<std::string> g_log;
}  // namespace test

namespace views {
bool RightEyeAtLeft() { return test::g_rightAtLeft; }
float EyeOffset(int eye) { return eye == 0 || eye == 1 ? test::g_offsets[eye] : 0.0f; }
float RenderedFov() { return test::g_fov; }
}  // namespace views
namespace scopeview {
std::atomic<bool> g_drawing{false};
}
namespace camera {
void NoteEyePicture(int) {}
}
namespace gputime {
void Stamp(Mark, ID3D11DeviceContext*) {}
}
namespace shaders {
uint64_t ChecksumOf(ID3D11PixelShader* ps) {
    const auto it = test::g_sums.find(ps);
    return it == test::g_sums.end() ? 0 : it->second;
}
}  // namespace shaders
namespace logx {
void Open(HMODULE) {}
void Write(const char* fmt, ...) {
    char line[2048];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);
    test::g_log.push_back(line);
}
}  // namespace logx

namespace {

int g_failed = 0, g_counted = 0, g_skipped = 0;

void Report(const char* id, const char* what, bool pass, const char* format, ...) {
    char numbers[1024];
    va_list args;
    va_start(args, format);
    std::vsnprintf(numbers, sizeof(numbers), format, args);
    va_end(args);
    std::printf("%-4s %-5s %s | %s\n", id, pass ? "PASS" : "FAIL", what, numbers);
    ++g_counted;
    g_failed += !pass;
}

void Skip(const char* id, const char* what) {
    std::printf("%-4s SKIP  %s\n", id, what);
    ++g_skipped;
}

constexpr UINT kEye = 1000, kWidth = 2 * kEye;
// The HUD's share at 2 m with 64 mm between the eyes and 1250 px a unit of tangent: 64e-3 x 1250 / 2 = 40 px, 20 each eye.
constexpr int kShift = 20;

struct Color {
    float r, g, b;
};
const Color kLeftPicture{0.2f, 0.0f, 0.0f}, kRightPicture{0.0f, 0.2f, 0.0f}, kBlack{0.0f, 0.0f, 0.0f};
const Color kHud{0.0f, 0.0f, 1.0f}, kEdgeRight{1.0f, 1.0f, 1.0f}, kEdgeLeft{0.0f, 0.5f, 1.0f};
const Color kClear0{0.5f, 0.5f, 0.0f}, kClear1{0.0f, 0.5f, 0.5f};      // the clear middle, the first view's and the second's
const Color kDistance0{0.4f, 0.4f, 0.4f}, kDistance1{0.8f, 0.8f, 0.8f};  // the depth of field by distance
const Color kTint0{1.0f, 0.0f, 1.0f}, kTint1{0.5f, 0.0f, 1.0f};          // the damage tint
const Color kAlone0{0.0f, 1.0f, 0.5f}, kAlone1{0.0f, 0.3f, 1.0f};        // an effect only the first view draws (and its later twin)
const Color kReader0{0.3f, 0.3f, 1.0f}, kReader1{0.6f, 0.6f, 1.0f};      // an effect reading its view's picture
const Color kHudReader{0.9f, 0.9f, 1.0f};                                // a HUD piece reading an eye's picture

struct Gpu {
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    ID3D11Texture2D* window = nullptr;
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11Texture2D* staging = nullptr;
    ID3D11VertexShader* vs = nullptr;
    ID3D11Buffer* constants = nullptr;
    ID3D11BlendState* blended = nullptr;
    ID3D11Texture2D* eyePicture = nullptr;  // half an eye: what the game's blur and its readers sample
    ID3D11ShaderResourceView* eyeView = nullptr;
    std::vector<uint8_t> pixels;  // the last frame read back, RGBA
};

constexpr char kShaders[] = R"(
cbuffer C : register(b0) { float4 rect; float4 color; };
void VS(uint id : SV_VertexID, out float4 pos : SV_Position) {
    uint i = id % 6;
    pos = float4(i == 1 || i == 4 || i == 5 ? rect.z : rect.x, i == 0 || i == 1 || i == 4 ? rect.w : rect.y, 0, 1);
}
float4 PS(float4 pos : SV_Position) : SV_Target { return float4(color.rgb + TAG * 1e-9, 1); }
)";

ID3D11PixelShader* MakePs(Gpu& g, int tag, uint64_t sum) {
    char define[16];
    std::snprintf(define, sizeof(define), "%d", tag);
    const D3D_SHADER_MACRO macros[] = {{"TAG", define}, {nullptr, nullptr}};
    ID3DBlob* code = nullptr;
    ID3DBlob* errors = nullptr;
    ID3D11PixelShader* ps = nullptr;
    if (SUCCEEDED(D3DCompile(kShaders, sizeof(kShaders) - 1, "test", macros, nullptr, "PS", "ps_4_0", 0, 0, &code, &errors)))
        g.device->CreatePixelShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &ps);
    if (errors)
        std::printf("shader: %s\n", static_cast<const char*>(errors->GetBufferPointer())), errors->Release();
    if (code)
        code->Release();
    if (ps)
        test::g_sums[ps] = sum;
    return ps;
}

bool Setup(Gpu& g) {
    if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &g.device, nullptr, &g.ctx)))
        return false;
    D3D11_TEXTURE2D_DESC d{};
    d.Width = kWidth;
    d.Height = kEye;
    d.MipLevels = 1;
    d.ArraySize = 1;
    d.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    d.SampleDesc.Count = 1;
    d.Usage = D3D11_USAGE_DEFAULT;
    d.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(g.device->CreateTexture2D(&d, nullptr, &g.window)) || FAILED(g.device->CreateRenderTargetView(g.window, nullptr, &g.rtv)))
        return false;
    D3D11_TEXTURE2D_DESC s = d;
    s.Usage = D3D11_USAGE_STAGING;
    s.BindFlags = 0;
    s.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    if (FAILED(g.device->CreateTexture2D(&s, nullptr, &g.staging)))
        return false;
    D3D11_TEXTURE2D_DESC e = d;
    e.Width = e.Height = kEye / 2;
    e.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(g.device->CreateTexture2D(&e, nullptr, &g.eyePicture)) || FAILED(g.device->CreateShaderResourceView(g.eyePicture, nullptr, &g.eyeView)))
        return false;
    ID3DBlob* code = nullptr;
    const D3D_SHADER_MACRO macros[] = {{"TAG", "0"}, {nullptr, nullptr}};
    if (FAILED(D3DCompile(kShaders, sizeof(kShaders) - 1, "test", macros, nullptr, "VS", "vs_4_0", 0, 0, &code, nullptr)) ||
        FAILED(g.device->CreateVertexShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &g.vs)))
        return false;
    code->Release();
    D3D11_BUFFER_DESC b{};
    b.ByteWidth = 32;
    b.Usage = D3D11_USAGE_DYNAMIC;
    b.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    b.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(g.device->CreateBuffer(&b, nullptr, &g.constants)))
        return false;
    D3D11_BLEND_DESC bd{};
    bd.RenderTarget[0].BlendEnable = TRUE;  // blended as far as the overlay can tell, the colour simply replaced
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    return SUCCEEDED(g.device->CreateBlendState(&bd, &g.blended));
}

enum class Vp { Window, Eye };

// One draw as the game would make it: its viewport at the window's left edge (the window's or an eye's), its rectangle in that viewport's own -1..1, blended or
// not, reading an eye's picture or not; through the overlay first, drawn by the test if the overlay doesn't take it. True when the overlay took it.
bool Quad(Gpu& g, ID3D11PixelShader* ps, Vp vp, float x0, float x1, float y0, float y1, const Color& c, bool blended, bool reads, UINT count) {
    ID3D11DeviceContext* ctx = g.ctx;
    const D3D11_VIEWPORT v{0.0f, 0.0f, vp == Vp::Window ? float(kWidth) : float(kEye), float(kEye), 0.0f, 1.0f};
    ctx->RSSetViewports(1, &v);
    ctx->RSSetState(nullptr);
    ctx->OMSetBlendState(blended ? g.blended : nullptr, nullptr, 0xFFFFFFFF);
    ctx->OMSetDepthStencilState(nullptr, 0);
    D3D11_MAPPED_SUBRESOURCE m{};
    if (SUCCEEDED(ctx->Map(g.constants, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) {
        const float data[8] = {x0, y0, x1, y1, c.r, c.g, c.b, 1.0f};
        std::memcpy(m.pData, data, sizeof(data));
        ctx->Unmap(g.constants, 0);
    }
    ctx->VSSetShader(g.vs, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &g.constants);
    ctx->PSSetShader(ps, nullptr, 0);
    ctx->PSSetConstantBuffers(0, 1, &g.constants);
    ID3D11ShaderResourceView* view = reads ? g.eyeView : nullptr;
    ctx->PSSetShaderResources(0, 1, &view);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->OMSetRenderTargets(1, &g.rtv, nullptr);
    const auto draw = [&] { ctx->Draw(count, 0); };
    const bool took = overlay::Draw(ctx, count, draw);
    if (!took)
        draw();
    return took;
}

// Eye pixels (x across an eye, y down) to the viewport's -1..1 for an eye's viewport.
float Nx(float px) { return px / kEye * 2.0f - 1.0f; }
float Ny(float py) { return 1.0f - py / kEye * 2.0f; }

struct Shaders {
    ID3D11PixelShader *picture, *hud, *edgeR, *edgeL, *clear, *distance, *tint, *alone, *reader, *hudReader;
};

// What a frame draws (each a switch): the pieces laid out apart from each other on the eye's square.
struct Frame {
    bool clear = true, distance = true, tint = true, alone = false, aloneTwin = false, reader = false, hudReader = false, hud = true, edges = true;
};

void ReadBack(Gpu& g) {
    g.ctx->CopyResource(g.staging, g.window);
    D3D11_MAPPED_SUBRESOURCE m{};
    g.pixels.assign(kWidth * kEye * 4, 0);
    if (SUCCEEDED(g.ctx->Map(g.staging, 0, D3D11_MAP_READ, 0, &m))) {
        for (UINT y = 0; y < kEye; ++y)
            std::memcpy(&g.pixels[y * kWidth * 4], static_cast<const uint8_t*>(m.pData) + y * m.RowPitch, kWidth * 4);
        g.ctx->Unmap(g.staging, 0);
    }
}

void RunFrame(Gpu& g, const Shaders& s, const Frame& f, int* taken = nullptr) {
    int took = 0;
    const float black[4] = {0, 0, 0, 1};
    g.ctx->ClearRenderTargetView(g.rtv, black);
    // The left eye's picture onto the window (unblended, the window's viewport, its geometry over the left half), then its own effects.
    took += Quad(g, s.picture, Vp::Window, -1.0f, 0.0f, -1.0f, 1.0f, kLeftPicture, false, false, 6);
    if (f.clear)  // the suppression blur's clear middle: the window's viewport, a band across the left half, reading the eye's blurred picture
        took += Quad(g, s.clear, Vp::Window, -1.0f, 0.0f, Ny(80), Ny(20), kClear0, true, true, 6);
    if (f.distance)  // the depth of field by distance, an eye's viewport (never seen in the game: the case dofroute is for)
        took += Quad(g, s.distance, Vp::Eye, Nx(600), Nx(700), Ny(750), Ny(650), kDistance0, true, true, 6);
    if (f.tint)  // the damage tint: no picture read, an eye's viewport
        took += Quad(g, s.tint, Vp::Eye, Nx(400), Nx(500), Ny(550), Ny(450), kTint0, true, false, 24);
    if (f.alone)
        took += Quad(g, s.alone, Vp::Eye, Nx(800), Nx(900), Ny(400), Ny(300), kAlone0, true, false, 24);
    if (f.reader)
        took += Quad(g, s.reader, Vp::Eye, Nx(100), Nx(200), Ny(350), Ny(250), kReader0, true, true, 24);
    // The right eye's picture, drawn by the game at the left edge too.
    took += Quad(g, s.picture, Vp::Window, -1.0f, 0.0f, -1.0f, 1.0f, kRightPicture, false, false, 6);
    // Its own effects, then the HUD.
    if (f.clear)
        took += Quad(g, s.clear, Vp::Window, -1.0f, 0.0f, Ny(80), Ny(20), kClear1, true, true, 6);
    if (f.distance)
        took += Quad(g, s.distance, Vp::Eye, Nx(600), Nx(700), Ny(750), Ny(650), kDistance1, true, true, 6);
    if (f.tint)
        took += Quad(g, s.tint, Vp::Eye, Nx(400), Nx(500), Ny(550), Ny(450), kTint1, true, false, 24);
    if (f.aloneTwin)
        took += Quad(g, s.alone, Vp::Eye, Nx(800), Nx(900), Ny(400), Ny(300), kAlone1, true, false, 24);
    if (f.reader)
        took += Quad(g, s.reader, Vp::Eye, Nx(100), Nx(200), Ny(350), Ny(250), kReader1, true, true, 24);
    if (f.hud)
        took += Quad(g, s.hud, Vp::Eye, Nx(250), Nx(350), Ny(600), Ny(400), kHud, true, false, 6);
    if (f.edges) {
        took += Quad(g, s.edgeR, Vp::Eye, Nx(950), Nx(1000), Ny(200), Ny(100), kEdgeRight, true, false, 6);
        took += Quad(g, s.edgeL, Vp::Eye, Nx(0), Nx(50), Ny(900), Ny(800), kEdgeLeft, true, false, 6);
    }
    if (f.hudReader)
        took += Quad(g, s.hudReader, Vp::Eye, Nx(600), Nx(700), Ny(950), Ny(850), kHudReader, true, true, 6);
    // Present.
    overlay::FinishFrame(nullptr);
    overlay::NewFrame();
    ReadBack(g);
    if (taken)
        *taken = took;
}

bool Is(const Gpu& g, int x, int y, const Color& c) {
    const uint8_t* p = &g.pixels[(static_cast<size_t>(y) * kWidth + x) * 4];
    return std::abs(p[0] - int(c.r * 255.0f + 0.5f)) <= 2 && std::abs(p[1] - int(c.g * 255.0f + 0.5f)) <= 2 && std::abs(p[2] - int(c.b * 255.0f + 0.5f)) <= 2;
}

std::string At(const Gpu& g, int x, int y) {
    const uint8_t* p = &g.pixels[(static_cast<size_t>(y) * kWidth + x) * 4];
    char s[48];
    std::snprintf(s, sizeof(s), "(%d,%d): %d %d %d", x, y, p[0], p[1], p[2]);
    return s;
}

// A list of pixel expectations, all of which must hold; the first that doesn't is said.
struct Expect {
    int x, y;
    Color c;
    const char* what;
};
bool All(const Gpu& g, std::initializer_list<Expect> list, std::string& wrong) {
    for (const Expect& e : list)
        if (!Is(g, e.x, e.y, e.c)) {
            wrong = std::string(e.what) + " " + At(g, e.x, e.y);
            return false;
        }
    wrong = "all as expected";
    return true;
}

// The switches as each check wants them (the mod's own defaults but the HUD at 2 m: "huddepth 0" is the default, checked in o1).
void Defaults() {
    overlay::SetDofRoute(true);
    overlay::SetCensus(true);
    overlay::SetReaders(false);
    overlay::SetHold(false);
    overlay::SetHudDepth(2.0f);
    overlay::SetHudScale(1.0f);
    test::g_rightAtLeft = true;
    scopeview::g_drawing = false;
}

}  // namespace

int main() {
    const auto start = std::chrono::steady_clock::now();
    SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
    test::g_fov = 2.0f * std::atan(500.0f / 1250.0f);
    Gpu g;
    if (!Setup(g)) {
        Skip("o*", "no software graphics card (WARP) here");
        std::printf("%d checks, %d failed, %d skipped\n", g_counted, g_failed, g_skipped);
        return 0;
    }
    Shaders s{};
    s.picture = MakePs(g, 1, 0x4a68a7d8c656f62full);  // a tone map's
    s.hud = MakePs(g, 2, 0x3e75fa4c00000001ull);
    s.edgeR = MakePs(g, 3, 0x3e75fa4c00000002ull);
    s.edgeL = MakePs(g, 4, 0x3e75fa4c00000003ull);
    s.clear = MakePs(g, 5, 0x2b2ca40a51aa1b4dull);
    s.distance = MakePs(g, 6, 0x94a5f6c1e17c947dull);
    s.tint = MakePs(g, 7, 0x5553fe3fbb6528bbull);
    s.alone = MakePs(g, 8, 0x0bf13c2500000001ull);
    s.reader = MakePs(g, 9, 0x353d151100000001ull);
    s.hudReader = MakePs(g, 10, 0x2372326c00000001ull);
    overlay::NewFrame();
    std::string wrong;

    // o0: the mod's own defaults, nothing set: each eye its own depth of field, the HUD at infinity (the same pixels in both eyes), the census on.
    RunFrame(g, s, Frame{});
    Report("o0", "the defaults as built: each eye its own depth of field (dofroute 1); the HUD at the same place in both eyes (huddepth 0); the rest as before",
           All(g, {{650, 700, kDistance0, "left depth of field: the first view's"}, {1650, 700, kDistance1, "right: the second's"}, {300, 500, kHud, "left HUD"},
                   {245, 500, kLeftPicture, "left of the left HUD"}, {1300, 500, kHud, "right HUD"}, {1245, 500, kRightPicture, "left of the right HUD"},
                   {450, 500, kTint1, "left tint"}, {1450, 500, kTint1, "right tint"}, {500, 50, kClear0, "left clear middle"}, {1500, 50, kClear1, "right clear middle"}},
               wrong),
           "%s", wrong.c_str());

    // o1: as before 2026-11-25.
    Defaults();
    overlay::SetHudDepth(0.0f);
    overlay::SetDofRoute(false);
    RunFrame(g, s, Frame{});
    Report("o1", "as before (huddepth 0, dofroute 0): pictures in their halves; the first view's tint left out, the second's in both; the HUD alike in both; the clear middle each its own",
           All(g, {{100, 500, kLeftPicture, "left picture"}, {1100, 500, kRightPicture, "right picture"}, {450, 500, kTint1, "left tint (the second view's)"},
                   {1450, 500, kTint1, "right tint"}, {300, 500, kHud, "left HUD"}, {1300, 500, kHud, "right HUD"}, {245, 500, kLeftPicture, "left of the left HUD"},
                   {1245, 500, kRightPicture, "left of the right HUD"}, {500, 50, kClear0, "left clear middle (its own)"}, {1500, 50, kClear1, "right clear middle (its own)"},
                   {975, 150, kEdgeRight, "right-edge piece, left"}, {1010, 150, kRightPicture, "nothing over the middle"}, {990, 850, kLeftPicture, "nothing over the middle"},
                   {1025, 850, kEdgeLeft, "left-edge piece, right"}},
               wrong),
           "%s", wrong.c_str());
    // o2: the depth of field by distance with an eye's viewport.
    Report("o2a", "as before (dofroute 0): the depth of field by distance with an eye's viewport put the RIGHT eye's into the left eye (the case found)",
           All(g, {{650, 700, kDistance1, "left eye: the second view's"}, {1650, 700, kDistance1, "right eye"}}, wrong), "%s", wrong.c_str());
    Defaults();
    overlay::SetHudDepth(0.0f);
    RunFrame(g, s, Frame{});
    Report("o2b", "dofroute 1: each eye its own depth of field (left the first view's, right the second's); the clear middle and the rest as before",
           All(g, {{650, 700, kDistance0, "left eye: the first view's"}, {1650, 700, kDistance1, "right eye: the second's"}, {500, 50, kClear0, "left clear middle"},
                   {1500, 50, kClear1, "right clear middle"}, {450, 500, kTint1, "left tint"}, {1450, 500, kTint1, "right tint"}, {300, 500, kHud, "left HUD"},
                   {1300, 500, kHud, "right HUD"}},
               wrong),
           "%s", wrong.c_str());
    {
        int said = 0;
        for (const std::string& l : test::g_log)
            said += l.find("the depth of field by distance (94a5f6c1e17c947d) drawn by the") != std::string::npos;
        Report("o2c", "each composite's first sight said once (its view and its viewport)", said >= 2, "%d lines", said);
    }
    // o3: the HUD at a depth.
    Defaults();
    RunFrame(g, s, Frame{});
    Report("o3a", "huddepth 2 (40 px at 2 m here): the left eye's HUD 20 px right, the right eye's 20 px left",
           All(g, {{265, 500, kLeftPicture, "left of the left HUD"}, {275, 500, kHud, "left HUD's left edge"}, {365, 500, kHud, "left HUD's right edge"},
                   {372, 500, kLeftPicture, "right of the left HUD"}, {1225, 500, kRightPicture, "left of the right HUD"}, {1235, 500, kHud, "right HUD's left edge"},
                   {1325, 500, kHud, "right HUD's right edge"}, {1335, 500, kRightPicture, "right of the right HUD"}},
               wrong),
           "%s", wrong.c_str());
    Report("o3b", "nothing spilled across the middle: the right-edge piece's left copy stops at the left eye's edge, the left-edge piece's right copy starts at the right eye's",
           All(g, {{985, 150, kEdgeRight, "right-edge piece, left eye"}, {1005, 150, kRightPicture, "the right eye's first pixels"},
                   {1975, 150, kEdgeRight, "right-edge piece, right eye"}, {1990, 150, kRightPicture, "past it"}, {10, 850, kLeftPicture, "the left eye's first pixels"},
                   {30, 850, kEdgeLeft, "left-edge piece, left eye"}, {995, 850, kLeftPicture, "the left eye's last pixels"}, {1010, 850, kEdgeLeft, "left-edge piece, right eye"},
                   {1040, 850, kRightPicture, "past it"}},
               wrong),
           "%s", wrong.c_str());
    Report("o3c", "the eye effects and the views' own passes not moved by the HUD's depth",
           All(g, {{405, 500, kTint1, "left tint's edge"}, {1405, 500, kTint1, "right tint's edge"}, {5, 50, kClear0, "left clear middle's edge"},
                   {1005, 50, kClear1, "right clear middle's edge"}, {605, 700, kDistance0, "left depth of field's edge"}, {1605, 700, kDistance1, "right's"}},
               wrong),
           "%s", wrong.c_str());
    {
        int said = 0;
        for (const std::string& l : test::g_log)
            said += l.find("the HUD at 2.00 m: each eye's copy moved 20 px") != std::string::npos;
        Report("o3d", "the HUD's place said", said >= 1, "%d lines", said);
    }
    // o4: scaled (the tint left out: the scaled HUD comes over its place).
    Defaults();
    overlay::SetHudDepth(0.0f);
    overlay::SetHudScale(0.5f);
    Frame noTint;
    noTint.tint = false;
    RunFrame(g, s, noTint);
    Report("o4a", "hudscale 0.5: the HUD half its size about each eye's middle (250..350 -> 375..425, rows 400..600 -> 450..550)",
           All(g, {{370, 500, kLeftPicture, "left of it"}, {380, 500, kHud, "its left"}, {420, 500, kHud, "its right"}, {430, 500, kLeftPicture, "right of it"},
                   {400, 445, kLeftPicture, "above it"}, {400, 455, kHud, "its top"}, {1380, 500, kHud, "right eye's"}, {1430, 500, kRightPicture, "right of the right eye's"}},
               wrong),
           "%s", wrong.c_str());
    overlay::SetHudDepth(2.0f);
    RunFrame(g, s, noTint);
    Report("o4b", "hudscale 0.5 at 2 m: scaled, then moved by each eye's share",
           All(g, {{390, 500, kLeftPicture, "left of it"}, {400, 500, kHud, "its left (395)"}, {440, 500, kHud, "its right (445)"}, {450, 500, kLeftPicture, "right of it"},
                   {1360, 500, kHud, "right eye's left (1355)"}, {1400, 500, kHud, "right eye's right (1405)"}, {1410, 500, kRightPicture, "right of it"}},
               wrong),
           "%s", wrong.c_str());
    // o5: a HUD piece reading an eye's picture.
    Defaults();
    Frame withReader;
    withReader.hudReader = true;
    RunFrame(g, s, withReader);
    Report("o5", "a HUD piece reading an eye's picture: drawn at the left and into the right half, each moved by its share",
           All(g, {{615, 900, kLeftPicture, "left of the left one"}, {625, 900, kHudReader, "left one (620..720)"}, {715, 900, kHudReader, "its right"},
                   {725, 900, kLeftPicture, "past it"}, {1575, 900, kRightPicture, "left of the right one"}, {1585, 900, kHudReader, "right one (1580..1680)"},
                   {1675, 900, kHudReader, "its right"}, {1685, 900, kRightPicture, "past it"}},
               wrong),
           "%s", wrong.c_str());
    // o6: the census (said at most once a second: each frame here after a second's wait).
    Defaults();
    test::g_log.clear();
    Frame aloneFrame;
    aloneFrame.alone = true;
    Sleep(1100);
    RunFrame(g, s, aloneFrame);
    {
        bool said = false;
        for (const std::string& l : test::g_log)
            said |= l.find("screen effects in the last second") != std::string::npos && l.find("no match") != std::string::npos &&
                    l.find("0bf13c2500000001") != std::string::npos;
        Report("o6a", "an effect the first view drew alone: left out as before (nowhere), and said by its checksum",
               said && Is(g, 850, 350, kLeftPicture) && Is(g, 1850, 350, kRightPicture), "%s; %s; %s", said ? "said" : "NOT said", At(g, 850, 350).c_str(),
               At(g, 1850, 350).c_str());
    }
    test::g_log.clear();
    Sleep(1100);  // (a second's line)
    RunFrame(g, s, Frame{});
    {
        bool said = false;
        for (const std::string& l : test::g_log)
            said |= l.find("screen effects in the last second") != std::string::npos;
        Report("o6b", "a frame with every effect matched: nothing new said", !said, "%s", said ? "a line" : "no line");
    }
    // o7: fxreaders.
    Defaults();
    Frame readerFrame;
    readerFrame.reader = true;
    RunFrame(g, s, readerFrame);
    Report("o7a", "fxreaders 0: an eye effect reading its view's picture as before: the second view's in both eyes",
           All(g, {{150, 300, kReader1, "left eye: the second view's"}, {1150, 300, kReader1, "right eye"}}, wrong), "%s", wrong.c_str());
    overlay::SetReaders(true);
    RunFrame(g, s, readerFrame);
    Report("o7b", "fxreaders 1: each eye its own view's (the first view's between the pictures, the second's into the right half only)",
           All(g, {{150, 300, kReader0, "left eye: the first view's"}, {1150, 300, kReader1, "right eye: the second's"}}, wrong), "%s", wrong.c_str());
    // o8: fxhold.
    Defaults();
    overlay::SetHold(true);
    std::string seen;
    for (int frame = 1; frame <= 3; ++frame) {
        RunFrame(g, s, aloneFrame);
        seen += Is(g, 850, 350, kAlone0) ? "L" : "-";
        seen += Is(g, 1850, 350, kAlone0) ? "R " : "- ";
    }
    RunFrame(g, s, aloneFrame);
    const bool held = Is(g, 850, 350, kAlone0) && Is(g, 1850, 350, kAlone0) && Is(g, 795, 350, kLeftPicture) && Is(g, 1795, 350, kRightPicture) &&
                      Is(g, 905, 350, kLeftPicture) && Is(g, 1905, 350, kRightPicture);
    Report("o8a", "fxhold 1: left out three frames (the census's streak), then in both eyes at its own place, its constants as at its draw (the one buffer filled again since)",
           seen == "-- -- -- " && held, "frames 1-3: %s; frame 4: %s %s", seen.c_str(), At(g, 850, 350).c_str(), At(g, 1850, 350).c_str());
    Frame twinFrame = aloneFrame;
    twinFrame.aloneTwin = true;
    Sleep(1100);  // (the census's line with the held one comes with this frame)
    RunFrame(g, s, twinFrame);
    const bool twinSkipped = Is(g, 850, 350, kAlone0) && Is(g, 1850, 350, kAlone0);
    const std::string twinSeen = At(g, 850, 350) + " " + At(g, 1850, 350);
    RunFrame(g, s, twinFrame);
    const bool matchedAgain = Is(g, 850, 350, kAlone1) && Is(g, 1850, 350, kAlone1);
    Report("o8b", "its twin from the second view: that frame the held one serves both eyes; the next, matched as any eye effect (the hold ended)",
           twinSkipped && matchedAgain, "the twin's frame %s; the next %s %s", twinSeen.c_str(), At(g, 850, 350).c_str(), At(g, 1850, 350).c_str());
    {
        bool said = false;
        for (const std::string& l : test::g_log)
            said |= l.find("held for the right eye (fxhold)") != std::string::npos;
        Report("o8c", "the held effect said by the census", said, "%s", said ? "said" : "not said");
    }
    // o9: nothing taken with one view, or in the scope's picture.
    Defaults();
    test::g_rightAtLeft = false;
    int took = -1;
    RunFrame(g, s, Frame{}, &took);
    const int oneView = took;
    test::g_rightAtLeft = true;
    scopeview::g_drawing = true;
    RunFrame(g, s, Frame{}, &took);
    const int scope = took;
    Defaults();
    Report("o9", "nothing taken with one view or while the scope's own picture is drawn", oneView == 0 && scope == 0, "taken: %d with one view, %d in the scope's",
           oneView, scope);
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    Report("o10", "the time", ms < 60000.0, "%.0f ms", ms);
    std::printf("%d checks, %d failed, %d skipped\n", g_counted, g_failed, g_skipped);
    return g_failed;
}

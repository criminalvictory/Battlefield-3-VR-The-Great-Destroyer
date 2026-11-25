#include "skyrays.h"

#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>

#include <atomic>
#include <mutex>
#include <vector>

#include <cmath>

#include "eyefx.h"
#include "log.h"
#include "overlay.h"
#include "scopeview.h"
#include "shaders.h"
#include "views.h"

namespace {

std::atomic<int> g_mode{0};
std::mutex g_mutex;
ID3D11VertexShader* g_vs = nullptr;  // the mod's (eyefx.h kSkyRaysVs), made once
ID3D11Buffer* g_eyeBuffer = nullptr;  // its b1: the eye's angle and window move, written at each sky draw (16 bytes, written only)
bool g_failed = false;

// The game's rasterizer states with nothing culled (the mod's corners turn one way, whatever the game's state culls), each made once and the game's held.
struct NoCull {
    ID3D11RasterizerState* from;
    ID3D11RasterizerState* clone;
};
std::vector<NoCull> g_noCull;

// What was refused, said once each.
enum Refusal { kDepthTest, kStages, kNoConstants, kNoShader, kNoState, kNoEye, kRefusals };
const char* const kRefusalNames[kRefusals] = {"its depth test is on (the mod's corners have a depth of their own)",
                                              "shaders past the pixel shader are bound", "no constant buffer for its pixel shader",
                                              "the mod's vertex shader couldn't be made", "no rasterizer state without culling could be made",
                                              "not an eye's scene (no eye's angle, or drawn after both eyes' pictures)"};
std::atomic<uint32_t> g_refusedSaid{0};
std::atomic<bool> g_drawnSaid{false};

void Refuse(Refusal why) {
    const uint32_t bit = 1u << why;
    if (!(g_refusedSaid.fetch_or(bit, std::memory_order_relaxed) & bit))
        Log("Sky: the game's own rays kept for a sky draw: %s", kRefusalNames[why]);
}

ID3D11VertexShader* Shader(ID3D11Device* device) {
    if (g_vs || g_failed)
        return g_vs;
    D3D11_BUFFER_DESC b{};
    b.ByteWidth = 16;
    b.Usage = D3D11_USAGE_DEFAULT;
    b.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(device->CreateBuffer(&b, nullptr, &g_eyeBuffer)) || !g_eyeBuffer) {
        g_failed = true;
        return nullptr;
    }
    ID3DBlob* code = nullptr;
    ID3DBlob* errors = nullptr;
    const bool ok = SUCCEEDED(D3DCompile(eyefx::kSkyRaysVs, sizeof(eyefx::kSkyRaysVs) - 1, "skyrays", nullptr, nullptr, "VS", "vs_4_0", 0, 0, &code, &errors)) &&
                    SUCCEEDED(device->CreateVertexShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &g_vs));
    if (errors) {
        Log("Sky: the rays' vertex shader: %s", static_cast<const char*>(errors->GetBufferPointer()));
        errors->Release();
    }
    if (code)
        code->Release();
    if (!ok) {
        g_failed = true;
        g_vs = nullptr;
    }
    return g_vs;
}

ID3D11RasterizerState* NoCullClone(ID3D11Device* device, ID3D11RasterizerState* from) {
    for (const NoCull& n : g_noCull)
        if (n.from == from)
            return n.clone;
    if (g_noCull.size() >= 32)
        return nullptr;
    D3D11_RASTERIZER_DESC d{};
    if (from) {
        from->GetDesc(&d);
    } else {
        d.FillMode = D3D11_FILL_SOLID;
        d.DepthClipEnable = TRUE;
    }
    d.CullMode = D3D11_CULL_NONE;
    ID3D11RasterizerState* clone = nullptr;
    if (FAILED(device->CreateRasterizerState(&d, &clone)) || !clone)
        return nullptr;
    if (from)
        from->AddRef();
    g_noCull.push_back({from, clone});
    return clone;
}

bool OffCentreNow() {
    float right = 0.0f, up = 0.0f;
    return views::OffCentre(0, right, up) || views::OffCentre(1, right, up);
}

}  // namespace

namespace skyrays {

void SetMode(int mode) {
    mode = mode < 0 ? 0 : (mode > 2 ? 2 : mode);
    if (g_mode.exchange(mode) != mode)
        Log("Sky: %s", mode == 0   ? "the game's own view rays (skyrays 0)"
                       : mode == 1 ? "the view rays made from each eye's own camera while the pictures are off-centre (skyrays 1)"
                                   : "the view rays made from each eye's own camera always (skyrays 2: a test)");
}

bool Draw(ID3D11DeviceContext* ctx, UINT count, const std::function<void(UINT, UINT)>& draw) {
    const int mode = g_mode.load(std::memory_order_relaxed);
    if (mode <= 0 || count != 6 || scopeview::Drawing() || (mode == 1 && !OffCentreNow()))
        return false;
    ID3D11PixelShader* ps = nullptr;
    ctx->PSGetShader(&ps, nullptr, nullptr);
    if (!ps)
        return false;
    ps->Release();  // (only compared)
    if (!shaders::IsSky(ps))
        return false;
    // Which eye's scene this is (its picture not yet on the window), its angle and its window's move (views.cpp wrote them into its cameras).
    const int eye = overlay::EyePicturesSoFar();
    const float fov = views::RenderedFov();
    if ((eye != 0 && eye != 1) || !(fov > 0.2f && fov < 3.0f)) {
        Refuse(kNoEye);
        return false;
    }
    float eyeNumbers[4] = {std::tan(0.5f * fov), 0.0f, 0.0f, 0.0f};
    views::OffCentre(eye, eyeNumbers[1], eyeNumbers[2]);  // (false: centred, 0 0)
    {
        ID3D11DepthStencilState* depth = nullptr;
        UINT ref = 0;
        ctx->OMGetDepthStencilState(&depth, &ref);
        bool tested = false;
        if (depth) {
            D3D11_DEPTH_STENCIL_DESC d;
            depth->GetDesc(&d);
            tested = d.DepthEnable && d.DepthFunc != D3D11_COMPARISON_ALWAYS;
            depth->Release();
        }
        if (tested) {
            Refuse(kDepthTest);
            return false;
        }
    }
    {
        ID3D11GeometryShader* gs = nullptr;
        ID3D11HullShader* hs = nullptr;
        ID3D11DomainShader* ds = nullptr;
        ctx->GSGetShader(&gs, nullptr, nullptr);
        ctx->HSGetShader(&hs, nullptr, nullptr);
        ctx->DSGetShader(&ds, nullptr, nullptr);
        const bool stages = gs || hs || ds;
        if (gs)
            gs->Release();
        if (hs)
            hs->Release();
        if (ds)
            ds->Release();
        if (stages) {
            Refuse(kStages);
            return false;
        }
    }
    // The pixel shader's constants as bound (a range of a bigger buffer, D3D11.1, or the whole).
    ID3D11DeviceContext1* ctx1 = nullptr;
    if (FAILED(ctx->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&ctx1))))
        ctx1 = nullptr;
    ID3D11Buffer* constants = nullptr;
    UINT first = 0, num = 0;
    if (ctx1)
        ctx1->PSGetConstantBuffers1(0, 1, &constants, &first, &num);
    else
        ctx->PSGetConstantBuffers(0, 1, &constants);
    if (!constants) {
        if (ctx1)
            ctx1->Release();
        Refuse(kNoConstants);
        return false;
    }
    bool part = false;
    {
        D3D11_BUFFER_DESC d;
        constants->GetDesc(&d);
        part = ctx1 && (first != 0 || (num != 0 && num * 16 < d.ByteWidth));
    }
    ID3D11Device* device = nullptr;
    ctx->GetDevice(&device);
    ID3D11VertexShader* ours = nullptr;
    ID3D11RasterizerState* noCull = nullptr;
    ID3D11RasterizerState* rs = nullptr;
    ctx->RSGetState(&rs);
    if (device) {
        std::lock_guard lock(g_mutex);
        ours = Shader(device);
        noCull = ours ? NoCullClone(device, rs) : nullptr;
        device->Release();
    }
    if (!ours || !noCull) {
        Refuse(!ours ? kNoShader : kNoState);
        if (rs)
            rs->Release();
        constants->Release();
        if (ctx1)
            ctx1->Release();
        return false;
    }
    // What this changes, put back after.
    ID3D11VertexShader* vs = nullptr;
    ctx->VSGetShader(&vs, nullptr, nullptr);
    ID3D11Buffer* vsBuffer = nullptr;
    UINT vsFirst = 0, vsNum = 0;
    if (ctx1)
        ctx1->VSGetConstantBuffers1(0, 1, &vsBuffer, &vsFirst, &vsNum);
    else
        ctx->VSGetConstantBuffers(0, 1, &vsBuffer);
    bool vsPart = false;
    if (vsBuffer && ctx1) {
        D3D11_BUFFER_DESC d;
        vsBuffer->GetDesc(&d);
        vsPart = vsFirst != 0 || (vsNum != 0 && vsNum * 16 < d.ByteWidth);
    }
    ID3D11Buffer* vsSecond = nullptr;
    UINT secondFirst = 0, secondNum = 0;
    if (ctx1)
        ctx1->VSGetConstantBuffers1(1, 1, &vsSecond, &secondFirst, &secondNum);
    else
        ctx->VSGetConstantBuffers(1, 1, &vsSecond);
    bool secondPart = false;
    if (vsSecond && ctx1) {
        D3D11_BUFFER_DESC d;
        vsSecond->GetDesc(&d);
        secondPart = secondFirst != 0 || (secondNum != 0 && secondNum * 16 < d.ByteWidth);
    }
    ID3D11InputLayout* layout = nullptr;
    ctx->IAGetInputLayout(&layout);
    D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ctx->IAGetPrimitiveTopology(&topology);

    ctx->UpdateSubresource(g_eyeBuffer, 0, nullptr, eyeNumbers, 0, 0);
    ctx->VSSetShader(ours, nullptr, 0);
    if (part)
        ctx1->VSSetConstantBuffers1(0, 1, &constants, &first, &num);
    else
        ctx->VSSetConstantBuffers(0, 1, &constants);
    ctx->VSSetConstantBuffers(1, 1, &g_eyeBuffer);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->RSSetState(noCull);
    draw(6, 0);
    ctx->RSSetState(rs);
    ctx->IASetPrimitiveTopology(topology);
    ctx->IASetInputLayout(layout);
    if (secondPart)
        ctx1->VSSetConstantBuffers1(1, 1, &vsSecond, &secondFirst, &secondNum);
    else
        ctx->VSSetConstantBuffers(1, 1, &vsSecond);
    if (vsPart)
        ctx1->VSSetConstantBuffers1(0, 1, &vsBuffer, &vsFirst, &vsNum);
    else
        ctx->VSSetConstantBuffers(0, 1, &vsBuffer);
    ctx->VSSetShader(vs, nullptr, 0);

    if (layout)
        layout->Release();
    if (vsSecond)
        vsSecond->Release();
    if (vsBuffer)
        vsBuffer->Release();
    if (vs)
        vs->Release();
    if (rs)
        rs->Release();
    constants->Release();
    if (ctx1)
        ctx1->Release();
    if (!g_drawnSaid.exchange(true, std::memory_order_relaxed))
        Log("Sky: its view rays made by the mod from each eye's own camera (the draw's own constants: skyrays %d)", mode);
    return true;
}

}  // namespace skyrays

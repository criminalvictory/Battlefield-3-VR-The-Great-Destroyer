#include "edgefill.h"

#include <windows.h>
#include <d3dcompiler.h>

#include <cstring>
#include <vector>

#include "log.h"

namespace {

// Each pixel outside the picture reads the picture's nearest edge, from a mip that blurs it more the further out it is.
const char kShader[] = R"(
cbuffer Fill : register(b0) {
    float4 pic;    // the picture in the target: x, y, width, height (pixels)
    float4 src;    // and in the copy: u, v, width, height
    float4 shape;  // x: the blur's scale (pixels out); y: pixels out to the darkest; z: the darkest; w: 1 for a target that takes linear values
};
Texture2D picture : register(t0);
SamplerState linearClamp : register(s0);

void VS(uint id : SV_VertexID, out float4 pos : SV_Position) {
    float2 uv = float2((id << 1) & 2, id & 2);
    pos = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
}

float4 PS(float4 pos : SV_Position) : SV_Target {
    float2 p = (pos.xy - pic.xy) / pic.zw;
    float2 c = saturate(p);
    float out_ = length((p - c) * pic.zw);
    // Blurred quickly, within a few dozen pixels: read at one mip all along, the picture's edge came out as long hard streaks.
    float3 color = picture.SampleLevel(linearClamp, src.xy + c * src.zw, min(1.5 * log2(1.0 + out_ / shape.x), 6.0)).rgb;
    color *= lerp(1.0, shape.z, smoothstep(0.0, shape.y, out_));
    if (shape.w > 0.5)
        color = pow(color, 2.2);
    return float4(color, 1.0);
}
)";

struct Params {
    float pic[4];
    float src[4];
    float shape[4];
};

struct TargetView {
    ID3D11Texture2D* texture = nullptr;
    ID3D11RenderTargetView* view = nullptr;
    bool linear = false;
};

struct Resources {
    bool tried = false, ok = false;
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11Buffer* params = nullptr;
    ID3D11SamplerState* sampler = nullptr;
    ID3D11RasterizerState* raster = nullptr;
    ID3D11BlendState* blend = nullptr;
    ID3D11DepthStencilState* noDepth = nullptr;
    ID3D11Texture2D* copy = nullptr;
    ID3D11ShaderResourceView* copyView = nullptr;
    UINT copyWidth = 0, copyHeight = 0;
    DXGI_FORMAT copyFormat = DXGI_FORMAT_UNKNOWN;
    std::vector<TargetView> targets;
} g;

template <class T>
void SafeRelease(T*& p) {
    if (p)
        p->Release();
    p = nullptr;
}

ID3DBlob* Compile(const char* entry, const char* profile) {
    ID3DBlob* code = nullptr;
    ID3DBlob* errors = nullptr;
    if (FAILED(D3DCompile(kShader, sizeof(kShader) - 1, "edgefill", nullptr, nullptr, entry, profile, D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &code, &errors))) {
        Log("Edge fill: shader %s failed: %s", entry, errors ? static_cast<const char*>(errors->GetBufferPointer()) : "no message");
        SafeRelease(code);
    }
    SafeRelease(errors);
    return code;
}

bool CreateFixed(ID3D11Device* device) {
    if (g.tried)
        return g.ok;
    g.tried = true;
    ID3DBlob* vs = Compile("VS", "vs_5_0");
    ID3DBlob* ps = Compile("PS", "ps_5_0");
    bool ok = vs && ps && SUCCEEDED(device->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &g.vs)) &&
              SUCCEEDED(device->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &g.ps));
    SafeRelease(vs);
    SafeRelease(ps);
    D3D11_BUFFER_DESC cb{sizeof(Params), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0};
    D3D11_SAMPLER_DESC sd{};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    D3D11_RASTERIZER_DESC rd{};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.DepthClipEnable = TRUE;
    D3D11_BLEND_DESC bd{};
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    D3D11_DEPTH_STENCIL_DESC dd{};
    dd.DepthEnable = FALSE;
    dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    ok = ok && SUCCEEDED(device->CreateBuffer(&cb, nullptr, &g.params)) && SUCCEEDED(device->CreateSamplerState(&sd, &g.sampler)) &&
         SUCCEEDED(device->CreateRasterizerState(&rd, &g.raster)) && SUCCEEDED(device->CreateBlendState(&bd, &g.blend)) &&
         SUCCEEDED(device->CreateDepthStencilState(&dd, &g.noDepth));
    g.ok = ok;
    if (!ok)
        Log("Edge fill: its shaders could not be made; black round the picture");
    return ok;
}

// The picture's copy with its mips, to read the blurred edge from.
bool PrepareCopy(ID3D11Device* device, const D3D11_TEXTURE2D_DESC& back) {
    if (g.copy && g.copyWidth == back.Width && g.copyHeight == back.Height && g.copyFormat == back.Format)
        return true;
    SafeRelease(g.copyView);
    SafeRelease(g.copy);
    D3D11_TEXTURE2D_DESC td{};
    td.Width = back.Width;
    td.Height = back.Height;
    td.MipLevels = 7;
    td.ArraySize = 1;
    td.Format = back.Format;
    td.SampleDesc = {1, 0};
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    td.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
    if (FAILED(device->CreateTexture2D(&td, nullptr, &g.copy)) || FAILED(device->CreateShaderResourceView(g.copy, nullptr, &g.copyView))) {
        Log("Edge fill: no copy of the picture (format %d)", back.Format);
        SafeRelease(g.copyView);
        SafeRelease(g.copy);
        return false;
    }
    g.copyWidth = back.Width;
    g.copyHeight = back.Height;
    g.copyFormat = back.Format;
    return true;
}

// A view to draw into a target with: the picture's numbers as they are (a typeless swapchain image viewed as plain
// UNORM); a target typed sRGB takes linear values.
TargetView* ViewOf(ID3D11Device* device, ID3D11Texture2D* texture) {
    for (TargetView& t : g.targets)
        if (t.texture == texture)
            return &t;
    D3D11_TEXTURE2D_DESC td;
    texture->GetDesc(&td);
    D3D11_RENDER_TARGET_VIEW_DESC vd{};
    vd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    vd.Format = td.Format;
    bool linear = false;
    switch (td.Format) {
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        vd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        vd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        break;
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        linear = true;
        break;
    default:
        break;
    }
    ID3D11RenderTargetView* view = nullptr;
    if (FAILED(device->CreateRenderTargetView(texture, &vd, &view))) {
        Log("Edge fill: no view of the target (format %d)", td.Format);
        return nullptr;
    }
    Log("Edge fill: drawing into %ux%u, format %d, as %d%s", td.Width, td.Height, td.Format, vd.Format, linear ? " (linear values)" : "");
    texture->AddRef();
    g.targets.push_back({texture, view, linear});
    return &g.targets.back();
}

// The pipeline state the pass changes, put back afterwards.
struct SavedState {
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11DepthStencilView* dsv = nullptr;
    D3D11_VIEWPORT viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    UINT viewportCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    ID3D11VertexShader* vs = nullptr;
    ID3D11HullShader* hs = nullptr;
    ID3D11DomainShader* ds = nullptr;
    ID3D11GeometryShader* gs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11InputLayout* layout = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ID3D11ShaderResourceView* srv = nullptr;
    ID3D11SamplerState* sampler = nullptr;
    ID3D11Buffer* cb = nullptr;
    ID3D11BlendState* blend = nullptr;
    FLOAT blendFactor[4] = {};
    UINT sampleMask = 0;
    ID3D11DepthStencilState* depth = nullptr;
    UINT stencilRef = 0;
    ID3D11RasterizerState* raster = nullptr;

    void Save(ID3D11DeviceContext* c) {
        c->OMGetRenderTargets(1, &rtv, &dsv);
        c->RSGetViewports(&viewportCount, viewports);
        c->VSGetShader(&vs, nullptr, nullptr);
        c->HSGetShader(&hs, nullptr, nullptr);
        c->DSGetShader(&ds, nullptr, nullptr);
        c->GSGetShader(&gs, nullptr, nullptr);
        c->PSGetShader(&ps, nullptr, nullptr);
        c->IAGetInputLayout(&layout);
        c->IAGetPrimitiveTopology(&topology);
        c->PSGetShaderResources(0, 1, &srv);
        c->PSGetSamplers(0, 1, &sampler);
        c->PSGetConstantBuffers(0, 1, &cb);
        c->OMGetBlendState(&blend, blendFactor, &sampleMask);
        c->OMGetDepthStencilState(&depth, &stencilRef);
        c->RSGetState(&raster);
    }
    void Restore(ID3D11DeviceContext* c) {
        c->OMSetRenderTargets(1, &rtv, dsv);
        c->RSSetViewports(viewportCount, viewports);
        c->VSSetShader(vs, nullptr, 0);
        c->HSSetShader(hs, nullptr, 0);
        c->DSSetShader(ds, nullptr, 0);
        c->GSSetShader(gs, nullptr, 0);
        c->PSSetShader(ps, nullptr, 0);
        c->IASetInputLayout(layout);
        c->IASetPrimitiveTopology(topology);
        c->PSSetShaderResources(0, 1, &srv);
        c->PSSetSamplers(0, 1, &sampler);
        c->PSSetConstantBuffers(0, 1, &cb);
        c->OMSetBlendState(blend, blendFactor, sampleMask);
        c->OMSetDepthStencilState(depth, stencilRef);
        c->RSSetState(raster);
        SafeRelease(rtv);
        SafeRelease(dsv);
        SafeRelease(vs);
        SafeRelease(hs);
        SafeRelease(ds);
        SafeRelease(gs);
        SafeRelease(ps);
        SafeRelease(layout);
        SafeRelease(srv);
        SafeRelease(sampler);
        SafeRelease(cb);
        SafeRelease(blend);
        SafeRelease(depth);
        SafeRelease(raster);
    }
};

}  // namespace

namespace edgefill {

bool Draw(ID3D11Device* device, ID3D11Texture2D* back, const D3D11_TEXTURE2D_DESC& backDesc, ID3D11Texture2D* target, const Layout& layout) {
    if (backDesc.SampleDesc.Count > 1 || !CreateFixed(device) || !PrepareCopy(device, backDesc))
        return false;
    TargetView* view = ViewOf(device, target);
    if (!view)
        return false;
    ID3D11DeviceContext* c = nullptr;
    device->GetImmediateContext(&c);
    // The picture, for reading from, with its mips; then in the target as it is.
    c->CopySubresourceRegion(g.copy, 0, 0, 0, 0, back, 0, nullptr);
    c->GenerateMips(g.copyView);
    for (int e = 0; e < 2; ++e) {
        const D3D11_BOX box{static_cast<UINT>(e * layout.pictureWidth), 0, 0, static_cast<UINT>((e + 1) * layout.pictureWidth), static_cast<UINT>(layout.pictureHeight), 1};
        c->CopySubresourceRegion(target, 0, e * layout.eyeWidth + layout.pictureX[e], layout.pictureY[e], 0, back, 0, &box);
    }
    SavedState saved;
    saved.Save(c);
    c->OMSetRenderTargets(1, &view->view, nullptr);
    c->VSSetShader(g.vs, nullptr, 0);
    c->HSSetShader(nullptr, nullptr, 0);
    c->DSSetShader(nullptr, nullptr, 0);
    c->GSSetShader(nullptr, nullptr, 0);
    c->PSSetShader(g.ps, nullptr, 0);
    c->IASetInputLayout(nullptr);
    c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    c->PSSetShaderResources(0, 1, &g.copyView);
    c->PSSetSamplers(0, 1, &g.sampler);
    c->PSSetConstantBuffers(0, 1, &g.params);
    const FLOAT factor[4] = {};
    c->OMSetBlendState(g.blend, factor, 0xFFFFFFFF);
    c->OMSetDepthStencilState(g.noDepth, 0);
    c->RSSetState(g.raster);
    for (int e = 0; e < 2; ++e) {
        const float ex = static_cast<float>(e * layout.eyeWidth), ew = static_cast<float>(layout.eyeWidth), eh = static_cast<float>(layout.eyeHeight);
        const float px = ex + layout.pictureX[e], py = static_cast<float>(layout.pictureY[e]);
        const float pw = static_cast<float>(layout.pictureWidth), ph = static_cast<float>(layout.pictureHeight);
        Params p{};
        p.pic[0] = px, p.pic[1] = py, p.pic[2] = pw, p.pic[3] = ph;
        p.src[0] = e * 0.5f, p.src[1] = 0.0f, p.src[2] = 0.5f, p.src[3] = 1.0f;
        p.shape[0] = 4.0f, p.shape[1] = 700.0f, p.shape[2] = 0.55f, p.shape[3] = view->linear ? 1.0f : 0.0f;
        c->UpdateSubresource(g.params, 0, nullptr, &p, 0, 0);
        // The four strips round the picture.
        const D3D11_VIEWPORT strips[4] = {
            {ex, 0.0f, px - ex, eh, 0.0f, 1.0f},                    // left of it, top to bottom
            {px + pw, 0.0f, ex + ew - px - pw, eh, 0.0f, 1.0f},     // right of it
            {px, 0.0f, pw, py, 0.0f, 1.0f},                         // above
            {px, py + ph, pw, eh - py - ph, 0.0f, 1.0f},            // below
        };
        for (const D3D11_VIEWPORT& strip : strips) {
            if (strip.Width < 1.0f || strip.Height < 1.0f)
                continue;
            c->RSSetViewports(1, &strip);
            c->Draw(3, 0);
        }
    }
    ID3D11ShaderResourceView* none = nullptr;
    c->PSSetShaderResources(0, 1, &none);
    saved.Restore(c);
    c->Release();
    return true;
}

void Reset() {
    for (TargetView& t : g.targets) {
        SafeRelease(t.view);
        SafeRelease(t.texture);
    }
    g.targets.clear();
    SafeRelease(g.copyView);
    SafeRelease(g.copy);
}

}  // namespace edgefill

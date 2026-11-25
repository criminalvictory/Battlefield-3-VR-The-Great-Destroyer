// Hosting a lying gun's draws on the graphics card (gunhost.h; the rules in hostrules.h).
//
// How a draw of the game's first-person gun is made (skin.cpp's census and DrawHeldPart): a DrawIndexed of its mesh, skinned by its bones in the game's
// 4096-byte per-draw block (the mod draws a lying gun by binding its own block there, the bones moved), in each eye's passes of the first-person
// view: its depth pass (writes depth) and its colour pass (the G-buffer, three or more targets, its depth tested against the depth pass's), stencil 32
// in both. Drawn again here: what the draw was drawn with, kept while the game drew it lying, bound again in a pass of the same kind, with the pass's
// own view constants and the mod's bones; then everything as it was.
#include "gunhost.h"

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <vector>

namespace {

using hostrules::Cb;

constexpr int kCbSlots = D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
constexpr int kVsSrvs = 16, kVsSamplers = 16;
constexpr int kPsSrvs = 32, kPsSamplers = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;
constexpr int kVbSlots = 8;
constexpr int kTargets = hostrules::kTargets;
constexpr int kBones = hostrules::kBones;
constexpr UINT kPalette = hostrules::kPaletteBytes;
constexpr UINT kBoneBytes = kBones * 48;
constexpr int kMostDraws = 24;  // kept draws of one kind (a body, its rounds, its magazine's own draw, a sight's body, glass and marks, both passes)
constexpr int kMostKinds = 6;   // kinds kept at once (idle ones go first: Sweep)

template <typename T>
void SafeRelease(T*& p) {
    if (p) {
        p->Release();
        p = nullptr;
    }
}
template <typename T>
T* Ref(T* p) {
    if (p)
        p->AddRef();
    return p;
}

// ---------------------------------------------------------------- what's noted while a lying gun is drawn (each thread its own)
struct Noted {
    UINT count = 0, start = 0;
    INT base = 0;
    ID3D11RasterizerState* rs = nullptr;  // referenced
    uint8_t palette[kPalette] = {};
};
struct Notes {
    bool on = false;
    int n = 0;
    bool overflow = false;
    Noted d[gunhost::kMostSubs];
    void Drop() {
        for (int i = 0; i < n; ++i)
            SafeRelease(d[i].rs);
        n = 0;
        on = false;
        overflow = false;
    }
};
thread_local std::unique_ptr<Notes> t_notes;

// ---------------------------------------------------------------- what's kept of a draw
struct KeptCb {
    ID3D11Buffer* original = nullptr;  // the game's, as bound (referenced)
    ID3D11Buffer* copy = nullptr;      // the mod's, copied from it on the card at the draw (kCopy)
    UINT size = 0;
    Cb rule = Cb::kNone;
};
struct State {
    hostrules::Pass pass;
    uint32_t passHash = 0;
    ID3D11InputLayout* layout = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ID3D11Buffer* ib = nullptr;  // the mod's copy, or the game's held
    DXGI_FORMAT ibFormat = DXGI_FORMAT_UNKNOWN;
    UINT ibOffset = 0;
    ID3D11Buffer* vb[kVbSlots] = {};
    UINT stride[kVbSlots] = {}, offset[kVbSlots] = {};
    ID3D11VertexShader* vs = nullptr;
    KeptCb vsCb[kCbSlots];
    ID3D11ShaderResourceView* vsSrv[kVsSrvs] = {};
    ID3D11SamplerState* vsSampler[kVsSamplers] = {};
    ID3D11PixelShader* ps = nullptr;
    KeptCb psCb[kCbSlots];
    ID3D11ShaderResourceView* psSrv[kPsSrvs] = {};
    ID3D11SamplerState* psSampler[kPsSamplers] = {};
    ID3D11BlendState* blend = nullptr;
    FLOAT blendFactor[4] = {};
    UINT sampleMask = 0xFFFFFFFFu;
    ID3D11DepthStencilState* depth = nullptr;
    UINT stencilRef = 0;
    hostrules::Depth depthDesc;
    ID3D11DepthStencilState* relaxed = nullptr;  // made at its first host (hostrules::Relaxed), if it's to be other than depth
    bool relaxedTried = false;
    unsigned vsPalette = 0, psPalette = 0;  // the slots the game's per-draw block was bound in (bit i: slot i)

    void Release() {
        SafeRelease(layout);
        SafeRelease(ib);
        for (ID3D11Buffer*& b : vb)
            SafeRelease(b);
        SafeRelease(vs);
        SafeRelease(ps);
        for (KeptCb* cbs : {vsCb, psCb})
            for (int i = 0; i < kCbSlots; ++i) {
                SafeRelease(cbs[i].original);
                SafeRelease(cbs[i].copy);
                cbs[i] = KeptCb{};
            }
        for (ID3D11ShaderResourceView*& v : vsSrv)
            SafeRelease(v);
        for (ID3D11ShaderResourceView*& v : psSrv)
            SafeRelease(v);
        for (ID3D11SamplerState*& s : vsSampler)
            SafeRelease(s);
        for (ID3D11SamplerState*& s : psSampler)
            SafeRelease(s);
        SafeRelease(blend);
        SafeRelease(depth);
        SafeRelease(relaxed);
        *this = State{};
    }
};
// One of the mod's draws of it, as noted: its indices, rasterizer state and bones on the gun's frame (the rest of the block as it was).
struct Sub {
    UINT count = 0, start = 0;
    INT base = 0;
    ID3D11RasterizerState* rs = nullptr;  // referenced
    float rel[kBones * 12] = {};
    uint8_t tail[kPalette - kBoneBytes] = {};
};
struct Draw {
    UINT count = 0, start = 0;
    INT base = 0;
    State state;
    bool haveState = false;
    unsigned long long stateAt = 0;  // kept (or a try that failed)
    std::vector<Sub> sub;            // the mod's draws of it as last noted (1 to kMostSubs)
    unsigned long long notedAt = 0;  // its last note (hostrules::NotedLately: hosted, and kept on, only while noted with its kind's others)
    void Release() {
        state.Release();
        haveState = false;
        for (Sub& s : sub)
            SafeRelease(s.rs);
        sub.clear();
    }
};
// A buffer of its mesh: the game's (referenced, compared) and the one drawn from (the mod's copy, or the game's held).
struct Mesh {
    ID3D11Buffer* source = nullptr;
    ID3D11Buffer* own = nullptr;
    bool copied = false, dynamic = false;
};
struct Kind {
    uint32_t kind = 0;
    std::vector<std::unique_ptr<Draw>> draws;
    std::vector<Mesh> meshes;
    unsigned long long keptAt = 0, usedAt = 0;  // its newest note (the newest of its draws' notedAt), its last host
    unsigned long long notingSince = 0;         // noted again from then on after a pause (a new time it lies): its draws let go of only after
                                                // hostrules::kNotedWithinMs of it (those from before are noted again in its first frame)
    int convention = 0;  // its depth draws' test (hostrules::kLess ...): 0 none seen
    hostrules::ColourSeen colour[3];  // its colour draws as last hosted in each eye's colour pass (hostrules::EyeIndex): its depth draws wait on it
    void Release() {
        for (auto& d : draws)
            d->Release();
        draws.clear();
        for (Mesh& m : meshes) {
            SafeRelease(m.source);
            SafeRelease(m.own);
        }
        meshes.clear();
    }
};

std::mutex g_mutex;  // everything below
std::vector<std::unique_ptr<Kind>> g_kinds;
std::atomic<bool> g_exact{false};
gunhost::Stats g_stats;
int g_convention = 0;  // any kind's depth draws' test, for one with none of its own

Kind* FindKind(uint32_t kind) {
    for (auto& k : g_kinds)
        if (k->kind == kind)
            return k.get();
    return nullptr;
}

// ms from then to now, 0 for a then after now (one thread's GetTickCount64 read just before another's tick was kept: never a wrap to an age of years).
unsigned long long Age(unsigned long long now, unsigned long long then) {
    return now > then ? now - then : 0;
}

// The kind's mesh buffers no kept state draws from any more (a buffer the game put in another's place, a draw let go of, a keep that failed after its
// copy was made): let go of, so its copies are only those its kept draws use (the review of d43b6df: each new buffer of the game's was copied on, none
// let go of till the kind went). Under g_mutex, with no state being made.
void DropUnusedMeshes(Kind& k) {
    for (auto it = k.meshes.begin(); it != k.meshes.end();) {
        bool used = false;
        for (const auto& d : k.draws) {
            const State& s = d->state;
            used = used || s.ib == it->own;
            for (ID3D11Buffer* b : s.vb)
                used = used || (b && b == it->own);
        }
        if (used) {
            ++it;
        } else {
            SafeRelease(it->source);
            SafeRelease(it->own);
            it = k.meshes.erase(it);
        }
    }
}

// ---------------------------------------------------------------- the pass a draw is in
D3D11_TEXTURE2D_DESC TextureDesc(ID3D11Resource* resource, bool& ok) {
    D3D11_TEXTURE2D_DESC desc{};
    ok = false;
    if (!resource)
        return desc;
    D3D11_RESOURCE_DIMENSION dimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    resource->GetType(&dimension);
    if (dimension != D3D11_RESOURCE_DIMENSION_TEXTURE2D)
        return desc;
    static_cast<ID3D11Texture2D*>(resource)->GetDesc(&desc);
    ok = true;
    return desc;
}

// Whether the draw now writes any colour to its targets (a depth pass's draws with the targets left bound write none).
bool WritesColour(ID3D11DeviceContext* ctx, UINT targets) {
    if (!targets)
        return false;
    ID3D11BlendState* blend = nullptr;
    FLOAT factor[4] = {};
    UINT mask = 0;
    ctx->OMGetBlendState(&blend, factor, &mask);
    if (!blend)
        return true;  // the default state writes everything
    D3D11_BLEND_DESC d{};
    blend->GetDesc(&d);
    blend->Release();
    bool writes = false;
    for (UINT i = 0; i < std::min<UINT>(targets, kTargets); ++i)
        writes = writes || (d.IndependentBlendEnable ? d.RenderTarget[i] : d.RenderTarget[0]).RenderTargetWriteMask != 0;
    return writes;
}

// ---------------------------------------------------------------- keeping a draw's state

// Its mesh's buffer as it's drawn from: the kind's copy of that buffer (made once on the card; a dynamic one copied again each time, its contents
// as they are now), or the game's held when it's too big to copy (never a dynamic one: it changes under it). source: Get's reference, taken.
ID3D11Buffer* MeshOf(ID3D11DeviceContext* ctx, ID3D11Device* device, Kind& k, ID3D11Buffer* source) {
    D3D11_BUFFER_DESC d{};
    source->GetDesc(&d);
    const bool dynamic = d.Usage == D3D11_USAGE_DYNAMIC;
    for (Mesh& m : k.meshes)
        if (m.source == source) {
            if (m.copied && dynamic)
                ctx->CopyResource(m.own, source);
            source->Release();
            return Ref(m.own);
        }
    Mesh m;
    m.source = source;  // (the Get's reference, kept)
    m.dynamic = dynamic;
    if (d.ByteWidth <= gunhost::kMeshCopyMost && (d.BindFlags & (D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER))) {
        D3D11_BUFFER_DESC c = d;
        c.Usage = D3D11_USAGE_DEFAULT;
        c.CPUAccessFlags = 0;
        if (SUCCEEDED(device->CreateBuffer(&c, nullptr, &m.own)) && m.own) {
            ctx->CopyResource(m.own, source);
            m.copied = true;
        } else {
            m.own = nullptr;
        }
    }
    if (!m.own) {
        if (dynamic) {
            SafeRelease(m.source);
            return nullptr;
        }
        m.own = Ref(source);
    }
    k.meshes.push_back(m);
    return Ref(m.own);
}

// The constant buffers bound on a context now, both stages, their sizes and which are views' (fns.viewKind, or the views' size): taken before this
// file's mutex is (viewKind takes skin.cpp's own lock).
struct CbsNow {
    ID3D11Buffer* b[2][kCbSlots] = {};  // [0] the vertex shader's, [1] the pixel shader's (referenced)
    UINT size[2][kCbSlots] = {};
    bool view[2][kCbSlots] = {};
    void Take(ID3D11DeviceContext* ctx, const gunhost::Fns& fns) {
        ctx->VSGetConstantBuffers(0, kCbSlots, b[0]);
        ctx->PSGetConstantBuffers(0, kCbSlots, b[1]);
        for (int s = 0; s < 2; ++s)
            for (int i = 0; i < kCbSlots; ++i) {
                if (!b[s][i])
                    continue;
                D3D11_BUFFER_DESC d{};
                b[s][i]->GetDesc(&d);
                size[s][i] = d.ByteWidth;
                view[s][i] = (fns.viewKind && fns.viewKind(b[s][i]) != 0) || d.ByteWidth == hostrules::kViewBytes;
            }
    }
    ~CbsNow() {
        for (auto& stage : b)
            for (ID3D11Buffer*& p : stage)
                SafeRelease(p);
    }
};

// Its constant buffers (one stage's, as bound for the draw): each referenced; the game's per-draw block noted (paletteMask); a view's left to the pass;
// an immutable one as it is; any other copied on the card as it is now (into the last kept copy of that size, if there is one). False: a copy
// couldn't be made.
bool KeepCbs(ID3D11DeviceContext* ctx, ID3D11Device* device, const CbsNow& now, int stage, ID3D11Resource* palette, KeptCb* out, const KeptCb* old,
             unsigned& paletteMask) {
    for (int i = 0; i < kCbSlots; ++i) {
        ID3D11Buffer* b = now.b[stage][i];
        if (!b)
            continue;
        D3D11_BUFFER_DESC d{};
        b->GetDesc(&d);
        const bool isPalette = palette && static_cast<ID3D11Resource*>(b) == palette;
        const bool isView = now.view[stage][i];
        KeptCb& c = out[i];
        c.original = Ref(b);
        c.size = d.ByteWidth;
        c.rule = hostrules::RuleOf(true, d.ByteWidth, isView, isPalette, d.Usage == D3D11_USAGE_IMMUTABLE);
        if (isPalette)
            paletteMask |= 1u << i;
        if (c.rule != Cb::kCopy)
            continue;
        if (old && old[i].copy && old[i].size == d.ByteWidth) {
            c.copy = Ref(old[i].copy);
        } else {
            D3D11_BUFFER_DESC copy{d.ByteWidth, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0};
            if (FAILED(device->CreateBuffer(&copy, nullptr, &c.copy)) || !c.copy) {
                c.copy = nullptr;
                return false;
            }
        }
        ctx->CopyResource(c.copy, b);
    }
    return true;
}

// Everything the draw now on ctx is drawn with but its targets, viewports and bones (cbs: its constant buffers as bound, taken before the lock; pass:
// its pass): why not (nullptr: kept).
const char* Capture(ID3D11DeviceContext* ctx, ID3D11Device* device, const CbsNow& cbs, const hostrules::Pass& pass, uint32_t passHash,
                    ID3D11Resource* palette, Kind& k, const State& old, State& out) {
    out = State{};
    out.pass = pass;
    out.passHash = passHash;
    if (!out.passHash)
        return "no depth target bound";
    {
        ID3D11HullShader* hs = nullptr;
        ID3D11DomainShader* ds = nullptr;
        ID3D11GeometryShader* gs = nullptr;
        ctx->HSGetShader(&hs, nullptr, nullptr);
        ctx->DSGetShader(&ds, nullptr, nullptr);
        ctx->GSGetShader(&gs, nullptr, nullptr);
        const bool stages = hs || ds || gs;
        SafeRelease(hs);
        SafeRelease(ds);
        SafeRelease(gs);
        if (stages)
            return "a hull, domain or geometry shader with it";
    }
    ctx->VSGetShader(&out.vs, nullptr, nullptr);
    ctx->PSGetShader(&out.ps, nullptr, nullptr);
    if (!out.vs)
        return "no vertex shader";
    if (out.pass.colour && !out.ps)
        return "colour written with no pixel shader";
    ctx->IAGetInputLayout(&out.layout);
    if (!out.layout)
        return "no input layout";
    ctx->IAGetPrimitiveTopology(&out.topology);
    ID3D11Buffer* ib = nullptr;
    ctx->IAGetIndexBuffer(&ib, &out.ibFormat, &out.ibOffset);
    if (!ib)
        return "no index buffer";
    if (!(out.ib = MeshOf(ctx, device, k, ib)))
        return "its index buffer is rewritten each frame and too big to copy";
    ID3D11Buffer* vbs[kVbSlots] = {};
    ctx->IAGetVertexBuffers(0, kVbSlots, vbs, out.stride, out.offset);
    if (!vbs[0]) {
        for (ID3D11Buffer*& b : vbs)
            SafeRelease(b);
        return "no vertex buffer in slot 0";
    }
    bool meshKept = true;
    for (int i = 0; i < kVbSlots; ++i)
        if (vbs[i] && !(out.vb[i] = MeshOf(ctx, device, k, vbs[i])))  // (MeshOf takes the Get's reference whatever it answers)
            meshKept = false;
    if (!meshKept)
        return "a vertex buffer is rewritten each frame and too big to copy";
    bool copied = KeepCbs(ctx, device, cbs, 0, palette, out.vsCb, old.vsCb, out.vsPalette);
    copied = KeepCbs(ctx, device, cbs, 1, palette, out.psCb, old.psCb, out.psPalette) && copied;
    if (!copied)
        return "its constants couldn't be copied";
    if (!out.vsPalette)
        return "the game's per-draw block isn't bound in its vertex shader";
    // Each pass's own view goes where it had its own: with none found, a copy of the one it was kept with would show it where it was kept from.
    if (std::none_of(std::begin(out.vsCb), std::end(out.vsCb), [](const KeptCb& c) { return c.rule == Cb::kView; }))
        return "no view constants among its vertex shader's";
    ctx->VSGetShaderResources(0, kVsSrvs, out.vsSrv);
    ctx->VSGetSamplers(0, kVsSamplers, out.vsSampler);
    ctx->PSGetShaderResources(0, kPsSrvs, out.psSrv);
    ctx->PSGetSamplers(0, kPsSamplers, out.psSampler);
    ctx->OMGetBlendState(&out.blend, out.blendFactor, &out.sampleMask);
    ctx->OMGetDepthStencilState(&out.depth, &out.stencilRef);
    D3D11_DEPTH_STENCIL_DESC depth{};
    if (out.depth) {
        out.depth->GetDesc(&depth);
    } else {
        depth.DepthEnable = TRUE, depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL, depth.DepthFunc = D3D11_COMPARISON_LESS;  // (the default state)
    }
    out.depthDesc.enable = depth.DepthEnable != FALSE;
    out.depthDesc.writes = depth.DepthWriteMask == D3D11_DEPTH_WRITE_MASK_ALL;
    out.depthDesc.func = static_cast<int>(depth.DepthFunc);
    return nullptr;
}

// ---------------------------------------------------------------- hosting
// Everything Host changes, as it was, put back afterwards (its targets, viewports, scissors, compute stage and stream output it never touches).
struct Saved {
    ID3D11InputLayout* layout = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ID3D11Buffer* ib = nullptr;
    DXGI_FORMAT ibFormat = DXGI_FORMAT_UNKNOWN;
    UINT ibOffset = 0;
    ID3D11Buffer* vb[kVbSlots] = {};
    UINT stride[kVbSlots] = {}, offset[kVbSlots] = {};
    ID3D11VertexShader* vs = nullptr;
    ID3D11Buffer* vsCb[kCbSlots] = {};
    ID3D11ShaderResourceView* vsSrv[kVsSrvs] = {};
    ID3D11SamplerState* vsSampler[kVsSamplers] = {};
    ID3D11HullShader* hs = nullptr;
    ID3D11DomainShader* ds = nullptr;
    ID3D11GeometryShader* gs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11Buffer* psCb[kCbSlots] = {};
    ID3D11ShaderResourceView* psSrv[kPsSrvs] = {};
    ID3D11SamplerState* psSampler[kPsSamplers] = {};
    ID3D11RasterizerState* raster = nullptr;
    ID3D11BlendState* blend = nullptr;
    FLOAT blendFactor[4] = {};
    UINT sampleMask = 0xFFFFFFFFu;
    ID3D11DepthStencilState* depth = nullptr;
    UINT stencilRef = 0;
    ID3D11Predicate* predicate = nullptr;
    BOOL predicateValue = FALSE;

    void Save(ID3D11DeviceContext* c) {
        c->IAGetInputLayout(&layout);
        c->IAGetPrimitiveTopology(&topology);
        c->IAGetIndexBuffer(&ib, &ibFormat, &ibOffset);
        c->IAGetVertexBuffers(0, kVbSlots, vb, stride, offset);
        c->VSGetShader(&vs, nullptr, nullptr);
        c->VSGetConstantBuffers(0, kCbSlots, vsCb);
        c->VSGetShaderResources(0, kVsSrvs, vsSrv);
        c->VSGetSamplers(0, kVsSamplers, vsSampler);
        c->HSGetShader(&hs, nullptr, nullptr);
        c->DSGetShader(&ds, nullptr, nullptr);
        c->GSGetShader(&gs, nullptr, nullptr);
        c->PSGetShader(&ps, nullptr, nullptr);
        c->PSGetConstantBuffers(0, kCbSlots, psCb);
        c->PSGetShaderResources(0, kPsSrvs, psSrv);
        c->PSGetSamplers(0, kPsSamplers, psSampler);
        c->RSGetState(&raster);
        c->OMGetBlendState(&blend, blendFactor, &sampleMask);
        c->OMGetDepthStencilState(&depth, &stencilRef);
        c->GetPredication(&predicate, &predicateValue);
    }
    void Restore(ID3D11DeviceContext* c) {
        c->IASetInputLayout(layout);
        c->IASetPrimitiveTopology(topology);
        c->IASetIndexBuffer(ib, ibFormat, ibOffset);
        c->IASetVertexBuffers(0, kVbSlots, vb, stride, offset);
        c->VSSetShader(vs, nullptr, 0);
        c->VSSetConstantBuffers(0, kCbSlots, vsCb);
        c->VSSetShaderResources(0, kVsSrvs, vsSrv);
        c->VSSetSamplers(0, kVsSamplers, vsSampler);
        c->HSSetShader(hs, nullptr, 0);
        c->DSSetShader(ds, nullptr, 0);
        c->GSSetShader(gs, nullptr, 0);
        c->PSSetShader(ps, nullptr, 0);
        c->PSSetConstantBuffers(0, kCbSlots, psCb);
        c->PSSetShaderResources(0, kPsSrvs, psSrv);
        c->PSSetSamplers(0, kPsSamplers, psSampler);
        c->RSSetState(raster);
        c->OMSetBlendState(blend, blendFactor, sampleMask);
        c->OMSetDepthStencilState(depth, stencilRef);
        c->SetPredication(predicate, predicateValue);
        SafeRelease(layout);
        SafeRelease(ib);
        for (ID3D11Buffer*& b : vb)
            SafeRelease(b);
        SafeRelease(vs);
        for (ID3D11Buffer*& b : vsCb)
            SafeRelease(b);
        for (ID3D11ShaderResourceView*& v : vsSrv)
            SafeRelease(v);
        for (ID3D11SamplerState*& s : vsSampler)
            SafeRelease(s);
        SafeRelease(hs);
        SafeRelease(ds);
        SafeRelease(gs);
        SafeRelease(ps);
        for (ID3D11Buffer*& b : psCb)
            SafeRelease(b);
        for (ID3D11ShaderResourceView*& v : psSrv)
            SafeRelease(v);
        for (ID3D11SamplerState*& s : psSampler)
            SafeRelease(s);
        SafeRelease(raster);
        SafeRelease(blend);
        SafeRelease(depth);
        SafeRelease(predicate);
    }
};

// The kept constant buffers (one stage's: 0 the vertex shader's, 1 the pixel shader's) as they're bound again: a view's slot the host pass's own view of
// its size (hostrules::PickView: in that slot, else the first slot of that stage that has one; a pixel shader's, else the vertex shader's, else the one
// it was kept with); the palette's the mod's; an immutable one the game's; any other its copy. False: no view of that size in the vertex shader's slots
// (nothing drawn). standIns: a pixel shader's view slots bound from the vertex shader's or as kept, counted.
bool PickCbs(const KeptCb* kept, const CbsNow& host, int stage, ID3D11Buffer* palette, ID3D11Buffer** out, int& standIns) {
    for (int i = 0; i < kCbSlots; ++i) {
        const KeptCb& c = kept[i];
        out[i] = nullptr;
        switch (c.rule) {
        case Cb::kPalette: out[i] = palette; break;
        case Cb::kOriginal: out[i] = c.original; break;
        case Cb::kCopy: out[i] = c.copy ? c.copy : c.original; break;
        case Cb::kView: {
            const auto viewIn = [&](int s, int j) { return host.b[s][j] && host.view[s][j] && host.size[s][j] == c.size; };
            const auto firstIn = [&](int s) {
                for (int j = 0; j < kCbSlots; ++j)
                    if (viewIn(s, j))
                        return j;
                return -1;
            };
            const int inStage = firstIn(stage), inVertex = stage == 1 ? firstIn(0) : -1;
            switch (hostrules::PickView(stage == 1, viewIn(stage, i), inStage >= 0, inVertex >= 0)) {
            case hostrules::ViewFrom::kSlot: out[i] = host.b[stage][i]; break;
            case hostrules::ViewFrom::kStage: out[i] = host.b[stage][inStage]; break;
            case hostrules::ViewFrom::kVertex:
                out[i] = host.b[0][inVertex];
                ++standIns;
                break;
            case hostrules::ViewFrom::kKept:
                out[i] = c.original;
                ++standIns;
                break;
            default: return false;
            }
            break;
        }
        default: break;
        }
    }
    return true;
}

// This thread's own palette buffer (dynamic, written whole with WRITE_DISCARD at each draw: never read), for the device ctx belongs to.
struct OwnPalette {
    ID3D11Device* device = nullptr;  // not referenced: compared only
    ID3D11Buffer* buffer = nullptr;
};
thread_local OwnPalette t_palette;
ID3D11Buffer* PaletteBuffer(ID3D11DeviceContext* ctx) {
    ID3D11Device* device = nullptr;
    ctx->GetDevice(&device);
    if (!device)
        return nullptr;
    if (t_palette.buffer && t_palette.device != device)
        SafeRelease(t_palette.buffer);
    if (!t_palette.buffer) {
        D3D11_BUFFER_DESC desc{kPalette, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0};
        if (FAILED(device->CreateBuffer(&desc, nullptr, &t_palette.buffer)))
            t_palette.buffer = nullptr;
        t_palette.device = device;
    }
    device->Release();
    return t_palette.buffer;
}

// The depth state a colour draw is hosted with (hostrules::Relaxed): made once for it. Null: the kept one.
ID3D11DepthStencilState* DepthFor(ID3D11DeviceContext* ctx, State& s, int convention) {
    if (!s.pass.colour)
        return nullptr;
    const hostrules::Depth want = hostrules::Relaxed(s.depthDesc, g_exact.load(std::memory_order_relaxed), convention);
    if (want.func == s.depthDesc.func && want.writes == s.depthDesc.writes)
        return nullptr;
    if (!s.relaxedTried) {
        s.relaxedTried = true;
        D3D11_DEPTH_STENCIL_DESC d{};
        if (s.depth) {
            s.depth->GetDesc(&d);
        } else {
            d.DepthEnable = TRUE;
            d.StencilReadMask = d.StencilWriteMask = 0xFF;
            d.FrontFace = d.BackFace = {D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS};
        }
        d.DepthWriteMask = want.writes ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
        d.DepthFunc = static_cast<D3D11_COMPARISON_FUNC>(want.func);
        ID3D11Device* device = nullptr;
        ctx->GetDevice(&device);
        if (device) {
            if (FAILED(device->CreateDepthStencilState(&d, &s.relaxed)))
                s.relaxed = nullptr;
            device->Release();
        }
    }
    return s.relaxed;
}

void Bind(ID3D11DeviceContext* ctx, const State& s, ID3D11Buffer* const* vsCbs, ID3D11Buffer* const* psCbs, ID3D11DepthStencilState* depth) {
    ctx->IASetInputLayout(s.layout);
    ctx->IASetPrimitiveTopology(s.topology);
    ctx->IASetIndexBuffer(s.ib, s.ibFormat, s.ibOffset);
    ctx->IASetVertexBuffers(0, kVbSlots, s.vb, s.stride, s.offset);
    ctx->VSSetShader(s.vs, nullptr, 0);
    ctx->VSSetConstantBuffers(0, kCbSlots, vsCbs);
    ctx->VSSetShaderResources(0, kVsSrvs, s.vsSrv);
    ctx->VSSetSamplers(0, kVsSamplers, s.vsSampler);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->PSSetShader(s.ps, nullptr, 0);
    ctx->PSSetConstantBuffers(0, kCbSlots, psCbs);
    ctx->PSSetShaderResources(0, kPsSrvs, s.psSrv);
    ctx->PSSetSamplers(0, kPsSamplers, s.psSampler);
    ctx->OMSetBlendState(s.blend, s.blendFactor, s.sampleMask);
    ctx->OMSetDepthStencilState(depth ? depth : s.depth, s.stencilRef);
}

void DescribeCbs(const char* stage, const KeptCb* cbs, char* text, size_t size) {
    size_t used = std::strlen(text);
    bool any = false;
    for (int i = 0; i < kCbSlots && used + 32 < size; ++i)
        if (cbs[i].rule != Cb::kNone) {
            if (!any) {
                const int n = std::snprintf(text + used, size - used, " %s", stage);
                used += n > 0 ? static_cast<size_t>(n) : 0;
                any = true;
            }
            const int n = std::snprintf(text + used, size - used, " %d:%u %s", i, cbs[i].size, hostrules::CbName(cbs[i].rule));
            used += n > 0 ? static_cast<size_t>(n) : 0;
        }
}

double Ms(const LARGE_INTEGER& from) {
    static const double frequency = [] {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        return static_cast<double>(f.QuadPart);
    }();
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return static_cast<double>(t.QuadPart - from.QuadPart) * 1000.0 / frequency;
}

}  // namespace

namespace gunhost {

// ---------------------------------------------------------------- keeping

void Begin() {
    if (!t_notes)
        t_notes = std::make_unique<Notes>();
    t_notes->Drop();
    t_notes->on = true;
}

bool Noting() {
    return t_notes && t_notes->on;
}

void Note(ID3D11DeviceContext* ctx, UINT count, UINT start, INT base, const void* palette, UINT size) {
    Notes* notes = t_notes.get();
    if (!notes || !notes->on || !ctx || !palette)
        return;
    if (notes->n >= kMostSubs) {
        notes->overflow = true;
        return;
    }
    Noted& d = notes->d[notes->n++];
    d.count = count, d.start = start, d.base = base;
    ctx->RSGetState(&d.rs);
    const UINT n = std::min(size, kPalette);
    std::memcpy(d.palette, palette, n);
    if (n < kPalette)
        std::memset(d.palette + n, 0, kPalette - n);
}

void Cancel() {
    if (t_notes)
        t_notes->Drop();
}

void End(ID3D11DeviceContext* ctx, const Fns& fns, uint32_t kind, UINT count, UINT start, INT base, const float* pose, ID3D11Resource* paletteBuffer,
         unsigned long long now) {
    Notes* notes = t_notes.get();
    if (!notes || !notes->on)
        return;
    notes->on = false;
    if (notes->overflow) {
        // (More of the mod's draws for it than kept: none taken, the ones kept before stay as they were; said with the refusals.)
        notes->Drop();
        static_assert(kMostSubs == 12, "the line below says 12");
        std::lock_guard lock(g_mutex);
        ++g_stats.refused;
        g_stats.refusal = "more than 12 of the mod's draws for one draw of the game's (gunhost::kMostSubs)";
        return;
    }
    // The bones on the gun's frame, out of the lock (a draw thread's own work, into this thread's own room for them).
    const int n = notes->n;
    thread_local std::unique_ptr<Sub[]> t_subs;
    if (!t_subs)
        t_subs = std::make_unique<Sub[]>(kMostSubs);
    Sub* const subs = t_subs.get();
    bool ok = n > 0 && kind && pose && ctx && hostrules::Finite(pose, 12);
    for (int i = 0; ok && i < n; ++i) {
        const Noted& d = notes->d[i];
        Sub& s = subs[i];
        s.rs = nullptr;
        s.count = d.count, s.start = d.start, s.base = d.base;
        ok = hostrules::Rel(pose, reinterpret_cast<const float*>(d.palette), kBones, s.rel);
        std::memcpy(s.tail, d.palette + kBoneBytes, sizeof(s.tail));
    }
    if (!ok) {
        notes->Drop();
        return;
    }
    for (int i = 0; i < n; ++i) {
        subs[i].rs = notes->d[i].rs;  // (the note's reference, handed on)
        notes->d[i].rs = nullptr;
    }
    notes->n = 0;
    ID3D11Device* device = nullptr;
    ctx->GetDevice(&device);
    // What the state's keep needs from the context and from skin.cpp, before the lock (the views' check takes skin.cpp's own).
    bool due = false;
    {
        std::lock_guard lock(g_mutex);
        const Kind* k = FindKind(kind);
        const Draw* d = nullptr;
        if (k)
            for (const auto& each : k->draws)
                if (each->count == count && each->start == start && each->base == base)
                    d = each.get();
        due = !d || !d->haveState || Age(now, d->stateAt) >= kRefreshMs;
    }
    CbsNow cbs;
    hostrules::Pass pass;
    uint32_t passHash = 0;
    if (due) {
        cbs.Take(ctx, fns);
        passHash = PassNow(ctx, &pass);
    }
    std::lock_guard lock(g_mutex);
    Kind* k = FindKind(kind);
    if (!k) {
        // (Room for it: the kind used longest ago goes, never one hosted lately: Sweep keeps those; with all hosted, none kept.)
        if (static_cast<int>(g_kinds.size()) >= kMostKinds) {
            auto oldest = g_kinds.end();
            for (auto it = g_kinds.begin(); it != g_kinds.end(); ++it)
                if (Age(now, (*it)->usedAt) > 5000 && (oldest == g_kinds.end() || std::max((*it)->keptAt, (*it)->usedAt) < std::max((*oldest)->keptAt, (*oldest)->usedAt)))
                    oldest = it;
            if (oldest != g_kinds.end()) {
                (*oldest)->Release();
                g_kinds.erase(oldest);
            }
        }
        if (static_cast<int>(g_kinds.size()) < kMostKinds) {
            g_kinds.push_back(std::make_unique<Kind>());
            k = g_kinds.back().get();
            k->kind = kind;
        }
    }
    Draw* d = nullptr;
    bool letGo = false;
    if (k) {
        // This draw's own; any other not noted within hostrules::kNotedWithinMs of the kind's newest note (this one) while the kind has been noted that
        // long let go of: a part the game stopped drawing, or one from an earlier time it lay that this time hasn't (the review of d43b6df: a part kept
        // once was hosted on for good, room taken from the rest). Host leaves those out meanwhile (NotedLately).
        if (Age(now, k->keptAt) > hostrules::kNotedWithinMs)
            k->notingSince = now;  // (noted again after a pause: a new time it lies, its draws from before noted again in its first frame)
        const unsigned long long newest = std::max(k->keptAt, now);
        const bool settled = Age(newest, k->notingSince) > hostrules::kNotedWithinMs;
        for (auto it = k->draws.begin(); it != k->draws.end();) {
            Draw& each = **it;
            if (each.count == count && each.start == start && each.base == base) {
                d = &each;
                ++it;
            } else if (settled && !hostrules::NotedLately(each.notedAt, newest)) {
                each.Release();
                it = k->draws.erase(it);
                ++g_stats.letGo;
                letGo = true;
            } else {
                ++it;
            }
        }
        if (!d && static_cast<int>(k->draws.size()) < kMostDraws) {
            k->draws.push_back(std::make_unique<Draw>());
            d = k->draws.back().get();
            d->count = count, d->start = start, d->base = base;
        }
    }
    if (!d) {
        for (int i = 0; i < n; ++i)
            SafeRelease(subs[i].rs);
        if (letGo)
            DropUnusedMeshes(*k);
        SafeRelease(device);
        return;
    }
    for (Sub& s : d->sub)
        SafeRelease(s.rs);
    d->sub.assign(subs, subs + n);  // (their rs references move with them)
    for (int i = 0; i < n; ++i)
        subs[i].rs = nullptr;
    d->notedAt = now;
    k->keptAt = std::max(k->keptAt, now);
    bool captured = false;
    if (device && due && (!d->haveState || Age(now, d->stateAt) >= kRefreshMs)) {
        State fresh;
        const char* why = Capture(ctx, device, cbs, pass, passHash, paletteBuffer, *k, d->state, fresh);
        d->stateAt = now;  // (a try that failed is tried again a refresh on)
        captured = true;
        if (!why) {
            d->state.Release();
            d->state = fresh;
            d->haveState = true;
            ++g_stats.keptStates;
            if (!fresh.pass.colour && fresh.depthDesc.enable && fresh.depthDesc.writes)
                k->convention = g_convention = fresh.depthDesc.func;
        } else {
            fresh.Release();
            ++g_stats.refused;
            g_stats.refusal = why;
        }
    }
    if (captured || letGo)
        DropUnusedMeshes(*k);  // (the copies the state kept before used, the game's buffer since put in another's place: gone)
    SafeRelease(device);
}

// ---------------------------------------------------------------- hosting

uint32_t PassNow(ID3D11DeviceContext* ctx, hostrules::Pass* out) {
    hostrules::Pass p;
    ID3D11RenderTargetView* rtvs[kTargets] = {};
    ID3D11DepthStencilView* dsv = nullptr;
    ctx->OMGetRenderTargets(kTargets, rtvs, &dsv);
    bool gap = false;
    for (int i = 0; i < kTargets; ++i) {
        if (!rtvs[i]) {
            gap = true;
            continue;
        }
        if (!gap) {
            D3D11_RENDER_TARGET_VIEW_DESC d{};
            rtvs[i]->GetDesc(&d);
            p.format[p.targets++] = static_cast<unsigned>(d.Format);
        }
        rtvs[i]->Release();
    }
    if (dsv) {
        ID3D11Resource* resource = nullptr;
        dsv->GetResource(&resource);
        bool ok = false;
        const D3D11_TEXTURE2D_DESC t = TextureDesc(resource, ok);
        if (ok) {
            p.depth = true;
            p.depthFormat = static_cast<unsigned>(t.Format), p.width = t.Width, p.height = t.Height, p.samples = t.SampleDesc.Count;
        }
        SafeRelease(resource);
        dsv->Release();
    }
    p.colour = WritesColour(ctx, p.targets);
    if (out)
        *out = p;
    return p.depth ? hostrules::PassHash(p) : 0;
}

uintptr_t ViewNow(ID3D11DeviceContext* ctx, const Fns& fns) {
    ID3D11Buffer* bound[kCbSlots] = {};
    ctx->VSGetConstantBuffers(0, kCbSlots, bound);
    uintptr_t view = 0;
    for (ID3D11Buffer* b : bound) {
        if (!b)
            continue;
        if (!view) {
            D3D11_BUFFER_DESC d{};
            b->GetDesc(&d);
            if ((fns.viewKind && fns.viewKind(b) != 0) || d.ByteWidth == hostrules::kViewBytes)
                view = reinterpret_cast<uintptr_t>(b);
        }
        b->Release();
    }
    return view;
}

HostOut Host(ID3D11DeviceContext* ctx, const Fns& fns, uint32_t kind, const float* frameInDraw, unsigned long long now, int eye, uint32_t frame) {
    HostOut out;
    if (!ctx || !fns.draw || !fns.map || !fns.unmap || !frameInDraw || !hostrules::Finite(frameInDraw, 12)) {
        out.why = "nothing to draw with";
        return out;
    }
    LARGE_INTEGER began;
    QueryPerformanceCounter(&began);
    hostrules::Pass pass;
    const uint32_t hash = PassNow(ctx, &pass);
    if (!hash) {
        out.why = "no depth target bound";
        return out;
    }
    // The pass's own constant buffers (the views' check takes skin.cpp's own lock) and the mod's palette buffer, before this file's lock.
    CbsNow host;
    host.Take(ctx, fns);
    ID3D11Buffer* palette = PaletteBuffer(ctx);
    std::lock_guard lock(g_mutex);
    if (!palette) {
        ++g_stats.noPalette;
        out.why = "no palette buffer of the mod's";
        return out;
    }
    Kind* k = FindKind(kind);
    if (!k) {
        out.why = "nothing kept of it";
        return out;
    }
    // Its draws of this pass, noted with the kind's newest (hostrules::NotedLately: never a part the game had stopped drawing before it went).
    Draw* todo[kMostDraws];
    int count = 0;
    for (auto& d : k->draws)
        if (d->haveState && !d->sub.empty() && hostrules::NotedLately(d->notedAt, k->keptAt) && d->state.passHash == hash &&
            hostrules::SamePass(d->state.pass, pass) && count < kMostDraws)
            todo[count++] = d.get();
    out.kept = count;
    if (!count) {
        ++g_stats.noPass;
        out.why = "none of its draws is of this pass";
        return out;
    }
    // Its depth draws only right after its colour draws were all drawn in this eye's colour pass (hostrules::DepthAfterColour): never its depth and
    // stencil alone with nothing in the G-buffer (a black or wrongly lit shape). The relaxed colour draw stands alone meanwhile.
    hostrules::ColourSeen& seen = k->colour[hostrules::EyeIndex(eye)];
    if (!pass.colour && !hostrules::DepthAfterColour(seen, frame)) {
        ++g_stats.depthHeld;
        k->usedAt = now;
        out.why = "its colour draws weren't all drawn in this eye's colour pass just before: its depth left out (never its depth alone)";
        return out;
    }
    Saved saved;
    saved.Save(ctx);
    ctx->SetPredication(nullptr, FALSE);
    const int convention = k->convention ? k->convention : g_convention;
    thread_local std::vector<uint8_t> t_bytes;
    t_bytes.resize(kPalette);
    int left = 0, standIns = 0;  // kept draws (or the mod's draws of one) left out; pixel shader view slots bound from elsewhere
    for (int i = 0; i < count; ++i) {
        Draw& d = *todo[i];
        ID3D11Buffer* vsCbs[kCbSlots] = {};
        ID3D11Buffer* psCbs[kCbSlots] = {};
        int vsStandIns = 0, psStandIns = 0;
        if (!PickCbs(d.state.vsCb, host, 0, palette, vsCbs, vsStandIns) || !PickCbs(d.state.psCb, host, 1, palette, psCbs, psStandIns)) {
            ++g_stats.noView;
            ++left;
            continue;
        }
        if (d.state.ps)
            standIns += psStandIns;  // (a depth draw with no pixel shader: its slots left bound from before read by nothing, not counted)
        Bind(ctx, d.state, vsCbs, psCbs, DepthFor(ctx, d.state, convention));
        for (const Sub& sub : d.sub) {
            hostrules::Place(frameInDraw, sub.rel, kBones, reinterpret_cast<float*>(t_bytes.data()));
            std::memcpy(t_bytes.data() + kBoneBytes, sub.tail, sizeof(sub.tail));
            D3D11_MAPPED_SUBRESOURCE mapped{};
            if (FAILED(fns.map(ctx, palette, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)) || !mapped.pData) {
                ++g_stats.noPalette;
                ++left;
                continue;
            }
            std::memcpy(mapped.pData, t_bytes.data(), kPalette);  // (written whole: never read back)
            fns.unmap(ctx, palette, 0);
            ctx->RSSetState(sub.rs);
            fns.draw(ctx, sub.count, sub.start, sub.base);
            ++out.draws;
        }
    }
    saved.Restore(ctx);
    k->usedAt = now;
    g_stats.viewStandIns += static_cast<uint32_t>(standIns);
    if (pass.colour) {  // (what its depth draws in this eye wait on, next frame)
        seen.have = true;
        seen.all = left == 0 && out.draws > 0;
        seen.frame = frame;
    }
    if (out.draws) {
        ++g_stats.passes;
        g_stats.draws += static_cast<uint32_t>(out.draws);
    } else {
        out.why = "no view constants of its size in its vertex shader's slots in this pass (or its palette not written)";
    }
    g_stats.worstMs = std::max(g_stats.worstMs, Ms(began));
    return out;
}

bool Ready(uint32_t kind, unsigned long long now) {
    std::lock_guard lock(g_mutex);
    const Kind* k = FindKind(kind);
    if (!k || Age(now, std::max(k->keptAt, k->usedAt)) > hostrules::kIdleMs)
        return false;
    for (const auto& d : k->draws)
        if (d->haveState && !d->sub.empty() && d->state.pass.colour && hostrules::NotedLately(d->notedAt, k->keptAt))
            return true;
    return false;
}

bool Info(uint32_t kind, KindInfo& out) {
    out = KindInfo{};
    std::lock_guard lock(g_mutex);
    const Kind* k = FindKind(kind);
    if (!k)
        return false;
    out.keptAt = k->keptAt;
    size_t used = 0;
    bool described = false;
    for (const auto& d : k->draws) {
        if (!d->haveState)
            continue;
        (d->state.pass.colour ? out.colour : out.depth) += 1;
        out.subs += static_cast<int>(d->sub.size());
        if (used + 48 < sizeof(out.draws)) {
            const int n = std::snprintf(out.draws + used, sizeof(out.draws) - used, "%s%u/%u %s%s", used ? ", " : "", d->count, d->start,
                                        d->state.pass.colour ? "colour" : "depth", d->sub.size() > 1 ? " (more than one draw of the mod's)" : "");
            used += n > 0 ? static_cast<size_t>(n) : 0;
        }
        if (!described && d->state.pass.colour) {
            described = true;
            DescribeCbs("vs", d->state.vsCb, out.cbs, sizeof(out.cbs));
            DescribeCbs("ps", d->state.psCb, out.cbs, sizeof(out.cbs));
        }
    }
    for (const Mesh& m : k->meshes)
        (m.copied ? out.meshCopied : out.meshHeld) += 1;
    return true;
}

void Sweep(unsigned long long now, const uint32_t* hosted, int n) {
    std::lock_guard lock(g_mutex);
    for (auto it = g_kinds.begin(); it != g_kinds.end();) {
        bool host = false;
        for (int i = 0; i < n; ++i)
            host = host || hosted[i] == (*it)->kind;
        if (!host && Age(now, std::max((*it)->keptAt, (*it)->usedAt)) > hostrules::kIdleMs) {
            (*it)->Release();
            it = g_kinds.erase(it);
        } else {
            ++it;
        }
    }
}

void Clear() {
    std::lock_guard lock(g_mutex);
    for (auto& k : g_kinds)
        k->Release();
    g_kinds.clear();
}

void SetExact(bool exact) {
    g_exact = exact;
}

Stats TakeStats() {
    std::lock_guard lock(g_mutex);
    const Stats s = g_stats;
    g_stats = Stats{};
    return s;
}

}  // namespace gunhost

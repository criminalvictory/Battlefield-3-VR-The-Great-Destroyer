#include "effects.h"

#include <windows.h>
#include <MinHook.h>
#include <d3dcompiler.h>

#include <algorithm>
#include <atomic>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <map>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "ammo.h"
#include "camera.h"
#include "depth3d.h"
#include "log.h"
#include "overlay.h"
#include "shaders.h"
#include "skin.h"

namespace {

using DrawFn = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, UINT, UINT);
using DrawInstancedFn = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, UINT, UINT, UINT, UINT);
using DrawIndexedInstancedFn = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, UINT, UINT, UINT, INT, UINT);
using DrawIndirectFn = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Buffer*, UINT);

DrawFn g_origDraw[2] = {};
DrawInstancedFn g_origDrawInstanced[2] = {};
DrawIndexedInstancedFn g_origDrawIndexedInstanced[2] = {};
DrawIndirectFn g_origDrawIndexedIndirect[2] = {}, g_origDrawIndirect[2] = {};

// ---- Census ----
// Which draws come with firing: every draw for a few seconds, tallied by what it's drawn with (the
// kind of call, the shaders, the stencil reference: BF3 marks first-person draws with 32), first
// standing still and then firing. An effect's draw keeps its shaders while its counts change with
// how many particles there are.
enum DrawType : uint8_t { kIndexed, kPlain, kIndexedInstanced, kInstanced, kIndexedIndirect, kIndirect, kTypes };
const char* const kTypeNames[kTypes] = {"DrawIndexed",          "Draw",
                                        "DrawIndexedInstanced", "DrawInstanced",
                                        "DrawIndexedInstancedIndirect", "DrawInstancedIndirect"};

struct Kind {
    uint8_t type;
    const void* vs;
    const void* ps;
    UINT stencilRef;
    bool operator==(const Kind& o) const {
        return type == o.type && vs == o.vs && ps == o.ps && stencilRef == o.stencilRef;
    }
};
struct KindHash {
    size_t operator()(const Kind& k) const {
        return std::hash<const void*>()(k.vs) ^ (std::hash<const void*>()(k.ps) << 1) ^ (size_t{k.type} << 3) ^
               (size_t{k.stencilRef} << 7);
    }
};
// Draws of a kind in the baseline and after it, the frames it was drawn in (a muzzle flash shows
// in some frames only, a draw or two each: too few on average to stand out), their counts' and
// instances' range, and the state the first one was drawn with.
struct Tally {
    uint32_t before = 0, after = 0;
    uint32_t framesBefore = 0, framesAfter = 0, lastFrame = UINT32_MAX;
    UINT minCount = UINT_MAX, maxCount = 0, minInstances = UINT_MAX, maxInstances = 0;
    bool described = false;
    D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    UINT stride = 0;
    bool blend = false, depthTest = false, depthWrite = false;
    D3D11_BLEND src = D3D11_BLEND_ONE, dst = D3D11_BLEND_ZERO;
    char buffers[160] = "";  // the vertex shader's constant buffers: slot:size, "a" the arms' view, "w" the world's
};

constexpr ULONGLONG kBaseline = 2000, kCensus = 6000;  // ms
// weapon.txt "hidefx": indexed draws with these index counts are skipped, to tell in the headset
// which are the muzzle flash and the casings (0 = unused).
std::atomic<UINT> g_hideFx[4];

// weapon.txt "hideps": draws with these pixel shaders are skipped (pointers, as the census's last
// lines log them, not the checksums of its others; they change with every start of the game).
std::atomic<uintptr_t> g_hidePs[4];
std::atomic<bool> g_anyHidePs{false};

bool HiddenPs(ID3D11DeviceContext* ctx) {
    if (!g_anyHidePs.load(std::memory_order_relaxed))
        return false;
    ID3D11PixelShader* ps = nullptr;
    ctx->PSGetShader(&ps, nullptr, nullptr);
    if (!ps)
        return false;
    ps->Release();
    for (const auto& h : g_hidePs)
        if (h.load(std::memory_order_relaxed) == reinterpret_cast<uintptr_t>(ps))
            return true;
    return false;
}

// The sun's lens flare and dirty lens (shaders.cpp): quads of their own, so only a 6-vertex draw is
// asked about.
bool LensFlare(ID3D11DeviceContext* ctx, UINT count) {
    if (count != 6)
        return false;
    ID3D11PixelShader* ps = nullptr;
    ctx->PSGetShader(&ps, nullptr, nullptr);
    if (!ps)
        return false;
    ps->Release();
    return shaders::IsLensFlare(ps);
}

bool Hidden(UINT count) {
    for (const auto& h : g_hideFx)
        if (UINT c = h.load(std::memory_order_relaxed); c && c == count)
            return true;
    return false;
}

// weapon.txt "movefx": indexed draws with these index counts are drawn where the gun in the right
// hand is, not where the game holds its own (free hands): the view-projection they're drawn with
// (the per-view constants at +176) times the gun's move. The muzzle flash is drawn in the world
// pass at the game's own muzzle, at the chest.
std::atomic<UINT> g_moveFx[4];

bool Moved(UINT count) {
    for (const auto& m : g_moveFx)
        if (UINT c = m.load(std::memory_order_relaxed); c && c == count)
            return true;
    return false;
}

// False when it can't be moved: the gun isn't being moved, or no view constants are known.
// Row-major 4x4 times 4x4 (column vectors): out = a b.
void Mul4(const float* a, const float* b, float* out) {
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            out[r * 4 + c] = a[r * 4] * b[c] + a[r * 4 + 1] * b[4 + c] + a[r * 4 + 2] * b[8 + c] + a[r * 4 + 3] * b[12 + c];
}

// Which of the view constants a moved draw gets changed (weapon.txt "flashmats", a sum): 1 the view
// (+48), 2 the view-projection (+176), 4 the one without translation that takes points less the eye
// (+240, the eye at +320), 8 the projection (+112) as if the view-space points were moved in the
// world. Changing one a shader doesn't use does nothing; changing two it chains moves it twice.
std::atomic<int> g_moveMatrices{2 | 4};
std::atomic<float> g_lift{0.0f};  // weapon.txt "flashlift": metres up on top of the move, to see it plainly
// What became of the draws there were to move, since the last log.
std::atomic<uint32_t> g_movedDraws{0}, g_noDelta{0}, g_noView{0}, g_noSlot{0}, g_shots{0};

// Rigid 4x4 inverse (rotation transposed, translation turned back).
void InvertRigid4(const float* m, float* out) {
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c)
            out[r * 4 + c] = m[c * 4 + r];
        out[r * 4 + 3] = -(m[r] * m[3] + m[4 + r] * m[7] + m[8 + r] * m[11]);
    }
    out[12] = out[13] = out[14] = 0.0f;
    out[15] = 1.0f;
}

// Drawn with the per-view constants changed so whatever it draws moves with the gun: whichever of
// them g_moveMatrices says. False when it can't be: the gun isn't being moved, no view constants
// are known, or they aren't bound to the vertex shader.
bool DrawMoved(int which, ID3D11DeviceContext* ctx, const std::function<void()>& draw) {
    float d[16];
    if (!skin::GunDeltaWorld(d)) {
        ++g_noDelta;
        return false;
    }
    d[7] += g_lift.load(std::memory_order_relaxed);  // the world's up is y
    ID3D11Resource* resource = nullptr;
    const uint8_t* view = skin::LatestViewConstants(resource);
    if (!view) {
        ++g_noView;
        return false;
    }
    uint8_t copy[352];
    std::memcpy(copy, view, sizeof(copy));
    auto at = [&](size_t offset) { return reinterpret_cast<float*>(copy + offset); };
    const int mats = g_moveMatrices.load(std::memory_order_relaxed);
    float moved[16];
    if (mats & 8) {
        // View-space points moved in the world: P (V D V^-1), the view as it came.
        float inverse[16], vd[16], vdv[16];
        InvertRigid4(at(48), inverse);
        Mul4(at(48), d, vd);
        Mul4(vd, inverse, vdv);
        Mul4(at(112), vdv, moved);
        std::memcpy(at(112), moved, sizeof(moved));
    }
    if (mats & 4) {
        // p - eye moved: R (p - eye) + (R eye + t - eye).
        const float* eye = at(320);
        float relative[16];
        std::memcpy(relative, d, sizeof(relative));
        for (int r = 0; r < 3; ++r)
            relative[r * 4 + 3] = d[r * 4] * eye[0] + d[r * 4 + 1] * eye[1] + d[r * 4 + 2] * eye[2] + d[r * 4 + 3] - eye[r];
        Mul4(at(240), relative, moved);
        std::memcpy(at(240), moved, sizeof(moved));
    }
    for (int bit : {1, 2}) {
        if (!(mats & bit))
            continue;
        const size_t offset = bit == 1 ? 48 : 176;
        Mul4(at(offset), d, moved);
        std::memcpy(at(offset), moved, sizeof(moved));
    }
    if (!skin::DrawWithCopy(which, ctx, resource, copy, sizeof(copy), draw)) {
        ++g_noSlot;
        return false;
    }
    ++g_movedDraws;
    return true;
}

// ---- The muzzle flash ----
// The game draws it at its own muzzle, at the chest; with free hands it's moved with the gun. What's seen of it is quads: "The
// flash's particles" below. The 6-vertex sprites of 24 bytes that come with firing (the census, 2026-10-30), chased for a while
// as the flash, are not it: their vertex shaders take clip space as it is (their disassembly, 2026-11-02), the lens ghosts of
// its light (centred across the view's middle from where the light shows, 5.6 times as far). Nor is the 36-index mesh found
// with them, also drawn when nobody fires, and moving that one made somebody's gear jump. The flash doesn't come with every
// round, in the game as it is too: more smoke than fire, a flame now and then.
std::atomic<ULONGLONG> g_lastShot{0};
int g_lastLoaded = -1;
// The fire button last held (the headset's trigger comes as the left mouse button: input.cpp). The first flash of a burst is
// drawn before the server's count of rounds goes down, and would be left at the game's muzzle: what comes with a shot is
// looked for from the trigger on.
std::atomic<ULONGLONG> g_triggerAt{0};

ULONGLONG MsSinceFiring() {
    return GetTickCount64() - std::max(g_lastShot.load(std::memory_order_relaxed), g_triggerAt.load(std::memory_order_relaxed));
}

// A buffer's bytes read back through a staging copy (a stall: for probes only). A vertex buffer's
// range is copied as it is; a constant buffer whole (it can't be copied in part).
bool ReadBack(ID3D11DeviceContext* ctx, ID3D11Buffer* buffer, UINT offset, UINT size, void* out) {
    D3D11_BUFFER_DESC desc;
    buffer->GetDesc(&desc);
    const bool constants = (desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER) != 0;
    if (constants)
        offset = 0, size = std::min(size, desc.ByteWidth);
    if (offset + size > desc.ByteWidth || !size)
        return false;
    ID3D11Device* device = nullptr;
    buffer->GetDevice(&device);
    D3D11_BUFFER_DESC staging{};
    staging.ByteWidth = constants ? desc.ByteWidth : (size + 15) & ~15u;
    staging.Usage = D3D11_USAGE_STAGING;
    staging.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Buffer* copy = nullptr;
    bool ok = false;
    if (device && SUCCEEDED(device->CreateBuffer(&staging, nullptr, &copy))) {
        if (constants) {
            ctx->CopyResource(copy, buffer);
        } else {
            const D3D11_BOX box{offset, 0, 0, offset + size, 1, 1};
            ctx->CopySubresourceRegion(copy, 0, 0, 0, 0, buffer, 0, &box);
        }
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(ctx->Map(copy, 0, D3D11_MAP_READ, 0, &mapped))) {
            std::memcpy(out, static_cast<const uint8_t*>(mapped.pData) + (constants ? offset : 0), size);
            ctx->Unmap(copy, 0);
            ok = true;
        }
        copy->Release();
    }
    if (device)
        device->Release();
    return ok;
}

std::atomic<ULONGLONG> g_scanUntil{0}, g_scanFrom{0}, g_scanBaseUntil{0};  // weapon.txt "flashscan"
// How much of what comes out of the gun is moved with it (weapon.txt "flashquads"): the flash's quads, below. A long shape round
// the barrel's line: how far off it all the way and not at all (m), how far past the muzzle.
std::atomic<bool> g_moveQuads{true};
std::atomic<float> g_quadsAcross{0.6f}, g_quadsAlong{2.2f};
constexpr float kQuadsAcrossFade = 0.4f, kQuadsAlongFade = 1.0f;
constexpr float kQuadsBehind = 0.6f;  // behind the muzzle, for the ejection port's puff
// What's moved for a while after firing lets go gently: in its last second less and less, not with a jump.
float FiringFade(ULONGLONG lasts) {
    const ULONGLONG since = MsSinceFiring();
    return since >= lasts ? 0.0f : std::min(1.0f, static_cast<float>(lasts - since) / 1000.0f);
}

// ---- The casings ----
// A small mesh (234 indices the M416's, 114 the train's pistol's, the same shaders; the census, 2026-10-30, 2026-11-02) drawn in each pass
// (colour, depth, shadows), flying from where the game's gun throws them, at the chest. Their world placements are 3x4
// matrices in the instance stream (stride 48: the instance probe, 2026-10-31 04:12; changing the view constants had moved
// nothing), one a draw on the M416, up to eight on the pistol. They're copied on the graphics card and moved as the gun is
// (from the game's gun to the drawn one) by a small compute shader: all the way near the ejection port, less as the casing
// flies off, not at all further than that, so the ones lying on the ground stay where they fell (crouched, the gun is
// higher). Only while firing (and a little after): then casings are flying. weapon.txt "casings 0": as the game draws them;
// "casings 1 0.45 1.3": moved all the way within 0.45 m of the port, not at 1.3 m. A casing leaves the port as fast as it's
// thrown, about 3 m/s: at 0.2 m and 0.8 m, as these were at first, it was soon on its way back to the game's own gun.
constexpr bool IsCasingCount(UINT count) {
    return count == 234 || count == 114 || count == 303;  // (303: the M240's links, weapon.txt flashsurvey, 2026-11-05: "just implement them in the right place")
}
constexpr UINT kInstanceStride = 48;
constexpr UINT kCasingsMost = 64;  // instances in a draw
constexpr ULONGLONG kCasingFlight = 1500;  // ms after the last shot
std::atomic<bool> g_moveCasings{true};
std::atomic<float> g_casingNear{0.45f}, g_casingFar{1.3f};
std::atomic<uint32_t> g_casingsMoved{0}, g_casingsNot{0};

constexpr char kCasingShader[] = R"(
cbuffer Move : register(b0) { float4 d0, d1, d2, port, reach; uint count, pad0, pad1, pad2; };
RWByteAddressBuffer dst : register(u0);
[numthreads(16, 1, 1)] void Main(uint3 id : SV_DispatchThreadID) {
    if (id.x >= count)
        return;
    const uint at = id.x * 48;
    float4 m0 = asfloat(dst.Load4(at)), m1 = asfloat(dst.Load4(at + 16)), m2 = asfloat(dst.Load4(at + 32));
    // The gun's move times the casing's placement (3x4 rows: turn | place).
    float4 n0 = d0.x * m0 + d0.y * m1 + d0.z * m2 + float4(0, 0, 0, d0.w);
    float4 n1 = d1.x * m0 + d1.y * m1 + d1.z * m2 + float4(0, 0, 0, d1.w);
    float4 n2 = d2.x * m0 + d2.y * m1 + d2.z * m2 + float4(0, 0, 0, d2.w);
    // reach: all the way that near the port, not at all that far (metres).
    float w = reach.z * saturate((reach.y - distance(float3(m0.w, m1.w, m2.w), port.xyz)) / (reach.y - reach.x));
    dst.Store4(at, asuint(lerp(m0, n0, w)));
    dst.Store4(at + 16, asuint(lerp(m1, n1, w)));
    dst.Store4(at + 32, asuint(lerp(m2, n2, w)));
}
)";

struct CasingResources {
    ID3D11Buffer* out = nullptr;  // the game's instances, copied and moved: the instance stream drawn with
    ID3D11Buffer* move = nullptr;
    ID3D11UnorderedAccessView* outView = nullptr;
    ID3D11ComputeShader* cs = nullptr;
    bool failed = false;
} g_casing;
std::mutex g_casingMutex;  // the resources made once; the draws come from several threads

bool EnsureCasingResources(ID3D11Device* device) {
    std::lock_guard lock(g_casingMutex);
    if (g_casing.cs || g_casing.failed)
        return g_casing.cs != nullptr;
    ID3DBlob* code = nullptr;
    ID3DBlob* errors = nullptr;
    bool ok = SUCCEEDED(D3DCompile(kCasingShader, sizeof(kCasingShader) - 1, "casings", nullptr, nullptr, "Main", "cs_5_0",
                                   0, 0, &code, &errors)) &&
              SUCCEEDED(device->CreateComputeShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &g_casing.cs));
    if (errors) {
        Log("Effects: the casings' shader: %s", static_cast<const char*>(errors->GetBufferPointer()));
        errors->Release();
    }
    if (code)
        code->Release();
    D3D11_BUFFER_DESC out{kCasingsMost * kInstanceStride, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_UNORDERED_ACCESS,
                          0, D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS, 0};
    D3D11_BUFFER_DESC move{96, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0};
    D3D11_UNORDERED_ACCESS_VIEW_DESC uv{};
    uv.Format = DXGI_FORMAT_R32_TYPELESS;
    uv.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uv.Buffer.NumElements = kCasingsMost * kInstanceStride / 4;
    uv.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
    ok = ok && SUCCEEDED(device->CreateBuffer(&out, nullptr, &g_casing.out)) &&
         SUCCEEDED(device->CreateBuffer(&move, nullptr, &g_casing.move)) &&
         SUCCEEDED(device->CreateUnorderedAccessView(g_casing.out, &uv, &g_casing.outView));
    if (!ok) {
        g_casing.failed = true;
        Log("Effects: the casings can't be moved with the gun (no compute shader or buffers)");
    }
    return ok;
}

// A casing draw (its placements in stream 1) drawn moved with the gun: draw gets the first instance to use. False: as is.
// Bullets aren't drawn but as tracers (192 indices, added to the picture, placed in the same stream; 2026-11-02: one flew past
// at 90 m/s along a straight line, from another soldier's gun): the M416 and the train's pistol make none of their own (25
// rounds, the survey), so there's nothing of that to move. Another gun's would show in weapon.txt "flashsurvey".
bool DrawInstancesMoved(ID3D11DeviceContext* ctx, UINT instances, UINT first, const std::function<void(UINT)>& draw) {
    if (!instances || instances > kCasingsMost || MsSinceFiring() >= kCasingFlight || !g_moveCasings.load(std::memory_order_relaxed))
        return false;
    float d[16], port[3];
    if (!skin::GunDeltaWorld(d) || !skin::GameGunPort(port))
        return false;  // a gun the mod doesn't place: where the game has them
    ID3D11Buffer* stream = nullptr;
    UINT stride = 0, offset = 0;
    ctx->IAGetVertexBuffers(1, 1, &stream, &stride, &offset);
    if (!stream || stride != kInstanceStride) {
        if (stream)
            stream->Release();
        ++g_casingsNot;
        return false;
    }
    ID3D11Device* device = nullptr;
    ctx->GetDevice(&device);
    const bool ready = device && EnsureCasingResources(device);
    if (device)
        device->Release();
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (!ready || FAILED(ctx->Map(g_casing.move, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        stream->Release();
        ++g_casingsNot;
        return false;
    }
    float* cb = static_cast<float*>(mapped.pData);
    std::memcpy(cb, d, 12 * sizeof(float));  // the move's three rows
    cb[12] = port[0], cb[13] = port[1], cb[14] = port[2], cb[15] = 0.0f;  // the port where the game's gun has it
    const float nearest = g_casingNear.load(std::memory_order_relaxed);
    cb[16] = nearest, cb[17] = std::max(g_casingFar.load(std::memory_order_relaxed), nearest + 0.01f);
    cb[18] = FiringFade(kCasingFlight), cb[19] = 0.0f;
    reinterpret_cast<UINT*>(cb)[20] = instances;
    ctx->Unmap(g_casing.move, 0);
    const UINT left = offset + first * kInstanceStride;
    const D3D11_BOX box{left, 0, 0, left + instances * kInstanceStride, 1, 1};
    D3D11_BUFFER_DESC streamDesc;
    stream->GetDesc(&streamDesc);
    if (box.right > streamDesc.ByteWidth) {
        stream->Release();
        ++g_casingsNot;
        return false;
    }
    // weapon.txt "flashscan": where they are on the game's gun, from its port or muzzle (a stall: for finding things only).
    if (float body[12]; GetTickCount64() <= g_scanUntil.load(std::memory_order_relaxed) && skin::GameGunBody(body)) {
        float m[12] = {};
        for (UINT i = 0; i < std::min(instances, 8u) && ReadBack(ctx, stream, left + i * kInstanceStride, 48, m); ++i) {
            const float rel[3] = {m[3] - port[0], m[7] - port[1], m[11] - port[2]};
            float local[3];
            for (int a = 0; a < 3; ++a) {
                const float axis[3] = {body[a], body[4 + a], body[8 + a]};
                const float len2 = axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2];
                local[a] = (rel[0] * axis[0] + rel[1] * axis[1] + rel[2] * axis[2]) / (len2 > 0.0f ? len2 : 1.0f);
            }
            const float dist = std::sqrt(rel[0] * rel[0] + rel[1] * rel[1] + rel[2] * rel[2]);
            Log("Instances +%3d ms: %s %u/%u at %.2f m, on the gun %+.3f %+.3f %+.3f (x left, y up, z forward)",
                static_cast<int>(GetTickCount64() - g_scanFrom.load(std::memory_order_relaxed)), "casing", i, instances,
                dist, local[0], local[1], local[2]);
        }
    }
    ctx->CopySubresourceRegion(g_casing.out, 0, 0, 0, 0, stream, 0, &box);
    // The compute stage as the game had it, put back afterwards.
    ID3D11ComputeShader* cs = nullptr;
    ID3D11UnorderedAccessView* uav = nullptr;
    ID3D11Buffer* cbuf = nullptr;
    ctx->CSGetShader(&cs, nullptr, nullptr);
    ctx->CSGetUnorderedAccessViews(0, 1, &uav);
    ctx->CSGetConstantBuffers(0, 1, &cbuf);
    ctx->CSSetShader(g_casing.cs, nullptr, 0);
    const UINT keepCount = UINT_MAX;  // an append buffer's counter left as it is
    ctx->CSSetUnorderedAccessViews(0, 1, &g_casing.outView, &keepCount);
    ctx->CSSetConstantBuffers(0, 1, &g_casing.move);
    ctx->Dispatch((instances + 15) / 16, 1, 1);
    ID3D11UnorderedAccessView* noUav = nullptr;
    ctx->CSSetUnorderedAccessViews(0, 1, &noUav, &keepCount);  // before it's read as a vertex buffer
    ctx->CSSetShader(cs, nullptr, 0);
    ctx->CSSetUnorderedAccessViews(0, 1, &uav, &keepCount);
    ctx->CSSetConstantBuffers(0, 1, &cbuf);
    for (IUnknown* p : std::initializer_list<IUnknown*>{cs, uav, cbuf})
        if (p)
            p->Release();
    const UINT zero = 0;
    ctx->IASetVertexBuffers(1, 1, &g_casing.out, &stride, &zero);
    draw(0);
    ctx->IASetVertexBuffers(1, 1, &stream, &stride, &offset);
    stream->Release();
    ++g_casingsMoved;
    return true;
}

// ---- The flash's particles ----
// What's seen of a flash (the fireball, its core, sparks and smoke; found 2026-11-02 with weapon.txt "flashscan" and
// "flashburst"): quads of 48-byte vertices already in the world (the vertex shader takes the first three floats less the
// camera), four vertices and six indices each, from one big dynamic vertex buffer. The game batches them by material, so a
// batch also holds what's nothing to do with the gun (a cloud of smoke two metres off, far sparks). On the game's gun they
// sit along its barrel (the pistol's fireball 28 cm along the frame bone's z, on its axis), so moved as the gun is moved they
// are along the drawn gun's. The batch is copied on the graphics card and a small compute shader moves each quad near the
// game's barrel, whole; a quad further off goes less far, and none past the far distance. Only for a while after a shot.
// Smoke from the barrel and the ejection port (the M416: a small puff 1.3 cm from the port, a plume along the barrel to 2 m
// past the muzzle growing to quads 1.5 m across, the fireball 55 cm past it; the pistol: a puff on the muzzle, its fireball,
// a thin smoke for 2 s) and the dirt and sparks that go with them are all among these quads, and move with them: the survey
// (weapon.txt "flashsurvey") found nothing else near the gun that comes with firing on either gun but the casings. It follows
// a long shape, not a ball round the muzzle, so the smoke hanging in a burning train car isn't taken along, and it ends where
// the plume does: the dirt a shot throws up where it lands (the same material, in the same batches: 6.6 m past the muzzle
// firing at the ground in the rifle mission) stays there. Shot at something within 3 m, that dirt goes with the gun too: nothing
// in a quad tells them apart. Nor smoke already hanging there, in a burning train car: taken along at once with the first shot,
// it would hop as far as the hand is from the game's gun. So the shape starts as small as the first flash (the fireball, on the
// barrel's line up to 55 cm past the muzzle) and grows to its whole size in 0.6 s: what was there before is eased in. When the
// shooting stops the move is kept as it was: what's in the air stays there as the gun moves on, not sweeping about with the
// hand. While shooting, all of it within the shape goes with the gun, quads old and new.
// weapon.txt "flashquads 0": as the game draws them; "flashquads 1 0.6 2.2": all the way within 0.6 m of the barrel's line up to
// 2.2 m past the muzzle, none 0.4 m and 1 m further on.
constexpr UINT kQuadStride = 48;
constexpr UINT kQuadsMost = 4096;
constexpr UINT kQuadRing = 1024 * 1024;  // bytes, at least: the one they're in has 8 MB
constexpr ULONGLONG kQuadsAfterShot = 3000;  // ms: the smoke's still rising
constexpr float kQuadsStartAcross = 0.15f, kQuadsStartAcrossFade = 0.15f, kQuadsStartAlong = 1.0f, kQuadsStartAlongFade = 0.5f;
constexpr float kQuadsGrowMs = 600.0f;
constexpr ULONGLONG kQuadsLive = 200;  // ms after the last shot: the fireball's still there
std::atomic<ULONGLONG> g_quadsOpenedAt{0};
std::atomic<uint32_t> g_quadBatches{0}, g_quadsNot{0};

// A shot or the trigger, before it's noted: with the window shut or nearly (its fade under a half), the shape starts small again.
void NoteQuadsOpening() {
    if (MsSinceFiring() >= kQuadsAfterShot - 500)
        g_quadsOpenedAt = GetTickCount64();
}

constexpr char kQuadShader[] = R"(
cbuffer Move : register(b0) { float4 d0, d1, d2, muzzle, axis, reach; uint quads, stride, pad0, pad1; };
RWByteAddressBuffer vertices : register(u0);
[numthreads(64, 1, 1)] void Main(uint3 id : SV_DispatchThreadID) {
    if (id.x >= quads)
        return;
    const uint first = id.x * 4 * stride;
    float3 p[4];
    float3 centre = 0;
    for (uint k = 0; k < 4; ++k) {
        p[k] = asfloat(vertices.Load3(first + k * stride));
        centre += p[k] / 4;
    }
    // How far along the barrel's line from the muzzle, how far off it; reach: across, all the way and not at all, then along;
    // muzzle.w: how far behind the muzzle it counts all the way, axis.w: how much is left of the moment after firing.
    const float3 rel = centre - muzzle.xyz;
    const float along = dot(rel, axis.xyz);
    const float across = length(rel - along * axis.xyz);
    const float w = axis.w * saturate((reach.y - across) / (reach.y - reach.x)) * saturate((reach.w - along) / (reach.w - reach.z)) *
                    saturate((along + muzzle.w + 0.4) / 0.4);
    if (w <= 0)
        return;
    for (uint v = 0; v < 4; ++v) {
        const float3 moved = float3(dot(d0.xyz, p[v]) + d0.w, dot(d1.xyz, p[v]) + d1.w, dot(d2.xyz, p[v]) + d2.w);
        vertices.Store3(first + v * stride, asuint(lerp(p[v], moved, w)));
    }
}
)";

std::mutex g_quadMutex;  // the shader made once
ID3D11ComputeShader* g_quadShader = nullptr;
bool g_quadShaderFailed = false;
// Each thread's own: the draws come from several, each with its own context.
struct QuadBuffers {
    ID3D11Buffer* vertices = nullptr;
    ID3D11Buffer* move = nullptr;
    ID3D11UnorderedAccessView* view = nullptr;
};
thread_local QuadBuffers t_quads;

bool EnsureQuadResources(ID3D11Device* device) {
    {
        std::lock_guard lock(g_quadMutex);
        if (!g_quadShader && !g_quadShaderFailed) {
            ID3DBlob* code = nullptr;
            ID3DBlob* errors = nullptr;
            const bool ok = SUCCEEDED(D3DCompile(kQuadShader, sizeof(kQuadShader) - 1, "flash quads", nullptr, nullptr, "Main",
                                                 "cs_5_0", 0, 0, &code, &errors)) &&
                            SUCCEEDED(device->CreateComputeShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr,
                                                                  &g_quadShader));
            if (errors) {
                Log("Effects: the flash quads' shader: %s", static_cast<const char*>(errors->GetBufferPointer()));
                errors->Release();
            }
            if (code)
                code->Release();
            g_quadShaderFailed = !ok;
        }
        if (!g_quadShader)
            return false;
    }
    if (t_quads.view)
        return true;
    const UINT bytes = kQuadsMost * 4 * kQuadStride;
    D3D11_BUFFER_DESC vertices{bytes, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_UNORDERED_ACCESS, 0,
                               D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS, 0};
    D3D11_BUFFER_DESC move{112, D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0};
    D3D11_UNORDERED_ACCESS_VIEW_DESC uv{};
    uv.Format = DXGI_FORMAT_R32_TYPELESS;
    uv.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uv.Buffer.NumElements = bytes / 4;
    uv.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
    if (SUCCEEDED(device->CreateBuffer(&vertices, nullptr, &t_quads.vertices)) &&
        SUCCEEDED(device->CreateBuffer(&move, nullptr, &t_quads.move)) &&
        SUCCEEDED(device->CreateUnorderedAccessView(t_quads.vertices, &uv, &t_quads.view)))
        return true;
    static std::atomic<bool> s_logged{false};
    if (!s_logged.exchange(true))
        Log("Effects: the flash's quads can't be moved with the gun (no buffers)");
    return false;
}

bool Blended(ID3D11DeviceContext* ctx);

// A batch of quads like the flash's, for a while after a shot. One comparison when there's been no shot.
bool QuadBatch(ID3D11DeviceContext* ctx, UINT count, UINT start) {
    if (MsSinceFiring() >= kQuadsAfterShot || !g_moveQuads.load(std::memory_order_relaxed) || !count || count % 6 != 0 ||
        start % 6 != 0 || count / 6 > kQuadsMost)
        return false;
    ID3D11Buffer* stream = nullptr;
    UINT stride = 0, offset = 0;
    ctx->IAGetVertexBuffers(0, 1, &stream, &stride, &offset);
    if (!stream)
        return false;
    // Their big dynamic buffer: the level's meshes of 48-byte vertices (with their own indices) stay out of it.
    D3D11_BUFFER_DESC desc;
    stream->GetDesc(&desc);
    stream->Release();
    return stride == kQuadStride && desc.Usage == D3D11_USAGE_DYNAMIC && desc.ByteWidth >= kQuadRing && Blended(ctx);
}

// The move and the game's gun the quads go by: as they are now, while shooting; from then on as they were.
std::mutex g_quadMoveMutex;
struct QuadMove {
    bool have = false;
    float d[16] = {}, muzzle[3] = {}, body[12] = {};
} g_quadMove;

bool QuadMoveNow(float (&d)[16], float (&muzzle)[3], float (&body)[12]) {
    std::lock_guard lock(g_quadMoveMutex);
    if (MsSinceFiring() < kQuadsLive || !g_quadMove.have) {
        g_quadMove.have = skin::GunDeltaWorld(g_quadMove.d) && skin::GameGunMuzzle(g_quadMove.muzzle) && skin::GameGunBody(g_quadMove.body);
        if (!g_quadMove.have)
            return false;
    }
    std::memcpy(d, g_quadMove.d, sizeof(d));
    std::memcpy(muzzle, g_quadMove.muzzle, sizeof(muzzle));
    std::memcpy(body, g_quadMove.body, sizeof(body));
    return true;
}

// Drawn with its quads near the game's muzzle moved as the gun is. False: not moved, draw it as it is. The index buffer is
// the usual list of quads (0 1 2 ..., 4 5 6 ...) from 0 for each batch, so its vertices follow each other from its offset.
bool DrawQuadsMoved(ID3D11DeviceContext* ctx, UINT count, UINT start, INT base, const std::function<void(INT)>& draw) {
    float d[16], muzzle[3], body[12];
    if (!QuadMoveNow(d, muzzle, body))
        return false;  // a gun the mod doesn't place: where the game has it (off the barrel by as far as the head has moved)
    const float barrel[3] = {body[2], body[6], body[10]};  // the gun's z, to its muzzle
    const float barrelLength = std::sqrt(barrel[0] * barrel[0] + barrel[1] * barrel[1] + barrel[2] * barrel[2]);
    if (!(barrelLength > 0.0f))
        return false;
    ID3D11Device* device = nullptr;
    ctx->GetDevice(&device);
    const bool ready = device && EnsureQuadResources(device);
    if (device)
        device->Release();

    ID3D11Buffer* stream = nullptr;
    UINT stride = 0, offset = 0;
    ctx->IAGetVertexBuffers(0, 1, &stream, &stride, &offset);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (!ready || !stream || FAILED(ctx->Map(t_quads.move, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        if (stream)
            stream->Release();
        ++g_quadsNot;
        return false;
    }
    const UINT quads = count / 6;
    auto* cb = static_cast<float*>(mapped.pData);
    std::memcpy(cb, d, 12 * sizeof(float));
    cb[12] = muzzle[0], cb[13] = muzzle[1], cb[14] = muzzle[2], cb[15] = kQuadsBehind;
    cb[16] = barrel[0] / barrelLength, cb[17] = barrel[1] / barrelLength, cb[18] = barrel[2] / barrelLength;
    cb[19] = FiringFade(kQuadsAfterShot);
    const float grown = std::min(static_cast<float>(GetTickCount64() - g_quadsOpenedAt.load(std::memory_order_relaxed)) / kQuadsGrowMs, 1.0f);
    const auto Grown = [grown](float from, float to) {
        from = std::min(from, to);
        return from + (to - from) * grown;
    };
    const float across = Grown(kQuadsStartAcross, g_quadsAcross.load(std::memory_order_relaxed));
    const float along = Grown(kQuadsStartAlong, g_quadsAlong.load(std::memory_order_relaxed));
    cb[20] = across, cb[21] = across + Grown(kQuadsStartAcrossFade, kQuadsAcrossFade);
    cb[22] = along, cb[23] = along + Grown(kQuadsStartAlongFade, kQuadsAlongFade);
    auto* counts = reinterpret_cast<UINT*>(cb + 24);
    counts[0] = quads, counts[1] = kQuadStride, counts[2] = counts[3] = 0;
    ctx->Unmap(t_quads.move, 0);
    // Each batch's vertices start at its stream offset and its indices at 0 (read back on the train, 2026-11-02).
    const UINT left = offset + static_cast<UINT>(base) * kQuadStride;
    const D3D11_BOX box{left, 0, 0, left + quads * 4 * kQuadStride, 1, 1};

    D3D11_BUFFER_DESC streamDesc;
    stream->GetDesc(&streamDesc);
    if (box.right > streamDesc.ByteWidth || base < 0) {
        stream->Release();
        ++g_quadsNot;
        return false;
    }
    ctx->CopySubresourceRegion(t_quads.vertices, 0, 0, 0, 0, stream, 0, &box);
    // The compute stage as the game had it, put back afterwards.
    ID3D11ComputeShader* cs = nullptr;
    ID3D11UnorderedAccessView* uav = nullptr;
    ID3D11Buffer* cbuf = nullptr;
    ctx->CSGetShader(&cs, nullptr, nullptr);
    ctx->CSGetUnorderedAccessViews(0, 1, &uav);
    ctx->CSGetConstantBuffers(0, 1, &cbuf);
    ctx->CSSetShader(g_quadShader, nullptr, 0);
    const UINT keepCount = UINT_MAX;
    ctx->CSSetUnorderedAccessViews(0, 1, &t_quads.view, &keepCount);
    ctx->CSSetConstantBuffers(0, 1, &t_quads.move);
    ctx->Dispatch((quads + 63) / 64, 1, 1);
    ID3D11UnorderedAccessView* noUav = nullptr;
    ctx->CSSetUnorderedAccessViews(0, 1, &noUav, &keepCount);  // before it's read as a vertex buffer
    ctx->CSSetShader(cs, nullptr, 0);
    ctx->CSSetUnorderedAccessViews(0, 1, &uav, &keepCount);
    ctx->CSSetConstantBuffers(0, 1, &cbuf);
    for (IUnknown* p : std::initializer_list<IUnknown*>{cs, uav, cbuf})
        if (p)
            p->Release();
    const UINT quadStride = kQuadStride, zero = 0;
    ctx->IASetVertexBuffers(0, 1, &t_quads.vertices, &quadStride, &zero);
    draw(0);  // the batch now starts its buffer
    ctx->IASetVertexBuffers(0, 1, &stream, &stride, &offset);
    stream->Release();
    ++g_quadBatches;
    return true;
}

// weapon.txt "dumpfx 234": the next three indexed instanced draws with that index count log the
// start of the 4096-byte block they're drawn with (whether each casing's own place is in it).
std::atomic<UINT> g_dumpFx{0};
std::atomic<int> g_dumpFxLeft{0};

float Length3(const float* v);
float Distance3(const float* a, const float* b);

// General 4x4 inverse (row-major); false when singular.
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
    if (!(std::fabs(det) > 1e-12f))
        return false;
    for (int i = 0; i < 16; ++i)
        out[i] = inv[i] / det;
    return true;
}

// weapon.txt "dumpfx N" (N = 6: the flash's sprites, 234: the casings): the next three draws with
// that count log their 4096-byte block, and any 4x4 in it that is the view-projection times a
// rigid placement (each row of the view-projection's inverse times it, rows or columns: a world-
// view-projection), with where that placement is against the game's muzzle.
void DumpFx(UINT count) {
    if (count != g_dumpFx.load(std::memory_order_relaxed) || g_dumpFxLeft.fetch_sub(1) <= 0)
        return;
    ID3D11Resource* resource = nullptr;
    std::vector<uint8_t> block;
    ID3D11Resource* viewBuffer = nullptr;
    const uint8_t* view = skin::LatestViewConstants(viewBuffer);
    float body[12], world[3], arms[3];
    const bool gun = skin::GameGunBody(body) && skin::GameGunPlaces(world, arms);
    float muzzle[3] = {};
    for (int r = 0; r < 3 && gun; ++r)
        muzzle[r] = body[r * 4 + 1] * 0.04f + body[r * 4 + 2] * 0.78f + body[r * 4 + 3];
    if (!skin::LatestBlock(resource, block) || block.size() < 128 || !view) {
        Log("Effects: dump of a %u-count draw: no block or view", count);
        return;
    }
    const auto* f = reinterpret_cast<const float*>(block.data());
    Log("Effects: dump of a %u-count draw (the game's muzzle at %.3f %.3f %.3f in the world):", count, muzzle[0],
        muzzle[1], muzzle[2]);
    for (int r = 0; r < 8; ++r)
        Log("Effects:   +%3d: %12.4f %12.4f %12.4f %12.4f", r * 16, f[r * 4], f[r * 4 + 1], f[r * 4 + 2], f[r * 4 + 3]);
    float vpInverse[16];
    if (!Invert4(reinterpret_cast<const float*>(view + 176), vpInverse))
        return;
    int found = 0;
    for (size_t k = 0; k + 16 <= block.size() / 4 && found < 6; k += 4) {
        for (int transposed = 0; transposed < 2; ++transposed) {
            float m[16], w[16];
            for (int i = 0; i < 16; ++i)
                m[i] = transposed ? f[k + (i % 4) * 4 + i / 4] : f[k + i];
            Mul4(vpInverse, m, w);
            // Rigid: orthonormal rows, nothing in the last row but (0 0 0 1).
            bool rigid = std::fabs(w[12]) < 0.02f && std::fabs(w[13]) < 0.02f && std::fabs(w[14]) < 0.02f &&
                         std::fabs(w[15] - 1.0f) < 0.02f;
            for (int r = 0; r < 3 && rigid; ++r)
                rigid = std::fabs(Length3(w + r * 4) - 1.0f) < 0.05f;
            if (!rigid)
                continue;
            const float at[3] = {w[3], w[7], w[11]};
            ++found;
            Log("Effects:   +%zu%s is the view-projection times a placement at %.3f %.3f %.3f, %.2f m from the muzzle",
                k * 4, transposed ? " (columns)" : "", at[0], at[1], at[2], Distance3(at, muzzle));
        }
    }
    if (!found)
        Log("Effects:   no world-view-projection in the block");
}

// weapon.txt "dumpfx 234" on an instanced draw (the casings: one instance each): its vertex streams,
// and on the immediate context each stream's first two instances read back (a stall: probe only).
// The casings' own block held no placement (2026-10-30): an instance stream may.
std::atomic<int> g_dumpInstancesLeft{0};

void DumpInstances(ID3D11DeviceContext* ctx, UINT count, UINT instances, UINT first, bool) {
    // The flash's 36-index mesh only, not the world's other boxes ("dumpfx 36", 2026-10-31).
    if (count != g_dumpFx.load(std::memory_order_relaxed) ||
        g_dumpInstancesLeft.fetch_sub(1) <= 0)
        return;
    // Read back only on the immediate context (a deferred one can't be read; both hooks may be one).
    const bool immediate = ctx->GetType() == D3D11_DEVICE_CONTEXT_IMMEDIATE;
    ID3D11Buffer* vbs[4] = {};
    UINT strides[4] = {}, offsets[4] = {};
    ctx->IAGetVertexBuffers(0, 4, vbs, strides, offsets);
    Log("Effects: instanced %u-count draw, %u instances from %u, %s context; streams (stride/offset): %u/%u %u/%u %u/%u "
        "%u/%u",
        count, instances, first, immediate ? "the immediate" : "a deferred", strides[0], offsets[0], strides[1],
        offsets[1], strides[2], offsets[2], strides[3], offsets[3]);
    for (int s = 1; s < 4 && immediate; ++s) {
        if (!vbs[s] || !strides[s] || strides[s] > 128)
            continue;
        float data[64] = {};
        const UINT bytes = std::min(2 * strides[s], static_cast<UINT>(sizeof(data)));
        if (!ReadBack(ctx, vbs[s], offsets[s] + first * strides[s], bytes, data))
            continue;
        char line[900];
        int used = 0;
        for (UINT i = 0; i < bytes / 4 && used < 860; ++i)
            used += snprintf(line + used, sizeof(line) - used, " %.4g", data[i]);
        Log("Effects:   stream %d's first instances:%s", s, line);
    }
    for (ID3D11Buffer* b : vbs)
        if (b)
            b->Release();
}

std::atomic<bool> g_census{false};
std::mutex g_censusMutex;
std::unordered_map<Kind, Tally, KindHash> g_tally;
ULONGLONG g_censusStart = 0;
uint32_t g_framesBefore = 0, g_framesAfter = 0;
std::atomic<uint32_t> g_censusFrame{0};

void Describe(ID3D11DeviceContext* ctx, ID3D11DepthStencilState* depth, Tally& t) {
    t.described = true;
    ctx->IAGetPrimitiveTopology(&t.topology);
    ID3D11Buffer* vb = nullptr;
    UINT offset = 0;
    ctx->IAGetVertexBuffers(0, 1, &vb, &t.stride, &offset);
    if (vb)
        vb->Release();
    ID3D11BlendState* blend = nullptr;
    FLOAT factor[4];
    UINT mask = 0;
    ctx->OMGetBlendState(&blend, factor, &mask);
    if (blend) {
        D3D11_BLEND_DESC d;
        blend->GetDesc(&d);
        t.blend = d.RenderTarget[0].BlendEnable != FALSE;
        t.src = d.RenderTarget[0].SrcBlend;
        t.dst = d.RenderTarget[0].DestBlend;
        blend->Release();
    }
    if (depth) {
        D3D11_DEPTH_STENCIL_DESC d;
        depth->GetDesc(&d);
        t.depthTest = d.DepthEnable != FALSE;
        t.depthWrite = d.DepthWriteMask == D3D11_DEPTH_WRITE_MASK_ALL;
    }
    ID3D11Buffer* cb[8] = {};
    ctx->VSGetConstantBuffers(0, 8, cb);
    int used = 0;
    for (int i = 0; i < 8; ++i) {
        if (!cb[i])
            continue;
        D3D11_BUFFER_DESC d;
        cb[i]->GetDesc(&d);
        int view = skin::ViewBufferKind(cb[i]);
        used += snprintf(t.buffers + used, sizeof(t.buffers) - used, "%d:%u%s ", i, d.ByteWidth,
                         view == 1 ? "a" : view == 2 ? "w" : "");
        cb[i]->Release();
        if (used >= static_cast<int>(sizeof(t.buffers)) - 1)
            break;
    }
}

// ---- Frame tail ----
// weapon.txt "tail": one frame's draws to targets the window's size, in order, with their shaders,
// blending and viewport. The HUD (and with it the screen's blood and suppression) is drawn into the
// right eye's half only (2026-10-30); this tells its draws from the rest.
std::atomic<int> g_tailState{0};  // 0 off, 1 from the next frame, 2 recording this one
struct TailDraw {
    DrawType type;
    UINT count, instances;
    const void *ctx, *vs, *ps, *target;
    UINT width, height, format, stencil;
    D3D11_VIEWPORT viewport;
    bool blend;
    D3D11_BLEND src, dst;
    // The depth-stencil view (its size, 0 without one) and tests, and the scissor: the ammo counter
    // drew in the right eye only (2026-10-30).
    UINT depthWidth, depthHeight;
    bool depthTest, depthWrite, stencilTest, scissorOn;
    D3D11_COMPARISON_FUNC depthFunc, stencilFunc;
    D3D11_STENCIL_OP stencilPass;
    D3D11_RECT scissor;
    UINT texture0;  // the first pixel-shader texture's width (0 none)
    uint64_t psSum;  // the pixel shader's bytecode checksum (shaders.cpp; the same every start)
    const void* depthTarget;  // the depth-stencil view's texture (not referenced; compared only)
};
std::mutex g_tailMutex;
std::vector<TailDraw> g_tail;
UINT g_tailAll = 0;  // every draw of the frame
// A depth-stencil clear or another module's note, before the draw recorded at at.
struct TailEvent {
    size_t at;
    char text[200];
};
std::vector<TailEvent> g_tailEvents;

// An eye's picture going onto the window (unblended, a target twice as wide as high): its pixel
// shader's constants (the first four buffers, 64 floats each at most), read back through a staging
// copy, logged with the tail. The suppression blur stayed in the right eye (2026-10-30): if it is
// a constant of this pass, the two eyes' values differ while suppressed.
void LogPassConstants(ID3D11DeviceContext* ctx, size_t index) {
    ID3D11Buffer* buffers[4] = {};
    ctx->PSGetConstantBuffers(0, 4, buffers);
    for (int slot = 0; slot < 4; ++slot) {
        ID3D11Buffer* buffer = buffers[slot];
        if (!buffer)
            continue;
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
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (device && SUCCEEDED(device->CreateBuffer(&staging, nullptr, &copy))) {
            ctx->CopyResource(copy, buffer);
            if (SUCCEEDED(ctx->Map(copy, 0, D3D11_MAP_READ, 0, &mapped))) {
                const auto* f = static_cast<const float*>(mapped.pData);
                const UINT count = std::min<UINT>(desc.ByteWidth / 4, 64);
                char line[1400];
                int used = 0;
                for (UINT i = 0; i < count && used < 1360; ++i)
                    used += snprintf(line + used, sizeof(line) - used, " %g", f[i]);
                Log("Tail %zu: PS constants %d (%u bytes):%s", index, slot, desc.ByteWidth, line);
                ctx->Unmap(copy, 0);
            }
            copy->Release();
        }
        if (device)
            device->Release();
        buffer->Release();
    }
}

void Tail(ID3D11DeviceContext* ctx, DrawType type, UINT count, UINT instances) {
    TailDraw d{type, count, instances, ctx};
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11DepthStencilView* dsv = nullptr;
    ctx->OMGetRenderTargets(1, &rtv, &dsv);
    if (dsv) {
        ID3D11Resource* resource = nullptr;
        dsv->GetResource(&resource);
        d.depthTarget = resource;
        ID3D11Texture2D* texture = nullptr;
        if (resource && SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&texture)))) {
            D3D11_TEXTURE2D_DESC desc;
            texture->GetDesc(&desc);
            d.depthWidth = desc.Width, d.depthHeight = desc.Height;
            texture->Release();
        }
        if (resource)
            resource->Release();
        dsv->Release();
    }
    if (rtv) {
        ID3D11Resource* resource = nullptr;
        rtv->GetResource(&resource);
        if (resource) {
            d.target = resource;
            ID3D11Texture2D* texture = nullptr;
            if (SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&texture)))) {
                D3D11_TEXTURE2D_DESC desc;
                texture->GetDesc(&desc);
                d.width = desc.Width, d.height = desc.Height, d.format = desc.Format;
                texture->Release();
            }
            resource->Release();
        }
        rtv->Release();
    }
    {
        std::lock_guard lock(g_tailMutex);
        ++g_tailAll;
    }
    if (d.width < 1000 || g_tail.size() >= 20000)
        return;
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11DepthStencilState* depth = nullptr;
    ctx->VSGetShader(&vs, nullptr, nullptr);
    ctx->PSGetShader(&ps, nullptr, nullptr);
    ctx->OMGetDepthStencilState(&depth, &d.stencil);
    d.vs = vs, d.ps = ps;
    d.psSum = shaders::ChecksumOf(ps);
    if (vs)
        vs->Release();
    if (ps)
        ps->Release();
    if (depth) {
        D3D11_DEPTH_STENCIL_DESC desc;
        depth->GetDesc(&desc);
        d.depthTest = desc.DepthEnable != FALSE;
        d.depthWrite = desc.DepthWriteMask != D3D11_DEPTH_WRITE_MASK_ZERO;
        d.depthFunc = desc.DepthFunc;
        d.stencilTest = desc.StencilEnable != FALSE;
        d.stencilFunc = desc.FrontFace.StencilFunc;
        d.stencilPass = desc.FrontFace.StencilPassOp;
        depth->Release();
    }
    ID3D11RasterizerState* raster = nullptr;
    ctx->RSGetState(&raster);
    if (raster) {
        D3D11_RASTERIZER_DESC desc;
        raster->GetDesc(&desc);
        d.scissorOn = desc.ScissorEnable != FALSE;
        raster->Release();
    }
    UINT scissors = 1;
    ctx->RSGetScissorRects(&scissors, &d.scissor);
    if (!scissors)
        d.scissor = {};
    ID3D11ShaderResourceView* srv = nullptr;
    ctx->PSGetShaderResources(0, 1, &srv);
    if (srv) {
        ID3D11Resource* resource = nullptr;
        srv->GetResource(&resource);
        ID3D11Texture2D* texture = nullptr;
        if (resource && SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&texture)))) {
            D3D11_TEXTURE2D_DESC desc;
            texture->GetDesc(&desc);
            d.texture0 = desc.Width;
            texture->Release();
        }
        if (resource)
            resource->Release();
        srv->Release();
    }
    UINT viewports = 1;
    ctx->RSGetViewports(&viewports, &d.viewport);
    if (!viewports)
        d.viewport = {};
    ID3D11BlendState* blend = nullptr;
    FLOAT factor[4];
    UINT mask = 0;
    ctx->OMGetBlendState(&blend, factor, &mask);
    d.src = D3D11_BLEND_ONE, d.dst = D3D11_BLEND_ZERO;
    if (blend) {
        D3D11_BLEND_DESC desc;
        blend->GetDesc(&desc);
        d.blend = desc.RenderTarget[0].BlendEnable != FALSE;
        d.src = desc.RenderTarget[0].SrcBlend, d.dst = desc.RenderTarget[0].DestBlend;
        blend->Release();
    }
    size_t index;
    {
        std::lock_guard lock(g_tailMutex);
        index = g_tail.size();
        g_tail.push_back(d);
    }
    if (!d.blend && d.width == 2 * d.height)
        LogPassConstants(ctx, index);
}

const char* BlendName(D3D11_BLEND b);

void LogTail() {
    std::lock_guard lock(g_tailMutex);
    Log("Tail: %u draws in the frame, %zu of them to targets 1000 or more wide; in runs to one target:", g_tailAll,
        g_tail.size());
    // Runs: the same target, viewport and blending in a row, with their first pixel shaders.
    for (size_t i = 0; i < g_tail.size();) {
        const TailDraw& a = g_tail[i];
        size_t j = i + 1;
        while (j < g_tail.size() && g_tail[j].target == a.target && g_tail[j].blend == a.blend &&
               g_tail[j].viewport.TopLeftX == a.viewport.TopLeftX && g_tail[j].viewport.Width == a.viewport.Width)
            ++j;
        char shaders[200] = "";
        int used = 0;
        for (size_t k = i; k < j && used < 180; ++k) {
            bool seen = false;
            for (size_t m = i; m < k && !seen; ++m)
                seen = g_tail[m].ps == g_tail[k].ps;
            if (!seen)
                used += snprintf(shaders + used, sizeof(shaders) - used, "%p ", g_tail[k].ps);
        }
        Log("Tail run %zu-%zu (%zu): target %p %ux%u format %u, viewport %.0f,%.0f %.0fx%.0f, blend %s; PS %s", i, j - 1,
            j - i, a.target, a.width, a.height, a.format, a.viewport.TopLeftX, a.viewport.TopLeftY, a.viewport.Width,
            a.viewport.Height, a.blend ? "on" : "off", shaders);
        i = j;
    }
    // The last of them one by one.
    for (size_t i = g_tail.size() > 150 ? g_tail.size() - 150 : 0; i < g_tail.size(); ++i) {
        const TailDraw& d = g_tail[i];
        Log("Tail %zu: %s %u x%u, context %p, VS %p PS %p (%016llx), target %p %ux%u format %u, viewport %.0f,%.0f "
            "%.0fx%.0f, blend %s (%s, %s), stencil %u; depth view %ux%u, depth %s%s func %d, stencil test %s func %d "
            "pass %d, scissor %s %ld,%ld-%ld,%ld, texture 0 %u wide",
            i, kTypeNames[d.type], d.count, d.instances, d.ctx, d.vs, d.ps, static_cast<unsigned long long>(d.psSum),
            d.target, d.width, d.height, d.format,
            d.viewport.TopLeftX, d.viewport.TopLeftY, d.viewport.Width, d.viewport.Height, d.blend ? "on" : "off",
            BlendName(d.src), BlendName(d.dst), d.stencil, d.depthWidth, d.depthHeight, d.depthTest ? "test" : "off",
            d.depthWrite ? "+write" : "", static_cast<int>(d.depthFunc), d.stencilTest ? "on" : "off",
            static_cast<int>(d.stencilFunc), static_cast<int>(d.stencilPass), d.scissorOn ? "on" : "off",
            d.scissor.left, d.scissor.top, d.scissor.right, d.scissor.bottom, d.texture0);
    }
    // In order: the clears and notes among the first-person draws (stencil 32), the eyes' pictures (unblended, a
    // target twice as wide as high) and each draw that changes its context's depth-stencil buffer.
    Log("Tail order: %zu events among %zu draws", g_tailEvents.size(), g_tail.size());
    size_t e = 0;
    std::vector<std::pair<const void*, const void*>> lastDepth;  // context, its last depth-stencil buffer
    for (size_t i = 0; i <= g_tail.size(); ++i) {
        for (; e < g_tailEvents.size() && g_tailEvents[e].at <= i; ++e)
            Log("Tail order %zu: %s", i, g_tailEvents[e].text);
        if (i == g_tail.size())
            break;
        const TailDraw& d = g_tail[i];
        bool newDepth = true, seenContext = false;
        for (auto& last : lastDepth)
            if (last.first == d.ctx) {
                seenContext = true;
                newDepth = last.second != d.depthTarget;
                last.second = d.depthTarget;
            }
        if (!seenContext)
            lastDepth.push_back({d.ctx, d.depthTarget});
        const bool picture = !d.blend && d.width == 2 * d.height;
        if (!(d.stencil & 32) && !picture && !newDepth)
            continue;
        Log("Tail order %zu: draw %u, context %p, target %p %ux%u, depth-stencil %p (%ux%u), stencil %u; depth %s%s func "
            "%d, stencil test %s func %d pass %d; viewport %.0f,%.0f %.0fx%.0f; PS %p%s%s",
            i, d.count, d.ctx, d.target, d.width, d.height, d.depthTarget, d.depthWidth, d.depthHeight, d.stencil,
            d.depthTest ? "test" : "off", d.depthWrite ? "+write" : "", static_cast<int>(d.depthFunc),
            d.stencilTest ? "on" : "off", static_cast<int>(d.stencilFunc), static_cast<int>(d.stencilPass),
            d.viewport.TopLeftX, d.viewport.TopLeftY, d.viewport.Width, d.viewport.Height, d.ps,
            picture ? " (an eye's picture)" : "", newDepth ? " (its context's depth-stencil changes)" : "");
    }
    g_tailEvents.clear();
    g_tail.clear();
}

void ProbeDraw(ID3D11DeviceContext* ctx, DrawType type, UINT count, UINT instances);
thread_local UINT t_start = 0;  // the indexed draw being noted: its first index (the probe logs it)

// An eye's picture: six vertices, unblended, onto a target the window's size and twice as wide as high (the tail of a
// drive's frame: one at the end of each eye's pass).
bool IsEyePicture(ID3D11DeviceContext* ctx) {
    ID3D11RenderTargetView* rtv = nullptr;
    ctx->OMGetRenderTargets(1, &rtv, nullptr);
    bool window = false;
    if (rtv) {
        ID3D11Resource* resource = nullptr;
        rtv->GetResource(&resource);
        ID3D11Texture2D* texture = nullptr;
        if (resource && SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&texture)))) {
            D3D11_TEXTURE2D_DESC desc;
            texture->GetDesc(&desc);
            window = desc.Width >= 2000 && desc.Width == 2 * desc.Height;
            texture->Release();
        }
        if (resource)
            resource->Release();
        rtv->Release();
    }
    if (!window)
        return false;
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
    return !blended;
}

// ---- Everything that comes out of the gun: a survey ----
// weapon.txt "flashsurvey 3000": every kind of draw, of any kind at all, for a moment now (the baseline) and for that many
// ms from the next press of the fire button, with whatever it places near the game's gun: its vertices if they're in a
// dynamic buffer (read where the game wrote them, so nothing stalls and the flash's frames aren't missed), its instances, the
// first matrix of its constant block in the world or the arms' space. One line a kind when it's over: what isn't moved with
// the gun yet, smoke or anything else, shows here. For finding things only; nothing runs while it's off.
struct SurveyRow {
    const char* type = "";
    UINT minCount = UINT_MAX, maxCount = 0, maxInstances = 0, stride = 0, stencil = 0;
    bool blended = false, dynamicBuffer = false;
    uint32_t before = 0, after = 0, nearVertices = 0, beyondQuads = 0;
    float nearest = 1e9f, local[3] = {}, instance[3] = {};
    const char* source = "";
    int firstMs = -1, lastMs = -1;
};
std::mutex g_surveyMutex;
std::map<std::tuple<int, uint64_t, uint64_t, UINT, UINT>, SurveyRow> g_survey;
std::atomic<int> g_surveyPhase{0};  // 0 off, 1 the baseline, 2 waiting for the press, 3 after it
std::atomic<ULONGLONG> g_surveyUntil{0}, g_surveyFrom{0};
std::atomic<int> g_surveyMs{3000};
struct MappedBuffer {
    ID3D11Resource* resource;
    const uint8_t* data;
};
MappedBuffer g_mapped[64] = {};  // under g_surveyMutex

const uint8_t* MappedData(ID3D11Resource* resource) {
    std::lock_guard lock(g_surveyMutex);
    for (const MappedBuffer& m : g_mapped)
        if (m.resource == resource)
            return m.data;
    return nullptr;
}

bool Blended(ID3D11DeviceContext* ctx);
thread_local INT t_base = 0;
thread_local UINT t_first = 0;  // the instanced draw being noted: its first instance

void SurveyDraw(ID3D11DeviceContext* ctx, DrawType type, UINT count, UINT instances) {
    const int phase = g_surveyPhase.load(std::memory_order_relaxed);
    if (phase != 1 && phase != 3)
        return;
    float muzzle[3], port[3], body[12];
    if (!skin::GameGunMuzzle(muzzle) || !skin::GameGunPort(port) || !skin::GameGunBody(body))
        return;
    float gunWorld[3], gunArms[3];
    if (!skin::GameGunPlaces(gunWorld, gunArms))
        return;
    const float origin[3] = {gunWorld[0] - gunArms[0], gunWorld[1] - gunArms[1], gunWorld[2] - gunArms[2]};
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11DepthStencilState* depth = nullptr;
    UINT ref = 0;
    ctx->VSGetShader(&vs, nullptr, nullptr);
    ctx->PSGetShader(&ps, nullptr, nullptr);
    ctx->OMGetDepthStencilState(&depth, &ref);
    const uint64_t vsSum = shaders::ChecksumOfVs(vs), psSum = shaders::ChecksumOf(ps);
    if (vs)
        vs->Release();
    if (ps)
        ps->Release();
    if (depth)
        depth->Release();
    ID3D11Buffer* streams[2] = {};
    UINT strides[2] = {}, offsets[2] = {};
    ctx->IAGetVertexBuffers(0, 2, streams, strides, offsets);
    bool dynamicBuffer = false;
    if (streams[0]) {
        D3D11_BUFFER_DESC d;
        streams[0]->GetDesc(&d);
        dynamicBuffer = d.Usage == D3D11_USAGE_DYNAMIC;
    }
    float nearest = 1e9f, local[3] = {};
    const char* source = "";
    uint32_t nearVertices = 0, beyondQuads = 0;
    // The first instance whose place is read, as it came, for telling a stale read from a real one.
    float firstInstance[3] = {};
    bool haveInstance = false;
    auto note = [&](float x, float y, float z, const char* from) {
        const float a[3] = {x - muzzle[0], y - muzzle[1], z - muzzle[2]};
        const float b[3] = {x - port[0], y - port[1], z - port[2]};
        const float da = std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]), db = std::sqrt(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]);
        const float d = std::min(da, db);
        if (!(d < 1e8f))
            return;
        if (d < 1.6f)
            ++nearVertices;
        if (d < nearest) {
            nearest = d, source = from;
            const float rel[3] = {x - body[3], y - body[7], z - body[11]};  // GameGunBody's place is in the world
            for (int k = 0; k < 3; ++k) {
                const float axis[3] = {body[k], body[4 + k], body[8 + k]};
                const float len2 = axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2];
                local[k] = (rel[0] * axis[0] + rel[1] * axis[1] + rel[2] * axis[2]) / (len2 > 0.0f ? len2 : 1.0f);
            }
        }
    };
    // Its vertices, if they're in a buffer the game writes each frame.
    const bool indexed = type == kIndexed || type == kIndexedInstanced;
    if (streams[0] && dynamicBuffer && strides[0] >= 12 && strides[0] <= 256) {
        if (const uint8_t* data = MappedData(streams[0])) {
            const UINT stride = strides[0];
            const UINT first = indexed ? static_cast<UINT>(std::max(t_base, 0)) : t_start;
            const UINT vertices = std::min(indexed ? (count / 6 * 4 + (count % 6)) : count, 8192u);
            for (UINT v = 0; v < vertices; ++v) {
                float p[3];
                if (!camera::SafeRead(reinterpret_cast<uintptr_t>(data) + offsets[0] + (first + v) * stride, p, sizeof(p)))
                    break;
                note(p[0], p[1], p[2], "vertices");
            }
            // The particles' quads near the muzzle: how many, how big, how far along the barrel, ten times a second.
            if (indexed && stride == kQuadStride && phase == 3) {
                float along = 0.0f, sideways = 0.0f, biggest = 0.0f, rising = 0.0f;
                float biggestAt[3] = {};
                int quadsNear = 0;
                for (UINT q = 0; q < std::min(count / 6, 2048u); ++q) {
                    float c[3] = {}, first3[3] = {};
                    bool ok = true;
                    for (UINT v = 0; v < 4 && ok; ++v) {
                        float p[3];
                        ok = camera::SafeRead(reinterpret_cast<uintptr_t>(data) + offsets[0] + (first + q * 4 + v) * stride, p, sizeof(p));
                        for (int k = 0; ok && k < 3; ++k) {
                            c[k] += p[k] / 4.0f;
                            if (v == 0)
                                first3[k] = p[k];
                        }
                    }
                    if (!ok)
                        break;
                    const float rel[3] = {c[0] - muzzle[0], c[1] - muzzle[1], c[2] - muzzle[2]};
                    const float half[3] = {first3[0] - c[0], first3[1] - c[1], first3[2] - c[2]};
                    float axisLocal[3];
                    for (int k = 0; k < 3; ++k) {
                        const float axis[3] = {body[k], body[4 + k], body[8 + k]};
                        const float len2 = axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2];
                        axisLocal[k] = (rel[0] * axis[0] + rel[1] * axis[1] + rel[2] * axis[2]) / (len2 > 0.0f ? len2 : 1.0f);
                    }
                    if (rel[0] * rel[0] + rel[1] * rel[1] + rel[2] * rel[2] > 2.56f)
                        continue;
                    ++quadsNear;
                    along += axisLocal[2], sideways += std::fabs(axisLocal[0]), rising += axisLocal[1];
                    const float size = std::sqrt(half[0] * half[0] + half[1] * half[1] + half[2] * half[2]);
                    if (size > biggest) {
                        biggest = size;
                        std::memcpy(biggestAt, c, sizeof(c));
                    }

                }
                static std::atomic<ULONGLONG> s_lastQuads{0};
                const ULONGLONG tick = GetTickCount64();
                // The kinds that only come with firing each time, the rest ten times a second.
                bool onlyFiring = false;
                {
                    std::lock_guard lock(g_surveyMutex);
                    const auto it = g_survey.find({static_cast<int>(type), vsSum, psSum, strides[0], ref});
                    onlyFiring = it == g_survey.end() || it->second.before == 0;
                }
                if (quadsNear && (onlyFiring || tick - s_lastQuads.load(std::memory_order_relaxed) >= 100)) {
                    if (!onlyFiring)
                        s_lastQuads = tick;
                    float was[2] = {}, now[2] = {}, moved[3] = {}, d[16];
                    ID3D11Resource* resource = nullptr;
                    if (const uint8_t* view = skin::LatestViewConstants(resource); view && skin::GunDeltaWorld(d)) {
                        const float* eye = reinterpret_cast<const float*>(view + 320);
                        const float* vp = reinterpret_cast<const float*>(view + 240);
                        auto project = [&](const float* at, float* out) {
                            const float rel[3] = {at[0] - eye[0], at[1] - eye[1], at[2] - eye[2]};
                            const float w = vp[12] * rel[0] + vp[13] * rel[1] + vp[14] * rel[2] + vp[15];
                            out[0] = (vp[0] * rel[0] + vp[1] * rel[1] + vp[2] * rel[2] + vp[3]) / w;
                            out[1] = (vp[4] * rel[0] + vp[5] * rel[1] + vp[6] * rel[2] + vp[7]) / w;
                        };
                        for (int r = 0; r < 3; ++r)
                            moved[r] = d[r * 4] * biggestAt[0] + d[r * 4 + 1] * biggestAt[1] + d[r * 4 + 2] * biggestAt[2] + d[r * 4 + 3];
                        project(biggestAt, was);
                        project(moved, now);
                    }
                    Log("Quads +%d ms: PS %016llx %d near the muzzle of %u; on average %+.2f along the barrel from the muzzle, %.2f sideways, %+.2f up; the biggest half %.2f m, it shows at %.3f %.3f, moved all the way %.3f %.3f (the game's muzzle at %.2f %.2f %.2f, it at %.2f %.2f %.2f)",
                        static_cast<int>(tick - g_surveyFrom.load(std::memory_order_relaxed)), static_cast<unsigned long long>(psSum), quadsNear, count / 6,
                        along / quadsNear, sideways / quadsNear, rising / quadsNear, biggest, was[0], was[1], now[0], now[1], muzzle[0], muzzle[1], muzzle[2],
                        biggestAt[0], biggestAt[1], biggestAt[2]);
                }
            }
            // Whether its indices stay within the quads taken for it (DrawQuadsMoved copies only those).
            if (indexed && stride == kQuadStride && count % 6 == 0) {
                ID3D11Buffer* ib = nullptr;
                DXGI_FORMAT format = DXGI_FORMAT_R16_UINT;
                UINT ibOffset = 0;
                ctx->IAGetIndexBuffer(&ib, &format, &ibOffset);
                if (ib) {
                    if (const uint8_t* indices = MappedData(ib)) {
                        const UINT size = format == DXGI_FORMAT_R32_UINT ? 4 : 2;
                        for (UINT i = 0; i < std::min(count, 24576u); ++i) {
                            UINT index = 0;
                            if (!camera::SafeRead(reinterpret_cast<uintptr_t>(indices) + ibOffset + (t_start + i) * size, &index, size))
                                break;
                            if (index >= count / 6 * 4)
                                ++beyondQuads;
                        }
                    }
                    ib->Release();
                }
            }
        }
    }
    // Its instances.
    if ((type == kIndexedInstanced || type == kInstanced) && streams[1] && strides[1] >= 48) {
        const uint8_t* data = MappedData(streams[1]);
        for (UINT i = 0; i < std::min(instances, 256u); ++i) {
            float m[12];
            bool read = false;
            if (data)
                read = camera::SafeRead(reinterpret_cast<uintptr_t>(data) + offsets[1] + (t_first + i) * strides[1], m, sizeof(m));
            else if (i < 8 && phase == 3)
                read = ReadBack(ctx, streams[1], offsets[1] + (t_first + i) * strides[1], sizeof(m), m);
            if (!read)
                break;
            if (!haveInstance) {
                haveInstance = true;
                firstInstance[0] = m[3], firstInstance[1] = m[7], firstInstance[2] = m[11];
            }
            note(m[3], m[7], m[11], "instance");
        }
    }
    // The first matrix of its block, in the world or the arms' space.
    ID3D11Buffer* block = nullptr;
    ctx->VSGetConstantBuffers(0, 1, &block);
    if (block) {
        unsigned int size = 0;
        uint32_t serial = 0;
        if (const uint8_t* bytes = skin::LatestBlockOf(block, size, serial); bytes && size >= 48) {
            const float* m = reinterpret_cast<const float*>(bytes);
            note(m[3], m[7], m[11], "block");
            note(m[3] + origin[0], m[7] + origin[1], m[11] + origin[2], "block, arms");
        }
        block->Release();
    }
    // A sprite in clip space that only comes with firing: where it is against the game's muzzle and the drawn one, in this
    // draw's own view.
    bool firingSprite = false;
    if (phase == 3 && type == kPlain && count == 6 && streams[0] && dynamicBuffer && strides[0] == 24) {
        std::lock_guard lock(g_surveyMutex);
        const auto it = g_survey.find({static_cast<int>(type), vsSum, psSum, strides[0], ref});
        firingSprite = it == g_survey.end() || it->second.before == 0;
    }
    if (firingSprite) {
        if (const uint8_t* data = MappedData(streams[0])) {
            float v[6 * 6];
            if (camera::SafeRead(reinterpret_cast<uintptr_t>(data) + offsets[0] + t_start * 24, v, sizeof(v))) {
                ID3D11Resource* resource = nullptr;
                float d[16];
                if (const uint8_t* view = skin::LatestViewConstants(resource); view && skin::GunDeltaWorld(d)) {
                    const float* eye = reinterpret_cast<const float*>(view + 320);
                    const float* vp = reinterpret_cast<const float*>(view + 240);
                    auto project = [&](const float* at, float* out) {
                        const float rel[3] = {at[0] - eye[0], at[1] - eye[1], at[2] - eye[2]};
                        const float w = vp[12] * rel[0] + vp[13] * rel[1] + vp[14] * rel[2] + vp[15];
                        out[0] = (vp[0] * rel[0] + vp[1] * rel[1] + vp[2] * rel[2] + vp[3]) / w;
                        out[1] = (vp[4] * rel[0] + vp[5] * rel[1] + vp[6] * rel[2] + vp[7]) / w;
                    };
                    float moved[3], game2[2], drawn2[2];
                    for (int r = 0; r < 3; ++r)
                        moved[r] = d[r * 4] * muzzle[0] + d[r * 4 + 1] * muzzle[1] + d[r * 4 + 2] * muzzle[2] + d[r * 4 + 3];
                    project(muzzle, game2);
                    project(moved, drawn2);
                    float lo[2] = {1e9f, 1e9f}, hi[2] = {-1e9f, -1e9f};
                    for (int c = 0; c < 6; ++c)
                        for (int k = 0; k < 2; ++k)
                            lo[k] = std::min(lo[k], v[c * 6 + k]), hi[k] = std::max(hi[k], v[c * 6 + k]);
                    static std::atomic<int> s_sprites{0};
                    if (s_sprites.fetch_add(1) < 400)
                        Log("Sprite +%d ms: VS %016llx PS %016llx: x %.3f to %.3f, y %.3f to %.3f, third %.3f | fields %.3f %.3f %.3f | the game's muzzle shows at %.3f %.3f, the drawn one at %.3f %.3f",
                            static_cast<int>(GetTickCount64() - g_surveyFrom.load(std::memory_order_relaxed)), static_cast<unsigned long long>(vsSum),
                            static_cast<unsigned long long>(psSum), lo[0], hi[0], lo[1], hi[1], v[2], v[3], v[4], v[5], game2[0], game2[1], drawn2[0], drawn2[1]);
                }
            }
        }
    }
    for (ID3D11Buffer* s : streams)
        if (s)
            s->Release();
    const bool blended = Blended(ctx);
    const int ms = static_cast<int>(GetTickCount64() - g_surveyFrom.load(std::memory_order_relaxed));
    std::lock_guard lock(g_surveyMutex);
    SurveyRow& r = g_survey[{static_cast<int>(type), vsSum, psSum, strides[0], ref}];
    r.type = kTypeNames[type], r.stride = strides[0], r.stencil = ref, r.blended = blended, r.dynamicBuffer = dynamicBuffer;
    r.minCount = std::min(r.minCount, count), r.maxCount = std::max(r.maxCount, count), r.maxInstances = std::max(r.maxInstances, instances);
    ++(phase == 1 ? r.before : r.after);
    if (phase == 3) {
        r.nearVertices += nearVertices, r.beyondQuads += beyondQuads;
        if (nearest < 3.0f) {
            if (r.firstMs < 0)
                r.firstMs = ms;
            r.lastMs = ms;
        }
        if (nearest < r.nearest) {
            r.nearest = nearest, r.source = source;
            std::memcpy(r.local, local, sizeof(local));
            if (haveInstance)
                std::memcpy(r.instance, firstInstance, sizeof(firstInstance));
        }
    }
}

void SurveyReport() {
    std::lock_guard lock(g_surveyMutex);
    std::vector<std::pair<float, std::string>> lines;
    for (const auto& [key, r] : g_survey) {
        if (r.after == 0 || r.nearest >= 3.0f)
            continue;
        char line[420];
        snprintf(line, sizeof(line),
                 "Survey: %.2f m (%s), on the gun %+.2f %+.2f %+.2f | %s | %s c%u-%u x%u stride %u stencil %u %s%s VS %016llx PS %016llx | "
                 "%u draws, %u before; %u vertices or matrices near; +%d to +%d ms; instance at %.2f %.2f %.2f%s",
                 r.nearest, r.source, r.local[0], r.local[1], r.local[2], r.before ? "there before" : "NEW", r.type, r.minCount, r.maxCount,
                 r.maxInstances, r.stride, r.stencil, r.blended ? "blended" : "opaque", r.dynamicBuffer ? " dynamic" : "",
                 static_cast<unsigned long long>(std::get<1>(key)), static_cast<unsigned long long>(std::get<2>(key)), r.after, r.before,
                 r.nearVertices, r.firstMs, r.lastMs, r.instance[0], r.instance[1], r.instance[2], r.beyondQuads ? " INDICES BEYOND ITS QUADS" : "");
        lines.push_back({r.nearest, line});
    }
    std::sort(lines.begin(), lines.end());
    Log("Survey: %zu kinds of draw, %zu of them placing something within 3 m of the game's gun", g_survey.size(), lines.size());
    for (const auto& [d, line] : lines)
        Log("%s", line.c_str());
    g_survey.clear();
}

// weapon.txt "flashscan": for 250 ms after the next shot, every blended draw with what's drawn with it and its first
// vertex, read back (a stall: for finding things only).
std::atomic<bool> g_scanArmed{false}, g_burstArmed{false};

void ScanDraw(ID3D11DeviceContext* ctx, DrawType type, UINT count, UINT instances) {
    const bool base = GetTickCount64() <= g_scanBaseUntil.load(std::memory_order_relaxed);
    if (!base && GetTickCount64() > g_scanUntil.load(std::memory_order_relaxed))
        return;
    if (!Blended(ctx) && type != kIndexedInstanced && type != kInstanced)
        return;
    static std::atomic<int> s_lines{0};
    if (s_lines.fetch_add(1) > 12000)
        return;
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11DepthStencilState* depth = nullptr;
    UINT ref = 0;
    ctx->VSGetShader(&vs, nullptr, nullptr);
    ctx->PSGetShader(&ps, nullptr, nullptr);
    ctx->OMGetDepthStencilState(&depth, &ref);
    const auto vsSum = static_cast<unsigned long long>(shaders::ChecksumOfVs(vs));
    const auto psSum = static_cast<unsigned long long>(shaders::ChecksumOf(ps));
    if (vs)
        vs->Release();
    if (ps)
        ps->Release();
    if (depth)
        depth->Release();
    ID3D11Buffer* vb = nullptr;
    UINT stride = 0, offset = 0;
    ctx->IAGetVertexBuffers(0, 1, &vb, &stride, &offset);
    UINT vertex = 0;
    const bool indexed = type == kIndexed || type == kIndexedInstanced;
    if (indexed) {
        ID3D11Buffer* ib = nullptr;
        DXGI_FORMAT format = DXGI_FORMAT_R16_UINT;
        UINT ibOffset = 0;
        ctx->IAGetIndexBuffer(&ib, &format, &ibOffset);
        if (ib) {
            const UINT size = format == DXGI_FORMAT_R32_UINT ? 4 : 2;
            UINT index = 0;
            ReadBack(ctx, ib, ibOffset + t_start * size, size, &index);
            vertex = static_cast<UINT>(static_cast<INT>(index) + t_base);
            ib->Release();
        }
    }
    float f[32] = {};
    if (vb) {
        if (stride >= 12 && stride <= sizeof(f))
            ReadBack(ctx, vb, offset + vertex * stride, stride, f);
        vb->Release();
    }
    float gunWorld[3] = {}, gunArms[3] = {};
    const bool gun = skin::GameGunPlaces(gunWorld, gunArms);
    // Which view it's drawn in, the eye there, and what else may place it: cb0's first matrix, the instance stream's.
    ID3D11Buffer* cbs[3] = {};
    ctx->VSGetConstantBuffers(0, 3, cbs);
    float eye[3] = {}, block[12] = {}, instance[12] = {};
    int viewKind = 0;
    UINT blockSize = 0;
    if (cbs[2]) {
        viewKind = skin::ViewBufferKind(cbs[2]);
        float view[84] = {};
        if (ReadBack(ctx, cbs[2], 0, 336, view))
            std::memcpy(eye, view + 80, sizeof(eye));
    }
    if (cbs[0]) {
        D3D11_BUFFER_DESC d;
        cbs[0]->GetDesc(&d);
        blockSize = d.ByteWidth;
        ReadBack(ctx, cbs[0], 0, sizeof(block), block);
    }
    for (ID3D11Buffer* b : cbs)
        if (b)
            b->Release();
    if (type == kIndexedInstanced || type == kInstanced) {
        ID3D11Buffer* stream = nullptr;
        UINT streamStride = 0, streamOffset = 0;
        ctx->IAGetVertexBuffers(1, 1, &stream, &streamStride, &streamOffset);
        if (stream) {
            if (streamStride >= sizeof(instance))
                ReadBack(ctx, stream, streamOffset, sizeof(instance), instance);
            stream->Release();
        }
    }
    if (stride == 48 && indexed && count >= 6 && !base && QuadBatch(ctx, count, t_start)) {
        ID3D11Buffer* again = nullptr;
        UINT againStride = 0, againOffset = 0;
        ctx->IAGetVertexBuffers(0, 1, &again, &againStride, &againOffset);
        const UINT quads = std::min(count / 6, 1024u);
        thread_local std::vector<float> t_q;
        t_q.assign(quads * 4 * 12, 0.0f);
        float* q = t_q.data();
        float body[12] = {};
        if (again && ReadBack(ctx, again, againOffset, quads * 4 * 48, q) && skin::GameGunBody(body)) {
            for (UINT i = 0; i < quads; ++i) {
                float c[3] = {};
                for (int v = 0; v < 4; ++v)
                    for (int k = 0; k < 3; ++k)
                        c[k] += q[(i * 4 + v) * 12 + k] / 4.0f;
                const float dc[3] = {q[i * 4 * 12] - c[0], q[i * 4 * 12 + 1] - c[1], q[i * 4 * 12 + 2] - c[2]};
                const float rel[3] = {c[0] - body[3], c[1] - body[7], c[2] - body[11]};
                // On the gun: its rows' columns are its axes, of whatever length.
                float local[3];
                for (int a = 0; a < 3; ++a) {
                    const float axis[3] = {body[a], body[4 + a], body[8 + a]};
                    const float len2 = axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2];
                    local[a] = (rel[0] * axis[0] + rel[1] * axis[1] + rel[2] * axis[2]) / (len2 > 0.0f ? len2 : 1.0f);
                }
                // Near the gun, or along its barrel however far (a tracer).
                if (rel[0] * rel[0] + rel[1] * rel[1] + rel[2] * rel[2] > 9.0f &&
                    (std::fabs(local[0]) > 1.0f || std::fabs(local[1]) > 1.0f || local[2] < 0.0f))
                    continue;
                Log("Quad +%3d ms %d/%u: half %.3f, on the gun %+.3f %+.3f %+.3f (%.2f m) VS %016llx PS %016llx",
                    static_cast<int>(GetTickCount64() - g_scanFrom.load(std::memory_order_relaxed)), static_cast<int>(i), count / 6,
                    std::sqrt(dc[0] * dc[0] + dc[1] * dc[1] + dc[2] * dc[2]), local[0], local[1], local[2],
                    std::sqrt(rel[0] * rel[0] + rel[1] * rel[1] + rel[2] * rel[2]), vsSum, psSum);
            }
        }
        if (again)
            again->Release();
    }
    const char* where = base ? "Base" : "Scan";
    Log("%s +%3d ms: %s %u x%u stride %u stencil %u VS %016llx PS %016llx | first vertex %.3f %.3f %.3f %.3f | view %d eye "
        "%.2f %.2f %.2f | cb0 (%u) %.2f %.2f %.2f | instance %.2f %.2f %.2f | gun arms %.2f %.2f %.2f world %.2f %.2f %.2f%s",
        where, static_cast<int>(GetTickCount64() - g_scanFrom.load(std::memory_order_relaxed)), kTypeNames[type], count,
        instances, stride, ref, vsSum, psSum, f[0], f[1], f[2], f[3], viewKind, eye[0], eye[1], eye[2], blockSize, block[3],
        block[7], block[11], instance[3], instance[7], instance[11], gunArms[0], gunArms[1], gunArms[2], gunWorld[0],
        gunWorld[1], gunWorld[2], gun ? "" : " (not known)");
}

void NoteDraw(ID3D11DeviceContext* ctx, DrawType type, UINT count, UINT instances) {
    ScanDraw(ctx, type, count, instances);
    SurveyDraw(ctx, type, count, instances);
    // An eye's picture ends its eye's pass: the body lock tells the second eye's body views by it (skin.cpp), while the
    // body of a scripted picture is moved to each eye's place.
    if (count == 6 && skin::EyePictureWanted() && IsEyePicture(ctx))
        skin::NoteEyePicture();
    if (g_tailState.load(std::memory_order_relaxed) == 2)
        Tail(ctx, type, count, instances);
    ProbeDraw(ctx, type, count, instances);
    if (!g_census.load(std::memory_order_relaxed))
        return;
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11DepthStencilState* depth = nullptr;
    UINT ref = 0;
    ctx->VSGetShader(&vs, nullptr, nullptr);
    ctx->PSGetShader(&ps, nullptr, nullptr);
    ctx->OMGetDepthStencilState(&depth, &ref);
    {
        std::lock_guard lock(g_censusMutex);
        Tally& t = g_tally[Kind{type, vs, ps, ref}];
        const bool before = GetTickCount64() - g_censusStart < kBaseline;
        ++(before ? t.before : t.after);
        if (uint32_t frame = g_censusFrame.load(std::memory_order_relaxed); frame != t.lastFrame) {
            t.lastFrame = frame;
            ++(before ? t.framesBefore : t.framesAfter);
        }
        t.minCount = std::min(t.minCount, count);
        t.maxCount = std::max(t.maxCount, count);
        t.minInstances = std::min(t.minInstances, instances);
        t.maxInstances = std::max(t.maxInstances, instances);
        if (!t.described)
            Describe(ctx, depth, t);
    }
    if (vs)
        vs->Release();
    if (ps)
        ps->Release();
    if (depth)
        depth->Release();
}

const char* BlendName(D3D11_BLEND b) {
    switch (b) {
    case D3D11_BLEND_ZERO: return "0";
    case D3D11_BLEND_ONE: return "1";
    case D3D11_BLEND_SRC_ALPHA: return "srcA";
    case D3D11_BLEND_INV_SRC_ALPHA: return "1-srcA";
    case D3D11_BLEND_DEST_ALPHA: return "dstA";
    case D3D11_BLEND_INV_DEST_ALPHA: return "1-dstA";
    case D3D11_BLEND_SRC_COLOR: return "srcC";
    case D3D11_BLEND_INV_SRC_COLOR: return "1-srcC";
    case D3D11_BLEND_DEST_COLOR: return "dstC";
    default: return "other";
    }
}

// The kinds that came with firing: new after the baseline, or in clearly more of the frames.
void Report() {
    struct Row {
        Kind kind;
        const Tally* tally;
        double before, after;  // the share of frames drawn in
    };
    std::vector<Row> rows;
    const double framesBefore = std::max<uint32_t>(g_framesBefore, 1), framesAfter = std::max<uint32_t>(g_framesAfter, 1);
    for (const auto& [kind, t] : g_tally) {
        double before = t.framesBefore / framesBefore, after = t.framesAfter / framesAfter;
        if ((t.before == 0 && t.after > 0) || (after >= 0.05 && after >= 2.0 * before + 0.03))
            rows.push_back({kind, &t, before, after});
    }
    std::sort(rows.begin(), rows.end(),
              [](const Row& a, const Row& b) { return a.after - a.before > b.after - b.before; });
    Log("Census: %zu kinds of draw, %u frames of baseline and %u after; %zu came with what happened after:",
        g_tally.size(), g_framesBefore, g_framesAfter, rows.size());
    for (size_t i = 0; i < rows.size() && i < 40; ++i) {
        const Row& r = rows[i];
        const Tally& t = *r.tally;
        Log("Census: %s VS %016llx PS %016llx, stencil %u%s: in %.0f%% of frames before, %.0f%% after (%.2f draws a frame "
            "after); counts %u-%u, instances %u-%u; topology %d, stride %u, blend %s (%s, %s), depth %s%s; VS "
            "buffers %s",
            kTypeNames[r.kind.type],
            static_cast<unsigned long long>(shaders::ChecksumOfVs(static_cast<ID3D11VertexShader*>(const_cast<void*>(r.kind.vs)))),
            static_cast<unsigned long long>(shaders::ChecksumOf(static_cast<ID3D11PixelShader*>(const_cast<void*>(r.kind.ps)))),
            r.kind.stencilRef, r.kind.stencilRef & 32 ? " (first person)" : "",
            100.0 * r.before, 100.0 * r.after, t.after / framesAfter, t.minCount, t.maxCount, t.minInstances,
            t.maxInstances, static_cast<int>(t.topology), t.stride, t.blend ? "on" : "off", BlendName(t.src),
            BlendName(t.dst), t.depthTest ? "tested" : "off", t.depthWrite ? ", written" : "", t.buffers);
    }
}

// ---- Gun probe ----
// F19: for 10 s (fire meanwhile), every draw is checked for something placed at the game's own
// gun: a placement in its vertex shader's constant buffers (three unit-length rows with the
// translation in their fourth components, the rows in any order: BF3 keeps some as y, z, x) or
// vertices in its vertex buffer (the first three floats of each), within 1 m of the gun body, in
// the world or in the arms' space. Each kind of draw found so is logged once, with where. The
// muzzle flash and the casings start where the game's gun is: this finds their draws and where
// their placements are. Uploads are seen only with weapon.txt "watch".
std::atomic<ULONGLONG> g_probeUntil{0};
float g_gunWorld[3] = {}, g_gunArms[3] = {};
std::atomic<bool> g_haveGun{false};
std::mutex g_probeMutex;
std::vector<Kind> g_probeLogged;

bool Blended(ID3D11DeviceContext* ctx) {
    ID3D11BlendState* blend = nullptr;
    FLOAT factor[4];
    UINT mask = 0;
    ctx->OMGetBlendState(&blend, factor, &mask);
    bool on = false;
    if (blend) {
        D3D11_BLEND_DESC d;
        blend->GetDesc(&d);
        on = d.RenderTarget[0].BlendEnable != FALSE;
        blend->Release();
    }
    return on;
}

float Length3(const float* v) {
    return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

float Distance3(const float* a, const float* b) {
    const float d[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    return Length3(d);
}

// The nearest anything in some data comes to the gun: where, in which space, and for placements
// which order of rows (0: x y z; 1: y z x; 2: z x y), for vertices how many lie within 1 m.
struct Hit {
    float distance = 1e9f;
    size_t offset = 0;
    bool arms = false;
    int order = -1;  // -1: vertices
    int within = 0;
};

void ScanPlacements(const uint8_t* data, UINT size, Hit& hit) {
    const auto* f = reinterpret_cast<const float*>(data);
    const size_t floats = size / 4;
    for (size_t k = 0; k + 12 <= floats; k += 4) {
        const float* m = f + k;
        if (std::fabs(Length3(m) - 1) > 0.05f || std::fabs(Length3(m + 4) - 1) > 0.05f ||
            std::fabs(Length3(m + 8) - 1) > 0.05f)
            continue;
        const float w[3] = {m[3], m[7], m[11]};
        for (int order = 0; order < 3; ++order) {
            const float t[3] = {w[(3 - order) % 3], w[(4 - order) % 3], w[(5 - order) % 3]};
            for (int space = 0; space < 2; ++space) {
                const float d = Distance3(t, space ? g_gunArms : g_gunWorld);
                if (d < hit.distance)
                    hit = Hit{d, k * 4, space == 1, order, 0};
            }
        }
    }
}

void ScanVertices(const uint8_t* data, UINT size, UINT stride, Hit& hit) {
    if (stride < 12)
        return;
    Hit best;
    int within = 0;
    for (UINT o = 0; o + 12 <= size; o += stride) {
        const float* p = reinterpret_cast<const float*>(data + o);
        for (int space = 0; space < 2; ++space) {
            const float d = Distance3(p, space ? g_gunArms : g_gunWorld);
            within += d < 1.0f;
            if (d < best.distance)
                best = Hit{d, o, space == 1, -1, 0};
        }
    }
    best.within = within;
    if (best.distance < hit.distance)
        hit = best;
}

// Uploads made on this thread up to its last draw: older data a draw has bound was some earlier
// draw's (one buffer is refilled for draw after draw), so only newer data counts as its own.
thread_local uint32_t t_uploadsAtLastDraw = 0;

void ProbeDraw(ID3D11DeviceContext* ctx, DrawType type, UINT count, UINT instances) {
    const uint32_t before = t_uploadsAtLastDraw;
    t_uploadsAtLastDraw = skin::UploadSerial();
    if (GetTickCount64() > g_probeUntil.load(std::memory_order_relaxed) || !g_haveGun.load(std::memory_order_relaxed))
        return;
    ID3D11DepthStencilState* depth = nullptr;
    UINT ref = 0;
    ctx->OMGetDepthStencilState(&depth, &ref);
    if (depth)
        depth->Release();
    if ((ref & 32) || !Blended(ctx))
        return;  // effects only: blended, and not the first-person gun and arms
    // Every placement within 1.2 m of the game's muzzle in the data bound: where, which order of
    // rows, in which space, how far, and whether it was uploaded for this draw ("new").
    static const char* const kOrders[] = {"xyz", "yzx", "zxy"};
    char found[512] = "";
    int used = 0, matches = 0;
    ID3D11Buffer* cb[4] = {};
    ctx->VSGetConstantBuffers(0, 4, cb);
    for (int i = 0; i < 4; ++i) {
        if (!cb[i])
            continue;
        UINT size = 0;
        uint32_t serial = 0;
        const uint8_t* data = skin::LatestBlockOf(cb[i], size, serial);
        if (!data)
            data = skin::RecentUploadOf(cb[i], size, serial);
        cb[i]->Release();
        if (!data)
            continue;
        const auto* f = reinterpret_cast<const float*>(data);
        // Plain points (a particle's centre): any register's x, y, z within 25 cm of the muzzle.
        for (size_t k = 0; k + 3 <= size / 4 && used < 440; k += 4) {
            for (int space = 0; space < 2; ++space) {
                const float d = Distance3(f + k, space ? g_gunArms : g_gunWorld);
                if (d < 0.25f && used < 440) {
                    used += snprintf(found + used, sizeof(found) - used, " cb%d+%zu point %s %.2fm%s;", i, k * 4,
                                     space ? "arms" : "world", d, serial > before ? " new" : "");
                    ++matches;
                }
            }
        }
        for (size_t k = 0; k + 12 <= size / 4 && used < 440; k += 4) {
            const float* m = f + k;
            if (std::fabs(Length3(m) - 1) > 0.05f || std::fabs(Length3(m + 4) - 1) > 0.05f ||
                std::fabs(Length3(m + 8) - 1) > 0.05f)
                continue;
            const float w[3] = {m[3], m[7], m[11]};
            for (int order = 0; order < 3; ++order) {
                const float t[3] = {w[(3 - order) % 3], w[(4 - order) % 3], w[(5 - order) % 3]};
                for (int space = 0; space < 2; ++space) {
                    const float d = Distance3(t, space ? g_gunArms : g_gunWorld);
                    if (d < 0.3f && used < 440) {
                        used += snprintf(found + used, sizeof(found) - used, " cb%d+%zu %s %s %.2fm%s;", i, k * 4,
                                         kOrders[order], space ? "arms" : "world", d, serial > before ? " new" : "");
                        ++matches;
                    }
                }
            }
        }
    }
    {
        // Vertices uploaded for it (batched particles): how many lie within 25 cm of the muzzle.
        ID3D11Buffer* vb = nullptr;
        UINT vbStride = 0, vbOffset = 0;
        ctx->IAGetVertexBuffers(0, 1, &vb, &vbStride, &vbOffset);
        if (vb) {
            UINT size = 0;
            uint32_t serial = 0;
            const uint8_t* data = skin::RecentUploadOf(vb, size, serial);
            vb->Release();
            int close[2] = {};
            for (UINT o = 0; data && vbStride >= 12 && o + 12 <= size; o += vbStride)
                for (int space = 0; space < 2; ++space)
                    close[space] += Distance3(reinterpret_cast<const float*>(data + o), space ? g_gunArms : g_gunWorld) < 0.25f;
            if ((close[0] || close[1]) && used < 440) {
                used += snprintf(found + used, sizeof(found) - used, " vertices near the muzzle: %d world, %d arms%s;",
                                 close[0], close[1], serial > before ? " new" : "");
                ++matches;
            }
        }
    }
    if (!matches)
        return;
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ctx->VSGetShader(&vs, nullptr, nullptr);
    ctx->PSGetShader(&ps, nullptr, nullptr);
    const Kind kind{static_cast<uint8_t>(type), vs, ps, ref};
    if (vs)
        vs->Release();
    if (ps)
        ps->Release();
    ID3D11Buffer* vb = nullptr;
    UINT stride = 0, offset = 0;
    ctx->IAGetVertexBuffers(0, 1, &vb, &stride, &offset);
    if (vb)
        vb->Release();
    std::lock_guard lock(g_probeMutex);
    if (g_probeLogged.size() >= 40 ||
        std::find(g_probeLogged.begin(), g_probeLogged.end(), kind) != g_probeLogged.end())
        return;
    g_probeLogged.push_back(kind);
    Log("Muzzle probe: %s count %u (from index %u), %u instances, stride %u, stencil %u, VS %p PS %p:%s",
        kTypeNames[type], count, type == kIndexed || type == kIndexedInstanced ? t_start : 0u, instances, stride, ref,
        kind.vs, kind.ps, found);
}

void STDMETHODCALLTYPE Draw0(ID3D11DeviceContext* c, UINT count, UINT start) {
    t_start = start;
    NoteDraw(c, kPlain, count, 1);
    if (HiddenPs(c) || LensFlare(c, count))
        return;
    if (overlay::Draw(c, count, [&] { g_origDraw[0](c, count, start); }))
        return;
    g_origDraw[0](c, count, start);
}
void STDMETHODCALLTYPE Draw1(ID3D11DeviceContext* c, UINT count, UINT start) {
    t_start = start;
    NoteDraw(c, kPlain, count, 1);
    if (HiddenPs(c) || LensFlare(c, count))
        return;
    if (overlay::Draw(c, count, [&] { g_origDraw[1](c, count, start); }))
        return;
    g_origDraw[1](c, count, start);
}
void STDMETHODCALLTYPE DrawInstanced0(ID3D11DeviceContext* c, UINT count, UINT instances, UINT start, UINT first) {
    t_start = start;
    t_first = first;
    NoteDraw(c, kInstanced, count, instances);
    if (HiddenPs(c))
        return;
    g_origDrawInstanced[0](c, count, instances, start, first);
}
void STDMETHODCALLTYPE DrawInstanced1(ID3D11DeviceContext* c, UINT count, UINT instances, UINT start, UINT first) {
    t_start = start;
    t_first = first;
    NoteDraw(c, kInstanced, count, instances);
    if (HiddenPs(c))
        return;
    g_origDrawInstanced[1](c, count, instances, start, first);
}
void STDMETHODCALLTYPE DrawIndexedInstanced0(ID3D11DeviceContext* c, UINT count, UINT instances, UINT start, INT base,
                                             UINT first) {
    t_start = start;
    t_base = base;
    t_first = first;
    NoteDraw(c, kIndexedInstanced, count, instances);
    DumpFx(count);
    DumpInstances(c, count, instances, first, true);
    skin::LogShaderDraw(c, count, start, base, instances);
    if (Hidden(count) || HiddenPs(c))
        return;
    if (IsCasingCount(count) &&
        DrawInstancesMoved(c, instances, first, [&](UINT at) { g_origDrawIndexedInstanced[0](c, count, instances, start, base, at); }))
        return;
    if (Moved(count) &&
        DrawMoved(0, c, [&] { g_origDrawIndexedInstanced[0](c, count, instances, start, base, first); }))
        return;
    g_origDrawIndexedInstanced[0](c, count, instances, start, base, first);
}
void STDMETHODCALLTYPE DrawIndexedInstanced1(ID3D11DeviceContext* c, UINT count, UINT instances, UINT start, INT base,
                                             UINT first) {
    t_start = start;
    t_base = base;
    t_first = first;
    NoteDraw(c, kIndexedInstanced, count, instances);
    DumpFx(count);
    DumpInstances(c, count, instances, first, false);
    skin::LogShaderDraw(c, count, start, base, instances);
    if (Hidden(count) || HiddenPs(c))
        return;
    if (IsCasingCount(count) &&
        DrawInstancesMoved(c, instances, first, [&](UINT at) { g_origDrawIndexedInstanced[1](c, count, instances, start, base, at); }))
        return;
    if (Moved(count) &&
        DrawMoved(1, c, [&] { g_origDrawIndexedInstanced[1](c, count, instances, start, base, first); }))
        return;
    g_origDrawIndexedInstanced[1](c, count, instances, start, base, first);
}
void STDMETHODCALLTYPE DrawIndexedIndirect0(ID3D11DeviceContext* c, ID3D11Buffer* args, UINT offset) {
    NoteDraw(c, kIndexedIndirect, 0, 0);
    g_origDrawIndexedIndirect[0](c, args, offset);
}
void STDMETHODCALLTYPE DrawIndexedIndirect1(ID3D11DeviceContext* c, ID3D11Buffer* args, UINT offset) {
    NoteDraw(c, kIndexedIndirect, 0, 0);
    g_origDrawIndexedIndirect[1](c, args, offset);
}
void STDMETHODCALLTYPE DrawIndirect0(ID3D11DeviceContext* c, ID3D11Buffer* args, UINT offset) {
    NoteDraw(c, kIndirect, 0, 0);
    g_origDrawIndirect[0](c, args, offset);
}
void STDMETHODCALLTYPE DrawIndirect1(ID3D11DeviceContext* c, ID3D11Buffer* args, UINT offset) {
    NoteDraw(c, kIndirect, 0, 0);
    g_origDrawIndirect[1](c, args, offset);
}

// Hooks one function of both kinds of context (one hook when they share it).
template <typename Fn>
bool HookPair(void* const (&targets)[2], Fn hook0, Fn hook1, Fn (&original)[2]) {
    if (MH_CreateHook(targets[0], reinterpret_cast<void*>(hook0), reinterpret_cast<void**>(&original[0])) != MH_OK)
        return false;
    if (targets[1] == targets[0]) {
        original[1] = original[0];
        return true;
    }
    return MH_CreateHook(targets[1], reinterpret_cast<void*>(hook1), reinterpret_cast<void**>(&original[1])) == MH_OK;
}

}

namespace effects {

bool InstallHooks(const DrawFunctions& f) {
    bool ok = HookPair(f.draw, &Draw0, &Draw1, g_origDraw) &&
              HookPair(f.drawInstanced, &DrawInstanced0, &DrawInstanced1, g_origDrawInstanced) &&
              HookPair(f.drawIndexedInstanced, &DrawIndexedInstanced0, &DrawIndexedInstanced1,
                       g_origDrawIndexedInstanced) &&
              HookPair(f.drawIndexedIndirect, &DrawIndexedIndirect0, &DrawIndexedIndirect1, g_origDrawIndexedIndirect) &&
              HookPair(f.drawIndirect, &DrawIndirect0, &DrawIndirect1, g_origDrawIndirect);
    if (!ok)
        Log("Effects: hooking the other draw calls failed");
    return ok;
}


int NoteDrawIndexed(ID3D11DeviceContext* ctx, UINT count, UINT start, INT base) {
    t_start = start;
    t_base = base;
    NoteDraw(ctx, kIndexed, count, 1);
    if (Hidden(count) || HiddenPs(ctx))
        return kHide;
    return QuadBatch(ctx, count, start) ? kQuads : kNormal;
}

bool DrawQuads(ID3D11DeviceContext* ctx, UINT count, UINT start, INT base, const std::function<void(INT)>& draw) {
    return DrawQuadsMoved(ctx, count, start, base, draw);
}

void SetFlashQuads(bool on, float across, float along) {
    g_moveQuads = on;
    g_quadsAcross = across;
    g_quadsAlong = along;
    Log("Effects: the flash and smoke %s (all the way within %.2f m of the game's barrel's line, up to %.2f m past its muzzle)",
        on ? "moved with the gun" : "where the game has them", across, along);
}


void SetCasings(bool on, float nearest, float farthest) {
    nearest = std::clamp(nearest, 0.0f, 2.0f);
    farthest = std::clamp(farthest, nearest + 0.01f, 3.0f);
    const bool was = g_moveCasings.exchange(on);
    const float wasNear = g_casingNear.exchange(nearest), wasFar = g_casingFar.exchange(farthest);
    if (was != on || wasNear != nearest || wasFar != farthest)
        Log(on ? "Effects: the casings come out of the gun's ejection port (all the way within %.2f m of it, not at "
                 "%.2f m)"
               : "Effects: the casings drawn where the game throws them (%.2f %.2f)",
            nearest, farthest);
}

void NoteMapped(ID3D11Resource* resource, const void* data) {
    if (g_surveyPhase.load(std::memory_order_relaxed) == 0)
        return;
    std::lock_guard lock(g_surveyMutex);
    for (MappedBuffer& m : g_mapped)
        if (m.resource == resource || !m.resource) {
            m.resource = resource, m.data = static_cast<const uint8_t*>(data);
            return;
        }
}

void FlashSurvey(int ms) {
    g_surveyMs = ms;
    g_surveyFrom = GetTickCount64();
    g_surveyUntil = GetTickCount64() + 300;
    g_surveyPhase = 1;
    Log("Flash: every draw for 300 ms, then for %d ms from the next press of the fire button", ms);
}

void FlashScan() {
    g_scanArmed = true;
    g_scanBaseUntil = GetTickCount64() + 100;
    g_scanFrom = GetTickCount64();
    Log("Flash: the draws after the next shot will be logged");
}

void FlashBurst(int frames) {
    depth3d::BurstArm(frames);
    g_burstArmed = true;
    Log("Flash: %d frames from the next shot will be saved", frames);
}

void BurstNow(int frames, int every) {
    depth3d::BurstArm(frames, every);
    depth3d::BurstStart();
}


void SetLift(float metres) {
    if (g_lift.exchange(metres) != metres)
        Log("Effects: what's moved with the gun is lifted %.2f m more", metres);
}

ULONGLONG MsSinceShot() {
    return GetTickCount64() - g_lastShot.load(std::memory_order_relaxed);
}

ULONGLONG LastShot() {
    return g_lastShot.load(std::memory_order_relaxed);
}

void SetMoveMatrices(int which) {
    if (which > 0 && g_moveMatrices.exchange(which) != which)
        Log("Effects: moved draws change view constants %d (1 view, 2 view-projection, 4 from the eye, 8 projection)",
            which);
}

void SetHiddenShaders(const uintptr_t* shaders, int n) {
    bool changed = false;
    for (int i = 0; i < 4; ++i)
        changed |= g_hidePs[i].exchange(i < n ? shaders[i] : 0) != (i < n ? shaders[i] : 0);
    g_anyHidePs = n > 0;
    if (changed)
        Log("Effects: hiding draws with %d pixel shaders: %08X %08X %08X %08X", n,
            static_cast<unsigned>(g_hidePs[0].load()), static_cast<unsigned>(g_hidePs[1].load()),
            static_cast<unsigned>(g_hidePs[2].load()), static_cast<unsigned>(g_hidePs[3].load()));
}

void SetMoved(const UINT* counts, int n) {
    bool changed = false;
    for (int i = 0; i < 4; ++i)
        changed |= g_moveFx[i].exchange(i < n ? counts[i] : 0) != (i < n ? counts[i] : 0);
    if (changed)
        Log("Effects: drawing draws of %d index counts at the gun in the hand: %u %u %u %u", n, g_moveFx[0].load(),
            g_moveFx[1].load(), g_moveFx[2].load(), g_moveFx[3].load());
}

void DumpNext(UINT count) {
    g_dumpFx = count;
    g_dumpFxLeft = count ? 3 : 0;
    g_dumpInstancesLeft = count ? 6 : 0;
}

void SetHidden(const UINT* counts, int n) {
    bool changed = false;
    for (int i = 0; i < 4; ++i)
        changed |= g_hideFx[i].exchange(i < n ? counts[i] : 0) != (i < n ? counts[i] : 0);
    if (changed)
        Log("Effects: hiding draws of %d index counts: %u %u %u %u", n, g_hideFx[0].load(), g_hideFx[1].load(),
            g_hideFx[2].load(), g_hideFx[3].load());
}

void StartFlashProbe() {
    g_probeUntil = GetTickCount64() + 10000;
    {
        std::lock_guard lock(g_probeMutex);
        g_probeLogged.clear();
    }
    Log("Muzzle probe: the next 10 s (fire, with weapon.txt \"watch\")");
}

void LogFrameTail() {
    g_tailState = 1;
}

bool TailRecording() {
    return g_tailState.load(std::memory_order_relaxed) == 2;
}

void TailClear(ID3D11DeviceContext* ctx, ID3D11DepthStencilView* dsv, UINT flags, float depth, UINT8 stencil) {
    ID3D11Resource* resource = nullptr;
    UINT width = 0, height = 0;
    if (dsv) {
        dsv->GetResource(&resource);
        ID3D11Texture2D* texture = nullptr;
        if (resource && SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&texture)))) {
            D3D11_TEXTURE2D_DESC desc;
            texture->GetDesc(&desc);
            width = desc.Width, height = desc.Height;
            texture->Release();
        }
        if (resource)
            resource->Release();
    }
    TailEvent e{};
    std::snprintf(e.text, sizeof(e.text), "clear, context %p, depth-stencil %p (%ux%u):%s%s to %.3f / %u",
                  static_cast<void*>(ctx), static_cast<void*>(resource), width, height,
                  flags & D3D11_CLEAR_DEPTH ? " depth" : "", flags & D3D11_CLEAR_STENCIL ? " stencil" : "", depth,
                  static_cast<unsigned>(stencil));
    std::lock_guard lock(g_tailMutex);
    e.at = g_tail.size();
    g_tailEvents.push_back(e);
}

void TailNote(const char* text) {
    TailEvent e{};
    std::snprintf(e.text, sizeof(e.text), "%s", text);
    std::lock_guard lock(g_tailMutex);
    e.at = g_tail.size();
    g_tailEvents.push_back(e);
}

void StartCensus() {
    std::lock_guard lock(g_censusMutex);
    g_tally.clear();
    g_framesBefore = g_framesAfter = 0;
    g_censusStart = GetTickCount64();
    g_census = true;
    Log("Census: every draw for 6 s; the first 2 s are the baseline (fire after them)");
}

void Update() {
    // Shots: the rounds in the gun going down by one (the flash comes with them).
    int loaded = 0;
    if (ammo::RoundsInHand(loaded)) {
        if (g_lastLoaded >= 0 && loaded == g_lastLoaded - 1) {
            NoteQuadsOpening();
            g_lastShot = GetTickCount64(), ++g_shots;
        }
        g_lastLoaded = loaded;
    }
    // weapon.txt "flashscan", "flashburst": from the moment the button goes down (a flash comes before the server's count).
    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
        NoteQuadsOpening();
        g_triggerAt = GetTickCount64();
        if (g_scanArmed.exchange(false)) {
            g_scanFrom = GetTickCount64();
            g_scanUntil = GetTickCount64() + 800;
        }
        if (g_burstArmed.exchange(false))
            depth3d::BurstStart();
    }

    switch (g_surveyPhase.load(std::memory_order_relaxed)) {
    case 1:
        if (GetTickCount64() > g_surveyUntil.load(std::memory_order_relaxed))
            g_surveyPhase = 2;
        break;
    case 2:
        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
            g_surveyFrom = GetTickCount64();
            g_surveyUntil = GetTickCount64() + g_surveyMs.load(std::memory_order_relaxed);
            g_surveyPhase = 3;
        }
        break;
    case 3:
        if (GetTickCount64() > g_surveyUntil.load(std::memory_order_relaxed)) {
            g_surveyPhase = 0;
            SurveyReport();
        }
        break;
    default:
        break;
    }
    // Once a second while anything happened: shots, and what became of the flash's draws.
    static ULONGLONG lastLog = 0;
    if (const ULONGLONG now = GetTickCount64(); now - lastLog >= 1000) {
        lastLog = now;
        const uint32_t shots = g_shots.exchange(0), moved = g_movedDraws.exchange(0), noDelta = g_noDelta.exchange(0),
                       noView = g_noView.exchange(0), noSlot = g_noSlot.exchange(0);
        if (const uint32_t batches = g_quadBatches.exchange(0), notMoved = g_quadsNot.exchange(0); batches || notMoved)
            Log("Effects: last second %u batches of the flash's quads moved, %u not", batches, notMoved);
        if (const uint32_t casings = g_casingsMoved.exchange(0), notMoved = g_casingsNot.exchange(0); casings || notMoved)
            Log("Effects: last second %u casing draws at the gun's ejection port, %u not (no instance stream)", casings,
                notMoved);
        if (shots)
            Log("Effects: last second %u shots", shots);
        if (moved || noDelta || noView || noSlot)  // weapon.txt "movefx"
            Log("Effects: last second moved by the view %u, not moved: %u no gun move, %u no view constants, %u not bound (matrices %d)",
                moved, noDelta, noView, noSlot, g_moveMatrices.load());
    }
    if (GetTickCount64() <= g_probeUntil.load(std::memory_order_relaxed)) {
        // The game's muzzle: the end of the barrel in the gun's model space (the mesh dump).
        constexpr float kMuzzle[3] = {0.0f, 0.04f, 0.78f};
        float body[12], world[3], arms[3];
        bool have = skin::GameGunBody(body) && skin::GameGunPlaces(world, arms);
        if (have)
            for (int r = 0; r < 3; ++r) {
                g_gunWorld[r] = body[r * 4] * kMuzzle[0] + body[r * 4 + 1] * kMuzzle[1] + body[r * 4 + 2] * kMuzzle[2] +
                                body[r * 4 + 3];
                g_gunArms[r] = g_gunWorld[r] - (world[r] - arms[r]);
            }
        g_haveGun = have;
    }
    if (const int tail = g_tailState.load(std::memory_order_relaxed); tail == 1) {
        std::lock_guard lock(g_tailMutex);
        g_tail.clear();
        g_tailAll = 0;
        g_tailState = 2;
    } else if (tail == 2) {
        g_tailState = 0;
        LogTail();
    }
    if (!g_census.load(std::memory_order_relaxed))
        return;
    std::lock_guard lock(g_censusMutex);
    g_censusFrame.fetch_add(1, std::memory_order_relaxed);
    ULONGLONG elapsed = GetTickCount64() - g_censusStart;
    if (elapsed < kBaseline) {
        ++g_framesBefore;
    } else if (elapsed < kCensus) {
        ++g_framesAfter;
    } else {
        g_census = false;
        Report();
        g_tally.clear();
    }
}

}

#include "scopeview.h"

#include <windows.h>
#include <MinHook.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <mutex>

#include "camera.h"
#include "gputime.h"
#include "log.h"

// The game's world renderer (bf3.exe 1.6, not relocated, read in the memory image; its one object is made for each level by the factory, 0x48880 bytes). Everything of the frame that belongs to one renderer is
// in the object: its targets (own size), its passes, its settings. The camera comes in with each dispatch. So a second object is a second picture of the same world: it dispatches first, on the game's client
// thread, and is waited for (the entity renderers keep what one dispatch needs in themselves, but its draw lists end up in the frame's memory); then the eyes' dispatch. On the render thread it draws just
// before the eyes do, so its picture's there for the glass in the same frame.
namespace scopeview {

std::atomic<bool> g_drawing{false};

namespace {

constexpr uintptr_t kFactory = 0x017C7F10, kDispatchAt = 0x017C7F70, kRenderAt = 0x017CA330, kFrameAt = 0x017C8540, kPostAt = 0x0068F600, kLightsAt = 0x017B3F30, kViewSetupAt = 0x017C22D0, kDestroyAt = 0x017C7B70;
constexpr uint32_t kRendererVtable = 0x02238B48;
// The entity renderers by their vtables: its own occlusion, and the world's light bounce, which is global, aren't shared; the terrain's is watched.
constexpr uint32_t kOcclusionVtable = 0x02237778, kBounceVtable = 0x02238728, kTerrainVtable = 0x020D3574;
constexpr uintptr_t kDeviceHolder = 0x023577D4, kFrameMemoryHolder = 0x02356C90;
constexpr uintptr_t kDeviceBack = 0x017BBD50;

constexpr uint8_t kDispatchStart[] = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0, 0x83, 0xEC, 0x64, 0x53, 0x56, 0x57};
constexpr uint8_t kFrameStart[] = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0, 0xB8, 0x74, 0xA8, 0x00, 0x00, 0xE8};
constexpr uint8_t kRenderStart[] = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0, 0x81, 0xEC, 0x74, 0x01, 0x00, 0x00};
constexpr uint8_t kPostStart[] = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0, 0x81, 0xEC, 0xC4, 0x02, 0x00, 0x00};
constexpr uint8_t kLightsStart[] = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0, 0xB8, 0x54, 0x24, 0x00, 0x00, 0xE8};
constexpr uint8_t kViewSetupStart[] = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0, 0xB8, 0x44, 0x53, 0x00, 0x00, 0xE8};
constexpr uint8_t kDestroyStart[] = {0x56, 0x8B, 0xF1, 0xE8, 0xC8, 0xEB, 0xFF, 0xFF, 0xF6, 0x44, 0x24, 0x08, 0x01};
constexpr uint8_t kFactoryStart[] = {0x56, 0x8B, 0x74, 0x24, 0x08, 0x6A, 0x10, 0x68, 0x80, 0x88, 0x04, 0x00};

constexpr size_t kDescSize = 0x1100, kMainCamera = 0x30, kCullCamera = 0x4B0;
constexpr size_t kViewFirst = 0x8C20, kViewStride = 0x1FDC0;

struct Handle {
    void* vtable;
    void* job;
    uint32_t gen;
};

struct RenderParams {
    uint32_t frame;
    float dt;
    double time;
    float dt2;
    uint32_t viewCount;
    uint8_t* descs;
    Handle prereq;
};
static_assert(sizeof(RenderParams) == 0x28);

struct RendererParams {
    const void* name;
    void* post;
    void* decals;
    uint32_t width, height;
};

using DispatchFn = void(__fastcall*)(uint8_t* self, void*, RenderParams* params, Handle* out1, Handle* out2);
using RenderFn = void(__fastcall*)(uint8_t* self, void*);
using PostFn = void(__fastcall*)(void* self, void*, void* arg);
using LightsFn = void(__fastcall*)(void* self, void*, void* a, void* b, void* c, void* d);
using ViewSetupFn = void(__fastcall*)(uint8_t* self, void*, void* a, uint8_t* view);
using DestroyFn = void*(__fastcall*)(uint8_t* self, void*, uint32_t flags);
using FactoryFn = uint8_t*(__cdecl*)(void* allocator, const RendererParams* params);
using HandleInitFn = void(__fastcall*)(Handle* self, void*);
using WaitFn = void(__cdecl*)(Handle* handle);
using CloneFn = uint8_t*(__fastcall*)(void* self, void*, void* allocator, void* a, void* b);
using ReleaseFn = void(__fastcall*)(void* self, void*);

DispatchFn g_dispatch = nullptr;
RenderFn g_frame = nullptr, g_render = nullptr;
PostFn g_post = nullptr;
LightsFn g_lights = nullptr;
ViewSetupFn g_viewSetup = nullptr;
DestroyFn g_destroy = nullptr;

bool g_hooked = false, g_hookFailed = false;
std::atomic<int> g_size{720};        // weapon.txt "scopeview S": its picture's size; 0 none (the lens then magnifies the eye's own picture, as before)
std::atomic<int> g_saveWanted{0};
uint8_t* g_first = nullptr;          // the eyes' renderer
uint8_t* g_second = nullptr;         // the scope's
// Its dispatch till both its handles are waited for: the entity renderers' dispatches run in a job of its own (with the setup job on), after the dispatch call has returned.
std::atomic<bool> g_inSecondDispatch{false};
std::atomic<uint32_t> g_readyFrame{0xFFFFFFFFu};
std::atomic<float> g_tangent{0.0f};  // the half width of its picture, as a slope, as it was last dispatched
bool g_failed = false;
bool g_warm = false;  // made this frame: dispatched once, not drawn (client thread)
std::atomic<bool> g_settingsRead{false};  // weapon.txt read once: "scopeview 0" there keeps the engine unhooked
alignas(16) uint8_t g_desc[kDescSize];

// What's drawn: its camera and the eyes' main cameras of the same build, their right, up, back (9 floats each).
std::mutex g_rowsMutex;
float g_rows[9] = {};
float g_eyeRows[2][9] = {};
std::atomic<bool> g_pictureNow{false};  // drawn this frame, on the render thread

// The sight being looked through, as the last frame's lens saw it: in the view's axes, for the next build's cameras.
struct Line {
    float z[3], y[3], at[3];  // its line, its up, the glass's middle from the eye: right, up, back
    float radius = 0.0f, off = 0.0f, power = 0.0f;
    float focal = 0.0f;  // the eye's picture's pixels per unit of slope (906 at 100 degrees and 2160)
    int eye = -1;
    ULONGLONG when = 0;
};
Line g_line;  // under g_rowsMutex
std::atomic<int> g_activeFrames{0};  // frames in a row it's been wanted
std::atomic<bool> g_on{false};
float g_c = 0.0f;        // the picture's half width, as a slope before the power's taken out
float g_detailC = 0.0f;  // the same, followed slowly: its level of detail goes by it

// The time it takes, on each of its threads, for the once-a-second line.
LARGE_INTEGER g_ticksPerSecond{};
std::atomic<int64_t> g_dispatchTicks{0}, g_renderTicks{0};
std::atomic<uint32_t> g_dispatches{0}, g_renders{0};
ULONGLONG g_lastLine = 0;

std::atomic<float> g_wantedFov{0.0f};
std::atomic<bool> g_force{false};  // weapon.txt "scopeviewforce": always, whatever's looked through
// weapon.txt "scopeviewshare": which of the first's entity renderers go on the second, by their place in its list. Not its emitters (3): their dispatch maps the view's particle buffers by the view's index, and the
// eyes' dispatch maps them again before the second draws (its quads would be the eyes', misplaced).
std::atomic<int> g_shareMask{0x1E};
// The terrain isn't worked out for it: done for a third view it cost the eyes 4.5 ms a frame on the graphics card from then on (the game's one terrain object keeps what each view asked of it). It draws the
// first eye's terrain, made this frame for a wider view round the same place. Its particles likewise: a renderer of its own drew them as flat cards (the user, 2026-11-05: "it's all squares"), so it
// draws the first eye's, made for the same place and facing the same way. Each borrowed renderer's place in the two lists.
struct Borrowed {
    uint32_t vtable;
    const char* name;
    int first = -1, second = -1;
    std::atomic<uint32_t> kept{0}, seen{0};  // the last frame's entries, for the log
};
Borrowed g_borrowed[2] = {{0x020D3574, "terrain"}, {0x02246040, "particles"}};
std::atomic<bool> g_borrowTerrain{true};  // weapon.txt "scopeviewterrain 1": worked out for it too, as a test (both borrowed renderers)
// weapon.txt "scopeviewshadows 0": no sun shadows of its own. (The user, 2026-11-05, first "Shadows off through the scope is a great idea", then, told the frame drops were the terrain's: "can we turn on the
// shadows then?". Off, they saved nothing measurable.)
std::atomic<bool> g_shadows{true};
std::atomic<bool> g_ownParticles{false};  // weapon.txt "scopeviewparticles 1": a particles renderer of its own (read when it's made), not the eye's borrowed
using EntityDispatchFn = void(__fastcall*)(void* self, void*, uint32_t count, void* entries, Handle* h1, Handle* h2);
EntityDispatchFn g_borrowedDispatch[2] = {};
void __fastcall TerrainDispatchHook(void* self, void*, uint32_t count, void* entries, Handle* h1, Handle* h2);
void __fastcall ParticlesDispatchHook(void* self, void*, uint32_t count, void* entries, Handle* h1, Handle* h2);
std::atomic<float> g_detailRatio{1.15f};  // weapon.txt "scopeviewdetail": its level of detail against the eyes': tan(its FOV/2)/tan(what it goes by/2) times the power

template <class T>
T Read(uintptr_t at) {
    T v{};
    camera::SafeRead(at, &v, sizeof(v));
    return v;
}

bool SameCode(uintptr_t at, const uint8_t* code, size_t size) {
    uint8_t now[16];
    return size <= sizeof(now) && camera::SafeRead(at, now, size) && std::memcmp(now, code, size) == 0;
}

// Which of the eyes' entity renderers it shares, by what they are, not where they sit in the list (another level's may be in another order): the bit of each in weapon.txt "scopeviewshare".
constexpr struct {
    uint32_t vtable;
    int bit;
} kShareable[] = {{0x02238760, 1}, {0x020D3574, 2}, {0x02246040, 3}, {0x022463D4, 4}};

// The entity renderers of the first go on the second too: not by its AddEntityRenderer, which also makes them listeners and would tick their update, begin and end a second time.
void ShareRenderers(uint8_t* first, uint8_t* second) {
    for (Borrowed& b : g_borrowed)
        b.first = b.second = -1;
    auto** from = *reinterpret_cast<uint32_t***>(first + 0x899C);
    auto** to = *reinterpret_cast<uint32_t***>(first + 0x89A0);
    int shared = 0, index = 0;
    const int mask = g_shareMask.load(std::memory_order_relaxed);
    for (auto** at = from; at != to; ++at, ++index) {
        uint32_t* renderer = *at;
        const uint32_t vtable = renderer ? *renderer : 0;
        int bit = -1;
        for (const auto& s : kShareable)
            if (s.vtable == vtable)
                bit = s.bit;
        // A borrowed one only with its own dispatch hooked to pass over the second's view (or it'd work out a third view: the terrain's cost the eyes 4.5 ms for good).
        bool unhooked = false;
        for (int k = 0; k < 2; ++k)
            if (vtable == g_borrowed[k].vtable && !g_borrowedDispatch[k])
                unhooked = true;
        if (!renderer || bit < 0 || !(mask >> bit & 1) || unhooked) {
            Log("Scope view: entity renderer %d at %08X, vtable %08X, not shared%s", index, reinterpret_cast<uintptr_t>(renderer), vtable, unhooked ? " (its dispatch isn't hooked)" : "");
            continue;
        }
        auto** end = reinterpret_cast<uint32_t***>(second + 0x89A0);
        if (reinterpret_cast<uint8_t*>(*end) >= second + 0x89AC + 8 * 4) {
            Log("Scope view: no room for another entity renderer");
            break;
        }
        // Its AddRef: slot 0.
        using AddRefFn = void(__fastcall*)(void*, void*);
        reinterpret_cast<AddRefFn>(reinterpret_cast<uint32_t*>(vtable)[0])(renderer, nullptr);
        for (Borrowed& b : g_borrowed)
            if (vtable == b.vtable) {
                b.first = index;
                b.second = static_cast<int>(*end - *reinterpret_cast<uint32_t***>(second + 0x899C));
            }
        **end = renderer;
        *end += 1;
        ++shared;
        Log("Scope view: entity renderer %08X, vtable %08X, shared", reinterpret_cast<uintptr_t>(renderer), vtable);
    }
    Log("Scope view: %d entity renderers shared", shared);
}

// Its own settings: the factory's are the eyes' very object, which changed here would change them.
bool OwnSettings(uint8_t* second) {
    uint8_t* shared = *reinterpret_cast<uint8_t**>(second + 0x8988);
    if (!shared)
        return false;
    auto clone = reinterpret_cast<CloneFn>(0x00450060);
    auto release = reinterpret_cast<ReleaseFn>(0x00448320);
    uint8_t* own = clone(shared, nullptr, *reinterpret_cast<void**>(second + 0x14), nullptr, nullptr);
    if (!own)
        return false;
    InterlockedIncrement(reinterpret_cast<volatile LONG*>(own + 4));
    release(shared, nullptr);
    *reinterpret_cast<uint8_t**>(second + 0x8988) = own;
    // Nothing of the player's own body; no screen's effects, motion blur or anti-aliasing passes of its own; one sample; its sky from the eyes'; no planar or dynamic maps.
    own[0x19A] = 0;  // the foreground: first person
    own[0x1A6] = 0;
    own[0x204] = 0;  // motion blur
    own[0x233] = 0;  // FXAA
    own[0x1AC] = 0;  // the screen's effects
    own[0x1BE] = 0;  // lens flares
    own[0x1C1] = 0;
    own[0x1AB] = 0;  // dynamic environment map
    own[0x229] = 0;  // planar reflection
    own[0x1A1] = 0;  // sky environment map
    *reinterpret_cast<uint32_t*>(own + 0xDC) = 1;     // multisample
    *reinterpret_cast<uint32_t*>(own + 0x48) = 0x2000;  // visible entities: the frame's memory is shared
    // No spot lights' or transparencies' shadows (each shadowed spot light draws the scene again); its sun's shadows neither drawn nor applied unless asked for. (Its shadow maps stay made: with them
    // switched off altogether its passes read what isn't there.)
    own[0x1FB] = 0;
    *reinterpret_cast<uint32_t*>(own + 0xC0) = 0;
    own[0x22A] = 0;
    const bool shadows = g_shadows.load(std::memory_order_relaxed);
    own[0x211] = shadows ? 1 : 0;
    own[0x210] = shadows ? 1 : 0;
    return true;
}

bool Create(uint8_t* first, int size) {
    uint8_t* device = *reinterpret_cast<uint8_t**>(kDeviceHolder);
    if (!device)
        return false;
    RendererParams params{};
    params.name = *reinterpret_cast<const void**>(first + 0x18);
    params.post = *reinterpret_cast<void**>(first + 0x2C);
    params.decals = nullptr;
    params.width = params.height = static_cast<uint32_t>(size);
    void* const wasFirst = *reinterpret_cast<void**>(device + 0x310);
    void* const wasBack = *reinterpret_cast<void**>(device + 0x314);
    LARGE_INTEGER t0, t1;
    QueryPerformanceCounter(&t0);
    uint8_t* second = reinterpret_cast<FactoryFn>(kFactory)(*reinterpret_cast<void**>(first + 0x14), &params);
    // The device's callback goes to the newest renderer: back to the eyes'.
    *reinterpret_cast<void**>(device + 0x310) = wasFirst;
    *reinterpret_cast<void**>(device + 0x314) = wasBack;
    if (!second)
        return false;
    using AddRefFn = void(__fastcall*)(void*, void*);
    reinterpret_cast<AddRefFn>(reinterpret_cast<uint32_t*>(*reinterpret_cast<uint32_t*>(second))[0])(second, nullptr);
    if (!OwnSettings(second)) {
        Log("Scope view: its own settings could not be made");
        // Let go of it (its destructor clears the device's callback: the eyes' put back after).
        using ReleaseSlot = void(__fastcall*)(void*, void*);
        reinterpret_cast<ReleaseSlot>(reinterpret_cast<uint32_t*>(*reinterpret_cast<uint32_t*>(second))[1])(second, nullptr);
        *reinterpret_cast<void**>(device + 0x310) = wasFirst;
        *reinterpret_cast<void**>(device + 0x314) = wasBack;
        return false;
    }
    ShareRenderers(first, second);
    // Its own particles' renderer, made as the level makes the eyes' (0101D9A7-0101D9E2): theirs maps each view's particle buffers by the view's index, and the eyes' dispatch would map them again before
    // the second draws. Added the game's way (slot +20), a listener of its own renderer only.
    if (g_ownParticles.load(std::memory_order_relaxed)) {
        void* const allocator = *reinterpret_cast<void**>(first + 0x14);
        using AllocFn = void*(__fastcall*)(void* allocator, void*, uint32_t size, uint32_t align);
        using EmittersFn = void*(__fastcall*)(void* self, void*, void* allocator, void* settings);
        using GetterFn = void*(__fastcall*)(void* self, void*);
        using AddFn = void(__fastcall*)(void* self, void*, void* renderer);
        uint8_t* owner = *reinterpret_cast<uint8_t**>(0x02384D78);
        void* settings = owner ? reinterpret_cast<GetterFn>(reinterpret_cast<uint32_t*>(*reinterpret_cast<uint32_t*>(owner))[0x38 / 4])(owner, nullptr) : nullptr;
        void* memory = settings ? reinterpret_cast<AllocFn>(0x0041BE10)(allocator, nullptr, 0x1E550, 16) : nullptr;
        void* emitters = memory ? reinterpret_cast<EmittersFn>(0x0187E300)(memory, nullptr, allocator, settings) : nullptr;
        if (emitters) {
            reinterpret_cast<AddFn>(reinterpret_cast<uint32_t*>(*reinterpret_cast<uint32_t*>(second))[0x20 / 4])(second, nullptr, emitters);
            Log("Scope view: its own particles' renderer at %08X", reinterpret_cast<uintptr_t>(emitters));
        } else {
            Log("Scope view: its own particles' renderer could not be made");
        }
    }
    for (const Borrowed& b : g_borrowed)
        if (b.first >= 0)
            Log("Scope view: it draws the first eye's %s (%d in the eyes' list, %d in its own)", b.name, b.first, b.second);
    QueryPerformanceCounter(&t1);
    g_second = second;
    g_first = first;
    g_warm = true;  // dispatched once now, not drawn: its targets are rebuilt for its own settings at the level's start, not the first time a scope's raised
    Log("Scope view: a second renderer at %08X, %dx%d, made in %.1f ms; the eyes' at %08X", reinterpret_cast<uintptr_t>(second), size, size, static_cast<double>(t1.QuadPart - t0.QuadPart) * 1000.0 / static_cast<double>(g_ticksPerSecond.QuadPart), reinterpret_cast<uintptr_t>(first));
    return true;
}

// The terrain's object: the first dispatch writes per-view things into it; the second's are put back afterwards.
uint8_t* TerrainOf(uint8_t* first) {
    auto** from = *reinterpret_cast<uint32_t***>(first + 0x899C);
    auto** to = *reinterpret_cast<uint32_t***>(first + 0x89A0);
    for (auto** at = from; at != to; ++at)
        if (*at && **at == kTerrainVtable)
            return *reinterpret_cast<uint8_t**>(reinterpret_cast<uint8_t*>(*at) + 0xC);
    return nullptr;
}

void Normalize(float* v) {
    const float length = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (length > 1e-6f)
        v[0] /= length, v[1] /= length, v[2] /= length;
}

// Its camera for this frame: copied from the first eye's description, whatever eye's looking (its view's index picks things by it), with the rows and place of the sight's line in the eye's own camera.
void MakeDesc(const RenderParams* p, int size) {
    std::memcpy(g_desc, p->descs, kDescSize);
    auto* viewport = reinterpret_cast<uint16_t*>(g_desc + 0x10);
    viewport[0] = viewport[1] = 0;
    viewport[2] = viewport[3] = static_cast<uint16_t>(size);
    *reinterpret_cast<uint32_t*>(g_desc + 0x20) = 0;
    g_desc[0x1072] = 1;  // no motion blur passes
    std::lock_guard lock(g_rowsMutex);
    for (int e = 0; e < 2; ++e) {
        const auto* m = reinterpret_cast<const float*>(p->descs + e * kDescSize + kMainCamera);
        std::memcpy(g_eyeRows[e], m, sizeof(float) * 3);
        std::memcpy(g_eyeRows[e] + 3, m + 4, sizeof(float) * 3);
        std::memcpy(g_eyeRows[e] + 6, m + 8, sizeof(float) * 3);
    }
    const Line& line = g_line;
    const bool haveLine = line.eye == 0 || line.eye == 1;
    const auto* m = reinterpret_cast<const float*>(p->descs + (haveLine ? line.eye : 0) * kDescSize + kMainCamera);
    float back[3], up[3], right[3], place[3] = {m[12], m[13], m[14]};
    if (haveLine) {
        for (int i = 0; i < 3; ++i) {
            back[i] = -(line.z[0] * m[i] + line.z[1] * m[4 + i] + line.z[2] * m[8 + i]);
            up[i] = line.y[0] * m[i] + line.y[1] * m[4 + i] + line.y[2] * m[8 + i];
            place[i] += line.at[0] * m[i] + line.at[1] * m[4 + i] + line.at[2] * m[8 + i];
        }
    } else {
        std::memcpy(right, m, sizeof(right));
        std::memcpy(up, m + 4, sizeof(up));
        std::memcpy(back, m + 8, sizeof(back));
    }
    Normalize(back);
    const float along = up[0] * back[0] + up[1] * back[1] + up[2] * back[2];
    for (int i = 0; i < 3; ++i)
        up[i] -= along * back[i];
    Normalize(up);
    right[0] = up[1] * back[2] - up[2] * back[1];
    right[1] = up[2] * back[0] - up[0] * back[2];
    right[2] = up[0] * back[1] - up[1] * back[0];
    const float power = haveLine && line.power > 1.0f ? line.power : 3.44f;
    // As wide as the lens needs, with room for the line moving before the next: fitted, or a sharper picture's wasted on what the glass doesn't show.
    // No wider than the vignette shows (0.38) and its margin: wider only softens it.
    const float focal = haveLine && line.focal > 0.0f ? line.focal : 906.0f;
    const float wanted = std::fmin(std::fmax((haveLine ? (line.radius + line.off) / focal : 0.5f) + 0.07f, 0.2f), 0.45f);
    // Wider at once (or the lens's edge goes dark till it catches up); narrower slowly. Its level of detail follows a slow one of its own, or it pumps.
    g_c = wanted > g_c ? wanted : g_c + (wanted - g_c) * 0.05f;
    g_detailC = g_detailC > 0.0f ? g_detailC + (wanted - g_detailC) * 0.05f : wanted;
    float tangent = g_c / power;
    if (const float fixedFov = g_wantedFov.load(std::memory_order_relaxed); fixedFov > 0.0f)
        tangent = std::tan(fixedFov * 0.5f);
    const float fov = 2.0f * std::atan(tangent);
    const float detail = 2.0f * std::atan(g_detailC / g_detailRatio.load(std::memory_order_relaxed));  // the same level of detail however wide it's fitted: as far as the game's own zoom has it
    for (const size_t camera : {kMainCamera, kCullCamera}) {
        auto* c = reinterpret_cast<float*>(g_desc + camera);
        std::memcpy(c, right, sizeof(right));
        std::memcpy(c + 4, up, sizeof(up));
        std::memcpy(c + 8, back, sizeof(back));
        std::memcpy(c + 12, place, sizeof(place));
        c[0x48 / 4] = fov;
        c[0x4C / 4] = detail;
        c[0x50 / 4] = 0.1f;  // its near plane: nothing of the gun's in it
        c[0x58 / 4] = 1.0f;  // square, as its picture is (the lens reads x and y alike)
        *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(c) + 0x80) |= 0xD;  // all of it worked out again, whatever the eye's said
    }
    g_tangent.store(tangent, std::memory_order_relaxed);
    std::memcpy(g_rows, right, sizeof(right));
    std::memcpy(g_rows + 3, up, sizeof(up));
    std::memcpy(g_rows + 6, back, sizeof(back));
}

// Wanted while a magnified sight's lens is big enough to look through: on quickly, off only when it's clearly gone.
bool Wanted() {
    if (g_force.load(std::memory_order_relaxed))
        return true;
    Line line;
    {
        std::lock_guard lock(g_rowsMutex);
        line = g_line;
    }
    const ULONGLONG age = GetTickCount64() - line.when;
    // Past a slope of 0.38 from its line a scope shows black (the lens's vignette): a lens whose whole disc is past that shows nothing worth drawing.
    const float seen = 0.38f * (line.focal > 0.0f ? line.focal : 906.0f) + line.radius;
    if (!g_on.load(std::memory_order_relaxed)) {
        if (age < 100 && line.power > 1.5f && line.radius >= 80.0f && line.off < seen) {
            if (g_activeFrames.fetch_add(1, std::memory_order_relaxed) >= 1) {
                g_on.store(true, std::memory_order_relaxed);
                Log("Scope view: on: the lens %.0f pixels across, %.0f off its line, power %.2f, eye %d", line.radius * 2.0f, line.off, line.power, line.eye);
            }
        } else {
            g_activeFrames.store(0, std::memory_order_relaxed);
        }
        return g_on.load(std::memory_order_relaxed);
    }
    static int s_small = 0;
    s_small = line.radius < 55.0f || line.power <= 1.5f || line.off >= seen ? s_small + 1 : 0;
    if (age > 120 || s_small > 36) {
        g_on.store(false, std::memory_order_relaxed);
        g_activeFrames.store(0, std::memory_order_relaxed);
        s_small = 0;
        g_c = 0.0f;  // (the next look through starts fitted to its lens, not to how the last one ended)
        g_detailC = 0.0f;
        Log("Scope view: off");
    }
    return g_on.load(std::memory_order_relaxed);
}

void __fastcall DispatchHook(uint8_t* self, void*, RenderParams* params, Handle* out1, Handle* out2) {
    const int size = g_size.load(std::memory_order_relaxed);
    if (self != g_second && params && params->viewCount == 2 && params->descs && size > 0 && !g_failed) {
        if (!g_second && !Create(self, size)) {
            g_failed = true;
            Log("Scope view: the second renderer could not be made: left off");
        }
        const bool warm = std::exchange(g_warm, false);
        if (g_second && self == g_first && (warm || Wanted())) {
            LARGE_INTEGER t0, t1;
            QueryPerformanceCounter(&t0);
            uint8_t* terrain = TerrainOf(self);
            uint8_t kept[32];
            if (terrain)
                std::memcpy(kept, terrain + 0xA0, sizeof(kept));
            MakeDesc(params, size);
            RenderParams own = *params;
            own.viewCount = 1;
            own.descs = g_desc;
            own.dt = own.dt2 = 0.0f;
            Handle a, b;
            const auto init = reinterpret_cast<HandleInitFn>(0x009E6000);
            init(&a, nullptr);
            init(&b, nullptr);
            g_inSecondDispatch.store(true, std::memory_order_release);
            g_dispatch(g_second, nullptr, &own, &a, &b);
            const auto wait = reinterpret_cast<WaitFn>(0x00473C00);
            wait(&b);
            wait(&a);
            g_inSecondDispatch.store(false, std::memory_order_release);
            if (terrain)
                std::memcpy(terrain + 0xA0, kept, sizeof(kept));
            if (warm) {
                g_c = g_detailC = 0.0f;  // (fitted to the first real lens, not this one's guess)
                QueryPerformanceCounter(&t1);
                Log("Scope view: dispatched once as the level starts, %.1f ms (its targets made for its own settings)", static_cast<double>(t1.QuadPart - t0.QuadPart) * 1000.0 / static_cast<double>(g_ticksPerSecond.QuadPart));
                g_dispatch(self, nullptr, params, out1, out2);
                return;
            }
            g_readyFrame.store(params->frame, std::memory_order_release);
            QueryPerformanceCounter(&t1);
            g_dispatchTicks.fetch_add(t1.QuadPart - t0.QuadPart, std::memory_order_relaxed);
            g_dispatches.fetch_add(1, std::memory_order_relaxed);
        }
    }
    g_dispatch(self, nullptr, params, out1, out2);
}

// Whether an entity renderer's dispatch is for the second's view: its entry's view info names a view inside the second's object.
bool ForSecond(void* entries) {
    if (!g_second || !entries)
        return false;
    const auto info = Read<uintptr_t>(reinterpret_cast<uintptr_t>(entries));
    const auto view = info ? Read<uintptr_t>(info + 0x10) - 0x1F2F0 : 0;
    return view >= reinterpret_cast<uintptr_t>(g_second) && view < reinterpret_cast<uintptr_t>(g_second) + 0x48880;
}

// A borrowed renderer's dispatch for the second's view is passed over: its handles stay empty (nothing waited for), nothing's drawn till it's lent the eye's.
bool PassOver(void* entries) {
    return g_borrowTerrain.load(std::memory_order_relaxed) && (g_inSecondDispatch.load(std::memory_order_acquire) || ForSecond(entries));
}
void __fastcall TerrainDispatchHook(void* self, void*, uint32_t count, void* entries, Handle* h1, Handle* h2) {
    if (PassOver(entries))
        return;
    g_borrowedDispatch[0](self, nullptr, count, entries, h1, h2);
}
void __fastcall ParticlesDispatchHook(void* self, void*, uint32_t count, void* entries, Handle* h1, Handle* h2) {
    if (PassOver(entries))
        return;
    g_borrowedDispatch[1](self, nullptr, count, entries, h1, h2);
}

// The second's picture, drawn just before the eyes' passes.
void __fastcall FrameHook(uint8_t* self, void*) {
    if (self == g_first) {
        bool drawn = false;
        if (g_second && g_readyFrame.load(std::memory_order_acquire) == *reinterpret_cast<uint32_t*>(self + 0x8C00)) {
            g_readyFrame.store(0xFFFFFFFFu, std::memory_order_relaxed);
            // The first eye's terrain and particles, as its dispatch left them for this frame: only its own view's entries (sub-view 0), not its shadow slices' or reflections', which the second's view
            // hasn't the same of (their indices would run past its own).
            if (g_borrowTerrain.load(std::memory_order_relaxed)) {
                struct Entry {
                    uint32_t sub, kind;
                    void* list;
                };
                static Entry s_entries[2][512];
                // Each list still holding the borrowed renderer where it was found (a new level's list may differ).
                const auto holds = [](const uint8_t* renderer, int at, uint32_t vtable) {
                    auto** from = *reinterpret_cast<uint32_t** const*>(renderer + 0x899C);
                    auto** to = *reinterpret_cast<uint32_t** const*>(renderer + 0x89A0);
                    return at >= 0 && at < to - from && from[at] && *from[at] == vtable;
                };
                for (int k = 0; k < 2; ++k) {
                    Borrowed& b = g_borrowed[k];
                    if (!holds(self, b.first, b.vtable) || !holds(g_second, b.second, b.vtable))
                        continue;
                    const auto* from = reinterpret_cast<const uint32_t*>(self + kViewFirst + 0x1F2B0 + 8 * b.first);
                    const uint32_t count = from[0];
                    const auto* listed = reinterpret_cast<const Entry*>(static_cast<uintptr_t>(from[1]));
                    uint32_t kept = 0;
                    for (uint32_t i = 0; listed && i < count && kept < 512; ++i)
                        if (listed[i].sub == 0)
                            s_entries[k][kept++] = listed[i];
                    auto* to = reinterpret_cast<uint32_t*>(g_second + kViewFirst + 0x1F2B0 + 8 * b.second);
                    to[0] = kept;
                    to[1] = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(s_entries[k]));
                    b.kept.store(kept, std::memory_order_relaxed);
                    b.seen.store(count, std::memory_order_relaxed);
                }
            }
            // The eyes' sky map as their dispatch left it this frame (a pointer taken at the second's dispatch could be one the eyes' rebuild has freed since).
            *reinterpret_cast<uint32_t*>(g_second + kViewFirst + 0x1F2A0) = *reinterpret_cast<const uint32_t*>(self + kViewFirst + 0x1F2A0);
            LARGE_INTEGER t0, t1;
            QueryPerformanceCounter(&t0);
            g_drawing.store(true, std::memory_order_relaxed);
            g_render(g_second, nullptr);
            g_drawing.store(false, std::memory_order_relaxed);
            gputime::Stamp(gputime::kScopeView);
            QueryPerformanceCounter(&t1);
            g_renderTicks.fetch_add(t1.QuadPart - t0.QuadPart, std::memory_order_relaxed);
            g_renders.fetch_add(1, std::memory_order_relaxed);
            drawn = true;
        }
        g_pictureNow.store(drawn, std::memory_order_relaxed);
    }
    g_frame(self, nullptr);
}

// Its picture isn't tone-mapped or put on the window: the lens goes into the eyes' light, before theirs.
void __fastcall PostHook(void* self, void*, void* arg) {
    if (g_second && self == *reinterpret_cast<void**>(g_second + 0x34))
        return;
    g_post(self, nullptr, arg);
}

// The lights' list is one: the eyes' dispatch makes it, the second goes by the last frame's.
void __fastcall LightsHook(void* self, void*, void* a, void* b, void* c, void* d) {
    if (g_inSecondDispatch.load(std::memory_order_acquire))
        return;
    g_lights(self, nullptr, a, b, c, d);
}

// Its view: its sky's map is the eyes', no debug drawing.
void __fastcall ViewSetupHook(uint8_t* self, void*, void* a, uint8_t* view) {
    g_viewSetup(self, nullptr, a, view);
    if (self == g_second && g_first && view) {
        *reinterpret_cast<uint32_t*>(view + 0x1F2A0) = *reinterpret_cast<uint32_t*>(g_first + kViewFirst + 0x1F2A0);  // (the last frame's: FrameHook lends it this frame's before it's drawn)
        view[0x1FC09] = 0;
    }
}

// The level's renderer going: the second before it, into the same level's memory.
void* __fastcall DestroyHook(uint8_t* self, void*, uint32_t flags) {
    if (self == g_first && g_second) {
        uint8_t* second = g_second;
        g_second = nullptr;
        g_first = nullptr;
        g_pictureNow.store(false, std::memory_order_relaxed);
        g_readyFrame.store(0xFFFFFFFFu, std::memory_order_relaxed);
        for (Borrowed& b : g_borrowed)
            b.first = b.second = -1;
        using ReleaseSlot = void(__fastcall*)(void*, void*);
        reinterpret_cast<ReleaseSlot>(reinterpret_cast<uint32_t*>(*reinterpret_cast<uint32_t*>(second))[1])(second, nullptr);
        Log("Scope view: the second renderer let go of with the level's");
    }
    return g_destroy(self, nullptr, flags);
}

bool HookOne(uintptr_t at, void* detour, void** original, const char* name) {
    if (MH_CreateHook(reinterpret_cast<void*>(at), detour, original) != MH_OK || MH_EnableHook(reinterpret_cast<void*>(at)) != MH_OK) {
        Log("Scope view: could not hook %s", name);
        return false;
    }
    return true;
}

void Hook() {
    if (g_hooked || g_hookFailed)
        return;
    if (!SameCode(kDispatchAt, kDispatchStart, sizeof(kDispatchStart)) || !SameCode(kFrameAt, kFrameStart, sizeof(kFrameStart)) || !SameCode(kRenderAt, kRenderStart, sizeof(kRenderStart)) ||
        !SameCode(kPostAt, kPostStart, sizeof(kPostStart)) || !SameCode(kLightsAt, kLightsStart, sizeof(kLightsStart)) || !SameCode(kViewSetupAt, kViewSetupStart, sizeof(kViewSetupStart)) ||
        !SameCode(kDestroyAt, kDestroyStart, sizeof(kDestroyStart)) || !SameCode(kFactory, kFactoryStart, sizeof(kFactoryStart)))
        return;  // the code isn't there yet (it's decrypted at start-up), or it's another version: then never
    QueryPerformanceFrequency(&g_ticksPerSecond);
    g_render = reinterpret_cast<RenderFn>(kRenderAt);
    const bool ok = HookOne(kFrameAt, reinterpret_cast<void*>(&FrameHook), reinterpret_cast<void**>(&g_frame), "the frame's passes") &&
                    HookOne(kPostAt, reinterpret_cast<void*>(&PostHook), reinterpret_cast<void**>(&g_post), "the post") &&
                    HookOne(kLightsAt, reinterpret_cast<void*>(&LightsHook), reinterpret_cast<void**>(&g_lights), "the lights") &&
                    HookOne(kViewSetupAt, reinterpret_cast<void*>(&ViewSetupHook), reinterpret_cast<void**>(&g_viewSetup), "the view's setup") &&
                    HookOne(kDestroyAt, reinterpret_cast<void*>(&DestroyHook), reinterpret_cast<void**>(&g_destroy), "the destructor") &&
                    HookOne(kDispatchAt, reinterpret_cast<void*>(&DispatchHook), reinterpret_cast<void**>(&g_dispatch), "the dispatch");
    // The borrowed renderers' own dispatches, passed over for its view: here with the rest, at start-up, not in the middle of a level's frame. Each checked by its first bytes; one that isn't as read
    // stays unhooked, and its renderer then isn't shared (ShareRenderers).
    constexpr uintptr_t kBorrowedAt[2] = {0x009C5690, 0x01884A50};
    static constexpr uint8_t kBorrowedStart[2][12] = {{0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0, 0x81, 0xEC, 0xA4, 0x07, 0x00, 0x00}, {0x83, 0xEC, 0x4C, 0x53, 0x55, 0x56, 0x8B, 0xF1, 0xC7, 0x86, 0x40, 0xE5}};
    void* const detours[2] = {reinterpret_cast<void*>(&TerrainDispatchHook), reinterpret_cast<void*>(&ParticlesDispatchHook)};
    for (int k = 0; ok && k < 2; ++k) {
        int slot = -1;  // (its renderer's own: one of its table's first slots)
        for (int s = 0; s < 16 && slot < 0; ++s)
            if (Read<uint32_t>(g_borrowed[k].vtable + s * 4) == kBorrowedAt[k])
                slot = s;
        if (slot < 0 || !SameCode(kBorrowedAt[k], kBorrowedStart[k], sizeof(kBorrowedStart[k]))) {
            Log("Scope view: the %s' dispatch isn't as read: not borrowed", g_borrowed[k].name);
            continue;
        }
        void* original = nullptr;
        if (MH_CreateHook(reinterpret_cast<void*>(kBorrowedAt[k]), detours[k], &original) != MH_OK) {
            Log("Scope view: the %s' dispatch couldn't be hooked: not borrowed", g_borrowed[k].name);
            continue;
        }
        if (MH_EnableHook(reinterpret_cast<void*>(kBorrowedAt[k])) != MH_OK) {
            MH_RemoveHook(reinterpret_cast<void*>(kBorrowedAt[k]));
            Log("Scope view: the %s' dispatch couldn't be hooked: not borrowed", g_borrowed[k].name);
            continue;
        }
        g_borrowedDispatch[k] = reinterpret_cast<EntityDispatchFn>(original);
    }
    g_hooked = ok;
    g_hookFailed = !ok;
    Log("Scope view: %s", ok ? "hooked" : "not hooked: left off");
}

}  // namespace

void SetSize(int size) {
    if (g_size.exchange(size, std::memory_order_relaxed) != size)
        Log("Scope view: %d", size);
}

void SetFov(float radians) { g_wantedFov.store(radians, std::memory_order_relaxed); }

void SetForce(bool on) { g_force.store(on, std::memory_order_relaxed); }

void SetShare(int mask) { g_shareMask.store(mask, std::memory_order_relaxed); }

void SetDetail(float ratio) { g_detailRatio.store(ratio, std::memory_order_relaxed); }

void SetOwnTerrain(bool own) { g_borrowTerrain.store(!own, std::memory_order_relaxed); }

void SetShadows(bool on) { g_shadows.store(on, std::memory_order_relaxed); }

void SetParticles(bool on) { g_ownParticles.store(on, std::memory_order_relaxed); }

void PokeSetting(int offset, int bytes, float value) {
    if (!g_second || offset < 0 || offset > 0x23C)
        return;
    uint8_t* own = *reinterpret_cast<uint8_t**>(g_second + 0x8988);
    if (!own)
        return;
    if (bytes == 1)
        own[offset] = static_cast<uint8_t>(value);
    else if (bytes == 4)
        *reinterpret_cast<uint32_t*>(own + offset) = static_cast<uint32_t>(value);
    else
        *reinterpret_cast<float*>(own + offset) = value;
    Log("Scope view: its setting +%X now %g", offset, value);
}

void SettingsRead() { g_settingsRead.store(true, std::memory_order_relaxed); }

void OnFrame() {
    if (g_size.load(std::memory_order_relaxed) > 0 && g_settingsRead.load(std::memory_order_relaxed))
        Hook();
    const ULONGLONG now = GetTickCount64();
    if (now - g_lastLine < 1000)
        return;
    g_lastLine = now;
    const uint32_t dispatches = g_dispatches.exchange(0, std::memory_order_relaxed), renders = g_renders.exchange(0, std::memory_order_relaxed);
    if (!dispatches && !renders)
        return;
    const double toMs = 1000.0 / static_cast<double>(g_ticksPerSecond.QuadPart);
    const int64_t d = g_dispatchTicks.exchange(0, std::memory_order_relaxed), r = g_renderTicks.exchange(0, std::memory_order_relaxed);
    uint32_t used = 0, whole = 0;
    if (const auto holder = Read<uintptr_t>(kFrameMemoryHolder))
        if (const auto memory = Read<uintptr_t>(holder + 0x60)) {
            used = Read<uint32_t>(memory + 0x10);
            whole = Read<uint32_t>(memory + 0x4);
        }
    Log("Scope view: %u dispatches, %.2f ms each on the client thread; %u drawn, %.2f ms each on the render thread; the frame's memory %u of %u KB; lent by the eye: terrain %u of %u entries, particles %u of %u",
        dispatches, dispatches ? static_cast<double>(d) * toMs / dispatches : 0.0, renders, renders ? static_cast<double>(r) * toMs / renders : 0.0, used / 1024, whole / 1024,
        g_borrowed[0].kept.load(std::memory_order_relaxed), g_borrowed[0].seen.load(std::memory_order_relaxed), g_borrowed[1].kept.load(std::memory_order_relaxed),
        g_borrowed[1].seen.load(std::memory_order_relaxed));
}

ID3D11ShaderResourceView* Picture() {
    if (!g_second || !g_pictureNow.load(std::memory_order_relaxed))
        return nullptr;
    const auto texture = Read<uintptr_t>(reinterpret_cast<uintptr_t>(g_second) + 0x89F8);
    if (!texture)
        return nullptr;
    auto* view = Read<ID3D11ShaderResourceView*>(texture + 0x70);
    // Where the game keeps its view of a target isn't its own word: checked, once for each picture.
    static ID3D11ShaderResourceView* s_checked = nullptr;
    static bool s_good = false;
    if (view != s_checked) {
        s_checked = view;
        s_good = false;
        ID3D11ShaderResourceView* asked = nullptr;
        if (view && SUCCEEDED(view->QueryInterface(IID_PPV_ARGS(&asked))) && asked) {
            asked->Release();
            s_good = true;
            ID3D11Resource* resource = nullptr;
            view->GetResource(&resource);
            ID3D11Texture2D* target = nullptr;
            if (resource && SUCCEEDED(resource->QueryInterface(IID_PPV_ARGS(&target))) && target) {
                D3D11_TEXTURE2D_DESC d{};
                target->GetDesc(&d);
                Log("Scope view: its picture %ux%u, format %d, %u samples, bound %X", d.Width, d.Height, static_cast<int>(d.Format), d.SampleDesc.Count, d.BindFlags);
                if (d.SampleDesc.Count != 1)
                    s_good = false;
                target->Release();
            }
            if (resource)
                resource->Release();
        }
        if (!s_good)
            Log("Scope view: its picture can't be read as it's kept");
    }
    return s_good ? view : nullptr;
}

int PictureSize() { return g_size.load(std::memory_order_relaxed); }

bool PictureFrom(int eye, const float* v, float* rows) {
    if (eye < 0 || eye > 1)
        return false;
    float own[9], rowsJ[9];
    {
        std::lock_guard lock(g_rowsMutex);
        std::memcpy(own, g_rows, sizeof(own));
        std::memcpy(rowsJ, g_eyeRows[eye], sizeof(rowsJ));
    }
    const float cot = 1.0f / std::fmax(g_tangent.load(std::memory_order_relaxed), 1e-4f);
    for (int i = 0; i < 3; ++i) {
        float k[3];
        for (int c = 0; c < 3; ++c)
            k[c] = own[i * 3] * rowsJ[c * 3] + own[i * 3 + 1] * rowsJ[c * 3 + 1] + own[i * 3 + 2] * rowsJ[c * 3 + 2];
        const float scale = i == 2 ? -1.0f : cot;  // (its back is its row 2: what's in front has w positive)
        for (int a = 0; a < 3; ++a)
            rows[i * 4 + a] = scale * (k[0] * v[a * 4] + k[1] * v[a * 4 + 1] + k[2] * v[a * 4 + 2]);
        rows[i * 4 + 3] = 0.0f;
    }
    return true;
}

void NoteLens(int eye, const float* v, const float* scope, float radiusPixels, float offPixels, float power, float focal) {
    if (g_size.load(std::memory_order_relaxed) <= 0 || eye < 0 || eye > 1)
        return;
    std::lock_guard lock(g_rowsMutex);
    Line& line = g_line;
    // The eye looking through it: the one with the bigger lens, nearer its line; the other's only when that one's gone. (Another sight's, a reflex sight's, only once that one's gone.)
    const ULONGLONG now = GetTickCount64();
    if (now - line.when < 40 && ((line.eye != eye && radiusPixels - offPixels < line.radius - line.off) || (power <= 1.5f && line.power > 1.5f)))
        return;
    line.focal = focal;
    for (int c = 0; c < 3; ++c) {
        line.z[c] = scope[2] * v[c] + scope[6] * v[4 + c] + scope[10] * v[8 + c];
        line.y[c] = scope[1] * v[c] + scope[5] * v[4 + c] + scope[9] * v[8 + c];
        line.at[c] = (scope[3] - v[3]) * v[c] + (scope[7] - v[7]) * v[4 + c] + (scope[11] - v[11]) * v[8 + c];
    }
    line.radius = radiusPixels;
    line.off = offPixels;
    line.power = power;
    line.eye = eye;
    line.when = now;
}

void NotePlainDraw(ID3D11DeviceContext*) {}

}

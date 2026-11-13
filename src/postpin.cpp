#include "postpin.h"

#include <d3d11_1.h>
#include <MinHook.h>

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <new>

#include "config.h"
#include "log.h"
#include "scopeview.h"
#include "shaders.h"
#include "views.h"

// What the shaders do (fxc /dumpbin of the game's own, 2026-11-12):
//   the 24 tone map shaders, after the colour grading:   add r1.xy, v2.xyxx, l(-0.5, -0.5)
//                                                        mul r1.xy, r1.xyxx, cb0[24].xyxx      vignetteParams.xy (offset 384)
//                                                        dp2 r0.w, r1.xyxx, r1.xyxx
//                                                        log, mul by cb0[24].z, exp            (d.d) to the power vignetteParams.z
//                                                        mul r0.w, r0.w, cb0[25].w             vignetteColor.w
//                                                        colour times lerp(1, vignetteColor.rgb, that)
//   the suppression blur's clear middle (2b2ca40a51aa1b4d): the same first five lines with cb0[20] (dofParams, offset 320), then add_sat dofParams.w into the
//                                                        mask's alpha.
// v2 is the picture's uv (the same register samples the whole picture, t0), 0 to 1 across it, so v2 - 0.5 = T / (2 tan(half the angle)) for a pixel whose
// angle from the middle has tangent T (per axis: a flat picture is even in tangent). With .xy times tan(half)/tan(50 degrees), (v2 - 0.5) x .xy is T x .xy /
// (2 tan 50), the base picture's own at that angle; .z and the colour are untouched, so the darkening at every angle is the base's. The only difference is the
// rounding of one multiply in each of the two numbers (a few parts in 10 million). The pixels are the base's too: 2496 - 2160 = 336, 168 each side, a whole number.
//
// What a draw reads from a constant buffer (D3D11's rules, as relied on here):
//   - On the immediate context: what the last write before it in the immediate context's own order left. The writes are Map/Unmap, UpdateSubresource,
//     CopyResource and CopySubresourceRegion made on it, and those recorded in a command list, which ExecuteCommandList performs there in their recorded order
//     at the point it is called. The game must call the immediate context from one thread at a time, so the order the hooks see is that order.
//   - On a deferred context: what was last written on that same context since its last FinishCommandList. Before such a write, what it reads depends on what
//     the immediate context will have when the list is executed: not known when it is recorded, so such a draw is left alone.
//   - A dynamic buffer mapped with WRITE_DISCARD on a deferred context: its contents belong to that command list. Whether the immediate context sees them after
//     the list is executed is not said plainly by the documentation, so that one is not carried over: the immediate context's copy is ended there (counted).
//   - UpdateSubresource of a constant buffer takes no box in D3D11.0; with a box (11.1's partial update) only the part is new: no copy. The caller's data is
//     read during the call, so it is read here, after the call has returned, from the same pointer. (The deferred context's box bug moves the source by the
//     box's start: with no box, or one starting at 0, there is nothing to move.) A call the runtime refuses (a box past the end, a buffer not of default usage
//     for UpdateSubresource, buffers of two sizes or an immutable one for CopyResource) leaves no copy: whether it changed anything is not assumed.
//   - The learned buffers and the deferred contexts with a place are held (AddRef) until forgotten: a pointer is never another object's while it is in the table.
// So every write of a learned buffer on every context is followed, and the copy is either all of the buffer as the card will read it at that draw or none.

namespace postpin {

namespace detail {
std::atomic<bool> g_on{false};
}

namespace {

enum Kind { kVignette, kClearMiddle, kKinds };
constexpr int kLuminance = kKinds;  // the exposure's first luminance pass: looked at (the exposure's census below), never changed
constexpr UINT kOffset[kKinds] = {384, 320};  // vignetteParams, dofParams: their .xy
const char* const kNames[kKinds + 1] = {"the vignette", "the suppression blur's clear middle", "the exposure's first luminance pass"};

// The tone map's shaders with the vignette: all 24 read it with the same five lines above (checked one by one).
constexpr uint64_t kVignetteShaders[] = {
    0x1e3cc4f722828063ull, 0x2202888254341b3full, 0x4a68a7d8c656f62full, 0x4bde44cebb47744full, 0x4c8a6e7a3f3cf5cfull, 0x50bbbb8d7dae7395ull,
    0x5529d7a113e84521ull, 0x63ccb61c2b49de48ull, 0x793c41549bf65acaull, 0x8ebbb149db7c8ff5ull, 0x9707a48fa7242414ull, 0xa7863ecf9fc01d34ull,
    0xad6a9b37e7ec90edull, 0xb0b4d43e8eba01afull, 0xb848a3010fd7db08ull, 0xbe572a84984e320cull, 0xce306fd8b5b5ec45ull, 0xd8848e79e827218full,
    0xdfba3d10934ec088ull, 0xe429f87b5975289dull, 0xe5f5aa5ddfaeb067ull, 0xe82f10ef9242e282ull, 0xecb3997a9036970cull, 0xf9050ad270356adfull};
// The suppression blur's clear middle. The other shader that reads dofParams (94a5f6c1e17c947d) is a depth of field by distance: its dofParams are depths,
// not reaches, and are left alone (it isn't in this list).
constexpr uint64_t kClearMiddleShader = 0x2b2ca40a51aa1b4dull;
// The log of the luminance over the picture (4 taps and 1 tap): the start of the exposure's average.
constexpr uint64_t kLuminanceShaders[] = {0x05a7d00f23bf6721ull, 0x63fc0a4c68c154daull};

int KindOfChecksum(uint64_t c) {
    for (uint64_t v : kVignetteShaders)
        if (v == c)
            return kVignette;
    for (uint64_t v : kLuminanceShaders)
        if (v == c)
            return kLuminance;
    return c == kClearMiddleShader ? kClearMiddle : -1;
}

// The shaders as the game made them (their pointers change every start), each with its kind.
// A shader the game lets go isn't seen going, so when the list is full the oldest entry makes room (a live one lost that way is only left unpinned).
constexpr int kMostShaders = 128;
std::atomic<uintptr_t> g_shaders[kMostShaders];
std::atomic<int> g_shaderKinds[kMostShaders];
std::atomic<bool> g_madeLogged[kKinds + 1];
std::mutex g_shaderMutex;
int g_nextShader = 0;  // under the mutex: where the next one goes once the list is full
bool g_shadersFullLogged = false;  // under the mutex

int KindOf(ID3D11PixelShader* ps) {
    const auto p = reinterpret_cast<uintptr_t>(ps);
    for (int i = 0; i < kMostShaders; ++i)
        if (g_shaders[i].load(std::memory_order_acquire) == p)
            return g_shaderKinds[i].load(std::memory_order_relaxed);
    return -1;
}

// ---- What the card will read from each learned buffer ----
// The constant buffers these shaders were seen drawing with: only their writes are followed. Read without the lock by the hooks' first look (Known); changed
// only under g_state.
constexpr int kMostBuffers = 16;
std::atomic<uintptr_t> g_buffers[kMostBuffers];
std::atomic<bool> g_anyBuffer{false};  // the hooks' first test: false (one load, nothing else) until a buffer is learned, which only happens on and wider
std::atomic<bool> g_buffersFullLogged{false};
std::atomic<bool> g_hooked{false};  // UpdateSubresource, the copies and the command lists followed: without them no copy can be trusted, so none is drawn

int Known(const void* buffer) {
    const auto p = reinterpret_cast<uintptr_t>(buffer);
    if (!p)
        return -1;
    for (int i = 0; i < kMostBuffers; ++i)
        if (g_buffers[i].load(std::memory_order_relaxed) == p)
            return i;
    return -1;
}

// The biggest copy taken: 65536 bytes, 4096 registers of 16 bytes (D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT). That is the biggest constant buffer D3D11.0 lets
// be made and the most a shader can read from one bound whole (cb0[4095]); a bigger one (11.1) can only be read through a range, which is refused below anyway.
// The game's tone map draws from a 12800-byte buffer (seen 2026-11-12: one dynamic buffer, mapped WRITE_DISCARD about 160 times a frame on the immediate
// context, the tone map's draw among the draws it feeds). Every whole map of a learned buffer is copied at its Unmap, since which one a vignette draw will
// follow isn't known then; the storage of the last copy is filled again in place (no allocation per map), and only the map just before a draw is the copy it gets.
constexpr UINT kMostBytes = 65536;
// All copies held at once, at most (a guard: one buffer on the immediate context holds one copy; command lists waiting could hold more).
constexpr int64_t kMostHeldBytes = 8 * 1024 * 1024;
std::atomic<int64_t> g_heldBytes{0};
// Copies taken at Unmap or UpdateSubresource, their bytes and the time spent on them, for the 30 s line (the cost per frame).
std::atomic<uint32_t> g_copiesTaken{0};
std::atomic<uint64_t> g_copiedBytes{0};
std::atomic<int64_t> g_copyTicks{0};
// How a whole copy was made.
enum Way : uint8_t { kWayMap, kWayUpdate, kWayCopy, kWays };
const char* const kWayNames[kWays] = {"Map (WRITE_DISCARD)", "UpdateSubresource", "CopyResource from a copied buffer"};
// Why a draw found none.
enum Why : uint8_t {
    kWhyNotWritten,     // deferred: nothing written on its context since its command list began; immediate: no write seen since the buffer was learned
    kWhyMapUnfollowed,  // a Map whose Unmap brought no data (the scope's own drawing, a fifth map at once on a thread)
    kWhyMapAppend,      // WRITE_NO_OVERWRITE: only part of it new
    kWhyTooBig,         // more than kMostBytes
    kWhyUpdateBox,      // UpdateSubresource with a box short of the whole buffer
    kWhyCopyUnknown,    // CopyResource from a buffer with no copy, or CopySubresourceRegion
    kWhyListMap,        // a command list's Map of it, executed on the immediate context (not carried: see the rules above)
    kWhyListUnseen,     // a command list executed whose recording wasn't followed whole
    kWhyRange,          // bound with an offset or a count (PSSetConstantBuffers1): the copy would not be what the shader reads
    kWhyNoBuffer,       // nothing bound in slot 0
    kWhyFull,           // more buffers than kMostBuffers
    kWhyNotHooked,      // the hooks above could not be made
    kWhyDiscarded,      // DiscardResource: its contents undefined till written again
    kWhyUpdateUsage,    // UpdateSubresource of a buffer not of default usage (the runtime refuses it: what the card has is not known)
    kWhyHeldFull,       // all copies held together past kMostHeldBytes
    kWhys
};
const char* const kWhyNames[kWhys] = {"nothing written on its context since its command list began (or since learned)",
                                      "a Map not followed (no data at Unmap)",
                                      "WRITE_NO_OVERWRITE (part of it)",
                                      "bigger than 65536 bytes",
                                      "UpdateSubresource with a box",
                                      "a copy from something without a copy",
                                      "a command list's Map, executed (not carried)",
                                      "a command list executed whose recording wasn't followed",
                                      "bound with an offset (PSSetConstantBuffers1)",
                                      "no buffer in slot 0",
                                      "past 16 buffers",
                                      "hooks missing",
                                      "DiscardResource",
                                      "UpdateSubresource of a buffer not of default usage",
                                      "copies held past 8 MB"};

// A whole copy. Once it is in more than one place (an entry and a draw reading it, an entry and a command list's record) it is never changed: only a blob held
// by one entry alone is filled again in place (TakeCopy). Every new reference to a blob is made under g_state, so a use count of 1 seen under it stays 1.
struct Blob {
    UINT size = 0;
    UINT capacity = 0;
    Way way = kWayMap;
    std::unique_ptr<uint8_t[]> bytes;
    Blob() = default;
    Blob(const Blob&) = delete;
    Blob& operator=(const Blob&) = delete;
    ~Blob() {
        g_heldBytes.fetch_sub(capacity, std::memory_order_relaxed);
    }
};
using BlobPtr = std::shared_ptr<Blob>;

// Storage for a copy of size bytes, or null past kMostHeldBytes (or out of memory).
BlobPtr NewBlob(UINT size) {
    if (g_heldBytes.fetch_add(size, std::memory_order_relaxed) + size > kMostHeldBytes) {
        g_heldBytes.fetch_sub(size, std::memory_order_relaxed);
        return nullptr;
    }
    auto b = std::make_shared<Blob>();
    b->capacity = size;  // counted from here: the destructor gives it back
    b->bytes.reset(new (std::nothrow) uint8_t[size]);
    if (!b->bytes)
        return nullptr;
    return b;
}

BlobPtr MakeBlob(const void* data, UINT size, Way way) {
    BlobPtr b = NewBlob(size);
    if (!b)
        return nullptr;
    b->size = size;
    b->way = way;
    std::memcpy(b->bytes.get(), data, size);
    return b;
}

// One buffer's contents on one context, or in one finished command list.
struct Entry {
    bool written = false;  // in a recording or a list: written in it. On the immediate context: anything known (blob or why)
    bool open = false;     // mapped, Unmap not yet seen
    bool viaList = false;  // on the immediate context: carried there by an executed command list
    bool byMap = false;    // last written by a Map (followed or not): in a command list, not carried to the context executing it
    Why why = kWhyNotWritten;
    BlobPtr blob;   // all of it, or none
    BlobPtr spare;  // storage of an earlier copy no longer valid, held by this entry alone: the next copy goes there (never read as contents)
};

// Under g_state: the entry's copy ends (a refill begins, or a write that leaves none); its storage is kept for the next one if nothing else holds it.
void Drop(Entry* e) {
    if (e->blob && e->blob.use_count() == 1)
        e->spare = std::move(e->blob);
    e->blob = nullptr;
}

// Under g_state: the entry's copy becomes all size bytes of data, in the storage it already has when no one else holds it (no allocation per map); a new
// one only for the first copy, a bigger buffer, or while a draw or a command list still holds the last.
void TakeCopy(Entry* e, const void* data, UINT size, Way way) {
    LARGE_INTEGER t0, t1;
    QueryPerformanceCounter(&t0);
    BlobPtr b;
    if (e->blob && e->blob.use_count() == 1)
        b = std::move(e->blob);
    else if (e->spare && e->spare.use_count() == 1)
        b = std::move(e->spare);
    e->blob = nullptr;
    e->spare = nullptr;
    if (!b || b->capacity < size) {
        b = nullptr;  // gives back the smaller one before the new is counted
        b = NewBlob(size);
    }
    if (!b) {
        e->why = kWhyHeldFull;
        return;
    }
    b->size = size;
    b->way = way;
    std::memcpy(b->bytes.get(), data, size);
    e->blob = std::move(b);
    QueryPerformanceCounter(&t1);
    g_copiesTaken.fetch_add(1, std::memory_order_relaxed);
    g_copiedBytes.fetch_add(size, std::memory_order_relaxed);
    g_copyTicks.fetch_add(t1.QuadPart - t0.QuadPart, std::memory_order_relaxed);
}

// A deferred context's recording since its last FinishCommandList. start: g_seq when it began (0: not seen beginning, so any buffer may have been written in it
// before it was learned). The context is held (AddRef) while it has a place, so its pointer can't be handed to a new context that would find this one's
// writes; a place is never taken from a context (with all 32 held, a further context's writes go unfollowed: its draws find nothing, its lists are unseen).
constexpr int kMostRecordings = 32;
struct Recording {
    ID3D11DeviceContext* ctx = nullptr;
    uint32_t start = 0;
    Entry e[kMostBuffers];
};
// A finished command list, until it is executed (and after: it can be executed again).
constexpr int kMostLists = 256;
struct Finished {
    const void* list = nullptr;
    uint32_t start = 0;
    Entry e[kMostBuffers];
};

std::mutex g_state;  // everything below; never held while calling the device context
Entry g_immediate[kMostBuffers];
Recording g_recordings[kMostRecordings];
bool g_recordingsFull = false;
Finished g_lists[kMostLists];
int g_nextList = 0;
uint32_t g_seq = 0;
uint32_t g_learnedAt[kMostBuffers];

// Under g_state: the index if the buffer is (still) learned.
int Confirmed(const void* buffer) {
    const int i = Known(buffer);
    return i >= 0 && g_buffers[i].load(std::memory_order_relaxed) == reinterpret_cast<uintptr_t>(buffer) ? i : -1;
}

// Under g_state. Null when it has no place and make is false, or all places are held.
Recording* RecordingOf(ID3D11DeviceContext* ctx, bool make) {
    Recording* empty = nullptr;
    for (Recording& r : g_recordings) {
        if (r.ctx == ctx)
            return &r;
        if (!r.ctx && !empty)
            empty = &r;
    }
    if (!make)
        return nullptr;
    if (!empty) {
        g_recordingsFull = true;
        return nullptr;
    }
    *empty = Recording{};
    empty->ctx = ctx;
    ctx->AddRef();  // only counts up: nothing of the device is called
    empty->start = 0;  // its beginning not seen
    return empty;
}

// Under g_state. Null for a deferred context with no place: its write is then not followed, which leaves its draws and its lists alone.
Entry* EntryOf(ID3D11DeviceContext* ctx, bool immediate, int i) {
    if (immediate)
        return &g_immediate[i];
    Recording* r = RecordingOf(ctx, true);
    return r ? &r->e[i] : nullptr;
}

// Everything forgotten: switched off, or back at the base width. Nothing is then read or written by the hooks until a buffer is learned again. The buffers
// and contexts held are let go after the lock (a last Release may go into the runtime).
void Forget() {
    IUnknown* held[kMostBuffers + kMostRecordings] = {};
    int n = 0;
    {
        std::lock_guard lock(g_state);
        g_anyBuffer.store(false, std::memory_order_relaxed);
        for (auto& b : g_buffers)
            if (const uintptr_t p = b.exchange(0, std::memory_order_relaxed))
                held[n++] = reinterpret_cast<ID3D11Buffer*>(p);
        for (Entry& e : g_immediate)
            e = Entry{};
        for (Recording& r : g_recordings) {
            if (r.ctx)
                held[n++] = r.ctx;
            r = Recording{};
        }
        g_recordingsFull = false;
        for (Finished& f : g_lists)
            f = Finished{};
        g_buffersFullLogged.store(false, std::memory_order_relaxed);
    }
    for (int k = 0; k < n; ++k)
        held[k]->Release();
}

// ---- The diagnostic: how the learned buffers were written, and what the draws found ----
enum Write {
    kWMap,             // NoteMap
    kWMapWhole,        // ... and its Unmap brought all of it (WRITE_DISCARD)
    kWMapAppend,       // ... WRITE_NO_OVERWRITE
    kWUpdateWhole,     // UpdateSubresource, no box or one over all of it
    kWUpdateBox,       // ... with a smaller box
    kWCopyResource,    // as CopyResource's destination
    kWCopyRegion,      // as CopySubresourceRegion's destination
    kWListCarried,     // an executed command list's UpdateSubresource or copy of it, carried to the executing context
    kWListMap,         // an executed command list's Map of it (the executing context's copy ended)
    kWListUnseen,      // an executed command list whose recording wasn't followed (the executing context's copy ended)
    kWDiscard,         // DiscardResource (11.1)
    kWStructureCount,  // as CopyStructureCount's destination
    kWrites
};
const char* const kWriteNames[kWrites] = {"Map",
                                          "of them whole (WRITE_DISCARD)",
                                          "of them WRITE_NO_OVERWRITE",
                                          "UpdateSubresource whole",
                                          "UpdateSubresource with a box",
                                          "CopyResource as destination",
                                          "CopySubresourceRegion as destination",
                                          "executed command lists carrying an UpdateSubresource or copy",
                                          "executed command lists carrying a Map (not carried)",
                                          "executed command lists not followed",
                                          "DiscardResource",
                                          "CopyStructureCount as destination"};
std::atomic<uint32_t> g_writes[kWrites][2][2];  // [what][deferred][on another thread than the vignette's last draw]
std::atomic<DWORD> g_drawThread{0};
std::atomic<DWORD> g_drawThreadWas{0};
std::atomic<uint32_t> g_drawThreads{0};  // times the drawing thread changed
std::atomic<uint32_t> g_drawsOn[2];      // [deferred]
std::atomic<uint32_t> g_scaledBy[kWays][2];  // [way][through an executed command list]
std::atomic<uint32_t> g_leftBy[kWhys];
std::atomic<uint32_t> g_copySources[4];  // CopyResource into a learned buffer, by the source's usage

// The immediate context's thread: the order of its writes is only known if it is called from one thread at a time. Counted where its learned buffers' writes
// and the pinned draws on it come from another thread than the last (in turn is sound; at once from two threads it would not be: then this is not 0).
std::atomic<DWORD> g_immediateThread{0};
std::atomic<uint32_t> g_immediateThreads{0};

void NoteImmediateThread() {
    const DWORD t = GetCurrentThreadId();
    const DWORD was = g_immediateThread.exchange(t, std::memory_order_relaxed);
    if (was && was != t)
        g_immediateThreads.fetch_add(1, std::memory_order_relaxed);
}

void Count(Write w, bool deferred) {
    if (!deferred)
        NoteImmediateThread();
    const bool other = GetCurrentThreadId() != g_drawThread.load(std::memory_order_relaxed);
    g_writes[w][deferred][other].fetch_add(1, std::memory_order_relaxed);
}

bool IsImmediate(ID3D11DeviceContext* ctx) {
    return ctx->GetType() == D3D11_DEVICE_CONTEXT_IMMEDIATE;
}

const char* UsageName(D3D11_USAGE u) {
    switch (u) {
    case D3D11_USAGE_DEFAULT: return "default";
    case D3D11_USAGE_IMMUTABLE: return "immutable";
    case D3D11_USAGE_DYNAMIC: return "dynamic";
    case D3D11_USAGE_STAGING: return "staging";
    }
    return "?";
}

// A resource in a few words, for the log.
void Describe(ID3D11Resource* r, char* out, size_t n) {
    if (!r) {
        std::snprintf(out, n, "nothing");
        return;
    }
    D3D11_RESOURCE_DIMENSION dim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    r->GetType(&dim);
    if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D) {
        D3D11_TEXTURE2D_DESC d{};
        static_cast<ID3D11Texture2D*>(r)->GetDesc(&d);
        std::snprintf(out, n, "a %ux%u texture (format %u, %u mips, %u in the array, %u samples, usage %s, bind 0x%x, CPU 0x%x)", d.Width, d.Height,
                      static_cast<unsigned>(d.Format), d.MipLevels, d.ArraySize, d.SampleDesc.Count, UsageName(d.Usage), d.BindFlags, d.CPUAccessFlags);
    } else if (dim == D3D11_RESOURCE_DIMENSION_BUFFER) {
        D3D11_BUFFER_DESC d{};
        static_cast<ID3D11Buffer*>(r)->GetDesc(&d);
        std::snprintf(out, n, "a %u-byte buffer (usage %s, bind 0x%x, CPU 0x%x, misc 0x%x)", d.ByteWidth, UsageName(d.Usage), d.BindFlags, d.CPUAccessFlags,
                      d.MiscFlags);
    } else {
        std::snprintf(out, n, "a resource of dimension %d", static_cast<int>(dim));
    }
}

// ---- The exposure's census (a finding, nothing changed) ----
// For two minutes after the pin first works: the first luminance pass's draw (what it reads, where it writes, its viewport), every small draw that then reads
// what the chain wrote, and every copy out of it (a staging texture there is the average read back by the game). Each step once; at most 24 lines.
constexpr int kMostChain = 12;
std::mutex g_censusMutex;
const void* g_chain[kMostChain];  // compared only, never used
int g_chainCount = 0;  // under the mutex
std::atomic<int> g_chainSize{0};
std::atomic<bool> g_censusOn{false};
std::atomic<ULONGLONG> g_censusUntil{0};
std::atomic<int> g_censusLines{0};
constexpr int kMostCensusLines = 24;

bool InChain(const void* r) {
    for (int i = 0; i < g_chainCount; ++i)
        if (g_chain[i] == r)
            return true;
    return false;
}

bool AddToChain(const void* r) {
    if (!r || InChain(r) || g_chainCount >= kMostChain)
        return false;
    g_chain[g_chainCount++] = r;
    g_chainSize.store(g_chainCount, std::memory_order_relaxed);
    return true;
}

bool CensusLine() {
    return g_censusLines.fetch_add(1, std::memory_order_relaxed) < kMostCensusLines;
}

void CensusDraw(ID3D11DeviceContext* ctx, ID3D11PixelShader* ps, int kind, UINT count) {
    if (kind != kLuminance && !g_chainSize.load(std::memory_order_relaxed))
        return;
    ID3D11ShaderResourceView* views[4] = {};
    ctx->PSGetShaderResources(0, 4, views);
    ID3D11Resource* read[4] = {};
    D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc[4] = {};
    for (int k = 0; k < 4; ++k)
        if (views[k]) {
            views[k]->GetResource(&read[k]);
            views[k]->GetDesc(&viewDesc[k]);
            views[k]->Release();
        }
    ID3D11RenderTargetView* rtv = nullptr;
    ctx->OMGetRenderTargets(1, &rtv, nullptr);
    ID3D11Resource* target = nullptr;
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
    if (rtv) {
        rtv->GetResource(&target);
        rtv->GetDesc(&rtvDesc);
        rtv->Release();
    }
    D3D11_VIEWPORT vp{};
    UINT viewports = 1;
    ctx->RSGetViewports(&viewports, &vp);
    int from = -1;
    bool log = false;
    {
        std::lock_guard lock(g_censusMutex);
        for (int k = 0; k < 4 && from < 0; ++k)
            if (read[k] && InChain(read[k]))
                from = k;
        if (kind == kLuminance)
            log = AddToChain(target);
        else if (from >= 0)
            log = AddToChain(target);
    }
    if (log && CensusLine()) {
        const int slot = kind == kLuminance ? 0 : from;
        char src[256], dst[256];
        Describe(read[slot], src, sizeof(src));
        Describe(target, dst, sizeof(dst));
        ID3D11VertexShader* vs = nullptr;
        ctx->VSGetShader(&vs, nullptr, nullptr);
        const unsigned long long vsSum = vs ? shaders::ChecksumOfVs(vs) : 0;
        if (vs)
            vs->Release();
        const unsigned mip = viewDesc[slot].ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2D ? viewDesc[slot].Texture2D.MostDetailedMip : 0;
        const unsigned mips = viewDesc[slot].ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2D ? viewDesc[slot].Texture2D.MipLevels : 0;
        const unsigned rtMip = rtvDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D ? rtvDesc.Texture2D.MipSlice : 0;
        Log("Post pin: exposure: %s %016llx (%u vertices, vertex shader %016llx, the %s context) reads %s (slot %d, from mip %u, %u mips) and writes %s (mip %u), "
            "viewport %.1f %.1f %.1f x %.1f%s",
            kind == kLuminance ? "the first luminance pass" : "then", static_cast<unsigned long long>(shaders::ChecksumOf(ps)), count, vsSum,
            IsImmediate(ctx) ? "immediate" : "a deferred", src, slot, mip, mips, dst, rtMip, vp.TopLeftX, vp.TopLeftY, vp.Width, vp.Height,
            viewports ? "" : " (none set)");
    }
    for (ID3D11Resource* r : read)
        if (r)
            r->Release();
    if (target)
        target->Release();
}

void CensusCopy(ID3D11DeviceContext* ctx, const char* how, ID3D11Resource* dst, ID3D11Resource* src, UINT srcSub) {
    if (!g_censusOn.load(std::memory_order_relaxed) || !g_chainSize.load(std::memory_order_relaxed))
        return;
    bool log = false;
    {
        std::lock_guard lock(g_censusMutex);
        log = InChain(src) && AddToChain(dst);
    }
    if (log && CensusLine()) {
        char a[256], b[256];
        Describe(src, a, sizeof(a));
        Describe(dst, b, sizeof(b));
        Log("Post pin: exposure: %s of %s (subresource %u) into %s, on the %s context", how, a, srcSub, b, IsImmediate(ctx) ? "immediate" : "a deferred");
    }
}

// ---- This thread's own buffers the scaled copy is drawn with ----
// Default usage, filled with UpdateSubresource (not through the hooked Map; the UpdateSubresource hook passes them by: they are never learned), one for each size
// drawn (the vignette's and the clear middle's constants need not be the same size: one buffer remade for each would be a CreateBuffer at every draw).
struct Own {
    ID3D11Buffer* buffer = nullptr;
    UINT size = 0;
};
constexpr int kOwn = 2;
thread_local Own t_own[kOwn];
thread_local int t_nextOwn = 0;

ID3D11Buffer* OwnBuffer(ID3D11DeviceContext* ctx, UINT size) {
    for (const Own& o : t_own)
        if (o.buffer && o.size == size)
            return o.buffer;
    Own& o = t_own[t_nextOwn];
    t_nextOwn = (t_nextOwn + 1) % kOwn;
    if (o.buffer)
        o.buffer->Release();  // a command list still holding it keeps it alive
    o = {};
    ID3D11Device* device = nullptr;
    ctx->GetDevice(&device);
    if (!device)
        return nullptr;
    D3D11_BUFFER_DESC desc{size, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0};
    ID3D11Buffer* made = nullptr;
    const HRESULT hr = device->CreateBuffer(&desc, nullptr, &made);
    device->Release();
    if (FAILED(hr) || !made)
        return nullptr;
    o.buffer = made;
    o.size = size;
    return made;
}

std::atomic<int> g_test{-1};  // weapon.txt "postpin"

bool Wanted() {
    const int t = g_test.load(std::memory_order_relaxed);
    return t >= 0 ? t != 0 : config::Get().pinEffects;
}

std::atomic<bool> g_anyShader{false};

void Refresh() {
    const bool on = Wanted() && g_anyShader.load(std::memory_order_relaxed);
    if (!detail::g_on.exchange(on, std::memory_order_relaxed) || on)
        return;
    Forget();  // switched off: nothing followed or kept from here on
}

// The picture's half-width in tangent over the base's (100 degrees), from the angle the eyes were last drawn with; 0 when it is the base's (or not known).
constexpr double kBaseTangent = 1.19175359259421;  // tan(50 degrees)
thread_local float t_fov = -1.0f;
thread_local float t_factor = 0.0f;

float Factor(float& fov) {
    fov = views::RenderedFov();
    if (fov != t_fov) {
        t_fov = fov;
        const double r = fov > 0.0f ? std::tan(0.5 * static_cast<double>(fov)) / kBaseTangent : 1.0;
        t_factor = std::isfinite(r) && r > 0.2 && r < 5.0 && std::fabs(r - 1.0) >= 1e-4 ? static_cast<float>(r) : 0.0f;
    }
    return t_factor;
}

// The log: once for each factor, once for each kind's first pinned draw at it (with the numbers the game sent), and at most every 30 s what was done.
std::atomic<uint32_t> g_loggedFactor{0};
std::atomic<uint32_t> g_valuesLogged[kKinds];
std::atomic<uint32_t> g_pinned[kKinds], g_noCopy[kKinds], g_tooSmall[kKinds];
std::atomic<ULONGLONG> g_lastSummary{0};

uint32_t Bits(float f) {
    uint32_t b;
    std::memcpy(&b, &f, sizeof(b));
    return b;
}

void LogValues(int kind, const uint8_t* bytes, UINT size, float factor) {
    if (g_valuesLogged[kind].exchange(Bits(factor), std::memory_order_relaxed) == Bits(factor))
        return;
    const auto* f = reinterpret_cast<const float*>(bytes);
    if (kind == kVignette && size >= 432)
        Log("Post pin: the vignette's numbers as the game sent them: vignetteParams %.5f %.5f %.5f, vignetteColor %.4f %.4f %.4f %.4f; drawn with .xy %.5f %.5f "
            "(film grain's texture scale and offset %.4f %.4f %.4f %.4f, chromostereopsis %.4f %.5f %.5f %.4f, colorScale %.5f %.5f %.5f %.5f: left as they are)",
            f[96], f[97], f[98], f[100], f[101], f[102], f[103], f[96] * factor, f[97] * factor, f[72], f[73], f[74], f[75], f[104], f[105], f[106], f[107], f[40],
            f[41], f[42], f[43]);
    else if (kind == kVignette && size >= 416)
        Log("Post pin: the vignette's numbers as the game sent them: vignetteParams %.5f %.5f %.5f, vignetteColor %.4f %.4f %.4f %.4f; drawn with .xy %.5f %.5f",
            f[96], f[97], f[98], f[100], f[101], f[102], f[103], f[96] * factor, f[97] * factor);
    else if (kind == kClearMiddle)
        Log("Post pin: the suppression blur's clear middle as the game sent it: dofParams %.5f %.5f %.5f %.5f; drawn with .xy %.5f %.5f", f[80], f[81], f[82],
            f[83], f[80] * factor, f[81] * factor);
}

// Appends "name N (immediate a + b, deferred c + d)" for a count that isn't 0.
void AppendWrite(char* out, size_t n, size_t& at, Write w, const uint32_t (&v)[2][2]) {
    const uint32_t total = v[0][0] + v[0][1] + v[1][0] + v[1][1];
    if (!total || at >= n)
        return;
    const int r = std::snprintf(out + at, n - at, "%s%s %u (immediate %u + %u, deferred %u + %u)", at ? "; " : "", kWriteNames[w], total, v[0][0], v[0][1],
                                v[1][0], v[1][1]);
    if (r > 0)
        at += static_cast<size_t>(r);
}

void Summary() {
    const ULONGLONG now = GetTickCount64();
    ULONGLONG last = g_lastSummary.load(std::memory_order_relaxed);
    if (now - last < 30000 || !g_lastSummary.compare_exchange_strong(last, now))
        return;
    if (!last)
        return;  // the first call only starts the clock
    uint32_t n[kKinds][3];
    for (int k = 0; k < kKinds; ++k) {
        n[k][0] = g_pinned[k].exchange(0);
        n[k][1] = g_noCopy[k].exchange(0);
        n[k][2] = g_tooSmall[k].exchange(0);
    }
    Log("Post pin: the last 30 s: the vignette's draws %u scaled, %u left as they were (no whole copy of their last upload on their context), %u with constants "
        "too small; the clear middle's %u scaled, %u left (no copy), %u too small",
        n[0][0], n[0][1], n[0][2], n[1][0], n[1][1], n[1][2]);

    // How the learned buffers were written: each way, on which kind of context, on the drawing thread + on others.
    int learned = 0;
    for (const auto& b : g_buffers)
        learned += b.load(std::memory_order_relaxed) != 0;
    char line[1600];
    size_t at = 0;
    line[0] = 0;
    for (int w = 0; w < kWrites; ++w) {
        uint32_t v[2][2];
        for (int d = 0; d < 2; ++d)
            for (int o = 0; o < 2; ++o)
                v[d][o] = g_writes[w][d][o].exchange(0);
        AppendWrite(line, sizeof(line), at, static_cast<Write>(w), v);
    }
    const uint32_t sources[4] = {g_copySources[0].exchange(0), g_copySources[1].exchange(0), g_copySources[2].exchange(0), g_copySources[3].exchange(0)};
    Log("Post pin: the last 30 s: the %d buffer(s) the vignette's draws use were written by: %s (on the drawing thread + on others)%s; CopyResource's sources "
        "by usage: default %u, immutable %u, dynamic %u, staging %u",
        learned, at ? line : "nothing seen", g_buffersFullLogged.load() ? "; more than 16 buffers seen" : "", sources[0], sources[1], sources[2], sources[3]);

    // What the draws found.
    const uint32_t onImmediate = g_drawsOn[0].exchange(0), onDeferred = g_drawsOn[1].exchange(0);
    at = 0;
    line[0] = 0;
    for (int w = 0; w < kWays; ++w) {
        const uint32_t direct = g_scaledBy[w][0].exchange(0), carried = g_scaledBy[w][1].exchange(0);
        if (direct + carried && at < sizeof(line)) {
            const int r = std::snprintf(line + at, sizeof(line) - at, "%sby %s %u (%u of them through an executed command list)", at ? ", " : "", kWayNames[w],
                                        direct + carried, carried);
            if (r > 0)
                at += static_cast<size_t>(r);
        }
    }
    char left[1200];
    size_t lat = 0;
    left[0] = 0;
    for (int y = 0; y < kWhys; ++y) {
        const uint32_t c = g_leftBy[y].exchange(0);
        if (c && lat < sizeof(left)) {
            const int r = std::snprintf(left + lat, sizeof(left) - lat, "%s%s %u", lat ? ", " : "", kWhyNames[y], c);
            if (r > 0)
                lat += static_cast<size_t>(r);
        }
    }
    bool recordingsFull = false;
    {
        std::lock_guard lock(g_state);
        recordingsFull = g_recordingsFull;
    }
    Log("Post pin: the last 30 s: the vignette's and the clear middle's draws on the immediate context %u, on deferred contexts %u (thread %lu, changed %u "
        "times; the immediate context's writes and draws came from another thread than the one before %u times%s); scaled with a whole copy %s; left as "
        "they were: %s",
        onImmediate, onDeferred, static_cast<unsigned long>(g_drawThread.load()), g_drawThreads.exchange(0), g_immediateThreads.exchange(0),
        recordingsFull ? "; more than 32 deferred contexts: the rest not followed" : "", at ? line : "none", lat ? left : "none");

    // What the copies cost: every whole write of a learned buffer is copied (a vignette draw takes only the one just before it).
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    const uint32_t copies = g_copiesTaken.exchange(0);
    const uint64_t copied = g_copiedBytes.exchange(0);
    const double ms = 1000.0 * static_cast<double>(g_copyTicks.exchange(0)) / static_cast<double>(frequency.QuadPart);
    Log("Post pin: the last 30 s: %u copies taken of the learned buffers (%.1f MB), %.1f ms copying (%.3f ms a frame at 72 a second); copies held now %lld "
        "bytes",
        copies, static_cast<double>(copied) / 1048576.0, ms, ms / (30.0 * 72.0), static_cast<long long>(g_heldBytes.load(std::memory_order_relaxed)));
}

// Under g_state: learns a buffer drawn with these shaders. -1 if there's no room.
int LearnLocked(ID3D11Buffer* buffer, bool& isNew) {
    isNew = false;
    const int i = Confirmed(buffer);
    if (i >= 0)
        return i;
    const auto p = reinterpret_cast<uintptr_t>(buffer);
    for (int k = 0; k < kMostBuffers; ++k) {
        if (g_buffers[k].load(std::memory_order_relaxed))
            continue;
        // A new occupant: nothing of whatever had the place before.
        g_immediate[k] = Entry{};
        for (Recording& r : g_recordings)
            r.e[k] = Entry{};
        for (Finished& f : g_lists)
            f.e[k] = Entry{};
        g_learnedAt[k] = ++g_seq;
        // Held while learned: its pointer can't then be handed to a new resource (one made with its data, never written where the hooks see it, or a texture
        // that GetDesc would be called on as a buffer) that would find this one's copy. Let go in Forget.
        buffer->AddRef();
        g_buffers[k].store(p, std::memory_order_relaxed);
        g_anyBuffer.store(true, std::memory_order_relaxed);
        isNew = true;
        return k;
    }
    return -1;
}

// ---- The hooks (installed from d3d11_hook.cpp) ----
using UpdateSubresourceFn = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, UINT, const D3D11_BOX*, const void*, UINT, UINT);
using CopyResourceFn = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, ID3D11Resource*);
using CopySubresourceRegionFn = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, UINT, UINT, UINT, UINT, ID3D11Resource*, UINT,
                                                         const D3D11_BOX*);
using ExecuteCommandListFn = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11CommandList*, BOOL);
using FinishCommandListFn = HRESULT(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, BOOL, ID3D11CommandList**);
UpdateSubresourceFn g_origUpdate[2] = {};
CopyResourceFn g_origCopy[2] = {};
CopySubresourceRegionFn g_origCopyRegion[2] = {};
ExecuteCommandListFn g_origExecute[2] = {};
FinishCommandListFn g_origFinish[2] = {};

void AfterUpdate(ID3D11DeviceContext* ctx, ID3D11Resource* dst, UINT sub, const D3D11_BOX* box, const void* src) {
    if (Known(dst) < 0)
        return;
    D3D11_RESOURCE_DIMENSION dim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    dst->GetType(&dim);
    if (dim != D3D11_RESOURCE_DIMENSION_BUFFER)
        return;  // can't be a learned one (they are held), but never GetDesc a texture as a buffer
    const bool immediate = IsImmediate(ctx);
    D3D11_BUFFER_DESC desc{};
    static_cast<ID3D11Buffer*>(dst)->GetDesc(&desc);
    // A box past the end makes the runtime drop the call: only one exactly over it all counts as whole.
    const bool whole = sub == 0 && (!box || (box->left == 0 && box->right == desc.ByteWidth && box->top == 0 && box->bottom == 1 && box->front == 0 &&
                                             box->back == 1));
    Count(whole ? kWUpdateWhole : kWUpdateBox, !immediate);
    std::lock_guard lock(g_state);
    const int i = Confirmed(dst);
    if (i < 0)
        return;
    Entry* e = EntryOf(ctx, immediate, i);
    if (!e)
        return;
    e->written = true;
    e->open = false;
    e->viaList = false;
    e->byMap = false;
    Drop(e);
    if (!whole)
        e->why = kWhyUpdateBox;
    else if (desc.Usage != D3D11_USAGE_DEFAULT)
        e->why = kWhyUpdateUsage;  // refused by the runtime (dynamic, immutable): whether anything was written isn't known
    else if (!src || desc.ByteWidth > kMostBytes)
        e->why = kWhyTooBig;
    else
        TakeCopy(e, src, desc.ByteWidth, kWayUpdate);
}

void AfterCopy(ID3D11DeviceContext* ctx, ID3D11Resource* dst, ID3D11Resource* src, bool region) {
    if (Known(dst) < 0)
        return;
    D3D11_RESOURCE_DIMENSION dim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    dst->GetType(&dim);
    if (dim != D3D11_RESOURCE_DIMENSION_BUFFER)
        return;
    D3D11_BUFFER_DESC dd{};
    static_cast<ID3D11Buffer*>(dst)->GetDesc(&dd);
    const bool immediate = IsImmediate(ctx);
    Count(region ? kWCopyRegion : kWCopyResource, !immediate);
    // CopyResource only does anything between buffers of one size into one that isn't immutable; otherwise the runtime drops it and no copy is kept.
    bool sameSize = false;
    if (!region && src) {
        D3D11_RESOURCE_DIMENSION sdim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
        src->GetType(&sdim);
        if (sdim == D3D11_RESOURCE_DIMENSION_BUFFER) {
            D3D11_BUFFER_DESC d{};
            static_cast<ID3D11Buffer*>(src)->GetDesc(&d);
            if (static_cast<unsigned>(d.Usage) < 4)
                g_copySources[d.Usage].fetch_add(1, std::memory_order_relaxed);
            sameSize = d.ByteWidth == dd.ByteWidth && dd.Usage != D3D11_USAGE_IMMUTABLE;
        }
    }
    std::lock_guard lock(g_state);
    const int i = Confirmed(dst);
    if (i < 0)
        return;
    // CopyResource of a buffer with a whole copy on this same context (a learned one, as the card will have it here): the destination gets the same.
    BlobPtr from;
    if (!region && sameSize) {
        const int j = Confirmed(src);
        if (j >= 0) {
            const Recording* r = immediate ? nullptr : RecordingOf(ctx, false);
            const Entry* s = immediate ? &g_immediate[j] : (r ? &r->e[j] : nullptr);
            if (s && (immediate || s->written) && s->blob && s->blob->size == dd.ByteWidth)
                from = MakeBlob(s->blob->bytes.get(), s->blob->size, kWayCopy);  // null past kMostHeldBytes: then no copy
        }
    }
    Entry* e = EntryOf(ctx, immediate, i);
    if (!e)
        return;
    e->written = true;
    e->open = false;
    e->viaList = false;
    // A copy in a command list from a buffer mapped in it is in the list's own order, and a default buffer keeps it after. A dynamic destination is treated
    // as its Map would be: not carried out of the list.
    e->byMap = dd.Usage == D3D11_USAGE_DYNAMIC;
    e->blob = from;
    e->why = kWhyCopyUnknown;
}

template <int W>
void STDMETHODCALLTYPE UpdateHook(ID3D11DeviceContext* ctx, ID3D11Resource* dst, UINT sub, const D3D11_BOX* box, const void* src, UINT rowPitch,
                                  UINT depthPitch) {
    g_origUpdate[W](ctx, dst, sub, box, src, rowPitch, depthPitch);
    if (g_anyBuffer.load(std::memory_order_relaxed))
        AfterUpdate(ctx, dst, sub, box, src);
}

template <int W>
void STDMETHODCALLTYPE CopyHook(ID3D11DeviceContext* ctx, ID3D11Resource* dst, ID3D11Resource* src) {
    g_origCopy[W](ctx, dst, src);
    if (g_anyBuffer.load(std::memory_order_relaxed)) {
        AfterCopy(ctx, dst, src, false);
        CensusCopy(ctx, "CopyResource", dst, src, 0);
    }
}

template <int W>
void STDMETHODCALLTYPE CopyRegionHook(ID3D11DeviceContext* ctx, ID3D11Resource* dst, UINT dstSub, UINT x, UINT y, UINT z, ID3D11Resource* src, UINT srcSub,
                                      const D3D11_BOX* box) {
    g_origCopyRegion[W](ctx, dst, dstSub, x, y, z, src, srcSub, box);
    if (g_anyBuffer.load(std::memory_order_relaxed)) {
        AfterCopy(ctx, dst, src, true);
        CensusCopy(ctx, "CopySubresourceRegion", dst, src, srcSub);
    }
}

// ID3D11DeviceContext1's: the same writes with flags (D3D11_COPY_DISCARD / NO_OVERWRITE say what the card may still be using, not what the buffer then holds).
using UpdateSubresource1Fn = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext1*, ID3D11Resource*, UINT, const D3D11_BOX*, const void*, UINT, UINT, UINT);
using CopySubresourceRegion1Fn = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext1*, ID3D11Resource*, UINT, UINT, UINT, UINT, ID3D11Resource*, UINT,
                                                          const D3D11_BOX*, UINT);
using DiscardResourceFn = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext1*, ID3D11Resource*);
UpdateSubresource1Fn g_origUpdate1[2] = {};
CopySubresourceRegion1Fn g_origCopyRegion1[2] = {};
DiscardResourceFn g_origDiscard[2] = {};

template <int W>
void STDMETHODCALLTYPE Update1Hook(ID3D11DeviceContext1* ctx, ID3D11Resource* dst, UINT sub, const D3D11_BOX* box, const void* src, UINT rowPitch,
                                   UINT depthPitch, UINT flags) {
    g_origUpdate1[W](ctx, dst, sub, box, src, rowPitch, depthPitch, flags);
    if (g_anyBuffer.load(std::memory_order_relaxed))
        AfterUpdate(ctx, dst, sub, box, src);
}

template <int W>
void STDMETHODCALLTYPE CopyRegion1Hook(ID3D11DeviceContext1* ctx, ID3D11Resource* dst, UINT dstSub, UINT x, UINT y, UINT z, ID3D11Resource* src,
                                       UINT srcSub, const D3D11_BOX* box, UINT flags) {
    g_origCopyRegion1[W](ctx, dst, dstSub, x, y, z, src, srcSub, box, flags);
    if (g_anyBuffer.load(std::memory_order_relaxed)) {
        AfterCopy(ctx, dst, src, true);
        CensusCopy(ctx, "CopySubresourceRegion1", dst, src, srcSub);
    }
}

template <int W>
void STDMETHODCALLTYPE DiscardHook(ID3D11DeviceContext1* ctx, ID3D11Resource* resource) {
    g_origDiscard[W](ctx, resource);
    if (!g_anyBuffer.load(std::memory_order_relaxed) || Known(resource) < 0)
        return;
    const bool immediate = IsImmediate(ctx);
    Count(kWDiscard, !immediate);
    std::lock_guard lock(g_state);
    const int i = Confirmed(resource);
    if (i < 0)
        return;
    Entry* e = EntryOf(ctx, immediate, i);
    if (!e)
        return;
    e->written = true;
    e->open = false;
    e->viaList = false;
    e->byMap = false;
    e->blob = nullptr;
    e->why = kWhyDiscarded;
}

// CopyStructureCount: 4 bytes of a counter written into a buffer at an offset. Never a whole copy.
using CopyStructureCountFn = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Buffer*, UINT, ID3D11UnorderedAccessView*);
CopyStructureCountFn g_origStructureCount[2] = {};

template <int W>
void STDMETHODCALLTYPE StructureCountHook(ID3D11DeviceContext* ctx, ID3D11Buffer* dst, UINT offset, ID3D11UnorderedAccessView* view) {
    g_origStructureCount[W](ctx, dst, offset, view);
    if (!g_anyBuffer.load(std::memory_order_relaxed) || Known(dst) < 0)
        return;
    const bool immediate = IsImmediate(ctx);
    Count(kWStructureCount, !immediate);
    std::lock_guard lock(g_state);
    const int i = Confirmed(dst);
    if (i < 0)
        return;
    Entry* e = EntryOf(ctx, immediate, i);
    if (!e)
        return;
    e->written = true;
    e->open = false;
    e->viaList = false;
    e->byMap = false;
    e->blob = nullptr;
    e->why = kWhyCopyUnknown;
}

template <int W>
HRESULT STDMETHODCALLTYPE FinishHook(ID3D11DeviceContext* ctx, BOOL restore, ID3D11CommandList** list) {
    const HRESULT hr = g_origFinish[W](ctx, restore, list);
    if (!g_anyBuffer.load(std::memory_order_relaxed))
        return hr;
    std::lock_guard lock(g_state);
    Recording* r = RecordingOf(ctx, false);
    if (SUCCEEDED(hr) && list && *list) {
        // Kept by the list's pointer (a new list on a freed one's place replaces it). One not kept (more than 256 waiting) is found missing at its execution.
        Finished* f = nullptr;
        for (Finished& x : g_lists)
            if (x.list == *list)
                f = &x;
        if (!f) {
            f = &g_lists[g_nextList];
            g_nextList = (g_nextList + 1) % kMostLists;
        }
        f->list = *list;
        f->start = r ? r->start : 0;
        for (int i = 0; i < kMostBuffers; ++i) {
            f->e[i] = r ? r->e[i] : Entry{};
            f->e[i].spare = nullptr;  // a list is never filled again: storage kept there would only count against the 8 MB
        }
    }
    // The context starts a new recording, seen from its beginning (if it has a place: else its next list is found unseen).
    if (!r)
        r = RecordingOf(ctx, true);
    if (r) {
        for (Entry& e : r->e)
            e = Entry{};
        r->start = g_seq;
    }
    return hr;
}

template <int W>
void STDMETHODCALLTYPE ExecuteHook(ID3D11DeviceContext* ctx, ID3D11CommandList* list, BOOL restore) {
    g_origExecute[W](ctx, list, restore);
    if (!g_anyBuffer.load(std::memory_order_relaxed))
        return;
    const bool immediate = IsImmediate(ctx);
    bool carried = false, map = false, unseen = false;
    {
        std::lock_guard lock(g_state);
        const Finished* f = nullptr;
        for (const Finished& x : g_lists)
            if (list && x.list == list)
                f = &x;
        for (int i = 0; i < kMostBuffers; ++i) {
            if (!g_buffers[i].load(std::memory_order_relaxed))
                continue;
            Entry* e = EntryOf(ctx, immediate, i);
            if (!e)
                continue;  // a deferred context with no place: its draws are left alone anyway
            if (f && f->e[i].written) {
                const Entry& s = f->e[i];
                e->written = true;
                e->open = false;
                e->byMap = s.byMap;
                if (s.byMap) {
                    e->blob = nullptr;
                    e->viaList = false;
                    e->why = kWhyListMap;
                    map = true;
                } else {
                    e->blob = s.blob;
                    e->why = s.why;
                    e->viaList = true;
                    carried = true;
                }
            } else if (!f || g_learnedAt[i] > f->start) {
                // Its recording may have written this buffer before the mod followed it.
                e->written = true;
                e->open = false;
                e->viaList = false;
                e->byMap = false;
                e->blob = nullptr;
                e->why = kWhyListUnseen;
                unseen = true;
            }
        }
    }
    if (carried)
        Count(kWListCarried, !immediate);
    if (map)
        Count(kWListMap, !immediate);
    if (unseen)
        Count(kWListUnseen, !immediate);
}

template <typename Fn>
bool HookPair(void* const (&target)[2], void* hook0, void* hook1, Fn (&original)[2], const char* name) {
    if (!target[0] || MH_CreateHook(target[0], hook0, reinterpret_cast<void**>(&original[0])) != MH_OK) {
        Log("Post pin: hooking %s failed", name);
        return false;
    }
    if (!target[1]) {
        Log("Post pin: the deferred contexts' %s not found", name);  // never taken for the immediate context's: it may be another function
        return false;
    }
    if (target[1] == target[0]) {
        original[1] = original[0];  // deferred contexts share the immediate context's function: one hook covers both
        return true;
    }
    if (MH_CreateHook(target[1], hook1, reinterpret_cast<void**>(&original[1])) != MH_OK) {
        Log("Post pin: hooking the deferred contexts' %s failed", name);
        return false;
    }
    return true;
}

}  // namespace

bool InstallHooks(const Targets& t, bool mapFollowed) {
    // Map is followed by skin.cpp's hook (NoteMap, NoteUpload): without it a Map would leave an old copy standing.
    bool ok = mapFollowed;
    if (!mapFollowed)
        Log("Post pin: Map isn't hooked (skin.cpp)");
    ok = HookPair(t.updateSubresource, reinterpret_cast<void*>(&UpdateHook<0>), reinterpret_cast<void*>(&UpdateHook<1>), g_origUpdate, "UpdateSubresource") && ok;
    ok = HookPair(t.copyResource, reinterpret_cast<void*>(&CopyHook<0>), reinterpret_cast<void*>(&CopyHook<1>), g_origCopy, "CopyResource") && ok;
    ok = HookPair(t.copySubresourceRegion, reinterpret_cast<void*>(&CopyRegionHook<0>), reinterpret_cast<void*>(&CopyRegionHook<1>), g_origCopyRegion,
                  "CopySubresourceRegion") &&
         ok;
    ok = HookPair(t.executeCommandList, reinterpret_cast<void*>(&ExecuteHook<0>), reinterpret_cast<void*>(&ExecuteHook<1>), g_origExecute,
                  "ExecuteCommandList") &&
         ok;
    ok = HookPair(t.finishCommandList, reinterpret_cast<void*>(&FinishHook<0>), reinterpret_cast<void*>(&FinishHook<1>), g_origFinish, "FinishCommandList") && ok;
    ok = HookPair(t.copyStructureCount, reinterpret_cast<void*>(&StructureCountHook<0>), reinterpret_cast<void*>(&StructureCountHook<1>),
                  g_origStructureCount, "CopyStructureCount") &&
         ok;
    // The 11.1 ones only where the runtime has them.
    if (t.updateSubresource1[0])
        ok = HookPair(t.updateSubresource1, reinterpret_cast<void*>(&Update1Hook<0>), reinterpret_cast<void*>(&Update1Hook<1>), g_origUpdate1,
                      "UpdateSubresource1") &&
             ok;
    if (t.copySubresourceRegion1[0])
        ok = HookPair(t.copySubresourceRegion1, reinterpret_cast<void*>(&CopyRegion1Hook<0>), reinterpret_cast<void*>(&CopyRegion1Hook<1>),
                      g_origCopyRegion1, "CopySubresourceRegion1") &&
             ok;
    if (t.discardResource[0])
        ok = HookPair(t.discardResource, reinterpret_cast<void*>(&DiscardHook<0>), reinterpret_cast<void*>(&DiscardHook<1>), g_origDiscard, "DiscardResource") &&
             ok;
    g_hooked.store(ok, std::memory_order_relaxed);
    if (!ok)
        Log("Post pin: not every way of writing a constant buffer is followed; the vignette is left as the game draws it");
    return ok;
}

void NoteShader(ID3D11PixelShader* ps, uint64_t checksum) {
    const auto p = reinterpret_cast<uintptr_t>(ps);
    const int kind = KindOfChecksum(checksum);
    std::lock_guard lock(g_shaderMutex);  // shaders are made seldom; the draws only read
    // A pointer the driver hands out again is the new shader's: an old entry for it goes.
    for (int i = 0; i < kMostShaders; ++i)
        if (g_shaders[i].load(std::memory_order_relaxed) == p) {
            if (g_shaderKinds[i].load(std::memory_order_relaxed) == kind)
                return;
            g_shaders[i].store(0, std::memory_order_relaxed);
        }
    if (kind < 0)
        return;
    int at = -1;
    for (int i = 0; i < kMostShaders && at < 0; ++i)
        if (!g_shaders[i].load(std::memory_order_relaxed))
            at = i;
    if (at < 0) {
        at = g_nextShader;
        g_nextShader = (g_nextShader + 1) % kMostShaders;
        if (!g_shadersFullLogged) {
            g_shadersFullLogged = true;
            Log("Post pin: more than %d of the vignette's shaders made; the oldest make room", kMostShaders);
        }
        g_shaders[at].store(0, std::memory_order_release);  // a draw reading it now finds nothing rather than the new pointer with the old kind
    }
    g_shaderKinds[at].store(kind, std::memory_order_relaxed);  // before the pointer: a draw finding the pointer finds its kind
    g_shaders[at].store(p, std::memory_order_release);
    if (!g_madeLogged[kind].exchange(true))
        Log("Post pin: the game made %s's pixel shader (%016llx)%s", kNames[kind], static_cast<unsigned long long>(checksum),
            kind == kLuminance ? "; looked at only (where its average is made: the exposure's census)"
            : Wanted()         ? "; held as at 100 degrees whenever the picture is drawn wider"
                               : "; left as it is (pin_effects off)");
    g_anyShader.store(true, std::memory_order_relaxed);
    Refresh();
}

void NoteMap(ID3D11DeviceContext* ctx, ID3D11Resource* resource) {
    if (!g_anyBuffer.load(std::memory_order_relaxed))
        return;
    if (Known(resource) < 0)
        return;
    const bool immediate = IsImmediate(ctx);
    Count(kWMap, !immediate);
    std::lock_guard lock(g_state);
    const int i = Confirmed(resource);
    if (i < 0)
        return;
    Entry* e = EntryOf(ctx, immediate, i);
    if (!e)
        return;
    e->written = true;
    e->open = true;
    e->viaList = false;
    e->byMap = true;
    Drop(e);  // the last map's copy is no longer what the card reads; its storage waits for this map's
    e->why = kWhyMapUnfollowed;  // until its Unmap brings all of it
}

void NoteUpload(ID3D11DeviceContext* ctx, ID3D11Resource* resource, const void* data, UINT size, bool fresh) {
    if (!g_anyBuffer.load(std::memory_order_relaxed))
        return;
    if (Known(resource) < 0)
        return;
    const bool immediate = IsImmediate(ctx);
    std::lock_guard lock(g_state);
    const int i = Confirmed(resource);
    if (i < 0)
        return;
    Entry* e = EntryOf(ctx, immediate, i);
    if (!e || !e->open)
        return;  // its map wasn't seen (learned in between): it stays as its last write left it, or unknown
    e->open = false;
    if (!fresh)
        e->why = kWhyMapAppend;
    else if (!data || size > kMostBytes)
        e->why = kWhyTooBig;
    else
        TakeCopy(e, data, size, kWayMap);  // before the game's Unmap (skin.cpp calls this first): the mapped bytes are still the ones written
    g_writes[fresh ? kWMapWhole : kWMapAppend][!immediate][GetCurrentThreadId() != g_drawThread.load(std::memory_order_relaxed)].fetch_add(
        1, std::memory_order_relaxed);
}

void SetTest(int value) {
    if (value > 1)
        value = 1;
    if (value < -1)
        value = -1;
    const int was = g_test.exchange(value);
    Refresh();
    if (was != value) {
        if (value < 0)
            Log("Post pin: as bf3vr.ini's [stereo] pin_effects has it (%s)", config::Get().pinEffects ? "on" : "off");
        else
            Log("Post pin: %s (weapon.txt postpin %d)", value ? "on" : "off", value);
        g_loggedFactor.store(0);
        for (auto& v : g_valuesLogged)
            v.store(0);
    }
}

void Pin::Begin(ID3D11DeviceContext* ctx, UINT count) {
    if (scopeview::Drawing())
        return;  // the scope's own picture: its angle isn't the eyes'
    float fov = 0.0f;
    const float factor = Factor(fov);
    if (factor == 0.0f) {
        if (g_anyBuffer.load(std::memory_order_relaxed))
            Forget();  // back at the base: nothing followed
        return;
    }
    ID3D11PixelShader* ps = nullptr;
    ctx->PSGetShader(&ps, nullptr, nullptr);
    if (!ps)
        return;
    ps->Release();
    const int kind = KindOf(ps);
    if (g_censusOn.load(std::memory_order_relaxed)) {
        if (GetTickCount64() < g_censusUntil.load(std::memory_order_relaxed))
            CensusDraw(ctx, ps, kind, count);
        else
            g_censusOn.store(false, std::memory_order_relaxed);
    }
    if (kind < 0 || kind >= kKinds)
        return;
    if (g_loggedFactor.exchange(Bits(factor)) != Bits(factor))
        Log("Post pin: the picture %.2f degrees wide: the vignette's and the suppression blur's clear middle's reach (.xy) scaled by %.5f, so each is as at 100 "
            "degrees at every angle from the middle",
            fov * 57.2957795f, factor);
    if (!g_censusUntil.load(std::memory_order_relaxed)) {
        ULONGLONG none = 0;
        if (g_censusUntil.compare_exchange_strong(none, GetTickCount64() + 120000))
            g_censusOn.store(true, std::memory_order_relaxed);
    }
    Summary();
    const DWORD thread = GetCurrentThreadId();
    if (g_drawThreadWas.exchange(thread, std::memory_order_relaxed) != thread)
        g_drawThreads.fetch_add(1, std::memory_order_relaxed);
    g_drawThread.store(thread, std::memory_order_relaxed);
    const bool immediate = IsImmediate(ctx);
    g_drawsOn[!immediate].fetch_add(1, std::memory_order_relaxed);
    if (!g_hooked.load(std::memory_order_relaxed)) {
        g_leftBy[kWhyNotHooked].fetch_add(1, std::memory_order_relaxed);
        g_noCopy[kind].fetch_add(1, std::memory_order_relaxed);
        return;
    }
    ID3D11Buffer* game = nullptr;
    ctx->PSGetConstantBuffers(0, 1, &game);
    if (!game) {
        g_leftBy[kWhyNoBuffer].fetch_add(1, std::memory_order_relaxed);
        g_noCopy[kind].fetch_add(1, std::memory_order_relaxed);
        return;
    }
    // The pin's own buffer bound already (a draw inside a pinned one): its contents are scaled, and learning it would scale them again.
    for (const Own& o : t_own)
        if (o.buffer && o.buffer == game) {
            game->Release();
            return;
        }
    // The copy: this buffer's whole contents as the card will read them at this draw, if every write since is known.
    BlobPtr blob;
    Why why = kWhyNotWritten;
    bool viaList = false, isNew = false, full = false;
    int index = -1;
    if (immediate)
        NoteImmediateThread();
    if (detail::g_on.load(std::memory_order_relaxed)) {
        std::lock_guard lock(g_state);
        if (!detail::g_on.load(std::memory_order_relaxed)) {
            // Looked at again under the lock: switched off just now (Forget done, or waiting for the lock): nothing is learned, so the hooks stay idle.
        } else if ((index = LearnLocked(game, isNew)) < 0) {
            why = kWhyFull;
            full = true;
        } else if (immediate) {
            blob = g_immediate[index].blob;
            why = g_immediate[index].why;
            viaList = g_immediate[index].viaList;
        } else if (const Recording* r = RecordingOf(ctx, false); r && r->e[index].written) {
            blob = r->e[index].blob;
            why = r->e[index].why;
        }
    }
    if (isNew) {
        char d[256];
        Describe(game, d, sizeof(d));
        Log("Post pin: %s's draws use constant buffer %d: %s; first drawn on the %s context (thread %lu)", kNames[kind], index, d,
            immediate ? "immediate" : "a deferred", static_cast<unsigned long>(thread));
    }
    if (full && !g_buffersFullLogged.exchange(true))
        Log("Post pin: more than %d constant buffers drawn with the vignette's shaders; the rest are left as the game fills them", kMostBuffers);
    if (blob) {
        // Bound whole? A range (PSSetConstantBuffers1) would put the shader's offset 384 elsewhere in the buffer.
        ID3D11DeviceContext1* ctx1 = nullptr;
        if (SUCCEEDED(ctx->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&ctx1))) && ctx1) {
            ID3D11Buffer* bound = nullptr;
            UINT first = 0, constants = 0;
            ctx1->PSGetConstantBuffers1(0, 1, &bound, &first, &constants);
            if (bound)
                bound->Release();
            ctx1->Release();
            if (bound != game || first != 0 || (constants != 0 && constants * 16 < blob->size)) {
                blob = nullptr;
                why = kWhyRange;
            }
        }
    }
    if (!blob) {
        g_leftBy[why].fetch_add(1, std::memory_order_relaxed);
        g_noCopy[kind].fetch_add(1, std::memory_order_relaxed);
        game->Release();
        return;
    }
    if (blob->size < kOffset[kind] + 8 || blob->size % 16) {
        g_tooSmall[kind].fetch_add(1, std::memory_order_relaxed);
        game->Release();
        return;
    }
    ID3D11Buffer* own = OwnBuffer(ctx, blob->size);
    if (!own) {
        game->Release();
        return;
    }
    // This thread's scratch for the scaled copy (the blob is never changed: it may still be an entry's): grown, never given back, so no allocation per draw.
    thread_local std::unique_ptr<uint8_t[]> t_scratch;
    thread_local UINT t_scratchSize = 0;
    if (t_scratchSize < blob->size) {
        t_scratch.reset(new (std::nothrow) uint8_t[blob->size]);
        t_scratchSize = t_scratch ? blob->size : 0;
        if (!t_scratch) {
            game->Release();
            return;
        }
    }
    uint8_t* const bytes = t_scratch.get();
    std::memcpy(bytes, blob->bytes.get(), blob->size);
    LogValues(kind, bytes, blob->size, factor);
    float xy[2];
    std::memcpy(xy, bytes + kOffset[kind], sizeof(xy));
    xy[0] *= factor;
    xy[1] *= factor;
    std::memcpy(bytes + kOffset[kind], xy, sizeof(xy));
    ctx->UpdateSubresource(own, 0, nullptr, bytes, 0, 0);
    ctx->PSSetConstantBuffers(0, 1, &own);
    g_pinned[kind].fetch_add(1, std::memory_order_relaxed);
    g_scaledBy[blob->way][viaList].fetch_add(1, std::memory_order_relaxed);
    m_ctx = ctx;
    m_game = game;  // our reference, given back in End
}

void Pin::End() {
    m_ctx->PSSetConstantBuffers(0, 1, &m_game);
    m_game->Release();
    m_game = nullptr;
    m_ctx = nullptr;
}

}  // namespace postpin

#include "fpbatch.h"

#include <windows.h>
#include <d3d11.h>
#include <MinHook.h>

#include <atomic>
#include <cstring>
#include <iterator>
#include <string>
#include <unordered_map>

#include "camera.h"
#include "fpbatchrules.h"
#include "log.h"

// The hook itself (its own anonymous namespace: FpSetterHook, g_fpSetterOriginal, g_fpRelReturn, g_fpAbsReturn; FpNoteCall is below).
#include "fpbatchstub.h"

namespace fpbatch {
std::atomic<bool> g_on{false};
}

namespace {

std::atomic<bool> g_wanted{false};
bool g_made = false, g_failed = false, g_enabled = false;  // (the frame's thread's)
ULONGLONG g_quietSince = 0;                                // the last line, or the probe's start: "nothing" is said after 10 s of it

// The books: two seconds' worth, the current one written by the hook (the game's threads) and the draw hooks under the lock; the other read
// and emptied by the frame's thread once a second, outside it (nothing else holds it then).
SRWLOCK g_lock = SRWLOCK_INIT;
fpbatch::Second g_books[2];
fpbatch::Second* g_current = &g_books[0];
uint32_t g_generation = 1;

// This thread's draws: the batches whose camera its last per-view upload was, and the buffer it went into (fpbatchrules.h Tag, and the
// rules it goes by: TagView, TagWorldView, TagDraw). It ends at the next per-view upload, at the first draw that finds the buffer bound to
// no vertex-shader slot, at a second's end, or at an 80-byte view-projection in the world after the batch's draws.
thread_local fpbatch::Tag t_tag;

bool Read(uint32_t at, void* out, size_t size) {
    return camera::SafeRead(at, out, size);
}

// (On the game's threads, inside the setter's hook, for the batch camera's two calls only: guarded reads, a lock, no allocation, nothing
// thrown.)
void __stdcall FpNoteCall(const uint8_t* camera, const float* matrix, uintptr_t ebx, uintptr_t ebp, uintptr_t entry) {
    if (!fpbatch::g_on.load(std::memory_order_relaxed))
        return;
    uint32_t returnAt = 0;
    if (!Read(static_cast<uint32_t>(entry), &returnAt, sizeof(returnAt)))
        return;
    fpbatch::Call call;
    const fpbatch::Fault fault = fpbatch::ReadCall(Read, returnAt, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(camera)),
                                                   static_cast<uint32_t>(reinterpret_cast<uintptr_t>(matrix)), static_cast<uint32_t>(ebx),
                                                   static_cast<uint32_t>(ebp), static_cast<uint32_t>(entry), call);
    AcquireSRWLockExclusive(&g_lock);
    if (fault == fpbatch::kTaken)
        fpbatch::NoteCall(*g_current, call);
    else
        fpbatch::NoteFault(*g_current, fault);
    ReleaseSRWLockExclusive(&g_lock);
}

// ---- Names, at the log (the frame's thread) ----

// Regions of memory as VirtualQuery last told them, this second: whether the name search reads them (fpbatchrules.h ReadableProtect).
struct Region {
    uintptr_t base = 0, end = 0;
    bool ok = false;
};
Region g_regions[8];
int g_regionNext = 0;

// How many of size bytes from at can be read (0 none): committed, readable the ordinary way, never write-combined, uncached or a guard page.
size_t Readable(uintptr_t at, size_t size) {
    if (at < 0x10000 || at + size < at)
        return 0;
    for (const Region& r : g_regions)
        if (r.end && at >= r.base && at < r.end)
            return r.ok ? (at + size <= r.end ? size : r.end - at) : 0;
    MEMORY_BASIC_INFORMATION mbi{};
    if (VirtualQuery(reinterpret_cast<LPCVOID>(at), &mbi, sizeof(mbi)) != sizeof(mbi))
        return 0;
    Region& r = g_regions[g_regionNext];
    g_regionNext = (g_regionNext + 1) % static_cast<int>(std::size(g_regions));
    r.base = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
    r.end = r.base + mbi.RegionSize;
    r.ok = mbi.State == MEM_COMMIT && fpbatch::ReadableProtect(mbi.Protect);
    return r.ok ? (at + size <= r.end ? size : r.end - at) : 0;
}

// A pointer to a name among an object's first 32 words: the best kept (one with a '/' first, then the longest), with where it was.
void ScanForName(uint32_t object, const char* via, std::string& best, size_t& bestScore) {
    uint32_t words[32] = {};
    if (Readable(object, sizeof(words)) != sizeof(words) || !Read(object, words, sizeof(words)))
        return;
    for (int i = 0; i < 32; ++i) {
        uint8_t text[80] = {};
        const size_t n = Readable(words[i], sizeof(text) - 1);
        if (n < 5 || !Read(words[i], text, n))
            continue;
        char name[80];
        const size_t length = fpbatch::LooksLikeName(text, n + 1, name, sizeof(name), 4);
        if (!length)
            continue;
        const size_t score = length + (std::strchr(name, '/') || std::strchr(name, '\\') ? 1000 : 0) + (std::strchr(name, '_') ? 100 : 0);
        if (score > bestScore) {
            bestScore = score;
            char at[32];
            std::snprintf(at, sizeof(at), "%s+0x%X", via, i * 4);
            best = std::string("'") + name + "' (" + at + ")";
        }
    }
}

std::unordered_map<uint32_t, std::string> g_names;  // a mesh's name as found ("" none), the frame's thread's
int g_searchesLeft = 0;                             // name searches left this second

void NameOf(uint32_t mesh, char* text, size_t size) {
    text[0] = 0;
    if (!mesh)
        return;
    auto it = g_names.find(mesh);
    if (it == g_names.end()) {
        if (g_searchesLeft <= 0 || g_names.size() > 4096)
            return;
        --g_searchesLeft;
        // Its own first words, then those of what its +0x10 points at (what the batch camera's draws read there).
        std::string best;
        size_t score = 0;
        ScanForName(mesh, "", best, score);
        uint32_t inner = 0;
        if (Readable(mesh + 0x10, 4) == 4 && Read(mesh + 0x10, &inner, 4) && inner)
            ScanForName(inner, "+0x10>", best, score);
        it = g_names.emplace(mesh, best).first;
    }
    std::snprintf(text, size, "%s", it->second.c_str());
}

// ---- The hook ----

void Install() {
#if defined(_M_IX86)
    int failed = -1;
    if (!fpbatch::CodeAsRead(Read, &failed)) {
        g_failed = true;
        Log("FP batches: the game's code isn't as read at %08X (%s): the probe stays off, nothing hooked", failed >= 0 ? fpbatch::kCode[failed].at : 0u,
            failed >= 0 ? fpbatch::kCode[failed].what : "?");
        return;
    }
    g_fpRelReturn = fpbatch::kRelReturn;
    g_fpAbsReturn = fpbatch::kAbsReturn;
    void* target = reinterpret_cast<void*>(static_cast<uintptr_t>(fpbatch::kSetter));
    if (const MH_STATUS status = MH_CreateHook(target, reinterpret_cast<void*>(&FpSetterHook), &g_fpSetterOriginal); status != MH_OK) {
        g_failed = true;
        Log("FP batches: the camera set couldn't be hooked (MinHook %d): the probe stays off", static_cast<int>(status));
        return;
    }
    g_made = true;
#else
    g_failed = true;
    Log("FP batches: the probe is the 32-bit game's only");
#endif
}

void SetEnabled(bool on) {
    void* target = reinterpret_cast<void*>(static_cast<uintptr_t>(fpbatch::kSetter));
    if (on) {
        AcquireSRWLockExclusive(&g_lock);
        fpbatch::Reset(g_books[0]);
        fpbatch::Reset(g_books[1]);
        g_current = &g_books[0];
        g_current->generation = ++g_generation;
        ReleaseSRWLockExclusive(&g_lock);
    }
    const MH_STATUS status = on ? MH_EnableHook(target) : MH_DisableHook(target);
    if (status != MH_OK) {
        Log("FP batches: the hook couldn't be turned %s (MinHook %d)%s", on ? "on" : "off", static_cast<int>(status), on ? ": the probe stays off" : "");
        if (on)
            g_failed = true;
        return;
    }
    g_enabled = on;
    g_quietSince = GetTickCount64();
    if (on)
        Log("FP batches: the probe on (weapon.txt fpbatchlog 1): the game's camera set (%08X) hooked for its two calls from the first-person batch camera "
            "(%08X: the view's own first-person camera, returning to %08X, and the transform made relative to the batch's origin, returning to %08X), "
            "the game's code as read at %d places; a line a second for each kind of batch, with the draws made with its camera",
            fpbatch::kSetter, fpbatch::kBatchCamera, fpbatch::kAbsReturn, fpbatch::kRelReturn, fpbatch::kCodeCount);
    else
        Log("FP batches: the probe off: the hook turned off, nothing of it runs");
}

// The second just gone, logged: its first line, then a line for each kind of batch (kLines at most).
void LogSecond(const fpbatch::Second& s) {
    constexpr int kLines = 12;
    static char line[8192];
    fpbatch::Summary(s, line, sizeof(line));
    Log("%s", line);
    bool done[fpbatch::kRecords] = {};
    int lines = 0, notLogged = 0;
    g_searchesLeft = 6;
    g_regions[0] = g_regions[1] = g_regions[2] = g_regions[3] = g_regions[4] = g_regions[5] = g_regions[6] = g_regions[7] = Region{};
    for (int i = 0; i < s.count; ++i) {
        if (done[i])
            continue;
        bool group[fpbatch::kRecords] = {};
        for (int j = i; j < s.count; ++j)
            if (!done[j] && fpbatch::SameMeshes(s.records[i], s.records[j]))
                group[j] = done[j] = true;
        if (lines >= kLines) {
            ++notLogged;
            continue;
        }
        ++lines;
        fpbatch::Line(s, i, group, NameOf, line, sizeof(line));
        Log("%s", line);
    }
    if (notLogged)
        Log("FP batches: %d more kinds of batch this second not logged", notLogged);
}

}  // namespace

namespace fpbatch {

void SetLog(bool on) {
    if (g_wanted.exchange(on) != on && !on)
        Log("FP batches: the probe no longer wanted (weapon.txt fpbatchlog gone or 0)");
}

void OnFrame() {
    const bool wanted = g_wanted.load(std::memory_order_relaxed);
    if (wanted && !g_made && !g_failed)
        Install();
    if (g_made && !g_failed && wanted != g_enabled)
        SetEnabled(wanted);
    g_on.store(g_enabled, std::memory_order_relaxed);
    if (!g_enabled)
        return;
    static ULONGLONG s_last = 0;
    const ULONGLONG now = GetTickCount64();
    if (now - s_last < 1000)
        return;
    s_last = now;
    AcquireSRWLockExclusive(&g_lock);
    Second* done = g_current;
    g_current = done == &g_books[0] ? &g_books[1] : &g_books[0];
    g_current->generation = ++g_generation;
    ReleaseSRWLockExclusive(&g_lock);
    if (done->calls || done->matched) {
        LogSecond(*done);
        g_quietSince = now;
    } else if (now - g_quietSince >= 10000) {
        Log("FP batches: nothing in the last 10 s: no calls of the camera set from the first-person batch camera (%u per-view uploads in the last second)",
            done->uploads);
        g_quietSince = now;
    }
    Reset(*done);
}

void NoteView(ID3D11Resource* buffer, const float* rows, const double* eye) {
    AcquireSRWLockExclusive(&g_lock);
    const Tag tag = TagView(*g_current, buffer, rows, eye);
    ReleaseSRWLockExclusive(&g_lock);
    t_tag = tag;
}

void NoteWorldView() {
    Tag& t = t_tag;
    if (!t.generation || t.ended)
        return;
    AcquireSRWLockExclusive(&g_lock);
    TagWorldView(*g_current, t);
    ReleaseSRWLockExclusive(&g_lock);
}

bool Tagged() {
    return t_tag.generation != 0;
}

void NoteDraw(ID3D11DeviceContext* ctx, unsigned count, unsigned start, int base, char kind) {
    Tag& t = t_tag;
    if (!t.generation || !ctx)
        return;
    ID3D11Buffer* bound[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = {};
    ctx->VSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, bound);
    bool isBound = false;
    for (ID3D11Buffer* b : bound) {
        if (b && static_cast<const void*>(static_cast<ID3D11Resource*>(b)) == t.buffer)
            isBound = true;
        if (b)
            b->Release();
    }
    UINT stencil = 0;
    if (isBound && !t.ended) {
        ID3D11DepthStencilState* state = nullptr;
        ctx->OMGetDepthStencilState(&state, &stencil);
        if (state)
            state->Release();
    }
    AcquireSRWLockExclusive(&g_lock);
    TagDraw(*g_current, t, isBound, count, start, base, static_cast<uint8_t>(stencil), kind);
    ReleaseSRWLockExclusive(&g_lock);
}

}  // namespace fpbatch

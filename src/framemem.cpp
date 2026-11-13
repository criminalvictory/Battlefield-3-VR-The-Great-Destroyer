#include "framemem.h"

#include <windows.h>
#include <MinHook.h>

#include <utility>

#include "camera.h"
#include "config.h"
#include "log.h"

// The game's side (bf3.exe 1.6, not relocated, read in the memory image). The shader system (one object, its address kept at 0x02356C90) holds the frame's block at +60h, its settings in use at
// +58h and any waiting to be taken at +5Ch. A frame, on the client side: the dispatch (0x00FB97A0) first waits for the frame before's render job, then starts the shader system's frame (its
// table's +1Ch: the frame's lock, and the waiting settings taken), then has every renderer write its draw lists by jobs ("buildJobs"); the render ("renderDispatch", a job made to run after
// them) draws them and, at its end, calls the shader system's end of frame (+28h, 0x00696CD0), which empties the block with one store and lets the frame's lock go. So at that call nothing of
// this frame is in the block any more and nothing of the next is yet: there the block is measured and, once, made bigger.
namespace framemem {

namespace {

constexpr uintptr_t kHolder = 0x02356C90, kEndFrameSlot = 0x020981D0;
constexpr uintptr_t kEndFrameAt = 0x00696CD0, kSettingsAt = 0x0069AE80, kGameFreeAt = 0x00418290;

// Everything this rests on, as the running game has it: nothing is hooked, read or changed unless every one of these is as it was read.
struct Code {
    uintptr_t at;
    size_t size;
    uint8_t bytes[16];
    const char* what;
};
constexpr Code kCode[] = {
    {0x00696CD0, 16, {0x80, 0x3D, 0x2D, 0x6C, 0x35, 0x02, 0x00, 0x56, 0x8B, 0xF1, 0xC6, 0x46, 0x4F, 0x00, 0x74, 0x07}, "the shader system's end of frame"},
    {0x00696CFF, 14, {0x8B, 0x4E, 0x60, 0xE8, 0xA9, 0xD0, 0xE6, 0xFF, 0x8D, 0x8E, 0xE0, 0x00, 0x00, 0x00}, "its emptying of the block at +60h, then the frame's lock let go"},
    {0x00503DB0, 8, {0xC7, 0x41, 0x10, 0x00, 0x00, 0x00, 0x00, 0xC3}, "the block's emptying (its use at +10h)"},
    {0x0050F060, 16, {0x8B, 0x51, 0x08, 0x8B, 0x44, 0x24, 0x08, 0x8D, 0x44, 0x02, 0xFF, 0x4A, 0xF7, 0xD2, 0x23, 0xC2}, "a request of the block (its alignment at +8)"},
    {0x0050F070, 16, {0x56, 0x8B, 0xD0, 0x8D, 0x71, 0x10, 0xF0, 0x0F, 0xC1, 0x16, 0x03, 0xC2, 0x5E, 0x3B, 0x41, 0x04}, "a request of the block (its use at +10h, its size at +4)"},
    {0x0050F080, 12, {0x76, 0x05, 0x33, 0xC0, 0xC2, 0x08, 0x00, 0x8B, 0x01, 0x03, 0xC2, 0xC2}, "a request of the block (null past its end, its start at +0)"},
    {0x00503D90, 16, {0x66, 0x83, 0x79, 0x0C, 0x00, 0x74, 0x0B, 0x8B, 0x01, 0x8B, 0x49, 0x14, 0x50, 0xE8, 0xEE, 0x44}, "the block's buffer given back (owned at +0Ch, its allocator at +14h)"},
    {0x00418290, 16, {0x8B, 0x51, 0x04, 0xF6, 0xC2, 0x02, 0x75, 0x45, 0x8B, 0x44, 0x24, 0x04, 0x85, 0xC0, 0x74, 0x3D}, "the allocators' free"},
    {0x0069AE80, 16, {0x51, 0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x5C, 0x85, 0xC0, 0x0F, 0x84, 0xC9, 0x00, 0x00, 0x00, 0x3B}, "the taking of waiting settings"},
    {0x0069AEC9, 16, {0x8B, 0x46, 0x60, 0x85, 0xC0, 0x74, 0x0B, 0x8B, 0x4E, 0x58, 0x8B, 0x50, 0x04, 0x3B, 0x51, 0x24}, "its check of the block's size against the settings' (+24h)"},
    {0x006A6120, 11, {0x8B, 0x50, 0x64, 0x8B, 0xCE, 0xC6, 0x46, 0x4E, 0x01, 0xFF, 0xD2}, "the frame's start taking the waiting settings"},
    {0x00FB97B0, 8, {0x8B, 0x06, 0x8B, 0x50, 0x18, 0x57, 0xFF, 0xD2}, "the dispatch's first call: the wait for the frame before's render"},
    {0x00FAE300, 16, {0x80, 0xB9, 0x5C, 0x0E, 0x00, 0x00, 0x00, 0x74, 0x27, 0x56, 0x8D, 0xB1, 0x60, 0x0E, 0x00, 0x00}, "that wait"},
    {0x00FB99F8, 13, {0x8B, 0x0D, 0x90, 0x6C, 0x35, 0x02, 0x8B, 0x11, 0x8B, 0x42, 0x1C, 0xFF, 0xD0}, "the dispatch starting the shader system's frame"},
    {0x00FB9D34, 12, {0x8D, 0x4C, 0x24, 0x2C, 0x51, 0x8B, 0xCF, 0xE8, 0x50, 0x06, 0xA3, 0xFF}, "the render job made to run after the draw lists' jobs"},
    {0x00FB5B8C, 16, {0x8B, 0x0D, 0x90, 0x6C, 0x35, 0x02, 0x8B, 0x01, 0x8B, 0x50, 0x20, 0xFF, 0xD2, 0x8B, 0x0D, 0x90}, "the render's end, through the shader system's holder"},
    {0x00FB5B9C, 15, {0x6C, 0x35, 0x02, 0x8B, 0x01, 0x8B, 0x40, 0x28, 0x8D, 0x54, 0x24, 0x74, 0x52, 0xFF, 0xD0}, "the render's call of the frame's end (the table's +28h)"},
};

using EndFrameFn = void*(__fastcall*)(uint8_t* self, void* edx, void* out);
using SettingsFn = void(__fastcall*)(uint8_t* self, void* edx);
using GameFreeFn = void(__fastcall*)(void* allocator, void* edx, void* buffer);

EndFrameFn g_endFrame = nullptr;
SettingsFn g_settings = nullptr;

Meter g_meter;
Keeper g_keeper;
uint32_t g_wantedMb = 0;                     // the size asked for, once hooked
std::atomic<bool> g_layout{false};           // the game's code is as read: the block may be read
std::atomic<bool> g_stopped{false};          // a fault in the mod's own work at a frame's end: nothing more is done there, nothing measured
std::atomic<bool> g_settingsStopped{false};  // the same where waiting settings are taken (each side stops by itself)
bool g_settled = false, g_saidOdd = false;
uint32_t g_tries = 0;
ULONGLONG g_lastLine = 0;

// A buffer of the mod's own, from the top of the address space down, where the game's own heaps are.
void* Take(uint32_t bytes) { return VirtualAlloc(nullptr, bytes, MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN, PAGE_READWRITE); }

void GiveBack(void* buffer) { VirtualFree(buffer, 0, MEM_RELEASE); }

// As the block's own destructor gives its buffer back (0x00503D90): the allocators' free, with the block's allocator.
void GameFree(uint32_t allocator, uint32_t buffer) { reinterpret_cast<GameFreeFn>(kGameFreeAt)(At<void>(allocator), nullptr, At<void>(buffer)); }

void Space(uint32_t* freeMb, uint32_t* largestMb) {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    uint64_t free = 0, largest = 0;
    auto at = reinterpret_cast<uintptr_t>(info.lpMinimumApplicationAddress);
    const auto end = reinterpret_cast<uintptr_t>(info.lpMaximumApplicationAddress);
    MEMORY_BASIC_INFORMATION region;
    while (at < end && VirtualQuery(reinterpret_cast<const void*>(at), &region, sizeof(region)) == sizeof(region)) {
        if (region.State == MEM_FREE) {
            free += region.RegionSize;
            largest = std::max<uint64_t>(largest, region.RegionSize);
        }
        const uintptr_t next = reinterpret_cast<uintptr_t>(region.BaseAddress) + region.RegionSize;
        if (next <= at)
            break;
        at = next;
    }
    *freeMb = static_cast<uint32_t>(free / kMb);
    *largestMb = static_cast<uint32_t>(largest / kMb);
}

Calls g_calls = {Take, GiveBack, GameFree, Space};

double Mb(uint32_t bytes) { return static_cast<double>(bytes) / kMb; }

void Say(const Report& r, uint32_t blockAt) {
    if (r.ownFreed)
        Log("Frame memory: the mod's buffer of the block that went given back to the system");
    if (r.oldFreed)
        Log("Frame memory: the game's first buffer given back to its allocator");
    if (r.oldLeft)
        Log("Frame memory: the game's first buffer left where it is: the block names another allocator now");
    if (r.remade)
        Log("Frame memory: the game has made its block anew (the one the mod had made bigger is gone, or holds another buffer): the mod's buffer goes back to the system at the next frame's end");
    if (r.enough)
        Log("Frame memory: the game's block is %.0f MB, no smaller than the %u asked for: left as it is", Mb(r.was), g_wantedMb);
    if (r.strange) {
        const Block* block = At<Block>(blockAt);
        Log("Frame memory: the block isn't as the game makes one (buffer %08X, owned %u, allocator %08X): left alone", block->start, block->owned, block->allocator);
    }
    for (uint32_t i = 0; i < r.passedCount; ++i) {
        if (r.noPiece[i])
            Log("Frame memory: the system gave no piece of %u MB", r.passed[i]);
        else
            Log("Frame memory: %u MB not taken: the address space has %u MB free and its largest piece is %u MB, and the game needs room of its own", r.passed[i], r.freeMb, r.largestMb);
    }
    if (r.gaveUp)
        Log("Frame memory: nothing bigger to be had: the block stays the game's own %.0f MB (its use is still measured, and the scope's view left out of a crowded frame)", Mb(r.was));
    if (r.enlargedTo)
        Log("Frame memory: the block made %.0f MB in place (it was %.0f MB at %08X; now at %08X, a buffer of the mod's own); the game's first buffer goes back to its allocator at the next "
            "frame's end. The address space had %u MB free, its largest piece %u MB",
            Mb(r.enlargedTo), Mb(r.was), r.oldStart, r.newStart, r.freeMb, r.largestMb);
}

void AtEndOfFrame(uint8_t* self) {
    const uint32_t blockAt = Word(self + kBlock);
    if (const Block* block = At<Block>(blockAt))
        g_meter.EndOfFrame(block->used, block->size);
    const Report r = g_keeper.EndOfFrame(self, g_wantedMb, g_calls);
    if (const Block* block = At<Block>(blockAt))
        g_meter.SizeNow(block->size);
    Say(r, blockAt);
}

// Which faults the guards below take: only a bad read or write by an instruction of the mod's own (a block that isn't where it was read to be). One inside the game's code called from here
// (its allocator's free) goes on to the crash record like any other: taken here, it would leave the game's heap locked and the game hung.
int OwnFault(const EXCEPTION_POINTERS* info) {
    if (info->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;
    MEMORY_BASIC_INFORMATION own;
    if (!VirtualQuery(reinterpret_cast<const void*>(&OwnFault), &own, sizeof(own)) || !own.AllocationBase)
        return EXCEPTION_CONTINUE_SEARCH;
    const auto* base = static_cast<const uint8_t*>(own.AllocationBase);
    const auto* headers = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + reinterpret_cast<const IMAGE_DOS_HEADER*>(base)->e_lfanew);
    const auto* at = static_cast<const uint8_t*>(info->ExceptionRecord->ExceptionAddress);
    return at >= base && at < base + headers->OptionalHeader.SizeOfImage ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}

// (A block that isn't where it was read to be must not stop the game here: after such a fault the frame's end is the game's alone again. The settings' side goes on by itself: a block
// already made bigger must still be kept when the game takes new settings, or the game would throw it away with the world renderers still holding its address.)
void AtEndOfFrameGuarded(uint8_t* self) {
    __try {
        AtEndOfFrame(self);
    } __except (OwnFault(GetExceptionInformation())) {
        if (!g_stopped.exchange(true, std::memory_order_relaxed))
            Log("Frame memory: a fault reading the game's block at a frame's end: left alone from here on");
    }
}

void BeforeSettingsGuarded(uint8_t* self) {
    __try {
        uint32_t came = 0;
        if (g_keeper.BeforeSettings(self, &came))
            Log("Frame memory: new shader settings came with %.0f MB for the block: given the block's own size, so the game keeps it", Mb(came));
    } __except (OwnFault(GetExceptionInformation())) {
        if (!g_settingsStopped.exchange(true, std::memory_order_relaxed))
            Log("Frame memory: a fault reading the game's waiting settings: left alone from here on");
    }
}

// Only the game's one shader system, the one its holder names: the keeper knows one block. (Its constructor takes its first settings before the holder names it: passed on, it has no block yet.)
bool TheShaderSystem(const uint8_t* self) {
    uint32_t held = 0;
    return self && camera::SafeRead(kHolder, &held, sizeof(held)) && held == static_cast<uint32_t>(reinterpret_cast<uintptr_t>(self));
}

// The shader system's end of frame (thiscall, one argument, ret 4), on whatever thread renders: the mod's part before the game's own code empties the block.
void* __fastcall EndFrameHook(uint8_t* self, void* edx, void* out) {
    if (!g_stopped.load(std::memory_order_relaxed) && TheShaderSystem(self))
        AtEndOfFrameGuarded(self);
    return g_endFrame(self, edx, out);
}

// The taking of waiting settings (thiscall, no arguments), from the frame's start on the client side.
void __fastcall SettingsHook(uint8_t* self, void* edx) {
    if (!g_settingsStopped.load(std::memory_order_relaxed) && TheShaderSystem(self))
        BeforeSettingsGuarded(self);
    g_settings(self, edx);
}

const Code* NotAsRead() {
    for (const Code& code : kCode) {
        uint8_t now[16];
        if (!camera::SafeRead(code.at, now, code.size) || std::memcmp(now, code.bytes, code.size) != 0)
            return &code;
    }
    static constexpr Code kSlot = {kEndFrameSlot, 4, {}, "the shader system's table (+28h)"};
    uint32_t slot = 0;
    return camera::SafeRead(kEndFrameSlot, &slot, sizeof(slot)) && slot == kEndFrameAt ? nullptr : &kSlot;
}

void Install() {
    if (g_settled || g_tries++ % 16)
        return;
    // (The game's code is decrypted as it starts: looked at again every 16 frames till it's there. Another version of the game: never.)
    if (const Code* odd = NotAsRead()) {
        if (g_tries > 16 * 135 && !std::exchange(g_saidOdd, true))
            Log("Frame memory: the game's code isn't as it was read (%s, at %08X): nothing hooked, nothing measured", odd->what, static_cast<unsigned>(odd->at));
        return;
    }
    g_settled = true;
    g_layout.store(true, std::memory_order_release);
    const int mb = config::Get().frameMemoryMb;
    if (mb <= 0) {
        Log("Frame memory: left as the game has it ([world] frame_memory_mb=0): nothing hooked, nothing measured");
        return;
    }
    const auto endFrameAt = reinterpret_cast<void*>(kEndFrameAt), settingsAt = reinterpret_cast<void*>(kSettingsAt);
    void* endFrame = nullptr;
    void* settings = nullptr;
    if (MH_CreateHook(settingsAt, reinterpret_cast<void*>(&SettingsHook), &settings) != MH_OK) {
        Log("Frame memory: could not hook the taking of waiting settings: the block left as the game has it, nothing measured");
        return;
    }
    if (MH_CreateHook(endFrameAt, reinterpret_cast<void*>(&EndFrameHook), &endFrame) != MH_OK) {
        MH_RemoveHook(settingsAt);
        Log("Frame memory: could not hook the shader system's end of frame: the block left as the game has it, nothing measured");
        return;
    }
    g_settings = reinterpret_cast<SettingsFn>(settings);
    g_endFrame = reinterpret_cast<EndFrameFn>(endFrame);
    g_wantedMb = static_cast<uint32_t>(mb);
    // The settings' side first: the block is never made bigger without it.
    if (MH_EnableHook(settingsAt) != MH_OK || MH_EnableHook(endFrameAt) != MH_OK) {
        MH_DisableHook(settingsAt);
        MH_RemoveHook(settingsAt);
        MH_RemoveHook(endFrameAt);
        Log("Frame memory: could not switch its hooks on: the block left as the game has it, nothing measured");
        return;
    }
    Log("Frame memory: hooked: the block is made %u MB at the next frame's end, and every frame's use of it is measured", g_wantedMb);
}

}  // namespace

void OnFrame() {
    Install();
    const ULONGLONG now = GetTickCount64();
    if (now - g_lastLine < 1000)
        return;
    g_lastLine = now;
    const Meter::Second s = g_meter.TakeSecond();
    if (!s.frames)
        return;
    Log("Frame memory: peak %.1f of %.0f MB (the scope's view %.1f)", Mb(s.peak), Mb(s.size), Mb(s.scope));
    if (s.over)
        Log("Frame memory: warning: %u frames asked for more than the whole block (%.1f of %.0f MB at the most): requests were refused, and the game looks at none of what comes back", s.over,
            Mb(s.peak), Mb(s.size));
    else if (static_cast<uint64_t>(s.peak) * 100 > static_cast<uint64_t>(s.size) * 75)
        Log("Frame memory: warning: a frame took %.1f of %.0f MB, more than three quarters of the block", Mb(s.peak), Mb(s.size));
}

uint32_t Used() {
    uint32_t system = 0, block = 0, used = 0;
    if (!g_layout.load(std::memory_order_acquire) || !camera::SafeRead(kHolder, &system, sizeof(system)) || !system || !camera::SafeRead(system + kBlock, &block, sizeof(block)) || !block ||
        !camera::SafeRead(block + offsetof(Block, used), &used, sizeof(used)))
        return 0;
    return used;
}

void NoteScopeShare(uint32_t bytes) { g_meter.NoteScope(bytes); }

// (After a fault at a frame's end nothing is measured any more, and the last frame's numbers would go on deciding for ever: the net is off then, as it is while nothing has been measured.)
bool RoomForScope() { return g_stopped.load(std::memory_order_relaxed) || g_meter.RoomForScope(); }

Reading Last() { return {static_cast<float>(Mb(g_meter.LastWithout())), static_cast<float>(Mb(g_meter.ScopeShare())), static_cast<float>(Mb(g_meter.Size()))}; }

void LogForCrash() {
    uint32_t system = 0, at = 0;
    Block block{};
    if (!g_layout.load(std::memory_order_acquire) || !camera::SafeRead(kHolder, &system, sizeof(system)) || !system || !camera::SafeRead(system + kBlock, &at, sizeof(at)) || !at ||
        !camera::SafeRead(at, &block, sizeof(block))) {
        Log("CRASH? frame memory: not read (no shader system yet, or the game's code isn't as it was read)");
        return;
    }
    Log("CRASH? frame memory: %u of %u bytes handed out so far this frame (%.1f of %.0f MB)%s; the frame before took %.1f MB", block.used, block.size, Mb(block.used), Mb(block.size),
        block.used > block.size ? ": more than there is, so requests were being refused, and a null from one is the likely cause" : "", Mb(g_meter.LastPeak()));
}

}

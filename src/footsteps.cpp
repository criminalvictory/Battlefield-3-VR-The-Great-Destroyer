#include "footsteps.h"

#include <windows.h>
#include <MinHook.h>

#include <atomic>
#include <cstdint>
#include <cstring>

#include "camera.h"
#include "log.h"

namespace {

// The animation-event entity's "tag entered" (thiscall: self, a word not used, the tag; ret 8) and a sound entity's event handler (thiscall: the event, its id at +4; ret 4),
// each checked by its first bytes (read from bf3_mem.exe, the footsteps study 2026-11-05).
constexpr uintptr_t kTagAt = 0x010A6CD0, kEventAt = 0x00665DD0;
constexpr uint8_t kTagStart[] = {0x83, 0xEC, 0x08, 0x56, 0x57, 0x8B, 0xF9, 0x0F, 0xB6, 0x47, 0x1C};
constexpr uint8_t kEventStart[] = {0x83, 0xEC, 0x1C, 0x53, 0x55, 0x56, 0x8B, 0xF1};
// The first-person walk and run's two step tags, the step events they send the footsteps, its start and stop (the soldier's data).
constexpr uint32_t kStepTag[2] = {0xFDE0A58Au, 0xC3E97083u};
constexpr uint32_t kStepEvent[2] = {0xD9FEB84Bu, 0x184BEDD0u};
constexpr uint32_t kStartEvent = 0xE6810570u, kStopEvent = 0xFB4D5CB9u;

using TagFn = uintptr_t(__fastcall*)(void* self, void* edx, void* unused, int32_t tag);
using EventFn = uintptr_t(__fastcall*)(void* self, void* edx, void* event);
TagFn g_tag = nullptr;
EventFn g_event = nullptr;

std::atomic<bool> g_wanted{false};
bool g_hooked = false, g_failed = false;
std::atomic<uint32_t> g_tags{0}, g_steps{0}, g_starts{0}, g_stops{0};

uint32_t EventId(const void* event) {
    __try {
        return event ? *reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(event) + 4) : 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

// (Called from the game's animation and sound workers: counts only.)
uintptr_t __fastcall TagHook(void* self, void* edx, void* unused, int32_t tag) {
    const auto t = static_cast<uint32_t>(tag);
    if (t == kStepTag[0] || t == kStepTag[1])
        g_tags.fetch_add(1, std::memory_order_relaxed);
    return g_tag(self, edx, unused, tag);
}

uintptr_t __fastcall EventHook(void* self, void* edx, void* event) {
    const uint32_t id = EventId(event);
    if (id == kStepEvent[0] || id == kStepEvent[1])
        g_steps.fetch_add(1, std::memory_order_relaxed);
    else if (id == kStartEvent)
        g_starts.fetch_add(1, std::memory_order_relaxed);
    else if (id == kStopEvent)
        g_stops.fetch_add(1, std::memory_order_relaxed);
    return g_event(self, edx, event);
}

bool SameCode(uintptr_t at, const uint8_t* code, size_t size) {
    uint8_t now[16];
    return size <= sizeof(now) && camera::SafeRead(at, now, size) && std::memcmp(now, code, size) == 0;
}

void Hook() {
    if (g_hooked || g_failed)
        return;
    if (!SameCode(kTagAt, kTagStart, sizeof(kTagStart)) || !SameCode(kEventAt, kEventStart, sizeof(kEventStart))) {
        g_failed = true;
        Log("Footsteps: the game's code isn't as read: nothing counted");
        return;
    }
    void* tagOriginal = nullptr;
    void* eventOriginal = nullptr;
    if (MH_CreateHook(reinterpret_cast<void*>(kTagAt), reinterpret_cast<void*>(&TagHook), &tagOriginal) != MH_OK) {
        g_failed = true;
        Log("Footsteps: the animation's tags couldn't be hooked");
        return;
    }
    if (MH_CreateHook(reinterpret_cast<void*>(kEventAt), reinterpret_cast<void*>(&EventHook), &eventOriginal) != MH_OK) {
        MH_RemoveHook(reinterpret_cast<void*>(kTagAt));
        g_failed = true;
        Log("Footsteps: the sounds' events couldn't be hooked");
        return;
    }
    g_tag = reinterpret_cast<TagFn>(tagOriginal);
    g_event = reinterpret_cast<EventFn>(eventOriginal);
    if (MH_EnableHook(reinterpret_cast<void*>(kTagAt)) != MH_OK || MH_EnableHook(reinterpret_cast<void*>(kEventAt)) != MH_OK) {
        MH_DisableHook(reinterpret_cast<void*>(kTagAt));
        MH_DisableHook(reinterpret_cast<void*>(kEventAt));
        g_failed = true;
        Log("Footsteps: the hooks couldn't be turned on");
        return;
    }
    g_hooked = true;
    Log("Footsteps: counting the walk's step tags and the footsteps' events");
}

}  // namespace

namespace footsteps {

void SetLog(bool on) {
    if (g_wanted.exchange(on) != on)
        Log("Footsteps: the count %s", on ? "on (steplog 1)" : "off");
}

void OnFrame() {
    if (!g_wanted.load(std::memory_order_relaxed))
        return;
    Hook();
    static ULONGLONG s_last = 0;
    const ULONGLONG now = GetTickCount64();
    if (!g_hooked || now - s_last < 1000)
        return;
    s_last = now;
    const uint32_t tags = g_tags.exchange(0), steps = g_steps.exchange(0), starts = g_starts.exchange(0), stops = g_stops.exchange(0);
    if (tags || steps || starts || stops)
        Log("Footsteps: last second, the walk's step tags %u, step events reaching a sound %u, the footsteps' start %u, stop %u", tags, steps, starts, stops);
}

}  // namespace footsteps

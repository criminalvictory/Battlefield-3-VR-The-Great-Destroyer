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
// each checked by its first bytes (read from bf3_mem.exe, the footsteps study 2026-11-05). The handler is every sound entity's, so the count is of step events to any
// soldier's sound: the player's come from the first-person walk's two tags, the other soldiers' from their sound component (by foot contact while their animation runs, by a
// timer of 3 to 5 a second each while the game has it switched off out of the picture: dozens of soldiers out of view read as hundreds of steps a second, which is the game's own).
constexpr uintptr_t kTagAt = 0x010A6CD0, kEventAt = 0x00665DD0;
constexpr uint8_t kTagStart[] = {0x83, 0xEC, 0x08, 0x56, 0x57, 0x8B, 0xF9, 0x0F, 0xB6, 0x47, 0x1C};
constexpr uint8_t kEventStart[] = {0x83, 0xEC, 0x1C, 0x53, 0x55, 0x56, 0x8B, 0xF1};
// The first-person walk and run's two step tags, the step events they send the footsteps, its start and stop (the soldier's data).
constexpr uint32_t kStepTag[2] = {0xFDE0A58Au, 0xC3E97083u};
constexpr uint32_t kStepEvent[2] = {0xD9FEB84Bu, 0x184BEDD0u};
constexpr uint32_t kStartEvent = 0xE6810570u, kStopEvent = 0xFB4D5CB9u;

// The game's animation manager keeps one list of things asking to have their animation switched on or off (and to be taken off its books): 96 entries at +0x3C0, the count at
// +0x6C0, emptied twice in each of its updates, written with no check. Two pushes are functions: push(what, whose) (thiscall, ret 8) for "switch on" at 0x525740 and its twin
// for "switch off" at 0x525780, alike but for one byte; two more kinds are written in line and cannot be reached. A view swung onto a field of soldiers and kept bodies in a
// tenth of a second asked for more than 96 switch-ons in one update: the 97th landed on the count itself, the next write went wild, and the game stopped (bf3.exe+12575C,
// 2026-11-12). Each asker asks again at the manager's next update while it still wants the change, so a push turned away is a soldier animated one update later.
constexpr uintptr_t kPushOnAt = 0x00525740, kPushOffAt = 0x00525780;
constexpr uint8_t kPushStart[] = {0x8B, 0x54, 0x24, 0x08, 0x56, 0x8B, 0x74, 0x24, 0x08, 0x57, 0x8D, 0xB9, 0xC0, 0x06, 0x00, 0x00};

using TagFn = uintptr_t(__fastcall*)(void* self, void* edx, void* unused, int32_t tag);
using EventFn = uintptr_t(__fastcall*)(void* self, void* edx, void* event);
TagFn g_tag = nullptr;
EventFn g_event = nullptr;

std::atomic<bool> g_wanted{false};
bool g_counting = false, g_countFailed = false, g_guardTried = false, g_guarded = false;
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

#if defined(_M_IX86)
void* g_pushOn = nullptr;   // the game's own two pushes, past the hooks
void* g_pushOff = nullptr;
volatile LONG g_pushes = 0;   // pushes through the two functions since the last line
volatile LONG g_refused = 0;  // of them, turned away from a list all but full
volatile LONG g_listMost = 0; // the fullest the list was found at a push since the last line

// From 80 entries on the push is turned away (16 places kept: for pushes already past this check on other threads, and for the two kinds written in line), with the registers as
// the game's own leaves them (the count in eax, the second word pushed in edx, ecx as it came). Else straight on into the game's own; eax and edx are the game's to overwrite there.
__declspec(naked) void PushOnHook() {
    __asm {
        mov eax, dword ptr [ecx + 6C0h]
        cmp eax, dword ptr [g_listMost]
        jbe counted
        mov dword ptr [g_listMost], eax
    counted:
        lock inc dword ptr [g_pushes]
        cmp eax, 80
        jae full
        jmp dword ptr [g_pushOn]
    full:
        lock inc dword ptr [g_refused]
        mov edx, dword ptr [esp + 8]
        ret 8
    }
}

__declspec(naked) void PushOffHook() {
    __asm {
        mov eax, dword ptr [ecx + 6C0h]
        cmp eax, dword ptr [g_listMost]
        jbe counted
        mov dword ptr [g_listMost], eax
    counted:
        lock inc dword ptr [g_pushes]
        cmp eax, 80
        jae full
        jmp dword ptr [g_pushOff]
    full:
        lock inc dword ptr [g_refused]
        mov edx, dword ptr [esp + 8]
        ret 8
    }
}
#endif

bool SameCode(uintptr_t at, const uint8_t* code, size_t size) {
    uint8_t now[16];
    return size <= sizeof(now) && camera::SafeRead(at, now, size) && std::memcmp(now, code, size) == 0;
}

// The list's guard: always, and on its own (it doesn't wait for the count's hooks or go by their checks).
void Guard() {
    if (g_guardTried)
        return;
    g_guardTried = true;
#if defined(_M_IX86)
    if (!SameCode(kPushOnAt, kPushStart, sizeof(kPushStart)) || !SameCode(kPushOffAt, kPushStart, sizeof(kPushStart))) {
        Log("Animation list: the game's code isn't as read: not guarded");
        return;
    }
    if (MH_CreateHook(reinterpret_cast<void*>(kPushOnAt), reinterpret_cast<void*>(&PushOnHook), &g_pushOn) != MH_OK ||
        MH_CreateHook(reinterpret_cast<void*>(kPushOffAt), reinterpret_cast<void*>(&PushOffHook), &g_pushOff) != MH_OK ||
        MH_EnableHook(reinterpret_cast<void*>(kPushOnAt)) != MH_OK || MH_EnableHook(reinterpret_cast<void*>(kPushOffAt)) != MH_OK) {
        MH_DisableHook(reinterpret_cast<void*>(kPushOnAt));
        MH_DisableHook(reinterpret_cast<void*>(kPushOffAt));
        Log("Animation list: couldn't be guarded");
        return;
    }
    g_guarded = true;
    Log("Animation list: guarded (96 places: switch-on and switch-off requests turned away from 80, asked again at the next update)");
#endif
}

// The count's two hooks: only while "steplog" asks for the count.
void Count() {
    if (g_counting || g_countFailed)
        return;
    if (!SameCode(kTagAt, kTagStart, sizeof(kTagStart)) || !SameCode(kEventAt, kEventStart, sizeof(kEventStart))) {
        g_countFailed = true;
        Log("Footsteps: the game's code isn't as read: nothing counted");
        return;
    }
    void* tagOriginal = nullptr;
    void* eventOriginal = nullptr;
    if (MH_CreateHook(reinterpret_cast<void*>(kTagAt), reinterpret_cast<void*>(&TagHook), &tagOriginal) != MH_OK) {
        g_countFailed = true;
        Log("Footsteps: the animation's tags couldn't be hooked");
        return;
    }
    if (MH_CreateHook(reinterpret_cast<void*>(kEventAt), reinterpret_cast<void*>(&EventHook), &eventOriginal) != MH_OK) {
        MH_RemoveHook(reinterpret_cast<void*>(kTagAt));
        g_countFailed = true;
        Log("Footsteps: the sounds' events couldn't be hooked");
        return;
    }
    g_tag = reinterpret_cast<TagFn>(tagOriginal);
    g_event = reinterpret_cast<EventFn>(eventOriginal);
    if (MH_EnableHook(reinterpret_cast<void*>(kTagAt)) != MH_OK || MH_EnableHook(reinterpret_cast<void*>(kEventAt)) != MH_OK) {
        MH_DisableHook(reinterpret_cast<void*>(kTagAt));
        MH_DisableHook(reinterpret_cast<void*>(kEventAt));
        g_countFailed = true;
        Log("Footsteps: the hooks couldn't be turned on");
        return;
    }
    g_counting = true;
    Log("Footsteps: counting the walk's step tags and the step events to every soldier's sound");
}

}  // namespace

namespace footsteps {

void SetLog(bool on) {
    if (g_wanted.exchange(on) != on)
        Log("Footsteps: the count %s", on ? "on (steplog 1)" : "off");
}

void OnFrame() {
    Guard();
    const bool wanted = g_wanted.load(std::memory_order_relaxed);
    if (wanted)
        Count();
    static ULONGLONG s_last = 0;
    const ULONGLONG now = GetTickCount64();
    if (now - s_last < 1000)
        return;
    s_last = now;
#if defined(_M_IX86)
    if (g_guarded) {
        const long pushes = InterlockedExchange(&g_pushes, 0), refused = InterlockedExchange(&g_refused, 0), most = InterlockedExchange(&g_listMost, 0);
        // (A line when the list came half full or a push was turned away: ordinary play stays far under.)
        if (refused || most >= 48)
            Log("Animation list: last second %ld switch requests, the list at most %ld of 96 full, %ld turned away to the next update", pushes, most, refused);
    }
#endif
    if (!g_counting || !wanted)
        return;
    const uint32_t tags = g_tags.exchange(0), steps = g_steps.exchange(0), starts = g_starts.exchange(0), stops = g_stops.exchange(0);
    if (tags || steps || starts || stops)
        Log("Footsteps: last second, the walk's step tags %u, step events reaching a sound %u, the footsteps' start %u, stop %u", tags, steps, starts, stops);
}

}  // namespace footsteps

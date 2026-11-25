#include "pad.h"

#include <windows.h>
#include <unknwn.h>
#include <MinHook.h>
#include <xinput.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>

#include "log.h"

namespace {

using GetStateFn = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);
using GetCapabilitiesFn = DWORD(WINAPI*)(DWORD, DWORD, XINPUT_CAPABILITIES*);
GetStateFn g_getState = nullptr;
GetCapabilitiesFn g_getCapabilities = nullptr;
bool g_tried = false, g_hooked = false;

std::atomic<SHORT> g_x{0}, g_y{0};
std::atomic<ULONGLONG> g_setAt{0};  // when the mod last set the stick (it lets go if that stops)
// Buttons held on controller 0 for a while (PressButtons): the game's own actions that only a pad has (its keyboard has
// no key for the grenade launcher under the rifle: the user's profile binds ConceptSwitchToGrenadeLauncher on the pad only).
std::atomic<WORD> g_buttons{0};
std::atomic<ULONGLONG> g_buttonsUntil{0};

WORD ButtonsNow() {
    return GetTickCount64() < g_buttonsUntil.load(std::memory_order_relaxed) ? g_buttons.load(std::memory_order_relaxed) : WORD{0};
}
std::atomic<DWORD> g_packet{1};
std::atomic<bool> g_polled{false};
// When the game last asked for controller 0 with the mod's stick on: it does every frame while it reads it. Not asked
// for this long, the stick goes nowhere and walking goes on W/A/S/D (input.cpp) until it asks again.
std::atomic<ULONGLONG> g_readAt{0};
constexpr ULONGLONG kReadWithin = 500;
std::atomic<int> g_asked{0};  // the game's XInputGetState calls, any controller, the stick on or not (the log's)
// What each controller XInput itself answers for had down, the last time it had anything (buttons | left trigger << 16 | right << 24), and when
// (pad::RealPressed: the fights' lines). Only a controller there (XInput's own ERROR_SUCCESS), before the mod's buttons go in.
constexpr DWORD kRealControllers = 4;
std::atomic<uint32_t> g_realState[kRealControllers];
std::atomic<ULONGLONG> g_realAt[kRealControllers];
std::atomic<bool> g_realKnown{false};  // XInput hooked
constexpr ULONGLONG kRealWithin = 200;

void NoteReal(DWORD index, const XINPUT_GAMEPAD& g) {
    if (index >= kRealControllers ||
        (!g.wButtons && g.bLeftTrigger <= XINPUT_GAMEPAD_TRIGGER_THRESHOLD && g.bRightTrigger <= XINPUT_GAMEPAD_TRIGGER_THRESHOLD))
        return;
    g_realState[index].store(static_cast<uint32_t>(g.wButtons) | static_cast<uint32_t>(g.bLeftTrigger) << 16 | static_cast<uint32_t>(g.bRightTrigger) << 24,
                             std::memory_order_relaxed);
    g_realAt[index].store(GetTickCount64(), std::memory_order_relaxed);
}
// Not asked for this long, the game is told the devices changed (WM_DEVICECHANGE, as Windows tells it when a
// controller is plugged in), at most kKicks times a stall: its controllers looked for again.
constexpr ULONGLONG kKickAfter = 2000;
constexpr int kKicks = 3;

bool Active() {
    return GetTickCount64() - g_setAt.load(std::memory_order_relaxed) < 250;
}

// How the game comes to read a controller at all (bf3.exe 1.6, read 2026-11-02): as it starts, IDirectInput8::EnumDevices
// lists the game controllers attached (DI8DEVCLASS_GAMECTRL), and for each that's an XInput device (kIsXInput, by its
// product id through WMI) kPadCallback makes a pad object with the next controller index, which asks XInput from then
// on. With nothing attached there's no pad object, and XInput is never called again: the left stick walks nothing. That
// was every run started with the controllers asleep ("i cannot move ... after fighting in the interrogation room", the
// user, 2026-11-01; the runs at 20:37 and 23:56 before it, relaunched with nobody in the headset), and neither a hook on
// XInput nor telling the game the devices changed brings it back. So when its list comes back with no controller, the
// callback is called once more for one that isn't there, the check told it's an XInput device: the game makes its own
// pad object for controller 0, on its own thread, in the middle of its own start-up.
constexpr uintptr_t kPadCallback = 0x00FD93F0, kIsXInput = 0x00FC3010;
constexpr size_t kManagerPads = 0x24;  // the manager's count of XInput pads (the callback's reference, +0x24)
constexpr uint8_t kPadCallbackStart[] = {0x51, 0xA1, 0xC4, 0x39, 0x2E, 0x02, 0x53, 0x32, 0xDB};
constexpr uint8_t kIsXInputStart[] = {0x81, 0xEC, 0x88, 0x00, 0x00, 0x00, 0x53, 0x56, 0x33, 0xDB};
constexpr DWORD kGameControllers = 4;  // DI8DEVCLASS_GAMECTRL
using EnumDevicesFn = HRESULT(STDMETHODCALLTYPE*)(void*, DWORD, void*, void*, DWORD);
EnumDevicesFn g_enumDevices[2] = {};
using IsXInputFn = bool(__cdecl*)(const void*);
IsXInputFn g_isXInput = nullptr;
const void* g_noController = nullptr;

bool __cdecl IsXInputOrOurs(const void* product) {
    return product == g_noController || g_isXInput(product);
}

void AddController(void* callback, void* manager) {
    if (reinterpret_cast<uintptr_t>(callback) != kPadCallback || !manager)
        return;
    const int found = *reinterpret_cast<const int*>(static_cast<const char*>(manager) + kManagerPads);
    if (found > 0) {
        Log("Pad: the game found %d XInput controller%s as it started", found, found == 1 ? "" : "s");
        return;
    }
    if (std::memcmp(callback, kPadCallbackStart, sizeof(kPadCallbackStart)) != 0 ||
        std::memcmp(reinterpret_cast<const void*>(kIsXInput), kIsXInputStart, sizeof(kIsXInputStart)) != 0) {
        Log("Pad: the game found no controller, and its code isn't as expected: none added (walking on W/A/S/D)");
        return;
    }
    void* check = reinterpret_cast<void*>(kIsXInput);
    if (MH_CreateHook(check, reinterpret_cast<void*>(&IsXInputOrOurs), reinterpret_cast<void**>(&g_isXInput)) != MH_OK ||
        MH_EnableHook(check) != MH_OK) {
        Log("Pad: the game found no controller, and one couldn't be added (walking on W/A/S/D)");
        return;
    }
    // A DIDEVICEINSTANCE: only its product id's place is looked at on this path (the check gets a pointer to it).
    alignas(8) static unsigned char instance[0x400] = {};
    g_noController = instance + 0x14;
    reinterpret_cast<BOOL(CALLBACK*)(const void*, void*)>(callback)(instance, manager);
    g_noController = nullptr;
    MH_DisableHook(check);
    MH_RemoveHook(check);
    Log("Pad: the game found no controller as it started: controller 0 added (%d now), so it reads the walking stick",
        *reinterpret_cast<const int*>(static_cast<const char*>(manager) + kManagerPads));
}

HRESULT STDMETHODCALLTYPE EnumDevices0(void* self, DWORD type, void* callback, void* ref, DWORD flags) {
    const HRESULT hr = g_enumDevices[0](self, type, callback, ref, flags);
    if (type == kGameControllers)
        AddController(callback, ref);
    return hr;
}
HRESULT STDMETHODCALLTYPE EnumDevices1(void* self, DWORD type, void* callback, void* ref, DWORD flags) {
    const HRESULT hr = g_enumDevices[1](self, type, callback, ref, flags);
    if (type == kGameControllers)
        AddController(callback, ref);
    return hr;
}

// The list's function, found on a throwaway DirectInput object of each kind (their tables' fifth entry).
void HookEnumDevices() {
    HMODULE dinput = LoadLibraryW(L"dinput8.dll");
    auto create = dinput ? reinterpret_cast<HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, void**, void*)>(
                               GetProcAddress(dinput, "DirectInput8Create"))
                         : nullptr;
    if (!create) {
        Log("Pad: DirectInput isn't there to hook");
        return;
    }
    static const GUID kIds[2] = {{0xBF798030, 0x483A, 0x4DA2, {0xAA, 0x99, 0x5D, 0x64, 0xED, 0x36, 0x97, 0x00}},   // A
                                 {0xBF798031, 0x483A, 0x4DA2, {0xAA, 0x99, 0x5D, 0x64, 0xED, 0x36, 0x97, 0x00}}};  // W
    void* hooks[2] = {reinterpret_cast<void*>(&EnumDevices0), reinterpret_cast<void*>(&EnumDevices1)};
    for (int i = 0; i < 2; ++i) {
        IUnknown* object = nullptr;
        if (FAILED(create(GetModuleHandleW(nullptr), 0x0800, kIds[i], reinterpret_cast<void**>(&object), nullptr)) || !object)
            continue;
        void* target = (*reinterpret_cast<void***>(object))[4];
        object->Release();
        if (MH_CreateHook(target, hooks[i], reinterpret_cast<void**>(&g_enumDevices[i])) == MH_OK)
            MH_EnableHook(target);
    }
    Log("Pad: DirectInput's list of controllers hooked");
}

BOOL CALLBACK FindGameWindow(HWND hwnd, LPARAM out) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == GetCurrentProcessId() && IsWindowVisible(hwnd) && !GetWindow(hwnd, GW_OWNER)) {
        *reinterpret_cast<HWND*>(out) = hwnd;
        return FALSE;
    }
    return TRUE;
}

// Controller 0 is there whenever the game's pad object asks (AddController makes one if none was attached), the mod's
// stick at rest until it's set.
DWORD WINAPI GetState(DWORD index, XINPUT_STATE* state) {
    const DWORD result = g_getState(index, state);
    g_asked.fetch_add(1, std::memory_order_relaxed);
    if (state && result == ERROR_SUCCESS)
        NoteReal(index, state->Gamepad);  // (a real controller's own state, before the mod's buttons: the fights' lines)
    if (index != 0 || !state)
        return result;
    g_readAt.store(GetTickCount64(), std::memory_order_relaxed);
    if (!g_polled.exchange(true))
        Log("Pad: the game reads controller 0; walking goes through its left stick");
    const bool active = Active();
    const WORD buttons = ButtonsNow();
    static WORD s_buttonsWere = 0;
    if (buttons != s_buttonsWere) {
        s_buttonsWere = buttons;
        ++g_packet;
    }
    if (!active && !buttons && result == ERROR_SUCCESS)
        return result;  // a real controller, the mod's stick not in use
    if (result != ERROR_SUCCESS)
        std::memset(state, 0, sizeof(*state));  // no controller there: only the mod's stick
    state->dwPacketNumber = g_packet.load(std::memory_order_relaxed);
    state->Gamepad.sThumbLX = active ? g_x.load(std::memory_order_relaxed) : SHORT{0};
    state->Gamepad.sThumbLY = active ? g_y.load(std::memory_order_relaxed) : SHORT{0};
    state->Gamepad.wButtons |= buttons;
    return ERROR_SUCCESS;
}

DWORD WINAPI GetCapabilities(DWORD index, DWORD flags, XINPUT_CAPABILITIES* caps) {
    const DWORD result = g_getCapabilities(index, flags, caps);
    if (index != 0 || !caps || result == ERROR_SUCCESS)
        return result;
    std::memset(caps, 0, sizeof(*caps));
    caps->Type = XINPUT_DEVTYPE_GAMEPAD;
    caps->SubType = XINPUT_DEVSUBTYPE_GAMEPAD;
    caps->Gamepad.sThumbLX = caps->Gamepad.sThumbLY = static_cast<SHORT>(0xFFC0);  // full range
    return ERROR_SUCCESS;
}

bool Hook() {
    g_tried = true;
    HMODULE xinput = GetModuleHandleW(L"XINPUT9_1_0.dll");
    if (!xinput)
        xinput = GetModuleHandleW(L"XInput1_3.dll");
    if (!xinput)  // too early for the game to have it: the one it goes on to use (seen in its modules)
        xinput = LoadLibraryW(L"XINPUT9_1_0.dll");
    void* getState = xinput ? reinterpret_cast<void*>(GetProcAddress(xinput, "XInputGetState")) : nullptr;
    void* getCapabilities = xinput ? reinterpret_cast<void*>(GetProcAddress(xinput, "XInputGetCapabilities")) : nullptr;
    if (!getState || !getCapabilities ||
        MH_CreateHook(getState, reinterpret_cast<void*>(&GetState), reinterpret_cast<void**>(&g_getState)) != MH_OK ||
        MH_CreateHook(getCapabilities, reinterpret_cast<void*>(&GetCapabilities),
                      reinterpret_cast<void**>(&g_getCapabilities)) != MH_OK ||
        MH_EnableHook(getState) != MH_OK || MH_EnableHook(getCapabilities) != MH_OK) {
        Log("Pad: the game's XInput couldn't be hooked; walking stays on W/A/S/D");
        return false;
    }
    g_realKnown.store(true, std::memory_order_relaxed);
    Log("Pad: XInput hooked");
    return true;
}

}

namespace pad {

Real RealPressed() {
    Real r;
    r.known = g_realKnown.load(std::memory_order_relaxed);
    const ULONGLONG now = GetTickCount64();
    for (DWORD i = 0; i < kRealControllers; ++i) {
        const ULONGLONG at = g_realAt[i].load(std::memory_order_relaxed);
        if (!at || now < at || now - at > kRealWithin)
            continue;
        const uint32_t v = g_realState[i].load(std::memory_order_relaxed);
        r.any = true;
        r.index = static_cast<int>(i);
        r.buttons = static_cast<unsigned short>(v & 0xFFFF);
        r.left = static_cast<unsigned char>(v >> 16 & 0xFF);
        r.right = static_cast<unsigned char>(v >> 24 & 0xFF);
        break;
    }
    return r;
}

void PressButtons(unsigned short mask, unsigned milliseconds) {
    if (!g_tried)
        g_hooked = Hook();
    g_buttons.store(mask, std::memory_order_relaxed);
    g_buttonsUntil.store(GetTickCount64() + milliseconds, std::memory_order_relaxed);
    Log("Pad: buttons %04X held on controller 0 for %u ms%s", mask, milliseconds, g_polled.load() ? "" : " (the game hasn't read it yet)");
}

void Install() {
    if (!g_tried)
        g_hooked = Hook();
    HookEnumDevices();
}

bool SetMove(float x, float y) {
    if (!g_tried)
        g_hooked = Hook();
    if (!g_hooked)
        return false;
    // Past the game's own dead zone (XInput's usual 24% of the range) as soon as the stick moves
    // at all, then on to full: a small push walks slowly rather than not at all.
    constexpr float kGameDeadZone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / 32767.0f;
    const float length = std::sqrt(x * x + y * y);
    SHORT sx = 0, sy = 0;
    if (length > 0.001f) {
        const float scale = (kGameDeadZone + (1.0f - kGameDeadZone) * std::min(length, 1.0f)) / length * 32767.0f;
        sx = static_cast<SHORT>(std::clamp(x * scale, -32767.0f, 32767.0f));
        sy = static_cast<SHORT>(std::clamp(y * scale, -32767.0f, 32767.0f));
    }
    if (sx != g_x.load() || sy != g_y.load()) {
        g_x = sx;
        g_y = sy;
        ++g_packet;
    }
    const ULONGLONG now = GetTickCount64();
    static ULONGLONG s_since = 0;  // since when the mod's stick has been on without a break
    if (now - g_setAt.load() >= 250)
        s_since = now;
    g_setAt = now;

    // Walking through the stick only while the game reads it (each frame when it does).
    const ULONGLONG readAt = g_readAt.load(std::memory_order_relaxed);
    const bool read = readAt && now - readAt < kReadWithin;
    static bool s_read = true;
    static int s_kicks = 0;
    static ULONGLONG s_kickedAt = 0;
    if (read != s_read && (read || now - s_since >= kReadWithin)) {
        s_read = read;
        if (read) {
            Log("Pad: the game reads controller 0 again%s; walking goes through its left stick",
                s_kicks ? " (after being told the devices changed)" : "");
            s_kicks = 0;
        } else {
            Log("Pad: the game isn't reading controller 0; walking on W/A/S/D until it does (it asked for any "
                "controller %d times since the hook)",
                g_asked.load());
        }
    }
    // Not read for a while (the stick on all along): the game told the devices changed, so it looks for its
    // controllers again; a few times a stall, not to keep it re-scanning.
    const ULONGLONG quiet = now - std::max(readAt, s_since);
    if (!read && quiet >= kKickAfter && s_kicks < kKicks && now - s_kickedAt >= kKickAfter) {
        HWND game = nullptr;
        EnumWindows(FindGameWindow, reinterpret_cast<LPARAM>(&game));
        if (game) {
            ++s_kicks;
            s_kickedAt = now;
            PostMessageW(game, WM_DEVICECHANGE, 0x0007 /* DBT_DEVNODES_CHANGED */, 0);
            Log("Pad: controller 0 not read for %.1f s; the game told the devices changed (%d of %d)", quiet / 1000.0,
                s_kicks, kKicks);
        }
    }
    return read;
}

}

#pragma once

// Which OpenXR runtime the game asks for, decided before the OpenXR loader starts (xr_runtime.cpp PreferSteamVr). BF3 is a
// 32-bit game, so it needs a runtime's 32-bit half. SteamVR's (steamxr_win32.json) is the one the mod is built and tested on;
// Meta's own 32-bit runtime has been reported to fail creating a session for 32-bit games. So whenever SteamVR's 32-bit
// runtime is on the PC and the runtime the loader would take is another (a Quest PC's Meta Horizon runtime, or none at all
// registered for 32 bits), the game is pointed at SteamVR's through XR_RUNTIME_JSON, the loader's own first choice, in the
// game's own environment only: nothing on the PC is changed. bf3vr.ini [general] prefer_steamvr=0 leaves the loader's choice.
// Header only, nothing of Windows: tools/campaigntest.cpp runs it offline.

#include <string>

namespace xrpick {

inline wchar_t Lower(wchar_t c) { return c >= L'A' && c <= L'Z' ? static_cast<wchar_t>(c - L'A' + L'a') : c; }

// The file's name alone, lowered: "C:\x\SteamVR\steamxr_win32.json" gives "steamxr_win32.json".
inline std::wstring FileName(const std::wstring& path) {
    const size_t slash = path.find_last_of(L"\\/");
    std::wstring name = slash == std::wstring::npos ? path : path.substr(slash + 1);
    while (!name.empty() && (name.back() == L' ' || name.back() == L'"'))
        name.pop_back();
    for (wchar_t& c : name)
        c = Lower(c);
    return name;
}

// SteamVR's 32-bit runtime manifest, by its file's name.
inline bool IsSteamVr32(const std::wstring& manifest) { return FileName(manifest) == L"steamxr_win32.json"; }

struct Choice {
    bool set = false;       // point XR_RUNTIME_JSON at `manifest`
    std::wstring manifest;  // SteamVR's 32-bit manifest, when set
    const char* why = "";   // for the log
};

// override: XR_RUNTIME_JSON as the game has it now (after FixRuntimeOverride's 64-to-32 correction; empty: not set).
// active: the 32-bit ActiveRuntime registered on the PC (empty: none). steamVr: SteamVR's 32-bit manifest found on the PC (a
// file that is there; empty: none). prefer: bf3vr.ini [general] prefer_steamvr.
inline Choice Pick(const std::wstring& override, const std::wstring& active, const std::wstring& steamVr, bool prefer) {
    Choice c;
    const std::wstring& current = override.empty() ? active : override;
    if (!prefer) {
        c.why = "left to the loader (bf3vr.ini [general] prefer_steamvr=0)";
        return c;
    }
    if (IsSteamVr32(current)) {
        c.why = override.empty() ? "SteamVR's is the PC's active 32-bit runtime" : "XR_RUNTIME_JSON already names SteamVR's 32-bit runtime";
        return c;
    }
    if (steamVr.empty()) {
        c.why = current.empty() ? "no 32-bit runtime registered and SteamVR's isn't found: the loader will find none"
                                : "SteamVR's 32-bit runtime isn't found: the active one is used";
        return c;
    }
    c.set = true;
    c.manifest = steamVr;
    c.why = current.empty()       ? "no 32-bit OpenXR runtime is registered on this PC"
            : override.empty()    ? "the PC's active 32-bit runtime is another (SteamVR's is preferred for this 32-bit game)"
                                  : "XR_RUNTIME_JSON named another runtime (SteamVR's is preferred for this 32-bit game)";
    return c;
}

}  // namespace xrpick

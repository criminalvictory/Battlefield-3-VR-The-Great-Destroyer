#pragma once
// Other programs that load themselves into the game from its folder (a proxy DLL: a dxgi.dll, d3d11.dll, dinput8.dll... beside bf3.exe, which
// Windows loads in place of its own) and draw over its picture or take its input: ReShade, 3DMigoto, DXVK, ENB, Special K, SweetFX, an ASI loader's
// plugins, x360ce, or any other such DLL. They hook what the mod hooks: a black headset, a crash as the game starts, one eye's picture wrong, a
// walking stick that stops. The launch report names each one found, with what to do (launchreport.cpp, as the mod starts: the game's folder
// looked at once). Only a DLL there loads anything: a program's other files alone (a ReShade.ini left behind) are nothing to say. Pure, header
// only: the folder's top-level names in, the findings out (tools/conflictstest.cpp).

#include <cctype>
#include <cstddef>
#include <string>
#include <vector>

namespace conflicts {

struct Entry {
    std::string name;  // as the folder has it
    bool folder = false;
};

struct Finding {
    std::string what;   // "ReShade"
    std::string files;  // its files found, as named ("dxgi.dll, ReShade.ini")
    std::string todo;   // why it matters and what to do
};

// The DLL names Windows takes from the game's folder before its own (none of them a known DLL): what proxies are named.
constexpr const char* kProxies[] = {"dxgi.dll",    "d3d11.dll", "d3d10.dll",     "d3d10_1.dll",   "d3d9.dll",      "dinput8.dll",
                                    "dinput.dll",  "version.dll", "winmm.dll",   "dsound.dll",    "xinput1_3.dll", "xinput1_4.dll",
                                    "xinput9_1_0.dll", "opengl32.dll", "winhttp.dll", "wininet.dll"};

// A program known by its own files beside its DLL: the files (and folders, and name endings) that are its signs, the proxy names its DLL goes
// by, and why it matters. Looked for in this order, each DLL named once (3DMigoto's d3d11.dll before ReShade's dxgi.dll: the two are often
// together).
struct Program {
    const char* what;
    const char* files[6];
    const char* folders[3];
    const char* ending;  // a name ending that's one of its signs (nullptr none)
    const char* dlls[8];
    const char* why;
};

constexpr Program kPrograms[] = {
    {"3DMigoto",
     {"d3dx.ini", "d3dx_user.ini", "d3d11_log.txt", "nvapi.dll"},
     {"shaderfixes", "shadercache"},
     nullptr,
     {"d3d11.dll"},
     "it replaces the game's Direct3D 11, which the mod and the headset's picture are made from"},
    {"DXVK",
     {"dxvk.conf", "bf3_d3d11.log", "bf3_dxgi.log", "bf3_d3d9.log"},
     {},
     ".dxvk-cache",
     {"d3d11.dll", "dxgi.dll", "d3d10.dll", "d3d10_1.dll", "d3d9.dll"},
     "it draws the game through Vulkan, and the mod and the VR runtime need the game's own Direct3D 11"},
    {"ReShade",
     {"reshade.ini", "reshadepreset.ini", "reshade.log", "reshade.log.old"},
     {"reshade-shaders", "reshade-presets"},
     nullptr,
     {"dxgi.dll", "d3d11.dll", "d3d9.dll", "opengl32.dll", "dinput8.dll"},
     "it draws over the game's picture where the mod hooks it, and can leave the headset black, crash the game or give one eye a different "
     "picture"},
    {"ENB", {"enbseries.ini", "enblocal.ini"}, {"enbseries", "enbcache"}, nullptr, {"d3d11.dll", "dxgi.dll", "d3d9.dll"},
     "it draws over the game's picture where the mod hooks it"},
    {"Special K", {"specialk32.dll", "specialk64.dll", "specialk.central"}, {}, nullptr, {"dxgi.dll", "d3d11.dll", "d3d9.dll", "dinput8.dll"},
     "it hooks the game's picture and input as the mod does"},
    {"SweetFX", {"sweetfx_settings.txt", "sweetfx_preset.txt", "injector.ini"}, {"sweetfx"}, nullptr, {"dxgi.dll", "d3d11.dll", "d3d9.dll"},
     "it draws over the game's picture where the mod hooks it"},
    {"An ASI loader's plugins", {}, {"scripts", "plugins"}, ".asi",
     {"dinput8.dll", "version.dll", "winmm.dll", "dsound.dll", "winhttp.dll", "wininet.dll", "xinput1_3.dll"},
     "they run inside the game beside the mod: if the game crashes or the headset stays black, they're the first to try without"},
    {"x360ce", {"x360ce.ini", "x360ce.gdb", "x360ce.exe"}, {}, nullptr, {"xinput1_3.dll", "xinput1_4.dll", "xinput9_1_0.dll", "dinput8.dll"},
     "it takes the game's Xbox controller input, which the mod's walking stick goes through: the stick may stop walking"},
};

namespace detail {

inline std::string Lower(const std::string& s) {
    std::string out(s);
    for (char& c : out)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

inline bool EndsWith(const std::string& s, const char* tail) {
    const std::string t(tail);
    return s.size() > t.size() && s.compare(s.size() - t.size(), t.size(), t) == 0;
}

inline bool IsProxy(const std::string& lower) {
    for (const char* p : kProxies)
        if (lower == p)
            return true;
    return false;
}

}  // namespace detail

// What to do with the DLLs found: renamed while playing in VR.
inline std::string RenameTodo(const std::vector<std::string>& dlls) {
    std::string s = "while you play in VR, rename ";
    for (size_t k = 0; k < dlls.size(); ++k) {
        if (k)
            s += k + 1 == dlls.size() ? " and " : ", ";
        s += dlls[k];
    }
    s += " in the game's folder (add .off to the end of the name), and rename it back afterwards";
    return s;
}

// Everything found among the game folder's top-level entries.
inline std::vector<Finding> Find(const std::vector<Entry>& entries) {
    std::vector<Finding> out;
    std::vector<std::string> lower;
    lower.reserve(entries.size());
    for (const Entry& e : entries)
        lower.push_back(detail::Lower(e.name));
    std::vector<bool> used(entries.size(), false);
    for (const Program& p : kPrograms) {
        // Its DLLs among the proxies not named yet.
        std::vector<size_t> dlls, signs;
        for (size_t i = 0; i < entries.size(); ++i) {
            if (used[i])
                continue;
            if (!entries[i].folder) {
                for (const char* d : p.dlls)
                    if (d && lower[i] == d)
                        dlls.push_back(i);
                for (const char* f : p.files)
                    if (f && lower[i] == f)
                        signs.push_back(i);
                if (p.ending && detail::EndsWith(lower[i], p.ending))
                    signs.push_back(i);
            } else {
                for (const char* f : p.folders)
                    if (f && lower[i] == f)
                        signs.push_back(i);
            }
        }
        if (dlls.empty() || signs.empty())
            continue;  // (its files without a DLL that loads it: nothing loads; a DLL without its files: another's, or the last finding's)
        Finding f;
        f.what = p.what;
        std::vector<std::string> dllNames;
        for (size_t i : dlls) {
            used[i] = true;
            dllNames.push_back(entries[i].name);
            f.files += (f.files.empty() ? "" : ", ") + entries[i].name;
        }
        for (size_t i : signs) {
            used[i] = true;
            f.files += ", " + entries[i].name + (entries[i].folder ? " (a folder)" : "");
        }
        f.todo = std::string(p.why) + ". What to do: " + RenameTodo(dllNames);
        out.push_back(f);
    }
    // Any other proxy DLL: some program's hook.
    std::vector<std::string> left;
    Finding f;
    f.what = "Another program's DLL";
    for (size_t i = 0; i < entries.size(); ++i)
        if (!used[i] && !entries[i].folder && detail::IsProxy(lower[i])) {
            left.push_back(entries[i].name);
            f.files += (f.files.empty() ? "" : ", ") + entries[i].name;
        }
    if (!left.empty()) {
        f.todo = std::string("Windows loads it into the game in place of its own (ReShade and tools like it are named so), and it may hook what the "
                             "mod hooks. If the headset stays black or the game crashes, ") +
                 RenameTodo(left);
        out.push_back(f);
    }
    return out;
}

}  // namespace conflicts

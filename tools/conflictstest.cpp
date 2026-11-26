// Other programs in the game's folder (src/conflicts.h, header only, as launchreport.cpp's GameFolderHooks runs it), without the game.
//   1  the game's own folder (Steam's, the test PC's; the EA app's has the same kinds of files): nothing found
//   2  ReShade (dxgi.dll beside ReShade.ini, its shaders' folder): found, dxgi.dll named, to be renamed while playing in VR
//   3  a program's files without a DLL that loads it (a ReShade.ini left behind, a dxvk.conf alone, .asi files alone): nothing
//   4  3DMigoto's d3d11.dll and ReShade's dxgi.dll together: two findings, each DLL its own program's
//   5  DXVK (d3d11.dll and dxgi.dll beside dxvk.conf or a cache); ENB; Special K; SweetFX; an ASI loader (dinput8.dll and .asi files, version.dll and
//      a scripts folder); x360ce (xinput1_3.dll and x360ce.ini)
//   6  any other proxy DLL (dinput8.dll, winmm.dll alone): "another program's DLL", all of them in one line
//   7  names in any case; a folder named like a DLL isn't one; each DLL named in one finding only
//   8  the words: every finding says why and what to do, naming its DLLs
//   9  this PC's own game folder, when it's there (read only, its names only; SKIP without it)
// Usage: conflictstest [<the game's folder>]
// The exit code is the number of failed checks.

#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

#include "../src/conflicts.h"

namespace {

int g_failed = 0, g_counted = 0, g_skipped = 0;

void Report(const char* id, const char* what, bool pass, const char* format, ...) {
    char numbers[2048];
    va_list args;
    va_start(args, format);
    std::vsnprintf(numbers, sizeof(numbers), format, args);
    va_end(args);
    std::printf("%-4s %-5s %s | %s\n", id, pass ? "PASS" : "FAIL", what, numbers);
    ++g_counted;
    g_failed += !pass;
}

void Skip(const char* id, const char* what) {
    std::printf("%-4s SKIP  %s\n", id, what);
    ++g_skipped;
}

using Entries = std::vector<conflicts::Entry>;

// The game's own folder (Steam's copy on the test PC, 2026-11-25; the EA app's has Core, __Installer and the like too).
Entries GameFolder() {
    return {{"Core", true},           {"Data", true},
            {"pb", true},             {"Support", true},
            {"Update", true},         {"__Installer", true},
            {"__overlay", true},      {"bf3.exe", false},
            {"bf3.par", false},       {"BF3WebHelper.exe", false},
            {"EAappInstaller_installScript.vdf", false},
            {"EAStore.ini", false},   {"Engine.BuildInfo_Win32_Retail_dll.dll", false},
            {"GDFBinary_en_US.dll", false}, {"installScript.vdf", false},
            {"pbsvc.exe", false},     {"steam_appid.txt", false},
            {"version.json", false}};
}

Entries With(const Entries& added) {
    Entries e = GameFolder();
    e.insert(e.end(), added.begin(), added.end());
    return e;
}

std::string Say(const std::vector<conflicts::Finding>& f) {
    std::string s;
    for (const conflicts::Finding& x : f)
        s += (s.empty() ? "" : " || ") + x.what + " [" + x.files + "]";
    return s.empty() ? "nothing" : s;
}

const conflicts::Finding* Named(const std::vector<conflicts::Finding>& f, const char* what) {
    for (const conflicts::Finding& x : f)
        if (x.what == what)
            return &x;
    return nullptr;
}

bool Has(const std::string& s, const char* part) {
    return s.find(part) != std::string::npos;
}

}  // namespace

int main(int argc, char** argv) {
    {
        const auto f = conflicts::Find(GameFolder());
        Report("1", "the game's own folder: nothing found", f.empty(), "%s", Say(f).c_str());
    }
    {
        const auto f = conflicts::Find(With({{"dxgi.dll", false}, {"ReShade.ini", false}, {"reshade-shaders", true}, {"ReShade.log", false}}));
        const conflicts::Finding* r = Named(f, "ReShade");
        Report("2", "ReShade: dxgi.dll beside ReShade.ini and its shaders: found, dxgi.dll named, to be renamed while playing in VR",
               f.size() == 1 && r && Has(r->files, "dxgi.dll") && Has(r->files, "ReShade.ini") && Has(r->files, "reshade-shaders (a folder)") &&
                   Has(r->todo, "rename dxgi.dll"),
               "%s; %s", Say(f).c_str(), r ? r->todo.c_str() : "");
    }
    {
        const auto a = conflicts::Find(With({{"ReShade.ini", false}, {"reshade-shaders", true}}));
        const auto b = conflicts::Find(With({{"dxvk.conf", false}}));
        const auto c = conflicts::Find(With({{"widescreen.asi", false}, {"scripts", true}}));
        Report("3", "a program's files without a DLL that loads it (ReShade's, DXVK's, an ASI plugin and its folder): nothing", a.empty() && b.empty() &&
               c.empty(), "%s; %s; %s", Say(a).c_str(), Say(b).c_str(), Say(c).c_str());
    }
    {
        const auto f = conflicts::Find(With({{"d3d11.dll", false}, {"d3dx.ini", false}, {"ShaderFixes", true}, {"dxgi.dll", false}, {"ReShade.ini", false}}));
        const conflicts::Finding* m = Named(f, "3DMigoto");
        const conflicts::Finding* r = Named(f, "ReShade");
        Report("4", "3DMigoto's d3d11.dll and ReShade's dxgi.dll together: two findings, each DLL its own program's",
               f.size() == 2 && m && r && Has(m->files, "d3d11.dll") && !Has(m->files, "dxgi.dll") && Has(r->files, "dxgi.dll") && !Has(r->files, "d3d11.dll"),
               "%s", Say(f).c_str());
    }
    {
        int found = 0;
        std::string said;
        const struct {
            const char* what;
            Entries entries;
            const char* dll;
        } cases[] = {
            {"DXVK", {{"d3d11.dll", false}, {"dxgi.dll", false}, {"dxvk.conf", false}}, "d3d11.dll and dxgi.dll"},
            {"DXVK", {{"d3d11.dll", false}, {"bf3.dxvk-cache", false}}, "d3d11.dll"},
            {"ENB", {{"d3d11.dll", false}, {"enbseries.ini", false}, {"enblocal.ini", false}}, "d3d11.dll"},
            {"Special K", {{"dxgi.dll", false}, {"SpecialK32.dll", false}}, "dxgi.dll"},
            {"SweetFX", {{"dxgi.dll", false}, {"SweetFX_settings.txt", false}, {"injector.ini", false}}, "dxgi.dll"},
            {"An ASI loader's plugins", {{"dinput8.dll", false}, {"widescreen.asi", false}}, "dinput8.dll"},
            {"An ASI loader's plugins", {{"version.dll", false}, {"scripts", true}}, "version.dll"},
            {"x360ce", {{"xinput1_3.dll", false}, {"x360ce.ini", false}}, "xinput1_3.dll"},
        };
        for (const auto& c : cases) {
            const auto f = conflicts::Find(With(c.entries));
            const conflicts::Finding* x = Named(f, c.what);
            const bool ok = f.size() == 1 && x && Has(x->todo, (std::string("rename ") + c.dll).c_str());
            found += ok;
            if (!ok)
                said += std::string(" ") + c.what + ": " + Say(f);
        }
        Report("5", "DXVK (twice), ENB, Special K, SweetFX, an ASI loader (twice), x360ce: each found alone, its DLLs to be renamed", found == 8,
               "%d of 8%s", found, said.c_str());
    }
    {
        const auto f = conflicts::Find(With({{"dinput8.dll", false}, {"winmm.dll", false}}));
        const conflicts::Finding* o = Named(f, "Another program's DLL");
        Report("6", "any other proxy DLL (dinput8.dll, winmm.dll alone): another program's DLL, both in one finding", f.size() == 1 && o &&
               Has(o->files, "dinput8.dll") && Has(o->files, "winmm.dll") && Has(o->todo, "rename dinput8.dll and winmm.dll"), "%s", Say(f).c_str());
    }
    {
        const auto upper = conflicts::Find(With({{"DXGI.DLL", false}, {"RESHADE.INI", false}}));
        const auto folder = conflicts::Find(With({{"dxgi.dll", true}}));
        const auto many = conflicts::Find(With({{"d3d11.dll", false}, {"dxgi.dll", false}, {"dxvk.conf", false}, {"ReShade.ini", false},
                                                {"dinput8.dll", false}}));
        int named = 0;
        for (const conflicts::Finding& x : many)
            named += static_cast<int>(Has(x.files, "d3d11.dll")) + static_cast<int>(Has(x.files, "dxgi.dll")) + static_cast<int>(Has(x.files, "dinput8.dll"));
        // (DXVK takes d3d11.dll and dxgi.dll by its dxvk.conf; ReShade.ini is left with dinput8.dll, a name ReShade goes by too.)
        const conflicts::Finding* reshade = Named(many, "ReShade");
        Report("7", "names in any case; a folder named dxgi.dll isn't a DLL; each DLL in one finding only (DXVK's two, ReShade's dinput8.dll apart)",
               upper.size() == 1 && Named(upper, "ReShade") && folder.empty() && named == 3 && many.size() == 2 && Named(many, "DXVK") && reshade &&
                   Has(reshade->files, "dinput8.dll"),
               "%s; %s; %s", Say(upper).c_str(), Say(folder).c_str(), Say(many).c_str());
    }
    {
        // Every program's finding says why and what to do.
        const auto f = conflicts::Find(With({{"d3d11.dll", false}, {"d3dx.ini", false}, {"dxgi.dll", false}, {"ReShade.ini", false}, {"xinput1_3.dll", false},
                                             {"x360ce.ini", false}, {"winmm.dll", false}}));
        int worded = 0;
        for (const conflicts::Finding& x : f)
            worded += !x.files.empty() && Has(x.todo, "rename") && Has(x.todo, "in the game's folder") && x.todo.size() > 60;
        Report("8", "the words: each finding names its files, says why, and what to do", f.size() == 4 && worded == 4, "%d of %d worded: %s", worded,
               static_cast<int>(f.size()), f.empty() ? "" : f[0].todo.c_str());
    }
    {
        // The folder given, else the game where Steam and the EA app install it by default.
        std::string folder = argc > 1 ? argv[1] : "";
        if (folder.empty())
            for (const char* d : {"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Battlefield 3", "C:\\Program Files\\EA Games\\Battlefield 3",
                                  "C:\\Program Files (x86)\\Origin Games\\Battlefield 3"})
                if (GetFileAttributesA((std::string(d) + "\\bf3.exe").c_str()) != INVALID_FILE_ATTRIBUTES) {
                    folder = d;
                    break;
                }
        Entries entries;
        WIN32_FIND_DATAA d{};
        HANDLE find = FindFirstFileA((folder + "\\*").c_str(), &d);
        if (find != INVALID_HANDLE_VALUE) {
            do {
                if (std::string(d.cFileName) != "." && std::string(d.cFileName) != "..")
                    entries.push_back({d.cFileName, (d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0});
            } while (FindNextFileA(find, &d));
            FindClose(find);
        }
        if (folder.empty() || entries.empty() || GetFileAttributesA((folder + "\\bf3.exe").c_str()) == INVALID_FILE_ATTRIBUTES) {
            Skip("9", "no Battlefield 3 folder where Steam or the EA app put it by default (give its folder as the argument)");
        } else {
            const auto f = conflicts::Find(entries);
            Report("9", "this PC's own game folder (its names only, read only): what the launch report would say", true, "%zu entries: %s", entries.size(),
                   Say(f).c_str());
        }
    }
    std::printf("\n%d checks, %d failed, %d skipped\n", g_counted, g_failed, g_skipped);
    return g_failed;
}

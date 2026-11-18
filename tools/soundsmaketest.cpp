// The mod's own making of its sounds at its start (src/sounds.cpp's MakeMissing, as dllmain.cpp runs it), without the game and without a
// sound played: a copy of this program and of gamesounds.exe (beside this program, or "--gamesounds <path>") in a folder of its own under
// the temp folder, so the "mod's folder" is that one and its sounds folder holds stand-ins.
//   1  every clip there: one line saying so, at once; gamesounds.exe only asked for its list, no clip touched
//   2  an update of the mod in place (two of this version's clips not there): a line naming the two and that only those are made, the
//      rest left as they are; with no game in "the game's folder" the making fails: a problem line naming the two, every clip that
//      was there left as it was
//   3  no clip at all (a first start without the launcher): the line that all are being made, then the problem line
//   4  gamesounds.exe not there: with clips a line (not checked for new ones), with none the problem line
//   5  the start never waits: MakeMissing is back in under 50 ms each time
//   6  (only with "--game <Battlefield 3's folder>") the real thing: two clips gone are made from the game's own files, the rest left as
//      they were, and how long it took
// Usage: soundsmaketest [--gamesounds <path>] [--game <folder>]
// The exit code is the number of failed checks.
#include <windows.h>

#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "../src/sounds.h"

namespace logx {
void Write(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::printf("LOG: ");
    std::vprintf(fmt, args);
    va_end(args);
    std::printf("\n");
}
}

namespace {

int g_failed = 0, g_counted = 0;

void Report(const char* id, const char* what, bool pass, const char* format, ...) {
    char numbers[4096];
    va_list args;
    va_start(args, format);
    std::vsnprintf(numbers, sizeof(numbers), format, args);
    va_end(args);
    std::printf("%-4s %-5s %s | %s\n", id, pass ? "PASS" : "FAIL", what, numbers);
    ++g_counted;
    g_failed += !pass;
}

std::wstring SelfPath() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return path;
}

std::string Narrow(const std::wstring& w) {
    std::string s;
    for (wchar_t c : w)
        s += c < 128 ? static_cast<char>(c) : '?';
    return s;
}

std::string OneLine(std::string s) {
    for (char& c : s)
        if (c == '\r' || c == '\n')
            c = ' ';
    return s.size() > 900 ? s.substr(0, 900) + "..." : s;
}

struct Ran {
    DWORD code = 0xFFFF;
    std::string out;
};

// A program run hidden, its words read, ended after `seconds`.
Ran Run(const std::wstring& exe, const std::wstring& line, const std::wstring& folder, unsigned seconds) {
    Ran r;
    SECURITY_ATTRIBUTES inherit{sizeof(inherit), nullptr, TRUE};
    HANDLE readEnd = nullptr, writeEnd = nullptr;
    if (!CreatePipe(&readEnd, &writeEnd, &inherit, 1 << 16))
        return r;
    SetHandleInformation(readEnd, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOW si{sizeof(si)};
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = si.hStdError = writeEnd;
    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> buffer(line.begin(), line.end());
    buffer.push_back(0);
    const bool ok = CreateProcessW(exe.c_str(), buffer.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, folder.c_str(), &si, &pi) != 0;
    CloseHandle(writeEnd);
    if (!ok) {
        CloseHandle(readEnd);
        return r;
    }
    const ULONGLONG until = GetTickCount64() + seconds * 1000ull;
    auto drain = [&] {
        DWORD avail = 0;
        while (PeekNamedPipe(readEnd, nullptr, 0, nullptr, &avail, nullptr) && avail) {
            char chunk[4096];
            DWORD got = 0;
            if (!ReadFile(readEnd, chunk, std::min<DWORD>(avail, sizeof(chunk)), &got, nullptr) || !got)
                break;
            r.out.append(chunk, got);
        }
    };
    for (;;) {
        drain();
        if (WaitForSingleObject(pi.hProcess, 50) == WAIT_OBJECT_0 || GetTickCount64() > until)
            break;
    }
    if (WaitForSingleObject(pi.hProcess, 0) != WAIT_OBJECT_0) {
        TerminateProcess(pi.hProcess, 0xDEAD);
        WaitForSingleObject(pi.hProcess, 5000);
    }
    drain();
    GetExitCodeProcess(pi.hProcess, &r.code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(readEnd);
    return r;
}

// ---- the copy's side: MakeMissing as the mod's start runs it, its lines printed, until the last one came ----
std::mutex g_lock;
bool g_done = false;

void Said(const char* kind, const char* text) {
    std::lock_guard lock(g_lock);
    std::printf("%s: %s\n", kind, text);
    std::fflush(stdout);
    // (The lines that something is being made now are followed by another; every other line is the last.)
    if (!std::strstr(text, " now from your game's own files"))
        g_done = true;
}

int Child(const wchar_t* game, int seconds) {
    const auto t0 = std::chrono::steady_clock::now();
    sounds::MakeMissing([](const char* text) { Said("LINE", text); }, [](const char* text) { Said("WARN", text); }, game);
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    for (int i = 0; i < seconds * 20; ++i, Sleep(50)) {
        std::lock_guard lock(g_lock);
        if (g_done)
            break;
    }
    std::lock_guard lock(g_lock);
    std::printf("BACK: %.1f ms\n%s\n", ms, g_done ? "DONE" : "NOT DONE");
    return g_done ? 0 : 1;
}

struct Stamp {
    FILETIME written{};
    DWORD size = 0;
    bool there = false;
    bool operator==(const Stamp& o) const { return there == o.there && size == o.size && CompareFileTime(&written, &o.written) == 0; }
};

int Count(const std::string& text, const char* what) {
    int n = 0;
    for (size_t at = text.find(what); at != std::string::npos; at = text.find(what, at + 1))
        ++n;
    return n;
}

double BackMs(const std::string& out) {
    const size_t at = out.find("BACK: ");
    return at == std::string::npos ? 1e9 : std::atof(out.c_str() + at + 6);
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc > 2 && wcscmp(argv[1], L"--child") == 0)
        return Child(argv[2], argc > 3 ? _wtoi(argv[3]) : 60);
    const std::wstring self = SelfPath();
    std::wstring cutter = self.substr(0, self.find_last_of(L'\\') + 1) + L"gamesounds.exe", realGame;
    for (int i = 1; i + 1 < argc; ++i) {
        if (wcscmp(argv[i], L"--gamesounds") == 0)
            cutter = argv[i + 1];
        else if (wcscmp(argv[i], L"--game") == 0)
            realGame = argv[i + 1];
    }
    if (GetFileAttributesW(cutter.c_str()) == INVALID_FILE_ATTRIBUTES) {
        Report("0", "gamesounds.exe to test with", false, "%s isn't there (build it, or give --gamesounds <path>)", Narrow(cutter).c_str());
        std::printf("%d of %d checks failed\n", g_failed, g_counted);
        return g_failed;
    }
    wchar_t temp[MAX_PATH];
    GetTempPathW(MAX_PATH, temp);
    const std::wstring folder = std::wstring(temp) + L"soundsmaketest_" + std::to_wstring(GetCurrentProcessId());
    const std::wstring sounds = folder + L"\\sounds", copy = folder + L"\\soundsmaketest.exe", cut = folder + L"\\gamesounds.exe";
    CreateDirectoryW(folder.c_str(), nullptr);
    CreateDirectoryW(sounds.c_str(), nullptr);
    const bool copied = CopyFileW(self.c_str(), copy.c_str(), FALSE) && CopyFileW(cutter.c_str(), cut.c_str(), FALSE);
    const Ran list = Run(cut, L"\"" + cut + L"\" --list", folder, 20);
    std::vector<std::string> names;
    {
        std::stringstream ss(list.out);
        for (std::string line; std::getline(ss, line);) {
            while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
                line.pop_back();
            if (!line.empty())
                names.push_back(line);
        }
    }
    Report("0", "a copy of this program and gamesounds.exe in a folder of their own, its list", copied && list.code == 0 && names.size() >= 100,
           "copied %d, --list exit %lu, %zu names", copied, list.code, names.size());
    const int all = static_cast<int>(names.size());
    auto path = [&](const std::string& name) { return sounds + L"\\" + std::wstring(name.begin(), name.end()) + L".wav"; };
    auto write = [&](const std::string& name) {
        std::ofstream f(path(name), std::ios::binary);
        const std::string body = "RIFF....WAVEfmt a stand-in for " + name + " ";
        for (size_t i = 0; i < 200; ++i)
            f.put(body[i % body.size()]);
    };
    auto stamp = [&](const std::string& name) {
        Stamp s;
        WIN32_FILE_ATTRIBUTE_DATA info;
        if (GetFileAttributesExW(path(name).c_str(), GetFileExInfoStandard, &info)) {
            s.there = true;
            s.size = info.nFileSizeLow;
            s.written = info.ftLastWriteTime;
        }
        return s;
    };
    auto stamps = [&] {
        std::vector<Stamp> v;
        for (const std::string& n : names)
            v.push_back(stamp(n));
        return v;
    };
    auto same = [&](const std::vector<Stamp>& before, size_t skipA, size_t skipB) {
        int n = 0;
        for (size_t i = 0; i < names.size(); ++i)
            n += i != skipA && i != skipB && stamp(names[i]) == before[i];
        return n;
    };
    auto child = [&](const std::wstring& game, int seconds) {
        return Run(copy, L"\"" + copy + L"\" --child \"" + game + L"\" " + std::to_wstring(seconds), folder, seconds + 10);
    };
    auto logThere = [&] { return GetFileAttributesW((folder + L"\\gamesounds.log").c_str()) != INVALID_FILE_ATTRIBUTES; };
    double slowest = 0;
    if (copied && all >= 100) {
        const size_t none = names.size(), a = names.size() / 3, b = names.size() - 2;
        const std::string allText = "all " + std::to_string(all) + " in build\\sounds";
        const std::string two = "2 of " + std::to_string(all) + " aren't in build\\sounds yet";
        const std::string pair = names[a] + ", " + names[b];

        // 1: every clip there.
        for (const std::string& n : names)
            write(n);
        std::vector<Stamp> before = stamps();
        Sleep(50);
        Ran r = child(folder, 30);
        slowest = std::max(slowest, BackMs(r.out));
        int kept = same(before, none, none);
        Report("1", "every clip there: one line saying so, nothing made, no clip touched",
               r.code == 0 && Count(r.out, "LINE: ") == 1 && Count(r.out, "WARN: ") == 0 && r.out.find(allText) != std::string::npos && kept == all && !logThere(),
               "exit %lu, %d of %d clips as they were, gamesounds.log written %d: %s", r.code, kept, all, logThere(), OneLine(r.out).c_str());

        // 2: an update in place: two of this version's clips aren't there. No game in "the game's folder": the making fails.
        DeleteFileW(path(names[a]).c_str());
        DeleteFileW(path(names[b]).c_str());
        r = child(folder, 60);
        slowest = std::max(slowest, BackMs(r.out));
        kept = same(before, a, b);
        const bool told = r.out.find("LINE: the mod's sounds: " + two) != std::string::npos && r.out.find(pair) != std::string::npos &&
                          r.out.find("making only those now from your game's own files") != std::string::npos &&
                          r.out.find("the " + std::to_string(all - 2) + " there are left as they are") != std::string::npos;
        const bool warned = r.out.find("WARN: 2 of the mod's sounds couldn't be made (" + pair) != std::string::npos &&
                            r.out.find("ended with 3") != std::string::npos;
        Report("2", "two of this version's clips not there: only those made, the rest left; no game: the problem line names them",
               r.code == 0 && told && warned && kept == all - 2 && logThere() && !stamp(names[a]).there && !stamp(names[b]).there,
               "exit %lu, told %d, warned %d, %d of %d other clips as they were, gamesounds.log written %d: %s", r.code, told, warned, kept, all - 2,
               logThere(), OneLine(r.out).c_str());

        // 6: the real thing, with the game's own files (only when asked: it reads them, some seconds).
        if (!realGame.empty()) {
            const auto t0 = std::chrono::steady_clock::now();
            r = child(realGame, 300);
            const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
            kept = same(before, a, b);
            const bool made = r.out.find("LINE: the mod's sounds: 2 made from your game's own files") != std::string::npos &&
                              r.out.find(std::to_string(all) + " of " + std::to_string(all) + " in build\\sounds now") != std::string::npos;
            Report("6", "the real thing: the two made from the game's own files, the rest left as they were",
                   r.code == 0 && made && Count(r.out, "WARN: ") == 0 && kept == all - 2 && stamp(names[a]).size > 1000 && stamp(names[b]).size > 1000,
                   "exit %lu in %.1f s, made %d, %d of %d other clips as they were, the two's sizes %lu and %lu bytes: %s", r.code, s, made, kept, all - 2,
                   stamp(names[a]).size, stamp(names[b]).size, OneLine(r.out).c_str());
        }

        // 3: no clip at all.
        for (const std::string& n : names)
            DeleteFileW(path(n).c_str());
        r = child(folder, 60);
        slowest = std::max(slowest, BackMs(r.out));
        Report("3", "no clip at all: all are made, said so; no game: the problem line",
               r.code == 0 && r.out.find("LINE: the mod's sounds weren't made yet") != std::string::npos &&
                   r.out.find("WARN: " + std::to_string(all) + " of the mod's sounds couldn't be made") != std::string::npos,
               "exit %lu: %s", r.code, OneLine(r.out).c_str());

        // 4: gamesounds.exe not there.
        DeleteFileW(cut.c_str());
        r = child(folder, 30);
        slowest = std::max(slowest, BackMs(r.out));
        const bool noneWarn = r.code == 0 && r.out.find("WARN: the mod's sounds aren't there (build\\sounds is empty) and build\\gamesounds.exe") != std::string::npos;
        write(names[0]);
        write(names[1]);
        const Ran q = child(folder, 30);
        slowest = std::max(slowest, BackMs(q.out));
        const bool someLine = q.code == 0 && q.out.find("LINE: the mod's sounds: 2 in build\\sounds") != std::string::npos && Count(q.out, "WARN: ") == 0;
        Report("4", "gamesounds.exe not there: with no clips the problem line, with clips a line", noneWarn && someLine, "%s || %s", OneLine(r.out).c_str(),
               OneLine(q.out).c_str());

        Report("5", "the start never waits: MakeMissing back at once", slowest < 50.0, "%.1f ms at the most", slowest);
    }
    for (const std::string& n : names)
        DeleteFileW(path(n).c_str());
    DeleteFileW(copy.c_str());
    DeleteFileW(cut.c_str());
    DeleteFileW((folder + L"\\gamesounds.log").c_str());
    RemoveDirectoryW(sounds.c_str());
    RemoveDirectoryW(folder.c_str());
    std::printf("%d of %d checks failed\n", g_failed, g_counted);
    return g_failed;
}

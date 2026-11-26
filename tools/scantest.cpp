// The scans of the game's memory for a gun's data (src/scanthread.h, as skin.cpp's ClearRifleSpread, FindStinger, ClearLauncherSpread and FindAt4 run
// it), without the game. Each check prints PASS or FAIL, then the totals; the exit code is the number failed.
//   s1  ScanThread on a thread of its own: in background mode from its first line (Windows says so: another BEGIN refused as already in it); its own
//       priorities again at Foreground, a second Foreground nothing; StartScan's count one less at its end; with no Foreground, its own priorities
//       again at its end all the same
//   s2  skin.cpp's four scans (the review of d32a39b, finding 6; run from the project's folder: src\skin.cpp): each one's walk of the game's memory
//       followed at once by Foreground, so nothing after it (its lines logged, the spread's and the hands' locks, its places added, the rows written)
//       runs in background mode; nothing of that inside a walk (the launcher's rocket starts said after its walk); before a walk, a line logged only
//       after Foreground (the rifle's table refused); ScanThread the header's, and these four the only scans with one
// Usage: scantest (from the project's folder)

#include <cstdio>
#include <cstring>
#include <string>
#include <thread>

#include "../src/scanthread.h"

namespace {

int g_failed = 0, g_counted = 0;

void Check(bool ok, const char* what) {
    ++g_counted;
    if (!ok)
        ++g_failed;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
}

// Whether this thread is in background mode, as Windows has it: a BEGIN refused as already in it. One taken (it wasn't) is put back at once.
bool InBackground() {
    if (SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN)) {
        SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_END);
        return false;
    }
    return GetLastError() == ERROR_THREAD_MODE_ALREADY_BACKGROUND;
}

std::string ReadText(const char* path) {
    std::string text;
    FILE* f = nullptr;
    if (fopen_s(&f, path, "rb") != 0 || !f)
        return text;
    char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
        text.append(buf, n);
    fclose(f);
    std::string out;
    out.reserve(text.size());
    for (char c : text)
        if (c != '\r')
            out += c;
    return out;
}

std::string Trimmed(const std::string& s) {
    const size_t a = s.find_first_not_of(" \t");
    if (a == std::string::npos)
        return {};
    return s.substr(a, s.find_last_not_of(" \t") + 1 - a);
}

// What a scan's lines may not do in background mode: log, lock, add its places, write.
const char* const kForegroundOnly[] = {"Log(", "lock_guard", "unique_lock", "AddPlaces(", "AddLauncherShots(", "ApplyRifleSpread(", "WriteProcessMemory("};

struct Scan {
    bool found = false, guardFirst = false, oneWalk = false, foregroundAfter = false, walkClean = false, beforeClean = false;
};

Scan Look(const std::string& skin, const std::string& head) {
    Scan s;
    const size_t at = skin.find(head);
    const size_t end = at == std::string::npos ? std::string::npos : skin.find("\n}\n", at);
    if (end == std::string::npos)
        return s;
    s.found = true;
    const std::string body = skin.substr(at, end + 3 - at);
    const std::string guard = "\n    ScanThread scanning(g_scansRunning);";
    s.guardFirst = body.compare(head.size(), guard.size(), guard) == 0;
    const std::string walkHead = "\n    for (uintptr_t at = 0x10000; VirtualQuery(reinterpret_cast<LPCVOID>(at), &info, sizeof(info)) == sizeof(info);) {\n";
    const std::string walkEnd = "\n        if (end <= at)\n            break;\n        at = end;\n    }\n";
    const size_t walk = body.find(walkHead);
    s.oneWalk = walk != std::string::npos && body.find(walkHead, walk + 1) == std::string::npos;
    const size_t walked = walk == std::string::npos ? std::string::npos : body.find(walkEnd, walk);
    if (!s.oneWalk || walked == std::string::npos)
        return s;
    const size_t after = walked + walkEnd.size();
    s.foregroundAfter = body.compare(after, std::strlen("    scanning.Foreground();"), "    scanning.Foreground();") == 0;
    const std::string inWalk = body.substr(walk, after - walk);
    s.walkClean = true;
    for (const char* word : kForegroundOnly)
        s.walkClean = s.walkClean && inWalk.find(word) == std::string::npos;
    // Before the walk: a line logged only right after Foreground (the scan given up before it walks); nothing locked, added or written.
    const size_t from = head.size() + guard.size();
    const std::string before = body.substr(from, walk - from);
    s.beforeClean = true;
    for (const char* word : kForegroundOnly) {
        for (size_t p = before.find(word); p != std::string::npos; p = before.find(word, p + 1)) {
            if (std::strcmp(word, "Log(") != 0) {
                s.beforeClean = false;
                continue;
            }
            const size_t lineAt = before.rfind('\n', p);
            const size_t prevAt = lineAt == std::string::npos || lineAt == 0 ? std::string::npos : before.rfind('\n', lineAt - 1);
            const std::string prev = prevAt == std::string::npos ? std::string() : before.substr(prevAt + 1, lineAt - prevAt - 1);
            s.beforeClean = s.beforeClean && Trimmed(prev) == "scanning.Foreground();";
        }
    }
    return s;
}

size_t Count(const std::string& text, const std::string& what) {
    size_t n = 0;
    for (size_t p = text.find(what); p != std::string::npos; p = text.find(what, p + 1))
        ++n;
    return n;
}

}  // namespace

int main() {
    // ---- s1: the guard itself, on a thread of its own as StartScan makes one ----
    {
        std::atomic<int> running{1};  // (StartScan's count: one added before its thread starts)
        bool before = true, during = false, says = false, afterForeground = true, saysAfter = true, again = true;
        std::thread([&] {
            before = InBackground();
            {
                ScanThread scanning(running);
                during = InBackground();
                says = scanning.Background();
                scanning.Foreground();
                afterForeground = InBackground();
                saysAfter = scanning.Background();
                scanning.Foreground();
                again = InBackground();
            }
        }).join();
        char what[512];
        std::snprintf(what, sizeof(what), "s1 a scan's thread: not in background mode before its guard %d, in it from its first line %d (the guard says %d), its own "
                                          "priorities again at Foreground %d (says %d), a second Foreground nothing %d; StartScan's count %d at its end",
                      !before, during, says, !afterForeground, !saysAfter, !again, running.load());
        Check(!before && during && says && !afterForeground && !saysAfter && !again && running.load() == 0, what);
        std::atomic<int> left{1};
        bool inIt = false, afterEnd = true;
        std::thread([&] {
            {
                ScanThread scanning(left);
                inIt = InBackground();
            }
            afterEnd = InBackground();
        }).join();
        std::snprintf(what, sizeof(what), "s1 a scan ended with no Foreground (its guard's end alone): in background mode %d, its own priorities again after %d, the count %d",
                      inIt, !afterEnd, left.load());
        Check(inIt && !afterEnd && left.load() == 0, what);
    }

    // ---- s2: skin.cpp's four scans ----
    const std::string skin = ReadText("src\\skin.cpp");
    if (skin.empty()) {
        Check(false, "s2 skin.cpp's scans (src\\skin.cpp not read: run from the project's folder)");
    } else {
        const char* const heads[][2] = {{"DWORD WINAPI ClearRifleSpread(LPVOID which) {", "ClearRifleSpread (a rifle's scatter)"},
                                        {"DWORD WINAPI FindStinger(LPVOID) {", "FindStinger (the Stinger's start and lock)"},
                                        {"DWORD WINAPI ClearLauncherSpread(LPVOID) {", "ClearLauncherSpread (the SMAW's start and the launchers' spread)"},
                                        {"DWORD WINAPI FindAt4(LPVOID) {", "FindAt4 (the AT4's start and scatter)"}};
        for (const auto& h : heads) {
            const Scan s = Look(skin, h[0]);
            char what[640];
            std::snprintf(what, sizeof(what), "s2 %s: found %d; its first line the guard %d; one walk of the game's memory %d, followed at once by Foreground %d; "
                                              "nothing logged, locked, added or written inside it %d; before it a line logged only after Foreground, nothing "
                                              "else %d",
                          h[1], s.found, s.guardFirst, s.oneWalk, s.foregroundAfter, s.walkClean, s.beforeClean);
            Check(s.found && s.guardFirst && s.oneWalk && s.foregroundAfter && s.walkClean && s.beforeClean, what);
        }
        const size_t guards = Count(skin, "ScanThread scanning(");
        const bool header = skin.find("#include \"scanthread.h\"") != std::string::npos && skin.find("struct ScanThread {") == std::string::npos &&
                            skin.find("const ScanThread scanning;") == std::string::npos;
        char what[320];
        std::snprintf(what, sizeof(what), "s2 skin.cpp's ScanThread the header's (scanthread.h, no struct of its own, no guard without its count) %d; %zu scans with one, "
                                          "these four",
                      header, guards);
        Check(header && guards == std::size(heads), what);
    }
    std::printf("%d checks, %d failed\n", g_counted, g_failed);
    return g_failed;
}

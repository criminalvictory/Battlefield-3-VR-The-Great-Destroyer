// Waits for the game process and loads bf3vr.dll (from this exe's folder) into it, then stays until the game has gone: the
// mod holds keys and buttons down as the controllers do (a real crouch holds Left Ctrl), and a game that crashes or is killed
// with one held never lets it go: the desktop is left with Ctrl down.
// Only into the single-player campaign: each bf3.exe that starts is judged by its command line first (src/campaign.h), and
// one started for multiplayer or co-op, or one whose line can't be read, gets nothing; the injector says so and waits on for
// a campaign start. What it says also goes to injector.log beside it (the launcher shows it).
// Usage: injector.exe [process name]   (default: bf3.exe)
//        injector.exe --judge <pid>     only says whether that process is the campaign (exit 0 yes, 3 no, 1 can't tell); loads nothing

#include <windows.h>
#include <tlhelp32.h>

#include <share.h>

#include <cstdarg>
#include <cstdio>
#include <cwchar>
#include <map>
#include <set>
#include <string>

#include "../src/campaign.h"
#include "cmdline.h"

namespace {

FILE* g_log = nullptr;

std::wstring BesideMe(const wchar_t* name) {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring p(path);
    return p.substr(0, p.find_last_of(L"\\/") + 1) + name;
}

// To the window and to injector.log, each line timed in the log, both flushed at once (the launcher reads the log as it grows).
void Say(const char* fmt, ...) {
    char text[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);
    printf("%s\n", text);
    fflush(stdout);
    if (g_log) {
        SYSTEMTIME t;
        GetLocalTime(&t);
        fprintf(g_log, "[%02d:%02d:%02d] %s\n", t.wHour, t.wMinute, t.wSecond, text);
        fflush(g_log);
    }
}

std::string Narrow(const std::wstring& w) {
    std::string s;
    for (wchar_t c : w)
        s += c < 128 ? static_cast<char>(c) : '?';
    return s;
}

// Processes of that name already looked at and left alone (refused, or closed before they could be looked at), each with a
// handle held: while it's held, its pid can't be given to a new process. Let go of once the process is no longer listed.
std::map<DWORD, HANDLE> g_passed;

// The first process of that name not passed over already; the passed-over ones no longer listed are forgotten.
DWORD FindProcess(const std::wstring& name) {
    DWORD pid = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return 0;
    std::set<DWORD> listed;
    PROCESSENTRY32W entry{sizeof(entry)};
    for (BOOL ok = Process32FirstW(snap, &entry); ok; ok = Process32NextW(snap, &entry)) {
        if (_wcsicmp(entry.szExeFile, name.c_str()) != 0)
            continue;
        listed.insert(entry.th32ProcessID);
        if (!pid && !g_passed.count(entry.th32ProcessID))
            pid = entry.th32ProcessID;
    }
    CloseHandle(snap);
    for (auto it = g_passed.begin(); it != g_passed.end();) {
        if (listed.count(it->first)) {
            ++it;
            continue;
        }
        if (it->second)
            CloseHandle(it->second);
        it = g_passed.erase(it);
    }
    return pid;
}

void KeyUp(WORD scan) {
    INPUT in{};
    in.type = INPUT_KEYBOARD;
    in.ki.wScan = scan;
    in.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    SendInput(1, &in, sizeof(in));
}

// All that the mod's input.cpp may hold: W, A, S, D, Space, Left Ctrl, R, E, Left Shift, G, and the mouse's two buttons.
void LetGo() {
    constexpr WORD kCtrl = 0x1D;
    for (WORD scan : {WORD{0x11}, WORD{0x1E}, WORD{0x1F}, WORD{0x20}, WORD{0x39}, kCtrl, WORD{0x13}, WORD{0x12}, WORD{0x2A}, WORD{0x22}})
        KeyUp(scan);
    for (DWORD up : {DWORD{MOUSEEVENTF_LEFTUP}, DWORD{MOUSEEVENTF_RIGHTUP}}) {
        INPUT in{};
        in.type = INPUT_MOUSE;
        in.mi.dwFlags = up;
        SendInput(1, &in, sizeof(in));
    }
    // A key-up can't go in while an elevated window (Task Manager) is in front: tried again for a minute.
    for (int i = 0; i < 240 && (GetAsyncKeyState(VK_LCONTROL) & 0x8000); ++i) {
        Sleep(250);
        KeyUp(kCtrl);
    }
}

// The verdict on a running process's command line. Never prints the line: it carries the account's codes.
campaign::Judgement JudgeProcess(HANDLE process, const char** how, DWORD& error) {
    std::wstring line;
    if (!cmdline::ReadCommandLine(process, line, error, how))
        return campaign::Judge(nullptr);
    return campaign::Judge(line.c_str());
}

// --judge <pid>: the verdict alone, nothing loaded.
int JudgeOnly(const wchar_t* pidText) {
    const DWORD pid = wcstoul(pidText, nullptr, 10);
    HANDLE process = pid ? OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, pid) : nullptr;
    if (!process) {
        Say("Process %lu can't be opened (%lu).", pid, pid ? GetLastError() : 0ul);
        return 1;
    }
    const char* how = "";
    DWORD error = 0;
    const campaign::Judgement j = JudgeProcess(process, &how, error);
    CloseHandle(process);
    if (j.verdict == campaign::Verdict::kNoLine) {
        Say("Process %lu: its command line couldn't be read (%lu).", pid, error);
        return 1;
    }
    Say("Process %lu: %s (%s; read from its %s).", pid, campaign::Words(j.verdict), j.detail.c_str(), how);
    return j.verdict == campaign::Verdict::kCampaign ? 0 : 3;
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc > 2 && wcscmp(argv[1], L"--judge") == 0)
        return JudgeOnly(argv[2]);

    std::wstring target = argc > 1 ? argv[1] : L"bf3.exe";
    std::wstring dll = BesideMe(L"bf3vr.dll");
    g_log = _wfsopen(BesideMe(L"injector.log").c_str(), L"w", _SH_DENYNO);
    if (GetFileAttributesW(dll.c_str()) == INVALID_FILE_ATTRIBUTES) {
        Say("Can't find %s", Narrow(dll).c_str());
        return 1;
    }

    Say("Waiting for %s... start the game now (Battlelog's PLAY CAMPAIGN).", Narrow(target).c_str());
    HANDLE process = nullptr;
    DWORD pid = 0;
    for (;;) {
        while (!(pid = FindProcess(target)))
            Sleep(500);
        Say("Found %s (pid %lu)", Narrow(target).c_str(), pid);
        // Give the game a moment to finish starting before loading into it.
        Sleep(3000);

        process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE |
                                  PROCESS_VM_READ | SYNCHRONIZE,
                              FALSE, pid);
        if (!process) {
            const DWORD error = GetLastError();
            if (error == ERROR_INVALID_PARAMETER) {  // gone already: a start that handed over to another (the EA app's)
                Say("It closed again before the mod could look at it; waiting for the next one.");
                g_passed[pid] = nullptr;
                continue;
            }
            if (error == ERROR_ACCESS_DENIED)
                Say("Battlefield 3 is running as administrator, so the mod can't load into it (error 5). Close the game, then "
                    "either run the launcher as administrator too, or stop the game (and the EA app) from running as administrator.");
            else
                Say("OpenProcess failed (%lu). Try running as administrator.", error);
            return 1;
        }
        if (WaitForSingleObject(process, 0) == WAIT_OBJECT_0) {
            Say("It closed again before the mod could look at it; waiting for the next one.");
            g_passed[pid] = process;
            process = nullptr;
            continue;
        }

        BOOL selfWow = FALSE, targetWow = FALSE;
        IsWow64Process(GetCurrentProcess(), &selfWow);
        IsWow64Process(process, &targetWow);
        if (selfWow != targetWow) {
            Say("Bitness mismatch: the game is %s but this injector is %s. Use the matching build.", targetWow ? "32-bit" : "64-bit",
                selfWow ? "32-bit" : "64-bit");
            CloseHandle(process);
            return 1;
        }

        // The campaign only: its command line first.
        const char* how = "";
        DWORD readError = 0;
        const campaign::Judgement verdict = JudgeProcess(process, &how, readError);
        if (verdict.verdict == campaign::Verdict::kCampaign) {
            Say("Campaign start (%s; read from its %s): loading the mod.", verdict.detail.c_str(), how);
            break;
        }
        if (verdict.verdict == campaign::Verdict::kNoLine)
            Say("REFUSED: how this Battlefield 3 was started couldn't be read (error %lu), so the mod stays out of it. The mod loads "
                "only into the single-player campaign. Close the game and start it again with PLAY CAMPAIGN in Battlelog.",
                readError);
        else
            Say("REFUSED: %s (%s), so the mod stays out of it: it loads only into the single-player campaign. Never use it "
                "online. To play in VR, close the game and start it with PLAY CAMPAIGN in Battlelog.",
                campaign::Words(verdict.verdict), verdict.detail.c_str());
        g_passed[pid] = process;  // never looked at again while it runs
        process = nullptr;
        Say("Waiting for a campaign start...");
    }

    size_t bytes = (dll.size() + 1) * sizeof(wchar_t);
    void* remote = VirtualAllocEx(process, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote || !WriteProcessMemory(process, remote, dll.c_str(), bytes, nullptr)) {
        Say("Writing into the game failed (%lu)", GetLastError());
        CloseHandle(process);
        return 1;
    }

    // kernel32 is mapped at the same address in every process of the same bitness.
    auto loadLibrary = reinterpret_cast<LPTHREAD_START_ROUTINE>(
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"));
    HANDLE thread = CreateRemoteThread(process, nullptr, 0, loadLibrary, remote, 0, nullptr);
    if (!thread) {
        Say("CreateRemoteThread failed (%lu)", GetLastError());
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        CloseHandle(process);
        return 1;
    }
    WaitForSingleObject(thread, INFINITE);
    DWORD result = 0;
    GetExitCodeThread(thread, &result);
    CloseHandle(thread);
    VirtualFreeEx(process, remote, 0, MEM_RELEASE);

    if (!result) {
        Say("LoadLibrary failed inside the game (see bf3vr.log next to the DLL)");
        CloseHandle(process);
        return 1;
    }
    Say("Loaded. See bf3vr.log next to the DLL.");
    WaitForSingleObject(process, INFINITE);
    CloseHandle(process);
    LetGo();
    Say("The game has closed.");
    return 0;
}

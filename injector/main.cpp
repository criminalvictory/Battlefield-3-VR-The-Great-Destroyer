// Waits for the game process and loads bf3vr.dll (from this exe's folder) into it, then stays until the game has gone: the
// mod holds keys and buttons down as the controllers do (a real crouch holds Left Ctrl), and a game that crashes or is killed
// with one held never lets it go: the desktop is left with Ctrl down.
// Usage: injector.exe [process name]   (default: bf3.exe)

#include <windows.h>
#include <tlhelp32.h>

#include <cstdio>
#include <string>

namespace {

DWORD FindProcess(const std::wstring& name) {
    DWORD pid = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return 0;
    PROCESSENTRY32W entry{sizeof(entry)};
    for (BOOL ok = Process32FirstW(snap, &entry); ok; ok = Process32NextW(snap, &entry)) {
        if (_wcsicmp(entry.szExeFile, name.c_str()) == 0) {
            pid = entry.th32ProcessID;
            break;
        }
    }
    CloseHandle(snap);
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

std::wstring DllPath() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring p(path);
    return p.substr(0, p.find_last_of(L"\\/") + 1) + L"bf3vr.dll";
}

}

int wmain(int argc, wchar_t** argv) {
    std::wstring target = argc > 1 ? argv[1] : L"bf3.exe";
    std::wstring dll = DllPath();
    if (GetFileAttributesW(dll.c_str()) == INVALID_FILE_ATTRIBUTES) {
        wprintf(L"Can't find %ls\n", dll.c_str());
        return 1;
    }

    wprintf(L"Waiting for %ls... start the game now.\n", target.c_str());
    DWORD pid = 0;
    while (!(pid = FindProcess(target)))
        Sleep(500);
    wprintf(L"Found %ls (pid %lu)\n", target.c_str(), pid);
    // Give the game a moment to finish starting before loading into it.
    Sleep(3000);

    HANDLE process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
                                     PROCESS_VM_WRITE | PROCESS_VM_READ | SYNCHRONIZE,
                                 FALSE, pid);
    if (!process) {
        wprintf(L"OpenProcess failed (%lu). Try running as administrator.\n", GetLastError());
        return 1;
    }

    BOOL selfWow = FALSE, targetWow = FALSE;
    IsWow64Process(GetCurrentProcess(), &selfWow);
    IsWow64Process(process, &targetWow);
    if (selfWow != targetWow) {
        wprintf(L"Bitness mismatch: the game is %ls but this injector is %ls. Use the matching build.\n",
                targetWow ? L"32-bit" : L"64-bit", selfWow ? L"32-bit" : L"64-bit");
        CloseHandle(process);
        return 1;
    }

    size_t bytes = (dll.size() + 1) * sizeof(wchar_t);
    void* remote = VirtualAllocEx(process, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote || !WriteProcessMemory(process, remote, dll.c_str(), bytes, nullptr)) {
        wprintf(L"Writing into the game failed (%lu)\n", GetLastError());
        CloseHandle(process);
        return 1;
    }

    // kernel32 is mapped at the same address in every process of the same bitness.
    auto loadLibrary = reinterpret_cast<LPTHREAD_START_ROUTINE>(
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"));
    HANDLE thread = CreateRemoteThread(process, nullptr, 0, loadLibrary, remote, 0, nullptr);
    if (!thread) {
        wprintf(L"CreateRemoteThread failed (%lu)\n", GetLastError());
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
        wprintf(L"LoadLibrary failed inside the game\n");
        CloseHandle(process);
        return 1;
    }
    wprintf(L"Loaded. See bf3vr.log next to the DLL.\n");
    WaitForSingleObject(process, INFINITE);
    CloseHandle(process);
    LetGo();
    return 0;
}

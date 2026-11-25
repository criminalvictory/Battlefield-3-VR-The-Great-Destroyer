// The player's own key bindings from the game's settings file (keybind.h), read on a thread of the mod's own: once as the mod starts, again
// whenever the file changes (the game writes it when its options are changed). Read only: the file is opened for reading, shared with the game.

#include "keybindfile.h"

#include <windows.h>

#include <atomic>
#include <string>
#include <vector>

#include "config.h"
#include "log.h"

namespace {

// The tables made so far: the newest is the one used. Each stays (never freed): a few a run at most, one each time the file changes.
std::atomic<const keybind::Table*> g_current{nullptr};
std::atomic<bool> g_started{false};

const keybind::Table* Defaults() {
    static keybind::Table* t = [] {
        auto* d = new keybind::Table();
        keybind::Fill(*d);
        return d;
    }();
    return t;
}

// Documents\Battlefield 3\settings\PROF_SAVE_profile, as the game finds it (the Documents folder wherever it is: OneDrive's too). Empty when the
// system won't say. shell32 is asked at run time: nothing new in the mod's imports.
std::wstring ProfilePath() {
    using KnownFolderFn = HRESULT(WINAPI*)(REFGUID, DWORD, HANDLE, PWSTR*);
    static const GUID kDocuments = {0xFDD39AD0, 0x238F, 0x46AF, {0xAD, 0xB4, 0x6C, 0x85, 0x48, 0x03, 0x69, 0xC7}};  // FOLDERID_Documents
    HMODULE shell = LoadLibraryW(L"shell32.dll");
    HMODULE ole = LoadLibraryW(L"ole32.dll");
    auto known = shell ? reinterpret_cast<KnownFolderFn>(GetProcAddress(shell, "SHGetKnownFolderPath")) : nullptr;
    auto freeMemory = ole ? reinterpret_cast<void(WINAPI*)(LPVOID)>(GetProcAddress(ole, "CoTaskMemFree")) : nullptr;
    std::wstring path;
    PWSTR folder = nullptr;
    if (known && freeMemory && SUCCEEDED(known(kDocuments, 0, nullptr, &folder)) && folder) {
        path = folder;
        path += L"\\Battlefield 3\\settings\\PROF_SAVE_profile";
    }
    if (folder && freeMemory)
        freeMemory(folder);
    return path;
}

struct Stamp {
    bool there = false;
    FILETIME written{};
    DWORD sizeLow = 0;
    bool operator==(const Stamp& o) const {
        return there == o.there && written.dwLowDateTime == o.written.dwLowDateTime && written.dwHighDateTime == o.written.dwHighDateTime &&
               sizeLow == o.sizeLow;
    }
};

Stamp StampOf(const std::wstring& path) {
    Stamp s;
    WIN32_FILE_ATTRIBUTE_DATA a{};
    if (!path.empty() && GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &a) && !(a.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        s.there = true;
        s.written = a.ftLastWriteTime;
        s.sizeLow = a.nFileSizeLow;
    }
    return s;
}

// The file's text, at most 4 MB (the game's is about 20 KB), shared with the game's own writes. False: not there, or not read.
bool ReadText(const std::wstring& path, std::vector<char>& out) {
    HANDLE f = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE)
        return false;
    LARGE_INTEGER size{};
    bool ok = GetFileSizeEx(f, &size) && size.QuadPart > 0 && size.QuadPart <= (4 << 20);
    if (ok) {
        out.resize(static_cast<size_t>(size.QuadPart));
        DWORD got = 0;
        ok = ReadFile(f, out.data(), static_cast<DWORD>(out.size()), &got, nullptr) && got == out.size();
    }
    CloseHandle(f);
    return ok;
}

// The file read and its table made, said in the log. first: the run's first read.
void Take(const std::wstring& path, bool first) {
    std::vector<char> text;
    const bool read = ReadText(path, text);
    auto* t = new keybind::Table();
    t->bindings = keybind::Parse(read ? text.data() : nullptr, read ? text.size() : 0, read);
    keybind::Fill(*t);
    // A file read just as the game writes it (cut short): its bindings kept as they were, read again at the next look.
    const keybind::Table* was = g_current.load();
    if (read && !t->bindings.lines && was && was->bindings.lines) {
        Log("Keys: the game's settings file has no key bindings in it now (written just now?): the keys as they were; looked at again in 5 s");
        delete t;
        return;
    }
    g_current.store(t);
    char said[1600];
    const int n = keybind::Describe(*t, said, sizeof(said));
    if (!read) {
        Log("Keys: the game's settings file (Documents\\Battlefield 3\\settings\\PROF_SAVE_profile) %s: the mod presses the game's default keys "
            "(crouch X, prone Z, use E, reload R, jump Space)",
            path.empty() ? "couldn't be found (no Documents folder?)" : "isn't there or couldn't be read");
    } else if (!t->bindings.lines) {
        Log("Keys: the game's settings file has no key bindings: the mod presses the game's default keys (crouch X, prone Z, use E, reload R, jump Space)");
    } else if (!n) {
        Log("Keys: your key bindings read from the game's settings (%d lines%s): the mod's keys are the game's defaults (crouch X, prone Z, use E, "
            "reload R, jump Space, sprint Left Shift)",
            t->bindings.lines, first ? "" : ", changed in the game");
    } else {
        Log("Keys: your key bindings read from the game's settings (%d lines%s): the mod presses your keys: %s", t->bindings.lines,
            first ? "" : ", changed in the game", said);
    }
    if (t->bindings.bad)
        Log("Keys: %d of the game's key binding lines couldn't be read (left out)", t->bindings.bad);
}

DWORD WINAPI Watch(LPVOID) {
    const std::wstring path = ProfilePath();
    Stamp seen = StampOf(path);
    Take(path, true);
    for (;;) {
        Sleep(5000);
        const Stamp now = StampOf(path);
        if (now == seen)
            continue;
        seen = now;
        Sleep(500);  // (the game's write finished)
        Take(path, false);
    }
}

}  // namespace

namespace keybind {

void Start() {
    if (g_started.exchange(true))
        return;
    if (!config::Get().followBindings) {
        Log("Keys: the mod presses the game's default keys ([controllers] follow_bindings=0 in bf3vr.ini)");
        return;
    }
    if (HANDLE t = CreateThread(nullptr, 0, Watch, nullptr, 0, nullptr))
        CloseHandle(t);
}

const Table* Current() {
    const Table* t = g_current.load(std::memory_order_acquire);
    return t ? t : Defaults();
}

}  // namespace keybind

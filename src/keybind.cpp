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

// One look at the file (keybind.h's LookAt): its table made and taken when there's something to take, said in the log. last: the look before's
// Kept. Returns this look's: anything but kNo has the watch read the file again at its next look, changed or not.
keybind::Kept Take(const std::wstring& path, keybind::Kept last) {
    static bool s_everRead = false;  // (a read has been taken before: the next one's the file changed, or read again)
    std::vector<char> text;
    const bool read = ReadText(path, text);
    const bool there = read || StampOf(path).there;
    const keybind::Table* was = g_current.load();
    const bool players = was && was->bindings.read && was->bindings.lines > 0;
    auto* t = new keybind::Table();
    t->bindings = keybind::Parse(read ? text.data() : nullptr, read ? text.size() : 0, read);
    keybind::Fill(*t);
    const keybind::Look look = keybind::LookAt(read, there, t->bindings.lines, was != nullptr, players, last);
    if (!look.take) {
        delete t;
        if (look.say && look.kept == keybind::Kept::kNoKeys)
            Log("Keys: the game's settings file has no key bindings in it now (written just now?): your keys as they were; read again in 5 s");
        else if (look.say && look.kept == keybind::Kept::kUnread)
            Log("Keys: the game's settings file couldn't be read just now (the game writing it?): your keys as they were; read again every 5 s until "
                "it reads");
        else if (look.say)
            Log("Keys: the game's settings file isn't there just now: your keys as they were (read again when it's back)");
        return look.kept;
    }
    g_current.store(t);
    const char* again = last != keybind::Kept::kNo ? ", read again" : s_everRead ? ", changed in the game" : "";
    s_everRead = s_everRead || read;
    char said[1600];
    const int n = keybind::Describe(*t, said, sizeof(said));
    if (!read) {
        Log("Keys: the game's settings file (Documents\\Battlefield 3\\settings\\PROF_SAVE_profile) %s: the mod presses the game's default keys "
            "(crouch X, prone Z, use E, reload R, jump Space)",
            path.empty() ? "couldn't be found (no Documents folder?)"
            : there      ? "couldn't be read just now (read again every 5 s until it reads)"
                         : "isn't there");
    } else if (!t->bindings.lines) {
        Log("Keys: the game's settings file has no key bindings%s: the mod presses the game's default keys (crouch X, prone Z, use E, reload R, jump "
            "Space)",
            last == keybind::Kept::kNoKeys ? " (none at the second look either)" : "");
    } else if (!n) {
        Log("Keys: your key bindings read from the game's settings (%d lines%s): the mod's keys are the game's defaults (crouch X, prone Z, use E, "
            "reload R, jump Space, sprint Left Shift)",
            t->bindings.lines, again);
    } else {
        Log("Keys: your key bindings read from the game's settings (%d lines%s): the mod presses your keys: %s", t->bindings.lines, again, said);
    }
    if (t->bindings.bad)
        Log("Keys: %d of the game's key binding lines couldn't be read (left out)", t->bindings.bad);
    return look.kept;
}

DWORD WINAPI Watch(LPVOID) {
    const std::wstring path = ProfilePath();
    Stamp seen = StampOf(path);
    keybind::Kept kept = Take(path, keybind::Kept::kNo);
    for (;;) {
        Sleep(5000);
        const Stamp now = StampOf(path);
        const bool changed = !(now == seen);
        if (!changed && kept == keybind::Kept::kNo)
            continue;
        seen = now;
        if (changed)
            Sleep(500);  // (the game's write finished)
        kept = Take(path, kept);
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

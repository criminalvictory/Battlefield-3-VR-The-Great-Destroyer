// Read only: the running game's list of live input triggers, through the mod's own reader (src/qtelist.h, as src/qtewatch.cpp runs it inside the game),
// from outside (ReadProcessMemory; nothing is ever written): the signature's finds in the game's code, the place the manager's vtable picks, the
// list with each trigger's action and key, the awaited keys in play and in a scripted moment, the game's own input state. For checking the reader
// against the real game without the mod, and for the next mission's quick-time events.
// Usage: qtepeek            the finds, the pick and the list as it is now
//        qtepeek 120        ... then every change of the list and of the game's input for 120 seconds
// The exit code: 0 read, 1 no bf3.exe, 2 the manager not found (no level loaded, or none of the places is it).

#include <windows.h>

#include <psapi.h>
#include <tlhelp32.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "../src/qte.h"
#include "../src/qtelist.h"

namespace {

HANDLE g_process = nullptr;

struct Reader {
    bool operator()(uint32_t address, void* out, size_t size) const {
        SIZE_T got = 0;
        return address >= 0x10000 && ReadProcessMemory(g_process, reinterpret_cast<LPCVOID>(static_cast<uintptr_t>(address)), out, size, &got) && got == size;
    }
};

DWORD FindGame() {
    DWORD id = 0;
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    for (BOOL more = Process32FirstW(snapshot, &entry); more && !id; more = Process32NextW(snapshot, &entry))
        if (_wcsicmp(entry.szExeFile, L"bf3.exe") == 0)
            id = entry.th32ProcessID;
    CloseHandle(snapshot);
    return id;
}

void Time(char* text, size_t size) {
    SYSTEMTIME t;
    GetLocalTime(&t);
    std::snprintf(text, size, "%02d:%02d:%02d.%03d", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
}

// The list as one line: each trigger's entity, action, key, held or tapped, an add not yet applied.
void Describe(const qtelist::List& list, char* text, size_t size) {
    static const char* const kStatus[] = {"no manager (between levels)", "read", "UNREADABLE", "NOT THE MANAGER (its vtable)"};
    int at = std::snprintf(text, size, "%s", kStatus[static_cast<int>(list.status)]);
    if (list.status != qtelist::Status::kOk)
        return;
    at += std::snprintf(text + at, size - at, ", manager %08X, %d live:", list.manager, list.count);
    for (int i = 0; i < list.count && at < static_cast<int>(size) - 80; ++i) {
        const qtelist::Trigger& t = list.at[i];
        const unsigned key = qtelist::KeyOf(t.action);
        at += std::snprintf(text + at, size - at, " [%08X action %02X %s%s%s%s%s]", t.entity, t.action, qtelist::ActionName(t.action), key ? ": " : "",
                            key ? qte::KeyName(key) : "", t.hold ? ", held" : ", tapped", t.pending ? ", add not yet applied" : "");
    }
    if (list.refused || list.twice || list.capped)
        std::snprintf(text + at, size - at, " (left out %d, listed again %d%s)", list.refused, list.twice, list.capped ? ", capped" : "");
}

}  // namespace

int main(int argc, char** argv) {
    const double seconds = argc > 1 ? std::atof(argv[1]) : 0.0;
    const DWORD id = FindGame();
    g_process = id ? OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, id) : nullptr;
    if (!g_process) {
        std::printf("bf3.exe isn't running (or can't be opened for reading)\n");
        return 1;
    }
    HMODULE module = nullptr;
    DWORD needed = 0;
    uint32_t base = 0x400000;
    if (K32EnumProcessModules(g_process, &module, sizeof(module), &needed) && module)
        base = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(module));
    const Reader read;
    IMAGE_DOS_HEADER dos{};
    IMAGE_NT_HEADERS32 nt{};
    uint32_t imageEnd = base + 0x208A000;
    if (read(base, &dos, sizeof(dos)) && dos.e_magic == IMAGE_DOS_SIGNATURE && read(base + static_cast<uint32_t>(dos.e_lfanew), &nt, sizeof(nt)) &&
        nt.Signature == IMAGE_NT_SIGNATURE)
        imageEnd = base + nt.OptionalHeader.SizeOfImage;
    std::printf("bf3.exe %lu: image %08X-%08X\n", id, base, imageEnd);
    const auto valid = [&](uint32_t global) { return global >= base && global < imageEnd && !(global & 3); };

    // The signature where 1.6.0.0 has it, and every find in the code's executable sections.
    uint8_t code[qtelist::kSignatureSize] = {};
    const bool there = read(base + qtelist::kSignatureAt, code, sizeof(code)) && qtelist::SignatureAt(code);
    std::printf("the signature at %08X (1.6.0.0's place): %s", base + qtelist::kSignatureAt, there ? "there" : "NOT there");
    if (there)
        std::printf(", its pointer at %08X", qtelist::SignatureGlobal(code));
    std::printf("\n");
    qtelist::Candidates candidates;
    int finds = 0, unread = 0;
    LARGE_INTEGER frequency, a, b;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&a);
    const uint32_t sections = base + static_cast<uint32_t>(dos.e_lfanew) + static_cast<uint32_t>(offsetof(IMAGE_NT_HEADERS32, OptionalHeader)) +
                              nt.FileHeader.SizeOfOptionalHeader;
    for (unsigned i = 0; i < nt.FileHeader.NumberOfSections && i < 32; ++i) {
        IMAGE_SECTION_HEADER section{};
        if (!read(sections + i * static_cast<uint32_t>(sizeof(section)), &section, sizeof(section)))
            break;
        if (!(section.Characteristics & (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_CNT_CODE)))
            continue;
        qtelist::Scan scan;
        scan.at = base + section.VirtualAddress;
        scan.end = scan.at + section.Misc.VirtualSize;
        for (;;) {
            if (qtelist::ScanStep(read, scan, 1 << 20, valid)) {
                ++finds;
                std::printf("  found at %08X: a pointer at %08X\n", scan.found, scan.global);
                qtelist::AddCandidate(candidates, scan.global, scan.found);
            } else if (scan.at >= scan.end) {
                break;
            }
        }
        unread += scan.unread;
    }
    QueryPerformanceCounter(&b);
    std::printf("the code looked through in %.0f ms (from outside): %d find%s, %d pages unread\n",
                1000.0 * static_cast<double>(b.QuadPart - a.QuadPart) / static_cast<double>(frequency.QuadPart), finds, finds == 1 ? "" : "s", unread);
    qtelist::AddCandidate(candidates, base + qtelist::kManagerGlobal, 0);

    const qtelist::Types types = qtelist::TypesAt(base);
    const int pick = qtelist::Pick(read, candidates, types.manager);
    if (pick < 0) {
        std::printf("none of the %d places leads to the trigger manager now (no level loaded, or none is it)\n", candidates.count);
        for (int i = 0; i < candidates.count; ++i) {
            uint32_t manager = 0, type = 0;
            read(candidates.global[i], &manager, 4);
            if (manager)
                read(manager, &type, 4);
            std::printf("  [%08X] = %08X, its type %08X (the manager's is %08X)\n", candidates.global[i], manager, type, types.manager);
        }
        if (seconds <= 0.0)
            return 2;
    } else {
        std::printf("picked by the manager's vtable: the pointer at %08X%s\n", candidates.global[pick],
                    candidates.global[pick] == base + qtelist::kManagerGlobal ? " (1.6.0.0's address)" : " (NOT 1.6.0.0's address)");
    }

    char line[4096], last[4096] = "", stamp[32];
    uint64_t lastBits = ~0ull;
    const ULONGLONG until = GetTickCount64() + static_cast<ULONGLONG>(seconds * 1000.0);
    int picked = pick;
    do {
        if (picked < 0)
            picked = qtelist::Pick(read, candidates, types.manager);
        if (picked >= 0) {
            const qtelist::List list = qtelist::Read(read, candidates.global[picked], types);
            Describe(list, line, sizeof(line));
            if (std::strcmp(line, last) != 0) {
                char play[128], scripted[128];
                Time(stamp, sizeof(stamp));
                std::printf("%s %s\n", stamp, line);
                std::printf("%s   awaited in play: %s; in a scripted moment: %s\n", stamp, qte::KeyNames(qtelist::Awaited(list, false).await, play, sizeof(play)),
                            qte::KeyNames(qtelist::Awaited(list, true).await, scripted, sizeof(scripted)));
                std::strcpy(last, line);
            }
        }
        const qtelist::Input input = qtelist::ReadInput(read, base + qtelist::kGameContext);
        const uint64_t bits = input.read ? input.digital : ~1ull;
        if (bits != lastBits) {
            char keys[128];
            Time(stamp, sizeof(stamp));
            if (input.read)
                std::printf("%s   the game's input: %016llX, keys seen: %s\n", stamp, static_cast<unsigned long long>(input.digital),
                            qte::KeyNames(qtelist::SeenKeys(input), keys, sizeof(keys)));
            else
                std::printf("%s   the game's input state: not read (no player)\n", stamp);
            lastBits = bits;
        }
        std::fflush(stdout);
        if (seconds > 0.0)
            Sleep(4);
    } while (GetTickCount64() < until);
    CloseHandle(g_process);
    return picked >= 0 ? 0 : 2;
}

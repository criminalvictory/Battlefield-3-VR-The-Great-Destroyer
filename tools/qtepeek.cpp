// Read only: the running game's list of live input triggers, through the mod's own reader (src/qtelist.h, as src/qtewatch.cpp runs it inside the game),
// from outside (ReadProcessMemory; nothing is ever written): the signature's finds in the game's code, the place the manager's vtable picks, the
// list with each trigger's action and key, the awaited keys in play and in a scripted moment, the game's own input state. For checking the reader
// against the real game without the mod, and for the next mission's quick-time events.
// Usage: qtepeek                 the finds, the pick and the list as it is now
//        qtepeek 120             ... then every change of the list and of the game's input for 120 seconds
//        qtepeek 240 fight       ... and the fist fights' reader (src/qtefight.h, build\designs\qte_fights.md section 7): each of its constants
//                                proven against the running game, each bus with live quick-time triggers attached as the mod does (every check
//                                said), then every change of a fight's 12 gates, its select, its count and its fail gate (#85), with times, and
//                                what the mod would press
//        qtepeek 240 fight registry   ... and every 5 s the client bus registry walked for every QuickTimePrefab bus (those not live too)
// The exit code: 0 read, 1 no bf3.exe, 2 the manager not found (no level loaded, or none of the places is it).

#include <windows.h>

#include <psapi.h>
#include <tlhelp32.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "../src/qte.h"
#include "../src/qtefight.h"
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

// ---- The fights (qtefight.h), read only.
struct Peeked {
    uint32_t bus = 0;
    bool fight = false;     // attached
    bool live = false;      // its triggers live in the list (else found by the registry)
    qtefight::Fight f;
    qtefight::Live triggers;
    char last[256] = "";    // its gates' line as last printed
};
constexpr int kMostPeeked = 32;
Peeked g_peeked[kMostPeeked];
int g_peekedCount = 0;

Peeked* PeekedAt(uint32_t bus) {
    for (int i = 0; i < g_peekedCount; ++i)
        if (g_peeked[i].bus == bus)
            return &g_peeked[i];
    return nullptr;
}

// A fight's gates as one line: each action's right and wrong gate (1 open), the select, the count, the fail gate.
void GatesLine(const qtefight::State& s, char* text, size_t size) {
    static const char* const kShort[6] = {"E", "LMB", "RMB", "F", "Space", "Z"};
    int at = std::snprintf(text, size, "right");
    for (int a = 0; a < 6; ++a)
        at += std::snprintf(text + at, size - at, " %s %d", kShort[a], (s.right >> a) & 1);
    at += std::snprintf(text + at, size - at, " | wrong");
    for (int a = 0; a < 6; ++a)
        at += std::snprintf(text + at, size - at, " %s %d", kShort[a], (s.wrong >> a) & 1);
    std::snprintf(text + at, size - at, " | select %d (%s), count %d, fail gate %d", s.select, s.select ? "mash" : "press", s.count, s.fail);
}

void SayAttach(const char* stamp, uint32_t bus, const qtefight::Attach& a, const qtefight::Live& live, bool registry) {
    char text[256], keys[128];
    if (a.check == qtefight::Check::kNone) {
        const qtefight::Fight& f = a.fight;
        std::printf("%s FIGHT bus %08X attached%s: %s, %d targets, %d live trigger%s (%s): the trigger's own entr%s the trigger; the 12 gates, the select and "
                    "the count found\n",
                    stamp, bus, registry ? " (from the registry)" : "", f.named ? qtefight::kPrefabName : "its name not readable", f.targets, live.count,
                    live.count == 1 ? "" : "s", qte::KeyNames(live.keys, keys, sizeof(keys)), live.count == 1 ? "y is" : "ies are");
        std::printf("%s   info %08X, blueprint %08X, objects %08X, entities %08X\n", stamp, f.info, f.blueprint, f.objects, f.entities);
        for (int a2 = 0; a2 < 6; ++a2)
            std::printf("%s   %-20s right gate %08X (data %08X), wrong %08X (data %08X)\n", stamp, qtelist::ActionName(qtefight::kActions[a2].action), f.gate[0][a2],
                        f.gateData[0][a2], f.gate[1][a2], f.gateData[1][a2]);
        std::printf("%s   select %08X (data %08X), count %08X (data %08X, from %d), fail gate %08X (data %08X)\n", stamp, f.select, f.selectData, f.count,
                    f.countData, f.countStart, f.fail, f.failData);
    } else {
        std::printf("%s bus %08X (%d live quick-time trigger%s: %s) not attached: %s%s\n", stamp, bus, live.count, live.count == 1 ? "" : "s",
                    qte::KeyNames(live.keys, keys, sizeof(keys)), qtefight::Describe(a, text, sizeof(text)),
                    qtefight::NotThePrefab(a.check) ? " [not a fist fight's]" : " [a check failed]");
    }
}

// The client bus registry: every bus by its network id; each QuickTimePrefab one attached (no triggers: the checks that need one left out).
void WalkRegistry(const Reader& read, const qtefight::Types& types, uint32_t global) {
    char stamp[32];
    Time(stamp, sizeof(stamp));
    uint32_t registry = 0, range[2] = {};
    if (!read(global, &registry, 4) || !qtelist::Pointer(registry) || !read(registry, range, 8) || range[1] < range[0] || ((range[1] - range[0]) & 3)) {
        std::printf("%s the bus registry [%08X] = %08X: not readable\n", stamp, global, registry);
        return;
    }
    const uint32_t n = (range[1] - range[0]) / 4 > 65536 ? 65536 : (range[1] - range[0]) / 4;
    int buses = 0, prefabs = 0;
    for (uint32_t i = 0; i < n; i += 1024) {
        uint32_t chunk[1024];
        const uint32_t take = n - i < 1024 ? n - i : 1024;
        if (!read(range[0] + 4 * i, chunk, take * 4))
            continue;
        for (uint32_t k = 0; k < take; ++k) {
            uint32_t vtable = 0;
            if (!qtelist::Pointer(chunk[k]) || !read(chunk[k], &vtable, 4) || vtable != types.bus)
                continue;
            ++buses;
            const qtefight::Attach a = qtefight::AttachBus(read, types, chunk[k], qtefight::Live{});
            if (a.check != qtefight::Check::kNone)
                continue;
            ++prefabs;
            if (PeekedAt(chunk[k]) || g_peekedCount == kMostPeeked)
                continue;
            Peeked& p = g_peeked[g_peekedCount++];
            p = Peeked{};
            p.bus = chunk[k];
            p.fight = true;
            p.f = a.fight;
            SayAttach(stamp, chunk[k], a, p.triggers, true);
        }
    }
    std::printf("%s the bus registry: %u places, %d connected buses, %d of them QuickTimePrefab's\n", stamp, n, buses, prefabs);
}

// Each poll: the list's buses with quick-time triggers attached once each (again when their triggers change), every fight's gates read.
void PeekFights(const Reader& read, const qtefight::Types& types, const qtelist::List& list) {
    char stamp[32];
    qtefight::Live groups[qtelist::kMost + qtelist::kMostPending];
    uint32_t buses[qtelist::kMost + qtelist::kMostPending];
    int count = 0;
    if (list.status == qtelist::Status::kOk)
        for (int i = 0; i < list.count; ++i) {
            const qtelist::Trigger& t = list.at[i];
            if (!qtelist::QuickTime(t.action))
                continue;
            int g = 0;
            while (g < count && buses[g] != t.bus)
                ++g;
            if (g == count) {
                buses[g] = t.bus;
                groups[g] = qtefight::Live{};
                ++count;
            }
            groups[g].Add(t.entity, t.data, t.action);
        }
    for (int i = 0; i < g_peekedCount; ++i)
        g_peeked[i].live = false;
    for (int g = 0; g < count; ++g) {
        Peeked* p = PeekedAt(buses[g]);
        if (p && p->triggers.Same(groups[g])) {
            p->live = true;
            continue;
        }
        Time(stamp, sizeof(stamp));
        const qtefight::Attach a = qtefight::AttachBus(read, types, buses[g], groups[g]);
        SayAttach(stamp, buses[g], a, groups[g], false);
        if (!p) {
            if (g_peekedCount == kMostPeeked)
                continue;
            p = &g_peeked[g_peekedCount++];
            *p = Peeked{};
            p->bus = buses[g];
        }
        p->live = true;
        p->triggers = groups[g];
        p->fight = a.check == qtefight::Check::kNone;
        if (p->fight)
            p->f = a.fight;
        p->last[0] = 0;
    }
    qtefight::State states[kMostPeeked];
    int read2 = 0;
    for (int i = 0; i < g_peekedCount; ++i) {
        Peeked& p = g_peeked[i];
        if (!p.fight)
            continue;
        const qtefight::State s = qtefight::ReadState(read, types, p.f, true);
        char line[256];
        if (!s.read)
            std::snprintf(line, sizeof(line), "NOT READ as attached (%s, object %d)", qtefight::CheckName(s.check), s.index);
        else
            GatesLine(s, line, sizeof(line));
        if (std::strcmp(line, p.last) != 0) {
            Time(stamp, sizeof(stamp));
            std::printf("%s   bus %08X%s: %s\n", stamp, p.bus, p.live ? "" : " (no live trigger)", line);
            std::strcpy(p.last, line);
        }
        if (s.read && p.live)
            states[read2++] = s;
    }
    // What the mod would press (the rule over the live fights).
    static char lastAnswer[128] = "";
    const qtefight::Answer a = qtefight::Decide(states, read2);
    char answer[128], names[96];
    switch (a.why) {
    case qtefight::Why::kNone: std::snprintf(answer, sizeof(answer), "no prompt: nothing pressed"); break;
    case qtefight::Why::kAnswer:
        std::snprintf(answer, sizeof(answer), "the mod would press %s (%s), %s, %d to go", qte::KeyName(a.key), qtefight::ActionOfKey(a.key), a.mash ? "a mash" : "a press",
                      a.left);
        break;
    case qtefight::Why::kTwo: std::snprintf(answer, sizeof(answer), "two prompts open (%s): nothing", qtefight::ActionNames(a.right, names, sizeof(names))); break;
    case qtefight::Why::kWrongAlone: std::snprintf(answer, sizeof(answer), "a wrong gate with no prompt (%s's): nothing", qtefight::ActionNames(a.wrong, names, sizeof(names))); break;
    case qtefight::Why::kWrongOther: std::snprintf(answer, sizeof(answer), "another's wrong gate open (%s's): nothing", qtefight::ActionNames(a.wrong & ~a.right, names, sizeof(names))); break;
    }
    if (read2 && std::strcmp(answer, lastAnswer) != 0) {
        Time(stamp, sizeof(stamp));
        std::printf("%s   %s\n", stamp, answer);
        std::strcpy(lastAnswer, answer);
    }
}

}  // namespace

int main(int argc, char** argv) {
    const double seconds = argc > 1 ? std::atof(argv[1]) : 0.0;
    bool fight = false, registry = false;
    for (int i = 1; i < argc; ++i) {
        fight = fight || _stricmp(argv[i], "fight") == 0;
        registry = registry || _stricmp(argv[i], "registry") == 0;
    }
    fight = fight || registry;
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
    // The fights' reader's constants, each against the running game as the mod proves them at its start.
    const qtefight::Types fightTypes = qtefight::TypesAt(base);
    bool fightsProven = false, registryProven = false;
    if (fight) {
        int failed = 0;
        for (int i = 0; i < qtefight::kProofs + (registry ? 1 : 0); ++i) {
            uint32_t at = 0;
            const bool ok = qtefight::ProveOne(read, base, i, &at);
            if (i < qtefight::kProofs)
                failed += !ok;
            else
                registryProven = ok;
            std::printf("  %s %s (at %08X)\n", ok ? "borne out:    " : "NOT BORNE OUT:", qtefight::ProofName(i), at);
        }
        fightsProven = failed == 0;
        std::printf("the fights' reader: %s\n", fightsProven ? "every constant borne out (the mod's reader on)"
                                                             : "a constant NOT borne out (the mod's reader stays off; nothing more read here)");
    }
    ULONGLONG walkedAt = 0;
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
                if (list.status == qtelist::Status::kOk) {
                    std::printf("%s   buses:", stamp);
                    for (int i = 0; i < list.count; ++i)
                        std::printf(" [%08X: bus %08X, data %08X]", list.at[i].entity, list.at[i].bus, list.at[i].data);
                    std::printf("\n");
                }
                std::printf("%s   awaited in play (a fight's six left out): %s; in a scripted moment: %s\n", stamp,
                            qte::KeyNames(qtelist::Awaited(list, false).await, play, sizeof(play)),
                            qte::KeyNames(qtelist::Awaited(list, true).await, scripted, sizeof(scripted)));
                std::strcpy(last, line);
            }
            if (fight && fightsProven)
                PeekFights(read, fightTypes, list);
        }
        if (registryProven && fightsProven && GetTickCount64() - walkedAt >= 5000) {
            walkedAt = GetTickCount64();
            WalkRegistry(read, fightTypes, base + qtefight::kRegistryGlobal);
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

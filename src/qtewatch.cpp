#include "qtewatch.h"

#include <windows.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "camera.h"
#include "log.h"
#include "qte.h"
#include "qtelist.h"

namespace {

// Every read of the game's memory: guarded (camera::SafeRead), never the first 64 KB.
struct GameReader {
    bool operator()(uint32_t address, void* out, size_t size) const {
        return address >= 0x10000 && address <= 0xFFFFFFFFu - static_cast<uint32_t>(size) && camera::SafeRead(address, out, size);
    }
};

// Where the manager's pointer is: looked for by the signature of the code that reads it (qtelist.h), every find a candidate, and the one that
// leads to an object with the manager's vtable is it. Nothing is read through an address the vtable hasn't borne out.
enum class Find { kNotBegun, kScanning, kPicking, kPicked, kNone };

Find g_find = Find::kNotBegun;
uint32_t g_base = 0, g_imageEnd = 0;
uint32_t g_global = 0;  // where the manager's pointer is read from (picked)
qtelist::Candidates g_candidates;
qtelist::Scan g_scan;
constexpr uint32_t kScanAFrame = 1 << 20;
// The game's code: its executable sections, looked through one after another.
struct Range {
    uint32_t begin = 0, end = 0;
};
Range g_code[8];
int g_codeCount = 0, g_codeAt = 0;
int g_unread = 0;
ULONGLONG g_levelSince = 0;  // since when a level runs with no candidate borne out
bool g_saidNone = false;
constexpr ULONGLONG kSayNoneAfter = 10000;

bool GlobalValid(uint32_t global) {
    return global >= g_base && global < g_imageEnd && !(global & 3);
}

// The game's image by its headers: its size, and its code (the executable sections). Headers that can't be read or make no sense: 1.6.0.0's
// own numbers (its .text; an address found there is still checked like any other).
void Image() {
    const GameReader read;
    IMAGE_DOS_HEADER dos{};
    IMAGE_NT_HEADERS32 nt{};
    g_codeCount = 0;
    if (read(g_base, &dos, sizeof(dos)) && dos.e_magic == IMAGE_DOS_SIGNATURE && dos.e_lfanew > 0 && dos.e_lfanew <= 0x1000 &&
        read(g_base + static_cast<uint32_t>(dos.e_lfanew), &nt, sizeof(nt)) && nt.Signature == IMAGE_NT_SIGNATURE &&
        nt.OptionalHeader.SizeOfImage >= 0x2000 && nt.OptionalHeader.SizeOfImage <= 0x10000000) {
        g_imageEnd = g_base + nt.OptionalHeader.SizeOfImage;
        const uint32_t first = g_base + static_cast<uint32_t>(dos.e_lfanew) + static_cast<uint32_t>(offsetof(IMAGE_NT_HEADERS32, OptionalHeader)) +
                               nt.FileHeader.SizeOfOptionalHeader;
        for (unsigned i = 0; i < nt.FileHeader.NumberOfSections && i < 32 && g_codeCount < 8; ++i) {
            IMAGE_SECTION_HEADER section{};
            if (!read(first + i * static_cast<uint32_t>(sizeof(section)), &section, sizeof(section)))
                break;
            if (!(section.Characteristics & (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_CNT_CODE)))
                continue;
            const uint32_t begin = g_base + section.VirtualAddress, end = begin + section.Misc.VirtualSize;
            if (begin < g_base || end <= begin || end > g_imageEnd)
                continue;
            g_code[g_codeCount].begin = begin;
            g_code[g_codeCount++].end = end;
        }
    }
    if (!g_codeCount) {
        g_imageEnd = g_base + 0x208A000;
        g_code[0].begin = g_base + 0x1000;
        g_code[0].end = g_base + 0x1AF0000;
        g_codeCount = 1;
        Log("QTE: the game's headers gave no code section: 1.6.0.0's own (%08X-%08X) is taken", g_code[0].begin, g_code[0].end);
    }
}

// The address of the manager's pointer, once found and borne out by the manager's vtable. Until then nothing is read through it.
bool Found() {
    if (g_find == Find::kPicked)
        return true;
    if (g_find == Find::kNone)
        return false;
    const GameReader read;
    if (g_find == Find::kNotBegun) {
        g_base = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)));
        if (!g_base) {
            g_find = Find::kNone;
            Log("QTE: the game's image wasn't found: its trigger manager isn't looked for, the grips answer no quick-time events");
            return false;
        }
        Image();
        // Where bf3.exe 1.6.0.0 has the signature, first: there, it's the only candidate (the code's other find is another manager's).
        uint8_t code[qtelist::kSignatureSize] = {};
        if (read(g_base + qtelist::kSignatureAt, code, sizeof(code)) && qtelist::SignatureAt(code) && GlobalValid(qtelist::SignatureGlobal(code))) {
            qtelist::AddCandidate(g_candidates, qtelist::SignatureGlobal(code), g_base + qtelist::kSignatureAt);
            g_find = Find::kPicking;
            Log("QTE: the trigger manager's signature is in the game's code where 1.6.0.0 has it (%08X): its pointer at %08X, taken once the manager's type bears it out",
                g_base + qtelist::kSignatureAt, g_candidates.global[0]);
        } else {
            g_codeAt = 0;
            g_unread = 0;
            g_scan = qtelist::Scan{};
            g_scan.at = g_code[0].begin;
            g_scan.end = g_code[0].end;
            g_find = Find::kScanning;
            Log("QTE: the trigger manager's signature isn't where 1.6.0.0 has it: the game's code (%d section%s from %08X) is looked through, a megabyte a frame",
                g_codeCount, g_codeCount == 1 ? "" : "s", g_code[0].begin);
        }
    }
    if (g_find == Find::kScanning) {
        while (qtelist::ScanStep(read, g_scan, kScanAFrame, GlobalValid))  // (every find kept: the vtable picks among them)
            qtelist::AddCandidate(g_candidates, g_scan.global, g_scan.found);
        if (g_scan.at < g_scan.end)
            return false;  // (more of this section next frame)
        g_unread += g_scan.unread;
        if (++g_codeAt < g_codeCount) {
            g_scan = qtelist::Scan{};
            g_scan.at = g_code[g_codeAt].begin;
            g_scan.end = g_code[g_codeAt].end;
            return false;
        }
        const int finds = g_candidates.count;
        qtelist::AddCandidate(g_candidates, g_base + qtelist::kManagerGlobal, 0);  // (1.6.0.0's address too, as one more to be borne out)
        g_find = Find::kPicking;
        Log("QTE: the game's code looked through (%d pages unread): the signature found %d time%s; with 1.6.0.0's address %d place%s to try, the one "
            "leading to the manager's type is taken",
            g_unread, finds, finds == 1 ? "" : "s", g_candidates.count, g_candidates.count == 1 ? "" : "s");
    }
    const int pick = qtelist::Pick(read, g_candidates, g_base + qtelist::kManagerType);
    if (pick >= 0) {
        g_global = g_candidates.global[pick];
        g_find = Find::kPicked;
        if (g_candidates.found[pick])
            Log("QTE: the trigger manager's pointer is at %08X (by the signature at %08X), borne out by the manager's type: a fresh squeeze of either grip "
                "presses what the game's live triggers await",
                g_global, g_candidates.found[pick]);
        else
            Log("QTE: the trigger manager's pointer is at %08X (1.6.0.0's address; no signature led to it), borne out by the manager's type: a fresh "
                "squeeze of either grip presses what the game's live triggers await",
                g_global);
        return true;
    }
    // None yet: no level is loaded (the pointer is null between levels), or none of them is it: said once, when a level has run 10 s without.
    if (!g_saidNone) {
        const ULONGLONG now = GetTickCount64();
        if (!qtelist::ReadInput(read, g_base + qtelist::kGameContext).read) {
            g_levelSince = 0;
        } else if (!g_levelSince) {
            g_levelSince = now;
        } else if (now - g_levelSince > kSayNoneAfter) {
            g_saidNone = true;
            Log("QTE: a level is running and none of the %d place%s found leads to the game's trigger manager: the grips answer no quick-time events (it is "
                "still looked for)",
                g_candidates.count, g_candidates.count == 1 ? "" : "s");
        }
    }
    return false;
}

// The triggers last said to be live, for the lines of one appearing and leaving.
struct Shown {
    uint32_t entity = 0, action = 0;
    ULONGLONG since = 0;
};
Shown g_shown[qtelist::kMost + qtelist::kMostPending];
int g_shownCount = 0;
uint32_t g_shownManager = 0;
qtelist::Steady g_steady;
qtelist::Status g_saidStatus = qtelist::Status::kOk;
bool g_saidWrong = false, g_saidUnread = false;
int g_lines = 0;
constexpr int kMostLines = 600;  // (a run's; a level's quick-time events are a handful)
uint8_t g_saidOther[32] = {};    // other actions said once each (a bit an action)

bool Line() {
    if (g_lines == kMostLines)
        Log("QTE: more triggers appearing and leaving aren't logged (%d lines so far)", g_lines);
    return g_lines++ < kMostLines;
}

// The list not readable any more, or what the pointer leads to no longer the manager: said once each.
void SayStatus(const qtelist::List& now, const qtelist::Types& types) {
    if (now.status == g_saidStatus)
        return;
    const qtelist::Status was = g_saidStatus;
    g_saidStatus = now.status;
    if (now.status == qtelist::Status::kWrongType && !g_saidWrong) {
        g_saidWrong = true;
        uint32_t type = 0;
        GameReader()(now.manager, &type, sizeof(type));
        Log("QTE: what the manager's pointer (at %08X) leads to now, %08X, isn't the trigger manager (its type %08X, not %08X): nothing is read through it, "
            "the grips answer no quick-time events meanwhile",
            g_global, now.manager, type, types.manager);
    } else if (now.status == qtelist::Status::kUnreadable && !g_saidUnread) {
        g_saidUnread = true;
        Log("QTE: the trigger manager (pointer at %08X, manager %08X) couldn't be read: the grips answer no quick-time events meanwhile", g_global,
            now.manager);
    } else if (now.status == qtelist::Status::kOk && (was == qtelist::Status::kWrongType || was == qtelist::Status::kUnreadable)) {
        Log("QTE: the trigger manager is read again (%08X)", now.manager);
    }
}

void SayChanges(const qtelist::List& list, bool scripted) {
    const ULONGLONG now = GetTickCount64();
    const int count = list.status == qtelist::Status::kOk ? list.count : 0;
    if (list.status == qtelist::Status::kOk && list.manager != g_shownManager) {
        g_shownManager = list.manager;
        if (Line())
            Log("QTE: a level's trigger manager at %08X (%d live trigger%s)", list.manager, list.count, list.count == 1 ? "" : "s");
    }
    // Gone: said before the new ones, as they went.
    for (int i = 0; i < g_shownCount;) {
        bool still = false;
        for (int k = 0; k < count && !still; ++k)
            still = list.at[k].entity == g_shown[i].entity && list.at[k].action == g_shown[i].action;
        if (still) {
            ++i;
            continue;
        }
        const Shown gone = g_shown[i];
        g_shown[i] = g_shown[--g_shownCount];
        if (qtelist::KeyOf(gone.action) && Line())
            Log("QTE: the game no longer awaits %s (action 0x%02X, %s) after %.2f s [trigger %08X]", qtelist::ActionName(gone.action), gone.action,
                qte::KeyName(qtelist::KeyOf(gone.action)), static_cast<double>(now - gone.since) / 1000.0, gone.entity);
    }
    for (int k = 0; k < count; ++k) {
        const qtelist::Trigger& t = list.at[k];
        bool known = false;
        for (int i = 0; i < g_shownCount && !known; ++i)
            known = g_shown[i].entity == t.entity && g_shown[i].action == t.action;
        if (known || g_shownCount >= static_cast<int>(sizeof(g_shown) / sizeof(g_shown[0])))
            continue;
        Shown& s = g_shown[g_shownCount++];
        s.entity = t.entity;
        s.action = t.action;
        s.since = now;
        const unsigned key = qtelist::KeyOf(t.action);
        if (key) {
            if (Line())
                Log("QTE: the game awaits %s (action 0x%02X): %s, %s%s%s [trigger %08X]", qtelist::ActionName(t.action), t.action, qte::KeyName(key),
                    t.hold ? "held" : "tapped", t.pending ? ", its add not yet applied" : "",
                    !qtelist::Gated(t.action) ? ""
                    : scripted                ? " (play's own action: answered, this being a scripted moment)"
                                              : " (play's own action: answered only in a scripted moment)",
                    t.entity);
        } else if (t.action >= 10 && t.action < 256 && !(g_saidOther[t.action >> 3] & (1 << (t.action & 7)))) {
            // Another digital action (the analog ones, yaw and pitch among them, are always there): once each, for the next mission's reader.
            g_saidOther[t.action >> 3] |= static_cast<uint8_t>(1 << (t.action & 7));
            if (Line())
                Log("QTE: a live trigger for action 0x%02X, none the grips answer (%s) [trigger %08X]", t.action, t.hold ? "read by its level" : "by its edge",
                    t.entity);
        }
    }
}

}  // namespace

namespace qtewatch {

Frame Update(bool scripted) {
    Frame frame;
    if (!Found())
        return frame;
    const GameReader read;
    const qtelist::Types types = qtelist::TypesAt(g_base);
    const qtelist::List now = qtelist::Read(read, g_global, types);
    SayStatus(now, types);
    const qtelist::List& list = qtelist::Settle(g_steady, now, GetTickCount64());
    SayChanges(list, scripted);
    const qtelist::Keys keys = qtelist::Awaited(list, scripted);
    frame.await = keys.await;
    frame.hold = keys.hold;
    frame.read = list.status == qtelist::Status::kOk || list.status == qtelist::Status::kNone;
    if (list.status == qtelist::Status::kOk) {  // (also with nothing awaited: the frame a trigger goes, its last press may show)
        const qtelist::Input input = qtelist::ReadInput(read, g_base + qtelist::kGameContext);
        frame.seen = qtelist::SeenKeys(input);
        frame.seenRead = input.read;
    }
    return frame;
}

}  // namespace qtewatch

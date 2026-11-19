#include "qtewatch.h"

#include <windows.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "camera.h"
#include "log.h"
#include "qte.h"
#include "qtefight.h"
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
            // (The game makes the manager with the first Enable of an input trigger on a level, so a level without one has none yet: Swordbreaker's
            // garage after a load, till its fights' Start. build\designs\qte_fights.md section 6.)
            Log("QTE: a level is running and none of the %d place%s found leads to the game's trigger manager: no input trigger enabled yet on this level "
                "(the manager is made with the first), so nothing to answer yet (it is still looked for)",
                g_candidates.count, g_candidates.count == 1 ? "" : "s");
        }
    }
    return false;
}

// The triggers last said to be live, for the lines of one appearing and leaving.
struct Shown {
    uint32_t entity = 0, action = 0;
    ULONGLONG since = 0;
    bool fight = false;  // a fist fight's (said so: pressed only for its open prompt)
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

// fights: the buses proven a fist fight's (qtefight.h); a bus with all six quick-time actions live is one too (qtelist::Awaited leaves them out).
void SayChanges(const qtelist::List& list, bool scripted, const qtelist::Skip& fights) {
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
            Log("QTE: %s %s (action 0x%02X, %s) after %.2f s [trigger %08X]", gone.fight ? "a fist fight's trigger gone:" : "the game no longer awaits",
                qtelist::ActionName(gone.action), gone.action, qte::KeyName(qtelist::KeyOf(gone.action)), static_cast<double>(now - gone.since) / 1000.0,
                gone.entity);
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
        s.fight = fights.Has(t.bus) || qtelist::QuickTimeOn(list, t.bus) == qte::kKeysV2;
        const unsigned key = qtelist::KeyOf(t.action);
        if (key && s.fight) {
            if (Line())
                Log("QTE: a fist fight's trigger: %s (action 0x%02X, %s) [trigger %08X, bus %08X]: pressed only for its open prompt", qtelist::ActionName(t.action),
                    t.action, qte::KeyName(key), t.entity, t.bus);
        } else if (key) {
            if (Line())
                Log("QTE: the game awaits %s (action 0x%02X): %s, %s%s%s [trigger %08X, bus %08X]", qtelist::ActionName(t.action), t.action, qte::KeyName(key),
                    t.hold ? "held" : "tapped", t.pending ? ", its add not yet applied" : "",
                    !qtelist::Gated(t.action) ? ""
                    : scripted                ? " (play's own action: answered, this being a scripted moment)"
                                              : " (play's own action: answered only in a scripted moment)",
                    t.entity, t.bus);
        } else if (t.action >= 10 && t.action < 256 && !(g_saidOther[t.action >> 3] & (1 << (t.action & 7)))) {
            // Another digital action (the analog ones, yaw and pitch among them, are always there): once each, for the next mission's reader.
            g_saidOther[t.action >> 3] |= static_cast<uint8_t>(1 << (t.action & 7));
            if (Line())
                Log("QTE: a live trigger for action 0x%02X, none the grips answer (%s) [trigger %08X]", t.action, t.hold ? "read by its level" : "by its edge",
                    t.entity);
        }
    }
}

// ---- The fist fights (qtefight.h): the reader's constants borne out once a run; the buses it watches; their attaches, prompts and presses said.
enum class Proved { kNotYet, kYes, kNo };
Proved g_proved = Proved::kNotYet;
qtefight::Types g_fightTypes;
qtefight::Watch g_watch;
int g_fightLines = 0;
constexpr int kMostFightLines = 1500;  // (a run's: 13 fights of about ten prompts, a mash's count said as it goes)

bool FightLine() {
    if (g_fightLines == kMostFightLines)
        Log("QTE fight: more of the fights' lines aren't logged (%d so far)", g_fightLines);
    return g_fightLines++ < kMostFightLines;
}

void ProveFights() {
    const qtefight::Proofs p = qtefight::Prove(GameReader(), g_base);
    g_fightTypes = qtefight::TypesAt(g_base);
    g_proved = p.ok ? Proved::kYes : Proved::kNo;
    if (p.ok)
        Log("QTE fight: the fights' reader's %d constants borne out by the game's code and type names (EventGateEntity %08X, SelectEventEntity %08X, "
            "CountDownEntity %08X, the connected bus %08X, LogicPrefabBlueprint %08X): a fist fight's open prompt is read from its gates",
            qtefight::kProofs, g_fightTypes.gate, g_fightTypes.select, g_fightTypes.count, g_fightTypes.bus, g_fightTypes.prefab);
    else
        Log("QTE fight: %s isn't as bf3.exe 1.6.0.0 has it (looked for at %08X): the fights' reader is off for this run; a fist fight's six quick-time "
            "triggers press nothing, never a guess (the single quick-time events are answered as before)",
            qtefight::ProofName(p.failed), p.at);
}

void SayFightEvents(const qtefight::Frame& f, bool fightsOn) {
    char text[256], keys[128];
    for (int i = 0; i < f.events; ++i) {
        const qtefight::Event& e = f.event[i];
        const bool six = e.keys == qte::kKeysV2;
        switch (e.what) {
        case qtefight::Ev::kAttached: {
            if (e.again || !FightLine())
                break;
            const qtefight::Fight& g = e.attach.fight;
            Log("QTE fight: bus %08X, %s, %d targets: the 12 gates, the select and the count found (%d live trigger%s: %s) [right gates %08X %08X %08X %08X %08X "
                "%08X, wrong %08X %08X %08X %08X %08X %08X, select %08X, count %08X from %d, fail gate %08X]%s",
                e.bus, g.named ? qtefight::kPrefabName : "its name not readable", g.targets, e.triggers, e.triggers == 1 ? "" : "s",
                qte::KeyNames(e.keys, keys, sizeof(keys)), g.gate[0][0], g.gate[0][1], g.gate[0][2], g.gate[0][3], g.gate[0][4], g.gate[0][5], g.gate[1][0],
                g.gate[1][1], g.gate[1][2], g.gate[1][3], g.gate[1][4], g.gate[1][5], g.select, g.count, g.countStart, g.fail,
                g.fail ? "; its prompts counted while its fail gate is open"
                       : "; its fail gate not as the prefab has it: each prompt counted 10 s at most");
            break;
        }
        case qtefight::Ev::kStale:
            if (FightLine()) {
                char right[96], wrong[96];
                Log("QTE fight: bus %08X's gates have stood open %llu s unchanged (right: %s; wrong: %s; fail gate %d): taken as over, nothing pressed for them "
                    "and nothing held up till they change",
                    e.bus, (e.state.fail < 0 ? qtefight::kStaleMs : qtefight::kStaleArmedMs) / 1000, qtefight::ActionNames(e.state.right, right, sizeof(right)),
                    qtefight::ActionNames(e.state.wrong, wrong, sizeof(wrong)), e.state.fail);
            }
            break;
        case qtefight::Ev::kDisarmed:
            if (FightLine()) {
                char right[96], wrong[96];
                Log("QTE fight: bus %08X's gates open (right: %s; wrong: %s) with its fail gate shut (the fight won, or failed): no prompt, nothing pressed for "
                    "them, nothing held up",
                    e.bus, qtefight::ActionNames(e.state.right, right, sizeof(right)), qtefight::ActionNames(e.state.wrong, wrong, sizeof(wrong)));
            }
            break;
        case qtefight::Ev::kNotPrefab:
            if (!FightLine())
                break;
            if (six)
                Log("QTE fight: bus %08X has all six quick-time actions live but isn't the quick-time prefab (%s): its triggers press nothing (tried again "
                    "every %.2f s while they're live: a read that didn't go says so too)",
                    e.bus, qtefight::Describe(e.attach, text, sizeof(text)), static_cast<double>(qtefight::kRetryMs) / 1000.0);
            else
                Log("QTE fight: bus %08X (%d quick-time trigger%s: %s) isn't a fist fight's (%s): its triggers are answered as before", e.bus, e.triggers,
                    e.triggers == 1 ? "" : "s", qte::KeyNames(e.keys, keys, sizeof(keys)), qtefight::Describe(e.attach, text, sizeof(text)));
            break;
        case qtefight::Ev::kFailed:
            if (FightLine())
                Log("QTE fight: bus %08X (%d quick-time trigger%s: %s) not attached: %s: %s; tried again every %.2f s", e.bus, e.triggers,
                    e.triggers == 1 ? "" : "s", qte::KeyNames(e.keys, keys, sizeof(keys)), qtefight::Describe(e.attach, text, sizeof(text)),
                    six                                    ? "its six press nothing meanwhile, never a guess"
                    : qtefight::ThePrefab(e.attach.check) ? "its blueprint the prefab's: its triggers press nothing meanwhile, never a guess"
                                                           : "its triggers answered as before meanwhile",
                    static_cast<double>(qtefight::kRetryMs) / 1000.0);
            break;
        case qtefight::Ev::kLost:
            if (FightLine()) {
                char object[32] = "";
                if (e.state.index >= 0)
                    std::snprintf(object, sizeof(object), ", object %d", e.state.index);
                Log("QTE fight: bus %08X didn't read as attached this frame (%s%s): nothing pressed; attached again", e.bus,
                    e.state.check == qtefight::Check::kBus ? "the bus itself" : qtefight::CheckName(e.state.check), object);
            }
            break;
        case qtefight::Ev::kGone:
            if (FightLine())
                Log("QTE fight: bus %08X's quick-time triggers gone (the fight's Stop, or a load): forgotten", e.bus);
            break;
        case qtefight::Ev::kOff:
            if (FightLine())
                Log("QTE fight: a fist fight's six quick-time triggers are live (bus %08X): the fights' reader is off (%s): they press nothing", e.bus,
                    fightsOn ? "its constants not borne out by the game's code: the line at the start" : "weapon.txt qtefight 0");
            break;
        }
    }
}

// The prompt being said, its presses as the game's input shows them.
struct Prompt {
    unsigned key = 0;  // the answer (0: none)
    bool mash = false;
    int left = 0;
    ULONGLONG since = 0;
    int seen = 0;      // presses of it the game's input showed
    bool down = false; // ... its key shown down last frame
};
Prompt g_prompt;
qtefight::Why g_saidWhy = qtefight::Why::kNone;  // (the gates unclear, as last said)
unsigned g_saidGatesRight = 0, g_saidGatesWrong = 0;

// scripted: version 1's scripted moment, said with each prompt (whether the fights always run in one: runbook step 2).
void SayPrompts(const qtefight::Frame& f, unsigned seen, bool seenRead, ULONGLONG now, bool scripted) {
    if (f.unknown)
        return;  // (a frame not read as attached: said by its own line; the prompt as it was)
    const unsigned key = f.prompt ? f.answer : 0;
    if (g_prompt.key && g_prompt.key != key) {
        if (FightLine())
            Log("QTE fight: %s no longer awaited after %.2f s (presses seen %d)", qtefight::ActionOfKey(g_prompt.key),
                static_cast<double>(now >= g_prompt.since ? now - g_prompt.since : 0) / 1000.0, g_prompt.seen);
        g_prompt = Prompt{};
    }
    if (key && !g_prompt.key) {
        g_prompt.key = key;
        g_prompt.mash = f.mash;
        g_prompt.left = f.left;
        g_prompt.since = now;
        g_prompt.down = seenRead && (seen & key);  // (a key already down as it opened isn't a press of it)
        if (FightLine()) {
            const char* moment = scripted ? " [a scripted moment]" : " [not a scripted moment]";
            if (f.mash)
                Log("QTE fight: the game awaits %s (%s), a mash, %d to go%s", qtefight::ActionOfKey(key), qte::KeyName(key), f.left, moment);
            else
                Log("QTE fight: the game awaits %s (%s), a press%s", qtefight::ActionOfKey(key), qte::KeyName(key), moment);
        }
    } else if (key) {
        if (f.mash != g_prompt.mash) {
            if (FightLine())
                Log(f.mash ? "QTE fight: %s now a mash, %d to go" : "QTE fight: %s now a press (%d)", qtefight::ActionOfKey(key), f.left);
            g_prompt.mash = f.mash;
            g_prompt.left = f.left;
        } else if (f.mash && f.left != g_prompt.left) {
            if (FightLine())
                Log("QTE fight: mash: %d to go", f.left);
            g_prompt.left = f.left;
        }
        const bool down = seenRead && (seen & key);
        if (down && !g_prompt.down)
            ++g_prompt.seen;
        g_prompt.down = down;
    }
    // The gates unclear (two prompts, a wrong gate): said as they change.
    const qtefight::Why why = f.prompt && !f.answer ? f.gates.why : qtefight::Why::kNone;
    if (why != g_saidWhy || (why != qtefight::Why::kNone && (f.gates.right != g_saidGatesRight || f.gates.wrong != g_saidGatesWrong))) {
        char a[96], b[96];
        if (why == qtefight::Why::kTwo && FightLine())
            Log("QTE fight: two prompts open (%s): nothing pressed", qtefight::ActionNames(f.gates.right, a, sizeof(a)));
        else if (why == qtefight::Why::kWrongAlone && FightLine())
            Log("QTE fight: a wrong gate open with no prompt (%s's): nothing pressed", qtefight::ActionNames(f.gates.wrong, a, sizeof(a)));
        else if (why == qtefight::Why::kWrongOther && FightLine())
            Log("QTE fight: %s's prompt with another's wrong gate open (%s's): nothing pressed", qtefight::ActionNames(f.gates.right, a, sizeof(a)),
                qtefight::ActionNames(f.gates.wrong & ~f.gates.right, b, sizeof(b)));
        g_saidWhy = why;
        g_saidGatesRight = f.gates.right;
        g_saidGatesWrong = f.gates.wrong;
    }
}

}  // namespace

namespace qtewatch {

Frame Update(bool scripted, bool fights) {
    Frame frame;
    const bool found = Found();
    if (g_base && g_proved == Proved::kNotYet)
        ProveFights();  // (once a run, as soon as the game's image is known)
    if (!found)
        return frame;
    const GameReader read;
    const qtelist::Types types = qtelist::TypesAt(g_base);
    const qtelist::List now = qtelist::Read(read, g_global, types);
    SayStatus(now, types);
    const ULONGLONG ms = GetTickCount64();
    const qtelist::List& list = qtelist::Settle(g_steady, now, ms);
    // A fist fight's open prompt from its gates, and every other trigger's keys as before (a fight's six left out of them, read or not).
    const qtefight::Frame f = qtefight::Step(g_watch, read, g_fightTypes, list, scripted, ms, fights && g_proved == Proved::kYes);
    SayFightEvents(f, fights);
    SayChanges(list, scripted, f.skip);
    frame.await = f.await;
    frame.hold = f.hold;
    frame.prompt = f.prompt;
    frame.answer = f.answer;
    frame.mash = f.mash;
    frame.unknown = f.unknown;
    frame.read = list.status == qtelist::Status::kOk || list.status == qtelist::Status::kNone;
    if (list.status == qtelist::Status::kOk) {  // (also with nothing awaited: the frame a trigger goes, its last press may show)
        const qtelist::Input input = qtelist::ReadInput(read, g_base + qtelist::kGameContext);
        frame.seen = qtelist::SeenKeys(input);
        frame.seenRead = input.read;
    }
    SayPrompts(f, frame.seen, frame.seenRead, ms, scripted);
    return frame;
}

}  // namespace qtewatch

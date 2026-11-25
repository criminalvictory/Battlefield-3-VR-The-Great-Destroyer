#pragma once
// The game's fist fights' quick-time events: the one key an open prompt awaits (build\designs\qte_fights.md, 2026-11-18; bf3.exe 1.6.0.0).
// All 13 fights are one prefab, Animations/PreFabs/QuickTimePrefab: its six quick-time triggers (E, the left and the right mouse button, F,
// Space, Z) are live for the whole fight and a key the prompt doesn't name FAILS it, so version 2 (every live trigger's key, qte.h) can't
// answer one, and three fights are never stopped (their six triggers live on in free play). The prefab's own gates say what to press: for each
// action a "right" EventGate (a press of it goes through to done) and a "wrong" one (a press of any OTHER quick-time action goes through it to
// Failed); both open as the animation raises that action's prompt, both shut as it lowers it, all twelve shut between prompts. So a key is
// pressed only while its right gate is the one open and no other action's wrong gate is; its SelectEvent says a press or a mash (a CountDown
// of 10 presses).
// The way to the gates, once a fight: a live trigger's +0x0C is its prefab's EntityBus (the connected bus's vtable) -> +0x30 the compiled info
// -> +0x68 the blueprint (a LogicPrefabBlueprint, QuickTimePrefab's 103 objects) -> Objects[i], each gate's data -> its place in the info's
// sorted client target table (+0x6C/+0x70) = its place in the bus's entity array (+0x1C): the game's own lookup (the bus's getEntity,
// 0x00551870). The attach proves itself: the table hands back the very triggers the list gave. Then each frame one read of the bus and 15 of
// 0x18 bytes (the 12 gates, the select, the count, the fail gate), every object checked by its vtable, its bus and its data; anything not as
// attached: nothing pressed that frame, attached again. Open gates are a prompt only while the fight runs them: its fail gate open (shut once it's
// won or failed) and the same state not standing unchanged for a minute (Track): a never-stopped fight's gates left open after it hold nothing up.
// Every constant is borne out at startup by the game's code bytes or type names (Prove); one that isn't turns the reader off for the run.
// Never a guess: a fight that can't be read presses nothing (every bus with all six quick-time actions live is left out of version 2's keys:
// qtelist::Awaited), never all six. Plain heap and image memory only, nothing the game mapped from the graphics card (AMD-safe).
// Pure: memory is read through the caller's reader (qtewatch.cpp's guarded one; a made-up image in tools/qtetest.cpp; ReadProcessMemory in
// tools/qtepeek.cpp), never directly.

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "qte.h"
#include "qtelist.h"

namespace qtefight {

// ------------------------------------------------- the constants (bf3.exe's image base taken off) -------------------------------------------------
constexpr uint32_t kGateType = 0x1C7ACA0;        // EventGateEntity's vtable
constexpr uint32_t kSelectType = 0x1C7A840;      // SelectEventEntity's
constexpr uint32_t kCountType = 0x1C7A640;       // CountDownEntity's
constexpr uint32_t kBusType = 0x1C793A8;         // the connected EntityBus's (0x3C bytes: a blueprint with connections; its getType gives 0)
constexpr uint32_t kGateDataType = 0x1C77658;    // EventGateEntityData's
constexpr uint32_t kSelectDataType = 0x1C77CF8;  // SelectEventEntityData's
constexpr uint32_t kCountDataType = 0x1C773D4;   // CountDownEntityData's
constexpr uint32_t kPrefabType = 0x1C77DBC;      // LogicPrefabBlueprint's
constexpr uint32_t kRegistryGlobal = 0x1F4F4EC;  // the client's bus registry, by network id (tools/qtepeek.cpp's walk only)
// The code that bears them out.
constexpr uint32_t kGateEvent = 0x1621D0;     // EventGateEntity's event handler (its vtable's slot 4): Open writes 1 at +0x14, Close 0
constexpr uint32_t kGateCtor = 0x168E4A;      // its constructor: the vtable, +0x14 the data's Default
constexpr uint32_t kSelectEvent = 0x171670;   // SelectEventEntity's handler
constexpr uint32_t kSelectWrite = 0x171756;   // ... its write of the selected event's index at +0x14
constexpr uint32_t kCountEvent = 0x161170;    // CountDownEntity's handler: CountDownOne takes one off +0x14
constexpr uint32_t kCountCtor = 0x1672CA;     // its constructor: the vtable, +0x14 the data's StartValue
constexpr uint32_t kBusCtor = 0x1551E5;       // the connected bus's constructor's vtable write
constexpr uint32_t kBusGetEntity = 0x151870;  // its getEntity (vtable slot 16): +0x14 the realm, +0x30 the info, +0x6C the table, +0x1C the entities
constexpr uint32_t kInfoCtor = 0x153AE5;      // the compiled info's constructor: +0x68 the blueprint, +0x6C/+0x70 the client target table
constexpr uint32_t kTriggerCtor = 0xBCCBB9;   // the input trigger's constructor: +0x0C its bus, +0x10 its data
constexpr uint32_t kRegistryCode = 0x1527C5;  // code reading the bus registry's pointer (at +2)

constexpr const char* kGateEventBytes =
    "8B 54 24 04 8B 42 04 3D 02 73 59 00 74 ?? 3D 53 88 AC 0C 74 ?? 3D B1 BE 83 7C 75 ?? C6 41 14 01 C2 04 00 C6 41 14 00 C2 04 00";
constexpr const char* kGateCtorBytes = "C7 06 ?? ?? ?? ?? 8A 4F 10 5F 88 4E 14 8B C6";
constexpr const char* kSelectEventBytes = "83 EC 30 53 55 56 57 8B 7C 24 44 81 7F 04 02 73 59 00 8B D9 75 ?? 8B 73 10 8B 43 14 8B 56 10 8B 14 82";
constexpr const char* kSelectWriteBytes = "89 73 14 E8 ?? ?? ?? ?? 5F 5E 5D 5B 83 C4 30 C2 04 00";
constexpr const char* kCountEventBytes = "8B 44 24 04 8B 40 04 56 8B F1 3D 30 B3 B4 0D 74 ?? 3D D0 1C DF 6D 75 ?? 83 46 14 FF";
constexpr const char* kCountCtorBytes = "C7 06 ?? ?? ?? ?? 8B 4F 0C 5F 89 4E 14 8B C6";
constexpr const char* kBusCtorBytes = "C7 06 ?? ?? ?? ?? 89 5E 18 85 D0";
constexpr const char* kBusGetEntityBytes =
    "53 55 8B 6C 24 0C 8B D9 85 ED 75 07 5D 33 C0 5B C2 08 00 F7 43 14 00 00 00 60 8B 43 30 75 05 83 C0 6C EB 03 83 E8 80 56 8B 70 04 57 8B 38 0F "
    "B6 40 10 50 8D 4C 24 18 51 56 57 E8 ?? ?? ?? ?? 83 C4 10 3B C6 74 0B 3B 28 72 07 8D 48 04 3B C1 75 02 8B C6 3B C6 75 09 5F 5E 5D 33 C0 5B C2 "
    "08 00 8B 53 1C 2B C7 5F 5E C1 F8 02 8B 04 82";
constexpr const char* kInfoCtorBytes = "8B 4C 24 14 8B 51 04 89 56 68 33 D2 89 56 6C 89 56 70 89 56 74 89 6E 78";
constexpr const char* kTriggerCtorBytes = "89 46 0C 89 6E 10 8B 47 0C";
constexpr const char* kRegistryBytes = "8B 0D ?? ?? ?? ?? 8B 11 C7 44 82 FC 00 00 00 00";

// The layouts.
constexpr uint32_t kEntityBus = 0x0C, kEntityData = 0x10, kEntityState = 0x14, kEntityRead = 0x18;  // every entity: its bus, its data, its own
constexpr uint32_t kBusFlags = 0x14, kBusEntities = 0x1C, kBusInfo = 0x30, kBusRead = 0x34;         // bits 29-30 of the flags the realm (0 client)
constexpr uint32_t kInfoBlueprint = 0x68;  // then +0x6C/+0x70 the client target table: DataContainer pointers sorted by address, each once
constexpr uint32_t kBlueprintName = 0x08, kBlueprintObjects = 0x20, kBlueprintRead = 0x24;  // the objects' count at [Objects - 4]
constexpr uint32_t kDataIndex = 0x08;      // GameObjectData's IndexInBlueprint (16 bits)
constexpr uint32_t kGateDefault = 0x10;    // EventGateEntityData's Default (a byte); its data 0x14 bytes
constexpr uint32_t kCountStart = 0x0C;     // CountDownEntityData's StartValue; its data 0x18 bytes
constexpr uint32_t kDataRead = 0x14;
constexpr int kMostTargets = 512;
constexpr char kPrefabName[] = "Animations/PreFabs/QuickTimePrefab";

// The prefab (its 103 objects, every one at its own IndexInBlueprint): for each quick-time action, in the order of its key's bit (qte::Key: bit a
// is kActions[a]'s), its trigger's data and its two gates.
constexpr int kObjects = 103;
struct ActionGates {
    uint32_t action;
    int trigger, right, wrong;
};
constexpr ActionGates kActions[6] = {{0x3C, 52, 7, 15},  {0x3D, 53, 8, 19},  {0x3E, 50, 28, 29},
                                     {0x3F, 51, 31, 32}, {0x40, 54, 59, 58}, {0x41, 55, 62, 61}};
constexpr int kFirstTrigger = 50;  // the six triggers' data, 50-55: Block, FastMelee, InteractDrag, Fire, JumpClimb, CrouchDuck
constexpr uint32_t kTriggerActions[6] = {0x3E, 0x3F, 0x3C, 0x3D, 0x40, 0x41};
constexpr int kSelect = 98;  // SelectEventEntity: +0x14 0 Press (one press finishes the prompt), 1 Mash (the count's presses)
constexpr int kCount = 75;   // CountDownEntity: +0x14 the presses a mash still wants (from 10)
// The fail gate (Default True): its Out is the prefab's Failed. Open while the fight runs (opened again as each prompt opens, #84), shut by the
// fight's success (the animation's success signal, #1 -> #71) and by its own Out as it fails the fight (#97): with it shut no key can fail the fight,
// so a fight's open gates are its prompt only while it reads 1 (Track).
constexpr int kFail = 85;

// The key of action a (its bit), and a key's action index (-1: none, or more than one key).
inline unsigned KeyOfIndex(int a) {
    return a >= 0 && a < 6 ? 1u << a : 0u;
}
inline int IndexOfKey(unsigned key) {
    for (int a = 0; a < 6; ++a)
        if (key == (1u << a))
            return a;
    return -1;
}
// The action's name of a key (a single one), as qtelist says it ("Block"; "the quick-time Fire").
inline const char* ActionOfKey(unsigned key) {
    const int a = IndexOfKey(key);
    return a >= 0 ? qtelist::ActionName(kActions[a].action) : "nothing";
}

// The absolute vtables (the module's base added).
struct Types {
    uint32_t gate = 0, select = 0, count = 0, bus = 0, gateData = 0, selectData = 0, countData = 0, prefab = 0, trigger = 0, triggerData = 0;
};
inline Types TypesAt(uint32_t base) {
    Types t;
    t.gate = base + kGateType;
    t.select = base + kSelectType;
    t.count = base + kCountType;
    t.bus = base + kBusType;
    t.gateData = base + kGateDataType;
    t.selectData = base + kSelectDataType;
    t.countData = base + kCountDataType;
    t.prefab = base + kPrefabType;
    t.trigger = base + qtelist::kEntityType;
    t.triggerData = base + qtelist::kDataType;
    return t;
}

// ------------------------------------------------------------- reading the image -------------------------------------------------------------

// A pattern's bytes at an address ("8B 54 ?? 04": ?? any byte; at most 128 bytes).
template <class Reader>
bool Matches(const Reader& read, uint32_t at, const char* pattern) {
    uint8_t want[128], mask[128], got[128];
    int n = 0;
    const auto hex = [](char c) -> int {
        return c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10 : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
    };
    for (const char* p = pattern; *p;) {
        if (*p == ' ') {
            ++p;
            continue;
        }
        if (n == 128 || !p[1])
            return false;
        if (p[0] == '?' && p[1] == '?') {
            want[n] = 0, mask[n++] = 0;
        } else {
            const int hi = hex(p[0]), lo = hex(p[1]);
            if (hi < 0 || lo < 0)
                return false;
            want[n] = static_cast<uint8_t>(hi * 16 + lo), mask[n++] = 0xFF;
        }
        p += 2;
    }
    if (!n || !read(at, got, static_cast<size_t>(n)))
        return false;
    for (int i = 0; i < n; ++i)
        if ((got[i] & mask[i]) != want[i])
            return false;
    return true;
}

// Letters at an address up to their end (at most size - 1 of them): a piece at a time, so letters ending just before memory that can't be read
// are still read. False: unreadable, or no end within size - 1.
template <class Reader>
bool ReadText(const Reader& read, uint32_t at, char* text, int size) {
    if (size < 1)
        return false;
    text[0] = 0;
    if (at < 0x10000 || at >= 0xFFFF0000u)
        return false;
    int n = 0;
    while (n < size - 1) {
        char piece[16];
        int take = 16;
        if (!read(at + static_cast<uint32_t>(n), piece, sizeof(piece))) {
            take = 1;
            if (!read(at + static_cast<uint32_t>(n), piece, 1))
                return false;
        }
        for (int i = 0; i < take && n < size - 1; ++i) {
            text[n] = piece[i];
            if (!piece[i])
                return true;
            ++n;
        }
        text[n] = 0;
    }
    return false;
}

// The name of the type a vtable's getType gives (its slot 0: mov eax, <ClassInfo>; ret; [ClassInfo] its ClassInfoData, whose +0 is the name).
template <class Reader>
bool TypeName(const Reader& read, uint32_t vtable, char* name, int size) {
    uint32_t getType = 0, info = 0, data = 0, text = 0;
    uint8_t code[6] = {};
    if (size > 0)
        name[0] = 0;
    if (!read(vtable, &getType, 4) || !read(getType, code, 6) || code[0] != 0xB8 || code[5] != 0xC3)
        return false;
    std::memcpy(&info, code + 1, 4);
    return read(info, &data, 4) && read(data, &text, 4) && ReadText(read, text, name, size);
}

template <class Reader>
uint32_t Word(const Reader& read, uint32_t at, bool* ok = nullptr) {
    uint32_t v = 0;
    const bool got = read(at, &v, 4);
    if (ok)
        *ok = got;
    return got ? v : 0;
}

// ------------------------------------------------------------------ the proofs ------------------------------------------------------------------

constexpr int kProofs = 18;
inline const char* ProofName(int i) {
    static const char* const kNames[kProofs + 1] = {
        "EventGateEntity's vtable (by its type's name)",
        "EventGateEntity's event handler (its vtable's slot 4: Open writes 1 at +0x14, Close 0)",
        "EventGateEntity's constructor (its vtable, +0x14 the data's Default)",
        "SelectEventEntity's vtable (by its type's name)",
        "SelectEventEntity's event handler (its vtable's slot 4, +0x14 the selected event's index)",
        "CountDownEntity's vtable (by its type's name)",
        "CountDownEntity's event handler (its vtable's slot 4: CountDownOne takes one off +0x14)",
        "CountDownEntity's constructor (its vtable, +0x14 the data's StartValue)",
        "the connected EntityBus's constructor (its vtable)",
        "the bus's getEntity (its vtable's slot 16: +0x14 the realm, +0x30 the info, +0x6C the target table, +0x1C the entities)",
        "the compiled info's constructor (+0x68 the blueprint, +0x6C/+0x70 the target table)",
        "an entity's +0x0C bus and +0x10 data (the input trigger's constructor)",
        "EventGateEntityData's vtable (by its type's name)",
        "SelectEventEntityData's vtable (by its type's name)",
        "CountDownEntityData's vtable (by its type's name)",
        "LogicPrefabBlueprint's vtable (by its type's name)",
        "PlayerInputTriggerEntityData's vtable (by its type's name)",
        "ClientPlayerInputTriggerEntity's vtable (by its type's name)",
        "the client bus registry's pointer (the code reading it)",
    };
    return i >= 0 && i <= kProofs ? kNames[i] : "?";
}

struct Proofs {
    bool ok = false;
    int failed = -1;      // the first proof not borne out (ProofName)
    uint32_t at = 0;      // where it was looked for
    uint32_t global = 0;  // registry: the bus registry's pointer's address (0: not looked for, or not found)
};

// One proof (ProofName(i)) from the image itself: its code bytes, or the name behind a vtable's getType. at: where it was looked for.
template <class Reader>
bool ProveOne(const Reader& read, uint32_t base, int i, uint32_t* at) {
    const Types t = TypesAt(base);
    char name[48];
    uint32_t where = 0;
    const auto named = [&](uint32_t vtable, const char* want) {
        where = vtable;
        return TypeName(read, vtable, name, sizeof(name)) && std::strcmp(name, want) == 0;
    };
    const auto slot = [&](uint32_t vtable, int index, uint32_t function, const char* pattern) {
        where = vtable;
        bool got = false;
        return Word(read, vtable + 4u * static_cast<uint32_t>(index), &got) == function && got && Matches(read, function, pattern);
    };
    const auto ctor = [&](uint32_t code, const char* pattern, uint32_t vtable) {
        where = code;
        bool got = false;
        return Matches(read, code, pattern) && Word(read, code + 2, &got) == vtable && got;
    };
    const auto bytes = [&](uint32_t code, const char* pattern) {
        where = code;
        return Matches(read, code, pattern);
    };
    bool ok = false;
    switch (i) {
    case 0: ok = named(t.gate, "EventGateEntity"); break;
    case 1: ok = slot(t.gate, 4, base + kGateEvent, kGateEventBytes); break;
    case 2: ok = ctor(base + kGateCtor, kGateCtorBytes, t.gate); break;
    case 3: ok = named(t.select, "SelectEventEntity"); break;
    case 4: ok = slot(t.select, 4, base + kSelectEvent, kSelectEventBytes) && bytes(base + kSelectWrite, kSelectWriteBytes); break;
    case 5: ok = named(t.count, "CountDownEntity"); break;
    case 6: ok = slot(t.count, 4, base + kCountEvent, kCountEventBytes); break;
    case 7: ok = ctor(base + kCountCtor, kCountCtorBytes, t.count); break;
    case 8: ok = ctor(base + kBusCtor, kBusCtorBytes, t.bus); break;
    case 9: ok = slot(t.bus, 16, base + kBusGetEntity, kBusGetEntityBytes); break;
    case 10: ok = bytes(base + kInfoCtor, kInfoCtorBytes); break;
    case 11: ok = bytes(base + kTriggerCtor, kTriggerCtorBytes); break;
    case 12: ok = named(t.gateData, "EventGateEntityData"); break;
    case 13: ok = named(t.selectData, "SelectEventEntityData"); break;
    case 14: ok = named(t.countData, "CountDownEntityData"); break;
    case 15: ok = named(t.prefab, "LogicPrefabBlueprint"); break;
    case 16: ok = named(t.triggerData, "PlayerInputTriggerEntityData"); break;
    case 17: ok = named(t.trigger, "ClientPlayerInputTriggerEntity"); break;
    case 18: {
        where = base + kRegistryCode;
        bool got = false;
        ok = Matches(read, where, kRegistryBytes) && Word(read, where + 2, &got) == base + kRegistryGlobal && got;
        break;
    }
    default: break;
    }
    if (at)
        *at = where;
    return ok;
}

// Every constant above, in order; the first not borne out named. registry: the bus registry's pointer as well (tools/qtepeek.cpp's walk; not
// one of the reader's).
template <class Reader>
Proofs Prove(const Reader& read, uint32_t base, bool registry = false) {
    Proofs p;
    for (int i = 0; i < kProofs + (registry ? 1 : 0); ++i) {
        uint32_t at = 0;
        if (!ProveOne(read, base, i, &at)) {
            p.failed = i;
            p.at = at;
            return p;
        }
    }
    if (registry)
        p.global = base + kRegistryGlobal;
    p.ok = true;
    return p;
}

// --------------------------------------------------------------- a fight's bus ---------------------------------------------------------------

// The checks of an attach (design 3.2), each named when it fails.
enum class Check : uint8_t {
    kNone,
    kBus,        // the bus not readable, or not the connected bus's vtable
    kRealm,      // ... not the client's
    kInfo,       // its compiled info, or the info's blueprint, not readable
    kBlueprint,  // the blueprint isn't a LogicPrefabBlueprint
    kName,       // its name readable and another's
    kObjects,    // its objects not readable, or not 103 of them
    kTriggers,   // the six trigger data (50-55) not as the prefab has them (type, action, index)
    kOwnData,    // a live trigger's data isn't one of them
    kTable,      // the info's target table: not readable, empty, too big, ragged, or not sorted
    kEntities,   // the bus's entity array not readable
    kOwnEntry,   // the table doesn't hand back a live trigger for its own data: not the way the game delivers its events
    kGate,       // a gate's data or entity not as the prefab has it
    kSelect,     // the press-or-mash select's
    kCount,      // the mash's count's
    kState,      // an object's state not as each frame reads it (a gate's byte not 0 or 1, the select not 0 or 1), or the bus changed as it was read
};
// The checks that say the bus is some other blueprint's (the single quick-time events' buses: answered as before). Three of them also fail on a
// read that didn't go (kBus, kObjects, kTriggers): the watch tries such a bus again every kRetryMs while all six quick-time actions are live on it.
inline bool NotThePrefab(Check c) {
    return c == Check::kBus || c == Check::kRealm || c == Check::kBlueprint || c == Check::kName || c == Check::kObjects || c == Check::kTriggers ||
           c == Check::kOwnData;
}
// The checks past the prefab's own (its blueprint, its 103 objects and its six trigger data all borne out): failing one, the bus is still a fight's,
// its triggers kept out of version 2's keys however few of them the list shows.
inline bool ThePrefab(Check c) {
    return c == Check::kTable || c == Check::kEntities || c == Check::kOwnEntry || c == Check::kGate || c == Check::kSelect || c == Check::kCount ||
           c == Check::kState;
}
inline const char* CheckName(Check c) {
    switch (c) {
    case Check::kNone: return "every check passed";
    case Check::kBus: return "not the connected EntityBus (its vtable), or not readable";
    case Check::kRealm: return "not the client's bus (its realm)";
    case Check::kInfo: return "its compiled info or the info's blueprint not readable";
    case Check::kBlueprint: return "its blueprint isn't a LogicPrefabBlueprint";
    case Check::kName: return "its blueprint is another's (by its name)";
    case Check::kObjects: return "its blueprint's objects not 103, or not readable";
    case Check::kTriggers: return "its objects 50-55 aren't the prefab's six trigger data";
    case Check::kOwnData: return "a live trigger's data isn't one of the prefab's six";
    case Check::kTable: return "the info's target table not readable or not as the game keeps it (sorted, 1-512 entries)";
    case Check::kEntities: return "the bus's entity array not readable";
    case Check::kOwnEntry: return "the bus's own entry for a live trigger's data isn't that trigger";
    case Check::kGate: return "a gate's data or entity isn't as the prefab has it";
    case Check::kSelect: return "the press-or-mash select's data or entity isn't as the prefab has it";
    case Check::kCount: return "the mash's count's data or entity isn't as the prefab has it";
    case Check::kState: return "an object's state isn't as each frame reads it (a gate's byte not 0 or 1, the select not 0 or 1)";
    }
    return "?";
}

// A bus's live quick-time triggers (as the list gave them), sorted by entity.
constexpr int kMostLive = 8;
struct Live {
    int count = 0;
    bool more = false;  // more than kMostLive of them (never the prefab's)
    uint32_t entity[kMostLive] = {}, data[kMostLive] = {}, action[kMostLive] = {};
    unsigned keys = 0;  // their actions' keys
    void Add(uint32_t e, uint32_t d, uint32_t a) {
        keys |= qtelist::KeyOf(a);
        if (count == kMostLive) {
            more = true;
            return;
        }
        int at = count++;
        for (; at > 0 && entity[at - 1] > e; --at)
            entity[at] = entity[at - 1], data[at] = data[at - 1], action[at] = action[at - 1];
        entity[at] = e, data[at] = d, action[at] = a;
    }
    bool Same(const Live& o) const {
        if (count != o.count || more != o.more)
            return false;
        for (int i = 0; i < count; ++i)
            if (entity[i] != o.entity[i] || data[i] != o.data[i] || action[i] != o.action[i])
                return false;
        return true;
    }
};

// A fight's bus as attached: the 14 objects read each frame, each its entity and its data.
struct Fight {
    uint32_t bus = 0, info = 0, blueprint = 0, objects = 0, entities = 0;
    int targets = 0;
    bool named = false;  // its name read (QuickTimePrefab's); false: not readable (the structure's checks are exact on their own)
    uint32_t gate[2][6] = {}, gateData[2][6] = {};  // [0] the right gates, [1] the wrong ones, by action (kActions)
    uint32_t select = 0, selectData = 0, count = 0, countData = 0;
    int countStart = 0;
    uint32_t fail = 0, failData = 0;  // the fail gate, 0 if not as the prefab has it (its prompts then kStaleMs at most each: Track)
    bool SameObjects(const Fight& o) const {
        return bus == o.bus && info == o.info && entities == o.entities && std::memcmp(gate, o.gate, sizeof(gate)) == 0 && select == o.select &&
               count == o.count;
    }
};

struct Attach {
    Check check = Check::kNone;
    int index = -1;           // the object (its index in the blueprint) the failed check is of, -1 none
    uint32_t at = 0, value = 0;  // what was read where (for the line)
    char name[40] = {};       // the blueprint's name as read (kName)
    Fight fight;
};

// Its place in a sorted table, -1: not in it.
inline int Place(const uint32_t* table, int n, uint32_t v) {
    int lo = 0, hi = n;
    while (lo < hi) {
        const int mid = (lo + hi) / 2;
        if (table[mid] < v)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo < n && table[lo] == v ? lo : -1;
}

// ------------------------------------------------------------------ each frame ------------------------------------------------------------------

struct State {
    bool read = false;
    Check check = Check::kNone;  // what didn't read as attached (kBus: the bus itself; kGate/kSelect/kCount: index the object's)
    int index = -1;
    unsigned right = 0, wrong = 0;  // the keys whose right / wrong gate is open
    int select = 0;                 // 0 a press, 1 a mash
    int count = 0;                  // the mash's presses still wanted
    int fail = -1;                  // the fail gate's byte, -1: no fail gate attached
};

// The entity of an attached fight's object (a gate, the select, the count, the fail gate) by its index in the blueprint; 0 for any other.
inline uint32_t EntityOf(const Fight& f, int index) {
    for (int a = 0; a < 6; ++a) {
        if (kActions[a].right == index)
            return f.gate[0][a];
        if (kActions[a].wrong == index)
            return f.gate[1][a];
    }
    return index == kSelect ? f.select : index == kCount ? f.count : index == kFail ? f.fail : 0;
}

// One frame's read of an attached fight (design 3.3): the bus as attached, each of its objects (the 12 gates, the select, the count, the fail gate if
// attached) its entity of its type, made by this bus for its data; a gate's byte 0 or 1, the select 0 or 1. Anything else: not read (nothing is held
// over from a last good read: a stale "open" must never start a press).
template <class Reader>
State ReadState(const Reader& read, const Types& types, const Fight& f) {
    State s;
    const auto fail = [&](Check c, int index) {
        State bad;
        bad.check = c, bad.index = index;
        return bad;
    };
    uint32_t b[kBusRead / 4] = {};
    if (!read(f.bus, b, sizeof(b)) || b[0] != types.bus || b[kBusEntities / 4] != f.entities || b[kBusInfo / 4] != f.info || ((b[kBusFlags / 4] >> 29) & 3))
        return fail(Check::kBus, -1);
    const auto entity = [&](uint32_t at, uint32_t type, uint32_t data, uint32_t& state) {
        uint32_t e[kEntityRead / 4] = {};
        if (!read(at, e, sizeof(e)) || e[0] != type || e[kEntityBus / 4] != f.bus || e[kEntityData / 4] != data)
            return false;
        state = e[kEntityState / 4];
        return true;
    };
    for (int side = 0; side < 2; ++side)
        for (int i = 0; i < 6; ++i) {
            uint32_t v = 0;
            if (!entity(f.gate[side][i], types.gate, f.gateData[side][i], v) || (v & 0xFF) > 1)
                return fail(Check::kGate, side == 0 ? kActions[i].right : kActions[i].wrong);
            if (v & 1)
                (side == 0 ? s.right : s.wrong) |= KeyOfIndex(i);
        }
    uint32_t v = 0;
    if (!entity(f.select, types.select, f.selectData, v) || v > 1)
        return fail(Check::kSelect, kSelect);
    s.select = static_cast<int>(v);
    if (!entity(f.count, types.count, f.countData, v))
        return fail(Check::kCount, kCount);
    s.count = static_cast<int>(v);
    if (f.fail) {
        if (!entity(f.fail, types.gate, f.failData, v) || (v & 0xFF) > 1)
            return fail(Check::kGate, kFail);
        s.fail = static_cast<int>(v & 1);
    }
    s.read = true;
    return s;
}

// Attaches a bus (design 3.2): every check, in order; the first that fails is named. live: its live quick-time triggers (each must be the
// bus's own entity for one of the prefab's six trigger data); none: the checks that need one left out (qtepeek's walk of the registry).
template <class Reader>
Attach AttachBus(const Reader& read, const Types& types, uint32_t bus, const Live& live) {
    Attach a;
    Fight& f = a.fight;
    f.bus = bus;
    const auto fail = [&](Check c, int index = -1, uint32_t at = 0, uint32_t value = 0) {
        a.check = c, a.index = index, a.at = at, a.value = value;
        return a;
    };
    uint32_t b[kBusRead / 4] = {};
    if (!qtelist::Pointer(bus) || !read(bus, b, sizeof(b)))
        return fail(Check::kBus, -1, bus);
    if (b[0] != types.bus)
        return fail(Check::kBus, -1, bus, b[0]);
    if ((b[kBusFlags / 4] >> 29) & 3)
        return fail(Check::kRealm, -1, bus + kBusFlags, b[kBusFlags / 4]);
    f.info = b[kBusInfo / 4];
    f.entities = b[kBusEntities / 4];
    uint32_t info[3] = {};  // +0x68 the blueprint, +0x6C/+0x70 the table
    if (!qtelist::Pointer(f.info) || !read(f.info + kInfoBlueprint, info, sizeof(info)) || !qtelist::Pointer(info[0]))
        return fail(Check::kInfo, -1, f.info, info[0]);
    f.blueprint = info[0];
    uint32_t bp[kBlueprintRead / 4] = {};
    if (!read(f.blueprint, bp, sizeof(bp)))
        return fail(Check::kInfo, -1, f.blueprint);
    if (bp[0] != types.prefab)
        return fail(Check::kBlueprint, -1, f.blueprint, bp[0]);
    // Its name: read whole and the prefab's (no case), else another's; not readable, the structure below says it.
    char name[sizeof(kPrefabName)] = {};
    const uint32_t nameAt = bp[kBlueprintName / 4];
    if (nameAt >= 0x10000 && nameAt < 0xFFFF0000u && read(nameAt, name, sizeof(name))) {
        bool same = name[sizeof(name) - 1] == 0;
        for (size_t i = 0; same && i + 1 < sizeof(name); ++i) {
            const char c = name[i] >= 'a' && name[i] <= 'z' ? static_cast<char>(name[i] - 32) : name[i];
            const char w = kPrefabName[i] >= 'a' && kPrefabName[i] <= 'z' ? static_cast<char>(kPrefabName[i] - 32) : kPrefabName[i];
            same = c == w;
        }
        if (!same) {
            size_t i = 0;
            for (; i + 1 < sizeof(a.name) && i < sizeof(name) && name[i]; ++i)
                a.name[i] = name[i] >= 32 && name[i] < 127 ? name[i] : '?';
            a.name[i] = 0;
            return fail(Check::kName, -1, nameAt);
        }
        f.named = true;
    }
    f.objects = bp[kBlueprintObjects / 4];
    uint32_t count = 0;
    if (!qtelist::Pointer(f.objects) || !read(f.objects - 4, &count, 4) || count != static_cast<uint32_t>(kObjects))
        return fail(Check::kObjects, -1, f.objects, count);
    uint32_t objs[kObjects] = {};
    if (!read(f.objects, objs, sizeof(objs)))
        return fail(Check::kObjects, -1, f.objects, count);
    for (int i = 0; i < 6; ++i) {
        const int idx = kFirstTrigger + i;
        uint32_t d[kDataRead / 4] = {};
        if (!qtelist::Pointer(objs[idx]) || !read(objs[idx], d, sizeof(d)))
            return fail(Check::kTriggers, idx, objs[idx]);
        if (d[0] != types.triggerData)
            return fail(Check::kTriggers, idx, objs[idx], d[0]);
        if (d[qtelist::kDataAction / 4] != kTriggerActions[i] || (d[kDataIndex / 4] & 0xFFFF) != static_cast<uint32_t>(idx))
            return fail(Check::kTriggers, idx, objs[idx], d[qtelist::kDataAction / 4]);
    }
    if (live.more)
        return fail(Check::kOwnData, -1, bus, static_cast<uint32_t>(live.count));
    for (int j = 0; j < live.count; ++j) {
        bool one = false;
        for (int i = 0; i < 6 && !one; ++i)
            one = objs[kFirstTrigger + i] == live.data[j] && kTriggerActions[i] == live.action[j];
        if (!one)
            return fail(Check::kOwnData, -1, live.entity[j], live.data[j]);
    }
    // The info's client target table and the bus's entities, side by side.
    const uint32_t tb = info[1], te = info[2];
    if (!qtelist::Pointer(tb) || te < tb || ((te - tb) & 3) || (te - tb) / 4 < 1 || (te - tb) / 4 > static_cast<uint32_t>(kMostTargets))
        return fail(Check::kTable, -1, f.info + kInfoBlueprint + 4, te >= tb ? (te - tb) / 4 : 0);
    const int n = static_cast<int>((te - tb) / 4);
    f.targets = n;
    uint32_t table[kMostTargets], ents[kMostTargets];
    if (!read(tb, table, static_cast<size_t>(n) * 4))
        return fail(Check::kTable, -1, tb, static_cast<uint32_t>(n));
    for (int k = 1; k < n; ++k)
        if (table[k - 1] >= table[k])
            return fail(Check::kTable, k, tb + 4u * static_cast<uint32_t>(k), table[k]);
    if (!qtelist::Pointer(f.entities) || !read(f.entities, ents, static_cast<size_t>(n) * 4))
        return fail(Check::kEntities, -1, f.entities, static_cast<uint32_t>(n));
    // The way proven: the bus's own entry for each live trigger's data is that very trigger.
    for (int j = 0; j < live.count; ++j) {
        const int k = Place(table, n, live.data[j]);
        if (k < 0 || ents[k] != live.entity[j])
            return fail(Check::kOwnEntry, -1, live.entity[j], k < 0 ? 0 : ents[k]);
    }
    // An object: its data of that type at its own index (a gate's Default as the prefab has it, def -1 not looked at), and the bus's entity for
    // it of that type, made by this bus for this data.
    const auto object = [&](int idx, uint32_t dataType, uint32_t entityType, int def, uint32_t& entity, uint32_t& data, int* start) -> bool {
        data = objs[idx];
        uint32_t d[0x18 / 4] = {};
        const size_t size = start ? 0x18 : kDataRead;  // (a gate's and a select's data 0x14 bytes, the count's 0x18)
        if (!qtelist::Pointer(data) || !read(data, d, size) || d[0] != dataType || (d[kDataIndex / 4] & 0xFFFF) != static_cast<uint32_t>(idx))
            return false;
        if (def >= 0 && (d[kGateDefault / 4] & 0xFF) != static_cast<uint32_t>(def))
            return false;
        if (start)
            *start = static_cast<int>(d[kCountStart / 4]);
        const int k = Place(table, n, data);
        if (k < 0 || !qtelist::Pointer(ents[k]))
            return false;
        entity = ents[k];
        uint32_t e[kEntityRead / 4] = {};
        return read(entity, e, sizeof(e)) && e[0] == entityType && e[kEntityBus / 4] == bus && e[kEntityData / 4] == data;
    };
    for (int side = 0; side < 2; ++side)
        for (int i = 0; i < 6; ++i) {
            const int idx = side == 0 ? kActions[i].right : kActions[i].wrong;
            if (!object(idx, types.gateData, types.gate, 0, f.gate[side][i], f.gateData[side][i], nullptr))
                return fail(Check::kGate, idx, objs[idx]);
        }
    if (!object(kSelect, types.selectData, types.select, -1, f.select, f.selectData, nullptr))
        return fail(Check::kSelect, kSelect, objs[kSelect]);
    if (!object(kCount, types.countData, types.count, -1, f.count, f.countData, &f.countStart))
        return fail(Check::kCount, kCount, objs[kCount]);
    if (!object(kFail, types.gateData, types.gate, 1, f.fail, f.failData, nullptr))
        f.fail = f.failData = 0;  // (without it each prompt counts kStaleMs at most: Track)
    // The state as each frame will read it: an attach that wouldn't read (a gate's byte not 0 or 1, the select not 0 or 1) is none: tried again
    // every kRetryMs and said once, never attached and lost frame after frame.
    const State s = ReadState(read, types, f);
    if (!s.read) {
        const uint32_t e = EntityOf(f, s.index);
        uint32_t v = 0;
        if (e)
            read(e + kEntityState, &v, 4);
        return fail(Check::kState, s.index, e ? e : bus, v);
    }
    return a;
}

// What the fights' gates say this frame (design 1, the answer rule): R the keys whose right gate is open, W those whose wrong gate is, over
// every fight read. Answer X only if R is X alone and W holds nothing but X: a press of X then reaches X's right gate(s) and only shut wrong
// ones. R empty and W empty: no prompt. Otherwise (two prompts, a wrong gate with no prompt or another action's): press nothing, a moment
// while the game changes prompts, or a fight already failed.
enum class Why : uint8_t { kNone, kAnswer, kTwo, kWrongAlone, kWrongOther };
struct Answer {
    Why why = Why::kNone;
    unsigned right = 0, wrong = 0;
    unsigned key = 0;   // the one key to press (kAnswer), else 0
    bool mash = false;  // ... a mash (the select of a fight whose right gate for it is open)
    int left = 0;       // ... its presses still wanted (for the log: at zero the count is reloaded, and the right gate stays open till the
                        // animation lowers the prompt, so the mash's taps go on till then, each a harmless press of the open right gate)
    bool Prompt() const { return (right | wrong) != 0; }
};
inline Answer Decide(const State* states, int n) {
    Answer a;
    for (int i = 0; i < n; ++i)
        if (states[i].read)
            a.right |= states[i].right, a.wrong |= states[i].wrong;
    if (!a.Prompt())
        return a;
    if (!a.right) {
        a.why = Why::kWrongAlone;
        return a;
    }
    if (IndexOfKey(a.right) < 0) {
        a.why = Why::kTwo;
        return a;
    }
    if (a.wrong & ~a.right) {
        a.why = Why::kWrongOther;
        return a;
    }
    a.why = Why::kAnswer;
    a.key = a.right;
    bool first = true;
    for (int i = 0; i < n; ++i)
        if (states[i].read && (states[i].right & a.key)) {
            a.mash = a.mash || states[i].select == 1;
            if (first || states[i].select == 1)
                a.left = states[i].count;
            first = false;
        }
    return a;
}

// ------------------------------------------------------- a prompt only while the fight runs -------------------------------------------------------
// (Both reviews of 2026-11-18.) A fight's gates open with nothing running it (a never-stopped fight's last beat left up after it was won, or its
// signal raised again by another animation) would be a prompt for the rest of the level: every other quick-time key the mod sends held up (the
// trigger's fire, jump, use) and its key pressed by any fresh squeeze of a grip. So a fight's open gates are its prompt only while its fail gate is
// open (shut once it's won or failed: no key can fail it then); and an open state that hasn't changed (the same gates, the same fail gate) for
// kStaleArmedMs of the step's own time, kStaleMs with no fail gate attached, is taken as over, said once, till its gates or its fail gate change. Each
// step counts kStaleStepMs at most, so time the step didn't run (the headset off, bf3 not in front) isn't counted. A real window is a few seconds
// (runbook step 2 measures them), but a beat may wait for its key with no timeout (the train's single quick-time events have none): a minute of the
// headset on with no squeeze before an armed prompt is let be.
constexpr unsigned long long kStaleMs = 10000, kStaleArmedMs = 60000, kStaleStepMs = 100;
constexpr unsigned long long kDisarmedSayMs = 250;  // (open with the fail gate shut that long: said; a moment between a beat's writes isn't)
struct Open {
    unsigned right = 0, wrong = 0;  // the gates as last read
    int fail = -1;                  // ... and the fail gate
    unsigned long long ms = 0;      // how long the step has seen them so (kStaleStepMs a step at most)
    unsigned long long lastMs = 0;  // the last step's time
    bool seen = false;              // ... there was one
    bool stale = false;             // taken as over (said)
    bool saidDisarmed = false;      // open with the fail gate shut (said)
};
struct Tracked {
    bool counts = false;    // this frame's open gates are the fight's prompt
    bool stale = false;     // ... open, unchanged too long: not
    bool disarmed = false;  // ... open with the fail gate shut (the fight won or failed): not
    bool sayStale = false, sayDisarmed = false;  // the frame each is to be said (once till the gates or the fail gate change)
};
inline Tracked Track(Open& o, const State& s, unsigned long long nowMs) {
    Tracked t;
    const unsigned long long step = !o.seen || nowMs < o.lastMs ? 0 : nowMs - o.lastMs < kStaleStepMs ? nowMs - o.lastMs : kStaleStepMs;
    o.seen = true;
    o.lastMs = nowMs;
    const bool open = (s.right | s.wrong) != 0;
    if (!open || s.right != o.right || s.wrong != o.wrong || s.fail != o.fail) {
        o.right = s.right, o.wrong = s.wrong, o.fail = s.fail;
        o.ms = 0;
        o.stale = o.saidDisarmed = false;
    } else {
        o.ms += step;
    }
    if (!open)
        return t;
    if (s.fail == 0) {
        t.disarmed = true;
        if (!o.saidDisarmed && o.ms >= kDisarmedSayMs)
            o.saidDisarmed = t.sayDisarmed = true;
        return t;
    }
    if (!o.stale && o.ms >= (s.fail < 0 ? kStaleMs : kStaleArmedMs))
        o.stale = t.sayStale = true;
    t.stale = o.stale;
    t.counts = !o.stale;
    return t;
}

// --------------------------------------------------------- the watch over the frames ---------------------------------------------------------

// What a bus is to the reader.
enum class Kind : uint8_t {
    kFight,      // attached: its prompts are read from its gates
    kNotPrefab,  // proven another blueprint's: its triggers answered as before (version 2), unless all six quick-time actions are live on it (then
                 // they press nothing, and it's tried again every kRetryMs: a read that didn't go says "not the prefab" too)
    kFailed,     // an attach check failed: tried again every kRetryMs (all six live meanwhile: they press nothing)
    kOff,        // all six live with the reader off (weapon.txt qtefight 0, or a proof not borne out): they press nothing
};
constexpr unsigned long long kRetryMs = 250;
constexpr int kMostBuses = qtelist::Skip::kMostBuses;

struct Known {
    uint32_t bus = 0;
    Kind kind = Kind::kFailed;
    bool proven = false;  // attached once since it came: a fight's, its triggers out of version 2's keys till it goes (an attach failing later included)
    bool prefab = false;  // ... or its blueprint borne out as the prefab's by an attach failing later on (ThePrefab): the same
    Fight fight;
    Live live;  // its live quick-time triggers as last attached
    unsigned long long retryAt = 0;
    Check said = Check::kNone;  // the last failed check said (each once a bus)
    bool saidOff = false, saidNot = false;
    State state;  // this frame's read (a fight's)
    Open open;    // its gates over the frames (Track: a prompt only while the fight runs)
};
struct Watch {
    int count = 0;
    Known known[kMostBuses];
};

// What happened to a bus this frame, for the log (qtewatch.cpp says each as it sees fit).
enum class Ev : uint8_t {
    kAttached,  // attached (again: the same objects as before, after its live triggers changed)
    kNotPrefab, // proven another blueprint's (once a bus)
    kFailed,    // an attach check failed (once a bus and check)
    kLost,      // attached, but this frame's read wasn't as attached: nothing pressed this frame, attached again
    kGone,      // a fight's bus with no live trigger any more (its Stop, a load): forgotten
    kOff,       // all six live with the reader off (once a bus)
    kStale,     // its open gates unchanged too long (Track): taken as over, nothing pressed for them, nothing held up, till they change (once)
    kDisarmed,  // its gates open with its fail gate shut (the fight won or failed): no prompt (once till they change)
};
struct Event {
    Ev what = Ev::kAttached;
    uint32_t bus = 0;
    bool again = false;
    unsigned keys = 0;  // its live quick-time triggers' keys
    int triggers = 0;
    Attach attach;      // kAttached, kNotPrefab, kFailed
    State state;        // kLost; kStale and kDisarmed: the gates as read
};

struct Frame {
    unsigned await = 0, hold = 0;  // the keys the grips press now (qte::In's await and hold)
    bool prompt = false;           // a fight's prompt is up (its right or wrong gate open, the fight running it: Track): the other inputs' quick-time
                                   // keys held up (Guard)
    unsigned answer = 0;           // ... the one key it awaits (0: none safe to press)
    bool mash = false;             // ... a mash, not a press
    int left = 0;                  // ... the presses it still wants
    bool unknown = false;          // a fight attached till now didn't read as attached this frame, or the list wasn't read: nothing pressed at
                                   // all, and the guard left as it was (GuardFrame: it lapses by itself), a prompt may be up
    int fights = 0;                // fights read this frame
    Answer gates;                  // the gates as read (a fight's counted only while it runs its prompt: Track)
    qtelist::Skip skip;            // the fights' buses (their triggers out of version 2's keys)
    int events = 0;
    Event event[kMostBuses];
};

// One frame (qtewatch.cpp's Update, after the list is read and steadied): the list's buses with quick-time triggers attached once each (a bus
// not seen before, or whose live triggers changed; a failed one again after kRetryMs), each fight's gates read, the answer decided; then the
// keys: a fight's prompt up, its answer alone (0 when the rule says nothing); none, version 2's own keys from every other bus's triggers (a
// bus with all six quick-time actions live, and every fight's, left out: qtelist::Awaited). on: the reader on and its proofs borne out.
template <class Reader>
Frame Step(Watch& w, const Reader& read, const Types& types, const qtelist::List& list, bool scripted, unsigned long long nowMs, bool on) {
    Frame out;
    // The list not read (its manager unreadable, or what its pointer leads to not the manager; Settle has given it its 50 ms): nothing pressed, the
    // guard as it was (a prompt may be up), the buses forgotten as before (attached again as the list comes back).
    if (list.status == qtelist::Status::kUnreadable || list.status == qtelist::Status::kWrongType)
        out.unknown = true;
    // This frame's buses with quick-time triggers, and their triggers.
    struct Group {
        uint32_t bus;
        Live live;
    };
    Group groups[qtelist::kMost + qtelist::kMostPending];
    int groupCount = 0;
    if (list.status == qtelist::Status::kOk)
        for (int i = 0; i < list.count; ++i) {
            const qtelist::Trigger& t = list.at[i];
            if (!qtelist::QuickTime(t.action))
                continue;
            int g = 0;
            while (g < groupCount && groups[g].bus != t.bus)
                ++g;
            if (g == groupCount) {
                groups[g].bus = t.bus;
                groups[g].live = Live{};
                ++groupCount;
            }
            groups[g].live.Add(t.entity, t.data, t.action);
        }
    const auto event = [&](Ev what, const Known& k) -> Event& {
        Event& e = out.event[out.events < kMostBuses ? out.events++ : kMostBuses - 1];
        e = Event{};
        e.what = what;
        e.bus = k.bus;
        e.keys = k.live.keys;
        e.triggers = k.live.count;
        return e;
    };
    // Buses gone from the list: forgotten (a fight's said).
    for (int i = 0; i < w.count;) {
        bool live = false;
        for (int g = 0; g < groupCount && !live; ++g)
            live = groups[g].bus == w.known[i].bus;
        if (live) {
            ++i;
            continue;
        }
        if (w.known[i].kind == Kind::kFight)
            event(Ev::kGone, w.known[i]);
        w.known[i] = w.known[--w.count];
    }
    State states[kMostBuses];
    int stateCount = 0;
    for (int g = 0; g < groupCount; ++g) {
        const Group& group = groups[g];
        const bool six = group.live.keys == qte::kKeysV2;
        int at = 0;
        while (at < w.count && w.known[at].bus != group.bus)
            ++at;
        if (at == w.count) {
            if (w.count == kMostBuses)
                continue;  // (more buses than are watched: all six live on one, it presses nothing; fewer, as before)
            w.known[w.count] = Known{};
            w.known[w.count].bus = group.bus;
            w.known[w.count].kind = Kind::kFailed;
            w.known[w.count].live = group.live;
            ++w.count;
        }
        Known& k = w.known[at];
        if (k.proven || k.prefab)
            out.skip.Add(k.bus);
        if (!on) {
            // The reader off: a fight's six press nothing (said once a bus); every other bus as before.
            if (six && !k.saidOff) {
                k.saidOff = true;
                event(Ev::kOff, k);
            }
            k.kind = Kind::kOff;
            k.live = group.live;
            continue;
        }
        const bool changed = !k.live.Same(group.live);
        bool attach = false;
        const bool due = nowMs >= k.retryAt || nowMs + kRetryMs < k.retryAt;  // (a clock gone back: due)
        switch (k.kind) {
        case Kind::kFight: attach = changed; break;
        case Kind::kNotPrefab: attach = changed || (six && due); break;  // (all six live: perhaps a read that didn't go, so again)
        case Kind::kFailed: attach = changed || due; break;
        case Kind::kOff: attach = true; break;
        }
        if (attach) {
            const Attach a = AttachBus(read, types, group.bus, group.live);
            const bool wasFight = k.proven;  // (attached before: after its triggers changed, or a frame not read as attached)
            const Fight before = k.fight;
            k.live = group.live;
            if (a.check == Check::kNone) {
                k.kind = Kind::kFight;
                k.proven = true;
                k.fight = a.fight;
                k.said = Check::kNone;
                Event& e = event(Ev::kAttached, k);
                e.attach = a;
                e.again = wasFight && before.SameObjects(a.fight);
            } else if (NotThePrefab(a.check)) {
                k.kind = Kind::kNotPrefab;
                k.retryAt = nowMs + kRetryMs;
                if (!k.saidNot) {
                    k.saidNot = true;
                    event(Ev::kNotPrefab, k).attach = a;
                }
            } else {
                k.kind = Kind::kFailed;
                k.retryAt = nowMs + kRetryMs;
                if (ThePrefab(a.check)) {  // (its blueprint the prefab's: a fight's, however few of its triggers the list shows)
                    k.prefab = true;
                    out.skip.Add(k.bus);
                }
                if (k.said != a.check) {
                    k.said = a.check;
                    event(Ev::kFailed, k).attach = a;
                }
            }
        }
        if (k.kind != Kind::kFight)
            continue;
        out.skip.Add(k.bus);
        k.state = ReadState(read, types, k.fight);
        if (!k.state.read) {
            event(Ev::kLost, k).state = k.state;
            out.unknown = true;
            k.kind = Kind::kFailed;  // (attached again next frame)
            k.retryAt = 0;
            k.said = Check::kNone;
            continue;
        }
        // Its gates count as its prompt only while the fight runs them (its fail gate open, the state not unchanged too long: Track).
        const Tracked t = Track(k.open, k.state, nowMs);
        if (t.sayStale)
            event(Ev::kStale, k).state = k.state;
        if (t.sayDisarmed)
            event(Ev::kDisarmed, k).state = k.state;
        State counted = k.state;
        if (!t.counts)
            counted.right = counted.wrong = 0;  // (read, no prompt of its own)
        states[stateCount++] = counted;
    }
    out.fights = stateCount;
    out.gates = Decide(states, stateCount);
    const qtelist::Keys keys = qtelist::Awaited(list, scripted, &out.skip);
    if (out.unknown)
        return out;  // (nothing at all: a prompt may be up)
    if (out.gates.Prompt()) {
        out.prompt = true;
        out.answer = out.gates.key;
        out.await = out.gates.key;  // (the prefab's triggers are edge triggers: tapped)
        out.mash = out.gates.mash;
        out.left = out.gates.left;
        return out;
    }
    out.await = keys.await;
    out.hold = keys.hold;
    return out;
}

// ---------------------------------------------------------------- the guard ----------------------------------------------------------------
// While a fight's prompt is up, a key-down of one of the six quick-time keys that isn't its answer must not reach the game, whatever sends it
// (the right trigger's fire, the left's aim, the jump and use buttons, the stick's prone, the grenade launcher's shot...): a wrong one fails the
// fight. input.cpp's SendKey and SendMouseButton ask GuardKey for each; releases always go; outside prompts nothing is held up. Set each frame by
// the grips' step (GuardFrame); one not set again within kGuardMs (the step not running: the headset off; frames the fights couldn't be read) lapses.
constexpr unsigned long long kGuardMs = 250;
constexpr unsigned kSaidStance = 1u << 8;  // (Guard::said: the stick's stance said left undone in this prompt)
struct Guard {
    bool on = false;
    unsigned answer = 0;
    unsigned long long until = 0;
    unsigned said = 0;  // the keys already said held up in this prompt (and kSaidStance)
};
inline void SetGuard(Guard& g, bool prompt, unsigned answer, unsigned long long nowMs) {
    answer &= qte::kKeysV2;
    const bool same = g.on && prompt && answer == g.answer && nowMs < g.until;
    if (!same)
        g.said = 0;
    g.on = prompt;
    g.answer = prompt ? answer : 0;
    g.until = prompt ? nowMs + kGuardMs : 0;
}
// The step's frame: the guard set from it, but on a frame the fights couldn't be read (Frame::unknown) left as it was: a prompt may be up, and it
// lapses by itself kGuardMs on (two reviews of 2026-11-18: the guard was lifted for that frame).
inline void GuardFrame(Guard& g, bool unknown, bool prompt, unsigned answer, unsigned long long nowMs) {
    if (!unknown)
        SetGuard(g, prompt, answer, nowMs);
}
inline bool GuardOn(const Guard& g, unsigned long long nowMs) {
    return g.on && nowMs < g.until;
}
struct Pass {
    bool drop = false;  // held up
    bool say = false;   // ... and the first of that key in this prompt
};
// The guard's keys (qte::Key bits) of the quick-time actions a key is on (keybind.h's QuickTimeOn bits: 1 InteractDrag, 2 FastMelee, 4 JumpClimb,
// 8 CrouchDuck): E, F, Space and Z as the prompts name the actions, whatever key the player has each on.
inline unsigned KeysOfQuickTime(unsigned on) {
    return (on & 1u ? qte::kKeyE : 0u) | (on & 2u ? qte::kKeyF : 0u) | (on & 4u ? qte::kKeySpace : 0u) | (on & 8u ? qte::kKeyZ : 0u);
}
inline Pass GuardKey(Guard& g, unsigned key, bool down, unsigned long long nowMs) {
    Pass p;
    key &= qte::kKeysV2;
    if (!down || !key || !GuardOn(g, nowMs) || (key & g.answer))
        return p;
    p.drop = true;
    p.say = !(g.said & key);
    g.said |= key;
    return p;
}
// A stance change of the right stick's (X its crouch toggle, Z its prone toggle: input.cpp UpdateStance) while a fight's prompt is up: left undone
// whatever the prompt awaits (a scripted fight has no stance, and CrouchDuck's Z is the grips' to press; X may set it too, unmeasured), so the mod's
// stance stays the game's. Said once a prompt.
inline Pass GuardStance(Guard& g, unsigned long long nowMs) {
    Pass p;
    if (!GuardOn(g, nowMs))
        return p;
    p.drop = true;
    p.say = !(g.said & kSaidStance);
    g.said |= kSaidStance;
    return p;
}

// ---------------------------------------------------------------- for the log ----------------------------------------------------------------

// The actions of a mask of keys as words ("the quick-time Fire, Block"; "nothing").
inline const char* ActionNames(unsigned keys, char* text, size_t size) {
    if (!size)
        return "";
    size_t at = 0;
    text[0] = 0;
    const auto add = [&](const char* word) {
        for (; *word && at + 1 < size; ++word)
            text[at++] = *word;
        text[at] = 0;
    };
    for (int a = 0; a < 6; ++a)
        if (keys & KeyOfIndex(a)) {
            if (at)
                add(", ");
            add(qtelist::ActionName(kActions[a].action));
        }
    if (!at)
        add("nothing");
    return text;
}

// A failed attach's check with what was read where ("its blueprint is another's (by its name): \"Gameplay/...\""; "... object 28 at 1A2B3C40").
inline const char* Describe(const Attach& a, char* text, size_t size) {
    if (!size)
        return "";
    int n = std::snprintf(text, size, "%s", CheckName(a.check));
    if (a.check == Check::kName && n >= 0 && static_cast<size_t>(n) < size)
        n += std::snprintf(text + n, size - static_cast<size_t>(n), ": \"%s\"", a.name);
    if (a.index >= 0 && n >= 0 && static_cast<size_t>(n) < size)
        n += std::snprintf(text + n, size - static_cast<size_t>(n), ", object %d", a.index);
    if (a.check != Check::kNone && n >= 0 && static_cast<size_t>(n) < size)
        std::snprintf(text + n, size - static_cast<size_t>(n), " (at %08X: %08X)", a.at, a.value);
    return text;
}

}  // namespace qtefight

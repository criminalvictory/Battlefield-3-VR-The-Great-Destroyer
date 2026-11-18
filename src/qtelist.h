#pragma once
// The game's list of live input triggers, read (found 2026-11-18, bf3.exe 1.6.0.0): what a quick-time event awaits. A level's quick-time events
// are ClientPlayerInputTriggerEntity objects; enabled, each is in a client manager's list, and its data names the input action it waits for.
//   the manager     one a level, null between levels; its pointer is a global the code below finds by the signature of the code that reads it
//                   +0x0C/+0x10  the enabled entities, a pointer array's begin and end
//                   +0x1C/+0x20  adds and removes not yet applied (within 50 ms), 8 bytes each: the entity, 0 add / 1 remove
//   an entity       +0x10 its data; +0x18/+0x1C/+0x20 property links (none: the trigger sends an event on the input's edge)
//   its data        +0x10 the action, +0x20 SendTriggerEvents
// Every object is known by its vtable, checked on every read. The actions: InteractDrag 0x3C (E), the quick-time Fire 0x3D (the left mouse
// button), Block 0x3E (the right), FastMelee 0x3F (F), JumpClimb 0x40 (Space), CrouchDuck 0x41 (Z): the game's quick-time actions, answered
// whenever a trigger awaits them; Fire 0x07, Jump 0x0C and Interact 0x23 (the same keys): play's own actions, answered only in a scripted moment
// (an ordinary Fire trigger must never fire the gun). Everything else (Yaw 4 and Pitch 5 are always in the list; cameras) is ignored.
// Pure: memory is read through the caller's reader (the game's, guarded, in qtewatch.cpp; a made-up image in tools/qtetest.cpp), never directly.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "qte.h"

namespace qtelist {

// Offsets in bf3.exe (its image base 0x400000 taken off).
constexpr uint32_t kManagerType = 0x1D40E18;  // vtables
constexpr uint32_t kEntityType = 0x1D403A0;
constexpr uint32_t kDataType = 0x1DBDD5C;
constexpr uint32_t kSignatureAt = 0xBD9F46;    // where the signature was found (looked at first; the whole code section otherwise)
constexpr uint32_t kManagerGlobal = 0x1F84F7C;  // where it pointed (tried only if the signature isn't found, and trusted only by the manager's vtable)
constexpr uint32_t kGameContext = 0x1F80B58;   // the client's game context: +0x30 the players, +0xBC the local one, +0x3E0 its input state

// The code that reads the manager's global: mov ecx,[global]; test ecx,ecx; jnz +41; push edi; push 10h; push 4Ch; mov ecx,...
constexpr int kSignatureSize = 16;
inline bool SignatureAt(const uint8_t* p) {
    static const uint8_t kTail[] = {0x85, 0xC9, 0x75, 0x41, 0x57, 0x6A, 0x10, 0x6A, 0x4C, 0xB9};
    return p[0] == 0x8B && p[1] == 0x0D && std::memcmp(p + 6, kTail, sizeof(kTail)) == 0;
}
// Its first place in size bytes from `from` on, or -1; the global's address is the four bytes at +2 (SignatureGlobal).
inline long long FindSignature(const uint8_t* data, size_t size, size_t from = 0) {
    for (size_t i = from; i + kSignatureSize <= size; ++i)
        if (data[i] == 0x8B && SignatureAt(data + i))
            return static_cast<long long>(i);
    return -1;
}
inline uint32_t SignatureGlobal(const uint8_t* at) {
    uint32_t v;
    std::memcpy(&v, at + 2, sizeof(v));
    return v;
}

// The search of the game's code for it, a part a call (the whole section is 28 MB: a megabyte a frame costs under a millisecond).
struct Scan {
    uint32_t at = 0, end = 0;  // what is left to look through
    uint32_t found = 0;        // where the last find is (the search goes on past it)
    uint32_t global = 0;       // ... and the address it reads a pointer from
    int unread = 0;            // pages that couldn't be read (left out)
};
constexpr uint32_t kScanPage = 4096;
// Looks through the next `bytes` of [scan.at, scan.end), page by page (a signature across two pages is found too; an unreadable page is left
// out). valid(global): whether a find's global can be one (in the game's image, aligned); a find that fails it is passed over. True at a
// find (scan.found, scan.global; the next call goes on after it); false with scan.at == scan.end: no more.
// The signature is not the only one of its shape: 1.6.0.0's code has it twice (at 0x00A36996 another manager's, got and made the same way),
// so every find is a candidate (Candidates) and the manager's vtable picks the one (Pick).
template <class Reader, class Valid>
bool ScanStep(const Reader& read, Scan& scan, uint32_t bytes, const Valid& valid) {
    uint8_t page[kScanPage + kSignatureSize - 1];
    const uint32_t stop = scan.end - scan.at > bytes ? scan.at + bytes : scan.end;
    while (scan.at < stop) {
        const uint32_t left = scan.end - scan.at;
        uint32_t size = left < sizeof(page) ? left : static_cast<uint32_t>(sizeof(page));
        if (!read(scan.at, page, size)) {
            size = left < kScanPage ? left : kScanPage;  // (without the next page's first bytes)
            if (!read(scan.at, page, size)) {
                ++scan.unread;
                scan.at += size;
                continue;
            }
        }
        for (long long from = 0;;) {
            const long long at = FindSignature(page, size, static_cast<size_t>(from));
            if (at < 0)
                break;
            const uint32_t place = scan.at + static_cast<uint32_t>(at), global = SignatureGlobal(page + at);
            if (valid(global) && (!scan.found || place > scan.found)) {  // (not the last find again: its page is looked through once more)
                scan.found = place;
                scan.global = global;
                return true;
            }
            from = at + 1;
        }
        scan.at += size < kScanPage ? size : kScanPage;
    }
    return false;
}

// The addresses the finds read a pointer from; the manager's is among them.
constexpr int kMostCandidates = 8;
struct Candidates {
    int count = 0;
    uint32_t global[kMostCandidates] = {};
    uint32_t found[kMostCandidates] = {};  // where each was found in the code (0: not by the signature)
};
inline void AddCandidate(Candidates& c, uint32_t global, uint32_t found) {
    for (int i = 0; i < c.count; ++i)
        if (c.global[i] == global)
            return;
    if (c.count < kMostCandidates) {
        c.global[c.count] = global;
        c.found[c.count++] = found;
    }
}
// The candidate whose pointer leads to the manager right now (an object with the manager's vtable), or -1: none does (no level loaded, or
// none of them is it). Nothing is read through a candidate before this picks it.
template <class Reader>
int Pick(const Reader& read, const Candidates& c, uint32_t managerType) {
    for (int i = 0; i < c.count; ++i) {
        uint32_t manager = 0, type = 0;
        if (c.global[i] && read(c.global[i], &manager, sizeof(manager)) && manager >= 0x10000 && manager < 0xFFFF0000u && !(manager & 3) &&
            read(manager, &type, sizeof(type)) && type == managerType)
            return i;
    }
    return -1;
}

constexpr uint32_t kMgrBegin = 0x0C,kMgrEnd = 0x10, kMgrPendingBegin = 0x1C, kMgrPendingEnd = 0x20;
constexpr uint32_t kEntityData = 0x10, kEntityLinks = 0x18;  // (three links)
constexpr uint32_t kDataAction = 0x10, kDataSendEvents = 0x20;

constexpr int kMost = 32;         // enabled entities walked
constexpr int kMostPending = 16;  // adds and removes walked

// The absolute vtables (the module's base added).
struct Types {
    uint32_t manager = 0, entity = 0, data = 0;
};
inline Types TypesAt(uint32_t base) {
    return Types{base + kManagerType, base + kEntityType, base + kDataType};
}

struct Trigger {
    uint32_t entity = 0;
    uint32_t action = 0;
    bool hold = false;     // read by its level (property links, or no trigger events): the key is held; else its edge: tapped
    bool pending = false;  // an add not yet applied
};

enum class Status {
    kNone,        // no manager (between levels): nothing awaited
    kOk,
    kUnreadable,  // the global, the manager or its arrays couldn't be read
    kWrongType,   // what the global points at isn't the manager (its vtable)
};

struct List {
    Status status = Status::kNone;
    uint32_t manager = 0;
    int count = 0;
    Trigger at[kMost + kMostPending];
    int refused = 0;      // entries left out: a bad pointer, an unreadable object, a wrong vtable, an action out of range
    int twice = 0;        // ... an entity already in the list
    bool capped = false;  // more entries than are walked, or bounds that make no sense
};

inline bool Pointer(uint32_t v) {
    return v >= 0x10000 && v < 0xFFFF0000u && !(v & 3);
}

// One entity into the list, if it and its data are what their vtables say.
template <class Reader>
void Add(const Reader& read, const Types& types, List& list, uint32_t entity, bool pending) {
    for (int i = 0; i < list.count; ++i)
        if (list.at[i].entity == entity) {
            ++list.twice;
            return;
        }
    uint32_t e[9] = {};  // +0x00 the vtable ... +0x10 the data, +0x18/+0x1C/+0x20 the links
    if (!Pointer(entity) || !read(entity, e, sizeof(e)) || e[0] != types.entity || !Pointer(e[kEntityData / 4])) {
        ++list.refused;
        return;
    }
    uint32_t d[9] = {};  // +0x00 the vtable ... +0x10 the action, +0x20 SendTriggerEvents (a byte)
    if (!read(e[kEntityData / 4], d, sizeof(d)) || d[0] != types.data || d[kDataAction / 4] > 0xFF) {
        ++list.refused;
        return;
    }
    Trigger& t = list.at[list.count++];
    t.entity = entity;
    t.action = d[kDataAction / 4];
    const bool links = e[kEntityLinks / 4] || e[kEntityLinks / 4 + 1] || e[kEntityLinks / 4 + 2];
    t.hold = links || !(d[kDataSendEvents / 4] & 0xFF);
    t.pending = pending;
}

// The list as it is now: the enabled entities (at most kMost), then the adds not yet applied in, the removes out. global: the address the
// manager's pointer is read from. Nothing here can fault: every read goes through read(address, out, bytes), false if it can't be read.
template <class Reader>
List Read(const Reader& read, uint32_t global, const Types& types) {
    List list;
    uint32_t manager = 0;
    if (!global || !read(global, &manager, sizeof(manager))) {
        list.status = Status::kUnreadable;
        return list;
    }
    if (!manager)
        return list;  // (kNone)
    list.manager = manager;
    uint32_t m[9] = {};  // +0x00 the vtable ... +0x20
    if (!Pointer(manager) || !read(manager, m, sizeof(m))) {
        list.status = Status::kUnreadable;
        return list;
    }
    if (m[0] != types.manager) {
        list.status = Status::kWrongType;
        return list;
    }
    list.status = Status::kOk;
    const uint32_t begin = m[kMgrBegin / 4], end = m[kMgrEnd / 4];
    if (begin != end) {
        if (!Pointer(begin) || end < begin || ((end - begin) & 3)) {
            list.capped = true;
        } else {
            uint32_t n = (end - begin) / 4;
            if (n > static_cast<uint32_t>(kMost)) {
                n = kMost;
                list.capped = true;
            }
            uint32_t entities[kMost] = {};
            if (!read(begin, entities, n * 4)) {
                list.status = Status::kUnreadable;
                return list;
            }
            for (uint32_t i = 0; i < n; ++i)
                Add(read, types, list, entities[i], false);
        }
    }
    const uint32_t pBegin = m[kMgrPendingBegin / 4], pEnd = m[kMgrPendingEnd / 4];
    if (pBegin != pEnd) {
        if (!Pointer(pBegin) || pEnd < pBegin || ((pEnd - pBegin) & 7)) {
            list.capped = true;
        } else {
            uint32_t n = (pEnd - pBegin) / 8;
            if (n > static_cast<uint32_t>(kMostPending)) {
                n = kMostPending;
                list.capped = true;
            }
            uint32_t pending[kMostPending * 2] = {};
            if (!read(pBegin, pending, n * 8)) {
                list.status = Status::kUnreadable;
                return list;
            }
            for (uint32_t i = 0; i < n; ++i) {
                const uint32_t entity = pending[i * 2], remove = pending[i * 2 + 1] & 0xFF;  // (the flag's byte alone: what lies above it isn't the game's)
                if (remove == 0) {
                    Add(read, types, list, entity, true);
                } else if (remove == 1) {
                    for (int k = 0; k < list.count; ++k)
                        if (list.at[k].entity == entity) {
                            for (int j = k; j + 1 < list.count; ++j)
                                list.at[j] = list.at[j + 1];
                            --list.count;
                            break;
                        }
                } else {
                    ++list.refused;
                }
            }
        }
    }
    return list;
}

// A read that may have caught the game changing its list (a failed read, an entry refused): this frame's or the last good one's.
struct Steady {
    bool have = false;
    unsigned long long at = 0;
    List good;
};
constexpr unsigned long long kSteadyMs = 50;
// The list to go by: a doubtful one (not read, or entries refused) gives way to the last good one for kSteadyMs, so a read torn by the game
// changing the list on its own thread doesn't drop a press for a frame; past that, or with none, the doubtful one as it is.
inline const List& Settle(Steady& steady, const List& now, unsigned long long nowMs) {
    const bool good = (now.status == Status::kOk || now.status == Status::kNone) && !now.refused && !now.capped;
    if (good) {
        steady.have = true;
        steady.at = nowMs;
        steady.good = now;
        return steady.good;
    }
    if (steady.have && nowMs >= steady.at && nowMs - steady.at <= kSteadyMs)
        return steady.good;
    steady.have = false;
    return now;
}

// The key an action is answered with (a qte::Key), 0 for one not answered.
inline unsigned KeyOf(uint32_t action) {
    switch (action) {
    case 0x3C: case 0x23: return qte::kKeyE;
    case 0x3D: case 0x07: return qte::kKeyLeftMouse;
    case 0x3E: return qte::kKeyRightMouse;
    case 0x3F: return qte::kKeyF;
    case 0x40: case 0x0C: return qte::kKeySpace;
    case 0x41: return qte::kKeyZ;
    default: return 0;
    }
}
// Play's own actions: answered only in a scripted moment.
inline bool Gated(uint32_t action) {
    return action == 0x07 || action == 0x0C || action == 0x23;
}
inline const char* ActionName(uint32_t action) {
    switch (action) {
    case 0x3C: return "InteractDrag";
    case 0x3D: return "the quick-time Fire";
    case 0x3E: return "Block";
    case 0x3F: return "FastMelee";
    case 0x40: return "JumpClimb";
    case 0x41: return "CrouchDuck";
    case 0x07: return "Fire";
    case 0x0C: return "Jump";
    case 0x23: return "Interact";
    case 0x04: return "Yaw";
    case 0x05: return "Pitch";
    default: return "another action";
    }
}

// The awaited keys: await, and among them hold (a key any of its live triggers reads by its level is held).
struct Keys {
    unsigned await = 0, hold = 0;
};
inline Keys Awaited(const List& list, bool scripted) {
    Keys keys;
    if (list.status != Status::kOk)
        return keys;
    for (int i = 0; i < list.count; ++i) {
        const Trigger& t = list.at[i];
        const unsigned key = KeyOf(t.action);
        if (!key || (Gated(t.action) && !scripted))
            continue;
        keys.await |= key;
        if (t.hold)
            keys.hold |= key;
    }
    return keys;
}

// The game's own input state for the local player: the digital actions as a mask (bit = action - 10), the analog ones 0-9 as floats.
constexpr uint32_t kContextPlayers = 0x30, kPlayersLocal = 0xBC, kPlayerInput = 0x3E0;
constexpr uint32_t kInputAnalog = 0x10, kInputDigital = 0x1E0;
struct Input {
    bool read = false;
    uint64_t digital = 0;
    float fire = 0.0f;  // the analog action 7
};
template <class Reader>
Input ReadInput(const Reader& read, uint32_t context) {
    Input in;
    uint32_t at = 0;
    if (!context || !read(context, &at, 4) || !Pointer(at) || !read(at + kContextPlayers, &at, 4) || !Pointer(at) ||
        !read(at + kPlayersLocal, &at, 4) || !Pointer(at) || !read(at + kPlayerInput, &at, 4) || !Pointer(at))
        return in;
    if (!read(at + kInputAnalog + 4 * 7, &in.fire, sizeof(in.fire)) || !read(at + kInputDigital, &in.digital, sizeof(in.digital)))
        return in;
    in.read = true;
    return in;
}
inline bool ActionOn(const Input& in, uint32_t action) {
    if (!in.read)
        return false;
    if (action == 0x07)
        return in.fire > 0.5f;  // (not a number: off)
    return action >= 10 && action < 74 && (in.digital >> (action - 10) & 1);
}
// The keys that state shows down ("seen by the game"), by the six quick-time actions: each key sets its own whether a trigger awaits it or not
// (play's Interact, 0x23, goes on with E and with the left mouse button alike: no telling by it).
inline unsigned SeenKeys(const Input& in) {
    unsigned keys = 0;
    for (uint32_t action = 0x3C; action <= 0x41; ++action)
        if (ActionOn(in, action))
            keys |= KeyOf(action);
    return keys;
}

}  // namespace qtelist

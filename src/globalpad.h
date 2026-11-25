#pragma once
// The game's pad read with its window in the background (bf3.exe 1.6.0.0, read in build\bf3_mem.exe 2026-11-25). The mod walks the soldier through
// a virtual pad's left stick (pad.cpp: XInput's controller 0), and the game reads its pads only while its window has the keyboard focus: each of its
// pad updates asks ClientSettings.UseGlobalGamePadInput first and, that being false, Windows' GetFocus (USER32), and with no focus takes nothing
// from the pad, nor asks XInput at all. The setting true, the pad is read whatever window is in front. So the mod sets it: the stick walks with
// another window in front (a click on Discord, a VR app's window). Keys and clicks still need the game's window in front (they go to whichever
// window has it).
//
// Where it is. ClientSettings (its type info 0x023F7F90 -> its data 0x023F68F8: "ClientSettings", 0x9C bytes, 69 fields from 0x02431450) is the
// game's settings object, found by type from the settings manager ([0x022ED35C], its getter 0x0049EB60) and kept in 0x022E39C4 by every place that
// uses it. Its field records (12 bytes each: the name, the flags with the field's offset in the high half, the type) say UseGlobalGamePadInput is a
// Boolean at +0x8A (the record at 0x024316D8). Its vtable 0x021BAB20, whose getType (0x01FB9800) gives that type info. Four places read it, each the
// same way (the cached pointer, else the getter, then cmp byte ptr [eax+8Ah],0); the first three then ask GetFocus where it's 0, the fourth doesn't:
//   0x00FCE450  a check of the same (then GetFocus, then [0x022C2E08]); nothing in the image calls it or holds its address (the review of
//               2026-11-25), so it's borne out with the others and does no more
//   0x00FCE4C9  the pad state's update (0x00FCE490): the same test, then XInput read into the pad object 0x022C2CE8 (0x00FCBBA0) and copied
//   0x00FCE6F0  the pad's other update, the last of the 17 entries of the table at 0x02140280 (held at 0x021402C0): the same test, then two
//               numbers clamped to 0-1 handed to the pad object 0x022C2CE8 (0x00FC2EC0: its two motors' buzz)
//   0x00FD93F1  DirectInput's controller callback (pad.cpp's kPadCallback 0x00FD93F0): no GetFocus; the setting picks how a controller that
//               isn't an XInput one is opened as it's found: in the background and shared (cooperative level 10) where it's 1, else in the
//               foreground and exclusive (5)
// GetFocus is the import at 0x023BDECC (the original import table: USER32.dll, GetFocus).
//
// Every constant is borne out at the first look by the game's code bytes, its field record and type names (Prove); one that isn't leaves the
// setting alone for the run. Each look: the pointer the game keeps, the object's vtable, the byte 0 or 1. Plain heap and image memory, nothing the
// game mapped from the graphics card (AMD-safe). Pure: memory is read through the caller's reader (pad.cpp's guarded one; a made-up image in
// tools/globalpadtest.cpp, bf3_mem.exe there too), never directly; the write is the caller's.

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace globalpad {

// ------------------------------------------------- the constants (bf3.exe's image base taken off) -------------------------------------------------
constexpr uint32_t kSettingsGlobal = 0x1EE39C4;   // [0x022E39C4]: the game's ClientSettings
constexpr uint32_t kSettingsManager = 0x1EED35C;  // [0x022ED35C]: the settings manager the getter is asked on
constexpr uint32_t kGetter = 0x9EB60;             // 0x0049EB60: the manager's settings object by type
constexpr uint32_t kClassInfo = 0x1FF7F90;        // ClientSettings' type info
constexpr uint32_t kClassData = 0x1FF68F8;        // ... its data: name, flags and size, module, alignment and field count, base, ..., fields
constexpr uint32_t kFields = 0x2031450;           // ... its field records
constexpr uint32_t kFieldRecord = 0x20316D8;      // UseGlobalGamePadInput's record
constexpr uint32_t kFieldName = 0x1DB9B78;        // "UseGlobalGamePadInput"
constexpr uint32_t kBoolType = 0x1FC2400;         // Boolean's type info
constexpr uint32_t kVtable = 0x1DBAB20;           // ClientSettings' vtable
constexpr uint32_t kGetFocusSlot = 0x1FBDECC;     // the import of USER32's GetFocus
constexpr uint32_t kField = 0x8A;                 // UseGlobalGamePadInput's byte
constexpr uint32_t kRecordFlags = kField << 16;   // the record's flags word: the offset in its high half, no flags
constexpr uint32_t kClassSize = 0x9C;
constexpr uint32_t kRecordSize = 12;

// The four places that read it (Site): where each starts (mov eax,[global]), its bytes with ?? for the addresses in it, and where in it each address is.
struct Site {
    uint32_t at;
    const char* bytes;
    uint8_t global1, global2;  // the cached pointer: read, and written after the getter
    uint8_t manager;           // the settings manager
    uint8_t info;              // ClientSettings' type info, pushed for the getter
    uint8_t call;              // E8 rel32: the getter
    uint8_t test;              // cmp byte ptr [eax+8Ah],0
    uint8_t focus;             // FF 15 [GetFocus] (0: this one hands the flag on instead)
};
constexpr int kSites = 4;
constexpr Site kSite[kSites] = {
    {0xBCE450, "A1 ?? ?? ?? ?? 85 C0 75 15 8B 0D ?? ?? ?? ?? 68 ?? ?? ?? ?? E8 ?? ?? ?? ?? A3 ?? ?? ?? ?? 80 B8 8A 00 00 00 00 75 0D FF 15 ?? ?? ?? ?? 85 C0",
     1, 26, 11, 16, 20, 30, 41},
    {0xBCE4C9, "A1 ?? ?? ?? ?? 85 C0 75 15 8B 0D ?? ?? ?? ?? 68 ?? ?? ?? ?? E8 ?? ?? ?? ?? A3 ?? ?? ?? ?? 80 B8 8A 00 00 00 00 75 0E FF 15 ?? ?? ?? ?? 85 C0",
     1, 26, 11, 16, 20, 30, 41},
    {0xBCE6F0, "A1 ?? ?? ?? ?? 56 8B F1 85 C0 75 15 8B 0D ?? ?? ?? ?? 68 ?? ?? ?? ?? E8 ?? ?? ?? ?? A3 ?? ?? ?? ?? 80 B8 8A 00 00 00 00 75 0A FF 15 ?? ?? ?? ?? 85 C0",
     1, 29, 14, 19, 23, 33, 44},
    {0xBD93F1, "A1 ?? ?? ?? ?? 53 32 DB 56 57 88 5C 24 0C 85 C0 75 15 8B 0D ?? ?? ?? ?? 68 ?? ?? ?? ?? E8 ?? ?? ?? ?? A3 ?? ?? ?? ?? 80 B8 8A 00 00 00 00 74 06 B3 01",
     1, 35, 20, 25, 29, 39, 0},
};

// ------------------------------------------------------------- reading the image -------------------------------------------------------------

// A pattern's bytes at an address ("8B 54 ?? 04": ?? any byte; at most 96 bytes).
template <class Reader>
bool Matches(const Reader& read, uint32_t at, const char* pattern) {
    uint8_t want[96], mask[96], got[96];
    int n = 0;
    const auto hex = [](char c) -> int {
        return c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10 : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
    };
    for (const char* p = pattern; *p;) {
        if (*p == ' ') {
            ++p;
            continue;
        }
        if (n == 96 || !p[1])
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

template <class Reader>
uint32_t Word(const Reader& read, uint32_t at, bool* ok = nullptr) {
    uint32_t v = 0;
    const bool got = read(at, &v, 4);
    if (ok)
        *ok = got;
    return got ? v : 0;
}

// The text at an address is want, its end included.
template <class Reader>
bool TextIs(const Reader& read, uint32_t at, const char* want) {
    const size_t n = std::strlen(want) + 1;
    char got[64] = {};
    return n <= sizeof(got) && read(at, got, n) && std::memcmp(got, want, n) == 0;
}

// A type info's name ([[info]]: its data, whose +0 is the name).
template <class Reader>
bool TypeInfoNamed(const Reader& read, uint32_t info, const char* want) {
    bool a = false, b = false;
    const uint32_t data = Word(read, info, &a);
    const uint32_t text = a ? Word(read, data, &b) : 0;
    return a && b && TextIs(read, text, want);
}

// ------------------------------------------------------------------ the proofs ------------------------------------------------------------------

constexpr int kProofs = 3 + kSites;
inline const char* ProofName(int i) {
    static const char* const kNames[kProofs] = {
        "UseGlobalGamePadInput's field record (its name, a Boolean at +0x8A)",
        "ClientSettings' type (its name, its size, its fields with that record among them)",
        "ClientSettings' vtable (its getType gives that type)",
        "the pad's check (the setting, else GetFocus)",
        "the pad state's update (the setting, else GetFocus)",
        "the pad's other update (the setting, else GetFocus)",
        "DirectInput's controller callback (the setting as its flag)",
    };
    return i >= 0 && i < kProofs ? kNames[i] : "?";
}

struct Proofs {
    bool ok = false;
    int failed = -1;  // the first proof not borne out (ProofName)
    uint32_t at = 0;  // where it was looked for
};

template <class Reader>
bool SiteOk(const Reader& read, uint32_t base, const Site& s) {
    const uint32_t at = base + s.at;
    if (!Matches(read, at, s.bytes))
        return false;
    bool ok[6] = {};
    const uint32_t g1 = Word(read, at + s.global1, &ok[0]), g2 = Word(read, at + s.global2, &ok[1]);
    const uint32_t manager = Word(read, at + s.manager, &ok[2]), info = Word(read, at + s.info, &ok[3]);
    const uint32_t rel = Word(read, at + s.call + 1, &ok[4]);
    for (int i = 0; i < 5; ++i)
        if (!ok[i])
            return false;
    if (g1 != base + kSettingsGlobal || g2 != base + kSettingsGlobal || manager != base + kSettingsManager || info != base + kClassInfo ||
        at + s.call + 5 + rel != base + kGetter)
        return false;
    uint8_t test[7] = {};
    if (!read(at + s.test, test, sizeof(test)) || test[0] != 0x80 || test[1] != 0xB8 || test[2] != kField || test[3] || test[4] || test[5] || test[6])
        return false;
    if (s.focus) {
        const uint32_t slot = Word(read, at + s.focus, &ok[5]);
        if (!ok[5] || slot != base + kGetFocusSlot)
            return false;
    }
    return true;
}

template <class Reader>
bool ProveOne(const Reader& read, uint32_t base, int i, uint32_t* at) {
    uint32_t where = 0;
    bool ok = false;
    switch (i) {
    case 0: {
        where = base + kFieldRecord;
        bool a = false, b = false, c = false;
        const uint32_t name = Word(read, where, &a), flags = Word(read, where + 4, &b), type = Word(read, where + 8, &c);
        ok = a && b && c && name == base + kFieldName && TextIs(read, name, "UseGlobalGamePadInput") && flags == kRecordFlags &&
             type == base + kBoolType && TypeInfoNamed(read, type, "Boolean");
        break;
    }
    case 1: {
        where = base + kClassInfo;
        bool a = false, b = false, c = false, d = false;
        uint8_t count = 0;
        const uint32_t data = Word(read, where, &a);
        const uint32_t name = a ? Word(read, data, &b) : 0;
        const uint32_t sizeWord = a ? Word(read, data + 4, &c) : 0;
        const uint32_t fields = a ? Word(read, data + 0x18, &d) : 0;
        const bool countRead = a && read(data + 0x0D, &count, 1);
        const uint32_t index = (kFieldRecord - kFields) / kRecordSize;
        ok = a && b && c && d && countRead && data == base + kClassData && TextIs(read, name, "ClientSettings") && (sizeWord >> 16) == kClassSize &&
             kClassSize > kField && fields == base + kFields && index < count && (kFieldRecord - kFields) % kRecordSize == 0;
        break;
    }
    case 2: {
        where = base + kVtable;
        bool a = false;
        uint8_t code[6] = {};
        const uint32_t getType = Word(read, where, &a);
        uint32_t info = 0;
        if (a && read(getType, code, sizeof(code)) && code[0] == 0xB8 && code[5] == 0xC3)
            std::memcpy(&info, code + 1, 4);
        ok = info == base + kClassInfo && TypeInfoNamed(read, info, "ClientSettings");
        break;
    }
    default:
        if (i >= 3 && i < kProofs) {
            where = base + kSite[i - 3].at;
            ok = SiteOk(read, base, kSite[i - 3]);
        }
        break;
    }
    if (at)
        *at = where;
    return ok;
}

// Every constant above, in order; the first not borne out named.
template <class Reader>
Proofs Prove(const Reader& read, uint32_t base) {
    Proofs p;
    for (int i = 0; i < kProofs; ++i) {
        uint32_t at = 0;
        if (!ProveOne(read, base, i, &at)) {
            p.failed = i;
            p.at = at;
            return p;
        }
    }
    p.ok = true;
    return p;
}

// ------------------------------------------------------------------ a look ------------------------------------------------------------------

enum class Miss : uint8_t {
    kNone,
    kNotYet,  // the game hasn't kept its ClientSettings yet (its first pad update does): looked for again
    kType,    // what it keeps isn't a ClientSettings (its vtable), or isn't readable
    kValue,   // the setting's byte isn't 0 or 1
};
inline const char* MissName(Miss m) {
    switch (m) {
    case Miss::kNone: return "read";
    case Miss::kNotYet: return "the game hasn't kept its ClientSettings yet";
    case Miss::kType: return "the game's ClientSettings pointer isn't a ClientSettings";
    case Miss::kValue: return "UseGlobalGamePadInput isn't 0 or 1";
    }
    return "?";
}

struct Look {
    bool ok = false;
    Miss miss = Miss::kNotYet;
    uint32_t settings = 0;  // the object
    uint32_t at = 0;        // its byte's address (the caller's write goes there)
    bool on = false;        // the byte is 1
};

// One look at the setting (after Prove).
template <class Reader>
Look Read(const Reader& read, uint32_t base) {
    Look l;
    bool got = false;
    const uint32_t settings = Word(read, base + kSettingsGlobal, &got);
    if (!got || !settings)
        return l;
    l.settings = settings;
    if (Word(read, settings, &got) != base + kVtable || !got) {
        l.miss = Miss::kType;
        return l;
    }
    uint8_t value = 0xFF;
    if (!read(settings + kField, &value, 1) || value > 1) {
        l.miss = Miss::kValue;
        return l;
    }
    l.ok = true;
    l.miss = Miss::kNone;
    l.at = settings + kField;
    l.on = value == 1;
    return l;
}

// What to write: 1 where it's 0 (the game's own default), nothing where it's 1 already or not read.
inline bool Wants(const Look& l) {
    return l.ok && !l.on;
}

}  // namespace globalpad

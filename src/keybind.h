#pragma once
// The player's own key bindings, so every key the mod sends follows a player who moved an action to another key (crouch X, prone Z, use E,
// reload R, jump Space, sprint, a weapon's key, the walking keys, the quick-time keys...). Read only from the game's settings file,
// Documents\Battlefield 3\settings\PROF_SAVE_profile (plain text, "key value" a line), its GstKeyBinding.infantry.<Concept>.<slot>.<field>
// lines; today's keys are the fallback. Pure, header only: tools/keybindtest.cpp checks it, keybind.cpp finds and watches the file, input.cpp's
// SendKey sends each key as it says.
//
// The file as bf3.exe 1.6.0.0 writes it (the test PC's, 2026-11-25): each concept has numbered slots, each four lines: type 0 a key (button: its
// DirectInput key code, DIK_*: W 17, Space 57, E 18...), 1 the mouse (button 0 its left button, 1 its right; axes), 2 the pad's; button 255 (or 0)
// nothing; negate 1 a key that drives its axis backwards (S on ConceptMoveFB, A on ConceptMoveLR). A concept's lines are the game's whole binding
// of it, so:
//   - a concept with no lines (or no file, or a file with no key bindings at all) keeps the game's own default: today's key. The quick-time
//     actions are such: the game's options never write them;
//   - a concept with lines takes its first key the mod can send (its slots in order);
//   - a concept with lines and no such key (on the mouse or the pad alone, unbound, or only on a key the mod listens to itself) gets nothing
//     pressed for it: its old key may be another action's now. Said in the log once.
// Keys the mod never sends for a player (Sendable): Escape, F1-F12, Home, End, Insert, Delete, Page Up/Down, Pause, Scroll Lock, Num Lock, Caps
// Lock, Print Screen, the Windows and menu keys, the media and power keys: the mod's own keys (F8 the VR view, F9 recenter, End god mode, Home
// the monitor's view, Insert the hands) would react to their own press, and the others do things outside the game.
// The mouse buttons the mod presses (fire, zoom) aren't moved: Fire and Zoom are only checked to be on the left and right mouse button (a line
// in the log if not).

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace keybind {

// The mod's keys, each the game's action it stands for.
enum Key : uint8_t {
    kForward, kBack, kLeft, kRight,  // W S A D (the pad's stick walks unless the game stops reading it)
    kJump,                           // Space
    kCrouchHold,                     // Left Ctrl: the head's real crouch
    kCrouch,                         // X: the crouch toggle (the right stick)
    kProne,                          // Z: the prone toggle (the right stick)
    kInteract,                       // E: use (Y), and a scripted moment's play Interact on the grips
    kReload,                         // R: reload (X held), pickups, the prompt on the reload key
    kSprint,                         // Left Shift: the left stick's click
    kGrenade,                        // G
    kSlot1, kSlot2, kSlot5,          // 1, 2, 4: the rifle, the second gun, the launcher
    kQteInteractDrag,                // E   \  the quick-time actions proper (the grips' presses: qte.h): the game's own defaults, never in the
    kQteFastMelee,                   // F    | player's settings (a line for one is followed all the same)
    kQteJumpClimb,                   // Space|
    kQteCrouchDuck,                  // Z   /
    kKeyCount
};

struct KeyInfo {
    const char* name;    // its concept's GstKeyBinding name
    int negate;          // -1 any slot; 0 or 1 only a slot with that negate (the walking keys on their axes)
    uint8_t dik;         // today's key
    const char* action;  // for the log
};

constexpr KeyInfo kKeys[kKeyCount] = {
    {"ConceptMoveFB", 0, 0x11, "walk forward"},
    {"ConceptMoveFB", 1, 0x1F, "walk back"},
    {"ConceptMoveLR", 1, 0x1E, "walk left"},
    {"ConceptMoveLR", 0, 0x20, "walk right"},
    {"ConceptJump", -1, 0x39, "jump"},
    {"ConceptCrouchOnHold", -1, 0x1D, "crouch while held"},
    {"ConceptCrouch", -1, 0x2D, "crouch"},
    {"ConceptProne", -1, 0x2C, "prone"},
    {"ConceptInteract", -1, 0x12, "use"},
    {"ConceptReload", -1, 0x13, "reload"},
    {"ConceptSprint", -1, 0x2A, "sprint"},
    {"ConceptThrowGrenade", -1, 0x22, "throw a grenade"},
    {"ConceptSelectInventoryItem1", -1, 0x02, "the first weapon"},
    {"ConceptSelectInventoryItem2", -1, 0x03, "the second weapon"},
    {"ConceptSelectInventoryItem5", -1, 0x05, "the launcher"},
    {"ConceptQuicktimeInteractDrag", -1, 0x12, "a quick-time E"},
    {"ConceptQuicktimeFastMelee", -1, 0x21, "a quick-time F"},
    {"ConceptQuicktimeJumpClimb", -1, 0x39, "a quick-time Space"},
    {"ConceptQuicktimeCrouchDuck", -1, 0x2C, "a quick-time Z"},
};

// The concepts read: the keys' (each once), and Fire and Zoom (only checked).
constexpr const char* kConcepts[] = {
    "ConceptMoveFB", "ConceptMoveLR", "ConceptJump", "ConceptCrouchOnHold", "ConceptCrouch", "ConceptProne", "ConceptInteract",
    "ConceptReload", "ConceptSprint", "ConceptThrowGrenade", "ConceptSelectInventoryItem1", "ConceptSelectInventoryItem2",
    "ConceptSelectInventoryItem5", "ConceptQuicktimeInteractDrag", "ConceptQuicktimeFastMelee", "ConceptQuicktimeJumpClimb",
    "ConceptQuicktimeCrouchDuck", "ConceptFire", "ConceptZoom",
};
constexpr int kConceptCount = static_cast<int>(sizeof(kConcepts) / sizeof(kConcepts[0]));
constexpr int kFire = kConceptCount - 2, kZoom = kConceptCount - 1;

inline int ConceptIndex(const char* name, size_t length) {
    for (int i = 0; i < kConceptCount; ++i)
        if (std::strlen(kConcepts[i]) == length && std::memcmp(kConcepts[i], name, length) == 0)
            return i;
    return -1;
}

inline int ConceptOf(Key k) {
    return ConceptIndex(kKeys[k].name, std::strlen(kKeys[k].name));
}

constexpr int kSlots = 8;
constexpr int kTypeKey = 0, kTypeMouse = 1, kTypePad = 2;

struct Slot {
    bool any = false;  // a line of it was read
    int type = -1, button = -1, negate = 0, axis = 0;
};

struct Concept {
    bool found = false;  // the file has lines for it: they're the game's whole binding of it
    Slot slot[kSlots];
};

struct Bindings {
    bool read = false;  // a file was read
    int lines = 0;      // its GstKeyBinding.infantry lines taken (0: the file has no key bindings: today's keys for all)
    int bad = 0;        // such lines that couldn't be read (a field or a number not as the game writes them)
    Concept concepts[kConceptCount];
};

// ------------------------------------------------------------------ the keys ------------------------------------------------------------------

// A DirectInput key code the mod may send for a player (the file's numbers are those codes; SendInput takes the scan code, with E0's extended
// flag for codes over 0x80).
inline bool Sendable(unsigned dik) {
    if (dik >= 0x02 && dik <= 0x39)  // 1-0, - =, Backspace, Tab, the letters, [ ], Enter, Ctrl, ; ' `, the Shifts, \ , . /, numpad *, Alt, Space
        return true;
    if (dik >= 0x47 && dik <= 0x53)  // the number pad's keys (Num Lock, 0x45, isn't)
        return true;
    switch (dik) {
    case 0x56:                                                    // the key beside the left Shift (< > |)
    case 0x70: case 0x73: case 0x79: case 0x7B: case 0x7D: case 0x7E:  // Japanese and Brazilian keyboards' own
    case 0x9C: case 0x9D:                                         // the number pad's Enter, the right Ctrl
    case 0xB5: case 0xB8:                                         // the number pad's /, the right Alt
    case 0xC8: case 0xCB: case 0xCD: case 0xD0:                   // the arrows
        return true;
    default:
        return false;
    }
}

// SendInput's scan code and extended flag for a key code.
inline uint16_t ScanOf(unsigned dik) {
    return static_cast<uint16_t>(dik & 0x7F);
}
inline bool Extended(unsigned dik) {
    return (dik & 0x80) != 0;
}
// The scan code Windows' MapVirtualKeyEx takes for a key code (MAPVK_VSC_TO_VK_EX: E0 in its high byte for an extended key), for its key code.
inline unsigned MapScanOf(unsigned dik) {
    return ScanOf(dik) | (Extended(dik) ? 0xE000u : 0u);
}
// The number pad's own Windows key code (VK_NUMPAD7 ... VK_DECIMAL: Num Lock on) of a key code there; 0 for any other. Windows' scan code to key
// code gives a number pad key's other meaning (Home, the arrows...), so a key looked at by its code (ammo.cpp's reload) looks at both.
inline int NumberPadVk(unsigned dik) {
    static const unsigned char kVk[] = {0x67, 0x68, 0x69, 0x6D, 0x64, 0x65, 0x66, 0x6B, 0x61, 0x62, 0x63, 0x60, 0x6E};  // 7 8 9 - 4 5 6 + 1 2 3 0 .
    return dik >= 0x47 && dik <= 0x53 ? kVk[dik - 0x47] : 0;
}

// A key's name for the log ("E", "Left Ctrl", "the key 0x56").
inline const char* KeyName(unsigned dik, char* buffer, size_t size) {
    static const char* const kLetters = "QWERTYUIOP";  // 0x10-0x19
    static const char* const kRow2 = "ASDFGHJKL";     // 0x1E-0x26
    static const char* const kRow3 = "ZXCVBNM";       // 0x2C-0x32
    const char* name = nullptr;
    char one[2] = {0, 0};
    if (dik >= 0x02 && dik <= 0x0B) {
        one[0] = static_cast<char>(dik == 0x0B ? '0' : '1' + (dik - 0x02));
        name = one;
    } else if (dik >= 0x10 && dik <= 0x19) {
        one[0] = kLetters[dik - 0x10];
        name = one;
    } else if (dik >= 0x1E && dik <= 0x26) {
        one[0] = kRow2[dik - 0x1E];
        name = one;
    } else if (dik >= 0x2C && dik <= 0x32) {
        one[0] = kRow3[dik - 0x2C];
        name = one;
    } else {
        switch (dik) {
        case 0x01: name = "Escape"; break;
        case 0x0C: name = "-"; break;
        case 0x0D: name = "="; break;
        case 0x0E: name = "Backspace"; break;
        case 0x0F: name = "Tab"; break;
        case 0x1A: name = "["; break;
        case 0x1B: name = "]"; break;
        case 0x1C: name = "Enter"; break;
        case 0x1D: name = "Left Ctrl"; break;
        case 0x27: name = ";"; break;
        case 0x28: name = "'"; break;
        case 0x29: name = "`"; break;
        case 0x2A: name = "Left Shift"; break;
        case 0x2B: name = "\\"; break;
        case 0x33: name = ","; break;
        case 0x34: name = "."; break;
        case 0x35: name = "/"; break;
        case 0x36: name = "Right Shift"; break;
        case 0x37: name = "the number pad's *"; break;
        case 0x38: name = "Left Alt"; break;
        case 0x39: name = "Space"; break;
        case 0x3A: name = "Caps Lock"; break;
        case 0x45: name = "Num Lock"; break;
        case 0x46: name = "Scroll Lock"; break;
        case 0x9C: name = "the number pad's Enter"; break;
        case 0x9D: name = "Right Ctrl"; break;
        case 0xB5: name = "the number pad's /"; break;
        case 0xB7: name = "Print Screen"; break;
        case 0xB8: name = "Right Alt"; break;
        case 0xC5: name = "Pause"; break;
        case 0xC7: name = "Home"; break;
        case 0xC8: name = "Up"; break;
        case 0xC9: name = "Page Up"; break;
        case 0xCB: name = "Left"; break;
        case 0xCD: name = "Right"; break;
        case 0xCF: name = "End"; break;
        case 0xD0: name = "Down"; break;
        case 0xD1: name = "Page Down"; break;
        case 0xD2: name = "Insert"; break;
        case 0xD3: name = "Delete"; break;
        case 0xDB: name = "the left Windows key"; break;
        case 0xDC: name = "the right Windows key"; break;
        case 0xDD: name = "the menu key"; break;
        default: break;
        }
        if (!name && dik >= 0x3B && dik <= 0x44) {
            std::snprintf(buffer, size, "F%u", dik - 0x3B + 1);
            return buffer;
        }
        if (!name && (dik == 0x57 || dik == 0x58)) {
            std::snprintf(buffer, size, "F%u", dik - 0x57 + 11);
            return buffer;
        }
        if (!name && dik >= 0x47 && dik <= 0x53) {
            static const char* const kPad[] = {"7", "8", "9", "-", "4", "5", "6", "+", "1", "2", "3", "0", "."};
            std::snprintf(buffer, size, "the number pad's %s", kPad[dik - 0x47]);
            return buffer;
        }
    }
    if (name)
        std::snprintf(buffer, size, "%s", name);
    else
        std::snprintf(buffer, size, "the key 0x%02X", dik);
    return buffer;
}

// ------------------------------------------------------------------ the file ------------------------------------------------------------------

namespace detail {

inline bool Digits(const char* p, const char* end, long long& value) {
    bool minus = false;
    if (p < end && (*p == '-' || *p == '+'))
        minus = *p++ == '-';
    if (p >= end)
        return false;
    long long v = 0;
    for (; p < end; ++p) {
        if (*p < '0' || *p > '9')
            return false;
        v = v * 10 + (*p - '0');
        if (v > 0x7FFFFFFF)
            return false;
    }
    value = minus ? -v : v;
    return true;
}

// One line, without its end ("GstKeyBinding.infantry.ConceptJump.1.button 57"): taken into b if it's an infantry binding of a concept read.
inline void Line(Bindings& b, const char* p, const char* end) {
    static const char kPrefix[] = "GstKeyBinding.infantry.";
    constexpr size_t kPrefixLength = sizeof(kPrefix) - 1;
    while (p < end && (*p == ' ' || *p == '\t'))
        ++p;
    while (end > p && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r'))
        --end;
    if (static_cast<size_t>(end - p) <= kPrefixLength || std::memcmp(p, kPrefix, kPrefixLength) != 0)
        return;
    p += kPrefixLength;
    const char* name = p;
    while (p < end && *p != '.' && *p != ' ' && *p != '\t')
        ++p;
    if (p >= end || *p != '.')
        return;
    const int which = ConceptIndex(name, static_cast<size_t>(p - name));
    ++b.lines;  // (every infantry binding line counts: a file with any is the game's own list)
    if (which < 0)
        return;
    ++p;
    const char* slotAt = p;
    while (p < end && *p >= '0' && *p <= '9')
        ++p;
    long long slot = 0;
    if (p >= end || *p != '.' || !Digits(slotAt, p, slot) || slot < 0) {
        ++b.bad;
        return;
    }
    ++p;
    const char* field = p;
    while (p < end && *p != ' ' && *p != '\t')
        ++p;
    const size_t fieldLength = static_cast<size_t>(p - field);
    while (p < end && (*p == ' ' || *p == '\t'))
        ++p;
    long long value = 0;
    if (!Digits(p, end, value)) {
        ++b.bad;
        return;
    }
    Concept& c = b.concepts[which];
    c.found = true;
    if (slot >= kSlots)
        return;  // (a slot past the eighth: the game's options have at most four)
    Slot& s = c.slot[slot];
    const auto is = [&](const char* f) { return std::strlen(f) == fieldLength && std::memcmp(f, field, fieldLength) == 0; };
    if (is("type"))
        s.type = static_cast<int>(value);
    else if (is("button"))
        s.button = static_cast<int>(value);
    else if (is("negate"))
        s.negate = static_cast<int>(value);
    else if (is("axis"))
        s.axis = static_cast<int>(value);
    else {
        ++b.bad;
        return;
    }
    s.any = true;
}

}  // namespace detail

// The settings file's text (any size; lines end in \n or \r\n). read: it was there to be read.
inline Bindings Parse(const char* text, size_t size, bool read = true) {
    Bindings b;
    b.read = read;
    if (!text)
        return b;
    const char* p = text;
    const char* end = text + size;
    while (p < end) {
        const char* lineEnd = static_cast<const char*>(std::memchr(p, '\n', static_cast<size_t>(end - p)));
        if (!lineEnd)
            lineEnd = end;
        detail::Line(b, p, lineEnd);
        p = lineEnd + 1;
    }
    if (!b.lines) {  // a file with no key bindings at all: the game's defaults for every key
        for (Concept& c : b.concepts)
            c = Concept{};
    }
    return b;
}

// ------------------------------------------------------------------ a key ------------------------------------------------------------------

enum class Source : uint8_t {
    kDefault,  // today's key: no file, no key bindings in it, or no lines for this concept (the game's own default)
    kPlayer,   // the player's key for it (today's or another)
    kNone,     // the concept's lines have no key the mod can send: nothing is pressed for it
};

struct Out {
    uint8_t dik = 0;      // the key to send; 0 nothing
    Source source = Source::kDefault;
    uint8_t refused = 0;  // kNone: the first key it has that the mod won't send (0: on the mouse or the pad alone, or unbound)
    bool mouse = false;   // kNone: it has a mouse button or axis
    bool pad = false;     // kNone: it has the pad's
};

inline Out Resolve(const Bindings& b, Key k) {
    Out o;
    const KeyInfo& info = kKeys[k];
    const int ci = ConceptOf(k);
    if (!b.read || !b.lines || ci < 0 || !b.concepts[ci].found) {
        o.dik = info.dik;
        return o;
    }
    const Concept& c = b.concepts[ci];
    for (const Slot& s : c.slot) {
        if (!s.any)
            continue;
        if (s.type == kTypeKey && s.button > 0 && s.button < 255 && (info.negate < 0 || (s.negate != 0) == (info.negate != 0))) {
            if (Sendable(static_cast<unsigned>(s.button))) {
                o.dik = static_cast<uint8_t>(s.button);
                o.source = Source::kPlayer;
                return o;
            }
            if (!o.refused)
                o.refused = static_cast<uint8_t>(s.button);
        } else if (s.type == kTypeMouse && s.button >= 0 && s.button < 255) {
            o.mouse = true;
        } else if (s.type == kTypePad && s.button >= 0 && s.button < 255) {
            o.pad = true;
        }
    }
    o.source = Source::kNone;
    return o;
}

// Whether a concept read is on that mouse button (0 the left, 1 the right) in one of its slots; true when it has no lines (the game's default is).
inline bool OnMouseButton(const Bindings& b, int which, int button) {
    if (!b.read || !b.lines || which < 0 || which >= kConceptCount || !b.concepts[which].found)
        return true;
    for (const Slot& s : b.concepts[which].slot)
        if (s.any && s.type == kTypeMouse && s.button == button && s.axis == 24)
            return true;
    return false;
}

// Every key at once.
struct Table {
    Bindings bindings;
    Out key[kKeyCount];
};

inline void Fill(Table& t) {
    for (int k = 0; k < kKeyCount; ++k)
        t.key[k] = Resolve(t.bindings, static_cast<Key>(k));
}

// The quick-time actions proper a key is on now, as bits in their order from kQteInteractDrag (kQuickTimeDrag InteractDrag, kQuickTimeMelee
// FastMelee, kQuickTimeClimb JumpClimb, kQuickTimeDuck CrouchDuck): what a press of that key is to a fist fight, where a quick-time key that isn't
// its prompt's answer fails it. input.cpp's SendKey asks the fights' guard by these, whichever of the mod's keys the key goes down as: use moved
// onto F is FastMelee's F to a fight, never use's E (the review of 2026-11-25: Y during an E prompt sent F through the guard). 0: none, or no key.
constexpr unsigned kQuickTimeDrag = 1, kQuickTimeMelee = 2, kQuickTimeClimb = 4, kQuickTimeDuck = 8;
inline unsigned QuickTimeOn(const Table& t, unsigned dik) {
    unsigned on = 0;
    if (!dik)
        return on;
    for (int k = kQteInteractDrag; k < kKeyCount; ++k)
        if (t.key[k].dik == dik)
            on |= 1u << (k - kQteInteractDrag);
    return on;
}

// The grips' quick-time press as keys (input.cpp's QteKeys and QteMerged). Each of its bits (E, Space, Left Ctrl, F, Z) pressed apart from the
// buttons' mappings stays down as the mod's key it went down as till it's let go (QuickTimeKeyAs): never let go and pressed again as another key on
// a frame that reads otherwise, a frame the fist fights couldn't be read or a prompt closing into play's Interact beside it (the players lane's
// review, 2026-11-25: with use moved to Q a press prompt's E went up, Q down, then E down again, its one press made two). E, Space and Left Ctrl go
// through their buttons' own mappings (use, jump, crouch-and-hold: one press of that key) while the key each goes down as now is its mapping's own
// (QuickTimeMapped), unless it's down apart as another key: that one stays apart till it's let go. Down apart as the mapping's very key (the test's
// grip with nobody in the headset, then the headset on), it goes to the mapping as before. One down through its mapping goes apart at once when a
// quick-time action's own key, another, comes to be awaited for it (play's Interact, then InteractDrag beside it: the game's E for that, never the
// player's use). With the game's default keys every key is its mapping's own throughout: as ever. Pure (tools/keybindtest.cpp 15).
inline uint16_t QuickTimeKeyAs(bool wanted, uint16_t downAs, uint16_t now) {
    return !wanted ? uint16_t{0} : downAs ? downAs : now;
}
inline bool QuickTimeMapped(bool wanted, bool nowMappingsOwn, bool downApart, bool downApartAsMappingsOwn) {
    return wanted && nowMappingsOwn && (!downApart || downApartAsMappingsOwn);
}

// ------------------------------------------------------------------ a look at the file ------------------------------------------------------------------

// What keybind.cpp's watch does with one look at the settings file. The review of 2026-11-25: a read that failed (the game writing the file, locked) or
// found it empty, while the player's keys were in use, put today's keys in their place for the rest of the run, and nothing read it again until the
// game wrote it once more. Now the keys stay as they were and the file is read again at the next look (5 s on); a read that finds no key bindings
// while the player's are in use is believed the second time running (the game's defaults then).
enum class Kept : uint8_t {
    kNo,      // settled: the next look only when the file changes
    kUnread,  // the file's there but couldn't be read (locked, empty, cut short): the keys as they were, read again at the next look
    kNoKeys,  // read with no key bindings in it while the player's were in use (written just now?): the keys as they were, read again
};
struct Look {
    bool take = false;  // the table of what was read made the keys now (said as ever)
    bool say = false;   // not taken, and said: the first look of a streak that keeps the keys as they were
    Kept kept = Kept::kNo;
};
// read: the file read whole; there: it's there (a read that failed: locked or empty, against gone); lines: the read's key binding lines; have: a
// table's been taken already this run; players: the one taken holds a file's key bindings (the player's); last: the look before's Kept.
inline Look LookAt(bool read, bool there, int lines, bool have, bool players, Kept last) {
    Look l;
    if (!read) {
        l.take = !have;  // (the run's first look: today's keys, as before)
        l.say = have && players && last == Kept::kNo;
        l.kept = there ? Kept::kUnread : Kept::kNo;  // (gone: the file's coming back changes its stamp, which brings the next look)
    } else if (!lines && players && last != Kept::kNoKeys) {
        l.say = true;
        l.kept = Kept::kNoKeys;
    } else {
        l.take = true;
    }
    return l;
}

// What the log says of a table: the keys that differ from today's, those with none, two of the mod's on one key, Fire and Zoom off the mouse
// buttons. Into text; the number of things said.
inline int Describe(const Table& t, char* text, size_t size) {
    size_t used = 0;
    int said = 0;
    const auto add = [&](const char* fmt, auto... args) {
        if (used + 1 >= size)
            return;
        const int n = std::snprintf(text + used, size - used, fmt, args...);
        if (n > 0)
            used = used + static_cast<size_t>(n) < size ? used + static_cast<size_t>(n) : size - 1;
    };
    text[0] = 0;
    char a[40], b2[40];
    for (int k = 0; k < kKeyCount; ++k) {
        const Out& o = t.key[k];
        if (o.source == Source::kPlayer && o.dik != kKeys[k].dik) {
            add("%s%s %s (was %s)", said ? "; " : "", kKeys[k].action, KeyName(o.dik, a, sizeof(a)), KeyName(kKeys[k].dik, b2, sizeof(b2)));
            ++said;
        } else if (o.source == Source::kNone) {
            if (o.refused)
                add("%s%s: nothing pressed (its key %s is one the mod doesn't send)", said ? "; " : "", kKeys[k].action, KeyName(o.refused, a, sizeof(a)));
            else
                add("%s%s: nothing pressed (no key for it in the game's controls: %s)", said ? "; " : "", kKeys[k].action,
                    o.mouse && o.pad ? "the mouse and the pad only" : o.mouse ? "the mouse only" : o.pad ? "the pad only" : "unbound");
            ++said;
        }
    }
    // Two of the mod's keys (not the quick-time ones, which share play's by default) on one key.
    for (int k = 0; k < kQteInteractDrag; ++k)
        for (int j = k + 1; j < kQteInteractDrag; ++j)
            if (t.key[k].dik && t.key[k].dik == t.key[j].dik) {
                add("%s%s and %s are both on %s", said ? "; " : "", kKeys[k].action, kKeys[j].action, KeyName(t.key[k].dik, a, sizeof(a)));
                ++said;
            }
    if (!OnMouseButton(t.bindings, kFire, 0)) {
        add("%sFire isn't on the left mouse button (the triggers press that)", said ? "; " : "");
        ++said;
    }
    if (!OnMouseButton(t.bindings, kZoom, 1)) {
        add("%sZoom isn't on the right mouse button (the left trigger presses that)", said ? "; " : "");
        ++said;
    }
    return said;
}

}  // namespace keybind

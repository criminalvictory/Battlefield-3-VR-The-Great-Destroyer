#pragma once
// The seat the player sits in, if any: which vehicle (its type by its vtable's name, its name as its data has it) and which of its entries (the mounted
// gun's design, docs/research/mounted_gun.md 4.1; src/mountgun.h tells the gun from it). Written as poseread.h is: pure, memory read through the caller's
// reader (input.cpp's guarded one through src/mountseat.cpp; a made-up image in tools/mountguntest.cpp), heap objects and the image only, every object
// checked by its vtable before its fields are read, nothing the game mapped from the graphics card (AMD-safe).
//
// From the local player (god.cpp's chain, read every frame for god mode and the stance: [[[bf3.exe+0x1F80B58]+0x30]+0xBC], its soldier at +0x3D8): the
// controllable it is attached to, its entry index, the controllable's type and its data's name. NONE of those places is measured yet: they are the
// runbook's R3 (dwords.ps1 on the local player on foot and in the seat, -Chain 1F80B58,30,BC,0 from 0x380; findcode.ps1 on the image for the code that
// reads them). Each is a row below, unmeasured, with where it must come from; Prove refuses at the first one (its name said once in the log by
// src/mountseat.cpp), and nothing reads a seat until every row is measured and borne out by the game's own code: until then the seat work is off and the
// game's seats are as they were.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "poseread.h"  // (its pattern match, word read and type name: the same proofs)

namespace seatread {

// The local player, as god.cpp reads it (measured long ago: god mode and the stance read it every frame).
constexpr uint32_t kClientGameContext = 0x1F80B58;  // bf3.exe's image base taken off
constexpr uint32_t kPlayerManager = 0x30, kLocalPlayer = 0xBC, kPlayerSoldier = 0x3D8;

// One place in the game's memory the reader needs: measured or not, and where it must come from.
struct Offset {
    const char* name;
    const char* from;
    bool measured;
    uint32_t value;
};
// One piece of the game's code that bears a place out (its bytes at its address, the image base taken off).
struct Code {
    const char* name;
    const char* from;
    bool measured;
    uint32_t at;
    const char* bytes;  // poseread::Matches' pattern
};
struct Text {
    const char* name;
    const char* from;
    bool measured;
    const char* value;
};

constexpr int kCodeMost = 4;
struct Layout {
    Offset attached;     // ClientPlayer: the controllable it is attached to (a vehicle while seated; 0 or the soldier on foot)
    Offset entry;        // ClientPlayer: its entry index in that controllable (an int)
    Text vehicleType;    // the attached controllable's type name when it's a vehicle (its vtable's getType)
    Offset data;         // the vehicle: its data
    Text dataType;       // the data's type name
    Offset name;         // the data: its name (a pointer to the characters if nameIsPointer, else the characters there)
    bool nameIsPointer;
    Code code[kCodeMost];
    int codeCount;
};

// The game's layout: every row unmeasured (R3 has not been run). Expected kinds only, in the words of the dump's types: nothing here is read until measured.
inline constexpr Layout kLayout = {
    {"the player's attached controllable (ClientPlayer +?)", "R3: dwords.ps1 -Chain 1F80B58,30,BC,0 -Start 0x380 -Length 0xC0, on foot and in the seat",
     false, 0},
    {"the player's entry index in it (ClientPlayer +?)", "R3: the same dump: a small integer, 1 in the gunner's seat", false, 0},
    {"the vehicle's type name", "R3: owner.ps1 or the vtable's getType on the pointer that changes (ClientVehicleEntity or its kind expected)", false,
     nullptr},
    {"the vehicle's data (+?)", "R3: fields.ps1 on the vehicle", false, 0},
    {"the vehicle data's type name", "R3: its vtable's getType (VehicleEntityData or its kind expected)", false, nullptr},
    {"the vehicle data's name (+?)", "R3: the data's name field (Venice-EBX: its asset's name)", false, 0},
    false,
    {{"the code that reads the attached controllable", "R3: findcode.ps1 on build\\bf3_mem.exe for the offset's reads", false, 0, nullptr},
     {"the code that reads the entry index", "R3: findcode.ps1 likewise", false, 0, nullptr}},
    2,
};

// The first row of a layout not measured (its name and where it must come from); nullptr: all measured.
inline const char* FirstUnmeasured(const Layout& l, const char** from = nullptr) {
    const Offset* offsets[] = {&l.attached, &l.entry, &l.data, &l.name};
    const Text* texts[] = {&l.vehicleType, &l.dataType};
    const auto said = [&](const char* name, const char* where) {
        if (from)
            *from = where;
        return name;
    };
    if (!l.attached.measured)
        return said(l.attached.name, l.attached.from);
    if (!l.entry.measured)
        return said(l.entry.name, l.entry.from);
    if (!l.vehicleType.measured || !l.vehicleType.value)
        return said(l.vehicleType.name, l.vehicleType.from);
    for (const Offset* o : offsets)
        if (!o->measured)
            return said(o->name, o->from);
    for (const Text* t : texts)
        if (!t->measured || !t->value)
            return said(t->name, t->from);
    if (l.codeCount < 1)
        return said("the code that bears the places out", "R3: findcode.ps1");
    for (int i = 0; i < l.codeCount && i < kCodeMost; ++i)
        if (!l.code[i].measured || !l.code[i].bytes)
            return said(l.code[i].name, l.code[i].from);
    return nullptr;
}

// ------------------------------------------------------------------ the proofs ------------------------------------------------------------------

struct Proofs {
    bool ok = false;
    const char* failed = nullptr;  // the first row not measured, or not borne out
    const char* from = nullptr;    // ... where it must come from
    uint32_t at = 0;               // ... where its code was looked for (a code row)
    bool unmeasured = false;       // ... it isn't measured yet (as against measured and not borne out)
};

// Every row measured, then each code row's bytes where it says (one not as the layout has it: another bf3.exe, the reader off for the run).
template <class Reader>
Proofs Prove(const Reader& read, uint32_t base, const Layout& l) {
    Proofs p;
    if (const char* missing = FirstUnmeasured(l, &p.from)) {
        p.failed = missing;
        p.unmeasured = true;
        return p;
    }
    for (int i = 0; i < l.codeCount && i < kCodeMost; ++i)
        if (!poseread::Matches(read, base + l.code[i].at, l.code[i].bytes)) {
            p.failed = l.code[i].name;
            p.from = l.code[i].from;
            p.at = base + l.code[i].at;
            return p;
        }
    p.ok = true;
    return p;
}

// ------------------------------------------------------------------ a frame's read ------------------------------------------------------------------

enum class Miss : uint8_t {
    kNone,
    kNoPlayer,    // no local player (a menu, a load)
    kEntry,       // in a vehicle, its entry index not one (0 to 15)
    kNotVehicle,  // attached to something that isn't a vehicle of the layout's type
    kData,        // the vehicle's data isn't of the layout's type, or not readable
    kName,        // its name not readable or not a name
};
inline const char* MissName(Miss m) {
    switch (m) {
    case Miss::kNone: return "read";
    case Miss::kNoPlayer: return "no local player";
    case Miss::kEntry: return "the entry index isn't one";
    case Miss::kNotVehicle: return "attached to something not a vehicle of the layout's type";
    case Miss::kData: return "the vehicle's data isn't of the layout's type";
    case Miss::kName: return "the vehicle's name isn't readable";
    }
    return "?";
}

// The type names already looked up, by vtable (a few reads a frame instead of a dozen): what a vtable was found to be named, yes or no.
struct TypeCache {
    static constexpr int kMost = 8;
    uint32_t vtable[kMost] = {};
    bool named[kMost] = {};
    const char* want[kMost] = {};
    int next = 0;
};
template <class Reader>
bool Named(const Reader& read, TypeCache& cache, uint32_t vtable, const char* want) {
    for (int i = 0; i < TypeCache::kMost; ++i)
        if (cache.vtable[i] == vtable && cache.want[i] == want && vtable)
            return cache.named[i];
    const bool named = poseread::TypeNamed(read, vtable, want);
    const int i = cache.next;
    cache.vtable[i] = vtable, cache.named[i] = named, cache.want[i] = want;
    cache.next = (cache.next + 1) % TypeCache::kMost;
    return named;
}

struct Seat {
    bool ok = false;      // read: vehicle says whether the player sits in one
    Miss miss = Miss::kNoPlayer;
    bool vehicle = false;
    uint32_t player = 0, controllable = 0, data = 0;
    int entry = -1;
    char name[64] = {};
};

// One frame's read (with the layout's every row measured and proved: Prove first). On foot (attached to nothing, or to something not of the vehicle's
// type while the entry says none) is a read: ok, vehicle false.
template <class Reader>
Seat Read(const Reader& read, uint32_t base, const Layout& l, TypeCache& cache) {
    Seat s;
    bool got = false;
    const uint32_t context = poseread::Word(read, base + kClientGameContext, &got);
    const uint32_t manager = got && context ? poseread::Word(read, context + kPlayerManager, &got) : 0;
    const uint32_t player = got && manager ? poseread::Word(read, manager + kLocalPlayer, &got) : 0;
    if (!got || !player)
        return s;
    s.player = player;
    const uint32_t attached = poseread::Word(read, player + l.attached.value, &got);
    if (!got) {
        s.miss = Miss::kNoPlayer;
        return s;
    }
    const uint32_t entry = poseread::Word(read, player + l.entry.value, &got);
    if (!attached) {
        s.ok = got;
        s.miss = got ? Miss::kNone : Miss::kEntry;
        return s;
    }
    const uint32_t vtable = poseread::Word(read, attached, &got);
    if (!got || !Named(read, cache, vtable, l.vehicleType.value)) {
        // (attached to the soldier itself, or anything else: no vehicle's seat)
        s.ok = got;
        s.miss = got ? Miss::kNone : Miss::kNotVehicle;
        return s;
    }
    s.controllable = attached;
    if (entry > 15) {
        s.miss = Miss::kEntry;
        return s;
    }
    s.entry = static_cast<int>(entry);
    const uint32_t data = poseread::Word(read, attached + l.data.value, &got);
    if (!got || !data || !Named(read, cache, poseread::Word(read, data), l.dataType.value)) {
        s.miss = Miss::kData;
        return s;
    }
    s.data = data;
    uint32_t text = data + l.name.value;
    if (l.nameIsPointer) {
        text = poseread::Word(read, data + l.name.value, &got);
        if (!got || !text) {
            s.miss = Miss::kName;
            return s;
        }
    }
    // A name: printable, ended within its room.
    char name[sizeof(s.name)] = {};
    if (!read(text, name, sizeof(name) - 1)) {
        // (near the end of a mapped piece: a shorter read)
        if (!read(text, name, 16)) {
            s.miss = Miss::kName;
            return s;
        }
    }
    size_t n = 0;
    while (n < sizeof(name) - 1 && name[n] >= 0x20 && name[n] < 0x7F)
        ++n;
    if (n == 0 || (n < sizeof(name) - 1 && name[n] != 0)) {
        s.miss = Miss::kName;
        return s;
    }
    std::memcpy(s.name, name, n);
    s.name[n] = 0;
    s.vehicle = true;
    s.ok = true;
    s.miss = Miss::kNone;
    return s;
}

}  // namespace seatread

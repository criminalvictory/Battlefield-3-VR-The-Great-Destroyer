#pragma once
// A held gun's scatter rows (skin.cpp's ClearRifleSpread) where another gun's gun sway is its own to the bit. The Mk11 Mod 0's GunSwayData is the SVD's
// in every number (tools/recoil/stances.tsv, the install's own: each stance zoomed and not, its recoil's too), so the scan for its numbers finds the SVD's
// data as well wherever both are loaded: Night Shift (its snipers' SVDs, its own Mk11), Kaffarov (both). Written over, the SVD's hip scatter would have
// been taken down to its zoomed one for every soldier carrying an SVD. So such a gun takes only the GunSwayData its own firing data names, as the AT4's
// and the SMAW's rocket starts are told apart by their FiringFunctionData (at4shot.h): a weapon of the soldier's own, each slot's firing state ->
// +0x94 its WeaponFiringData -> +0x20 its GunSwayData (recoilrun.cpp's read of every gun's own kick, borne out in the game since 2026-11-11), whose own
// FiringFunctionData (that WeaponFiringData's +0x08, the PrimaryFire) shoots at this gun's speed (its shot's InitialSpeed.z at +0x88: the Mk11's 550 m/s
// is no other gun's; the SVD's 530). A block no such weapon names is never written, nor is one when no weapon could be read: then the gun keeps the
// game's own hip scatter (the log says so). Pure, header only: skin.cpp's ClearRifleSpread, tools/mk11test.cpp on made-up memory.
// (The table's only other twin in stances.tsv: the M1014's rows are the DAO-12's, and its buckshot's 300 m/s the DAO-12's too. Not guarded yet: the
// weapons catalog has the M1014 on the trains, Guillotine and Comrades, the DAO-12 in Kaffarov's drops; a level found to load both wants a row here for
// it, told by the slot's sway alone, the speeds being the same.)

#include <cstddef>
#include <cstdint>

namespace swayown {

constexpr uint32_t kStateData = 0x94;    // a weapon's firing state -> its WeaponFiringData (ammo.cpp's, at4shot.h's, recoilrun.cpp's kFiringData)
constexpr uint32_t kPrimaryFire = 0x08;  // WeaponFiringData -> its FiringFunctionData (recoilrun.cpp's kPrimaryFire)
constexpr uint32_t kWeaponSway = 0x20;   // WeaponFiringData -> its GunSwayData (recoilrun.cpp's kWeaponSway)
constexpr uint32_t kSpeedAt = 0x88;      // FiringFunctionData: its shot's InitialSpeed.z (recoilrun.cpp's kSpeedZ; ClearRifleSpread's shot + 10 floats)
// GunSwayData: the scan's standing block (Stand.NoZoom: its six groups and its recoil, 30 floats) right after the class's header (its vtable and count):
// its recoil at +0x68, recoilrun.cpp's kRecoilRows[0], 24 floats on (the AT4's on the roof: its vtable at E8860BC0, its standing block from E8860BC8).
// A block is taken for a GunSwayData's anywhere from there to the data's end (recoilrun.cpp's kSwaySize): another gun's data never lies inside it.
constexpr uint32_t kStandAt = 0x08, kSwayBytes = 0x328;
constexpr int kSlots = 8;                // the soldier's slots ammo::FiringOf reads (0-7)

// A gun whose sway numbers are another gun's to the bit: its row in ClearRifleSpread's table, its bullets' speed (its own FiringFunctionData's), whose.
struct Twin {
    size_t gun;
    float speed;
    const char* other;
};
constexpr size_t kMk11Row = 13;  // (skin.cpp's kHeldKinds row for the Mk11 names it; ClearRifleSpread holds its row to kMk11Speed)
constexpr float kMk11Speed = 550.0f;
constexpr Twin kTwins[] = {{kMk11Row, kMk11Speed, "the SVD's"}};

// The twin entry for that row of ClearRifleSpread's table, or null (a gun whose numbers are its own alone: taken as before).
inline const Twin* TwinOf(size_t gun) {
    for (const Twin& t : kTwins)
        if (t.gun == gun)
            return &t;
    return nullptr;
}

// One weapon of the soldier's own as read now (each read through the game's memory may fail: read false, nothing of it used).
struct Slot {
    bool read = false;   // its firing state, WeaponFiringData, GunSwayData pointer and FiringFunctionData's speed all read
    uint32_t sway = 0;   // its GunSwayData
    float speed = 0.0f;  // its FiringFunctionData's shot's InitialSpeed.z
};

// Whether the standing block the scan found at place is the gun's own: some weapon of the soldier's own names the GunSwayData it lies in (at
// kStandAt, or anywhere up to kSwayBytes) and shoots at the gun's speed.
inline bool Own(uintptr_t place, float speed, const Slot* slots, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        const Slot& s = slots[i];
        const uintptr_t data = s.sway;
        if (s.read && data && place >= data + kStandAt && place < data + kSwayBytes && s.speed == speed)
            return true;
    }
    return false;
}

// How many of the slots were read (none: nothing could tell its own data, and nothing is taken).
inline int Read(const Slot* slots, size_t count) {
    int n = 0;
    for (size_t i = 0; i < count; ++i)
        n += slots[i].read;
    return n;
}

}  // namespace swayown

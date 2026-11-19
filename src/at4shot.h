#pragma once
// The rocket launchers' (the AT4's, Operation Swordbreaker's M136, and the SMAW's, Rock and a Hard Place's) rocket's start in their own data: which
// ShotConfigData is whose. Pure, header only: skin.cpp's FindAt4 and ClearLauncherSpread and their writers, tools/at4test.cpp against the roof's
// dump (build\survey\AT4R_R10_scan.txt).
// Their ShotConfigData (InitialPosition (-0.3 -0.1 2.0), InitialDirection (0 0.01 1), InitialSpeed (0 0 40), each padded to 16 bytes, from the game's
// eye) is one to the bit for the M136, the SMAW, the RPG-7 and the enemies' RPG-7 (RPG7_AI: Swordbreaker's RPG soldiers) (Venice-EBX); their
// FiringFunctionData isn't. That is the class's (its vtable 021C2A68 at its start: both of the roof's two exact matches had it, the M136's and
// RPG7_AI's, each its own instance's GUID 16 bytes before it) with the shot at +0x60, its RateOfFire at +0x120 and its ReloadTime at +0x13C:
// bf3_mem.exe's own reflection has FiringFunctionData's Shot at +0x60 and its FireLogic at +0xC0, FireLogicData's RateOfFire at +0x60, its
// ReloadThreshold at +0x74 and its ReloadTime at +0x7C; the roof's dump of the M136's reads 350, 0.8 and 4.2 there, its Venice-EBX numbers. Each
// one's RateOfFire and ReloadTime (Venice-EBX's FiringFunctionData): the M136's 350 and 4.2, the SMAW's 350 and 5.8, the RPG-7's 100 and 4.8,
// RPG7_AI's 18 and 4.8. Only the gun's own in a hand is ever written.
// The M136's numbers were read on the roof; the SMAW's never were in this game (no Rock and a Hard Place dump), so the SMAW's own is told first by
// its slot: the FiringFunctionData the soldier's own weapon points at (its firing state's WeaponFiringData at +0x94, ammo.cpp's and recoilrun.cpp's;
// that data's PrimaryFire at +0x08, bf3_mem.exe's own field table, the way recoilrun reads every gun's RateOfFire; on the roof the AT4's GunSwayData
// that data names sat right after the FiringFunctionData the scan found), whatever its numbers; then by Venice-EBX's numbers; then, neither telling
// it, as the scan before the AT4 took it, the known others left out (SmawOwn).
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace at4shot {

constexpr uint32_t kFiringVtable = 0x021C2A68;                          // FiringFunctionData's (bf3.exe 1.6's image: fixed)
constexpr uint32_t kShotAt = 0x60, kRateAt = 0x120, kReloadAt = 0x13C;  // in it: its ShotConfigData, RateOfFire, ReloadTime
constexpr uint32_t kFiringBytes = 0x140;                                // read from its start: through its ReloadTime
constexpr uint32_t kStateData = 0x94, kPrimaryFire = 0x08;              // a weapon's firing state -> its WeaponFiringData -> its FiringFunctionData
constexpr float kRate = 350.0f, kReload = 4.2f;                         // the M136's (to the bit: 43AF0000, 40866666)
constexpr float kStart[3] = {-0.3f, -0.1f, 2.0f};                       // its data's start (the SMAW's too)

// Each launcher whose ShotConfigData is the M136's to the bit, by its own FiringFunctionData's RateOfFire and ReloadTime (to the bit).
struct Launcher {
    const char* name;
    float rate, reload;
};
constexpr Launcher kM136{"the AT4", kRate, kReload};
constexpr Launcher kSmaw{"the SMAW", 350.0f, 5.8f};       // (43AF0000, 40B9999A)
constexpr Launcher kRpg7{"the RPG-7", 100.0f, 4.8f};      // (42C80000, 4099999A)
constexpr Launcher kRpg7Ai{"the enemies' RPG-7", 18.0f, 4.8f};  // (41900000, 4099999A)
constexpr Launcher kLaunchers[] = {kM136, kSmaw, kRpg7, kRpg7Ai};

// The ShotConfigData at there (11 floats: the start, its pad, the direction, its pad, the speed): the direction and speed as its data has them, and the
// start as its data has it or as last written there (last; null: the data's alone).
inline bool ShotOurs(const float* there, const float* last) {
    const bool start = std::fabs(there[0] - kStart[0]) < 1e-4f && std::fabs(there[1] - kStart[1]) < 1e-4f && std::fabs(there[2] - kStart[2]) < 1e-4f;
    return there[4] == 0.0f && std::fabs(there[5] - 0.01f) < 1e-4f && std::fabs(there[6] - 1.0f) < 1e-4f && there[8] == 0.0f && there[9] == 0.0f &&
           std::fabs(there[10] - 40.0f) < 1e-3f && (start || (last && std::memcmp(there, last, 3 * sizeof(float)) == 0));
}

// The FiringFunctionData whose first kFiringBytes are at firing: that launcher's own (its vtable, its RateOfFire and its ReloadTime to the bit), its
// shot as ShotOurs has it (last: the start last written there; null: the data's alone).
inline bool FiringIs(const uint8_t* firing, const Launcher& who, const float* last) {
    uint32_t vtable = 0;
    float rate = 0.0f, reload = 0.0f, shot[11];
    std::memcpy(&vtable, firing, sizeof(vtable));
    std::memcpy(&rate, firing + kRateAt, sizeof(rate));
    std::memcpy(&reload, firing + kReloadAt, sizeof(reload));
    std::memcpy(shot, firing + kShotAt, sizeof(shot));
    return vtable == kFiringVtable && rate == who.rate && reload == who.reload && ShotOurs(shot, last);
}

// The AT4's own (FiringIs with the M136's numbers).
inline bool FiringOurs(const uint8_t* firing, const float* last) {
    return FiringIs(firing, kM136, last);
}

// Whose FiringFunctionData it is, of kLaunchers, by its numbers (its shot as ShotOurs has it, last as there): its name, or null (another gun's, or
// another class's). For the logs.
inline const char* WhoseFiring(const uint8_t* firing, const float* last) {
    for (const Launcher& who : kLaunchers)
        if (FiringIs(firing, who, last))
            return who.name;
    return nullptr;
}

// The known launchers other than the SMAW whose shot is its own to the bit (the AT4's, the RPG-7's, the enemies' RPG-7's), by their numbers: the
// name of the one it is, or null.
constexpr const Launcher* kNotSmaw[] = {&kM136, &kRpg7, &kRpg7Ai};
inline const char* OtherThanSmaw(const uint8_t* firing, const float* last) {
    for (const Launcher* who : kNotSmaw)
        if (FiringIs(firing, *who, last))
            return who->name;
    return nullptr;
}

// FiringIs without the numbers: the class's vtable at its start, its shot as ShotOurs has it.
inline bool ClassAndShot(const uint8_t* firing, const float* last) {
    uint32_t vtable = 0;
    float shot[11];
    std::memcpy(&vtable, firing, sizeof(vtable));
    std::memcpy(shot, firing + kShotAt, sizeof(shot));
    return vtable == kFiringVtable && ShotOurs(shot, last);
}

// Its RateOfFire and ReloadTime as they are there (to the bit), named: what FiringIs holds that place to from then on.
inline Launcher NumbersOf(const uint8_t* firing, const char* name) {
    Launcher numbers{name, 0.0f, 0.0f};
    std::memcpy(&numbers.rate, firing + kRateAt, sizeof(numbers.rate));
    std::memcpy(&numbers.reload, firing + kReloadAt, sizeof(numbers.reload));
    return numbers;
}

// The SMAW's own among the scan's exact matches of its rocket's start (skin.cpp's ClearLauncherSpread; Rock and a Hard Place), by the first rule that
// takes any, in this order:
//   kSlot     each one whose FiringFunctionData is a weapon's of the soldier's own (fires: each slot's [[firing state + kStateData] + kPrimaryFire], 0
//             none read), its class's with the shot, no other known launcher's by its numbers: the SMAW's own, whatever its numbers are;
//   kNumbers  each one with the SMAW's numbers as Venice-EBX has them (kSmaw);
//   kNoOther  each one whose FiringFunctionData is its class's with the shot and no known launcher's (the scan before the AT4 took every match:
//             this is that, the AT4's and both RPG-7s' left out);
//   kNone     none: nothing taken.
// Never the AT4's, the RPG-7's or the enemies' RPG-7's by their numbers, nor one whose firing data couldn't be read.
enum class SmawBy : int { kNone = 0, kSlot = 1, kNumbers = 2, kNoOther = 3 };
struct Match {
    uintptr_t place = 0;                // its ShotConfigData (its FiringFunctionData starts kShotAt before it)
    bool read = false;                  // its FiringFunctionData's first kFiringBytes read
    uint8_t firing[kFiringBytes] = {};  // ... as read
    bool own = false;                   // SmawOwn's answer: taken for the SMAW's own
};
// The slot of the soldier's weapon whose FiringFunctionData m's is (its class's, with the shot, no other known launcher's), or -1.
inline int SmawSlotOf(const Match& m, const uintptr_t* fires, size_t fireCount) {
    if (!m.read || m.place <= kShotAt || !ClassAndShot(m.firing, nullptr) || OtherThanSmaw(m.firing, nullptr))
        return -1;
    for (size_t k = 0; k < fireCount; ++k)
        if (fires[k] && fires[k] == m.place - kShotAt)
            return static_cast<int>(k);
    return -1;
}
// Each match's own set; the rule that took them.
inline SmawBy SmawOwn(Match* matches, size_t count, const uintptr_t* fires, size_t fireCount) {
    for (int rule = 1; rule <= 3; ++rule) {
        bool any = false;
        for (size_t i = 0; i < count; ++i) {
            Match& m = matches[i];
            m.own = rule == 1   ? SmawSlotOf(m, fires, fireCount) >= 0
                    : rule == 2 ? m.read && FiringIs(m.firing, kSmaw, nullptr)
                                : m.read && ClassAndShot(m.firing, nullptr) && !OtherThanSmaw(m.firing, nullptr);
            any = any || m.own;
        }
        if (any)
            return static_cast<SmawBy>(rule);
    }
    return SmawBy::kNone;
}

}  // namespace at4shot

#pragma once
// The AT4's (Operation Swordbreaker's M136) rocket's start in its own data: which ShotConfigData is its own. Pure, header only: skin.cpp's FindAt4 and
// its writers, tools/at4test.cpp against the roof's dump (build\survey\AT4R_R10_scan.txt).
// Its ShotConfigData (InitialPosition (-0.3 -0.1 2.0), InitialDirection (0 0.01 1), InitialSpeed (0 0 40), each padded to 16 bytes, from the game's
// eye) is the SMAW's, the RPG-7's and the enemies' RPG-7's (RPG7_AI: Swordbreaker's RPG soldiers) to the bit (Venice-EBX); its FiringFunctionData
// isn't. That is the class's (its vtable 021C2A68 at its start: both of the roof's two exact matches had it) with the shot at +0x60, its RateOfFire at
// +0x120 and its ReloadTime at +0x13C (the roof's dump of the AT4's: 350 and 4.2, its ReloadThreshold 0.8 at +0x134): the M136's 350 and 4.2, the
// SMAW's 350 and 5.8, the RPG-7's 100 and 4.8, RPG7_AI's 18 and 4.8. Only the AT4's own is ever written.
#include <cmath>
#include <cstdint>
#include <cstring>

namespace at4shot {

constexpr uint32_t kFiringVtable = 0x021C2A68;                          // FiringFunctionData's (bf3.exe 1.6's image: fixed)
constexpr uint32_t kShotAt = 0x60, kRateAt = 0x120, kReloadAt = 0x13C;  // in it: its ShotConfigData, RateOfFire, ReloadTime
constexpr uint32_t kFiringBytes = 0x140;                                // read from its start: through its ReloadTime
constexpr float kRate = 350.0f, kReload = 4.2f;                         // the M136's (to the bit: 43AF0000, 40866666)
constexpr float kStart[3] = {-0.3f, -0.1f, 2.0f};                       // its data's start (the SMAW's too)

// The ShotConfigData at there (11 floats: the start, its pad, the direction, its pad, the speed): the direction and speed as its data has them, and the
// start as its data has it or as last written there (last; null: the data's alone).
inline bool ShotOurs(const float* there, const float* last) {
    const bool start = std::fabs(there[0] - kStart[0]) < 1e-4f && std::fabs(there[1] - kStart[1]) < 1e-4f && std::fabs(there[2] - kStart[2]) < 1e-4f;
    return there[4] == 0.0f && std::fabs(there[5] - 0.01f) < 1e-4f && std::fabs(there[6] - 1.0f) < 1e-4f && there[8] == 0.0f && there[9] == 0.0f &&
           std::fabs(there[10] - 40.0f) < 1e-3f && (start || (last && std::memcmp(there, last, 3 * sizeof(float)) == 0));
}

// The FiringFunctionData whose first kFiringBytes are at firing: the AT4's own (its vtable, its RateOfFire and its ReloadTime to the bit), its shot
// as ShotOurs has it (last: the start last written there; null: the data's alone).
inline bool FiringOurs(const uint8_t* firing, const float* last) {
    uint32_t vtable = 0;
    float rate = 0.0f, reload = 0.0f, shot[11];
    std::memcpy(&vtable, firing, sizeof(vtable));
    std::memcpy(&rate, firing + kRateAt, sizeof(rate));
    std::memcpy(&reload, firing + kReloadAt, sizeof(reload));
    std::memcpy(shot, firing + kShotAt, sizeof(shot));
    return vtable == kFiringVtable && rate == kRate && reload == kReload && ShotOurs(shot, last);
}

}  // namespace at4shot

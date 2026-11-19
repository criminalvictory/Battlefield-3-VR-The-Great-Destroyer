#pragma once
// Every campaign weapon's recoil, from the game's own data (tools/recoil/*.tsv; written by tools/recoilgen.ps1, never by hand). The gun in hand
// reads its own numbers live (recoilrun.cpp); this is how it's known, how big its kick in the hands is, and its numbers when it has no gun sway.
//   - The game's own (guns.tsv, stances.tsv, unlocks.tsv: the install's data, patch over base): the rate, the muzzle speed, the magazine, the
//     pellets, each stance's and zoom's climb a round (up, sideways, how fast it comes back) and caps, the first shot's multiplier, the scale
//     while firing, the camera spring, the rumble, and each sight's ZoomedKickbackFactor.
//   - kick.tsv's, each line with its source: the kind, the draw and the shape it's known by, and the kick in the hands, one round from rest with
//     two hands at the hip. A recorded gun's from its own recording of the game's kick: the size whose drawn peak, the drift's answer to the
//     round with it, is the recording's; the M416's the anchor; a gun not recorded is a placeholder, its physics' square root about the M416's
//     (the pistols' and the launchers' the design's own).
//     Its timings are its class's, its roll, sideways share and jitter the two-hand hold's. Held fire's push back: none for a gun the game fires a
//     round at a time, a recorded loop's own, else recoil.h's.
// One row a blueprint, 58: the 40 guns, the launchers, the unused (the A-91, the Glock 18, the .44, the enemies' RPG-7) and the 9 with no gun
// sway (kind kNone: no recoil). The M320's row is also the one under the M416, whose 40 mm is the same data. The Stinger and the Javelin share
// theirs: the Stinger, which is drawn, comes first.

#include <cstdint>
#include <cstring>

#include "recoil.h"

namespace recoilguns {

struct Row {
    const char* name;       // "M416": recoil.txt's "gun <name>", the logs
    const char* blueprint;  // "weapons/m416/m416"
    const char* shape;      // skin.cpp's HeldKind::shape ("an94" ...), nullptr if none
    recoil::Kind kind;
    uint32_t body;          // its first-person body's colour draw (the profile's id; the train pistol recoilrun.h's kTrainPistol), 0 not known
    float rate, speed;      // RateOfFire, InitialSpeed.z
    int magazine, pellets;  // Ammo.MagazineCapacity, NumberOfBulletsPerShell
    float sig[8];           // its standing hip row and gun numbers: up, hmin, hmax, decrease, firstShot, shootingScale, spring, damping
    float up[3][2], sideMin[3][2], sideMax[3][2], decrease[3][2];  // [stand, crouch, prone][hip, zoomed]
    float capUp, capSide;
    float rumble[3];        // LowRumble, HighRumble, RumbleDuration (s)
    recoil::Kick kick;      // the kick in the hands: two hands at the hip
    float pairGap;          // seconds: two rounds within it are a burst pair (the AN-94 0.075), else 0
    float eye[8];           // ZoomedKickbackFactor: irons (the gun's own), Kobra, PK-AS, PK-A, PSO-1, PKS-07, ACOG, another 6x+ scope (its
                            // campaign scope if it has one, else its 6x rifle scope, else its ballistic); one it can't carry: the usual
};

inline constexpr Row kRows[] = {
    // M9: given, a placeholder: 1.0 cm, 4.0 deg
    {"M9", "weapons/m9/m9", nullptr, recoil::Kind::kPistol, 1, 400.0f, 380.0f, 16, 1,
     {0.7f, -0.1f, 0.1f, 20.0f, 1.0f, 0.2f, 150.0f, 0.35f},
     {{0.7f, 0.7f}, {0.7f, 0.7f}, {0.7f, 0.7f}}, {{-0.1f, -0.1f}, {-0.1f, -0.1f}, {-0.1f, -0.1f}},
     {{0.1f, 0.1f}, {0.1f, 0.1f}, {0.1f, 0.1f}}, {{20.0f, 20.0f}, {20.0f, 20.0f}, {20.0f, 20.0f}},
     10.0f, 10.0f, {0.39f, 0.6f, 0.2f},
     {.back = {0.01f, 0.025f, 0.8f}, .rise = {4.0f, 0.035f, 0.8f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.5f, .yaw = 0.1f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
    // MP-443: given, a placeholder: 1.0 cm, 3.8 deg
    {"MP-443", "weapons/mp443/mp443", nullptr, recoil::Kind::kPistol, 0, 400.0f, 320.0f, 18, 1,
     {0.7f, -0.1f, 0.1f, 20.0f, 1.0f, 0.2f, 250.0f, 0.35f},
     {{0.7f, 0.7f}, {0.7f, 0.7f}, {0.7f, 0.7f}}, {{-0.1f, -0.1f}, {-0.1f, -0.1f}, {-0.1f, -0.1f}},
     {{0.1f, 0.1f}, {0.1f, 0.1f}, {0.1f, 0.1f}}, {{20.0f, 20.0f}, {20.0f, 20.0f}, {20.0f, 20.0f}},
     10.0f, 10.0f, {0.39f, 0.6f, 0.2f},
     {.back = {0.01f, 0.025f, 0.8f}, .rise = {3.8f, 0.035f, 0.8f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.5f, .yaw = 0.1f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
    // UMP-45: recorded: 1.76 cm; 1.98 deg (at 78 ms) drawn with the drift's 0.21 where they peak, 1.8; held fire 2.33 cm over 1.76, 1.32
    {"UMP-45", "weapons/ump45/ump45", "ump45", recoil::Kind::kSmg, 19314, 600.0f, 320.0f, 26, 1,
     {0.25f, -0.2f, 0.2f, 20.0f, 2.75f, 0.1f, 250.0f, 0.35f},
     {{0.25f, 0.25f}, {0.25f, 0.25f}, {0.25f, 0.25f}}, {{-0.2f, -0.2f}, {-0.2f, -0.2f}, {-0.2f, -0.2f}},
     {{0.2f, 0.2f}, {0.2f, 0.2f}, {0.2f, 0.2f}}, {{20.0f, 20.0f}, {20.0f, 20.0f}, {20.0f, 20.0f}},
     10.0f, 10.0f, {0.34f, 0.8f, 0.2f},
     {.back = {0.0176f, 0.03f, 0.75f}, .rise = {1.8f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 1.32f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // MP7: physics, a placeholder: 1.5 sqrt(0.5) = 1.1 cm, 1.8 sqrt(0.5 / 1.2) = 1.2 deg
    {"MP7", "weapons/mp7/mp7", nullptr, recoil::Kind::kSmg, 0, 950.0f, 390.0f, 21, 1,
     {0.12f, -0.45f, 0.45f, 20.0f, 2.2f, 0.1f, 250.0f, 0.35f},
     {{0.12f, 0.12f}, {0.12f, 0.12f}, {0.12f, 0.12f}}, {{-0.45f, -0.45f}, {-0.45f, -0.45f}, {-0.45f, -0.45f}},
     {{0.45f, 0.45f}, {0.45f, 0.45f}, {0.45f, 0.45f}}, {{20.0f, 20.0f}, {20.0f, 20.0f}, {20.0f, 20.0f}},
     10.0f, 10.0f, {0.2f, 1.0f, 0.07f},
     {.back = {0.011f, 0.03f, 0.75f}, .rise = {1.2f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // P90: physics, a placeholder: 1.5 sqrt(0.5) = 1.1 cm, 1.8 sqrt(0.5 / 1.2) = 1.2 deg
    {"P90", "weapons/p90/p90", nullptr, recoil::Kind::kSmg, 0, 900.0f, 420.0f, 51, 1,
     {0.18f, -0.4f, 0.4f, 20.0f, 2.0f, 0.1f, 250.0f, 0.35f},
     {{0.18f, 0.18f}, {0.18f, 0.18f}, {0.18f, 0.18f}}, {{-0.4f, -0.4f}, {-0.4f, -0.4f}, {-0.4f, -0.4f}},
     {{0.4f, 0.4f}, {0.4f, 0.4f}, {0.4f, 0.4f}}, {{20.0f, 20.0f}, {20.0f, 20.0f}, {20.0f, 20.0f}},
     10.0f, 10.0f, {0.2f, 1.0f, 0.07f},
     {.back = {0.011f, 0.03f, 0.75f}, .rise = {1.2f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // PP-2000: physics, a placeholder: 1.5 sqrt(1.0) = 1.5 cm, 1.8 sqrt(1.1 / 1.2) = 1.7 deg
    {"PP-2000", "weapons/pp2000/pp2000", nullptr, recoil::Kind::kSmg, 0, 650.0f, 360.0f, 21, 1,
     {0.18f, -0.2f, 0.3f, 20.0f, 2.5f, 0.1f, 800.0f, 0.6f},
     {{0.18f, 0.18f}, {0.18f, 0.18f}, {0.18f, 0.18f}}, {{-0.2f, -0.2f}, {-0.2f, -0.2f}, {-0.2f, -0.2f}},
     {{0.3f, 0.3f}, {0.3f, 0.3f}, {0.3f, 0.3f}}, {{20.0f, 20.0f}, {20.0f, 20.0f}, {20.0f, 20.0f}},
     10.0f, 10.0f, {0.34f, 0.6f, 0.2f},
     {.back = {0.015f, 0.03f, 0.75f}, .rise = {1.7f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // PDW-R: physics, a placeholder: 1.5 sqrt(1.1) = 1.6 cm, 1.8 sqrt(1.2 / 1.2) = 1.8 deg
    {"PDW-R", "weapons/magpulpdr/magpulpdr", nullptr, recoil::Kind::kSmg, 0, 750.0f, 430.0f, 21, 1,
     {0.2f, -0.4f, 0.2f, 18.0f, 2.0f, 0.1f, 250.0f, 0.35f},
     {{0.2f, 0.2f}, {0.2f, 0.2f}, {0.2f, 0.2f}}, {{-0.4f, -0.4f}, {-0.4f, -0.4f}, {-0.4f, -0.4f}},
     {{0.2f, 0.2f}, {0.2f, 0.2f}, {0.2f, 0.2f}}, {{18.0f, 18.0f}, {18.0f, 18.0f}, {18.0f, 18.0f}},
     10.0f, 10.0f, {0.2f, 1.0f, 0.12f},
     {.back = {0.016f, 0.03f, 0.75f}, .rise = {1.8f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // AS Val: physics, a placeholder: 1.5 sqrt(1.0) = 1.5 cm, 1.8 sqrt(1.4 / 1.2) = 1.9 deg
    {"AS Val", "weapons/asval/asval", nullptr, recoil::Kind::kSmg, 0, 900.0f, 333.0f, 21, 1,
     {0.4f, -0.3f, 0.2f, 18.0f, 0.5f, 0.1f, 250.0f, 0.35f},
     {{0.4f, 0.4f}, {0.4f, 0.4f}, {0.4f, 0.4f}}, {{-0.3f, -0.3f}, {-0.3f, -0.3f}, {-0.3f, -0.3f}},
     {{0.2f, 0.2f}, {0.2f, 0.2f}, {0.2f, 0.2f}}, {{18.0f, 18.0f}, {18.0f, 18.0f}, {18.0f, 18.0f}},
     10.0f, 10.0f, {1.0f, 0.4f, 0.1f},
     {.back = {0.015f, 0.03f, 0.75f}, .rise = {1.9f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.67f, 0.5f, 0.67f, 0.5f}},
    // AKS-74u: recorded: 1.91 cm; 2.17 deg (at 78 ms) drawn with the drift's 0.11 where they peak, 2.06; held fire 2.45 cm over 1.91, 1.28
    {"AKS-74u", "weapons/aks74u/aks74u", "aks74u", recoil::Kind::kRifle, 17991, 650.0f, 440.0f, 31, 1,
     {0.28f, -0.3f, 0.2f, 18.0f, 1.5f, 0.1f, 250.0f, 0.35f},
     {{0.28f, 0.28f}, {0.28f, 0.28f}, {0.28f, 0.28f}}, {{-0.3f, -0.3f}, {-0.3f, -0.3f}, {-0.3f, -0.3f}},
     {{0.2f, 0.2f}, {0.2f, 0.2f}, {0.2f, 0.2f}}, {{18.0f, 18.0f}, {18.0f, 18.0f}, {18.0f, 18.0f}},
     10.0f, 10.0f, {0.4f, 0.6f, 0.15f},
     {.back = {0.0191f, 0.03f, 0.75f}, .rise = {2.06f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 1.28f},
     0.0f, {1.0f, 1.0f, 0.75f, 0.67f, 0.5f, 0.34f, 0.67f, 0.34f}},
    // M4A1: recorded: 1.72 cm; 2.08 deg (at 63 ms) drawn with the drift's 0.22 where they peak, 1.88; held fire 2.79 cm over 1.72, 1.62
    {"M4A1", "weapons/m4a1/m4a1", "m4a1", recoil::Kind::kRifle, 22194, 800.0f, 580.0f, 31, 1,
     {0.26f, -0.4f, 0.1f, 18.0f, 2.8f, 0.1f, 250.0f, 0.35f},
     {{0.26f, 0.26f}, {0.26f, 0.26f}, {0.26f, 0.26f}}, {{-0.4f, -0.4f}, {-0.4f, -0.4f}, {-0.4f, -0.4f}},
     {{0.1f, 0.1f}, {0.1f, 0.1f}, {0.1f, 0.1f}}, {{18.0f, 18.0f}, {18.0f, 18.0f}, {18.0f, 18.0f}},
     10.0f, 10.0f, {0.2f, 1.0f, 0.12f},
     {.back = {0.0172f, 0.03f, 0.75f}, .rise = {1.88f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 1.62f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // SCAR-H: physics, a placeholder: 1.5 sqrt(2.1) = 2.2 cm, 1.8 sqrt(2.5 / 1.2) = 2.6 deg
    {"SCAR-H", "weapons/scar-h/scar-h", nullptr, recoil::Kind::kRifle, 0, 600.0f, 420.0f, 21, 1,
     {0.5f, -0.2f, 0.2f, 13.0f, 1.35f, 0.1f, 250.0f, 0.35f},
     {{0.5f, 0.5f}, {0.5f, 0.5f}, {0.5f, 0.5f}}, {{-0.2f, -0.2f}, {-0.2f, -0.2f}, {-0.2f, -0.2f}},
     {{0.2f, 0.2f}, {0.2f, 0.2f}, {0.2f, 0.2f}}, {{13.0f, 13.0f}, {13.0f, 13.0f}, {13.0f, 13.0f}},
     10.0f, 10.0f, {0.2f, 0.8f, 0.2f},
     {.back = {0.022f, 0.03f, 0.75f}, .rise = {2.6f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // G36C: physics, a placeholder: 1.5 sqrt(1.1) = 1.6 cm, 1.8 sqrt(1.2 / 1.2) = 1.8 deg
    {"G36C", "weapons/g36c/g36c", nullptr, recoil::Kind::kRifle, 0, 750.0f, 500.0f, 31, 1,
     {0.28f, -0.3f, 0.3f, 18.0f, 1.8f, 0.1f, 250.0f, 0.35f},
     {{0.28f, 0.28f}, {0.28f, 0.28f}, {0.28f, 0.28f}}, {{-0.3f, -0.3f}, {-0.3f, -0.3f}, {-0.3f, -0.3f}},
     {{0.3f, 0.3f}, {0.3f, 0.3f}, {0.3f, 0.3f}}, {{18.0f, 18.0f}, {18.0f, 18.0f}, {18.0f, 18.0f}},
     10.0f, 10.0f, {0.2f, 1.0f, 0.12f},
     {.back = {0.016f, 0.03f, 0.75f}, .rise = {1.8f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // SG 553: physics, a placeholder: 1.5 sqrt(1.1) = 1.6 cm, 1.8 sqrt(1.2 / 1.2) = 1.8 deg
    {"SG 553", "weapons/sg553lb/sg553lb", nullptr, recoil::Kind::kRifle, 0, 700.0f, 430.0f, 31, 1,
     {0.25f, -0.4f, 0.2f, 18.0f, 2.2f, 0.5f, 250.0f, 0.35f},
     {{0.25f, 0.25f}, {0.25f, 0.25f}, {0.25f, 0.25f}}, {{-0.4f, -0.4f}, {-0.4f, -0.4f}, {-0.4f, -0.4f}},
     {{0.2f, 0.2f}, {0.2f, 0.2f}, {0.2f, 0.2f}}, {{18.0f, 18.0f}, {18.0f, 18.0f}, {18.0f, 18.0f}},
     10.0f, 10.0f, {0.2f, 1.0f, 0.12f},
     {.back = {0.016f, 0.03f, 0.75f}, .rise = {1.8f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f},
     0.0f, {1.0f, 1.0f, 0.75f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // M416: the anchor: 1.5 cm, 1.8 deg
    {"M416", "weapons/m416/m416", nullptr, recoil::Kind::kRifle, 12768, 750.0f, 600.0f, 31, 1,
     {0.26f, -0.3f, 0.1f, 18.0f, 2.0f, 0.1f, 250.0f, 0.35f},
     {{0.26f, 0.26f}, {0.26f, 0.26f}, {0.26f, 0.26f}}, {{-0.3f, -0.3f}, {-0.3f, -0.3f}, {-0.3f, -0.3f}},
     {{0.1f, 0.1f}, {0.1f, 0.1f}, {0.1f, 0.1f}}, {{18.0f, 18.0f}, {18.0f, 18.0f}, {18.0f, 18.0f}},
     10.0f, 10.0f, {0.2f, 1.0f, 0.12f},
     {.back = {0.015f, 0.03f, 0.75f}, .rise = {1.8f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // AK-74M: recorded: 1.72 cm; 2.06 deg (at 62 ms) drawn with the drift's 0.11 where they peak, 1.95; held fire 2.77 cm over 1.72, 1.61
    {"AK-74M", "weapons/ak74m/ak74", "ak74", recoil::Kind::kRifle, 21723, 650.0f, 600.0f, 31, 1,
     {0.28f, -0.3f, 0.2f, 18.0f, 1.5f, 0.1f, 250.0f, 0.35f},
     {{0.28f, 0.28f}, {0.28f, 0.28f}, {0.28f, 0.28f}}, {{-0.3f, -0.3f}, {-0.3f, -0.3f}, {-0.3f, -0.3f}},
     {{0.2f, 0.2f}, {0.2f, 0.2f}, {0.2f, 0.2f}}, {{18.0f, 18.0f}, {18.0f, 18.0f}, {18.0f, 18.0f}},
     10.0f, 10.0f, {0.4f, 0.6f, 0.15f},
     {.back = {0.0172f, 0.03f, 0.75f}, .rise = {1.95f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 1.61f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.5f, 0.5f, 0.75f, 0.5f}},
    // AN-94: a recorded pair 47 ms apart: 1.59 cm with held fire's push back in it, 0.8 a round; 3.11 deg with the drift's 0.39 at the peak, 1.43 a round
    {"AN-94", "weapons/an94/an94", "an94", recoil::Kind::kRifle, 16188, 600.0f, 600.0f, 31, 1,
     {0.3f, -0.3f, 0.2f, 18.0f, 1.5f, 0.1f, 250.0f, 0.35f},
     {{0.3f, 0.3f}, {0.3f, 0.3f}, {0.3f, 0.3f}}, {{-0.3f, -0.3f}, {-0.3f, -0.3f}, {-0.3f, -0.3f}},
     {{0.2f, 0.2f}, {0.2f, 0.2f}, {0.2f, 0.2f}}, {{18.0f, 18.0f}, {18.0f, 18.0f}, {18.0f, 18.0f}},
     10.0f, 10.0f, {0.5f, 0.8f, 0.15f},
     {.back = {0.008f, 0.03f, 0.75f}, .rise = {1.43f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 0.3f},
     0.075f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // AEK-971: recorded: 1.76 cm; 2.09 deg (at 47 ms) drawn with the drift's 0.17 where they peak, 1.93; held fire 2.82 cm over 1.76, 1.6
    {"AEK-971", "weapons/aek971/aek971", "aek971", recoil::Kind::kRifle, 21585, 900.0f, 580.0f, 31, 1,
     {0.2f, -0.3f, 0.5f, 18.0f, 3.0f, 0.1f, 250.0f, 0.35f},
     {{0.2f, 0.2f}, {0.2f, 0.2f}, {0.2f, 0.2f}}, {{-0.3f, -0.3f}, {-0.3f, -0.3f}, {-0.3f, -0.3f}},
     {{0.5f, 0.5f}, {0.5f, 0.5f}, {0.5f, 0.5f}}, {{18.0f, 18.0f}, {18.0f, 18.0f}, {18.0f, 18.0f}},
     10.0f, 10.0f, {0.2f, 1.0f, 0.07f},
     {.back = {0.0176f, 0.03f, 0.75f}, .rise = {1.93f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 1.6f},
     0.0f, {0.75f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // KH2002: recorded: 1.7 cm; 2.04 deg (at 63 ms) drawn with the drift's 0.08 where they peak, 1.97
    {"KH2002", "weapons/kh2002/kh2002", "kh2002", recoil::Kind::kRifle, 15237, 800.0f, 650.0f, 31, 1,
     {0.2f, -0.4f, 0.4f, 18.0f, 1.5f, 0.1f, 250.0f, 0.35f},
     {{0.2f, 0.2f}, {0.2f, 0.2f}, {0.2f, 0.2f}}, {{-0.4f, -0.4f}, {-0.4f, -0.4f}, {-0.4f, -0.4f}},
     {{0.4f, 0.4f}, {0.4f, 0.4f}, {0.4f, 0.4f}}, {{18.0f, 18.0f}, {18.0f, 18.0f}, {18.0f, 18.0f}},
     10.0f, 10.0f, {0.4f, 0.65f, 0.15f},
     {.back = {0.017f, 0.03f, 0.75f}, .rise = {1.97f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // G3A3: recorded: 1.69 cm; 2.35 deg (at 94 ms) drawn with the drift's 0.18 where they peak, 2.19; held fire 2.63 cm over 1.69, 1.56
    {"G3A3", "weapons/g3a3/g3a3", "g3a3", recoil::Kind::kRifle, 23064, 550.0f, 500.0f, 21, 1,
     {0.45f, -0.2f, 0.2f, 15.0f, 1.4f, 0.1f, 250.0f, 0.35f},
     {{0.45f, 0.45f}, {0.45f, 0.45f}, {0.45f, 0.45f}}, {{-0.2f, -0.2f}, {-0.2f, -0.2f}, {-0.2f, -0.2f}},
     {{0.2f, 0.2f}, {0.2f, 0.2f}, {0.2f, 0.2f}}, {{15.0f, 15.0f}, {15.0f, 15.0f}, {15.0f, 15.0f}},
     10.0f, 10.0f, {0.0f, 1.0f, 0.2f},
     {.back = {0.0169f, 0.03f, 0.75f}, .rise = {2.19f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 1.56f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // F2000: physics, a placeholder: 1.5 sqrt(1.2) = 1.6 cm, 1.8 sqrt(1.4 / 1.2) = 1.9 deg
    {"F2000", "weapons/f2000/f2000", nullptr, recoil::Kind::kRifle, 0, 850.0f, 600.0f, 31, 1,
     {0.26f, -0.4f, 0.5f, 18.0f, 3.0f, 0.1f, 250.0f, 0.35f},
     {{0.26f, 0.26f}, {0.26f, 0.26f}, {0.26f, 0.26f}}, {{-0.4f, -0.4f}, {-0.4f, -0.4f}, {-0.4f, -0.4f}},
     {{0.5f, 0.5f}, {0.5f, 0.5f}, {0.5f, 0.5f}}, {{18.0f, 18.0f}, {18.0f, 18.0f}, {18.0f, 18.0f}},
     10.0f, 10.0f, {1.0f, 0.4f, 0.12f},
     {.back = {0.016f, 0.03f, 0.75f}, .rise = {1.9f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // M16A4: given, a placeholder: 1.73 cm, 1.88 deg; held fire 2.97 cm over 1.73, 1.72
    {"M16A4", "weapons/m16a4/m16a4", "m16a4", recoil::Kind::kRifle, 17013, 800.0f, 650.0f, 31, 1,
     {0.26f, -0.4f, 0.1f, 18.0f, 2.5f, 0.1f, 250.0f, 0.35f},
     {{0.26f, 0.26f}, {0.26f, 0.26f}, {0.26f, 0.26f}}, {{-0.4f, -0.4f}, {-0.4f, -0.4f}, {-0.4f, -0.4f}},
     {{0.1f, 0.1f}, {0.1f, 0.1f}, {0.1f, 0.1f}}, {{18.0f, 18.0f}, {18.0f, 18.0f}, {18.0f, 18.0f}},
     10.0f, 10.0f, {0.2f, 1.0f, 0.12f},
     {.back = {0.0173f, 0.03f, 0.75f}, .rise = {1.88f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 1.72f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // A-91: given, a placeholder: 1.5 cm, 1.8 deg
    {"A-91", "weapons/a91/a91", nullptr, recoil::Kind::kRifle, 0, 800.0f, 420.0f, 31, 1,
     {0.2f, -0.5f, 0.5f, 18.0f, 3.0f, 0.1f, 250.0f, 0.35f},
     {{0.2f, 0.2f}, {0.2f, 0.2f}, {0.2f, 0.2f}}, {{-0.5f, -0.5f}, {-0.5f, -0.5f}, {-0.5f, -0.5f}},
     {{0.5f, 0.5f}, {0.5f, 0.5f}, {0.5f, 0.5f}}, {{18.0f, 18.0f}, {18.0f, 18.0f}, {18.0f, 18.0f}},
     10.0f, 10.0f, {0.2f, 1.0f, 0.12f},
     {.back = {0.015f, 0.03f, 0.75f}, .rise = {1.8f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // RPK-74M: recorded: 1.7 cm; 2.07 deg (at 31 ms) drawn with the drift's 0.15 where they peak, 1.93; held fire 2.81 cm over 1.7, 1.65
    {"RPK-74M", "weapons/rpk/rpk", "rpk", recoil::Kind::kMg, 24492, 700.0f, 600.0f, 46, 1,
     {0.3f, -0.2f, 0.2f, 13.0f, 1.8f, 0.1f, 250.0f, 0.35f},
     {{0.3f, 0.3f}, {0.3f, 0.3f}, {0.3f, 0.3f}}, {{-0.2f, -0.2f}, {-0.2f, -0.2f}, {-0.2f, -0.2f}},
     {{0.2f, 0.2f}, {0.2f, 0.2f}, {0.2f, 0.2f}}, {{13.0f, 13.0f}, {13.0f, 13.0f}, {13.0f, 13.0f}},
     10.0f, 10.0f, {0.3f, 0.75f, 0.15f},
     {.back = {0.017f, 0.03f, 0.75f}, .rise = {1.93f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 1.65f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // M240B: recorded: 1.53 cm; 2.57 deg (at 63 ms) drawn with the drift's 0.41 where they peak, 2.23; held fire 2.4 cm over 1.53, 1.57
    {"M240B", "weapons/m240/m240", "m240", recoil::Kind::kMg, 27588, 650.0f, 610.0f, 100, 1,
     {0.7f, -0.5f, 0.5f, 12.0f, 1.7f, 0.1f, 250.0f, 0.35f},
     {{0.7f, 0.7f}, {0.7f, 0.7f}, {0.7f, 0.7f}}, {{-0.5f, -0.5f}, {-0.5f, -0.5f}, {-0.5f, -0.5f}},
     {{0.5f, 0.5f}, {0.5f, 0.5f}, {0.5f, 0.5f}}, {{12.0f, 12.0f}, {12.0f, 12.0f}, {12.0f, 12.0f}},
     10.0f, 10.0f, {1.0f, 1.0f, 0.2f},
     {.back = {0.0153f, 0.03f, 0.75f}, .rise = {2.23f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 1.57f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // M249: recorded: 1.51 cm; 2.44 deg (at 63 ms) drawn with the drift's 0.27 where they peak, 2.2; held fire 1.57 cm over 1.51, 1.04
    {"M249", "weapons/m249/m249", "m249", recoil::Kind::kMg, 24291, 800.0f, 620.0f, 100, 1,
     {0.4f, -0.4f, 0.4f, 12.0f, 2.2f, 0.1f, 250.0f, 0.35f},
     {{0.4f, 0.4f}, {0.4f, 0.4f}, {0.4f, 0.4f}}, {{-0.4f, -0.4f}, {-0.4f, -0.4f}, {-0.4f, -0.4f}},
     {{0.4f, 0.4f}, {0.4f, 0.4f}, {0.4f, 0.4f}}, {{12.0f, 12.0f}, {12.0f, 12.0f}, {12.0f, 12.0f}},
     10.0f, 10.0f, {0.4f, 0.7f, 0.15f},
     {.back = {0.0151f, 0.03f, 0.75f}, .rise = {2.2f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 1.04f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.5f, 0.5f, 0.75f, 0.5f}},
    // PKP: physics, a placeholder: 1.5 sqrt(1.3) = 1.7 cm, 1.8 sqrt(1.7 / 1.2) = 2.1 deg
    {"PKP", "weapons/pecheneg/pecheneg", nullptr, recoil::Kind::kMg, 0, 600.0f, 560.0f, 100, 1,
     {0.65f, -0.4f, 0.4f, 13.0f, 1.5f, 0.1f, 250.0f, 0.35f},
     {{0.65f, 0.65f}, {0.65f, 0.65f}, {0.65f, 0.65f}}, {{-0.4f, -0.4f}, {-0.4f, -0.4f}, {-0.4f, -0.4f}},
     {{0.4f, 0.4f}, {0.4f, 0.4f}, {0.4f, 0.4f}}, {{13.0f, 13.0f}, {13.0f, 13.0f}, {13.0f, 13.0f}},
     10.0f, 10.0f, {1.0f, 1.0f, 0.112f},
     {.back = {0.017f, 0.03f, 0.75f}, .rise = {2.1f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f},
     0.0f, {1.0f, 1.0f, 0.75f, 0.67f, 0.5f, 0.5f, 0.67f, 1.0f}},
    // Type 88: physics, a placeholder: 1.5 sqrt(0.8) = 1.3 cm, 1.8 sqrt(1.1 / 1.2) = 1.7 deg
    {"Type 88", "weapons/type88/type88", nullptr, recoil::Kind::kMg, 0, 650.0f, 600.0f, 100, 1,
     {0.36f, -0.3f, 0.3f, 12.0f, 1.5f, 0.1f, 250.0f, 0.35f},
     {{0.36f, 0.36f}, {0.36f, 0.36f}, {0.36f, 0.36f}}, {{-0.3f, -0.3f}, {-0.3f, -0.3f}, {-0.3f, -0.3f}},
     {{0.3f, 0.3f}, {0.3f, 0.3f}, {0.3f, 0.3f}}, {{12.0f, 12.0f}, {12.0f, 12.0f}, {12.0f, 12.0f}},
     10.0f, 10.0f, {0.4f, 0.7f, 0.15f},
     {.back = {0.013f, 0.03f, 0.75f}, .rise = {1.7f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // M60E4: physics, a placeholder: 1.5 sqrt(1.3) = 1.7 cm, 1.8 sqrt(1.6 / 1.2) = 2.1 deg
    {"M60E4", "weapons/m60/m60", nullptr, recoil::Kind::kMg, 0, 580.0f, 560.0f, 100, 1,
     {0.6f, -0.2f, 0.2f, 12.0f, 1.5f, 0.1f, 250.0f, 0.35f},
     {{0.6f, 0.6f}, {0.6f, 0.6f}, {0.6f, 0.6f}}, {{-0.2f, -0.2f}, {-0.2f, -0.2f}, {-0.2f, -0.2f}},
     {{0.2f, 0.2f}, {0.2f, 0.2f}, {0.2f, 0.2f}}, {{12.0f, 12.0f}, {12.0f, 12.0f}, {12.0f, 12.0f}},
     10.0f, 10.0f, {1.0f, 1.0f, 0.25f},
     {.back = {0.017f, 0.03f, 0.75f}, .rise = {2.1f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // M27 IAR: physics, a placeholder: 1.5 sqrt(1.2) = 1.6 cm, 1.8 sqrt(1.4 / 1.2) = 1.9 deg
    {"M27 IAR", "weapons/m27iar/m27iar", nullptr, recoil::Kind::kMg, 0, 750.0f, 650.0f, 46, 1,
     {0.35f, -0.3f, 0.1f, 13.0f, 2.2f, 0.1f, 250.0f, 0.35f},
     {{0.35f, 0.35f}, {0.35f, 0.35f}, {0.35f, 0.35f}}, {{-0.3f, -0.3f}, {-0.3f, -0.3f}, {-0.3f, -0.3f}},
     {{0.1f, 0.1f}, {0.1f, 0.1f}, {0.1f, 0.1f}}, {{13.0f, 13.0f}, {13.0f, 13.0f}, {13.0f, 13.0f}},
     10.0f, 10.0f, {0.4f, 1.0f, 0.12f},
     {.back = {0.016f, 0.03f, 0.75f}, .rise = {1.9f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // 870 MCS: physics, a placeholder: 1.5 sqrt(3.1) = 2.6 cm, 1.8 sqrt(4.5 / 1.2) = 3.5 deg
    {"870 MCS", "weapons/remington870/remington870mcs", nullptr, recoil::Kind::kShotgun, 0, 600.0f, 300.0f, 5, 12,
     {1.0f, -0.3f, -0.1f, 10.0f, 1.0f, 1.0f, 250.0f, 0.35f},
     {{1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}}, {{-0.3f, -0.3f}, {-0.3f, -0.3f}, {-0.3f, -0.3f}},
     {{-0.1f, -0.1f}, {-0.1f, -0.1f}, {-0.1f, -0.1f}}, {{10.0f, 10.0f}, {10.0f, 10.0f}, {10.0f, 10.0f}},
     10.0f, 10.0f, {1.0f, 1.0f, 0.4f},
     {.back = {0.026f, 0.035f, 0.75f}, .rise = {3.5f, 0.06f, 0.7f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // M1014: recorded: 0.2 cm; 7.15 deg (at 94 ms) drawn with the drift's 0.44 where they peak, 6.74
    {"M1014", "weapons/m1014/m1014", "m1014", recoil::Kind::kShotgun, 13377, 220.0f, 300.0f, 5, 10,
     {1.2f, -0.3f, -0.1f, 15.0f, 1.0f, 0.34f, 250.0f, 0.35f},
     {{1.2f, 1.2f}, {1.2f, 1.2f}, {1.2f, 1.2f}}, {{-0.3f, -0.3f}, {-0.3f, -0.3f}, {-0.3f, -0.3f}},
     {{-0.1f, -0.1f}, {-0.1f, -0.1f}, {-0.1f, -0.1f}}, {{15.0f, 15.0f}, {15.0f, 15.0f}, {15.0f, 15.0f}},
     10.0f, 10.0f, {0.4f, 1.0f, 0.2f},
     {.back = {0.002f, 0.035f, 0.75f}, .rise = {6.74f, 0.06f, 0.7f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // Saiga 12K: physics, a placeholder: 1.5 sqrt(2.5) = 2.4 cm, 1.8 sqrt(3.7 / 1.2) = 3.2 deg
    {"Saiga 12K", "weapons/saiga20k/saiga_20k", nullptr, recoil::Kind::kShotgun, 0, 220.0f, 300.0f, 7, 9,
     {0.8f, -0.2f, 0.2f, 15.0f, 1.0f, 0.34f, 250.0f, 0.35f},
     {{0.8f, 0.8f}, {0.8f, 0.8f}, {0.8f, 0.8f}}, {{-0.2f, -0.2f}, {-0.2f, -0.2f}, {-0.2f, -0.2f}},
     {{0.2f, 0.2f}, {0.2f, 0.2f}, {0.2f, 0.2f}}, {{15.0f, 15.0f}, {15.0f, 15.0f}, {15.0f, 15.0f}},
     10.0f, 10.0f, {0.34f, 1.0f, 0.2f},
     {.back = {0.024f, 0.035f, 0.75f}, .rise = {3.2f, 0.06f, 0.7f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // USAS-12: physics, a placeholder: 1.5 sqrt(1.9) = 2.1 cm, 1.8 sqrt(2.3 / 1.2) = 2.5 deg
    {"USAS-12", "weapons/usas-12/usas-12", nullptr, recoil::Kind::kShotgun, 0, 275.0f, 300.0f, 7, 7,
     {1.5f, -0.4f, 0.4f, 6.0f, 2.0f, 1.0f, 250.0f, 0.35f},
     {{1.5f, 1.5f}, {1.5f, 1.5f}, {1.5f, 1.5f}}, {{-0.4f, -0.4f}, {-0.4f, -0.4f}, {-0.4f, -0.4f}},
     {{0.4f, 0.4f}, {0.4f, 0.4f}, {0.4f, 0.4f}}, {{6.0f, 6.0f}, {6.0f, 6.0f}, {6.0f, 6.0f}},
     10.0f, 10.0f, {1.0f, 1.0f, 0.2f},
     {.back = {0.021f, 0.035f, 0.75f}, .rise = {2.5f, 0.06f, 0.7f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // DAO-12: physics, a placeholder: 1.5 sqrt(2.7) = 2.5 cm, 1.8 sqrt(3.5 / 1.2) = 3.1 deg
    {"DAO-12", "weapons/dao-12/dao-12", nullptr, recoil::Kind::kShotgun, 0, 220.0f, 300.0f, 8, 9,
     {1.2f, -0.3f, -0.1f, 15.0f, 1.0f, 0.34f, 250.0f, 0.35f},
     {{1.2f, 1.2f}, {1.2f, 1.2f}, {1.2f, 1.2f}}, {{-0.3f, -0.3f}, {-0.3f, -0.3f}, {-0.3f, -0.3f}},
     {{-0.1f, -0.1f}, {-0.1f, -0.1f}, {-0.1f, -0.1f}}, {{15.0f, 15.0f}, {15.0f, 15.0f}, {15.0f, 15.0f}},
     10.0f, 10.0f, {0.34f, 1.0f, 0.2f},
     {.back = {0.025f, 0.035f, 0.75f}, .rise = {3.1f, 0.06f, 0.7f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // Mk11 Mod 0: recorded: 3.43 cm; 2.19 deg (at 141 ms) drawn with the drift's 1.36 where they peak, 1.37
    {"Mk11 Mod 0", "weapons/mk11/mk11", "mk11", recoil::Kind::kMarksman, 12456, 260.0f, 550.0f, 11, 1,
     {1.5f, -0.3f, -0.1f, 6.0f, 1.0f, 0.34f, 250.0f, 0.35f},
     {{1.5f, 1.5f}, {1.5f, 1.5f}, {1.5f, 1.5f}}, {{-0.3f, -0.3f}, {-0.3f, -0.3f}, {-0.3f, -0.3f}},
     {{-0.1f, -0.1f}, {-0.1f, -0.1f}, {-0.1f, -0.1f}}, {{6.0f, 6.0f}, {6.0f, 6.0f}, {6.0f, 6.0f}},
     10.0f, 10.0f, {0.8f, 0.6f, 0.25f},
     {.back = {0.0343f, 0.035f, 0.75f}, .rise = {1.37f, 0.06f, 0.7f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
    // M39 EMR: physics, a placeholder: 1.5 sqrt(1.9) = 2.1 cm, 1.8 sqrt(2.8 / 1.2) = 2.7 deg
    {"M39 EMR", "weapons/m39ebr/m39ebr", nullptr, recoil::Kind::kMarksman, 0, 300.0f, 570.0f, 11, 1,
     {1.2f, -0.2f, 0.2f, 6.0f, 1.0f, 0.34f, 250.0f, 0.35f},
     {{1.2f, 1.2f}, {1.2f, 1.2f}, {1.2f, 1.2f}}, {{-0.2f, -0.2f}, {-0.2f, -0.2f}, {-0.2f, -0.2f}},
     {{0.2f, 0.2f}, {0.2f, 0.2f}, {0.2f, 0.2f}}, {{6.0f, 6.0f}, {6.0f, 6.0f}, {6.0f, 6.0f}},
     10.0f, 10.0f, {0.8f, 1.0f, 0.25f},
     {.back = {0.021f, 0.035f, 0.75f}, .rise = {2.7f, 0.06f, 0.7f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // SVD: physics, a placeholder: 1.5 sqrt(2.2) = 2.2 cm, 1.8 sqrt(3.4 / 1.2) = 3.0 deg
    {"SVD", "weapons/svd/svd", nullptr, recoil::Kind::kMarksman, 0, 260.0f, 530.0f, 11, 1,
     {1.5f, -0.3f, -0.1f, 6.0f, 1.0f, 0.34f, 250.0f, 0.35f},
     {{1.5f, 1.5f}, {1.5f, 1.5f}, {1.5f, 1.5f}}, {{-0.3f, -0.3f}, {-0.3f, -0.3f}, {-0.3f, -0.3f}},
     {{-0.1f, -0.1f}, {-0.1f, -0.1f}, {-0.1f, -0.1f}}, {{6.0f, 6.0f}, {6.0f, 6.0f}, {6.0f, 6.0f}},
     10.0f, 10.0f, {0.8f, 0.6f, 0.25f},
     {.back = {0.022f, 0.035f, 0.75f}, .rise = {3.0f, 0.06f, 0.7f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f}},
    // M82A3: physics, a placeholder: 1.5 sqrt(2.0) = 2.1 cm, 1.8 sqrt(2.7 / 1.2) = 2.7 deg
    {"M82A3", "weapons/m82a3/m82a3", nullptr, recoil::Kind::kMarksman, 0, 175.0f, 560.0f, 11, 1,
     {2.0f, -0.3f, -0.1f, 12.0f, 1.0f, 0.1f, 250.0f, 0.34f},
     {{2.0f, 2.0f}, {2.0f, 2.0f}, {2.0f, 2.0f}}, {{-0.3f, -0.3f}, {-0.3f, -0.3f}, {-0.3f, -0.3f}},
     {{-0.1f, -0.1f}, {-0.1f, -0.1f}, {-0.1f, -0.1f}}, {{12.0f, 12.0f}, {12.0f, 12.0f}, {12.0f, 12.0f}},
     10.0f, 10.0f, {1.0f, 1.0f, 0.35f},
     {.back = {0.021f, 0.035f, 0.75f}, .rise = {2.7f, 0.06f, 0.7f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.75f}},
    // M40A5: physics, a placeholder: 1.5 sqrt(1.4) = 1.8 cm, 1.8 sqrt(1.9 / 1.2) = 2.3 deg
    {"M40A5", "weapons/m40a5/m40a5", nullptr, recoil::Kind::kBolt, 0, 360.0f, 490.0f, 11, 1,
     {2.0f, 0.0f, 0.0f, 15.0f, 1.0f, 1.0f, 250.0f, 0.35f},
     {{2.0f, 0.5f}, {2.0f, 0.5f}, {2.0f, 0.5f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
     {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}}, {{15.0f, 6.0f}, {15.0f, 6.0f}, {15.0f, 6.0f}},
     10.0f, 10.0f, {0.8f, 0.5f, 0.25f},
     {.back = {0.018f, 0.035f, 0.75f}, .rise = {2.3f, 0.06f, 0.7f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
    // M98B: physics, a placeholder: 1.5 sqrt(2.0) = 2.1 cm, 1.8 sqrt(2.5 / 1.2) = 2.6 deg
    {"M98B", "weapons/model98b/model_98b", nullptr, recoil::Kind::kBolt, 0, 360.0f, 650.0f, 6, 1,
     {2.0f, 0.0f, 0.0f, 15.0f, 1.0f, 1.0f, 250.0f, 0.35f},
     {{2.0f, 0.5f}, {2.0f, 0.5f}, {2.0f, 0.5f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
     {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}}, {{15.0f, 6.0f}, {15.0f, 6.0f}, {15.0f, 6.0f}},
     10.0f, 10.0f, {1.0f, 1.0f, 0.25f},
     {.back = {0.021f, 0.035f, 0.75f}, .rise = {2.6f, 0.06f, 0.7f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // SV-98: physics, a placeholder: 1.5 sqrt(1.9) = 2.1 cm, 1.8 sqrt(2.5 / 1.2) = 2.6 deg
    {"SV-98", "weapons/sv98/sv98", nullptr, recoil::Kind::kBolt, 0, 360.0f, 520.0f, 11, 1,
     {2.0f, 0.0f, 0.0f, 15.0f, 1.0f, 1.0f, 250.0f, 0.35f},
     {{2.0f, 0.5f}, {2.0f, 0.5f}, {2.0f, 0.5f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
     {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}}, {{15.0f, 6.0f}, {15.0f, 6.0f}, {15.0f, 6.0f}},
     10.0f, 10.0f, {1.0f, 0.5f, 0.24f},
     {.back = {0.021f, 0.035f, 0.75f}, .rise = {2.6f, 0.06f, 0.7f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 0.75f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // M320: recorded: 2.7 cm; 5.8 deg (at 47 ms) drawn with the drift's 1.39 where they peak, 4.75
    {"M320", "weapons/gadgets/m320/m320", nullptr, recoil::Kind::kM320, 0, 250.0f, 45.0f, 1, 1,
     {2.5f, -1.0f, -0.5f, 15.0f, 1.0f, 1.0f, 250.0f, 0.35f},
     {{2.5f, 2.5f}, {2.5f, 2.5f}, {3.0f, 2.5f}}, {{-1.0f, -1.0f}, {-1.0f, -1.0f}, {-0.4f, -1.0f}},
     {{-0.5f, -0.5f}, {-0.5f, -0.5f}, {-0.2f, -0.5f}}, {{15.0f, 15.0f}, {15.0f, 15.0f}, {15.0f, 15.0f}},
     10.0f, 10.0f, {1.0f, 1.0f, 0.2f},
     {.back = {0.027f, 0.06f, 0.9f}, .rise = {4.75f, 0.06f, 0.9f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
    // SMAW: given, a placeholder: 0.8 cm, 1.5 deg, lift 1.5 cm
    {"SMAW", "weapons/smaw/smaw", nullptr, recoil::Kind::kRocket, 3414, 350.0f, 40.0f, 1, 1,
     {2.5f, -1.0f, -0.5f, 15.0f, 1.0f, 1.0f, 250.0f, 0.35f},
     {{2.5f, 2.5f}, {2.5f, 2.5f}, {3.0f, 2.5f}}, {{-1.0f, -1.0f}, {-1.0f, -1.0f}, {-0.4f, -1.0f}},
     {{-0.5f, -0.5f}, {-0.5f, -0.5f}, {-0.2f, -0.5f}}, {{15.0f, 15.0f}, {15.0f, 15.0f}, {15.0f, 15.0f}},
     10.0f, 10.0f, {1.0f, 1.0f, 0.2f},
     {.back = {0.008f, 0.04f, 0.75f}, .rise = {1.5f, 0.08f, 0.7f}, .lift = {0.015f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
    // Stinger: given, a placeholder: 0.6 cm, 2.0 deg, lift 3.0 cm
    {"Stinger", "weapons/fim92a/fim92a_sp_valley", "fim92", recoil::Kind::kMissile, 20298, 60.0f, 16.0f, 1, 1,
     {2.5f, -1.0f, -0.5f, 15.0f, 1.0f, 1.0f, 250.0f, 0.35f},
     {{2.5f, 2.5f}, {2.5f, 2.5f}, {3.0f, 2.5f}}, {{-1.0f, -1.0f}, {-1.0f, -1.0f}, {-0.4f, -1.0f}},
     {{-0.5f, -0.5f}, {-0.5f, -0.5f}, {-0.2f, -0.5f}}, {{15.0f, 15.0f}, {15.0f, 15.0f}, {15.0f, 15.0f}},
     10.0f, 10.0f, {1.0f, 1.0f, 0.2f},
     {.back = {0.006f, 0.04f, 0.75f}, .rise = {2.0f, 0.08f, 0.7f}, .lift = {0.03f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
    // Javelin: given, a placeholder: 0.6 cm, 1.5 deg, lift 3.0 cm
    {"Javelin", "weapons/fgm148/fgm148_sp", nullptr, recoil::Kind::kMissile, 0, 60.0f, 16.0f, 1, 1,
     {2.5f, -1.0f, -0.5f, 15.0f, 1.0f, 1.0f, 250.0f, 0.35f},
     {{2.5f, 2.5f}, {2.5f, 2.5f}, {3.0f, 2.5f}}, {{-1.0f, -1.0f}, {-1.0f, -1.0f}, {-0.4f, -1.0f}},
     {{-0.5f, -0.5f}, {-0.5f, -0.5f}, {-0.2f, -0.5f}}, {{15.0f, 15.0f}, {15.0f, 15.0f}, {15.0f, 15.0f}},
     10.0f, 10.0f, {1.0f, 1.0f, 0.2f},
     {.back = {0.006f, 0.04f, 0.75f}, .rise = {1.5f, 0.08f, 0.7f}, .lift = {0.03f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
    // AT4: given, a placeholder: 1.0 cm, 1.0 deg, lift 1.0 cm
    {"AT4", "weapons/m136/m136", nullptr, recoil::Kind::kAt4, 0, 350.0f, 40.0f, 1, 1,
     {4.0f, -1.0f, 1.0f, 4.0f, 1.0f, 0.1f, 0.0f, 0.0f},
     {{4.0f, 1.0f}, {4.0f, 1.0f}, {2.0f, 2.0f}}, {{-1.0f, -1.0f}, {-1.0f, -1.0f}, {-1.0f, -1.0f}},
     {{1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}}, {{4.0f, 8.0f}, {4.0f, 8.0f}, {4.0f, 4.0f}},
     10.0f, 10.0f, {0.0f, 0.0f, 0.2f},
     {.back = {0.01f, 0.04f, 0.75f}, .rise = {1.0f, 0.08f, 0.7f}, .lift = {0.01f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
    // RPG-7: given, a placeholder: 0.8 cm, 1.5 deg, lift 1.5 cm
    {"RPG-7", "weapons/rpg7/rpg7_ai", nullptr, recoil::Kind::kRocket, 0, 18.0f, 40.0f, 1, 1,
     {2.5f, -1.0f, -0.5f, 15.0f, 1.0f, 1.0f, 250.0f, 0.35f},
     {{2.5f, 2.5f}, {2.5f, 2.5f}, {3.0f, 2.5f}}, {{-1.0f, -1.0f}, {-1.0f, -1.0f}, {-0.4f, -1.0f}},
     {{-0.5f, -0.5f}, {-0.5f, -0.5f}, {-0.2f, -0.5f}}, {{15.0f, 15.0f}, {15.0f, 15.0f}, {15.0f, 15.0f}},
     10.0f, 10.0f, {1.0f, 1.0f, 0.2f},
     {.back = {0.008f, 0.04f, 0.75f}, .rise = {1.5f, 0.08f, 0.7f}, .lift = {0.015f, 0.1f, 0.7f},
      .roll = 0.9f, .yaw = 0.25f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
    // Glock 18: given, a placeholder: 1.0 cm, 4.0 deg
    {"Glock 18", "weapons/glock18/glock18", nullptr, recoil::Kind::kPistol, 0, 900.0f, 375.0f, 20, 1,
     {0.7f, -0.4f, 0.4f, 30.0f, 2.4f, 1.0f, 150.0f, 0.35f},
     {{0.7f, 0.7f}, {0.7f, 0.7f}, {0.7f, 0.7f}}, {{-0.4f, -0.4f}, {-0.4f, -0.4f}, {-0.4f, -0.4f}},
     {{0.4f, 0.4f}, {0.4f, 0.4f}, {0.4f, 0.4f}}, {{30.0f, 30.0f}, {30.0f, 30.0f}, {30.0f, 30.0f}},
     10.0f, 10.0f, {0.39f, 0.6f, 0.1f},
     {.back = {0.01f, 0.025f, 0.8f}, .rise = {4.0f, 0.035f, 0.8f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.5f, .yaw = 0.1f, .jitter = 0.1f, .burstFirst = 1.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
    // .44 Magnum: given, a placeholder: 1.0 cm, 4.0 deg
    {".44 Magnum", "weapons/taurus44/taurus44", nullptr, recoil::Kind::kPistol, 0, 160.0f, 460.0f, 6, 1,
     {2.0f, -0.2f, 0.2f, 4.0f, 1.0f, 1.0f, 150.0f, 0.35f},
     {{2.0f, 2.0f}, {2.0f, 2.0f}, {2.0f, 2.0f}}, {{-0.2f, -0.2f}, {-0.2f, -0.2f}, {-0.2f, -0.2f}},
     {{0.2f, 0.2f}, {0.2f, 0.2f}, {0.2f, 0.2f}}, {{4.0f, 4.0f}, {4.0f, 4.0f}, {4.0f, 4.0f}},
     10.0f, 10.0f, {1.0f, 0.8f, 0.3f},
     {.back = {0.01f, 0.025f, 0.8f}, .rise = {4.0f, 0.035f, 0.8f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.5f, .yaw = 0.1f, .jitter = 0.1f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 1.0f, 0.75f, 0.5f, 0.75f, 0.5f}},
    // M224 mortar: no gun sway: no recoil
    {"M224 mortar", "weapons/gadgets/m224/m224_sponly", nullptr, recoil::Kind::kNone, 0, 30.0f, 0.0f, 1, 1,
     {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
     {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
     {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
     0.0f, 0.0f, {0.0f, 0.0f, 0.2f},
     {.back = {0.0f, 0.03f, 0.75f}, .rise = {0.0f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.0f, .yaw = 0.0f, .jitter = 0.0f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
    // M67: no gun sway: no recoil
    {"M67", "weapons/m67/m67", nullptr, recoil::Kind::kNone, 0, 300.0f, 18.0f, 1, 1,
     {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
     {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
     {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
     0.0f, 0.0f, {0.0f, 0.0f, 0.2f},
     {.back = {0.0f, 0.03f, 0.75f}, .rise = {0.0f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.0f, .yaw = 0.0f, .jitter = 0.0f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
    // M67 (computer-controlled): no gun sway: no recoil
    {"M67 (computer-controlled)", "weapons/m67/m67_ai", nullptr, recoil::Kind::kNone, 0, 300.0f, 18.0f, 10000, 1,
     {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
     {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
     {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
     0.0f, 0.0f, {0.0f, 0.0f, 0.2f},
     {.back = {0.0f, 0.03f, 0.75f}, .rise = {0.0f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.0f, .yaw = 0.0f, .jitter = 0.0f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
    // M18 Claymore: no gun sway: no recoil
    {"M18 Claymore", "weapons/gadgets/claymore/claymore", nullptr, recoil::Kind::kNone, 0, 300.0f, 1.0f, 1, 1,
     {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
     {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
     {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
     0.0f, 0.0f, {0.0f, 0.0f, 0.2f},
     {.back = {0.0f, 0.03f, 0.75f}, .rise = {0.0f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.0f, .yaw = 0.0f, .jitter = 0.0f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
    // Knife: no gun sway: no recoil
    {"Knife", "weapons/knife/knife", nullptr, recoil::Kind::kNone, 0, 175.0f, 1.8f, 1, 1,
     {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
     {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
     {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
     0.0f, 0.0f, {0.0f, 0.0f, 0.2f},
     {.back = {0.0f, 0.03f, 0.75f}, .rise = {0.0f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.0f, .yaw = 0.0f, .jitter = 0.0f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
    // Machete: no gun sway: no recoil
    {"Machete", "weapons/machete/machete", nullptr, recoil::Kind::kNone, 0, 175.0f, 1.8f, 1, 1,
     {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
     {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
     {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
     0.0f, 0.0f, {0.0f, 0.0f, 0.2f},
     {.back = {0.0f, 0.03f, 0.75f}, .rise = {0.0f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.0f, .yaw = 0.0f, .jitter = 0.0f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
    // Handcuffs: no gun sway: no recoil
    {"Handcuffs", "weapons/handcuffs/handcuff", nullptr, recoil::Kind::kNone, 0, 550.0f, 350.0f, 30, 1,
     {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
     {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
     {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
     0.0f, 0.0f, {0.0f, 0.0f, 0.2f},
     {.back = {0.0f, 0.03f, 0.75f}, .rise = {0.0f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.0f, .yaw = 0.0f, .jitter = 0.0f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
    // M26 MASS: no gun sway: no recoil
    {"M26 MASS", "weapons/gadgets/m26mass/m26mass", nullptr, recoil::Kind::kNone, 0, 0.0f, 0.0f, 0, 0,
     {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
     {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
     {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
     0.0f, 0.0f, {0.0f, 0.0f, 0.0f},
     {.back = {0.0f, 0.03f, 0.75f}, .rise = {0.0f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.0f, .yaw = 0.0f, .jitter = 0.0f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
    // Weapon dummy: no gun sway: no recoil
    {"Weapon dummy", "weapons/weapon_dummy/weapon_dummy", nullptr, recoil::Kind::kNone, 0, 550.0f, 350.0f, 30, 1,
     {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
     {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
     {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}},
     0.0f, 0.0f, {0.0f, 0.0f, 0.2f},
     {.back = {0.0f, 0.03f, 0.75f}, .rise = {0.0f, 0.05f, 0.75f}, .lift = {0.0f, 0.1f, 0.7f},
      .roll = 0.0f, .yaw = 0.0f, .jitter = 0.0f, .burstFirst = 1.0f, .heldShare = 0.0f},
     0.0f, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
};
inline constexpr int kRowCount = static_cast<int>(sizeof(kRows) / sizeof(kRows[0]));

// The next letter or digit of a name, in lower case (0 at its end).
inline char NameChar(const char*& p) {
    for (; *p; ++p) {
        const char c = *p;
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            ++p;
            return c;
        }
        if (c >= 'A' && c <= 'Z') {
            ++p;
            return static_cast<char>(c - 'A' + 'a');
        }
    }
    return 0;
}
// Two names alike in their letters and digits, whatever the case: "AS Val", "asval" and "as-val" are one.
inline bool SameName(const char* a, const char* b) {
    for (;;) {
        const char x = NameChar(a), y = NameChar(b);
        if (x != y)
            return false;
        if (!x)
            return true;
    }
}

// The row whose own numbers these are, as read from a gun's data (sig as Row::sig: a gun with no camera spring, the AT4, has 0 and 0 there), each
// within 1e-4; of two that share them the first (the Stinger before the Javelin). Rows with no gun sway are never found this way. nullptr if none.
inline const Row* BySignature(float rate, float speed, int magazine, int pellets, const float sig[8]) {
    if (!sig)
        return nullptr;
    auto within = [](float a, float b) { return a - b <= 1e-4f && b - a <= 1e-4f; };
    for (const Row& row : kRows) {
        if (row.kind == recoil::Kind::kNone || row.magazine != magazine || row.pellets != pellets || !within(row.rate, rate) ||
            !within(row.speed, speed))
            continue;
        bool same = true;
        for (int i = 0; same && i < 8; ++i)
            same = within(row.sig[i], sig[i]);
        if (same)
            return &row;
    }
    return nullptr;
}
// The row of the gun the mod draws with that body (its colour draw's index count; the train pistol's kTrainPistol), nullptr if none (0 is none).
inline const Row* ByBody(uint32_t body) {
    if (body)
        for (const Row& row : kRows)
            if (row.body == body)
                return &row;
    return nullptr;
}
// The row of a held gun by its shape (skin::HeldShape(): "an94", "fim92" ...), nullptr if none.
inline const Row* ByShape(const char* shape) {
    if (shape)
        for (const Row& row : kRows)
            if (row.shape && std::strcmp(row.shape, shape) == 0)
                return &row;
    return nullptr;
}
// The row by its name, its blueprint (whole, or its last part: "m240", "model_98b") or its shape, as SameName compares them (recoil.txt's
// "gun <name>"); no two rows share one. nullptr if none.
inline const Row* ByName(const char* name) {
    if (!name)
        return nullptr;
    const char* letters = name;
    if (!NameChar(letters))
        return nullptr;
    for (const Row& row : kRows) {
        const char* last = std::strrchr(row.blueprint, '/');
        if (SameName(row.name, name) || SameName(row.blueprint, name) || SameName(last ? last + 1 : row.blueprint, name) ||
            (row.shape && SameName(row.shape, name)))
            return &row;
    }
    return nullptr;
}
// A kind's middle row, for a gun known only by its kind: M9, UMP-45, M416, M240B, 870 MCS, Mk11 Mod 0, M98B, M320, SMAW, Stinger, AT4;
// kNone's (Knife) has no recoil.
inline const Row& Default(recoil::Kind kind) {
    switch (kind) {
    case recoil::Kind::kPistol:
        return kRows[0];  // M9
    case recoil::Kind::kSmg:
        return kRows[2];  // UMP-45
    case recoil::Kind::kRifle:
        return kRows[13];  // M416
    case recoil::Kind::kMg:
        return kRows[23];  // M240B
    case recoil::Kind::kShotgun:
        return kRows[29];  // 870 MCS
    case recoil::Kind::kMarksman:
        return kRows[34];  // Mk11 Mod 0
    case recoil::Kind::kBolt:
        return kRows[39];  // M98B
    case recoil::Kind::kM320:
        return kRows[41];  // M320
    case recoil::Kind::kRocket:
        return kRows[42];  // SMAW
    case recoil::Kind::kMissile:
        return kRows[43];  // Stinger
    case recoil::Kind::kAt4:
        return kRows[45];  // AT4
    default:
        return kRows[53];  // Knife
    }
}

}  // namespace recoilguns

// The M416's own ACOG as a sight of a held gun (skin.cpp's kHeldSights): the bridge M249's on Operation Swordbreaker's overpass (measured 2026-11-19 with
// nobody in the headset: build\survey\M249ACOG_*, docs/research/m249.md), its numbers in one place, which skin.cpp builds its row from and
// tools/m249acogtest.cpp holds to the dumps and the recordings (an edit here is an edit the test sees). The M416's own ACOG on the M416 (skin.cpp's DrawArms:
// kScopeBody, g_acogEyepiece, kSightsFall) is its own, as it was approved, untouched. Pure, no game.
//
// On the M249 the game draws the same six draws as on the M416 (kGunPartDraws: those keys took them as the rifle's parts, placed by the rifle's learned
// offsets, so they floated where the game holds its gun, in both eyes: the owner, 2026-11-19 01:25), each on a palette of its own, bone 0, their meshes
// the M416's ACOG's byte for byte; the glass, the chevron and the fibre share the body's vertices. Its bone 0 rides the feed cover on its mount
// (m249rows::kSightMount).

#pragma once

namespace acog {

// Its draws (index count, start): its body and depth pass, its glass (the eyepiece), its red chevron (drawn by the game unzoomed, at the sight, near; zoomed
// the game swaps in another model, scopemarks::kAcog's), its red fibre on top (two draws).
inline constexpr unsigned kBody[2] = {9273, 12}, kDepth[2] = {9273, 9594}, kGlass[2] = {72, 9285}, kChevron[2] = {12, 0};
inline constexpr unsigned kFibre[2][2] = {{129, 9357}, {108, 9486}};
// Its glass in its bone 0, the eyepiece: flat at z -0.0673, a disc of 24 rim vertices 10.9 to 11.0 mm round its middle vertex (0.0001 0.0317), the ACOG's
// own line (its chevron's tip at x 0.00001). (The M416's g_acogEyepiece, placed by eye: (0 0.032 -0.067), 11 mm.)
inline constexpr float kEyepiece[3] = {0.0f, 0.0317f, -0.0673f};
inline constexpr float kRadius = 0.011f;
// The game's own zoom (the owner's rule: whatever the vanilla game magnifies): from 55 to 17.2 degrees on the M249 as on the M416, tan(27.5) / tan(8.6).
inline constexpr float kPower = 3.44f;

}  // namespace acog

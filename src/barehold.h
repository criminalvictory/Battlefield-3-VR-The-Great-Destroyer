// Blackburn's bare arms (Semper Fidelis, The Great Destroyer: one draw, 52 bones, barestate.h) as the gloves under another numbering, so a gun held in bare hands
// gets everything the gloves get: the game's own hold learned on the gun's frame (skin.cpp's rig: the right hand round the pistol grip, the game's left on the
// handguard), and the fitted hands hands.cpp poses for the gloves (the magazine's hold, the lever's fists, the handguard, the shell's pinch). The bare hands' skeleton is
// the gloves' (each bone's vertices at the same rest place, within a centimetre: tools\baregloves.ps1), so a bare palette written in the gloves' numbers is a gloves'
// palette of the same frame: it goes through the gloves' own code, and what that places comes back by the same table. The forearms are the bare arms' own (the gloves
// have only their cuffs): they go by the arm solved from the hand as drawn (hands.cpp PlaceBareForearms).
// Header only and pure (no game calls, no logs), so tools/bareholdtest.cpp runs it without the game.
#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>

#include "armik.h"

namespace barehold {

constexpr int kBareBones = 52, kGloveBones = 45;

// Each bare arm bone as the gloves' (bonematch.ps1, the vertices at the same rest place): the right hand's 0-20 the right glove's 23-44, the left's 26-46 the left
// glove's 0-22, -1 the bare forearms' own (and the upper arms'). The palms are 41 and 15 (the gloves' 1 and 32); 46 and 20 the forearms' wrist ends (the gloves'
// cuffs' 0 and 37, twist bone B).
inline constexpr int8_t kBareGlove[kBareBones] = {
    27, 41, 43, 40, 33, 42, 31, 25, 26, 38, 23, 28, 29, 35, 36, 32, 24, 30, 34, 39, 37, -1, -1, -1, -1, -1,
    11, 20, 10, 21, 22, 7,  9,  19, 15, 6,  8,  12, 13, 18, 17, 1,  14, 16, 3,  2,  0,  -1, -1, -1, -1, -1};
constexpr int kBarePalm[2] = {41, 15};  // left, right
// Each side's palette entries, first and last (left, right).
constexpr int kBareSide[2][2] = {{26, 51}, {0, 25}};

// The forearms' bones, each side (left, right), wrist to elbow, with the solved frame each goes on (armik::Result::frame: 0 the elbow end, 1 twist bone A, 2 B):
// 46 / 20 at the wrist, 49 / 23, 50 / 24 in the forearm's middle, then the open end's rim: 47 / 21 and, past the elbow, 48 / 22 and the upper arm's 51 / 25.
constexpr int kForearmBones = 6;
constexpr int kForearm[2][kForearmBones][2] = {{{46, 2}, {49, 1}, {50, 0}, {47, 0}, {48, 0}, {51, 0}}, {{20, 2}, {23, 1}, {24, 0}, {21, 0}, {22, 0}, {25, 0}}};
constexpr int kRimFrom = 3;  // kForearm's entries from here on are the open elbow end's rim (its cap draws them in: armik::CapFrame)
// The gloves' three bones with no twin among the hands' (the left cuff's twist bone A and elbow end, the right cuff's A): the bare forearm's bones at the same places.
constexpr int kGloveOnly[3][2] = {{4, 49}, {5, 50}, {44, 23}};

constexpr bool ForearmOnly(int bare) {
    return bare >= 0 && bare < kBareBones && kBareGlove[bare] < 0;
}
constexpr bool GloveOnly(int glove) {
    return glove == kGloveOnly[0][0] || glove == kGloveOnly[1][0] || glove == kGloveOnly[2][0];
}
// How many bare bones have a glove's number (the two hands and the forearms' wrist ends).
constexpr int Mapped() {
    int n = 0;
    for (int8_t g : kBareGlove)
        n += g >= 0;
    return n;
}

// A bare arms' palette (52 bones of 12 floats) in the gloves' numbers (45): each hand bone under its glove's number, the gloves' three cuff bones from the bare
// forearm's bones there.
inline void ToGloves(const float* bare, float* gloves) {
    for (int k = 0; k < kBareBones; ++k)
        if (kBareGlove[k] >= 0)
            std::memcpy(gloves + kBareGlove[k] * 12, bare + k * 12, 12 * sizeof(float));
    for (const auto& only : kGloveOnly)
        std::memcpy(gloves + only[0] * 12, bare + only[1] * 12, 12 * sizeof(float));
}

// The way back: every bare bone with a glove's number takes that glove's bone; the forearms' own (ForearmOnly) are left as they are.
inline void FromGloves(const float* gloves, float* bare) {
    for (int k = 0; k < kBareBones; ++k)
        if (kBareGlove[k] >= 0)
            std::memcpy(bare + k * 12, gloves + kBareGlove[k] * 12, 12 * sizeof(float));
}

// The bare arms' palette as the gloves' levels' bare forearms' draw has its own (2016 from 13035, 18 entries: skin.cpp's kBareForearm), for what the rig learns from
// that draw (each forearm's middle, by its entries 0 and 12: the bare arms' 50 and 23): the forearm's bone and its helper (0, 3), the upper arm's two (1, 6), twist bones A and B (2, 4), the
// two at the wrist (8 with the hand's frame, 9 with the glove's bone 18); then the right's alike (10-17; 17 with its hand's frame). Entries without vertices stay as
// given.
constexpr int kForearmsDrawBones = 18;
inline constexpr int8_t kForearmsDraw[kForearmsDrawBones] = {50, 51, 49, 50, 46, -1, 48, -1, 41, 39, 24, 25, 23, 24, 20, -1, 22, 15};
inline void ToForearmsDraw(const float* bare, float* forearms) {
    for (int k = 0; k < kForearmsDrawBones; ++k)
        if (kForearmsDraw[k] >= 0)
            std::memcpy(forearms + k * 12, bare + kForearmsDraw[k] * 12, 12 * sizeof(float));
}

namespace detail {
inline void Compose(const float* a, const float* b, float* out) {  // a after b (three rows: rotation | translation)
    float r[12];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j)
            r[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        r[i * 4 + 3] += a[i * 4 + 3];
    }
    std::memcpy(out, r, sizeof(r));
}
inline bool Invert(const float* m, float* out) {
    const float a = m[0], b = m[1], c = m[2], d = m[4], e = m[5], f = m[6], g = m[8], h = m[9], i = m[10];
    const float det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
    if (!(std::fabs(det) > 1e-12f) || !std::isfinite(det))
        return false;
    const float r[9] = {(e * i - f * h) / det, (c * h - b * i) / det, (b * f - c * e) / det, (f * g - d * i) / det, (a * i - c * g) / det,
                        (c * d - a * f) / det, (d * h - e * g) / det, (b * g - a * h) / det, (a * e - b * d) / det};
    for (int k = 0; k < 3; ++k) {
        out[k * 4] = r[k * 3], out[k * 4 + 1] = r[k * 3 + 1], out[k * 4 + 2] = r[k * 3 + 2];
        out[k * 4 + 3] = -(r[k * 3] * m[3] + r[k * 3 + 1] * m[7] + r[k * 3 + 2] * m[11]);
    }
    return true;
}
inline void Point(const float* m, const float* p, float* out) {
    const float r[3] = {m[0] * p[0] + m[1] * p[1] + m[2] * p[2] + m[3], m[4] * p[0] + m[5] * p[1] + m[6] * p[2] + m[7],
                        m[8] * p[0] + m[9] * p[1] + m[10] * p[2] + m[11]};
    std::memcpy(out, r, sizeof(r));
}
}  // namespace detail

// One side's forearm as solved from its hand as drawn (hands.cpp PlaceBareForearms): the elbow end and twist bones A and B, and how much of a cap its open elbow
// end gets (armik::EndCap). have false: none solved.
struct Arm {
    bool have = false;
    const float (*frame)[12] = nullptr;
    float cap = 0.0f;
};

struct Placed {
    bool solved[2] = {};     // that side's forearm went by its solved arm
    float wristOff[2] = {};  // how far the solved forearm's wrist end was from the hand's wrist as drawn (metres; solved only within kWristTogether)
};
// A solved forearm goes with a hand only while their wrists are in one place: solved from another hand than the one drawn (the first frames of a gun in hand, before
// its hold is known; a frame the two sides took different ways), the forearm goes whole with the hand instead, as the game holds it, and nothing tears at the wrist.
constexpr float kWristTogether = 0.02f;

// The bare arms as drawn with a gun in hand, from the gloves' palette as skin.cpp's rig placed it (the game's hold on the gun, the hands' own fitted poses): bare is
// the game's own palette (52 bones) and is changed in place, left then right (side 0, 1).
//   - A hand that's there: each of its bones is its glove's. Its forearm is the arm solved from that hand (kForearm; the open end's rim drawn in by the cap), or,
//     with none solved for this hand, the game's own forearm moved whole as the palm was.
//   - A hand that isn't (folded away, as the gloves'): all of that side folds onto its fold point: nothing of it is drawn.
inline Placed FromRig(const float* gloves, const bool here[2], const float fold[2][3], const Arm arm[2], float* bare) {
    Placed placed;
    for (int side = 0; side < 2; ++side) {
        const int first = kBareSide[side][0], last = kBareSide[side][1];
        if (!here[side]) {
            for (int k = first; k <= last; ++k) {
                float* bone = bare + k * 12;
                std::memset(bone, 0, 12 * sizeof(float));
                bone[3] = fold[side][0], bone[7] = fold[side][1], bone[11] = fold[side][2];
            }
            continue;
        }
        float gamePalm[12];
        std::memcpy(gamePalm, bare + kBarePalm[side] * 12, sizeof(gamePalm));
        for (int k = first; k <= last; ++k)
            if (kBareGlove[k] >= 0)
                std::memcpy(bare + k * 12, gloves + kBareGlove[k] * 12, 12 * sizeof(float));
        const float* palm = bare + kBarePalm[side] * 12;
        const float wristBind[3] = {side == 0 ? armik::kWrist[0] : -armik::kWrist[0], armik::kWrist[1], armik::kWrist[2]};
        if (arm[side].have && arm[side].frame) {
            float handWrist[3], armWrist[3];
            detail::Point(palm, wristBind, handWrist);
            detail::Point(arm[side].frame[2], wristBind, armWrist);
            const float d[3] = {handWrist[0] - armWrist[0], handWrist[1] - armWrist[1], handWrist[2] - armWrist[2]};
            placed.wristOff[side] = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
            placed.solved[side] = placed.wristOff[side] <= kWristTogether;  // (a number not finite: not solved)
        }
        if (placed.solved[side]) {
            float capped[12];
            armik::CapFrame(arm[side].frame[0], side == 1, arm[side].cap, capped);
            for (int f = 0; f < kForearmBones; ++f)
                std::memcpy(bare + kForearm[side][f][0] * 12, f >= kRimFrom ? capped : arm[side].frame[kForearm[side][f][1]], 12 * sizeof(float));
        } else {
            // Whole with the hand: the game's own forearm moved as the palm bone was. (The wrist end, 46 / 20, is the gloves' cuff's bone and has come back with them.)
            float inverse[12], move[12];
            if (detail::Invert(gamePalm, inverse)) {
                detail::Compose(palm, inverse, move);
                for (int k = first; k <= last; ++k)
                    if (kBareGlove[k] < 0)
                        detail::Compose(move, bare + k * 12, bare + k * 12);
            }
        }
    }
    return placed;
}

}  // namespace barehold

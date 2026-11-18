// Blackburn's bare arms and the body drawn with them (Semper Fidelis, The Great Destroyer), by their draws: the two states the game has them in, in one
// table. Header only and pure (no game calls, no logs), so tools/barestatetest.cpp runs it without the game.
//
// Handcuffed (the train, the sewer until the grapple): the arms are 18108 indices from 16053, the cuff round the right wrist a draw of its own before
// them in the same buffer (2682 from 13371). After the grapple's QTE the cuffs come off (found in the game 2026-11-18 10:00, Semper Fidelis with the
// AKS-74u in hand): the cuff's 2682 indices leave the buffer, so THE SAME ARM MESH is drawn 18108 from 13371 (16053 - 2682), and the body's draws
// move with it: the torso and the legs change places (the legs 7545 from 0 and the torso 5604 from 7545, for the torso 5604 from 0 and the legs 7545
// from 5604), the body's depth-only copy loses the cuff's too (23664 from 31479, for 26346 from 34161), the one hand's depth-only copy starts 5364
// earlier (7815 from 55143, for 60507). Known only by the first key, the arms were left as the game holds them with a gun in hand. The recordings:
// build\survey\AKS74u_*.bin, UMP45_*.bin, M1014_*.bin (handcuffed), aks_test_idle.bin (the cuffs off); the same skeleton in both (the palms 2.214
// apart on the AKS-74u in each).
//
// A draw's key says only which mesh it may be: skin.cpp still asks for the player's own palette (its size, the first bone by the eye) before it
// moves the arms, and hides the body only while the arms were drawn moved a moment ago, as before. The torso with its sleeves and the strip over the
// shoulders are drawn posed with the arms instead (PartOf says which draw is which; skin.cpp's DrawBareSleeves), the rest stays hidden.
#pragma once

#include <cstdint>

namespace barestate {

struct Draw {
    uint32_t count, start;  // the draw's index count and its first index (the base vertex is 0 for all of them)
};

enum Kind : uint8_t {
    kNone,  // nothing of the bare arms' levels
    kArms,  // the bare arms' own draw
    kCuff,  // the cuff round the right wrist
    kBody,  // the body drawn with them: hidden while the arms are on the controllers, but for the torso and the strip posed with them (Part)
};

// The body's draws by what each is (the mesh dump, build\mesh_18108_16053: where each one's vertices lie). Until 2026-11-18 the first two were named
// the wrong way round here (5604 the legs): 5604 indices are the torso and both sleeves (y 0.895 to 1.576, the arms out to x +-0.39), 7545 the legs
// (the feet at y 0). Nothing went by the names then: all were hidden alike.
struct State {
    const char* name;
    Draw arms;
    Draw cuff;       // count 0: none in this state
    Draw torso;      // the torso and both sleeves, 13 bones: posed with the arms (barehold.h's kSleeveTorso)
    Draw legs;       // hidden
    Draw bodyDepth;  // the body's depth-only copy, a second forearm and hand in it: hidden
    Draw handDepth;  // one hand's depth-only copy (a skeleton of its own, 21 bones): hidden
};
inline constexpr State kStates[] = {
    {"handcuffed", {18108, 16053}, {2682, 13371}, {5604, 0}, {7545, 5604}, {26346, 34161}, {7815, 60507}},
    {"the cuffs off", {18108, 13371}, {0, 0}, {5604, 7545}, {7545, 0}, {23664, 31479}, {7815, 55143}},
};
constexpr int kStateCount = static_cast<int>(sizeof(kStates) / sizeof(kStates[0]));

// The body's draws that are the same in both states: the strip of uniform over both shoulders (222 from 13149, the legs' shader; there with the cuffs
// off too: 7 bones, posed with the torso, barehold.h's kSleeveStrip), then the chain's depth-only copy (3492 from 3492), the chain and its open cuff
// (3492 from 0), the train pistol's depth-only copy (11970 from 11970).
inline constexpr Draw kStrip{222, 13149};
inline constexpr Draw kBodyEither[] = {kStrip, {3492, 3492}, {3492, 0}, {11970, 11970}};

// A state's four body draws together (the torso, the legs, the two depth-only copies), for going through them.
constexpr int kStateBody = 4;
struct BodyDraws {
    Draw draws[kStateBody];
    constexpr const Draw* begin() const { return draws; }
    constexpr const Draw* end() const { return draws + kStateBody; }
};
constexpr BodyDraws BodyOf(const State& s) {
    return {{s.torso, s.legs, s.bodyDepth, s.handDepth}};
}

constexpr bool Same(const Draw& d, uint32_t count, uint32_t start) {
    return d.count != 0 && d.count == count && d.start == start;
}

// Which state's arms this draw is (0 handcuffed, 1 the cuffs off), -1 when it isn't the arms' draw at all.
constexpr int ArmsState(uint32_t count, uint32_t start, int32_t base) {
    if (base != 0)
        return -1;
    for (int s = 0; s < kStateCount; ++s)
        if (Same(kStates[s].arms, count, start))
            return s;
    return -1;
}

// Which of the body's draws this is, in either state (kPartNone: none of them).
enum Part : uint8_t {
    kPartNone,
    kTorso,      // the torso and sleeves
    kLegs,
    kBodyDepth,
    kHandDepth,
    kStripPart,  // the strip over the shoulders
    kPartOther,  // the chain, its depth-only copy, the pistol's
};
constexpr Part PartOf(uint32_t count, uint32_t start, int32_t base) {
    if (base != 0)
        return kPartNone;
    for (const State& s : kStates) {
        if (Same(s.torso, count, start))
            return kTorso;
        if (Same(s.legs, count, start))
            return kLegs;
        if (Same(s.bodyDepth, count, start))
            return kBodyDepth;
        if (Same(s.handDepth, count, start))
            return kHandDepth;
    }
    if (Same(kStrip, count, start))
        return kStripPart;
    for (const Draw& b : kBodyEither)
        if (Same(b, count, start))
            return kPartOther;
    return kPartNone;
}
// Which state's torso this draw is (0 handcuffed, 1 the cuffs off), -1 when it isn't the torso's.
constexpr int TorsoState(uint32_t count, uint32_t start, int32_t base) {
    if (base != 0)
        return -1;
    for (int s = 0; s < kStateCount; ++s)
        if (Same(kStates[s].torso, count, start))
            return s;
    return -1;
}

// What a draw is on a level with bare arms, in either state.
constexpr Kind KindOf(uint32_t count, uint32_t start, int32_t base) {
    if (base != 0)
        return kNone;
    for (const State& s : kStates) {
        if (Same(s.arms, count, start))
            return kArms;
        if (Same(s.cuff, count, start))
            return kCuff;
    }
    return PartOf(count, start, base) != kPartNone ? kBody : kNone;
}

}  // namespace barestate

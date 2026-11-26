// What a magazine dropped from the gun in hand is made of, for the sound it makes on the ground (hands.cpp's loose hits): the AK family's, the UMP-45's and
// the Saiga 12K's polymer magazines knock as hollow plastic, the M249's cloth pouch lands as the game's own ammo bag (the review of 2026-11-18: after its first landing
// it rattled as metal), every other as metal. By skin.cpp's HeldShape. Pure.

#pragma once

#include <cstring>

namespace magsound {

enum class Stuff { kMetal, kPlastic, kCloth };

inline Stuff Of(const char* shape) {
    if (!shape)
        return Stuff::kMetal;
    if (!std::strcmp(shape, "m249"))
        return Stuff::kCloth;
    for (const char* polymer : {"ak74", "an94", "aek971", "rpk", "aks74u", "ump45", "saiga12k"})  // (the AKS-74u's is the AK-74's; the UMP-45's and the Saiga 12K's are polymer too)
        if (!std::strcmp(shape, polymer))
            return Stuff::kPlastic;
    return Stuff::kMetal;
}

// The game's own ammo bag landing (gamesounds.cpp's bag_v0_loose ... bag_v6_loose: Bag_Collision_Wave's seven takes).
constexpr int kBagTakes = 7;

}  // namespace magsound

// The left hand round each gun's own magazine, fitted by tools/magfit.cpp from the game's own reload: raised to the middle of the magazine, its fingers laid on the mesh one by one (the user, 2026-11-05: "Make sure
// that all the finger formations are really pressed against the magazine, and he's not holding the magazine too low"). For each gun: the magazine in the glove and the glove's bones.
#pragma once

#include "handpose.h"

namespace magposes {

#include "magposes/an94.h"
#include "magposes/rpk.h"
#include "magposes/aek971.h"
#include "magposes/ak74.h"
#include "magposes/g3a3.h"
#include "magposes/kh2002.h"
#include "magposes/m16a4.h"
#include "magposes/m4a1.h"
#include "magposes/m240.h"
#include "magposes/aks74u.h"
#include "magposes/ump45.h"
#include "magposes/mk11.h"
// The M249's box (a cloth pouch under the gun) at its left end, as its own reload holds it, its palm brought onto the pouch and its fingers closed on it,
// the cloth giving (tools/magfit.cpp soft).
#include "magposes/m249.h"
// The Saiga 12K's box magazine, a mesh of its own (its socket's), from its own reload's hand on it while still seated (tools/magfit.cpp grip ... seated).
#include "magposes/saiga12k.h"
// The hand on a side lever that hardly stands off the gun, as the game's own reload has it (the user, 2026-11-05: "exactly positioned with the lever... next to the lever on the right side of the gun"), the fingers closed on the lever.
#include "magposes/aek971_lever.h"
#include "magposes/ak74_lever.h"
#include "magposes/an94_lever.h"
#include "magposes/g3a3_lever.h"
#include "magposes/aks74u_lever.h"
#include "magposes/ump45_lever.h"
#include "magposes/saiga12k_lever.h"
// The M416's own two fingers on a T handle (the user, 2026-11-05: "that like specific two-finger pull"), and the index hooked round the KH2002's fin, through its handle.
#include "magposes/m16a4_lever.h"
#include "magposes/m4a1_lever.h"
#include "magposes/mk11_lever.h"
#include "magposes/kh2002_lever.h"
// The other way round a side lever (the user, 2026-11-05: "let's have an under and over lever pull"): a fist from below on each; from above too on the two with a long knob, where the game's own hand on it is no use.
#include "magposes/an94_lever_under.h"
#include "magposes/aek971_lever_under.h"
#include "magposes/ak74_lever_under.h"
#include "magposes/aks74u_lever_under.h"
#include "magposes/saiga12k_lever_under.h"
#include "magposes/rpk_lever_under.h"
#include "magposes/rpk_lever_over.h"
#include "magposes/m240_lever_under.h"
#include "magposes/m240_lever_over.h"
// The M240's lid and belt by hand.
#include "magposes/m240_lid.h"
#include "magposes/m240_belt.h"
// The M249's: its handle over and under, its feed cover, its belt (the pinch is the M240's: the glove's own), its box's way off and on (tools/boxway.cpp).
#include "magposes/m249_lever_over.h"
#include "magposes/m249_lever_under.h"
#include "magposes/m249_lid.h"
#include "magposes/m249_belt.h"
#include "magposes/m249_way.h"
// The PKP's (Operation Swordbreaker): the game's own reload does it all with the RIGHT hand (its left stays on the handguard), so each hand is that one
// mirrored onto the left glove and fitted on this gun's own mesh (tools/magfit.cpp vanillaright, then soft): its box at its left end, its feed cover on
// top; its handle over and under (magfit over, under); its belt as its own reload has it; its box's way off and on (tools/boxway.cpp).
#include "magposes/pkp.h"
#include "magposes/pkp_lid.h"
#include "magposes/pkp_lever_over.h"
#include "magposes/pkp_lever_under.h"
#include "magposes/pkp_belt.h"
#include "magposes/pkp_way.h"
// The Stinger's aerial folding, lifted by the hand, springing open.
#include "magposes/fim92_flap.h"

}

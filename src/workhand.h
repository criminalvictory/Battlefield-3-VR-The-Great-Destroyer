#pragma once
// Which hand works the gun's parts, on its own so tools/otherhandtest.cpp runs it with grasp.h and handmag.cpp as hands.cpp does. Pure, header only.
//
// The owner, 2026-11-19 (the M249 on Operation Swordbreaker's bridge, held by its front with the left, the right on its charging handle after laying the
// belt, and nothing took the handle): "I should be able to hold a barrel of a gun and still pull whatever lever ... that needs to be a basic mechanic
// where you can hold a gun by any barrel instead of the trigger grip ... and still pull whatever lever with whichever hand." The rule, for every gun:
//   - While one hand holds the gun, by its pistol grip or by its front alone (the handguard, the barrel, a front grip), the OTHER hand is the hand at work:
//     it works every part the gun has (a charging handle or a lever over and under, a bolt, a feed cover, the belt, a box or magazine out and in, a bolt
//     catch, the tube's loading, the grenade launcher), each module by that hand's own controller. Either hand in either role.
//   - The gun goes where the holding hand alone has it (grasp.h: frozen on the hand holding its front, or on the pistol grip's hand); its trigger fires it
//     only for a hand on its pistol grip (grasp.h's fireHand: none while it's held by its front alone).
//   - A hand that lets go of the part goes back to holding (a fresh squeeze on the pistol grip or the front) or is free.
//   - Nobody while it lies, flies in from the ground or is put away; the left while free hands are dormant (the right holds the gun as the game does), as
//     before: everything held the usual way is as it was, bit for bit.
// A module that serves one hand at a time (handmag.cpp: every held gun's magazine, box, lever, lid and belt) is fed the hand it has something in until
// it's done, else the hand at work (the right only while its own pose is drawn: freehands.txt "rightpose 0" leaves its mechanics off); the left for
// weapon.txt "heldtest"'s scripted left. A hand with something in it that's no longer the hand at work (the gun back in that hand, as at a new life;
// the right as free hands go to sleep) lets go of it, as when the gun leaves the hands; not on the frame the gun's let go of (the module lets go there
// itself, from where it was drawn).

#include "grasp.h"

namespace workhand {

// The hand at work: 0 the left, 1 the right, -1 nobody. enabled: free hands awake for the gun in hand.
inline int WorkHand(bool enabled, grasp::Holder holder, int trigger, int barrel) {
    if (!enabled)
        return 0;
    if ((holder == grasp::Holder::kHeld || holder == grasp::Holder::kGlide) && (trigger == 0 || trigger == 1))
        return 1 - trigger;
    if (holder == grasp::Holder::kBarrel && (barrel == 0 || barrel == 1))
        return 1 - barrel;
    return -1;
}

// The hand a one-hand module is fed: busy, the hand it has something in (-1 none); work, WorkHand's; rightWorks, the right's own pose drawn; leftScripted,
// weapon.txt "heldtest"'s scripted left.
inline int FedSide(int busy, int work, bool rightWorks, bool leftScripted) {
    if (leftScripted)
        return 0;
    if (busy == 0 || busy == 1)
        return busy;
    return work == 1 && rightWorks ? 1 : 0;
}

// The hand with something in it (busy) is no longer the hand at work: it lets go of it. freeOn: free hands awake; lying: the gun let go of (kLoose).
// Free hands asleep (seated, the train pistol, a long switch): the left alone works the gun as before them, so the right lets go of what it has (a magazine
// falls, a lever drawn springs home and chambers as let go of; the review of fc010e2: it had been kept with its hand taken for lost, then gone without a
// fall); the left keeps its own, as ever. (Asleep for a frame, the grasp puts the gun back in the right hand and the right lets go of it the frame after
// all the same.)
inline bool NoLongerAtWork(int busy, int work, bool freeOn, bool lying) {
    if (!freeOn)
        return busy == 1;
    return (busy == 0 || busy == 1) && !lying && work != busy;
}

// A lever drawn far enough and let go of (handmag's charged) chambers a round: the chamber was empty, a magazine is in, the belt is laid with the lid
// shut, and the count holds a round more than the one the gun keeps.
inline bool LeverChambers(bool charged, bool dry, bool magazineOut, bool beltReady, bool counted, int loaded) {
    return charged && dry && !magazineOut && beltReady && counted && loaded > 1;
}

// A held gun fires (hands.cpp's HeldChamberEmpty the other way round): a round chambered, its belt laid and its lid shut, no handle resting in its notch.
inline bool HeldMayFire(bool dry, bool beltBlocked, bool notchBlocked) {
    return !dry && !beltBlocked && !notchBlocked;
}

}  // namespace workhand

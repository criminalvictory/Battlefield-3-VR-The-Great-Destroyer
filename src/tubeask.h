#pragma once
// The asking for the game's own reload of a tube shotgun, as shotgunammo.h's caller is to keep it (docs\research\shotgun-shells.md, section 6's
// contract; ammo.cpp's StepTube runs it around each look, tools\tubeasktest.cpp without the game). Pure: no game calls, no clock of its own.
//   - Asked is the reload key only (X held with the gun in the hands, the game's R): shotgunammo::Asked on how long ago the key was let go and how
//     long ago the game's count last rose by a shell that was kept. Nothing else asks: never a swap of the game's.
//   - A shot ends the asking (CountOut.shot): both times are forgotten, and a key still held through the shot asks again only once it has been let
//     go and pressed anew.
//   - "The asking began on an empty gun" (CountIn.fromEmpty) is latched as the asking begins and dropped when the asking is over and the hidden round
//     is back in the game's count.
//   - When the game's count last rose by itself is kept for CountIn.sinceRise.
//   - The trigger pulled on the gun's last shell while the hidden round is out for an asking (End): the asking is ended there, as a shot ends it, so the
//     next look puts the hidden round back before the shot is let through (input.cpp holds the shot that frame). Fired with the hidden round out, the
//     last shell left the game's count at 0 and the game began its own reload by itself before the mod's next look.
// Times are the caller's milliseconds (any clock that only goes forward; 0 is "never" for the two it keeps, so a clock never at 0).

#include "shotgunammo.h"

namespace tubeask {

constexpr unsigned long long kNever = ~0ULL;

struct State {
    bool dead = false;            // a shot ended the asking with the key still held (or since): no asking until a fresh press
    bool letGo = false;           // ... and the key has been let go since that shot
    unsigned long long shellAt = 0;  // when the game's count last rose by a kept shell (0 never)
    unsigned long long roseAt = 0;   // when the game's count last rose by itself at all (0 never)
    bool fromEmpty = false;       // the asking began on an empty gun
    bool askedWas = false;        // the last look was asked (and saw no shot)
};

// Before a look. keySince: how long ago the reload key was let go, 0 while it is held, kNever if never pressed. total: the shells in the gun as the
// mod counts them (shotgunammo::Total). Fills the look's asked, fromEmpty and sinceRise; gives asked.
inline bool Before(State& a, unsigned long long keySince, unsigned long long now, int total, shotgunammo::CountIn& in) {
    const bool keyDown = keySince == 0;
    if (a.dead) {
        if (!keyDown)
            a.letGo = true;
        else if (a.letGo)
            a.dead = false, a.letGo = false;  // a fresh press
    }
    const unsigned long long sinceX = a.dead ? kNever : keySince;
    const unsigned long long sinceShell = a.shellAt ? now - a.shellAt : kNever;
    const bool asked = shotgunammo::Asked(sinceX, sinceShell);
    if (asked && !a.askedWas)
        a.fromEmpty = total == 0;  // the asking begins: on an empty gun the game plays its empty reload
    in.asked = asked;
    in.fromEmpty = a.fromEmpty;
    in.sinceRise = a.roseAt ? now - a.roseAt : kNever;
    return asked;
}

// The asking ended without a shot seen (the trigger pulled: above): as a shot ends it. keySince as Before's.
inline void End(State& a, unsigned long long keySince) {
    a.shellAt = 0;
    a.dead = true;
    a.letGo = keySince != 0;
}

// After the look. asked: what Before gave; keySince: as given to Before; hidden: the hidden round as it is after the look (CountOut.hidden once its
// write is made, else as it was).
inline void After(State& a, const shotgunammo::CountOut& out, bool asked, unsigned long long keySince, unsigned long long now, int hidden) {
    if (out.rose)
        a.roseAt = now;
    if (out.gameLoaded > 0)
        a.shellAt = now;
    if (out.shot) {
        a.shellAt = 0;
        a.dead = true;
        a.letGo = keySince != 0;
    }
    a.askedWas = asked && !out.shot;
    if (!a.askedWas && hidden)
        a.fromEmpty = false;
}

}  // namespace tubeask

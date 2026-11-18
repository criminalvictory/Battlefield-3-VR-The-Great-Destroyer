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
//   - The key is the gun's own (Key, KeySince): it counts as down only while this gun is the one in the hands, and only from a press begun with it
//     there. A reload key held or let go for another gun (the shotgun put away, lying, not yet drawn) is nothing to this one: fired dry, put away,
//     the other gun reloaded and the shotgun drawn again inside kAskedMs, its hidden round stays in and the game never reloads it by itself. A key
//     still down as the gun comes into the hands (a flag left set, R held for a pickup) asks nothing until it has been seen up.
//   - A gun the game swapped out with its hidden round in its count carries that round in the pickup; taken back, it's met as already carrying it
//     (Meet, carried), not a shell richer.
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

// The reload key as this gun has it.
struct Key {
    bool seenUp = false;          // the key seen up with the gun in the hands, since it came into them: a press from now is this gun's
    bool down = false;            // ... down for this gun
    unsigned long long upAt = 0;  // when it was last let go for this gun (0 never)
};

// Each look. inHands: this gun is the one in the hands (its slot the hands' slot, drawn there). keyDown: the reload key is down just now, whoever
// it is for. Gives Before's keySince: 0 while down for this gun, else how long ago it was let go for it, kNever if never.
inline unsigned long long KeySince(Key& k, bool inHands, bool keyDown, unsigned long long now) {
    if (!inHands) {
        if (k.down)
            k.upAt = now;  // (leaving the hands with the key down: let go there, for this gun)
        k.down = false;
        k.seenUp = false;
    } else if (!keyDown) {
        if (k.down)
            k.upAt = now;
        k.down = false;
        k.seenUp = true;
    } else if (k.seenUp) {
        k.down = true;
    }
    if (k.down)
        return 0;
    if (!k.upAt)
        return kNever;
    return now > k.upAt ? now - k.upAt : 1;  // (never 0: that is "held")
}

// The gun as it's met (ammo.cpp's UpdateTube): raw the game's count of it, capacity its tube's, hidden what the mod knows of this firing state
// (1: its hidden round is in that count), carried: the game swapped this very gun out with its hidden round in and it has come back with the same
// count. total: the shells in it; hidden: as it is from now (0: it goes on top at the next look).
struct Met {
    int total = 0;
    int hidden = 0;
    int how = 0;  // 0 the hidden round to go on top; 1 known in; 2 a count above a full gun; 3 carried through a pickup
};
inline Met Meet(int raw, int capacity, int hidden, bool carried) {
    Met m;
    if (hidden) {
        m.total = raw > 1 ? raw - 1 : 0, m.hidden = hidden, m.how = 1;
    } else if (raw > capacity + 1) {
        m.total = raw - 1, m.hidden = 1, m.how = 2;  // (a checkpoint brought the count back with the hidden round in it)
    } else if (carried && raw >= 1) {
        m.total = raw - 1, m.hidden = 1, m.how = 3;
    } else {
        m.total = raw;
    }
    return m;
}

// Before a look. keySince: how long ago the reload key was let go, 0 while it is held, kNever if never pressed (the gun's own: KeySince). total: the shells in the gun as the
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

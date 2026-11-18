#pragma once
// The train pistol's rounds with its magazine by hand (the M9 of Semper Fidelis and The Great Destroyer; build\pistol.txt "reload on", "mag 1"):
// its own mechanism, nothing the M416's. Pure, no game calls and no logs: ammo.cpp runs it on the game's count, hands.cpp tells it what the hands
// did, tools\pistolammotest.cpp runs it without the game.
//
// The pistol as the mod counts it: a round in the chamber or none, a magazine in the grip or none, the rounds in that magazine, and the slide held
// open by its stop or not. The game's own count of it is kept one above the mod's (the hidden round, as the rifles'): the trigger stops at an empty
// chamber, so the game never runs it dry and never reloads it by itself.
//   - A shot (the game's count down by one, the mod writing nothing) takes the chambered round; the slide feeds the next from the magazine. The last
//     one fired with a magazine in: the slide stays open on its stop. With no magazine in (the chambered round fired after the magazine came out) it
//     goes home on an empty chamber.
//   - The magazine out by hand: its rounds go back to the spare ones with it; the chambered round stays, and the slide stays where it is.
//   - A fresh magazine seated by hand: as many rounds as it holds, or as the spare ones have, taken from the spare ones. It chambers nothing.
//   - The slide: let go off its stop (a tug by hand, or the trigger's first pull with a fresh magazine in: the user's answer, 2026-10-31) it goes
//     home and chambers a round from the magazine; with an empty magazine in the stop takes it again; with none in it goes home empty. Racked by
//     hand from home: a round in the chamber is thrown out, lost (the user's answer), and the next is fed, as above.
//   - The trigger with nothing in the chamber fires nothing.
//   - The game's own reload, asked for (X held): kept, the pistol as the game left it (a magazine in, a round chambered). Not asked for: undone, its
//     rounds back to the spare ones. The hidden round fired (the mouse passes no trigger's stop): put back from the spare ones, a few times a
//     magazine.

namespace pistolammo {

struct State {
    bool magIn = true;     // a magazine in the grip
    int mag = 0;           // the rounds in it (0 with none in)
    bool chamber = false;  // a round in the chamber
    bool locked = false;   // the slide held open by its stop
};

// The rounds in the pistol.
inline int Total(const State& s) {
    return (s.chamber ? 1 : 0) + (s.magIn && s.mag > 0 ? s.mag : 0);
}

// The pistol as a count of the game's leaves it (a new gun, the game's own reload): a magazine in, a round chambered if it has any; none at all,
// the slide on its stop.
inline State FromCount(int total) {
    State s;
    s.magIn = true;
    s.chamber = total >= 1;
    s.mag = total >= 1 ? total - 1 : 0;
    s.locked = total < 1;
    return s;
}

// The mod's count gone astray from the game's (never by the rules below; a count something else wrote): the pistol as that count has it, the
// magazine left out if it was and the count has no rounds for one.
inline void Adopt(State& s, int total) {
    if (total < 0)
        total = 0;
    s.chamber = total >= 1;
    const int rest = total - (s.chamber ? 1 : 0);
    if (rest > 0)
        s.magIn = true;
    s.mag = s.magIn ? rest : 0;
    s.locked = s.magIn && total < 1;
}

// Shots: each takes the chambered round and feeds the next from the magazine. How many it had rounds for. The last one fired with a magazine in:
// the slide on its stop.
inline int Fire(State& s, int shots) {
    int fired = 0;
    for (; fired < shots && s.chamber; ++fired) {
        s.chamber = false;
        if (s.magIn && s.mag > 0) {
            --s.mag;
            s.chamber = true;
        }
    }
    if (fired > 0 && !s.chamber)
        s.locked = s.magIn;
    return fired;
}

// The magazine out of the grip: the rounds that go back to the spare ones with it (-1: none was in).
inline int Eject(State& s) {
    if (!s.magIn)
        return -1;
    const int back = s.mag > 0 ? s.mag : 0;
    s.magIn = false;
    s.mag = 0;
    return back;
}

// A fresh magazine seated: the rounds it brings from the spare ones (-1: one is in already).
inline int Insert(State& s, int capacity, int spare) {
    if (s.magIn)
        return -1;
    int take = capacity < spare ? capacity : spare;
    if (take < 0)
        take = 0;
    s.magIn = true;
    s.mag = take;
    return take;
}

// The slide let go: racked by hand from home, tugged off its stop, or sent home by the trigger.
struct Racked {
    bool wasLocked = false;  // off its stop
    bool threw = false;      // the chambered round thrown out (lost)
    bool chambered = false;  // a round fed from the magazine
    bool locked = false;     // the stop holds it open (again): an empty magazine in
};
inline Racked Rack(State& s) {
    Racked r;
    r.wasLocked = s.locked;
    r.threw = s.chamber;
    s.chamber = false;
    if (s.magIn && s.mag > 0) {
        --s.mag;
        s.chamber = true;
        s.locked = false;
        r.chambered = true;
    } else {
        s.locked = s.magIn;
    }
    r.locked = s.locked;
    return r;
}

// The right trigger pulled.
enum class Pull : int {
    kFires = 0,      // a round in the chamber: the game's fire button
    kClick = 1,      // nothing in the chamber, and nothing the trigger does about it
    kSendsHome = 2,  // the slide on its stop with a fresh magazine in: this pull only sends it home (it chambers), the next fires
    kRacks = 3,      // the slide home on an empty chamber with rounds in the magazine, and the slide not worked by hand (pistol.txt "slide 0"):
                     // this pull cycles it (it chambers), the next fires. With the slide by hand, the hand racks it.
};
inline Pull OnPull(const State& s, bool slideByHand) {
    if (s.chamber)
        return Pull::kFires;
    if (s.magIn && s.mag > 0 && s.locked)
        return Pull::kSendsHome;
    if (s.magIn && s.mag > 0 && !slideByHand)
        return Pull::kRacks;
    return Pull::kClick;
}

// ---- The game's count of it, a frame (ammo.cpp): what the game did, and what the mod writes. ----
constexpr int kMostPutBacks = 3;  // the hidden round put back this many times since its count was last 2 or more (pickrules.h's, the rifles')
struct CountIn {
    int last = -1;       // the game's count when the mod last looked (after what it wrote then); -1 not looked yet
    int raw = 0;         // ... now
    int spare = 0;       // its spare rounds now
    int hidden = 0;      // the hidden round in the game's count (1 once it has it)
    bool asked = false;  // the game's own reload asked for (X held lately and no magazine seated by hand since, a swap of the game's just now)
    int putBacks = 0;    // the hidden round's put-backs since the count was last 2 or more
    int lost = 0;        // rounds thrown out by the slide since the count was last written (they go to no one)
};
struct CountOut {
    int fired = 0;             // shots the pistol had rounds for
    bool hiddenFired = false;  // ... and one more: the hidden round
    bool lockedNow = false;    // those shots left the slide on its stop
    bool kept = false;         // the game's own reload, asked for: the pistol as the game left it
    bool undone = false;       // the game's own reload, not asked for: its rounds back to the spare ones
    bool putBack = false;      // the hidden round put back from the spare ones
    bool settled = false;      // what the hands did (a magazine out or in, a round thrown out) written into the game's count
    bool adopted = false;      // the two counts apart and the spare rounds short of the difference: the pistol as the game's count has it
    bool write = false;        // loaded and spare below are to be written
    int loaded = 0, spare = 0;
};
inline CountOut See(State& s, const CountIn& in) {
    CountOut out;
    out.loaded = in.raw, out.spare = in.spare;
    if (in.raw < 0 || in.spare < 0)
        return out;
    const int hidden = in.hidden > 0 ? 1 : 0;
    if (in.last >= 0 && in.raw < in.last) {
        const bool was = s.locked;
        const int shots = in.last - in.raw;
        out.fired = Fire(s, shots);
        out.hiddenFired = shots > out.fired;
        out.lockedNow = s.locked && !was;
    } else if (in.last >= 0 && in.raw > in.last && in.asked) {
        // (As the rifles': the game's count is the pistol's, the hidden round goes back on top of it.)
        out.kept = true;
        s = FromCount(in.raw);
        out.loaded = in.raw + hidden;
        out.write = hidden > 0;
        return out;
    }
    const int want = hidden + Total(s);
    if (in.raw == want)
        return out;
    // The hidden round fired with nothing else in the pistol: back from the spare ones, unless the game's reload is wanted, none are left, or it
    // has been put back too often (the game then reloads by itself, and that is undone).
    if (in.raw == 0 && hidden && Total(s) == 0) {
        if (in.asked || in.spare < 1 || in.putBacks >= kMostPutBacks)
            return out;
        out.putBack = true;
        out.write = true;
        out.loaded = 1;
        out.spare = in.spare - 1;
        return out;
    }
    // The game's count to the mod's, the difference to or from the spare ones; the rounds the slide threw out to no one.
    const int lost = in.lost > 0 ? in.lost : 0;
    const int spare = in.spare + (in.raw - want) - lost;
    if (spare < 0 && in.raw < want) {
        Adopt(s, in.raw - hidden);
        out.adopted = true;
        return out;
    }
    out.undone = in.last >= 0 && in.raw > in.last;
    out.settled = !out.undone;
    out.write = true;
    out.loaded = want;
    out.spare = spare > 0 ? spare : 0;
    return out;
}

}  // namespace pistolammo

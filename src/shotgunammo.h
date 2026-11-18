#pragma once
// The M1014's shells by hand (the Benelli M4: a semi-automatic 12-gauge with a tube under the barrel; docs\research\shotgun-shells.md): its own
// mechanism, nothing the pistol's or the M416's. Pure, no game calls and no logs: tools\shotgunammotest.cpp runs it without the game. Nothing in
// the game runs it yet (the gun isn't measured): ammo.cpp is to run it on the game's count, hands.cpp to tell it what the hands did, as they do
// pistolammo.h.
//
// The shotgun as the mod counts it: the shells in the tube, the chamber (empty, a live shell, or a fired hull: the pump's, later), the bolt held
// open or home, a shell lying in the ejection port, and the hammer down or cocked. The tube's size is the caller's number (the game's
// MagazineCapacity less one: 4, 6 with the longer tube; the chamber is the one more): nothing here knows it. The game's own count of the gun
// is kept one above the mod's (the hidden round, as the pistol's): the trigger stops at an empty chamber, so the game never runs it dry and
// never reloads it by itself.
//   - A shot (the game's count down by one, the mod writing nothing) takes the chambered shell; the bolt feeds the next from the tube. With the
//     tube empty the bolt stays open (always: there is no magazine to be out).
//   - A shell pushed up through the loading port until the latch clicks: one more in the tube, from the spare ones. Refused when the tube is full.
//     Allowed with the bolt home or open. It chambers nothing.
//   - A shell dropped into the ejection port: only with the bolt held open, the chamber empty and no shell there already. It lies on the carrier
//     and isn't counted yet: to the game it's still a spare one (so nothing has to be undone if it never gets chambered), and the hands can't take
//     it from the belly a second time (Belly).
//   - The bolt sent home off its hold (the release button, or a tug of the handle): the port's shell is chambered (now it leaves the spare ones);
//     with none there, one from the tube; with none there either, the bolt goes home on an empty chamber.
//   - The handle drawn fully back from home and let go: a live shell in the chamber is thrown out, lost to the count (it falls as a loose shell);
//     the next is fed from the tube, or with the tube empty the bolt stays open. FORGIVING, not the real Benelli M4: on the real gun a shell
//     leaves the tube only when the hammer falls or the cartridge drop lever is pressed, so racking a cocked gun throws the chambered shell out
//     and closes on an empty chamber. Here racking always feeds, as most players expect of a shotgun (the owner chose forgiving rules for the
//     pistol's slide too); "tube then release" (the bolt sent home off its hold feeding from the tube) is forgiving in the same way.
//   - The trigger: fires with a live shell chambered, the bolt home and the handle not held. Bolt open, or the handle held: nothing (out of
//     battery, no click). Home on an empty chamber: the first pull clicks (the hammer falls), later ones do nothing until the bolt is worked. The
//     trigger never sends the bolt home (that is the pistol's own rule).
//   - The game's own reload, asked for (X only: never a swap of the game's): the hidden round is taken out of the game's count while it runs and
//     the tube has room, so the game fills to its own size and plays its empty reload at 0 (a full tube: the round stays in, the game sees a full
//     gun and starts nothing); each shell its count rises by is one shell (the asking begun on an empty gun: the first into the chamber, the
//     bolt home; otherwise into the tube); the hidden round goes back on top when it's over. A shot ends the asking at once (the hidden round
//     back in that same look), so a gun fired dry after X is never reloaded by the game by itself.
//   - The game's own reload, not asked for (or shells of it the tube has no room for): left alone while its count still rises (nothing is
//     written into a reload that runs: a game that looks at its count each loop might never finish), then given back to the spare ones once,
//     when the count has stopped rising for kRisingMs.
//   - The mouse passes no trigger's stop: a shot the game makes with nothing chambered takes a shell of the tube's (the gun's own: nothing comes
//     from the belly by itself); with none in the gun it is the hidden round's, put back from the spare ones, a few times.
//   - The 870's pump, later: the same tube, port and count. Its own pieces to add beside these: a shot that leaves the fired hull in the chamber
//     and feeds nothing (Chamber::kSpent is here for it), the fore-end back (whatever is chambered thrown out) and forward (the port's shell, else
//     one from the tube), the port load allowed with the fore-end back in place of the bolt held open, and a pull that asks for the pump. Nothing
//     below changes for it.

namespace shotgunammo {

enum class Chamber : int {
    kNone = 0,   // empty
    kLive = 1,   // a shell to fire
    kSpent = 2,  // a fired hull not yet thrown out (the pump's, later: the M1014 throws its own out as it fires)
};

struct State {
    int tube = 0;                      // the shells in the tube
    Chamber chamber = Chamber::kNone;  // what the chamber holds
    bool locked = false;               // the bolt held open (after the last shell, or drawn back on an empty tube)
    bool port = false;                 // a shell lying in the ejection port, on the carrier (only with the bolt held open and the chamber empty)
    bool down = false;                 // the hammer down on an empty chamber (a click made): no second click until the bolt is worked
};

// The shells in the gun as the game's count has them: the tube's and a live one chambered. The port's isn't in it yet.
inline int Total(const State& s) {
    return (s.chamber == Chamber::kLive ? 1 : 0) + (s.tube > 0 ? s.tube : 0);
}

// The shotgun as a count of the game's leaves it (a new gun, a checkpoint, the train's scripted pickup, the forced kit): a shell chambered if it
// has any, the rest in the tube; none at all, the bolt held open.
inline State FromCount(int total) {
    State s;
    s.chamber = total >= 1 ? Chamber::kLive : Chamber::kNone;
    s.tube = total >= 1 ? total - 1 : 0;
    s.locked = total < 1;
    return s;
}

// The mod's count gone astray from the game's (never by the rules below; a count something else wrote, or the spare shells short of what the
// hands loaded): the gun as that count has it, changed as little as can be: the tube's shells first; the chambered one only when the count has
// none at all (the bolt then held open, as after the last shot).
inline void Adopt(State& s, int total) {
    if (total < 0)
        total = 0;
    const int live = s.chamber == Chamber::kLive ? 1 : 0;
    if (total >= live) {
        s.tube = total - live;
        return;
    }
    s.tube = 0;
    s.chamber = Chamber::kNone;
    s.locked = true;
    s.down = false;
}

// Shots: each takes the chambered shell and feeds the next from the tube. How many it had shells for. The last one: the bolt held open.
inline int Fire(State& s, int shots) {
    int fired = 0;
    for (; fired < shots && s.chamber == Chamber::kLive; ++fired) {
        s.chamber = Chamber::kNone;
        s.down = false;  // (the bolt's stroke cocks it)
        if (s.tube > 0) {
            --s.tube;
            s.chamber = Chamber::kLive;
        } else {
            s.locked = true;
        }
    }
    return fired;
}

// ---- What the hands do ----
// belly, below: the shells a hand can still take from the belly (Belly: the game's spare ones, less what's in the gun the game hasn't been told
// of, less the one in the port). A shell goes into the gun only if there is one.
enum class Load : int {
    kIn = 0,        // in: the tube one more, or the shell lying in the port
    kFull = 1,      // the tube holds no more (the shell stops at the lifter)
    kNoShell = 2,   // no shell left at the belly for it
    kBoltHome = 3,  // the port: the bolt isn't held open
    kTaken = 4,     // the port: a shell lies there already, or the chamber holds something
};

// A shell pushed up through the loading port to the latch. It chambers nothing; the bolt may be home or open.
inline Load LoadTube(State& s, int capacity, int belly) {
    if (s.tube >= capacity)
        return Load::kFull;
    if (belly < 1)
        return Load::kNoShell;
    ++s.tube;
    return Load::kIn;
}

// A shell dropped into the ejection port. Not counted until the bolt takes it (BoltHome).
inline Load PortLoad(State& s, int belly) {
    if (!s.locked)
        return Load::kBoltHome;
    if (s.port || s.chamber != Chamber::kNone)
        return Load::kTaken;
    if (belly < 1)
        return Load::kNoShell;
    s.port = true;
    return Load::kIn;
}

// May a hand take a shell from the belly now: inHand, the shells the hands hold already (not in the gun yet).
enum class Take : int {
    kYes = 0,
    kNoShells = 1,  // none left at the belly
    kFull = 2,      // the tube is full, and the port takes none (the bolt home, a shell there, or one chambered)
};
inline Take CanTake(const State& s, int capacity, int belly, int inHand) {
    if (belly - (inHand > 0 ? inHand : 0) < 1)
        return Take::kNoShells;
    if (s.tube < capacity || (s.locked && !s.port && s.chamber == Chamber::kNone))
        return Take::kYes;
    return Take::kFull;
}

// The bolt worked: sent home off its hold, or racked by the handle.
struct Bolt {
    bool moved = false;      // it did something (BoltHome with the bolt home already: nothing)
    bool wasLocked = false;  // off its hold
    bool threw = false;      // a live shell thrown out of the chamber: lost to the count (the caller's `lost` one more; it falls as a loose shell)
    bool threwHull = false;  // a fired hull thrown out (the pump's, later): no shell of the count
    bool fromPort = false;   // the port's shell chambered (it leaves the spare ones now)
    bool fromTube = false;   // one from the tube chambered
    bool locked = false;     // the bolt held open after it
};

// The release button pressed, or the handle tugged off its hold: only with the bolt held open. (One from the tube with nothing in the port:
// forgiving, see Rack.)
inline Bolt BoltHome(State& s) {
    Bolt b;
    if (!s.locked)
        return b;
    b.moved = true;
    b.wasLocked = true;
    s.locked = false;
    s.down = false;
    if (s.port) {
        s.port = false;
        s.chamber = Chamber::kLive;
        b.fromPort = true;
    } else if (s.tube > 0) {
        --s.tube;
        s.chamber = Chamber::kLive;
        b.fromTube = true;
    }
    return b;
}

// The handle drawn fully back and let go. From home: what's chambered is thrown out, the next fed from the tube; the tube empty, the bolt stays
// open. Held open already: let go it goes home, as BoltHome.
// Forgiving, not the real Benelli M4: there a shell leaves the tube only when the hammer falls or the cartridge drop lever is pressed, so a
// rack of the cocked gun closes on an empty chamber. Here it always feeds (and so does BoltHome's "tube then release").
inline Bolt Rack(State& s) {
    if (s.locked)
        return BoltHome(s);
    Bolt b;
    b.moved = true;
    b.threw = s.chamber == Chamber::kLive;
    b.threwHull = s.chamber == Chamber::kSpent;
    s.chamber = Chamber::kNone;
    s.down = false;
    if (s.tube > 0) {
        --s.tube;
        s.chamber = Chamber::kLive;
        b.fromTube = true;
    } else {
        s.locked = true;
    }
    b.locked = s.locked;
    return b;
}

// The right trigger pulled. handleHeld: a hand holds the bolt handle back (out of battery).
enum class Pull : int {
    kFires = 0,    // a live shell chambered, the bolt home: the game's fire button
    kNothing = 1,  // the bolt held open, the handle held, or the hammer down already: no shot and no click
    kClick = 2,    // home on an empty chamber, the hammer cocked: it falls (Pulled marks it down)
};
inline Pull OnPull(const State& s, bool handleHeld) {
    if (s.locked || handleHeld)
        return Pull::kNothing;
    if (s.chamber == Chamber::kLive)
        return Pull::kFires;
    return s.down ? Pull::kNothing : Pull::kClick;
}
// ... and the pull made: a click leaves the hammer down. (A shot is counted when the game's count shows it: See.)
inline Pull Pulled(State& s, bool handleHeld) {
    const Pull pull = OnPull(s, handleHeld);
    if (pull == Pull::kClick)
        s.down = true;
    return pull;
}

// ---- The game's own reload (X held): its count rises a shell a loop ----
// X let go this long ago still asks for it, and so does each shell of it for this long after: the reload runs on after the key (its start before
// the first shell 0.57 s, 1.93 s from empty; a loop 0.50 s; its end 0.80 s: the catalog's waves, to be confirmed in the game).
constexpr unsigned long long kAskedMs = 3000;
constexpr unsigned long long kRisingMs = 1500;
// sinceX: how long ago X was let go (0 while held; a magazine-less gun has no "seated by hand since" to cancel it). sinceShell: how long ago the
// game's count last rose by a shell that was kept. A shot ends the asking (CountOut.shot): the caller then forgets both (as never), and X held
// through the shot asks again only by a fresh press. Nothing else asks: not a swap of the game's (the pistol's rule, not this gun's: with the
// hidden round out the game would reload an empty shotgun by itself).
inline bool Asked(unsigned long long sinceX, unsigned long long sinceShell) {
    return sinceX < kAskedMs || sinceShell < kRisingMs;
}

// The game's own reload put shells into the gun. fromEmpty: the asking began on an empty gun (the game's count at 0: its empty reload: a shell
// into the port, the bolt released, then the tube; judged once for the whole reload, not per shell: a shell the hands pushed up the tube
// meanwhile changes nothing of what the game plays): the first shell that finds no live one chambered goes into the chamber, the bolt home; a
// shell the hands left in the port goes back to the belly (it never left the spare ones). Otherwise all into the tube, the bolt left as it is.
// The tube takes what it has room for.
struct GameLoad {
    int taken = 0;          // shells now in the gun
    int refused = 0;        // shells the tube had no room for (See gives them back to the spare ones)
    bool chambered = false; // the first went into the chamber, the bolt home
    bool portBack = false;  // the hands' shell in the port: back to the belly
};
inline GameLoad GameLoads(State& s, int shells, bool fromEmpty, int capacity) {
    GameLoad g;
    if (shells <= 0)
        return g;
    if (fromEmpty && s.chamber != Chamber::kLive) {
        g.portBack = s.port;
        s.port = false;
        s.chamber = Chamber::kLive;
        s.locked = false;
        s.down = false;
        g.chambered = true;
        g.taken = 1;
    }
    const int room = capacity > s.tube ? capacity - s.tube : 0;
    const int rest = shells - g.taken;
    const int in = rest < room ? rest : room;
    s.tube += in;
    g.taken += in;
    g.refused = shells - g.taken;
    return g;
}

// ---- The game's count of it, a frame: what the game did, and what the mod writes. ----
constexpr int kMostPutBacks = 3;  // the hidden round put back this many times since its count was last 2 or more (pickrules.h's, the rifles')
struct CountIn {
    int last = -1;       // the game's count when the mod last looked (after what it wrote then); -1 not looked yet
    int raw = 0;         // ... now
    int spare = 0;       // its spare shells now
    int hidden = 0;      // the hidden round: 1 in the game's count (or owed to it: fired by the mouse, the count at 0), 0 out of it (the gun just
                         // met; taken out for the game's own reload)
    bool asked = false;  // the game's own reload asked for: X only (Asked). Nothing else: no swap of the game's
    bool fromEmpty = false;  // ... and that asking began on an empty gun (the caller's latch: set as the asking begins with Total 0, kept until
                             // the asking is over and the hidden round back in): the game plays its empty reload (GameLoads)
    unsigned long long sinceRise = ~0ULL;  // how long ago a look last saw the game's count rise by itself (CountOut.rose), ms; never: ~0
    int putBacks = 0;    // the hidden round's put-backs since the count was last 2 or more
    int lost = 0;        // live shells the bolt threw out since the count was last written (they go to no one)
    int capacity = 4;    // the shells the tube holds
};
struct CountOut {
    int fired = 0;             // shots the gun had shells for
    int firedTube = 0;         // ... and shots the game made with nothing chambered (the mouse): each took a shell of the tube's
    bool hiddenFired = false;  // ... and one more: the hidden round
    bool shot = false;         // the game's count fell (a shot): the asking is over, whatever `asked` said (the hidden round is in after this
                               // look): the caller forgets its X and shell times (Asked), a fresh press of X asks again
    bool lockedNow = false;    // those shots left the bolt held open
    bool rose = false;         // the game's count rose by itself (its own reload): the caller notes the time (CountIn.sinceRise)
    int gameLoaded = 0;        // the game's own reload, asked for: shells of it now in the gun
    int refused = 0;           // ... and shells of it the tube had no room for: not kept (given back later: back)
    bool gameChambered = false;  // ... its first went into the chamber (its empty reload), the bolt home
    bool hiddenPaid = false;   // ... its first was the hidden round's (fired before, the count at 0): no shell of the gun's
    bool undone = false;       // the game's own reload, not asked for: this rise of its count isn't kept (given back later: back)
    bool waiting = false;      // shells of the game's that aren't kept are in its count still, and it rose lately: nothing written this look
                               // (hidden and lost as they were), the hands' moves wait with it
    int back = 0;              // shells of the game's (not asked for, or no room) given back to the spare ones now, all at once: its count has
                               // stopped rising for kRisingMs
    bool putBack = false;      // the hidden round put back from the spare ones
    bool hiddenOut = false;    // the hidden round taken out of the game's count: its own reload is asked for
    bool hiddenIn = false;     // the hidden round (back) on top of the game's count: the gun just met, or the game's reload over
    bool settled = false;      // what the hands did (shells loaded, one thrown out) written into the game's count (not: the hidden round in
                               // or out alone, the game's shells given back alone)
    bool adopted = false;      // the two counts apart and the spare shells short of the difference: the gun holds as many fewer
    bool portBack = false;     // the shell in the port is no more (the game's empty reload, or no spare shell left behind it): back to the belly
    bool write = false;        // loaded and spare below are to be written
    int loaded = 0, spare = 0;
    int hidden = 0, lost = 0;  // the hidden round and the shells thrown out still to settle after this look: the caller's to keep (CountIn's
                               // next time) once `write` is made, or with nothing to write
};

// The shells a hand can still take from the belly: the game's spare ones, less what the gun holds that the game's count hasn't been told of yet
// (shells loaded by hand, the hidden round owed), less the one lying in the port.
inline int Belly(const State& s, const CountIn& in) {
    int owed = (in.hidden > 0 ? 1 : 0) + Total(s) + (in.lost > 0 ? in.lost : 0) - in.raw;
    if (owed < 0)
        owed = 0;
    const int belly = in.spare - owed - (s.port ? 1 : 0);
    return belly > 0 ? belly : 0;
}

namespace detail {
inline CountOut See(State& s, const CountIn& in) {
    CountOut out;
    out.loaded = in.raw, out.spare = in.spare;
    const int hid = in.hidden > 0 ? 1 : 0;
    const int lost = in.lost > 0 ? in.lost : 0;
    out.hidden = hid;
    out.lost = lost;
    if (in.raw < 0 || in.spare < 0)
        return out;
    bool asked = in.asked;
    if (in.last >= 0 && in.raw < in.last) {
        // A shot ends the asking: the hidden round goes back in below, so shots that follow stop at the mod's empty chamber with the game's
        // count at 1, and the game starts no reload of its own.
        out.shot = true;
        asked = false;
        const bool was = s.locked;
        int shots = in.last - in.raw;
        out.fired = Fire(s, shots);
        // Shots the gun had nothing chambered for (the mouse passes no trigger's stop): each a shell of the tube's while it has any (the gun's
        // own, never one from the belly: nothing loads by itself), the bolt left as it is; then the hidden round (the count at 0: above 0 they
        // were shells of the game's own reload, not kept yet).
        shots -= out.fired;
        out.firedTube = shots < s.tube ? shots : s.tube;
        s.tube -= out.firedTube;
        shots -= out.firedTube;
        out.hiddenFired = hid && shots > 0 && in.raw == 0;
        out.lockedNow = s.locked && !was;
    } else if (in.last >= 0 && in.raw > in.last) {
        out.rose = true;
        // The game's own reload, asked for (or its last shells as the asking ends, the hidden round still out): each shell its count rose by is a
        // shell. From 0 with the hidden round owed (the mouse fired it): the first is that round's, paid by the game from the spare ones.
        if (asked || !hid) {
            int shells = in.raw - in.last;
            if (in.last == 0 && hid) {
                --shells;
                out.hiddenPaid = true;
            }
            const GameLoad g = GameLoads(s, shells, in.fromEmpty || in.last == 0, in.capacity);
            out.gameLoaded = g.taken;
            out.refused = g.refused;
            out.gameChambered = g.chambered;
            out.portBack = g.portBack;
        } else {
            out.undone = true;
        }
    }
    // The hidden round in the game's count: out while its own reload is asked for and the tube has room (a full tube: left in, so the game sees
    // a full gun and starts nothing).
    const bool reload = asked && s.tube < in.capacity;
    const int want = reload ? 0 : 1;
    // The hidden round fired with nothing else in the gun: back from the spare ones, unless the game's reload is wanted (its count at 0 lets its
    // empty reload play; the first shell of it pays for the round), none are left, or it has been put back too often (the game then reloads by
    // itself, and that is given back). (With shells thrown out still in the count's tally, the game fired those too: settled below, all at once.)
    if (hid && in.raw == 0 && Total(s) == 0 && lost == 0) {
        if (reload || in.spare < 1 || in.putBacks >= kMostPutBacks)
            return out;
        out.putBack = true;
        out.write = true;
        out.loaded = 1;
        out.spare = in.spare - 1;
        return out;
    }
    // Shells of the game's own in its count that aren't kept (a reload not asked for; shells the tube has no room for): nothing is written
    // while its count still rises (a count written back down each loop may keep the game's reload going for ever); once it has stopped for
    // kRisingMs they go back to the spare ones below, all at once.
    const int over = in.raw - (hid + Total(s)) - lost;
    if (over > 0 && (out.rose || in.sinceRise < kRisingMs)) {
        out.waiting = true;
        return out;
    }
    // The game's count to the mod's, the difference to or from the spare ones; the shells the bolt threw out to no one. The hidden round goes in
    // and out for nothing.
    out.lost = 0;
    int spare = in.spare + over;
    if (spare < 0) {
        // The spare ones short of what the gun is said to hold: it holds as many fewer. Not even that many in it: the gun as the game's count has
        // it, nothing written.
        out.adopted = true;
        const int total = Total(s) + spare;
        if (total < 0) {
            Adopt(s, in.raw > hid ? in.raw - hid : 0);
            return out;
        }
        Adopt(s, total);
        spare = 0;
    }
    const int loaded = want + Total(s);
    out.hidden = want;
    out.hiddenOut = hid && !want;
    out.hiddenIn = !hid && want;
    if (loaded == in.raw && spare == in.spare)
        return out;
    out.back = over > 0 ? over : 0;
    out.settled = over < 0 || lost > 0;
    out.write = true;
    out.loaded = loaded;
    out.spare = spare;
    return out;
}
}  // namespace detail

// One look at the game's count. What it says to write (loaded, spare) the caller writes; hidden and lost are the caller's to keep (CountOut).
inline CountOut See(State& s, const CountIn& in) {
    CountOut out = detail::See(s, in);
    // A shell in the port with no spare one behind it (the game took it: its own reload, the hidden round's put-back, or that round still owed):
    // it is no more.
    const int owed = out.hidden && out.loaded == 0 && Total(s) == 0 ? 1 : 0;
    if (s.port && out.spare - owed < 1) {
        s.port = false;
        out.portBack = true;
    }
    return out;
}

}  // namespace shotgunammo

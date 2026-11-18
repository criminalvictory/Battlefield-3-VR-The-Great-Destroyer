// The train pistol's rounds with its magazine by hand (src/pistolammo.h, header only) without the game: a stand-in for the game's count (it fires
// while its count is above 0, reloads by itself at 0 with spare rounds, reloads when asked) and the mod's look at it each frame, as ammo.cpp's
// StepPistol runs it (the hidden round on top of the game's count, what See says written unless a take of the game's holds the write off). Each
// check prints PASS or FAIL, then the totals; the exit code is the number failed.
//   T1  met full (16 loaded, 144 spare): a round chambered and 15 in the magazine, the game's count 17; met empty: the slide on its stop
//   T2  fired to empty: each shot counts the magazine down, the 16th leaves the slide on its stop and the chamber empty; the game's count stays
//       at 1 (its hidden round) and it never reloads by itself; the trigger then fires nothing
//   T3  empty, the magazine out (none back to the spare ones, the slide stays open), a fresh one seated: 15 from the spare ones, nothing chambered;
//       the trigger's first pull only sends the slide home (it chambers: 14 and one), the next fires; a tug by hand does the same
//   T4  the magazine out with rounds in it: they go back to the spare ones, the chambered round stays and still fires; fired, the slide goes home
//       (no magazine: no stop); a fresh magazine in: the trigger does nothing with the slide by hand, a rack chambers; with "slide 0" the
//       trigger cycles it
//   T5  a tactical reload of a full pistol: 16 again (the game's own capacity), the spare ones as they were
//   T6  few spare rounds: a magazine takes what there is; none: an empty magazine, the stop takes the slide again when it's let go
//   T7  racked by hand with a round chambered: thrown out, lost (in the gun and the spare ones one fewer), the next fed; an empty magazine in:
//       the stop holds it; no magazine: home on an empty chamber
//   T8  the game's own reload: not asked for, undone from any count, every round back to the spare ones; asked for (X held), kept: a magazine in,
//       a round chambered, the hidden round back on top
//   T9  the hidden round fired (the mouse passes no stop): put back from the spare ones three times a magazine, then left for the game's reload,
//       which is undone; with no spare rounds left alone
//   T10 a write held off a frame (a take of the game's open) is made the next: a magazine seated, the count unwritten, a shot between
//   T11 the counts apart with the spare rounds short: the pistol as the game's count has it
//   T12 6000 random sessions (shots, the mouse's too, magazines out and in, racks, pulls, the game's reloads asked for or not, writes held off): the
//       game's count is the mod's and its hidden round after every settled frame, no round is made or lost but those thrown out and the hidden
//       round's own, the magazine never over 15, nothing below 0, and the game never ends a frame reloaded by itself
// Usage: pistolammotest

#include <cstdio>
#include <cstdlib>

#include "../src/pistolammo.h"

namespace {

using namespace pistolammo;

int g_failed = 0, g_counted = 0;

void Check(bool ok, const char* row, const char* what) {
    ++g_counted;
    if (!ok)
        ++g_failed;
    std::printf("%s %s %s\n", ok ? "PASS" : "FAIL", row, what);
}

constexpr int kMagazine = 15;  // the M9's: Ammo.MagazineCapacity 16 less the chambered round

// The game's side.
struct Game {
    int loaded = 16, spare = 144;
    int fired = 0;
    bool Shoot() {
        if (loaded < 1)
            return false;
        --loaded;
        ++fired;
        return true;
    }
    // Its own reload: a magazine, and the chambered round if it wasn't empty.
    bool Reload() {
        const int full = kMagazine + (loaded > 0 ? 1 : 0);
        const int take = full - loaded < spare ? full - loaded : spare;
        if (take <= 0)
            return false;
        loaded += take;
        spare -= take;
        return true;
    }
};

// The mod's side, as ammo.cpp keeps it.
struct Mod {
    State s;
    int last = -1, hidden = 0, putBacks = 0, lost = 0;
    int charged = 0;  // (the test's own tally: rounds thrown out by the slide that left the counts)
};

void Meet(Mod& m, Game& g) {
    m = Mod{};
    m.s = FromCount(g.loaded);
    g.loaded += 1;
    m.hidden = 1;
    m.last = g.loaded;
}

CountOut Step(Mod& m, Game& g, bool asked = false, bool canWrite = true) {
    CountIn in;
    in.last = m.last, in.raw = g.loaded, in.spare = g.spare, in.hidden = m.hidden, in.asked = asked, in.putBacks = m.putBacks, in.lost = m.lost;
    const CountOut out = See(m.s, in);
    if (out.write && canWrite) {
        if (!out.kept)
            m.charged += g.spare + (g.loaded - out.loaded) - out.spare;
        g.loaded = out.loaded;
        g.spare = out.spare;
        m.lost = 0;
        if (out.putBack)
            ++m.putBacks;
    }
    m.last = g.loaded;
    if (g.loaded >= 2)
        m.putBacks = 0;
    return out;
}

// The hands, as ammo.cpp's PistolEject, PistolInsert and PistolRack: a look before, the hands' move, a look after.
int Out(Mod& m, Game& g) {
    Step(m, g);
    const int back = Eject(m.s);
    Step(m, g);
    return back;
}
int In(Mod& m, Game& g) {
    Step(m, g);
    const int take = Insert(m.s, kMagazine, g.spare);
    Step(m, g);
    return take;
}
Racked Pulled(Mod& m, Game& g) {
    Step(m, g);
    const Racked r = Rack(m.s);
    if (r.threw)
        ++m.lost;
    Step(m, g);
    return r;
}
// The right trigger through the mod's stop: a shot of the game's if a round is chambered.
bool Trigger(Mod& m, Game& g) {
    if (OnPull(m.s, true) != Pull::kFires)
        return false;
    g.Shoot();
    Step(m, g);
    return true;
}

void T1() {
    Game g;
    Mod m;
    Meet(m, g);
    Check(m.s.chamber && m.s.magIn && m.s.mag == 15 && !m.s.locked && Total(m.s) == 16 && g.loaded == 17 && g.spare == 144, "T1",
          "met with 16 loaded, 144 spare: one chambered, 15 in the magazine; the game's count 17 (the hidden round), the spare ones untouched");
    Check(Step(m, g).write == false && g.loaded == 17, "T1", "a frame with nothing done: nothing written");
    Game e;
    e.loaded = 0;
    Meet(m, e);
    Check(!m.s.chamber && m.s.mag == 0 && m.s.locked && Total(m.s) == 0 && e.loaded == 1 && OnPull(m.s, true) == Pull::kClick, "T1",
          "met empty: nothing chambered, the slide on its stop, the game's count 1; the trigger fires nothing");
}

void T2() {
    Game g;
    Mod m;
    Meet(m, g);
    bool counted = true, early = false;
    for (int shot = 1; shot <= 16; ++shot) {
        const bool fired = Trigger(m, g);
        counted = counted && fired && Total(m.s) == 16 - shot && g.loaded == 1 + Total(m.s);
        if (shot < 16 && (m.s.locked || !m.s.chamber))
            early = true;
    }
    Check(counted && !early, "T2", "16 shots: each counts down (the chambered round fired, the next fed), the game's count one above all the way");
    Check(!m.s.chamber && m.s.mag == 0 && m.s.magIn && m.s.locked && g.loaded == 1 && g.spare == 144 && g.fired == 16, "T2",
          "the 16th: the chamber empty, the slide on its stop; the game's count 1, so it doesn't reload by itself; 144 spare");
    Check(!Trigger(m, g) && OnPull(m.s, true) == Pull::kClick && OnPull(m.s, false) == Pull::kClick && g.fired == 16, "T2",
          "empty: the trigger fires nothing, and does nothing to the slide (an empty magazine in)");
    // Two shots between two looks (a frame dropped): both counted.
    Game h;
    Mod n;
    Meet(n, h);
    h.Shoot(), h.Shoot();
    const CountOut two = Step(n, h);
    Check(two.fired == 2 && !two.hiddenFired && Total(n.s) == 14 && n.s.chamber && n.s.mag == 13, "T2", "two shots between two looks: both counted");
}

void T3() {
    Game g;
    Mod m;
    Meet(m, g);
    for (int i = 0; i < 16; ++i)
        Trigger(m, g);
    const int back = Out(m, g);
    Check(back == 0 && !m.s.magIn && m.s.locked && !m.s.chamber && g.loaded == 1 && g.spare == 144 && OnPull(m.s, true) == Pull::kClick, "T3",
          "empty, the magazine out: none back to the spare ones, the slide still on its stop, the trigger fires nothing");
    const int take = In(m, g);
    Check(take == 15 && m.s.magIn && m.s.mag == 15 && !m.s.chamber && m.s.locked && g.loaded == 16 && g.spare == 129, "T3",
          "a fresh magazine seated: 15 from the spare ones (129 left), nothing chambered yet, the slide still on its stop");
    Check(OnPull(m.s, true) == Pull::kSendsHome && OnPull(m.s, false) == Pull::kSendsHome && !Trigger(m, g) && g.fired == 16, "T3",
          "the trigger's first pull fires nothing: it only sends the slide home");
    const Racked r = Pulled(m, g);
    Check(r.wasLocked && !r.threw && r.chambered && !r.locked && m.s.chamber && m.s.mag == 14 && !m.s.locked && g.loaded == 16 && g.spare == 129, "T3",
          "the slide home off its stop: a round chambered, 14 in the magazine, no round made or lost");
    Check(OnPull(m.s, true) == Pull::kFires && Trigger(m, g) && Total(m.s) == 14 && g.fired == 17, "T3", "the next pull fires");
}

void T4() {
    Game g;
    Mod m;
    Meet(m, g);
    for (int i = 0; i < 8; ++i)
        Trigger(m, g);
    const int back = Out(m, g);
    Check(back == 7 && !m.s.magIn && m.s.chamber && !m.s.locked && Total(m.s) == 1 && g.loaded == 2 && g.spare == 151, "T4",
          "the magazine out with 7 in it and one chambered: 7 back to the spare ones (151), the chambered round stays, the game's count 2");
    Check(OnPull(m.s, true) == Pull::kFires && Trigger(m, g) && !m.s.chamber && !m.s.locked && g.loaded == 1 && g.fired == 9, "T4",
          "the chambered round fires with the magazine out; then the chamber is empty and the slide home (no magazine: no stop)");
    Check(OnPull(m.s, true) == Pull::kClick && OnPull(m.s, false) == Pull::kClick && !Trigger(m, g), "T4", "then the trigger fires nothing");
    const int take = In(m, g);
    Check(take == 15 && !m.s.chamber && !m.s.locked && m.s.mag == 15 && g.loaded == 16 && g.spare == 136, "T4",
          "a fresh magazine seated: 15 in it, the chamber still empty, the slide home");
    Check(OnPull(m.s, true) == Pull::kClick && OnPull(m.s, false) == Pull::kRacks, "T4",
          "the trigger: nothing with the slide worked by hand (rack it); with \"slide 0\" it cycles the slide");
    const Racked r = Pulled(m, g);
    Check(!r.wasLocked && !r.threw && r.chambered && m.s.chamber && m.s.mag == 14 && g.loaded == 16 && g.spare == 136 && Trigger(m, g), "T4",
          "racked by hand: a round chambered, 14 in the magazine; the pistol fires");
}

void T5() {
    Game g;
    Mod m;
    Meet(m, g);
    const int back = Out(m, g);
    const int take = In(m, g);
    Check(back == 15 && take == 15 && Total(m.s) == 16 && m.s.chamber && m.s.mag == 15 && g.loaded == 17 && g.spare == 144, "T5",
          "a full pistol's magazine out and a fresh one in: 16 again (15 and the chambered round, the game's own capacity), 144 spare as before");
    Check(Eject(m.s) == 15 && Eject(m.s) == -1 && Insert(m.s, kMagazine, 144) == 15 && Insert(m.s, kMagazine, 144) == -1, "T5",
          "a magazine out twice, or in twice: the second is nothing");
}

void T6() {
    Game g;
    g.loaded = 3, g.spare = 9;
    Mod m;
    Meet(m, g);
    Out(m, g);  // 2 back: 11 spare
    const int take = In(m, g);
    Check(take == 11 && m.s.mag == 11 && g.spare == 0 && g.loaded == 13, "T6", "11 spare rounds: the magazine takes them all, none left");
    for (int i = 0; i < 12; ++i)
        Trigger(m, g);
    Out(m, g);
    const int none = In(m, g);
    Check(none == 0 && m.s.magIn && m.s.mag == 0 && m.s.locked && g.loaded == 1 && g.spare == 0 && OnPull(m.s, true) == Pull::kClick, "T6",
          "no spare rounds: an empty magazine in, the slide on its stop, the trigger fires nothing");
    const Racked r = Pulled(m, g);
    Check(r.wasLocked && !r.chambered && r.locked && m.s.locked && g.loaded == 1, "T6", "tugged off its stop on an empty magazine: the stop takes it again");
}

void T7() {
    Game g;
    Mod m;
    Meet(m, g);
    const Racked r = Pulled(m, g);
    Check(r.threw && r.chambered && !r.locked && Total(m.s) == 15 && m.s.mag == 14 && g.loaded == 16 && g.spare == 144 && m.lost == 0 &&
              m.charged == 1,
          "T7",
          "racked with a round chambered: it's thrown out (lost: 15 in the pistol, the spare ones as they were), the next fed");
    Game e;
    e.loaded = 1, e.spare = 20;
    Meet(m, e);
    const Racked last = Pulled(m, e);
    Check(last.threw && !last.chambered && last.locked && m.s.locked && Total(m.s) == 0 && e.loaded == 1 && e.spare == 20, "T7",
          "racked with the last round chambered and an empty magazine in: thrown out, the stop holds the slide open");
    Game n;
    n.loaded = 5;
    Meet(m, n);
    Out(m, n);
    const Racked bare = Pulled(m, n);
    Check(bare.threw && !bare.chambered && !bare.locked && !m.s.locked && !m.s.chamber && n.loaded == 1 && n.spare == 148, "T7",
          "racked with no magazine in: the chambered round thrown out, home on an empty chamber");
}

void T8() {
    // Not asked for: from the stop (the game's count at its hidden round), from part way, with the magazine out.
    Game g;
    Mod m;
    Meet(m, g);
    for (int i = 0; i < 16; ++i)
        Trigger(m, g);
    g.Reload();  // 1 -> 16, 15 fewer spare
    CountOut out = Step(m, g);
    Check(out.undone && g.loaded == 1 && g.spare == 144 && Total(m.s) == 0 && m.s.locked, "T8",
          "the game's reload of the empty pistol, not asked for: undone, its 15 rounds back to the spare ones, the slide still on its stop");
    Game h;
    Mod n;
    Meet(n, h);
    for (int i = 0; i < 10; ++i)
        Trigger(n, h);
    h.Reload();
    out = Step(n, h);
    Check(out.undone && h.loaded == 7 && h.spare == 144 && Total(n.s) == 6, "T8", "... of a pistol with 6 left: undone, 6 still in it");
    Out(n, h);
    h.Reload();
    out = Step(n, h);
    Check(out.undone && h.loaded == 2 && h.spare == 149 && !n.s.magIn && Total(n.s) == 1, "T8", "... with the magazine out: undone, the magazine still out");
    // Asked for (X held).
    Game k;
    Mod x;
    Meet(x, k);
    for (int i = 0; i < 16; ++i)
        Trigger(x, k);
    Out(x, k);
    k.Reload();  // (the game's count 1: it takes 15)
    out = Step(x, k, true);
    Check(out.kept && x.s.magIn && x.s.chamber && !x.s.locked && Total(x.s) == 16 && k.loaded == 17 && k.spare == 129, "T8",
          "X held, the game's own reload: kept, a magazine in and a round chambered, the slide home, the hidden round back on top");
    Check(Trigger(x, k) && Total(x.s) == 15, "T8", "... and it fires");
}

void T9() {
    Game g;
    g.loaded = 2;
    Mod m;
    Meet(m, g);  // 2 in it, the game's count 3
    bool ok = true;
    // The mouse: no stop of the mod's. Two real shots, then the hidden round, put back three times.
    g.Shoot(), Step(m, g);
    g.Shoot(), Step(m, g);
    ok = ok && Total(m.s) == 0 && m.s.locked && g.loaded == 1;
    for (int i = 1; i <= 3; ++i) {
        g.Shoot();
        const CountOut out = Step(m, g);
        ok = ok && out.hiddenFired && out.putBack && g.loaded == 1 && g.spare == 144 - i && m.putBacks == i;
    }
    Check(ok, "T9", "the hidden round fired three times: put back each time from the spare ones (141 left), the game's count never left at 0");
    g.Shoot();
    CountOut out = Step(m, g);
    Check(out.hiddenFired && !out.putBack && !out.write && g.loaded == 0, "T9", "the fourth: not put back");
    g.Reload();  // by itself, from empty: 15
    out = Step(m, g);
    Check(out.undone && g.loaded == 1 && g.spare == 140 && Total(m.s) == 0 && m.s.locked, "T9",
          "the game's reload that follows: undone, the count back at the hidden round (one from the spare ones), the pistol still empty");
    Game e;
    e.loaded = 1, e.spare = 0;
    Meet(m, e);
    e.Shoot(), Step(m, e);
    e.Shoot();
    out = Step(m, e);
    Check(out.hiddenFired && !out.putBack && e.loaded == 0 && e.spare == 0 && !Step(m, e).write, "T9", "no spare rounds: left at 0, nothing written");
}

void T10() {
    Game g;
    Mod m;
    Meet(m, g);
    for (int i = 0; i < 16; ++i)
        Trigger(m, g);
    Eject(m.s);
    Step(m, g);
    Insert(m.s, kMagazine, g.spare);
    Rack(m.s);  // off the stop: 14 and one
    CountOut out = Step(m, g, false, false);  // (a take of the game's open: not written)
    Check(out.write && g.loaded == 1 && g.spare == 144 && Total(m.s) == 15, "T10", "a magazine seated while no count may be written: the game's count waits");
    out = Step(m, g);
    Check(out.settled && g.loaded == 16 && g.spare == 129, "T10", "the next frame: written, 15 from the spare ones");
    // A shot between the hands' move and the write.
    Game h;
    Mod n;
    Meet(n, h);
    Eject(n.s);  // (not yet written: the game still counts 17)
    h.Shoot();
    out = Step(n, h);
    Check(out.fired == 1 && !n.s.chamber && h.loaded == 1 && h.spare == 159 && h.loaded + h.spare + h.fired == 161, "T10",
          "a shot before the magazine's rounds were written back: counted, then written; no round made or lost");
}

void T11() {
    Game g;
    g.loaded = 4, g.spare = 0;
    Mod m;
    Meet(m, g);  // 4 in it, the game's count 5
    g.loaded = 2;  // (something else took rounds out, between two looks: read as two shots)
    Step(m, g);
    m.s = FromCount(9);  // (the mod's count astray: 9 where the game has 1 and its hidden round, no spare ones to make it so)
    const CountOut out = Step(m, g);
    Check(out.adopted && !out.write && Total(m.s) == 1 && m.s.chamber && g.loaded == 2 && g.spare == 0, "T11",
          "the mod's count above the game's with no spare rounds to take: the pistol as the game's count has it, nothing written");
    State s;
    s.magIn = false, s.chamber = true;
    Adopt(s, 0);
    Check(!s.chamber && !s.magIn && !s.locked, "T11", "Adopt: none, the magazine out: it stays out, the slide home");
    Adopt(s, 6);
    Check(s.chamber && s.magIn && s.mag == 5 && !s.locked, "T11", "Adopt: 6, the magazine out: one chambered, a magazine of 5 in");
}

void T12() {
    std::srand(20261118);
    int bad = 0, sessions = 0, events = 0;
    for (; sessions < 6000 && !bad; ++sessions) {
        Game g;
        g.loaded = std::rand() % 17;
        g.spare = std::rand() % 4 == 0 ? std::rand() % 20 : 30 + std::rand() % 150;
        const int rounds = g.loaded + g.spare;
        Mod m;
        Meet(m, g);
        int free = 1;       // rounds the mod made: the hidden round at the meeting ...
        int hiddenGone = 0;  // (the hidden round fired and not yet put back or undone)
        for (int e = 0; e < 120 && !bad; ++e, ++events) {
            const int what = std::rand() % 16;
            bool asked = false, canWrite = std::rand() % 10 != 0;
            if (what < 6) {
                Trigger(m, g);
            } else if (what == 6) {
                g.Shoot();  // the mouse: no stop
            } else if (what == 7) {
                if (m.s.magIn) {
                    Eject(m.s);
                } else {
                    Step(m, g);
                    Insert(m.s, kMagazine, g.spare);
                }
            } else if (what == 8) {
                Step(m, g);
                if (m.s.magIn)
                    Eject(m.s);
                Step(m, g);
                Insert(m.s, kMagazine, g.spare);
            } else if (what == 9) {
                const Racked r = Rack(m.s);
                if (r.threw)
                    ++m.lost;
            } else if (what == 10) {
                const Pull pull = OnPull(m.s, std::rand() % 2 == 0);
                if (pull == Pull::kSendsHome || pull == Pull::kRacks)
                    Rack(m.s);  // (nothing chambered: nothing thrown out)
            } else if (what == 11) {
                // X held: the game's own reload, asked for. It lands on the game's count as it is (settled first, as frames pass before it does).
                Step(m, g);
                if (g.Reload()) {
                    asked = true;
                    // (As the rifles': the hidden round the game's reload counted as its chambered one goes back on top, free.)
                    ++free;
                }
                canWrite = true;
            } else if (what == 12 && g.loaded == 0) {
                g.Reload();  // by itself
            }
            const CountOut out = Step(m, g, asked, canWrite);
            hiddenGone = g.loaded == 0 && Total(m.s) == 0 ? 1 : 0;
            // Settled (written this frame, or nothing to write): the game's count is the mod's and its hidden round; nothing below 0; the magazine
            // within its size.
            const bool settled = canWrite || !out.write;
            if (settled && !hiddenGone && g.loaded != 1 + Total(m.s))
                ++bad;
            if (settled && g.loaded > 1 + kMagazine + 1)
                ++bad;
            if (g.loaded < 0 || g.spare < 0 || m.s.mag < 0 || m.s.mag > kMagazine || (!m.s.magIn && m.s.mag != 0))
                ++bad;
            if (m.s.locked && m.s.chamber)
                ++bad;
            // No round made or lost: what's in the gun, spare, fired and thrown out is what there was, and the mod's free rounds.
            if (settled && g.loaded + g.spare + g.fired + m.charged != rounds + free)
                ++bad;
        }
    }
    char what[200];
    std::snprintf(what, sizeof(what),
                  "%d random sessions, %d moves: the counts agree after every settled frame, no round made or lost, nothing out of range (%d wrong)",
                  sessions, events, bad);
    Check(bad == 0 && sessions == 6000, "T12", what);
}

}  // namespace

int main() {
    T1();
    T2();
    T3();
    T4();
    T5();
    T6();
    T7();
    T8();
    T9();
    T10();
    T11();
    T12();
    std::printf("%d of %d checks failed\n", g_failed, g_counted);
    return g_failed;
}

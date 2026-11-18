// The asking for the game's own reload of a tube shotgun (src/tubeask.h, header only) as ammo.cpp's StepTube keeps it around each look of
// src/shotgunammo.h, without the game: a stand-in for the game's count (it fires while its count is above 0; its own reload, once begun, loads a
// shell a loop while its count is below its size, begins by itself at 0, and is ended by a shot: tools/shotgunammotest.cpp's) and a clock in
// milliseconds. Each check prints PASS or FAIL, then the totals; the exit code is the number failed.
//   A1  met full: the hidden round goes on top at the first look; nothing asked
//   A2  the key let go still asks for 3 s (kAskedMs), not after; never pressed never asks
//   A3  X held with room in the tube: the hidden round out, each shell of the game's kept, each keeps the asking 1.5 s more; then the hidden round
//       back on top, the gun full, no shell made
//   A3b an empty gun, X tapped for 100 ms, the game's reload at a real pace (its first shell 0.8 s after the key, then one every 0.67 s):
//       its last shells land more than 3 s after the key was let go and are still kept, each shell keeping the asking 1.5 s more
//   A4  an empty gun, X held: the latch "it began on an empty gun" is set, a shell pushed up by hand before the game's first still leaves that first
//       for the chamber, and the latch is dropped once the asking is over and the hidden round back in
//   A5  two shells, X tapped, both fired inside the 3 s: the first shot ends the asking (the hidden round back in that same look), the game's count
//       never reaches 0 and it never reloads by itself
//   A6  X held through a shot asks no more while it stays held, nor in the 3 s after it's let go; pressed anew it asks
//   A7  the game's reload nobody asked for: its shells wait while its count rises and are given back kRisingMs after its last rise, all at once
//   A8  one shell left, X tapped, the trigger pulled inside its 3 s: the pull ends the asking (tubeask::End, as input.cpp does it: the shot held that
//       frame), the look puts the hidden round back, then the shot goes, a frame late: the game's count never at 0, no reload by itself. Without the hold (the shot
//       let through at once) the stand-in game does begin one by itself: what the hold is for
//   A9  2000 random sessions on the clock (shots through the trigger's stop and that hold, X tapped and held, shells by hand, the handle, the
//       release): the game's count is never at 0 at a frame's end unless the key asks, it never begins a reload at 0 with nobody asking (asked for
//       on an empty gun it does begin its empty reload there: that one is the key's), whenever the counts are settled no shell has been made or lost,
//       and every session ends settled
// Usage: tubeasktest

#include <cstdio>
#include <cstdlib>

#include "../src/tubeask.h"

namespace {

using namespace shotgunammo;

int g_failed = 0, g_counted = 0;

void Check(bool ok, const char* row, const char* what) {
    ++g_counted;
    if (!ok)
        ++g_failed;
    std::printf("%s %s %s\n", ok ? "PASS" : "FAIL", row, what);
}

// The game's side (tools/shotgunammotest.cpp's).
struct Game {
    int loaded = 5, spare = 32;
    int fired = 0;
    int size = 5;
    bool reloading = false;
    bool byItself = false;  // ... its reload began by itself, at 0
    int loops = 0;
    int wait = 0;
    bool Shoot() {
        if (loaded < 1)
            return false;
        --loaded;
        ++fired;
        reloading = false;
        return true;
    }
    bool Start(bool itself = false) {
        if (reloading || loaded >= size || spare < 1)
            return false;
        reloading = true;
        byItself = itself;
        wait = loaded == 0 ? 8 : 5;
        return true;
    }
    bool Loop() {
        if (!reloading && loaded == 0)
            Start(true);
        if (!reloading)
            return false;
        if (loaded >= size || spare < 1) {
            reloading = false;
            return false;
        }
        ++loaded;
        --spare;
        ++loops;
        if (loaded >= size || spare < 1)
            reloading = false;
        return true;
    }
    bool Tick() {
        if (!reloading && loaded == 0)
            Start(true);
        if (!reloading || --wait > 0)
            return false;
        wait = 4;
        return Loop();
    }
};

// The mod's side, as ammo.cpp's StepTube keeps it.
struct Mod {
    State s;
    int capacity = 4;
    int last = -1, hidden = 0, putBacks = 0, lost = 0;
    tubeask::State ask;
    bool asked = false;  // the last look's
};

CountOut Look(Mod& m, Game& g, unsigned long long keySince, unsigned long long now) {
    CountIn in;
    in.last = m.last, in.raw = g.loaded, in.spare = g.spare, in.hidden = m.hidden, in.putBacks = m.putBacks, in.lost = m.lost, in.capacity = m.capacity;
    m.asked = tubeask::Before(m.ask, keySince, now, Total(m.s), in);
    const CountOut out = See(m.s, in);
    if (out.write) {
        g.loaded = out.loaded, g.spare = out.spare;
        if (out.putBack)
            ++m.putBacks;
    }
    m.hidden = out.hidden, m.lost = out.lost;
    tubeask::After(m.ask, out, m.asked, keySince, now, m.hidden);
    m.last = g.loaded;
    if (g.loaded >= 2)
        m.putBacks = 0;
    return out;
}

// Met as the game's count has it, the first look made.
void Meet(Mod& m, Game& g, unsigned long long now) {
    m = Mod{};
    m.s = FromCount(g.loaded);
    m.last = g.loaded;
    Look(m, g, tubeask::kNever, now);
}

constexpr unsigned long long kFrame = 14;  // ms

void A1() {
    Game g;
    Mod m;
    Meet(m, g, 1000);
    Check(g.loaded == 6 && m.hidden == 1 && !m.asked && Total(m.s) == 5 && g.spare == 32, "A1", "met full: the hidden round on top at the first look (6 as the game counts), nothing asked");
}

void A2() {
    Game g;
    Mod m;
    Meet(m, g, 1000);
    Look(m, g, tubeask::kNever, 1014);
    const bool never = !m.asked;
    Look(m, g, 2999, 5000);
    const bool still = m.asked;
    Look(m, g, 3000, 5001);
    const bool over = !m.asked;
    Check(never && still && over && g.loaded == 6 && m.hidden == 1, "A2", "the key let go asks for 3 s and not after; never pressed never asks (a full gun: its hidden round stays in)");
}

void A3() {
    Game g;
    g.loaded = 3;
    Mod m;
    Meet(m, g, 1000);  // 2 in the tube, one chambered; 4 as the game counts
    unsigned long long now = 2000;
    const CountOut out = Look(m, g, 0, now);  // X held
    const bool taken = out.hiddenOut && g.loaded == 3 && m.hidden == 0 && m.asked && !m.ask.fromEmpty;
    g.Start();
    int kept = 0;
    bool latch = false;
    unsigned long long keySince = 0;
    for (int i = 0; i < 120; ++i) {
        now += kFrame;
        if (i == 20)
            keySince = 1;  // X let go
        else if (keySince)
            keySince += kFrame;
        if (g.Tick()) {
        }
        const CountOut o = Look(m, g, keySince, now);
        kept += o.gameLoaded;
        latch = latch || m.ask.fromEmpty;
    }
    // Long after: the asking over, the hidden round back.
    for (int i = 0; i < 300; ++i) {
        now += kFrame;
        keySince += kFrame;
        g.Tick();
        Look(m, g, keySince, now);
    }
    Check(taken && kept == 2 && !latch && Total(m.s) == 5 && m.s.tube == 4 && g.loaded == 6 && m.hidden == 1 && g.spare == 30 && !m.asked && !g.reloading, "A3",
          "X held with room in the tube: the hidden round out, the game's two shells kept, then the hidden round back on top: 5 in the gun, 6 as the game counts, 30 spare");
}

void A3b() {
    Game g;
    g.loaded = 1;
    Mod m;
    Meet(m, g, 1000);
    unsigned long long now = 2000;
    g.Shoot();
    Look(m, g, tubeask::kNever, now);  // empty, the bolt held open
    now += kFrame;
    g.Start();  // the game reads its key
    Look(m, g, 0, now);  // X pressed
    unsigned long long keySince = 0, lastKeptAt = 0, letGoAt = 0;
    int kept = 0;
    for (int i = 1; i <= 600; ++i) {  // 8.4 s
        now += kFrame;
        if (i == 7)
            keySince = 1, letGoAt = now;  // let go after 100 ms
        else if (keySince)
            keySince += kFrame;
        if (i % 12 == 0)
            g.Tick();  // its reload at a twelfth of the frames: 0.8 s to its first shell, then one every 0.67 s (the last 3.5 s after the key)
        const CountOut o = Look(m, g, keySince, now);
        if (o.gameLoaded > 0)
            kept += o.gameLoaded, lastKeptAt = now;
    }
    Check(kept == 5 && lastKeptAt - letGoAt > kAskedMs && Total(m.s) == 5 && g.loaded == 6 && g.spare == 27 && m.hidden == 1 && !m.asked, "A3b",
          "X tapped on an empty gun, the game's reload at its real pace: all five shells kept, the last more than 3 s after the key was let go (each keeps the asking 1.5 s more)");
}

void A4() {
    Game g;
    g.loaded = 1;
    Mod m;
    Meet(m, g, 1000);  // one chambered; 2 as the game counts
    unsigned long long now = 2000;
    g.Shoot();
    Look(m, g, tubeask::kNever, now);  // fired: empty, the bolt held open, the game's count 1 (its hidden round)
    const bool empty = Total(m.s) == 0 && m.s.locked && g.loaded == 1 && m.hidden == 1;
    now += kFrame;
    const CountOut out = Look(m, g, 0, now);  // X held on the empty gun
    const bool began = out.hiddenOut && g.loaded == 0 && m.ask.fromEmpty;
    // A shell pushed up the tube by hand before the game's first.
    CountIn in;
    in.raw = g.loaded, in.spare = g.spare, in.hidden = m.hidden, in.lost = m.lost;
    const bool pushed = LoadTube(m.s, m.capacity, Belly(m.s, in)) == Load::kIn;
    now += kFrame;
    Look(m, g, 0, now);
    const bool stillLatched = m.ask.fromEmpty && m.s.locked && m.s.tube == 1;
    g.Start();
    bool chambered = false;
    unsigned long long keySince = 0;
    for (int i = 0; i < 200; ++i) {
        now += kFrame;
        if (i == 30)
            keySince = 1;
        else if (keySince)
            keySince += kFrame;
        g.Tick();
        const CountOut o = Look(m, g, keySince, now);
        chambered = chambered || o.gameChambered;
    }
    for (int i = 0; i < 300; ++i) {
        now += kFrame;
        keySince += kFrame;
        g.Tick();
        Look(m, g, keySince, now);
    }
    Check(empty && began && pushed && stillLatched && chambered && !m.ask.fromEmpty && Total(m.s) == 5 && !m.s.locked && g.loaded == 6 && g.spare == 27 && m.hidden == 1, "A4",
          "an empty gun, X held: the latch set; a shell pushed up by hand first, the game's first still goes into the chamber; the latch dropped when it's over (5 in the gun, 27 spare)");
}

void A5() {
    Game g;
    g.loaded = 2;
    Mod m;
    Meet(m, g, 1000);  // one in the tube, one chambered; 3 as the game counts
    unsigned long long now = 2000;
    Look(m, g, 0, now);  // X tapped: held 100 ms
    const bool out = g.loaded == 2 && m.hidden == 0;
    now += 100;
    Look(m, g, 0, now);
    bool zero = false, itself = false;
    unsigned long long keySince = 1;
    // Fired 300 ms and 600 ms after the tap, inside the asking's 3 s; the game's reload never begun by the tap here (it looks at its key's press: nothing held).
    bool backAtOnce = false;
    for (int i = 1; i <= 200; ++i) {
        now += kFrame;
        keySince += kFrame;
        if (i == 21 || i == 43) {
            if (m.s.chamber == Chamber::kLive && !m.s.locked)
                g.Shoot();
            const CountOut o = Look(m, g, keySince, now);
            if (i == 21)
                backAtOnce = o.shot && m.hidden == 1 && g.loaded == 2 && !m.ask.askedWas;
        } else {
            g.Tick();
            Look(m, g, keySince, now);
        }
        zero = zero || g.loaded == 0;
        itself = itself || (g.reloading && g.byItself);
    }
    Check(out && backAtOnce && !zero && !itself && Total(m.s) == 0 && m.s.locked && g.loaded == 1 && g.fired == 2 && g.spare == 32, "A5",
          "two shells, X tapped, both fired inside its 3 s: the first shot ends the asking and puts the hidden round back in that look; the game's count never at 0, no reload by itself");
}

void A6() {
    Game g;
    g.loaded = 3;
    Mod m;
    Meet(m, g, 1000);
    unsigned long long now = 2000;
    Look(m, g, 0, now);  // X held
    const bool asked = m.asked && m.hidden == 0;
    now += kFrame;
    g.Shoot();
    Look(m, g, 0, now);  // a shot, X still held
    const bool ended = m.hidden == 1 && g.loaded == 3;
    bool quiet = true;
    for (int i = 0; i < 50; ++i) {  // still held
        now += kFrame;
        Look(m, g, 0, now);
        quiet = quiet && !m.asked && m.hidden == 1;
    }
    for (unsigned long long since = 1; since < 2500; since += kFrame) {  // let go: its 3 s don't count
        now += kFrame;
        Look(m, g, since, now);
        quiet = quiet && !m.asked && m.hidden == 1;
    }
    now += kFrame;
    Look(m, g, 0, now);  // pressed anew
    Check(asked && ended && quiet && m.asked && m.hidden == 0, "A6", "X held through a shot asks no more while held nor after it's let go; pressed anew it asks");
}

void A7() {
    Game g;
    g.loaded = 3;
    Mod m;
    Meet(m, g, 1000);  // 4 as the game counts, 32 spare
    unsigned long long now = 2000, lastRise = 0, backAt = 0;
    g.Start();  // a key nobody pressed
    int back = 0, backLooks = 0, wroteWhileRising = 0;
    for (int i = 0; i < 400; ++i) {
        now += kFrame;
        const int before = g.loaded;
        if (g.Tick())
            lastRise = now;
        const int raw = g.loaded;
        const CountOut o = Look(m, g, tubeask::kNever, now);
        if (o.write && g.reloading)
            ++wroteWhileRising;
        if (o.back > 0) {
            back += o.back, ++backLooks;
            backAt = now;
        }
        (void)before, (void)raw;
    }
    Check(back == 1 && backLooks == 1 && wroteWhileRising == 0 && backAt - lastRise >= kRisingMs && backAt - lastRise < kRisingMs + 2 * kFrame && g.loaded == 4 && g.spare == 32 &&
              Total(m.s) == 3 && m.hidden == 1,
          "A7", "a reload nobody asked for: nothing written while its count rises; its shell given back, once, kRisingMs after its last rise");
}

// One shell left (one chambered), X tapped, the trigger pulled 100 ms later. held: the pull ends the asking and the shot waits for the hidden round
// (input.cpp's hold); not held: the shot goes at once. Whether the game began a reload by itself, and whether its count was ever at 0 at a frame's end.
void LastShell(bool held, bool& itself, bool& zero, int& fired, int& endCount, int& shotFrame) {
    Game g;
    g.loaded = 1;
    Mod m;
    Meet(m, g, 1000);  // 2 as the game counts
    unsigned long long now = 2000;
    Look(m, g, 0, now);  // X tapped: its hidden round out, 1 as the game counts
    unsigned long long keySince = 1;
    itself = zero = false;
    shotFrame = -1;
    bool pulled = true;  // the trigger held from 100 ms on until it has fired
    now += 100, keySince += 100;
    Look(m, g, keySince, now);
    for (int i = 0; i < 300; ++i) {
        now += kFrame;
        keySince += kFrame;
        if (pulled && OnPull(m.s, false) == Pull::kFires && g.loaded > 0) {
            if (held && m.hidden == 0 && Total(m.s) <= 1) {
                tubeask::End(m.ask, keySince);  // (the shot waits: the next look puts the hidden round back)
            } else {
                g.Shoot();
                pulled = false;
                shotFrame = i;
            }
        }
        g.Tick();
        itself = itself || (g.reloading && g.byItself);
        Look(m, g, keySince, now);
        zero = zero || g.loaded == 0;
    }
    fired = g.fired;
    endCount = g.loaded;
}

void A8() {
    bool itself = false, zero = false, itselfBare = false, zeroBare = false;
    int fired = 0, count = 0, firedBare = 0, countBare = 0, shotFrame = -1, shotFrameBare = -1;
    LastShell(true, itself, zero, fired, count, shotFrame);
    LastShell(false, itselfBare, zeroBare, firedBare, countBare, shotFrameBare);
    Check(!itself && !zero && fired == 1 && count == 1 && shotFrame == 1 && itselfBare && firedBare == 1 && countBare == 1 && shotFrameBare == 0, "A8",
          "one shell left, X tapped, the trigger pulled: the pull ends the asking and the shot waits one frame for the hidden round: the game's count never at 0, no reload by "
          "itself (let through at once, the stand-in game begins one)");
}

void A9() {
    std::srand(20261118);
    constexpr unsigned long long kStep = 125;  // ms a frame, as the core's random runs
    long long zeros = 0, selfStarts = 0, selfKept = 0, unsettledAtEnd = 0, madeOrLost = 0, sessions = 0, asks = 0, shots = 0, loads = 0;
    for (int session = 0; session < 2000; ++session) {
        Game g;
        g.loaded = 1 + std::rand() % 5;
        g.spare = std::rand() % 4 == 0 ? std::rand() % 3 : 8 + std::rand() % 30;
        const int all = g.loaded + g.spare;
        Mod m;
        unsigned long long now = 1000;
        Meet(m, g, now);
        int thrown = 0, xHeld = 0;
        unsigned long long keySince = tubeask::kNever;
        bool pressedSinceShot = false;
        bool unasked = false;  // the game's reload now running began at 0 with nobody asking
        for (int frame = 0; frame < 400; ++frame) {
            now += kStep;
            // The key.
            if (xHeld > 0) {
                if (--xHeld == 0)
                    keySince = 1;
            } else if (keySince != tubeask::kNever) {
                keySince += kStep;
            }
            if (xHeld == 0 && std::rand() % 40 == 0) {
                xHeld = 1 + std::rand() % 10;
                keySince = 0;
                pressedSinceShot = true;
                unasked = false;
                ++asks;
                g.Start();  // the game reads its key
            }
            const unsigned long long key = xHeld > 0 ? 0 : keySince;
            CountOut o = Look(m, g, key, now);
            // The hands (never while the game's reload is asked for or its shells wait: ammo.cpp's own rule for a shell; the bolt's moves alike here).
            if (!m.asked && !o.waiting && m.hidden) {
                const int r = std::rand() % 12;
                CountIn in;
                in.raw = g.loaded, in.spare = g.spare, in.hidden = m.hidden, in.lost = m.lost;
                if (r == 0) {
                    if (LoadTube(m.s, m.capacity, Belly(m.s, in)) == Load::kIn)
                        ++loads;
                } else if (r == 1) {
                    PortLoad(m.s, Belly(m.s, in));
                } else if (r == 2) {
                    BoltHome(m.s);
                } else if (r == 3 && std::rand() % 3 == 0) {
                    const Bolt b = Rack(m.s);
                    if (b.threw)
                        ++m.lost, ++thrown;
                }
                o = Look(m, g, key, now);
            }
            // The game's frame: the trigger (through its stop), then its reload's tick.
            if (std::rand() % 3 == 0 && OnPull(m.s, false) == Pull::kFires && g.loaded > 0) {
                if (m.hidden == 0 && Total(m.s) <= 1) {
                    tubeask::End(m.ask, key);  // (input.cpp's hold: the last shell with the hidden round out waits for it)
                    pressedSinceShot = false;
                } else {
                    g.Shoot();
                    ++shots;
                    pressedSinceShot = false;
                }
            }
            const bool wasItself = g.reloading && g.byItself;
            const bool asking = pressedSinceShot && m.asked;  // (as the last look had it: before the game's frame)
            g.Tick();
            if (g.reloading && g.byItself && !wasItself && !asking) {
                ++selfStarts;  // begun at 0 with nobody asking
                unasked = true;
            }
            if (!g.reloading)
                unasked = false;
            o = Look(m, g, key, now);
            if (o.gameLoaded > 0 && unasked)
                selfKept += o.gameLoaded;  // a shell kept of such a reload
            // The game's count at 0 at a frame's end only while the key asks.
            if (g.loaded == 0 && !(pressedSinceShot && m.asked))
                ++zeros;
            // Settled (the counts agree, nothing waiting): no shell made or lost.
            const bool settled = !o.waiting && g.loaded == m.hidden + Total(m.s) + m.lost;
            if (settled && Total(m.s) + g.spare + g.fired + thrown != all)
                ++madeOrLost;
        }
        // At the session's end, long after: settled.
        for (int i = 0; i < 60; ++i) {
            now += kStep;
            if (keySince != tubeask::kNever)
                keySince += kStep;
            g.Tick();
            Look(m, g, keySince, now);
        }
        if (g.loaded != m.hidden + Total(m.s) || Total(m.s) + g.spare + g.fired + thrown != all)
            ++unsettledAtEnd;
        ++sessions;
    }
    char what[400];
    std::snprintf(what, sizeof(what),
                  "%lld random sessions (%lld shots, %lld asks, %lld shells by hand): the game's count at 0 with nobody asking in %lld frames; %lld reloads it began at 0 "
                  "with nobody asking, %lld shells of those kept; a shell made or lost in %lld settled frames, %lld sessions not settled at their end",
                  sessions, shots, asks, loads, zeros, selfStarts, selfKept, madeOrLost, unsettledAtEnd);
    Check(zeros == 0 && selfStarts == 0 && selfKept == 0 && madeOrLost == 0 && unsettledAtEnd == 0 && shots > 1000 && asks > 1000 && loads > 1000, "A9", what);
}

}  // namespace

int main() {
    A1();
    A2();
    A3();
    A3b();
    A4();
    A5();
    A6();
    A7();
    A8();
    A9();
    std::printf("%d of %d checks failed\n", g_failed, g_counted);
    return g_failed;
}

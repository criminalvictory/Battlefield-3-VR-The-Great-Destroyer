// The M1014's shells by hand (src/shotgunammo.h, header only) without the game: a stand-in for the game's count (it fires while its count is
// above 0; its own reload, once begun, loads a shell a loop for as long as its count is below its own size, looking at the count each loop, and
// begins by itself at 0) and the mod's look at it each frame, as ammo.cpp is to run it (the hidden round on top of the game's count, what See
// says written unless the write is held off, the hands' moves between two looks, the time since its count last rose, the asking's latch
// "begun on an empty gun"). Each check prints PASS or FAIL, then the totals; the exit code is the number failed.
//   T1  met full (5 loaded, 32 spare, a tube of 4): one chambered and 4 in the tube, the game's count 6; the longer tube (7: one and 6, count 8);
//       met empty: the bolt held open, the trigger nothing
//   T2  fired to empty: each shot feeds the next from the tube, the 5th leaves the bolt held open; the game's count stays at 1 (its hidden round)
//       and it never reloads by itself; the trigger then does nothing (no click: out of battery)
//   T3  the ejection port: a shell dropped in with the bolt held open lies there, not counted yet (still a spare one, one fewer at the belly); the
//       release chambers it (now one fewer spare); refused with the bolt home, with a shell there already, with none left
//   T4  the loading port, empty gun: a shell at a time into the tube, each from the spare ones, nothing chambered, the bolt still open; full: the
//       next refused; the release then chambers one from the tube; the tube's size is the caller's number (5, 7)
//   T5  topping off with a shell chambered, the bolt home all through; a shot in the middle of a load
//   T6  the bolt: the release with the bolt home does nothing; sent home on an empty gun: the first pull clicks, later ones nothing till the bolt
//       is worked; racked on an empty tube: held open again; racked with shells in the tube and none chambered: one fed; racked with a live shell
//       chambered: thrown out, lost (the gun one fewer, the spare ones as they were); the last one racked out: held open; racked while held
//       open: home, as the release; the handle held: nothing fires
//   T7  few spare shells: the belly's count (the port's shell can't be taken a second time), may a hand take one (none left, the tube full, the
//       port free); the port's shell with the last spare one owed to the hidden round: back to the belly
//   T8  the game's own reload: not asked for: nothing written while its count rises (it ends by itself, its loops counted), then all given back
//       at once; asked for (X held): the hidden round out, each shell kept (into the tube; begun on an empty gun the first into the chamber, the
//       bolt home), the game stops at its own size, the hidden round back on top; a full tube: the hidden round stays in and the game starts
//       nothing; a tube of 5 is then one short and takes it by hand; a shell left in the port goes back; shells the tube has no room for go
//       back, once, when the count has stopped rising; how long X asks
//   T9  the hidden round fired (the mouse passes no stop): put back from the spare ones three times, then left for the game's reload, which
//       ends by itself and is given back; no spare shells: left alone; the mouse with the bolt open on shells in the tube: each shot a shell
//       of the tube's; a shot from shells of the game's not kept yet isn't the hidden round's; X held through the shot asks no longer; the
//       hidden round owed and X pressed: left at 0, and the game's first shell pays for it
//   T10 a write held off a frame is made the next; a shot between the hands' move and the write; a shell loaded and not yet written isn't at the
//       belly any more, nor is one behind a shell thrown out and not yet written
//   T11 the counts apart with the spare shells short: the gun holds as many fewer; Adopt changes as little as can be; broken counts: nothing
//   T12 a fired hull in the chamber (the pump's, later): no shell of the count, nothing to fire, thrown out by the handle at no loss
//   T13 6000 random sessions as ammo.cpp is to run it (a look, the hands' move, a look, the game's frame, a look; every write made; 125 ms a
//       frame; X pressed and held some frames, the asking by Asked on the times since, forgotten at a shot as See says; the game's reload begun
//       by X, by a key nobody pressed, or by itself at 0, a shell every 4th frame until its count is full): shots, the mouse's too, shells into
//       the tube and the port, the release, racks, pulls, tubes of 4, 5, 6 and 7: the game's count is the mod's and its hidden round after
//       every frame (but for its own shells waiting to go back), no shell is made or lost, the tube never over its size, the bolt never open
//       on a live shell, the port's shell always a spare one, the spare ones never short of what the gun holds (nothing adopted), the game's
//       count never at 0 with nobody asking (X pressed and no shot since: kept apart from what the mod is told), no shell kept of a reload
//       the game began by itself unasked, every reload of the game's ends and every wait too
//   T14 6000 more, rough: moves with no look before, writes held off, the game's shells racing the hands': still no shell made or lost, but for
//       a shot the game fired from a shell already thrown out with nothing left to pay for it (counted, few)
//   T15 what the review of 2026-11-18 found: two shells left, X tapped, both fired: the hidden round back in at the first shot, the game's
//       count never at 0, no reload by itself; a swap of the game's is no asking; X on an empty gun and a shell pushed up the tube by hand
//       before the game's first: that first still goes into the chamber (the asking began on an empty gun); and so does the first of a
//       reload the game's count was at 0 for, however the asking began
// Usage: shotgunammotest

#include <cstdio>
#include <cstdlib>

#include "../src/shotgunammo.h"

namespace {

using namespace shotgunammo;

int g_failed = 0, g_counted = 0;

void Check(bool ok, const char* row, const char* what) {
    ++g_counted;
    if (!ok)
        ++g_failed;
    std::printf("%s %s %s\n", ok ? "PASS" : "FAIL", row, what);
}

// The game's side.
struct Game {
    int loaded = 5, spare = 32;
    int fired = 0;
    int size = 5;  // its Ammo.MagazineCapacity: 5, 7 with the longer tube
    bool reloading = false;
    int loops = 0;  // shells its own reload loaded, all told
    int wait = 0;   // (Tick's: frames to its next loop)
    bool Shoot() {
        if (loaded < 1)
            return false;
        --loaded;
        ++fired;
        reloading = false;  // (a shot ends its reload)
        return true;
    }
    // Its own reload begun (its key; by itself at 0: Loop): only with room in its count and a spare shell.
    bool Start() {
        if (reloading || loaded >= size || spare < 1)
            return false;
        reloading = true;
        wait = loaded == 0 ? 8 : 5;
        return true;
    }
    // A loop of its reload: one shell, if its count is below its own size as it looks now (whatever the mod wrote meanwhile: a count written
    // back down each loop keeps it going); full after it, or no spare shell: its reload is over. At 0 it begins by itself.
    bool Loop() {
        if (!reloading && loaded == 0)
            Start();
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
    // A frame of it (the random runs): a loop every 4th frame of a reload (the first after 5 frames, 8 from empty).
    bool Tick() {
        if (!reloading && loaded == 0)
            Start();
        if (!reloading || --wait > 0)
            return false;
        wait = 4;
        return Loop();
    }
};

// The mod's side, as ammo.cpp is to keep it.
constexpr unsigned long long kLong = 1000000;  // (a time long ago, ms)
struct Mod {
    State s;
    int capacity = 4;
    int last = -1, hidden = 0, putBacks = 0, lost = 0;
    unsigned long long sinceRise = kLong;  // since a look last saw the game's count rise by itself
    bool wasAsked = false;                 // the last look was asked
    bool fromEmpty = false;                // the asking began on an empty gun
    int thrown = 0;   // (the test's own tallies: live shells the bolt threw out,
    int adopted = 0;  // looks that found the counts apart and the spare shells short)
};

CountIn In(const Mod& m, const Game& g, bool asked) {
    CountIn in;
    in.last = m.last, in.raw = g.loaded, in.spare = g.spare, in.hidden = m.hidden, in.asked = asked, in.putBacks = m.putBacks, in.lost = m.lost;
    in.capacity = m.capacity;
    in.fromEmpty = m.fromEmpty;
    in.sinceRise = m.sinceRise;
    return in;
}

CountOut Step(Mod& m, Game& g, bool asked = false, bool canWrite = true) {
    if (asked && !m.wasAsked)
        m.fromEmpty = Total(m.s) == 0;  // the asking begins: on an empty gun?
    const CountOut out = See(m.s, In(m, g, asked));
    const bool wrote = out.write && canWrite;
    if (wrote) {
        g.loaded = out.loaded;
        g.spare = out.spare;
        if (out.putBack)
            ++m.putBacks;
    }
    if (wrote || !out.write)
        m.hidden = out.hidden, m.lost = out.lost;
    if (out.adopted)
        ++m.adopted;
    if (out.rose)
        m.sinceRise = 0;
    m.wasAsked = asked && !out.shot;
    if (!m.wasAsked && m.hidden)
        m.fromEmpty = false;
    m.last = g.loaded;
    if (g.loaded >= 2)
        m.putBacks = 0;
    return out;
}
// The game's count has stopped rising for kRisingMs.
void Wait(Mod& m) {
    m.sinceRise = kRisingMs;
}

// The gun met: as the game's count has it, the hidden round then put on top by the first look.
void Meet(Mod& m, Game& g, int capacity = 4) {
    m = Mod{};
    m.capacity = capacity;
    m.s = FromCount(g.loaded);
    Step(m, g);
}

int Belly(const Mod& m, const Game& g) {
    return shotgunammo::Belly(m.s, In(m, g, false));
}
// What there is: the shells in the gun, spare, fired and thrown out.
int Sum(const Mod& m, const Game& g) {
    return Total(m.s) + g.spare + g.fired + m.thrown;
}

// The hands, as ammo.cpp is to run them: a look before, the hands' move, a look after.
Load Tube(Mod& m, Game& g, bool asked = false) {
    Step(m, g, asked);
    const Load l = LoadTube(m.s, m.capacity, Belly(m, g));
    Step(m, g, asked);
    return l;
}
Load Port(Mod& m, Game& g, bool asked = false) {
    Step(m, g, asked);
    const Load l = PortLoad(m.s, Belly(m, g));
    Step(m, g, asked);
    return l;
}
Bolt Release(Mod& m, Game& g, bool asked = false) {
    Step(m, g, asked);
    const Bolt b = BoltHome(m.s);
    Step(m, g, asked);
    return b;
}
Bolt Racked(Mod& m, Game& g, bool asked = false) {
    Step(m, g, asked);
    const Bolt b = Rack(m.s);
    if (b.threw)
        ++m.lost, ++m.thrown;
    Step(m, g, asked);
    return b;
}
// The right trigger through the mod's stop: a shot of the game's if a live shell is chambered and the bolt home.
bool Trigger(Mod& m, Game& g) {
    if (Pulled(m.s, false) != Pull::kFires)
        return false;
    g.Shoot();
    Step(m, g);
    return true;
}
// X held: the hidden round out, the game's own reload a shell a loop up to its size, the hidden round back. The shells it loaded (20: it never
// ended).
int XReload(Mod& m, Game& g) {
    Step(m, g, true);
    g.Start();
    int shells = 0;
    while (shells < 20 && g.Loop()) {
        ++shells;
        Step(m, g, true);
    }
    Step(m, g, false);
    return shells;
}
void FireAll(Mod& m, Game& g, int shots) {
    for (int i = 0; i < shots; ++i)
        Trigger(m, g);
}

void T1() {
    Game g;
    Mod m;
    Meet(m, g);
    Check(m.s.chamber == Chamber::kLive && m.s.tube == 4 && !m.s.locked && !m.s.port && Total(m.s) == 5 && g.loaded == 6 && g.spare == 32 &&
              m.hidden == 1,
          "T1", "met with 5 loaded, 32 spare: one chambered, 4 in the tube; the game's count 6 (the hidden round), the spare ones untouched");
    Check(Step(m, g).write == false && g.loaded == 6 && OnPull(m.s, false) == Pull::kFires, "T1",
          "a frame with nothing done: nothing written; the trigger fires");
    Game x;
    x.loaded = 7, x.size = 7;
    Meet(m, x, 6);
    Check(m.s.chamber == Chamber::kLive && m.s.tube == 6 && Total(m.s) == 7 && x.loaded == 8, "T1",
          "the longer tube (7 loaded): one chambered, 6 in the tube; the game's count 8");
    Game e;
    e.loaded = 0;
    Meet(m, e);
    Check(m.s.chamber == Chamber::kNone && m.s.tube == 0 && m.s.locked && Total(m.s) == 0 && e.loaded == 1 && e.spare == 32 &&
              OnPull(m.s, false) == Pull::kNothing,
          "T1", "met empty: nothing chambered, the bolt held open, the game's count 1; the trigger does nothing");
}

void T2() {
    Game g;
    Mod m;
    Meet(m, g);
    bool counted = true, early = false;
    for (int shot = 1; shot <= 4; ++shot) {
        const bool fired = Trigger(m, g);
        counted = counted && fired && Total(m.s) == 5 - shot && m.s.tube == 4 - shot && g.loaded == 1 + Total(m.s);
        if (m.s.locked || m.s.chamber != Chamber::kLive)
            early = true;
    }
    Check(counted && !early, "T2", "4 shots: each counts down (the chambered shell fired, the next fed from the tube), the game's count one above");
    Check(Pulled(m.s, false) == Pull::kFires, "T2", "the last shell: the trigger fires");
    g.Shoot();
    const CountOut last = Step(m, g);
    Check(last.fired == 1 && last.lockedNow && !last.hiddenFired && m.s.chamber == Chamber::kNone && m.s.tube == 0 && m.s.locked &&
              g.loaded == 1 && g.spare == 32 && g.fired == 5,
          "T2", "the 5th: the chamber empty, the bolt held open; the game's count 1, so it doesn't reload by itself; 32 spare");
    Check(!Trigger(m, g) && Pulled(m.s, false) == Pull::kNothing && !m.s.down && g.fired == 5 && !Step(m, g).write, "T2",
          "empty, the bolt open: the trigger fires nothing and clicks nothing, pull after pull");
    Check(!g.Loop() && !g.reloading && g.loaded == 1 && g.loops == 0, "T2", "the game's own reload doesn't begin by itself (its count isn't 0)");
    Game h;
    Mod n;
    Meet(n, h);
    h.Shoot(), h.Shoot();
    const CountOut two = Step(n, h);
    Check(two.fired == 2 && !two.hiddenFired && !two.lockedNow && Total(n.s) == 3 && n.s.chamber == Chamber::kLive && n.s.tube == 2, "T2",
          "two shots between two looks: both counted");
}

void T3() {
    Game g;
    Mod m;
    Meet(m, g);
    Check(Port(m, g) == Load::kBoltHome && !m.s.port && g.loaded == 6 && g.spare == 32, "T3",
          "a shell at the ejection port with the bolt home: refused, nothing changes");
    FireAll(m, g, 5);
    const int before = Sum(m, g);
    Check(Belly(m, g) == 32 && Port(m, g) == Load::kIn && m.s.port && m.s.locked && m.s.chamber == Chamber::kNone && Total(m.s) == 0 &&
              g.loaded == 1 && g.spare == 32 && Belly(m, g) == 31,
          "T3", "empty, the bolt open: a shell dropped into the port lies there, not counted yet (32 spare still, 31 left at the belly)");
    Check(Port(m, g) == Load::kTaken && OnPull(m.s, false) == Pull::kNothing && !Trigger(m, g), "T3",
          "a second one: refused (one lies there); the trigger still does nothing");
    const Bolt b = Release(m, g);
    Check(b.moved && b.wasLocked && b.fromPort && !b.fromTube && !b.threw && !b.locked && m.s.chamber == Chamber::kLive && !m.s.port &&
              !m.s.locked && m.s.tube == 0 && g.loaded == 2 && g.spare == 31 && Sum(m, g) == before,
          "T3", "the release pressed: the port's shell chambered, the bolt home; now one fewer spare (31), no shell made or lost");
    Check(Trigger(m, g) && m.s.locked && Total(m.s) == 0 && g.loaded == 1 && g.fired == 6, "T3", "it fires; the tube empty: the bolt held open again");
    // The port's shell, then the tube, then the release: the port's is the one chambered.
    Port(m, g);
    Tube(m, g), Tube(m, g);
    const Bolt c = Release(m, g);
    Check(c.fromPort && !c.fromTube && m.s.chamber == Chamber::kLive && m.s.tube == 2 && g.loaded == 4 && g.spare == 28, "T3",
          "a shell in the port and two in the tube, the release: the port's is chambered, the tube keeps its two");
    Game e;
    e.loaded = 0, e.spare = 0;
    Meet(m, e);
    Check(Port(m, e) == Load::kNoShell && !m.s.port && Tube(m, e) == Load::kNoShell && Total(m.s) == 0 && e.loaded == 1 && e.spare == 0, "T3",
          "no spare shells: none into the port, none into the tube");
}

void T4() {
    Game g;
    Mod m;
    Meet(m, g);
    FireAll(m, g, 5);
    const int before = Sum(m, g);
    bool ok = true;
    for (int k = 1; k <= 4; ++k)
        ok = ok && Tube(m, g) == Load::kIn && m.s.tube == k && m.s.chamber == Chamber::kNone && m.s.locked && g.loaded == 1 + k &&
             g.spare == 32 - k && Sum(m, g) == before;
    Check(ok, "T4", "empty, the bolt open: four shells up into the tube, each from the spare ones, nothing chambered, the bolt still open");
    Check(Tube(m, g) == Load::kFull && m.s.tube == 4 && g.loaded == 5 && g.spare == 28 && CanTake(m.s, 4, Belly(m, g), 0) == Take::kYes, "T4",
          "the fifth: refused, the tube is full (nothing changes); a hand may still take one, for the port");
    Check(OnPull(m.s, false) == Pull::kNothing && !Trigger(m, g), "T4", "the trigger: nothing yet (the bolt is open on an empty chamber)");
    const Bolt b = Release(m, g);
    Check(b.wasLocked && b.fromTube && !b.fromPort && m.s.chamber == Chamber::kLive && m.s.tube == 3 && !m.s.locked && g.loaded == 5 &&
              g.spare == 28 && Sum(m, g) == before,
          "T4", "the release: one from the tube chambered (3 left in it), no shell made or lost");
    Check(Tube(m, g) == Load::kIn && Total(m.s) == 5 && g.loaded == 6 && g.spare == 27 && Tube(m, g) == Load::kFull && Trigger(m, g), "T4",
          "one more up the tube: full again (4 and one), the next refused; it fires");
    Game w;
    w.loaded = 0;
    Mod five, seven;
    Meet(five, w, 5);
    int in5 = 0, in7 = 0;
    while (Tube(five, w) == Load::kIn)
        ++in5;
    Game x;
    x.loaded = 0, x.size = 7;
    Meet(seven, x, 7);
    while (Tube(seven, x) == Load::kIn)
        ++in7;
    Check(in5 == 5 && five.s.tube == 5 && w.spare == 27 && in7 == 7 && seven.s.tube == 7 && x.spare == 25, "T4",
          "the tube's size is the caller's: a tube of 5 takes 5, one of 7 takes 7");
}

void T5() {
    Game g;
    Mod m;
    Meet(m, g);
    FireAll(m, g, 2);
    const int before = Sum(m, g);
    const bool one = Tube(m, g) == Load::kIn && m.s.chamber == Chamber::kLive && !m.s.locked && Total(m.s) == 4;
    const bool two = Tube(m, g) == Load::kIn && m.s.chamber == Chamber::kLive && !m.s.locked && Total(m.s) == 5;
    Check(one && two && g.loaded == 6 && g.spare == 30 && Sum(m, g) == before && Tube(m, g) == Load::kFull, "T5",
          "two fired, two pushed up the tube with a shell chambered: 5 again, 30 spare, the bolt home all through; a third refused");
    FireAll(m, g, 3);
    const bool mid = Tube(m, g) == Load::kIn && Trigger(m, g) && Tube(m, g) == Load::kIn;
    Check(mid && Total(m.s) == 3 && m.s.chamber == Chamber::kLive && m.s.tube == 2 && g.loaded == 4 && g.spare == 28 && g.fired == 6 &&
              Sum(m, g) == before,
          "T5", "a shot in the middle of a load: a shell in, a shot, a shell in: each counted, no shell made or lost");
}

void T6() {
    Game g;
    Mod m;
    Meet(m, g);
    const State was = m.s;
    const Bolt none = Release(m, g);
    Check(!none.moved && !none.wasLocked && m.s.tube == was.tube && m.s.chamber == was.chamber && !m.s.locked && g.loaded == 6, "T6",
          "the release pressed with the bolt home: nothing");
    FireAll(m, g, 5);
    const Bolt home = Release(m, g);
    Check(home.wasLocked && !home.fromPort && !home.fromTube && !home.locked && !m.s.locked && m.s.chamber == Chamber::kNone && g.loaded == 1, "T6",
          "empty, the release: the bolt goes home on an empty chamber");
    const bool click = OnPull(m.s, false) == Pull::kClick && Pulled(m.s, false) == Pull::kClick && m.s.down;
    Check(click && Pulled(m.s, false) == Pull::kNothing && Pulled(m.s, false) == Pull::kNothing && !Trigger(m, g) && g.fired == 5, "T6",
          "the trigger then: the first pull clicks, later ones do nothing; nothing fires");
    const Bolt open = Racked(m, g);
    Check(!open.threw && !open.fromTube && open.locked && m.s.locked && !m.s.down && m.thrown == 0 && g.loaded == 1 && g.spare == 32, "T6",
          "racked on an empty tube: nothing thrown out, the bolt held open again, the hammer cocked");
    Release(m, g);
    Check(Pulled(m.s, false) == Pull::kClick, "T6", "home again: the first pull clicks again");
    Tube(m, g), Tube(m, g);
    Check(m.s.tube == 2 && m.s.chamber == Chamber::kNone && !m.s.locked && OnPull(m.s, false) == Pull::kNothing && !Trigger(m, g), "T6",
          "two shells up the tube with the bolt home on an empty chamber: nothing chambered, the trigger fires nothing");
    const Bolt fed = Racked(m, g);
    Check(!fed.threw && fed.fromTube && !fed.locked && m.s.chamber == Chamber::kLive && m.s.tube == 1 && g.loaded == 3 && g.spare == 30 &&
              Trigger(m, g),
          "T6", "racked: one fed from the tube, nothing thrown out; it fires");
    // A live shell racked out.
    Game h;
    Mod n;
    Meet(n, h);
    const Bolt out = Racked(n, h);
    Check(out.threw && !out.threwHull && out.fromTube && !out.locked && Total(n.s) == 4 && n.s.tube == 3 && h.loaded == 5 && h.spare == 32 &&
              n.lost == 0 && n.thrown == 1 && Sum(n, h) == 37,
          "T6", "racked with a live shell chambered: it's thrown out (lost: 4 in the gun, the spare ones as they were), the next fed");
    Game k;
    k.loaded = 1;
    Meet(n, k);
    const Bolt lastOut = Racked(n, k);
    Check(lastOut.threw && !lastOut.fromTube && lastOut.locked && n.s.locked && Total(n.s) == 0 && k.loaded == 1 && k.spare == 32, "T6",
          "the only shell racked out: the tube empty, the bolt held open");
    Port(n, k);
    const Bolt tug = Racked(n, k);
    Check(tug.wasLocked && tug.fromPort && !tug.threw && !tug.locked && n.s.chamber == Chamber::kLive && k.loaded == 2 && k.spare == 31, "T6",
          "the handle let go while held open (a tug): the bolt home, the port's shell chambered, as the release");
    Check(OnPull(n.s, true) == Pull::kNothing && OnPull(n.s, false) == Pull::kFires, "T6", "a live shell chambered, the handle held: nothing fires");
    State s;  // (home on an empty chamber)
    Check(Pulled(s, true) == Pull::kNothing && !s.down && Pulled(s, false) == Pull::kClick, "T6",
          "the handle held on an empty chamber: no click (the hammer stays cocked for the next pull)");
}

void T7() {
    Game g;
    g.loaded = 0, g.spare = 2;
    Mod m;
    Meet(m, g);
    Check(Belly(m, g) == 2 && CanTake(m.s, 4, Belly(m, g), 0) == Take::kYes && CanTake(m.s, 4, Belly(m, g), 1) == Take::kYes &&
              CanTake(m.s, 4, Belly(m, g), 2) == Take::kNoShells,
          "T7", "two spare shells: a hand may take one, and a second; with two in the hands, none left");
    Check(Port(m, g) == Load::kIn && g.spare == 2 && Belly(m, g) == 1 && CanTake(m.s, 4, Belly(m, g), 1) == Take::kNoShells, "T7",
          "one into the port: 2 spare as the game counts them, one left at the belly");
    Check(Tube(m, g) == Load::kIn && g.spare == 1 && Belly(m, g) == 0 && Tube(m, g) == Load::kNoShell && m.s.tube == 1 && m.s.port &&
              CanTake(m.s, 4, Belly(m, g), 0) == Take::kNoShells,
          "T7", "the other up the tube: the last spare one is the port's, it can't be taken a second time");
    const Bolt b = Release(m, g);
    Check(b.fromPort && Total(m.s) == 2 && g.loaded == 3 && g.spare == 0 && Sum(m, g) == 2, "T7",
          "the release: the port's shell chambered, none spare; two in the gun, as there were");
    Game f;
    Mod n;
    Meet(n, f);
    Check(CanTake(n.s, 4, Belly(n, f), 0) == Take::kFull, "T7", "a full gun, the bolt home: no shell taken (the tube is full)");
    // The port's shell is the last spare one, and then the hidden round is fired and not put back (too often already): that spare one is the
    // hidden round's now.
    Game o;
    o.loaded = 0, o.spare = 4;
    Mod q;
    Meet(q, o);
    for (int i = 0; i < 3; ++i)
        o.Shoot(), Step(q, o);  // the mouse: the hidden round, put back three times
    const bool lastOne = o.spare == 1 && q.putBacks == 3 && Port(q, o) == Load::kIn && q.s.port && Belly(q, o) == 0;
    o.Shoot();
    const CountOut owed = Step(q, o);
    Check(lastOne && owed.hiddenFired && !owed.putBack && !owed.write && owed.portBack && !q.s.port && o.loaded == 0 && o.spare == 1 &&
              q.hidden == 1,
          "T7", "the last spare shell in the port, then the hidden round fired and left owed: the port's shell is no more (it pays that round)");
    State s;
    s.tube = 4, s.locked = true;
    State p = s;
    p.port = true;
    Check(CanTake(s, 4, 10, 0) == Take::kYes && CanTake(p, 4, 10, 0) == Take::kFull, "T7",
          "a full tube with the bolt open: one for the port; a shell there already: none");
}

void T8() {
    // Not asked for (its key reached the game, nobody pressed X): nothing written while its count rises, so it ends by itself; then all back.
    Game g;
    Mod m;
    Meet(m, g);
    FireAll(m, g, 5);
    bool each = g.Start();
    int loops = 0;
    while (loops < 20 && g.Loop()) {
        ++loops;
        const CountOut out = Step(m, g);
        each = each && out.rose && out.undone && out.waiting && !out.write && out.gameLoaded == 0 && out.back == 0 && Total(m.s) == 0 &&
               m.s.locked && g.loaded == 1 + loops && m.hidden == 1;
    }
    Check(each && loops == 4 && !g.reloading && g.loaded == 5 && g.spare == 28, "T8",
          "the game's reload of the empty gun, not asked for: no shell kept and nothing written while its count rises; it ends by itself after "
          "4 loops (its count full)");
    CountOut out = Step(m, g);
    const bool still = out.waiting && !out.write && !out.undone && g.loaded == 5;
    Wait(m);
    out = Step(m, g);
    Check(still && out.back == 4 && !out.waiting && !out.settled && !out.undone && out.write && g.loaded == 1 && g.spare == 32 &&
              Total(m.s) == 0 && m.s.locked && !g.Loop() && g.loops == 4,
          "T8", "... its count still for 1.5 s: the four given back at once, the bolt still open; the game begins no other");
    Game h;
    Mod n;
    Meet(n, h);
    FireAll(n, h, 3);
    h.size = 7;  // (so that it has room, the hidden round in its count)
    h.Start();
    loops = 0;
    while (loops < 20 && h.Loop()) {
        ++loops;
        Step(n, h);
    }
    Wait(n);
    out = Step(n, h);
    Check(loops == 4 && out.back == 4 && h.loaded == 3 && h.spare == 32 && Total(n.s) == 2 && !h.Loop(), "T8",
          "... of a gun with 2 left: ends by itself, given back, 2 still in it");
    // Asked for (X held), with shells in the gun: into the tube.
    Game k;
    Mod x;
    Meet(x, k);
    FireAll(x, k, 3);
    out = Step(x, k, true);
    Check(out.hiddenOut && out.write && !out.settled && out.back == 0 && k.loaded == 2 && k.spare == 32 && x.hidden == 0 && Total(x.s) == 2 &&
              !x.fromEmpty,
          "T8", "X held: the hidden round out of the game's count (2, as the gun holds), for nothing");
    bool kept = k.Start();
    for (int i = 1; i <= 3; ++i) {
        kept = kept && k.Loop();
        out = Step(x, k, true);
        kept = kept && out.gameLoaded == 1 && !out.gameChambered && !out.undone && !out.waiting && x.s.tube == 1 + i && k.spare == 32 - i;
        // (the third fills the tube: the hidden round back in at once)
        kept = kept && (i < 3 ? !out.write && k.loaded == 2 + i : out.hiddenIn && !out.settled && k.loaded == 6);
    }
    Check(kept && !k.Loop() && x.s.chamber == Chamber::kLive && Total(x.s) == 5 && x.hidden == 1, "T8",
          "the game's own reload, a shell a loop: each kept, into the tube; it stops at its own size (5: the tube's 4 and the chambered one), "
          "the tube full: the hidden round back on top (6)");
    out = Step(x, k, false);
    Check(!out.write && k.loaded == 6 && k.spare == 29 && Sum(x, k) == 37 && Trigger(x, k), "T8",
          "the reload over: nothing more to write; no shell made or lost; it fires");
    // A full gun: the hidden round stays in, the game sees a full gun and starts nothing.
    Game full;
    Mod fm;
    Meet(fm, full);
    out = Step(fm, full, true);
    Check(!out.write && !out.hiddenOut && fm.hidden == 1 && full.loaded == 6 && !full.Start() && !full.Loop(), "T8",
          "X held on a full gun: the hidden round stays in (no room in the tube), the game starts nothing");
    State openFull;  // (the tube filled by hand, the bolt still open on an empty chamber)
    openFull.tube = 4, openFull.locked = true;
    CountIn fin;
    fin.last = 5, fin.raw = 5, fin.spare = 20, fin.hidden = 1, fin.asked = true;
    out = See(openFull, fin);
    Check(!out.write && out.hidden == 1 && !out.hiddenOut, "T8",
          "... and on a full tube with the bolt open: the same (the hand presses the release)");
    // Asked for, from empty: the game's empty reload.
    Game e;
    Mod y;
    Meet(y, e);
    FireAll(y, e, 5);
    out = Step(y, e, true);
    Check(out.hiddenOut && e.loaded == 0 && y.hidden == 0 && y.fromEmpty, "T8",
          "X held, the gun empty: the game's count at 0 (its empty reload plays)");
    e.Loop();  // (it begins at 0)
    out = Step(y, e, true);
    Check(out.gameLoaded == 1 && out.gameChambered && y.s.chamber == Chamber::kLive && !y.s.locked && y.s.tube == 0 && e.loaded == 1, "T8",
          "its first shell: into the chamber, the bolt home");
    int more = 0;
    while (more < 20 && e.Loop()) {
        ++more;
        out = Step(y, e, true);
    }
    const bool top = out.hiddenIn && e.loaded == 6;  // (the tube full at its last shell)
    Step(y, e, false);
    Check(more == 4 && top && y.s.tube == 4 && Total(y.s) == 5 && e.loaded == 6 && e.spare == 27 && Sum(y, e) == 37 && !y.fromEmpty, "T8",
          "then four into the tube: 5 in the gun, the hidden round back on top; no shell made or lost");
    // The hidden round still in the game's count as its first shell lands (X and that shell between two looks).
    Game q;
    Mod z;
    Meet(z, q);
    FireAll(z, q, 2);  // 3 in it, the game's count 4
    q.Start(), q.Loop();
    out = Step(z, q, true);
    Check(out.gameLoaded == 1 && out.hiddenOut && !out.undone && Total(z.s) == 4 && z.s.tube == 3 && q.loaded == 4 && q.spare == 31 &&
              z.hidden == 0,
          "T8", "X and the first shell between two looks: the shell kept, then the hidden round out");
    // A tube of 5: the game stops one short, the hand's shell fills it.
    Game w;
    Mod five;
    Meet(five, w, 5);
    FireAll(five, w, 5);
    const int shells = XReload(five, w);
    Check(shells == 5 && Total(five.s) == 5 && five.s.tube == 4 && w.loaded == 6 && Tube(five, w) == Load::kIn && Total(five.s) == 6 &&
              w.loaded == 7 && w.spare == 26 && Tube(five, w) == Load::kFull,
          "T8", "a tube of 5: the game's reload stops at 5 in the gun, one short; the hand's shell fills it (the game's count 7)");
    // A shell the hands left in the port, then the game's empty reload.
    Game r;
    Mod p;
    Meet(p, r);
    FireAll(p, r, 5);
    Port(p, r);
    Step(p, r, true);
    r.Loop();
    out = Step(p, r, true);
    Check(out.portBack && out.gameChambered && !p.s.port && p.s.chamber == Chamber::kLive && Total(p.s) == 1 && r.spare == 31 && Sum(p, r) == 37,
          "T8", "a shell in the port, X held: the game's own goes into the chamber, the hands' goes back to the belly (it was never counted)");
    // The game's size above the tube's (the longer tube not seen): what has no room goes back, once, when its count has stopped rising.
    Game big;
    big.size = 7;
    Mod small;
    Meet(small, big, 4);
    out = Step(small, big, true);  // 5 in it, the game's count 6: the hidden round stays in
    bool none = !out.write && small.hidden == 1 && big.Start();
    loops = 0;
    while (loops < 20 && big.Loop()) {
        ++loops;
        out = Step(small, big, true);
        none = none && out.refused == 1 && out.gameLoaded == 0 && !out.undone && out.waiting && !out.write && out.back == 0;
    }
    Check(none && loops == 1 && !big.reloading && big.loaded == 7 && big.spare == 31 && Total(small.s) == 5 && small.s.tube == 4, "T8",
          "the game's shell into a full tube: no room (not kept, and not \"not asked for\"), nothing written; its reload ends by itself");
    Wait(small);
    out = Step(small, big, true);
    Check(out.back == 1 && out.refused == 0 && !out.undone && !out.settled && big.loaded == 6 && big.spare == 32 && Total(small.s) == 5 &&
              Sum(small, big) == 37,
          "T8", "... its count still for 1.5 s: back to the spare ones");
    Check(Asked(0, ~0ULL) && Asked(kAskedMs - 1, ~0ULL) && !Asked(kAskedMs, ~0ULL) && Asked(~0ULL, kRisingMs - 1) && !Asked(~0ULL, kRisingMs) &&
              !Asked(~0ULL, ~0ULL),
          "T8", "X asks while held and 3 s after; each shell of the game's keeps it asked 1.5 s more; then no longer");
}

void T9() {
    Game g;
    g.loaded = 2;
    Mod m;
    Meet(m, g);  // 2 in it, the game's count 3
    bool ok = true;
    g.Shoot(), Step(m, g);
    g.Shoot(), Step(m, g);
    ok = ok && Total(m.s) == 0 && m.s.locked && g.loaded == 1;
    for (int i = 1; i <= 3; ++i) {
        g.Shoot();
        const CountOut out = Step(m, g);
        ok = ok && out.shot && out.hiddenFired && out.putBack && g.loaded == 1 && g.spare == 32 - i && m.putBacks == i && m.hidden == 1;
    }
    Check(ok, "T9", "the hidden round fired three times: put back each time from the spare ones (29 left), the game's count never left at 0");
    g.Shoot();
    CountOut out = Step(m, g);
    Check(out.hiddenFired && !out.putBack && !out.write && g.loaded == 0 && m.hidden == 1, "T9", "the fourth: not put back");
    int loops = 0;
    bool left = true;
    while (loops < 20 && g.Loop()) {  // by itself, from empty
        ++loops;
        out = Step(m, g);
        // (its first shell is the hidden round's, owed: nothing over yet)
        left = left && out.undone && out.waiting == (loops > 1) && !out.write && !out.hiddenPaid && out.gameLoaded == 0;
    }
    Wait(m);
    out = Step(m, g);
    Check(left && loops == 5 && out.back == 4 && g.loaded == 1 && g.spare == 28 && Total(m.s) == 0 && m.s.locked && Sum(m, g) == 34 && !g.Loop(),
          "T9",
          "the game's reload that follows: left to end by itself (5 loops), then given back: the count at the hidden round (one from the spare "
          "ones), the gun still empty; every shot paid for");
    Game e;
    e.loaded = 1, e.spare = 0;
    Meet(m, e);
    e.Shoot(), Step(m, e);
    e.Shoot();
    out = Step(m, e);
    Check(out.hiddenFired && !out.putBack && e.loaded == 0 && e.spare == 0 && !Step(m, e).write && !e.Loop(), "T9",
          "no spare shells: left at 0, nothing written");
    // The mouse with the bolt open on shells in the tube (nothing chambered: no shot of the gun's): each takes a shell of the tube's, none from
    // the belly; the tube empty, the next is the hidden round's.
    Game t;
    t.loaded = 0, t.spare = 3;
    Mod u;
    Meet(u, t);
    Tube(u, t), Tube(u, t);  // 2 in the tube, the bolt open, the game's count 3, one spare
    t.Shoot();
    out = Step(u, t);
    const bool first = out.fired == 0 && out.firedTube == 1 && !out.hiddenFired && !out.adopted && !out.write && u.s.tube == 1 && u.s.locked &&
                       t.loaded == 2 && t.spare == 1;
    t.Shoot();
    out = Step(u, t);
    const bool second = out.firedTube == 1 && !out.hiddenFired && u.s.tube == 0 && u.s.locked && t.loaded == 1 && t.spare == 1 && Sum(u, t) == 3;
    t.Shoot();
    out = Step(u, t);
    Check(first && second && out.firedTube == 0 && out.hiddenFired && out.putBack && t.loaded == 1 && t.spare == 0 && Sum(u, t) == 3, "T9",
          "the mouse fires with the bolt open on two in the tube: each shot a shell of the tube's, the spare ones untouched; then the hidden round");
    // The mouse fires from shells of the game's own reload that aren't kept yet (its count above 0): not the hidden round's.
    Game v;
    v.loaded = 0, v.spare = 10;
    Mod b;
    Meet(b, v);  // empty, the game's count 1
    v.Start();
    v.Loop(), Step(b, v);
    v.Loop(), Step(b, v);  // its count 3, 8 spare
    v.Shoot();
    out = Step(b, v);
    const bool notHidden = out.shot && out.fired == 0 && out.firedTube == 0 && !out.hiddenFired && !out.putBack && out.waiting && !out.write &&
                           v.loaded == 2 && !v.reloading;
    Wait(b);
    out = Step(b, v);
    Check(notHidden && out.back == 1 && v.loaded == 1 && v.spare == 9 && b.hidden == 1 && Total(b.s) == 0 && Sum(b, v) == 10, "T9",
          "the mouse fires a shell of the game's reload nobody asked for: not the hidden round's (that is still in); the rest given back");
    // X held through the shot asks no longer: the hidden round is put back as ever. Owed (put back too often) and X pressed: left at 0, and the
    // game's first shell is the hidden round's.
    Game k;
    k.loaded = 1, k.spare = 10;
    Mod n;
    Meet(n, k);
    k.Shoot(), Step(n, k);
    k.Shoot();
    out = Step(n, k, true);
    Check(out.shot && out.hiddenFired && out.putBack && k.loaded == 1 && k.spare == 9 && n.hidden == 1, "T9",
          "the hidden round fired with X held: the shot ends the asking, the round put back (the count not left at 0)");
    k.Shoot(), Step(n, k);
    k.Shoot(), Step(n, k);
    k.Shoot();
    out = Step(n, k);  // the fourth: owed, the count at 0, 7 spare
    const bool owedNow = !out.putBack && k.loaded == 0 && k.spare == 7 && n.hidden == 1;
    out = Step(n, k, true);
    Check(owedNow && !out.write && !out.putBack && k.loaded == 0 && n.hidden == 1 && n.fromEmpty, "T9",
          "the hidden round owed, X pressed: not put back, the count left at 0 for the game's empty reload");
    k.Loop();
    out = Step(n, k, true);
    Check(out.hiddenPaid && !out.undone && !out.waiting && out.gameLoaded == 0 && out.back == 0 && Total(n.s) == 0 && k.loaded == 0 &&
              k.spare == 6 && n.hidden == 0 && Sum(n, k) == 11,
          "T9", "the game's first shell: the hidden round's (paid from the spare ones), then that round out: no shell of the gun's yet");
    int shells = 0;
    while (shells < 20 && k.Loop()) {
        ++shells;
        Step(n, k, true);
    }
    Step(n, k, false);
    Check(shells == 5 && Total(n.s) == 5 && n.s.chamber == Chamber::kLive && k.loaded == 6 && k.spare == 1 && Sum(n, k) == 11, "T9",
          "the reload goes on from 0 and ends (5 loops): one chambered, 4 in the tube, the hidden round back on top; no shell made or lost");
}

void T10() {
    Game g;
    Mod m;
    Meet(m, g);
    FireAll(m, g, 5);
    Step(m, g);
    LoadTube(m.s, m.capacity, Belly(m, g));
    CountOut out = Step(m, g, false, false);  // (a take of the game's open: not written)
    Check(out.write && g.loaded == 1 && g.spare == 32 && Total(m.s) == 1 && Belly(m, g) == 31, "T10",
          "a shell up the tube while no count may be written: the game's count waits; the shell isn't at the belly any more (31)");
    out = Step(m, g);
    Check(out.settled && g.loaded == 2 && g.spare == 31, "T10", "the next frame: written, one from the spare ones");
    // A shot between the hands' move and the write.
    Game h;
    Mod n;
    Meet(n, h);
    FireAll(n, h, 1);
    LoadTube(n.s, n.capacity, Belly(n, h));  // (not yet written: the game still counts 5)
    h.Shoot();
    out = Step(n, h);
    Check(out.fired == 1 && Total(n.s) == 4 && h.loaded == 5 && h.spare == 31 && Sum(n, h) == 37, "T10",
          "a shot before the loaded shell was written: counted, then written; no shell made or lost");
    // The last spare shell, loaded and not yet written: no second one.
    Game k;
    k.loaded = 0, k.spare = 1;
    Mod x;
    Meet(x, k);
    const Load first = LoadTube(x.s, x.capacity, Belly(x, k));
    const Load second = LoadTube(x.s, x.capacity, Belly(x, k));
    Step(x, k);
    Check(first == Load::kIn && second == Load::kNoShell && x.s.tube == 1 && k.loaded == 2 && k.spare == 0, "T10",
          "one spare shell, up the tube and not yet written: a second is refused (none left)");
    // A live shell racked out and not yet written: the game's count still has it, so the shell loaded after it is owed from the spare ones.
    Game r;
    r.loaded = 3, r.spare = 1;
    Mod y;
    Meet(y, r);  // 3 in it, the game's count 4
    if (Rack(y.s).threw)
        ++y.lost, ++y.thrown;
    const Load third = LoadTube(y.s, y.capacity, Belly(y, r));
    const int bellyThen = Belly(y, r);
    const Load fourth = LoadTube(y.s, y.capacity, Belly(y, r));
    out = Step(y, r);
    Check(y.thrown == 1 && third == Load::kIn && bellyThen == 0 && fourth == Load::kNoShell && out.settled && !out.adopted && Total(y.s) == 3 &&
              r.loaded == 4 && r.spare == 0 && y.lost == 0 && Sum(y, r) == 4,
          "T10", "one spare shell, a live one racked out and one pushed up, neither written yet: a second is refused (none left)");
}

void T11() {
    Game g;
    g.loaded = 3, g.spare = 0;
    Mod m;
    Meet(m, g);  // 3 in it, the game's count 4
    m.s = FromCount(9);  // (the mod's count astray: 9 where the game has 3 and its hidden round, no spare ones to make it so)
    CountOut out = Step(m, g);
    Check(out.adopted && !out.write && Total(m.s) == 3 && m.s.chamber == Chamber::kLive && m.s.tube == 2 && g.loaded == 4 && g.spare == 0, "T11",
          "the mod's count above the game's with no spare shells to take: the gun as the game's count has it, nothing written");
    Game h;
    h.loaded = 3, h.spare = 2;
    Meet(m, h);
    m.s = FromCount(9);
    out = Step(m, h);
    Check(out.adopted && out.write && Total(m.s) == 5 && h.loaded == 6 && h.spare == 0, "T11",
          "... with 2 spare: the gun holds what they pay for (5), none spare: no shell made");
    m.lost = 2;  // (a tally something broke: more thrown out than the gun and the spare ones ever had)
    m.s = FromCount(0);
    h.loaded = 1;
    m.last = 1;
    out = Step(m, h);
    Check(out.adopted && !out.write && Total(m.s) == 0 && h.loaded == 1 && m.lost == 0, "T11",
          "a tally that can't be: the gun as the game's count has it, nothing written");
    State s = FromCount(4);
    Adopt(s, 2);
    const bool fewer = s.chamber == Chamber::kLive && s.tube == 1 && !s.locked;
    Adopt(s, 0);
    const bool none = s.chamber == Chamber::kNone && s.tube == 0 && s.locked;
    Adopt(s, -3);
    Check(fewer && none && s.tube == 0 && s.locked, "T11", "Adopt: fewer: the tube's shells first; none: the chamber empty, the bolt held open");
    State t;  // (home on an empty chamber)
    t.tube = 2;
    Adopt(t, 3);
    Check(t.tube == 3 && t.chamber == Chamber::kNone && !t.locked, "T11",
          "Adopt: more, the bolt home on an empty chamber: into the tube, nothing chambered");
    State d;  // (a state the rules never make: the hammer down on a live shell)
    d.chamber = Chamber::kLive, d.down = true;
    Adopt(d, 0);
    Check(d.chamber == Chamber::kNone && d.locked && !d.down, "T11", "Adopt: emptied: the bolt held open, the hammer never left down with it");
    Game b;
    Mod n;
    Meet(n, b);
    const State before = n.s;
    CountIn in = In(n, b, false);
    in.raw = -1;
    out = See(n.s, in);
    in = In(n, b, false);
    in.spare = -1;
    const CountOut out2 = See(n.s, in);
    Check(!out.write && !out2.write && n.s.tube == before.tube && n.s.chamber == before.chamber && !n.s.locked, "T11",
          "a count below 0 (unread): nothing done");
}

void T12() {
    State s;
    s.chamber = Chamber::kSpent, s.tube = 2;
    const bool count = Total(s) == 2 && Fire(s, 1) == 0 && OnPull(s, false) == Pull::kClick;
    State open = s;
    open.locked = true;
    const bool port = PortLoad(open, 5) == Load::kTaken;
    const Bolt b = Rack(s);
    Check(count && port && b.threwHull && !b.threw && b.fromTube && s.chamber == Chamber::kLive && s.tube == 1, "T12",
          "a fired hull in the chamber (the pump's, later): no shell of the count, nothing to fire, no port load over it; racked out at no loss, "
          "the next fed");
}

// Random sessions, 125 ms a frame. careful: as ammo.cpp is to run it (a look, the hands' move, a look, the game's frame, a look; every write
// made). Not careful: moves with no look before, writes held off now and then, the game's shells racing the hands'.
void Random(const char* row, unsigned seed, bool careful) {
    std::srand(seed);
    static const int kSizes[4][2] = {{5, 4}, {7, 6}, {5, 5}, {7, 7}};  // the game's size, the tube's
    constexpr unsigned long long kFrame = 125;
    int bad = 0, kind = 0, sessions = 0, events = 0, adopted = 0, unpaid = 0, portLoads = 0, racksOut = 0, gameShells = 0;
    int selfKept = 0, backs = 0, askedShots = 0, zeros = 0;
    for (; sessions < 6000 && !bad; ++sessions) {
        const int* size = kSizes[std::rand() % 4];
        Game g;
        g.size = size[0];
        g.loaded = std::rand() % (g.size + 1);
        g.spare = std::rand() % 4 == 0 ? std::rand() % 6 : 8 + std::rand() % 40;
        int shells = g.loaded + g.spare;
        Mod m;
        Meet(m, g, size[1]);
        // What the mod is told (the asking, by its times) ...
        int xHeld = 0;  // frames X is held still
        unsigned long long sinceX = kLong, sinceShell = kLong;
        // ... and, apart from it, what is so: X pressed and no shot since; the game's reload under way begun by itself with nobody asking.
        bool shotSinceX = true, selfStarted = false;
        int seen = 0;      // (the adopted looks counted so far)
        int waited = 0;    // frames on end the game's own shells have waited to go back with its reload over
        int reloading = 0; // frames on end the game's reload has run
        const auto asking = [&] { return Asked(xHeld > 0 ? 0 : sinceX, sinceShell); };
        const auto look = [&](bool canWrite) {
            const bool asked = asking();
            const CountOut out = Step(m, g, asked, canWrite);
            if (out.shot) {  // the asking is over: forgotten
                if (asked)
                    ++askedShots;
                xHeld = 0;
                sinceX = sinceShell = kLong;
            }
            if (out.gameLoaded > 0) {
                sinceShell = 0;
                gameShells += out.gameLoaded;
                if (selfStarted)
                    selfKept += out.gameLoaded;
            }
            if (out.back > 0 && canWrite)
                ++backs;
            return out;
        };
        for (int e = 0; e < 200 && !bad; ++e, ++events) {
            if (xHeld > 0) {
                if (--xHeld == 0)
                    sinceX = 0;
            } else {
                sinceX += kFrame;
            }
            sinceShell += kFrame;
            m.sinceRise += kFrame;
            const int what = std::rand() % 20;
            const bool canWrite = careful || std::rand() % 10 != 0;
            const bool press = what == 13 && std::rand() % 2 == 0;
            if (press) {
                xHeld = 1 + std::rand() % 6;  // X pressed
                shotSinceX = false;
                selfStarted = false;
            }
            if (careful)
                look(true);
            if (press)
                g.Start();  // (the game reads the key after the mod's look)
            if (what < 5) {
                if (Pulled(m.s, false) == Pull::kFires && g.Shoot())
                    shotSinceX = true;
            } else if (what == 5) {
                if (g.Shoot())  // the mouse: no stop
                    shotSinceX = true;
            } else if (what < 9) {
                LoadTube(m.s, m.capacity, Belly(m, g));
            } else if (what == 9) {
                if (PortLoad(m.s, Belly(m, g)) == Load::kIn)
                    ++portLoads;
            } else if (what == 10) {
                BoltHome(m.s);
            } else if (what == 11) {
                const Bolt b = Rack(m.s);
                if (b.threw)
                    ++m.lost, ++m.thrown, ++racksOut;
            } else if (what == 12) {
                Pulled(m.s, true);  // (the handle held: nothing)
            } else if (what == 14) {
                if (g.Start() && !(asking() && !shotSinceX))
                    selfStarted = true;  // a reload nobody asked for
            }
            if (careful)
                look(true);
            const bool was = g.reloading;
            const bool zero = g.loaded == 0;
            g.Tick();  // the game's frame: its reload a shell on, or begun by itself at 0
            if (!was && g.reloading && zero && !(asking() && !shotSinceX))
                selfStarted = true;
            const CountOut out = look(canWrite);
            // The counts apart and the spare shells short: never, as ammo.cpp is to run it. Rough: a shell loaded and not yet written that the
            // game's own reload took as well, and a shell thrown out that the game fired before its count was told, with nothing left to pay
            // for it (a shot from nothing, the one thing not made good: counted, and what there is counted anew).
            const bool apart = m.adopted > seen;
            adopted += m.adopted - seen;
            seen = m.adopted;
            if (!careful && out.adopted && !out.write) {
                const int owedNow = m.hidden && g.loaded == 0 && Total(m.s) == 0 ? 1 : 0;
                if (Sum(m, g) != shells + owedNow)
                    ++unpaid;
                shells = Sum(m, g) - owedNow;
            }
            // Settled (written this frame, or nothing to write): the game's count is the mod's and its hidden round, unless that round was fired
            // and is owed still (the count at 0), or shells of the game's own wait in its count to go back; the shells in the gun, spare, fired
            // and thrown out are what there was, and that owed round, less those waiting.
            const bool settled = canWrite || !out.write;
            const int owed = m.hidden && g.loaded == 0 && Total(m.s) == 0 ? 1 : 0;
            const int over = out.waiting ? g.loaded - m.hidden - Total(m.s) - m.lost : 0;
            const bool wanted = asking() && !shotSinceX;
            waited = out.waiting && !g.reloading ? waited + 1 : 0;
            reloading = g.reloading ? reloading + 1 : 0;
            if (g.loaded == 0 && wanted)
                ++zeros;
            int now = 0;
            if (settled && !owed && !out.waiting && g.loaded != m.hidden + Total(m.s))
                now = 1;
            else if (settled && Sum(m, g) + over != shells + owed)
                now = 2;
            else if (g.loaded < 0 || g.spare < 0 || m.s.tube < 0 || m.s.tube > m.capacity || over < 0)
                now = 3;
            else if ((m.s.locked && m.s.chamber == Chamber::kLive) || (m.s.port && (!m.s.locked || m.s.chamber != Chamber::kNone)) ||
                     (m.s.down && (m.s.locked || m.s.chamber == Chamber::kLive)) || m.s.chamber == Chamber::kSpent)
                now = 4;
            else if (settled && m.s.port && g.spare < 1)
                now = 5;
            else if (careful && apart)
                now = 6;
            else if (settled && !out.waiting && m.lost != 0)
                now = 7;
            else if (careful && g.loaded == 0 && !owed && !wanted)
                now = 8;  // the game's count at 0 with nobody asking: it reloads the gun by itself
            else if (waited > 13 || reloading > 80)
                now = 9;  // the game's reload never ends, or its shells aren't given back 1.5 s after it
            if (now) {
                ++bad;
                kind = now;
            }
        }
    }
    char what[640];
    std::snprintf(what, sizeof(what),
                  "%d random sessions%s, %d frames (%d port loads, %d live shells racked out, %d shells of the game's own reload kept, %d of them "
                  "of a reload it began by itself unasked, %d times its shells given back, %d shots that ended an asking, %d frames at 0 for "
                  "its empty reload; the spare ones short %d times%s): the counts agree after every settled frame, no shell made or lost, "
                  "nothing out of range, every reload of the game's ends (%d wrong, check %d)",
                  sessions, careful ? " as ammo.cpp is to run it" : ", rough", events, portLoads, racksOut, gameShells, selfKept, backs, askedShots,
                  zeros, adopted, careful ? "" : ", each a race of the game's with an unwritten move", bad, kind);
    Check(bad == 0 && sessions == 6000 && portLoads > 100 && racksOut > 100 && gameShells > 100 && backs > 100 && askedShots > 100 &&
              zeros > 100 && (!careful || selfKept == 0),
          row, what);
    if (!careful) {
        std::snprintf(what, sizeof(what), "... but %d shots the game fired from shells already thrown out, with nothing left to pay for them", unpaid);
        Check(unpaid < sessions / 20, row, what);
    }
}

// What the review of 2026-11-18 found.
void T15() {
    // Two shells left, X tapped, both fired before the game's first shell: the asking must not outlive the shots.
    Game g;
    g.loaded = 2;
    Mod m;
    Meet(m, g);  // 2 in it, the game's count 3
    CountOut out = Step(m, g, true);
    const bool tapped = out.hiddenOut && g.loaded == 2 && g.Start();
    const bool one = Pulled(m.s, false) == Pull::kFires && g.Shoot();
    out = Step(m, g, true);  // (X let go 0.1 s ago: by its time it asks still)
    Check(tapped && one && out.shot && out.fired == 1 && out.hiddenIn && out.write && m.hidden == 1 && Total(m.s) == 1 && g.loaded == 2 &&
              !g.reloading,
          "T15", "2 shells, X tapped, a shot: the shot ends the asking, the hidden round back in the same look (the game's count 2)");
    const bool two = Pulled(m.s, false) == Pull::kFires && g.Shoot();
    out = Step(m, g);  // (the caller forgot X at the shot)
    Check(two && out.fired == 1 && out.lockedNow && Total(m.s) == 0 && m.s.locked && g.loaded == 1 && g.spare == 32 && !g.Loop() &&
              g.loops == 0 && Pulled(m.s, false) == Pull::kNothing,
          "T15", "the second: the gun empty, the bolt open, the game's count 1: it reloads nothing by itself");
    // A swap of the game's is no asking (asked is X only): the looks of a swap's window leave the hidden round in.
    Game e;
    e.loaded = 0;
    Mod n;
    Meet(n, e);  // empty, the game's count 1
    bool in = true;
    for (int i = 0; i < 20; ++i) {  // (a swap just made: nothing X did)
        out = Step(n, e, false);
        in = in && !out.write && !out.hiddenOut && n.hidden == 1 && e.loaded == 1 && !e.Loop();
    }
    Check(in && e.spare == 32 && Total(n.s) == 0 && n.s.locked, "T15",
          "the empty gun through a swap of the game's: no asking, the hidden round stays in, the game reloads nothing by itself");
    // X on an empty gun, a shell pushed up the tube by hand before the game's first: its empty reload plays all the same.
    Game k;
    k.loaded = 0;
    Mod x;
    Meet(x, k);
    out = Step(x, k, true);
    const bool began = out.hiddenOut && k.loaded == 0 && x.fromEmpty;
    const bool pushed = Tube(x, k, true) == Load::kIn && x.s.tube == 1 && x.s.locked && k.loaded == 1 && k.spare == 31 && x.fromEmpty;
    k.Start(), k.Loop();  // (its count 2: not from 0 any more)
    out = Step(x, k, true);
    Check(began && pushed && out.gameLoaded == 1 && out.gameChambered && x.s.chamber == Chamber::kLive && !x.s.locked && x.s.tube == 1 &&
              k.loaded == 2 && k.spare == 30,
          "T15", "X on an empty gun, a shell pushed up by hand, then the game's first: into the chamber, the bolt home (the asking began on empty)");
    State s = FromCount(0);
    s.tube = 1;
    const GameLoad late = GameLoads(s, 2, false, 4);
    Check(!late.chambered && late.taken == 2 && s.tube == 3 && s.locked && s.chamber == Chamber::kNone, "T15",
          "... not begun on empty (shells in the tube, the bolt open: the game's tactical reload): all into the tube, the bolt left open");
    int shells = 0;
    while (shells < 20 && k.Loop()) {
        ++shells;
        Step(x, k, true);
    }
    Step(x, k, false);
    Check(shells == 3 && Total(x.s) == 5 && x.s.tube == 4 && k.loaded == 6 && k.spare == 27 && Sum(x, k) == 32 && !x.fromEmpty, "T15",
          "the rest into the tube: full, the hidden round back on top; no shell made or lost");
    // The asking begun with a shell in the gun, that one racked out: the game's count at 0, its empty reload plays all the same.
    Game r;
    r.loaded = 1;
    Mod y;
    Meet(y, r);  // one chambered, the game's count 2
    Step(y, r, true);
    const Bolt b = Racked(y, r, true);
    const bool emptied = b.threw && b.locked && !y.fromEmpty && Total(y.s) == 0 && r.loaded == 0 && y.hidden == 0;
    r.Loop();  // (it begins at 0)
    out = Step(y, r, true);
    Check(emptied && out.gameLoaded == 1 && out.gameChambered && y.s.chamber == Chamber::kLive && !y.s.locked && r.loaded == 1 && r.spare == 31, "T15",
          "X with one shell in the gun, that one racked out, the game's first from 0: into the chamber, the bolt home");
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
    Random("T13", 20261118u, true);
    Random("T14", 20261119u, false);
    T15();
    std::printf("%d of %d checks failed\n", g_failed, g_counted);
    return g_failed;
}

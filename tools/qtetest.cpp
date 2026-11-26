// The game's quick-time events on the grips (src/qte.h, header only, as input.cpp runs it), without the game, on frames at 72 Hz.
//   1  ordinary play: no squeeze of either grip, however made, ever holds anything (10 000 frames of both grips working)
//   2  a scripted moment: a fresh squeeze of either grip holds the six from its frame and lets go as it's let go; the other grip too; both at
//      once (held till the last is let go, one press); the thresholds (0.6 presses, 0.4 lets go, between them nothing changes)
//   3  not fresh: a grip already squeezed as the moment begins (the gun's) presses nothing until it's been seen let go in the moment
//   4  the moment over: everything let go that frame, whatever the grips do; the grip still squeezed in play holds nothing, and a new moment
//      wants a new squeeze
//   5  a flick a frame long: held 120 ms, never past the moment's end
//   6  the game's turn of the look: taken with no gun in hand as it began, not with one (said once a turn, nothing held); a cutscene's camera
//      with a gun in hand taken
//   7  bf3 not in front, "qte 0": nothing held, a press under way let go with its reason; Stop from outside
//   8  a grip not read or not a number: no press, one under way let go, and it must be seen let go again
//   9  a clock going back; the time
// Version 2 (qte.h's Step2: only what the game awaits; src/qtelist.h: the reader of the game's list of live input triggers, here on a memory
// image made up in the test):
//   10 exactly the awaited keys: each of the 63 masks pressed whole and nothing else, never Left Ctrl; nothing awaited, nothing pressed;
//      a grip squeezed as a key comes to be awaited presses nothing; either grip; both
//   11 a tapped key: down 70 ms and up; again every 130 ms while the grip stays squeezed (7.7 a second); "qtemash 0" once a squeeze; a tap
//      finished when the grip lets go in it; the player's own mash; two squeezes never one press (60 ms between)
//   12 a held key: down while squeezed, 120 ms at the least; held and tapped keys together
//   13 everything let go at every end: nothing awaited any more, a key no longer awaited (the others go on), another key awaited instead (a
//      fresh squeeze wanted), bf3 not in front, switched off, Stop2 from outside (the headset out of focus), a grip not read, a clock gone back
//   14 play's own Fire, Jump and Interact triggers count only in a scripted moment (an ordinary Fire trigger never fires the gun), the six
//      quick-time actions always; every other action ignored; the moment over, let go
//   15 the list not found or not read: nothing awaited, nothing pressed (the grips do nothing new)
//   16 "seen by the game": each press as the game's input shows it, or doesn't; a press run into the last one isn't seen
//   17 the reader: the train's lists; adds and removes not yet applied; wrong vtables; no manager, a wrong one, an unreadable one; an entity
//      listed again and again, a list without end, bounds that make no sense, pointers leading back (never a fault, the reads bounded); held
//      and tapped triggers; a read torn by the game's change steadied for 50 ms; the game's input state; the signature's search
//   18 the train's three quick-time events, the game's side simulated (its list in the image, its input read 30 times a second): the grapple
//      won by one left click, the hanging by one Space, the dangling climb by holding the grip (30 presses in about 4 s); "qtemash 0" and one
//      squeeze doesn't climb; never a key the game didn't await
//   19 the time
// The fist fights (src/qtefight.h, build\designs\qte_fights.md: all six quick-time triggers live, the prompt's one key read from the prefab's gates):
//   20 the proofs of every constant: against the real code (build\bf3_mem.exe beside the test; SKIP without it), on a made-up image each broken alone
//   21 the attach on a made-up fight's bus as the game builds it: passing; every check failing alone, named; the name unreadable; no trigger (the walk);
//      a state that wouldn't read each frame: no attach, said once, never lost frame after frame
//   22 each frame's gates: each prompt alone, a press or a mash; two prompts, a lone wrong gate, another's: nothing; a switch caught halfway in every
//      order of its writes; each frame's checks failing: nothing held over
//   23 the watch over the frames: a fight's Start, prompts and Stop; the train's singles on their level's bus as before; a fight not yet readable; a
//      checkpoint's load; a frame torn mid-fight (nothing at all pressed); the reader off; more buses than watched; two fights live; a prompt only while
//      the fight runs it (a won fight's gates left open with its fail gate shut: nothing; open gates standing a minute, ten seconds with no fail gate:
//      taken as over); a fight's bus failing past the prefab's checks with three triggers listed; a bus not readable at first; the list not read
//   24 the whole loop with the game's side simulated (its gates by the prefab's logic, its fail gate opened with each prompt and shut as it's won or
//      failed, a wrong key failing it, its input read 30 times a second): six press prompts won; a mash won by holding the grip; held and early squeezes;
//      version 2 as it was fails, the reader off presses nothing; a fight never stopped in free play; a fight and a single together; the test's grip
//      over prompts with no frame between; a never-stopped fight won with its last gates left open
//   25 the guard: the other inputs' quick-time keys held up in a prompt but its answer's, releases always, nothing outside prompts; the guard in a fight;
//      a frame the fights couldn't be read (the guard as it was, lapsing by itself); the right stick's stance in a prompt
//   26 the time
//   18aF-gF, 18h  the train's three quick-time events again as the mod runs them now (through the fights' reader, their triggers on a level's own bus)
//   18i  ... and with weapon.txt qtetest's grip, as filmed (a scripted moment); 18j the same not in one, as this test ran them before 2026-11-19
// A grip squeezed already answers (2026-11-19, the owner's cellar fight: E then a 0.41 s Fire prompt read, no grip press ever began):
//   27   the cellar with a grip held from before the fight (either hand) and from before each prompt: each prompt pressed as it opens, once, won; a
//        re-squeeze in a press prompt already pressed: nothing (one press a prompt), the next prompt freshly squeezed: pressed; two prompts with no frame
//        between, held: each once; a mash held: the rhythm from its first frame, a re-squeeze goes on; the one press kept over a frame not read, its
//        gates unclear, the headset off 0.1 s; the trains with qtetest's held grip and with the gun's grip held, in a scripted moment: won as filmed,
//        in play: nothing; nothing pressed with no prompt; either grip, any squeeze past 0.6; why a single waits 0.1 s; the grips' log lines; TestHeld
// Its review (2026-11-19):
//   28   a held grip's press of a single ends with its scripted moment (never on into play), comes back with it, ends beside a fight's triggers, goes on
//        beside play's own key; a press prompt's tap keeps its 70 ms over a frame read unclearly (the cellar held, a frame torn after each tap: won in all
//        three tick phases); a tap the game's input never showed leaves the prompt to a fresh squeeze (the cellar's Fire opening with the trigger's button
//        down: won), one shown or not told refuses it; the other quick-time keys the game's input shows while a prompt is up (the owner's cellar: a stray
//        Space from outside the mod fails it, named); the grips' log over frames read unclearly: one stretch; the words
// The in-game check of 2026-11-18's two log lines:
//   29a  the squeeze's line says the taps it makes (qte::TapWords as QteSay2 says them, each prompt as QteStep asks for it: qte::TapRule): a fight's press
//        prompt once, a mash and a single again every 130 ms, "qtemash 0" once a squeeze; 29b a fight's trigger's line with the fights' reader off says it
//        presses nothing (qtefight::TriggerWords, qtewatch.cpp's SayChanges)
// One owner per squeeze (the audit of 2026-11-19):
//   30   a single's key awaited in play: a hand carrying something (a magazine at the belly) presses nothing, a free hand's squeeze presses it, and
//        so does the gun's own grip or the handguard's, let go and squeezed afresh (the review of 2026-11-25: HoldsForKey never counts the gun's own
//        holds); the owner (OwnerStep) from the press's first frame till that grip is let go, never the holding hand, and from a press owed for the
//        key-up gap (a hand that comes to hold something meanwhile begins none); in a scripted moment and a fight's prompt the holds count for nothing
//        (as 27); a press whose hand comes to hold something ends; a fight's run (FightStep); the words; a key in play keeps the gun in the hand 10 s
//        at most (KeyKeepsGun)
// The key each of E and Space goes down as (the players lane, 2026-11-25: a player who moved use or jump):
//   31   qtelist::ProperKeys (qtewatch.cpp's Frame::proper, input.cpp's QteLogical): the quick-time action's own key where one awaits it, a fight's
//        triggers live beside it or not (QuickKeys gives none then); play's own alone play's; a fight's buses and a bus with all six left out; the
//        train's climb beside its fight in the whole loop
// Usage: qtetest
// The exit code is the number of failed checks.

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../src/qte.h"
#include "../src/qtefight.h"
#include "../src/qtelist.h"

namespace {

int g_failed = 0, g_counted = 0, g_skipped = 0;
const char* g_tag = "";  // added to the ids of a run done again another way (18's through the fights' reader: "F")

void Report(const char* id, const char* what, bool pass, const char* format, ...) {
    char numbers[768];
    va_list args;
    va_start(args, format);
    std::vsnprintf(numbers, sizeof(numbers), format, args);
    va_end(args);
    char tagged[16];
    std::snprintf(tagged, sizeof(tagged), "%s%s", id, g_tag);
    std::printf("%-4s %-5s %s | %s\n", tagged, pass ? "PASS" : "FAIL", what, numbers);
    ++g_counted;
    g_failed += !pass;
}

void Skip(const char* id, const char* what) {
    std::printf("%-4s SKIP  %s\n", id, what);
    ++g_skipped;
}

constexpr unsigned long long kFrame = 14;  // (a frame at 72 Hz, about)

// One run: a state, a clock, and what the frames gave.
struct Run {
    qte::State s;
    unsigned long long now = 100000;
    int began = 0, ended = 0, heldFrames = 0, ignored = 0;
    qte::End lastWhy = qte::End::kNone;
    unsigned long long lastHeldMs = 0;
    int lastHand = -1;
    qte::Out last;

    qte::Out Frame(bool cutscene, float left, float right, bool turn = false, bool armed = false, bool front = true, bool on = true,
                   bool readLeft = true, bool readRight = true) {
        qte::In in;
        in.nowMs = now;
        in.on = on;
        in.front = front;
        in.cutscene = cutscene;
        in.turn = turn;
        in.turnArmed = armed;
        in.read[0] = readLeft, in.read[1] = readRight;
        in.squeeze[0] = left, in.squeeze[1] = right;
        last = qte::Step(s, in);
        began += last.began;
        ended += last.ended;
        heldFrames += last.held;
        ignored += last.ignored;
        if (last.began)
            lastHand = last.hand;
        if (last.ended)
            lastWhy = last.why, lastHeldMs = last.heldMs;
        now += kFrame;
        return last;
    }
    void Frames(int n, bool cutscene, float left, float right, bool turn = false, bool armed = false) {
        for (int i = 0; i < n; ++i)
            Frame(cutscene, left, right, turn, armed);
    }
};

void Play() {
    Run r;
    for (int i = 0; i < 10000; ++i) {
        // Both grips working as in play: the right holding the gun with a let-go now and then, the left at magazines.
        const float right = i % 700 < 650 ? 1.0f : 0.0f;
        const float left = 0.5f + 0.5f * std::sin(i * 0.05f);
        r.Frame(false, left, right);
    }
    Report("1a", "ordinary play: nothing held in 10 000 frames of both grips", r.heldFrames == 0 && r.began == 0 && r.ended == 0 && r.ignored == 0,
           "held frames %d, presses %d, ends %d", r.heldFrames, r.began, r.ended);
}

void Moment() {
    {
        Run r;
        r.Frames(20, true, 0.0f, 0.0f);
        const int before = r.heldFrames;
        const qte::Out o = r.Frame(true, 0.0f, 1.0f);
        r.Frames(30, true, 0.0f, 1.0f);
        const int during = r.heldFrames;
        const qte::Out e = r.Frame(true, 0.0f, 0.0f);
        Report("2a", "the right grip's fresh squeeze holds from its frame, lets go as it's let go",
               before == 0 && o.began && o.held && o.hand == 1 && o.cutscene && during == 31 && e.ended && !e.held && e.why == qte::End::kGrip &&
                   e.heldMs == 31 * kFrame,
               "before %d, began %d hand %d, held frames %d, ended %d after %llu ms", before, o.began, o.hand, during, e.ended, e.heldMs);
        const qte::Out l = r.Frame(true, 1.0f, 0.0f);
        r.Frames(10, true, 1.0f, 0.0f);
        const qte::Out le = r.Frame(true, 0.1f, 0.0f);
        Report("2b", "the left grip the same", l.began && l.hand == 0 && le.ended && le.why == qte::End::kGrip && r.began == 2 && r.ended == 2,
               "began %d hand %d, ended %d; presses %d ends %d", l.began, l.hand, le.ended, r.began, r.ended);
    }
    {
        Run r;
        r.Frames(5, true, 0.0f, 0.0f);
        r.Frames(20, true, 1.0f, 0.0f);
        r.Frames(20, true, 1.0f, 1.0f);
        const qte::Out mid = r.Frame(true, 0.0f, 1.0f);  // (the left let go, the right still squeezed)
        r.Frames(20, true, 0.0f, 1.0f);
        const qte::Out e = r.Frame(true, 0.0f, 0.0f);
        Report("2c", "both grips: one press, held till the last is let go", r.began == 1 && r.ended == 1 && mid.held && !mid.ended && e.ended && r.lastHand == 0,
               "presses %d, ends %d, held with one let go %d", r.began, r.ended, mid.held);
    }
    {
        Run r;
        r.Frames(5, true, 0.0f, 0.0f);
        r.Frames(10, true, 0.59f, 0.0f);
        const bool under = r.heldFrames == 0;
        r.Frame(true, 0.61f, 0.0f);
        const bool over = r.last.began;
        r.Frames(10, true, 0.41f, 0.0f);  // (between the two: still held)
        const bool between = r.last.held && r.ended == 0;
        r.Frames(10, true, 0.39f, 0.0f);
        Report("2d", "the thresholds: 0.6 presses, 0.4 lets go, between them nothing changes", under && over && between && r.ended == 1 && !r.last.held,
               "0.59 none %d, 0.61 press %d, 0.41 held %d, 0.39 let go %d", under, over, between, r.ended);
    }
}

void NotFresh() {
    Run r;
    r.Frames(50, false, 0.0f, 1.0f);  // (play: the right grip holds the gun)
    r.Frames(200, true, 0.0f, 1.0f);  // (the moment begins with it squeezed)
    const bool none = r.heldFrames == 0 && r.began == 0;
    r.Frames(3, true, 0.0f, 0.5f);  // (eased, not let go)
    r.Frames(10, true, 0.0f, 1.0f);
    const bool eased = r.heldFrames == 0;
    r.Frames(3, true, 0.0f, 0.2f);  // (let go in the moment)
    r.Frames(10, true, 0.0f, 1.0f);
    Report("3a", "a grip squeezed as the moment begins presses nothing until seen let go in it", none && eased && r.began == 1 && r.heldFrames == 10,
           "held as it began %d, after easing to 0.5 %d, after a let-go: presses %d held frames %d", !none, !eased, r.began, r.heldFrames);
    // The left, fresh, works meanwhile.
    Run q;
    q.Frames(50, false, 0.0f, 1.0f);
    q.Frames(5, true, 0.0f, 1.0f);
    q.Frames(10, true, 1.0f, 1.0f);
    const bool leftWorks = q.began == 1 && q.lastHand == 0;
    q.Frames(5, true, 0.0f, 1.0f);
    Report("3b", "the other grip, fresh, presses meanwhile; let go, the held one doesn't keep it", leftWorks && q.ended == 1 && !q.last.held,
           "presses %d by hand %d, ends %d, held at the end %d", q.began, q.lastHand, q.ended, q.last.held);
}

void MomentOver() {
    Run r;
    r.Frames(5, true, 0.0f, 0.0f);
    r.Frames(20, true, 1.0f, 1.0f);
    const qte::Out e = r.Frame(false, 1.0f, 1.0f);  // (the moment ends, both grips squeezed)
    const int heldAtEnd = r.heldFrames;
    r.Frames(500, false, 1.0f, 1.0f);
    r.Frames(500, false, 0.0f, 1.0f);
    r.Frames(500, false, 1.0f, 0.0f);
    Report("4a", "the moment over: let go that frame, whatever the grips do; nothing held in play after",
           e.ended && !e.held && e.why == qte::End::kMoment && r.heldFrames == heldAtEnd && r.began == 1 && r.ended == 1,
           "ended %d (why %d), held frames after %d, presses %d", e.ended, static_cast<int>(e.why), r.heldFrames - heldAtEnd, r.began);
    r.Frames(100, true, 1.0f, 0.0f);  // (a new moment, the left still squeezed from play)
    const bool carried = r.began == 2;
    r.Frames(2, true, 0.0f, 0.0f);
    r.Frames(5, true, 1.0f, 0.0f);
    Report("4b", "a new moment wants a new squeeze", !carried && r.began == 2, "carried in %d, presses in all %d", carried, r.began);
    // A one-frame gap in the moment drops the press too (nothing is carried over it).
    Run g;
    g.Frames(5, true, 0.0f, 0.0f);
    g.Frames(10, true, 1.0f, 0.0f);
    g.Frame(false, 1.0f, 0.0f);
    g.Frames(50, true, 1.0f, 0.0f);
    Report("4c", "a frame out of the moment ends the press; held on through it, the grip presses nothing after", g.ended == 1 && g.began == 1 && !g.last.held,
           "presses %d, ends %d", g.began, g.ended);
}

void Flick() {
    Run r;
    r.Frames(5, true, 0.0f, 0.0f);
    r.Frame(true, 1.0f, 0.0f);
    int frames = 1;
    while (r.Frame(true, 0.0f, 0.0f).held && frames < 100)
        ++frames;
    Report("5a", "a flick a frame long is held 120 ms", r.ended == 1 && r.lastHeldMs >= qte::kShortest && r.lastHeldMs < qte::kShortest + 2 * kFrame,
           "held %d frames, %llu ms", frames, r.lastHeldMs);
    Run q;
    q.Frames(5, true, 0.0f, 0.0f);
    q.Frame(true, 1.0f, 0.0f);
    q.Frame(true, 0.0f, 0.0f);
    const qte::Out e = q.Frame(false, 0.0f, 0.0f);
    Report("5b", "... never past the moment's end", e.ended && !e.held && e.why == qte::End::kMoment && e.heldMs == 2 * kFrame, "ended %d after %llu ms",
           e.ended, e.heldMs);
}

void Turn() {
    Run r;
    r.Frames(5, false, 0.0f, 0.0f, true, false);
    const qte::Out o = r.Frame(false, 1.0f, 0.0f, true, false);
    r.Frames(10, false, 1.0f, 0.0f, true, false);
    const qte::Out e = r.Frame(false, 1.0f, 0.0f, false, false);  // (the turn over: 1.5 s without the game moving the look)
    Report("6a", "the game's turn of the look, no gun in hand as it began: taken", o.began && !o.cutscene && r.heldFrames == 11 && e.ended &&
                                                                                    e.why == qte::End::kMoment,
           "began %d, held frames %d, ended %d", o.began, r.heldFrames, e.ended);
    Run a;
    a.Frames(5, false, 0.0f, 1.0f, true, true);
    a.Frames(5, false, 0.0f, 0.0f, true, true);
    a.Frames(10, false, 1.0f, 1.0f, true, true);
    a.Frames(5, false, 0.0f, 0.0f, true, true);
    a.Frames(10, false, 1.0f, 1.0f, true, true);
    Report("6b", "... begun gun in hand: nothing held, said once", a.heldFrames == 0 && a.began == 0 && a.ignored == 1, "held frames %d, said %d", a.heldFrames,
           a.ignored);
    a.Frames(5, false, 0.0f, 0.0f);
    a.Frames(5, false, 0.0f, 0.0f, true, true);
    a.Frames(5, false, 1.0f, 0.0f, true, true);
    Report("6c", "... said again in the next such turn", a.ignored == 2 && a.heldFrames == 0, "said %d", a.ignored);
    Run c;
    c.Frames(5, true, 0.0f, 0.0f, true, true);
    c.Frames(5, true, 1.0f, 0.0f, true, true);
    Report("6d", "a cutscene's camera with such a turn: taken", c.began == 1 && c.heldFrames == 5 && c.ignored == 0, "presses %d", c.began);
    // A turn begun gun in hand going over into a cutscene's camera: a grip let go in the turn doesn't count as let go in the moment.
    Run t;
    t.Frames(5, false, 0.0f, 0.0f, true, true);
    t.Frame(true, 1.0f, 0.0f, true, true);
    Report("6e", "a grip let go before the moment, squeezed on its first frame: not fresh", t.began == 0 && t.heldFrames == 0, "presses %d", t.began);
}

void FrontAndOff() {
    Run r;
    r.Frames(5, true, 0.0f, 0.0f, false, false);
    for (int i = 0; i < 20; ++i)
        r.Frame(true, i < 5 ? 0.0f : 1.0f, 0.0f, false, false, false);  // (bf3 not in front)
    const bool none = r.heldFrames == 0;
    r.Frames(10, true, 1.0f, 0.0f);  // (in front again, the grip still squeezed: not fresh in this moment)
    Report("7a", "bf3 not in front: nothing held, nor by the same squeeze as it comes back", none && r.heldFrames == 0, "held frames %d", r.heldFrames);
    Run f;
    f.Frames(5, true, 0.0f, 0.0f);
    f.Frames(10, true, 1.0f, 0.0f);
    const qte::Out e = f.Frame(true, 1.0f, 0.0f, false, false, false);
    Report("7b", "a press under way let go as bf3 leaves the front", e.ended && !e.held && e.why == qte::End::kFront, "ended %d why %d", e.ended,
           static_cast<int>(e.why));
    Run o;
    for (int i = 0; i < 200; ++i)
        o.Frame(true, i % 20 < 10 ? 0.0f : 1.0f, i % 14 < 7 ? 0.0f : 1.0f, true, false, true, false);
    Report("7c", "\"qte 0\": nothing held, ever", o.heldFrames == 0 && o.began == 0, "held frames %d", o.heldFrames);
    Run s;
    s.Frames(5, true, 0.0f, 0.0f);
    s.Frames(10, true, 1.0f, 0.0f);
    const qte::Out off = s.Frame(true, 1.0f, 0.0f, false, false, true, false);
    Report("7d", "switched off with a press under way: let go", off.ended && off.why == qte::End::kOff && !off.held, "ended %d", off.ended);
    Run p;
    p.Frames(5, true, 0.0f, 0.0f);
    p.Frames(10, true, 1.0f, 0.0f);
    const qte::Out stop = qte::Stop(p.s, p.now, qte::End::kFront);
    const qte::Out again = qte::Stop(p.s, p.now, qte::End::kFront);
    p.Frames(10, true, 1.0f, 0.0f);
    Report("7e", "Stop from outside: the press ends once, the same squeeze presses nothing after",
           stop.ended && stop.heldMs == 10 * kFrame && !again.ended && p.began == 1 && !p.last.held, "ended %d after %llu ms, again %d", stop.ended, stop.heldMs,
           again.ended);
}

void Unread() {
    Run r;
    r.Frames(5, true, 0.0f, 0.0f);
    r.Frames(10, true, 1.0f, 0.0f);
    for (int i = 0; i < 20; ++i)
        r.Frame(true, 1.0f, 0.0f, false, false, true, true, false, true);  // (the left unread, its last value kept)
    const bool letGo = r.ended == 1 && !r.last.held;
    r.Frames(10, true, 1.0f, 0.0f);  // (read again, still squeezed)
    const bool noneYet = r.began == 1;
    r.Frames(2, true, 0.0f, 0.0f);
    r.Frames(5, true, 1.0f, 0.0f);
    Report("8a", "a grip not read: its press let go (past the 120 ms), and it must be seen let go again", letGo && noneYet && r.began == 2,
           "let go %d, pressed while unseen %d, presses in all %d", letGo, !noneYet, r.began);
    Run n;
    const float nan = std::numeric_limits<float>::quiet_NaN();
    n.Frames(5, true, 0.0f, 0.0f);
    n.Frames(20, true, nan, nan);
    n.Frames(2, true, 0.0f, 0.0f);
    n.Frames(20, true, std::numeric_limits<float>::infinity(), 0.0f);
    Report("8b", "a squeeze not a number presses nothing; an endless one, after a let-go, is a squeeze", n.began == 1 && n.heldFrames == 20, "presses %d, held frames %d", n.began,
           n.heldFrames);
}

void ClockAndTime() {
    Run r;
    r.Frames(5, true, 0.0f, 0.0f);
    r.Frame(true, 1.0f, 0.0f);
    r.now -= 100000;  // (the clock gone back)
    const qte::Out e = r.Frame(true, 0.0f, 0.0f);
    Report("9a", "a clock going back: let go rather than held on", e.ended && !e.held && e.heldMs == 0, "ended %d, %llu ms", e.ended, e.heldMs);
    LARGE_INTEGER f, a, b;
    QueryPerformanceFrequency(&f);
    Run t;
    constexpr int kCalls = 2000000;
    QueryPerformanceCounter(&a);
    for (int i = 0; i < kCalls; ++i)
        t.Frame(i % 3000 < 1500, i % 40 < 20 ? 0.0f : 1.0f, i % 56 < 28 ? 0.0f : 1.0f, i % 5000 < 100, i % 10000 < 100);
    QueryPerformanceCounter(&b);
    const double ns = 1e9 * static_cast<double>(b.QuadPart - a.QuadPart) / static_cast<double>(f.QuadPart) / kCalls;
    Report("9b", "the time: well under a microsecond a frame", ns < 1000.0 && t.began > 0, "%.0f ns a call (%d presses)", ns, t.began);
    Run k;
    k.Frames(5, true, 0.0f, 0.0f);
    const bool none = k.last.keys == 0;
    k.Frames(5, true, 1.0f, 0.0f);
    const unsigned held = k.last.keys;
    k.Frame(false, 1.0f, 0.0f);
    Report("9c", "version 1's keys: all seven while held, none otherwise", none && held == qte::kKeysV1 && k.last.keys == 0, "held %02X, after %02X", held,
           k.last.keys);
}

// ---------------------------------------------------------------- version 2 ----------------------------------------------------------------

constexpr unsigned kE = qte::kKeyE, kLmb = qte::kKeyLeftMouse, kRmb = qte::kKeyRightMouse, kF = qte::kKeyF, kSpace = qte::kKeySpace, kZ = qte::kKeyZ;

int Bit(unsigned key) {
    int b = 0;
    while (b < 31 && !(key >> b & 1))
        ++b;
    return b;
}

// One run of version 2: a state, a clock, the awaited keys, and what the frames gave (each key's presses and how long it was down and up).
struct Run2 {
    qte::State2 s;
    unsigned long long now = 100000;
    unsigned await = 0, hold = 0;
    bool mash = true, front = true, on = true;
    // As input.cpp's QteStep gives them: a fight's prompt up, a press prompt (one tap), a frame the fights couldn't be read; a scripted moment (a
    // cutscene's camera). All off: a single quick-time event in play, the rules as they were before 2026-11-19 (a fresh squeeze alone).
    bool prompt = false, once = false, unknown = false, scripted = false;
    unsigned quick = 0;  // (of await, a single's keys by a quick-time action proper: qtelist::QuickKeys, as qtewatch.cpp gives them)
    unsigned gates = 0;  // (a fight's prompt up: the keys of its gates read open, right or wrong)
    bool holds[2] = {};  // (each hand carrying something, hands.cpp's last frame: in play it never answers; 30)
    // The game's input as the mod reads it: 0 never shows a key, 1 the keys of two frames before, 2 a key once down shown ever after, 3 what the
    // simulated game's last input tick took (external).
    int game = 1;
    unsigned external = 0;
    bool seenRead = true;  // (that input state read this frame: a tap never shown with it read throughout leaves a press prompt to another press)
    unsigned sent[2] = {};
    int began = 0, ended = 0, heldFrames = 0, settled = 0, settledSeen = 0;
    unsigned ever = 0, keys = 0;
    int edges[qte::kKeyCount] = {}, downFrames[qte::kKeyCount] = {};
    unsigned long long wentDown[qte::kKeyCount] = {}, wentUp[qte::kKeyCount] = {};
    unsigned long long shortestDown = ~0ull, longestDown = 0, shortestUp = ~0ull, firstEdge = 0, lastEdge = 0;
    unsigned long long longestSeenMs = 0;
    qte::Out last;
    qte::In lastIn;
    int refused = 0, byHold = 0;  // squeezes refused (a press prompt's one made), presses begun by a grip squeezed already
    int refusedHeld = 0, again = 0, cut = 0;  // ... refused a grip held on (no squeeze); a press prompt pressed again (its tap before never shown); taps cut
    qte::End lastWhy = qte::End::kNone;
    unsigned lastTaps = 0, lastSeen = 0, lastAwaited = 0, lastHolds = 0;
    unsigned long long lastHeldMs = 0;
    int lastHand = -1;

    void Note(const qte::Out& o) {
        const unsigned k = o.keys;
        for (int b = 0; b < qte::kKeyCount; ++b) {
            const unsigned bit = 1u << b;
            if ((k & bit) && !(keys & bit)) {
                ++edges[b];
                if (wentUp[b] && now >= wentUp[b] && now - wentUp[b] < shortestUp)
                    shortestUp = now - wentUp[b];
                wentDown[b] = now;
                if (!firstEdge)
                    firstEdge = now;
                lastEdge = now;
            }
            if (!(k & bit) && (keys & bit)) {
                const unsigned long long d = now >= wentDown[b] ? now - wentDown[b] : 0;
                if (d < shortestDown)
                    shortestDown = d;
                if (d > longestDown)
                    longestDown = d;
                wentUp[b] = now;
            }
            downFrames[b] += (k & bit) != 0;
        }
        keys = k;
        ever |= k;
        began += o.began;
        ended += o.ended;
        heldFrames += o.held;
        settled += o.tapDone;
        settledSeen += o.tapDone && o.tapSeen;
        if (o.tapDone && o.tapSeen && o.tapMs > longestSeenMs)
            longestSeenMs = o.tapMs;
        if (o.began)
            lastHand = o.hand, lastAwaited = o.awaited, lastHolds = o.holds;
        if (o.ended)
            lastWhy = o.why, lastTaps = o.taps, lastSeen = o.seenTaps, lastHeldMs = o.heldMs;
        refused += o.refused;
        byHold += o.began && o.byHold;
        refusedHeld += o.refused && o.refusedHeld;
        again += o.began && o.again;
        cut += o.tapDone && o.tapCut;
    }
    qte::Out Frame(float left, float right, bool readLeft = true, bool readRight = true) {
        qte::In in;
        in.nowMs = now;
        in.on = on;
        in.front = front;
        in.read[0] = readLeft, in.read[1] = readRight;
        in.squeeze[0] = left, in.squeeze[1] = right;
        in.await = await;
        in.hold = hold;
        in.mash = mash;
        in.prompt = prompt;
        in.once = once;
        in.unknown = unknown;
        in.quick = quick;
        in.gates = gates;
        in.holds[0] = holds[0], in.holds[1] = holds[1];
        in.cutscene = scripted;
        in.seen = !seenRead ? 0u : game == 1 ? sent[1] : game == 2 ? ever : game == 3 ? external : 0u;
        in.seenRead = seenRead;
        lastIn = in;
        last = qte::Step2(s, in);
        Note(last);
        sent[1] = sent[0];
        sent[0] = last.keys;
        now += kFrame;
        return last;
    }
    void Frames(int n, float left, float right) {
        for (int i = 0; i < n; ++i)
            Frame(left, right);
    }
    qte::Out Stop(qte::End why) {
        last = qte::Stop2(s, now, why);
        Note(last);
        return last;
    }
};

void Exact() {
    bool whole = true, only = true;
    int presses = 0;
    for (unsigned mask = 1; mask <= qte::kKeysV2; ++mask) {
        Run2 r;
        r.await = mask;
        r.Frames(3, 0.0f, 0.0f);
        r.Frames(3, 0.0f, 1.0f);
        whole = whole && r.last.keys == mask && r.lastAwaited == mask;
        r.Frames(60, 0.0f, 1.0f);
        r.Frames(10, 0.0f, 0.0f);
        only = only && r.ever == mask && r.keys == 0;
        presses += r.began;
    }
    Report("10a", "each of the 63 masks: a fresh squeeze presses exactly its keys, nothing else", whole && only && presses == 63,
           "whole %d, nothing else %d, presses %d", whole, only, presses);
    Run2 c;
    c.await = 0xFFFFFFFFu;
    c.Frames(3, 0.0f, 0.0f);
    c.Frames(30, 1.0f, 1.0f);
    Run2 d;
    d.await = qte::kKeyCtrl | 0xFFFFFF00u;
    d.Frames(3, 0.0f, 0.0f);
    d.Frames(30, 1.0f, 1.0f);
    Report("10b", "Left Ctrl and bits that are no key: never pressed", c.ever == qte::kKeysV2 && d.ever == 0 && d.began == 0, "all bits awaited %02X, Ctrl alone %02X",
           c.ever, d.ever);
    Run2 n;
    for (int i = 0; i < 10000; ++i)
        n.Frame(0.5f + 0.5f * std::sin(i * 0.05f), i % 700 < 650 ? 1.0f : 0.0f);
    Report("10c", "nothing awaited: nothing pressed in 10 000 frames of both grips", n.ever == 0 && n.began == 0 && n.heldFrames == 0, "keys %02X, presses %d", n.ever,
           n.began);
    Run2 f;
    f.Frames(50, 0.0f, 1.0f);  // (play: the right grip holds the gun, nothing awaited)
    f.await = kSpace;
    f.Frames(200, 0.0f, 1.0f);  // (a key comes to be awaited with it squeezed)
    const bool none = f.ever == 0 && f.began == 0;
    f.Frames(3, 0.0f, 0.5f);  // (eased, not let go)
    f.Frames(10, 0.0f, 1.0f);
    const bool eased = f.began == 0;
    f.Frames(3, 0.0f, 0.2f);
    f.Frames(10, 0.0f, 1.0f);
    Report("10d", "a grip squeezed as a key comes to be awaited presses nothing until seen let go", none && eased && f.began == 1 && f.ever == kSpace,
           "as it began %d, eased %d, after a let-go: presses %d", !none, !eased, f.began);
    Run2 l;
    l.await = kLmb;
    l.Frames(50, 0.0f, 1.0f);  // (the right one holding on, not fresh)
    l.Frames(5, 1.0f, 1.0f);
    const bool left = l.began == 1 && l.lastHand == 0;
    l.Frames(20, 0.0f, 1.0f);
    const bool over = l.ended == 1 && l.keys == 0;
    Run2 b;
    b.await = kSpace;
    b.Frames(5, 0.0f, 0.0f);
    b.Frames(20, 1.0f, 0.0f);
    b.Frames(20, 1.0f, 1.0f);
    b.Frames(20, 0.0f, 1.0f);  // (the left let go, the right still on it)
    const bool one = b.began == 1 && b.ended == 0 && b.last.held;
    b.Frames(10, 0.0f, 0.0f);
    Report("10e", "either grip presses (the other, held on from play, doesn't keep it); both: one press till the last is let go",
           left && over && one && b.ended == 1 && b.lastHand == 0, "left %d, over %d, both one press %d, ends %d", left, over, one, b.ended);
}

void Taps() {
    {
        Run2 r;
        r.await = kSpace;
        r.Frames(5, 0.0f, 0.0f);
        r.Frame(1.0f, 0.0f);
        r.Frames(20, 0.0f, 0.0f);
        Report("11a", "a flick a frame long: one tap, down 70 ms", r.edges[Bit(kSpace)] == 1 && r.shortestDown >= qte::kTap && r.shortestDown < qte::kTap + 2 * kFrame &&
                                                                          r.ended == 1 && r.lastWhy == qte::End::kGrip && r.lastTaps == 1,
               "taps %d, down %llu ms, ended %d", r.edges[Bit(kSpace)], r.shortestDown, r.ended);
    }
    {
        Run2 r;
        r.await = kSpace;
        r.Frames(5, 0.0f, 0.0f);
        r.Frames(286, 0.0f, 1.0f);  // (4 s)
        r.Frames(10, 0.0f, 0.0f);
        const int n = r.edges[Bit(kSpace)];
        const double rate = n > 1 ? (n - 1) * 1000.0 / static_cast<double>(r.lastEdge - r.firstEdge) : 0.0;
        Report("11b", "the grip held 4 s: a tap every 130 ms (7.7 a second), each down 70 ms, up 60",
               n >= 30 && n <= 32 && r.shortestDown >= qte::kTap - kFrame && r.longestDown <= qte::kTap + kFrame && r.shortestUp >= qte::kGap - kFrame - 4 &&
                   rate > 7.5 && rate < 7.9 && r.began == 1 && r.lastTaps == static_cast<unsigned>(n) && r.ever == kSpace,
               "%d taps, %.2f a second, down %llu-%llu ms, up %llu ms at the least", n, rate, r.shortestDown, r.longestDown, r.shortestUp);
    }
    {
        Run2 r;
        r.await = kSpace;
        r.mash = false;
        r.Frames(5, 0.0f, 0.0f);
        r.Frames(286, 0.0f, 1.0f);
        const bool once = r.edges[Bit(kSpace)] == 1 && r.shortestDown == 5 * kFrame && r.keys == 0 && r.last.held;
        r.Frames(5, 0.0f, 0.0f);
        r.Frames(20, 0.0f, 1.0f);
        Report("11c", "\"qtemash 0\": one tap a squeeze, however long; the next squeeze the next", once && r.edges[Bit(kSpace)] == 2 && r.began == 2,
               "one in 4 s %d, taps in all %d, presses %d", once, r.edges[Bit(kSpace)], r.began);
    }
    {
        Run2 r;
        r.await = kSpace;
        r.Frames(5, 0.0f, 0.0f);
        r.Frames(2, 0.0f, 1.0f);
        r.Frames(20, 0.0f, 0.0f);  // (let go 28 ms into the tap)
        const bool finished = r.edges[Bit(kSpace)] == 1 && r.shortestDown == 5 * kFrame && r.ended == 1;
        Run2 u;
        u.await = kSpace;
        u.Frames(5, 0.0f, 0.0f);
        u.Frames(7, 0.0f, 1.0f);  // (84 ms: the key up since 70)
        const bool up = u.keys == 0 && u.ended == 0;
        const qte::Out e = u.Frame(0.0f, 0.0f);
        Report("11d", "let go in a tap: the tap finished; let go between taps: over that frame", finished && up && e.ended && e.why == qte::End::kGrip && u.edges[Bit(kSpace)] == 1,
               "finished %d (down %llu ms), up between %d, over at once %d", finished, r.shortestDown, up, e.ended);
    }
    {
        Run2 r;
        r.await = kLmb;
        r.mash = false;
        r.Frames(5, 0.0f, 0.0f);
        int squeezes = 0;
        for (int i = 0; i < 286; ++i) {  // (the player's own mash, 5 a second)
            squeezes += i % 14 == 0;
            r.Frame(0.0f, i % 14 < 7 ? 1.0f : 0.0f);
        }
        r.Frames(10, 0.0f, 0.0f);
        Run2 m;
        m.await = kLmb;
        m.Frames(5, 0.0f, 0.0f);
        for (int i = 0; i < 286; ++i)
            m.Frame(0.0f, i % 14 < 7 ? 1.0f : 0.0f);
        m.Frames(10, 0.0f, 0.0f);
        Report("11e", "the player's own mash, 5 squeezes a second: a tap each, with the mash on or off",
               r.edges[Bit(kLmb)] == squeezes && r.began == squeezes && m.edges[Bit(kLmb)] == squeezes && r.shortestUp >= qte::kGap - kFrame && r.ever == kLmb,
               "%d squeezes: %d taps (mash off), %d (on); up %llu ms at the least", squeezes, r.edges[Bit(kLmb)], m.edges[Bit(kLmb)], r.shortestUp);
    }
    {
        Run2 r;
        r.await = kSpace;
        r.Frames(5, 0.0f, 0.0f);
        r.Frames(2, 0.0f, 1.0f);
        r.Frames(2, 0.0f, 0.0f);
        r.Frames(2, 0.0f, 1.0f);  // (squeezed again 56 ms after the first, its tap still down)
        r.Frames(30, 0.0f, 0.0f);
        Report("11f", "two quick squeezes: two taps, the key up 60 ms between (never one press)",
               r.edges[Bit(kSpace)] == 2 && r.began == 2 && r.ended == 2 && r.shortestUp >= qte::kGap - kFrame && r.shortestDown >= qte::kTap - kFrame,
               "taps %d, presses %d, up %llu ms between, down %llu ms at the least", r.edges[Bit(kSpace)], r.began, r.shortestUp, r.shortestDown);
    }
    {
        // A 94 ms frame gap from inside the first tap's down time (56 ms) into the second's (150 ms): the key goes up for that frame and the
        // second tap goes down on the next, so each tap counted is a press of its own (the review of 2026-11-18: they ran into one).
        Run2 r;
        r.await = kSpace;
        r.Frames(5, 0.0f, 0.0f);
        r.Frames(5, 0.0f, 1.0f);  // (frames 0, 14, 28, 42 and 56 ms into the press: the first tap down)
        r.now += 94 - kFrame;     // (the next frame 150 ms in: the second cycle's down time)
        const qte::Out gap = r.Frame(0.0f, 1.0f);
        r.Frames(60, 0.0f, 1.0f);
        r.Frames(20, 0.0f, 0.0f);
        Report("11g", "a 94 ms frame gap across two taps' down times: up for a frame, then the next tap (each tap a press of its own)",
               (gap.keys & kSpace) == 0 && r.edges[Bit(kSpace)] == static_cast<int>(r.lastTaps) && r.lastTaps >= 6 && r.ended == 1,
               "the gap's frame %s, %d presses for %u taps counted", gap.keys & kSpace ? "down" : "up", r.edges[Bit(kSpace)], r.lastTaps);
    }
}

void Holds() {
    Run2 r;
    r.await = r.hold = kE;
    r.Frames(5, 0.0f, 0.0f);
    r.Frames(50, 0.0f, 1.0f);
    const qte::Out e = r.Frame(0.0f, 0.0f);
    Run2 f;
    f.await = f.hold = kE;
    f.Frames(5, 0.0f, 0.0f);
    f.Frame(1.0f, 0.0f);
    f.Frames(20, 0.0f, 0.0f);
    Report("12a", "a held key: down while squeezed, up as it's let go; a flick 120 ms",
           r.edges[Bit(kE)] == 1 && r.downFrames[Bit(kE)] == 50 && e.ended && e.why == qte::End::kGrip && r.lastHolds == kE && f.edges[Bit(kE)] == 1 &&
               f.shortestDown >= qte::kShortest && f.shortestDown < qte::kShortest + 2 * kFrame,
           "down %d of 50 frames in %d press, a flick %llu ms", r.downFrames[Bit(kE)], r.edges[Bit(kE)], f.shortestDown);
    Run2 b;
    b.await = kE | kSpace;
    b.hold = kE;
    b.Frames(5, 0.0f, 0.0f);
    b.Frames(143, 1.0f, 0.0f);  // (2 s)
    Report("12b", "a held key and a tapped one together: E down throughout, Space tapped",
           b.edges[Bit(kE)] == 1 && b.downFrames[Bit(kE)] == 143 && b.edges[Bit(kSpace)] >= 15 && b.edges[Bit(kSpace)] <= 16 && b.lastAwaited == (kE | kSpace) &&
               b.lastHolds == kE && b.ever == (kE | kSpace),
           "E down %d of 143 frames, Space %d taps", b.downFrames[Bit(kE)], b.edges[Bit(kSpace)]);
}

void Ends() {
    {
        Run2 r;
        r.await = kSpace;
        r.Frames(5, 0.0f, 0.0f);
        r.Frames(2, 0.0f, 1.0f);
        const bool down = r.keys == kSpace;
        r.await = 0;
        const qte::Out e = r.Frame(0.0f, 1.0f);
        r.await = kSpace;
        r.Frames(100, 0.0f, 1.0f);  // (awaited again, the grip never let go)
        const bool none = r.began == 1 && r.keys == 0;
        r.Frames(3, 0.0f, 0.0f);
        r.Frames(3, 0.0f, 1.0f);
        Report("13a", "nothing awaited any more: let go that frame, mid-tap; awaited again, a fresh squeeze is wanted",
               down && e.ended && e.why == qte::End::kAwait && e.keys == 0 && none && r.began == 2, "down %d, ended %d (why %d), held on %d, presses %d", down, e.ended,
               static_cast<int>(e.why), !none, r.began);
    }
    {
        Run2 r;
        r.await = kSpace | kLmb;
        r.Frames(5, 0.0f, 0.0f);
        r.Frames(2, 0.0f, 1.0f);
        const bool both = r.keys == (kSpace | kLmb);
        r.await = kSpace;
        const qte::Out o = r.Frame(0.0f, 1.0f);
        r.Frames(60, 0.0f, 1.0f);
        Report("13b", "one of two keys no longer awaited: it's let go at once, the other goes on",
               both && o.keys == kSpace && r.ended == 0 && r.edges[Bit(kLmb)] == 1 && r.edges[Bit(kSpace)] >= 6, "both %d, then %02X, LMB presses %d, Space %d", both,
               o.keys, r.edges[Bit(kLmb)], r.edges[Bit(kSpace)]);
    }
    {
        Run2 r;
        r.await = kSpace;
        r.Frames(5, 0.0f, 0.0f);
        r.Frames(2, 0.0f, 1.0f);
        r.await = kE;  // (another key instead, the grip held on)
        const qte::Out e = r.Frame(0.0f, 1.0f);
        r.Frames(100, 0.0f, 1.0f);
        const bool none = !(r.ever & kE) && r.began == 1;
        r.Frames(3, 0.0f, 0.0f);
        r.Frames(3, 0.0f, 1.0f);
        Report("13c", "another key awaited instead: the old one let go, the new one only by a fresh squeeze",
               e.ended && e.why == qte::End::kAwait && e.keys == 0 && none && r.began == 2 && r.keys == kE && r.ever == (kSpace | kE),
               "ended %d, E by the old squeeze %d, presses %d", e.ended, !none, r.began);
        // A key newly awaited beside the one being pressed waits for a fresh squeeze too.
        Run2 j;
        j.await = kSpace;
        j.Frames(5, 0.0f, 0.0f);
        j.Frames(2, 0.0f, 1.0f);
        j.await = kSpace | kF;
        j.Frames(60, 0.0f, 1.0f);
        const bool waits = !(j.ever & kF) && j.ended == 0;
        j.Frames(8, 0.0f, 0.0f);
        j.Frames(3, 0.0f, 1.0f);
        Report("13d", "a key newly awaited beside it: not pressed by the squeeze under way, pressed by the next", waits && j.keys == (kSpace | kF) && j.began == 2,
               "waited %d, then %02X", waits, j.keys);
    }
    {
        Run2 r;
        r.await = kLmb;
        r.Frames(5, 0.0f, 0.0f);
        r.Frames(2, 0.0f, 1.0f);
        r.front = false;
        const qte::Out e = r.Frame(0.0f, 1.0f);
        r.Frames(20, 0.0f, 1.0f);
        r.front = true;
        r.Frames(50, 0.0f, 1.0f);
        Run2 o;
        o.await = kLmb;
        o.Frames(5, 0.0f, 0.0f);
        o.Frames(2, 0.0f, 1.0f);
        o.on = false;
        const qte::Out off = o.Frame(0.0f, 1.0f);
        o.Frames(100, 1.0f, 1.0f);
        Report("13e", "bf3 not in front, or switched off: let go that frame; back in front, the same squeeze presses nothing",
               e.ended && e.why == qte::End::kFront && e.keys == 0 && r.began == 1 && r.edges[Bit(kLmb)] == 1 && off.ended && off.why == qte::End::kOff &&
                   off.keys == 0 && o.began == 1,
               "front: ended %d, presses %d; off: ended %d", e.ended, r.began, off.ended);
    }
    {
        Run2 r;
        r.await = kSpace;
        r.Frames(5, 0.0f, 0.0f);
        r.Frames(10, 1.0f, 0.0f);
        const qte::Out stop = r.Stop(qte::End::kFront);
        const qte::Out again = r.Stop(qte::End::kFront);
        r.Frames(50, 1.0f, 0.0f);
        Report("13f", "Stop2 from outside (the headset out of focus): the press ends once, the same squeeze presses nothing after",
               stop.ended && stop.keys == 0 && stop.heldMs == 10 * kFrame && !again.ended && r.began == 1 && r.keys == 0 && r.ended == 1,
               "ended %d after %llu ms, again %d, presses %d", stop.ended, stop.heldMs, again.ended, r.began);
    }
    {
        Run2 r;
        r.await = kSpace;
        r.Frames(5, 0.0f, 0.0f);
        r.Frames(2, 1.0f, 0.0f);
        for (int i = 0; i < 20; ++i)
            r.Frame(1.0f, 0.0f, false, true);  // (the left unread, its last value kept)
        const bool letGo = r.ended == 1 && r.keys == 0 && r.edges[Bit(kSpace)] == 1 && r.shortestDown == 5 * kFrame;
        r.Frames(10, 1.0f, 0.0f);  // (read again, still squeezed)
        const bool noneYet = r.began == 1;
        r.Frames(8, 0.0f, 0.0f);
        r.Frames(5, 1.0f, 0.0f);
        Run2 n;
        n.await = kSpace;
        const float nan = std::numeric_limits<float>::quiet_NaN();
        n.Frames(5, 0.0f, 0.0f);
        n.Frames(20, nan, nan);
        Report("13g", "a grip not read: its tap finished, no more, and it must be seen let go again; not a number: nothing",
               letGo && noneYet && r.began == 2 && n.began == 0 && n.ever == 0, "let go %d, pressed unseen %d, presses %d; NaN presses %d", letGo, !noneYet, r.began,
               n.began);
    }
    {
        Run2 r;
        r.await = kSpace;
        r.Frames(5, 0.0f, 0.0f);
        r.Frames(2, 0.0f, 1.0f);
        r.now -= 100000;
        const qte::Out e = r.Frame(0.0f, 1.0f);
        r.Frames(50, 0.0f, 1.0f);
        Report("13h", "a clock gone back: let go rather than held on", e.ended && e.keys == 0 && e.heldMs == 0 && r.began == 1 && r.keys == 0, "ended %d, %llu ms",
               e.ended, e.heldMs);
    }
}

void Seen() {
    Run2 r;
    r.await = kSpace;
    r.Frames(5, 0.0f, 0.0f);
    r.Frames(143, 0.0f, 1.0f);
    r.Frames(10, 0.0f, 0.0f);
    Report("16a", "the game's input showing each press two frames on: every one seen", r.lastTaps >= 15 && r.lastSeen == r.lastTaps &&
                                                                                          r.settled == static_cast<int>(r.lastTaps) && r.settledSeen == r.settled &&
                                                                                          r.longestSeenMs <= 3 * kFrame,
           "%u presses, %u seen, settled %d (%d seen), after %llu ms at the most", r.lastTaps, r.lastSeen, r.settled, r.settledSeen, r.longestSeenMs);
    Run2 n;
    n.game = 0;
    n.await = kSpace;
    n.Frames(5, 0.0f, 0.0f);
    n.Frames(143, 0.0f, 1.0f);
    n.Frames(10, 0.0f, 0.0f);
    Report("16b", "the game's input never showing it: every press settled as not seen", n.lastTaps >= 15 && n.lastSeen == 0 && n.settled == static_cast<int>(n.lastTaps) &&
                                                                                           n.settledSeen == 0,
           "%u presses, %u seen, settled %d", n.lastTaps, n.lastSeen, n.settled);
    Run2 m;
    m.game = 2;
    m.await = kSpace;
    m.Frames(5, 0.0f, 0.0f);
    m.Frames(143, 0.0f, 1.0f);
    m.Frames(10, 0.0f, 0.0f);
    Report("16c", "the game's input showing the key without a break (presses run into one): only the first seen", m.lastTaps >= 15 && m.lastSeen == 1,
           "%u presses, %u seen", m.lastTaps, m.lastSeen);
    Run2 h;
    h.await = h.hold = kE;
    h.Frames(5, 0.0f, 0.0f);
    h.Frames(50, 0.0f, 1.0f);
    h.Frames(5, 0.0f, 0.0f);
    Report("16d", "a held key: one press, seen once", h.lastTaps == 1 && h.lastSeen == 1 && h.settled == 1, "%u press, %u seen", h.lastTaps, h.lastSeen);
    // The trigger gone two frames into the press (the game took the answer): seen if its input shows it that frame; else too short to tell.
    Run2 w;
    w.await = kLmb;
    w.Frames(5, 0.0f, 0.0f);
    w.Frames(2, 0.0f, 1.0f);
    w.await = 0;
    const qte::Out won = w.Frame(0.0f, 1.0f);
    Run2 u;
    u.game = 0;
    u.await = kLmb;
    u.Frames(5, 0.0f, 0.0f);
    u.Frames(2, 0.0f, 1.0f);
    u.await = 0;
    const qte::Out cut = u.Frame(0.0f, 1.0f);
    Report("16e", "the trigger gone 28 ms into the press: seen if the game's input shows it that frame; if not, marked as cut short (no \"not seen\")",
           won.ended && won.tapDone && won.tapSeen == kLmb && !won.tapCut && won.seenTaps == 1 && cut.ended && cut.tapDone && !cut.tapSeen && cut.tapCut &&
               cut.tapMs == 2 * kFrame,
           "shown: seen %02X cut %d; not shown: seen %02X cut %d after %llu ms", won.tapSeen, won.tapCut, cut.tapSeen, cut.tapCut, cut.tapMs);
}

// A made-up image of the game's memory: pages by their number; a read of a page not there fails, as a guarded read of bad memory does.
struct Image {
    std::map<uint32_t, std::vector<uint8_t>> pages;
    mutable int reads = 0;
    uint32_t next = 0xE7100000u;  // (where the game's heap was, past 2 GB)

    uint8_t* Page(uint32_t at) {
        std::vector<uint8_t>& p = pages[at >> 12];
        if (p.empty())
            p.assign(4096, 0);
        return p.data();
    }
    void Put(uint32_t at, const void* data, size_t n) {
        const uint8_t* b = static_cast<const uint8_t*>(data);
        for (size_t i = 0; i < n; ++i)
            Page(at + static_cast<uint32_t>(i))[(at + i) & 4095] = b[i];
    }
    void W32(uint32_t at, uint32_t v) { Put(at, &v, 4); }
    void W8(uint32_t at, uint8_t v) { Put(at, &v, 1); }
    void Drop(uint32_t at) { pages.erase(at >> 12); }
    bool operator()(uint32_t at, void* out, size_t n) const {
        ++reads;
        uint8_t* o = static_cast<uint8_t*>(out);
        while (n) {
            const auto it = pages.find(at >> 12);
            if (it == pages.end())
                return false;
            const size_t off = at & 4095, take = n < 4096 - off ? n : 4096 - off;
            std::memcpy(o, it->second.data() + off, take);
            o += take;
            at += static_cast<uint32_t>(take);
            n -= take;
        }
        return true;
    }
};

constexpr uint32_t kBase = 0x400000;
const qtelist::Types kTypes = qtelist::TypesAt(kBase);
constexpr uint32_t kGlobal = kBase + qtelist::kManagerGlobal;
constexpr uint32_t kMgr = 0xE72867C0u;

// The bus the triggers made next are given (+0x0C): 0 (none, as 1-19 have them), or a level's bus (18 done again through the fights' reader).
uint32_t g_triggerBus = 0;

// A trigger entity and its data as the game has them (the entity 0x24 bytes, its data 0x40 on).
uint32_t Entity(Image& m, uint32_t action, bool sendEvents = true, int link = -1, uint32_t entityType = kTypes.entity, uint32_t dataType = kTypes.data) {
    const uint32_t at = m.next, data = at + 0x40;
    m.next += 0x100;
    for (uint32_t o = 0; o < 0x24; o += 4)
        m.W32(at + o, 0), m.W32(data + o, 0);
    m.W32(at, entityType);
    m.W32(at + 0x0C, g_triggerBus);
    m.W32(at + 0x10, data);
    if (link >= 0)
        m.W32(at + 0x18 + 4 * static_cast<uint32_t>(link), 0xE7000040u);
    const float one = 1.0f;
    m.W32(data, dataType);
    m.W32(data + 0x10, action);
    m.Put(data + 0x14, &one, 4);
    m.W8(data + 0x20, sendEvents ? 1 : 0);
    m.W8(data + 0x21, 0);
    return at;
}

using Pending = std::vector<std::pair<uint32_t, uint32_t>>;

// The manager with its enabled entities and its adds and removes not yet applied, and the global pointing at it.
void Manager(Image& m, const std::vector<uint32_t>& enabled, const Pending& pending = {}, uint32_t type = kTypes.manager, uint32_t at = kMgr) {
    const uint32_t array = at + 0x1000, list = at + 0x4000;
    for (uint32_t o = 0; o < 0x24; o += 4)
        m.W32(at + o, 0);
    m.W32(at, type);
    m.W32(array, 0);
    for (size_t i = 0; i < enabled.size(); ++i)
        m.W32(array + 4 * static_cast<uint32_t>(i), enabled[i]);
    m.W32(at + 0x0C, array);
    m.W32(at + 0x10, array + 4 * static_cast<uint32_t>(enabled.size()));
    m.W32(list, 0);
    for (size_t i = 0; i < pending.size(); ++i) {
        m.W32(list + 8 * static_cast<uint32_t>(i), pending[i].first);
        m.W32(list + 8 * static_cast<uint32_t>(i) + 4, pending[i].second);
    }
    m.W32(at + 0x1C, list);
    m.W32(at + 0x20, list + 8 * static_cast<uint32_t>(pending.size()));
    m.W32(kGlobal, at);
}

qtelist::List ReadList(const Image& m) {
    return qtelist::Read(m, kGlobal, kTypes);
}

void Gate() {
    Image m;
    const uint32_t yaw = Entity(m, 4), pitch = Entity(m, 5), fire = Entity(m, 0x07), jump = Entity(m, 0x0C), interact = Entity(m, 0x23);
    Manager(m, {yaw, pitch, fire, jump, interact});
    const qtelist::List l = ReadList(m);
    const qtelist::Keys play = qtelist::Awaited(l, false), scripted = qtelist::Awaited(l, true);
    Report("14a", "play's own Fire, Jump and Interact triggers: nothing awaited in play, their keys in a scripted moment",
           l.status == qtelist::Status::kOk && l.count == 5 && play.await == 0 && scripted.await == (kLmb | kSpace | kE) && scripted.hold == 0,
           "%d triggers; in play %02X, scripted %02X", l.count, play.await, scripted.await);
    Run2 r;
    for (int i = 0; i < 2000; ++i) {
        r.await = qtelist::Awaited(ReadList(m), false).await;
        r.Frame(i % 20 < 10 ? 0.0f : 1.0f, i % 14 < 7 ? 0.0f : 1.0f);
    }
    Report("14b", "an ordinary Fire trigger live in play: 2000 frames of squeezes never press the left mouse button (the gun never fires)",
           r.ever == 0 && r.began == 0, "keys %02X, presses %d", r.ever, r.began);
    bool always = true;
    static const struct { uint32_t action; unsigned key; } kSix[] = {{0x3C, kE}, {0x3D, kLmb}, {0x3E, kRmb}, {0x3F, kF}, {0x40, kSpace}, {0x41, kZ}};
    for (const auto& a : kSix) {
        Image one;
        Manager(one, {Entity(one, 4), Entity(one, 5), Entity(one, a.action)});
        const qtelist::List q = ReadList(one);
        always = always && qtelist::Awaited(q, false).await == a.key && qtelist::Awaited(q, true).await == a.key && qtelist::Awaited(q, false).hold == 0;
    }
    Report("14c", "the six quick-time actions (0x3C-0x41): each its own key, in play and in a scripted moment", always, "%d", always);
    int others = 0, answered = 0;
    for (uint32_t action = 0; action < 256; ++action) {
        const bool nine = (action >= 0x3C && action <= 0x41) || action == 0x07 || action == 0x0C || action == 0x23;
        Image one;
        Manager(one, {Entity(one, action)});
        const qtelist::List q = ReadList(one);
        if (!nine) {
            ++others;
            answered += qtelist::Awaited(q, true).await != 0 || q.count != 1;
        }
    }
    Report("14d", "every other action (yaw, pitch, cameras, the rest): in the list, answered with nothing", others == 247 && answered == 0, "%d others, %d answered",
           others, answered);
    Image s;
    Manager(s, {Entity(s, 4), Entity(s, 5), Entity(s, 0x0C)});
    Run2 e;
    e.await = qtelist::Awaited(ReadList(s), true).await;
    e.Frames(5, 0.0f, 0.0f);
    e.Frames(3, 0.0f, 1.0f);
    const bool down = e.keys == kSpace;
    e.await = qtelist::Awaited(ReadList(s), false).await;  // (the scripted moment over, the trigger still live)
    const qtelist::List still = ReadList(s);
    const qte::Out over = e.Frame(0.0f, 1.0f);
    e.Frames(100, 0.0f, 1.0f);
    Report("14e", "the scripted moment over with play's Jump trigger still live: let go that frame, nothing after",
           down && still.count == 3 && over.ended && over.why == qte::End::kAwait && over.keys == 0 && e.began == 1 && e.keys == 0, "down %d, ended %d, presses %d",
           down, over.ended, e.began);
}

void Fallback() {
    Image m;
    const uint32_t climb = Entity(m, 0x40);
    Manager(m, {climb});
    const bool works = qtelist::Awaited(ReadList(m), false).await == kSpace;
    // The manager's pointer not found (no address), not readable, or leading to something else: nothing awaited.
    const qtelist::List noGlobal = qtelist::Read(m, 0, kTypes);
    const qtelist::List badGlobal = qtelist::Read(m, 0x00D00000u, kTypes);
    Manager(m, {climb}, {}, kTypes.manager + 0x10);
    const qtelist::List wrong = ReadList(m);
    Run2 r;
    for (int i = 0; i < 2000; ++i) {
        const qtelist::List& l = i % 3 == 0 ? noGlobal : i % 3 == 1 ? badGlobal : wrong;
        r.await = qtelist::Awaited(l, true).await;
        r.hold = qtelist::Awaited(l, true).hold;
        r.Frame(i % 20 < 10 ? 0.0f : 1.0f, i % 14 < 7 ? 0.0f : 1.0f);
    }
    Report("15a", "the manager not found, not readable or not the manager: nothing awaited, 2000 frames of squeezes press nothing",
           works && noGlobal.status == qtelist::Status::kUnreadable && badGlobal.status == qtelist::Status::kUnreadable &&
               wrong.status == qtelist::Status::kWrongType && wrong.count == 0 && r.ever == 0 && r.began == 0,
           "readable first %d; statuses %d %d %d; keys %02X, presses %d", works, static_cast<int>(noGlobal.status), static_cast<int>(badGlobal.status),
           static_cast<int>(wrong.status), r.ever, r.began);
}

void Reader() {
    {
        Image m;
        const uint32_t yaw = Entity(m, 4), pitch = Entity(m, 5), fire = Entity(m, 0x3D), climb = Entity(m, 0x40), jump = Entity(m, 0x0C);
        Manager(m, {yaw, pitch});
        const qtelist::List idle = ReadList(m);
        Manager(m, {yaw, pitch, fire});
        const qtelist::List grapple = ReadList(m);
        Manager(m, {yaw, pitch, climb, jump});
        const qtelist::List hanging = ReadList(m);
        Manager(m, {});
        const qtelist::List empty = ReadList(m);
        Report("17a", "the train's lists: yaw and pitch alone nothing; the grapple the left mouse button; the hanging Space; an empty list",
               idle.status == qtelist::Status::kOk && idle.count == 2 && qtelist::Awaited(idle, true).await == 0 && grapple.count == 3 &&
                   grapple.at[2].action == 0x3D && !grapple.at[2].hold && !grapple.at[2].pending && grapple.manager == kMgr &&
                   qtelist::Awaited(grapple, false).await == kLmb && hanging.count == 4 && qtelist::Awaited(hanging, false).await == kSpace &&
                   qtelist::Awaited(hanging, true).await == kSpace && empty.status == qtelist::Status::kOk && empty.count == 0 && !grapple.refused &&
                   !grapple.twice && !grapple.capped,
               "idle %d, grapple %d (%02X), hanging %d (%02X), empty %d", idle.count, grapple.count, qtelist::Awaited(grapple, false).await, hanging.count,
               qtelist::Awaited(hanging, false).await, empty.count);
        // Adds and removes not yet applied.
        Manager(m, {yaw, pitch}, {{fire, 0}});
        const qtelist::List add = ReadList(m);
        Manager(m, {yaw, pitch, fire}, {{fire, 1}});
        const qtelist::List remove = ReadList(m);
        Manager(m, {yaw, pitch}, {{fire, 0}, {fire, 1}});
        const qtelist::List both = ReadList(m);
        Manager(m, {yaw, pitch, fire}, {{fire, 1}, {fire, 0}});
        const qtelist::List back = ReadList(m);
        Manager(m, {yaw, pitch, fire}, {{climb, 1}});
        const qtelist::List notThere = ReadList(m);
        Manager(m, {yaw, pitch}, {{fire, 7}});
        const qtelist::List neither = ReadList(m);
        Manager(m, {yaw, pitch, fire}, {{fire, 0}});
        const qtelist::List already = ReadList(m);
        Manager(m, {yaw, pitch}, {{climb, 0}, {jump, 0}});
        const qtelist::List two = ReadList(m);
        // The flag's byte alone counts: an add and a remove with other bytes above them (if the game writes the flag as a byte).
        Manager(m, {yaw, pitch}, {{fire, 0xCDCDCD00u}});
        const qtelist::List dirtyAdd = ReadList(m);
        Manager(m, {yaw, pitch, fire}, {{fire, 0xCDCDCD01u}});
        const qtelist::List dirtyRemove = ReadList(m);
        Report("17b", "adds not yet applied count, removes are out: one add, one remove, both, a remove then an add, a remove of one not there, an entry that is "
                      "neither, an add of one enabled, two adds, an add and a remove with other bytes above the flag",
               add.count == 3 && add.at[2].pending && qtelist::Awaited(add, false).await == kLmb && remove.count == 2 && qtelist::Awaited(remove, false).await == 0 &&
                   both.count == 2 && back.count == 3 && back.at[2].pending && notThere.count == 3 && !notThere.refused && neither.count == 2 &&
                   neither.refused == 1 && already.count == 3 && already.twice == 1 && !already.at[2].pending && two.count == 4 &&
                   qtelist::Awaited(two, false).await == kSpace && dirtyAdd.count == 3 && !dirtyAdd.refused && qtelist::Awaited(dirtyAdd, false).await == kLmb &&
                   dirtyRemove.count == 2 && !dirtyRemove.refused,
               "%d %d %d %d %d %d %d %d, dirty %d %d", add.count, remove.count, both.count, back.count, notThere.count, neither.count, already.count, two.count,
               dirtyAdd.count, dirtyRemove.count);
    }
    {
        Image m;
        const uint32_t yaw = Entity(m, 4), climb = Entity(m, 0x40);
        const uint32_t badEntity = Entity(m, 0x3D, true, -1, kTypes.entity + 4);
        const uint32_t badData = Entity(m, 0x3E, true, -1, kTypes.entity, kTypes.data - 4);
        const uint32_t noData = Entity(m, 0x3F);
        m.W32(noData + 0x10, 0);
        const uint32_t lostData = Entity(m, 0x41);
        m.W32(lostData + 0x10, 0xD0000000u);  // (its data nowhere readable)
        const uint32_t bigAction = Entity(m, 0x1234);
        Manager(m, {yaw, badEntity, badData, noData, lostData, bigAction, 0, 3, 0x1234, 0xD0000000u, 0xFFFFFFF0u, climb});
        const qtelist::List l = ReadList(m);
        Report("17c", "a wrong entity vtable, a wrong data vtable, no data, unreadable data, an action out of range, null, odd, low, unreadable and wild pointers: "
                      "each left out, the good ones kept",
               l.status == qtelist::Status::kOk && l.count == 2 && l.refused == 10 && l.at[0].entity == yaw && l.at[1].entity == climb &&
                   qtelist::Awaited(l, false).await == kSpace,
               "kept %d, left out %d, awaited %02X", l.count, l.refused, qtelist::Awaited(l, false).await);
    }
    {
        Image m;
        const uint32_t climb = Entity(m, 0x40);
        Manager(m, {climb}, {}, kTypes.entity);  // (an entity where the manager should be)
        const qtelist::List wrong = ReadList(m);
        m.W32(kGlobal, 0);
        const qtelist::List none = ReadList(m);
        m.W32(kGlobal, 0xD0000000u);
        const qtelist::List lost = ReadList(m);
        m.W32(kGlobal, kMgr + 1);
        const qtelist::List odd = ReadList(m);
        Manager(m, {climb});
        m.W32(kMgr + 0x0C, 0xD0000000u);
        m.W32(kMgr + 0x10, 0xD0000008u);
        const qtelist::List lostArray = ReadList(m);
        Manager(m, {climb});
        m.W32(kMgr + 0x1C, 0xD0000000u);
        m.W32(kMgr + 0x20, 0xD0000008u);
        const qtelist::List lostPending = ReadList(m);
        Image blank;
        const qtelist::List noPage = ReadList(blank);
        using S = qtelist::Status;
        Report("17d", "a wrong manager vtable, a null manager, an unreadable one, an odd pointer, unreadable arrays, an unreadable global: nothing awaited, each named",
               wrong.status == S::kWrongType && wrong.count == 0 && none.status == S::kNone && lost.status == S::kUnreadable && odd.status == S::kUnreadable &&
                   lostArray.status == S::kUnreadable && lostPending.status == S::kUnreadable && noPage.status == S::kUnreadable &&
                   !qtelist::Awaited(wrong, true).await && !qtelist::Awaited(none, true).await && !qtelist::Awaited(lost, true).await &&
                   !qtelist::Awaited(lostArray, true).await && !qtelist::Awaited(lostPending, true).await && !qtelist::Awaited(noPage, true).await,
               "statuses %d %d %d %d %d %d %d", static_cast<int>(wrong.status), static_cast<int>(none.status), static_cast<int>(lost.status),
               static_cast<int>(odd.status), static_cast<int>(lostArray.status), static_cast<int>(lostPending.status), static_cast<int>(noPage.status));
    }
    {
        // Lists that go round or never end.
        Image m;
        const uint32_t climb = Entity(m, 0x40);
        Manager(m, std::vector<uint32_t>(32, climb));
        const qtelist::List same = ReadList(m);
        std::vector<uint32_t> many;
        Pending manyPending;
        for (int i = 0; i < 1000; ++i)
            many.push_back(Entity(m, 0x3C + i % 6));
        for (int i = 0; i < 1000; ++i)
            manyPending.push_back({Entity(m, 0x3C), 0});
        Manager(m, many, manyPending);
        m.reads = 0;
        const qtelist::List endless = ReadList(m);
        const int reads = m.reads;
        Manager(m, {climb});
        m.W32(kMgr + 0x10, kMgr + 0x1000 - 8);  // (the end before the beginning)
        const qtelist::List backwards = ReadList(m);
        Manager(m, {climb});
        m.W32(kMgr + 0x10, kMgr + 0x1000 + 6);  // (not a whole number of pointers)
        const qtelist::List ragged = ReadList(m);
        Manager(m, {climb});
        m.W32(kMgr + 0x0C, 0);  // (no beginning, an end)
        const qtelist::List headless = ReadList(m);
        Manager(m, {climb});
        m.W32(kMgr + 0x0C, kMgr);  // (the list is the manager itself)
        m.W32(kMgr + 0x10, kMgr + 0x24);
        const qtelist::List itself = ReadList(m);
        Manager(m, {climb});
        m.W32(kMgr + 0x1000, kMgr + 0x1000);  // (its entry points at the list)
        const qtelist::List inward = ReadList(m);
        const uint32_t loop = Entity(m, 0x40);
        m.W32(loop + 0x10, loop);  // (its data is itself)
        Manager(m, {loop});
        const qtelist::List selfData = ReadList(m);
        Manager(m, {climb}, {{climb, 1}});
        m.W32(kMgr + 0x20, kMgr + 0x4000 - 16);  // (the adds and removes: the end before the beginning)
        const qtelist::List pendingBackwards = ReadList(m);
        using S = qtelist::Status;
        Report("17e", "an entity listed 32 times: once; 1000 entities and 1000 adds: 32 and 16 walked, the reads bounded; bounds that make no sense; a list that is "
                      "the manager, an entry that is the list, data that is its entity: no fault, nothing wrong pressed",
               same.count == 1 && same.twice == 31 && endless.status == S::kOk && endless.count == qtelist::kMost + qtelist::kMostPending && endless.capped &&
                   reads <= 4 + 2 * (qtelist::kMost + qtelist::kMostPending) && backwards.status == S::kOk && backwards.count == 0 && backwards.capped &&
                   ragged.count == 0 && ragged.capped && headless.count == 0 && headless.capped && itself.count == 0 && itself.refused > 0 && inward.count == 0 &&
                   inward.refused == 1 && selfData.count == 0 && selfData.refused == 1 && pendingBackwards.count == 1 && pendingBackwards.capped,
               "same %d (again %d), endless %d in %d reads, backwards %d, ragged %d, headless %d, itself %d, inward %d, own data %d", same.count, same.twice,
               endless.count, reads, backwards.count, ragged.count, headless.count, itself.count, inward.count, selfData.count);
    }
    {
        Image m;
        const uint32_t edge = Entity(m, 0x40), link0 = Entity(m, 0x3C, true, 0), link1 = Entity(m, 0x3D, true, 1), link2 = Entity(m, 0x3E, true, 2),
                       level = Entity(m, 0x3F, false), jumpHeld = Entity(m, 0x0C, false);
        Manager(m, {edge, link0, link1, link2, level, jumpHeld});
        const qtelist::List l = ReadList(m);
        const qtelist::Keys play = qtelist::Awaited(l, false), scripted = qtelist::Awaited(l, true);
        Report("17f", "a trigger with a property link or without trigger events is held, the rest tapped; a key is held if any of its live triggers is",
               l.count == 6 && !l.at[0].hold && l.at[1].hold && l.at[2].hold && l.at[3].hold && l.at[4].hold && play.await == (kSpace | kE | kLmb | kRmb | kF) &&
                   play.hold == (kE | kLmb | kRmb | kF) && scripted.hold == (kE | kLmb | kRmb | kF | kSpace),
               "held in play %02X of %02X, scripted %02X", play.hold, play.await, scripted.hold);
    }
    {
        // A read torn by the game changing its list on its own thread.
        Image m;
        const uint32_t climb = Entity(m, 0x40);
        Manager(m, {climb});
        const qtelist::List good = ReadList(m);
        Manager(m, {climb}, {}, kTypes.manager + 4);
        const qtelist::List wrong = ReadList(m);
        Manager(m, {climb, 0x1234});
        const qtelist::List refused = ReadList(m);
        m.W32(kGlobal, 0);
        const qtelist::List none = ReadList(m);
        qtelist::Steady st;
        const unsigned a = qtelist::Awaited(qtelist::Settle(st, good, 1000), false).await;
        const unsigned b = qtelist::Awaited(qtelist::Settle(st, wrong, 1014), false).await;
        const unsigned c = qtelist::Awaited(qtelist::Settle(st, refused, 1042), false).await;
        const qtelist::List& late = qtelist::Settle(st, wrong, 1056);  // (past 50 ms: as it is)
        const unsigned d = qtelist::Awaited(late, false).await;
        const bool lateWrong = late.status == qtelist::Status::kWrongType;
        const unsigned e = qtelist::Awaited(qtelist::Settle(st, wrong, 1070), false).await;
        const unsigned f = qtelist::Awaited(qtelist::Settle(st, good, 1084), false).await;
        const unsigned g = qtelist::Awaited(qtelist::Settle(st, none, 1098), false).await;  // (the game's own end isn't held over)
        qtelist::Steady fresh;
        const unsigned h = qtelist::Awaited(qtelist::Settle(fresh, wrong, 1000), false).await;
        Report("17g", "a doubtful read (not the manager, an entry left out) gives way to the last good list for 50 ms, no longer; no manager is taken at once",
               a == kSpace && b == kSpace && c == kSpace && d == 0 && lateWrong && e == 0 && f == kSpace && g == 0 && h == 0, "%02X %02X %02X %02X %02X %02X %02X %02X",
               a, b, c, d, e, f, g, h);
    }
    {
        // The game's own input state, with the masks the research logged for each key.
        Image m;
        const uint32_t context = kBase + qtelist::kGameContext, state = 0xE6003000u;
        m.W32(context, 0xE6000000u);
        m.W32(0xE6000000u + 0x30, 0xE6001000u);
        m.W32(0xE6001000u + 0xBC, 0xE6002000u);
        m.W32(0xE6002000u + 0x3E0, state);
        static const struct { uint64_t bits; unsigned key; } kLogged[] = {
            {0x004000000C000004ull, kSpace}, {0x0008000002000000ull, kLmb}, {0x0004002002000000ull, kE}, {0x0020000000000000ull, kF},
            {0x0080000000000010ull, kZ},     {0x0010000000000003ull, kRmb}, {0x0000000000000000ull, 0},  {0x0000000100000000ull, 0},
            {0x004C000000000000ull, kSpace | kLmb | kE},
        };
        bool each = true;
        for (const auto& k : kLogged) {
            m.Put(state + 0x1E0, &k.bits, 8);
            const qtelist::Input in = qtelist::ReadInput(m, context);
            each = each && in.read && qtelist::SeenKeys(in) == k.key;
        }
        const float pulled = 1.0f;
        m.Put(state + 0x10 + 4 * 7, &pulled, 4);
        const qtelist::Input fire = qtelist::ReadInput(m, context);
        m.W32(0xE6002000u + 0x3E0, 0xD0000000u);
        const qtelist::Input lost = qtelist::ReadInput(m, context);
        m.W32(0xE6001000u + 0xBC, 0);
        const qtelist::Input noPlayer = qtelist::ReadInput(m, context);
        Report("17h", "the game's input state: each key by its own quick-time action's bit (the masks the research logged); the fire axis; no player or an "
                      "unreadable state: not read, nothing seen",
               each && qtelist::ActionOn(fire, 0x07) && !qtelist::ActionOn(fire, 0x08) && !lost.read && !qtelist::SeenKeys(lost) && !noPlayer.read &&
                   !qtelist::ReadInput(m, 0).read,
               "each %d, fire %d, lost read %d, no player read %d", each, qtelist::ActionOn(fire, 0x07), lost.read, noPlayer.read);
    }
    {
        // The signature's search through made-up code.
        static const uint8_t kCode[] = {0x8B, 0x0D, 0x7C, 0x4F, 0x38, 0x02, 0x85, 0xC9, 0x75, 0x41, 0x57, 0x6A, 0x10, 0x6A, 0x4C, 0xB9};
        bool fixedBytes = qtelist::SignatureAt(kCode) && qtelist::SignatureGlobal(kCode) == 0x02384F7Cu;
        for (int i = 0; i < 16; ++i) {
            uint8_t changed[16];
            std::memcpy(changed, kCode, 16);
            changed[i] ^= 0x20;
            const bool wild = i >= 2 && i < 6;
            fixedBytes = fixedBytes && qtelist::SignatureAt(changed) == wild;
        }
        Image code;
        const uint32_t begin = kBase + 0x1000, end = begin + 0x20000;
        uint32_t seed = 12345;
        for (uint32_t at = begin; at < end; at += 4) {
            seed = seed * 1664525u + 1013904223u;
            code.W32(at, seed);
        }
        const auto valid = [](uint32_t global) { return global >= kBase && global < kBase + 0x2400000u && !(global & 3); };
        const auto search = [&](qtelist::Scan& scan) {
            scan = qtelist::Scan{};
            scan.at = begin;
            scan.end = end;
            int calls = 0;
            bool found = false;
            while (!found && scan.at < scan.end && calls < 1000) {
                found = qtelist::ScanStep(code, scan, 0x4000, valid);
                ++calls;
            }
            return found ? calls : -calls;
        };
        qtelist::Scan scan;
        const int nothing = search(scan);
        const bool notThere = nothing == -8 && scan.at == scan.end && !scan.found;
        const uint32_t place = begin + 0x15FF8;  // (across two pages)
        code.Put(place, kCode, 16);
        const int calls = search(scan);
        const bool across = calls == 6 && scan.found == place && scan.global == 0x02384F7Cu;
        uint8_t decoy[16];
        std::memcpy(decoy, kCode, 16);
        decoy[2] = 0x7D;  // (an odd address: no pointer's)
        code.Put(begin + 0x3210, decoy, 16);
        decoy[2] = 0x7C, decoy[5] = 0x7F;  // (outside the game's image)
        code.Put(begin + 0x9000, decoy, 16);
        search(scan);
        const bool passedOver = scan.found == place;
        code.Drop(begin + 0x8000);  // (a page that can't be read)
        search(scan);
        const bool overHole = scan.found == place && scan.unread == 1;
        code.Put(end - 16, kCode, 16);  // (and at the very end, with the first one gone)
        code.W8(place, 0x8A);
        search(scan);
        const bool atEnd = scan.found == end - 16;
        uint8_t flat[64] = {};
        std::memcpy(flat + 48, kCode, 16);
        Report("17i", "the signature: its ten fixed bytes, any address in it; found across two pages, past finds whose address can be no pointer's and past an "
                      "unreadable page, at the code's very end; not there: said so",
               fixedBytes && notThere && across && passedOver && overHole && atEnd && qtelist::FindSignature(flat, 64) == 48 && qtelist::FindSignature(flat, 63) == -1,
               "bytes %d, not there %d, across %d (%d calls), past decoys %d, past a hole %d, at the end %d", fixedBytes, notThere, across, calls, passedOver, overHole,
               atEnd);
    }
    {
        // The signature twice in the code, as in 1.6.0.0 (another manager is got the same way): every find a candidate, the manager's vtable picks.
        static const uint8_t kOurs[] = {0x8B, 0x0D, 0x7C, 0x4F, 0x38, 0x02, 0x85, 0xC9, 0x75, 0x41, 0x57, 0x6A, 0x10, 0x6A, 0x4C, 0xB9};
        static const uint8_t kOther[] = {0x8B, 0x0D, 0x78, 0xF3, 0x36, 0x02, 0x85, 0xC9, 0x75, 0x41, 0x57, 0x6A, 0x10, 0x6A, 0x4C, 0xB9};
        Image m;
        const uint32_t begin = kBase + 0x1000, end = begin + 0x10000, other = 0x0236F378u;
        for (uint32_t at = begin; at < end; at += 4)
            m.W32(at, 0x90909090u);
        m.Put(begin + 0x2000, kOther, 16);  // (the other manager's comes first, as in the game)
        m.Put(begin + 0x2010, kOther, 16);  // (and once more, the same address)
        m.Put(begin + 0x9FF4, kOurs, 16);
        const auto valid = [](uint32_t global) { return global >= kBase && global < kBase + 0x2400000u && !(global & 3); };
        qtelist::Candidates c;
        qtelist::Scan scan;
        scan.at = begin;
        scan.end = end;
        int finds = 0, calls = 0;
        while (calls++ < 100) {  // (as qtewatch.cpp: every find kept, the search going on after each)
            if (qtelist::ScanStep(m, scan, 0x3000, valid)) {
                qtelist::AddCandidate(c, scan.global, scan.found);
                ++finds;
            } else if (scan.at >= scan.end) {
                break;
            }
        }
        qtelist::AddCandidate(c, kGlobal, 0);  // (1.6.0.0's address: there already)
        const bool two = finds == 3 && c.count == 2 && c.global[0] == other && c.global[1] == kGlobal && c.found[1] == begin + 0x9FF4 && scan.at == scan.end;
        const int nowhere = qtelist::Pick(m, c, kTypes.manager);  // (neither pointer readable)
        m.W32(other, 0xE7300000u);
        m.W32(0xE7300000u, kTypes.manager + 0x40);  // (the other manager: another type)
        m.W32(kGlobal, 0);                          // (ours null: no level)
        const int noLevel = qtelist::Pick(m, c, kTypes.manager);
        Manager(m, {Entity(m, 0x40)});
        const int picked = qtelist::Pick(m, c, kTypes.manager);
        m.W32(kGlobal, kMgr + 2);
        const int oddPointer = qtelist::Pick(m, c, kTypes.manager);
        qtelist::Candidates full;
        for (uint32_t i = 0; i < 20; ++i)
            qtelist::AddCandidate(full, kBase + 0x1F00000 + 4 * i, 1);
        Report("17j", "the signature twice in the code (another manager's first): two places to try; neither taken with no level loaded; ours taken by the "
                      "manager's vtable; at most 8 kept",
               two && nowhere == -1 && noLevel == -1 && picked == 1 && oddPointer == -1 && full.count == qtelist::kMostCandidates,
               "places %d, unreadable %d, no level %d, picked %d, odd %d, kept %d of 20", c.count, nowhere, noLevel, picked, oddPointer, full.count);
    }
}

// 18 done again as the mod runs it now (qtewatch.cpp): through the fights' reader (qtefight.h's Step, its watch fresh for each game), the
// triggers on a level's own bus (a LogicPrefabBlueprint of another name): the singles must come out exactly as before.
qtefight::Watch* g_viaFights = nullptr;
uint32_t LevelBus(Image& m, uint32_t at, const char* name, uint32_t objectCount);

// The game's side of a quick-time event: its list of live triggers in the image (adds and removes applied 30 ms on), and its input, read 30 times a second.
struct Game {
    Image m;
    uint32_t yaw = 0, pitch = 0;
    std::vector<uint32_t> enabled;
    Pending pending;
    unsigned long long pendingAt = 0, nextTick = 0;
    unsigned tickKeys = 0;
    int presses[qte::kKeyCount] = {};  // the presses it saw, a key

    Game() {
        if (g_viaFights) {
            *g_viaFights = qtefight::Watch{};
            g_triggerBus = LevelBus(m, 0xE7380000u, "Levels/SP_Finale/TrainRide_SCM", 412);
        }
        yaw = Entity(m, 4);
        pitch = Entity(m, 5);
        enabled = {yaw, pitch};
        Manager(m, enabled);
    }
    bool Live(uint32_t entity) const {
        for (uint32_t e : enabled)
            if (e == entity)
                return true;
        return false;
    }
    void Change(uint32_t entity, bool remove, unsigned long long now) {
        pending.push_back({entity, remove ? 1u : 0u});
        pendingAt = now;
        Manager(m, enabled, pending);
    }
    void Apply(unsigned long long now) {
        if (pending.empty() || now < pendingAt + 30)
            return;
        for (const auto& p : pending) {
            if (p.second == 0)
                enabled.push_back(p.first);
            else
                for (size_t i = 0; i < enabled.size(); ++i)
                    if (enabled[i] == p.first) {
                        enabled.erase(enabled.begin() + static_cast<long>(i));
                        break;
                    }
        }
        pending.clear();
        Manager(m, enabled);
    }
    // Its input tick, if one is due: the keys that went down since the last.
    unsigned Tick(unsigned long long now, unsigned keys) {
        if (now < nextTick)
            return 0;
        nextTick = now + 33;
        const unsigned went = keys & ~tickKeys;
        tickKeys = keys;
        for (int b = 0; b < qte::kKeyCount; ++b)
            presses[b] += (went >> b & 1) != 0;
        return went;
    }
};

const qtefight::Types kFightTypes = qtefight::TypesAt(kBase);
int g_viaFightsAttaches = 0, g_viaFightsPrompts = 0;  // (18 through the fights' reader: buses it took for a fight's, frames it saw a prompt in)

// The mod's side of a frame, as input.cpp's QteStep and qtewatch.cpp's Update run it: the list read and steadied, the awaited keys, the grips.
unsigned ModFrame(Game& game, qtelist::Steady& steady, Run2& run, bool scripted, float left, float right) {
    const qtelist::List now = ReadList(game.m);
    const qtelist::List& list = qtelist::Settle(steady, now, run.now);
    if (g_viaFights) {
        const qtefight::Frame f = qtefight::Step(*g_viaFights, game.m, kFightTypes, list, scripted, run.now, true);
        for (int i = 0; i < f.events; ++i)
            g_viaFightsAttaches += f.event[i].what == qtefight::Ev::kAttached;
        g_viaFightsPrompts += f.prompt;
        run.await = f.await;
        run.hold = f.hold;
    } else {
        const qtelist::Keys keys = qtelist::Awaited(list, scripted);
        run.await = keys.await;
        run.hold = keys.hold;
    }
    run.game = 3;  // (the game's own input state: the keys as of its last input tick)
    run.external = game.tickKeys;
    return run.Frame(left, right).keys;
}

void Train() {
    {
        // The grapple: the quick-time Fire, live 3.3 s in, 3.6 s to answer it or the mission fails; one left click wins. Play's own Fire trigger is live too.
        Game game;
        const uint32_t playFire = Entity(game.m, 0x07), fire = Entity(game.m, 0x3D);
        game.enabled.push_back(playFire);
        Manager(game.m, game.enabled);
        qtelist::Steady steady;
        Run2 run;
        const unsigned long long start = run.now;
        unsigned long long appeared = 0, won = 0;
        bool failed = false, added = false;
        for (int i = 0; i < 715; ++i) {  // (10 s)
            const unsigned long long t = run.now;
            game.Apply(t);
            if (!added && t >= start + 3300) {
                added = true;
                game.Change(fire, false, t);
            }
            if (!appeared && game.Live(fire))
                appeared = t;
            // The player: the right grip on the gun throughout; the left squeezed 0.4 s after the prompt, for half a second.
            const bool squeeze = appeared && t >= appeared + 400 && t < appeared + 900;
            const unsigned keys = ModFrame(game, steady, run, false, squeeze ? 1.0f : 0.0f, 1.0f);
            const unsigned went = game.Tick(t, keys);
            if (game.Live(fire) && game.pending.empty()) {
                if (went & kLmb) {
                    won = t;
                    game.Change(fire, true, t);
                } else if (t > appeared + 3600) {
                    failed = true;
                }
            }
        }
        Report("18a", "the grapple: the left grip squeezed 0.4 s after the prompt: one left click, seen by the game, the event won; no other key, nothing after",
               won && !failed && won - appeared >= 400 && won - appeared < 500 && run.edges[Bit(kLmb)] == 1 && run.ever == kLmb && game.presses[Bit(kLmb)] == 1 &&
                   run.keys == 0 && run.began == 1 && run.lastWhy == qte::End::kAwait && run.lastHand == 0 && run.lastSeen == 1 && run.settledSeen == 1,
               "won %d after %llu ms, failed %d, left clicks %d, keys ever %02X, presses %d", won != 0, won - appeared, failed, run.edges[Bit(kLmb)], run.ever,
               run.began);
        // Nobody squeezing: nothing pressed, the mission fails as it does untouched.
        Game idle;
        const uint32_t fire2 = Entity(idle.m, 0x3D);
        idle.Change(fire2, false, 0);
        qtelist::Steady steady2;
        Run2 none;
        for (int i = 0; i < 400; ++i) {
            idle.Apply(none.now);
            idle.Tick(none.now, ModFrame(idle, steady2, none, true, 0.0f, 1.0f));
        }
        Report("18b", "... with only the gun's grip held on throughout: nothing pressed", none.ever == 0 && none.began == 0 && idle.presses[Bit(kLmb)] == 0, "keys %02X",
               none.ever);
    }
    for (int scripted = 0; scripted < 2; ++scripted) {
        // Hanging outside: JumpClimb and play's Jump live together, no timeout; one Space starts the shimmy.
        Game game;
        const uint32_t climb = Entity(game.m, 0x40), jump = Entity(game.m, 0x0C);
        qtelist::Steady steady;
        Run2 run;
        const unsigned long long start = run.now;
        unsigned long long appeared = 0, won = 0;
        bool added = false;
        for (int i = 0; i < 500; ++i) {
            const unsigned long long t = run.now;
            game.Apply(t);
            if (!added && t >= start + 1000) {
                added = true;
                game.Change(climb, false, t);
                game.Change(jump, false, t);
            }
            if (!appeared && game.Live(climb))
                appeared = t;
            const bool squeeze = appeared && t >= appeared + 600 && t < appeared + 1400;
            const unsigned keys = ModFrame(game, steady, run, scripted != 0, 0.0f, squeeze ? 1.0f : 0.0f);
            const unsigned went = game.Tick(t, keys);
            if (game.Live(climb) && game.pending.empty() && (went & kSpace)) {
                won = t;
                game.Change(climb, true, t);
                game.Change(jump, true, t);
            }
        }
        Report(scripted ? "18d" : "18c", scripted ? "... and in a scripted moment (play's Jump counting too): the same" : "hanging outside: the right grip squeezed: one Space, the shimmy starts; no other key",
               won && run.edges[Bit(kSpace)] == 1 && run.ever == kSpace && game.presses[Bit(kSpace)] == 1 && run.keys == 0 && run.lastHand == 1 &&
                   run.lastSeen == 1,
               "won %d after %llu ms, Space presses %d, keys ever %02X", won != 0, won - appeared, run.edges[Bit(kSpace)], run.ever);
    }
    // The dangling climb: JumpClimb alone, no timeout; Space pressed 30 times, never 0.6 s without one (it slides back), climbs.
    const auto dangle = [](bool mash, int player, unsigned long long& took, int& gamePresses, Run2& run) {
        Game game;
        const uint32_t climb = Entity(game.m, 0x40);
        qtelist::Steady steady;
        const unsigned long long start = run.now;
        run.mash = mash;
        unsigned long long appeared = 0, firstPress = 0, lastPress = 0, won = 0;
        int climbed = 0;
        bool added = false;
        for (int i = 0; i < 1100; ++i) {  // (15 s)
            const unsigned long long t = run.now;
            game.Apply(t);
            if (!added && t >= start + 500) {
                added = true;
                game.Change(climb, false, t);
            }
            if (!appeared && game.Live(climb))
                appeared = t;
            // The player: 0 holds the grip from 0.5 s after the prompt; 1 squeezes it five times a second.
            const bool reacting = appeared && t >= appeared + 500;
            const bool squeeze = reacting && (player == 0 || (t - appeared) % 200 < 100);
            const unsigned keys = ModFrame(game, steady, run, false, 0.0f, squeeze ? 1.0f : 0.0f);
            const unsigned went = game.Tick(t, keys);
            if (game.Live(climb) && game.pending.empty()) {
                if (climbed && t - lastPress > 600)
                    climbed = 0;  // (slid back)
                if (went & kSpace) {
                    if (!firstPress)
                        firstPress = t;
                    lastPress = t;
                    if (++climbed == 30) {
                        won = t;
                        game.Change(climb, true, t);
                    }
                }
            }
        }
        took = won ? won - firstPress : 0;
        gamePresses = game.presses[Bit(kSpace)];
        return won != 0;
    };
    {
        Run2 run;
        unsigned long long took = 0;
        int presses = 0;
        const bool won = dangle(true, 0, took, presses, run);
        Report("18e", "the dangling climb: the grip held: Space 30 times in about 4 s, each seen by the game, the climb won; let go as the game awaits it no more; "
                      "no other key",
               won && took >= 3500 && took <= 4300 && presses == 30 && run.edges[Bit(kSpace)] >= 30 && run.edges[Bit(kSpace)] <= 31 && run.ever == kSpace &&
                   run.keys == 0 && run.began == 1 && run.lastWhy == qte::End::kAwait && run.lastSeen == run.lastTaps && run.lastTaps == 30,
               "won %d in %llu ms, the game saw %d presses of %d, keys ever %02X", won, took, presses, run.edges[Bit(kSpace)], run.ever);
        Run2 single;
        const bool wonSingle = dangle(false, 0, took, presses, single);
        Report("18f", "... with \"qtemash 0\" and the grip held: one Space, no climb", !wonSingle && presses == 1 && single.edges[Bit(kSpace)] == 1 && single.ever == kSpace,
               "won %d, presses %d", wonSingle, presses);
        Run2 own;
        const bool wonOwn = dangle(false, 1, took, presses, own);
        Report("18g", "... with \"qtemash 0\" and the player's own five squeezes a second: 30 presses, the climb won", wonOwn && presses == 30 && took >= 5500 && took <= 6500 &&
                                                                                                                         own.ever == kSpace,
               "won %d in %llu ms, presses %d", wonOwn, took, presses);
    }
}

// The train's three quick-time events through the fights' reader as the mod runs them now (input.cpp's QteStep: the reader's frame into Step2 with its
// prompt and the scripted moment): the grapple (the quick-time Fire 3.3 s in, play's Fire live too in play, 3.6 s to answer it: one left click), the hanging
// (JumpClimb and play's Jump, no timeout: one Space), the dangling climb (JumpClimb alone, no timeout: 30 Spaces, never 0.6 s without one). grip: 0
// weapon.txt qtetest's as filmed on 2026-11-18 (standin 1 + qtetest 600: let go, squeezed from 0.3 s after a key is awaited until none is); 1
// "qtetest 600 held" (squeezed the whole test from 0.3 s on: qte::TestHeld); 2 the right grip held throughout, the left let go (the gun's grip as the
// grapple's cutscene begins).
struct TrainOutcome {
    bool grapple = false, hanging = false, dangling = false;
    int clicks = 0, spaces = 0, climbPresses = 0, byHold = 0;
    unsigned grappleKeys = 0, hangingKeys = 0, danglingKeys = 0;
    unsigned long long grappleAfter = 0, hangingAfter = 0, took = 0;  // the winning press after its trigger was first listed (its add); the climb's 30 presses
    int via = 0;
};
TrainOutcome TrainRuns(bool scripted, int gripMode) {
    TrainOutcome out;
    qtefight::Watch via;
    qtefight::Watch* const was = g_viaFights;
    g_viaFights = &via;
    const auto frame = [scripted, gripMode](Game& game, qtelist::Steady& steady, Run2& run, qte::TestGrip& grip, unsigned long long start,
                                            unsigned long long until) {
        const qtelist::List now = ReadList(game.m);
        const qtelist::List& list = qtelist::Settle(steady, now, run.now);
        const qtefight::Frame f = qtefight::Step(*g_viaFights, game.m, kFightTypes, list, scripted, run.now, true);
        run.await = f.await;
        run.hold = f.hold;
        run.mash = !f.prompt || f.mash;
        run.prompt = f.prompt;
        run.once = f.prompt && !f.mash;
        run.unknown = f.unknown;
        run.quick = f.prompt || f.unknown ? 0u : qtelist::QuickKeys(list, &f.skip) & f.await;  // (as qtewatch.cpp's Update)
        run.gates = f.prompt ? f.gates.right | f.gates.wrong : 0u;
        run.scripted = scripted;
        run.game = 3;
        run.external = game.tickKeys;
        const bool squeezed = qte::TestSqueezed(grip, run.await, run.now, until);
        const bool held = qte::TestHeld(run.now, start + qte::kTestAfter, until);
        return run.Frame(gripMode == 0 ? (squeezed ? 1.0f : 0.0f) : gripMode == 1 ? (held ? 1.0f : 0.0f) : 0.0f, gripMode == 2 ? 1.0f : 0.0f).keys;
    };
    bool grapple = false;
    int clicks = 0;
    unsigned grappleKeys = 0;
    {
        // (Play's own Fire live too, in play: never answered there. In the scripted moment as filmed only the quick-time Fire was listed: play's Fire would
        // be answered in one, by version 2's design of 2026-11-18, so it isn't put there.)
        Game game;
        const uint32_t playFire = Entity(game.m, 0x07), fire = Entity(game.m, 0x3D);
        if (!scripted)
            game.enabled.push_back(playFire);
        Manager(game.m, game.enabled);
        qtelist::Steady steady;
        Run2 run;
        qte::TestGrip grip;
        const unsigned long long start = run.now, until = start + 600000;
        unsigned long long appeared = 0, addedAt = 0;  // (applied; listed as an add first)
        bool added = false, failed = false, won = false;
        for (int i = 0; i < 715; ++i) {
            const unsigned long long t = run.now;
            game.Apply(t);
            if (!added && t >= start + 3300) {
                added = true;
                addedAt = t;
                game.Change(fire, false, t);
            }
            if (!appeared && game.Live(fire))
                appeared = t;
            const unsigned went = game.Tick(t, frame(game, steady, run, grip, start, until));
            if (game.Live(fire) && game.pending.empty()) {
                if (went & kLmb) {
                    won = true;
                    out.grappleAfter = t - addedAt;
                    game.Change(fire, true, t);
                } else if (t > appeared + 3600) {
                    failed = true;
                }
            }
        }
        clicks = game.presses[Bit(kLmb)];
        grappleKeys = run.ever;
        grapple = won && !failed && clicks == 1 && run.ever == kLmb && run.keys == 0;
        out.byHold += run.byHold;
    }
    // Hanging outside: JumpClimb and play's Jump, no timeout; one Space.
    bool hanging = false;
    int spaces = 0;
    {
        Game game;
        const uint32_t climb = Entity(game.m, 0x40), jump = Entity(game.m, 0x0C);
        qtelist::Steady steady;
        Run2 run;
        qte::TestGrip grip;
        const unsigned long long start = run.now, until = start + 600000;
        unsigned long long addedAt = 0;
        bool added = false, won = false;
        for (int i = 0; i < 500; ++i) {
            const unsigned long long t = run.now;
            game.Apply(t);
            if (!added && t >= start + 1000) {
                added = true;
                addedAt = t;
                game.Change(climb, false, t);
                game.Change(jump, false, t);
            }
            const unsigned went = game.Tick(t, frame(game, steady, run, grip, start, until));
            if (game.Live(climb) && game.pending.empty() && (went & kSpace) && !won) {
                won = true;
                out.hangingAfter = t - addedAt;
                game.Change(climb, true, t);
                game.Change(jump, true, t);
            }
        }
        spaces = game.presses[Bit(kSpace)];
        hanging = won && spaces == 1 && run.ever == kSpace && run.keys == 0;
        out.hangingKeys = run.ever;
        out.byHold += run.byHold;
    }
    // The dangling climb: JumpClimb alone, no timeout; 30 Spaces, never 0.6 s without one.
    bool dangling = false;
    int climbPresses = 0;
    unsigned long long took = 0;
    {
        Game game;
        const uint32_t climb = Entity(game.m, 0x40);
        qtelist::Steady steady;
        Run2 run;
        qte::TestGrip grip;
        const unsigned long long start = run.now, until = start + 600000;
        unsigned long long firstPress = 0, lastPress = 0;
        int climbed = 0;
        bool added = false, won = false;
        for (int i = 0; i < 1100; ++i) {
            const unsigned long long t = run.now;
            game.Apply(t);
            if (!added && t >= start + 500) {
                added = true;
                game.Change(climb, false, t);
            }
            const unsigned went = game.Tick(t, frame(game, steady, run, grip, start, until));
            if (game.Live(climb) && game.pending.empty()) {
                if (climbed && t - lastPress > 600)
                    climbed = 0;
                if (went & kSpace) {
                    if (!firstPress)
                        firstPress = t;
                    lastPress = t;
                    if (++climbed == 30) {
                        won = true;
                        took = t - firstPress;
                        game.Change(climb, true, t);
                    }
                }
            }
        }
        climbPresses = game.presses[Bit(kSpace)];
        dangling = won && climbPresses == 30 && took >= 3500 && took <= 4300 && run.ever == kSpace && run.keys == 0;
        out.danglingKeys = run.ever;
        out.byHold += run.byHold;
    }
    g_viaFights = was;
    g_triggerBus = 0;
    out.grapple = grapple, out.hanging = hanging, out.dangling = dangling;
    out.clicks = clicks, out.spaces = spaces, out.climbPresses = climbPresses;
    out.grappleKeys = grappleKeys;
    out.took = took;
    out.via = via.count;
    return out;
}

void TrainTestGrip() {
    // As the game runs them (a scripted moment, 2026-11-18's film) and as this test ran them till 2026-11-19 (not one): qtetest's grip the same in both.
    const TrainOutcome a = TrainRuns(true, 0), b = TrainRuns(false, 0);
    Report("18i", "the train's three with qtetest's grip through the fights' reader, as filmed (a scripted moment): the grapple one left click, the hanging one "
                  "Space, the dangling climb 30 Spaces in about 4 s; never another key",
           a.grapple && a.hanging && a.dangling && a.via <= 1 && a.byHold == 0,
           "grapple %d (clicks %d, keys %02X), hanging %d (Spaces %d), dangling %d (%d presses in %llu ms)", a.grapple, a.clicks, a.grappleKeys, a.hanging, a.spaces,
           a.dangling, a.climbPresses, a.took);
    Report("18j", "... the same not in a scripted moment, as this test ran them before 2026-11-19: the grapple one left click after the same time, the hanging one "
                  "Space, the climb's 30 Spaces in the same time",
           b.grapple && b.hanging && b.dangling && b.via <= 1 && b.grappleAfter == a.grappleAfter && b.hangingAfter == a.hangingAfter && b.took == a.took,
           "grapple %d after %llu ms (scripted: %llu), hanging %d after %llu ms (%llu), dangling %d in %llu ms (%llu)", b.grapple, b.grappleAfter, a.grappleAfter,
           b.hanging, b.hangingAfter, a.hangingAfter, b.dangling, b.took, a.took);
}

void Time2() {
    LARGE_INTEGER f, a, b;
    QueryPerformanceFrequency(&f);
    Run2 t;
    constexpr int kCalls = 2000000;
    QueryPerformanceCounter(&a);
    for (int i = 0; i < kCalls; ++i) {
        t.await = i % 3000 < 1500 ? kSpace | kE : 0;
        t.hold = kE;
        t.Frame(i % 40 < 20 ? 0.0f : 1.0f, i % 560 < 280 ? 0.0f : 1.0f);
    }
    QueryPerformanceCounter(&b);
    const double ns = 1e9 * static_cast<double>(b.QuadPart - a.QuadPart) / static_cast<double>(f.QuadPart) / kCalls;
    Game game;
    game.enabled.push_back(Entity(game.m, 0x40));
    game.enabled.push_back(Entity(game.m, 0x0C));
    Manager(game.m, game.enabled, {{Entity(game.m, 0x3D), 0}});
    constexpr int kReads = 200000;
    unsigned sum = 0;
    QueryPerformanceCounter(&a);
    for (int i = 0; i < kReads; ++i)
        sum += qtelist::Awaited(ReadList(game.m), i & 1).await;
    QueryPerformanceCounter(&b);
    const double us = 1e6 * static_cast<double>(b.QuadPart - a.QuadPart) / static_cast<double>(f.QuadPart) / kReads;
    Report("19a", "the time: a frame's step well under a microsecond; the list of five triggers read (through the test's image, slower than memory) in a few",
           ns < 1000.0 && us < 50.0 && t.began > 0 && sum > 0, "%.0f ns a step (%d presses), %.2f us a read", ns, t.began, us);
}

// ------------------------------------------------------------- the fist fights (qtefight.h) -------------------------------------------------------------

using qtefight::Check;

// bf3.exe as it lies in memory (build\bf3_mem.exe: 1.6.0.0, its code decrypted, a flat image from its base), for the proofs against the real code.
struct FileImage {
    std::vector<uint8_t> bytes;
    bool operator()(uint32_t at, void* out, size_t n) const {
        if (at < kBase || at - kBase > bytes.size() || n > bytes.size() - (at - kBase))
            return false;
        std::memcpy(out, bytes.data() + (at - kBase), n);
        return true;
    }
};

bool LoadFile(const std::wstring& path, std::vector<uint8_t>& out) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"rb") != 0 || !f)
        return false;
    std::fseek(f, 0, SEEK_END);
    const long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    out.resize(size > 0 ? static_cast<size_t>(size) : 0);
    const size_t got = out.empty() ? 0 : std::fread(out.data(), 1, out.size(), f);
    std::fclose(f);
    return got == out.size() && !out.empty();
}

// A pattern's bytes ("8B ?? 04"), ?? as fill.
std::vector<uint8_t> Bytes(const char* pattern, uint8_t fill = 0xCC) {
    std::vector<uint8_t> b;
    for (const char* p = pattern; *p;) {
        if (*p == ' ') {
            ++p;
            continue;
        }
        if (p[0] == '?') {
            b.push_back(fill);
        } else {
            unsigned v = 0;
            std::sscanf(p, "%2x", &v);
            b.push_back(static_cast<uint8_t>(v));
        }
        p += 2;
    }
    return b;
}

// Everything the 19 proofs read, made up where bf3.exe 1.6.0.0 has it: each named vtable's getType (mov eax, <ClassInfo>; ret) and its type's name, the
// handlers in their vtables' slots, the code bytes at their places, the bus's getType giving 0 as the game's does. names: each vtable's name's address.
void ProofImage(Image& m, std::map<uint32_t, uint32_t>& names) {
    const qtefight::Types t = qtefight::TypesAt(kBase);
    uint32_t fn = kBase + 0x1E80000, info = kBase + 0x2010000;
    const auto named = [&](uint32_t vtable, const char* name) {
        m.W32(vtable, fn);
        const uint8_t code[6] = {0xB8, static_cast<uint8_t>(info), static_cast<uint8_t>(info >> 8), static_cast<uint8_t>(info >> 16), static_cast<uint8_t>(info >> 24),
                                 0xC3};
        m.Put(fn, code, 6);
        m.W32(info, info + 0x10);
        m.W32(info + 0x10, info + 0x20);
        m.Put(info + 0x20, name, std::strlen(name) + 1);
        names[vtable] = info + 0x20;
        fn += 0x10;
        info += 0x80;
    };
    const auto code = [&](uint32_t rva, const char* pattern) {
        const std::vector<uint8_t> b = Bytes(pattern);
        m.Put(kBase + rva, b.data(), b.size());
    };
    named(t.gate, "EventGateEntity");
    m.W32(t.gate + 0x10, kBase + qtefight::kGateEvent);
    code(qtefight::kGateEvent, qtefight::kGateEventBytes);
    code(qtefight::kGateCtor, qtefight::kGateCtorBytes);
    m.W32(kBase + qtefight::kGateCtor + 2, t.gate);
    named(t.select, "SelectEventEntity");
    m.W32(t.select + 0x10, kBase + qtefight::kSelectEvent);
    code(qtefight::kSelectEvent, qtefight::kSelectEventBytes);
    code(qtefight::kSelectWrite, qtefight::kSelectWriteBytes);
    named(t.count, "CountDownEntity");
    m.W32(t.count + 0x10, kBase + qtefight::kCountEvent);
    code(qtefight::kCountEvent, qtefight::kCountEventBytes);
    code(qtefight::kCountCtor, qtefight::kCountCtorBytes);
    m.W32(kBase + qtefight::kCountCtor + 2, t.count);
    code(qtefight::kBusCtor, qtefight::kBusCtorBytes);
    m.W32(kBase + qtefight::kBusCtor + 2, t.bus);
    const uint8_t zero[3] = {0x33, 0xC0, 0xC3};  // (the bus's getType: xor eax, eax; ret)
    m.W32(t.bus, kBase + 0x148D50);
    m.Put(kBase + 0x148D50, zero, 3);
    m.W32(t.bus + 0x40, kBase + qtefight::kBusGetEntity);
    code(qtefight::kBusGetEntity, qtefight::kBusGetEntityBytes);
    code(qtefight::kInfoCtor, qtefight::kInfoCtorBytes);
    code(qtefight::kTriggerCtor, qtefight::kTriggerCtorBytes);
    named(t.gateData, "EventGateEntityData");
    named(t.selectData, "SelectEventEntityData");
    named(t.countData, "CountDownEntityData");
    named(t.prefab, "LogicPrefabBlueprint");
    named(t.triggerData, "PlayerInputTriggerEntityData");
    named(t.trigger, "ClientPlayerInputTriggerEntity");
    code(qtefight::kRegistryCode, qtefight::kRegistryBytes);
    m.W32(kBase + qtefight::kRegistryCode + 2, kBase + qtefight::kRegistryGlobal);
}

// Proof i not borne out by one change: a type's name, a handler's slot, a byte of its code, a constructor's vtable.
void BreakProof(Image& m, int i, std::map<uint32_t, uint32_t>& names) {
    const qtefight::Types t = qtefight::TypesAt(kBase);
    const auto name = [&](uint32_t vtable) { m.W8(names[vtable] + 1, 'X'); };
    const auto flip = [&](uint32_t rva, uint32_t offset) { uint8_t b = 0; m(kBase + rva + offset, &b, 1); m.W8(kBase + rva + offset, static_cast<uint8_t>(b ^ 0x01)); };
    switch (i) {
    case 0: name(t.gate); break;
    case 1: m.W32(t.gate + 0x10, kBase + qtefight::kGateEvent + 0x10); break;  // (another function in its slot)
    case 2: flip(qtefight::kGateCtor, 11); break;                              // (+0x14 no more: 88 4E 15)
    case 3: name(t.select); break;
    case 4: flip(qtefight::kSelectWrite, 2); break;                            // (its write elsewhere than +0x14)
    case 5: name(t.count); break;
    case 6: flip(qtefight::kCountEvent, 26); break;                            // (83 46 15 FF)
    case 7: m.W32(kBase + qtefight::kCountCtor + 2, t.count + 8); break;
    case 8: m.W32(kBase + qtefight::kBusCtor + 2, t.bus + 8); break;
    case 9: flip(qtefight::kBusGetEntity, 28); break;                          // (+0x31 the info)
    case 10: flip(qtefight::kInfoCtor, 9); break;                              // (+0x69 the blueprint)
    case 11: flip(qtefight::kTriggerCtor, 2); break;                           // (+0x0D the bus)
    case 12: name(t.gateData); break;
    case 13: name(t.selectData); break;
    case 14: name(t.countData); break;
    case 15: name(t.prefab); break;
    case 16: name(t.triggerData); break;
    case 17: name(t.trigger); break;
    case 18: m.W32(kBase + qtefight::kRegistryCode + 2, kBase + qtefight::kRegistryGlobal + 4); break;
    default: break;
    }
}

std::wstring OwnFolder() {
    wchar_t self[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    std::wstring s = self;
    return s.substr(0, s.find_last_of(L"\\/") + 1);
}

void Proofs() {
    FileImage real;
    if (LoadFile(OwnFolder() + L"bf3_mem.exe", real.bytes) && real.bytes.size() >= 0x2000000) {
        int borne = 0;
        char notBorne[256] = "";
        for (int i = 0; i <= qtefight::kProofs; ++i) {
            uint32_t at = 0;
            if (qtefight::ProveOne(real, kBase, i, &at))
                ++borne;
            else
                std::snprintf(notBorne + std::strlen(notBorne), sizeof(notBorne) - std::strlen(notBorne), " %d (%08X)", i, at);
        }
        const qtefight::Proofs p = qtefight::Prove(real, kBase, true);
        // qtelist's own constants against it as well: the manager's vtable named, its signature where 1.6.0.0 has it.
        char managerName[64] = "";
        qtefight::TypeName(real, kBase + qtelist::kManagerType, managerName, sizeof(managerName));
        uint8_t sig[qtelist::kSignatureSize] = {};
        const bool signature = real(kBase + qtelist::kSignatureAt, sig, sizeof(sig)) && qtelist::SignatureAt(sig) && qtelist::SignatureGlobal(sig) == kBase + qtelist::kManagerGlobal;
        Report("20a", "the real code (build\\bf3_mem.exe, bf3.exe 1.6.0.0 decrypted): every one of the reader's 18 constants borne out, and the registry's; qtelist's "
                      "signature where 1.6.0.0 has it",
               p.ok && borne == qtefight::kProofs + 1 && p.global == kBase + qtefight::kRegistryGlobal && signature,
               "%d of %d borne out%s%s, the registry's pointer at %08X, the signature %d (the manager's getType: %s)", borne, qtefight::kProofs + 1,
               notBorne[0] ? "; not:" : "", notBorne, p.global, signature, managerName[0] ? managerName : "no name");
    } else {
        Skip("20a", "no build\\bf3_mem.exe beside the test: the constants against the real code not checked");
    }
    Image m;
    std::map<uint32_t, uint32_t> names;
    ProofImage(m, names);
    const qtefight::Proofs all = qtefight::Prove(m, kBase, true), reader = qtefight::Prove(m, kBase);
    int alone = 0;
    char missed[256] = "";
    for (int i = 0; i <= qtefight::kProofs; ++i) {
        Image c = m;
        std::map<uint32_t, uint32_t> n = names;
        BreakProof(c, i, n);
        const qtefight::Proofs p = qtefight::Prove(c, kBase, true);
        bool others = true;
        for (int j = 0; j <= qtefight::kProofs; ++j)
            if (j != i)
                others = others && qtefight::ProveOne(c, kBase, j, nullptr);
        const bool ok = !p.ok && p.failed == i && others && !qtefight::ProveOne(c, kBase, i, nullptr);
        alone += ok;
        if (!ok)
            std::snprintf(missed + std::strlen(missed), sizeof(missed) - std::strlen(missed), " %d", i);
    }
    // Nothing there at all (another game's image): the first not borne out, nothing read past it.
    Image blank;
    const qtefight::Proofs none = qtefight::Prove(blank, kBase);
    Report("20b", "a made-up image of the 19 proofs: all borne out; each one broken alone (a type's name, a handler's slot, a byte of its code, a constructor's "
                  "vtable) is named, and only it; nothing there: the first named",
           all.ok && reader.ok && reader.global == 0 && alone == qtefight::kProofs + 1 && !none.ok && none.failed == 0,
           "all %d, broken alone and named %d of %d%s%s, blank's first %d", all.ok, alone, qtefight::kProofs + 1, missed[0] ? "; missed:" : "", missed, none.failed);
}

// A fist fight's prefab as the game builds it, made up (design section 2): the blueprint (a LogicPrefabBlueprint named QuickTimePrefab, its 103 objects,
// each its data at its own index, every gate Default False but the fail gate's), the compiled info (its client target table: 97 data and the interface's,
// sorted by address; the data's addresses shuffled, so the table's order isn't the objects'), shared by every instance as the game's is.
struct Prefab {
    uint32_t info = 0, blueprint = 0, name = 0, objects = 0, table = 0, descriptor = 0;
    uint32_t data[qtefight::kObjects] = {};
    std::vector<uint32_t> targets;  // sorted
};
// An instance: the connected bus (the client's), its entity array side by side with the table, every target's entity made by this bus for its data.
struct Fighting {
    uint32_t bus = 0, ents = 0;
    uint32_t entity[qtefight::kObjects] = {};  // object i's entity (0: a property target alone, none)
    uint32_t trigger[6] = {};                  // the six triggers' entities, by action (qtefight::kActions' order)
};

bool PropertyOnly(int i) {
    return i == 16 || i == 18 || i == 27 || i == 30 || i == 60 || i == 63;  // (the BoolToEvents: property targets alone)
}
bool IsGate(int i) {
    for (const auto& a : qtefight::kActions)
        if (a.right == i || a.wrong == i)
            return true;
    return i == qtefight::kFail;
}

Prefab MakePrefab(Image& m, uint32_t at) {
    Prefab p;
    p.info = at + 0x100;
    p.blueprint = at + 0x200;
    p.name = at + 0x300;
    p.objects = at + 0x404;
    p.table = at + 0x600;
    p.descriptor = at + 0x3400;
    m.Put(p.name, qtefight::kPrefabName, sizeof(qtefight::kPrefabName));
    m.W32(p.objects - 4, qtefight::kObjects);
    for (int i = 0; i < qtefight::kObjects; ++i) {
        const uint32_t d = at + 0x1000 + static_cast<uint32_t>((i * 37) % qtefight::kObjects) * 0x40;
        p.data[i] = d;
        m.W32(p.objects + 4u * static_cast<uint32_t>(i), d);
        for (uint32_t o = 0; o < 0x40; o += 4)
            m.W32(d + o, 0);
        const bool gate = IsGate(i), trigger = i >= 50 && i <= 55;
        const uint32_t type = gate ? kFightTypes.gateData : i == qtefight::kSelect ? kFightTypes.selectData : i == qtefight::kCount ? kFightTypes.countData
                              : trigger ? kFightTypes.triggerData : kBase + 0x1C77000 + 4u * static_cast<uint32_t>(i % 16);
        m.W32(d, type);
        m.W32(d + 4, 1);
        m.W32(d + 8, static_cast<uint32_t>(i) | (PropertyOnly(i) ? 3u : 0u) << 16);  // (IndexInBlueprint; IsEventConnectionTarget: Client, or None)
        if (gate)
            m.W8(d + 0x10, i == qtefight::kFail ? 1 : 0);
        if (i == qtefight::kCount) {
            m.W32(d + 0x0C, 10);  // (StartValue)
            m.W32(d + 0x14, 0);   // (RunOnce)
        }
        if (trigger) {
            const float one = 1.0f;
            m.W32(d + 0x10, qtefight::kTriggerActions[i - 50]);
            m.Put(d + 0x14, &one, 4);
            m.W8(d + 0x20, 1);  // (SendTriggerEvents)
        }
    }
    m.W32(p.descriptor, kBase + 0x1C77100);
    for (int i = 0; i < qtefight::kObjects; ++i)
        if (!PropertyOnly(i))
            p.targets.push_back(p.data[i]);
    p.targets.push_back(p.descriptor);
    std::sort(p.targets.begin(), p.targets.end());
    for (size_t k = 0; k < p.targets.size(); ++k)
        m.W32(p.table + 4 * static_cast<uint32_t>(k), p.targets[k]);
    for (uint32_t o = 0; o < 0xC0; o += 4)
        m.W32(p.info + o, 0);
    m.W32(p.info + 0x68, p.blueprint);
    m.W32(p.info + 0x6C, p.table);
    m.W32(p.info + 0x70, p.table + 4 * static_cast<uint32_t>(p.targets.size()));
    for (uint32_t o = 0; o < 0x40; o += 4)
        m.W32(p.blueprint + o, 0);
    m.W32(p.blueprint, kFightTypes.prefab);
    m.W32(p.blueprint + 0x08, p.name);
    m.W32(p.blueprint + 0x20, p.objects);
    return p;
}

Fighting MakeInstance(Image& m, const Prefab& p, uint32_t at) {
    Fighting f;
    f.bus = at;
    f.ents = at + 0x100;
    for (uint32_t o = 0; o < 0x40; o += 4)
        m.W32(f.bus + o, 0);
    m.W32(f.bus, kFightTypes.bus);
    m.W32(f.bus + 0x04, at + 0x3F00);  // (its owner)
    m.W32(f.bus + 0x10, 105);           // (its references)
    m.W32(f.bus + 0x14, 0x00000011u);   // (its slot in the registry, the realm 0: the client's)
    m.W32(f.bus + 0x1C, f.ents);
    m.W32(f.bus + 0x30, p.info);
    for (int i = 0; i < qtefight::kObjects; ++i) {
        if (PropertyOnly(i))
            continue;
        const uint32_t e = at + 0x1000 + static_cast<uint32_t>(i) * 0x40;
        f.entity[i] = e;
        for (uint32_t o = 0; o < 0x40; o += 4)
            m.W32(e + o, 0);
        const bool trigger = i >= 50 && i <= 55;
        const uint32_t type = IsGate(i) ? kFightTypes.gate : i == qtefight::kSelect ? kFightTypes.select : i == qtefight::kCount ? kFightTypes.count
                              : trigger ? kFightTypes.trigger : kBase + 0x1C7B000 + 4u * static_cast<uint32_t>(i % 16);
        m.W32(e, type);
        m.W32(e + 0x08, 0x4);  // (registered with its bus)
        m.W32(e + 0x0C, f.bus);
        m.W32(e + 0x10, p.data[i]);
        if (IsGate(i))
            m.W8(e + 0x14, i == qtefight::kFail ? 1 : 0);
        if (i == qtefight::kCount)
            m.W32(e + 0x14, 10);
        if (trigger)
            for (int a = 0; a < 6; ++a)
                if (qtefight::kActions[a].trigger == i)
                    f.trigger[a] = e;
    }
    const uint32_t descriptor = at + 0x3000;
    m.W32(descriptor, kBase + 0x1C7B100);
    m.W32(descriptor + 0x0C, f.bus);
    m.W32(descriptor + 0x10, p.descriptor);
    for (size_t k = 0; k < p.targets.size(); ++k) {
        uint32_t e = descriptor;
        for (int i = 0; i < qtefight::kObjects; ++i)
            if (p.data[i] == p.targets[k])
                e = f.entity[i];
        m.W32(f.ents + 4 * static_cast<uint32_t>(k), e);
    }
    return f;
}

// A level's own bus (not the prefab: another LogicPrefabBlueprint), for single quick-time events' triggers.
uint32_t LevelBus(Image& m, uint32_t at, const char* name, uint32_t objectCount) {
    const uint32_t info = at + 0x100, blueprint = at + 0x200, text = at + 0x300, objects = at + 0x404;
    for (uint32_t o = 0; o < 0x40; o += 4)
        m.W32(at + o, 0), m.W32(blueprint + o, 0);
    for (uint32_t o = 0; o < 0xC0; o += 4)
        m.W32(info + o, 0);
    m.W32(at, kFightTypes.bus);
    m.W32(at + 0x14, 0x00000002u);
    m.W32(at + 0x1C, at + 0x800);
    m.W32(at + 0x30, info);
    m.W32(info + 0x68, blueprint);
    m.W32(blueprint, kFightTypes.prefab);
    m.W32(blueprint + 0x08, text);
    m.W32(blueprint + 0x20, objects);
    m.Put(text, name, std::strlen(name) + 1);
    m.W32(objects - 4, objectCount);
    return at;
}

constexpr uint32_t kPrefabAt = 0xE7400000u, kFightAt = 0xE7500000u, kFightAt2 = 0xE7600000u;

// The list's live quick-time triggers of a bus, as the watch groups them.
qtefight::Live LiveOf(const qtelist::List& list, uint32_t bus) {
    qtefight::Live live;
    for (int i = 0; i < list.count; ++i)
        if (list.at[i].bus == bus && qtelist::QuickTime(list.at[i].action))
            live.Add(list.at[i].entity, list.at[i].data, list.at[i].action);
    return live;
}

// The made-up fight: its prefab, one instance, its six triggers enabled in the manager's list with the level's Yaw and Pitch.
struct FightImage {
    Image m;
    Prefab p;
    Fighting f;
    uint32_t yaw = 0, pitch = 0;
    FightImage() {
        p = MakePrefab(m, kPrefabAt);
        f = MakeInstance(m, p, kFightAt);
        yaw = Entity(m, 4);
        pitch = Entity(m, 5);
        Enable(true);
    }
    void Enable(bool on, const Pending& pending = {}) {
        std::vector<uint32_t> enabled = {yaw, pitch};
        if (on)
            for (uint32_t t : f.trigger)
                enabled.push_back(t);
        Manager(m, enabled, pending);
    }
    qtefight::Attach Attach() const {
        const qtelist::List list = ReadList(m);
        return qtefight::AttachBus(m, kFightTypes, f.bus, LiveOf(list, f.bus));
    }
    void Gate(int index, bool open) { m.W8(f.entity[index] + 0x14, open ? 1 : 0); }
    void Prompt(int a, bool open) {
        Gate(qtefight::kActions[a].right, open);
        Gate(qtefight::kActions[a].wrong, open);
    }
    void Select(int v) { m.W32(f.entity[qtefight::kSelect] + 0x14, static_cast<uint32_t>(v)); }
    void Count(int v) { m.W32(f.entity[qtefight::kCount] + 0x14, static_cast<uint32_t>(v)); }
    // The fail gate (#85, Default True): opened again as each prompt opens (#84), shut by the fight's success (#1 -> #71) or as it fails it (#97).
    void FailGate(bool open) { Gate(qtefight::kFail, open); }
};

// The watch over the frames (qtefight::Step, as qtewatch.cpp runs it).
struct Watching {
    qtefight::Watch w;
    unsigned long long now = 100000;
    int attached = 0, again = 0, notPrefab = 0, failed = 0, lost = 0, gone = 0, off = 0, stale = 0, disarmed = 0;
    qtefight::Check lastFailed = Check::kNone;
    int lastFailedIndex = -1;
    qtefight::Frame last;
    qtefight::Frame Frame(const Image& m, bool on = true, bool scripted = false) {
        const qtelist::List list = ReadList(m);
        last = qtefight::Step(w, m, kFightTypes, list, scripted, now, on);
        for (int i = 0; i < last.events; ++i) {
            const qtefight::Event& e = last.event[i];
            attached += e.what == qtefight::Ev::kAttached && !e.again;
            again += e.what == qtefight::Ev::kAttached && e.again;
            notPrefab += e.what == qtefight::Ev::kNotPrefab;
            failed += e.what == qtefight::Ev::kFailed;
            lost += e.what == qtefight::Ev::kLost;
            gone += e.what == qtefight::Ev::kGone;
            off += e.what == qtefight::Ev::kOff;
            stale += e.what == qtefight::Ev::kStale;
            disarmed += e.what == qtefight::Ev::kDisarmed;
            if (e.what == qtefight::Ev::kFailed)
                lastFailed = e.attach.check, lastFailedIndex = e.attach.index;
        }
        now += kFrame;
        return last;
    }
};

void Attaches() {
    {
        FightImage g;
        const qtefight::Attach a = g.Attach();
        const qtefight::Fight& f = a.fight;
        bool objects = true;
        for (int i = 0; i < 6; ++i)
            objects = objects && f.gate[0][i] == g.f.entity[qtefight::kActions[i].right] && f.gate[1][i] == g.f.entity[qtefight::kActions[i].wrong] &&
                      f.gateData[0][i] == g.p.data[qtefight::kActions[i].right] && f.gateData[1][i] == g.p.data[qtefight::kActions[i].wrong];
        objects = objects && f.select == g.f.entity[qtefight::kSelect] && f.count == g.f.entity[qtefight::kCount] && f.fail == g.f.entity[qtefight::kFail] &&
                  f.countStart == 10;
        char text[256];
        Report("21a", "a fight's bus as the game builds it (the prefab's 103 objects, its 98 targets sorted, the six triggers live): attached; the trigger's own "
                      "entries, the 12 gates, the select, the count and the fail gate found, the name read",
               a.check == Check::kNone && objects && f.targets == 98 && f.named && f.bus == g.f.bus && f.info == g.p.info && f.entities == g.f.ents,
               "%s; %d targets, named %d, objects %d", qtefight::Describe(a, text, sizeof(text)), f.targets, f.named, objects);
    }
    // Every check failing, one at a time: each named by its check (and the object it's of); those saying another blueprint's taken as not the prefab.
    struct Case {
        const char* what;
        Check want;
        int index;
        void (*change)(FightImage&);
    };
    static const Case kCases[] = {
        {"the bus's vtable", Check::kBus, -1, [](FightImage& g) { g.m.W32(g.f.bus, kFightTypes.bus + 0x58); }},
        {"the bus unreadable", Check::kBus, -1, [](FightImage& g) { g.m.Drop(g.f.bus); }},
        {"the server's bus", Check::kRealm, -1, [](FightImage& g) { g.m.W32(g.f.bus + 0x14, 0x20000011u); }},
        {"no info", Check::kInfo, -1, [](FightImage& g) { g.m.W32(g.f.bus + 0x30, 0); }},
        {"the info's blueprint unreadable", Check::kInfo, -1, [](FightImage& g) { g.m.W32(g.p.info + 0x68, 0xD0000000u); }},
        {"another blueprint type", Check::kBlueprint, -1, [](FightImage& g) { g.m.W32(g.p.blueprint, kFightTypes.prefab + 0x40); }},
        {"another prefab's name", Check::kName, -1, [](FightImage& g) { g.m.Put(g.p.name, "Animations/PreFabs/QuickTimeFabric", 35); }},
        {"a longer name", Check::kName, -1, [](FightImage& g) { g.m.Put(g.p.name + 34, "2", 2); }},
        {"102 objects", Check::kObjects, -1, [](FightImage& g) { g.m.W32(g.p.objects - 4, 102); }},
        {"the objects unreadable", Check::kObjects, -1, [](FightImage& g) { g.m.W32(g.p.blueprint + 0x20, 0xD0000000u); }},
        {"a trigger data's action", Check::kTriggers, 50, [](FightImage& g) { g.m.W32(g.p.data[50] + 0x10, 0x3F); }},
        {"a trigger data's index", Check::kTriggers, 52, [](FightImage& g) { g.m.W32(g.p.data[52] + 8, 51); }},
        {"a trigger data's type", Check::kTriggers, 55, [](FightImage& g) { g.m.W32(g.p.data[55], kFightTypes.gateData); }},
        {"a live trigger's data not the prefab's", Check::kOwnData, -1,
         [](FightImage& g) {
             const uint32_t other = 0xE7700000u;  // (a trigger data of another blueprint, the same action)
             for (uint32_t o = 0; o < 0x40; o += 4)
                 g.m.W32(other + o, 0);
             g.m.W32(other, kFightTypes.triggerData);
             g.m.W32(other + 0x10, 0x40);
             g.m.W32(g.f.trigger[4] + 0x10, other);
         }},
        {"the table's end before its beginning", Check::kTable, -1, [](FightImage& g) { g.m.W32(g.p.info + 0x70, g.p.table - 8); }},
        {"a ragged table", Check::kTable, -1, [](FightImage& g) { g.m.W32(g.p.info + 0x70, g.p.table + 98 * 4 + 2); }},
        {"600 targets", Check::kTable, -1, [](FightImage& g) { g.m.W32(g.p.info + 0x70, g.p.table + 600 * 4); }},
        {"an unsorted table", Check::kTable, 41,
         [](FightImage& g) {
             uint32_t a = 0, b = 0;
             g.m(g.p.table + 40 * 4, &a, 4);
             g.m(g.p.table + 41 * 4, &b, 4);
             g.m.W32(g.p.table + 40 * 4, b);
             g.m.W32(g.p.table + 41 * 4, a);
         }},
        {"the table unreadable", Check::kTable, -1, [](FightImage& g) { g.m.W32(g.p.info + 0x6C, 0xD0000000u), g.m.W32(g.p.info + 0x70, 0xD0000000u + 98 * 4); }},
        {"the entities unreadable", Check::kEntities, -1, [](FightImage& g) { g.m.W32(g.f.bus + 0x1C, 0xD0000000u); }},
        {"a trigger not its own entry", Check::kOwnEntry, -1,
         [](FightImage& g) {
             for (size_t k = 0; k < g.p.targets.size(); ++k)
                 if (g.p.targets[k] == g.p.data[53])
                     g.m.W32(g.f.ents + 4 * static_cast<uint32_t>(k), g.f.entity[54]);
         }},
        {"a gate's data type", Check::kGate, 7, [](FightImage& g) { g.m.W32(g.p.data[7], kFightTypes.selectData); }},
        {"a gate's data index", Check::kGate, 15, [](FightImage& g) { g.m.W32(g.p.data[15] + 8, 14); }},
        {"a gate's Default true", Check::kGate, 28, [](FightImage& g) { g.m.W8(g.p.data[28] + 0x10, 1); }},
        {"a gate with no entity", Check::kGate, 62,
         [](FightImage& g) {
             for (size_t k = 0; k < g.p.targets.size(); ++k)
                 if (g.p.targets[k] == g.p.data[62])
                     g.m.W32(g.f.ents + 4 * static_cast<uint32_t>(k), 0);
         }},
        {"a gate's entity's vtable", Check::kGate, 59, [](FightImage& g) { g.m.W32(g.f.entity[59], kFightTypes.select); }},
        {"a gate's entity another bus's", Check::kGate, 32, [](FightImage& g) { g.m.W32(g.f.entity[32] + 0x0C, kFightAt2); }},
        {"a gate's entity for other data", Check::kGate, 8, [](FightImage& g) { g.m.W32(g.f.entity[8] + 0x10, g.p.data[19]); }},
        {"a gate's data not a target", Check::kGate, 29, [](FightImage& g) { g.m.W32(g.p.objects + 29 * 4, 0xE7720000u), g.m.W32(0xE7720000u, kFightTypes.gateData),
                                                                              g.m.W32(0xE7720008u, 29); }},
        {"the select's data type", Check::kSelect, 98, [](FightImage& g) { g.m.W32(g.p.data[98], kFightTypes.countData); }},
        {"the select's entity's vtable", Check::kSelect, 98, [](FightImage& g) { g.m.W32(g.f.entity[98], kFightTypes.count); }},
        {"the count's data index", Check::kCount, 75, [](FightImage& g) { g.m.W32(g.p.data[75] + 8, 76); }},
        {"the count's entity another bus's", Check::kCount, 75, [](FightImage& g) { g.m.W32(g.f.entity[75] + 0x0C, 0); }},
    };
    int named = 0, classed = 0, total = 0;
    char wrong[512] = "";
    for (const Case& c : kCases) {
        FightImage g;
        c.change(g);
        const qtefight::Attach a = g.Attach();
        const bool ok = a.check == c.want && (c.index < 0 || a.index == c.index);
        const bool notPrefab = c.want == Check::kBus || c.want == Check::kRealm || c.want == Check::kBlueprint || c.want == Check::kName ||
                               c.want == Check::kObjects || c.want == Check::kTriggers || c.want == Check::kOwnData;
        named += ok;
        // (and those past the prefab's own checks taken as a fight's: its triggers out of version 2's keys however few are listed)
        classed += qtefight::NotThePrefab(a.check) == notPrefab && qtefight::ThePrefab(a.check) == (!notPrefab && c.want != Check::kInfo);
        ++total;
        if (!ok && std::strlen(wrong) < sizeof(wrong) - 80)
            std::snprintf(wrong + std::strlen(wrong), sizeof(wrong) - std::strlen(wrong), " [%s: got %d/%d]", c.what, static_cast<int>(a.check), a.index);
    }
    Report("21b", "every attach check failing alone (the bus, its realm, the info, the blueprint, its name, its objects, the six trigger data, a live trigger's "
                  "own data, the table, the entities, a trigger's own entry, each gate's data and entity, the select, the count): each named, with its object; "
                  "another blueprint's checks taken as not the prefab",
           named == total && classed == total, "%d of %d named, %d classed%s", named, total, classed, wrong);
    {
        FightImage u;
        u.m.W32(u.p.blueprint + 8, 0xD0000000u);  // (its name not readable)
        const qtefight::Attach unread = u.Attach();
        FightImage z;
        z.m.W32(z.p.blueprint + 8, 0);
        const qtefight::Attach none = z.Attach();
        FightImage l;
        l.m.Put(l.p.name, "animations/prefabs/quicktimeprefab", 35);
        const qtefight::Attach lower = l.Attach();
        FightImage n;
        n.m.W8(n.f.entity[qtefight::kFail] + 0x14, 0);
        n.m.W8(n.p.data[qtefight::kFail] + 0x10, 0);  // (the fail gate not as the prefab has it: only said)
        const qtefight::Attach noFail = n.Attach();
        FightImage r;  // (the registry's walk: no trigger to start from)
        const qtefight::Attach walked = qtefight::AttachBus(r.m, kFightTypes, r.f.bus, qtefight::Live{});
        Report("21c", "the name not readable or none: attached by the structure alone, said so; in lower case: the prefab's; the fail gate not as the prefab has "
                      "it: attached without it; no trigger (the registry's walk): attached",
               unread.check == Check::kNone && !unread.fight.named && none.check == Check::kNone && !none.fight.named && lower.check == Check::kNone &&
                   lower.fight.named && noFail.check == Check::kNone && noFail.fight.fail == 0 && walked.check == Check::kNone,
               "unread %d (named %d), none %d, lower %d (named %d), no fail gate %d (%08X), walked %d", static_cast<int>(unread.check), unread.fight.named,
               static_cast<int>(none.check), static_cast<int>(lower.check), lower.fight.named, static_cast<int>(noFail.check), noFail.fight.fail,
               static_cast<int>(walked.check));
    }
    {
        // The reads bounded: a table of 512 targets read once; no fault on pointers into nothing.
        FightImage g;
        g.m.reads = 0;
        const qtefight::Attach a = g.Attach();
        const int reads = g.m.reads;
        FightImage w;
        w.m.W32(w.p.objects + 50 * 4, 0xFFFFFFF0u);
        w.m.W32(w.p.objects + 51 * 4, 3);
        const qtefight::Attach wild = w.Attach();
        Report("21d", "an attach's reads bounded (about 60 with the list's); wild and odd pointers in the objects: refused, no fault", a.check == Check::kNone && reads < 120 &&
                                                                                                                 wild.check == Check::kTriggers && wild.index == 50,
               "%d reads, wild %d (object %d)", reads, static_cast<int>(wild.check), wild.index);
    }
    {
        // An attach whose state wouldn't read each frame (a gate's byte 2, the select 3, the fail gate's byte 2): no attach (the review of 2026-11-18: it
        // attached, then was lost every frame, a line a frame). Watched: said once, never lost, tried again every 0.25 s with nothing pressed, its triggers
        // out of version 2's keys; the byte back: attached, its prompt answered.
        FightImage b;
        b.m.W8(b.f.entity[7] + 0x14, 2);
        const qtefight::Attach gate = b.Attach();
        FightImage s;
        s.m.W32(s.f.entity[qtefight::kSelect] + 0x14, 3);
        const qtefight::Attach select = s.Attach();
        FightImage fg;
        fg.m.W8(fg.f.entity[qtefight::kFail] + 0x14, 2);
        const qtefight::Attach fail = fg.Attach();
        Watching w;
        unsigned ever = 0;
        bool skipped = true;
        for (int i = 0; i < 100; ++i) {
            const qtefight::Frame f = w.Frame(b.m);
            ever |= f.await;
            skipped = skipped && f.skip.Has(b.f.bus);
        }
        const int failedSaid = w.failed, lostSaid = w.lost;
        b.m.W8(b.f.entity[7] + 0x14, 0);
        b.Prompt(0, true);
        int until = 0;
        while (!w.attached && until++ < 40)
            w.Frame(b.m);
        Report("21e", "an attach whose state wouldn't read (a gate's byte 2, the select 3, the fail gate's byte 2): none, the object named, a fight's; watched: said "
                      "once, never lost, nothing pressed, out of version 2's keys; the byte back: attached within 0.25 s, its prompt answered",
               gate.check == Check::kState && gate.index == 7 && select.check == Check::kState && select.index == qtefight::kSelect && fail.check == Check::kState &&
                   fail.index == qtefight::kFail && qtefight::ThePrefab(gate.check) && failedSaid == 1 && lostSaid == 0 && ever == 0 && skipped &&
                   w.attached == 1 && until <= 19 && w.last.answer == kE,
               "gate %d/%d, select %d/%d, fail gate %d/%d; said %d, lost %d, keys %02X, skipped %d; attached after %d frames, answer %02X",
               static_cast<int>(gate.check), gate.index, static_cast<int>(select.check), select.index, static_cast<int>(fail.check), fail.index, failedSaid,
               lostSaid, ever, skipped, until, w.last.answer);
    }
}

void Gates() {
    FightImage g;
    const qtefight::Fight f = g.Attach().fight;
    const auto now = [&]() {
        qtefight::State s = qtefight::ReadState(g.m, kFightTypes, f);
        return s;
    };
    bool each = true, mash = true;
    for (int a = 0; a < 6; ++a) {
        g.Prompt(a, true);
        g.Select(0);
        const qtefight::State s = now();
        const qtefight::Answer x = qtefight::Decide(&s, 1);
        g.Select(1);
        g.Count(7);
        const qtefight::State sm = now();
        const qtefight::Answer y = qtefight::Decide(&sm, 1);
        g.Prompt(a, false);
        g.Select(0);
        g.Count(10);
        each = each && s.read && s.right == (1u << a) && s.wrong == (1u << a) && x.why == qtefight::Why::kAnswer && x.key == (1u << a) && !x.mash && s.fail == 1;
        mash = mash && y.why == qtefight::Why::kAnswer && y.key == (1u << a) && y.mash && y.left == 7;
    }
    const qtefight::State shut = now();
    const qtefight::Answer none = qtefight::Decide(&shut, 1);
    Report("22a", "each action's prompt alone (its right and its wrong gate open): its own key, a press; the select at 1: a mash, its count; all shut: no prompt",
           each && mash && shut.read && none.why == qtefight::Why::kNone && !none.Prompt() && none.key == 0, "each %d, mash %d, shut %d", each, mash,
           static_cast<int>(none.why));
    // The answer rule's other cases.
    const auto decide = [&](unsigned right, unsigned wrong) {
        for (int a = 0; a < 6; ++a) {
            g.Gate(qtefight::kActions[a].right, (right >> a) & 1);
            g.Gate(qtefight::kActions[a].wrong, (wrong >> a) & 1);
        }
        const qtefight::State s = now();
        return qtefight::Decide(&s, 1);
    };
    const qtefight::Answer two = decide(kLmb | kRmb, kLmb | kRmb), lone = decide(0, kF), other = decide(kSpace, kZ), both = decide(kSpace, kSpace | kE),
                           own = decide(kZ, kZ), rightAlone = decide(kE, 0);
    Report("22b", "two prompts: nothing; a wrong gate with no prompt: nothing; a prompt with another's wrong gate open: nothing; its own wrong gate alone with it, "
                  "or none: its key",
           two.why == qtefight::Why::kTwo && two.key == 0 && two.Prompt() && lone.why == qtefight::Why::kWrongAlone && lone.key == 0 && lone.Prompt() &&
               other.why == qtefight::Why::kWrongOther && other.key == 0 && both.why == qtefight::Why::kWrongOther && own.why == qtefight::Why::kAnswer &&
               own.key == kZ && rightAlone.why == qtefight::Why::kAnswer && rightAlone.key == kE,
           "two %d, lone %d, other %d, both %d, own %d (%02X), right alone %d", static_cast<int>(two.why), static_cast<int>(lone.why), static_cast<int>(other.why),
           static_cast<int>(both.why), static_cast<int>(own.why), own.key, static_cast<int>(rightAlone.why));
    // A switch between prompts caught halfway, every order the game's thread may write the four bytes in: never a key but the one prompt up's.
    {
        int frames = 0, wrongKey = 0, answered = 0;
        for (int order = 0; order < 24; ++order) {
            int perm[4] = {0, 1, 2, 3};
            for (int k = 0, o = order; k < 4; ++k) {  // (the order-th permutation)
                const int pick = o % (4 - k);
                o /= 4 - k;
                std::swap(perm[k], perm[k + pick]);
            }
            unsigned right = kRmb, wrong = kRmb;  // (Block's prompt up; Fire's comes: Block's two gates shut, Fire's two open)
            for (int step = 0; step < 4; ++step) {
                switch (perm[step]) {
                case 0: right &= ~kRmb; break;
                case 1: wrong &= ~kRmb; break;
                case 2: right |= kLmb; break;
                case 3: wrong |= kLmb; break;
                }
                const qtefight::Answer x = decide(right, wrong);
                ++frames;
                if (x.key) {
                    ++answered;
                    const unsigned up = (right == kLmb && wrong == kLmb) ? kLmb : (right == kRmb && (wrong & ~kRmb) == 0) ? kRmb : (right == kLmb && (wrong & ~kLmb) == 0) ? kLmb : 0;
                    wrongKey += x.key != up;
                    // A press of it must reach only shut wrong gates: no other action's wrong gate open.
                    wrongKey += (wrong & ~x.key) != 0;
                }
            }
        }
        Report("22c", "a switch between two prompts caught halfway (all 24 orders of its four writes): a key only while one prompt's gates alone are open, never "
                      "one whose press would reach another's open wrong gate",
               wrongKey == 0 && answered > 0 && frames == 96, "%d frames, %d answered, %d wrong", frames, answered, wrongKey);
    }
    decide(0, 0);
    // The per-frame checks: anything not as attached, not read (nothing held over).
    struct Case {
        const char* what;
        void (*change)(FightImage&);
    };
    static const Case kCases[] = {
        {"the bus's vtable", [](FightImage& h) { h.m.W32(h.f.bus, 0); }},
        {"its entities moved", [](FightImage& h) { h.m.W32(h.f.bus + 0x1C, h.f.ents + 0x40); }},
        {"its info another", [](FightImage& h) { h.m.W32(h.f.bus + 0x30, h.p.info + 0x40); }},
        {"the server's", [](FightImage& h) { h.m.W32(h.f.bus + 0x14, 0x40000011u); }},
        {"a gate freed (its vtable)", [](FightImage& h) { h.m.W32(h.f.entity[31], kBase + 0x1C7B000); }},
        {"a gate another bus's", [](FightImage& h) { h.m.W32(h.f.entity[58] + 0x0C, kFightAt2); }},
        {"a gate for other data", [](FightImage& h) { h.m.W32(h.f.entity[61] + 0x10, 0); }},
        {"a gate's byte 2", [](FightImage& h) { h.m.W8(h.f.entity[7] + 0x14, 2); }},
        {"the select 2", [](FightImage& h) { h.m.W32(h.f.entity[98] + 0x14, 2); }},
        {"the select -1", [](FightImage& h) { h.m.W32(h.f.entity[98] + 0x14, 0xFFFFFFFFu); }},
        {"the count unreadable", [](FightImage& h) { h.m.Drop(h.f.entity[75]); }},
        {"the bus unreadable", [](FightImage& h) { h.m.Drop(h.f.bus); }},
        {"the fail gate freed (its vtable)", [](FightImage& h) { h.m.W32(h.f.entity[qtefight::kFail], kBase + 0x1C7B000); }},
        {"the fail gate's byte 2", [](FightImage& h) { h.m.W8(h.f.entity[qtefight::kFail] + 0x14, 2); }},
    };
    int refused = 0, total = 0;
    for (const Case& c : kCases) {
        FightImage h;
        const qtefight::Fight hf = h.Attach().fight;
        h.Prompt(2, true);  // (Block's prompt up as it happens)
        c.change(h);
        const qtefight::State s = qtefight::ReadState(h.m, kFightTypes, hf);
        const qtefight::Answer x = qtefight::Decide(&s, 1);
        refused += !s.read && x.key == 0 && !x.Prompt();
        ++total;
    }
    FightImage pad;
    const qtefight::Fight pf = pad.Attach().fight;
    pad.m.W8(pad.f.entity[7] + 0x15, 0xAB);  // (the gate's byte alone counts: what lies above it isn't the game's)
    pad.Prompt(0, true);
    const qtefight::State padded = qtefight::ReadState(pad.m, kFightTypes, pf);
    Report("22d", "each frame's checks (the bus's vtable, entities, info and realm; a gate freed, another bus's, for other data; a byte not 0 or 1; the select not "
                  "0 or 1; an object unreadable; the fail gate freed or its byte 2): not read, nothing pressed; the bytes past a gate's own not looked at",
           refused == total && padded.read && padded.right == kE, "%d of %d not read, padded %d (%02X)", refused, total, padded.read, padded.right);
}

void Watches() {
    {
        // A fight from its Start: the six come as adds not yet applied, then applied; its prompts; its Stop.
        FightImage g;
        g.Enable(false);
        Watching w;
        const qtefight::Frame before = w.Frame(g.m);
        Pending adds;
        for (uint32_t t : g.f.trigger)
            adds.push_back({t, 0});
        Manager(g.m, {g.yaw, g.pitch}, adds);
        const qtefight::Frame started = w.Frame(g.m);
        g.Enable(true);
        const qtefight::Frame applied = w.Frame(g.m);
        for (int i = 0; i < 100; ++i)
            w.Frame(g.m);
        const int attachesAfter = w.attached;
        g.Prompt(3, true);  // (F's prompt)
        const qtefight::Frame prompt = w.Frame(g.m);
        g.Prompt(3, false);
        const qtefight::Frame between = w.Frame(g.m);
        // Its Stop: three of the six's removes not yet applied (a read caught as the game queues them: the rest still live), then all six.
        Pending some;
        for (int a = 0; a < 3; ++a)
            some.push_back({g.f.trigger[a], 1});
        g.Enable(true, some);
        const qtefight::Frame stopping = w.Frame(g.m);
        g.Enable(false);
        const qtefight::Frame stopped = w.Frame(g.m);
        const qtefight::Frame after = w.Frame(g.m);
        Report("23a", "a fight's Start (its six as adds not yet applied, then applied): attached once, its six out of version 2's keys from the first frame; its "
                      "prompt: F alone, a press; between prompts nothing; its Stop: half its triggers gone still a fight's (nothing pressed), all gone forgotten",
               before.await == 0 && !before.prompt && started.events == 1 && started.event[0].what == qtefight::Ev::kAttached && started.skip.Has(g.f.bus) &&
                   started.await == 0 && applied.await == 0 && attachesAfter == 1 && w.again == 1 && prompt.prompt && prompt.answer == kF && prompt.await == kF &&
                   !prompt.mash && prompt.fights == 1 && !between.prompt && between.await == 0 && stopping.await == 0 && stopping.skip.Has(g.f.bus) &&
                   stopped.events == 1 && stopped.event[0].what == qtefight::Ev::kGone && stopped.await == 0 && after.events == 0 && w.w.count == 0,
               "attached %d (again %d), first frame's keys %02X, prompt %02X (%d), between %02X, stopping %02X, gone %d", w.attached, w.again, started.await,
               prompt.await, prompt.prompt, between.await, stopping.await, w.gone);
    }
    {
        // The train's singles on a level's own bus: proven another blueprint's once, answered as before; a single on no bus at all as before too.
        Image m;
        const uint32_t level = LevelBus(m, 0xE7380000u, "Levels/SP_Finale/TrainRide_SCM", 412);
        g_triggerBus = level;
        const uint32_t climb = Entity(m, 0x40), jump = Entity(m, 0x0C);
        g_triggerBus = 0;
        const uint32_t grapple = Entity(m, 0x3D);  // (+0x0C 0)
        Manager(m, {Entity(m, 4), Entity(m, 5), climb, jump});
        Watching w;
        const qtefight::Frame hang = w.Frame(m, true, true);
        for (int i = 0; i < 300; ++i)
            w.Frame(m, true, true);
        const int reads = m.reads;
        w.Frame(m, true, true);
        const int aFrame = m.reads - reads;
        Manager(m, {Entity(m, 4), Entity(m, 5), grapple});
        const qtefight::Frame grab = w.Frame(m);
        Report("23b", "the train's hanging (JumpClimb and play's Jump on the level's own bus): proven another blueprint's once (by its name), Space awaited as "
                      "before, never attached again while it stays; the grapple on no bus: the left mouse button as before",
               hang.await == kSpace && !hang.prompt && w.notPrefab == 2 && w.attached == 0 && hang.event[0].attach.check == Check::kName && grab.await == kLmb &&
                   aFrame < 20,
               "hanging %02X, not the prefab %d (check %d), a frame's reads %d, grapple %02X", hang.await, w.notPrefab, static_cast<int>(hang.event[0].attach.check),
               aFrame, grab.await);
    }
    {
        // A fight whose gate isn't there yet at its first frames: its six press nothing (never a guess), said once; tried again every 0.25 s; attached.
        FightImage g;
        uint32_t keep = 0;
        g.m(g.f.entity[59], &keep, 4);
        g.m.W32(g.f.entity[59], 0);
        g.Prompt(4, true);  // (a prompt up even: nothing)
        g.m.W32(g.f.entity[59], 0);
        Watching w;
        unsigned ever = 0;
        int frames = 0;
        for (; frames < 16; ++frames)  // (0.22 s)
            ever |= w.Frame(g.m).await;
        const int failedSaid = w.failed;
        g.m.W32(g.f.entity[59], keep);
        int until = 0;
        while (!w.attached && until++ < 40)
            w.Frame(g.m);
        const qtefight::Frame after = w.last;
        Report("23c", "a fight not yet readable (a gate's entity not there): its six press nothing, said once; tried again after 0.25 s, then attached and its "
                      "prompt answered",
               ever == 0 && failedSaid == 1 && w.attached == 1 && until >= 1 && until <= 5 && after.answer == kSpace,
               "keys meanwhile %02X, said %d, attached after %d more frames, answer %02X", ever, failedSaid, until, after.answer);
    }
    {
        // A checkpoint's load: the old bus gone, a new one at the same address with new triggers: attached anew; the same triggers again: not.
        FightImage g;
        Watching w;
        w.Frame(g.m);
        g.Prompt(1, true);
        const qtefight::Frame old = w.Frame(g.m);
        g.m.W32(kGlobal, 0);  // (the load: no manager)
        const qtefight::Frame loading = w.Frame(g.m);
        g.Prompt(1, false);
        // The new instance where the old one was, its triggers new entities.
        for (int a = 0; a < 6; ++a) {
            const uint32_t moved = g.f.trigger[a] + 0x20;
            for (uint32_t o = 0; o < 0x20; o += 4) {
                uint32_t v = 0;
                g.m(g.f.trigger[a] + o, &v, 4);
                g.m.W32(moved + o, v);
            }
            g.m.W32(g.f.trigger[a], 0);
            for (size_t k = 0; k < g.p.targets.size(); ++k)
                if (g.p.targets[k] == g.p.data[qtefight::kActions[a].trigger])
                    g.m.W32(g.f.ents + 4 * static_cast<uint32_t>(k), moved);
            g.f.entity[qtefight::kActions[a].trigger] = moved;
            g.f.trigger[a] = moved;
        }
        g.Enable(true);
        const qtefight::Frame reloaded = w.Frame(g.m);
        g.Prompt(5, true);
        const qtefight::Frame z = w.Frame(g.m);
        Report("23d", "a checkpoint's load (no manager, then a new instance at the old bus's place with new triggers): forgotten, attached anew, its prompt answered",
               old.answer == kLmb && loading.events == 1 && loading.event[0].what == qtefight::Ev::kGone && reloaded.events == 1 &&
                   reloaded.event[0].what == qtefight::Ev::kAttached && z.answer == kZ && w.attached == 2,
               "old %02X, gone %d, attached %d, new prompt %02X", old.answer, w.gone, w.attached, z.answer);
    }
    {
        // A frame that doesn't read as attached, mid-fight: nothing at all pressed that frame (a single's key neither), attached again the next.
        FightImage g;
        g_triggerBus = LevelBus(g.m, 0xE7380000u, "Levels/SP_Paris/GasEventSluice_Logic_Schematic", 300);
        const uint32_t gas = Entity(g.m, 0x3C);
        g_triggerBus = 0;
        std::vector<uint32_t> enabled = {g.yaw, g.pitch, gas};
        for (uint32_t t : g.f.trigger)
            enabled.push_back(t);
        Manager(g.m, enabled);
        Watching w;
        const qtefight::Frame single = w.Frame(g.m);
        g.Prompt(0, true);
        const qtefight::Frame prompt = w.Frame(g.m);
        g.m.W32(g.f.entity[8], 0);  // (a gate's vtable gone for a frame)
        const qtefight::Frame torn = w.Frame(g.m);
        g.m.W32(g.f.entity[8], kFightTypes.gate);
        const qtefight::Frame back = w.Frame(g.m);
        g.Prompt(0, false);
        const qtefight::Frame after = w.Frame(g.m);
        Report("23e", "a fight's frame not as attached (a gate's vtable gone a frame): nothing at all pressed that frame, the single's E neither; attached again "
                      "the next, its prompt answered; between prompts the gas event's E as before",
               single.await == kE && !single.prompt && prompt.prompt && prompt.answer == kE && torn.unknown && torn.await == 0 && !torn.prompt && back.prompt &&
                   back.answer == kE && w.lost == 1 && w.again == 1 && after.await == kE && !after.prompt,
               "single %02X, prompt %02X, torn %02X (unknown %d), back %02X, after %02X", single.await, prompt.answer, torn.await, torn.unknown, back.answer,
               after.await);
    }
    {
        // The reader off (weapon.txt qtefight 0, or a proof not borne out): a fight's six press nothing, said once; a single as before.
        FightImage g;
        g_triggerBus = LevelBus(g.m, 0xE7380000u, "Levels/SP_Bank/BlockedDoor01", 200);
        const uint32_t door = Entity(g.m, 0x3C);
        g_triggerBus = 0;
        Watching w;
        unsigned ever = 0;
        g.Prompt(2, true);
        for (int i = 0; i < 200; ++i)
            ever |= w.Frame(g.m, false).await;
        std::vector<uint32_t> enabled = {g.yaw, g.pitch, door};
        Manager(g.m, enabled);
        const qtefight::Frame doorFrame = w.Frame(g.m, false);
        Report("23f", "the reader off: a fight's six press nothing whatever its gates say, said once; the bank's blocked door (one E) as before",
               ever == 0 && w.off == 1 && w.attached == 0 && doorFrame.await == kE && !doorFrame.prompt, "keys %02X, said %d, the door %02X", ever, w.off,
               doorFrame.await);
    }
    {
        // More buses than are watched: no fault; a bus with all six live and not watched presses nothing; the singles' buses as before.
        Image m;
        std::vector<uint32_t> enabled;
        for (int b = 0; b < 12; ++b) {
            g_triggerBus = LevelBus(m, 0xE6000000u + static_cast<uint32_t>(b) * 0x10000u, "Levels/Many", 50);
            enabled.push_back(Entity(m, 0x3C + static_cast<uint32_t>(b % 2)));
        }
        g_triggerBus = 0xE6F00000u;  // (a bus not readable, all six on it)
        for (uint32_t a = 0x3C; a <= 0x41; ++a)
            enabled.push_back(Entity(m, a));
        g_triggerBus = 0;
        Manager(m, enabled);
        Watching w;
        qtefight::Frame f;
        for (int i = 0; i < 50; ++i)
            f = w.Frame(m);
        Report("23g", "13 buses with quick-time triggers (8 watched): no fault; the singles' keys as before; six live on a bus not readable: nothing of them",
               f.await == (kE | kLmb) && !f.prompt && w.w.count == qtefight::kMostBuses, "keys %02X, watched %d", f.await, w.w.count);
    }
    {
        // Two fights' buses at once (Comrades: the corridor's never stopped, then the stairs'): the stale one's gates shut, the live one's prompt answered; a
        // prompt in each at once of different keys: nothing.
        FightImage g;
        const Fighting second = MakeInstance(g.m, g.p, kFightAt2);
        std::vector<uint32_t> enabled = {g.yaw, g.pitch};
        for (uint32_t t : g.f.trigger)
            enabled.push_back(t);
        for (uint32_t t : second.trigger)
            enabled.push_back(t);
        Manager(g.m, enabled);
        Watching w;
        w.Frame(g.m);
        g.m.W8(second.entity[qtefight::kActions[4].right] + 0x14, 1);
        g.m.W8(second.entity[qtefight::kActions[4].wrong] + 0x14, 1);
        const qtefight::Frame stairs = w.Frame(g.m);
        g.Prompt(1, true);
        const qtefight::Frame both = w.Frame(g.m);
        Report("23h", "two fights live (one stale, its gates shut): the other's prompt answered; prompts of two keys at once across them: nothing",
               w.attached == 2 && stairs.answer == kSpace && stairs.fights == 2 && both.prompt && both.answer == 0 && both.gates.why == qtefight::Why::kTwo,
               "attached %d, stairs %02X, both %02X (%d)", w.attached, stairs.answer, both.answer, static_cast<int>(both.gates.why));
    }
    {
        // (Both reviews of 2026-11-18.) A fight never stopped, won with its last prompt's gates left open: its success shut its fail gate. No prompt (said
        // once): 20 s of free play with the left grip at magazines every 3 s press nothing, the trigger's left mouse button, the zoom and the jump all
        // go; the gas event's E (one trigger, its own bus) answered; a new prompt (its fail gate opened again with it, #84) answered at once.
        FightImage g;
        Watching w;
        qtefight::Guard guard;
        Run2 run;
        run.game = 0;
        w.Frame(g.m);
        g.Prompt(1, true);  // (the quick-time Fire's prompt)
        const qtefight::Frame live = w.Frame(g.m);
        g.FailGate(false);  // (won: the animation's success; Fire's gates never lowered)
        int promptFrames = 0, lmb = 0, heldUp = 0, pulls = 0;
        unsigned was = 0;
        for (int i = 0; i < 72 * 20; ++i) {
            const qtefight::Frame f = w.Frame(g.m);
            promptFrames += f.prompt;
            run.await = f.await, run.hold = f.hold, run.mash = !f.prompt || f.mash;
            qtefight::GuardFrame(guard, f.unknown, f.prompt, f.answer, run.now);
            const bool magazine = (run.now % 3000) < 400;
            run.Frame(magazine ? 1.0f : 0.0f, 1.0f);
            lmb += (run.keys & kLmb) && !(was & kLmb);
            was = run.keys;
            if ((run.now % 1000) < kFrame) {
                ++pulls;
                heldUp += qtefight::GuardKey(guard, kLmb, true, run.now).drop + qtefight::GuardKey(guard, kRmb, true, run.now).drop +
                          qtefight::GuardKey(guard, kSpace, true, run.now).drop;
            }
        }
        const int disarmedSaid = w.disarmed;
        g_triggerBus = LevelBus(g.m, 0xE7380000u, "Levels/SP_Paris/GasEventSluice_Logic_Schematic", 300);
        const uint32_t gas = Entity(g.m, 0x3C);
        g_triggerBus = 0;
        std::vector<uint32_t> enabled = {g.yaw, g.pitch, gas};
        for (uint32_t t : g.f.trigger)
            enabled.push_back(t);
        Manager(g.m, enabled);
        const qtefight::Frame gasFrame = w.Frame(g.m);
        g.Prompt(1, false);
        g.Prompt(2, true);
        g.FailGate(true);  // (a prompt again: Block's, the fail gate opened with it)
        const qtefight::Frame again = w.Frame(g.m);
        Report("23i", "a never-stopped fight won with its last prompt's gates left open (its fail gate shut): no prompt, said once; 20 s of free play: the "
                      "magazine squeezes press nothing, the trigger, zoom and jump all go; the gas event's E answered; a new prompt with its fail gate open: answered",
               live.answer == kLmb && promptFrames == 0 && lmb == 0 && heldUp == 0 && pulls >= 19 && disarmedSaid == 1 && gasFrame.await == kE &&
                   !gasFrame.prompt && again.prompt && again.answer == kRmb,
               "live %02X; prompt frames %d, left clicks %d, held up %d of %d, said %d; the gas %02X; again %02X", live.answer, promptFrames, lmb, heldUp, 3 * pulls,
               disarmedSaid, gasFrame.await, again.answer);
    }
    {
        // Open gates standing unchanged with the fail gate open (a fight nothing runs any more, or a beat waiting with no timeout): answered for a minute of
        // the step's own time, then taken as over, said once: no prompt, nothing held up, version 2's other keys back; its gates changing: a prompt again
        // at once. A five-minute pause between two steps (the headset off) counts 0.1 s.
        FightImage g;
        g_triggerBus = LevelBus(g.m, 0xE7380000u, "Levels/SP_Paris/GasEventSluice_Logic_Schematic", 300);
        const uint32_t gas = Entity(g.m, 0x3C);
        g_triggerBus = 0;
        std::vector<uint32_t> enabled = {g.yaw, g.pitch, gas};
        for (uint32_t t : g.f.trigger)
            enabled.push_back(t);
        Manager(g.m, enabled);
        Watching w;
        w.Frame(g.m);
        g.Prompt(3, true);  // (F's prompt, standing)
        int frames = 0;
        while (w.Frame(g.m).answer == kF && frames < 10000)
            ++frames;
        const qtefight::Frame over = w.last;
        int after = 0;
        for (int i = 0; i < 200; ++i)
            after += w.Frame(g.m).prompt;
        const int staleSaid = w.stale;
        g.Prompt(3, false);
        g.Prompt(0, true);  // (another prompt: E's)
        const qtefight::Frame next = w.Frame(g.m);
        const unsigned long long answeredMs = static_cast<unsigned long long>(frames) * kFrame;
        FightImage h;
        Watching v;
        v.Frame(h.m);
        h.Prompt(2, true);
        v.Frame(h.m);
        v.now += 300000;  // (five minutes with no step)
        const qtefight::Frame late = v.Frame(h.m);
        Report("23j", "open gates standing unchanged, the fail gate open: answered a minute of the step's time, then taken as over (said once): no prompt, the "
                      "gas event's E back; the gates changing: answered at once; five minutes with no step count 0.1 s",
               answeredMs >= qtefight::kStaleArmedMs - 2 * kFrame && answeredMs <= qtefight::kStaleArmedMs + 2 * kFrame && !over.prompt && over.answer == 0 &&
                   over.await == kE && after == 0 && staleSaid == 1 && next.prompt && next.answer == kE && late.answer == kRmb && v.stale == 0,
               "answered %llu ms, then prompt %d (keys %02X), %d frames up after, said %d; next %02X; after the pause %02X", answeredMs, over.prompt, over.await,
               after, staleSaid, next.answer, late.answer);
    }
    {
        // The fail gate not as the prefab has it (attached without it): each prompt counted 10 s at most, then taken as over (said once).
        FightImage g;
        g.m.W8(g.f.entity[qtefight::kFail] + 0x14, 0);
        g.m.W8(g.p.data[qtefight::kFail] + 0x10, 0);
        Watching w;
        w.Frame(g.m);
        const bool without = w.w.count == 1 && w.w.known[0].fight.fail == 0 && w.attached == 1;
        g.Prompt(5, true);  // (Z's prompt)
        int frames = 0;
        while (w.Frame(g.m).answer == kZ && frames < 10000)
            ++frames;
        const unsigned long long answeredMs = static_cast<unsigned long long>(frames) * kFrame;
        Report("23k", "no fail gate attached: each prompt answered 10 s of the step's time at most, then taken as over, said once",
               without && answeredMs >= qtefight::kStaleMs - 2 * kFrame && answeredMs <= qtefight::kStaleMs + 2 * kFrame && !w.last.prompt && w.stale == 1,
               "attached without it %d, answered %llu ms, said %d", without, answeredMs, w.stale);
    }
    {
        // A fight's bus whose attach fails past the prefab's own checks (a gate's data not as it has it) with only three of its six triggers listed (the
        // list capped): version 2 presses none of them (they were pressed together, failing the fight). And one whose bus couldn't be read at its first
        // frame (taken for another blueprint's): tried again while all six are live, attached within 0.25 s, its prompt answered.
        FightImage g;
        g.m.W32(g.p.data[7], kFightTypes.selectData);
        Manager(g.m, {g.yaw, g.pitch, g.f.trigger[0], g.f.trigger[1], g.f.trigger[4]});
        Watching w;
        unsigned ever = 0;
        for (int i = 0; i < 50; ++i)
            ever |= w.Frame(g.m).await;
        const bool skipped = w.last.skip.Has(g.f.bus) && w.failed == 1 && w.lastFailed == Check::kGate;
        FightImage u;
        const std::vector<uint8_t> page = u.m.pages[u.f.bus >> 12];
        u.m.Drop(u.f.bus);
        Watching v;
        v.Frame(u.m);
        u.m.pages[u.f.bus >> 12] = page;
        u.Prompt(3, true);
        int until = 0;
        while (!v.attached && until++ < 40)
            v.Frame(u.m);
        Report("23l", "a fight's bus failing past the prefab's checks with three of its six triggers listed: none of their keys pressed; a bus not readable at its "
                      "first frame: tried again while all six are live, attached within 0.25 s, its prompt answered",
               ever == 0 && skipped && v.notPrefab == 1 && v.attached == 1 && until <= 19 && v.last.answer == kF,
               "keys %02X, skipped %d; not the prefab %d, attached after %d frames, answer %02X", ever, skipped, v.notPrefab, until, v.last.answer);
    }
    {
        // The list not read for a moment mid-fight (its manager unreadable): nothing pressed, unknown (the guard as it was: a prompt may be up); the fight
        // forgotten as before, attached again and its prompt answered as the list comes back.
        FightImage g;
        Watching w;
        w.Frame(g.m);
        g.Prompt(2, true);
        const qtefight::Frame before = w.Frame(g.m);
        const std::vector<uint8_t> page = g.m.pages[kMgr >> 12];
        g.m.Drop(kMgr);
        const qtefight::Frame unread = w.Frame(g.m);
        g.m.pages[kMgr >> 12] = page;
        const qtefight::Frame back = w.Frame(g.m);
        Report("23m", "the list not read mid-fight (its manager unreadable): nothing pressed, unknown (the guard as it was); the fight forgotten as before, "
                      "attached again and answered as the list comes back",
               before.answer == kRmb && unread.unknown && unread.await == 0 && !unread.prompt && back.answer == kRmb && w.gone == 1 && w.attached == 2,
               "before %02X, unread %d (%02X), back %02X, gone %d, attached %d", before.answer, unread.unknown, unread.await, back.answer, w.gone, w.attached);
    }
}

// A fist fight as the game runs it (design section 1): its six triggers live from its Start; the animation raises one action's prompt at a time (its
// right and its wrong gate open, the select set), and lowers it a moment after the press is done (a mash: its count's presses), or at its window's end,
// which fails the fight; a press goes through its trigger to its right gate and every OTHER action's wrong gate: an open wrong gate FAILS the fight.
// Its input read 30 times a second (a key's press: it went down since the last read).
struct Beat {
    int action;
    bool mash;
    unsigned long long window;
};
struct FightGame {
    FightImage g;
    std::vector<Beat> beats;
    int beat = 0;
    enum class Phase { kStart, kGap, kOpen, kClosing, kWon, kFailed } phase = Phase::kStart;
    unsigned long long phaseAt = 0, nextTick = 0, gapMs = 800, closeMs = 300, startMs = 500;
    unsigned tickKeys = 0;
    int count = 10;
    int presses[qte::kKeyCount] = {}, rightPresses = 0, wrongPresses = 0, betweenPresses = 0;
    const char* failed = "";
    bool neverStopped = false;  // its six left live after it (the meat market, the corridor, the ending)
    bool lastLeftOpen = false;  // its last prompt's gates never lowered: the animation's success comes with them still open

    explicit FightGame(std::vector<Beat> b) : beats(std::move(b)) { g.Enable(false); }
    bool Over() const { return phase == Phase::kWon || phase == Phase::kFailed; }
    void Fail(const char* why) {
        if (phase != Phase::kFailed)
            failed = why;
        phase = Phase::kFailed;
        g.FailGate(false);  // (its Out fired, and it shut itself: #97)
        for (int a = 0; a < 6; ++a)
            g.Prompt(a, false);
    }
    void Win() {
        phase = Phase::kWon;
        g.FailGate(false);  // (the animation's success signal: #1 -> #71 shuts it)
        if (!neverStopped)
            g.Enable(false);  // (its Stop)
    }
    void Step(unsigned long long now, unsigned keys) {
        if (phase == Phase::kStart && now >= startMs + 100000) {
            g.Enable(true);
            phase = Phase::kGap;
            phaseAt = now;
        }
        if (now >= nextTick) {
            nextTick = now + 33;
            const unsigned went = keys & ~tickKeys;
            tickKeys = keys;
            const bool live = phase != Phase::kStart && (!Over() || neverStopped);
            for (int k = 0; k < 6 && live; ++k) {
                if (!(went >> k & 1))
                    continue;
                ++presses[k];
                const Beat* b = phase == Phase::kOpen || phase == Phase::kClosing ? &beats[static_cast<size_t>(beat)] : nullptr;
                if (!b) {
                    ++betweenPresses;  // (every gate shut: nothing)
                    continue;
                }
                if (b->action != k) {
                    ++wrongPresses;
                    Fail("a wrong key while a prompt was up");
                    break;
                }
                ++rightPresses;
                if (phase == Phase::kOpen && (!b->mash || --count == 0)) {
                    count = 10;
                    phase = Phase::kClosing;
                    phaseAt = now;
                }
                g.Count(b->mash ? count : 10);
            }
        }
        if (phase == Phase::kGap && now >= phaseAt + gapMs) {
            if (beat >= static_cast<int>(beats.size())) {
                Win();
                return;
            }
            const Beat& b = beats[static_cast<size_t>(beat)];
            g.Select(b.mash ? 1 : 0);
            g.Count(10);
            count = 10;
            g.Prompt(b.action, true);
            g.FailGate(true);  // (#84, as each prompt opens)
            phase = Phase::kOpen;
            phaseAt = now;
        } else if (phase == Phase::kOpen && now >= phaseAt + beats[static_cast<size_t>(beat)].window) {
            Fail("a prompt's window ran out");
        } else if (phase == Phase::kClosing && now >= phaseAt + closeMs) {
            if (lastLeftOpen && beat + 1 == static_cast<int>(beats.size())) {
                Win();  // (its gates left as they are)
                return;
            }
            g.Prompt(beats[static_cast<size_t>(beat)].action, false);
            ++beat;
            phase = Phase::kGap;
            phaseAt = now;
        }
    }
};

// The mod's side of a fight's frame, as qtewatch.cpp and input.cpp run it: the list read and steadied, the fights' step, the grips (version 2's Step2, a
// fight's press one tap a squeeze); other: another input's quick-time keys this frame (the right trigger's fire...), through the guard unless guard is
// false. The keys the game is sent.
struct FightMod {
    qtelist::Steady steady;
    qtefight::Watch w;
    qtefight::Guard guard;
    Run2 run;
    bool reader = true, useGuard = true, oldV2 = false;
    // In a scripted moment, as every fight's prompt in the game was (the interrogation's, 2026-11-18; the cellar's, 2026-11-19: "[a scripted moment]").
    bool scripted = true;
    qte::Watch2 watch;  // (the grips' log, as input.cpp keeps it)
    int watchLines = 0, watchEnds = 0;
    // The other quick-time keys the game's input shows while a prompt is up (qte::OthersStep, as qtewatch.cpp's SayPrompts runs it): what was said.
    qte::Others others;
    unsigned othersAtOpen = 0, othersWent = 0, othersAsClosed = 0, othersWhileUp = 0;
    int othersClosed = 0;
    unsigned otherDown = 0;  // the other input's keys the game was sent down
    int heldUp = 0, saidHeldUp = 0, promptFrames = 0, keysOutsidePrompts = 0;
    unsigned proper = 0;  // the frame's keys a quick-time action proper awaits (qtewatch.cpp's Frame::proper: the key each goes down as, input.cpp)
    unsigned Frame(FightGame& game, float left, float right, unsigned other = 0) {
        const qtelist::List now = ReadList(game.g.m);
        const qtelist::List& list = qtelist::Settle(steady, now, run.now);
        const qtefight::Frame f = qtefight::Step(w, game.g.m, kFightTypes, list, scripted, run.now, reader);
        if (oldV2) {  // (version 2 before the fights' reader: every live trigger's key)
            unsigned all = 0;
            for (int i = 0; i < list.count; ++i)
                all |= qtelist::KeyOf(list.at[i].action);
            run.await = all & qte::kKeysV2;
            run.hold = 0;
            run.mash = true;
            run.prompt = run.once = run.unknown = run.scripted = false;  // (and none of 2026-11-19's held grips)
            run.quick = 0;
        } else {  // (as input.cpp's QteStep)
            run.await = f.await;
            run.hold = f.hold;
            run.mash = !f.prompt || f.mash;
            run.prompt = f.prompt;
            run.once = f.prompt && !f.mash;
            run.unknown = f.unknown;
            run.quick = f.prompt || f.unknown ? 0u : qtelist::QuickKeys(list, &f.skip) & f.await;  // (as qtewatch.cpp's Update)
            proper = f.prompt || f.unknown ? 0u : qtelist::ProperKeys(list, &f.skip) & f.await;
            run.gates = f.prompt ? f.gates.right | f.gates.wrong : 0u;
            run.scripted = scripted;
        }
        qtefight::GuardFrame(guard, f.unknown, f.prompt, f.answer, run.now);  // (as input.cpp's QteStep)
        promptFrames += f.prompt;
        run.game = 3;
        run.external = game.tickKeys;
        if (!f.unknown) {  // (qtewatch.cpp's SayPrompts, before the grips: the input as the game's last tick took it)
            const qte::OthersOut oo = qte::OthersStep(others, f.prompt ? f.answer : 0u, game.tickKeys, true);
            othersAtOpen |= oo.atOpen;
            othersWent |= oo.went;
            othersAsClosed |= oo.wentAsClosed;
            if (oo.closed) {
                ++othersClosed;
                othersWhileUp |= oo.wentWhileUp;
            }
        }
        const unsigned grips = run.Frame(left, right).keys;
        const qte::WatchOut wo = qte::WatchStep(watch, run.lastIn, run.last);
        watchLines += wo.say;
        watchEnds += wo.sayEnd;
        if (!f.prompt && grips && !f.await)
            ++keysOutsidePrompts;
        // The other input: each of its keys down as it changes, through the guard (a release always goes).
        for (int k = 0; k < 6; ++k) {
            const unsigned bit = 1u << k;
            if ((other & bit) && !(otherDown & bit)) {
                const qtefight::Pass p = qtefight::GuardKey(guard, bit, true, run.now - kFrame);
                if (useGuard && p.drop) {
                    ++heldUp;
                    saidHeldUp += p.say;
                } else {
                    otherDown |= bit;
                }
            } else if (!(other & bit)) {
                otherDown &= ~bit;
            }
        }
        const unsigned sent = grips | otherDown;
        game.Step(run.now - kFrame, sent);
        return sent;
    }
};

std::vector<Beat> SixPresses() {
    return {{2, false, 2000}, {1, false, 2000}, {4, false, 2000}, {0, false, 2000}, {3, false, 2000}, {5, false, 2000}};
}

void Fights() {
    {
        // Six press prompts, one of each action, answered by the left grip squeezed 0.4 s after each opens, the right let go (a grip held from before
        // presses a prompt at once since 2026-11-19: 27a).
        FightGame game(SixPresses());
        FightMod mod;
        unsigned long long openedAt = 0;
        int lastBeat = -1;
        for (int i = 0; i < 2000 && !game.Over(); ++i) {
            if (game.phase == FightGame::Phase::kOpen && game.beat != lastBeat) {
                lastBeat = game.beat;
                openedAt = mod.run.now;
            }
            const bool squeeze = game.phase == FightGame::Phase::kOpen && mod.run.now >= openedAt + 400 && mod.run.now < openedAt + 700;
            mod.Frame(game, squeeze ? 1.0f : 0.0f, 0.0f);
        }
        bool once = true;
        for (int k = 0; k < 6; ++k)
            once = once && game.presses[k] == 1;
        Report("24a", "a fight of six press prompts (Block, the quick-time Fire, JumpClimb, InteractDrag, FastMelee, CrouchDuck), a fresh squeeze of the left grip "
                      "each: won; each key pressed once, in its own prompt; never a wrong key; nothing between prompts",
               game.phase == FightGame::Phase::kWon && once && game.wrongPresses == 0 && game.betweenPresses == 0 && mod.run.ever == qte::kKeysV2 &&
                   mod.run.began == 6 && mod.run.byHold == 0 && mod.keysOutsidePrompts == 0,
               "%s%s; presses E %d LMB %d RMB %d F %d Space %d Z %d, wrong %d, between %d, squeezes %d", game.phase == FightGame::Phase::kWon ? "won" : "NOT WON: ",
               game.failed, game.presses[0], game.presses[1], game.presses[2], game.presses[3], game.presses[4], game.presses[5], game.wrongPresses,
               game.betweenPresses, mod.run.began);
    }
    {
        // A mash prompt (Block, 10 presses) between two presses, the grip held from 0.4 s after it opens until the game awaits it no more.
        FightGame game({{1, false, 2000}, {2, true, 3000}, {4, false, 2000}});
        FightMod mod;
        unsigned long long openedAt = 0, mashBegan = 0, mashDone = 0;
        int lastBeat = -1;
        for (int i = 0; i < 2000 && !game.Over(); ++i) {
            if (game.phase == FightGame::Phase::kOpen && game.beat != lastBeat) {
                lastBeat = game.beat;
                openedAt = mod.run.now;
            }
            const bool mashing = game.beat == 1;
            const bool squeeze = game.phase == FightGame::Phase::kOpen
                                     ? mod.run.now >= openedAt + 400 && (mashing || mod.run.now < openedAt + 700)
                                     : mashing && game.phase == FightGame::Phase::kClosing;
            const int before = game.presses[2];
            mod.Frame(game, squeeze ? 1.0f : 0.0f, 0.0f);
            if (game.presses[2] && !before)
                mashBegan = mod.run.now;
            if (game.beat == 1 && game.phase == FightGame::Phase::kClosing && !mashDone)
                mashDone = mod.run.now;
        }
        Report("24b", "a mash prompt (Block, its count from 10) between two presses: the grip held: the right mouse button every 130 ms, the count done in about "
                      "1.3 s, let go as the game awaits it no more; won, never a wrong key",
               game.phase == FightGame::Phase::kWon && game.wrongPresses == 0 && game.presses[2] >= 10 && game.presses[2] <= 13 && mashDone - mashBegan >= 1100 &&
                   mashDone - mashBegan <= 1400 && game.presses[1] == 1 && game.presses[4] == 1,
               "%s%s; Block presses %d, the count done %llu ms after the first, wrong %d", game.phase == FightGame::Phase::kWon ? "won" : "NOT WON: ", game.failed,
               game.presses[2], mashDone - mashBegan, game.wrongPresses);
    }
    {
        // The player: a squeeze held from the fight's start throughout (never fresh): each prompt pressed at once as it opens, once, the fight won (till
        // 2026-11-19 nothing was: the owner's cellar fight); another squeezing in the gaps between prompts and holding one squeeze from a prompt into the
        // next: nothing between prompts, the next prompt pressed at once by the squeeze held into it, once (a fresh squeeze 0.8 s in, the prompt closed by
        // then: nothing).
        FightGame held(SixPresses());
        FightMod a;
        for (int i = 0; i < 2000 && !held.Over(); ++i)
            a.Frame(held, 1.0f, 1.0f);
        bool heldOnce = true;
        for (int k = 0; k < 6; ++k)
            heldOnce = heldOnce && held.presses[k] == 1;
        FightGame gaps({{2, false, 2000}, {1, false, 2500}});
        FightMod b;
        unsigned long long gapAt = 0, openedAt = 0;
        int lastBeat = -1;
        FightGame::Phase was = FightGame::Phase::kStart;
        int heldIntoNext = 0;
        for (int i = 0; i < 2000 && !gaps.Over(); ++i) {
            if (gaps.phase == FightGame::Phase::kGap && was != FightGame::Phase::kGap)
                gapAt = b.run.now;
            if (gaps.phase == FightGame::Phase::kOpen && gaps.beat != lastBeat) {
                lastBeat = gaps.beat;
                openedAt = b.run.now;
            }
            was = gaps.phase;
            bool squeeze = false;
            if (gaps.phase == FightGame::Phase::kGap)
                squeeze = b.run.now >= gapAt + 200 && b.run.now < gapAt + 400;  // (a squeeze in the gap: nothing)
            if (gaps.beat == 0 && gaps.phase != FightGame::Phase::kGap)
                squeeze = b.run.now >= openedAt + 300;  // (the first prompt's squeeze held on ...)
            if (gaps.beat == 1 && gaps.phase == FightGame::Phase::kGap)
                squeeze = true;  // (... through the gap into the next prompt)
            if (gaps.beat == 1 && (gaps.phase == FightGame::Phase::kOpen || gaps.phase == FightGame::Phase::kClosing)) {
                squeeze = b.run.now < openedAt + 500 || (b.run.now >= openedAt + 800 && b.run.now < openedAt + 1000);  // (let go at 0.5 s, a fresh one at 0.8)
                heldIntoNext += b.run.now < openedAt + 500 && gaps.presses[1] > 0;
            }
            b.Frame(gaps, squeeze ? 1.0f : 0.0f, 0.0f);
        }
        Report("24c", "a grip held from the fight's start: each prompt pressed at once, once, won (as it was: nothing, the fight failed by its window); "
                      "squeezes in the gaps between prompts: nothing; a squeeze held from one prompt into the next: pressed at once, once",
               held.phase == FightGame::Phase::kWon && heldOnce && held.wrongPresses == 0 && held.betweenPresses == 0 && a.run.byHold == 6 && a.run.ever == qte::kKeysV2 &&
                   gaps.phase == FightGame::Phase::kWon && gaps.betweenPresses == 0 && gaps.wrongPresses == 0 && gaps.presses[2] == 1 && gaps.presses[1] == 1 &&
                   heldIntoNext > 0 && b.run.byHold == 1,
               "held: %s%s, by the grip squeezed already %d, keys %02X; gaps: %s%s, between %d, Block %d, Fire %d (frames with Fire pressed before the let-go %d)",
               held.phase == FightGame::Phase::kWon ? "won" : "NOT WON: ", held.failed, a.run.byHold, a.run.ever,
               gaps.phase == FightGame::Phase::kWon ? "won" : "NOT WON: ", gaps.failed, gaps.betweenPresses, gaps.presses[2], gaps.presses[1], heldIntoNext);
    }
    {
        // Why the reader: version 2 as it was (every live trigger's key at once) fails the fight at its first squeeze; the reader off presses nothing.
        FightGame game(SixPresses());
        FightMod mod;
        mod.oldV2 = true;
        unsigned long long openedAt = 0;
        int lastBeat = -1;
        for (int i = 0; i < 2000 && !game.Over(); ++i) {
            if (game.phase == FightGame::Phase::kOpen && game.beat != lastBeat) {
                lastBeat = game.beat;
                openedAt = mod.run.now;
            }
            const bool squeeze = game.phase == FightGame::Phase::kOpen && mod.run.now >= openedAt + 400 && mod.run.now < openedAt + 700;
            mod.Frame(game, squeeze ? 1.0f : 0.0f, 1.0f);
        }
        FightGame off(SixPresses());
        FightMod quiet;
        quiet.reader = false;
        for (int i = 0; i < 2000 && !off.Over(); ++i)
            quiet.Frame(off, (i % 40) < 20 ? 1.0f : 0.0f, 1.0f);
        Report("24d", "version 2 as it was (all six at once): the fight failed at the first squeeze; the reader off (qtefight 0): nothing pressed at all, the fight "
                      "fails by its window as untouched",
               game.phase == FightGame::Phase::kFailed && game.wrongPresses > 0 && game.beat == 0 && off.phase == FightGame::Phase::kFailed && quiet.run.ever == 0 &&
                   off.wrongPresses == 0,
               "as it was: %s after %d prompt%s (wrong %d); off: keys %02X, %s", game.failed, game.beat, game.beat == 1 ? "" : "s", game.wrongPresses, quiet.run.ever,
               off.failed);
    }
    {
        // A fight never stopped (the meat market, the corridor, the ending): its six live on in free play, its gates shut. 10 000 frames of both grips at
        // play press nothing; a single in the same level (the gas event's E on its own bus) still answered, and the stale fight's E trigger harms nothing.
        FightGame game({{0, false, 2000}});
        game.neverStopped = true;
        FightMod mod;
        unsigned long long openedAt = 0;
        int lastBeat = -1;
        for (int i = 0; i < 400 && !game.Over(); ++i) {
            if (game.phase == FightGame::Phase::kOpen && game.beat != lastBeat) {
                lastBeat = game.beat;
                openedAt = mod.run.now;
            }
            mod.Frame(game, game.phase == FightGame::Phase::kOpen && mod.run.now >= openedAt + 300 && mod.run.now < openedAt + 600 ? 1.0f : 0.0f, 0.0f);
        }
        const bool won = game.phase == FightGame::Phase::kWon;
        const unsigned everAfterWin = mod.run.ever;
        Run2& r = mod.run;
        r.ever = 0;
        const int beganBefore = r.began;
        for (int i = 0; i < 10000; ++i)
            mod.Frame(game, 0.5f + 0.5f * std::sin(i * 0.05f), i % 700 < 650 ? 1.0f : 0.0f);
        const unsigned freePlay = r.ever;
        const int freeSqueezes = r.began - beganBefore;
        // The gas event comes: its E on its own bus.
        g_triggerBus = LevelBus(game.g.m, 0xE7380000u, "Levels/SP_Paris/GasEventSluice_Logic_Schematic", 300);
        const uint32_t gas = Entity(game.g.m, 0x3C);
        g_triggerBus = 0;
        std::vector<uint32_t> enabled = {game.g.yaw, game.g.pitch, gas};
        for (uint32_t t : game.g.f.trigger)
            enabled.push_back(t);
        Manager(game.g.m, enabled);
        for (int i = 0; i < 10; ++i)
            mod.Frame(game, 0.0f, 0.0f);
        for (int i = 0; i < 20; ++i)
            mod.Frame(game, 1.0f, 0.0f);
        Report("24e", "a fight never stopped: won, then 10 000 frames of both grips at play with its six still live: nothing pressed; the gas event's E (one "
                      "trigger, its own bus) answered by a squeeze, the stale fight's gates shut: no harm",
               won && everAfterWin == kE && freePlay == 0 && freeSqueezes == 0 && r.ever == kE && game.wrongPresses == 0,
               "won %d, after it %02X in %d squeezes, the gas %02X, wrong %d", won, freePlay, freeSqueezes, r.ever, game.wrongPresses);
    }
    {
        // The Great Destroyer's train: a fight and a single (a climb's JumpClimb on the level's bus) live together: a fight prompt up, only its key; between
        // prompts the climb's Space.
        FightGame game({{2, false, 3000}});
        g_triggerBus = LevelBus(game.g.m, 0xE7380000u, "Levels/SP_Finale/TrainRide_SCM", 412);
        const uint32_t climb = Entity(game.g.m, 0x40);
        g_triggerBus = 0;
        FightMod mod;
        unsigned duringPrompt = 0, between = 0;
        bool added = false;
        unsigned long long openedAt = 0;
        int lastBeat = -1;
        for (int i = 0; i < 600 && !game.Over(); ++i) {
            if (!added && game.phase != FightGame::Phase::kStart) {
                added = true;
                std::vector<uint32_t> enabled = {game.g.yaw, game.g.pitch, climb};
                for (uint32_t t : game.g.f.trigger)
                    enabled.push_back(t);
                Manager(game.g.m, enabled);
            }
            if (game.phase == FightGame::Phase::kOpen && game.beat != lastBeat) {
                lastBeat = game.beat;
                openedAt = mod.run.now;
            }
            const bool inPrompt = game.phase == FightGame::Phase::kOpen;
            const bool squeeze = (inPrompt && mod.run.now >= openedAt + 300 && mod.run.now < openedAt + 600) ||
                                 (game.phase == FightGame::Phase::kGap && added && (i % 60) >= 30 && (i % 60) < 40);
            const unsigned sent = mod.Frame(game, squeeze ? 1.0f : 0.0f, 0.0f);
            (inPrompt ? duringPrompt : between) |= sent;
        }
        Report("24f", "a fight and a single live together (the train's): a fight prompt up, only its key (the right mouse button); between prompts the climb's Space",
               game.phase == FightGame::Phase::kWon && duringPrompt == kRmb && (between & kSpace) && !(between & ~(kSpace | kRmb)) && game.wrongPresses == 0,
               "%s%s; during the prompt %02X, between %02X", game.phase == FightGame::Phase::kWon ? "won" : "NOT WON: ", game.failed, duringPrompt, between);
    }
    {
        // weapon.txt qtetest's grip (qte::TestSqueezed, as input.cpp runs it) over two prompts with no frame between them (Block's gates shut and Fire's open
        // in one write): each pressed once (the review of 2026-11-18: the grip as it was, held on through the change, never pressed Fire; since 2026-11-19
        // a grip held on presses the next prompt at once: once too); with a gap the same.
        const auto run = [](unsigned long long gapMs, bool asItWas, int& block, int& fire) {
            FightImage g;
            Watching w;
            Run2 r;
            r.game = 0;
            qte::TestGrip grip;
            unsigned long long since = 0;  // (the grip as it was: since a key was awaited, held on through any change)
            block = fire = 0;
            unsigned was = 0;
            const unsigned long long open = w.now + 200, swap = open + 1500, end = swap + gapMs + 1500, until = w.now + 60000;
            for (int i = 0; i < 72 * 5; ++i) {
                const unsigned long long t = w.now;
                g.Prompt(2, t >= open && t < swap);
                g.Prompt(1, t >= swap + gapMs && t < end);
                const qtefight::Frame f = w.Frame(g.m);
                r.await = f.await, r.hold = f.hold, r.mash = !f.prompt || f.mash;
                r.prompt = f.prompt, r.once = f.prompt && !f.mash, r.unknown = f.unknown, r.scripted = true;  // (as input.cpp's QteStep, in a scripted moment)
                bool squeezed = qte::TestSqueezed(grip, f.await, r.now, until);
                if (asItWas) {
                    since = !f.await ? 0 : since ? since : r.now;
                    squeezed = since && r.now >= since + qte::kTestAfter && r.now + qte::kTestEnd < until;
                }
                r.Frame(squeezed ? 1.0f : 0.0f, 0.0f);
                block += (r.keys & kRmb) && !(was & kRmb);
                fire += (r.keys & kLmb) && !(was & kLmb);
                was = r.keys;
            }
        };
        int b0 = 0, f0 = 0, b14 = 0, f14 = 0, b100 = 0, f100 = 0, bOld = 0, fOld = 0;
        run(0, false, b0, f0);
        run(14, false, b14, f14);
        run(100, false, b100, f100);
        run(0, true, bOld, fOld);
        Report("24g", "the test's grip (weapon.txt qtetest) over Block's prompt then Fire's with no frame between: each pressed once; a frame or 0.1 s between: "
                      "the same; the grip as it was, held on through the change: each once too (till 2026-11-19: Fire never)",
               b0 == 1 && f0 == 1 && b14 == 1 && f14 == 1 && b100 == 1 && f100 == 1 && bOld == 1 && fOld == 1,
               "no gap: Block %d Fire %d; 14 ms: %d %d; 100 ms: %d %d; the grip as it was: %d %d", b0, f0, b14, f14, b100, f100, bOld, fOld);
    }
    {
        // A fight never stopped, won with its last prompt's gates left open (its fail gate shut by its success): 10 000 frames of free play with both grips at
        // play and the right trigger pulled now and then: the grips press nothing, every trigger pull goes, nothing held up; nothing fails.
        FightGame game({{2, false, 2000}, {0, false, 2000}});
        game.neverStopped = true;
        game.lastLeftOpen = true;
        FightMod mod;
        unsigned long long openedAt = 0;
        int lastBeat = -1;
        for (int i = 0; i < 600 && !game.Over(); ++i) {
            if (game.phase == FightGame::Phase::kOpen && game.beat != lastBeat) {
                lastBeat = game.beat;
                openedAt = mod.run.now;
            }
            mod.Frame(game, game.phase == FightGame::Phase::kOpen && mod.run.now >= openedAt + 300 && mod.run.now < openedAt + 600 ? 1.0f : 0.0f, 0.0f);
        }
        const bool won = game.phase == FightGame::Phase::kWon;
        uint8_t eGate = 0;
        game.g.m(game.g.f.entity[qtefight::kActions[0].right] + 0x14, &eGate, 1);
        Run2& r = mod.run;
        r.ever = 0;
        const int beganBefore = r.began, heldBefore = mod.heldUp, promptBefore = mod.promptFrames, lmbBefore = game.presses[1];
        for (int i = 0; i < 10000; ++i) {
            const bool pulled = (r.now % 700) < 200;
            mod.Frame(game, 0.5f + 0.5f * std::sin(i * 0.05f), i % 700 < 650 ? 1.0f : 0.0f, pulled ? kLmb : 0);
        }
        Report("24h", "a fight never stopped, won with its last prompt's gates left open (its fail gate shut): 10 000 frames of play, the trigger pulled now and "
                      "then: the grips press nothing, every pull's left mouse button goes, nothing held up, no prompt; nothing fails",
               won && eGate == 1 && r.ever == 0 && r.began == beganBefore && mod.heldUp == heldBefore && mod.promptFrames == promptBefore &&
                   game.presses[1] - lmbBefore >= 190 && game.wrongPresses == 0,
               "won %d (E's gate still %d); grips' presses %d, keys %02X, held up %d, prompt frames %d, left clicks %d", won, eGate, r.began - beganBefore, r.ever,
               mod.heldUp - heldBefore, mod.promptFrames - promptBefore, game.presses[1] - lmbBefore);
    }
}

void Guards() {
    {
        qtefight::Guard g;
        const unsigned long long t = 5000;
        const bool before = !qtefight::GuardKey(g, kLmb, true, t).drop;
        qtefight::SetGuard(g, true, kRmb, t);  // (Block's prompt)
        const qtefight::Pass trigger = qtefight::GuardKey(g, kLmb, true, t + 1), again = qtefight::GuardKey(g, kLmb, true, t + 2);
        const qtefight::Pass prone = qtefight::GuardKey(g, kZ, true, t + 3), use = qtefight::GuardKey(g, kE, true, t + 3), jump = qtefight::GuardKey(g, kSpace, true, t + 3),
                             melee = qtefight::GuardKey(g, kF, true, t + 3);
        const qtefight::Pass answer = qtefight::GuardKey(g, kRmb, true, t + 4), up = qtefight::GuardKey(g, kLmb, false, t + 5), other = qtefight::GuardKey(g, 64, true, t + 5);
        qtefight::SetGuard(g, true, kRmb, t + 14);  // (the next frame, the same prompt)
        const qtefight::Pass third = qtefight::GuardKey(g, kLmb, true, t + 15);
        qtefight::SetGuard(g, false, 0, t + 28);  // (the prompt over)
        const bool after = !qtefight::GuardKey(g, kLmb, true, t + 29).drop && !qtefight::GuardKey(g, kZ, true, t + 29).drop;
        Report("25a", "a Block prompt up: the right trigger's left mouse button held up, said once a prompt; the stick's Z, the use button's E, Space and F held up; "
                      "the answer (the right mouse button) goes, releases go, other keys go; outside prompts everything goes",
               before && trigger.drop && trigger.say && again.drop && !again.say && prone.drop && prone.say && use.drop && jump.drop && melee.drop && !answer.drop &&
                   !up.drop && !other.drop && third.drop && !third.say && after,
               "before %d, trigger %d/%d, again %d/%d, Z %d E %d Space %d F %d, answer %d, up %d, after %d", before, trigger.drop, trigger.say, again.drop, again.say,
               prone.drop, use.drop, jump.drop, melee.drop, answer.drop, up.drop, after);
    }
    {
        qtefight::Guard g;
        qtefight::SetGuard(g, true, 0, 1000);  // (two prompts: nothing safe)
        int held = 0;
        for (int k = 0; k < 6; ++k)
            held += qtefight::GuardKey(g, 1u << k, true, 1001).drop;
        qtefight::SetGuard(g, true, kE, 1014);  // (one prompt now: E)
        const qtefight::Pass said = qtefight::GuardKey(g, kLmb, true, 1015);  // (said again: another prompt)
        const bool lapsed = !qtefight::GuardKey(g, kLmb, true, 1014 + qtefight::kGuardMs).drop;  // (not set again: the headset off)
        Report("25b", "nothing safe (two prompts): all six held up; a new prompt's first held-up key said again; not set again for 0.25 s: lapsed",
               held == 6 && said.drop && said.say && lapsed, "held %d, said again %d, lapsed %d", held, said.say, lapsed);
    }
    {
        // The guard in a fight: the right trigger pulled now and then throughout (200 ms every 0.7 s): with it, the fight won; without it, failed.
        const auto run = [](bool guard, int& heldUp, int& said) {
            FightGame game(SixPresses());
            FightMod mod;
            mod.useGuard = guard;
            unsigned long long openedAt = 0;
            int lastBeat = -1;
            for (int i = 0; i < 2000 && !game.Over(); ++i) {
                if (game.phase == FightGame::Phase::kOpen && game.beat != lastBeat) {
                    lastBeat = game.beat;
                    openedAt = mod.run.now;
                }
                const bool squeeze = game.phase == FightGame::Phase::kOpen && mod.run.now >= openedAt + 400 && mod.run.now < openedAt + 700;
                const bool pulled = (mod.run.now % 700) < 200;
                mod.Frame(game, squeeze ? 1.0f : 0.0f, 1.0f, pulled ? kLmb : 0);
            }
            heldUp = mod.heldUp;
            said = mod.saidHeldUp;
            return game.phase == FightGame::Phase::kWon;
        };
        int heldUp = 0, said = 0, heldUp2 = 0, said2 = 0;
        const bool with = run(true, heldUp, said), without = run(false, heldUp2, said2);
        Report("25c", "the right trigger pulled now and then through a fight: the guard holds its left mouse button up in every prompt but Fire's: won; without the "
                      "guard: failed",
               with && !without && heldUp > 0 && said > 0 && said <= heldUp, "with: %s (held up %d, said %d); without: %s", with ? "won" : "NOT WON",
               heldUp, said, without ? "WON?" : "failed");
    }
    {
        // A frame the fights couldn't be read (a gate's vtable gone a frame) in Block's prompt: the guard left as it was (GuardFrame), the right trigger's left
        // mouse button still held up that frame (both reviews of 2026-11-18: it went through); frames on frames unread: it lapses by itself in 0.25 s.
        FightImage g;
        Watching w;
        qtefight::Guard guard;
        g.Prompt(2, true);
        qtefight::Frame f;
        for (int i = 0; i < 5; ++i) {
            f = w.Frame(g.m);
            qtefight::GuardFrame(guard, f.unknown, f.prompt, f.answer, w.now);
        }
        const bool before = qtefight::GuardKey(guard, kLmb, true, w.now).drop;
        const uint32_t e = g.f.entity[qtefight::kActions[0].right];
        g.m.W32(e, 0);
        f = w.Frame(g.m);
        qtefight::GuardFrame(guard, f.unknown, f.prompt, f.answer, w.now);
        const bool lost = f.unknown, during = qtefight::GuardKey(guard, kLmb, true, w.now).drop;
        g.m.W32(e, kFightTypes.gate);
        f = w.Frame(g.m);
        qtefight::GuardFrame(guard, f.unknown, f.prompt, f.answer, w.now);
        const bool after = qtefight::GuardKey(guard, kLmb, true, w.now).drop && qtefight::GuardKey(guard, kRmb, true, w.now).drop == false;
        // Unread on and on (the manager unreadable): held a moment, then lapsed.
        g.m.Drop(kMgr);
        bool heldOn = true, lapsed = false;
        const unsigned long long from = w.now;
        for (int i = 0; i < 40; ++i) {
            f = w.Frame(g.m);
            qtefight::GuardFrame(guard, f.unknown, f.prompt, f.answer, w.now);
            const bool drop = qtefight::GuardKey(guard, kSpace, true, w.now).drop;
            if (w.now < from + qtefight::kGuardMs - kFrame)
                heldOn = heldOn && drop;
            else if (w.now >= from + qtefight::kGuardMs + kFrame)
                lapsed = !drop;
        }
        Report("25d", "a frame the fights couldn't be read in Block's prompt: the guard as it was, the trigger's left mouse button held up that frame too; frames on "
                      "end unread: held up a moment, lapsed after 0.25 s",
               before && lost && during && after && heldOn && lapsed, "before %d, lost %d, during %d, after %d, held on %d, lapsed %d", before, lost, during, after,
               heldOn, lapsed);
    }
    {
        // The right stick's stance (X its crouch, Z its prone) in a prompt: left undone whatever the prompt awaits, CrouchDuck's Z included; said once a
        // prompt; outside prompts it goes, and a guard not set again lapses.
        qtefight::Guard g;
        const unsigned long long t = 9000;
        const bool before = !qtefight::GuardStance(g, t).drop;
        qtefight::SetGuard(g, true, kRmb, t);
        const qtefight::Pass block = qtefight::GuardStance(g, t + 1), again = qtefight::GuardStance(g, t + 2);
        const qtefight::Pass zKey = qtefight::GuardKey(g, kZ, true, t + 3);
        const bool keyStill = zKey.drop && zKey.say;  // (the stance's line didn't take the Z key's own)
        qtefight::SetGuard(g, true, kZ, t + 14);  // (CrouchDuck's prompt: Z its answer)
        const qtefight::Pass duck = qtefight::GuardStance(g, t + 15);
        const bool zGoes = !qtefight::GuardKey(g, kZ, true, t + 15).drop;
        qtefight::SetGuard(g, false, 0, t + 28);
        const bool after = !qtefight::GuardStance(g, t + 29).drop;
        qtefight::SetGuard(g, true, kE, t + 40);
        const bool lapsed = !qtefight::GuardStance(g, t + 40 + qtefight::kGuardMs).drop;
        Report("25e", "the right stick's stance in a prompt: left undone (Block's, CrouchDuck's too), said once a prompt, the keys' own lines apart; outside "
                      "prompts it goes; the guard not set again: lapsed",
               before && block.drop && block.say && again.drop && !again.say && keyStill && duck.drop && duck.say && zGoes && after && lapsed,
               "before %d, Block %d/%d, again %d/%d, Z key %d, CrouchDuck %d/%d (Z goes %d), after %d, lapsed %d", before, block.drop, block.say, again.drop,
               again.say, keyStill, duck.drop, duck.say, zGoes, after, lapsed);
    }
}

void FightTime() {
    FightImage g;
    g.Prompt(2, true);
    qtefight::Watch w;
    LARGE_INTEGER f, a, b;
    QueryPerformanceFrequency(&f);
    const qtelist::List list = ReadList(g.m);
    qtefight::Step(w, g.m, kFightTypes, list, false, 0, true);  // (attached)
    constexpr int kSteps = 50000;
    int answered = 0;
    g.m.reads = 0;
    QueryPerformanceCounter(&a);
    for (int i = 0; i < kSteps; ++i) {
        if (i % 2000 == 1000) {  // (the prompt changed now and then, Block and Fire by turns: none stands a minute)
            const bool block = (i / 2000) % 2 == 1;
            g.Prompt(2, block);
            g.Prompt(1, !block);
        }
        answered += qtefight::Step(w, g.m, kFightTypes, list, false, static_cast<unsigned long long>(i) * 14, true).answer != 0;
    }
    QueryPerformanceCounter(&b);
    const double us = 1e6 * static_cast<double>(b.QuadPart - a.QuadPart) / static_cast<double>(f.QuadPart) / kSteps;
    const int reads = g.m.reads / kSteps;
    Report("26a", "the time: a frame of an attached fight (one read of its bus and 15 of its objects: the 12 gates, the select, the count, the fail gate) in "
                  "microseconds, through the test's image",
           us < 50.0 && reads == 16 && answered == kSteps, "%.2f us a frame, %d reads a frame, %d of %d answered", us, reads, answered, kSteps);
}

// ---------------------------------------- 2026-11-19: a grip squeezed already answers (the owner's cellar fight) ----------------------------------------

// Operation Swordbreaker's cellar fight as the owner's log has it (bf3vr_1119_userplay_qte.log): InteractDrag's press prompt (E) 2.47 s, then 3.3 s on the
// quick-time Fire's (the left mouse button), 0.41 s to answer it. No grip press ever began there: a grip had to be seen let go under 0.4 and squeezed past
// 0.6 inside each prompt.
FightGame CellarFight() {
    FightGame game({{0, false, 2470}, {1, false, 410}});
    game.gapMs = 3300;
    return game;
}

// A fight run frame by frame: grips(game, now) the two grips' squeezes; each key's first press the game took, after its prompt opened (ms; -1 never).
struct FightRun {
    long long took[6] = {-1, -1, -1, -1, -1, -1};
    unsigned long long openedAt[8] = {};
    int opened = 0;
};
template <class Grips>
FightRun RunFight(FightGame& game, FightMod& mod, Grips grips, int frames = 3000) {
    FightRun r;
    int lastBeat = -1;
    for (int i = 0; i < frames && !game.Over(); ++i) {
        int before[6];
        for (int k = 0; k < 6; ++k)
            before[k] = game.presses[k];
        float left = 0.0f, right = 0.0f;
        grips(game, mod.run.now, left, right);
        mod.Frame(game, left, right);
        if ((game.phase == FightGame::Phase::kOpen || game.phase == FightGame::Phase::kClosing) && game.beat != lastBeat && r.opened < 8) {
            lastBeat = game.beat;
            r.openedAt[r.opened++] = game.phaseAt;
        }
        for (int k = 0; k < 6; ++k)
            if (game.presses[k] > before[k] && r.took[k] < 0 && r.opened)
                r.took[k] = static_cast<long long>(mod.run.now - kFrame) - static_cast<long long>(r.openedAt[r.opened - 1]);
    }
    return r;
}

void HeldGrips() {
    {
        // 27a The cellar with the right grip held from before the fight's start, through it all (a fist): E pressed as its prompt opens, Fire as its 0.41 s prompt
        // opens, each once; won; nothing between the prompts.
        for (int hand = 0; hand < 2; ++hand) {
            FightGame game = CellarFight();
            FightMod mod;
            const FightRun r = RunFight(game, mod, [hand](FightGame&, unsigned long long, float& lg, float& rg) { (hand ? rg : lg) = 1.0f; });
            const bool ok = game.phase == FightGame::Phase::kWon && game.presses[0] == 1 && game.presses[1] == 1 && game.presses[2] + game.presses[3] + game.presses[4] +
                                                                                                                          game.presses[5] == 0 &&
                            game.wrongPresses == 0 && game.betweenPresses == 0 && mod.keysOutsidePrompts == 0 && mod.run.byHold == 2 && mod.run.began == 2 &&
                            mod.run.refused == 0 && r.took[0] >= 0 && r.took[0] <= 50 && r.took[1] >= 0 && r.took[1] <= 50 && mod.run.lastHand == hand;
            Report(hand ? "27a" : "27aL", hand ? "the owner's cellar fight (E 2.47 s, then the quick-time Fire's 0.41 s), the right grip held from before the fight: E "
                                                 "pressed as its prompt opens, Fire as its opens, each once by the grip squeezed already; won; nothing between"
                                               : "... the left grip held instead: the same",
                   ok, "%s%s; E taken %lld ms after it opened, Fire %lld ms; presses E %d Fire %d, others %d, wrong %d, between %d; by the grip squeezed already %d of %d",
                   game.phase == FightGame::Phase::kWon ? "won" : "NOT WON: ", game.failed, r.took[0], r.took[1], game.presses[0], game.presses[1],
                   game.presses[2] + game.presses[3] + game.presses[4] + game.presses[5], game.wrongPresses, game.betweenPresses, mod.run.byHold, mod.run.began);
        }
    }
    {
        // 27b A grip squeezed in the gap half a second before each prompt opens and held into it (let go 0.3 s after each closes): pressed at once, each once.
        FightGame game = CellarFight();
        FightMod mod;
        const FightRun r = RunFight(game, mod, [](FightGame& g, unsigned long long now, float&, float& right) {
            const bool soon = g.phase == FightGame::Phase::kGap && g.beat < static_cast<int>(g.beats.size()) && now + 500 >= g.phaseAt + g.gapMs;
            const bool after = g.phase == FightGame::Phase::kGap && g.beat > 0 && now < g.phaseAt + 300;
            right = soon || after || g.phase == FightGame::Phase::kOpen || g.phase == FightGame::Phase::kClosing ? 1.0f : 0.0f;
        });
        Report("27b", "a grip squeezed 0.5 s before each prompt opens and held into it: each pressed as it opens, once; won", game.phase == FightGame::Phase::kWon &&
                   game.presses[0] == 1 && game.presses[1] == 1 && game.wrongPresses == 0 && game.betweenPresses == 0 && mod.run.byHold == 2 && r.took[0] <= 50 &&
                   r.took[1] <= 50 && r.took[0] >= 0 && r.took[1] >= 0,
               "%s%s; E %lld ms, Fire %lld ms; by the grip squeezed already %d", game.phase == FightGame::Phase::kWon ? "won" : "NOT WON: ", game.failed, r.took[0],
               r.took[1], mod.run.byHold);
    }
    {
        // 27c A re-squeeze in a press prompt already pressed (its animation slow: the prompt up 1.5 s after the press): let go and squeezed again twice:
        // nothing (one press a prompt, refused); the next prompt: let go as it opens and freshly squeezed 0.2 s in: pressed, as before 2026-11-19.
        FightGame game = CellarFight();
        game.closeMs = 1500;
        FightMod mod;
        const FightRun r = RunFight(game, mod, [](FightGame& g, unsigned long long now, float&, float& right) {
            right = 0.0f;
            if (g.beat == 0 && g.phase != FightGame::Phase::kGap)
                right = (now >= g.phaseAt + 200 && now < g.phaseAt + 300) || (now >= g.phaseAt + 600 && now < g.phaseAt + 700) ? 0.0f : 1.0f;
            else if (g.beat == 0)
                right = 1.0f;  // (held from before E's prompt)
            else if (g.beat == 1 && g.phase == FightGame::Phase::kOpen)
                right = now >= g.phaseAt + 200 ? 1.0f : 0.0f;
        });
        Report("27c", "a press prompt pressed (by the grip held) and squeezed again twice while it's up: nothing more (refused, one press a prompt); the next "
                      "prompt freshly squeezed 0.2 s in: pressed; won",
               game.phase == FightGame::Phase::kWon && game.presses[0] == 1 && game.presses[1] == 1 && game.wrongPresses == 0 && mod.run.refused >= 2 &&
                   mod.run.byHold == 1 && mod.run.began == 2 && r.took[1] >= 200 && r.took[1] <= 260,
               "%s%s; E presses %d (squeezes refused %d), Fire %d taken %lld ms in; presses begun %d, by the grip squeezed already %d",
               game.phase == FightGame::Phase::kWon ? "won" : "NOT WON: ", game.failed, game.presses[0], mod.run.refused, game.presses[1], r.took[1], mod.run.began,
               mod.run.byHold);
    }
    {
        // 27d Two prompts in a row, Block's gates shut and Fire's open in one write (no frame between), the grip held throughout: each pressed once, Fire in
        // the very frame its gates are first read open; a frame or 0.1 s between: the same.
        const auto run = [](unsigned long long gapMs, int& block, int& fire, long long& fireAfter) {
            FightImage g;
            Watching w;
            Run2 r;
            r.game = 0;
            block = fire = 0;
            fireAfter = -1;
            unsigned was = 0;
            const unsigned long long open = w.now + 200, swap = open + 1500, end = swap + gapMs + 1500;
            for (int i = 0; i < 72 * 5; ++i) {
                const unsigned long long t = w.now;
                g.Prompt(2, t >= open && t < swap);
                g.Prompt(1, t >= swap + gapMs && t < end);
                const qtefight::Frame f = w.Frame(g.m);
                r.await = f.await, r.hold = f.hold, r.mash = !f.prompt || f.mash;
                r.prompt = f.prompt, r.once = f.prompt && !f.mash, r.unknown = f.unknown, r.scripted = true;
                r.Frame(1.0f, 0.0f);
                block += (r.keys & kRmb) && !(was & kRmb);
                if ((r.keys & kLmb) && !(was & kLmb)) {
                    ++fire;
                    if (fireAfter < 0)
                        fireAfter = static_cast<long long>(t) - static_cast<long long>(swap + gapMs);
                }
                was = r.keys;
            }
        };
        int b0 = 0, f0 = 0, b14 = 0, f14 = 0, b100 = 0, f100 = 0;
        long long a0 = 0, a14 = 0, a100 = 0;
        run(0, b0, f0, a0);
        run(14, b14, f14, a14);
        run(100, b100, f100, a100);
        Report("27d", "Block's prompt then Fire's with no frame between, the grip held throughout: each pressed once, Fire in the frame its gates are first read; a "
                      "frame or 0.1 s between: the same",
               b0 == 1 && f0 == 1 && b14 == 1 && f14 == 1 && b100 == 1 && f100 == 1 && a0 >= 0 && a0 < static_cast<long long>(kFrame) && a14 >= 0 &&
                   a14 < static_cast<long long>(kFrame) && a100 >= 0 && a100 < static_cast<long long>(kFrame),
               "no gap: Block %d Fire %d (%lld ms on); 14 ms: %d %d (%lld); 100 ms: %d %d (%lld)", b0, f0, a0, b14, f14, a14, b100, f100, a100);
    }
    {
        // 27e A mash prompt (Block, its count from 10) between two presses, the grip held from before the fight throughout: the right mouse button from the
        // frame it opens, every 130 ms, the count done in about 1.3 s; won. Let go 0.2 s in the mash and squeezed again 0.1 s later: the taps go on (a mash is
        // no press prompt: nothing refused).
        for (int again = 0; again < 2; ++again) {
            FightGame game({{1, false, 2000}, {2, true, 3000}, {4, false, 2000}});
            FightMod mod;
            const FightRun r = RunFight(game, mod, [again](FightGame& g, unsigned long long now, float& left, float&) {
                left = again && g.beat == 1 && g.phase == FightGame::Phase::kOpen && now >= g.phaseAt + 200 && now < g.phaseAt + 300 ? 0.0f : 1.0f;
            });
            bool mashDone = game.phase == FightGame::Phase::kWon;
            Report(again ? "27eR" : "27e", again ? "... let go 0.2 s into the mash and squeezed again 0.1 s later: its taps go on, nothing refused; won"
                                                 : "a mash prompt between two presses, the grip held throughout: its key from the frame it opens, every 130 ms, the count "
                                                   "done; the presses once each; won",
                   mashDone && game.wrongPresses == 0 && game.presses[1] == 1 && game.presses[4] == 1 && game.presses[2] >= 10 && game.presses[2] <= 14 &&
                       r.took[2] >= 0 && r.took[2] <= 50 && mod.run.refused == 0 && mod.run.byHold >= 3,
                   "%s%s; Fire %d, Block %d (the first %lld ms after it opened), Space %d; refused %d, by the grip squeezed already %d",
                   game.phase == FightGame::Phase::kWon ? "won" : "NOT WON: ", game.failed, game.presses[1], game.presses[2], r.took[2], game.presses[4],
                   mod.run.refused, mod.run.byHold);
        }
    }
    {
        // 27f A press prompt's one press kept: a frame the fights couldn't be read just after it, the gates unclear a frame (a prompt up with no one key), the
        // headset off 0.1 s, all with the grip held: never a second press; a frame read with no prompt and then E's prompt again (a new one): pressed again;
        // the headset off a whole second with the prompt still up: forgotten, one more press (by the prefab, harmless: its right gate is the one open).
        Run2 r;
        r.scripted = r.prompt = r.once = true;
        r.mash = false;
        r.await = kE;
        r.Frames(10, 0.0f, 1.0f);
        const int first = r.edges[Bit(kE)];
        r.await = 0, r.prompt = false, r.unknown = true;
        r.Frame(0.0f, 1.0f);  // (a frame the fights couldn't be read)
        r.await = kE, r.prompt = true, r.unknown = false;
        r.Frames(20, 0.0f, 1.0f);
        const int afterUnknown = r.edges[Bit(kE)];
        r.await = 0;  // (the gates unclear: a prompt up, no one key)
        r.Frame(0.0f, 1.0f);
        r.await = kE;
        r.Frames(20, 0.0f, 1.0f);
        const int afterUnclear = r.edges[Bit(kE)];
        r.Stop(qte::End::kFront);
        r.now += 100;
        r.Frames(20, 0.0f, 1.0f);
        const int afterOff = r.edges[Bit(kE)];
        r.await = 0, r.prompt = false;  // (read with no prompt: the gates shut)
        r.Frame(0.0f, 1.0f);
        r.await = kE, r.prompt = true;  // (E's prompt again: a new one)
        r.Frames(20, 0.0f, 1.0f);
        const int newPrompt = r.edges[Bit(kE)];
        r.Stop(qte::End::kFront);
        r.now += 1000;
        r.Frames(20, 0.0f, 1.0f);
        const int longOff = r.edges[Bit(kE)];
        Report("27f", "a press prompt's one press kept over a frame not read, a frame with its gates unclear and the headset off 0.1 s, the grip held: no second "
                      "press; a new prompt of the same key: pressed; the headset off a second: one more (forgotten after 0.25 s)",
               first == 1 && afterUnknown == 1 && afterUnclear == 1 && afterOff == 1 && newPrompt == 2 && longOff == 3 && r.refused >= 3 && r.ever == kE,
               "first %d, after the unread frame %d, after the unclear one %d, after 0.1 s off %d, a new prompt %d, after 1 s off %d; refused %d", first, afterUnknown,
               afterUnclear, afterOff, newPrompt, longOff, r.refused);
    }
    {
        // 27g The train's three with "qtetest 600 held" in a scripted moment as in the game: the grapple one left click 0.1 s after its trigger appeared, the
        // hanging one Space, the dangling climb 30 Spaces in about 4 s, never another key; the right grip held (the gun's, as the grapple's cutscene begins):
        // the same; the held grip in play (no scripted moment): nothing pressed at all, as before.
        const TrainOutcome held = TrainRuns(true, 1), gun = TrainRuns(true, 2), play = TrainRuns(false, 1);
        Report("27g", "the train's three with qtetest's grip held throughout (a scripted moment): the grapple one left click 0.1 s after its trigger, the hanging "
                      "one Space, the dangling climb 30 Spaces in about 4 s; never another key",
               held.grapple && held.hanging && held.dangling && held.grappleAfter >= qte::kHeldSettle && held.grappleAfter <= qte::kHeldSettle + 60 &&
                   held.hangingAfter >= qte::kHeldSettle && held.hangingAfter <= qte::kHeldSettle + 60 && held.byHold == 3,
               "grapple %d after %llu ms (clicks %d, keys %02X), hanging %d after %llu ms (Spaces %d), dangling %d (%d presses in %llu ms); by the grip held %d",
               held.grapple, held.grappleAfter, held.clicks, held.grappleKeys, held.hanging, held.hangingAfter, held.spaces, held.dangling, held.climbPresses, held.took,
               held.byHold);
        Report("27gR", "... the right grip held instead (the gun's as the cutscene began), the left let go: the same", gun.grapple && gun.hanging && gun.dangling &&
                                                                                                                           gun.byHold == 3,
               "grapple %d after %llu ms, hanging %d, dangling %d (%d presses)", gun.grapple, gun.grappleAfter, gun.hanging, gun.dangling, gun.climbPresses);
        Report("27gP", "... the grip held in play (no scripted moment): nothing pressed, not one key (a fresh squeeze wanted, as before)",
               !play.grapple && !play.hanging && !play.dangling && play.clicks == 0 && play.spaces == 0 && play.climbPresses == 0 && play.grappleKeys == 0 &&
                   play.hangingKeys == 0 && play.danglingKeys == 0,
               "clicks %d, Spaces %d and %d, keys %02X %02X %02X", play.clicks, play.spaces, play.climbPresses, play.grappleKeys, play.hangingKeys, play.danglingKeys);
    }
    {
        // 27h Nothing pressed with no prompt: 10 000 frames of both grips at play in a scripted moment with nothing awaited; a single quick-time event in play
        // with both grips held from before (the gun, the handguard): nothing until a fresh squeeze, as before; a fight between its prompts with both held:
        // nothing (27a's between).
        Run2 n;
        n.scripted = true;
        for (int i = 0; i < 10000; ++i)
            n.Frame(0.5f + 0.5f * std::sin(i * 0.05f), i % 700 < 650 ? 1.0f : 0.0f);
        Run2 p;
        p.Frames(20, 1.0f, 1.0f);
        p.await = kLmb;
        p.Frames(300, 1.0f, 1.0f);
        const qte::Idle heldIdle = p.last.idle;
        const bool none = p.ever == 0 && p.began == 0 && heldIdle == qte::Idle::kNotFresh;
        p.Frames(3, 0.0f, 1.0f);
        p.Frames(5, 1.0f, 1.0f);
        Report("27h", "nothing awaited in a scripted moment: nothing pressed in 10 000 frames of both grips; a single in play with both grips held from before: "
                      "nothing (why: held, not fresh, not a scripted moment) until a fresh squeeze",
               n.ever == 0 && n.began == 0 && n.heldFrames == 0 && none && p.began == 1 && p.byHold == 0 && p.ever == kLmb,
               "scripted, nothing awaited: keys %02X, presses %d; in play: held %s (idle %d), then a fresh squeeze: presses %d", n.ever, n.began,
               none ? "nothing" : "PRESSED", static_cast<int>(heldIdle), p.began);
    }
    {
        // 27i Either grip, any squeeze past 0.6: a fight's press prompt held at 0.61 by the left, by the right, by both (one press); held at 0.59 (half
        // squeezed): nothing; a single in a scripted moment held at 0.61: pressed 0.1 s after its key came.
        const auto prompt = [](float left, float right, int& presses, int& hand) {
            Run2 r;
            r.scripted = r.prompt = r.once = true;
            r.mash = false;
            r.await = kF;
            r.Frames(30, left, right);
            presses = r.edges[Bit(kF)];
            hand = r.lastHand;
        };
        int pl = 0, hl = -1, pr = 0, hr = -1, pb = 0, hb = -1, ph = 0, hh = -1;
        prompt(0.61f, 0.0f, pl, hl);
        prompt(0.0f, 0.61f, pr, hr);
        prompt(1.0f, 0.7f, pb, hb);
        prompt(0.59f, 0.59f, ph, hh);
        Run2 s;
        s.scripted = true;
        s.await = s.quick = kSpace;
        int firstAt = -1;
        for (int i = 0; i < 30; ++i) {
            const unsigned long long t = s.now;
            s.Frame(0.0f, 0.61f);
            if (firstAt < 0 && (s.keys & kSpace))
                firstAt = static_cast<int>(t - 100000);
        }
        Report("27i", "either grip, any squeeze past 0.6: a press prompt held at 0.61 by the left, the right, both (one press); 0.59 (half squeezed): nothing; a "
                      "single in a scripted moment at 0.61: pressed 0.1 s after its key came",
               pl == 1 && hl == 0 && pr == 1 && hr == 1 && pb == 1 && ph == 0 && firstAt >= static_cast<int>(qte::kHeldSettle) &&
                   firstAt < static_cast<int>(qte::kHeldSettle + kFrame),
               "left %d (hand %d), right %d (hand %d), both %d, half %d; the single at %d ms", pl, hl, pr, hr, pb, ph, firstAt);
    }
    {
        // 27j Why a single's held grip waits kHeldSettle: the grapple's trigger is an add the game applies 30 ms on (the test's game, its input read every 33
        // ms), and a key already down as it takes the trigger is no press of it. With "qtemash 0" (one tap a squeeze), over the three ways the game's input
        // ticks fall on the frames: freshly squeezed on the trigger's second frame (its add not yet applied): its one tap lost in some, the event failed;
        // a grip held from before: its tap 0.1 s on taken in every one, won.
        const auto grapple = [](bool held, int phase, unsigned long long& wonAfter, int& clicks) {
            Game game;
            const uint32_t fire = Entity(game.m, 0x3D);
            qtelist::Steady steady;
            Run2 run;
            run.mash = false;
            const unsigned long long start = run.now;
            unsigned long long appeared = 0;
            bool added = false, won = false;
            wonAfter = 0;
            for (int i = 0; i < 400; ++i) {
                const unsigned long long t = run.now;
                game.Apply(t);
                if (!added && t >= start + 1000 + static_cast<unsigned long long>(phase) * kFrame) {
                    added = true;
                    game.Change(fire, false, t);
                    appeared = t;
                }
                const qtelist::List now = ReadList(game.m);
                const qtelist::List& list = qtelist::Settle(steady, now, run.now);
                const qtelist::Keys keys = qtelist::Awaited(list, true);
                run.await = keys.await, run.hold = keys.hold;
                run.quick = qtelist::QuickKeys(list) & keys.await;
                run.scripted = true;
                run.game = 3;
                run.external = game.tickKeys;
                const float grip = held ? 1.0f : (appeared && t > appeared ? 1.0f : 0.0f);  // (let go on its first frame, squeezed from its second)
                const unsigned went = game.Tick(t, run.Frame(grip, 0.0f).keys);
                if (game.Live(fire) && game.pending.empty() && (went & kLmb) && !won) {
                    won = true;
                    wonAfter = t - appeared;
                    game.Change(fire, true, t);
                }
            }
            clicks = run.edges[Bit(kLmb)];
            return won;
        };
        int earlyWon = 0, heldWon = 0, earlyClicks = 0, heldClicks = 0;
        unsigned long long heldLatest = 0, heldSoonest = ~0ull;
        for (int phase = 0; phase < 3; ++phase) {
            unsigned long long after = 0;
            int clicks = 0;
            earlyWon += grapple(false, phase, after, clicks);
            earlyClicks += clicks;
            heldWon += grapple(true, phase, after, clicks);
            heldClicks += clicks;
            heldLatest = std::max(heldLatest, after);
            heldSoonest = std::min(heldSoonest, after);
        }
        Report("27j", "why a single's held grip waits 0.1 s: \"qtemash 0\", over the 3 ways the game's input ticks fall: freshly squeezed on the trigger's second "
                      "frame (its add not yet applied): its one tap lost in some, failed; held from before: its tap 0.1 s on taken in all 3, won",
               earlyWon < 3 && earlyClicks == 3 && heldWon == 3 && heldClicks == 3 && heldSoonest >= qte::kHeldSettle && heldLatest <= qte::kHeldSettle + 60,
               "squeezed on its second frame: won %d of 3 (%d taps); held: won %d of 3 (%d taps), %llu-%llu ms after it appeared", earlyWon, earlyClicks, heldWon,
               heldClicks, heldSoonest, heldLatest);
    }
    {
        // 27k The grips' log while keys are awaited (input.cpp's "QTE grips:" lines, qte::WatchStep): the cellar with the right grip held: a line as each prompt
        // opens (the grip squeezed, taken as it was held, its key down) and one as it ends (one press, by the right grip taken as it was held, 0.00 s in);
        // nothing between prompts. A single in play held from before: why nothing is down. Neither grip squeezed in a 0.41 s prompt: the end says so. A long
        // stretch: twice a second, four lines at most, and its end.
        char text[640];
        FightGame game = CellarFight();
        FightMod mod;
        int lines = 0, ends = 0;
        bool openLine = false, endLine = false;
        for (int i = 0; i < 3000 && !game.Over(); ++i)
            mod.Frame(game, 0.0f, 1.0f);
        lines = mod.watchLines, ends = mod.watchEnds;
        // (The texts on frames made here: the Fire prompt's first frame with the right grip held, as Step2 left it.)
        Run2 r;
        r.scripted = r.prompt = r.once = true;
        r.mash = false;
        r.await = kLmb;
        qte::Watch2 w;
        r.Frame(0.0f, 1.0f);
        qte::WatchOut wo = qte::WatchStep(w, r.lastIn, r.last);
        qte::WatchText(w, r.lastIn, r.s, r.last, text, sizeof(text));
        openLine = wo.say && std::strstr(text, "the left mouse button awaited 0.00 s (a fight's press prompt, a scripted moment)") &&
                   std::strstr(text, "the right 1.00 squeezed (taken as it was held)") && std::strstr(text, ": the left mouse button down") &&
                   std::strstr(text, "the right grip's press, taken as it was held, tap 1");
        std::string open = text;
        for (int i = 0; i < 29; ++i) {
            r.Frame(0.0f, 1.0f);
            qte::WatchStep(w, r.lastIn, r.last);
        }
        r.await = 0, r.prompt = false;
        r.Frame(0.0f, 1.0f);
        wo = qte::WatchStep(w, r.lastIn, r.last);
        qte::WatchEndText(wo.ended, text, sizeof(text));
        endLine = wo.sayEnd && std::strstr(text, "over: 1 press, the first by the right grip taken as it was held, 0.00 s in") && std::strstr(text, "the right 1.00-1.00");
        std::string end = text;
        // In play, a single, both grips held from before.
        Run2 p;
        p.Frames(5, 1.0f, 1.0f);
        p.await = kE;
        qte::Watch2 pw;
        p.Frame(1.0f, 1.0f);
        qte::WatchStep(pw, p.lastIn, p.last);
        qte::WatchText(pw, p.lastIn, p.s, p.last, text, sizeof(text));
        const bool why = std::strstr(text, "nothing down: a grip held from before presses only a fight's prompt or in a scripted moment") &&
                         std::strstr(text, "squeezed, held from before (not seen let go since the keys came)") && std::strstr(text, "(a single quick-time event, in play)");
        std::string whyText = text;
        // Neither grip squeezed past 0.6 in a 0.41 s press prompt.
        Run2 o;
        o.scripted = o.prompt = o.once = true;
        o.mash = false;
        o.await = kLmb;
        qte::Watch2 ow;
        int oLines = 0;
        for (int i = 0; i < 30; ++i) {
            o.Frame(0.3f, 0.55f);
            oLines += qte::WatchStep(ow, o.lastIn, o.last).say;
        }
        o.await = 0, o.prompt = false;
        o.Frame(0.0f, 0.0f);
        wo = qte::WatchStep(ow, o.lastIn, o.last);
        qte::WatchEndText(wo.ended, text, sizeof(text));
        const bool nothing = wo.sayEnd && oLines == 1 && std::strstr(text, "over: nothing pressed by the grips (nothing down at times: neither grip squeezed past 0.6)") &&
                             std::strstr(text, "the left 0.30-0.30; the right 0.55-0.55");
        std::string nothingText = text;
        // A long stretch (the dangling climb's 4.4 s): a line every 0.5 s, four at most, and its end.
        Run2 c;
        c.scripted = true;
        c.await = c.quick = kSpace;
        qte::Watch2 cw;
        int cLines = 0, cEnds = 0;
        unsigned long long lastLine = 0, shortest = ~0ull;
        for (int i = 0; i < 316; ++i) {
            c.Frame(0.0f, 1.0f);
            const qte::WatchOut x = qte::WatchStep(cw, c.lastIn, c.last);
            if (x.say) {
                if (lastLine && c.lastIn.nowMs - lastLine < shortest)
                    shortest = c.lastIn.nowMs - lastLine;
                lastLine = c.lastIn.nowMs;
                ++cLines;
            }
        }
        c.await = 0;
        c.Frame(0.0f, 1.0f);
        cEnds += qte::WatchStep(cw, c.lastIn, c.last).sayEnd;
        Report("27k", "the grips' log: the cellar held: a line as each prompt opens and one as it ends, nothing between; the texts (the grip taken as it was held, "
                      "its key down; one press by it 0.00 s in; why nothing is down in play; nothing pressed, neither past 0.6); a 4.4 s stretch: every 0.5 s, 4 at most",
               lines == 2 && ends == 2 && openLine && endLine && why && nothing && cLines == qte::kWatchLines && cEnds == 1 && shortest >= qte::kWatchEvery,
               "cellar: %d lines, %d ends; open \"%s\"; end \"%s\"; in play \"%s\"; nothing \"%s\"; long: %d lines (%llu ms apart at the least), %d end", lines, ends,
               open.c_str(), end.c_str(), whyText.c_str(), nothingText.c_str(), cLines, shortest == ~0ull ? 0ull : shortest, cEnds);
    }
    {
        // 27l "qtetest 600 held" (qte::TestHeld): squeezed from the test's 0.3 s until 0.2 s before its end, whatever is awaited and between prompts.
        const unsigned long long from = 1000, until = 61000;
        const bool before = !qte::TestHeld(999, from, until), atFrom = qte::TestHeld(1000, from, until), mid = qte::TestHeld(30000, from, until),
                   nearEnd = !qte::TestHeld(until - qte::kTestEnd, from, until), last = qte::TestHeld(until - qte::kTestEnd - 1, from, until);
        Report("27l", "qtetest's held grip: squeezed from 0.3 s into the test until 0.2 s before its end, whatever is awaited", before && atFrom && mid && nearEnd && last,
               "before %d, at 0.3 s %d, mid %d, 0.2 s before the end %d, a ms earlier %d", before, atFrom, mid, nearEnd, last);
    }
    {
        // 27m Play's own actions in a scripted moment (answered there by version 2's design of 2026-11-18): a grip held from before (a ride's gun grip) never
        // presses play's own Fire alone, 2 s of it (why said); a fresh squeeze does, as before; play's Jump live beside the hanging's JumpClimb (the train):
        // Space is a quick-time action's too, the held grip presses it 0.1 s on; qtelist::QuickKeys leaves a fight's six and play's own out.
        const auto run = [](const std::vector<uint32_t>& actions, float left, float right, int frames, Run2& r, unsigned& quick) {
            Game game;
            for (uint32_t a : actions)
                game.enabled.push_back(Entity(game.m, a));
            Manager(game.m, game.enabled);
            r.scripted = true;
            for (int i = 0; i < frames; ++i) {
                const qtelist::List list = ReadList(game.m);
                const qtelist::Keys keys = qtelist::Awaited(list, true);
                r.await = keys.await, r.hold = keys.hold;
                r.quick = quick = qtelist::QuickKeys(list) & keys.await;
                r.Frame(left, right);
            }
        };
        Run2 fire;
        unsigned fireQuick = 0;
        run({0x07}, 0.0f, 1.0f, 143, fire, fireQuick);
        const bool held = fire.ever == 0 && fire.began == 0 && fire.last.idle == qte::Idle::kPlayKey;
        Run2 fresh;
        unsigned freshQuick = 0;
        run({0x07}, 0.0f, 0.0f, 5, fresh, freshQuick);
        Game g2;  // (then a fresh squeeze of the left)
        g2.enabled.push_back(Entity(g2.m, 0x07));
        Manager(g2.m, g2.enabled);
        for (int i = 0; i < 10; ++i) {
            const qtelist::List list = ReadList(g2.m);
            const qtelist::Keys keys = qtelist::Awaited(list, true);
            fresh.await = keys.await, fresh.quick = qtelist::QuickKeys(list) & keys.await;
            fresh.Frame(1.0f, 0.0f);
        }
        Run2 hang;
        unsigned hangQuick = 0;
        run({0x40, 0x0C}, 1.0f, 0.0f, 30, hang, hangQuick);
        // A fight's six on one bus, and play's own: none of them a single's quick-time key.
        Image m;
        std::vector<uint32_t> six;
        for (uint32_t a = 0x3C; a <= 0x41; ++a)
            six.push_back(Entity(m, a));
        six.push_back(Entity(m, 0x23));
        Manager(m, six);
        const unsigned sixQuick = qtelist::QuickKeys(ReadList(m));
        Report("27m", "play's own Fire alone in a scripted moment: a grip held from before presses nothing in 2 s (why said), a fresh squeeze does; the hanging's "
                      "JumpClimb with play's Jump: the held grip's Space 0.1 s on; a fight's six and play's Interact: no single's quick-time key",
               held && fireQuick == 0 && fresh.began == 1 && fresh.byHold == 0 && fresh.ever == kLmb && hangQuick == kSpace && hang.began == 1 && hang.byHold == 1 &&
                   hang.ever == kSpace && sixQuick == 0,
               "held on play's Fire: keys %02X (idle %d); fresh: presses %d, keys %02X; the hanging: quick %02X, presses %d by the grip held %d; the six and Interact: %02X",
               fire.ever, static_cast<int>(fire.last.idle), fresh.began, fresh.ever, hangQuick, hang.began, hang.byHold, sixQuick);
    }
    {
        // 27n A fight and a single live together (The Great Destroyer's train: a climb's JumpClimb on the level's bus beside the fight), the grip held
        // throughout: the fight's prompt pressed as it opens, once; between its prompts the climb's Space never by the held grip (its taps on and on could meet
        // the next prompt's wrong gate as it opens); a fresh squeeze between prompts presses it, as before; never a wrong key.
        const auto run = [](bool freshBetween, unsigned& duringPrompt, unsigned& between, int& wrong, bool& won, unsigned& quickBetween) {
            FightGame game({{2, false, 3000}, {1, false, 3000}});
            g_triggerBus = LevelBus(game.g.m, 0xE7380000u, "Levels/SP_Finale/TrainRide_SCM", 412);
            const uint32_t climb = Entity(game.g.m, 0x40);
            g_triggerBus = 0;
            FightMod mod;
            duringPrompt = between = quickBetween = 0;
            bool added = false;
            for (int i = 0; i < 900 && !game.Over(); ++i) {
                if (!added && game.phase != FightGame::Phase::kStart) {
                    added = true;
                    std::vector<uint32_t> enabled = {game.g.yaw, game.g.pitch, climb};
                    for (uint32_t t : game.g.f.trigger)
                        enabled.push_back(t);
                    Manager(game.g.m, enabled);
                }
                const bool inPrompt = game.phase == FightGame::Phase::kOpen || game.phase == FightGame::Phase::kClosing;
                const bool gap = game.phase == FightGame::Phase::kGap && added;
                const float left = !freshBetween || !gap ? 1.0f : (i % 60) >= 30 && (i % 60) < 40 ? 1.0f : 0.0f;
                const unsigned sent = mod.Frame(game, left, 0.0f);
                (inPrompt ? duringPrompt : between) |= sent;
                if (gap)
                    quickBetween |= mod.run.quick;
            }
            wrong = game.wrongPresses;
            won = game.phase == FightGame::Phase::kWon;
        };
        unsigned during = 0, between = 0, quick = 0, duringF = 0, betweenF = 0, quickF = 0;
        int wrong = 0, wrongF = 0;
        bool won = false, wonF = false;
        run(false, during, between, wrong, won, quick);
        run(true, duringF, betweenF, wrongF, wonF, quickF);
        Report("27n", "a fight and a single live together (the train's), the grip held throughout: each fight prompt's key at once; between prompts the climb's "
                      "Space never by the held grip (no single's key for it beside a fight); a fresh squeeze between prompts: Space, as before; never a wrong key",
               won && wrong == 0 && during == (kRmb | kLmb) && between == 0 && quick == 0 && wonF && wrongF == 0 && (betweenF & kSpace) &&
                   !(betweenF & ~kSpace),
               "held: %s, during the prompts %02X, between %02X (its quick keys %02X), wrong %d; fresh squeezes between: %s, between %02X, wrong %d",
               won ? "won" : "NOT WON", during, between, quick, wrong, wonF ? "won" : "NOT WON", betweenF, wrongF);
    }
}

// 28: the review of 27's held grips (2026-11-19).
void ReviewFixes() {
    {
        // 28a A press begun by a grip squeezed already ends with its scripted moment: the dangling climb's Space (a single in a scripted moment) held by the
        // gun's grip, then the moment over with its trigger still live: not one tap in 3 s of play (before the review: 24, one every 130 ms); held, not
        // fresh, said why; the moment back: the climb's taps again. The quick-time Fire held by the gun's grip: no left click in play. A fresh squeeze's
        // press goes on into play while squeezed, as ever (version 2 of 2026-11-18).
        Run2 r;
        r.scripted = true;
        r.await = r.quick = kSpace;
        r.Frames(72, 0.0f, 1.0f);
        const int inMoment = r.edges[Bit(kSpace)];
        r.scripted = false;
        int downInPlay = 0;
        for (int i = 0; i < 216; ++i) {
            r.Frame(0.0f, 1.0f);
            downInPlay += (r.keys & kSpace) != 0;
        }
        const int inPlay = r.edges[Bit(kSpace)] - inMoment;
        const qte::End why = r.lastWhy;
        const qte::Idle idle = r.last.idle;
        r.scripted = true;
        r.Frames(72, 0.0f, 1.0f);
        const int back = r.edges[Bit(kSpace)] - inMoment - inPlay;
        Run2 f;
        f.scripted = true;
        f.await = f.quick = kLmb;
        f.Frames(36, 0.0f, 1.0f);
        const int fireMoment = f.edges[Bit(kLmb)];
        f.scripted = false;
        f.Frames(216, 0.0f, 1.0f);
        const int firePlay = f.edges[Bit(kLmb)] - fireMoment;
        Run2 n;
        n.scripted = true;
        n.await = n.quick = kSpace;
        n.Frames(3, 0.0f, 0.0f);
        n.Frames(36, 0.0f, 1.0f);
        const int nMoment = n.edges[Bit(kSpace)];
        n.scripted = false;
        n.Frames(72, 0.0f, 1.0f);
        const int nPlay = n.edges[Bit(kSpace)] - nMoment;
        Report("28a", "a held grip's press of a single ends with its scripted moment (the trigger still live): no tap in 3 s of play, why said; the moment back: "
                      "its taps again; the quick-time Fire on the gun's grip: no click in play; a fresh squeeze's press on into play as ever",
               inMoment >= 6 && inPlay == 0 && downInPlay == 0 && why == qte::End::kMoment && idle == qte::Idle::kNotFresh && back >= 6 && fireMoment >= 3 &&
                   firePlay == 0 && n.byHold == 0 && nMoment >= 3 && nPlay >= 6,
               "held: %d taps in the moment, %d in play (%d frames down), ended \"%s\", idle %d, %d taps with the moment back; Fire: %d clicks, %d in play; fresh: "
               "%d taps, %d on in play",
               inMoment, inPlay, downInPlay, qte::Word(why), static_cast<int>(idle), back, fireMoment, firePlay, nMoment, nPlay);
    }
    {
        // 28b The scripted flag gone one frame 1 s into a held climb: its taps go on (the press ended that frame, taken again as the moment comes back),
        // never more than 0.22 s between two; a fist fight's triggers live beside it (a single's quick keys none then): the held press ends ("a grip held
        // from before presses it no more"), nothing more pressed, why said; play's own Fire live beside it (Space a quick-time action's still): its taps
        // go on, the left mouse button never pressed.
        Run2 r;
        r.await = r.quick = kSpace;
        unsigned long long lastEdge = 0, longest = 0;
        int edges = 0;
        for (int i = 0; i < 216; ++i) {
            r.scripted = i != 72;
            const int before = r.edges[Bit(kSpace)];
            const unsigned long long t = r.now;
            r.Frame(0.0f, 1.0f);
            if (r.edges[Bit(kSpace)] > before) {
                if (lastEdge && t - lastEdge > longest)
                    longest = t - lastEdge;
                lastEdge = t;
                ++edges;
            }
        }
        Run2 g;
        g.scripted = true;
        g.await = g.quick = kSpace;
        g.Frames(72, 0.0f, 1.0f);
        const int gBefore = g.edges[Bit(kSpace)];
        g.quick = 0;
        g.Frames(144, 0.0f, 1.0f);
        const int gAfter = g.edges[Bit(kSpace)] - gBefore;
        Run2 p;
        p.scripted = true;
        p.await = p.quick = kSpace;
        p.Frames(72, 0.0f, 1.0f);
        const int pBefore = p.edges[Bit(kSpace)];
        p.await = kSpace | kLmb;  // (its quick keys still Space)
        p.Frames(144, 0.0f, 1.0f);
        const int pAfter = p.edges[Bit(kSpace)] - pBefore;
        Report("28b", "the scripted flag gone a frame in a held climb: its taps go on, 0.22 s apart at most; a fight's triggers live beside it: the held press "
                      "ends, nothing more, why said; play's Fire live beside it: Space's taps go on, the left mouse button never",
               edges >= 20 && longest <= 220 && gBefore >= 6 && gAfter == 0 && g.lastWhy == qte::End::kHeldOff && g.last.idle == qte::Idle::kPlayKey &&
                   pAfter >= 12 && !(p.ever & kLmb),
               "the flag gone a frame: %d taps in 3 s, %llu ms apart at the most; a fight beside: %d taps, then %d (ended \"%s\", idle %d); play's Fire beside: %d "
               "taps on, keys ever %02X",
               edges, longest, gBefore, gAfter, qte::Word(g.lastWhy), static_cast<int>(g.last.idle), pAfter, p.ever);
    }
    {
        // 28c A press prompt's tap (the grip held, on the prompt's first frame) with its next frame read unclearly: the fights not read, or its gates
        // unclear with only its own key's open (its right gate read shut, its wrong one open: its closing caught halfway): E stays down its full 70 ms
        // (before the review: cut to one frame, 14 ms, and the prompt counted pressed), shown by the game's input, one press, a later squeeze refused; so
        // unclear 140 ms: up at 70 ms all the same. Another action's gate open with it (the game changing prompts: E a wrong key for the next), or bf3
        // out of front the frame after: let go at once, as ever (the tap cut, counted pressed: no telling it wasn't taken).
        const auto run = [](int kind, unsigned long long& downMs, int& presses, int& refused, int& cut, unsigned& keysNext) {
            Run2 r;
            r.scripted = r.prompt = r.once = true;
            r.mash = false;
            r.await = kE;
            r.gates = kE;
            r.Frame(0.0f, 1.0f);  // (its first frame: E down)
            const int unclear = kind == 2 ? 10 : 1;
            for (int i = 0; i < unclear; ++i) {
                if (kind == 0)
                    r.await = 0, r.prompt = false, r.unknown = true, r.gates = 0;  // (the fights not read)
                else if (kind == 3)
                    r.front = false;  // (bf3 out of front)
                else if (kind == 4)
                    r.await = 0, r.gates = kE | kSpace;  // (Space's wrong gate open beside E's)
                else
                    r.await = 0;  // (its gates unclear: E's alone)
                r.Frame(0.0f, 1.0f);
                if (!i)
                    keysNext = r.keys;
            }
            r.await = kE, r.prompt = true, r.unknown = false, r.front = true, r.gates = kE;
            r.Frames(30, 0.0f, 1.0f);
            r.Frames(3, 0.0f, 0.0f);
            r.Frames(20, 0.0f, 1.0f);
            downMs = r.longestDown;
            presses = r.edges[Bit(kE)];
            refused = r.refused;
            cut = r.cut;
        };
        unsigned long long d[5] = {};
        int p[5] = {}, f[5] = {}, c[5] = {};
        unsigned k[5] = {};
        for (int kind = 0; kind < 5; ++kind)
            run(kind, d[kind], p[kind], f[kind], c[kind], k[kind]);
        bool kept = true;
        for (int kind = 0; kind < 3; ++kind)
            kept = kept && d[kind] == qte::kTap && p[kind] == 1 && f[kind] >= 1 && c[kind] == 0;
        Report("28c", "a press prompt's tap with its next frame read unclearly (the fights not read; its gates unclear with its own alone; so 140 ms): E down "
                      "its 70 ms, one press, a later squeeze refused, no tap cut; another action's gate open beside, or bf3 out of front: let go at once (cut, "
                      "counted pressed)",
               kept && k[0] == kE && k[1] == kE && d[3] == kFrame && p[3] == 1 && f[3] >= 1 && c[3] == 1 && k[3] == 0 && d[4] == kFrame && p[4] == 1 && f[4] >= 1 &&
                   c[4] == 1 && k[4] == 0,
               "not read: E down %llu ms, presses %d, refused %d, cut %d, keys the frame after %02X; its own gates unclear: %llu, %d, %d, %d, %02X; so 140 ms: %llu, "
               "%d, %d, %d; out of front: %llu, %d, %d, %d, %02X; another's gate beside: %llu, %d, %d, %d, %02X",
               d[0], p[0], f[0], c[0], k[0], d[1], p[1], f[1], c[1], k[1], d[2], p[2], f[2], c[2], d[3], p[3], f[3], c[3], k[3], d[4], p[4], f[4], c[4], k[4]);
    }
    {
        // 28d The cellar with the right grip held from before, and the frame after each prompt's tap began one the fights couldn't be read (a gate's byte
        // neither 0 nor 1 that frame: the fight not as attached, attached again the next): over the three ways the game's input ticks (30 a second) fall
        // on the frames, won every time, each prompt pressed once (before the review the tap was cut to that one frame and the prompt counted pressed:
        // one of the two prompts' taps missed by the game's ticks in each of the three, its window run out: lost in all three, as qte.h without the fix
        // measured on 2026-11-19).
        int won = 0, once = 0, tears = 0;
        std::string failed;
        for (int phase = 0; phase < 3; ++phase) {
            FightGame game = CellarFight();
            game.startMs = 500 + static_cast<unsigned long long>(phase) * kFrame;
            FightMod mod;
            const uint32_t byte = game.g.f.entity[qtefight::kActions[4].wrong] + 0x14;  // (Space's wrong gate, shut)
            for (int i = 0; i < 3000 && !game.Over(); ++i) {
                const bool tear = mod.run.last.began && (game.phase == FightGame::Phase::kOpen || game.phase == FightGame::Phase::kClosing);
                if (tear) {
                    game.g.m.W8(byte, 2);
                    ++tears;
                }
                mod.Frame(game, 0.0f, 1.0f);
                if (tear)
                    game.g.m.W8(byte, 0);
            }
            won += game.phase == FightGame::Phase::kWon;
            once += game.presses[0] == 1 && game.presses[1] == 1 && game.wrongPresses == 0;
            if (game.phase != FightGame::Phase::kWon)
                failed += failed.empty() ? game.failed : "";
        }
        Report("28d", "the cellar held from before with the frame after each tap one the fights couldn't be read, over the 3 ways the game's input ticks fall: "
                      "won every time, each prompt pressed once",
               won == 3 && once == 3 && tears == 6, "won %d of 3, once each %d of 3, unclear frames %d%s%s", won, once, tears, failed.empty() ? "" : "; failed: ",
               failed.c_str());
    }
    {
        // 28e A press prompt whose tap the game's input never showed, read throughout (a key held down elsewhere as it opened, a click held back): a fresh
        // squeeze presses it again ("once more"), why nothing was down meanwhile said (its tap never shown); a tap shown: the next squeeze refused, one
        // press a prompt as before; the input not read during a tap: counted pressed (no telling), a squeeze after refused; a tap cut by bf3 leaving the
        // front 14 ms in, or the headset's focus lost (Stop2): the same; a grip held on over a frame not read clearly after a tap shown: not taken for a
        // second press, said as held on (no squeeze of the player's).
        Run2 r;
        r.scripted = r.prompt = r.once = true;
        r.mash = false;
        r.await = kLmb;
        r.game = 0;  // (read, never showing it)
        r.Frames(10, 0.0f, 1.0f);
        const qte::Idle unshownIdle = r.last.idle;
        r.Frames(3, 0.0f, 0.0f);
        r.Frames(10, 0.0f, 1.0f);
        const int second = r.edges[Bit(kLmb)];
        r.game = 1;  // (shown from here on)
        r.Frames(3, 0.0f, 0.0f);
        r.Frames(10, 0.0f, 1.0f);
        const int third = r.edges[Bit(kLmb)];
        const qte::Idle answeredIdle = (r.Frames(3, 0.0f, 0.0f), r.Frame(0.0f, 1.0f).idle);
        r.Frames(10, 0.0f, 1.0f);
        Run2 b;
        b.scripted = b.prompt = b.once = true;
        b.mash = false;
        b.await = kE;
        b.game = 0;
        b.seenRead = false;
        b.Frames(10, 0.0f, 1.0f);
        b.seenRead = true;
        b.Frames(3, 0.0f, 0.0f);
        b.Frames(10, 0.0f, 1.0f);
        Run2 c;
        c.scripted = c.prompt = c.once = true;
        c.mash = false;
        c.await = kE;
        c.game = 0;
        c.Frame(0.0f, 1.0f);
        c.front = false;
        c.Frame(0.0f, 1.0f);
        c.front = true;
        c.Frames(3, 0.0f, 0.0f);
        c.Frames(10, 0.0f, 1.0f);
        Run2 s;
        s.scripted = s.prompt = s.once = true;
        s.mash = false;
        s.await = kE;
        s.game = 0;
        s.Frame(0.0f, 1.0f);
        s.Stop(qte::End::kFront);
        s.Frames(3, 0.0f, 0.0f);
        s.Frames(10, 0.0f, 1.0f);
        Run2 h;
        h.scripted = h.prompt = h.once = true;
        h.mash = false;
        h.await = kE;
        h.Frames(10, 0.0f, 1.0f);
        h.await = 0;  // (its gates unclear a frame, past the tap's 70 ms)
        h.Frame(0.0f, 1.0f);
        h.await = kE;
        h.Frames(10, 0.0f, 1.0f);
        Report("28e", "a press prompt's tap never shown by the game's input: a fresh squeeze presses it again, why said; shown: the next squeeze refused; the input "
                      "not read, or the tap cut (out of front, Stop2): refused after; a grip held on over an unclear frame: not taken again, said as held on",
               unshownIdle == qte::Idle::kUnshown && second == 2 && third == 3 && r.again == 2 && r.refused == 1 && r.edges[Bit(kLmb)] == 3 &&
                   answeredIdle == qte::Idle::kAnswered && b.edges[Bit(kE)] == 1 && b.refused == 1 && c.edges[Bit(kE)] == 1 && c.refused == 1 && c.cut == 1 &&
                   s.edges[Bit(kE)] == 1 && s.refused == 1 && h.edges[Bit(kE)] == 1 && h.refused >= 1 && h.refusedHeld == h.refused,
               "never shown: idle %d, presses %d then %d (again %d), then %d, refused %d, idle %d; not read: presses %d refused %d; cut out of front: %d %d (cut %d); "
               "Stop2: %d %d; held on: presses %d, refused %d (held on %d)",
               static_cast<int>(unshownIdle), second, third, r.again, r.edges[Bit(kLmb)], r.refused, static_cast<int>(answeredIdle), b.edges[Bit(kE)], b.refused,
               c.edges[Bit(kE)], c.refused, c.cut, s.edges[Bit(kE)], s.refused, h.edges[Bit(kE)], h.refused, h.refusedHeld);
    }
    {
        // 28f The cellar's Fire prompt with the right trigger holding the left mouse button down as it opens (pulled 0.2 s before, let go 0.1 s in): the
        // held grip's tap makes no new press (the button down already), the game's input never shows it; the grip let go 0.15 s in and squeezed again
        // 0.07 s later: pressed again, won (before the review: refused, one press a prompt, and the 0.41 s window ran out).
        FightGame game = CellarFight();
        FightMod mod;
        for (int i = 0; i < 3000 && !game.Over(); ++i) {
            const unsigned long long t = mod.run.now;
            const bool open = game.phase == FightGame::Phase::kOpen;  // (till its press is taken: its closing counted from then)
            const unsigned long long in = open && t >= game.phaseAt ? t - game.phaseAt : 0;
            const bool fire = game.beat == 1;
            const unsigned other = fire && ((game.phase == FightGame::Phase::kGap && t + 200 >= game.phaseAt + game.gapMs) || (open && in < 100)) ? kLmb : 0u;
            const float right = fire && open && in >= 150 && in < 220 ? 0.0f : 1.0f;
            mod.Frame(game, 0.0f, right, other);
        }
        Report("28f", "the cellar's Fire prompt opening with the trigger's left mouse button down: the held grip's tap no press, never shown; let go and squeezed "
                      "again: pressed again, won",
               game.phase == FightGame::Phase::kWon && game.wrongPresses == 0 && mod.run.again == 1 && mod.run.refused == 0 && mod.run.began == 3 &&
                   game.rightPresses >= 2,
               "%s%s; presses begun %d (again %d), refused %d; the game took E %d, Fire %d (right %d, between prompts %d)",
               game.phase == FightGame::Phase::kWon ? "won" : "NOT WON: ", game.failed, mod.run.began, mod.run.again, mod.run.refused, game.presses[0],
               game.presses[1], game.rightPresses, game.betweenPresses);
    }
    {
        // 28g Other quick-time keys the game's input shows while a fight's prompt is up (qte::OthersStep, qtewatch.cpp's lines): E's prompt opening with Space
        // down (held from before: said, no press of it); F gone down while it's up: said once, E's own presses never; a frame the input wasn't read:
        // nothing, counted; Z gone down on the frame it closes: said as closing it, with F among those while up; a next prompt with no frame between:
        // its own key going down is its answer, never the last one's wrong key.
        qte::Others w;
        const qte::OthersOut o1 = qte::OthersStep(w, kE, kSpace, true);
        const qte::OthersOut o2 = qte::OthersStep(w, kE, kSpace | kF | kE, true);
        const qte::OthersOut o3 = qte::OthersStep(w, kE, kSpace | kF, true);
        const qte::OthersOut o4 = qte::OthersStep(w, kE, 0, false);
        const qte::OthersOut o5 = qte::OthersStep(w, kE, kSpace, true);
        const qte::OthersOut o6 = qte::OthersStep(w, 0, kSpace | kZ, true);
        const qte::OthersOut o7 = qte::OthersStep(w, 0, kSpace | kZ | kF, true);
        qte::Others v;
        qte::OthersStep(v, kE, 0, true);
        const qte::OthersOut v2 = qte::OthersStep(v, kLmb, kLmb, true);
        const qte::OthersOut v3 = qte::OthersStep(v, kLmb, kLmb | kRmb, true);
        Report("28g", "other quick-time keys while a fight's prompt is up: down as it opened said as held; one gone down said once, the answer's own never; an "
                      "unread frame nothing; one gone down as it closes said so; a next prompt's own key no wrong key of the last",
               o1.atOpen == kSpace && !o1.went && !o1.closed && o2.went == kF && !o3.went && !o4.went && !o5.went && o6.closed && o6.closedKey == kE &&
                   o6.wentAsClosed == kZ && o6.wentWhileUp == kF && o6.unread == 1 && o6.frames == 5 && !o7.closed && !o7.went && v2.closed && !v2.wentAsClosed &&
                   !v2.atOpen && v3.went == kRmb,
               "open: held %02X; while up %02X %02X %02X %02X; closed %d (key %02X): as it closed %02X, while up %02X, unread %d of %d; after %d; next prompt: as "
               "closed %02X, held %02X, then %02X",
               o1.atOpen, o2.went, o3.went, o4.went, o5.went, o6.closed, o6.closedKey, o6.wentAsClosed, o6.wentWhileUp, o6.unread, o6.frames, o7.closed,
               v2.wentAsClosed, v2.atOpen, v3.went);
    }
    {
        // 28h The owner's cellar of 2026-11-19 as its log has it: no grip squeezed, and a quick-time key from outside the mod (Space: no guard between it and
        // the game, a real keyboard's or a pad's) 0.2 s into the 0.41 s Fire prompt: the fight fails, a wrong key; the other keys said name Space (gone down
        // while Fire was up or as its prompt closed), nothing else; the cellar won with the grip held and nothing from outside: no other key said.
        FightGame game = CellarFight();
        FightMod mod;
        mod.useGuard = false;  // (the other input stands for one outside the mod: never through the guard)
        int after = 0;  // (frames read after the fight is over: the reader goes on, as in the game)
        for (int i = 0; i < 3000 && after < 5; ++i) {
            after += game.Over();
            const unsigned long long t = mod.run.now;
            const bool open = game.phase == FightGame::Phase::kOpen || game.phase == FightGame::Phase::kClosing;
            const unsigned long long in = open && t >= game.phaseAt ? t - game.phaseAt : 0;
            const float right = game.beat == 0 ? 1.0f : 0.0f;  // (E answered, as the owner's was; Fire's left to the stray key)
            const unsigned other = game.beat == 1 && open && in >= 200 && in < 270 ? kSpace : 0u;
            mod.Frame(game, 0.0f, right, other);
        }
        const unsigned said = mod.othersWent | mod.othersAsClosed | mod.othersWhileUp;
        FightGame clean = CellarFight();
        FightMod held;
        RunFight(clean, held, [](FightGame&, unsigned long long, float&, float& r) { r = 1.0f; });
        const unsigned cleanSaid = held.othersWent | held.othersAsClosed | held.othersWhileUp | held.othersAtOpen;
        Report("28h", "the owner's cellar: a stray Space from outside the mod 0.2 s into the Fire prompt fails the fight; the other keys said name Space alone; "
                      "won held with nothing from outside: none said",
               game.phase == FightGame::Phase::kFailed && game.wrongPresses == 1 && said == kSpace && mod.othersClosed >= 2 &&
                   clean.phase == FightGame::Phase::kWon && cleanSaid == 0,
               "%s (%s); said %02X (while up %02X, as it closed %02X), prompts closed %d; held, nothing from outside: %s, said %02X",
               game.phase == FightGame::Phase::kFailed ? "failed" : "NOT FAILED", game.failed, said, mod.othersWent, mod.othersAsClosed, mod.othersClosed,
               clean.phase == FightGame::Phase::kWon ? "won" : "NOT WON", cleanSaid);
    }
    {
        // 28j The grips' log with a prompt's gates read unclearly every other frame for 10 s (the review's count: 719 lines, the run's 1500 used up in 21 s):
        // one stretch, its lines as ever (4 at most) and one end; the fights not read every other frame: the same; a frame read with no prompt between two
        // ends the stretch, as before.
        const auto run = [](int kind, int& says, int& ends) {
            Run2 r;
            r.scripted = true;
            r.mash = false;
            qte::Watch2 w;
            says = ends = 0;
            for (int i = 0; i < 72 * 10; ++i) {
                const bool clear = i % 2 == 0;
                r.prompt = clear || kind == 0;
                r.once = r.prompt;
                r.unknown = !clear && kind == 1;
                r.await = clear ? kE : 0u;
                r.gates = r.prompt ? kE : 0u;
                if (!clear && kind == 2)
                    r.prompt = r.once = false;  // (read with no prompt)
                r.Frame(0.0f, 1.0f);
                const qte::WatchOut wo = qte::WatchStep(w, r.lastIn, r.last);
                says += wo.say, ends += wo.sayEnd;
            }
            r.await = 0, r.prompt = r.once = r.unknown = false;
            r.Frame(0.0f, 1.0f);
            ends += qte::WatchStep(w, r.lastIn, r.last).sayEnd;
        };
        int s0 = 0, e0 = 0, s1 = 0, e1 = 0, s2 = 0, e2 = 0;
        run(0, s0, e0);
        run(1, s1, e1);
        run(2, s2, e2);
        Report("28j", "the grips' log, a prompt's gates unclear every other frame 10 s: one stretch (4 lines at most, one end); the fights not read every other "
                      "frame: the same; read with no prompt between: a stretch each, as before",
               s0 <= qte::kWatchLines && e0 == 1 && s1 <= qte::kWatchLines && e1 == 1 && s2 > 300 && e2 > 300,
               "unclear: %d lines, %d ends; not read: %d, %d; no prompt between: %d, %d", s0, e0, s1, e1, s2, e2);
    }
    {
        // 28i The words: a held press ended as such a grip may press its keys no more; its tap never shown; a stretch's end with a press again and a held grip
        // not taken again.
        char text[640];
        qte::Watch2 w;
        w.keys = kE, w.prompt = w.once = w.scripted = true, w.frames = 30, w.presses = 2, w.again = 1, w.refused = 2, w.refusedHeld = 1, w.firstHand = 1;
        w.least[0] = w.least[1] = 0.0f, w.most[0] = w.most[1] = 1.0f;
        qte::WatchEndText(w, text, sizeof(text));
        const bool end = std::strstr(text, "pressed again 1 time, the press before never shown by the game's input") &&
                         std::strstr(text, "; 1 squeeze after its one press refused") && std::strstr(text, "a grip held on through a break in it") &&
                         std::strstr(text, "not taken for a second press 1 time");
        Report("28i", "the words: a held press ended (play's key or a fight's triggers beside it); a tap never shown; a stretch's end with a press again and a "
                      "held grip not taken again",
               std::strstr(qte::Word(qte::End::kHeldOff), "held from before") && std::strstr(qte::IdleWord(qte::Idle::kUnshown), "never shown") && end,
               "\"%s\"; \"%s\"; \"%s\"", qte::Word(qte::End::kHeldOff), qte::IdleWord(qte::Idle::kUnshown), text);
    }
}

// ---------------------------------------------------------------- 29: the log lines' truth ----------------------------------------------------------------
// The in-game check of 2026-11-18 (its fixes A and B): the squeeze's line said a fight's press prompt was "tapped 70 ms, and again every 130 ms while the grip
// stays squeezed" (one tap a squeeze, in.mash false), and with "qtefight 0" each fight's trigger's line said it was "pressed only for its open prompt".
void LogTruth() {
    // 29a: each kind of prompt as input.cpp's QteStep asks Step2 for it (qte::TapRule), the grip squeezed a second: the squeeze's line's words (qte::TapWords,
    // QteSay2's) and the taps it made.
    struct Kind {
        const char* name;
        bool qtemash, prompt, promptMash;
        const char* words;  // in the line
        int least, most;    // taps in the second
    } const kinds[] = {
        {"a fight's press prompt", true, true, false, "once (a fight's press prompt: one press a prompt)", 1, 1},
        {"a fight's mash", true, true, true, "and again every 130 ms while the grip stays squeezed", 7, 9},
        {"a single", true, false, false, "and again every 130 ms while the grip stays squeezed", 7, 9},
        {"a single, qtemash 0", false, false, false, "once a squeeze", 1, 1},
        {"a fight's mash, qtemash 0", false, true, true, "once a squeeze", 1, 1},
        {"a fight's press prompt, qtemash 0", false, true, false, "once (a fight's press prompt: one press a prompt)", 1, 1},
    };
    int right = 0;
    std::string wrong;
    for (const Kind& k : kinds) {
        Run2 r;
        r.await = kE;
        qte::TapRule(k.qtemash, k.prompt, k.promptMash, r.mash, r.once);
        r.prompt = k.prompt;
        r.gates = k.prompt ? kE : 0u;
        r.Frames(5, 0.0f, 0.0f);
        std::string said = "(no squeeze began)";
        for (int i = 0; i < 72; ++i)  // (a second)
            if (const qte::Out o = r.Frame(0.0f, 1.0f); o.began)
                said = qte::TapWords(o);
        const int taps = r.edges[Bit(kE)];
        const bool ok = r.began == 1 && said.find(k.words) != std::string::npos && taps >= k.least && taps <= k.most &&
                        (k.least > 1) == (said.find("again every") != std::string::npos);
        right += ok;
        if (!ok && wrong.empty()) {
            char text[400];
            std::snprintf(text, sizeof(text), "%s: said \"%s\", %d taps, %d presses", k.name, said.c_str(), taps, r.began);
            wrong = text;
        }
    }
    // The words with every key held: nothing of taps; the press prompt pressed again (its tap before never shown): "once more".
    qte::Out held;
    held.awaited = held.holds = kSpace;
    qte::Out again;
    again.awaited = kE, again.once = again.again = true;
    const bool words = !*qte::TapWords(held) && std::strstr(qte::TapWords(again), "once more");
    Report("29a", "the squeeze's line says the taps it makes: a fight's press prompt once (never \"again every 130 ms\"), a mash and a single again every 130 ms, "
                  "\"qtemash 0\" once a squeeze; every key held, no taps said",
           right == static_cast<int>(std::size(kinds)) && words, "%d of %zu kinds right%s%s; held \"%s\", pressed again \"%s\"", right, std::size(kinds),
           wrong.empty() ? "" : "; the first wrong: ", wrong.c_str(), qte::TapWords(held), qte::TapWords(again));
    // 29b: a fight's trigger's line: pressed for its open prompt only with the fights' reader on; with it off ("qtefight 0", or a constant not borne out)
    // it presses nothing, and says so.
    const char* on = qtefight::TriggerWords(true);
    const char* off = qtefight::TriggerWords(false);
    Report("29b", "a fist fight's trigger's line: \"pressed only for its open prompt\" with the fights' reader on, \"presses nothing\" with it off",
           std::strstr(on, "pressed only for its open prompt") && !std::strstr(off, "pressed only") && std::strstr(off, "presses nothing") &&
               std::strstr(off, "reader is off"),
           "on: \"%s\"; off: \"%s\"", on, off);
}

}  // namespace

// ---- 30: one owner per squeeze (the audit of 2026-11-19): a single's key awaited in play took the first fresh squeeze of either grip while the hands took
// the very same squeeze. In play a hand holding something never answers; a free hand's squeeze that presses is the event's till it's let go (Owner); a
// fist fight's run holds the hands (FightStep). ----
void Owners() {
    // 30a: JumpClimb's Space awaited in play, the left hand holding a magazine taken at the belly: its squeeze presses nothing, held on or squeezed again
    // (still holding it), and the grips' log says why; the magazine let go of (the hand free), seen open and squeezed: Space.
    {
        Run2 r;
        r.await = kSpace;
        r.holds[0] = true;
        r.Frames(10, 1.0f, 0.0f);
        const qte::Idle why = r.last.idle;
        r.Frames(2, 0.2f, 0.0f);
        r.Frames(10, 1.0f, 0.0f);
        const unsigned whileHolding = r.ever;
        r.holds[0] = false;
        r.Frames(3, 0.0f, 0.0f);
        r.Frames(10, 1.0f, 0.0f);
        Report("30a", "a single's Space awaited in play, the left holding a magazine (the belly's): its squeeze presses nothing, held or squeezed again; free, "
                      "let go and squeezed: Space",
               whileHolding == 0 && why == qte::Idle::kHolding && r.edges[Bit(kSpace)] >= 1 && r.began == 1 && r.lastHand == 0,
               "while holding %02X (why %d), then %d presses of Space by the %s", whileHolding, static_cast<int>(why), r.edges[Bit(kSpace)],
               r.lastHand == 0 ? "left" : "right");
    }
    // 30b: the gun in the right hand (its grip held) and the left on its handguard, Space awaited in play. The gun's own holds never bar a grip
    // (qte::HoldsForKey: hands.cpp publishes them as not holding; grasp.h's keyAwaited keeps the gun in the hand meanwhile): the right let go 3 frames and
    // squeezed afresh (0.1.14's "squeeze afresh") presses Space; held on, nothing more; the left let go of the handguard and squeezed afresh presses it
    // again (the review of 2026-11-25, finding 2: with the rifle in both hands no grip answered).
    {
        qte::Carry gunHand, guardHand;
        gunHand.gunGrip = true;
        guardHand.handguard = true;
        Run2 r;
        r.await = kSpace;
        r.holds[1] = qte::HoldsForKey(gunHand);
        r.holds[0] = qte::HoldsForKey(guardHand);
        r.Frames(5, 1.0f, 1.0f);
        const unsigned heldOn = r.ever;
        r.Frames(3, 1.0f, 0.1f);
        r.Frames(10, 1.0f, 1.0f);
        const int byGun = r.began, byGunHand = r.lastHand;
        const bool spaced = (r.ever & kSpace) != 0;
        r.Frames(10, 1.0f, 0.1f);
        r.Frames(3, 0.1f, 0.1f);
        r.Frames(10, 1.0f, 0.1f);
        Report("30b", "the gun's own grips in play: held on nothing; the gun's right let go and squeezed afresh: Space; the handguard's left likewise: Space",
               heldOn == 0 && byGun == 1 && byGunHand == 1 && spaced && r.began == 2 && r.lastHand == 0,
               "held on %02X; the right %d press(es) (hand %d, Space %d); then the left: %d in all (hand %d)", heldOn, byGun, byGunHand, spaced, r.began,
               r.lastHand);
    }
    // 30c: the owner (qte::OwnerStep, as input.cpp publishes it for hands.cpp): the left's squeeze that pressed Space is the event's from its first frame,
    // through the key's end, till it's let go under 0.4 (eased to 0.5: still); the right holding the gun never; a squeeze in play that pressed nothing
    // (the holding hand's) never.
    {
        Run2 r;
        qte::Owner w;
        const auto frame = [&](float left, float right) {
            r.Frame(left, right);
            const bool read[2] = {true, true};
            const float squeeze[2] = {left, right};
            qte::OwnerStep(w, r.s, r.last, read, squeeze);
        };
        r.await = kSpace;
        r.holds[1] = true;
        for (int i = 0; i < 5; ++i)
            frame(0.0f, 1.0f);
        const bool before = w.owns[0] || w.owns[1];
        frame(1.0f, 1.0f);
        const bool first = w.owns[0] && r.began == 1;
        for (int i = 0; i < 10; ++i)
            frame(1.0f, 1.0f);
        r.await = 0;  // (the trigger gone: the press over, the squeeze still the event's)
        for (int i = 0; i < 10; ++i)
            frame(1.0f, 1.0f);
        const bool afterKey = w.owns[0];
        for (int i = 0; i < 5; ++i)
            frame(0.5f, 1.0f);
        const bool eased = w.owns[0];
        frame(0.3f, 1.0f);
        const bool letGo = !w.owns[0];
        Report("30c", "the owner: the left's pressing squeeze the event's from its first frame, through the key's end and eased to 0.5, free once under 0.4; "
                      "the gun's right hand never",
               !before && first && afterKey && eased && letGo && !w.owns[1], "before %d, first frame %d, after the key %d, eased %d, let go %d, right %d",
               before, first, afterKey, eased, !letGo, w.owns[1]);
    }
    // 30d: in a scripted moment the holds are as before (hands held still there): the gun's right grip held as a single's quick key comes presses it kHeldSettle
    // on (27's rule), a fresh squeeze of a holding hand presses too.
    {
        Run2 r;
        r.scripted = true;
        r.await = kSpace;
        r.quick = kSpace;
        r.holds[0] = r.holds[1] = true;
        r.Frames(12, 0.0f, 1.0f);
        const bool held = r.began == 1 && r.byHold == 1 && r.lastHand == 1;
        Run2 q;
        q.scripted = true;
        q.await = kE;
        q.holds[0] = true;
        q.Frames(3, 0.0f, 0.0f);
        q.Frames(5, 1.0f, 0.0f);
        Report("30d", "a scripted moment: the holds count for nothing (the gun's held grip presses a single's key 0.1 s on; a holding hand's fresh squeeze presses)",
               held && q.began == 1 && q.lastHand == 0, "held grip: %d presses by hold %d (hand %d); fresh: %d", r.began, r.byHold, r.lastHand, q.began);
    }
    // 30e: a fight's press prompt: the gun's right grip, held, answers it the frame it opens (27), whatever it holds (the fight holds the hands still).
    {
        Run2 r;
        r.prompt = true, r.once = true;
        r.await = kE;
        r.gates = kE;
        r.holds[1] = true;
        r.Frame(0.0f, 1.0f);
        Report("30e", "a fight's prompt: the gun's held grip answers at once, its holding no bar", r.began == 1 && r.byHold == 1 && r.lastHand == 1 &&
                                                                                                      r.edges[Bit(kE)] == 1,
               "%d presses (by hold %d, hand %d)", r.began, r.byHold, r.lastHand);
    }
    // 30f: a press under way by the free left, the left then holding something (a gun given into it): its press ends, nothing more pressed till it's free,
    // seen let go and squeezed again.
    {
        Run2 r;
        r.await = kSpace;
        r.Frames(3, 0.0f, 0.0f);
        r.Frames(4, 1.0f, 0.0f);
        const bool pressing = r.s.on;
        r.holds[0] = true;
        r.Frames(20, 1.0f, 0.0f);
        const int edgesHolding = r.edges[Bit(kSpace)];
        const bool ended = !r.s.on && r.keys == 0;
        r.holds[0] = false;
        r.Frames(10, 1.0f, 0.0f);
        const int stillNone = r.edges[Bit(kSpace)] - edgesHolding;
        r.Frames(3, 0.0f, 0.0f);
        r.Frames(5, 1.0f, 0.0f);
        Report("30f", "a press under way, its hand then holding something: ended, nothing more; free but not let go: nothing; let go and squeezed: pressed",
               pressing && ended && stillNone == 0 && r.edges[Bit(kSpace)] > edgesHolding, "pressing %d, ended %d, presses held on free %d, after %d",
               pressing, ended, stillNone, r.edges[Bit(kSpace)] - edgesHolding);
    }
    // 30g: a fist fight's run (qte::FightStep): its prompt; between its beats (a fight still read) within 6 s; past that, or no fight read: over; a frame
    // the fights couldn't be read: as it was, 0.25 s at most; a clock gone back: over.
    {
        qte::FightLive f;
        unsigned long long t = 500000;
        const bool none = !qte::FightStep(f, false, false, 1, t);  // (a fight read, no prompt yet: the hands as ever)
        const bool prompt = qte::FightStep(f, true, false, 1, t += 100);
        const bool gap = qte::FightStep(f, false, false, 1, t += 3000);
        const bool unknownKept = qte::FightStep(f, false, true, 0, t += 200);
        const bool unknownLapsed = !qte::FightStep(f, false, true, 0, t += 300);
        qte::FightLive g;
        qte::FightStep(g, true, false, 1, 600000);
        const bool atEdge = qte::FightStep(g, false, false, 1, 606000);  // (6.0 s after its last prompt: still)
        const bool past = atEdge && !qte::FightStep(g, false, false, 1, 606001);
        qte::FightLive h;
        qte::FightStep(h, true, false, 1, 700000);
        const bool gone = !qte::FightStep(h, false, false, 0, 701000);
        const bool back = !qte::FightStep(h, false, false, 1, 701500);  // (its beats forgotten with the fight)
        qte::FightLive k;
        qte::FightStep(k, true, false, 1, 800000);
        const bool clockBack = !qte::FightStep(k, false, false, 1, 700000);
        Report("30g", "a fight's run: its prompt, between its beats within 6 s; not before its first prompt, past 6 s, with no fight read; a frame not read "
                      "0.25 s at most; a clock gone back: over",
               none && prompt && gap && unknownKept && unknownLapsed && past && gone && back && clockBack,
               "none %d, prompt %d, gap %d, unread kept %d lapsed %d, past %d, gone %d, back %d, clock %d", none, prompt, gap, unknownKept, unknownLapsed, past,
               gone, back, clockBack);
    }
    // 30h: the words: the grips' log for a holding hand in play, and why nothing's down.
    {
        qte::In in;
        in.read[0] = true, in.squeeze[0] = 1.0f, in.holds[0] = true, in.await = kSpace;
        qte::State2 s;
        const std::string inPlay = qte::GripState(in, s, 0);
        in.cutscene = true;
        const std::string scripted = qte::GripState(in, s, 0);
        const std::string why = qte::IdleWord(qte::Idle::kHolding);
        Report("30h", "the words: a holding hand's grip in play, the same in a scripted moment, the idle reason",
               inPlay.find("holds something") != std::string::npos && scripted.find("holds") == std::string::npos && why.find("empty hand") != std::string::npos,
               "\"%s\" / \"%s\" / \"%s\"", inPlay.c_str(), scripted.c_str(), why.c_str());
    }
    // 30i: what holding is (qte::HoldsForKey, as hands.cpp publishes In::holds): the gun's own holds never (its pistol grip, its front, the handguard, a
    // launcher's front grip, the train pistol in the hand or both hands on it); a carried thing always (a magazine, round or case, the charging handle, the
    // pistol's slide or magazine, a prompt's use, the gun in a hand not seen at a holster's place), with the gun's holds or without.
    {
        bool gunNever = true, carriedAlways = true;
        for (int g = 0; g < 64; ++g) {
            qte::Carry c;
            c.gunGrip = g & 1, c.gunFront = g & 2, c.handguard = g & 4, c.launcherFront = g & 8, c.pistol = g & 16, c.pistolBoth = g & 32;
            gunNever = gunNever && !qte::HoldsForKey(c);
            for (int k = 0; k < 6; ++k) {
                qte::Carry d = c;
                (k == 0 ? d.item : k == 1 ? d.handle : k == 2 ? d.slide : k == 3 ? d.pistolMag : k == 4 ? d.use : d.unseenGun) = true;
                carriedAlways = carriedAlways && qte::HoldsForKey(d);
            }
        }
        Report("30i", "holding, for a key in play: the gun's own holds never (64 mixes), a carried thing always (with any of them)", gunNever && carriedAlways,
               "gun's holds never %d, carried always %d", gunNever, carriedAlways);
    }
    // 30j: a squeeze owed for the key-up gap (a re-squeeze within 60 ms of the last key going up) is the event's at once (Owner), so the hands take nothing
    // with it; a hand that comes to hold something meanwhile has its owed press dropped (none begins with a squeeze the hands took: one owner). The
    // review's probe of 2026-11-25: owed 1, owned 0, then a press began with the hand holding a magazine.
    {
        Run2 r;
        qte::Owner w;
        const auto frame = [&](float left, float right) {
            r.Frame(left, right);
            const bool read[2] = {true, true};
            const float squeeze[2] = {left, right};
            qte::OwnerStep(w, r.s, r.last, read, squeeze);
        };
        r.await = kSpace;
        for (int i = 0; i < 3; ++i)
            frame(0.0f, 1.0f);
        frame(1.0f, 1.0f);
        for (int i = 0; i < 3; ++i)
            frame(1.0f, 1.0f);
        frame(0.0f, 1.0f);
        for (int k = 0; r.s.on && k < 20; ++k)
            frame(0.0f, 1.0f);
        const int pressesBefore = r.began;
        frame(1.0f, 1.0f);  // (again at once, inside the gap: owed)
        const bool owed = r.s.owed, ownedWhileOwed = w.owns[0];
        r.holds[0] = true;  // (the hands' magazine at the belly, had they taken it: they don't, it's the event's; were it so, no press)
        for (int i = 0; i < 8; ++i)
            frame(1.0f, 1.0f);
        const int pressesHolding = r.began - pressesBefore;
        // Not holding: the owed press begins after the gap, by that hand.
        Run2 q;
        qte::Owner v;
        const auto frameQ = [&](float left, float right) {
            q.Frame(left, right);
            const bool read[2] = {true, true};
            const float squeeze[2] = {left, right};
            qte::OwnerStep(v, q.s, q.last, read, squeeze);
        };
        q.await = kSpace;
        for (int i = 0; i < 3; ++i)
            frameQ(0.0f, 1.0f);
        frameQ(1.0f, 1.0f);
        for (int i = 0; i < 3; ++i)
            frameQ(1.0f, 1.0f);
        frameQ(0.0f, 1.0f);
        for (int k = 0; q.s.on && k < 20; ++k)
            frameQ(0.0f, 1.0f);
        const int before = q.began;
        frameQ(1.0f, 1.0f);
        const bool qOwned = v.owns[0];
        for (int i = 0; i < 8; ++i)
            frameQ(1.0f, 1.0f);
        Report("30j", "a squeeze owed for the key-up gap: the event's at once; its hand then holding something: no press begins with it; not holding: it begins",
               owed && ownedWhileOwed && pressesHolding == 0 && qOwned && q.began - before == 1 && q.lastHand == 0,
               "owed %d, owned %d, presses while holding %d; free: owned %d, presses %d (hand %d)", owed, ownedWhileOwed, pressesHolding, qOwned, q.began - before,
               q.lastHand);
    }
    // 30k: how long a key in play keeps the gun in the hand (qte::KeyKeepsGun, input.cpp's QteKeyInPlay: grasp.h's keyAwaited): from its first frame 10 s
    // at most, said once as it passes; a gap under a second keeps its age; a second without it, or a clock gone back: afresh; not awaited: never.
    {
        qte::KeyAge a;
        bool passed = false;
        const bool first = qte::KeyKeepsGun(a, true, 50000, &passed) && !passed;
        bool through = true;
        for (unsigned long long t = 50014; t < 55000; t += 14)
            through = through && qte::KeyKeepsGun(a, true, t);
        const bool gapOff = !qte::KeyKeepsGun(a, false, 55200);  // (half a second not awaited: off then, its age kept)
        for (unsigned long long t = 55500; t < 59990; t += 14)
            through = through && qte::KeyKeepsGun(a, true, t);
        const bool before = qte::KeyKeepsGun(a, true, 59990, &passed) && !passed;
        const bool at = !qte::KeyKeepsGun(a, true, 60000, &passed) && passed;
        const bool after = !qte::KeyKeepsGun(a, true, 60014, &passed) && !passed;
        const bool afresh = qte::KeyKeepsGun(a, true, 61100);  // (a second and more since it was last awaited)
        qte::KeyAge b;
        for (unsigned long long t = 80000; t < 89990; t += 14)
            qte::KeyKeepsGun(b, true, t);
        const bool clockBack = qte::KeyKeepsGun(b, true, 70000);
        qte::KeyAge c;
        const bool never = !qte::KeyKeepsGun(c, false, 100000);
        Report("30k", "a key in play keeps the gun in the hand 10 s at most from its first frame (said once), a half-second gap kept, a second's gap or a "
                      "clock gone back afresh, never when not awaited",
               first && through && gapOff && before && at && after && afresh && clockBack && never,
               "first %d, through %d, gap %d, at 9.99 s %d, at 10 s off and said %d, after (not said again) %d, afresh %d, clock back %d, never %d", first,
               through, gapOff, before, at, after, afresh, clockBack, never);
    }
}

// 31: the key each of a press's E and Space goes down as (input.cpp's QteLogical, by qtewatch.cpp's Frame::proper; the players lane, 2026-11-25):
// the quick-time action's own where one awaits it, a fist fight's triggers live beside it or not; play's Interact and Jump (the player's keys) only
// where play's own action alone awaits it. Before, the routing was QuickKeys', which gives none while a fight's triggers are live: The Great
// Destroyer's train climb beside its fight went down as the player's jump key, never the game's Space for JumpClimb.
void Proper() {
    struct Keys3 {
        unsigned await = 0, quick = 0, proper = 0;
    };
    constexpr uint32_t kBusFight = 0xE7100000u, kBusLevel = 0xE7200000u, kBusPrompt = 0xE7300000u;
    // The list of these triggers (each an action on a bus) in a scripted moment: what Awaited, QuickKeys and ProperKeys give, as qtewatch.cpp asks.
    const auto keysOf = [](const std::vector<std::pair<uint32_t, uint32_t>>& triggers, const qtelist::Skip* skip = nullptr, bool read = true) {
        Image m;
        std::vector<uint32_t> enabled;
        for (const auto& t : triggers) {
            g_triggerBus = t.second;
            enabled.push_back(Entity(m, t.first));
        }
        g_triggerBus = 0;
        Manager(m, enabled);
        const qtelist::List l = read ? ReadList(m) : qtelist::Read(m, 0, kTypes);
        Keys3 k;
        k.await = qtelist::Awaited(l, true, skip).await;
        k.quick = qtelist::QuickKeys(l, skip) & k.await;
        k.proper = qtelist::ProperKeys(l, skip) & k.await;
        return k;
    };
    {
        // 31a each action alone: the six quick-time actions their own keys, play's Fire, Jump and Interact none (as play's).
        int right = 0;
        std::string wrong;
        for (uint32_t a : {0x3Cu, 0x3Du, 0x3Eu, 0x3Fu, 0x40u, 0x41u, 0x07u, 0x0Cu, 0x23u}) {
            const Keys3 k = keysOf({{a, kBusLevel}});
            const unsigned want = qtelist::QuickTime(a) ? qtelist::KeyOf(a) : 0u;
            const bool ok = k.await == qtelist::KeyOf(a) && k.proper == want && k.quick == want;
            right += ok;
            if (!ok) {
                char one[64];
                std::snprintf(one, sizeof(one), " %02X: await %02X proper %02X quick %02X", a, k.await, k.proper, k.quick);
                wrong += one;
            }
        }
        Report("31a", "the key each goes down as: each quick-time action alone its own (as QuickKeys), play's Fire, Jump and Interact alone none (play's)",
               right == 9, "%d of 9%s", right, wrong.c_str());
    }
    {
        // 31b beside each other on one bus: the hanging's JumpClimb with play's Jump, InteractDrag with play's Interact: the quick-time action's key.
        const Keys3 hang = keysOf({{0x40, kBusLevel}, {0x0C, kBusLevel}}), drag = keysOf({{0x3C, kBusLevel}, {0x23, kBusLevel}}),
                    both = keysOf({{0x23, kBusLevel}, {0x0C, kBusLevel}, {0x3F, kBusLevel}});
        Report("31b", "a quick-time action with play's own on the same key: the quick-time action's (the hanging's Space, InteractDrag's E); play's Interact "
                      "and Jump beside FastMelee: F alone",
               hang.await == kSpace && hang.proper == kSpace && drag.await == kE && drag.proper == kE && both.await == (kE | kSpace | kF) && both.proper == kF,
               "hanging %02X of %02X, drag %02X of %02X, play's two with F %02X of %02X", hang.proper, hang.await, drag.proper, drag.await, both.proper, both.await);
    }
    {
        // 31c the review's case: a fight's six live on their bus (one never stopped, in free play after it) and a single's JumpClimb on the level's: the
        // single's Space awaited, QuickKeys none (no held grip's press beside a fight), ProperKeys its Space; the six with play's Interact: none (play's).
        std::vector<std::pair<uint32_t, uint32_t>> six;
        for (uint32_t a = 0x3C; a <= 0x41; ++a)
            six.push_back({a, kBusFight});
        std::vector<std::pair<uint32_t, uint32_t>> climb = six, interact = six;
        climb.push_back({0x40, kBusLevel});
        interact.push_back({0x23, kBusLevel});
        const Keys3 c = keysOf(climb), i = keysOf(interact), alone = keysOf(six);
        Report("31c", "a fight's six live beside a single's JumpClimb: Space awaited, QuickKeys none, its key the quick-time action's (ProperKeys Space); "
                      "beside play's Interact: E play's; the six alone: nothing awaited",
               c.await == kSpace && c.quick == 0 && c.proper == kSpace && i.await == kE && i.quick == 0 && i.proper == 0 && alone.await == 0 && alone.proper == 0,
               "climb: await %02X quick %02X proper %02X; Interact: await %02X quick %02X proper %02X; alone %02X", c.await, c.quick, c.proper, i.await, i.quick,
               i.proper, alone.await);
    }
    {
        // 31d a fight's bus by the fights' reader (qtefight's Skip: two of its triggers listed, a prompt's) and the single's climb beside it: the fight's
        // left out of both, the climb's Space proper; the list not read: none.
        qtelist::Skip skip;
        skip.Add(kBusPrompt);
        const Keys3 k = keysOf({{0x3C, kBusPrompt}, {0x3F, kBusPrompt}, {0x40, kBusLevel}}, &skip);
        const Keys3 unread = keysOf({{0x40, kBusLevel}}, nullptr, false);
        Report("31d", "a fight's bus by the fights' reader left out, the single's climb beside it its Space (QuickKeys none); the list not read: nothing",
               k.await == kSpace && k.quick == 0 && k.proper == kSpace && unread.await == 0 && unread.proper == 0,
               "await %02X quick %02X proper %02X; not read: %02X %02X", k.await, k.quick, k.proper, unread.await, unread.proper);
    }
    {
        // 31e the whole loop (27n's: The Great Destroyer's train, a climb's JumpClimb on the level's bus beside the fight, fresh squeezes between the
        // prompts): between the prompts the climb's Space awaited with QuickKeys none and ProperKeys Space every frame; in the prompts none (input.cpp
        // gives a fight's prompt all of version 2's); the fight won, never a wrong key.
        FightGame game({{2, false, 3000}, {1, false, 3000}});
        g_triggerBus = LevelBus(game.g.m, 0xE7380000u, "Levels/SP_Finale/TrainRide_SCM", 412);
        const uint32_t climb = Entity(game.g.m, 0x40);
        g_triggerBus = 0;
        FightMod mod;
        bool added = false;
        int gapFrames = 0, gapSpace = 0, promptProper = 0;
        unsigned gapQuick = 0, sent = 0;
        for (int i = 0; i < 900 && !game.Over(); ++i) {
            if (!added && game.phase != FightGame::Phase::kStart) {
                added = true;
                std::vector<uint32_t> enabled = {game.g.yaw, game.g.pitch, climb};
                for (uint32_t t : game.g.f.trigger)
                    enabled.push_back(t);
                Manager(game.g.m, enabled);
            }
            const bool inPrompt = game.phase == FightGame::Phase::kOpen || game.phase == FightGame::Phase::kClosing;
            const bool gap = game.phase == FightGame::Phase::kGap && added;
            const float left = !gap ? 1.0f : (i % 60) >= 30 && (i % 60) < 40 ? 1.0f : 0.0f;
            sent |= mod.Frame(game, left, 0.0f);
            if (gap && (mod.run.await & kSpace)) {
                ++gapFrames;
                gapSpace += mod.proper == kSpace;
                gapQuick |= mod.run.quick;
            }
            promptProper += inPrompt && mod.proper != 0;
        }
        const bool won = game.phase == FightGame::Phase::kWon;
        Report("31e", "the train's climb beside its fight (27n's loop): between prompts its Space awaited with QuickKeys none and the quick-time action's key "
                      "every frame; in the prompts none here (the prompt's own: all of version 2's); won, never a wrong key",
               won && game.wrongPresses == 0 && gapFrames > 0 && gapSpace == gapFrames && gapQuick == 0 && promptProper == 0 && (sent & kSpace),
               "%s, wrong %d; between prompts %d frames with Space awaited, %d of them proper, quick %02X; prompt frames with proper %d; sent %02X",
               won ? "won" : "NOT WON", game.wrongPresses, gapFrames, gapSpace, gapQuick, promptProper, sent);
    }
}

int main() {
    Play();
    Moment();
    NotFresh();
    MomentOver();
    Flick();
    Turn();
    FrontAndOff();
    Unread();
    ClockAndTime();
    Exact();
    Taps();
    Holds();
    Ends();
    Gate();
    Fallback();
    Seen();
    Reader();
    Train();
    Time2();
    // The fist fights (qtefight.h), and the singles of 18 again as the mod runs them now: through the fights' reader, on a level's own bus.
    Proofs();
    Attaches();
    Gates();
    Watches();
    Fights();
    Guards();
    FightTime();
    {
        qtefight::Watch via;
        g_viaFights = &via;
        g_tag = "F";
        Train();
        g_tag = "";
        g_viaFights = nullptr;
        g_triggerBus = 0;
        Report("18h", "... 18a-g through the fights' reader: every single's bus proven another blueprint's (its name), never a fight's prompt", g_viaFightsAttaches == 0 &&
                                                                                                                        g_viaFightsPrompts == 0,
               "fights attached %d, prompt frames %d", g_viaFightsAttaches, g_viaFightsPrompts);
    }
    TrainTestGrip();
    HeldGrips();
    ReviewFixes();
    LogTruth();
    Owners();
    Proper();
    std::printf("%d checks, %d failed, %d skipped\n", g_counted, g_failed, g_skipped);
    return g_failed;
}

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
// Usage: qtetest
// The exit code is the number of failed checks.

#include <windows.h>

#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <map>
#include <utility>
#include <vector>

#include "../src/qte.h"
#include "../src/qtelist.h"

namespace {

int g_failed = 0, g_counted = 0;

void Report(const char* id, const char* what, bool pass, const char* format, ...) {
    char numbers[512];
    va_list args;
    va_start(args, format);
    std::vsnprintf(numbers, sizeof(numbers), format, args);
    va_end(args);
    std::printf("%-4s %-5s %s | %s\n", id, pass ? "PASS" : "FAIL", what, numbers);
    ++g_counted;
    g_failed += !pass;
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
    // The game's input as the mod reads it: 0 never shows a key, 1 the keys of two frames before, 2 a key once down shown ever after, 3 what the
    // simulated game's last input tick took (external).
    int game = 1;
    unsigned external = 0;
    unsigned sent[2] = {};
    int began = 0, ended = 0, heldFrames = 0, settled = 0, settledSeen = 0;
    unsigned ever = 0, keys = 0;
    int edges[qte::kKeyCount] = {}, downFrames[qte::kKeyCount] = {};
    unsigned long long wentDown[qte::kKeyCount] = {}, wentUp[qte::kKeyCount] = {};
    unsigned long long shortestDown = ~0ull, longestDown = 0, shortestUp = ~0ull, firstEdge = 0, lastEdge = 0;
    unsigned long long longestSeenMs = 0;
    qte::Out last;
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
        in.seen = game == 1 ? sent[1] : game == 2 ? ever : game == 3 ? external : 0;
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

// A trigger entity and its data as the game has them (the entity 0x24 bytes, its data 0x40 on).
uint32_t Entity(Image& m, uint32_t action, bool sendEvents = true, int link = -1, uint32_t entityType = kTypes.entity, uint32_t dataType = kTypes.data) {
    const uint32_t at = m.next, data = at + 0x40;
    m.next += 0x100;
    for (uint32_t o = 0; o < 0x24; o += 4)
        m.W32(at + o, 0), m.W32(data + o, 0);
    m.W32(at, entityType);
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
        Report("17b", "adds not yet applied count, removes are out: one add, one remove, both, a remove then an add, a remove of one not there, an entry that is "
                      "neither, an add of one enabled, two adds",
               add.count == 3 && add.at[2].pending && qtelist::Awaited(add, false).await == kLmb && remove.count == 2 && qtelist::Awaited(remove, false).await == 0 &&
                   both.count == 2 && back.count == 3 && back.at[2].pending && notThere.count == 3 && !notThere.refused && neither.count == 2 &&
                   neither.refused == 1 && already.count == 3 && already.twice == 1 && !already.at[2].pending && two.count == 4 &&
                   qtelist::Awaited(two, false).await == kSpace,
               "%d %d %d %d %d %d %d %d", add.count, remove.count, both.count, back.count, notThere.count, neither.count, already.count, two.count);
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

// The mod's side of a frame, as input.cpp's QteStep and qtewatch.cpp's Update run it: the list read and steadied, the awaited keys, the grips.
unsigned ModFrame(Game& game, qtelist::Steady& steady, Run2& run, bool scripted, float left, float right) {
    const qtelist::List now = ReadList(game.m);
    const qtelist::Keys keys = qtelist::Awaited(qtelist::Settle(steady, now, run.now), scripted);
    run.await = keys.await;
    run.hold = keys.hold;
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

}  // namespace

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
    std::printf("%d checks, %d failed\n", g_counted, g_failed);
    return g_failed;
}

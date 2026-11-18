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
// Usage: qtetest
// The exit code is the number of failed checks.

#include <windows.h>

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <limits>

#include "../src/qte.h"

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
    std::printf("%d checks, %d failed\n", g_counted, g_failed);
    return g_failed;
}

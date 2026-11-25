// The view through a scope's frame-rate guard (src/scopegov.h, header only), without the game: seconds of frames handed to it as xr_runtime.cpp's
// pace counts them (frames, the second's own length between frames, the scope view's dispatches, the frames going to the headset, a lens looked
// through), and the lens's own tests for the view. Each check prints PASS or FAIL, then the totals; the exit code is the number failed.
//   G1  a steady 72 with the view on in every frame: never over, never held
//   G2  the owner's 11-19 Swordbreaker street with the Mk11's view on (58, 55, 57 frames): held back on the third second, 10 s; two alone never
//   G3  any refresh: 64 frames at 72 Hz, 72 at 80, 82 at 90, 61 at 120, a perfect 144 at 144, a full 60 Hz read a timer's fraction long: never over (the old
//       late rule tripped at 64, 82 and every 144 Hz second: the review's findings 1 and 2); 59 a second three times at any refresh: held
//   G4  a hold's time over with the scope still looked through: kept (said once), let go at the first second it's lowered; never while looked through
//   G5  held back again soon after a hold's END: 10, 20, 40, then 60 s at most, and a trip right after a 60 s hold stays 60 (the old calm from the
//       hold's start flapped 8, 16, 32, 60, 8: finding 3); after a calm two minutes 10 s again
//   G6  a sustained heavy scene, the scope up all along: the real picture 3 seconds of 400 (each switch at a raise, never mid-aim back and forth);
//       up 20 s and lowered 5 s, again and again: every trip a raise's, holds never shorter than the last within the calm
//   G7  under the floor with the view off, or on in under half the frames: never over (the lens isn't the cause); on in half: counted
//   G8  a heavy second between good ones: never held; a second not going to the headset, or of no known length, breaks a run
//   G9  a clock gone back during a hold: its time taken as over (let go once nothing's looked through), never held for good
//   G10 the lens's tests: on its line within 0.38 of the eye's (300 pixels off, a lens of 80: on, main's rule kept), sane to turn on (a lens 2860
//       pixels across, the gun past the face, 11-19: not; one 1200 across, an eyepiece pressed close: yes), in front of the eye; a number not a
//       number: no
//   G11 the rate and the floor: frames over the second's own length, rounded at the floor (59.4 under, 59.6 not); nothing known: not under
// Usage: scopegovtest

#include <cmath>
#include <cstdio>

#include "../src/scopegov.h"

namespace {

int g_failed = 0, g_counted = 0;

void Check(bool ok, const char* row, const char* what) {
    ++g_counted;
    if (!ok)
        ++g_failed;
    std::printf("%s %s %s\n", ok ? "PASS" : "FAIL", row, what);
}

scopegov::Second S(unsigned long long nowMs, int frames, int drawn, bool looking = true, double ms = 1000.0, bool paced = true) {
    scopegov::Second s;
    s.nowMs = nowMs;
    s.ms = ms;
    s.frames = frames;
    s.drawn = drawn;
    s.looking = looking;
    s.paced = paced;
    return s;
}

// Three seconds under the floor with the view on from `t` (ms), the last one returned.
scopegov::Out Trip(scopegov::State& s, unsigned long long& t, int frames = 55) {
    scopegov::Out o;
    for (int i = 0; i < scopegov::kOverSeconds; ++i, t += 1000)
        o = scopegov::Step(s, S(t, frames, frames));
    return o;
}

}  // namespace

int main() {
    using namespace scopegov;
    {
        State s;
        bool held = false, over = false;
        for (int i = 1; i <= 120; ++i) {
            const Out o = Step(s, S(i * 1000ull, 72, 72));
            held = held || o.heldNow || s.held;
            over = over || o.over;
        }
        Check(!held && !over, "G1", "a steady 72 with the view on: never over, never held");
    }
    {
        State s;
        const Out a = Step(s, S(1000, 58, 58)), b = Step(s, S(2000, 55, 55)), c = Step(s, S(3000, 57, 57));
        Check(a.over && b.over && !a.heldNow && !b.heldNow && c.heldNow && c.holdMs == kHoldFirst && s.held && kHoldFirst == 10000, "G2",
              "three seconds under 60 with the view on (58, 55, 57): held back on the third, 10 s");
        State q;
        Step(q, S(1000, 58, 58));
        Step(q, S(2000, 55, 55));
        const Out d = Step(q, S(3000, 66, 66));
        Step(q, S(4000, 55, 55));
        const Out e = Step(q, S(5000, 55, 55));
        Check(!d.over && !d.heldNow && !e.heldNow && !q.held, "G2", "two seconds under 60, a good one, two more: never held");
    }
    {
        // (Frames a second at each refresh as xr_runtime.cpp now hands them: the frames over the second's own length.)
        struct Case {
            const char* name;
            int frames;
            double ms;
        } const fine[] = {{"64 at 72 Hz", 64, 1000.0},    {"60 at 72 Hz", 60, 999.6},   {"72 at 80 Hz", 72, 1000.0},
                          {"82 at 90 Hz", 82, 1000.0},    {"61 at 120 Hz", 61, 1000.0}, {"144 at 144 Hz", 144, 1000.0},
                          {"a full 60 Hz, 0.4 ms long", 60, 1000.4},                    {"a full 60 Hz over 1.016 s", 61, 1016.7}};
        bool ok = true;
        for (const Case& c : fine) {
            State s;
            for (int i = 1; i <= 30; ++i) {
                const Out o = Step(s, S(i * 1000ull, c.frames, c.frames, true, c.ms));
                if (o.over || o.heldNow || s.held) {
                    ok = false;
                    std::printf("     over at %s\n", c.name);
                    break;
                }
            }
        }
        Check(ok, "G3", "64 at 72 Hz, 72 at 80, 82 at 90, 61 at 120, 144 at 144, a full 60 Hz a timer's fraction long: never over");
        State s;
        unsigned long long t = 1000;
        const bool held = Trip(s, t, 59).heldNow;
        State q;
        Step(q, S(1000, 59, 59, true, 1003.0));
        Step(q, S(2000, 59, 59, true, 1003.0));
        const Out r = Step(q, S(3000, 59, 59, true, 1003.0));
        Check(held && r.heldNow, "G3", "59 frames a second three seconds running: held, whatever the refresh");
    }
    {
        State s;
        unsigned long long t = 1000;
        Trip(s, t);
        const unsigned long long until = s.until;
        bool keptWhileLooking = true;
        int waitingSaid = 0;
        for (t = until; t < until + 30000; t += 1000) {
            const Out o = Step(s, S(t, 70, 0, true));
            keptWhileLooking = keptWhileLooking && s.held && !o.released;
            waitingSaid += o.waiting ? 1 : 0;
        }
        const Out r = Step(s, S(t, 70, 0, false));
        Check(keptWhileLooking && waitingSaid == 1 && r.released && !s.held && s.endedAt == t, "G4",
              "its time over with the scope looked through: kept 30 s more (said once), let go at the first second it's lowered");
        State q;
        unsigned long long u = 1000;
        Trip(q, u);
        bool early = false;
        for (; u < q.until; u += 1000)
            early = early || Step(q, S(u, 70, 0, false)).released;
        Check(!early && q.held, "G4", "the scope lowered before its time is over: still held till then");
    }
    {
        State s;
        unsigned long long t = 1000, holds[6] = {};
        for (int h = 0; h < 5; ++h) {
            holds[h] = Trip(s, t).holdMs;
            t = s.until;
            Step(s, S(t, 70, 0, false));  // (its time over, the scope down: let go)
            t += 1000;
        }
        Check(holds[0] == 10000 && holds[1] == 20000 && holds[2] == 40000 && holds[3] == 60000 && holds[4] == 60000, "G5",
              "held again soon after a hold's end: 10, 20, 40, 60, then 60 again (never back to 10 straight after a 60 s hold)");
        t += kCalm;
        holds[5] = Trip(s, t).holdMs;
        Check(holds[5] == kHoldFirst && kCalm > kHoldMost, "G5", "after a calm two minutes from the last hold's end: 10 s again");
    }
    {
        // The review's sustained heavy scene, the scope up all along: 55 frames a second with the view on, 70 with it held back.
        State s;
        int shown = 0, switches = 0;
        bool was = true;
        for (int i = 1; i <= 400; ++i) {
            const bool on = !s.held;
            Step(s, S(i * 1000ull, on ? 55 : 70, on ? 55 : 0, true));
            shown += on ? 1 : 0;
            switches += on != was ? 1 : 0;
            was = on;
        }
        Check(shown == kOverSeconds && switches == 1, "G6", "a heavy scene with the scope up 400 s: its own picture the first 3 s, then held back the rest, "
                                                            "one switch");
        // Up 20 s and down 5 s, again and again (the view 55 when on, 70 when held back or down).
        State q;
        int trips = 0, shorter = 0, flips = 0;
        unsigned long long last = 0, lastEnd = 0;
        for (int i = 1; i <= 600; ++i) {
            const bool up = (i % 25) < 20;
            const bool on = up && !q.held;
            const Out o = Step(q, S(i * 1000ull, on ? 55 : 70, on ? 55 : 0, up));
            if (o.heldNow) {
                ++trips;
                shorter += last && i * 1000ull - lastEnd < kCalm && o.holdMs < last ? 1 : 0;
                last = o.holdMs;
            }
            if (o.released) {
                lastEnd = i * 1000ull;
                flips += up ? 1 : 0;  // (let go while looked through: never)
            }
        }
        Check(trips >= 2 && shorter == 0 && flips == 0, "G6", "the scope up 20 s and lowered 5 s for 10 minutes: no hold shorter than the last within the "
                                                              "calm, none let go while looked through");
    }
    {
        State s;
        bool held = false;
        for (int i = 1; i <= 10; ++i)
            held = held || Step(s, S(i * 1000ull, 50, 0)).heldNow;
        for (int i = 11; i <= 20; ++i)
            held = held || Step(s, S(i * 1000ull, 50, 24)).heldNow;
        Check(!held, "G7", "under the floor with the view off, or on in under half the frames: never held");
        unsigned long long t = 21000;
        Out o;
        for (int i = 0; i < kOverSeconds; ++i, t += 1000)
            o = Step(s, S(t, 50, 25));
        Check(o.over && o.heldNow, "G7", "on in half the frames: counted");
    }
    {
        State s;
        bool held = false;
        for (int i = 1; i <= 30; ++i)
            held = held || Step(s, S(i * 1000ull, i % 3 ? 55 : 72, 60)).heldNow;
        Check(!held, "G8", "two heavy seconds between good ones, again and again: never held");
        State q;
        Step(q, S(1000, 55, 55));
        Step(q, S(2000, 55, 55));
        Step(q, S(3000, 55, 55, true, 1000.0, false));
        Step(q, S(4000, 55, 55));
        const Out c = Step(q, S(5000, 55, 55));
        State w;
        Step(w, S(1000, 55, 55));
        Step(w, S(2000, 55, 55));
        Step(w, S(3000, 55, 55, true, 0.0));
        Step(w, S(4000, 55, 55));
        const Out d = Step(w, S(5000, 55, 55));
        Check(!c.heldNow && !q.held && !d.heldNow && !w.held, "G8", "a second not going to the headset, or of no known length, breaks the run");
    }
    {
        State s;
        unsigned long long t = 100000;
        Trip(s, t);
        const Out kept = Step(s, S(5000, 72, 0, true));
        const Out r = Step(s, S(6000, 72, 0, false));
        Check(kept.waiting && s.endedAt == 6000 && r.released && !s.held, "G9", "a clock gone back during a hold: its time over, let go once lowered");
    }
    {
        const float nan = std::nanf("");
        Check(LensOnLine(300.0f, 80.0f, 906.0f) && !LensOnLine(350.0f, 0.0f, 906.0f) && LensOnLine(340.0f, 0.0f, 0.0f), "G10",
              "on its line within 0.38 of the eye's and its radius: 300 off a lens of 80 (424) on, as main had it; 350 off a point (344) not");
        Check(!LensSane(1430.0f, 906.0f, -0.10f) && !LensSane(18619.5f, 906.0f, -0.10f), "G10", "a lens 2860 pixels across or more: not one to turn on for");
        Check(LensSane(160.0f, 906.0f, -0.10f) && LensSane(600.0f, 0.0f, -0.05f) && LensSane(1200.0f, 906.0f, -0.014f), "G10",
              "a real lens in front of the eye: one (an eyepiece pressed 1.4 cm from the eye, 2400 pixels across, too)");
        Check(!LensSane(160.0f, 906.0f, 0.0f) && !LensSane(160.0f, 906.0f, 0.05f) && InFront(-0.01f) && !InFront(0.0f) && !InFront(0.05f), "G10",
              "a lens at or behind the eye: not one, not in front");
        Check(!LensSane(nan, 906.0f, -0.1f) && !LensSane(160.0f, 906.0f, nan) && !InFront(nan) && !LensOnLine(nan, 80.0f, 906.0f) &&
                  !LensOnLine(100.0f, nan, 906.0f),
              "G10", "a number not a number: not one, never on its line");
    }
    {
        Check(UnderFloor(Rate(59, 1000.0)) && UnderFloor(59.4) && !UnderFloor(59.6) && !UnderFloor(Rate(60, 1000.4)) && !UnderFloor(Rate(0, 1000.0)) &&
                  !UnderFloor(Rate(55, 0.0)) && std::fabs(Rate(144, 1000.0) - 144.0) < 1e-9,
              "G11", "frames over the second's own length, rounded at the floor: 59.4 under, 59.6 and a full 60 Hz not, nothing known not");
    }
    std::printf("%d checks, %d failed\n", g_counted, g_failed);
    return g_failed;
}

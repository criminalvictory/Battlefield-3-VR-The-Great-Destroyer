// The view through a scope's frame-rate guard (src/scopegov.h, header only), without the game: seconds of frames handed to it as xr_runtime.cpp's
// pace counts them (frames, late ones, the scope view's dispatches, the frames going to the headset), and the lens's own tests for turning the view on.
// Each check prints PASS or FAIL, then the totals; the exit code is the number failed.
//   G1  a steady 72 with the view on in every frame: never over, never held
//   G2  the owner's 11-19 Swordbreaker seconds with the Mk11's view on (58 and 55 frames): held back on the second, 8 s; nothing more till then;
//       tried again at 8 s
//   G3  held back again within a minute of the last hold: 16 s, then 32, then 60 at most; after a calm minute 8 s again
//   G4  under the floor with the view off, or on in under half the frames: never over (the lens isn't the cause)
//   G5  72 frames but 8 of them a refresh late, twice: held; 7 late: never
//   G6  one heavy second between good ones: never held; a second not going to the headset (nobody in it, a load) breaks a run
//   G7  a clock gone back during a hold: let go, never held for good
//   G8  the lens's tests: a lens 2860 pixels across (the gun past the face, 11-19) or behind the eye isn't one to look through; 300 pixels off its
//       line a lens of 80 doesn't turn the view on (0.2 of 906 and its radius: 261) but keeps it on (0.38: 424); a number not a number: no
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

scopegov::Second S(unsigned long long ms, int frames, int late, int drawn, bool paced = true) {
    scopegov::Second s;
    s.nowMs = ms;
    s.frames = frames;
    s.late = late;
    s.drawn = drawn;
    s.paced = paced;
    return s;
}

}  // namespace

int main() {
    using namespace scopegov;
    {
        State s;
        bool held = false;
        for (int i = 1; i <= 120; ++i)
            held = held || Step(s, S(i * 1000ull, 72, 0, 72)).heldNow || s.held;
        Check(!held, "G1", "a steady 72 with the view on: never held");
    }
    {
        State s;
        const Out a = Step(s, S(1000, 58, 6, 58));
        const Out b = Step(s, S(2000, 55, 9, 55));
        Check(a.over && !a.heldNow && b.heldNow && b.holdMs == kHoldFirst && s.held, "G2", "two seconds under 60 with the view on: held back, 8 s");
        bool quiet = true;
        for (unsigned long long t = 3000; t < 10000; t += 1000) {
            const Out o = Step(s, S(t, 72, 0, 0));
            quiet = quiet && !o.released && !o.heldNow && s.held;
        }
        const Out r = Step(s, S(10000, 72, 0, 0));
        Check(quiet && r.released && !s.held, "G2", "nothing more while held; tried again 8 s on");
    }
    {
        State s;
        unsigned long long t = 1000;
        unsigned long long holds[5] = {};
        for (int h = 0; h < 4; ++h) {
            Step(s, S(t, 55, 9, 55));
            t += 1000;
            holds[h] = Step(s, S(t, 55, 9, 55)).holdMs;
            t = s.until;
            Step(s, S(t, 72, 0, 0));  // (released)
            t += 1000;
        }
        Check(holds[0] == 8000 && holds[1] == 16000 && holds[2] == 32000 && holds[3] == 60000, "G3", "held again soon after: 8, 16, 32, then 60 s at most");
        t += kCalm + 1000;
        Step(s, S(t, 55, 9, 55));
        t += 1000;
        holds[4] = Step(s, S(t, 55, 9, 55)).holdMs;
        Check(holds[4] == kHoldFirst, "G3", "after a calm minute: 8 s again");
    }
    {
        State s;
        bool held = false;
        for (int i = 1; i <= 10; ++i)
            held = held || Step(s, S(i * 1000ull, 50, 12, 0)).heldNow;
        for (int i = 11; i <= 20; ++i)
            held = held || Step(s, S(i * 1000ull, 50, 12, 24)).heldNow;
        Check(!held, "G4", "under the floor with the view off, or on in under half the frames: never held");
        const Out a = Step(s, S(21000, 50, 12, 25));
        const Out b = Step(s, S(22000, 50, 12, 25));
        Check(a.over && b.heldNow, "G4", "on in half the frames: counted");
    }
    {
        State s;
        const Out a = Step(s, S(1000, 72, 8, 72)), b = Step(s, S(2000, 72, 8, 72));
        State q;
        bool held = false;
        for (int i = 1; i <= 10; ++i)
            held = held || Step(q, S(i * 1000ull, 72, 7, 72)).heldNow;
        Check(a.over && b.heldNow && !held, "G5", "8 frames a second a refresh late twice: held; 7: never");
    }
    {
        State s;
        bool held = false;
        for (int i = 1; i <= 30; ++i)
            held = held || Step(s, S(i * 1000ull, i % 2 ? 55 : 72, 0, 60)).heldNow;
        Check(!held, "G6", "heavy seconds between good ones: never held");
        State q;
        Step(q, S(1000, 55, 9, 55));
        Step(q, S(2000, 55, 9, 55, false));
        const Out c = Step(q, S(3000, 55, 9, 55));
        Check(!c.heldNow && !q.held, "G6", "a second not going to the headset breaks the run");
    }
    {
        State s;
        Step(s, S(100000, 55, 9, 55));
        Step(s, S(101000, 55, 9, 55));
        const Out r = Step(s, S(5000, 72, 0, 0));
        Check(r.released && !s.held, "G7", "a clock gone back during a hold: let go");
    }
    {
        const float nan = std::nanf("");
        Check(!LensSane(1430.0f, 906.0f, -0.10f) && !LensSane(18619.5f, 906.0f, -0.10f), "G8", "a lens 2860 pixels across or more: not one to look through");
        Check(LensSane(160.0f, 906.0f, -0.10f) && LensSane(600.0f, 0.0f, -0.05f) && LensSane(1200.0f, 906.0f, -0.01f), "G8",
              "a real lens in front of the eye: one (an eyepiece pressed 1 cm from the eye, 2400 pixels across, too)");
        Check(!LensSane(160.0f, 906.0f, 0.0f) && !LensSane(160.0f, 906.0f, 0.05f), "G8", "a lens at or behind the eye: not one");
        Check(!LensSane(nan, 906.0f, -0.1f) && !LensSane(160.0f, 906.0f, nan), "G8", "a number not a number: not one");
        Check(!LensOnLine(300.0f, 80.0f, 906.0f, false) && LensOnLine(300.0f, 80.0f, 906.0f, true) && LensOnLine(200.0f, 80.0f, 906.0f, false),
              "G8", "300 pixels off: doesn't turn it on (261), keeps it on (424); 200 off turns it on");
        Check(!LensOnLine(nan, 80.0f, 906.0f, false) && !LensOnLine(nan, 80.0f, 906.0f, true), "G8", "an offset not a number: never on");
    }
    std::printf("%d checks, %d failed\n", g_counted, g_failed);
    return g_failed;
}

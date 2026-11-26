// The view through a scope's frame-rate guard (src/scopegov.h, header only), without the game: seconds of frames handed to it as xr_runtime.cpp's
// pace counts them (frames, the second's own length between frames, the scope view's dispatches, the frames going to the headset, a lens looked
// through), the lens's own tests for the view, and the view's own switch a dispatch at a time (StepView, as scopeview.cpp's Wanted runs it). Each check
// prints PASS or FAIL, then the totals; the exit code is the number failed. G12 and G13 read src\scopeview.cpp and src\skin.cpp: run it from the project's
// folder.
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
//       pixels across, the gun past the face, 11-19: not; the Mk11's 1.9 cm eyepiece 1.43 cm from the eye, 2400 across: yes; at 1.40 cm, 2460 across,
//       past the cap: not, nearer than 1.41 cm none is), in front of the eye; a number not a number: no
//   G11 the rate and the floor: frames over the second's own length, rounded at the floor (59.4 under, 59.6 not); nothing known: not under
//   G12 the view's own switch (StepView): the rules Wanted had kept (on at the second dispatch of a sane lens; never for one under 80 pixels,
//       unmagnified, off its line, noted 100 ms ago, behind the eye, or past the cap at a fresh raise; once on, an eyepiece pressed to 1.1 cm keeps
//       it; off past 120 ms with no lens noted, or 37 dispatches running behind the eye); the review of d32a39b's finding 10: on with the eye 1.6 cm
//       from the Mk11's eyepiece, a 150 ms gap, the lens back at 1.2, 1.25 or 1.3 cm: on again within two dispatches (it never was); the eye a moment
//       through the glass, back at 1.2 cm: on again; the cap kept for a raise a second or more after it went off; scopeview.cpp's Wanted runs StepView
//       (word for word)
//   G13 weapon.txt's scopegov line taken away (the review of d32a39b, finding 5; skin.cpp's reader and scopeview.cpp's SetGovernor, word for word): with
//       no line at a read, or no file, the guard only watches again (its hold let go and said; silent when nothing changes)
// Usage: scopegovtest (from the project's folder)

#include <cmath>
#include <cstdio>
#include <string>

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

// ---- G12: the view's own switch, a dispatch at a time (72 a second: 14 ms apart) ----

constexpr float kFocal = 906.0f;              // the eye's picture's pixels per unit of slope (100 degrees on 2160)
constexpr float kMk11Eyepiece = 0.019f;       // kHeldSights' rifle scope: its eyepiece 1.9 cm in radius, 6.84 times
constexpr unsigned long long kDispatch = 14;  // ms

// The Mk11's eyepiece with the eye d metres behind it on its line, as NoteLens notes it (ageMs since).
scopegov::Seen Mk11At(float d, unsigned long long ageMs = 0) {
    scopegov::Seen s;
    s.ageMs = ageMs;
    s.power = 6.84f;
    s.radius = kFocal * kMk11Eyepiece / d;
    s.off = 0.0f;
    s.focal = kFocal;
    s.back = -d;
    return s;
}

// `n` dispatches from t (ms, moved on) with the lens noted as `seen` at each: the first dispatch that turned it (kNo: none), and at which one (1 on).
struct Turn {
    scopegov::Turned what = scopegov::Turned::kNo;
    int at = 0;
};
Turn Run(scopegov::View& v, unsigned long long& t, int n, const scopegov::Seen& seen) {
    Turn first;
    for (int i = 1; i <= n; ++i, t += kDispatch) {
        const scopegov::Turned got = scopegov::StepView(v, seen, t);
        if (got != scopegov::Turned::kNo && first.what == scopegov::Turned::kNo)
            first = {got, i};
    }
    return first;
}

// A gap: `ms` with no lens noted after `last` (its age growing a dispatch at a time, t moved on); the first turn, and at which dispatch.
Turn Gap(scopegov::View& v, unsigned long long& t, unsigned long long ms, const scopegov::Seen& last) {
    Turn first;
    int i = 0;
    for (unsigned long long age = kDispatch; age <= ms; age += kDispatch, t += kDispatch) {
        ++i;
        scopegov::Seen s = last;
        s.ageMs = age;
        const scopegov::Turned got = scopegov::StepView(v, s, t);
        if (got != scopegov::Turned::kNo && first.what == scopegov::Turned::kNo)
            first = {got, i};
    }
    return first;
}

bool IsOn(scopegov::Turned t) {
    return t == scopegov::Turned::kOn || t == scopegov::Turned::kOnPastCap;
}

std::string ReadText(const char* path) {
    std::string text;
    FILE* f = nullptr;
    if (fopen_s(&f, path, "rb") != 0 || !f)
        return text;
    char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
        text.append(buf, n);
    fclose(f);
    std::string out;
    for (char c : text)
        if (c != '\r')
            out += c;
    return out;
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
        Check(LensSane(160.0f, 906.0f, -0.10f) && LensSane(600.0f, 0.0f, -0.05f) && LensSane(1200.0f, 906.0f, -0.0143f) &&
                  LensSane(kFocal * kMk11Eyepiece / 0.0141f, kFocal, -0.0141f) && !LensSane(kFocal * kMk11Eyepiece / 0.014f, kFocal, -0.014f),
              "G10", "a real lens in front of the eye: one (the Mk11's 1.9 cm eyepiece 1.43 cm from the eye, 2400 pixels across, too; at 1.41 cm, 2442 "
                     "across, still; at 1.40 cm, 2459, past the cap: not one to turn on for at a fresh raise)");
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
    // ---- G12: the view's own switch, a dispatch at a time (StepView, as scopeview.cpp's Wanted runs it) ----
    {
        // The rules Wanted had, kept: a fresh raise with the eye 1.6 cm from the Mk11's eyepiece (2152 pixels across, under the cap).
        View fresh;
        unsigned long long t = 10000;
        const Turned first = StepView(fresh, Mk11At(0.016f), t);
        t += kDispatch;
        const Turned second = StepView(fresh, Mk11At(0.016f), t);
        Check(first == Turned::kNo && second == Turned::kOn && fresh.on, "G12",
              "a fresh raise, the eye 1.6 cm from the Mk11's eyepiece (2152 pixels across): nothing at the first dispatch, on at the second");
        Seen tiny = Mk11At(0.016f), plain = Mk11At(0.016f), offLine = Mk11At(0.016f), stale = Mk11At(0.016f, kFresh), behind = Mk11At(0.016f);
        tiny.radius = 79.0f;
        plain.power = 1.5f;
        offLine.radius = 100.0f, offLine.off = kSlope * kFocal + 101.0f;
        behind.back = 0.001f;
        const Seen pastCap = Mk11At(0.0125f);  // (1377 pixels in radius: past the cap's 1223)
        bool never = true;
        for (const Seen& s : {tiny, plain, offLine, stale, behind, pastCap}) {
            View v;
            unsigned long long u = 10000;
            const Turn got = Run(v, u, 100, s);
            never = never && got.what == Turned::kNo && !v.on;
        }
        Check(never, "G12", "never on, 100 dispatches each: a lens under 80 pixels, unmagnified (1.5), off its line, noted 100 ms ago, behind the eye, or "
                            "past the cap at a fresh raise (the eye 1.25 cm from the Mk11's eyepiece, the view never on before)");
        View kept;
        unsigned long long k = 10000;
        const Turn on = Run(kept, k, 2, Mk11At(0.016f));
        const Turn pressed = Run(kept, k, 500, Mk11At(0.011f));
        Check(IsOn(on.what) && pressed.what == Turned::kNo && kept.on, "G12",
              "once on, the eye pressed to 1.1 cm from the eyepiece (3130 pixels across, past the cap) 500 dispatches: kept on");
        View gone;
        unsigned long long g = 10000;
        Run(gone, g, 2, Mk11At(0.016f));
        const Turn gap = Gap(gone, g, 150, Mk11At(0.016f));
        View through;
        unsigned long long h = 10000;
        Run(through, h, 2, Mk11At(0.016f));
        Seen past = Mk11At(0.016f);
        past.back = 0.005f;
        const Turn behindRun = Run(through, h, 40, past);
        Check(gap.what == Turned::kOff && gap.at == 9 && behindRun.what == Turned::kOff && behindRun.at == kSmallRun + 1, "G12",
              "off: no lens noted past 120 ms (at the dispatch 126 ms on, not at 112), or its lens behind the eye 37 dispatches running");
        // The review of d32a39b, finding 10: on, a 150 ms gap (a long frame), the lens back with the eye nearer than the cap.
        bool back = true;
        int worst = 0;
        for (const float d : {0.012f, 0.0125f, 0.013f}) {
            View v;
            unsigned long long u = 10000;
            Run(v, u, 2, Mk11At(0.016f));
            const Turn off = Gap(v, u, 150, Mk11At(0.016f));
            const Turn again = Run(v, u, 2, Mk11At(d));
            back = back && off.what == Turned::kOff && again.what == Turned::kOnPastCap && v.on;
            worst = again.at > worst ? again.at : worst;
        }
        char what[400];
        std::snprintf(what, sizeof(what), "on with the eye 1.6 cm from the Mk11's eyepiece, a 150 ms gap (off), the lens back at 1.2, 1.25 and 1.3 cm (2869, "
                                          "2754 and 2648 pixels across, past the cap): on again each time, within %d dispatches", worst);
        Check(back && worst == 2, "G12", what);
        View pushed;
        unsigned long long p = 10000;
        Run(pushed, p, 2, Mk11At(0.016f));
        const Turn pushedOff = Run(pushed, p, kSmallRun + 1, past);
        const Turn pushedBack = Run(pushed, p, 2, Mk11At(0.012f));
        Check(pushedOff.what == Turned::kOff && pushedBack.what == Turned::kOnPastCap && pushedBack.at == 2, "G12",
              "the eye a moment through the glass (behind it 37 dispatches: off), back in front at 1.2 cm: on again at the second dispatch");
        // The cap kept for a fresh raise: the lens back a second or more after the view went off.
        View later;
        unsigned long long l = 10000;
        Run(later, l, 2, Mk11At(0.016f));
        Gap(later, l, 150, Mk11At(0.016f));
        const unsigned long long offAt = later.offAt;
        while (l < offAt + kBackMs) {
            Seen lowered = Mk11At(0.016f, l - offAt + 150);
            StepView(later, lowered, l);
            l += kDispatch;
        }
        const Turn lateClose = Run(later, l, 100, Mk11At(0.0125f));
        const Turn lateSane = Run(later, l, 2, Mk11At(0.016f));
        Check(offAt && lateClose.what == Turned::kNo && lateSane.what == Turned::kOn, "G12",
              "the cap kept for a raise a second or more after it went off: the eye at 1.25 cm 100 dispatches, never on; at 1.6 cm, on at the second");
        const std::string view = ReadText("src\\scopeview.cpp");
        const bool runs =
            !view.empty() && view.find("    const scopegov::Turned turned = scopegov::StepView(s_view, seen, now);") != std::string::npos &&
            view.find("    seen.ageMs = now - line.when;\n    seen.power = line.power, seen.radius = line.radius, seen.off = line.off, seen.focal = line.focal, "
                      "seen.back = line.at[2];") != std::string::npos &&
            view.find("    if (turned == scopegov::Turned::kOn || turned == scopegov::Turned::kOnPastCap) {\n        g_on.store(true, std::memory_order_relaxed);") !=
                std::string::npos &&
            view.find("    } else if (turned == scopegov::Turned::kOff) {\n        g_on.store(false, std::memory_order_relaxed);") != std::string::npos &&
            view.find("    return s_view.on;\n}") != std::string::npos && view.find("g_activeFrames") == std::string::npos &&
            view.find("LensSane(") == std::string::npos && view.find("constexpr ULONGLONG kLensGone = scopegov::kLensGone;") != std::string::npos;
        Check(runs, "G12", view.empty() ? "scopeview.cpp's Wanted (src\\scopeview.cpp not read: run from the project's folder)"
                                        : "scopeview.cpp's Wanted runs StepView on the lens as noted, its on and off said to the other threads (g_on), no rule of "
                                          "its own beside it (word for word)");
    }
    // ---- G13: weapon.txt's scopegov line taken away (skin.cpp's reader, scopeview.cpp's SetGovernor) ----
    {
        const std::string skin = ReadText("src\\skin.cpp"), view = ReadText("src\\scopeview.cpp");
        if (skin.empty() || view.empty()) {
            Check(false, "G13", "skin.cpp's reader and scopeview.cpp's SetGovernor (src\\skin.cpp or src\\scopeview.cpp not read: run from the project's folder)");
        } else {
            size_t calls = 0;
            for (size_t at = skin.find("scopeview::SetGovernor("); at != std::string::npos; at = skin.find("scopeview::SetGovernor(", at + 1))
                ++calls;
            const size_t fights = skin.find("    if (!sawQteFight)\n        input::SetQteFight(true);");
            const size_t after = skin.find("    if (!sawScopeGov)\n        scopeview::SetGovernor(false);");
            const bool noFile = skin.find("    if (!GetFileAttributesExW(path, GetFileExInfoStandard, &info)) {\n        scopeview::SetGovernor(false);") != std::string::npos;
            const bool line = skin.find("    bool sawScopeGov = false;") != std::string::npos &&
                              skin.find("            scopeview::SetGovernor(on != 0);\n            sawScopeGov = true;") != std::string::npos;
            const bool afterLoop = after != std::string::npos && fights != std::string::npos && after > fights;
            const bool setGov = view.find("void SetGovernor(bool on) {\n    if (g_govAct.exchange(on, std::memory_order_relaxed) == on)\n        return;\n    const bool "
                                          "wasHeld = !on && g_govHeld.exchange(false, std::memory_order_relaxed);") != std::string::npos;
            char what[400];
            std::snprintf(what, sizeof(what), "the line read sets the guard and is marked seen %d; no line at a read: only watching again, after the loop %d; no "
                                              "file: the same %d; %zu calls in all (those three); SetGovernor silent when nothing changes, its hold let go when "
                                              "it stops acting %d",
                          line, afterLoop, noFile, calls, setGov);
            Check(line && afterLoop && noFile && calls == 3 && setGov, "G13", what);
        }
    }
    std::printf("%d checks, %d failed\n", g_counted, g_failed);
    return g_failed;
}

// input.cpp's and ammo.cpp's rules that need no game (src/inputrules.h) and the controllers' tracking record with the reach test's lines
// (src/tracking.h), header only, run here as they are, without the game.
//   1  a lone release (StepRelease): owed at a new soldier, as bf3 comes back to the front and after a release of the mod's own Windows refused, sent
//      once bf3 is in front with the mod's own button up and no other button held, each button on its own, and again 0.25, 0.5, 1, 2 and 4 s after;
//      never while bf3 isn't in front, never one the mod holds (a trigger, a film's press, the grenade launcher's shot, the zoom held); those due while
//      it waited made one, afresh as bf3 comes back; the first step's front no coming back; the clock going back; Operation Swordbreaker's roof replayed
//      (a death with the trigger held, the headset off, the new soldier 17 s later: the release at the new soldier's frame, and again to 4 s after)
//   2  the fire button's sends said (StepSay): two lines a second with bf3 in front, four with it not, every send said or counted into the next line said,
//      a long fight's taps at most six lines a second, a clock going back
//   3  a film round's real rounds (RoundFireRefused): none without the word, with one hand, or the head under 45 degrees down (or not a number); the
//      self-test's round (test 1: no word, the left hand alone, the head 55 down) refused for the word
//   4  X held waiting for the game's reload (StepPress): Rock and a Hard Place's count up 1.39 s after the hold (loaded then, never before), Operation
//      Swordbreaker's six refused holds (never a magazine drawn: the old rule drew six), a rise of three rounds, another gun, the 8 s, a clock going back;
//      a magazine pushed home by hand in the wait (StillAsked, with pickrules' SeeCount): the game's reload landing on it undone; the rise's own steps
//      (DrawAtRise: the M416's bolt, a held gun's chamber)
//   5  the tracking record (Step): seen, guessed, no place and each change's line and numbers; the headset off and on no loss; the first watch and
//      "again"; the speed by the sensors or by two places; a turn not valid or a place not finite; the caps (six in ten seconds, the rest in line 7
//      once a state has held two seconds, or as the watch ends)
//   6  the grip (Squeeze): armed under 0.4, a squeeze past 0.6 once; a seen hand's said only within 0.55 m of the neck point; four a second, the rest
//      counted; an unread grip held is no squeeze; a grip not a number
//   7  the holsters' places and their words: each place's middle at it, no point of a 1 cm grid inside two places, "by", "in front", "low",
//      "elsewhere", the body's axes as the pouch's (yaw 0 and a quarter turn), a place not finite
//   8  the time: each rule well under a microsecond a call
// Usage: inputrulestest
// The exit code is the number of failed checks.

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>

#include "../src/inputrules.h"
#include "../src/pickrules.h"
#include "../src/tracking.h"

namespace {

int g_failed = 0, g_counted = 0;

void Report(const char* id, const char* what, bool pass, const char* format, ...) {
    char numbers[1536];
    va_list args;
    va_start(args, format);
    std::vsnprintf(numbers, sizeof(numbers), format, args);
    va_end(args);
    std::printf("%-4s %-5s %s | %s\n", id, pass ? "PASS" : "FAIL", what, numbers);
    ++g_counted;
    g_failed += !pass;
}

constexpr float kPi = 3.14159265f;
constexpr float kNaN = std::numeric_limits<float>::quiet_NaN();

// ---- 1: the lone release ----
constexpr unsigned long long kStepMs = 14;  // (a frame at 72 Hz, about)
struct Frames {
    inputrules::ReleaseState s;
    unsigned long long now = 1000000;  // ms
    int sent[2] = {}, firsts[2] = {}, lasts[2] = {}, sentAway = 0, sentHeld = 0, sentOther = 0;
    unsigned long long sentAt[2][64] = {};  // each send's time (the first 64)
    // One step at ms: counts each release sent (the firsts and the lasts of their sends after), and any sent while bf3 wasn't in front, while the mod held
    // it or another button was held (all three wrong).
    inputrules::ReleaseOut At(unsigned long long ms, unsigned soldier, bool front, bool holdLeft = false, bool holdRight = false, bool other = false,
                              bool refusedLeft = false) {
        now = ms;
        inputrules::ReleaseIn in;
        in.nowMs = ms, in.soldier = soldier, in.front = front, in.modHolds[0] = holdLeft, in.modHolds[1] = holdRight, in.otherHeld = other;
        in.refused[0] = refusedLeft;
        const inputrules::ReleaseOut out = inputrules::StepRelease(s, in);
        for (int b = 0; b < 2; ++b) {
            if (!out.send[b])
                continue;
            if (sent[b] < 64)
                sentAt[b][sent[b]] = ms;
            ++sent[b];
            firsts[b] += !out.again[b];
            lasts[b] += out.last[b];
            sentAway += !front;
            sentHeld += in.modHolds[b];
            sentOther += other;
        }
        return out;
    }
    // ... a frame on.
    inputrules::ReleaseOut Step(unsigned soldier, bool front, bool holdLeft = false, bool holdRight = false, bool other = false, bool refusedLeft = false) {
        return At(now + kStepMs, soldier, front, holdLeft, holdRight, other, refusedLeft);
    }
    // ... frames on for ms.
    void Run(unsigned long long ms, unsigned soldier, bool front, bool holdLeft = false, bool holdRight = false) {
        const unsigned long long until = now + ms;
        while (now + kStepMs <= until)
            Step(soldier, front, holdLeft, holdRight);
    }
    bool Clean() const { return sentAway == 0 && sentHeld == 0 && sentOther == 0; }
};
using Why = inputrules::Why;

void RowRelease() {
    {
        // 1a: the game starts with bf3 in front and no soldier: nothing; the first soldier: both at once, for the soldier, then again 0.25, 0.5, 1, 2 and
        // 4 s after (each within a frame of its time; the last marked), and nothing more.
        Frames f;
        f.Step(0, true);
        const int before = f.sent[0] + f.sent[1];
        const inputrules::ReleaseOut first = f.Step(1, true);
        const unsigned long long t0 = f.now;
        f.Run(7000, 1, true);
        bool onTime = f.sent[0] == 1 + inputrules::kAgainCount && f.sent[1] == 1 + inputrules::kAgainCount;
        double worst = 0.0;
        for (int b = 0; b < 2 && onTime; ++b)
            for (int k = 0; k < inputrules::kAgainCount; ++k) {
                const double late = static_cast<double>(f.sentAt[b][k + 1]) - static_cast<double>(t0 + inputrules::kAgainMs[k]);
                worst = std::max(worst, std::fabs(late));
                onTime = onTime && late >= 0.0 && late < static_cast<double>(kStepMs);
            }
        Report("1a", "a new soldier: each button's release at once and again 0.25 to 4 s after, bf3 in front",
               before == 0 && first.send[0] && first.send[1] && first.why[0] == Why::kSoldier && first.why[1] == Why::kSoldier && !first.again[0] &&
                   first.newSoldier && onTime && f.firsts[0] == 1 && f.lasts[0] == 1 && f.lasts[1] == 1 && f.Clean(),
               "before the soldier %d; at it left %d right %d (for the soldier %d %d); in 7 s: left %d (%d first, %d last), right %d; the sends after the first "
               "at most %.0f ms off their times",
               before, first.send[0], first.send[1], first.why[0] == Why::kSoldier, first.why[1] == Why::kSoldier, f.sent[0], f.firsts[0], f.lasts[0], f.sent[1],
               worst);
    }
    {
        // 1b: a new soldier while another window is in front: nothing till bf3 is back in front; then its first, owed for the soldier, and its sends after.
        Frames f;
        f.Step(0, true);
        f.Step(0, false);
        f.Step(1, false);
        int away = 0;
        for (int k = 0; k < 300; ++k)
            away += f.Step(1, false).send[0];
        const inputrules::ReleaseOut back = f.Step(1, true);
        f.Run(5000, 1, true);
        Report("1b", "a new soldier with bf3 not in front: sent as it comes back", away == 0 && back.send[0] && back.send[1] && back.why[0] == Why::kSoldier &&
                                                                                     back.backInFront && f.firsts[0] == 1 && f.sent[0] == 1 + inputrules::kAgainCount && f.Clean(),
               "sent while away %d; back in front: left %d right %d, for the soldier %d; in all %d (%d first)", away, back.send[0], back.send[1],
               back.why[0] == Why::kSoldier, f.sent[0], f.firsts[0]);
    }
    {
        // 1c: bf3 out of the front and back (no new soldier): a first each coming back, owed for the coming back; out and back again: one more.
        Frames f;
        f.Step(3, true);
        f.Step(3, true);
        const int start = f.firsts[0];
        f.Step(3, false);
        const inputrules::ReleaseOut back = f.Step(3, true);
        f.Step(3, true);
        f.Step(3, false);
        f.Step(3, false);
        f.Step(3, true);
        Report("1c", "bf3 back in front: a first each coming back", start == 1 && back.send[0] && back.send[1] && back.why[0] == Why::kFront && back.backInFront &&
                                                                         f.firsts[0] == 3 && f.firsts[1] == 3 && f.sent[0] == 3 && f.Clean(),
               "the first soldier's %d; the first coming back left %d right %d (for coming back %d); firsts %d %d, sends %d", start, back.send[0], back.send[1],
               back.why[0] == Why::kFront, f.firsts[0], f.firsts[1], f.sent[0]);
    }
    {
        // 1d: the mod holds the fire button (a trigger, a film's press, the grenade launcher's shot): the left waits, the right goes; let go: the left.
        Frames f;
        f.Step(0, true);
        const inputrules::ReleaseOut at = f.Step(1, true, true, false);
        int held = 0;
        for (int k = 0; k < 200; ++k)
            held += f.Step(1, true, true, false).send[0];
        const inputrules::ReleaseOut letGo = f.Step(1, true, false, false);
        Report("1d", "the mod's own fire held: its release waits for it", !at.send[0] && at.send[1] && held == 0 && letGo.send[0] && !letGo.again[0] &&
                                                                                letGo.why[0] == Why::kSoldier && f.sent[0] == 1 && f.Clean(),
               "at the soldier: left %d right %d; while held %d; let go: left %d (its first %d, for the soldier %d); the right's sends meanwhile %d", at.send[0],
               at.send[1], held, letGo.send[0], !letGo.again[0], letGo.why[0] == Why::kSoldier, f.sent[1]);
    }
    {
        // 1e: weapon.txt "zoom 1" holds the right a long time: never sent while held, its first after; the left goes at once, a first for each soldier.
        Frames f;
        f.Step(0, true);
        int right = 0;
        for (int k = 0; k < 2000; ++k)
            right += f.Step(k < 1000 ? 1 : 2, true, false, true).send[1];
        const int leftFirsts = f.firsts[0];
        const inputrules::ReleaseOut off = f.Step(2, true, false, false);
        Report("1e", "the zoom held: the right's release never while it's held", right == 0 && leftFirsts == 2 && off.send[1] && !off.again[1] && f.firsts[1] == 1 && f.Clean(),
               "while held: right %d, the left's firsts %d (two soldiers); let go: right %d (its first %d)", right, leftFirsts, off.send[1], !off.again[1]);
    }
    {
        // 1f: a mouse button held that the mod isn't holding (a real click to bring bf3 to the front): nothing till it's let go.
        Frames f;
        f.Step(5, true);
        f.Step(5, true);
        f.Step(5, false);
        int clicked = 0;
        for (int k = 0; k < 20; ++k)
            clicked += f.Step(5, true, false, false, true).send[0];
        const inputrules::ReleaseOut after = f.Step(5, true);
        Report("1f", "a real button held: the release after it", clicked == 0 && after.send[0] && after.send[1] && f.sentOther == 0,
               "while held %d; let go: left %d right %d", clicked, after.send[0], after.send[1]);
    }
    {
        // 1g: the first step's front isn't a coming back; a first step away then in front is.
        Frames a, b;
        const inputrules::ReleaseOut first = a.Step(0, true);
        b.Step(0, false);
        const inputrules::ReleaseOut in = b.Step(0, true);
        Report("1g", "the first step: no coming back; away, then in front: one", !first.send[0] && !first.send[1] && in.send[0] && in.send[1] && in.backInFront,
               "first in front: %d %d; away then in front: %d %d", first.send[0], first.send[1], in.send[0], in.send[1]);
    }
    {
        // 1h: in front, the same soldier, its sends done, then 100000 frames: nothing. Flapping every frame for 1000 frames: a first each coming back (500)
        // and none of the sends after (each coming back starts them again), never away.
        Frames f;
        f.Step(1, true);
        f.Run(5000, 1, true);
        const int start = f.sent[0];
        for (int k = 0; k < 100000; ++k)
            f.Step(1, true);
        const int still = f.sent[0] - start;
        const int firstsBefore = f.firsts[0];
        for (int k = 0; k < 1000; ++k)
            f.Step(1, (k & 1) != 0);
        const int flaps = f.sent[0] - start - still, flapFirsts = f.firsts[0] - firstsBefore;
        Report("1h", "nothing while nothing changes; a first a coming back when it flaps", start == 1 + inputrules::kAgainCount && still == 0 && flaps == 500 &&
                                                                                                flapFirsts == 500 && f.Clean(),
               "the first soldier's %d sends; still: %d in 100000 frames; flapping: %d sends (%d firsts) of 500 returns, %d away", start, still, flaps, flapFirsts,
               f.sentAway);
    }
    {
        // 1i: Operation Swordbreaker's roof (13:22): the trigger held at the death (13:22:29.9), the headset off (13:22:33.6: the mod's buttons let go, bf3
        // in front throughout), the checkpoint's new soldier at 13:22:46.7: the fire release at that frame and again 0.25 to 4 s after, none while the
        // trigger was held (the old: none at all).
        Frames f;
        f.At(10000, 1, true);  // (the soldier who died: its own release at its start, long before)
        f.Run(5000, 1, true);
        const int before = f.sent[0];
        const double kFrame = 1.0 / 72.0;
        double first = -1.0, last = -1.0;
        int frames = 0;
        for (double t = 29.0; t < 70.0; t += kFrame, ++frames) {
            const bool trigger = t >= 29.9 && t < 33.6;
            const unsigned soldier = t >= 46.7 ? 2 : 1;
            const inputrules::ReleaseOut o = f.At(static_cast<unsigned long long>(t * 1000.0), soldier, true, trigger, false);
            if (o.send[0] && first < 0.0)
                first = t;
            if (o.send[0])
                last = t;
        }
        Report("1i", "Swordbreaker's roof replayed: the release at the new soldier, and again", f.sent[0] - before == 1 + inputrules::kAgainCount && first >= 46.7 - 1e-9 &&
                                                                                                    first < 46.7 + 2 * kFrame && last >= 50.7 - 1e-9 && last < 50.7 + 2 * kFrame && f.Clean(),
               "fire releases from the death on %d, the first at %.3f s (the new soldier at 46.700), the last at %.3f (50.700 wanted)", f.sent[0] - before, first, last);
    }
    {
        // 1j: the mod presses the fire button 0.1 s after the first and holds it to 1.7 s: nothing while held; at its let-go one send for the three due
        // (0.25, 0.5, 1 s: not made up for), then 2 and 4 s.
        Frames f;
        f.At(20000, 0, true);
        f.At(20014, 1, true);
        const int firstSends = f.sent[0];
        int held = 0;
        for (unsigned long long t = 20100; t < 21700; t += kStepMs)
            held += f.At(t, 1, true, true, false).send[0];
        const inputrules::ReleaseOut letGo = f.At(21700, 1, true);
        f.Run(4000, 1, true);
        Report("1j", "the sends after the first wait while the mod holds it, the ones due made one", firstSends == 1 && held == 0 && letGo.send[0] && letGo.again[0] &&
                                                                                                         f.sent[0] == 4 && f.lasts[0] == 1 && f.Clean(),
               "the first %d; while held %d; at the let-go %d; in all %d (4 wanted), the last marked %d", firstSends, held, letGo.send[0], f.sent[0], f.lasts[0]);
    }
    {
        // 1k: bf3 out of the front after the 0.25 s send, back at 3 s: nothing away; back: a first afresh for coming back, then its own sends after.
        Frames f;
        f.At(30000, 0, true);
        f.At(30014, 1, true);
        f.Run(300, 1, true);
        const int beforeAway = f.sent[0];
        int away = 0;
        for (unsigned long long t = f.now + kStepMs; t < 33014; t += kStepMs)
            away += f.At(t, 1, false).send[0];
        const inputrules::ReleaseOut back = f.At(33014, 1, true);
        f.Run(5000, 1, true);
        Report("1k", "out of the front between the sends: none away, afresh as it comes back", beforeAway == 2 && away == 0 && back.send[0] && !back.again[0] &&
                                                                                                    back.why[0] == Why::kFront && f.sent[0] == 2 + 1 + inputrules::kAgainCount &&
                                                                                                    f.firsts[0] == 2 && f.Clean(),
               "before going %d; away %d; back: %d (a first %d, for coming back %d); in all %d (%d firsts)", beforeAway, away, back.send[0], !back.again[0],
               back.why[0] == Why::kFront, f.sent[0], f.firsts[0]);
    }
    {
        // 1l: a release of the mod's own refused by Windows (input.cpp: that button's down then is its own, not a real one held): owed; not while bf3 is out
        // of the front, nor while the mod holds it again; sent as soon as neither, for the refusal.
        Frames f;
        f.At(40000, 7, true);
        f.Run(5000, 7, true);
        const int start = f.sent[0];
        int away = 0, held = 0;
        for (int k = 0; k < 50; ++k)
            away += f.Step(7, false, false, false, false, true).send[0];
        f.Step(7, true, true, false, false, true);  // (back in front with the mod holding it: a first owed for coming back too, waiting)
        for (int k = 0; k < 50; ++k)
            held += f.Step(7, true, true, false, false, true).send[0];
        const inputrules::ReleaseOut sent = f.Step(7, true, false, false, false, true);
        // ... and in front all along, nothing else owed: the refusal alone owes it.
        Frames g;
        g.At(50000, 7, true);
        g.Run(5000, 7, true);
        const inputrules::ReleaseOut alone = g.Step(7, true, false, false, false, true);
        Report("1l", "a release Windows refused: owed, sent once bf3 is in front and the mod lets go", start == 1 + inputrules::kAgainCount && away == 0 && held == 0 &&
                                                                                                         sent.send[0] && alone.send[0] && alone.why[0] == Why::kRefused &&
                                                                                                         !alone.send[1] && f.Clean() && g.Clean(),
               "away %d; held %d; then sent %d; in front all along: sent %d, for the refusal %d, the right %d", away, held, sent.send[0], alone.send[0],
               alone.why[0] == Why::kRefused, alone.send[1]);
    }
    {
        // 1m: the clock going back between the sends: the rest dropped, never a flood.
        Frames f;
        f.At(60000, 0, true);
        f.At(60014, 1, true);
        f.At(60300, 1, true);
        const int before = f.sent[0];
        f.At(50000, 1, true);
        for (int k = 0; k < 1000; ++k)
            f.Step(1, true);
        Report("1m", "the clock going back: the sends after it dropped", before == 2 && f.sent[0] == 2 && f.Clean(), "before %d, after %d", before, f.sent[0]);
    }
}

// ---- 2: the fire button's sends said ----
void RowSay() {
    {
        // 2a: 100 sends in one second with bf3 in front: two said; the next second's first says the 98.
        inputrules::SayState s;
        int said = 0;
        for (int k = 0; k < 100; ++k)
            said += inputrules::StepSay(s, 10000 + k * 9, true).say;
        const inputrules::SayOut next = inputrules::StepSay(s, 11000, true);
        Report("2a", "two lines a second in front, the rest told next", said == 2 && next.say && next.quiet == 98 && next.quietAway == 0,
               "said %d of 100; the next second's first: said %d, %d before it (%d away)", said, next.say, next.quiet, next.quietAway);
    }
    {
        // 2b: away sends: four a second, apart from the in-front ones' two.
        inputrules::SayState s;
        int said = 0;
        for (int k = 0; k < 10; ++k)
            said += inputrules::StepSay(s, 5000 + k, false).say;
        int inFront = 0;
        for (int k = 0; k < 10; ++k)
            inFront += inputrules::StepSay(s, 5100 + k, true).say;
        Report("2b", "four lines a second with bf3 not in front, two in front", said == 4 && inFront == 2, "away %d of 10, in front %d of 10", said, inFront);
    }
    {
        // 2c: a long fight: a tap every 5 frames for 60 s, a send each down and up (some stretches away): every send said or counted; at most six lines
        // in a second of the rule's own, twelve in any 1000 ms.
        inputrules::SayState s;
        int sends = 0, said = 0, counted = 0, worst = 0, inWindow = 0, slidingWorst = 0;
        unsigned long long windowFrom = 0, lines[64] = {};
        for (int frame = 0; frame < 72 * 60; ++frame) {
            const unsigned long long now = 100000 + static_cast<unsigned long long>(frame * 1000.0 / 72.0);
            if (frame % 5 > 1)
                continue;
            if (windowFrom == 0 || now - windowFrom >= 1000)
                windowFrom = now, inWindow = 0;
            const bool front = (frame / 360) % 7 != 3;  // (a stretch of every seven with bf3 not in front)
            const inputrules::SayOut o = inputrules::StepSay(s, now, front);
            ++sends;
            if (o.say) {
                lines[said % 64] = now;
                int recent = 0;
                for (int k = 0; k < 64 && k <= said; ++k)
                    recent += now - lines[(said - k) % 64] < 1000;
                slidingWorst = std::max(slidingWorst, recent);
                ++said;
                counted += o.quiet;
                worst = std::max(worst, ++inWindow);
            }
        }
        const int left = s.quiet;
        Report("2c", "a minute's fight: every send said or counted, six lines a second", said + counted + left == sends && worst <= 6 && slidingWorst <= 12,
               "%d sends: %d said, %d told in later lines, %d still to tell; the most lines in a second of the rule's %d, in any 1000 ms %d", sends, said, counted,
               left, worst, slidingWorst);
    }
    {
        // 2d: the clock going back: a fresh second, not a long silence.
        inputrules::SayState s;
        inputrules::StepSay(s, 50000, true);
        inputrules::StepSay(s, 50001, true);
        const bool third = inputrules::StepSay(s, 50002, true).say;
        const bool back = inputrules::StepSay(s, 40000, true).say;
        Report("2d", "the clock going back starts a second afresh", !third && back, "the third in a second %d; after the clock went back %d", third, back);
    }
}

// ---- 3: a film round's real rounds ----
void RowRoundFire() {
    const float d45 = -45.0f * kPi / 180.0f, d55 = -55.0f * kPi / 180.0f, d30 = -30.0f * kPi / 180.0f;
    const char* noWord = inputrules::RoundFireRefused(false, true, d55);
    const char* oneHand = inputrules::RoundFireRefused(true, false, d55);
    const char* up = inputrules::RoundFireRefused(true, true, d30);
    const char* nan = inputrules::RoundFireRefused(true, true, kNaN);
    const char* at45 = inputrules::RoundFireRefused(true, true, d45);
    const char* at55 = inputrules::RoundFireRefused(true, true, d55);
    const char* selfTest = inputrules::RoundFireRefused(false, false, d55);
    Report("3", "a round's pull fires only with the word, two hands and the head down",
           noWord && std::strstr(noWord, "fire 1") && oneHand && std::strstr(oneHand, "one hand") && up && std::strstr(up, "45 degrees") && nan && !at45 && !at55 &&
               selfTest && std::strstr(selfTest, "fire 1"),
           "no word: %s; one hand: %s; 30 down: %s; NaN: %s; 45 down: %s; 55 down: %s; the self-test's (test 1): %s", noWord ? noWord : "fires",
           oneHand ? oneHand : "fires", up ? up : "fires", nan ? nan : "fires", at45 ? at45 : "fires", at55 ? at55 : "fires", selfTest ? selfTest : "fires");
}

// ---- 4: X held waiting for the game's reload ----
// A frame's order as ammo.cpp has it: the game's count (raw) read, its rise looked at, the wait stepped; the hold comes from input's Sync later in the
// frame. The picture's magazine: the old rule put it in at the hold, the new at the step's kLoaded (and the game's own reload's kept insert).
struct Gun {
    int raw = 2;          // the game's count (1 kept in the chamber, 1 hidden)
    int last = 2;         // as last seen
    bool magOut = true;   // the picture (new rule)
    bool oldMagOut = true;  // the picture (old rule)
    int wrongFrames = 0, oldWrongFrames = 0;  // frames a magazine was drawn in a gun the game hadn't loaded (raw 2 or less)
    int loadedAt = -1;    // the frame the new rule drew it
};
void RowPress() {
    constexpr double kFrame = 1000.0 / 72.0;
    {
        // 4a, 4b: Rock and a Hard Place's F11: the magazine ejected, X held at 6.6 s, the game's count up 1.39 s after the hold.
        inputrules::PressWait w;
        Gun g;
        int seenLoaded = 0, before = 0;
        const unsigned long long t0 = 1000000;
        const bool none = inputrules::StepPress(w, t0, 7, false) == inputrules::PressSeen::kNone;
        for (int frame = 0; frame < 72 * 12; ++frame) {
            const unsigned long long now = t0 + static_cast<unsigned long long>(frame * kFrame);
            const double t = frame / 72.0;
            if (t >= 6.6 + 1.39 && g.raw == 2)
                g.raw = 32;  // (the game's 31 and the hidden round back on top)
            const bool rose = g.raw > g.last;
            g.last = g.raw;
            const inputrules::PressSeen s = inputrules::StepPress(w, now, 7, rose);
            if (s == inputrules::PressSeen::kLoaded) {
                ++seenLoaded;
                g.magOut = false;
                g.loadedAt = frame;
            }
            if (!g.magOut && g.raw <= 2)
                ++g.wrongFrames;
            if (!g.oldMagOut && g.raw <= 2)
                ++g.oldWrongFrames;
            if (t >= 6.6 && !w.at && g.loadedAt < 0 && seenLoaded == 0) {  // the hold, once
                w.at = now, w.gun = 7, w.loaded = 1;
                g.oldMagOut = false;
                before = frame;
            }
        }
        const double after = (g.loadedAt - before) / 72.0;
        Report("4a", "no wait: nothing", none, "kNone %d", none);
        Report("4b", "Rock and a Hard Place: the magazine drawn when the count rose", seenLoaded == 1 && !g.magOut && g.wrongFrames == 0 && after > 1.38 && after < 1.42,
               "loaded %d time(s), %.2f s after the hold; frames with a magazine the game hadn't loaded: %d (the old rule: %d, the hold to the rise)", seenLoaded,
               after, g.wrongFrames, g.oldWrongFrames);
    }
    {
        // 4c: Operation Swordbreaker's opening: six holds 4 s apart, the game refusing every one. The old rule drew a magazine at each; the new never.
        inputrules::PressWait w;
        Gun g;
        int loaded = 0, gaveUp = 0, waits = 0;
        const unsigned long long t0 = 2000000;
        for (int frame = 0; frame < 72 * 40; ++frame) {
            const unsigned long long now = t0 + static_cast<unsigned long long>(frame * kFrame);
            const inputrules::PressSeen s = inputrules::StepPress(w, now, 9, false);
            loaded += s == inputrules::PressSeen::kLoaded;
            gaveUp += s == inputrules::PressSeen::kGaveUp;
            if (!g.magOut && g.raw <= 2)
                ++g.wrongFrames;
            if (!g.oldMagOut && g.raw <= 2)
                ++g.oldWrongFrames;
            if (frame % (72 * 4) == 36 && frame < 72 * 24) {
                w.at = now, w.gun = 9, w.loaded = 1;  // (a hold while a wait runs starts it again)
                ++waits;
                g.oldMagOut = false;  // (the old rule's insert at the hold; the next tap's eject took it out again)
            }
            if (frame % (72 * 4) == 36 + 72 * 3)
                g.oldMagOut = true;
        }
        Report("4c", "Swordbreaker's six refused holds: never a magazine drawn", loaded == 0 && g.magOut && g.wrongFrames == 0 && g.oldWrongFrames > 0 && gaveUp == 1,
               "%d holds: loaded %d, gave up %d (the last, 8 s on); frames with a magazine drawn the game hadn't loaded: %d (the old rule %d)", waits, loaded,
               gaveUp, g.wrongFrames, g.oldWrongFrames);
    }
    {
        // 4d: the 8 s; a rise of three rounds (the last spare ones) loads; another gun ends it; the clock going back ends it.
        inputrules::PressWait w;
        w.at = 100000, w.gun = 3, w.loaded = 1;
        const bool waiting = inputrules::StepPress(w, 100000 + 7999, 3, false) == inputrules::PressSeen::kWaiting;
        const bool gave = inputrules::StepPress(w, 100000 + 8000, 3, false) == inputrules::PressSeen::kGaveUp && !w.at;
        w.at = 100000, w.gun = 3;
        const bool three = inputrules::StepPress(w, 100500, 3, true) == inputrules::PressSeen::kLoaded && !w.at;
        w.at = 100000, w.gun = 3;
        const bool other = inputrules::StepPress(w, 100500, 4, true) == inputrules::PressSeen::kOtherGun && !w.at;
        w.at = 100000, w.gun = 3;
        const bool newAtSame = inputrules::StepPress(w, 100500, 0, false) == inputrules::PressSeen::kOtherGun;
        w.at = 100000, w.gun = 3;
        const bool back = inputrules::StepPress(w, 99000, 3, false) == inputrules::PressSeen::kGaveUp;
        const bool after = inputrules::StepPress(w, 99000, 3, true) == inputrules::PressSeen::kNone;
        Report("4d", "the wait's end: 8 s, any rise, another gun, the clock", waiting && gave && three && other && newAtSame && back && after,
               "7.999 s waiting %d; 8 s gave up %d; three rounds loaded %d; another gun %d; a new gun at the same address %d; the clock back %d; then nothing %d",
               waiting, gave, three, other, newAtSame, back, after);
    }
    {
        // 4e: a magazine pushed home by hand during the wait (StillAsked): the game's reload of that hold not asked for from then, R down or let go, up to
        // 7.999 s after its let-go; asked again 8 s on, at a new press of R after it was let go, with another gun; never asked when it wasn't anyway.
        inputrules::HandOver h;
        const bool plain = inputrules::StillAsked(h, true, 5, 300) && !inputrules::StillAsked(h, false, 5, 300);
        h.gun = 5;
        const bool down = !inputrules::StillAsked(h, true, 5, 0);
        const bool up1 = !inputrules::StillAsked(h, true, 5, 1000);
        const bool up79 = !inputrules::StillAsked(h, true, 5, 7999);
        const bool out8 = inputrules::StillAsked(h, true, 5, 8000) && h.gun == 0;
        h = inputrules::HandOver{};
        h.gun = 5;
        inputrules::StillAsked(h, true, 5, 0);
        inputrules::StillAsked(h, true, 5, 500);
        const bool again = inputrules::StillAsked(h, true, 5, 0) && h.gun == 0;
        h = inputrules::HandOver{};
        h.gun = 5;
        const bool other = inputrules::StillAsked(h, true, 6, 100) && h.gun == 0;
        h = inputrules::HandOver{};
        h.gun = 5;
        const bool notAsked = !inputrules::StillAsked(h, false, 5, 100);
        Report("4e", "a magazine pushed home by hand in the wait: that hold's reload not asked for after it", plain && down && up1 && up79 && out8 && again && other && notAsked,
               "none: as asked %d; R down %d, 1 s %d, 7.999 s %d not asked; 8 s asked again %d; R pressed again %d; another gun %d; not asked anyway %d", plain, down,
               up1, up79, out8, again, other, notAsked);
    }
    {
        // 4f: Rock and a Hard Place's F11 with a hand quicker than the game: the magazine out (1 loaded and the hidden round: 2), X held, a fresh one
        // pushed home by hand 0.4 s after the hold (2 -> 32, 341 spare), then the game's reload landing on it 1.39 s after the hold, as on 2026-10-30 (its
        // 29 on top: 61, 312 spare). As ammo.cpp works it: undone to the hand's 32 and 341 (the old: kept, and 60 learned as the magazine's size). With
        // no hand first, the game's rise from 2 to 32 is kept as before.
        inputrules::HandOver h;
        h.gun = 7;
        pickrules::CountIn in;
        in.last = 32, in.raw = 61, in.spare = 312, in.hidden = 1, in.byHand = true;
        const bool askedByR = 1390 - 800 < pickrules::kReloadAskedMs;  // (R let go 0.8 s after X went down, 0.2 s after the hold)
        in.asked = inputrules::StillAsked(h, askedByR, 7, 1390 - 200);
        const pickrules::CountOut now = pickrules::SeeCount(in);
        pickrules::CountIn old = in;
        old.asked = askedByR;
        const pickrules::CountOut before = pickrules::SeeCount(old);
        pickrules::CountIn plainIn;
        plainIn.last = 2, plainIn.raw = 32, plainIn.spare = 341, plainIn.hidden = 1, plainIn.byHand = true;
        inputrules::HandOver none;
        plainIn.asked = inputrules::StillAsked(none, askedByR, 7, 1390 - 200);
        const pickrules::CountOut plain = pickrules::SeeCount(plainIn);
        Report("4f", "the game's reload landing on a hand's magazine: undone; with no hand first: kept",
               now.what == pickrules::CountSeen::kUndone && now.loaded == 32 && now.spare == 341 && before.what == pickrules::CountSeen::kKept &&
                   plain.what == pickrules::CountSeen::kKept,
               "now %d (%d loaded as the game counts them, %d spare); the old rule %d (kept, %d learned); no hand first %d", static_cast<int>(now.what), now.loaded,
               now.spare, static_cast<int>(before.what), in.raw - 1, static_cast<int>(plain.what));
    }
    {
        // 4g: what the rise draws: the magazine for both; the bolt for the M416 alone, the chamber for a held gun alone (a held gun's reload leaves the
        // M416's bolt as it was in its slot, and the other way round).
        const inputrules::PressDraw rifle = inputrules::DrawAtRise(inputrules::kPressRifle), held = inputrules::DrawAtRise(inputrules::kPressHeld);
        Report("4g", "the rise draws each gun's own: the M416's bolt, a held gun's chamber", rifle.magazine && rifle.bolt && !rifle.chamber && held.magazine && !held.bolt &&
                                                                                               held.chamber,
               "the M416: magazine %d, bolt %d, chamber %d; a held gun: magazine %d, bolt %d, chamber %d", rifle.magazine, rifle.bolt, rifle.chamber, held.magazine,
               held.bolt, held.chamber);
    }
}

// ---- 5: the tracking record ----
tracking::Frame Seen(double t, float x, float y, float z, bool sensed = false, float vx = 0, float vy = 0, float vz = 0) {
    tracking::Frame f;
    f.t = t, f.watched = true, f.positionValid = f.positionTracked = f.orientationValid = true;
    f.place[0] = x, f.place[1] = y, f.place[2] = z;
    f.sensed = sensed, f.speed[0] = vx, f.speed[1] = vy, f.speed[2] = vz;
    return f;
}
tracking::Frame Guessed(double t, float x, float y, float z) {
    tracking::Frame f = Seen(t, x, y, z);
    f.positionTracked = false;
    return f;
}
tracking::Frame NoPlace(double t) {
    tracking::Frame f;
    f.t = t, f.watched = true;
    return f;
}
tracking::Frame Off(double t) {
    tracking::Frame f;
    f.t = t;
    return f;
}

void RowTracking() {
    const double dt = 1.0 / 72.0;
    {
        // 5a-5f: seen, lost with a guess, the guess stopped, guessed again, seen again; seen again from none.
        tracking::Record r;
        const tracking::Out first = tracking::Step(r, Seen(10.0, 0.1f, 1.0f, 0.2f, true, 0.3f, 0.0f, -0.4f));
        tracking::Out still = tracking::Step(r, Seen(10.0 + dt, 0.1f, 1.0f, 0.19f, true, 0.3f, 0.0f, -0.4f));
        const bool a = first.watchedNow && first.firstWatch && first.change == tracking::Change::kNone && still.change == tracking::Change::kNone && r.haveSeen &&
                       r.speedKnown && r.speedSensed && std::fabs(r.seenSpeed[2] + 0.4f) < 1e-6f && std::fabs(r.seenAt[2] - 0.19f) < 1e-6f;
        const float seenZ = r.seenAt[2], speedZ = r.seenSpeed[2];
        const bool sensedThen = r.speedSensed;
        const tracking::Out lost = tracking::Step(r, Guessed(10.0 + 2 * dt, 0.12f, 1.0f, 0.15f));
        const bool b = lost.change == tracking::Change::kLost && lost.guessed && r.state == tracking::State::kGuessed && r.haveGuess &&
                       std::fabs(r.seenAt[2] - 0.19f) < 1e-6f;
        tracking::Step(r, Guessed(10.5, 0.15f, 1.0f, 0.10f));
        const tracking::Out stopped = tracking::Step(r, NoPlace(10.84 + 2 * dt));
        const bool c = stopped.change == tracking::Change::kGuessStopped && std::fabs(stopped.after - (10.84 + 2 * dt - (10.0 + 2 * dt))) < 1e-9 &&
                       std::fabs(stopped.guessOff - std::sqrt(0.05f * 0.05f + 0.09f * 0.09f)) < 1e-4f;
        const tracking::Out again = tracking::Step(r, Guessed(11.14 + 2 * dt, 0.0f, 1.2f, 0.0f));
        const bool d = again.change == tracking::Change::kGuessedAgain && std::fabs(again.after - 0.30) < 1e-6;
        const tracking::Out seen = tracking::Step(r, Seen(11.5, 0.0f, 1.26f, 0.08f));
        const bool e = seen.change == tracking::Change::kSeenAgain && seen.guessBefore && std::fabs(seen.guessOff - 0.1f) < 1e-4f &&
                       std::fabs(seen.after - (11.5 - (10.0 + dt))) < 1e-9 && r.state == tracking::State::kSeen;
        tracking::Step(r, NoPlace(12.0));
        const tracking::Out fromNone = tracking::Step(r, Seen(12.5, 0.0f, 1.0f, 0.0f));
        const bool f = fromNone.change == tracking::Change::kSeenAgain && !fromNone.guessBefore && std::fabs(fromNone.after - 1.0) < 1e-9;
        Report("5a", "seen: the record follows it, its own speed", a, "first watch %d, its place z %.3f, speed by the sensors %d (z %.2f m/s)", first.firstWatch, seenZ,
               sensedThen, speedZ);
        Report("5b", "lost, the runtime guessing: line 2", b, "change %d, guessed %d, last seen kept %d", static_cast<int>(lost.change), lost.guessed, b);
        Report("5c", "the guess stopped: line 3 with its time and its last guess's distance", c, "after %.3f s, the guess %.1f cm from the last seen place",
               stopped.after, stopped.guessOff * 100.0f);
        Report("5d", "guessed again after none: line 4", d, "after %.3f s", again.after);
        Report("5e", "seen again after a guess: line 5 with the guess's error", e, "after %.3f s, the guess %.1f cm off", seen.after, seen.guessOff * 100.0f);
        Report("5f", "seen again from no place: no guess just before", f, "after %.3f s, guess before %d", fromNone.after, fromNone.guessBefore);
    }
    {
        // 5g, 5h: the headset off (not watched) and on again: no loss and no regain said, "again" said; the first watch is no "again".
        tracking::Record r;
        tracking::Step(r, Seen(1.0, 0, 1, 0));
        tracking::Step(r, Seen(1.0 + dt, 0, 1, 0));
        const tracking::Out off = tracking::Step(r, Off(2.0));
        int lines = off.change != tracking::Change::kNone;
        for (int k = 1; k < 500; ++k)
            lines += tracking::Step(r, Off(2.0 + k * dt)).change != tracking::Change::kNone;
        const tracking::Out on = tracking::Step(r, NoPlace(30.0));
        const tracking::Out seen = tracking::Step(r, Seen(30.5, 0, 1, 0));
        const bool g = lines == 0 && !off.summary && r.everWatched && on.watchedNow && !on.firstWatch && on.change == tracking::Change::kNone &&
                       seen.change == tracking::Change::kSeenAgain && std::fabs(seen.after - 0.5) < 1e-9;
        Report("5g", "the headset off and on: no loss said, the watch afresh", g, "changes while off %d; on again: watched %d, again %d; then seen after %.2f s (from the watch's start)",
               lines, on.watchedNow, !on.firstWatch, seen.after);
    }
    {
        // 5h: a turn not valid with the place tracked is guessed; a place not finite is none.
        tracking::Record r;
        tracking::Frame noTurn = Seen(1.0, 0, 1, 0);
        noTurn.orientationValid = false;
        tracking::Step(r, noTurn);
        const bool guessed = r.state == tracking::State::kGuessed;
        tracking::Frame bad = Seen(1.1, kNaN, 1, 0);
        tracking::Step(r, bad);
        const bool none = r.state == tracking::State::kNone;
        Report("5h", "a turn not valid: guessed; a place not finite: no place", guessed && none, "guessed %d, none %d", guessed, none);
    }
    {
        // 5i: the speed by two seen places 1/72 s apart; not after a gap of 0.3 s; the sensors' when given.
        tracking::Record r;
        tracking::Step(r, Seen(5.0, 0.0f, 1.0f, 0.0f));
        tracking::Step(r, Seen(5.0 + dt, 0.01f, 1.0f, 0.0f));
        const bool step = r.speedKnown && !r.speedSensed && std::fabs(r.seenSpeed[0] - 0.72f) < 1e-3f;
        const float stepSpeed = r.seenSpeed[0];
        tracking::Step(r, Seen(5.3 + dt, 0.02f, 1.0f, 0.0f));
        const bool gap = !r.speedKnown;
        tracking::Step(r, Seen(5.3 + 2 * dt, 0.03f, 1.0f, 0.0f, true, 0.0f, 0.5f, 0.0f));
        const bool sensed = r.speedKnown && r.speedSensed && std::fabs(r.seenSpeed[1] - 0.5f) < 1e-6f;
        Report("5i", "the speed: by two places, not over a gap, its sensors' when given", step && gap && sensed, "two places %d (%.3f m/s), a 0.3 s gap not known %d, sensed %d",
               step, stepSpeed, gap, sensed);
    }
    {
        // 5j: a flicker every frame for a second: six changes said, the rest counted; seen 2 s: line 7 with them, their span and where.
        tracking::Record r;
        double t = 50.0;
        tracking::Step(r, Seen(t, 0.1f, 1.0f, -0.1f));
        int said = 0;
        for (int k = 1; k <= 72; ++k) {
            t += dt;
            const tracking::Out o = (k & 1) ? tracking::Step(r, NoPlace(t)) : tracking::Step(r, Seen(t, 0.1f, 1.0f, -0.1f));
            said += o.change != tracking::Change::kNone;
        }
        const int counted = r.unsaid;
        const double spanWanted = (72 - 7) * dt;  // (the seventh change, k 7, to the last, k 72)
        tracking::Out summary;
        int summaries = 0;
        for (int k = 1; k <= 72 * 3; ++k) {
            t += dt;
            const tracking::Out o = tracking::Step(r, Seen(t, 0.1f, 1.0f, -0.1f));
            if (o.summary && summaries++ == 0)
                summary = o;
        }
        const bool j = said == tracking::kChangesSaid && counted == 72 - tracking::kChangesSaid && summaries == 1 && summary.summaryCount == counted &&
                       std::fabs(summary.summarySpan - spanWanted) < 1e-6 && std::fabs(summary.summaryAt[0] - 0.1f) < 1e-5f && std::fabs(summary.summaryAt[2] + 0.1f) < 1e-5f;
        Report("5j", "a flicker: six said in ten seconds, the rest in one line once it holds 2 s", j,
               "said %d, counted %d; line 7 %d time(s): %d changes in %.3f s (%.3f wanted), about (%.2f %.2f %.2f)", said, counted, summaries, summary.summaryCount,
               summary.summarySpan, spanWanted, summary.summaryAt[0], summary.summaryAt[1], summary.summaryAt[2]);
        // ... and ten seconds on: twelve changes 1.1 s apart, six said at once, the next four within ten seconds of the first counted, the last two said as
        // the first two age out.
        int again = 0;
        t += 10.0;
        const int owedBefore = r.unsaid;
        for (int k = 1; k <= 12; ++k) {
            t += 1.1;
            again += ((k & 1) ? tracking::Step(r, NoPlace(t)) : tracking::Step(r, Seen(t, 0.1f, 1.0f, -0.1f))).change != tracking::Change::kNone;
        }
        Report("5k", "ten seconds on: six in any ten seconds again", again == 8 && owedBefore == 0 && r.unsaid == 4,
               "said %d of 12 changes 1.1 s apart (8 wanted), %d counted", again, r.unsaid);
        // ... counted ones owed as the headset comes off: said then.
        for (int k = 0; k < 20; ++k) {
            t += dt;
            (k & 1) ? tracking::Step(r, NoPlace(t)) : tracking::Step(r, Seen(t, 0.1f, 1.0f, -0.1f));
        }
        const int owed = r.unsaid;
        const tracking::Out off = tracking::Step(r, Off(t + dt));
        Report("5l", "changes counted when the headset comes off: said then", owed > 0 && off.summary && off.summaryUnwatched && off.summaryCount == owed && r.unsaid == 0,
               "owed %d; as it came off: line 7 %d (%d changes, not watched %d)", owed, off.summary, off.summaryCount, off.summaryUnwatched);
    }
}

// ---- 6: the grip ----
void RowSqueeze() {
    const float neck[3] = {0.0f, 1.0f, 0.0f};
    {
        // 6a: armed under 0.4, a squeeze past 0.6 once; held: once; let go and again: again.
        tracking::Record r;
        tracking::Step(r, Seen(1.0, 0.1f, 1.0f, -0.1f));
        int squeezes = 0, said = 0;
        const float grips[] = {0.0f, 0.3f, 0.5f, 0.7f, 0.9f, 1.0f, 0.8f, 0.5f, 0.45f, 0.65f, 0.3f, 0.65f, 0.0f};
        double t = 1.0;
        for (float g : grips) {
            t += 0.3;
            const tracking::SqueezeOut s = tracking::Squeeze(r, t, true, g, true, neck);
            squeezes += s.squeezed, said += s.say;
        }
        Report("6a", "a squeeze: past 0.6 from under 0.4, once a press", squeezes == 2 && said == 2, "squeezes %d, said %d (two wanted: 0.45 doesn't arm)", squeezes, said);
    }
    {
        // 6b: a seen hand 0.3 m from the neck point said, 0.8 m out front (the handguard) not; a hand not seen anywhere said; no body: not said for a seen hand.
        tracking::Record close, out, lost, nobody;
        tracking::Step(close, Seen(1.0, 0.2f, 0.9f, -0.2f));
        tracking::Step(out, Seen(1.0, 0.1f, 0.8f, -0.8f));
        tracking::Step(lost, Seen(1.0, 0.1f, 0.8f, -0.8f));
        tracking::Step(lost, NoPlace(1.1));
        tracking::Step(nobody, Seen(1.0, 0.2f, 0.9f, -0.2f));
        auto squeeze = [&](tracking::Record& r, bool body) {
            tracking::Squeeze(r, 2.0, true, 0.0f, body, neck);
            return tracking::Squeeze(r, 2.1, true, 1.0f, body, neck);
        };
        const tracking::SqueezeOut a = squeeze(close, true), b = squeeze(out, true), c = squeeze(lost, true), d = squeeze(nobody, false);
        Report("6b", "said near the neck point or not seen; not out front", a.say && b.squeezed && !b.say && c.say && d.squeezed && !d.say,
               "0.3 m: %d; 0.8 m out front: %d (squeezed %d); not seen: %d; no body placed: %d", a.say, b.say, b.squeezed, c.say, d.say);
    }
    {
        // 6c: ten squeezes in a second: four said, the fifth on counted, the next said a second on tells them.
        tracking::Record r;
        tracking::Step(r, Seen(1.0, 0.1f, 1.0f, -0.1f));
        int said = 0;
        double t = 3.0;
        for (int k = 0; k < 10; ++k) {
            tracking::Squeeze(r, t, true, 0.0f, true, neck);
            t += 0.05;
            said += tracking::Squeeze(r, t, true, 1.0f, true, neck).say;
        }
        tracking::Squeeze(r, 5.0, true, 0.0f, true, neck);
        const tracking::SqueezeOut next = tracking::Squeeze(r, 5.05, true, 1.0f, true, neck);
        Report("6c", "four squeezes a second said, the rest told next", said == 4 && next.say && next.unsaidBefore == 6, "said %d of 10; the next: said %d, %d before it",
               said, next.say, next.unsaidBefore);
    }
    {
        // 6d: a grip held while it isn't read (the controller asleep, the headset off) is no squeeze when it's read again; a grip not a number: nothing.
        tracking::Record r;
        tracking::Step(r, Seen(1.0, 0.1f, 1.0f, -0.1f));
        tracking::Squeeze(r, 1.0, true, 0.0f, true, neck);
        tracking::Squeeze(r, 1.1, false, 1.0f, true, neck);
        const tracking::SqueezeOut held = tracking::Squeeze(r, 1.2, true, 1.0f, true, neck);
        const tracking::SqueezeOut nan = tracking::Squeeze(r, 1.3, true, kNaN, true, neck);
        tracking::Squeeze(r, 1.4, true, 0.0f, true, neck);
        const tracking::SqueezeOut fresh = tracking::Squeeze(r, 1.5, true, 1.0f, true, neck);
        Report("6d", "a grip held through not being read: no squeeze; NaN: nothing", !held.squeezed && !nan.squeezed && fresh.squeezed,
               "held through %d, NaN %d, fresh after %d", held.squeezed, nan.squeezed, fresh.squeezed);
    }
}

// ---- 7: the places ----
void RowPlaces() {
    {
        // 7a: each place's middle is at it.
        int right = 0;
        for (int k = 0; k < tracking::kPlaceCount; ++k)
            right += std::strcmp(tracking::Word(tracking::kPlaces[k].middle), tracking::kPlaces[k].at) == 0;
        Report("7a", "each place's middle: at it", right == tracking::kPlaceCount, "%d of %d", right, tracking::kPlaceCount);
    }
    {
        // 7b: no point of a 1 cm grid inside two places (the belly too).
        int two = 0, points = 0;
        for (int x = -40; x <= 40; ++x)
            for (int y = -70; y <= 20; ++y)
                for (int z = -35; z <= 45; ++z) {
                    const float p[3] = {x * 0.01f, y * 0.01f, z * 0.01f};
                    int inside = 0;
                    for (int k = 0; k < tracking::kPlaceCount; ++k)
                        inside += tracking::SizeIn(p, tracking::kPlaces[k]) <= 1.0f;
                    two += inside > 1;
                    ++points;
                }
        Report("7b", "no point of a 1 cm grid in two places", two == 0, "%d of %d points in two", two, points);
    }
    {
        // 7c: "by", "in front", "low", "elsewhere", not finite.
        const float by[3] = {0.17f + 0.08f * 1.3f, -0.09f, 0.04f};  // 1.3 of the right shoulder's size out to its right
        const float front[3] = {0.0f, -0.2f, 0.5f}, low[3] = {0.0f, -0.75f, 0.0f}, behind[3] = {0.0f, 0.3f, -0.5f}, bad[3] = {kNaN, 0.0f, 0.0f};
        const char* w[5] = {tracking::Word(by), tracking::Word(front), tracking::Word(low), tracking::Word(behind), tracking::Word(bad)};
        Report("7c", "the words outside the places", std::strcmp(w[0], "by the right shoulder") == 0 && std::strcmp(w[1], "in front") == 0 && std::strcmp(w[2], "low") == 0 &&
                                                        std::strcmp(w[3], "elsewhere") == 0 && std::strcmp(w[4], "elsewhere") == 0,
               "%s; %s; %s; %s; %s", w[0], w[1], w[2], w[3], w[4]);
    }
    {
        // 7d: the body's axes as the pouch's: heading 0, right is +x and forward -z; a quarter turn left, right is -z and forward -x.
        const float neck[3] = {1.0f, 1.0f, 1.0f};
        const float a[3] = {1.2f, 0.9f, 0.7f};  // 20 cm right, 10 down, 30 forward at heading 0
        float p[3], q[3];
        tracking::FromNeck(a, neck, 0.0f, p);
        const float b[3] = {0.7f, 0.9f, 0.8f};  // at heading 90 degrees: 20 cm right (-z), 10 down, 30 forward (-x)
        tracking::FromNeck(b, neck, kPi / 2.0f, q);
        const bool ok = std::fabs(p[0] - 0.2f) < 1e-5f && std::fabs(p[1] + 0.1f) < 1e-5f && std::fabs(p[2] - 0.3f) < 1e-5f && std::fabs(q[0] - 0.2f) < 1e-5f &&
                        std::fabs(q[1] + 0.1f) < 1e-5f && std::fabs(q[2] - 0.3f) < 1e-5f;
        Report("7d", "the body's axes: right, up, forward at two headings", ok, "heading 0 (%.3f %.3f %.3f); 90 degrees (%.3f %.3f %.3f)", p[0], p[1], p[2], q[0], q[1], q[2]);
    }
}

// ---- 8: the time ----
void RowTime() {
    LARGE_INTEGER f, t0, t1;
    QueryPerformanceFrequency(&f);
    volatile int sink = 0;
    double worst = 0.0;
    const char* worstName = "";
    auto time = [&](const char* name, auto&& body) {
        double best = 1e30;
        for (int run = 0; run < 3; ++run) {
            QueryPerformanceCounter(&t0);
            for (int k = 0; k < 200000; ++k)
                body(k);
            QueryPerformanceCounter(&t1);
            best = std::min(best, static_cast<double>(t1.QuadPart - t0.QuadPart) / f.QuadPart * 1e6 / 200000.0);
        }
        if (best > worst)
            worst = best, worstName = name;
    };
    inputrules::ReleaseState rs;
    inputrules::SayState ss;
    inputrules::PressWait pw;
    tracking::Record tr;
    const float neck[3] = {0.0f, 1.0f, 0.0f};
    time("StepRelease", [&](int k) {
        inputrules::ReleaseIn in;
        in.nowMs = 1000 + static_cast<unsigned long long>(k) * kStepMs;
        in.soldier = static_cast<unsigned>(k >> 10), in.front = (k & 3) != 0;
        sink = sink + inputrules::StepRelease(rs, in).send[0];
    });
    time("StepSay", [&](int k) { sink = sink + inputrules::StepSay(ss, 1000 + k, (k & 7) != 0).say; });
    time("StepPress", [&](int k) {
        if (!pw.at)
            pw.at = 1000 + k, pw.gun = 5;
        sink = sink + static_cast<int>(inputrules::StepPress(pw, 1000 + k, 5, (k & 255) == 0));
    });
    time("tracking::Step", [&](int k) {
        const tracking::Frame fr = (k & 15) ? Seen(k * 0.0139, 0.1f, 1.0f, -0.1f) : NoPlace(k * 0.0139);
        sink = sink + static_cast<int>(tracking::Step(tr, fr).change);
    });
    time("tracking::Squeeze", [&](int k) { sink = sink + tracking::Squeeze(tr, k * 0.0139, true, (k & 1) ? 1.0f : 0.0f, true, neck).say; });
    time("tracking::Word", [&](int k) {
        const float p[3] = {(k % 80 - 40) * 0.01f, (k % 90 - 70) * 0.01f, (k % 80 - 35) * 0.01f};
        sink = sink + tracking::Word(p)[3];
    });
    Report("8", "the time", worst < 1.0, "the slowest %s %.4f us a call", worstName, worst);
}

}  // namespace

int main() {
    RowRelease();
    RowSay();
    RowRoundFire();
    RowPress();
    RowTracking();
    RowSqueeze();
    RowPlaces();
    RowTime();
    std::printf("%d of %d checks failed\n", g_failed, g_counted);
    return g_failed;
}

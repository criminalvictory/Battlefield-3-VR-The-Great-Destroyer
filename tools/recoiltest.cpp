// Recoil for every gun (src/recoil.h, as recoilrun.cpp runs it), on scripted frames at 72 Hz unless a row says otherwise, with fixed seeds,
// without the game. Each check prints PASS or FAIL (INFO is reported, never judged), then the totals; the exit code is the number failed.
// The guns are table 4.2's numbers (the game's own data; the kick's sizes the design's). Model space: x left, y up, z forward to the muzzle.
//   1  one M416 round, two hands at the hip (no jitter; 1 ms frames to time the peaks): the back 1.5 cm (3%) at 30 ms (3), the rise 1.8 degrees
//      (3%) at 50 ms (3), both within 5% of their peak from 0.20 s, the back never below -0.5 mm; A's target 0.26 x 2 x [0.9, 1.0], A as drawn
//      under 1.35 x it and under 0.02 degrees from 1.2 s; the drift's camera spring never drawn lighter than 0.35 (the data's 0.2 and 0.34, the
//      M82A3's, drawn as 0.35 bit for bit: the critic's 14)
//   2  30 M416 rounds at 750 rpm, two hands: A's target at the 30th in [0.9, 1.0] x 0.26 x 31 (7.25-8.06) and never past 10; B's mean rise over
//      rounds 5-30 2.3 (0.2); the back with held fire's push back 2.4-3.0 cm on average over rounds 5-30 (the critic's 8), never 4 cm; the roll
//      to the other side every round
//   3  the M240, 50 rounds at 650 rpm: A's target at 10.0 by round 15 (1), then exactly 10.0; one hand, 40 M416 rounds: it stops at exactly 15
//      (the caps x1.5)
//   4  rows 1-3 at 72, 60 and 36 fps, the rounds on frames all three share (every 1/12 s), against a 360 fps run at the same moments: every curve
//      within 2% of its own peak
//   5  the string's recovery after row 2's last round: A's target held for 1.25 gaps, then down at 18 x 0.1 degrees a second until 2 gaps after
//      the round, then at 18 (by rounds, not the button: the critic's 14): the closed form within 0.05 degrees, the button held 0.5 s or let go;
//      one hand after 20 rounds: half of it (0.9, then 9 degrees a second)
//   6  the first shot: a pull, a release, a pull 0.12 s later (sooner than the time rule's 1.5 gaps + 50 ms, 0.17 s: the release alone makes it
//      one): its round x2 again; held 1 s with rounds: only the first x2; held, no round for 0.25 s (the time rule alone): x2 again
//   7  one hand, the M416, 10 rounds at 750 rpm, seeds 1-40: each round's own rise in [0.5, 1.5] x 9, reaching both ends (under 0.6 and over 1.4
//      x 9); its yaw within 0.8 of it (past 0.6 in some), its side changing in 30-70% of rounds; its roll 0.6 of it, to a side at random (changing
//      in 30-70%); A: the first round 0.26 x 2 x 2 x 1.5 x [0.9, 1] (1.40-1.56), the rest 0.26 x 2 x [0.9, 1], sideways 3 x -0.3..0.1 and a toss
//      of up to 0.26 either way (past -0.92 and 0.37 in some); the rise never past 20, the back never 6 cm, the turn never 30; everything finite
//   8  one hand against two, the same seed, no jitter: the first round's rise 5.0 times (0.5), its back 2.5 times; one hand's own timing: the rise
//      at its peak at 70 ms (3) and 20-30% past the line after, the back at its peak at 45 ms (3); a launcher's lift 1.5 times
//   9  the M9: one hand 6.0 degrees and 1.5 cm, two hands 4.0 and 1.0 (3%); home within 5% by 0.15 s; A a round 0.7 x [0.9, 1] in one hand, x0.8
//      in two; each round's size within 1 -+ 0.2 in one hand (past 0.15 in some), 0.1 in two; at the eye with two hands still the wrist's: the
//      turn about the grip (under 0.1 mm across), the rise as at the hip; a 9-degree, 2 cm pistol stopped at its caps, 10 and 2.5 cm in one hand,
//      7 and 2 cm in two
//  10  the sight at the eye, the M416 and its ACOG (0.75), two hands: the eye's point moving only along the gun's line (under 0.1 mm across it),
//      the rise 1.35 (3%), the slide toward the eye at most 4.5 mm and 6; the glass 2 cm in front of the eye: at most 5 mm; 1.7 cm: at most 2 mm;
//      one hand at the eye: the turn about the grip again; the eye eased in over 0.1 s and out over 0.2; irons (1) and a 1.69 cm back: the
//      slide exactly 6 mm; the glass 1 cm in front of the eye as held: never nearer it, the gun never 1 mm forward; 30 rounds: the eyepiece never
//      1 mm forward (the critic's F5); the support hand letting go at the eye after 30 rounds: the turn about the eye eased to the grip's over
//      0.2 s, no frame moving the eyepiece 4 mm
//  11  the M98B: at the eye its zoomed row (0.5 a round, back at 6 a second), at the hip 2.0 and 15; in one hand at the eye its zoomed row too
//      (x2, the first x1.5; back at 3)
//  12  the clears: newGun, held false: the identity at once, the state at its defaults, the random numbers kept; a frame of 0.3 s: the state at its
//      defaults, the last move easing out within 0.1 s, then the identity; a scripted moment with the gun still drawn: easing out too
//  13  the mirror: the left trigger with the same seed: the yaw, the roll and the drift's sideways exact negatives, the rise and the back the same;
//      mirror off: the drift's sideways and the yaw as the right's, the roll still mirrored
//  14  the fold, random aims, raises 0, 0.29 and 0.7 degrees, kicks up to 30 degrees: toView . Fold(aim) . F . raise against toView . aim . F . raise
//      . Mq within 1e-4 rad (hands.cpp's own QuatToMatrix, Mul3 and raise[9] copied here); 14b (INFO): the steer's own fold the same way
//  15  the move: with no back and no lift, the pivot kept within 1e-6 m, the turn orthonormal (1e-6)
//  16  nothing not finite: dt 0, negative, NaN, 10 s; shots 0, 3, 100 (taken as 3); damping 0, 1, NaN; rate 0; NaN holds, eyes, pivots, tunes
//  17  the buzz, every gun of table 4.2 as src/recoilguns.h has it (its kind, rate, rumble and lift; Gun::automatic left by its kind, as recoilrun
//      leaves it): its amplitude and length (1 ms), never longer than 0.8 of the gap on the automatics, the support hand's share, none with one
//      hand, the launchers' late pulse
//  18  determinism: the same seed and frames, bit for bit
//  19  the AN-94: a burst pair 50 ms apart 0.3 then 1.7, the kick (B, the drift aside) as the second round leaves within 0.3 x the rise of the
//      first's line; a lone round in its burst mode 0.3 at once and 0.7 75 ms later; the mode not known: a pull's pair shows a burst, rounds 100 ms
//      apart automatic; in automatic every round whole, at once (the critic's 12)
//  20  src/recoilguns.h (when it's there): every row's name its own; each gun's own numbers find its own row (the Stinger and the Javelin share);
//      every gun the mod draws found by its body; the M416's row as its data; every row through Step, one hand and two: finite, within the caps,
//      at rest again; each recorded row against the recording it's sized from (two hands at the hip): its first round's back and drawn rise
//      (the kick and the drift together) at their peaks, the AN-94's pair, each loop's held fire on average, all within 3%
//  21  one hand at the eye, 20 seeds, the ACOG's glass 3.5 cm in front: never within 1.5 cm of the eye (the critic's 2); irons by the gun's rear too
//  22  the scope picture's slack covers the struck frames of an SVD at 3.44x and an Mk11 at 8x, two hands at the eye, the picture one frame or two
//      behind with this frame's slack or the last's, as scopeview's MakeDesc takes it (the critic's 5); the gun's very first round isn't foreseen
//      (before it the slack is 0: the picture as it always was), so its frames are INFO; the button up from each round's frame on (no strike
//      term): the turns of this frame, the one before and the next alone cover them
//  23  a 0.3 s gap with dt capped at 0.1 clears (the critic's 6), and the next round is a first shot
//  24  the hold taken 30 ms into a one-hand kick: no frame's move larger than the springs' own speed and pull allow over it (from the stiffer and
//      more damped of the two holds' constants, with room): a cap falling below the state never yanks it (the critic's 7)
//  25  held fire's flip (recoil.txt release 1): up 2.7 degrees and back below the line; none with release 0
//  26  the hip spread wanted: one hand on a long gun only; settled here: the blend past a half for under 0.2 s changes nothing, for 0.2 s it does;
//      a hold taken afresh at once; let go (no round can leave), as it was last held
//  27  a Step takes under 20 microseconds at dt 0.25 with 3 rounds, and allocates nothing
//  28  never break: a gun held and no round fired (the button down, the eye at the sight and away, one hand then two, the left trigger): the identity
//      every frame, no slack, no pulse; the hip spread by the hold alone: on with one hand, off once two hands have held 0.2 s, never on by itself
//      with two hands; recoil.txt kick 0 and drift 0 (probe P2's set-up): 30 rounds, one hand and two, and the drawn gun never moves
// Usage: recoiltest [-v ROW]   (-v prints that row's frames)

#include <windows.h>

#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>

#include "../src/recoil.h"
#if __has_include("../src/recoilguns.h")
#include "../src/recoilguns.h"
#define RECOILTEST_TABLE 1
#endif

namespace {

int g_failed = 0, g_counted = 0;
const char* g_verbose = nullptr;
long g_allocations = 0;

bool Verbose(const char* row) { return g_verbose && std::strcmp(g_verbose, row) == 0; }

void Check(const char* row, bool pass, const char* what, const char* format, ...) {
    char numbers[400];
    va_list args;
    va_start(args, format);
    std::vsnprintf(numbers, sizeof(numbers), format, args);
    va_end(args);
    ++g_counted;
    if (!pass)
        ++g_failed;
    std::printf("%-3s %-5s %s | %s\n", row, pass ? "PASS" : "FAIL", what, numbers);
}

void Info(const char* row, const char* what, const char* format, ...) {
    char numbers[400];
    va_list args;
    va_start(args, format);
    std::vsnprintf(numbers, sizeof(numbers), format, args);
    va_end(args);
    std::printf("%-3s %-5s %s | %s\n", row, "INFO", what, numbers);
}

bool Near(float a, float b, float share) { return std::fabs(a - b) <= share * std::fabs(b); }

// ---- The guns (table 4.2) ----

recoil::Gun MakeGun(recoil::Kind kind, float rate, float up, float sideMin, float sideMax, float first, float decrease, float firing, float back,
                    float rise) {
    recoil::Gun g;
    g.kind = kind;
    g.rate = rate;
    for (int i = 0; i < 2; ++i)
        g.drift.up[i] = up, g.drift.sideMin[i] = sideMin, g.drift.sideMax[i] = sideMax, g.drift.decrease[i] = decrease;
    g.drift.firstShot = first, g.drift.shootingScale = firing;
    g.kick.back.peak = back, g.kick.rise.peak = rise;
    return g;
}

recoil::Gun M416() {
    recoil::Gun g = MakeGun(recoil::Kind::kRifle, 750, 0.26f, -0.3f, 0.1f, 2.0f, 18, 0.1f, 0.015f, 1.8f);
    g.rumble[0] = 0.2f, g.rumble[1] = 1.0f, g.rumble[2] = 0.12f;
    g.eyeFactor = 0.75f;
    return g;
}

recoil::Gun M240() {
    recoil::Gun g = MakeGun(recoil::Kind::kMg, 650, 0.7f, -0.5f, 0.5f, 1.7f, 12, 0.1f, 0.016f, 2.0f);
    g.rumble[0] = 1.0f, g.rumble[1] = 1.0f, g.rumble[2] = 0.2f;
    return g;
}

recoil::Gun M9() {
    recoil::Gun g = MakeGun(recoil::Kind::kPistol, 400, 0.7f, -0.1f, 0.1f, 1.0f, 20, 0.2f, 0.010f, 4.0f);
    g.drift.spring = 150.0f;
    g.kick.back.at = 0.025f, g.kick.back.damping = 0.8f, g.kick.rise.at = 0.035f, g.kick.rise.damping = 0.8f;
    g.rumble[0] = 0.39f, g.rumble[1] = 0.6f, g.rumble[2] = 0.2f;
    return g;
}

recoil::Gun Marksman(float rate, float up, float sideMin, float sideMax, float back, float rise) {   // the shotguns', marksman's and bolt guns' timing
    recoil::Gun g = MakeGun(recoil::Kind::kMarksman, rate, up, sideMin, sideMax, 1.0f, 6, 0.34f, back, rise);
    g.kick.back.at = 0.035f, g.kick.back.damping = 0.75f, g.kick.rise.at = 0.06f, g.kick.rise.damping = 0.7f;
    return g;
}

recoil::Gun M98B() {
    recoil::Gun g = Marksman(360, 2.0f, 0.0f, 0.0f, 0.021f, 2.6f);
    g.kind = recoil::Kind::kBolt;
    g.drift.up[1] = 0.5f, g.drift.decrease[0] = 15, g.drift.decrease[1] = 6, g.drift.shootingScale = 1.0f;
    g.eyeFactor = 0.5f;
    return g;
}

recoil::Gun AN94() {
    recoil::Gun g = MakeGun(recoil::Kind::kRifle, 1200, 0.3f, -0.3f, 0.2f, 1.5f, 18, 0.1f, 0.012f, 1.6f);
    g.pairGap = 0.075f;
    g.kick.burstFirst = 0.3f;
    g.rumble[0] = 0.5f, g.rumble[1] = 0.8f, g.rumble[2] = 0.15f;
    return g;
}

// The M416's real places: the right palm the game holds it by, the game's zoomed eye.
constexpr float kGrip[3] = {-0.063f, -0.091f, 0.224f};
constexpr float kEye[3] = {0.0f, 0.112f, 0.148f};

// ---- A scripted run ----

struct Sim {
    recoil::Gun gun;
    recoil::Hold hold;
    recoil::Tune tune;
    recoil::State s;
    recoil::Frame f;
    recoil::Output out;
    double fps = 72.0;
    long frame = 0;
    double shots[256] = {};
    int shotCount = 0, nextShot = 0;

    Sim(const recoil::Gun& g, float hands, uint32_t seed, double rate = 72.0) : gun(g), fps(rate) {
        hold.held = true;
        hold.hands = hands;
        f.seed = seed;
        std::memcpy(f.pivot, kGrip, sizeof(f.pivot));
    }
    double Start() const { return static_cast<double>(frame) / fps; }      // before Step: the coming frame's start
    double End() const { return static_cast<double>(frame + 1) / fps; }    // before Step: the coming frame's end
    double Now() const { return static_cast<double>(frame) / fps; }        // after Step: the end of the frame just stepped (what its output shows)
    void Burst(double from, int rounds, double gap) {
        for (int i = 0; i < rounds && shotCount < 256; ++i)
            shots[shotCount++] = from + gap * i;
    }
    // One frame: the rounds that left since its start, dated by Step at its start.
    const recoil::Output& Step() {
        f.dt = static_cast<float>(1.0 / fps);
        f.shots = 0;
        const double end = End();
        while (nextShot < shotCount && shots[nextShot] < end - 1e-9) {
            ++f.shots;
            ++nextShot;
        }
        out = recoil::Step(gun, hold, f, tune, s);
        f.seed = 0;
        f.newGun = false;
        ++frame;
        return out;
    }
    float Kick() const { return out.pitch - out.drift[0]; }   // B's rise alone (at the hip, no total-turn cap reached)
};

bool AllFinite(const recoil::Output& o) {
    const float* lists[] = {o.k, o.q};
    for (int i = 0; i < 12; ++i)
        if (!std::isfinite(lists[0][i]))
            return false;
    for (int i = 0; i < 4; ++i)
        if (!std::isfinite(lists[1][i]))
            return false;
    const float rest[] = {o.pitch, o.yaw, o.roll, o.drift[0], o.drift[1], o.back, o.lift, o.slack, o.pulse[0], o.pulse[1], o.pulseFor[0], o.pulseFor[1],
                          o.lateIn, o.late, o.lateFor};
    for (float v : rest)
        if (!std::isfinite(v))
            return false;
    return true;
}

float Orthonormal(const float* k) {   // the worst entry of R R^T - I
    float worst = 0.0f;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            const float d = k[i * 4] * k[j * 4] + k[i * 4 + 1] * k[j * 4 + 1] + k[i * 4 + 2] * k[j * 4 + 2] - (i == j ? 1.0f : 0.0f);
            worst = std::fmax(worst, std::fabs(d));
        }
    return worst;
}

void Apply(const float* k, const float* p, float* out) {
    for (int i = 0; i < 3; ++i)
        out[i] = k[i * 4] * p[0] + k[i * 4 + 1] * p[1] + k[i * 4 + 2] * p[2] + k[i * 4 + 3];
}

// A point's move under k, less its part along the kicked gun's own z (the slide): under the turn about that point, nothing.
float Across(const float* k, const float* p) {
    float moved[3];
    Apply(k, p, moved);
    const float d[3] = {moved[0] - p[0], moved[1] - p[1], moved[2] - p[2]};
    const float z[3] = {k[2], k[6], k[10]};
    const float along = d[0] * z[0] + d[1] * z[1] + d[2] * z[2];
    const float e[3] = {d[0] - along * z[0], d[1] - along * z[1], d[2] - along * z[2]};
    return std::sqrt(e[0] * e[0] + e[1] * e[1] + e[2] * e[2]);
}

// The eye's place along z in the kicked gun's own space (k's inverse on it).
float EyeInKicked(const float* k, const float* eye) {
    const float d[3] = {eye[0] - k[3], eye[1] - k[7], eye[2] - k[11]};
    return k[2] * d[0] + k[6] * d[1] + k[10] * d[2];
}

// ---- 1. One round ----

void Check1() {
    recoil::Gun g = M416();
    g.kick.jitter = 0.0f;
    Sim r(g, 1.0f, 11, 1000.0);
    r.Burst(0.0, 1, 0.0);
    float backPeak = 0, backAt = 0, risePeak = 0, riseAt = 0, backLeast = 0, target = 0, drawnPeak = 0, lateDrift = 0, lateBack = 0, lateRise = 0;
    bool targetSet = false;
    struct Row { double t; float back, rise; } rows[1600];
    int n = 0;
    while (r.frame < 1500) {
        r.f.trigger = r.Start() < 0.05;
        r.Step();
        const double t = r.Now();
        if (!targetSet) {
            target = r.s.upTarget;
            targetSet = true;
        }
        const float back = r.out.back, rise = r.Kick();
        if (back > backPeak)
            backPeak = back, backAt = static_cast<float>(t);
        if (rise > risePeak)
            risePeak = rise, riseAt = static_cast<float>(t);
        backLeast = std::fmin(backLeast, back);
        drawnPeak = std::fmax(drawnPeak, r.out.drift[0]);
        if (t >= 1.2)
            lateDrift = std::fmax(lateDrift, std::fabs(r.out.drift[0]));
        if (n < 1600)
            rows[n++] = {t, back, rise};
        if (Verbose("1"))
            std::printf("  %6.3f back %.5f rise %.4f drift %.4f target %.4f\n", t, back, rise, r.out.drift[0], r.s.upTarget);
    }
    for (int i = 0; i < n; ++i)
        if (rows[i].t >= 0.20)
            lateBack = std::fmax(lateBack, std::fabs(rows[i].back)), lateRise = std::fmax(lateRise, std::fabs(rows[i].rise));
    Check("1", Near(backPeak, 0.015f, 0.03f) && std::fabs(backAt - 0.030f) <= 0.0031f, "one M416 round: the back's peak",
          "%.2f cm at %.0f ms (1.5 cm at 30 ms)", backPeak * 100, backAt * 1000);
    Check("1", Near(risePeak, 1.8f, 0.03f) && std::fabs(riseAt - 0.050f) <= 0.0031f, "one M416 round: the rise's peak", "%.3f deg at %.0f ms (1.8 at 50)",
          risePeak, riseAt * 1000);
    Check("1", lateBack <= 0.05f * backPeak && lateRise <= 0.05f * risePeak, "both within 5% of their peak from 0.20 s",
          "back %.1f%%, rise %.1f%%", lateBack / backPeak * 100, lateRise / risePeak * 100);
    Check("1", backLeast >= -0.0005f, "the back never below -0.5 mm (forward)", "least %.2f mm", backLeast * 1000);
    Check("1", target >= 0.26f * 2 * 0.9f - 1e-5f && target <= 0.26f * 2 + 1e-5f, "A's target 0.26 x 2 x [0.9, 1.0]", "%.4f", target);
    Check("1", drawnPeak < 1.35f * target && lateDrift < 0.02f, "A as drawn under 1.35 x the target, under 0.02 from 1.2 s",
          "peak %.4f (%.2f x), %.4f from 1.2 s", drawnPeak, drawnPeak / target, lateDrift);
    // The drift's camera spring never drawn lighter than 0.35 (the critic's 14): the same round with the data's damping 0.2, or the M82A3's 0.34,
    // draws exactly as with 0.35.
    static float drawn[3][1500];
    const float dampings[3] = {0.35f, 0.2f, 0.34f};
    float peaks[3] = {};
    for (int k = 0; k < 3; ++k) {
        recoil::Gun gd = M416();
        gd.kick.jitter = 0.0f, gd.drift.damping = dampings[k];
        Sim d(gd, 1.0f, 11, 1000.0);
        d.Burst(0.0, 1, 0.0);
        for (int i = 0; i < 1500; ++i) {
            d.f.trigger = d.Start() < 0.05;
            d.Step();
            drawn[k][i] = d.out.drift[0];
            peaks[k] = std::fmax(peaks[k], d.out.drift[0]);
        }
    }
    const bool least = std::memcmp(drawn[0], drawn[1], sizeof(drawn[0])) == 0 && std::memcmp(drawn[0], drawn[2], sizeof(drawn[0])) == 0;
    Check("1", least, "the drift's camera spring never lighter than 0.35: the data's 0.2 and 0.34 drawn as 0.35", "%s (peaks %.4f, %.4f, %.4f)",
          least ? "bit for bit" : "NOT THE SAME", peaks[0], peaks[1], peaks[2]);
}

// ---- 2. Thirty rounds ----

void Check2() {
    Sim r(M416(), 1.0f, 2);
    r.Burst(0.0, 30, 0.08);
    float target30 = 0, mostTarget = 0, mostBack = 0, lastRoll = 0;
    double sumRise = 0, sumBack = 0;
    int samples = 0, rollChanges = 0, rollPairs = 0;
    while (r.End() < 3.5) {
        r.f.trigger = r.Start() < 2.35;
        const int before = r.nextShot;
        r.Step();
        mostTarget = std::fmax(mostTarget, r.s.upTarget);
        mostBack = std::fmax(mostBack, r.out.back);
        if (before < 30 && r.nextShot == 30)
            target30 = r.s.upTarget;
        if (r.nextShot > before) {   // (two hands: the roll to the other side each round, like the game's)
            if (before > 0)
                ++rollPairs, rollChanges += (r.s.struck[3] < 0) != (lastRoll < 0) ? 1 : 0;
            lastRoll = r.s.struck[3];
        }
        // Rounds 5-30: from the frame the 5th is in to the one before the 30th's.
        if (r.nextShot >= 5 && r.nextShot < 30) {
            sumRise += r.Kick();
            sumBack += r.out.back;
            ++samples;
        }
        if (Verbose("2"))
            std::printf("  %6.3f rounds %d target %.3f drift %.3f kick %.3f back %.2f cm held %.2f cm\n", r.Now(), r.nextShot, r.s.upTarget,
                        r.out.drift[0], r.Kick(), r.out.back * 100, r.s.held.x * 100);
    }
    const float meanRise = static_cast<float>(sumRise / samples), meanBack = static_cast<float>(sumBack / samples);
    Check("2", target30 >= 0.9f * 0.26f * 31 - 1e-4f && target30 <= 0.26f * 31 + 1e-4f && mostTarget <= 10.0f, "30 rounds: A's target at the 30th",
          "%.3f (7.25-8.06), most %.3f", target30, mostTarget);
    Check("2", std::fabs(meanRise - 2.3f) <= 0.2f, "B's mean rise over rounds 5-30", "%.3f deg (2.3)", meanRise);
    Check("2", meanBack >= 0.024f && meanBack <= 0.030f, "the back on average over rounds 5-30, held fire's push back with it (the critic's 8)",
          "%.2f cm (2.4-3.0)", meanBack * 100);
    Check("2", mostBack < 0.04f, "the back never 4 cm", "most %.2f cm", mostBack * 100);
    Check("2", rollPairs == 29 && rollChanges == rollPairs, "two hands: the roll to the other side every round", "%d of %d rounds", rollChanges, rollPairs);
}

// ---- 3. The M240's cap ----

void Check3() {
    Sim r(M240(), 1.0f, 3);
    const double gap = 60.0 / 650.0;
    r.Burst(0.0, 50, gap);
    int capRound = 0;
    bool exact = true;
    while (r.End() < 50 * gap + 0.02) {
        r.f.trigger = true;
        r.Step();
        if (!capRound && r.s.upTarget >= 10.0f - 1e-6f)
            capRound = r.nextShot;
        if (capRound && r.nextShot < 50 && r.s.upTarget != 10.0f)
            exact = false;
    }
    Check("3", capRound >= 14 && capRound <= 16 && exact, "the M240's 50 rounds: A's target at the cap", "10.0 at round %d (15 +-1), then %s",
          capRound, exact ? "exactly 10.0" : "not exactly 10.0");
    // One hand (design 3.4): the caps x1.5, so a long string stops at 15.
    Sim one(M416(), 0.0f, 3);
    one.Burst(0.0, 40, 0.08);
    int oneRound = 0;
    bool oneExact = true;
    float most = 0;
    while (one.End() < 40 * 0.08 + 0.01) {
        one.f.trigger = true;
        one.Step();
        most = std::fmax(most, one.s.upTarget);
        if (!oneRound && one.s.upTarget >= 15.0f - 1e-6f)
            oneRound = one.nextShot;
        if (oneRound && one.nextShot < 40 && one.s.upTarget != 15.0f)
            oneExact = false;
    }
    Check("3", oneRound > 0 && oneExact && most == 15.0f, "one hand, 40 M416 rounds: A's target stops at its cap x1.5", "15.0 at round %d, then %s (most %.3f)",
          oneRound, oneExact ? "exactly 15.0" : "NOT exactly 15.0", most);
}

// ---- 4. 72, 60 and 36 fps ----

struct Curve { double t[4000]; float v[4000][6]; int n = 0; };

void RunCurve(const recoil::Gun& g, double fps, int rounds, double gap, double length, Curve& c) {
    Sim r(g, 1.0f, 44, fps);
    r.Burst(0.0, rounds, gap);
    c.n = 0;
    while (r.End() < length + 1e-9 && c.n < 4000) {
        r.f.trigger = r.Start() < rounds * gap;
        r.Step();
        c.t[c.n] = r.Now();
        const float v[6] = {r.Kick(), r.out.back, r.out.drift[0], r.out.drift[1], r.out.yaw - r.out.drift[1], r.out.roll};
        std::memcpy(c.v[c.n], v, sizeof(v));
        ++c.n;
    }
}

float Worst(const Curve& c, const Curve& ref) {   // the worst difference against the reference at the same moments, as a share of each curve's peak
    float peak[6] = {};
    for (int i = 0; i < ref.n; ++i)
        for (int k = 0; k < 6; ++k)
            peak[k] = std::fmax(peak[k], std::fabs(ref.v[i][k]));
    float worst = 0.0f;
    int j = 0;
    for (int i = 0; i < c.n; ++i) {
        while (j < ref.n && ref.t[j] < c.t[i] - 1e-7)
            ++j;
        if (j >= ref.n || std::fabs(ref.t[j] - c.t[i]) > 1e-7)
            continue;
        for (int k = 0; k < 6; ++k)
            if (peak[k] > 1e-6f)
                worst = std::fmax(worst, std::fabs(c.v[i][k] - ref.v[j][k]) / peak[k]);
    }
    return worst;
}

void Check4() {
    static Curve ref, c;
    recoil::Gun one = M416();
    one.kick.jitter = 0.0f;
    struct Case { const char* name; recoil::Gun gun; int rounds; double length; } cases[] = {
        {"row 1's round", one, 1, 1.5}, {"row 2's 30 rounds", M416(), 30, 3.5}, {"row 3's 50 M240 rounds", M240(), 50, 5.0}};
    const double rates[] = {72.0, 60.0, 36.0};
    for (const Case& k : cases) {
        RunCurve(k.gun, 360.0, k.rounds, 1.0 / 12.0, k.length, ref);
        float worst = 0.0f;
        char text[200] = {};
        int at = 0;
        for (double fps : rates) {
            RunCurve(k.gun, fps, k.rounds, 1.0 / 12.0, k.length, c);
            const float w = Worst(c, ref);
            worst = std::fmax(worst, w);
            at += std::snprintf(text + at, sizeof(text) - at, "%s%.0f fps %.2f%%", at ? ", " : "", fps, w * 100);
        }
        char what[100];
        std::snprintf(what, sizeof(what), "%s at 72, 60 and 36 fps against 360 fps", k.name);
        Check("4", worst <= 0.02f, what, "%s (2%% of each curve's peak)", text);
    }
}

// ---- 5. The string's recovery ----

void Check5() {
    // Two hands, the button held on or let go; then one hand (design 3.4: its recovery x0.5), let go.
    for (int run = 0; run < 3; ++run) {
        const bool held = run == 1, oneHand = run == 2;
        const int rounds = oneHand ? 20 : 30;   // (one hand's 20 stay under its cap of 15)
        const double scale = oneHand ? 0.5 : 1.0;
        Sim r(M416(), oneHand ? 0.0f : 1.0f, 5);
        r.Burst(0.0, rounds, 0.08);
        double last = -1;
        float atLast = 0, worst = 0;
        const double lastAt = 0.08 * (rounds - 1);
        while (r.End() < lastAt + 1.3) {
            r.f.trigger = held ? r.Start() < lastAt + 0.5 : r.Start() < lastAt + 0.01;
            const int before = r.nextShot;
            const double start = r.Start();
            r.Step();
            if (before < rounds && r.nextShot == rounds)
                last = start, atLast = r.s.upTarget;   // (the round dated at its frame's start)
            if (last >= 0 && r.nextShot == rounds && !(before < rounds)) {
                const double t = r.Now() - last, gap = 0.08;
                double want = atLast;
                if (t > 1.25 * gap)
                    want -= 18.0 * 0.1 * scale * (std::fmin(t, 2.0 * gap) - 1.25 * gap);
                if (t > 2.0 * gap)
                    want -= 18.0 * scale * (t - 2.0 * gap);
                want = std::fmax(want, 0.0);
                worst = std::fmax(worst, static_cast<float>(std::fabs(r.s.upTarget - want)));
                if (Verbose("5"))
                    std::printf("  %s +%.3f target %.4f closed form %.4f\n", oneHand ? "one" : held ? "held" : "two", t, r.s.upTarget, want);
            }
        }
        Check("5", worst <= 0.05f && atLast > 5.0f,
              oneHand ? "one hand: the recovery after the string at half the rate" : held ? "the recovery after the string, the button held 0.5 s"
                                                                                    : "the recovery after the string, the button let go",
              "worst %.4f deg off the closed form from %.2f (held 1.25 gaps, %.2f deg/s to 2 gaps, then %.0f)", worst, atLast, 1.8 * scale, 18.0 * scale);
    }
}

// ---- 6. The first shot ----

float Increment(Sim& r) {   // A's up the next frame's round adds, as a share of the row's up
    const float before = r.s.upTarget;
    r.Step();
    return (r.s.upTarget - before) / 0.26f;
}

void Check6() {
    {
        // The second pull's round 0.12 s after the first: sooner than the time rule's 1.5 x 80 ms + 50 ms (0.17 s), so only the release between
        // makes it a first shot.
        Sim r(M416(), 1.0f, 6);
        r.shots[r.shotCount++] = 0.0;
        r.shots[r.shotCount++] = 0.12;
        float first = 0, second = 0;
        while (r.End() < 0.5) {
            r.f.trigger = r.Start() < 0.04 || (r.Start() >= 0.1 && r.Start() < 0.16);
            const int before = r.nextShot;
            const float inc = Increment(r);
            if (before == 0 && r.nextShot == 1)
                first = inc;
            if (before == 1 && r.nextShot == 2)
                second = inc;
        }
        Check("6", first >= 1.8f - 1e-4f && first <= 2.0f + 1e-4f && second >= 1.8f - 1e-4f && second <= 2.0f + 1e-4f,
              "a pull, a release, a pull 0.12 s later (before the time rule's 0.17 s): its round a first shot again", "x%.3f, then x%.3f (1.8-2.0)", first,
              second);
    }
    {
        Sim r(M416(), 1.0f, 7);
        r.Burst(0.0, 13, 0.08);
        float first = 0, least = 9, most = 0;
        while (r.End() < 1.1) {
            r.f.trigger = true;
            const int before = r.nextShot;
            const float inc = Increment(r);
            if (r.nextShot > before) {
                if (before == 0)
                    first = inc;
                else
                    least = std::fmin(least, inc), most = std::fmax(most, inc);
            }
        }
        Check("6", first >= 1.8f - 1e-4f && least >= 0.9f - 1e-4f && most <= 1.0f + 1e-4f, "held 1 s with rounds: only the first a first shot",
              "first x%.3f, the rest x%.3f-%.3f", first, least, most);
    }
    {
        Sim r(M416(), 1.0f, 8);
        r.shots[r.shotCount++] = 0.0;
        r.shots[r.shotCount++] = 0.25;   // 0.25 s later, the button held all along: past 1.5 x 80 ms + 50 ms
        float second = 0;
        while (r.End() < 0.5) {
            r.f.trigger = true;
            const int before = r.nextShot;
            const float inc = Increment(r);
            if (before == 1 && r.nextShot == 2)
                second = inc;
        }
        Check("6", second >= 1.8f - 1e-4f, "held, no round for 0.25 s (the time rule alone): the next a first shot again", "x%.3f", second);
    }
}

// ---- 7. One hand, 20 seeds ----

void Check7() {
    float leastRise = 99, mostRise = 0, mostPitch = 0, mostBack = 0, mostTurn = 0, mostYaw = 0, rollLeast = 9, rollMost = 0;
    float firstLeast = 99, firstMost = 0, restLeast = 99, restMost = 0, sideLeast = 99, sideMost = -99;
    int changes = 0, rollChanges = 0, pairs = 0, low = 0, high = 0, wide = 0, firsts = 0, rounds = 0;
    bool finite = true;
    for (uint32_t seed = 1; seed <= 40; ++seed) {
        Sim r(M416(), 0.0f, seed);
        r.Burst(0.0, 10, 0.08);
        float lastYaw = 0, lastRoll = 0;
        int n = 0;
        while (r.End() < 1.8) {
            r.f.trigger = r.Start() < 0.8;
            const int before = r.nextShot;
            const float upBefore = r.s.upTarget, sideBefore = r.s.sideTarget;
            r.Step();
            if (r.nextShot > before) {   // (a round a frame at most: 80 ms apart; inside the string nothing comes back)
                const float rise = r.s.struck[1], size = rise / 9.0f;
                leastRise = std::fmin(leastRise, rise), mostRise = std::fmax(mostRise, rise);
                low += size < 0.6f ? 1 : 0, high += size > 1.4f ? 1 : 0;
                const float yaw = std::fabs(r.s.struck[2] / rise), roll = std::fabs(r.s.struck[3] / rise);
                mostYaw = std::fmax(mostYaw, yaw), wide += yaw > 0.6f ? 1 : 0;
                rollLeast = std::fmin(rollLeast, roll), rollMost = std::fmax(rollMost, roll);
                const float up = r.s.upTarget - upBefore, side = r.s.sideTarget - sideBefore;
                if (n == 0)
                    firstLeast = std::fmin(firstLeast, up), firstMost = std::fmax(firstMost, up), ++firsts;
                else
                    restLeast = std::fmin(restLeast, up), restMost = std::fmax(restMost, up);
                sideLeast = std::fmin(sideLeast, side), sideMost = std::fmax(sideMost, side);
                if (n > 0) {
                    ++pairs;
                    changes += (r.s.struck[2] < 0) != (lastYaw < 0) ? 1 : 0;
                    rollChanges += (r.s.struck[3] < 0) != (lastRoll < 0) ? 1 : 0;
                }
                lastYaw = r.s.struck[2], lastRoll = r.s.struck[3];
                ++n, ++rounds;
            }
            mostPitch = std::fmax(mostPitch, std::fabs(r.s.pitch.x));
            mostBack = std::fmax(mostBack, r.out.back);
            mostTurn = std::fmax(mostTurn, std::sqrt(r.out.pitch * r.out.pitch + r.out.yaw * r.out.yaw));
            finite = finite && AllFinite(r.out);
            if (Verbose("7") && seed == 1)
                std::printf("  %6.3f pitch %.2f yaw %.2f roll %.2f back %.2f cm drift %.2f %.2f\n", r.Now(), r.out.pitch, r.out.yaw, r.out.roll,
                            r.out.back * 100, r.out.drift[0], r.out.drift[1]);
        }
    }
    Check("7", leastRise >= 0.5f * 9 * 0.999f && mostRise <= 1.5f * 9 * 1.001f && low > 0 && high > 0,
          "one hand: each round's own rise in [0.5, 1.5] x 9, reaching both ends", "%.2f-%.2f deg (4.5-13.5) over %d rounds; %d under 0.6 x 9, %d over 1.4 x 9",
          leastRise, mostRise, rounds, low, high);
    const float share = static_cast<float>(changes) / static_cast<float>(pairs);
    Check("7", share >= 0.3f && share <= 0.7f && mostYaw <= 0.8f + 1e-5f && wide > 0, "one hand: the yaw within 0.8 of the rise either way, its side at random",
          "at most %.3f of it, past 0.6 in %d rounds; its side changing in %d of %d (%.0f%%; 30-70%%)", mostYaw, wide, changes, pairs, share * 100);
    const float rollShare = static_cast<float>(rollChanges) / static_cast<float>(pairs);
    Check("7", rollLeast >= 0.6f - 1e-5f && rollMost <= 0.6f + 1e-5f && rollShare >= 0.3f && rollShare <= 0.7f,
          "one hand: the roll 0.6 of the rise, to a side at random (not every other round)", "%.4f-%.4f of it; its side changing in %d of %d (%.0f%%; 30-70%%)",
          rollLeast, rollMost, rollChanges, pairs, rollShare * 100);
    Check("7", firsts > 0 && firstLeast >= 1.404f - 1e-4f && firstMost <= 1.56f + 1e-4f && restLeast >= 0.468f - 1e-4f && restMost <= 0.52f + 1e-4f,
          "one hand: A's first round 0.26 x 2 x 2 x 1.5 x [0.9, 1], the rest 0.26 x 2 x [0.9, 1]", "the first %.3f-%.3f (1.40-1.56), the rest %.3f-%.3f (0.47-0.52)",
          firstLeast, firstMost, restLeast, restMost);
    Check("7", sideLeast >= -1.16f - 1e-4f && sideMost <= 0.56f + 1e-4f && sideLeast < -0.92f && sideMost > 0.37f,
          "one hand: A's sideways 3 x -0.3..0.1 and a toss of up to 0.26 either way a round", "%.3f-%.3f (-1.16-0.56; past -0.92 and 0.37 only with both)",
          sideLeast, sideMost);
    Check("7", mostPitch <= 20.0f + 1e-4f && mostBack < 0.06f + 1e-6f && mostTurn <= 30.0f + 1e-3f && finite,
          "one hand: the rise never past 20, the back never 6 cm, the turn never 30, all finite", "rise %.2f, back %.2f cm, turn %.2f, %s", mostPitch,
          mostBack * 100, mostTurn, finite ? "finite" : "NOT FINITE");
}

// ---- 8, 9. One hand against two; the M9 ----

void Peaks(const recoil::Gun& g, float hands, uint32_t seed, float& rise, float& back, float* lateRise = nullptr, float* lateBack = nullptr,
           double from = 0.15) {
    Sim r(g, hands, seed, 1000.0);
    r.Burst(0.0, 1, 0.0);
    rise = back = 0;
    float late[2] = {};
    while (r.frame < 1000) {
        r.f.trigger = r.Start() < 0.05;
        r.Step();
        rise = std::fmax(rise, r.Kick()), back = std::fmax(back, r.out.back);
        if (r.Now() >= from)
            late[0] = std::fmax(late[0], std::fabs(r.Kick())), late[1] = std::fmax(late[1], std::fabs(r.out.back));
    }
    if (lateRise)
        *lateRise = late[0];
    if (lateBack)
        *lateBack = late[1];
}

void Check8() {
    recoil::Gun g = M416();
    g.kick.jitter = 0.0f;
    float rise1, back1, rise2, back2;
    Peaks(g, 0.0f, 9, rise1, back1);
    Peaks(g, 1.0f, 9, rise2, back2);
    Check("8", std::fabs(rise1 / rise2 - 5.0f) <= 0.5f && std::fabs(back1 / back2 - 2.5f) <= 0.25f, "one hand against two: the first round",
          "rise %.2f / %.2f = %.2f (5.0), back %.2f / %.2f cm = %.2f (2.5)", rise1, rise2, rise1 / rise2, back1 * 100, back2 * 100, back1 / back2);
    // One hand's own timing (design 3.3, 3.4): slower, and swinging past the line (a damping of 0.4 comes back 25% past it).
    {
        Sim r(g, 0.0f, 9, 1000.0);
        r.Burst(0.0, 1, 0.0);
        float rise = 0, riseAt = 0, back = 0, backAt = 0, under = 0;
        while (r.frame < 1000) {
            r.f.trigger = r.Start() < 0.05;
            r.Step();
            const float t = static_cast<float>(r.Now());
            if (r.Kick() > rise)
                rise = r.Kick(), riseAt = t;
            if (r.out.back > back)
                back = r.out.back, backAt = t;
            under = std::fmin(under, r.Kick());
        }
        const float past = -under / rise;
        Check("8", std::fabs(riseAt - 0.070f) <= 0.0031f && std::fabs(backAt - 0.045f) <= 0.0031f && past >= 0.20f && past <= 0.30f,
              "one hand's own timing: the rise at its peak at 70 ms and past the line after, the back at its peak at 45 ms",
              "the rise %.0f ms, %.0f%% past the line after (20-30%%); the back %.0f ms", riseAt * 1000, past * 100, backAt * 1000);
    }
    // A launcher's lift as its rocket leaves: 1.5 times with one hand.
    {
        recoil::Gun rocket = MakeGun(recoil::Kind::kRocket, 350, 2.5f, -1.0f, -0.5f, 1.0f, 15, 1.0f, 0.008f, 1.5f);
        rocket.kick.lift.peak = 0.015f, rocket.kick.jitter = 0.0f;
        float lift[2] = {};
        for (int hands = 0; hands < 2; ++hands) {
            Sim r(rocket, static_cast<float>(hands), 9, 1000.0);
            r.Burst(0.0, 1, 0.0);
            while (r.frame < 800) {
                r.f.trigger = r.Start() < 0.05;
                r.Step();
                lift[hands] = std::fmax(lift[hands], r.out.lift);
            }
        }
        Check("8", Near(lift[0] / lift[1], 1.5f, 0.01f) && Near(lift[1], 0.015f, 0.02f), "a launcher's lift: one hand against two",
              "%.2f / %.2f cm = %.3f (1.5)", lift[0] * 100, lift[1] * 100, lift[0] / lift[1]);
    }
}

void Check9() {
    recoil::Gun g = M9();
    g.kick.jitter = 0.0f;
    for (int hands = 0; hands < 2; ++hands) {
        float rise, back, lateRise, lateBack;
        Peaks(g, static_cast<float>(hands), 10, rise, back, &lateRise, &lateBack);
        const float wantRise = hands ? 4.0f : 6.0f, wantBack = hands ? 0.010f : 0.015f;
        Check("9", Near(rise, wantRise, 0.03f) && Near(back, wantBack, 0.03f), hands ? "the M9 in two hands" : "the M9 in one hand",
              "%.2f deg (%.1f), %.2f cm (%.1f)", rise, wantRise, back * 100, wantBack * 100);
        Check("9", lateRise <= 0.05f * rise && lateBack <= 0.05f * back, hands ? "the M9 in two hands: home by 0.15 s" : "the M9 in one hand: home by 0.15 s",
              "rise %.1f%%, back %.1f%% of the peak", lateRise / rise * 100, lateBack / back * 100);
    }
    // A (design 3.4): a pistol keeps its own drift in one hand, two hands take it down to 0.8 (the M9's first shot x1).
    for (int hands = 0; hands < 2; ++hands) {
        float least = 99, most = 0;
        for (uint32_t seed = 1; seed <= 10; ++seed) {
            Sim r(M9(), static_cast<float>(hands), seed);
            r.Burst(0.0, 1, 0.0);
            r.f.trigger = true;
            r.Step();
            least = std::fmin(least, r.s.upTarget), most = std::fmax(most, r.s.upTarget);
        }
        const float scale = hands ? 0.8f : 1.0f;
        Check("9", least >= 0.7f * 0.9f * scale - 1e-5f && most <= 0.7f * scale + 1e-5f, hands ? "the M9 in two hands: A a round x0.8" : "the M9 in one hand: A a round its own",
              "%.3f-%.3f (%.3f-%.3f)", least, most, 0.7f * 0.9f * scale, 0.7f * scale);
    }
    // Each round's size (design 3.4): 1 -+ 0.2 in one hand, 0.1 in two.
    for (int hands = 0; hands < 2; ++hands) {
        Sim r(M9(), static_cast<float>(hands), 31);
        r.Burst(0.0, 60, 0.2);
        const float peak = hands ? 4.0f : 6.0f;
        float most = 0;
        while (r.End() < 12.2) {
            r.f.trigger = r.nextShot < 60;
            const int before = r.nextShot;
            r.Step();
            if (r.nextShot > before)
                most = std::fmax(most, std::fabs(r.s.struck[1] / peak - 1.0f));
        }
        const float jitter = hands ? 0.1f : 0.2f;
        Check("9", most <= jitter + 1e-5f && most > 0.75f * jitter, hands ? "the M9 in two hands: each round's size within 1 -+ 0.1" : "the M9 in one hand: each round's size within 1 -+ 0.2",
              "most %.3f off 1 over 60 rounds (%.2f, past %.3f)", most, jitter, 0.75f * jitter);
    }
    // At the eye with two hands (3.5): a pistol keeps the wrist's turn and its own numbers (no sight factor).
    {
        recoil::Gun g9 = M9();
        g9.kick.jitter = 0.0f, g9.eyeFactor = 0.5f;
        const float grip[3] = {0.0f, -0.035f, 0.03f}, eye[3] = {0.0f, 0.03f, -0.30f};
        Sim r(g9, 1.0f, 10, 1000.0);
        std::memcpy(r.f.pivot, grip, sizeof(grip));
        std::memcpy(r.f.eyeAt, eye, sizeof(eye));
        r.f.haveEye = true, r.hold.eye = true;
        r.Burst(0.3, 1, 0.0);
        float rise = 0, across = 0;
        while (r.frame < 900) {
            r.f.trigger = r.Start() >= 0.3 && r.Start() < 0.35;
            r.Step();
            rise = std::fmax(rise, r.Kick()), across = std::fmax(across, Across(r.out.k, grip));
        }
        Check("9", across < 0.0001f && Near(rise, 4.0f, 0.03f) && r.s.eye > 0.99f, "the M9 at the eye in two hands: still about the grip, its own rise",
              "the grip's point across the line %.4f mm, the rise %.2f deg (4.0, its sight's 0.5 not taken)", across * 1000, rise);
    }
    // The caps (3.4): a pistol of 9 degrees and 2 cm a round stops at 10 and 2.5 cm in one hand (x1.5: 13.5 and 3), at 7 and 2 cm in two.
    {
        recoil::Gun big = M9();
        big.kick.jitter = 0.0f, big.kick.rise.peak = 9.0f, big.kick.back.peak = 0.02f;
        for (int hands = 0; hands < 2; ++hands) {
            float rise, back;
            Peaks(big, static_cast<float>(hands), 10, rise, back);
            const float capRise = hands ? 7.0f : 10.0f, capBack = hands ? 0.02f : 0.025f;
            Check("9", std::fabs(rise - capRise) < 1e-3f && std::fabs(back - capBack) < 2e-5f, hands ? "a 9-degree pistol in two hands: at its caps" : "a 9-degree pistol in one hand: at its caps",
                  "%.3f deg (%.0f), %.3f cm (%.1f)", rise, capRise, back * 100, capBack * 100);
        }
    }
}

// ---- 10. At the eye ----

// forward: the eyepiece's (on the sights' line at the glass) most forward move along the gun's z, away from the eye; nearer: the eye's most along
// the kicked gun's z past where it is as held (the glass that much nearer it).
struct EyeRun { float rise = 0, slide = 0, across = 0, acrossGrip = 0, forward = -1, nearer = -1; };

EyeRun AtEye(const recoil::Gun& gun, float hands, float glassAhead, bool haveGlass, uint32_t seed, int rounds = 1) {
    recoil::Gun g = gun;
    g.kick.jitter = 0.0f;
    Sim r(g, hands, seed, 1000.0);
    r.hold.eye = true;
    std::memcpy(r.f.eyeAt, kEye, sizeof(kEye));
    r.f.haveEye = true;
    r.f.glassZ = kEye[2] + glassAhead, r.f.haveGlass = haveGlass;
    r.Burst(0.3, rounds, 0.08);   // the eye eased in first
    const float eyepiece[3] = {kEye[0], kEye[1], kEye[2] + glassAhead};
    const long frames = 1300 + 80 * (rounds - 1);   // (1 s after the last round)
    EyeRun e;
    while (r.frame < frames) {
        r.f.trigger = r.Start() >= 0.3 && r.Start() < 0.35 + 0.08 * rounds;
        r.Step();
        if (r.Now() <= 0.3 + 1e-9)
            continue;
        float moved[3];
        Apply(r.out.k, eyepiece, moved);
        e.rise = std::fmax(e.rise, r.Kick());
        e.slide = std::fmax(e.slide, r.out.back);
        e.across = std::fmax(e.across, Across(r.out.k, kEye));
        e.acrossGrip = std::fmax(e.acrossGrip, Across(r.out.k, kGrip));
        e.forward = std::fmax(e.forward, moved[2] - eyepiece[2]);
        e.nearer = std::fmax(e.nearer, EyeInKicked(r.out.k, kEye) - kEye[2]);
        if (Verbose("10"))
            std::printf("  %6.3f rise %.3f slide %.2f mm eye across %.4f mm grip across %.4f mm eyepiece forward %.3f mm\n", r.Now(), r.Kick(), r.out.back * 1000,
                        Across(r.out.k, kEye) * 1000, Across(r.out.k, kGrip) * 1000, (moved[2] - eyepiece[2]) * 1000);
    }
    return e;
}
EyeRun AtEye(float hands, float glassAhead, bool haveGlass, uint32_t seed, int rounds = 1) { return AtEye(M416(), hands, glassAhead, haveGlass, seed, rounds); }

void Check10() {
    const EyeRun two = AtEye(1.0f, 0.035f, true, 12);
    Check("10", two.across < 0.0001f, "at the eye, two hands: the eye's point moves only along the gun's line", "across it %.4f mm (0.1)",
          two.across * 1000);
    Check("10", Near(two.rise, 1.35f, 0.03f), "at the eye, two hands: the rise x the ACOG's 0.75", "%.3f deg (1.35)", two.rise);
    Check("10", two.slide <= 0.0045f * 1.03f && two.slide <= 0.006f, "at the eye, two hands: the slide toward the eye", "%.2f mm (4.5, never 6)",
          two.slide * 1000);
    const EyeRun glass2 = AtEye(1.0f, 0.02f, true, 12), glass17 = AtEye(1.0f, 0.017f, true, 12);
    Check("10", glass2.slide <= 0.005f + 1e-6f && glass17.slide <= 0.002f + 1e-6f, "the glass 2 cm and 1.7 cm in front of the eye: the slide",
          "%.2f mm (5), %.2f mm (2)", glass2.slide * 1000, glass17.slide * 1000);
    const EyeRun one = AtEye(0.0f, 0.035f, true, 12);
    Check("10", one.acrossGrip < 0.0001f && one.across > 0.001f, "one hand at the eye: the turn about the grip again",
          "the grip's point across the line %.4f mm, the eye's %.1f mm", one.acrossGrip * 1000, one.across * 1000);
    // The sight at the eye eased in over 0.1 s and out over 0.2 (3.5).
    {
        Sim r(M416(), 1.0f, 12);
        std::memcpy(r.f.eyeAt, kEye, sizeof(kEye));
        r.f.haveEye = true, r.hold.eye = true;
        r.Step();
        const float firstIn = r.s.eye;
        int in = 1, out = 0;
        while (r.s.eye < 1.0f && in < 50)
            r.Step(), ++in;
        r.hold.eye = false;
        r.Step();
        const float firstOut = r.s.eye;
        out = 1;
        while (r.s.eye > 0.0f && out < 50)
            r.Step(), ++out;
        Check("10", Near(firstIn, 1.0f / 7.2f, 0.01f) && in == 8 && Near(firstOut, 1.0f - 1.0f / 14.4f, 0.001f) && out == 15,
              "the sight at the eye eased in over 0.1 s, out over 0.2 s", "a frame in %.3f, all in after %d frames (8); a frame out %.3f, all out after %d (15)",
              firstIn, in, firstOut, out);
    }
    // Irons (their factor 1) and a back of 1.69 cm (the G3A3's), two hands: 0.4 of it toward the eye is 6.8 mm, held at 6.
    {
        recoil::Gun irons = M416();
        irons.eyeFactor = 1.0f, irons.kick.back.peak = 0.0169f;
        const EyeRun e = AtEye(irons, 1.0f, 0.035f, false, 12);
        Check("10", std::fabs(e.slide - 0.006f) < 1e-6f, "irons and a 1.69 cm back at the eye: the slide toward the eye held at 6 mm", "%.3f mm (6.76 held at 6)",
              e.slide * 1000);
    }
    // The glass already within 1.5 cm of the eye as held (1 cm): the kick never brings it nearer, nor (two hands) moves the gun 1 mm forward.
    {
        const EyeRun two1 = AtEye(1.0f, 0.01f, true, 12), one1 = AtEye(0.0f, 0.01f, true, 12);
        Check("10", two1.nearer <= 1e-6f && one1.nearer <= 1e-6f && two1.forward < 0.001f, "the glass 1 cm in front of the eye as held: never nearer it, never 1 mm forward",
              "nearer by %.4f mm (two hands), %.4f mm (one); the eyepiece %.3f mm forward at most (two hands)", two1.nearer * 1000, one1.nearer * 1000,
              two1.forward * 1000);
    }
    // 30 rounds at the eye, two hands: the eyepiece never 1 mm forward, away from the eye (the critic's F5).
    {
        const EyeRun thirty = AtEye(1.0f, 0.035f, true, 12, 30);
        Check("10", thirty.forward < 0.001f && thirty.nearer <= 0.006f + 1e-6f, "30 rounds at the eye, two hands: the eyepiece never 1 mm forward",
              "%.3f mm forward at most; toward the eye %.2f mm at most (6)", thirty.forward * 1000, thirty.nearer * 1000);
    }
    // The support hand letting go at the eye 0.15 s after 30 rounds (the drift still up 7 degrees; its blend to 0 in a frame): the turn about the
    // eye eases to the grip's over 0.2 s, so no frame jumps the gun by the pivot's move.
    {
        recoil::Gun g = M416();
        g.kick.jitter = 0.0f;
        Sim r(g, 1.0f, 12);
        std::memcpy(r.f.eyeAt, kEye, sizeof(kEye));
        r.f.haveEye = true, r.hold.eye = true, r.f.glassZ = kEye[2] + 0.035f, r.f.haveGlass = true;
        r.Burst(0.3, 30, 0.08);
        const double lastAt = 0.3 + 0.08 * 29, goAt = lastAt + 0.15;
        const float eyepiece[3] = {kEye[0], kEye[1], kEye[2] + 0.035f};
        float was[3] = {eyepiece[0], eyepiece[1], eyepiece[2]}, before = 0, after = 0, firstPivot = -1, drift = 0;
        int easing = 0;
        bool gone = false;
        while (r.End() < goAt + 0.6) {
            r.f.trigger = r.Start() >= 0.3 && r.Start() < lastAt + 0.01;
            const bool let = r.Start() >= goAt;
            r.hold.hands = let ? 0.0f : 1.0f;
            r.Step();
            float moved[3];
            Apply(r.out.k, eyepiece, moved);
            const float step = std::sqrt((moved[0] - was[0]) * (moved[0] - was[0]) + (moved[1] - was[1]) * (moved[1] - was[1]) + (moved[2] - was[2]) * (moved[2] - was[2]));
            std::memcpy(was, moved, sizeof(was));
            if (!let && r.Start() > lastAt + 0.05)
                before = std::fmax(before, step);
            if (let) {
                if (firstPivot < 0)
                    firstPivot = r.s.eyePivot, drift = r.out.drift[0];
                after = std::fmax(after, step);
                easing += r.s.eyePivot > 0.0f ? 1 : 0;
                gone = r.s.eyePivot == 0.0f;
            }
        }
        Check("10", Near(firstPivot, 1.0f - 1.0f / 14.4f, 0.001f) && easing == 14 && gone && after < 0.004f,
              "the support hand letting go at the eye mid-kick: the turn about the eye eased to the grip's",
              "a frame on %.3f of the eye's, the grip's after %d frames (15); the eyepiece at most %.2f mm a frame (4; %.2f before), the drift %.1f deg",
              firstPivot, easing + 1, after * 1000, before * 1000, drift);
    }
}

// ---- 11. The M98B's rows ----

void Check11() {
    for (int run = 0; run < 3; ++run) {
        // The hip, the eye with two hands, the eye with one (its own aimed row too, with one hand's x2, its first x1.5 and half the recovery).
        const int eye = run > 0 ? 1 : 0;
        const bool oneHand = run == 2;
        Sim r(M98B(), oneHand ? 0.0f : 1.0f, 13);
        r.hold.eye = eye != 0;
        std::memcpy(r.f.eyeAt, kEye, sizeof(kEye));
        r.f.haveEye = true;
        r.Burst(0.4, 1, 0.0);
        float target = 0, after = -1, later = 0;
        double at = 0;
        while (r.End() < 1.2) {
            r.f.trigger = r.Start() >= 0.4 && r.Start() < 0.45;
            const int before = r.nextShot;
            const double start = r.Start();
            r.Step();
            if (before == 0 && r.nextShot == 1)
                target = r.s.upTarget, at = start;
            if (at > 0 && after < 0 && r.Now() - at >= 0.25)
                after = r.s.upTarget, later = static_cast<float>(r.Now() - at);
        }
        // A quarter second after the round: held for 1.25 gaps, then down at the row's own decrease (its x firing is 1).
        const float up = (eye ? 0.5f : 2.0f) * (oneHand ? 2.0f * 1.5f : 1.0f), decrease = (eye ? 6.0f : 15.0f) * (oneHand ? 0.5f : 1.0f);
        const float gap = 60.0f / 360.0f;
        const float want = std::fmax(target - decrease * (later - 1.25f * gap), 0.0f);
        Check("11", target >= 0.9f * up - 1e-4f && target <= up + 1e-4f && std::fabs(after - want) <= 0.02f,
              oneHand ? "the M98B at the eye in one hand: its zoomed row too" : eye ? "the M98B at the eye: its zoomed row" : "the M98B at the hip: its own row",
              "a round %.3f (%.1f x 0.9-1.0), %.3f s later %.3f (%.3f: %.1f a second)", target, up, later, after, want, decrease);
    }
}

// ---- 12. The clears ----

bool AtDefaults(const recoil::State& s, bool fading) {
    recoil::State d;
    return s.upTarget == d.upTarget && s.sideTarget == d.sideTarget && s.up.x == 0 && s.up.v == 0 && s.side.x == 0 && s.back.x == 0 && s.back.v == 0 &&
           s.pitch.x == 0 && s.pitch.v == 0 && s.yaw.x == 0 && s.roll.x == 0 && s.lift.x == 0 && s.held.x == 0 && s.heldTarget == 0 && s.since == d.since &&
           s.released && s.string == 0 && s.heldBackIn < 0 && s.eye == 0 && s.eyePivot == 0 && !s.haveEye && !s.live &&
           (fading ? s.fade == 1.0f : s.fade == 0.0f);
}

bool Identity(const recoil::Output& o) {
    const recoil::Output none;
    return std::memcmp(o.k, none.k, sizeof(o.k)) == 0 && std::memcmp(o.q, none.q, sizeof(o.q)) == 0 && !o.active;
}

void Check12() {
    for (int kind = 0; kind < 4; ++kind) {
        Sim r(M416(), 1.0f, 14);
        r.Burst(0.0, 5, 0.08);
        while (r.End() < 0.35) {
            r.f.trigger = true;
            r.Step();
        }
        const recoil::Output before = r.out;
        const uint32_t rng = r.s.rng;
        float prev[12];
        std::memcpy(prev, before.k, sizeof(prev));
        if (kind == 0)
            r.f.newGun = true;
        else if (kind == 1)
            r.hold.held = false;
        else if (kind == 2)
            r.f.gap = 0.3f;
        else
            r.hold.held = false, r.hold.stillDrawn = true;
        r.Step();
        r.f.gap = -1.0f;
        const bool easing = kind >= 2;
        const bool defaults = AtDefaults(r.s, easing) && r.s.rng == rng;
        if (!easing) {
            Check("12", Identity(r.out) && defaults, kind == 0 ? "another gun: the identity at once" : "let go: the identity at once",
                  "%s, the state %s, the random numbers %s", Identity(r.out) ? "identity" : "NOT the identity", AtDefaults(r.s, false) ? "at its defaults" : "NOT at its defaults",
                  r.s.rng == rng ? "kept" : "NOT kept");
            continue;
        }
        // Easing: the first frame the last move as it was, then within 0.1 s the identity; no frame's step larger than the first's share.
        float first = 0;
        for (int i = 0; i < 12; ++i)
            first = std::fmax(first, std::fabs(r.out.k[i] - prev[i]));
        int frames = 1;
        float biggest = 0;
        std::memcpy(prev, r.out.k, sizeof(prev));
        while (!Identity(r.out) && frames < 40) {
            r.Step();
            for (int i = 0; i < 12; ++i)
                biggest = std::fmax(biggest, std::fabs(r.out.k[i] - prev[i]));
            std::memcpy(prev, r.out.k, sizeof(prev));
            ++frames;
        }
        const float seconds = static_cast<float>((frames - 1) / 72.0);
        Check("12", defaults && first < 1e-6f && Identity(r.out) && seconds <= 0.1f + 1.5f / 72.0f,
              kind == 2 ? "a frame of 0.3 s: cleared, the last move easing out" : "a scripted moment, the gun still drawn: easing out",
              "the state %s, the first frame %.1e off the last, the identity after %.3f s (0.1), its biggest frame %.4f", defaults ? "at its defaults" : "NOT at its defaults",
              first, seconds, biggest);
    }
}

// ---- 13. The mirror ----

void Check13() {
    for (int mirror = 1; mirror >= 0; --mirror) {
        Sim right(M416(), 1.0f, 15), left(M416(), 1.0f, 15);
        left.hold.leftTrigger = true;
        right.tune.mirror = left.tune.mirror = mirror != 0;
        right.Burst(0.0, 12, 0.08), left.Burst(0.0, 12, 0.08);
        float worstNeg = 0, worstSame = 0, worstYaw = 0, worstSide = 0, peak = 1e-6f;
        while (right.End() < 1.8) {
            right.f.trigger = left.f.trigger = right.Start() < 1.0;
            right.Step(), left.Step();
            const recoil::Output& a = right.out;
            const recoil::Output& b = left.out;
            peak = std::fmax(peak, std::fabs(a.pitch));
            worstNeg = std::fmax(worstNeg, std::fabs(a.roll + b.roll));
            worstSame = std::fmax(worstSame, std::fmax(std::fabs(a.pitch - b.pitch), std::fabs(a.back - b.back) * 100));
            const float yawKick = a.yaw - a.drift[1], yawKickLeft = b.yaw - b.drift[1];
            worstYaw = std::fmax(worstYaw, mirror ? std::fabs(yawKick + yawKickLeft) : std::fabs(yawKick - yawKickLeft));
            worstSide = std::fmax(worstSide, mirror ? std::fabs(a.drift[1] + b.drift[1]) : std::fabs(a.drift[1] - b.drift[1]));
        }
        const float tolerance = 1e-5f * peak;
        if (mirror)
            Check("13", worstNeg <= tolerance && worstYaw <= tolerance && worstSide <= tolerance && worstSame <= tolerance,
                  "the left trigger: the yaw, the roll, the drift's sideways the negatives; the rise and the back the same",
                  "off by roll %.1e, yaw %.1e, sideways %.1e, rise/back %.1e", worstNeg, worstYaw, worstSide, worstSame);
        else
            Check("13", worstNeg <= tolerance && worstYaw <= tolerance && worstSide <= tolerance && worstSame <= tolerance,
                  "mirror off: the drift's sideways and the yaw as the right's, the roll still mirrored",
                  "off by roll %.1e, yaw %.1e, sideways %.1e, rise/back %.1e", worstNeg, worstYaw, worstSide, worstSame);
    }
}

// ---- 14. The fold ----

// hands.cpp's own, copied.
struct Q { float x, y, z, w; };
void QuatToMatrix(const Q& q, float* m) {
    float x = q.x, y = q.y, z = q.z, w = q.w;
    m[0] = 1 - 2 * (y * y + z * z), m[1] = 2 * (x * y - z * w), m[2] = 2 * (x * z + y * w);
    m[3] = 2 * (x * y + z * w), m[4] = 1 - 2 * (x * x + z * z), m[5] = 2 * (y * z - x * w);
    m[6] = 2 * (x * z - y * w), m[7] = 2 * (y * z + x * w), m[8] = 1 - 2 * (x * x + y * y);
}
void Mul3(const float* a, const float* b, float* out) {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            out[i * 3 + j] = a[i * 3] * b[j] + a[i * 3 + 1] * b[3 + j] + a[i * 3 + 2] * b[6 + j];
}
Q QuatMul(const Q& a, const Q& b) {
    return {a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y, a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w, a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}

uint32_t g_testRng = 12345;
float Random(float lo, float hi) { return recoil::Uniform(g_testRng, lo, hi); }
Q RandomTurn(float mostDegrees) {
    float axis[3] = {Random(-1, 1), Random(-1, 1), Random(-1, 1)};
    const float n = std::sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]) + 1e-9f;
    const float half = 0.5f * Random(0, mostDegrees) * recoil::kDegree;
    return {axis[0] / n * std::sin(half), axis[1] / n * std::sin(half), axis[2] / n * std::sin(half), std::cos(half)};
}
float Between(const float* a, const float* b) {   // radians between two rotations (a^T b's angle, precise near zero)
    float at[9], r[9];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            at[i * 3 + j] = a[j * 3 + i];
    Mul3(at, b, r);
    const float s = 0.5f * std::sqrt((r[7] - r[5]) * (r[7] - r[5]) + (r[2] - r[6]) * (r[2] - r[6]) + (r[3] - r[1]) * (r[3] - r[1]));
    return std::atan2(s, 0.5f * (r[0] + r[4] + r[8] - 1.0f));
}

void Check14() {
    const float raises[3] = {0.0f, 0.29f, 0.7f};
    float worst = 0, worstSteerBy[3] = {};
    for (int which = 0; which < 3; ++which) {
        const float upDegrees = raises[which];
        float& worstSteer = worstSteerBy[which];
        const float up = upDegrees * 3.14159265f / 180.0f;
        const float raise[9] = {1, 0, 0, 0, std::cos(up), std::sin(up), 0, -std::sin(up), std::cos(up)};   // hands.cpp's raise[9]
        const float flip[9] = {-1, 0, 0, 0, 1, 0, 0, 0, -1};
        for (int i = 0; i < 2000; ++i) {
            const Q view = RandomTurn(180), aim = RandomTurn(180), kick = RandomTurn(30);
            float toView[9], aimM[9], kickM[9], t1[9], t2[9], drawn[9], wantM[9];
            QuatToMatrix(view, toView), QuatToMatrix(aim, aimM), QuatToMatrix(kick, kickM);
            // What the drawn gun is: toView . aim . F . raise . Mq
            Mul3(toView, aimM, t1), Mul3(t1, flip, t2), Mul3(t2, raise, t1), Mul3(t1, kickM, wantM);
            // The folded aim drawn with no kick: toView . Fold(aim) . F . raise
            const float a[4] = {aim.x, aim.y, aim.z, aim.w}, k[4] = {kick.x, kick.y, kick.z, kick.w};
            float folded[4];
            recoil::Fold(a, k, upDegrees, folded);
            float foldM[9];
            QuatToMatrix({folded[0], folded[1], folded[2], folded[3]}, foldM);
            Mul3(toView, foldM, t1), Mul3(t1, flip, t2), Mul3(t2, raise, drawn);
            worst = std::fmax(worst, Between(drawn, wantM));
            // The steer's own (hands.cpp): inAim = raised . q . lowered, raised = {+sin, 0, 0, cos}; g_aimTurn = aim . {-x, y, -z, w}
            const float halfUp = upDegrees * 3.14159265f / 360.0f;
            const Q raised{std::sin(halfUp), 0.0f, 0.0f, std::cos(halfUp)}, lowered{-std::sin(halfUp), 0.0f, 0.0f, std::cos(halfUp)};
            const Q inAim = QuatMul(QuatMul(raised, kick), lowered);
            const Q steer = QuatMul(aim, Q{-inAim.x, inAim.y, -inAim.z, inAim.w});
            float steerM[9];
            QuatToMatrix(steer, steerM);
            Mul3(toView, steerM, t1), Mul3(t1, flip, t2), Mul3(t2, raise, drawn);
            worstSteer = std::fmax(worstSteer, Between(drawn, wantM));
        }
    }
    Check("14", worst <= 1e-4f, "the fold: the aim folded draws the gun as kicked (raises 0, 0.29, 0.7; kicks to 30 degrees)", "worst %.2e rad (1e-4)",
          worst);
    Info("14b", "the steer's own fold (raised . q . lowered, hands.cpp) checked the same way: for the steer's owner",
         "worst for turns to 30 degrees: raised 0: %.3f deg, 0.29 (the rifle): %.3f, 0.7 (the launcher): %.3f (the other conjugation)",
         worstSteerBy[0] / recoil::kDegree, worstSteerBy[1] / recoil::kDegree, worstSteerBy[2] / recoil::kDegree);
}

// ---- 15. The move keeps its pivot ----

void Check15() {
    recoil::Gun g = M416();
    g.kick.back.peak = 0.0f, g.kick.lift.peak = 0.0f;
    float worstPivot = 0, worstOrtho = 0;
    for (uint32_t seed = 1; seed <= 5; ++seed) {
        Sim r(g, seed % 2 ? 0.0f : 1.0f, seed);
        r.Burst(0.0, 10, 0.08);
        while (r.End() < 1.2) {
            r.f.trigger = r.Start() < 0.8;
            r.Step();
            float moved[3];
            Apply(r.out.k, kGrip, moved);
            const float d = std::sqrt((moved[0] - kGrip[0]) * (moved[0] - kGrip[0]) + (moved[1] - kGrip[1]) * (moved[1] - kGrip[1]) +
                                      (moved[2] - kGrip[2]) * (moved[2] - kGrip[2]));
            worstPivot = std::fmax(worstPivot, d);
            worstOrtho = std::fmax(worstOrtho, Orthonormal(r.out.k));
        }
    }
    Check("15", worstPivot <= 1e-6f && worstOrtho <= 1e-6f, "no back, no lift: the pivot kept, the turn orthonormal", "pivot %.2e m, %.2e off orthonormal",
          worstPivot, worstOrtho);
}

// ---- 16. Nothing not finite ----

void Check16() {
    const float nan = std::numeric_limits<float>::quiet_NaN();
    bool finite = true;
    int runs = 0;
    auto run = [&](recoil::Gun g, recoil::Hold hold, recoil::Tune tune, float dt, int shots, const float* pivot, const float* eye) {
        recoil::State s;
        recoil::Frame f;
        f.seed = 99;
        for (int i = 0; i < 6; ++i) {
            f.dt = i < 3 ? dt : 1.0f / 72.0f;
            f.shots = i < 3 ? shots : 1;
            f.trigger = true;
            if (pivot)
                std::memcpy(f.pivot, pivot, sizeof(f.pivot));
            if (eye)
                std::memcpy(f.eyeAt, eye, sizeof(f.eyeAt)), f.haveEye = true;
            const recoil::Output o = recoil::Step(g, hold, f, tune, s);
            f.seed = 0;
            finite = finite && AllFinite(o) && Orthonormal(o.k) < 1e-4f;
            ++runs;
        }
    };
    recoil::Hold two;
    two.held = true, two.hands = 1.0f, two.eye = true;
    const recoil::Tune tune;
    const float dts[] = {0.0f, -0.01f, nan, 10.0f, 1.0f / 72.0f};
    const int shotsList[] = {0, 3, 100};
    for (float dt : dts)
        for (int shots : shotsList)
            run(M416(), two, tune, dt, shots, kGrip, kEye);
    const float dampings[] = {0.0f, 1.0f, nan};
    for (float d : dampings) {
        recoil::Gun g = M416();
        g.kick.back.damping = g.kick.rise.damping = g.kick.lift.damping = d, g.drift.damping = d;
        run(g, two, tune, 1.0f / 72.0f, 2, kGrip, kEye);
    }
    recoil::Gun noRate = M416();
    noRate.rate = 0.0f;
    run(noRate, two, tune, 1.0f / 72.0f, 3, kGrip, kEye);
    recoil::Gun broken = M416();
    broken.drift.up[0] = nan, broken.drift.capUp = nan, broken.kick.rise.peak = nan, broken.kick.rise.at = 0.0f, broken.eyeFactor = nan, broken.rumble[0] = nan;
    broken.drift.spring = nan, broken.pairGap = nan, broken.kick.jitter = nan;
    run(broken, two, tune, 1.0f / 72.0f, 3, kGrip, kEye);
    recoil::Hold odd = two;
    odd.hands = nan;
    const float nanPoint[3] = {nan, 0.0f, nan};
    run(M416(), odd, tune, 1.0f / 72.0f, 3, nanPoint, nanPoint);
    recoil::Tune wild;
    wild.kick = nan, wild.drift = nan, wild.oneRise = nan, wild.liftMost = nan, wild.sideSign = nan, wild.gun = nan, wild.driftDamping = nan;
    run(M416(), two, wild, 1.0f / 72.0f, 3, kGrip, kEye);
    // 100 rounds in a frame are taken as 3.
    recoil::State a, b;
    recoil::Frame fa, fb;
    fa.seed = fb.seed = 5, fa.dt = fb.dt = 1.0f / 72.0f, fa.shots = 100, fb.shots = 3, fa.trigger = fb.trigger = true;
    recoil::Hold hold = two;
    hold.eye = false;
    const recoil::Output oa = recoil::Step(M416(), hold, fa, tune, a), ob = recoil::Step(M416(), hold, fb, tune, b);
    const bool same = std::memcmp(oa.k, ob.k, sizeof(oa.k)) == 0 && a.upTarget == b.upTarget;
    Check("16", finite && same, "nothing not finite (dt 0, negative, NaN, 10 s; shots 0, 3, 100; damping 0, 1, NaN; rate 0; NaN inputs)",
          "%d frames %s; 100 rounds %s 3", runs, finite ? "all finite and orthonormal" : "NOT ALL FINITE", same ? "taken as" : "NOT taken as");
}

// ---- 17. The buzz, every gun ----

#ifdef RECOILTEST_TABLE
// A row of the table as recoilrun.cpp sets its gun up (its numbers when its data has no gun sway: the standing rows, its own); Gun::automatic and
// Gun::burst left at -1 as recoilrun leaves them.
recoil::Gun GunOf(const recoilguns::Row& row) {
    recoil::Gun g;
    g.kind = row.kind, g.rate = row.rate, g.pairGap = row.pairGap, g.kick = row.kick, g.eyeFactor = row.eye[0];
    for (int i = 0; i < 2; ++i)
        g.drift.up[i] = row.up[0][i], g.drift.sideMin[i] = row.sideMin[0][i], g.drift.sideMax[i] = row.sideMax[0][i], g.drift.decrease[i] = row.decrease[0][i];
    g.drift.capUp = row.capUp, g.drift.capSide = row.capSide, g.drift.firstShot = row.sig[4], g.drift.shootingScale = row.sig[5];
    g.drift.spring = row.sig[6], g.drift.damping = row.sig[7];
    for (int i = 0; i < 3; ++i)
        g.rumble[i] = row.rumble[i];
    return g;
}
#endif

void Check17() {
#ifdef RECOILTEST_TABLE
    // Table 4.2's C column (its amplitude, its length in ms, the support hand's amplitude), each gun by its row's name; its kind, rate, rumble and lift
    // the table's (the game's own data), as recoilrun.cpp runs it.
    struct Want {
        const char* name;
        bool frontGrip;   // the support hand on a front grip (the M320's, a launcher's)
        float amp, ms, support;
    } wants[] = {
        {"M9", false, 0.78f, 43, 0.55f},        {"MP-443", false, 0.78f, 43, 0.55f},     {"UMP-45", false, 0.89f, 42, 0.53f},     {"MP7", false, 1.0f, 23, 0.6f},
        {"P90", false, 1.0f, 23, 0.6f},         {"PP-2000", false, 0.78f, 42, 0.47f},    {"PDW-R", false, 1.0f, 29, 0.6f},        {"AS Val", false, 1.0f, 35, 0.6f},
        {"AKS-74u", false, 0.78f, 36, 0.47f},   {"M4A1", false, 1.0f, 29, 0.6f},         {"SCAR-H", false, 0.89f, 39, 0.53f},     {"G36C", false, 1.0f, 29, 0.6f},
        {"SG 553", false, 1.0f, 29, 0.6f},      {"M416", false, 1.0f, 29, 0.6f},         {"AK-74M", false, 0.78f, 36, 0.47f},     {"AN-94", false, 0.89f, 38, 0.53f},
        {"AEK-971", false, 1.0f, 23, 0.6f},     {"KH2002", false, 0.81f, 36, 0.48f},     {"G3A3", false, 1.0f, 35, 0.6f},         {"F2000", false, 1.0f, 39, 0.6f},
        {"M16A4", false, 1.0f, 29, 0.6f},       {"RPK-74M", false, 0.86f, 34, 0.52f},    {"M240B", false, 1.0f, 55, 0.6f},        {"M249", false, 0.83f, 36, 0.5f},
        {"PKP", false, 1.0f, 37, 0.6f},         {"Type 88", false, 0.83f, 36, 0.5f},     {"M60E4", false, 1.0f, 65, 0.6f},        {"M27 IAR", false, 1.0f, 32, 0.6f},
        {"870 MCS", false, 1.0f, 95, 0.6f},     {"M1014", false, 1.0f, 43, 0.6f},        {"Saiga 12K", false, 1.0f, 42, 0.6f},    {"USAS-12", false, 1.0f, 55, 0.6f},
        {"DAO-12", false, 1.0f, 42, 0.6f},      {"Mk11 Mod 0", false, 0.89f, 60, 0.53f}, {"M39 EMR", false, 1.0f, 60, 0.6f},      {"SVD", false, 0.89f, 60, 0.53f},
        {"M40A5", false, 0.89f, 60, 0.53f},     {"M98B", false, 1.0f, 65, 0.6f},         {"SV-98", false, 1.0f, 63, 0.6f},        {"M82A3", false, 1.0f, 85, 0.6f},
        {"M320", true, 1.0f, 55, 0.8f},         {"SMAW", true, 1.0f, 55, 0.8f},          {"Stinger", true, 1.0f, 55, 0.8f},       {"Javelin", true, 1.0f, 55, 0.8f},
        {"AT4", true, 0.45f, 35, 0.36f},
    };
    int bad = 0, checked = 0, missing = 0;
    char first[200] = {};
    for (const Want& want : wants) {
        const recoilguns::Row* row = recoilguns::ByName(want.name);
        if (!row) {
            if (!missing++ && !bad)
                std::snprintf(first, sizeof(first), "%s not in the table", want.name);
            continue;
        }
        const recoil::Gun g = GunOf(*row);
        const bool automatic = g.kind == recoil::Kind::kSmg || g.kind == recoil::Kind::kRifle || g.kind == recoil::Kind::kMg;
        const bool lift = row->kick.lift.peak > 0.0f;
        for (int hands = 0; hands < 2; ++hands) {
            recoil::State s;
            recoil::Hold hold;
            hold.held = true, hold.hands = static_cast<float>(hands), hold.frontGrip = want.frontGrip;
            recoil::Frame f;
            f.dt = 1.0f / 72.0f, f.shots = 1, f.trigger = true, f.seed = 17;
            const recoil::Output o = recoil::Step(g, hold, f, recoil::Tune{}, s);
            const float gap = 60.0f / g.rate;
            bool ok = std::fabs(o.pulse[0] - want.amp) <= 0.006f && std::fabs(o.pulseFor[0] * 1000 - want.ms) <= 1.0f;
            ok = ok && (!automatic || o.pulseFor[0] <= 0.8f * gap + 1e-6f);
            ok = ok && (hands ? std::fabs(o.pulse[1] - want.support) <= 0.006f && o.pulseFor[1] == o.pulseFor[0] : o.pulse[1] == 0.0f);
            ok = ok && (lift ? std::fabs(o.lateIn - 0.03f) < 1e-6f && std::fabs(o.late - 0.6f) < 1e-6f && std::fabs(o.lateFor - 0.12f) < 1e-6f : o.late == 0.0f);
            ++checked;
            if (!ok && !bad++ && !missing)
                std::snprintf(first, sizeof(first), "%s (%s): %.3f for %.1f ms, support %.3f, late %.2f", want.name, hands ? "two hands" : "one hand", o.pulse[0],
                              o.pulseFor[0] * 1000, o.pulse[1], o.late);
            if (Verbose("17"))
                std::printf("  %-12s %s amp %.3f (%.2f) %.1f ms (%.0f) support %.3f (%.2f) late %.2f\n", want.name, hands ? "two" : "one", o.pulse[0], want.amp,
                            o.pulseFor[0] * 1000, want.ms, o.pulse[1], hands ? want.support : 0.0f, o.late);
        }
    }
    // An automatic whose own length would pass 0.8 of its gap is cut there.
    recoil::Gun fast = M416();
    fast.rate = 1200, fast.rumble[0] = 1.0f, fast.rumble[2] = 0.4f;
    recoil::State s;
    recoil::Hold hold;
    hold.held = true, hold.hands = 1.0f;
    recoil::Frame f;
    f.dt = 1.0f / 72.0f, f.shots = 1, f.trigger = true;
    const recoil::Output o = recoil::Step(fast, hold, f, recoil::Tune{}, s);
    const bool cut = std::fabs(o.pulseFor[0] - 0.8f * 0.05f) < 1e-6f;
    Check("17", bad == 0 && missing == 0 && cut, "the buzz, every gun of table 4.2 as the table has it, one hand and two",
          "%d of %d off, %d not found%s%s; an automatic's long pulse cut to 0.8 of its gap: %s", bad, checked, missing, bad || missing ? ", the first: " : "", first,
          cut ? "yes" : "NO");
#else
    Info("17", "src/recoilguns.h not there yet", "skipped");
#endif
}

// ---- 18. Determinism ----

void Check18() {
    Sim a(M416(), 0.5f, 18), b(M416(), 0.5f, 18);
    a.Burst(0.0, 20, 0.08), b.Burst(0.0, 20, 0.08);
    bool same = true;
    while (a.End() < 2.5) {
        a.f.trigger = b.f.trigger = a.Start() < 1.0 || (a.Start() > 1.5 && a.Start() < 1.6);
        a.hold.hands = b.hold.hands = a.Start() < 0.6 ? 0.3f : 0.9f;
        a.Step(), b.Step();
        same = same && std::memcmp(a.out.k, b.out.k, sizeof(a.out.k)) == 0 && std::memcmp(a.out.q, b.out.q, sizeof(a.out.q)) == 0 &&
               std::memcmp(&a.out.slack, &b.out.slack, sizeof(float)) == 0 && std::memcmp(a.out.pulse, b.out.pulse, sizeof(a.out.pulse)) == 0;
    }
    Check("18", same, "the same seed and frames: the same, bit for bit", "%s", same ? "identical" : "DIFFERENT");
}

// ---- 19. The AN-94 ----

void Check19() {
    {
        // A burst pair 50 ms apart (its 1200 rpm), the mode not known: 0.3 then 1.7; the kick (B, the drift aside) as the second round leaves near
        // the first's line.
        Sim r(AN94(), 1.0f, 19, 1000.0);
        r.gun.kick.jitter = 0.0f;
        r.Burst(0.0, 2, 0.05);
        float shares[2] = {}, atSecond = 0;
        int got = 0;
        while (r.frame < 600) {
            r.f.trigger = r.Start() < 0.06;
            const int before = r.nextShot;
            const float kickBefore = r.Kick();
            r.Step();
            if (r.nextShot > before && got < 2) {
                shares[got++] = r.s.share;
                if (got == 2)
                    atSecond = kickBefore;
            }
        }
        Check("19", std::fabs(shares[0] - 0.3f) < 1e-6f && std::fabs(shares[1] - 1.7f) < 1e-6f && std::fabs(atSecond) <= 0.3f * 1.6f * 1.02f,
              "the AN-94's pair 50 ms apart", "%.2f then %.2f (0.3, 1.7); the kick's rise as the second left %.3f deg (within 0.3 x 1.6 = 0.48)", shares[0],
              shares[1], atSecond);
    }
    {
        // A lone round in its burst mode: 0.3 at once, the 0.7 75 ms later.
        Sim r(AN94(), 1.0f, 20, 1000.0);
        r.gun.kick.jitter = 0.0f, r.gun.burst = 1;
        r.Burst(0.0, 1, 0.0);
        float share = 0, vAt[2] = {};
        while (r.frame < 300) {
            r.f.trigger = r.Start() < 0.02;
            const float v = r.s.pitch.v;
            const double start = r.Start();
            r.Step();
            if (r.frame == 1)
                share = r.s.share;
            if (r.frame >= 70 && r.frame <= 80 && r.s.pitch.v - v > 1.0f)
                vAt[0] = static_cast<float>(start), vAt[1] = r.s.pitch.v - v;
        }
        const recoil::Params p = recoil::Blend(r.gun, r.hold, r.tune, r.s, false);
        const float whole = 1.6f * p.wn[1] / p.gain[1];
        Check("19", std::fabs(share - 0.3f) < 1e-6f && std::fabs(vAt[0] - 0.075f) <= 0.0011f && std::fabs(vAt[1] / whole - 0.7f) < 0.05f,
              "the AN-94's lone round in its burst mode", "%.2f at once, %.2f more at %.0f ms (0.7 at 75)", share, vAt[1] / whole, vAt[0] * 1000);
    }
    {
        // The mode not known, rounds 100 ms apart (its automatic 600 rpm): the pull's first split, then every round whole at once.
        Sim r(AN94(), 1.0f, 21, 1000.0);
        r.gun.rate = 600;
        r.Burst(0.0, 6, 0.1);
        float shares[6] = {};
        int got = 0;
        while (r.frame < 900) {
            r.f.trigger = r.Start() < 0.62;
            const int before = r.nextShot;
            r.Step();
            if (r.nextShot > before && got < 6)
                shares[got++] = r.s.share;
        }
        bool whole = true;
        for (int i = 1; i < 6; ++i)
            whole = whole && shares[i] == 1.0f;
        Check("19", std::fabs(shares[0] - 0.3f) < 1e-6f && whole && r.s.pull == 0, "the AN-94, the mode not known, rounds 100 ms apart",
              "the first %.2f, then %.2f %.2f %.2f %.2f %.2f (automatic: whole)", shares[0], shares[1], shares[2], shares[3], shares[4], shares[5]);
    }
    {
        // In automatic, known (the critic's 12): every round whole, at once.
        Sim r(AN94(), 1.0f, 22, 1000.0);
        r.gun.rate = 600, r.gun.burst = 0;
        r.Burst(0.0, 8, 0.1);
        bool whole = true, nothingLate = true;
        while (r.frame < 1100) {
            r.f.trigger = r.Start() < 0.82;
            const int before = r.nextShot;
            r.Step();
            if (r.nextShot > before)
                whole = whole && r.s.share == 1.0f;
            nothingLate = nothingLate && r.s.heldBackIn < 0.0f;
        }
        Check("19", whole && nothingLate, "the AN-94 in automatic: every round whole, at once (the critic's 12)", "%s, %s", whole ? "whole" : "NOT whole",
              nothingLate ? "nothing held back" : "SOMETHING HELD BACK");
    }
}

// ---- 20. The table (src/recoilguns.h) ----

void Check20() {
#ifdef RECOILTEST_TABLE
    using recoilguns::kRowCount;
    using recoilguns::kRows;
    int clashes = 0;
    for (int i = 0; i < kRowCount; ++i) {
        for (int j = i + 1; j < kRowCount; ++j)
            clashes += recoilguns::SameName(kRows[i].name, kRows[j].name) ? 1 : 0;
        clashes += recoilguns::ByName(kRows[i].name) != &kRows[i] ? 1 : 0;
    }
    Check("20", clashes == 0, "the table: every row's name its own, and found by it", "%d rows, %d clashes", kRowCount, clashes);
    int sway = 0, found = 0, shared = 0;
    char missed[160] = {};
    for (const recoilguns::Row& row : kRows) {
        if (row.kind == recoil::Kind::kNone)
            continue;
        ++sway;
        const recoilguns::Row* got = recoilguns::BySignature(row.rate, row.speed, row.magazine, row.pellets, row.sig);
        if (got == &row)
            ++found;
        else if (got && std::strcmp(row.name, "Javelin") == 0 && std::strcmp(got->name, "Stinger") == 0)
            ++shared;
        else if (!missed[0])
            std::snprintf(missed, sizeof(missed), "; missed: %s (found %s)", row.name, got ? got->name : "none");
    }
    Check("20", sway == 49 && found == 48 && shared == 1, "the table: each gun's own numbers find its own row (the Stinger and the Javelin share theirs)",
          "%d of %d rows with a gun sway, the Javelin's the Stinger's: %s%s", found, sway, shared ? "yes" : "NO", missed);
    int bodies = 0, byBody = 0;
    for (const recoilguns::Row& row : kRows)
        if (row.body)
            ++bodies, byBody += recoilguns::ByBody(row.body) == &row ? 1 : 0;
    Check("20", bodies >= 11 && byBody == bodies, "the table: every gun the mod draws found by its body", "%d of %d (the design's 11 draws, and the train pistol's id)",
          byBody, bodies);
    const recoilguns::Row* m = recoilguns::ByName("M416");
    const float want[8] = {0.26f, -0.3f, 0.1f, 18.0f, 2.0f, 0.1f, 250.0f, 0.35f};
    bool own = m && m->rate == 750.0f && m->kick.back.peak == 0.015f && m->kick.rise.peak == 1.8f;
    for (int i = 0; own && i < 8; ++i)
        own = m->sig[i] == want[i];
    Check("20", own, "the table: the M416's own row", "%s", own ? "0.26, -0.3..0.1, 18, x2, x0.1, 250/0.35, 750 a minute; 1.5 cm and 1.8 degrees" : "NOT AS THE DATA");
    // Every row through Step, one hand and two, a dozen rounds at its rate: finite, within the caps, at rest again after.
    int ran = 0, bad = 0;
    char first[200] = {};
    for (const recoilguns::Row& row : kRows) {
        if (row.kind == recoil::Kind::kNone)
            continue;
        const recoil::Gun g = GunOf(row);
        for (int hands = 0; hands < 2; ++hands) {
            Sim r(g, static_cast<float>(hands), 31);
            r.Burst(0.0, 12, 60.0 / row.rate);
            // Long enough for its rounds, its string's end (held kFiringGaps of its gaps at most), its drift to come back from its cap at its own rate
            // (one hand: x1.5 the cap, half the rate), and to settle.
            const double length = (11.0 + recoil::kFiringGaps) * 60.0 / row.rate + 1.5 * row.capUp / (0.5 * std::fmax(row.decrease[0][0], 1.0f)) + 4.0;
            bool finite = true;
            float turn = 0, back = 0;
            while (r.End() < length) {
                r.f.trigger = r.nextShot < 12;
                r.Step();
                finite = finite && AllFinite(r.out) && Orthonormal(r.out.k) < 1e-4f;
                turn = std::fmax(turn, std::sqrt(r.out.pitch * r.out.pitch + r.out.yaw * r.out.yaw)), back = std::fmax(back, r.out.back);
            }
            const bool ok = finite && turn <= recoil::kTotalTurnMost + 1e-3f && back <= 0.06f + recoil::kHeldMost && !r.out.active;
            ++ran;
            if (!ok && !bad++)
                std::snprintf(first, sizeof(first), "; the first: %s (%s): %s, turn %.1f, back %.1f cm, %s", row.name, hands ? "two hands" : "one hand",
                              finite ? "finite" : "NOT FINITE", turn, back * 100, r.out.active ? "still kicking at the end" : "at rest");
            if (Verbose("20"))
                std::printf("  %-26s %s turn %5.2f back %.2f cm\n", row.name, hands ? "two" : "one", turn, back * 100);
        }
    }
    Check("20", bad == 0, "every row of the table through Step, one hand and two: finite, within the caps, at rest again", "%d of %d runs wrong%s", bad, ran, first);
    // The recorded rows against the recordings they're sized from (kick.tsv's: the game's own kick in the hands, hip fire): one round from rest with
    // two hands, its back's peak and its drawn rise's (the kick and the drift together); held fire's mean back over rounds 5-30 of a string at
    // its rate. The M16A4's rise isn't its own (its recording's is disturbed: the M4A1's in its place). All within 3%.
    struct Recording { const char* name; float back, rise, held; };   // cm, degrees (< 0: not its own), cm (0: no loop)
    const Recording recordings[] = {
        {"M4A1", 1.72f, 2.08f, 2.79f},  {"AK-74M", 1.72f, 2.06f, 2.77f},  {"AEK-971", 1.76f, 2.09f, 2.82f}, {"KH2002", 1.70f, 2.04f, 0.0f},
        {"G3A3", 1.69f, 2.35f, 2.63f},  {"M16A4", 1.73f, -1.0f, 2.97f},   {"RPK-74M", 1.70f, 2.07f, 2.81f}, {"M240B", 1.53f, 2.57f, 2.40f},
        {"M320", 2.70f, 5.80f, 0.0f},
    };
    int recorded = 0, off = 0;
    char worst[200] = {};
    float worstShare = 0;
    auto against = [&](const char* name, const char* what, float got, float want) {
        const float share = std::fabs(got / want - 1.0f);
        ++recorded;
        if (share > 0.03f)
            ++off;
        if (share >= worstShare)
            worstShare = share, std::snprintf(worst, sizeof(worst), "%s's %s %.2f against %.2f", name, what, got, want);
        if (Verbose("20"))
            std::printf("  %-8s %-10s %.3f against %.3f (%+.1f%%)\n", name, what, got, want, (got / want - 1.0f) * 100);
    };
    for (const Recording& rec : recordings) {
        const recoilguns::Row* row = recoilguns::ByName(rec.name);
        if (!row) {
            ++recorded, ++off;
            continue;
        }
        recoil::Gun g = GunOf(*row);
        g.kick.jitter = 0.0f;
        float back = 0, rise = 0;
        Sim one(g, 1.0f, 3, 1000.0);
        one.Burst(0.0, 1, 0.0);
        while (one.frame < 1000) {
            one.f.trigger = one.Start() < 0.02;
            one.Step();
            back = std::fmax(back, one.out.back), rise = std::fmax(rise, one.out.pitch);
        }
        against(rec.name, "back", back * 100, rec.back);
        if (rec.rise > 0.0f)
            against(rec.name, "rise", rise, rec.rise);
        if (rec.held > 0.0f) {
            const double gap = 60.0 / row->rate;
            Sim loop(g, 1.0f, 3, 1000.0);
            loop.Burst(0.0, 30, gap);
            double sum = 0;
            int samples = 0;
            while (loop.Now() < 30 * gap + 0.3) {
                loop.f.trigger = loop.Start() <= 29 * gap + 0.01;
                loop.Step();
                if (loop.Now() > 4 * gap && loop.Now() <= 29 * gap)
                    sum += loop.out.back, ++samples;
            }
            against(rec.name, "held fire", static_cast<float>(sum / samples * 100), rec.held);
        }
    }
    {
        // The AN-94's burst pair, 47 ms apart: 1.59 cm back and 3.11 degrees up at their peaks.
        const recoilguns::Row* row = recoilguns::ByName("AN-94");
        if (row) {
            recoil::Gun g = GunOf(*row);
            g.kick.jitter = 0.0f;
            Sim pair(g, 1.0f, 3, 1000.0);
            pair.Burst(0.0, 2, 0.047);
            float back = 0, rise = 0;
            while (pair.frame < 1000) {
                pair.f.trigger = pair.Start() < 0.06;
                pair.Step();
                back = std::fmax(back, pair.out.back), rise = std::fmax(rise, pair.out.pitch);
            }
            against("AN-94", "pair back", back * 100, 1.59f);
            against("AN-94", "pair rise", rise, 3.11f);
        } else {
            ++recorded, ++off;
        }
    }
    Check("20", off == 0, "the recorded rows through Step against their recordings: the first round's back and rise, the AN-94's pair, held fire",
          "%d of %d off by over 3%%; the furthest: %s (%.1f%%)", off, recorded, worst, worstShare * 100);
#else
    Info("20", "src/recoilguns.h not there yet", "skipped");
#endif
}

// ---- 21. One hand at the eye never brings the glass within 1.5 cm ----

void Check21() {
    float worst = 1.0f, worstRear = 1.0f;
    for (uint32_t seed = 1; seed <= 20; ++seed) {
        for (int irons = 0; irons < 2; ++irons) {
            Sim r(M416(), 0.0f, seed);
            r.hold.eye = true;
            std::memcpy(r.f.eyeAt, kEye, sizeof(kEye));
            r.f.haveEye = true;
            if (irons)
                r.f.rearZ = kEye[2] + 0.06f, r.f.haveRear = true;   // a rear sight 6 cm ahead of the eye
            else
                r.f.glassZ = kEye[2] + 0.035f, r.f.haveGlass = true;
            const float guard = irons ? r.f.rearZ : r.f.glassZ;
            r.Burst(0.3, 10, 0.08);
            r.shots[r.shotCount++] = 1.6;   // and a tap after the burst
            while (r.End() < 2.5) {
                r.f.trigger = (r.Start() >= 0.3 && r.Start() < 1.15) || (r.Start() >= 1.6 && r.Start() < 1.65);
                r.Step();
                const float room = guard - EyeInKicked(r.out.k, kEye);
                if (irons)
                    worstRear = std::fmin(worstRear, room);
                else
                    worst = std::fmin(worst, room);
                if (Verbose("21") && seed == 1)
                    std::printf("  %6.3f %s %.1f mm from the eye, pitch %.2f back %.2f cm\n", r.Now(), irons ? "rear" : "glass", room * 1000, r.out.pitch,
                                r.out.back * 100);
            }
        }
    }
    Check("21", worst >= recoil::kEyeGlassLeast - 1e-5f && worstRear >= recoil::kEyeGlassLeast - 1e-5f,
          "one hand at the eye, 20 seeds: the glass (and the irons' rear) never within 1.5 cm of the eye", "nearest: the glass %.2f mm, the rear %.2f mm (15)",
          worst * 1000, worstRear * 1000);
}

// ---- 22. The scope picture's slack ----

float TurnBetween(const float* a, const float* b) {   // radians between two unit lines (precise near zero)
    const float c[3] = {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
    return std::atan2(std::sqrt(c[0] * c[0] + c[1] * c[1] + c[2] * c[2]), a[0] * b[0] + a[1] * b[1] + a[2] * b[2]);
}

// What scopeview's MakeDesc lets the line turn by with this slack, the picture `frames` behind (its own doubling), within its 0.2-0.45 bounds.
float Covered(float lens, float slack, float power, int frames) {
    const float wanted = std::fmin(std::fmax(lens + 0.07f + slack * static_cast<float>(frames) * power, 0.2f), 0.45f);
    return (wanted - lens) / power;
}

void Check22() {
    struct Case { const char* name; recoil::Gun gun; float power; } cases[] = {
        {"an SVD at 3.44x", Marksman(260, 1.5f, -0.3f, -0.1f, 0.022f, 3.0f), 3.44f}, {"an Mk11 at 8x", Marksman(260, 1.5f, -0.3f, -0.1f, 0.021f, 2.6f), 8.0f},
        // The Mk11 as its row has it since it was recorded (recoilguns.h: 3.43 cm back, a rise of 1.37 degrees) behind its rifle scope's own 6.84x.
        {"the Mk11 with its recorded kick at its scope's 6.84x", Marksman(260, 1.5f, -0.3f, -0.1f, 0.0343f, 1.37f), 6.84f}};
    const float lens = 0.12f;   // the lens's own slope as MakeDesc takes it: (its radius + its offset) / focal, 80-110 pixels at 906 once the view is on
    for (Case& k : cases) {
        k.gun.eyeFactor = 1.0f;
        // tap 0: the button down a frame or more before each round leaves and on through them; tap 1: down only the frame before each round's,
        // up from the round's own frame on, so no strike term covers the frames after it: the turns of this frame, the one before and the next do.
        for (int tap = 0; tap < 2; ++tap) {
            Sim r(k.gun, 1.0f, 23);
            r.hold.eye = true;
            std::memcpy(r.f.eyeAt, kEye, sizeof(kEye));
            r.f.haveEye = true, r.f.glassZ = kEye[2] + 0.04f, r.f.haveGlass = true;
            for (int i = 0; i < 4; ++i)
                r.shots[r.shotCount++] = 0.5 + 0.3 * i;   // aimed rounds 0.3 s apart (its 260 rpm is 0.23)
            float line[3][3] = {}, slack[2] = {}, worstShort = 1.0f, firstShort = 1.0f, firstOwn = 1.0f, mostSlack = 0, mostTurn = 0, mostWanted = 0;
            int frames = 0, sinceFirst = 99;
            while (r.End() < 2.2) {
                if (tap) {
                    const double next = r.nextShot < r.shotCount ? r.shots[r.nextShot] : 1e9;   // (its round in the frame after this one)
                    r.f.trigger = next >= r.End() - 1e-9 && next < r.End() + 1.0 / 72.0 - 1e-9;
                } else {
                    // The button down a frame or more before each round leaves (the game's round comes on its tick after the press).
                    r.f.trigger = r.Start() >= 0.45 && r.Start() < 1.45;
                }
                const int before = r.nextShot;
                r.Step();
                if (before == 0 && r.nextShot == 1)
                    sinceFirst = 0;
                // The sights' line in the view (the held pose the identity): k's turn on z.
                const float now[3] = {r.out.k[2], r.out.k[6], r.out.k[10]};
                if (frames >= 3) {
                    // The picture drawn for this frame along the line of one or two frames before, its slack this frame's or the last's. The gun's very
                    // first round isn't foreseen (before it, nothing changes: the picture as it always was), so its own frames are told apart.
                    for (int behind = 1; behind <= 2; ++behind) {
                        const float turn = TurnBetween(now, line[behind - 1]);
                        for (int which = 0; which < 2; ++which) {
                            const float sl = which ? slack[0] : r.out.slack;
                            const float spare = Covered(lens, sl, k.power, behind) - turn;
                            if (sinceFirst < behind)
                                firstShort = std::fmin(firstShort, spare), firstOwn = which ? firstOwn : std::fmin(firstOwn, spare);
                            else
                                worstShort = std::fmin(worstShort, spare);
                        }
                        mostTurn = std::fmax(mostTurn, turn);
                    }
                }
                ++sinceFirst;
                mostSlack = std::fmax(mostSlack, r.out.slack);
                mostWanted = std::fmax(mostWanted, lens + 0.07f + r.out.slack * k.power);
                std::memcpy(line[2], line[1], sizeof(line[0]));
                std::memcpy(line[1], line[0], sizeof(line[0]));
                std::memcpy(line[0], now, sizeof(now));
                slack[1] = slack[0], slack[0] = r.out.slack;
                ++frames;
            }
            char what[160];
            if (tap)
                std::snprintf(what, sizeof(what), "... %s tapped, the button up from each round's frame on: the frames after it covered by the turns alone", k.name);
            else
                std::snprintf(what, sizeof(what), "the scope picture's slack covers the struck frames of %s, two hands at the eye, one frame or two behind", k.name);
            Check("22", worstShort >= -1e-5f, what, "the line's most turn %.2f deg; the slack at most %.2f deg; %.3f deg to spare", mostTurn / recoil::kDegree,
                  mostSlack / recoil::kDegree, worstShort / recoil::kDegree);
            if (tap)
                continue;
            Info("22", "... the gun's very first round (not foreseen)", "%s: with the struck frame's own slack %.3f deg to spare; with the frame before's %.3f",
                 k.name, firstOwn / recoil::kDegree, firstShort / recoil::kDegree);
            Info("22", "... the margin wanted against its 0.45 ceiling", "%s: lens %.2f + 0.07 + slack x power: at most %.3f", k.name, lens, mostWanted);
        }
    }
}

// ---- 23. A 0.3 s gap clears ----

void Check23() {
    Sim r(M416(), 1.0f, 24);
    r.Burst(0.0, 6, 0.08);
    r.shots[r.shotCount++] = 0.9;
    while (r.End() < 0.5) {
        r.f.trigger = true;
        r.Step();
    }
    const bool liveBefore = r.s.live;
    // The game stalls 0.3 s: hands.cpp's frame time is capped at 0.1, the real gap isn't.
    r.f.dt = 0.1f, r.f.gap = 0.3f, r.f.shots = 0;
    r.out = recoil::Step(r.gun, r.hold, r.f, r.tune, r.s);
    ++r.frame;
    r.f.gap = -1.0f;
    const bool cleared = !r.s.live && r.s.upTarget == 0.0f && r.s.since == recoil::State{}.since && r.s.released && r.s.fade == 1.0f;
    // The next round, the button held all along: a first shot again (its state started clear).
    float inc = 0;
    while (r.End() < 1.2) {
        const int before = r.nextShot;
        const float was = r.s.upTarget;
        r.Step();
        if (r.nextShot > before)
            inc = (r.s.upTarget - was) / 0.26f;
    }
    Check("23", liveBefore && cleared && inc >= 1.8f - 1e-4f, "a 0.3 s gap with dt capped at 0.1: everything clears", "%s; the next round x%.3f (a first shot)",
          cleared ? "cleared, the last move easing out" : "NOT CLEARED", inc);
}

// ---- 24. The hold taken mid-kick ----

void Check24() {
    // A yank is a jump in place: a frame's move larger than the spring's own speed and pull allow over the frame (|v| dt + a dt^2 / 2, a from its
    // stiffest and most damped constants of the two holds, with room). A cap below the state that cut it would jump; one that holds it stops it.
    const float dt = 1.0f / 72.0f;
    const float wRise = std::fmax(recoil::Omega(0.07f, 0.4f), recoil::Omega(0.05f, 0.75f)), wBack = std::fmax(recoil::Omega(0.045f, 0.6f), recoil::Omega(0.03f, 0.75f));
    float worst = 0, below = 0;
    int capped = 0;
    for (uint32_t seed = 1; seed <= 20; ++seed) {
        for (int size = 0; size < 2; ++size) {
            recoil::Gun g = M416();
            if (size)
                g.kick.jitter = 0.0f;   // a round of 9 degrees, yaw to 7, roll 5.4: under every one-hand cap, over the two-hand ones (6, 3, 5)
            Sim r(g, 0.0f, seed);
            r.Burst(0.0, 1, 0.0);
            while (r.End() < 0.8) {
                r.f.trigger = r.Start() < 0.05;
                if (r.Start() >= 0.03)   // the support glove's blend onto the gun over five frames (hands.cpp's kGripBlendStep)
                    r.hold.hands = std::fmin(r.hold.hands + 0.2f, 1.0f);
                const recoil::Osc before[4] = {r.s.pitch, r.s.yaw, r.s.roll, r.s.back};
                const int shotsBefore = r.nextShot;
                r.Step();
                if (r.nextShot != shotsBefore)
                    continue;   // (a round's strike is a jump in speed)
                const recoil::Osc after[4] = {r.s.pitch, r.s.yaw, r.s.roll, r.s.back};
                const float limits[4] = {6.0f, 3.0f, 5.0f, 0.04f};
                for (int i = 0; i < 4; ++i) {
                    const float w = i == 3 ? wBack : wRise, x = std::fabs(before[i].x), v = std::fabs(before[i].v);
                    const float pull = w * w * (x + v * dt) + 2.0f * 0.75f * w * (v + w * w * x * dt);
                    const float most = 1.5f * (v * dt + 0.5f * pull * dt * dt) + 1e-6f;
                    worst = std::fmax(worst, std::fabs(after[i].x - before[i].x) / most);
                    if (r.hold.hands >= 1.0f && std::fabs(after[i].x) > limits[i])
                        ++capped, below = std::fmax(below, std::fabs(after[i].x) / limits[i]);
                }
            }
        }
    }
    Check("24", worst <= 1.0f && capped > 0, "the hold taken 30 ms into a one-hand kick: never a jump, a cap now below the state holding it, never cutting it",
          "every frame within %.0f%% of its own motion's bound; %d frames with two hands above a two-hand cap (to %.2f x it), brought down by the springs",
          worst * 100, capped, below);
}

// ---- 28. Never break: a gun held, no round fired ----

void Check28() {
    Sim r(M416(), 0.0f, 28);
    std::memcpy(r.f.eyeAt, kEye, sizeof(kEye));
    r.f.haveEye = true, r.f.glassZ = kEye[2] + 0.035f, r.f.haveGlass = true;
    bool same = true, spreadOk = true;
    float slack = 0, pulse = 0;
    double twoSince = -1;
    while (r.End() < 2.0) {
        r.f.trigger = r.Start() > 0.2;                          // the button down, no round (a dry trigger, or the round not yet out)
        r.hold.eye = r.Start() > 0.5 && r.Start() < 1.2;        // the sight at the eye and away
        r.hold.hands = r.Start() > 0.8 ? 1.0f : 0.0f;           // one hand, then two
        r.hold.leftTrigger = r.Start() > 1.5;
        if (r.hold.hands > 0.5f && twoSince < 0)
            twoSince = r.Start();
        r.Step();
        same = same && Identity(r.out);
        slack = std::fmax(slack, r.out.slack), pulse = std::fmax(pulse, std::fmax(r.out.pulse[0], r.out.late));
        // The one thing that does change before a round (the critic's 1): the game's hip scatter, by the hold alone.
        if (r.hold.hands < 0.5f)
            spreadOk = spreadOk && r.out.hipSpread;
        else if (r.Now() - twoSince >= 0.2 + 1.5 / 72.0)
            spreadOk = spreadOk && !r.out.hipSpread;
    }
    // Two hands throughout, the same eye and triggers: the hip scatter never comes on by itself.
    Sim two(M416(), 1.0f, 28);
    std::memcpy(two.f.eyeAt, kEye, sizeof(kEye));
    two.f.haveEye = true, two.f.glassZ = kEye[2] + 0.035f, two.f.haveGlass = true;
    bool never = true;
    while (two.End() < 2.0) {
        two.f.trigger = two.Start() > 0.2;
        two.hold.eye = two.Start() > 0.5 && two.Start() < 1.2;
        two.hold.leftTrigger = two.Start() > 1.5;
        two.Step();
        never = never && !two.out.hipSpread && Identity(two.out);
    }
    Check("28", same && slack == 0.0f && pulse == 0.0f, "never break: a gun held, no round fired (the button down, the eye, the hold changing)",
          "%s, slack %.1e, pulse %.1e", same ? "the identity every frame" : "NOT the identity", slack, pulse);
    Check("28", spreadOk && never, "never break: before any round only the game's hip scatter changes, and by the hold alone",
          "%s with one hand, off 0.2 s after two take it; with two hands throughout %s", spreadOk ? "on" : "NOT AS THE HOLD", never ? "never on" : "ON BY ITSELF");
    // recoil.txt "kick 0" and "drift 0" (probe P2's set-up: the game's own kick read alone): 30 rounds, one hand and two, and the drawn gun never
    // moves (its buzz still comes).
    bool still = true;
    float buzz = 0;
    for (int hands = 0; hands < 2; ++hands) {
        Sim q(M416(), static_cast<float>(hands), 28);
        q.tune.kick = 0.0f, q.tune.drift = 0.0f, q.tune.release = true;
        q.Burst(0.1, 30, 0.08);
        while (q.End() < 3.5) {
            q.f.trigger = q.Start() < 2.5;
            q.Step();
            still = still && Identity(q.out);
            buzz = std::fmax(buzz, q.out.pulse[0]);
        }
    }
    Check("28", still && buzz > 0.0f, "kick 0 and drift 0 (probe P2's set-up): 30 rounds, one hand and two, and the drawn gun never moves",
          "%s; its buzz %.2f", still ? "the identity every frame" : "IT MOVED", buzz);
}

// ---- 25. The flip after held fire ----

void Check25() {
    for (int on = 0; on < 2; ++on) {
        Sim r(M416(), 1.0f, 25);
        r.tune.release = on != 0;
        r.Burst(0.0, 10, 0.08);
        float up = 0, down = 0;
        while (r.End() < 2.5) {
            r.f.trigger = r.Start() < 0.75;
            r.Step();
            up = std::fmax(up, r.s.flip.x), down = std::fmin(down, r.s.flip.x);
        }
        if (on)
            Check("25", Near(up, 2.7f, 0.05f) && down < -1.0f && down > -2.5f, "release 1: the flip after held fire", "up %.2f deg (2.7), down to %.2f",
                  up, down);
        else
            Check("25", up == 0.0f && down == 0.0f, "release 0: no flip", "up %.2f, down %.2f", up, down);
    }
}

// ---- 26. The hip spread ----

void Check26() {
    auto wanted = [](recoil::Kind kind, float hands, bool on) {
        recoil::Gun g = M416();
        g.kind = kind;
        recoil::Hold hold;
        hold.held = true, hold.hands = hands;
        recoil::Tune tune;
        tune.oneHandSpread = on;
        recoil::State s;
        recoil::Frame f;
        f.dt = 1.0f / 72.0f;
        return recoil::Step(g, hold, f, tune, s).hipSpread;
    };
    const bool ok = wanted(recoil::Kind::kRifle, 0.0f, true) && wanted(recoil::Kind::kSmg, 0.3f, true) && wanted(recoil::Kind::kShotgun, 0.0f, true) &&
                    !wanted(recoil::Kind::kRifle, 1.0f, true) && !wanted(recoil::Kind::kRifle, 0.0f, false) && !wanted(recoil::Kind::kPistol, 0.0f, true) &&
                    !wanted(recoil::Kind::kRocket, 0.0f, true) && !wanted(recoil::Kind::kM320, 0.0f, true);
    Check("26", ok, "the game's hip spread wanted: one hand on a long gun only", "%s", ok ? "rifle, SMG, shotgun in one hand; not two hands, a pistol, a launcher, or off" : "WRONG");
    // Settled here, in one place (skin::HoldSpread writes what it's given): a hold taken afresh at once; while held, the support hand's blend past a
    // half for under 0.2 s (15 frames at 72 fps) changes nothing, for 0.2 s it does; while nothing holds the gun (no round can leave it), as it was
    // last held, so a quick re-take never finds the rows changed under it.
    struct Span { int frames; float hands; bool held; bool want; } spans[] = {
        {10, 1.0f, true, false},   // two hands from the start: the zoomed scatter at once
        {3, 0.0f, true, false},    // the support hand off for 3 frames: nothing
        {7, 1.0f, true, false},
        {14, 0.0f, true, false},   // off 14 frames (0.19 s): still nothing
        {16, 0.0f, true, true},    // ... the 15th (0.21 s): the hip scatter
        {2, 1.0f, true, true},     // on again for 2 frames: nothing
        {8, 0.0f, true, true},
        {14, 1.0f, true, true},    // on 14 frames: still the hip scatter
        {6, 1.0f, true, false},    // ... the 15th: the zoomed
        {3, 0.0f, false, false},   // let go: as it was held
        {4, 0.0f, true, true},     // taken again in one hand: at once
        {3, 0.0f, false, true},    // let go: as it was held
        {4, 0.0f, true, true},     // taken again in one hand: still
        {3, 1.0f, false, true},    // let go
        {4, 1.0f, true, false},    // taken again in two: at once
    };
    Sim r(M416(), 1.0f, 26);
    int frame = 0, wrong = 0, firstWrong = -1;
    for (const Span& span : spans)
        for (int i = 0; i < span.frames; ++i, ++frame) {
            r.hold.hands = span.hands, r.hold.held = span.held;
            r.Step();
            if (r.out.hipSpread != span.want && !wrong++)
                firstWrong = frame;
        }
    Check("26", wrong == 0, "the hip spread settled: a change of hold counts once it has lasted 0.2 s, a hold taken afresh at once, let go as it was",
          "%d of %d frames wrong%s", wrong, frame, wrong ? " (see -v)" : "");
    if (wrong && Verbose("26"))
        std::printf("  the first wrong at frame %d\n", firstWrong);
}

// ---- 27. The cost ----

void Check27() {
    recoil::Gun g = M416();
    recoil::Hold hold;
    hold.held = true, hold.hands = 1.0f, hold.eye = true;
    recoil::Tune tune;
    recoil::State s;
    recoil::Frame f;
    f.dt = 0.25f, f.shots = 3, f.trigger = true;
    std::memcpy(f.pivot, kGrip, sizeof(f.pivot));
    std::memcpy(f.eyeAt, kEye, sizeof(kEye));
    f.haveEye = true, f.glassZ = kEye[2] + 0.035f, f.haveGlass = true;
    LARGE_INTEGER freq, a, b;
    QueryPerformanceFrequency(&freq);
    double best = 1e9;
    float sink = 0;
    const long allocations = g_allocations;
    for (int batch = 0; batch < 20; ++batch) {
        QueryPerformanceCounter(&a);
        for (int i = 0; i < 200; ++i) {
            const recoil::Output o = recoil::Step(g, hold, f, tune, s);
            sink += o.k[3];
        }
        QueryPerformanceCounter(&b);
        best = std::fmin(best, static_cast<double>(b.QuadPart - a.QuadPart) / static_cast<double>(freq.QuadPart) / 200.0);
    }
    // And the ordinary frame, for scale.
    f.dt = 1.0f / 72.0f, f.shots = 1;
    double ordinary = 1e9;
    for (int batch = 0; batch < 20; ++batch) {
        QueryPerformanceCounter(&a);
        for (int i = 0; i < 200; ++i) {
            const recoil::Output o = recoil::Step(g, hold, f, tune, s);
            sink += o.k[3];
        }
        QueryPerformanceCounter(&b);
        ordinary = std::fmin(ordinary, static_cast<double>(b.QuadPart - a.QuadPart) / static_cast<double>(freq.QuadPart) / 200.0);
    }
    const bool none = g_allocations == allocations;
    Check("27", best * 1e6 < 20.0 && none, "a Step at dt 0.25 with 3 rounds", "%.2f us (20); a 72 fps frame with a round %.2f us; %s%s", best * 1e6,
          ordinary * 1e6, none ? "no allocation" : "ALLOCATES", std::isfinite(sink) ? "" : " (not finite)");
}

}  // namespace

// Every allocation counted (a Step must make none).
void* operator new(size_t size) {
    ++g_allocations;
    if (void* p = std::malloc(size ? size : 1))
        return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, size_t) noexcept { std::free(p); }

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "-v") == 0 && i + 1 < argc)
            g_verbose = argv[++i];
    std::printf("recoiltest: src/recoil.h, recoil for every gun. Springs stepped every %.0f ms; a string while the next round comes within %.2f gaps; "
                "one hand on a long gun: the rise x5, the back x2.5; a pistol in one hand x1.5; at the eye the slide at most %.0f mm, never within %.1f cm "
                "of the glass.\n",
                recoil::kStep * 1000, recoil::kStringGap, recoil::kEyeSlideMost * 1000, recoil::kEyeGlassLeast * 100);
    Check1();
    Check2();
    Check3();
    Check4();
    Check5();
    Check6();
    Check7();
    Check8();
    Check9();
    Check10();
    Check11();
    Check12();
    Check13();
    Check14();
    Check15();
    Check16();
    Check17();
    Check18();
    Check19();
    Check20();
    Check21();
    Check22();
    Check23();
    Check24();
    Check25();
    Check26();
    Check28();
    Check27();
    std::printf("%d of %d checks failed\n", g_failed, g_counted);
    return g_failed;
}

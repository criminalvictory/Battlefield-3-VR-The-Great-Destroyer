// The look angles' search rules and a held gun's bullets' line (src/aimrules.h, header only, as aim.cpp and skin.cpp run them), without the game.
// The rules that stood before this round are written out here as they were at 40be42d (aim.cpp's Pairs test, VehicleTurn and the pitch-alone pass's
// turn, MouseAim's step, Update's pull, skin::Sight's line), and every rule "without find" is checked against them bit for bit.
//   1  StrictPair: the walk's own test (0.0099 passes, 0.0101 fails, the yaw and the pitch, across the wrap, any yaw from 0.02 to 7), and equal to
//      40be42d's expression on two million random pairs (NaN, infinities, huge and tiny floats among them)
//   2  NearPair: 0.049 passes, 0.051 fails; a zero yaw or pitch never; any yaw: the pitch alone and a yaw from 0.02 to 7; a camera yaw of -0.01 and a
//      slot yaw of 6.27 (the same look) pass
//   3  a walk against a turning look (the slot updated at the game's ticks, 30 a second, its camera a tick behind, the slot read 203 ms into the walk):
//      40be42d's test (the camera read once, at the walk's start) keeps it at 2 degrees a second and loses it at 5; the near test with the camera read at
//      the window keeps it at 20 and 80 and loses it at 90; the strict test after the hold (the look held, the camera settled) at every speed
//   4  PitchPassWanted: with find, not with two of the look's copies at the camera's yaw, whatever crowded says never; without, as before
//   5  ProofTurn: none on foot (equal, 0.003, 1.4 degrees, across the wrap), the train's 14 degrees kept; 40be42d's turn from a camera read 1.5 s before
//      while the look turned 10 degrees a second: 15 degrees, the proof's: none
//   6  MouseStep, single steps: the cut step as before (and bit for bit 40be42d's on two million random gaps), the last step after 4 quiet frames, the
//      150 cap, nothing for a gap not finite, nothing straight back after a last step
//   7  mouse aim's loop (the game taking each count 1 or 2 frames late, fixed or at random, a count really 0.20 to 0.29 degrees, starts from -3 to +3
//      degrees, 150 frames): with the last step all end within 0.15 degrees, nothing sent in the last 10 frames; without, within 0.5 as before; plain
//      rounding of half the gap no nearer (within a count, still sending late); a count of 0.35 degrees (the review's case) swings with a last step that
//      may go straight back, and comes to rest within a count with it held; a count of 0.5 degrees 2 frames late makes the half-gap step itself swing,
//      as it did before, the last step adding nothing to it; a hand turning 20 degrees a second trails no more than a count beyond the cut step's
//   8  Proven: one address all life through finds and losses gives what 40be42d's one pair gave; another address comes first, the first second; a fifth
//      drops the oldest; the same again moves to the front; a new soldier forgets them; nothing for 0
//   9  waiting (KnownLikely, RouteLikely, StrikesAfter): the newest as before whatever its pitch, an older one by its pitch; none proved before a take
//      (the user's try: 40be42d waited 15 s after the take); 5 strikes in a row end it, a busy camera or a proof start them again; the route's for a
//      soldier it wasn't proven on, by its pitch
//  10  PullTeaches: a film's pull with the yaw moved teaches nothing with find (-11.6 without, as before); a real clamp with the yaw left alone does;
//      nothing under 52 degrees, nothing read beyond the write; without find equal to 40be42d's on two million random pulls
//  11  LimitProbe on a simulated game (its ticks 30 a second, frames 72): a real clamp pulls the try back and it stands; a false limit held 100 ms is
//      dropped; the game writing the slot over from another (the yaw moved) drops nothing; one try in 2 s; never without find or with nothing learned
//  12  HeldShotLine: no glass, every held gun's sights bit for bit; the AN-94 with the PSO-1's glass on its line; a raised gun's line as Sight's; equal to
//      40be42d's Sight line bit for bit on two million random glasses; a glass not finite: the sights
//  13  broken inputs (NaN, infinities in every argument): nothing not finite out of ProofTurn and HeldShotLine, no pass from a test; PitchPassGoesOn; each
//      rule in well under a microsecond
// Usage: aimrulestest
// The exit code is the number of failed checks.

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <limits>
#include <random>
#include <vector>

#include "../src/aimrules.h"

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
constexpr float kDeg = kPi / 180.0f;
constexpr float kNaN = std::numeric_limits<float>::quiet_NaN();
constexpr float kInf = std::numeric_limits<float>::infinity();

// ---- As they were at 40be42d (src/aim.cpp, src/skin.cpp), copied ----
bool OldLooksLikeAngle(float v, bool pitch) {
    if (!std::isfinite(v))
        return false;
    const float size = std::fabs(v);
    return (size == 0.0f || size >= 1e-6f) && size <= (pitch ? 1.6f : 1000.0f);
}
float OldYawApart(float a, float b) {
    float d = std::fmod(a - b, 2.0f * kPi);
    if (d > kPi)
        d -= 2.0f * kPi;
    if (d < -kPi)
        d += 2.0f * kPi;
    return std::fabs(d);
}
bool OldPairTest(float yaw, float pitch, float cameraYaw, float cameraPitch, bool anyYaw) {
    return OldLooksLikeAngle(yaw, false) &&
           (anyYaw ? std::fabs(yaw) >= 0.02f && std::fabs(yaw) < 7.0f : OldYawApart(yaw, cameraYaw) < 0.01f) &&
           std::fabs(pitch - cameraPitch) < 0.01f;
}
constexpr float kOldNoVehicle = 1.5f / 57.2957795f;
float OldVehicleTurn(float off) {
    return std::fabs(off) < kOldNoVehicle ? 0.0f : off;
}
long OldMouseStep(float gap) {  // MouseAim's dx (and dy with the gap negated): kMousePixelsPerRadian 230
    return static_cast<long>(std::clamp(gap * 230.0f * 0.5f, -150.0f, 150.0f));
}
int OldPull(float written, float read, float pitchMax, float pitchMin) {  // Update, kSteep 0.9
    if (std::fabs(read - written) > 0.002f) {
        if (written > 0.9f && read < written && read < pitchMax)
            return 1;
        if (written < -0.9f && read > written && read > pitchMin)
            return -1;
    }
    return 0;
}
void OldSightLine(const float* sights, float up, const float* glass, float* line) {  // skin::Sight
    std::memcpy(line, sights, 3 * sizeof(float));
    line[0] = glass[0];
    line[1] = glass[1] - (line[2] - glass[2]) * std::tan(up * 3.14159265f / 180.0f);
}

bool SameBits(float a, float b) {
    uint32_t x = 0, y = 0;
    std::memcpy(&x, &a, 4);
    std::memcpy(&y, &b, 4);
    return x == y;
}

// A float of any kind: angles, near zeros, pointers' bits, NaN, infinities.
float AnyFloat(std::mt19937& rng) {
    std::uniform_int_distribution<int> kind(0, 9);
    std::uniform_real_distribution<float> angle(-7.0f, 7.0f), small(-0.03f, 0.03f);
    switch (kind(rng)) {
    case 0: {
        const uint32_t bits = rng();
        float f = 0.0f;
        std::memcpy(&f, &bits, 4);
        return f;
    }
    case 1:
        return small(rng);
    case 2:
        return kNaN;
    case 3:
        return (rng() & 1) ? kInf : -kInf;
    case 4:
        return 0.0f;
    default:
        return angle(rng);
    }
}

// ---- 3: a walk against a turning look ----
// The look turns at w rad/s from 0.3; the game updates the slot at its ticks (every 1/30 s) with the look then, and builds the camera from the slot of
// the tick before (a tick behind). Returns the slot's value and the camera's at time t (s).
struct Turning {
    double w;
    double stop = 1e9;  // the look held from then (mouse aim holding its corrections)
    double LookAt(double t) const { return 0.3 + w * std::min(t, stop); }
    double SlotAt(double t) const { return LookAt(std::floor(t * 30.0) / 30.0); }
    double CameraAt(double t) const { return LookAt((std::floor(t * 30.0) - 1.0) / 30.0); }
};

// ---- 7: mouse aim's loop ----
struct LoopEnd {
    double endDeg;       // the gap at the end
    int lastSent;        // counts sent in the last 10 frames
    double meanLateDeg;  // the mean of the gap's size over frames 50 to 150
};
enum class Stepper { kCut, kLast, kRound, kLastNoGuard };
// Frames at 72 Hz: each frame the gap (the hand's aim less the game's camera) gives counts; the game takes each frame's counts `delay` frames later (1
// or 2; 0: 1 or 2 at random), each a real count of countDeg. The hand turns handDegPerSec.
LoopEnd RunLoop(double startDeg, double countDeg, int delay, Stepper stepper, double handDegPerSec, unsigned seed) {
    std::mt19937 rng(seed);
    aimrules::MouseAxis axis;
    std::deque<std::pair<int, long>> due;  // (frame, counts)
    double camera = -startDeg * kDeg, aim = 0.0;
    int lastStep = 0;  // (kLastNoGuard's own)
    int quiet = 0;
    LoopEnd out{0.0, 0, 0.0};
    int lateFrames = 0;
    for (int f = 0; f < 150; ++f) {
        aim = handDegPerSec * kDeg * f / 72.0;
        while (!due.empty() && due.front().first <= f) {
            camera += due.front().second * countDeg * kDeg;
            due.pop_front();
        }
        const float gap = static_cast<float>(aim - camera);
        long n = 0;
        switch (stepper) {
        case Stepper::kCut:
            n = aimrules::MouseStep(gap, 230.0f, axis, false);
            break;
        case Stepper::kLast:
            n = aimrules::MouseStep(gap, 230.0f, axis, true);
            break;
        case Stepper::kRound:
            n = std::lround(std::clamp(gap * 230.0f * 0.5f, -150.0f, 150.0f));
            break;
        case Stepper::kLastNoGuard: {
            // The last step without its guard against going straight back (the review's worry): one count whenever past 0.6 after 4 quiet frames.
            const long cut = static_cast<long>(std::clamp(gap * 230.0f * 0.5f, -150.0f, 150.0f));
            if (cut) {
                n = cut, quiet = 0;
            } else if (quiet >= 4 && std::fabs(gap * 230.0f) > 0.6f) {
                n = gap > 0.0f ? 1 : -1, quiet = 0;
            } else {
                ++quiet;
            }
            (void)lastStep;
            break;
        }
        }
        if (n) {
            const int d = delay ? delay : 1 + static_cast<int>(rng() % 2);
            due.push_back({f + d, n});
            std::sort(due.begin(), due.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
            if (f >= 140)
                out.lastSent += static_cast<int>(std::labs(n));
        }
        if (f >= 50) {
            out.meanLateDeg += std::fabs(gap) / kDeg;
            ++lateFrames;
        }
    }
    while (!due.empty()) {
        camera += due.front().second * countDeg * kDeg;
        due.pop_front();
    }
    out.endDeg = (aim - camera) / kDeg;
    out.meanLateDeg /= std::max(lateFrames, 1);
    return out;
}

// ---- 11: a simulated game for the probe ----
// The slot's pitch as the game keeps it: the mod writes it each frame (72 a second); at each of its ticks (30 a second) the game clamps it at `clamp`
// (none: -10), or writes it over from another slot holding `other` (moving the yaw too). The probe runs as Update runs it.
struct ProbeRun {
    int drops = 0, tries = 0, framesPast = 0;
    int dropSide = 0;
};
ProbeRun RunProbe(float learnedMin, float wanted, float clamp, bool copyOver, float other, bool may, int frames) {
    aimrules::LimitProbe probe;
    float slot = -0.3f, written = -0.3f, pitchMin = learnedMin;
    ProbeRun out;
    uint64_t now = 100000;
    double nextTick = 100000.0 + 7.0;
    bool havePitch = false;
    bool yawLeftAlone = true;
    for (int f = 0; f < frames; ++f) {
        now = 100000 + static_cast<uint64_t>(f * 1000.0 / 72.0);
        // The game's ticks since the last frame.
        yawLeftAlone = true;
        while (nextTick <= static_cast<double>(now)) {
            if (copyOver)
                slot = other, yawLeftAlone = false;
            else if (slot < clamp)
                slot = clamp;
            nextTick += 1000.0 / 30.0;
        }
        const float read = slot;
        if (probe.side && havePitch) {
            if (const int drop = probe.Read(written, read, yawLeftAlone, now)) {
                ++out.drops, out.dropSide = drop;
                if (drop < 0)
                    pitchMin = -aimrules::kPitchDefault;
            }
        }
        float pitch = std::clamp(wanted, pitchMin, aimrules::kPitchDefault);
        const int sideBefore = probe.side;
        if (probe.Past(wanted, aimrules::kPitchDefault, pitchMin, may && havePitch, now)) {
            pitch = std::clamp(wanted, -aimrules::kPitchDefault, aimrules::kPitchDefault);
            ++out.framesPast;
            if (!sideBefore)
                ++out.tries;
        }
        slot = written = pitch;
        havePitch = true;
    }
    return out;
}

}  // namespace

int main() {
    std::mt19937 rng(20261113);

    // ---- 1 ----
    {
        const float cy = 1.0f, cp = -0.3f;
        const bool a = aimrules::StrictPair(1.0099f, cp, cy, cp, false), b = !aimrules::StrictPair(1.0101f, cp, cy, cp, false);
        const bool c = aimrules::StrictPair(cy, -0.3099f, cy, cp, false), d = !aimrules::StrictPair(cy, -0.3101f, cy, cp, false);
        const bool e = aimrules::StrictPair(6.2800f, cp, -0.0031f, cp, false);
        const bool g = !aimrules::StrictPair(0.019f, cp, 5.0f, cp, true) && aimrules::StrictPair(6.9f, cp, 5.0f, cp, true) &&
                       !aimrules::StrictPair(7.01f, cp, 5.0f, cp, true) && aimrules::StrictPair(-6.9f, cp, 5.0f, cp, true);
        Report("1a", "StrictPair: the walk's own test", a && b && c && d && e && g, "0.0099 %d, 0.0101 refused %d; pitch %d %d; across the wrap %d; any yaw %d", a, b, c,
               d, e, g);
        int differ = 0;
        for (int k = 0; k < 2000000; ++k) {
            const float y = AnyFloat(rng), p = AnyFloat(rng), camY = AnyFloat(rng), camP = (k % 3) ? p + (static_cast<float>(rng() % 2001) - 1000.0f) * 2e-5f : AnyFloat(rng);
            const bool any = rng() & 1;
            differ += aimrules::StrictPair(y, p, camY, camP, any) != OldPairTest(y, p, camY, camP, any);
        }
        Report("1b", "StrictPair equal to 40be42d's test", differ == 0, "%d of 2000000 random pairs differ", differ);
    }

    // ---- 2 ----
    {
        const float cy = 1.0f, cp = -0.3f;
        const bool a = aimrules::NearPair(1.049f, cp, cy, cp, false) && !aimrules::NearPair(1.051f, cp, cy, cp, false);
        const bool b = aimrules::NearPair(cy, -0.349f, cy, cp, false) && !aimrules::NearPair(cy, -0.351f, cy, cp, false);
        const bool c = !aimrules::NearPair(0.0f, cp, 0.03f, cp, false) && !aimrules::NearPair(cy, 0.015f, cy, 0.03f, false) && !aimrules::NearPair(0.01f, cp, 0.03f, cp, true);
        const bool d = aimrules::NearPair(5.0f, cp, 1.0f, -0.29f, true) && !aimrules::NearPair(7.01f, cp, 1.0f, cp, true) && !aimrules::NearPair(5.0f, cp, 1.0f, -0.36f, true);
        const bool e = aimrules::NearPair(6.27f, -0.2f, -0.01f, -0.2f, false);
        // The pass by the pitch alone's walk: within kStrict of the camera's pitch at the window.
        const bool f = aimrules::NearPair(5.0f, -0.3099f, 1.0f, cp, true, aimrules::kStrict) && !aimrules::NearPair(5.0f, -0.3101f, 1.0f, cp, true, aimrules::kStrict) &&
                       !aimrules::NearPair(0.01f, -0.3f, 1.0f, cp, true, aimrules::kStrict);
        Report("2", "NearPair: within 0.05 at the window, no zeros", a && b && c && d && e && f,
               "yaw %d, pitch %d, zeros refused %d, any yaw %d, 6.27 against -0.01 %d, the pitch alone within 0.01 %d", a, b, c, d, e, f);
    }

    // ---- 3 ----
    {
        auto keeps = [](double degPerSec, int test) {
            const Turning look{degPerSec * kDeg};
            const double start = 0.0, read = 0.203;  // the walk's start, the slot's window 203 ms in
            const float slot = static_cast<float>(look.SlotAt(read)), pitch = -0.3f;
            if (test == 0)  // 40be42d: the camera read once, at the walk's start
                return aimrules::StrictPair(slot, pitch, static_cast<float>(look.CameraAt(start)), pitch, false);
            if (test == 1)  // the near test, the camera read just after the window
                return aimrules::NearPair(slot, pitch, static_cast<float>(look.CameraAt(read + 0.0005)), pitch, false);
            // After the hold: the look held from 0.6 s (the walk over), the camera settled 60 ms on, the pair and the camera read at one instant.
            Turning held = look;
            held.stop = 0.6;
            const double at = held.stop + 0.06 + 0.005;
            return aimrules::StrictPair(static_cast<float>(held.SlotAt(at)), pitch, static_cast<float>(held.CameraAt(at)), pitch, false);
        };
        const bool oldOk = keeps(2, 0) && !keeps(5, 0);
        const bool nearOk = keeps(20, 1) && keeps(80, 1) && !keeps(90, 1);
        bool heldOk = true;
        for (double v : {2.0, 20.0, 80.0, 90.0, 200.0, 500.0})
            heldOk = heldOk && keeps(v, 2);
        Report("3", "a walk against a turning look", oldOk && nearOk && heldOk,
               "40be42d's keeps 2 deg/s %d, loses 5 %d; the near test keeps 20 %d and 80 %d, loses 90 %d; strict after the hold at every speed %d", keeps(2, 0),
               !keeps(5, 0), keeps(20, 1), keeps(80, 1), !keeps(90, 1), heldOk);
    }

    // ---- 4 ----
    {
        const bool on = aimrules::PitchPassWanted(-1, false, true) && aimrules::PitchPassWanted(0, false, true) && aimrules::PitchPassWanted(1, false, true) &&
                        !aimrules::PitchPassWanted(2, false, true) && !aimrules::PitchPassWanted(5, false, true) && !aimrules::PitchPassWanted(0, true, true);
        const bool off = aimrules::PitchPassWanted(0, false, false) && aimrules::PitchPassWanted(5, false, false) && !aimrules::PitchPassWanted(5, true, false);
        Report("4", "PitchPassWanted", on && off, "with find %d, without (as before) %d", on, off);
    }

    // ---- 5 ----
    {
        const bool a = aimrules::ProofTurn(0.5f, 0.5f) == 0.0f && aimrules::ProofTurn(0.503f, 0.5f) == 0.0f && aimrules::ProofTurn(0.5f + 0.0244f, 0.5f) == 0.0f;
        const float train = aimrules::ProofTurn(0.5f + 0.2443f, 0.5f);
        const bool b = std::fabs(train - 0.2443f) < 1e-5f;
        const bool c = aimrules::ProofTurn(-3.13f, 3.13f) == 0.0f;
        // The pass by the pitch alone before: the camera's yaw at its start, the pair's 1.5 s later, the look turning 10 degrees a second.
        const float cameraAtStart = 1.0f, pairLater = cameraAtStart - 15.0f * kDeg;
        const float before = OldVehicleTurn(std::remainder(cameraAtStart - pairLater, 2.0f * kPi)), now = aimrules::ProofTurn(pairLater, pairLater);
        const bool d = std::fabs(before / kDeg - 15.0f) < 0.01f && now == 0.0f;
        bool same = true;
        for (int k = 0; k < 200000; ++k) {
            const float off = (static_cast<float>(rng() % 200001) - 100000.0f) * 1e-5f;
            same = same && SameBits(aimrules::VehicleTurn(off), OldVehicleTurn(off));
        }
        Report("5", "ProofTurn: the vehicle's turn at the proof", a && b && c && d && same,
               "on foot none %d; 14 degrees kept %d (%.4f); across the wrap %d; 40be42d's pass %.1f degrees, the proof's %.1f %d; VehicleTurn as before %d", a, b, train, c,
               before / kDeg, now / kDeg, d, same);
    }

    // ---- 6 ----
    {
        const float gaps[] = {0.1f, 0.2f, 0.3f, 0.45f, 0.6f, 1.0f, 2.0f, -0.3f};
        const long cutWanted[] = {0, 0, 0, 0, 1, 2, 4, 0}, lastWanted[] = {0, 1, 1, 1, 1, 2, 4, -1};
        bool cutOk = true, lastOk = true, busyOk = true;
        char got[160] = {};
        int used = 0;
        for (size_t k = 0; k < std::size(gaps); ++k) {
            aimrules::MouseAxis off, recent, quiet;
            quiet.quiet = 4;
            recent.quiet = 2;
            const long c = aimrules::MouseStep(gaps[k] * kDeg, 230.0f, off, false), r = aimrules::MouseStep(gaps[k] * kDeg, 230.0f, recent, true),
                       q = aimrules::MouseStep(gaps[k] * kDeg, 230.0f, quiet, true);
            cutOk = cutOk && c == cutWanted[k];
            busyOk = busyOk && r == cutWanted[k];
            lastOk = lastOk && q == lastWanted[k];
            used += std::snprintf(got + used, sizeof(got) - used, "%s%ld/%ld", k ? " " : "", c, q);
        }
        aimrules::MouseAxis a, b, n1, n2;
        const bool cap = aimrules::MouseStep(80.0f * kDeg, 230.0f, a, true) == 150 && aimrules::MouseStep(-80.0f * kDeg, 230.0f, b, false) == -150;
        n1.quiet = 10, n2.quiet = 10;
        const bool broken = aimrules::MouseStep(kNaN, 230.0f, n1, true) == 0 && aimrules::MouseStep(kInf, 230.0f, n2, true) == 0 &&
                            aimrules::MouseStep(-kInf, 230.0f, n2, false) == 0;
        // Straight back after a last step: not sent; the same way again: sent; after a cut step: free again.
        aimrules::MouseAxis g;
        g.quiet = 4;
        const long s1 = aimrules::MouseStep(0.2f * kDeg, 230.0f, g, true);
        g.quiet = 4;
        const long s2 = aimrules::MouseStep(-0.2f * kDeg, 230.0f, g, true);
        g.quiet = 4;
        const long s3 = aimrules::MouseStep(0.2f * kDeg, 230.0f, g, true);
        const long s4 = aimrules::MouseStep(1.0f * kDeg, 230.0f, g, true);
        g.quiet = 4;
        const long s5 = aimrules::MouseStep(-0.2f * kDeg, 230.0f, g, true);
        const bool guard = s1 == 1 && s2 == 0 && s3 == 1 && s4 == 2 && s5 == -1;
        int differ = 0;
        for (int k = 0; k < 2000000; ++k) {
            const float gap = (k % 4) ? (static_cast<float>(rng() % 2000001) - 1000000.0f) * 1e-6f : (static_cast<float>(rng() % 20001) - 10000.0f) * 1e-3f;
            aimrules::MouseAxis ax;
            ax.quiet = static_cast<int>(rng() % 8);
            differ += aimrules::MouseStep(gap, 230.0f, ax, false) != OldMouseStep(gap);
        }
        Report("6", "MouseStep: single steps", cutOk && lastOk && busyOk && cap && broken && guard && differ == 0,
               "cut/last %s; counts sent lately: the cut %d; the cap %d; not finite none %d; straight back held %d (%ld %ld %ld %ld %ld); %d of 2000000 differ from 40be42d's",
               got, busyOk, cap, broken, guard, s1, s2, s3, s4, s5, differ);
    }

    // ---- 7 ----
    {
        double worstOn = 0.0, worstOff = 0.0;
        int sentOn = 0;
        for (double count : {0.20, 0.246, 0.29})
            for (int delay : {1, 2, 0})
                for (double start = -3.0; start <= 3.001; start += 0.25)
                    for (unsigned seed : {1u, 2u}) {
                        const LoopEnd on = RunLoop(start, count, delay, Stepper::kLast, 0.0, seed), off = RunLoop(start, count, delay, Stepper::kCut, 0.0, seed);
                        worstOn = std::max(worstOn, std::fabs(on.endDeg));
                        worstOff = std::max(worstOff, std::fabs(off.endDeg));
                        sentOn += on.lastSent;
                    }
        Report("7a", "mouse aim's loop with the last step", worstOn <= 0.15 && sentOn == 0, "worst end %.3f degrees (0.15 at most), counts in the last 10 frames %d",
               worstOn, sentOn);
        Report("7b", "mouse aim's loop without it, as before", worstOff <= 0.50, "worst end %.3f degrees (0.50 at most)", worstOff);
        // Plain rounding of half the gap (the other way to close the last count): in this model it comes to rest too, but only within a count.
        double worstRound = 0.0;
        int roundSent = 0;
        for (double count : {0.20, 0.246, 0.29})
            for (int delay : {1, 2, 0})
                for (double start = -3.0; start <= 3.001; start += 0.25) {
                    const LoopEnd r = RunLoop(start, count, delay, Stepper::kRound, 0.0, 1);
                    worstRound = std::max(worstRound, std::fabs(r.endDeg));
                    roundSent += r.lastSent;
                }
        Report("7c", "plain rounding of half the gap: no nearer than the last step", worstRound >= worstOn,
               "its worst end %.3f degrees, the last step's %.3f; its counts in the last 10 frames %d", worstRound, worstOn, roundSent);
        // A count of 0.35 degrees (the review: a count more than 0.30 crosses the band, and a last step free to go straight back swings).
        int cycleFree = 0, cycleHeld = 0;
        double restHeld = 0.0;
        for (int delay : {1, 2})
            for (double start = -3.0; start <= 3.001; start += 0.25) {
                cycleFree += RunLoop(start, 0.35, delay, Stepper::kLastNoGuard, 0.0, 3).lastSent;
                const LoopEnd held = RunLoop(start, 0.35, delay, Stepper::kLast, 0.0, 3);
                cycleHeld += held.lastSent;
                restHeld = std::max(restHeld, std::fabs(held.endDeg));
            }
        Report("7d", "a count of 0.35 degrees: no swing with the last step held", cycleFree > 0 && cycleHeld == 0 && restHeld <= 0.35,
               "a last step free to go back: %d counts in the last 10 frames; held: %d, at rest within %.3f degrees (a count at most)", cycleFree, cycleHeld, restHeld);
        // A count of 0.5 degrees, twice what mouse aim takes it for, 2 frames late: the half-gap step itself swings (so it did at 40be42d); the last
        // step adds no swing of its own to it.
        int cutSwing = 0, lastSwing = 0;
        for (double start = -3.0; start <= 3.001; start += 0.25) {
            cutSwing += RunLoop(start, 0.5, 2, Stepper::kCut, 0.0, 3).lastSent;
            lastSwing += RunLoop(start, 0.5, 2, Stepper::kLast, 0.0, 3).lastSent;
        }
        Report("7e", "a count of 0.5 degrees 2 frames late: the half-gap step's own swing, as before", cutSwing > 0 && lastSwing <= cutSwing,
               "counts in the last 10 frames: the cut step alone %d, with the last step %d", cutSwing, lastSwing);
        double lagOn = 0.0, lagOff = 0.0;
        for (unsigned seed : {1u, 2u, 3u}) {
            lagOn = std::max(lagOn, RunLoop(0.0, 0.246, 0, Stepper::kLast, 20.0, seed).meanLateDeg);
            lagOff = std::max(lagOff, RunLoop(0.0, 0.246, 0, Stepper::kCut, 20.0, seed).meanLateDeg);
        }
        Report("7f", "a hand turning 20 degrees a second", lagOn <= lagOff + 0.246, "mean gap %.3f degrees with the last step, %.3f without", lagOn, lagOff);
    }

    // ---- 8 ----
    {
        aimrules::Proven table;
        uintptr_t oldFor = 0, oldAddress = 0;  // 40be42d's one pair
        const uintptr_t soldier = 0xD8B037B0, slot = 0xE76567CC;
        bool same = true;
        for (int k = 0; k < 6; ++k) {  // found, lost, found again: the same address all life
            table.Remember(soldier, slot, 0);
            oldFor = soldier, oldAddress = slot;
            table.For(soldier);
            same = same && table.entry[0].address == (oldFor == soldier ? oldAddress : 0) && table.Count() == 1;
        }
        table.Remember(soldier, 0xE7705DCC, 1);
        const bool second = table.entry[0].address == 0xE7705DCC && table.entry[1].address == slot && table.entry[0].swap == 1 && table.Count() == 2;
        table.Remember(soldier, slot, 1);
        const bool front = table.entry[0].address == slot && table.entry[1].address == 0xE7705DCC && table.Count() == 2;
        table.Remember(soldier, 0x1000, 2), table.Remember(soldier, 0x2000, 2), table.Remember(soldier, 0x3000, 2);
        const bool oldest = table.Count() == 4 && table.entry[0].address == 0x3000 && table.entry[3].address == slot;
        table.Remember(soldier, 0, 3);
        const bool none = table.Count() == 4 && table.entry[0].address == 0x3000;
        table.For(0x12345678);
        const bool fresh = table.Count() == 0 && table.soldier == 0x12345678;
        Report("8", "Proven: what proved itself, per soldier", same && second && front && oldest && none && fresh,
               "one address all life as 40be42d's pair %d; another first %d; the same again to the front %d; the fifth drops the oldest %d; 0 nothing %d; a new soldier %d",
               same, second, front, oldest, none, fresh);
    }

    // ---- 9 ----
    {
        const bool newest = aimrules::KnownLikely(true, false, 0, 1e9f) && aimrules::KnownLikely(true, false, 4, 0.5f);
        const bool older = aimrules::KnownLikely(false, false, 0, 0.04f) && !aimrules::KnownLikely(false, false, 0, 0.06f);
        const bool take = !aimrules::KnownLikely(true, true, 0, 0.0f) && !aimrules::KnownLikely(false, true, 0, 0.0f);
        // The user's try: the take at 10:39:40.629, the next search at 10:39:41.751: 40be42d waited (a known address), the take's rule doesn't.
        const bool oldWaited = true /* likely = known != 0 */, nowWaits = aimrules::KnownLikely(true, /*takenSince*/ true, 0, 1e9f);
        int strikes = 0;
        bool four = true;
        for (int k = 0; k < 4; ++k)
            strikes = aimrules::StrikesAfter(strikes, aimrules::Answer::kOverwritten, false), four = four && aimrules::KnownLikely(true, false, strikes, 0.0f);
        const int fifthStrikes = aimrules::StrikesAfter(strikes, aimrules::Answer::kNearNotMatching, false);
        const bool fifth = !aimrules::KnownLikely(true, false, fifthStrikes, 0.0f);
        const bool reset = aimrules::StrikesAfter(4, aimrules::Answer::kBusy, false) == 0 && aimrules::StrikesAfter(4, aimrules::Answer::kYes, true) == 0;
        const bool kept = aimrules::StrikesAfter(2, aimrules::Answer::kKept, true) == 3 && aimrules::StrikesAfter(2, aimrules::Answer::kKept, false) == 2;
        const bool nothing = aimrules::StrikesAfter(2, aimrules::Answer::kNo, true) == 2 && aimrules::StrikesAfter(2, aimrules::Answer::kNone, false) == 2;
        const bool route = aimrules::RouteLikely(false, 0, 0.04f) && !aimrules::RouteLikely(false, 0, 0.06f) && !aimrules::RouteLikely(true, 0, 0.0f) &&
                           !aimrules::RouteLikely(false, 5, 0.0f) && aimrules::RouteLikely(false, 4, 0.0f);
        Report("9", "waiting for what proved itself", newest && older && take && oldWaited && !nowWaits && four && fifth && reset && kept && nothing && route,
               "the newest as before %d; an older by its pitch %d; none after a take %d (40be42d waited %d, now %d); 4 strikes wait %d, the 5th ends it %d; busy or a "
               "proof start again %d; kept a strike for the route alone %d; a refusal none %d; the route %d",
               newest, older, take, oldWaited, nowWaits, four, fifth, reset, kept, nothing, route);
    }

    // ---- 10 ----
    {
        const bool yawMoved = aimrules::PullTeaches(-0.975f, -0.2033f, false, true, 1.5f, -1.5f) == 0 && aimrules::PullTeaches(-0.975f, -0.2033f, false, false, 1.5f, -1.5f) == -1;
        const bool real = aimrules::PullTeaches(1.55f, 1.4835f, true, true, 1.5f, -1.5f) == 1 && aimrules::PullTeaches(-1.2f, -0.7854f, true, true, 1.5f, -1.5f) == -1;
        const bool shallow = aimrules::PullTeaches(0.85f, 0.5f, true, true, 1.5f, -1.5f) == 0 && aimrules::PullTeaches(-0.85f, -0.5f, true, false, 1.5f, -1.5f) == 0;
        const bool beyond = aimrules::PullTeaches(-1.0f, -1.2f, true, true, 1.5f, -1.5f) == 0 && aimrules::PullTeaches(1.0f, 1.2f, true, false, 1.5f, -1.5f) == 0;
        const bool narrower = aimrules::PullTeaches(-1.2f, -0.9f, true, true, 1.5f, -0.8f) == 0;
        int differ = 0;
        for (int k = 0; k < 2000000; ++k) {
            const float w = (k % 5) ? (static_cast<float>(rng() % 3001) - 1500.0f) * 1e-3f : AnyFloat(rng);
            const float r = (k % 3) ? w + (static_cast<float>(rng() % 2001) - 1000.0f) * 1e-3f : AnyFloat(rng);
            const float mx = (k % 7) ? 1.5f : (static_cast<float>(rng() % 1001)) * 1.5e-3f, mn = (k % 11) ? -1.5f : -(static_cast<float>(rng() % 1001)) * 1.5e-3f;
            differ += aimrules::PullTeaches(w, r, (rng() & 1) != 0, false, mx, mn) != OldPull(w, r, mx, mn);
        }
        Report("10", "PullTeaches: a limit learned only where the yaw was left alone", yawMoved && real && shallow && beyond && narrower && differ == 0,
               "the pull with the yaw moved: none with find, -11.6 without %d; a real clamp %d; under 52 degrees none %d; read beyond the write none %d; not narrower none "
               "%d; %d of 2000000 differ from 40be42d's",
               yawMoved, real, shallow, beyond, narrower, differ);
    }

    // ---- 11 ----
    {
        // The game's clamp at -45 degrees, real: tried, pulled back, it stands; a try every 2 s over 10 s.
        const ProbeRun real = RunProbe(-0.7854f, -1.05f, -0.7854f, false, 0.0f, true, 720);
        // A false limit (a stand-in film's -45): held 100 ms, dropped once; then nothing more to try.
        const ProbeRun none = RunProbe(-0.7854f, -1.05f, -10.0f, false, 0.0f, true, 720);
        // As filmed on 2026-11-13: the game writes the slot over from another one at -11.6 (the yaw moving too): it stands.
        const ProbeRun copied = RunProbe(-0.2025f, -0.5f, -10.0f, true, -0.2025f, true, 720);
        const ProbeRun copiedLower = RunProbe(-0.2025f, -0.5f, -10.0f, true, -0.5f, true, 720);
        const ProbeRun off = RunProbe(-0.7854f, -1.05f, -10.0f, false, 0.0f, false, 720);
        const ProbeRun nothing = RunProbe(-aimrules::kPitchDefault, -1.4f, -10.0f, false, 0.0f, true, 720);
        const ProbeRun within = RunProbe(-0.7854f, -0.5f, -10.0f, false, 0.0f, true, 720);
        const bool a = real.drops == 0 && real.tries >= 4 && real.tries <= 6 && real.framesPast <= real.tries * 3;
        const bool b = none.drops == 1 && none.dropSide == -1 && none.tries == 1;
        const bool c = copied.drops == 0 && copiedLower.drops == 0;
        const bool d = off.tries == 0 && nothing.tries == 0 && within.tries == 0;
        Report("11", "LimitProbe on a simulated game", a && b && c && d,
               "a real clamp: %d tries, %d frames past it, %d dropped; a false one: %d tries, dropped %d (side %d); written over from another slot: dropped %d, %d; without "
               "find %d tries, nothing learned %d, within the limit %d",
               real.tries, real.framesPast, real.drops, none.tries, none.drops, none.dropSide, copied.drops, copiedLower.drops, off.tries, nothing.tries, within.tries);
        bool rules = aimrules::ProbeWanted(-1.0f, 1.5f, -0.7854f, 2000, true) == -1 && aimrules::ProbeWanted(-1.0f, 1.5f, -0.7854f, 1999, true) == 0 &&
                     aimrules::ProbeWanted(1.2f, 0.9f, -1.5f, 5000, true) == 1 && aimrules::ProbeWanted(-1.0f, 1.5f, -0.7854f, 5000, false) == 0 &&
                     aimrules::ProbeWanted(kNaN, 0.9f, -0.7854f, 5000, true) == 0 && aimrules::ProbeWanted(-1.4f, 1.5f, -1.5f, 5000, true) == 0;
        rules = rules && !aimrules::ProbePulled(-1.0f, -1.0019f) && aimrules::ProbePulled(-1.0f, -1.0021f) && aimrules::ProbePulled(-1.0f, kNaN);
        Report("11b", "ProbeWanted and ProbePulled", rules, "%d", rules);
    }

    // ---- 12 ----
    {
        struct Kind {
            const char* name;
            float up, sights[3];
        };
        const Kind kinds[] = {
            {"RPK-74M", 0.0f, {0.0f, 0.086f, 1.12f}},  {"AN-94", 0.0f, {0.0f, 0.092f, 0.95f}},   {"AEK-971", 0.21f, {0.0f, 0.092f, 0.99f}},
            {"AK-74M", 0.03f, {0.0f, 0.0695f, 0.936f}}, {"G3A3", 0.48f, {0.0f, 0.1015f, 1.03f}},  {"KH2002", -0.47f, {0.0f, 0.088f, 0.711f}},
            {"M16A4", 0.0f, {0.0f, 0.097f, 1.046f}},    {"M4A1", 0.0f, {0.0f, 0.096f, 0.909f}},   {"M240", 0.22f, {0.0f, 0.083f, 1.197f}},
            {"AKS-74u", -0.42f, {0.0f, 0.0783f, 0.710f}}, {"UMP-45", 0.05f, {0.0f, 0.0965f, 0.648f}}, {"Mk11", 0.0f, {0.0f, 0.0922f, 1.004f}},
            {"M249", 0.78f, {0.0f, 0.083f, 0.935f}},     {"PKP", 0.14f, {0.0f, 0.1122f, 1.2139f}},
        };
        bool bare = true;
        for (const Kind& k : kinds) {
            float out[3] = {9.0f, 9.0f, 9.0f};
            aimrules::HeldShotLine(k.sights, k.up, nullptr, out);
            bare = bare && SameBits(out[0], k.sights[0]) && SameBits(out[1], k.sights[1]) && SameBits(out[2], k.sights[2]);
        }
        const float an94[3] = {0.0f, 0.092f, 0.95f}, pso1[3] = {0.0f, 0.127f, 0.263f};
        float line[3] = {};
        aimrules::HeldShotLine(an94, 0.0f, pso1, line);
        const bool on = SameBits(line[0], 0.0f) && SameBits(line[1], 0.127f) && SameBits(line[2], 0.95f);
        const float raised[3] = {0.0f, 0.092f, 0.99f}, glass[3] = {0.0f, 0.12f, 0.30f};
        float r[3] = {};
        aimrules::HeldShotLine(raised, 0.21f, glass, r);
        const bool up = std::fabs(r[1] - (0.12f - 0.69f * std::tan(0.21f * kDeg))) < 1e-6f && SameBits(r[2], 0.99f);
        int differ = 0;
        for (int k = 0; k < 2000000; ++k) {
            const Kind& kind = kinds[k % std::size(kinds)];
            const float g[3] = {(static_cast<float>(rng() % 2001) - 1000.0f) * 1e-5f, (static_cast<float>(rng() % 2001)) * 1e-4f,
                                (static_cast<float>(rng() % 2001) - 500.0f) * 1e-3f};
            float mine[3] = {}, old[3] = {};
            aimrules::HeldShotLine(kind.sights, kind.up, g, mine);
            OldSightLine(kind.sights, kind.up, g, old);
            differ += !(SameBits(mine[0], old[0]) && SameBits(mine[1], old[1]) && SameBits(mine[2], old[2]));
        }
        const float bad[3] = {0.0f, kNaN, 0.2f};
        float b[3] = {};
        aimrules::HeldShotLine(an94, 0.0f, bad, b);
        const bool nan = SameBits(b[1], an94[1]) && SameBits(b[2], an94[2]);
        Report("12", "HeldShotLine: a held gun's bullets' line", bare && on && up && differ == 0 && nan,
               "no glass the sights bit for bit %d; the AN-94 with the PSO-1 (%.3f %.3f %.3f) %d; raised 0.21 %d; %d of 2000000 differ from 40be42d's Sight line; a glass "
               "not finite %d",
               bare, line[0], line[1], line[2], on, up, differ, nan);
    }

    // ---- 13 ----
    {
        const float odd[] = {kNaN, kInf, -kInf, 1e30f, -1e30f, 0.0f, 0.5f};
        bool none = true;
        int cases = 0;
        for (float a : odd)
            for (float b : odd)
                for (float c : odd)
                    for (float d : odd)
                        for (bool any : {false, true}) {
                            ++cases;
                            // A pair's tests: no pass with any angle they use not finite (with any yaw the camera's yaw isn't used).
                            const bool used = std::isfinite(a) && std::isfinite(b) && std::isfinite(d) && (any || std::isfinite(c));
                            none = none && (used || (!aimrules::StrictPair(a, b, c, d, any) && !aimrules::NearPair(a, b, c, d, any)));
                            // The turn: always finite.
                            none = none && std::isfinite(aimrules::ProofTurn(a, b));
                            // The held line: finite for any glass.
                            const float sights[3] = {0.0f, 0.092f, 0.95f}, glass[3] = {a, b, c};
                            float out[3] = {};
                            aimrules::HeldShotLine(sights, 0.21f, glass, out);
                            none = none && std::isfinite(out[0]) && std::isfinite(out[1]) && std::isfinite(out[2]);
                            // Waiting by a pitch not finite: never for an older address or the route.
                            if (!std::isfinite(a))
                                none = none && !aimrules::KnownLikely(false, false, 0, std::fabs(a)) && !aimrules::RouteLikely(false, 0, std::fabs(a));
                            // A pull with what was written or read not a number: nothing learned.
                            if (std::isnan(a) || std::isnan(b))
                                none = none && aimrules::PullTeaches(a, b, any, true, 1.5f, -1.5f) == 0 && aimrules::PullTeaches(a, b, any, false, 1.5f, -1.5f) == 0;
                        }
        const bool goesOn = aimrules::PitchPassGoesOn(0, 0) && aimrules::PitchPassGoesOn(10000, 0) && aimrules::PitchPassGoesOn(599, 3) &&
                            !aimrules::PitchPassGoesOn(600, 1);
        // The time: a million of each rule.
        LARGE_INTEGER f, t0, t1;
        QueryPerformanceFrequency(&f);
        volatile float sink = 0.0f;
        double worst = 0.0;
        const char* worstName = "";
        auto time = [&](const char* name, auto&& body) {
            double best = 1e30;
            for (int run = 0; run < 3; ++run) {
                QueryPerformanceCounter(&t0);
                for (int k = 0; k < 1000000; ++k)
                    body(k);
                QueryPerformanceCounter(&t1);
                best = std::min(best, static_cast<double>(t1.QuadPart - t0.QuadPart) / f.QuadPart * 1e6 / 1e6);
            }
            if (best > worst)
                worst = best, worstName = name;
        };
        time("StrictPair", [&](int k) { sink = sink + aimrules::StrictPair(0.001f * k, -0.3f, 1.0f, -0.3f, k & 1); });
        time("NearPair", [&](int k) { sink = sink + aimrules::NearPair(0.001f * k, -0.3f, 1.0f, -0.3f, k & 1); });
        time("ProofTurn", [&](int k) { sink = sink + aimrules::ProofTurn(0.001f * k, 1.0f); });
        time("MouseStep", [&](int k) {
            aimrules::MouseAxis axis;
            axis.quiet = k & 7;
            sink = sink + static_cast<float>(aimrules::MouseStep(1e-5f * k, 230.0f, axis, true));
        });
        time("PullTeaches", [&](int k) { sink = sink + static_cast<float>(aimrules::PullTeaches(-1.0f, -1.0f + 1e-6f * k, k & 1, true, 1.5f, -1.5f)); });
        time("HeldShotLine", [&](int k) {
            const float s[3] = {0.0f, 0.092f, 0.95f}, g[3] = {0.0f, 0.127f + 1e-9f * k, 0.263f};
            float o[3];
            aimrules::HeldShotLine(s, 0.21f, g, o);
            sink = sink + o[1];
        });
        Report("13", "broken inputs, the pass's hold, the time", none && goesOn && worst < 1.0,
               "nothing not finite or passing out of NaN and infinities in %d cases %d; PitchPassGoesOn %d; the slowest rule %s %.4f us a call", cases, none, goesOn,
               worstName, worst);
    }

    std::printf("%d of %d checks failed\n", g_failed, g_counted);
    return g_failed;
}

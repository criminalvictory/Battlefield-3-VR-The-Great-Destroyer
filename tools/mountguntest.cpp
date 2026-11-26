// The mounted gun (src/mountgun.h, src/seatread.h: header only, as src/mountseat.cpp runs them; docs/research/mounted_gun.md), without the game. The
// guns' rows here are the dump's figures where it has them and made-up ones for the rest (the handles, the turret's answer): a test of the law, never
// the game's numbers. The turret is simulated as the design reads the dump: a rate input with momentum (each game tick the rate moves part of the way to
// what the input asks), the game's 30 Hz tick, its delay, the pad's dead zones and the seat's curve or the mouse's counts, a momentum threshold.
//   r  the rows: every shipped row unmeasured, Ready naming the first missing one and its source for each need, the dump's figures as the design sums
//      them, the Humvee's pad curve the truck's (the review's third finding), a test table all measured; the second review's: a yes or no measured as
//      no refused (r7), a row half filled refused (r8), the law's rows from the gun's rows alone (r9), the game's invert flipping the pitch (r10)
//   s  the seat's states: on foot, another seat (a passenger's, Rock's ride, an unknown vehicle's gunner seat: today's behaviour, the review's second
//      high finding), the truck's gunner seat with its rows unmeasured or its runtime not built (as before), the climb-in and its end at its row's
//      length, the seat gone; a guard's stop putting it back as before (s11), a read missed a moment held (s12)
//   q  the quiet list; the right stick never through the mouse in the seat (the review's first high finding), by the rule input.cpp runs, today's
//      routing shown sending it
//   h  the handle law: the gun's points, the camera's angles, the ring from the camera; a take never jumps; both hands, one hand; the truck's post and
//      the Humvee's ring each way; the pitch limits; spread and roll; the stick's orbit with the hands still (the review's fifth finding: offsets kept in
//      the world frame shown 26 degrees off); a seated torso's turn on both guns, both hands and each alone; the let-go measured after the offset; the
//      frame's changes never jump; a lost hand's place moving nothing (h15)
//   o  the stick's orbit: the eye's speed held (the review's sixth finding); mountpush off by default
//   m  the target's smoothing: a hand's wobble, a sweep
//   t  the steering against the simulated turret, both inputs: steps converge with no overshoot, saturate cleanly; rows off by twice; a sweep followed;
//      dead zones and thresholds settle without a hunt; nothing sent when it mustn't be; a wrong sign and no answer stopped; the strength learned only in
//      steady input; and the broken laws shown failing (the design's first gain, no guards, learning from the coast); the second review's: a read that
//      doesn't follow the turret (t17), the invert on with a small first move (t18), a stop met short of its row (t19), a deaf game at a small error
//      (t20), no false stop with the rows off (t21)
//   e  end to end: two grips swept, the gun follows and settles
//   f  the fire rule
//   g  the seat's reader on a made-up memory image: the shipped layout refused (unmeasured), a made-up one read: on foot, the gunner's seat, a passenger's,
//      not a vehicle, the entry or the data wrong, unreadable memory; its proofs each broken
//   z  the time
// Usage: mountguntest. The exit code is the number of failed checks.

#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <deque>
#include <map>
#include <string>
#include <vector>

#include "../src/mountgun.h"
#include "../src/seatread.h"

namespace {

using namespace mountgun;

int g_failed = 0, g_counted = 0;

void Report(const char* id, const char* what, bool pass, const char* format, ...) {
    char numbers[1024];
    va_list args;
    va_start(args, format);
    std::vsnprintf(numbers, sizeof(numbers), format, args);
    va_end(args);
    std::printf("%-4s %-5s %s | %s\n", id, pass ? "PASS" : "FAIL", what, numbers);
    ++g_counted;
    g_failed += !pass;
}

constexpr float kTick = 1.0f / 30.0f;

// ------------------------------------------------------------------ the test guns ------------------------------------------------------------------

void Set(Gun& g, int row, std::initializer_list<float> v, const char* text = nullptr) {
    Measure& m = g.m[row];
    m.measured = true;
    int i = 0;
    for (float x : v)
        if (i < kRowNumbers)
            m.value[i++] = x;
    m.text = text;
}

// Every row filled: the dump's figures where it has them, made-up ones for the rest (the right handle mirrors the truck's left; the Humvee's handles
// 0.55 m behind its pivot, 0.2 m apart; the turret's answer the simulation's own).
Gun TestGun(int which) {
    Gun g = kGuns[which];
    const bool truck = which == 0;
    Set(g, kRowEntry, {1.0f});
    Set(g, kRowVehicle, {}, truck ? "TechnicalTruck_Restricted" : "HumveeArmored_hmg");
    Set(g, kRowPivot, {g.dump[kRowPivot][0], g.dump[kRowPivot][1], g.dump[kRowPivot][2]});
    Set(g, kRowCamera, {g.dump[kRowCamera][0], g.dump[kRowCamera][1], g.dump[kRowCamera][2]});
    Set(g, kRowCameraRigid, {1.0f});
    Set(g, kRowPitchLimits, {g.dump[kRowPitchLimits][0], g.dump[kRowPitchLimits][1]});
    if (truck) {
        Set(g, kRowHandleLeft, {0.139f, 0.032f, -0.307f});
        Set(g, kRowHandleRight, {-0.139f, 0.032f, -0.307f});
    } else {
        Set(g, kRowHandleLeft, {0.10f, 0.0f, -0.55f});
        Set(g, kRowHandleRight, {-0.10f, 0.0f, -0.55f});
    }
    Set(g, kRowEye, {g.dump[kRowEye][0], g.dump[kRowEye][1], g.dump[kRowEye][2]});
    Set(g, kRowClimbIn, {5.0f});
    Set(g, kRowLag, {0.15f, 0.1f});
    Set(g, kRowPadAnswer, {0.1f, 1.0f, 1.0f, 0.02f, 0.02f});
    Set(g, kRowMouseAnswer, {0.067f, 1.0f, 1.0f, 0.02f, 0.03f});
    Set(g, kRowInvert, {0.0f, 3.0f}, "GstInput.TestFlipY");  // (a made-up key: which key it is is R5's)
    Set(g, kRowBarrelLine, {1.0f});
    Set(g, kRowFires, {1.0f});
    Set(g, kRowPadYaw, {0.405f, 0.0f, 0.475f, 9.0f, 0.825f, 19.8f, 0.9125f, 37.5f, 0.965f, 60.0f, 1.0f, 60.0f});
    Set(g, kRowPadPitch, {0.405f, 0.0f, 0.475f, 9.0f, 0.825f, 19.8f, 0.9125f, 37.5f, 0.965f, 60.0f, 1.0f, 60.0f});
    Set(g, kRowMouseYaw, {0.05f, 1200.0f});
    Set(g, kRowMousePitch, {0.04f, 1200.0f});
    Set(g, kRowArmsDraws, {13755.0f, 15051.0f});
    Set(g, kRowCrosshairDraw, {1.0f});
    Set(g, kRowTurnCentre, {0.0f});
    return g;
}

Geometry TestGeometry(int which) {
    Geometry g;
    const Gun gun = TestGun(which);
    GeometryOf(gun, g);
    return g;
}

float Deg(float radians) { return radians / kDeg; }

// ------------------------------------------------------------------ r: the rows ------------------------------------------------------------------

void Rows() {
    {
        int measured = 0, rows = 0;
        for (int i = 0; i < kGunCount; ++i)
            for (int r = 0; r < kRowCount; ++r, ++rows)
                measured += kGuns[i].m[r].measured ? 1 : 0;
        Report("r1", "every shipped row is unmeasured: nothing is steered, placed or matched on a guess (the runbook not run)", measured == 0,
               "%d of %d rows measured", measured, rows);
    }
    {
        bool ok = true;
        std::string said;
        const uint32_t needs[] = {kNeedMatch, kNeedView, kNeedSteer, kNeedPad, kNeedMouse, kNeedHands, kNeedHud};
        const char* names[] = {"match", "view", "steer", "pad", "mouse", "hands", "hud"};
        for (int i = 0; i < kGunCount; ++i)
            for (int n = 0; n < 7; ++n) {
                int missing = -2;
                const bool ready = Ready(kGuns[i], needs[n], &missing);
                ok = ok && !ready && missing >= 0 && missing < kRowCount && (kRowKinds[missing].need & needs[n]);
                if (i == 0) {
                    char line[160];
                    std::snprintf(line, sizeof(line), "%s%s: %s (%s)", said.empty() ? "" : "; ", names[n], missing >= 0 ? kRowKinds[missing].name : "?",
                                  missing >= 0 ? kRowKinds[missing].from : "?");
                    said += line;
                }
            }
        Report("r2", "each need refuses on each gun, naming its first unmeasured row and where it comes from", ok, "the truck: %s", said.c_str());
    }
    {
        // The design's sums (2.2): the truck's camera 0.61 m behind and 0.20 above its pivot, 0.63 m behind the ring's axis; the Humvee's 0.98 and 0.27,
        // 0.18; the truck's left hand's IK point 0.31 m behind its pivot, 0.14 to the side; the eye's figure the camera's at zero pitch.
        const Gun& t = kGuns[0];
        const Gun& h = kGuns[1];
        const auto near = [](float a, float b) { return std::fabs(a - b) < 0.0015f; };
        const bool ok = near(-t.dump[kRowCamera][2], 0.608f) && near(t.dump[kRowCamera][1], 0.202f) &&
                        near(t.dump[kRowPivot][2] + t.dump[kRowCamera][2], -0.632f) && near(-h.dump[kRowCamera][2], 0.978f) &&
                        near(h.dump[kRowCamera][1], 0.268f) && near(h.dump[kRowPivot][2] + h.dump[kRowCamera][2], -0.182f) &&
                        near(t.dump[kRowHandleLeft][2], -0.307f) && near(t.dump[kRowHandleLeft][0], 0.139f) &&
                        near(t.dump[kRowEye][1], t.dump[kRowPivot][1] + t.dump[kRowCamera][1]) &&
                        near(h.dump[kRowEye][2], h.dump[kRowPivot][2] + h.dump[kRowCamera][2]) && t.dump[kRowPitchLimits][1] == 20.0f &&
                        h.dump[kRowPitchLimits][1] == 35.0f;
        Report("r3", "the dump's figures beside the rows are the design's sums (2.2): only to check a measurement against", ok,
               "truck camera %.3f back %.3f up, eye %.3f behind the axis; Humvee camera %.3f back %.3f up, eye %.3f behind", -t.dump[kRowCamera][2],
               t.dump[kRowCamera][1], -(t.dump[kRowPivot][2] + t.dump[kRowCamera][2]), -h.dump[kRowCamera][2], h.dump[kRowCamera][1],
               -(h.dump[kRowPivot][2] + h.dump[kRowCamera][2]));
    }
    {
        // The review's third finding: the Humvee's active gunner entry carries the truck's pad curves (the "none" was an excluded duplicate's).
        bool same = true;
        for (int r : {kRowPadYaw, kRowPadPitch})
            for (int k = 0; k < 2; ++k)
                same = same && kGuns[0].dump[r][k] == kGuns[1].dump[r][k];
        Report("r4", "the Humvee's pad curve is the truck's (its dead zone to 0.15), not a straight line", same, "truck %.2f, Humvee %.2f",
               kGuns[0].dump[kRowPadYaw][0], kGuns[1].dump[kRowPadYaw][0]);
    }
    {
        bool ok = true;
        for (int i = 0; i < kGunCount; ++i) {
            const Gun g = TestGun(i);
            int missing = -2;
            ok = ok && Ready(g, kNeedMatch | kNeedView | kNeedSteer | kNeedPad | kNeedMouse | kNeedHands | kNeedHud, &missing) && missing == -1;
            Geometry geo;
            ok = ok && GeometryOf(g, geo) && std::fabs(geo.pitchMax - g.dump[kRowPitchLimits][1] * kDeg) < 1e-6f;
        }
        Report("r5", "a table with every row measured is ready for every need, its geometry read from it", ok, "both test guns");
    }
    {
        const char* from = nullptr;
        const char* first = seatread::FirstUnmeasured(seatread::kLayout, &from);
        Report("r6", "the seat reader's shipped layout: unmeasured, its first row named with its source", first != nullptr && from != nullptr,
               "%s (%s)", first ? first : "none", from ? from : "");
    }
    {
        // The second review's fourth finding: a yes or no measured as no (the camera animated, the rounds along the view, the left button not firing)
        // keeps the seat off, named as no; the camera's rigidity is the steering's too (it reads the game's gun off the camera).
        bool ok = true;
        std::string said;
        for (int row : {kRowCameraRigid, kRowBarrelLine, kRowFires}) {
            Gun g = TestGun(0);
            g.m[row].value[0] = 0.0f;
            int missing = -2;
            RowFault fault = RowFault::kNone;
            const bool ready = Ready(g, kNeedMatch | kNeedView | kNeedSteer | kNeedMouse, &missing, &fault);
            const bool steer = Ready(g, kNeedSteer);
            Seat s;
            SeatRules built;
            built.built = true;
            CameraNow still;
            still.fresh = true;
            const Gun guns[2] = {g, TestGun(1)};
            SeatStep(s, SeatRead{true, true, 1, "TechnicalTruck_Restricted"}, still, 0.0, guns, 2, built);
            ok = ok && !ready && !steer && missing == row && fault == RowFault::kNo && s.state == State::kOtherSeat && s.missing == row && s.fault == RowFault::kNo;
            said += std::string(said.empty() ? "" : "; ") + kRowKinds[row].name + ": " + RowFaultName(fault);
        }
        Report("r7", "a yes or no row measured as no keeps the seat as before, named (the camera animated, the rounds along the view, the left button)", ok, "%s",
               said.c_str());
    }
    {
        // The second review's eleventh: a row half filled is refused, never read as zeros: the lag with its yaw alone, a pad table of one pair, one with a
        // gap, one not rising, the invert row without its key, a measure's numbers not a number until filled.
        const auto fault = [](void (*spoil)(Gun&), int row) {
            Gun g = TestGun(0);
            spoil(g);
            return RowState(g, row);
        };
        const RowFault lag = fault([](Gun& g) { g.m[kRowLag].value[1] = kNaN; }, kRowLag);
        const RowFault onePair = fault([](Gun& g) { for (int i = 2; i < kRowNumbers; ++i) g.m[kRowPadYaw].value[i] = kNaN; }, kRowPadYaw);
        const RowFault gap = fault([](Gun& g) { g.m[kRowPadYaw].value[4] = g.m[kRowPadYaw].value[5] = kNaN; }, kRowPadYaw);
        const RowFault falling = fault([](Gun& g) { g.m[kRowPadYaw].value[5] = 5.0f; }, kRowPadYaw);
        const RowFault noKey = fault([](Gun& g) { g.m[kRowInvert].text = nullptr; }, kRowInvert);
        const RowFault answer = fault([](Gun& g) { g.m[kRowMouseAnswer].value[4] = kNaN; }, kRowMouseAnswer);
        const Measure fresh;
        bool nan = true;
        for (float v : fresh.value)
            nan = nan && std::isnan(v);
        Gun lagHalf = TestGun(0);
        lagHalf.m[kRowLag].value[1] = kNaN;
        AxisRows rows;
        const bool refused = !AxisRowsOf(lagHalf, 1, kNeedMouse, rows) && !RowsReady(rows);
        Report("r8", "a row half filled is refused (the lag's yaw alone, a table of one pair, with a gap, falling; the invert's key missing; an answer short)",
               lag == RowFault::kHalf && onePair == RowFault::kHalf && gap == RowFault::kHalf && falling == RowFault::kNotRising && noKey == RowFault::kHalf &&
                   answer == RowFault::kHalf && nan && refused,
               "%s; %s; %s; %s; %s; %s; a fresh row's numbers %s; the pitch's rows from a half lag %s", RowFaultName(lag), RowFaultName(onePair),
               RowFaultName(gap), RowFaultName(falling), RowFaultName(noKey), RowFaultName(answer), nan ? "not numbers" : "NUMBERS",
               refused ? "refused" : "TAKEN");
    }
    {
        // The second review's twelfth: the law's numbers from the rows alone (AxisRowsOf), each way and axis; an AxisRows not filled refused by the law
        // itself; the climb-in's length a row.
        const Gun g = TestGun(0);
        AxisRows py, pp, my, mp;
        PadTable table;
        float perCount = 0.0f;
        const bool got = AxisRowsOf(g, 0, kNeedPad, py, kNaN, &table) && AxisRowsOf(g, 1, kNeedPad, pp) && AxisRowsOf(g, 0, kNeedMouse, my, kNaN, nullptr, &perCount) &&
                         AxisRowsOf(g, 1, kNeedMouse, mp);
        const bool pad = py.most == 60.0f && py.lag == 0.15f && py.delay == 0.1f && py.least == 0.02f && py.sign == 1.0f && std::isinf(py.lo) && std::isinf(py.hi) &&
                         pp.lag == 0.1f && pp.lo == -20.0f && pp.hi == 20.0f && table.n == 6;
        const bool mouse = std::fabs(my.most - 60.0f) < 1e-3f && my.delay == 0.067f && my.least == 0.02f && std::fabs(mp.most - 48.0f) < 1e-3f && mp.least == 0.03f &&
                           perCount == 0.05f;
        AxisState s;
        const AxisOut unfilled = SteerAxis(s, AxisRows{}, 10.0f, 0.0f, 0.0, true, true);
        AxisRows wrong;
        const bool noWay = !AxisRowsOf(g, 0, kNeedHands, wrong) && !AxisRowsOf(g, 2, kNeedMouse, wrong);
        Report("r9", "the law's rows from the gun's rows alone, each way and axis (the pitch's limits its row's); an AxisRows not filled refused by the law",
               got && pad && mouse && unfilled.refused && !unfilled.sent && noWay,
               "pad yaw most %.0f lag %.2f delay %.3f least %.2f; pitch limits %.0f %.0f; mouse yaw most %.0f, pitch %.0f; unfilled %s", py.most, py.lag, py.delay,
               py.least, pp.lo, pp.hi, my.most, mp.most, unfilled.refused ? "refused" : "TAKEN");
    }
    {
        // The second review's second finding, its first fix: the game's vertical invert. The pitch's sign as R5 measured it with the key at its value then,
        // flipped for a player whose key differs (each way by the row's flips), the yaw never; a key not read, or none: as measured.
        Gun g = TestGun(0);
        const float same = SignOf(g, kNeedMouse, 1, 0.0f), other = SignOf(g, kNeedMouse, 1, 1.0f), padOther = SignOf(g, kNeedPad, 1, 1.0f),
                    yaw = SignOf(g, kNeedMouse, 0, 1.0f), unread = SignOf(g, kNeedMouse, 1, kNaN);
        g.m[kRowInvert].value[1] = 1.0f;  // (the key flips the pad's alone)
        const float mouseOnly = SignOf(g, kNeedMouse, 1, 1.0f), padOnly = SignOf(g, kNeedPad, 1, 1.0f);
        g.m[kRowInvert].text = "none";
        const float none = SignOf(g, kNeedPad, 1, 1.0f);
        AxisRows flipped;
        AxisRowsOf(TestGun(0), 1, kNeedMouse, flipped, 1.0f);
        Report("r10", "the game's invert: the pitch's sign flipped for a player whose key differs from R5's (each way by the row), the yaw never, unread or none as measured",
               same == 1.0f && other == -1.0f && padOther == -1.0f && yaw == 1.0f && unread == 1.0f && mouseOnly == 1.0f && padOnly == -1.0f && none == 1.0f &&
                   flipped.sign == -1.0f,
               "same %+.0f, other %+.0f, the pad's %+.0f, the yaw %+.0f, unread %+.0f; the pad's key: the mouse %+.0f, the pad %+.0f; none %+.0f", same, other,
               padOther, yaw, unread, mouseOnly, padOnly, none);
    }
}

// ------------------------------------------------------------------ s: the seat's states ------------------------------------------------------------------

SeatRead Read(bool vehicle, int entry, const char* name) {
    SeatRead r;
    r.known = true;
    r.vehicle = vehicle;
    r.entry = entry;
    r.name = name;
    return r;
}

void States() {
    const Gun tested[2] = {TestGun(0), TestGun(1)};
    SeatRules built;
    built.built = true;
    CameraNow still;
    still.fresh = true;
    // s1 on foot
    {
        Seat s;
        SeatStep(s, Read(false, -1, nullptr), still, 0.0, tested, 2, built);
        bool quiet = false;
        for (int w = 0; w < kWhatCount; ++w)
            quiet = quiet || Quiet(s.state, static_cast<What>(w));
        Report("s1", "on foot: on foot, nothing quiet", s.state == State::kOnFoot && !quiet, "%s", StateName(s.state));
    }
    // s2-s4 other seats: as before
    {
        const struct {
            const char* id;
            const char* what;
            SeatRead read;
        } cases[] = {
            {"s2", "the truck's passenger seat (entry 0): another seat, as before (the review's second high finding)", Read(true, 0, "TechnicalTruck_Restricted")},
            {"s3", "Rock and a Hard Place's ride (an unknown vehicle's seat): as before", Read(true, 1, "Growler_ITV")},
            {"s4", "The Great Destroyer's police car (an unknown vehicle, entry 2: the M9 needs free hands and the look): as before", Read(true, 2, "PoliceCar")},
        };
        for (const auto& c : cases) {
            Seat s;
            bool quiet = false, ok = true;
            for (int f = 0; f < 600; ++f) {  // (ten seconds at 60: never anything else)
                SeatStep(s, c.read, still, f / 60.0, tested, 2, built);
                ok = ok && s.state == State::kOtherSeat && s.gun < 0;
                for (int w = 0; w < kWhatCount; ++w)
                    quiet = quiet || Quiet(s.state, static_cast<What>(w));
            }
            Report(c.id, c.what, ok && !quiet, "%s, quiet %s", StateName(s.state), quiet ? "something" : "nothing");
        }
    }
    {
        Seat s;
        bool ok = true;
        for (int f = 0; f < 600; ++f) {
            SeatStep(s, Read(true, 1, "TechnicalTruck_Restricted"), still, f / 60.0, kGuns, kGunCount, built);
            ok = ok && s.state == State::kOtherSeat;
        }
        Report("s5", "the truck's gunner seat with the shipped rows (unmeasured: not even told): another seat, as before", ok && s.seen < 0, "%s",
               StateName(s.state));
    }
    {
        Seat s;
        SeatRules notBuilt = built;
        notBuilt.built = false;
        bool ok = true;
        for (int f = 0; f < 600; ++f) {
            SeatStep(s, Read(true, 1, "TechnicalTruck_Restricted"), still, f / 60.0, tested, 2, notBuilt);
            ok = ok && s.state == State::kOtherSeat;
        }
        Report("s6", "the truck's gunner seat told, its rows all measured, step 1's runtime not built (today): as before", ok && s.seen == 0, "%s, told %d",
               StateName(s.state), s.seen);
    }
    {
        // One row unmeasured keeps the told gun out, named.
        Gun partial[2] = {TestGun(0), TestGun(1)};
        partial[0].m[kRowLag].measured = false;
        Seat s;
        SeatStep(s, Read(true, 1, "technicaltruck_restricted"), still, 0.0, partial, 2, built);
        Report("s7", "a told gun with one row unmeasured (the turret's lag): as before, that row named", s.state == State::kOtherSeat && s.seen == 0 &&
                                                                                                      s.missing == kRowLag,
               "%s, missing: %s", StateName(s.state), s.missing >= 0 ? kRowKinds[s.missing].name : "none");
    }
    {
        // The climb-in: the camera moving to 0.3 s then still: ends at the least, 1 s. Moving to 2 s: at 2.5 s. Never still: at its row's length and a
        // second (the test gun's 5 s: at 6; a gun whose climb-in is 2.5 s: at 3.5).
        Gun shortIn[2] = {TestGun(0), TestGun(1)};
        shortIn[0].m[kRowClimbIn].value[0] = 2.5f;
        const auto run = [&](double movingUntil, double& endedAt, int& ends, const Gun* guns = nullptr) {
            Seat s;
            endedAt = -1.0, ends = 0;
            for (int f = 0; f <= 600; ++f) {
                const double t = f / 60.0;
                CameraNow c;
                c.fresh = true;
                c.yaw = t < movingUntil ? static_cast<float>(t) * 0.5f : static_cast<float>(movingUntil) * 0.5f;
                c.place = {static_cast<float>(t < movingUntil ? t : movingUntil) * 0.1f, 2.0f, 0.0f};
                SeatStep(s, Read(true, 1, "TechnicalTruck_Restricted"), c, t, guns ? guns : tested, 2, built);
                if (f == 0 && s.state != State::kClimbIn)
                    return false;
                if (s.climbEnded) {
                    ++ends;
                    endedAt = t;
                }
            }
            return s.state == State::kGun;
        };
        double a = 0, b = 0, c = 0, d = 0;
        int na = 0, nb = 0, nc = 0, nd = 0;
        const bool ra = run(0.3, a, na), rb = run(2.0, b, nb), rc = run(100.0, c, nc), rd = run(100.0, d, nd, shortIn);
        Report("s8", "the climb-in: the gun's seat begins with it; ends after 0.5 s still, no sooner than 1 s, at its row's length and a second at most, once",
               ra && rb && rc && rd && std::fabs(a - 1.0) < 0.02 && std::fabs(b - 2.5) < 0.04 && std::fabs(c - 6.0) < 0.02 && std::fabs(d - 3.5) < 0.02 && na == 1 &&
                   nb == 1 && nc == 1 && nd == 1,
               "still from 0.3 s: %.2f s; from 2 s: %.2f s; never: %.2f s (its row 5 s), %.2f s (2.5 s)", a, b, c, d);
    }
    {
        Seat s;
        for (int f = 0; f < 600; ++f)
            SeatStep(s, Read(true, 1, "TechnicalTruck_Restricted"), still, f / 60.0, tested, 2, built);
        const State on = s.state;
        SeatStep(s, Read(false, -1, nullptr), still, 10.01, tested, 2, built);
        const State off = s.state;
        SeatStep(s, Read(true, 1, "TechnicalTruck_Restricted"), still, 10.02, tested, 2, built);
        const State again = s.state;
        Report("s9", "the seat gone (a pull-out, a death: the read says no vehicle): on foot at once; back on: the climb-in again",
               on == State::kGun && off == State::kOnFoot && again == State::kClimbIn, "%s, %s, %s", StateName(on), StateName(off), StateName(again));
    }
    {
        // The second review's ninth finding: a read missed for a moment (the reader's memory unreadable, a hitch) holds the seat as it was; missed past a
        // quarter second, on foot. Before, one unread frame gave a frame of today's rules and a new climb-in of 1 to 6 s.
        Seat s;
        for (int f = 0; f < 600; ++f)
            SeatStep(s, Read(true, 1, "TechnicalTruck_Restricted"), still, f / 60.0, tested, 2, built);
        const SeatRead unknown;
        bool held = true;
        for (int f = 1; f <= 12; ++f) {  // (0.2 s at 60)
            SeatStep(s, unknown, still, 10.0 + f / 60.0, tested, 2, built);
            held = held && s.state == State::kGun && !s.changed;
        }
        SeatStep(s, Read(true, 1, "TechnicalTruck_Restricted"), still, 10.21, tested, 2, built);
        const State back = s.state;
        for (int f = 1; f <= 18; ++f)  // (0.3 s)
            SeatStep(s, unknown, still, 10.21 + f / 60.0, tested, 2, built);
        Report("s12", "a read missed 0.2 s holds the gun's seat as it was (no climb-in again); missed 0.3 s, on foot", held && back == State::kGun &&
                                                                                                                   s.state == State::kOnFoot,
               "held %s, after it %s, then %s", held ? "yes" : "NO", StateName(back), StateName(s.state));
    }
    {
        // The second review's third finding: a guard's stop (here a camera that doesn't follow the turret: no answer) ends the steering of both axes
        // and puts the seat back to today's behaviour, said, till the seat is left; back on, the climb-in again.
        Seat s;
        for (int f = 0; f < 600; ++f)
            SeatStep(s, Read(true, 1, "TechnicalTruck_Restricted"), still, f / 60.0, tested, 2, built);
        const State on = s.state;
        SeatAxes axes;
        const bool rows = AxisRowsOf(tested[0], 0, kNeedMouse, axes.rows[0]) && AxisRowsOf(tested[0], 1, kNeedMouse, axes.rows[1]);
        CameraNow frozen;  // (fresh, but never moving: a copy of some other camera)
        frozen.fresh = true;
        double stoppedAt = -1.0;
        float sentAfter = 0.0f;
        bool otherAfter = true;
        for (int f = 0; f < 72 * 4; ++f) {
            const double now = 10.0 + f / 72.0;
            SeatStep(s, Read(true, 1, "TechnicalTruck_Restricted"), frozen, now, tested, 2, built);
            const SeatSteerOut o = SeatSteer(s, axes, 6.0f * kDeg, 2.0f * kDeg, frozen, now, true);
            if (stoppedAt >= 0.0) {
                sentAfter += std::fabs(o.axis[0].rate) + std::fabs(o.axis[1].rate);
                otherAfter = otherAfter && s.state == State::kOtherSeat;
            }
            if (o.stopped && stoppedAt < 0.0)
                stoppedAt = now - 10.0;
        }
        SeatStep(s, Read(false, -1, nullptr), frozen, 15.0, tested, 2, built);
        SeatStep(s, Read(true, 1, "TechnicalTruck_Restricted"), frozen, 15.1, tested, 2, built);
        Report("s11", "a guard's stop ends both axes' steering and puts the seat back as before till it's left (said); back on: the climb-in again",
               on == State::kGun && rows && stoppedAt > 0.0 && stoppedAt < 2.0 && sentAfter == 0.0f && otherAfter && s.state == State::kClimbIn &&
                   s.stopped == SteerStop::kNone,
               "stopped at %.2f s (%s), nothing asked after (%.1f), another seat after: %s; back on: %s", stoppedAt, SteerStopName(axes.axis[0].stop != SteerStop::kNone ?
                                                                                                                        axes.axis[0].stop : axes.axis[1].stop),
               sentAfter, otherAfter ? "yes" : "NO", StateName(s.state));
    }
    {
        // The design's first table (the review's second high finding): any seat the reader can't place in a row as a scripted one. Shown quieting Rock's ride.
        const auto broken = [&](const SeatRead& r) { return r.known && r.vehicle ? (MatchGun(tested, 2, r) >= 0 ? State::kGun : State::kClimbIn) : State::kOnFoot; };
        const State ride = broken(Read(true, 1, "Growler_ITV"));
        Report("s10", "the design's first table, shown failing: it would hold Rock's ride's look writes and free hands quiet",
               Quiet(ride, kLookWrites) && Quiet(ride, kFreeHands), "the ride as %s: the look's writes %s", StateName(ride),
               Quiet(ride, kLookWrites) ? "quiet (broken)" : "as before");
    }
}

// ------------------------------------------------------------------ q: the quiet list ------------------------------------------------------------------

void QuietList() {
    {
        bool ok = true;
        std::string quiet;
        for (int w = 0; w < kWhatCount; ++w) {
            const What what = static_cast<What>(w);
            const bool always = what == kFire || what == kUse || what == kRecenter;
            ok = ok && !Quiet(State::kOnFoot, what) && !Quiet(State::kOtherSeat, what);
            ok = ok && Quiet(State::kClimbIn, what) == !always && Quiet(State::kGun, what) == !always;
            if (!always) {
                if (!quiet.empty())
                    quiet += ", ";
                quiet += WhatName(w);
            }
        }
        Report("q1", "in a gun's seat everything but the fire, the use and the recenter is quiet; on foot and in other seats nothing", ok, "quiet: %s",
               quiet.c_str());
    }
    {
        bool ok = true;
        for (State s : {State::kOnFoot, State::kOtherSeat})
            ok = ok && StickDirect(s, true) && !StickDirect(s, false);
        for (State s : {State::kClimbIn, State::kGun})
            ok = ok && StickDirect(s, true) && StickDirect(s, false);
        Report("q2", "the stick's turn: on foot and in other seats as before; in the gun's seat always taken directly", ok, "");
    }
    {
        // The review's case, through the rule input.cpp's UpdateTurn runs (RouteStick, MouseTurnRate: the second review's thirteenth finding, the test had
        // copied the routing): the stick held 2 s at 72 Hz, the look's writes quiet (today's rule says "through the mouse"), turn_speed 15 (the default:
        // 1080 counts a second at full deflection). In the climb-in and on the gun, no mouse X; on foot and in another seat, today's.
        const auto counts = [](State state, bool onFootDirect) {
            float sent = 0.0f, taken = 0.0f;
            for (int f = 0; f < 144; ++f) {
                const StickRoute route = RouteStick(state, onFootDirect, 1.0f);
                sent += MouseTurnRate(route.mouse, 15.0f) / 72.0f;
                taken += StickAmount(route.turn);
            }
            return std::make_pair(sent, taken);
        };
        const auto gun = counts(State::kGun, false), climb = counts(State::kClimbIn, false), foot = counts(State::kOnFoot, false),
                   other = counts(State::kOtherSeat, false), direct = counts(State::kOnFoot, true);
        const bool half = std::fabs(StickAmount(0.6f) - 0.5f) < 1e-5f && StickAmount(-0.2f) == 0.0f && std::fabs(MouseTurnRate(-0.6f, 15.0f) + 540.0f) < 0.01f &&
                          MouseTurnRate(0.1f, 15.0f) == 0.0f;
        Report("q3", "the stick held 2 s in the gun's seat (climb-in and gun): no mouse X sent (the ring's turn), the turn taken for the body; elsewhere as today",
               gun.first == 0.0f && climb.first == 0.0f && gun.second > 0.0f && std::fabs(foot.first - 2160.0f) < 0.5f && other.first == foot.first &&
                   direct.first == 0.0f && direct.second > 0.0f && half,
               "%.0f counts sent in the seat; today's routing with the look quiet sends %.0f (on foot, another seat %.0f), turning the turret", gun.first,
               foot.first, other.first);
    }
}

// ------------------------------------------------------------------ h: the handle law ------------------------------------------------------------------

struct Scene {
    Geometry g;
    V3 ring{10.0f, 2.0f, -30.0f};
    float body = 0.3f;  // the body's yaw (radians)
};

V3 HandsAt(const Scene& sc, int handle, float yaw, float pitch, V3 offset) { return HandleAt(sc.g, sc.ring, handle, yaw, pitch) + offset; }

// Two hands taken at the game's (yaw0, pitch0) with offsets; then hands placed for the gun at (yaw1, pitch1) (each hand its handle there plus its offset,
// turned with the frame); the solve's answer.
struct Answer {
    float yaw, pitch;
};
Answer TakeAndMove(const Scene& sc, float yaw0, float pitch0, V3 off0, V3 off1, float yaw1, float pitch1) {
    Holds h;
    HandsNow n;
    n.at[0] = HandsAt(sc, 0, yaw0, pitch0, off0);
    n.at[1] = HandsAt(sc, 1, yaw0, pitch0, off1);
    n.tracked[0] = n.tracked[1] = n.squeezed[0] = n.squeezed[1] = true;
    HoldStep(h, n, sc.g, sc.ring, yaw0, pitch0, sc.body);
    // The hands moved as the handles would for (yaw1, pitch1), the offsets turned as a hand pair turned with the gun would turn them.
    const float turn = yaw1 - yaw0;
    n.at[0] = HandsAt(sc, 0, yaw1, pitch1, Turn(turn, off0));
    n.at[1] = HandsAt(sc, 1, yaw1, pitch1, Turn(turn, off1));
    HoldStep(h, n, sc.g, sc.ring, yaw0, pitch0, sc.body);
    Answer a{yaw0, pitch0};
    Solve(sc.g, sc.ring, h, n.at, a.yaw, a.pitch);
    return a;
}

void HandleLaw() {
    {
        float worst = 0.0f;
        for (float yaw = -3.0f; yaw <= 3.0f; yaw += 0.37f)
            for (float pitch = -1.2f; pitch <= 1.2f; pitch += 0.29f) {
                const V3 f = Forward(yaw, pitch);
                worst = std::fmax(worst, std::fabs(Wrap(YawOf(f) - yaw)) + std::fabs(PitchOf(f) - pitch));
                // A camera's rows (right, up, back) at that yaw and pitch: back = -forward.
                const V3 up = Turn(yaw, Tilt(pitch, {0.0f, 1.0f, 0.0f})), right = Turn(yaw, {-1.0f, 0.0f, 0.0f});
                const float rows[9] = {right.x, right.y, right.z, up.x, up.y, up.z, -f.x, -f.y, -f.z};
                float y = 0.0f, p = 0.0f;
                AnglesOfRows(rows, y, p);
                worst = std::fmax(worst, std::fabs(Wrap(y - yaw)) + std::fabs(p - pitch));
                // and the game's look yaw's own sense: turning right grows it (the right row at yaw 0 is -x: a turn right moves forward toward -x)
                const V3 g = Forward(yaw + 0.01f, pitch);
                worst = std::fmax(worst, Dot(g - f, right) < 0.0f ? 1.0f : 0.0f);
            }
        Report("h1", "the angles: a forward and a camera's rows give back their yaw and pitch, the yaw growing turning right as the game's look", worst < 1e-4f,
               "worst %.2e rad", worst);
    }
    {
        float worst = 0.0f;
        for (int which = 0; which < 2; ++which) {
            Scene sc;
            sc.g = TestGeometry(which);
            for (float yaw = -3.0f; yaw <= 3.0f; yaw += 0.41f)
                for (float pitch = sc.g.pitchMin; pitch <= sc.g.pitchMax; pitch += 0.07f) {
                    const V3 cam = CameraAt(sc.g, sc.ring, yaw, pitch);
                    worst = std::fmax(worst, Length(RingFromCamera(sc.g, cam, yaw, pitch) - sc.ring));
                }
        }
        Report("h2", "the ring's axis point from the game's camera (rigid on the gun), both guns, every yaw and pitch", worst < 1e-5f, "worst %.2e m", worst);
    }
    {
        // A take never jumps: hands 5 to 25 cm off the handles every way (arms not the gunner's), the solve right after gives the game's gun.
        float worst = 0.0f;
        for (int which = 0; which < 2; ++which) {
            Scene sc;
            sc.g = TestGeometry(which);
            for (int k = 0; k < 40; ++k) {
                const float yaw0 = -2.5f + 0.13f * k, pitch0 = sc.g.pitchMin + (sc.g.pitchMax - sc.g.pitchMin) * (k % 7) / 6.0f;
                const V3 off0{0.05f * std::sin(k * 1.3f), -0.1f + 0.04f * std::cos(k * 0.7f), -0.15f + 0.02f * (k % 5)};
                const V3 off1{-0.06f * std::cos(k * 0.9f), -0.08f, -0.2f + 0.03f * (k % 3)};
                const Answer a = TakeAndMove(sc, yaw0, pitch0, off0, off1, yaw0, pitch0);
                worst = std::fmax(worst, std::fabs(Wrap(a.yaw - yaw0)) + std::fabs(a.pitch - pitch0));
            }
        }
        Report("h3", "a take never moves the gun: two hands closing 5 to 25 cm off the handles, the gun stays where the game has it", Deg(worst) < 0.01f,
               "worst %.4f degrees", Deg(worst));
    }
    {
        // Both hands moved as the handles would be for another yaw and pitch: the solve finds it. Both shapes, a full grid.
        float worst[2] = {};
        for (int which = 0; which < 2; ++which) {
            Scene sc;
            sc.g = TestGeometry(which);
            for (float dy = -2.1f; dy <= 2.1f; dy += 0.26f)
                for (float pitch = sc.g.pitchMin; pitch <= sc.g.pitchMax + 1e-4f; pitch += (sc.g.pitchMax - sc.g.pitchMin) / 6.0f) {
                    const float yaw0 = 0.4f, pitch0 = 0.05f;
                    const Answer a = TakeAndMove(sc, yaw0, pitch0, {0.03f, -0.12f, -0.18f}, {-0.04f, -0.1f, -0.2f}, yaw0 + dy, pitch);
                    worst[which] = std::fmax(worst[which], std::fabs(Wrap(a.yaw - (yaw0 + dy))) + std::fabs(a.pitch - pitch));
                }
        }
        Report("h4", "two hands put the handles anywhere the gun can take them (120 degrees each way, every pitch): the gun is put there", Deg(worst[0]) < 0.05f &&
                                                                                                                                       Deg(worst[1]) < 0.05f,
               "worst: the truck %.4f, the Humvee %.4f degrees", Deg(worst[0]), Deg(worst[1]));
    }
    // The owner's call 1, both shapes, and up and down.
    {
        Scene sc;
        sc.g = TestGeometry(0);
        const float yaw0 = sc.body, pitch0 = 0.0f;
        Holds h;
        HandsNow n;
        n.at[0] = HandsAt(sc, 0, yaw0, pitch0, {0.0f, -0.1f, -0.15f});
        n.at[1] = HandsAt(sc, 1, yaw0, pitch0, {0.0f, -0.1f, -0.15f});
        n.tracked[0] = n.tracked[1] = n.squeezed[0] = n.squeezed[1] = true;
        HoldStep(h, n, sc.g, sc.ring, yaw0, pitch0, sc.body);
        const V3 left = Turn(sc.body, {0.1f, 0.0f, 0.0f});  // (the body's left: +x at its yaw 0)
        HandsNow moved = n;
        moved.at[0] = n.at[0] + left, moved.at[1] = n.at[1] + left;
        HoldStep(h, moved, sc.g, sc.ring, yaw0, pitch0, sc.body);
        float yaw = yaw0, pitch = pitch0;
        Solve(sc.g, sc.ring, h, moved.at, yaw, pitch);
        const float turned = Deg(Wrap(yaw - yaw0));
        HandsNow raised = n;
        raised.at[0].y += 0.05f, raised.at[1].y += 0.05f;
        HoldStep(h, raised, sc.g, sc.ring, yaw0, pitch0, sc.body);
        float yaw2 = yaw0, pitch2 = pitch0;
        Solve(sc.g, sc.ring, h, raised.at, yaw2, pitch2);
        Report("h5", "the truck's post (the owner's call 1): both handles pushed 10 cm left swing the barrel right; raised 5 cm, the muzzle goes down",
               turned > 10.0f && turned < 30.0f && Deg(pitch2) < -5.0f, "the barrel %.1f degrees right; the muzzle %.1f degrees", turned, Deg(pitch2));
    }
    {
        // The Humvee: both arms swung left round the gunner (about his eye) by 20 degrees: the gun turns left.
        Scene sc;
        sc.g = TestGeometry(1);
        const float yaw0 = sc.body, pitch0 = 0.0f;
        const V3 eye = sc.ring + Turn(sc.body, sc.g.eye);
        Holds h;
        HandsNow n;
        n.at[0] = HandsAt(sc, 0, yaw0, pitch0, {0.0f, -0.05f, -0.1f});
        n.at[1] = HandsAt(sc, 1, yaw0, pitch0, {0.0f, -0.05f, -0.1f});
        n.tracked[0] = n.tracked[1] = n.squeezed[0] = n.squeezed[1] = true;
        HoldStep(h, n, sc.g, sc.ring, yaw0, pitch0, sc.body);
        HandsNow swung = n;
        for (int i = 0; i < 2; ++i)
            swung.at[i] = eye + Turn(-20.0f * kDeg, n.at[i] - eye);
        HoldStep(h, swung, sc.g, sc.ring, yaw0, pitch0, sc.body);
        float yaw = yaw0, pitch = pitch0;
        Solve(sc.g, sc.ring, h, swung.at, yaw, pitch);
        const float turned = Deg(Wrap(yaw - yaw0));
        Report("h6", "the Humvee's ring: both arms swung 20 degrees left round the gunner turn the gun left", turned < -15.0f, "%.1f degrees", turned);
    }
    {
        // One hand: its handle follows it. The other let go: the gun still goes by the one.
        float worst = 0.0f;
        for (int which = 0; which < 2; ++which) {
            Scene sc;
            sc.g = TestGeometry(which);
            for (int k = 0; k < 24; ++k) {
                const float yaw0 = 0.2f, pitch0 = 0.0f, yaw1 = yaw0 + 0.06f * (k - 12), pitch1 = 0.02f * ((k % 9) - 4);
                Holds h;
                HandsNow n;
                n.at[1] = HandsAt(sc, 1, yaw0, pitch0, {0.0f, -0.08f, -0.12f});
                n.tracked[1] = n.squeezed[1] = true;
                n.at[0] = {0.0f, 0.0f, 0.0f};
                HoldStep(h, n, sc.g, sc.ring, yaw0, pitch0, sc.body);
                n.at[1] = HandsAt(sc, 1, yaw1, pitch1, {0.0f, -0.08f, -0.12f});
                HoldStep(h, n, sc.g, sc.ring, yaw0, pitch0, sc.body);
                float yaw = yaw0, pitch = pitch0;
                for (int f = 0; f < 4; ++f)  // (a few frames: it goes by steps from its last answer)
                    Solve(sc.g, sc.ring, h, n.at, yaw, pitch);
                worst = std::fmax(worst, Length(Wanted(h.hold[1], n.at[1], h.heading) - HandleAt(sc.g, sc.ring, h.hold[1].handle, yaw, pitch)));
            }
        }
        Report("h7", "one hand alone: its handle comes to it within a few frames (any yaw, pitch inside the limits)", worst < 0.001f, "worst %.2f mm",
               worst * 1000.0f);
    }
    {
        Scene sc;
        sc.g = TestGeometry(0);
        const Answer a = TakeAndMove(sc, 0.0f, 0.0f, {}, {}, 0.0f, 0.0f);
        (void)a;
        // Hands asking past the limits: pitch held at them.
        Holds h;
        HandsNow n;
        n.at[0] = HandsAt(sc, 0, 0.0f, 0.0f, {});
        n.at[1] = HandsAt(sc, 1, 0.0f, 0.0f, {});
        n.tracked[0] = n.tracked[1] = n.squeezed[0] = n.squeezed[1] = true;
        HoldStep(h, n, sc.g, sc.ring, 0.0f, 0.0f, sc.body);
        HandsNow low = n, high = n;
        low.at[0].y -= 0.4f, low.at[1].y -= 0.4f;     // handles down: the muzzle up, past +20
        high.at[0].y += 0.4f, high.at[1].y += 0.4f;   // handles up: the muzzle down, past -20
        float y1 = 0.0f, p1 = 0.0f, y2 = 0.0f, p2 = 0.0f;
        Solve(sc.g, sc.ring, h, low.at, y1, p1);
        Solve(sc.g, sc.ring, h, high.at, y2, p2);
        Report("h8", "the pitch limits: hands asking past them hold the gun at them", std::fabs(Deg(p1) - 20.0f) < 1e-3f && std::fabs(Deg(p2) + 20.0f) < 1e-3f,
               "%.3f and %.3f degrees", Deg(p1), Deg(p2));
    }
    {
        // The spread and the roll move nothing: the hands apart by 10 cm each way, one up and the other down by 4 cm.
        Scene sc;
        sc.g = TestGeometry(0);
        Holds h;
        HandsNow n;
        n.at[0] = HandsAt(sc, 0, 0.3f, 0.05f, {0.02f, -0.1f, -0.15f});
        n.at[1] = HandsAt(sc, 1, 0.3f, 0.05f, {-0.02f, -0.1f, -0.15f});
        n.tracked[0] = n.tracked[1] = n.squeezed[0] = n.squeezed[1] = true;
        HoldStep(h, n, sc.g, sc.ring, 0.3f, 0.05f, sc.body);
        V3 side = n.at[0] - n.at[1];  // (along the hands' own line, across the ground: a spread across it turns the line, and the gun with it)
        side.y = 0.0f;
        side = side * (1.0f / Length(side));
        HandsNow spread = n, roll = n;
        spread.at[0] = n.at[0] + side * 0.1f, spread.at[1] = n.at[1] - side * 0.1f;
        roll.at[0].y += 0.04f, roll.at[1].y -= 0.04f;
        float ys = 0.3f, ps = 0.05f, yr = 0.3f, pr = 0.05f;
        HoldStep(h, spread, sc.g, sc.ring, 0.3f, 0.05f, sc.body);
        Solve(sc.g, sc.ring, h, spread.at, ys, ps);
        HoldStep(h, roll, sc.g, sc.ring, 0.3f, 0.05f, sc.body);
        Solve(sc.g, sc.ring, h, roll.at, yr, pr);
        const float moved = std::fmax(std::fabs(Deg(Wrap(ys - 0.3f))) + std::fabs(Deg(ps - 0.05f)), std::fabs(Deg(Wrap(yr - 0.3f))) + std::fabs(Deg(pr - 0.05f)));
        Report("h9", "the hands' spread and roll move nothing (the gun can't stretch or roll)", moved < 0.05f, "%.4f degrees at most", moved);
    }
    {
        // The stick's orbit (the review's fifth finding): the hands still in the room, the body turned 90 degrees about the ring's axis: the room with it,
        // the gun's target with it, exactly. With the offsets kept in the world frame instead (the design as written), 26 degrees off on the truck.
        float off[2] = {}, broken = 0.0f;
        for (int pair = 0; pair < 2; ++pair) {
            Scene sc;
            sc.g = TestGeometry(0);
            Body b;
            b.have = true, b.yaw = 0.2f, b.ring = sc.ring;
            const float yaw0 = b.yaw, pitch0 = 0.0f;
            // The room: its centre at the eye; the hands placed in it where they reach the handles plus a 10 cm offset.
            const V3 centre{0.0f, 1.2f, 0.0f};
            const auto roomOf = [&](V3 world) {  // (the world point back into the room, the body as it is)
                const V3 d = Turn(-b.yaw, world - EyeAt(b, sc.g));
                return centre + V3{-d.x, d.y, -d.z};
            };
            V3 room[2] = {roomOf(HandsAt(sc, 0, yaw0, pitch0, Turn(b.yaw, {0.0f, -0.1f, -0.1f}))),
                          roomOf(HandsAt(sc, 1, yaw0, pitch0, Turn(b.yaw, {0.0f, -0.1f, -0.1f})))};
            Holds h;
            HandsNow n;
            for (int i = 0; i < 2; ++i) {
                n.at[i] = RoomToWorld(b, sc.g, room[i], centre);
                n.tracked[i] = true;
                n.squeezed[i] = pair == 1 || i == 1;  // (one hand, then both)
            }
            HoldStep(h, n, sc.g, sc.ring, yaw0, pitch0, b.yaw);
            const V3 takenWorld[2] = {n.at[0], n.at[1]};
            const Hold taken[2] = {h.hold[0], h.hold[1]};
            for (int f = 0; f < 90; ++f) {
                b.yaw += 1.0f * kDeg;
                for (int i = 0; i < 2; ++i)
                    n.at[i] = RoomToWorld(b, sc.g, room[i], centre);
                HoldStep(h, n, sc.g, sc.ring, yaw0, pitch0, b.yaw);
            }
            float yaw = yaw0, pitch = pitch0;
            for (int f = 0; f < 4; ++f)
                Solve(sc.g, sc.ring, h, n.at, yaw, pitch);
            off[pair] = std::fabs(Deg(Wrap(yaw - (yaw0 + 90.0f * kDeg))));
            if (pair == 1) {
                // The offsets in the world frame: wanted = hand - (the hand at the take - the handle at the take).
                Holds world;
                for (int i = 0; i < 2; ++i) {
                    world.hold[i] = taken[i];
                    world.hold[i].offset = takenWorld[i] - HandleAt(sc.g, sc.ring, taken[i].handle, yaw0, pitch0);
                }
                float wy = yaw0, wp = pitch0;
                for (int f = 0; f < 4; ++f)
                    Solve(sc.g, sc.ring, world, n.at, wy, wp);
                broken = std::fabs(Deg(Wrap(wy - (yaw0 + 90.0f * kDeg))));
            }
        }
        Report("h10", "the stick's orbit of 90 degrees with the hands still: the gun's target turns 90 with the body, one hand and two (offsets turned with it)",
               off[0] < 0.05f && off[1] < 0.05f, "off by %.4f (one hand) and %.4f (two) degrees", off[0], off[1]);
        Report("h11", "the design's first take (offsets kept in the world frame), shown failing: the same orbit leaves the truck's gun off", broken > 5.0f,
               "%.1f degrees off (the review: about 25)", broken);
    }
    {
        // A seated torso's turn ("hands and all"), the hands about the player's spine 12 cm behind the eye and 10 cm under it, the turret following the
        // target, on both guns with both hands and each alone: the gun's turn for the torso's and the hands' distance from the handles after their
        // offsets (step 2's let-go). The second review's fifth finding: not the Humvee's alone; on the truck too the handles' circle (about the post) and
        // the hands' (about the spine) are not one. Said for R10's turn-centre row (kRowTurnCentre, both guns), which step 2's hands wait on; the senses
        // checked: the Humvee's gun turns with the torso, the truck's against it (its handles go the torso's way, its barrel the other: the owner's call 1).
        std::string said;
        bool senses = true, finite = true;
        for (int which = 0; which < 2; ++which)
            for (int held = 0; held < 3; ++held) {  // (both, the left alone, the right alone)
                Scene sc;
                sc.g = TestGeometry(which);
                const float yaw0 = sc.body, pitch0 = 0.0f;
                const V3 eye = sc.ring + Turn(sc.body, sc.g.eye), spine = eye + Turn(sc.body, {0.0f, -0.1f, -0.12f});
                char line[200];
                std::snprintf(line, sizeof(line), "%s%s %s:", said.empty() ? "" : "; ", which == 0 ? "truck" : "Humvee",
                              held == 0 ? "both" : held == 1 ? "left" : "right");
                said += line;
                for (int side : {-1, 1})
                    for (int most : {20, 45}) {
                        Holds h;
                        HandsNow n;
                        n.at[0] = HandsAt(sc, 0, yaw0, pitch0, Turn(sc.body, {0.03f, -0.08f, -0.12f}));
                        n.at[1] = HandsAt(sc, 1, yaw0, pitch0, Turn(sc.body, {-0.03f, -0.08f, -0.12f}));
                        n.tracked[0] = n.tracked[1] = true;
                        n.squeezed[0] = held != 2, n.squeezed[1] = held != 1;
                        HoldStep(h, n, sc.g, sc.ring, yaw0, pitch0, sc.body);
                        const V3 start[2] = {n.at[0], n.at[1]};
                        float yaw = yaw0, pitch = pitch0, apart = 0.0f;
                        for (int deg = 1; deg <= most; ++deg) {
                            for (int i = 0; i < 2; ++i)
                                n.at[i] = spine + Turn(side * deg * kDeg, start[i] - spine);
                            HoldStep(h, n, sc.g, sc.ring, yaw, pitch, sc.body);
                            Solve(sc.g, sc.ring, h, n.at, yaw, pitch);
                            for (int i = 0; i < 2; ++i)
                                if (h.hold[i].on)
                                    apart = std::fmax(apart, Apart(h.hold[i], n.at[i], h.heading, sc.g, sc.ring, yaw, pitch));
                        }
                        const float gun = Deg(Wrap(yaw - yaw0)) * static_cast<float>(side);  // (positive: the gun turned the torso's way)
                        finite = finite && std::isfinite(gun) && std::isfinite(apart);
                        if (held == 0)
                            senses = senses && (which == 1 ? gun > 0.0f : gun < 0.0f);
                        std::snprintf(line, sizeof(line), " %+d->%+.0f (%.0f cm)", side * most, gun, apart * 100.0f);
                        said += line;
                    }
            }
        Report("h12", "a seated torso's turn (20 and 45 each way), both guns, both hands and each alone: the gun's turn and the hands' distance said for R10's turn centre",
               senses && finite, "the torso -> the gun its way (the hands from the handles): %s", said.c_str());
    }
    {
        // A hand joining and leaving (the frame changing from the body's to the hands' line and back): the gun's target never jumps.
        Scene sc;
        sc.g = TestGeometry(0);
        Holds h;
        HandsNow n;
        n.at[0] = HandsAt(sc, 0, 0.25f, 0.1f, Turn(0.25f, {0.02f, -0.1f, -0.2f}));
        n.at[1] = HandsAt(sc, 1, 0.25f, 0.1f, Turn(0.25f, {-0.05f, -0.12f, -0.15f}));
        n.tracked[0] = n.tracked[1] = true;
        n.squeezed[1] = true;
        HoldStep(h, n, sc.g, sc.ring, 0.25f, 0.1f, sc.body);
        float yaw = 0.25f, pitch = 0.1f, most = 0.0f;
        for (int f = 0; f < 4; ++f)
            Solve(sc.g, sc.ring, h, n.at, yaw, pitch);
        const auto step = [&](bool left) {
            n.squeezed[0] = left;
            HoldStep(h, n, sc.g, sc.ring, yaw, pitch, sc.body);
            float y = yaw, p = pitch;
            for (int f = 0; f < 4; ++f)
                Solve(sc.g, sc.ring, h, n.at, y, p);
            most = std::fmax(most, std::fabs(Deg(Wrap(y - yaw))) + std::fabs(Deg(p - pitch)));
            yaw = y, pitch = p;
        };
        step(true);
        step(true);
        step(false);
        step(true);
        Report("h13", "a hand taking hold and letting go (the frame changing between the body's and the hands' line): the gun's target never jumps",
               most < 0.01f && h.pair, "%.4f degrees at most", most);
    }
    {
        // Step 2's rules: a take only within reach by a fresh squeeze; a let-go past 25 cm (after the offset).
        Scene sc;
        sc.g = TestGeometry(0);
        HoldRules two;
        two.reach = kReach, two.letGo = kLetGo;
        Holds h;
        HandsNow n;
        n.at[0] = HandsAt(sc, 0, 0.0f, 0.0f, {0.0f, -0.3f, 0.0f});  // 30 cm under: out of reach
        n.tracked[0] = true, n.squeezed[0] = true;
        const HoldEvents e1 = HoldStep(h, n, sc.g, sc.ring, 0.0f, 0.0f, sc.body, two);
        n.at[0] = HandsAt(sc, 0, 0.0f, 0.0f, {0.0f, -0.05f, 0.0f});  // in reach, still squeezed: not fresh
        const HoldEvents e2 = HoldStep(h, n, sc.g, sc.ring, 0.0f, 0.0f, sc.body, two);
        n.squeezed[0] = false;
        HoldStep(h, n, sc.g, sc.ring, 0.0f, 0.0f, sc.body, two);
        n.squeezed[0] = true;
        const HoldEvents e3 = HoldStep(h, n, sc.g, sc.ring, 0.0f, 0.0f, sc.body, two);
        n.at[0] = n.at[0] + V3{0.0f, -0.3f, 0.0f};
        const HoldEvents e4 = HoldStep(h, n, sc.g, sc.ring, 0.0f, 0.0f, sc.body, two);
        Report("h14", "step 2's rules: a fresh squeeze within 12 cm takes; 30 cm from it lets go", !e1.took[0] && !e2.took[0] && e3.took[0] && e4.letGo[0],
               "out of reach %d, not fresh %d, fresh in reach %d, let go %d", e1.took[0], e2.took[0], e3.took[0], e4.letGo[0]);
    }
    {
        // The second review's tenth finding: both hands holding, the right lost (not tracked) with its place its last one, 30 cm off, or (0, 0, 0): the
        // left's target the same each way (the frame changed from the last frame's heading, never the lost hand's place). Before: 7.6 and 10.3 degrees.
        float most = 0.0f;
        for (int which = 0; which < 2; ++which) {
            const auto lost = [&](V3 place) {
                Scene sc;
                sc.g = TestGeometry(which);
                Holds h;
                HandsNow n;
                n.at[0] = HandsAt(sc, 0, 0.3f, 0.05f, Turn(0.3f, {0.03f, -0.1f, -0.15f}));
                n.at[1] = HandsAt(sc, 1, 0.3f, 0.05f, Turn(0.3f, {-0.04f, -0.1f, -0.12f}));
                n.tracked[0] = n.tracked[1] = n.squeezed[0] = n.squeezed[1] = true;
                HoldStep(h, n, sc.g, sc.ring, 0.3f, 0.05f, sc.body);
                n.at[0] = n.at[0] + V3{0.02f, 0.01f, 0.0f};  // (the left moves on a little)
                HoldStep(h, n, sc.g, sc.ring, 0.3f, 0.05f, sc.body);
                n.tracked[1] = false;
                n.at[1] = place.x == 1e9f ? n.at[1] : place;
                HoldStep(h, n, sc.g, sc.ring, 0.3f, 0.05f, sc.body);
                float yaw = 0.3f, pitch = 0.05f;
                for (int f = 0; f < 4; ++f)
                    Solve(sc.g, sc.ring, h, n.at, yaw, pitch);
                return Answer{yaw, pitch};
            };
            Scene sc;
            sc.g = TestGeometry(which);
            const V3 last = HandsAt(sc, 1, 0.3f, 0.05f, Turn(0.3f, {-0.04f, -0.1f, -0.12f}));
            const Answer base = lost({1e9f, 0.0f, 0.0f}), off = lost(last + V3{0.3f, 0.0f, 0.0f}), zero = lost({0.0f, 0.0f, 0.0f});
            for (const Answer& a : {off, zero})
                most = std::fmax(most, std::fabs(Deg(Wrap(a.yaw - base.yaw))) + std::fabs(Deg(a.pitch - base.pitch)));
        }
        Report("h15", "a holding hand lost, its place 30 cm off or (0, 0, 0): the other hand's target moves nothing (the last frame's heading kept)", most < 1e-3f,
               "%.5f degrees at most (before: 7.6 and 10.3)", most);
    }
}

// ------------------------------------------------------------------ o: the orbit ------------------------------------------------------------------

void Orbit() {
    {
        const Geometry truck = TestGeometry(0), humvee = TestGeometry(1);
        const float dt = 1.0f / 72.0f, foot = 266.0f * kDeg * dt;  // the on-foot turn at full stick, a frame
        const float t = OrbitTurn(foot, dt, truck), hv = OrbitTurn(foot, dt, humvee);
        const float tr = std::sqrt(truck.eye.x * truck.eye.x + truck.eye.z * truck.eye.z), hr = std::sqrt(humvee.eye.x * humvee.eye.x + humvee.eye.z * humvee.eye.z);
        const float truckSpeed = t / dt * tr, humveeSpeed = hv / dt * hr;
        Report("o1", "the stick's orbit: the eye at most 0.75 m/s on its circle, the truck's 0.63 m arm and the Humvee's 0.18 m alike, never past the on-foot turn",
               truckSpeed <= kOrbitEyeSpeed + 1e-4f && humveeSpeed <= kOrbitEyeSpeed + 1e-4f && hv <= foot + 1e-6f && hv > t && t > 0.0f,
               "the truck %.0f degrees a second (eye %.2f m/s; on foot it'd be %.2f), the Humvee %.0f (eye %.2f m/s)", Deg(t / dt), truckSpeed,
               266.0f * kDeg * tr, Deg(hv / dt), humveeSpeed);
    }
    {
        PushRules off;
        PushRules on;
        on.on = true;
        const float dt = 1.0f / 72.0f;
        const float none = PushTurn(80.0f * kDeg, 0.0f, dt, off), inside = PushTurn(40.0f * kDeg, 0.0f, dt, on), past = PushTurn(80.0f * kDeg, 0.0f, dt, on),
                    left = PushTurn(-120.0f * kDeg, 0.0f, dt, on);
        Report("o2", "mountpush: off by default; on, the body turns past 50 degrees toward the gun, at most 60 a second",
               none == 0.0f && inside == 0.0f && past > 0.0f && Deg(past / dt) <= 60.0f + 1e-3f && left < 0.0f && Deg(-left / dt) <= 60.0f + 1e-3f,
               "off %.1f, at 40 %.1f, at 80 %.1f, at -120 %.1f degrees a second", Deg(none / dt), Deg(inside / dt), Deg(past / dt), Deg(left / dt));
    }
}

// ------------------------------------------------------------------ m: the smoothing ------------------------------------------------------------------

void Smoothing() {
    // A 1 cm wobble at a 0.3 m lever (about 2 degrees) at 6 Hz on a still target; a sweep of 90 degrees a second.
    Smooth f;
    const float dt = 1.0f / 72.0f;
    float most = 0.0f;
    for (int k = 0; k < 720; ++k) {
        const float x = 2.0f * kDeg * std::sin(2.0f * kPi * 6.0f * k * dt);
        const float y = SmoothStep(f, x, dt, true);
        if (k > 144)
            most = std::fmax(most, std::fabs(y));
    }
    Smooth g;
    float lag = 0.0f;
    for (int k = 0; k < 216; ++k) {
        const float x = Wrap(90.0f * kDeg * k * dt);
        const float y = SmoothStep(g, x, dt, true);
        if (k > 108)
            lag = std::fmax(lag, std::fabs(Wrap(x - y)));
    }
    Report("m1", "the target's smoothing: a hand's 2-degree wobble at 6 Hz held to a fraction; a 90-degree-a-second sweep followed closely",
           Deg(most) < 0.6f && Deg(lag) < 3.0f, "the wobble %.2f degrees, the sweep's lag %.2f degrees", Deg(most), Deg(lag));
}

// ------------------------------------------------------------------ t: the steering against a simulated turret ------------------------------------------------------------------

// The seat's pad curve as the dump has it (PadInputCurve_OpenGunner_Roll/_Pitch): a dead zone to 0.15, then 0.03 at 0.25, 0.066 at 0.75, 0.125 at
// 0.875, 0.2 from 0.95; after the game's own pad dead zone (the owner's profile: 0.30), applied first and taken out of the range.
float SeatCurve(float stick, float gameDeadZone) {
    float s = std::fabs(stick);
    s = s <= gameDeadZone ? 0.0f : (s - gameDeadZone) / (1.0f - gameDeadZone);
    const float xs[] = {0.15f, 0.25f, 0.75f, 0.875f, 0.95f, 1.0f}, ys[] = {0.0f, 0.03f, 0.066f, 0.125f, 0.2f, 0.2f};
    float out = 0.0f;
    if (s > xs[0]) {
        out = ys[5];
        for (int i = 1; i < 6; ++i)
            if (s <= xs[i]) {
                out = ys[i - 1] + (ys[i] - ys[i - 1]) * (s - xs[i - 1]) / (xs[i] - xs[i - 1]);
                break;
            }
    }
    return std::copysign(out, stick);
}

struct Turret {
    // The truth.
    float most = 300.0f;   // degrees a second at the game's whole input (the pad's curve gives at most 0.2 of it)
    float per = 0.2f;      // each tick the rate moves this part of the way to what the input asks (the momentum)
    int delayTicks = 1;
    float threshold = 0.0f;  // degrees a second: a rate under this with the input asking under it stops dead
    float sign = 1.0f;
    bool limited = false;
    float lo = -20.0f, hi = 20.0f;
    bool pad = false;          // the pad's right stick; else the mouse
    float gameDeadZone = 0.3f; // the pad's
    float countsFull = 40.0f;  // the mouse: counts in a tick for the whole input
    bool interp = false;       // the camera between the last two ticks (one behind), else at the last tick
    bool deaf = false;         // the game not reading the input
    // State.
    float angle = 0.0f, rate = 0.0f, prev = 0.0f;
    double nextTick = 0.0, lastTick = 0.0;
    float stick = 0.0f;
    int counts = 0;
    std::deque<float> queue;

    float LagSeconds() const { return -kTick / std::log(1.0f - per); }
    void Frame(double now, float stickNow, int countsNow) {
        stick = stickNow;
        counts += countsNow;
        while (nextTick <= now) {
            float in = pad ? SeatCurve(stick, gameDeadZone) : std::fmax(-1.0f, std::fmin(1.0f, counts / countsFull));
            counts = 0;
            if (deaf)
                in = 0.0f;
            queue.push_back(in);
            float act = 0.0f;
            if (static_cast<int>(queue.size()) > delayTicks) {
                act = queue.front();
                queue.pop_front();
            }
            const float wanted = sign * act * most;
            rate += (wanted - rate) * per;
            if (std::fabs(rate) < threshold && std::fabs(wanted) < threshold)
                rate = 0.0f;
            prev = angle;
            angle += rate * kTick;
            if (limited) {
                if (angle > hi)
                    angle = hi, rate = std::fmin(rate, 0.0f);
                if (angle < lo)
                    angle = lo, rate = std::fmax(rate, 0.0f);
            }
            lastTick = nextTick;
            nextTick += kTick;
        }
    }
    float Camera(double now) const {
        if (!interp)
            return angle;
        const float k = static_cast<float>(std::fmin(1.0, std::fmax(0.0, (now - lastTick) / kTick)));
        return prev + (angle - prev) * k;
    }
};

// The rows a turret gives (as R5 would measure them): its rate at the pad's sticks, the mouse's degrees a count, its lag, its delay (to the first turn
// the camera shows), the least input that moves it.
struct Way {
    AxisRows rows;
    PadTable table;
    float degreesPerCount = 0.0f, countsPerSecond = 0.0f;
};
Way WayOf(const Turret& t, float scaleMost = 1.0f, float scaleLag = 1.0f) {
    Way w;
    if (t.pad) {
        // (R5 at the dead zone's edge and the curve's knees, through the game's own dead zone)
        const float knees[6] = {0.15f, 0.25f, 0.75f, 0.875f, 0.95f, 1.0f};
        w.table.n = 6;
        for (int i = 0; i < 6; ++i) {
            const float stick = t.gameDeadZone + knees[i] * (1.0f - t.gameDeadZone);
            w.table.stick[i] = stick, w.table.rate[i] = SeatCurve(stick, t.gameDeadZone) * t.most * scaleMost;
        }
        w.rows.most = PadMost(w.table);
    } else {
        w.degreesPerCount = t.most * kTick / t.countsFull * scaleMost;
        w.countsPerSecond = t.countsFull / kTick;
        w.rows.most = w.degreesPerCount * w.countsPerSecond;
    }
    w.rows.lag = t.LagSeconds() * scaleLag;
    // (as R5 measures it, from an input to the camera's first turn: the game's delay, the camera's place in the tick, and for the pad the wait for the
    // tick that reads the stick, a tick at most)
    w.rows.delay = (t.delayTicks + (t.interp ? 1.0f : 0.5f) + (t.pad ? 1.0f : 0.0f)) * kTick;
    w.rows.least = t.threshold > 0.0f ? std::fmin(1.0f, 1.2f * t.threshold / w.rows.most) : 0.0f;
    w.rows.sign = 1.0f;
    if (t.limited)  // (the pitch's row: the stops as measured)
        w.rows.lo = t.lo, w.rows.hi = t.hi;
    return w;
}

struct Run {
    float overshoot = 0.0f;  // past the target the way it was going (degrees)
    float settle = -1.0f;    // seconds to within 0.25 degrees for good (-1 never)
    float finalError = 0.0f;
    int reversals = 0;       // the gun's own reversals once within 2 degrees (its motion turned back by 0.1 degree or more)
    float mostAsk = 0.0f;    // the largest stick or counts a frame sent
    float mostRate = 0.0f;   // the turret's fastest
    SteerStop stop = SteerStop::kNone;
    float sentWhenNot = 0.0f;  // anything sent while not allowed
    float excursion = 0.0f;    // the furthest the angle went from its start
};

// One axis stepped from 0 to `target` (or along target(t)), steered by the law at fps for `seconds`.
template <class Target, class Allowed>
Run Steer(Turret t, const Way& w, Target target, Allowed allowed, float seconds, float fps, bool wrap, const SteerRules& rules = SteerRules{}) {
    Run r;
    AxisState s;
    MouseCarry carry;
    const float dt = 1.0f / fps;
    const float goal = target(1e9);
    const float from = t.angle;
    bool within = false;
    float lastSign = 0.0f, turnAt = t.angle;
    double settledAt = -1.0;
    for (int f = 0; f < static_cast<int>(seconds * fps); ++f) {
        const double now = f * dt;
        const float cam = t.Camera(now);
        const bool ok = allowed(now);
        const AxisOut o = SteerAxis(s, w.rows, target(now), cam, now, wrap, ok, rules);
        float stick = 0.0f;
        int counts = 0;
        if (t.pad) {
            stick = PadStick(w.table, o.ask);
        } else {
            counts = MouseCounts(carry, o.ask, dt, w.degreesPerCount, static_cast<int>(w.countsPerSecond * dt));
        }
        if (!ok && (stick != 0.0f || counts != 0))
            r.sentWhenNot += std::fabs(stick) + std::abs(counts);
        r.mostAsk = std::fmax(r.mostAsk, t.pad ? std::fabs(stick) : static_cast<float>(std::abs(counts)));
        t.Frame(now + dt, stick, counts);
        r.mostRate = std::fmax(r.mostRate, std::fabs(t.rate));
        const float err = goal - t.angle;
        const float dir = goal - from > 0.0f ? 1.0f : -1.0f;
        r.overshoot = std::fmax(r.overshoot, -err * dir);
        r.excursion = std::fmax(r.excursion, std::fabs(t.angle - from));
        if (std::fabs(err) < 2.0f)
            within = true;
        // (the gun's motion's sign, with 0.1 degree of hysteresis from where it last turned)
        const float moved = t.angle - turnAt;
        if (std::fabs(moved) >= 0.1f) {
            const float sign = moved > 0.0f ? 1.0f : -1.0f;
            if (within && lastSign != 0.0f && sign != lastSign)
                ++r.reversals;
            lastSign = sign;
            turnAt = t.angle;
        } else if (lastSign * moved > 0.0f) {
            turnAt = t.angle;  // (still going its way)
        }
        if (std::fabs(err) < 0.25f) {
            if (settledAt < 0.0)
                settledAt = now + dt;
        } else {
            settledAt = -1.0;
        }
        r.finalError = err;
        r.stop = s.stop;
    }
    r.settle = static_cast<float>(settledAt);
    return r;
}

auto Always() {
    return [](double) { return true; };
}
auto StepTo(float goal) {
    return [goal](double) { return goal; };
}

void Steering() {
    // t1: steps against a grid of turrets (the momentum from 0.1 to 0.6 a tick, a delay of 1 or 2 ticks, the camera at the tick or between, both inputs,
    // the pad's and the mouse's own most), the rows right.
    {
        int runs = 0, bad = 0;
        float worstOver = 0.0f, worstSettle = 0.0f;
        char worstCase[160] = "";
        for (int pad = 0; pad < 2; ++pad)
            for (float per : {0.1f, 0.2f, 0.32f, 0.6f})
                for (int delay : {1, 2})
                    for (int interp = 0; interp < 2; ++interp)
                        for (float step : {3.0f, 15.0f, 60.0f, -120.0f}) {
                            Turret t;
                            t.pad = pad != 0, t.per = per, t.delayTicks = delay, t.interp = interp != 0;
                            t.most = pad ? 300.0f : 150.0f;
                            const Way w = WayOf(t);
                            const Run r = Steer(t, w, StepTo(step), Always(), 6.0f, 72.0f, true);
                            ++runs;
                            const float allowed = std::fabs(step) / w.rows.most + w.rows.lag * 8.0f + w.rows.delay + 1.5f;
                            const bool good = r.overshoot < 0.3f && r.settle >= 0.0f && r.settle < allowed && r.reversals <= 2 && r.stop == SteerStop::kNone;
                            if (!good) {
                                ++bad;
                                if (bad <= 8)
                                    std::printf("     t1 bad: %s per %.2f delay %d %s step %.0f: over %.2f settle %.2f of %.2f, reversals %d, stop %d, final %.2f\n",
                                                pad ? "pad" : "mouse", per, delay, interp ? "between" : "at tick", step, r.overshoot, r.settle, allowed, r.reversals,
                                                static_cast<int>(r.stop), r.finalError);
                            }
                            if (r.overshoot > worstOver || !good) {
                                worstOver = std::fmax(worstOver, r.overshoot);
                                std::snprintf(worstCase, sizeof(worstCase), "%s per %.2f delay %d %s step %.0f: over %.2f settle %.2f/%.2f rev %d stop %d",
                                              pad ? "pad" : "mouse", per, delay, interp ? "between" : "at tick", step, r.overshoot, r.settle, allowed, r.reversals,
                                              static_cast<int>(r.stop));
                            }
                            if (r.settle > 0.0f)
                                worstSettle = std::fmax(worstSettle, r.settle / allowed);
                        }
        Report("t1", "steps of 3 to 120 degrees against 64 turrets, the rows right: each settles within 0.25 degrees, no overshoot past 0.3, no hunt",
               bad == 0, "%d of %d bad; worst overshoot %.3f degrees; settle at most %.0f%% of its allowance; worst: %s", bad, runs, worstOver,
               worstSettle * 100.0f, worstCase);
    }
    // t2: the rows off (the strength half or twice, the lag half or twice).
    {
        int runs = 0, bad = 0;
        float worstOver = 0.0f;
        char worstCase[160] = "";
        for (float sm : {0.5f, 2.0f})
            for (float sl : {0.5f, 2.0f})
                for (int pad = 0; pad < 2; ++pad)
                    for (float per : {0.15f, 0.32f})
                        for (float step : {10.0f, -45.0f}) {
                            Turret t;
                            t.pad = pad != 0, t.per = per, t.delayTicks = 1;
                            t.most = pad ? 300.0f : 150.0f;
                            const Way w = WayOf(t, sm, sl);
                            const Run r = Steer(t, w, StepTo(step), Always(), 9.0f, 72.0f, true);
                            ++runs;
                            const bool good = r.overshoot < 5.0f && std::fabs(r.finalError) < 0.5f && r.stop == SteerStop::kNone && r.reversals <= 3;
                            bad += good ? 0 : 1;
                            if (!good && bad <= 8)
                                std::printf("     t2 bad: strength x%.1f lag x%.1f %s per %.2f step %.0f: over %.2f settle %.2f, reversals %d, stop %d, final %.2f\n", sm, sl,
                                            pad ? "pad" : "mouse", per, step, r.overshoot, r.settle, r.reversals, static_cast<int>(r.stop), r.finalError);
                            if (r.overshoot >= worstOver || !good) {
                                worstOver = std::fmax(worstOver, r.overshoot);
                                std::snprintf(worstCase, sizeof(worstCase), "strength x%.1f lag x%.1f %s per %.2f step %.0f: over %.2f settle %.2f stop %d", sm, sl,
                                              pad ? "pad" : "mouse", per, step, r.overshoot, r.settle, static_cast<int>(r.stop));
                            }
                        }
        Report("t2", "the rows off by half or twice (its strength, its lag) before any learning: converging, no hunt, no false stop, overshoot under 5 degrees", bad == 0,
               "%d of %d bad; worst overshoot %.2f; worst: %s", bad, runs, worstOver, worstCase);
    }
    // t3: saturation: a 170-degree step: the input at its most (the pad's whole stick, the mouse's most counts), the turret at its most rate, settling.
    {
        bool ok = true;
        char said[200] = "";
        for (int pad = 0; pad < 2; ++pad) {
            Turret t;
            t.pad = pad != 0;
            t.most = pad ? 300.0f : 150.0f;
            const Way w = WayOf(t);
            const Run r = Steer(t, w, StepTo(170.0f), Always(), 8.0f, 72.0f, true);
            const float mostInput = pad ? 1.0f : static_cast<float>(static_cast<int>(w.countsPerSecond / 72.0f));
            // (the mouse's counts come to the game's ticks two or three frames at a time, cut at the tick's most: 85% of the most rate there)
            ok = ok && r.mostAsk <= mostInput + 1e-4f && r.mostAsk >= mostInput - 1e-4f && r.mostRate > (pad ? 0.95f : 0.85f) * w.rows.most &&
                 r.overshoot < 0.3f && r.settle > 0.0f;
            char line[100];
            std::snprintf(line, sizeof(line), "%s%s: input at most %.2f of %.2f, rate %.0f of %.0f, over %.2f", pad ? "; " : "", pad ? "pad" : "mouse", r.mostAsk,
                          mostInput, r.mostRate, w.rows.most, r.overshoot);
            std::strcat(said, line);
        }
        Report("t3", "saturation: a 170-degree step asks the input's most, turns at the turret's most, and still stops on the target", ok, "%s", said);
    }
    // t4: a sweep: the target at 30 degrees a second, then stopping: the lag while it sweeps, the overshoot after.
    {
        bool ok = true;
        char said[200] = "";
        for (int pad = 0; pad < 2; ++pad) {
            Turret t;
            t.pad = pad != 0;
            t.most = pad ? 300.0f : 150.0f;
            const Way w = WayOf(t);
            AxisState s;
            MouseCarry carry;
            float lagMost = 0.0f, over = 0.0f;
            const float dt = 1.0f / 72.0f;
            for (int f = 0; f < 72 * 6; ++f) {
                const double now = f * dt;
                // 30 degrees a second, eased to a stop over its last quarter second as a hand stops (the sweep's end at 3 s, at 86.25 degrees)
                const double e = now < 2.75 ? 0.0 : std::fmin(1.0, (now - 2.75) / 0.25);
                const float goal = now < 2.75 ? 30.0f * static_cast<float>(now) : 82.5f + 30.0f * 0.25f * static_cast<float>(e - e * e / 2.0);
                const AxisOut o = SteerAxis(s, w.rows, goal, t.Camera(now), now, true, true);
                const float stick = t.pad ? PadStick(w.table, o.ask) : 0.0f;
                const int counts = t.pad ? 0 : MouseCounts(carry, o.ask, dt, w.degreesPerCount, static_cast<int>(w.countsPerSecond * dt));
                t.Frame(now + dt, stick, counts);
                if (std::getenv("MOUNTGUN_DEBUG") && f % 18 == 0)
                    std::printf("     t4 %s %.2f goal %.2f angle %.2f model %.1f rate %.1f target %.1f asked %.1f\n", pad ? "pad" : "mouse", now, goal, t.angle, s.model,
                                s.rate, s.targetRate, o.rate);
                if (now > 1.0 && now < 2.75)
                    lagMost = std::fmax(lagMost, std::fabs(goal - t.angle));
                if (now >= 3.0)
                    over = std::fmax(over, t.angle - 86.25f);
            }
            ok = ok && lagMost < 1.5f && over < 1.0f;
            char line[100];
            std::snprintf(line, sizeof(line), "%s%s: lag %.2f, over %.2f", pad ? "; " : "", pad ? "pad" : "mouse", lagMost, over);
            std::strcat(said, line);
        }
        Report("t4", "a sweep at 30 degrees a second followed with no standing lag (within 1.5 degrees: the game's tick alone turns it 1), stopping as a hand stops (a quarter second) within 1 degree", ok,
               "%s", said);
    }
    {
        // The worst case: the target stopping dead from 30 degrees a second (no hand stops so): its overshoot kept small.
        Turret t;
        t.most = 150.0f;
        const Way w = WayOf(t);
        AxisState s;
        MouseCarry carry;
        float over = 0.0f;
        const float dt = 1.0f / 72.0f;
        for (int f = 0; f < 72 * 6; ++f) {
            const double now = f * dt;
            const float goal = now < 3.0 ? 30.0f * static_cast<float>(now) : 90.0f;
            const AxisOut o = SteerAxis(s, w.rows, goal, t.Camera(now), now, true, true);
            t.Frame(now + dt, 0.0f, MouseCounts(carry, o.ask, dt, w.degreesPerCount, static_cast<int>(w.countsPerSecond * dt)));
            if (now >= 3.0)
                over = std::fmax(over, t.angle - 90.0f);
        }
        Report("t4b", "the target stopping dead from 30 degrees a second (no hand stops so): its overshoot held under 2 degrees", over < 2.0f, "%.2f degrees", over);
    }
    // t5: dead zones and a momentum threshold: small steps settle within the least's own coast, no hunt.
    {
        bool ok = true;
        char said[200] = "";
        for (int pad = 0; pad < 2; ++pad) {
            Turret t;
            t.pad = pad != 0;
            t.most = pad ? 300.0f : 150.0f;
            t.threshold = 4.0f;  // degrees a second
            const Way w = WayOf(t);
            const float coast = w.rows.least * w.rows.most * (w.rows.lag + w.rows.delay);
            float worst = 0.0f;
            int revMost = 0;
            for (float step : {0.5f, 1.0f, 2.0f, -3.0f, 8.0f}) {
                const Run r = Steer(t, w, StepTo(step), Always(), 5.0f, 72.0f, true);
                worst = std::fmax(worst, std::fabs(r.finalError));
                revMost = std::max(revMost, r.reversals);
            }
            ok = ok && worst <= std::fmax(coast, 0.25f) + 0.05f && revMost <= 2;
            char line[120];
            std::snprintf(line, sizeof(line), "%s%s: standing error %.2f (the least's coast %.2f), reversals %d", pad ? "; " : "", pad ? "pad" : "mouse", worst,
                          coast, revMost);
            std::strcat(said, line);
        }
        Report("t5", "the dead zones and a momentum threshold: small steps settle within the least input's own coast, never hunting", ok, "%s", said);
    }
    // t6: nothing sent when it mustn't be (no hand, the climb-in, a stale camera, the game behind): the input at rest that frame, nothing left standing;
    // the turret coasts to a stop.
    {
        bool ok = true;
        float sentNot = 0.0f, drift = 0.0f;
        for (int pad = 0; pad < 2; ++pad) {
            Turret t;
            t.pad = pad != 0;
            t.most = pad ? 300.0f : 150.0f;
            const Way w = WayOf(t);
            const auto allowed = [](double now) { return now < 0.6 || (now > 2.0 && now < 2.05) || now > 4.0; };
            const Run r = Steer(t, w, StepTo(100.0f), allowed, 3.9f, 72.0f, true);
            sentNot += r.sentWhenNot;
            drift = std::fmax(drift, r.mostRate);
            ok = ok && r.sentWhenNot == 0.0f;
        }
        Report("t6", "nothing sent the frame it mustn't be (no hand on a handle, the climb-in, a stale camera, the game behind): no input left standing", ok,
               "sent while not allowed: %.2f", sentNot);
    }
    // t7: a wrong sign (the game's vehicle invert): the law stops itself before the gun reaches its stop; without the guards it runs into it.
    {
        Turret t;
        t.sign = -1.0f;
        t.limited = true, t.lo = -20.0f, t.hi = 20.0f;
        t.most = 150.0f;
        const Way w = WayOf(t);
        const Run guarded = Steer(t, w, StepTo(15.0f), Always(), 4.0f, 72.0f, false);
        SteerRules none;
        none.guards = false;
        const Run unguarded = Steer(t, w, StepTo(15.0f), Always(), 4.0f, 72.0f, false, none);
        // (and after the stop nothing more: the gun rests where the turret's momentum leaves it, never held at its stop)
        Turret t2 = t;
        AxisState s2;
        MouseCarry c2;
        double stoppedAt = -1.0;
        float pushedAfter = 0.0f;
        const float dt = 1.0f / 72.0f;
        for (int f = 0; f < 72 * 3; ++f) {
            const double now = f * dt;
            const AxisOut o = SteerAxis(s2, w.rows, 15.0f, t2.Camera(now), now, false, true);
            const int counts = MouseCounts(c2, o.ask, dt, w.degreesPerCount, static_cast<int>(w.countsPerSecond * dt));
            if (stoppedAt >= 0.0)
                pushedAfter += std::abs(counts);
            t2.Frame(now + dt, 0.0f, counts);
            if (s2.stop != SteerStop::kNone && stoppedAt < 0.0)
                stoppedAt = now;
        }
        Report("t7", "a wrong sign (the game's vehicle invert): the law stops itself within a few ticks, said, and pushes nothing after",
               s2.stop == SteerStop::kWrongWay && stoppedAt > 0.0 && stoppedAt < 0.35 && pushedAfter == 0.0f,
               "%s at %.2f s; it went %.1f degrees the wrong way (a turret of 150 a second); after: %.0f counts", SteerStopName(s2.stop), stoppedAt,
               guarded.excursion, pushedAfter);
        Report("t8", "the same without the guards, shown failing: the gun driven into its stop and held there", unguarded.excursion >= 19.9f,
               "%.1f degrees the wrong way (its stop at 20)", unguarded.excursion);
    }
    // t9: no answer (the game not reading the input): stopped within about a second, said.
    {
        Turret t;
        t.deaf = true;
        t.most = 150.0f;
        const Way w = WayOf(t);
        AxisState s;
        MouseCarry carry;
        double stoppedAt = -1.0;
        const float dt = 1.0f / 72.0f;
        for (int f = 0; f < 72 * 3; ++f) {
            const double now = f * dt;
            SteerAxis(s, w.rows, 30.0f, t.Camera(now), now, true, true);
            t.Frame(now + dt, 0.0f, MouseCounts(carry, 10.0f, dt, w.degreesPerCount, 100));
            if (s.stop != SteerStop::kNone && stoppedAt < 0.0)
                stoppedAt = now;
        }
        Report("t9", "no answer (the game not reading the input): stopped within about a second", s.stop == SteerStop::kNoAnswer && stoppedAt > 0.3 && stoppedAt < 1.6,
               "%s at %.2f s", SteerStopName(s.stop), stoppedAt);
    }
    // t10: the strength learned only in steady input: a turret 1.6 times the row's learns it within 10%; one 3 times it stops at twice (the bound); and the
    // design's first learner (the camera's turn over the last half second against what was sent, whatever the input did) shown wrong by the coast.
    {
        float learned[2] = {};
        for (int k = 0; k < 2; ++k) {
            Turret t;
            t.most = 150.0f * (k == 0 ? 1.6f : 3.0f);
            const Way w = WayOf(t, 1.0f / (k == 0 ? 1.6f : 3.0f));  // (the rows say 150)
            AxisState s;
            MouseCarry carry;
            const float dt = 1.0f / 72.0f;
            for (int f = 0; f < 72 * 8; ++f) {
                const double now = f * dt;
                const float goal = (k == 0 ? 60.0f : 100.0f) * static_cast<float>(now);  // (a steady sweep: the input steady, over the learning's floor)
                const AxisOut o = SteerAxis(s, w.rows, goal, t.Camera(now), now, true, true);
                t.Frame(now + dt, 0.0f, MouseCounts(carry, o.ask, dt, w.degreesPerCount, static_cast<int>(w.countsPerSecond * dt)));
                if (std::getenv("MOUNTGUN_DEBUG") && k == 1 && f % 9 == 0 && f < 72 * 2)
                    std::printf("     t10 trace %.3f goal %.1f angle %.1f model %.1f rate %.1f asked %.1f steady %.2f window %d\n", now, goal, t.angle, s.model, s.rate,
                                o.rate, s.steadySince, s.window);
            }
            learned[k] = s.gain;
            if (std::getenv("MOUNTGUN_DEBUG"))
                std::printf("     t10 %d: gain %.2f learned %d stop %d\n", k, s.gain, s.learned, static_cast<int>(s.stop));
        }
        // The first learner: start-stop moves, half a second windows, the turn against what was sent in it (the coast after a stop counted).
        float naive = 0.0f;
        {
            Turret t;
            t.most = 150.0f;
            t.per = 0.12f;
            const float dt = 1.0f / 72.0f;
            const Way w = WayOf(t);
            float sentSum = 0.0f, turnedFrom = 0.0f;
            double windowAt = 0.0;
            float ratioSum = 0.0f, ratioWorst = 0.0f;
            int windows = 0;
            MouseCarry carry;
            for (int f = 0; f < 72 * 6; ++f) {
                const double now = f * dt;
                const bool on = std::fmod(now, 0.5) < 0.2;  // (0.2 s pushes, 0.3 s coasting)
                const float rate = on ? 100.0f : 0.0f;
                const int counts = MouseCounts(carry, rate, dt, w.degreesPerCount, 1000);
                sentSum += counts * w.degreesPerCount;
                t.Frame(now + dt, 0.0f, counts);
                if (now - windowAt >= 0.45) {  // (windows a little off the pushes' rhythm, as a real one would be)
                    if (sentSum > 0.0f) {
                        const float ratio = (t.angle - turnedFrom) / sentSum;
                        ratioSum += ratio;
                        ratioWorst = std::fmax(ratioWorst, std::fabs(ratio - 1.0f));
                        ++windows;
                    }
                    sentSum = 0.0f, turnedFrom = t.angle, windowAt = now;
                }
            }
            naive = windows ? ratioWorst : 0.0f;
        }
        Report("t10", "the strength learned in steady input: a turret 1.6 times the row's learned within 10%, one 3 times held at twice",
               std::fabs(learned[0] - 1.6f) < 0.16f && std::fabs(learned[1] - 2.0f) < 0.05f, "learned %.2f (1.6) and %.2f (3, bound 2)", learned[0], learned[1]);
        Report("t11", "the design's first learner (the last half second's turn against what was sent, the coast counted), shown wrong window by window",
               naive > 0.2f, "its windows read the strength off by up to %.0f%%", naive * 100.0f);
    }
    // t12: the design's first gain (a third of what closes the gap in one tick, no stopping point), shown failing on a turret with momentum.
    {
        Turret t;
        t.per = 0.15f;
        t.delayTicks = 1;
        t.most = 150.0f;
        const Way w = WayOf(t);
        AxisState s;
        MouseCarry carry;
        const float dt = 1.0f / 72.0f;
        float over = 0.0f;
        for (int f = 0; f < 72 * 5; ++f) {
            const double now = f * dt;
            const float game = t.Camera(now);
            // u = K e with K a third of a tick's closure, cut to the most; the target's rate is 0 on a step.
            const float rate = std::fmax(-w.rows.most, std::fmin(w.rows.most, (20.0f - game) / (3.0f * kTick)));
            t.Frame(now + dt, 0.0f, MouseCounts(carry, rate, dt, w.degreesPerCount, static_cast<int>(w.countsPerSecond * dt)));
            over = std::fmax(over, t.angle - 20.0f);
        }
        (void)s;
        const Run law = Steer(t, w, StepTo(20.0f), Always(), 5.0f, 72.0f, false);
        Report("t12", "the design's first gain (a third of a tick's closure, no braking for the momentum), shown failing: it overshoots; the law doesn't",
               over > 1.0f && law.overshoot < 0.3f, "the first gain %.1f degrees over, the law %.2f", over, law.overshoot);
    }
    // t13: the frame rate: 90 and 120 frames a second alike.
    {
        bool ok = true;
        float worst = 0.0f;
        for (float fps : {90.0f, 120.0f, 45.0f}) {
            Turret t;
            t.most = 150.0f;
            const Way w = WayOf(t);
            const Run r = Steer(t, w, StepTo(40.0f), Always(), 5.0f, fps, true);
            ok = ok && r.overshoot < 0.3f && r.settle > 0.0f;
            worst = std::fmax(worst, r.overshoot);
        }
        Report("t13", "45, 90 and 120 frames a second alike", ok, "worst overshoot %.3f", worst);
    }
    // t14: the yaw across the circle's seam: a target at 179 from -179 goes the short way.
    {
        Turret t;
        t.most = 150.0f;
        t.angle = -179.0f;
        const Way w = WayOf(t);
        const Run r = Steer(t, w, [](double) { return 179.0f; }, Always(), 3.0f, 72.0f, true);
        Report("t14", "the yaw across the circle's seam goes the short way (2 degrees, not 358)", r.excursion < 3.0f && r.mostRate < 100.0f,
               "it went %.1f degrees", r.excursion);
    }
    // t15: inputs not numbers: nothing sent, nothing broken after.
    {
        AxisState s;
        AxisRows rows;
        rows.most = 100.0f, rows.lag = 0.1f, rows.delay = 0.05f, rows.least = 0.0f, rows.sign = 1.0f;
        const AxisOut a = SteerAxis(s, rows, NAN, 0.0f, 0.0, true, true);
        const AxisOut b = SteerAxis(s, rows, 10.0f, INFINITY, 0.014, true, true);
        const AxisOut c = SteerAxis(s, rows, 10.0f, 0.0f, NAN, true, true);
        const AxisOut d = SteerAxis(s, rows, 10.0f, 0.0f, 0.03, true, true);
        Report("t15", "inputs not numbers: nothing sent that frame, steering after", !a.sent && !b.sent && !c.sent && d.sent && std::isfinite(d.rate),
               "%d %d %d, then %.1f", a.sent, b.sent, c.sent, d.rate);
    }
    // t16: the pad's table both ways, the mouse's counts with their carry.
    {
        Turret t;
        t.pad = true;
        const Way w = WayOf(t);
        float worst = 0.0f;
        for (float rate = 1.0f; rate < PadMost(w.table); rate += 0.5f)
            worst = std::fmax(worst, std::fabs(PadRate(w.table, PadStick(w.table, rate)) - rate));
        const bool ends = PadStick(w.table, 0.0f) == 0.0f && PadStick(w.table, 1e6f) == 1.0f && PadStick(w.table, -1e6f) == -1.0f &&
                          PadStick(w.table, 1e-4f) >= w.table.stick[0] - 1e-6f;
        MouseCarry carry;
        int sum = 0;
        for (int f = 0; f < 1000; ++f)
            sum += MouseCounts(carry, 33.3f, 1.0f / 72.0f, 0.05f, 100);
        const float wanted = 33.3f * 1000.0f / 72.0f / 0.05f;
        MouseCarry capped;
        const int most = MouseCounts(capped, 1e6f, 1.0f / 72.0f, 0.05f, 7);
        Report("t16", "the pad's table read back and forth; the mouse's counts carried (no drift) and cut at their most",
               worst < 1e-3f && ends && std::fabs(sum - wanted) <= 1.0f && most == 7 && std::fabs(capped.carry) <= 1.0f,
               "the table %.4f off at most; %d counts for %.1f; capped %d", worst, sum, wanted, most);
    }
    // The second review's probes, each case its own run of one axis against the simulated turret, the law given a read of its own (read(now, real)).
    struct Probe {
        SteerStop stop = SteerStop::kNone;
        double stopAt = -1.0;
        float travel = 0.0f;     // the real turret's furthest from its start
        float final = 0.0f;
        int sentAfterStop = 0;   // frames sent anything after the stop
        int sentLastSecond = 0;  // frames sent anything in the run's last second
        int judged = 0;
    };
    const auto probe = [](Turret t, const Way& w, float goal, auto read, float seconds, bool wrap, const SteerRules& rules) {
        Probe p;
        AxisState s;
        MouseCarry carry;
        const float dt = 1.0f / 72.0f, from = t.angle;
        for (int f = 0; f < static_cast<int>(seconds * 72.0f); ++f) {
            const double now = f * dt;
            const AxisOut o = SteerAxis(s, w.rows, goal, read(now, t.Camera(now)), now, wrap, true, rules);
            float stick = 0.0f;
            int counts = 0;
            if (t.pad)
                stick = PadStick(w.table, o.ask);
            else
                counts = MouseCounts(carry, o.ask, dt, w.degreesPerCount, static_cast<int>(w.countsPerSecond * dt));
            const bool sent = stick != 0.0f || counts != 0;
            if (p.stopAt >= 0.0 && sent)
                ++p.sentAfterStop;
            if (now >= seconds - 1.0 && sent)
                ++p.sentLastSecond;
            if (s.stop != SteerStop::kNone && p.stopAt < 0.0)
                p.stopAt = now, p.stop = s.stop;
            t.Frame(now + dt, stick, counts);
            p.travel = std::fmax(p.travel, std::fabs(t.angle - from));
            p.final = t.angle;
        }
        p.judged = s.judged;
        return p;
    };
    const auto realRead = [](double, float real) { return real; };
    // t17: a read that is fresh but doesn't follow the turret (a copy of another camera, one stopped updating): stopped whatever the error's size,
    // nothing sent after; before, under a hard push nothing was judged (the review's probe: 0.5 degrees ran the mouse's turret 28 in 20 s, 13 degrees 723).
    {
        bool ok = true;
        float worstTravel = 0.0f;
        double latest = 0.0;
        char worst[160] = "";
        for (int pad = 0; pad < 2; ++pad)
            for (float e : {0.25f, 0.5f, 1.0f, 3.0f, 5.0f, 13.0f, 40.0f}) {
                Turret t;
                t.pad = pad != 0;
                t.most = pad ? 300.0f : 150.0f;
                const Way w = WayOf(t);
                const Probe p = probe(t, w, e, [](double, float) { return 0.0f; }, 20.0f, true, SteerRules{});
                const bool good = p.stop == SteerStop::kNoAnswer && p.stopAt < 2.5 && p.sentAfterStop == 0;
                ok = ok && good;
                if (p.travel > worstTravel || !good) {
                    worstTravel = std::fmax(worstTravel, p.travel);
                    std::snprintf(worst, sizeof(worst), "%s error %.2f: %s at %.2f s, the real turret %.1f degrees", pad ? "pad" : "mouse", e,
                                  p.stop == SteerStop::kNoAnswer ? "no answer" : p.stop == SteerStop::kWrongWay ? "WRONG WAY" : "NOT STOPPED", p.stopAt, p.travel);
                }
                latest = std::fmax(latest, p.stopAt);
            }
        Report("t17", "a read that doesn't follow the turret (0.25 to 40 degrees off, both inputs): stopped as no answer within 2.5 s, nothing sent after", ok,
               "the latest stop at %.2f s; the real turret's most travel before it %.1f degrees (%s)", latest, worstTravel, worst);
    }
    // t18: the game's invert on (the turret's sign flipped) and a small first move, the pitch limited to 20 each way (the truck's) and the yaw free,
    // both inputs: stopped as a wrong way well before the stop, nothing sent after (before: 0.5 to 2 degrees ran the mouse's pitch into -20 and held it
    // there, sending every frame; the yaw went 29 to 32 degrees the wrong way, the pad's 6.5 to 14).
    {
        bool ok = true;
        float worstTravel = 0.0f;
        char worst[200] = "";
        for (int pad = 0; pad < 2; ++pad)
            for (int limited = 0; limited < 2; ++limited)
                for (float e : {0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 15.0f}) {
                    Turret t;
                    t.pad = pad != 0;
                    t.most = pad ? 300.0f : 150.0f;
                    t.sign = -1.0f;
                    t.limited = limited != 0;
                    const Way w = WayOf(t);
                    const Probe p = probe(t, w, e, realRead, 10.0f, !t.limited, SteerRules{});
                    const bool good = p.stop == SteerStop::kWrongWay && p.travel < 10.0f && p.sentAfterStop == 0 && p.sentLastSecond == 0;
                    ok = ok && good;
                    if (p.travel > worstTravel || !good) {
                        worstTravel = std::fmax(worstTravel, p.travel);
                        std::snprintf(worst, sizeof(worst), "%s %s target %.1f: %s at %.2f s, %.1f degrees the wrong way", pad ? "pad" : "mouse",
                                      limited ? "pitch" : "yaw", e, p.stop == SteerStop::kWrongWay ? "wrong way" : p.stop == SteerStop::kNoAnswer ? "NO ANSWER" : "NOT STOPPED",
                                      p.stopAt, p.travel);
                    }
                }
        Report("t18", "the game's invert on, a small first move (0.5 to 15 degrees; the pitch at 20 each way, the yaw free; both inputs): a wrong way, stopped under 10 degrees",
               ok, "the most the wrong way %.1f degrees (%s)", worstTravel, worst);
    }
    // t19: the pitch's stop met before its row's limit (the hands' target at the row's limit, the real stop 1, 3 or 6 degrees short of it: a row a little
    // off, the truck on a slope with its pitch held in the world): the stop found where the turret stands, never a fault, the target held inside it and
    // nothing more pushed into it; both ways, pushed down too. 15 short (past the 10 degrees a stop may be found in): no answer. Pinned at the other
    // stop by a wrong sign: stopped (t18).
    {
        bool found = true;
        std::string said;
        for (float shortBy : {1.0f, 3.0f, 6.0f})
            for (float side : {1.0f, -1.0f}) {
                Turret t;
                t.most = 150.0f;
                t.limited = true, t.lo = -20.0f, t.hi = 20.0f;
                Way w = WayOf(t);
                w.rows.lo = -20.0f - shortBy, w.rows.hi = 20.0f + shortBy;
                const Probe p = probe(t, w, side * (20.0f + shortBy), realRead, 8.0f, false, SteerRules{});
                found = found && p.stop == SteerStop::kNone && std::fabs(p.final - side * 20.0f) < 0.1f && p.sentLastSecond == 0;
                char line[80];
                std::snprintf(line, sizeof(line), "%s%+.0f past: %s at %.1f, %d frames sent in the last second", said.empty() ? "" : "; ", side * shortBy,
                              p.stop == SteerStop::kNone ? "held" : "STOPPED", p.final, p.sentLastSecond);
                said += line;
            }
        Turret t;
        t.most = 150.0f;
        t.limited = true, t.lo = -20.0f, t.hi = 20.0f;
        Way offRow = WayOf(t);
        offRow.rows.lo = -35.0f, offRow.rows.hi = 35.0f;
        const Probe wrong = probe(t, offRow, 35.0f, realRead, 8.0f, false, SteerRules{});
        Report("t19", "the pitch's stop 1, 3 or 6 degrees short of its row, either way: the stop found, held there, nothing pushed into it after; 15 short: no answer",
               found && wrong.stop == SteerStop::kNoAnswer && wrong.sentAfterStop == 0, "%s; 15 past: %s at %.2f s", said.c_str(),
               wrong.stop == SteerStop::kNoAnswer ? "no answer" : "NOT STOPPED", wrong.stopAt);
    }
    // t20: a game not reading the input with a small error (0.5 and 2 degrees): stopped as no answer (t9's was 30 degrees).
    {
        bool ok = true;
        double latest = 0.0;
        for (int pad = 0; pad < 2; ++pad)
            for (float e : {0.5f, 2.0f}) {
                Turret t;
                t.pad = pad != 0;
                t.most = pad ? 300.0f : 150.0f;
                t.deaf = true;
                const Probe p = probe(t, WayOf(t), e, realRead, 6.0f, true, SteerRules{});
                ok = ok && p.stop == SteerStop::kNoAnswer && p.sentAfterStop == 0 && p.stopAt < 2.5;
                latest = std::fmax(latest, p.stopAt);
            }
        Report("t20", "a game not reading the input, a small error (0.5 and 2 degrees, both inputs): stopped as no answer", ok, "the latest at %.2f s", latest);
    }
    // t21: no false stop: the hand going back and forth (20 degrees, 0.5 and 1 Hz) with the rows off (the delay a tick either way; the lag a third to
    // three times; the strength half to twice), a quick 60-degree turn then still, and steps with a coast, both inputs: the guards never trip.
    {
        int runs = 0, falseStops = 0;
        char first[200] = "";
        const auto note = [&](const Probe& p, const char* what) {
            ++runs;
            if (p.stop != SteerStop::kNone) {
                if (!falseStops)
                    std::snprintf(first, sizeof(first), "%s: %s at %.2f s", what, SteerStopName(p.stop), p.stopAt);
                ++falseStops;
            }
        };
        for (int pad = 0; pad < 2; ++pad) {
            for (int delay : {1, 2, 3})
                for (float off : {-1.0f, 0.0f, 1.0f})
                    for (float hz : {0.5f, 1.0f}) {
                        Turret t;
                        t.pad = pad != 0;
                        t.most = pad ? 300.0f : 150.0f;
                        t.delayTicks = delay;
                        Way w = WayOf(t);
                        w.rows.delay += off * kTick;
                        if (w.rows.delay <= 0.0f)
                            continue;
                        AxisState s;
                        MouseCarry carry;
                        Probe p;
                        for (int f = 0; f < 72 * 10; ++f) {
                            const double now = f / 72.0;
                            const float goal = 20.0f * static_cast<float>(std::sin(2.0 * kPi * hz * now));
                            const AxisOut o = SteerAxis(s, w.rows, goal, t.Camera(now), now, true, true);
                            const float stick = t.pad ? PadStick(w.table, o.ask) : 0.0f;
                            const int counts = t.pad ? 0 : MouseCounts(carry, o.ask, 1.0f / 72.0f, w.degreesPerCount, static_cast<int>(w.countsPerSecond / 72.0f));
                            t.Frame(now + 1.0 / 72.0, stick, counts);
                            if (s.stop != SteerStop::kNone && p.stopAt < 0.0)
                                p.stop = s.stop, p.stopAt = now;
                        }
                        char what[120];
                        std::snprintf(what, sizeof(what), "%s delay %d ticks, row %+.0f, %.1f Hz", pad ? "pad" : "mouse", delay, off, hz);
                        note(p, what);
                    }
            for (float sl : {0.33f, 0.5f, 2.0f, 3.0f})
                for (float sm : {0.5f, 1.0f, 2.0f}) {
                    Turret proto;
                    proto.pad = pad != 0;
                    proto.most = pad ? 300.0f : 150.0f;
                    Turret t = proto;
                    const Way w = WayOf(t, sm, sl);
                    AxisState s;
                    MouseCarry carry;
                    Probe p;
                    for (int f = 0; f < 72 * 10; ++f) {
                        const double now = f / 72.0;
                        const float goal = 20.0f * static_cast<float>(std::sin(2.0 * kPi * now));
                        const AxisOut o = SteerAxis(s, w.rows, goal, t.Camera(now), now, true, true);
                        const float stick = t.pad ? PadStick(w.table, o.ask) : 0.0f;
                        const int counts = t.pad ? 0 : MouseCounts(carry, o.ask, 1.0f / 72.0f, w.degreesPerCount, static_cast<int>(w.countsPerSecond / 72.0f));
                        t.Frame(now + 1.0 / 72.0, stick, counts);
                        if (s.stop != SteerStop::kNone && p.stopAt < 0.0)
                            p.stop = s.stop, p.stopAt = now;
                    }
                    char what[120];
                    std::snprintf(what, sizeof(what), "%s lag x%.2f strength x%.1f, 1 Hz", pad ? "pad" : "mouse", sl, sm);
                    note(p, what);
                    // (the same rows, steps of 3 to 120 degrees there and back: a coast against the push at every turn)
                    for (float step : {3.0f, 30.0f, 120.0f}) {
                        AxisState s2;
                        MouseCarry c2;
                        Turret t2 = proto;  // (a fresh turret: its clock from 0)
                        Probe p2;
                        for (int f = 0; f < 72 * 8; ++f) {
                            const double now = f / 72.0;
                            const float goal = now < 2.0 ? step : now < 4.0 ? -step : now < 6.0 ? step * 0.5f : 0.0f;
                            const AxisOut o = SteerAxis(s2, w.rows, goal, t2.Camera(now), now, true, true);
                            const float stick = t2.pad ? PadStick(w.table, o.ask) : 0.0f;
                            const int counts = t2.pad ? 0 : MouseCounts(c2, o.ask, 1.0f / 72.0f, w.degreesPerCount, static_cast<int>(w.countsPerSecond / 72.0f));
                            t2.Frame(now + 1.0 / 72.0, stick, counts);
                            if (s2.stop != SteerStop::kNone && p2.stopAt < 0.0)
                                p2.stop = s2.stop, p2.stopAt = now;
                        }
                        std::snprintf(what, sizeof(what), "%s lag x%.2f strength x%.1f, steps of %.0f", pad ? "pad" : "mouse", sl, sm, step);
                        note(p2, what);
                    }
                }
            {
                Turret t;
                t.pad = pad != 0;
                t.most = pad ? 300.0f : 150.0f;
                AxisState s;
                MouseCarry carry;
                const Way w = WayOf(t);
                Probe p;
                for (int f = 0; f < 72 * 6; ++f) {
                    const double now = f / 72.0;
                    const float goal = now < 0.25 ? 240.0f * static_cast<float>(now) : 60.0f;
                    const AxisOut o = SteerAxis(s, w.rows, goal, t.Camera(now), now, true, true);
                    const float stick = t.pad ? PadStick(w.table, o.ask) : 0.0f;
                    const int counts = t.pad ? 0 : MouseCounts(carry, o.ask, 1.0f / 72.0f, w.degreesPerCount, static_cast<int>(w.countsPerSecond / 72.0f));
                    t.Frame(now + 1.0 / 72.0, stick, counts);
                    if (s.stop != SteerStop::kNone && p.stopAt < 0.0)
                        p.stop = s.stop, p.stopAt = now;
                }
                note(p, pad ? "pad 60 degrees in 0.25 s" : "mouse 60 degrees in 0.25 s");
            }
        }
        Report("t21", "no false stop: a hand back and forth with the rows off (the delay a tick, the lag x0.33 to x3, the strength x0.5 to x2), steps there and back, a quick turn",
               falseStops == 0, "%d of %d runs stopped%s%s", falseStops, runs, falseStops ? "; the first: " : "", first);
    }
}

// ------------------------------------------------------------------ e: end to end ------------------------------------------------------------------

void EndToEnd() {
    // The truck: both grips taken, swept 25 degrees each way and up and down over 4 s, then held: the hands' gun through the smoothing and the steering
    // into two simulated axes (the mouse), the game's gun read off its camera. It follows within a few degrees and settles where the hands stop.
    Scene sc;
    sc.g = TestGeometry(0);
    sc.body = 0.0f;
    Turret yawT, pitchT;
    yawT.most = 150.0f, pitchT.most = 90.0f;
    pitchT.limited = true, pitchT.lo = -20.0f, pitchT.hi = 20.0f;
    const Way wy = WayOf(yawT), wp = WayOf(pitchT);
    AxisState sy, sp;
    MouseCarry cy, cp;
    Smooth fy, fp;
    Holds h;
    HandsNow n;
    const V3 off0 = {0.02f, -0.1f, -0.15f}, off1 = {-0.03f, -0.1f, -0.12f};
    const float dt = 1.0f / 72.0f;
    float followMost = 0.0f, endOver = 0.0f, endError = 0.0f;
    float yaw = 0.0f, pitch = 0.0f;
    for (int f = 0; f < 72 * 7; ++f) {
        const double now = f * dt;
        const float gy = yawT.Camera(now) * kDeg, gp = pitchT.Camera(now) * kDeg;
        // Where the hands are: the handles at the swept gun plus their offsets (turned with it), as a player's would be.
        const float sweep = now < 4.0 ? static_cast<float>(std::sin(now * kPi / 2.0)) : 0.0f;
        const float wantYaw = 25.0f * kDeg * sweep, wantPitch = 8.0f * kDeg * static_cast<float>(now < 4.0 ? std::sin(now * kPi) : 0.0);
        n.at[0] = HandsAt(sc, 0, wantYaw, wantPitch, Turn(wantYaw, off0));
        n.at[1] = HandsAt(sc, 1, wantYaw, wantPitch, Turn(wantYaw, off1));
        n.tracked[0] = n.tracked[1] = n.squeezed[0] = n.squeezed[1] = true;
        if (f == 0) {  // (taken with the gun where the game has it: the hands there too)
            n.at[0] = HandsAt(sc, 0, gy, gp, off0);
            n.at[1] = HandsAt(sc, 1, gy, gp, off1);
        }
        HoldStep(h, n, sc.g, sc.ring, gy, gp, sc.body);
        Solve(sc.g, sc.ring, h, n.at, yaw, pitch);
        const float ty = SmoothStep(fy, yaw, dt, true), tp = SmoothStep(fp, pitch, dt, false);
        const AxisOut oy = SteerAxis(sy, wy.rows, ty / kDeg, yawT.Camera(now), now, true, true);
        const AxisOut op = SteerAxis(sp, wp.rows, tp / kDeg, pitchT.Camera(now), now, false, true);
        yawT.Frame(now + dt, 0.0f, MouseCounts(cy, oy.ask, dt, wy.degreesPerCount, static_cast<int>(wy.countsPerSecond * dt)));
        pitchT.Frame(now + dt, 0.0f, MouseCounts(cp, op.ask, dt, wp.degreesPerCount, static_cast<int>(wp.countsPerSecond * dt)));
        if (now > 0.5 && now < 4.0)
            followMost = std::fmax(followMost, std::fabs(yawT.angle - Deg(wantYaw)) + std::fabs(pitchT.angle - Deg(wantPitch)));
        if (now >= 4.0) {
            endOver = std::fmax(endOver, std::fmax(std::fabs(yawT.angle), std::fabs(pitchT.angle)) * (now > 6.0 ? 1.0f : 0.0f));
            endError = std::fabs(yawT.angle) + std::fabs(pitchT.angle);
        }
    }
    Report("e1", "end to end on the truck: both grips swept 25 degrees each way and 8 up and down; the game's gun follows and settles where the hands stop",
           followMost < 10.0f && endError < 0.4f, "following within %.1f degrees (a turret's momentum and delay behind a 39-degree-a-second sweep); at the end %.2f off",
           followMost, endError);
}

// ------------------------------------------------------------------ f: fire ------------------------------------------------------------------

void FireRule() {
    Fire f;
    const auto step = [&](float l, float r, bool hl, bool hr) {
        const float t[2] = {l, r};
        const bool h[2] = {hl, hr};
        return FireStep(f, t, h);
    };
    // Either trigger with its grip; a trigger without its grip nothing; the hysteresis; a pulled trigger as its grip closes waits for a let-go.
    const bool a = step(0.9f, 0.0f, true, false);        // the left fires (owner's call 3)
    const bool b = step(0.0f, 0.0f, true, false);        // let go
    const bool c = step(0.0f, 0.9f, false, true);        // the right fires
    const bool d = step(0.0f, 0.9f, false, false);       // its grip let go: nothing
    const bool e = step(0.9f, 0.0f, false, false);       // a trigger with no grip: nothing
    Fire g;
    const auto stepg = [&](float l, float r, bool hl, bool hr) {
        const float t[2] = {l, r};
        const bool h[2] = {hl, hr};
        return FireStep(g, t, h);
    };
    const bool h1 = stepg(0.9f, 0.0f, false, false);  // pulled first
    const bool h2 = stepg(0.9f, 0.0f, true, false);   // the grip closes on it: held off
    const bool h3 = stepg(0.2f, 0.0f, true, false);   // let go
    const bool h4 = stepg(0.9f, 0.0f, true, false);   // pulled again: fires
    Fire k;
    const auto stepk = [&](float l) {
        const float t[2] = {l, 0.0f};
        const bool h[2] = {true, false};
        return FireStep(k, t, h);
    };
    const bool k1 = stepk(0.65f), k2 = stepk(0.5f), k3 = stepk(0.35f), k4 = stepk(0.55f);
    Fire both;
    const float t2[2] = {0.9f, 0.9f};
    const bool h2b[2] = {true, true};
    const bool b1 = FireStep(both, t2, h2b);
    Report("f1", "either trigger fires while its grip holds a handle (the owner's call 3); without its grip nothing", a && !b && c && !d && !e,
           "%d %d %d %d %d", a, b, c, d, e);
    Report("f2", "a trigger pulled as its grip closes waits to be let go (no burst by closing a hand on it)", !h1 && !h2 && !h3 && h4, "%d %d %d %d", h1, h2, h3, h4);
    Report("f3", "0.6 to press, under 0.4 to let go; both triggers one button", k1 && k2 && !k3 && !k4 && b1, "%d %d %d %d, both %d", k1, k2, k3, k4, b1);
}

// ------------------------------------------------------------------ g: the seat's reader ------------------------------------------------------------------

struct Image {
    std::map<uint32_t, std::vector<uint8_t>> pages;
    mutable int reads = 0;
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
    void Text(uint32_t at, const char* s) { Put(at, s, std::strlen(s) + 1); }
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
    // A type: its vtable's slot 0 a getType (mov eax, info; ret), the info's data's text the name.
    uint32_t Type(uint32_t vtable, uint32_t code, const char* name) {
        const uint32_t info = code + 0x100, data = code + 0x200, text = code + 0x300;
        W32(vtable, code);
        uint8_t getType[6] = {0xB8, 0, 0, 0, 0, 0xC3};
        std::memcpy(getType + 1, &info, 4);
        Put(code, getType, 6);
        W32(info, data);
        W32(data, text);
        Text(text, name);
        return vtable;
    }
};

constexpr uint32_t kBase = 0x400000;

// A made-up layout (every row measured, made-up places) and an image laid out by it.
seatread::Layout MadeLayout() {
    seatread::Layout l = seatread::kLayout;
    l.attached = {"attached", "test", true, 0x3E0};
    l.entry = {"entry", "test", true, 0x3E4};
    l.vehicleType = {"vehicle type", "test", true, "ClientVehicleEntity"};
    l.data = {"data", "test", true, 0x30};
    l.dataType = {"data type", "test", true, "VehicleEntityData"};
    l.name = {"name", "test", true, 0x10};
    l.nameIsPointer = true;
    l.code[0] = {"reads attached", "test", true, 0x123450, "8B 81 E0 03 00 00 C3"};
    l.code[1] = {"reads entry", "test", true, 0x123460, "8B 81 E4 03 00 00 C3"};
    l.codeCount = 2;
    return l;
}
constexpr uint32_t kPlayer = 0x20000000, kVehicle = 0x21000000, kData = 0x22000000, kSoldier = 0x23000000;
Image MadeImage(int entry, const char* name, bool vehicle) {
    Image m;
    m.W32(kBase + seatread::kClientGameContext, 0x1F000000);
    m.W32(0x1F000000 + seatread::kPlayerManager, 0x1F100000);
    m.W32(0x1F100000 + seatread::kLocalPlayer, kPlayer);
    m.W32(kPlayer + seatread::kPlayerSoldier, kSoldier);
    m.Type(0x00600000, 0x00610000, "ClientSoldierEntity");
    m.W32(kSoldier, 0x00600000);
    m.Type(0x00620000, 0x00630000, "ClientVehicleEntity");
    m.Type(0x00640000, 0x00650000, "VehicleEntityData");
    m.W32(kVehicle, 0x00620000);
    m.W32(kVehicle + 0x30, kData);
    m.W32(kData, 0x00640000);
    m.W32(kData + 0x10, kData + 0x3000);
    m.Text(kData + 0x3000, name);
    m.W32(kPlayer + 0x3E0, vehicle ? kVehicle : 0);
    m.W32(kPlayer + 0x3E4, static_cast<uint32_t>(entry));
    const uint8_t a[] = {0x8B, 0x81, 0xE0, 0x03, 0x00, 0x00, 0xC3}, b[] = {0x8B, 0x81, 0xE4, 0x03, 0x00, 0x00, 0xC3};
    m.Put(kBase + 0x123450, a, sizeof(a));
    m.Put(kBase + 0x123460, b, sizeof(b));
    return m;
}

void Reader() {
    {
        const Image m = MadeImage(1, "TechnicalTruck_Restricted", true);
        const seatread::Proofs p = seatread::Prove(m, kBase, seatread::kLayout);
        Report("g1", "the shipped layout is refused before any read of the game (unmeasured), its first row named", !p.ok && p.unmeasured && p.failed && m.reads == 0,
               "%s; reads %d", p.failed ? p.failed : "?", m.reads);
    }
    const seatread::Layout l = MadeLayout();
    {
        const Image m = MadeImage(1, "TechnicalTruck_Restricted", true);
        const seatread::Proofs p = seatread::Prove(m, kBase, l);
        seatread::TypeCache cache;
        const seatread::Seat s = seatread::Read(m, kBase, l, cache);
        SeatRead r;
        r.known = s.ok, r.vehicle = s.vehicle, r.entry = s.entry, r.name = s.name;
        const Gun tested[2] = {TestGun(0), TestGun(1)};
        Report("g2", "a made-up layout proved and read: the truck's gunner seat, told for the truck's gun", p.ok && s.ok && s.vehicle && s.entry == 1 &&
                                                                                                        std::strcmp(s.name, "TechnicalTruck_Restricted") == 0 &&
                                                                                                        MatchGun(tested, 2, r) == 0,
               "entry %d, %s, gun %d", s.entry, s.name, MatchGun(tested, 2, r));
    }
    {
        const Image m = MadeImage(0, "TechnicalTruck_Restricted", false);
        seatread::TypeCache cache;
        const seatread::Seat s = seatread::Read(m, kBase, l, cache);
        Report("g3", "on foot (attached to nothing): read, no vehicle", s.ok && !s.vehicle && s.miss == seatread::Miss::kNone, "%s", seatread::MissName(s.miss));
    }
    {
        Image m = MadeImage(0, "TechnicalTruck_Restricted", true);
        seatread::TypeCache cache;
        const seatread::Seat passenger = seatread::Read(m, kBase, l, cache);
        m.W32(kPlayer + 0x3E0, kSoldier);  // attached to the soldier itself
        const seatread::Seat self = seatread::Read(m, kBase, l, cache);
        const Gun tested[2] = {TestGun(0), TestGun(1)};
        SeatRead r;
        r.known = passenger.ok, r.vehicle = passenger.vehicle, r.entry = passenger.entry, r.name = passenger.name;
        Report("g4", "a passenger's entry is read as a vehicle's seat and told for no gun; attached to the soldier: no vehicle",
               passenger.ok && passenger.vehicle && passenger.entry == 0 && MatchGun(tested, 2, r) < 0 && self.ok && !self.vehicle, "passenger entry %d gun %d; self %s",
               passenger.entry, MatchGun(tested, 2, r), self.vehicle ? "a vehicle" : "no vehicle");
    }
    {
        // Each wrong thing alone: the entry out of range, the data's type, its name not a name, the name's memory gone, the local player gone.
        const auto miss = [&](void (*spoil)(Image&)) {
            Image m = MadeImage(1, "TechnicalTruck_Restricted", true);
            spoil(m);
            seatread::TypeCache cache;
            const seatread::Seat s = seatread::Read(m, kBase, l, cache);
            return std::make_pair(s.ok && s.vehicle, s.miss);
        };
        const auto e = miss([](Image& m) { m.W32(kPlayer + 0x3E4, 0xFFFFFFFFu); });
        const auto d = miss([](Image& m) { m.W32(kData, 0x00620000); });
        const auto n = miss([](Image& m) { m.Text(kData + 0x3000, "\x01\x02 not a name"); });
        const auto g = miss([](Image& m) { m.Drop(kData + 0x3000); });
        const auto p = miss([](Image& m) { m.W32(0x1F100000 + seatread::kLocalPlayer, 0); });
        const bool ok = !e.first && e.second == seatread::Miss::kEntry && !d.first && d.second == seatread::Miss::kData && !n.first &&
                        n.second == seatread::Miss::kName && !g.first && g.second == seatread::Miss::kName && !p.first && p.second == seatread::Miss::kNoPlayer;
        Report("g5", "each wrong thing alone is a miss, named, never a seat: the entry, the data's type, the name, its memory, the player", ok,
               "%s; %s; %s; %s; %s", seatread::MissName(e.second), seatread::MissName(d.second), seatread::MissName(n.second), seatread::MissName(g.second),
               seatread::MissName(p.second));
    }
    {
        // The proofs: each code row's bytes changed alone is refused, named; the vtable's type name read once (the cache).
        Image m = MadeImage(1, "TechnicalTruck_Restricted", true);
        const uint8_t wrong = 0x90;
        m.Put(kBase + 0x123452, &wrong, 1);
        const seatread::Proofs p = seatread::Prove(m, kBase, l);
        Image m2 = MadeImage(1, "TechnicalTruck_Restricted", true);
        seatread::TypeCache cache;
        seatread::Read(m2, kBase, l, cache);
        const int first = m2.reads;
        m2.reads = 0;
        seatread::Read(m2, kBase, l, cache);
        Report("g6", "the proofs: a code row's bytes not as the layout has them refused, named, not as unmeasured; the type names cached after the first read",
               !p.ok && !p.unmeasured && p.failed && std::strcmp(p.failed, "reads attached") == 0 && m2.reads < first,
               "%s at %08X; reads %d then %d", p.failed ? p.failed : "?", p.at, first, m2.reads);
    }
}

}  // namespace

int main() {
    const auto start = std::chrono::steady_clock::now();
    Rows();
    States();
    QuietList();
    HandleLaw();
    Orbit();
    Smoothing();
    Steering();
    EndToEnd();
    FireRule();
    Reader();
    // The time a frame's work takes: both axes' steering and a two-hand solve, 100000 times.
    {
        Scene sc;
        sc.g = TestGeometry(0);
        Holds h;
        HandsNow n;
        n.at[0] = HandsAt(sc, 0, 0.0f, 0.0f, {});
        n.at[1] = HandsAt(sc, 1, 0.0f, 0.0f, {});
        n.tracked[0] = n.tracked[1] = n.squeezed[0] = n.squeezed[1] = true;
        AxisState sy, sp;
        AxisRows rows;
        rows.most = 100.0f, rows.lag = 0.1f, rows.delay = 0.05f, rows.least = 0.0f, rows.sign = 1.0f;
        float yaw = 0.0f, pitch = 0.0f, sink = 0.0f;
        const auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < 100000; ++i) {
            n.at[0].x += 1e-6f;
            HoldStep(h, n, sc.g, sc.ring, 0.0f, 0.0f, sc.body);
            Solve(sc.g, sc.ring, h, n.at, yaw, pitch);
            sink += SteerAxis(sy, rows, yaw / kDeg, 0.0f, i / 72.0, true, true).rate + SteerAxis(sp, rows, pitch / kDeg, 0.0f, i / 72.0, false, true).rate;
        }
        const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count() / 100000.0;
        Report("z1", "a frame's work (both axes' steering, the holds and a two-hand solve)", us < 50.0 && std::isfinite(sink), "%.2f us a frame", us);
    }
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    Report("z2", "the time", ms < 20000.0, "%.0f ms", ms);
    std::printf("%d checks, %d failed\n", g_counted, g_failed);
    return g_failed;
}

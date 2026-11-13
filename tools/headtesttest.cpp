// The head replay's motions and its meters' math (src/headtest.cpp, as xr_runtime.cpp runs them) with no game: the angles there and back and against the
// view turn's own rows (src/viewturn.h), the slide's split into yaw and pitch, a sine's fastest, a burst's shape, a track between its rows and round its loop,
// the eyes on their pivot, the predictor's carry and its error, the ease in and out, the level head, headtest.txt's lines, the frame's line read back, the noise,
// the time a frame takes; the rules a picture is named by (weapon.txt "nameby"), the world's turn against the view turn's own rows, and the two lines a second
// that run with a person in the headset (the world's turn, the edge). Its exit code is its number of failures. With "--synth <file>": writes a made-up run (a known lag, a known wobble of the camera's
// place) in the log's own lines, for tools\headfit.ps1 -SelfTest to get back. With "--track <file>": a track file as the replay reads and plays it (its rows, its
// loop, how fast the head turns playing it, how far the eyes swing). With "--sim [<headtest.txt lines; ...> ...] [--log <path and name>]": the pose filters
// (the fixed one, "poseadapt", "poserate") run on those motions with no game, 72 frames a second: how far each one's pictures slide off the head and how rough
// their poses are, by the head's speed; without a motion, on the user's own head of 2026-11-12 (headtracks\user_b1.txt and its located rows, b2, b3, beside
// this program). --log writes each run in the log's own lines (<name>_<filter>_<motion>.log) for tools\headfit.ps1 to judge. Its exit code is 0 when "poserate"
// holds what it must (no rougher than about twice the fixed filter, sliding no more than "poseadapt").
#include "../src/headtest.h"
#include "../src/viewturn.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {

using namespace headtest;

int g_checks = 0, g_failed = 0;

void Check(bool ok, const char* name, double got = 0.0, double want = 0.0) {
    ++g_checks;
    if (!ok) {
        ++g_failed;
        std::printf("FAIL %s (got %.6f, wanted %.6f)\n", name, got, want);
    }
}

void Near(double got, double want, double within, const char* name) {
    Check(std::fabs(got - want) <= within, name, got, want);
}

double Wrap(double degrees) {
    return std::remainder(degrees, 360.0);
}

// How fast the head turns at t (degrees a second), over a tenth of a millisecond either side.
double SpeedAt(const Config& c, const Pose& base, double t) {
    const double h = 0.0001;
    return Between(HeadAt(c, base, t - h).turn, HeadAt(c, base, t + h).turn) / (2.0 * h);
}

Pose Standing() {
    Pose p;
    p.place[1] = 1.6;
    return p;
}

void TestAngles() {
    for (double yaw = -170.0; yaw <= 170.0; yaw += 34.0)
        for (double pitch = -80.0; pitch <= 80.0; pitch += 20.0)
            for (double roll = -170.0; roll <= 170.0; roll += 68.0) {
                const Angles a = ToAngles(FromAngles({yaw, pitch, roll}));
                Near(a.yaw, yaw, 1e-9, "angles there and back: yaw");
                Near(a.pitch, pitch, 1e-9, "angles there and back: pitch");
                Near(a.roll, roll, 1e-9, "angles there and back: roll");
            }
    const double forward[3] = {0.0, 0.0, -1.0}, right[3] = {1.0, 0.0, 0.0};
    double v[3];
    Rotate(FromAngles({90.0, 0.0, 0.0}), forward, v);
    Near(v[0], -1.0, 1e-12, "a yaw of +90 looks to the left (-x)");
    Rotate(FromAngles({0.0, 30.0, 0.0}), forward, v);
    Near(v[1], 0.5, 1e-12, "a pitch of +30 looks up");
    Rotate(FromAngles({0.0, 0.0, 20.0}), right, v);
    Near(v[1], std::sin(20.0 * kPi / 180.0), 1e-12, "a roll of +20 lifts the head's right side (tilted to the left shoulder)");
    // Straight up: the yaw and the roll are one turn, and it comes back whole.
    const Turn up = FromAngles({40.0, 90.0, 0.0});
    Near(Between(up, FromAngles(ToAngles(up))), 0.0, 1e-6, "looking straight up comes back the same turn");
    Near(Between(FromAngles({10.0, 0.0, 0.0}), FromAngles({10.01, 0.0, 0.0})), 0.01, 1e-9, "a hundredth of a degree apart is told");
    Near(Between(FromAngles({0.0, 0.0, 0.0}), FromAngles({179.0, 0.0, 0.0})), 179.0, 1e-9, "179 degrees apart");
}

// The view turn's own rows (viewturn.h PureRows: the body's yaw as the game reads one, then the head) in this convention.
void TestRows() {
    for (double body = -150.0; body <= 180.0; body += 55.0) {
        camera::ViewTurn turn{};
        turn.head[3] = 1.0f;
        turn.lookYaw = static_cast<float>(body * kPi / 180.0);
        turn.currentYaw = 0.0f;
        float m[16] = {};
        viewturn::PureRows(m, turn);
        const float level[9] = {m[0], m[1], m[2], m[4], m[5], m[6], m[8], m[9], m[10]};
        const Angles a = RowsAngles(level);
        Near(Wrap(a.yaw - (180.0 - body)), 0.0, 1e-4, "a level camera's yaw is 180 less the game's own");
        Near(a.pitch, 0.0, 1e-4, "a level camera has no pitch");
        Near(a.roll, 0.0, 1e-4, "a level camera has no roll");
        Near(Wrap(std::atan2(m[8], -m[10]) * 180.0 / kPi - body), 0.0, 1e-4, "the game's own yaw of those rows is the body's");
        // The head on it: the rows are the body's turn times the head's, and their angles are that turn's.
        const Turn head = FromAngles({25.0, -10.0, 5.0});
        turn.head[0] = static_cast<float>(head.x), turn.head[1] = static_cast<float>(head.y), turn.head[2] = static_cast<float>(head.z), turn.head[3] = static_cast<float>(head.w);
        viewturn::PureRows(m, turn);
        const float rows[9] = {m[0], m[1], m[2], m[4], m[5], m[6], m[8], m[9], m[10]};
        const Turn want = Mul(FromAngles({180.0 - body, 0.0, 0.0}), head);
        Near(Between(RowsTurn(rows), want), 0.0, 2e-4, "the turned rows are the body's turn times the head's");
        const Angles b = RowsAngles(rows), w = ToAngles(want);
        Near(Wrap(b.yaw - w.yaw), 0.0, 2e-4, "the turned rows' yaw");
        Near(b.pitch, w.pitch, 2e-4, "the turned rows' pitch");
        Near(b.roll, w.roll, 2e-4, "the turned rows' roll");
        Near(b.pitch, -10.0, 2e-4, "the head's pitch is the camera's");
    }
}

void TestSlide() {
    const Turn picture = FromAngles({30.0, 0.0, 0.0});
    Angles s = Slide(picture, FromAngles({37.0, 0.0, 0.0}));
    Near(s.yaw, 7.0, 1e-9, "the head 7 degrees left of the picture: the slide's yaw +7");
    Near(s.pitch, 0.0, 1e-9, "and no pitch in it");
    s = Slide(picture, FromAngles({26.0, 0.0, 0.0}));
    Near(s.yaw, -4.0, 1e-9, "the head 4 degrees right of the picture: -4");
    // About the picture's own axes, whatever way it looks.
    const Turn tilted = FromAngles({-50.0, 20.0, 3.0});
    s = Slide(tilted, Mul(tilted, FromAngles({0.0, 4.0, 0.0})));
    Near(s.pitch, 4.0, 1e-9, "the head 4 degrees above a tilted picture: the slide's pitch +4");
    Near(s.yaw, 0.0, 1e-9, "and no yaw in it");
    s = Slide(tilted, Mul(tilted, FromAngles({-3.0, 2.0, 0.0})));
    Near(s.yaw, -3.0, 1e-9, "a slide both ways: its yaw");
    Near(s.pitch, 2.0, 1e-9, "a slide both ways: its pitch");
    Near(s.roll, 0.0, 1e-9, "a slide both ways: no roll");
    s = Slide(tilted, tilted);
    Near(std::fabs(s.yaw) + std::fabs(s.pitch) + std::fabs(s.roll), 0.0, 1e-9, "the head on the picture: no slide");
}

void TestShake() {
    Config c = Parse("shake yaw 35 2\nneck 0 0\nseconds 100");
    const Pose base = Standing();
    double fastest = 0.0;
    for (double t = 1.0; t < 1.5; t += 0.0005)
        fastest = std::max(fastest, SpeedAt(c, base, t));
    Near(fastest, 2.0 * kPi * 2.0 * 35.0, 0.5, "a sine's fastest is 2 pi x swings a second x degrees");
    Near(ToAngles(HeadAt(c, base, 1.125).turn).yaw, 35.0, 1e-6, "a sine's furthest left a quarter swing in");
    Near(ToAngles(HeadAt(c, base, 1.375).turn).yaw, -35.0, 1e-6, "and right three quarters in");
    c = Parse("shake pitch 20 1.5\nneck 0 0\nseconds 100");
    Near(ToAngles(HeadAt(c, base, 1.5).turn).pitch, 20.0, 1e-6, "a nod at its highest");
    Near(ToAngles(HeadAt(c, base, 1.5).turn).yaw, 0.0, 1e-9, "a nod has no yaw");
    c = Parse("shake both 30 10 2\nneck 0 0\nseconds 100");
    const Angles a = ToAngles(HeadAt(c, base, 1.125).turn);
    Near(a.yaw, 30.0, 1e-6, "both ways at once: the yaw");
    Near(a.pitch, 10.0, 1e-6, "both ways at once: the pitch");
}

void TestBurst() {
    Config c = Parse("burst yaw 70 110 300\nneck 0 0\nseconds 100");
    const Pose base = Standing();
    auto yaw = [&](double t) { return ToAngles(HeadAt(c, base, t).turn).yaw; };
    const double start = kEase, swing = 0.110, pause = 0.300, whole = 2.0 * (swing + pause);
    Near(yaw(start * 0.5), 0.0, 1e-9, "a burst waits for the ease-in");
    Near(yaw(start), 0.0, 1e-9, "a burst starts from the head as it was");
    Near(yaw(start + swing / 2.0), 35.0, 1e-6, "half way there at half the swing");
    Near(yaw(start + swing), 70.0, 1e-6, "there at the swing's end");
    Near(yaw(start + swing + pause / 2.0), 70.0, 1e-9, "it stays there for the pause");
    Near(yaw(start + swing + pause + swing / 2.0), 35.0, 1e-6, "half way back");
    Near(yaw(start + 2.0 * swing + pause + 0.001), 0.0, 1e-6, "back where it was");
    Near(yaw(start + whole + swing), 70.0, 1e-6, "and there again a round later");
    double fastest = 0.0;
    for (double t = start; t < start + whole; t += 0.0002)
        fastest = std::max(fastest, SpeedAt(c, base, t));
    Near(fastest, kPi / 2.0 * 70.0 / swing, 1.5, "a burst's fastest is pi/2 x degrees / swing");
    c = Parse("burst pitch -25 150 200\nneck 0 0\nseconds 100");
    Near(ToAngles(HeadAt(c, base, start + 0.150).turn).pitch, -25.0, 1e-6, "a burst of the pitch, downwards");
    Near(ToAngles(HeadAt(c, base, start + 0.150).turn).yaw, 0.0, 1e-9, "with no yaw");
}

void TestTrack() {
    Config c = Parse("track some.txt\nneck 0 0\nseconds 100");
    Check(c.kind == Kind::kTrack && c.trackFile == "some.txt", "a track line names its file");
    // (Rows on one straight line: the curve through them is that line.)
    Check(ParseTrack("# a comment\n1000 0 0\n1100 10 -5\n\n1300 30 -15\nrubbish\n1200 99 99\n", c), "a track's rows read");
    Check(c.track.size() == 3, "three good rows (one out of order and one not a row left out)", static_cast<double>(c.track.size()), 3.0);
    Near(c.trackMs, 300.0 + kEase * 1000.0, 1e-9, "its loop: its length and a quarter second back to the start");
    Offsets o = OffsetsAt(c, 0.050);
    Near(o.yaw, 5.0, 1e-9, "between two rows, half way: the yaw");
    Near(o.pitch, -2.5, 1e-9, "between two rows, half way: the pitch");
    o = OffsetsAt(c, 0.200);
    Near(o.yaw, 20.0, 1e-9, "between the next two");
    o = OffsetsAt(c, 0.300 + kEase / 2.0);
    Near(o.yaw, 15.0, 1e-9, "half way back from the last row to the first: the yaw");
    Near(o.pitch, -7.5, 1e-9, "half way back: the pitch");
    o = OffsetsAt(c, c.trackMs * 1e-3 + 0.050);
    Near(o.yaw, 5.0, 1e-9, "round the loop: the same place a round later");
    Check(!o.placed, "no place of its own without x y z");
    Check(ParseTrack("0 0 0 0 0 0 0\n100 20 0 4 0.1 0.02 -0.3\n", c), "a track with its own places");
    o = OffsetsAt(c, 0.050);
    Check(o.placed, "its own place wins");
    Near(o.x, 0.05, 1e-12, "its x half way");
    Near(o.z, -0.15, 1e-12, "its z half way");
    Near(o.roll, 2.0, 1e-12, "its roll half way");
    const Pose base = Standing();
    c.seconds = 100.0;
    const Pose p = HeadAt(c, base, c.trackMs * 1e-3 * 3.0 + 0.100);
    Near(p.place[0], 0.1, 1e-9, "the head goes where the track puts it (x)");
    Near(p.place[1], 1.62, 1e-9, "(y)");
    Near(p.place[2], -0.3, 1e-9, "(z)");
    // The short way round between rows either side of 180.
    Check(ParseTrack("0 170 0\n100 -170 0\n", c), "rows either side of 180");
    Near(std::fabs(Wrap(OffsetsAt(c, 0.050).yaw)), 180.0, 1e-9, "go the short way round");
    Check(!ParseTrack("0 1 2\n", c) && c.kind == Kind::kNone, "one row is no track: nothing replayed");
}

void TestNeck() {
    Config c = Parse("track t\nseconds 100");
    ParseTrack("0 90 0\n100000 90 0\n", c);
    const Pose base = Standing();
    Pose p = HeadAt(c, base, 10.0);
    Near(p.place[0], -0.10, 1e-9, "turned 90 left about a pivot 10 cm behind the eyes: 10 cm to the left");
    Near(p.place[1], 1.6, 1e-9, "at the same height");
    Near(p.place[2], 0.10, 1e-9, "and 10 cm back");
    ParseTrack("0 0 90\n100000 0 89\n", c);  // (89 at the far end: the rows are kept under straight up)
    p = HeadAt(c, base, 0.0001 + kEase * 4.0);
    Near(p.place[1], 1.6 - 0.08 + 0.10, 2e-3, "looking straight up, the eyes 10 cm above the pivot");
    Near(p.place[2], 0.10 + 0.08, 2e-3, "and 8 cm behind it");
    c = Parse("shake yaw 35 2\nneck 0.12 0.05\nseconds 100");
    double widest = 0.0;
    for (double t = 1.0; t < 1.5; t += 0.001) {
        p = HeadAt(c, base, t);
        const double d[3] = {p.place[0], p.place[1] - (1.6 - 0.05), p.place[2] - 0.12};
        Near(std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]), std::sqrt(0.12 * 0.12 + 0.05 * 0.05), 1e-9, "the eyes stay their own distance from the pivot");
        widest = std::max(widest, std::fabs(p.place[0]));
    }
    Near(widest, 0.12 * std::sin(35.0 * kPi / 180.0), 1e-6, "and swing as far to the side as the turn takes them");
}

void TestPredict() {
    // A steady turn: carried on by its speed, the located head is the true one.
    Config c = Parse("track t\nneck 0 0\npredict 30\nseconds 100");
    ParseTrack("0 0 0\n1000 100 0\n", c);
    const Pose base = Standing();
    const Pose located = LocatedAt(c, base, 0.6, 0), truth = HeadAt(c, base, 0.6), before = HeadAt(c, base, 0.57);
    Near(Between(before.turn, located.turn), 100.0 * 0.030, 1e-6, "a steady turn is carried on by its speed x the ms");
    Near(Between(truth.turn, located.turn), 0.0, 1e-6, "which is the true head: no error in a steady turn");
    c.predictMs = 0.0;
    Near(Between(HeadAt(c, base, 0.6).turn, LocatedAt(c, base, 0.6, 0).turn), 0.0, 1e-12, "without predict the located head is the true one");
    // A sine: the error is what the speed changed by meanwhile, about half the acceleration x the ms squared at the turn's end.
    for (const double ms : {15.0, 30.0}) {
        c = Parse("shake yaw 35 2\nneck 0 0\nseconds 100");
        c.predictMs = ms;
        const double w = 2.0 * kPi * 2.0, t = 1.125, p = ms * 1e-3;
        const double want = 35.0 * std::sin(w * (t - p)) + 35.0 * w * std::cos(w * (t - p)) * p;
        Near(ToAngles(LocatedAt(c, base, t, 0).turn).yaw, want, 1e-4, "a sine carried on from a moment before by its speed then");
        const double error = ToAngles(LocatedAt(c, base, t, 0).turn).yaw - 35.0;
        Near(error, 0.5 * 35.0 * w * w * p * p, 0.02 * 35.0 * w * w * p * p, "its error at the turn's end is half the acceleration x the ms squared");
    }
    // The place too, on the neck.
    c = Parse("shake yaw 35 2\npredict 20\nseconds 100");
    const Pose a = HeadAt(c, base, 1.0), b = LocatedAt(c, base, 1.0, 0);
    Check(std::fabs(a.place[0] - b.place[0]) < 0.005 && std::fabs(a.place[0] - b.place[0]) > 1e-6, "the place is carried on too, with its own small error", a.place[0] - b.place[0], 0.0);
}

void TestEase() {
    Config c = Parse("shake yaw 35 2\nseconds 3");
    Near(Envelope(c, 0.0), 0.0, 1e-12, "nothing of the motion at the start");
    Near(Envelope(c, kEase / 2.0), 0.5, 1e-12, "half of it half way through the ease-in");
    Near(Envelope(c, kEase), 1.0, 1e-12, "all of it from then");
    Near(Envelope(c, 3.0), 1.0, 1e-12, "until its seconds are up");
    Near(Envelope(c, 3.0 + kEase / 2.0), 0.5, 1e-12, "half of it half way back");
    Near(Envelope(c, 3.0 + kEase), 0.0, 1e-12, "none after");
    Check(!Over(c, 3.0 + kEase - 0.001) && Over(c, 3.0 + kEase), "over once it has eased back");
    Pose base = Standing();
    base.turn = FromAngles({-19.0, 3.0, 1.5});
    Near(Between(HeadAt(c, base, 0.0).turn, base.turn), 0.0, 1e-12, "the head starts where it was");
    Near(Between(HeadAt(c, base, 3.0 + kEase).turn, base.turn), 0.0, 1e-12, "and ends there");
    // No jump for the filter: never more in a millisecond than its fastest allows.
    double most = 0.0;
    for (double t = 0.0; t < 3.5; t += 0.001)
        most = std::max(most, Between(HeadAt(c, base, t).turn, HeadAt(c, base, t + 0.001).turn));
    Check(most < 2.0 * kPi * 2.0 * 35.0 * 0.001 * 1.6, "no jump anywhere from start to end", most, 0.44);
    Check(Parse("shake yaw 35 2").seconds == 60.0, "a minute without a seconds line");
}

void TestLevel() {
    Config c = Parse("burst yaw 40 100 300\nlevel 5\nseconds 100");
    Pose base = Standing();
    base.turn = FromAngles({33.0, -30.0, 20.0});
    Angles a = ToAngles(HeadAt(c, base, kEase).turn);
    Near(a.yaw, 33.0, 1e-9, "made level: the head's own yaw kept");
    Near(a.pitch, 5.0, 1e-9, "the pitch asked for");
    Near(a.roll, 0.0, 1e-9, "no roll");
    Near(HeadAt(c, base, kEase).place[1], 1.6, 1e-9, "and its place where it was");
    a = ToAngles(HeadAt(c, base, kEase + 0.100).turn);
    Near(a.yaw, 73.0, 1e-6, "the swing goes from there");
    c = Parse("burst yaw 40 100 300\nseconds 100");
    a = ToAngles(HeadAt(c, base, kEase).turn);
    Near(a.pitch, -30.0, 1e-9, "without the line: from the head's own pitch");
    Near(a.roll, 20.0, 1e-9, "and roll");
}

void TestParse() {
    Config c = Parse("# a run\r\n  shake both 30 12 1.5\r\nneck 0.09 0.07\r\npredict 22\r\nnoise 0.02 0.3\r\nlog 12\r\nseconds 10\r\nnonsense 1 2 3\r\n");
    Check(c.kind == Kind::kShake && c.yaw == 30.0 && c.pitch == 12.0 && c.hz == 1.5, "shake both, with CRLF lines and a comment");
    Check(c.neckForward == 0.09 && c.neckUp == 0.07 && c.predictMs == 22.0 && c.noiseDegrees == 0.02 && c.noiseMm == 0.3, "neck, predict, noise");
    Check(c.logSeconds == 12.0 && c.seconds == 10.0 && !c.level, "log and seconds");
    c = Parse("shake yaw 35 2\noff\n");
    Check(!c.Moves(), "off wins over everything else in the file");
    Check(!Parse(nullptr).Moves() && !Parse("").Moves() && !Parse("log 5\nseconds 3\n").Moves(), "no motion line: nothing replayed");
    c = Parse("burst pitch 20 120\n");
    Check(c.kind == Kind::kBurst && c.burstPitch && c.burst == 20.0 && c.swingMs == 120.0 && c.pauseMs == 300.0, "a burst without its pause: 300 ms");
    c = Parse("shake yaw 999 99\npredict 5000\nseconds 0\nneck 9 9\n");
    Check(c.yaw == 180.0 && c.hz == 20.0 && c.predictMs == 200.0 && c.seconds == 0.1 && c.neckForward == 0.5, "wild numbers are kept in range");
    Check(!Parse("shake sideways 3 4\n").Moves() && !Parse("shake yaw 3\n").Moves() && !Parse("shaker yaw 3 4\n").Moves(), "a line that isn't one is left out");
    c = Parse("track \"head tracks\\one.txt\"  \n");
    Check(c.trackFile == "head tracks\\one.txt", "a track's name in quotes, spaces in it");
    c = Parse("shake yaw 35 2\nlevel\n");
    Check(c.level && c.levelPitch == 0.0, "level alone: at no pitch");
    Check(Describe(Parse("shake yaw 35 2\npredict 20")).find("440") != std::string::npos, "the description says a sine's fastest");
    Check(Describe(Parse("burst yaw 70 110 300")).find("1000") != std::string::npos, "and a burst's");
}

void TestLines() {
    Frame f;
    f.ms = 1234.567;
    f.truth.turn = FromAngles({12.3456, -1.2345, 0.5});
    f.truth.place[0] = 0.0123, f.truth.place[1] = 1.6012, f.truth.place[2] = -0.0456;
    f.located = f.truth;
    f.filtered.turn = FromAngles({11.0, -1.0, 0.25});
    f.picture.turn = FromAngles({10.5, -0.9, 0.2});
    f.picture.place[1] = 1.6;
    f.age = 2;
    f.how = kNamed;
    f.haveCamera = true, f.cameraOwn = true;
    f.camera = {123.4567, -0.9, 0.2};
    f.game = {120.0, -3.0, 0.0};
    f.cameraPlace[0] = 1299.68512, f.cameraPlace[1] = 85.54901, f.cameraPlace[2] = -1140.58703;
    f.gamePlace[0] = 1299.685, f.gamePlace[1] = 85.549, f.gamePlace[2] = -1140.585;
    f.moveMm[0] = -1.25, f.moveMm[1] = 3.5, f.moveMm[2] = 5.75;
    f.look = true, f.lookYaw = 77.52, f.handYaw = -19.18;
    f.pose = 4711, f.build = 4720;
    char line[800];
    const int n = FrameLine(f, line, sizeof(line));
    Check(n > 0 && n < static_cast<int>(sizeof(line)) - 1, "a frame's line fits", n, 0.0);
    double t = 0.0, v[36] = {};
    unsigned age = 0, pose = 0, build = 0;
    int look = 0;
    char how[16] = {}, own[16] = {};
    const int got = sscanf_s(line,
                             "Head test: t %lf | true %lf %lf %lf at %lf %lf %lf | located %lf %lf %lf at %lf %lf %lf | filtered %lf %lf %lf at %lf %lf %lf | picture %lf %lf %lf at "
                             "%lf %lf %lf age %u %15s | camera %15s %lf %lf %lf at %lf %lf %lf | game %lf %lf %lf at %lf %lf %lf | move %lf %lf %lf | look %d %lf hand %lf | pose %u "
                             "build %u",
                             &t, &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8], &v[9], &v[10], &v[11], &v[12], &v[13], &v[14], &v[15], &v[16], &v[17], &v[18], &v[19],
                             &v[20], &v[21], &v[22], &v[23], &age, how, static_cast<unsigned>(sizeof(how)), own, static_cast<unsigned>(sizeof(own)), &v[24], &v[25], &v[26], &v[27],
                             &v[28], &v[29], &v[30], &v[31], &v[32], &v[33], &v[34], &v[35], &t, &t, &t, &look, &t, &t, &pose, &build);
    Check(got == 48, "every number of the line reads back", got, 48.0);
    Near(v[0], 12.3456, 1e-4, "the true head's yaw in the line");
    Near(v[1], -1.2345, 1e-4, "its pitch");
    Near(v[4], 1601.2, 1e-2, "its height in mm");
    Near(v[18], 10.5, 1e-4, "the picture's yaw");
    Check(age == 2 && std::strcmp(how, "named") == 0 && std::strcmp(own, "own") == 0, "its age and how it was told");
    Near(v[24], 123.4567, 1e-4, "the camera's yaw");
    Near(v[29], -1140.58703, 1e-5, "the camera's place to a hundredth of a millimetre");
    Check(look == 1 && pose == 4711 && build == 4720, "the look, the pose and the build");
    Check(std::strstr(line, "build 4720 | smooth ") != nullptr && std::strstr(line, " | by fixed 0.0") != nullptr, "then the smooth head and how the pose was made");
    f.smooth = FromAngles({9.5, -0.8, 0.1});
    f.by = kByRate;
    f.rate = 412.34;
    f.haveCamera = false;
    FrameLine(f, line, sizeof(line));
    Check(std::strstr(line, "| camera none | pose 4711 | smooth 9.5000 -0.8000 0.1000 | by rate 412.3") != nullptr, "no camera: said so, and the rest of the line as ever");
    f.by = kByFallback;
    FrameLine(f, line, sizeof(line));
    Check(std::strstr(line, " | by fallback 412.3") != nullptr, "a frame fallen back says so");
    Second s;
    s.Add({3.0, -1.0, 0.0}, 400.0, 1, true);
    s.Add({-5.0, 0.5, 0.0}, 650.0, 2, true);
    s.Add({1.0, 0.0, 0.0}, 100.0, 1, false);
    s.Line(line, sizeof(line));
    Check(std::strstr(line, "in yaw 3.00 degrees on average, 5.00 at the worst, in pitch 0.50 and 1.00") != nullptr, "a second's slide: the mean and the worst of each");
    Check(std::strstr(line, "up to 650 degrees a second; 3 pictures, their poses 1 frame old 1, 2 1, older 0, not named 1") != nullptr, "and its speed and ages");
    Check(std::strncmp(line, "Head test: a second:", 20) == 0, "a second's line is not taken for a frame's");
}

void TestNoise() {
    Config c = Parse("shake yaw 35 2\nnoise 0.05 0.4\nseconds 1000");
    const Pose base = Standing();
    double turnSum = 0.0, placeSum = 0.0;
    const int n = 4000;
    for (int i = 0; i < n; ++i) {
        const double t = 1.0 + i / 72.0;
        const Pose truth = HeadAt(c, base, t), located = LocatedAt(c, base, t, static_cast<uint32_t>(i));
        const double off = Between(truth.turn, located.turn);
        turnSum += off * off;
        for (int k = 0; k < 3; ++k)
            placeSum += (located.place[k] - truth.place[k]) * (located.place[k] - truth.place[k]);
    }
    Near(std::sqrt(turnSum / n / 3.0), 0.05, 0.004, "the noise's turn: that much an axis");
    Near(std::sqrt(placeSum / n / 3.0) * 1000.0, 0.4, 0.03, "the noise's place: that much an axis");
    Near(Between(LocatedAt(c, base, 2.0, 7).turn, LocatedAt(c, base, 2.0, 7).turn), 0.0, 1e-12, "the same frame, the same noise");
    Check(Between(LocatedAt(c, base, 2.0, 7).turn, LocatedAt(c, base, 2.0, 8).turn) > 1e-4, "another frame, another");
}

void TestTime() {
    Config c = Parse("track t\npredict 25\nnoise 0.02 0.2\nseconds 1000");
    std::string text;
    char row[64];
    for (int i = 0; i < 8000; ++i) {
        std::snprintf(row, sizeof(row), "%.3f %.2f %.2f\n", i * 13.889, 35.0 * std::sin(i * 0.17), 8.0 * std::sin(i * 0.05));
        text += row;
    }
    Check(ParseTrack(text.c_str(), c), "a long track read");
    const Pose base = Standing();
    const auto before = std::chrono::steady_clock::now();
    double sum = 0.0;
    const int frames = 20000;
    for (int i = 0; i < frames; ++i) {
        const double t = i / 72.0;
        sum += HeadAt(c, base, t).turn.y + LocatedAt(c, base, t, static_cast<uint32_t>(i)).turn.y;
    }
    const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - before).count() / frames;
    std::printf("a frame's true and located head (a long track, predict and noise on): %.2f microseconds (%g)\n", us, sum);
    Check(us < 100.0, "a frame's heads in under a tenth of a millisecond", us, 100.0);
}

// A made-up run for tools\headfit.ps1 -SelfTest, in the log's own lines: a sine in yaw and a slower one in pitch on the neck; the picture drawn with the true
// head of 37 ms before; the camera that pose through a fixed place in the world (the body's yaw 48.33 as the game reads it) with its place wobbling 3 mm along
// the world's x 18 times a second; the pictures' poses 1 frame old, every seventh 2. headfit must get the lag, the slide and the wobble back from it.
int Synth(const char* path) {
    FILE* f = nullptr;
    if (fopen_s(&f, path, "w") != 0 || !f) {
        std::printf("cannot write %s\n", path);
        return 1;
    }
    Config c = Parse("track t\nseconds 30");
    std::string text;
    char row[96];
    const double step = 1000.0 / 72.0;
    for (int i = 0; i <= 2300; ++i) {
        const double t = i * step * 1e-3;
        std::snprintf(row, sizeof(row), "%.4f %.5f %.5f\n", i * step, 35.0 * std::sin(2.0 * kPi * 2.0 * t), 8.0 * std::sin(2.0 * kPi * 0.7 * t));
        text += row;
    }
    ParseTrack(text.c_str(), c);
    Pose base = Standing();
    base.turn = FromAngles({-19.0, 3.0, 0.0});
    base.place[0] = 0.02, base.place[2] = -0.03;
    const Turn body = FromAngles({180.0 - 48.33, 0.0, 0.0});
    const double origin[3] = {1299.7, 85.5, -1140.6};
    std::fprintf(f, "[02:00:00.000] Head test: started: %s\n", Describe(c).c_str());
    char line[800];
    WorldSecond world;
    EdgeSecond edge;
    for (int i = 0; i < 2160; ++i) {
        const double t = i * step * 1e-3;
        Frame fr;
        fr.ms = i * step;
        fr.truth = HeadAt(c, base, t);
        fr.located = fr.truth;
        fr.filtered = HeadAt(c, base, t - 0.023);
        fr.picture = HeadAt(c, base, t - 0.037);
        fr.age = i % 7 == 6 ? 2 : 1;
        fr.how = kNamed;
        fr.haveCamera = fr.cameraOwn = true;
        fr.camera = ToAngles(Mul(body, fr.picture.turn));
        double at[3];
        Rotate(body, fr.picture.place, at);
        for (int k = 0; k < 3; ++k)
            fr.cameraPlace[k] = origin[k] + at[k];
        fr.cameraPlace[0] += 0.003 * std::sin(2.0 * kPi * 18.0 * t);
        fr.game = ToAngles(body);
        for (int k = 0; k < 3; ++k)
            fr.gamePlace[k] = origin[k], fr.moveMm[k] = (fr.cameraPlace[k] - origin[k]) * 1000.0;
        fr.look = true, fr.lookYaw = 48.33, fr.handYaw = 0.0;
        fr.pose = 100 + i, fr.build = 200 + i;
        fr.smooth = fr.filtered.turn;
        // The world's turn: every seventh picture (the one named 2 old) told a pose 1.25 degrees from the cameras most of its uploads carried, its uploads of two
        // builds; the others told right, and their first and last uploads those of the build before in every third.
        fr.world = i % 7 == 6 ? 1.25 : 0.0;
        fr.worldLast = i % 3 == 0 ? 2.5 : 0.0;
        fr.worldFirst = fr.worldLast;
        fr.mixed = i % 7 == 6 || i % 3 == 0;
        FrameLine(fr, line, sizeof(line));
        const int ms = static_cast<int>(fr.ms);
        std::fprintf(f, "[02:00:%02d.%03d] %s\n", ms / 1000, ms % 1000, line);
        // The two lines a second that run with a person in the headset too, as xr_runtime.cpp writes them, from the same numbers.
        world.Add(fr.world, fr.worldLast, true, fr.worldFirst, fr.mixed, false);
        edge.Add(Slide(fr.picture.turn, fr.located.turn), Between(fr.picture.turn, fr.located.turn),
                 i > 0 ? Between(HeadAt(c, base, t - step * 1e-3).turn, fr.located.turn) / (step * 1e-3) : 0.0);
        if (i % 72 == 71) {
            char second[900];
            if (world.Line(second, sizeof(second)) > 0)
                std::fprintf(f, "[02:00:%02d.%03d] %s\n", ms / 1000, ms % 1000, second);
            world.Clear();
            if (edge.Line(second, sizeof(second)) > 0)
                std::fprintf(f, "[02:00:%02d.%03d] %s\n", ms / 1000, ms % 1000, second);
            edge.Clear();
        }
    }
    std::fprintf(f, "[02:00:30.250] Head test: ended after 30.0 s: the head is back where it was\n");
    fclose(f);
    std::printf("wrote %s: 2160 frames, the picture 37 ms behind the head, the camera's place wobbling 3 mm at 18 a second\n", path);
    return 0;
}

// A track file as the replay reads and plays it: its rows, its loop, how fast the head turns playing it (frame to frame at 72 a second), how far the
// eyes swing on their pivot. 0 if it is a track.
int Track(const char* path) {
    FILE* f = nullptr;
    if (fopen_s(&f, path, "rb") != 0 || !f) {
        std::printf("cannot read %s\n", path);
        return 1;
    }
    std::string text;
    char block[4096];
    for (size_t n = 0; (n = fread(block, 1, sizeof(block), f)) > 0;)
        text.append(block, n);
    fclose(f);
    Config c = Parse("track t\nseconds 3600");
    c.trackFile = path;
    if (!ParseTrack(text.c_str(), c)) {
        std::printf("%s: no track of two rows or more\n", path);
        return 1;
    }
    const Pose base = Standing();
    double fastest = 0.0, fastestAt = 0.0, widest = 0.0, yawLow = 0.0, yawHigh = 0.0, pitchLow = 0.0, pitchHigh = 0.0;
    const double step = 1.0 / 72.0, length = c.trackMs * 1e-3;
    Pose last = HeadAt(c, base, kEase);
    for (double t = kEase + step; t < length + kEase; t += step) {
        const Pose p = HeadAt(c, base, t);
        const double speed = Between(last.turn, p.turn) / step;
        if (speed > fastest)
            fastest = speed, fastestAt = t;
        const double d[3] = {p.place[0] - base.place[0], p.place[1] - base.place[1], p.place[2] - base.place[2]};
        widest = std::max(widest, std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]));
        const Offsets o = OffsetsAt(c, t);
        yawLow = std::min(yawLow, o.yaw), yawHigh = std::max(yawHigh, o.yaw);
        pitchLow = std::min(pitchLow, o.pitch), pitchHigh = std::max(pitchHigh, o.pitch);
        last = p;
    }
    std::printf("%s\n", Describe(c).c_str());
    std::printf("  played: yaw %.1f to %.1f, pitch %.1f to %.1f degrees; the head at its fastest %.0f degrees a second (%.2f s in); the eyes up to %.0f mm from where they were\n", yawLow,
                yawHigh, pitchLow, pitchHigh, fastest, fastestAt, widest * 1000.0);
    return 0;
}

// ---- The pose filters with no game ----
// The fixed filter and "poseadapt" as xr_runtime.cpp FilterViews has them: its turn only, in floats, step for step (a copy, kept in step with it by hand; the
// checks in TestModels hold it to what that filter is known to do). "poserate" is src/headtest.cpp's RateFilter, the very code the game runs.
struct Q4 {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;
};
constexpr float kPiF = 3.14159265f;

float AngleBetweenF(const Q4& a, const Q4& b) {
    const float dot = std::fabs(a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w);
    return 2.0f * std::acos(std::min(dot, 1.0f)) * 57.29578f;
}

Q4 TowardsF(const Q4& from, Q4 to, float share) {
    if (from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w < 0.0f)
        to = {-to.x, -to.y, -to.z, -to.w};
    Q4 r{from.x + (to.x - from.x) * share, from.y + (to.y - from.y) * share, from.z + (to.z - from.z) * share, from.w + (to.w - from.w) * share};
    const float n = std::sqrt(r.x * r.x + r.y * r.y + r.z * r.z + r.w * r.w);
    return n > 0.0f ? Q4{r.x / n, r.y / n, r.z / n, r.w / n} : from;
}

Q4 MulF(const Q4& l, const Q4& r) {
    return Q4{l.w * r.x + l.x * r.w + l.y * r.z - l.z * r.y, l.w * r.y - l.x * r.z + l.y * r.w + l.z * r.x, l.w * r.z + l.x * r.y - l.y * r.x + l.z * r.w,
              l.w * r.w - l.x * r.x - l.y * r.y - l.z * r.z};
}

Q4 ToQ4(const Turn& t) {
    return {static_cast<float>(t.x), static_cast<float>(t.y), static_cast<float>(t.z), static_cast<float>(t.w)};
}

Turn ToTurn(const Q4& q) {
    return {q.x, q.y, q.z, q.w};
}

struct FilterModel {
    bool have = false;
    Q4 turn[2], turnWas, rawWas;
    float speed[3] = {}, rawRate[3] = {};
    float period = 1.0f / 72.0f, age = 1.0f;
    // q: the located turn. seconds: since the last frame. turnHz, leadMs: "posefilter", "poselead". adaptMs: "poseadapt" (0: the fixed filter). drawnAge: how
    // many poses old the last picture's was.
    Q4 Step(const Q4& q, float seconds, bool freshAsked, float turnHz, float leadMs, float adaptMs, uint32_t drawnAge) {
        if (!have)
            seconds = 0.0f;
        const bool fresh = !have || freshAsked;
        if (fresh || AngleBetweenF(q, turn[1]) > 40.0f || turnHz <= 0.0f) {
            turn[0] = turn[1] = turnWas = q;
            speed[0] = speed[1] = speed[2] = 0.0f;
        }
        if (fresh) {
            rawWas = q;
            rawRate[0] = rawRate[1] = rawRate[2] = 0.0f;
        }
        have = true;
        if (seconds > 0.004f && seconds < 0.02f)
            period += (seconds - period) * 0.1f;
        age += (std::clamp(static_cast<float>(drawnAge), 1.0f, 3.0f) - age) * 0.1f;
        if (seconds > 0.0f) {
            const Q4 w{-rawWas.x, -rawWas.y, -rawWas.z, rawWas.w};
            Q4 step{q.w * w.x + q.x * w.w + q.y * w.z - q.z * w.y, q.w * w.y - q.x * w.z + q.y * w.w + q.z * w.x, q.w * w.z + q.x * w.y - q.y * w.x + q.z * w.w,
                    q.w * w.w - q.x * w.x - q.y * w.y - q.z * w.z};
            if (step.w < 0.0f)
                step = {-step.x, -step.y, -step.z, -step.w};
            const float share = 1.0f - std::exp(-2.0f * kPiF * 12.0f * seconds), toRate = 2.0f / seconds;
            rawRate[0] += (step.x * toRate - rawRate[0]) * share;
            rawRate[1] += (step.y * toRate - rawRate[1]) * share;
            rawRate[2] += (step.z * toRate - rawRate[2]) * share;
        }
        rawWas = q;
        const float rawSpeed = std::sqrt(rawRate[0] * rawRate[0] + rawRate[1] * rawRate[1] + rawRate[2] * rawRate[2]) * (180.0f / kPiF);
        const float hzNow = adaptMs > 0.0f ? turnHz * (1.0f + rawSpeed / 15.0f) : turnHz;
        if (turnHz > 0.0f) {
            const float share = 1.0f - std::exp(-2.0f * kPiF * hzNow * seconds);
            turn[0] = TowardsF(turn[0], q, share);
            turn[1] = TowardsF(turn[1], turn[0], share);
        }
        Q4 f = turn[1];
        if (turnHz > 0.0f && seconds > 0.0f) {
            Q4 step = MulF(f, Q4{-turnWas.x, -turnWas.y, -turnWas.z, turnWas.w});
            if (step.w < 0.0f)
                step = {-step.x, -step.y, -step.z, -step.w};
            const float share = 1.0f - std::exp(-2.0f * kPiF * 3.0f * seconds);
            const float toSpeed = 2.0f / seconds;
            speed[0] += (step.x * toSpeed - speed[0]) * share;
            speed[1] += (step.y * toSpeed - speed[1]) * share;
            speed[2] += (step.z * toSpeed - speed[2]) * share;
            turnWas = f;
            float r[3];
            if (adaptMs > 0.0f) {
                const float held = hzNow > 0.0f ? 1.0f / (kPiF * hzNow) : 0.0f;
                const float lead = ((adaptMs > 1.0f ? adaptMs * 1e-3f : age * period) + held) * std::clamp((rawSpeed - 10.0f) / 30.0f, 0.0f, 1.0f);
                r[0] = rawRate[0] * lead, r[1] = rawRate[1] * lead, r[2] = rawRate[2] * lead;
            } else {
                const float lead = leadMs * 1e-3f;
                r[0] = speed[0] * lead, r[1] = speed[1] * lead, r[2] = speed[2] * lead;
            }
            const float angle = std::sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2]);
            if (angle > 1e-5f) {
                const float kept = std::min(angle, 0.6f);
                const float s = std::sin(kept / 2) / angle, c = std::cos(kept / 2);
                f = MulF(Q4{r[0] * s, r[1] * s, r[2] * s, c}, f);
            }
        }
        return f;
    }
};

// The head as the aim's limit and the walk take it beside a pose candidate (xr_runtime.cpp FilterViews): the located turn through two low-passes, no lead.
struct SmoothModel {
    bool have = false;
    Q4 turn[2];
    Q4 Step(const Q4& q, float seconds, float hz) {
        if (!have || AngleBetweenF(q, turn[1]) > 40.0f || hz <= 0.0f)
            turn[0] = turn[1] = q;
        else {
            const float share = 1.0f - std::exp(-2.0f * kPiF * hz * seconds);
            turn[0] = TowardsF(turn[0], q, share);
            turn[1] = TowardsF(turn[1], turn[0], share);
        }
        have = true;
        return turn[1];
    }
};

constexpr float kFrame = 13888889.0f / 1e9f;  // a frame at 72 a second as the game has it: nanoseconds, in a float

// The fixed filter and "poseadapt" as copied, held to what they are known to do: a steady turn's lead, and how much of a zigzag in the located head comes out.
void TestModels() {
    auto yawOf = [](const Q4& q) { return ToAngles(ToTurn(q)).yaw; };
    // A steady turn: the fixed filter holds back 2 (1 - share) / share frames (40.4 ms at 6 Hz) and leads 80 ms: 39.6 ms ahead of the located head.
    for (const double speed : {100.0, 300.0}) {
        FilterModel fixed, adapt;
        double aheadFixed = 0.0, aheadAdapt = 0.0;
        for (int k = 0; k < 400; ++k) {
            const Q4 q = ToQ4(FromAngles({speed * k / 72.0 - 60.0, 0.0, 0.0}));
            aheadFixed = Wrap(yawOf(fixed.Step(q, kFrame, false, 6.0f, 80.0f, 0.0f, 2)) - yawOf(q));
            aheadAdapt = Wrap(yawOf(adapt.Step(q, kFrame, false, 6.0f, 80.0f, 1.0f, 1)) - yawOf(q));
        }
        const double share = 1.0 - std::exp(-2.0 * kPi * 6.0 / 72.0), held = 2.0 * (1.0 - share) / share / 72.0;
        Near(aheadFixed, speed * (0.080 - held), 0.02 * speed / 100.0, "the fixed filter in a steady turn: ahead of the located head by its lead less what it holds back");
        // So with pictures two frames old the picture's pose is ahead of the head when shown by 1.2 degrees at 100 a second and 3.6 at 300.
        Near(aheadFixed - speed * 2.0 / 72.0, speed == 100.0 ? 1.2 : 3.6, 0.1, "which is 1.2 and 3.6 degrees past the head for a picture two frames old");
        // "poseadapt" for pictures one frame old: a frame's turn ahead, and up to 0.8 more (its delay taken as a continuous filter's).
        Check(aheadAdapt - speed / 72.0 > 0.0 && aheadAdapt - speed / 72.0 < 0.85, "poseadapt in a steady turn: a frame ahead and up to 0.8 degrees more", aheadAdapt - speed / 72.0, 0.8);
    }
    // A zigzag of a hundredth of a degree each way on alternate frames, on a steady turn: what comes out, as a share of what went in.
    auto zigzag = [&](double speed, float adaptMs, uint32_t age) {
        FilterModel plain, rough;
        double most = 0.0;
        for (int k = 0; k < 500; ++k) {
            const double yaw = speed * k / 72.0 - 60.0;
            const double a = yawOf(plain.Step(ToQ4(FromAngles({yaw, 0.0, 0.0})), kFrame, false, 6.0f, 80.0f, adaptMs, age));
            const double b = yawOf(rough.Step(ToQ4(FromAngles({yaw + (k % 2 ? 0.01 : -0.01), 0.0, 0.0})), kFrame, false, 6.0f, 80.0f, adaptMs, age));
            if (k >= 400)
                most = std::max(most, std::fabs(Wrap(b - a)));
        }
        return most / 0.01;
    };
    Near(zigzag(100.0, 0.0f, 2), 0.16, 0.03, "the fixed filter hands on 0.16 of a zigzag");
    Near(zigzag(300.0, 0.0f, 1), 0.16, 0.03, "at any speed and age");
    Near(zigzag(100.0, 1.0f, 2), 3.0, 0.45, "poseadapt hands on three times a zigzag at 100 degrees a second, pictures two frames old");
    Near(zigzag(300.0, 1.0f, 2), 2.9, 0.45, "and at 300");
    Near(zigzag(100.0, 1.0f, 1), 2.1, 0.35, "twice with pictures one frame old");
    // The smooth head: the two low-passes alone hand on 0.07 of it, and stand 40.4 ms behind in a steady turn.
    SmoothModel smooth;
    double out = 0.0, behind = 0.0;
    for (int k = 0; k < 400; ++k) {
        const Q4 q = ToQ4(FromAngles({100.0 * k / 72.0 - 60.0 + (k % 2 ? 0.01 : -0.01), 0.0, 0.0}));
        const double was = behind;
        behind = Wrap(yawOf(q) - yawOf(smooth.Step(q, kFrame, 6.0f))) - (k % 2 ? 0.01 : -0.01);
        out = std::fabs(behind - was) / 2.0;
    }
    Near(out / 0.01, 0.065, 0.02, "two low-passes at 6 Hz hand on 0.07 of a zigzag");
    Near(behind, 100.0 * 0.0404, 0.05, "and stand 40.4 ms behind a steady turn");
}

// The cubic through a track's rows: a sine sampled 72 times a second comes back between its rows, and its turn rate has no step at a row.
void TestCurve() {
    Config c = Parse("track t\nneck 0 0\nseconds 1000");
    std::string text;
    char row[64];
    const double w = 2.0 * kPi * 2.0;
    for (int i = 0; i <= 720; ++i) {
        std::snprintf(row, sizeof(row), "%.4f %.6f %.6f\n", i * 1000.0 / 72.0, 35.0 * std::sin(w * i / 72.0), 10.0 * std::sin(0.5 * w * i / 72.0));
        text += row;
    }
    Check(ParseTrack(text.c_str(), c), "a sine's rows read");
    double worst = 0.0, worstRate = 0.0, worstStep = 0.0;
    for (int i = 0; i < 4000; ++i) {
        const double t = 1.0 + i * 0.00173;
        const Offsets o = OffsetsAt(c, t);
        worst = std::max(worst, std::fabs(o.yaw - 35.0 * std::sin(w * t)));
        const double h = 0.0002, rate = (OffsetsAt(c, t + h).yaw - OffsetsAt(c, t - h).yaw) / (2.0 * h);
        worstRate = std::max(worstRate, std::fabs(rate - 35.0 * w * std::cos(w * t)));
    }
    for (int i = 80; i < 400; ++i) {
        // (Just before a row and just after it, a fifth of a millisecond apart: the sine's own rate changes 1.1 degrees a second in that at the most.)
        const double at = i / 72.0, h = 0.00005;
        const double before = (OffsetsAt(c, at - h).yaw - OffsetsAt(c, at - 3.0 * h).yaw) / (2.0 * h), after = (OffsetsAt(c, at + 3.0 * h).yaw - OffsetsAt(c, at + h).yaw) / (2.0 * h);
        worstStep = std::max(worstStep, std::fabs(after - before));
    }
    Check(worst < 0.004, "the curve through a sine's rows is the sine between them (to 0.004 degrees)", worst, 0.004);
    Check(worstRate < 3.0, "its turn rate is the sine's (to 3 of 440 degrees a second)", worstRate, 3.0);
    Check(worstStep < 1.5, "and has no step at a row", worstStep, 1.5);
    // The yaw goes on past 180 without a leap.
    Check(ParseTrack("0 150 0\n100 170 0\n200 -170 0\n300 -150 0\n", c), "rows crossing 180");
    Near(Wrap(OffsetsAt(c, 0.150).yaw - 180.0), 0.0, 1e-9, "the curve crosses 180 the short way");
}

// The located head from rows of its own: off the true head by the nearest row's error, whole.
void TestLocated() {
    Config c = Parse("track true.txt\nlocated \"as located.txt\"\npredict 30\nnoise 0.5 0.4\nneck 0 0\nseconds 1000");
    Check(c.locatedFile == "as located.txt", "a located line names its file");
    Check(!ParseLocated("0 0 0\n100 10 0\n", c) && c.locatedOff.empty(), "no located rows without the track read first");
    Check(ParseTrack("0 0 0\n100 10 0\n200 20 0\n300 30 0\n", c), "the true head's rows");
    Check(ParseLocated("0 0.5 0.1\n100 10.2 -0.3\n200 19 0\n300 30 0.4\n", c) && c.locatedOff.size() == 4, "the located head's rows");
    const Pose base = Standing();
    const double round = c.trackMs * 1e-3 * 3.0;  // (a few rounds in: the ease-in is over)
    Angles a = ToAngles(LocatedAt(c, base, round + 0.100, 7).turn);
    Near(a.yaw, 10.2, 1e-9, "at a row: that row's own yaw");
    Near(a.pitch, -0.3, 1e-9, "and pitch");
    a = ToAngles(LocatedAt(c, base, round + 0.104, 8).turn);
    Near(a.yaw, 10.4 + 0.2, 1e-9, "4 ms after it: the true head then, off by that row's error");
    a = ToAngles(LocatedAt(c, base, round + 0.151, 9).turn);
    Near(a.yaw, 15.1 - 1.0, 1e-9, "nearer the next row: off by that one's (never one between the two)");
    Near(ToAngles(HeadAt(c, base, round + 0.151).turn).yaw, 15.1, 1e-9, "the true head has none of it");
    Check(Describe(c).find("as located.txt") != std::string::npos && Describe(c).find("predict") == std::string::npos, "the description names the rows, and no predictor");
    // Its place still has the noise's millimetres; its turn has neither the noise nor the predictor.
    double placeSum = 0.0;
    for (uint32_t f = 0; f < 2000; ++f) {
        const Pose p = LocatedAt(c, base, round + 0.100, f), truth = HeadAt(c, base, round + 0.100);
        for (int k = 0; k < 3; ++k)
            placeSum += (p.place[k] - truth.place[k]) * (p.place[k] - truth.place[k]);
        if (f < 5)
            Near(ToAngles(p.turn).yaw, 10.2, 1e-9, "whatever the frame: the row's yaw");
    }
    Near(std::sqrt(placeSum / 2000.0 / 3.0) * 1000.0, 0.4, 0.03, "the place's noise stays");
    Check(ParseTrack("0 0 0\n100 10 0\n", c) && c.locatedOff.empty(), "another track read: the located rows go with the old one");
}

// The noise growing with the head's speed, and the rate as a runtime gives it.
void TestNoiseFastAndRate() {
    Config c = Parse("shake yaw 35 2\nnoise 0.02\nnoisefast 0.3 30 150\nneck 0 0\nseconds 100000");
    Check(c.noiseFast == 0.3 && c.noiseFrom == 30.0 && c.noiseTo == 150.0, "noisefast and its two speeds");
    Check(Parse("noisefast 0.2").noiseFrom == 30.0 && Parse("noisefast 0.2").noiseTo == 150.0, "without them: 30 and 150");
    const Pose base = Standing();
    double slow = 0.0, fast = 0.0;
    int slowN = 0, fastN = 0;
    for (int i = 0; i < 60000; ++i) {
        const double t = 1.0 + i / 72.0 + i * 1e-5, speed = SpeedAt(c, base, t);
        if (speed > 30.0 && speed < 150.0)
            continue;
        const double off = Between(HeadAt(c, base, t).turn, LocatedAt(c, base, t, static_cast<uint32_t>(i)).turn);
        (speed <= 30.0 ? slow : fast) += off * off;
        ++(speed <= 30.0 ? slowN : fastN);
    }
    Near(std::sqrt(slow / slowN / 3.0), 0.02, 0.003, "the noise under 30 degrees a second: as the noise line says");
    Near(std::sqrt(fast / fastN / 3.0), 0.3, 0.02, "over 150: as noisefast says");
    // The rate: the true head's own, as the room has it.
    c = Parse("shake yaw 35 2\nneck 0 0\nseconds 100000");
    const double w = 2.0 * kPi * 2.0, t = 3.21;
    double r[3];
    RateAt(c, base, t, 0, r);
    Near(r[1] * 180.0 / kPi, 35.0 * w * std::cos(w * t), 0.01, "a yaw shake's rate is about the room's up, as fast as the sine turns");
    Near(std::fabs(r[0]) + std::fabs(r[2]), 0.0, 1e-9, "and about no other axis");
    c = Parse("shake yaw 35 2\nrate 20\nneck 0 0\nseconds 100000");
    Check(c.rateLateMs == 20.0 && c.rateNoise == 0.0 && !c.rateOwn, "a rate line: late");
    RateAt(c, base, t, 0, r);
    Near(r[1] * 180.0 / kPi, 35.0 * w * std::cos(w * (t - 0.020)), 0.01, "given 20 ms late: the rate of 20 ms before");
    c = Parse("shake yaw 35 2\nrate 0 0 own\nlevel 30\nneck 0 0\nseconds 100000");
    Check(c.rateOwn, "a rate line: own");
    RateAt(c, base, t, 0, r);
    const double size = 35.0 * w * std::cos(w * t);
    Near(r[1] * 180.0 / kPi, size * std::cos(30.0 * kPi / 180.0), 0.01, "as a head looking 30 degrees up has it: the cosine of it about its own up");
    Near(std::fabs(r[2]) * 180.0 / kPi, std::fabs(size) * std::sin(30.0 * kPi / 180.0), 0.01, "and the sine about its own back");
    c = Parse("shake yaw 35 2\nrate 0 4\nneck 0 0\nseconds 100000");
    double sum = 0.0;
    for (uint32_t f = 0; f < 4000; ++f) {
        double clean[3];
        Config quiet = c;
        quiet.rateNoise = 0.0;
        RateAt(c, base, t, f, r);
        RateAt(quiet, base, t, f, clean);
        for (int k = 0; k < 3; ++k)
            sum += (r[k] - clean[k]) * (r[k] - clean[k]);
    }
    Near(std::sqrt(sum / 4000.0 / 3.0) * 180.0 / kPi, 4.0, 0.3, "the rate's noise: that many degrees a second an axis");
}

// The turn carried by the head's rate.
void TestRateFilter() {
    const double dt = 1.0 / 72.0, rad = kPi / 180.0;
    // A steady turn with its rate: the carried turn is the located one, and a frame's lead puts it where the head is a frame later.
    {
        RateFilter f;
        const double rate[3] = {0.0, 300.0 * rad, 0.0};
        double apart = 0.0, worst = 0.0, worstApart = 0.0;
        for (int k = 0; k < 300; ++k) {
            const Turn out = f.Step(FromAngles({300.0 * k * dt, 5.0, 0.0}), rate, k ? dt : 0.0, 6.0, dt, false, &apart);
            if (k > 0) {
                worst = std::max(worst, Between(out, FromAngles({300.0 * (k + 1) * dt, 5.0, 0.0})));
                worstApart = std::max(worstApart, apart);
            }
        }
        Near(worst, 0.0, 1e-6, "a steady turn carried by its rate and led a frame: the head a frame later, exactly");
        Near(worstApart, 0.0, 1e-6, "the carried turn on the located one all the way");
    }
    // A shake of 35 degrees three times a second: within a frame's lead error of the head when shown (half the speeding up x a frame squared: 1.2 degrees), where
    // the fixed filter is tens of degrees off.
    {
        RateFilter f;
        FilterModel fixed;
        Config c = Parse("shake yaw 35 3\nneck 0 0\nseconds 100000");
        const Pose base = Standing();
        double worst = 0.0, worstFixed = 0.0, worstApart = 0.0;
        for (int k = 0; k < 720; ++k) {
            const double t = 1.0 + k * dt;
            double rate[3], apart = 0.0;
            RateAt(c, base, t, 0, rate);
            const Turn located = HeadAt(c, base, t).turn, out = f.Step(located, rate, k ? dt : 0.0, 6.0, dt, false, &apart);
            const Turn shown = HeadAt(c, base, t + dt).turn;
            const Q4 led = fixed.Step(ToQ4(located), kFrame, false, 6.0f, 80.0f, 0.0f, 1);
            if (k > 72) {
                worst = std::max(worst, Between(out, shown));
                worstApart = std::max(worstApart, apart);
                worstFixed = std::max(worstFixed, Between(ToTurn(led), shown));
            }
        }
        Check(worst < 1.3, "a shake of 35 degrees three times a second: within 1.3 degrees of the head when shown", worst, 1.3);
        Check(worstApart < 0.2, "the carried turn within 0.2 degrees of the located one", worstApart, 0.2);
        Check(worstFixed > 20.0, "where the fixed filter is over 20 degrees off", worstFixed, 20.0);
    }
    // The same shake with a frame lost in every nine (the display time two frames on): carried across the longer step as well.
    {
        RateFilter f;
        Config c = Parse("shake yaw 35 3\nneck 0 0\nseconds 100000");
        const Pose base = Standing();
        double t = 1.0, worst = 0.0, worstApart = 0.0;
        for (int k = 0; k < 720; ++k) {
            const double step = k == 0 ? 0.0 : k % 9 == 0 ? 2.0 * dt : dt;
            t += step;
            double rate[3], apart = 0.0;
            RateAt(c, base, t, 0, rate);
            const Turn out = f.Step(HeadAt(c, base, t).turn, rate, step, 6.0, dt, false, &apart);
            if (k > 72) {
                worst = std::max(worst, Between(out, HeadAt(c, base, t + dt).turn));
                worstApart = std::max(worstApart, apart);
            }
        }
        Check(worst < 1.6, "with a frame lost in every nine: within 1.6 degrees of the head a frame on", worst, 1.6);
        Check(worstApart < 0.6, "the carried turn within 0.6 degrees of the located one", worstApart, 0.6);
    }
    // A zigzag in the located head on a still one: 0.07 of it comes out (the two stages), against the fixed filter's 0.16.
    {
        RateFilter f;
        const double none[3] = {};
        double was = 0.0, out = 0.0;
        for (int k = 0; k < 300; ++k) {
            const double yaw = ToAngles(f.Step(FromAngles({k % 2 ? 0.1 : -0.1, 0.0, 0.0}), none, k ? dt : 0.0, 6.0, dt, false)).yaw;
            out = std::fabs(yaw - was) / 2.0;
            was = yaw;
        }
        Near(out / 0.1, 0.065, 0.01, "a zigzag in the located head: 0.07 of it comes out");
    }
    // A rate that is none at all, on a steady turn: the carried turn stands 2 (1 - share) / share frames behind, as two low-passes do.
    {
        RateFilter f;
        const double none[3] = {};
        double apart = 0.0;
        for (int k = 0; k < 300; ++k)
            f.Step(FromAngles({300.0 * k * dt, 0.0, 0.0}), none, k ? dt : 0.0, 6.0, dt, false, &apart);
        const double share = 1.0 - std::exp(-2.0 * kPi * 6.0 * dt);
        Near(apart, 2.0 * (1.0 - share) / share * 300.0 * dt, 0.01, "with no rate at all it is two low-passes: 12.1 degrees behind at 300 a second");
    }
    // Started again: asked to, and by itself 40 degrees from the located turn; with no cut-off the located turn is taken whole.
    {
        RateFilter f;
        const double rate[3] = {0.0, 100.0 * rad, 0.0};
        f.Step(FromAngles({0.0, 0.0, 0.0}), rate, 0.0, 6.0, 0.0, false);
        f.Step(FromAngles({10.0, 0.0, 0.0}), rate, dt, 6.0, 0.0, false);
        Near(ToAngles(f.Step(FromAngles({70.0, 0.0, 0.0}), rate, dt, 6.0, 0.0, false)).yaw, 70.0, 1e-9, "a jump of over 40 degrees: taken at once");
        Check(ToAngles(f.Step(FromAngles({80.0, 0.0, 0.0}), rate, dt, 6.0, 0.0, false)).yaw < 79.0, "a step under 40 is pulled to, not taken");
        Near(ToAngles(f.Step(FromAngles({85.0, 0.0, 0.0}), rate, dt, 6.0, 0.0, true)).yaw, 85.0, 1e-9, "asked to start again: at the located turn");
        Near(ToAngles(f.Step(FromAngles({87.0, 0.0, 0.0}), rate, dt, 0.0, 0.010, false)).yaw, 88.0, 1e-9, "no cut-off: the located turn, led by the rate (10 ms at 100 a second)");
        f.Follow(FromAngles({20.0, 0.0, 0.0}));
        double apart = 0.0;
        f.Step(FromAngles({20.0, 0.0, 0.0}), rate, dt, 6.0, 0.0, false, &apart);
        Check(apart < 2.0, "followed from outside: it goes on from that turn", apart, 2.0);
        const double wild[3] = {0.0, 5000.0 * rad, 0.0};
        Near(Between(f.Step(FromAngles({20.0, 0.0, 0.0}), wild, 0.0, 0.0, 0.1, true), FromAngles({20.0, 0.0, 0.0})), 0.6 * 180.0 / kPi, 1e-6, "the lead is 0.6 radians at the most");
    }
    // The pictures' age, held: it changes once 36 pictures in a row came with another.
    {
        RateFilter f;
        for (int k = 0; k < 35; ++k)
            f.NoteAge(2);
        Check(f.age == 1, "35 pictures two frames old: still leading for one");
        f.NoteAge(1);
        for (int k = 0; k < 35; ++k)
            f.NoteAge(2);
        Check(f.age == 1, "one of another age between: counted again");
        f.NoteAge(2);
        Check(f.age == 2, "36 in a row: leading for two");
        for (int k = 0; k < 200; ++k)
            f.NoteAge(k % 9 == 0 ? 1 : 2);
        Check(f.age == 2, "a slip now and then changes nothing");
        f.NoteAge(9);
        Check(f.ageSeen == 3, "never more than three frames");
    }
}

// Whether a rate given is the head's.
void TestRateFit() {
    const Pose base = Standing();
    char line[600];
    auto run = [&](const char* text, int frames, bool own, RateFit& fit) {
        Config c = Parse(text);
        int length = 0;
        for (int k = 0; k < frames; ++k) {
            const double t = 1.0 + k / 72.0;
            double rate[3];
            RateAt(c, base, t, static_cast<uint32_t>(k), rate);
            fit.Add(LocatedAt(c, base, t, static_cast<uint32_t>(k)).turn, rate, t);
        }
        length = fit.Line(line, sizeof(line), own);
        return length;
    };
    RateFit fit;
    Check(run("shake yaw 35 2\nneck 0 0\nseconds 100000", 144, false, fit) > 0 && fit.fits[0], "the head's own rate fits");
    Check(std::strstr(line, "as the head's rate of 0 ms before") != nullptr, "and is taken for the rate of that moment");
    fit = RateFit{};
    Check(run("shake yaw 35 2\nrate 14\nnoise 0.05\nneck 0 0\nseconds 100000", 144, false, fit) > 0 && fit.fits[0], "given 14 ms late it still fits");
    Check(std::strstr(line, "as the head's rate of 14 ms before") != nullptr, "and how late it is given is told");
    fit = RateFit{};
    Check(run("shake yaw 35 2\nrate 28\nneck 0 0\nseconds 100000", 144, false, fit) > 0 && std::strstr(line, "of 28 ms before") != nullptr, "28 ms late: told");
    // No rate at all, or one about the wrong axes with the head tilted far: not the head's.
    fit = RateFit{};
    {
        Config c = Parse("shake yaw 35 2\nneck 0 0\nseconds 100000");
        const double none[3] = {};
        for (int k = 0; k < 144; ++k)
            fit.Add(HeadAt(c, base, 1.0 + k / 72.0).turn, none, 1.0 + k / 72.0);
        Check(fit.Line(line, sizeof(line), false) > 0 && !fit.fits[0] && std::strstr(line, "NOT the head's rate") != nullptr, "a rate of nothing is not the head's");
    }
    fit = RateFit{};
    Check(run("shake yaw 35 2\nrate 0 0 own\nlevel 60\nneck 0 0\nseconds 100000", 144, false, fit) > 0 && !fit.fits[0] && fit.fits[1],
          "a rate as the head itself has it, the head looking 60 degrees up: not the head's as the room has it, the head's as the head has it");
    fit = RateFit{};
    Check(run("shake yaw 35 2\nrate 0 0 own\nlevel 60\nneck 0 0\nseconds 100000", 144, true, fit) > 0 && std::strstr(line, "NOT the head's rate") == nullptr, "used that way it fits");
    // Too little turning to tell: no line, no verdict.
    fit = RateFit{};
    Check(run("shake yaw 35 2\nneck 0 0\nseconds 100000", 20, false, fit) == 0 && fit.fits[0], "a few frames: nothing said");
    fit = RateFit{};
    Check(run("shake yaw 0.2 1\nneck 0 0\nseconds 100000", 300, false, fit) == 0, "a head all but still: nothing said");
    // A gap in the frames: no turning told across it.
    fit = RateFit{};
    {
        Config c = Parse("shake yaw 35 2\nneck 0 0\nseconds 100000");
        for (int k = 0; k < 400; ++k) {
            const double t = 1.0 + k / 72.0 + (k / 5) * 0.2;  // (a fifth of a second lost every five frames)
            double rate[3];
            RateAt(c, base, t, 0, rate);
            fit.Add(HeadAt(c, base, t).turn, rate, t);
        }
        Check(fit.frames == 0, "frames in fives with gaps between: none judged", fit.frames, 0.0);
    }
}

// "poserate" whole, as the game runs it each frame.
void TestRatePose() {
    const Pose base = Standing();
    const double dt = 1.0 / 72.0;
    char line[700], fitLine[700];
    // A shake with its own rate given 28 ms late: used as it comes, the picture is degrees off at the swings' ends; told how late it is and made up for, far less.
    double worst[2] = {};
    for (int makeUp = 0; makeUp < 2; ++makeUp) {
        Config c = Parse("shake yaw 35 2.5\nrate 28\nneck 0 0\nseconds 100000");
        RatePose pose;
        int waited = 0, carried = 0;
        for (int k = 0; k < 720; ++k) {
            const double t = 1.0 + k * dt;
            double rate[3];
            RateAt(c, base, t, static_cast<uint32_t>(k), rate);
            const Turn located = HeadAt(c, base, t).turn;
            Turn out = located;
            const bool by = pose.Step(located, rate, false, makeUp != 0, t, k ? dt : 0.0, 6.0, dt, 0.0, 1, false, located, out);
            carried += by;
            waited += !by && carried == 0;
            if (k % 72 == 71)
                pose.Line(line, sizeof(line), fitLine, sizeof(fitLine), false, makeUp != 0);
            if (k > 360)
                worst[makeUp] = std::max(worst[makeUp], Between(out, HeadAt(c, base, t + dt).turn));
        }
        // Not used until it has shown itself the head's rate: 24 frames of turning, and the few before them the fit needs to begin.
        Check(waited >= 24 && waited <= 36, "the rate is used once 24 frames of turning have shown it to be the head's", waited, 30.0);
        Check(carried == 720 - waited, "and in every frame from then on", carried, 720.0 - waited);
        Near(pose.lateMs, 28.0, 0.5, "the fit says how late the rate is given");
        Check(std::strstr(line, "Head rate: the head's turn carried by the rate given (taken as the room has it)") == line, "the second's line");
        Check((std::strstr(line, "carried on 28 ms by its own change") != nullptr) == (makeUp != 0), "which says when the rate is carried on for its lateness");
    }
    Check(worst[0] > 6.0, "a rate 28 ms late, used as it comes: over 6 degrees off at the worst", worst[0], 6.0);
    Check(worst[1] < 0.5 * worst[0], "made up for: less than half of that", worst[1], 0.5 * worst[0]);
    // No rate given: the turn of the filter beside it, counted; then on from there with no jump.
    {
        RatePose pose;
        pose.fit.told = true;  // (the rate has shown itself the head's already)
        const double rate[3] = {0.0, 2.0, 0.0};
        const Turn beside = FromAngles({33.0, 0.0, 0.0});
        Turn out;
        pose.Step(FromAngles({10.0, 0.0, 0.0}), rate, false, false, 1.0, 0.0, 6.0, dt, 0.0, 1, false, beside, out);
        Check(!pose.Step(FromAngles({12.0, 0.0, 0.0}), nullptr, false, false, 1.0 + dt, dt, 6.0, dt, 0.0, 1, false, beside, out) && pose.back, "no rate given: fallen back");
        const double nan[3] = {0.0, std::nan(""), 0.0};
        Check(!pose.Step(FromAngles({12.0, 0.0, 0.0}), nan, false, false, 1.0 + 2.0 * dt, dt, 6.0, dt, 0.0, 1, false, beside, out), "a rate that is not a number: fallen back");
        Check(pose.Step(FromAngles({14.0, 0.0, 0.0}), rate, false, false, 1.0 + 3.0 * dt, dt, 6.0, 0.0, 0.0, 1, false, beside, out) && !pose.back, "a rate again: carried again");
        Check(ToAngles(out).yaw > 24.0, "from the turn the filter beside it had, pulled towards the located one from there", ToAngles(out).yaw, 24.0);
        pose.Line(line, sizeof(line), fitLine, sizeof(fitLine), false, false);
        Check(std::strstr(line, "2 of 4 frames fell back to the filter beside it (2 with no rate given, 0 with a rate that is not the head's, 0 before the rate had shown itself the head's)") != nullptr, "the fallen back frames are counted");
    }
    // A head all but still: all but none of the rate is used, however noisy it is; at speed all of it.
    {
        RatePose pose;
        pose.fit.told = true;
        Turn out;
        const double small[3] = {0.0, 1.0 * kPi / 180.0, 0.0}, fast[3] = {0.0, 200.0 * kPi / 180.0, 0.0};
        pose.Step(FromAngles({0.0, 0.0, 0.0}), small, false, false, 1.0, 0.0, 6.0, dt, 0.0, 1, false, Turn{}, out);
        Check(pose.size < 0.02, "a rate of 1 degree a second: a sixtieth of it used", pose.size, 0.02);
        pose.Step(FromAngles({0.0, 0.0, 0.0}), fast, false, false, 1.0 + dt, dt, 6.0, dt, 0.0, 1, false, Turn{}, out);
        Check(pose.size > 199.0, "one of 200: all of it", pose.size, 199.0);
    }
    // A rate that is not the head's (none at all while the head turns): never used. Before the first verdict it has not shown itself the head's; after, it is told not to be.
    {
        Config c = Parse("shake yaw 35 2\nneck 0 0\nseconds 100000");
        RatePose pose;
        const double none[3] = {};
        int back = 0;
        for (int k = 0; k < 216; ++k) {
            const double t = 1.0 + k * dt;
            const Turn located = HeadAt(c, base, t).turn;
            Turn out;
            back += !pose.Step(located, none, false, false, t, k ? dt : 0.0, 6.0, dt, 0.0, 1, false, located, out);
            if (k % 72 == 71)
                pose.Line(line, sizeof(line), fitLine, sizeof(fitLine), false, false);
        }
        Check(back == 216, "a rate of nothing: never used", back, 216.0);
        Check(std::strstr(fitLine, "NOT the head's rate") != nullptr, "and the fit's line says so");
        Check(std::strstr(line, "72 of 72 frames fell back to the filter beside it (0 with no rate given, 72 with a rate that is not the head's, 0 before the rate had shown itself the head's)") != nullptr, "and the second's line");
    }
    // How uneven the rate given is itself: a shaken head's own rate is smooth; noise on it shows.
    for (const double noise : {0.0, 4.0}) {
        Config c = Parse("shake yaw 35 3\nneck 0 0\nseconds 100000");
        c.rateNoise = noise;
        RatePose pose;
        for (int k = 0; k < 720; ++k) {
            const double t = 1.0 + k * dt;
            double rate[3];
            RateAt(c, base, t, static_cast<uint32_t>(k), rate);
            const Turn located = HeadAt(c, base, t).turn;
            Turn out;
            pose.Step(located, rate, false, false, t, k ? dt : 0.0, 6.0, dt, 0.0, 1, false, located, out);
        }
        const double uneven = pose.unevenSum / pose.unevenOf;
        if (noise == 0.0)
            Check(uneven < 0.6, "a head shaken 35 degrees three times a second: its rate is under 0.6 degrees a second off its own curve", uneven, 0.6);
        else
            Near(uneven, 2.22 * noise, 1.0, "a rate off by 4 degrees a second an axis reads about 9");
        pose.Line(line, sizeof(line), fitLine, sizeof(fitLine), false, false);
        Check(std::strstr(line, "degrees a second off its own curve on average") != nullptr, "and the second's line says it");
    }
    // Located ahead already ("poseahead"): led that much less. As the head itself has it: turned into the room's.
    {
        RatePose pose;
        pose.fit.told = true;
        Turn out;
        const double rate[3] = {0.0, 100.0 * kPi / 180.0, 0.0};
        pose.Step(FromAngles({0.0, 0.0, 0.0}), rate, false, false, 1.0, 0.0, 0.0, 0.020, 5.0, 1, false, Turn{}, out);
        Near(ToAngles(out).yaw, 100.0 * 0.015 * (10000.0 / 10064.0), 1e-6, "located 5 ms ahead already: led 15 of a frame's 20 ms (the rate less its knee's share)");
        RatePose own;
        own.fit.told = true;
        const double tilted[3] = {0.0, 100.0 * kPi / 180.0 * std::cos(0.5), -100.0 * kPi / 180.0 * std::sin(0.5)};  // about the room's up, as a head pitched 0.5 radians up has it
        own.Step(FromAngles({0.0, 0.5 * 180.0 / kPi, 0.0}), tilted, true, false, 1.0, 0.0, 0.0, 0.020, 0.0, 1, false, Turn{}, out);
        Near(ToAngles(out).yaw, 100.0 * 0.020 * (10000.0 / 10064.0), 1e-6, "a rate as the head itself has it: about the room's up once turned into the room's axes");
    }
}

void TestOffCurve() {
    Near(OffCurve(1.0, 8.0, 27.0, 64.0, 125.0), 0.0, 1e-12, "a cubic's middle point lies on the cubic through its neighbours");
    Near(OffCurve(0.3, -0.3, 0.3, -0.3, 0.3), 0.8, 1e-12, "a zigzag of 0.3 each way reads 0.8");
    double worst = 0.0;
    for (int k = 0; k < 200; ++k) {
        auto at = [&](int i) { return 35.0 * std::sin(2.0 * kPi * 3.0 * (k + i) / 72.0); };
        worst = std::max(worst, OffCurve(at(-2), at(-1), at(0), at(1), at(2)));
    }
    Near(worst, 35.0 * std::pow(2.0 * kPi * 3.0 / 72.0, 4.0) / 6.0, 0.001, "a shake of 35 degrees three times a second reads 0.03 at its turning points (and nothing at its fastest)");
}

// ---- Which view build a picture is named by (weapon.txt "nameby"), the world's turn and the edge ----
// A picture's uploads as they were counted on 2026-11-12 with the head turning: of each eye's main views a few carry the build before the newest (here two before
// its others and two after them), the rest the newest. old and now: the two builds; eyes: whether the eyes' pictures are marked.
Naming Mixed(uint32_t old, uint32_t now, bool eyes) {
    Naming n;
    for (int eye = 0; eye < 2; ++eye) {
        n.Add(old), n.Add(old);
        for (int i = 0; i < 7; ++i)
            n.Add(now);
        n.Add(old), n.Add(old);
        if (eyes)
            n.EyePicture();
    }
    return n;
}

void TestNaming() {
    {
        const Naming none;
        Check(none.Named(0, 41) == 41 && none.Named(1, 41) == 41 && none.Named(2, 41) == 41, "no main-view upload to go by: the first upload's rule names it, whatever the rule");
        Check(none.Most() == 0 && none.All() == 0 && none.Builds() == 0, "and nothing is counted");
    }
    {
        const Naming n = Mixed(41, 42, true);
        Check(n.first == 41 && n.last == 41, "a mixed picture: its first and its last upload carry the build before");
        Check(n.lastBeforeEyes == 41, "and so does its last before the first eye's picture");
        Check(n.Of(41) == 8 && n.Of(42) == 14 && n.All() == 22 && n.Builds() == 2, "eight carry the build before, fourteen the newest");
        Check(n.Of(41, 0) == 4 && n.Of(42, 0) == 7 && n.Of(41, 1) == 4 && n.Of(42, 1) == 7 && n.Of(41, 2) == 0 && n.Of(42, 2) == 0, "counted before the first eye's picture and between the two");
        Check(n.Most() == 42, "most carry the newest");
        Check(n.Named(0, 41) == 41, "rule 0: by its first upload, as ever: the build before");
        Check(n.Named(1, 41) == 41, "rule 1: by its last before the first eye's picture: here the build before too");
        Check(n.Named(2, 41) == 42, "rule 2: by the build most carried: the newest");
        Check(n.eyes == 2, "both eyes' pictures marked");
    }
    {
        // The few that carry the build before all at the frame's start and end, the eyes' own all the newest: rule 1 then names the newest.
        Naming n;
        for (int i = 0; i < 4; ++i)
            n.Add(41);
        for (int i = 0; i < 7; ++i)
            n.Add(42);
        n.EyePicture();
        for (int i = 0; i < 7; ++i)
            n.Add(42);
        n.EyePicture();
        for (int i = 0; i < 4; ++i)
            n.Add(41);
        Check(n.lastBeforeEyes == 42 && n.last == 41, "the build before only before the left eye's passes and after the right eye's: the last before the first eye's picture is the newest");
        Check(n.Named(1, 41) == 42 && n.Named(2, 41) == 42 && n.Named(0, 41) == 41, "rules 1 and 2 name the newest there, rule 0 the build before");
        Check(n.Of(41, 0) == 4 && n.Of(41, 1) == 0 && n.Of(41, 2) == 4 && n.Of(42, 1) == 7, "counted in their three stretches");
    }
    {
        const Naming n = Mixed(41, 42, false);
        Check(n.eyes == 0 && n.lastBeforeEyes == 0, "the eyes' pictures not marked");
        Check(n.Named(1, 41) == n.last, "rule 1 then goes by the frame's last upload");
        Check(n.Of(41, 0) == 8 && n.Of(42, 0) == 14, "and all its uploads count as before the first eye's picture");
    }
    {
        Naming n;
        n.EyePicture();
        n.Add(42), n.Add(42), n.Add(41);
        Check(n.lastBeforeEyes == 0 && n.Named(1, 40) == 41, "nothing before the first eye's picture: rule 1 goes by the frame's last");
        Check(n.Of(42, 1) == 2 && n.Of(41, 1) == 1, "what came after it is counted as between the two eyes' pictures");
        n.EyePicture(), n.EyePicture();
        n.Add(42);
        Check(n.eyes == 2 && n.Of(42, 2) == 1, "a third mark is none: after the second eye's picture is the last stretch");
    }
    {
        Naming n;
        for (int i = 0; i < 20; ++i)
            n.Add(42);
        Check(n.Named(0, 42) == 42 && n.Named(1, 42) == 42 && n.Named(2, 42) == 42 && n.Builds() == 1, "a picture all of the newest: every rule names it");
        Naming tie;
        tie.Add(41), tie.Add(42), tie.Add(42), tie.Add(41);
        Check(tie.Most() == 42, "as many of each: the newer");
        Naming older;
        older.Add(40), older.Add(40), older.Add(40), older.Add(42);
        Check(older.Most() == 40 && older.Named(2, 42) == 40, "most carried a build two back (a frame dropped): rule 2 names that one");
        Naming zero;
        zero.Add(0);
        Check(zero.All() == 0 && zero.first == 0, "no build is none");
    }
}

// A view build's camera rows as views.cpp makes them with the look written (viewturn.h PureRows: the body's yaw as the game reads one, then the head), and the head's
// turn as the build keeps it (floats).
void BuildRows(double bodyDegrees, const Turn& head, float* rows, Turn& kept) {
    camera::ViewTurn turn{};
    turn.head[0] = static_cast<float>(head.x), turn.head[1] = static_cast<float>(head.y), turn.head[2] = static_cast<float>(head.z), turn.head[3] = static_cast<float>(head.w);
    turn.lookYaw = static_cast<float>(bodyDegrees * kPi / 180.0);
    float m[16] = {};
    viewturn::PureRows(m, turn);
    for (int r = 0; r < 3; ++r)
        for (int i = 0; i < 3; ++i)
            rows[3 * r + i] = m[4 * r + i];
    kept = {turn.head[0], turn.head[1], turn.head[2], turn.head[3]};
}

void TestWorldTurn() {
    // Two view builds a frame apart with the head turning 500 degrees a second (6.9 degrees) and nodding a little.
    float before[9], newest[9];
    Turn headBefore, headNewest;
    BuildRows(48.33, FromAngles({20.0, -5.0, 2.0}), before, headBefore);
    BuildRows(48.33, FromAngles({26.9, -4.0, 2.5}), newest, headNewest);
    const double step = Between(headBefore, headNewest);
    Check(step > 6.5 && step < 7.5, "the two heads are about 7 degrees apart", step, 7.0);
    Near(CameraApart(newest, CameraOfPose(newest, headNewest, headNewest)), 0.0, 0.002, "told the pose it was drawn with: nothing (it prints 0.00)");
    Near(CameraApart(before, CameraOfPose(before, headBefore, headBefore)), 0.0, 0.002, "the same of the build before");
    Near(CameraApart(newest, CameraOfPose(before, headBefore, headNewest)), 0.0, 0.002, "a pose made a camera by another build's mapping of the same body: the same camera");
    Near(CameraApart(newest, CameraOfPose(before, headBefore, headBefore)), step, 0.002, "drawn with the newest cameras and told the pose of the build before: the head's turn in a frame");
    Near(CameraApart(before, CameraOfPose(newest, headNewest, headNewest)), step, 0.002, "and the other way round");
    // weapon.txt "posetest 2": the layer told a pose turned 2 degrees about the room's up.
    const Turn tested = Mul(FromAngles({2.0, 0.0, 0.0}), headNewest);
    Near(CameraApart(newest, CameraOfPose(newest, headNewest, tested)), 2.0, 0.002, "the layer's pose turned 2 degrees by the test: 2");
    // The stick has turned the body between the two builds. The pose told is made a camera by the mapping of the build the cameras were drawn with (as
    // xr_runtime.cpp does it): what is left is the head's turn between the two poses, which is all the headset gets wrong. By the mapping of the build the pose
    // belongs to it would be the whole turn between the two cameras, the stick's with it.
    float turned[9];
    Turn headTurned;
    BuildRows(78.33, FromAngles({26.9, -4.0, 2.5}), turned, headTurned);
    Near(CameraApart(turned, CameraOfPose(turned, headTurned, headTurned)), 0.0, 0.002, "the body turned by the stick: told its own pose, nothing");
    Near(CameraApart(turned, CameraOfPose(turned, headTurned, headBefore)), step, 0.002,
         "told the pose of the build before the stick's turn, by the body the cameras were drawn with: the head's turn between the two poses alone");
    Near(CameraApart(turned, CameraOfPose(before, headBefore, headBefore)), Between(RowsTurn(before), RowsTurn(turned)), 0.002,
         "by the body of the build the pose belongs to: the whole turn between the two cameras");
    Check(std::fabs(Between(RowsTurn(before), RowsTurn(turned)) - step) > 5.0, "which is not the head's turn: the stick's 30 degrees are in it");
    // A build turned the other way (the look not written: the hand's yaw and the game's pitch taken out of the game's camera, the head put in): the same holds.
    float game[16] = {};
    {
        camera::ViewTurn aim{};
        const Turn pitched = FromAngles({0.0, 12.0, 0.0});
        aim.head[0] = static_cast<float>(pitched.x), aim.head[1] = static_cast<float>(pitched.y), aim.head[2] = static_cast<float>(pitched.z), aim.head[3] = static_cast<float>(pitched.w);
        aim.lookYaw = 1.1f;
        viewturn::PureRows(game, aim);  // (a camera with a yaw and a pitch, as the game's own)
    }
    float rows[2][9];
    Turn heads[2] = {FromAngles({-12.0, 3.0, 0.0}), FromAngles({-5.5, 2.0, 1.0})};
    for (int k = 0; k < 2; ++k) {
        camera::ViewTurn turn{};
        turn.head[0] = static_cast<float>(heads[k].x), turn.head[1] = static_cast<float>(heads[k].y), turn.head[2] = static_cast<float>(heads[k].z), turn.head[3] = static_cast<float>(heads[k].w);
        turn.controllerYaw = 0.3f;
        heads[k] = {turn.head[0], turn.head[1], turn.head[2], turn.head[3]};
        float m[16];
        std::memcpy(m, game, sizeof(m));
        viewturn::Level level;
        viewturn::Turn(m, turn, false, 1, false, level);
        for (int r = 0; r < 3; ++r)
            for (int i = 0; i < 3; ++i)
                rows[k][3 * r + i] = m[4 * r + i];
    }
    Near(CameraApart(rows[1], CameraOfPose(rows[0], heads[0], heads[1])), 0.0, 0.002, "a build turned from the game's own camera: another pose by its mapping is that pose's build");
    Near(CameraApart(rows[1], CameraOfPose(rows[0], heads[0], heads[0])), Between(heads[0], heads[1]), 0.002, "and told the other's pose, the heads' turn");
}

void TestMeters() {
    char line[900];
    WorldSecond none;
    Check(none.Line(line, sizeof(line)) == 0, "a second with no picture has no line");
    none.AddNone(), none.AddNone();
    none.Line(line, sizeof(line));
    Check(std::strstr(line, "World turn: none of the second's 2 pictures had a main-view upload to go by") != nullptr, "pictures with nothing to go by are said so");
    WorldSecond w;
    for (int i = 0; i < 70; ++i)
        w.Add(0.001, 0.001, true, 0.001, false, false);
    w.Add(6.9, 0.0, true, 0.0, true, false);
    w.Add(3.6, 0.0, false, -1.0, true, true);
    w.AddNone();
    const int n = w.Line(line, sizeof(line));
    Check(n > 0 && n < static_cast<int>(sizeof(line)) - 1, "the world's line fits", n, 0.0);
    Check(std::strstr(line, "World turn: the headset is told a pose 0.15 degrees from the cameras the picture was drawn with on average, 6.90 at the worst; 2 frames over 0.05 (of 72; by "
                            "the view build most of each picture's main-view uploads carried)") == line,
          "the world's turn: its mean, its worst, the frames over 0.05");
    Check(std::strstr(line, "by its last upload before the first eye's picture went out 0.00 and 0.00 (1 of them by the frame's last: that moment not marked), by its first 0.00 and 0.00; 2 "
                            "pictures with main-view uploads of more than one view build; 1 more with no main-view upload to go by; the gun and arms placed for another view build than "
                            "the picture was named by in 1") != nullptr,
          "and against its last upload before the first eye's picture and its first, the mixed pictures, those with none, the hands");
    w.Clear();
    Check(w.frames == 0 && w.Line(line, sizeof(line)) == 0, "cleared, it starts again");
    WorldSecond right;
    for (int i = 0; i < 72; ++i)
        right.Add(0.0004, 6.9, true, 6.9, true, false);
    right.Line(line, sizeof(line));
    Check(std::strstr(line, "told a pose 0.00 degrees from the cameras the picture was drawn with on average, 0.00 at the worst; 0 frames over 0.05") != nullptr &&
              std::strstr(line, "went out 6.90 and 6.90, by its first 6.90 and 6.90; 72 pictures with") != nullptr,
          "named by the build most carried, mixed pictures read 0.00 by it and the head's step by their first and last");

    EdgeSecond still;
    for (int i = 0; i < 72; ++i)
        still.Add({0.2, 0.1, 0.0}, 0.22, 12.0);
    Check(still.Line(line, sizeof(line)) == 0 && still.frames == 72 && still.turning == 0, "the edge: a head turning under 30 degrees a second says nothing");
    EdgeSecond few;
    for (int i = 0; i < 11; ++i)
        few.Add({1.0, 0.0, 0.0}, 1.0, 200.0);
    Check(few.Line(line, sizeof(line)) == 0, "nor do fewer than 12 frames of turning");
    EdgeSecond e;
    for (int i = 1; i <= 20; ++i)
        e.Add({-static_cast<double>(i), 0.5 * i, 0.0}, 1.5 * i, 100.0 + i);
    for (int i = 0; i < 52; ++i)
        e.Add({40.0, 40.0, 0.0}, 50.0, 5.0);
    const int m = e.Line(line, sizeof(line));
    Check(m > 0 && m < static_cast<int>(sizeof(line)) - 1, "the edge's line fits", m, 0.0);
    Check(std::strstr(line, "Edge: the picture 15.75 degrees from the head as located for the frame it is shown in on average, 28.50 at 95%, 30.00 at the worst; in yaw 10.50, 19.00 and "
                            "20.00, in pitch 5.25, 9.50 and 10.00; over the 20 of the second's 72 frames in which the head turned more than 30 degrees a second (up to 120)") != nullptr,
          "the edge: the mean, 95% and worst of the whole, the yaw and the pitch, over the frames of turning alone");
    EdgeSecond many;
    for (int i = 0; i < 400; ++i)
        many.Add({1.0, 1.0, 0.0}, 1.4, 90.0);
    Check(many.turning == EdgeSecond::kMost && many.frames == 400 && many.Line(line, sizeof(line)) > 0, "more frames than it holds: the first of them kept");

    // The frame's line with the world's turn at its end.
    Frame f;
    f.world = 0.0, f.worldLast = 6.9, f.worldFirst = 6.9012, f.mixed = true;
    FrameLine(f, line, sizeof(line));
    Check(std::strstr(line, " | by fixed 0.0 | world 0.0000 last 6.9000 first 6.9012 mixed") != nullptr, "the frame's line ends with the world's turn: by the most, by the last, by the first");
    f.worldLast = -1.0, f.worldFirst = -1.0, f.mixed = false, f.world = 0.0123;
    FrameLine(f, line, sizeof(line));
    Check(std::strstr(line, " | world 0.0123 last - first -") != nullptr && std::strstr(line, "mixed") == nullptr, "what is not known is a dash");
    f.world = -1.0;
    FrameLine(f, line, sizeof(line));
    Check(std::strstr(line, "world") == nullptr, "a picture with no main-view upload has no such part");
}

// ---- The filters run on a motion with no game (--sim): how far each one's pictures slide off the head, and how rough their poses are ----
struct SimWhat {
    const char* name;
    const char* tag;
    int kind;      // 0 the located head as it comes, 1 the fixed filter, 2 "poseadapt 1", 3 "poserate 1", 4 the smooth head (the aim's and the walk's), 5 the true head
    uint32_t age;  // the pictures shown that many frames after their pose
    float hz;      // "posefilter"
    double late, noise;  // "poserate": the rate given that many ms late and off by that many degrees a second an axis (on top of the motion's own "rate" line)
    bool dead;     // ... or as none at all
    bool makeUp;   // "poserate 2": the rate carried on for as late as it is given
};

int SpeedBin(double s) {
    return s < 30.0 ? 0 : s < 100.0 ? 1 : s < 300.0 ? 2 : 3;
}

struct SimStat {
    // By the true head's speed (under 30, 30 to 100, 100 to 300, over 300 degrees a second), the yaw (0) and the pitch (1) each by its own.
    std::vector<double> slide[2][4], rough[2][4];
    int back[2][4] = {}, steps[2][4] = {};
};

double Middle(std::vector<double> v, double share) {
    if (v.empty())
        return 0.0;
    std::sort(v.begin(), v.end());
    return v[std::min(v.size() - 1, static_cast<size_t>(share * (v.size() - 1) + 0.5))];
}

double Mean(const std::vector<double>& v) {
    double sum = 0.0;
    for (const double d : v)
        sum += d;
    return v.empty() ? 0.0 : sum / v.size();
}

bool ReadFile(const std::string& path, std::string& text) {
    FILE* f = nullptr;
    if (fopen_s(&f, path.c_str(), "rb") != 0 || !f)
        return false;
    text.clear();
    char block[4096];
    for (size_t n = 0; (n = fread(block, 1, sizeof(block), f)) > 0;)
        text.append(block, n);
    fclose(f);
    return true;
}

// A motion from headtest.txt's lines (";" between them), its track and located rows read from the folder given when they are not found as named.
bool SimMotion(const char* lines, const std::string& folder, Config& c) {
    std::string text(lines);
    for (char& ch : text)
        if (ch == ';')
            ch = '\n';
    c = Parse(text.c_str());
    if (c.kind == Kind::kTrack) {
        std::string rows;
        if (!ReadFile(c.trackFile, rows) && !ReadFile(folder + c.trackFile, rows)) {
            std::printf("cannot read the track %s\n", c.trackFile.c_str());
            return false;
        }
        if (!ParseTrack(rows.c_str(), c)) {
            std::printf("%s: no track of two rows or more\n", c.trackFile.c_str());
            return false;
        }
        if (!c.locatedFile.empty()) {
            if ((!ReadFile(c.locatedFile, rows) && !ReadFile(folder + c.locatedFile, rows)) || !ParseLocated(rows.c_str(), c)) {
                std::printf("cannot read the located rows %s\n", c.locatedFile.c_str());
                return false;
            }
        }
        if (c.seconds == 60.0)
            c.seconds = c.track.back().ms * 1e-3;  // (once through, without a seconds line)
    }
    if (!c.Moves()) {
        std::printf("no motion in \"%s\"\n", lines);
        return false;
    }
    return true;
}

// One motion through one filter, 72 frames a second, as the game feeds it: its numbers added to stat, and its frames written in the log's own lines if asked.
void SimOne(const Config& motion, const SimWhat& what, SimStat& stat, FILE* log) {
    Config c = motion;
    c.rateLateMs += what.late;
    c.rateNoise = std::sqrt(c.rateNoise * c.rateNoise + what.noise * what.noise);
    const Pose base = Standing();
    const int frames = static_cast<int>((c.seconds + kEase) * 72.0) + 1, age = static_cast<int>(what.age);
    std::vector<Pose> truth(frames), located(frames);
    std::vector<Turn> pose(frames), smoothed(frames);
    std::vector<double> rateSize(frames);
    std::vector<By> by(frames, what.kind == 2 ? kByAdapt : kByFixed);
    if (log)
        std::fprintf(log, "[10:00:00.000] Head test: started (offline, %s): %s\n", what.name, Describe(c).c_str());
    FilterModel model;
    SmoothModel smooth;
    RatePose carried;
    carried.TakeAsShown();  // (as a head replay's rate is taken in the game: the made-up head's own)
    for (int k = 0; k < frames; ++k) {
        const double t = k / 72.0;
        truth[k] = HeadAt(c, base, t);
        located[k] = LocatedAt(c, base, t, static_cast<uint32_t>(k));
        const Q4 q = ToQ4(located[k].turn);
        const float seconds = k ? kFrame : 0.0f;
        smoothed[k] = ToTurn(smooth.Step(q, seconds, what.hz));
        Q4 f = q;
        if (what.kind == 1 || what.kind == 2) {
            f = model.Step(q, seconds, false, what.hz, 80.0f, what.kind == 2 ? 1.0f : 0.0f, what.age);
        } else if (what.kind == 3) {
            // As xr_runtime.cpp FilterViews runs it: the fixed filter beside it, its turn kept where the rate cannot be used; the lines' verdicts once a second.
            f = model.Step(q, seconds, false, what.hz, 80.0f, 0.0f, what.age);
            double rate[3] = {};
            if (!what.dead)
                RateAt(c, base, t, static_cast<uint32_t>(k), rate);
            Turn out;
            if (carried.Step(ToTurn(q), rate, c.rateOwn, what.makeUp, t, seconds, what.hz, 1.0 / 72.0, 0.0, what.age, false, ToTurn(f), out))
                f = ToQ4(out);
            by[k] = carried.back ? kByFallback : kByRate;
            rateSize[k] = carried.size;
            if (k % 72 == 71) {
                char line[700], fitLine[700];
                carried.Line(line, sizeof(line), fitLine, sizeof(fitLine), c.rateOwn, what.makeUp);
                if (log)
                    std::fprintf(log, "[10:00:00.000] %s\n%s%s", line, fitLine, fitLine[0] ? "\n" : "");
            }
        } else if (what.kind == 4) {
            f = ToQ4(smoothed[k]);
        } else if (what.kind == 5) {
            f = ToQ4(truth[k].turn);
        }
        pose[k] = ToTurn(f);
    }
    // The angles, the yaw without its jumps at +-180.
    std::vector<double> yaw(frames), pitch(frames), trueYaw(frames), truePitch(frames);
    for (int k = 0; k < frames; ++k) {
        const Angles a = ToAngles(pose[k]), b = ToAngles(truth[k].turn);
        yaw[k] = k ? yaw[k - 1] + Wrap(a.yaw - yaw[k - 1]) : a.yaw;
        trueYaw[k] = k ? trueYaw[k - 1] + Wrap(b.yaw - trueYaw[k - 1]) : b.yaw;
        pitch[k] = a.pitch, truePitch[k] = b.pitch;
    }
    // Judged as tools\headfit.ps1 judges a run: the first half second and the ease back left out. The picture shown in frame k has the pose of frame k - age
    // (the smooth head is held against the head of its own frame: how far behind the head the aim's limit and the walk are).
    const int from = 36 + age + 2, to = std::min(frames - 3, static_cast<int>(c.seconds * 72.0) - 2);
    const int old = what.kind >= 4 ? 0 : age;
    for (int k = from; k <= to; ++k) {
        const Angles slide = Slide(pose[k - old], truth[k].turn);
        for (int axis = 0; axis < 2; ++axis) {
            const std::vector<double>& head = axis ? truePitch : trueYaw;
            const std::vector<double>& own = axis ? pitch : yaw;
            const int bin = SpeedBin((std::fabs(head[k] - head[k - 1]) + std::fabs(head[k + 1] - head[k])) / 2.0 * 72.0);
            stat.slide[axis][bin].push_back(std::fabs(axis ? slide.pitch : slide.yaw));
            const int m = k - old;
            stat.rough[axis][bin].push_back(OffCurve(own[m - 2], own[m - 1], own[m], own[m + 1], own[m + 2]));
            const double before = own[m - 1] - own[m - 2], step = own[m] - own[m - 1], after = own[m + 1] - own[m];
            ++stat.steps[axis][bin];
            if (before * after > 0.0 && step * before < 0.0 && std::fabs(step) > 0.1)
                ++stat.back[axis][bin];
        }
    }
    if (!log)
        return;
    char line[900];
    for (int k = age; k < frames; ++k) {
        Frame fr;
        fr.ms = k * 1000.0 / 72.0;
        fr.truth = truth[k];
        fr.located = located[k];
        fr.filtered.turn = pose[k];
        fr.picture.turn = pose[k - age];
        for (int i = 0; i < 3; ++i)
            fr.filtered.place[i] = located[k].place[i], fr.picture.place[i] = located[k - age].place[i];
        fr.age = what.age;
        fr.how = kNamed;
        fr.pose = static_cast<uint32_t>(100 + k - age);
        fr.smooth = what.kind == 0 || what.kind == 1 ? pose[k] : smoothed[k];
        fr.by = by[k];
        fr.rate = rateSize[k];
        FrameLine(fr, line, sizeof(line));
        const int ms = static_cast<int>(fr.ms);
        std::fprintf(log, "[10:%02d:%02d.%03d] %s\n", ms / 60000, ms / 1000 % 60, ms % 1000, line);
    }
    std::fprintf(log, "[10:59:59.000] Head test: ended after %.1f s: the head is back where it was\n", c.seconds);
}

int Sim(int argc, char** argv) {
    std::string folder(argv[0]);
    const size_t slash = folder.find_last_of("\\/");
    folder = slash == std::string::npos ? std::string() : folder.substr(0, slash + 1);
    std::vector<Config> motions;
    std::string logTo;
    for (int i = 2; i < argc; ++i) {
        if (std::strcmp(argv[i], "--log") == 0 && i + 1 < argc) {
            logTo = argv[++i];
            continue;
        }
        Config c;
        if (!SimMotion(argv[i], folder, c))
            return 1;
        motions.push_back(c);
    }
    if (motions.empty()) {
        // The user's own head of 2026-11-12, three stretches of it: a smooth true head, and the head as it was located.
        for (const char* name : {"b1", "b2", "b3"}) {
            const std::string lines = std::string("track headtracks\\user_") + name + ".txt;located headtracks\\user_" + name + "_located.txt";
            Config c;
            if (!SimMotion(lines.c_str(), folder, c))
                return 1;
            motions.push_back(c);
        }
    }
    static const SimWhat kRuns[] = {
        {"the located head as it comes", "located", 0, 1, 6.0f, 0.0, 0.0, false, false},
        {"the fixed filter (posefilter 6, poselead 80)", "fixed", 1, 1, 6.0f, 0.0, 0.0, false, false},
        {"poseadapt 1", "adapt", 2, 1, 6.0f, 0.0, 0.0, false, false},
        {"poserate 1", "rate", 3, 1, 6.0f, 0.0, 0.0, false, false},
        {"poserate 1, the rate 7 ms late", "rate_late7", 3, 1, 6.0f, 7.0, 0.0, false, false},
        {"poserate 1, the rate 14 ms late", "rate_late14", 3, 1, 6.0f, 14.0, 0.0, false, false},
        {"poserate 1, the rate 28 ms late", "rate_late28", 3, 1, 6.0f, 28.0, 0.0, false, false},
        {"poserate 2, the rate 14 ms late", "rate2_late14", 3, 1, 6.0f, 14.0, 0.0, false, true},
        {"poserate 2, the rate 28 ms late", "rate2_late28", 3, 1, 6.0f, 28.0, 0.0, false, true},
        {"poserate 1, the rate off by 3 deg/s", "rate_noise3", 3, 1, 6.0f, 0.0, 3.0, false, false},
        {"poserate 1, the rate off by 10 deg/s", "rate_noise10", 3, 1, 6.0f, 0.0, 10.0, false, false},
        {"poserate 2, the rate 28 ms late, off by 3 deg/s", "rate2_late28_noise3", 3, 1, 6.0f, 28.0, 3.0, false, true},
        {"poserate 1, posefilter 12", "rate_hz12", 3, 1, 12.0f, 0.0, 0.0, false, false},
        {"poserate 1, posefilter 12, the rate 14 ms late", "rate_hz12_late14", 3, 1, 12.0f, 14.0, 0.0, false, false},
        {"poserate 1, no rate at all (falls back)", "rate_dead", 3, 1, 6.0f, 0.0, 0.0, true, false},
        {"the fixed filter", "fixed_2", 1, 2, 6.0f, 0.0, 0.0, false, false},
        {"poseadapt 1", "adapt_2", 2, 2, 6.0f, 0.0, 0.0, false, false},
        {"poserate 1", "rate_2", 3, 2, 6.0f, 0.0, 0.0, false, false},
        {"poserate 1, the rate 14 ms late", "rate_2_late14", 3, 2, 6.0f, 14.0, 0.0, false, false},
        {"the smooth head (the aim's and the walk's)", "smooth", 4, 1, 6.0f, 0.0, 0.0, false, false},
        {"the true head itself", "true", 5, 1, 6.0f, 0.0, 0.0, false, false},
    };
    const int runs = static_cast<int>(sizeof(kRuns) / sizeof(kRuns[0]));
    std::vector<SimStat> stats(runs);
    double seconds = 0.0;
    for (size_t m = 0; m < motions.size(); ++m) {
        std::printf("motion %zu: %s\n", m + 1, Describe(motions[m]).c_str());
        seconds += motions[m].seconds;
        for (int r = 0; r < runs; ++r) {
            FILE* log = nullptr;
            if (!logTo.empty()) {
                char path[600];
                std::snprintf(path, sizeof(path), "%s_%s_%zu.log", logTo.c_str(), kRuns[r].tag, m + 1);
                if (fopen_s(&log, path, "w") != 0)
                    log = nullptr;
            }
            SimOne(motions[m], kRuns[r], stats[r], log);
            if (log)
                fclose(log);
        }
    }
    static const char* const kBins[] = {"under 30", "30 to 100", "100 to 300", "over 300"};
    std::printf("\n%.0f s of head at 72 frames a second. Degrees. slide: the picture's pose from the true head when the picture is shown (mean, 90%% of frames under). rough: the\n"
                "picture's pose off the curve through its four neighbours (median, 90%%); back: frames stepping back against the motion either side of them. By the true\n"
                "head's speed in degrees a second. The marks at 100 to 300: rough 0.06 is the fixed filter as the user accepts it, 0.6 was \"really bad\".\n",
                seconds);
    for (int axis = 0; axis < 2; ++axis) {
        std::printf("\n%s\n%-50s", axis ? "PITCH" : "YAW", "");
        for (int b = 1; b < 4; ++b)
            std::printf(" | %-10s frames  slide        rough        back", kBins[b]);
        std::printf("\n");
        for (int r = 0; r < runs; ++r) {
            if (r == 0 || (kRuns[r].kind < 4 && kRuns[r].age != kRuns[r - 1].age) || kRuns[r].kind == 4)
                std::printf("%s\n", kRuns[r].kind == 4 ? "against the head of its own frame:" : kRuns[r].age == 1 ? "pictures shown one frame after their pose:" : "pictures shown two frames after their pose:");
            std::printf("  %-48s", kRuns[r].name);
            for (int b = 1; b < 4; ++b) {
                const SimStat& s = stats[r];
                std::printf(" | %17zu  %5.2f %5.2f  %5.3f %5.2f  %4.1f%%", s.slide[axis][b].size(), Mean(s.slide[axis][b]), Middle(s.slide[axis][b], 0.9), Middle(s.rough[axis][b], 0.5),
                            Middle(s.rough[axis][b], 0.9), s.steps[axis][b] ? 100.0 * s.back[axis][b] / s.steps[axis][b] : 0.0);
            }
            std::printf("\n");
        }
    }
    // What "poserate" has to hold: no rougher than about twice the fixed filter, and sliding no more than "poseadapt" (the yaw, each speed; pictures a frame old).
    const SimStat& fixed = stats[1];
    const SimStat& adapt = stats[2];
    const SimStat& rate = stats[3];
    int failed = 0;
    std::printf("\n");
    for (int b = 1; b < 4; ++b) {
        const double roughRate = Middle(rate.rough[0][b], 0.5), roughFixed = Middle(fixed.rough[0][b], 0.5), slideRate = Mean(rate.slide[0][b]), slideAdapt = Mean(adapt.slide[0][b]);
        const bool roughOk = roughRate <= 2.2 * roughFixed + 0.005, slideOk = slideRate <= slideAdapt + 0.05;
        failed += !roughOk + !slideOk;
        std::printf("%s: poserate rough %.3f against the fixed filter's %.3f (%.1f times): %s; its slide %.2f against poseadapt's %.2f: %s\n", kBins[b], roughRate, roughFixed,
                    roughFixed > 0.0 ? roughRate / roughFixed : 0.0, roughOk ? "holds" : "TOO ROUGH", slideRate, slideAdapt, slideOk ? "holds" : "SLIDES MORE");
    }
    return failed;
}

}

int main(int argc, char** argv) {
    if (argc >= 3 && std::strcmp(argv[1], "--synth") == 0)
        return Synth(argv[2]);
    if (argc >= 3 && std::strcmp(argv[1], "--track") == 0)
        return Track(argv[2]);
    if (argc >= 2 && std::strcmp(argv[1], "--sim") == 0)
        return Sim(argc, argv);
    TestAngles();
    TestRows();
    TestSlide();
    TestShake();
    TestBurst();
    TestTrack();
    TestNeck();
    TestPredict();
    TestEase();
    TestLevel();
    TestParse();
    TestLines();
    TestNoise();
    TestCurve();
    TestLocated();
    TestNoiseFastAndRate();
    TestRateFilter();
    TestRateFit();
    TestRatePose();
    TestOffCurve();
    TestNaming();
    TestWorldTurn();
    TestMeters();
    TestModels();
    TestTime();
    std::printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed;
}

// The rules of the game's dropped guns (src/pickrules.h, header only) without the game. Each check prints PASS or FAIL, then the totals; the exit
// code is the number failed.
//   T1  DrawnPose: rotation 0 gives the transform; (0 0 88) a roll of 88 degrees about the pickup's own forward axis, its sign as the client's z
//       turn (00557B50) builds it; the offset added in the world whatever the turn; (10 20 0) against a hand-built y then x turn; a NaN: false
//   T2  CountdownFor: the three policies x in view or not x ttl 0 and 180 x raised or not; nothing written with every key at its default
//   T3  InView's edges (70 degrees, 80 m)
//   T4  Fresh: 499 ms yes, 501 ms no
//   T5  SeqLock: a writer thread publishing 100000 payloads whose words all equal their serial, a reader on another: never a torn copy accepted, at
//       least 1000 reads
//   T6  WorldToRoom and WorldPointToRoom far from the world's middle: within 0.1 mm of the same worked out in doubles
//   T7  Landed: a triple changed; only the counts; exactly as before
//   T8  CountdownFor, the player's own: -1 brings it back to its time under 30 s whatever the others say; 0 follows them; N raises once; rate 20
//   T9  StartAtCentre: the hand's turn, the box's middle on the point; a box with no size: false
//   T10 ActiveSlot: one in hand; none; two; a slot with no weapon left out
//   T11 Diff by triples: another blueprint at the same addresses is a change; nothing changed: 0
//   T12 Forgets: its own slot among the swap's; its firing state among those that went; neither
//   T13 OnSwap: every case of its rules
//   T14 Net: the landed take given up, the retired gun given back, the retired copy gone some other way, nothing early
//   T15 the guns' table by the game's own spellings of its blueprints
// Usage: picktest

#include <windows.h>

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <thread>

#include "../src/pickrules.h"
#include "../src/recoilguns.h"

namespace {

int g_failed = 0, g_counted = 0;

void Check(bool ok, const char* row, const char* what) {
    ++g_counted;
    if (!ok)
        ++g_failed;
    std::printf("%s %s %s\n", ok ? "PASS" : "FAIL", row, what);
}

bool Near(float a, float b, float within) {
    return std::fabs(a - b) <= within;
}

bool Near12(const float* a, const float* b, float within) {
    for (int i = 0; i < 12; ++i)
        if (!Near(a[i], b[i], within))
            return false;
    return true;
}

// A pickup's transform (rows right, up, forward, translation) turned by yaw degrees about the world's up, at t.
void Transform(float yawDegrees, const float* t, float* out16) {
    const float a = yawDegrees * 3.14159265f / 180.0f, c = std::cos(a), s = std::sin(a);
    const float m[16] = {c, 0.0f, -s, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, s, 0.0f, c, 0.0f, t[0], t[1], t[2], 1.0f};
    std::memcpy(out16, m, sizeof(m));
}

// Column c of a 3x4 (rows rotation | translation): the model's axis c in the world.
void Axis(const float* m12, int c, double* out) {
    for (int r = 0; r < 3; ++r)
        out[r] = m12[4 * r + c];
}

double Dot(const double* a, const float* b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void T1() {
    const float at[3] = {1260.6f, 65.15f, -637.7f};
    float m[16];
    Transform(30.0f, at, m);
    const float zero[3] = {}, none[3] = {};
    float out[12] = {};
    bool ok = pickrules::DrawnPose(m, zero, none, out);
    bool same = ok;
    for (int r = 0; r < 3 && same; ++r) {
        for (int c = 0; c < 3; ++c)
            same = same && Near(out[4 * r + c], m[4 * c + r], 1e-6f);
        same = same && out[4 * r + 3] == m[12 + r];
    }
    Check(same, "T1", "DrawnPose: rotation 0 and no offset give the transform itself (its rows the columns here)");

    const float roll[3] = {0.0f, 0.0f, 88.0f};
    ok = pickrules::DrawnPose(m, zero, roll, out);
    double right[3], up[3], forward[3];
    Axis(out, 0, right), Axis(out, 1, up), Axis(out, 2, forward);
    const float* mRight = m;
    const float* mUp = m + 4;
    const float* mForward = m + 8;
    const double c88 = std::cos(88.0 * 3.14159274101257 / 180.0), s88 = std::sin(88.0 * 3.14159274101257 / 180.0);
    Check(ok && std::fabs(Dot(forward, mForward) - 1.0) < 1e-6, "T1", "DrawnPose (0 0 88): its forward axis unchanged");
    Check(ok && std::fabs(Dot(right, mRight) - c88) < 1e-6 && std::fabs(Dot(right, mUp) - s88) < 1e-6,
          "T1", "DrawnPose (0 0 88): its right axis turned 88 degrees towards its up (the z turn's first row (c s 0))");
    Check(ok && std::fabs(Dot(up, mUp) - c88) < 1e-6 && std::fabs(Dot(up, mRight) + s88) < 1e-6,
          "T1", "DrawnPose (0 0 88): its up axis turned 88 degrees away from its right (the z turn's second row (-s c 0))");
    Check(ok && out[3] == m[12] && out[7] == m[13] && out[11] == m[14], "T1", "DrawnPose (0 0 88): the translation untouched");

    const float offset[3] = {0.0f, 0.025f, 0.0f};
    float a[12], b[12];
    const bool okA = pickrules::DrawnPose(m, offset, zero, a);
    const bool okB = pickrules::DrawnPose(m, offset, roll, b);
    Check(okA && okB && Near(a[7], m[13] + 0.025f, 1e-4f) && Near(b[7], m[13] + 0.025f, 1e-4f) && a[3] == m[12] && b[3] == m[12] &&
              a[11] == m[14] && b[11] == m[14],
          "T1", "DrawnPose: the offset (0 0.025 0) added in the world (2.5 cm up), turned or not");

    // (10 20 0): the y turn first, then the x turn, each before the transform: Rx Ry M, worked out here by hand in doubles.
    const float xy[3] = {10.0f, 20.0f, 0.0f};
    ok = pickrules::DrawnPose(m, zero, xy, out);
    const double rad = 3.14159274101257 / 180.0, cx = std::cos(10.0 * rad), sx = std::sin(10.0 * rad), cy = std::cos(20.0 * rad),
                 sy = std::sin(20.0 * rad);
    const double ry[3][3] = {{cy, 0, -sy}, {0, 1, 0}, {sy, 0, cy}};
    const double rx[3][3] = {{1, 0, 0}, {0, cx, sx}, {0, -sx, cx}};
    double ryM[3][3], rows[3][3];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            ryM[i][j] = ry[i][0] * m[j] + ry[i][1] * m[4 + j] + ry[i][2] * m[8 + j];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            rows[i][j] = rx[i][0] * ryM[0][j] + rx[i][1] * ryM[1][j] + rx[i][2] * ryM[2][j];
    bool built = ok;
    for (int r = 0; r < 3 && built; ++r)
        for (int c = 0; c < 3; ++c)
            built = built && std::fabs(out[4 * r + c] - rows[c][r]) < 1e-6;
    Check(built, "T1", "DrawnPose (10 20 0): the y turn, then the x turn, in the pickup's own axes, as a hand-built Rx Ry M");
    // And not the other order.
    double other[3][3], rxM[3][3];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            rxM[i][j] = rx[i][0] * m[j] + rx[i][1] * m[4 + j] + rx[i][2] * m[8 + j];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            other[i][j] = ry[i][0] * rxM[0][j] + ry[i][1] * rxM[1][j] + ry[i][2] * rxM[2][j];
    double most = 0.0;
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            most = std::fmax(most, std::fabs(out[4 * r + c] - other[c][r]));
    Check(ok && most > 1e-3, "T1", "DrawnPose (10 20 0): not the x turn first (the order matters, and it's the game's)");

    float bad[16];
    std::memcpy(bad, m, sizeof(bad));
    bad[5] = NAN;
    float untouched[12];
    for (float& v : untouched)
        v = 7.0f;
    const float nanTurn[3] = {0.0f, NAN, 0.0f};
    Check(!pickrules::DrawnPose(bad, zero, zero, untouched) && !pickrules::DrawnPose(m, zero, nanTurn, untouched) && untouched[0] == 7.0f,
          "T1", "DrawnPose: a NaN in the transform or the turn: false, nothing written");
}

void T2() {
    // seconds: 0 the game's own, -1 never, 40; unseen in view or not; ttl 0 and 180; raised already or not. left 100, rate 1 (unless said).
    struct Row {
        int seconds;
        bool unseen, inView;
        float ttl, left;
        bool raised;
        bool write;
        float out;
        bool raisedAfter;
        const char* what;
    };
    const Row rows[] = {
        {0, false, false, 180.0f, 100.0f, false, false, 0.0f, false, "the game's own, nothing else: nothing written"},
        {0, false, true, 180.0f, 100.0f, false, false, 0.0f, false, "the game's own, in view, unseen off: nothing"},
        {0, true, false, 180.0f, 1.0f, false, false, 0.0f, false, "the game's own, unseen on, out of view, 1 s left: nothing (it goes)"},
        {0, true, true, 180.0f, 1.0f, false, true, 3.0f, false, "the game's own, unseen on, in view, 1 s left: held at 3 s (3 x the rate 1)"},
        {0, true, true, 180.0f, 100.0f, false, false, 0.0f, false, "the game's own, unseen on, in view, 100 s left: nothing yet"},
        {-1, false, false, 180.0f, 100.0f, false, false, 0.0f, false, "never, 100 s left: nothing yet"},
        {-1, false, false, 180.0f, 29.0f, false, true, 180.0f, false, "never, 29 s left: back to 180"},
        {-1, true, true, 180.0f, 29.0f, false, true, 180.0f, false, "never, in view, 29 s left: back to 180 (no hold needed)"},
        {40, false, false, 180.0f, 175.0f, false, true, 35.0f, true, "40 s, not raised: 175 + (40 - 180) = 35, raised once"},
        {40, false, false, 180.0f, 100.0f, true, false, 0.0f, true, "40 s, raised already: nothing"},
        {40, true, true, 180.0f, 1.0f, true, true, 3.0f, true, "40 s, raised, unseen in view at 1 s: held at 3 s"},
        {40, false, false, 180.0f, 100.0f, false, true, 0.5f, true, "40 s, 100 s left of 180: 100 - 140 lowered to 0.5 at least"},
        {0, true, true, 0.0f, 1.0f, false, false, 0.0f, false, "ttl 0 (forever), unseen in view: nothing"},
        {-1, false, false, 0.0f, 1.0f, false, false, 0.0f, false, "ttl 0, never: nothing"},
        {40, false, false, 0.0f, 1.0f, false, false, 0.0f, false, "ttl 0, 40 s: nothing, not raised"},
        {40, true, true, 0.0f, 1.0f, true, false, 0.0f, true, "ttl 0, raised, in view: nothing"},
    };
    int i = 0;
    for (const Row& r : rows) {
        bool raised = r.raised;
        float out = -99.0f;
        const bool write = pickrules::CountdownFor(r.seconds, false, -1, r.unseen, r.ttl, r.left, 1.0f, r.inView, raised, out);
        char what[200];
        snprintf(what, sizeof(what), "CountdownFor %d: %s", ++i, r.what);
        Check(write == r.write && (!write || Near(out, r.out, 1e-4f)) && (write || out == -99.0f) && raised == r.raisedAfter, "T2", what);
    }
    // Every key at its default (seconds 0, unseen off, own -1), a pickup not the player's: nothing, however it lies.
    bool none = true;
    for (float left : {0.1f, 1.0f, 29.0f, 100.0f, 179.0f})
        for (bool view : {false, true}) {
            bool raised = false;
            float out = 0.0f;
            none = none && !pickrules::CountdownFor(0, false, -1, false, 180.0f, left, 1.0f, view, raised, out) && !raised;
        }
    Check(none, "T2", "CountdownFor: every key at its default: nothing written for any pickup not the player's");
    bool raised = false;
    float out = 0.0f;
    Check(!pickrules::CountdownFor(-1, false, -1, true, NAN, 1.0f, 1.0f, true, raised, out) &&
              !pickrules::CountdownFor(-1, false, -1, true, 180.0f, NAN, 1.0f, true, raised, out),
          "T2", "CountdownFor: a ttl or a count not finite: nothing");
}

void T3() {
    const float eye[3] = {0.0f, 0.0f, 0.0f}, look[3] = {0.0f, 0.0f, 2.0f};
    auto at = [](float degrees, float distance, float* out) {
        const float a = degrees * 3.14159265358979f / 180.0f;
        out[0] = distance * std::sin(a), out[1] = 0.0f, out[2] = distance * std::cos(a);
    };
    float p[3];
    at(69.5f, 10.0f, p);
    const bool in695 = pickrules::InView(eye, look, p);
    at(70.5f, 10.0f, p);
    const bool in705 = pickrules::InView(eye, look, p);
    Check(in695 && !in705, "T3", "InView: 69.5 degrees off the look in, 70.5 out");
    at(0.0f, 79.9f, p);
    const bool in799 = pickrules::InView(eye, look, p);
    at(0.0f, 80.1f, p);
    const bool in801 = pickrules::InView(eye, look, p);
    Check(in799 && !in801, "T3", "InView: 79.9 m in, 80.1 m out");
    at(180.0f, 1.0f, p);
    const float none[3] = {};
    Check(!pickrules::InView(eye, look, p) && !pickrules::InView(eye, none, look), "T3", "InView: behind: out; no look: out");
}

void T4() {
    Check(pickrules::Fresh(1000, 1499) && !pickrules::Fresh(1000, 1501) && pickrules::Fresh(1000, 1000), "T4",
          "Fresh: 499 ms yes, 501 ms no, 0 yes");
}

struct Payload {
    uint32_t words[256];
};

void T5() {
    static pickrules::SeqLock<Payload> lock;
    std::atomic<bool> done{false};
    std::thread writer([&] {
        static Payload p;
        for (uint32_t serial = 1; serial <= 100000; ++serial) {
            for (uint32_t& w : p.words)
                w = serial;
            lock.Write(p);
        }
        done.store(true);
    });
    int reads = 0, torn = 0;
    uint32_t last = 0;
    bool backwards = false;
    static Payload got;
    while (!done.load() || reads < 1000) {
        if (!lock.Read(got))
            continue;
        ++reads;
        for (uint32_t w : got.words)
            if (w != got.words[0]) {
                ++torn;
                break;
            }
        if (got.words[0] < last)
            backwards = true;
        last = got.words[0];
        if (done.load() && reads >= 1000)
            break;
    }
    writer.join();
    char what[160];
    snprintf(what, sizeof(what), "SeqLock: %d reads accepted while 100000 were written, %d of them torn, none going back", reads, torn);
    Check(torn == 0 && reads >= 1000 && !backwards, "T5", what);
    pickrules::SeqLock<Payload> empty;
    Check(!empty.Read(got), "T5", "SeqLock: nothing written yet: false");
}

void T6() {
    // The room turned 35 degrees about up and moved; the arms' space's origin (1260 65 -638).
    const double a = 35.0 * 3.14159265358979 / 180.0;
    const float armsToRoom[12] = {static_cast<float>(std::cos(a)), 0.0f, static_cast<float>(std::sin(a)), 0.4f,
                                  0.0f, 1.0f, 0.0f, -1.2f,
                                  static_cast<float>(-std::sin(a)), 0.0f, static_cast<float>(std::cos(a)), 0.7f};
    const int origin[3] = {1260, 65, -638};
    const float b = 20.0f * 3.14159265f / 180.0f;
    const float world[12] = {std::cos(b), -std::sin(b), 0.0f, 1260.6f, std::sin(b), std::cos(b), 0.0f, 65.15f, 0.0f, 0.0f, 1.0f, -637.7f};
    float room[12];
    pickrules::WorldToRoom(armsToRoom, origin, world, room);
    double most = 0.0;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            const double want = static_cast<double>(armsToRoom[4 * r]) * world[c] + static_cast<double>(armsToRoom[4 * r + 1]) * world[4 + c] +
                                static_cast<double>(armsToRoom[4 * r + 2]) * world[8 + c];
            most = std::fmax(most, std::fabs(want - room[4 * r + c]));
        }
        const double t[3] = {static_cast<double>(world[3]) - origin[0], static_cast<double>(world[7]) - origin[1], static_cast<double>(world[11]) - origin[2]};
        const double want = armsToRoom[4 * r] * t[0] + armsToRoom[4 * r + 1] * t[1] + armsToRoom[4 * r + 2] * t[2] + armsToRoom[4 * r + 3];
        most = std::fmax(most, std::fabs(want - room[4 * r + 3]));
    }
    char what[160];
    snprintf(what, sizeof(what), "WorldToRoom at (1260.6 65.15 -637.7), origin (1260 65 -638), a turned room: %.4f mm from doubles", most * 1000.0);
    Check(most < 1e-4, "T6", what);
    const float point[3] = {1260.6f, 65.15f, -637.7f};
    float roomPoint[3];
    pickrules::WorldPointToRoom(armsToRoom, origin, point, roomPoint);
    Check(Near(roomPoint[0], room[3], 1e-6f) && Near(roomPoint[1], room[7], 1e-6f) && Near(roomPoint[2], room[11], 1e-6f), "T6",
          "WorldPointToRoom: the same point as the pose's translation");
    float inPlace[12];
    std::memcpy(inPlace, world, sizeof(inPlace));
    pickrules::WorldToRoom(armsToRoom, origin, inPlace, inPlace);
    Check(Near12(inPlace, room, 0.0f), "T6", "WorldToRoom: into the same array it reads from, the same");
}

void T7() {
    pickrules::Triple before[10], after[10];
    before[0] = {0x1000, 0x2000, 0x3000};
    for (int i = 0; i < 10; ++i)
        after[i] = before[i];
    Check(!pickrules::Landed(before, after, 10, 0, 30, 90, 30, 90), "T7", "Landed: the same triples and counts: no (the game swapped nothing)");
    Check(pickrules::Landed(before, after, 10, 0, 30, 90, 31, 90) && pickrules::Landed(before, after, 10, 0, 30, 90, 30, 60), "T7",
          "Landed: only the taken slot's loaded or spare count changed: yes");
    Check(!pickrules::Landed(before, after, 10, -1, 30, 90, 31, 90), "T7", "Landed: no taken slot known, the triples the same: no");
    after[2] = {0x1100, 0x2100, 0x3100};
    Check(pickrules::Landed(before, after, 10, 0, 30, 90, 30, 90), "T7", "Landed: a triple changed (another slot): yes");
}

void T8() {
    bool raised = false;
    float out = 0.0f;
    // -1 for the player's own, whatever seconds says.
    bool ok = true;
    for (int seconds : {0, -1, 40}) {
        raised = true;  // (raised already, so seconds N doesn't move it)
        out = 0.0f;
        ok = ok && pickrules::CountdownFor(seconds, true, -1, false, 180.0f, 29.0f, 1.0f, false, raised, out) && out == 180.0f;
        raised = true;
        ok = ok && !pickrules::CountdownFor(seconds, true, -1, false, 180.0f, 31.0f, 1.0f, false, raised, out);
    }
    Check(ok, "T8", "CountdownFor, the player's own with -1: back to 180 under 30 s, nothing over, whatever seconds says");
    raised = false;
    const bool follows0 = !pickrules::CountdownFor(0, true, 0, false, 180.0f, 29.0f, 1.0f, false, raised, out);
    raised = false;
    const bool followsN = pickrules::CountdownFor(-1, true, 0, false, 180.0f, 29.0f, 1.0f, false, raised, out) && out == 180.0f;
    Check(follows0 && followsN, "T8", "CountdownFor, the player's own with 0: as any other (seconds 0 nothing, -1 back to 180)");
    raised = false;
    const bool once = pickrules::CountdownFor(0, true, 300, false, 180.0f, 170.0f, 1.0f, false, raised, out) && Near(out, 290.0f, 1e-3f) && raised;
    const bool notTwice = !pickrules::CountdownFor(0, true, 300, false, 180.0f, 289.0f, 1.0f, false, raised, out);
    Check(once && notTwice, "T8", "CountdownFor, the player's own with 300: raised once to 170 + 120 = 290, not again");
    raised = false;
    const bool at59 = pickrules::CountdownFor(0, true, -1, false, 180.0f, 59.0f, 20.0f, false, raised, out) && out == 180.0f;
    const bool at61 = !pickrules::CountdownFor(0, true, -1, false, 180.0f, 61.0f, 20.0f, false, raised, out);
    const bool unseen = pickrules::CountdownFor(0, false, -1, true, 180.0f, 59.0f, 20.0f, true, raised, out) && out == 60.0f;
    Check(at59 && at61 && unseen, "T8", "CountdownFor at the rate 20: the player's own back up from 59 s, not 61; the hold in view at 60 s");
    const bool clamped = pickrules::CountdownFor(0, false, -1, true, 180.0f, 1.0f, 1000.0f, true, raised, out) && out == 180.0f &&
                         pickrules::CountdownFor(0, false, -1, true, 180.0f, 1.0f, 0.01f, true, raised, out) && out == 3.0f;
    Check(clamped, "T8", "CountdownFor: the rate taken within 1 to 60 (180 s hold at 1000, 3 s at 0.01)");
}

void T9() {
    const float a = 0.6f;
    const float hand[12] = {std::cos(a), 0.0f, std::sin(a), 9.0f, 0.0f, 1.0f, 0.0f, 9.0f, -std::sin(a), 0.0f, std::cos(a), 9.0f};
    const float lo[3] = {-0.05f, -0.16f, -0.30f}, hi[3] = {0.05f, 0.14f, 0.80f}, centre[3] = {0.3f, 0.1f, 1.2f};
    float out[12];
    bool ok = pickrules::StartAtCentre(hand, lo, hi, centre, out);
    bool turn = ok;
    for (int r = 0; r < 3 && turn; ++r)
        for (int c = 0; c < 3; ++c)
            turn = turn && out[4 * r + c] == hand[4 * r + c];
    Check(turn, "T9", "StartAtCentre: the turn the hand's");
    const float mid[3] = {0.0f, -0.01f, 0.25f};
    float lands[3];
    for (int r = 0; r < 3; ++r)
        lands[r] = out[4 * r] * mid[0] + out[4 * r + 1] * mid[1] + out[4 * r + 2] * mid[2] + out[4 * r + 3];
    Check(ok && Near(lands[0], centre[0], 1e-5f) && Near(lands[1], centre[1], 1e-5f) && Near(lands[2], centre[2], 1e-5f), "T9",
          "StartAtCentre: the box's middle lands on the point given");
    const float flat[3] = {0.05f, -0.16f, 0.80f};
    float untouched[12] = {};
    Check(!pickrules::StartAtCentre(hand, lo, lo, centre, untouched) && !pickrules::StartAtCentre(hand, flat, hi, centre, untouched) &&
              untouched[0] == 0.0f,
          "T9", "StartAtCentre: a box with no size: false, nothing written");
}

void T10() {
    pickrules::Triple slots[10];
    int words[10];
    for (int& w : words)
        w = -1;
    slots[0] = {1, 2, 3}, slots[2] = {4, 5, 6}, slots[5] = {7, 8, 9};
    words[0] = 2, words[2] = 3, words[5] = 3;
    Check(pickrules::ActiveSlot(slots, words, 10) == 0, "T10", "ActiveSlot: slot 0 ready, 2 and 5 put away: 0");
    words[0] = 3;
    Check(pickrules::ActiveSlot(slots, words, 10) == -1, "T10", "ActiveSlot: every one put away: -1");
    words[0] = 2, words[5] = 1;
    Check(pickrules::ActiveSlot(slots, words, 10) == -1, "T10", "ActiveSlot: two in hand: -1");
    words[5] = 0, words[7] = 2;
    Check(pickrules::ActiveSlot(slots, words, 10) == 0, "T10", "ActiveSlot: a word in a slot with no weapon, one never in hand: left out");
}

void T11() {
    pickrules::Triple before[10], after[10];
    before[0] = {0x1000, 0x2000, 0x3000}, before[2] = {0x1100, 0x2100, 0x3100};
    for (int i = 0; i < 10; ++i)
        after[i] = before[i];
    int changed[10] = {};
    Check(pickrules::Diff(before, after, 10, changed) == 0, "T11", "Diff: nothing changed: 0");
    after[0].blueprint = 0x3333;
    const int n = pickrules::Diff(before, after, 10, changed);
    Check(n == 1 && changed[0] == 0, "T11", "Diff: a new object at a freed one's address with another blueprint: slot 0 changed");
    after[2] = {};
    const int m = pickrules::Diff(before, after, 10, changed);
    Check(m == 2 && changed[0] == 0 && changed[1] == 2, "T11", "Diff: slot 2 emptied too: slots 0 and 2");
}

void T12() {
    const int slots[2] = {0, 2};
    const uint32_t went[2] = {0xA000, 0xB000};
    Check(pickrules::Forgets(slots, went, 2, 0, 0x1234), "T12", "Forgets: its own slot among the swap's: yes");
    Check(pickrules::Forgets(slots, went, 2, 1, 0xB000), "T12", "Forgets: its firing state among those that went: yes");
    Check(!pickrules::Forgets(slots, went, 2, 1, 0x1234) && !pickrules::Forgets(slots, went, 2, 1, 0), "T12",
          "Forgets: neither (the Stinger taken into slot 1 leaves slots 0 and 2 alone): no; no firing state: no");
}

void T13() {
    const int slots[2] = {0, 2};
    const uint32_t went[2] = {0x416, 0x320};
    pickrules::SwapSeen s;
    s.lies = true, s.lyingSlot = 0, s.releasedAt = 1000, s.at = 2000, s.changed = 2, s.slot = slots, s.wentBlueprint = went;
    s.takenBlueprint = 0x94, s.takenSlot = 0, s.byHand = true, s.request = 7, s.side = 0, s.waiting = 7, s.knownId = 16188;
    pickrules::SwapVerdict v = pickrules::OnSwap(s);
    Check(v.retire && v.answered && v.landedSide == 0 && v.landedId == 16188 && !v.refused && !v.sameKind && !v.swappedAway, "T13",
          "OnSwap: the left takes a known AN-94 for the lying M416: retired, answered, due in the left as 16188");
    s.retired = true;
    v = pickrules::OnSwap(s);
    Check(!v.retire && v.answered && v.landedSide == 0, "T13", "OnSwap: retired already: not again");
    s.retired = false;
    s.takenBlueprint = 0x416, s.knownId = 12768;
    v = pickrules::OnSwap(s);
    Check(v.retire && v.sameKind && v.landedSide == 0 && !v.swappedAway, "T13", "OnSwap: an M416 for the M416: the same kind, due in the hand");
    s.takenBlueprint = 0x94, s.knownId = 0, s.noRow = true;
    v = pickrules::OnSwap(s);
    Check(v.landedSide == 0 && v.landedAny && !v.refused, "T13", "OnSwap: no row for its name: due as any other profile");
    s.noRow = false;
    v = pickrules::OnSwap(s);
    Check(v.refused && v.unknown && v.landedSide < 0 && !v.swappedAway && v.retire, "T13",
          "OnSwap: a gun free hands don't know: refused, unknown, never swapped away (the copy stays), still retired");
    s.knownId = 16188, s.side = -1;
    v = pickrules::OnSwap(s);
    Check(v.answered && v.refused && v.landedSide < 0, "T13", "OnSwap: a take with no hand (pickups.txt take): answered, refused");
    s.side = 1, s.waiting = 6;
    v = pickrules::OnSwap(s);
    Check(!v.answered && v.landedSide < 0 && !v.refused && v.retire, "T13", "OnSwap: another request than the one waiting: not an answer");
    // The Stinger into slot 1 while the M416 lies in slot 0: not took.
    const int stingerSlot[1] = {1};
    const uint32_t nothing[1] = {0};
    s.waiting = 7, s.request = 7, s.side = 1, s.changed = 1, s.slot = stingerSlot, s.wentBlueprint = nothing, s.takenBlueprint = 0x92,
    s.takenSlot = 1, s.knownId = 20298, s.forceSlot = false;
    v = pickrules::OnSwap(s);
    Check(!v.retire && v.landedSide == 1 && v.landedId == 20298 && v.refused && v.freedAtOnce, "T13",
          "OnSwap: another slot, not forced: not retired, freed at once, the arrival still due");
    s.forceSlot = true;
    v = pickrules::OnSwap(s);
    Check(!v.retire && v.landedSide == 1 && !v.refused && !v.freedAtOnce, "T13", "OnSwap: another slot, forced: due, nothing refused");
    // The game's own R.
    pickrules::SwapSeen r;
    r.lies = true, r.lyingSlot = 0, r.releasedAt = 1000, r.at = 2000, r.changed = 2, r.slot = slots, r.wentBlueprint = went, r.takenBlueprint = 0x416;
    v = pickrules::OnSwap(r);
    Check(v.retire && v.sameKind && v.swappedAway && !v.answered, "T13", "OnSwap: the game's own R, the same kind, the gun lying: swapped away");
    r.takenBlueprint = 0x94;
    v = pickrules::OnSwap(r);
    Check(v.retire && !v.sameKind && !v.swappedAway, "T13", "OnSwap: the game's own R, another kind: retired only (the change of gun does the rest)");
    r.at = 500;
    v = pickrules::OnSwap(r);
    Check(!v.retire && !v.swappedAway && !v.answered && !v.refused, "T13", "OnSwap: a swap from before the release: nothing");
    r.at = 2000, r.lies = false;
    v = pickrules::OnSwap(r);
    Check(!v.retire && !v.swappedAway, "T13", "OnSwap: no gun lying: nothing");
}

void T14() {
    pickrules::NetVerdict v = pickrules::Net(true, true, 1000, 0, 1000, 2400);
    Check(!v.landedOver && !v.swappedAway && !v.refused, "T14", "Net: 1.4 s after a landed take: nothing yet");
    v = pickrules::Net(true, true, 1000, 0, 1000, 2600);
    Check(v.landedOver && v.refused && v.swappedAway, "T14", "Net: 1.6 s on, the gun still lying and retired: given up, back in the right hand");
    v = pickrules::Net(false, false, 0, 1, 1000, 2600);
    Check(v.landedOver && v.refused && !v.swappedAway, "T14", "Net: 1.6 s on, nothing lying: given up only");
    v = pickrules::Net(true, true, 1000, -1, 0, 2400);
    Check(!v.swappedAway, "T14", "Net: retired 1.4 s, no take due: nothing yet");
    v = pickrules::Net(true, true, 1000, -1, 0, 2600);
    Check(v.swappedAway && !v.retiredOver, "T14", "Net: retired 1.6 s, no take due: back in the right hand");
    v = pickrules::Net(false, true, 1000, -1, 0, 2600);
    Check(v.retiredOver && !v.swappedAway, "T14", "Net: retired and nothing lies any more: gone some other way, nothing given back");
    v = pickrules::Net(false, true, 1000, 0, 1000, 2600);
    Check(v.landedOver && v.retiredOver && !v.swappedAway, "T14", "Net: never swapped away unless the gun still lies");
}

void T15() {
    struct Row {
        const char* spelling;
        const char* name;
        uint32_t body;
    };
    const Row rows[] = {
        {"Weapons/M416/M416", "M416", 12768},
        {"Weapons/AN94/AN94", "AN-94", 16188},
        {"Weapons/RPK/RPK", "RPK-74M", 24492},
        {"Weapons/FIM92A/FIM92A_SP_Valley", "Stinger", 20298},
        {"Weapons/AK74M/AK74", "AK-74M", 21723},
        {"Weapons/M16A4/M16A4", "M16A4", 17013},
        {"Weapons/SMAW/SMAW", "SMAW", 3414},
        {"Weapons/RPG7/RPG7_AI", "RPG-7", 0},
    };
    for (const Row& r : rows) {
        const recoilguns::Row* row = recoilguns::ByName(r.spelling);
        char what[160];
        snprintf(what, sizeof(what), "the guns' table: \"%s\" is the %s's row (its draw %u)", r.spelling, r.name, r.body);
        Check(row && std::strcmp(row->name, r.name) == 0 && row->body == r.body, "T15", what);
    }
    Check(recoilguns::ByName("Weapons/Nothing/Nothing") == nullptr && recoilguns::ByName("") == nullptr, "T15",
          "the guns' table: a name it hasn't: no row");
}

}  // namespace

int main() {
    T1();
    T2();
    T3();
    T4();
    T5();
    T6();
    T7();
    T8();
    T9();
    T10();
    T11();
    T12();
    T13();
    T14();
    T15();
    std::printf("%d of %d checks failed\n", g_failed, g_counted);
    return g_failed;
}

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
//   T16 SeatedSleeps: the seated tell says nothing for 2 s after a hand's take landed, as before otherwise
//   T17 StaysWithSoldier: a hand-over count goes back only into a gun whose slot is as it was
//   T18 HandSlot: the M416, the Stinger, the SMAW and the pistol as before; a gun taken up into slot 1 (by its name, the words late or not); a test's switch under the M416's name; two of a
//       kind; a gun the table doesn't know; nothing drawn (the game's own current weapon when it's one of the two and a gun the hands work, else as
//       it was); only slots 0 and 1 ever
//   T19 GivesCountBack: an entry naming a slot whose gun stayed (the gun into another) gives its count back; landed by its counts alone it doesn't
//   T20 SeeCount: the game's reload of a gun with its magazine by hand, not asked for, undone from any count (0 too); its hidden round fired put
//       back; X held, weapon.txt "empty", a swap and a gun not by hand as before; shots, small rises, a new gun, no spare
//   T21 StaysAtEject: a magazine out of an empty gun leaves none in it and takes none from the spare ones (a fresh magazine then reads its own 30);
//       a gun with rounds keeps the chambered one, as before; a gun with no hidden round as before
//   T22 holsters, the gun put away at its place (SwapSeen::lies false: nothing of it lies): a hand's take into another slot or into the put-away gun's
//       own (Operation Swordbreaker's two slots full) retires nothing and is answered and due, freed at once unless the pickup forces its slot; the
//       game's own pickup of the same kind swaps nothing away; the net gives a landed take up after 1.5 s and never sends a gun back to a hand
//   T23 TakeVictims: the guns a take takes out of the soldier's slots (the slot each content entry names, ReplaceAllContent too: only those are
//       replaced; Unspawn the named ones gone; entries not all read: every gun carried), against the swaps seen in the game; holsters keep a
//       holstered gun taken so on its shoulder, in that pickup (holstertest HA), and refuse only a take that couldn't give it back
//   T24 an open bolt's box (the M249's, in-game 2026-11-18: "Magazine: in; 100 rounds loaded ... (a magazine holds 99)"): MagazineHolds 100, the
//       bridge's 200 (not 199), a count the mod wrote still 100; its own reload shows the whole box; its floor from the spare ones on a whole box
//       only; a box met full, pushed home by hand and filled by the game's own reload each fire 100 at the stop (200 the bridge's), the spare
//       ones paying 100 a box; every closed bolt's (the M416's 30, the RPK's 45, the M240B's 99 as approved, no row 1) as before
// Usage: picktest

#include <windows.h>

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>
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
        {"Weapons/AKS74u/AKS74u", "AKS-74u", 17991},
        {"Weapons/UMP45/UMP45", "UMP-45", 19314},
        {"Weapons/M1014/M1014", "M1014", 13377},
        {"Weapons/MK11/Mk11", "Mk11 Mod 0", 12456},
        {"Weapons/M249/M249", "M249", 24291},
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

// 2026-11-12, the Stinger by the left hand: at the game's switch to it the M416's seated tell came on for 0.5 s, free hands slept, the gun went
// back to the right hand (another gun, seated) and the arrival never came. The tell, the take landed at 1000.
void T16() {
    Check(pickrules::SeatedSleeps(true, 0, 5000) && !pickrules::SeatedSleeps(false, 0, 5000), "T16",
          "SeatedSleeps: no take: the tell as it is (seated asleep, not seated awake)");
    Check(!pickrules::SeatedSleeps(true, 1000, 1000) && !pickrules::SeatedSleeps(true, 1000, 1236) && !pickrules::SeatedSleeps(true, 1000, 1736) &&
              !pickrules::SeatedSleeps(true, 1000, 2999),
          "T16", "SeatedSleeps: the tell from the take to 2 s after it (the 2026-11-12 run's 0.24 to 0.74 s among them): awake");
    Check(pickrules::SeatedSleeps(true, 1000, 3000) && pickrules::SeatedSleeps(true, 1000, 60000), "T16",
          "SeatedSleeps: 2 s after the take and later: seated asleep again");
    Check(pickrules::SeatedSleeps(true, 5000, 4000) && !pickrules::SeatedSleeps(false, 1000, 1500), "T16",
          "SeatedSleeps: a take time after now: the tell as it is; not seated: awake");
}

// The hand-over's count goes back only into a gun left in the soldier's hands (the Stinger's ReplaceAllContent took nothing from slot 0).
void T17() {
    pickrules::Triple before[10], after[10];
    before[0] = {0x1000, 0x2000, 0x3000}, before[2] = {0x1100, 0x2100, 0x3100};
    for (int i = 0; i < 10; ++i)
        after[i] = before[i];
    after[1] = {0x1200, 0x2200, 0x3200};  // the Stinger into the empty slot 1
    Check(pickrules::StaysWithSoldier(before, after, 10, 0) && pickrules::StaysWithSoldier(before, after, 10, 2), "T17",
          "StaysWithSoldier: the M416 and the M320 as they were (the Stinger into slot 1): they stay, their counts back");
    Check(!pickrules::StaysWithSoldier(before, after, 10, 1), "T17", "StaysWithSoldier: the slot the Stinger came into (empty before): no");
    after[0] = {0x1000, 0x2000, 0x3300};
    Check(!pickrules::StaysWithSoldier(before, after, 10, 0), "T17", "StaysWithSoldier: another blueprint at the same addresses: gone into the pickup");
    after[0] = {0x4000, 0x5000, 0x6000};
    Check(!pickrules::StaysWithSoldier(before, after, 10, 0) && !pickrules::StaysWithSoldier(before, after, 10, -1) &&
              !pickrules::StaysWithSoldier(before, after, 10, 10) && !pickrules::StaysWithSoldier(nullptr, after, 10, 0),
          "T17", "StaysWithSoldier: another gun in the slot; a slot out of range; nothing to go by: no");
}

// The hands' slot: slots 0 and 1 as given (a weapon in each that's named), their words, the drawn gun's names.
pickrules::HandSlotIn Hands(pickrules::Drawn drawn, unsigned weapon, unsigned byName, unsigned rifleName, int word0, int word1) {
    pickrules::HandSlotIn in;
    in.drawn = drawn;
    in.known = drawn == pickrules::Drawn::kRifle || drawn == pickrules::Drawn::kHeld;
    in.weapon = weapon, in.byName = byName, in.rifleName = rifleName;
    in.word[0] = word0, in.word[1] = word1;
    return in;
}

void T18() {
    using pickrules::Drawn;
    using pickrules::HandSlot;
    // The never-break cases: the M416 alone; with the M320 up (the M416's word put away, slot 2 not looked at); the Stinger, the SMAW, the pistol.
    Check(HandSlot(Hands(Drawn::kRifle, 1, 1, 1, 2, -1)).slot == 0, "T18", "HandSlot: the M416 alone in slot 0: 0");
    Check(HandSlot(Hands(Drawn::kRifle, 1, 1, 1, 3, -1)).slot == 0, "T18", "HandSlot: the M416 with the grenade launcher up (its word 3): 0");
    Check(HandSlot(Hands(Drawn::kRifle, 3, 1, 1, 2, 0)).slot == 0 && HandSlot(Hands(Drawn::kRifle, 3, 1, 1, 2, 3)).slot == 0, "T18",
          "HandSlot: the M416 with the Stinger in slot 1 (never in hand, or put away): 0");
    Check(HandSlot(Hands(Drawn::kRifle, 3, 1, 1, 3, 2)).slot == 0, "T18",
          "HandSlot: the M416 drawn, by its name, though the words say slot 1: 0 (the name wins)");
    pickrules::HandSlotIn own = Hands(Drawn::kOwnPath, 3, 0, 1, 3, 2);
    own.last = 1, own.lastSame = true, own.landed = 1;
    Check(HandSlot(own).slot == 0, "T18", "HandSlot: the Stinger, the SMAW or the train pistol: 0 whatever was before");
    // The dead soldier's AN-94 in slot 1, the M416 left in slot 0.
    Check(HandSlot(Hands(Drawn::kHeld, 3, 2, 1, 3, 2)).slot == 1 && HandSlot(Hands(Drawn::kHeld, 3, 2, 1, -1, -1)).slot == 1 &&
              HandSlot(Hands(Drawn::kHeld, 3, 2, 1, 2, 0)).slot == 1,
          "T18", "HandSlot: the AN-94 taken up into slot 1 (the M416 in 0): 1, by its name, the words as they may be");
    Check(HandSlot(Hands(Drawn::kRifle, 3, 1, 1, 2, 3)).slot == 0, "T18", "HandSlot: the M416 picked again (the AN-94 put away in 1): 0");
    // A test's switch: the AN-94 drawn under the M416's name in slot 0.
    Check(HandSlot(Hands(Drawn::kHeld, 1, 0, 1, 2, -1)).slot == 0 && HandSlot(Hands(Drawn::kHeld, 1, 0, 1, -1, -1)).slot == 0, "T18",
          "HandSlot: a test's switch (the AN-94 under the M416's name, slot 0): 0");
    Check(HandSlot(Hands(Drawn::kHeld, 3, 2, 1, 2, 3)).slot == 1, "T18",
          "HandSlot: the AN-94 in slot 1 drawn again, the words a state late (slot 0 in hand, slot 1 put away): by its name, 1");
    Check(HandSlot(Hands(Drawn::kHeld, 3, 2, 1, 2, 0)).slot == 1, "T18",
          "HandSlot: ... the words not sure (slot 1's never in hand): by its name, 1");
    // Two of a kind (an enemy's M416 taken into 1 with the player's in 0).
    pickrules::HandSlotIn two = Hands(Drawn::kRifle, 3, 3, 3, -1, -1);
    Check(HandSlot(two).slot == 0, "T18", "HandSlot: two of a kind, nothing else known: 0");
    two.landed = 1;
    Check(HandSlot(two).slot == 1, "T18", "HandSlot: two of a kind: the one the last take put there");
    two.word[0] = 2, two.word[1] = 3;
    Check(HandSlot(two).slot == 0, "T18", "HandSlot: two of a kind: the sure words before the take's slot");
    two.word[0] = 0, two.word[1] = 2;
    Check(HandSlot(two).slot == 1, "T18", "HandSlot: two of a kind: the words, the other never in hand");
    two.word[0] = two.word[1] = -1, two.landed = -1, two.last = 1, two.lastSame = true;
    Check(HandSlot(two).slot == 1, "T18", "HandSlot: two of a kind: as it was, its gun the same");
    two.lastSame = false;
    Check(HandSlot(two).slot == 0, "T18", "HandSlot: two of a kind: as it was no more (its gun changed): 0");
    // A gun the table doesn't know.
    pickrules::HandSlotIn unknown = Hands(Drawn::kHeld, 3, 0, 1, -1, -1);
    unknown.known = false;
    Check(HandSlot(unknown).slot == 0, "T18", "HandSlot: a gun the table doesn't know, nothing to go by: 0");
    unknown.landed = 1;
    Check(HandSlot(unknown).slot == 1, "T18", "HandSlot: a gun the table doesn't know: where the last take put it");
    unknown.landed = -1, unknown.last = 1, unknown.lastSame = true;
    Check(HandSlot(unknown).slot == 1, "T18", "HandSlot: a gun the table doesn't know: as it was");
    unknown.word[0] = 2, unknown.word[1] = 3, unknown.landed = 1;
    Check(HandSlot(unknown).slot == 0, "T18", "HandSlot: a gun the table doesn't know: the sure words first");
    pickrules::HandSlotIn landedEmpty = Hands(Drawn::kHeld, 1, 0, 1, -1, -1);
    landedEmpty.known = false, landedEmpty.landed = 1;
    Check(HandSlot(landedEmpty).slot == 0, "T18", "HandSlot: the take's slot empty now: not it");
    // Nothing drawn: as it was while its gun is the same (the Stinger since put in its place: no).
    pickrules::HandSlotIn none = Hands(Drawn::kNothing, 3, 0, 1, -1, -1);
    none.last = 1, none.lastSame = true;
    Check(HandSlot(none).slot == 1, "T18", "HandSlot: nothing drawn: as it was (its gun the same)");
    none.lastSame = false;
    Check(HandSlot(none).slot == 0, "T18", "HandSlot: nothing drawn, another gun in that slot now (the Stinger in the AN-94's place): 0");
    pickrules::HandSlotIn odd = Hands(Drawn::kNothing, 3, 0, 0, -1, -1);
    odd.last = 5, odd.lastSame = true;
    Check(HandSlot(odd).slot == 0, "T18", "HandSlot: a slot outside 0 and 1 never comes out");
    // Nothing drawn: the game's own current weapon, when it is one of the first two slots and a gun the hands work the magazine of. The night's
    // load: the M416 in slot 0, the AN-94 of slot 1 in the soldier's hands, the words 2 and 0, the hands not drawn for five minutes.
    pickrules::HandSlotIn load = Hands(Drawn::kNothing, 3, 0, 1, 2, 0);
    load.current = 1, load.handGun = 3;
    Check(HandSlot(load).slot == 1 && std::strcmp(HandSlot(load).why, "nothing drawn: the game's own current weapon") == 0, "T18",
          "HandSlot: nothing drawn after a load, the game's own current weapon in slot 1 (the AN-94): 1, said");
    load.last = 0, load.lastSame = true;
    Check(HandSlot(load).slot == 1, "T18", "HandSlot: ... slot 0 before with its gun the same (key 2 with the hands not drawn): the game's own, 1");
    load.current = 0, load.last = 1;
    Check(HandSlot(load).slot == 0, "T18", "HandSlot: ... the game's own current weapon in slot 0, slot 1 before: 0");
    load.current = -1;
    Check(HandSlot(load).slot == 1, "T18", "HandSlot: nothing drawn, the game's own not read: as it was (its gun the same)");
    load.current = 2;
    Check(HandSlot(load).slot == 1, "T18", "HandSlot: nothing drawn, the game's own the grenade launcher's slot: as it was");
    load.current = 5, load.lastSame = false;
    Check(HandSlot(load).slot == 0, "T18", "HandSlot: nothing drawn, the game's own the SMAW's slot, the last slot's gun another: the first slot, as before");
    load.current = 1, load.weapon = 1, load.last = 0, load.lastSame = true;
    Check(HandSlot(load).slot == 0, "T18", "HandSlot: nothing drawn, the game's own names slot 1 with no weapon in it: as it was");
    load.weapon = 3, load.handGun = 1;
    Check(HandSlot(load).slot == 0, "T18",
          "HandSlot: nothing drawn, the game's own in slot 1 a gun the hands don't work (the Stinger, a pistol, one not measured): as it was");
    pickrules::HandSlotIn drawnOther = Hands(Drawn::kHeld, 3, 2, 1, 3, 2), ownCurrent = Hands(Drawn::kOwnPath, 3, 0, 1, 3, 2);
    drawnOther.current = 0, drawnOther.handGun = 3;
    ownCurrent.current = 1, ownCurrent.handGun = 3;
    Check(HandSlot(drawnOther).slot == 1 && HandSlot(ownCurrent).slot == 0, "T18",
          "HandSlot: a gun drawn: by its name whatever the game's own current weapon says; the Stinger drawn: the first slot, as before");
    Check(HandSlot(Hands(Drawn::kRifle, 3, 1, 1, 2, 2)).slot == 0 && pickrules::WordSlot(Hands(Drawn::kRifle, 3, 1, 1, 2, 2), false) == -1, "T18",
          "WordSlot: both in hand: none (the name)");
    Check(std::strcmp(HandSlot(Hands(Drawn::kHeld, 3, 2, 1, 3, 2)).why, "by its name") == 0, "T18", "HandSlot: its reason said");
}

void T19() {
    pickrules::Triple before[10], after[10];
    before[0] = {0x1000, 0x2000, 0x3000}, before[2] = {0x1100, 0x2100, 0x3100};
    for (int i = 0; i < 10; ++i)
        after[i] = before[i];
    after[1] = {0x1200, 0x2200, 0x3200};  // the AN-94 into the empty slot 1, its entry naming slot 0
    Check(pickrules::GivesCountBack(before, after, 10, 0, 0, 17, 17), "T19",
          "GivesCountBack: the entry named slot 0, the gun went into 1, the M416 stayed in 0 with the count written: given back");
    Check(pickrules::GivesCountBack(before, after, 10, 2, 0, 0, 0), "T19", "GivesCountBack: the M320 left in slot 2: given back");
    Check(!pickrules::GivesCountBack(before, after, 10, 0, 0, 30, 17), "T19", "GivesCountBack: a count the game changed since: not");
    Check(!pickrules::GivesCountBack(before, after, 10, 1, 0, 30, 30), "T19", "GivesCountBack: the slot that changed: not");
    for (int i = 0; i < 10; ++i)
        after[i] = before[i];
    Check(!pickrules::GivesCountBack(before, after, 10, 0, 0, 17, 17), "T19",
          "GivesCountBack: landed by its counts alone (no triple changed): the taken entry's slot is the game's write, not given back");
    Check(pickrules::GivesCountBack(before, after, 10, 2, 0, 0, 0), "T19", "GivesCountBack: ... another slot that stayed: given back, as before");
    after[0] = {0x4000, 0x5000, 0x6000};
    Check(!pickrules::GivesCountBack(before, after, 10, 0, 0, 17, 17), "T19", "GivesCountBack: the entry's slot swapped for the taken gun: not");
}

// The game's count of the gun in the hands, a frame: the night's M416 (1 of 370 fired to 0, then 30 of 340 by the game) and every other case.
void T20() {
    using pickrules::CountIn;
    using pickrules::CountSeen;
    using pickrules::SeeCount;
    const auto in = [](int last, int raw, int spare, int hidden, bool byHand, bool asked, int putBacks = 0) {
        CountIn c;
        c.last = last, c.raw = raw, c.spare = spare, c.hidden = hidden, c.byHand = byHand, c.asked = asked, c.putBacks = putBacks;
        return c;
    };
    // The reload from 0 the old rule missed: undone, the hidden round back, the 30 it took given back.
    pickrules::CountOut o = SeeCount(in(0, 30, 340, 1, true, false));
    Check(o.what == CountSeen::kUndone && o.loaded == 1 && o.spare == 369, "T20", "SeeCount: 0 -> 30 of 340 by itself: undone, 1 in it, 369 spare");
    // ... and a frame earlier, its hidden round fired: put back from the spare ones before the game can reload.
    o = SeeCount(in(1, 0, 370, 1, true, false));
    Check(o.what == CountSeen::kPutBack && o.loaded == 1 && o.spare == 369, "T20", "SeeCount: the hidden round fired (1 -> 0 of 370): put back, 369 spare");
    // A reload by itself from a round or more (the hidden round still in): undone to what it was.
    o = SeeCount(in(1, 31, 339, 1, true, false));
    Check(o.what == CountSeen::kUndone && o.loaded == 1 && o.spare == 369, "T20", "SeeCount: 1 -> 31 of 339 by itself: undone, 1 in it, 369 spare");
    o = SeeCount(in(16, 31, 285, 1, true, false));
    Check(o.what == CountSeen::kUndone && o.loaded == 16 && o.spare == 300, "T20", "SeeCount: 16 -> 31 by itself: undone, 16 in it, the 15 back");
    // X held (R down or just let go), weapon.txt "empty", a swap just now: the game's reload kept, as before; from 0 nothing, as before.
    o = SeeCount(in(1, 31, 339, 1, true, true));
    Check(o.what == CountSeen::kKept && o.loaded == 31 && o.spare == 339, "T20", "SeeCount: asked for (X held), from 1: kept, nothing written");
    o = SeeCount(in(0, 30, 270, 0, true, true));
    Check(o.what == CountSeen::kNone, "T20", "SeeCount: asked for (weapon.txt empty), from 0: nothing, as before");
    o = SeeCount(in(0, 0, 300, 1, true, true));
    Check(o.what == CountSeen::kNone, "T20", "SeeCount: at 0 while X is held: not put back (the game's own reload is asked for)");
    // A gun whose magazine isn't worked by hand: as before (kept from a round or more, nothing from 0, never put back).
    o = SeeCount(in(3, 31, 200, 0, false, false));
    Check(o.what == CountSeen::kKept, "T20", "SeeCount: not by hand, 3 -> 31: kept, as before");
    o = SeeCount(in(0, 30, 200, 1, false, false));
    Check(o.what == CountSeen::kNone, "T20", "SeeCount: not by hand, 0 -> 30: nothing, as before");
    o = SeeCount(in(1, 0, 200, 1, false, false));
    Check(o.what == CountSeen::kNone, "T20", "SeeCount: not by hand, the hidden round fired: nothing");
    // Not a reload: shots, a magazine by hand's few rounds, a new gun, no spare, no hidden round, the put-backs used up.
    o = SeeCount(in(20, 19, 300, 1, true, false));
    Check(o.what == CountSeen::kNone && o.loaded == 19 && o.spare == 300, "T20", "SeeCount: a shot: nothing");
    o = SeeCount(in(20, 24, 300, 1, true, false));
    Check(o.what == CountSeen::kNone, "T20", "SeeCount: up by 4 (under a reload's jump): nothing");
    o = SeeCount(in(-1, 30, 300, 0, true, false));
    Check(o.what == CountSeen::kNone, "T20", "SeeCount: a new gun (no last count): nothing");
    o = SeeCount(in(1, 0, 0, 1, true, false));
    Check(o.what == CountSeen::kNone, "T20", "SeeCount: the hidden round fired, no spare: nothing (the game can't reload it)");
    o = SeeCount(in(0, 0, 300, 0, true, false));
    Check(o.what == CountSeen::kNone, "T20", "SeeCount: at 0 with no hidden round (ammo gives it one): nothing here");
    o = SeeCount(in(1, 0, 300, 1, true, false, pickrules::kMostPutBacks));
    Check(o.what == CountSeen::kNone, "T20", "SeeCount: the hidden round fired a fourth time this magazine: not put back");
    o = SeeCount(in(0, 30, 270, 1, true, false, pickrules::kMostPutBacks));
    Check(o.what == CountSeen::kUndone && o.loaded == 1 && o.spare == 299, "T20", "SeeCount: ... and the game's reload after it: undone all the same");
    // The last few spare rounds reloaded by itself from empty (a rise under a reload's jump), the put-backs used up: undone, all of them back.
    o = SeeCount(in(0, 3, 0, 1, true, false, pickrules::kMostPutBacks));
    Check(o.what == CountSeen::kUndone && o.loaded == 1 && o.spare == 2, "T20", "SeeCount: 0 -> 3 with the last 3 spare, put-backs used up: undone, 1 in it, 2 spare");
    o = SeeCount(in(1, 4, 0, 1, true, false));
    Check(o.what == CountSeen::kUndone && o.loaded == 1 && o.spare == 3, "T20", "SeeCount: 1 -> 4 (only the hidden round in it) by itself: undone, the 3 back");
    o = SeeCount(in(0, 3, 0, 1, true, true));
    Check(o.what == CountSeen::kNone, "T20", "SeeCount: 0 -> 3 asked for (X held): nothing, as before");
    o = SeeCount(in(0, 3, 0, 1, false, false));
    Check(o.what == CountSeen::kNone, "T20", "SeeCount: 0 -> 3 not by hand: nothing, as before");
    o = SeeCount(in(0, 1, 300, 1, true, false));
    Check(o.what == CountSeen::kNone, "T20", "SeeCount: 0 -> 1 (back at the hidden round only): nothing");
    o = SeeCount(in(5, 3, -1, 1, true, false));
    Check(o.what == CountSeen::kNone, "T20", "SeeCount: a count not read (below 0): nothing");
}

// A magazine out by hand, and a fresh one of 30 in (ammo.cpp's Eject and Insert: what stays, the rest back to the spare ones, then up to 30 of them).
// The rounds the gun can fire are the count less the one the trigger's stop keeps.
void T21() {
    using pickrules::StaysAtEject;
    const auto reload = [](int loaded, int spare, int& loadedAfter, int& spareAfter) {
        const int kept = StaysAtEject(loaded, spare);
        spare += loaded - kept;
        const int take = spare < 30 ? spare : 30;
        loadedAfter = kept + take, spareAfter = spare - take;
        return kept;
    };
    const auto fires = [](int loaded) { return loaded > 1 ? loaded - 1 : 0; };
    int loaded = 0, spare = 0;
    // The night's AN-94: its burst fired past the trigger's stop (0 of 193 as the mod counts, the hidden round in the game's count).
    Check(reload(0, 193, loaded, spare) == 1 && fires(loaded) == 30 && spare == 162, "T21",
          "StaysAtEject: a gun fired past the stop (0 of 193): one stays, from the spare ones; a magazine in: its 30 fire, 162 spare");
    Check(reload(1, 193, loaded, spare) == 1 && fires(loaded) == 30 && spare == 163, "T21",
          "StaysAtEject: its last round still in it (the trigger stopped there, 1 of 193): it stays; a magazine in: its 30 fire, 163 spare, as before");
    Check(reload(14, 91, loaded, spare) == 1 && loaded == 31 && spare == 74, "T21",
          "StaysAtEject: 14 of 91: the chambered round stays, 13 back; a magazine in: 31 loaded, 74 spare, as before");
    Check(reload(0, 0, loaded, spare) == 0 && fires(loaded) == 0 && spare == 0, "T21",
          "StaysAtEject: empty with no spare rounds: none, and none to load");
    Check(reload(0, 12, loaded, spare) == 1 && fires(loaded) == 11 && spare == 0, "T21",
          "StaysAtEject: empty with 12 spare: one of them stays; the 11 others in, all of them fire");
    Check(StaysAtEject(0, 300) == 1 && StaysAtEject(0, 0) == 0 && StaysAtEject(5, 300) == 1, "T21",
          "StaysAtEject: one stays, from the spare ones if there are any, as before");
}

// Holsters (hands.cpp's NoteSwaps with the grasp put away: lies false, nothing retired): no case of its own is needed in OnSwap or Net.
void T22() {
    const int intoOne[1] = {1}, intoZero[1] = {0};
    const uint32_t nothing[1] = {0}, m4a1[1] = {0x4a1};
    pickrules::SwapSeen s;
    s.lies = false, s.lyingSlot = 0, s.releasedAt = 0, s.retired = false, s.at = 2000, s.changed = 1, s.slot = intoOne, s.wentBlueprint = nothing;
    s.takenBlueprint = 0x94, s.takenSlot = 1, s.forceSlot = false, s.byHand = true, s.request = 9, s.side = 1, s.waiting = 9, s.knownId = 16188;
    pickrules::SwapVerdict v = pickrules::OnSwap(s);
    Check(!v.retire && v.answered && v.landedSide == 1 && v.landedId == 16188 && v.refused && v.freedAtOnce && !v.sameKind && !v.swappedAway, "T22",
          "OnSwap, put away: the right takes an AN-94 into slot 1: nothing retired, answered, due in the right, freed at once (the game keeps its weapon)");
    s.slot = intoZero, s.wentBlueprint = m4a1, s.takenSlot = 0;
    v = pickrules::OnSwap(s);
    Check(!v.retire && v.answered && v.landedSide == 1 && v.refused && v.freedAtOnce && !v.sameKind && !v.swappedAway, "T22",
          "OnSwap, put away: the AN-94 into the put-away M4A1's slot 0: nothing retired (no copy lies), due should it come, never swapped away");
    s.forceSlot = true;
    v = pickrules::OnSwap(s);
    Check(!v.retire && v.landedSide == 1 && !v.refused && !v.freedAtOnce && !v.swappedAway, "T22",
          "OnSwap, put away: the pickup forcing its slot: due, the grasp's take still waiting for it");
    pickrules::SwapSeen r;
    r.lies = false, r.lyingSlot = 0, r.releasedAt = 0, r.at = 2000, r.changed = 1, r.slot = intoZero, r.wentBlueprint = m4a1, r.takenBlueprint = 0x4a1;
    v = pickrules::OnSwap(r);
    Check(!v.retire && !v.swappedAway && !v.answered && !v.refused && v.landedSide < 0, "T22",
          "OnSwap, put away: the game's own R, the same kind as the gun put away: nothing (no copy lies, nothing swapped away)");
    pickrules::NetVerdict n = pickrules::Net(false, false, 0, 1, 1000, 2400);
    Check(!n.landedOver && !n.refused && !n.swappedAway && !n.retiredOver, "T22", "Net, put away: 1.4 s after a landed take: nothing yet");
    n = pickrules::Net(false, false, 0, 1, 1000, 2600);
    Check(n.landedOver && n.refused && !n.swappedAway && !n.retiredOver, "T22", "Net, put away: 1.6 s on: given up, never sent back to a hand");
    n = pickrules::Net(false, false, 0, -1, 0, 100000);
    Check(!n.landedOver && !n.refused && !n.swappedAway && !n.retiredOver, "T22", "Net, put away, no take due: nothing, however long");
}

// TakeVictims: which of the player's guns a take puts into the pickup, against the swaps the game made (the logs' lines named in each check).
void T23() {
    using pickrules::TakeIn;
    using pickrules::TakeVictims;
    const auto bits = [](std::initializer_list<int> slots) {
        unsigned b = 0;
        for (int s : slots)
            b |= 1u << s;
        return b;
    };
    TakeIn a;  // an AN-94 drop naming slot 1
    a.entries = 1, a.entry[0] = {true, 1};
    a.carried = bits({0, 2, 5, 6, 7});
    Check(TakeVictims(a) == 0u, "T23",
          "TakeVictims: an AN-94 naming slot 1, slot 1 empty: nothing goes into the pickup (2026-11-13 01:21:31: \"nothing into its place ... it went with "
          "the take\"), the M416 on the ground stays in slot 0");
    a.carried = bits({0, 1, 2, 5, 6, 7});
    Check(TakeVictims(a) == bits({1}), "T23", "TakeVictims: the same with a gun in slot 1: that gun (01:22:07: \"the AN-94 into slot 1; the AN-94 into its place\")");
    TakeIn b;  // an AN-94 drop naming slot 0 with a null entry for slot 2
    b.entries = 2, b.entry[0] = {true, 0}, b.entry[1] = {false, 2};
    b.carried = bits({0, 1, 2, 5, 6, 7});
    Check(TakeVictims(b) == bits({0, 2}), "T23",
          "TakeVictims: naming slot 0 and a null slot 2: the M416 and its M320 (02:00:40: \"the AN-94 into slot 0; the M416 and the M416 into its place\")");
    TakeIn m;  // Swordbreaker's M249 naming slot 0
    m.entries = 1, m.entry[0] = {true, 0};
    m.carried = bits({0, 1, 6});
    Check(TakeVictims(m) == bits({0}), "T23", "TakeVictims: the M249 naming slot 0: the M4A1 (2026-11-18: \"the M249 into slot 0; the M4A1 into its place\")");
    TakeIn r = m;
    r.replaceAll = true;
    Check(TakeVictims(r) == bits({0}), "T23",
          "TakeVictims: ReplaceAllContent: the named slot alone (008AB94E-008AB99F copies every gun into the pickup, the soldier's left where they are; the "
          "give loop 008ABAE0-008ABC20 replaces only the slots its entries name: the review of 2026-11-18)");
    TakeIn u;  // the Stinger: UnspawnOnPickup, slot 1
    u.entries = 1, u.entry[0] = {true, 1}, u.unspawnOrStatic = true, u.replaceAll = true;
    u.carried = bits({0, 2, 5});
    const unsigned stingerEmpty = TakeVictims(u);
    u.carried = bits({0, 1, 2, 5});
    Check(stingerEmpty == 0u && TakeVictims(u) == bits({1}), "T23",
          "TakeVictims: UnspawnOnPickup (the Stinger) puts none in: slot 1 empty nothing goes; held, that gun is gone (pickups' keepsNoPlace: never a hand's)");
    TakeIn o;  // slots out of range
    o.entries = 4, o.entry[0] = {true, -1}, o.entry[1] = {true, 40}, o.entry[2] = {false, 5}, o.entry[3] = {true, 6};
    o.carried = bits({0, 5, 6});
    Check(TakeVictims(o) == bits({5, 6}) && !pickrules::TakeUnread(o), "T23", "TakeVictims: slots out of range left out, the four entries read named");
    TakeIn x = o;  // more entries than are read (pickups keeps four): what the others name isn't known
    x.entriesAll = 6;
    TakeIn y = o;
    y.entries = 9;
    Check(pickrules::TakeUnread(x) && TakeVictims(x) == o.carried && pickrules::TakeUnread(y) && TakeVictims(y) == o.carried, "T23",
          "TakeVictims: more entries than the four read (entriesAll 6, or entries 9): every gun carried, as the most it could take (holsters refuse it with "
          "a holstered gun: kUnread)");
}

// An open bolt's box against the closed bolts' magazines, as ammo.cpp counts them: Capacity (MagazineHolds), the game's own reload (ReloadHolds), the
// floor on a whole box (FloorFromSpare, at first sight and after the game's reload), Eject (StaysAtEject) and Insert (up to a magazine from the spare
// ones). The rounds a gun fires are its count less the one the trigger's stop keeps.
void T24() {
    using pickrules::FloorFromSpare;
    using pickrules::MagazineHolds;
    using pickrules::OpenBoltOf;
    using pickrules::ReloadHolds;
    const auto fires = [](int loaded) { return loaded > 1 ? loaded - 1 : 0; };
    const recoilguns::Row* m249 = recoilguns::ByName("M249");
    const recoilguns::Row* bySlot = recoilguns::ByName("Weapons/M249/M249");  // (ammo.cpp's slots find it by its blueprint's name)
    const recoilguns::Row* m240 = recoilguns::ByName("M240B");
    const recoilguns::Row* m416 = recoilguns::ByName("M416");
    const recoilguns::Row* rpk = recoilguns::ByName("RPK-74M");
    const recoilguns::Row* an94 = recoilguns::ByName("AN-94");
    const pickrules::OpenBolt* bolt = m249 ? OpenBoltOf(m249->shape) : nullptr;
    Check(m249 && bolt && bySlot == m249 && bolt->box == m249->magazine && bolt->box == 100 && bolt->bigBox == 200, "T24",
          "OpenBoltOf: the M249's row (its shape m249; by its blueprint's name too) has an open bolt, its box its row's 100, the bridge's 200");
    Check(m240 && m416 && rpk && an94 && !OpenBoltOf(m240->shape) && !OpenBoltOf(m416->shape) && !OpenBoltOf(rpk->shape) && !OpenBoltOf(an94->shape) &&
              !OpenBoltOf(nullptr) && !OpenBoltOf("m240"),
          "T24", "OpenBoltOf: the M240B (kept at 99 a box, as approved), the M416, the RPK-74M, the AN-94, no shape: closed");
    if (!m249 || !bolt || !m240 || !m416 || !rpk || !an94)
        return;
    // The M249's box: 100, never 99; the bridge's 200, never 199.
    Check(MagazineHolds(m249->magazine, 0, 0, bolt) == 100, "T24", "MagazineHolds: the M249 not yet counted: its box of 100 (was 99)");
    Check(MagazineHolds(m249->magazine, 100, 0, bolt) == 100, "T24", "MagazineHolds: the M249 met full (100 as the game counts): 100 (was 99)");
    Check(MagazineHolds(m249->magazine, 200, 0, bolt) == 200, "T24", "MagazineHolds: the bridge's M249 met with its 200: 200 (was 199)");
    Check(MagazineHolds(m249->magazine, 101, 0, bolt) == 100 && MagazineHolds(m249->magazine, 102, 0, bolt) == 100, "T24",
          "MagazineHolds: an M249 met with a count the mod wrote (its floor, and its hidden round, on a box of 100: 101, 102): still 100");
    Check(MagazineHolds(m249->magazine, 37, 0, bolt) == 100 && MagazineHolds(m249->magazine, 150, 0, bolt) == 200, "T24",
          "MagazineHolds: a part-used box: 37 one of 100, 150 the bridge's 200");
    Check(ReloadHolds(100, bolt) == 100 && ReloadHolds(200, bolt) == 200 && MagazineHolds(m249->magazine, 100, ReloadHolds(100, bolt), bolt) == 100 &&
              MagazineHolds(m249->magazine, 0, ReloadHolds(200, bolt), bolt) == 200 && MagazineHolds(m249->magazine, 0, ReloadHolds(60, bolt), bolt) == 100,
          "T24", "ReloadHolds: the game's own reload of the M249 shows its whole box, 100 or 200 (was 99, 199); a short one (60, the spare running out) still a box of 100");
    // Its floor: on a whole box only, paid by a spare round.
    Check(FloorFromSpare(bolt, 100, 372) == 1 && FloorFromSpare(bolt, 200, 400) == 1, "T24",
          "FloorFromSpare: a whole box as the game counts it (100, the bridge's 200) with spare rounds: its floor from them");
    Check(FloorFromSpare(bolt, 101, 300) == 0 && FloorFromSpare(bolt, 102, 300) == 0 && FloorFromSpare(bolt, 63, 300) == 0 && FloorFromSpare(bolt, 0, 300) == 0 &&
              FloorFromSpare(bolt, 100, 0) == 0,
          "T24", "FloorFromSpare: none on a count the mod wrote (101, 102), a part-used box (63), an empty gun (0: its eject's), nor with no spare rounds");
    Check(FloorFromSpare(nullptr, 100, 300) == 0 && FloorFromSpare(nullptr, 31, 300) == 0 && FloorFromSpare(OpenBoltOf(m240->shape), 100, 300) == 0, "T24",
          "FloorFromSpare: none for a closed bolt (the M240B's 100 as approved, the M416's 31)");
    // Box after box, as ammo.cpp counts them.
    {
        int loaded = 100, spare = 372;  // Operation Swordbreaker's M249 as the game gives it (the in-game run's first sight)
        const int floorMet = FloorFromSpare(bolt, loaded, spare);
        loaded += floorMet, spare -= floorMet;
        const int firstBox = fires(loaded);
        const int total = loaded + spare;
        int boxes[3] = {};
        bool kept = true;
        for (int b = 0; b < 3; ++b) {
            loaded = 1;  // fired to the trigger's stop
            const int stays = pickrules::StaysAtEject(loaded, spare);
            spare += loaded - stays;
            const int holds = MagazineHolds(m249->magazine, 100, 0, bolt);
            const int take = spare < holds ? spare : holds;
            loaded = stays + take, spare -= take;
            boxes[b] = fires(loaded);
            kept = kept && loaded + spare + 100 * (b + 1) == total;
        }
        Check(floorMet == 1 && firstBox == 100 && boxes[0] == 100 && boxes[1] == 100 && boxes[2] == 100 && spare == 71 && kept, "T24",
              "the M249 box after box: met full (100 of 372: 101 loaded, 371 spare) fires 100, each box pushed home after the stop 100 (101 loaded, "
              "271 spare after the first), 100 of the spare ones a box (was 99 a box)");
        // The game fired past the stop (the in-game run's every box: 0 as the mod counts): its floor back from the spare ones at the eject, then 100.
        loaded = 0, spare = 271;
        const int stays = pickrules::StaysAtEject(loaded, spare);
        spare += loaded - stays;
        const int take = MagazineHolds(m249->magazine, 100, 0, bolt);
        loaded = stays + take, spare -= take;
        Check(stays == 1 && loaded == 101 && spare == 170 && fires(loaded) == 100, "T24",
              "the M249 fired past the stop (0 of 271): its floor from the spare ones at the eject, a box in: 101 loaded, 170 spare, 100 fire");
        // The game's own reload (X held), from 30 as the mod counts (its hidden round on top): the game's count to its box, the spare ones paying.
        loaded = 30, spare = 300;
        const int raw = 100;  // the game's count right after its reload, its hidden round in what it topped up
        const int spareAfter = spare - (raw - (loaded + 1));
        const int learned = ReloadHolds(raw, bolt);
        const int floorGame = FloorFromSpare(bolt, raw, spareAfter);
        Check(learned == 100 && floorGame == 1 && fires(raw + floorGame) == 100 && spareAfter - floorGame == 230, "T24",
              "the M249's own reload by the game (30 of 300): learned 100, its floor from the spare ones on top: 101 loaded, 100 fire, 230 spare");
        // The bridge's box of 200: met full, then pushed home by hand.
        loaded = 200, spare = 400;
        const int floorBig = FloorFromSpare(bolt, loaded, spare);
        loaded += floorBig, spare -= floorBig;
        const int bigFirst = fires(loaded);
        loaded = 1;
        const int holdsBig = MagazineHolds(m249->magazine, 200, 0, bolt);
        const int takeBig = spare < holdsBig ? spare : holdsBig;
        loaded += takeBig, spare -= takeBig;
        Check(bigFirst == 200 && holdsBig == 200 && fires(loaded) == 200 && spare == 199, "T24",
              "the bridge's M249: met with its 200 fires 200, a box of 200 pushed home 200 (201 loaded, 199 spare) (was 199 each)");
    }
    // Every closed bolt as before: Ammo.MagazineCapacity less the chambered round, learned first, the most seen less one, at least 1.
    const auto before = [](int table, int fullest, int learned) {
        const int t = table > 1 ? table - 1 : 1;
        if (learned)
            return learned;
        int most = fullest - 1;
        if (most < 1)
            most = 1;
        return most > t ? most : t;
    };
    bool same = true;
    for (const recoilguns::Row* row : {m416, rpk, an94, m240})
        for (int fullest : {0, 1, 2, 16, 30, 31, 32, 45, 46, 47, 99, 100, 101, 102, 200})
            for (int learned : {0, 4, 29, 30, 45, 99, 199})
                same = same && MagazineHolds(row->magazine, fullest, learned, nullptr) == before(row->magazine, fullest, learned) &&
                       MagazineHolds(row->magazine, fullest, learned, OpenBoltOf(row->shape)) == before(row->magazine, fullest, learned);
    for (int fullest : {0, 1, 5, 31})
        same = same && MagazineHolds(0, fullest, 0, nullptr) == before(0, fullest, 0);
    for (int raw : {5, 30, 31, 46, 100})
        same = same && ReloadHolds(raw, nullptr) == raw - 1;
    Check(same && MagazineHolds(m416->magazine, 31, 0, nullptr) == 30 && MagazineHolds(rpk->magazine, 0, 0, nullptr) == 45 &&
              MagazineHolds(m240->magazine, 100, 0, OpenBoltOf(m240->shape)) == 99 && MagazineHolds(0, 0, 0, nullptr) == 1,
          "T24", "MagazineHolds and ReloadHolds for every closed bolt as before, bit for bit (the M416's 30, the RPK's 45, the M240B's 99, no row 1)");
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
    T16();
    T17();
    T18();
    T19();
    T20();
    T21();
    T22();
    T23();
    T24();
    std::printf("%d of %d checks failed\n", g_failed, g_counted);
    return g_failed;
}

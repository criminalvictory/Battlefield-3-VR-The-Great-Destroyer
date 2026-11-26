// A scripted scene's straight ahead (src/sceneview.h, header only, as camera.cpp's TurnToHead and xr_runtime.cpp run it, with src/viewturn.h's turn of a
// scripted camera), without the game. A camera is BF3's LinearTransform (the rows right, up, back and translation, the w lanes filled as a scripted
// one's); a turn is a quaternion in OpenXR's axes (x right, y up, -z forward), its yaws turning left positive; BF3's own yaws (atan2 of the back
// row's x and -z) grow turning right. Where a world direction is shown in the room: the turned camera's rows take it apart (what the picture shows
// there), the head's pose puts that in the room (the compositor shows the picture from the head).
//   1  HeadYaw: a yaw, then a pitch, then a roll, gives that yaw back (0.01 deg; every 5 degrees of yaw, pitch within 89, roll within 60); straight
//      down and straight up by the way the top points; not a number: 0
//   2  SceneLocal is viewturn::QuatMul's yaw(-y) times the head (1e-6); viewturn::Turn with the yaw 0 is bit for bit the turn before this change
//      (MakeLevel, the head put on, mirrored or not: copied below), and the player's kind of camera bit for bit whatever the yaw
//   3  the owner's breach (Operation Swordbreaker, 2026-11-26 03:25:03, the laundry room's door): the head 85 degrees left of the room's straight
//      ahead (his recenter 4 s after: "yaw 85 deg"), the scene's camera at BF3's yaw -80.0, pitch -6.1, roll -1.2, its w lanes as logged: as it
//      was, its forward shown 85 degrees right of his face; now at his face (0.01 deg) from the scene's first picture, both eyes' and their cull
//      cameras alike (one latch), and with his head turned about after (30 left, 20 up, 45 right and rolled) the scene stays where it is in the room
//      (its forward at the latch, level), the head looking about it
//   4  the scene's end: the player's camera back as the picture body holds it (the yaw taken out of the scene's cameras, ShownTurn's taken, now the
//      held yaw), its forward shown where the scene's was (0.01 deg): no jump of the world, the scene's straight ahead kept; as it was, no jump
//      either (both at the room's), the scene 85 off the face
//   5  a ride (Comrades' drive: one scripted run of 100 s): one latch for all of it while the head looks about (90 left and right, up and down,
//      every frame a new head) and the car turns and cuts (a 45-degree cut, a steady turn): the latch never again, the scene's forward always at it
//   6  the scenes' numbering (StepScenes): play before the first scene; play under a second between two cutscenes' pictures one scene (Comrades'
//      16 pictures of the player's camera, 0.22 s, before its cut into the car; 0.5 s; 0.99 s); a second of play a new scene, its number up before
//      the next scene's first scripted camera (which comes two pictures before g_cutscene says so); the interrogation's retries (a load between,
//      1.04 s of play: a new scene each); Semper Fidelis's death, load and checkpoint (8 s, 4.5 s, 1.2 s of play: the checkpoint's own scene a new
//      one); a recenter (NewScene): the next scripted camera latches afresh, in the room as recentered; the clock going back
//   7  in play nothing: the player's camera (the look written, or followed) turned bit for bit as before with any scene latched, and no scripted
//      camera, no latch
//   8  mirrored (a cutscene's rows of the other hand, the mirror fix on): turned with the latch, the same forward and up as its unmirrored twin turned
//      with it and the right row the other way (1e-6), as at the room's: the scene's forward where the twin's is
//   9  the head's place (headmove.h, the body's level frame: the turned camera less the head's turn, camera.cpp's NoteFrameBody): a lean of 10 cm
//      along the face moves the scene's camera 10 cm along the scene's forward (1 mm); as it was, 85 degrees off it
//  10  the time: a latch and a turn under a microsecond
// Usage: sceneviewtest
// The exit code is the number of failed checks.

#include <windows.h>

#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <limits>
#include <random>

#include "../src/sceneview.h"
#include "../src/viewturn.h"

namespace {

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

constexpr float kPi = 3.14159265f;
constexpr float kDeg = kPi / 180.0f;

struct Q {
    float v[4] = {0.0f, 0.0f, 0.0f, 1.0f};
};
Q Mul(const Q& a, const Q& b) {
    Q out;
    viewturn::QuatMul(a.v, b.v, out.v);
    return out;
}
Q YawQ(float r) {
    Q q;
    q.v[1] = std::sin(r / 2), q.v[3] = std::cos(r / 2);
    return q;
}
Q PitchQ(float r) {
    Q q;
    q.v[0] = std::sin(r / 2), q.v[3] = std::cos(r / 2);
    return q;
}
Q RollQ(float r) {
    Q q;
    q.v[2] = std::sin(r / 2), q.v[3] = std::cos(r / 2);
    return q;
}
// A head: a yaw, then a pitch, then a roll (degrees).
Q Head(float yaw, float pitch = 0.0f, float roll = 0.0f) {
    return Mul(Mul(YawQ(yaw * kDeg), PitchQ(pitch * kDeg)), RollQ(roll * kDeg));
}

// A camera of BF3's at its yaw (degrees, growing right), pitched and rolled in its own frame, the w lanes filled as a scripted camera's (or 0: the
// player's kind), at a place. Its level rows by viewturn::PureRows' formula, then turned in its own frame (viewturn::TurnLocal).
void Camera(float* m, float yaw, float pitch, float roll, const float* w = nullptr, int hand = 1) {
    const float s = std::sin(yaw * kDeg), c = std::cos(yaw * kDeg);
    float rows[16] = {-c, 0.0f, -s, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, s, 0.0f, -c, 0.0f, 12.5f, 80.0f, -2078.8f, 1.0f};
    const Q local = Mul(PitchQ(pitch * kDeg), RollQ(roll * kDeg));
    viewturn::TurnLocal(rows, local.v);
    if (hand < 0)
        for (int i = 0; i < 3; ++i)
            rows[i] = -rows[i];  // (its right row the other way: a mirrored camera)
    if (w)
        rows[3] = w[0], rows[7] = w[1], rows[11] = w[2];
    else
        rows[3] = rows[7] = rows[11] = 0.0f;
    std::memcpy(m, rows, sizeof(rows));
}

// A world direction's coordinates in a camera's own frame (along its right, up, back rows), as its picture shows it.
void InCamera(const float* m, const float* world, float* local) {
    for (int r = 0; r < 3; ++r)
        local[r] = world[0] * m[4 * r] + world[1] * m[4 * r + 1] + world[2] * m[4 * r + 2];
}
// Where a world direction is shown in the room by a picture from turned camera m handed over with head h: its yaw (degrees, left positive) and pitch.
void ShownAt(const float* m, const Q& h, const float* world, float& yaw, float& pitch) {
    float local[3], room[3];
    InCamera(m, world, local);
    viewturn::QuatRotate(h.v, local, room);
    const float len = std::sqrt(room[0] * room[0] + room[1] * room[1] + room[2] * room[2]);
    yaw = std::atan2(-room[0], -room[2]) / kDeg;
    pitch = std::asin(std::fmax(-1.0f, std::fmin(1.0f, room[1] / len))) / kDeg;
}
// The forward along the ground of a camera made level from m (BF3's yaw, as MakeLevel takes it when it's sure: its back row along the ground).
void LevelForward(const float* m, float* f) {
    const float x = m[8], z = m[10], len = std::sqrt(x * x + z * z);
    f[0] = -x / len, f[1] = 0.0f, f[2] = -z / len;
}
float Wrap(float degrees) {
    return std::remainder(degrees, 360.0f);
}

camera::ViewTurn TurnFor(const Q& head, unsigned scene = 1) {
    camera::ViewTurn t{};
    std::memcpy(t.head, head.v, sizeof(t.head));
    std::memcpy(t.smoothHead, head.v, sizeof(t.smoothHead));
    t.controllerYaw = 0.0f;
    t.haveLook = false;
    t.lookSign = -1.0f;
    t.sceneNumber = scene;
    return t;
}

// camera.cpp's TurnToHead for a scripted camera, in short: the scene's latch, then the turn. Returns the yaw taken out (the picture body's taken).
float TurnScripted(float* m, const camera::ViewTurn& turn, sceneview::Latch& latch, viewturn::Level& level, bool facing = true, int hand = 1,
                   bool mirrored = false, bool* fresh = nullptr) {
    float sceneYaw = 0.0f;
    bool latched = false;
    if (facing)
        sceneYaw = sceneview::LatchYaw(latch, turn.sceneNumber, turn.head, latched);
    if (fresh)
        *fresh = latched;
    return viewturn::Turn(m, turn, true, hand, mirrored, level, nullptr, sceneYaw);
}

// The scripted turn before this change (viewturn.h at f1552ad): made level, the head put on as it is, mirrored across the right row if so.
void OldScripted(float* m, const camera::ViewTurn& turn, int hand, bool mirrored, viewturn::Level& level) {
    viewturn::MakeLevel(m, hand, level);
    float local[4];
    std::memcpy(local, turn.head, sizeof(local));
    if (mirrored) {
        local[1] = -local[1];
        local[2] = -local[2];
    }
    viewturn::TurnLocal(m, local);
}

const float kBreachW[3] = {-1.022f, 0.942f, -0.175f};  // the breach's camera's w lanes, as logged (03:25:03.536)

// ------------------------------------------------------------------ 1 ------------------------------------------------------------------

void RowHeadYaw() {
    float worst = 0.0f;
    int cases = 0;
    for (int y = -180; y < 180; y += 5)
        for (int p = -89; p <= 89; p += 8)
            for (int r = -60; r <= 60; r += 15) {
                const Q h = Head(static_cast<float>(y), static_cast<float>(p), static_cast<float>(r));
                const float got = sceneview::HeadYaw(h.v) / kDeg;
                worst = std::fmax(worst, std::fabs(Wrap(got - static_cast<float>(y))));
                ++cases;
            }
    Report("1a", "HeadYaw: a yaw, a pitch, a roll gives the yaw back", worst < 0.01f, "%d heads, the worst %.4f degrees off", cases, worst);
    // Straight down / up: the way the top points (looking down it points forward, looking up back).
    const float down = sceneview::HeadYaw(Head(40.0f, -90.0f).v) / kDeg, up = sceneview::HeadYaw(Head(-120.0f, 90.0f).v) / kDeg;
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float bad[4] = {nan, 0.0f, 0.0f, 1.0f};
    const float none = sceneview::HeadYaw(bad);
    Report("1b", "HeadYaw straight down and up by the way the top points; not a number 0", std::fabs(Wrap(down - 40.0f)) < 0.05f &&
           std::fabs(Wrap(up + 120.0f)) < 0.05f && none == 0.0f, "down %.3f (wanted 40), up %.3f (wanted -120), NaN %.1f", down, up, none);
}

// ------------------------------------------------------------------ 2 ------------------------------------------------------------------

void RowSame() {
    std::mt19937 rng(7);
    std::uniform_real_distribution<float> any(-180.0f, 180.0f), pitch(-75.0f, 75.0f), roll(-40.0f, 40.0f), yaw(-3.0f, 3.0f);
    float worst = 0.0f;
    int mismatch = 0, playerMismatch = 0;
    for (int i = 0; i < 20000; ++i) {
        const Q head = Head(any(rng), pitch(rng), roll(rng));
        const float y = yaw(rng);
        float a[4], b[4];
        sceneview::SceneLocal(head.v, y, a);
        viewturn::QuatMul(YawQ(-y).v, head.v, b);
        for (int k = 0; k < 4; ++k)
            worst = std::fmax(worst, std::fabs(a[k] - b[k]));
        // The yaw 0: the turn before, bit for bit (both hands, mirrored or not).
        const int hand = i % 4 == 3 ? -1 : 1;
        const bool mirrored = hand < 0 && i % 8 == 7;
        float m[16], old[16];
        Camera(m, any(rng), pitch(rng), roll(rng), kBreachW, hand);
        std::memcpy(old, m, sizeof(m));
        viewturn::Level la, lb;
        const camera::ViewTurn turn = TurnFor(head);
        viewturn::Turn(m, turn, true, hand, mirrored, la, nullptr, 0.0f);
        OldScripted(old, turn, hand, mirrored, lb);
        mismatch += std::memcmp(m, old, sizeof(m)) != 0;
        // The player's kind (w lanes 0): bit for bit whatever the yaw.
        float p[16], q[16];
        Camera(p, any(rng), pitch(rng), 0.0f);
        std::memcpy(q, p, sizeof(p));
        camera::ViewTurn look = TurnFor(head);
        look.haveLook = i % 2 == 0;
        look.lookYaw = any(rng) * kDeg;
        look.currentYaw = any(rng) * kDeg;
        viewturn::Level lc, ld;
        const float takenP = viewturn::Turn(p, look, false, 1, false, lc, nullptr, y);
        const float takenQ = viewturn::Turn(q, look, false, 1, false, ld);
        playerMismatch += std::memcmp(p, q, sizeof(p)) != 0 || takenP != takenQ;
    }
    Report("2", "SceneLocal is yaw(-y) times the head; the yaw 0 the scripted turn before, bit for bit; the player's camera bit for bit whatever the yaw",
           worst < 1e-6f && mismatch == 0 && playerMismatch == 0, "worst %.2e; 20000 scripted turns, %d not bit for bit; 20000 of the player's, %d", worst,
           mismatch, playerMismatch);
}

// ------------------------------------------------------------------ 3, 4 ------------------------------------------------------------------

void RowBreach() {
    // The breach's camera, and its level forward in the world.
    float scene[16];
    Camera(scene, -80.0f, -6.1f, -1.2f, kBreachW);
    float forward[3];
    LevelForward(scene, forward);
    const Q face = Head(85.0f, -5.0f);  // his head: 85 left of the room's straight ahead, a little down at the door
    // As it was: the scene's forward at the room's.
    float before[16];
    std::memcpy(before, scene, sizeof(scene));
    sceneview::Latch none;
    viewturn::Level l0;
    const float takenBefore = TurnScripted(before, TurnFor(face), none, l0, false);
    float yawBefore = 0.0f, pitchBefore = 0.0f;
    ShownAt(before, face, forward, yawBefore, pitchBefore);
    // Now: the first picture's four cameras (both eyes' main and cull cameras: one turn, one latch).
    sceneview::Latch latch;
    viewturn::Level level;
    float cams[4][16];
    int fresh = 0;
    float taken[4] = {};
    for (int c = 0; c < 4; ++c) {
        std::memcpy(cams[c], scene, sizeof(scene));
        viewturn::Level frame = level;  // (camera.cpp latches the last frame's level for all of a frame's cameras)
        bool f = false;
        taken[c] = TurnScripted(cams[c], TurnFor(face, 7), latch, frame, true, 1, false, &f);
        fresh += f;
        if (c == 3)
            level = frame;
    }
    float yawNow = 0.0f, pitchNow = 0.0f;
    ShownAt(cams[0], face, forward, yawNow, pitchNow);
    const bool alike = std::memcmp(cams[0], cams[1], sizeof(cams[0])) == 0 && std::memcmp(cams[0], cams[2], sizeof(cams[0])) == 0 &&
                       std::memcmp(cams[0], cams[3], sizeof(cams[0])) == 0 && taken[0] == taken[3];
    Report("3a", "the owner's breach as it was: the scene's forward shown at the room's straight ahead, 85 degrees right of his face",
           std::fabs(Wrap(yawBefore - 0.0f)) < 0.01f && std::fabs(Wrap(yawBefore - 85.0f)) > 84.9f && takenBefore == 0.0f,
           "shown at %.2f degrees in the room, his face at 85.00: %.2f off", yawBefore, Wrap(yawBefore - 85.0f));
    Report("3b", "... now: at his face from the scene's first picture, one latch for its four cameras", std::fabs(Wrap(yawNow - 85.0f)) < 0.01f &&
           std::fabs(pitchNow) < 0.01f && fresh == 1 && alike && std::fabs(taken[0] / kDeg - 85.0f) < 0.01f,
           "shown at %.3f degrees (his face 85), pitch %.3f (level); latched %d time(s), the four cameras alike %d, the yaw taken out %.3f", yawNow, pitchNow,
           fresh, alike, taken[0] / kDeg);
    // The head about after it: the scene stays put in the room, its forward at the latch, level; the head looks about it.
    const Q heads[4] = {Head(115.0f), Head(85.0f, 20.0f), Head(40.0f, 0.0f, 25.0f), Head(85.0f, -5.0f)};
    float worstYaw = 0.0f, worstPitch = 0.0f, worstUp = 0.0f;
    int again = 0;
    for (const Q& h : heads) {
        float m[16];
        std::memcpy(m, scene, sizeof(scene));
        viewturn::Level frame = level;
        bool f = false;
        TurnScripted(m, TurnFor(h, 7), latch, frame, true, 1, false, &f);
        again += f;
        float y = 0.0f, p = 0.0f;
        ShownAt(m, h, forward, y, p);
        worstYaw = std::fmax(worstYaw, std::fabs(Wrap(y - 85.0f)));
        worstPitch = std::fmax(worstPitch, std::fabs(p));
        // The world's up shown as the room's up (the horizon level).
        const float worldUp[3] = {0.0f, 1.0f, 0.0f};
        float local[3], room[3];
        InCamera(m, worldUp, local);
        viewturn::QuatRotate(h.v, local, room);
        worstUp = std::fmax(worstUp, std::fabs(room[0]) + std::fabs(room[2]));
    }
    Report("3c", "... the head about after it (30 left, 20 up, 45 right and rolled): the scene stays where it is in the room, level; never latched again",
           worstYaw < 0.01f && worstPitch < 0.01f && worstUp < 1e-5f && again == 0,
           "its forward at most %.4f degrees from the latch, %.4f from level; the world's up off the room's by %.1e; latched again %d", worstYaw, worstPitch, worstUp,
           again);

    // 4: the scene's end. The player's camera comes back at the scene's last yaw (the player's view where the scene left it), the picture body holding
    // the yaw taken out (xr_runtime.cpp: the held yaw); ViewTurnLocal takes the held yaw out of it.
    for (int now = 0; now < 2; ++now) {
        float player[16];
        Camera(player, -80.0f, -6.1f, 0.0f);  // (the player's kind: w lanes 0)
        float pf[3];
        LevelForward(player, pf);
        camera::ViewTurn back = TurnFor(face);
        back.controllerYaw = now ? taken[0] : takenBefore;  // (camera.cpp's ShownTurn taken, held through the scene)
        viewturn::Level l;
        viewturn::Turn(player, back, false, 1, false, l);
        float y = 0.0f, p = 0.0f;
        ShownAt(player, face, pf, y, p);
        const float sceneAt = now ? yawNow : yawBefore;
        if (now)
            Report("4a", "the scene's end: the player's camera back where the scene's forward was shown (no jump), at his face", std::fabs(Wrap(y - sceneAt)) < 0.01f &&
                   std::fabs(Wrap(y - 85.0f)) < 0.01f, "the player's forward at %.3f degrees, the scene's was at %.3f", y, sceneAt);
        else
            Report("4b", "... as it was: no jump either (both at the room's straight ahead), 85 degrees off his face", std::fabs(Wrap(y - sceneAt)) < 0.01f &&
                   std::fabs(Wrap(y - 0.0f)) < 0.01f, "the player's forward at %.3f degrees, the scene's was at %.3f", y, sceneAt);
    }
}

// ------------------------------------------------------------------ 5 ------------------------------------------------------------------

void RowRide() {
    // 100 s at 72 frames a second: the car turning slowly (its yaw a degree a second), a cut at 40 s (45 degrees), the head sweeping 90 left and right,
    // up and down. The latch at the first frame (the head 12 degrees left then), never again.
    sceneview::Latch latch;
    viewturn::Level level;
    int fresh = 0;
    float worst = 0.0f, latched = 0.0f;
    for (int frame = 0; frame < 7200; ++frame) {
        const float t = frame / 72.0f;
        const float carYaw = 56.8f + t + (t >= 40.0f ? 45.0f : 0.0f);
        float m[16];
        Camera(m, carYaw, -1.9f + 2.0f * std::sin(t), 3.2f * std::sin(0.7f * t), kBreachW);
        float forward[3];
        LevelForward(m, forward);
        const Q head = Head(12.0f + 90.0f * std::sin(0.3f * t), 25.0f * std::sin(0.11f * t), 10.0f * std::sin(0.5f * t));
        bool f = false;
        const float taken = TurnScripted(m, TurnFor(head, 3), latch, level, true, 1, false, &f);
        fresh += f;
        if (frame == 0)
            latched = taken / kDeg;
        float y = 0.0f, p = 0.0f;
        ShownAt(m, head, forward, y, p);
        worst = std::fmax(worst, std::fabs(Wrap(y - latched)));
    }
    Report("5", "a ride (100 s, the head looking about, the car turning and cutting): one latch, the scene's forward always at it",
           fresh == 1 && std::fabs(latched - 12.0f) < 0.01f && worst < 0.01f, "latched %d time(s), at %.3f degrees (the head's 12 then); the forward at most %.4f "
           "degrees from it", fresh, latched, worst);
}

// ------------------------------------------------------------------ 6 ------------------------------------------------------------------

// Frames at 72 a second through runs of cutscene pictures (true) and play (false), each this long (ms); returns the numbers seen at given times.
struct Timeline {
    sceneview::Scenes s;
    unsigned long long now = 1000000;
    unsigned last = 0;
    int changes = 0;
    void Run(bool cutscene, unsigned long long ms) {
        const unsigned long long until = now + ms;
        while (now < until) {
            const unsigned n = sceneview::StepScenes(s, now, cutscene);
            changes += last && n != last;
            last = n;
            now += 14;
        }
    }
};

void RowNumbers() {
    {
        Timeline t;
        t.Run(false, 5000);  // play before any scene
        const unsigned first = t.last;
        t.Run(true, 2000);  // a scene
        const unsigned inScene = t.last;
        t.Run(false, 500);
        t.Run(true, 1000);  // half a second of play: the same scene
        const unsigned sameAfterHalf = t.last;
        t.Run(false, 986);  // just under a second
        const unsigned underSecond = t.last;
        t.Run(false, 28);  // past it
        const unsigned overSecond = t.last;
        Report("6a", "the numbering: no change in play before the first scene; under a second of play one scene; a second a new one",
               first == 1 && inScene == 1 && sameAfterHalf == 1 && underSecond == 1 && overSecond == 2 && t.changes == 1,
               "before %u, in %u, after 0.5 s %u, after 0.99 s %u, after 1.01 s %u; %d change(s)", first, inScene, sameAfterHalf, underSecond, overSecond, t.changes);
    }
    {
        // The next scene's first scripted cameras come before g_cutscene says so (three pictures in a row): the number is up already from the play before.
        Timeline t;
        t.Run(true, 3000);
        t.Run(false, 4000);  // play
        const unsigned beforeScene = t.last;
        // The scene: its first two pictures are play as far as g_cutscene knows; the view builder's first scripted camera latches with this number.
        sceneview::Latch latch;
        bool fresh = false;
        sceneview::LatchYaw(latch, 1, Head(0.0f).v, fresh);  // (the last scene's latch)
        t.Run(false, 28);
        const unsigned firstCamera = t.last;
        sceneview::LatchYaw(latch, firstCamera, Head(33.0f).v, fresh);
        t.Run(true, 5000);
        Report("6b", "a scene's first scripted camera, two pictures before g_cutscene: its number new already, latched at it", beforeScene == 2 && firstCamera == 2 &&
               fresh && std::fabs(latch.yaw / kDeg - 33.0f) < 0.01f && t.last == 2, "the number in play %u, at the first camera %u, latched %d at %.2f", beforeScene,
               firstCamera, fresh, latch.yaw / kDeg);
    }
    {
        // Comrades: the load (unturned), 16 pictures of the player's camera (0.22 s), the scripted drive (100 s): one scene.
        Timeline t;
        t.Run(false, 3000);
        t.Run(true, 5000);
        const unsigned load = t.last;
        t.Run(false, 224);
        t.Run(true, 100000);
        Report("6c", "Comrades' load, its 16 pictures of the player's camera before the cut into the car, the drive: one scene", t.last == load && t.changes == 0,
               "the load's %u, the drive's %u, %d change(s)", load, t.last, t.changes);
    }
    {
        // The interrogation's retries (screenshots\logs\bf3vr_0155_interrogation_cutsceneturn.log): scripted 22.5 s, unturned 6.3 s, play 1.04 s, again.
        Timeline t;
        t.Run(true, 22500);
        t.Run(true, 6300);
        t.Run(false, 1040);
        const unsigned retry = t.last;
        t.Run(true, 22500);
        t.Run(true, 6200);
        t.Run(false, 1060);
        Report("6d", "the interrogation's retries: a second of play between, each retry a new scene (its own latch, where the head faces then)",
               retry == 2 && t.last == 3, "the first %u, the retry %u, the next %u", 1u, retry, t.last);
    }
    {
        // Semper Fidelis (build\logs\bf3vr_1118_sfplay.log 05:43:09-05:43:23): a death's camera 8 s, the load 4.5 s, the player's camera 1.2 s, the
        // checkpoint's own scene: the death and the load one scene, the checkpoint's another.
        Timeline t;
        t.Run(false, 20000);
        t.Run(true, 8000);
        const unsigned death = t.last;
        t.Run(true, 4500);
        const unsigned load = t.last;
        t.Run(false, 1200);
        t.Run(true, 9000);
        Report("6e", "Semper Fidelis: a death's camera and the load after it one scene, the checkpoint's own scene after 1.2 s of play a new one",
               death == 1 && load == 1 && t.last == 2, "the death %u, the load %u, the checkpoint's scene %u", death, load, t.last);
    }
    {
        // A recenter in a scene: the next scripted camera latches afresh, the head in the recentered room (its yaw there about 0).
        Timeline t;
        t.Run(true, 2000);
        sceneview::Latch latch;
        bool fresh = false;
        sceneview::LatchYaw(latch, t.last, Head(85.0f).v, fresh);
        sceneview::NewScene(t.s);
        t.Run(true, 2000);
        bool again = false;
        sceneview::LatchYaw(latch, t.last, Head(0.4f).v, again);
        // The clock going back: taken as play since, a new scene.
        sceneview::Scenes c;
        sceneview::StepScenes(c, 5000, true);
        const unsigned back = sceneview::StepScenes(c, 1000, false);
        Report("6f", "a recenter: a new scene, the next scripted camera latched afresh in the recentered room; the clock going back: a new scene",
               again && std::fabs(latch.yaw / kDeg - 0.4f) < 0.01f && t.last == 2 && back == 2, "latched again %d at %.2f, the number %u; after the clock went back %u",
               again, latch.yaw / kDeg, t.last, back);
    }
}

// ------------------------------------------------------------------ 7, 8, 9 ------------------------------------------------------------------

void RowPlay() {
    // In play: the player's camera (the look written and not), with a scene latched long before: bit for bit as without any scene.
    std::mt19937 rng(11);
    std::uniform_real_distribution<float> any(-180.0f, 180.0f), pitch(-70.0f, 70.0f);
    int differ = 0;
    sceneview::Latch latch;
    bool fresh = false;
    sceneview::LatchYaw(latch, 4, Head(85.0f).v, fresh);
    int latchedInPlay = 0;
    for (int i = 0; i < 20000; ++i) {
        float a[16], b[16];
        Camera(a, any(rng), pitch(rng), 0.0f);
        std::memcpy(b, a, sizeof(a));
        camera::ViewTurn t = TurnFor(Head(any(rng), pitch(rng)), 5);
        t.haveLook = i % 3 != 0;
        t.lookYaw = any(rng) * kDeg;
        t.currentYaw = any(rng) * kDeg;
        t.controllerYaw = any(rng) * kDeg;
        // (camera.cpp: the latch only for a scripted camera; the player's kind never reaches it)
        const bool scripted = viewturn::Classify(a, true) == viewturn::kScripted;
        if (scripted)
            sceneview::LatchYaw(latch, t.sceneNumber, t.head, fresh), ++latchedInPlay;
        viewturn::Level la, lb;
        const float ta = viewturn::Turn(a, t, scripted, 1, false, la, nullptr, scripted ? latch.yaw : 0.0f);
        const float tb = viewturn::Turn(b, t, false, 1, false, lb);
        differ += std::memcmp(a, b, sizeof(a)) != 0 || ta != tb;
    }
    Report("7", "in play: the player's camera turned bit for bit as before with a scene latched; none of it scripted, no latch", differ == 0 && latchedInPlay == 0 &&
           latch.number == 4, "%d of 20000 differ; latched in play %d", differ, latchedInPlay);
}

void RowMirrored() {
    std::mt19937 rng(13);
    std::uniform_real_distribution<float> any(-180.0f, 180.0f), pitch(-40.0f, 40.0f), roll(-30.0f, 30.0f);
    float worst = 0.0f;
    for (int i = 0; i < 5000; ++i) {
        const float yaw = any(rng), p = pitch(rng), r = roll(rng);
        float twin[16], mirror[16];
        Camera(twin, yaw, p, r, kBreachW, 1);
        Camera(mirror, yaw, p, r, kBreachW, -1);
        const Q head = Head(any(rng), pitch(rng), roll(rng));
        sceneview::Latch lt, lm;
        viewturn::Level vt, vm;
        TurnScripted(twin, TurnFor(head, 9), lt, vt, true, 1, false);
        TurnScripted(mirror, TurnFor(head, 9), lm, vm, true, -1, true);
        for (int k = 0; k < 3; ++k) {
            worst = std::fmax(worst, std::fabs(mirror[k] + twin[k]));          // the right row the other way
            worst = std::fmax(worst, std::fabs(mirror[4 + k] - twin[4 + k]));  // the up the same
            worst = std::fmax(worst, std::fabs(mirror[8 + k] - twin[8 + k]));  // the back (the forward) the same
        }
    }
    Report("8", "mirrored: turned with the latch, the same forward and up as its twin turned with it, the right row the other way", worst < 1e-5f,
           "5000 cameras, the worst %.2e", worst);
}

void RowLean() {
    // camera.cpp's NoteFrameBody: the turned camera with the head's turn taken back off it, its right and back along the ground: headmove.h's body frame.
    float scene[16];
    Camera(scene, -80.0f, -6.1f, -1.2f, kBreachW);
    float forward[3];
    LevelForward(scene, forward);
    const Q face = Head(85.0f);
    for (int now = 0; now < 2; ++now) {
        float m[16];
        std::memcpy(m, scene, sizeof(scene));
        sceneview::Latch latch;
        viewturn::Level level;
        TurnScripted(m, TurnFor(face, 2), latch, level, now != 0);
        const float inverse[4] = {-face.v[0], -face.v[1], -face.v[2], face.v[3]};
        viewturn::TurnLocal(m, inverse);
        const float rl = std::sqrt(m[0] * m[0] + m[2] * m[2]), bl = std::sqrt(m[8] * m[8] + m[10] * m[10]);
        const float right[3] = {m[0] / rl, 0.0f, m[2] / rl}, back[3] = {m[8] / bl, 0.0f, m[10] / bl};
        // A lean of 10 cm along the face (the room's forward turned to 85 left: x -sin, z -cos), into the world as headmove.h puts it.
        const float room[3] = {-0.1f * std::sin(85.0f * kDeg), 0.0f, -0.1f * std::cos(85.0f * kDeg)};
        float world[3];
        for (int i = 0; i < 3; ++i)
            world[i] = room[0] * right[i] + room[2] * back[i];
        const float along = world[0] * forward[0] + world[2] * forward[2];
        const float off = std::sqrt(std::fmax(0.0f, world[0] * world[0] + world[2] * world[2] - along * along));
        const float angle = std::atan2(off, along) / kDeg;
        if (now)
            Report("9a", "the head's place: a lean of 10 cm along the face moves the scene's camera 10 cm along the scene's forward", std::fabs(along - 0.1f) < 0.001f &&
                   off < 0.001f, "%.1f mm along it, %.1f mm off it (%.2f degrees)", along * 1000.0f, off * 1000.0f, angle);
        else
            Report("9b", "... as it was: 85 degrees off the scene's forward", std::fabs(angle - 85.0f) < 0.1f, "%.1f mm along it, %.1f mm off it (%.2f degrees)",
                   along * 1000.0f, off * 1000.0f, angle);
    }
}

void RowTime() {
    float scene[16];
    Camera(scene, -80.0f, -6.1f, -1.2f, kBreachW);
    sceneview::Latch latch;
    viewturn::Level level;
    float sink = 0.0f;
    const int n = 200000;
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < n; ++i) {
        float m[16];
        std::memcpy(m, scene, sizeof(scene));
        sink += TurnScripted(m, TurnFor(Head(static_cast<float>(i % 360)), static_cast<unsigned>(i / 1000)), latch, level);
        sink += m[8];
    }
    const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count() / n;
    Report("10", "the time: a latch and a scripted camera's turn", us < 1.0, "%.3f us a call (%.1f)", us, sink * 0.0f);
}

}  // namespace

int main() {
    RowHeadYaw();
    RowSame();
    RowBreach();
    RowRide();
    RowNumbers();
    RowPlay();
    RowMirrored();
    RowLean();
    RowTime();
    std::printf("%d checks, %d failed\n", g_counted, g_failed);
    return g_failed;
}

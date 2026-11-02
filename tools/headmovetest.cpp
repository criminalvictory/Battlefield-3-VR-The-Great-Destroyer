// The head's place moving the game's cameras (src/headmove.h, as views.cpp runs it), on scripted heads and a simulated
// soldier at 72 frames a second: a still head stays still, leaning one to one and never past the limit, a turned or bowed
// head walks and crouches nothing, the soldier walked under a step and only as far as it really goes, stopped by a wall,
// the player's stick winning, crouching for real without the view dropping twice, the button's crouch left to the game,
// the smooth turn about the head, cutscenes, tracking lost and found, a recenter, the soldier put somewhere else.
#define HEADMOVE_CORE_ONLY
#include "../src/headmove.h"

#include <chrono>
#include <cstdio>

namespace {

using namespace headmove;

int g_checks = 0, g_failed = 0;

void Check(bool ok, const char* name, double got = 0.0, double want = 0.0) {
    ++g_checks;
    if (!ok) {
        ++g_failed;
        std::printf("FAIL %s (got %.4f, wanted %.4f)\n", name, got, want);
    }
}

constexpr float kDt = 1.0f / 72.0f;
constexpr float kPi = 3.14159265f;

// A soldier and a room: the body's yaw (left positive), the way the game walks from the room's straight ahead, the
// world's place of the game's camera, and what a stick does to it.
struct Sim {
    State s;
    Tuning t;
    Output out;
    Sim() { t.putDownAfter = 1e9f; }  // these heads are as still as a headset on a desk: PutDown has its own
    double eye[3] = {100.0, 68.0, -40.0};
    float bodyYaw = 0.0f;
    float facing = 0.0f;
    float head[3] = {0.0f, 0.0f, 0.0f};
    float q[4] = {0, 0, 0, 1};
    bool tracked = true, scripted = false, stick = false, recentered = false;
    bool blocked = false;    // a wall: the stick moves nothing
    float eyeDropping = 0.0f;  // the game's crouch: its eye eases to this far below standing
    float gameDrop = 0.0f;
    double eyeStanding = 68.0;

    void Rows(float* right, float* back) const {
        // The room's axes in the world: its x right, z back, turned left by the body's yaw about the world's up.
        const float c = std::cos(bodyYaw), sn = std::sin(bodyYaw);
        right[0] = c, right[1] = 0.0f, right[2] = -sn;
        back[0] = sn, back[1] = 0.0f, back[2] = c;
    }

    void Frame(float dt = kDt) {
        Input in;
        in.dt = dt;
        for (int i = 0; i < 3; ++i)
            in.head[i] = head[i], in.eye[i] = eye[i];
        for (int i = 0; i < 4; ++i)
            in.headQ[i] = q[i];
        in.tracked = tracked;
        Rows(in.right, in.back);
        in.scripted = scripted;
        in.stick = stick;
        in.facingYaw = facing;
        in.recentered = recentered;
        recentered = false;
        Step(s, t, in, out);
        // The game then moves: the virtual stick walks the soldier, its crouch eases its eye (0.6 m in 0.25 s).
        if (out.following && !blocked) {
            const float c = std::cos(facing), sn = std::sin(facing);
            // From the way the game walks to the room: back the other way round from Step's.
            const float x = out.stickX * c - out.stickY * sn, y = out.stickX * sn + out.stickY * c;
            const float length = std::sqrt(x * x + y * y);
            // As the game walks: nothing below the least stick, then its speed over the whole stick's.
            const float speed = length > 1e-5f ? std::max(0.0f, length - t.stickLeast) * t.stickFull / length : 0.0f;
            for (int i = 0; i < 3; i += 2)
                eye[i] += (x * in.right[i] - y * in.back[i]) * speed * dt;
        }
        const float wanted = out.crouch || buttonCrouch ? 0.6f : 0.0f;
        const float stepDrop = 2.4f * dt;
        gameDrop = gameDrop < wanted ? std::min(wanted, gameDrop + stepDrop) : std::max(wanted, gameDrop - stepDrop);
        eye[1] = eyeStanding - gameDrop;
    }

    bool buttonCrouch = false;

    // A head moves there in its own time (one step that far would be taken for tracking found again).
    void MoveTo(float x, float y, float z, float seconds = 0.5f) {
        const float from[3] = {head[0], head[1], head[2]};
        const int n = std::max(1, static_cast<int>(seconds * 72));
        for (int i = 1; i <= n; ++i) {
            const float k = static_cast<float>(i) / n;
            head[0] = from[0] + (x - from[0]) * k;
            head[1] = from[1] + (y - from[1]) * k;
            head[2] = from[2] + (z - from[2]) * k;
            Frame();
        }
    }

    // Where the camera is in the world: the game's camera as Step saw it plus the move (Frame moved the soldier since).
    double lastEye[3] = {};
    void FrameKept(float dt = kDt) {
        for (int i = 0; i < 3; ++i)
            lastEye[i] = eye[i];
        Frame(dt);
    }
    double Camera(int i) const { return lastEye[i] + out.world[i]; }
};

void Yaw(float* q, float yaw, float pitch = 0.0f) {
    const float a[4] = {0.0f, std::sin(yaw / 2), 0.0f, std::cos(yaw / 2)};
    const float b[4] = {std::sin(pitch / 2), 0.0f, 0.0f, std::cos(pitch / 2)};
    q[0] = a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1];
    q[1] = a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0];
    q[2] = a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3];
    q[3] = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];
}

void StillHead() {
    Sim m;
    float most = 0.0f;
    for (int i = 0; i < 72 * 60; ++i) {
        m.Frame();
        for (float v : m.out.room)
            most = std::max(most, std::fabs(v));
        Check(!m.out.following && !m.out.crouch, "still head: nothing walked or crouched");
    }
    Check(most < 1e-5f, "still head: no move, no drift", most, 0.0);
}

void Lean() {
    Sim m;
    m.t.walk = false;
    m.bodyYaw = 0.5f;
    m.Frame();
    for (int k = 0; k < 40; ++k) {
        m.head[0] = 0.005f * k;
        m.Frame();
        Check(std::fabs(m.out.room[0] - m.head[0]) < 1e-3f, "lean: one to one inside the limit", m.out.room[0], m.head[0]);
    }
    // In the world along the body's right, whatever the head's turn.
    float right[3], back[3];
    m.Rows(right, back);
    Yaw(m.q, 1.2f);
    m.head[0] = 0.15f;
    m.head[1] = 0.05f;
    m.head[2] = -0.10f;
    m.Frame();
    const float want[3] = {0.15f * right[0] - 0.10f * back[0], 0.05f, 0.15f * right[2] - 0.10f * back[2]};
    for (int i = 0; i < 3; ++i)
        Check(std::fabs(m.out.world[i] - want[i]) < 2e-3f, "lean: along the body's axes, not the head's", m.out.world[i], want[i]);
}

void NeckDecides() {
    // The head turned and bowed about a still neck: the eyes go round it. Nothing walks, nothing crouches.
    Sim m;
    const float neck[3] = {0.0f, -kNeckDown, kNeckBack};
    m.Frame();
    bool any = false;
    float widest = 0.0f;
    for (int i = 0; i < 72 * 8; ++i) {
        const float yaw = 3.0f * std::sin(i * 0.02f);
        const float pitch = -1.2f * std::fabs(std::sin(i * 0.013f));
        Yaw(m.q, yaw, pitch);
        const float eyeAt[3] = {0.0f, kNeckDown, -kNeckBack};
        float turned[3];
        Rotate(m.q, eyeAt, turned);
        for (int k = 0; k < 3; ++k)
            m.head[k] = neck[k] + turned[k];
        m.Frame();
        any = any || m.out.following || m.out.crouch;
        widest = std::max(widest, std::sqrt(m.out.room[0] * m.out.room[0] + m.out.room[2] * m.out.room[2]));
    }
    Check(!any, "head turned and bowed about the neck: nothing walked or crouched");
    Check(widest > 0.05f, "head turned: the eyes' own move still shown (real parallax)", widest, 0.05);
}

void Limit() {
    Sim m;
    m.t.walk = false;
    m.Frame();
    float last = 0.0f, biggest = 0.0f, most = 0.0f;
    for (int i = 0; i < 72 * 3; ++i) {
        m.head[0] = 0.4f * i * kDt;  // out past the limit at 0.4 m/s
        m.Frame();
        const float r = m.out.room[0];
        biggest = std::max(biggest, std::fabs(r - last));
        most = std::max(most, r);
        last = r;
        Check(r >= -1e-4f, "limit: never the other way");
    }
    Check(most <= m.t.leanMost + 1e-4f, "limit: never past it", most, m.t.leanMost);
    Check(most > m.t.leanMost - 0.02f, "limit: gets near it", most, m.t.leanMost);
    Check(biggest <= 0.4f * kDt + 1e-3f, "limit: no jump in the view", biggest, 0.4f * kDt);
    // Stood still far out: the neutral place eases to the head, gently.
    float worst = 0.0f;
    for (int i = 0; i < 72 * 6; ++i) {
        m.Frame();
        worst = std::max(worst, std::fabs(m.out.room[0] - last));
        last = m.out.room[0];
    }
    Check(std::fabs(last) < 0.015f, "far out and still: eased back to the middle", last, 0.0);
    Check(worst < 0.012f, "far out and still: gently", worst, 0.012);
    // The head going back the other way then moves the view at once, one to one.
    const float before = m.out.room[0];
    m.head[0] -= 0.05f;
    m.Frame();
    Check(std::fabs(m.out.room[0] - (before - 0.05f)) < 0.002f, "and moves one to one again", m.out.room[0], before - 0.05f);
}

void Height() {
    Sim m;
    m.t.crouch = false;
    m.Frame();
    float most = 0.0f;
    for (int i = 0; i < 72 * 2; ++i) {
        m.head[1] = 0.5f * i * kDt;
        m.Frame();
        most = std::max(most, m.out.room[1]);
    }
    Check(most <= m.t.upMost + 1e-4f, "tiptoe: never past the upper limit", most, m.t.upMost);
    // Having dragged the neutral height up (a headset put on): still a moment, and the view's back at the game's height.
    for (int i = 0; i < 72 * 5; ++i)
        m.Frame();
    Check(std::fabs(m.out.room[1]) < 0.015f, "headset put on: the height settles", m.out.room[1], 0.0);
    // Ducking is one to one, down to well past a crouch.
    const float base = m.head[1];
    m.MoveTo(m.head[0], base - 0.9f, m.head[2], 1.0f);
    Check(std::fabs(m.out.room[1] + 0.9f) < 0.02f, "duck: one to one", m.out.room[1], -0.9);
    m.MoveTo(m.head[0], base - 2.5f, m.head[2], 2.0f);
    Check(m.out.room[1] >= -m.t.downMost - 1e-4f, "never through the floor's side of the lower limit", m.out.room[1], -m.t.downMost);
}

void Follow(float bodyYaw, float facing, float dx, float dz) {
    Sim m;
    m.bodyYaw = bodyYaw;
    m.facing = facing;
    m.FrameKept();
    const double start[3] = {m.eye[0], m.eye[1], m.eye[2]};
    float worstOff = 0.0f;
    bool walked = false;
    for (int i = 0; i < 72 * 8; ++i) {
        const float k = std::min(1.0f, i * kDt / 0.6f);  // a step, in 0.6 s
        m.head[0] = dx * k;
        m.head[2] = dz * k;
        m.FrameKept();
        walked = walked || m.out.following;
        float right[3], back[3];
        m.Rows(right, back);
        // One to one in the world, up to the follow's first frames: where the head is, whoever's walked.
        for (int a = 0; a < 3; a += 2) {
            const double want = start[a] + m.head[0] * right[a] + m.head[2] * back[a];
            worstOff = std::max(worstOff, static_cast<float>(std::fabs(m.Camera(a) - want)));
        }
    }
    Check(walked, "step: the soldier's walked under the head");
    Check(!m.out.following, "step: and stops");
    // A quick step outruns the walking: at most the limit's squeeze behind, for a moment.
    Check(worstOff < 0.16f, "step: the camera stays near where the head is in the world", worstOff, 0.16);
    float right[3], back[3];
    m.Rows(right, back);
    for (int a = 0; a < 3; a += 2) {
        const double want = start[a] + m.head[0] * right[a] + m.head[2] * back[a];
        Check(std::fabs(m.Camera(a) - want) < 0.03, "step: and ends where the head is in the world", m.Camera(a), want);
    }
    const double moved[2] = {(m.eye[0] - start[0]) * right[0] + (m.eye[2] - start[2]) * right[2],
                             (m.eye[0] - start[0]) * back[0] + (m.eye[2] - start[2]) * back[2]};
    const double length = std::sqrt(dx * dx + dz * dz);
    const double along = (moved[0] * dx + moved[1] * dz) / length;
    Check(along > length - 0.2 && along < length, "step: the soldier went that way, nearly all of it", along, length);
    const double across = std::fabs(moved[0] * dz - moved[1] * dx) / length;
    Check(across < 0.02, "step: straight there", across, 0.0);
}

void Blocked() {
    Sim m;
    m.blocked = true;
    m.Frame();
    m.MoveTo(0.4f, 0.0f, 0.0f);
    int commanded = 0;
    for (int i = 0; i < 72 * 30; ++i) {
        m.Frame();
        commanded += m.out.following ? 1 : 0;
    }
    Check(commanded > 0 && commanded < 72 * 5, "at a wall: asked now and then, not all the time", commanded, 72 * 5);
    Check(m.out.room[0] > 0.25f && m.out.room[0] <= m.t.leanMost, "at a wall: the view at the limit", m.out.room[0], m.t.leanMost);
    // Coming back, it's one to one again.
    m.MoveTo(0.1f, 0.0f, 0.0f);
    Check(std::fabs(m.out.room[0] - 0.1f) < 0.002f, "at a wall: back one to one", m.out.room[0], 0.1);
}

void PlayerStick() {
    Sim m;
    m.Frame();
    m.stick = true;
    m.MoveTo(0.4f, 0.0f, 0.0f);
    float before = m.s.walked[0];
    for (int i = 0; i < 72; ++i) {
        m.eye[2] -= 2.0 * kDt;  // the player's stick walks it
        m.Frame();
        Check(!m.out.following, "the player's stick wins: no walking under the head");
    }
    Check(m.s.walked[0] == before && m.s.walked[1] == 0.0f, "the player's stick: nothing taken off");
    m.stick = false;
    m.Frame();
    m.Frame();
    Check(m.out.following, "the stick let go: the soldier's walked under the head again");
}

void Crouch() {
    Sim m;
    m.Frame();
    auto run = [&](float y, float seconds) {
        float worst = 0.0f;
        for (int i = 0; i < static_cast<int>(seconds * 72); ++i) {
            m.head[1] += (y - m.head[1]) * 0.5f;
            m.FrameKept();
            // The view follows the real head alone: the game's camera plus the move.
            const float off = static_cast<float>(m.Camera(1) - (m.eyeStanding + m.head[1]));
            worst = std::max(worst, std::fabs(off));
        }
        return worst;
    };
    float worst = run(-0.25f, 2.0f);
    Check(!m.out.crouch, "25 cm down: no crouch");
    Check(worst < 0.012f, "25 cm down: one to one", worst, 0.012);
    run(0.0f, 0.5f);
    run(-0.33f, 0.1f);
    Check(!m.out.crouch, "a quick duck: no crouch");
    run(0.0f, 0.5f);
    worst = run(-0.35f, 2.0f);
    Check(m.out.crouch, "35 cm down, held: the game's crouch held");
    Check(m.gameDrop > 0.59f, "and the game crouched", m.gameDrop, 0.6);
    Check(worst < 0.04f, "the view doesn't drop twice", worst, 0.04);
    worst = run(-0.25f, 1.0f);
    Check(m.out.crouch, "25 cm: still crouched");
    Check(worst < 0.012f, "still one to one", worst, 0.012);
    worst = run(-0.10f, 3.0f);
    Check(!m.out.crouch, "10 cm: stood again");
    Check(m.gameDrop < 0.01f, "and the game stood", m.gameDrop, 0.0);
    Check(worst < 0.04f, "the view goes up by the head alone", worst, 0.04);
    Check(std::fabs(m.out.comp) < 0.005f, "nothing left over", m.out.comp, 0.0);
    Check(std::fabs(m.Camera(1) - (m.eyeStanding + m.head[1])) < 0.01f, "back where the head is",
          m.Camera(1), m.eyeStanding + m.head[1]);
}

void ButtonCrouch() {
    Sim m;
    m.Frame();
    m.buttonCrouch = true;
    for (int i = 0; i < 72; ++i)
        m.FrameKept();
    Check(!m.out.crouch, "the button's crouch isn't the body's");
    Check(std::fabs(m.out.room[1]) < 1e-4f, "the button's crouch: the game's drop stays", m.out.room[1], 0.0);
    Check(m.Camera(1) < m.eyeStanding - 0.59, "the view drops with it", m.Camera(1), m.eyeStanding - 0.6);
}

void TurnAboutHead() {
    Sim m;
    m.t.walk = false;
    m.Frame();
    m.head[0] = 0.2f;
    m.FrameKept();
    const double at[2] = {m.Camera(0), m.Camera(2)};
    float worst = 0.0f;
    for (int i = 0; i < 144; ++i) {
        m.bodyYaw += 266.0f * kPi / 180.0f * kDt;  // the stick at full turn
        m.FrameKept();
        worst = std::max(worst, static_cast<float>(std::fabs(m.Camera(0) - at[0]) + std::fabs(m.Camera(2) - at[1])));
    }
    Check(worst < 0.012f, "the stick's turn: about the head, the camera stays", worst, 0.012);
}

void Scripted() {
    Sim m;
    m.Frame();
    m.head[0] = 0.12f;
    m.head[1] = -0.05f;
    m.Frame();
    float last[3] = {m.out.room[0], m.out.room[1], m.out.room[2]};
    m.scripted = true;
    float worstStep = 0.0f, worstEnd = 0.0f;
    for (int i = 0; i < 72; ++i) {
        m.Frame();
        for (int k = 0; k < 3; ++k) {
            worstStep = std::max(worstStep, std::fabs(m.out.room[k] - last[k]));
            last[k] = m.out.room[k];
        }
    }
    Check(worstStep < 0.02f, "into a cutscene: no jump, eased", worstStep, 0.02);
    Check(std::fabs(m.out.room[0]) < 0.002f, "into a cutscene: the head's place from there", m.out.room[0], 0.0);
    m.head[0] += 0.5f;
    m.head[1] -= 0.5f;
    for (int i = 0; i < 72 * 4; ++i) {
        m.Frame();
        Check(!m.out.following && !m.out.crouch, "in a cutscene: nothing walked or crouched");
        const float r = std::sqrt(m.out.room[0] * m.out.room[0] + m.out.room[1] * m.out.room[1] + m.out.room[2] * m.out.room[2]);
        worstEnd = std::max(worstEnd, r);
    }
    Check(worstEnd <= m.t.scriptedMost + 1e-3f, "in a cutscene: a small limit", worstEnd, m.t.scriptedMost);
    m.head[0] -= 0.5f;
    m.head[1] += 0.5f;
    for (int i = 0; i < 72; ++i)
        m.Frame();
    last[0] = m.out.room[0], last[1] = m.out.room[1], last[2] = m.out.room[2];
    m.scripted = false;
    worstStep = 0.0f;
    for (int i = 0; i < 72 * 3; ++i) {
        m.Frame();
        for (int k = 0; k < 3; ++k) {
            worstStep = std::max(worstStep, std::fabs(m.out.room[k] - last[k]));
            last[k] = m.out.room[k];
        }
    }
    Check(worstStep < 0.03f, "out of a cutscene: no jump, eased", worstStep, 0.03);
    Check(std::fabs(m.out.room[0]) < 0.002f && !m.out.following, "out of a cutscene: over the soldier again", m.out.room[0], 0.0);
}

// Into a cutscene with the head off to one side, the body facing along the world's x: the move's eased in the world too, not
// only in the room's numbers (it hopped 18.6 cm in a frame in the world's x).
void ScriptedEasedInWorld() {
    for (float yaw : {0.0f, 1.2f, 1.57f, -2.4f}) {
        Sim m;
        m.bodyYaw = yaw;
        m.Frame();
        m.MoveTo(0.0f, 0.0f, -0.20f);
        float was[3] = {m.out.world[0], m.out.world[1], m.out.world[2]};
        float most = 0.0f;
        const auto Watch = [&](int frames) {
            for (int i = 0; i < frames; ++i) {
                m.Frame();
                for (int k = 0; k < 3; ++k) {
                    most = std::max(most, std::fabs(m.out.world[k] - was[k]));
                    was[k] = m.out.world[k];
                }
            }
        };
        m.scripted = true;
        Watch(72);
        Check(most < 0.03f, "into a cutscene: the world's move eased", most, 0.03);
        most = 0.0f;
        m.scripted = false;
        Watch(144);
        Check(most < 0.03f, "out of a cutscene: the world's move eased", most, 0.03);
    }
}

// Squatting while the stick walks the soldier over a slope or stairs: the ground's height isn't the game's crouch.
void CrouchOnSlope() {
    for (float rate : {-1.0f, -0.6f, -0.2f, 0.2f, 0.6f}) {
        Sim m;
        m.Frame();
        m.stick = true;
        const auto Ground = [&](int frames) {
            for (int i = 0; i < frames; ++i) {
                m.eyeStanding += rate * kDt;
                m.Frame();
            }
        };
        Ground(72);
        const int squat = 36;
        for (int i = 1; i <= squat; ++i) {
            m.head[1] = -0.35f * i / squat;
            m.eyeStanding += rate * kDt;
            m.Frame();
        }
        Ground(72 * 2);
        Check(m.out.crouch, "crouched on a slope");
        Check(std::fabs(m.out.comp - 0.6f) < 0.02f, "on a slope, the game's own drop added back and no more", m.out.comp, 0.6);
        // Level ground again: it stays right.
        for (int i = 0; i < 72 * 3; ++i)
            m.Frame();
        Check(std::fabs(m.out.comp - 0.6f) < 0.02f, "and after it", m.out.comp, 0.6);
    }
}

// Dragged past the lean's far limit (a wall, a real walk), then ducking before it has settled: the height isn't dragged down
// to the duck, so the view doesn't rise by itself and stays with the head after standing up.
void DuckWhileSettling() {
    for (float duck : {-0.20f, -0.25f, -0.33f}) {
        Sim m;
        m.t.walk = false;
        m.Frame();
        m.MoveTo(0.70f, 0.0f, 0.0f, 1.0f);
        Check(m.s.settle, "dragged: to settle");
        m.MoveTo(0.70f, duck, 0.0f, 0.5f);
        for (int i = 0; i < 72 * 6; ++i) {
            m.Frame();
            Check(std::fabs(m.out.room[1] - m.s.comp - duck) < 0.02f, "ducked: the view stays with the head", m.out.room[1] - m.s.comp, duck);
        }
        m.MoveTo(0.70f, 0.0f, 0.0f, 0.5f);
        for (int i = 0; i < 72 * 3; ++i)
            m.Frame();
        Check(std::fabs(m.out.room[1]) < 0.02f, "stood up: the view at the head's height", m.out.room[1], 0.0);
    }
}

// Put down on a desk, far below standing and off to the side: for a moment that's a crouch and a step, then nobody's there: the
// game's crouch is let go, no walking. Lifted and worn, it all works again.
void PutDown() {
    Sim m;
    m.t.putDownAfter = 5.0f;
    m.Frame();
    m.MoveTo(0.2f, -0.8f, 0.0f, 1.0f);
    for (int i = 0; i < 36; ++i)
        m.Frame();
    Check(m.out.crouch, "put down: at first it's a crouch");
    for (int i = 0; i < 72 * 6; ++i)
        m.Frame();
    Check(!m.out.crouch, "lying still: the game's crouch let go");
    Check(!m.out.following, "lying still: nothing walked");
    m.MoveTo(0.2f, 0.0f, 0.0f, 1.0f);
    for (int i = 0; i < 72; ++i) {
        m.head[0] = 0.2f + 0.01f * std::sin(i * 0.2f);
        m.Frame();
    }
    Check(!m.out.crouch, "lifted back up: standing");
    // Tracking lost while crouched: let go too, after the same time.
    Sim c;
    c.t.putDownAfter = 5.0f;
    c.Frame();
    c.MoveTo(0.0f, -0.45f, 0.0f, 0.5f);
    for (int i = 0; i < 72; ++i) {
        c.head[0] = 0.01f * std::sin(i * 0.2f);
        c.Frame();
    }
    Check(c.out.crouch, "a squat crouches");
    c.tracked = false;
    for (int i = 0; i < 72 * 6; ++i)
        c.Frame();
    Check(!c.out.crouch, "tracking lost for long: the game's crouch let go");
}

// Lost low and found standing (the headset off to a desk and back on; a squat, tracking lost, stood up unseen): all starts afresh,
// the view at the head. Any way the height went: a head lost above neutral too.
void BackOn() {
    for (float low : {-0.8f, -0.5f, -0.4f, -0.28f}) {
        Sim m;
        m.Frame();
        m.MoveTo(0.1f, low, 0.1f, 1.0f);
        for (int i = 0; i < 36; ++i)
            m.Frame();
        m.tracked = false;
        for (int i = 0; i < 72 * 3; ++i)
            m.Frame();
        m.tracked = true;
        m.head[0] = 0.0f, m.head[1] = 0.0f, m.head[2] = 0.0f;
        for (int i = 0; i < 72 * 2; ++i)
            m.Frame();
        Check(!m.out.crouch, "back on: standing");
        Check(std::fabs(m.out.room[1]) < 0.02f, "back on: the view at the head's height", m.out.room[1], 0.0);
        Check(std::fabs(m.out.comp) < 0.01f, "back on: nothing left of the crouch", m.out.comp, 0.0);
        m.MoveTo(0.0f, -0.40f, 0.0f, 0.5f);
        for (int i = 0; i < 72; ++i)
            m.Frame();
        Check(m.out.crouch, "back on: a squat crouches");
    }
    {
        Sim m;
        m.Frame();
        m.MoveTo(0.0f, 0.15f, 0.3f, 0.5f);
        m.tracked = false;
        for (int i = 0; i < 72 * 3; ++i)
            m.Frame();
        m.tracked = true;
        m.head[0] = 0.0f, m.head[1] = 0.0f, m.head[2] = 0.0f;
        for (int i = 0; i < 72 * 2; ++i)
            m.Frame();
        Check(std::fabs(m.out.room[1]) < 0.02f, "lost high, back on: the view at the head's height", m.out.room[1], 0.0);
    }
    {
        // Put back on lower than it was: that's the new height.
        Sim m;
        m.Frame();
        m.tracked = false;
        for (int i = 0; i < 72; ++i)
            m.Frame();
        m.tracked = true;
        m.head[1] = -0.55f;
        for (int i = 0; i < 72 * 2; ++i)
            m.Frame();
        Check(std::fabs(m.out.room[1]) < 0.02f, "back on lower: the new height", m.out.room[1], 0.0);
        Check(!m.out.crouch, "back on lower: not a crouch");
    }
    for (float low : {0.15f, 0.30f}) {
        // The first frame's head, bent over: the real standing height's found, and a small duck moves the view one to one.
        Sim m;
        m.head[1] = -low;
        m.Frame();
        m.MoveTo(0.0f, 0.0f, 0.0f, 0.5f);
        for (int i = 0; i < 72 * 6; ++i)
            m.Frame();
        Check(std::fabs(m.out.room[1]) < 0.02f, "started low: the real standing height found", m.out.room[1], 0.0);
        const float was = m.out.room[1];
        m.MoveTo(0.0f, -0.05f, 0.0f, 0.3f);
        for (int i = 0; i < 18; ++i)
            m.Frame();
        Check(std::fabs((m.out.room[1] - was) + 0.05f) < 0.01f, "started low: a small duck one to one", m.out.room[1] - was, -0.05);
    }
}

// A pose only passing through, above the worn height (a headset lifted off and put back, a hop), isn't the standing height: the
// soldier didn't stay crouched for good with the view looking right.
void LiftedOff() {
    for (float lift : {0.25f, 0.31f, 0.40f}) {
        Sim m;
        m.Frame();
        m.MoveTo(0.0f, lift, 0.0f, 0.5f);
        m.MoveTo(0.0f, -0.90f, 0.0f, 0.8f);
        for (int i = 0; i < 72 * 3; ++i)
            m.Frame();
        m.MoveTo(0.0f, 0.0f, 0.0f, 1.5f);
        for (int i = 0; i < 72 * 2; ++i)
            m.Frame();
        Check(!m.out.crouch, "lifted off, set down, worn again: not crouched", m.out.drop, 0.0);
        Check(m.gameDrop < 0.01f, "lifted off: the game stands", m.gameDrop, 0.0);
    }
    Sim m;
    m.Frame();
    m.MoveTo(0.0f, 0.31f, 0.0f, 0.25f);
    m.MoveTo(0.0f, 0.0f, 0.0f, 0.25f);
    for (int i = 0; i < 72 * 2; ++i)
        m.Frame();
    Check(!m.out.crouch, "a hop: not crouched", m.out.drop, 0.0);
    m.MoveTo(0.0f, -0.40f, 0.0f, 0.5f);
    for (int i = 0; i < 72; ++i)
        m.Frame();
    Check(m.out.crouch, "after a hop, a squat crouches");
    m.MoveTo(0.0f, 0.0f, 0.0f, 0.5f);
    for (int i = 0; i < 72; ++i)
        m.Frame();
    Check(!m.out.crouch, "and standing up stands");
    Sim d;
    d.head[1] = -0.9f;
    d.Frame();
    d.MoveTo(0.0f, 0.0f, 0.0f, 1.0f);
    for (int i = 0; i < 72 * 10; ++i)
        d.Frame();
    d.MoveTo(0.0f, -0.40f, 0.0f, 0.5f);
    for (int i = 0; i < 72; ++i)
        d.Frame();
    Check(d.out.crouch, "picked up from a desk: 10 s later a squat crouches");
    d.MoveTo(0.0f, 0.0f, 0.0f, 0.5f);
    for (int i = 0; i < 72; ++i)
        d.Frame();
    Check(!d.out.crouch, "and standing up stands");
}

void Lost() {
    Sim m;
    m.Frame();
    m.head[0] = 0.15f;
    m.Frame();
    m.tracked = false;
    m.head[0] = 9.0f;  // whatever it says now isn't the head
    m.Frame();
    Check(std::fabs(m.out.room[0] - 0.15f) < 1e-3f, "lost: held", m.out.room[0], 0.15);
    for (int i = 0; i < 36; ++i)
        m.Frame();
    Check(std::fabs(m.out.room[0] - 0.15f) < 1e-3f, "lost half a second: still held", m.out.room[0], 0.15);
    float last = m.out.room[0], worst = 0.0f;
    for (int i = 0; i < 72 * 6; ++i) {
        m.Frame();
        worst = std::max(worst, std::fabs(m.out.room[0] - last));
        last = m.out.room[0];
        Check(!m.out.following && !m.out.crouch, "lost: nothing walked or crouched");
    }
    Check(std::fabs(last) < 0.005f, "lost for good: back to the middle", last, 0.0);
    Check(worst < 0.005f, "lost for good: gently", worst, 0.005);
    // Found again far away: taken up, no jump.
    m.tracked = true;
    m.head[0] = 1.5f;
    m.head[1] = 0.4f;
    m.Frame();
    Check(std::fabs(m.out.room[0] - last) < 0.002f && std::fabs(m.out.room[1]) < 0.002f, "found again: no jump", m.out.room[0], last);
    m.head[0] += 0.05f;
    m.Frame();
    Check(std::fabs(m.out.room[0] - last - 0.05f) < 0.003f, "found again: one to one from there", m.out.room[0], last + 0.05f);
}

void Recentered() {
    Sim m;
    m.Frame();
    m.MoveTo(0.0f, -0.5f, 0.0f);
    for (int i = 0; i < 72; ++i)
        m.Frame();
    Check(m.out.crouch, "squatting");
    m.head[0] = m.head[1] = m.head[2] = 0.0f;
    m.recentered = true;
    m.Frame();
    Check(!m.out.crouch, "a recenter: this height is standing");
    Check(std::fabs(m.out.room[0]) < 1e-4f, "a recenter: over the soldier", m.out.room[0], 0.0);
    for (int i = 0; i < 72 * 2; ++i)
        m.Frame();
    Check(std::fabs(m.out.room[1]) < 0.005f, "a recenter: at the game's height again", m.out.room[1], 0.0);
}

void PutSomewhere() {
    Sim m;
    m.Frame();
    m.MoveTo(0.4f, 0.0f, 0.0f);
    m.FrameKept();
    Check(m.out.following, "walking");
    const float walked = m.s.walked[0];
    m.eye[0] += 30.0;  // a checkpoint, a respawn
    m.Frame();
    Check(std::fabs(m.s.walked[0] - walked) < 0.1f, "put somewhere else: not taken for a walk", m.s.walked[0], walked);
}

void Time() {
    Sim m;
    const auto a = std::chrono::steady_clock::now();
    for (int i = 0; i < 100000; ++i) {
        m.head[0] = 0.1f * std::sin(i * 0.01f);
        m.Frame();
    }
    const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - a).count() / 100000.0;
    Check(us < 20.0, "time: under 20 microseconds a step", us, 20.0);
    std::printf("a step takes %.2f microseconds\n", us);
}

}

int main() {
    StillHead();
    Lean();
    NeckDecides();
    Limit();
    Height();
    Follow(0.0f, 0.0f, 0.5f, 0.0f);
    Follow(0.6f, 0.0f, 0.0f, -0.6f);
    Follow(-1.1f, 0.9f, 0.4f, 0.35f);
    Follow(2.8f, -2.0f, -0.7f, 0.1f);
    Blocked();
    PlayerStick();
    Crouch();
    ButtonCrouch();
    TurnAboutHead();
    Scripted();
    ScriptedEasedInWorld();
    CrouchOnSlope();
    DuckWhileSettling();
    PutDown();
    BackOn();
    LiftedOff();
    Lost();
    Recentered();
    PutSomewhere();
    Time();
    std::printf("%d of %d checks failed\n", g_failed, g_checks);
    return g_failed;
}

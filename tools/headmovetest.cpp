// The head's place moving the game's cameras (src/headmove.h, as views.cpp runs it), on scripted heads and a simulated
// soldier at 72 frames a second: a still head stays still, leaning one to one and never past the limit, a turned or bowed
// head walks and crouches nothing, the soldier walked under a step and only as far as it really goes, stopped by a wall,
// the player's stick winning, crouching for real without the view dropping twice, the button's crouch left to the game,
// the smooth turn about the head, cutscenes, tracking lost and found, a recenter, the soldier put somewhere else.
// And the game's eye taken off its arm (ArmTell) on a made-up game with the game's 30 ticks a second: the look shaking with the soldier standing, walking and sprinting, the game moving the
// look itself, the mod's own rows in a build or in every one, a step of the look, a wrong start. With "--replay <log> [from] [to] [limit]" a log's "View:" lines are put through it instead.
#define HEADMOVE_CORE_ONLY
#include "../src/headmove.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

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

// ---- The game's eye and its arm (ArmTell) ----
// A made-up game with what two sessions' logs showed: builds at 72 a second, the game's ticks at 30, the eye at the soldier's middle plus 0.0773 m along the look yaw its last tick read (93% the
// one written the build before, 6% this build's, 1% two before), its place in floats (0.12 mm at 1300 m), the rows' yaw that tick's with a sway of the game's own, some builds with the rows the
// mod left the build before (a yaw 30 degrees away), the game moving the look itself for a tick now and then. The look shakes in bursts of 3 s with 3 s of rest between.
struct Random {
    uint64_t state;
    double Next() {  // 0 up to 1
        state = state * 6364136223846793005ull + 1442695040888963407ull;
        return static_cast<double>(state >> 11) / 9007199254740992.0;
    }
};

constexpr double kTurn = 6.283185307179586, kDeg = kTurn / 360.0;

struct ArmGame {
    double hz = 4.0, amp = 35.0;  // the look's shake: times a second, degrees each way (880 degrees a second at its fastest)
    double sway = 1.5;            // the game's rows off the tick's yaw by up to this (degrees)
    double stale = 0.002;         // the share of builds that come with the mod's own rows
    bool told = true;             // and whether they are told apart (ArmIn::stale), as views.cpp tells them
    double walk = 0.0;            // metres a second
    double jumps = 0.0;           // the share of ticks in which the game moves the look itself, 60 degrees
    double flicker = 0.0;         // the eye's place off by up to this in every build (metres)
    double step = 0.0;            // in place of the shake: the look stepping this many degrees once, 2 s in (the self-test's gun turn)
    double turn = 0.0;            // or turning steadily this many degrees a second from 2 s in (the stick's smooth turn)
    int wrongAt = 0;              // the rule's yaw put 123 degrees off before this build
    int seed = 1;
    int builds = 72 * 120;
    bool shakeFirst = false;      // the look shaking from the first build on (else it rests 3 s first)
    bool trace = false;           // print each build the rule puts more than a millimetre off
};

constexpr int kArmWays = 3;  // as it was guessed, the rows at the eye's steps, the rule
constexpr int kArmSpeeds = 4;
constexpr double kArmSpeedLow[kArmSpeeds] = {50.0, 200.0, 400.0, 750.0}, kArmSpeedHigh[kArmSpeeds] = {150.0, 400.0, 600.0, 1100.0};

struct ArmResult {
    std::vector<float> off[kArmWays];  // mm from the soldier's middle, build by build from the second second on (off[w][n - 73] is build n's)
    std::vector<float> bySpeed[kArmWays][kArmSpeeds];
    int how[kArmHows] = {};
};

struct Spread {
    double mean = 0.0, p90 = 0.0, p99 = 0.0, p999 = 0.0, worst = 0.0;
    size_t count = 0;
};

Spread SpreadOf(std::vector<float> v) {
    Spread s;
    s.count = v.size();
    if (v.empty())
        return s;
    std::sort(v.begin(), v.end());
    for (float e : v)
        s.mean += e;
    s.mean /= static_cast<double>(v.size());
    const auto at = [&](double share) { return v[std::min(v.size() - 1, static_cast<size_t>(static_cast<double>(v.size()) * share))]; };
    s.p90 = at(0.9), s.p99 = at(0.99), s.p999 = at(0.999), s.worst = v.back();
    return s;
}

ArmResult RunArm(const ArmGame& g) {
    ArmResult out;
    Random random{0x9E3779B97F4A7C15ull * static_cast<uint64_t>(g.seed + 1)};
    const double dt = 1.0 / 72.0, tick = 1.0 / 30.0, arm = 0.0773;
    // The look yaw written at each build's Present (degrees).
    std::vector<double> written(static_cast<size_t>(g.builds) + 2);
    for (int n = 0; n <= g.builds + 1; ++n) {
        const double t = n * dt;
        if (g.step != 0.0) {
            written[n] = 20.0 + (t >= 2.0 ? g.step : 0.0);
        } else if (g.turn != 0.0) {
            written[n] = 20.0 + g.turn * std::max(0.0, t - 2.0);
        } else {
            const int third = static_cast<int>(t / 3.0);
            written[n] = 20.0 + g.amp * std::sin(kTurn * g.hz * (third % 2 == (g.shakeFirst ? 0 : 1) ? t : third * 3.0));
        }
    }
    const auto stored = [&](int n) {  // as the game keeps it: radians, 0 to 2 pi
        const double yaw = written[n] * kDeg;
        return static_cast<float>(yaw - kTurn * std::floor(yaw / kTurn));
    };
    double nextTick = random.Next() * tick;
    double px = 1299.76, pz = -1140.60, tickYaw = written[0];
    ArmGuess guess;
    Arm rows, rule;
    char traceBefore[400] = "";
    int traceLeft = 0;
    for (int n = 1; n <= g.builds; ++n) {
        const double t = n * dt;
        if (g.walk > 0.0) {  // with an uneven step, round a wide circle (the eye's place stays where a float's step is 0.12 mm)
            const double speed = g.walk * (1.0 + 0.15 * (random.Next() - 0.5)), heading = kTurn * t / 120.0;
            px += speed * dt * std::cos(heading);
            pz += speed * dt * std::sin(heading);
        }
        if (t >= nextTick) {
            nextTick += tick;
            const double r = random.Next();
            int lag = r < 0.93 ? 1 : r < 0.99 ? 0 : 2;
            if (n - lag < 1)
                lag = 0;
            tickYaw = written[n - lag];
            if (random.Next() < g.jumps)
                tickYaw += 60.0;
        }
        const double flickX = g.flicker * (2.0 * random.Next() - 1.0), flickZ = g.flicker * (2.0 * random.Next() - 1.0);
        const float eye[3] = {static_cast<float>(px + flickX - arm * std::sin(tickYaw * kDeg)), 68.0f, static_cast<float>(pz + flickZ + arm * std::cos(tickYaw * kDeg))};
        const bool stale = random.Next() < g.stale;
        const double rowsYaw = (stale ? written[n > 1 ? n - 1 : n] + 30.0 : tickYaw + g.sway * (2.0 * random.Next() - 1.0)) * kDeg;
        const float back[3] = {static_cast<float>(std::sin(rowsYaw)), 0.0f, static_cast<float>(-std::cos(rowsYaw))};
        ArmIn in;
        in.eye[0] = eye[0];
        in.eye[1] = eye[2];
        in.grain = ArmGrain(eye[0], eye[2]);
        in.rows = true;
        in.rowsYaw = std::atan2(back[0], -back[2]);
        in.stale = stale && g.told;
        in.written = std::min(n, kArmYaws);
        for (int k = 0; k < in.written; ++k)
            in.yaws[k] = stored(n - k);
        in.first = n == 1;
        float forward[kArmWays][2];
        ArmGuessStep(guess, eye, back, true, in.yaws[0], static_cast<float>(arm), 1.0f, static_cast<float>(dt));
        forward[0][0] = guess.forward[0], forward[0][1] = guess.forward[1];
        ArmTell(rows, in, forward[1], 1);
        if (n == g.wrongAt)
            rule.yaw += static_cast<float>(123.0 * kDeg);
        ArmTell(rule, in, forward[2], 2);
        ++out.how[rule.how];
        if (g.trace) {  // each build the rule puts more than a millimetre off, the one before and the two after
            const double ex = eye[0] - arm * forward[2][0] - px, ez = eye[2] - arm * forward[2][1] - pz;
            const double off = std::sqrt(ex * ex + ez * ez) * 1000.0;
            char line[400];
            std::snprintf(line, sizeof(line), "  build %d (%.3f s): %.2f mm off; the eye stepped %.3f mm, its tick's yaw %.2f, the rows %.2f%s, written %.2f %.2f %.2f %.2f, told %.2f by %d\n", n, t,
                          off, rule.eyeStep * 1000.0, tickYaw, rowsYaw / kDeg, stale ? " (the mod's own)" : "", written[n], written[std::max(n - 1, 0)], written[std::max(n - 2, 0)],
                          written[std::max(n - 3, 0)], rule.yaw / kDeg, static_cast<int>(rule.how));
            if (off > 1.0) {
                if (traceLeft == 0)
                    std::printf("%s", traceBefore);
                std::printf("%s", line);
                traceLeft = 2;
            } else if (traceLeft > 0) {
                std::printf("%s", line);
                --traceLeft;
            }
            std::memcpy(traceBefore, line, sizeof(line));
        }
        if (n <= 72)
            continue;
        const double speed = std::fabs(written[n + 1] - written[n - 1]) / (2.0 * dt);
        for (int w = 0; w < kArmWays; ++w) {
            const double ex = eye[0] - arm * forward[w][0] - px, ez = eye[2] - arm * forward[w][1] - pz;
            const float off = static_cast<float>(std::sqrt(ex * ex + ez * ez) * 1000.0);
            out.off[w].push_back(off);
            for (int b = 0; b < kArmSpeeds; ++b)
                if (speed >= kArmSpeedLow[b] && speed < kArmSpeedHigh[b])
                    out.bySpeed[w][b].push_back(off);
        }
    }
    return out;
}

const char* const kArmWayNames[kArmWays] = {"armby 0, as it was guessed", "armby 1, the rows at the eye's steps", "armby 2, the rule"};

void PrintArm(const char* title, const ArmResult& r) {
    std::printf("%s\n", title);
    for (int w = 0; w < kArmWays; ++w) {
        const Spread all = SpreadOf(r.off[w]);
        std::printf("  %-37s mean %6.2f  99%% %6.2f  99.9%% %6.2f  worst %6.2f mm |", kArmWayNames[w], all.mean, all.p99, all.p999, all.worst);
        for (int b = 0; b < kArmSpeeds; ++b) {
            const Spread s = SpreadOf(r.bySpeed[w][b]);
            if (s.count == 0)  // (a shake in step with the builds never turns at some speeds)
                std::printf(" %4.0f-%4.0f deg/s: no builds%s", kArmSpeedLow[b], kArmSpeedHigh[b], b + 1 < kArmSpeeds ? ";" : " (mean, 99%, worst)");
            else
                std::printf(" %4.0f-%4.0f deg/s: %5.2f %5.2f %5.2f%s", kArmSpeedLow[b], kArmSpeedHigh[b], s.mean, s.p99, s.worst, b + 1 < kArmSpeeds ? ";" : " (mean, 99%, worst)");
        }
        std::printf("\n");
    }
}

void ArmStanding() {
    // The soldier standing, the look shaking, none of the game's own jumps: the cameras never 0.3 mm from his middle, at any rate of shaking.
    for (const double hz : {4.0, 2.0, 1.0, 0.3})
        for (int seed = 1; seed <= 3; ++seed) {
            ArmGame g;
            g.hz = hz;
            g.seed = seed;
            const Spread s = SpreadOf(RunArm(g).off[2]);
            Check(s.worst < 0.3, "arm: standing, the look shaking: never 0.3 mm off the soldier's middle", s.worst, 0.3);
            Check(s.mean < 0.1, "arm: standing: under 0.1 mm on average", s.mean, 0.1);
        }
    // The game moving the look itself for a tick now and then (60 degrees, the eye 8 cm out and back): the rows' sway shows for that tick, no more.
    for (int seed = 1; seed <= 6; ++seed) {
        ArmGame g;
        g.jumps = 0.003;
        g.seed = seed;
        const ArmResult r = RunArm(g);
        const Spread s = SpreadOf(r.off[2]);
        Check(s.worst < 2.5, "arm: the game's own look jumps: never 2.5 mm off", s.worst, 2.5);
        Check(s.p99 < 0.3, "arm: the game's own look jumps: 99% under 0.3 mm", s.p99, 0.3);
        // As it was guessed, the same game reads about 1.9 mm for every 100 degrees a second, as the logs do: the meter measures what they measured.
        const Spread was = SpreadOf(r.bySpeed[0][2]);
        Check(was.mean > 6.0 && was.mean < 13.0, "arm: as it was guessed, 400 to 600 degrees a second: 6 to 13 mm off on average", was.mean, 9.4);
    }
    // The three ways side by side, by how fast the look turns (a shake that is not in step with the builds, so every speed comes up).
    {
        ArmGame g;
        g.hz = 3.7;
        g.amp = 40.0;
        g.jumps = 0.003;
        const ArmResult r = RunArm(g);
        PrintArm("The look shaking 3.7 times a second, 40 degrees each way (930 degrees a second at its fastest), the soldier standing; mm off his middle:", r);
        for (int b = 0; b < kArmSpeeds; ++b) {
            const Spread s = SpreadOf(r.bySpeed[2][b]), was = SpreadOf(r.bySpeed[0][b]);
            Check(s.count > 100 && s.p99 < 0.3, "arm: at every speed of the look: 99% under 0.3 mm", s.p99, 0.3);
            Check(was.p99 > 5.0 * s.p99, "arm: (as it was guessed, the same speed is five times as far off)", was.p99, 5.0 * s.p99);
        }
    }
    // Every build with the mod's own rows (a launch without the headset): the eye's own step tells.
    for (int seed = 1; seed <= 3; ++seed) {
        ArmGame g;
        g.stale = 1.0;
        g.jumps = 0.003;
        g.seed = seed;
        const ArmResult r = RunArm(g);
        const Spread s = SpreadOf(r.off[2]);
        Check(s.worst < 0.3, "arm: the mod's own rows in every build: never 0.3 mm off", s.worst, 0.3);
        if (seed == 1)
            PrintArm("The look shaking 4 times a second, 35 degrees each way, the mod's own rows in every build:", r);
    }
}

void ArmWalking() {
    // Walking and sprinting, the look shaking, the game's own jumps among it: the rows' sway shows in a jump's builds (the soldier's own step hides the eye's), nothing else.
    for (const double walk : {1.5, 5.8})
        for (int seed = 1; seed <= 3; ++seed) {
            ArmGame g;
            g.walk = walk;
            g.jumps = 0.003;
            g.seed = seed;
            g.builds = 72 * 2400;
            const ArmResult r = RunArm(g);
            const Spread s = SpreadOf(r.off[2]);
            Check(s.p999 < 1.5, "arm: walking, the look shaking: 99.9% under 1.5 mm", s.p999, 1.5);
            Check(s.p99 < 0.3, "arm: walking: 99% under 0.3 mm", s.p99, 0.3);
            if (seed == 1)
                PrintArm(walk < 2.0 ? "Walking 1.5 m a second:" : "Sprinting 5.8 m a second:", r);
        }
}

void ArmStep() {
    // The self-test's gun turn: the look stepping 20 degrees at once, with the game's own rows and with the mod's in every build.
    for (const double stale : {0.0, 1.0}) {
        ArmGame g;
        g.step = 20.0;
        g.stale = stale;
        g.builds = 72 * 6;
        const ArmResult r = RunArm(g);
        const Spread s = SpreadOf(r.off[2]);
        Check(s.worst < 0.3, "arm: a 20 degree step of the look: never 0.3 mm off", s.worst, 0.3);
        // (As it was guessed, one step alone is right too: its guard waits for the eye. It is a look that goes on turning that it cannot follow.)
        const Spread was = SpreadOf(r.off[0]);
        Check(was.worst < 0.3, "arm: a 20 degree step, as it was guessed: never 0.3 mm off either", was.worst, 0.3);
    }
    // The look turning steadily (the stick's smooth turn at 120 degrees a second, a slow sweep, a fast one): the yaws written lie evenly apart, and the eye's step still tells which.
    for (const double turn : {120.0, 20.0, 5.0, 400.0})
        for (const double stale : {0.002, 1.0}) {
            ArmGame g;
            g.turn = turn;
            g.stale = stale;
            g.jumps = 0.0;
            g.builds = 72 * 20;
            const ArmResult r = RunArm(g);
            const Spread s = SpreadOf(r.off[2]);
            Check(s.worst < 0.3, "arm: the look turning steadily: never 0.3 mm off", s.worst, 0.3);
        }
}

void ArmWrongStart() {
    // The yaw kept 123 degrees wrong (a launch once started so, the arm a fixed 14 cm off): right again within 0.3 s, at rest and in a shake, with the game's rows; at rest with the mod's.
    struct Case {
        double stale;
        int at;
        const char* name;
    };
    for (const Case& c : {Case{0.0, 72 * 1 + 36, "arm: a wrong start at rest: right within 0.3 s"}, Case{0.0, 72 * 4, "arm: a wrong start in a shake: right within 0.3 s"},
                          Case{1.0, 72 * 1 + 36, "arm: a wrong start at rest, the mod's own rows: right within 0.3 s"}}) {
        ArmGame g;
        g.stale = c.stale;
        g.wrongAt = c.at;
        g.builds = 72 * 12;
        const ArmResult r = RunArm(g);
        float worst = 0.0f;
        for (int n = c.at + 22; n <= g.builds; ++n)
            worst = std::max(worst, r.off[2][n - 73]);
        Check(worst < 0.3f, c.name, worst, 0.3);
    }
    // A start in the middle of a shake. With the game's rows: right from the first tick. With the mod's own in every build there is nothing to tell the yaw by but the eye's steps, which
    // only say how far it swung: the cameras stand off by what the first guess was off (a build's turn at most: 16 mm here), nearer with every tick and never jumping, and right once the
    // look rests.
    for (const double stale : {0.0, 1.0})
        for (int seed = 1; seed <= 3; ++seed) {
            ArmGame g;
            g.stale = stale;
            g.shakeFirst = true;
            g.seed = seed;
            g.builds = 72 * 12;
            const ArmResult r = RunArm(g);
            float early = 0.0f, late = 0.0f, jump = 0.0f;
            for (int n = 73; n <= g.builds; ++n) {
                const float off = r.off[2][n - 73];
                float& most = n <= 72 * 3 + 15 ? early : late;
                most = std::max(most, off);
                if (n > 73)
                    jump = std::max(jump, std::fabs(off - r.off[2][n - 74]));
            }
            if (stale == 0.0) {
                Check(early < 0.3f, "arm: a start in a shake: right within a second", early, 0.3);
            } else {
                Check(early < 5.0f, "arm: a start in a shake, the mod's own rows: under 5 mm off after a second", early, 5.0);
                Check(jump < 1.0f, "arm: a start in a shake, the mod's own rows: the cameras never jump a millimetre", jump, 1.0);
            }
            Check(late < 0.3f, "arm: a start in a shake: right once the look has rested", late, 0.3);
        }
}

void ArmRows() {
    // The mod's own rows told from the game's: all nine numbers alike, or the yaw alone.
    float left[9] = {0.8f, 0.0f, -0.6f, 0.0f, 1.0f, 0.0f, 0.6f, 0.0f, 0.8f}, built[9];
    for (int i = 0; i < 9; ++i)
        built[i] = left[i] + (i % 2 ? 1e-4f : -1e-4f);
    Check(ArmRowsStale(built, left), "arm rows: nine numbers within 2e-4 are the mod's own");
    // Turned a degree: the game's.
    const float c = std::cos(0.01745f), s = std::sin(0.01745f);
    for (int r = 0; r < 3; ++r) {
        built[3 * r] = left[3 * r] * c - left[3 * r + 2] * s;
        built[3 * r + 1] = left[3 * r + 1];
        built[3 * r + 2] = left[3 * r] * s + left[3 * r + 2] * c;
    }
    Check(!ArmRowsStale(built, left), "arm rows: a degree apart are the game's");
    // The same yaw, another pitch: the mod's own all the same (the yaw is all the logs show of them).
    const float pitched[9] = {0.8f, 0.0f, -0.6f, 0.3f, 0.8f, 0.4f, 0.48f, -0.6f, 0.64f};
    Check(ArmRowsStale(pitched, left), "arm rows: the same yaw is the mod's own");
    // A float's step where the game's eye stands.
    Check(std::fabs(ArmGrain(1299.76f, -1140.6f) - 1.2207e-4f) < 1e-7f, "arm: a float's step at 1300 m is 0.12 mm", ArmGrain(1299.76f, -1140.6f) * 1000.0, 0.122);
    Check(std::fabs(ArmGrain(-300.0f, 2500.0f) - 2.4414e-4f) < 1e-7f, "arm: and 0.24 mm at 2500 m", ArmGrain(-300.0f, 2500.0f) * 1000.0, 0.244);
}

void ArmUnknowns() {
    // Two things not known of the game until it is tried. The rows the mod left not told apart from the game's at all (ArmRowsStale failing): the eye's step does not bear them out, and
    // the rule does not take them. And the eye's place flickering by a float's step in every build, where the rule takes it to move at the game's ticks alone.
    for (int seed = 1; seed <= 3; ++seed) {
        ArmGame g;
        g.jumps = 0.003;
        g.told = false;
        g.seed = seed;
        const ArmResult r = RunArm(g);
        const Spread s = SpreadOf(r.off[2]);
        Check(s.worst < 2.5 && s.p99 < 0.3, "arm: the mod's rows in 0.2% of builds, not told apart: never 2.5 mm off, 99% under 0.3", s.worst, 2.5);
        if (seed == 1)
            PrintArm("The mod's own rows in 0.2% of builds and NOT told apart:", r);
    }
    for (int seed = 1; seed <= 3; ++seed) {
        ArmGame g;
        g.jumps = 0.003;
        g.flicker = 0.000122;
        g.seed = seed;
        const ArmResult r = RunArm(g);
        const Spread s = SpreadOf(r.off[2]);
        Check(s.worst < 2.5 && s.p99 < 0.5, "arm: the eye's place flickering a float's step every build: never 2.5 mm off, 99% under 0.5", s.worst, 2.5);
        if (seed == 1)
            PrintArm("The eye's place flickering by a float's step (0.12 mm) in every build:", r);
    }
}

void ArmTime() {
    ArmGame g;
    g.builds = 72 * 600;
    const auto a = std::chrono::steady_clock::now();
    const ArmResult r = RunArm(g);
    const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - a).count() / g.builds;
    Check(us < 20.0, "arm: the made-up game and all three ways under 20 microseconds a build", us, 20.0);
    std::printf("a build of the made-up game with the arm told three ways takes %.2f microseconds; the rule's yaw came from the look written the build before %d times, this build's %d, an older "
                "one %d, the rows alone %d, the eye's step %d, the look at rest %d, kept %d\n",
                us, r.how[kArmBefore], r.how[kArmThis], r.how[kArmOlder], r.how[kArmRows], r.how[kArmStep], r.how[kArmRest], r.how[kArmKept]);
}

// ---- A log's "View:" lines replayed ----
// headmovetest --replay <log> [from hh:mm:ss] [to hh:mm:ss] [limit mm] [trace|half]: every view build of the log ("viewlog" must have been on) put through the three ways, and where the
// soldier stands (seconds in which the eye less its arm by the game's own rows keeps within 4 mm, all but two of its builds within 8) how far each way puts the cameras from his middle, by
// how fast the look turned. Passes if the rule's 99% is under the limit (1.5 mm) at every speed over 50 degrees a second with 20 builds or more.
// What a log is short of, against the game: its places are to the millimetre (nothing under about 0.4 mm shows; the rule is told so, ArmIn::grain, and its limits widen eightfold), and a
// build in nine has no line. Such a build is put back between its neighbours, with the look yaw it wrote taken from the game's own camera where that shows it ("half": half way between its
// neighbours' always, to see what that costs). "trace": each judged build the rule puts further off than the limit, with the two before it.
// Also reads the lines as columns (seconds, hand, written, camera, x, y, z, head, shown, two pitches, a mark, the time).
struct ViewLine {
    double t = 0.0, written = 0.0, camera = 0.0, x = 0.0, z = 0.0, shown = 0.0;
    bool look = false;
    char stamp[16] = {};
};

double Wrap180(double degrees) {
    return std::remainder(degrees, 360.0);
}

bool ParseView(const char* line, ViewLine& v) {
    char written[24] = {};
    double hand = 0.0, y = 0.0, head = 0.0;
    if (line[0] == '[') {
        const char* view = std::strstr(line, "] View: hand ");
        int hours = 0, minutes = 0;
        double seconds = 0.0;
        if (!view || view - line != 13 || sscanf_s(line + 1, "%d:%d:%lf", &hours, &minutes, &seconds) != 3)
            return false;
        if (sscanf_s(view + 2, "View: hand %lf written %23s | camera %lf at %lf %lf %lf | head %lf shown %lf", &hand, written, static_cast<unsigned>(sizeof(written)), &v.camera, &v.x, &y,
                     &v.z, &head, &v.shown) != 8)
            return false;
        std::memcpy(v.stamp, line + 1, 12);
        v.stamp[12] = 0;
        v.t = hours * 3600.0 + minutes * 60.0 + seconds;
        if (v.t < 12.0 * 3600.0)
            v.t += 86400.0;  // past midnight
    } else {
        double pitchHead = 0.0, pitchShown = 0.0;
        char mark[24] = {}, stamp[24] = {};
        if (sscanf_s(line, "%lf %lf %23s %lf %lf %lf %lf %lf %lf %lf %lf %23s %23s", &v.t, &hand, written, static_cast<unsigned>(sizeof(written)), &v.camera, &v.x, &y, &v.z, &head, &v.shown,
                     &pitchHead, &pitchShown, mark, static_cast<unsigned>(sizeof(mark)), stamp, static_cast<unsigned>(sizeof(stamp))) != 13)
            return false;
        std::memcpy(v.stamp, stamp, 12);
        v.stamp[12] = 0;
    }
    v.look = std::strcmp(written, "0.00") != 0;  // (no look written: the turn's yaw is 0)
    v.written = std::atof(written);
    return true;
}

int Replay(const char* path, const char* from, const char* to, double limit, const char* how6) {
    const bool trace = !std::strcmp(how6, "trace"), halfWay = !std::strcmp(how6, "half");
    FILE* f = nullptr;
    if (fopen_s(&f, path, "rb") != 0 || !f) {
        std::printf("No file at %s\n", path);
        return 2;
    }
    std::vector<ViewLine> lines;
    char text[1024];
    while (std::fgets(text, sizeof(text), f)) {
        ViewLine v;
        if (!ParseView(text, v) || (*from && std::strncmp(v.stamp, from, 8) < 0) || (*to && std::strncmp(v.stamp, to, 8) > 0))
            continue;
        lines.push_back(v);
    }
    std::fclose(f);
    if (lines.size() < 100) {
        std::printf("Only %zu \"View:\" lines in %s between those times\n", lines.size(), path);
        return 2;
    }
    // Every build: a log line is missing for about one in nine.
    struct Build {
        ViewLine v;
        bool real = true, gap = false;
    };
    std::vector<Build> builds;
    int putBack = 0;
    for (size_t i = 0; i < lines.size(); ++i) {
        const ViewLine& v = lines[i];
        bool gap = false;
        if (i > 0) {
            const ViewLine& u = lines[i - 1];
            const int frames = static_cast<int>((v.t - u.t) / 0.01389 + 0.5);
            if (frames == 2) {
                // One build without a line. Its look yaw: the game's camera, in one of the next three lines, standing on a yaw between its neighbours' that is neither of them, stands on
                // the one it wrote (the mod has every write; the log only the builds with a line); else half way. The camera's step, if there was one, at it or at the next line
                // (whichever the camera's yaw says).
                Build mid;
                mid.real = false;
                mid.v = u;
                mid.v.t = (v.t + u.t) / 2.0;
                const double whole = Wrap180(v.written - u.written);
                mid.v.written = u.written + whole / 2.0;
                for (size_t q = i; !halfWay && q < lines.size() && q < i + 3; ++q) {
                    const double part = Wrap180(lines[q].camera - u.written);
                    if (part * whole > 0.0 && std::fabs(part) > 0.015 && std::fabs(part) < std::fabs(whole) - 0.015) {
                        mid.v.written = lines[q].camera;
                        break;
                    }
                }
                mid.v.look = v.look && u.look;
                if ((v.x != u.x || v.z != u.z || std::fabs(Wrap180(v.camera - u.camera)) > 0.005) &&
                    std::fabs(Wrap180(v.camera - u.written)) <= std::fabs(Wrap180(v.camera - mid.v.written))) {
                    mid.v.camera = v.camera;
                    mid.v.x = v.x;
                    mid.v.z = v.z;
                }
                builds.push_back(mid);
                ++putBack;
            } else if (frames > 2) {
                gap = true;
            }
        }
        Build b;
        b.v = v;
        b.gap = gap;
        builds.push_back(b);
    }
    const int n = static_cast<int>(builds.size());
    constexpr int kWays = 4;  // the three, and the game's own rows in every build: what the log's rounding allows, and where the soldier stands
    const char* const names[kWays] = {"armby 0, as it was guessed", "armby 1, the rows at the eye's steps", "armby 2, the rule", "(the game's rows in every build)"};
    std::vector<double> px[kWays], pz[kWays];
    std::vector<double> rate(n, -1.0);
    std::vector<float> toldYaw(n, 0.0f);
    std::vector<int> toldHow(n, 0);
    std::vector<char> staleAt(n, 0);
    for (auto& p : px)
        p.resize(n);
    for (auto& p : pz)
        p.resize(n);
    ArmGuess guess;
    Arm rows, rule;
    int since = 0, staleBuilds = 0, how[kArmHows] = {};
    const float arm = 0.0773f;
    for (int j = 0; j < n; ++j) {
        const ViewLine& v = builds[j].v;
        const bool first = j == 0 || builds[j].gap;
        since = first ? 1 : since + 1;
        const float eye[3] = {static_cast<float>(v.x), 0.0f, static_cast<float>(v.z)};
        const float back[3] = {static_cast<float>(std::sin(v.camera * kDeg)), 0.0f, static_cast<float>(-std::cos(v.camera * kDeg))};
        const auto stored = [](double degrees) { return static_cast<float>((degrees - 360.0 * std::floor(degrees / 360.0)) * kDeg); };
        ArmIn in;
        in.eye[0] = eye[0];
        in.eye[1] = eye[2];
        in.grain = 0.001f;
        in.rows = true;
        in.rowsYaw = std::atan2(back[0], -back[2]);
        in.first = first;
        for (int k = 0; k < kArmYaws && k < since && builds[j - k].v.look; ++k)
            in.yaws[in.written++] = stored(builds[j - k].v.written);
        // The mod's own rows: the camera's yaw as built is the yaw the last picture showed, and no look yaw written lately.
        if (!first && builds[j].real && builds[j - 1].real && v.look && std::fabs(Wrap180(v.camera - builds[j - 1].v.shown)) <= 0.015) {
            in.stale = true;
            for (int k = 0; k < 4 && k < since; ++k)
                if (std::fabs(Wrap180(v.camera - builds[j - k].v.written)) <= 0.015)
                    in.stale = false;
        }
        staleBuilds += in.stale ? 1 : 0;
        const double dt = first ? 0.0 : std::min(0.1, v.t - builds[j - 1].v.t);
        ArmGuessStep(guess, eye, back, v.look, stored(v.written), arm, first ? 0.0f : 1.0f, static_cast<float>(dt));
        float forward[kWays][2];
        forward[0][0] = guess.forward[0], forward[0][1] = guess.forward[1];
        ArmTell(rows, in, forward[1], 1);
        ArmTell(rule, in, forward[2], 2);
        ++how[rule.how];
        toldYaw[j] = rule.yaw;
        toldHow[j] = rule.how;
        staleAt[j] = in.stale;
        forward[3][0] = -back[0], forward[3][1] = -back[2];
        for (int w = 0; w < kWays; ++w) {
            px[w][j] = v.x - static_cast<double>(arm) * forward[w][0];
            pz[w][j] = v.z - static_cast<double>(arm) * forward[w][1];
        }
        if (j > 0 && j + 1 < n && !builds[j].gap && !builds[j + 1].gap && v.look)
            rate[j] = std::fabs(Wrap180(builds[j + 1].v.written - builds[j - 1].v.written)) / (builds[j + 1].v.t - builds[j - 1].v.t);
    }
    // The seconds the soldier stands in, and his middle in each: by the game's own rows.
    std::vector<char> stands(n, 0);
    std::vector<double> middleX(n, 0.0), middleZ(n, 0.0);
    int seconds = 0, standing = 0;
    for (int j = 0; j < n;) {
        int e = j;
        while (e + 1 < n && !std::strncmp(builds[e + 1].v.stamp, builds[j].v.stamp, 8))
            ++e;
        const int count = e - j + 1;
        std::vector<double> ax(px[3].begin() + j, px[3].begin() + e + 1), az(pz[3].begin() + j, pz[3].begin() + e + 1);
        std::sort(ax.begin(), ax.end());
        std::sort(az.begin(), az.end());
        const int low = static_cast<int>(count * 0.05), high = std::max(low, static_cast<int>(count * 0.95) - 1);
        const double spread = std::hypot(ax[high] - ax[low], az[high] - az[low]) * 1000.0;
        const bool stand = count >= 40 && spread < 4.0 && std::hypot(ax[count - 2] - ax[1], az[count - 2] - az[1]) * 1000.0 < 8.0;  // (and all but two builds within 8 mm)
        for (int q = j; q <= e; ++q) {
            stands[q] = stand;
            middleX[q] = ax[(count + 1) / 2 - 1];
            middleZ[q] = az[(count + 1) / 2 - 1];
        }
        ++seconds;
        standing += stand ? 1 : 0;
        j = e + 1;
    }
    constexpr int kBins = 9;
    const double low[kBins] = {0, 30, 50, 150, 200, 400, 600, 750, 1100}, high[kBins] = {30, 50, 150, 200, 400, 600, 750, 1100, 1e9};
    std::vector<float> off[kWays][kBins], moved[kWays][kBins];
    for (int j = 1; j < n; ++j) {
        if (!stands[j] || !stands[j - 1] || builds[j].gap || rate[j] < 0.0 || !builds[j].real)
            continue;
        if (trace && std::hypot(px[2][j] - middleX[j], pz[2][j] - middleZ[j]) * 1000.0 > limit) {  // each judged build the rule puts further off than the limit, with the two before it
            for (int q = std::max(j - 2, 0); q <= j; ++q) {
                const ViewLine& v = builds[q].v;
                std::printf("  %s%s %4.0f deg/s: written %.2f, camera %.2f%s at %.3f %.3f, shown %.2f; told %.2f by %d; off %.2f (as guessed %.2f, the rows %.2f) mm\n", v.stamp,
                            builds[q].real ? " " : "+", rate[q], v.written, v.camera, staleAt[q] ? " (the mod's)" : "", v.x, v.z, v.shown, toldYaw[q] / kDeg, toldHow[q],
                            std::hypot(px[2][q] - middleX[q], pz[2][q] - middleZ[q]) * 1000.0, std::hypot(px[0][q] - middleX[q], pz[0][q] - middleZ[q]) * 1000.0,
                            std::hypot(px[3][q] - middleX[q], pz[3][q] - middleZ[q]) * 1000.0);
            }
            std::printf("\n");
        }
        for (int b = 0; b < kBins; ++b)
            if (rate[j] >= low[b] && rate[j] < high[b])
                for (int w = 0; w < kWays; ++w) {
                    off[w][b].push_back(static_cast<float>(std::hypot(px[w][j] - middleX[j], pz[w][j] - middleZ[j]) * 1000.0));
                    moved[w][b].push_back(static_cast<float>(std::hypot(px[w][j] - px[w][j - 1], pz[w][j] - pz[w][j - 1]) * 1000.0));
                }
    }
    std::printf("%s%s%s%s%s: %zu \"View:\" lines, %d builds (%d put back), %d seconds of which the soldier stands in %d; the mod's own rows in %d builds\n", path, *from ? " from " : "", from,
                *to ? " to " : "", to, lines.size(), n, putBack, seconds, standing, staleBuilds);
    std::printf("the rule's yaw came from the look written the build before %d times, this build's %d, an older one %d, the rows alone %d, the eye's step %d, the look at rest %d, a start %d, "
                "kept %d\n",
                how[kArmBefore], how[kArmThis], how[kArmOlder], how[kArmRows], how[kArmStep], how[kArmRest], how[kArmStart], how[kArmKept]);
    int failed = 0;
    for (int w = 0; w < kWays; ++w) {
        std::printf("== %s: the cameras off the soldier's middle, mm (mean 90%% 99%% worst) | moved from the last picture (mean 99%%)\n", names[w]);
        for (int b = 0; b < kBins; ++b) {
            if (off[w][b].empty())
                continue;
            const Spread s = SpreadOf(off[w][b]), m = SpreadOf(moved[w][b]);
            const bool judged = w == 2 && low[b] >= 50.0 && s.count >= 20;
            const bool bad = judged && !(s.p99 < limit);
            failed += bad ? 1 : 0;
            std::printf("   %5.0f to %-5.0f deg/s %5zu builds | %6.2f %6.2f %6.2f %7.2f | %6.2f %6.2f%s\n", low[b], std::min(high[b], 9999.0), s.count, s.mean, s.p90, s.p99, s.worst, m.mean, m.p99,
                        bad ? "   FAIL: 99% over the limit" : judged ? "   ok" : "");
        }
    }
    std::printf("%d of the rule's speeds over 50 degrees a second have their 99%% over %.1f mm\n", failed, limit);
    return failed;
}

}

int main(int argc, char** argv) {
    if (argc >= 3 && !std::strcmp(argv[1], "--replay"))
        return Replay(argv[2], argc > 3 ? argv[3] : "", argc > 4 ? argv[4] : "", argc > 5 ? std::atof(argv[5]) : 1.5, argc > 6 ? argv[6] : "");
    if (argc >= 2 && !std::strcmp(argv[1], "--arm")) {
        // headmovetest --arm [hz amp sway stale walk jumps flicker seed told]: one made-up game, each build the rule puts more than a millimetre off, and the three ways side by side.
        ArmGame g;
        double* const numbers[] = {&g.hz, &g.amp, &g.sway, &g.stale, &g.walk, &g.jumps, &g.flicker};
        for (int i = 0; i < 7 && i + 2 < argc; ++i)
            *numbers[i] = std::atof(argv[i + 2]);
        if (argc > 9)
            g.seed = std::atoi(argv[9]);
        if (argc > 10)
            g.told = std::atoi(argv[10]) != 0;
        g.trace = true;
        PrintArm("The made-up game (mm off the soldier's middle):", RunArm(g));
        return 0;
    }
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
    ArmStanding();
    ArmWalking();
    ArmStep();
    ArmWrongStart();
    ArmRows();
    ArmUnknowns();
    ArmTime();
    std::printf("%d of %d checks failed\n", g_failed, g_checks);
    return g_failed;
}

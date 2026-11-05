#include "headmove.h"

#include <windows.h>

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>

#include "camera.h"
#include "config.h"
#include "log.h"

namespace {

std::once_flag g_loaded;
std::atomic<bool> g_on{true}, g_walk{true}, g_crouch{true};
std::atomic<bool> g_recentered{false};
std::atomic<bool> g_stickMoving{false};

// The view builder's thread only.
headmove::State g_state;
headmove::Tuning g_tuning;
LARGE_INTEGER g_frequency{}, g_last{};
bool g_testWas = false;
float g_testBase[3] = {};
bool g_wasOn = false, g_loggedMove = false, g_wasCrouch = false, g_wasFollowing = false;

// What the render thread's input takes from the last build.
std::atomic<float> g_stickX{0.0f}, g_stickY{0.0f};
std::atomic<bool> g_following{false}, g_crouchWanted{false};
std::atomic<ULONGLONG> g_builtAt{0};
constexpr ULONGLONG kStale = 150;  // ms: no build for this long (a load, another mode), the outputs let go

// Tests (weapon.txt).
std::atomic<bool> g_testOn{false};
std::atomic<float> g_test[3] = {};
std::atomic<ULONGLONG> g_logUntil{0};
std::atomic<ULONGLONG> g_moveTestUntil{0};
std::atomic<float> g_moveTestX{0.0f}, g_moveTestY{0.0f};
std::atomic<float> g_leanFree{0.0f}, g_leanMost{0.0f};  // weapon.txt "sixlean": instead of bf3vr.ini's

// The game's camera isn't over the soldier's middle: it sits 7.73 cm ahead of it along its own level forward (fitted over 30 s of turning on the spot, 2026-11-04: 0.0773 forward, 0.0000 to
// the side, what's left 0.4 mm, the log's rounding; the same over the whole mission, whatever the look's pitch). So it swings round as the look yaw does, a step at each of the game's ticks,
// 1.3 mm a degree: the near world shook 3-10 mm a frame or two whenever the gun moved, or the head took it along past the aim's limit, with the head's place switched off too. The cameras go
// from the soldier's middle instead: the head's own place has the neck's swing in it already. Not in a ride or a cutscene, whose camera has no such arm (eased: it's 7.7 cm of view).
std::atomic<float> g_eyeArm{0.0773f};
float g_armShare = 0.0f;
float g_armForward[2] = {0.0f, 1.0f};
constexpr float kArmEase = 0.12f;  // seconds

// Once a second: how rough the place was, off a straight line, in mm: the game's eye, the same less its arm, the cameras' in the end.
struct Rough {
    double before[3] = {}, last[3] = {};
    int have = 0;
    float sum = 0.0f, most = 0.0f;
    void Add(const double* p) {
        if (have >= 2) {
            float r = 0.0f;
            for (int i = 0; i < 3; ++i) {
                const float d = static_cast<float>(p[i] - 2.0 * last[i] + before[i]);
                r += d * d;
            }
            r = std::sqrt(r) * 1000.0f;
            if (r < 100.0f) {  // not a cut
                sum += r;
                most = std::max(most, r);
            }
        }
        for (int i = 0; i < 3; ++i) {
            before[i] = last[i];
            last[i] = p[i];
        }
        have = std::min(have + 1, 2);
    }
    void Clear() { sum = most = 0.0f; }
};
Rough g_roughGame, g_roughPivot, g_roughFinal;
int g_roughBuilds = 0;
ULONGLONG g_roughSince = 0;

void NoteRough(const float* gameEye, const float* arm, const float* world) {
    double game[3], pivot[3], final[3];
    for (int i = 0; i < 3; ++i) {
        game[i] = gameEye[i];
        pivot[i] = static_cast<double>(gameEye[i]) + arm[i];
        final[i] = static_cast<double>(gameEye[i]) + world[i];
    }
    g_roughGame.Add(game);
    g_roughPivot.Add(pivot);
    g_roughFinal.Add(final);
    ++g_roughBuilds;
    const ULONGLONG tick = GetTickCount64();
    if (tick - g_roughSince < 1000)
        return;
    g_roughSince = tick;
    if (g_roughBuilds > 10) {
        const float n = static_cast<float>(g_roughBuilds);
        Log("Eye rough (mm a frame, worst): the game's %.2f %.2f, without its arm %.2f %.2f, the cameras' %.2f %.2f; the arm %.3f m at %.0f%%", g_roughGame.sum / n, g_roughGame.most,
            g_roughPivot.sum / n, g_roughPivot.most, g_roughFinal.sum / n, g_roughFinal.most, g_eyeArm.load(std::memory_order_relaxed), g_armShare * 100.0f);
    }
    g_roughGame.Clear();
    g_roughPivot.Clear();
    g_roughFinal.Clear();
    g_roughBuilds = 0;
}

void Load() {
    std::call_once(g_loaded, [] {
        const config::Settings& s = config::Get();
        g_on = s.headMove;
        g_walk = s.headWalk;
        g_crouch = s.headCrouch;
        g_tuning.leanFree = s.headLean;
        g_tuning.leanMost = std::max(s.headLeanMost, s.headLean + 0.01f);
        g_tuning.leanFar = g_tuning.leanMost + 0.23f;
        g_tuning.followStart = s.headFollowStart;
        g_tuning.stickFull = s.headStickFull;
    });
}

}

namespace headmove {

bool Build(const float* gameEye, const float* gameBack, float* world) {
    Load();
    const bool on = g_on.load(std::memory_order_relaxed);
    if (on != g_wasOn) {
        g_wasOn = on;
        Log("Head: its place in the room %s the view", on ? "moves" : "no longer moves");
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    if (!g_frequency.QuadPart)
        QueryPerformanceFrequency(&g_frequency);
    const float dt = g_last.QuadPart ? static_cast<float>(now.QuadPart - g_last.QuadPart) /
                                           static_cast<float>(g_frequency.QuadPart)
                                     : 0.0f;
    g_last = now;
    Input in;
    camera::ViewTurn turn{};
    const bool haveBody = camera::FrameBody(in.right, in.back, in.scripted, turn);
    // The game's eye less its arm (above): the soldier's middle.
    float arm[3] = {};
    if (haveBody) {
        const float wanted = in.scripted || turn.scriptedHint ? 0.0f : 1.0f;
        g_armShare += (wanted - g_armShare) * (1.0f - std::exp(-std::min(dt, 0.1f) / kArmEase));
        if (std::fabs(wanted - g_armShare) < 0.001f)
            g_armShare = wanted;
        // Its own forward as the arm has it: only from a build where the eye moved as far as that says it swung (half of it, for the rounding at 1500 m). The game's rows can change every frame while
        // its place steps at its ticks (2.4 mm of false arm when it draws the newest build), and now and then a build comes with the mod's last turned camera in its rows (a yaw 30 degrees off
        // for a frame, the eye where it was: the view hopped 4-8 cm, 21:11:11). In some launches they have it all the time (no hand's turn in them, the eye swinging 27 mm with its forward standing
        // still: the self-test's cameras row, on and off from launch to launch, 23:56), so with a look yaw written it's by that, which is what the eye swings by (20 degrees, to 0.1 mm, 23:56:05).
        static float s_eyeWas[3] = {};
        const float movedX = gameEye[0] - s_eyeWas[0], movedZ = gameEye[2] - s_eyeWas[2];
        std::memcpy(s_eyeWas, gameEye, sizeof(s_eyeWas));
        const bool byLook = turn.haveLook && turn.lookSign < 0.0f;
        const float level = std::sqrt(gameBack[0] * gameBack[0] + gameBack[2] * gameBack[2]);
        if (byLook || level > 0.03f) {  // (straight up or down the rows have no way of telling; the game keeps the look from there)
            const float fx = byLook ? -std::sin(turn.lookYaw) : -gameBack[0] / level, fz = byLook ? std::cos(turn.lookYaw) : -gameBack[2] / level;
            const float dx = fx - g_armForward[0], dz = fz - g_armForward[1];
            const float swing = g_eyeArm.load(std::memory_order_relaxed) * std::sqrt(dx * dx + dz * dz);
            // Refused for long, it isn't the eye waiting for its tick: the forward kept is the wrong one (a launch started it 123 degrees off, 00:03: every swing since came to too little to
            // correct it, the arm a fixed 14 cm off and none of the eye's swing taken out).
            static float s_refusedFor = 0.0f;
            s_refusedFor += std::min(dt, 0.1f);
            if (g_armShare == 0.0f || std::sqrt(movedX * movedX + movedZ * movedZ) >= 0.5f * swing || s_refusedFor > 0.25f) {
                g_armForward[0] = fx;
                g_armForward[1] = fz;
                s_refusedFor = 0.0f;
            }
        }
        const float by = g_eyeArm.load(std::memory_order_relaxed) * g_armShare;
        arm[0] = -by * g_armForward[0];
        arm[2] = -by * g_armForward[1];
    }
    if (!on || !haveBody) {
        g_following.store(false, std::memory_order_relaxed);
        g_crouchWanted.store(false, std::memory_order_relaxed);
        g_state.haveEye = false;  // a gap: no step across it to take for a walk
        g_state.haveRows = false;
        if (!haveBody || (arm[0] == 0.0f && arm[2] == 0.0f))
            return false;
        for (int i = 0; i < 3; ++i)
            world[i] = arm[i];
        camera::NoteBodyMove(world);
        NoteRough(gameEye, arm, world);
        return true;
    }
    in.dt = dt;
    for (int i = 0; i < 3; ++i) {
        in.head[i] = turn.headPlace[i];
        in.eye[i] = static_cast<double>(gameEye[i]) + arm[i];
    }
    for (int i = 0; i < 4; ++i)
        in.headQ[i] = turn.head[i];
    in.tracked = turn.placeTracked;
    in.scripted = in.scripted || turn.scriptedHint;
    in.stick = g_stickMoving.load(std::memory_order_relaxed);
    in.facingYaw = turn.facingYaw;
    in.recentered = g_recentered.exchange(false, std::memory_order_relaxed);
    g_tuning.walk = g_walk.load(std::memory_order_relaxed);
    g_tuning.crouch = g_crouch.load(std::memory_order_relaxed);
    if (const float free = g_leanFree.load(std::memory_order_relaxed); free > 0.0f) {
        g_tuning.leanFree = free;
        g_tuning.leanMost = g_leanMost.load(std::memory_order_relaxed);
        g_tuning.leanFar = g_tuning.leanMost + 0.23f;
    }

    const bool test = g_testOn.load(std::memory_order_relaxed);
    g_tuning.putDownAfter = test ? 1e9f : Tuning{}.putDownAfter;  // weapon.txt's head is as still as can be
    if (test) {
        if (!g_testWas)
            for (int i = 0; i < 3; ++i)
                g_testBase[i] = g_state.have ? g_state.lastHead[i] : in.head[i];
        for (int i = 0; i < 3; ++i)
            in.head[i] = g_testBase[i] + g_test[i].load(std::memory_order_relaxed);
        in.headQ[0] = in.headQ[1] = in.headQ[2] = 0.0f;
        in.headQ[3] = 1.0f;
        in.tracked = true;
    }
    g_testWas = test;

    Output out;
    Step(g_state, g_tuning, in, out);
    for (int i = 0; i < 3; ++i)
        world[i] = out.world[i] + arm[i];
    camera::NoteBodyMove(world);
    NoteRough(gameEye, arm, world);
    g_stickX.store(out.stickX, std::memory_order_relaxed);
    g_stickY.store(out.stickY, std::memory_order_relaxed);
    g_following.store(out.following, std::memory_order_relaxed);
    g_crouchWanted.store(out.crouch, std::memory_order_relaxed);
    g_builtAt.store(GetTickCount64(), std::memory_order_relaxed);

    if (!g_loggedMove && (std::fabs(out.room[0]) > 0.02f || std::fabs(out.room[1]) > 0.02f || std::fabs(out.room[2]) > 0.02f)) {
        g_loggedMove = true;
        Log("Head: the view moved with the head (%.0f %.0f %.0f cm right, up, back)%s", out.room[0] * 100.0f,
            out.room[1] * 100.0f, out.room[2] * 100.0f, test ? ", from weapon.txt's headtest" : "");
    }
    if (out.crouch != g_wasCrouch) {
        g_wasCrouch = out.crouch;
        Log("Head: %s for real (the neck %.0f cm below standing): the game's crouch %s", out.crouch ? "crouching" : "standing",
            out.drop * 100.0f, out.crouch ? "held" : "let go");
    }
    if (out.following != g_wasFollowing) {
        g_wasFollowing = out.following;
        static int s_logged = 0;
        if (s_logged < 40) {
            ++s_logged;
            Log("Head: %s (head %.0f cm from the soldier, %.2f m walked so far along the room's right, %.2f along its back)",
                out.following ? "walking the soldier under the head" : "the soldier's under the head, or stopped",
                std::sqrt(out.raw[0] * out.raw[0] + out.raw[2] * out.raw[2]) * 100.0f, g_state.walked[0], g_state.walked[1]);
        }
    }
    const ULONGLONG tick = GetTickCount64();
    if (tick < g_logUntil.load(std::memory_order_relaxed)) {  // every build
        Log("Six: head %.4f %.4f %.4f%s raw %.3f %.3f %.3f | room %.4f %.4f %.4f world %.4f %.4f %.4f | eye %.3f %.3f %.3f | "
            "walked %.3f %.3f | stick %.2f %.2f%s%s%s | comp %.3f drop %.3f neck %.3f | right %.4f %.4f back %.4f %.4f facing %.1f",
            in.head[0], in.head[1], in.head[2], in.tracked ? "" : " (lost)", out.raw[0], out.raw[1], out.raw[2], out.room[0],
            out.room[1], out.room[2], world[0], world[1], world[2], gameEye[0], gameEye[1], gameEye[2],
            g_state.walked[0], g_state.walked[1], out.stickX, out.stickY, out.following ? " following" : "",
            out.crouch ? " crouched" : "", g_state.scripted ? " scripted" : "", out.comp, out.drop, out.neck, in.right[0],
            in.right[2], in.back[0], in.back[2], in.facingYaw * 57.29578f);
    }
    return true;
}

void NoteRecentered() {
    g_recentered.store(true, std::memory_order_relaxed);
}

void SetEyeArm(float metres) {
    if (g_eyeArm.exchange(metres) != metres)
        Log("Head: the game's eye taken %.4f m back along its own forward", metres);
}

void NoteStick(bool moving) {
    g_stickMoving.store(moving, std::memory_order_relaxed);
}

bool FollowStick(float& x, float& y) {
    const ULONGLONG now = GetTickCount64();
    if (now < g_moveTestUntil.load(std::memory_order_relaxed)) {
        x = g_moveTestX.load(std::memory_order_relaxed);
        y = g_moveTestY.load(std::memory_order_relaxed);
        return true;
    }
    if (!g_following.load(std::memory_order_relaxed) || now - g_builtAt.load(std::memory_order_relaxed) > kStale)
        return false;
    x = g_stickX.load(std::memory_order_relaxed);
    y = g_stickY.load(std::memory_order_relaxed);
    return true;
}

bool CrouchWanted() {
    return g_crouchWanted.load(std::memory_order_relaxed) &&
           GetTickCount64() - g_builtAt.load(std::memory_order_relaxed) <= kStale;
}

void SetOn(bool on) {
    Load();
    g_on = on;
}

void SetWalk(bool on) {
    Load();
    if (g_walk.exchange(on) != on)
        Log("Head: walking the soldier under the head %s", on ? "on" : "off");
}

void SetCrouch(bool on) {
    Load();
    if (g_crouch.exchange(on) != on)
        Log("Head: crouching for real %s", on ? "on" : "off");
}

void SetHeadTest(const float* place) {
    Load();
    if (place) {
        for (int i = 0; i < 3; ++i)
            g_test[i].store(place[i], std::memory_order_relaxed);
        g_testOn = true;
        Log("Head: test: the head %.3f %.3f %.3f m from where it is (right, up, back)", place[0], place[1], place[2]);
    } else if (g_testOn.exchange(false)) {
        Log("Head: test off");
    }
}

bool TestOn() {
    return g_testOn.load(std::memory_order_relaxed);
}

void SetLean(float free, float most) {
    Load();
    g_leanMost = std::max(most, free + 0.01f);
    g_leanFree = free;
    Log("Head: the view moves one to one up to %.2f m along the ground, at most %.2f m", free, most);
}

void LogFor(int seconds) {
    g_logUntil.store(GetTickCount64() + 1000ull * seconds, std::memory_order_relaxed);
}

void StartMoveTest(float length, float seconds, float degrees) {
    g_moveTestX.store(length * std::sin(degrees * 0.01745329f), std::memory_order_relaxed);
    g_moveTestY.store(length * std::cos(degrees * 0.01745329f), std::memory_order_relaxed);
    g_moveTestUntil.store(GetTickCount64() + static_cast<ULONGLONG>(seconds * 1000.0f), std::memory_order_relaxed);
    Log("Head: walking test: the stick %.2f of whole for %.1f s, %.0f degrees right of the way the game walks", length,
        seconds, degrees);
}

}

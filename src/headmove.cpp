#include "headmove.h"

#include <windows.h>

#include <atomic>
#include <cmath>
#include <cstdio>
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

bool Build(const float* gameEye, float* world) {
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
    if (!on || !camera::FrameBody(in.right, in.back, in.scripted, turn)) {
        g_following.store(false, std::memory_order_relaxed);
        g_crouchWanted.store(false, std::memory_order_relaxed);
        g_state.haveEye = false;  // a gap: no step across it to take for a walk
        g_state.haveRows = false;
        return false;
    }
    in.dt = dt;
    for (int i = 0; i < 3; ++i) {
        in.head[i] = turn.headPlace[i];
        in.eye[i] = gameEye[i];
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
        world[i] = out.world[i];
    camera::NoteBodyMove(world);
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
    static ULONGLONG s_logged = 0;
    if (tick < g_logUntil.load(std::memory_order_relaxed) && tick - s_logged >= 100) {
        s_logged = tick;
        Log("Six: head %.3f %.3f %.3f%s raw %.3f %.3f %.3f | room %.3f %.3f %.3f world %.3f %.3f %.3f | eye %.3f %.3f %.3f | "
            "walked %.3f %.3f | stick %.2f %.2f%s%s%s | comp %.3f drop %.3f neck %.3f | right %.3f %.3f back %.3f %.3f facing %.1f",
            in.head[0], in.head[1], in.head[2], in.tracked ? "" : " (lost)", out.raw[0], out.raw[1], out.raw[2], out.room[0],
            out.room[1], out.room[2], out.world[0], out.world[1], out.world[2], gameEye[0], gameEye[1], gameEye[2],
            g_state.walked[0], g_state.walked[1], out.stickX, out.stickY, out.following ? " following" : "",
            out.crouch ? " crouched" : "", g_state.scripted ? " scripted" : "", out.comp, out.drop, out.neck, in.right[0],
            in.right[2], in.back[0], in.back[2], in.facingYaw * 57.29578f);
    }
    return true;
}

void NoteRecentered() {
    g_recentered.store(true, std::memory_order_relaxed);
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

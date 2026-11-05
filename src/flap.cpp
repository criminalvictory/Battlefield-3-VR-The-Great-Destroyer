#include "flap.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace flap {
namespace {

constexpr float kAfterShot = 0.35f;   // s from the shot until it starts folding
constexpr float kFoldTime = 0.55f;    // s to fold (the game's takes 0.9)
constexpr float kSpringTime = 0.14f;  // s to spring open
constexpr float kArm = 0.4f, kTake = 0.6f;
constexpr float kReach = 0.14f;       // how near where the hand takes it
constexpr float kLost = 0.30f;        // the real hand this far off the way: let go
constexpr float kOpensAt = 0.75f;     // this far along its way it springs open by itself
constexpr float kHandInGlove[3] = {0.5363f, 1.0976f, 0.2092f};

enum class State { kReady, kWaiting, kFolding, kFolded, kLifting, kSpringing };
State g_state = State::kReady;
float g_time = 0.0f, g_at = 0.0f, g_blend = 0.0f, g_gloveInGun[12] = {};
bool g_armedGrip = true, g_armedTrigger = true;

void Compose(const float* a, const float* b, float* out) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j)
            out[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        out[i * 4 + 3] += a[i * 4 + 3];
    }
}

void InvertRigid(const float* m, float* out) {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            out[i * 4 + j] = m[j * 4 + i];
    for (int i = 0; i < 3; ++i)
        out[i * 4 + 3] = -(out[i * 4] * m[3] + out[i * 4 + 1] * m[7] + out[i * 4 + 2] * m[11]);
}

void Transform(const float* m, const float* p, float* out) {
    for (int i = 0; i < 3; ++i)
        out[i] = m[i * 4] * p[0] + m[i * 4 + 1] * p[1] + m[i * 4 + 2] * p[2] + m[i * 4 + 3];
}

float Apart(const float* a, const float* b) {
    const float d[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    return std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
}

// Between two keys of a table.
void Panels(const float (*table)[4][12], int keys, float at, Result& r) {
    const int k = std::clamp(static_cast<int>(at), 0, keys - 2);
    const float t = std::clamp(at - k, 0.0f, 1.0f);
    for (int b = 0; b < 4; ++b)
        for (int j = 0; j < 12; ++j)
            r.bones[b][j] = table[k][b][j] + (table[k + 1][b][j] - table[k][b][j]) * t;
    r.panels = true;
}

}  // namespace

Result Update(const Frame& f) {
    Result r;
    const float dt = std::clamp(f.dt, 0.0f, 0.1f);
    const bool crossedGrip = f.left && g_armedGrip && f.squeeze > kTake, crossedTrigger = f.left && g_armedTrigger && f.trigger > kTake;
    if (f.squeeze < kArm)
        g_armedGrip = true;
    if (f.trigger < kArm)
        g_armedTrigger = true;
    if (crossedGrip)
        g_armedGrip = false;
    if (crossedTrigger)
        g_armedTrigger = false;
    const float grasp = std::max(f.squeeze, f.trigger);
    if (!f.aerial) {
        g_blend = 0.0f;
        if (g_state == State::kLifting)  // put away with the hand on it
            g_state = State::kFolded;
        return r;
    }
    const Aerial& a = *f.aerial;
    if (g_state == State::kReady && f.shot && f.spare) {
        g_state = State::kWaiting;
        g_time = 0.0f;
    }
    r.empty = g_state != State::kReady;
    float inverse[12], palm[3], onGun[3];
    InvertRigid(f.gunPose, inverse);
    Transform(f.glove, kHandInGlove, palm);
    Transform(inverse, palm, onGun);
    switch (g_state) {
    case State::kReady:
        break;
    case State::kWaiting:
        g_time += dt;
        if (g_time >= kAfterShot) {
            g_state = State::kFolding;
            g_time = 0.0f;
        }
        break;
    case State::kFolding:
        g_time += dt;
        Panels(a.foldBones, a.foldKeys, g_time / kFoldTime * (a.foldKeys - 1), r);
        if (g_time >= kFoldTime) {
            g_state = State::kFolded;
            r.folded = true;
        }
        break;
    case State::kFolded:
        Panels(a.foldBones, a.foldKeys, static_cast<float>(a.foldKeys - 1), r);
        if ((crossedGrip || crossedTrigger) && f.left && f.gunTracked && f.handFree && Apart(onGun, a.liftHand[0]) < kReach) {
            g_state = State::kLifting;
            g_at = 0.0f;
            r.taken = true;
            r.spentSqueeze = true;
        }
        break;
    case State::kLifting: {
        // Along the way the hand takes it, as the game's hand goes.
        float nearest = 1e9f, at = g_at;
        for (int k = 0; k + 1 < a.liftKeys; ++k) {
            const float* p = a.liftHand[k];
            const float* q = a.liftHand[k + 1];
            const float pq[3] = {q[0] - p[0], q[1] - p[1], q[2] - p[2]}, ph[3] = {onGun[0] - p[0], onGun[1] - p[1], onGun[2] - p[2]};
            const float ll = pq[0] * pq[0] + pq[1] * pq[1] + pq[2] * pq[2];
            const float t = ll > 1e-8f ? std::clamp((ph[0] * pq[0] + ph[1] * pq[1] + ph[2] * pq[2]) / ll, 0.0f, 1.0f) : 0.0f;
            const float c[3] = {p[0] + pq[0] * t, p[1] + pq[1] * t, p[2] + pq[2] * t};
            const float d = Apart(onGun, c);
            if (d < nearest)
                nearest = d, at = k + t;
        }
        if (grasp < kArm || !f.left || nearest > kLost) {  // let go of before it's up: it stays folded
            g_state = State::kFolded;
            Panels(a.foldBones, a.foldKeys, static_cast<float>(a.foldKeys - 1), r);
            break;
        }
        g_at = std::max(g_at, at);  // up only
        if (g_at >= (a.liftKeys - 1) * kOpensAt) {
            g_state = State::kSpringing;
            g_time = 0.0f;
        }
        Panels(a.liftBones, a.liftKeys, g_at, r);
        const int k = std::clamp(static_cast<int>(g_at), 0, a.liftKeys - 2);
        const float t = std::clamp(g_at - k, 0.0f, 1.0f);
        for (int j = 0; j < 12; ++j)
            g_gloveInGun[j] = a.liftGlove[k][j] + (a.liftGlove[k + 1][j] - a.liftGlove[k][j]) * t;
        g_blend = std::min(g_blend + 0.35f, 1.0f);
        Compose(f.gunPose, g_gloveInGun, r.glove);
        r.gloveBlend = g_blend;
        r.fingers = reinterpret_cast<const float (*)[12]>(a.liftFingers + (t < 0.5f ? k : k + 1) * 23 * 12);
        r.holding = true;
        return r;
    }
    case State::kSpringing:
        g_time += dt;
        Panels(a.springBones, a.springKeys, g_time / kSpringTime * (a.springKeys - 1), r);
        if (g_time >= kSpringTime) {
            g_state = State::kReady;
            r.opened = true;
            r.panels = false;
            r.empty = false;
        }
        break;
    }
    // The glove eases back to the hand.
    if (g_blend > 0.0f) {
        g_blend = std::max(g_blend - 0.35f, 0.0f);
        Compose(f.gunPose, g_gloveInGun, r.glove);
        r.gloveBlend = g_blend;
    }
    return r;
}

void Reset() {
    g_state = State::kReady;
    g_blend = 0.0f;
}

}

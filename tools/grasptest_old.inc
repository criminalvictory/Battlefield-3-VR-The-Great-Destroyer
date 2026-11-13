// tools/grasptest.cpp's reference: src/grasp.h as it was before the game's dropped guns (commit d699493), word for word but for these: all of
// it is inside namespace old_grasp, and its #pragma once and its own #includes are gone (grasptest.cpp includes what it needs before it).
// Never built by itself.
namespace old_grasp {

// Who holds the rifle with free hands, worked out on its own so tools/grasptest.cpp can run it without the
// game (hands.cpp builds the Frame each frame before the modules and owns the State). Four ways:
//   kHeld   a hand on its pistol grip (the trigger hand); the other may support it on the handguard
//   kBarrel a hand on its handguard alone (or the left on the M320's grip): the rifle frozen on that hand,
//           still aimed, nothing fires
//   kGlide  flying into a hand by its pistol grip for Frame::glide seconds: picked up from the world, or
//           taken from a barrel hold (a regrip: the barrel hand stays on as the support)
//   kLoose  in the world
// Everything is by side, [0] left and [1] right.
//
// The user's rules (2026-11-06): the grip button holds the gun on either hand; let go and it stays only if
// the other hand holds the barrel, else it falls; whichever hand picks it up holds it by its pistol grip
// and its trigger fires it. The defaults told them: at a spawn, a checkpoint or a cutscene's end the rifle
// is in the right hand and drops only after that grip has been squeezed and let go; the headset off or a
// controller asleep freezes the hold; the stand-in counts as squeezing (own false: it never arms, drops or
// takes).
//   - A hand takes on a fresh squeeze: past kTake within kReachWindow of crossing it from below kArm, not
//     busy, and not within kRetakeAfter of its own let-go.
//   - A squeeze a module takes (claimed: the free hand on the seated magazine or at the charging handle)
//     never regrips the rifle held by its handguard alone; only a test's forced take still does.
//   - The pistol-grip hand lets go once it has been past kTake since it took hold (armed) and then under
//     kLetGo for kLetGoFrames frames. A grip not read, a hand not tracked, the game's reload or a scripted
//     moment disarm it: none of them drops the rifle, then or after.
//   - Nothing changes hands in a scripted moment (frozen); a glide under way finishes.
//   - A new life, the rifle lost on the ground, or free hands not on for it (another gun, seated, off):
//     back in the right hand as at a spawn (returned).


namespace grasp {

constexpr float kArm = 0.4f, kTake = 0.6f, kLetGo = 0.35f, kReachWindow = 0.4f, kRetakeAfter = 0.25f, kHeadEase = 0.25f;
constexpr int kLetGoFrames = 3;

enum class Holder : uint8_t { kHeld, kBarrel, kGlide, kLoose };

struct HandIn {
    bool tracked = false, own = false, read = false;  // seen; a real controller (not the stand-in); its grip read this frame
    float squeeze = 0.0f, trigger = 0.0f;              // 0-1
    float pose[12] = {};                                // its grip pose (room; three rows: rotation | translation)
    bool busy = false, onSupport = false, forceTake = false;  // last frame: its hands full; on the handguard; a test's take
    // This frame's squeeze is a module's to take (hands.cpp: the hand on the seated magazine as pouch::OnSeated has it, or at the charging
    // handle as reload::NearHandle): with the rifle held by its handguard alone it doesn't regrip with it (a test's forceTake still does).
    bool claimed = false;
};

struct Frame {
    float dt = 0.0f;  // seconds (clamped 0-0.1)
    bool enabled = false, frozen = false, newLife = false, gameReload = false;
    HandIn hand[2];
    bool haveGun = false; float gunRoom[12] = {};   // the rifle as drawn last frame (model -> room), a glide's too
    bool lying = false; float lyingRoom[12] = {};   // loose's rifle after last frame's step (model -> room)
    bool lost = false;                               // loose lost it (no ground, numbers not finite)
    float gripPoint[2][3] = {};                      // each side's pistol-grip point (model): [1] the right palm's, [0] it mirrored
    float boxMin[3] = {}, boxMax[3] = {};            // the hull's box (model): a pickup's reach is measured to it
    float glide = 0.12f, pickReach = 0.25f, regripReach = 0.15f;  // freehands.txt "glide", "reach" (freetune), passed in: no globals
};

struct State {
    Holder holder = Holder::kHeld;
    int trigger = 1, barrel = -1;  // the pistol-grip hand (kHeld, kGlide), the handguard hand (kBarrel, a regrip's glide); -1 none
    bool armed[2] = {};            // squeezed past kTake since it took hold: its let-go drops the rifle
    bool fireReset[2] = {false, true};  // its trigger has been under kArm since it took hold
    int low[2] = {};               // frames under kLetGo while armed
    bool armTake[2] = {};          // under kArm since its last crossing: the next crossing of kTake is fresh
    float crossedAt[2] = {-1.0f, -1.0f}, letGoAt[2] = {-10.0f, -10.0f};  // clock seconds (-1: no crossing to take with)
    float glideTime = 0.0f;
    bool glideFromWorld = false;
    float headShare = 0.0f, clock = 0.0f;
    Holder beforeHolder = Holder::kHeld;  // before the latest release (NotLetGo goes back to it)
    int beforeTrigger = 1, beforeBarrel = -1;
};

struct Result {
    Holder holder = Holder::kHeld;
    int trigger = 1, barrel = -1, fireHand = 1;  // fireHand: whose trigger fires it (kHeld once its trigger has been under kArm), -1 none
    float glide = 1.0f;                          // kGlide: 0-1 of the way into the hand (smoothed); 1 otherwise
    bool glideFromWorld = false;
    float headShare = 0.0f, reach = 0.0f;        // how far the game's aim has gone to the head; a take's distance (m)
    bool armed[2] = {}, spent[2] = {};           // spent: that hand took it this frame (its fresh squeeze used up)
    int letGo = -1, toBarrel = -1, released = -1, pickedUp = -1, regrip = -1, arrived = -1;  // this frame's events: the side
    bool returned = false;                       // back in the right hand (why says what put it back)
    const char* why = "";
};

inline void Clear(State& s);
inline void NotLetGo(State& s);
inline Result Update(const Frame& f, State& s);
inline float Smooth(float u);

// ---- Pieces ----

inline float Smooth(float u) {
    u = u > 0.0f ? (u < 1.0f ? u : 1.0f) : 0.0f;  // not a number: 0
    return u * u * (3.0f - 2.0f * u);
}

inline bool Finite(const float* v, int n) {
    for (int i = 0; i < n; ++i)
        if (!std::isfinite(v[i]))
            return false;
    return true;
}

// The room point p in the model space of a rigid pose m (model -> room, three rows: rotation | translation).
inline void ToModel(const float m[12], const float p[3], float out[3]) {
    const float d[3] = {p[0] - m[3], p[1] - m[7], p[2] - m[11]};
    for (int i = 0; i < 3; ++i)
        out[i] = m[i] * d[0] + m[4 + i] * d[1] + m[8 + i] * d[2];
}

// Metres from p to the box lo-hi (0 inside it).
inline float BoxDistance(const float p[3], const float lo[3], const float hi[3]) {
    float sum = 0.0f;
    for (int i = 0; i < 3; ++i) {
        const float out = p[i] < lo[i] ? lo[i] - p[i] : p[i] > hi[i] ? p[i] - hi[i] : 0.0f;
        sum += out * out;
    }
    return std::sqrt(sum);
}

inline void Clear(State& s) { s = State{}; }

// The loose start didn't happen (no room in loose, loose off, no frame): back to how it was held, unarmed.
inline void NotLetGo(State& s) {
    if (s.holder != Holder::kLoose)
        return;
    s.holder = s.beforeHolder;
    s.trigger = s.beforeTrigger;
    s.barrel = s.beforeBarrel;
    s.armed[0] = s.armed[1] = false;
    s.low[0] = s.low[1] = 0;
    s.glideTime = 0.0f;
    s.glideFromWorld = false;
}

// ---- The update, once a frame ----

inline Result Update(const Frame& f, State& s) {
    Result r;
    // Sanitized: a pose with a number not finite is a hand not tracked; a grip not finite is 0 and not read
    // (it never lets go), a trigger not finite 0 and not read (it re-arms nothing).
    const float dt = std::isfinite(f.dt) ? (f.dt > 0.0f ? (f.dt < 0.1f ? f.dt : 0.1f) : 0.0f) : 0.0f;
    struct Hand {
        bool tracked, own, read, live, busy, onSupport, forceTake, triggerRead, claimed;
        float squeeze, trigger, at[3];
    } h[2];
    for (int i = 0; i < 2; ++i) {
        const HandIn& in = f.hand[i];
        Hand& o = h[i];
        o.tracked = in.tracked && Finite(in.pose, 12);
        o.own = in.own;
        const bool squeezeOk = std::isfinite(in.squeeze), triggerOk = std::isfinite(in.trigger);
        o.read = in.read && squeezeOk;
        o.triggerRead = in.read && triggerOk;
        o.squeeze = squeezeOk ? (in.squeeze > 0.0f ? (in.squeeze < 1.0f ? in.squeeze : 1.0f) : 0.0f) : 0.0f;
        o.trigger = triggerOk ? (in.trigger > 0.0f ? (in.trigger < 1.0f ? in.trigger : 1.0f) : 0.0f) : 0.0f;
        o.live = o.own && o.read && o.tracked;
        o.busy = in.busy;
        o.onSupport = in.onSupport;
        o.forceTake = in.forceTake;
        o.claimed = in.claimed;
        o.at[0] = o.tracked ? in.pose[3] : 0.0f;
        o.at[1] = o.tracked ? in.pose[7] : 0.0f;
        o.at[2] = o.tracked ? in.pose[11] : 0.0f;
    }
    const bool haveGun = f.haveGun && Finite(f.gunRoom, 12);
    const bool lying = f.lying && Finite(f.lyingRoom, 12);
    const bool boxOk = Finite(f.boxMin, 3) && Finite(f.boxMax, 3);
    const bool gripOk[2] = {Finite(f.gripPoint[0], 3), Finite(f.gripPoint[1], 3)};
    const float glide = std::isfinite(f.glide) && f.glide > 0.0f ? f.glide : 0.12f;
    const float pickReach = std::isfinite(f.pickReach) && f.pickReach >= 0.0f ? f.pickReach : 0.25f;
    const float regripReach = std::isfinite(f.regripReach) && f.regripReach >= 0.0f ? f.regripReach : 0.15f;
    const bool inOrder = (s.holder == Holder::kHeld && (s.trigger == 0 || s.trigger == 1) && s.barrel == -1) ||
                         (s.holder == Holder::kBarrel && s.trigger == -1 && (s.barrel == 0 || s.barrel == 1)) ||
                         (s.holder == Holder::kGlide && (s.trigger == 0 || s.trigger == 1) &&
                          (s.barrel == -1 || s.barrel == 1 - s.trigger)) ||
                         (s.holder == Holder::kLoose && s.trigger == -1 && s.barrel == -1);
    s.clock += dt;

    // 1. Each hand's squeeze and trigger.
    bool fresh[2];
    for (int i = 0; i < 2; ++i) {
        const Hand& o = h[i];
        if (o.own && o.read && o.squeeze < kArm)
            s.armTake[i] = true;
        if (o.live && s.armTake[i] && o.squeeze >= kTake)
            s.crossedAt[i] = s.clock, s.armTake[i] = false;
        fresh[i] = o.forceTake || (o.live && !o.busy && s.crossedAt[i] >= 0.0f && s.clock - s.crossedAt[i] <= kReachWindow &&
                                   o.squeeze >= kTake && s.clock - s.letGoAt[i] > kRetakeAfter);
        if (o.triggerRead && o.trigger < kArm)
            s.fireReset[i] = true;
    }

    // 2. Back in the right hand, as at a spawn.
    if (f.newLife || f.lost || !f.enabled || !inOrder) {
        const bool changed = !(s.holder == Holder::kHeld && s.trigger == 1);
        Clear(s);
        r.returned = changed;
        r.why = f.newLife ? "a new life"
                : f.lost  ? "the ground lost it"
                : !f.enabled ? "another gun, seated or free hands off"
                             : "its hold out of order";
        r.holder = s.holder, r.trigger = s.trigger, r.barrel = s.barrel;
        r.fireHand = s.fireReset[s.trigger] ? s.trigger : -1;
        return r;
    }

    // 3. The pistol-grip hand: armed once past kTake, let go after kLetGoFrames under kLetGo.
    bool letGoNow = false;
    if (s.holder == Holder::kHeld || s.holder == Holder::kGlide) {
        const int t = s.trigger;
        if (!h[t].live || f.gameReload || f.frozen) {
            s.armed[t] = false, s.low[t] = 0;
        } else {
            if (h[t].squeeze >= kTake)
                s.armed[t] = true;
            s.low[t] = s.armed[t] && h[t].squeeze < kLetGo ? (s.low[t] < kLetGoFrames ? s.low[t] + 1 : kLetGoFrames) : 0;
        }
        letGoNow = !f.frozen && haveGun && s.low[t] >= kLetGoFrames;
    }
    const auto before = [&s](Holder holder, int trigger, int barrel) {
        s.beforeHolder = holder, s.beforeTrigger = trigger, s.beforeBarrel = barrel;
    };
    const auto take = [&s, &r, &h](int side) {
        s.armed[side] = true;
        s.low[side] = 0;
        s.fireReset[side] = h[side].triggerRead && h[side].trigger < kArm;
        s.crossedAt[side] = -1.0f;
        r.spent[side] = true;
    };
    // Let go of the pistol grip: held on by the other hand's hold on the handguard, else into the world.
    const auto letGo = [&](int t, bool barrelPossible) {
        before(Holder::kHeld, t, -1);
        r.letGo = t;
        s.armed[t] = false;
        s.low[t] = 0;
        s.letGoAt[t] = s.clock;
        const int o = 1 - t;
        if (barrelPossible && h[o].onSupport && h[o].tracked) {
            s.holder = Holder::kBarrel, s.trigger = -1, s.barrel = o;
            r.toBarrel = o;
        } else {
            s.holder = Holder::kLoose, s.trigger = -1, s.barrel = -1;
            r.released = t;
            s.headShare = 0.0f;
        }
    };

    // 4. Who has it now.
    switch (s.holder) {
    case Holder::kHeld:
        if (letGoNow)
            letGo(s.trigger, true);
        break;
    case Holder::kBarrel: {
        const int b = s.barrel, g = 1 - b;
        if (!f.frozen && haveGun && !h[b].onSupport) {
            before(Holder::kBarrel, -1, b);
            s.letGoAt[b] = s.clock;
            s.holder = Holder::kLoose, s.trigger = -1, s.barrel = -1;
            r.released = b;
            s.headShare = 0.0f;
        } else if (!f.frozen && fresh[g] && (h[g].forceTake || !h[g].claimed) && haveGun) {  // a claimed squeeze is the module's
            float d = 1e9f;
            if (h[g].forceTake)
                d = 0.0f;
            else if (h[g].tracked && gripOk[g]) {
                float p[3];
                ToModel(f.gunRoom, h[g].at, p);
                const float a = p[0] - f.gripPoint[g][0], c = p[1] - f.gripPoint[g][1], e = p[2] - f.gripPoint[g][2];
                d = std::sqrt(a * a + c * c + e * e);
            }
            if (d < regripReach) {
                s.holder = Holder::kGlide, s.trigger = g;  // the barrel hand stays on as the support
                s.glideTime = 0.0f, s.glideFromWorld = false;
                take(g);
                r.regrip = g;
                r.reach = d;
            }
        }
        break;
    }
    case Holder::kGlide:
        if (letGoNow) {
            // From a regrip the barrel hand still holds the handguard: the rifle stays in that hold.
            letGo(s.trigger, !s.glideFromWorld);
        } else {
            s.glideTime += dt;
            if (s.glideTime >= glide) {
                s.holder = Holder::kHeld, s.barrel = -1;
                r.arrived = s.trigger;
            }
        }
        break;
    case Holder::kLoose:
        s.headShare = s.headShare + dt / kHeadEase < 1.0f ? s.headShare + dt / kHeadEase : 1.0f;
        if (!f.frozen && lying) {
            int best = -1;
            float bestD = 0.0f;
            for (int i = 1; i >= 0; --i) {  // the right first: a tie goes to it
                if (!fresh[i])
                    continue;
                float d = 1e9f;
                if (h[i].forceTake)
                    d = 0.0f;
                else if (h[i].tracked && boxOk) {
                    float p[3];
                    ToModel(f.lyingRoom, h[i].at, p);
                    d = BoxDistance(p, f.boxMin, f.boxMax);
                }
                if (d < pickReach && (best < 0 || d < bestD))
                    best = i, bestD = d;
            }
            if (best >= 0) {
                s.holder = Holder::kGlide, s.trigger = best, s.barrel = -1;
                s.glideTime = 0.0f, s.glideFromWorld = true;
                take(best);
                r.pickedUp = best;
                r.reach = bestD;
            }
        }
        break;
    }

    // 5. The game's aim: to the head while it lies, back to the gun as it's held.
    const float u = s.holder == Holder::kGlide ? Smooth(s.glideTime / glide) : 1.0f;
    if (s.holder == Holder::kHeld || s.holder == Holder::kBarrel)
        s.headShare = s.headShare - dt / kHeadEase > 0.0f ? s.headShare - dt / kHeadEase : 0.0f;
    else if (s.holder == Holder::kGlide)
        s.headShare = s.glideFromWorld ? 1.0f - u : 0.0f;

    // 6. The result.
    r.holder = s.holder;
    r.trigger = s.trigger;
    r.barrel = s.barrel;
    r.glide = u;
    r.glideFromWorld = s.holder == Holder::kGlide && s.glideFromWorld;
    r.headShare = s.headShare;
    r.armed[0] = s.armed[0], r.armed[1] = s.armed[1];
    r.fireHand = s.holder == Holder::kHeld && s.fireReset[s.trigger] ? s.trigger : -1;
    return r;
}

}  // namespace grasp

}  // namespace old_grasp

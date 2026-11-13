#pragma once
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
//   - A hand's fresh squeeze on a dropped gun of the game's (Frame::candidates) asks the game for it and
//     waits (State::pendingSide): meanwhile nothing is taken by either hand.
//   - The gun the game gives (Frame::arriveSide) flies into that hand by its pistol grip, from any state.
//   - The magnet (Frame::magnet; the user, 2026-11-12: "Make it very magnetic and snappy"): a gun on the ground answers a fresh squeeze
//     within pickReach as before, and past it within Frame::magnetSphere of its box when it lies at rest and the hand is settled
//     (HandIn::settled, from its crossing on); or, for a hand reaching down now (HandIn::reachDown) and settled from its crossing on, from the
//     column under it when it lies at rest. Anything in the sphere goes before anything found only in the column. A take
//     within pickReach glides in Frame::glide as before; any other a little longer the further it comes (Frame::snapPerMetre, at most
//     snapMost), and isn't let go of in mid-air: an open grip lets go once it's in the hand.

#include <cmath>
#include <cstdint>

#include "ammozone.h"

namespace grasp {

constexpr float kArm = 0.4f, kTake = 0.6f, kLetGo = 0.35f, kReachWindow = 0.4f, kRetakeAfter = 0.25f, kHeadEase = 0.25f;
constexpr int kLetGoFrames = 3;
constexpr int kMostCandidates = 8;
constexpr float kPendingMost = 3.0f;  // s: a take asked of the game is given up after this (a backstop: hands.cpp ends every take within 2 s itself)
constexpr float kMagnetStep = 0.05f;  // m between the samples along a hand's vertical line (the column)
constexpr float kDepthCost = 0.35f;   // in the column, a metre further down the line counts as this far off it
constexpr int kSeenForColumn = 4;     // hands.cpp: frames a hand has been seen with its own reading before it reaches down

enum class Holder : uint8_t { kHeld, kBarrel, kGlide, kLoose };

// A dropped gun of the game's that a free hand may take (hands.cpp fills them from pickups::Reachable, while the player's gun lies): its drawn pose
// (model -> room) and its box there.
struct Candidate {
    float room[12] = {};
    float boxMin[3] = {}, boxMax[3] = {};
};

struct HandIn {
    bool tracked = false, own = false, read = false;  // seen; a real controller (not the stand-in); its grip read this frame
    float squeeze = 0.0f, trigger = 0.0f;              // 0-1
    float pose[12] = {};                                // its grip pose (room; three rows: rotation | translation)
    bool busy = false, onSupport = false, forceTake = false;  // last frame: its hands full; on the handguard; a test's take
    // This frame's squeeze is a module's to take (hands.cpp: the hand on the seated magazine as pouch::OnSeated has it, or at the charging
    // handle as reload::NearHandle): with the rifle held by its handguard alone it doesn't regrip with it (a test's forceTake still does).
    bool claimed = false;
    // hands.cpp: the hand lowered past the gate under the neck, outside the belly, seen with its own reading these 4 frames (ReachesDown). The column
    // counts for a squeeze only when this holds now, and the hand was settled (below) from its crossing of kTake on (Frame::magnetLowFirst: reaching
    // down at the crossing itself).
    bool reachDown = false;
    // hands.cpp: seen with its own reading these 4 frames (not held through a moment out of sight, nor just back from a jump taken out). The magnet's
    // reach past pickReach counts only for a hand settled now and ever since its crossing; within pickReach nothing asks it. (Set by hands.cpp every
    // frame; true here so a test's hand is settled unless it says otherwise.)
    bool settled = true;
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
    // The game's own dropped guns (pickups.h). All at their defaults, every path is as before.
    Candidate candidates[kMostCandidates];
    int candidateCount = 0;
    bool refused = false;        // the take waiting is over with nothing for a hand (the game said no, or what it gave isn't a hand's): given up now
    bool swappedAway = false;    // the game swapped the gun on the ground away and nothing came to a hand: back in the right hand, as lost
    int arriveSide = -1;         // the game gave that hand's take: the gun flies into that hand ...
    float arriveFrom[12] = {};   // ... from there (model -> room)
    // The magnet (freehands.txt "magnet" and "snap", passed in; on in the game by default, off here so every path is as before).
    bool magnet = false;
    float magnetSphere = 0.35f;                     // m from the box all round (never less than pickReach)
    float magnetDown = 1.2f, magnetUp = 0.7f;       // m the column reaches below and above a hand reaching down
    float magnetWiden = 0.2f;                       // the column's radius: magnetSphere and this much more a metre down (or up)
    float snapPerMetre = 0.1f, snapMost = 0.25f;    // s: a magnet take's glide, Frame::glide and this a metre past pickReach, at most snapMost
    bool lyingResting = false;  // the lying gun settled (hands.cpp: at rest now or within the last second, or let go of a while ago): the column's
    bool arriveMagnet = false;  // the take the game gave (arriveSide) was won by the magnet: it flies in as a magnet take does (hands.cpp keeps it with the take)
    bool magnetLowFirst = false;  // freehands.txt "lowsqueeze 1": the column only for a squeeze that crossed kTake reaching down (0: one closed on the way down too)
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
    int pendingSide = -1;        // a take asked of the game by that hand, waiting (kLoose only): nothing is taken meanwhile by either hand
    float pendingFor = 0.0f;
    float glideFor = 0.0f;       // this glide's seconds (0: Frame::glide); over 0 a magnet take's: not let go of until it's in the hand
    bool crossLow[2] = {};       // its latest crossing of kTake was made reaching down (HandIn::reachDown then), the hand settled ever since
    bool crossSettled[2] = {};   // its latest crossing of kTake was made settled (HandIn::settled), and the hand has stayed settled since
    bool missWatch[2] = {};      // a squeeze reaching down in its window, nothing taken yet: at the end of its window the miss is said ...
    float missOff[2] = {1e9f, 1e9f}, missDepth[2] = {};  // ... with the least the lying gun lay off its line (m) and how far below then
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
    int reachedFor = -1, candidate = -1;  // this frame that hand's fresh squeeze took candidate i (hands.cpp asks the game); spent[that hand] too
    int pending = -1;                     // the hand whose take waits
    int gameGave = -1;                    // this frame the game's gun came into that hand (kGlide from the world)
    bool pendingLapsed = false;           // a take waited kPendingMost and was given up
    // The magnet. reach above stays the hand's distance to the box taken, whichever way it was reached.
    float glideFor = 0.0f;                // kGlide: this glide's seconds
    bool magnet = false;                  // this frame's take (pickedUp, reachedFor) was the magnet's: by the column, or from pickReach or further
    float reachDepth = 0.0f, reachOff = 0.0f;  // ... its box point: metres below the hand (above: under 0) and off its vertical line
    bool arrivedByMagnet = false;         // arrived (above) from a magnet take's glide
    int missed = -1;                      // a hand that squeezed reaching down got nothing in its window, the lying gun at rest: that side ...
    float missOff = 0.0f, missDepth = 0.0f;  // ... the least the gun lay off its line (m, the column's radius not asked) and how far below then
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

// The point of the box lo-hi (model) nearest the room point p, in the room, through the rigid pose room (model -> room).
inline void BoxNearest(const float room[12], const float lo[3], const float hi[3], const float p[3], float q[3]) {
    float m[3];
    ToModel(room, p, m);
    for (int i = 0; i < 3; ++i)
        m[i] = m[i] < lo[i] ? lo[i] : m[i] > hi[i] ? hi[i] : m[i];
    for (int i = 0; i < 3; ++i)
        q[i] = room[i * 4] * m[0] + room[i * 4 + 1] * m[1] + room[i * 4 + 2] * m[2] + room[i * 4 + 3];
}

// A hand reaching down (hands.cpp asks this for HandIn::reachDown): its grip point at least gate metres under the neck point and outside the belly's
// zone grown as pouch.cpp grows it for its own squeeze (ammozone::kPutBackMore), measured in the body's axes (right (cos, 0, -sin), up y, forward
// (-sin, 0, -cos): pouch::BodyFrame's). The chest, the shoulders and the work in front of the body are all above the gate; a zone below it would be
// one more exclusion here. False when a number isn't finite.
inline bool ReachesDown(const float hand[3], const float neck[3], float yaw, float gate) {
    if (!Finite(hand, 3) || !Finite(neck, 3) || !std::isfinite(yaw) || !std::isfinite(gate))
        return false;
    const float d[3] = {hand[0] - neck[0], hand[1] - neck[1], hand[2] - neck[2]};
    const float c = std::cos(yaw), s = std::sin(yaw);
    const float fromNeck[3] = {d[0] * c - d[2] * s, d[1], -d[0] * s - d[2] * c};
    return fromNeck[1] <= -gate && !ammozone::Inside(fromNeck, ammozone::kPutBackMore);
}

// The column under a hand at at (the room, y up): samples every kMagnetStep along its vertical line, from up above it to down below it, and at each
// the box point nearest it, measured from the hand: depth (metres below the hand, under 0 above it) and off (metres from its vertical line). A point
// answers when depth is within [-up, down] and off is under sphere + widen * |depth|; its rank is off + kDepthCost * |depth|. True with the least
// rank's numbers. least: the least off over every sample with its depth, whether it answers or not (a miss's numbers). False when none answers
// or a number isn't finite.
inline bool MagnetReach(const float at[3], const float room[12], const float lo[3], const float hi[3], float sphere, float down, float up, float widen,
                        float& rank, float& depth, float& off, float* leastOff = nullptr, float* leastDepth = nullptr) {
    if (!Finite(at, 3) || !Finite(room, 12) || !Finite(lo, 3) || !Finite(hi, 3))
        return false;
    const int above = static_cast<int>(std::ceil(up / kMagnetStep)), below = static_cast<int>(std::ceil(down / kMagnetStep));
    bool found = false;
    for (int j = -above; j <= below; ++j) {
        float along = static_cast<float>(j) * kMagnetStep;  // down the line from the hand
        along = along < -up ? -up : along > down ? down : along;
        const float p[3] = {at[0], at[1] - along, at[2]};
        float q[3];
        BoxNearest(room, lo, hi, p, q);
        const float dx = q[0] - at[0], dz = q[2] - at[2];
        const float dep = at[1] - q[1], o = std::sqrt(dx * dx + dz * dz);
        if (!std::isfinite(dep) || !std::isfinite(o))
            return false;
        if (leastOff && o < *leastOff) {
            *leastOff = o;
            if (leastDepth)
                *leastDepth = dep;
        }
        const float a = std::fabs(dep);
        if (dep < -up || dep > down || !(o < sphere + widen * a))
            continue;
        const float k = o + kDepthCost * a;
        if (!found || k < rank)
            rank = k, depth = dep, off = o, found = true;
    }
    return found;
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
    s.glideFor = 0.0f;
}

// ---- The update, once a frame ----

inline Result Update(const Frame& f, State& s) {
    Result r;
    // Sanitized: a pose with a number not finite is a hand not tracked; a grip not finite is 0 and not read
    // (it never lets go), a trigger not finite 0 and not read (it re-arms nothing).
    const float dt = std::isfinite(f.dt) ? (f.dt > 0.0f ? (f.dt < 0.1f ? f.dt : 0.1f) : 0.0f) : 0.0f;
    struct Hand {
        bool tracked, own, read, live, busy, onSupport, forceTake, triggerRead, claimed, reachDown, settled;
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
        o.reachDown = f.magnet && in.reachDown && o.tracked;
        o.settled = f.magnet && in.settled && o.tracked;
    }
    const bool haveGun = f.haveGun && Finite(f.gunRoom, 12);
    const bool lying = f.lying && Finite(f.lyingRoom, 12);
    const int candidates = f.candidateCount < 0 ? 0 : f.candidateCount > kMostCandidates ? kMostCandidates : f.candidateCount;
    const bool boxOk = Finite(f.boxMin, 3) && Finite(f.boxMax, 3);
    const bool gripOk[2] = {Finite(f.gripPoint[0], 3), Finite(f.gripPoint[1], 3)};
    const float glide = std::isfinite(f.glide) && f.glide > 0.0f ? f.glide : 0.12f;
    const float pickReach = std::isfinite(f.pickReach) && f.pickReach >= 0.0f ? f.pickReach : 0.25f;
    const float regripReach = std::isfinite(f.regripReach) && f.regripReach >= 0.0f ? f.regripReach : 0.15f;
    // The magnet's numbers (one not finite or under 0: its default; the column's at most 3 m each way; a glide's most never under Frame::glide).
    const auto metres = [](float v, float otherwise, float most) { return std::isfinite(v) && v >= 0.0f ? (v < most ? v : most) : otherwise; };
    const float sphereReach = metres(f.magnetSphere, 0.35f, 3.0f) > pickReach ? metres(f.magnetSphere, 0.35f, 3.0f) : pickReach;
    const float columnDown = metres(f.magnetDown, 1.2f, 3.0f), columnUp = metres(f.magnetUp, 0.7f, 3.0f);
    const float columnWiden = metres(f.magnetWiden, 0.2f, 3.0f);
    const float snapPerMetre = metres(f.snapPerMetre, 0.1f, 10.0f);
    const float snapMost = metres(f.snapMost, 0.25f, 10.0f) > glide ? metres(f.snapMost, 0.25f, 10.0f) : glide;
    // A magnet take's glide (s): Frame::glide and snapPerMetre for each metre the pistol grip comes from past pickReach, at most snapMost.
    const auto snap = [&](float pull) {
        const float more = std::isfinite(pull) && pull > pickReach ? pull - pickReach : 0.0f;
        const float t = glide + snapPerMetre * more;
        return t < snapMost ? t : snapMost;
    };
    // Metres from the hand at to side's pistol-grip point on the pose room (model -> room); not known: 0.
    const auto pullTo = [&](const float at[3], const float room[12], int side) {
        if (!gripOk[side] || !Finite(room, 12))
            return 0.0f;
        const float* g = f.gripPoint[side];
        float d2 = 0.0f;
        for (int i = 0; i < 3; ++i) {
            const float w = room[i * 4] * g[0] + room[i * 4 + 1] * g[1] + room[i * 4 + 2] * g[2] + room[i * 4 + 3] - at[i];
            d2 += w * w;
        }
        return std::sqrt(d2);
    };
    const bool holdOrder = (s.holder == Holder::kHeld && (s.trigger == 0 || s.trigger == 1) && s.barrel == -1) ||
                           (s.holder == Holder::kBarrel && s.trigger == -1 && (s.barrel == 0 || s.barrel == 1)) ||
                           (s.holder == Holder::kGlide && (s.trigger == 0 || s.trigger == 1) &&
                            (s.barrel == -1 || s.barrel == 1 - s.trigger)) ||
                           (s.holder == Holder::kLoose && s.trigger == -1 && s.barrel == -1);
    const bool inOrder = holdOrder && (s.pendingSide == -1 || ((s.pendingSide == 0 || s.pendingSide == 1) && s.holder == Holder::kLoose));
    s.clock += dt;

    // 1. Each hand's squeeze and trigger.
    bool fresh[2];
    for (int i = 0; i < 2; ++i) {
        const Hand& o = h[i];
        if (o.own && o.read && o.squeeze < kArm)
            s.armTake[i] = true;
        if (o.live && s.armTake[i] && o.squeeze >= kTake) {
            s.crossedAt[i] = s.clock, s.armTake[i] = false;
            // (Settled at the crossing: a squeeze made by a hand held through a moment out of sight, or only just seen again, never takes by the
            // column nor from past pickReach. With Frame::magnetLowFirst, reaching down at the crossing too.)
            s.crossLow[i] = o.reachDown;
            s.crossSettled[i] = o.settled;
            s.missWatch[i] = o.reachDown, s.missOff[i] = 1e9f, s.missDepth[i] = 0.0f;
        }
        if (!o.settled)  // (out of sight or eased back from a jump since its crossing: that squeeze no longer reaches past pickReach)
            s.crossLow[i] = s.crossSettled[i] = false;
        fresh[i] = o.forceTake || (o.live && !o.busy && s.crossedAt[i] >= 0.0f && s.clock - s.crossedAt[i] <= kReachWindow &&
                                   o.squeeze >= kTake && s.clock - s.letGoAt[i] > kRetakeAfter);
        if (o.triggerRead && o.trigger < kArm)
            s.fireReset[i] = true;
    }

    const auto take = [&s, &r, &h](int side) {
        s.armed[side] = true;
        s.low[side] = 0;
        s.fireReset[side] = h[side].triggerRead && h[side].trigger < kArm;
        s.crossedAt[side] = -1.0f;
        s.missWatch[side] = false;
        r.spent[side] = true;
    };

    // 2a. The game gave a hand's take (a dropped gun taken off the ground): it flies into that hand from where it lay, whatever held the gun before
    // (that one went into the game's pickup, or back to its slot). Not in a scripted moment, a new life or while dormant: hands.cpp gives it again.
    if ((f.arriveSide == 0 || f.arriveSide == 1) && f.enabled && !f.frozen && !f.newLife && Finite(f.arriveFrom, 12)) {
        const int a = f.arriveSide;
        Clear(s);
        s.holder = Holder::kGlide, s.trigger = a, s.barrel = -1;
        s.glideTime = 0.0f, s.glideFromWorld = true, s.headShare = 1.0f;
        if (f.magnet && f.arriveMagnet)  // (won by the magnet: as long as its pull from where it lay; a hand not seen, Frame::glide)
            s.glideFor = h[a].tracked ? snap(pullTo(h[a].at, f.arriveFrom, a)) : glide;
        take(a);
        r.gameGave = a;
    }

    // 2. Back in the right hand, as at a spawn.
    if (r.gameGave < 0 && (f.newLife || f.lost || f.swappedAway || !f.enabled || !inOrder)) {
        const bool changed = !(s.holder == Holder::kHeld && s.trigger == 1);
        Clear(s);
        r.returned = changed;
        r.why = f.newLife ? "a new life"
                : f.lost  ? "the ground lost it"
                : f.swappedAway ? "the game swapped it"
                : !f.enabled ? "another gun, seated or free hands off"
                             : "its hold out of order";
        r.holder = s.holder, r.trigger = s.trigger, r.barrel = s.barrel;
        r.fireHand = s.fireReset[s.trigger] ? s.trigger : -1;
        return r;
    }

    // 3 and 4 wait for the frame after an arrival: on its frame nothing is armed twice and nothing lets go.
    if (r.gameGave < 0) {
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
                    s.glideTime = 0.0f, s.glideFromWorld = false, s.glideFor = 0.0f;
                    take(g);
                    r.regrip = g;
                    r.reach = d;
                }
            }
            break;
        }
        case Holder::kGlide:
            // (A magnet take isn't let go of in mid-air: its open grip lets go the frame after it's in the hand, the frames under kLetGo counted.)
            if (letGoNow && !(s.glideFor > 0.0f && s.glideFromWorld)) {
                // From a regrip the barrel hand still holds the handguard: the rifle stays in that hold.
                letGo(s.trigger, !s.glideFromWorld);
            } else {
                s.glideTime += dt;
                if (s.glideTime >= (s.glideFor > 0.0f ? s.glideFor : glide)) {
                    s.holder = Holder::kHeld, s.barrel = -1;
                    r.arrived = s.trigger;
                    r.arrivedByMagnet = s.glideFor > 0.0f && s.glideFromWorld;
                }
            }
            break;
        case Holder::kLoose:
            s.headShare = s.headShare + dt / kHeadEase < 1.0f ? s.headShare + dt / kHeadEase : 1.0f;
            if (s.pendingSide >= 0) {
                s.pendingFor += dt;
                if (f.refused || s.pendingFor > kPendingMost) {
                    r.pendingLapsed = !f.refused;
                    s.pendingSide = -1, s.pendingFor = 0.0f;
                }
            } else if (!f.frozen && (lying || candidates > 0)) {
                // Metres from p to the middle of the box lo-hi: two things whose boxes both hold the hand go to the nearer middle.
                const auto toMiddle = [](const float p[3], const float lo[3], const float hi[3]) {
                    float sum = 0.0f;
                    for (int i = 0; i < 3; ++i) {
                        const float d = p[i] - (lo[i] + hi[i]) * 0.5f;
                        sum += d * d;
                    }
                    return std::sqrt(sum);
                };
                int best = -1, bestThing = -1;          // bestThing: -1 the lying gun, else the candidate
                float bestD = 0.0f, bestMiddle = 0.0f;  // to its box; to its box's middle
                for (int i = 1; !f.magnet && i >= 0; --i) {  // the right first: a tie goes to it (the magnet's: below)
                    if (!fresh[i])
                        continue;
                    // Each thing in turn: the lying gun first, then candidates 0, 1, ... A forced hand takes the lying gun (it is 0 from it wherever
                    // it lies); with nothing lying it is measured to a candidate by its real distance (a candidate is never forced).
                    for (int k = lying ? -1 : 0; k < candidates; ++k) {
                        float d = 1e9f, middle = 1e9f;
                        float p[3];
                        if (k < 0) {
                            const bool seen = h[i].tracked && boxOk;
                            if (seen)
                                ToModel(f.lyingRoom, h[i].at, p);
                            if (h[i].forceTake)
                                d = 0.0f;
                            else if (seen)
                                d = BoxDistance(p, f.boxMin, f.boxMax);
                            if (seen)
                                middle = toMiddle(p, f.boxMin, f.boxMax);
                        } else {
                            const Candidate& c = f.candidates[k];
                            if (!h[i].tracked || (h[i].forceTake && lying) || !Finite(c.room, 12) || !Finite(c.boxMin, 3) || !Finite(c.boxMax, 3))
                                continue;
                            ToModel(c.room, h[i].at, p);
                            d = BoxDistance(p, c.boxMin, c.boxMax);
                            middle = toMiddle(p, c.boxMin, c.boxMax);
                        }
                        if (d < pickReach && (best < 0 || d < bestD || (k != bestThing && d == 0.0f && bestD == 0.0f && middle < bestMiddle)))
                            best = i, bestThing = k, bestD = d, bestMiddle = middle;
                    }
                }
                // The magnet: the same hands and things in the same order, each answering within pickReach of its box, or past it within sphereReach
                // (rank: that distance), or else from the column under a hand reaching down now and settled from its crossing on (both past
                // pickReach: the lying gun only at rest; the game's always). Anything
                // in the sphere goes before anything only in the column; within each the least rank; ties as above.
                bool bestColumn = false;
                float bestRank = 0.0f, bestDepth = 0.0f, bestOff = 0.0f;
                for (int i = 1; f.magnet && i >= 0; --i) {
                    if (!fresh[i])
                        continue;
                    const bool column = h[i].reachDown && (f.magnetLowFirst ? s.crossLow[i] : s.crossSettled[i]) && !h[i].forceTake;
                    const bool past = h[i].settled && s.crossSettled[i];  // (the sphere past pickReach: settled from its crossing on)
                    for (int k = lying ? -1 : 0; k < candidates; ++k) {
                        float d = 1e9f, middle = 1e9f;
                        float p[3];
                        const float *room = nullptr, *lo = nullptr, *hi = nullptr;
                        if (k < 0) {
                            const bool seen = h[i].tracked && boxOk;
                            if (seen)
                                ToModel(f.lyingRoom, h[i].at, p), room = f.lyingRoom, lo = f.boxMin, hi = f.boxMax;
                            if (h[i].forceTake)
                                d = 0.0f;
                            else if (seen)
                                d = BoxDistance(p, f.boxMin, f.boxMax);
                            if (seen)
                                middle = toMiddle(p, f.boxMin, f.boxMax);
                        } else {
                            const Candidate& c = f.candidates[k];
                            if (!h[i].tracked || (h[i].forceTake && lying) || !Finite(c.room, 12) || !Finite(c.boxMin, 3) || !Finite(c.boxMax, 3))
                                continue;
                            ToModel(c.room, h[i].at, p);
                            room = c.room, lo = c.boxMin, hi = c.boxMax;
                            d = BoxDistance(p, c.boxMin, c.boxMax);
                            middle = toMiddle(p, c.boxMin, c.boxMax);
                        }
                        // (Past pickReach only at rest and settled: a catch in mid-air, or a hand held where it was last seen, stays within pickReach.)
                        const bool inSphere = d < pickReach || (d < sphereReach && past && (k >= 0 || f.lyingResting));
                        float rank = d, depth = 0.0f, off = 0.0f;
                        if (!inSphere && !(column && room && (k >= 0 || f.lyingResting) &&
                                           MagnetReach(h[i].at, room, lo, hi, sphereReach, columnDown, columnUp, columnWiden, rank, depth, off)))
                            continue;
                        if (inSphere && room) {  // (its box point nearest the hand, for the log)
                            float q[3];
                            BoxNearest(room, lo, hi, h[i].at, q);
                            depth = h[i].at[1] - q[1];
                            off = std::sqrt((q[0] - h[i].at[0]) * (q[0] - h[i].at[0]) + (q[2] - h[i].at[2]) * (q[2] - h[i].at[2]));
                        }
                        const bool better = best < 0 || (bestColumn && inSphere) ||
                                            (bestColumn == !inSphere &&
                                             (rank < bestRank || (inSphere && k != bestThing && d == 0.0f && bestD == 0.0f && middle < bestMiddle)));
                        if (better)
                            best = i, bestThing = k, bestD = d, bestMiddle = middle, bestColumn = !inSphere, bestRank = rank, bestDepth = depth,
                            bestOff = off;
                    }
                }
                if (best >= 0 && f.magnet) {
                    r.magnet = bestColumn || bestD >= pickReach;
                    r.reachDepth = bestDepth, r.reachOff = bestOff;
                }
                if (best >= 0 && bestThing < 0) {
                    s.holder = Holder::kGlide, s.trigger = best, s.barrel = -1;
                    s.glideTime = 0.0f, s.glideFromWorld = true;
                    s.glideFor = r.magnet ? snap(pullTo(h[best].at, f.lyingRoom, best)) : 0.0f;
                    take(best);
                    r.pickedUp = best;
                    r.reach = bestD;
                } else if (best >= 0) {
                    // A dropped gun of the game's: hands.cpp asks the game for it. Nothing is in the hand yet, so nothing is armed.
                    s.pendingSide = best, s.pendingFor = 0.0f, s.crossedAt[best] = -1.0f, s.missWatch[best] = false;
                    r.spent[best] = true;
                    r.reachedFor = best, r.candidate = bestThing, r.reach = bestD;
                }
            }
            // A hand that squeezed reaching down: while its window runs, how near the lying gun at rest came to its line; nothing taken by the
            // window's end, said once (r.missed).
            for (int i = 1; i >= 0; --i) {
                // (A squeeze closed on the way down, the hand settled since: watched from the first frame it reaches down within its window.)
                if (!s.missWatch[i] && !f.magnetLowFirst && f.magnet && fresh[i] && !h[i].forceTake && h[i].reachDown && s.crossSettled[i] &&
                    s.holder == Holder::kLoose && s.pendingSide < 0)
                    s.missWatch[i] = true, s.missOff[i] = 1e9f, s.missDepth[i] = 0.0f;
                if (!s.missWatch[i])
                    continue;
                if (s.holder == Holder::kLoose && s.pendingSide < 0 && fresh[i] && lying && f.lyingResting && boxOk && h[i].tracked) {
                    float rank = 0.0f, depth = 0.0f, off = 0.0f;
                    MagnetReach(h[i].at, f.lyingRoom, f.boxMin, f.boxMax, sphereReach, columnDown, columnUp, columnWiden, rank, depth, off,
                                &s.missOff[i], &s.missDepth[i]);
                }
                if (s.holder != Holder::kLoose || s.pendingSide >= 0) {
                    s.missWatch[i] = false;
                } else if (!(s.crossedAt[i] >= 0.0f && s.clock - s.crossedAt[i] <= kReachWindow)) {
                    if (s.missOff[i] < 1e8f && r.missed < 0)
                        r.missed = i, r.missOff = s.missOff[i], r.missDepth = s.missDepth[i];
                    s.missWatch[i] = false;
                }
            }
            break;
        }
    }

    // 5. The game's aim: to the head while it lies, back to the gun as it's held.
    const float glideSeconds = s.glideFor > 0.0f ? s.glideFor : glide;
    const float u = s.holder == Holder::kGlide ? Smooth(s.glideTime / glideSeconds) : 1.0f;
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
    r.pending = s.pendingSide;
    r.glideFor = s.holder == Holder::kGlide ? glideSeconds : 0.0f;
    return r;
}

}  // namespace grasp

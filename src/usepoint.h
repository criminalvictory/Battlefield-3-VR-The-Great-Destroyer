#pragma once
// A hand's use of a scripted prompt: a point of the world the game shows a prompt at (pickups.h's UsePoint, the game's ServerInteractionEntity:
// Operation Swordbreaker's "pick up the RPG" on the hotel roof, where Matkovic lets the AT4 fall), used by reaching for it and holding the grip (the
// user, 2026-11-18: "hover his hand by the door to where it or where the prompt shows up, and he just has to hold the grip button"). Generic for every
// such prompt. Pure, header only: hands.cpp runs Update once a frame and makes the use (the game's own, else the prompt's key); tools/at4test.cpp
// checks it without the game.
//   - Only while the game itself targets the point (its own prompt is up: the soldier within its radius and angle), the point on, not used up and no
//     flag (Frame::point.on), free hands awake and no scripted moment.
//   - The reach (Reach): within kDisc across of the point, from kBelow under it to kAbove over it, kWiden wider each metre up: a hand lowered
//     toward a point on the floor, prone, crouched or stooping (the magnet's lowered-hand column, grasp.h, about a point instead of a gun's box). Once
//     where the thing lies is measured (the AT4's on the roof: the runbook's R4), the capsule round it, kCapsuleGrow wider, instead.
//   - A tick on entering the reach (the holsters' own: 0.25 for 20 ms), once until that hand has left it.
//   - The use: an empty hand (holding nothing: no gun by its grip or its front, no magazine, round or case, no module's), a fresh squeeze (crossing
//     kSqueezeOn while in reach, not one carried into it), held kHold seconds in reach. Asked once a squeeze (UseOut::use); again only after a let-go.
//   - No gun in either hand (the AT4's rule, 2026-11-18, as the user's of 2026-11-16 for a draw at a holster: "they have to holster their primary
//     first"): a squeeze there with a gun in a hand is refused, the holsters' double buzz (0.5 for 15 ms, the second 60 ms on) and its line, once a
//     squeeze. The player holsters or drops the rifle first, then reaches.
//   - The squeeze begun in reach is the use's (spent: hands.cpp tells the grasp the hand is busy) till it's let go, refused or not: a lying gun's
//     take, the magnet's or a regrip never comes of it.
#include <cmath>
#include <cstdint>

namespace usepoint {

constexpr float kDisc = 0.5f;         // m across from the point
constexpr float kBelow = 0.3f;        // m under it (a point a little above the floor, a hand on the floor)
constexpr float kAbove = 1.2f;        // m over it (standing, a hand lowered to the knees)
constexpr float kWiden = 0.2f;        // m wider for each metre up
constexpr float kCapsuleGrow = 0.15f; // m round a measured capsule
constexpr float kSqueezeOn = 0.6f, kSqueezeOff = 0.4f;  // the grip: squeezed past, let go under (the grasp's own)
constexpr float kHold = 0.25f;        // s the squeeze is held in reach
constexpr float kTickAmp = 0.25f, kTickSeconds = 0.02f;                          // holster.h's tick
constexpr float kDoubleAmp = 0.5f, kDoubleSeconds = 0.015f, kDoubleGap = 0.06f;  // holster.h's refusal, the second pulse kDoubleGap on

struct Point {
    bool on = false;           // the game targets it, it's on, not used up, not a flag
    float at[3] = {};          // where it is (the room: y up)
    bool capsule = false;      // where the thing lies is measured: the segment a..b of radius r (the room)
    float a[3] = {}, b[3] = {};
    float radius = 0.0f;
};
struct Hand {
    bool tracked = false;      // seen (its pose read this frame)
    float at[3] = {};          // its grip point (the room)
    float squeeze = 0.0f;      // 0-1
    bool empty = false;        // holding nothing (no gun by its grip or front, nothing carried, no module's)
};
struct Frame {
    float dt = 0.0f;
    Point point;
    Hand hand[2];
    bool gunInHand = false;    // a gun in either hand (by its pistol grip, by its front alone, flying into one)
    bool asleep = false;       // free hands asleep, a scripted moment: nothing
};
enum class Why : uint8_t { kNone, kGunInHand };
struct HandState {
    bool inReach = false;      // in reach last frame (its tick said)
    bool down = false;         // the squeeze past kSqueezeOn (till under kSqueezeOff)
    bool owned = false;        // that squeeze is the use's, begun in reach: spent till let go
    bool asked = false;        // ... and its use asked or refused already
    float held = 0.0f;         // s it has been held in reach
};
struct State {
    HandState hand[2];
    float clock = 0.0f;
    float pulseAt[2] = {-1.0f, -1.0f};  // the refusal's second pulse, when it's due (-1 none)
};
struct Pulse {
    float amplitude = 0.0f, seconds = 0.0f;
};
struct Out {
    int use = -1;              // the hand whose use is asked now (-1 none)
    int refused = -1;          // the hand whose use is refused now (-1 none) ...
    Why why = Why::kNone;      // ... why
    bool entered[2] = {};      // that hand came into reach this frame (the tick)
    bool spent[2] = {};        // that hand's squeeze is the use's this frame (the grasp is told the hand is busy)
    Pulse pulse[2];            // to send now
};

inline float Dot3(const float* a, const float* b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

// A hand at `at` (the room) is in the point's reach. Not finite: never.
inline bool Reach(const Point& p, const float at[3]) {
    for (int i = 0; i < 3; ++i)
        if (!std::isfinite(at[i]) || !std::isfinite(p.at[i]))
            return false;
    if (p.capsule) {
        float ab[3], ap[3];
        for (int i = 0; i < 3; ++i)
            ab[i] = p.b[i] - p.a[i], ap[i] = at[i] - p.a[i];
        const float length = Dot3(ab, ab);
        float u = length > 1e-8f ? Dot3(ap, ab) / length : 0.0f;
        u = u < 0.0f ? 0.0f : u > 1.0f ? 1.0f : u;
        float d = 0.0f;
        for (int i = 0; i < 3; ++i) {
            const float c = ap[i] - u * ab[i];
            d += c * c;
        }
        return std::isfinite(p.radius) && std::sqrt(d) <= p.radius + kCapsuleGrow;
    }
    const float up = at[1] - p.at[1];
    if (up < -kBelow || up > kAbove)
        return false;
    const float dx = at[0] - p.at[0], dz = at[2] - p.at[2];
    return std::sqrt(dx * dx + dz * dz) <= kDisc + kWiden * (up > 0.0f ? up : 0.0f);
}

inline void Clear(State& s) { s = State{}; }

// Once a frame (hands.cpp's FreeHands, before the grasp: its spent hands are told the grasp as busy).
inline void Update(const Frame& f, State& s, Out& out) {
    out = Out{};
    const float dt = std::isfinite(f.dt) && f.dt > 0.0f && f.dt < 0.25f ? f.dt : 0.0f;
    s.clock += dt;
    for (int side = 0; side < 2; ++side) {
        HandState& h = s.hand[side];
        const Hand& in = f.hand[side];
        // The refusal's second pulse, whatever else.
        if (s.pulseAt[side] >= 0.0f && s.clock >= s.pulseAt[side]) {
            out.pulse[side] = Pulse{kDoubleAmp, kDoubleSeconds};
            s.pulseAt[side] = -1.0f;
        }
        const float squeeze = in.tracked && std::isfinite(in.squeeze) ? in.squeeze : 0.0f;
        const bool wasDown = h.down;
        h.down = h.down ? squeeze >= kSqueezeOff : squeeze > kSqueezeOn;
        const bool fresh = h.down && !wasDown;
        if (!h.down)
            h.owned = h.asked = false, h.held = 0.0f;
        const bool live = !f.asleep && f.point.on && in.tracked;
        const bool reach = live && Reach(f.point, in.at);
        // The tick: into reach with an empty hand (a hand holding something isn't reaching for it).
        if (reach && !h.inReach && in.empty) {
            out.entered[side] = true;
            if (out.pulse[side].seconds <= 0.0f)
                out.pulse[side] = Pulse{kTickAmp, kTickSeconds};
        }
        h.inReach = reach && in.empty;
        // A fresh squeeze in reach by an empty hand is the use's till it's let go.
        if (fresh && reach && in.empty)
            h.owned = true;
        if (!h.owned)
            continue;
        out.spent[side] = true;
        if (h.asked)
            continue;
        if (f.gunInHand) {
            h.asked = true;
            out.refused = side, out.why = Why::kGunInHand;
            out.pulse[side] = Pulse{kDoubleAmp, kDoubleSeconds};
            s.pulseAt[side] = s.clock + kDoubleGap;
            continue;
        }
        h.held = reach ? h.held + dt : 0.0f;  // (out of reach: the hold starts again once it's back, the squeeze still the use's)
        if (h.held >= kHold && out.use < 0) {
            h.asked = true;
            out.use = side;
        }
    }
}

}  // namespace usepoint

#pragma once
// A hand's use of a scripted prompt: a point of the world the game shows a prompt at (pickups.h's UsePoint, the game's ServerInteractionEntity:
// Operation Swordbreaker's "pick up the RPG" on the hotel roof, where Matkovic lets the AT4 fall; a door to breach), used by reaching for it and
// holding the grip (the user, 2026-11-18: "hover his hand by the door to where it or where the prompt shows up, and he just has to hold the grip
// button"). Pure, header only: hands.cpp runs Update once a frame and makes the use (the game's own, else the prompt's key); tools/at4test.cpp checks
// it without the game. Not the game's own pickups (a ServerPickupEntity: the train's M1014, a dead soldier's gun): those are pickups.h's takes.
//   - Only a prompt a hand uses (ByHand): on the interact key (EIAInteract: doors, breaches, climbing out, a rappel, a mortar, a seat) or the reload
//     key (EIAReload: the game's own pickup key, the AT4's), a press (HoldToInteractTime 0: one call of the game's use finishes it, where a hold's user
//     idles out after 0.4 s with its use counted), never a flag, the game's own reach for it kMostRadius at most. The campaign's 75 use points
//     (Venice-EBX, 2026-11-18): 38 on the interact key and the AT4's on the reload key, every one a press; its pose prompts (EIAProne, EIAChangePose,
//     EIAClutch, four of them holds of 1 s: the AT4's roof has two, 100 m round, 8 m off), jumps, fires, weapon picks and the jet's and the tank's
//     300 m prompts are never a hand's (their own buttons, or the quick-time events').
//   - Only while the game itself targets the point (its own prompt is up: the soldier within its radius and angle), the point on, not used up
//     (Frame::point.on), free hands awake and no scripted moment.
//   - The reach (Reach): within the point's disc (Point::disc, kDisc unless the prompt has its own: the AT4's, its tube lying 0.65 m off the point)
//     across, from kBelow under it to kAbove over it, kWiden wider each metre up: a hand lowered toward a point on the floor, prone, crouched or
//     stooping. Once where the thing lies is measured, the capsule round it, kCapsuleGrow wider, instead.
//   - A tick on entering the reach (the holsters' own: 0.25 for 20 ms), once until that hand has left it by kLeaveMore past it.
//   - The use: an empty hand (holding nothing: no gun by its grip or its front, no magazine, round or case, no module's), a fresh squeeze (crossing
//     kSqueezeOn while in reach, not one carried into it) that nothing else of the hands' wants (Hand::yield, hands.cpp: the hand at one of the body's
//     places, the holsters', the pouch's, the chests'; near a gun in a hand, its front, its magazine or its handle; on a gun lying within the grasp's
//     own reach), held kHold seconds in reach. Asked once a squeeze (Out::use); again only after a let-go.
//   - A gun in the other hand is no bar: the free hand opens a door with the rifle in the right hand, and takes the AT4 with it there too (the scene's
//     kit takes the rifle, as the game's own key does; it gives it back after the shot).
//   - The squeeze begun in reach is the use's (spent: hands.cpp tells the grasp the hand is busy) till it's let go: a lying gun's take, the magnet's or
//     a regrip never comes of it. If the hand takes something else before its use is asked (it isn't empty any more: a magazine, a grenade, the gun's
//     front), its use is given up (Out::given).
#include <cmath>
#include <cstdint>

namespace usepoint {

constexpr float kDisc = 0.5f;         // m across from the point
constexpr float kBelow = 0.3f;        // m under it (a point a little above the floor, a hand on the floor)
constexpr float kAbove = 1.2f;        // m over it (standing, a hand lowered to the knees)
constexpr float kWiden = 0.2f;        // m wider for each metre up
constexpr float kCapsuleGrow = 0.15f; // m round a measured capsule
constexpr float kLeaveMore = 0.05f;   // m past the reach before a hand in it has left it (its tick isn't said again and again at the edge)
constexpr float kSqueezeOn = 0.6f, kSqueezeOff = 0.4f;  // the grip: squeezed past, let go under (the grasp's own)
constexpr float kHold = 0.25f;        // s the squeeze is held in reach
constexpr float kTickAmp = 0.25f, kTickSeconds = 0.02f;  // holster.h's tick
// The prompts a hand uses (ByHand): the game's own enum (EntryInputActionEnum, read in the image: pickups.h's kUseReload, kUseInteract), its flag kind.
constexpr int kActionReload = 15, kActionInteract = 35;
constexpr int kFlagKind = 3;          // IET_CTFFlag: its own path (a ray test, no key)
constexpr float kMostRadius = 3.0f;   // m: the game's own UseWithinRadius for it at most (the campaign's interact prompts' 0.75-3; the jet's 300)

// A prompt a hand may use: its key's action, its kind, its HoldToInteractTime (s) and its UseWithinRadius (m), as pickups.h's UsePoint has them.
inline bool ByHand(int action, int kind, float hold, float radius) {
    return (action == kActionReload || action == kActionInteract) && kind != kFlagKind && hold == 0.0f && std::isfinite(radius) && radius > 0.0f &&
           radius <= kMostRadius;
}

struct Point {
    bool on = false;           // the game targets it, it's on, not used up, a prompt a hand uses (ByHand)
    float at[3] = {};          // where it is (the room: y up)
    float disc = kDisc;        // m across: its own (the AT4's 1.0), else kDisc
    bool capsule = false;      // where the thing lies is measured: the segment a..b of radius r (the room)
    float a[3] = {}, b[3] = {};
    float radius = 0.0f;
};
struct Hand {
    bool tracked = false;      // seen (its pose read this frame)
    float at[3] = {};          // its grip point (the room)
    float squeeze = 0.0f;      // 0-1
    bool empty = false;        // holding nothing (no gun by its grip or front, nothing carried, no module's)
    bool yield = false;        // its squeeze is something else's of the hands' (hands.cpp: a body's place, a gun in a hand near, a lying gun in reach)
};
struct Frame {
    float dt = 0.0f;
    Point point;
    Hand hand[2];
    bool asleep = false;       // free hands asleep, a scripted moment: nothing
};
struct HandState {
    bool inReach = false;      // in reach last frame (its tick said)
    bool down = false;         // the squeeze past kSqueezeOn (till under kSqueezeOff)
    bool owned = false;        // that squeeze is the use's, begun in reach: spent till let go
    bool asked = false;        // ... and its use asked or given up already
    float held = 0.0f;         // s it has been held in reach
};
struct State {
    HandState hand[2];
    float clock = 0.0f;
};
struct Pulse {
    float amplitude = 0.0f, seconds = 0.0f;
};
struct Out {
    int use = -1;              // the hand whose use is asked now (-1 none)
    bool entered[2] = {};      // that hand came into reach this frame (the tick)
    bool spent[2] = {};        // that hand's squeeze is the use's this frame (the grasp is told the hand is busy)
    bool given[2] = {};        // that hand's squeeze, the use's, took something else before its use was asked: given up this frame
    Pulse pulse[2];            // to send now
};

inline float Dot3(const float* a, const float* b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

// A hand at `at` (the room) is in the point's reach, more metres wider all round (kLeaveMore: still in it). Not finite: never.
inline bool Reach(const Point& p, const float at[3], float more = 0.0f) {
    for (int i = 0; i < 3; ++i)
        if (!std::isfinite(at[i]) || !std::isfinite(p.at[i]))
            return false;
    if (!std::isfinite(more) || more < 0.0f)
        more = 0.0f;
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
        return std::isfinite(p.radius) && std::sqrt(d) <= p.radius + kCapsuleGrow + more;
    }
    const float up = at[1] - p.at[1];
    if (up < -kBelow - more || up > kAbove + more)
        return false;
    const float disc = std::isfinite(p.disc) && p.disc > 0.0f ? p.disc : kDisc;
    const float dx = at[0] - p.at[0], dz = at[2] - p.at[2];
    return std::sqrt(dx * dx + dz * dz) <= disc + kWiden * (up > 0.0f ? up : 0.0f) + more;
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
        const float squeeze = in.tracked && std::isfinite(in.squeeze) ? in.squeeze : 0.0f;
        const bool wasDown = h.down;
        h.down = h.down ? squeeze >= kSqueezeOff : squeeze > kSqueezeOn;
        const bool fresh = h.down && !wasDown;
        if (!h.down)
            h.owned = h.asked = false, h.held = 0.0f;
        const bool live = !f.asleep && f.point.on && in.tracked;
        const bool reach = live && Reach(f.point, in.at);
        // A hand free for it: holding nothing, its squeeze nothing else's. The tick: into reach (a hand holding something, or at something else of the
        // hands', isn't reaching for it); out of it only kLeaveMore past it.
        const bool free = in.empty && !in.yield;
        if (reach && !h.inReach && free) {
            out.entered[side] = true;
            out.pulse[side] = Pulse{kTickAmp, kTickSeconds};
        }
        h.inReach = free && (h.inReach ? live && Reach(f.point, in.at, kLeaveMore) : reach);
        // A fresh squeeze in reach by a free hand is the use's till it's let go.
        if (fresh && reach && free)
            h.owned = true;
        if (!h.owned)
            continue;
        out.spent[side] = true;
        if (h.asked)
            continue;
        if (!in.empty) {  // (it took something else first: a magazine, a grenade, the gun's front, a gun given to it)
            h.asked = true;
            out.given[side] = true;
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

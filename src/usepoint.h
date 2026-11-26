#pragma once
// A hand's use of a scripted prompt: a point of the world the game shows a prompt at (pickups.h's UsePoint, the game's ServerInteractionEntity:
// Operation Swordbreaker's "pick up the RPG" on the hotel roof, where Matkovic lets the AT4 fall; a door to breach; the cellar's wire), used by holding
// the grip. Pure, header only: hands.cpp runs Update once a frame and makes the use (the game's own, else the prompt's key); tools/at4test.cpp checks
// it without the game. Not the game's own pickups (a ServerPickupEntity: the train's M1014, a dead soldier's gun): those are pickups.h's takes.
//   - Only a prompt a hand uses (ByHand): on the interact key (EIAInteract: doors, breaches, climbing out, a rappel, a mortar, a seat, the HALO jump's
//     start) or the reload key (EIAReload: the game's own pickup key, the AT4's), a press (HoldToInteractTime 0: one call of the game's use finishes
//     it, as one press of its key does; a hold's user idles out after 0.4 s with its use counted), never a flag, the game's own reach for it kMostRadius
//     at most. The campaign's 41 use points on E and R (Venice-EBX, 2026-11-25): every one a press; its pose prompts (EIAProne, EIAChangePose, EIAClutch,
//     holds of 1 s: the AT4's roof has two, 100 m round), jumps, fires, weapon picks and the jet's and the tank's 50-300 m prompts are never a hand's
//     (their own buttons, or the quick-time events').
//   - Frame::anywhere (the default; the owner, 2026-11-25: "holding grip down if you needed to open a door and it said hold R or E to open. I want to
//     be able to just hold the grip button instead"): while the game shows the prompt (it targets the point: the soldier within its radius and angle;
//     the point on, not used up: Point::on), holding the grip of a free hand uses it, wherever the hand is. Nothing in a new life's first frame, a weapon
//     switch's bridging frames or a fist fight's run (Frame::asleep); a scripted moment, or free hands asleep (another gun, the train pistol), no bar.
//     A prompt that goes unseen kGapMost at most (the game's target rescanned, a frame asleep) is still the one showing: a hold under way is kept.
//     A grip not read (its controller unseen a moment, the headset off) keeps its state and counts nothing, never a let-go; read again squeezed (or
//     squeezed as the state begins, after a new life) it is no fresh squeeze: let go and squeeze again.
//       - The hand free: holding nothing (no gun by its grip or its front, no magazine, round or case, no module's: Hand::empty) and its squeeze nothing
//         else's of the hands' (Hand::yield, hands.cpp's UseYields: a holster's place, a draw or a put-away; the belly and the chests with a gun in a
//         hand (BodyKeeps: a magazine, a round, a rocket); near a gun in a hand, its front, its magazine or its handle; on a gun lying nearer than the
//         prompt's target (LyingFirst)). One owner per squeeze.
//       - Its squeeze the prompt's: a fresh one (reaching kSqueezeOn) while the prompt shows; or one held from before, begun free (not at a body's place
//         or near the gun: HandState::beganFree) and free now, once the prompt has shown kHeldInto (a hand brushed past by a prompt never uses it at
//         once). Held kHold seconds while it shows, its use is asked (Out::use), with a tick on that hand (Out::entered: holster.h's 0.25 for 20 ms;
//         the line hands.cpp says with it is the word for a player whose buzz is off). One use a squeeze: again only after a let-go.
//       - A use not made for a moment (Transient: the game didn't target it at its next tick, too late, the soldier not awake, not answered in time)
//         is asked again kRetryAfter on while the squeeze is held and the prompt shows (Rearm: hands.cpp's NoteUse), kMostRetries a squeeze at most.
//   - pickups.txt "usereach 1" (Frame::anywhere false): 0.1.14's rule, bit for bit: only while free hands are awake and no scripted moment; the hand
//     within the point's reach (Reach: its disc, Point::disc, kDisc unless the prompt has its own: the AT4's, its tube lying 0.65 m off the point;
//     across, from kBelow under it to kAbove over it, kWiden wider each metre up; once where the thing lies is measured, the capsule round it,
//     kCapsuleGrow wider); a tick on entering it, once until that hand has left it by kLeaveMore past it; a fresh squeeze in reach (never one carried
//     into it) held kHold in reach.
//   - A prompt that gives a gun (Point::gives: the reload key's, the AT4's) only with no gun in either hand (Frame::gunHeld: by its grip or its front,
//     flying into one, a take of one under way; free hands asleep: the game's gun in the right hand). The owner, 2026-11-19: "Rifle dropped on the floor
//     first", then "Better to physically drop the rifle and pick up the AT": the rifle dropped or put on a shoulder first, never swallowed from a hand by
//     the scene's kit. With a gun in a hand: no tick, and the squeeze held kHold is refused (Out::refused: the double click, holster.h's, and hands.cpp's
//     line), never asked. A door gives no gun: a gun in the other hand is no bar there (the free hand opens it with the rifle in the right hand).
//   - The squeeze that is the prompt's is the use's (spent: hands.cpp tells the grasp the hand is busy) till it's let go: a lying gun's take, the
//     magnet's or a regrip never comes of it, nor of a refused one. If the hand takes something else before its use is asked (it isn't empty any more:
//     a magazine, a grenade, the gun's front), its use is given up (Out::given). Once its use is asked or refused with it (Out::claimed), the hands'
//     other modules take nothing with it either (hands.cpp's g_useOwnsBy: the handguard, the pouch, a magazine, the handle...: one owner per squeeze,
//     the audit of 2026-11-19); before that a module's take still wins and the use gives up (the handguard squeezed 20-35 cm out and brought to it, the
//     belly's 0.4 s reach window: the review of 2026-11-25, finding 1).
#include <cmath>
#include <cstdint>
#include <cstring>

namespace usepoint {

constexpr float kDisc = 0.5f;         // m across from the point
constexpr float kBelow = 0.3f;        // m under it (a point a little above the floor, a hand on the floor)
constexpr float kAbove = 1.2f;        // m over it (standing, a hand lowered to the knees)
constexpr float kWiden = 0.2f;        // m wider for each metre up
constexpr float kCapsuleGrow = 0.15f; // m round a measured capsule
constexpr float kLeaveMore = 0.05f;   // m past the reach before a hand in it has left it (its tick isn't said again and again at the edge)
constexpr float kSqueezeOn = 0.6f, kSqueezeOff = 0.4f;  // the grip: squeezed at or past, let go under (the grasp's own: grasp.h takes at kTake or more)
constexpr float kOnLying = 0.03f;     // m: a hand this near a lying gun's box is on it (LyingFirst: a prompt that gives a gun, its target unmeasured)
constexpr float kHold = 0.25f;        // s the squeeze is held (in reach: "usereach 1")
constexpr float kHeldInto = 0.4f;     // s a prompt has shown before a squeeze held from before (begun free) is its (Frame::anywhere)
constexpr float kGapMost = 0.3f;      // s a prompt may go unseen and still be the one showing (the game rescans its target every 0.17-0.20 s)
constexpr float kRetryAfter = 0.5f;   // s before a use not made for a moment is asked again (Rearm) ...
constexpr int kMostRetries = 2;       // ... at most this many times a squeeze
constexpr float kTickAmp = 0.25f, kTickSeconds = 0.02f;  // holster.h's tick
constexpr float kDoubleAmp = 0.5f, kDoubleSeconds = 0.015f, kDoubleGap = 0.06f;  // holster.h's double click (a refusal): the second 60 ms on
// The prompts a hand uses (ByHand): the game's own enum (EntryInputActionEnum, read in the image: pickups.h's kUseReload, kUseInteract), its flag kind.
constexpr int kActionReload = 15, kActionInteract = 35;
constexpr int kFlagKind = 3;          // IET_CTFFlag: its own path (a ray test, no key)
constexpr float kMostRadius = 5.0f;   // m: the game's own UseWithinRadius for it at most (the campaign's E and R prompts' 0.75-3, Kaffarov's HALO start
                                      // 5; the jet's 50-300 and the tank's laser 300 never)

// A prompt a hand may use: its key's action, its kind, its HoldToInteractTime (s) and its UseWithinRadius (m), as pickups.h's UsePoint has them.
inline bool ByHand(int action, int kind, float hold, float radius) {
    return (action == kActionReload || action == kActionInteract) && kind != kFlagKind && hold == 0.0f && std::isfinite(radius) && radius > 0.0f &&
           radius <= kMostRadius;
}

struct Point {
    bool on = false;           // the game targets it, it's on, not used up, a prompt a hand uses (ByHand); "usereach 1": and its place known
    uint32_t id = 0;           // which (the game's entity: another one shows anew; 0 not known: one prompt)
    float at[3] = {};          // where it is (the room: y up; not a number: not known, Frame::anywhere only)
    float disc = kDisc;        // m across: its own (the AT4's 1.0), else kDisc
    bool capsule = false;      // where the thing lies is measured: the segment a..b of radius r (the room)
    float a[3] = {}, b[3] = {};
    float radius = 0.0f;
    bool gives = false;        // it gives a gun (the reload key's, the game's pickup key: the AT4's): used only with no gun in either hand
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
    bool asleep = false;       // nothing now (anywhere: a new life's first frame, a switch's bridging frames, a fist fight's run; "usereach 1": free
                               // hands asleep or a scripted moment too)
    bool gunHeld = false;      // a gun in a hand (hands.cpp: by its grip or its front alone, flying into one, a take or a draw of one under way)
    bool anywhere = true;      // the grip of a free hand anywhere while the prompt shows; false: "usereach 1", 0.1.14's reach rule
};
struct HandState {
    bool inReach = false;      // in reach last frame (its tick said; "usereach 1")
    bool down = false;         // the squeeze past kSqueezeOn (till under kSqueezeOff)
    bool owned = false;        // that squeeze is the use's (begun in reach; anywhere: fresh while it shows, or held into it): spent till let go
    bool asked = false;        // ... and its use asked, given up or refused already
    bool claimed = false;      // ... asked or refused with it (not given up): the use's for good till let go (Out::claimed)
    bool refused = false;      // ... refused (a gun in a hand at a prompt that gives one): never asked again with it
    float held = 0.0f;         // s it has been held (in reach; anywhere: while the prompt shows)
    float secondAt = -1.0f;    // a refusal's second pulse due then (State::clock; under 0 none)
    bool beganFree = false;    // anywhere: the squeeze's first frame was a free hand's (tracked, empty, its squeeze nothing else's)
    bool seen = false;         // anywhere: the grip read last frame (a squeeze is fresh only from a let-go read so: never one unread, nor at the start)
    int retries = 0;           // anywhere: its use asked again so far (Rearm)
    float retryAt = 0.0f;      // ... not before then (State::clock)
};
struct State {
    HandState hand[2];
    float clock = 0.0f;
    bool anywhere = true;      // the rule last frame's Update went by (Rearm acts only on Frame::anywhere's)
    bool showing = false;      // anywhere: a prompt shows (the game targets it now, or did within kGapMost)
    uint32_t id = 0;           // ... which (Point::id)
    float shownFor = 0.0f;     // ... s it has shown (its gaps not counted)
    float gap = 0.0f;          // ... s since it last showed (0 while it shows)
};
struct Pulse {
    float amplitude = 0.0f, seconds = 0.0f;
};
struct Out {
    int use = -1;              // the hand whose use is asked now (-1 none)
    bool entered[2] = {};      // that hand's tick this frame: its use asked (anywhere: the hand that does it, the first ask of its squeeze); "usereach
                               // 1": come into reach
    bool spent[2] = {};        // that hand's squeeze is the use's this frame (the grasp is told the hand is busy)
    bool claimed[2] = {};      // ... and its use asked or refused with it: no other module of the hands' takes anything with it (spent alone: a
                               // module's take still wins, and the use gives up)
    bool given[2] = {};        // that hand's squeeze, the use's, took something else before its use was asked: given up this frame
    bool refused[2] = {};      // that hand's squeeze held kHold of a prompt that gives a gun with a gun in a hand: refused this frame (the double
                               // click's first pulse in pulse; hands.cpp says why)
    bool took[2] = {};         // anywhere: that hand's squeeze became the prompt's this frame ...
    bool heldInto[2] = {};     // ... held from before (begun free), not a fresh squeeze
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

// How near the use's target is to a hand at `at` (the room, m), for the hands' choice between it and a gun lying within the grasp's reach (hands.cpp's
// UseYields; the audit of 2026-11-19: the prompt was judged by the grasp's 25 cm while the grasp reached 35 and the column, a few centimetres deciding
// who took the squeeze). The measured capsule: to its surface (0 inside it). A prompt whose point is where its thing is (its reach the plain kDisc): across
// from the point, its reach's own measure. A prompt whose thing lies off its point unmeasured (its own disc: the AT4's until a census at the prompt fills
// its capsule): -1, no distance known. A number not finite: -1.
inline float TargetDistance(const Point& p, const float at[3]) {
    for (int i = 0; i < 3; ++i)
        if (!std::isfinite(at[i]) || !std::isfinite(p.at[i]))
            return -1.0f;
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
        const float off = std::sqrt(d) - (std::isfinite(p.radius) && p.radius > 0.0f ? p.radius : 0.0f);
        return std::isfinite(off) ? (off > 0.0f ? off : 0.0f) : -1.0f;
    }
    if (p.disc != kDisc)
        return -1.0f;
    const float dx = at[0] - p.at[0], dz = at[2] - p.at[2];
    return std::sqrt(dx * dx + dz * dz);
}
// Whether a gun lying within the grasp's own reach of a hand (gun: its distance by the grasp's measure, m) goes before the use's target for that hand's
// squeeze: only when it is nearer (a tie is the prompt's). With no target distance known (-1): only within pickReach, as before it was measured.
inline bool GunFirst(float gun, float target, float pickReach) {
    if (!(gun >= 0.0f))
        return false;
    if (!(target >= 0.0f))
        return gun < pickReach;
    return gun < target;
}
// One gun lying within the grasp's own reach of a hand against the use's target for that hand's squeeze (hands.cpp's UseYields, each lying gun): box,
// its box's distance from the hand by the grasp's measure (m); within, within the grasp's reach of it (pickReach, or the magnet's sphere for a settled
// hand and a gun at rest); rank, the magnet's column's rank for a hand reaching down over it (under 0: not in its column). The nearer goes first (GunFirst, a
// tie the prompt's). A prompt that gives a gun whose thing lies off its point unmeasured (the AT4's own 1 m disc: TargetDistance -1): the lying gun
// only for a hand on it (its box within kOnLying, 3 cm), never by the column (the integration review of d32a39b, finding 2: the M4A1 let go of 5-15 cm
// beside the tube took a squeeze made on the tube, and a hand 35-60 cm over the rifle on its way down took the rifle where 0.1.14 asked for the AT4; a
// rifle dropped at the tube lands by the hand, so the owner's flow (b) could go round and round; with 8 cm, the review of 075277d, a rifle 1-8 cm
// beside the tube still took 18-20 of 21 grips along it). A rifle dropped onto the tube, or within 3 cm of the hand on it, is still the rifle's till
// the tube's capsule is measured (a census at the prompt: then the nearer goes first). Any other prompt with no target known (its place not known,
// Frame::anywhere): within pickReach, as before.
inline bool LyingFirst(const Point& p, float target, float box, bool within, float rank, float pickReach) {
    if (p.gives && !(target >= 0.0f))
        return within && box >= 0.0f && box < kOnLying;
    if (within)
        return GunFirst(box, target, pickReach);
    if (rank >= 0.0f)
        return GunFirst(rank, target, pickReach);
    return false;
}
// Whether the body's places (the belly and the chests: holster::AtBodyTake) keep a hand's squeeze from this prompt (hands.cpp's UseYields): theirs are
// the pouch's, handmag's, the grenade launcher's, the rocket's, a grenade's. Not for a prompt that gives a gun with no gun in either hand (the
// integration review of d32a39b, finding 7): every one of those wants a gun or a launcher in the hands then, and the chests answer nothing yet, so a
// hand there in the prompt's reach (resting on the lap, standing at the roof's AT4) had no tick and no use, and its squeeze took nothing. With a gun in
// a hand that prompt is barred yet owns a fresh squeeze in its reach (Update: its refusal claims at kHold, inside the pouch's 0.4 s window): the body
// keeps it then, so a squeeze at the belly for a magazine is the magazine's, never the double click.
inline bool BodyFirst(const Point& p, bool gunHeld) { return !(p.gives && !gunHeld); }
// ... as the rule in use has it (Frame::anywhere): any prompt, held anywhere, by finding 7's reason: with no gun in either hand nothing at the body takes a
// squeeze (the pouch's magazines, the launchers' rounds and rockets want a gun or a launcher in the hands; the chests answer nothing yet), so a hand
// resting at the belly at a door is the door's; with a gun in a hand the body keeps it (a magazine, a round). "usereach 1": BodyFirst, as 0.1.14.
inline bool BodyKeeps(const Point& p, bool gunHeld, bool anywhere) { return anywhere ? gunHeld : BodyFirst(p, gunHeld); }

// A refusal's double click for that hand (holster.h's: two short pulses kDoubleGap apart): the first now (returned), the second sent by Update when
// it's due. Also for one refused outside Update (hands.cpp's: X held's R at a prompt that gives a gun, with a gun in a hand).
inline Pulse Refuse(State& s, int side) {
    if (side < 0 || side > 1)
        return Pulse{};
    s.hand[side].secondAt = s.clock + kDoubleGap;
    return Pulse{kDoubleAmp, kDoubleSeconds};
}

// Whether a use not made failed for a moment, worth asking again while its squeeze is held (Rearm), by the words pickups.cpp gives (RequestUse's,
// ServeUse's, UseOutcome's): the game didn't target it at its next tick, it came too late or wasn't taken in a second (a stall: never made), the soldier
// wasn't awake. Never used up, not on, the game's own checks refusing it, its use not as read or faulted, off, its outcome not heard (perhaps made),
// another use asked or waiting (a second use could kick a door twice).
inline bool Transient(const char* why) {
    static const char* const kTransient[] = {"the game doesn't target it now", "too late", "the soldier isn't awake", "not answered in 1 s"};
    if (!why)
        return false;
    for (const char* t : kTransient)
        if (std::strcmp(why, t) == 0)
            return true;
    return false;
}

// That hand's use, asked and not made for a moment (Transient: hands.cpp's NoteUse), asked again kRetryAfter on, while its squeeze is still held and
// the prompt shows (the squeeze stays claimed meanwhile: nothing else of the hands' takes it), kMostRetries times a squeeze at most; Frame::anywhere's
// rule only. False: none to ask again (let go, never asked, refused or given up, the retries spent, "usereach 1").
inline bool Rearm(State& s, int side) {
    if (side < 0 || side > 1 || !s.anywhere)
        return false;
    HandState& h = s.hand[side];
    if (!h.down || !h.owned || !h.asked || !h.claimed || h.refused || h.retries >= kMostRetries)
        return false;
    h.asked = false;
    ++h.retries;
    h.retryAt = s.clock + kRetryAfter;
    return true;
}

namespace detail {

// "usereach 1": 0.1.14's step, bit for bit (the tests' t6 rows).
inline void ReachStep(const Frame& f, State& s, Out& out, float dt) {
    // A prompt that gives a gun with a gun in a hand: no hand uses it (the owner, 2026-11-19: "Better to physically drop the rifle and pick up the AT").
    const bool barred = f.point.gives && f.gunHeld;
    for (int side = 0; side < 2; ++side) {
        HandState& h = s.hand[side];
        const Hand& in = f.hand[side];
        if (h.secondAt >= 0.0f && s.clock >= h.secondAt - 1e-4f) {  // (a refusal's second pulse, whatever else this frame)
            out.pulse[side] = Pulse{kDoubleAmp, kDoubleSeconds};
            h.secondAt = -1.0f;
        }
        const float squeeze = in.tracked && std::isfinite(in.squeeze) ? in.squeeze : 0.0f;
        const bool wasDown = h.down;
        // (Down at kSqueezeOn, as the grasp takes at its kTake: with "past" alone a frame reading exactly 0.6 let the grasp take a lying gun before the
        // use owned the squeeze; the integration review of d32a39b.)
        h.down = h.down ? squeeze >= kSqueezeOff : squeeze >= kSqueezeOn;
        const bool fresh = h.down && !wasDown;
        if (!h.down)
            h.owned = h.asked = h.claimed = false, h.held = 0.0f;
        const bool live = !f.asleep && f.point.on && in.tracked;
        const bool reach = live && Reach(f.point, in.at);
        // A hand free for it: holding nothing, its squeeze nothing else's. The tick: into reach (a hand holding something, or at something else of the
        // hands', isn't reaching for it; nor one while the prompt is barred: the tick says "hold the grip here"), out of it only kLeaveMore past it; the
        // bar lifted with the hand still in reach (the rifle let go of, or put away): the tick then.
        const bool free = in.empty && !in.yield;
        const bool usable = free && !barred;
        if (reach && !h.inReach && usable && out.pulse[side].seconds <= 0.0f) {  // (a refusal's second pulse now: the tick the frame after)
            out.entered[side] = true;
            out.pulse[side] = Pulse{kTickAmp, kTickSeconds};
        }
        h.inReach = h.inReach ? usable && live && Reach(f.point, in.at, kLeaveMore) : out.entered[side];
        // A fresh squeeze in reach by a free hand is the use's till it's let go (barred too: its refusal, never another thing's).
        if (fresh && reach && free)
            h.owned = true;
        if (!h.owned)
            continue;
        out.spent[side] = true;
        if (h.asked) {
            out.claimed[side] = h.claimed;
            continue;
        }
        if (!in.empty) {  // (it took something else first: a magazine, a grenade, the gun's front, a gun given to it)
            h.asked = true;
            out.given[side] = true;
            continue;
        }
        h.held = reach ? h.held + dt : 0.0f;  // (out of reach: the hold starts again once it's back, the squeeze still the use's)
        if (h.held >= kHold && out.use < 0) {
            h.asked = true;
            h.claimed = true;
            out.claimed[side] = true;
            if (barred) {  // (as the hold ends: a gun let go of, or put away, within it still lets the use be asked)
                out.refused[side] = true;
                out.pulse[side] = Refuse(s, side);
            } else {
                out.use = side;
            }
        }
    }
}

// Frame::anywhere's step: the grip of a free hand while the prompt shows.
inline void AnywhereStep(const Frame& f, State& s, Out& out, float dt) {
    // The prompt showing: the game targets it now, nothing asleep. Gone kGapMost at most, it's still the one showing (its holds kept, none counted
    // meanwhile); another prompt shows anew (its holds start again, the squeezes the use's stay so).
    const bool shows = f.point.on && !f.asleep;
    if (shows) {
        if (s.showing && f.point.id != s.id) {
            s.shownFor = 0.0f;
            s.hand[0].held = s.hand[1].held = 0.0f;
        }
        s.showing = true;
        s.id = f.point.id;
        s.gap = 0.0f;
        s.shownFor += dt;
    } else if (s.showing) {
        s.gap += dt;
        if (s.gap > kGapMost + 1e-4f) {
            s.showing = false;
            s.id = 0;
            s.shownFor = s.gap = 0.0f;
            s.hand[0].held = s.hand[1].held = 0.0f;
        }
    }
    // A prompt that gives a gun with a gun in a hand: no hand uses it (the owner, 2026-11-19: "Better to physically drop the rifle and pick up the AT").
    const bool barred = f.point.gives && f.gunHeld;
    for (int side = 0; side < 2; ++side) {
        HandState& h = s.hand[side];
        const Hand& in = f.hand[side];
        if (h.secondAt >= 0.0f && s.clock >= h.secondAt - 1e-4f) {  // (a refusal's second pulse, whatever else this frame)
            out.pulse[side] = Pulse{kDoubleAmp, kDoubleSeconds};
            h.secondAt = -1.0f;
        }
        h.inReach = false;
        // Not read (its controller unseen a moment, the headset off, the stand-in's): nothing is known of the grip: its state kept, nothing counted,
        // never a let-go; read again, a squeeze then is none fresh (as one at the state's first frame, after a new life: held from before, begun no
        // one knows where).
        if (!in.tracked || !std::isfinite(in.squeeze)) {
            h.seen = false;
            if (h.owned)
                out.spent[side] = true, out.claimed[side] = h.claimed;
            continue;
        }
        const float squeeze = in.squeeze;
        const bool wasDown = h.down;
        h.down = h.down ? squeeze >= kSqueezeOff : squeeze >= kSqueezeOn;  // (down at kSqueezeOn, as the grasp takes at its kTake)
        const bool fresh = h.down && !wasDown && h.seen;
        h.seen = true;
        if (!h.down)
            h.owned = h.asked = h.claimed = h.refused = h.beganFree = false, h.held = 0.0f, h.retries = 0, h.retryAt = 0.0f;
        // A hand free for it: seen, holding nothing, its squeeze nothing else's. Its squeeze begun so may become the prompt's held from before.
        const bool free = in.empty && !in.yield;
        if (fresh)
            h.beganFree = free;
        // The prompt's: a fresh squeeze of a free hand while it shows; one held from before, begun free and free now, once it has shown kHeldInto (barred
        // too: its refusal, never another thing's).
        if (!h.owned && h.down && free && shows && (fresh || (h.beganFree && s.shownFor >= kHeldInto - 1e-4f))) {
            h.owned = true;
            h.held = 0.0f;
            out.took[side] = true;
            out.heldInto[side] = !fresh;
        }
        if (!h.owned)
            continue;
        out.spent[side] = true;
        out.claimed[side] = h.claimed;  // (asked again after a moment's failure: still claimed meanwhile)
        if (h.asked)
            continue;
        if (!in.empty) {  // (it took something else first: a magazine, a grenade, the gun's front, a gun given to it)
            h.asked = true;
            out.given[side] = !h.claimed;
            continue;
        }
        if (shows)
            h.held += dt;  // (in a gap: kept, not counted)
        if (h.held >= kHold - 1e-4f && shows && out.use < 0 && s.clock >= h.retryAt - 1e-4f) {
            h.asked = true;
            h.claimed = true;
            out.claimed[side] = true;
            if (barred) {  // (as the hold ends: a gun let go of, or put away, within it still lets the use be asked)
                h.refused = true;
                out.refused[side] = true;
                out.pulse[side] = Refuse(s, side);
            } else {
                out.use = side;
                if (!h.retries && out.pulse[side].seconds <= 0.0f) {  // (the tick on the hand that does it, its first ask; a second pulse now: none)
                    out.entered[side] = true;
                    out.pulse[side] = Pulse{kTickAmp, kTickSeconds};
                }
            }
        }
    }
}

}  // namespace detail

// Once a frame (hands.cpp's FreeHands, before the grasp: its spent hands are told the grasp as busy).
inline void Update(const Frame& f, State& s, Out& out) {
    out = Out{};
    const float dt = std::isfinite(f.dt) && f.dt > 0.0f && f.dt < 0.25f ? f.dt : 0.0f;
    s.clock += dt;
    s.anywhere = f.anywhere;
    if (f.anywhere)
        detail::AnywhereStep(f, s, out, dt);
    else
        detail::ReachStep(f, s, out, dt);
}

// Why a hand doesn't answer the prompt showing now, after Update (hands.cpp's "Use grips:" lines: each grip's state and, if it isn't answering, why).
enum class Grip : unsigned char {
    kUnseen,        // not tracked or not read (its controller asleep, the stand-in's, the headset off)
    kOpen,          // not squeezed (under kSqueezeOn)
    kHolds,         // it holds something (the gun by its grip or front, a magazine, a round, the handle): a free hand's grip answers
    kElse,          // its squeeze is something else's (a holster's place, the belly or a chest with a gun in a hand, near the gun, a lying gun nearer)
    kBeganElse,     // held from before, begun where something else had it (a body's place, near the gun) or with the hand not free: let go, squeeze again
    kSettling,      // held from before, begun free: the prompt's once it has shown kHeldInto
    kHolding,       // the prompt's, held toward kHold
    kWaitRetry,     // the prompt's, its use to be asked again (a moment's failure)
    kAsked,         // its use asked (or made): one a squeeze
    kRefused,       // refused: a gun in a hand at a prompt that gives one
    kGiven,         // it took something else first: given up
    kNotShowing,    // the prompt doesn't show (a gap, asleep)
};
inline Grip GripOf(const Frame& f, const State& s, int side) {
    if (side < 0 || side > 1)
        return Grip::kUnseen;
    const Hand& in = f.hand[side];
    const HandState& h = s.hand[side];
    if (!in.tracked || !std::isfinite(in.squeeze))
        return Grip::kUnseen;
    if (!h.down)
        return Grip::kOpen;
    if (h.owned) {
        if (h.refused)
            return Grip::kRefused;
        if (h.asked)
            return h.claimed ? Grip::kAsked : Grip::kGiven;
        if (s.clock < h.retryAt - 1e-4f)
            return Grip::kWaitRetry;
        return Grip::kHolding;
    }
    if (!in.empty)
        return Grip::kHolds;
    if (in.yield)
        return Grip::kElse;
    if (!f.point.on || f.asleep)
        return Grip::kNotShowing;
    return h.beganFree ? Grip::kSettling : Grip::kBeganElse;
}
inline const char* GripWord(Grip g) {
    switch (g) {
    case Grip::kUnseen: return "not seen (its controller asleep, the stand-in's, or the headset off)";
    case Grip::kOpen: return "let go";
    case Grip::kHolds: return "its hand holds something (the gun by its grip or its front, a magazine, a round, the handle): a free hand's grip answers";
    case Grip::kElse: return "its squeeze is something else's";
    case Grip::kBeganElse: return "held from a squeeze begun where something else had it, or with the hand holding something: let go and squeeze again";
    case Grip::kSettling: return "held from before: the prompt's once it has shown 0.4 s";
    case Grip::kHolding: return "the prompt's, held toward 0.25 s";
    case Grip::kWaitRetry: return "the prompt's, its use asked again in a moment";
    case Grip::kAsked: return "its use asked (one a squeeze: let go and squeeze again for another)";
    case Grip::kRefused: return "refused: a gun in a hand (drop it or put it on a shoulder first)";
    case Grip::kGiven: return "it took something else first (let go and squeeze again)";
    case Grip::kNotShowing: return "squeezed, the prompt not showing this moment";
    default: return "";
    }
}

}  // namespace usepoint

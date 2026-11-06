#pragma once

#include <cstdint>

// The ground under a point, the game's own (the user, 2026-11-05: "I want it to hit the floor. I want it to stay there for a minute... since I'm seated,
// it has a hard time trying to find the floor. But I think you'll find the way"): the game's own ray cast against its own collision, the terrain, the
// buildings, crates, cars and the rest, the same cast its dropped weapons find the ground with (theirs in the server's world, 0089DE8C; these in the
// client's). Asked from the render thread and answered from the rays already known; what isn't known is cast on the game's own thread a frame or two
// later, in the client level's update right after it has put its lists of what's where up to date and before its physics starts its step (the step's
// jobs not running then; the lists' own query jobs, which the flush starts again at its end, may be: readers, beside which the game's own casts on that
// thread run too), a few rays a frame at most. Each ray answers for the 4 cm round where it was cast; till a point has its own, one cast near it is carried
// over, not firm. Before anything's hooked or called, every piece of the game's code it goes through is checked byte for byte against bf3_mem.exe, and every
// pointer it reads is guarded; anything not as read leaves it off for the session, said once in the log, and the soldier's feet answer. A fault in a cast
// leaves it 5 s without casting (what's known and the feet answer meanwhile); the third this session, off for the session.
// The world here is the game's own space (metres, y up): the arms' space plus its origin in whole metres (skin.cpp's g_fpOrigin).
namespace ground {

// What a ray hit: its physics entity's kind, by its vtable.
enum What : uint8_t { kUnknown, kTerrain, kStatic, kGroup, kMoving, kBreakable, kWater, kTrigger, kCharacter, kBody };

// Where an answer came from.
enum Source : uint8_t {
    kNone,     // nothing known yet (a ray has been asked for: a frame or two), or nothing known at all
    kNothing,  // a ray cast within 4 cm of it found nothing as far down as asked
    kRay,      // a ray cast within 4 cm of it (straight down, from no lower): firm
    kNear,     // a ray cast near it (in its 25 cm column or one beside it), its plane carried over until its own comes: not firm, it lands a falling thing,
               // it never holds a lying one (by a crate's edge one cast a few cm off finds the floor beside it, or the crate's top)
    kFeet,     // the soldier's feet: the ray under the eye (cast twice a second while anything asks; its plane within 0.5 m of where it hit, flat at its
               // height beyond), or the eye less its height for the stance
};

struct Hit {
    float point[3] = {};                   // on the ground, in the world
    float normal[3] = {0.0f, 1.0f, 0.0f};  // out of it, unit; facing down, the ray began inside something and found its underside
    int material = -1;                     // the game's own material index (what its impacts and footsteps go by); -1 not known
    What what = kUnknown;
    Source source = kNone;
};

// The first surface under at (the world), howFar metres down at most. True with it in out (kRay, kNear or kFeet); false with out.source kNothing (a ray
// found nothing that far down) or kNone (not known yet). A ray known already may have found it further down than asked: out.point says where. Any
// thread; never calls the game.
// Ask from a thing's top, or from where it was last, not from its middle: 3 cm into a surface still counts as on it, but a ray from deeper under it finds
// the next surface down (a thing that landed on a guess lower than the real floor, or moved fast near it, would be answered from under the floor: the
// review's item 14, for loose's and hands' callers to change).
bool Down(const float* at, float howFar, Hit& out);

// The same for the loose things (loose::FloorFn): the ground under a point of the arms' space, origin the arms' space's place in the world (whole
// metres), as a plane in the arms' space, looked for 6 m down and then 50. Only ground facing up (a normal's y over 0.05): an underside isn't a floor.
// How many planes it gave: 1; 0 none known now (a ray not back yet, or the ground found faces down); -1 nothing there: even the ray 50 m down found
// nothing (loose::FloorFn's -1: it falls; a caller passes it straight through, never a guess of its own in its place). Stairs and edges: ask at each
// corner.
struct Plane {
    float point[3];
    float normal[3];
    int material;
    What what;
    Source source;
};
int Planes(const float* atArms, const float* origin, Plane* planes, int most);

// Each frame: the game's eye in the world and the soldier's stance (0 standing, 1 crouched, 2 prone: eye heights 1.6, 1.0 and 0.3 m), for the feet and
// the probe. Any thread.
void SetEye(const float* eye, int stance);

// weapon.txt "ground 0": nothing of the game's hooked or called, the feet answer. On by default; nothing's hooked until something asks.
void SetOn(bool on);
// weapon.txt "groundprobe 1": once a second a ray 3 m straight down from the eye, logged both ways (flags 0xB5 and 0x34): how far below the eye it hit,
// the normal, the material, what it was, the microseconds; and for its first 10 s the game's own casts counted by thread, both the cast and the caster's
// own copy of it by its interface (their hooks on for those 10 s alone; the count ends early, said, if the probe goes off). "groundprobe 2": a 7 x 7 map
// of heights round the eye every 5 s as well, 0.5 m apart (its rays only logged, never remembered).
void SetProbe(int level);
// weapon.txt "groundat 1", for a test only: cast at the end of the level's update instead of right after its flush. The physics' step doesn't wait for
// its new jobs before it returns (01882DEA-01882E01), so these casts run beside them; it's quiet only with the physics paused (its step then flushes
// nothing, and groundat 0 has nowhere to cast: the probe says so).
void SetWhere(int where);
// weapon.txt "groundflags 0xB5": the flags the rays are cast with (the rays known are forgotten).
void SetFlags(uint32_t flags);
// Forget every ray known (a change of level is seen by itself).
void Clear();
// Once a frame on the render thread: hooks once something's asked, the feet's ray, the probe, the log.
void OnFrame();

}  // namespace ground

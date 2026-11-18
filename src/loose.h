#pragma once

// Loose things let go of: magazines, the pistol's magazine, the launchers' spent cases, casings, the player's own rifle (gunbody.h), each a rigid body lying in
// the world. The user, 2026-11-04: "it needs to physically hit the floor, perfect physics, collision... bounce a little bit... if you had a magazine
// and you were to throw it... it rotates around the air"; 2026-11-05: "I want magazine physics. I want the launcher physics. When I take out the empty
// shell, I want all of that, and I want it to hit the floor. I want it to stay there for a minute. Same thing as the bullet casings".
//   - Out with the button (Eject): it slides down its well with the gun, pushed, held to the well's line by the walls (the more the gun is held
//     sideways, the harder they grip: held sideways it "should slide out... gravity pull and weight"), until its top is past the mouth; it tips about
//     the mouth's lower lip as the last of it leaves; then it's free, with the gun's own speed and turn.
//   - In an open barrel (TipOut: the M320's spent case; the user, 2026-11-05: "If the M320 is open, then it wants to"): it stays in, held by the
//     barrel's walls, until gravity along the barrel beats their grip (the muzzle raised past about 30 degrees), or a jerk of the gun leaves it
//     behind; then it slides out of its own weight and falls.
//   - Let go of, or thrown (Release, or ReleaseBy for either hand): the hand's own speed and turn over its last frames, in the room (the stick's
//     turning isn't a throw), and the player's walking.
//   - Thrown off the gun (Flick): knocked out by a fresh one (handmag's flick), a casing out of the port, the launcher's case slid off its back: the
//     gun's own speed and turn there, and its own on top.
//   - Free: gravity, the air, its corners (or its round ends) on the ground: bounce, friction, rolling, tipping onto a flat side; at rest it sleeps.
//   - It lies 30 s (kLieSeconds) once at rest, then goes only while nobody looks at it: never in view. A gun (Spec::keep) lies until it's taken.
// Pure: no game calls and no logs, as handmag.cpp. One thread (the render thread at Present, where hands.cpp runs): a frame's starts, then that
// frame's Step with the same Frame. skin.cpp draws each Pose through each draw's own way from the world (HandTargets::drawnFromWorld,
// WorldToDrawn with Frame::origin), as the dropped magazine is.
// The world here is the arms' space with its origin (whole metres: skin.cpp's g_fpOrigin): y up, metres. All that's given and given back is in the
// arms' space with this frame's origin (Frame::origin); each thing keeps the origin it started with inside, so it stays put in the world as the
// origin jumps a metre, and floats stay fine far from it.

namespace loose {

constexpr int kMost = 10;             // magazines, cases and the rifle lying and moving at once (the drawing's cost decides: SetMost)
constexpr int kCasings = 192;         // casings, lying and moving at once: a minute of steady fire's worth
constexpr int kAll = kMost + kCasings;
constexpr int kHits = 32;             // sounds a frame at most (the loudest kept)
constexpr int kPlanes = 4;            // the most planes the ground gives for one place
// At rest this long, then it goes when out of view. A minute at first ("I want it to stay there for a minute"); the user, 2026-11-05: "let's have them disappear after 30
// seconds. Because I don't want to flood the map full of artifacts" (magazines, casings, the launchers' cases, the 40 mm cases: all of them).
constexpr float kLieSeconds = 30.0f;

// The game's own sound material index: its footsteps' graph (sound/character/soldier_movement_footsteps_sp) switches on it, and its dropped
// weapons' (sound/maoism/.../soldier_ragdoll_and_weapon_drop_sp) maps it to weapon_on_<surface>: parameter hash 0xD565F120 in both.
enum Surface { kDirt, kTarmac, kGrass, kGrassTall, kGravel, kMetal, kVehicleMetal, kSand, kWater, kWood, kForest, kWaterTarmac, kWoodIndoor, kSurfaces };

// What it is. A magazine (the M240's box too): its box in its own vertices' space (skin.cpp's tables), y along it with its top (the end in the
// well) at boxMax's y, z from its back to its front. Its body is the box's depth less the sweep deep from front to back; its bottom sits sweep
// forward of its top (+z; - back, the pistol's); the middle of its back bows sag out behind the line from its top to its bottom (the AK family's
// curve; handmag.h's back edges).
// A round thing instead (round 0 x, 1 y, 2 z: the axis it's round about, in its own vertices' space): rings along that axis through roundAt (its
// other two coordinates, in order), each at ringAt along it with ringRadius, from its back to its front; between them it's straight; head: the
// share of its weight in a solid head at its back (a casing's brass base), the rest in its wall.
// A gun (gunbody.h): keep, it lies until it's taken (never pushed out by a newer one, never gone for lying its time or sliding too long; still gone
// with no ground under it, or broken numbers); a box with its own hull: its weight and turn from the box, where it touches the ground from the hull's
// points (its own vertices' space, at most 22) instead of the box's corners, so it tips over onto a side as a real one does. A round thing has no hull.
struct Spec {
    const char* name;
    float boxMin[3], boxMax[3];
    float sweep, sag;
    float massEmpty, massRound;  // kg
    int capacity;
    float bounce;                // its own share of a bounce: 1 a metal magazine, more a hollow polymer one or brass, less a soft box
    int round = -1;              // -1: a magazine's box
    float roundAt[2] = {};
    int rings = 0;               // 2 to 4
    float ringAt[4] = {}, ringRadius[4] = {};
    float head = 0.0f;
    bool casing = false;         // one of many small ones: its own kCasings, a cheaper step
    bool keep = false;           // lies until taken (a gun)
    const float (*hull)[3] = nullptr;  // where it touches, instead of the box's corners (read only when it's started: a caller's own may be on the stack)
    int hullPoints = 0;
};

// By name: skin.cpp's HeldKind::shape ("an94", "rpk", "aek971", "ak74", "g3a3", "kh2002", "m16a4", "m4a1", "m240"), and "m416", "m9" (the pistol's),
// "smaw" (the launcher's case, on rocket.cpp's case bone), "m320" (its spent case: 46 mm long, 5.1 cm across; its base's middle at 0, its mouth along
// +z), "12g" (a live 12-gauge shell: 6.4 cm long, 2.1 cm across, likewise), casings "556", "545", "762", "9mm" (likewise, base at 0, mouth along +z).
// Null for any other.
const Spec* SpecFor(const char* name);

// A plane in the arms' space: the ground under it, or anything it runs into.
struct Plane {
    float point[3];
    float normal[3];      // out of the ground, unit
    int surface = kDirt;
    // The game's own ground (its ray cast): one lying where this says the ground is not settles again on it. Not firm, a guess (the soldier's
    // feet): it lands things, but never moves one lying (the feet go up a hill, or the eye down a crouch, with the magazine where it fell).
    bool firm = false;
};
// The caller's: the planes near this place (the arms' space this frame), at most `most`, and how many; 0: none known now (a ray not back yet, a
// place the guess doesn't reach: each keeps the ground it had); -1: nothing there (a ray that found nothing: it falls, and goes after 5 s of it).
// Asked each frame for each one moving, every 0.25 s for each one lying (spread over frames), at its middle.
using FloorFn = int (*)(void* user, const float* at, Plane* planes, int most);

// A hand, either one: tracked, its grip in the room (its turn and its grip point), and its controller's own speed (m/s) and turn (rad/s) in the
// room when the runtime gives them (sensed).
struct HandAt {
    bool tracked = false;
    float room[12] = {};
    bool sensed = false;
    float velocity[3] = {}, spin[3] = {};
};

struct Frame {
    float dt = 0.0f;                  // s since the last
    float roomToArms[12] = {};        // the room (the tracked space) into the arms' space this frame: rotation | translation
    int origin[3] = {};               // the arms' space's origin in the world, whole metres
    float walk[3] = {};               // the player's own walking, m/s in the arms' axes (how the room moves in the world at the head: not his turning)
    bool gun = false;                 // a gun in the right hand, as drawn: its frame bone in the room
    float gunRoom[12] = {};
    bool hand = false;                // the left hand tracked: its grip in the room (its turn and its grip point)
    float handRoom[12] = {};
    // Its controller's own speed (m/s) and turn (rad/s) in the room, as the runtime gives them from its motion sensors (input::HandVelocity): a throw's, truer than the
    // steps between its places (the user, 2026-11-05: a hard throw "sometimes locks itself behind my shoulder").
    bool handSensed = false;
    float handVelocity[3] = {}, handSpin[3] = {};
    float eye[3] = {}, look[3] = {};  // the head (the middle of the eyes) and where it looks, in the arms' space
    float seenCos = 0.34f;            // within acos of this of the look (70 degrees: the headset's view and some) is seen
    FloorFn floor = nullptr;
    void* floorUser = nullptr;
    HandAt hands[2];                  // both hands, [0] the left (the same as hand... above), [1] the right: ReleaseBy's
};

// Where it's drawn: its vertices' space into the arms' space with this frame's origin (bones drawn with it: this times their seat in its own space).
struct Pose {
    int id = 0, kind = 0;
    float bone[12] = {};
    bool resting = false;
    bool inGun = false;  // still in its well or its barrel, moving with the gun
};
// A contact loud enough to be heard (kept apart and gated as the game's own impact player does), and how loud (the game's curve, 0 dB at 6 m/s,
// and its fall-off with distance): the caller picks the clip by kind and surface.
struct Hit {
    int id = 0, kind = 0;
    float at[3] = {};
    float speed = 0.0f;  // m/s into the ground
    float gainDb = 0.0f;
    int surface = kDirt;
    bool first = false;  // its first landing
};
struct Result {
    int poses = 0;
    Pose pose[kAll];
    int hits = 0;
    Hit hit[kHits];
    int gone = 0;
    int goneId[kAll] = {};  // went since the last Step: unseen after its minute, fallen out of the world, or pushed out by a newer one
};

// The starts. Each gives an id (0: not started: no room, no gun, or broken numbers); kind is the caller's (which gun's draws it's drawn with).
// rounds: what's left in it (its weight). A thing started is drawn just where it was given until the next Step moves it.
// Out with the button: seated at onGun (its vertices' space on the gun's frame bone, Frame::gunRoom's: where the well holds it), out along axisOut
// (on that bone, unit), pushed at push m/s (0: SetPush's), well m from its seat to clear of the mouth.
int Eject(const Spec& spec, int kind, const float* onGun, const float* axisOut, float well, float push, int rounds, const Frame& f);
// In an open barrel: seated at onGun, its way out along axisOut (on the gun's frame bone), length m to clear of it; it goes when gravity along the
// barrel beats grip times what presses it on the walls (the tangent of how far the open end must point down: 0.58 for 30 degrees), or the gun's
// pushed away from it hard enough. Until then it moves with the gun (Pose::inGun). Remove it if the barrel's shut on it.
int TipOut(const Spec& spec, int kind, const float* onGun, const float* axisOut, float length, float grip, const Frame& f);
// Let go of, or thrown: bone as last drawn (its vertices' space into the arms' space), at the left hand's speed and turn over its last frames.
int Release(const Spec& spec, int kind, const float* bone, int rounds, const Frame& f);
// The same by either hand (side 0 the left, 1 the right): Frame::hands[side]'s speed and turn (its sensors', else its last steps' middle), the
// lever arm from that hand's grip point to its middle, and the walking. Release's numbers exactly for side 0 given the same hand.
int ReleaseBy(const Spec& spec, int kind, const float* bone, int rounds, const Frame& f, int side);
// Thrown off the gun: bone as last drawn; the gun's own speed and turn there, plus velocity (the arms' axes, m/s), turning at spin rad/s about
// axis (the arms' axes).
int Flick(const Spec& spec, int kind, const float* bone, int rounds, const float* velocity, const float* axis, float spin, const Frame& f);

// Once a frame, after the starts: moves them all. What it gives stays until the next Step or start.
const Result& Step(const Frame& f);
// Taken back (the barrel shut on it, the hand catching it): no longer here, and not told as gone.
bool Remove(int id);
bool Has(int id);
// All gone (a new life, a checkpoint, another level, a change of mission).
void Clear();
// How many there are now, and at rest.
int Count();
int Resting();

// weapon.txt, for tuning in the headset: the push out of a well (m/s), its walls' friction, how long they lie (s), how bouncy every ground is (x),
// how many may lie at once (big ones, casings).
void SetPush(float metresPerSecond);
void SetWellFriction(float mu);
void SetLie(float seconds);
void SetBounce(float scale);
void SetMost(int big, int casings);

}  // namespace loose

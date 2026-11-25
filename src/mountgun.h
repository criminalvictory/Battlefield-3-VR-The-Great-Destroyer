#pragma once
// The mounted gun: Operation Swordbreaker's truck .50 and Uprising's Humvee M2 held by both grips. The owner, 2026-11-18, at Swordbreaker's end ("Get on
// the 50 cal"): "held with both hands ... two grips held ... together and close ... move both of their arms ... at the same directions, like mirroring each
// other, so it can move the machine gun"; 2026-11-25: "The truck 50 cal is important as well." The design and its review: docs/research/mounted_gun.md.
// The owner's three questions there are settled: the gun goes where the hands put its handles (on the truck's post, the handles pushed left swing the barrel
// right); past the arms' reach the right stick turns, as on foot (the body's own push at the edge, "mountpush", stays off); either trigger fires while its
// grip holds a handle.
//
// Pure: no game, no Windows. src/mountseat.cpp runs it in the game; tools/mountguntest.cpp tests every rule here, against a simulated turret for the
// steering. What is here:
//   the rows: every number the seat needs, each named with the runbook's step it must come from (docs/research/mounted_gun.md section 6) and marked
//     measured or not. The dump's figure stands beside a row where the dump has one, only to check a measurement against: nothing is ever steered,
//     placed or matched on a row that isn't measured (Ready names the first one missing, and src/mountseat.cpp says it in the log).
//   the seat's states (4.1): on foot; another seat (any seat no row is told for: Rock's ride, Guillotine's Humvee, Kaffarov's SUV, The Great
//     Destroyer's police car: today's behaviour exactly, the review's second high finding); the gun's climb-in (the game's own scene); the gun.
//   the quiet list (4.8): what the seat holds quiet, the one table the gates read (aim.cpp, input.cpp, hands.cpp, xr_runtime.cpp); and the right
//     stick's turn never through the mouse in the seat, where the mouse's X is the ring's own turn (the review's first high finding).
//   the handle law (4.4): the gun put where the hands put its handles; each hand's offset at its take kept in the body's frame and turned with every
//     stick turn (the review's fifth finding); both handles fitted at once.
//   the stick's turn in the seat (4.5): the body about the ring's axis, the eye's speed on its circle held to a comfortable one (the review's sixth).
//   the steering (4.6): the gun the hands want against the game's gun (read off the game's own camera, which looks along the barrel), into the turret's
//     own input, the pad's right stick or the mouse; it brakes for the turret's momentum by its stopping point, follows a sweep without a standing
//     lag, learns the input's strength only while the input holds steady, and stops itself on a wrong sign or no answer (the review's fourth).
//   the fire rule (4.7): either trigger, its grip holding a handle.
//
// Angles: the game's own sense, as its look yaw has them (aim.cpp: atan2 of the camera's back row's x and -z, growing turning right; pitch up
// positive). The world: the game's (y up). The steering works in degrees (its rows are), the geometry in radians and metres.

#include <cmath>
#include <cstdint>
#include <cstring>

namespace mountgun {

constexpr float kPi = 3.14159265358979f;
constexpr float kDeg = kPi / 180.0f;

// ------------------------------------------------------------------ vectors ------------------------------------------------------------------

struct V3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
};
inline V3 operator+(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline V3 operator-(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline V3 operator*(V3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
inline float Dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline float Length(V3 a) { return std::sqrt(Dot(a, a)); }
inline bool Finite(V3 a) { return std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z); }
inline float Wrap(float radians) { return std::remainder(radians, 2.0f * kPi); }
inline float WrapDegrees(float degrees) { return std::remainder(degrees, 360.0f); }
inline float Clamp(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }

// The turn about the vertical by a yaw, the game's way: (0, 0, 1) turned by yaw looks along yaw (YawOf). The ring's turn.
inline V3 Turn(float yaw, V3 v) {
    const float c = std::cos(yaw), s = std::sin(yaw);
    return {v.x * c - v.z * s, v.y, v.x * s + v.z * c};
}
// The tilt about the gun's own sideways axis: (0, 0, 1) tilted by pitch looks up by pitch. The gun's turn on its pivot.
inline V3 Tilt(float pitch, V3 v) {
    const float c = std::cos(pitch), s = std::sin(pitch);
    return {v.x, v.y * c + v.z * s, -v.y * s + v.z * c};
}
inline float YawOf(V3 forward) { return std::atan2(-forward.x, forward.z); }
inline float PitchOf(V3 forward) {
    const float l = Length(forward);
    return l > 0.0f ? std::asin(Clamp(forward.y / l, -1.0f, 1.0f)) : 0.0f;
}
inline V3 Forward(float yaw, float pitch) { return Turn(yaw, Tilt(pitch, {0.0f, 0.0f, 1.0f})); }
// The way a camera looks, from its rows as the game makes them (right, up, back: 9 floats; camera.cpp's, before the mod turns them).
inline void AnglesOfRows(const float* rows, float& yaw, float& pitch) {
    const V3 back{rows[6], rows[7], rows[8]};
    yaw = std::atan2(back.x, -back.z);
    const float l = Length(back);
    pitch = l > 0.0f ? -std::asin(Clamp(back.y / l, -1.0f, 1.0f)) : 0.0f;
}

// ------------------------------------------------------------------ the rows ------------------------------------------------------------------

// What can't run without a row.
enum Need : uint32_t {
    kNeedMatch = 1u << 0,  // telling a seat is this gun's (its entry, its vehicle's name, its camera on the gun)
    kNeedView = 1u << 1,   // the body fixed to the vehicle, the eye at the gunner's place (4.2)
    kNeedSteer = 1u << 2,  // the gun turned by the hands (4.4, 4.6) and fired by its triggers (4.7)
    kNeedPad = 1u << 3,    // ... through the pad's right stick
    kNeedMouse = 1u << 4,  // ... through the mouse
    kNeedHands = 1u << 5,  // step 2: the gloves drawn on the handles
    kNeedHud = 1u << 6,    // step 2: the seat's crosshair hidden
};

enum RowId : int {
    kRowEntry,          // the seat's entry index in its vehicle
    kRowVehicle,        // the vehicle's name as the seat reader reads it (text)
    kRowPivot,          // p: the gun's pivot from the ring's axis point, in the ring's frame (m)
    kRowCamera,         // c: the game's camera from the pivot, in the gun's frame (m)
    kRowCameraRigid,    // 1: the game's camera rides the gun's pitch body rigidly in play; 0: animated (then the ring's point is taken once)
    kRowPitchLimits,    // the gun's pitch: least and most (degrees)
    kRowHandleLeft,     // the left handle's grip point from the pivot, in the gun's frame (m)
    kRowHandleRight,    // the right handle's
    kRowEye,            // e: the gunner's eye from the ring's axis point, in the ring's frame (m)
    kRowLag,            // the turret's time to speed, yaw and pitch (s): its momentum as the steering's model has it
    kRowDelay,          // from an input to the turret's first turn (s): the game's tick and its delay
    kRowSigns,          // which way each input turns the gun: the pad's X on the yaw, its Y on the pitch, the mouse's X, its Y (+1 or -1)
    kRowLeast,          // the least input that moves it, yaw and pitch (a fraction of the input's most): the dead zones and thresholds
    kRowBarrelLine,     // 1: the rounds go along the game's gun (its camera), not along the view the mod turns to the head
    kRowFires,          // 1: the left mouse button fires the seat's gun (and nothing else of the mod's is needed for it)
    kRowPadYaw,         // the yaw's rate at the right stick: up to six (stick, degrees a second) pairs from the dead zone's edge to the whole stick
    kRowPadPitch,       // the pitch's
    kRowMouseYaw,       // the mouse's X: degrees a count at a steady stream, the most counts a frame that still add
    kRowMousePitch,     // its Y
    kRowArmsDraws,      // step 2: the gunner's arm draws in the seat (their keys)
    kRowCrosshairDraw,  // step 2: the seat's crosshair's draw
    kRowCount
};

struct RowKind {
    const char* name;
    const char* from;  // where it must come from: the runbook's step (docs/research/mounted_gun.md section 6)
    uint32_t need;
    int count;  // numbers in it (0: text)
};
inline constexpr RowKind kRowKinds[kRowCount] = {
    {"the gunner's entry index", "R1 and R3: the entry the use key gives, read beside the vehicle", kNeedMatch, 1},
    {"the vehicle's name", "R3: read from the vehicle the player is attached to", kNeedMatch, 0},
    {"the pivot from the ring's axis", "R4: the camera's circle and arc fitted (yaw a full turn, pitch from limit to limit, no input on the fitted frames)",
     kNeedMatch | kNeedView | kNeedSteer, 3},
    {"the camera from the pivot", "R4: the same fit", kNeedMatch | kNeedView | kNeedSteer, 3},
    {"the camera rigid on the gun", "R4: the fit's residual against the gun's turn", kNeedView, 1},
    {"the pitch limits", "R4: the pitch driven to each stop", kNeedSteer, 2},
    {"the left handle", "R6: the game's own left palm on the gun at four yaws and three pitches, against the camera", kNeedSteer | kNeedHands, 3},
    {"the right handle", "R6: the game's own right palm, the same", kNeedSteer | kNeedHands, 3},
    {"the gunner's eye", "R6: the gunner's head against the camera (the camera's place at zero pitch where they agree)", kNeedView, 3},
    {"the turret's time to speed", "R5: step responses, each held 1 s from rest", kNeedSteer, 2},
    {"the turret's delay", "R5: the first frame the camera turns after an input", kNeedSteer, 1},
    {"which way each input turns it", "R5: each input's step, with the game's vehicle invert as the owner has it, and a run with it on", kNeedSteer, 4},
    {"the least input that moves it", "R5: the smallest stick and mouse step that turns each axis", kNeedSteer, 2},
    {"the rounds along the gun", "R5: a burst with the view turned 30-40 degrees from the barrel: the hits on the barrel's line", kNeedSteer, 1},
    {"the left mouse button fires", "R2: the trigger in the seat with today's DLL", kNeedSteer, 1},
    {"the yaw's rate at the stick", "R5: the right stick from its dead zone's edge through the curve's knees to the whole stick, held 1 s each", kNeedPad, 12},
    {"the pitch's rate at the stick", "R5: the same on the pitch", kNeedPad, 12},
    {"the mouse's X", "R5: 20, 50, 100 and 200 counts in one frame, and a steady 5 a frame", kNeedMouse, 2},
    {"the mouse's Y", "R5: the same on the pitch", kNeedMouse, 2},
    {"the gunner's arm draws", "R6: tail in the seat (the glove draws expected)", kNeedHands, 2},
    {"the seat's crosshair draw", "R6: tail in the seat", kNeedHud, 1},
};

constexpr float kNone = 1e30f;  // a dump figure there isn't

constexpr int kRowNumbers = 12;
struct Measure {
    bool measured = false;
    float value[kRowNumbers] = {};
    const char* text = nullptr;  // a text row's
};

struct Gun {
    const char* name;
    const char* level;  // where it is (the game's level name: what the runbook loads)
    float dump[kRowCount][kRowNumbers];  // the dump's figures (kNone: none): to check a measurement against, never used for anything else
    Measure m[kRowCount];
};

// The guns. Every row unmeasured: the runbook (docs/research/mounted_gun.md section 6) has not been run in the game yet. The dump's figures are its
// component transforms summed at rest (section 2.2); a measured row replaces nothing here but its own Measure.
inline constexpr Gun kGuns[] = {
    {"the truck's .50 (Operation Swordbreaker's end)",
     "SP_Earthquake",
     {
         {1.0f, kNone},                                  // the entry: the gunner, entry order 1
         {kNone},                                        // the vehicle's name: how the reader names it is R3's
         {0.0f, 0.173f, -0.024f, kNone},                 // the pivot: (0.008, 2.024, -1.438) less the ring's (0.008, 1.851, -1.414)
         {0.0f, 0.202f, -0.608f, kNone},                 // the camera: (0.008, 2.226, -2.046) less the pivot
         {kNone},                                        // rigid: SoldierAnimatedCamera on (0.5 s): the dump can't say
         {-20.0f, 20.0f, kNone},                         // the pitch limits
         {0.139f, 0.032f, -0.307f, kNone},               // the left handle: the left hand's IK target (0.147, 2.056, -1.745) less the pivot
         {kNone},                                        // the right handle: none in the data
         {0.0f, 0.375f, -0.632f, kNone},                 // the eye: the camera at zero pitch, if the gunner's head is where it is
         {kNone}, {kNone}, {kNone}, {kNone}, {kNone}, {kNone},
         {0.15f, 0.0f, kNone},                           // the stick: the seat's curve has its dead zone to 0.15 (PadInputCurve_OpenGunner_Roll)
         {0.15f, 0.0f, kNone},                           // (_Pitch)
         {kNone}, {kNone}, {kNone}, {kNone},
     },
     {}},
    {"the Humvee's M2 (Uprising's staging area)",
     "SP_Earthquake2",
     {
         {1.0f, kNone},
         {kNone},
         {-0.010f, 0.495f, 0.796f, kNone},               // the pivot: (-0.011, 2.410, 0.682) less the ring's (-0.001, 1.915, -0.114)
         {0.0f, 0.268f, -0.978f, kNone},                 // the camera: (-0.011, 2.678, -0.296) less the pivot
         {kNone},
         {-35.0f, 35.0f, kNone},
         {kNone},                                        // no hand point in the data: the gunner's animation holds both handles
         {kNone},
         {-0.010f, 0.763f, -0.182f, kNone},              // the eye: the camera at zero pitch
         {kNone}, {kNone}, {kNone}, {kNone}, {kNone}, {kNone},
         {0.15f, 0.0f, kNone},                           // the same pad curves as the truck's (the review: the active gunner entry carries them)
         {0.15f, 0.0f, kNone},
         {kNone}, {kNone}, {kNone}, {kNone},
     },
     {}},
};
constexpr int kGunCount = static_cast<int>(sizeof(kGuns) / sizeof(kGuns[0]));

// Whether every row the need names is measured for this gun (kNeedPad and kNeedMouse alike: both asked, both wanted); the first that isn't, otherwise.
inline bool Ready(const Gun& g, uint32_t need, int* missing = nullptr) {
    for (int i = 0; i < kRowCount; ++i) {
        if (!(kRowKinds[i].need & need))
            continue;
        const Measure& m = g.m[i];
        const bool have = m.measured && (kRowKinds[i].count ? std::isfinite(m.value[0]) : m.text != nullptr && m.text[0] != 0);
        if (!have) {
            if (missing)
                *missing = i;
            return false;
        }
    }
    if (missing)
        *missing = -1;
    return true;
}

// ------------------------------------------------------------------ the geometry ------------------------------------------------------------------

struct Geometry {
    V3 pivot;          // p: the pivot from the ring's axis point, the ring's frame
    V3 camera;         // c: the camera from the pivot, the gun's frame
    V3 handle[2];      // each handle's grip point from the pivot, the gun's frame (0 left, 1 right)
    V3 eye;            // e: the gunner's eye from the ring's axis point, the ring's frame
    float pitchMin = -0.35f, pitchMax = 0.35f;  // radians
};

inline V3 RowV3(const Measure& m) { return {m.value[0], m.value[1], m.value[2]}; }

// The geometry from a gun's measured rows; false while any of them isn't measured.
inline bool GeometryOf(const Gun& g, Geometry& out) {
    const int rows[] = {kRowPivot, kRowCamera, kRowHandleLeft, kRowHandleRight, kRowEye, kRowPitchLimits};
    for (int r : rows)
        if (!g.m[r].measured)
            return false;
    out.pivot = RowV3(g.m[kRowPivot]);
    out.camera = RowV3(g.m[kRowCamera]);
    out.handle[0] = RowV3(g.m[kRowHandleLeft]);
    out.handle[1] = RowV3(g.m[kRowHandleRight]);
    out.eye = RowV3(g.m[kRowEye]);
    out.pitchMin = g.m[kRowPitchLimits].value[0] * kDeg;
    out.pitchMax = g.m[kRowPitchLimits].value[1] * kDeg;
    return true;
}

// A point of the gun (q from the pivot, the gun's frame) with the gun at yaw and pitch, the ring's axis point at ring (the world).
inline V3 PointAt(const Geometry& g, V3 ring, V3 q, float yaw, float pitch) { return ring + Turn(yaw, g.pivot + Tilt(pitch, q)); }
inline V3 HandleAt(const Geometry& g, V3 ring, int handle, float yaw, float pitch) { return PointAt(g, ring, g.handle[handle & 1], yaw, pitch); }
inline V3 CameraAt(const Geometry& g, V3 ring, float yaw, float pitch) { return PointAt(g, ring, g.camera, yaw, pitch); }
// The ring's axis point from the game's camera (its place, the way it looks): exact while the camera rides the pitch body rigidly. Taken once, at the
// climb-in's end (BodyAtClimbEnd): the camera leans by up to 10 cm with the turret's own input (LeftCurve, UpCurve), so taken every frame the eye would
// wobble with the hands (the review's eighth finding).
inline V3 RingFromCamera(const Geometry& g, V3 camera, float yaw, float pitch) { return camera - Turn(yaw, g.pivot + Tilt(pitch, g.camera)); }

// ------------------------------------------------------------------ the seat's states ------------------------------------------------------------------

enum class State : uint8_t {
    kOnFoot,     // no seat
    kOtherSeat,  // a seat no row is told for, or one whose rows aren't all measured: today's behaviour, nothing changed
    kClimbIn,    // a gun's seat, its climb-in (the game's scene): the cutscene rules, the seat's quiet list on
    kGun,        // a gun's seat, the gun the hands'
};
inline const char* StateName(State s) {
    return s == State::kOnFoot ? "on foot" : s == State::kOtherSeat ? "another seat (as before)" : s == State::kClimbIn ? "the gun's climb-in" : "on the gun";
}
inline bool InSeat(State s) { return s == State::kClimbIn || s == State::kGun; }

// A frame's read of the seat (src/seatread.h, through src/mountseat.cpp).
struct SeatRead {
    bool known = false;    // the reader is borne out and read this frame (false: nothing known, taken as on foot)
    bool vehicle = false;  // the player sits in a vehicle's entry
    int entry = -1;
    const char* name = nullptr;  // the vehicle's name as read (nullptr: not read)
};
// The game's camera this frame as the game made it, before the mod turns it.
struct CameraNow {
    bool fresh = false;  // made within the last 100 ms
    float yaw = 0.0f, pitch = 0.0f;  // radians
    V3 place;
};

inline bool SameText(const char* a, const char* b) {
    if (!a || !b)
        return false;
    for (; *a && *b; ++a, ++b) {
        const char ca = *a >= 'A' && *a <= 'Z' ? static_cast<char>(*a + 32) : *a, cb = *b >= 'A' && *b <= 'Z' ? static_cast<char>(*b + 32) : *b;
        if (ca != cb)
            return false;
    }
    return *a == *b;
}

// The gun a seat is: its row's entry and vehicle name both measured and both the seat's (-1: none). A seat whose gun's rows can't tell it is no gun's.
inline int MatchGun(const Gun* guns, int count, const SeatRead& r) {
    if (!r.known || !r.vehicle)
        return -1;
    for (int i = 0; i < count; ++i) {
        const Measure& entry = guns[i].m[kRowEntry];
        const Measure& name = guns[i].m[kRowVehicle];
        if (entry.measured && name.measured && static_cast<int>(entry.value[0]) == r.entry && SameText(name.text, r.name))
            return i;
    }
    return -1;
}

struct SeatRules {
    float stillDegrees = 0.05f;  // the camera still: turned less than this a frame
    float stillMetres = 0.002f;  // ... and moved less than this
    double stillFor = 0.5;       // still this long ends the climb-in
    double least = 1.0;          // ... no sooner than this after the seat appeared
    double most = 6.0;           // ... and at this, still or not
    // What the seat needs ready beside the rows (src/mountseat.cpp's): the steering's way (kNeedPad or kNeedMouse), and whether its own view and outputs
    // are built (step 1's runtime: false until they are, and then no seat is ever a gun's).
    uint32_t way = kNeedMouse;
    bool built = false;
};

struct Seat {
    State state = State::kOnFoot;
    int gun = -1;        // the gun told (kGuns' index), -1 none
    int missing = -1;    // a gun told but kept out: the first row of it not measured (-1: none, or the runtime not built)
    int seen = -1;       // the gun a seat was told for even while kept out (for the log), -1 none
    double since = 0.0;  // when this seat appeared (s)
    double stillSince = -1.0;
    bool haveLast = false;
    float lastYaw = 0.0f, lastPitch = 0.0f;
    V3 lastPlace;
    bool changed = false;  // this step changed the state
    bool climbEnded = false;  // this step ended the climb-in (the body takes the camera's yaw now: BodyAtClimbEnd)
};

// Each frame: the seat's state from its read and the game's camera. Only a seat told for a gun whose every row the seat needs is measured, with the
// runtime built, is ever the gun's (the climb-in, then the gun); every other seat is another seat, today's behaviour exactly; no seat, on foot. A seat
// gone (the reader's pointer gone: a pull-out, a death, a load) ends the gun's at once.
inline void SeatStep(Seat& s, const SeatRead& r, const CameraNow& c, double now, const Gun* guns, int count, const SeatRules& rules) {
    const State was = s.state;
    s.changed = false;
    s.climbEnded = false;
    if (!r.known || !r.vehicle) {
        s.state = State::kOnFoot;
        s.gun = -1, s.seen = -1, s.missing = -1;
        s.haveLast = false;
        s.stillSince = -1.0;
        s.changed = s.state != was;
        return;
    }
    const int gun = MatchGun(guns, count, r);
    const bool fresh = was == State::kOnFoot || gun != s.seen;
    if (fresh)
        s.since = now, s.stillSince = -1.0, s.haveLast = false;
    s.seen = gun;
    int missing = -1;
    const bool ready = gun >= 0 && Ready(guns[gun], kNeedMatch | kNeedView | kNeedSteer | rules.way, &missing);
    s.missing = gun >= 0 ? missing : -1;
    if (gun < 0 || !ready || !rules.built) {
        s.state = State::kOtherSeat;
        s.gun = -1;
        s.changed = s.state != was;
        return;
    }
    s.gun = gun;
    if (!InSeat(was) || fresh) {
        s.state = State::kClimbIn;
    } else if (was == State::kClimbIn) {
        // The climb-in ends when the game's camera has kept still a while, no sooner than `least` after the seat came and at `most` whatever it does.
        bool still = false;
        if (c.fresh && s.haveLast) {
            const float turned = std::fabs(Wrap(c.yaw - s.lastYaw)) + std::fabs(c.pitch - s.lastPitch);
            still = turned < rules.stillDegrees * kDeg && Length(c.place - s.lastPlace) < rules.stillMetres;
        }
        if (!still)
            s.stillSince = -1.0;
        else if (s.stillSince < 0.0)
            s.stillSince = now;
        const double in = now - s.since;
        if ((in >= rules.least && s.stillSince >= 0.0 && now - s.stillSince >= rules.stillFor) || in >= rules.most) {
            s.state = State::kGun;
            s.climbEnded = true;
        }
    }
    if (c.fresh) {
        s.lastYaw = c.yaw, s.lastPitch = c.pitch, s.lastPlace = c.place;
        s.haveLast = true;
    }
    s.changed = s.state != was;
}

// ------------------------------------------------------------------ the quiet list ------------------------------------------------------------------

// What the mod does that the seat changes (4.8). Each gate asks Quiet with its own; on foot and in any other seat nothing is quiet.
enum What : int {
    kLookWrites,         // aim.cpp: the look angles written (they aim nothing in the seat: LockSoldierAimingToEntry)
    kLookSearch,         // aim.cpp: the search and its nudges of candidates (a nudge in a vehicle is an unknown)
    kMouseAim,           // aim.cpp: the view steered by mouse moves while the look isn't found (the mouse turns the ring in the seat)
    kStickThroughMouse,  // input.cpp: the right stick's turn sent as mouse X by its thread (the ring's own turn in the seat): the turn goes to the body
    kStance,             // input.cpp: the stick's crouch and prone (X is the seat's countermeasures; the level locks the stance)
    kJump,               // input.cpp: the jump button and the stick's click
    kCrouchKey,          // input.cpp: the head's real crouch holding Left Ctrl
    kSprintKey,          // input.cpp: the left stick's click (Left Shift)
    kWalk,               // input.cpp, pad.cpp: W/A/S/D and the pad's left stick (the seat's throttle and yaw)
    kGrenadeKey,         // input.cpp: G
    kWeaponKeys,         // input.cpp: the picks, 1, 2 and 4 (they select the vehicle's weapons), Y held's swap
    kReloadKey,          // input.cpp: X (its tap, its hold's R) and the D-pad's eject
    kZoom,               // input.cpp: the left trigger's right mouse button and the "zoom 1" test (it would narrow the headset's picture to 22 degrees)
    kFreeHands,          // hands.cpp: the free hands, the held guns and everything they work (the game's own arms drawn as it draws them)
    kHolsters,           // holster.h through hands.cpp: no draws, no put-aways, no keys 1, 2 or 4
    kGrenadesByHand,     // hands.cpp: the grenades at the chest
    kMagazines,          // hands.cpp, handmag.cpp, pouch.cpp: magazines and reloads by hand, the pouch
    kQuickTimeGrips,     // input.cpp's QteStep for qtewatch.cpp: no scripted moment's presses; a fist fight's open prompt still answered, as in play
    kFire,               // never quiet: the seat's own rule (FireStep) takes the trigger
    kUse,                // never quiet: the use button (get off where a level allows it)
    kRecenter,           // never quiet: the right stick's click held (the room's front at the body's, the head at the eye)
    kWhatCount
};
inline const char* WhatName(int w) {
    static const char* const kNames[kWhatCount] = {"the look's writes", "the look's search", "mouse aim", "the stick's turn through the mouse",
                                                   "the stick's stance", "the jump", "the head's crouch key", "the sprint key", "walking",
                                                   "the grenade key", "the weapon keys", "X and the D-pad's eject", "the zoom", "the free hands",
                                                   "the holsters", "grenades by hand", "magazines by hand", "the grips' quick-time presses",
                                                   "the fire button", "the use button", "the recenter"};
    return w >= 0 && w < kWhatCount ? kNames[w] : "?";
}
// In the seat (its climb-in and the gun) everything but the fire, the use button and the recenter: one gate on every mapping (the review's thirteenth).
inline bool Quiet(State s, What w) {
    if (!InSeat(s))
        return false;
    return w != kFire && w != kUse && w != kRecenter;
}
// The right stick's turn this frame: into the frame's own turn (on foot the look yaw it writes, in the seat the body) or through the mouse thread
// (input.cpp's UpdateTurn). In the seat always the first: the mouse's X turns the ring there, and the stick through it would fight the steering and turn
// the gun in the climb-in (the review's first high finding: with the look writes quiet, the stick went to the mouse thread).
inline bool StickDirect(State s, bool onFootDirect) { return InSeat(s) || onFootDirect; }

// ------------------------------------------------------------------ the hands on the handles ------------------------------------------------------------------

// A hand's hold of a handle (4.3). Step 1: a hand holds while its grip is squeezed (the game's own arms are drawn on the gun; there is no reach to take a
// handle by yet). Step 2: a fresh squeeze within reach takes one, past kLetGo from it lets go.
struct Hold {
    bool on = false;
    int handle = -1;
    V3 offset;  // the hand from its handle at its take, in the holding frame (Holds: the hands' line, or the body): turned with that frame after
};
constexpr float kReach = 0.12f;      // step 2: a squeeze takes a handle within this (m)
constexpr float kLetGo = 0.25f;      // step 2: a hand this far from its handle (its offset taken off first) lets go (m)
constexpr float kPairLeast = 0.08f;  // two holding hands closer than this across the ground: their line's heading is unsure, the body's frame instead

// Both hands' holds and the frame their offsets turn with (the review's fifth finding, and a torso's turn): while both hold, apart, the line between them
// (from the left to the right, across the ground: a seated player's torso turned on the Humvee, "hands and all", turns the offsets with the hands; a
// wrist's twist doesn't move it), else the body's yaw (the stick's turn: the room turns about the ring's axis and the offsets with it). A change of frame
// (a hand taking hold or letting go, the hands coming together) puts every holding hand's offset into the new one with its wanted place where it was.
struct Holds {
    Hold hold[2];
    bool pair = false;
    float heading = 0.0f;  // the frame's heading as of the last step
    V3 spread[2];          // with the pair: each held handle from the handles' middle as the pair formed, in its frame (the handles' own spacing)
    bool was[2] = {};      // each grip squeezed at the last step (step 2's fresh squeezes)
};

inline bool PairHeading(const Hold hold[2], const V3 hands[2], float& heading) {
    if (!(hold[0].on && hold[1].on && Finite(hands[0]) && Finite(hands[1])))
        return false;
    const V3 d = hands[1] - hands[0];
    if (std::sqrt(d.x * d.x + d.z * d.z) < kPairLeast)
        return false;
    heading = std::atan2(-d.z, -d.x);  // (the yaw whose right, Turn(yaw, (-1, 0, 0)), lies along the line)
    return true;
}

// A hand closes (0 left, 1 right): the handle nearest it with the gun where the game has it, the other's if the nearest is held; its offset from it kept
// in the frame of the given heading, so the gun never jumps as a hand closes and a player whose arms aren't the gunner's moves it as far as his hands move.
inline void Take(Hold holds[2], int hand, V3 handWorld, const Geometry& g, V3 ring, float gameYaw, float gamePitch, float heading) {
    Hold& h = holds[hand & 1];
    const Hold& other = holds[(hand & 1) ^ 1];
    const V3 at[2] = {HandleAt(g, ring, 0, gameYaw, gamePitch), HandleAt(g, ring, 1, gameYaw, gamePitch)};
    int pick = Length(handWorld - at[0]) <= Length(handWorld - at[1]) ? 0 : 1;
    if (other.on && other.handle == pick)
        pick ^= 1;
    h.on = true;
    h.handle = pick;
    h.offset = Turn(-heading, handWorld - at[pick]);
}
inline void LetGo(Hold& h) { h = Hold{}; }
// Where a holding hand wants its handle: the hand less its take's offset, turned with its frame.
inline V3 Wanted(const Hold& h, V3 handWorld, float heading) { return handWorld - Turn(heading, h.offset); }
// How far a holding hand is from its handle as the gun is (step 2's let-go): after its take's offset is taken off (a raw distance let go of the Humvee's
// handles mid-turn, the ring's axis 0.18 m in front of the eye and not at the spine: the review's fifth finding).
inline float Apart(const Hold& h, V3 handWorld, float heading, const Geometry& g, V3 ring, float yaw, float pitch) {
    return Length(Wanted(h, handWorld, heading) - HandleAt(g, ring, h.handle, yaw, pitch));
}

struct HandsNow {
    V3 at[2];  // each hand in the world
    bool tracked[2] = {};
    bool squeezed[2] = {};
};
struct HoldRules {
    float reach = 0.0f;  // step 2: kReach (a take only within it, by a fresh squeeze); 0 step 1: any squeeze holds
    float letGo = 0.0f;  // step 2: kLetGo; 0 step 1: only the grip's release (or the hand lost) lets go
};
struct HoldEvents {
    bool took[2] = {}, letGo[2] = {};
};

// Each frame on the gun: let-goes (the grip released, the hand not tracked, step 2's distance), takes, the frame kept and changed with its offsets'
// places kept. gameYaw and gamePitch: the game's gun now (its camera); bodyYaw: the body's.
inline HoldEvents HoldStep(Holds& h, const HandsNow& n, const Geometry& g, V3 ring, float gameYaw, float gamePitch, float bodyYaw,
                           const HoldRules& rules = HoldRules{}) {
    HoldEvents e;
    float pairNow = 0.0f;
    const float before = h.pair ? (PairHeading(h.hold, n.at, pairNow) ? pairNow : h.heading) : bodyYaw;
    for (int i = 0; i < 2; ++i)
        if (h.hold[i].on && (!n.squeezed[i] || !n.tracked[i] || !Finite(n.at[i]) ||
                             (rules.letGo > 0.0f && Apart(h.hold[i], n.at[i], before, g, ring, gameYaw, gamePitch) > rules.letGo))) {
            LetGo(h.hold[i]);
            e.letGo[i] = true;
        }
    for (int i = 0; i < 2; ++i) {
        const bool fresh = n.squeezed[i] && !h.was[i];
        h.was[i] = n.squeezed[i];
        if (h.hold[i].on || !n.squeezed[i] || !n.tracked[i] || !Finite(n.at[i]))
            continue;
        if (rules.reach > 0.0f) {
            const float nearest = std::fmin(Length(n.at[i] - HandleAt(g, ring, 0, gameYaw, gamePitch)), Length(n.at[i] - HandleAt(g, ring, 1, gameYaw, gamePitch)));
            if (!fresh || nearest > rules.reach)
                continue;
        }
        Take(h.hold, i, n.at[i], g, ring, gameYaw, gamePitch, before);
        e.took[i] = true;
    }
    float pairHeading = 0.0f;
    const bool pair = PairHeading(h.hold, n.at, pairHeading);
    const float heading = pair ? pairHeading : bodyYaw;
    if (pair != h.pair) {
        for (Hold& x : h.hold)
            if (x.on)
                x.offset = Turn(-heading, Turn(before, x.offset));
        if (pair) {
            // The handles' own spacing as they are now (the game's gun), in the pair's frame: the hands' spread and roll never stretch or roll it.
            const V3 a = HandleAt(g, ring, h.hold[0].handle, gameYaw, gamePitch), b = HandleAt(g, ring, h.hold[1].handle, gameYaw, gamePitch);
            const V3 middle = (a + b) * 0.5f;
            h.spread[0] = Turn(-heading, a - middle);
            h.spread[1] = Turn(-heading, b - middle);
        }
    }
    h.pair = pair;
    h.heading = heading;
    return e;
}

// One Gauss-Newton step of the gun's (yaw, pitch) toward the wanted handle places (n of them).
inline void FitStep(const Geometry& g, V3 ring, const int* handles, const V3* wanted, int n, float& yaw, float& pitch) {
    float a = 0.0f, b = 0.0f, d = 0.0f, ry = 0.0f, rp = 0.0f;
    for (int i = 0; i < n; ++i) {
        const V3 tilted = Tilt(pitch, g.handle[handles[i] & 1]);
        const V3 v = g.pivot + tilted;
        const V3 jy = Turn(yaw, {-v.z, 0.0f, v.x});            // d/dyaw
        const V3 jp = Turn(yaw, {0.0f, tilted.z, -tilted.y});  // d/dpitch
        const V3 r = wanted[i] - (ring + Turn(yaw, v));
        a += Dot(jy, jy), b += Dot(jy, jp), d += Dot(jp, jp);
        ry += Dot(jy, r), rp += Dot(jp, r);
    }
    const float damp = 1e-6f + 1e-3f * (a + d);
    a += damp, d += damp;
    const float det = a * d - b * b;
    if (!(std::fabs(det) > 1e-12f))
        return;
    float dy = (d * ry - b * rp) / det, dp = (a * rp - b * ry) / det;
    const float step = std::sqrt(dy * dy + dp * dp);
    if (step > 0.5f)  // (half a radian a step at most: far answers come in a few frames)
        dy *= 0.5f / step, dp *= 0.5f / step;
    if (std::isfinite(dy) && std::isfinite(dp))
        yaw = Wrap(yaw + dy), pitch += dp;
}

// The gun's yaw and pitch that put the held handles nearest where their hands want them (4.4). Both held: the handles' middle where the hands' middle
// wants it, their line turned as the hands' line turns, their own spacing kept (the hands' spread and roll move nothing the gun can't do; the owner's
// "at the same directions, like mirroring each other": hands moved together move the middle), fitted at once: the tilt from the handles' height, the turn
// from their places about the ring's axis (closed form), then one Gauss-Newton step on both. One held: its handle at its hand, two steps from the last
// answer. The pitch inside the gun's limits. From the last answer (yaw, pitch), which it overwrites; false with no hand holding (they're left as they were).
inline bool Solve(const Geometry& g, V3 ring, const Holds& holding, const V3 hands[2], float& yaw, float& pitch) {
    const Hold* holds = holding.hold;
    const float heading = holding.heading;
    int handles[2] = {}, n = 0, which[2] = {};
    V3 wanted[2];
    for (int i = 0; i < 2; ++i)
        if (holds[i].on && Finite(hands[i])) {
            handles[n] = holds[i].handle;
            which[n] = i;
            wanted[n] = Wanted(holds[i], hands[i], heading);
            ++n;
        }
    if (!n || !std::isfinite(yaw) || !std::isfinite(pitch))
        return false;
    if (n == 2 && holding.pair) {
        const V3 middle = (wanted[0] + wanted[1]) * 0.5f;
        for (int k = 0; k < 2; ++k)
            wanted[k] = middle + Turn(heading, holding.spread[which[k]]);
    }
    if (n == 2 && handles[0] != handles[1]) {
        // The tilt: the handles' middle at the wanted places' height, m.y cos(pitch) + m.z sin(pitch) = t, the root nearest the last pitch.
        const V3 m = (g.handle[0] + g.handle[1]) * 0.5f;
        const float t = (wanted[0].y + wanted[1].y) * 0.5f - ring.y - g.pivot.y;
        const float rr = std::sqrt(m.y * m.y + m.z * m.z);
        if (rr > 1e-4f) {
            const float phi = std::atan2(m.z, m.y), across = std::acos(Clamp(t / rr, -1.0f, 1.0f));
            const float p1 = Wrap(phi + across), p2 = Wrap(phi - across);
            pitch = std::fabs(Wrap(p1 - pitch)) <= std::fabs(Wrap(p2 - pitch)) ? p1 : p2;
        }
        pitch = Clamp(pitch, g.pitchMin, g.pitchMax);
        // The turn: the handles about the ring's axis onto the wanted places about it (a rotation in the ground's plane, least squares; the ground's plane as
        // the complex number z - i x, which the yaw turns by e^(i yaw)).
        float re = 0.0f, im = 0.0f;
        for (int i = 0; i < 2; ++i) {
            const V3 v = g.pivot + Tilt(pitch, g.handle[handles[i]]);
            const V3 u = wanted[i] - ring;
            const float a = u.z, b = -u.x, c = v.z, dd = -v.x;
            re += a * c + b * dd;
            im += b * c - a * dd;
        }
        if (re * re + im * im > 1e-10f)
            yaw = std::atan2(im, re);
        FitStep(g, ring, handles, wanted, n, yaw, pitch);
    } else {
        FitStep(g, ring, handles, wanted, n, yaw, pitch);
        FitStep(g, ring, handles, wanted, n, yaw, pitch);
    }
    pitch = Clamp(pitch, g.pitchMin, g.pitchMax);
    return std::isfinite(yaw) && std::isfinite(pitch);
}

// The target's smoothing (4.4): a 1 cm wobble of a hand at a 0.3 m lever is 2 degrees of gun. A low-pass that opens with the target's own speed: steady
// while the hands are slow, no lag to speak of when they sweep.
struct Smooth {
    bool have = false;
    float value = 0.0f, speed = 0.0f;
};
struct SmoothRules {
    float slowHz = 1.5f;   // the cutoff with the target still
    float perRadian = 2.0f;  // ... opened this much a radian a second of its speed
    float speedHz = 1.0f;  // the speed's own cutoff
};
inline float SmoothStep(Smooth& f, float x, float dt, bool wrap, const SmoothRules& r = SmoothRules{}) {
    if (!f.have || !(dt > 0.0f) || !std::isfinite(x)) {
        f.have = std::isfinite(x);
        f.value = x, f.speed = 0.0f;
        return x;
    }
    const auto alpha = [dt](float hz) {
        const float tau = 1.0f / (2.0f * kPi * hz);
        return 1.0f / (1.0f + tau / dt);
    };
    const float step = wrap ? Wrap(x - f.value) : x - f.value;
    f.speed += (step / dt - f.speed) * alpha(r.speedHz);
    const float hz = r.slowHz + r.perRadian * std::fabs(f.speed);
    f.value += step * alpha(hz);
    if (wrap)
        f.value = Wrap(f.value);
    return f.value;
}

// ------------------------------------------------------------------ the body, the eye, the stick ------------------------------------------------------------------

// The body in the seat (4.2): its yaw (the game's sense) and the ring's axis point, fixed to the vehicle. Both vehicles stand still while the player is
// on the gun (the truck moves only in the last 0.3 s before its pull-out), so the world is the vehicle's.
struct Body {
    bool have = false;
    float yaw = 0.0f;
    V3 ring;
};
// At the climb-in's end: the body takes the game camera's yaw (nothing moves), the ring's point taken once from the camera.
inline void BodyAtClimbEnd(Body& b, const Geometry& g, const CameraNow& c) {
    b.ring = RingFromCamera(g, c.place, c.yaw, c.pitch);
    b.yaw = c.yaw;
    b.have = Finite(b.ring) && std::isfinite(b.yaw);
}
// The eye: the gunner's place on the ring with the ring facing the body's yaw.
inline V3 EyeAt(const Body& b, const Geometry& g) { return b.ring + Turn(b.yaw, g.eye); }
// A point in the room (OpenXR's: x right, y up, z back) into the world: the room's straight ahead along the body's front, its centre (where the head
// was at the recenter) at the eye. The stick's turn of the body turns the room about the ring's axis with it.
inline V3 RoomToWorld(const Body& b, const Geometry& g, V3 room, V3 roomCentre) {
    const V3 d = room - roomCentre;
    return EyeAt(b, g) + Turn(b.yaw, {-d.x, d.y, -d.z});
}

// The stick's turn in the seat (4.5): the body about the ring's axis, so the handles stay in the hands and the gun turns with it (on the Humvee a turn in
// place, on the truck an orbit about the post). The turn the stick asks this frame (radians, the game's sense: right positive, as on foot at its speed)
// cut so the eye moves at most eyeSpeed along its circle: on the truck the eye stands 0.63 m from the post, and the on-foot 266 degrees a second would
// carry it sideways at 2.9 m/s (the review's sixth finding); at 0.75 m/s the truck turns 68 degrees a second, the Humvee (0.18 m) its on-foot speed.
constexpr float kOrbitEyeSpeed = 0.75f;
inline float OrbitTurn(float turn, float dt, const Geometry& g, float eyeSpeed = kOrbitEyeSpeed) {
    if (!std::isfinite(turn) || !(dt > 0.0f))
        return 0.0f;
    const float radius = std::sqrt(g.eye.x * g.eye.x + g.eye.z * g.eye.z);
    if (radius < 1e-3f)
        return turn;
    const float most = eyeSpeed / radius * dt;
    return Clamp(turn, -most, most);
}

// "mountpush 1" (off by default: players first, the owner's word wanted): the body turning by itself while the hands hold the gun past startDegrees from
// the body's front, faster the further past, at most mostPerSecond.
struct PushRules {
    bool on = false;
    float startDegrees = 50.0f, perDegree = 2.0f, mostPerSecond = 60.0f;
};
inline float PushTurn(float gunYaw, float bodyYaw, float dt, const PushRules& r) {
    if (!r.on || !(dt > 0.0f))
        return 0.0f;
    const float off = Wrap(gunYaw - bodyYaw) / kDeg, past = std::fabs(off) - r.startDegrees;
    if (past <= 0.0f)
        return 0.0f;
    const float rate = std::fmin(past * r.perDegree, r.mostPerSecond);
    return std::copysign(rate * kDeg * dt, off);
}

// ------------------------------------------------------------------ steering the game's turret ------------------------------------------------------------------

// One axis of the turret as its rows have it (degrees and seconds). most: its rate at the input's most (the way's: the pad's full stick, the mouse's
// most counts a frame), lag: its time to speed (its momentum), delay: an input to the first turn, least: the least input that moves it (a fraction of
// the most), sign: which way the input turns it.
struct AxisRows {
    float most = 60.0f, lag = 0.1f, delay = 0.05f, least = 0.0f, sign = 1.0f;
};
struct SteerRules {
    float deadband = 0.15f;       // degrees: an error under this, nothing sent (4.6)
    float horizonTicks = 3.0f;    // the time the law means to close the stopping point's gap in: at least this many game ticks...
    float horizonLags = 1.5f;     // ... and this many of the turret's lags (with its rows off by twice, its coast still braked for)
    float tick = 1.0f / 30.0f;    // the game's tick (s)
    float targetHz = 10.0f;       // the target's own rate's low-pass (Hz; the hands' target comes through SmoothStep already)
    float rateSpan = 0.1f;        // the camera's measured rate: its turn over about this long (s; the camera steps at the game's ticks, so a frame's step is no rate)
    float modelPull = 5.0f;       // the model's rate pulled toward the measured this much a second (a stop it didn't make, a limit, an input of someone else's)
    float brakeTicks = 1.0f;      // the stopping point past the target while the turret turns toward it: the gap braked away over a tick, as hard as the input allows
    float wrongWayDegrees = 1.0f; // turned the other way this far with the input pushing hard: a wrong sign
    float stillDegrees = 0.1f;    // turned less than this with the input pushing hard: no answer
    float hardPush = 0.25f;       // a push this fraction of the most counts for the guards (a limit met a little early never stops the steering)
    float guardSeconds = 0.1f;    // a wrong sign told this long past the turret's delay and half its lag; no answer 0.5 s later
    float learnLeast = 0.15f;     // the strength learned only from a push one way held at least this fraction of the most...
    float learnWindow = 0.25f;    // ... over windows this long, each once the push has lasted the turret's delay and three lags (never the coast after one)
    float learnLow = 0.5f, learnHigh = 2.0f;  // the learned strength within half and twice the row's
};
enum class SteerStop : uint8_t { kNone, kWrongWay, kNoAnswer };
inline const char* SteerStopName(SteerStop s) {
    return s == SteerStop::kWrongWay ? "the turret turned the other way from the input (a sign wrong: the game's vehicle invert?)"
           : s == SteerStop::kNoAnswer ? "the turret didn't answer the input (the game not reading it?)"
                                       : "steering";
}
constexpr int kSentMost = 96;  // the rates asked, kept for the stopping point (at 90 frames a second, a little over a second)

constexpr int kSeenMost = 16;  // the camera's last turns kept for its measured rate (each frame it moved)

struct AxisState {
    bool have = false;
    double at = 0.0;          // the last frame's time
    float angle = 0.0f;       // the game's angle, unwrapped (degrees)
    float rate = 0.0f;        // its rate as measured: its turn over the last rateSpan (degrees a second)
    double seenAt[kSeenMost] = {};
    float seen[kSeenMost] = {};
    int seenNext = 0, seenCount = 0;
    float model = 0.0f;       // its rate as the rates asked make it, through the delay and the lag, kept honest by the measured one
    bool haveTarget = false;
    float target = 0.0f, targetRate = 0.0f;  // the target unwrapped near the angle, and its own rate
    double sentAt[kSentMost] = {};
    float sent[kSentMost] = {};
    int sentNext = 0, sentCount = 0;
    float gain = 1.0f;        // the learned strength against the row's (learnLow to learnHigh)
    int learned = 0;          // windows learned from
    double steadySince = -1.0;  // pushed one way, strongly enough to learn from, since (-1: not)
    float steadySign = 0.0f;
    bool window = false;      // a learning window open
    double windowAt = 0.0;
    float windowFrom = 0.0f;
    float windowSent = 0.0f;  // the turn the rates asked in it ask for, as the turret acts on them (its delay after)
    float targetAcc = 0.0f;   // the target's own change of rate (degrees a second a second)
    double pushSince = -1.0;  // pushed hard one way since (-1: not)
    float pushFrom = 0.0f, pushSign = 0.0f;
    bool pushWatched = false;  // the push's watch begun (past the turret's delay and half its lag)
    float watchFrom = 0.0f, watchRate = 0.0f;
    double leastUntil = -1.0;  // the least input held till then once begun
    float leastSign = 0.0f;
    SteerStop stop = SteerStop::kNone;
};

// The rate asked at a moment (the newest asked no later than it; 0 before any).
inline float SentAt(const AxisState& s, double when) {
    for (int k = 0; k < s.sentCount; ++k) {
        const int i = (s.sentNext - 1 - k + kSentMost) % kSentMost;
        if (s.sentAt[i] <= when)
            return s.sent[i];
    }
    return 0.0f;
}

// Where the turret would come to rest if nothing more were asked from now: the rates asked over the last delay still arriving, then its momentum's coast
// (the lag). Degrees, as the game's angle (unwrapped).
inline float StopPoint(const AxisState& s, const AxisRows& r, double now) {
    float angle = s.angle, rate = s.model;
    const float lag = r.lag > 1e-3f ? r.lag : 1e-3f;
    if (r.delay > 0.0f) {
        constexpr int kSteps = 8;
        const float dt = r.delay / kSteps;
        const float pull = 1.0f - std::exp(-dt / lag);
        for (int k = 0; k < kSteps; ++k) {
            rate += (SentAt(s, now - r.delay + (k + 0.5f) * dt) - rate) * pull;
            angle += rate * dt;
        }
    }
    return angle + rate * lag;
}

// The strength learning: over a window of a sustained push, the turret's turn against the turn the rates asked for as it acted on them (the strength
// already in them); the learned strength moves a quarter of the way to what it says, kept within the rules' bounds of the row's.
inline void Learn(AxisState& s, float askedTurn, float turned, const SteerRules& rules = SteerRules{}) {
    if (!(std::fabs(askedTurn) > 0.05f) || !std::isfinite(turned) || !std::isfinite(askedTurn))
        return;
    const float ratio = turned / askedTurn;
    if (!(ratio > 0.0f))
        return;
    const float wanted = Clamp(s.gain * ratio, rules.learnLow, rules.learnHigh);
    s.gain += (wanted - s.gain) * 0.25f;
    ++s.learned;
}

struct AxisOut {
    float rate = 0.0f;   // the rate asked of the turret (degrees a second, the game's sense): 0 sends nothing
    float ask = 0.0f;    // ... at the row's strength, signed for the input (the row's sign applied): what the input's table or counts are made from
    float input = 0.0f;  // ... as a fraction of the input's most (for the log)
    bool sent = false;
};

// Each frame for one axis: the target (the hands' gun, degrees), the game's angle (its camera's, degrees), the time (s). wrap: a yaw (both taken round the
// circle). allowed: false asks nothing (no hand on a handle and no stick turn, the climb-in, a stale camera, the game's window behind for the mouse): the
// input goes back to rest at once, nothing left standing. The law: the rate that closes the gap between the target and where the turret would come to
// rest (StopPoint) over a horizon of a few ticks and its lag, plus the target's own rate (a steady sweep followed with no standing lag), cut to the
// turret's most. With its rows right the stopping point brakes for the momentum: no overshoot (for a first-order turret the law's roots are -1/lag and
// -1/horizon, both real; the delay is the stopping point's to carry). Nothing inside the deadband. Under the least input that moves it, the least only
// while the gap is more than the least's own coast (a standing error under that, never a hunt). Pushed hard one way, the turret going the other (a sign
// wrong) past its own delay and half its lag, or nowhere (not read) a while longer: the axis stops for good (stop) and asks nothing more.
inline AxisOut SteerAxis(AxisState& s, const AxisRows& r, float target, float game, double now, bool wrap, bool allowed,
                         const SteerRules& rules = SteerRules{}) {
    AxisOut out;
    if (!std::isfinite(game) || !std::isfinite(target) || !std::isfinite(now) || (now < s.at && s.have)) {
        allowed = false;
        if (!std::isfinite(game))
            game = s.angle;
        if (!std::isfinite(target))
            target = s.target;
        if (!std::isfinite(now) || now < s.at)
            now = s.at;
    }
    float dt = s.have ? static_cast<float>(now - s.at) : 0.0f;
    if (!(dt > 0.0f) || dt > 0.25f)
        dt = 0.0f;
    const float lag = r.lag > 1e-3f ? r.lag : 1e-3f;
    // The game's angle (unwrapped) and its measured rate; the model's rate from the rates asked, through the delay and the lag.
    const auto saw = [&]() {
        s.seenAt[s.seenNext] = now;
        s.seen[s.seenNext] = s.angle;
        s.seenNext = (s.seenNext + 1) % kSeenMost;
        if (s.seenCount < kSeenMost)
            ++s.seenCount;
    };
    if (!s.have) {
        s.angle = game;
        s.rate = s.model = 0.0f;
        s.seenCount = 0;
        saw();
    } else {
        const float step = wrap ? WrapDegrees(game - s.angle) : game - s.angle;
        s.angle += step;
        if (step != 0.0f)
            saw();
        if (dt > 0.0f) {
            // The measured rate: the turn from the oldest place seen within the span to now (the camera moves at the game's ticks, two or three frames
            // apart); not moved for a tick and a half, at rest.
            const int newest = (s.seenNext - 1 + kSeenMost) % kSeenMost;
            if (now - s.seenAt[newest] > 1.5f * rules.tick) {
                s.rate = 0.0f;
            } else {
                int oldest = newest;
                for (int k = 1; k < s.seenCount; ++k) {
                    const int i = (s.seenNext - 1 - k + kSeenMost) % kSeenMost;
                    if (now - s.seenAt[i] > rules.rateSpan)
                        break;
                    oldest = i;
                }
                // (from the place before the oldest in the span, when there is one: the span's whole turn)
                const int before = (oldest - 1 + kSeenMost) % kSeenMost;
                const int from = s.seenCount > 1 && before != newest && s.seenAt[before] < s.seenAt[oldest] ? before : oldest;
                const double span = s.seenAt[newest] - s.seenAt[from];  // (between two of its moves: never sagging between ticks)
                if (from != newest && span > 1e-3)
                    s.rate = (s.seen[newest] - s.seen[from]) / static_cast<float>(span);
            }
            s.model += (SentAt(s, now - r.delay) - s.model) * (1.0f - std::exp(-dt / lag));
            s.model += (s.rate - s.model) * (1.0f - std::exp(-dt * rules.modelPull));
        }
    }
    s.have = true;
    s.at = now;
    // The target, unwrapped near the game's angle, and its own rate.
    const float t = wrap ? s.angle + WrapDegrees(target - s.angle) : target;
    if (s.haveTarget && dt > 0.0f) {
        const float moved = wrap ? WrapDegrees(t - s.target) : t - s.target;
        const float was = s.targetRate;
        s.targetRate += (moved / dt - s.targetRate) * (1.0f - std::exp(-dt * 2.0f * kPi * rules.targetHz));
        s.targetAcc += ((s.targetRate - was) / dt - s.targetAcc) * (1.0f - std::exp(-dt * 2.0f * kPi * rules.targetHz * 0.6f));
    } else {
        s.targetRate = s.targetAcc = 0.0f;
    }
    s.target = t;
    s.haveTarget = true;
    const auto ask = [&](float rate) {
        s.sentAt[s.sentNext] = now;
        s.sent[s.sentNext] = rate;
        s.sentNext = (s.sentNext + 1) % kSentMost;
        if (s.sentCount < kSentMost)
            ++s.sentCount;
    };
    if (!allowed || s.stop != SteerStop::kNone || !(r.most > 0.0f)) {
        ask(0.0f);
        s.steadySince = -1.0, s.window = false;
        s.pushSince = -1.0;
        if (!allowed)
            s.targetRate = s.targetAcc = 0.0f;
        return out;
    }
    const float most = r.most * s.gain;
    const float lead = r.delay + lag;
    const float error = t - s.angle;
    // Where the target will be when what's asked now has acted: on at its rate, with its own slowing (a hand stopping), never past where that slowing
    // stops it.
    float ahead = s.targetRate * lead + 0.5f * s.targetAcc * lead * lead;
    if (s.targetRate * s.targetAcc < 0.0f) {
        const float stops = s.targetRate * s.targetRate / (2.0f * std::fabs(s.targetAcc));
        ahead = s.targetRate > 0.0f ? Clamp(ahead, 0.0f, stops) : Clamp(ahead, -stops, 0.0f);
    }
    const float gap = t + ahead - StopPoint(s, r, now);
    // The horizon: a few ticks and the turret's lag. Braking (the stopping point past the target while the turret turns toward it: a hand stopping
    // sooner than the turret can coast to a stop) takes the gap away over a tick, as hard as the input allows: coasting from 30 degrees a second the
    // turret stops 6 degrees on, the full input the other way stops it in 2, and a hand stops in about 4.
    const bool braking = gap * s.model < 0.0f && error * s.model > -rules.deadband;
    float rate = braking ? gap / (rules.brakeTicks * rules.tick) : gap / std::fmax(rules.horizonTicks * rules.tick, rules.horizonLags * lag) + s.targetRate;
    if (std::fabs(error) < rules.deadband && std::fabs(gap) < rules.deadband && std::fabs(s.targetRate) * lead < rules.deadband)
        rate = 0.0f;
    rate = Clamp(rate, -most, most);
    // Under the least input that moves the turret (its dead zones, its momentum's threshold): the least, held for its delay and lag once begun (a pulse
    // shorter never gets its rate past the threshold, and the model would count a turn that never came), while the gap is more than the least's own coast;
    // else nothing: a standing error under that coast, never a hunt.
    const float least = Clamp(r.least, 0.0f, 1.0f) * most;
    if (rate != 0.0f && std::fabs(rate) < least) {
        const float sign = rate > 0.0f ? 1.0f : -1.0f;
        if (now < s.leastUntil && sign == s.leastSign && error * sign > 0.0f) {
            rate = sign * least;
        } else if (std::fabs(gap) > least * lead) {
            rate = sign * least;
            s.leastUntil = now + lead;
            s.leastSign = sign;
        } else {
            rate = 0.0f;
        }
    }
    // The guards, on hard pushes only.
    const float sign = std::fabs(rate) >= rules.hardPush * most ? (rate > 0.0f ? 1.0f : -1.0f) : 0.0f;
    if (sign == 0.0f || sign != s.pushSign) {
        s.pushSince = sign != 0.0f ? now : -1.0;
        s.pushFrom = s.angle;
        s.pushSign = sign;
        s.pushWatched = false;
    } else if (s.pushSince >= 0.0) {
        // Watched from the turret's delay and half its lag on: pushed one way, a wrong sign turns it faster and faster the other way (an overshoot's coast
        // against the push only slows); no answer, it doesn't turn at all.
        const double over = now - s.pushSince - r.delay - 0.5f * lag;
        if (over > 0.0 && !s.pushWatched) {
            s.pushWatched = true;
            s.watchFrom = s.angle;
            s.watchRate = s.rate * sign;
        }
        const float moved = (s.angle - s.watchFrom) * sign, sped = s.rate * sign - s.watchRate;
        if (s.pushWatched && over > rules.guardSeconds && moved < -rules.wrongWayDegrees && sped < -0.1f * most)
            s.stop = SteerStop::kWrongWay;
        else if (s.pushWatched && over > rules.guardSeconds + 1.5f * lag + 0.5f && std::fabs((s.angle - s.pushFrom) * sign) < rules.stillDegrees)
            s.stop = SteerStop::kNoAnswer;
        if (s.stop != SteerStop::kNone) {
            ask(0.0f);
            return out;
        }
    }
    // The strength learned only from a push one way held strongly past the turret's delay and three lags, never from the coast after one (the review's
    // fourth finding): each window's turn against the turn the rates asked for, as the turret acted on them (its delay after).
    const float pushing = std::fabs(rate) >= rules.learnLeast * most ? (rate > 0.0f ? 1.0f : -1.0f) : 0.0f;
    if (pushing != 0.0f && pushing == s.steadySign && s.steadySince >= 0.0) {
        if (now - s.steadySince >= r.delay + 3.0f * lag) {
            if (!s.window) {
                s.window = true, s.windowAt = now, s.windowFrom = s.angle, s.windowSent = 0.0f;
            } else {
                s.windowSent += SentAt(s, now - r.delay) * dt;
                if (now - s.windowAt >= rules.learnWindow) {
                    Learn(s, s.windowSent, s.angle - s.windowFrom, rules);
                    s.windowAt = now, s.windowFrom = s.angle, s.windowSent = 0.0f;
                }
            }
        }
    } else {
        s.steadySince = pushing != 0.0f ? now : -1.0;
        s.steadySign = pushing;
        s.window = false;
    }
    ask(rate);
    out.rate = rate;
    out.ask = r.sign * rate / s.gain;
    out.input = r.sign * rate / most;
    out.sent = rate != 0.0f;
    return out;
}

// ------------------------------------------------------------------ the inputs: the pad's right stick, the mouse ------------------------------------------------------------------

// A rate table as R5 measures it: (stick, degrees a second) pairs from the dead zone's edge to the whole stick, rising (the seat's curve has knees at
// 0.25, 0.75, 0.875 and 0.95 of its own range: four points from 0.25 to 1 read its top half as a straight line and missed half the rate there).
struct PadTable {
    int n = 0;
    float stick[6] = {}, rate[6] = {};
};
inline PadTable PadTableOf(const Measure& m) {
    PadTable t;
    for (int i = 0; i + 1 < kRowNumbers && t.n < 6; i += 2)
        if (std::isfinite(m.value[i]) && std::isfinite(m.value[i + 1]))
            t.stick[t.n] = m.value[i], t.rate[t.n] = m.value[i + 1], ++t.n;
    return t;
}
inline float PadMost(const PadTable& t) { return t.n ? t.rate[t.n - 1] : 0.0f; }
// The stick that asks a rate (signed; degrees a second): the table read backward, the dead zone's edge for the least; 0 for none; the full stick past
// the most.
inline float PadStick(const PadTable& t, float rate) {
    if (rate == 0.0f || t.n < 2 || !std::isfinite(rate))
        return 0.0f;
    const float want = std::fabs(rate);
    float stick = t.stick[t.n - 1];
    if (want <= t.rate[0]) {
        stick = t.stick[0];
    } else {
        for (int i = 1; i < t.n; ++i)
            if (want <= t.rate[i]) {
                const float span = t.rate[i] - t.rate[i - 1];
                stick = t.stick[i - 1] + (t.stick[i] - t.stick[i - 1]) * (span > 0.0f ? (want - t.rate[i - 1]) / span : 1.0f);
                break;
            }
    }
    return std::copysign(Clamp(stick, 0.0f, 1.0f), rate);
}
// ... and the rate a stick asks (the table read forward; for the tests).
inline float PadRate(const PadTable& t, float stick) {
    const float s = std::fabs(stick);
    if (t.n < 2 || s < t.stick[0])
        return 0.0f;
    for (int i = 1; i < t.n; ++i)
        if (s <= t.stick[i]) {
            const float span = t.stick[i] - t.stick[i - 1];
            return std::copysign(t.rate[i - 1] + (t.rate[i] - t.rate[i - 1]) * (span > 0.0f ? (s - t.stick[i - 1]) / span : 1.0f), stick);
        }
    return std::copysign(t.rate[t.n - 1], stick);
}

// The mouse: counts this frame for a rate (degrees a second) over the frame's seconds, the fraction carried to the next (no drift), at most `most` a frame.
struct MouseCarry {
    float carry = 0.0f;
};
inline int MouseCounts(MouseCarry& c, float rate, float seconds, float degreesPerCount, int most) {
    if (!(degreesPerCount > 0.0f) || !std::isfinite(rate) || !(seconds > 0.0f) || rate == 0.0f) {
        c.carry = 0.0f;  // (stopped: nothing owed)
        return 0;
    }
    c.carry += rate * seconds / degreesPerCount;
    int n = static_cast<int>(c.carry);  // (toward zero, the rest carried)
    if (n > most)
        n = most;
    else if (n < -most)
        n = -most;
    c.carry -= static_cast<float>(n);
    if (std::fabs(c.carry) > 1.0f)
        c.carry = std::copysign(1.0f, c.carry);  // (cut to the most: what's over isn't owed)
    return n;
}

// ------------------------------------------------------------------ fire ------------------------------------------------------------------

// The fire in the seat (4.7): the left mouse button down while the trigger of a hand that holds a handle is pulled; either hand, both one button; 0.6 to
// press, under 0.4 to let go. A trigger pulled already, before its hand took hold, waits to be let go first (no burst by closing a hand on a pulled
// trigger); the grip and the trigger in the same frame fire. A trigger of a hand on no handle does nothing. Step 1: a hand holds while its grip is
// squeezed. (input.cpp's release guard, StepRelease, goes on behind it as for every press of the button.)
struct Fire {
    bool down = false;
    bool pulled[2] = {};
    bool heldOff[2] = {};
    bool holding[2] = {};
    float last[2] = {};  // each trigger's pull the frame before
};
inline bool FireStep(Fire& f, const float trigger[2], const bool holds[2]) {
    bool any = false;
    for (int i = 0; i < 2; ++i) {
        const float pull = std::isfinite(trigger[i]) ? trigger[i] : 0.0f;
        f.pulled[i] = pull > (f.pulled[i] ? 0.4f : 0.6f);
        if (holds[i] && !f.holding[i] && f.last[i] >= 0.4f && pull >= 0.4f)
            f.heldOff[i] = true;
        if (pull < 0.4f || !holds[i])
            f.heldOff[i] = false;
        f.holding[i] = holds[i];
        f.last[i] = pull;
        any = any || (holds[i] && f.pulled[i] && !f.heldOff[i]);
    }
    f.down = any;
    return any;
}

}  // namespace mountgun

// Loose things let go of (src/loose.cpp) on scripted frames at 72 Hz, without the game: out of the well with the button, the gun level, on its side (it
// slides out sideways along the well's line, then falls and tips), pointing down, upside down, the pistol's raked grip; carried by walking, not by the
// stick's turning; let go of, thrown (it tumbles, its spin kept in the air; spun about its middle axis it flips over), flicked; every magazine on dirt
// and tarmac, empty and full; never more than 2 mm into the ground (its own shape, curved backs and round rims, checked apart from the module's own
// points); at rest; the ground moving under one (a firm ground moves it, a guess of it doesn't); slopes; the origin's metre jumps; a minute lying, then
// going only unseen (the real minute); too many at once; no ground at all; the launcher's case rolling; the M320's spent case held in its open barrel
// until the muzzle's raised past about 30 degrees (or the gun's shoved away from it), the tracking's shake never walking it out; casings by the dozen,
// and what they cost; a tracking jump as it's let go of; what's heard. The player's rifle (gunbody.h, checks g1-g9): dropped upright it tips onto
// its side, dropped on its side it stays there, never a hull point into the ground; it lies past any time, looked away; it's never pushed out by
// magazines, nor by fewer allowed; let go of by either hand at that hand's own speed and turn; by the left exactly as Release; gone only with no
// ground or refused with broken numbers; no spin is fine. The floor the soldier stands on where the game's rays see none (src/stood.h, checks s1-s19:
// Operation Swordbreaker's overpass from the owner's log of 2026-11-26): every magazine, box and gun let go of there lands on the deck, where without it
// each fell through to the embankment; nothing changes where the rays see his floor (a roof's edge or corner, a crate's, still drops a magazine past
// it); a print reaches 1 m (past the railing, a box 0.9 m out falls); one in use is never forgotten.
// The room is the arms' space here (identity) unless a check turns it; y up; the ground at y 0 unless a check moves it; the origin 0.
// Prints PASS or FAIL per check, with what was measured; the exit code is the failures.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <vector>

#include "../src/ground.h"
#include "../src/gunbodies.h"
#include "../src/gunbody.h"
#include "../src/loose.h"
#include "../src/stood.h"

namespace {

constexpr float kDt = 1.0f / 72.0f;
constexpr float kPi = 3.14159265f;
constexpr float kDegree = kPi / 180.0f;

int g_failures = 0, g_checks = 0;
void Check(bool ok, const char* what) {
    ++g_checks;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        ++g_failures;
}
// The same, with what was measured in it.
template <typename... Args>
void CheckF(bool ok, const char* format, Args... args) {
    char what[400];
    std::snprintf(what, sizeof(what), format, args...);
    Check(ok, what);
}

// The ground: a plane through (0, height, 0), tilted slope radians down toward +z. firm: the game's own (it moves one lying); nothing: none under
// anything (the floor says -1).
struct Ground {
    float height = 0.0f, slope = 0.0f;
    int surface = loose::kDirt;
    bool firm = true;
    bool nothing = false;
};
Ground g_ground;
int GroundFn(void*, const float*, loose::Plane* planes, int most) {
    if (g_ground.nothing)
        return -1;
    if (most < 1)
        return 0;
    planes[0].point[0] = 0.0f, planes[0].point[1] = g_ground.height, planes[0].point[2] = 0.0f;
    planes[0].normal[0] = 0.0f, planes[0].normal[1] = std::cos(g_ground.slope), planes[0].normal[2] = std::sin(g_ground.slope);
    planes[0].surface = g_ground.surface;
    planes[0].firm = g_ground.firm;
    return 1;
}
float AboveGround(const float* p) {
    return p[1] * std::cos(g_ground.slope) + p[2] * std::sin(g_ground.slope) - g_ground.height * std::cos(g_ground.slope);
}

void Identity(float* m) {
    const float id[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    std::memcpy(m, id, sizeof(id));
}
// A turn about the unit axis (row-major 3x4, no move).
void Turn(float* m, const float* a, float angle) {
    const float c = std::cos(angle), s = std::sin(angle), u = 1.0f - c;
    const float r[12] = {u * a[0] * a[0] + c,        u * a[0] * a[1] - s * a[2], u * a[0] * a[2] + s * a[1], 0,
                         u * a[0] * a[1] + s * a[2], u * a[1] * a[1] + c,        u * a[1] * a[2] - s * a[0], 0,
                         u * a[0] * a[2] - s * a[1], u * a[1] * a[2] + s * a[0], u * a[2] * a[2] + c,        0};
    std::memcpy(m, r, sizeof(r));
}
void Compose(const float* a, const float* b, float* out) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j)
            out[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        out[i * 4 + 3] += a[i * 4 + 3];
    }
}
void Apply(const float* m, const float* p, float* out) {
    for (int i = 0; i < 3; ++i)
        out[i] = m[i * 4] * p[0] + m[i * 4 + 1] * p[1] + m[i * 4 + 2] * p[2] + m[i * 4 + 3];
}
float Length(const float* v) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }
float Distance(const float* a, const float* b) {
    const float d[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    return Length(d);
}
// The turn from bone a to bone b in a's own axes, as a rotation vector (axis times angle).
void TurnInOwn(const float* a, const float* b, float* out) {
    float d[9];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            d[i * 3 + j] = a[i] * b[j] + a[4 + i] * b[4 + j] + a[8 + i] * b[8 + j];  // a's turn transposed, times b's
    const float axis[3] = {d[7] - d[5], d[2] - d[6], d[3] - d[1]};
    const float s = Length(axis);
    const float angle = std::atan2(0.5f * s, 0.5f * (d[0] + d[4] + d[8] - 1.0f));
    for (int i = 0; i < 3; ++i)
        out[i] = s > 1e-12f ? axis[i] * angle / s : 0.0f;
}
float AngleBetween(const float* a, const float* b) {
    float w[3];
    TurnInOwn(a, b, w);
    return Length(w);
}

loose::Frame Base() {
    loose::Frame f;
    f.dt = kDt;
    Identity(f.roomToArms);
    f.floor = GroundFn;
    f.eye[1] = 1.6f;
    f.look[2] = 1.0f;  // looking ahead along +z
    return f;
}
void LookAt(loose::Frame& f, const float* at) {
    for (int i = 0; i < 3; ++i)
        f.look[i] = at[i] - f.eye[i];
}
void LookUp(loose::Frame& f) { f.look[0] = 0.0f, f.look[1] = 1.0f, f.look[2] = 0.0f; }

// How far its lowest point is above the ground (negative: into it), from its shape as its spec gives it, apart from the module's own points: a
// magazine's sides along its sheared, bowed back and front, finely; a round thing's rims, finely; a gun's hull points (where it touches).
float Lowest(const loose::Spec& s, const float* bone) {
    float low = 1e9f;
    auto at = [&](float x, float y, float z) {
        const float v[3] = {x, y, z};
        float p[3];
        Apply(bone, v, p);
        low = std::fmin(low, AboveGround(p));
    };
    if (s.round < 0 && s.hull && s.hullPoints > 0) {
        for (int i = 0; i < s.hullPoints; ++i)
            at(s.hull[i][0], s.hull[i][1], s.hull[i][2]);
        return low;
    }
    if (s.round >= 0) {
        const int axis = s.round, u = (axis + 1) % 3, w = (axis + 2) % 3;
        for (int i = 0; i < s.rings; ++i)
            for (int k = 0; k < 64; ++k) {
                float c[3];
                c[axis] = s.ringAt[i];
                c[u] = s.roundAt[0] + s.ringRadius[i] * std::cos(2.0f * kPi * k / 64.0f);
                c[w] = s.roundAt[1] + s.ringRadius[i] * std::sin(2.0f * kPi * k / 64.0f);
                at(c[0], c[1], c[2]);
            }
        return low;
    }
    const float zSpan = s.boxMax[2] - s.boxMin[2], depth = zSpan - std::fabs(s.sweep);
    const float backTop = s.sweep >= 0.0f ? s.boxMin[2] : s.boxMin[2] - s.sweep, length = s.boxMax[1] - s.boxMin[1];
    for (float x : {s.boxMin[0], s.boxMax[0]})
        for (int j = 0; j <= 48; ++j) {
            const float t = j / 48.0f, y = s.boxMax[1] - length * t, b = backTop + s.sweep * t - s.sag * 4.0f * t * (1.0f - t);
            at(x, y, b);
            at(x, y, b + depth);
        }
    return low;
}
// The middle of its box (a round thing's: on its axis, between its end rings), in the arms' space.
void Middle(const loose::Spec& s, const float* bone, float* out) {
    float v[3];
    if (s.round >= 0) {
        const int axis = s.round;
        v[axis] = 0.5f * (s.ringAt[0] + s.ringAt[s.rings - 1]), v[(axis + 1) % 3] = s.roundAt[0], v[(axis + 2) % 3] = s.roundAt[1];
    } else {
        for (int i = 0; i < 3; ++i)
            v[i] = 0.5f * (s.boxMin[i] + s.boxMax[i]);
    }
    Apply(bone, v, out);
}

// What one run saw of one thing.
struct Seen {
    int frames = 0;
    float lowest = 1e9f;       // its lowest point above the ground (m; negative: into it)
    float firstHitAt = -1.0f, firstHitSpeed = 0.0f, firstHitDb = 0.0f;
    bool firstHitFirst = false;  // told as its first landing
    int hits = 0;
    float closestHits = 1e9f;  // s between two sounds
    float restAt = -1.0f;
    float bone[12] = {};
    bool gone = false;
    float goneAt = -1.0f;
};

// Runs frames until seconds have passed; the frame made each time by make (gun, hand, turn...); watch sees each pose of the thing with this id.
template <typename Make, typename Watch>
Seen Run(const loose::Spec& spec, int id, float seconds, Make make, Watch watch) {
    Seen s;
    float lastHit = -1e9f, time = 0.0f;
    const int frames = static_cast<int>(seconds / kDt + 0.5f);
    for (int i = 0; i < frames; ++i) {
        time += kDt;
        loose::Frame f = Base();
        make(f, time);
        const loose::Result& r = loose::Step(f);
        for (int k = 0; k < r.gone; ++k)
            if (r.goneId[k] == id && !s.gone)
                s.gone = true, s.goneAt = time;
        for (int k = 0; k < r.hits; ++k)
            if (r.hit[k].id == id) {
                if (s.firstHitAt < 0.0f)
                    s.firstHitAt = time, s.firstHitSpeed = r.hit[k].speed, s.firstHitDb = r.hit[k].gainDb, s.firstHitFirst = r.hit[k].first;
                s.closestHits = std::fmin(s.closestHits, time - lastHit);
                lastHit = time;
                ++s.hits;
            }
        for (int k = 0; k < r.poses; ++k) {
            const loose::Pose& p = r.pose[k];
            if (p.id != id)
                continue;
            s.lowest = std::fmin(s.lowest, Lowest(spec, p.bone));
            if (p.resting && s.restAt < 0.0f)
                s.restAt = time;
            std::memcpy(s.bone, p.bone, sizeof(s.bone));
            watch(p, time, f);
        }
        ++s.frames;
    }
    return s;
}
template <typename Make>
Seen Run(const loose::Spec& spec, int id, float seconds, Make make) {
    return Run(spec, id, seconds, make, [](const loose::Pose&, float, const loose::Frame&) {});
}
void Still(loose::Frame&, float) {}

// The gun's frame bone in the room at (x, y, z), turned by turn (3x4, no move).
void GunAt(loose::Frame& f, const float* turn, float x, float y, float z) {
    std::memcpy(f.gunRoom, turn, sizeof(f.gunRoom));
    f.gunRoom[3] = x, f.gunRoom[7] = y, f.gunRoom[11] = z;
    f.gun = true;
}
// A frame's steps with the gun held still at 1.40 m: its history.
void Hold(loose::Frame& f, const float* turn, int frames = 6) {
    GunAt(f, turn, 0.0f, 1.40f, 0.0f);
    for (int i = 0; i < frames; ++i)
        loose::Step(f);
}

constexpr float kDown[3] = {0.0f, -1.0f, 0.0f};
float WellOf(const char* name) {  // its mouth's distance (READ: pouch.cpp's 8.7 cm for the M416; the study's for the others)
    return std::strcmp(name, "m416") == 0 ? 0.087f : std::strcmp(name, "m16a4") == 0 ? 0.075f : std::strcmp(name, "kh2002") == 0 ? 0.05f : 0.077f;
}

// What the eject showed: when it left the well, how it moved in it, and how it fell.
struct Out {
    float outAt = -1.0f;        // its first frame free
    float start[3] = {};        // its middle at the button
    bool haveStart = false;
    float dropInWell = 0.0f;    // the most its middle fell while still in the well
    float tiltInWell = 0.0f;    // the most its long axis left level while in the well (radians)
    float alongAtOut[3] = {};   // its middle at its last frame in the well, from the start
    float middleAtOut[3] = {};
    float fellBy = 0.0f;        // its middle 0.2 s after leaving, below where it left
    float tiltFalling = 0.0f;   // the most its long axis turned from level after leaving, before landing
    float bounced = 0.0f;       // the most its middle rose after it first touched the ground (it lands on a corner: one end stays down)
    float middleAtLanding = 0.0f;
};

// An M4A1-type magazine out with the button from the gun at 1.40 m turned by gunTurn (its vertices' space the gun's frame, as the M416's box is in the
// gun's model space), walking ahead at walk m/s (the room moving in the world).
Seen EjectRun(const char* name, const float* gunTurn, float seconds, float walk = 0.0f, int surface = loose::kDirt, Out* out = nullptr) {
    loose::Clear();
    g_ground = Ground{};
    g_ground.surface = surface;
    const loose::Spec& spec = *loose::SpecFor(name);
    loose::Frame f = Base();
    Hold(f, gunTurn);
    float onGun[12];
    Identity(onGun);
    const int id = loose::Eject(spec, 1, onGun, kDown, WellOf(name), 0.0f, 0, f);
    float landedAt = -1.0f;
    return Run(
        spec, id, seconds,
        [&](loose::Frame& g, float t) {
            GunAt(g, gunTurn, 0.0f, 1.40f, 0.0f);
            g.roomToArms[11] = walk * t;
            g.walk[2] = walk;
        },
        [&](const loose::Pose& p, float t, const loose::Frame&) {
            if (!out)
                return;
            float middle[3];
            Middle(spec, p.bone, middle);
            const float level = std::asin(std::fmin(1.0f, std::fabs(p.bone[5])));  // its own y (its length) against the world's level
            if (!out->haveStart)
                std::memcpy(out->start, middle, sizeof(middle)), out->haveStart = true;
            if (p.inGun) {
                out->dropInWell = std::fmax(out->dropInWell, out->start[1] - middle[1]);
                out->tiltInWell = std::fmax(out->tiltInWell, level);
                for (int i = 0; i < 3; ++i)
                    out->alongAtOut[i] = middle[i] - out->start[i];
                std::memcpy(out->middleAtOut, middle, sizeof(middle));
                return;
            }
            if (out->outAt < 0.0f)
                out->outAt = t;
            if (t - out->outAt <= 0.2f + 1e-4f)
                out->fellBy = out->middleAtOut[1] - middle[1];
            if (landedAt < 0.0f && Lowest(spec, p.bone) < 0.01f)  // (within a frame's fall of touching: at 4 m/s it's 6 cm a frame)
                landedAt = t, out->middleAtLanding = middle[1];
            if (landedAt < 0.0f)
                out->tiltFalling = std::fmax(out->tiltFalling, std::fabs(level - out->tiltInWell));
            else
                out->bounced = std::fmax(out->bounced, middle[1] - out->middleAtLanding);
        });
}

// Let go of from a still hand at height (its grip), the magazine upright as held.
Seen DropRun(const char* name, float height, int surface, int rounds) {
    loose::Clear();
    g_ground = Ground{};
    g_ground.surface = surface;
    const loose::Spec& spec = *loose::SpecFor(name);
    loose::Frame f = Base();
    f.hand = true;
    Identity(f.handRoom);
    f.handRoom[7] = height;
    for (int i = 0; i < 6; ++i)
        loose::Step(f);
    float bone[12];
    Identity(bone);
    bone[7] = height + 0.17f;  // its middle near the hand
    const int id = loose::Release(spec, 2, bone, rounds, f);
    float hand[12];
    std::memcpy(hand, f.handRoom, sizeof(hand));
    return Run(spec, id, 4.0f, [&](loose::Frame& g, float) { g.hand = true, std::memcpy(g.handRoom, hand, sizeof(hand)); });
}

// The M320 under the rifle: its spent case seated in the barrel, its base at (0 -0.08 0.20) on the gun's frame, its mouth toward the muzzle; the
// barrel open behind it, the way out toward the gun's back.
constexpr float kCaseSeat[12] = {1, 0, 0, 0, 0, 1, 0, -0.08f, 0, 0, 1, 0.20f};
constexpr float kCaseOut[3] = {0.0f, 0.0f, -1.0f};
constexpr float kCaseLength = 0.046f;
const float kCaseGrip = std::tan(30.0f * kDegree);
// The gun's frame bone with the muzzle raised by pitch (radians), at (0, 1.3, 0) and moved by at (the gun's own place on top).
void Pitched(loose::Frame& f, float pitch, const float* at = nullptr) {
    const float x[3] = {1.0f, 0.0f, 0.0f};
    float turn[12];
    Turn(turn, x, -pitch);
    GunAt(f, turn, at ? at[0] : 0.0f, 1.3f + (at ? at[1] : 0.0f), at ? at[2] : 0.0f);
}
// How far its base is from its seat in the barrel (m), the gun as this frame has it.
float OffSeat(const loose::Pose& p, const loose::Frame& f) {
    float seat[3];
    const float base[3] = {0.0f, 0.0f, 0.0f};
    float onGun[12];
    Compose(f.gunRoom, kCaseSeat, onGun);
    Apply(onGun, base, seat);
    float at[3];
    Apply(p.bone, base, at);
    return Distance(seat, at);
}
// A small steady shake (the tracking's: a millimetre or less), the same every run.
unsigned g_noise = 12345u;
float Noise() {
    g_noise = g_noise * 1664525u + 1013904223u;
    return static_cast<float>((g_noise >> 8) & 0xFFFF) / 65535.0f * 2.0f - 1.0f;
}

// The rifle (gunbody.h) rolled about its barrel (its z) by roll radians, its lowest hull point height above a level ground at 0.
void GunBone(float roll, float height, float* bone) {
    const float along[3] = {0.0f, 0.0f, 1.0f};
    Turn(bone, along, roll);
    const Ground was = g_ground;
    g_ground = Ground{};
    bone[7] = height - Lowest(gunbody::kM416, bone);
    g_ground = was;
}
bool AllFinite(const float* p, int n) {
    for (int i = 0; i < n; ++i)
        if (!std::isfinite(p[i]))
            return false;
    return true;
}
// The middles' steps over their first frame free (from where they were started; at most 4 started in the same frame), the arms' space: the
// starts' own Step, then one more.
bool FirstSteps(const loose::Spec& spec, const int* ids, int n, float (*steps)[3]) {
    float from[4][3] = {}, to[4][3] = {};
    int have = 0;
    for (int i = 0; i < 2; ++i) {
        const loose::Result& r = loose::Step(Base());
        for (int k = 0; k < r.poses; ++k)
            for (int j = 0; j < n && j < 4; ++j)
                if (r.pose[k].id == ids[j]) {
                    Middle(spec, r.pose[k].bone, i == 0 ? from[j] : to[j]);
                    ++have;
                }
    }
    for (int j = 0; j < n && j < 4; ++j)
        for (int i = 0; i < 3; ++i)
            steps[j][i] = to[j][i] - from[j][i];
    return have == 2 * n;
}

// ---- The soldier's floor where the game's rays see none (stood.h) ----
// Operation Swordbreaker's overpass as the owner's log had it (build\bf3vr.log, 2026-11-26 03:28:41-03:29:56, the M249 there): the deck at y 77.85 (the
// boxes lay on it a moment, on the feet's guess: the soldier crouched, the game's eye at 78.85), never seen by the game's ray cast (the ray down from his
// eye found nothing within 3 m for 70 s); under it, the embankment its far ray found 6.85 m down (71.0, firm). The arms' space's origin as the game's
// whole metres; the room is the arms' space. Or a visible floor (the hotel's roof at 94.2 with its edge, the street 12.6 m under it past the edge; with
// roofEdgeZ an outside corner, or a crate's top), or a crate the rays see on the deck.
struct Overpass {
    int origin[3] = {-609, 77, -2090};
    float deck = 77.85f, below = 71.0f;
    float eye[3] = {-609.6f, 78.85f, -2089.3f};  // the game's eye (the soldier crouched)
    int stance = 1;
    bool stoodOn = true;      // the soldier's floor where the rays see none (the fix); off: the old rule (ground::Planes before stood.h)
    float raysAfter = 1.0f;   // s from a start before its far ray's back: nothing's known till then (the feet's guess holds it: #17's second on it)
    float clock = 0.0f;       // s since the last start
    bool roof = false;
    float roofY = 94.2f, roofEdge = -445.3f, streetY = 81.6f;  // the roof where x < roofEdge (and z < roofEdgeZ), the street past it
    float roofEdgeZ = 1e9f;
    bool crate = false;
    float crateY = 78.35f, crateMin[2] = {}, crateMax[2] = {};  // its top, across x and z
    stood::Watch watch;
    unsigned long long now = 5000000;  // ms (GetTickCount64's)
};
Overpass g_over;
float OverO(int i) { return static_cast<float>(g_over.origin[i]); }

// The first floor a ray straight down from `from` finds within reach (the visible ones: the roof or the street, a crate, else the embankment).
bool RayDown(const float* from, float reach, float& y) {
    float floorY = g_over.below;
    if (g_over.roof)
        floorY = from[0] < g_over.roofEdge && from[2] < g_over.roofEdgeZ ? g_over.roofY : g_over.streetY;
    else if (g_over.crate && from[0] >= g_over.crateMin[0] && from[0] <= g_over.crateMax[0] && from[2] >= g_over.crateMin[1] &&
             from[2] <= g_over.crateMax[1] && from[1] >= g_over.crateY - 0.03f)
        floorY = g_over.crateY;
    if (from[1] - floorY > reach || from[1] < floorY - 0.03f)
        return false;
    y = floorY;
    return true;
}
// As hands.cpp's SoldierFloor over ground::Planes, line for line in what they decide: the rays' answer under the thing's middle (a visible floor at once;
// the embankment once its far ray's back, nothing known before), the soldier's floor in its place where stood.h says (Planes: kStood), then a ray's or
// his, firm by ground::Firm, else the feet's guess (the eye less the stance's height), not firm.
int OverpassFloor(void*, const float* at, loose::Plane* planes, int most) {
    if (most < 1)
        return 0;
    const float w[3] = {at[0] + OverO(0), at[1] + OverO(1), at[2] + OverO(2)};
    float raysY = 0.0f;
    stood::Rays rays = stood::Rays::kNone;
    const bool overCrate = g_over.crate && w[0] >= g_over.crateMin[0] && w[0] <= g_over.crateMax[0] && w[2] >= g_over.crateMin[1] &&
                           w[2] <= g_over.crateMax[1] && w[1] >= g_over.crateY - 0.03f;
    if (g_over.roof || overCrate || g_over.clock >= g_over.raysAfter)  // (what they see at once; the embankment once its far ray's back)
        rays = RayDown(w, 50.0f, raysY) ? stood::Rays::kFloor : stood::Rays::kNothing;
    ground::Source source = rays == stood::Rays::kFloor ? ground::kRay : ground::kNone;
    float y = raysY, printY = 0.0f;
    if (g_over.stoodOn && g_over.watch.Under(w, printY) && stood::StandsIn(rays, raysY, printY))
        source = ground::kStood, y = printY;
    loose::Plane& p = planes[0];
    p.point[0] = at[0], p.point[2] = at[2];
    p.normal[0] = 0.0f, p.normal[1] = 1.0f, p.normal[2] = 0.0f;
    p.surface = loose::kTarmac;
    if (source == ground::kRay || source == ground::kStood) {
        p.point[1] = y - OverO(1);
        p.firm = ground::Firm(source);
    } else {
        p.point[1] = g_over.eye[1] - stood::kEyeHeight[g_over.stance] - OverO(1);
        p.firm = false;
    }
    return 1;
}
// A frame there: its floor and origin, the head (the soldier's eye, looking down ahead), the clock; the game's eye told to the watch (on foot) and its
// frame, as ground.cpp's SetEye and OnFrame do.
void OverFrame(loose::Frame& f, float) {
    f.floor = OverpassFloor;
    for (int i = 0; i < 3; ++i) {
        f.origin[i] = g_over.origin[i];
        f.eye[i] = g_over.eye[i] - OverO(i);
    }
    f.look[0] = 0.0f, f.look[1] = -0.6f, f.look[2] = 1.0f;
    g_over.clock += kDt;
    g_over.now += 14;
    g_over.watch.Eye(g_over.now, g_over.eye, g_over.stance, true);
    g_over.watch.Frame(g_over.now, g_over.eye);
}
// The eye's ray and, if it asks, the survey round him, as ground.cpp tells the watch them: 1 a verdict of unseen (kept), 0 an edge, -1 none.
int OverSurvey() {
    float feet = 0.0f;
    const bool steady = g_over.watch.Steady(g_over.now, feet);
    float y = 0.0f;
    const bool found = RayDown(g_over.eye, 3.0f, y);
    if (g_over.watch.EyeBack(g_over.now, g_over.eye, steady, feet, found, 1.0f, y)) {
        const stood::Survey& s = g_over.watch.survey();
        const unsigned serial = s.serial;
        float centre[3];
        std::memcpy(centre, s.eye, sizeof(centre));
        for (int i = 0; i < stood::kRound; ++i) {
            float from[3], ry = 0.0f;
            stood::RoundFrom(centre, i, from);
            const bool hit = RayDown(from, 3.0f, ry);
            g_over.watch.Round(g_over.now + 28, serial, i, hit, 1.0f, ry);
        }
    }
    const stood::Verdict& v = g_over.watch.verdict();
    return v.have ? (v.unseen ? 1 : 0) : -1;
}
// The soldier as the log had him, still for 0.6 s (the watch told each frame), nothing lying; the stood floor on or off.
void OverStart(bool stoodOn) {
    loose::Clear();
    g_over = Overpass{};
    g_over.stoodOn = stoodOn;
    g_ground = Ground{};
    g_ground.height = g_over.deck - OverO(1);  // (Lowest measures over the deck)
    for (int i = 0; i < 43; ++i) {
        g_over.now += 14;
        g_over.watch.Eye(g_over.now, g_over.eye, g_over.stance, true);
    }
}
// Let go of from a still hand: its middle up m over the deck (over the floor under it), ahead m in front of his eye (+z), side m to his left (+x); the
// clock from 0 (its far ray asked from now).
int OverDrop(const loose::Spec& spec, int kind, float up, float ahead, float side = 0.0f, int rounds = 0, float floorY = -1e9f) {
    loose::Frame f = Base();
    OverFrame(f, 0.0f);
    f.hand = true;
    Identity(f.handRoom);
    f.handRoom[3] = g_over.eye[0] + side - OverO(0);
    f.handRoom[7] = (floorY > -1e8f ? floorY : g_over.deck) + up - OverO(1);
    f.handRoom[11] = g_over.eye[2] + ahead - OverO(2);
    for (int i = 0; i < 6; ++i)
        loose::Step(f);
    float bone[12], mid[3];
    Identity(bone);
    Middle(spec, bone, mid);
    bone[3] = f.handRoom[3] - mid[0], bone[7] = f.handRoom[7] - mid[1], bone[11] = f.handRoom[11] - mid[2];
    const int id = loose::Release(spec, kind, bone, rounds, f);
    g_over.clock = 0.0f;
    return id;
}
// Its lowest point over the deck now (negative: under it), its shape's own.
float OverLowest(const loose::Spec& spec, const float* bone) { return Lowest(spec, bone); }

}  // namespace

int main() {
    float level[12], side[12], down60[12], upside[12];
    Identity(level);
    const float z[3] = {0, 0, 1}, x[3] = {1, 0, 0}, y[3] = {0, 1, 0};
    Turn(side, z, -kPi / 2);
    Turn(down60, x, kPi / 3);
    Turn(upside, z, kPi);

    // ---- Out with the button ----
    {
        Out out;
        const Seen s = EjectRun("m4a1", level, 4.0f, 0.0f, loose::kDirt, &out);
        CheckF(out.outAt > 0.04f && out.outAt < 0.11f, "M4A1 level: out of its well %.3f s after the button (model: 0.069 s)", out.outAt);
        CheckF(s.firstHitAt > 0.35f && s.firstHitAt < 0.50f, "... and down 1.1 m, it lands %.3f s after the button (model: 0.406 s)", s.firstHitAt);
        CheckF(s.firstHitSpeed > 4.0f && s.firstHitSpeed < 5.0f, "... at %.2f m/s into the ground (model: 4.43)", s.firstHitSpeed);
        CheckF(s.restAt > 0.0f && s.restAt < 2.0f, "... at rest %.2f s after (within 2 s; model: 1.04 s)", s.restAt);
        CheckF(s.lowest > -0.002f, "... never more than 2 mm into the ground (its deepest %.2f mm)", -s.lowest * 1000.0f);
        CheckF(std::fabs(s.bone[5]) < 0.3f, "... lying with its length level (its length %.0f degrees from level)", std::asin(std::fmin(1.0f, std::fabs(s.bone[5]))) / kDegree);
        CheckF(std::fabs(s.bone[4]) > 0.85f, "... on its broad side (its side's face %.0f degrees from facing up or down)", std::acos(std::fmin(1.0f, std::fabs(s.bone[4]))) / kDegree);
        CheckF(s.hits >= 1 && s.hits <= 3, "... heard %d times (1-3; model: 2)", s.hits);
        CheckF(s.firstHitFirst && s.firstHitDb > -6.0f && s.firstHitDb < 0.5f, "... its first landing told as such, at %.1f dB 1.6 m from the head (the game's impact curve; model: -2)", s.firstHitDb);
        CheckF(s.closestHits >= 0.079f, "... never two sounds within 0.08 s (closest %.3f s apart%s)", std::fmin(s.closestHits, 9.999f), s.hits < 2 ? ": only one" : "");
    }
    {
        Out onDirt, onTarmac;
        const Seen dirt = EjectRun("m4a1", level, 4.0f, 0.0f, loose::kDirt, &onDirt), tarmac = EjectRun("m4a1", level, 4.0f, 0.0f, loose::kTarmac, &onTarmac);
        CheckF(tarmac.restAt > 0.0f && tarmac.restAt < 2.0f && tarmac.lowest > -0.002f,
               "M4A1 level on tarmac: bounces, at rest at %.2f s, never into it (deepest %.2f mm; model: 1.08 s)", tarmac.restAt, -tarmac.lowest * 1000.0f);
        CheckF(onTarmac.bounced > 0.02f && onTarmac.bounced < 0.10f && onDirt.bounced < 0.5f * onTarmac.bounced,
               "... it bounces a little (\"bounce a little bit\"): its middle up %.0f mm after touching tarmac, %.0f mm on dirt (model: 68 and 12)", onTarmac.bounced * 1000.0f,
               onDirt.bounced * 1000.0f);
        CheckF(tarmac.hits >= dirt.hits && tarmac.closestHits >= 0.079f, "... heard as often as on dirt or more (%d against %d: it clatters), never two within 0.08 s (closest %.3f s)",
               tarmac.hits, dirt.hits, std::fmin(tarmac.closestHits, 9.999f));
    }
    {
        // The user, 2026-11-05: held sideways it "should slide out... gravity pull and weight", not fall straight from the gun.
        Out out;
        const Seen s = EjectRun("m4a1", side, 4.0f, 0.0f, loose::kDirt, &out);
        const float sideways = std::sqrt(out.alongAtOut[0] * out.alongAtOut[0] + out.alongAtOut[2] * out.alongAtOut[2]);
        CheckF(out.dropInWell < 0.002f && out.tiltInWell < 3.0f * kDegree,
               "M4A1, the gun on its side: in the well it slides out along the well's line, its middle dropping %.1f mm, its length %.1f degrees off level",
               out.dropInWell * 1000.0f, out.tiltInWell / kDegree);
        CheckF(sideways > 0.05f && out.alongAtOut[0] < 0.0f, "... sideways, out of the gun's underside: %.0f mm along it by its last frame in the well", sideways * 1000.0f);
        CheckF(out.outAt > 0.09f && out.outAt < 0.17f, "... slowed by the walls' grip (held sideways): out %.3f s after the button (model: 0.122 s, level 0.069)", out.outAt);
        CheckF(out.fellBy > 0.12f, "... then it falls: %.0f mm lower 0.2 s after leaving", out.fellBy * 1000.0f);
        CheckF(out.tiltFalling > 15.0f * kDegree, "... tipping over the lip as it goes (its length %.0f degrees off level before it lands)", out.tiltFalling / kDegree);
        CheckF(s.firstHitAt > 0.55f && s.firstHitAt < 0.75f, "... lands %.3f s after the button (model: 0.642 s)", s.firstHitAt);
        float lies[3];
        Middle(*loose::SpecFor("m4a1"), s.bone, lies);
        CheckF(lies[0] < -0.25f, "... its middle lies well to the side of the gun: %.2f m (model: 0.39 m)", -lies[0]);
        CheckF(s.restAt > 0.0f && s.restAt < 2.5f && s.lowest > -0.002f, "... at rest at %.2f s (within 2.5 s), never into the ground (deepest %.2f mm)", s.restAt, -s.lowest * 1000.0f);
    }
    {
        const Seen a = EjectRun("m4a1", down60, 3.0f), b = EjectRun("m4a1", upside, 3.0f);
        CheckF(a.firstHitAt > 0.0f && a.firstHitAt < 0.8f && a.lowest > -0.002f, "M4A1 pointed 60 degrees down: still out and down at %.3f s (model: 0.503 s)", a.firstHitAt);
        CheckF(b.firstHitAt > 0.0f && b.firstHitAt < 1.2f && b.lowest > -0.002f, "M4A1 upside down: out at the least speed, down at %.3f s (model: 0.812 s)", b.firstHitAt);
    }
    {
        const Seen s = EjectRun("m4a1", level, 3.0f, 1.5f);
        CheckF(s.bone[11] > 0.4f, "walking ahead 1.5 m/s: it lands ahead with the player's own speed, %.2f m (model: 0.69 m)", s.bone[11]);
    }
    {
        // The frame rate: 60 a second (the floor the user allows) and 36 (the headset's halved rate): the same fall, never into the ground.
        const Seen at72 = EjectRun("m4a1", level, 3.0f);
        for (float rate : {60.0f, 36.0f}) {
            loose::Clear();
            g_ground = Ground{};
            const loose::Spec& spec = *loose::SpecFor("m4a1");
            loose::Frame f = Base();
            f.dt = 1.0f / rate;
            Hold(f, level);
            float onGun[12];
            Identity(onGun);
            const int id = loose::Eject(spec, 1, onGun, kDown, WellOf("m4a1"), 0.0f, 0, f);
            loose::Step(f);
            float landed = -1.0f, lowest = 1e9f, restAt = -1.0f, time = 0.0f;
            for (int i = 0; i < static_cast<int>(3.0f * rate); ++i) {
                time += 1.0f / rate;
                loose::Frame g = Base();
                g.dt = 1.0f / rate;
                GunAt(g, level, 0.0f, 1.40f, 0.0f);
                const loose::Result& r = loose::Step(g);
                for (int k = 0; k < r.hits; ++k)
                    if (r.hit[k].id == id && landed < 0.0f)
                        landed = time;
                for (int k = 0; k < r.poses; ++k)
                    if (r.pose[k].id == id) {
                        lowest = std::fmin(lowest, Lowest(spec, r.pose[k].bone));
                        if (r.pose[k].resting && restAt < 0.0f)
                            restAt = time;
                    }
            }
            CheckF(std::fabs(landed - (at72.firstHitAt - kDt)) < 1.5f / rate && restAt > 0.0f && restAt < 2.0f && lowest > -0.002f,
                   "at %.0f frames a second: it lands %.3f s after the button's frame (at 72: %.3f), at rest at %.2f s, never into the ground (deepest %.2f mm)", rate,
                   landed, at72.firstHitAt - kDt, restAt, -lowest * 1000.0f);
        }
    }
    {
        // The gun swung (3 rad/s about up) as the button's pressed: it leaves with the swing, not as from a still gun.
        const Seen still = EjectRun("m4a1", level, 3.0f);
        loose::Clear();
        g_ground = Ground{};
        const loose::Spec& spec = *loose::SpecFor("m4a1");
        auto swung = [&](loose::Frame& g, float t) {
            float turn[12];
            Turn(turn, y, 3.0f * t);
            const float arm[3] = {0.0f, 0.0f, 0.5f};  // the gun 0.5 m out from the shoulder it swings about
            float at[3];
            Apply(turn, arm, at);
            GunAt(g, turn, at[0], 1.40f, at[2]);
        };
        loose::Frame f = Base();
        float time = -6.0f * kDt;
        for (int i = 0; i < 6; ++i) {
            time += kDt;
            swung(f, time);
            loose::Step(f);
        }
        float onGun[12];
        Identity(onGun);
        const int id = loose::Eject(spec, 1, onGun, kDown, WellOf("m4a1"), 0.0f, 0, f);
        loose::Step(f);
        const Seen s = Run(spec, id, 3.0f, [&](loose::Frame& g, float t) { swung(g, std::fmin(time + t, 0.3f)); });
        float a[3], b[3];
        Middle(spec, still.bone, a);
        Middle(spec, s.bone, b);
        CheckF(Distance(a, b) > 0.3f && s.lowest > -0.002f, "the gun swung at 3 rad/s as the button's pressed: it flies off with the swing, %.2f m from where a still gun drops it (model: 0.96 m out)", Distance(a, b));
    }
    {
        // The gun gone while it's still in the well (another gun taken): it goes on as it was going, no jump.
        loose::Clear();
        g_ground = Ground{};
        const loose::Spec& spec = *loose::SpecFor("m4a1");
        loose::Frame f = Base();
        Hold(f, side);
        float onGun[12];
        Identity(onGun);
        const int id = loose::Eject(spec, 1, onGun, kDown, WellOf("m4a1"), 0.0f, 0, f);
        loose::Step(f);
        float last[3] = {}, jump = 0.0f, step = 0.0f;
        bool have = false;
        const Seen s = Run(spec, id, 3.0f, [&](loose::Frame& g, float t) {
            if (t < 0.05f)
                GunAt(g, side, 0.0f, 1.40f, 0.0f);
        }, [&](const loose::Pose& p, float t, const loose::Frame&) {
            float m[3];
            Middle(spec, p.bone, m);
            if (have && t < 0.05f)
                step = std::fmax(step, Distance(m, last));
            if (have && t >= 0.05f && t < 0.05f + kDt)
                jump = Distance(m, last);
            std::memcpy(last, m, sizeof(last));
            have = true;
        });
        CheckF(jump < 2.0f * step + 0.002f && s.restAt > 0.0f && s.lowest > -0.002f,
               "the gun gone with it still in the well: it goes on from where it was (%.1f mm in that frame, %.1f a frame before), lands and rests", jump * 1000.0f, step * 1000.0f);
    }
    {
        // Broken numbers given: nothing starts.
        loose::Clear();
        loose::Frame f = Base();
        Hold(f, level);
        float bone[12];
        Identity(bone);
        bone[7] = std::nanf("");
        float onGun[12];
        Identity(onGun);
        const float none[3] = {0.0f, 0.0f, 0.0f}, nan3[3] = {std::nanf(""), 0.0f, 0.0f};
        const int a = loose::Release(*loose::SpecFor("m4a1"), 1, bone, 0, f);
        const int b = loose::Eject(*loose::SpecFor("m4a1"), 1, onGun, none, 0.08f, 0.0f, 0, f);
        const int c = loose::Flick(*loose::SpecFor("556"), 1, onGun, 0, nan3, kDown, 10.0f, f);
        f.gun = false;
        const int d = loose::Eject(*loose::SpecFor("m4a1"), 1, onGun, kDown, 0.08f, 0.0f, 0, f);
        CheckF(!a && !b && !c && !d && loose::Count() == 0, "broken numbers (a NaN, no way out, no gun to eject from): nothing starts (%d %d %d %d)", a, b, c, d);
    }
    for (const char* name : {"m416", "m16a4", "kh2002"}) {
        const Seen s = EjectRun(name, level, 4.0f);
        CheckF(s.restAt > 0.0f && s.restAt < 2.0f && s.lowest > -0.002f, "%s level: down and at rest at %.2f s, never into the ground (deepest %.2f mm)", name, s.restAt, -s.lowest * 1000.0f);
    }
    {
        // The pistol's: out of its grip along its line, raked 16 degrees back.
        loose::Clear();
        g_ground = Ground{};
        g_ground.surface = loose::kTarmac;
        const loose::Spec& spec = *loose::SpecFor("m9");
        loose::Frame f = Base();
        Hold(f, level);
        float onGun[12];
        Identity(onGun);
        const float raked[3] = {0.0f, -std::cos(16.0f * kDegree), -std::sin(16.0f * kDegree)};
        const int id = loose::Eject(spec, 6, onGun, raked, 0.1108f, 0.5f, 0, f);
        const Seen s = Run(spec, id, 3.0f, [&](loose::Frame& g, float) { GunAt(g, level, 0.0f, 1.40f, 0.0f); });
        CheckF(s.firstHitAt > 0.3f && s.restAt > 0.0f && s.restAt < 2.0f && s.lowest > -0.002f,
               "M9 out of its grip: lands at %.3f s, at rest at %.2f s, never into the ground (deepest %.2f mm; model: 0.451, 1.19)", s.firstHitAt, s.restAt, -s.lowest * 1000.0f);
    }
    {
        // Where it starts: where it was in the gun, not a frame on.
        loose::Clear();
        g_ground = Ground{};
        const loose::Spec& spec = *loose::SpecFor("m416");
        loose::Frame f = Base();
        const float tilt[3] = {0.6f, 0.0f, 0.8f};
        float turned[12];
        Turn(turned, tilt, 0.7f);
        Hold(f, turned);
        float onGun[12];
        Identity(onGun);
        onGun[7] = -0.01f;
        const int id = loose::Eject(spec, 1, onGun, kDown, 0.087f, 0.0f, 0, f);
        float seated[12];
        Compose(f.gunRoom, onGun, seated);
        const loose::Result& r = loose::Step(f);
        float off = 1e9f, placeOff = 1e9f;
        for (int k = 0; k < r.poses; ++k)
            if (r.pose[k].id == id) {
                const float a[3] = {seated[3], seated[7], seated[11]}, b[3] = {r.pose[k].bone[3], r.pose[k].bone[7], r.pose[k].bone[11]};
                off = AngleBetween(seated, r.pose[k].bone);
                placeOff = Distance(a, b);
            }
        CheckF(placeOff < 1e-4f && off < 1e-3f, "the first frame after the button: drawn just where it sat in the gun (%.3f mm, %.3f degrees off)", placeOff * 1000.0f, off / kDegree);
        // Started with last frame's Frame (skin.cpp's starts take the one the hands last gave), the gun 2 cm on and turned a little since: seated on it as now.
        loose::Clear();
        loose::Frame before = Base();
        Hold(before, turned);
        const int late = loose::Eject(spec, 1, onGun, kDown, 0.087f, 0.0f, 0, before);
        loose::Frame now = Base();
        const float little[3] = {0.0f, 1.0f, 0.0f};
        float on[12];
        Turn(on, little, 0.03f);
        float nowTurn[12];
        Compose(on, turned, nowTurn);
        GunAt(now, nowTurn, 0.02f, 1.40f, 0.0f);
        const loose::Result& r2 = loose::Step(now);
        Compose(now.gunRoom, onGun, seated);
        placeOff = 1e9f;
        for (int k = 0; k < r2.poses; ++k)
            if (r2.pose[k].id == late) {
                const float a[3] = {seated[3], seated[7], seated[11]}, b[3] = {r2.pose[k].bone[3], r2.pose[k].bone[7], r2.pose[k].bone[11]};
                off = AngleBetween(seated, r2.pose[k].bone);
                placeOff = Distance(a, b);
            }
        CheckF(placeOff < 1e-4f && off < 1e-3f, "... started with last frame's Frame, the gun moved 2 cm since: drawn in the gun as it is now (%.3f mm, %.3f degrees off)", placeOff * 1000.0f, off / kDegree);
    }

    // ---- Let go of, thrown, flicked ----
    for (const char* name : {"an94", "aek971", "ak74", "rpk", "g3a3", "m416", "m16a4", "m4a1", "kh2002", "m240", "m9", "aks74u", "ump45", "mk11", "m249", "saiga12k", "pkp"})
        for (int surface : {loose::kDirt, loose::kTarmac})
            for (int full : {0, 1}) {
                const loose::Spec& spec = *loose::SpecFor(name);
                const Seen s = DropRun(name, 0.95f, surface, full ? spec.capacity : 0);
                CheckF(s.firstHitAt > 0.0f && s.restAt > 0.0f && s.restAt < 2.5f && s.lowest > -0.002f,
                       "%s %s let go of on %s: lands at %.2f s, at rest at %.2f s, never 2 mm into the ground (deepest %.2f mm)", name, full ? "full" : "empty",
                       surface == loose::kDirt ? "dirt" : "tarmac", s.firstHitAt, s.restAt, -s.lowest * 1000.0f);
            }
    // The stick's turning isn't a throw: the room turning 3 rad/s under a still hand, the magazine falls straight.
    {
        loose::Clear();
        g_ground = Ground{};
        const loose::Spec& spec = *loose::SpecFor("m4a1");
        float time = 0.0f;
        auto frame = [&](loose::Frame& f, float t) {
            f.hand = true;
            Identity(f.handRoom);
            f.handRoom[3] = 0.45f, f.handRoom[7] = 1.1f;
            Turn(f.roomToArms, y, 3.0f * t);
        };
        loose::Frame f = Base();
        for (int i = 0; i < 6; ++i) {
            time += kDt;
            frame(f, time);
            loose::Step(f);
        }
        float bone[12];
        Compose(f.roomToArms, f.handRoom, bone);
        const int id = loose::Release(spec, 2, bone, 0, f);
        float from[3], to[3];
        Middle(spec, bone, from);
        const Seen s = Run(spec, id, 3.0f, [&](loose::Frame& g, float t) { frame(g, time + t); });
        Middle(spec, s.bone, to);
        const float moved = std::sqrt((to[0] - from[0]) * (to[0] - from[0]) + (to[2] - from[2]) * (to[2] - from[2]));
        CheckF(moved < 0.2f, "let go of while the stick turns the player 3 rad/s: it falls nearly straight, its middle %.2f m off (with the world's speed: 1 m)", moved);
    }
    // Thrown: the hand at 6 m/s, 30 degrees up, over its last frames.
    {
        loose::Clear();
        g_ground = Ground{};
        const loose::Spec& spec = *loose::SpecFor("an94");
        const float vy = 3.0f, vz = 5.196f;
        loose::Frame f = Base();
        f.hand = true;
        Identity(f.handRoom);
        for (int i = 0; i < 8; ++i) {
            const float t = static_cast<float>(i - 7) * kDt;
            f.handRoom[7] = 1.3f + vy * t, f.handRoom[11] = vz * t;
            loose::Step(f);
        }
        float bone[12];
        Identity(bone);
        bone[7] = 1.3f + 0.17f;
        const int id = loose::Release(spec, 2, bone, 0, f);
        const Seen s = Run(spec, id, 4.0f, Still);
        CheckF(s.bone[11] > 3.5f && s.bone[11] < 5.5f, "thrown 6 m/s at 30 degrees from 1.3 m: it lands %.2f m away (3.5-5.5; model: 4.3-4.9 m)", s.bone[11]);
        CheckF(s.restAt > 0.0f && s.lowest > -0.002f, "... and comes to rest at %.2f s, never into the ground (deepest %.2f mm)", s.restAt, -s.lowest * 1000.0f);
    }
    // Thrown, its speed as the controller's own sensors give it (input::HandVelocity): the same throw, though the hand's places held still behind the shoulder its last
    // frames (the user, 2026-11-05: a hard throw "sometimes locks itself behind my shoulder": its steps were held back as a tracking jump), and with a turn of the wrist.
    {
        loose::Clear();
        g_ground = Ground{};
        const loose::Spec& spec = *loose::SpecFor("an94");
        loose::Frame f = Base();
        f.hand = true;
        Identity(f.handRoom);
        f.handRoom[7] = 1.3f;
        for (int i = 0; i < 8; ++i)  // held still: its steps say nothing of the throw
            loose::Step(f);
        f.handSensed = true;
        f.handVelocity[0] = 0.0f, f.handVelocity[1] = 3.0f, f.handVelocity[2] = 5.196f;
        f.handSpin[0] = 12.0f, f.handSpin[1] = 0.0f, f.handSpin[2] = 0.0f;
        float bone[12];
        Identity(bone);
        bone[7] = 1.3f + 0.17f;
        const int id = loose::Release(spec, 2, bone, 0, f);
        int turns = 0;
        float lastUp = 1.0f;
        const Seen s = Run(spec, id, 4.0f, Still, [&](const loose::Pose& p, float, const loose::Frame&) {
            const float up = p.bone[5];  // its y axis' height: the sign flips each half turn end over end
            if ((up < 0.0f) != (lastUp < 0.0f))
                ++turns;
            lastUp = up;
        });
        CheckF(s.bone[11] > 3.5f && s.bone[11] < 5.5f, "thrown 6 m/s at 30 degrees by its controller's own sensors, its places held still: it lands %.2f m away (3.5-5.5)", s.bone[11]);
        CheckF(turns >= 2, "... tumbling end over end on the way (%d half turns)", turns);
        // Not given: the steps as before (held still: it drops).
        loose::Clear();
        g_ground = Ground{};
        f.handSensed = false;
        const int still = loose::Release(spec, 2, bone, 0, f);
        const Seen d = Run(spec, still, 3.0f, Still);
        CheckF(std::fabs(d.bone[11]) < 0.5f, "... and without them, by the held-still steps: it drops at the hand (%.2f m)", d.bone[11]);
    }
    // Thrown with a turn of the wrist: it tumbles through the air ("it rotates around the air when you throw it"), its spin kept till it lands.
    {
        loose::Clear();
        g_ground = Ground{};
        const loose::Spec& spec = *loose::SpecFor("m4a1");
        constexpr float kSpin = 12.0f;
        loose::Frame f = Base();
        f.hand = true;
        for (int i = 0; i < 8; ++i) {
            const float t = static_cast<float>(i - 7) * kDt;
            Turn(f.handRoom, x, -kSpin * t);
            f.handRoom[7] = 1.3f + 2.5f * t, f.handRoom[11] = 4.33f * t;
            loose::Step(f);
        }
        float inHand[12], bone[12];
        Identity(inHand);
        inHand[7] = 0.17f;
        Compose(f.handRoom, inHand, bone);
        const int id = loose::Release(spec, 2, bone, 0, f);
        float last[12] = {}, turned = 0.0f, firstRate = -1.0f, lastRate = 0.0f;
        bool have = false, landed = false;
        const Seen s = Run(spec, id, 4.0f, Still, [&](const loose::Pose& p, float, const loose::Frame&) {
            if (!landed && Lowest(spec, p.bone) < 0.003f)
                landed = true;
            if (have && !landed) {
                const float a = AngleBetween(last, p.bone);
                turned += a;
                if (firstRate < 0.0f)
                    firstRate = a / kDt;
                lastRate = a / kDt;
            }
            std::memcpy(last, p.bone, sizeof(last));
            have = true;
        });
        CheckF(turned > 2.0f * kPi, "thrown with a turn of the wrist (12 rad/s): it tumbles, %.1f turns in the air before it lands", turned / (2.0f * kPi));
        CheckF(firstRate > 0.0f && std::fabs(lastRate / firstRate - 1.0f) < 0.15f, "... its spin kept in the air: %.1f rad/s as it left, %.1f as it landed", firstRate, lastRate);
        CheckF(s.restAt > 0.0f && s.lowest > -0.002f, "... lands, at rest at %.2f s, never into the ground (deepest %.2f mm)", s.restAt, -s.lowest * 1000.0f);
    }
    // Spun about its middle axis (the M4A1's: its z, between its x and y), it flips over in the air, as a real one does; about its biggest (x) it doesn't.
    for (int about : {2, 0}) {
        loose::Clear();
        g_ground = Ground{};
        const loose::Spec& spec = *loose::SpecFor("m4a1");
        loose::Frame f = Base();
        float bone[12];
        Identity(bone);
        bone[7] = 200.0f;  // high: 4 s in the air
        float axis[3] = {about == 0 ? 1.0f : 0.02f, 0.0f, about == 2 ? 1.0f : 0.02f};
        const float none[3] = {0.0f, 0.0f, 0.0f};
        const int id = loose::Flick(spec, 2, bone, 0, none, axis, 12.0f, f);
        float last[12] = {};
        bool have = false;
        int flips = 0, sign = 0;
        Run(spec, id, 4.0f, Still, [&](const loose::Pose& p, float, const loose::Frame&) {
            if (have) {
                float w[3];
                TurnInOwn(last, p.bone, w);
                const int now = w[about] > 0.0f ? 1 : -1;
                if (sign && now != sign)
                    ++flips;
                sign = now;
            }
            std::memcpy(last, p.bone, sizeof(last));
            have = true;
        });
        if (about == 2)
            CheckF(flips >= 2, "spun about its middle axis, it flips over in the air: its spin about it turned round %d times in 4 s (model: 4)", flips);
        else
            CheckF(flips == 0, "spun about its biggest axis, it spins on steadily: turned round %d times in 4 s", flips);
    }
    // A tracking jump of 10 cm on the frame it's let go of: not a throw.
    {
        loose::Clear();
        g_ground = Ground{};
        const loose::Spec& spec = *loose::SpecFor("m4a1");
        loose::Frame f = Base();
        f.hand = true;
        Identity(f.handRoom);
        f.handRoom[7] = 1.1f;
        for (int i = 0; i < 7; ++i)
            loose::Step(f);
        f.handRoom[11] += 0.10f;
        loose::Step(f);
        float bone[12];
        Identity(bone);
        bone[7] = 1.27f;
        const int id = loose::Release(spec, 2, bone, 0, f);
        const Seen s = Run(spec, id, 3.0f, Still);
        CheckF(std::fabs(s.bone[11]) < 0.5f, "a 10 cm tracking jump as it's let go of: it doesn't fly off (%.2f m; the mean of three steps read 2.4 m/s)", std::fabs(s.bone[11]));
    }
    // The first frame after it's let go of: where it was drawn in the hand.
    {
        loose::Clear();
        g_ground = Ground{};
        const loose::Spec& spec = *loose::SpecFor("ak74");
        loose::Frame f = Base();
        f.hand = true;
        Identity(f.handRoom);
        for (int i = 0; i < 8; ++i) {
            f.handRoom[3] = 0.3f + 2.0f * kDt * static_cast<float>(i), f.handRoom[7] = 1.2f;
            loose::Step(f);
        }
        const float tilt[3] = {0.0f, 0.6f, 0.8f};
        float bone[12];
        Turn(bone, tilt, 1.1f);
        bone[3] = 0.4f, bone[7] = 1.35f, bone[11] = 0.2f;
        const int id = loose::Release(spec, 2, bone, 0, f);
        const loose::Result& r = loose::Step(f);
        float off = 1e9f;
        for (int k = 0; k < r.poses; ++k)
            if (r.pose[k].id == id) {
                off = 0.0f;
                for (int i = 0; i < 12; ++i)
                    off = std::fmax(off, std::fabs(r.pose[k].bone[i] - bone[i]));
            }
        CheckF(off < 1e-4f, "the first frame after it's let go of: drawn just where it was in the hand (%.5f off)", off);
    }
    // The flick: handmag's (-0.5 0.5 2.4) m/s, 11 rad/s bottom forward.
    {
        loose::Clear();
        g_ground = Ground{};
        const loose::Spec& spec = *loose::SpecFor("an94");
        loose::Frame f = Base();
        Hold(f, level);
        float bone[12];
        std::memcpy(bone, f.gunRoom, sizeof(bone));
        const float v[3] = {-0.5f, 0.5f, 2.4f}, axis[3] = {-1.0f, 0.0f, 0.0f};
        const int id = loose::Flick(spec, 3, bone, 0, v, axis, 11.0f, f);
        const Seen s = Run(spec, id, 4.0f, [&](loose::Frame& g, float) { GunAt(g, level, 0.0f, 1.40f, 0.0f); });
        CheckF(s.bone[11] > 1.0f && s.bone[11] < 2.5f, "the Afghan flick: the old one lands %.2f m ahead (1-2.5; model: 1.6 m)", s.bone[11]);
        CheckF(s.restAt > 0.0f && s.restAt < 2.5f && s.lowest > -0.002f, "... at rest at %.2f s, never into the ground (deepest %.2f mm)", s.restAt, -s.lowest * 1000.0f);
    }

    // ---- The ground ----
    {
        loose::Clear();
        g_ground = Ground{};
        g_ground.surface = loose::kTarmac;
        const loose::Spec& spec = *loose::SpecFor("m4a1");
        loose::Frame f = Base();
        float bone[12];
        Identity(bone);
        bone[7] = 0.9f;
        const int id = loose::Release(spec, 2, bone, 0, f);
        Seen s = Run(spec, id, 3.0f, Still);
        const float before = s.bone[7];
        g_ground.height = 0.03f;
        s = Run(spec, id, 1.5f, Still);
        CheckF(s.restAt > 0.0f && std::fabs(s.bone[7] - before - 0.03f) < 0.002f, "the game's ground under one at rest found 3 cm higher: it's lifted onto it and rests (%.1f mm up)", (s.bone[7] - before) * 1000.0f);
        g_ground.height = -0.05f;
        s = Run(spec, id, 1.5f, Still);
        CheckF(s.restAt > 0.0f && std::fabs(s.bone[7] - before + 0.05f) < 0.002f, "... found 5 cm lower: it falls onto it and rests (%.1f mm down)", (before - s.bone[7]) * 1000.0f);
        const float lying = s.bone[7];
        g_ground.firm = false;
        g_ground.height = 0.35f;  // the feet up a hill as the player walks on
        s = Run(spec, id, 1.0f, Still);
        const float up = s.bone[7] - lying;
        g_ground.height = -0.65f;  // the eye down a crouch before the stance says so
        s = Run(spec, id, 1.0f, Still);
        CheckF(std::fabs(up) < 1e-4f && std::fabs(s.bone[7] - lying) < 1e-4f && loose::Has(id),
               "a guess of the ground (the soldier's feet) 35 cm up, then 65 cm down, under one lying: it stays where it fell (%.2f, %.2f mm)", up * 1000.0f, (s.bone[7] - lying) * 1000.0f);
    }
    {
        // Dropped onto a guess, it lands on it all the same.
        loose::Clear();
        g_ground = Ground{};
        g_ground.firm = false;
        g_ground.height = 0.2f;
        const loose::Spec& spec = *loose::SpecFor("g3a3");
        loose::Frame f = Base();
        float bone[12];
        Identity(bone);
        bone[7] = 1.2f;
        const int id = loose::Release(spec, 2, bone, 0, f);
        const Seen s = Run(spec, id, 3.0f, Still);
        CheckF(s.restAt > 0.0f && s.lowest > -0.002f && s.lowest < 0.003f, "let go of over a guess of the ground: it lands on it and rests (%.1f mm above it)", s.lowest * 1000.0f);
    }
    for (float degrees : {15.0f, 35.0f}) {
        loose::Clear();
        g_ground = Ground{};
        g_ground.slope = degrees * kDegree;
        const loose::Spec& spec = *loose::SpecFor("m4a1");
        loose::Frame f = Base();
        float bone[12];
        Identity(bone);
        bone[7] = 0.8f;
        const int id = loose::Release(spec, 2, bone, 0, f);
        const Seen s = Run(spec, id, 4.0f, Still);
        float from[3], to[3];  // its middle where it was let go of and where it is (its bone's own place is 20 cm off its middle)
        Middle(spec, bone, from);
        Middle(spec, s.bone, to);
        const float away = std::sqrt((to[0] - from[0]) * (to[0] - from[0]) + (to[2] - from[2]) * (to[2] - from[2]));
        if (degrees < 20.0f)
            CheckF(s.restAt > 0.0f && away < 0.5f && s.lowest > -0.002f, "on 15 degrees of dirt: it stays where it lands (%.2f m off; model: stays)", away);
        else
            CheckF(away > 1.0f && s.lowest > -0.002f, "on 35 degrees of dirt: it slides away down it (%.2f m in 4 s; model: away), never into it", away);
    }
    {
        // The arms' space's origin jumping a metre as the player walks across: it stays put in the world.
        loose::Clear();
        g_ground = Ground{};
        const loose::Spec& spec = *loose::SpecFor("m416");
        loose::Frame f = Base();
        float bone[12];
        Identity(bone);
        bone[7] = 0.6f;
        const int id = loose::Release(spec, 2, bone, 0, f);
        Seen s = Run(spec, id, 2.0f, Still);
        const float place[3] = {s.bone[3], s.bone[7], s.bone[11]};
        float moved[3] = {};
        s = Run(spec, id, 1.0f, [](loose::Frame& g, float) { g.origin[0] = 1, g.origin[2] = -2; },
                [&](const loose::Pose& p, float, const loose::Frame&) { moved[0] = p.bone[3], moved[1] = p.bone[7], moved[2] = p.bone[11]; });
        CheckF(std::fabs(moved[0] - (place[0] - 1.0f)) < 1e-5f && std::fabs(moved[1] - place[1]) < 1e-5f && std::fabs(moved[2] - (place[2] + 2.0f)) < 1e-5f,
               "the arms' space's origin a metre over and two back: it stays put in the world (drawn %.3f %.3f %.3f from %.3f %.3f %.3f)", moved[0], moved[1], moved[2], place[0], place[1], place[2]);
    }
    {
        // No ground at all under it: it falls, and goes (not forever).
        loose::Clear();
        g_ground = Ground{};
        g_ground.nothing = true;
        const loose::Spec& spec = *loose::SpecFor("m4a1");
        loose::Frame f = Base();
        float bone[12];
        Identity(bone);
        bone[7] = 1.0f;
        const int id = loose::Release(spec, 2, bone, 0, f);
        const Seen s = Run(spec, id, 7.0f, Still);
        CheckF(s.gone && s.goneAt > 4.9f && s.goneAt < 5.5f, "nothing under it at all: it falls and is gone after %.2f s (5 s), not falling forever", s.goneAt);
    }

    // ---- Its time lying (kLieSeconds: 30 s since 2026-11-05, a minute before), then only unseen ----
    {
        // Looked away the whole time: it goes its time after it came to rest, not before.
        loose::Clear();
        g_ground = Ground{};
        const loose::Spec& spec = *loose::SpecFor("m4a1");
        loose::Frame f = Base();
        float bone[12];
        Identity(bone);
        bone[7] = 0.9f;
        const int id = loose::Release(spec, 2, bone, 0, f);
        const Seen s = Run(spec, id, loose::kLieSeconds + 5.0f, [](loose::Frame& g, float) { LookUp(g); });
        CheckF(s.gone && s.restAt > 0.0f && s.goneAt - s.restAt >= loose::kLieSeconds && s.goneAt - s.restAt < loose::kLieSeconds + 1.0f,
               "out of view from the start: it lies its %.0f s and goes %.2f s after it came to rest", loose::kLieSeconds, s.goneAt - s.restAt);
    }
    {
        // Looked at past its minute: it stays; looked away: it goes within a second; never while looked at.
        loose::Clear();
        g_ground = Ground{};
        const loose::Spec& spec = *loose::SpecFor("m4a1");
        loose::Frame f = Base();
        float bone[12];
        Identity(bone);
        bone[7] = 0.9f;
        const int id = loose::Release(spec, 2, bone, 0, f);
        const float at[3] = {0.0f, 0.0f, 0.0f};
        bool goneSeen = false;
        float lastSeenAt = 0.0f;
        const Seen s = Run(spec, id, 75.0f, [&](loose::Frame& g, float t) {
            if (t < 70.0f)
                LookAt(g, at), lastSeenAt = t;
            else
                LookUp(g);
        }, [&](const loose::Pose&, float, const loose::Frame&) {});
        goneSeen = s.gone && s.goneAt <= lastSeenAt + 1e-3f;
        CheckF(!goneSeen && s.gone && s.goneAt > 70.0f && s.goneAt < 71.0f,
               "looked at for 70 s: it stays past its time (never gone in view); looked away, it goes %.2f s later", s.goneAt - 70.0f);
    }
    {
        // The never-break film's look away (loosefilm.ps1 -Away, 2026-11-12): the head pitched 70 degrees down at a magazine lying at the feet, turned 150
        // degrees about the up for 2.5 s and back. Turning about the up hardly moves what's under the feet off the look: it stays in view and lies on
        // (the film's magazine lay 123 s). Looking ahead, 8 degrees up, it goes half a second later.
        loose::Clear();
        g_ground = Ground{};
        const loose::Spec& spec = *loose::SpecFor("m416");
        loose::Frame f = Base();
        float bone[12];
        Identity(bone);
        bone[3] = 0.2f, bone[7] = 0.9f, bone[11] = 0.2f;
        const int id = loose::Release(spec, 1, bone, 0, f);
        auto look = [](loose::Frame& g, float pitch, float yaw) {
            g.look[0] = std::cos(pitch * kDegree) * std::sin(yaw * kDegree);
            g.look[1] = std::sin(pitch * kDegree);
            g.look[2] = std::cos(pitch * kDegree) * std::cos(yaw * kDegree);
        };
        const Seen s = Run(spec, id, 82.0f, [&](loose::Frame& g, float t) {
            if (t < 63.0f || (t >= 65.5f && t < 80.0f))
                look(g, -70.0f, 45.0f);
            else if (t < 65.5f)
                look(g, -70.0f, 195.0f);
            else
                look(g, 8.0f, 45.0f);
        });
        const bool goneTurned = s.gone && s.goneAt < 80.0f;
        CheckF(s.restAt > 0.0f && !goneTurned && s.gone && s.goneAt > 80.0f && s.goneAt < 81.0f,
               "at the feet, the head 70 degrees down and turned 150 about the up: still in view, never gone; looked ahead it goes %.2f s later", s.goneAt - 80.0f);
    }
    {
        // Too many at once: the one at rest longest that nobody sees goes.
        loose::Clear();
        g_ground = Ground{};
        const loose::Spec& spec = *loose::SpecFor("ak74");
        int first = 0, second = 0;
        for (int k = 0; k <= loose::kMost; ++k) {
            loose::Frame f = Base();
            if (k == loose::kMost) {  // the first one in view at the end, alone (a narrow look): the second goes
                const float at[3] = {0.0f, 0.0f, 0.0f};
                LookAt(f, at);
                f.seenCos = 0.995f;
                for (int i = 0; i < 2; ++i)
                    loose::Step(f);
            }
            float bone[12];
            Identity(bone);
            bone[3] = 0.3f * static_cast<float>(k), bone[7] = 0.4f;
            const int id = loose::Release(spec, 2, bone, 0, f);
            if (k == 0)
                first = id;
            if (k == 1)
                second = id;
            for (int i = 0; i < 150 && k < loose::kMost; ++i)
                loose::Step(f);
        }
        CheckF(loose::Count() == loose::kMost, "one more than %d let go of: still %d", loose::kMost, loose::Count());
        CheckF(loose::Has(first) && !loose::Has(second), "... the one at rest longest out of view is the one gone (the first, in view, stays)");
    }

    // What ten magazines falling and settling at once cost a frame (the most there can be): the first ten guns', then a ten with the newest two (the AKS-74u's and the UMP-45's) in the
    // KH2002's and the M240's places.
    for (int set = 0; set < 2; ++set) {
        loose::Clear();
        g_ground = Ground{};
        g_ground.surface = loose::kTarmac;
        loose::Frame f = Base();
        const char* names[2][10] = {{"an94", "aek971", "ak74", "rpk", "g3a3", "m416", "m16a4", "m4a1", "kh2002", "m240"},
                                    {"an94", "aek971", "ak74", "rpk", "g3a3", "m416", "m16a4", "m4a1", "aks74u", "ump45"}};
        for (int k = 0; k < loose::kMost; ++k) {
            const float tilt[3] = {0.6f, 0.0f, 0.8f};
            float bone[12];
            Turn(bone, tilt, 0.4f * static_cast<float>(k));
            bone[3] = 0.4f * static_cast<float>(k), bone[7] = 1.2f;
            loose::Release(*loose::SpecFor(names[set][k]), 2, bone, 0, f);
        }
        double micros = 0.0, slowest = 0.0;
        long long moving = 0;
        for (int i = 0; i < 4 * 72; ++i) {
            const auto before = std::chrono::steady_clock::now();
            const loose::Result& r = loose::Step(f);
            const double took = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - before).count();
            int now = 0;
            for (int k = 0; k < r.poses; ++k)
                now += !r.pose[k].resting;
            if (now)
                micros += took, moving += now, slowest = std::fmax(slowest, took);
        }
        const double each = moving ? micros / static_cast<double>(moving) : 0.0;
        CheckF(each < 25.0 && loose::Resting() == loose::kMost, "ten magazines let go of at once (%s): %.1f us a frame for each moving one, the slowest frame %.0f us; all %d at rest after 4 s",
               set ? "with the AKS-74u's and the UMP-45's" : "the first ten guns'", each, slowest, loose::Resting());
    }

    // ---- The launcher's case: a tube with a flange rolls on a 6 degree slope, stays on 3 ----
    for (float degrees : {3.0f, 6.0f}) {
        loose::Clear();
        g_ground = Ground{};
        g_ground.slope = degrees * kDegree;
        const loose::Spec& spec = *loose::SpecFor("smaw");
        loose::Frame f = Base();
        float bone[12];
        Turn(bone, y, kPi / 2);  // its axis along x, across the slope
        bone[7] = 1.0f;
        const int id = loose::Release(spec, 4, bone, 0, f);
        const Seen s = Run(spec, id, 8.0f, Still);
        if (degrees > 5.0f)
            CheckF(std::fabs(s.bone[11]) > 0.5f && s.lowest > -0.002f, "the launcher's case on 6 degrees of dirt: rolls down %.2f m (model: 1.5 m), never into it", std::fabs(s.bone[11]));
        else
            CheckF(std::fabs(s.bone[11]) < 0.2f && s.restAt > 0.0f && s.lowest > -0.002f, "the launcher's case on 3 degrees of dirt: stays (%.2f m; model: 1 cm), at rest at %.2f s", std::fabs(s.bone[11]), s.restAt);
    }

    // ---- A shotgun's live shell let go of (shellload.h): it lands, rolls and comes to rest, never into the ground ----
    for (int surface : {loose::kDirt, loose::kTarmac}) {
        const Seen s = DropRun("12g", 0.95f, surface, 0);
        CheckF(s.firstHitAt > 0.0f && s.restAt > 0.0f && s.restAt < 4.0f && s.lowest > -0.002f,
               "a 12-gauge shell let go of on %s: lands at %.2f s, at rest at %.2f s, never 2 mm into the ground (deepest %.2f mm)",
               surface == loose::kDirt ? "dirt" : "tarmac", s.firstHitAt, s.restAt, -s.lowest * 1000.0f);
    }

    // ---- The M320's spent case in its open barrel ("If the M320 is open, then it wants to") ----
    {
        const loose::Spec& spec = *loose::SpecFor("m320");
        auto start = [&](float pitch, loose::Frame& f) {
            loose::Clear();
            g_ground = Ground{};
            f = Base();
            Pitched(f, pitch);
            for (int i = 0; i < 6; ++i)
                loose::Step(f);
            const int id = loose::TipOut(spec, 7, kCaseSeat, kCaseOut, kCaseLength, kCaseGrip, f);
            loose::Step(f);  // as the caller does: the start, then that frame's step
            return id;
        };
        // Level, with the tracking's shake: it stays in.
        {
            loose::Frame f;
            const int id = start(0.0f, f);
            float most = 0.0f;
            bool out = false;
            Run(spec, id, 4.0f, [](loose::Frame& g, float) {
                const float shake[3] = {0.0003f * Noise(), 0.0003f * Noise(), 0.0003f * Noise()};
                Pitched(g, 0.0f, shake);
            }, [&](const loose::Pose& p, float, const loose::Frame& g) {
                most = std::fmax(most, OffSeat(p, g));
                out |= !p.inGun;
            });
            CheckF(!out && most < 0.001f, "the M320 open and level, the tracking shaking it 0.3 mm a frame: the case stays in (%.2f mm off its seat at most, 4 s)", most * 1000.0f);
        }
        for (float degrees : {20.0f, 25.0f}) {
            loose::Frame f;
            const int id = start(degrees * kDegree, f);
            float most = 0.0f;
            bool out = false;
            Run(spec, id, 3.0f, [&](loose::Frame& g, float) { Pitched(g, degrees * kDegree); }, [&](const loose::Pose& p, float, const loose::Frame& g) {
                most = std::fmax(most, OffSeat(p, g));
                out |= !p.inGun;
            });
            CheckF(!out && most < 0.001f, "... the muzzle raised %.0f degrees: still in (%.2f mm off its seat)", degrees, most * 1000.0f);
        }
        {
            loose::Frame f;
            const int id = start(35.0f * kDegree, f);
            float outAt = -1.0f;
            const Seen s = Run(spec, id, 5.0f, [](loose::Frame& g, float) { Pitched(g, 35.0f * kDegree); }, [&](const loose::Pose& p, float t, const loose::Frame&) {
                if (!p.inGun && outAt < 0.0f)
                    outAt = t;
            });
            CheckF(outAt > 0.0f && outAt < 0.6f, "... raised 35 degrees: it slides out of its own weight, out %.2f s after (model of the slide: 0.22 s)", outAt);
            CheckF(s.firstHitAt > outAt && s.lowest > -0.002f, "... falls and lands at %.2f s, never into the ground (deepest %.2f mm)", s.firstHitAt, -s.lowest * 1000.0f);
            CheckF(s.restAt > 0.0f && s.restAt < 4.0f, "... and lies at rest at %.2f s", s.restAt);
        }
        {
            // Raised slowly, 10 degrees a second: where it starts to come.
            loose::Frame f;
            const int id = start(0.0f, f);
            float goesAt = -1.0f;
            Run(spec, id, 6.0f, [](loose::Frame& g, float t) { Pitched(g, std::fmin(10.0f * t, 60.0f) * kDegree); }, [&](const loose::Pose& p, float t, const loose::Frame& g) {
                if (goesAt < 0.0f && OffSeat(p, g) > 0.001f)
                    goesAt = 10.0f * t;
            });
            CheckF(goesAt > 28.0f && goesAt < 33.0f, "... the muzzle raised slowly: it starts to come out at %.1f degrees (about 30)", goesAt);
        }
        {
            // Level, the gun shoved toward its muzzle: hard (15 m/s^2 for 0.15 s), the case is left behind and comes out; gently (3), it stays.
            for (float shove : {15.0f, 3.0f}) {
                loose::Frame f;
                const int id = start(0.0f, f);
                bool out = false;
                Run(spec, id, 1.5f, [&](loose::Frame& g, float t) {
                    const float a = shove, t1 = std::fmin(t, 0.15f), t2 = std::fmin(std::fmax(t - 0.15f, 0.0f), 0.15f);
                    const float along = 0.5f * a * t1 * t1 + a * 0.15f * t2 - 0.5f * a * t2 * t2;  // up to speed, then stopped
                    const float at[3] = {0.0f, 0.0f, along};
                    Pitched(g, 0.0f, at);
                }, [&](const loose::Pose& p, float, const loose::Frame&) { out |= !p.inGun; });
                if (shove > 10.0f)
                    CheckF(out, "... level, the gun shoved forward hard (15 m/s^2 for 0.15 s): the case is left behind, out of the barrel");
                else
                    CheckF(!out, "... shoved gently (3 m/s^2): it stays in");
            }
        }
        {
            loose::Frame f;
            const int id = start(0.0f, f);
            loose::Step(f);
            const bool removed = loose::Remove(id);
            CheckF(removed && !loose::Has(id) && loose::Count() == 0, "... the barrel shut on it: taken back (Remove), no longer there");
        }
        {
            // A caller's own spec, built on the stack each time (skin.cpp's: the whole round as its mesh draws it, 11.9 cm, as the game's reload pulls it out):
            // its shape built from what's in it, not where it is; out at 35 degrees past its whole length, and it lies on the ground, not in it.
            auto round = [](float length) {
                loose::Spec s{};
                s.name = "m320 round as drawn";
                s.round = 2, s.rings = 2;
                s.ringAt[1] = length;
                s.ringRadius[0] = s.ringRadius[1] = 0.0255f;
                s.head = 0.35f, s.massEmpty = 0.06f, s.bounce = 0.8f;
                return s;
            };
            for (float length : {0.119f, 0.046f}) {
                const loose::Spec drawn = round(length);
                loose::Clear();
                g_ground = Ground{};
                loose::Frame f = Base();
                Pitched(f, 35.0f * kDegree);
                for (int i = 0; i < 6; ++i)
                    loose::Step(f);
                const int id = loose::TipOut(drawn, 3, kCaseSeat, kCaseOut, length, kCaseGrip, f);
                loose::Step(f);
                float outAt = -1.0f;
                const Seen s = Run(drawn, id, 4.0f, [](loose::Frame& g, float) { Pitched(g, 35.0f * kDegree); }, [&](const loose::Pose& p, float t, const loose::Frame&) {
                    if (!p.inGun && outAt < 0.0f)
                        outAt = t;
                });
                CheckF(outAt > 0.0f && outAt < 0.7f && s.firstHitAt > outAt && s.restAt > 0.0f && s.lowest > -0.002f,
                       "... a caller's own spec (%.1f cm long, made afresh each time): out at 35 degrees %.2f s after, lands, at rest at %.2f s, never into the ground (deepest %.2f mm)",
                       length * 100.0f, outAt, s.restAt, -s.lowest * 1000.0f);
            }
        }
        {
            // The gun on its side with the barrel level: still in.
            loose::Frame f;
            loose::Clear();
            g_ground = Ground{};
            f = Base();
            GunAt(f, side, 0.0f, 1.3f, 0.0f);
            for (int i = 0; i < 6; ++i)
                loose::Step(f);
            const int id = loose::TipOut(spec, 7, kCaseSeat, kCaseOut, kCaseLength, kCaseGrip, f);
            bool out = false;
            Run(spec, id, 2.0f, [&](loose::Frame& g, float) { GunAt(g, side, 0.0f, 1.3f, 0.0f); }, [&](const loose::Pose& p, float, const loose::Frame&) { out |= !p.inGun; });
            CheckF(!out, "... the gun rolled onto its side, the barrel level: still in");
        }
    }

    // ---- Casings: the port's, one every 0.1 s (600 a minute), by the dozen ----
    {
        loose::Clear();
        g_ground = Ground{};
        g_ground.surface = loose::kTarmac;
        const loose::Spec& spec = *loose::SpecFor("556");
        loose::Frame f = Base();
        GunAt(f, level, 0.0f, 1.3f, 0.0f);
        for (int i = 0; i < 6; ++i)
            loose::Step(f);
        struct Casing {
            int id = 0;
            float firstHit = -1.0f, restAt = -1.0f, lowest = 1e9f;
            float place[3] = {};
            bool gone = false;
        };
        std::vector<Casing> casings;
        double movingMicros = 0.0, restingMicros = 0.0, slowest = 0.0;
        long long movingThings = 0;
        int restingFrames = 0, mostMoving = 0;
        float time = 0.0f, lastOut = 0.0f;
        for (int frame = 0; frame < static_cast<int>(11.0f / kDt); ++frame) {
            time += kDt;
            loose::Frame g = Base();
            GunAt(g, level, 0.0f, 1.3f, 0.0f);
            if (casings.size() < 60 && time >= 0.1f * static_cast<float>(casings.size())) {
                float bone[12];
                Identity(bone);
                bone[3] = -0.02f, bone[7] = 1.33f, bone[11] = 0.30f;
                const float v[3] = {-2.5f + 0.3f * Noise(), 1.0f + 0.3f * Noise(), -0.3f + 0.2f * Noise()}, axis[3] = {0.1f * Noise(), 1.0f, 0.1f * Noise()};
                Casing c;
                c.id = loose::Flick(spec, 5, bone, 0, v, axis, 40.0f, g);
                casings.push_back(c);
                lastOut = time;
            }
            const auto before = std::chrono::steady_clock::now();
            const loose::Result& r = loose::Step(g);
            const double micros = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - before).count();
            int moving = 0;
            for (int k = 0; k < r.poses; ++k)
                moving += !r.pose[k].resting;
            mostMoving = std::max(mostMoving, moving);
            if (moving > 0)
                movingMicros += micros, movingThings += moving;
            else if (casings.size() == 60)
                restingMicros += micros, ++restingFrames;
            slowest = std::fmax(slowest, micros);
            for (Casing& c : casings) {
                for (int k = 0; k < r.hits; ++k)
                    if (r.hit[k].id == c.id && c.firstHit < 0.0f)
                        c.firstHit = time;
                for (int k = 0; k < r.gone; ++k)
                    c.gone |= r.goneId[k] == c.id;
                for (int k = 0; k < r.poses; ++k)
                    if (r.pose[k].id == c.id) {
                        c.lowest = std::fmin(c.lowest, Lowest(spec, r.pose[k].bone));
                        if (r.pose[k].resting && c.restAt < 0.0f)
                            c.restAt = time;
                        c.place[0] = r.pose[k].bone[3], c.place[1] = r.pose[k].bone[7], c.place[2] = r.pose[k].bone[11];
                    }
            }
        }
        int landed = 0, rested = 0, inside = 0, gone = 0, right = 0;
        float deepest = 1e9f, lastRest = 0.0f;
        for (const Casing& c : casings) {
            landed += c.firstHit > 0.0f;
            rested += c.restAt > 0.0f;
            lastRest = std::fmax(lastRest, c.restAt);
            deepest = std::fmin(deepest, c.lowest);
            inside += c.lowest > -0.002f;
            gone += c.gone;
            right += c.place[0] < -0.3f && c.place[0] > -5.0f;
        }
        CheckF(landed == 60 && gone == 0, "60 casings out of the port, 10 a second: %d of them land, %d gone", landed, gone);
        CheckF(rested == 60 && lastRest - lastOut < 4.0f, "... %d lie at rest, the last %.2f s after it came out", rested, lastRest - lastOut);
        CheckF(inside == 60, "... none ever more than 2 mm into the ground (the deepest %.2f mm)", -deepest * 1000.0f);
        CheckF(right >= 54, "... %d of them lie 0.3-5 m out to the right of the gun, as thrown", right);
        const double perThing = movingThings ? movingMicros / static_cast<double>(movingThings) : 0.0;
        const double resting = restingFrames ? restingMicros / restingFrames : 0.0;
        CheckF(perThing < 8.0 && resting < 40.0,
               "... cheap: %.2f us a frame for each moving one (%d moving at most), %.1f us a frame with all 60 lying, the slowest frame %.0f us", perThing, mostMoving, resting, slowest);
        // A box of them: no more than kCasings, and the big ones' room untouched.
        loose::Clear();
        loose::Frame h = Base();
        float bone[12];
        Identity(bone);
        bone[7] = 0.5f;
        const int magazine = loose::Release(*loose::SpecFor("m416"), 1, bone, 0, h);
        for (int k = 0; k < loose::kCasings + 20; ++k) {
            bone[3] = 0.05f * static_cast<float>(k % 40), bone[11] = 0.05f * static_cast<float>(k / 40);
            const float v[3] = {0.0f, 0.0f, 0.0f}, axis[3] = {0.0f, 1.0f, 0.0f};
            loose::Flick(spec, 5, bone, 0, v, axis, 0.0f, h);
            for (int i = 0; i < 4; ++i)
                loose::Step(h);
        }
        CheckF(loose::Count() <= loose::kCasings + 1 && loose::Has(magazine), "%d casings let go of: %d there (at most %d and the magazine), the magazine still there",
               loose::kCasings + 20, loose::Count(), loose::kCasings + 1);
    }

    // ---- The player's rifle (gunbody.h): kept, its own hull, let go of by either hand ----
    {
        const loose::Spec& gun = gunbody::kM416;
        const loose::Spec& stanag = *loose::SpecFor("m416");
        constexpr int kGunKind = 5;  // hands.cpp's kLooseGun
        const float kNan = std::numeric_limits<float>::quiet_NaN(), kInf = std::numeric_limits<float>::infinity();
        // A rifle's step over a frame from still: gravity's alone (what's taken off a thrown one's to read its speed).
        float gravity[3] = {};
        bool haveGravity = false;
        {
            loose::Clear();
            g_ground = Ground{};
            float bone[12];
            GunBone(0.0f, 5.0f, bone);
            const int still = loose::ReleaseBy(gun, kGunKind, bone, 0, Base(), 1);
            float step[1][3];
            haveGravity = FirstSteps(gun, &still, 1, step);
            std::memcpy(gravity, step[0], sizeof(gravity));
        }

        // g1, g2: dropped from a still hand, its lowest point 1 m up: upright, rolled 2 degrees onto its right and onto its left, it tips over onto a
        // side (its bottom's points are all at x 0); dropped on either side, it stays on it.
        for (float roll : {2.0f, -2.0f, 90.0f, -90.0f}) {
            loose::Clear();
            g_ground = Ground{};
            float bone[12];
            GunBone(roll * kDegree, 1.0f, bone);
            const int id = loose::ReleaseBy(gun, kGunKind, bone, 30, Base(), 1);
            const Seen s = Run(gun, id, 4.0f, Still);
            const float across = s.bone[4];  // its x (its left) against up: +1 lying on its right side, -1 on its left
            const bool upright = std::fabs(roll) < 45.0f;
            CheckF(id && s.restAt > 0.0f && s.restAt < 3.0f && std::fabs(across) > 0.9f && s.lowest > -0.002f,
                   "g%d the rifle dropped from 1 m %s, rolled %.0f degrees onto its %s: at rest at %.2f s (within 3), lying on its %s side (its x %.2f of up), never a hull "
                   "point 2 mm into the ground (deepest %.2f mm)",
                   upright ? 1 : 2, upright ? "upright" : "on its side", std::fabs(roll), roll > 0.0f ? "right" : "left", s.restAt, across > 0.0f ? "right" : "left", across,
                   -s.lowest * 1000.0f);
        }
        // g1 ... and near upright every way (its lowest point 5 cm, 30 cm and 1 m up; pitched -20 to 20 degrees; rolled -6 to 6 by 1): never left
        // standing on its bottom. (With the butt's heel at x +-0.03, as first estimated, 9 of 500 such drops stood on it and the magazine.)
        {
            int runs = 0, standing = 0, unrested = 0;
            for (float height : {0.05f, 0.3f, 1.0f})
                for (float pitch : {-20.0f, -5.0f, 0.0f, 5.0f, 20.0f})
                    for (int roll = -6; roll <= 6; ++roll) {
                        loose::Clear();
                        g_ground = Ground{};
                        float rolled[12], pitched[12], bone[12];
                        Turn(rolled, z, static_cast<float>(roll) * kDegree);
                        Turn(pitched, x, pitch * kDegree);
                        Compose(pitched, rolled, bone);
                        bone[7] = height - Lowest(gun, bone);
                        const int id = loose::ReleaseBy(gun, kGunKind, bone, 30, Base(), 1);
                        const Seen s = Run(gun, id, 4.0f, Still);
                        ++runs;
                        standing += s.bone[5] > 0.7f;  // its y (its top) still up
                        unrested += s.restAt < 0.0f;
                    }
            CheckF(standing == 0 && unrested == 0, "g1 ... dropped %d ways near upright: %d left standing on its bottom, %d not at rest after 4 s", runs, standing,
                   unrested);
        }
        // g2 ... thrown every which way (200 throws, the same each run: any turn, 0.3 to 1.5 m up, its hand at up to 1.5 m/s and 6 rad/s about each
        // axis): every one at rest within 4 s, none standing on its bottom (on a side, or upside down on its scope), never a hull point 2 mm into the
        // ground; and what it costs while it moves.
        {
            const unsigned noiseWas = g_noise;
            g_noise = 777u;
            int standing = 0, late = 0;
            float latest = 0.0f, deepest = 1e9f;
            double micros = 0.0;
            long long moving = 0;
            for (int n = 0; n < 200; ++n) {
                loose::Clear();
                g_ground = Ground{};
                float axis[3] = {Noise(), Noise(), Noise()};
                const float l = std::fmax(Length(axis), 1e-3f);
                for (float& a : axis)
                    a /= l;
                float bone[12];
                Turn(bone, axis, kPi * (Noise() + 1.0f));
                bone[7] = 0.3f + 0.6f * (Noise() + 1.0f) - Lowest(gun, bone);
                loose::Frame f = Base();
                loose::HandAt& h = f.hands[1];
                h.tracked = h.sensed = true;
                Identity(h.room);
                h.room[7] = bone[7];
                for (int i = 0; i < 3; ++i)
                    h.velocity[i] = 1.5f * Noise(), h.spin[i] = 6.0f * Noise();
                const int id = loose::ReleaseBy(gun, kGunKind, bone, 30, f, 1);
                float restAt = -1.0f, time = 0.0f, last[12] = {};
                for (int i = 0; i < static_cast<int>(6.0f / kDt); ++i) {
                    time += kDt;
                    const auto before = std::chrono::steady_clock::now();
                    const loose::Result& r = loose::Step(Base());
                    const double took = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - before).count();
                    for (int k = 0; k < r.poses; ++k)
                        if (r.pose[k].id == id) {
                            std::memcpy(last, r.pose[k].bone, sizeof(last));
                            deepest = std::fmin(deepest, Lowest(gun, last));
                            if (r.pose[k].resting && restAt < 0.0f)
                                restAt = time;
                            if (!r.pose[k].resting)
                                micros += took, ++moving;
                        }
                }
                standing += last[5] > 0.7f;
                late += restAt < 0.0f || restAt > 4.0f;
                latest = std::fmax(latest, restAt < 0.0f ? 99.0f : restAt);
            }
            g_noise = noiseWas;
            const double each = moving ? micros / static_cast<double>(moving) : 0.0;
            CheckF(standing == 0 && late == 0 && deepest > -0.002f && each < 40.0,
                   "g2 ... thrown 200 ways: %d left standing on its bottom, %d not at rest within 4 s (the latest %.2f s), the deepest hull point %.2f mm in; %.1f us a "
                   "frame while it moves",
                   standing, late, latest, -deepest * 1000.0f, each);
        }

        // g3: lying 120 s, looked away (four times a magazine's time): never gone; a magazine let go of beside it goes at its time. Sliding down a 60
        // degree slope of metal 40 s, never at rest, looked away (past the 30 s anything free may go on for): still there; a magazine beside it goes.
        for (int sliding : {0, 1}) {
            loose::Clear();
            g_ground = Ground{};
            float bone[12], mag[12];
            GunBone(kPi / 2, sliding ? 1.5f : 0.2f, bone);  // (over the slope's high end too)
            Identity(mag);
            mag[3] = 1.5f, mag[7] = 0.5f;
            if (sliding)
                g_ground.slope = 60.0f * kDegree, g_ground.surface = loose::kMetal;
            const int id = loose::ReleaseBy(gun, kGunKind, bone, 30, Base(), 1);
            const int magazine = loose::Release(stanag, 1, mag, 0, Base());
            bool told = false, resting = false, rested = false, finite = true;
            float magazineGone = -1.0f, time = 0.0f;
            const float seconds = sliding ? 40.0f : 120.0f;
            for (int i = 0; i < static_cast<int>(seconds / kDt); ++i) {
                time += kDt;
                loose::Frame g = Base();
                LookUp(g);
                const loose::Result& r = loose::Step(g);
                for (int k = 0; k < r.gone; ++k) {
                    told |= r.goneId[k] == id;
                    if (r.goneId[k] == magazine && magazineGone < 0.0f)
                        magazineGone = time;
                }
                for (int k = 0; k < r.poses; ++k)
                    if (r.pose[k].id == id)
                        resting = r.pose[k].resting, rested |= resting, finite &= AllFinite(r.pose[k].bone, 12);
            }
            if (!sliding)
                CheckF(id && magazine && !told && loose::Has(id) && resting && magazineGone > loose::kLieSeconds && magazineGone < loose::kLieSeconds + 3.0f,
                       "g3 the rifle lying 120 s, looked away: still there (%s), never told gone, at rest; a magazine beside it gone at %.1f s (its %.0f s once at rest)",
                       loose::Has(id) ? "yes" : "no", magazineGone, loose::kLieSeconds);
            else
                CheckF(id && magazine && !told && loose::Has(id) && !rested && finite && magazineGone > 29.5f && magazineGone < 31.0f,
                       "g3 the rifle sliding down 60 degrees of metal 40 s, never at rest, looked away: still there (%s), never told gone, its numbers fine; a magazine "
                       "sliding beside it gone at %.1f s (30 s free)",
                       loose::Has(id) ? "yes" : "no", magazineGone);
        }

        // g4: the big ones' room full (the rifle and nine magazines lying), ten more magazines let go of, five looked away and five in view: each pushes
        // out a magazine, never the rifle, though it's lain longest of all.
        {
            loose::Clear();
            g_ground = Ground{};
            int rifleOut = 0, pushed = 0, id = 0;
            auto settle = [&](float seconds, bool inView) {
                for (int i = 0; i < static_cast<int>(seconds / kDt); ++i) {
                    loose::Frame g = Base();
                    if (inView)
                        g.seenCos = -1.0f;  // everything in view
                    else
                        LookUp(g);
                    const loose::Result& r = loose::Step(g);
                    for (int k = 0; k < r.gone; ++k)
                        ++(r.goneId[k] == id ? rifleOut : pushed);
                }
            };
            float bone[12];
            GunBone(kPi / 2, 0.2f, bone);
            id = loose::ReleaseBy(gun, kGunKind, bone, 0, Base(), 1);
            settle(3.0f, false);
            for (int k = 0; k < loose::kMost - 1; ++k) {
                float mag[12];
                Identity(mag);
                mag[3] = 0.3f * static_cast<float>(k + 1), mag[7] = 0.4f;
                loose::Release(stanag, 1, mag, 0, Base());
                settle(1.5f, false);
            }
            const int full = loose::Count(), fullResting = loose::Resting();
            int started = 0;
            for (int k = 0; k < 10; ++k) {
                float mag[12];
                Identity(mag);
                mag[3] = -0.3f * static_cast<float>(k + 1), mag[7] = 0.4f;
                started += loose::Release(stanag, 1, mag, 0, Base()) != 0;
                settle(1.5f, k >= 5);
            }
            CheckF(id && full == loose::kMost && fullResting == loose::kMost && started == 10 && pushed == 10 && rifleOut == 0 && loose::Has(id) &&
                       loose::Count() == loose::kMost,
                   "g4 the rifle and nine magazines lying, ten more magazines let go of (five looked away, five in view): %d started, %d magazines pushed out, the rifle %s",
                   started, pushed, rifleOut == 0 && loose::Has(id) ? "never" : "PUSHED OUT");
        }
        // ... all ten moving (just let go of, high): no room for the rifle (the caller keeps it in the hand), nothing pushed out for it; nor room for a
        // magazine with the rifle lying among nine moving ones.
        {
            loose::Clear();
            g_ground = Ground{};
            for (int k = 0; k < loose::kMost; ++k) {
                float mag[12];
                Identity(mag);
                mag[3] = 0.3f * static_cast<float>(k), mag[7] = 5.0f;
                loose::Release(stanag, 1, mag, 0, Base());
            }
            float bone[12];
            GunBone(0.0f, 5.0f, bone);
            const int none = loose::ReleaseBy(gun, kGunKind, bone, 0, Base(), 1);
            const int goneThen = loose::Step(Base()).gone, countThen = loose::Count();
            loose::Clear();
            GunBone(kPi / 2, 0.2f, bone);
            const int id = loose::ReleaseBy(gun, kGunKind, bone, 0, Base(), 1);
            Run(gun, id, 3.0f, Still);
            for (int k = 0; k < loose::kMost - 1; ++k) {
                float mag[12];
                Identity(mag);
                mag[3] = 0.3f * static_cast<float>(k + 1), mag[7] = 5.0f;
                loose::Release(stanag, 1, mag, 0, Base());
            }
            float mag[12];
            Identity(mag);
            mag[7] = 5.0f;
            const int extra = loose::Release(stanag, 1, mag, 0, Base());
            const int goneAfter = loose::Step(Base()).gone;
            CheckF(none == 0 && goneThen == 0 && countThen == loose::kMost && id && extra == 0 && goneAfter == 0 && loose::Has(id),
                   "g4 ten magazines moving: no room for the rifle (%d), nothing pushed out for it (%d gone); the rifle lying among nine moving: no room for a magazine "
                   "(%d), the rifle stays",
                   none, goneThen, extra);
        }

        // g5: let go of by the right hand, its sensors' speed (1 2 -3) m/s and turn (0 0 6) rad/s, walking (0.4 0 0.6): its middle leaves at that speed
        // plus the turn times its arm from the grip point, plus the walking (within 1%); the left hand's own numbers (another throw) not used.
        {
            loose::Clear();
            g_ground = Ground{};
            const float up[3] = {0.0f, 1.0f, 0.0f};
            float bone[12];
            Turn(bone, up, 0.3f);
            bone[3] = 0.2f, bone[7] = 5.0f, bone[11] = 0.1f;
            loose::Frame f = Base();
            f.walk[0] = 0.4f, f.walk[2] = 0.6f;
            f.hand = f.handSensed = true;
            Identity(f.handRoom);
            f.handRoom[3] = -0.3f, f.handRoom[7] = 4.9f;
            f.handVelocity[0] = -4.0f, f.handSpin[1] = 5.0f;
            f.hands[0].tracked = f.hands[0].sensed = true;
            std::memcpy(f.hands[0].room, f.handRoom, sizeof(f.handRoom));
            std::memcpy(f.hands[0].velocity, f.handVelocity, sizeof(f.handVelocity));
            std::memcpy(f.hands[0].spin, f.handSpin, sizeof(f.handSpin));
            loose::HandAt& right = f.hands[1];
            right.tracked = right.sensed = true;
            Identity(right.room);
            right.room[3] = 0.25f, right.room[7] = 4.92f, right.room[11] = 0.3f;
            right.velocity[0] = 1.0f, right.velocity[1] = 2.0f, right.velocity[2] = -3.0f;
            right.spin[2] = 6.0f;
            const int id = loose::ReleaseBy(gun, kGunKind, bone, 30, f, 1);
            float com[3];
            Middle(gun, bone, com);
            const float arm[3] = {com[0] - right.room[3], com[1] - right.room[7], com[2] - right.room[11]};
            const float* w = right.spin;
            const float want[3] = {right.velocity[0] + w[1] * arm[2] - w[2] * arm[1] + f.walk[0], right.velocity[1] + w[2] * arm[0] - w[0] * arm[2] + f.walk[1],
                                   right.velocity[2] + w[0] * arm[1] - w[1] * arm[0] + f.walk[2]};
            float step[1][3], got[3], off[3];
            const bool have = FirstSteps(gun, &id, 1, step);
            for (int i = 0; i < 3; ++i)
                got[i] = (step[0][i] - gravity[i]) / kDt, off[i] = got[i] - want[i];
            CheckF(haveGravity && have && Length(off) < 0.01f * Length(want),
                   "g5 let go of by the right hand (its sensors' 1 2 -3 m/s and 0 0 6 rad/s, walking 0.4 0 0.6): its middle leaves at %.3f %.3f %.3f m/s, wanted %.3f "
                   "%.3f %.3f (off by %.2f%%)",
                   got[0], got[1], got[2], want[0], want[1], want[2], 100.0f * Length(off) / Length(want));
        }
        // ... by its last steps (its sensors not given): the right moving (0.8 0 -1.2) m/s, the left held still; one let go of by each in the same frame:
        // the right's at the right's speed, the left's still. Each hand's own.
        {
            loose::Clear();
            g_ground = Ground{};
            loose::Frame f = Base();
            for (int i = 0; i < 8; ++i) {
                const float t = static_cast<float>(i - 7) * kDt;
                for (loose::HandAt& h : f.hands) {
                    h.tracked = true;
                    Identity(h.room);
                    h.room[7] = 4.9f;
                }
                f.hands[0].room[3] = -0.3f;
                f.hands[1].room[3] = 0.3f + 0.8f * t, f.hands[1].room[11] = -1.2f * t;
                loose::Step(f);
            }
            float left[12], right[12];
            Identity(left), Identity(right);
            left[3] = -0.3f, left[7] = 5.0f;
            right[3] = 0.3f, right[7] = 5.0f;
            const int ids[2] = {loose::ReleaseBy(gun, kGunKind, left, 0, f, 0), loose::ReleaseBy(gun, kGunKind, right, 0, f, 1)};
            float steps[2][3], got[2][3];
            const bool have = FirstSteps(gun, ids, 2, steps);
            for (int j = 0; j < 2; ++j)
                for (int i = 0; i < 3; ++i)
                    got[j][i] = (steps[j][i] - gravity[i]) / kDt;
            const float want[3] = {0.8f, 0.0f, -1.2f}, off[3] = {got[1][0] - want[0], got[1][1] - want[1], got[1][2] - want[2]};
            CheckF(haveGravity && have && Length(off) < 0.01f * Length(want) && Length(got[0]) < 0.01f,
                   "g5 ... by the hands' last steps: the right's leaves at %.3f %.3f %.3f m/s (its hand 0.8 0 -1.2), the left's at %.3f m/s (its hand still)", got[1][0],
                   got[1][1], got[1][2], Length(got[0]));
        }

        // g6: let go of by the left (ReleaseBy 0) given the same hand as Release's (Frame::hand...): the same body bit for bit over its start and 10 steps;
        // a rifle and a magazine, thrown by the sensors' numbers and by the steps.
        {
            int alike = 0, all = 0;
            for (const loose::Spec* spec : {&gun, loose::SpecFor("m4a1")})
                for (int sensed : {1, 0}) {
                    float bones[2][11][12] = {};
                    int counts[2] = {};
                    for (int by : {0, 1}) {
                        loose::Clear();
                        g_ground = Ground{};
                        loose::Frame f = Base();
                        for (int i = 0; i < 8; ++i) {
                            const float t = static_cast<float>(i - 7) * kDt;
                            f.hand = true;
                            Turn(f.handRoom, x, -9.0f * t);
                            f.handRoom[3] = 0.2f, f.handRoom[7] = 1.3f + 2.0f * t, f.handRoom[11] = 3.0f * t;
                            f.hands[0].tracked = true;
                            std::memcpy(f.hands[0].room, f.handRoom, sizeof(f.handRoom));
                            loose::Step(f);
                        }
                        if (sensed) {
                            const float v[3] = {0.1f, 2.0f, 3.0f}, w[3] = {-9.0f, 0.5f, 0.0f};
                            f.handSensed = f.hands[0].sensed = true;
                            std::memcpy(f.handVelocity, v, sizeof(v)), std::memcpy(f.hands[0].velocity, v, sizeof(v));
                            std::memcpy(f.handSpin, w, sizeof(w)), std::memcpy(f.hands[0].spin, w, sizeof(w));
                        }
                        float inHand[12], bone[12];
                        Identity(inHand);
                        inHand[7] = -0.05f, inHand[11] = 0.1f;
                        Compose(f.handRoom, inHand, bone);
                        const int id = by ? loose::ReleaseBy(*spec, kGunKind, bone, 12, f, 0) : loose::Release(*spec, kGunKind, bone, 12, f);
                        for (int i = 0; i <= 10; ++i) {
                            const loose::Result& r = loose::Step(i == 0 ? f : Base());
                            for (int k = 0; k < r.poses; ++k)
                                if (r.pose[k].id == id && counts[by] < 11)
                                    std::memcpy(bones[by][counts[by]++], r.pose[k].bone, sizeof(r.pose[k].bone));
                        }
                    }
                    ++all;
                    alike += counts[0] == 11 && counts[1] == 11 && std::memcmp(bones[0], bones[1], sizeof(bones[0])) == 0;
                }
            CheckF(alike == all, "g6 let go of by the left through ReleaseBy, the same hand as Release's: the same body bit for bit over its start and 10 steps (%d of %d: "
                                 "the rifle and a magazine, by the sensors and by the steps)",
                   alike, all);
        }

        // g7: no ground under it at all: it falls and is gone after 5 s all the same (kept, but never falling forever).
        {
            loose::Clear();
            g_ground = Ground{};
            g_ground.nothing = true;
            float bone[12];
            GunBone(0.0f, 1.0f, bone);
            const int id = loose::ReleaseBy(gun, kGunKind, bone, 0, Base(), 1);
            const Seen s = Run(gun, id, 7.0f, Still);
            CheckF(id && s.gone && s.goneAt > 4.9f && s.goneAt < 5.5f && !loose::Has(id), "g7 the rifle with nothing under it at all: it falls and is gone after %.2f s (5 s)",
                   s.goneAt);
        }
        // ... broken numbers: nothing starts; a hand's broken sensors: not used (its steps instead), it drops; a caller's own hull with a broken point:
        // that point left out.
        {
            loose::Clear();
            g_ground = Ground{};
            float bone[12], broken[12];
            GunBone(kPi / 2, 0.5f, bone);
            std::memcpy(broken, bone, sizeof(bone));
            broken[7] = kNan;
            loose::Frame turned = Base(), walking = Base();
            turned.roomToArms[3] = kInf;
            walking.walk[0] = kNan;
            const int a = loose::ReleaseBy(gun, kGunKind, broken, 0, Base(), 1);
            const int b = loose::ReleaseBy(gun, kGunKind, bone, 0, turned, 1);
            const int c = loose::ReleaseBy(gun, kGunKind, bone, 0, walking, 0);
            const int d = loose::ReleaseBy(gun, kGunKind, nullptr, 0, Base(), 1);
            const int e = loose::ReleaseBy(gun, kGunKind, bone, 0, Base(), 2) + loose::ReleaseBy(gun, kGunKind, bone, 0, Base(), -1);
            CheckF(!a && !b && !c && !d && !e && loose::Count() == 0,
                   "g7 broken numbers (a NaN in its pose, an infinity in the room, a NaN in the walking; no pose; no such hand): nothing starts (%d %d %d %d %d)", a, b, c, d, e);
            loose::Frame bad = Base();
            bad.hands[1].tracked = bad.hands[1].sensed = true;
            Identity(bad.hands[1].room);
            bad.hands[1].room[7] = 0.6f;
            bad.hands[1].velocity[0] = kNan, bad.hands[1].spin[2] = kInf;
            const int k = loose::ReleaseBy(gun, kGunKind, bone, 0, bad, 1);
            bool finite = true;
            const Seen s = Run(gun, k, 3.0f, Still, [&](const loose::Pose& p, float, const loose::Frame&) { finite &= AllFinite(p.bone, 12); });
            float from[3], to[3];
            Middle(gun, bone, from);
            Middle(gun, s.bone, to);
            const float away = std::sqrt((to[0] - from[0]) * (to[0] - from[0]) + (to[2] - from[2]) * (to[2] - from[2]));
            CheckF(k && finite && s.restAt > 0.0f && away < 0.2f && s.lowest > -0.002f,
                   "g7 its hand's sensors broken (a NaN, an infinity): not used, it drops where it was (%.2f m off), at rest at %.2f s, every number fine", away, s.restAt);
            float hull[22][3];
            std::memcpy(hull, gunbody::kM416Hull, sizeof(hull));
            hull[6][0] = kNan;  // the ACOG's top
            loose::Spec own = gun;
            own.hull = hull;
            loose::Clear();
            const int m = loose::ReleaseBy(own, kGunKind, bone, 0, Base(), 1);
            finite = true;
            const Seen o = Run(own, m, 3.0f, Still, [&](const loose::Pose& p, float, const loose::Frame&) { finite &= AllFinite(p.bone, 12); });
            CheckF(m && finite && o.restAt > 0.0f && o.lowest > -0.002f,
                   "g7 a caller's own hull with a NaN in a point: it's left out, the rifle lands and rests on the rest (at %.2f s, deepest %.2f mm)", o.restAt, -o.lowest * 1000.0f);
        }

        // g8: fewer allowed (SetMost 5) with the rifle in the eighth place: the two magazines past five go, the rifle stays, and goes on lying.
        {
            loose::Clear();
            g_ground = Ground{};
            int mags[7] = {};
            for (int k = 0; k < 7; ++k) {
                float mag[12];
                Identity(mag);
                mag[3] = 0.3f * static_cast<float>(k), mag[7] = 0.4f;
                mags[k] = loose::Release(stanag, 1, mag, 0, Base());
            }
            float bone[12];
            GunBone(kPi / 2, 0.3f, bone);
            bone[11] += 1.0f;
            const int id = loose::ReleaseBy(gun, kGunKind, bone, 0, Base(), 1);
            loose::Step(Base());
            loose::SetMost(5, loose::kCasings);
            bool told = false, resting = false, firstFive = true;
            int gone = 0, frames = 0, drawn = 0;
            for (int i = 0; i < static_cast<int>(3.0f / kDt); ++i) {
                const loose::Result& r = loose::Step(Base());
                ++frames;
                for (int k = 0; k < r.gone; ++k) {
                    told |= r.goneId[k] == id;
                    gone += r.goneId[k] == mags[5] || r.goneId[k] == mags[6];
                }
                for (int k = 0; k < r.poses; ++k)
                    if (r.pose[k].id == id)
                        ++drawn, resting = r.pose[k].resting;
            }
            for (int k = 0; k < 5; ++k)
                firstFive &= loose::Has(mags[k]);
            const int count = loose::Count();
            loose::SetMost(loose::kMost, loose::kCasings);  // back as it was
            CheckF(id && !told && loose::Has(id) && gone == 2 && firstFive && drawn == frames && resting && count == 6,
                   "g8 fewer allowed (5) with the rifle eighth: %d magazines past five gone, the first five kept, the rifle kept (%s), drawn every frame, at rest; %d there",
                   gone, loose::Has(id) ? "yes" : "no", count);
        }

        // g9: let go of with no turn at all (its sensors' 0, and a still hand's steps): every number fine, it falls and rests.
        for (int sensed : {1, 0}) {
            loose::Clear();
            g_ground = Ground{};
            loose::Frame f = Base();
            loose::HandAt& h = f.hands[1];
            h.tracked = true, h.sensed = sensed != 0;
            Identity(h.room);
            h.room[7] = 1.0f;
            h.velocity[2] = sensed ? 1.5f : 0.0f;
            for (int i = 0; i < 6 && !sensed; ++i)
                loose::Step(f);
            float bone[12];
            GunBone(0.3f, 0.6f, bone);
            const int id = loose::ReleaseBy(gun, kGunKind, bone, 0, f, 1);
            bool finite = true;
            const Seen s = Run(gun, id, 4.0f, Still, [&](const loose::Pose& p, float, const loose::Frame&) { finite &= AllFinite(p.bone, 12); });
            CheckF(id && finite && s.restAt > 0.0f && s.lowest > -0.002f, "g9 let go of with no turn at all (%s): every number fine, at rest at %.2f s (deepest %.2f mm)",
                   sensed ? "its sensors' 0, moving 1.5 m/s" : "a still hand's steps", s.restAt, -s.lowest * 1000.0f);
        }
        // g10 Place (hands.cpp's flow (b) on the hotel roof: the rifle that lay when the scene's kit took its slot, laid down again where it lay once the
        // kit gives it back): the rifle let go of and at rest, taken away, laid at that pose: at rest again within 0.5 s, within 5 mm and 1 degree of where
        // it lay, never 2 mm into the ground, with none of a moving gun's speed in the frame (Flick would carry it: the spent AT4 swinging in the hand) nor
        // the walk's; laid with its pose carried to a frame whose origin moved 1 m on x (as hands.cpp's still start does): the same place in the world;
        // broken numbers or no bone: refused.
        {
            loose::Clear();
            g_ground = Ground{};
            float bone[12];
            GunBone(90.0f * kDegree, 0.3f, bone);
            const int first = loose::ReleaseBy(gun, kGunKind, bone, 30, Base(), 1);
            const Seen lay = Run(gun, first, 4.0f, Still);
            loose::Remove(first);
            loose::Frame f = Base();
            const float swing[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
            for (int i = 0; i < 8; ++i) {  // (a gun swung fast in the hand frame by frame, and the player walking: none of it is the laid gun's)
                GunAt(f, swing, 0.4f * static_cast<float>(i), 1.4f, 0.0f);
                loose::Step(f);
            }
            f.walk[0] = 2.0f;
            const int id = loose::Place(gun, kGunKind, lay.bone, 30, f);
            float firstMove = 0.0f;
            bool firstSeen = false;
            const Seen again = Run(gun, id, 1.0f, Still, [&](const loose::Pose& p, float, const loose::Frame&) {
                if (!firstSeen) {
                    firstSeen = true;
                    for (int i = 0; i < 3; ++i)
                        firstMove = std::fmax(firstMove, std::fabs(p.bone[i * 4 + 3] - lay.bone[i * 4 + 3]));
                }
            });
            float moved = 0.0f, turned = 0.0f;
            for (int i = 0; i < 3; ++i) {
                moved = std::fmax(moved, std::fabs(again.bone[i * 4 + 3] - lay.bone[i * 4 + 3]));
                for (int j = 0; j < 3; ++j)
                    turned = std::fmax(turned, std::fabs(again.bone[i * 4 + j] - lay.bone[i * 4 + j]));
            }
            // Carried by the origin: the arms' space 1 m further on x, its pose in it 1 m less (hands.cpp's still start), the world the same.
            loose::Clear();
            loose::Frame moved1 = Base();
            moved1.origin[0] = 1;
            float shifted[12];
            std::memcpy(shifted, lay.bone, sizeof(shifted));
            shifted[3] -= 1.0f;
            const int carried = loose::Place(gun, kGunKind, shifted, 30, moved1);
            float worldX = 1e9f;
            {
                const loose::Result& r = loose::Step(moved1);
                for (int k = 0; k < r.poses; ++k)
                    if (r.pose[k].id == carried)
                        worldX = r.pose[k].bone[3] + 1.0f;
            }
            float broken[12];
            std::memcpy(broken, lay.bone, sizeof(broken));
            broken[7] = kNan;
            loose::Frame badRoom = Base();
            badRoom.roomToArms[0] = kInf;
            const int refused = loose::Place(gun, kGunKind, broken, 30, Base()) + loose::Place(gun, kGunKind, nullptr, 30, Base()) +
                                loose::Place(gun, kGunKind, lay.bone, 30, badRoom);
            CheckF(first && lay.restAt > 0.0f && id && again.restAt > 0.0f && again.restAt <= 0.5f && moved < 0.005f && turned < 0.0175f &&
                       firstMove < 0.005f && again.lowest > -0.002f && carried && std::fabs(worldX - lay.bone[3]) < 0.002f && refused == 0,
                   "g10 laid down where it lay (Place): at rest at %.2f s (within 0.5), %.1f mm and %.4f of its turn from where it lay (5 mm, 1 degree), its first "
                   "frame %.1f mm off (none of the swung gun's speed or the walk's), deepest %.2f mm; carried by a 1 m origin, %.1f mm off in the world; broken "
                   "numbers refused (%d started)",
                   again.restAt, moved * 1000.0f, turned, firstMove * 1000.0f, -again.lowest * 1000.0f, std::fabs(worldX - lay.bone[3]) * 1000.0f, refused);
        }
    }

    // ---- The floor the soldier stands on where the game's rays see none (stood.h): Operation Swordbreaker's overpass, the owner 2026-11-26: "when I drop
    // it, it falls through the floor ... It just falls right through the pavement" ----
    {
        const loose::Spec& box = *loose::SpecFor("m249");
        const loose::Spec& stanag = *loose::SpecFor("m4a1");
        // s1 his report, reproduced with the old rule (no soldier's floor): the M249's box let go of 0.75 m over the deck, 0.5 m ahead of him crouched: it
        // lies a moment on the feet's guess (77.85, not firm), then its far ray's embankment (firm) takes it 6.85 m through the deck.
        {
            OverStart(false);
            const int id = OverDrop(box, 2, 0.75f, 0.5f);
            float at09 = 1e9f;
            const Seen s = Run(box, id, 6.0f, OverFrame, [&](const loose::Pose& p, float t, const loose::Frame&) {
                if (std::fabs(t - 0.9f) < 0.5f * kDt)
                    at09 = OverLowest(box, p.bone);
            });
            const float end = OverLowest(box, s.bone);
            CheckF(id && std::fabs(at09) < 0.01f && end < -6.84f && end > -6.86f,
                   "s1 the owner's report reproduced with the old rule: the M249's box lies on the feet's guess at 0.9 s (%.1f mm over the deck), then its far "
                   "ray's embankment takes it %.2f m through the deck (6.85; #17, #18, #19 and the M249 itself on 2026-11-26)",
                   at09 * 1000.0f, -end);
        }
        // s2 with the soldier's floor: still 0.6 s, the ray down from his eye finds nothing within 3 m and the eight round him nothing at his feet, so the
        // floor he stands on is kept; the same box lands on the deck, at rest, never 2 mm into it, and lies there 25 s with the embankment firm under it
        // (asked again every 0.25 s at rest), the last 20 of them with nothing new kept.
        {
            OverStart(true);
            const int verdict = OverSurvey();
            const int kept = g_over.watch.Prints();
            const int id = OverDrop(box, 2, 0.75f, 0.5f);
            const Seen s = Run(box, id, 25.0f, OverFrame);
            const float end = OverLowest(box, s.bone);
            CheckF(verdict == 1 && kept == 1 && g_over.watch.verdict().atFeet == 0 && id && s.restAt > 0.0f && s.restAt < 2.0f && s.lowest > -0.002f &&
                       end > -0.002f && end < 0.003f && loose::Has(id),
                   "s2 the floor he stands on kept (the eye's ray found nothing, %d of 8 round him his floor): the M249's box lands on the deck, at rest at %.2f "
                   "s, %.1f mm over it after 25 s (never %.1f mm into it), the embankment 6.85 m under it",
                   g_over.watch.verdict().atFeet, s.restAt, end * 1000.0f, std::fmax(0.0f, -s.lowest) * 1000.0f);
        }
        // s3 the first box on that deck: its far ray back in 4 frames (the embankment firm at once), the floor kept 6 frames after it's let go of (ground's
        // feet ray asked at its first ask, the survey a frame or two on): it lands on the deck. Kept only 0.6 s on, when it has fallen past the deck, it
        // falls on to the embankment (a floor kept is never one under a thing).
        {
            float end[2] = {};
            for (int late = 0; late < 2; ++late) {
                OverStart(true);
                g_over.raysAfter = 4.0f * kDt;
                const int id = OverDrop(box, 2, 0.75f, 0.5f);
                Run(box, id, (late ? 43 : 6) * kDt, OverFrame);
                OverSurvey();
                const Seen s = Run(box, id, 5.0f, OverFrame);
                end[late] = OverLowest(box, s.bone);
            }
            CheckF(end[0] > -0.002f && end[0] < 0.003f && end[1] < -6.84f,
                   "s3 the floor kept 6 frames after the box is let go of (its far ray back in 4): it lands on the deck (%.1f mm over it); kept 0.6 s on, past "
                   "the deck by then, it falls on to the embankment (%.2f m under the deck)",
                   end[0] * 1000.0f, -end[1]);
        }
        // s4 every magazine and box (the M240's and the PKP's too), the 12-gauge shell, the M320's case, a casing, the rifle and the M249 itself, each let go
        // of there: with the old rule every one goes through to the embankment; with the soldier's floor every one lies on the deck, never 2 mm into it.
        {
            std::vector<const loose::Spec*> specs;
            for (const char* name : {"m416", "m16a4", "m4a1", "kh2002", "an94", "aek971", "ak74", "rpk", "g3a3", "m240", "m9", "aks74u", "ump45", "mk11",
                                     "m249", "saiga12k", "pkp", "12g", "m320", "556"})
                specs.push_back(loose::SpecFor(name));
            specs.push_back(&gunbody::kM416);
            specs.push_back(&gunbodies::kM249);
            int n = 0, through = 0, on = 0;
            float worstOver = 0.0f, worstInto = 0.0f;
            char missed[160] = "";
            for (const loose::Spec* sp : specs) {
                if (!sp)
                    continue;
                ++n;
                const int kind = sp->keep ? 5 : 2;
                for (int fix = 0; fix < 2; ++fix) {
                    OverStart(fix == 1);
                    if (fix)
                        OverSurvey();
                    const int id = OverDrop(*sp, kind, 0.75f, 0.5f);
                    const Seen s = Run(*sp, id, 6.0f, OverFrame);
                    const float end = OverLowest(*sp, s.bone);
                    if (!fix) {
                        through += id && end < -6.8f;
                        continue;
                    }
                    // (A round one is on the deck lying or still rocking: the M320's case dropped level from a still hand rocks past 6 s on any
                    // ground, the plain one the rays see too.)
                    const bool ok = id && end > -0.002f && end < 0.004f && s.lowest > -0.002f && (s.restAt > 0.0f || sp->round >= 0);
                    on += ok;
                    worstOver = std::fmax(worstOver, end), worstInto = std::fmax(worstInto, -s.lowest);
                    if (!ok && std::strlen(missed) + std::strlen(sp->name) + 3 < sizeof(missed))
                        std::strcat(std::strcat(missed, " "), sp->name);
                }
            }
            CheckF(n == 22 && through == n && on == n,
                   "s4 all %d let go of on the deck (every magazine and box, the M240's and the PKP's, the shell, the M320's case, a casing, the rifle, the "
                   "M249): with the old rule %d went through it, with the soldier's floor %d lie on it (at worst %.1f mm over it, %.1f mm into it)%s%s",
                   n, through, on, worstOver * 1000.0f, worstInto * 1000.0f, missed[0] ? "; not:" : "", missed);
        }
        // s5 the box lying on the deck, the soldier walks 10 m off along it (his eye there): what was kept where he stood stays kept; it lies on 20 s on.
        {
            OverStart(true);
            OverSurvey();
            const int id = OverDrop(box, 2, 0.75f, 0.5f);
            Seen s = Run(box, id, 3.0f, OverFrame);
            const float before = s.bone[7];
            g_over.eye[0] += 10.0f;
            s = Run(box, id, 20.0f, OverFrame);
            CheckF(loose::Has(id) && std::fabs(s.bone[7] - before) < 1e-4f && OverLowest(box, s.bone) > -0.002f,
                   "s5 the soldier walked 10 m off: the box still lies on the deck 20 s on (%.3f mm moved)", std::fabs(s.bone[7] - before) * 1000.0f);
        }
        // s6 as the log had it: the M249's, the M240's and the PKP's boxes let go of 1.5 s apart round him, then the M249 itself: with the old rule all four
        // go through (#17, #18, #19, #20); with the soldier's floor all four lie on the deck.
        {
            const loose::Spec* four[4] = {&box, loose::SpecFor("m240"), loose::SpecFor("pkp"), &gunbodies::kM249};
            const float place[4][2] = {{0.5f, 0.0f}, {0.45f, 0.35f}, {0.55f, -0.35f}, {0.3f, 0.6f}};  // ahead, to his left
            int lying[2] = {};
            for (int fix = 0; fix < 2; ++fix) {
                OverStart(fix == 1);
                if (fix)
                    OverSurvey();
                int ids[4] = {};
                for (int k = 0; k < 4; ++k) {
                    ids[k] = OverDrop(*four[k], four[k]->keep ? 5 : 2, 0.75f, place[k][0], place[k][1]);
                    for (int i = 0; i < 108; ++i) {  // (1.5 s)
                        loose::Frame f = Base();
                        OverFrame(f, 0.0f);
                        loose::Step(f);
                    }
                }
                for (int i = 0; i < 360; ++i) {  // (5 s more)
                    loose::Frame f = Base();
                    OverFrame(f, 0.0f);
                    const loose::Result& r = loose::Step(f);
                    if (i == 359)
                        for (int p = 0; p < r.poses; ++p)
                            for (int k = 0; k < 4; ++k)
                                if (r.pose[p].id == ids[k] && r.pose[p].resting && OverLowest(*four[k], r.pose[p].bone) > -0.002f &&
                                    OverLowest(*four[k], r.pose[p].bone) < 0.004f)
                                    ++lying[fix];
                }
            }
            CheckF(four[1] && four[2] && lying[0] == 0 && lying[1] == 4,
                   "s6 the three boxes and the M249 let go of round him 1.5 s apart: with the old rule %d of 4 on the deck, with the soldier's floor %d of 4",
                   lying[0], lying[1]);
        }
        // s7 a crate the rays see on the deck (its top 0.5 m over it, 0.6 m square, 0.5-1.1 m ahead of him): the floor round him still kept (the round
        // rays over the crate find its top, not his floor); a box let go of over the crate lies on the crate (a floor of theirs over his is theirs), one
        // let go of beside it on the deck (0.78 m from his eye, within the print's 1 m).
        {
            OverStart(true);
            g_over.crate = true;
            g_over.crateY = g_over.deck + 0.5f;
            g_over.crateMin[0] = g_over.eye[0] - 0.3f, g_over.crateMax[0] = g_over.eye[0] + 0.3f;
            g_over.crateMin[1] = g_over.eye[2] + 0.5f, g_over.crateMax[1] = g_over.eye[2] + 1.1f;
            const int verdict = OverSurvey();
            const int over = OverDrop(box, 2, 0.4f, 0.8f, 0.0f, 0, g_over.crateY);
            const int beside = OverDrop(box, 2, 0.75f, 0.5f, 0.6f);
            float onCrate = 1e9f, onDeck = 1e9f;
            Run(box, over, 6.0f, OverFrame, [&](const loose::Pose& p, float, const loose::Frame&) { onCrate = OverLowest(box, p.bone); });
            for (int k = 0; k < 2; ++k) {
                const loose::Result& r = loose::Step([] {
                    loose::Frame f = Base();
                    OverFrame(f, 0.0f);
                    return f;
                }());
                for (int p = 0; p < r.poses; ++p)
                    if (r.pose[p].id == beside)
                        onDeck = OverLowest(box, r.pose[p].bone);
            }
            CheckF(verdict == 1 && std::fabs(onCrate - 0.5f) < 0.003f && onDeck > -0.002f && onDeck < 0.003f,
                   "s7 a crate the rays see on the deck: the floor round him kept all the same; a box let go of over it lies on the crate (%.1f mm over the "
                   "deck: 500), one beside it on the deck (%.1f mm over it)",
                   onCrate * 1000.0f, onDeck * 1000.0f);
        }
        // s8 where the rays see his floor nothing changes: on the hotel's roof (the street 12.6 m under its edge), the soldier standing 0.3 m from the
        // edge: the eye's ray finds the roof at his feet, no survey, nothing kept; a magazine let go of 0.5 m past the edge falls to the street, the same
        // to the last bit with the soldier's floor as without it (#6 on 2026-11-26 fell so, 0.5 m from #5 on the roof).
        // s9 his eye 0.15 m past the edge (on its lip): the eye's ray finds nothing within 3 m, but 3 of the 8 round him find the roof at his feet: an
        // edge, nothing kept; the magazine past the edge falls to the street as before.
        for (float eyeX : {-445.6f, -445.15f}) {
            Seen s[2];
            int verdict[2] = {}, prints[2] = {};
            for (int fix = 0; fix < 2; ++fix) {
                OverStart(fix == 1);
                g_over.roof = true;
                g_over.origin[0] = -446, g_over.origin[1] = 94, g_over.origin[2] = -2011;
                g_over.eye[0] = eyeX, g_over.eye[1] = g_over.roofY + 1.6f, g_over.eye[2] = -2011.0f;
                g_over.stance = 0;
                g_over.deck = g_over.roofY;
                g_ground.height = g_over.roofY - OverO(1);
                for (int i = 0; i < 43; ++i) {
                    g_over.now += 14;
                    g_over.watch.Eye(g_over.now, g_over.eye, g_over.stance, true);
                }
                verdict[fix] = OverSurvey();
                const int id = OverDrop(stanag, 2, 0.75f, 0.0f, -445.3f + 0.5f - eyeX);
                s[fix] = Run(stanag, id, 6.0f, OverFrame);
                prints[fix] = g_over.watch.Prints();
            }
            const float end = OverLowest(stanag, s[1].bone);
            const bool same = std::memcmp(s[0].bone, s[1].bone, sizeof(s[0].bone)) == 0;
            if (eyeX < -445.3f)
                CheckF(verdict[1] == -1 && prints[1] == 0 && same && end < -12.59f && end > -12.61f,
                       "s8 on the hotel's roof the rays see (0.3 m from its edge): no survey, nothing kept; a magazine let go of 0.5 m past the edge falls to "
                       "the street %.2f m down (12.6), the same to the bit with the soldier's floor as without (%s)",
                       -end, same ? "the same" : "not the same");
            else
                CheckF(verdict[1] == 0 && g_over.watch.verdict().atFeet == 3 && prints[1] == 0 && same && end < -12.59f,
                       "s9 his eye 0.15 m past the roof's edge: the eye's ray finds nothing, %d of 8 round him the roof at his feet: an edge, nothing kept; the "
                       "magazine past it falls to the street (%.2f m down), as without (%s)",
                       g_over.watch.verdict().atFeet, -end, same ? "the same" : "not the same");
        }
        // s10 the eye's ray (stood::FromEye) and a round ray's (AtFeet): nothing, or a floor further down than 0.5 m under his feet, is no floor seen; one
        // at his feet, 0.3 or 0.49 m under them, is; something 0.4 m over them (a table), or facing a wall, tells nothing. A round ray's floor: at his
        // feet's height facing up (0.1 m off still), not 0.2 m off, nor a steep face.
        {
            using stood::Eye;
            const float feet = 77.85f;
            const bool eye = stood::FromEye(false, 1.0f, 0.0f, feet) == Eye::kUnseen && stood::FromEye(true, 1.0f, feet, feet) == Eye::kSeen &&
                             stood::FromEye(true, 1.0f, feet - 0.3f, feet) == Eye::kSeen && stood::FromEye(true, 1.0f, feet - 0.49f, feet) == Eye::kSeen &&
                             stood::FromEye(true, 1.0f, feet - 0.6f, feet) == Eye::kUnseen && stood::FromEye(true, 1.0f, 71.0f, feet) == Eye::kUnseen &&
                             stood::FromEye(true, 1.0f, feet + 0.4f, feet) == Eye::kNotSure && stood::FromEye(true, 0.2f, feet, feet) == Eye::kNotSure;
            const bool round = stood::AtFeet(true, 1.0f, feet, feet) && stood::AtFeet(true, 0.9f, feet + 0.1f, feet) && !stood::AtFeet(true, 1.0f, feet - 0.2f, feet) &&
                               !stood::AtFeet(true, 0.5f, feet, feet) && !stood::AtFeet(false, 1.0f, feet, feet);
            CheckF(eye && round, "s10 the eye's ray: nothing or a floor 0.6 m under his feet unseen, at them or 0.3-0.49 m under seen, a table or a wall's face "
                                 "nothing told; a round ray's floor at his feet within 0.12 m, facing up (%s, %s)",
                   eye ? "the eye's right" : "the eye's WRONG", round ? "the round ones right" : "the round ones WRONG");
        }
        // s11 steady (stood::Steady): still 0.6 s crouched, his feet at the eye less 1.0 m; watched 0.3 s, not yet; a jump (0.45 m up and down over 0.6 s),
        // never within it; a crouch (0.6 m down in 0.25 s), not till 0.5 s after it's down; a scripted camera within the half second, not; a ride at
        // 20 m/s, not; a walk at 1.5 m/s, steady; no eye for 0.2 s, not.
        {
            auto run = [](stood::Steady& st, unsigned long long& now, float seconds, auto eyeAt, int stance = 1, bool onFoot = true) {
                const int frames = static_cast<int>(seconds / kDt + 0.5f);
                for (int i = 0; i < frames; ++i) {
                    now += 14;
                    float e[3];
                    eyeAt(static_cast<float>(i) * kDt, e);
                    st.Add(now, e, stance, onFoot);
                }
            };
            auto still = [](float, float* e) { e[0] = 1.0f, e[1] = 78.85f, e[2] = 2.0f; };
            float feet = 0.0f;
            stood::Steady a;
            unsigned long long now = 100000;
            run(a, now, 0.3f, still);
            const bool notYet = !a.Feet(now, feet);
            run(a, now, 0.3f, still);
            const bool steady = a.Feet(now, feet) && std::fabs(feet - 77.85f) < 0.001f;
            bool jumpSteady = false;
            run(a, now, 0.6f, [&](float t, float* e) {
                still(t, e);
                e[1] += 0.45f * std::sin(kPi * t / 0.6f);
            });
            {
                float f2 = 0.0f;
                jumpSteady = a.Feet(now, f2);
            }
            stood::Steady c;
            now = 200000;
            run(c, now, 0.6f, still, 0);
            run(c, now, 0.25f, [&](float t, float* e) {
                still(t, e);
                e[1] = 78.85f + 0.6f - 0.6f * t / 0.25f;
            }, 1);
            run(c, now, 0.3f, still, 1);
            const bool crouchSoon = c.Feet(now, feet);
            run(c, now, 0.25f, still, 1);
            const bool crouchAfter = c.Feet(now, feet) && std::fabs(feet - 77.85f) < 0.001f;
            stood::Steady d;
            now = 300000;
            run(d, now, 0.4f, still);
            run(d, now, 0.2f, still, 1, false);
            run(d, now, 0.3f, still);
            const bool cutscene = d.Feet(now, feet);
            stood::Steady r;
            now = 400000;
            run(r, now, 0.6f, [&](float t, float* e) {
                still(t, e);
                e[2] += 20.0f * t;
            });
            const bool ride = r.Feet(now, feet);
            stood::Steady w;
            now = 500000;
            run(w, now, 0.6f, [&](float t, float* e) {
                still(t, e);
                e[2] += 1.5f * t;
            });
            const bool walk = w.Feet(now, feet);
            const bool stale = !w.Feet(now + 200, feet);
            CheckF(notYet && steady && !jumpSteady && !crouchSoon && crouchAfter && !cutscene && !ride && walk && stale,
                   "s11 steady: still 0.6 s yes (his feet %.3f), watched 0.3 s %s, a jump %s, a crouch 0.3 s after %s and 0.55 s after %s, a scripted camera "
                   "within it %s, a ride at 20 m/s %s, a walk at 1.5 m/s %s, the eye 0.2 s old %s",
                   77.85f, notYet ? "not yet" : "YES", jumpSteady ? "YES" : "no", crouchSoon ? "YES" : "no", crouchAfter ? "yes" : "NO",
                   cutscene ? "YES" : "no", ride ? "YES" : "no", walk ? "yes" : "NO", stale ? "no" : "YES");
        }
        // s12 the places kept (stood::Prints): the same place twice is one; 0.4 m on and 3 cm higher, a second at the first's height (one floor, one
        // height); 0.3 m up, its own; under a point, the highest floor within 1.5 m across no more than 3 cm over it (0.3 m up: another floor, over this
        // one), and of that floor's (within 25 cm under it) the nearest (a 15 cm step: its own side's height either side); nothing 1.6 m off, nothing
        // under a point 5 cm under all of them; 256 kept, the 257th forgets the first; cleared, none.
        {
            stood::Prints p;
            p.Add(0.0f, 77.85f, 0.0f);
            p.Add(0.1f, 77.86f, 0.1f);
            const int same = p.Count();
            p.Add(0.4f, 77.88f, 0.0f);
            float y = 0.0f;
            const float at1[3] = {0.45f, 79.0f, 0.0f};
            const bool level = p.Count() == 2 && p.Under(at1, y) && y == 77.85f;
            p.Add(-1.0f, 78.15f, 0.0f);
            const float at2[3] = {-0.3f, 79.0f, 0.0f}, at3[3] = {2.0f, 79.0f, 0.0f}, at4[3] = {0.0f, 77.80f, 0.0f}, at5[3] = {0.0f, 77.83f, 0.0f};
            float y2 = 0.0f, y5 = 0.0f;
            bool step = p.Count() == 3 && p.Under(at2, y2) && y2 == 78.15f;
            const bool far = !p.Under(at3, y);
            const bool under = !p.Under(at4, y) && p.Under(at5, y5) && y5 == 77.85f;
            {
                stood::Prints s;
                s.Add(0.0f, 77.85f, 0.0f);
                s.Add(1.0f, 78.0f, 0.0f);
                const float near0[3] = {0.2f, 79.0f, 0.0f}, near1[3] = {0.8f, 79.0f, 0.0f};
                float a = 0.0f, b = 0.0f;
                step = step && s.Count() == 2 && s.Under(near0, a) && a == 77.85f && s.Under(near1, b) && b == 78.0f;
            }
            stood::Prints q;
            for (int i = 0; i < stood::kPrints + 1; ++i)
                q.Add(static_cast<float>(i) * 5.0f, 77.85f, 0.0f);
            const float first[3] = {0.0f, 79.0f, 0.0f}, last[3] = {5.0f * stood::kPrints, 79.0f, 0.0f};
            const bool ring = q.Count() == stood::kPrints && !q.Under(first, y) && q.Under(last, y);
            q.Clear();
            const bool cleared = q.Count() == 0 && !q.Under(last, y);
            CheckF(same == 1 && level && step && far && under && ring && cleared,
                   "s12 the places kept: one for the same place (%d), a level floor one height (%s), a floor over it and a step's sides their own (%s), "
                   "nothing 1.6 m off (%s), nothing under a point under them all (%s), %d kept and the oldest forgotten (%s), cleared (%s)",
                   same, level ? "yes" : "NO", step ? "yes" : "NO", far ? "yes" : "NO", under ? "yes" : "NO", stood::kPrints, ring ? "yes" : "NO",
                   cleared ? "yes" : "NO");
        }
        // s13 his floor in place of the rays' (stood::StandsIn): theirs none known or nothing as far as 50 m, his; the embankment 6.85 m under his, his; a
        // floor of theirs 3 cm under his, theirs; a crate 0.5 m over it, theirs; a guess from beside at his height, theirs; one 1 m under, his; ground::Firm:
        // a ray's and his firm, a guess beside, the feet, none and nothing not.
        {
            using stood::Rays;
            const float his = 77.85f;
            const bool in = stood::StandsIn(Rays::kNone, 0.0f, his) && stood::StandsIn(Rays::kNothing, 0.0f, his) && stood::StandsIn(Rays::kFloor, 71.0f, his) &&
                            !stood::StandsIn(Rays::kFloor, his - 0.03f, his) && !stood::StandsIn(Rays::kFloor, his + 0.5f, his) &&
                            !stood::StandsIn(Rays::kGuess, his, his) && stood::StandsIn(Rays::kGuess, his - 1.0f, his) &&
                            stood::StandsIn(Rays::kFloor, std::numeric_limits<float>::quiet_NaN(), his);
            const bool firm = ground::Firm(ground::kRay) && ground::Firm(ground::kStood) && !ground::Firm(ground::kNear) && !ground::Firm(ground::kFeet) &&
                              !ground::Firm(ground::kNone) && !ground::Firm(ground::kNothing);
            CheckF(in && firm, "s13 his floor in place of the rays' where they know none, see none or find one more than 5 cm under it, never over one of "
                               "theirs (%s); firm: a ray's and his alone (%s)",
                   in ? "right" : "WRONG", firm ? "right" : "WRONG");
        }
        // s14 the watch's rules: after a verdict of unseen the eye's ray missing again 0.2 m on within 3 s asks no survey (the same place, kept as it
        // was); 1.0 m on it asks one; an edge's verdict keeps nothing and asks none again there for 3 s, then one; not steady, a miss asks nothing; a
        // survey not all back in 1.5 s is given up and the next miss asks again; his floor seen under his eye ends the verdict (nothing more of his walk
        // kept); cleared, nothing.
        {
            stood::Watch wt;
            unsigned long long now = 900000;
            float eye[3] = {0.0f, 78.85f, 0.0f};
            for (int i = 0; i < 43; ++i)
                wt.Eye(now += 14, eye, 1, true);
            auto miss = [&](const float* at) { return wt.EyeBack(now, at, true, 77.85f, false, 1.0f, 0.0f); };
            auto roundAll = [&](int atFeet) {
                const unsigned serial = wt.survey().serial;
                for (int i = 0; i < stood::kRound; ++i)
                    wt.Round(now, serial, i, i < atFeet, 1.0f, 77.85f);
            };
            const bool first = miss(eye);
            roundAll(0);
            const bool unseen = wt.verdict().have && wt.verdict().unseen && wt.Prints() == 1;
            now += 500;
            const float near[3] = {0.2f, 78.85f, 0.0f}, farther[3] = {1.0f, 78.85f, 0.0f};
            const bool refreshed = !miss(near) && wt.Prints() == 1 && !wt.survey().on;
            const bool resurvey = miss(farther);
            roundAll(3);  // (an edge there)
            const bool edge = wt.verdict().have && !wt.verdict().unseen && wt.Prints() == 1;
            now += 1000;
            const bool edgeHolds = !miss(farther);
            now += 2100;
            const bool edgeAgain = miss(farther);
            wt.Frame(now + 1600, farther);  // (its rays never back)
            const bool gaveUp = !wt.survey().on && miss(farther);
            const bool unsteady = !wt.EyeBack(now, eye, false, 77.85f, false, 1.0f, 0.0f);
            roundAll(0);
            const bool unseenAgain = wt.verdict().unseen;
            wt.EyeBack(now, farther, true, 77.85f, true, 1.0f, 77.85f);  // (his floor seen under his eye)
            const bool ended = !wt.verdict().have;
            wt.Clear();
            float y = 0.0f;
            const bool cleared = wt.Prints() == 0 && !wt.Under(eye, y) && !wt.verdict().have && !wt.survey().on;
            CheckF(first && unseen && refreshed && resurvey && edge && edgeHolds && edgeAgain && gaveUp && unsteady && unseenAgain && ended && cleared,
                   "s14 the watch: a survey at the first miss (%s), unseen kept (%s), a miss 0.2 m on kept with no survey (%s), 1 m on a survey (%s), an edge "
                   "kept nothing (%s) nor asked again there for 3 s (%s, then %s), a survey not back given up (%s), not steady nothing (%s), his floor seen "
                   "ends it (%s), cleared (%s)",
                   first ? "yes" : "NO", unseen ? "yes" : "NO", refreshed ? "yes" : "NO", resurvey ? "yes" : "NO", edge ? "yes" : "NO",
                   edgeHolds ? "yes" : "NO", edgeAgain ? "asked" : "NOT ASKED", gaveUp ? "yes" : "NO", unsteady ? "yes" : "NO", ended && unseenAgain ? "yes" : "NO",
                   cleared ? "yes" : "NO");
        }
        // s15 his walk: after a verdict of unseen, walking at 1.5 m/s along the deck (steady), the places he walks to kept for its second, within 1 m of
        // it; a box let go of ahead of him at 0.8 m on lands on the deck; past 1 m nothing more kept, nor once the verdict is a second old.
        {
            OverStart(true);
            OverSurvey();
            const float from[3] = {g_over.eye[0], g_over.eye[1], g_over.eye[2]};
            int keptWalking = 0, id = 0;
            for (int i = 0; i < 72; ++i) {
                g_over.eye[2] = from[2] + 1.5f * static_cast<float>(i + 1) * kDt;
                loose::Frame f = Base();
                OverFrame(f, 0.0f);
                loose::Step(f);
                keptWalking = g_over.watch.Prints();
                if (i == 38)  // (0.8 m on)
                    id = OverDrop(box, 2, 0.75f, 0.5f);
            }
            const Seen s = Run(box, id, 5.0f, OverFrame);
            const float end = OverLowest(box, s.bone);
            float y = 0.0f;
            const float past[3] = {from[0], 79.0f, from[2] + 1.0f + stood::kReach + 0.2f};
            CheckF(keptWalking >= 3 && end > -0.002f && end < 0.003f && !g_over.watch.Under(past, y),
                   "s15 his walk along the deck kept as he goes (%d places in 1 s at 1.5 m/s, within 1 m of the survey): a box let go of ahead of him 0.8 m on "
                   "lands on the deck (%.1f mm over it); nothing kept past 1 m",
                   keptWalking, end * 1000.0f);
        }
        // A floor the rays see with an outside corner (x < edgeX and z < edgeZ, its top at topY; floorY round it), the soldier standing at the corner, his
        // eye e past both of its sides; the eye's ray and, if it asks, the survey round him: 1 unseen (kept), 0 an edge, -1 none.
        auto corner = [&](bool stoodOn, float edgeX, float edgeZ, float topY, float floorY, const int* origin, float e) {
            OverStart(stoodOn);
            g_over.roof = true;
            g_over.roofEdge = edgeX, g_over.roofEdgeZ = edgeZ, g_over.roofY = topY, g_over.streetY = floorY;
            for (int i = 0; i < 3; ++i)
                g_over.origin[i] = origin[i];
            g_over.eye[0] = edgeX + e, g_over.eye[1] = topY + 1.6f, g_over.eye[2] = edgeZ + e;
            g_over.stance = 0;
            g_over.deck = topY;
            g_ground.height = topY - OverO(1);
            for (int i = 0; i < 43; ++i) {
                g_over.now += 14;
                g_over.watch.Eye(g_over.now, g_over.eye, g_over.stance, true);
            }
            return OverSurvey();
        };
        // s16 a roof's outside corner the rays see (the hotel's roof where x < -445.3 and z < -2011.0, the street 12.6 m under it round the corner): the
        // soldier at the corner, his eye (7.7 cm ahead of his middle) 2, 4 or 5.4 cm past both of its sides. The eye's ray finds nothing within 3 m and of
        // the eight round him only the one back over the roof finds it at his feet: an edge, nothing kept; a magazine let go of 0.7 m out past the corner,
        // or 1.1 m out, falls to the street, the same to the bit as without the soldier's floor (the review of a2c8c2e, 2026-11-26: one of the eight let
        // pass, the corner was kept, and the magazine 0.7 m out lay on the air at the roof's height).
        {
            const int origin[3] = {-446, 94, -2011};
            const float X0 = -445.3f, Z0 = -2011.0f;
            int runs = 0, edges = 0, kept = 0, same = 0, street = 0;
            float worst = -1e9f;
            for (float e : {0.02f, 0.04f, 0.054f})
                for (int out = 0; out < 2; ++out) {
                    const float ox = out ? 0.95f : 0.5f, oz = out ? 0.6f : 0.5f;  // (0.7 m out past the corner, or 1.1 m)
                    Seen s[2];
                    for (int fix = 0; fix < 2; ++fix) {
                        const int verdict = corner(fix == 1, X0, Z0, 94.2f, 81.6f, origin, e);
                        if (fix)
                            edges += verdict == 0 && g_over.watch.verdict().atFeet == 1;
                        const int id = OverDrop(stanag, 2, 0.75f, Z0 + oz - g_over.eye[2], X0 + ox - g_over.eye[0]);
                        s[fix] = Run(stanag, id, 6.0f, OverFrame);
                        if (fix)
                            kept += g_over.watch.Prints();
                    }
                    const float end = OverLowest(stanag, s[1].bone);
                    ++runs;
                    same += std::memcmp(s[0].bone, s[1].bone, sizeof(s[0].bone)) == 0;
                    street += end < -12.59f && end > -12.61f;
                    worst = std::fmax(worst, end);
                }
            CheckF(runs == 6 && edges == runs && kept == 0 && same == runs && street == runs,
                   "s16 a roof's outside corner, his eye 2-5.4 cm past both its sides: an edge %d of %d times (one of the eight round him the roof at his "
                   "feet), %d places kept; a magazine let go of 0.7 or 1.1 m out past it falls to the street %d of %d times (at worst %.2f m down: 12.6), "
                   "the same to the bit as without the soldier's floor %d of %d",
                   edges, runs, kept, street, runs, -worst, same, runs);
        }
        // s17 the same at a 1 m crate's corner on a floor the rays see (the crate where x < 1 and z < 1), his eye 4 cm past both of its sides: an edge,
        // nothing kept; a magazine let go of beside the crate (0.45 m out past the corner on each side) lies on the floor, as without the soldier's floor,
        // never on the air at the crate's top (the review: it lay at 1.01).
        {
            const int origin[3] = {0, 0, 0};
            Seen s[2];
            int verdict = -2, atFeet = -1, prints = -1;
            for (int fix = 0; fix < 2; ++fix) {
                const int v = corner(fix == 1, 1.0f, 1.0f, 1.0f, 0.0f, origin, 0.04f);
                if (fix)
                    verdict = v, atFeet = g_over.watch.verdict().atFeet;
                const int id = OverDrop(stanag, 2, 0.75f, 1.45f - g_over.eye[2], 1.45f - g_over.eye[0]);
                s[fix] = Run(stanag, id, 6.0f, OverFrame);
                if (fix)
                    prints = g_over.watch.Prints();
            }
            const float end = OverLowest(stanag, s[1].bone);
            const bool same = std::memcmp(s[0].bone, s[1].bone, sizeof(s[0].bone)) == 0;
            CheckF(verdict == 0 && atFeet == 1 && prints == 0 && same && end < -0.99f && end > -1.01f,
                   "s17 on a 1 m crate's corner, his eye 4 cm past both its sides: an edge (%d of 8 round him the crate at his feet), %d places kept; a "
                   "magazine let go of beside the crate lies on the floor %.2f m under its top (1), as without the soldier's floor (%s)",
                   atFeet, prints, -end, same ? "the same" : "not the same");
        }
        // s18 the overpass's railing (the deck unseen, his eye 0.35 m from where it ends): a box let go of 0.9 or 1.3 m past the railing, 1.25 or 1.65 m
        // from his eye, falls to the embankment (the review of a2c8c2e: with the print's reach 1.5 m, the one 0.9 m past lay on the air); one let go of
        // 0.9 m from his eye on the deck's side lies on the deck. One 0.2 or 0.5 m past the railing lies on the air beside it still (said, not checked:
        // the known limit while the rays don't see the deck).
        {
            const float past[5] = {0.2f, 0.5f, 0.9f, 1.3f, -1.25f};  // x from the railing; the last 0.9 m from his eye on the deck's side
            float end[5] = {};
            for (int k = 0; k < 5; ++k) {
                OverStart(true);
                OverSurvey();
                const float rail = g_over.eye[0] + 0.35f;
                const int id = OverDrop(box, 2, 0.75f, 0.0f, rail + past[k] - g_over.eye[0]);
                const Seen s = Run(box, id, 6.0f, OverFrame);
                end[k] = OverLowest(box, s.bone);
            }
            CheckF(end[2] < -6.84f && end[3] < -6.84f && end[4] > -0.002f && end[4] < 0.003f,
                   "s18 the overpass's railing, his eye 0.35 m from it: a box let go of 0.9 m past it falls %.2f m (6.85), 1.3 m past %.2f m; one 0.9 m from "
                   "his eye on the deck lies on it (%.1f mm over it); 0.2 and 0.5 m past the railing they lie %.0f and %.0f mm over the deck's height (the "
                   "known limit)",
                   -end[2], -end[3], end[4] * 1000.0f, end[0] * 1000.0f, end[1] * 1000.0f);
        }
        // s19 a print kept while it's used: the M249 let go of on the deck lies there; the soldier then walks 104 m on another floor the rays don't see
        // (260 places kept, 0.4 m apart, the M249 asking its floor every 0.25 s meanwhile, as in the game): it still lies on the deck (the review of
        // a2c8c2e: kept oldest first, its print was forgotten at the 256th and it fell through the deck). The rule alone: all 256 kept, the one longest
        // unused makes way (the first, just asked, stays; the second goes), and one stood on again stays too.
        {
            OverStart(true);
            OverSurvey();
            const int id = OverDrop(gunbodies::kM249, 5, 0.55f, 0.5f);
            Seen s = Run(gunbodies::kM249, id, 4.0f, OverFrame);
            const float onDeck = OverLowest(gunbodies::kM249, s.bone);
            for (int k = 0; k < 260; ++k) {
                g_over.eye[0] = -400.0f + 0.4f * static_cast<float>(k % 20);
                g_over.eye[2] = -2000.0f + 0.4f * static_cast<float>(k / 20);
                s = Run(gunbodies::kM249, id, 43 * kDt, OverFrame);
                OverSurvey();
            }
            const int kept = g_over.watch.Prints();
            s = Run(gunbodies::kM249, id, 6.0f, OverFrame);
            const float after = OverLowest(gunbodies::kM249, s.bone);
            stood::Prints q;
            for (int i = 0; i < stood::kPrints; ++i)
                q.Add(5.0f * static_cast<float>(i), 77.85f, 0.0f);
            float under = 0.0f;
            const float p0[3] = {0.0f, 79.0f, 0.0f}, p1[3] = {5.0f, 79.0f, 0.0f}, p2[3] = {10.0f, 79.0f, 0.0f}, p3[3] = {15.0f, 79.0f, 0.0f};
            const bool asked = q.Under(p0, under);
            q.Add(5.0f * static_cast<float>(stood::kPrints), 77.85f, 0.0f);  // (the 257th)
            const bool used = asked && q.Under(p0, under) && !q.Under(p1, under);
            q.Add(10.0f, 77.85f, 0.0f);  // (the third's place stood on again)
            q.Add(5.0f * static_cast<float>(stood::kPrints + 1), 77.85f, 0.0f);  // (the 258th)
            const bool again = q.Under(p2, under) && !q.Under(p3, under) && q.Count() == stood::kPrints;
            CheckF(onDeck > -0.002f && onDeck < 0.004f && kept == stood::kPrints && loose::Has(id) && after > -0.002f && after < 0.004f && used && again,
                   "s19 the M249 lying on the deck (%.1f mm over it) still lies there (%.1f mm) after 260 places kept 200 m off (%d kept), asking every "
                   "0.25 s; all %d kept, the one longest unused makes way: one just asked %s, one stood on again %s",
                   onDeck * 1000.0f, after * 1000.0f, kept, stood::kPrints, used ? "stays" : "FORGOTTEN", again ? "stays" : "FORGOTTEN");
        }
    }

    std::printf("%d checks, %d failed\n", g_checks, g_failures);
    return g_failures;
}

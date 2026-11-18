#include "shellload.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "ammozone.h"

namespace shellload {

// The M1014 (Semper Fidelis; measured 2026-11-18 from its mesh dump mesh_13377_0_* and the game's own reloads, build\survey\M1014_partial.bin and
// M1014_dry.bin, on its frame bone 3). Its shell's bone: the shell's line at x -0.0013, y -0.1026, its base at z 0.049, its nose at 0.113. The way
// in is the game's own top-up (3.75 to 4.06 s of M1014_partial.bin): the nose at the loading port's mouth (the lifter's plate, y 0.062, z 0.415 to
// 0.503), up at 63 degrees, tilting forward as it rises, then along the tube's line (y 0.080) until the game hides it. In the ejection port the game
// lays it with its base at (0 0.104 0.463) (4.9 to 5.0 s of M1014_dry.bin). The release: the round button on the receiver's right face under the
// port. The handle: its knob's end on the bolt (bone 0), 6.0 cm of travel (the bolt held open in M1014_dry.bin).
const Gun kM1014 = {
    {-0.0013f, -0.1026f, 0.049f},
    {-0.0013f, -0.1026f, 0.113f},
    7,
    {{0.0f, 0.005f, 0.452f}, {0.0f, 0.031f, 0.462f}, {0.0f, 0.050f, 0.471f}, {0.0f, 0.066f, 0.480f}, {0.0f, 0.071f, 0.504f}, {0.0f, 0.077f, 0.530f}, {0.0f, 0.080f, 0.535f}},
    {63.0f, 52.0f, 35.0f, 27.0f, 15.0f, 3.0f, 0.0f},
    0.015f,
    {-0.001f, 0.104f, 0.463f},
    {-0.008f, 0.106f, 0.495f},
    0.045f,
    {-0.019f, 0.084f, 0.4915f},
    0.03f,
    {-0.030f, 0.1045f, 0.492f},
    0.07f,
    0.060f,
};

const float kHandInGlove[3] = {0.5363f, 1.0976f, 0.2092f};

namespace {

constexpr float kPi = 3.14159265f;
constexpr float kPress = 0.6f, kLetGo = 0.35f;  // a hand's grip or trigger: pressed past the first, let go under the second
constexpr float kOutFor = 0.2f;        // s out of use before what the hand has falls
constexpr float kLostFor = 1.0f;       // s a hand unseen keeps its shell (where it was in the room)
constexpr float kHandleCoast = 0.2f;   // s a hand unseen keeps the handle
constexpr float kEnterReach = 0.03f;   // m: the nose this near the way's first stretch goes onto it
constexpr float kEnterAlong = 0.03f;   // ... the first this much of the way
constexpr float kEnterCos = 0.5f;      // ... turned at most 60 degrees from the way it lies there
constexpr float kRearm = 0.06f;        // m away from the mouth before a shell pulled out or knocked back goes in again
constexpr float kStray = 0.09f;        // m: the hand this far off the way takes the shell off it
// (More than kEnterReach, or a shell taken onto the way short of the mouth is thrown off it the next frame; less than kRearm. In the game 2026-11-18 it
// was 1.5 cm: every shell that came up to the mouth along the port's line went on up to 3 cm short of it and was out again 12 ms later, 5 of 5.)
constexpr float kBackOut = 0.045f;     // m: the hand this far back past the way's start: its nose is clear
constexpr float kFinish = 0.5f;        // m/s: its last stretch by itself
constexpr float kPortCos = 0.5f;       // within 60 degrees of the bore, nose forward
constexpr float kPortHold = 0.3f;      // s after a port drop before that hand presses the release
constexpr float kPokeSpeed = 0.15f;    // m/s on the gun: a hand arriving on the release, or poking it
constexpr float kPokeMemory = 0.15f;   // s that speed is remembered
constexpr float kRackShare = 0.85f;    // of the bolt's travel: drawn this far from home and let go, it's racked
constexpr float kTug = 0.006f;         // m further back on the held-open bolt: a tug
constexpr float kOverTravel = 0.012f;  // m the hand may draw it past its stop (the drawing stops 4 mm past)
constexpr float kBoltHome = 0.07f;     // s for the bolt's whole way home
constexpr float kMostThrow = 3.0f;     // m/s

void Compose(const float* a, const float* b, float* out) {
    float r[12];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            r[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        r[i * 4 + 3] = a[i * 4] * b[3] + a[i * 4 + 1] * b[7] + a[i * 4 + 2] * b[11] + a[i * 4 + 3];
    }
    std::memcpy(out, r, sizeof(r));
}

// A rigid matrix's inverse.
void InvertRigid(const float* m, float* out) {
    float r[12];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            r[i * 4 + j] = m[j * 4 + i];
    for (int i = 0; i < 3; ++i)
        r[i * 4 + 3] = -(r[i * 4] * m[3] + r[i * 4 + 1] * m[7] + r[i * 4 + 2] * m[11]);
    std::memcpy(out, r, sizeof(r));
}

void Transform(const float* m, const float* p, float* out) {
    const float r[3] = {m[0] * p[0] + m[1] * p[1] + m[2] * p[2] + m[3], m[4] * p[0] + m[5] * p[1] + m[6] * p[2] + m[7], m[8] * p[0] + m[9] * p[1] + m[10] * p[2] + m[11]};
    std::memcpy(out, r, sizeof(r));
}

// A 3x4 seen in a mirror across x (both its spaces): the right glove's from the left's (handpose::Mirror).
void Mirror(const float* m, float* out) {
    std::memcpy(out, m, 12 * sizeof(float));
    out[1] = -out[1], out[2] = -out[2], out[3] = -out[3], out[4] = -out[4], out[8] = -out[8];
}

bool Finite(const float* v, int n) {
    for (int i = 0; i < n; ++i)
        if (!std::isfinite(v[i]))
            return false;
    return true;
}

float Distance(const float* a, const float* b) {
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}

// The shell's bone on the gun with its base's middle at base and its nose tilted up from the gun's z by tilt (radians, about x).
void ShellAt(const Gun& gun, const float* base, float tilt, float* out) {
    const float c = std::cos(tilt), s = std::sin(tilt);
    // The bone's z onto (0, s, c), its y onto (0, c, -s), its x kept.
    const float r[12] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, c, s, 0.0f, 0.0f, -s, c, 0.0f};
    std::memcpy(out, r, sizeof(r));
    float at[3];
    Transform(out, gun.baseOnBone, at);
    for (int i = 0; i < 3; ++i)
        out[i * 4 + 3] = base[i] - at[i];
}

// The way's base point and tilt (radians) u along it.
void PathAt(const Gun& gun, float u, float* base, float& tilt) {
    for (int k = 0; k + 1 < gun.keys; ++k) {
        const float length = Distance(gun.keyBase[k], gun.keyBase[k + 1]);
        if (u <= length || k + 2 == gun.keys) {
            const float t = length > 1e-6f ? std::clamp(u / length, 0.0f, 1.0f) : 0.0f;
            for (int i = 0; i < 3; ++i)
                base[i] = gun.keyBase[k][i] + (gun.keyBase[k + 1][i] - gun.keyBase[k][i]) * t;
            tilt = (gun.keyTilt[k] + (gun.keyTilt[k + 1] - gun.keyTilt[k]) * t) * kPi / 180.0f;
            return;
        }
        u -= length;
    }
    std::memcpy(base, gun.keyBase[0], 3 * sizeof(float));
    tilt = gun.keyTilt[0] * kPi / 180.0f;
}

// The point of the way nearest p, within [from, to] along it: how far along, and how far off.
float NearestOnPath(const Gun& gun, const float* p, float from, float to, float& off) {
    float best = from, bestOff = 1e9f, at = 0.0f;
    for (int k = 0; k + 1 < gun.keys; ++k) {
        const float* a = gun.keyBase[k];
        const float* b = gun.keyBase[k + 1];
        const float length = Distance(a, b);
        if (length > 1e-6f && at + length >= from && at <= to) {
            float t = ((p[0] - a[0]) * (b[0] - a[0]) + (p[1] - a[1]) * (b[1] - a[1]) + (p[2] - a[2]) * (b[2] - a[2])) / (length * length);
            t = std::clamp(t, std::max(0.0f, (from - at) / length), std::min(1.0f, (to - at) / length));
            const float q[3] = {a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t};
            if (const float d = Distance(p, q); d < bestOff)
                bestOff = d, best = at + t * length;
        }
        at += length;
    }
    off = bestOff;
    return best;
}

// The first stretch's direction (the port's line at the mouth).
void FirstWay(const Gun& gun, float* way) {
    const float* a = gun.keyBase[0];
    const float* b = gun.keyBase[1];
    const float l = Distance(a, b);
    for (int i = 0; i < 3; ++i)
        way[i] = l > 1e-6f ? (b[i] - a[i]) / l : (i == 1 ? 1.0f : 0.0f);
}

// The shell's bone on the gun u along the way; under 0: short of the mouth, back down the port's line from the way's start, turned as at the start.
void ShellOnWay(const Gun& gun, float u, float* out) {
    ShellOnPath(gun, u, out);
    if (u < 0.0f) {
        float way[3];
        FirstWay(gun, way);
        out[3] += way[0] * u, out[7] += way[1] * u, out[11] += way[2] * u;
    }
}

// Where the hand has the shell along the way (its base at `base` on the gun), looked for within 3 cm of `around`: over 0 on the way, under 0 short of the
// mouth (back down the port's line from the way's start); off: how far from the way.
float AlongWay(const Gun& gun, const float* base, float around, float length, float& off) {
    const float at = std::max(around, 0.0f);
    const float along = NearestOnPath(gun, base, std::max(0.0f, at - 0.03f), std::min(length, at + 0.03f), off);
    if (along > 0.0f)
        return along;
    float way[3];
    FirstWay(gun, way);
    const float* a = gun.keyBase[0];
    const float ahead = (base[0] - a[0]) * way[0] + (base[1] - a[1]) * way[1] + (base[2] - a[2]) * way[2];
    return std::min(ahead, 0.0f);
}

void FromNeck(const Frame& f, const float* gripRoom, float* out) {
    const float forward[3] = {-std::sin(f.yaw), 0.0f, -std::cos(f.yaw)};
    const float right[3] = {std::cos(f.yaw), 0.0f, -std::sin(f.yaw)};
    const float d[3] = {gripRoom[0] - f.neck[0], gripRoom[1] - f.neck[1], gripRoom[2] - f.neck[2]};
    out[0] = d[0] * right[0] + d[2] * right[2];
    out[1] = d[1];
    out[2] = d[0] * forward[0] + d[2] * forward[2];
}

struct Hand {
    bool pressed = false;       // its grip or trigger held (with the hysteresis)
    bool known = false;         // ... seen once (a hand that arrives squeezed is no fresh squeeze)
    bool onRelease = false;     // it pressed the release this visit (off it and on again presses again)
    bool haveTouch = false;     // lastTouch is last frame's
    float lastTouch[3] = {};    // its nearest touch to the release last frame, on the gun
    float poke = 0.0f;          // its touch's speed on the gun lately (m/s), and how long ago
    float pokeAge = 1e9f;
    float sincePort = 1e9f;     // s since it laid a shell into the ejection port
};

struct State {
    Hand hand[2];
    // A shell in the hand at work.
    bool have = false;
    int side = 0;
    bool onLine = false;        // ... on the way in through the loading port
    float u = 0.0f;
    bool caught = false;        // ... past the latch
    bool rearm = false;         // a shell pulled out or knocked back: not in again until it's been taken away
    float shellRoom[12] = {};   // its bone in the room as last known (a hand unseen keeps it there)
    bool haveRoom = false;
    float lastAt[3] = {};       // ... its base there a frame ago, and that frame's length (its speed when let go of)
    float lastDt = 0.0f;
    bool haveLast = false;
    float velocity[3] = {};
    float lostFor = 0.0f;
    float outFor = 0.0f;
    float glove = 0.0f;         // the hand's glove on the gun, eased
    float gloveOnGun[12] = {};
    float fingers = 0.0f;       // the pinch, eased
    // The bolt handle.
    int handle = -1;            // the side holding it
    bool fromLocked = false;
    float grabZ = 0.0f;         // the hand's middle along the gun as it took it, and the bolt's place then
    float pull0 = 0.0f;
    float pull = 0.0f, pullMost = 0.0f;
    float handleLost = 0.0f;
    float gloveOnKnob[12] = {}; // the glove as it took the knob, on the gun, the knob's own move taken out
    // The bolt as drawn.
    float bolt = 0.0f;
    int boltWait = 0;           // frames it waits where it is for the count's answer to a rack
    bool cleared = true;
};
State g;

float g_inGlove[2][12] = {};  // the shell's bone in each glove's bind space
const float (*g_fingers)[12] = nullptr;
bool g_haveHold = false;

// The shell's bone in the view from this side's glove.
void InHand(int side, const float* glove, float* out) {
    Compose(glove, g_inGlove[side & 1], out);
}

void DropFromHand(Result& r, const Frame& f, const float* shellView, bool fromHand) {
    r.letGo = true;
    r.dropFromHand = fromHand;
    std::memcpy(r.dropView, shellView, sizeof(r.dropView));
    float speed = std::sqrt(g.velocity[0] * g.velocity[0] + g.velocity[1] * g.velocity[1] + g.velocity[2] * g.velocity[2]);
    const float scale = fromHand && std::isfinite(speed) ? (speed > kMostThrow ? kMostThrow / speed : 1.0f) : 0.0f;
    for (int i = 0; i < 3; ++i)
        r.dropVelocityRoom[i] = g.velocity[i] * scale;
    (void)f;
    g.have = false, g.onLine = false, g.caught = false, g.haveRoom = false, g.haveLast = false;
}

}  // namespace

float PathLength(const Gun& gun) {
    float length = 0.0f;
    for (int k = 0; k + 1 < gun.keys; ++k)
        length += Distance(gun.keyBase[k], gun.keyBase[k + 1]);
    return length;
}

void ShellOnPath(const Gun& gun, float u, float* out) {
    float base[3], tilt = 0.0f;
    PathAt(gun, std::clamp(u, 0.0f, PathLength(gun)), base, tilt);
    ShellAt(gun, base, tilt, out);
}

void ShellInPort(const Gun& gun, float* out) {
    ShellAt(gun, gun.portBase, 0.0f, out);
}

void GloveFor(int side, const float* shellView, float* glove) {
    float inverse[12];
    InvertRigid(g_inGlove[side & 1], inverse);
    Compose(shellView, inverse, glove);
}

void SetHold(const float* shellInGlove, const float (*fingers)[12]) {
    std::memcpy(g_inGlove[0], shellInGlove, sizeof(g_inGlove[0]));
    Mirror(g_inGlove[0], g_inGlove[1]);
    g_fingers = fingers;
    g_haveHold = true;
}

void Clear() {
    g = State{};
}

bool Holding() {
    return g.have || g.handle >= 0;
}

Result Update(const Frame& f) {
    Result r;
    const float dt = std::isfinite(f.dt) && f.dt > 0.0f ? std::min(f.dt, 0.1f) : 0.0f;
    const Gun* gun = f.gun;
    if (!g_haveHold) {  // (no hold given: the shell along the glove's own z, 10 cm out, so nothing is ever a number not finite)
        const float plain[12] = {1, 0, 0, kHandInGlove[0], 0, 1, 0, kHandInGlove[1], 0, 0, 1, kHandInGlove[2] + 0.1f};
        SetHold(plain, nullptr);
    }
    for (Hand& h : g.hand)
        h.sincePort += dt, h.pokeAge += dt;
    r.side = f.gunHand == 0 ? 1 : 0;
    // Out of use: kept a moment, then what the hand has falls and the handle is let go of.
    if (!f.active || !gun) {
        g.outFor += dt;
        if (g.outFor > kOutFor || !gun) {
            if (g.have && g.haveRoom) {
                float toView[12], view[12];
                InvertRigid(f.headPose, toView);
                Compose(toView, g.shellRoom, view);
                if (Finite(view, 12)) {
                    r.side = g.side;
                    DropFromHand(r, f, view, !g.onLine);
                }
            }
            g.have = false, g.onLine = false, g.caught = false, g.handle = -1, g.glove = 0.0f, g.fingers = 0.0f, g.rearm = false;
            g.bolt = 0.0f, g.boltWait = 0;
            for (Hand& h : g.hand)
                h = Hand{};
        }
        r.state = "not in use";
        return r;
    }
    g.outFor = 0.0f;
    r.active = true;
    const int work = f.gunHand == 0 ? 1 : 0;
    const float length = PathLength(*gun);
    float toGun[12] = {};
    const bool gunOk = f.gunTracked && Finite(f.gunPose, 12);
    if (gunOk)
        InvertRigid(f.gunPose, toGun);
    // Each hand's squeeze: pressed, let go, fresh.
    bool fresh[2] = {}, released[2] = {}, seen[2] = {};
    for (int s = 0; s < 2; ++s) {
        const HandIn& in = f.hand[s];
        Hand& h = g.hand[s];
        seen[s] = in.tracked && Finite(in.glove, 12) && Finite(in.gripRoom, 3);
        const float hold = std::fmax(std::isfinite(in.squeeze) ? in.squeeze : 0.0f, std::isfinite(in.trigger) ? in.trigger : 0.0f);
        const bool was = h.pressed;
        if (!seen[s]) {
            // (Unseen: neither pressed anew nor let go.)
        } else if (!h.known) {
            h.pressed = hold >= kPress;
            h.known = true;
        } else if (!h.pressed && hold >= kPress) {
            h.pressed = true;
            fresh[s] = true;
        } else if (h.pressed && hold < kLetGo) {
            h.pressed = false;
        }
        released[s] = was && !h.pressed;
    }
    // ---- A shell in the hand ----
    if (g.have && g.side == f.gunHand) {
        // The hand that carries it took the gun: it falls from where it was.
        float toView[12], view[12];
        InvertRigid(f.headPose, toView);
        Compose(toView, g.shellRoom, view);
        r.side = g.side;
        if (g.haveRoom && Finite(view, 12))
            DropFromHand(r, f, view, true);
        g.have = false, g.onLine = false, g.caught = false;
    }
    float shellView[12] = {}, shellGun[12] = {};
    bool shellSeen = false;
    if (g.have) {
        const int s = g.side;
        r.side = s;
        if (seen[s]) {
            g.lostFor = 0.0f;
            InHand(s, f.hand[s].glove, shellView);
            shellSeen = Finite(shellView, 12);
        } else {
            g.lostFor += dt;
        }
        if (shellSeen && !g.onLine) {
            float room[12];
            Compose(f.headPose, shellView, room);
            const float at[3] = {room[3], room[7], room[11]};
            if (g.haveLast && dt > 1e-4f)
                for (int i = 0; i < 3; ++i)
                    g.velocity[i] = (at[i] - g.lastAt[i]) / dt;
            std::memcpy(g.lastAt, at, sizeof(at));
            g.haveLast = true;
            std::memcpy(g.shellRoom, room, sizeof(room));
            g.haveRoom = true;
        } else if (!shellSeen) {
            g.haveLast = false;
            if (g.haveRoom) {  // where it was in the room
                float toView[12];
                InvertRigid(f.headPose, toView);
                Compose(toView, g.shellRoom, shellView);
            }
        }
        if (g.lostFor > kLostFor) {
            // The hand unseen a second: it falls from where it was.
            g.velocity[0] = g.velocity[1] = g.velocity[2] = 0.0f;
            if (g.haveRoom && Finite(shellView, 12))
                DropFromHand(r, f, shellView, !g.onLine);
            g.have = false, g.onLine = false, g.caught = false;
        }
    }
    // On the way in: moved by the hand, or by itself past the latch.
    if (g.have && g.onLine) {
        const int s = g.side;
        if (!gunOk) {
            // (The gun unseen this frame: it stays where it is on it.)
        } else if (g.caught) {
            g.u += kFinish * dt;
            if (g.u >= length) {
                r.loaded = true;
                g.have = false, g.onLine = false, g.caught = false, g.haveRoom = false, g.haveLast = false;
            }
        } else if (released[s] && shellSeen) {
            // Let go of part way: it falls out of the port from where it was.
            float on[12], view[12];
            ShellOnWay(*gun, g.u, on);
            Compose(f.gunPose, on, view);
            g.velocity[0] = g.velocity[1] = g.velocity[2] = 0.0f;
            DropFromHand(r, f, view, false);
            g.rearm = false;
        } else if (!f.canLoad || f.gameReload) {
            // The tube filled meanwhile (the game's own reload), or that reload is asked for: back into the hand.
            g.onLine = false;
            g.rearm = true;
            r.pulledOut = true;
            r.note = f.gameReload ? "the game's own reload is asked for" : "the tube took no more";
        } else if (shellSeen) {
            Compose(toGun, shellView, shellGun);
            float base[3], off = 0.0f;
            Transform(shellGun, gun->baseOnBone, base);
            // Short of the mouth (it goes onto the way up to kEnterReach before it) the shell is where the hand has it down the port's line; kBackOut
            // back past the way's start its nose is clear.
            const float along = AlongWay(*gun, base, g.u, length, off);
            if (off > kStray || along < -kBackOut) {
                g.onLine = false;
                g.rearm = true;
                r.pulledOut = true;
            } else {
                g.u = along;
                if (g.u >= length - gun->latch) {
                    g.caught = true;
                    r.caught = true;
                }
            }
        }
    }
    // In the hand, off the gun: let go of, the ejection port, the loading port's mouth.
    if (g.have && !g.onLine) {
        const int s = g.side;
        if (shellSeen && released[s]) {
            float fromNeck[3];
            FromNeck(f, f.hand[s].gripRoom, fromNeck);
            if (f.haveBody && (f.hand[s].forceBelly || ammozone::Inside(fromNeck, ammozone::kPutBackMore))) {
                r.putBack = true;
                g.have = false, g.haveRoom = false, g.haveLast = false;
            } else {
                DropFromHand(r, f, shellView, true);
            }
            g.rearm = false;
        } else if (shellSeen && gunOk) {
            Compose(toGun, shellView, shellGun);
            float base[3], nose[3];
            Transform(shellGun, gun->baseOnBone, base);
            Transform(shellGun, gun->noseOnBone, nose);
            const float span = Distance(base, nose);
            const float axis[3] = {(nose[0] - base[0]) / std::fmax(span, 1e-6f), (nose[1] - base[1]) / std::fmax(span, 1e-6f), (nose[2] - base[2]) / std::fmax(span, 1e-6f)};
            const float middle[3] = {(base[0] + nose[0]) * 0.5f, (base[1] + nose[1]) * 0.5f, (base[2] + nose[2]) * 0.5f};
            // The mouth: the nose's place at the way's start.
            float startBase[3], startTilt = 0.0f, startNose[3];
            PathAt(*gun, 0.0f, startBase, startTilt);
            startNose[0] = startBase[0], startNose[1] = startBase[1] + span * std::sin(startTilt), startNose[2] = startBase[2] + span * std::cos(startTilt);
            if (g.rearm && Distance(nose, startNose) > kRearm)
                g.rearm = false;
            if (f.portOpen && Distance(middle, gun->portAt) <= gun->portReach && axis[2] >= kPortCos) {
                r.portDrop = true;
                g.hand[s].sincePort = 0.0f;
                g.have = false, g.haveRoom = false, g.haveLast = false;
            } else if (!g.rearm && !f.gameReload) {
                // The nose near the way's first stretch, the shell turned about as it lies there.
                float bestOff = 1e9f, bestU = 0.0f;
                for (int n = 0; n <= 6; ++n) {
                    const float u = kEnterAlong * static_cast<float>(n) / 6.0f;
                    float b[3], tilt = 0.0f;
                    PathAt(*gun, u, b, tilt);
                    const float at[3] = {b[0], b[1] + span * std::sin(tilt), b[2] + span * std::cos(tilt)};
                    const float way[3] = {0.0f, std::sin(tilt), std::cos(tilt)};
                    const float turned = axis[0] * way[0] + axis[1] * way[1] + axis[2] * way[2];
                    if (const float d = Distance(nose, at); turned >= kEnterCos && d < bestOff)
                        bestOff = d, bestU = u;
                }
                if (bestOff <= kEnterReach) {
                    if (f.canLoad) {
                        g.onLine = true;
                        g.caught = false;
                        // (Where the hand has it: on the way, or short of the mouth down the port's line.)
                        float off = 0.0f;
                        g.u = std::max(AlongWay(*gun, base, bestU, length, off), -kEnterReach);
                        r.entered = true;
                    } else if (f.tubeFull) {
                        g.rearm = true;
                        r.fullKnock = true;
                        r.note = "the tube is full";
                    }
                }
            }
        }
    }
    // ---- A fresh squeeze of the hand at work: a shell from the belly, or the bolt handle ----
    if (fresh[work] && !g.have && g.handle < 0 && !f.hand[work].busy) {
        const HandIn& in = f.hand[work];
        float fromNeck[3];
        FromNeck(f, in.gripRoom, fromNeck);
        const bool atBelly = f.haveBody && (in.forceBelly || ammozone::Inside(fromNeck));
        bool tookHandle = false;
        if (gunOk && !atBelly) {
            float middle[3], onGun[3];
            Transform(in.glove, kHandInGlove, middle);
            if (work == 1) {  // (the right glove's bind space is the left's mirrored)
                const float mirrored[3] = {-kHandInGlove[0], kHandInGlove[1], kHandInGlove[2]};
                Transform(in.glove, mirrored, middle);
            }
            Transform(toGun, middle, onGun);
            const float knob[3] = {gun->handle[0], gun->handle[1], gun->handle[2] - g.bolt};
            if (Distance(onGun, knob) <= gun->handleReach) {
                g.handle = work;
                g.fromLocked = f.locked;
                g.grabZ = onGun[2];
                g.pull0 = g.bolt;
                g.pull = g.bolt, g.pullMost = g.bolt;
                g.handleLost = 0.0f;
                float gloveGun[12];
                Compose(toGun, in.glove, gloveGun);
                gloveGun[11] += g.bolt;  // (as on the knob at home)
                std::memcpy(g.gloveOnKnob, gloveGun, sizeof(gloveGun));
                r.handleTaken = true;
                r.spentSqueeze = true;
                tookHandle = true;
            }
        }
        if (!tookHandle && atBelly) {
            r.spentSqueeze = true;
            if (f.take == Take::kYes) {
                g.have = true;
                g.side = work;
                g.onLine = false, g.caught = false, g.rearm = false;
                g.lostFor = 0.0f;
                g.haveLast = false;
                g.velocity[0] = g.velocity[1] = g.velocity[2] = 0.0f;
                InHand(work, in.glove, shellView);
                Compose(f.headPose, shellView, g.shellRoom);
                g.haveRoom = Finite(g.shellRoom, 12);
                shellSeen = g.haveRoom;
                r.took = true;
            } else {
                r.squeezed = true;
                std::memcpy(r.fromNeck, fromNeck, sizeof(fromNeck));
                r.note = f.take == Take::kNoShells ? "no shells left" : "the tube is full";
            }
        }
    }
    // ---- The bolt handle held ----
    if (g.handle >= 0) {
        const int s = g.handle;
        const HandIn& in = f.hand[s];
        bool let = released[s] || s == f.gunHand;
        if (!seen[s]) {
            g.handleLost += dt;
            let = let || g.handleLost > kHandleCoast;
        } else {
            g.handleLost = 0.0f;
            if (gunOk && !let) {
                float middle[3], onGun[3];
                const float bind[3] = {s == 1 ? -kHandInGlove[0] : kHandInGlove[0], kHandInGlove[1], kHandInGlove[2]};
                Transform(in.glove, bind, middle);
                Transform(toGun, middle, onGun);
                g.pull = std::clamp(g.pull0 + (g.grabZ - onGun[2]), 0.0f, gun->travel + kOverTravel);
                g.pullMost = std::max(g.pullMost, g.pull);
            }
        }
        if (let) {
            r.handleLet = true;
            r.handlePulled = g.pullMost;
            if (g.fromLocked)
                r.tugged = g.pullMost - g.pull0 >= kTug;
            else
                r.racked = g.pullMost >= kRackShare * gun->travel;
            g.bolt = std::min(g.pull, gun->travel);
            g.boltWait = r.tugged || r.racked ? 2 : 0;  // (the count's answer comes next frame: held open, or home)
            g.handle = -1;
        }
    }
    // ---- The bolt release ----
    for (int s = 0; s < 2 && gunOk; ++s) {
        Hand& h = g.hand[s];
        if (s == f.gunHand || !seen[s] || g.handle == s) {
            h.onRelease = false, h.haveTouch = false;
            continue;
        }
        const HandIn& in = f.hand[s];
        float nearest[3] = {}, best = 1e9f;
        const int count = std::clamp(in.touches, 0, 4);
        for (int k = 0; k < std::max(count, 1); ++k) {
            float view[3], onGun[3];
            if (count > 0) {
                std::memcpy(view, in.touch[k], sizeof(view));
            } else {
                const float bind[3] = {s == 1 ? -kHandInGlove[0] : kHandInGlove[0], kHandInGlove[1], kHandInGlove[2]};
                Transform(in.glove, bind, view);
            }
            if (!Finite(view, 3))
                continue;
            Transform(toGun, view, onGun);
            if (const float d = Distance(onGun, gun->release); d < best)
                best = d, std::memcpy(nearest, onGun, sizeof(nearest));
        }
        if (best > 1e8f) {
            h.onRelease = false, h.haveTouch = false;
            continue;
        }
        if (h.haveTouch && dt > 1e-4f) {
            if (const float speed = Distance(nearest, h.lastTouch) / dt; speed >= kPokeSpeed)
                h.poke = speed, h.pokeAge = 0.0f;
        }
        std::memcpy(h.lastTouch, nearest, sizeof(nearest));
        h.haveTouch = true;
        const bool on = best <= gun->releaseReach;
        const bool poked = h.pokeAge <= kPokeMemory;
        // Arriving on it with some speed, or poking it while on it; once a visit (off it and on again, or still and poked again).
        if (!on)
            h.onRelease = false;
        if (on && poked && !h.onRelease && f.locked && h.sincePort >= kPortHold && !(g.have && g.side == s && g.onLine)) {
            h.onRelease = true;
            r.released = true;
            r.side = s;
        }
    }
    // ---- The bolt as drawn ----
    if (g.handle >= 0) {
        g.bolt = std::min(g.pull, gun->travel + 0.004f);
    } else if (g.boltWait > 0) {
        --g.boltWait;
    } else if (f.locked) {
        g.bolt = gun->travel;
    } else if (g.bolt > 0.0f) {
        g.bolt = std::max(0.0f, g.bolt - gun->travel / kBoltHome * dt);
    }
    r.boltBack = g.bolt;
    r.handleHeld = g.handle >= 0 && (g.fromLocked || g.pull > 0.005f);
    // ---- The hand at work as drawn ----
    const bool riding = g.have && g.onLine;
    if (g.handle >= 0) {
        r.side = g.handle;
        std::memcpy(g.gloveOnGun, g.gloveOnKnob, sizeof(g.gloveOnGun));
        g.gloveOnGun[11] -= g.bolt;
        g.glove = std::min(1.0f, g.glove + dt / 0.06f);
        r.fist = true;
    } else if (riding) {
        r.side = g.side;
        float on[12], inverse[12];
        ShellOnWay(*gun, g.u, on);
        InvertRigid(g_inGlove[g.side], inverse);
        Compose(on, inverse, g.gloveOnGun);
        g.glove = std::min(1.0f, g.glove + dt / 0.08f);
        r.lineShell = true;
        std::memcpy(r.shellOnGun, on, sizeof(on));
    } else {
        g.glove = std::max(0.0f, g.glove - dt / 0.12f);
    }
    r.gloveBlend = g.glove;
    std::memcpy(r.gloveOnGun, g.gloveOnGun, sizeof(r.gloveOnGun));
    if (g.have && !g.onLine) {
        r.side = g.side;
        r.handShell = Finite(shellView, 12) && (shellSeen || g.haveRoom);
        std::memcpy(r.shellView, shellView, sizeof(r.shellView));
        std::memcpy(r.shellInGlove, g_inGlove[g.side], sizeof(r.shellInGlove));
    }
    g.fingers += ((g.have ? 1.0f : 0.0f) - g.fingers) * std::min(1.0f, dt / 0.05f);
    if (r.took)
        g.fingers = 1.0f;  // (taken: the pinch at once, the shell is drawn in it)
    r.fingers = g_fingers;
    r.fingersShare = g_fingers ? g.fingers : 0.0f;
    r.holding = g.have || g.handle >= 0;
    r.state = g.handle >= 0 ? (g.fromLocked ? "the handle held, the bolt held open" : "the handle held")
              : g.caught   ? "a shell going home in the tube"
              : riding     ? "a shell on its way up the loading port"
              : g.have     ? "a shell in the hand"
              : f.locked   ? "the bolt held open"
                           : "nothing in the hand";
    return r;
}

}  // namespace shellload

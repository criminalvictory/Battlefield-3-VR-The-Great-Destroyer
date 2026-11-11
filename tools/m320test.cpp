// The grenade launcher under the rifle reloaded by hand (src/m320.cpp, as hands.cpp runs it), on scripted frames without the game: the release and the
// game's own swing open, the snap shut, gravity tipping the spent case out, the jab, the hand on the rim, the belly's two claims, the magnet, the count
// (as ammo.cpp keeps it), when the left trigger may fire, the wrist's flick, a lost hand, the game's own reload; then the push shut by touch, the barrel
// opened by the hand, the release refused, the right hand at work, the launcher gone mid-reload, broken inputs, the launcher's shape against the
// recording, the round's way home without a jump, and the time a frame takes. Since the review: the hand off the rim lets go, gunRoom a frame late with
// uneven frames, only the gun lost, two drops in a frame, a frame not in use, let go of at the stop, the push by the glove's own points, a case sliding
// back in muzzle down, the right hand's own rim, a fitted pinch, broken inputs while holding, the game's reload ready to fire, a stale in-hand reading.
// Either hand (rows 36-44): the right's round and holds the left's flipped (palms given and not, the carry mixed), the barrel's back end the left's only,
// the right's slap, the flick mirrored with the rifle in the left hand, the release pressed by the hand on the launcher's grip, a spent case put in for the
// film rounds, a scripted hand's glove where the module reaches, and the right's whole rim and push home frame for frame against the left's.
// The room is y up, metres: the head level at (0 1.2 0) faces -z. The view is the head's own space (x right, y up, z back). The gun is held level in
// front of the chest, pointing ahead (its x left, y up, z to the muzzle).
// Usage: m320test   The exit code is the number of failed checks.

#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <limits>

#include "../src/ammozone.h"
#include "../src/m320.h"
#include "../src/m320carry.h"

namespace {

constexpr float kPi = 3.14159265f, kDegree = kPi / 180.0f;
constexpr float kDt = 1.0f / 72.0f;
constexpr float kMid[3] = {0.5363f, 1.0976f, 0.2092f};  // the hand's middle in the left glove's bind space
constexpr float kTip = 30.0f * kDegree, kMu = 0.3f, kG = 9.81f;

int g_failed = 0, g_checks = 0;

void Check(bool ok, const char* format, ...) {
    ++g_checks;
    if (!ok)
        ++g_failed;
    char text[600];
    va_list args;
    va_start(args, format);
    std::vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    std::printf("%s %s\n", ok ? "ok  " : "FAIL", text);
}

// ---- Matrices: three rows, rotation | translation ----
void Identity(float* m) {
    const float i[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    std::memcpy(m, i, sizeof(i));
}

void Compose(const float* a, const float* b, float* out) {
    float r[12];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j)
            r[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        r[i * 4 + 3] += a[i * 4 + 3];
    }
    std::memcpy(out, r, sizeof(r));
}

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
    float r[3];
    for (int i = 0; i < 3; ++i)
        r[i] = m[i * 4] * p[0] + m[i * 4 + 1] * p[1] + m[i * 4 + 2] * p[2] + m[i * 4 + 3];
    std::memcpy(out, r, sizeof(r));
}

void Turn(const float* m, const float* v, float* out) {
    float r[3];
    for (int i = 0; i < 3; ++i)
        r[i] = m[i * 4] * v[0] + m[i * 4 + 1] * v[1] + m[i * 4 + 2] * v[2];
    std::memcpy(out, r, sizeof(r));
}

float Dot(const float* a, const float* b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

float Length(const float* v) {
    return std::sqrt(Dot(v, v));
}

float Apart(const float* a, const float* b) {
    const float d[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    return Length(d);
}

void Cross(const float* a, const float* b, float* o) {
    const float r[3] = {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
    std::memcpy(o, r, sizeof(r));
}

// A turn of angle about the unit axis (no translation).
void Rotation(const float* k, float angle, float* m) {
    const float c = std::cos(angle), s = std::sin(angle), t = 1.0f - c;
    const float r[12] = {t * k[0] * k[0] + c,        t * k[0] * k[1] - s * k[2], t * k[0] * k[2] + s * k[1], 0,
                         t * k[0] * k[1] + s * k[2], t * k[1] * k[1] + c,        t * k[1] * k[2] - s * k[0], 0,
                         t * k[0] * k[2] - s * k[1], t * k[1] * k[2] + s * k[0], t * k[2] * k[2] + c,        0};
    std::memcpy(m, r, sizeof(r));
}

// The turn taking unit a onto unit b.
void RotationBetween(const float* a, const float* b, float* m) {
    float axis[3];
    Cross(a, b, axis);
    const float s = Length(axis), c = Dot(a, b);
    if (s < 1e-7f) {
        Identity(m);
        return;
    }
    for (float& v : axis)
        v /= s;
    Rotation(axis, std::atan2(s, c), m);
}

float Determinant(const float* m) {
    return m[0] * (m[5] * m[10] - m[6] * m[9]) - m[1] * (m[4] * m[10] - m[6] * m[8]) + m[2] * (m[4] * m[9] - m[5] * m[8]);
}

// ---- The launcher's own shape, from the module ----
void ExitOnGun(float angle, float* out) {  // the barrel's way out, on the gun: the round's bone moving back along it
    float a[12], b[12];
    m320::RoundOnLine(angle, 0.0f, a);
    m320::RoundOnLine(angle, 0.1f, b);
    for (int i = 0; i < 3; ++i)
        out[i] = (b[i * 4 + 3] - a[i * 4 + 3]) / 0.1f;
}

void OnTube(float angle, const float* local, float* onGun) {  // a point of the tube as shut, swung with it
    float hinge[12];
    m320::HingeAt(angle, hinge);
    Transform(hinge, local, onGun);
}

void ToTube(float angle, const float* onGun, float* local) {
    float hinge[12], back[12];
    m320::HingeAt(angle, hinge);
    InvertRigid(hinge, back);
    Transform(back, onGun, local);
}

// Any 3x4's inverse (a round in the glove re-anchored on a palm that widens isn't rigid).
void InvertAny(const float* m, float* out) {
    const float a = m[0], b = m[1], c = m[2], d = m[4], e = m[5], f = m[6], g = m[8], h = m[9], i = m[10];
    const float A = e * i - f * h, B = f * g - d * i, C = d * h - e * g;
    const float det = a * A + b * B + c * C;
    const float r[9] = {A / det, (c * h - b * i) / det, (b * f - c * e) / det, B / det, (a * i - c * g) / det, (c * d - a * f) / det,
                        C / det, (b * g - a * h) / det, (a * e - b * d) / det};
    for (int k = 0; k < 3; ++k) {
        out[k * 4] = r[k * 3], out[k * 4 + 1] = r[k * 3 + 1], out[k * 4 + 2] = r[k * 3 + 2];
        out[k * 4 + 3] = -(r[k * 3] * m[3] + r[k * 3 + 1] * m[7] + r[k * 3 + 2] * m[11]);
    }
}

void RoundPose(const float* axis, const float* nose, float* out);

// ---- The scene ----
constexpr float kGunLevel[12] = {-1, 0, 0, 0.15f, 0, 1, 0, -0.25f, 0, 0, -1, -0.20f};  // the gun in the view: ahead, 15 cm right, 25 cm down

void SetGunRoom(m320::Frame& f, const float* room) {
    std::memcpy(f.gunRoom, room, sizeof(f.gunRoom));
    float fromRoom[12];
    InvertRigid(f.headPose, fromRoom);
    Compose(fromRoom, room, f.gunPose);
    f.haveGunRoom = true;
}

void LevelGun(m320::Frame& f) {
    float room[12];
    Compose(f.headPose, kGunLevel, room);
    SetGunRoom(f, room);
}

// The gun's turn in the room changed by turn (on its own axes: the gun's body turns about itself), its place kept.
void TurnGunOwn(m320::Frame& f, const float* turn) {
    float room[12];
    Compose(f.gunRoom, turn, room);
    SetGunRoom(f, room);
}

// The gun turned in the room so the open barrel's way out points this far below level (the muzzle up), as from level.
void TiltExit(m320::Frame& f, float down) {
    float level[12], exitGun[3], exitRoom[3];
    Compose(f.headPose, kGunLevel, level);
    ExitOnGun(m320::kOpenMost, exitGun);
    Turn(level, exitGun, exitRoom);
    const float flat = std::sqrt(exitRoom[0] * exitRoom[0] + exitRoom[2] * exitRoom[2]);
    const float want[3] = {exitRoom[0] / flat * std::cos(down), -std::sin(down), exitRoom[2] / flat * std::cos(down)};
    float turn[12], room[12];
    RotationBetween(exitRoom, want, turn);
    Compose(turn, level, room);
    room[3] = level[3], room[7] = level[7], room[11] = level[11];
    SetGunRoom(f, room);
}

// The hand's middle at this place in the view's space (its turn kept); its grip point there too.
void PutHandInView(m320::Frame& f, int side, const float* view) {
    float* g = f.hand[side].glove;
    const float mid[3] = {side == 1 ? -kMid[0] : kMid[0], kMid[1], kMid[2]};
    float turnOnly[12], at[3];
    std::memcpy(turnOnly, g, sizeof(turnOnly));
    turnOnly[3] = turnOnly[7] = turnOnly[11] = 0.0f;
    Transform(turnOnly, mid, at);
    g[3] = view[0] - at[0], g[7] = view[1] - at[1], g[11] = view[2] - at[2];
    Transform(f.headPose, view, f.hand[side].gripRoom);
}

void PutHandOnGun(m320::Frame& f, int side, const float* onGun) {
    float view[3];
    Transform(f.gunPose, onGun, view);
    PutHandInView(f, side, view);
}

void HandOnGun(const m320::Frame& f, int side, float* onGun) {
    const float mid[3] = {side == 1 ? -kMid[0] : kMid[0], kMid[1], kMid[2]};
    float view[3], inverse[12];
    Transform(f.hand[side].glove, mid, view);
    InvertRigid(f.gunPose, inverse);
    Transform(inverse, view, onGun);
}

// The belly as pouch.cpp places it: the neck point 7.5 cm below and 8 cm behind the eyes, the zone's middle 43 cm below it and 13 cm forward.
void BellyInView(const m320::Frame& f, float* view) {
    const float room[3] = {f.neck[0] + ammozone::kBelly[0], f.neck[1] + ammozone::kBelly[1], f.neck[2] - ammozone::kBelly[2]};
    float fromRoom[12];
    InvertRigid(f.headPose, fromRoom);
    Transform(fromRoom, room, view);
}

constexpr float kAway[3] = {0.30f, -0.25f, 0.20f};  // on the gun: 30 cm to its left and below, near nothing

m320::Frame Base() {
    m320::Frame f;
    f.dt = kDt;
    f.active = true;
    Identity(f.headPose);
    f.headPose[7] = 1.2f;
    LevelGun(f);
    for (int side = 0; side < 2; ++side) {
        f.hand[side].tracked = true;
        Identity(f.hand[side].glove);
    }
    f.gunHand = 1;
    f.neck[0] = 0.0f, f.neck[1] = 1.2f - 0.075f, f.neck[2] = 0.08f;
    f.yaw = 0.0f;
    f.haveBody = true;
    f.loaded = true;
    f.spare = true;
    PutHandOnGun(f, 0, kAway);
    const float rifleGrip[3] = {0.0f, -0.08f, 0.22f};
    PutHandOnGun(f, 1, rifleGrip);
    return f;
}

struct Counts {
    int released = 0, openStop = 0, contentMoving = 0, unload = 0, took = 0, putBack = 0, caught = 0, seated = 0, caseSeated = 0, shut = 0, latchedNow = 0,
        dryPull = 0, flickOpen = 0, flickShut = 0, barrelTaken = 0, barrelLet = 0, sprungBack = 0, blocked = 0, pinched = 0, freed = 0, jabbed = 0,
        letGo = 0, cancelled = 0, fired = 0, reloaded = 0, drop = 0, squeezed = 0, slapped = 0, entered = 0;
};

// The module with the count kept as ammo.cpp keeps it: a round seated by hand takes one of the spare ones, a live one out gives it back.
struct Sim {
    m320::Frame f = Base();
    int loaded = 1, spare = 3;
    Counts n;
    m320::Result r;
    float time = 0.0f;
    // lag: gunRoom as hands.cpp gives it, the gun as drawn the frame before (each SetGunRoom still sets this frame's gun; gunPose is this frame's).
    bool lag = false, haveLag = false;
    float lagRoom[12] = {};

    const m320::Result& Step() {
        f.loaded = loaded >= 1;
        f.spare = spare >= 1;
        float now[12];
        if (lag) {
            std::memcpy(now, f.gunRoom, sizeof(now));
            std::memcpy(f.gunRoom, lagRoom, sizeof(f.gunRoom));
            f.haveGunRoom = haveLag;
        }
        r = m320::Update(f);
        if (lag) {
            std::memcpy(lagRoom, now, sizeof(lagRoom));
            haveLag = true;
            std::memcpy(f.gunRoom, now, sizeof(f.gunRoom));
            f.haveGunRoom = true;
        }
        time += f.dt;
        if (r.seated && loaded < 1 && spare >= 1)
            loaded = 1, --spare;
        if (r.unload && loaded >= 1)
            loaded = 0, ++spare;
        n.released += r.released, n.openStop += r.openStop, n.contentMoving += r.contentMoving, n.unload += r.unload, n.took += r.took;
        n.putBack += r.putBack, n.caught += r.caught, n.seated += r.seated, n.caseSeated += r.caseSeated, n.shut += r.shut, n.latchedNow += r.latchedNow;
        n.dryPull += r.dryPull, n.flickOpen += r.flickOpen, n.flickShut += r.flickShut, n.barrelTaken += r.barrelTaken, n.barrelLet += r.barrelLet;
        n.sprungBack += r.sprungBack, n.blocked += r.blocked, n.pinched += r.pinched, n.freed += r.freed, n.jabbed += r.jabbed, n.letGo += r.letGo;
        n.cancelled += r.cancelled, n.fired += r.fired, n.reloaded += r.reloaded, n.drop += r.drop, n.squeezed += r.squeezed;
        n.slapped += r.slapped, n.entered += r.entered;
        f.releasePress = false;
        return r;
    }
    void Run(float seconds) {
        const int frames = static_cast<int>(seconds / f.dt + 0.5f);
        for (int i = 0; i < frames; ++i)
            Step();
    }
    template <class Pred>
    float RunUntil(Pred done, float most) {  // seconds it took, or -1
        const float from = time;
        const int frames = static_cast<int>(most / f.dt + 0.5f);
        for (int i = 0; i < frames; ++i) {
            Step();
            if (done(r))
                return time - from;
        }
        return -1.0f;
    }
    void Fire() {  // the game fired it: the count falls
        loaded = 0;
        Step();
    }
    void OnGripB() {
        f.hand[0].onGripB = true;
        f.hand[0].squeeze = 1.0f;
        f.hand[0].trigger = 0.0f;
        float grip[3];
        m320::GripMiddle(grip);
        PutHandOnGun(f, 0, grip);
    }
    void OffGrip() {
        f.hand[0].onGripB = false;
        f.hand[0].squeeze = 0.0f;
        f.hand[0].trigger = 0.0f;
        PutHandOnGun(f, 0, kAway);
        Step();
    }
    void Release() {  // input.cpp's pull of the trigger on the grip, not ready to fire
        f.releasePress = true;
        f.hand[0].trigger = 1.0f;
        Step();
        f.hand[0].trigger = 0.0f;
    }
    // Squeezed by the grip on the spot: a frame open, then shut.
    const m320::Result& Squeeze(int side = 0) {
        f.hand[side].squeeze = 0.0f;
        Step();
        f.hand[side].squeeze = 1.0f;
        return Step();
    }
    const m320::Result& LetGo(int side = 0) {
        f.hand[side].squeeze = 0.0f;
        f.hand[side].trigger = 0.0f;
        return Step();
    }
    // Fired, the hand on the grip presses the release, the barrel springs open, the hand goes off the grip.
    void Open() {
        m320::Clear();
        Step();
        Fire();
        OnGripB();
        Step();
        Release();
        Run(0.35f);
        OffGrip();
    }
    // ... and the spent case tipped out (the muzzle up), the gun level again.
    void OpenEmpty() {
        Open();
        TiltExit(f, 60.0f * kDegree);
        RunUntil([](const m320::Result& x) { return x.drop; }, 1.0f);
        LevelGun(f);
        Step();
    }
    // A new round from the belly (its grip point there).
    const m320::Result& TakeAtBelly(int side = 0) {
        float belly[3];
        BellyInView(f, belly);
        PutHandInView(f, side, belly);
        return Squeeze(side);
    }
    // The hand's middle where the game pinches the rim, as the round is now.
    void HandOnRim(float s) {
        float round[12], target[3], onRound[3];
        m320::RoundOnLine(r.barrel, s, round);
        m320::PinchTarget(onRound);
        Transform(round, onRound, target);
        PutHandOnGun(f, 0, target);
    }
    // The round held in the left hand at this pose on the gun (the glove placed for it, so the round is drawn there: as the module last said it sits in the glove,
    // the game's pinch until a round's in the hand).
    void HoldRoundAt(const float* roundOnGun) {
        float view[12];
        Compose(f.gunPose, roundOnGun, view);
        if (r.handRound) {
            float inverse[12];
            InvertAny(r.roundInGlove, inverse);
            Compose(view, inverse, f.hand[0].glove);
        } else {
            m320::GloveFor(m320::Hold::kPinch, 0, view, f.hand[0].glove);
        }
        float middle[3];
        Transform(f.hand[0].glove, kMid, middle);
        Transform(f.headPose, middle, f.hand[0].gripRoom);
    }
    // The left hand moved by this much along a way on the gun (its turn kept).
    void MoveHandOnGun(const float* wayOnGun, float metres) {
        float way[3];
        Turn(f.gunPose, wayOnGun, way);
        float* g = f.hand[0].glove;
        g[3] += way[0] * metres, g[7] += way[1] * metres, g[11] += way[2] * metres;
        float middle[3];
        Transform(g, kMid, middle);
        Transform(f.headPose, middle, f.hand[0].gripRoom);
    }
    // A round in the hand (taken first) brought in line with the open barrel, its nose 1 cm short of the back end, then pushed in along the line at speed until
    // it's home: the seconds from going onto the line to home (-1: never home within most).
    float InsertByHand(float speed = 0.3f, float most = 1.5f) {
        float mouth[3], inward[3], nose[3], pose[12];
        m320::MouthAt(r.barrel, mouth);
        ExitOnGun(r.barrel, inward);
        for (float& v : inward)
            v = -v;
        for (int k = 0; k < 3; ++k)
            nose[k] = mouth[k] - inward[k] * 0.01f;
        RoundPose(inward, nose, pose);
        HoldRoundAt(pose);
        Step();
        float enteredAt = r.entered ? time : -1.0f;
        const int frames = static_cast<int>(most / f.dt + 0.5f);
        for (int i = 0; i < frames; ++i) {
            MoveHandOnGun(inward, speed * f.dt);
            Step();
            if (r.entered)
                enteredAt = time;
            if (r.seated || r.caseSeated)
                return enteredAt >= 0.0f ? time - enteredAt : -1.0f;
        }
        return -1.0f;
    }
    // The hand on the rim drawn all the way back out along the barrel (it's free with its nose out), 1 mm a frame.
    void PullOut(float most = 0.20f) {
        float exit[3];
        ExitOnGun(r.barrel, exit);
        const int steps = static_cast<int>(most / 0.001f + 0.5f);
        for (int i = 0; i < steps && !r.freed; ++i) {
            MoveHandOnGun(exit, 0.001f);
            Step();
        }
    }
};

// A round's bone on the gun with its axis (base to nose) along axis, its nose at nose.
void RoundPose(const float* axis, const float* nose, float* out) {
    float z[3] = {axis[0], axis[1], axis[2]};
    const float n = Length(z);
    for (float& v : z)
        v /= n;
    float y[3] = {0.0f, 1.0f, 0.0f};
    if (std::fabs(Dot(y, z)) > 0.95f)
        y[0] = 1.0f, y[1] = 0.0f;
    float x[3];
    Cross(y, z, x);
    const float nx = Length(x);
    for (float& v : x)
        v /= nx;
    Cross(z, x, y);
    const float m[12] = {x[0], y[0], z[0], 0, x[1], y[1], z[1], 0, x[2], y[2], z[2], 0};
    std::memcpy(out, m, sizeof(m));
    float at[3];
    Transform(out, m320::kRoundNoseOnBone, at);
    for (int i = 0; i < 3; ++i)
        out[i * 4 + 3] = nose[i] - at[i];
}

float ClearTime(float down) {
    return std::sqrt(2.0f * m320::kClear / (kG * (std::sin(down) - kMu * std::cos(down))));
}

bool Finite12(const float* m) {
    for (int i = 0; i < 12; ++i)
        if (!std::isfinite(m[i]))
            return false;
    return true;
}

// Frames alternately 13.9 and 27.8 ms (72 Hz with every other frame dropped).
float Uneven(int frame) {
    return frame % 2 == 0 ? 0.0139f : 0.0278f;
}

// The round's mesh drawn back 1 mm a frame along the open barrel's way out, by the hand on its rim.
void DrawAlong(Sim& s, float metres) {
    float exit[3];
    ExitOnGun(m320::kOpenMost, exit);
    const int steps = static_cast<int>(metres / 0.001f + 0.5f);
    for (int i = 0; i < steps && !s.r.freed; ++i) {
        float at[3];
        HandOnGun(s.f, 0, at);
        for (int k = 0; k < 3; ++k)
            at[k] += exit[k] * 0.001f;
        PutHandOnGun(s.f, 0, at);
        s.Step();
    }
}

// The hand's middle where its glove is drawn this frame (gloveOnGun at the full blend), on the gun.
void DrawnMiddle(const m320::Result& r, int side, float* onGun) {
    const float mid[3] = {side == 1 ? -kMid[0] : kMid[0], kMid[1], kMid[2]};
    Transform(r.gloveOnGun, mid, onGun);
}

// The hand beside the barrel's left side, 3 cm out from where it's taken, with the barrel at this angle.
void HandBesideBarrel(Sim& s, float degrees) {
    const float beside[3] = {m320::kBarrelGrab[0] + 0.03f, m320::kBarrelGrab[1], m320::kBarrelGrab[2]};
    float p[3];
    OnTube(degrees * kDegree, beside, p);
    PutHandOnGun(s.f, 0, p);
}

// ---- Either hand (the right's holds are the left's flipped: rows 16, 32, 36-44) ----
// A 3x4 seen in a mirror across x (both its spaces): the right glove's from the left's, as the module flips its holds.
void Mirror(const float* m, float* out) {
    float r[12];
    std::memcpy(r, m, sizeof(r));
    r[1] = -r[1], r[2] = -r[2], r[3] = -r[3], r[4] = -r[4], r[8] = -r[8];
    std::memcpy(out, r, sizeof(r));
}

// The most two sets of numbers differ by (infinite if any difference isn't finite).
float Differ(const float* a, const float* b, int n = 12) {
    float most = 0.0f;
    for (int i = 0; i < n; ++i) {
        const float d = std::fabs(a[i] - b[i]);
        if (!std::isfinite(d))
            return std::numeric_limits<float>::infinity();
        most = d > most ? d : most;
    }
    return most;
}

// The hand's middle in this side's glove (the right's the left's with x negated).
void MiddleOf(int side, float* out) {
    out[0] = side == 1 ? -kMid[0] : kMid[0], out[1] = kMid[1], out[2] = kMid[2];
}

// Either hand moved by this much along a way on the gun, its turn kept (Sim::MoveHandOnGun's for either side).
void MoveHandBy(m320::Frame& f, int side, const float* wayOnGun, float metres) {
    float way[3], mid[3], middle[3];
    Turn(f.gunPose, wayOnGun, way);
    float* g = f.hand[side].glove;
    g[3] += way[0] * metres, g[7] += way[1] * metres, g[11] += way[2] * metres;
    MiddleOf(side, mid);
    Transform(g, mid, middle);
    Transform(f.headPose, middle, f.hand[side].gripRoom);
}

// Either hand's middle where its glove pinches the rim, the round slid this far out (no palms given: the game's pinch, the right's mirrored on the round's bone).
void HandOnRimBy(Sim& s, int side, float along) {
    float round[12], onRound[3], target[3];
    m320::RoundOnLine(s.r.barrel, along, round);
    m320::PinchTarget(onRound);
    if (side == 1)
        onRound[0] = -onRound[0];
    Transform(round, onRound, target);
    PutHandOnGun(s.f, side, target);
}

// The glove put where it is (the view's space), its grip point at its middle.
void SetGlove(m320::Frame& f, int side, const float* glove) {
    float mid[3], middle[3];
    std::memcpy(f.hand[side].glove, glove, 12 * sizeof(float));
    MiddleOf(side, mid);
    Transform(glove, mid, middle);
    Transform(f.headPose, middle, f.hand[side].gripRoom);
}

// A palm off the table's own (as hands.cpp's, drawn at its glove's own width, is off the game's): turned this many degrees about the unit axis and moved by offset.
void PalmOff(const float* table, const float* axis, float degrees, const float* offset, float* out) {
    float turn[12];
    Rotation(axis, degrees * kDegree, turn);
    turn[3] = offset[0], turn[7] = offset[1], turn[11] = offset[2];
    Compose(table, turn, out);
}

}  // namespace

int main() {
    std::printf("m320test: the grenade launcher's reload by hand, on scripted frames\n");

    // (Info: the game's palm, bone 1, against its bone 0 in each hold's table: hands.cpp re-anchors the tables on the palm, as the arm is solved from it. Taken as they
    // came onto a glove at the controller, a round carried from the belly turned the palm by this and the arm went off: the user, 2026-11-05.)
    {
        Sim p;
        p.OpenEmpty();
        p.TakeAtBelly();
        if (p.r.took && p.r.fingers) {
            const float* m = p.r.fingers[1];
            const float scale = std::sqrt(m[0] * m[0] + m[4] * m[4] + m[8] * m[8]);
            const float trace = (m[0] + m[5] + m[10]) / (scale > 1e-6f ? scale : 1.0f);
            const float angle = std::acos(std::fmax(-1.0f, std::fmin(1.0f, (trace - 1.0f) / 2.0f))) * 57.29578f;
            std::printf("info the pinch's palm against the game's bone 0: turned %.1f degrees, %.1f cm off (scale %.3f)\n", angle,
                        100.0f * std::sqrt(m[3] * m[3] + m[7] * m[7] + m[11] * m[11]), scale);
        } else {
            std::printf("info no round taken for the palm check (%s)\n", p.r.note);
        }
    }

    // ---- 1. The spring open: the game's own swing, its stop at 0.25 s ----
    {
        const float vanilla[16] = {0.1f, 0.5f, 2.0f, 4.4f, 5.7f, 7.4f, 8.4f, 10.0f, 11.4f, 12.5f, 13.4f, 14.0f, 14.5f, 14.7f, 14.9f, 15.0f};  // frames 7312-7327
        Sim s;
        s.f.dt = 1.0f / 60.0f;
        m320::Clear();
        s.Step();
        s.Fire();
        s.OnGripB();
        s.Step();
        s.f.releasePress = true;
        s.f.hand[0].trigger = 1.0f;
        s.Step();  // the press: frame 0
        const bool pressed = s.r.released && s.r.indexOnRelease == 1.0f;
        const float lift = s.r.releaseLift, blade = s.r.trigger;
        float worst = 0.0f;
        int worstAt = 0, stopAt = -1;
        float angles[16];
        for (int k = 0; k < 16; ++k) {
            if (k > 0)
                s.Step();
            angles[k] = s.r.barrel / kDegree;
            const float off = std::fabs(angles[k] - vanilla[k]);
            if (off > worst)
                worst = off, worstAt = k;
            if (s.r.openStop && stopAt < 0)
                stopAt = k;
        }
        s.f.hand[0].trigger = 0.0f;
        s.Run(1.0f);
        Check(pressed && lift > 0.0039f && blade == 0.0f, "1 spring: the release pressed by the pull on the grip (released, the index on the release, lifted %.1f mm, the trigger blade %.2f)", lift * 1000.0f, blade);
        Check(worst <= 0.2f, "1 spring: the barrel on the game's own swing to within %.2f degrees at each of its 16 frames (worst frame %d: %.2f against %.1f); at 4 frames %.1f %.1f %.1f %.1f",
              worst, worstAt, angles[worstAt], vanilla[worstAt], angles[4], angles[8], angles[12], angles[15]);
        Check(stopAt == 15 && s.n.openStop == 1, "1 spring: at its stop at %.3f s (the game's 0.250), open_stop %d time(s), open_latch %d", stopAt / 60.0f, s.n.openStop, s.n.released);
        Check(s.r.indexOnRelease == 0.0f && s.r.barrelState == m320::Barrel::kOpen, "1 spring: the trigger let go, the index back on the trigger (%.0f), the barrel open at %.1f degrees",
              s.r.indexOnRelease, s.r.barrel / kDegree);
        // The same at 72 Hz: the stop within a frame of 0.25 s.
        Sim t;
        m320::Clear();
        t.Step();
        t.Fire();
        t.OnGripB();
        t.Step();
        t.Release();
        const float stop = t.RunUntil([](const m320::Result& x) { return x.openStop; }, 1.0f);
        Check(stop >= 0.25f - 0.001f && stop <= 0.25f + kDt + 0.001f, "1 spring: at 72 Hz at its stop %.3f s after the press", stop);
    }

    // ---- 2. The snap shut: let go short of it, it springs back; past it, quadratic, the bounce, the latch; blocked by the case half out ----
    {
        Sim s;
        s.Open();
        const float beside[3] = {m320::kBarrelGrab[0] + 0.03f, m320::kBarrelGrab[1], m320::kBarrelGrab[2]};  // 3 cm out from the barrel's left side
        auto handAt = [&](float degrees) {
            float p[3];
            OnTube(degrees * kDegree, beside, p);
            PutHandOnGun(s.f, 0, p);
        };
        handAt(15.0f);
        s.Squeeze();
        const bool taken = s.r.barrelTaken && s.r.handState == m320::HandState::kBarrel;
        float follow = 0.0f;
        for (float d = 14.0f; d >= 7.0f; d -= 1.0f) {
            handAt(d);
            s.Step();
            follow = std::fmax(follow, std::fabs(s.r.barrel / kDegree - d));
        }
        s.LetGo();
        const bool back = s.r.sprungBack && s.r.barrelState == m320::Barrel::kSpringing;
        const float reopen = s.RunUntil([](const m320::Result& x) { return x.openStop; }, 1.0f);
        Check(taken && follow < 0.01f, "2 snap: the barrel's back end taken by the hand, the barrel with the hand to within %.3f degrees", follow);
        Check(back && reopen > 0.0f && s.r.barrel == m320::kOpenMost, "2 snap: let go of at 7 degrees, its springs push it back open (at the stop %.3f s later)", reopen);
        // Past the snap: from 6.5 to 5.9 degrees in a frame.
        handAt(15.0f);
        s.Squeeze();
        for (float d = 14.0f; d >= 6.5f; d -= 0.5f) {
            handAt(d);
            s.Step();
        }
        handAt(5.9f);
        s.Step();
        const bool snapping = s.r.barrelState == m320::Barrel::kSnapping && s.r.handState == m320::HandState::kFree;
        const float from = s.r.barrel / kDegree;
        const Counts before = s.n;
        float hitAt = -1.0f, latchAt = -1.0f, least = 0.0f, t = 0.0f;
        for (int i = 0; i < 40; ++i) {
            s.Step();
            t += s.f.dt;
            least = std::fmin(least, s.r.barrel / kDegree);
            if (s.r.shut && hitAt < 0.0f)
                hitAt = t;
            if (s.r.latchedNow && latchAt < 0.0f)
                latchAt = t;
        }
        const float expected = 0.093f * std::sqrt(5.9f / 15.0f), overshoot = -0.7f * 5.9f / 15.0f;
        Check(snapping && std::fabs(from - 5.9f) < 0.01f, "2 snap: past 60%% of the way shut (%.1f degrees) it snaps from the hand", from);
        Check(expected <= 0.06f && hitAt >= expected - 0.001f && hitAt <= expected + kDt + 0.001f,
              "2 snap: shut %.4f s after (0.093 x sqrt(5.9/15) = %.4f, within the 0.06 s; seen at the next frame)", hitAt, expected);
        Check(std::fabs(least - overshoot) <= 0.02f, "2 snap: past shut by %.3f degrees (-0.7 x 5.9/15 = %.3f)", least, overshoot);
        Check(latchAt >= expected + 0.09f - 0.001f && latchAt <= expected + 0.09f + kDt + 0.001f && s.n.shut - before.shut == 1 && s.n.latchedNow - before.latchedNow == 1,
              "2 snap: latched %.3f s after it hit shut (0.09: seen at %.4f s, the hit at %.4f), close %d time(s), latched %d time(s)", latchAt - expected, latchAt, expected,
              s.n.shut - before.shut, s.n.latchedNow - before.latchedNow);
        Check(s.r.latched && s.r.barrel == 0.0f && s.r.content == m320::Content::kSeated && !s.r.roundLive,
              "2 snap: shut with the spent case still in it (%s)", s.r.state);
        // Blocked: the case drawn 6 mm out and left there, the barrel can't shut past 10 degrees.
        Sim b;
        b.Open();
        b.HandOnRim(0.0f);
        b.Squeeze();
        float exit[3];
        ExitOnGun(m320::kOpenMost, exit);
        for (int i = 0; i < 6; ++i) {
            float at[3];
            HandOnGun(b.f, 0, at);
            for (int k = 0; k < 3; ++k)
                at[k] += exit[k] * 0.001f;
            PutHandOnGun(b.f, 0, at);
            b.Step();
        }
        b.LetGo();
        const float out = b.r.roundS;
        auto bHandAt = [&](float degrees) {
            float p[3];
            OnTube(degrees * kDegree, beside, p);
            PutHandOnGun(b.f, 0, p);
        };
        bHandAt(15.0f);
        b.Squeeze();
        for (float d = 14.0f; d >= 3.0f; d -= 1.0f) {
            bHandAt(d);
            b.Step();
        }
        Check(std::fabs(out - 0.006f) < 0.0005f && std::fabs(b.r.barrel / kDegree - 10.0f) < 0.01f && b.n.blocked == 1 && b.r.content == m320::Content::kSliding,
              "2 snap: the case %.1f mm out, the hand swinging it to 3 degrees: it stops at %.2f (\"the case is in the way\", told %d time(s))", out * 1000.0f,
              b.r.barrel / kDegree, b.n.blocked);
    }

    // ---- 3. Gravity: the case slides out of the open barrel with the muzzle up (30 degrees and more down its way out), friction 0.3 ----
    {
        const float downs[5] = {25.0f, 30.05f, 45.0f, 60.0f, 90.0f};
        const float design[5] = {-1.0f, 0.34f, 0.24f, 0.20f, 0.17f};
        for (int k = 0; k < 5; ++k) {
            Sim s;
            s.Open();
            TiltExit(s.f, downs[k] * kDegree);
            const Counts before = s.n;
            const float took = s.RunUntil([](const m320::Result& x) { return x.drop; }, 2.0f);
            if (k == 0) {
                Check(took < 0.0f && s.r.roundS == 0.0f && s.r.content == m320::Content::kSeated, "3 gravity: its way out 25 degrees down: it stays in (%.3f m out after 2 s)",
                      s.r.roundS);
                continue;
            }
            const float expected = ClearTime(downs[k] * kDegree);
            const m320::Result& r = s.r;
            float exitGun[3], exitRoom[3];
            ExitOnGun(m320::kOpenMost, exitGun);
            Turn(s.f.gunRoom, exitGun, exitRoom);
            const float speed = Length(r.dropVelocityRoom), along = Dot(r.dropVelocityRoom, exitRoom);
            const float slid = kG * (std::sin(downs[k] * kDegree) - kMu * std::cos(downs[k] * kDegree)) * took;
            Check(took > 0.0f && std::fabs(took - expected) <= 0.02f && std::fabs(took - design[k]) <= 0.02f,
                  "3 gravity: %.0f degrees down: out in %.3f s (g(sin - 0.3 cos) gives %.3f; the design's %.2f)", downs[k], took, expected, design[k]);
            Check(r.dropFromBarrel && !r.dropLive && !r.unload && s.n.contentMoving - before.contentMoving == 1 && r.round == m320::Round::kHidden &&
                      std::fabs(speed - slid) < 0.05f * slid + 0.02f && along > 0.99f * speed,
                  "3 gravity: %.0f degrees: it falls out along the barrel at %.2f m/s (slid %.2f), spent, case_out once, nothing left in it", downs[k], speed, slid);
        }
        Sim s;
        m320::Clear();
        s.Step();
        s.Fire();
        TiltExit(s.f, 90.0f * kDegree);
        s.Run(2.0f);
        Check(s.n.drop == 0 && s.r.roundS == 0.0f && s.r.latched, "3 gravity: shut, the muzzle straight up for 2 s: nothing moves (%.3f m)", s.r.roundS);
    }

    // ---- 4. The jab: the gun thrust along its length, 25 m/s/s in 2 frames of 3, throws the case back ----
    {
        auto jab = [](float accel, bool shut, float& out, int& jabs) {
            Sim s;
            if (shut) {
                m320::Clear();
                s.Step();
                s.Fire();
            } else {
                s.Open();
            }
            s.Run(0.2f);
            float muzzle[3] = {s.f.gunRoom[2], s.f.gunRoom[6], s.f.gunRoom[10]};
            float room[12];
            std::memcpy(room, s.f.gunRoom, sizeof(room));
            float v = 0.0f;
            for (int i = 0; i < 40; ++i) {
                const float a = i < 3 ? accel : 0.0f;  // three frames' thrust (each frame's second difference exactly a), then on at the speed it got to
                v += a * s.f.dt;
                for (int k = 0; k < 3; ++k)
                    room[k * 4 + 3] += muzzle[k] * v * s.f.dt;
                SetGunRoom(s.f, room);
                s.Step();
            }
            out = s.r.roundS;
            jabs = s.n.jabbed;
        };
        float out25 = 0.0f, out20 = 0.0f, outShut = 0.0f;
        int jabs25 = 0, jabs20 = 0, jabsShut = 0;
        jab(25.2f, false, out25, jabs25);
        jab(20.0f, false, out20, jabs20);
        jab(25.2f, true, outShut, jabsShut);
        Check(jabs25 == 1 && out25 > 0.03f, "4 jab: 25 m/s/s along the gun for 3 frames: the case thrown back %.3f m (jabbed %d)", out25, jabs25);
        Check(jabs20 == 0 && out20 == 0.0f, "4 jab: 20 m/s/s: it stays (%.3f m, jabbed %d)", out20, jabs20);
        Check(jabsShut == 0 && outShut == 0.0f, "4 jab: 25 m/s/s with the barrel shut: nothing (%.3f m, jabbed %d)", outShut, jabsShut);
        // A tracking jump: the gun 5 cm off for a frame and back.
        Sim s;
        s.Open();
        s.Run(0.2f);
        float room[12];
        std::memcpy(room, s.f.gunRoom, sizeof(room));
        room[11] -= 0.05f;
        SetGunRoom(s.f, room);
        s.Step();
        room[11] += 0.05f;
        SetGunRoom(s.f, room);
        s.Run(0.5f);
        Check(s.n.jabbed == 0 && s.r.roundS == 0.0f, "4 jab: a 5 cm tracking jump of the gun and back: no jab (%.3f m)", s.r.roundS);
    }

    // ---- 5. The rim: behind the barrel's back end the hand takes the case, beside it the barrel; 2.9 cm drawn it stays, 13.0 cm it's still on the barrel's
    // line, drawn all the way out (13.4 cm, its nose out) it's free ----
    {
        Sim s;
        s.Open();
        s.HandOnRim(0.0f);
        float hand[3], local[3];
        HandOnGun(s.f, 0, hand);
        ToTube(s.r.barrel, hand, local);
        s.Squeeze();
        Check(s.r.pinched && s.r.handState == m320::HandState::kPinch && s.r.content == m320::Content::kPinched && s.r.spentSqueeze && s.r.holding,
              "5 rim: the hand behind the barrel's back end (%.3f along the tube, the back end at 0.495) takes the case by its rim", local[2]);
        // Drawn back along the barrel's way out. (One Sim at a time: the module is one, and each Sim's Open clears it.)
        auto draw = [](Sim& x, float metres) {
            float exit[3];
            ExitOnGun(m320::kOpenMost, exit);
            const int steps = static_cast<int>(metres / 0.001f + 0.5f);
            for (int i = 0; i < steps; ++i) {
                float at[3];
                HandOnGun(x.f, 0, at);
                const float step = std::fmin(0.001f, metres - 0.001f * static_cast<float>(i));
                for (int k = 0; k < 3; ++k)
                    at[k] += exit[k] * step;
                PutHandOnGun(x.f, 0, at);
                x.Step();
                if (x.r.freed)
                    return;
            }
        };
        draw(s, 0.029f);
        const float drawn = s.r.roundS;
        s.LetGo();
        s.Run(1.0f);
        Check(std::fabs(drawn - 0.029f) < 0.0005f && std::fabs(s.r.roundS - 0.029f) < 0.0005f && s.r.content == m320::Content::kSliding && s.n.freed == 0,
              "5 rim: drawn %.1f mm and let go of, the gun level: it stays %.1f mm out a second later", drawn * 1000.0f, s.r.roundS * 1000.0f);
        Sim b;
        b.Open();
        const float beside[3] = {m320::kBarrelGrab[0] + 0.03f, m320::kBarrelGrab[1], m320::kBarrelGrab[2]};
        float p[3];
        OnTube(m320::kOpenMost, beside, p);
        PutHandOnGun(b.f, 0, p);
        b.Squeeze();
        Check(b.r.barrelTaken && !b.r.pinched && b.r.handState == m320::HandState::kBarrel, "5 rim: beside the barrel the same squeeze takes the barrel, not the case");
        Sim c;
        c.Open();
        c.HandOnRim(0.0f);
        c.Squeeze();
        c.Run(0.2f);  // the glove fully on the rim
        draw(c, 0.130f);
        const bool stillOn = !c.r.freed && c.n.freed == 0 && c.r.handState == m320::HandState::kPinch && std::fabs(c.r.roundS - 0.130f) < 0.0005f;
        Check(stillOn, "5 rim: drawn 13.0 cm it's still on the barrel's line, in the hand on its rim (%.1f mm out)", c.r.roundS * 1000.0f);
        draw(c, m320::kClear - 0.130f + 0.0005f);
        const bool freed = c.r.freed && c.r.handState == m320::HandState::kRound && c.r.content == m320::Content::kNone && c.r.handRound && !c.r.handRoundLive;
        float glide = -1.0f;
        if (freed) {
            float t = 0.0f;
            for (int i = 0; i < 40 && glide < 0.0f; ++i) {
                c.Step();
                t += c.f.dt;
                if (c.r.gloveBlend == 0.0f)
                    glide = t;
            }
        }
        Check(freed && glide > 0.0f && glide <= 0.15f + 0.001f && c.n.unload == 0 && c.n.contentMoving == 1,
              "5 rim: drawn all the way out (its nose out) it's free in the hand, the glove gliding back to the real hand in %.3f s (spent: no unload), case_out once", glide);
        // The user's video, 2026-11-05: the case pulled free was caught again 40 ms later and pushed back in, a dozen times. Held there, at the back end and in
        // line, pushed back in: a spent case never goes back on the line.
        {
            float mouth[3], inward[3], nose[3], pose[12];
            m320::MouthAt(c.r.barrel, mouth);
            ExitOnGun(c.r.barrel, inward);
            for (float& v : inward)
                v = -v;
            for (int k = 0; k < 3; ++k)
                nose[k] = mouth[k] + inward[k] * 0.02f;
            RoundPose(inward, nose, pose);
            c.HoldRoundAt(pose);
            for (int i = 0; i < 30; ++i) {
                c.MoveHandOnGun(inward, 0.002f);
                c.Step();
            }
        }
        Check(c.n.caught == 0 && c.n.caseSeated == 0 && c.r.handState == m320::HandState::kRound && c.r.content == m320::Content::kNone,
              "5 rim: the case pulled out, put to the back end in line and pushed in: never back on the line (caught %d, pushed in %d)", c.n.caught, c.n.caseSeated);
        c.LetGo();
        Check(c.r.drop && c.r.letGo && !c.r.dropLive && !c.r.dropFromBarrel && c.r.handState == m320::HandState::kFree, "5 rim: let go of, the case falls from the hand");
    }

    // ---- 6. The belly gives what's open: the launcher's barrel, a 40mm; the rifle's magazine, a magazine; both, whichever was opened last ----
    {
        Sim a;
        a.Open();
        a.TakeAtBelly();
        Check(a.r.took && a.r.spentSqueeze && a.r.handRound && a.r.handRoundLive && a.r.bellyForM320, "6 belly: the launcher open, the rifle's magazine in: a 40mm");
        Sim b;
        m320::Clear();
        b.Step();
        b.Fire();
        b.f.rifleOpen = true;
        b.Step();
        b.TakeAtBelly();
        Check(!b.r.took && !b.r.spentSqueeze && !b.r.bellyForM320 && !b.r.wantsRound, "6 belly: the launcher shut, the rifle's magazine out: nothing from the launcher (the pouch's)");
        Sim c;
        m320::Clear();
        c.Step();
        c.Fire();
        c.f.rifleOpen = true;
        c.Run(0.2f);
        c.OnGripB();
        c.Step();
        c.Release();
        c.Run(0.3f);
        c.OffGrip();
        c.TakeAtBelly();
        Check(c.r.took && c.r.bellyForM320, "6 belly: both open, the launcher's opened last: a 40mm");
        Sim d;
        d.Open();
        d.f.rifleOpen = true;
        d.Step();
        d.TakeAtBelly();
        Check(!d.r.took && !d.r.spentSqueeze && !d.r.bellyForM320, "6 belly: both open, the rifle's magazine out last: nothing from the launcher");
        // A live round seated in the open barrel: in, not shut.
        Sim e;
        e.OpenEmpty();
        e.TakeAtBelly();
        e.InsertByHand();
        e.LetGo();
        e.Run(0.2f);
        e.TakeAtBelly();
        Check(!e.r.took && e.r.content == m320::Content::kSeated && e.r.roundLive && e.loaded == 1, "6 belly: the barrel open with a live round in it: nothing (\"%s\")", e.r.note);
        // The hand at the gun, not the belly: the belly counted as reached (a test's), 9 cm from the handguard's line nothing, 11 cm a round.
        auto nearGuard = [](float above) {
            Sim x;
            x.Open();
            const float at[3] = {0.0f, 0.045f + above, 0.57f};
            PutHandOnGun(x.f, 0, at);
            x.f.hand[0].forceBelly = true;
            x.Squeeze();
            return x.r.took;
        };
        const bool at9 = nearGuard(0.09f), at11 = nearGuard(0.11f);
        Check(!at9 && at11, "6 belly: the hand 9 cm from the handguard's line takes nothing, 11 cm a round");
        // Measured at the controller's grip point, as pouch.cpp does: the glove's middle doesn't count.
        Sim g;
        g.Open();
        float belly[3], room[3];
        BellyInView(g.f, belly);
        Transform(g.f.headPose, belly, room);
        g.Squeeze();  // (the hand away: nothing)
        g.LetGo();
        std::memcpy(g.f.hand[0].gripRoom, room, sizeof(room));  // the grip point at the belly, the glove's middle still away
        g.Squeeze();
        const bool byGrip = g.r.took;
        Sim h;
        h.Open();
        PutHandInView(h.f, 0, belly);
        h.f.hand[0].gripRoom[1] += 0.30f;  // the glove's middle at the belly, the grip point 30 cm above it
        h.Squeeze();
        Check(byGrip && !h.r.took, "6 belly: the zone by the controller's grip point only (ammozone::Inside: %.2f of its size at the belly, %.2f 30 cm up)",
              ammozone::Size(ammozone::kBelly), [] {
                  const float up[3] = {ammozone::kBelly[0], ammozone::kBelly[1] + 0.30f, ammozone::kBelly[2]};
                  return ammozone::Size(up);
              }());
        // The squeeze first, the hand there within 0.4 s: taken; at 0.45 s, not.
        auto late = [](float seconds) {
            Sim x;
            x.Open();
            x.Squeeze();  // away from everything
            x.Run(seconds);
            float bellyView[3];
            BellyInView(x.f, bellyView);
            PutHandInView(x.f, 0, bellyView);
            x.Step();
            return x.r.took;
        };
        const bool soon = late(0.30f), tooLate = late(0.45f);
        Check(soon && !tooLate, "6 belly: squeezed on the way, at the belly 0.30 s later a round, 0.45 s later nothing (the 0.4 s window)");
    }

    // ---- 7. Put in by hand (the user, 2026-11-05: the magnet was "too snappy ... too magnetic. I feel like it did it on its own"): a live round's nose at the
    // open barrel's back end, in line and going in, goes onto the barrel's line, then moves only as the hand pushes it; its last 2 cm home with a click ----
    {
        // Pushed in at 0.3 m/s from 1 cm short of the back end: on, carried in by the hand, home, counted once.
        Sim a;
        a.OpenEmpty();
        a.TakeAtBelly();
        a.Run(0.1f);
        const float took = a.InsertByHand(0.3f);
        a.Run(0.3f);
        Check(a.n.caught == 1 && a.n.seated == 1 && a.loaded == 1 && a.spare == 2,
              "7 by hand: pushed in at 0.3 m/s it went on and home (%.3f s, the hand's own 11 cm at 0.3 m/s and the last 2 cm in 0.08 s), counted once: loaded %d, %d spare",
              took, a.loaded, a.spare);
        Check(took > 0.30f && took < 0.50f, "7 by hand: home %.3f s after it went on (the push, not by itself)", took);
        Check(a.r.round == m320::Round::kGame && a.r.roundOut == 0.0f && a.r.roundLive && !a.r.handRound && a.r.handState == m320::HandState::kFree,
              "7 by hand: in its place on the barrel's line, live, nothing in the hand");
        // It moves only as the hand does: on, pushed 5 cm, then held still a second: 5 cm further in, and there it stays.
        Sim b;
        b.OpenEmpty();
        b.TakeAtBelly();
        b.Run(0.1f);
        float mouth[3], inward[3], nose[3], pose[12];
        m320::MouthAt(b.r.barrel, mouth);
        ExitOnGun(b.r.barrel, inward);
        for (float& v : inward)
            v = -v;
        for (int k = 0; k < 3; ++k)
            nose[k] = mouth[k] + inward[k] * 0.015f;
        RoundPose(inward, nose, pose);
        b.HoldRoundAt(pose);
        b.Step();
        const bool on = b.r.entered && b.r.content == m320::Content::kPinched && b.r.handState == m320::HandState::kPinch;
        const float onAt = b.r.roundS;
        for (int i = 0; i < 25; ++i) {
            b.MoveHandOnGun(inward, 0.002f);
            b.Step();
        }
        const float pushed = b.r.roundS;
        b.Run(1.0f);
        Check(on && std::fabs(onAt - (m320::kClear - 0.015f)) < 0.0005f && std::fabs(onAt - pushed - 0.05f) < 0.002f && std::fabs(b.r.roundS - pushed) < 1e-5f &&
                  b.n.seated == 0,
              "7 by hand: its nose 1.5 cm in, held still: on the line at %.1f mm out; pushed 5 cm: %.1f mm further in; held still a second: it stays (%.1f mm), not home",
              onAt * 1000.0f, (onAt - pushed) * 1000.0f, b.r.roundS * 1000.0f);
        // Pulled back out before it's home: in the hand again, nothing counted either way.
        b.PullOut();
        Check(b.r.freed && b.r.handRound && b.r.handRoundLive && b.n.unload == 0 && b.loaded == 0 && b.spare == 3,
              "7 by hand: pulled back out before home: in the hand again, never counted (unload %d, loaded %d, %d spare)", b.n.unload, b.loaded, b.spare);
        // Not on: its nose 3 cm off the line, turned 40 degrees, held still 1 cm short, base first; and a round just pulled out pushed straight back (it must be
        // out of the way first), then 7 cm off and back: on.
        auto tryOn = [](float off, float turnedDeg, float deep, bool push, bool baseFirst) {
            Sim s;
            s.OpenEmpty();
            s.TakeAtBelly();
            s.Run(0.1f);
            float m[3], in[3], n[3], p[12], axis[3];
            m320::MouthAt(s.r.barrel, m);
            ExitOnGun(s.r.barrel, in);
            for (float& v : in)
                v = -v;
            const float c = std::cos(turnedDeg * kDegree), sn = std::sin(turnedDeg * kDegree);
            axis[0] = in[0] * c + in[2] * sn, axis[1] = 0.0f, axis[2] = -in[0] * sn + in[2] * c;
            if (baseFirst)
                for (float& v : axis)
                    v = -v;
            for (int k = 0; k < 3; ++k)
                n[k] = m[k] + in[k] * deep;
            n[1] += off;
            if (baseFirst)  // (the base where the nose would be)
                for (int k = 0; k < 3; ++k)
                    n[k] -= axis[k] * 0.119f;
            RoundPose(axis, n, p);
            if (!push) {  // held still: brought there from above, 6 cm across the line (no push in it at all)
                float side[12];
                std::memcpy(side, p, sizeof(side));
                side[7] += 0.06f;
                s.HoldRoundAt(side);
                s.Step();
                const float across[3] = {0.0f, -1.0f, 0.0f};  // (straight down onto it: square to its line)
                for (int i = 0; i < 12; ++i) {
                    s.MoveHandOnGun(across, 0.005f);
                    s.Step();
                }
            } else {
                s.HoldRoundAt(p);
                s.Step();
            }
            for (int i = 0; i < 10 && !s.r.entered; ++i) {
                if (push)
                    s.MoveHandOnGun(in, 0.003f);
                s.Step();
            }
            return s.n.caught + (s.r.handState == m320::HandState::kPinch ? 1 : 0) + (s.r.content != m320::Content::kNone ? 1 : 0);
        };
        // (The user, 22:20: "now it's like harder to insert it": the window 4 cm and 45 degrees, its nose at the back end or in going on even held still.)
        const int offLine = tryOn(0.05f, 0.0f, -0.005f, true, false), turned = tryOn(0.0f, 50.0f, -0.005f, true, false),
                  still = tryOn(0.0f, 0.0f, -0.01f, false, false), base = tryOn(0.0f, 0.0f, -0.005f, true, true),
                  near = tryOn(0.035f, 40.0f, -0.005f, true, false), stillIn = tryOn(0.0f, 0.0f, 0.005f, false, false);
        Check(offLine == 0 && turned == 0 && still == 0 && base == 0 && near > 0 && stillIn > 0,
              "7 by hand: not on with its nose 5 cm off the line, turned 50 degrees, held still 1 cm short, base first (%d %d %d %d); on 3.5 cm off and 40 degrees (%d), and "
              "held still with its nose 5 mm in (%d)",
              offLine, turned, still, base, near, stillIn);
        Sim c;
        c.OpenEmpty();
        c.TakeAtBelly();
        c.Run(0.1f);
        c.InsertByHand(0.3f, 0.2f);  // on and part way in
        c.PullOut();
        const bool outAgain = c.r.freed && c.r.handRound;
        float m[3], in[3], n[3], p[12];
        m320::MouthAt(c.r.barrel, m);
        ExitOnGun(c.r.barrel, in);
        for (float& v : in)
            v = -v;
        for (int k = 0; k < 3; ++k)
            n[k] = m[k] - in[k] * 0.005f;
        RoundPose(in, n, p);
        c.HoldRoundAt(p);
        c.Step();
        for (int i = 0; i < 10; ++i) {
            c.MoveHandOnGun(in, 0.003f);
            c.Step();
        }
        const bool straightBack = c.r.handState == m320::HandState::kPinch;
        for (int k = 0; k < 3; ++k)
            n[k] = m[k] - in[k] * 0.07f;
        RoundPose(in, n, p);
        c.HoldRoundAt(p);
        c.Step();
        const float again = c.InsertByHand(0.3f);
        Check(outAgain && !straightBack && again > 0.0f && c.loaded == 1 && c.spare == 2,
              "7 by hand: pulled out, pushed straight back in line: not on (it's out of the way first); 7 cm off and back: on and home (%.3f s): loaded %d, %d spare", again,
              c.loaded, c.spare);
        // Let go part way in with the muzzle down: it slides home itself and is counted then, once.
        Sim d;
        d.OpenEmpty();
        d.TakeAtBelly();
        d.Run(0.1f);
        d.InsertByHand(0.3f, 0.2f);
        const float partWay = d.r.roundS;
        d.LetGo();
        TiltExit(d.f, -60.0f * kDegree);
        const float home = d.RunUntil([](const m320::Result& x) { return x.seated; }, 1.0f);
        d.Run(0.3f);
        Check(partWay > 0.02f && home > 0.0f && d.n.seated == 1 && d.loaded == 1 && d.spare == 2 && d.r.content == m320::Content::kSeated,
              "7 by hand: let go of %.1f cm out, the muzzle down 60 degrees: home by itself in %.3f s, counted once: loaded %d, %d spare", partWay * 100.0f, home, d.loaded,
              d.spare);
    }

    // ---- 8. The count: one round taken from the spare ones per seat, one given back per live round out; drops change nothing ----
    {
        Sim s;
        s.loaded = 1, s.spare = 3;
        m320::Clear();
        s.Step();
        s.Fire();  // the game fired it
        const bool fired = s.n.fired == 1 && !s.r.fireReady;
        s.OnGripB();
        s.Step();
        s.Release();
        s.Run(0.3f);
        s.OffGrip();
        TiltExit(s.f, 60.0f * kDegree);
        s.RunUntil([](const m320::Result& x) { return x.drop; }, 1.0f);
        LevelGun(s.f);
        s.Step();
        s.TakeAtBelly();
        float mouth[3], inward[3], nose[3], pose[12];
        m320::MouthAt(m320::kOpenMost, mouth);
        ExitOnGun(m320::kOpenMost, inward);
        for (float& v : inward)
            v = -v;
        for (int k = 0; k < 3; ++k)
            nose[k] = mouth[k] - inward[k] * 0.04f;
        RoundPose(inward, nose, pose);
        s.InsertByHand();
        s.LetGo();
        s.Run(0.2f);
        const int seatedAfterOne = s.n.seated, spareAfterOne = s.spare;
        // Shut by a push on its left side.
        s.f.hand[0].touches = 1;
        float local[3] = {0.075f, -0.036f, 0.53f}, at[3];
        OnTube(s.r.barrel, local, at);
        for (int i = 0; i < 120 && !s.r.latched; ++i) {
            float view[3];
            Transform(s.f.gunPose, at, view);
            std::memcpy(s.f.hand[0].touch[0], view, sizeof(view));
            float handAt[3] = {at[0] + 0.04f, at[1], at[2]};
            PutHandOnGun(s.f, 0, handAt);
            s.Step();
            at[0] -= 0.3f * s.f.dt;
        }
        s.f.hand[0].touches = 0;
        PutHandOnGun(s.f, 0, kAway);
        s.Run(0.2f);
        Check(fired && seatedAfterOne == 1 && spareAfterOne == 2 && s.loaded == 1 && s.n.unload == 0 && s.r.fireReady && s.n.drop == 1,
              "8 count: fired, the case tipped out (a drop, nothing counted), a round from the belly seated (seated %d: loaded %d, %d spare), pushed shut: ready to fire", s.n.seated,
              s.loaded, s.spare);
        // A live round tipped out of an open barrel goes back with the spare ones, once.
        Sim t;
        t.OpenEmpty();
        t.TakeAtBelly();
        t.InsertByHand();
        t.LetGo();
        t.Run(0.2f);
        const int spareSeated = t.spare;
        TiltExit(t.f, 60.0f * kDegree);
        t.RunUntil([](const m320::Result& x) { return x.drop; }, 1.0f);
        const bool liveOut = t.r.drop && t.r.dropLive && t.n.unload == 1;
        t.Run(1.0f);
        Check(liveOut && t.n.unload == 1 && t.n.seated == 1 && t.loaded == 0 && t.spare == spareSeated + 1,
              "8 count: a live round in the open barrel tipped out: unload %d time(s), back with the spare ones (%d -> %d), loaded %d", t.n.unload, spareSeated, t.spare,
              t.loaded);
        // ... and one pulled out by its rim: unload once; let go of, it falls and nothing more is counted.
        Sim u;
        u.OpenEmpty();
        u.TakeAtBelly();
        u.InsertByHand();
        u.LetGo();
        u.Run(0.2f);
        const int spareIn = u.spare;
        u.HandOnRim(0.0f);
        u.Squeeze();
        u.Run(0.2f);
        float exit[3];
        ExitOnGun(m320::kOpenMost, exit);
        for (int i = 0; i < 200 && !u.r.freed; ++i) {
            float h[3];
            HandOnGun(u.f, 0, h);
            for (int k = 0; k < 3; ++k)
                h[k] += exit[k] * 0.001f;
            PutHandOnGun(u.f, 0, h);
            u.Step();
        }
        const bool pulled = u.r.freed && u.r.unload && u.r.handRoundLive;
        PutHandOnGun(u.f, 0, kAway);
        u.Step();
        u.LetGo();
        const bool fell = u.r.drop && u.r.dropLive;
        u.Run(1.0f);
        Check(pulled && fell && u.n.unload == 1 && u.spare == spareIn + 1 && u.loaded == 0, "8 count: a live round drawn out by its rim: unload %d time(s) (%d -> %d spare); dropped, nothing more",
              u.n.unload, spareIn, u.spare);
    }

    // ---- 9. When the left trigger fires: shut, latched, live; never springing, open or snapping; always when not in use ----
    {
        m320::Clear();
        const bool afterClear = m320::FireReady();
        Sim s;
        s.Step();
        const bool live = s.r.fireReady && m320::FireReady();
        s.Fire();
        const bool spent = !s.r.fireReady && !m320::FireReady();
        s.OnGripB();
        s.Step();
        s.Release();
        s.Step();
        const bool springing = s.r.barrelState == m320::Barrel::kSpringing && !m320::FireReady();
        s.Run(0.3f);
        const bool open = s.r.barrelState == m320::Barrel::kOpen && !m320::FireReady();
        s.f.active = false;
        s.Step();
        const bool unused = s.r.fireReady && m320::FireReady() && !s.r.drawn;
        s.f.active = true;
        s.Step();
        const bool backOpen = s.r.barrelState == m320::Barrel::kOpen && !m320::FireReady();
        // A flick shut: snapping, not ready until latched.
        s.OffGrip();
        float roll[12];
        const float axisZ[3] = {0.0f, 0.0f, 1.0f};
        Rotation(axisZ, -10.47f * s.f.dt, roll);
        for (int i = 0; i < 4; ++i) {
            TurnGunOwn(s.f, roll);
            s.Step();
        }
        bool snappingNotReady = true, sawSnap = false;
        for (int i = 0; i < 30; ++i) {
            s.Step();
            if (s.r.barrelState == m320::Barrel::kSnapping) {
                sawSnap = true;
                snappingNotReady = snappingNotReady && !m320::FireReady();
            }
        }
        const bool shutSpent = s.r.latched && !s.r.fireReady;
        Check(afterClear && live && spent && springing && open && unused && backOpen && sawSnap && snappingNotReady && shutSpent,
              "9 fire: ready after Clear %d, shut and live %d; not after the shot %d, springing %d, open %d, snapping %d; ready not in use %d; open again when back %d; shut on the spent case not %d",
              afterClear, live, spent, springing, open, sawSnap && snappingNotReady, unused, backOpen, shutSpent);
    }

    // ---- 10. The wrist's flick: a 600 degrees/s roll that stops opens it; slow, a swing, a roll that goes on, a tracking jump don't ----
    {
        const float z[3] = {0.0f, 0.0f, 1.0f}, y[3] = {0.0f, 1.0f, 0.0f};
        auto spun = [&](Sim& s, const float* axis, float rate, int frames) {
            float turn[12];
            Rotation(axis, rate * s.f.dt, turn);
            for (int i = 0; i < frames; ++i) {
                TurnGunOwn(s.f, turn);
                s.Step();
            }
        };
        auto spent = [](Sim& s) {
            m320::Clear();
            s.Step();
            s.Fire();
            s.Run(0.7f);
        };
        // The default (the user, 2026-11-06: "get rid of the flick to open"): the same snap opens nothing, spent or loaded; the release still opens a spent one.
        {
            Sim o;
            spent(o);
            spun(o, z, 10.47f, 4);
            o.Run(0.4f);
            Sim ol;
            m320::Clear();
            ol.Step();  // loaded, shut
            ol.Run(0.7f);
            spun(ol, z, 10.47f, 4);
            ol.Run(0.4f);
            Sim orel;
            spent(orel);
            orel.OnGripB();
            orel.Step();
            orel.Release();
            orel.OffGrip();
            orel.Run(0.4f);
            Check(o.n.flickOpen == 0 && o.r.latched && ol.n.flickOpen == 0 && ol.r.latched && ol.loaded == 1 && orel.n.released == 1 && !orel.r.latched,
                  "10 flick: by default it opens nothing (spent %d, loaded %d, still latched); the release opens a spent one (%d)", o.n.flickOpen, ol.n.flickOpen,
                  orel.n.released);
        }
        m320::SetFlickOpens(true);  // the detector itself, as it was
        Sim a;
        spent(a);
        spun(a, z, 10.47f, 4);  // 600 degrees/s for 50 ms: 33 degrees
        a.Run(0.4f);
        Check(a.n.flickOpen == 1 && a.n.released == 1 && !a.r.latched, "10 flick: rolled right at 600 degrees/s for 50 ms and stopped: open (flick %d)", a.n.flickOpen);
        Sim b;
        spent(b);
        spun(b, z, 5.24f, 6);
        b.Run(0.4f);
        Sim c;
        spent(c);
        spun(c, y, 10.47f, 4);
        c.Run(0.4f);
        Sim d;
        spent(d);
        spun(d, z, 10.47f, 72);
        d.Run(0.4f);
        Check(b.n.flickOpen == 0 && c.n.flickOpen == 0 && d.n.flickOpen == 0 && b.r.latched && c.r.latched && d.r.latched,
              "10 flick: 300 degrees/s (%d), 600 degrees/s about the vertical (%d), a second's roll at 600 (%d): none", b.n.flickOpen, c.n.flickOpen, d.n.flickOpen);
        Sim e;
        spent(e);
        {
            float turn[12];
            Rotation(z, 52.4f * e.f.dt, turn);  // 3000 degrees/s for a frame: 42 degrees
            float room[12];
            Compose(e.f.gunRoom, turn, room);
            room[3] += 0.05f;  // and 5 cm across: tracking
            SetGunRoom(e.f, room);
            e.Step();
            e.Run(0.4f);
        }
        Check(e.n.flickOpen == 0 && e.r.latched, "10 flick: 3000 degrees/s in a single frame with a 5 cm jump: none (%d)", e.n.flickOpen);
        Sim f;
        spent(f);
        spun(f, z, 10.47f, 4);
        f.Run(0.14f);
        spun(f, z, -10.47f, 4);  // and back
        f.Run(0.6f);
        Check(f.n.flickOpen == 1 && f.n.flickShut == 0 && !f.r.latched, "10 flick: snapped right then straight back: one event (open %d, shut %d)", f.n.flickOpen, f.n.flickShut);
        // Rolled left with it open, a while later: shut, and latched.
        f.Run(0.2f);
        spun(f, z, -10.47f, 4);
        const float latched = f.RunUntil([](const m320::Result& x) { return x.latchedNow; }, 0.5f);
        Check(f.n.flickShut == 1 && latched > 0.0f && f.r.latched, "10 flick: open, a snap to the left: shut and latched %.3f s later", latched);
        // Loaded and shut, the same snap opens it too (the user, 2026-11-05: a way to open it "without shooting the launcher": "Flick only"): the live round stays in
        // it, counted; a pull then is dry; the snap the other way shuts it, latched and ready to fire again. Not while a shot of it is under way.
        Sim g;
        m320::Clear();
        g.Step();  // loaded, shut
        g.Run(0.7f);
        spun(g, z, 10.47f, 4);
        g.Run(0.4f);
        const bool openLoaded = g.n.flickOpen == 1 && !g.r.latched && g.r.content == m320::Content::kSeated && g.r.roundLive && !g.r.fireReady && g.n.unload == 0 &&
                                g.loaded == 1;
        g.OnGripB();
        g.Step();
        g.Release();
        const bool dry = g.r.dryPull && !g.r.released;
        g.OffGrip();
        g.Run(0.3f);
        spun(g, z, -10.47f, 4);
        g.RunUntil([](const m320::Result& x) { return x.latchedNow; }, 0.6f);
        g.Run(0.1f);
        Check(openLoaded && dry && g.n.flickShut == 1 && g.r.latched && g.r.fireReady && g.loaded == 1 && g.n.unload == 0,
              "10 flick: loaded and shut, the same snap opens it, the live round still in it and counted (%d loaded); a pull then dry; snapped back: shut, ready to fire (%d)",
              g.loaded, g.r.fireReady);
        Sim u;
        m320::Clear();
        u.Step();
        u.Run(0.7f);
        u.f.shotUnderway = true;
        spun(u, z, 10.47f, 4);
        u.Run(0.4f);
        u.f.shotUnderway = false;
        Check(u.n.flickOpen == 0 && u.r.latched, "10 flick: a shot of it under way (picked, firing, the rifle coming back): the snap opens nothing (%d)", u.n.flickOpen);
        // Off (m320flick 0) and the other way round (m320flick -9).
        m320::SetFlick(0.0f);
        Sim h;
        spent(h);
        spun(h, z, 10.47f, 4);
        h.Run(0.4f);
        m320::SetFlick(-9.0f);
        Sim k;
        spent(k);
        spun(k, z, -10.47f, 4);
        k.Run(0.4f);
        m320::SetFlick(9.0f);
        Check(h.n.flickOpen == 0 && k.n.flickOpen == 1, "10 flick: \"m320flick 0\" none (%d); \"m320flick -9\" the roll to the left opens it (%d)", h.n.flickOpen, k.n.flickOpen);
        m320::SetFlickOpens(false);
    }

    // ---- 11. A lost hand keeps what it has a second: then the case falls, a new round goes back ----
    {
        Sim s;
        s.Open();
        s.HandOnRim(0.0f);
        s.Squeeze();
        s.Run(0.2f);
        float exit[3];
        ExitOnGun(m320::kOpenMost, exit);
        for (int i = 0; i < 200 && !s.r.freed; ++i) {
            float h[3];
            HandOnGun(s.f, 0, h);
            for (int k = 0; k < 3; ++k)
                h[k] += exit[k] * 0.001f;
            PutHandOnGun(s.f, 0, h);
            s.Step();
        }
        s.f.hand[0].tracked = false;
        const float drop = s.RunUntil([](const m320::Result& x) { return x.drop; }, 2.0f);
        const bool spentFell = s.r.drop && !s.r.dropLive && s.n.drop == 1 && Finite12(s.r.dropView);
        Sim t;
        t.OpenEmpty();
        t.TakeAtBelly();
        t.f.hand[0].tracked = false;
        const int dropsBefore = t.n.drop;  // (the case tipped out by OpenEmpty)
        const float gone = t.RunUntil([](const m320::Result& x) { return x.cancelled; }, 2.0f);
        t.n.drop -= dropsBefore;
        Check(spentFell && drop >= 1.0f - 0.001f && drop <= 1.0f + kDt + 0.001f, "11 lost: the hand lost with the case in it: it falls %.3f s later", drop);
        Check(gone >= 1.0f - 0.001f && gone <= 1.0f + kDt + 0.001f && t.n.drop == 0 && !t.r.handRound, "11 lost: with a new round: back %.3f s later, nothing falls", gone);
        // Not before: the hand back at 0.9 s keeps it.
        Sim u;
        u.OpenEmpty();
        u.TakeAtBelly();
        u.f.hand[0].tracked = false;
        u.Run(0.9f);
        u.f.hand[0].tracked = true;
        u.Run(0.5f);
        Check(u.n.cancelled == 0 && u.r.handRound && u.r.handState == m320::HandState::kRound, "11 lost: lost 0.9 s and back: the round still in the hand");
    }

    // ---- 12. The game's own reload: the hand lets go, shut; its round in, shut and loaded. Clear: shut and loaded ----
    {
        Sim s;
        s.OpenEmpty();
        s.TakeAtBelly();
        s.f.gameReload = true;
        s.Step();
        const bool yielded = s.r.cancelled && !s.r.drawn && s.r.latched && s.r.barrelState == m320::Barrel::kShut && !s.r.handRound && !s.r.holding;
        const bool readyMeanwhile = s.r.fireReady && m320::FireReady();
        s.Run(0.5f);
        const bool readyStill = s.r.fireReady && m320::FireReady();
        s.f.gameReload = false;
        s.loaded = 1;
        s.Step();
        Check(yielded, "12 game's reload: the round in the hand goes back, the barrel shut as the game's, the launcher the game's to draw");
        Check(readyMeanwhile && readyStill,
              "12 game's reload: meanwhile ready to fire as when not in use (%d, %d), so a reload read stale never turns every pull into a release press thrown away",
              readyMeanwhile, readyStill);
        Check(s.r.reloaded && s.r.fireReady && s.r.content == m320::Content::kSeated && s.r.roundLive && s.r.latched,
              "12 game's reload: its round in: shut and loaded, ready to fire (%s)", s.r.state);
        m320::Clear();
        const bool clearReady = m320::FireReady();
        Sim t;
        t.Step();
        const bool clearLive = t.r.content == m320::Content::kSeated && t.r.roundLive && t.r.latched && t.r.fireReady;
        m320::Clear();
        Sim u;
        u.loaded = 0;
        u.Step();
        Check(clearReady && clearLive && !u.r.roundLive && u.r.latched && !u.r.fireReady,
              "12 Clear: shut and loaded (ready %d, live %d); first seen with the count at 0: shut on a spent case", clearReady, clearLive);
    }

    // ---- 13. The push shut by touch: the barrel follows the hand shut, never through it; slow it doesn't start; stopped short it springs back ----
    {
        auto push = [](float speed, float stopAt, Sim& s, float& deepest) {
            s.OpenEmpty();
            s.f.hand[0].touches = 1;
            float local[3] = {0.075f, -0.036f, 0.53f}, at[3];
            OnTube(m320::kOpenMost, local, at);
            deepest = 1.0f;
            bool stopped = false;
            for (int i = 0; i < 150 && !s.r.latched; ++i) {
                float view[3];
                Transform(s.f.gunPose, at, view);
                std::memcpy(s.f.hand[0].touch[0], view, sizeof(view));
                const float handAt[3] = {at[0] + 0.04f, at[1], at[2]};
                PutHandOnGun(s.f, 0, handAt);
                s.Step();
                if (s.r.barrelState == m320::Barrel::kPushed) {
                    float l[3];
                    ToTube(s.r.barrel, at, l);
                    deepest = std::fmin(deepest, l[0] - (0.0483f + 0.009f));
                }
                if (stopAt > 0.0f && s.r.barrel <= stopAt * kDegree && s.r.barrelState == m320::Barrel::kPushed) {
                    stopped = true;
                    break;
                }
                at[0] -= speed * s.f.dt;
            }
            if (stopped) {  // the hand drawn back off it
                for (int i = 0; i < 10; ++i) {
                    at[0] += 0.3f * s.f.dt;
                    float view[3];
                    Transform(s.f.gunPose, at, view);
                    std::memcpy(s.f.hand[0].touch[0], view, sizeof(view));
                    s.Step();
                }
            }
        };
        Sim a;
        float deepest = 0.0f;
        push(0.3f, -1.0f, a, deepest);
        a.Run(0.2f);
        Check(a.n.shut == 1 && a.n.latchedNow == 1 && a.r.latched && deepest > -0.001f && a.r.content == m320::Content::kNone,
              "13 push: a palm pushed on its left side at 0.3 m/s: the barrel with it, never through it (deepest %.2f mm), snapped and latched, empty", deepest * 1000.0f);
        Sim b;
        push(0.02f, -1.0f, b, deepest);
        Check(b.n.shut == 0 && !b.r.latched, "13 push: at 0.02 m/s (under the slap's 0.05) it doesn't start (%s)", b.r.state);
        Sim c;
        push(0.3f, 8.0f, c, deepest);
        const float reopen = c.RunUntil([](const m320::Result& x) { return x.openStop; }, 1.0f);
        Check(c.n.sprungBack >= 1 && reopen > 0.0f && c.r.barrel == m320::kOpenMost, "13 push: pushed to 8 degrees and the hand drawn off: its springs push it back open (at the stop %.3f s later)",
              reopen);
    }

    // ---- 14. Shut, the hand on the barrel's back end opens nothing, spent or loaded: the release or the flick opens it (a squeeze for the handguard coming in from the
    // left took the barrel instead, the review 2026-11-05; the "forgiving" open by hand is gone) ----
    {
        const float beside[3] = {m320::kBarrelGrab[0] + 0.03f, m320::kBarrelGrab[1], m320::kBarrelGrab[2]};
        auto handAt = [&](Sim& s, float degrees) {
            float p[3];
            OnTube(degrees * kDegree, beside, p);
            PutHandOnGun(s.f, 0, p);
        };
        Sim a;
        m320::Clear();
        a.Step();
        a.Fire();
        handAt(a, 0.0f);
        a.Squeeze();
        const bool taken = a.r.barrelTaken || a.r.released || !a.r.latched || a.r.spentSqueeze;
        for (float d = 1.0f; d <= 5.0f; d += 1.0f) {
            handAt(a, d);
            a.Step();
        }
        Check(!taken && a.r.latched && a.r.barrel == 0.0f && a.r.barrelState == m320::Barrel::kShut,
              "14 by hand: spent and shut, the hand on its back end takes nothing (the squeeze left for the handguard), still latched");
        Sim b;
        m320::Clear();
        b.Step();
        b.Fire();
        b.f.releasePress = true;  // the release opens it; then the hand swings it shut from beside the back end
        b.Step();
        b.f.releasePress = false;
        b.RunUntil([](const m320::Result& x) { return x.openStop; }, 1.0f);
        handAt(b, 15.0f);
        b.Squeeze();
        const bool held = b.r.barrelTaken;
        for (float d = 14.0f; d >= 3.0f; d -= 1.0f) {
            handAt(b, d);
            b.Step();
        }
        b.LetGo();
        const float latched = b.RunUntil([](const m320::Result& x) { return x.latchedNow; }, 0.5f);
        Check(held && latched > 0.0f && b.r.latched, "14 by hand: opened by the release, the hand on its back end swings it shut: it snaps and latches %.3f s after the let go", latched);
        Sim c;
        m320::Clear();
        c.Step();  // loaded
        handAt(c, 0.0f);
        c.Squeeze();
        Check(!c.r.barrelTaken && c.r.latched && c.r.fireReady, "14 by hand: loaded, the same squeeze opens nothing");
    }

    // ---- 15. The release refused: loaded it fires (input.cpp's own), open it's dry ----
    {
        Sim s;
        m320::Clear();
        s.Step();
        s.OnGripB();
        s.Step();
        s.Release();
        Check(!s.r.released && s.r.latched && s.r.fireReady, "15 release: loaded and shut, a press of the release does nothing (\"%s\")", s.r.note);
        Sim t;
        t.Open();
        t.OnGripB();
        t.Step();
        t.Release();
        Check(t.r.dryPull && !t.r.released && t.n.dryPull == 1, "15 release: open, a pull of the trigger is dry (\"%s\")", t.r.note);
        // The game's hand read as still holding it long after a put-away (ammo::M320Up stale): input.cpp presses only from its idle, so the press opens it.
        Sim u;
        m320::Clear();
        u.Step();
        u.Fire();
        u.OnGripB();
        u.Step();
        u.f.inGameHand = true;
        u.Release();
        Check(u.r.released && !u.r.latched && u.r.barrelState == m320::Barrel::kSpringing,
              "15 release: spent and shut, the game's hand read in hand (stale): the press still opens it (%s)", u.r.state);
    }

    // ---- 16. The right hand at work (the rifle in the left): the same belly, a round mirrored into the right glove, its fingers the left's own table (in the left
    // glove's layout: hands.cpp flips them onto the right glove) ----
    {
        Sim s;
        s.f.gunHand = 0;
        m320::Clear();
        s.Step();
        s.Fire();
        s.f.hand[1].onGripB = true;
        s.f.hand[1].squeeze = 1.0f;
        s.Step();
        s.f.releasePress = true;
        s.f.hand[1].trigger = 1.0f;
        s.Step();
        s.f.hand[1].trigger = 0.0f;
        s.Run(0.3f);
        s.f.hand[1].onGripB = false;
        s.f.hand[1].squeeze = 0.0f;
        s.Step();
        s.TakeAtBelly(1);
        const bool took = s.r.took && s.r.side == 1 && s.r.handSide == 1;
        const float turn = Determinant(s.r.roundInGlove);
        const float (*rightTable)[12] = s.r.fingers;
        Sim l;  // (the left's from the belly: its table)
        l.OpenEmpty();
        l.TakeAtBelly(0);
        Check(took && std::fabs(turn - 1.0f) < 1e-3f && rightTable && rightTable == l.r.fingers,
              "16 right hand: it presses the release, the belly gives it a round (side %d, a true turn in its glove: %.4f, the left's finger table: %d)", s.r.handSide,
              turn, rightTable && rightTable == l.r.fingers);
    }

    // ---- 17. The launcher put away mid-reload: the case in the hand falls (never vanishes), the barrel stays open ----
    {
        Sim s;
        s.Open();
        s.HandOnRim(0.0f);
        s.Squeeze();
        s.Run(0.2f);
        float exit[3];
        ExitOnGun(m320::kOpenMost, exit);
        for (int i = 0; i < 200 && !s.r.freed; ++i) {
            float h[3];
            HandOnGun(s.f, 0, h);
            for (int k = 0; k < 3; ++k)
                h[k] += exit[k] * 0.001f;
            PutHandOnGun(s.f, 0, h);
            s.Step();
        }
        s.f.active = false;
        s.Step();
        // A moment (0.2 s) everything is kept, the case still drawn in the hand; then it falls.
        const bool kept = !s.r.drop && !s.r.drawn && s.r.fireReady && s.r.handRound && s.r.holding && s.r.handState == m320::HandState::kRound;
        const float fellAt = s.RunUntil([](const m320::Result& x) { return x.drop; }, 0.5f) + s.f.dt;  // (from the first frame not in use)
        const bool fell = s.r.drop && !s.r.dropLive && !s.r.drawn && s.r.fireReady && s.n.drop == 1;
        s.Run(0.3f);
        s.f.active = true;
        s.f.hand[0].squeeze = 0.0f;
        s.Step();
        Check(kept && fell && fellAt >= 0.2f - 0.001f && fellAt <= 0.2f + kDt + 0.001f && s.r.barrelState == m320::Barrel::kOpen && s.r.content == m320::Content::kNone &&
                  !s.r.handRound,
              "17 put away: the case in the hand kept a moment, then falls %.3f s after (0.2), the firing the game's meanwhile; back, the barrel still open and empty (%s)",
              fellAt, s.r.state);
    }

    // ---- 18. Broken inputs: nothing goes wrong ----
    {
        Sim s;
        s.Open();
        s.f.dt = std::nanf("");
        s.f.hand[0].squeeze = std::nanf("");
        s.f.hand[0].trigger = std::nanf("");
        s.Step();
        s.f.dt = kDt;
        s.f.hand[0].squeeze = 0.0f;
        s.f.hand[0].trigger = 0.0f;
        s.f.hand[0].glove[3] = std::nanf("");
        s.Step();
        s.f.hand[0].glove[3] = 0.0f;
        PutHandOnGun(s.f, 0, kAway);
        s.Run(0.2f);
        Check(std::isfinite(s.r.barrel) && Finite12(s.r.roundOnGun) && Finite12(s.r.hinge) && s.r.barrel == m320::kOpenMost, "18 broken inputs: a NaN dt, squeeze, trigger and glove: all still finite, open");
        // Holding something: a frame each of a NaN glove, a NaN gun pose, a NaN gun as drawn, an infinite grip point, a NaN head; each taken as not seen that
        // frame (never stored), what's held kept where it was, and all as before once they're good again.
        auto broken = [](Sim& x, int which) {
            if (which == 0) {
                const float keep = x.f.hand[0].glove[5];
                x.f.hand[0].glove[5] = std::nanf("");
                x.Step();
                x.f.hand[0].glove[5] = keep;
            } else if (which == 1) {
                const float keep = x.f.gunPose[3];
                x.f.gunPose[3] = std::nanf("");
                x.Step();
                x.f.gunPose[3] = keep;
            } else if (which == 2) {
                const float keep = x.f.gunRoom[7];
                x.f.gunRoom[7] = std::nanf("");
                x.Step();
                x.f.gunRoom[7] = keep;
            } else if (which == 3) {
                const float keep = x.f.hand[0].gripRoom[1];
                x.f.hand[0].gripRoom[1] = std::numeric_limits<float>::infinity();
                x.Step();
                x.f.hand[0].gripRoom[1] = keep;
            } else {
                const float keep = x.f.headPose[0];
                x.f.headPose[0] = std::nanf("");
                x.Step();
                x.f.headPose[0] = keep;
            }
        };
        auto allFinite = [](const m320::Result& x) {
            return std::isfinite(x.barrel) && std::isfinite(x.roundS) && std::isfinite(x.roundV) && Finite12(x.hinge) && Finite12(x.tubeOnGun) && Finite12(x.roundOnGun) &&
                   Finite12(x.gloveOnGun) && std::isfinite(x.gloveBlend) && Finite12(x.handRoundView) && Finite12(x.dropView);
        };
        bool barrelOk = true, pinchOk = true;
        float barrelWorst = 0.0f, pinchWorst = 0.0f;
        Sim b;
        b.Open();
        HandBesideBarrel(b, 15.0f);
        b.Squeeze();
        for (float d = 14.0f; d >= 12.0f; d -= 1.0f) {
            HandBesideBarrel(b, d);
            b.Step();
        }
        const float heldAt = b.r.barrel;
        for (int which = 0; which < 5; ++which) {
            broken(b, which);
            barrelOk = barrelOk && allFinite(b.r) && b.r.handState == m320::HandState::kBarrel;
            barrelWorst = std::fmax(barrelWorst, std::isfinite(b.r.barrel) ? std::fabs(b.r.barrel - heldAt) : 1.0f);
            b.Step();
            barrelOk = barrelOk && allFinite(b.r) && b.r.handState == m320::HandState::kBarrel;
            barrelWorst = std::fmax(barrelWorst, std::isfinite(b.r.barrel) ? std::fabs(b.r.barrel - heldAt) : 1.0f);
        }
        Sim p;
        p.Open();
        p.HandOnRim(0.0f);
        p.Squeeze();
        p.Run(0.2f);
        DrawAlong(p, 0.01f);
        const float pinchedAt = p.r.roundS;
        for (int which = 0; which < 5; ++which) {
            broken(p, which);
            pinchOk = pinchOk && allFinite(p.r) && p.r.handState == m320::HandState::kPinch;
            pinchWorst = std::fmax(pinchWorst, std::isfinite(p.r.roundS) ? std::fabs(p.r.roundS - pinchedAt) : 1.0f);
            p.Step();
            pinchOk = pinchOk && allFinite(p.r) && p.r.handState == m320::HandState::kPinch;
            pinchWorst = std::fmax(pinchWorst, std::isfinite(p.r.roundS) ? std::fabs(p.r.roundS - pinchedAt) : 1.0f);
        }
        DrawAlong(p, 0.01f);
        const float pulledOn = p.r.roundS - pinchedAt;
        Check(barrelOk && barrelWorst < 1e-5f, "18 broken inputs: the barrel held at %.1f degrees through a NaN glove, gun pose, gun as drawn, grip point and head: finite, held, %.5f degrees off",
              heldAt / kDegree, barrelWorst / kDegree);
        Check(pinchOk && pinchWorst < 1e-5f && std::fabs(pulledOn - 0.01f) < 0.0005f,
              "18 broken inputs: the rim pinched %.1f mm out through the same: finite, still pinched, %.4f mm moved; pulled on after, %.1f mm more (10)", pinchedAt * 1000.0f,
              pinchWorst * 1000.0f, pulledOn * 1000.0f);
    }

    // ---- 19. Its shape against the recording ----
    {
        float mouth[3], hinge[12];
        m320::MouthAt(m320::kOpenMost, mouth);
        m320::HingeAt(m320::kOpenMost, hinge);
        const float recorded[3] = {0.081f, -0.036f, 0.513f};
        const float tubeZ[3] = {hinge[2], hinge[6], hinge[10]};
        float target[3], round[12], onGun[3], grip[3];
        m320::PinchTarget(target);
        m320::RoundOnLine(m320::kOpenMost, 0.0f, round);
        Transform(round, target, onGun);
        m320::GripMiddle(grip);
        Check(Apart(mouth, recorded) < 0.0015f && std::fabs(tubeZ[0] + 0.259f) < 0.001f && std::fabs(tubeZ[2] - 0.966f) < 0.001f && mouth[0] > 0.07f,
              "19 shape: the barrel's back end 15 degrees open at (%.4f %.4f %.4f) (recorded 0.081 -0.036 0.513), the tube along (%.3f %.3f %.3f), to the gun's left", mouth[0],
              mouth[1], mouth[2], tubeZ[0], tubeZ[1], tubeZ[2]);
        Check(Apart(onGun, grip) > 0.12f, "19 shape: the hand on the rim at (%.3f %.3f %.3f), on the grip at (%.3f %.3f %.3f): %.1f cm apart", onGun[0], onGun[1], onGun[2],
              grip[0], grip[1], grip[2], Apart(onGun, grip) * 100.0f);
    }

    // ---- 21. A squeeze for the handguard, low on its left side, is the handguard's: the spent launcher's barrel isn't taken (or opened) by it ----
    {
        const float lowLeft[3] = {0.05f, 0.0f, 0.52f};  // 3.6 cm from the barrel's grab point, 6.7 cm from the handguard's line
        Sim s;
        m320::Clear();
        s.Step();
        s.Fire();
        PutHandOnGun(s.f, 0, lowLeft);
        s.Squeeze();
        const bool shutLeft = !s.r.barrelTaken && !s.r.spentSqueeze && s.r.latched;
        Sim t;
        t.Open();
        float p[3];
        const float local[3] = {0.05f, 0.0f, 0.52f};
        OnTube(m320::kOpenMost, local, p);
        p[0] = 0.05f;  // the same place on the gun, the barrel open beside it
        PutHandOnGun(t.f, 0, p);
        t.Squeeze();
        Check(shutLeft && !t.r.barrelTaken && !t.r.spentSqueeze, "21 handguard: a squeeze low on its left side is left for the handguard, shut (latched %d) or open", s.r.latched);
    }

    // ---- 22. Just in front of the barrel's back end but nearer the rim than the barrel: the case, not the barrel ----
    {
        Sim s;
        s.Open();
        const float local[3] = {0.11f, -0.025f, 0.49f};
        float p[3];
        OnTube(m320::kOpenMost, local, p);
        PutHandOnGun(s.f, 0, p);
        s.Squeeze();
        Check(s.r.pinched && !s.r.barrelTaken, "22 rim or barrel: the hand 5 mm in front of the back end, 1.7 cm from the rim and 7 cm from the barrel's grab: the rim");
    }

    // ---- 23. Seated with the muzzle up: it slides straight back out as the count comes up a frame later; that's the hand's own round, not a reload ----
    {
        Sim s;
        s.OpenEmpty();
        s.TakeAtBelly();
        TiltExit(s.f, 60.0f * kDegree);
        s.Step();
        float mouth[3], inward[3], nose[3], pose[12];
        m320::MouthAt(m320::kOpenMost, mouth);
        ExitOnGun(m320::kOpenMost, inward);
        for (float& v : inward)
            v = -v;
        for (int k = 0; k < 3; ++k)
            nose[k] = mouth[k] - inward[k] * 0.04f;
        RoundPose(inward, nose, pose);
        s.InsertByHand();
        s.LetGo();
        s.RunUntil([](const m320::Result& x) { return x.drop; }, 1.0f);
        s.Run(0.5f);
        Check(s.n.seated == 1 && s.n.reloaded == 0 && s.n.unload == 1 && s.loaded == 0 && s.spare == 3 && !s.r.latched,
              "23 seated, muzzle up: it slides back out (seated %d, unload %d, taken for a reload %d), %d spare, the barrel still open", s.n.seated, s.n.unload, s.n.reloaded,
              s.spare);
    }

    // ---- 24. The hand off the rim: pinched, the case drawn 1 cm, then the real hand straight down (nothing moves the case that way). 9 cm off, it holds on;
    // 3 frames over 12 cm, it has let go: the case stays where it was, the glove goes back to the real hand (an arm never stretches) ----
    {
        Sim s;
        s.Open();
        s.HandOnRim(0.0f);
        s.Squeeze();
        s.Run(0.2f);
        DrawAlong(s, 0.01f);
        const float drawnTo = s.r.roundS;
        float start[3], drawn[3];
        HandOnGun(s.f, 0, start);
        DrawnMiddle(s.r, 0, drawn);
        const float onRim = Apart(start, drawn);
        auto down = [&](float metres) {
            const float at[3] = {start[0], start[1] - metres, start[2]};
            PutHandOnGun(s.f, 0, at);
            s.Step();
        };
        for (int i = 1; i <= 6; ++i)
            down(0.015f * static_cast<float>(i));
        for (int i = 0; i < 5; ++i)
            down(0.09f);
        const bool heldAt9 = s.r.handState == m320::HandState::kPinch && s.r.gloveBlend == 1.0f;
        int over = 0, letGoAt = -1;
        for (int i = 1; i <= 10 && letGoAt < 0; ++i) {
            const float metres = std::fmin(0.09f + 0.014f * static_cast<float>(i), 0.15f);  // 10.4, 11.8, 13.2, 14.6, then 15 cm
            over += metres > 0.12f ? 1 : 0;
            down(metres);
            if (s.r.handState == m320::HandState::kFree)
                letGoAt = over;
        }
        s.Run(0.2f);
        Check(onRim < 0.001f && heldAt9, "24 off the rim: the glove drawn on the hand (%.4f m), the hand 9 cm straight down for 5 frames: still on the rim", onRim);
        Check(letGoAt == 3 && std::fabs(s.r.roundS - drawnTo) < 1e-6f && s.r.content == m320::Content::kSliding && !s.r.holding && s.r.gloveBlend == 0.0f && s.n.drop == 0,
              "24 off the rim: 15 cm down it lets go on the 3rd frame over 12 cm (%d), the case stays %.1f mm out, the glove back on the real hand (%.2f)", letGoAt,
              s.r.roundS * 1000.0f, s.r.gloveBlend);
    }

    // ---- 25. gunRoom a frame late, as hands.cpp gives it, the frames 13.9 and 27.8 ms by turns: each difference over the frame it spans. A quick cant under the
    // flick's rate doesn't flick, a real flick does; the gun carried steadily along its length doesn't jab; a case falling out takes the gun's own speed ----
    {
        const float z[3] = {0.0f, 0.0f, 1.0f};
        int frame = 0;
        auto still = [&frame](Sim& x, float seconds) {
            for (float t = 0.0f; t < seconds; ++frame) {
                x.f.dt = Uneven(frame);
                t += x.f.dt;
                x.Step();
            }
        };
        auto roll = [&frame, &z](Sim& x, float rate, float degrees) {
            float turned = 0.0f;
            while (turned < degrees * kDegree - 1e-6f) {
                x.f.dt = Uneven(frame++);
                const float step = std::fmin(rate * x.f.dt, degrees * kDegree - turned);
                float turn[12];
                Rotation(z, step, turn);
                TurnGunOwn(x.f, turn);
                turned += step;
                x.Step();
            }
        };
        auto spentLagged = [&frame](Sim& x) {
            m320::Clear();
            x.Step();
            x.Fire();
            x.Run(0.7f);
            x.lag = true;
            frame = 0;
        };
        m320::SetFlickOpens(true);  // the detector under uneven frames, as it was (it opens nothing by default: row 10)
        Sim a;
        spentLagged(a);
        still(a, 0.3f);
        roll(a, 7.5f, 30.0f);  // 430 degrees/s
        still(a, 0.6f);
        Sim b;
        spentLagged(b);
        still(b, 0.3f);
        roll(b, 10.47f, 33.0f);  // 600 degrees/s
        still(b, 0.6f);
        Check(a.n.flickOpen == 0 && a.r.latched && b.n.flickOpen == 1 && !b.r.latched,
              "25 a frame late, uneven frames: a 30 degree cant at 430 degrees/s doesn't flick it open (%d); 600 degrees/s for 33 degrees does (%d)", a.n.flickOpen,
              b.n.flickOpen);
        // A hitch: one frame of 0.1 s (the most dt is taken as) with the gun rolled 30 degrees across it: nothing is read across it, so no flick.
        Sim h;
        spentLagged(h);
        still(h, 0.3f);
        h.f.dt = 0.1f;
        {
            float turn[12];
            Rotation(z, 30.0f * kDegree, turn);
            TurnGunOwn(h.f, turn);
        }
        h.Step();
        still(h, 0.6f);
        Check(h.n.flickOpen == 0 && h.r.latched, "25 a frame late: a 0.1 s hitch with a 30 degree roll across it: nothing read across it, no flick (%d)", h.n.flickOpen);
        m320::SetFlickOpens(false);
        // Carried along its length at 0.5 m/s, the barrel open with the case in it.
        Sim c;
        c.Open();
        c.lag = true;
        frame = 0;
        float room[12];
        std::memcpy(room, c.f.gunRoom, sizeof(room));
        const float muzzle[3] = {room[2], room[6], room[10]};
        for (float t = 0.0f; t < 0.6f; ++frame) {
            c.f.dt = Uneven(frame);
            t += c.f.dt;
            for (int k = 0; k < 3; ++k)
                room[k * 4 + 3] += muzzle[k] * 0.5f * c.f.dt;
            SetGunRoom(c.f, room);
            c.Step();
        }
        Check(c.n.jabbed == 0 && c.r.roundS == 0.0f && c.r.content == m320::Content::kSeated,
              "25 a frame late, uneven frames: the gun carried along its length at 0.5 m/s: no jab (%d), the case still in (%.3f m)", c.n.jabbed, c.r.roundS);
        // The muzzle up 60 degrees, the gun carried 0.5 m/s to the room's right: the case falls out with that speed and its slide.
        Sim d;
        d.Open();
        TiltExit(d.f, 60.0f * kDegree);
        d.lag = true;
        frame = 0;
        std::memcpy(room, d.f.gunRoom, sizeof(room));
        const float across[3] = {0.5f, 0.0f, 0.0f};
        bool dropped = false;
        for (float t = 0.0f; t < 1.0f && !dropped; ++frame) {
            d.f.dt = Uneven(frame);
            t += d.f.dt;
            for (int k = 0; k < 3; ++k)
                room[k * 4 + 3] += across[k] * d.f.dt;
            SetGunRoom(d.f, room);
            d.Step();
            dropped = d.r.drop;
        }
        const float gun[3] = {d.r.dropVelocityRoom[0] - d.r.dropSlideRoom[0], d.r.dropVelocityRoom[1] - d.r.dropSlideRoom[1], d.r.dropVelocityRoom[2] - d.r.dropSlideRoom[2]};
        Check(dropped && d.r.dropFromBarrel && Apart(gun, across) < 0.01f && Length(d.r.dropSlideRoom) > 0.5f,
              "25 a frame late, uneven frames: the case out of the barrel with the gun's own (%.3f %.3f %.3f) m/s (0.5 to the right) and its slide %.2f", gun[0], gun[1], gun[2],
              Length(d.r.dropSlideRoom));
    }

    // ---- 26. Only the gun lost (the right controller, two seconds): a round or a case in the left hand stays in it; the rim needs the gun, and lets go a second on ----
    {
        Sim t;
        t.OpenEmpty();
        t.TakeAtBelly();
        PutHandOnGun(t.f, 0, kAway);
        t.Step();
        t.f.gunTracked = false;
        t.Run(2.0f);
        const bool roundKept = t.n.cancelled == 0 && t.r.handRound && t.r.handRoundLive && t.r.handState == m320::HandState::kRound;
        t.f.gunTracked = true;
        t.Run(0.2f);
        const bool roundBack = t.n.cancelled == 0 && t.r.handState == m320::HandState::kRound;
        Sim c;
        c.Open();
        c.HandOnRim(0.0f);
        c.Squeeze();
        c.Run(0.2f);
        DrawAlong(c, m320::kClear + 0.005f);
        const bool inHand = c.r.handState == m320::HandState::kRound && !c.r.handRoundLive;
        PutHandOnGun(c.f, 0, kAway);
        c.Step();
        const int dropsBefore = c.n.drop;
        c.f.gunTracked = false;
        c.Run(2.0f);
        const bool caseKept = c.n.drop == dropsBefore && c.r.handRound && !c.r.handRoundLive && c.r.handState == m320::HandState::kRound;
        Sim p;
        p.Open();
        p.HandOnRim(0.0f);
        p.Squeeze();
        p.Run(0.2f);
        DrawAlong(p, 0.01f);
        p.f.gunTracked = false;
        const float gone = p.RunUntil([](const m320::Result& x) { return x.handState == m320::HandState::kFree; }, 2.0f);
        Check(roundKept && roundBack && inHand && caseKept, "26 gun lost: a new round (%d) and a pulled case (%d) stay in the tracked left hand for 2 s, the round still there after (%d)",
              roundKept, caseKept, roundBack);
        Check(gone >= 1.0f - 0.001f && gone <= 1.0f + kDt + 0.001f && p.r.content == m320::Content::kSliding && std::fabs(p.r.roundS - 0.01f) < 0.0005f,
              "26 gun lost: on the rim, it lets go %.3f s later, the case staying %.1f mm out", gone, p.r.roundS * 1000.0f);
    }

    // ---- 27. Two drops in one frame: the new round let go of from the hand just as the old case slides out of the barrel. The hand's falls that frame; the
    // case waits that frame at the barrel's back end, drawn, and falls the next: nothing vanishes ----
    {
        Sim s;
        s.Open();
        s.TakeAtBelly();  // (the belly gives a 40mm with the spent case still in)
        const bool took = s.r.took;
        PutHandOnGun(s.f, 0, kAway);
        s.Step();
        TiltExit(s.f, 60.0f * kDegree);
        const float a = kG * (std::sin(60.0f * kDegree) - kMu * std::cos(60.0f * kDegree));
        bool together = false, handDrop = false, waited = false, caseDrop = false;
        for (int i = 0; i < 200 && !together; ++i) {
            const bool inBarrel = s.r.content == m320::Content::kSliding || s.r.content == m320::Content::kSeated;
            together = inBarrel && s.r.roundS + (s.r.roundV + a * s.f.dt) * s.f.dt >= m320::kClear;
            if (together)
                s.f.hand[0].squeeze = 0.0f;  // let go of on the very frame the case comes clear
            s.Step();
            if (together) {
                handDrop = s.r.drop && s.r.letGo && !s.r.dropFromBarrel && s.r.dropLive;
                waited = s.r.round == m320::Round::kGame && std::fabs(s.r.roundOut - m320::kClear) < 1e-6f && !s.r.roundLive;
                s.Step();
                caseDrop = s.r.drop && s.r.dropFromBarrel && !s.r.dropLive && s.r.round == m320::Round::kHidden;
            }
        }
        s.Run(0.3f);
        Check(took && together && handDrop && waited && caseDrop && s.n.drop == 2,
              "27 two drops: the live round from the hand falls (%d), the spent case waits a frame at the back end (%d) and falls the next (%d): %d drops", handDrop, waited,
              caseDrop, s.n.drop);
    }

    // ---- 28. A frame not in use (a state read that failed, a draw missed): everything kept, the round in the hand still drawn. 0.25 s: let go of as before ----
    {
        Sim p;
        p.Open();
        p.HandOnRim(0.0f);
        p.Squeeze();
        p.Run(0.2f);
        DrawAlong(p, 0.01f);
        const float pinchedAt = p.r.roundS;
        p.f.active = false;
        p.Step();
        const bool dipPinch = !p.r.drawn && p.r.holding && p.r.fireReady && !p.r.drop;
        p.f.active = true;
        p.Step();
        const bool backPinch = p.r.handState == m320::HandState::kPinch && std::fabs(p.r.roundS - pinchedAt) < 1e-6f && p.n.drop == 0 && p.r.gloveBlend == 1.0f;
        Sim q;
        q.OpenEmpty();
        q.TakeAtBelly();
        PutHandOnGun(q.f, 0, kAway);
        q.Step();
        q.f.active = false;
        q.Step();
        const bool dipRound = !q.r.drawn && q.r.handRound && q.r.handRoundLive && q.r.holding && Finite12(q.r.handRoundView);
        q.f.active = true;
        q.Step();
        const bool backRound = q.r.handState == m320::HandState::kRound && q.n.cancelled == 0;
        Sim b;
        b.Open();
        HandBesideBarrel(b, 15.0f);
        b.Squeeze();
        for (float d = 14.0f; d >= 12.0f; d -= 1.0f) {
            HandBesideBarrel(b, d);
            b.Step();
        }
        const float heldAt = b.r.barrel;
        b.f.active = false;
        b.Step();
        b.f.active = true;
        b.Step();
        const bool backBarrel = b.r.handState == m320::HandState::kBarrel && b.r.barrelState == m320::Barrel::kHeld && std::fabs(b.r.barrel - heldAt) < 1e-6f;
        b.f.active = false;
        b.Run(0.25f);
        bool longer = b.r.handState == m320::HandState::kFree && b.r.barrelState == m320::Barrel::kOpen && !b.r.holding && b.n.barrelLet == 1;
        b.f.active = true;
        b.f.hand[0].squeeze = 0.0f;
        b.Step();
        longer = longer && b.r.barrel == m320::kOpenMost && b.r.barrelState == m320::Barrel::kOpen && b.r.handState == m320::HandState::kFree;
        Check(dipPinch && backPinch, "28 a frame not in use: on the rim, drawn as the game's that frame, still pinched after it, the case %.1f mm out as it was", p.r.roundS * 1000.0f);
        Check(dipRound && backRound, "28 a frame not in use: a new round still drawn in the hand that frame (%d), still in it after (%d)", dipRound, backRound);
        Check(backBarrel && longer, "28 a frame not in use: the barrel held at %.1f degrees still held there after (%d); 0.25 s not in use, let go of, finished open (%d)",
              heldAt / kDegree, backBarrel, longer);
    }

    // ---- 29. The barrel let go of at its stop: open, nothing moves: no stop sound, nothing sprung back ----
    {
        Sim s;
        s.Open();
        HandBesideBarrel(s, 15.0f);
        s.Squeeze();
        const bool taken = s.r.barrelTaken;
        s.Run(0.1f);
        const Counts before = s.n;
        s.LetGo();
        const bool let = s.r.barrelLet && !s.r.sprungBack && s.r.barrelState == m320::Barrel::kOpen && s.r.barrel == m320::kOpenMost;
        s.Run(0.5f);
        Check(taken && let && s.n.openStop == before.openStop && s.n.sprungBack == before.sprungBack && s.r.barrelState == m320::Barrel::kOpen,
              "29 at its stop: taken at 15 degrees and let go of there: open (%d), open_stop %d more, sprung back %d more", let, s.n.openStop - before.openStop,
              s.n.sprungBack - before.sprungBack);
    }

    // ---- 30. The push shut by the glove's own points (none given, as hands.cpp runs it): the glove as the game's hand slaps it shut (frame 7469, its palm 3 cm
    // out from the barrel's left side) moving to the gun's right: at 0.3 m/s it follows the palm shut, never through it; at 0.02 m/s it doesn't start ----
    {
        constexpr float kPalm[3] = {0.5267f, 1.0864f, 0.2065f};  // the glove's palm point (m320.cpp kTouchInGlove[0])
        auto slap = [&kPalm](float speed, Sim& s, float& deepest, float& startOut) {
            s.OpenEmpty();
            s.f.hand[0].touches = 0;
            float view[12], glove[12], inverse[12];
            Compose(s.f.gunPose, s.r.tubeOnGun, view);
            m320::GloveFor(m320::Hold::kBarrel, 0, view, glove);
            InvertRigid(s.f.gunPose, inverse);
            const float left[3] = {s.f.gunPose[0], s.f.gunPose[4], s.f.gunPose[8]};  // the gun's x (its left) in the view
            auto palmOut = [&](float angle) {
                float palm[3], onGun[3], local[3];
                Transform(glove, kPalm, palm);
                Transform(inverse, palm, onGun);
                ToTube(angle, onGun, local);
                return local[0] - (0.0483f + 0.009f);
            };
            startOut = palmOut(s.r.barrel);
            deepest = 1.0f;
            for (int i = 0; i < 150 && !s.r.latched; ++i) {
                std::memcpy(s.f.hand[0].glove, glove, sizeof(glove));
                float middle[3];
                Transform(glove, kMid, middle);
                Transform(s.f.headPose, middle, s.f.hand[0].gripRoom);
                s.Step();
                if (s.r.barrelState == m320::Barrel::kPushed)
                    deepest = std::fmin(deepest, palmOut(s.r.barrel));
                for (int k = 0; k < 3; ++k)
                    glove[k * 4 + 3] -= left[k] * speed * s.f.dt;
            }
        };
        Sim a;
        float deepest = 0.0f, startOut = 0.0f;
        slap(0.3f, a, deepest, startOut);
        a.Run(0.2f);
        Check(startOut > 0.02f && a.n.shut == 1 && a.n.latchedNow == 1 && a.r.latched && deepest > -0.001f && a.r.content == m320::Content::kNone,
              "30 push by the glove: the palm %.1f cm out, at 0.3 m/s the barrel follows it shut, never through it (deepest %.2f mm), snapped and latched", startOut * 100.0f,
              deepest * 1000.0f);
        Sim b;
        slap(0.02f, b, deepest, startOut);
        Check(b.n.shut == 0 && !b.r.latched, "30 push by the glove: at 0.02 m/s it doesn't start (%s)", b.r.state);
    }

    // ---- 34. The slap (the user, 2026-11-05: "make the slap a lot more responsive ... Sometimes it doesn't really capture it"): the glove at the barrel's left side,
    // 4 cm higher than its line (out of the push's 3.5 cm band) and 8 cm along it, coming at it at 0.8 m/s: shut and latched; at 0.2 m/s there nothing; at 0.8 m/s
    // behind its back end (the rim's place) nothing; the barrel springing open into a hand held still beside it: nothing ----
    {
        auto slapAt = [](float speed, float up, float along, Sim& s) {
            s.OpenEmpty();
            s.f.hand[0].touches = 0;
            float view[12], glove[12];
            Compose(s.f.gunPose, s.r.tubeOnGun, view);
            m320::GloveFor(m320::Hold::kBarrel, 0, view, glove);
            const float left[3] = {s.f.gunPose[0], s.f.gunPose[4], s.f.gunPose[8]}, upView[3] = {s.f.gunPose[1], s.f.gunPose[5], s.f.gunPose[9]},
                        fore[3] = {s.f.gunPose[2], s.f.gunPose[6], s.f.gunPose[10]};
            for (int k = 0; k < 3; ++k)
                glove[k * 4 + 3] += left[k] * 0.04f + upView[k] * up + fore[k] * along;
            for (int i = 0; i < 40 && !s.r.latched; ++i) {
                std::memcpy(s.f.hand[0].glove, glove, sizeof(glove));
                float middle[3];
                Transform(glove, kMid, middle);
                Transform(s.f.headPose, middle, s.f.hand[0].gripRoom);
                s.Step();
                for (int k = 0; k < 3; ++k)
                    glove[k * 4 + 3] -= left[k] * speed * s.f.dt;
            }
            s.Run(0.2f);
        };
        Sim a, b, c;
        slapAt(0.8f, 0.04f, 0.08f, a);
        slapAt(0.2f, 0.04f, 0.08f, b);
        slapAt(0.8f, 0.0f, -0.07f, c);
        Check(a.r.latched && a.n.shut == 1 && a.n.slapped == 1 && b.n.slapped == 0 && c.n.slapped == 0,
              "34 slap: 4 cm high and 8 cm along at 0.8 m/s: slapped shut and latched (%d); at 0.2 m/s no slap (%d: the push may follow it, %s); behind its back end at 0.8 m/s "
              "no slap (%d)",
              a.r.latched, b.n.slapped, b.r.state, c.n.slapped);
        // The springs opening it into a hand held still 3 cm off its side: no slap.
        Sim d;
        m320::Clear();
        d.Step();
        d.Fire();
        d.OnGripB();
        d.Step();
        const float beside[3] = {m320::kBarrelGrab[0] + 0.03f, m320::kBarrelGrab[1] + 0.02f, m320::kBarrelGrab[2] + 0.05f};
        float p[3];
        OnTube(5.0f * kDegree, beside, p);
        d.Release();
        d.f.hand[0].onGripB = false;
        d.f.hand[0].squeeze = 0.0f;
        PutHandOnGun(d.f, 0, p);
        d.Run(0.6f);
        Check(d.n.shut == 0 && !d.r.latched, "34 slap: the springs opening it into a hand held still beside it: no slap (%s)", d.r.state);
    }

    // ---- 31. Muzzle down, a case half out slides back in, and stops there seated (never past): at 25 degrees it stays ----
    {
        Sim s;
        s.Open();
        s.HandOnRim(0.0f);
        s.Squeeze();
        s.Run(0.2f);
        DrawAlong(s, 0.029f);
        s.LetGo();
        const float out = s.r.roundS;
        TiltExit(s.f, -25.0f * kDegree);
        s.Run(0.5f);
        const float at25 = s.r.roundS;
        TiltExit(s.f, -60.0f * kDegree);
        const float in = s.RunUntil([](const m320::Result& x) { return x.content == m320::Content::kSeated; }, 1.0f);
        const float expected = std::sqrt(2.0f * out / (kG * (std::sin(60.0f * kDegree) - kMu * std::cos(60.0f * kDegree))));
        s.Run(0.5f);
        Check(std::fabs(out - 0.029f) < 0.0005f && std::fabs(at25 - out) < 1e-6f && in > 0.0f && std::fabs(in - expected) <= kDt + 0.002f &&
                  s.r.content == m320::Content::kSeated && s.r.roundS == 0.0f && s.r.roundV == 0.0f && s.n.drop == 0,
              "31 muzzle down: %.1f mm out, 25 degrees down it stays (%.1f mm); 60 degrees down it slides back in, seated in %.3f s (%.3f), and stays", out * 1000.0f,
              at25 * 1000.0f, in, expected);
    }

    // ---- 32. The right hand at work (the rifle in the left): it pinches the rim where its own glove (the left's mirrored) is drawn, and stays on it ----
    {
        Sim s;
        s.f.gunHand = 0;
        s.Open();
        float round[12], onRound[3], target[3];
        m320::RoundOnLine(s.r.barrel, 0.0f, round);
        m320::PinchTarget(onRound);
        onRound[0] = -onRound[0];  // (the right glove's hold: the left's mirrored on the round's bone)
        Transform(round, onRound, target);
        PutHandOnGun(s.f, 1, target);
        s.Squeeze(1);
        const bool pinched = s.r.pinched && s.r.side == 1 && s.r.handState == m320::HandState::kPinch;
        s.Run(0.2f);
        float drawn[3], real[3];
        DrawnMiddle(s.r, 1, drawn);
        HandOnGun(s.f, 1, real);
        Check(pinched && s.r.handState == m320::HandState::kPinch && s.r.gloveBlend == 1.0f && Apart(drawn, real) < 0.001f,
              "32 right hand: the rim pinched at its own hold (%d), still on it 0.2 s later, its glove drawn %.4f m from the hand", pinched, Apart(drawn, real));
    }

    // ---- 33. A pinch fitted by hand (SetHold, as magfit will): the reach point moves with it, so the glove is drawn where the hand took it (the game's put back) ----
    {
        const float identity[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
        float place[12], vanilla[12], fitted[12], fittedInGlove[12], before[3], after[3], restored[3];
        m320::GloveFor(m320::Hold::kPinch, 0, identity, place);  // the game's hold: the glove on the round's bone
        InvertRigid(place, vanilla);
        std::memcpy(fitted, place, sizeof(fitted));
        fitted[11] -= 0.03f;  // the glove 3 cm further back along the round
        InvertRigid(fitted, fittedInGlove);
        m320::PinchTarget(before);
        m320::SetHold(m320::Hold::kPinch, fittedInGlove, nullptr);
        m320::PinchTarget(after);
        Sim s;
        s.Open();
        s.HandOnRim(0.0f);
        s.Squeeze();
        const bool pinched = s.r.pinched;
        s.Run(0.2f);
        float drawn[3], real[3];
        DrawnMiddle(s.r, 0, drawn);
        HandOnGun(s.f, 0, real);
        const float jump = Apart(drawn, real);
        const bool held = s.r.handState == m320::HandState::kPinch && s.r.gloveBlend == 1.0f;
        m320::SetHold(m320::Hold::kPinch, vanilla, nullptr);
        m320::PinchTarget(restored);
        Check(std::fabs(after[0] - before[0]) < 1e-5f && std::fabs(after[1] - before[1]) < 1e-5f && std::fabs(after[2] - before[2] + 0.03f) < 1e-5f && pinched && held &&
                  jump < 0.001f && Apart(restored, before) < 1e-5f,
              "33 fitted pinch: the reach point moved with it (%.4f m along the round), the glove drawn %.4f m from the hand that took it; the game's put back", after[2] - before[2],
              jump);
    }

    // ---- 36. The right's round from the belly (the rifle in the left hand) is the left's flipped: no palms given, its round in the glove is the left's mirrored,
    // its fingers the left's own table. (Before the carry is fitted; after row 35 it's done again with the carry, and mixed.) ----
    const float (*pinchTable)[12] = nullptr;  // the left's pinch as the module gives it (rows 37 and 43 draw palms off its own)
    {
        float inGlove[2][12] = {};
        const float (*table[2])[12] = {};
        bool took[2] = {};
        for (int side = 0; side < 2; ++side) {
            Sim s;
            s.f.gunHand = 1 - side;
            s.OpenEmpty();
            s.TakeAtBelly(side);
            took[side] = s.r.took && s.r.side == side && s.r.handSide == side && s.r.handRound && !s.r.fingersAlt;
            std::memcpy(inGlove[side], s.r.roundInGlove, sizeof(inGlove[side]));
            table[side] = s.r.fingers;
        }
        float mirrored[12];
        Mirror(inGlove[0], mirrored);
        pinchTable = table[0];
        Check(took[0] && took[1] && Differ(inGlove[1], mirrored) <= 1e-6f && table[0] && table[1] == table[0],
              "36 either hand: from the belly the right's round in its glove is the left's mirrored (%.2g off), its fingers the left's own table (%d)",
              Differ(inGlove[1], mirrored), table[0] && table[1] == table[0]);
    }

    // ---- 37. Palms given (each glove's own, in the left glove's layout, as hands.cpp draws them): with the right's palm the left's, its round in the glove and its
    // hand's middle on the rim are the left's mirrored; the other glove's palm changes nothing, and the palm does move the round in the glove ----
    if (pinchTable) {
        const float axisA[3] = {0.48f, 0.80f, 0.36f}, axisB[3] = {-0.60f, 0.0f, 0.80f};
        const float offA[3] = {0.006f, -0.004f, 0.010f}, offB[3] = {-0.012f, 0.005f, 0.002f};
        float palm[12], other[12];
        PalmOff(pinchTable[1], axisA, 6.0f, offA, palm);
        PalmOff(pinchTable[1], axisB, -9.0f, offB, other);
        const float identity[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
        float inGlove[2][12] = {}, onRound[2][3] = {}, plain[12] = {};
        bool took[2] = {};
        for (int side = 0; side < 2; ++side) {
            Sim s;
            s.f.gunHand = 1 - side;
            std::memcpy(s.f.palm[side], palm, sizeof(palm));
            std::memcpy(s.f.palm[1 - side], other, sizeof(other));
            s.f.havePalm[0] = s.f.havePalm[1] = true;
            s.OpenEmpty();
            s.TakeAtBelly(side);
            took[side] = s.r.took && s.r.side == side && s.r.handRound;
            std::memcpy(inGlove[side], s.r.roundInGlove, sizeof(inGlove[side]));
            float glove[12], mid[3];  // (the glove drawn pinching the rim, on the round's own bone: the palms as this Update had them)
            m320::DrawnGloveFor(m320::Hold::kPinch, side, identity, glove);
            MiddleOf(side, mid);
            Transform(glove, mid, onRound[side]);
        }
        {
            Sim s;
            s.OpenEmpty();
            s.TakeAtBelly(0);
            std::memcpy(plain, s.r.roundInGlove, sizeof(plain));
        }
        float mirrored[12];
        Mirror(inGlove[0], mirrored);
        const float flipped[3] = {-onRound[0][0], onRound[0][1], onRound[0][2]};
        Check(took[0] && took[1] && Differ(inGlove[1], mirrored) <= 1e-6f && Differ(onRound[1], flipped, 3) <= 1e-6f && Differ(inGlove[0], plain) > 0.001f,
              "37 palms given: the right's round in its glove the left's mirrored (%.2g off), its middle on the rim too (%.2g m); the palm moved the round in the glove %.1f mm",
              Differ(inGlove[1], mirrored), Differ(onRound[1], flipped, 3), Differ(inGlove[0], plain) * 1000.0f);
    } else {
        Check(false, "37 palms given: no pinch table from row 36");
    }

    // ---- 35. The carry (the user, 2026-11-05: "I just need his thumb and his finger to sort of wrap around the shell"; m320carry.h, fitted on the meshes): a round
    // from the belly is held as carried (by its base) at once; one pulled out by its rim goes over to that once the glove's off the launcher, in kCarryEase; put into the
    // barrel, back to the rim's pinch as the glove goes on. The round in the glove is always where the module catches it. (Last: SetHold stays for the rest.) ----
    {
        m320::SetHold(m320::Hold::kCarry, m320carry::kRoundInGlove, m320carry::kFingers);
        auto same = [](const float* a, const float* b) {
            float most = 0.0f;
            for (int i = 0; i < 12; ++i)
                most = std::fmax(most, std::fabs(a[i] - b[i]));
            return most;
        };
        Sim a;
        a.OpenEmpty();
        a.TakeAtBelly();
        const bool fist = a.r.handRound && a.r.fingersAlt == m320carry::kFingers && a.r.fingersMix > 0.999f && same(a.r.roundInGlove, m320carry::kRoundInGlove) < 1e-5f;
        Sim b;
        b.Open();
        b.HandOnRim(0.0f);
        b.Squeeze();
        b.Run(0.2f);
        b.PullOut();
        const bool pulled = b.r.freed && b.r.fingersMix < 0.001f;
        float overIn = -1.0f, t = 0.0f;
        for (int i = 0; i < 60 && overIn < 0.0f; ++i) {
            b.Step();
            t += b.f.dt;
            if (b.r.fingersMix > 0.999f && same(b.r.roundInGlove, m320carry::kRoundInGlove) < 1e-5f)
                overIn = t;
        }
        Check(fist && pulled && overIn > 0.12f && overIn < 0.12f + 0.15f + 2.0f * kDt,
              "35 carry: from the belly held as carried at once (%d); pulled out by its rim, held as carried %.3f s later (the glove off in 0.12, then 0.15)", fist, overIn);
        Sim c;
        c.OpenEmpty();
        c.TakeAtBelly();
        c.Run(0.1f);
        float mouth[3], inward[3], nose[3], pose[12];
        m320::MouthAt(c.r.barrel, mouth);
        ExitOnGun(c.r.barrel, inward);
        for (float& v : inward)
            v = -v;
        for (int k = 0; k < 3; ++k)
            nose[k] = mouth[k] + inward[k] * 0.01f;
        RoundPose(inward, nose, pose);
        c.HoldRoundAt(pose);  // (the glove placed by the fist's own round in the glove: on as drawn)
        c.Step();
        const bool on = c.r.entered;
        c.Run(0.2f);
        // (Held by its base all the way in, as carried: no going over to the rim's pinch, the hand drawn where it was: the user, 2026-11-05, "inserting it in is awkward".)
        Check(on && c.r.handState == m320::HandState::kPinch && c.r.fingersMix > 0.999f && c.r.hold == m320::Hold::kCarry,
              "35 carry: held as carried and put in: on (%d), still held as carried (%.2f), not the rim's pinch", on, c.r.fingersMix);
        // Turned a quarter about its own length in the hand as it goes in: it keeps that turn on the barrel's line (it snapped to the game's own, up to half a
        // turn, and the hand swung round with it).
        Sim d;
        d.OpenEmpty();
        d.TakeAtBelly();
        d.Run(0.1f);
        float m2[3], in2[3], n2[3], p2[12], spun[12];
        m320::MouthAt(d.r.barrel, m2);
        ExitOnGun(d.r.barrel, in2);
        for (float& v : in2)
            v = -v;
        for (int k = 0; k < 3; ++k)
            n2[k] = m2[k] + in2[k] * 0.01f;
        RoundPose(in2, n2, p2);
        {
            // A quarter turn about the round's own axis (x 0.0003, y -0.0584 on its bone, along +z).
            const float ax = m320::kRoundBaseOnBone[0], ay = m320::kRoundBaseOnBone[1];
            const float turn[12] = {0, -1, 0, ax + ay, 1, 0, 0, ay - ax, 0, 0, 1, 0};
            Compose(p2, turn, spun);
        }
        d.HoldRoundAt(spun);
        d.Step();
        const bool on2 = d.r.entered;
        float handInverse[12], rel[12];
        InvertRigid(spun, handInverse);
        Compose(handInverse, d.r.roundOnGun, rel);  // (the round on the line against the round as the hand had it: the same turn)
        const float off = std::acos(std::fmax(-1.0f, std::fmin(1.0f, (rel[0] + rel[5] + rel[10] - 1.0f) / 2.0f))) / kDegree;
        Check(on2 && d.r.round == m320::Round::kOnGun && off < 2.0f,
              "35 carry: turned a quarter about its length as it goes in: on the line it keeps the hand's turn (%.2f degrees off it), drawn at its own pose", off);
    }

    // ---- 36 (the carry fitted, as the game has it). From the belly the right holds its round as carried at once, the left's mirrored; one pulled out of the barrel by
    // its rim goes over to the carry as the left's does, frame for frame, its round in the glove the left's mirrored all the way (mixed) ----
    {
        float belly[2][12] = {};
        bool carried[2] = {};
        constexpr int kFrames = 40;
        float pulled[2][kFrames][12] = {}, mix[2][kFrames] = {};
        bool freed[2] = {};
        for (int side = 0; side < 2; ++side) {
            Sim s;
            s.f.gunHand = 1 - side;
            s.OpenEmpty();
            s.TakeAtBelly(side);
            carried[side] = s.r.took && s.r.handSide == side && s.r.fingersAlt == m320carry::kFingers && s.r.fingersMix > 0.999f;
            std::memcpy(belly[side], s.r.roundInGlove, sizeof(belly[side]));
            Sim p;
            p.f.gunHand = 1 - side;
            p.Open();
            HandOnRimBy(p, side, 0.0f);
            p.Squeeze(side);
            p.Run(0.2f);
            float exit[3];
            ExitOnGun(m320::kOpenMost, exit);
            for (int i = 0; i < 200 && !p.r.freed; ++i) {
                MoveHandBy(p.f, side, exit, 0.001f);
                p.Step();
            }
            freed[side] = p.r.freed && p.r.handSide == side;
            for (int k = 0; k < kFrames; ++k) {
                p.Step();
                std::memcpy(pulled[side][k], p.r.roundInGlove, sizeof(pulled[side][k]));
                mix[side][k] = p.r.handRound ? p.r.fingersMix : -1.0f;
            }
        }
        float mirrored[12], worst = 0.0f, mixOff = 0.0f;
        Mirror(belly[0], mirrored);
        const float bellyOff = Differ(belly[1], mirrored);
        int mixed = 0;
        for (int k = 0; k < kFrames; ++k) {
            Mirror(pulled[0][k], mirrored);
            worst = std::fmax(worst, Differ(pulled[1][k], mirrored));
            mixOff = std::fmax(mixOff, std::fabs(mix[1][k] - mix[0][k]));
            mixed += mix[0][k] > 0.001f && mix[0][k] < 0.999f ? 1 : 0;
        }
        Check(carried[0] && carried[1] && bellyOff <= 1e-6f && freed[0] && freed[1] && worst <= 1e-6f && mixOff <= 1e-6f && mixed >= 3 && mix[0][kFrames - 1] > 0.999f,
              "36 either hand, the carry fitted: from the belly held as carried, the left's mirrored (%.2g off); pulled out by its rim, over to the carry frame for frame "
              "(%d frames mixed), the left's mirrored throughout (%.2g off, the mix %.2g)",
              bellyOff, mixed, worst, mixOff);
    }

    // ---- 38. The right never takes the barrel's back end (the left's hold there flipped would lie on the gun's right, where the barrel never swings: it shuts it by a
    // slap or a push on its left side, or the flick): its squeeze beside it takes nothing and says why. The left's in the same place takes it ----
    {
        Sim s;
        s.f.gunHand = 0;
        s.Open();
        const float beside[3] = {m320::kBarrelGrab[0] + 0.03f, m320::kBarrelGrab[1], m320::kBarrelGrab[2]};
        float p[3];
        OnTube(m320::kOpenMost, beside, p);
        PutHandOnGun(s.f, 1, p);
        s.Squeeze(1);
        const bool none = !s.r.barrelTaken && !s.r.spentSqueeze && s.r.squeezed && s.r.side == 1 && s.r.handState == m320::HandState::kFree &&
                          std::strstr(s.r.note, "doesn't take its back end") != nullptr;
        const char* note = s.r.note;
        s.Run(0.3f);
        const bool still = s.n.barrelTaken == 0 && s.r.barrelState == m320::Barrel::kOpen && !s.r.latched;
        Sim t;
        t.Open();
        HandBesideBarrel(t, 15.0f);
        t.Squeeze();
        Check(none && still && t.r.barrelTaken, "38 right hand: beside the barrel's back end its squeeze takes nothing (\"%s\"); the left's there takes it (%d)", note,
              t.r.barrelTaken);
    }

    // ---- 39. The right hand slaps the barrel's left side shut, by its own glove's points (the left glove's with x negated): the glove at 1 m/s from 7 cm out, 4 cm
    // off the barrel's line. Its glove is the left's as the game's hand slaps, flipped across its own bind space and over about the line's height (a true turn, the palm
    // still to the gun's right): it slaps and latches in the very frame the left's twin does, 4 cm on the line's other side ----
    {
        auto slapBy = [](int side, Sim& s) {
            const float kLineY = -0.0363f, kSurface = 0.0483f + 0.009f;  // the barrel's line and its outer left side (m320.cpp kMouthShut, kSurfaceX)
            s.f.gunHand = 1 - side;
            s.OpenEmpty();
            s.f.hand[side].touches = 0;
            float left[12], glove[12];
            m320::GloveFor(m320::Hold::kBarrel, 0, s.r.tubeOnGun, left);  // the left glove as the game's hand slaps it, on the gun
            if (side == 1) {
                const float acrossX[12] = {-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0}, overLine[12] = {1, 0, 0, 0, 0, -1, 0, 2.0f * kLineY, 0, 0, 1, 0};
                float flipped[12];
                Compose(left, acrossX, flipped);
                Compose(overLine, flipped, glove);
            } else {
                std::memcpy(glove, left, sizeof(glove));
            }
            const float palmInGlove[3] = {side == 1 ? -m320::kSlapPointInGlove[0] : m320::kSlapPointInGlove[0], m320::kSlapPointInGlove[1], m320::kSlapPointInGlove[2]};
            const float startLocal[3] = {kSurface + 0.07f, kLineY + (side == 1 ? -0.04f : 0.04f), 0.60f};
            float palm[3], start[3];
            Transform(glove, palmInGlove, palm);
            OnTube(s.r.barrel, startLocal, start);
            for (int k = 0; k < 3; ++k)
                glove[k * 4 + 3] += start[k] - palm[k];
            float view[12];
            Compose(s.f.gunPose, glove, view);
            const float leftWay[3] = {s.f.gunPose[0], s.f.gunPose[4], s.f.gunPose[8]};  // the gun's x (its left) in the view
            int at = -1;
            for (int i = 0; i < 40 && !s.r.latched; ++i) {
                SetGlove(s.f, side, view);
                s.Step();
                if (s.r.slapped && at < 0)
                    at = i;
                for (int k = 0; k < 3; ++k)
                    view[k * 4 + 3] -= leftWay[k] * 1.0f * s.f.dt;
            }
            s.Run(0.2f);
            return at;
        };
        Sim a, b;
        const int leftAt = slapBy(0, a), rightAt = slapBy(1, b);
        Check(b.n.slapped == 1 && b.n.latchedNow == 1 && b.r.latched && b.r.side == 1 && rightAt >= 0 && rightAt == leftAt && a.n.slapped == 1 && a.r.latched,
              "39 right hand: its palm slaps the barrel's left side shut (slapped %d, latched %d) in frame %d of the swing, as the left's twin (frame %d)", b.n.slapped,
              b.r.latched, rightAt, leftAt);
    }

    // ---- 40. The flick with the rifle in the left hand: its wrist's move is the mirror image (flickWay -1). "m320test 6"'s scripted roll (to the right at 0.5 s of
    // every 3, to the left at 1.5 s; the only flick that opens it) opens it at the right roll and shuts it at the left one a second later with flickWay 1 (today's);
    // with -1 it opens at the left roll and shuts at the right one two seconds later. With the flick's opening off (the default), an open barrel shuts at a real roll to
    // the right with -1 and to the left with 1; the other roll does nothing, and shut, the mirrored roll opens nothing ----
    {
        Sim s;
        m320::Clear();
        s.Step();
        s.Fire();
        s.f.test = 6;
        float open1 = -1.0f, shut1 = -1.0f, open2 = -1.0f, shut2 = -1.0f;
        for (int i = 0; i < 11 * 72 && shut2 < 0.0f; ++i) {
            s.Step();
            if (open1 < 0.0f) {
                if (s.r.flickOpen)
                    open1 = s.time;
            } else if (shut1 < 0.0f) {
                if (s.r.flickShut) {
                    shut1 = s.time;
                    s.f.flickWay = -1;  // (the rifle into the left hand)
                    s.f.gunHand = 0;
                }
            } else if (open2 < 0.0f) {
                if (s.r.flickOpen)
                    open2 = s.time;
            } else if (s.r.flickShut) {
                shut2 = s.time;
            }
        }
        s.f.test = 0;
        const bool scripted = open1 >= 0.0f && shut2 >= 0.0f && std::fabs(shut1 - open1 - 1.0f) < 0.05f && std::fabs(open2 - open1 - 4.0f) < 0.05f &&
                              std::fabs(shut2 - open1 - 6.0f) < 0.05f && s.n.flickOpen == 2 && s.n.flickShut == 2;
        auto rolled = [](int way, float rate, bool open, Sim& t) {
            t.f.flickWay = way;
            t.f.gunHand = way < 0 ? 0 : 1;
            if (open) {
                t.Open();
            } else {
                m320::Clear();
                t.Step();
                t.Fire();
            }
            t.Run(0.7f);
            const float z[3] = {0.0f, 0.0f, 1.0f};
            float turn[12];
            Rotation(z, rate * t.f.dt, turn);  // 600 degrees/s for 4 frames: 33 degrees
            for (int i = 0; i < 4; ++i) {
                TurnGunOwn(t.f, turn);
                t.Step();
            }
            t.Run(0.4f);
        };
        Sim a, b, c, d, e;
        rolled(-1, 10.47f, true, a);   // the rifle in the left hand, rolled to its right: shut
        rolled(-1, -10.47f, true, b);  // ... to its left: nothing
        rolled(1, -10.47f, true, c);   // in the right hand, rolled to its left: shut (today's)
        rolled(1, 10.47f, true, d);    // ... to its right: nothing
        rolled(-1, -10.47f, false, e);  // in the left hand, spent and shut, rolled to its left (its mirrored open): nothing
        const bool real = a.n.flickShut == 1 && a.r.latched && b.n.flickShut == 0 && !b.r.latched && c.n.flickShut == 1 && c.r.latched && d.n.flickShut == 0 &&
                          !d.r.latched && e.r.latched && a.n.flickOpen + b.n.flickOpen + c.n.flickOpen + d.n.flickOpen + e.n.flickOpen == 0;
        Check(scripted, "40 flick mirrored: the scripted roll with flickWay 1 opens at %.2f s and shuts %.2f s later (1, the left roll); with -1 opens %.2f s and shuts %.2f s "
                        "after that first open (4 and 6: the left roll opens, the right shuts)",
              open1, shut1 - open1, open2 - open1, shut2 - open1);
        Check(real, "40 flick mirrored, its opening off: open, the rifle in the left hand, a roll to the right shuts it (%d), to the left nothing (%d); in the right hand the left "
                    "shuts it (%d), the right nothing (%d); shut, the mirrored roll opens nothing (%d)",
              a.n.flickShut, b.n.flickShut, c.n.flickShut, d.n.flickShut, e.n.flickOpen);
    }

    // ---- 41. The release by the hand on the launcher's grip, whichever it is. The rifle held by the launcher's grip alone (the left on it, gunHand 0: the right at
    // work): a release press is the left's index (gripSide 0), on the release while its trigger is held, off once it's under 0.4, and meanwhile the right takes a round
    // at the belly. The rifle in the right hand by that grip alone (gunHand 1, the left at work): the right's (gripSide 1). Nobody on it: -1 ----
    {
        Sim s;
        s.f.gunHand = 0;
        m320::Clear();
        s.Step();
        s.Fire();
        const bool nobody = s.r.gripSide == -1 && s.r.indexOnRelease == 0.0f;
        s.OnGripB();  // (the left on the launcher's grip)
        s.Step();
        const bool idle = s.r.gripSide == 0 && s.r.indexOnRelease == 0.0f && s.r.side == 1;
        s.f.releasePress = true;
        s.f.hand[0].trigger = 1.0f;
        s.Step();
        const bool pressed = s.r.released && s.r.gripSide == 0 && s.r.indexOnRelease == 1.0f && std::fabs(s.r.releaseLift - 0.004f) < 1e-6f && s.r.trigger == 0.0f;
        s.Run(0.3f);
        s.TakeAtBelly(1);
        const bool took = s.r.took && s.r.side == 1 && s.r.handSide == 1 && s.r.gripSide == 0 && s.r.indexOnRelease == 1.0f;
        s.f.hand[0].trigger = 0.39f;
        s.Step();
        const bool off = s.r.gripSide == 0 && s.r.indexOnRelease == 0.0f && s.r.releaseLift == 0.0f && std::fabs(s.r.trigger - 0.39f) < 1e-6f && s.r.handRound;
        Sim t;
        m320::Clear();
        t.Step();
        t.Fire();
        t.f.hand[1].onGripB = true;  // (the right on the launcher's grip, the left at work)
        t.f.hand[1].squeeze = 1.0f;
        t.Step();
        t.f.releasePress = true;
        t.f.hand[1].trigger = 1.0f;
        t.Step();
        const bool right = t.r.released && t.r.side == 0 && t.r.gripSide == 1 && t.r.indexOnRelease == 1.0f;
        t.f.hand[1].trigger = 0.0f;
        t.Step();
        const bool rightOff = t.r.gripSide == 1 && t.r.indexOnRelease == 0.0f;
        Check(nobody && idle && pressed && took && off && right && rightOff,
              "41 the hand on its grip: the left's index on the release (%d), held there while the right takes a round (%d), off under 0.4 (%d); the right's on its grip "
              "(%d, off %d); nobody on it -1 (%d)",
              pressed, took, off, right, rightOff, nobody);
    }

    // ---- 42. TestOpenWithCase (free hands' film rounds): spent and shut, opened with the spent case in it, the count unchanged; loaded, the live round given back
    // first; a round in the hand let go of (back), the barrel already open given the case ----
    {
        Sim a;
        m320::Clear();
        a.Step();
        a.Fire();
        const int loaded0 = a.loaded, spare0 = a.spare;
        m320::TestOpenWithCase();
        a.Step();
        const bool opened = a.r.released && !a.r.unload && std::strcmp(a.r.note, "the test's round: a spent case in, opened") == 0;
        const float stop = a.RunUntil([](const m320::Result& x) { return x.openStop; }, 0.5f);
        const bool spentIn = stop > 0.0f && a.r.barrelState == m320::Barrel::kOpen && a.r.content == m320::Content::kSeated && !a.r.roundLive && !a.r.latched &&
                             a.loaded == loaded0 && a.spare == spare0 && a.n.unload == 0;
        Sim b;
        m320::Clear();
        b.Step();  // loaded
        const int spareB = b.spare;
        m320::TestOpenWithCase();
        b.Step();
        const bool backFirst = b.r.unload && b.r.released && b.loaded == 0 && b.spare == spareB + 1 && b.r.content == m320::Content::kSeated && !b.r.roundLive;
        b.Run(0.4f);
        const bool backOpen = b.n.fired == 0 && b.n.unload == 1 && b.r.barrelState == m320::Barrel::kOpen && b.loaded == 0 && !b.r.fireReady;
        Sim c;
        c.OpenEmpty();
        c.TakeAtBelly();
        const Counts beforeC = c.n;
        m320::TestOpenWithCase();
        c.Step();
        const bool handBack = c.r.cancelled && !c.r.handRound && c.r.handState == m320::HandState::kFree && c.r.content == m320::Content::kSeated && !c.r.roundLive &&
                              c.r.barrelState == m320::Barrel::kOpen && c.n.released == beforeC.released && c.n.drop == beforeC.drop && !c.r.unload;
        Check(opened && spentIn && backFirst && backOpen && handBack,
              "42 TestOpenWithCase: spent, opened with the case in (%d, at its stop %.3f s), the count unchanged; loaded, given back first (%d, open %d); a round in the hand "
              "back, the open barrel given the case (%d)",
              opened, stop, backFirst, backOpen, handBack);
    }

    // ---- 43. DrawnGloveFor (a scripted hand's place: freeplaces.cpp's kRim): the glove there has its middle exactly where the module reaches for the rim (the hold
    // re-anchored on that side's palm, worked out here apart), both hands, palms given and not; the hand put there takes the rim, and the glove is drawn (as hands.cpp
    // draws it) just where it is ----
    if (pinchTable) {
        const float axis[3] = {0.48f, 0.80f, 0.36f}, offset[3] = {0.006f, -0.004f, 0.010f};
        float palm[12];
        PalmOff(pinchTable[1], axis, 6.0f, offset, palm);
        const float identity[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
        float worst = 0.0f, worstJump = 0.0f;
        bool taken = true;
        for (int given = 0; given < 2; ++given) {
            for (int side = 0; side < 2; ++side) {
                Sim s;
                s.f.gunHand = 1 - side;
                if (given) {
                    for (int k = 0; k < 2; ++k) {
                        std::memcpy(s.f.palm[k], palm, sizeof(palm));
                        s.f.havePalm[k] = true;
                    }
                    s.f.palm[1 - side][3] += 0.02f;  // (the other glove's palm 2 cm off: a mix-up would show)
                }
                s.Open();
                float place[12], rebase[12], inverse[12], glove[12], mid[3], target[3], drawnGlove[12], got[3];
                m320::GloveFor(m320::Hold::kPinch, side, identity, place);
                Identity(rebase);
                if (given) {
                    float tableInverse[12], left[12];
                    InvertAny(pinchTable[1], tableInverse);
                    Compose(s.f.palm[side], tableInverse, left);  // (in the left glove's layout: the right's mirrored after)
                    if (side == 1)
                        Mirror(left, rebase);
                    else
                        std::memcpy(rebase, left, sizeof(rebase));
                }
                InvertAny(rebase, inverse);
                Compose(place, inverse, glove);
                MiddleOf(side, mid);
                Transform(glove, mid, target);
                m320::DrawnGloveFor(m320::Hold::kPinch, side, identity, drawnGlove);
                Transform(drawnGlove, mid, got);
                worst = std::fmax(worst, std::isfinite(Apart(got, target)) ? Apart(got, target) : 1.0f);
                // The hand put there, the round in the open barrel as drawn: it takes the rim, and its glove is drawn where it is.
                float round[12], roundView[12], handGlove[12], middle[3];
                m320::RoundOnLine(s.r.barrel, 0.0f, round);
                Compose(s.f.gunPose, round, roundView);
                m320::DrawnGloveFor(m320::Hold::kPinch, side, roundView, handGlove);
                SetGlove(s.f, side, handGlove);
                Transform(handGlove, mid, middle);
                s.Squeeze(side);
                const bool pinched = s.r.pinched && s.r.side == side;
                s.Run(0.2f);
                float onView[12], drawn[12], drawnMiddle[3];
                Compose(s.f.gunPose, s.r.gloveOnGun, onView);
                Compose(onView, inverse, drawn);
                Transform(drawn, mid, drawnMiddle);
                worstJump = std::fmax(worstJump, std::isfinite(Apart(drawnMiddle, middle)) ? Apart(drawnMiddle, middle) : 1.0f);
                taken = taken && pinched && s.r.handState == m320::HandState::kPinch && s.r.gloveBlend == 1.0f;
            }
        }
        Check(worst <= 1e-5f && worstJump <= 1e-5f && taken,
              "43 DrawnGloveFor: the hand's middle %.2g m from the module's reach for the rim, either hand, palms given and not; put there it takes the rim (%d) and is drawn "
              "%.2g m from where it is",
              worst, taken, worstJump);
    } else {
        Check(false, "43 DrawnGloveFor: no pinch table from row 36");
    }

    // ---- 44. The right's rim and push home (free hands' film round 7), the left's twin beside it: a spent case put in (TestOpenWithCase), the right pinches its rim and
    // draws it 10 cm out (moving), pushes it back in to 4 cm (caught past half way, riding it home with the push hold: the fingers mixed to the game's push, the glove on
    // the round the left's mirrored; then seated), pinches it again and pulls it out past its clear (freed), and lets go (a drop, side 1). Frame for frame the left's ----
    {
        struct RimFrame {
            unsigned events = 0;
            float s = 0.0f, mix = 0.0f, blend = 0.0f;
            int hold = 0, state = 0;
            const float (*fingers)[12] = nullptr;
            const float (*alt)[12] = nullptr;
            bool haveRel = false, handRound = false;
            float rel[12] = {}, inGlove[12] = {};
        };
        constexpr int kMost = 600;
        static RimFrame rec[2][kMost];
        int count[2] = {};
        Counts totals[2];
        int dropSide[2] = {-1, -1};
        bool dropLive[2] = {true, true}, dropFinite[2] = {};
        for (int side = 0; side < 2; ++side) {
            Sim s;
            s.f.gunHand = 1 - side;
            m320::Clear();
            s.Step();
            s.Fire();
            m320::TestOpenWithCase();
            s.Step();
            s.RunUntil([](const m320::Result& x) { return x.openStop; }, 0.5f);
            auto step = [&]() {
                s.Step();
                if (count[side] >= kMost)
                    return;
                RimFrame& k = rec[side][count[side]++];
                const m320::Result& x = s.r;
                k.events = (x.pinched ? 1u : 0u) | (x.contentMoving ? 2u : 0u) | (x.caught ? 4u : 0u) | (x.caseSeated ? 8u : 0u) | (x.freed ? 16u : 0u) |
                           (x.letGo ? 32u : 0u) | (x.drop ? 64u : 0u) | (x.unload ? 128u : 0u) | (x.seated ? 256u : 0u);
                k.s = x.roundS, k.mix = x.fingersMix, k.blend = x.gloveBlend;
                k.hold = static_cast<int>(x.hold), k.state = static_cast<int>(x.handState);
                k.fingers = x.fingers, k.alt = x.fingersAlt;
                k.haveRel = x.round != m320::Round::kHidden && x.gloveBlend > 0.0f;
                if (k.haveRel) {
                    float inverse[12];
                    InvertRigid(x.roundOnGun, inverse);
                    Compose(inverse, x.gloveOnGun, k.rel);
                }
                k.handRound = x.handRound;
                if (x.handRound)
                    std::memcpy(k.inGlove, x.roundInGlove, sizeof(k.inGlove));
                if (x.drop) {
                    dropSide[side] = x.side;
                    dropLive[side] = x.dropLive;
                    dropFinite[side] = Finite12(x.dropView) && std::isfinite(x.dropVelocityRoom[0] + x.dropVelocityRoom[1] + x.dropVelocityRoom[2]);
                }
            };
            float exit[3];
            ExitOnGun(m320::kOpenMost, exit);
            HandOnRimBy(s, side, 0.0f);
            s.f.hand[side].squeeze = 0.0f;
            step();
            s.f.hand[side].squeeze = 1.0f;
            step();  // the rim pinched
            for (int i = 0; i < 100; ++i) {  // drawn 10 cm out
                MoveHandBy(s.f, side, exit, 0.001f);
                step();
            }
            for (int i = 0; i < 60; ++i) {  // pushed back in to 4 cm: past half way caught, home by itself, the hand riding it
                MoveHandBy(s.f, side, exit, -0.001f);
                step();
            }
            s.f.hand[side].squeeze = 0.0f;  // let go of
            for (int i = 0; i < 5; ++i)
                step();
            HandOnRimBy(s, side, 0.0f);  // back on the rim
            for (int i = 0; i < 5; ++i)
                step();
            s.f.hand[side].squeeze = 1.0f;  // pinched again
            step();
            for (int i = 0; i < 200 && !s.r.freed; ++i) {  // pulled out past its clear
                MoveHandBy(s.f, side, exit, 0.001f);
                step();
            }
            for (int i = 0; i < 20; ++i)  // in the hand a moment
                step();
            s.f.hand[side].squeeze = 0.0f;  // let go of: it falls
            step();
            totals[side] = s.n;
        }
        // The right's own run: the steps in order.
        const Counts& n = totals[1];
        const bool run = n.pinched == 2 && n.contentMoving == 2 && n.caught == 1 && n.caseSeated == 1 && n.freed == 1 && n.letGo == 1 && n.drop == 1 && n.unload == 0 &&
                         n.seated == 0 && dropSide[1] == 1 && !dropLive[1] && dropFinite[1];
        // Riding it home: the push hold, its fingers mixed to the game's push (a table of its own, not the pinch's).
        int riding = 0;
        bool pushHold = true;
        for (int i = 0; i < count[1]; ++i) {
            const RimFrame& k = rec[1][i];
            if (k.state != static_cast<int>(m320::HandState::kRiding))
                continue;
            ++riding;
            pushHold = pushHold && k.hold == static_cast<int>(m320::Hold::kPush) && k.alt && k.alt != k.fingers && k.mix >= 0.0f;
        }
        // Frame for frame the left's: the same events, the case as far out, the same hold and fingers, the glove on the round and the case in the glove mirrored.
        bool same = count[0] == count[1];
        float sOff = 0.0f, relOff = 0.0f, inGloveOff = 0.0f, mixOff = 0.0f;
        int firstOff = -1;
        for (int i = 0; i < count[0] && i < count[1]; ++i) {
            const RimFrame &l = rec[0][i], &r = rec[1][i];
            bool ok = l.events == r.events && l.hold == r.hold && l.state == r.state && l.fingers == r.fingers && l.alt == r.alt && l.haveRel == r.haveRel &&
                      l.handRound == r.handRound && std::fabs(l.blend - r.blend) <= 1e-6f;
            sOff = std::fmax(sOff, std::fabs(l.s - r.s));
            mixOff = std::fmax(mixOff, std::fabs(l.mix - r.mix));
            float mirrored[12];
            if (l.haveRel && r.haveRel) {
                Mirror(l.rel, mirrored);
                relOff = std::fmax(relOff, Differ(r.rel, mirrored));
            }
            if (l.handRound && r.handRound) {
                Mirror(l.inGlove, mirrored);
                inGloveOff = std::fmax(inGloveOff, Differ(r.inGlove, mirrored));
            }
            if (!ok && firstOff < 0)
                firstOff = i;
            same = same && ok;
        }
        Check(run && riding >= 3 && pushHold, "44 right hand's rim: pinched %d, moving %d, caught %d, a case seated %d, freed %d, let go %d (side %d, a case); %d frames riding "
                                              "it home with the push hold (%d)",
              n.pinched, n.contentMoving, n.caught, n.caseSeated, n.freed, n.letGo, dropSide[1], riding, pushHold);
        Check(same && sOff <= 1e-6f && mixOff <= 1e-6f && relOff <= 1e-5f && inGloveOff <= 1e-6f && dropSide[0] == 0,
              "44 right hand's rim: frame for frame the left's (%d and %d frames, first apart %d): the case %.2g m apart, the glove on it the left's mirrored (%.2g), the case in "
              "the glove too (%.2g), the fingers' mix %.2g apart",
              count[0], count[1], firstOff, sOff, relOff, inGloveOff, mixOff);
    }

    // ---- 45. A round turned into line as the hand draws back along the barrel (free hands' film round 7, the right at 15.4-15.9 s: the wrist swings the round's
    // nose into the back end while the hand's middle, 20 cm behind it in the carry, goes out; twice in the user's own test with the left it "went in and straight
    // back out"). Turned 40 degrees to none about a point 5 cm behind its nose in 0.35 s, drawn 2 cm out as it ends, held, then pushed in at 0.25 m/s: never pulled
    // free, home by the push. Either hand ----
    {
        int home[2] = {}, freed[2] = {};
        const float kTurnTime = 0.35f;
        for (int side = 0; side < 2; ++side) {
            Sim s;
            s.f.gunHand = 1 - side;
            s.OpenEmpty();
            s.TakeAtBelly(side);
            s.Run(0.1f);
            float mouth[3], in[3];
            m320::MouthAt(s.r.barrel, mouth);
            ExitOnGun(s.r.barrel, in);
            for (float& v : in)
                v = -v;
            const float up[3] = {0.0f, 1.0f, 0.0f};
            float inGlove[12], inverse[12];  // (the round in the glove as the module has it: the carry, unchanged all the way)
            std::memcpy(inGlove, s.r.roundInGlove, sizeof(inGlove));
            InvertAny(inGlove, inverse);
            // The round held turned theta from the barrel's line (its nose in and up), about a point on the line 7 cm behind the back end and 5 cm behind its nose,
            // that point moved in along the line by along.
            auto hold = [&](float theta, float along) {
                float axis[3], nose[3], pose[12], view[12], glove[12];
                for (int k = 0; k < 3; ++k) {
                    axis[k] = in[k] * std::cos(theta) + up[k] * std::sin(theta);
                    nose[k] = mouth[k] + in[k] * (-0.07f + along) + axis[k] * 0.05f;
                }
                RoundPose(axis, nose, pose);
                Compose(s.f.gunPose, pose, view);
                Compose(view, inverse, glove);
                SetGlove(s.f, side, glove);
            };
            float lastAlong = 0.0f;
            const int turnFrames = static_cast<int>(kTurnTime / kDt + 0.5f);
            auto note = [&]() { freed[side] += s.r.freed ? 1 : 0; };
            hold(40.0f * kDegree, 0.0f);
            s.Step();
            for (int k = 1; k <= turnFrames; ++k) {  // turned into line, the last of it drawn out 2 cm
                const float u = static_cast<float>(k) / static_cast<float>(turnFrames);
                const float theta = 40.0f * kDegree * (1.0f - u);
                lastAlong = -0.02f * u * u * u;
                hold(theta, lastAlong);
                s.Step();
                note();
            }
            for (int k = 0; k < 7; ++k) {  // held a moment
                s.Step();
                note();
            }
            for (int k = 1; k <= 58 && !s.r.seated; ++k) {  // pushed in at 0.25 m/s
                hold(0.0f, lastAlong + 0.25f * kDt * static_cast<float>(k));
                s.Step();
                note();
            }
            s.Run(0.2f);
            home[side] = s.n.seated;
        }
        Check(freed[0] == 0 && freed[1] == 0 && home[0] == 1 && home[1] == 1,
              "45 turned into line as the hand draws back: never pulled free (left %d, right %d), home by the push (left %d, right %d)", freed[0], freed[1], home[0],
              home[1]);
    }

    // ---- 46. A round put in still turned, then levered into line about its nose (the hand's middle swinging back along the barrel, its nose staying at the back
    // end): not pulled free by the swing; the push after takes it home. Either hand ----
    {
        int home[2][2] = {}, freed[2][2] = {}, entered[2][2] = {};
        const float startDegrees[2] = {30.0f, 34.0f};
        for (int which = 0; which < 2; ++which)
            for (int side = 0; side < 2; ++side) {
                Sim s;
                s.f.gunHand = 1 - side;
                s.OpenEmpty();
                s.TakeAtBelly(side);
                s.Run(0.1f);
                float mouth[3], in[3];
                m320::MouthAt(s.r.barrel, mouth);
                ExitOnGun(s.r.barrel, in);
                for (float& v : in)
                    v = -v;
                const float up[3] = {0.0f, 1.0f, 0.0f};
                float inGlove[12], inverse[12];
                std::memcpy(inGlove, s.r.roundInGlove, sizeof(inGlove));
                InvertAny(inGlove, inverse);
                // Its nose on the line deep along the barrel from its back end (below 0: outside), the round turned theta (its nose in and up).
                auto hold = [&](float theta, float deep) {
                    float axis[3], nose[3], pose[12], view[12], glove[12];
                    for (int k = 0; k < 3; ++k) {
                        axis[k] = in[k] * std::cos(theta) + up[k] * std::sin(theta);
                        nose[k] = mouth[k] + in[k] * deep;
                    }
                    RoundPose(axis, nose, pose);
                    Compose(s.f.gunPose, pose, view);
                    Compose(view, inverse, glove);
                    SetGlove(s.f, side, glove);
                };
                const float start = startDegrees[which] * kDegree;
                hold(start, -0.012f);
                s.Step();
                float deep = -0.012f;
                for (int k = 0; k < 3 && !s.r.entered; ++k) {  // pushed in at 0.15 m/s: on
                    deep += 0.15f * kDt;
                    hold(start, deep);
                    s.Step();
                }
                entered[which][side] = s.r.entered ? 1 : 0;
                const int frames = static_cast<int>(0.3f / kDt + 0.5f);
                for (int k = 1; k <= frames; ++k) {  // levered into line about its nose in 0.3 s
                    hold(start * (1.0f - static_cast<float>(k) / static_cast<float>(frames)), deep);
                    s.Step();
                    freed[which][side] += s.r.freed ? 1 : 0;
                }
                for (int k = 1; k <= 60 && !s.r.seated; ++k) {  // pushed in at 0.25 m/s
                    hold(0.0f, deep + 0.25f * kDt * static_cast<float>(k));
                    s.Step();
                    freed[which][side] += s.r.freed ? 1 : 0;
                }
                s.Run(0.2f);
                home[which][side] = s.n.seated;
            }
        Check(entered[0][0] && entered[0][1] && entered[1][0] && entered[1][1] && freed[0][0] + freed[0][1] + freed[1][0] + freed[1][1] == 0 &&
                  home[0][0] + home[0][1] + home[1][0] + home[1][1] == 4,
              "46 put in turned 30 and 34 degrees, levered into line about its nose: never pulled free (%d %d %d %d), home by the push (%d %d %d %d)", freed[0][0],
              freed[0][1], freed[1][0], freed[1][1], home[0][0], home[0][1], home[1][0], home[1][1]);
    }

    // ---- 20. The time a frame takes, mid-reload ----
    {
        Sim s;
        s.Open();
        s.HandOnRim(0.0f);
        s.Squeeze();
        const auto start = std::chrono::steady_clock::now();
        const int frames = 20000;
        for (int i = 0; i < frames; ++i)
            m320::Update(s.f);
        const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count() / frames;
        Check(us < 20.0, "20 time: %.2f microseconds a frame", us);
    }

    std::printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed;
}

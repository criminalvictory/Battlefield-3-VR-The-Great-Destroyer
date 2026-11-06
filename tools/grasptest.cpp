// The hold machine (src/grasp.h: who holds the rifle with free hands, as hands.cpp runs it) on scripted frames
// at 72 Hz, without the game. The room is OpenXR's (x right, y up, z back, metres): the head at (0, 1.2, 0)
// facing -z, the right grip 18 cm right, 25 cm down and 35 cm ahead of it, the left grip its mirror image, the
// rifle placed in a hand by that side's pistol-grip point, or lying on its side on the floor 60 cm ahead.
// Every row runs twice: as written, and mirrored (the hands swapped, every pose S m S with S = diag(-1, 1, 1),
// each side's pistol-grip point and the hull's box mirrored, the machine started from the mirror of the spawn),
// its results read back with the sides swapped. The same checks hold for both ("m" after a mirrored check's
// row), and the two runs agree frame by frame ("=") up to a return to the right hand, which is the right in
// both. Every frame of every row is also held to the states' rules ("i").
//   1  the spawn: held by the right's pistol grip, its trigger firing it, nothing armed
//   2  the right never squeezed, its grip open for 10 s, or half squeezed (0.59): never let go
//   3  squeezed to 0.7 arms it; 0.3 for 2 frames keeps it, the 3rd lets go: in the world, nothing fires; the
//      count starting again; 0.35 exactly isn't under the let-go, 0.6 exactly arms
//   4  its grip not read (headset off, controller asleep) after arming: never let go, disarmed; read again
//      open: still held; an unread frame in the middle of the count
//   5  the stand-in's hand (not its own controller): squeezed and opened, never armed; the real one back open
//   6  out of tracking past the hold after arming: disarmed, never let go
//   7  let go with the other hand on the handguard: held by that hold alone; that hand not seen: it falls
//   8  held by the handguard alone, that hand lets go: it falls from it (not before the rifle is drawn)
//   9  held by the handguard alone, the free hand squeezes 10 cm from its pistol grip: a regrip, gliding in
//      over 0.12 s, the barrel hand kept on, firing only once its trigger is under 0.4
//   10 the same 30 cm away: nothing; the edge at 15 cm
//   11 lying, a fresh squeeze 20 cm from its box: picked up, half way at 0.06 s, in the hand at 0.12 s, armed;
//      by either hand; the edge at 25 cm
//   12 never taken by a held (not fresh) squeeze, one held since the spawn, 40 cm away, a busy hand, one not
//      tracked, the stand-in's, one not read, within 0.25 s of its own let-go (the other hand can)
//   13 both hands fresh in one frame: the nearer takes it
//   14 a trigger held through a pickup: nothing fires until it's under 0.4
//   15 let go mid-glide: a pickup's falls; a regrip's stays in the barrel hand's hold (or falls with it gone)
//   16 a new life from every state: back in the right hand, nothing armed, returned unless it was there
//   17 free hands off (another gun, seated, off) from lying, the left's and the right's pistol grip
//   18 lost on the ground: back in the right hand
//   19 a scripted moment: no let-go, no take, no regrip, a glide finishes; after it an open grip keeps it
//   20 the loose start refused after a let-go: back in the hand or the hold it left, unarmed
//   21 the game's reload with the grip open: disarmed, no drop during it nor after it
//   22 NaN and infinities in every input, and in each alone: nothing changes hands, every number finite
//   23 a fresh squeeze spent only on the frame it takes the rifle
//   24 the game's aim: at the head within 0.25 s of a drop, back to the gun over a pickup's glide
//   25 freehands.txt's tests 1, 2 and 3 (src/freetest.h) through the machine with the hands as hands.cpp places
//      them, two rounds each: every expected event, in order, within 2 frames; the scripts' own steps
//   26 the time: 1e5 updates under 50 ms

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <limits>
#include <string>
#include <vector>

#include "../src/freetest.h"
#include "../src/grasp.h"

namespace {

using freetest::Where;
using grasp::Holder;

constexpr float kDt = 1.0f / 72.0f;
constexpr int L = 0, R = 1;
// src/gunbody.h's numbers: the right palm's point on the rifle (skin::GunGrip, this run's) and the hull's box (model).
constexpr float kAnchor[3] = {-0.063f, -0.091f, 0.224f};
constexpr float kHullMin[3] = {-0.030f, -0.180f, -0.230f}, kHullMax[3] = {0.030f, 0.150f, 0.776f};

// ---- Reporting ----
int g_failed = 0, g_counted = 0;
bool g_mirrored = false;  // the run being checked

void Check(const char* id, const char* what, bool pass, const char* format = "", ...) {
    char numbers[2048];
    va_list args;
    va_start(args, format);
    std::vsnprintf(numbers, sizeof(numbers), format, args);
    va_end(args);
    char tag[16];
    std::snprintf(tag, sizeof(tag), "%s%s", id, g_mirrored ? "m" : "");
    std::printf("%-5s %-4s %s%s%s\n", tag, pass ? "PASS" : "FAIL", what, *numbers ? " | " : "", numbers);
    ++g_counted;
    g_failed += !pass;
}

bool Near(float a, float b, float within = 1e-5f) { return std::fabs(a - b) <= within; }

// ---- Poses: three rows, rotation | translation ----
struct V3 {
    float x, y, z;
};
struct Pose {
    float m[12];
};

Pose Identity() { return {{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0}}; }

Pose Mul(const Pose& a, const Pose& b) {
    Pose o{};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 4; ++j)
            o.m[i * 4 + j] = a.m[i * 4] * b.m[j] + a.m[i * 4 + 1] * b.m[4 + j] + a.m[i * 4 + 2] * b.m[8 + j] +
                             (j == 3 ? a.m[i * 4 + 3] : 0.0f);
    return o;
}

// Degrees about x, then y, then z.
Pose Turn(float xDeg, float yDeg, float zDeg) {
    const float k = 3.14159265f / 180.0f;
    const float cx = std::cos(xDeg * k), sx = std::sin(xDeg * k), cy = std::cos(yDeg * k), sy = std::sin(yDeg * k),
                cz = std::cos(zDeg * k), sz = std::sin(zDeg * k);
    const Pose rx = {{1, 0, 0, 0, 0, cx, -sx, 0, 0, sx, cx, 0}};
    const Pose ry = {{cy, 0, sy, 0, 0, 1, 0, 0, -sy, 0, cy, 0}};
    const Pose rz = {{cz, -sz, 0, 0, sz, cz, 0, 0, 0, 0, 1, 0}};
    return Mul(rz, Mul(ry, rx));
}

Pose Moved(Pose p, V3 at) {
    p.m[3] = at.x, p.m[7] = at.y, p.m[11] = at.z;
    return p;
}

V3 Pos(const Pose& p) { return {p.m[3], p.m[7], p.m[11]}; }

V3 Apply(const Pose& p, V3 v) {
    return {p.m[0] * v.x + p.m[1] * v.y + p.m[2] * v.z + p.m[3], p.m[4] * v.x + p.m[5] * v.y + p.m[6] * v.z + p.m[7],
            p.m[8] * v.x + p.m[9] * v.y + p.m[10] * v.z + p.m[11]};
}

Pose Inv(const Pose& p) {
    Pose o{};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            o.m[i * 4 + j] = p.m[j * 4 + i];
    for (int i = 0; i < 3; ++i)
        o.m[i * 4 + 3] = -(o.m[i * 4] * p.m[3] + o.m[i * 4 + 1] * p.m[7] + o.m[i * 4 + 2] * p.m[11]);
    return o;
}

// S m S, S = diag(-1, 1, 1): elements 1, 2, 3, 4 and 8 negated.
void MirrorInto(const float in[12], float out[12]) {
    for (int i = 0; i < 12; ++i)
        out[i] = in[i];
    for (const int i : {1, 2, 3, 4, 8})
        out[i] = -in[i];
}

Pose Mirror(const Pose& p) {
    Pose o;
    MirrorInto(p.m, o.m);
    return o;
}

// The turn's rotation, put so that the model point anchor lands at.
Pose PlaceAt(const Pose& turn, const float anchor[3], V3 at) {
    Pose o = turn;
    o.m[3] = at.x - (o.m[0] * anchor[0] + o.m[1] * anchor[1] + o.m[2] * anchor[2]);
    o.m[7] = at.y - (o.m[4] * anchor[0] + o.m[5] * anchor[1] + o.m[6] * anchor[2]);
    o.m[11] = at.z - (o.m[8] * anchor[0] + o.m[9] * anchor[1] + o.m[10] * anchor[2]);
    return o;
}

// A part of the way from a to b, its rotation made orthonormal again.
Pose Blend(const Pose& a, const Pose& b, float w) {
    Pose o{};
    for (int i = 0; i < 12; ++i)
        o.m[i] = a.m[i] + (b.m[i] - a.m[i]) * w;
    float c0[3] = {o.m[0], o.m[4], o.m[8]}, c1[3] = {o.m[1], o.m[5], o.m[9]};
    const float n0 = std::sqrt(c0[0] * c0[0] + c0[1] * c0[1] + c0[2] * c0[2]);
    for (float& c : c0)
        c /= n0;
    const float d = c0[0] * c1[0] + c0[1] * c1[1] + c0[2] * c1[2];
    for (int i = 0; i < 3; ++i)
        c1[i] -= d * c0[i];
    const float n1 = std::sqrt(c1[0] * c1[0] + c1[1] * c1[1] + c1[2] * c1[2]);
    for (float& c : c1)
        c /= n1;
    const float c2[3] = {c0[1] * c1[2] - c0[2] * c1[1], c0[2] * c1[0] - c0[0] * c1[2], c0[0] * c1[1] - c0[1] * c1[0]};
    for (int i = 0; i < 3; ++i)
        o.m[i * 4] = c0[i], o.m[i * 4 + 1] = c1[i], o.m[i * 4 + 2] = c2[i];
    return o;
}

Pose Get(const float m[12]) {
    Pose p;
    std::memcpy(p.m, m, sizeof(p.m));
    return p;
}

void Put(float m[12], const Pose& p) { std::memcpy(m, p.m, sizeof(p.m)); }

// ---- The mirror: a frame, a state and a result with the sides swapped ----
int Other(int side) { return side >= 0 ? 1 - side : side; }

grasp::Frame MirrorFrame(const grasp::Frame& in) {
    grasp::Frame f = in;
    for (int s = 0; s < 2; ++s) {
        f.hand[s] = in.hand[1 - s];
        MirrorInto(in.hand[1 - s].pose, f.hand[s].pose);
        f.gripPoint[s][0] = -in.gripPoint[1 - s][0];
        f.gripPoint[s][1] = in.gripPoint[1 - s][1];
        f.gripPoint[s][2] = in.gripPoint[1 - s][2];
    }
    MirrorInto(in.gunRoom, f.gunRoom);
    MirrorInto(in.lyingRoom, f.lyingRoom);
    f.boxMin[0] = -in.boxMax[0];
    f.boxMax[0] = -in.boxMin[0];
    return f;
}

grasp::State MirrorState(const grasp::State& in) {
    grasp::State s = in;
    s.trigger = Other(in.trigger), s.barrel = Other(in.barrel);
    s.beforeTrigger = Other(in.beforeTrigger), s.beforeBarrel = Other(in.beforeBarrel);
    for (int i = 0; i < 2; ++i) {
        s.armed[i] = in.armed[1 - i], s.fireReset[i] = in.fireReset[1 - i], s.low[i] = in.low[1 - i];
        s.armTake[i] = in.armTake[1 - i], s.crossedAt[i] = in.crossedAt[1 - i], s.letGoAt[i] = in.letGoAt[1 - i];
    }
    return s;
}

grasp::Result MirrorResult(const grasp::Result& in) {
    grasp::Result r = in;
    r.trigger = Other(in.trigger), r.barrel = Other(in.barrel), r.fireHand = Other(in.fireHand);
    r.letGo = Other(in.letGo), r.toBarrel = Other(in.toBarrel), r.released = Other(in.released);
    r.pickedUp = Other(in.pickedUp), r.regrip = Other(in.regrip), r.arrived = Other(in.arrived);
    for (int i = 0; i < 2; ++i)
        r.armed[i] = in.armed[1 - i], r.spent[i] = in.spent[1 - i];
    return r;
}

// ---- Results ----
const char* Name(Holder h) {
    switch (h) {
    case Holder::kHeld:
        return "held";
    case Holder::kBarrel:
        return "barrel";
    case Holder::kGlide:
        return "glide";
    case Holder::kLoose:
        return "loose";
    }
    return "?";
}

bool Quiet(const grasp::Result& r) {
    return r.letGo < 0 && r.toBarrel < 0 && r.released < 0 && r.pickedUp < 0 && r.regrip < 0 && r.arrived < 0 &&
           !r.returned && !r.spent[0] && !r.spent[1];
}

std::string Describe(const grasp::Result& r) {
    char b[320];
    std::snprintf(b, sizeof(b),
                  "%s trigger %d barrel %d fires %d glide %.3f%s head %.3f reach %.3f armed %d%d spent %d%d | let go %d "
                  "released %d to barrel %d picked up %d regrip %d arrived %d%s%s",
                  Name(r.holder), r.trigger, r.barrel, r.fireHand, r.glide, r.glideFromWorld ? " (from the world)" : "",
                  r.headShare, r.reach, r.armed[0], r.armed[1], r.spent[0], r.spent[1], r.letGo, r.released, r.toBarrel,
                  r.pickedUp, r.regrip, r.arrived, r.returned ? " returned: " : "", r.returned ? r.why : "");
    return b;
}

bool Same(const grasp::Result& a, const grasp::Result& b) {
    const auto close = [](float x, float y) { return (std::isnan(x) && std::isnan(y)) || std::fabs(x - y) <= 1e-6f; };
    return a.holder == b.holder && a.trigger == b.trigger && a.barrel == b.barrel && a.fireHand == b.fireHand &&
           close(a.glide, b.glide) && a.glideFromWorld == b.glideFromWorld && close(a.headShare, b.headShare) &&
           close(a.reach, b.reach) && a.armed[0] == b.armed[0] && a.armed[1] == b.armed[1] && a.spent[0] == b.spent[0] &&
           a.spent[1] == b.spent[1] && a.letGo == b.letGo && a.toBarrel == b.toBarrel && a.released == b.released &&
           a.pickedUp == b.pickedUp && a.regrip == b.regrip && a.arrived == b.arrived && a.returned == b.returned &&
           std::strcmp(a.why, b.why) == 0;
}

// The states' rules, every frame (cleared: a new life, lost or free hands off this frame; physRight: the right
// hand's side as the row has it).
const char* Broken(const grasp::Result& r, bool cleared, int physRight) {
    const bool hand = r.trigger == 0 || r.trigger == 1;
    switch (r.holder) {
    case Holder::kHeld:
        if (!hand || r.barrel != -1 || r.glide != 1.0f || r.glideFromWorld)
            return "held: a trigger hand, no barrel hand, no glide";
        break;
    case Holder::kBarrel:
        if (r.trigger != -1 || (r.barrel != 0 && r.barrel != 1) || r.fireHand != -1 || r.glide != 1.0f)
            return "barrel: a barrel hand, no trigger hand, nothing fires";
        break;
    case Holder::kGlide:
        if (!hand || (r.glideFromWorld ? r.barrel != -1 : r.barrel != 1 - r.trigger) || r.fireHand != -1 ||
            !(r.glide >= 0.0f && r.glide <= 1.0f))
            return "glide: a trigger hand (and the other's hold from a barrel), nothing fires";
        break;
    case Holder::kLoose:
        if (r.trigger != -1 || r.barrel != -1 || r.fireHand != -1 || r.glide != 1.0f)
            return "loose: no hands, nothing fires";
        break;
    }
    if (r.fireHand != -1 && r.fireHand != r.trigger)
        return "fires only by the pistol-grip hand";
    if (!(r.headShare >= 0.0f && r.headShare <= 1.0f) || !std::isfinite(r.reach) || r.reach < 0.0f)
        return "head share 0-1, reach finite";
    for (int s = 0; s < 2; ++s) {
        if (r.armed[s] && !((r.holder == Holder::kHeld || r.holder == Holder::kGlide) && r.trigger == s))
            return "armed: only the pistol-grip hand";
        if (r.spent[s] != (r.pickedUp == s || r.regrip == s))
            return "spent only on a take by that hand";
    }
    if (r.released >= 0 && r.holder != Holder::kLoose)
        return "released: in the world";
    if (r.toBarrel >= 0 && !(r.holder == Holder::kBarrel && r.barrel == r.toBarrel))
        return "to barrel: held by that hand's hold";
    if (r.pickedUp >= 0 && !(r.holder == Holder::kGlide && r.trigger == r.pickedUp && r.glideFromWorld && r.glide == 0.0f))
        return "picked up: gliding into that hand from the world";
    if (r.regrip >= 0 && !(r.holder == Holder::kGlide && r.trigger == r.regrip && !r.glideFromWorld && r.glide == 0.0f))
        return "regrip: gliding into that hand from the other's hold";
    if (r.arrived >= 0 && !(r.holder == Holder::kHeld && r.trigger == r.arrived))
        return "arrived: held by that hand";
    if (r.letGo >= 0 && r.released != r.letGo && r.toBarrel != 1 - r.letGo)
        return "let go: released by that hand or held by the other's hold";
    if (r.returned != (cleared && r.returned) || (r.returned && !*r.why))
        return "returned only on a return, with why";
    if (cleared && !(r.holder == Holder::kHeld && r.trigger == physRight && !r.armed[0] && !r.armed[1] &&
                     r.fireHand == physRight && !r.spent[0] && !r.spent[1]))
        return "a return: in the right hand, nothing armed, its trigger firing it";
    return nullptr;
}

// ---- A run of the machine, as written or mirrored ----
struct Entry {
    int segment, frame;
    bool cleared;
    grasp::Result r;
};

struct Run {
    explicit Run(bool mirror) : mirrored(mirror) { Reset(); }

    // A new machine at the spawn (for the mirrored run, the spawn's mirror: the rows' right hand holds it).
    void Reset() {
        grasp::Clear(state);
        if (mirrored)
            state = MirrorState(state);
        ++segment;
        frame = 0;
    }

    // One frame: the row's frame (mirrored for the mirrored run), its result read back by the row's sides.
    grasp::Result Step(const grasp::Frame& written) {
        const grasp::Frame f = mirrored ? MirrorFrame(written) : written;
        grasp::Result r = grasp::Update(f, state);
        if (mirrored)
            r = MirrorResult(r);
        const bool cleared = written.newLife || written.lost || !written.enabled;
        if (const char* why = Broken(r, cleared, PhysRight()))
            if (!bad++)
                first = std::string(why) + " (frame " + std::to_string(frame) + ": " + Describe(r) + ")";
        log.push_back({segment, frame++, cleared, r});
        return r;
    }

    grasp::Result Steps(const grasp::Frame& f, int n) {
        grasp::Result r;
        for (int i = 0; i < n; ++i)
            r = Step(f);
        return r;
    }

    void NotLetGo() { grasp::NotLetGo(state); }
    grasp::State Written() const { return mirrored ? MirrorState(state) : state; }  // the state by the row's sides
    int PhysRight() const { return mirrored ? L : R; }  // the right hand, by the row's sides

    bool mirrored;
    grasp::State state;
    std::vector<Entry> log;
    int segment = 0, frame = 0, bad = 0;
    std::string first;
};

// n frames of f: how many changed who has it (or brought an event), fired, were armed.
struct Span {
    grasp::Result last;
    int changed = 0, fires = 0, armed = 0;
};

Span Hold(Run& run, const grasp::Frame& f, int n) {
    const grasp::State was = run.Written();
    Span sp;
    for (int i = 0; i < n; ++i) {
        const grasp::Result r = run.Step(f);
        sp.last = r;
        sp.changed += !Quiet(r) || r.holder != was.holder || r.trigger != was.trigger || r.barrel != was.barrel;
        sp.fires += r.fireHand >= 0;
        sp.armed += r.armed[0] || r.armed[1];
    }
    return sp;
}

// ---- The scene ----
// The right grip 18 cm right, 25 cm down and 35 cm ahead of the head, turned a little; the left its mirror image.
Pose RightGrip() { return Moved(Turn(-25.0f, 12.0f, 8.0f), {0.18f, 0.95f, -0.35f}); }
// The rifle lying on its side on the floor 60 cm ahead.
Pose Floor() { return Moved(Turn(0.0f, 20.0f, 90.0f), {0.05f, 0.03f, -0.6f}); }

// The rifle placed in a hand by its pistol-grip point, turned with the hand.
Pose GunIn(const grasp::Frame& f, int side) {
    const Pose hand = Get(f.hand[side].pose);
    return PlaceAt(hand, f.gripPoint[side], Pos(hand));
}

// Both hands seen, their own controllers, read, open; the rifle drawn in the right hand; the floor pose ready.
grasp::Frame Base() {
    grasp::Frame f;
    f.dt = kDt;
    f.enabled = true;
    for (int s = 0; s < 2; ++s) {
        grasp::HandIn& h = f.hand[s];
        h.tracked = h.own = h.read = true;
        Put(h.pose, s == R ? RightGrip() : Mirror(RightGrip()));
    }
    for (int i = 0; i < 3; ++i) {
        f.gripPoint[R][i] = kAnchor[i];
        f.gripPoint[L][i] = i == 0 ? -kAnchor[i] : kAnchor[i];
        f.boxMin[i] = kHullMin[i];
        f.boxMax[i] = kHullMax[i];
    }
    f.haveGun = true;
    Put(f.gunRoom, GunIn(f, R));
    Put(f.lyingRoom, Floor());
    return f;
}

void SetPos(grasp::HandIn& h, V3 p) { h.pose[3] = p.x, h.pose[7] = p.y, h.pose[11] = p.z; }

// A hand moved (its turn kept) to d metres above the box of the rifle lying, over its middle.
void NearLying(grasp::Frame& f, int side, float d) {
    SetPos(f.hand[side], Apply(Get(f.lyingRoom), {0.0f, kHullMax[1] + d, 0.3f}));
}

// A hand moved to d metres from its side's pistol-grip point on the rifle as drawn.
void NearGrip(grasp::Frame& f, int side, float d) {
    const float* g = f.gripPoint[side];
    SetPos(f.hand[side], Apply(Get(f.gunRoom), {g[0], g[1] + 0.6f * d, g[2] + 0.8f * d}));
}

// A fresh squeeze: open for a frame (under kArm), then past kTake.
grasp::Result Fresh(Run& run, grasp::Frame& f, int side) {
    f.hand[side].squeeze = 0.0f;
    run.Step(f);
    f.hand[side].squeeze = 0.9f;
    return run.Step(f);
}

// The hand on the pistol grip squeezes (arming it), then opens for 3 frames: the last frame's result.
grasp::Result Drop(Run& run, grasp::Frame& f, int side) {
    f.hand[side].squeeze = 0.9f;
    run.Step(f);
    f.hand[side].squeeze = 0.2f;
    run.Step(f);
    run.Step(f);
    return run.Step(f);
}

// Dropped by the right and lying on the floor for half a second, both hands open.
grasp::Frame ToLoose(Run& run) {
    grasp::Frame f = Base();
    Drop(run, f, R);
    f.lying = true;
    f.hand[R].squeeze = 0.0f;
    run.Steps(f, 36);
    return f;
}

// On that side's pistol grip, armed (the left's by a pickup from the floor), drawn there.
grasp::Frame ToHeld(Run& run, int side) {
    if (side == R) {
        grasp::Frame f = Base();
        f.hand[R].squeeze = 0.9f;
        run.Step(f);
        return f;
    }
    grasp::Frame f = ToLoose(run);
    NearLying(f, L, 0.1f);
    Fresh(run, f, L);
    run.Steps(f, 10);  // in the hand after 9
    f.lying = false;
    Put(f.gunRoom, GunIn(f, L));
    return f;
}

// Held by side b's hold on the handguard alone: the other hand held it by its pistol grip and let go.
grasp::Frame ToBarrel(Run& run, int b) {
    grasp::Frame f = ToHeld(run, 1 - b);
    f.hand[b].onSupport = true;
    run.Step(f);
    f.hand[1 - b].squeeze = 0.2f;
    run.Steps(f, 3);
    return f;
}

// Gliding into side's hand from the floor, n frames in.
grasp::Frame ToGlideWorld(Run& run, int side, int n) {
    grasp::Frame f = ToLoose(run);
    NearLying(f, side, 0.1f);
    Fresh(run, f, side);
    run.Steps(f, n);
    return f;
}

// Gliding into side g's hand from the other hand's hold on the handguard (a regrip), n frames in.
grasp::Frame ToGlideBarrel(Run& run, int g, int n) {
    grasp::Frame f = ToBarrel(run, 1 - g);
    f.hand[g].squeeze = 0.0f;
    run.Steps(f, 30);  // past its own let-go's 0.25 s
    NearGrip(f, g, 0.05f);
    f.hand[g].squeeze = 0.9f;
    run.Step(f);
    run.Steps(f, n);
    return f;
}

struct Case {
    const char* name;
    grasp::Frame (*make)(Run&);
};
const Case kStates[] = {
    {"held by the right", [](Run& r) { return ToHeld(r, R); }},
    {"held by the left", [](Run& r) { return ToHeld(r, L); }},
    {"the left's hold alone", [](Run& r) { return ToBarrel(r, L); }},
    {"the right's hold alone", [](Run& r) { return ToBarrel(r, R); }},
    {"gliding into the left from the floor", [](Run& r) { return ToGlideWorld(r, L, 3); }},
    {"gliding into the right from the left's hold", [](Run& r) { return ToGlideBarrel(r, R, 3); }},
    {"lying", [](Run& r) { return ToLoose(r); }},
};

// ---- 1-3: the spawn, the open grip, arming and letting go ----
void Row1(Run& run) {
    const grasp::State s = run.Written();
    Check("1a", "the spawn: held by the right's pistol grip, nothing armed, no squeeze yet to take with",
          s.holder == Holder::kHeld && s.trigger == R && s.barrel == -1 && !s.armed[0] && !s.armed[1] && !s.armTake[0] &&
              !s.armTake[1] && s.fireReset[R] && !s.fireReset[L]);
    const grasp::Result r = run.Step(Base());
    Check("1b", "its first frame: the right's trigger fires it, nothing armed, no event",
          r.holder == Holder::kHeld && r.trigger == R && r.barrel == -1 && r.fireHand == R && !r.armed[0] &&
              !r.armed[1] && Quiet(r) && r.glide == 1.0f && r.headShare == 0.0f,
          "%s", Describe(r).c_str());
}

void Row2(Run& run) {
    grasp::Frame f = Base();
    const Span open = Hold(run, f, 720);
    Check("2a", "the right never squeezed, its grip open for 10 s: never armed, still held",
          open.changed == 0 && open.armed == 0 && open.last.holder == Holder::kHeld && open.last.trigger == R,
          "%d frames otherwise", open.changed + open.armed);
    f.hand[R].squeeze = 0.59f;
    const Span half = Hold(run, f, 36);
    f.hand[R].squeeze = 0.0f;
    const Span after = Hold(run, f, 72);
    Check("2b", "half squeezed (0.59, never past 0.6) for 0.5 s, then open: never armed, still held",
          half.changed + after.changed == 0 && half.armed + after.armed == 0, "%d frames otherwise",
          half.changed + after.changed + half.armed + after.armed);
}

void Row3(Run& run) {
    grasp::Frame f = Base();
    f.hand[R].squeeze = 0.7f;
    const grasp::Result armed = run.Step(f);
    Check("3a", "squeezed to 0.7: armed", armed.armed[R] && !armed.armed[L] && armed.holder == Holder::kHeld && Quiet(armed),
          "%s", Describe(armed).c_str());
    f.hand[R].squeeze = 0.3f;
    const grasp::Result r1 = run.Step(f), r2 = run.Step(f);
    Check("3b", "0.3 for two frames: still held", r1.holder == Holder::kHeld && r2.holder == Holder::kHeld && Quiet(r1) && Quiet(r2),
          "%s", Describe(r2).c_str());
    const grasp::Result r3 = run.Step(f);
    Check("3c", "the third frame: let go and released into the world, nothing fires",
          r3.letGo == R && r3.released == R && r3.toBarrel == -1 && r3.holder == Holder::kLoose && r3.trigger == -1 &&
              r3.barrel == -1 && r3.fireHand == -1 && !r3.armed[R] && r3.headShare == 0.0f,
          "%s", Describe(r3).c_str());

    run.Reset();
    f = Base();
    f.hand[R].squeeze = 0.7f;
    run.Step(f);
    int held = 0;
    for (int k = 0; k < 2; ++k) {
        f.hand[R].squeeze = 0.3f;
        held += run.Step(f).holder == Holder::kHeld;
        held += run.Step(f).holder == Holder::kHeld;
        f.hand[R].squeeze = 0.7f;
        held += run.Step(f).holder == Holder::kHeld;
    }
    f.hand[R].squeeze = 0.3f;
    held += run.Step(f).holder == Holder::kHeld;
    held += run.Step(f).holder == Holder::kHeld;
    const grasp::Result third = run.Step(f);
    Check("3d", "open two frames, closed, open two again: held; the third open frame in a row lets go",
          held == 8 && third.released == R, "%d of 8 held, then %s", held, Describe(third).c_str());

    run.Reset();
    f = Base();
    f.hand[R].squeeze = 0.7f;
    run.Step(f);
    f.hand[R].squeeze = grasp::kLetGo;
    const Span edge = Hold(run, f, 72);
    Check("3e", "held at 0.35 exactly (not under the let-go) for a second: still held", edge.changed == 0, "%d frames otherwise",
          edge.changed);

    run.Reset();
    f = Base();
    f.hand[R].squeeze = grasp::kTake;
    run.Step(f);
    const grasp::Result take = Drop(run, f, R);
    run.Reset();
    f = Base();
    f.hand[R].squeeze = 0.599f;
    run.Step(f);
    f.hand[R].squeeze = 0.2f;
    const Span under = Hold(run, f, 36);
    Check("3f", "0.6 exactly arms it (its let-go drops it); 0.599 doesn't", take.released == R && under.changed == 0,
          "%s; 0.599: %d frames otherwise", Describe(take).c_str(), under.changed);
}

// ---- 4-6: what never drops it ----
void Row4(Run& run) {
    grasp::Frame f = Base();
    f.hand[R].squeeze = 0.9f;
    run.Step(f);
    f.hand[R].read = false;
    f.hand[R].squeeze = 0.0f;
    const Span unread = Hold(run, f, 360);
    Check("4a", "armed, then its grip not read (headset off, controller asleep) and open for 5 s: never let go, disarmed",
          unread.changed == 0 && unread.armed == 0, "%d frames changed, %d armed", unread.changed, unread.armed);
    f.hand[R].read = true;
    const Span back = Hold(run, f, 360);
    Check("4b", "read again with the grip open: still held (it drops only once squeezed and let go)",
          back.changed == 0 && back.armed == 0, "%d frames changed, %d armed", back.changed, back.armed);
    const grasp::Result r = Drop(run, f, R);
    Check("4c", "then squeezed and let go: it falls", r.released == R && r.holder == Holder::kLoose, "%s", Describe(r).c_str());

    run.Reset();
    f = Base();
    f.hand[R].squeeze = 0.9f;
    run.Step(f);
    f.hand[R].squeeze = 0.2f;
    run.Step(f);
    run.Step(f);
    f.hand[R].read = false;
    run.Step(f);
    f.hand[R].read = true;
    const Span mid = Hold(run, f, 72);
    Check("4d", "open two frames, a frame unread, then open: disarmed, still held", mid.changed == 0, "%d frames otherwise",
          mid.changed);
}

void Row5(Run& run) {
    grasp::Frame f = Base();
    f.hand[R].own = false;
    f.hand[R].squeeze = 1.0f;
    const Span squeezed = Hold(run, f, 36);
    f.hand[R].squeeze = 0.0f;
    const Span open = Hold(run, f, 360);
    Check("5a", "the stand-in's hand (not its own controller): squeezed fully, then open for 5 s: never armed, still held",
          squeezed.changed + open.changed == 0 && squeezed.armed + open.armed == 0, "%d frames changed, %d armed",
          squeezed.changed + open.changed, squeezed.armed + open.armed);
    f.hand[R].own = true;
    const Span real = Hold(run, f, 72);
    Check("5b", "the real controller back, its grip open: still held", real.changed == 0 && real.armed == 0, "%d frames otherwise",
          real.changed + real.armed);
}

void Row6(Run& run) {
    grasp::Frame f = Base();
    f.hand[R].squeeze = 0.9f;
    run.Step(f);
    f.hand[R].tracked = false;
    f.hand[R].squeeze = 0.0f;
    const Span lost = Hold(run, f, 72);
    f.hand[R].tracked = true;
    const Span seen = Hold(run, f, 72);
    Check("6", "armed, then out of tracking past the hold with the grip open, then seen again open: disarmed, never let go",
          lost.changed + seen.changed == 0 && lost.armed + seen.armed == 0, "%d frames changed, %d armed",
          lost.changed + seen.changed, lost.armed + seen.armed);
}

// ---- 7-10: the hold on the handguard alone, the regrip ----
void Row7(Run& run) {
    grasp::Frame f = Base();
    f.hand[L].onSupport = true;
    const grasp::Result r = Drop(run, f, R);
    Check("7a", "the right lets go with the left on the handguard: held by the left's hold alone, nothing fires",
          r.letGo == R && r.toBarrel == L && r.released == -1 && r.holder == Holder::kBarrel && r.barrel == L &&
              r.trigger == -1 && r.fireHand == -1,
          "%s", Describe(r).c_str());
    const Span kept = Hold(run, f, 144);
    Check("7b", "held by the handguard alone for 2 s: stays", kept.changed == 0 && kept.fires == 0, "%d frames otherwise",
          kept.changed + kept.fires);

    run.Reset();
    f = Base();
    f.hand[L].onSupport = true;
    f.hand[L].tracked = false;
    const grasp::Result unseen = Drop(run, f, R);
    Check("7c", "the left's hold not seen as the right lets go: it falls", unseen.released == R && unseen.holder == Holder::kLoose,
          "%s", Describe(unseen).c_str());
}

void Row8(Run& run) {
    grasp::Frame f = ToBarrel(run, L);
    f.hand[L].onSupport = false;
    const grasp::Result r = run.Step(f);
    Check("8a", "held by the left's hold alone, the left lets go: it falls from the left",
          r.released == L && r.letGo == -1 && r.toBarrel == -1 && r.holder == Holder::kLoose && r.trigger == -1 &&
              r.barrel == -1 && r.fireHand == -1 && r.headShare == 0.0f,
          "%s", Describe(r).c_str());

    run.Reset();
    f = ToBarrel(run, L);
    f.haveGun = false;
    f.hand[L].onSupport = false;
    const Span undrawn = Hold(run, f, 10);
    f.haveGun = true;
    const grasp::Result drawn = run.Step(f);
    Check("8b", "let go of with the rifle not drawn last frame: held until it is, then it falls",
          undrawn.changed == 0 && drawn.released == L, "%d frames changed; then %s", undrawn.changed, Describe(drawn).c_str());
}

void Row9(Run& run) {
    grasp::Frame f = ToBarrel(run, L);
    f.hand[R].squeeze = 0.0f;
    run.Steps(f, 36);
    NearGrip(f, R, 0.10f);
    f.hand[R].trigger = 0.5f;
    f.hand[R].squeeze = 0.9f;
    grasp::Result r = run.Step(f);
    Check("9a", "the right squeezes 10 cm from its pistol grip: takes it, gliding in, the left still on the handguard",
          r.regrip == R && r.spent[R] && !r.spent[L] && r.holder == Holder::kGlide && r.trigger == R && r.barrel == L &&
              !r.glideFromWorld && r.glide == 0.0f && r.armed[R] && r.fireHand == -1 && Near(r.reach, 0.10f, 1e-4f) &&
              r.headShare == 0.0f,
          "%s", Describe(r).c_str());
    int arrivedAt = -1;
    float worst = 0.0f;
    bool kept = true;
    for (int n = 1; n <= 12 && arrivedAt < 0; ++n) {
        r = run.Step(f);
        if (r.arrived == R)
            arrivedAt = n;
        else {
            worst = std::max(worst, std::fabs(r.glide - grasp::Smooth(n * kDt / 0.12f)));
            kept = kept && r.holder == Holder::kGlide && r.barrel == L && r.fireHand == -1 && r.headShare == 0.0f;
        }
    }
    Check("9b", "glides in (smoothed) with the left's hold kept and nothing firing; in the hand at 0.12 s",
          arrivedAt == 9 && worst < 1e-5f && kept && r.holder == Holder::kHeld && r.trigger == R && r.barrel == -1,
          "arrived after %d frames (%.3f s), glide off its curve by %.6f", arrivedAt, arrivedAt * kDt, worst);
    const Span held = Hold(run, f, 36);
    Check("9c", "in the hand with its trigger still at 0.5 (held through the take): it doesn't fire",
          r.fireHand == -1 && held.fires == 0 && held.changed == 0, "%d frames fired", held.fires);
    f.hand[R].trigger = 0.2f;
    r = run.Step(f);
    Check("9d", "its trigger let go under 0.4: now its trigger fires it", r.fireHand == R, "%s", Describe(r).c_str());
}

void Row10(Run& run) {
    const auto attempt = [&run](float d) {
        run.Reset();
        grasp::Frame f = ToBarrel(run, L);
        f.hand[R].squeeze = 0.0f;
        run.Steps(f, 36);
        NearGrip(f, R, d);
        const grasp::Result r = Fresh(run, f, R);
        const Span after = Hold(run, f, 36);
        return r.regrip == R ? 1 : r.holder == Holder::kBarrel && after.changed == 0 ? 0 : -1;
    };
    const int far = attempt(0.30f), in = attempt(0.149f), out = attempt(0.151f);
    Check("10", "the right squeezes 30 cm from its pistol grip: nothing (still in the left's hold); 14.9 cm takes it, 15.1 doesn't",
          far == 0 && in == 1 && out == 0, "30 cm %d, 14.9 cm %d, 15.1 cm %d (1 taken, 0 not, -1 something else)", far, in, out);
}

// ---- 11-15: the pickup and the glide ----
void Row11(Run& run) {
    grasp::Frame f = ToLoose(run);
    NearLying(f, L, 0.20f);
    grasp::Result r = Fresh(run, f, L);
    Check("11a", "lying, the left squeezes 20 cm from its box: picked up by its pistol grip, gliding in",
          r.pickedUp == L && r.spent[L] && !r.spent[R] && r.holder == Holder::kGlide && r.trigger == L && r.barrel == -1 &&
              r.glideFromWorld && r.glide == 0.0f && r.armed[L] && Near(r.reach, 0.20f, 1e-4f) && r.headShare == 1.0f &&
              r.fireHand == -1,
          "%s", Describe(r).c_str());
    float g4 = -1.0f, g5 = -1.0f;
    int arrivedAt = -1;
    for (int n = 1; n <= 12 && arrivedAt < 0; ++n) {
        r = run.Step(f);
        if (n == 4)
            g4 = r.glide;
        if (n == 5)
            g5 = r.glide;
        if (r.arrived == L)
            arrivedAt = n;
    }
    Check("11b", "half way into the hand at 0.06 s",
          g4 < 0.5f && g5 > 0.5f && Near(g4, grasp::Smooth(4 * kDt / 0.12f)) && Near(g5, grasp::Smooth(5 * kDt / 0.12f)),
          "%.3f at %.3f s, %.3f at %.3f s", g4, 4 * kDt, g5, 5 * kDt);
    Check("11c", "in the hand at 0.12 s, armed, its trigger firing it, the aim back on the gun",
          arrivedAt == 9 && r.holder == Holder::kHeld && r.trigger == L && r.armed[L] && r.fireHand == L &&
              r.headShare == 0.0f,
          "arrived after %d frames: %s", arrivedAt, Describe(r).c_str());

    run.Reset();
    f = ToLoose(run);
    NearLying(f, R, 0.20f);
    const grasp::Result right = Fresh(run, f, R);
    Check("11d", "the right likewise, half a second after its own let-go", right.pickedUp == R && right.trigger == R,
          "%s", Describe(right).c_str());

    const auto attempt = [&run](float d) {
        run.Reset();
        grasp::Frame f = ToLoose(run);
        NearLying(f, L, d);
        const grasp::Result r = Fresh(run, f, L);
        const Span after = Hold(run, f, 36);
        return r.pickedUp == L ? 1 : r.holder == Holder::kLoose && after.changed == 0 ? 0 : -1;
    };
    const int in = attempt(0.249f), out = attempt(0.251f);
    Check("11e", "24.9 cm from its box takes it, 25.1 doesn't", in == 1 && out == 0, "24.9 cm %d, 25.1 cm %d", in, out);
}

void Row12(Run& run) {
    std::string missed;
    const auto expect = [&missed](bool ok, const char* what) {
        if (!ok)
            missed += std::string(missed.empty() ? "" : "; ") + what;
    };
    // A squeeze crossed 1 m away, still held when the hand reaches the rifle 0.5 s later.
    grasp::Frame f = ToLoose(run);
    Fresh(run, f, L);
    run.Steps(f, 36);
    NearLying(f, L, 0.10f);
    expect(Hold(run, f, 72).changed == 0, "a held squeeze");
    // A squeeze held since the spawn, never opened under 0.4.
    run.Reset();
    f = Base();
    f.hand[L].squeeze = 0.9f;
    Drop(run, f, R);
    f.lying = true;
    f.hand[R].squeeze = 0.0f;
    run.Steps(f, 36);
    NearLying(f, L, 0.10f);
    expect(Hold(run, f, 72).changed == 0, "a squeeze held since the spawn");
    // 40 cm away.
    run.Reset();
    f = ToLoose(run);
    NearLying(f, L, 0.40f);
    expect(Fresh(run, f, L).holder == Holder::kLoose && Hold(run, f, 72).changed == 0, "40 cm away");
    // Busy, not tracked, the stand-in's, not read.
    for (int k = 0; k < 4; ++k) {
        run.Reset();
        f = ToLoose(run);
        NearLying(f, L, 0.10f);
        grasp::HandIn& h = f.hand[L];
        (k == 0 ? h.busy : k == 1 ? h.tracked : k == 2 ? h.own : h.read) = k == 0;
        const grasp::Result r = Fresh(run, f, L);
        expect(r.holder == Holder::kLoose && Hold(run, f, 72).changed == 0,
               k == 0 ? "busy" : k == 1 ? "not tracked" : k == 2 ? "the stand-in's" : "not read");
    }
    Check("12a", "never taken by a held squeeze, one held since the spawn, 40 cm away, busy, not tracked, the stand-in's, not read",
          missed.empty(), "%s", missed.empty() ? "none taken" : ("taken: " + missed).c_str());

    // Within 0.25 s of its own let-go: the rifle lying 10 cm under the right from the frame after.
    run.Reset();
    f = Base();
    Drop(run, f, R);
    f.lying = true;
    f.hand[R].squeeze = 0.0f;
    NearLying(f, R, 0.10f);
    run.Steps(f, 3);
    f.hand[R].squeeze = 0.9f;  // crossed 4 frames after the let-go
    int takenAt = -1;
    for (int n = 4; n <= 30 && takenAt < 0; ++n)
        if (run.Step(f).pickedUp == R)
            takenAt = n;
    Check("12b", "the right squeezes again 0.06 s after its own let-go: not taken within 0.25 s of it (the same squeeze takes it after)",
          takenAt > 0 && takenAt * kDt >= 0.25f - 1e-4f && takenAt * kDt <= 0.25f + 2 * kDt, "taken %.3f s after the let-go",
          takenAt * kDt);
    run.Reset();
    f = Base();
    Drop(run, f, R);
    f.lying = true;
    f.hand[R].squeeze = 0.0f;
    NearLying(f, L, 0.10f);
    run.Steps(f, 2);
    const grasp::Result other = Fresh(run, f, L);
    Check("12c", "the other hand takes it at once (0.06 s after the right's let-go)", other.pickedUp == L, "%s",
          Describe(other).c_str());
}

void Row13(Run& run) {
    int got[2] = {-1, -1};
    for (int k = 0; k < 2; ++k) {
        run.Reset();
        grasp::Frame f = ToLoose(run);
        NearLying(f, L, k == 0 ? 0.10f : 0.20f);
        NearLying(f, R, k == 0 ? 0.20f : 0.10f);
        f.hand[L].squeeze = f.hand[R].squeeze = 0.0f;
        run.Step(f);
        f.hand[L].squeeze = f.hand[R].squeeze = 0.9f;
        const grasp::Result r = run.Step(f);
        got[k] = r.pickedUp >= 0 && r.spent[r.pickedUp] && !r.spent[1 - r.pickedUp] && Near(r.reach, 0.10f, 1e-4f) ? r.pickedUp : -2;
    }
    Check("13", "both hands fresh in one frame: the nearer takes it (the left at 10 cm against 20, then the right)",
          got[0] == L && got[1] == R, "taken by %d, then by %d", got[0], got[1]);
}

void Row14(Run& run) {
    grasp::Frame f = ToLoose(run);
    NearLying(f, R, 0.10f);
    f.hand[R].trigger = 0.8f;
    grasp::Result r = Fresh(run, f, R);
    int fired = r.fireHand >= 0, frames = 0;
    while (r.arrived < 0 && frames < 20)
        r = run.Step(f), fired += r.fireHand >= 0, ++frames;
    const Span held = Hold(run, f, 72);
    f.hand[R].trigger = 0.3f;
    const grasp::Result let = run.Step(f);
    Check("14a", "picked up with its trigger held (0.8): nothing fires through the glide and after, until it's under 0.4",
          r.arrived == R && fired == 0 && held.fires == 0 && let.fireHand == R, "%d frames fired, then %s", fired + held.fires,
          Describe(let).c_str());

    run.Reset();
    f = ToLoose(run);
    NearLying(f, R, 0.10f);
    f.hand[R].trigger = 0.39f;
    r = Fresh(run, f, R);
    frames = 0;
    while (r.arrived < 0 && frames < 20)
        r = run.Step(f), ++frames;
    Check("14b", "its trigger at 0.39 (under 0.4) at the pickup: fires once in the hand", r.arrived == R && r.fireHand == R,
          "%s", Describe(r).c_str());
}

void Row15(Run& run) {
    grasp::Frame f = ToGlideWorld(run, R, 2);
    f.hand[R].squeeze = 0.2f;
    const grasp::Result a = run.Step(f), b = run.Step(f), c = run.Step(f);
    Check("15a", "picked up, let go 2 frames into the glide: it falls from where it flew",
          a.holder == Holder::kGlide && b.holder == Holder::kGlide && c.letGo == R && c.released == R &&
              c.holder == Holder::kLoose && c.headShare == 0.0f,
          "%s", Describe(c).c_str());

    run.Reset();
    f = ToGlideBarrel(run, R, 2);
    f.hand[R].squeeze = 0.2f;
    run.Step(f);
    run.Step(f);
    const grasp::Result d = run.Step(f);
    Check("15b", "a regrip let go mid-glide with the left still on the handguard: back in the left's hold alone",
          d.letGo == R && d.toBarrel == L && d.released == -1 && d.holder == Holder::kBarrel && d.barrel == L, "%s",
          Describe(d).c_str());

    run.Reset();
    f = ToGlideBarrel(run, R, 2);
    f.hand[L].onSupport = false;
    const grasp::Result on = run.Step(f);
    f.hand[R].squeeze = 0.2f;
    run.Step(f);
    run.Step(f);
    const grasp::Result e = run.Step(f);
    Check("15c", "a regrip's glide goes on with the left off the handguard; then let go: it falls",
          on.holder == Holder::kGlide && on.barrel == L && e.letGo == R && e.released == R && e.holder == Holder::kLoose,
          "%s", Describe(e).c_str());
}

// ---- 16-18: back in the right hand ----
void Returns(Run& run, const char* id, const char* what, const Case* cases, int count, void (*spoil)(grasp::Frame&, bool),
             const char* why) {
    int bad = 0;
    std::string which;
    for (int k = 0; k < count; ++k) {
        run.Reset();
        grasp::Frame f = cases[k].make(run);
        const grasp::State was = run.Written();
        const int right = run.PhysRight();
        const bool there = was.holder == Holder::kHeld && was.trigger == right;
        spoil(f, true);
        const grasp::Result r = run.Step(f);
        const Span during = Hold(run, f, 36);
        spoil(f, false);
        const grasp::Result next = run.Step(f);
        const bool ok = r.holder == Holder::kHeld && r.trigger == right && r.barrel == -1 && !r.armed[0] && !r.armed[1] &&
                        r.fireHand == right && r.returned == !there && (!r.returned || std::strcmp(r.why, why) == 0) &&
                        during.changed == 0 && next.holder == Holder::kHeld && next.trigger == right && Quiet(next);
        if (!ok)
            ++bad, which += std::string(which.empty() ? "" : "; ") + cases[k].name + ": " + Describe(r);
    }
    Check(id, what, bad == 0, "%d of %d states otherwise%s%s", bad, count, bad ? ": " : "", which.c_str());
}

void Row16(Run& run) {
    Returns(run, "16", "a new life from every state: in the right hand, nothing armed, returned unless it was there", kStates,
            static_cast<int>(std::size(kStates)), [](grasp::Frame& f, bool on) { f.newLife = on; }, "a new life");
}

void Row17(Run& run) {
    const Case cases[] = {kStates[6], kStates[1], kStates[0], kStates[2]};
    Returns(run, "17", "free hands off (another gun, seated, off) from lying, the left's and the right's pistol grip, the left's hold",
            cases, static_cast<int>(std::size(cases)), [](grasp::Frame& f, bool on) { f.enabled = !on; },
            "another gun, seated or free hands off");
}

void Row18(Run& run) {
    const Case cases[] = {kStates[6], kStates[0]};
    Returns(run, "18", "lost on the ground (no ground 5 s, numbers not finite): back in the right hand", cases,
            static_cast<int>(std::size(cases)), [](grasp::Frame& f, bool on) { f.lost = on; }, "the ground lost it");
}

// ---- 19-21: scripted moments, the loose start refused, the game's reload ----
void Row19(Run& run) {
    grasp::Frame f = Base();
    f.hand[R].squeeze = 0.9f;
    run.Step(f);
    f.frozen = true;
    f.hand[R].squeeze = 0.0f;
    const Span during = Hold(run, f, 72);
    f.frozen = false;
    const Span after = Hold(run, f, 72);
    const grasp::Result drop = Drop(run, f, R);
    Check("19a", "held, a scripted moment with the grip open for a second: not let go; after it, still held until squeezed and let go",
          during.changed == 0 && after.changed == 0 && drop.released == R, "%d frames changed; then %s",
          during.changed + after.changed, Describe(drop).c_str());

    run.Reset();
    f = ToLoose(run);
    f.frozen = true;
    NearLying(f, L, 0.10f);
    const grasp::Result take = Fresh(run, f, L);
    const Span lying = Hold(run, f, 36);
    Check("19b", "lying, a fresh squeeze 10 cm from it in a scripted moment: not taken",
          take.holder == Holder::kLoose && Quiet(take) && lying.changed == 0, "%s", Describe(lying.last).c_str());

    run.Reset();
    f = ToBarrel(run, L);
    f.frozen = true;
    f.hand[L].onSupport = false;
    const Span kept = Hold(run, f, 36);
    f.frozen = false;
    const grasp::Result fell = run.Step(f);
    Check("19c", "held by the left's hold alone, off the handguard in a scripted moment: stays; after it, falls",
          kept.changed == 0 && fell.released == L, "%d frames changed; then %s", kept.changed, Describe(fell).c_str());

    run.Reset();
    f = ToBarrel(run, L);
    f.hand[R].squeeze = 0.0f;
    run.Steps(f, 36);
    f.frozen = true;
    NearGrip(f, R, 0.05f);
    const grasp::Result regrip = Fresh(run, f, R);
    const Span barrel = Hold(run, f, 36);
    Check("19d", "held by the left's hold alone, the right squeezes at its pistol grip in a scripted moment: no regrip",
          regrip.holder == Holder::kBarrel && Quiet(regrip) && barrel.changed == 0, "%s", Describe(barrel.last).c_str());

    run.Reset();
    f = ToGlideWorld(run, L, 3);
    f.frozen = true;
    int arrivedAt = -1;
    for (int n = 4; n <= 12 && arrivedAt < 0; ++n)
        if (run.Step(f).arrived == L)
            arrivedAt = n;
    Check("19e", "a glide under way finishes in a scripted moment", arrivedAt == 9, "arrived %d frames after the take", arrivedAt);
}

bool SameState(const grasp::State& a, const grasp::State& b) {
    bool same = a.holder == b.holder && a.trigger == b.trigger && a.barrel == b.barrel && a.glideTime == b.glideTime &&
                a.glideFromWorld == b.glideFromWorld && a.headShare == b.headShare && a.clock == b.clock &&
                a.beforeHolder == b.beforeHolder && a.beforeTrigger == b.beforeTrigger && a.beforeBarrel == b.beforeBarrel;
    for (int i = 0; i < 2; ++i)
        same = same && a.armed[i] == b.armed[i] && a.fireReset[i] == b.fireReset[i] && a.low[i] == b.low[i] &&
               a.armTake[i] == b.armTake[i] && a.crossedAt[i] == b.crossedAt[i] && a.letGoAt[i] == b.letGoAt[i];
    return same;
}

void Row20(Run& run) {
    grasp::Frame f = Base();
    const grasp::Result fell = Drop(run, f, R);
    run.NotLetGo();
    grasp::State s = run.Written();
    const Span open = Hold(run, f, 72);
    const grasp::Result again = Drop(run, f, R);
    Check("20a", "the loose start refused after the right let go: back on the right's pistol grip, unarmed (an open grip keeps it)",
          fell.released == R && s.holder == Holder::kHeld && s.trigger == R && s.barrel == -1 && !s.armed[0] && !s.armed[1] &&
              s.low[R] == 0 && open.changed == 0 && again.released == R,
          "%d frames changed; then %s", open.changed, Describe(again).c_str());

    run.Reset();
    f = ToHeld(run, L);
    const grasp::Result left = Drop(run, f, L);
    run.NotLetGo();
    s = run.Written();
    Check("20b", "after the left let go of its pistol grip: back in the left's hand, unarmed",
          left.released == L && s.holder == Holder::kHeld && s.trigger == L && !s.armed[0] && !s.armed[1]);

    run.Reset();
    f = ToBarrel(run, L);
    f.hand[L].onSupport = false;
    const grasp::Result off = run.Step(f);
    run.NotLetGo();
    s = run.Written();
    f.hand[L].onSupport = true;
    const Span held = Hold(run, f, 36);
    Check("20c", "after the left let go of the handguard: back in the left's hold, held there while it holds on",
          off.released == L && s.holder == Holder::kBarrel && s.barrel == L && s.trigger == -1 && !s.armed[0] && !s.armed[1] &&
              held.changed == 0,
          "%d frames changed", held.changed);

    run.Reset();
    f = Base();
    run.Step(f);
    const grasp::State before = run.Written();
    run.NotLetGo();
    const grasp::State after = run.Written();
    Check("20d", "with nothing let go of: changes nothing", SameState(before, after));
}

void Row21(Run& run) {
    for (int side = 0; side < 2; ++side) {
        if (side)
            run.Reset();
        grasp::Frame f = side == R ? ToHeld(run, R) : ToHeld(run, L);
        f.gameReload = true;
        f.hand[side].squeeze = 0.0f;
        const Span during = Hold(run, f, 180);
        f.gameReload = false;
        const Span after = Hold(run, f, 72);
        const grasp::Result drop = Drop(run, f, side);
        Check(side == R ? "21a" : "21b",
              side == R ? "the game's reload (X) for 2.5 s with the right's grip open: disarmed, never let go, then or after"
                        : "the same with the left on the pistol grip",
              during.changed == 0 && during.armed == 0 && after.changed == 0 && drop.released == side,
              "%d frames changed, %d armed; then squeezed and let go: %s", during.changed + after.changed, during.armed,
              Describe(drop).c_str());
    }
}

// ---- 22: broken numbers ----
grasp::Frame Spoiled(grasp::Frame f, float v) {
    f.dt = v;
    for (grasp::HandIn& h : f.hand) {
        h.squeeze = h.trigger = v;
        for (float& x : h.pose)
            x = v;
    }
    for (float& x : f.gunRoom)
        x = v;
    for (float& x : f.lyingRoom)
        x = v;
    for (auto& g : f.gripPoint)
        for (float& x : g)
            x = v;
    for (int i = 0; i < 3; ++i)
        f.boxMin[i] = f.boxMax[i] = v;
    f.glide = f.pickReach = f.regripReach = v;
    return f;
}

bool FiniteState(const grasp::State& s) {
    return std::isfinite(s.clock) && std::isfinite(s.glideTime) && std::isfinite(s.headShare) && std::isfinite(s.crossedAt[0]) &&
           std::isfinite(s.crossedAt[1]) && std::isfinite(s.letGoAt[0]) && std::isfinite(s.letGoAt[1]);
}

void Row22(Run& run) {
    const float bad[3] = {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(),
                          -std::numeric_limits<float>::infinity()};
    int wrong = 0, tried = 0;
    std::string which;
    for (const Case& c : kStates)
        for (const float v : bad) {
            run.Reset();
            const grasp::Frame f = c.make(run);
            const grasp::Result good = run.Step(f);
            const grasp::Frame g = Spoiled(f, v);
            bool ok = true;
            for (int i = 0; i < 36; ++i) {
                const grasp::Result r = run.Step(g);
                ok = ok && Quiet(r) && r.holder == good.holder && r.trigger == good.trigger && r.barrel == good.barrel &&
                     r.fireHand == good.fireHand && r.glide == good.glide && std::isfinite(r.headShare) &&
                     std::isfinite(r.reach);
            }
            ok = ok && FiniteState(run.Written());
            ++tried;
            if (!ok)
                ++wrong, which += std::string(which.empty() ? "" : "; ") + c.name + " " + std::to_string(v);
        }
    Check("22a", "NaN, +inf or -inf in every number of the frame for 0.5 s, from every state: nothing changes hands, all finite",
          wrong == 0, "%d of %d otherwise%s%s", wrong, tried, wrong ? ": " : "", which.c_str());

    // Each alone.
    const float nan = bad[0];
    std::string missed;
    const auto expect = [&missed](bool ok, const char* what) {
        if (!ok)
            missed += std::string(missed.empty() ? "" : "; ") + what;
    };
    run.Reset();
    grasp::Frame f = ToHeld(run, R);
    f.hand[R].squeeze = nan;
    expect(Hold(run, f, 36).changed == 0, "the right's grip NaN (its let-go)");
    f.hand[R].squeeze = 0.2f;
    expect(Hold(run, f, 36).changed == 0, "then open, disarmed by it");
    run.Reset();
    f = ToHeld(run, R);
    f.hand[R].squeeze = 0.2f;
    Put(f.hand[R].pose, Moved(RightGrip(), {nan, 0.95f, -0.35f}));
    expect(Hold(run, f, 36).changed == 0, "the right's pose NaN with its grip open");
    run.Reset();
    f = ToHeld(run, R);
    f.hand[R].squeeze = 0.2f;
    f.gunRoom[5] = nan;
    expect(Hold(run, f, 36).changed == 0, "the rifle's pose NaN with the grip open");
    run.Reset();
    f = ToLoose(run);
    NearLying(f, R, 0.10f);
    f.hand[R].trigger = 0.8f;
    Fresh(run, f, R);
    run.Steps(f, 10);  // in the hand, its trigger held through the take: not firing
    f.hand[R].trigger = nan;
    expect(Hold(run, f, 10).fires == 0, "the right's trigger NaN after a pickup with it held (still not firing)");
    run.Reset();
    f = Base();
    f.hand[R].trigger = nan;
    expect(Hold(run, f, 10).fires == 10, "the right's trigger NaN at the spawn (still firing)");
    run.Reset();
    f = ToGlideWorld(run, L, 1);
    const grasp::State glideBefore = run.Written();
    f.dt = nan;
    run.Steps(f, 5);
    expect(run.Written().glideTime == glideBefore.glideTime && FiniteState(run.Written()), "dt NaN (time stands)");
    run.Reset();
    f = ToGlideWorld(run, L, 0);
    f.glide = nan;
    int arrived = -1;
    for (int n = 1; n <= 12 && arrived < 0; ++n)
        if (run.Step(f).arrived == L)
            arrived = n;
    expect(arrived == 9, "the glide's length NaN (0.12 s)");
    run.Reset();
    f = ToLoose(run);
    NearLying(f, L, 0.10f);
    f.lyingRoom[3] = nan;
    expect(Fresh(run, f, L).holder == Holder::kLoose && Hold(run, f, 36).changed == 0, "the lying pose NaN (no pickup)");
    run.Reset();
    f = ToLoose(run);
    NearLying(f, L, 0.10f);
    f.boxMax[1] = nan;
    expect(Fresh(run, f, L).holder == Holder::kLoose && Hold(run, f, 36).changed == 0, "the box NaN (no pickup)");
    run.Reset();
    f = ToLoose(run);
    NearLying(f, L, 0.10f);
    f.pickReach = nan;
    expect(Fresh(run, f, L).pickedUp == L, "the pickup's reach NaN (25 cm: taken at 10)");
    run.Reset();
    f = ToBarrel(run, L);
    f.hand[R].squeeze = 0.0f;
    run.Steps(f, 36);
    NearGrip(f, R, 0.05f);
    f.gripPoint[R][2] = nan;
    expect(Fresh(run, f, R).holder == Holder::kBarrel && Hold(run, f, 36).changed == 0, "the right's grip point NaN (no regrip)");
    Check("22b", "NaN in each number alone: nothing changes hands by it", missed.empty(), "%s",
          missed.empty() ? "all held" : ("otherwise: " + missed).c_str());
}

// ---- 23-24: spent squeezes, the aim's share ----
void Row23(Run& run) {
    grasp::Frame f = ToLoose(run);
    int spent = 0, takes = 0;
    const auto tally = [&](const grasp::Result& r) {
        spent += r.spent[0] + r.spent[1];
        takes += (r.pickedUp >= 0) + (r.regrip >= 0);
    };
    NearLying(f, L, 0.1f);
    tally(Fresh(run, f, L));
    for (int i = 0; i < 72; ++i)
        tally(run.Step(f));  // picked up and still squeezing
    f.lying = false;
    Put(f.gunRoom, GunIn(f, L));
    f.hand[R].onSupport = true;
    f.hand[L].squeeze = 0.2f;
    for (int i = 0; i < 3; ++i)
        tally(run.Step(f));  // let go: in the right's hold
    f.hand[L].squeeze = 0.0f;
    for (int i = 0; i < 30; ++i)
        tally(run.Step(f));
    NearGrip(f, L, 0.05f);
    f.hand[L].squeeze = 0.9f;
    for (int i = 0; i < 72; ++i)
        tally(run.Step(f));  // the regrip, still squeezing
    Check("23", "a fresh squeeze is spent only on the frame it takes the rifle (a pickup and a regrip, each held a second)",
          spent == 2 && takes == 2, "%d frames spent, %d takes", spent, takes);
}

void Row24(Run& run) {
    grasp::Frame f = Base();
    const grasp::Result drop = Drop(run, f, R);
    f.hand[R].squeeze = 0.0f;
    int full = -1;
    float prev = drop.headShare;
    bool rising = drop.released == R && drop.headShare == 0.0f;
    for (int n = 1; n <= 30; ++n) {
        const grasp::Result r = run.Step(f);
        rising = rising && r.headShare >= prev && Near(r.headShare, std::min(1.0f, n * kDt / grasp::kHeadEase), 1e-4f);
        prev = r.headShare;
        if (full < 0 && r.headShare == 1.0f)
            full = n;
    }
    Check("24a", "dropped: the game's aim goes to the head, all of it within 0.25 s",
          rising && full > 0 && full * kDt <= 0.25f + kDt + 1e-4f, "all at the head %.3f s after the drop", full * kDt);
    f.lying = true;
    run.Steps(f, 36);
    NearLying(f, R, 0.1f);
    grasp::Result r = Fresh(run, f, R);
    bool easing = r.pickedUp == R && r.headShare == 1.0f;
    prev = 1.0f;
    for (int n = 1; n <= 20 && r.arrived < 0; ++n) {
        r = run.Step(f);
        if (r.arrived < 0) {
            easing = easing && r.headShare <= prev && Near(r.headShare, 1.0f - grasp::Smooth(n * kDt / 0.12f), 1e-5f);
            prev = r.headShare;
        }
    }
    Check("24b", "picked up: all at the head at the take, back to the gun as it glides in, none at its arrival",
          easing && r.arrived == R && r.headShare == 0.0f, "%s", Describe(r).c_str());
}

// ---- 25: the film rounds ----
struct Event {
    double t;
    char kind;  // g let go, r released, b to barrel, p picked up, R regrip, a arrived, x returned
    int side;
};

void Events(const grasp::Result& r, double t, std::vector<Event>& out) {
    if (r.letGo >= 0)
        out.push_back({t, 'g', r.letGo});
    if (r.released >= 0)
        out.push_back({t, 'r', r.released});
    if (r.toBarrel >= 0)
        out.push_back({t, 'b', r.toBarrel});
    if (r.pickedUp >= 0)
        out.push_back({t, 'p', r.pickedUp});
    if (r.regrip >= 0)
        out.push_back({t, 'R', r.regrip});
    if (r.arrived >= 0)
        out.push_back({t, 'a', r.arrived});
    if (r.returned)
        out.push_back({t, 'x', R});
}

// Test N's rounds through the machine with the hands placed as hands.cpp places them: the head fixed, each Where
// made a grip pose, the support held while a hand squeezes on the handguard of a held rifle, the rifle drawn at
// the trigger hand, frozen on the barrel hand, blended in glides, lying on the floor 0.6 s after a release.
// made: every frame given to the machine (the timing row's).
std::vector<Event> Film(Run& run, int test, int rounds, std::vector<grasp::Frame>* made = nullptr) {
    const Pose head = Moved(Identity(), {0.0f, 1.2f, 0.0f});
    const Pose standIn = Moved(Turn(-20.0f, 10.0f, 5.0f), {0.15f, -0.30f, -0.35f});  // head space
    const Pose front = Moved(standIn, {0.12f, -0.15f, -0.30f});
    const float guard[2][3] = {{0.02f, -0.03f, 0.55f}, {-0.02f, -0.03f, 0.55f}};  // each side's hold on the handguard (model)
    const Pose floor = Floor();
    grasp::Frame f = Base();
    Pose hand[2] = {Mul(head, Mirror(standIn)), Mul(head, standIn)};
    Where was[2] = {Where::kNone, Where::kNone};
    Pose gun = PlaceAt(hand[R], f.gripPoint[R], Pos(hand[R]));
    Pose onBarrel = Identity(), glideFrom = Identity();
    bool lying = false;
    double releasedAt = -1.0;
    grasp::Result last;  // the spawn: in the right hand
    std::vector<Event> events;
    const int count = static_cast<int>(std::lround(freetest::kRound[test] * rounds * 72.0));
    for (int n = 0; n < count; ++n) {
        const double t = n / 72.0;
        const float into = static_cast<float>(std::fmod(t, static_cast<double>(freetest::kRound[test])));
        freetest::Step step[2];
        freetest::At(test, into, step);
        const bool held = last.holder == Holder::kHeld || last.holder == Holder::kBarrel ||
                          (last.holder == Holder::kGlide && !last.glideFromWorld);
        for (int s = 0; s < 2; ++s) {
            const freetest::Step& st = step[s];
            switch (st.where) {
            case Where::kNone:
                break;
            case Where::kStandIn:
                hand[s] = Mul(head, standIn);
                break;
            case Where::kMirror:
                hand[s] = Mul(head, Mirror(standIn));
                break;
            case Where::kFront:
                hand[s] = Mul(head, s == R ? front : Mirror(front));
                break;
            case Where::kGuard:
                if (was[s] != Where::kGuard)
                    hand[s] = Mul(gun, Moved(Identity(), {guard[s][0], guard[s][1], guard[s][2]}));
                break;
            case Where::kGrip:
                if (was[s] != Where::kGrip)
                    hand[s] = Moved(hand[s], Apply(gun, {f.gripPoint[s][0], f.gripPoint[s][1], f.gripPoint[s][2]}));
                break;
            }
            was[s] = st.where;
            grasp::HandIn& h = f.hand[s];
            h.tracked = h.own = h.read = st.where != Where::kNone;
            h.squeeze = st.squeeze, h.trigger = st.trigger, h.forceTake = st.forceTake;
            h.onSupport = st.where == Where::kGuard && st.squeeze >= 0.6f && held;
            Put(h.pose, hand[s]);
        }
        f.haveGun = last.holder != Holder::kLoose || lying;
        Put(f.gunRoom, gun);
        f.lying = lying;
        if (made)
            made->push_back(f);
        const grasp::Result r = run.Step(f);
        Events(r, t, events);
        if (r.toBarrel >= 0)
            onBarrel = Mul(Inv(hand[r.toBarrel]), gun);
        if (r.released >= 0)
            releasedAt = t, lying = false;
        if (r.pickedUp >= 0)
            glideFrom = floor, lying = false;
        if (r.regrip >= 0)
            glideFrom = gun;
        switch (r.holder) {
        case Holder::kHeld:
            gun = PlaceAt(hand[r.trigger], f.gripPoint[r.trigger], Pos(hand[r.trigger]));
            break;
        case Holder::kBarrel:
            gun = Mul(hand[r.barrel], onBarrel);
            break;
        case Holder::kGlide:
            gun = Blend(glideFrom, PlaceAt(hand[r.trigger], f.gripPoint[r.trigger], Pos(hand[r.trigger])), r.glide);
            break;
        case Holder::kLoose:
            if (!lying && t - releasedAt >= 0.6 - 1e-9)
                lying = true;
            if (lying)
                gun = floor;
            break;
        }
        last = r;
    }
    return events;
}

void Row25(Run& run) {
    // The scripts' own steps.
    freetest::Step s[2];
    std::string wrong;
    const auto expect = [&wrong](bool ok, const char* what) {
        if (!ok)
            wrong += std::string(wrong.empty() ? "" : "; ") + what;
    };
    expect(freetest::kRound[0] == 0.0f && freetest::kRound[1] == 26.0f && freetest::kRound[2] == 50.0f &&
               freetest::kRound[3] == 12.0f,
           "the rounds 26, 50, 12 s");
    freetest::At(1, 0.5f, s);
    expect(s[R].where == Where::kStandIn && s[R].squeeze == 1.0f && s[L].where == Where::kNone, "1 at 0.5 s");
    freetest::At(1, 6.0f, s);
    expect(s[L].where == Where::kMirror && s[L].squeeze == 1.0f && s[L].trigger == 1.0f && s[R].where == Where::kStandIn &&
               s[R].squeeze == 0.0f,
           "1 at 6 s: the left's trigger pulled");
    freetest::At(1, 5.6f, s);
    expect(Near(s[L].trigger, 0.2f, 1e-4f), "1 at 5.6 s: the left's trigger on its way");
    freetest::At(1, 16.1f, s);
    expect(s[R].forceTake && s[R].squeeze == 1.0f && !s[L].forceTake, "1 at 16.1 s: the right's take");
    freetest::At(1, 20.0f, s);
    expect(s[L].where == Where::kGuard && s[L].squeeze == 1.0f && s[R].where == Where::kStandIn, "1 at 20 s");
    freetest::At(2, 1.1f, s);
    expect(s[R].sensed && s[R].velocity[0] == 0.3f && s[R].velocity[1] == 1.5f && s[R].velocity[2] == -3.0f &&
               s[R].spin[0] == 6.0f && s[R].spin[1] == 0.0f && s[R].squeeze == 0.0f,
           "2 at 1.1 s: the throw");
    freetest::At(2, 1.2f, s);
    expect(!s[R].sensed && s[R].where == Where::kStandIn, "2 at 1.2 s: thrown");
    freetest::At(3, 4.0f, s);
    expect(s[R].where == Where::kFront && s[R].squeeze == 1.0f && s[R].thumb == 1.0f && s[R].trigger == 0.0f &&
               s[L].where == Where::kFront && s[L].squeeze == 1.0f && s[L].thumb == 1.0f,
           "3 at 4 s: both pointing");
    freetest::At(3, 6.0f, s);
    expect(s[R].trigger == 1.0f && s[L].trigger == 1.0f && s[L].squeeze == 1.0f, "3 at 6 s: both fists");
    freetest::At(3, 9.5f, s);
    expect(s[R].where == Where::kStandIn && s[L].where == Where::kNone, "3 at 9.5 s");
    for (const float t : {-1.0f, 26.0f, std::numeric_limits<float>::quiet_NaN()}) {
        freetest::At(1, t, s);
        expect(s[0].where == Where::kNone && s[1].where == Where::kNone && s[1].squeeze == 0.0f, "1 outside its round");
    }
    freetest::At(4, 1.0f, s);
    expect(s[0].where == Where::kNone && s[1].where == Where::kNone, "no test 4");
    Check("25a", "the scripts' steps (freetest::At)", wrong.empty(), "%s", wrong.empty() ? "as written" : wrong.c_str());

    struct Expect {
        double t;
        char kind;
        int side;
    };
    const std::vector<Expect> want[4] = {
        {},
        {{1.04, 'g', R}, {1.04, 'r', R}, {5.0, 'p', L}, {5.12, 'a', L}, {9.04, 'g', L}, {9.04, 'b', R},
         {11.0, 'R', L}, {11.12, 'a', L}, {14.04, 'g', L}, {14.04, 'r', L}, {16.0, 'p', R}, {16.12, 'a', R},
         {21.04, 'g', R}, {21.04, 'b', L}, {23.0, 'R', R}, {23.12, 'a', R}},
        {{1.04, 'g', R}, {1.04, 'r', R}, {45.0, 'p', R}, {45.12, 'a', R}},
        {{0.54, 'g', R}, {0.54, 'r', R}, {9.0, 'p', R}, {9.12, 'a', R}},
    };
    for (int test = 1; test <= 3; ++test) {
        run.Reset();
        const std::vector<Event> got = Film(run, test, 2);
        std::vector<Expect> all;
        for (int round = 0; round < 2; ++round)
            for (const Expect& e : want[test])
                all.push_back({e.t + round * freetest::kRound[test], e.kind, e.side});
        bool ok = got.size() == all.size();
        double worst = 0.0;
        for (size_t i = 0; i < got.size() && i < all.size(); ++i) {
            ok = ok && got[i].kind == all[i].kind && got[i].side == all[i].side;
            worst = std::max(worst, std::fabs(got[i].t - all[i].t));
        }
        ok = ok && worst <= 2.0 / 72.0 + 1e-6;
        std::string list;
        for (const Event& e : got) {
            char b[32];
            std::snprintf(b, sizeof(b), "%s%.3f %c%d", list.empty() ? "" : ", ", e.t, e.kind, e.side);
            list += b;
        }
        char id[8], what[96];
        std::snprintf(id, sizeof(id), "25%c", 'a' + test);
        std::snprintf(what, sizeof(what), "test %d, two rounds of %.0f s: every expected event in order, within 2 frames", test,
                      freetest::kRound[test]);
        Check(id, what, ok, "%zu events (%zu expected), the worst %.3f s off%s%s", got.size(), all.size(), worst,
              ok ? "" : ": ", ok ? "" : list.c_str());
    }
}

// ---- 26: the time ----
void Row26(Run& run) {
    std::vector<grasp::Frame> frames;
    Run scratch(run.mirrored);
    Film(scratch, 1, 1, &frames);
    if (run.mirrored)
        for (grasp::Frame& f : frames)
            f = MirrorFrame(f);
    grasp::State s;
    grasp::Clear(s);
    if (run.mirrored)
        s = MirrorState(s);
    constexpr int kUpdates = 100000;
    float sum = 0.0f;
    const auto a = std::chrono::steady_clock::now();
    for (int i = 0; i < kUpdates; ++i) {
        const grasp::Result r = grasp::Update(frames[i % frames.size()], s);
        sum += r.glide + r.headShare + static_cast<float>(r.holder);
    }
    const auto b = std::chrono::steady_clock::now();
    static volatile float sink;
    sink = sum;
    const double ms = std::chrono::duration<double, std::milli>(b - a).count();
    Check("26", "1e5 updates under 50 ms (test 1's frames over and over)", ms < 50.0, "%.2f ms, %.3f us an update", ms,
          ms * 1000.0 / kUpdates);
}

// ---- Each row as written and mirrored ----
// The two runs' results, frame by frame, up to each segment's first return (after it the mirrored run's right hand
// is the other side of the script).
void Compare(const std::string& id, const Run& a, const Run& b) {
    int compared = 0, differ = 0;
    std::string first;
    int skip = -1;
    const bool sameSize = a.log.size() == b.log.size();
    for (size_t i = 0; i < a.log.size() && i < b.log.size(); ++i) {
        const Entry& x = a.log[i];
        const Entry& y = b.log[i];
        if (x.segment != y.segment || x.frame != y.frame) {
            ++differ, first = "the runs' frames out of step";
            break;
        }
        if (x.segment == skip)
            continue;
        if (x.cleared || y.cleared) {
            skip = x.segment;
            continue;
        }
        ++compared;
        if (!Same(x.r, y.r) && !differ++)
            first = "segment " + std::to_string(x.segment) + " frame " + std::to_string(x.frame) + ": " + Describe(x.r) +
                    " against " + Describe(y.r);
    }
    g_mirrored = false;
    Check((id + "=").c_str(), "mirrored, the same results frame by frame", sameSize && differ == 0, "%d frames compared%s%s%s",
          compared, sameSize ? "" : ", the runs' lengths differ", differ ? "; first differing: " : "", first.c_str());
}

void Row(const char* id, void (*row)(Run&)) {
    Run a(false), b(true);
    g_mirrored = false;
    row(a);
    g_mirrored = true;
    row(b);
    for (const Run* run : {&a, &b}) {
        g_mirrored = run->mirrored;
        Check((std::string(id) + "i").c_str(), "every frame by the states' rules", run->bad == 0, "%zu frames, %d otherwise%s%s",
              run->log.size(), run->bad, run->bad ? ", the first: " : "", run->first.c_str());
    }
    Compare(id, a, b);
}

}  // namespace

int main() {
    std::printf("grasptest: src/grasp.h, who holds the rifle with free hands, at 72 Hz, every row also mirrored. A hand takes "
                "it past %.1f (fresh: within %.1f s of crossing it from under %.1f, not within %.2f s of its own let-go); the "
                "pistol-grip hand lets go under %.2f for %d frames once armed; a take glides in over 0.12 s from within 25 cm "
                "of the box or 15 cm of the pistol grip; the aim goes to the head over %.2f s.\n",
                grasp::kTake, grasp::kReachWindow, grasp::kArm, grasp::kRetakeAfter, grasp::kLetGo, grasp::kLetGoFrames,
                grasp::kHeadEase);
    Row("1", Row1);
    Row("2", Row2);
    Row("3", Row3);
    Row("4", Row4);
    Row("5", Row5);
    Row("6", Row6);
    Row("7", Row7);
    Row("8", Row8);
    Row("9", Row9);
    Row("10", Row10);
    Row("11", Row11);
    Row("12", Row12);
    Row("13", Row13);
    Row("14", Row14);
    Row("15", Row15);
    Row("16", Row16);
    Row("17", Row17);
    Row("18", Row18);
    Row("19", Row19);
    Row("20", Row20);
    Row("21", Row21);
    Row("22", Row22);
    Row("23", Row23);
    Row("24", Row24);
    Row("25", Row25);
    Row("26", Row26);
    std::printf("%d of %d checks failed\n", g_failed, g_counted);
    return g_failed;
}

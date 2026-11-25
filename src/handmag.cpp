#include "handmag.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>

#include "pouch.h"

namespace handmag {
namespace {

constexpr float kPi = 3.14159265f;
constexpr float kDegree = kPi / 180.0f;
constexpr float kArm = 0.4f, kTake = 0.6f;   // a squeeze: armed below, taken crossing
// The hand's middle in the gloves' bind space (pouch.cpp's kHandInGlove), and the way its palm faces. By side: [1] the right glove's, the left's mirror
// image (x negated), as pouch.cpp's.
constexpr float kHandInGlove[2][3] = {{0.5363f, 1.0976f, 0.2092f}, {-0.5363f, 1.0976f, 0.2092f}};
constexpr float kPalmInGlove[2][3] = {{-0.639f, -0.748f, -0.179f}, {0.639f, -0.748f, -0.179f}};
// A part this near the gun's middle plane (its own middle's x on the frame) is mirrored for the right about the gun's middle: a magazine, a T handle.
constexpr float kOnMiddle = 0.02f;
// Taking the seated one, and not the gun (the user, 2026-11-05: "when I'm trying to grip the gun it takes the magazine out"): the hand's middle this near the magazine's middle and not forward of it, where the
// handguard starts; and a palm facing up there is a hand going for the handguard (the game's own hand faces 0.75 to 0.9 up on it, 0 on a magazine), unless the hand's well down the magazine, under the handguard's reach.
constexpr float kGrabReach = 0.09f, kGrabForward = 0.045f, kGrabLow = 0.025f, kPalmUp = 0.5f;
// Pulled this far down from where it was taken, it comes free; meanwhile it rocks forward about its hook by up to this. Only down: a hand going along the gun isn't pulling it.
constexpr float kPullOut = 0.05f, kPullDead = 0.006f, kRockOut = 20.0f * kDegree;
// Gone up or forward from there instead, with no pull yet: the hand is going for the gun.
constexpr float kToGripUp = 0.03f, kToGripForward = 0.05f, kToGripAway = 0.20f, kToGripPull = 0.01f;
// A box with a way (BoxWay; the review of 2026-11-18: pulled straight down it came free after 19.6 cm, leaping 8 cm along its way in a frame, the glove
// left on it 20 cm from the real hand): the hand's pull followed along the way from where the box is, the box going along it no further a frame than
// the hand itself has moved (and this much), never leaping to another part of it that a hand gone off it comes nearer to; the hand this far off the way,
// whatever the pull, leaves it in: the glove, drawn on the box, never further than that from the real hand by the way (pulled within about 50 degrees
// of it, it comes free first).
constexpr float kWaySlack = 0.001f, kWayOff = 0.08f;
// The knock. The fresh one's front corner against the old one's back edge, coming from behind it: within this across (their widths touch), and fast along the gun, measured over this many frames. The AN-94's own swings at
// 2 m/s. Slower, it stops against it, as far as this past it with the hand going on, and the old one gives a little.
std::atomic<float> g_knockSpeed{1.3f};
constexpr float kContactAcross = 0.03f, kContactAbove = 0.04f, kContactBelow = 0.01f;
constexpr int kSpeedFrames = 4;
constexpr float kWallMost = 0.15f;
constexpr float kGiveMost = 6.0f * kDegree, kGiveFade = 0.06f, kPressGive = 4.0f * kDegree, kPressFull = 0.06f;
// The magnet: with the well empty, its top this near the well's mouth (from any side, in the receiver too), turned less than this: it goes home by itself in this long, first to the mouth, hooked, then rocked home.
std::atomic<float> g_magnetReach{0.10f}, g_flyTime{0.15f};  // (the user tried 15 cm and 0.10 s: "a little too snappy"; these: "Yeah, that works", 2026-11-05)
constexpr float kMagnetTurn = 120.0f * kDegree, kFlySplit = 0.55f, kAfterFlick = 0.06f;
// Just pulled out: not back in until it's well clear, or gone this far and is coming back this much.
constexpr float kOutClear = 0.02f, kOutFar = 0.09f, kOutComeBack = 0.03f;
constexpr int kHeldFrames = 2;
constexpr float kLost = 1.0f;
constexpr float kMaxDropSpeed = 3.0f;
// The lever. The fist closes round this point in the gloves' bind space: in front of the palm, towards the fingers' roots. So the palm lies above a knob taken over the top, below one taken from under,
// beside the gun and not in it.
constexpr float kFistGrip[2][3] = {{0.5268f, 1.0601f, 0.2214f}, {-0.5268f, 1.0601f, 0.2214f}};  // ([1] the right glove's: x negated)
constexpr float kLeverReach = 0.12f, kLeverReachGrip = 0.10f, kLeverToCharge = 0.5f;  // how near it the trigger takes it, how near it the grip does; as much of its travel as chambers a round
constexpr float kLeverHeard = 0.02f, kLeverLost = 0.25f;
constexpr float kLeverSpring = 0.06f;  // s home
constexpr float kLeverSwingOut = 0.08f;  // s to swing a handle out as it's taken
float g_swing = 0.0f;  // how far it's swung out, 0-1
constexpr float kLeverPalmUnder = 0.25f;  // the palm this far up as it's taken: from below
bool g_leverUnder = false;
// A handle with a notch (the UMP-45's). Into it: drawn back this much of its travel, and the hand then lifted this much on the gun from its height where the pull got that far (the game's
// own hand rises 0.5 cm during the pull and 3.3 cm after it, turning it up: a lift at the back, never the pull's own rise). A pull that rises as it goes (a hand drawing back and up at 10
// or 15 degrees, or one that took the handle from under its knob) isn't a lift: while the hand still goes further back, the height the lift is measured from follows its rise, as much as
// kNotchSlope of the way back (20 degrees; a steeper rise is a lift). It turns in this long. Out of it: the hand's middle this near the handle and coming down on the gun this fast (the game's own hand knocks it down), or a fresh squeeze
// or trigger there. The hand that put it there doesn't knock it out on its way off to the magazine: a hand has to have been clear of it once (or unseen), and to come at it from above
// or beside, not from under it.
constexpr float kNotchAt = 0.85f, kNotchLift = 0.02f, kNotchTurn = 0.06f, kNotchSlope = 0.364f;
constexpr float kKnockClear = 0.10f, kKnockUnder = 0.02f;
bool g_knockArmed = false;
constexpr float kKnockReach = 0.08f, kKnockDown = 0.3f, kKnockMost = 8.0f;  // (faster than a hand moves: a jump of the tracking, no knock)
bool g_notched = false;   // it rests in its notch
float g_notch = 0.0f;     // how far it's turned up, 0-1
bool g_notchBack = false;  // the pull's that far back
float g_notchFromY = 0.0f, g_notchBackZ = 0.0f;  // the hand's height the lift's measured from; as far back as the hand's been since
float g_palmWas[3] = {};  // the hand's middle on the gun last frame (the knock's speed)
bool g_havePalmWas = false;
// The flick: along the gun's axes (x left, y up, z forward), m/s, at the speed it takes. The AN-94's own goes 3.4 m/s forward; here it isn't lost from view at once. Harder, further.
constexpr float kFlick[3] = {-0.5f, 0.5f, 2.4f};

enum class State { kNone, kOnGun, kHand, kFly, kLever, kLid, kBelt };
// A belt fed gun. The lid: how near its back end the hand takes it; how long it takes to go the rest of the way by itself, let go of; how far up (of all the way) it has to be for that to be up, and not shut. The belt: how near its end
// the hand takes it, how near the feed it goes in by itself, how long that takes.
// (The user, 2026-11-05: the lid "also snappy": past this much of the way it goes the rest by itself, out of the hand; the belt "very magnetically snappy": near the feed it's taken from the hand.)
constexpr float kLidReach = 0.11f, kLidSwing = 0.09f, kLidSnapUp = 0.30f, kLidSnapShut = 0.60f, kLidFrontGap = 0.05f, kLidPastFront = 0.02f;
constexpr float kBeltReach = 0.09f, kBeltMagnet = 0.10f, kBeltFly = 0.10f, kBeltLost = 0.50f, kBeltBlendIn = 0.25f;
float g_lid = 0.0f, g_lidAim = 0.0f, g_lidGrabAngle = 0.0f, g_lidGrabSlide = 0.0f;  // 0 shut, 1 all the way up; where it's going by itself; the hand's angle less its own, as it was taken; how far along it the hand took it
bool g_lidWasUp = false;
bool g_beltLaid = true, g_beltFlying = false;
float g_beltShare = 0.0f, g_beltFlown = 0.0f;  // from hanging to in the hand; on its way into the feed
float g_beltFrom[5][12] = {};                  // where it was as the feed took it
State g_state = State::kNone;
bool g_armed = true, g_triggerArmed = true;
float g_grabAt[3] = {};          // where the hand was when it took the seated one, on the gun
int g_heldFrames = 0;
bool g_justOut = false;
float g_outFar = 0.0f;
float g_fly = 0.0f, g_flyX = 1.0f, g_flyFrom[12] = {};
float g_lostFor = 0.0f, g_sinceFlick = 10.0f;
bool g_wasEmpty = false;
// The gun Update was last fed and where the hand holds it at the front, for Claims (the gun only compared, never read through); and whether the front was
// measured by the hand's palm too (Frame::havePalm).
const Gun* g_gunFed = nullptr;
float g_frontGrip[3] = {};
bool g_frontByPalm = false;
float g_gloveBlend = 0.0f, g_gloveInGun[12] = {};
// A box with a way (BoxWay), just come free: where it was on its way then (on the gun), and that it eases from there into the hand with the glove.
float g_wayFrom[12] = {};
bool g_wayEasing = false;
float g_wayAt = 0.0f;       // how far along its way off the box is, on the gun
float g_wayMove[3] = {};    // the hand's move from where it took it, last frame
// A wide box (Gun::wide) taken away from the game's own place on it: held where the hand took it, as the hand was then (the seated box in the glove, as
// a table's hold), not at the game's own place: on its way off, in the hand, and home again should it go back in; not a fresh one from the belly.
bool g_takeOwn = false;
float g_takeHold[12] = {};
float g_frontAt[kSpeedFrames][3] = {}, g_frontDt[kSpeedFrames] = {};
int g_frontNext = 0, g_frontCount = 0;
float g_frontThen[3] = {};
bool g_haveFrontThen = false, g_walled = false;
float g_give = 0.0f;
float g_gripAt[4][3] = {}, g_gripDt[4] = {};
int g_gripNext = 0, g_gripCount = 0;
bool g_leverSounded = false;
float g_leverGrabZ = 0.0f, g_leverPull = 0.0f, g_leverMost = 0.0f, g_leverSpring = -1.0f, g_leverGoing = 0.0f;
// Whose hand (Frame::side): the hand fed last (the one at work), and whether it was where a let go puts a magazine back (a hand no longer fed lets go of what
// it has as it was last seen).
int g_fedSide = 0;
bool g_atShoulderWas = false;

void Compose(const float* a, const float* b, float* out) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j)
            out[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        out[i * 4 + 3] += a[i * 4 + 3];
    }
}

void InvertRigid(const float* m, float* out) {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            out[i * 4 + j] = m[j * 4 + i];
    for (int i = 0; i < 3; ++i)
        out[i * 4 + 3] = -(out[i * 4] * m[3] + out[i * 4 + 1] * m[7] + out[i * 4 + 2] * m[11]);
}

void Transform(const float* m, const float* p, float* out) {
    for (int i = 0; i < 3; ++i)
        out[i] = m[i * 4] * p[0] + m[i * 4 + 1] * p[1] + m[i * 4 + 2] * p[2] + m[i * 4 + 3];
}

float Length(const float* d) {
    return std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
}

float Apart(const float* a, const float* b) {
    const float d[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    return Length(d);
}

bool Finite(const float* v, int n) {
    for (int i = 0; i < n; ++i)
        if (!std::isfinite(v[i]))
            return false;
    return true;
}

void ToQuat(const float* m, float* q) {
    const float trace = m[0] + m[5] + m[10];
    if (trace > 0.0f) {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        q[0] = (m[9] - m[6]) / s, q[1] = (m[2] - m[8]) / s, q[2] = (m[4] - m[1]) / s, q[3] = 0.25f * s;
    } else if (m[0] > m[5] && m[0] > m[10]) {
        const float s = std::sqrt(1.0f + m[0] - m[5] - m[10]) * 2.0f;
        q[0] = 0.25f * s, q[1] = (m[1] + m[4]) / s, q[2] = (m[2] + m[8]) / s, q[3] = (m[9] - m[6]) / s;
    } else if (m[5] > m[10]) {
        const float s = std::sqrt(1.0f + m[5] - m[0] - m[10]) * 2.0f;
        q[0] = (m[1] + m[4]) / s, q[1] = 0.25f * s, q[2] = (m[6] + m[9]) / s, q[3] = (m[2] - m[8]) / s;
    } else {
        const float s = std::sqrt(1.0f + m[10] - m[0] - m[5]) * 2.0f;
        q[0] = (m[2] + m[8]) / s, q[1] = (m[6] + m[9]) / s, q[2] = 0.25f * s, q[3] = (m[4] - m[1]) / s;
    }
}

void FromQuat(const float* q, float* m) {
    const float n = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    const float x = q[0] / n, y = q[1] / n, z = q[2] / n, w = q[3] / n;
    const float r[9] = {1 - 2 * (y * y + z * z), 2 * (x * y - z * w),     2 * (x * z + y * w),
                        2 * (x * y + z * w),     1 - 2 * (x * x + z * z), 2 * (y * z - x * w),
                        2 * (x * z - y * w),     2 * (y * z + x * w),     1 - 2 * (x * x + y * y)};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            m[i * 4 + j] = r[i * 3 + j];
}

// a's and b's turns blended, s 0 is a; the translation left for the caller.
void BlendTurn(const float* a, const float* b, float s, float* out) {
    float qa[4], qb[4];
    ToQuat(a, qa), ToQuat(b, qb);
    if (qa[0] * qb[0] + qa[1] * qb[1] + qa[2] * qb[2] + qa[3] * qb[3] < 0.0f)
        for (float& v : qb)
            v = -v;
    float q[4];
    for (int i = 0; i < 4; ++i)
        q[i] = qa[i] + (qb[i] - qa[i]) * s;
    FromQuat(q, out);
}

// How far apart two 3x4s' turns are (radians).
float TurnBetween(const float* a, const float* b) {
    float trace = 0.0f;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            trace += a[i * 4 + j] * b[i * 4 + j];
    return std::acos(std::clamp((trace - 1.0f) * 0.5f, -1.0f, 1.0f));
}

// The magazine turned about its front hook, bottom forward, by this much: from the seat.
void Rock(const Gun& gun, float angle, float* out) {
    const float c = std::cos(angle), s = std::sin(angle);
    // About x: y' = c y + s z, z' = -s y + c z: a point below the hook goes forward.
    const float r[12] = {1, 0, 0, 0, 0, c, s, 0, 0, -s, c, 0};
    std::memcpy(out, r, sizeof(r));
    float turned[3];
    Transform(out, gun.hook, turned);
    for (int i = 0; i < 3; ++i)
        out[i * 4 + 3] = gun.hook[i] - turned[i];
}

// On its way home: x 1 with its top at the mouth, rocked in by all of rockIn; 0 home.
void HookedAt(const Gun& gun, float x, float* out) {
    Rock(gun, gun.rockIn * x, out);
    out[7] += gun.mouth * x;
}

// A box's way (BoxWay) at this far along it, by its rows' first numbers: between the two rows about it, its turn blended and its move straight; before
// the first, the first; past the last, the last.
void WayAt(const float (*rows)[13], int count, float at, float* out) {
    int i = 0;
    while (i + 2 < count && rows[i + 1][0] < at)
        ++i;
    const float* a = rows[i] + 1;
    const float* b = rows[i + 1] + 1;
    const float span = rows[i + 1][0] - rows[i][0];
    const float s = span > 1e-6f ? std::clamp((at - rows[i][0]) / span, 0.0f, 1.0f) : 1.0f;
    BlendTurn(a, b, s, out);
    for (int k = 0; k < 3; ++k)
        out[k * 4 + 3] = a[k * 4 + 3] + (b[k * 4 + 3] - a[k * 4 + 3]) * s;
}

// How far along a box's way off the hand has pulled it: the nearest point of the way the held point goes (point, on the gun with the box seated: its grab
// point, or where the hand took a wide box far from it, whose own path differs from the grab point's as the box turns) to the hand's move since it took
// the box (on the gun), by the rows' measure, looked for only within reach of where the box was (from), so the box goes along the way as the hand does and
// never leaps to another part of it; never past the last row. off: how far the hand's move is from that point.
float WayAlong(const Gun& gun, const float* point, const float* move, float from, float reach, float& off) {
    const BoxWay& w = *gun.way;
    const float lo = from - reach, hi = from + reach;
    float best = 1e9f, along = from, a[3], b[3];
    auto grabAt = [&](int i, float* p) {
        Transform(w.off[i] + 1, point, p);
        for (int k = 0; k < 3; ++k)
            p[k] -= point[k];
    };
    grabAt(0, a);
    for (int i = 0; i + 1 < w.offCount; ++i) {
        grabAt(i + 1, b);
        const float s0 = w.off[i][0], s1 = w.off[i + 1][0], span = s1 - s0;
        if (s1 >= lo && s0 <= hi) {  // (this piece of it within reach of where the box was)
            const float tLo = span > 1e-6f ? std::clamp((lo - s0) / span, 0.0f, 1.0f) : 0.0f, tHi = span > 1e-6f ? std::clamp((hi - s0) / span, 0.0f, 1.0f) : 1.0f;
            const float e[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]}, m[3] = {move[0] - a[0], move[1] - a[1], move[2] - a[2]};
            const float ee = e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
            const float t = std::clamp(ee > 1e-12f ? (m[0] * e[0] + m[1] * e[1] + m[2] * e[2]) / ee : 0.0f, tLo, tHi);
            const float p[3] = {a[0] + e[0] * t - move[0], a[1] + e[1] * t - move[1], a[2] + e[2] * t - move[2]};
            const float apart = Length(p);
            if (apart < best)
                best = apart, along = s0 + span * t;
        }
        std::memcpy(a, b, sizeof(a));
    }
    off = best;
    return along;
}

// The magazine going home by itself, s 0 to 1: from where the hand had it to the mouth, hooked, along a straight line for its top; then rocked home. A box
// with a way (BoxWay): to where its way on starts, likewise; then along the way on, as the game's own comes in, home.
void FlyPose(const Gun& gun, float s, float* out) {
    if (s >= kFlySplit) {
        if (gun.way)
            WayAt(gun.way->on, gun.way->onCount, (s - kFlySplit) / (1.0f - kFlySplit) * gun.way->on[gun.way->onCount - 1][0], out);
        else
            HookedAt(gun, g_flyX * (1.0f - s) / (1.0f - kFlySplit), out);
        return;
    }
    const float u = s / kFlySplit, e = u * u * (3.0f - 2.0f * u);
    float hooked[12], a[3], b[3], turned[3];
    if (gun.way)
        std::memcpy(hooked, gun.way->on[0] + 1, sizeof(hooked));
    else
        HookedAt(gun, g_flyX, hooked);
    Transform(g_flyFrom, gun.top, a);
    Transform(hooked, gun.top, b);
    BlendTurn(g_flyFrom, hooked, e, out);
    out[3] = out[7] = out[11] = 0.0f;
    Transform(out, gun.top, turned);
    for (int i = 0; i < 3; ++i)
        out[i * 4 + 3] = a[i] + (b[i] - a[i]) * e - turned[i];
}

// The lid turned up by this much, on the frame.
void LidTurn(const BeltGun& b, float angle, float* out) {
    const float c = std::cos(angle), s = std::sin(angle), hy = b.lidHinge[0], hz = b.lidHinge[1];
    const float turn[12] = {1, 0, 0, 0, 0, c, -s, hy - (c * hy - s * hz), 0, s, c, hz - (s * hy + c * hz)};
    std::memcpy(out, turn, sizeof(turn));
}

// How far round the lid's hinge this point is, 0 behind it, up and over.
float LidAngle(const BeltGun& b, const float* onGun) {
    return std::atan2(onGun[1] - b.lidHinge[0], b.lidHinge[1] - onGun[2]);
}

// How near the lid the hand is, anywhere from its back end to near its hinge, wherever the lid is now. (lidGrab is where the left's fitted hand has its
// middle, beside the lid's back end: the right's is that mirrored across the gun's middle, beside it on the other side.)
float NearLid(const BeltGun& b, const float* onGun, int side = 0) {
    float turn[12], back[12], here[3];
    LidTurn(b, b.lidOpen * g_lid, turn);
    InvertRigid(turn, back);
    Transform(back, onGun, here);
    const float front = b.lidHinge[1] - kLidFrontGap;
    if (here[2] > front + kLidPastFront)  // over its hinge or beyond it lifting wouldn't open it
        return 1e9f;
    const float z = std::clamp(here[2], std::min(b.lidGrab[2], front), std::max(b.lidGrab[2], front));
    const float nearest[3] = {side == 1 ? -b.lidGrab[0] : b.lidGrab[0], b.lidGrab[1], z};
    return Apart(here, nearest);
}

// How far along the lid from its back end the hand is, as the fitted hand goes back: the glove stays by the real hand, not at the far end.
float LidSlide(const BeltGun& b, const float* onGun) {
    float turn[12], back[12], here[3];
    LidTurn(b, b.lidOpen * g_lid, turn);
    InvertRigid(turn, back);
    Transform(back, onGun, here);
    return std::clamp(here[2] - b.lidGrab[2], 0.0f, std::max(b.lidHinge[1] - kLidFrontGap - b.lidGrab[2], 0.0f));
}

// The belt with its end here (on the frame): a curve in the gun's cross section from here back to where it comes out of the box, as long as it is, bowing out to the side of the gun the box lets it out
// (the M240's and the M249's left, the PKP's right); each piece along it from its free end (BeltOrder: the bones it is), its rounds staying along the gun.
void BeltInHand(const BeltGun& b, const float* tip, float (*out)[12]) {
    float length[5], total = 0.0f;
    for (int i = 0; i < 5; ++i) {
        const float* piece = b.pieces[BeltOrder(b, i)];
        const float dx = piece[2] - piece[0], dy = piece[3] - piece[1];
        length[i] = std::sqrt(dx * dx + dy * dy);
        total += length[i];
    }
    float end[2] = {tip[0], tip[1]};
    float dx = b.anchor[0] - end[0], dy = b.anchor[1] - end[1], d = std::sqrt(dx * dx + dy * dy);
    const float most = total * 1.1f;
    if (d > most) {  // no further than the belt goes
        end[0] = b.anchor[0] - dx / d * most, end[1] = b.anchor[1] - dy / d * most;
        dx = b.anchor[0] - end[0], dy = b.anchor[1] - end[1], d = most;
    }
    d = std::max(d, 0.01f);
    const float sag = d < total ? d * std::sqrt(0.375f * (total / d - 1.0f)) : 0.0f;
    float nx = -dy / d, ny = dx / d;
    if ((nx < 0.0f) != (b.anchor[0] < 0.0f))  // (an anchor on the gun's left: as before, bit for bit)
        nx = -nx, ny = -ny;
    const float control[2] = {(end[0] + b.anchor[0]) * 0.5f + nx * sag * 2.0f, (end[1] + b.anchor[1]) * 0.5f + ny * sag * 2.0f};
    float node[6][3];
    float along = 0.0f;
    for (int i = 0; i <= 5; ++i) {
        const float t = along / total, u = 1.0f - t;
        node[i][0] = u * u * end[0] + 2.0f * u * t * control[0] + t * t * b.anchor[0];
        node[i][1] = u * u * end[1] + 2.0f * u * t * control[1] + t * t * b.anchor[1];
        node[i][2] = u * tip[2] + t * b.anchor[2];
        if (i < 5)
            along += length[i];
    }
    for (int i = 0; i < 5; ++i) {
        const int bone = BeltOrder(b, i);
        const float* piece = b.pieces[bone];
        const float own = std::atan2(piece[3] - piece[1], piece[2] - piece[0]);
        const float way = std::atan2(node[i + 1][1] - node[i][1], node[i + 1][0] - node[i][0]);
        const float c = std::cos(way - own), s = std::sin(way - own);
        const float m[12] = {c, -s, 0.0f, node[i][0] - (c * piece[0] - s * piece[1]), s, c, 0.0f, node[i][1] - (s * piece[0] + c * piece[1]), 0.0f, 0.0f, 1.0f, (node[i][2] + node[i + 1][2]) * 0.5f - b.pieceZ};
        std::memcpy(out[bone], m, sizeof(m));
    }
}

// How near the belt hanging out of its box a point is.
float NearBelt(const BeltGun& b, const float* at) {
    float nearest = 1e9f;
    for (int i = 0; i < 5; ++i) {
        const float a[3] = {b.pieces[i][0], b.pieces[i][1], b.pieceZ}, c[3] = {b.pieces[i][2], b.pieces[i][3], b.pieceZ};
        float pa[3], pc[3];
        Transform(b.hanging[i], a, pa);
        Transform(b.hanging[i], c, pc);
        nearest = std::min({nearest, Apart(at, pa), Apart(at, pc)});
    }
    return nearest;
}

// One frame of it, drawn with the glove on it; home at the end.
void Fly(const Gun& gun, const Frame& f, const float* hold, float dt, Result& r) {
    g_fly += dt / std::max(g_flyTime.load(std::memory_order_relaxed), 0.001f);
    float pose[12], inverse[12];
    FlyPose(gun, std::min(g_fly, 1.0f), pose);
    InvertRigid(hold, inverse);
    Compose(pose, inverse, g_gloveInGun);
    g_gloveBlend = 1.0f;
    Compose(f.gunPose, g_gloveInGun, r.glove);
    r.gloveBlend = 1.0f;
    if (g_fly >= 1.0f) {
        g_state = State::kNone;
        r.seated = true;
        return;
    }
    Compose(f.gunPose, pose, r.magPose);
    r.mag = true;
    r.holding = true;
}

// The seated magazine's back edge at this height on the gun: the paddle above it.
float BackAt(const Gun& gun, float y) {
    if (y >= gun.paddle[1])
        return gun.paddle[2];
    const auto& b = gun.back;
    if (y >= b[1][0]) {
        const float span = b[0][0] - b[1][0];
        const float t = span > 1e-4f ? std::max((b[0][0] - y) / span, 0.0f) : 0.0f;
        return b[0][1] + (b[1][1] - b[0][1]) * t;
    }
    const float span = b[1][0] - b[2][0];
    const float t = span > 1e-4f ? std::min((b[1][0] - y) / span, 1.0f) : 1.0f;
    return b[1][1] + (b[2][1] - b[1][1]) * t;
}

// The fresh one's corner, at this place, where the old one's width and height are.
bool AcrossOld(const Gun& gun, const float* at) {
    return std::fabs(at[0]) < kContactAcross && at[1] < gun.back[0][0] + kContactAbove && at[1] > gun.back[2][0] - kContactBelow;
}

// The hand's middle on the gun from where the hand takes the seated magazine: from grab, or for a wide box (Gun::wide) from the nearest point of its line.
void FromGrab(const Gun& gun, const float* onGun, float* from) {
    const float x = gun.wide ? std::clamp(onGun[0], std::min(gun.grab[0], gun.grabTo), std::max(gun.grab[0], gun.grabTo)) : gun.grab[0];
    from[0] = onGun[0] - x, from[1] = onGun[1] - gun.grab[1], from[2] = onGun[2] - gun.grab[2];
}

// How far the hand is from the front grip's place on the gun (frontGrip, where the game's own hand has its middle): its middle's distance, or its palm's
// (palmOnGun: the front grip's own take's measure, Frame::palm) where that's nearer. No palm: its middle's, as before.
float FrontApart(const float* onGun, const float* palmOnGun, const float* frontGrip) {
    const float middle = Apart(onGun, frontGrip);
    return palmOnGun ? std::min(middle, Apart(palmOnGun, frontGrip)) : middle;
}

// The lever against the lid, for a squeeze near both. By the hand's middle from the knob (toLever), as ever; but where that side's place on the lid is on
// the lever's side of the gun, over its handle (the right's on the M240 and the M249: 9 and 5 cm above the knob), each as the hand holds it: the point its
// fist closes round from the knob (the lid by where its fitted hand has its middle, NearLid), and only a lever this squeeze would take (leverTakes; else
// none). A right fist on the M240's handle over the top had had its middle nearer the lid's edge than the knob: the cover went up instead, a fifth of
// such squeezes (2026-11-25, probing the review of fc010e2). The left's place on the lid is across the gun from the handle: as before, bit for bit.
float LeverAgainstLid(const Gun& gun, const float* gunInverse, const float* glove, int side, float toLever, bool leverTakes) {
    if (!gun.belt || (side == 1 ? -gun.belt->lidGrab[0] : gun.belt->lidGrab[0]) * gun.lever[0] <= 0.0f)
        return toLever;
    if (!leverTakes)
        return 1e9f;
    float at[3], onGun[3];
    Transform(glove, kFistGrip[side == 1 ? 1 : 0], at);
    Transform(gunInverse, at, onGun);
    return Apart(onGun, gun.lever);
}

// The hand on the seated magazine, and not on its way to the gun.
bool OnMagazine(const Gun& gun, const float* from, float palmUp) {
    if (Length(from) > kGrabReach || from[2] > kGrabForward)
        return false;
    return from[1] < -kGrabLow || palmUp <= kPalmUp;
}

void NoteGrip(const Frame& f, float dt) {
    std::memcpy(g_gripAt[g_gripNext], f.gripRoom, sizeof(g_gripAt[0]));
    g_gripDt[g_gripNext] = dt;
    g_gripNext = (g_gripNext + 1) % 4, g_gripCount = std::min(g_gripCount + 1, 4);
}

void LetGoSpeed(Result& r) {
    if (g_gripCount != 4)
        return;
    const float* now = g_gripAt[(g_gripNext + 3) % 4];
    const float* then = g_gripAt[g_gripNext];
    const float span = g_gripDt[(g_gripNext + 1) % 4] + g_gripDt[(g_gripNext + 2) % 4] + g_gripDt[(g_gripNext + 3) % 4];
    if (span <= 0.0f)
        return;
    const float v[3] = {(now[0] - then[0]) / span, (now[1] - then[1]) / span, (now[2] - then[2]) / span};
    const float speed = Length(v), k = speed > kMaxDropSpeed ? kMaxDropSpeed / speed : 1.0f;
    for (int i = 0; i < 3; ++i)
        r.dropVelocity[i] = v[i] * k;
}

// A handle's knob as it rests in its notch, on the frame: turned up about the gun's z, back along it.
void NotchKnob(const Gun& gun, float* knob) {
    const float c = std::cos(gun.leverNotch), s = std::sin(gun.leverNotch), dx = gun.lever[0] - gun.leverNotchAxis[0], dy = gun.lever[1] - gun.leverNotchAxis[1];
    knob[0] = gun.leverNotchAxis[0] + c * dx - s * dy;
    knob[1] = gun.leverNotchAxis[1] + s * dx + c * dy;
    knob[2] = gun.lever[2] - gun.leverNotchBack;
}

void ForgetFront() {
    g_frontCount = 0;
    g_haveFrontThen = false;
    g_walled = false;
}

// The belt's pinch, the point the hand holds it by, in that side's glove's bind space (the right's: x negated).
void PinchIn(const BeltGun& b, int side, float* out) {
    out[0] = side == 1 ? -b.pinch[0] : b.pinch[0];
    out[1] = b.pinch[1];
    out[2] = b.pinch[2];
}

// The lid's hold for that side: the left's fitted table; the right's, it mirrored about the gun's middle (the lid is the receiver's own, across it).
void LidHoldFor(const BeltGun& b, int side, float* out) {
    if (side == 1)
        MirrorHold(b.lidHold, 0.0f, out);
    else
        std::memcpy(out, b.lidHold, 12 * sizeof(float));
}

}

void MirrorHold(const float* hold, float middle, float* out) {
    // hold = [R | t]: (S hold Mc)(p) = S (R (2 middle - x, y, z) + t) = S R S p + S (2 middle R's first column + t).
    float m[12];
    std::memcpy(m, hold, sizeof(m));
    const float u[3] = {2.0f * middle * hold[0] + hold[3], 2.0f * middle * hold[4] + hold[7], 2.0f * middle * hold[8] + hold[11]};
    m[1] = -hold[1], m[2] = -hold[2], m[4] = -hold[4], m[8] = -hold[8];
    m[3] = -u[0], m[7] = u[1], m[11] = u[2];
    std::memcpy(out, m, sizeof(m));
}

void FistPoint(int side, float* out) {
    std::memcpy(out, kFistGrip[side == 1 ? 1 : 0], 3 * sizeof(float));
}

float PartMiddle(const Gun& gun) {
    return gun.wide ? (gun.grab[0] + gun.grabTo) * 0.5f : gun.grab[0];
}

bool OffMiddle(float middle) {
    return !(std::fabs(middle) <= kOnMiddle);  // (not a number: off it, the hold as the hand takes it)
}

const float* LeverHoldFor(const Gun& gun, int side, bool under, float* mirrored, const float (**fingers)[12]) {
    if (!gun.leverHold) {  // (no fitted hand on it at all: a fist with the real wrist, whichever way)
        *fingers = nullptr;
        return nullptr;
    }
    const bool fromBelow = under && gun.leverUnderHold;
    const float* table = fromBelow ? gun.leverUnderHold : gun.leverHold;
    *fingers = fromBelow ? gun.leverUnderFingers : gun.leverFingers;
    if (side != 1)
        return table;
    if (OffMiddle(gun.lever[0])) {  // (the right on a lever off the gun's middle: a fist with its real wrist)
        *fingers = nullptr;
        return nullptr;
    }
    MirrorHold(table, 0.0f, mirrored);
    return mirrored;
}

void HoldFor(const Gun& gun, float* hold, int side) {
    float left[12];
    if (gun.hold) {
        std::memcpy(left, gun.hold, sizeof(left));
    } else {
        float m416[12];
        pouch::GetHold(m416);
        const float back[12] = {1, 0, 0, -gun.handOffset[0], 0, 1, 0, -gun.handOffset[1], 0, 0, 1, -gun.handOffset[2]};
        Compose(m416, back, left);
    }
    if (side == 1)
        MirrorHold(left, PartMiddle(gun), hold);
    else
        std::memcpy(hold, left, sizeof(left));
}

void GloveFor(const Gun& gun, const float* magPose, float* glove, int side) {
    float hold[12], inverse[12];
    HoldFor(gun, hold, side);
    InvertRigid(hold, inverse);
    Compose(magPose, inverse, glove);
}

void SetKnockSpeed(float metresPerSecond) {
    g_knockSpeed = metresPerSecond;
}

void SetMagnetReach(float metres) {
    g_magnetReach = metres;
}

void SetFlyTime(float seconds) {
    g_flyTime = seconds;
}

Result Update(const Frame& f) {
    Result r;
    const float dt = std::clamp(f.dt, 0.0f, 0.1f);
    // Whose hand (free hands: the hand at work). Fed the other one: what the last one had of the gun's is let go of as when the gun leaves the hands (below,
    // its events that hand's, at the belly if that hand was there when last fed), and nothing of that hand carries over: the new one's grip and trigger
    // count once they've been under kArm, no glove eases back from where the last one's was drawn, its speed and the knock's measured afresh.
    const int side = f.side == 1 ? 1 : 0;
    r.side = side;
    bool handOver = false;  // the last hand had something: let go of this frame, as dropHeld
    if (side != g_fedSide) {
        handOver = g_state != State::kNone;
        if (handOver)
            r.side = g_fedSide;
        else
            g_gloveBlend = 0.0f;
        g_armed = f.squeeze < kArm;
        g_triggerArmed = f.trigger < kArm;
        g_gripCount = 0;
        g_havePalmWas = false;
        ForgetFront();
        g_fedSide = side;
    }
    const bool atShoulder = handOver ? g_atShoulderWas : f.atShoulder;  // (a let-go at the hand-over: where the last hand was)
    g_atShoulderWas = f.atShoulder;
    // Either the grip or the trigger takes hold, and either keeps it (the user, 2026-11-05: "I'm starting to get confused whether it's grip or trigger. So let's just embed both").
    const float grasp = std::max(f.squeeze, f.trigger);
    const bool crossedGrip = f.left && g_armed && f.squeeze > kTake;
    if (f.squeeze < kArm)
        g_armed = true;
    const bool crossedTrigger = f.left && g_triggerArmed && f.trigger > kTake;
    if (f.trigger < kArm)
        g_triggerArmed = true;
    if (crossedTrigger)
        g_triggerArmed = false;
    const bool crossed = crossedGrip || crossedTrigger;
    // The lever springs home on its own.
    if (g_leverSpring >= 0.0f) {
        g_leverSpring += dt;
        const float s = g_leverSpring / kLeverSpring;
        if (s < 1.0f)
            r.leverBack = g_leverGoing * (1.0f - s);
        else
            g_leverSpring = -1.0f;
    }
    // The glove eases back to the hand after a seat, a let go, the lever.
    if (g_state != State::kFly && g_state != State::kOnGun && g_state != State::kLever && g_state != State::kLid && g_state != State::kBelt && g_gloveBlend > 0.0f) {
        g_gloveBlend = std::max(g_gloveBlend - 0.35f, 0.0f);
        if (f.gun && f.gunTracked) {
            Compose(f.gunPose, g_gloveInGun, r.glove);
            r.gloveBlend = g_gloveBlend;
        }
    }
    const bool wentIn = g_wasEmpty && !f.wellEmpty;
    g_wasEmpty = f.wellEmpty;
    if (g_state != State::kHand)  // (a box eases into the hand only straight after it came off its way, in the hand that took it off)
        g_wayEasing = false;
    g_gunFed = f.gun;
    std::memcpy(g_frontGrip, f.frontGrip, sizeof(g_frontGrip));
    g_frontByPalm = f.havePalm && Finite(f.palm, 3);
    g_sinceFlick = std::min(g_sinceFlick + dt, 10.0f);
    g_give *= std::exp(-dt / kGiveFade);
    // Another gun: nothing the hand was doing carries over to it (a held magazine from one with no button into one with).
    static const Gun* s_gunWas = nullptr;
    if (f.gun != s_gunWas && g_state != State::kNone) {
        Clear();
        r.cancelled = true;
    }
    if (f.gun != s_gunWas)  // (each gun's handle as the game has it: home)
        g_notched = false, g_notch = 0.0f, g_havePalmWas = false;
    s_gunWas = f.gun;
    if (!f.gun) {
        if (g_state != State::kNone) {
            Clear();
            r.cancelled = true;
        }
        if (crossedGrip)
            g_armed = false;
        return r;
    }
    const Gun& gun = *f.gun;
    if (g_state != State::kLever)  // a handle that's swung out goes in as it springs home
        g_swing = std::max(g_swing - dt / kLeverSpring, 0.0f);
    r.leverSwing = gun.leverSwing * g_swing;
    // Leaving the magazine, the lever or the lid, the glove eases off from where it's been drawn, not from the real hand for a frame.
    const auto easeFromHere = [&] {
        if (f.gunTracked) {
            Compose(f.gunPose, g_gloveInGun, r.glove);
            r.gloveBlend = g_gloveBlend;
        }
    };
    // A belt fed gun: the lid goes the rest of the way by itself; the belt, let go of, back to hanging from its box.
    if (gun.belt) {
        if (g_state != State::kLid && g_lid != g_lidAim) {
            const float step = dt / kLidSwing;
            g_lid = g_lid < g_lidAim ? std::min(g_lid + step, g_lidAim) : std::max(g_lid - step, g_lidAim);
            if (g_lid == g_lidAim)
                (g_lid > 0.0f ? r.lidUp : r.lidShut) = true;
        }
        r.lid = gun.belt->lidOpen * g_lid;
        r.beltReady = g_beltLaid && g_lid <= 0.0f && !f.wellEmpty;
        if (!g_beltLaid && !f.wellEmpty) {  // (in the hand: over this below)
            std::memcpy(r.beltBones, gun.belt->hanging, sizeof(r.beltBones));
            r.beltDrawn = true;
        }
    }
    if (g_give > 0.002f && !f.wellEmpty) {
        Rock(gun, g_give, r.oldRock);
        r.oldRocked = true;
    }
    float gunInverse[12], palm[3], onGun[3];
    InvertRigid(f.gunPose, gunInverse);
    Transform(f.glove, kHandInGlove[side], palm);
    Transform(gunInverse, palm, onGun);
    float hold[12];
    HoldFor(gun, &hold[0], side);
    // Where this side's fitted hold has the hand on the seated magazine or box (its grab: the right's mirrored about the part's own middle).
    const float ownGrab[3] = {side == 1 ? 2.0f * PartMiddle(gun) - gun.grab[0] : gun.grab[0], gun.grab[1], gun.grab[2]};
    if (g_state == State::kNone)  // (a wide box held where the hand took it: from a take of it on the gun, below, until it's let go of or home)
        g_takeOwn = false;
    else if (g_takeOwn)
        std::memcpy(hold, g_takeHold, sizeof(hold));
    // A handle in its notch: it stays back, turned up, until it's knocked down. The hand coming down on it (its middle near the handle as it lies in the notch, moving down on the gun), or a
    // fresh squeeze or trigger there, knocks it out: it turns down and springs home.
    if (gun.leverNotch > 0.0f) {
        if (g_notched && g_state == State::kNone && !f.dropHeld) {
            float knob[3];
            NotchKnob(gun, knob);
            const bool here = f.left && f.gunTracked && f.handFree && Apart(onGun, knob) < kKnockReach;
            if (!f.left || !f.gunTracked || Apart(onGun, knob) > kKnockClear)
                g_knockArmed = true;
            const float down = g_havePalmWas && dt > 0.0f && dt < 0.1f ? (g_palmWas[1] - onGun[1]) / dt : 0.0f;
            const bool slapped = here && g_knockArmed && onGun[1] > knob[1] - kKnockUnder && down > kKnockDown && down < kKnockMost;
            if (slapped || (here && crossed)) {
                g_notched = false;
                g_leverSpring = 0.0f;
                g_leverGoing = gun.leverNotchBack;
                r.knocked = true;
                r.charged = true;
                r.leverBack = gun.leverNotchBack;
                if (crossed)
                    r.spentSqueeze = true;
                if (crossedGrip)
                    g_armed = false;
                g_palmWas[0] = onGun[0], g_palmWas[1] = onGun[1], g_palmWas[2] = onGun[2];
                g_havePalmWas = f.left && f.gunTracked;
                g_notch = std::max(g_notch - dt / kNotchTurn, 0.0f);
                r.leverNotch = gun.leverNotch * g_notch;
                return r;
            }
        }
        g_notch = g_notched ? std::min(g_notch + dt / kNotchTurn, 1.0f) : std::max(g_notch - dt / kNotchTurn, 0.0f);
        r.leverNotch = gun.leverNotch * g_notch;
        r.leverHeld = g_notched;
        if (g_notched)
            r.leverBack = gun.leverNotchBack;
    }
    g_palmWas[0] = onGun[0], g_palmWas[1] = onGun[1], g_palmWas[2] = onGun[2];
    g_havePalmWas = f.left && f.gunTracked;

    if (g_state != State::kNone && wentIn && g_state != State::kLever) {
        Clear();
        r.cancelled = true;
        if (crossedGrip)
            g_armed = false;
        return r;
    }
    // Free hands: the gun has left the hands (loose has it, or it flies into a hand from the ground). A magazine in the hand is let go of as when the hand
    // opens (it falls at the hand's speed, or goes back at the shoulder), never cancelled; one going home by itself falls from where it's drawn, with no speed
    // of its own (the hand wasn't carrying it). The seated one stays in, the lever springs home, the lid goes back where it was and the belt hangs from its
    // box again, all on the gun. Nothing is taken from the gun or put into it meanwhile. The glove eases off what it was on only with the gun seen this
    // frame; otherwise it's straight back at the hand.
    // The same for what the last hand had as the other is fed (handOver: its events, glove and speed that hand's; this frame nothing more), and the glove
    // it leaves isn't eased back from on the new hand.
    if (f.dropHeld || handOver) {
        if (crossedGrip)
            g_armed = false;
        const auto offTheGun = [&] {
            if (f.gunTracked)
                easeFromHere();
            else
                g_gloveBlend = 0.0f;
        };
        switch (g_state) {
        case State::kHand:
            if (f.left && !handOver)
                NoteGrip(f, dt);
            g_walled = false;
            r.putBack = atShoulder;
            r.letGo = !atShoulder;
            if (r.letGo)
                LetGoSpeed(r);
            break;
        case State::kFly:
            offTheGun();
            r.putBack = atShoulder;
            r.letGo = !atShoulder;
            break;
        case State::kOnGun:
            offTheGun();
            break;
        case State::kLever:
            g_leverSpring = 0.0f;
            g_leverGoing = g_leverPull;
            r.leverLet = true;
            r.leverPulled = g_leverMost;
            r.charged = g_leverMost >= gun.leverTravel * kLeverToCharge;
            r.leverBack = g_leverPull;
            g_leverPull = 0.0f;
            offTheGun();
            break;
        case State::kLid: {
            // (As when let go of: short of where it goes on by itself, back where it was.)
            const bool snapped = g_lidWasUp ? g_lid < kLidSnapShut : g_lid > kLidSnapUp;
            g_lidAim = snapped ? (g_lidWasUp ? 0.0f : 1.0f) : g_lidWasUp ? 1.0f : 0.0f;
            offTheGun();
            break;
        }
        case State::kBelt:
            g_beltFlying = false;
            r.beltLet = true;
            break;
        case State::kNone:
            break;
        }
        g_state = State::kNone;
        g_lostFor = 0.0f;
        if (handOver)
            g_gloveBlend = 0.0f;  // (eased off this frame on the last hand: none on the new one's)
        return r;
    }
    // On its way home: nothing the hand does now changes that.
    if (g_state == State::kFly) {
        if (crossedGrip)
            g_armed = false;
        if (f.gunTracked)
            Fly(gun, f, hold, dt, r);
        else
            r.holding = true;
        if (r.seated && gun.belt) {  // a new box: its belt hanging out of it, to be laid by hand
            g_beltLaid = false;
            std::memcpy(r.beltBones, gun.belt->hanging, sizeof(r.beltBones));
            r.beltDrawn = true;
        }
        return r;
    }
    if (g_state == State::kNone) {
        const float toLever = Apart(onGun, gun.lever);
        float from[3];
        FromGrab(gun, onGun, from);
        float gloveInGun[12], palmFaces[3];
        Compose(gunInverse, f.glove, gloveInGun);
        const float* palmIn = kPalmInGlove[side];
        for (int i = 0; i < 3; ++i)
            palmFaces[i] = gloveInGun[i * 4] * palmIn[0] + gloveInGun[i * 4 + 1] * palmIn[1] + gloveInGun[i * 4 + 2] * palmIn[2];
        // (How far the hand is from the front grip's place: by its middle, or by its palm as the front grip's own take has it, the nearer.)
        float palmOnGun[3] = {};
        if (g_frontByPalm)
            Transform(gunInverse, f.palm, palmOnGun);
        const float toFront = FrontApart(onGun, g_frontByPalm ? palmOnGun : nullptr, f.frontGrip);
        // The trigger isn't the front grip's: near enough is enough. The grip, only with the hand on the magazine or the lever and not on its way to the gun.
        const bool onMagazine = crossedGrip ? OnMagazine(gun, from, palmFaces[1]) : Length(from) <= kGrabReach && from[2] <= kGrabForward;
        const bool onLever = crossedGrip ? toLever < kLeverReachGrip && toLever < toFront : toLever < kLeverReach;
        // A wide box taken anywhere along it (Gun::wide): by its far end it's under the lever's knob (the M249's pouch's right end, 9.9 cm under its
        // handle's), and the nearer of the two is taken there, not the lever first; but only a hand on the box, below its top, its palm not up as a
        // hand going for the handle from under it has it (the review of 2026-11-18: the M249's own fitted hold from under its handle, 6.0 cm from the
        // knob and 5.7 from the pouch's line, 1.4 cm above the pouch, took the box with the cover up).
        const bool boxFirst = gun.wide && onMagazine && !f.wellEmpty && (!gun.belt || g_lid > 0.5f) && Length(from) < toLever && onGun[1] < gun.top[1] &&
                              palmFaces[1] <= kLeverPalmUnder;
        // A belt fed gun: the belt hanging out of its box; its lid; before all else.
        float pinched[3] = {};
        if (gun.belt) {
            float inHand[3], pinch[3];
            PinchIn(*gun.belt, side, pinch);
            Transform(f.glove, pinch, inHand);
            Transform(gunInverse, inHand, pinched);
        }
        if (gun.belt && f.left && f.gunTracked && f.handFree && crossed && !g_beltLaid && !f.wellEmpty && NearBelt(*gun.belt, pinched) < kBeltReach) {
            g_state = State::kBelt;
            g_beltShare = 0.0f;
            g_beltFlying = false;
            r.beltTaken = true;
            r.spentSqueeze = true;
        // (Whichever's the nearest of the lid, the lever (LeverAgainstLid: by the fist, where the lid's place for this hand is over the handle) and, for
        // the grip, the front grip.)
        } else if (gun.belt && f.left && f.gunTracked && f.handFree && crossed && NearLid(*gun.belt, onGun, side) < kLidReach &&
                   NearLid(*gun.belt, onGun, side) <= LeverAgainstLid(gun, gunInverse, f.glove, side, toLever, onLever && !boxFirst && !g_notched) &&
                   (!crossedGrip || NearLid(*gun.belt, onGun, side) < toFront)) {
            g_state = State::kLid;
            g_lidGrabAngle = LidAngle(*gun.belt, onGun) - gun.belt->lidOpen * g_lid;
            g_lidGrabSlide = LidSlide(*gun.belt, onGun);
            g_lidWasUp = g_lid > 0.5f;
            r.lidTaken = true;
            r.spentSqueeze = true;
        // The lever; the seated magazine; or one from over the shoulder.
        } else if (f.left && f.gunTracked && f.handFree && crossed && onLever && !boxFirst && !g_notched) {  // (one resting in its notch is knocked down: above)
            g_state = State::kLever;
            g_leverUnder = palmFaces[1] > kLeverPalmUnder;
            g_leverGrabZ = onGun[2];
            g_notchBack = false;
            g_leverPull = g_leverMost = 0.0f;
            g_leverSounded = false;
            g_leverSpring = -1.0f;
            r.leverTaken = true;
        } else if (f.left && f.gunTracked && f.handFree && crossed && !f.wellEmpty && onMagazine && (!gun.belt || g_lid > 0.5f)) {  // (a box only with the lid up: its belt's held under it)
            g_state = State::kOnGun;
            std::memcpy(g_grabAt, onGun, sizeof(g_grabAt));
            g_gripCount = 0;
            g_wayAt = 0.0f;
            g_wayMove[0] = g_wayMove[1] = g_wayMove[2] = 0.0f;
            // A wide box taken further from the game's own place on it than any magazine is taken from its own (kGrabReach: the review of 2026-11-18, the
            // glove drawn there from the real hand 20 cm away and more, the box swung across under the gun as it came free): held where the hand took it,
            // as the hand is, the glove carried along with it, not at the game's own place. (The right's own place: the left's mirrored about the box's
            // middle. A box hung off the gun's middle, the M240's, taken by the right: always as the hand took it, the left's fit mirrored about it would
            // have the glove between the box and the gun.)
            const float fromOwn[3] = {onGun[0] - ownGrab[0], onGun[1] - ownGrab[1], onGun[2] - ownGrab[2]};
            g_takeOwn = (gun.wide && Length(fromOwn) > kGrabReach) || (side == 1 && OffMiddle(PartMiddle(gun)));
            if (g_takeOwn) {
                InvertRigid(gloveInGun, g_takeHold);
                std::memcpy(hold, g_takeHold, sizeof(hold));
            }
            r.grabbed = true;
            r.spentSqueeze = true;
        } else if (f.shoulder) {
            r.note = !f.handFree ? "the hand is busy" : !f.gunTracked ? "the gun not tracked this frame" : !f.spare ? "no rounds left for it" : "";
            if (!*r.note) {
                g_state = State::kHand;
                g_heldFrames = 0;
                g_lostFor = 0.0f;
                g_justOut = false;
                ForgetFront();
                g_gripCount = 0;
                r.took = true;
                r.spentSqueeze = true;
            }
        } else if (crossed && f.handFree && !f.wellEmpty && Length(from) < kGrabReach + 0.05f) {
            r.leftForGrip = true;
            std::memcpy(r.squeezeFrom, from, sizeof(from));
            r.palmUp = palmFaces[1];
        }
    }
    if (crossedGrip)
        g_armed = false;
    if (g_state == State::kNone)
        return r;

    // The hand or the gun lost: kept a second.
    if (!f.left || !f.gunTracked) {
        g_lostFor += dt;
        g_gripCount = 0;
        ForgetFront();  // or the gap reads as a fast hand, and knocks the old one out
        if (g_lostFor > kLost || grasp < kArm) {
            Clear();
            r.cancelled = true;
            return r;
        }
        r.holding = g_state != State::kLever;
        if (g_state == State::kLever) {
            r.leverBack = g_leverPull;
            r.fist = true;
            if (side == 1) {  // (the right's own: the left's mirrored on a handle on the middle, none for a fist)
                float mirrored[12];
                LeverHoldFor(gun, 1, g_leverUnder, mirrored, &r.leverFingers);
            } else {
                r.leverFingers = g_leverUnder && gun.leverUnderHold ? gun.leverUnderFingers : gun.leverFingers;
            }
        }
        return r;
    }
    g_lostFor = 0.0f;
    NoteGrip(f, dt);

    if (g_state == State::kLid) {
        const BeltGun& b = *gun.belt;
        // (Round the hinge the shortest way: atan2's seam isn't a jump of the lid.)
        const float angle = std::clamp(std::remainder(LidAngle(b, onGun) - g_lidGrabAngle, 2.0f * kPi), 0.0f, b.lidOpen);
        float turn[12];
        g_lid = angle / b.lidOpen;
        r.lid = angle;
        r.beltReady = false;
        // Let go of, the real hand gone off it, or past the point of no return: it goes the rest of the way by itself, snappily, whichever way it was going.
        const bool snapped = g_lidWasUp ? g_lid < kLidSnapShut : g_lid > kLidSnapUp;
        if (grasp < kArm || NearLid(b, onGun, side) >= kLeverLost || snapped) {
            g_state = State::kNone;
            g_lidAim = snapped ? (g_lidWasUp ? 0.0f : 1.0f) : g_lidWasUp ? 1.0f : 0.0f;
            easeFromHere();
            return r;
        }
        LidTurn(b, angle, turn);
        float inverse[12], slid[12], lidHold[12];
        LidHoldFor(b, side, lidHold);
        InvertRigid(lidHold, inverse);
        const float along[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, g_lidGrabSlide};
        Compose(turn, along, slid);
        Compose(slid, inverse, g_gloveInGun);
        g_gloveBlend = std::min(g_gloveBlend + 0.35f, 1.0f);
        Compose(f.gunPose, g_gloveInGun, r.glove);
        r.gloveBlend = g_gloveBlend;
        r.leverFingers = b.lidFingers;
        r.fist = true;
        return r;
    }

    if (g_state == State::kBelt) {
        const BeltGun& b = *gun.belt;
        float inHand[3], pinched[3], pinch[3];
        PinchIn(b, side, pinch);
        Transform(f.glove, pinch, inHand);
        Transform(gunInverse, inHand, pinched);
        if (f.wellEmpty || (!g_beltFlying && (grasp < kArm || Apart(pinched, b.anchor) > kBeltLost))) {
            g_state = State::kNone;
            r.beltLet = true;
            return r;
        }
        r.beltDrawn = true;
        if (!g_beltFlying) {
            // In the hand, where the hand has it; near the feed with the lid up, the feed takes it.
            g_beltShare = std::min(g_beltShare + kBeltBlendIn, 1.0f);
            float chain[5][12];
            BeltInHand(b, pinched, chain);
            for (int i = 0; i < 5; ++i)
                for (int j = 0; j < 12; ++j)
                    r.beltBones[i][j] = b.hanging[i][j] + (chain[i][j] - b.hanging[i][j]) * g_beltShare;
            if (g_lid > 0.5f && Apart(pinched, b.feed) < kBeltMagnet) {
                g_beltFlying = true;
                g_beltFlown = 0.0f;
                std::memcpy(g_beltFrom, r.beltBones, sizeof(g_beltFrom));
            }
        } else {
            g_beltFlown += dt / kBeltFly;
            if (g_beltFlown >= 1.0f) {
                g_state = State::kNone;
                g_beltLaid = true;
                g_beltFlying = false;
                r.beltLaid = true;
                r.beltDrawn = false;
                r.beltReady = g_lid <= 0.0f;
                return r;
            }
            const float e = g_beltFlown * g_beltFlown * (3.0f - 2.0f * g_beltFlown);
            for (int i = 0; i < 5; ++i)
                for (int j = 0; j < 12; ++j)
                    r.beltBones[i][j] = g_beltFrom[i][j] + (b.laid[i][j] - g_beltFrom[i][j]) * e;
        }
        r.leverFingers = b.pinchFingers;
        r.holding = true;
        return r;
    }

    if (g_state == State::kLever) {
        // Let go of; or the real hand gone off it, as the front grip's reach: the glove never stays glued with the arm stretched.
        const float knobNow[3] = {gun.lever[0], gun.lever[1], gun.lever[2] - g_leverPull};
        if (grasp < kArm || Apart(onGun, knobNow) >= kLeverLost) {
            g_state = State::kNone;
            g_leverSpring = 0.0f;
            g_leverGoing = g_leverPull;
            r.leverLet = true;
            r.leverPulled = g_leverMost;
            r.charged = g_leverMost >= gun.leverTravel * kLeverToCharge;
            r.leverBack = g_leverPull;
            g_leverPull = 0.0f;
            easeFromHere();
            return r;
        }
        g_leverPull = std::clamp(g_leverGrabZ - onGun[2], 0.0f, gun.leverTravel);
        g_leverMost = std::max(g_leverMost, g_leverPull);
        // Drawn back and lifted: a handle with a notch goes up into it and out of the hand; it rests there, the bolt held open.
        if (gun.leverNotch > 0.0f && g_leverPull >= gun.leverTravel * kNotchAt) {
            if (!g_notchBack) {
                g_notchBack = true;
                g_notchFromY = onGun[1], g_notchBackZ = onGun[2];
            } else if (onGun[2] < g_notchBackZ) {
                g_notchFromY = std::clamp(onGun[1], g_notchFromY, g_notchFromY + (g_notchBackZ - onGun[2]) * kNotchSlope);
                g_notchBackZ = onGun[2];
            }
        } else {
            g_notchBack = false;
        }
        if (g_notchBack && onGun[1] - g_notchFromY >= kNotchLift) {
            g_state = State::kNone;
            g_notched = true;
            g_knockArmed = false;
            g_leverPull = 0.0f;
            g_leverSpring = -1.0f;
            r.notched = true;
            r.leverHeld = true;
            r.leverBack = gun.leverNotchBack;
            r.leverNotch = gun.leverNotch * g_notch;
            easeFromHere();
            return r;
        }
        if (g_leverPull > kLeverHeard && !g_leverSounded) {
            g_leverSounded = true;
            r.leverMoving = true;
        }
        r.leverBack = g_leverPull;
        r.fist = true;
        const bool fromBelow = g_leverUnder && gun.leverUnderHold;
        if (!fromBelow)
            g_swing = std::min(g_swing + dt / kLeverSwingOut, 1.0f);
        r.leverSwing = gun.leverSwing * g_swing;
        g_gloveBlend = std::min(g_gloveBlend + 0.35f, 1.0f);
        // (The lever's place drawn back and swung out with it, a handle that swings.)
        const float c = std::cos(r.leverSwing), s = std::sin(r.leverSwing), px = gun.leverPivot[0], pz = gun.leverPivot[1];
        const float back[12] = {c, 0, s, px - (c * px + s * pz), 0, 1, 0, 0, -s, 0, c, pz - (-s * px + c * pz) - g_leverPull};
        float mirroredLever[12];
        const float (*leverFingers)[12] = nullptr;
        if (const float* table = LeverHoldFor(gun, side, g_leverUnder, mirroredLever, &leverFingers)) {
            // Exactly where the hand's been fitted on it, going back with it (and swung out with it, a handle that swings). (The right's: the left's fit
            // mirrored, a handle on the gun's middle.)
            float inverse[12];
            InvertRigid(table, inverse);
            Compose(back, inverse, g_gloveInGun);
            Compose(f.gunPose, g_gloveInGun, r.glove);
            r.gloveBlend = g_gloveBlend;
            r.leverFingers = leverFingers;
            return r;
        }
        // The fist on the knob, the wrist as the real hand has it: over the top or from below, whichever way it came in. (The right's on a lever off the
        // gun's middle: its knob where the handle has gone, swung out with it too.)
        float glove[12], at[3], closesOn[3];
        float knob[3] = {gun.lever[0], gun.lever[1], gun.lever[2] - g_leverPull};
        if (side == 1)
            Transform(back, gun.lever, knob);
        Transform(f.gunPose, knob, at);
        Transform(f.glove, kFistGrip[side], closesOn);
        std::memcpy(glove, f.glove, sizeof(glove));
        for (int i = 0; i < 3; ++i)
            glove[i * 4 + 3] += at[i] - closesOn[i];
        Compose(gunInverse, glove, g_gloveInGun);
        std::memcpy(r.glove, glove, sizeof(glove));
        r.gloveBlend = g_gloveBlend;
        return r;
    }

    if (g_state == State::kOnGun) {
        if (grasp < kArm || f.wellEmpty) {  // let go of before it came free: it stays in; or the button dropped it from under the hand
            g_state = State::kNone;
            easeFromHere();
            return r;
        }
        const float d[3] = {onGun[0] - g_grabAt[0], onGun[1] - g_grabAt[1], onGun[2] - g_grabAt[2]};
        if (gun.way) {
            // A box with a way: the pull followed along its own reload's way off from where the box is, the box drawn there, the glove on it, free once
            // it's come the way's free length; the hand going up or along to the gun first leaves it in, as below; and the hand gone kWayOff off the way,
            // whatever the pull (straight down, or steeply down: the way is to the gun's left), leaves it in, the glove easing back to the hand.
            float off = 0.0f, rock[12], inverse[12];
            const float moved[3] = {d[0] - g_wayMove[0], d[1] - g_wayMove[1], d[2] - g_wayMove[2]};
            std::memcpy(g_wayMove, d, sizeof(g_wayMove));
            float along = WayAlong(gun, gun.grab, d, g_wayAt, Length(moved) + kWaySlack, off);
            if (g_takeOwn) {
                // A wide box taken far from the game's own place on it: the way where the hand has it goes elsewhere as the box turns (the PKP's 29 cm box
                // tips 19.5 degrees, its bail end going 10 cm down as its left end slides 8 cm), so the nearer of the two is followed: drawn as the box
                // turns, the glove stays on the hand; drawn as the game's own hand draws it, it still comes off along its way. (Either within reach of
                // where it was: never a leap.)
                float heldOff = 0.0f;
                const float heldAlong = WayAlong(gun, g_grabAt, d, g_wayAt, Length(moved) + kWaySlack, heldOff);
                if (heldOff < off)
                    along = heldAlong, off = heldOff;
            }
            g_wayAt = along;
            const float pull = std::max(g_wayAt - kPullDead, 0.0f);
            if (pull < kToGripPull && (d[1] > kToGripUp || d[2] > kToGripForward || Length(d) > kToGripAway)) {
                g_state = State::kNone;
                r.toGrip = true;
                easeFromHere();
                return r;
            }
            if (off > kWayOff) {
                g_state = State::kNone;
                easeFromHere();
                return r;
            }
            WayAt(gun.way->off, gun.way->offCount, std::min(pull, gun.way->free), rock);
            InvertRigid(hold, inverse);
            Compose(rock, inverse, g_gloveInGun);
            if (pull < gun.way->free) {
                Compose(f.gunPose, rock, r.magPose);
                r.mag = true;
                r.hideSeated = true;
                r.holding = true;
                g_gloveBlend = std::min(g_gloveBlend + 0.35f, 1.0f);
                Compose(f.gunPose, g_gloveInGun, r.glove);
                r.gloveBlend = g_gloveBlend;
                return r;
            }
            // Free: it eases from there into the hand, with the glove (below).
            std::memcpy(g_wayFrom, rock, sizeof(g_wayFrom));
            g_wayEasing = true;
        } else {
            const float pull = std::max(-d[1] - kPullDead, 0.0f);
            // The hand went on up or along to the gun, or right away: it stays in, and the squeeze is the front grip's.
            if (pull < kToGripPull && (d[1] > kToGripUp || d[2] > kToGripForward || Length(d) > kToGripAway)) {
                g_state = State::kNone;
                r.toGrip = true;
                easeFromHere();
                return r;
            }
            // Drawn a little and the hand gone off somewhere else: it stays in, the squeeze spent; the glove isn't left glued to it with the arm stretched.
            const bool slides = gun.button || gun.straight;  // straight down the well until its top is out of it; else rocked off its hook
            if (Length(d) > kToGripAway && pull < (slides ? -gun.mouth : kPullOut)) {
                g_state = State::kNone;
                easeFromHere();
                return r;
            }
            // Rocked off its hook, it's free once it's rocked; on a gun with a button it slides straight down the well until its top is out of it.
            if (pull < (slides ? -gun.mouth : kPullOut)) {
                float rock[12], inverse[12];
                if (slides) {
                    const float down[12] = {1, 0, 0, 0, 0, 1, 0, -pull, 0, 0, 1, 0};
                    std::memcpy(rock, down, sizeof(down));
                } else {
                    Rock(gun, kRockOut * pull / kPullOut, rock);
                }
                Compose(f.gunPose, rock, r.magPose);
                r.mag = true;
                r.hideSeated = true;
                r.holding = true;
                InvertRigid(hold, inverse);
                Compose(rock, inverse, g_gloveInGun);
                g_gloveBlend = std::min(g_gloveBlend + 0.35f, 1.0f);
                Compose(f.gunPose, g_gloveInGun, r.glove);
                r.gloveBlend = g_gloveBlend;
                return r;
            }
        }
        g_state = State::kHand;
        g_heldFrames = kHeldFrames;
        g_justOut = true;
        g_outFar = 0.0f;
        ForgetFront();
        g_gripCount = 0;
        r.pulledOut = true;
        if (gun.belt)
            g_beltLaid = false;  // its belt's gone with it
        easeFromHere();
    }

    ++g_heldFrames;
    float held[12], inGun[12], top[3], front[3];
    Compose(f.glove, hold, held);
    Compose(gunInverse, held, inGun);
    Transform(inGun, gun.top, top);
    Transform(inGun, gun.hook, front);
    const float identity[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    const float turned = TurnBetween(inGun, identity);

    // Its front corner's speed along the gun, over the last few frames. A hitch's frame is clamped: not a speed.
    if (dt >= 0.1f)
        g_frontCount = 0;
    std::memcpy(g_frontAt[g_frontNext], front, sizeof(g_frontAt[0]));
    g_frontDt[g_frontNext] = dt;
    g_frontNext = (g_frontNext + 1) % kSpeedFrames, g_frontCount = std::min(g_frontCount + 1, kSpeedFrames);
    float forward = 0.0f;
    if (g_frontCount == kSpeedFrames) {
        const float* then = g_frontAt[g_frontNext];
        float span = 0.0f;
        for (int k = 1; k < kSpeedFrames; ++k)
            span += g_frontDt[(g_frontNext + k) % kSpeedFrames];
        if (span > 0.0f)
            forward = (front[2] - then[2]) / span;
    }

    // Against the old one: it has to touch, coming from behind it; it takes force to knock it out; gently, it stops there.
    float past = 0.0f;
    if (!gun.button && !gun.noKnock && !f.wellEmpty && !r.pulledOut && g_heldFrames > kHeldFrames) {
        const float knock = g_knockSpeed.load(std::memory_order_relaxed);
        const bool across = AcrossOld(gun, front);
        const float over = front[2] - BackAt(gun, front[1]);
        // It touches where its way crosses the old one's back edge between two frames, however fast and from whatever height it came.
        bool crossedEdge = false;
        if (g_haveFrontThen) {
            const float overThen = g_frontThen[2] - BackAt(gun, g_frontThen[1]);
            if (overThen <= 0.0f && over > 0.0f) {
                const float s = overThen / (overThen - over);
                const float at[3] = {g_frontThen[0] + (front[0] - g_frontThen[0]) * s, g_frontThen[1] + (front[1] - g_frontThen[1]) * s, 0.0f};
                crossedEdge = AcrossOld(gun, at);
            }
        }
        std::memcpy(g_frontThen, front, sizeof(g_frontThen));
        g_haveFrontThen = true;
        if (g_walled && (!across || over <= 0.0f || over > kWallMost)) {
            g_walled = false;
        } else if (!g_walled && crossedEdge) {
            g_walled = true;
            if (forward < knock) {
                r.touched = true;
                r.hitSpeed = forward;
                g_give = std::max(g_give, kGiveMost * std::clamp(forward / knock, 0.0f, 1.0f));
            }
        }
        if (g_walled && forward >= knock) {
            r.flicked = true;
            r.hitSpeed = forward;
            g_sinceFlick = 0.0f;
            g_walled = false;
            g_give = 0.0f;
            const float harder = std::clamp(forward / knock, 0.8f, 1.6f);
            const float flick[3] = {side == 1 ? -kFlick[0] : kFlick[0], kFlick[1], kFlick[2]};  // (the right's sweep the left's mirror image: off to the gun's other side)
            for (int i = 0; i < 3; ++i)
                r.flickVelocity[i] = (f.gunPose[i * 4] * flick[0] + f.gunPose[i * 4 + 1] * flick[1] + f.gunPose[i * 4 + 2] * flick[2]) * harder;
        } else if (g_walled) {
            past = over;
            g_give = std::max(g_give, kPressGive * std::min(over / kPressFull, 1.0f));
        }
        if (g_give > 0.002f) {
            Rock(gun, g_give, r.oldRock);
            r.oldRocked = true;
        }
    } else {
        g_walled = false;
        g_haveFrontThen = false;
    }

    // The magnet. (A box with a way: by where its way on starts, its top there.)
    float mouthAt[3] = {0.0f, gun.top[1] + gun.mouth, gun.top[2]};
    if (gun.way)
        Transform(gun.way->on[0] + 1, gun.top, mouthAt);
    const float toMouth = Apart(top, mouthAt);
    const float reach = g_magnetReach.load(std::memory_order_relaxed);
    if (g_justOut) {
        g_outFar = std::max(g_outFar, toMouth);
        if (toMouth > reach + kOutClear || (g_outFar > kOutFar && g_outFar - toMouth > kOutComeBack))
            g_justOut = false;
    }
    if (f.wellEmpty && !g_justOut && g_sinceFlick > kAfterFlick && g_heldFrames >= kHeldFrames && toMouth <= reach && turned <= kMagnetTurn) {
        g_state = State::kFly;
        g_fly = 0.0f;
        std::memcpy(g_flyFrom, inGun, sizeof(g_flyFrom));
        const float along = top[1] - gun.top[1];
        g_flyX = gun.mouth < 0.0f ? std::clamp(std::min(along, 0.0f) / gun.mouth, 0.0f, 1.0f) : 1.0f;
        r.caught = true;
        Fly(gun, f, hold, 0.0f, r);
        return r;
    }
    // Stopped against the old one: the magazine and the hand with it.
    if (past > 0.0f) {
        for (int i = 0; i < 3; ++i)
            held[i * 4 + 3] -= f.gunPose[i * 4 + 2] * past;
        std::memcpy(r.glove, f.glove, sizeof(r.glove));
        for (int i = 0; i < 3; ++i)
            r.glove[i * 4 + 3] -= f.gunPose[i * 4 + 2] * past;
        r.gloveBlend = 1.0f;
    }
    // A box just come off its way: eased from where it came free into the hand, as the glove eases off it (about where the hand has it: the game's own
    // place, or where the hand took a wide box), not jumped to the real hand, which may be a little off the way.
    if (gun.way && g_wayEasing) {
        if (g_gloveBlend > 0.0f && f.gunTracked) {
            float from[12], eased[12], a[3], b[3], turned[3];
            const float* about = g_takeOwn ? g_grabAt : ownGrab;
            Compose(f.gunPose, g_wayFrom, from);
            Transform(held, about, a);
            Transform(from, about, b);
            BlendTurn(held, from, g_gloveBlend, eased);
            eased[3] = eased[7] = eased[11] = 0.0f;
            Transform(eased, about, turned);
            for (int i = 0; i < 3; ++i)
                eased[i * 4 + 3] = a[i] + (b[i] - a[i]) * g_gloveBlend - turned[i];
            std::memcpy(held, eased, sizeof(held));
        } else {
            g_wayEasing = false;
        }
    }
    std::memcpy(r.magPose, held, sizeof(held));
    r.mag = true;
    r.holding = true;
    if (grasp < kArm) {
        g_state = State::kNone;
        g_walled = false;
        r.holding = false;
        r.mag = false;
        r.putBack = f.atShoulder;
        r.letGo = !f.atShoulder;
        if (r.letGo)
            LetGoSpeed(r);
    }
    return r;
}

bool Claims(const Gun& gun, const float* gunPose, const float* glove, const float* palm, int side, float* apart) {
    side = side == 1 ? 1 : 0;
    float unused = 0.0f;
    float& from = apart ? *apart : unused;  // (the claiming part's distance from the hand)
    from = 0.0f;
    // Only for the gun Update was last fed: its well, its lid, its belt and its front grip as Update had them. The palm (the caller's grip point on the
    // glove) only has to be a number: the hand is measured from the glove, as Update measures it; the front by the palm too when Update's was.
    if (&gun != g_gunFed || !gunPose || !glove || !Finite(gunPose, 12) || !Finite(glove, 12) || (palm && !Finite(palm, 3)))
        return false;
    // The hand already at work on this gun (a magazine in it, on the seated one, the lever, the lid, the belt): its squeeze is this one's. The other
    // hand's isn't (one hand at a time).
    if (g_state != State::kNone)
        return g_fedSide == side;
    // Update's takes for a fresh squeeze of the grip (the regrip's), the same tests on the same numbers in the same order: the belt hanging out of a new
    // box, the lid, the lever, the seated magazine. (The front grip this side's: the last fed hand's, mirrored for the other, as the profile's are.)
    const float frontGrip[3] = {side != g_fedSide ? -g_frontGrip[0] : g_frontGrip[0], g_frontGrip[1], g_frontGrip[2]};
    float gunInverse[12], palmAt[3], onGun[3], gloveInGun[12], palmFaces[3];
    InvertRigid(gunPose, gunInverse);
    Transform(glove, kHandInGlove[side], palmAt);
    Transform(gunInverse, palmAt, onGun);
    Compose(gunInverse, glove, gloveInGun);
    const float* palmIn = kPalmInGlove[side];
    for (int i = 0; i < 3; ++i)
        palmFaces[i] = gloveInGun[i * 4] * palmIn[0] + gloveInGun[i * 4 + 1] * palmIn[1] + gloveInGun[i * 4 + 2] * palmIn[2];
    const float toLever = Apart(onGun, gun.lever);
    const bool byPalm = g_frontByPalm && palm;
    float palmOnGun[3] = {};
    if (byPalm)
        Transform(gunInverse, palm, palmOnGun);
    const float toFront = FrontApart(onGun, byPalm ? palmOnGun : nullptr, frontGrip);
    if (g_notched && gun.leverNotch > 0.0f) {  // a handle in its notch: a squeeze by it knocks it down; none takes it where it rests at home
        float knob[3];
        NotchKnob(gun, knob);
        if (Apart(onGun, knob) < kKnockReach) {
            from = Apart(onGun, knob);
            return true;
        }
    }
    if (gun.belt) {
        float inHand[3], pinched[3], pinch[3];
        PinchIn(*gun.belt, side, pinch);
        Transform(glove, pinch, inHand);
        Transform(gunInverse, inHand, pinched);
        if (!g_beltLaid && !g_wasEmpty && NearBelt(*gun.belt, pinched) < kBeltReach) {
            from = NearBelt(*gun.belt, pinched);
            return true;
        }
        const float toLid = NearLid(*gun.belt, onGun, side);
        const bool leverTakes = toLever < kLeverReachGrip && toLever < toFront && !g_notched;  // (the grip's take of it, below)
        if (toLid < kLidReach && toLid <= LeverAgainstLid(gun, gunInverse, glove, side, toLever, leverTakes) && toLid < toFront) {
            from = toLid;
            return true;
        }
    }
    if (toLever < kLeverReachGrip && toLever < toFront && !g_notched) {
        from = toLever;
        return true;
    }
    float fromGrab[3];
    FromGrab(gun, onGun, fromGrab);
    if (!g_wasEmpty && OnMagazine(gun, fromGrab, palmFaces[1]) && (!gun.belt || g_lid > 0.5f)) {
        from = Length(fromGrab);
        return true;
    }
    return false;
}

bool Holding() {
    return g_state == State::kHand || g_state == State::kFly || g_state == State::kOnGun;
}

int BusySide() {
    return g_state != State::kNone ? g_fedSide : -1;
}

void BeltReset() {
    g_notched = false;
    g_notch = 0.0f;
    g_lid = g_lidAim = 0.0f;
    g_beltLaid = true;
    g_beltFlying = false;
    if (g_state == State::kLid || g_state == State::kBelt)
        g_state = State::kNone;
}

void Clear() {
    g_state = State::kNone;
    g_gloveBlend = 0.0f;
    g_wayEasing = false;
    g_takeOwn = false;
    g_lostFor = 0.0f;
    g_leverPull = 0.0f;
    g_leverSpring = -1.0f;
    g_walled = false;
    g_haveFrontThen = false;
}

}

// Free hands' film places for rounds 4-9 (freeplaces.h): where a scripted hand's controller is for each place, for either side. Each place is
// a glove (the gloves' bind space into the view's) put where that side's glove works the rifle, then taken back to its controller through the
// line-up; the right's glove is the left's tables mirrored (its line-up, palm point and holds), never fitted again. Round 13's places on the
// body are the grip point itself, the same for either hand, and are checked against holster.h's default places when this builds. Pure: no game
// calls, no logs.
#include "freeplaces.h"

#include <cmath>
#include <cstring>

#include "ammozone.h"
#include "handpose.h"
#include "holster.h"
#include "m320.h"
#include "pouch.h"
#include "reload.h"

namespace freeplaces {
namespace {

using freetest::Where;

constexpr float kPi = 3.14159265f;
// kFront: 30 cm out, 12 cm to its side, 15 cm down in the head's space (hands.cpp's RunFreeTest has the same).
constexpr float kFrontOut = -0.30f, kFrontSide = 0.12f, kFrontDown = -0.15f;
// kFace: the palm point 8 cm to its side, 2 cm up and 42 cm out from the eyes (the head's space). (6 cm down and 4 cm out to the side, the round
// turned in front of the eyes went through the rifle's sight with the rifle in the other hand at the hip: film 7, 2026-11-11, 14.34.)
constexpr float kFaceSide = 0.08f, kFaceDown = 0.02f, kFaceOut = -0.42f;
// kRelease: the palm point's way past the bolt release (the rifle's model space): from 10 cm out on its left to 2 cm into the receiver.
constexpr float kReleaseFrom[3] = {0.10f, -0.03f, 0.0f}, kReleaseTo[3] = {-0.02f, -0.03f, 0.0f};
// kSlap: the slap point's way across the launcher's barrel (the rifle's model space): from 8 cm out from its left side to past its middle.
constexpr float kSlapFrom[3] = {0.135f, -0.036f, 0.60f}, kSlapTo[3] = {-0.065f, -0.036f, 0.60f};
// kLow (round 11): the grip point 68 cm under the neck point, 10 cm to its side and 20 cm forward (the body's axes): a seated player's hand
// lowered beside the knee, past the magnet's 50 cm gate and outside the belly's zone grown by 5 cm (1.33 times its size).
constexpr float kLowDown = 0.68f, kLowSide = 0.10f, kLowForward = 0.20f;
// kBodyAt is measured from the neck point itself; kLauncherAt from the launcher's place's middle, holster.h's default (the back of the neck's)
// when the Scene gives none.
constexpr float kAtNeck[3] = {0.0f, 0.0f, 0.0f};
constexpr holster::Settings kHolsterDefaults{};

// ---- Round 13's places (freetest.h) against holster.h's default places, when this builds: each where the round means it, inside its place or
// in none, and its ways in where holster.h presumes a hand it no longer sees is going (holster::Presume). A default place moved: move these too.
namespace r13 = freetest::round13;
struct P3 {
    float v[3];
};
constexpr P3 Plus(const float* a, const float* b, float u = 1.0f) { return P3{{a[0] + b[0] * u, a[1] + b[1] * u, a[2] + b[2] * u}}; }
constexpr holster::Egg OtherSide(const holster::Egg& e) { return holster::Egg{{-e.mid[0], e.mid[1], e.mid[2]}, {e.half[0], e.half[1], e.half[2]}}; }
constexpr holster::Egg kBellyEgg{{ammozone::kBelly[0], ammozone::kBelly[1], ammozone::kBelly[2]},
                                 {ammozone::kBellyRadii[0], ammozone::kBellyRadii[1], ammozone::kBellyRadii[2]}};
// holster::Size squared, its egg grown by more.
constexpr float SizeSq(const holster::Egg& e, const P3& p, float more = 0.0f) {
    float s = 0.0f;
    for (int i = 0; i < 3; ++i) {
        const float d = (p.v[i] - e.mid[i]) / (e.half[i] + more);
        s += d * d;
    }
    return s;
}
constexpr float LengthSq(const float* v) { return v[0] * v[0] + v[1] * v[1] + v[2] * v[2]; }
// How many places hold p: both shoulders, the back of the neck, both rib cages, both chests and the belly (grown by bellyMore, its put-back).
constexpr int PlacesHolding(const P3& p, float bellyMore = 0.0f) {
    const holster::Settings& s = kHolsterDefaults;
    const holster::Egg eggs[8] = {s.shoulder, OtherSide(s.shoulder), s.neck, s.rib, OtherSide(s.rib), holster::kLeftChestEgg, OtherSide(holster::kLeftChestEgg),
                                  kBellyEgg};
    int n = 0;
    for (int i = 0; i < 8; ++i)
        n += SizeSq(eggs[i], p, i == 7 ? bellyMore : 0.0f) <= 1.0f ? 1 : 0;
    return n;
}
// A hand lost (or not seen) at p on its way straight into the middle of the launcher's place e, its speed's square speedSq: presumed there
// (within the approach, not over its top, going fast enough), and still outside it when outside says so.
constexpr bool PresumedIn(const holster::Egg& e, const P3& p, float speedSq, bool outside) {
    const holster::Settings& s = kHolsterDefaults;
    return SizeSq(e, p, s.approach) <= 1.0f && p.v[1] <= e.mid[1] + e.half[1] + s.above && speedSq >= s.leastSpeed * s.leastSpeed &&
           (!outside || SizeSq(e, p) > 1.0f);
}
// How far along a way in ({from s, to s, then its moments}) its at-th moment is (0-1).
constexpr float Fraction(const float* way, int at) { return (way[at] - way[0]) / (way[1] - way[0]); }
// The square of the speed along a way in of v's length over its seconds.
constexpr float SpeedSq(const float* v, const float* way) { return LengthSq(v) / ((way[1] - way[0]) * (way[1] - way[0])); }
constexpr const holster::Egg& kNeckEgg = kHolsterDefaults.neck;
constexpr const holster::Egg& kRibEgg = kHolsterDefaults.rib;

static_assert(SizeSq(kHolsterDefaults.shoulder, Plus(kAtNeck, r13::kShoulder)) <= 0.36f && PlacesHolding(Plus(kAtNeck, r13::kShoulder)) == 1,
              "round 13's kShoulder: within 0.6 of holster.h's right shoulder and in no other place");
static_assert(PlacesHolding(Plus(kAtNeck, r13::kOutside), ammozone::kPutBackMore) == 0 &&
                  SizeSq(kHolsterDefaults.shoulder, Plus(kAtNeck, r13::kOutside), 0.055f) > 1.0f,
              "round 13's kOutside: in no place, over 5.5 cm out of the right shoulder");
static_assert(PlacesHolding(Plus(kAtNeck, r13::kLeftRest), ammozone::kPutBackMore) == 0 && r13::kLeftRest[1] > -0.5f,
              "round 13's kLeftRest: in no place (the belly with its put-back), over the magnet's 0.5 m gate");
static_assert(PlacesHolding(Plus(kAtNeck, r13::kFrontOfShoulder)) == 0 && PlacesHolding(Plus(kAtNeck, r13::kBehindShoulder)) == 0,
              "round 13's sweep: from no place to no place");
static_assert(SizeSq(kHolsterDefaults.shoulder, P3{{r13::kFrontOfShoulder[0], r13::kFrontOfShoulder[1], r13::kSweepCloses}}) < 1.0f &&
                  PlacesHolding(P3{{r13::kFrontOfShoulder[0], r13::kFrontOfShoulder[1], r13::kSweepCloses}}) == 1 &&
                  r13::kSweep >= 1.5f * kHolsterDefaults.drawSpeed,
              "round 13's sweep: its grip closing inside the right shoulder alone, too fast to draw there");
static_assert(PlacesHolding(Plus(kNeckEgg.mid, r13::kOverShoulder)) == 0 && PlacesHolding(Plus(kRibEgg.mid, r13::kOverShoulder)) == 0,
              "round 13's way in over the shoulder: from no place (to the back of the neck or to the rib)");
static_assert(PresumedIn(kNeckEgg, Plus(kNeckEgg.mid, r13::kOverShoulder, 1.0f - Fraction(r13::kUnseenIn, 2)), SpeedSq(r13::kOverShoulder, r13::kUnseenIn),
                         true) &&
                  PresumedIn(kRibEgg, Plus(kRibEgg.mid, r13::kOverShoulder, 1.0f - Fraction(r13::kUnseenIn, 2)), SpeedSq(r13::kOverShoulder, r13::kUnseenIn),
                             true),
              "round 13's unseen put-away: not seen from outside the place, presumed going into it");
static_assert(LengthSq(r13::kOverShoulder) * (1.0f - Fraction(r13::kUnseenIn, 2)) * (1.0f - Fraction(r13::kUnseenIn, 2)) <=
                  kHolsterDefaults.guessMost * kHolsterDefaults.guessMost,
              "round 13's unseen put-away: the place it keeps (the runtime's guess) within guessMost of where it was last seen");
static_assert(PresumedIn(kNeckEgg, Plus(kNeckEgg.mid, r13::kOverShoulder, 1.0f - Fraction(r13::kLostIn, 2)), SpeedSq(r13::kOverShoulder, r13::kLostIn), false) &&
                  PresumedIn(kRibEgg, Plus(kRibEgg.mid, r13::kOverShoulder, 1.0f - Fraction(r13::kLostIn, 2)), SpeedSq(r13::kOverShoulder, r13::kLostIn),
                             false),
              "round 13's lost put-away: lost where it's presumed at the place");
static_assert(SizeSq(kHolsterDefaults.shoulder, Plus(kNeckEgg.mid, r13::kInShoulder)) < 1.0f && PlacesHolding(Plus(kNeckEgg.mid, r13::kInShoulder)) == 1 &&
                  SizeSq(kHolsterDefaults.shoulder, Plus(kNeckEgg.mid, r13::kInShoulder, 1.0f - Fraction(r13::kLeaveIn, 2))) < 1.0f &&
                  PresumedIn(kNeckEgg, Plus(kNeckEgg.mid, r13::kInShoulder, 1.0f - Fraction(r13::kLeaveIn, 2)), SpeedSq(r13::kInShoulder, r13::kLeaveIn),
                             true) &&
                  PresumedIn(kRibEgg, Plus(kRibEgg.mid, r13::kInShoulder, 1.0f - Fraction(r13::kLeaveIn, 2)), SpeedSq(r13::kInShoulder, r13::kLeaveIn), true),
              "round 13's lost take: lost inside the right shoulder on its way into the back of the neck, presumed there (or at the rib)");
static_assert(r13::kLeaveIn[3] > r13::kLeaveIn[2] && r13::kLeaveIn[3] - r13::kLeaveIn[2] <= kHolsterDefaults.unseenTake,
              "round 13's lost take: squeezed after the loss, within the take's seconds");

const float kIdentity[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};

bool Finite(const float* v, int n) {
    for (int i = 0; i < n; ++i)
        if (!std::isfinite(v[i]))
            return false;
    return true;
}

// a times b (three rows: rotation | translation); out may be a or b.
void Compose(const float* a, const float* b, float* out) {
    float o[12];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 4; ++j)
            o[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j] + (j == 3 ? a[i * 4 + 3] : 0.0f);
    std::memcpy(out, o, sizeof(o));
}

void Transform(const float* m, const float* p, float* out) {
    for (int i = 0; i < 3; ++i)
        out[i] = m[i * 4] * p[0] + m[i * 4 + 1] * p[1] + m[i * 4 + 2] * p[2] + m[i * 4 + 3];
}

// The inverse of a 3x4 (its rotation part need not be exactly a turn: grip.txt's line-up is written to 5 decimals); false when it has none.
bool Invert(const float* m, float* out) {
    const float a = m[0], b = m[1], c = m[2], d = m[4], e = m[5], f = m[6], g = m[8], h = m[9], k = m[10];
    const float c0 = e * k - f * h, c1 = f * g - d * k, c2 = d * h - e * g;
    const float det = a * c0 + b * c1 + c * c2;
    if (!std::isfinite(det) || std::fabs(det) < 1e-12f)
        return false;
    const float s = 1.0f / det;
    float o[12] = {c0 * s, (c * h - b * k) * s, (b * f - c * e) * s, 0.0f,  //
                   c1 * s, (a * k - c * g) * s, (c * d - a * f) * s, 0.0f,  //
                   c2 * s, (b * g - a * h) * s, (a * e - b * d) * s, 0.0f};
    for (int i = 0; i < 3; ++i)
        o[i * 4 + 3] = -(o[i * 4] * m[3] + o[i * 4 + 1] * m[7] + o[i * 4 + 2] * m[11]);
    std::memcpy(out, o, sizeof(o));
    return true;
}

// m's translation set so that its point p lands at at.
void PutAt(float* m, const float* p, const float* at) {
    for (int i = 0; i < 3; ++i)
        m[i * 4 + 3] = at[i] - (m[i * 4] * p[0] + m[i * 4 + 1] * p[1] + m[i * 4 + 2] * p[2]);
}

// A turn of angle (radians) about the unit axis, with no translation.
void AxisTurn(const float* axis, float angle, float* out) {
    const float c = std::cos(angle), s = std::sin(angle), t = 1.0f - c;
    const float x = axis[0], y = axis[1], z = axis[2];
    const float m[12] = {t * x * x + c,     t * x * y - s * z, t * x * z + s * y, 0.0f,  //
                         t * x * y + s * z, t * y * y + c,     t * y * z - s * x, 0.0f,  //
                         t * x * z - s * y, t * y * z + s * x, t * z * z + c,     0.0f};
    std::memcpy(out, m, sizeof(m));
}

void ToQuat(const float* m, float* q) {  // a 3x4's rotation; q: x y z w
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

void FromQuat(const float* q, float* m) {  // q unit; into a 3x4's rotation (its translation untouched)
    const float x = q[0], y = q[1], z = q[2], w = q[3];
    const float r[9] = {1 - 2 * (y * y + z * z), 2 * (x * y - z * w),     2 * (x * z + y * w),
                        2 * (x * y + z * w),     1 - 2 * (x * x + z * z), 2 * (y * z - x * w),
                        2 * (x * z - y * w),     2 * (y * z + x * w),     1 - 2 * (x * x + y * y)};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            m[i * 4 + j] = r[i * 3 + j];
}

// a to b by u (0-1): the turns' quaternions blended and made unit again, the origins along a line. False when that has no turn.
bool BlendPoses(const float* a, const float* b, float u, float* out) {
    float qa[4], qb[4];
    ToQuat(a, qa);
    ToQuat(b, qb);
    const float dot = qa[0] * qb[0] + qa[1] * qb[1] + qa[2] * qb[2] + qa[3] * qb[3];
    float q[4], n = 0.0f;
    for (int i = 0; i < 4; ++i) {
        q[i] = qa[i] + ((dot < 0.0f ? -qb[i] : qb[i]) - qa[i]) * u;
        n += q[i] * q[i];
    }
    n = std::sqrt(n);
    if (!(n > 1e-6f) || !std::isfinite(n))
        return false;
    for (float& v : q)
        v /= n;
    FromQuat(q, out);
    for (int i = 0; i < 3; ++i)
        out[i * 4 + 3] = a[i * 4 + 3] + (b[i * 4 + 3] - a[i * 4 + 3]) * u;
    return true;
}

// The scene as one hand has it: its stand-in grip in the head's space (the left's that grip mirrored about the head), its line-up (the right's the
// left's mirrored) and its inverse, its palm point (the right's x negated), the view's space into the room's and back.
struct Hand {
    int side = 0;
    float standIn[12] = {}, lineUp[12] = {}, lineUpInverse[12] = {};
    float anchor[3] = {};
    float headInverse[12] = {};
};

bool MakeHand(const Scene& scene, int side, Hand& h) {
    h.side = side;
    std::memcpy(h.standIn, scene.standInHead, sizeof(h.standIn));
    std::memcpy(h.lineUp, scene.lineUp, sizeof(h.lineUp));
    if (side == 0)
        handpose::Mirror(h.standIn, h.standIn);
    else
        handpose::Mirror(h.lineUp, h.lineUp);
    h.anchor[0] = side == 1 ? -scene.leftAnchor[0] : scene.leftAnchor[0];
    h.anchor[1] = scene.leftAnchor[1];
    h.anchor[2] = scene.leftAnchor[2];
    return Finite(scene.headPose, 12) && Finite(h.standIn, 12) && Finite(h.lineUp, 12) && Finite(h.anchor, 3) &&
           Invert(h.lineUp, h.lineUpInverse) && Invert(scene.headPose, h.headInverse);
}

// The right glove's fingers' way in the gloves' bind space (handpose.cpp's kAlong, the left's, x negated).
constexpr float kRightFingers[3] = {-0.437f, -0.544f, 0.716f};

// The glove's turn as the side's stand-in grip holds it (its controller turned as that grip, through the line-up); the right's for a slap turned
// half a turn so its palm faces the receiver's left side: for the bolt release about its own fingers (they stay forward and down, as the left's;
// rolled about the rifle's length they pointed up along the rail and the glove first touched the handguard's top, 16-18 cm from the release), for
// the launcher's barrel about the rifle's length.
void SlapTurn(const Hand& h, const float* gun, Where where, float* glove) {
    Compose(h.standIn, h.lineUp, glove);
    if (h.side != 1)
        return;
    float axis[3], roll[12];
    for (int i = 0; i < 3; ++i)
        axis[i] = where == Where::kRelease ? glove[i * 4] * kRightFingers[0] + glove[i * 4 + 1] * kRightFingers[1] + glove[i * 4 + 2] * kRightFingers[2]
                                           : gun[i * 4 + 2];
    const float n = std::sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
    for (float& v : axis)
        v = n > 0.0f ? v / n : 0.0f;
    AxisTurn(axis, kPi, roll);
    Compose(roll, glove, glove);
}

// The glove (the view's space) at a place on the rifle drawn at gun (its model space into the view's), before the shift. False: not now.
bool OnGun(const Scene& scene, const Hand& h, Where where, float along, const float* gun, float* glove) {
    const int side = h.side;
    switch (where) {
    case Where::kHandle: {
        // The V on the handle drawn back along the rifle.
        float lever[12], back[12], onGun[12];
        std::memcpy(lever, reload::kOnLever, sizeof(lever));
        if (side == 1)
            handpose::Mirror(lever, lever);
        std::memcpy(back, kIdentity, sizeof(back));
        back[11] = -along;
        Compose(gun, back, onGun);
        Compose(onGun, lever, glove);
        return true;
    }
    case Where::kMagazine:
    case Where::kWell: {
        // The magazine drawn down the well (kMagazine), or its top on the well's line below home (kWell): the pouch's own hold on it, the side's.
        float shift[12], magPose[12];
        std::memcpy(shift, kIdentity, sizeof(shift));
        shift[7] = where == Where::kMagazine ? -along : along;
        Compose(gun, shift, magPose);
        pouch::GloveFor(magPose, glove, side);
        return true;
    }
    case Where::kRelease:
    case Where::kSlap: {
        // The glove turned as for a slap, the palm point past the bolt release or the slap point across the launcher's barrel.
        SlapTurn(h, gun, where, glove);
        const float* from = where == Where::kRelease ? kReleaseFrom : kSlapFrom;
        const float* to = where == Where::kRelease ? kReleaseTo : kSlapTo;
        float onGun[3], at[3];
        for (int i = 0; i < 3; ++i)
            onGun[i] = (where == Where::kRelease ? reload::kBoltReleaseOnGun[i] : 0.0f) + from[i] + (to[i] - from[i]) * along;
        Transform(gun, onGun, at);
        if (where == Where::kRelease) {
            PutAt(glove, h.anchor, at);
        } else {
            const float point[3] = {side == 1 ? -m320::kSlapPointInGlove[0] : m320::kSlapPointInGlove[0], m320::kSlapPointInGlove[1],
                                    m320::kSlapPointInGlove[2]};
            PutAt(glove, point, at);
        }
        return true;
    }
    case Where::kM320Grip: {
        // The game's own hold on the launcher's grip (learned from the game's hand: none yet, not now).
        if (!scene.haveM320Grip || !Finite(scene.m320Grip, 12))
            return false;
        float hold[12];
        std::memcpy(hold, scene.m320Grip, sizeof(hold));
        if (side == 1)
            handpose::Mirror(hold, hold);
        Compose(gun, hold, glove);
        return true;
    }
    case Where::kMouth: {
        // The round held as that glove last held it, on the open barrel's line along back from home.
        if (!scene.haveRound[side] || !Finite(scene.roundInGlove[side], 12))
            return false;
        float round[12], inverse[12];
        if (!Invert(scene.roundInGlove[side], inverse))
            return false;
        m320::RoundOnLine(m320::kOpenMost, along, round);
        Compose(gun, round, round);
        Compose(round, inverse, glove);
        return true;
    }
    case Where::kRim: {
        // The round on the open barrel's line, the glove as drawn pinching its rim (the hold re-anchored on that side's palm).
        float round[12];
        m320::RoundOnLine(m320::kOpenMost, along, round);
        Compose(gun, round, round);
        m320::DrawnGloveFor(m320::Hold::kPinch, side, round, glove);
        return true;
    }
    default:
        return false;
    }
}

// The controller's grip pose (the room) for the hand at one place (body: kBodyAt's and kLauncherAt's own). False: not now.
bool Place(const Scene& scene, const Hand& h, Where where, float along, const float* shift, const float* body, float* grip) {
    if (!std::isfinite(along))
        return false;
    const int side = h.side;
    float glove[12];
    switch (where) {
    case Where::kStandIn:
    case Where::kMirror:
        Compose(scene.headPose, h.standIn, grip);  // its own side's stand-in grip
        return true;
    case Where::kBodyAt:
    case Where::kLauncherAt: {
        // The grip point at body from the neck point, or from the launcher's place's middle, in the body's axes (the body as the pouch places it),
        // turned as the stand-in grip: the same place for either hand.
        const float* from = where == Where::kBodyAt ? kAtNeck : scene.haveLauncher ? scene.launcher : kHolsterDefaults.neck.mid;
        if (!scene.haveBody || !Finite(scene.neck, 3) || !std::isfinite(scene.yaw) || !Finite(body, 3) || !Finite(from, 3))
            return false;
        Compose(scene.headPose, h.standIn, grip);
        const float forward[3] = {-std::sin(scene.yaw), 0.0f, -std::cos(scene.yaw)}, right[3] = {std::cos(scene.yaw), 0.0f, -std::sin(scene.yaw)};
        const float at[3] = {from[0] + body[0], from[1] + body[1], from[2] + body[2]};
        for (int i = 0; i < 3; ++i)
            grip[i * 4 + 3] = scene.neck[i] + right[i] * at[0] + (i == 1 ? at[1] : 0.0f) + forward[i] * at[2];
        return true;
    }
    case Where::kFront: {
        float inHead[12];
        std::memcpy(inHead, h.standIn, sizeof(inHead));
        inHead[3] = side == 0 ? -kFrontSide : kFrontSide, inHead[7] = kFrontDown, inHead[11] = kFrontOut;
        Compose(scene.headPose, inHead, grip);
        return true;
    }
    case Where::kBelly: {
        // The grip point at the belly's middle (the body as the pouch places it), turned as the stand-in grip.
        if (!scene.haveBody || !Finite(scene.neck, 3) || !std::isfinite(scene.yaw))
            return false;
        Compose(scene.headPose, h.standIn, grip);
        const float forward[3] = {-std::sin(scene.yaw), 0.0f, -std::cos(scene.yaw)}, right[3] = {std::cos(scene.yaw), 0.0f, -std::sin(scene.yaw)};
        for (int i = 0; i < 3; ++i)
            grip[i * 4 + 3] = scene.neck[i] + right[i] * ammozone::kBelly[0] + (i == 1 ? ammozone::kBelly[1] : 0.0f) + forward[i] * ammozone::kBelly[2];
        return true;
    }
    case Where::kLow: {
        // The grip point lowered beside its side's knee (the body as the pouch places it), turned as the stand-in grip: under the magnet's gate and
        // outside the belly's zone (grasp::ReachesDown).
        if (!scene.haveBody || !Finite(scene.neck, 3) || !std::isfinite(scene.yaw))
            return false;
        Compose(scene.headPose, h.standIn, grip);
        const float forward[3] = {-std::sin(scene.yaw), 0.0f, -std::cos(scene.yaw)}, right[3] = {std::cos(scene.yaw), 0.0f, -std::sin(scene.yaw)};
        const float across = side == 0 ? -kLowSide : kLowSide;
        for (int i = 0; i < 3; ++i)
            grip[i * 4 + 3] = scene.neck[i] + right[i] * across + (i == 1 ? -kLowDown : 0.0f) + forward[i] * kLowForward;
        return true;
    }
    case Where::kFace: {
        // The glove turned as the stand-in grip holds it, rolled about the view's forward (the right the other way); its palm point before the eyes.
        float turned[12], roll[12];
        Compose(h.standIn, h.lineUp, turned);
        const float z[3] = {0.0f, 0.0f, 1.0f};
        AxisTurn(z, (side == 0 ? along : -along) * kPi / 180.0f, roll);
        Compose(roll, turned, glove);
        const float face[3] = {side == 0 ? -kFaceSide : kFaceSide, kFaceDown, kFaceOut};
        PutAt(glove, h.anchor, face);
        break;
    }
    default: {
        // On the rifle as drawn last frame, moved by the shift along its own axes.
        if (!scene.haveGun || !Finite(scene.gunRoom, 12) || !Finite(shift, 3))
            return false;
        float gun[12];
        Compose(h.headInverse, scene.gunRoom, gun);
        if (!OnGun(scene, h, where, along, gun, glove))
            return false;
        for (int i = 0; i < 3; ++i)
            glove[i * 4 + 3] += gun[i * 4] * shift[0] + gun[i * 4 + 1] * shift[1] + gun[i * 4 + 2] * shift[2];
        break;
    }
    }
    // The controller behind the glove, through the line-up.
    float room[12];
    Compose(scene.headPose, glove, room);
    Compose(room, h.lineUpInverse, grip);
    return true;
}

}  // namespace

bool Handles(freetest::Where where) { return static_cast<int>(where) > static_cast<int>(freetest::Where::kFront); }

bool Grip(const Scene& scene, const freetest::Step& step, int side, float grip[12]) {
    if (side != 0 && side != 1)
        return false;
    Hand h;
    if (!MakeHand(scene, side, h))
        return false;
    // The way from the from place: eased (a blend not a number is all the way).
    const float b = std::isfinite(step.blend) ? (step.blend > 0.0f ? (step.blend < 1.0f ? step.blend : 1.0f) : 0.0f) : 1.0f;
    const float u = b * b * (3.0f - 2.0f * b);
    float at[12];
    if (step.from == Where::kNone || u >= 1.0f) {
        if (!Place(scene, h, step.where, step.along, step.shift, step.body, at))
            return false;
    } else {
        float from[12];
        if (!Place(scene, h, step.from, step.fromAlong, step.shift, step.fromBody, from))
            return false;
        if (u <= 0.0f) {
            std::memcpy(at, from, sizeof(at));
        } else {
            float to[12];
            if (!Place(scene, h, step.where, step.along, step.shift, step.body, to) || !BlendPoses(from, to, u, at))
                return false;
        }
    }
    if (!Finite(at, 12))
        return false;
    std::memcpy(grip, at, sizeof(at));
    return true;
}

}  // namespace freeplaces

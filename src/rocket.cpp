#include "rocket.h"

#include "ammozone.h"
#include "rocketposes.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace rocket {

// ---- The launcher, as the game has it (docs\research\smaw.md, the mesh dumps of 2026-11-04) ----
// In its frame: the case's bone at home is the frame's turn, moved by this. The case is a cylinder 64 cm long round the line x 0, y 0.0915, 8.9 cm
// across with a flange 13.6 across at its back; from z -0.726 (its back) to -0.085 (its front, 5 cm over the launch tube's back end and 10.6 cm
// inside the launcher's body, whose back is at -0.191).
const float kMouthOnFrame[3] = {0.0f, 0.0915f, -0.191f};
// On the case's own bone (its vertices as they are drawn): its front end. z towards the muzzle.
const float kFrontOnCase[3] = {0.0f, -0.0935f, 0.160f};
// Where the line starts: the case's front end 5 cm behind the launcher's back. Home is 0.
const float kCatchLine = 0.16f;
// The game's own reload, its case's bone frame by frame (allrec, 2026-11-04; the user: "a lock-in rotation system"): it comes 2.7 cm forward,
// twists 20.5 degrees open about its axis in 0.15 s and is pulled away twisted; brought back twisted, it twists shut in 0.14 s and, last of
// all, goes the 2.7 cm back.
const float kTwist = 0.358f;
const float kForward = 0.027f;

namespace {

constexpr float kHomeOnFrame[3] = {0.0f, 0.185f, -0.245f};
// On its bone: its axis runs along z at this x and y, from its back to its front.
constexpr float kAxisX = 0.0f, kAxisY = -0.0935f, kBackZ = -0.481f, kFrontZ = 0.160f;

// ---- Magnets (SetMagnet): make it easy, not automatic. ----
float g_catchHand = 0.20f;  // a new one: the hand this near the launcher's back
float g_catchCase = 0.11f;  // ... or the case's own line this near it, whichever way round it's held
float g_grabReach = 0.17f;  // the spent case: the hand this near its line (the part out of the launcher)
float g_dwell = 0.30f;      // on the line this long, it goes on by itself
float g_lockWait = 0.30f;   // at the end this long, it turns shut by itself

// ---- The wrist and the pull ----
constexpr float kWristTwist = 0.21f;  // a roll of the wrist this far (12 degrees) turns it all the way, open or shut
constexpr float kPullToOpen = 0.025f; // or a pull this far: it turns open as it starts to come
constexpr float kTwistTime = 0.14f;   // turning shut by itself (s)

// ---- Timing ----
constexpr float kFallLast = 1.3f;     // falling: gone after this (s), or on reaching the floor and lying a moment
constexpr float kFloor = 1.45f;       // the floor this far below the eyes (m; a seated player's is nearer, but it's soon out of sight)
constexpr float kGravity = 9.81f;
constexpr float kGlide = 0.15f;       // from the hand onto the line, or off the launcher into the hand (s)
constexpr float kGlideFar = 0.15f;    // more for a case turned right round (s)
constexpr float kSlideInSpeed = 0.5f; // the last of the way on by itself (m/s), at least kSlideInLeast (s)
constexpr float kSlideInLeast = 0.10f;
constexpr float kSlideOutSpeed = 1.1f;  // let go of part way off: on off by itself (m/s), at least kSlideOutLeast (s)
constexpr float kSlideOutLeast = 0.08f;
constexpr float kSettle = 0.20f;      // back to its place at rest, shut (s)
constexpr float kBackHome = 0.12f;    // let go of before it's open: back shut (s)
constexpr float kSeatNear = 0.04f;    // pushed this near home it goes on by itself
constexpr float kMaxStep = 0.05f;     // the most a push or a pull moves it a frame (m): a tracking jump can't throw it
constexpr float kGloveEase = 0.12f;   // the glove onto the case and back (s)
constexpr float kLost = 1.0f;         // a lost hand keeps it this long (s)
constexpr float kOffLine = 0.32f;     // the hand this far from where it caught or took it, other than along the line (m): off, back in the hand
constexpr int kOffFrames = 3;         // ... this many frames running (a tracking jump is a frame or two)
constexpr float kAwayMore = 1.5f;     // pulled off the line, not caught again until this much further away

// ---- The squeeze: armed below 0.4, crossing 0.6 from there; let go below 0.35 ----
constexpr float kArm = 0.4f, kTake = 0.6f, kLetGo = 0.35f;
constexpr float kReachWindow = 0.40f;  // it counts if the hand gets there within this of the crossing (s), still squeezed

// ---- The zone over the shoulder, on the neck point and the body's heading: over either shoulder, generous (nothing tracks the
// shoulders, and it's reached without looking). ----
float g_zoneBelow = 0.12f, g_zoneForward = 0.12f, g_zoneReach = 0.60f;  // (the shoulder's, kept for SetZone's callers: the zone is the belly's now, ammozone.h)
constexpr float kPutBackMore = ammozone::kPutBackMore;  // let go of within this more than the zone: back

// The hand's axes in the gloves' bind space (handpose.cpp): across the hand from the thumb to the little finger, out of the palm; its middle.
constexpr float kAcross[3] = {0.633f, -0.380f, -0.675f};
constexpr float kPalm[3] = {-0.639f, -0.748f, -0.179f};
constexpr float kMiddle[3] = {0.537f, 1.098f, 0.207f};
constexpr float kHoldAsTheGame = -2.0f;  // MakeHold's holdZ: where along the case the game's own hand is
// The hand's middle stays on the part of the case it fits on, 13 cm wide and not even about its middle, either way round: clear of the flange at the back (z -0.447) and of the
// launcher's back (z 0.027, the case come 2.7 cm forward).
constexpr float kHandBackZ = -0.376f, kHandFrontZ = -0.044f;

enum class State {
    kLoaded,     // a live one on, as the game has it
    kSpent,      // fired; the spent one on, waiting for the hand
    kGrabbed,    // the hand on it, turning it open
    kPulling,    // open, coming off along the line with the hand
    kSliding,    // let go of part way off: on off by itself
    kHandSpent,  // off, in the hand
    kFalling,    // let go of: falling
    kEmpty,      // nothing on: waiting for a new one
    kHand,       // a new one in the hand
    kLine,       // on the line, twisted open
    kSeating,    // its last slide on
    kLocking,    // on, waiting to be turned shut
    kSettling,   // shut, going back to its place at rest
};
State g_state = State::kLoaded;
float g_time = 0.0f;      // in this state (s)
float g_clock = 0.0f;
bool g_armed = true;
float g_crossedAt = -1.0f;
float g_along = 0.0f;     // on the line: metres back from its place at rest
float g_twistNow = 0.0f;  // its twist there (radians)
float g_alongFrom = 0.0f, g_slideTime = 0.0f;  // where the slide on or off started, how long it takes (s)
float g_handAt = 0.0f;    // the hand's place along the launcher's z when it was last measured
float g_gloveBlend = 0.0f;
float g_hold[12] = {};          // the case in the glove (case -> bind): how it's held now
float g_gloveOnFrame[12] = {};  // the glove on the case on the line, on the launcher's frame (it eases back from there)
float g_fromHand[12] = {};      // the case as the hand had it as it was caught, on the launcher's frame: the glide starts there
float g_fromLine[12] = {};      // the case as last drawn on the line, in the view's space: the glide into the hand starts there
float g_glide = kGlide;
float g_lostFor = 0.0f;
int g_offFor = 0;
float g_caughtHand[3] = {};  // the hand's middle on the launcher's frame when it was caught or took it
float g_rollFrom = 0.0f;     // the wrist's roll about the line then
float g_pullFrom = 0.0f;     // the hand's place along the launcher's z then
bool g_waitAway = false;     // pulled off the line: not caught again until well away
// Falling, in the room.
float g_fallAt[3] = {}, g_fallVelocity[3] = {}, g_fallTurn[9] = {}, g_fallSpin[3] = {}, g_fallMiddle[3] = {};
// Its fall's start this frame, for Result::fell; whose the fall is (SetFallOwn).
bool g_fellNow = false;
float g_fellPose[12] = {}, g_fellVelocity[3] = {};
bool g_fallOwn = true;
// The grip over the last four frames and their lengths: the hand's speed when it lets go.
float g_gripAt[4][3] = {};
float g_gripDt[4] = {};
int g_gripNext = 0, g_gripCount = 0;
constexpr float kMaxDropSpeed = 3.0f;
float g_lastCase[12] = {};  // the case as last drawn, in the view's space
float g_lastCaseRoom[12] = {};  // ... in the hand, in the room

float Length3(const float* v) {
    return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

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

void Turn(const float* m, const float* v, float* out) {
    for (int i = 0; i < 3; ++i)
        out[i] = m[i * 4] * v[0] + m[i * 4 + 1] * v[1] + m[i * 4 + 2] * v[2];
}

void ToQuat(const float* m, float* q) {  // m: a 3x4's rotation; q: x y z w
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

void FromQuat(const float* q, float* m) {  // into a 3x4's rotation (its translation untouched)
    const float n = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    const float x = q[0] / n, y = q[1] / n, z = q[2] / n, w = q[3] / n;
    const float r[9] = {1 - 2 * (y * y + z * z), 2 * (x * y - z * w),     2 * (x * z + y * w),
                        2 * (x * y + z * w),     1 - 2 * (x * x + z * z), 2 * (y * z - x * w),
                        2 * (x * z - y * w),     2 * (y * z + x * w),     1 - 2 * (x * x + y * y)};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            m[i * 4 + j] = r[i * 3 + j];
}

// a and b blended: s 0 is a, 1 is b.
void Blend(const float* a, const float* b, float s, float* out) {
    float qa[4], qb[4];
    ToQuat(a, qa), ToQuat(b, qb);
    if (qa[0] * qb[0] + qa[1] * qb[1] + qa[2] * qb[2] + qa[3] * qb[3] < 0.0f)
        for (float& v : qb)
            v = -v;
    float q[4];
    for (int i = 0; i < 4; ++i)
        q[i] = qa[i] + (qb[i] - qa[i]) * s;
    FromQuat(q, out);
    for (int i = 0; i < 3; ++i)
        out[i * 4 + 3] = a[i * 4 + 3] + (b[i * 4 + 3] - a[i * 4 + 3]) * s;
}

float Smooth(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float Wrap(float a) {
    return std::remainder(a, 6.2831853f);
}

void Normalize(float* v) {
    const float n = Length3(v);
    if (n > 1e-6f)
        for (int i = 0; i < 3; ++i)
            v[i] /= n;
}

void Cross(const float* a, const float* b, float* o) {
    o[0] = a[1] * b[2] - a[2] * b[1], o[1] = a[2] * b[0] - a[0] * b[2], o[2] = a[0] * b[1] - a[1] * b[0];
}

// How the hand holds the case (case -> the gloves' bind space): the game's own grip (rocketposes.h), as it may be turned on a round case. frontOutOfThumb:
// its front out of the thumb's side (else the little finger's; the game's own hold is the latter); palmOnCase: which way on the case's own axes the palm
// faces, across it (null: as in the game's hold); holdZ: where along the case (its bone's z) the hand's middle is, or kHoldAsTheGame.
void MakeHold(bool frontOutOfThumb, const float* palmOnCase, float holdZ, float* hold) {
    // The game's hold: the hand's middle, its across and its palm on the case.
    float inverse[12], middle[3], across[3], palm[3];
    InvertRigid(rocketposes::kCaseInHand, inverse);
    Transform(inverse, kMiddle, middle);
    Turn(inverse, kAcross, across);
    Turn(inverse, kPalm, palm);
    const float angleThere = std::atan2(palm[1], palm[0]);
    const bool flip = frontOutOfThumb != (across[2] < 0.0f);  // end for end in the hand
    const float z = holdZ == kHoldAsTheGame ? middle[2] : holdZ;
    // The way the palm is to face, and the turn about the axis that brings the case as it's to be onto the game's hold. Flipped first, about its x.
    float angleWanted = flip ? -angleThere : angleThere;
    if (palmOnCase && std::fabs(palmOnCase[0]) + std::fabs(palmOnCase[1]) > 0.05f)
        angleWanted = std::atan2(palmOnCase[1], palmOnCase[0]);
    const float turn = flip ? angleThere + angleWanted : angleThere - angleWanted;
    const float c = std::cos(turn), s = std::sin(turn), f = flip ? -1.0f : 1.0f;
    // The case as it's to be -> the case in the game's hold: its axis onto its axis, the place along it onto the hand's.
    float toThere[12] = {c, -s * f, 0.0f, 0.0f, s, c * f, 0.0f, 0.0f, 0.0f, 0.0f, f, 0.0f};
    const float from[3] = {kAxisX, kAxisY, z};
    const float to[3] = {kAxisX, kAxisY, middle[2]};
    for (int i = 0; i < 3; ++i)
        toThere[i * 4 + 3] = to[i] - (toThere[i * 4] * from[0] + toThere[i * 4 + 1] * from[1] + toThere[i * 4 + 2] * from[2]);
    Compose(rocketposes::kCaseInHand, toThere, hold);
}

// Held in the hand, as the game holds it, whichever way round points its front more the way the launcher points: never to be flipped on the way there.
void HoldTowardsLauncher(const Frame& f) {
    float acrossView[3];
    Turn(f.glove, kAcross, acrossView);
    const float towards = acrossView[0] * f.launcherPose[2] + acrossView[1] * f.launcherPose[6] + acrossView[2] * f.launcherPose[10];
    MakeHold(towards < 0.0f, nullptr, kHoldAsTheGame, g_hold);
}

// The hold nearest to the hand as it is, for the case where it is (view's space): whichever way round, the palm facing the way it faces, at the
// place along the case the hand is at (kept to the part of it a hand can be on).
void HoldNearest(const float* casePose, const float* glove, float lowZ, float highZ, float* hold) {
    float caseInverse[12], gloveOnCase[12];
    InvertRigid(casePose, caseInverse);
    Compose(caseInverse, glove, gloveOnCase);
    float across[3], palm[3], middle[3];
    Turn(gloveOnCase, kAcross, across);
    Turn(gloveOnCase, kPalm, palm);
    Transform(gloveOnCase, kMiddle, middle);
    // Its z is the hand's across or the opposite: the opposite is the thumb's side (MakeHold). Whichever the hand already has.
    MakeHold(across[2] < 0.0f, palm, std::clamp(middle[2], lowZ, highZ), hold);
}

// The wrist's roll about the launcher's line: which way across the line the palm faces (radians).
float WristRoll(const float* toFrame, const float* glove) {
    float palmView[3], palm[3];
    Turn(glove, kPalm, palmView);
    Turn(toFrame, palmView, palm);
    return std::atan2(palm[1], palm[0]);
}

void Enter(State s) {
    g_state = s;
    g_time = 0.0f;
}

// The case on the launcher's back, so a launcher let go of now lies with it: a live one, the spent one, one turning open or coming off, one settling.
bool CaseOn(State s) {
    return s == State::kLoaded || s == State::kSpent || s == State::kGrabbed || s == State::kPulling || s == State::kSettling;
}

void StartFall(const Frame& f, const float* casePose, const float* velocityRoom) {
    g_fellNow = true;
    std::memcpy(g_fellPose, casePose, sizeof(g_fellPose));
    std::memcpy(g_fellVelocity, velocityRoom, sizeof(g_fellVelocity));
    float room[12];
    Compose(f.headPose, casePose, room);
    // Fall about the case's own middle.
    g_fallMiddle[0] = kAxisX, g_fallMiddle[1] = kAxisY, g_fallMiddle[2] = (kBackZ + kFrontZ) * 0.5f;
    Transform(room, g_fallMiddle, g_fallAt);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            g_fallTurn[i * 3 + j] = room[i * 4 + j];
        g_fallVelocity[i] = velocityRoom[i];
    }
    // A slow tumble, about an axis across it.
    const float across[3] = {room[0], room[4], room[8]};
    for (int i = 0; i < 3; ++i)
        g_fallSpin[i] = across[i] * 1.4f;
}

// Falls one frame; the case's pose in the view's space.
void StepFall(const Frame& f, float dt, float* pose) {
    g_fallVelocity[1] -= kGravity * dt;
    for (int i = 0; i < 3; ++i)
        g_fallAt[i] += g_fallVelocity[i] * dt;
    const float eyes = f.headPose[7];
    if (g_fallAt[1] < eyes - kFloor) {
        g_fallAt[1] = eyes - kFloor;
        g_fallVelocity[0] = g_fallVelocity[1] = g_fallVelocity[2] = 0.0f;
        g_fallSpin[0] = g_fallSpin[1] = g_fallSpin[2] = 0.0f;
    }
    // Turned about the spin (Rodrigues; small steps).
    const float angle = Length3(g_fallSpin) * dt;
    if (angle > 1e-5f) {
        float k[3] = {g_fallSpin[0], g_fallSpin[1], g_fallSpin[2]};
        Normalize(k);
        const float c = std::cos(angle), s = std::sin(angle), t = 1.0f - c;
        const float r[9] = {t * k[0] * k[0] + c,        t * k[0] * k[1] - s * k[2], t * k[0] * k[2] + s * k[1],
                            t * k[0] * k[1] + s * k[2], t * k[1] * k[1] + c,        t * k[1] * k[2] - s * k[0],
                            t * k[0] * k[2] - s * k[1], t * k[1] * k[2] + s * k[0], t * k[2] * k[2] + c};
        float turned[9];
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                turned[i * 3 + j] = r[i * 3] * g_fallTurn[j] + r[i * 3 + 1] * g_fallTurn[3 + j] + r[i * 3 + 2] * g_fallTurn[6 + j];
        std::memcpy(g_fallTurn, turned, sizeof(turned));
    }
    float room[12], fromRoom[12];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            room[i * 4 + j] = g_fallTurn[i * 3 + j];
        room[i * 4 + 3] = g_fallAt[i] - (g_fallTurn[i * 3] * g_fallMiddle[0] + g_fallTurn[i * 3 + 1] * g_fallMiddle[1] +
                                         g_fallTurn[i * 3 + 2] * g_fallMiddle[2]);
    }
    InvertRigid(f.headPose, fromRoom);
    Compose(fromRoom, room, pose);
}

// The grip's speed over the last frames held, in the room; at most kMaxDropSpeed.
void HandSpeed(float* v) {
    v[0] = v[1] = v[2] = 0.0f;
    if (g_gripCount < 2)
        return;
    const int newest = (g_gripNext + 3) % 4, oldest = (g_gripNext + 4 - g_gripCount) % 4;
    float time = 0.0f;
    for (int k = 1; k < g_gripCount; ++k)
        time += g_gripDt[(oldest + k) % 4];
    if (time <= 0.0f)
        return;
    for (int i = 0; i < 3; ++i)
        v[i] = (g_gripAt[newest][i] - g_gripAt[oldest][i]) / time;
    const float speed = Length3(v);
    if (speed > kMaxDropSpeed)
        for (int i = 0; i < 3; ++i)
            v[i] *= kMaxDropSpeed / speed;
}

// How near a point (on the launcher's frame) comes to a stretch of the z axis's line at the launcher's middle.
float FromCaseLine(const float* p, float zLow, float zHigh) {
    const float z = std::clamp(p[2], zLow, zHigh);
    const float d[3] = {p[0] - kMouthOnFrame[0], p[1] - kMouthOnFrame[1], p[2] - z};
    return Length3(d);
}

// How near the launcher's back comes to the case's line as the hand holds it (the stretch between its ends).
float MouthFromCase(const float* caseOnFrame) {
    float back[3], front[3];
    const float backOnCase[3] = {kAxisX, kAxisY, kBackZ}, frontOnCase[3] = {kAxisX, kAxisY, kFrontZ};
    Transform(caseOnFrame, backOnCase, back);
    Transform(caseOnFrame, frontOnCase, front);
    float along[3], to[3];
    for (int i = 0; i < 3; ++i)
        along[i] = front[i] - back[i], to[i] = kMouthOnFrame[i] - back[i];
    const float length2 = along[0] * along[0] + along[1] * along[1] + along[2] * along[2];
    const float t = length2 > 0.0f ? std::clamp((to[0] * along[0] + to[1] * along[1] + to[2] * along[2]) / length2, 0.0f, 1.0f) : 0.0f;
    const float d[3] = {to[0] - along[0] * t, to[1] - along[1] * t, to[2] - along[2] * t};
    return Length3(d);
}

}  // namespace

void CaseOnLine(float s, float* rel, float twist) {
    // Turned about its own axis, which runs along the frame's z at the launcher's middle (y 0.0915): as the game's rows read, its y leaning
    // towards the frame's x.
    const float c = std::cos(twist), n = std::sin(twist), off = kHomeOnFrame[1] - kMouthOnFrame[1];
    const float m[12] = {c, n, 0, n * off, -n, c, 0, kMouthOnFrame[1] + c * off, 0, 0, 1, kHomeOnFrame[2] - s};
    std::memcpy(rel, m, sizeof(m));
}

void SetZone(float below, float forward, float reach) {
    g_zoneBelow = below, g_zoneForward = forward, g_zoneReach = reach;
}

void SetMagnet(float catchHand, float catchCase, float grab, float dwell, float lockWait) {
    g_catchHand = catchHand, g_catchCase = catchCase, g_grabReach = grab, g_dwell = dwell, g_lockWait = lockWait;
}

void Clear() {
    g_state = State::kLoaded;
    g_time = 0.0f;
    g_armed = true;
    g_crossedAt = -1.0f;
    g_gloveBlend = 0.0f;
    g_lostFor = 0.0f;
    g_offFor = 0;
    g_gripCount = 0;
    g_waitAway = false;
    g_along = 0.0f;
    g_twistNow = 0.0f;
    g_fellNow = false;
}

void SetFallOwn(bool own) {
    g_fallOwn = own;
}

Result Update(const Frame& f) {
    Result r;
    g_fellNow = false;
    const float dt = std::clamp(std::isfinite(f.dt) ? f.dt : 0.0f, 0.0f, 0.25f);
    g_clock += dt;
    g_time += dt;

    // The squeeze.
    const float squeeze = std::isfinite(f.squeeze) ? f.squeeze : 0.0f;
    bool crossed = false;
    if (squeeze < kArm)
        g_armed = true;
    if (g_armed && squeeze >= kTake) {
        g_armed = false;
        crossed = true;
        g_crossedAt = g_clock;
    }
    // Free hands: the hand on the launcher's pistol grip, or its squeeze just spent on taking a gun up: it takes nothing, now or later with that squeeze.
    if (!f.handFree) {
        crossed = false;
        g_crossedAt = -1.0f;
    }
    const bool fresh = g_crossedAt >= 0.0f && g_clock - g_crossedAt <= kReachWindow && squeeze >= kTake;

    // The grip point, kept for the speed it lets go at: where the glove's hand middle is, in the room.
    float handView[3] = {}, handRoom[3] = {};
    if (f.left) {
        Transform(f.glove, kMiddle, handView);
        Transform(f.headPose, handView, handRoom);
        const float d[3] = {handRoom[0] - f.neck[0], handRoom[1] - f.neck[1], handRoom[2] - f.neck[2]};
        r.fromNeck[0] = std::cos(f.yaw) * d[0] - std::sin(f.yaw) * d[2];
        r.fromNeck[1] = d[1];
        r.fromNeck[2] = -std::sin(f.yaw) * d[0] - std::cos(f.yaw) * d[2];
    }
    const bool inHandFree = g_state == State::kHand || g_state == State::kHandSpent;
    if (f.left && inHandFree) {
        std::memcpy(g_gripAt[g_gripNext], handRoom, sizeof(handRoom));
        g_gripDt[g_gripNext] = dt;
        g_gripNext = (g_gripNext + 1) % 4;
        g_gripCount = std::min(g_gripCount + 1, 4);
    } else if (!inHandFree) {
        g_gripCount = 0;
    }

    const bool newOne = g_state == State::kHand || g_state == State::kLine || g_state == State::kSeating || g_state == State::kLocking;
    const bool spentOne = g_state == State::kGrabbed || g_state == State::kPulling || g_state == State::kHandSpent;
    const bool holdingNow = newOne || spentOne;
    // The game's own reload: its animation has the case. A hand on it lets go.
    if (f.gameReload) {
        if (holdingNow)
            r.cancelled = true;
        Enter(State::kLoaded);
        g_along = g_twistNow = 0.0f;
        r.show = Show::kGame;
        r.empty = !f.loaded;
        g_gloveBlend = std::max(g_gloveBlend - dt / kGloveEase, 0.0f);
        r.gloveBlend = f.launcher ? g_gloveBlend : 0.0f;
        if (r.gloveBlend > 0.0f)
            Compose(f.launcherPose, g_gloveOnFrame, r.glove);
        r.state = "the game's reload";
        r.caseOn = CaseOn(g_state);
        return r;
    }
    // A rocket in it without the hand: a checkpoint, a new life.
    if (f.loaded && g_state != State::kLoaded && g_state != State::kSettling) {
        if (holdingNow)
            r.cancelled = true;
        g_along = g_twistNow = 0.0f;
        Enter(State::kLoaded);
    }
    if (!f.launcher) {
        // Gone with the launcher: a new one in the hand or on its way goes back, the spent one still on stays on, the one off is gone.
        if (newOne || g_state == State::kHandSpent || g_state == State::kFalling || g_state == State::kSliding) {
            if (holdingNow)
                r.cancelled = true;
            Enter(State::kEmpty);
        } else if (g_state == State::kGrabbed || g_state == State::kPulling) {
            r.cancelled = true;
            Enter(State::kSpent);
        } else if (g_state == State::kSettling) {
            Enter(State::kLoaded);
        }
        g_along = g_twistNow = 0.0f;
        g_gloveBlend = 0.0f;
        r.empty = !f.loaded;
        r.show = g_state == State::kLoaded || g_state == State::kSpent ? Show::kGame : Show::kNone;
        r.state = "no launcher";
        r.caseOn = CaseOn(g_state);
        return r;
    }
    // Free hands: no hand holds the launcher (it lies, or flies into a hand from the ground). Nothing is caught at its back or taken off it. The spent case
    // being turned or pulled stays on (let go of by itself); one on its line, going on or turning shut falls from where it's drawn, and so does the spent one
    // sliding off its back (at its slide's speed); one settling is on. One in the hand stays there and falls when let go of, as ever.
    if (!f.held) {
        const State was = g_state;
        if (g_state == State::kGrabbed || g_state == State::kPulling) {
            r.cancelled = true;
            g_along = g_twistNow = 0.0f;
            Enter(State::kSpent);
        } else if (g_state == State::kLine || g_state == State::kSeating || g_state == State::kLocking || g_state == State::kSliding) {
            float v[3] = {};
            if (g_state == State::kSliding) {
                const float zView[3] = {f.launcherPose[2], f.launcherPose[6], f.launcherPose[10]};
                for (int i = 0; i < 3; ++i)
                    v[i] = -(f.headPose[i * 4] * zView[0] + f.headPose[i * 4 + 1] * zView[1] + f.headPose[i * 4 + 2] * zView[2]) * kSlideOutSpeed;
            } else {
                r.letGo = true;
            }
            StartFall(f, g_lastCase, v);
            g_along = g_twistNow = 0.0f;
            Enter(State::kFalling);
        } else if (g_state == State::kSettling && f.loaded) {  // (its count not up yet: it settles as before)
            g_along = 0.0f;
            Enter(State::kLoaded);
        }
        if (g_state != was)
            g_lostFor = 0.0f;  // (nothing held now: a lost hand has nothing to let go of below)
    }

    float toFrame[12];
    InvertRigid(f.launcherPose, toFrame);
    float handOnFrame[3] = {};
    if (f.left)
        Transform(toFrame, handView, handOnFrame);

    // The case as the hand holds it, this frame.
    float inHand[12] = {};
    if (f.left && inHandFree)
        Compose(f.glove, g_hold, inHand);

    // The lost hand.
    if (holdingNow && !f.left) {
        g_lostFor += dt;
        if (g_lostFor > kLost) {
            r.cancelled = true;
            Enter(g_state == State::kGrabbed || g_state == State::kPulling ? State::kSpent : State::kEmpty);
        }
    } else {
        g_lostFor = 0.0f;
    }

    // ---- What goes on, state by state ----
    if (g_state == State::kLoaded && !f.loaded && !f.gameReload) {
        r.spent = true;
        g_along = g_twistNow = 0.0f;
        Enter(State::kSpent);
    }

    // A squeeze on the spent case: the part of it out of the launcher.
    if (g_state == State::kSpent) {
        // It comes back to rest if it was let go of on the way open.
        g_along = std::min(g_along + kForward * dt / kBackHome, 0.0f);
        g_twistNow = std::max(g_twistNow - kTwist * dt / kBackHome, 0.0f);
        if (f.left && f.held && (crossed || fresh)) {
            const float reach = FromCaseLine(handOnFrame, -0.70f, -0.20f);
            if (reach < g_grabReach) {
                r.grabbed = true;
                r.squeezed = crossed;
                g_crossedAt = -1.0f;
                float rel[12], onView[12];
                CaseOnLine(g_along, rel, g_twistNow);
                Compose(f.launcherPose, rel, onView);
                HoldNearest(onView, f.glove, kHandBackZ, kHandFrontZ, g_hold);
                g_rollFrom = WristRoll(toFrame, f.glove);
                g_pullFrom = handOnFrame[2];
                std::memcpy(g_caughtHand, handOnFrame, sizeof(g_caughtHand));
                g_offFor = 0;
                Enter(State::kGrabbed);
            } else if (crossed) {
                r.squeezed = true;
                r.note = "not on the spent case";
            }
        }
    }

    if (g_state == State::kGrabbed) {
        if (squeeze < kLetGo) {
            Enter(State::kSpent);  // back shut
        } else if (f.left) {
            // A roll of the wrist or the start of a pull turns it open and brings it the 2.7 cm forward, as the game's reload does.
            const float roll = std::fabs(Wrap(WristRoll(toFrame, f.glove) - g_rollFrom));
            const float pull = g_pullFrom - handOnFrame[2];
            const float open = std::clamp(std::max(roll / kWristTwist, pull / kPullToOpen), 0.0f, 1.0f);
            const float shown = std::max(open, g_twistNow / kTwist);  // it doesn't go back shut with the wrist
            g_twistNow = kTwist * shown;
            g_along = -kForward * shown;
            if (shown >= 1.0f) {
                r.unlocked = true;
                g_handAt = handOnFrame[2];
                Enter(State::kPulling);
            }
        }
    }

    if (g_state == State::kPulling) {
        if (squeeze < kLetGo) {
            // Let go of open: it goes on off by itself and falls.
            g_alongFrom = g_along;
            g_slideTime = std::max((kCatchLine - g_along) / kSlideOutSpeed, kSlideOutLeast);
            Enter(State::kSliding);
        } else if (f.left) {  // (a lost hand keeps it a moment: kLost)
            const float move = handOnFrame[2] - g_handAt;
            g_handAt = handOnFrame[2];
            g_along = std::clamp(g_along - std::clamp(move, -kMaxStep, kMaxStep), -kForward, kCatchLine);
            // Off the launcher: at the end of the line, or the hand taking it off sideways.
            const float off[3] = {handOnFrame[0] - g_caughtHand[0], handOnFrame[1] - g_caughtHand[1], 0.0f};
            g_offFor = Length3(off) > kOffLine ? g_offFor + 1 : 0;
            if (g_along >= kCatchLine || g_offFor >= kOffFrames) {
                r.freed = true;
                g_glide = kGlide;
                float rel[12];
                CaseOnLine(g_along, rel, kTwist);
                Compose(f.launcherPose, rel, g_fromLine);
                Enter(State::kHandSpent);
            }
        }
    }

    if (g_state == State::kHandSpent) {
        if (squeeze < kLetGo) {
            // From where it was last drawn: in its glide into the hand that isn't the hand yet.
            float v[3];
            HandSpeed(v);
            StartFall(f, g_lastCase, v);
            r.letGo = true;
            Enter(State::kFalling);
        }
    }

    if (g_state == State::kSliding && g_time >= g_slideTime) {
        // Clear of the launcher: it falls, keeping the speed it slid at (back along the line, in the room).
        float rel[12], onView[12];
        CaseOnLine(kCatchLine, rel, kTwist);
        Compose(f.launcherPose, rel, onView);
        const float zView[3] = {f.launcherPose[2], f.launcherPose[6], f.launcherPose[10]};
        float v[3];
        for (int i = 0; i < 3; ++i)
            v[i] = -(f.headPose[i * 4] * zView[0] + f.headPose[i * 4 + 1] * zView[1] + f.headPose[i * 4 + 2] * zView[2]) * kSlideOutSpeed;
        StartFall(f, onView, v);
        Enter(State::kFalling);
    }

    if (g_state == State::kFalling && g_time >= kFallLast)
        Enter(State::kEmpty);

    // A squeeze with nothing on it: a new one, at the belly (ammozone.h; over the shoulder until 2026-11-05).
    if ((g_state == State::kEmpty || g_state == State::kFalling) && f.left && (crossed || fresh)) {
        const bool inside = f.forceTake || (f.haveBody && ammozone::Inside(r.fromNeck));
        if (crossed) {
            r.squeezed = true;
            r.note = !f.haveBody ? "the body isn't placed yet" : !inside ? "not at the belly" : !f.spare ? "no rockets left" : "";
        }
        if (inside && f.spare) {
            r.took = true;
            r.squeezed = crossed;
            g_crossedAt = -1.0f;
            g_gripCount = 0;
            g_waitAway = false;
            HoldTowardsLauncher(f);
            Compose(f.glove, g_hold, inHand);
            g_glide = 0.0f;
            Enter(State::kHand);
        }
    }

    if (g_state == State::kHand) {
        if (squeeze < kLetGo) {
            // At the belly it goes back; anywhere else it falls.
            const bool atShoulder = f.left && f.haveBody && ammozone::Inside(r.fromNeck, kPutBackMore);
            if (atShoulder || !f.left) {
                r.putBack = atShoulder;
                r.cancelled = !atShoulder;
                Enter(State::kEmpty);
            } else {
                float v[3];
                HandSpeed(v);
                StartFall(f, inHand, v);
                r.letGo = true;
                Enter(State::kFalling);
            }
        } else if (f.left && f.held) {
            // Near the back of the launcher, whichever way it's held: by the hand, or by any of the case's own line.
            float onFrame[12];
            Compose(toFrame, inHand, onFrame);
            const float d[3] = {handOnFrame[0] - kMouthOnFrame[0], handOnFrame[1] - kMouthOnFrame[1], handOnFrame[2] - kMouthOnFrame[2]};
            const float byHand = Length3(d), byCase = MouthFromCase(onFrame);
            // Pulled off the line, it isn't caught again until it's been well away.
            if (g_waitAway && byHand > g_catchHand * kAwayMore && byCase > g_catchCase * kAwayMore)
                g_waitAway = false;
            if ((byHand < g_catchHand || byCase < g_catchCase) && !g_waitAway) {
                r.caught = true;
                g_along = kCatchLine;
                g_handAt = handOnFrame[2];
                std::memcpy(g_caughtHand, handOnFrame, sizeof(g_caughtHand));
                g_offFor = 0;
                std::memcpy(g_fromHand, onFrame, sizeof(g_fromHand));
                // The longer glide for a case that's right round the wrong way.
                g_glide = kGlide + kGlideFar * std::clamp(0.5f - 0.5f * onFrame[10], 0.0f, 1.0f);
                float rel[12], lineView[12];
                CaseOnLine(g_along, rel, kTwist);
                Compose(f.launcherPose, rel, lineView);
                HoldNearest(lineView, f.glove, kHandBackZ, kHandFrontZ, g_hold);
                Enter(State::kLine);
            }
        }
    }

    if (g_state == State::kLine) {
        const bool lost = !f.left;
        if (!lost) {
            // Pushed: the hand's move along the launcher's z, from the frame before.
            const float move = handOnFrame[2] - g_handAt;
            g_handAt = handOnFrame[2];
            if (g_time >= g_glide)
                g_along = std::clamp(g_along - std::clamp(move, -kMaxStep, kMaxStep), 0.0f, kCatchLine);
            // The hand moved far from where it caught it, running, other than along the push: it pulled it off.
            const float off[3] = {handOnFrame[0] - g_caughtHand[0], handOnFrame[1] - g_caughtHand[1], std::min(handOnFrame[2] - g_caughtHand[2], 0.0f)};
            g_offFor = Length3(off) > kOffLine && g_time > g_glide ? g_offFor + 1 : 0;
        }
        const bool letGo = squeeze < kLetGo && !lost;
        if (g_offFor >= kOffFrames && !letGo) {
            // Back in the hand as it was caught, gliding there: at once it jumped 40-50 cm in a frame.
            r.offLine = true;
            g_waitAway = true;
            std::memcpy(g_fromLine, g_lastCase, sizeof(g_fromLine));
            g_glide = kGlide;
            Compose(f.glove, g_hold, inHand);
            Enter(State::kHand);
        } else if (g_time >= g_glide && (g_along <= kSeatNear || letGo || (g_dwell >= 0.0f && g_time >= g_dwell + g_glide))) {
            g_alongFrom = g_along;
            g_slideTime = std::max((g_along + kForward) / kSlideInSpeed, kSlideInLeast);
            r.homing = true;
            Enter(State::kSeating);
        }
    }

    if (g_state == State::kSeating) {
        // On by itself, whatever the hand does: to forward of its place at rest, still twisted open.
        g_along = g_alongFrom + (-kForward - g_alongFrom) * Smooth(g_time / g_slideTime);
        g_twistNow = kTwist;
        if (g_time >= g_slideTime) {
            g_along = -kForward;
            if (f.left)
                g_rollFrom = WristRoll(toFrame, f.glove);
            Enter(State::kLocking);
        }
    }

    if (g_state == State::kLocking) {
        // A roll of the wrist turns it shut, or a moment's wait does; the hand on it turning with it. Only then is it loaded.
        const float roll = f.left ? std::fabs(Wrap(WristRoll(toFrame, f.glove) - g_rollFrom)) : 0.0f;
        const float byWrist = roll / kWristTwist;
        const float byItself = g_lockWait >= 0.0f ? Smooth((g_time - g_lockWait) / kTwistTime) : 0.0f;
        const float already = 1.0f - g_twistNow / kTwist;  // it doesn't go back open with the wrist
        const float shut = std::clamp(std::max(std::max(byWrist, byItself), already), 0.0f, 1.0f);
        g_twistNow = kTwist * (1.0f - shut);
        if (shut >= 1.0f) {
            r.seated = true;
            g_twistNow = 0.0f;
            Enter(State::kSettling);
        }
    }
    if (g_state == State::kSettling) {
        g_along = -kForward * (1.0f - Smooth(g_time / kSettle));
        if (g_time >= kSettle) {
            g_along = 0.0f;
            Enter(State::kLoaded);
        }
    }

    // The glove on the case, as the case is on the launcher.
    const bool onCase = g_state == State::kGrabbed || g_state == State::kPulling || g_state == State::kLine || g_state == State::kSeating ||
                        g_state == State::kLocking;

    // ---- What's drawn ----
    switch (g_state) {
    case State::kLoaded:
        r.show = Show::kGame;
        break;
    case State::kSpent:
        if (g_along != 0.0f || g_twistNow != 0.0f) {  // on its way back shut
            CaseOnLine(g_along, r.rel, g_twistNow);
            r.onFrame = true;
            Compose(f.launcherPose, r.rel, r.pose);
            r.show = Show::kAt;
        } else {
            r.show = Show::kGame;
        }
        break;
    case State::kSliding: {
        const float t = std::min(g_time / g_slideTime, 1.0f);
        CaseOnLine(g_alongFrom + (kCatchLine - g_alongFrom) * t, r.rel, kTwist);
        r.onFrame = true;
        Compose(f.launcherPose, r.rel, r.pose);
        std::memcpy(g_lastCase, r.pose, sizeof(g_lastCase));
        r.show = Show::kAt;
        break;
    }
    case State::kFalling:
        if (g_fallOwn) {
            StepFall(f, dt, r.pose);
            r.show = Show::kAt;
        } else {
            r.show = Show::kNone;  // (the caller's: loose.h has it from Result::fell)
        }
        break;
    case State::kEmpty:
        r.show = Show::kNone;
        break;
    case State::kHand:
    case State::kHandSpent:
        if (f.left) {
            Compose(f.glove, g_hold, inHand);
            if (g_time < g_glide)
                Blend(g_fromLine, inHand, Smooth(g_time / g_glide), r.pose);
            else
                std::memcpy(r.pose, inHand, sizeof(r.pose));
            std::memcpy(g_lastCase, r.pose, sizeof(g_lastCase));
            Compose(f.headPose, r.pose, g_lastCaseRoom);
        } else {
            // The hand lost: it stays where it was, in the room. In the view's space it went round with the head.
            float fromRoom[12];
            InvertRigid(f.headPose, fromRoom);
            Compose(fromRoom, g_lastCaseRoom, r.pose);
            std::memcpy(g_lastCase, r.pose, sizeof(g_lastCase));
        }
        r.show = Show::kAt;
        break;
    case State::kGrabbed:
    case State::kPulling:
    case State::kLine:
    case State::kSeating:
    case State::kLocking:
    case State::kSettling: {
        float rel[12];
        CaseOnLine(g_along, rel, g_state == State::kPulling || g_state == State::kLine ? kTwist : g_twistNow);
        if (g_state == State::kLine && g_time < g_glide)
            Blend(g_fromHand, rel, Smooth(g_time / g_glide), r.rel);
        else
            std::memcpy(r.rel, rel, sizeof(r.rel));
        r.onFrame = true;
        Compose(f.launcherPose, r.rel, r.pose);
        std::memcpy(g_lastCase, r.pose, sizeof(g_lastCase));
        r.show = Show::kAt;
        if (onCase) {
            float holdInverse[12];
            InvertRigid(g_hold, holdInverse);
            Compose(r.rel, holdInverse, g_gloveOnFrame);
        }
        break;
    }
    }

    // The glove on the case, easing in and back out.
    g_gloveBlend = std::clamp(g_gloveBlend + (onCase ? dt : -dt) / kGloveEase, 0.0f, 1.0f);
    if (g_gloveBlend > 0.0f) {
        r.gloveBlend = g_gloveBlend;
        Compose(f.launcherPose, g_gloveOnFrame, r.glove);
    }
    r.holding = g_state == State::kHand || g_state == State::kHandSpent || onCase;
    if (g_fellNow) {
        r.fell = true;
        r.fellFromHand = r.letGo;
        std::memcpy(r.fellPose, g_fellPose, sizeof(r.fellPose));
        std::memcpy(r.fellVelocity, g_fellVelocity, sizeof(r.fellVelocity));
        g_fellNow = false;
    }
    // The rounds: empty until home, then by the game's count; the frame of the seat isn't counted in yet.
    r.empty = !f.loaded && !r.seated;
    switch (g_state) {
    case State::kLoaded: r.state = "loaded"; break;
    case State::kSpent: r.state = "spent"; break;
    case State::kGrabbed: r.state = "turning open"; break;
    case State::kPulling: r.state = "coming off"; break;
    case State::kSliding: r.state = "sliding off"; break;
    case State::kHandSpent: r.state = "spent in the hand"; break;
    case State::kFalling: r.state = "falling"; break;
    case State::kEmpty: r.state = "empty"; break;
    case State::kHand: r.state = "in the hand"; break;
    case State::kLine: r.state = "on the line"; break;
    case State::kSeating: r.state = "going on"; break;
    case State::kLocking: r.state = "turning shut"; break;
    case State::kSettling: r.state = "settling"; break;
    }
    r.caseOn = CaseOn(g_state);
    return r;
}

}  // namespace rocket

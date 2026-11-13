#pragma once
// The head's place in the room moved into the game's cameras: lean, duck, peek, step, crouch for real (6DoF).
// The picture has followed the head's turn since the start; its place was known
// every frame and thrown away, so the world moved along with the head.
//
// The rule, each view build (views.cpp, once a frame, before the eyes' half eye distance): each eye's cameras go from the
// game's own place by the body's level frame (the game camera less the hand's yaw and the game's pitch: x its right, z
// its back, y the world's up) applied to the head's place in the room less its neutral place less how far the soldier has
// been walked. Inside the lean limit one to one; near it squeezed, never past it: the game keeps only the soldier's
// collision body out of walls, so the camera stays inside that. The gun and arms are worked out from the head and the
// controllers (hands.cpp), so they stay in the hands as the camera moves.
//
// Decisions (walking the soldier, crouching) go by a neck point behind and below the eyes: a turned or bowed head moves
// the eyes but not it. The numbers are the old Oculus SDK's (0.0805 m back, 0.075 m down).
//
// - Stepping in the room: past followStart the soldier is walked under the head through the virtual stick (pad.cpp), and
//   only the distance the game really moved it counts (FrostbiteVR's method, described in the research note): stopped by a
//   wall, nothing is taken off, and the command gives up for a while with growing pauses. The player's own stick wins.
// - Further than the limit (a real walk, a headset carried about): the neutral place is dragged along past leanFar, and
//   once the head is still it eases to where the head is. Nothing goes dark and nothing jumps.
// - Crouching: the neck past crouchOn below the standing height for crouchHold presses the game's crouch (held, as the
//   right B holds it; measured: the game's eye drops 0.60 m in 0.25 s), under crouchOff lets it go. The game's own drop is
//   added back as it happens, measured from the game's camera, so the view follows the real head alone and doesn't drop
//   twice. Pressed by the button, the game's drop stays: asked for without moving.
// - Higher than tiptoe (upFree) for highSettle seconds: that's the real standing height. The first frame's head may have been
//   a headset being pulled on, or someone bending for a controller.
// - Nobody in the headset (put down on a desk, tracking lost): worn, a head is never this still (putDownMoved in putDownAfter).
//   Nothing's walked, the game's crouch is let go (it's Left Ctrl, held on the desktop's keyboard too), and whatever height
//   it's at counts as standing until it's lifted.
// - A cutscene or a ride: the head moves a little about where it was as the moment began (scriptedFree, scriptedMost),
//   nothing is walked or crouched, and the moves into and out of it are eased away.
// - The stick's smooth turn turns the room about the soldier, and with the head off to one side that would swing the
//   camera in an arc. It's turned about the head instead: the camera stays where it is in the world.
//
// Header only and free of the game, so tools/headmovetest.cpp runs it on scripted heads and soldiers.

#include <algorithm>
#include <cmath>

namespace headmove {

constexpr float kNeckDown = 0.075f, kNeckBack = 0.0805f;

struct Tuning {
    bool walk = true, crouch = true;
    // Along the ground, metres: one to one up to leanFree, squeezed towards leanMost, the neutral place dragged past
    // leanFar.
    float leanFree = 0.25f, leanMost = 0.32f, leanFar = 0.55f;
    // Above the neutral height (tiptoe) and below it (the floor is about 1.6 m below the standing eye).
    float upFree = 0.10f, upMost = 0.18f, upFar = 0.35f;
    float downFree = 1.10f, downMost = 1.25f, downFar = 1.50f;
    // In a cutscene or a ride, in any direction, about where the head was as it began.
    float scriptedFree = 0.10f, scriptedMost = 0.15f, scriptedFar = 0.40f;
    float scriptedCalm = 0.5f;  // seconds after the last sign of one before it counts as over
    // The soldier's walked under the head from followStart (the neck's distance), to followStop, at
    // followGain * (distance - followStop) metres a second, at most followSpeed. The stick for a speed: stickLeast, under
    // which the game hardly walks at all (measured through pad.cpp: 0.1 of the stick walks 0.16 m/s and 0.07 nothing),
    // plus the speed over stickFull (0.2: 0.53 m/s, 0.3: 0.73, 0.5: 1.84, a whole stick 4).
    float followStart = 0.25f, followStop = 0.10f, followGain = 2.4f, followSpeed = 1.2f, stickFull = 3.0f;
    float stickLeast = 0.10f;
    float followTail = 0.4f;     // seconds its stopping still counts as walked
    float blockedWindow = 0.7f;  // seconds to have moved blockedMoved in, or it's stopped by something
    float blockedMoved = 0.03f;
    float blockedPause = 1.5f, blockedPauseMost = 12.0f;
    // The neck this far below the standing height for crouchHold seconds crouches; under crouchOff stands. The standing
    // height follows the neck up at crouchRise metres a second (a pose only passing through, a headset lifted off or a hop, isn't
    // it), sinking crouchSink metres a second while not crouched.
    float crouchOn = 0.30f, crouchOff = 0.20f, crouchHold = 0.2f, crouchSink = 0.01f, crouchRise = 0.10f;
    float crouchChange = 0.8f;  // seconds the game's own crouch or stand is followed for (it takes 0.25 s)
    float gameDrop = 0.60f;     // metres: the game's own crouch at its eye (measured)
    float jump = 0.25f;         // the head moving this far in a step isn't the head (tracking found again, a recenter)
    float teleport = 1.0f;      // the game's camera moving this far in a step: the soldier was put somewhere
    float walkFastest = 3.5f;   // metres a second: anything faster isn't the walk asked for (a platform, a vehicle)
    float lostHold = 1.0f;      // seconds the head's place is held, lost, before easing back to the middle
    float settleStill = 1.5f;   // seconds the head has to be still before a dragged neutral place eases to it
    float highSettle = 2.0f;    // seconds above upFree before that's taken for the standing height
    float putDownMoved = 0.002f, putDownAfter = 5.0f;  // not this many metres in this many seconds: nobody's wearing it
    float easeOut = 0.12f;      // seconds: how fast a change of rule (into a cutscene, a recenter) is eased away
};

struct Input {
    float dt = 0.0f;
    float head[3] = {};               // the eye centre in the room, metres: x right, y up, z back
    float headQ[4] = {0, 0, 0, 1};    // its orientation, x y z w
    bool tracked = false;             // its place known this frame
    double eye[3] = {};               // the game's own camera's place in the world
    float right[3] = {1, 0, 0};       // the body's level frame in the world
    float back[3] = {0, 0, 1};
    bool scripted = false;            // a cutscene, a ride, the game turning the view itself
    bool stick = false;               // the player's own left stick is moving the soldier
    float facingYaw = 0.0f;           // the way the game walks, from the room's straight ahead (radians, left positive)
    bool recentered = false;          // F9: the room's origin was put at the head
};

struct Output {
    float room[3] = {};   // the cameras' move in the room's axes (metres)
    float world[3] = {};  // and in the world
    bool following = false;
    float stickX = 0.0f, stickY = 0.0f;  // the virtual stick while following: x right, y forward of the way the game walks
    bool crouch = false;                 // the game's crouch held by the body
    // For the log.
    float raw[3] = {};
    float comp = 0.0f;
    float drop = 0.0f;
    float neck = 0.0f;
};

enum CrouchPhase { kPhaseNone, kPhaseDown, kPhaseHold, kPhaseUp, kPhaseDecay };

struct State {
    bool have = false;
    float neutral[3] = {};
    float scriptNeutral[3] = {};
    float walked[2] = {};  // x, z in the room
    float lastHead[3] = {};
    float lastQ[4] = {0, 0, 0, 1};
    float lostFor = 0.0f;
    bool scripted = false;
    float calm = 0.0f;
    double lastEye[3] = {};
    bool haveEye = false;
    float lastRight[3] = {1, 0, 0}, lastBack[3] = {0, 0, 1};
    bool haveRows = false;
    bool following = false;
    float tail = 0.0f;
    float windowT = 0.0f, movedWindow = 0.0f;
    float pause = 0.0f, nextPause = 0.0f;
    float standing = 0.0f;
    float crouchFor = 0.0f;
    bool crouch = false;
    CrouchPhase phase = kPhaseNone;
    float phaseT = 0.0f;
    double fromY = 0.0;
    float heldDrop = 0.0f;
    float comp = 0.0f;
    float headSpeed = 0.0f;
    float stillFor = 0.0f;
    bool settle = false;
    float highFor = 0.0f;
    float stillFrom[3] = {};
    float putDownFor = 0.0f;
    float corr[3] = {};
    float lastRoom[3] = {};
};

// One to one up to free, then a curve towards most that meets it smoothly and never gets there.
inline float Squeeze(float r, float free, float most) {
    if (r <= free)
        return r;
    const float room = most - free;
    return room > 1e-5f ? free + room * (1.0f - std::exp(-(r - free) / room)) : free;
}

inline void Rotate(const float* q, const float* v, float* out) {
    const float t[3] = {2 * (q[1] * v[2] - q[2] * v[1]), 2 * (q[2] * v[0] - q[0] * v[2]), 2 * (q[0] * v[1] - q[1] * v[0])};
    out[0] = v[0] + q[3] * t[0] + (q[1] * t[2] - q[2] * t[1]);
    out[1] = v[1] + q[3] * t[1] + (q[2] * t[0] - q[0] * t[2]);
    out[2] = v[2] + q[3] * t[2] + (q[0] * t[1] - q[1] * t[0]);
}

inline void Step(State& s, const Tuning& t, const Input& in, Output& out) {
    const float dt = std::clamp(in.dt, 0.0f, 0.1f);
    float head[3], q[4];
    for (int i = 0; i < 3; ++i)
        head[i] = in.tracked || !s.have ? in.head[i] : s.lastHead[i];
    for (int i = 0; i < 4; ++i)
        q[i] = in.tracked || !s.have ? in.headQ[i] : s.lastQ[i];
    s.lostFor = in.tracked ? 0.0f : s.lostFor + dt;

    const float rest[3] = {0.0f, -kNeckDown, kNeckBack};
    float neck[3];
    Rotate(q, rest, neck);
    for (int i = 0; i < 3; ++i)
        neck[i] += head[i];

    {
        const float d[3] = {head[0] - s.stillFrom[0], head[1] - s.stillFrom[1], head[2] - s.stillFrom[2]};
        s.putDownFor += dt;
        if (d[0] * d[0] + d[1] * d[1] + d[2] * d[2] > t.putDownMoved * t.putDownMoved) {
            s.putDownFor = 0.0f;
            for (int i = 0; i < 3; ++i)
                s.stillFrom[i] = head[i];
        }
    }
    const bool putDown = s.putDownFor >= t.putDownAfter;

    bool rebase = false;
    if (!s.have || in.recentered) {
        for (int i = 0; i < 3; ++i) {
            s.neutral[i] = head[i];
            s.scriptNeutral[i] = head[i];
            s.corr[i] = 0.0f;
            s.lastRoom[i] = 0.0f;
        }
        s.walked[0] = s.walked[1] = 0.0f;
        s.standing = neck[1];
        s.crouchFor = 0.0f;
        s.following = false;
        s.tail = 0.0f;
        s.settle = false;
        s.stillFor = 0.0f;
        s.pause = 0.0f;
        if (s.crouch) {  // this height is standing now
            s.crouch = false;
            s.heldDrop = s.comp;
            s.fromY = in.eye[1];
            s.phaseT = 0.0f;
            s.phase = kPhaseUp;
        }
        s.have = true;
    } else {
        const float jumped[3] = {head[0] - s.lastHead[0], head[1] - s.lastHead[1], head[2] - s.lastHead[2]};
        const float length = std::sqrt(jumped[0] * jumped[0] + jumped[1] * jumped[1] + jumped[2] * jumped[2]);
        if (length > t.jump) {
            // Not the head moving: taken up, the camera stays. (Tracking found again; the room's origin moved.) All but the
            // height that brings it back towards the neutral height, which is the head: taken up too, a head lost low (squatting,
            // a headset going down to a desk) and found standing kept the view that low, and the soldier crouched, until a recenter.
            const float was = s.lastHead[1] - s.neutral[1];
            const float back = was < 0.0f ? std::clamp(jumped[1], 0.0f, -was) : std::clamp(jumped[1], -was, 0.0f);
            for (int i = 0; i < 3; ++i) {
                s.neutral[i] += i == 1 ? jumped[1] - back : jumped[i];
                s.scriptNeutral[i] += jumped[i];
            }
            s.standing += std::min(jumped[1], jumped[1] - back);
        } else if (dt > 0.0f) {
            const float k = 1.0f - std::exp(-dt / 0.15f);
            s.headSpeed += (length / dt - s.headSpeed) * k;
        }
    }

    // A cutscene or a ride, and the way out of one.
    s.calm = in.scripted ? 0.0f : s.calm + dt;
    const bool scripted = in.scripted || (s.scripted && s.calm < t.scriptedCalm);
    if (scripted != s.scripted) {
        s.scripted = scripted;
        rebase = true;
        s.following = false;
        s.tail = 0.0f;
        s.settle = false;
        if (scripted) {
            for (int i = 0; i < 3; ++i)
                s.scriptNeutral[i] = head[i];
        } else {
            // Back over the soldier, the height as it was.
            s.neutral[0] = head[0] - s.walked[0];
            s.neutral[2] = head[2] - s.walked[1];
        }
    }

    // The stick's turn turns the room about the soldier: turned about the head, what's off to one side stays where it is
    // in the world. Not a cut (20 degrees in a step: cos 0.94). A pure turn by the angle between the right rows: the rows are square only to about 0.015 degrees, so going through both of
    // them crept a lean 4 mm a second sideways, with no turn at all; skipping small turns instead let the rows' wobble with the hand shake the eye's place, a head 30 cm off (2026-11-04).
    const float turned = in.right[0] * s.lastRight[0] + in.right[2] * s.lastRight[2];
    if (!s.scripted && s.haveRows && turned > 0.94f) {
        const float across = s.lastRight[0] * in.right[2] - s.lastRight[2] * in.right[0];
        const float length = std::sqrt(turned * turned + across * across);
        const float cosine = turned / length, sine = across / length;
        const float x = head[0] - s.neutral[0] - s.walked[0], z = head[2] - s.neutral[2] - s.walked[1];
        s.neutral[0] = head[0] - s.walked[0] - (x * cosine + z * sine);
        s.neutral[2] = head[2] - s.walked[1] - (z * cosine - x * sine);
    }
    for (int i = 0; i < 3; ++i) {
        s.lastRight[i] = in.right[i];
        s.lastBack[i] = in.back[i];
    }
    s.haveRows = true;

    // How far the game's camera moved. While the soldier's being walked under the head, that much is taken off.
    double step[3] = {0.0, 0.0, 0.0};
    if (s.haveEye)
        for (int i = 0; i < 3; ++i)
            step[i] = in.eye[i] - s.lastEye[i];
    for (int i = 0; i < 3; ++i)
        s.lastEye[i] = in.eye[i];
    s.haveEye = true;
    const float stepH = static_cast<float>(std::sqrt(step[0] * step[0] + step[2] * step[2]));
    const bool teleported = stepH > t.teleport || std::fabs(step[1]) > t.teleport;
    if (teleported) {
        s.following = false;
        s.tail = 0.0f;
    }
    if (!s.scripted && !in.stick && (s.following || s.tail > 0.0f) && stepH <= t.walkFastest * dt + 0.02f) {
        s.walked[0] += static_cast<float>(step[0] * in.right[0] + step[2] * in.right[2]);
        s.walked[1] += static_cast<float>(step[0] * in.back[0] + step[2] * in.back[2]);
        s.movedWindow += stepH;
    }

    float room[3] = {};
    float raw[3] = {};
    if (s.scripted) {
        for (int i = 0; i < 3; ++i)
            raw[i] = head[i] - s.scriptNeutral[i];
        float r = std::sqrt(raw[0] * raw[0] + raw[1] * raw[1] + raw[2] * raw[2]);
        if (r > t.scriptedFar) {
            const float k = (r - t.scriptedFar) / r;
            for (int i = 0; i < 3; ++i) {
                s.scriptNeutral[i] += raw[i] * k;
                raw[i] -= raw[i] * k;
            }
            r = t.scriptedFar;
        }
        const float kept = r > 1e-6f ? Squeeze(r, t.scriptedFree, t.scriptedMost) / r : 1.0f;
        for (int i = 0; i < 3; ++i)
            room[i] = raw[i] * kept;
    } else {
        raw[0] = head[0] - s.neutral[0] - s.walked[0];
        raw[1] = head[1] - s.neutral[1];
        raw[2] = head[2] - s.neutral[2] - s.walked[1];

        // Further than leanFar, or up past upFar (a real walk, a headset being put on): the neutral place comes along.
        float r = std::sqrt(raw[0] * raw[0] + raw[2] * raw[2]);
        if (r > t.leanFar) {
            const float k = (r - t.leanFar) / r;
            s.neutral[0] += raw[0] * k;
            s.neutral[2] += raw[2] * k;
            raw[0] -= raw[0] * k;
            raw[2] -= raw[2] * k;
            r = t.leanFar;
            s.settle = true;
        }
        if (raw[1] > t.upFar) {
            s.neutral[1] += raw[1] - t.upFar;
            raw[1] = t.upFar;
            s.settle = true;
        } else if (raw[1] < -t.downFar) {
            s.neutral[1] += raw[1] + t.downFar;
            raw[1] = -t.downFar;
        }
        s.highFor = raw[1] > t.upFree ? s.highFor + dt : 0.0f;
        if (s.highFor >= t.highSettle)
            s.settle = true;
        // And once the head's been still a moment, the neutral place eases to where it is.
        s.stillFor = s.headSpeed < 0.05f ? s.stillFor + dt : 0.0f;
        if (s.settle && s.stillFor >= t.settleStill) {
            const float ease = 1.0f - std::exp(-dt / 0.4f);
            for (int i = 0; i < 3; ++i) {
                if (i == 1 && raw[1] <= 0.0f)  // below the neutral height is ducking, not a new standing height
                    continue;
                s.neutral[i] += raw[i] * ease;
                raw[i] -= raw[i] * ease;
            }
            r = std::sqrt(raw[0] * raw[0] + raw[2] * raw[2]);
            if (r < 0.01f && raw[1] < 0.01f)
                s.settle = false;
        }

        const float kept = r > 1e-6f ? Squeeze(r, t.leanFree, t.leanMost) / r : 1.0f;
        room[0] = raw[0] * kept;
        room[2] = raw[2] * kept;
        room[1] = raw[1] >= 0.0f ? Squeeze(raw[1], t.upFree, t.upMost) : -Squeeze(-raw[1], t.downFree, t.downMost);
    }

    // Walking the soldier under the head, by the neck.
    out.following = false;
    out.stickX = out.stickY = 0.0f;
    const float neckX = neck[0] - s.neutral[0] - rest[0] - s.walked[0];
    const float neckZ = neck[2] - s.neutral[2] - rest[2] - s.walked[1];
    const float reach = std::sqrt(neckX * neckX + neckZ * neckZ);
    s.pause = std::max(0.0f, s.pause - dt);
    s.tail = std::max(0.0f, s.tail - dt);
    const bool mayWalk = t.walk && !s.scripted && !in.stick && !teleported && !putDown && (in.tracked || s.lostFor == 0.0f);
    if (!mayWalk) {
        if (s.following)
            s.tail = in.stick ? 0.0f : t.followTail;
        s.following = false;
    } else if (!s.following) {
        if (reach > t.followStart && s.pause <= 0.0f) {
            s.following = true;
            s.windowT = 0.0f;
            s.movedWindow = 0.0f;
        }
    } else if (reach < t.followStop) {
        s.following = false;
        s.tail = t.followTail;
        s.nextPause = 0.0f;
    }
    if (reach < t.followStart - 0.03f)
        s.pause = std::min(s.pause, 0.0f);  // came back from whatever stopped it
    if (s.following) {
        s.windowT += dt;
        if (s.windowT >= t.blockedWindow) {
            if (s.movedWindow < t.blockedMoved) {
                // Stopped by something (a wall, the pause menu): asked again later, later each time.
                s.following = false;
                s.tail = 0.2f;
                s.nextPause = std::clamp(s.nextPause * 2.0f, t.blockedPause, t.blockedPauseMost);
                s.pause = s.nextPause;
            } else {
                s.nextPause = 0.0f;
            }
            s.windowT = 0.0f;
            s.movedWindow = 0.0f;
        }
    }
    if (s.following && reach > 1e-4f) {
        const float speed = std::clamp(t.followGain * (reach - t.followStop), 0.0f, t.followSpeed);
        const float along = (t.stickLeast + speed / t.stickFull) / reach;
        // The room's right and forward, turned to the way the game walks (as input.cpp turns the stick from the head's).
        const float x = neckX * along, y = -neckZ * along;
        const float c = std::cos(-in.facingYaw), sn = std::sin(-in.facingYaw);
        out.stickX = x * c - y * sn;
        out.stickY = x * sn + y * c;
        out.following = true;
    }

    // Crouching for real.
    if (!s.scripted && t.crouch && (in.tracked || s.lostFor == 0.0f || putDown)) {
        if (putDown)
            s.standing = neck[1];
        else if (neck[1] >= s.standing)
            s.standing = std::min(neck[1], s.standing + t.crouchRise * dt);
        else if (!s.crouch)
            s.standing = std::max(neck[1], s.standing - t.crouchSink * dt);
        const float drop = s.standing - neck[1];
        if (!s.crouch) {
            s.crouchFor = drop > t.crouchOn ? s.crouchFor + dt : 0.0f;
            if (s.crouchFor >= t.crouchHold) {
                s.crouch = true;
                s.crouchFor = 0.0f;
                s.phase = kPhaseDown;
                s.phaseT = 0.0f;
                s.fromY = in.eye[1];
            }
        } else if (drop < t.crouchOff) {
            s.crouch = false;
            s.heldDrop = s.comp;
            s.fromY = in.eye[1];
            s.phaseT = 0.0f;
            s.phase = kPhaseUp;
        }
        out.drop = drop;
    } else if (!t.crouch && s.crouch) {
        s.crouch = false;
        s.heldDrop = s.comp;
        s.fromY = in.eye[1];
        s.phaseT = 0.0f;
        s.phase = kPhaseUp;
    }
    // The game's own drop added back as it happens, from its camera; in a cutscene its camera has nothing to do with the
    // soldier: left as it was.
    if (!s.scripted) {
        if (teleported && (s.phase == kPhaseDown || s.phase == kPhaseUp)) {
            s.phase = s.crouch ? kPhaseHold : kPhaseDecay;
        } else if (s.phase == kPhaseDown) {
            // Only grows, never past the game's whole drop: walking down a slope or stairs meanwhile isn't its crouch, and
            // left in, the view stayed that much too high for the whole crouch.
            s.comp = std::max(s.comp, std::clamp(static_cast<float>(s.fromY - in.eye[1]), 0.0f, t.gameDrop));
            s.phaseT += dt;
            if (s.phaseT >= t.crouchChange)
                s.phase = kPhaseHold;
        } else if (s.phase == kPhaseUp) {
            s.comp = std::min(s.comp, std::clamp(s.heldDrop - std::max(0.0f, static_cast<float>(in.eye[1] - s.fromY)), 0.0f, 1.2f));
            s.phaseT += dt;
            if (s.phaseT >= t.crouchChange)
                s.phase = kPhaseDecay;
        } else if (s.phase == kPhaseHold && s.comp > 0.5f * t.gameDrop) {
            // Rising ground hid some of the drop: the crouch was seen, so it's all of it.
            s.comp += (t.gameDrop - s.comp) * (1.0f - std::exp(-dt / 0.1f));
        } else if (s.phase == kPhaseDecay) {
            s.comp *= std::exp(-dt / 0.1f);
            if (s.comp < 0.002f) {
                s.comp = 0.0f;
                s.phase = kPhaseNone;
            }
        }
        room[1] += s.comp;
    }
    out.crouch = s.crouch;
    out.comp = s.comp;
    out.neck = neck[1];

    // A change of rule eased away, not shown as a jump: too big to be anything but a cut, it's left to the cut.
    if (rebase) {
        float length = 0.0f;
        for (int i = 0; i < 3; ++i) {
            s.corr[i] = s.lastRoom[i] - room[i];
            length += s.corr[i] * s.corr[i];
        }
        if (length > 0.25f * 0.25f)
            s.corr[0] = s.corr[1] = s.corr[2] = 0.0f;
    }
    const float fade = std::exp(-dt / t.easeOut);
    for (int i = 0; i < 3; ++i) {
        room[i] += s.corr[i];
        s.corr[i] *= fade;
        s.lastRoom[i] = room[i];
        s.lastHead[i] = head[i];
        out.room[i] = room[i];
        out.raw[i] = raw[i];
    }
    for (int i = 0; i < 3; ++i)
        out.world[i] = room[0] * in.right[i] + room[2] * in.back[i];
    out.world[1] += room[1];
    for (int i = 0; i < 4; ++i)
        s.lastQ[i] = q[i];

    // Its place lost for good: back to the middle over a second, along the ground (the height as it was).
    if (!in.tracked && s.lostFor > t.lostHold) {
        const float ease = 1.0f - std::exp(-dt / 0.5f);
        s.neutral[0] += raw[0] * ease;
        s.neutral[2] += raw[2] * ease;
    }
}

}

// ---- The game's eye and its arm ----
// The game's camera is not over the soldier's middle: it stands 7.73 cm ahead of it, along the look yaw the game's last tick read (30 ticks a second), and it moves only at those ticks: every 2
// or 3 pictures, a whole tick's swing at once (22 mm at 500 degrees a second). The cameras go from the soldier's middle instead (headmove.cpp), so each build the arm is taken off along the very
// yaw the eye stands on in that build. That yaw is one of the look yaws the mod wrote, most often the one written the build before (575 of 620 of the eye's steps in two minutes of head shakes;
// this build's 39 times, an older one 6), and the game's own camera rows of the same build carry it. Taken off along the yaw written this build, one build too new, the cameras stood 1.9 mm off
// the soldier's middle for every 100 degrees a second the look turned, another way at each tick, and 2 to 9 cm off for a few pictures when the guard on it refused: in a head shake the near
// world twitched against the gun and the hands, and a place, once drawn, nothing can put right.
//
// ArmTell, each build: from the eye's place as the game's floats have it, the yaw of the rows as built, whether those rows are the game's own of this build (a build can come with the rows the
// mod left in that camera the build before, the eye's place this build's all the same: now and then with the headset on, every build in some launches without it), and the look yaws written
// lately, the newest first.
// "By the rows" below: the yaw written that stands nearest the game's rows, if within 2 degrees of them; else the rows' own yaw (the game moved the look itself).
// 1. The first build after a gap: by the rows if they are the game's; else the yaw written the build before.
// 2. The eye where it was: the yaw is kept. But the game's rows more than 3 degrees from it two builds running put it right, by the rows (a wrong start cannot last), and so does the look
//    standing still: the eye still for 3 builds with the last 6 yaws written all alike, the newest of them is it.
//    (The rows' own yaw is not taken here while a look is written: with the eye still there is no step to bear it out.)
// 3. The eye moved and the soldier stood (the eye less its arm did not move the build before): of the yaws written (with the game's rows, only those within 2 degrees of them), the one whose
//    swing of the arm is nearest the eye's step; of equals the one written the build before, then this build's, then older ones. None within half a millimetre (the yaw kept was a little off,
//    the soldier moved after all, or the game moved the look itself for a tick: 60 to 155 degrees, four times in 75 seconds of play, the eye 8 to 15 cm out and back): by the rows, the
//    written yaw nearest them only if what it leaves of the eye's step is what a yaw kept a little off leaves (no more than the rows' sway, and across the forward kept). Without the
//    game's rows, the eye's step itself swings the arm.
// 4. The eye moved and the soldier is moving: by the rows. Without the game's rows: in the first such build the yaw kept is still exact, so the test of 3 with the soldier's own last step
//    taken off the eye's step, another yaw taken only if it fits 1.5 mm better; after that the yaw written the build before.
// The rows' own yaw, while a look is written, is taken in 3 and 4 only if the eye's step bears it out (3: the eye went where they say, to within their sway; 4: they explain the eye's step,
// less the soldier's own, better than the yaw kept does): rows that are the mod's own and were not told apart say a yaw the eye never went to, 4 to 8 cm of hop once.
// The rows are never taken while a written yaw fits: they carry a sway of the game's own that its eye does not follow (2 degrees as a scope comes up: 1 to 3 mm of false swing), and the eye's
// step tells yaws apart that the rows cannot (a float's step is 0.12 mm at 1300 m from the world's middle). Free of the game, as Step is: tools/headmovetest.cpp runs it on a made-up game and
// over a log's "View:" lines.
namespace headmove {

constexpr int kArmYaws = 8;  // the look yaws written that ArmTell is given: the eye stands on one of the newest three in all but a few builds

// What told the yaw in a build (headmove.cpp counts them for the log).
enum ArmHow {
    kArmKept,    // nothing: the yaw of the build before
    kArmBefore,  // the look yaw written the build before
    kArmThis,    // this build's
    kArmOlder,   // one written earlier
    kArmRows,    // the game's rows alone
    kArmStep,    // the eye's own step
    kArmRest,    // the look written, standing still
    kArmStart,   // the first build after a gap
    kArmHows
};

struct ArmIn {
    float eye[2] = {};          // the game's eye along the world's x and z, as its floats
    float grain = 0.0f;         // how coarse those are (metres): a float's own step there (ArmGrain), or a log's rounding
    bool rows = false;          // the rows as built tell a yaw: their back row's level part is over 0.03 (straight up or down they tell none)
    float rowsYaw = 0.0f;       // atan2(back.x, -back.z) (radians, the look yaw's sense)
    bool stale = false;         // those rows are the ones the mod left there the build before (ArmRowsStale), not the game's of this build
    int written = 0;            // how many look yaws written are given: none while no look is written
    float yaws[kArmYaws] = {};  // the newest first: [0] this build's own, [1] the one written the build before
    float arm = 0.0773f;        // metres
    bool first = false;         // the first build after a gap (no camera turned, a cutscene's camera, a long wait)
};

struct Arm {
    bool have = false;
    float yaw = 0.0f;       // the yaw the eye stands on (radians)
    float eye[2] = {};      // the eye in the build before
    double anchor[2] = {};  // the eye less its arm in the build before: the soldier's middle
    float step[2] = {};     // how far that had moved from the build before it
    int still = 0;          // builds the eye has stood where it was
    int away = 0;           // builds running the game's rows have stood more than 3 degrees from the yaw
    int staleRun = 0;       // builds running without the game's own rows
    // The last build, for the log:
    ArmHow how = kArmStart;
    bool moved = false;    // the eye moved
    float eyeStep = 0.0f;  // by this (metres)
    float sway = 0.0f;     // how far the game's rows stood from the yaw told (radians; 0 without them)
};

inline float ArmWrap(float radians) {
    return std::remainder(radians, 6.2831853f);
}

// A float's own step at the eye's place: its x and z come no finer than this.
inline float ArmGrain(float x, float z) {
    const float most = std::max(std::fabs(x), std::fabs(z));
    return std::nextafter(most, 3.0e38f) - most;
}

// Whether a camera's rows as built (nine numbers: right, up, back) are the ones the mod left in it at the last build: each within 2e-4 of the other's (how a camera's rows are told alike
// elsewhere: 0.01 degrees), or the yaw of their back rows alone the same to 0.02 degrees, which is all the logs show of such builds (the camera's yaw as built is the last picture's shown yaw
// to the log's 0.01). The game's own rows taken for the mod's (a head looking exactly along the gun, both still) cost nothing: the written yaws tell the arm then.
inline bool ArmRowsStale(const float* built, const float* left) {
    bool alike = true;
    for (int i = 0; i < 9 && alike; ++i)
        alike = std::fabs(built[i] - left[i]) < 2e-4f;
    if (alike)
        return true;
    const float level = std::sqrt(built[6] * built[6] + built[8] * built[8]), leftLevel = std::sqrt(left[6] * left[6] + left[8] * left[8]);
    return level > 0.03f && leftLevel > 0.03f && std::fabs(ArmWrap(std::atan2(built[6], -built[8]) - std::atan2(left[6], -left[8]))) < 0.02f * 0.01745329f;
}

// forward: the arm's level forward in the world, x and z, for this build. rule 1: the game's rows alone at each of the eye's steps (the yaw written the build before in a build without them),
// to compare; 2: the whole rule.
inline void ArmTell(Arm& a, const ArmIn& in, float* forward, int rule = 2) {
    constexpr float kDeg = 0.01745329f;
    constexpr float kSway = 2.0f * kDeg;  // the game's rows stand this near the yaw the eye is on
    // The limits are for a float's step at 1300 m (0.12 mm: the eye's step is known to 0.17 mm); coarser places (further out, a log's millimetres) widen them alike.
    const float coarse = std::max(1.0f, in.grain / 1.2207e-4f);
    const float stands = 0.0003f * coarse;  // the soldier's middle moved less than this the build before: he stands
    const float fits = 0.0005f * coarse;    // a yaw whose swing leaves less than this of the eye's step is the one
    const float same = 0.00005f * coarse;   // two that leave this little between them are one, as far as the step can tell
    const float A = in.arm, R = in.rowsYaw;
    const bool fresh = in.rows && !in.stale;
    a.staleRun = fresh ? 0 : a.staleRun + 1;
    float mx = in.eye[0] - a.eye[0], mz = in.eye[1] - a.eye[1];
    const bool first = in.first || !a.have || mx * mx + mz * mz > 1.0f;  // (a metre in a build: the soldier was put somewhere else)
    if (first)
        mx = mz = 0.0f;
    const bool moved = mx != 0.0f || mz != 0.0f;
    const float fx = -std::sin(a.yaw), fz = std::cos(a.yaw);  // the forward kept
    float yaw = a.yaw;
    ArmHow how = kArmKept;
    const auto written = [&](int k) {
        yaw = in.yaws[k];
        how = k == 1 ? kArmBefore : k == 0 ? kArmThis : kArmOlder;
    };
    // The yaw written the build before: the likeliest, with nothing to tell by.
    const auto before = [&] {
        if (in.written > 0)
            written(in.written > 1 ? 1 : 0);
    };
    // The yaw written that stands nearest the game's rows, if within their sway (which one it is; -1 none); else the rows' own (the game moved the look itself).
    const auto nearRows = [&] {
        int best = -1;
        float least = 1e9f;
        for (int k = 0; k < in.written; ++k)
            if (const float apart = std::fabs(ArmWrap(in.yaws[k] - R)); apart < least) {
                least = apart;
                best = k;
            }
        return least <= kSway ? best : -1;
    };
    const auto byRows = [&] {
        if (const int k = nearRows(); k >= 0) {
            written(k);
        } else {
            yaw = R;
            how = kArmRows;
        }
    };
    // How much of a step a yaw's swing of the arm leaves.
    const auto leaves = [&](float c, float sx, float sz) {
        const float rx = sx - A * (-std::sin(c) - fx), rz = sz - A * (std::cos(c) - fz);
        return std::sqrt(rx * rx + rz * rz);
    };
    if (first) {
        if (fresh && rule != 1)
            byRows();
        else if (fresh || (in.rows && in.written == 0))
            yaw = R;
        else
            before();
        how = kArmStart;
        a.still = a.away = 0;
    } else if (rule == 1) {
        if (moved) {
            if (fresh) {
                yaw = R;
                how = kArmRows;
            } else {
                before();
            }
        }
    } else if (moved) {
        a.still = 0;
        if (std::sqrt(a.step[0] * a.step[0] + a.step[1] * a.step[1]) < stands) {
            int best = -1, rank = 99;
            float least = 1e9f;
            for (int k = 0; k < in.written; ++k) {
                if (fresh && std::fabs(ArmWrap(in.yaws[k] - R)) > kSway)
                    continue;
                const float left = leaves(in.yaws[k], mx, mz);
                const int order = k == 1 ? 0 : k == 0 ? 1 : k;  // the build before first, then this build's, then older
                if (left < least - same || (left < least + same && order < rank)) {
                    least = left;
                    best = k;
                    rank = order;
                }
            }
            if (best >= 0 && least <= fits) {
                written(best);
            } else {
                // None fits. With the game's rows it is one of two things. The yaw kept was a little off (it came from the rows, their sway in it): the written yaw nearest the rows is the
                // one then, and what it leaves of the eye's step is the kept yaw's error, no more than the rows' sway and lying across the forward kept. Or the game moved the look itself:
                // the rows' own yaw, if the eye went where they say to within their sway (while a look is written: rows that are the mod's own and were not told apart say a yaw the eye
                // never went to). Neither, or no rows of the game's: the eye's own step swings the arm.
                bool told = false;
                if (fresh) {
                    if (const int k = nearRows(); k >= 0) {
                        const float c = in.yaws[k];
                        const float rx = mx - A * (-std::sin(c) - fx), rz = mz - A * (std::cos(c) - fz), left = std::sqrt(rx * rx + rz * rz);
                        if (left <= fits + A * kSway && std::fabs(rx * fx + rz * fz) <= 0.5f * left) {
                            written(k);
                            told = true;
                        }
                    }
                    if (!told && (in.written == 0 || leaves(R, mx, mz) <= fits + A * kSway)) {
                        yaw = R;
                        how = kArmRows;
                        told = true;
                    }
                }
                if (!told) {
                    const float by = std::max(A, 1e-4f);
                    const float nx = fx + mx / by, nz = fz + mz / by, length = std::sqrt(nx * nx + nz * nz);
                    if (length > 0.7f && length < 1.3f) {  // (else not a swing at all: kept)
                        yaw = std::atan2(-nx, nz);
                        how = kArmStep;
                    }
                }
            }
        } else if (fresh) {
            byRows();
            // The rows alone, while a look is written: only if they explain the eye's step, less the soldier's own as it was going, better than the yaw kept does.
            if (how == kArmRows && in.written > 0) {
                const float sx = mx - a.step[0], sz = mz - a.step[1];
                if (!(leaves(R, sx, sz) < std::sqrt(sx * sx + sz * sz))) {
                    yaw = a.yaw;
                    how = kArmKept;
                }
            }
        } else if (a.staleRun == 1) {
            const float sx = mx - a.step[0], sz = mz - a.step[1];
            int best = -1;
            float least = std::sqrt(sx * sx + sz * sz);  // (what keeping the yaw leaves)
            for (int k = 0; k < in.written && k < 4; ++k)
                if (const float left = leaves(in.yaws[k], sx, sz); left < least - 0.0015f) {
                    least = left;
                    best = k;
                }
            if (best >= 0)
                written(best);
        } else {
            before();
        }
    } else {
        ++a.still;
        if (fresh && std::fabs(ArmWrap(R - yaw)) > kSway + kDeg) {
            if (++a.away >= 2) {
                if (const int k = nearRows(); k >= 0) {
                    written(k);
                } else if (in.written == 0) {  // (while a look is written the rows' own yaw waits for the eye's step to bear it out: they sway and wobble with the eye standing still)
                    yaw = R;
                    how = kArmRows;
                }
            }
        } else {
            a.away = 0;
        }
        if (a.still >= 3 && in.written >= 6) {
            float low = 0.0f, high = 0.0f;
            for (int k = 1; k < 6; ++k) {
                const float apart = ArmWrap(in.yaws[k] - in.yaws[0]);
                low = std::min(low, apart);
                high = std::max(high, apart);
            }
            if (high - low < 0.02f * kDeg && (!fresh || std::fabs(ArmWrap(in.yaws[0] - R)) <= kSway)) {
                yaw = in.yaws[0];
                how = kArmRest;
            }
        }
    }
    if (!std::isfinite(yaw)) {  // (a yaw that is no number is never the eye's)
        yaw = std::isfinite(a.yaw) ? a.yaw : 0.0f;
        how = kArmKept;
    }
    a.yaw = yaw;
    forward[0] = -std::sin(yaw);
    forward[1] = std::cos(yaw);
    const double anchor[2] = {in.eye[0] - static_cast<double>(A) * forward[0], in.eye[1] - static_cast<double>(A) * forward[1]};
    for (int i = 0; i < 2; ++i) {
        a.step[i] = first ? 0.0f : static_cast<float>(anchor[i] - a.anchor[i]);
        a.anchor[i] = anchor[i];
        a.eye[i] = in.eye[i];
    }
    a.have = true;
    a.how = how;
    a.moved = moved;
    a.eyeStep = std::sqrt(mx * mx + mz * mz);
    a.sway = fresh ? std::fabs(ArmWrap(R - yaw)) : 0.0f;
}

// The forward as it was guessed before the rule ("armby 0"), kept as it ran: the look yaw written this build, or the rows' without one, taken only from a build where the eye moved as far as
// that says it swung (half of it, for the rounding at 1500 m), or after a quarter of a second of refusals. The game's rows can change every frame while its place steps at its ticks (2.4 mm of
// false arm when it draws the newest build), and a build can come with the mod's last turned camera in its rows (a yaw 30 degrees off for a frame, the eye where it was: the view hopped 4-8 cm);
// refused for long, it isn't the eye waiting for its tick: the forward kept is the wrong one (a launch started it 123 degrees off: every swing since came to too little to correct it, the arm
// a fixed 14 cm off and none of the eye's swing taken out).
struct ArmGuess {
    float eyeWas[3] = {};
    float forward[2] = {0.0f, 1.0f};
    float refusedFor = 0.0f;
};

inline void ArmGuessStep(ArmGuess& g, const float* eye, const float* back, bool byLook, float lookYaw, float arm, float share, float dt) {
    const float movedX = eye[0] - g.eyeWas[0], movedZ = eye[2] - g.eyeWas[2];
    for (int i = 0; i < 3; ++i)
        g.eyeWas[i] = eye[i];
    const float level = std::sqrt(back[0] * back[0] + back[2] * back[2]);
    if (byLook || level > 0.03f) {  // (straight up or down the rows have no way of telling; the game keeps the look from there)
        const float fx = byLook ? -std::sin(lookYaw) : -back[0] / level, fz = byLook ? std::cos(lookYaw) : -back[2] / level;
        const float dx = fx - g.forward[0], dz = fz - g.forward[1];
        const float swing = arm * std::sqrt(dx * dx + dz * dz);
        g.refusedFor += std::min(dt, 0.1f);
        if (share == 0.0f || std::sqrt(movedX * movedX + movedZ * movedZ) >= 0.5f * swing || g.refusedFor > 0.25f) {
            g.forward[0] = fx;
            g.forward[1] = fz;
            g.refusedFor = 0.0f;
        }
    }
}

}

#ifndef HEADMOVE_CORE_ONLY
// ---- The game's side (headmove.cpp) ----
namespace headmove {

// This view build's move of the cameras, world metres: from the frame's turn (camera.h FrameBody) and the game camera's
// place and back row, as the game made them; rowsStale: that camera's rows are the ones the mod left in it at the last build (ArmRowsStale), not the game's own of this one. False: nothing to
// move them by (off, no view turn, a camera that wasn't turned). The view builder's thread.
bool Build(const float* gameEye, const float* gameBack, bool rowsStale, float* world);
void SetEyeArm(float metres);  // weapon.txt "eyearm": 0 leaves the game's eye swinging
// weapon.txt "armby": along which yaw the eye's arm is taken off. 0: as it was, the look yaw written this build behind its guard (ArmGuessStep); 1: the game's rows at each of the eye's steps;
// 2: the rule (ArmTell). Whichever is used, all three are worked out, and the log's "Arm:" line, once a second, has how rough the eye less its arm was by each.
void SetArmBy(int rule);

// F9 or the right stick's hold: the room's origin was put at the head (xr_runtime.cpp Recenter).
void NoteRecentered();

// The player's own left stick moving the soldier (input.cpp, each frame): it wins over walking under the head.
void NoteStick(bool moving);

// The virtual stick for walking under the head (x right, y forward of the way the game walks), and whether the body is
// holding the game's crouch. Any thread.
bool FollowStick(float& x, float& y);
bool CrouchWanted();

// [head] in bf3vr.ini, and weapon.txt: "sixdof 0/1", "sixwalk 0/1", "sixcrouch 0/1"; "headtest x y z" (a head that far from
// its place when the line was read, room metres, instead of the real one; "headtest off"), "sixlog N" (N seconds of what
// it does, 10 lines a second), "movetest L S [degrees]" (the stick L of whole for S seconds, degrees to the right of
// straight ahead of the way the game walks: how fast it walks). Parsed in skin.cpp's reader; any thread.
void SetOn(bool on);
void SetWalk(bool on);
void SetCrouch(bool on);
void SetLean(float free, float most);  // weapon.txt "sixlean 0.25 0.32"
void SetHeadTest(const float* place);  // nullptr: off
bool TestOn();
void LogFor(int seconds);
void StartMoveTest(float length, float seconds, float degrees);

}
#endif

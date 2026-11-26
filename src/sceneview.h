#pragma once
// A scripted scene's straight ahead put where the head faces as the scene begins (camera.cpp's TurnToHead, xr_runtime.cpp's picture body).
//
// The owner, 2026-11-26 ~03:25, Operation Swordbreaker, the door into the laundry room: "When I pressed the grip button to open the door, which it
// did work ... the entire view of him opening that door in that like animated cinematic thing turned just like completely the other way, meaning I
// didn't see in front of me anymore. I saw like to my left ... There's no reason why the player's head should spin backwards when that's happening."
//
// Why: a scripted camera (a cutscene's or a ride's, its rows' w lanes filled: viewturn.h) is turned to the head about its own yaw, and its forward
// was shown at the room's straight ahead, wherever the head was. In play the room's straight ahead shows nothing: the game's camera has the head's
// turn in it, so the world stays put however far round the player sits or turns, and after an hour of play it can be anywhere. The owner's was 85
// degrees off: the breach's scripted camera came at 03:25:03.536 (its yaw -80.0), and his own recenter 4 s after it said "Recentered (yaw 85 deg)",
// the first since 01:56:32's "yaw 0": the breach played 85 degrees to one side of his face, and he saw the side of it.
//
// Now a scene's first scripted camera takes the head's yaw as the scene's straight ahead (LatchYaw), for that camera and every one after it in the
// scene: the scene's forward is shown where the head faced as it began, from its first picture on, and the head looks about it from there. Only the
// yaw: the head's pitch, roll and place go into the scene as before (the horizon level, the 6DoF head's own small moves). It's a cut, and it comes at
// the game's own cut into the scene (the first picture drawn from its camera), never a turn of the world in front of the eyes: a first-person scene
// that starts from the player's own view (the breach, a pick-up) carries on from the last picture of play with nothing moving at all. The yaw is taken
// out of the scripted camera as the hand's is out of the player's (viewturn::Turn's taken), so the picture body (xr_runtime.cpp) holds it: when the
// scene ends the player's camera comes back where the scene was shown and the look goes on from there, no jump of the world, the scene's straight
// ahead kept as a recenter keeps its own.
//
// A scene (StepScenes): pictures of a cutscene's camera, unturned or scripted (xr_runtime.cpp's g_cutscene), with less than kPlayMs of anything else
// between them. One latch a scene, however its cameras cut, roll or hand over: a ride's whole drive (Comrades' opening drive is one scripted run of
// 100 s; the 16 pictures of the player's camera before its cut into the car are 0.22 s), a death's camera with the load after it, the interrogation's
// retries (a load between them). So it never comes in play, nor in the game's own turns of the player's camera (a quick-time event's scripted look:
// the player's camera, never a scripted one, aimed by the hand as ever), nor again inside a ride while the head looks about. A recenter (the stick's,
// F9, the VR runtime's own) starts a new scene: the next scripted camera's straight ahead is where the head faces then, in the recentered room.
// weapon.txt "scenefacing 0": the scene's forward at the room's straight ahead, as before.
//
// Header only and free of the game: tools/sceneviewtest.cpp runs it, with viewturn.h's turn, on the owner's breach and the rest.

#include <cmath>

namespace sceneview {

constexpr unsigned long long kPlayMs = 1000;  // this long without a cutscene's picture: the next scripted camera begins a new scene

// The yaw (radians, turning left positive, as xr_runtime.cpp's Recenter reads one) of where an orientation (x, y, z, w; OpenXR's axes: x right, y
// up, -z forward) faces along the ground: its forward's own way along the ground, whatever its pitch and roll (a yaw, then a pitch, then a roll,
// gives that yaw back). Its forward straight up or down: the way its top points instead, taken the same way. Never NaN (a quaternion that isn't one:
// 0).
inline float HeadYaw(const float* q) {
    const float x = q[0], y = q[1], z = q[2], w = q[3];
    if (!(std::isfinite(x) && std::isfinite(y) && std::isfinite(z) && std::isfinite(w)))
        return 0.0f;
    float sx = 2.0f * (w * y + x * z), cz = 1.0f - 2.0f * (x * x + y * y);  // the back (+z) column's x and z: the forward's, the other way
    if (sx * sx + cz * cz < 1e-4f) {
        // Straight up or down: the way the top (+y) points along the ground, the other way round looking up.
        const float topX = 2.0f * (x * y - w * z), topZ = 2.0f * (y * z + w * x);
        const float forwardY = -2.0f * (y * z - w * x);
        const float away = forwardY < 0.0f ? -1.0f : 1.0f;  // looking down the top points forward
        sx = away * topX, cz = away * topZ;
        if (sx * sx + cz * cz < 1e-8f)
            return 0.0f;
    }
    return std::atan2(sx, cz);
}

// The scenes as xr_runtime.cpp counts them, once a frame: its number goes up once play (no cutscene's picture) has lasted kPlayMs after a scene, so the
// next scene's first scripted camera (which may come up to three pictures before the cutscene is told: g_cutscene wants three in a row) is a new one's.
struct Scenes {
    unsigned number = 1;               // the scene the next scripted camera belongs to (ViewTurn::sceneNumber)
    bool seen = false;                 // a cutscene's picture seen since the number last went up
    unsigned long long lastSceneAt = 0;  // the last frame with one (ms)
};
inline unsigned StepScenes(Scenes& s, unsigned long long nowMs, bool cutscene) {
    if (cutscene) {
        s.seen = true;
        s.lastSceneAt = nowMs;
    } else if (s.seen && (nowMs < s.lastSceneAt || nowMs - s.lastSceneAt >= kPlayMs)) {
        ++s.number;  // (the clock gone back: taken as play since)
        s.seen = false;
    }
    return s.number;
}
// A recenter: the next scripted camera begins a new scene, in the room as recentered.
inline void NewScene(Scenes& s) {
    ++s.number;
    s.seen = false;
}

// A scene's straight ahead as the view builder keeps it (camera.cpp, its own thread): taken from the head the first scripted camera of a scene is
// turned with, and kept for every scripted camera of that scene (both eyes and their cull cameras alike, every build of it).
struct Latch {
    bool have = false;
    unsigned number = 0;
    float yaw = 0.0f;  // radians, turning left positive: the room's yaw the scene's forward is shown at
};
// The yaw for a scripted camera built with the view turn of scene `number` and head `head` (x, y, z, w); fresh: latched now (to be said).
inline float LatchYaw(Latch& l, unsigned number, const float* head, bool& fresh) {
    fresh = !l.have || l.number != number;
    if (fresh) {
        l.have = true;
        l.number = number;
        l.yaw = HeadYaw(head);
    }
    return l.yaw;
}

// The turn put on a scripted camera made level (viewturn::Turn): the head's, from the scene's straight ahead (the yaw's turn taken off it first:
// yaw(-sceneYaw) times the head, as viewturn::QuatMul multiplies). local and head x, y, z, w.
inline void SceneLocal(const float* head, float sceneYaw, float* local) {
    const float s = -std::sin(sceneYaw / 2.0f), c = std::cos(sceneYaw / 2.0f);  // (0, s, 0, c): the yaw's turn the other way
    local[0] = c * head[0] + s * head[2];
    local[1] = c * head[1] + s * head[3];
    local[2] = c * head[2] - s * head[0];
    local[3] = c * head[3] - s * head[1];
}

}  // namespace sceneview

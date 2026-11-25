#pragma once
// The VR runtime's own recenter (XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING): a long press of the Quest's Meta button, SteamVR's "Reset
// seated position", Virtual Desktop's recenter. The runtime moves its LOCAL space's origin to where the head is; the mod's own space is LOCAL
// shifted by its last recenter (xr_runtime.cpp's Recenter: the origin under the headset, facing where the head looked), so the runtime's move
// would add to the mod's own and the view jump sideways. The mod recenters itself then too, once the change has taken (the event's changeTime: the
// time from which the runtime's poses are in the moved space), as the right stick's held click and F9 do.
//
// Only LOCAL concerns the mod: VIEW never moves, and STAGE (the play area redrawn) and the others are spaces it doesn't use. Each event counts, a
// second one replacing the first's time. A change whose time never comes within kWaitMost of the frames' display times (a runtime's time that's
// off) is taken anyway at kWaitMost. A runtime that kept sending them (more than kFloodCount within kFloodMs: none does for a player's recenter)
// would recenter the view as the head turns: followed no more that run, said. Pure, header only (tools/spacechangetest.cpp).

#include <cstdint>

namespace spacechange {

constexpr int32_t kView = 1, kLocal = 2, kStage = 3;     // XrReferenceSpaceType
constexpr int64_t kWaitMost = 2000000000;                // ns: 2 s of display time at most from the first frame after the event
constexpr int kFramesMost = 180;                         // ... and at most this many frames looking (a display clock gone back)
constexpr int kFloodCount = 5;                           // more LOCAL changes than this ...
constexpr int64_t kFloodMs = 10000;                      // ... within this long: no longer followed

struct State {
    bool pending = false;
    int64_t changeAt = 0;  // the event's changeTime (0: none given)
    int64_t firstSeen = 0; // the display time of the first frame that looked after the event (0: none yet)
    int frames = 0;        // frames that looked since the event
    int count = 0;         // LOCAL changes so far
    int ignored = 0;       // other spaces' changes
    int64_t recentMs[kFloodCount] = {};  // when the last kFloodCount LOCAL changes came (the caller's clock, ms), a ring
    int recentAt = 0;
    bool flooded = false;  // too many: not followed any more
};

// An event for the mod's session: its space's type and changeTime, and the caller's clock now (ms). True: it's the mod's to follow (a recenter
// is due). The one that makes it a flood is false, and flooded is set.
inline bool Note(State& s, int32_t type, int64_t changeTime, int64_t nowMs) {
    if (type != kLocal) {
        ++s.ignored;
        return false;
    }
    if (s.flooded)
        return false;
    const int64_t fifthBefore = s.recentMs[s.recentAt];  // (the change kFloodCount before this one, when there's been that many)
    s.recentMs[s.recentAt] = nowMs;
    s.recentAt = (s.recentAt + 1) % kFloodCount;
    if (s.count >= kFloodCount && nowMs - fifthBefore < kFloodMs) {
        s.flooded = true;
        s.pending = false;
        return false;
    }
    s.pending = true;
    s.changeAt = changeTime > 0 ? changeTime : 0;
    s.firstSeen = 0;
    s.frames = 0;
    ++s.count;
    return true;
}

// Each frame, before the frame's own recenter (its predicted display time): true once, when the change has taken (or waited kWaitMost, or
// kFramesMost frames).
inline bool Due(State& s, int64_t displayTime) {
    if (!s.pending)
        return false;
    if (!s.firstSeen)
        s.firstSeen = displayTime;
    ++s.frames;
    const bool taken = !s.changeAt || displayTime >= s.changeAt;
    const bool waited = displayTime - s.firstSeen >= kWaitMost || s.frames >= kFramesMost;
    if (!taken && !waited)
        return false;
    s.pending = false;
    return true;
}

}  // namespace spacechange

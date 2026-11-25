#pragma once
// The view through a scope's frame-rate guard (the audit of 2026-11-19). The owner's own Swordbreaker play that night: the Mk11's second renderer on
// for 154 s, those seconds at 61.9 frames a second on average, 31% of them under 60 (with it off: 67.9, 12% under 60); its own 2.58 ms of the card a
// frame and 1.3-1.8 ms of the game's threads, 49-58 frames a second within a minute of 66-72 with it off. The owner's floor is 60 (steady-72), and a
// 90 Hz headset has 11.1 ms a frame. So, once a second (xr_runtime.cpp's pace, scopeview.cpp): a second the view was on in at least half its frames
// and the frames were under kFloor, or kLateMost of them came a refresh late, is over budget; kOverSeconds of them running hold the view back
// (scopeview.cpp's Wanted: the lens magnifies the eye's own picture, as "scopeview 0" does) for kHoldFirst, tried again after; held back again soon
// after (within kCalm of the last hold) it waits twice as long, kHoldMost at most. A second whose frames weren't going to the headset (paced false:
// the headset off, nobody in it) counts for nothing. Pure, header only (tools/scopegovtest.cpp).

namespace scopegov {

constexpr int kFloor = 62;          // frames a second: under it with the view on, over budget (the owner's 60, two frames above it)
constexpr int kLateMost = 8;        // frames a second a refresh late (the pace's "off by a frame"): over budget too
constexpr int kOverSeconds = 2;     // seconds over budget running before the view is held back (one heavy second alone isn't)
constexpr unsigned long long kHoldFirst = 8000, kHoldMost = 60000, kCalm = 60000;  // ms

struct State {
    bool held = false;                // held back now
    unsigned long long until = 0;     // ... till then
    unsigned long long hold = 0;      // the last hold's length (0: none yet)
    unsigned long long lastHeld = 0;  // when the last hold began (0: never)
    int over = 0;                     // seconds over budget running, the view on
};

struct Second {
    unsigned long long nowMs = 0;
    int frames = 0;      // frames this second
    int late = 0;        // ... of them a refresh late
    int drawn = 0;       // ... with the view through the scope dispatched
    bool paced = false;  // the frames went to the headset (its session showing them)
};

struct Out {
    bool heldNow = false;   // held back from this second on (said)
    bool released = false;  // its hold over: tried again (said)
    unsigned long long holdMs = 0;
    bool over = false;      // this second counted over budget
};

inline Out Step(State& s, const Second& in) {
    Out o;
    if (s.held) {
        // (A clock gone back: let go rather than held on for good.)
        if (in.nowMs >= s.until || in.nowMs + kHoldMost < s.until) {
            s.held = false;
            s.over = 0;
            o.released = true;
        }
        return o;
    }
    if (!in.paced || in.frames <= 0) {
        s.over = 0;
        return o;
    }
    const bool viewOn = in.drawn > 0 && 2 * in.drawn >= in.frames;
    o.over = viewOn && (in.frames < kFloor || in.late >= kLateMost);
    s.over = o.over ? s.over + 1 : 0;
    if (s.over < kOverSeconds)
        return o;
    const bool soon = s.lastHeld && in.nowMs >= s.lastHeld && in.nowMs - s.lastHeld < kCalm;
    s.hold = soon && s.hold ? (s.hold * 2 < kHoldMost ? s.hold * 2 : kHoldMost) : kHoldFirst;
    s.held = true;
    s.until = in.nowMs + s.hold;
    s.lastHeld = in.nowMs;
    s.over = 0;
    o.heldNow = true;
    o.holdMs = s.hold;
    return o;
}

// The lens's own tests for turning the view on (scopeview.cpp's Wanted), beside its size and power: its line within kOnSlope of the eye's (past it the
// lens shows mostly the vignette's black: most turn-ons that night were 280-440 pixels off), its radius no more than kMostRadius of the picture's
// pixels per unit of slope (half the eye's picture across, about: that night's 2860 to 37238 pixels across came as the gun swung past the face to
// a shoulder), and its glass in front of the eye (back: the glass's middle from the eye along the view's back axis, metres; under 0 in front).
// focal 0: not known (906, 100 degrees on 2160 pixels).
// (kMostRadius 1.35: half the eye's picture across at the default 106 degrees, tan 53; in front: any distance, an eyepiece pressed close too.)
constexpr float kOnSlope = 0.2f, kStaySlope = 0.38f, kMostRadius = 1.35f, kInFront = 0.0f;
inline bool LensSane(float radius, float focal, float back) {
    const float f = focal > 0.0f ? focal : 906.0f;
    return radius == radius && back == back && radius <= kMostRadius * f && back < kInFront;
}
inline bool LensOnLine(float off, float radius, float focal, bool stay) {
    const float f = focal > 0.0f ? focal : 906.0f;
    return off < (stay ? kStaySlope : kOnSlope) * f + radius;
}

}  // namespace scopegov

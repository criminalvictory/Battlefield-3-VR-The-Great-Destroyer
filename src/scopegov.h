#pragma once
// The view through a scope's frame-rate guard (the audit of 2026-11-19, settled by its review of 2026-11-25). The owner's own Swordbreaker play that
// night: the Mk11's second renderer on for 154 s, those seconds at 61.9 frames a second on average, 31% of them under 60 (with it off: 67.9, 12% under
// 60); its own 2.58 ms of the card a frame, 49-58 frames a second within a minute of 66-72 with it off. Two of the owner's rules meet here: the real
// picture through every magnified scope (scopes-real-picture: "a foundation"; the Mk11's "very nice") and 60 the floor ("60 to 70 is perfect ...
// anything below that is not good": steady-72, said of the scope's own camera). The third settles it: never trade looks for frames without asking him
// (performance-last). So the guard is HIS SWITCH: weapon.txt "scopegov 1" acts on it; "scopegov 0", the default, only watches, and says in the log, once a
// trip, what it would have done (scopeview.cpp's NotePace), so a play's log shows how often and where the view takes the frames under the floor.
//
// Once a second (xr_runtime.cpp's pace): a second the view was on in at least half its frames and its frames came under kFloor a second by the clock
// (frames over the second's own length, measured between frames: the same at 72, 80, 90, 120 or 144 Hz; rounded, so a 60 Hz display at its full rate
// isn't under) is over budget. kOverSeconds of them running hold the view back (acting: the lens magnifies the eye's own picture meanwhile, as
// "scopeview 0" does) for kHoldFirst. Held back again within kCalm of the last hold's END: twice as long, kHoldMost at most; a calm kCalm since: kHoldFirst
// again. A hold whose time is over lets go only while no magnified lens is looked through (the scope lowered: Second::looking), so the real picture comes
// back at the next raise, never mid-aim. A second whose frames weren't going to the headset (paced false: the headset off, nobody in it, a load or a
// stall) counts for nothing and breaks a run. No late-frame rule: "a refresh late" counted against 72 Hz tripped the guard at 64 frames at 72 Hz, 82 at
// 90 Hz, and every second at 144 Hz (the review's findings 1 and 2). Pure, header only (tools/scopegovtest.cpp).

namespace scopegov {

constexpr double kFloor = 60.0;     // frames a second: under it (rounded) with the view on, over budget (the owner's floor)
constexpr int kOverSeconds = 3;     // seconds over budget running before the view is held back (a heavy second or two alone isn't)
constexpr unsigned long long kHoldFirst = 10000, kHoldMost = 60000;  // ms
constexpr unsigned long long kCalm = 120000;  // ms from the last hold's end: held back again sooner, the hold doubles (over kHoldMost: never flaps)

struct State {
    bool held = false;                // held back now
    unsigned long long until = 0;     // ... at least till then
    unsigned long long hold = 0;      // the last hold's length (0: none yet)
    unsigned long long endedAt = 0;   // when the last hold let go (0: never)
    int over = 0;                     // seconds over budget running, the view on
    bool waitSaid = false;            // its time over with a lens still looked through: said once a hold
};

struct Second {
    unsigned long long nowMs = 0;
    double ms = 0.0;       // this second's own length (the time its frames took, between frames; 0: not known)
    int frames = 0;        // frames this second
    int drawn = 0;         // ... with the view through the scope dispatched
    bool paced = false;    // the frames went to the headset (its session showing them), a whole second (no load, no stall)
    bool looking = false;  // a magnified lens is looked through now (scopeview.cpp: noted lately, on its line): a hold lets go only without one
};

struct Out {
    bool heldNow = false;   // held back from this second on (said)
    bool released = false;  // its hold over: back at the next raise (said)
    bool waiting = false;   // its time over, a lens still looked through: kept till it's lowered (said once a hold)
    unsigned long long holdMs = 0;
    bool over = false;      // this second counted over budget
    double rate = 0.0;      // this second's frames a second (0: not counted)
};

// Frames a second, rounded the way the floor is read (a full 60 Hz is 60, not under it by a timer's fraction).
inline double Rate(int frames, double ms) {
    return frames > 0 && ms > 0.0 ? frames * 1000.0 / ms : 0.0;
}
inline bool UnderFloor(double rate) {
    return rate > 0.0 && rate < kFloor - 0.5;
}

inline Out Step(State& s, const Second& in) {
    Out o;
    if (s.held) {
        // (A clock gone back: its time taken as over rather than held on for good.)
        const bool timeOver = in.nowMs >= s.until || in.nowMs + kHoldMost < s.until;
        if (!timeOver)
            return o;
        if (in.looking) {
            o.waiting = !s.waitSaid;
            s.waitSaid = true;
            return o;
        }
        s.held = false;
        s.over = 0;
        s.endedAt = in.nowMs ? in.nowMs : 1;
        s.waitSaid = false;
        o.released = true;
        return o;
    }
    if (!in.paced || in.frames <= 0 || !(in.ms > 0.0)) {
        s.over = 0;
        return o;
    }
    o.rate = Rate(in.frames, in.ms);
    const bool viewOn = in.drawn > 0 && 2 * in.drawn >= in.frames;
    o.over = viewOn && UnderFloor(o.rate);
    s.over = o.over ? s.over + 1 : 0;
    if (s.over < kOverSeconds)
        return o;
    const bool soon = s.endedAt && in.nowMs >= s.endedAt && in.nowMs - s.endedAt < kCalm;
    s.hold = soon && s.hold ? (s.hold * 2 < kHoldMost ? s.hold * 2 : kHoldMost) : kHoldFirst;
    s.held = true;
    s.until = in.nowMs + s.hold;
    s.over = 0;
    s.waitSaid = false;
    o.heldNow = true;
    o.holdMs = s.hold;
    return o;
}

// The lens's own tests for the view (scopeview.cpp's Wanted), beside its size and power. On its line: within kSlope of the eye's line (a slope, in the
// picture's pixels by its focal length) plus its radius: past that its whole disc shows the vignette's black (main's rule, kept: the audit's 0.2 for
// turning on showed the old magnified copy over most of every raise before the real picture popped in, and kept it off for a cheek weld between 0.2 and
// 0.38: the review's finding 5). Sane, to turn it on at a fresh raise: its radius no more than kMostRadius of the picture's pixels per unit of slope (that
// night's 2860 to 37238 pixels across came as the gun swung past the face to a shoulder, the smallest perhaps a lean; the owner's 35 real turn-ons were
// 165-645 across) and its glass in front of the eye. Once on, only in front: an eyepiece pressed right up to the eye keeps its real picture, and gets it
// back soon after the view went off (StepView below). The Mk11's 1.9 cm eyepiece reads past the cap nearer than 1.41 cm, the PK-A's 1.6 cm one nearer
// than 1.19 (under 1 cm no lens is noted at all, scopezoom.cpp); the owner's own logs never had his eye nearer than about 4 cm, but any player may press it
// closer. back: the glass's middle from the eye along the view's back axis, metres (under 0 in front). focal 0: not known (906, 100 degrees on 2160
// pixels).
constexpr float kSlope = 0.38f, kMostRadius = 1.35f, kInFront = 0.0f;
inline float Focal(float focal) {
    return focal > 0.0f ? focal : 906.0f;
}
inline bool InFront(float back) {
    return back == back && back < kInFront;
}
inline bool LensSane(float radius, float focal, float back) {
    return radius == radius && InFront(back) && radius <= kMostRadius * Focal(focal);
}
inline bool LensOnLine(float off, float radius, float focal) {
    return off < kSlope * Focal(focal) + radius;
}

// The view's own switch (scopeview.cpp's Wanted, a dispatch at a time, the frame-rate guard aside), from the lens the last frames noted (Seen): on at the
// second dispatch running with a lens to turn on for (noted under kFresh ago, magnified past 1.5, 80 pixels or more in radius, on its line, sane); off once
// none has been noted for kLensGone (a frame that long, the scope gone), or its lens has been small (under 55 pixels), unmagnified, off its line or behind
// the eye for more than kSmallRun dispatches running. Back on soon after it went off (within kBackMs): no size cap, its lens in front of the eye and on its
// line enough, as when on. An eyepiece pressed nearer than the cap through a frame over 120 ms or a moment off its line (the Mk11's under 1.41 cm, the
// PK-A's under 1.19) gets its own picture back within two dispatches, as it did before the cap (0.1.14), not the magnified copy of the eye's picture till
// the eye backs off a few millimetres or the gun is lowered and raised again (the review of d32a39b, finding 10). The cap stays for a fresh raise.
constexpr unsigned long long kFresh = 100, kLensGone = 120, kBackMs = 1000;  // ms
constexpr int kSmallRun = 36;                                               // dispatches
struct Seen {
    unsigned long long ageMs = ~0ull;  // since the lens was noted (~0: never)
    float power = 0.0f, radius = 0.0f, off = 0.0f, focal = 0.0f;  // as scopeview::NoteLens has them: pixels in the eye's picture (focal 0: not known)
    float back = 0.0f;                 // the glass's middle from the eye along the view's back axis, metres (under 0 in front)
};
struct View {
    bool on = false;
    int ready = 0;                 // off: dispatches running with a lens to turn on for
    int faint = 0;                 // on: dispatches running with its lens small, unmagnified, off its line or behind the eye
    unsigned long long offAt = 0;  // when it last went off by itself (0: never)
};
enum class Turned { kNo, kOn, kOnPastCap, kOff };  // kOnPastCap: back on soon after it went off, its lens past the cap a fresh raise has
inline Turned StepView(View& v, const Seen& s, unsigned long long nowMs) {
    const bool onLine = LensOnLine(s.off, s.radius, s.focal);
    if (!v.on) {
        const bool sane = LensSane(s.radius, s.focal, s.back);
        const bool soon = v.offAt && nowMs >= v.offAt && nowMs - v.offAt < kBackMs;
        if (s.ageMs < kFresh && s.power > 1.5f && s.radius >= 80.0f && onLine && (sane || (soon && InFront(s.back)))) {
            if (v.ready++ >= 1) {
                v.on = true;
                v.faint = 0;
                return sane ? Turned::kOn : Turned::kOnPastCap;
            }
        } else {
            v.ready = 0;
        }
        return Turned::kNo;
    }
    v.faint = s.radius < 55.0f || s.power <= 1.5f || !onLine || !InFront(s.back) ? v.faint + 1 : 0;
    if (s.ageMs > kLensGone || v.faint > kSmallRun) {
        v.on = false;
        v.ready = 0;
        v.faint = 0;
        v.offAt = nowMs ? nowMs : 1;
        return Turned::kOff;
    }
    return Turned::kNo;
}

}  // namespace scopegov

#pragma once

// Each controller's tracking as the runtime gives it, a frame at a time (input.cpp's NoteTracking and NoteReachSqueeze), and what the reach test's
// "Tracking:" lines say (the two minutes in the headset that show where the Steam Frame stops seeing a controller reached behind the neck, over a
// shoulder or under the arm, for the holsters' places): header only, no game memory and no Windows calls, so tools/inputrulestest.cpp runs it as it
// is. The holsters read the same record through input.h (input::TrackingOf).
//
// A controller in a frame is seen (its grip's place valid and tracked, its turn valid: the cameras have it), guessed (a place that isn't tracked: the
// runtime's own estimate from its motion sensors) or gives no place. Read raw, before input's 0.5 s hold of a hand lost for a moment (SteadyHand).
// Not watched (no controllers, or the session not focused: the headset off or out of focus): the record waits with no line and starts afresh when the
// controllers are read again, so taking the headset off and on never reads as a loss.

#include <cmath>
#include <cstdint>

#include "ammozone.h"

namespace tracking {

enum class State : uint8_t { kNotWatched = 0, kSeen = 1, kGuessed = 2, kNone = 3 };

// One controller in one frame, as the runtime gave it.
struct Frame {
    double t = 0.0;               // seconds (the frame's own time)
    bool watched = false;         // the controllers read this frame
    bool positionValid = false, positionTracked = false, orientationValid = false;  // its grip's flags (all false when it wasn't located)
    float place[3] = {};          // the grip's place as the runtime gave it (the room: input::HandPose's space), when positionValid
    bool sensed = false;          // its own speed given with it
    float speed[3] = {};          // ... (the room, m/s)
};

// The record of one controller, every frame: its state, where and when it was last seen and its speed then, the runtime's last guess.
struct Record {
    State state = State::kNotWatched;  // this frame's
    double stateSince = 0.0;           // when this state began (s)
    bool everWatched = false;          // read at some frame since the game began
    double watchSince = 0.0;           // when this watch began (s)
    bool haveSeen = false;             // seen since this watch began
    float seenAt[3] = {};              // where it was last seen (the room)
    double seenTime = 0.0;             // when (s)
    bool speedKnown = false;           // its speed then known: by its own sensors (speedSensed), else the step between its last two seen places
    bool speedSensed = false;
    float seenSpeed[3] = {};           // (the room, m/s)
    bool haveGuess = false;            // a guess of the runtime's since this watch began: this frame's while guessed, else the last one
    float guessAt[3] = {};
    double guessTime = 0.0;
    // The lines' own: when the last kChangesSaid of lines 2-5 were said, the changes not said since (line 7), when the last kSqueezesSaid squeezes were
    // said and how many weren't since, and the grip's arming.
    double changeSaid[6] = {-1e30, -1e30, -1e30, -1e30, -1e30, -1e30};
    int changeNext = 0;
    int unsaid = 0;
    double unsaidFrom = 0.0, unsaidTo = 0.0;
    float unsaidSum[3] = {};           // their places' sum (the room), for line 7's "about"
    int unsaidPlaces = 0;
    double squeezeSaid[4] = {-1e30, -1e30, -1e30, -1e30};
    int squeezeNext = 0;
    int squeezesUnsaid = 0;
    bool armed = false;                // the grip under kArm since its last squeeze
};

constexpr int kChangesSaid = 6;          // lines 2-5: a controller's at most this many ...
constexpr double kChangesWindow = 10.0;  // ... in any this many seconds; the rest counted for line 7
constexpr double kSummaryHeld = 2.0;     // line 7 once a state has held this long
constexpr int kSqueezesSaid = 4;         // line 6: a controller's at most this many ...
constexpr double kSqueezesWindow = 1.0;  // ... in any this many seconds
constexpr float kSqueezeNear = 0.55f;    // a seen hand's squeeze is said within this of the neck point (m): not a handguard's or a magazine well's
constexpr float kArm = 0.4f, kSqueeze = 0.6f;  // the grip: armed under, a squeeze past it from there (as grasp's and the pouch's)
constexpr double kStepMost = 0.25;       // the step between two seen places gives a speed only this close apart (s)

inline bool Finite3(const float* p) {
    return std::isfinite(p[0]) && std::isfinite(p[1]) && std::isfinite(p[2]);
}

inline float Apart(const float* a, const float* b) {
    const float d[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    return std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
}

// This frame's state.
inline State StateOf(const Frame& f) {
    if (!f.watched)
        return State::kNotWatched;
    if (!f.positionValid || !Finite3(f.place))
        return State::kNone;
    return f.positionTracked && f.orientationValid ? State::kSeen : State::kGuessed;
}

enum class Change : uint8_t {
    kNone = 0,
    kLost = 1,          // line 2: seen, now not (Out::guessed: the runtime guesses its place; else none)
    kGuessStopped = 2,  // line 3: guessed, now no place
    kGuessedAgain = 3,  // line 4: no place, now guessed
    kSeenAgain = 4,     // line 5: not seen, now seen
};

struct Out {
    bool watchedNow = false;        // the first frame read after none: line 1 (one line for both controllers)
    bool firstWatch = false;        // ... the first since the game began ("watching the controllers", not "again")
    Change change = Change::kNone;  // a change of state, said (lines 2-5); one over the cap is counted for line 7 instead
    bool guessed = false;           // line 2: the runtime guesses its place now (else it gives none)
    double after = 0.0;             // line 3: guessed for (s); line 4: no place for; line 5: not seen for
    bool guessBefore = false;       // line 5: a guess the frame before it was seen ...
    float guessOff = 0.0f;          // ... that far from the place seen (m); line 3: the last guess from the last seen place (when haveSeen)
    bool summary = false;           // line 7: the changes not said (summaryCount in summarySpan s, about summaryAt in the room)
    bool summaryUnwatched = false;  // ... said as the watch ended
    int summaryCount = 0;
    double summarySpan = 0.0;
    float summaryAt[3] = {};
};

namespace detail {

// One more of a line kind said at t within the cap (the times of the last `most` said, oldest overwritten): false when `most` were said within window.
inline bool Take(double* said, int most, int& next, double window, double t) {
    int recent = 0;
    for (int k = 0; k < most; ++k)
        recent += t - said[k] < window ? 1 : 0;
    if (recent >= most)
        return false;
    said[next] = t;
    next = (next + 1) % most;
    return true;
}

inline void Seen(Record& r, const Frame& f, bool seenBefore) {
    const double dt = f.t - r.seenTime;
    if (f.sensed && Finite3(f.speed)) {
        for (int i = 0; i < 3; ++i)
            r.seenSpeed[i] = f.speed[i];
        r.speedKnown = true, r.speedSensed = true;
    } else if (seenBefore && r.haveSeen && dt > 0.0 && dt <= kStepMost) {
        for (int i = 0; i < 3; ++i)
            r.seenSpeed[i] = static_cast<float>((f.place[i] - r.seenAt[i]) / dt);
        r.speedKnown = Finite3(r.seenSpeed), r.speedSensed = false;
    } else {
        r.speedKnown = false, r.speedSensed = false;
    }
    for (int i = 0; i < 3; ++i)
        r.seenAt[i] = f.place[i];
    r.seenTime = f.t;
    r.haveSeen = true;
}

inline void Guess(Record& r, const Frame& f) {
    for (int i = 0; i < 3; ++i)
        r.guessAt[i] = f.place[i];
    r.guessTime = f.t;
    r.haveGuess = true;
}

inline void Summary(Record& r, Out& out) {
    out.summary = true;
    out.summaryCount = r.unsaid;
    out.summarySpan = r.unsaidTo - r.unsaidFrom;
    for (int i = 0; i < 3; ++i)
        out.summaryAt[i] = r.unsaidPlaces > 0 ? r.unsaidSum[i] / static_cast<float>(r.unsaidPlaces) : 0.0f;
    r.unsaid = 0;
    r.unsaidPlaces = 0;
    r.unsaidSum[0] = r.unsaidSum[1] = r.unsaidSum[2] = 0.0f;
}

}  // namespace detail

// One frame of one controller: its record updated, and what the lines say of it.
inline Out Step(Record& r, const Frame& f) {
    Out out;
    const State now = StateOf(f);
    const State was = r.state;
    if (now == State::kNotWatched) {
        if (was != State::kNotWatched) {
            r.state = State::kNotWatched;
            r.stateSince = f.t;
            r.armed = false;  // (a grip held as it comes back is no squeeze)
            if (r.unsaid > 0) {
                detail::Summary(r, out);
                out.summaryUnwatched = true;
            }
        }
        return out;
    }
    if (was == State::kNotWatched) {
        // A watch begins: its first state only (line 1), its records afresh.
        out.watchedNow = true;
        out.firstWatch = !r.everWatched;
        r.everWatched = true;
        r.watchSince = f.t;
        r.haveSeen = r.haveGuess = r.speedKnown = r.speedSensed = false;
        r.unsaid = r.unsaidPlaces = 0;
        r.unsaidSum[0] = r.unsaidSum[1] = r.unsaidSum[2] = 0.0f;
        r.state = now;
        r.stateSince = f.t;
        if (now == State::kSeen)
            detail::Seen(r, f, false);
        else if (now == State::kGuessed)
            detail::Guess(r, f);
        return out;
    }
    if (now == was) {
        if (now == State::kSeen)
            detail::Seen(r, f, true);
        else if (now == State::kGuessed)
            detail::Guess(r, f);
    } else {
        Change change = Change::kNone;
        float at[3] = {};
        if (was == State::kSeen) {
            change = Change::kLost;
            out.guessed = now == State::kGuessed;
            for (int i = 0; i < 3; ++i)
                at[i] = r.seenAt[i];
            if (now == State::kGuessed)
                detail::Guess(r, f);
        } else if (now == State::kNone) {  // (was guessed)
            change = Change::kGuessStopped;
            out.after = f.t - r.stateSince;
            out.guessOff = r.haveSeen ? tracking::Apart(r.guessAt, r.seenAt) : 0.0f;
            for (int i = 0; i < 3; ++i)
                at[i] = r.guessAt[i];
        } else if (now == State::kGuessed) {  // (was none)
            change = Change::kGuessedAgain;
            out.after = f.t - r.stateSince;
            detail::Guess(r, f);
            for (int i = 0; i < 3; ++i)
                at[i] = f.place[i];
        } else {  // (seen again, from guessed or none)
            change = Change::kSeenAgain;
            out.after = f.t - (r.haveSeen ? r.seenTime : r.watchSince);
            out.guessBefore = was == State::kGuessed && r.haveGuess;
            out.guessOff = out.guessBefore ? tracking::Apart(r.guessAt, f.place) : 0.0f;
            detail::Seen(r, f, false);
            for (int i = 0; i < 3; ++i)
                at[i] = f.place[i];
        }
        r.state = now;
        r.stateSince = f.t;
        if (detail::Take(r.changeSaid, kChangesSaid, r.changeNext, kChangesWindow, f.t)) {
            out.change = change;
        } else {
            if (r.unsaid == 0)
                r.unsaidFrom = f.t;
            r.unsaidTo = f.t;
            ++r.unsaid;
            if (Finite3(at)) {
                for (int i = 0; i < 3; ++i)
                    r.unsaidSum[i] += at[i];
                ++r.unsaidPlaces;
            }
        }
    }
    if (r.unsaid > 0 && out.change == Change::kNone && f.t - r.stateSince >= kSummaryHeld)
        detail::Summary(r, out);
    return out;
}

struct SqueezeOut {
    bool squeezed = false;  // the grip crossed kSqueeze from under kArm this frame
    bool say = false;       // line 6 said: within its cap, and the hand not seen or seen within kSqueezeNear of the neck point
    int unsaidBefore = 0;   // squeezes over the cap since the last one said
};
// The grip this frame (read: the session focused and its action active, whether or not the controller is seen), after Step: its squeeze, and whether
// line 6 says it (body: the neck point known).
inline SqueezeOut Squeeze(Record& r, double t, bool read, float grip, bool body, const float* neck) {
    SqueezeOut out;
    if (!read || !std::isfinite(grip) || r.state == State::kNotWatched) {
        r.armed = false;  // (unread: a grip held as it's read again is no squeeze)
        return out;
    }
    if (grip < kArm) {
        r.armed = true;
        return out;
    }
    if (grip <= kSqueeze || !r.armed)
        return out;
    r.armed = false;
    out.squeezed = true;
    const bool sayable = r.state != State::kSeen || (body && Finite3(neck) && tracking::Apart(r.seenAt, neck) <= kSqueezeNear);
    if (!sayable)
        return out;
    if (detail::Take(r.squeezeSaid, kSqueezesSaid, r.squeezeNext, kSqueezesWindow, t)) {
        out.say = true;
        out.unsaidBefore = r.squeezesUnsaid;
        r.squeezesUnsaid = 0;
    } else {
        ++r.squeezesUnsaid;
    }
    return out;
}

// ---- The holsters' places, as first measured: from the neck point (pouch.cpp: 7.5 cm below and 8 cm behind the eyes) in the body's axes, right, up, forward
// (the body's heading as the pouch keeps it, pouch::BodyFrame); each an egg, its Size 1 at its edge (ammozone::Size's way). ----
struct Place {
    const char* at;   // its word inside it
    const char* by;   // ... near it
    float middle[3];  // right, up, forward (m)
    float half[3];    // its half-sizes
};
inline constexpr Place kPlaces[] = {
    {"at the back of the neck", "by the back of the neck", {0.0f, -0.06f, -0.11f}, {0.13f, 0.10f, 0.07f}},
    {"at the right shoulder", "by the right shoulder", {0.17f, -0.09f, 0.04f}, {0.08f, 0.08f, 0.09f}},
    {"at the left shoulder", "by the left shoulder", {-0.17f, -0.09f, 0.04f}, {0.08f, 0.08f, 0.09f}},
    {"at the left rib cage", "by the left rib cage", {-0.19f, -0.30f, -0.03f}, {0.06f, 0.09f, 0.07f}},
    {"at the left chest", "by the left chest", {-0.08f, -0.18f, 0.12f}, {0.06f, 0.05f, 0.09f}},
    {"at the right chest", "by the right chest", {0.08f, -0.18f, 0.12f}, {0.06f, 0.05f, 0.09f}},
    {"at the belly", "by the belly", {ammozone::kBelly[0], ammozone::kBelly[1], ammozone::kBelly[2]},
     {ammozone::kBellyRadii[0], ammozone::kBellyRadii[1], ammozone::kBellyRadii[2]}},
};
constexpr int kPlaceCount = static_cast<int>(sizeof(kPlaces) / sizeof(kPlaces[0]));

inline float SizeIn(const float* fromNeck, const Place& p) {
    float sum = 0.0f;
    for (int i = 0; i < 3; ++i) {
        const float d = (fromNeck[i] - p.middle[i]) / p.half[i];
        sum += d * d;
    }
    return std::sqrt(sum);
}

// The word for a place from the neck point (body axes): inside a place ("at the back of the neck"; the smaller Size first), else the nearest within
// 1.5 ("by the ..."), else "in front" (over 0.25 forward), "low" (over 0.50 down) or "elsewhere".
inline const char* Word(const float* fromNeck) {
    if (!Finite3(fromNeck))
        return "elsewhere";
    int best = -1;
    float bestSize = 0.0f;
    for (int k = 0; k < kPlaceCount; ++k) {
        const float s = SizeIn(fromNeck, kPlaces[k]);
        if (best < 0 || s < bestSize)
            best = k, bestSize = s;
    }
    if (bestSize <= 1.0f)
        return kPlaces[best].at;
    if (bestSize <= 1.5f)
        return kPlaces[best].by;
    if (fromNeck[2] > 0.25f)
        return "in front";
    if (fromNeck[1] < -0.50f)
        return "low";
    return "elsewhere";
}

// A direction in the room in the body's axes (right (cos, 0, -sin), up, forward (-sin, 0, -cos): pouch.cpp's), and a place from the neck point.
inline void InBody(const float* room, float yaw, float* out) {
    const float c = std::cos(yaw), s = std::sin(yaw);
    out[0] = room[0] * c - room[2] * s;
    out[1] = room[1];
    out[2] = -room[0] * s - room[2] * c;
}
inline void FromNeck(const float* room, const float* neck, float yaw, float* out) {
    const float d[3] = {room[0] - neck[0], room[1] - neck[1], room[2] - neck[2]};
    InBody(d, yaw, out);
}

}  // namespace tracking

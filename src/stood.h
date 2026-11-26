#pragma once
// The floor the soldier stands on where the game's ray cast sees none. The owner in the headset, Operation Swordbreaker's overpass, 2026-11-26: "when I
// was reloading that gun [the M249] ... if I dropped like the box ammo ... it falls through the floor, so there's no like collision from the floor and
// the ammo box. It just falls right through the pavement."
//
// What his log showed (build\bf3vr.log, 03:28:30-03:30:48, the overpass round x -611..-608, z -2090..-2083): the game's ray cast (ground.cpp, flags
// 0xB5, the client's world) sees nothing under the soldier there. The ray straight down from his own eye, cast twice a second, found nothing within 3 m
// in 70 s of standing and crouching on that deck (tallies of 1 hit in 21, 0 in 20), where every other place that session (the hotel's roof, the
// street, the bazaar's floors) it hit at once. So everything let go of there, the three boxes and the M249 itself alike (kind 2 and the gun, the same
// path), lay a moment on the feet's guess (not firm: the soldier's eye less his stance's height), until its own far ray, 50 m down, found the embankment
// 7 m under the deck (firm): then it fell to it and slid 14 m down to y 57 (#17, #18, #19, and the M249 at -599.85 56.97 -2094.81, 22 m below his hand).
//
// What stands in: where the soldier stands steady (his eye level for half a second, on foot in his body's own pose as the game has it, no faster than a
// run: his feet are his eye less that pose's eye height, never a belief's) and the ray down from his eye finds no floor at his feet, eight rays 0.6 m
// round him look for one: at most one of them finding a floor at his feet, the floor round him is one the rays don't see, and the place he stands is
// kept (a print: his feet's height there). A print answers for what's let go of within 1.5 m of it (the floor nearest under it: the highest, and of that
// floor's prints the nearest), as firm as a ray, wherever the rays find nothing or a floor more than 5 cm lower than it; a floor they find at its height
// or above (a crate on the deck) is theirs. Where the rays do see his floor (a roof, a street, the train) nothing is kept, and nothing changes: a
// magazine let go of past a roof's edge falls to the street as before (the eight rays find the roof at his feet). While the verdict is fresh (a
// second), the places he walks to are kept too.
// Its known limit: over an unseen floor's own edge (the overpass's railing) a print reaches 1.5 m past it, so a box let go of over the railing there lies
// on the air beside it, rather than every box going through the deck.
//
// Pure and header only: ground.cpp keeps one Watch under its lock and plumbs the game's rays to it; tools/loosetest.cpp drives it with rays of its own.

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace stood {

constexpr float kEyeHeight[3] = {1.6f, 1.0f, 0.3f};   // the eye above the feet standing, crouched, prone (ground.cpp's, the game's own)
constexpr unsigned long long kSteadyMs = 500;        // the eye steady this long ...
constexpr unsigned long long kSampleMs = 30;         // ... seen every 30 ms at least (one kept a frame, at most one every 30 ms)
constexpr float kSteadySpan = 0.06f;                 // ... its height within 6 cm, in one stance, on foot ...
constexpr float kSteadyMostSpeed = 8.0f;             // ... no faster across than a run (m/s): faster is a ride
constexpr float kFeetAbove = 0.15f, kFeetBelow = 0.5f;  // the eye's ray: a floor found this far over or under his feet is the one he stands on
constexpr int kRound = 8;                            // the rays round him
constexpr float kRoundAt = 0.6f;                     // m across from his eye
constexpr float kRoundFeet = 0.12f;                  // a round ray's floor this near his feet's height (facing up) is his floor, seen
constexpr int kRoundMost = 1;                        // at most this many of them seeing it: the floor round him is unseen
constexpr float kReach = 1.5f;                       // a print answers for points this far across from it
constexpr float kApart = 0.35f, kSame = 0.15f;       // a new print this near an old one across, and this near its height, is the old one
constexpr float kLevel = 0.05f;                      // a new print within kReach of an old one and this near its height takes its height (one floor, one height)
constexpr float kLetOff = 0.03f;                     // a point this far under a print is still on it (as the rays' own 3 cm)
constexpr float kBand = 0.25f;                       // prints this far under the highest one under a point are the same floor's (a step, a slope)
constexpr float kOver = 0.05f;                       // the rays' floor this much lower than a print: the print stands in
constexpr int kPrints = 256;                         // kept at once (a level's: forgotten oldest first)
constexpr unsigned long long kFreshMs = 1000;        // a verdict of unseen keeps the places he walks to for this long ...
constexpr float kWalkFrom = 1.0f;                    // ... within this of where it was made, his feet within kWalkFeet of its height
constexpr float kWalkFeet = 0.1f;
constexpr unsigned long long kRefreshMs = 3000;      // the eye's ray missing again this soon after a verdict of unseen, within kResurvey of
constexpr float kResurvey = 0.35f;                   // where it was made and kLevel of its height: the same floor, no new survey
constexpr unsigned long long kSurveyMs = 1500;       // a survey's rays not all back by then: given up (the next miss asks again)

// ---------------------------------------------------------------- the eye over the last moment
class Steady {
public:
    // Each frame: the game's eye in the world, the soldier's stance (0-2), on foot (not a cutscene's or a ride's camera).
    void Add(unsigned long long now, const float* eye, int stance, bool onFoot) {
        if (!eye || !std::isfinite(eye[0]) || !std::isfinite(eye[1]) || !std::isfinite(eye[2]))
            return;
        stance = std::clamp(stance, 0, 2);
        if (count_ > 0) {
            const Sample& last = At(0);
            if (now < last.at)
                Clear();  // (the clock went back: a new run of samples)
            else if (now - last.at < kSampleMs && last.stance == stance && last.onFoot == onFoot)
                return;
        }
        Sample& s = samples_[next_];
        s.at = now, s.eye[0] = eye[0], s.eye[1] = eye[1], s.eye[2] = eye[2], s.stance = stance, s.onFoot = onFoot;
        next_ = (next_ + 1) % kSamples;
        count_ = std::min(count_ + 1, kSamples);
    }
    // Steady over the last kSteadyMs: his feet's height (the eye's mean height less its height for the stance) and its stance. False: on the move up or
    // down, in another stance or a scripted camera within it, riding, or not watched long enough.
    bool Feet(unsigned long long now, float& feet, int* stance = nullptr) const {
        if (count_ < 2)
            return false;
        const Sample& newest = At(0);
        if (now < newest.at || now - newest.at > 4 * kSampleMs)
            return false;  // (no eye lately)
        float low = newest.eye[1], high = newest.eye[1], sum = 0.0f;
        int n = 0;
        const Sample* oldest = &newest;
        for (int k = 0; k < count_; ++k) {
            const Sample& s = At(k);
            if (k > 0 && newest.at - s.at > kSteadyMs)
                break;
            if (!s.onFoot || s.stance != newest.stance)
                return false;
            low = std::min(low, s.eye[1]), high = std::max(high, s.eye[1]);
            sum += s.eye[1];
            ++n;
            oldest = &s;
        }
        const unsigned long long span = newest.at - oldest->at;
        if (span + 2 * kSampleMs < kSteadyMs || n < 6 || high - low > kSteadySpan)
            return false;  // (watched too short a time, too few, or up and down)
        const float dx = newest.eye[0] - oldest->eye[0], dz = newest.eye[2] - oldest->eye[2];
        const float most = kSteadyMostSpeed * static_cast<float>(span) / 1000.0f + 0.05f;
        if (dx * dx + dz * dz > most * most)
            return false;
        feet = sum / static_cast<float>(n) - kEyeHeight[newest.stance];
        if (stance)
            *stance = newest.stance;
        return true;
    }
    void Clear() { count_ = next_ = 0; }

private:
    struct Sample {
        unsigned long long at = 0;
        float eye[3] = {};
        int stance = 0;
        bool onFoot = false;
    };
    static constexpr int kSamples = 32;
    const Sample& At(int back) const { return samples_[(next_ - 1 - back + 2 * kSamples) % kSamples]; }
    Sample samples_[kSamples];
    int next_ = 0, count_ = 0;
};

// ---------------------------------------------------------------- what the rays say of his floor
enum class Eye { kSeen, kUnseen, kNotSure };
// The ray straight down from his eye (its reach the caller's: 3 m): a floor at his feet (from kFeetBelow under them to kFeetAbove over them) is seen;
// nothing, or a floor further down, unseen; something over his feet (a table he leans over) or not facing up (a wall's edge, an underside) tells nothing.
inline Eye FromEye(bool found, float normalY, float pointY, float feet) {
    if (!found)
        return Eye::kUnseen;
    if (!std::isfinite(pointY) || !std::isfinite(normalY) || normalY < 0.3f || pointY > feet + kFeetAbove)
        return Eye::kNotSure;
    return pointY >= feet - kFeetBelow ? Eye::kSeen : Eye::kUnseen;
}
// A round ray's: a floor facing up at his feet's height.
inline bool AtFeet(bool found, float normalY, float pointY, float feet) {
    return found && std::isfinite(pointY) && normalY >= 0.7f && std::fabs(pointY - feet) <= kRoundFeet;
}
// Where the i-th round ray starts: kRoundAt across from his eye, at its height, every 45 degrees.
inline void RoundFrom(const float* eye, int i, float* out) {
    const float a = 0.78539816f * static_cast<float>(((i % kRound) + kRound) % kRound);
    out[0] = eye[0] + kRoundAt * std::cos(a);
    out[1] = eye[1];
    out[2] = eye[2] + kRoundAt * std::sin(a);
}

// ---------------------------------------------------------------- the prints
class Prints {
public:
    // A place he stood (his feet's height there): the same as one kept (kApart across, kSame up or down), it stays as it is; within kReach of one and
    // kLevel of its height, its height.
    void Add(float x, float y, float z) {
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
            return;
        for (int i = 0; i < count_; ++i)
            if (Across2(p_[i], x, z) <= kApart * kApart && std::fabs(p_[i].y - y) <= kSame)
                return;
        for (int i = 0; i < count_; ++i)
            if (Across2(p_[i], x, z) <= kReach * kReach && std::fabs(p_[i].y - y) <= kLevel) {
                y = p_[i].y;
                break;
            }
        p_[next_] = Print{x, y, z};
        next_ = (next_ + 1) % kPrints;
        count_ = std::min(count_ + 1, kPrints);
    }
    // The floor he stood on under a point (the world): of the prints within kReach across whose height is no more than kLetOff over it, the floor
    // nearest under it (the highest of them, and kBand under it: a floor's own, not one under it), the nearest of those across (a step, a slope: the
    // height where he stood nearest it).
    bool Under(const float* at, float& y) const {
        if (!at || !std::isfinite(at[0]) || !std::isfinite(at[1]) || !std::isfinite(at[2]))
            return false;
        bool found = false;
        float top = 0.0f;
        for (int i = 0; i < count_; ++i)
            if (p_[i].y <= at[1] + kLetOff && Across2(p_[i], at[0], at[2]) <= kReach * kReach && (!found || p_[i].y > top))
                top = p_[i].y, found = true;
        if (!found)
            return false;
        float nearest = 0.0f;
        bool have = false;
        for (int i = 0; i < count_; ++i) {
            const float d = Across2(p_[i], at[0], at[2]);
            if (p_[i].y <= top && p_[i].y >= top - kBand && d <= kReach * kReach && (!have || d < nearest))
                nearest = d, y = p_[i].y, have = true;
        }
        return have;
    }
    int Count() const { return count_; }
    void Clear() { count_ = next_ = 0; }

private:
    struct Print {
        float x = 0.0f, y = 0.0f, z = 0.0f;
    };
    static float Across2(const Print& p, float x, float z) { return (p.x - x) * (p.x - x) + (p.z - z) * (p.z - z); }
    Print p_[kPrints];
    int next_ = 0, count_ = 0;
};

// ---------------------------------------------------------------- the rays' floor, or his
enum class Rays { kFloor, kGuess, kNone, kNothing };  // a ray's own floor; a guess (one cast beside); none known now; nothing as far as they went
// His floor (a print's height) stands in for theirs: theirs isn't known or isn't there, or is lower than his by more than kOver (a floor they don't see
// over it). A floor of theirs at his height or above (a crate on the deck, a guess from beside one) is theirs.
inline bool StandsIn(Rays rays, float raysY, float printY) {
    if (rays == Rays::kNone || rays == Rays::kNothing)
        return true;
    return !std::isfinite(raysY) || raysY < printY - kOver;
}

// ---------------------------------------------------------------- the watch: the eye, the survey, the verdict, the prints
struct Survey {
    bool on = false;              // its rays asked for, not all back
    unsigned serial = 0;
    float eye[3] = {};            // where it was asked from (his eye as the eye's ray was asked)
    float feet = 0.0f;
    unsigned long long asked = 0;
    unsigned back = 0;            // a bit for each round ray back
    int atFeet = 0;               // of them, finding his floor
    bool eyeFound = false;        // the eye's own ray: what it found (for the line)
    float eyeY = 0.0f;
};
struct Verdict {
    bool have = false;
    bool unseen = false;          // the floor round him is one the rays don't see: kept
    float eye[3] = {};
    float feet = 0.0f;
    int atFeet = 0;
    bool eyeFound = false;
    float eyeY = 0.0f;
    unsigned long long at = 0;    // made (or the eye's ray missing again there)
    unsigned serial = 0;
};

class Watch {
public:
    // Each frame, the game's eye (SetEye).
    void Eye(unsigned long long now, const float* eye, int stance, bool onFoot) { steady_.Add(now, eye, stance, onFoot); }
    // The eye's ray, as it's asked for: steady (with his feet's height) or not.
    bool Steady(unsigned long long now, float& feet) const { return steady_.Feet(now, feet); }
    // The eye's ray back (asked from `from`, steady with feet then): true when a survey is wanted (its rays from RoundFrom(survey().eye, i), each told
    // with Round; GiveUp if they can't be asked). The floor seen at his feet ends a verdict of unseen; a miss where one stands refreshes it (and keeps
    // the place).
    bool EyeBack(unsigned long long now, const float* from, bool steady, float feet, bool found, float normalY, float pointY) {
        if (!steady || !from)
            return false;
        const stood::Eye e = FromEye(found, normalY, pointY, feet);
        if (e == stood::Eye::kSeen) {
            verdict_.have = false;  // (on a floor the rays see: no more of his walk kept)
            return false;
        }
        if (e == stood::Eye::kNotSure)
            return false;
        if (verdict_.have && now >= verdict_.at && now - verdict_.at <= kRefreshMs && std::fabs(feet - verdict_.feet) <= kLevel &&
            Across2(from, verdict_.eye) <= kResurvey * kResurvey) {
            // The place surveyed a moment ago: its verdict stands (an edge's too, till kRefreshMs); unseen, kept again.
            if (verdict_.unseen) {
                verdict_.at = now;
                prints_.Add(from[0], feet, from[2]);
            }
            return false;
        }
        if (survey_.on)
            return false;  // (one at a time)
        survey_ = Survey{};
        survey_.on = true;
        survey_.serial = ++serial_;
        survey_.eye[0] = from[0], survey_.eye[1] = from[1], survey_.eye[2] = from[2];
        survey_.feet = feet;
        survey_.asked = now;
        survey_.eyeFound = found, survey_.eyeY = pointY;
        return true;
    }
    void GiveUp() { survey_.on = false; }
    // A round ray back (its survey's serial, its number): true once all are back (the verdict's new: Said() is false till Say()).
    bool Round(unsigned long long now, unsigned serial, int i, bool found, float normalY, float pointY) {
        if (!survey_.on || serial != survey_.serial || i < 0 || i >= kRound || (survey_.back & (1u << i)))
            return false;
        survey_.back |= 1u << i;
        survey_.atFeet += AtFeet(found, normalY, pointY, survey_.feet) ? 1 : 0;
        if (survey_.back != (1u << kRound) - 1u)
            return false;
        survey_.on = false;
        verdict_ = Verdict{};
        verdict_.have = true;
        verdict_.unseen = survey_.atFeet <= kRoundMost;
        for (int k = 0; k < 3; ++k)
            verdict_.eye[k] = survey_.eye[k];
        verdict_.feet = survey_.feet;
        verdict_.atFeet = survey_.atFeet;
        verdict_.eyeFound = survey_.eyeFound, verdict_.eyeY = survey_.eyeY;
        verdict_.at = now;
        verdict_.serial = survey_.serial;
        if (verdict_.unseen)
            prints_.Add(survey_.eye[0], survey_.feet, survey_.eye[2]);
        said_ = false;
        return true;
    }
    // Each frame: a survey not all back in kSurveyMs given up; with a fresh verdict of unseen, the place he walks to kept (steady, his feet at its height,
    // within kWalkFrom of it).
    void Frame(unsigned long long now, const float* eye) {
        if (survey_.on && now > survey_.asked && now - survey_.asked > kSurveyMs)  // (its asking stamped by the game's thread: maybe a moment after now)
            survey_.on = false;
        if (!eye || !verdict_.have || !verdict_.unseen || now < verdict_.at || now - verdict_.at > kFreshMs)
            return;
        float feet = 0.0f;
        if (!steady_.Feet(now, feet) || std::fabs(feet - verdict_.feet) > kWalkFeet || Across2(eye, verdict_.eye) > kWalkFrom * kWalkFrom)
            return;
        prints_.Add(eye[0], feet, eye[2]);
    }
    // The floor he stood on under a point (the world), and whether it stands in for what the rays say there.
    bool Under(const float* at, float& y) const { return prints_.Under(at, y); }
    const Survey& survey() const { return survey_; }
    const Verdict& verdict() const { return verdict_; }
    int Prints() const { return prints_.Count(); }
    bool Said() const { return said_; }
    void Say() { said_ = true; }
    // All forgotten (a new world, a Clear).
    void Clear() {
        steady_.Clear();
        prints_.Clear();
        survey_ = Survey{};
        verdict_ = Verdict{};
        said_ = true;
    }

private:
    static float Across2(const float* a, const float* b) { return (a[0] - b[0]) * (a[0] - b[0]) + (a[2] - b[2]) * (a[2] - b[2]); }
    stood::Steady steady_;
    stood::Prints prints_;
    Survey survey_;
    Verdict verdict_;
    unsigned serial_ = 0;
    bool said_ = true;
};

}  // namespace stood

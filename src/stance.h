#pragma once
// Crouch and prone on the right stick (the user, 2026-11-05: "the right analog stick flicked down" crouches, "flicked downwards and held" goes
// prone, both stay ("they're also toggled"), and "to get up from crouching or proning ... flick the analog stick up"), on the game's own pose.
//
// The owner, 2026-11-19 in the headset: "crouching broke ... if I flick up, I crouch". The stance was only the mod's belief, and the game's keys
// are toggles that act on the soldier's real pose (poseread.h): a sprint press standing still (the belief went to standing, the soldier stayed
// crouched), a death (the belief kept prone, the new soldier stood), and from then on X stood the soldier when the stick meant crouch and crouched it
// when the stick meant up. The belief's own rules were off too: from prone it pressed Z then X for "up" after a crouch, and Z from prone stands, so X
// crouched again.
//
// So, with the game's pose read (poseread.h, every frame): a gesture asks for a pose; the one key that takes the game there from the pose its toggles
// act on (+0x80) is pressed (X standing <-> crouched and prone -> crouched; Z into prone and prone -> standing: KeyFor); the read shows whether it
// was taken; one not taken is pressed again once, then given up (no room, water, the air, a scripted lock), and the stance stays what the game says.
// In a scripted moment (the game turning the look itself: perhaps its own keys then, a quick-time event's Z among them) a key goes once and is never
// pressed again (the review of 2026-11-19); the first still goes, as a moment seen right after going prone is the game's own turn of the look.
// A pose the game doesn't allow now (its constraints) is never pressed for; a change during a change waits for it to end when the game takes none
// then. A new soldier drops any change under way.
// Without the read (another bf3.exe, or a frame the soldier isn't there): the mod's belief, one key a change by the same rules, and the game's own
// stand-ups taken into it as the game makes them: a sprint only with the left stick pushed forward over half (the game's throttle for it), a jump,
// a new soldier (standing, as the game makes one). A key into or out of prone waits for the game's change (up to about 1.05 s) before the next
// (kProneGap: a soldier whose data takes no change during a change refuses one in it, and the belief would part: held down, then up at once). In a
// scripted moment nothing is sent then (the game may not take it, and the belief would part). Its known limit: a checkpoint that lays the new
// soldier prone (Operation Swordbreaker's RPG) is taken as standing until a flick down (X from prone crouches, and from there the belief is right).
// In a cutscene nothing is sent either way, and nothing while a fist fight's prompt is up (qtefight::GuardStance: a change under way then is dropped,
// not pressed after it); with the reader borne out and no soldier at all (a menu, a load, a death), nothing either: no keys into those.
// The head's real crouch (headmove.h) holds the game's crouch-and-hold (Left Ctrl) only from standing and only while the stick asks for nothing:
// with it down the game takes no X, so a gesture lets it go first and waits for the game to stand.
//
// Header only and free of the game: tools/stancetest.cpp runs it against a model of the game's rules.

#include <cmath>
#include <cstdint>

namespace stance {

enum Pose : int { kStand = 0, kCrouch = 1, kProne = 2 };
enum Key : int { kNoKey = 0, kKeyX = 1, kKeyZ = 2 };  // X the game's crouch toggle (CrouchAction), Z its prone toggle (EIAProne)
enum Gesture : int { kFlickDown = 0, kHeldDown = 1, kFlickUp = 2 };

inline const char* PoseName(int p) {
    return p == kStand ? "standing" : p == kCrouch ? "crouched" : p == kProne ? "prone" : "?";
}
inline const char* KeyName(int k) {
    return k == kKeyX ? "X" : k == kKeyZ ? "Z" : "no key";
}

// The game's own rules (its pose input, 0x011F7050): what a press does to the pose its toggles act on.
inline int AfterKey(int key, int target) {
    if (key == kKeyX)
        return target == kCrouch ? kStand : kCrouch;
    if (key == kKeyZ)
        return target == kProne ? kStand : kProne;
    return target;
}
// The one key that takes the game from `from` to `want` (kNoKey: it's there).
inline int KeyFor(int from, int want) {
    if (from == want)
        return kNoKey;
    if (want == kProne || (from == kProne && want == kStand))
        return kKeyZ;
    return kKeyX;
}

constexpr unsigned long long kHold = 450;        // held down this long: prone
constexpr unsigned long long kPress = 300;       // a key held this long (a tap of 0.1 s through the game's keyboard read didn't always take)
constexpr unsigned long long kGap = 150;         // from one key's let-go to the next's press
constexpr unsigned long long kProneGap = 950;    // ... without the read, after a key into or out of prone: the next 1.25 s after its press (the game's change
                                                 // takes up to about 1.05 s, its keyboard read a tick more; one in it may be refused, the belief parting)
constexpr unsigned long long kTakeMs = 700;      // a press the game's pose doesn't show by then wasn't taken
constexpr unsigned long long kWantMost = 4000;   // a change not made by then is given up
constexpr unsigned long long kHeadSettle = 600;  // after the head's crouch-and-hold is let go, for the game to stand (its change ended: a key in it may be lost)
constexpr int kMostPresses = 3;                  // presses toward one change (the game's rules need one)
constexpr int kMostNotTaken = 2;                 // presses in a row not taken, then given up
constexpr float kSprintForward = 0.5f;           // the game's: the sprint stands the soldier only with the forward throttle over half

// The game's pose this frame (poseread::Pose, as the caller copies it).
struct Read {
    bool ok = false;
    int current = kStand, target = kStand;
    float timer = 0.0f;
    bool allowed[3] = {true, true, true};
    bool changeInTransition = true;
};

struct Frame {
    unsigned long long now = 0;  // ms
    float stickX = 0.0f, stickY = 0.0f;  // the right stick
    Read read;
    uint32_t soldier = 0;      // the player's soldier as the mod finds it (god.cpp; 0: none)
    bool noSoldier = false;    // the reader borne out and no soldier now (a menu, a load, a death): nothing sent (unknown game: false, the belief goes on)
    unsigned serial = 0;       // god::SoldierSerial(): one more for every soldier found anew
    bool sprint = false;       // the sprint (the left stick's click) held now
    float forward = 0.0f;      // the forward the game is given now (-1..1; W: 1)
    bool jump = false;         // the jump held now
    bool cutscene = false;     // a cutscene's camera
    bool turn = false;         // the game turning the look itself (a scripted moment)
    bool fightGuard = false;   // a fist fight's prompt is up
    bool headHold = false;     // the head's real crouch holds the game's crouch-and-hold (Left Ctrl) now
};

enum class Ev : uint8_t {
    kGesture,     // a: the gesture, b: the pose it was measured from, c: the pose asked for
    kAlready,     // a: the gesture, b: the pose (the soldier has it already), c: 1 the head's real crouch has the game's crouch-and-hold (Left Ctrl)
                  // now or just let go of it (the stick takes the soldier as standing meanwhile), d: the stance as taken (the game's pose, or the belief)
    kPress,       // a: the key, b: the game's pose (or the belief), c: the pose asked for, d: the press's number
    kTaken,       // a: the pose asked for, d: presses; ms since the gesture
    kNotTaken,    // a: the key, b: the game's pose, c: the pose asked for, d: 0 pressed again, else given up: 1 not taken again (or the presses' most),
                  // 2 a scripted moment (never pressed twice then), 3 a fist fight's prompt up, 4 a cutscene's camera
    kNotAllowed,  // a: the pose asked for (the game's constraints say no)
    kGaveUp,      // a: the pose asked for, b: the game's pose; ms since the gesture
    kScripted,    // a change left undone in a scripted moment (said once a moment): a 1 a cutscene; read: the game's pose read (a press again left undone)
    kStood,       // the belief taken as standing: a 0 a new soldier, 1 the sprint forward, 2 a jump; b: the pose it was
    kNewSoldier,  // a new soldier: the change under way (a: the pose asked for) dropped
    kNoSoldier,   // a gesture (a) with no soldier of the player's: nothing pressed (said once a stretch without one)
};
struct Event {
    Ev what = Ev::kGesture;
    int a = 0, b = 0, c = 0, d = 0;
    unsigned long long ms = 0;
    bool read = false;  // the game's pose read this frame
};

struct Out {
    int press = kNoKey, release = kNoKey;  // a key to press now (held till its let-go), a key to let go now
    bool heldUp = false;                   // a change left undone for a fight's prompt (the caller says it, once a prompt)
    int pose = kStand;                     // the stance as taken now: the game's (read) or the mod's belief
    bool fromRead = false;
    bool prone = false;                    // prone, or still on its way out of it
    unsigned long long changedAt = 0;      // when the stance as taken last changed
    Event event[6];
    int events = 0;
};

struct State {
    unsigned long long downSince = 0;
    bool downUsed = false, upHeld = false;
    uint32_t soldier = 0;
    unsigned serial = 0;
    bool seen = false;
    int pose = kStand;
    bool fromRead = false;
    unsigned long long changedAt = 0;
    int want = -1;  // the pose asked for and not yet the game's (-1: none)
    unsigned long long wantAt = 0;
    int presses = 0, notTaken = 0;
    int key = kNoKey;  // the key down now
    unsigned long long keyUntil = 0, nextAt = 0;
    bool verifying = false;  // a press whose taking is watched
    int pressedFrom = kStand, pressedKey = kNoKey;
    unsigned long long pressedAt = 0;
    bool scriptedSaid = false, noSoldierSaid = false;
    bool headWas = false;
    unsigned long long headSettleUntil = 0;
};

namespace detail {
inline void Say(Out& o, Ev what, int a = 0, int b = 0, int c = 0, int d = 0, unsigned long long ms = 0, bool read = false) {
    if (o.events >= 6)
        return;
    Event& e = o.event[o.events++];
    e.what = what, e.a = a, e.b = b, e.c = c, e.d = d, e.ms = ms, e.read = read;
}
inline void SetPose(State& s, int pose, unsigned long long now) {
    if (pose != s.pose)
        s.changedAt = now;
    s.pose = pose;
}
inline void PressKey(State& s, Out& o, int key, unsigned long long now, unsigned long long gap = kGap) {
    o.press = key;
    s.key = key;
    s.keyUntil = now + kPress;
    s.nextAt = s.keyUntil + gap;
}
inline void Drop(State& s) {
    s.want = -1;
    s.verifying = false;
    s.presses = s.notTaken = 0;
}
}  // namespace detail

// The game's pose taken in (each frame it's read, the headset on or not; Step does it too).
inline void Observe(State& s, const Read& r, unsigned long long now) {
    if (!r.ok) {
        s.fromRead = false;
        return;
    }
    detail::SetPose(s, r.target, now);
    s.fromRead = true;
}

// The head's real crouch has the game's crouch-and-hold (Left Ctrl) now, or let go of it a moment ago (the game standing).
inline bool HeadHolds(const State& s, unsigned long long now) {
    return s.headWas || now < s.headSettleUntil;
}
// The stick's stance as the head's crouch leaves it: while that holds the game's crouch-and-hold (only ever from standing), standing.
inline int StickPose(const State& s, unsigned long long now) {
    return HeadHolds(s, now) ? kStand : s.pose;
}
// Busy: a change asked for or a key down (the head's crouch-and-hold let go meanwhile: the game takes no X with it down).
inline bool Busy(const State& s) {
    return s.want >= 0 || s.key != kNoKey;
}
// The head's real crouch may begin to hold the game's crouch-and-hold (from standing, the stick asking for nothing), and may go on holding it.
inline bool HeadMayStart(const State& s) {
    return !Busy(s) && s.pose == kStand;
}
inline bool HeadMayHold(const State& s) {
    return !Busy(s);
}

// Everything let go (the game not in front, the headset off): the key to let go of (kNoKey: none), the gestures and the change asked for dropped.
inline int LetGo(State& s) {
    const int key = s.key;
    s.key = kNoKey;
    s.downSince = 0;
    s.downUsed = false;
    s.upHeld = false;
    detail::Drop(s);
    return key;
}

inline Out Step(State& s, const Frame& f) {
    using namespace detail;
    Out o;
    const unsigned long long now = f.now;
    const Read& r = f.read;
    // The key's let-go.
    if (s.key && now >= s.keyUntil) {
        o.release = s.key;
        s.key = kNoKey;
    }
    // A new soldier (another soldier object, or one more found anew in the server's table): what was asked of the last is over.
    bool fresh = false;
    if (f.soldier) {
        if (s.soldier && f.soldier != s.soldier)
            fresh = true;
        s.soldier = f.soldier;
    }
    if (s.seen && f.serial != s.serial)
        fresh = true;
    s.serial = f.serial;
    s.seen = true;
    if (fresh) {
        if (s.want >= 0)
            Say(o, Ev::kNewSoldier, s.want);
        Drop(s);
        if (!r.ok && s.pose != kStand) {
            Say(o, Ev::kStood, 0, s.pose);
            SetPose(s, kStand, now);
        }
    }
    // The head's crouch-and-hold: let go, the game stands a moment later.
    if (s.headWas && !f.headHold)
        s.headSettleUntil = now + kHeadSettle;
    s.headWas = f.headHold;
    // The stance as taken: the game's, or the belief with the game's own stand-ups.
    if (r.ok) {
        Observe(s, r, now);
    } else {
        s.fromRead = false;
        s.verifying = false;
        if (s.pose != kStand && f.sprint && f.forward > kSprintForward) {
            Say(o, Ev::kStood, 1, s.pose);
            SetPose(s, kStand, now);
        } else if (s.pose != kStand && f.jump) {
            Say(o, Ev::kStood, 2, s.pose);
            SetPose(s, kStand, now);
        }
    }
    if (!f.cutscene && !f.turn)
        s.scriptedSaid = false;
    if (!f.noSoldier)
        s.noSoldierSaid = false;
    else if (s.want >= 0)
        Drop(s);  // (no soldier to take it)
    // The gestures. Only a push near straight down or up counts, never a turn (its sideways part under about a third of it): every controller has the
    // stick, Quest's too.
    const bool straight = std::fabs(f.stickX) < 0.35f * std::fabs(f.stickY);
    const bool down = f.stickY < -0.8f && straight, up = f.stickY > 0.8f && straight;
    int gesture = -1;
    if (down && !s.downSince) {
        s.downSince = now;
        s.downUsed = false;
    }
    if (s.downSince && !s.downUsed && down && now - s.downSince >= kHold) {
        s.downUsed = true;
        gesture = kHeldDown;
    }
    if (s.downSince && f.stickY > -0.5f) {
        if (!s.downUsed)
            gesture = kFlickDown;
        s.downSince = 0;
    }
    if (up && !s.upHeld) {
        s.upHeld = true;
        gesture = kFlickUp;
    }
    if (s.upHeld && f.stickY < 0.5f)
        s.upHeld = false;
    if (gesture >= 0) {
        const int want = gesture == kFlickDown ? kCrouch : gesture == kHeldDown ? kProne : kStand;
        const int from = s.want >= 0 ? s.want : StickPose(s, now);
        if (f.noSoldier) {
            if (!s.noSoldierSaid)
                Say(o, Ev::kNoSoldier, gesture);
            s.noSoldierSaid = true;
        } else if (want == from) {
            if (s.want < 0)
                Say(o, Ev::kAlready, gesture, from, HeadHolds(s, now) ? 1 : 0, s.pose, 0, r.ok);
        } else if (f.fightGuard) {
            o.heldUp = true;
        } else if (f.cutscene || (!r.ok && f.turn)) {
            if (!s.scriptedSaid)
                Say(o, Ev::kScripted, f.cutscene ? 1 : 0);
            s.scriptedSaid = true;
        } else {
            Say(o, Ev::kGesture, gesture, from, want, 0, 0, r.ok);
            s.want = want;
            s.wantAt = now;
            s.presses = s.notTaken = 0;
        }
    }
    // The change asked for.
    if (s.want >= 0) {
        if (r.ok) {
            bool go = true;
            if (s.verifying) {
                if (r.target != s.pressedFrom) {
                    s.verifying = false;  // taken: the pose is what it is now, by the game's rules or not
                    s.notTaken = 0;
                } else if (now - s.pressedAt >= kTakeMs) {
                    s.verifying = false;
                    // Given up (kNotTaken's d): 1 twice not taken (or the presses' most); 2 a scripted moment, never a second press then (the game's own
                    // keys, perhaps, a quick-time event's Z among them); 3 a fist fight's prompt up (left undone, as a gesture in it); 4 a cutscene's camera.
                    const int why = ++s.notTaken >= kMostNotTaken || s.presses >= kMostPresses ? 1
                                    : f.turn                                                  ? 2
                                    : f.fightGuard                                            ? 3
                                    : f.cutscene                                              ? 4
                                                                                              : 0;
                    Say(o, Ev::kNotTaken, s.pressedKey, r.target, s.want, why, now - s.wantAt, true);
                    if (why) {
                        if (why == 3)
                            o.heldUp = true;
                        Drop(s);
                        go = false;
                    }
                } else {
                    go = false;
                }
            }
            if (go && s.want >= 0) {
                if (r.target == s.want && !f.headHold && now >= s.headSettleUntil) {
                    if (s.presses)
                        Say(o, Ev::kTaken, s.want, 0, 0, s.presses, now - s.wantAt, true);
                    Drop(s);
                } else if (now - s.wantAt >= kWantMost) {
                    Say(o, Ev::kGaveUp, s.want, r.target, 0, 0, now - s.wantAt, true);
                    Drop(s);
                } else if (!r.allowed[s.want]) {
                    Say(o, Ev::kNotAllowed, s.want, r.target, 0, 0, 0, true);
                    Drop(s);
                } else if (s.key || now < s.nextAt || f.headHold || now < s.headSettleUntil) {
                    // (a key still down or just let go; the head's crouch-and-hold being let go)
                } else if (r.current != r.target && !r.changeInTransition) {
                    // (the game takes no change during a change: its end awaited)
                } else if (f.fightGuard) {
                    o.heldUp = true;
                    Drop(s);  // (left undone, as without the read: never pressed after the prompt)
                } else if (f.cutscene || (f.turn && s.presses)) {
                    // (A cutscene: nothing. A scripted moment: the first press went, never one again.)
                    if (!s.scriptedSaid)
                        Say(o, Ev::kScripted, f.cutscene ? 1 : 0, 0, 0, s.presses, 0, true);
                    s.scriptedSaid = true;
                    Drop(s);
                } else if (s.presses >= kMostPresses) {
                    Say(o, Ev::kGaveUp, s.want, r.target, 0, 0, now - s.wantAt, true);
                    Drop(s);
                } else {
                    const int key = KeyFor(r.target, s.want);
                    PressKey(s, o, key, now);
                    s.verifying = true;
                    s.pressedFrom = r.target;
                    s.pressedKey = key;
                    s.pressedAt = now;
                    ++s.presses;
                    Say(o, Ev::kPress, key, r.target, s.want, s.presses, now - s.wantAt, true);
                }
            }
        } else if (!s.key && now >= s.nextAt && !f.headHold && now >= s.headSettleUntil) {
            // The belief: one key, taken as done.
            if (s.pose == s.want) {
                Drop(s);
            } else if (f.fightGuard) {
                o.heldUp = true;
                Drop(s);
            } else if (f.cutscene || f.turn) {
                if (!s.scriptedSaid)
                    Say(o, Ev::kScripted, f.cutscene ? 1 : 0);
                s.scriptedSaid = true;
                Drop(s);
            } else {
                const int key = KeyFor(s.pose, s.want);
                // (Into or out of prone: the next key waits for the game's change to end, kProneGap.)
                PressKey(s, o, key, now, s.pose == kProne || s.want == kProne ? kProneGap : kGap);
                Say(o, Ev::kPress, key, s.pose, s.want, 1, now - s.wantAt, false);
                SetPose(s, s.want, now);
                Drop(s);
            }
        } else if (now - s.wantAt >= kWantMost) {
            Drop(s);
        }
    }
    o.pose = s.pose;
    o.fromRead = s.fromRead;
    o.prone = s.pose == kProne || (r.ok && r.current == kProne);
    o.changedAt = s.changedAt;
    return o;
}

}  // namespace stance

#pragma once
// The game's fighting quick-time events on the grips (the user, 2026-11-18: "we got to start making all the fighting QuickTime events just
// working with grip buttons, no matter what"). The game's prompts name fixed PC keys (E, F, Space, Z, the left and the right mouse button);
// which one a prompt wants isn't read yet, so while the game is in a scripted moment a fresh squeeze of either grip holds all six down for as
// long as it stays squeezed (input.cpp sends them, and Left Ctrl with them: the game's list of quick-time actions has a "CrouchDuck"). Fresh: the grip was seen let go during the moment, so one already held as it began (the
// gun's) presses nothing. The moment over, everything is let go at once, whatever the grips do: nothing leaks into play (a held left mouse
// button would fire the gun). Outside a scripted moment this holds nothing, ever. Pure, header only (tools/qtetest.cpp checks it).
//
// That is version 1 (Step; weapon.txt "qte 1"), and it is wrong for some of the game's quick-time events: the train's playthrough (2026-11-18)
// showed a climb wants Space tapped again and again, not held, and a wrong key may fail one. Version 2 (Step2; "qte 2", the default) presses
// only what the game awaits: its list of live input triggers (qtewatch.cpp reads it each frame) names the keys, and whether each is tapped (an
// edge trigger) or held. A fresh squeeze of either grip while a key is awaited presses exactly the awaited keys: a tapped one goes down 70 ms
// and up, and again every 130 ms for as long as the grip stays squeezed and the game still awaits it (a mash by holding the grip: 7.7 presses a
// second; the train's dangling climb was won at 7.4), a held one stays down while the grip is squeezed. Everything is let go the frame the game
// awaits nothing more, bf3 leaves the front or the headset loses focus. Nothing awaited (or the list not read): the grips press nothing.

namespace qte {

// The keys and buttons a prompt may name, as a mask.
enum Key : unsigned {
    kKeyE = 1,           // InteractDrag (0x3C), Interact (0x23)
    kKeyLeftMouse = 2,   // the quick-time Fire (0x3D), Fire (0x07)
    kKeyRightMouse = 4,  // Block (0x3E)
    kKeyF = 8,           // FastMelee (0x3F)
    kKeySpace = 16,      // JumpClimb (0x40), Jump (0x0C)
    kKeyZ = 32,          // CrouchDuck (0x41)
    kKeyCtrl = 64,       // (version 1 only: Left Ctrl sets none of the game's quick-time actions)
};
constexpr unsigned kKeysV1 = 127;  // version 1 holds them all
constexpr unsigned kKeysV2 = 63;   // version 2 presses among these
constexpr int kKeyCount = 7;

// A grip's squeeze past kPress presses, under kLetGo lets go (the free hands' own thresholds).
constexpr float kPress = 0.6f, kLetGo = 0.4f;
// A press is held at least this long (ms), so a flick of the grip a frame long still reaches the game's own input ticks (30 a second); never
// past the moment's end.
constexpr unsigned long long kShortest = 120;
// Version 2's tap of an edge trigger's key: down kTap ms, and while the grip stays squeezed down again every kTapEvery ms (7.7 a second; each
// state several of the game's input ticks long). Two presses never run into one: the next begins kGap ms after the last key went up at the earliest.
constexpr unsigned long long kTap = 70, kTapEvery = 130, kGap = kTapEvery - kTap;

// Why a press ended.
enum class End { kNone, kGrip, kMoment, kFront, kOff, kAwait };

struct In {
    unsigned long long nowMs = 0;
    bool on = true;         // weapon.txt "qte 0/1"
    bool front = false;     // bf3's window in front (nothing is typed into another)
    bool cutscene = false;  // the game draws from its own camera (xr_runtime.cpp's g_cutscene: unturned or a scripted camera)
    bool turn = false;      // the game turns the look yaw itself (aim::Scripted)
    bool turnArmed = false; // ... and that began with a gun in hand (aim.cpp's g_scriptedArmed: perhaps no scripted moment at all)
    bool read[2] = {};      // each grip read this frame ([0] left, [1] right)
    float squeeze[2] = {};  // how far it's squeezed (0-1)
    // Version 2 (Step2), from the game's list of live input triggers (qtewatch.cpp):
    unsigned await = 0;     // the keys the game awaits now (Key bits; 0: nothing, or the list not read)
    unsigned hold = 0;      // those of them to be held down rather than tapped (a trigger read by its level, not its edge)
    unsigned seen = 0;      // the keys whose actions the game's own input state shows on this frame ("seen by the game")
    bool mash = true;       // weapon.txt "qtemash 0/1": a tapped key again and again while the grip stays squeezed (0: one tap a squeeze)
};

struct State {
    bool moment = false;     // a scripted moment that takes presses, last frame
    bool fresh[2] = {};      // the grip was seen let go during it
    bool pressed[2] = {};    // ... and is squeezed since
    bool held = false;       // the six inputs are down
    int hand = -1;           // the grip that began the press under way
    unsigned long long heldSince = 0;
    bool ignoredSaid = false;  // (a squeeze in a turn begun gun in hand: said once a turn)
};

struct Out {
    bool held = false;     // the six inputs are to be down this frame
    bool began = false;    // the press began this frame (hand: 0 left, 1 right)
    bool ended = false;    // it ended this frame (why, heldMs)
    int hand = -1;
    End why = End::kNone;
    unsigned long long heldMs = 0;
    bool cutscene = false;  // the moment is a cutscene's camera (else the game's turn of the look)
    bool ignored = false;   // a grip squeezed in a turn that began gun in hand: not taken for a QTE (once a turn)
    unsigned keys = 0;      // the keys to be down this frame (version 1: all of them while held)
    // Version 2:
    unsigned awaited = 0;   // began: the keys the press answers; holds: those of them held, the rest tapped
    unsigned holds = 0;
    bool tapDone = false;   // a press of the keys is settled this frame: the game's input showed it (tapSeen, after tapMs), or it's up again unseen
    unsigned tapNumber = 0, tapKeys = 0, tapSeen = 0;
    unsigned long long tapMs = 0;
    bool tapCut = false;    // ... not seen, and let go before its 70 ms were up (the press ended): too short to tell
    unsigned taps = 0, seenTaps = 0;  // ended: how many presses the squeeze made, and how many the game's input showed
};

// The scripted moment that takes presses: a cutscene's camera, or the game turning the look itself with no gun in hand as it began (with one
// it may be the game reading a look slot nothing writes, in ordinary play: a squeeze then must never fire the gun).
inline bool Moment(const In& in) {
    return in.on && in.front && (in.cutscene || (in.turn && !in.turnArmed));
}

inline Out Step(State& s, const In& in) {
    Out o;
    o.cutscene = in.cutscene;
    const bool moment = Moment(in);
    auto end = [&](End why) {
        o.ended = true;
        o.why = why;
        o.hand = s.hand;
        o.heldMs = in.nowMs >= s.heldSince ? in.nowMs - s.heldSince : 0;
        s.held = false;
        s.hand = -1;
    };
    if (!moment) {
        if (s.held)
            end(!in.on ? End::kOff : !in.front ? End::kFront : End::kMoment);
        // A squeeze in a turn that began gun in hand: said once, for the log's reader (a real fight missed would show here).
        if (in.on && in.front && in.turn && in.turnArmed) {
            for (int h = 0; h < 2; ++h) {
                const bool have = in.read[h] && in.squeeze[h] == in.squeeze[h];
                if (have && in.squeeze[h] < kLetGo)
                    s.fresh[h] = true;
                else if (have && in.squeeze[h] > kPress && s.fresh[h] && !s.ignoredSaid)
                    o.ignored = s.ignoredSaid = true;
                if (!have || in.squeeze[h] > kPress)
                    s.fresh[h] = false;
            }
        } else {
            s.fresh[0] = s.fresh[1] = false;
            s.ignoredSaid = false;
        }
        s.moment = false;
        s.pressed[0] = s.pressed[1] = false;
        return o;
    }
    if (!s.moment) {  // (it begins: nothing carried in, not even a grip seen let go in a turn not taken)
        s.fresh[0] = s.fresh[1] = false;
        s.pressed[0] = s.pressed[1] = false;
        s.ignoredSaid = false;
    }
    s.moment = true;
    int first = -1;
    for (int h = 0; h < 2; ++h) {
        const bool have = in.read[h] && in.squeeze[h] == in.squeeze[h];  // (not a number: as unread)
        if (!have) {
            // Nothing is known of it: no press held on a guess, and it must be seen let go again.
            s.fresh[h] = s.pressed[h] = false;
            continue;
        }
        if (in.squeeze[h] < kLetGo) {
            s.fresh[h] = true;
            s.pressed[h] = false;
        } else if (in.squeeze[h] > kPress && s.fresh[h] && !s.pressed[h]) {
            s.pressed[h] = true;
            if (first < 0)
                first = h;
        }
    }
    const bool want = s.pressed[0] || s.pressed[1];
    if (want && !s.held) {
        s.held = true;
        s.hand = first >= 0 ? first : s.pressed[0] ? 0 : 1;
        s.heldSince = in.nowMs;
        o.began = true;
        o.hand = s.hand;
    } else if (!want && s.held && (in.nowMs < s.heldSince || in.nowMs - s.heldSince >= kShortest)) {
        end(End::kGrip);  // (a clock gone back: let go rather than held on)
    }
    o.held = s.held;
    o.keys = s.held ? kKeysV1 : 0;
    if (o.held && !o.began)
        o.hand = s.hand;
    return o;
}

// Everything let go from outside (input.cpp's ReleaseAll: the headset off, bf3 not in front): the press under way ends, nothing is carried on.
inline Out Stop(State& s, unsigned long long nowMs, End why) {
    Out o;
    if (s.held) {
        o.ended = true;
        o.why = why;
        o.hand = s.hand;
        o.heldMs = nowMs >= s.heldSince ? nowMs - s.heldSince : 0;
    }
    s = State{};
    return o;
}

// Version 2's state.
struct State2 {
    bool live = false;     // keys were awaited last frame
    bool fresh[2] = {};    // the grip was seen let go while they were
    bool pressed[2] = {};  // ... and is squeezed since
    bool owed = false;     // a fresh squeeze not yet answered (it came while the last press's key was going up)
    int owedHand = -1;
    bool on = false;       // a press under way
    int hand = -1;         // the grip that began it
    unsigned keys = 0;     // the keys it answers: those awaited as it began, less any the game awaits no more
    unsigned down = 0;     // ... down this frame
    unsigned long long since = 0;  // when it began
    unsigned long long cycle = 0;  // the tap under way, counted from its beginning in kTapEvery steps
    unsigned taps = 0, seenTaps = 0;
    bool tapOpen = false;   // the last press of the keys isn't settled yet
    bool tapArmed = false;  // ... and the game's input was seen without them since it went down (a press run into the last one is never "seen")
    unsigned tapKeys = 0;
    unsigned long long tapAt = 0;
    bool stopping = false;  // the grips let go: the tap under way is finished (and a held key's shortest time), then it ends
    unsigned long long edgeUntil = 0, holdUntil = 0;
    bool wasUp = false;     // a key of a press went up before (upAt: when the last did)
    unsigned long long upAt = 0;
};

// Version 2: the keys to be down this frame (Out.keys), by the awaited keys (In.await, hold) and the grips.
inline Out Step2(State2& s, const In& in) {
    Out o;
    o.cutscene = in.cutscene;
    const unsigned await = in.await & kKeysV2;
    const bool live = in.on && in.front && await != 0;
    auto down = [&](unsigned keys) {
        if (s.down && !keys) {
            s.wasUp = true;
            s.upAt = in.nowMs;
        }
        s.down = keys;
    };
    auto settle = [&](bool seen) {
        if (!s.tapOpen)
            return;
        s.tapOpen = false;
        o.tapDone = true;
        o.tapNumber = s.taps;
        o.tapKeys = s.tapKeys;
        o.tapSeen = seen ? in.seen & s.tapKeys : 0;
        o.tapMs = in.nowMs >= s.tapAt ? in.nowMs - s.tapAt : 0;
        s.seenTaps += seen;
    };
    auto end = [&](End why) {
        if (s.tapOpen) {  // (the last press: seen if the game's input shows it this very frame; cut short, there may have been no time to)
            const bool shown = s.tapArmed && (in.seen & s.tapKeys) != 0;
            settle(shown);
            o.tapCut = !shown && o.tapMs < kTap;
        }
        down(0);
        o.ended = true;
        o.why = why;
        o.hand = s.hand;
        o.heldMs = in.nowMs >= s.since ? in.nowMs - s.since : 0;
        o.taps = s.taps;
        o.seenTaps = s.seenTaps;
        s.on = s.stopping = false;
        s.hand = -1;
        s.keys = 0;
    };
    if (!live) {
        if (s.on)
            end(!in.on ? End::kOff : !in.front ? End::kFront : End::kAwait);
        s.live = s.owed = false;
        s.fresh[0] = s.fresh[1] = false;
        s.pressed[0] = s.pressed[1] = false;
        return o;
    }
    if (!s.live) {  // (keys awaited from this frame: nothing carried in)
        s.fresh[0] = s.fresh[1] = false;
        s.pressed[0] = s.pressed[1] = false;
        s.owed = false;
    }
    s.live = true;
    for (int h = 0; h < 2; ++h) {
        const bool have = in.read[h] && in.squeeze[h] == in.squeeze[h];  // (not a number: as unread)
        if (!have) {
            s.fresh[h] = s.pressed[h] = false;  // (nothing known of it: no press held on a guess, and it must be seen let go again)
            continue;
        }
        if (in.squeeze[h] < kLetGo) {
            s.fresh[h] = true;
            s.pressed[h] = false;
        } else if (in.squeeze[h] > kPress && s.fresh[h] && !s.pressed[h]) {
            s.pressed[h] = true;
            if (!s.owed && (!s.on || s.stopping)) {  // (with a press under way and a grip still on it, it joins that one)
                s.owed = true;
                s.owedHand = h;
            }
        }
    }
    const bool squeezed = s.pressed[0] || s.pressed[1];
    if (s.on) {
        s.keys &= await;  // (a key the game awaits no more is let go at once; one it awaits newly waits for a fresh squeeze)
        if (!s.keys) {
            end(End::kAwait);
            // What it answered is over and something else is awaited: these squeezes are spent, a new one is wanted.
            s.fresh[0] = s.fresh[1] = false;
            s.pressed[0] = s.pressed[1] = false;
            s.owed = false;
        } else if (in.nowMs < s.since) {
            end(End::kGrip);  // (a clock gone back: let go rather than held on)
            s.owed = false;
        }
    }
    if (!s.on && s.owed && (!s.wasUp || in.nowMs < s.upAt || in.nowMs - s.upAt >= kGap)) {
        s.owed = false;
        s.on = true;
        s.stopping = false;
        s.hand = s.owedHand;
        s.keys = await;
        s.since = in.nowMs;
        s.cycle = 0;
        s.taps = 1;
        s.seenTaps = 0;
        s.tapOpen = true;
        s.tapArmed = false;
        s.tapKeys = s.keys;
        s.tapAt = in.nowMs;
        o.began = true;
        o.hand = s.hand;
        o.awaited = s.keys;
        o.holds = s.keys & in.hold;
    }
    if (!s.on)
        return o;
    const unsigned holdKeys = s.keys & in.hold, edgeKeys = s.keys & ~in.hold;
    const unsigned long long elapsed = in.nowMs - s.since;
    if (!s.stopping) {
        const unsigned long long cycle = in.mash ? elapsed / kTapEvery : 0;
        const bool edgeDown = elapsed - cycle * kTapEvery < kTap;
        if (!squeezed) {  // let go: the tap under way is finished, a held key gets its shortest time
            s.stopping = true;
            s.edgeUntil = edgeDown && cycle == s.cycle ? s.since + cycle * kTapEvery + kTap : 0;
            s.holdUntil = s.since + kShortest;
        } else {
            // The next tap, once its keys have been up for a frame: after a frame gap that ran this cycle's down time into the last one's,
            // they go up for this frame (cycle != s.cycle below) and the tap begins on the next, so two taps never run into one.
            if (edgeKeys && edgeDown && cycle != s.cycle && !(s.down & edgeKeys)) {
                settle(false);
                s.cycle = cycle;
                ++s.taps;
                s.tapOpen = true;
                s.tapArmed = false;
                s.tapKeys = edgeKeys;
                s.tapAt = in.nowMs;
            }
            down(holdKeys | (edgeDown && cycle == s.cycle ? edgeKeys : 0));
        }
    }
    if (s.stopping) {
        down((in.nowMs < s.edgeUntil ? edgeKeys : 0) | (in.nowMs < s.holdUntil ? holdKeys : 0));
        if (!s.down) {
            end(End::kGrip);
            return o;
        }
    }
    // Seen by the game: its input state shows a key of the press, having shown none since it went down.
    if (s.tapOpen) {
        const unsigned shown = in.seen & s.tapKeys;
        if (!s.tapArmed)
            s.tapArmed = !shown;
        else if (shown)
            settle(true);
        if (s.tapOpen && !(s.down & s.tapKeys))
            settle(false);  // (up again, never shown)
    }
    o.held = true;
    o.keys = s.down;
    if (!o.began)
        o.hand = s.hand;
    return o;
}

// Everything let go from outside, as Stop.
inline Out Stop2(State2& s, unsigned long long nowMs, End why) {
    Out o;
    if (s.on) {
        o.ended = true;
        o.why = why;
        o.hand = s.hand;
        o.heldMs = nowMs >= s.since ? nowMs - s.since : 0;
        o.taps = s.taps;
        o.seenTaps = s.seenTaps;
    }
    const bool wasUp = s.wasUp || s.down;
    const unsigned long long upAt = s.down ? nowMs : s.upAt;
    s = State2{};
    s.wasUp = wasUp;  // (the next press still keeps its distance from a key just let go)
    s.upAt = upAt;
    return o;
}

// weapon.txt "qtetest" with version 2 (input.cpp's QteStep): the test's grip as a player's: let go, and squeezed from kTestAfter after the game comes
// to await a key until it awaits none, let go kTestEnd before the test's end. Awaited keys wholly other than the last frame's count as come anew (a
// fist fight's prompt changed with no frame between: Step2 ends the press and wants a fresh squeeze, which a grip held on would never give).
constexpr unsigned long long kTestAfter = 300, kTestEnd = 200;
struct TestGrip {
    bool awaited = false;          // the game awaited a key last frame
    unsigned long long since = 0;  // ... since then
    unsigned keys = 0;             // ... those keys
};
inline bool TestSqueezed(TestGrip& t, unsigned await, unsigned long long nowMs, unsigned long long untilMs) {
    if (!await)
        t.awaited = false;
    else if (!t.awaited || !(await & t.keys))
        t.awaited = true, t.since = nowMs;
    t.keys = await;
    return t.awaited && nowMs >= t.since + kTestAfter && nowMs + kTestEnd < untilMs;
}

inline const char* Word(End why) {
    switch (why) {
    case End::kGrip: return "the grip let go";
    case End::kMoment: return "the scripted moment over";
    case End::kFront: return "bf3 not in front, or the headset off";
    case End::kOff: return "weapon.txt \"qte 0\"";
    case End::kAwait: return "the game awaits it no more";
    default: return "";
    }
}

inline const char* KeyName(unsigned key) {
    switch (key) {
    case kKeyE: return "E";
    case kKeyLeftMouse: return "the left mouse button";
    case kKeyRightMouse: return "the right mouse button";
    case kKeyF: return "F";
    case kKeySpace: return "Space";
    case kKeyZ: return "Z";
    case kKeyCtrl: return "Left Ctrl";
    default: return "?";
    }
}

// The keys of a mask as words ("Space"; "E, the left mouse button"; "nothing"), into text.
inline const char* KeyNames(unsigned keys, char* text, unsigned size) {
    if (!size)
        return "";
    unsigned at = 0;
    text[0] = 0;
    auto add = [&](const char* word) {
        for (; *word && at + 1 < size; ++word)
            text[at++] = *word;
        text[at] = 0;
    };
    for (int k = 0; k < kKeyCount; ++k)
        if (keys & (1u << k)) {
            if (at)
                add(", ");
            add(KeyName(1u << k));
        }
    if (!at)
        add("nothing");
    return text;
}

}  // namespace qte

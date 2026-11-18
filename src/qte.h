#pragma once
// The game's fighting quick-time events on the grips (the user, 2026-11-18: "we got to start making all the fighting QuickTime events just
// working with grip buttons, no matter what"). The game's prompts name fixed PC keys (E, F, Space, Z, the left and the right mouse button);
// which one a prompt wants isn't read yet, so while the game is in a scripted moment a fresh squeeze of either grip holds all six down for as
// long as it stays squeezed (input.cpp sends them, and Left Ctrl with them: the game's list of quick-time actions has a "CrouchDuck"). Fresh: the grip was seen let go during the moment, so one already held as it began (the
// gun's) presses nothing. The moment over, everything is let go at once, whatever the grips do: nothing leaks into play (a held left mouse
// button would fire the gun). Outside a scripted moment this holds nothing, ever. Pure, header only (tools/qtetest.cpp checks it).

namespace qte {

// A grip's squeeze past kPress presses, under kLetGo lets go (the free hands' own thresholds).
constexpr float kPress = 0.6f, kLetGo = 0.4f;
// A press is held at least this long (ms), so a flick of the grip a frame long still reaches the game's own input ticks (30 a second); never
// past the moment's end.
constexpr unsigned long long kShortest = 120;

// Why a press ended.
enum class End { kNone, kGrip, kMoment, kFront, kOff };

struct In {
    unsigned long long nowMs = 0;
    bool on = true;         // weapon.txt "qte 0/1"
    bool front = false;     // bf3's window in front (nothing is typed into another)
    bool cutscene = false;  // the game draws from its own camera (xr_runtime.cpp's g_cutscene: unturned or a scripted camera)
    bool turn = false;      // the game turns the look yaw itself (aim::Scripted)
    bool turnArmed = false; // ... and that began with a gun in hand (aim.cpp's g_scriptedArmed: perhaps no scripted moment at all)
    bool read[2] = {};      // each grip read this frame ([0] left, [1] right)
    float squeeze[2] = {};  // how far it's squeezed (0-1)
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

inline const char* Word(End why) {
    switch (why) {
    case End::kGrip: return "the grip let go";
    case End::kMoment: return "the scripted moment over";
    case End::kFront: return "bf3 not in front, or the headset off";
    case End::kOff: return "weapon.txt \"qte 0\"";
    default: return "";
    }
}

}  // namespace qte

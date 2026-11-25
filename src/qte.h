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
//
// A grip already squeezed answers too (the owner in Operation Swordbreaker's cellar, 2026-11-19: "the QuickTime events, even for opening doors and
// fighting enemies ... do not work"; the fight's E and its 0.41 s Fire prompt were read, and no grip press ever began: a grip had to be seen let go
// under 0.4 and squeezed past 0.6 inside each prompt, and one held as it opened, or squeezed on harder, pressed nothing). Now a grip squeezed past
// 0.6 as a key comes to be awaited, or since without having been seen let go, presses it as a fresh squeeze does: a fist fight's prompt at once
// (its gates are the game's own word that it's open); a single quick-time event's trigger (0x3C-0x41; play's own Fire, Jump and Interact, and a single
// live beside a fight, want a fresh squeeze) in a scripted moment, kHeldSettle after its key came to be awaited. In play a held grip presses nothing
// (the right grip holds the gun there, the left the handguard): a fresh squeeze, as before. A fight's press prompt is pressed once, by whichever grip,
// never a second time (qte_fights.md: a second press can reach the next prompt's wrong gate).
//
// The review of that (2026-11-19): a press begun by a grip squeezed already ends the frame such a grip may press its keys no more (a single's scripted
// moment over: never on into play, where the right grip holds the gun); a press prompt's tap keeps its 70 ms over a frame its prompt isn't read clearly
// (a frame the fights couldn't be read, its gates unclear with no other action's open), never cut to a frame the game may not see; its "one press" is
// one the game's input showed (or that couldn't be told): a tap it never showed (a key held down from elsewhere as the prompt opened, a click held
// back) leaves the prompt to a fresh squeeze. A grip held on through a break in a prompt pressed is never taken for a second press of it.

#include <cstdarg>
#include <cstdio>

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
// A grip already squeezed (not seen let go since the keys came to be awaited) presses a single quick-time event's key this long after it came to
// be awaited: the game applies a trigger's add within 50 ms, and a key already down as it takes the trigger is no press of it. A fight's prompt: at once.
constexpr unsigned long long kHeldSettle = 100;
// A fight's press prompt pressed once is kept so over frames the fights couldn't be read, frames whose gates are unclear (a prompt up with no one
// key), and the headset off or bf3 out of front this long at most (as the fights' guard lapses); a frame read with no prompt, or another key's, ends it.
constexpr unsigned long long kPromptForget = 250;

// Why a press ended. kHeldOff: a press begun by a grip squeezed already, its keys no longer a single quick-time event's proper in a scripted moment
// (play's own key with them, or a fist fight's triggers live): such a grip may press them no more (the moment over itself: kMoment).
enum class End { kNone, kGrip, kMoment, kFront, kOff, kAwait, kHeldOff };

// Version 2: why nothing is down on a frame keys are awaited (for the log: input.cpp's "QTE grips" lines).
enum class Idle : unsigned char {
    kNone,         // keys down, or nothing awaited
    kOpen,         // neither grip squeezed past kPress
    kUnread,       // neither grip read
    kNotFresh,     // a grip squeezed, but held from before, and the keys are no fight's prompt nor is this a scripted moment: a fresh squeeze is wanted
    kPlayKey,      // ... in a scripted moment, but a key is play's own Fire, Jump or Interact's alone, or a fight's triggers are live: a fresh squeeze is wanted
    kSettle,       // a grip held from before waits kHeldSettle for the game to take its trigger
    kAnswered,     // a fight's press prompt pressed already, the game's input showing it (or that couldn't be told): one press a prompt
    kGap,          // a squeeze waits for the last press's key to have been up kGap
    kTapDone,      // the press under way has made its tap (one a squeeze, or the press prompt's one), the grip still squeezed
    kBetweenTaps,  // a mash: its key up between two taps
    kSpent,        // a grip still squeezed whose press is over (a clock gone back): let go and squeeze again
    kUnshown,      // a press prompt's tap made, the grip still squeezed, and the game's input never showed it (a key down from elsewhere, a click
                   // held back): let go and squeeze again, that press it again
};
constexpr int kIdleCount = 12;

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
    bool seenRead = false;  // ... that state was read this frame (a tap it couldn't be read for is one the game may have taken: a fight's press
                            // prompt counts it pressed)
    bool mash = true;       // weapon.txt "qtemash 0/1": a tapped key again and again while the grip stays squeezed (0: one tap a squeeze)
    // A fist fight's prompt (qtefight.h, through qtewatch.cpp's frame): while one is up, await is its one key (or nothing while its gates are unclear).
    bool prompt = false;    // a fight's prompt is up
    bool once = false;      // ... a press: one tap the prompt, never a second
    bool unknown = false;   // the fights couldn't be read this frame (nothing awaited: a prompt may still be up, and what it was given is kept)
    unsigned gates = 0;     // a fight's prompt up: the keys of all its gates read open, right or wrong (read unclearly: is another action's among them)
    unsigned quick = 0;     // no fight's prompt: of await, the keys a quick-time action proper (0x3C-0x41) awaits, none while a fight's triggers are live
                            // (qtelist::QuickKeys); a grip squeezed already presses a single's keys in a scripted moment only if all of them are these
                            // (play's own Fire, Jump and Interact, and a single beside a fight: a fresh squeeze)
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
    bool byHold = false;    // began: by a grip already squeezed (not seen let go since the keys came to be awaited), not a fresh squeeze
    bool once = false;      // began: a fight's press prompt: this tap is its one
    bool again = false;     // ... pressed again: its press before wasn't shown by the game's input
    bool mashing = false;   // began: its tapped keys tapped again every kTapEvery while the grip stays squeezed
    bool refused = false;   // a squeeze this frame pressed nothing: its fight's press prompt was pressed already, the game's input showing it or
    int refusedHand = -1;   // that not told (refusedHand's)
    bool refusedHeld = false;  // ... a grip held on, taken again after a break in its prompt (a frame not read clearly, the headset off a moment): no
                               // squeeze of the player's
    Idle idle = Idle::kNone;  // keys awaited and none down this frame: why
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
    bool pressed[2] = {};  // ... and is squeezed since (fresh, or taken as it was held: a fight's prompt, a scripted moment)
    bool owed = false;     // a squeeze not yet answered (it came while the last press's key was going up)
    int owedHand = -1;
    bool owedHeld = false;  // ... by a grip already squeezed, not a fresh squeeze
    unsigned lastAwait = 0;            // the keys awaited last frame
    unsigned long long keysSince = 0;  // since when the newest of them is awaited (a held grip's kHeldSettle)
    unsigned promptKey = 0;            // a fight's prompt as read up (its key), and the taps begun in it
    unsigned promptTaps = 0;
    unsigned promptShown = 0;          // ... of them, those the game's input showed, or that couldn't be told (its input not read, a tap cut short):
                                       // its one press made (a press prompt is pressed again only while this is 0)
    unsigned promptId = 0;             // ... which prompt it is (one more each time a prompt's key is read up anew)
    unsigned long long promptAt = 0;   // ... the last frame it was read up (kPromptForget)
    bool on = false;       // a press under way
    int hand = -1;         // the grip that began it
    bool byHold = false;   // ... squeezed already (taken as it was held), not a fresh squeeze
    unsigned pressPrompt = 0;  // ... the prompt it began in (promptId), its taps counted for that prompt alone
    unsigned keys = 0;     // the keys it answers: those awaited as it began, less any the game awaits no more
    unsigned down = 0;     // ... down this frame
    unsigned long long since = 0;  // when it began
    unsigned long long cycle = 0;  // the tap under way, counted from its beginning in kTapEvery steps
    unsigned taps = 0, seenTaps = 0;
    bool tapOpen = false;   // the last press of the keys isn't settled yet
    bool tapArmed = false;  // ... and the game's input was seen without them since it went down (a press run into the last one is never "seen")
    bool tapBlind = false;  // ... a frame of it with the game's input not read: not shown is no telling it wasn't taken
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
    // A press of the keys settled: seen (the game's input showed it) or not. A fight's prompt's press counts as its one made (promptShown: a press
    // prompt is never pressed again) once shown, or when that can't be told (unsure: let go too soon to tell; a frame of it with the game's input not
    // read): only a tap the game's input was read for throughout and never showed leaves the prompt to another press.
    auto settle = [&](bool seen, bool unsure) {
        if (!s.tapOpen)
            return;
        s.tapOpen = false;
        o.tapDone = true;
        o.tapNumber = s.taps;
        o.tapKeys = s.tapKeys;
        o.tapSeen = seen ? in.seen & s.tapKeys : 0;
        o.tapMs = in.nowMs >= s.tapAt ? in.nowMs - s.tapAt : 0;
        s.seenTaps += seen;
        if (s.promptKey && s.pressPrompt == s.promptId && (seen || unsure || s.tapBlind || !in.seenRead))
            ++s.promptShown;
    };
    // Seen by the game: its input state shows a key of the press, having shown none since it went down.
    auto watchTap = [&]() {
        if (!s.tapOpen)
            return;
        s.tapBlind = s.tapBlind || !in.seenRead;
        const unsigned shown = in.seen & s.tapKeys;
        if (!s.tapArmed)
            s.tapArmed = !shown;
        else if (shown)
            settle(true, false);
    };
    auto end = [&](End why) {
        if (s.tapOpen) {  // (the last press: seen if the game's input shows it this very frame; cut short, there may have been no time to)
            const bool shown = s.tapArmed && (in.seen & s.tapKeys) != 0;
            const bool cut = !shown && (in.nowMs < s.tapAt || in.nowMs - s.tapAt < kTap);
            settle(shown, cut);
            o.tapCut = cut;
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
    // A fight's prompt as read: what its press prompt was given is kept over frames the fights couldn't be read and while its gates are unclear (a
    // prompt up with no one key), and kPromptForget without a frame; a frame read with no prompt, or another key's prompt, begins anew.
    if (s.promptKey && (in.nowMs < s.promptAt || in.nowMs - s.promptAt > kPromptForget))
        s.promptKey = s.promptTaps = s.promptShown = 0;
    if (!in.unknown) {
        if (!in.prompt) {
            s.promptKey = s.promptTaps = s.promptShown = 0;
        } else if (await && await != s.promptKey) {
            s.promptKey = await, s.promptTaps = s.promptShown = 0;
            ++s.promptId;
        }
        if (in.prompt && s.promptKey)
            s.promptAt = in.nowMs;
    }
    // Idle (for the log): why nothing is down this frame, keys being awaited.
    const bool scripted = in.cutscene || (in.turn && !in.turnArmed);
    bool have[2] = {}, waits = false, notFresh = false;
    const auto idle = [&]() {
        if (o.keys)
            return;
        const bool unshown = in.once && s.promptKey && s.pressPrompt == s.promptId && !s.promptShown && !s.tapOpen;  // (its tap never shown)
        o.idle = s.on                                      ? (!s.stopping && in.mash && (s.keys & ~in.hold) ? Idle::kBetweenTaps
                                                              : unshown                                     ? Idle::kUnshown
                                                                                                            : Idle::kTapDone)
                 : in.once && s.promptKey && s.promptShown ? Idle::kAnswered
                 : s.owed                                  ? Idle::kGap
                 : s.pressed[0] || s.pressed[1]            ? Idle::kSpent
                 : !have[0] && !have[1]                    ? Idle::kUnread
                 : waits                                   ? Idle::kSettle
                 : notFresh                                ? (scripted && !in.prompt ? Idle::kPlayKey : Idle::kNotFresh)
                                                           : Idle::kOpen;
    };
    if (!live) {
        // A fight's prompt still up as far as is known, read unclearly this frame: the fights not read, or its gates unclear with none open but its own
        // key's (its closing caught halfway): the tap under way keeps its keys down till its kTap is up (nothing new pressed; the guard still holds up
        // every other quick-time key), so the prompt's press is never cut to a frame the game may not see. Another action's gate open: the game is
        // changing prompts, and the key, a wrong one for the next, is let go at once (as the fights' rule presses nothing then); bf3 out of front, the
        // headset off, "qte 0": at once, as ever.
        if (s.on && s.promptKey && s.pressPrompt == s.promptId && (in.unknown || (in.prompt && !(in.gates & ~s.tapKeys))) && in.on && in.front &&
            (s.down & s.tapKeys) && in.nowMs >= s.tapAt && in.nowMs - s.tapAt < kTap) {
            watchTap();
            o.held = true;
            o.keys = s.down;
            o.hand = s.hand;
            return o;
        }
        if (s.on)
            end(!in.on ? End::kOff : !in.front ? End::kFront : End::kAwait);
        s.live = s.owed = false;
        s.fresh[0] = s.fresh[1] = false;
        s.pressed[0] = s.pressed[1] = false;
        s.lastAwait = 0;
        return o;
    }
    // A grip already squeezed may press: a fight's prompt (at once), or a single quick-time event's keys in a scripted moment (kHeldSettle after they
    // came; all of them a quick-time action's proper, never play's own Fire, Jump or Interact alone); in play never.
    const bool heldMay = in.prompt || (scripted && !(await & ~in.quick));
    // A press begun by a grip squeezed already goes on only while such a grip may press its keys: a fight's prompt, or a single quick-time event's keys
    // proper in a scripted moment. The moment over (or play's own key, or a fist fight's triggers, live with them), it ends this frame, the grips left as
    // held (taken again if the moment comes back, never in play): it never runs on into play, where the right grip holds the gun. A fresh squeeze's
    // press goes on as ever; keys awaited no more end a press below.
    if (s.on && s.byHold && (s.keys & await) && !(in.prompt || (scripted && !(s.keys & await & ~in.quick)))) {
        end(scripted ? End::kHeldOff : End::kMoment);
        s.owed = false;
        s.pressed[0] = s.pressed[1] = false;
    }
    if (!s.live) {  // (keys awaited from this frame: nothing carried in)
        s.fresh[0] = s.fresh[1] = false;
        s.pressed[0] = s.pressed[1] = false;
        s.owed = false;
        s.keysSince = in.nowMs;
    } else if (!(await & s.lastAwait)) {
        // Keys wholly other than the last frame's (a fight's next prompt with no frame between): a grip still squeezed may be taken anew for them, as
        // it was held (fresh as it was); the press under way ends below.
        if (heldMay)
            s.pressed[0] = s.pressed[1] = false;
        s.keysSince = in.nowMs;
    } else if (await & ~s.lastAwait) {
        s.keysSince = in.nowMs;  // (a key newly awaited beside them: a held grip's settle from now)
    }
    s.lastAwait = await;
    s.live = true;
    const bool settled = in.prompt || in.nowMs < s.keysSince || in.nowMs - s.keysSince >= kHeldSettle;
    for (int h = 0; h < 2; ++h) {
        have[h] = in.read[h] && in.squeeze[h] == in.squeeze[h];  // (not a number: as unread)
        if (!have[h]) {
            s.fresh[h] = s.pressed[h] = false;  // (nothing known of it: no press held on a guess, and it must be seen let go again)
            continue;
        }
        if (in.squeeze[h] < kLetGo) {
            s.fresh[h] = true;
            s.pressed[h] = false;
        } else if (in.squeeze[h] > kPress && !s.pressed[h]) {
            const bool held = !s.fresh[h];  // (squeezed as the keys came, or since without having been seen let go)
            if (held && !heldMay) {
                notFresh = true;
                continue;
            }
            if (held && !settled) {
                waits = true;
                continue;
            }
            s.pressed[h] = true;
            if (!s.owed && (!s.on || s.stopping)) {  // (with a press under way and a grip still on it, it joins that one)
                s.owed = true;
                s.owedHand = h;
                s.owedHeld = held;
            }
        }
    }
    if (s.on) {
        s.keys &= await;  // (a key the game awaits no more is let go at once; one it awaits newly waits for a fresh squeeze)
        if (!s.keys) {
            end(End::kAwait);
            // What it answered is over and something else is awaited: these squeezes are spent, a new one is wanted; where a grip already squeezed
            // may press, one still squeezed is taken for the keys awaited now as held from before (a fight's next prompt: this very frame).
            s.fresh[0] = s.fresh[1] = false;
            s.pressed[0] = s.pressed[1] = false;
            s.owed = false;
            for (int h = 0; h < 2 && heldMay; ++h) {
                if (!have[h] || !(in.squeeze[h] > kPress))
                    continue;
                if (!settled) {
                    waits = true;
                    continue;
                }
                s.pressed[h] = true;
                if (!s.owed) {
                    s.owed = true;
                    s.owedHand = h;
                    s.owedHeld = true;
                }
            }
        } else if (in.nowMs < s.since) {
            end(End::kGrip);  // (a clock gone back: let go rather than held on)
            s.owed = false;
        }
    }
    if (!s.on && s.owed && (!s.wasUp || in.nowMs < s.upAt || in.nowMs - s.upAt >= kGap)) {
        s.owed = false;
        if (in.once && s.promptKey && s.promptShown) {
            o.refused = true;  // (its fight's press prompt pressed already, the game's input showing it or that not told: never a second press of it)
            o.refusedHand = s.owedHand;
            o.refusedHeld = s.owedHeld;
        } else {
            s.on = true;
            s.stopping = false;
            s.hand = s.owedHand;
            s.byHold = s.owedHeld;
            s.pressPrompt = s.promptId;
            s.keys = await;
            s.since = in.nowMs;
            s.cycle = 0;
            s.taps = 1;
            s.seenTaps = 0;
            s.tapOpen = true;
            s.tapArmed = false;
            s.tapBlind = false;
            s.tapKeys = s.keys;
            s.tapAt = in.nowMs;
            o.began = true;
            o.hand = s.hand;
            o.awaited = s.keys;
            o.holds = s.keys & in.hold;
            o.byHold = s.owedHeld;
            o.once = in.once;
            o.again = in.once && s.promptKey && s.promptTaps > 0;  // (its press before never shown by the game's input)
            o.mashing = in.mash && (s.keys & ~in.hold) != 0;
            if (s.promptKey)
                ++s.promptTaps;
        }
    }
    if (!s.on) {
        idle();
        return o;
    }
    const bool squeezed = s.pressed[0] || s.pressed[1];
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
                settle(false, false);
                s.cycle = cycle;
                ++s.taps;
                if (s.promptKey)
                    ++s.promptTaps;
                s.tapOpen = true;
                s.tapArmed = false;
                s.tapBlind = false;
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
            idle();
            return o;
        }
    }
    watchTap();
    if (s.tapOpen && !(s.down & s.tapKeys))
        settle(false, false);  // (up again, never shown)
    o.held = true;
    o.keys = s.down;
    if (!o.began)
        o.hand = s.hand;
    idle();
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
    unsigned promptShown = s.promptShown;
    if (s.on && s.tapOpen && s.promptKey && s.pressPrompt == s.promptId)
        ++promptShown;  // (its prompt's tap cut short with no frame to tell: taken as one the game may have had)
    const unsigned promptKey = s.promptKey, promptTaps = s.promptTaps, promptId = s.promptId;
    const unsigned long long promptAt = s.promptAt;
    s = State2{};
    s.wasUp = wasUp;  // (the next press still keeps its distance from a key just let go)
    s.upAt = upAt;
    // (A fight's press prompt pressed stays so: kPromptForget.)
    s.promptKey = promptKey, s.promptTaps = promptTaps, s.promptShown = promptShown, s.promptId = promptId, s.promptAt = promptAt;
    return o;
}

// weapon.txt "qtetest" with version 2 (input.cpp's QteStep): the test's grip as a player's: let go, and squeezed from kTestAfter after the game comes
// to await a key until it awaits none, let go kTestEnd before the test's end. Awaited keys wholly other than the last frame's count as come anew (a
// fist fight's prompt changed with no frame between: the grip let go and squeezed again, a fresh squeeze for the new prompt as filmed on 2026-11-18).
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
// "qtetest 600 held": the test's grip held the whole test (from fromMs, the test's start and 0.3 s, until kTestEnd before its end), whatever the game
// awaits and between its prompts too: a player holding a grip through a fight, the owner's way (2026-11-19). It is never seen let go in a prompt, so
// it presses only as a grip already squeezed: a fight's prompts at once, a single quick-time event's in a scripted moment kHeldSettle on.
inline bool TestHeld(unsigned long long nowMs, unsigned long long fromMs, unsigned long long untilMs) {
    return nowMs >= fromMs && nowMs + kTestEnd < untilMs;
}

inline const char* Word(End why) {
    switch (why) {
    case End::kGrip: return "the grip let go";
    case End::kMoment: return "the scripted moment over";
    case End::kFront: return "bf3 not in front, or the headset off";
    case End::kOff: return "weapon.txt \"qte 0\"";
    case End::kAwait: return "the game awaits it no more";
    case End::kHeldOff: return "a grip held from before presses it no more: play's own key or a fist fight's triggers live with it";
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

// A frame's taps as input.cpp's QteStep asks Step2 for them (In::mash, In::once): a fight's prompt (prompt) a press, one tap the prompt and never mashed,
// or a mash (promptMash: tapped again every kTapEvery while the grip stays squeezed, as a single's is); weapon.txt "qtemash 0" (qtemash false) one tap a
// squeeze everywhere.
inline void TapRule(bool qtemash, bool prompt, bool promptMash, bool& mash, bool& once) {
    mash = qtemash && (!prompt || promptMash);
    once = prompt && !promptMash;
}

// The squeeze's line's words for its tapped keys (input.cpp QteSay2), true to what Step2 began (o.began): a fight's press prompt's one tap, a tap again
// every kTapEvery only where the keys are mashed (o.mashing), else one tap a squeeze; "" with every key held (none tapped). The 2026-11-18 check: the
// fights' press prompts were said to be "tapped again every 130 ms", which they never were (one tap a squeeze).
inline const char* TapWords(const Out& o) {
    static_assert(kTap == 70 && kTapEvery == 130, "TapWords' own numbers");
    const unsigned tapped = o.awaited & ~o.holds;
    return !tapped              ? ""
           : o.once && o.again  ? ": tapped 70 ms, once more (a fight's press prompt whose press before the game's input never showed)"
           : o.once             ? ": tapped 70 ms, once (a fight's press prompt: one press a prompt)"
           : o.mashing          ? ": tapped 70 ms, and again every 130 ms while the grip stays squeezed"
                                : ": tapped 70 ms, once a squeeze";
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

// ---- Version 2's grips while keys are awaited, for the log (input.cpp's "QTE grips:" lines: the owner's cellar fight of 2026-11-19 had none) ----
// A stretch: from keys coming to be awaited until none are, or keys wholly other (a fight's next prompt). Its first frame is said, again every
// kWatchEvery while it lasts (kWatchLines a stretch at most), and once as it ends: what the grips did in it and, if they pressed nothing, why.
constexpr unsigned long long kWatchEvery = 500;
constexpr int kWatchLines = 4;
struct Watch2 {
    bool on = false;
    unsigned keys = 0;  // its keys (the first frame's, and any awaited beside them since)
    bool prompt = false, once = false, scripted = false;  // as it began: a fight's prompt, a press prompt; a scripted moment
    unsigned long long since = 0, nextAt = 0, lastMs = 0;
    int lines = 0, frames = 0;
    int presses = 0, refused = 0;  // presses begun in it; squeezes that pressed nothing (its press prompt's one made)
    int refusedHeld = 0;           // ... of those, a grip held on taken again after a break in its prompt (no squeeze of the player's)
    int again = 0;                 // presses of its press prompt again, the one before never shown by the game's input
    int firstHand = -1;            // the first press's grip, held from before or fresh, and how far in
    bool firstByHold = false;
    unsigned long long firstMs = 0;
    float least[2] = {2.0f, 2.0f}, most[2] = {-1.0f, -1.0f};  // each grip's squeeze over the frames it was read
    int unread[2] = {};
    unsigned idles = 0;  // why nothing was down, over its frames (bits 1 << Idle)
};
struct WatchOut {
    bool say = false;     // a line now: the grips as this frame has them
    bool sayEnd = false;  // the stretch ended this frame: ended, as it was
    Watch2 ended;
};
// Each frame after Step2, with its In and Out.
inline WatchOut WatchStep(Watch2& w, const In& in, const Out& o) {
    WatchOut out;
    const unsigned await = in.on && in.front ? in.await & kKeysV2 : 0;
    // A frame a fight's prompt is read unclearly (its gates unclear, or the fights not read: nothing awaited, the prompt as it was kept by Step2) goes on
    // with the stretch under way, said nowhere: a prompt whose gates flicker so frame after frame is one stretch, never a line pair a frame (the review).
    if (w.on && !await && in.on && in.front && (in.unknown || in.prompt))
        return out;
    if (w.on && (!await || !(await & w.keys))) {
        out.sayEnd = true;
        out.ended = w;
        w.on = false;
    }
    if (!await)
        return out;
    if (!w.on) {
        w = Watch2{};
        w.on = true;
        w.prompt = in.prompt;
        w.once = in.once;
        w.scripted = in.cutscene || (in.turn && !in.turnArmed);
        w.since = w.nextAt = in.nowMs;
    }
    w.keys |= await;
    w.lastMs = in.nowMs;
    ++w.frames;
    for (int h = 0; h < 2; ++h) {
        if (in.read[h] && in.squeeze[h] == in.squeeze[h]) {
            if (in.squeeze[h] < w.least[h])
                w.least[h] = in.squeeze[h];
            if (in.squeeze[h] > w.most[h])
                w.most[h] = in.squeeze[h];
        } else {
            ++w.unread[h];
        }
    }
    if (o.began) {
        if (!w.presses) {
            w.firstHand = o.hand;
            w.firstByHold = o.byHold;
            w.firstMs = in.nowMs >= w.since ? in.nowMs - w.since : 0;
        }
        ++w.presses;
    }
    w.refused += o.refused;
    w.refusedHeld += o.refused && o.refusedHeld;
    w.again += o.began && o.again;
    if (o.idle != Idle::kNone)
        w.idles |= 1u << static_cast<unsigned>(o.idle);
    if (w.lines < kWatchLines && (in.nowMs >= w.nextAt || in.nowMs < w.since)) {
        out.say = true;
        ++w.lines;
        w.nextAt = in.nowMs + kWatchEvery;
    }
    return out;
}

inline const char* IdleWord(Idle idle) {
    switch (idle) {
    case Idle::kOpen: return "neither grip squeezed past 0.6";
    case Idle::kUnread: return "neither grip read (the controllers asleep, or the session out of focus)";
    case Idle::kNotFresh: return "a grip held from before presses only a fight's prompt or in a scripted moment, and this is neither: let go and squeeze again";
    case Idle::kPlayKey: return "a grip held from before doesn't press play's own Fire, Jump or Interact, nor a single beside a fist fight, even in a scripted "
                                "moment: let go and squeeze again";
    case Idle::kSettle: return "a grip held from before waits 0.1 s for the game to take its trigger";
    case Idle::kAnswered: return "its press prompt's one press is made (the game's input showed it, or couldn't be read)";
    case Idle::kGap: return "a squeeze waits for the last press's key to have been up 60 ms";
    case Idle::kTapDone: return "the press under way has made its tap (one a squeeze, or a press prompt's one)";
    case Idle::kBetweenTaps: return "between two taps";
    case Idle::kSpent: return "a grip still squeezed whose press is over: let go and squeeze again";
    case Idle::kUnshown: return "its press prompt's tap made and never shown by the game's input (a key held down elsewhere, or a click held back): let go "
                                "and squeeze again, that presses it again";
    default: return "";
    }
}

// One grip as this frame has it, after Step2 ("squeezed, held from before (not seen let go since the keys came)"; "let go"; "not read").
inline const char* GripState(const In& in, const State2& s, int h) {
    if (!(in.read[h] && in.squeeze[h] == in.squeeze[h]))
        return "not read";
    const float v = in.squeeze[h];
    if (v < kLetGo)
        return "let go";
    if (v <= kPress)
        return s.pressed[h] ? "eased (its press goes on till it's under 0.4)" : "half squeezed (0.4 to 0.6: neither)";
    if (s.pressed[h])
        return s.fresh[h] ? "squeezed (fresh)" : "squeezed (taken as it was held)";
    return s.fresh[h] ? "squeezed" : "squeezed, held from before (not seen let go since the keys came)";
}

namespace detail {
inline void Add(char* text, unsigned size, unsigned& at, const char* format, ...) {
    if (at + 1 >= size)
        return;
    va_list args;
    va_start(args, format);
    const int n = std::vsnprintf(text + at, size - at, format, args);
    va_end(args);
    if (n > 0)
        at = at + static_cast<unsigned>(n) < size ? at + static_cast<unsigned>(n) : size - 1;
}
inline void What(char* text, unsigned size, unsigned& at, bool prompt, bool once, bool scripted) {
    Add(text, size, at, "%s, %s", !prompt ? "a single quick-time event" : once ? "a fight's press prompt" : "a fight's mash",
        scripted ? "a scripted moment" : "in play");
}
}  // namespace detail

// A line of the grips now ("E awaited 0.00 s (a fight's press prompt, a scripted moment): the left 0.02 let go, the right 0.97 squeezed (taken as it
// was held): E down (the right grip's press, tap 1)"; "...: nothing down: <why>").
inline const char* WatchText(const Watch2& w, const In& in, const State2& s, const Out& o, char* text, unsigned size) {
    if (!size)
        return "";
    text[0] = 0;
    unsigned at = 0;
    char keys[96];
    detail::Add(text, size, at, "%s awaited %.2f s (", KeyNames(in.await & kKeysV2, keys, sizeof(keys)),
                static_cast<double>(in.nowMs >= w.since ? in.nowMs - w.since : 0) / 1000.0);
    detail::What(text, size, at, in.prompt, in.once, in.cutscene || (in.turn && !in.turnArmed));
    detail::Add(text, size, at, "):");
    for (int h = 0; h < 2; ++h) {
        const bool read = in.read[h] && in.squeeze[h] == in.squeeze[h];
        if (read)
            detail::Add(text, size, at, "%s the %s %.2f %s", h ? "," : "", h ? "right" : "left", static_cast<double>(in.squeeze[h]), GripState(in, s, h));
        else
            detail::Add(text, size, at, "%s the %s not read", h ? "," : "", h ? "right" : "left");
    }
    if (o.keys)
        detail::Add(text, size, at, ": %s down", KeyNames(o.keys, keys, sizeof(keys)));
    else
        detail::Add(text, size, at, ": nothing down: %s", IdleWord(o.idle));
    if (s.on)
        detail::Add(text, size, at, " (the %s grip's press%s, tap %u)", s.hand == 0 ? "left" : "right", s.byHold ? ", taken as it was held" : "", s.taps);
    return text;
}

// A stretch over ("E awaited 0.41 s (a fight's press prompt, a scripted moment), over: 1 press, by the right grip taken as it was held, 0.00 s in; the
// left 0.00-0.05, the right 0.95-1.00"; "... over: nothing pressed (neither grip squeezed past 0.6); the left 0.00-0.55, the right not read").
inline const char* WatchEndText(const Watch2& w, char* text, unsigned size) {
    if (!size)
        return "";
    text[0] = 0;
    unsigned at = 0;
    char keys[96];
    detail::Add(text, size, at, "%s awaited %.2f s (", KeyNames(w.keys, keys, sizeof(keys)),
                static_cast<double>(w.lastMs >= w.since ? w.lastMs - w.since : 0) / 1000.0);
    detail::What(text, size, at, w.prompt, w.once, w.scripted);
    if (w.presses)
        detail::Add(text, size, at, "), over: %d press%s, the first by the %s grip %s, %.2f s in", w.presses, w.presses == 1 ? "" : "es",
                    w.firstHand == 0 ? "left" : "right", w.firstByHold ? "taken as it was held" : "freshly squeezed", static_cast<double>(w.firstMs) / 1000.0);
    else
        detail::Add(text, size, at, "), over: nothing pressed by the grips");
    if (w.idles) {
        detail::Add(text, size, at, " (nothing down at times: ");
        bool first = true;
        for (int i = 1; i < kIdleCount; ++i)
            if (w.idles >> i & 1) {
                detail::Add(text, size, at, "%s%s", first ? "" : "; ", IdleWord(static_cast<Idle>(i)));
                first = false;
            }
        detail::Add(text, size, at, ")");
    }
    if (w.again)
        detail::Add(text, size, at, "; pressed again %d time%s, the press before never shown by the game's input", w.again, w.again == 1 ? "" : "s");
    if (w.refused > w.refusedHeld)
        detail::Add(text, size, at, "; %d squeeze%s after its one press refused", w.refused - w.refusedHeld, w.refused - w.refusedHeld == 1 ? "" : "s");
    if (w.refusedHeld)
        detail::Add(text, size, at, "; a grip held on through a break in it (a frame not read clearly, the headset off a moment) not taken for a second press %d time%s",
                    w.refusedHeld, w.refusedHeld == 1 ? "" : "s");
    for (int h = 0; h < 2; ++h) {
        const char* name = h ? "right" : "left";
        if (w.unread[h] >= w.frames)
            detail::Add(text, size, at, "; the %s not read", name);
        else if (w.unread[h])
            detail::Add(text, size, at, "; the %s %.2f-%.2f (not read in %d of %d frames)", name, static_cast<double>(w.least[h]), static_cast<double>(w.most[h]),
                        w.unread[h], w.frames);
        else
            detail::Add(text, size, at, "; the %s %.2f-%.2f", name, static_cast<double>(w.least[h]), static_cast<double>(w.most[h]));
    }
    return text;
}

// ---- A fist fight's prompt and the other quick-time keys the game's input shows while it's up (qtewatch.cpp's lines) ----
// The owner's cellar fight, 2026-11-19: its first Fire prompt closed after 0.41 s with its right gate shut and its wrong gate left open, the fail gate
// shut: a wrong quick-time key's mark by the prefab's wiring (a timeout shuts both gates together, as both of The Great Destroyer's did), and the guard
// held up nothing of the mod's then. So every other quick-time key the game's input shows going down while a prompt is up, or on the frame it closes
// (a wrong key shuts its right gate in the game's same tick), is said: a key, button or controller outside the mod's senders. Each frame of the fights'
// reader, with the prompt's key (0: none up) and the keys the game's input shows (read: that state was read this frame).
struct Others {
    unsigned key = 0;   // the prompt's key (0: none up)
    unsigned down = 0;  // other quick-time keys the input showed down on its last frame read (down as it opened: held from before, no press of them)
    unsigned went = 0;  // ... those gone down while it was up
    int frames = 0, unread = 0;  // its frames, and those with the input not read
};
struct OthersOut {
    unsigned atOpen = 0;  // a prompt opened this frame: other keys already down in the input (held from before: no press of them)
    unsigned went = 0;    // other keys gone down this frame, the prompt up
    bool closed = false;  // the prompt closed this frame (or gave way to another key's):
    unsigned closedKey = 0;
    unsigned wentAsClosed = 0;  // ... other keys gone down on this very frame (perhaps what closed it)
    unsigned wentWhileUp = 0;   // ... every other key gone down while it was up (wentAsClosed not among them)
    int unread = 0, frames = 0; // ... its frames with the input not read, of all its frames
};
inline OthersOut OthersStep(Others& w, unsigned key, unsigned shown, bool read) {
    OthersOut o;
    shown = read ? shown & kKeysV2 : 0;
    if (w.key && w.key != key) {
        o.closed = true;
        o.closedKey = w.key;
        o.wentAsClosed = read ? shown & ~w.key & ~w.down & ~key : 0;  // (a next prompt's own key, with no frame between, is its answer)
        o.wentWhileUp = w.went;
        o.unread = w.unread;
        o.frames = w.frames;
        w = Others{};
    }
    if (!key)
        return o;
    if (!w.key) {
        w.key = key;
        w.frames = 1;
        w.unread = !read;
        w.down = o.atOpen = shown & ~key;
        return o;
    }
    ++w.frames;
    if (!read) {
        ++w.unread;  // (what was down stays as it was)
        return o;
    }
    o.went = shown & ~key & ~w.down;
    w.went |= o.went;
    w.down = shown & ~key;
    return o;
}

}  // namespace qte

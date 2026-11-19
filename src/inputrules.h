#pragma once

// input.cpp's and ammo.cpp's rules that need no game: a lone release of a mouse button owed to the game, how often the fire button's sends are said,
// whether a film round's trigger may fire real rounds, and X held waiting for the game's own reload. Header only, no game memory and no Windows calls,
// so tools/inputrulestest.cpp runs them as they are.

#include <cmath>
#include <cstdint>

namespace inputrules {

// ---- A lone release of each mouse button (input.cpp's KeepButtonsUp, once a frame) ----
//
// The game keeps a button down when its release never reached it (it went to another window, or came while the game wasn't reading): after a death on
// Operation Swordbreaker's roof the player's soldier fired his M4A1 by himself for ten minutes, 3,741 rounds into the floor beside a squadmate, with
// nobody in the headset, until one release sent with the game in front stopped it at once (2026-11-13). So a release of each button is owed to the
// game at every new soldier and whenever bf3 comes back to the front after not being there. It is sent alone as soon as bf3 is in front with the
// mod's own button up and no mouse button held by anyone else, and again kAgainMs after that while all of it still holds: in the first frames of a new
// soldier, or of its window back in front, the game may not be reading its mouse yet (the mod's own release at 13:22:33, in the death's scripted
// camera, never reached it), and a release sent only then would be lost the same way. A release of the mod's own that Windows refused (SendInput
// takes nothing while a window of higher rights or the secure desktop is in front) is owed too: that button then reads down to Windows as well (the
// mod's press never let go), which is the mod's, not a real button held. It never brings the game to the front.
constexpr int kLeft = 0, kRight = 1;  // the fire button, the zoom
enum class Why : uint8_t {
    kNone = 0,
    kSoldier = 1,  // a new soldier
    kFront = 2,    // bf3 back in front
    kRefused = 3,  // the mod's own release refused by Windows
};
constexpr int kAgainCount = 5;
constexpr unsigned long long kAgainMs[kAgainCount] = {250, 500, 1000, 2000, 4000};  // the sends after the first, from the first (ms)
struct ReleaseState {
    bool owed[2] = {};                          // the first release owed to the game, by button
    Why why[2] = {};                            // ... why
    int again[2] = {kAgainCount, kAgainCount};  // the next of kAgainMs still to send (kAgainCount: none)
    Why againWhy[2] = {};                       // ... their first's why
    unsigned long long firstAt[2] = {};         // when the first was sent (ms)
    unsigned soldier = 0;                       // the soldier's serial as last seen (0 before the first soldier)
    bool front = false;                         // bf3 in front at the last step
    bool started = false;                       // stepped once (the first step's front is no coming back)
};
struct ReleaseIn {
    unsigned long long nowMs = 0;  // the time (ms)
    unsigned soldier = 0;          // the player's soldier found anew this many times so far (god::SoldierSerial)
    bool front = false;            // bf3 in front now (the foreground window is the game's)
    bool modHolds[2] = {};         // the mod holds that button down: sent down, a trigger's or a film round's press, the grenade launcher's shot, the zoom held
    bool otherHeld = false;        // a mouse button down that the mod isn't holding (a real mouse's)
    bool refused[2] = {};          // the mod's own last release of that button refused by Windows and not sent since (input.cpp's SendMouseButton)
};
struct ReleaseOut {
    bool send[2] = {};         // that button's release, alone, to send now
    Why why[2] = {};           // ... what it's owed for
    bool again[2] = {};        // ... one of the sends after the first
    bool last[2] = {};         // ... the last of them
    bool newSoldier = false;   // owed this step for a new soldier
    bool backInFront = false;  // owed this step for bf3 back in front
};
inline ReleaseOut StepRelease(ReleaseState& s, const ReleaseIn& in) {
    ReleaseOut out;
    // (Owed afresh: its first again, the sends after it from that one. keep: an owing already waiting keeps its why.)
    const auto owe = [&s](int b, Why why, bool keep) {
        if (!keep || !s.owed[b])
            s.why[b] = why;
        s.owed[b] = true;
        s.again[b] = kAgainCount;
    };
    if (in.soldier != s.soldier) {
        s.soldier = in.soldier;
        out.newSoldier = true;
        for (int b = 0; b < 2; ++b)
            owe(b, Why::kSoldier, false);
    }
    if (s.started && in.front && !s.front) {
        out.backInFront = true;
        for (int b = 0; b < 2; ++b)
            owe(b, Why::kFront, true);
    }
    for (int b = 0; b < 2; ++b)
        if (in.refused[b] && !s.owed[b])
            owe(b, Why::kRefused, true);
    s.front = in.front;
    s.started = true;
    if (!in.front || in.otherHeld)
        return out;  // (kept owed: a release now would go to whatever is in front, or cut a real click short)
    for (int b = 0; b < 2; ++b) {
        if (in.modHolds[b])
            continue;  // (one the mod holds waits: its own release comes when it lets go, and this one after it)
        if (s.owed[b]) {
            out.send[b] = true;
            out.why[b] = s.why[b];
            s.owed[b] = false;
            s.againWhy[b] = s.why[b];
            s.why[b] = Why::kNone;
            s.firstAt[b] = in.nowMs;
            s.again[b] = 0;
            continue;
        }
        if (s.again[b] >= kAgainCount)
            continue;
        if (in.nowMs < s.firstAt[b]) {
            s.again[b] = kAgainCount;  // (the clock gone back: the rest dropped)
            continue;
        }
        if (in.nowMs - s.firstAt[b] < kAgainMs[s.again[b]])
            continue;
        out.send[b] = true;
        out.why[b] = s.againWhy[b];
        out.again[b] = true;
        // (One send for all that are due: a wait while the mod held the button, or a real one was held, isn't made up for. bf3 out of the front and back
        // owes a first afresh, above.)
        while (s.again[b] < kAgainCount && in.nowMs - s.firstAt[b] >= kAgainMs[s.again[b]])
            ++s.again[b];
        out.last[b] = s.again[b] >= kAgainCount;
    }
    return out;
}

// ---- The fire button's sends said in the log (input.cpp's SendMouseButton) ----
//
// Each down and up the mod sends is said with whether bf3 had the focus: at most kSayInFront lines a second for those that went to bf3 and kSayAway for
// those that didn't (rarer, and the ones that matter: a release that went to another window), so a long fight's taps can't flood the log (a flood of
// lines crashed the game once). Those not said are counted, and told in the next line said.
constexpr int kSayInFront = 2, kSayAway = 4;
struct SayState {
    unsigned long long secondFrom = 0;  // this second's start (ms; 0: none yet)
    int said[2] = {};                   // its lines: [0] bf3 in front, [1] not
    int quiet = 0, quietAway = 0;       // sends not said since the last line said, and of them with bf3 not in front
};
struct SayOut {
    bool say = false;
    int quiet = 0, quietAway = 0;  // to tell with it: the sends not said before it
};
inline SayOut StepSay(SayState& s, unsigned long long nowMs, bool front) {
    SayOut out;
    if (s.secondFrom == 0 || nowMs < s.secondFrom || nowMs - s.secondFrom >= 1000) {
        s.secondFrom = nowMs;
        s.said[0] = s.said[1] = 0;
    }
    const int k = front ? 0 : 1;
    if (s.said[k] < (front ? kSayInFront : kSayAway)) {
        ++s.said[k];
        out.say = true;
        out.quiet = s.quiet;
        out.quietAway = s.quietAway;
        s.quiet = s.quietAway = 0;
    } else {
        ++s.quiet;
        if (!front)
            ++s.quietAway;
    }
    return out;
}

// ---- A film round's trigger on the game's fire button (input.cpp's FireAlone) ----
//
// A film round's pull (build\freehands.txt "test N", hands.cpp's scripted hands) presses the game's fire button only when real rounds are asked for
// (freehands.txt "fire 1"; the pickup round, test 10, by pickups.txt's own "fire 1"), and then only by the rule for real rounds in a film: the gun in two
// hands and the head looking steeply down (the user's rule after a one-handed burst hit a squadmate). Otherwise the pull presses nothing, said once a
// pull: the self-test's round among them, which fires nothing. (weapon.txt "filmtrigger" is no round's: its own word and its own check of the head.)
constexpr float kRoundPitchMost = -45.0f * 3.14159265f / 180.0f;  // the stand-in's head at least this far down (radians, up positive)
// Why the pull fires nothing, or null when it may fire.
inline const char* RoundFireRefused(bool asked, bool twoHands, float pitch) {
    if (!asked)
        return "no \"fire 1\" in freehands.txt: a test round's pulls fire nothing";
    if (!twoHands)
        return "one hand on the gun: real rounds only with two";
    if (!(pitch <= kRoundPitchMost))
        return "the head not 45 degrees down: real rounds only into the ground";
    return nullptr;
}

// ---- X held with a gun in a hand: the game's own reload waited for (ammo.cpp's HeldPress and Update) ----
//
// Nothing of the reload is drawn at the hold itself: the magazine, the rifle's bolt and a held gun's chamber change when the game's count of that gun
// rises (a rise of any size: the last few spare rounds too). On Operation Swordbreaker's opening the game refused the reload, and six held presses drew
// a seated magazine in a gun that held one round (2026-11-13); on Rock and a Hard Place its count rose 1.4 s after each hold. No rise
// within kPressWaitMs (as long as the game's reload is still asked for after R: pickrules::kReloadAskedMs), or another gun in the hands: the wait ends
// with nothing changed. Each step is its own gun's: the bolt the M416's (reload.cpp's), the chamber a held gun's (hands.cpp's).
constexpr unsigned long long kPressWaitMs = 8000;
constexpr int kPressRifle = 0, kPressHeld = 1;  // what was drawn at the hold: the M416, a held gun
struct PressWait {
    unsigned long long at = 0;  // the hold (ms; 0: no wait)
    uint64_t gun = 0;           // the gun in the hands then (its firing state's address)
    int loaded = 0;             // its count then, as the mod counts it (for the line)
    int kind = kPressRifle;     // the M416 (its bolt) or a held gun (its chamber)
};
// What the game's rise draws for that hold: its magazine in; the M416's bolt home only for the M416, a held gun's chamber filled only for a held gun
// (another gun's reload never moves the M416's bolt left back in its slot, nor the other way round).
struct PressDraw {
    bool magazine = true, bolt = false, chamber = false;
};
inline PressDraw DrawAtRise(int kind) {
    PressDraw d;
    d.bolt = kind == kPressRifle;
    d.chamber = kind == kPressHeld;
    return d;
}
// While the wait runs the well shows empty (or the M416's bolt back), so the belly gives a fresh magazine; one pushed home by hand before the game's
// count rises (ammo.cpp's Insert) is the hand's reload, and the game's own, still under way, would land on top of it (a reload on top of the game's own
// made the M416's count 59, then 88: 2026-10-30). From then the game's reload that hold asked for counts as not asked for, so a rise of its count is
// undone (pickrules' kUndone: every round it took back to the spare ones), till that ask has run out (R let go kPressWaitMs ago, as
// pickrules::kReloadAskedMs), R is pressed again (a new ask), or another gun is in the hands.
struct HandOver {
    uint64_t gun = 0;   // the gun the hand loaded (0: none)
    bool rUp = false;   // R seen let go since
};
// This frame's "asked for" (ammo.cpp's, from R and the rest), with a hand's magazine taken into account; rSince: R's ms since let go (0 while down).
inline bool StillAsked(HandOver& h, bool asked, uint64_t gun, unsigned long long rSince) {
    if (!h.gun)
        return asked;
    if (gun != h.gun || rSince >= kPressWaitMs || (h.rUp && rSince == 0)) {
        h = HandOver{};
        return asked;
    }
    if (rSince > 0)
        h.rUp = true;
    return false;
}
enum class PressSeen : int {
    kNone = 0,      // no wait
    kWaiting = 1,   // still waiting
    kLoaded = 2,    // the game's count rose: the reload's picture now
    kGaveUp = 3,    // no rise in kPressWaitMs: nothing changed
    kOtherGun = 4,  // another gun in the hands: nothing changed
};
// One frame of the wait: the gun in the hands now (0: another, or a new one at the same address), and whether the game's count of it rose this frame (the
// game's own rise, not a count the mod wrote).
inline PressSeen StepPress(PressWait& w, unsigned long long nowMs, uint64_t gun, bool gameRose) {
    if (!w.at)
        return PressSeen::kNone;
    if (gun != w.gun) {
        w.at = 0;
        return PressSeen::kOtherGun;
    }
    if (gameRose) {
        w.at = 0;
        return PressSeen::kLoaded;
    }
    if (nowMs < w.at || nowMs - w.at >= kPressWaitMs) {
        w.at = 0;
        return PressSeen::kGaveUp;
    }
    return PressSeen::kWaiting;
}

// ---- The right stick's stance as the mod last put it (input.cpp's UpdateStance and Sprinting) ----
//
// The game never says its soldier's stance: the mod keeps the one the stick last put him in (X the game's crouch toggle, Z its prone toggle; a sprint or a
// jump stands him up), and from it whether the game's hands move on its gun by the stance or a sprint (input::Sprinting): nothing of the game's hold is
// learned from them then (the rifle's hold, a held gun's palms at rest: skin.cpp).
enum class Stance : uint8_t { kStand, kCrouch, kProne };
constexpr unsigned long long kSprintBlendMs = 1000;  // the game's sprint takes that long to blend back after it's let go
constexpr unsigned long long kStanceBlendMs = 1500;  // and a stance change (its prone and its transitions, 0.3-1.05 s in its data)
// Whether the game's hands move on its gun by the stance or a sprint now: a sprint held, and kSprintBlendMs after it; prone; kStanceBlendMs after a change.
inline bool HandsMovedByStance(unsigned long long nowMs, unsigned long long sprintAt, bool prone, unsigned long long stanceChangedAt) {
    return nowMs - sprintAt < kSprintBlendMs || prone || nowMs - stanceChangedAt < kStanceBlendMs;
}
// Each frame, with the player's soldier's serial (god::SoldierSerial; seen: the serial it was last called with). A new soldier (a life, a checkpoint, a
// level) is the game's own, made standing with both its toggles off: the stance kept goes back to standing, the crouch toggle off. Kept through a death
// instead (Operation Swordbreaker, 2026-11-19: prone at 00:59:18, killed at 00:59:34), it said prone for the whole of the next life: input::Sprinting held
// the game's hands as moving on every gun drawn in it, none of their rests was taken, so no magazine's seat either, and a magazine out by hand was drawn
// nowhere in the hand (skin.cpp; tools\magseattest.cpp). True when it changed (to be said).
inline bool StanceForSoldier(unsigned& seen, unsigned soldier, Stance& stance, bool& crouchOn) {
    if (soldier == seen)
        return false;
    seen = soldier;
    const bool was = stance != Stance::kStand || crouchOn;
    stance = Stance::kStand;
    crouchOn = false;
    return was;
}

}  // namespace inputrules

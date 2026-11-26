#pragma once

// input.cpp's and ammo.cpp's rules that need no game: a lone release of a mouse button owed to the game, how often the fire button's sends are said,
// whether a film round's trigger may fire real rounds, and X held waiting for the game's own reload; and xr_runtime.cpp's Page Up and Page Down (a step
// of the player's view only from a key held with the headset on). Header only, no game memory and no Windows calls, so tools/inputrulestest.cpp runs
// them as they are.

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

// ---- Page Up and Page Down: a step of the player's view (xr_runtime.cpp's ViewKeyStep, once a frame for each) ----
//
// Each step moves the view 5 degrees (with the wide view on its angle, otherwise the zoom of the headset's picture) and is kept in bf3vr.player.ini, which
// no update replaces and which is read over bf3vr.ini at every start: a player fixed his double vision with them (the audit of 2026-11-19). The mod sees
// every key on the PC (GetAsyncKeyState), and on 2026-11-18 (08:15:57-08:16:25) a keyboard knocked off the desk, the headset just taken off and the game
// in front, pressed F10, F11 and F12 by itself. Counted as d32a39b counted them, with the game's window in front alone, each knock on these keys would have
// moved the view for good: one Page Down from the 105.9 degrees shipped brings back the black at the edges the owner called very noticeable at 100, one
// Page Up softens the picture about 9% (the integration review of 2026-11-25). So a step needs the key held kViewHoldMs, with the game's window in front and
// the VR session focused (state 5: somebody wears the headset) all through the hold. The owner's runtime drops the session to 3 as the headset comes off
// (2026-11-18 08:15:08, 2026-11-19 01:23:15); one without a proximity sensor keeps it focused, and there the hold is what keeps a knock or a bouncing key
// from stepping the view. One step a hold, at kViewHoldMs: held on, nothing more (let go and hold again for the next), so a keyboard lying on the key moves
// it once at most, and a hold begun with the headset off never moves it, the headset put on during it or not. No Ctrl with them: Left Ctrl is the game's
// crouch.
constexpr unsigned long long kViewHoldMs = 500;
// The key is read once a frame, so between two frames that both see it down it may have come up and gone down again unseen. Frames further apart than
// kViewGapMs (a load's stall: the owner's log of 2026-11-19 has seconds of 1 to 4 frames) count the hold afresh from the later one: two knocks of the
// keyboard 0.6 s apart, each seen by one frame across such a gap, were one hold of 0.6 s and stepped the view for good (the review of the keys' lane,
// 2026-11-25). A gap never gives a hold back: one that stepped or was refused stays so until a frame sees the key up. (Under 10 frames a second, then,
// a hold never steps: not a rate anybody tunes the view at.)
constexpr unsigned long long kViewGapMs = 100;
enum class ViewKeyNo : uint8_t {
    kNone = 0,
    kNotFront = 1,    // another window was in front during the hold
    kNotFocused = 2,  // the VR session wasn't focused during the hold (the headset off, the VR runtime's own menu up or gone, no headset)
    kShort = 3,       // let go before kViewHoldMs
};
constexpr int kViewKeyNos = 4;
struct ViewKeyState {
    bool down = false;                     // the key down at the last step
    unsigned long long downAt = 0;         // since when (ms): the hold counted from
    unsigned long long seenAt = 0;         // the last frame that saw it down (ms)
    bool stepped = false;                  // this hold has stepped the view
    ViewKeyNo spoiled = ViewKeyNo::kNone;  // this hold can no longer step it, and why
};
struct ViewKeyOut {
    bool step = false;                     // step the view now (and keep it in bf3vr.player.ini)
    ViewKeyNo refused = ViewKeyNo::kNone;  // this hold refused at this step, and why: once a hold, at the frame it's known
};
// One frame: the time (ms), the key down now, and while it's down the game's window in front and the VR session focused (SessionFocused).
inline ViewKeyOut StepViewKey(ViewKeyState& s, unsigned long long nowMs, bool down, bool front, bool focused) {
    ViewKeyOut out;
    if (!down) {
        if (s.down && !s.stepped && s.spoiled == ViewKeyNo::kNone)
            out.refused = ViewKeyNo::kShort;
        s = ViewKeyState{};
        return out;
    }
    // The hold counted from now: its first frame down, a gap in the frames over kViewGapMs, the clock gone back.
    if (!s.down || nowMs < s.seenAt || nowMs - s.seenAt > kViewGapMs)
        s.downAt = nowMs;
    s.down = true;
    s.seenAt = nowMs;
    if (s.stepped || s.spoiled != ViewKeyNo::kNone)
        return out;
    if (!front || !focused) {
        s.spoiled = !front ? ViewKeyNo::kNotFront : ViewKeyNo::kNotFocused;
        out.refused = s.spoiled;
        return out;
    }
    if (nowMs - s.downAt >= kViewHoldMs) {
        s.stepped = true;
        out.step = true;
    }
    return out;
}

// StepViewKey's "focused": the session's last state FOCUSED (5), the session running and not lost, and a frame begun (xrWaitFrame and xrBeginFrame both
// through) within kViewBegunMs. The state alone is the last event's, and it goes stale: an instance lost sets the mod's failed flag, after which no event
// is read at all, and a VR runtime gone without a word leaves it at 5 while xrWaitFrame fails every frame; the keys are read at every Present all the
// same, so with SteamVR closed or crashed in play a hold still stepped the view and kept it in bf3vr.player.ini as "held with the headset on" (the review
// of the keys' lane, 2026-11-25). lastBegunMs 0: no frame begun yet. A load's stall of a second or more counts as not focused too (that hold can't
// step; held again after it, it can).
constexpr int kSessionFocused = 5;  // XR_SESSION_STATE_FOCUSED (xr_runtime.cpp asserts it)
constexpr unsigned long long kViewBegunMs = 1000;
inline bool SessionFocused(int state, bool running, bool failed, unsigned long long nowMs, unsigned long long lastBegunMs) {
    if (state != kSessionFocused || !running || failed || !lastBegunMs)
        return false;
    return nowMs <= lastBegunMs || nowMs - lastBegunMs < kViewBegunMs;
}

// The two keys' steps of one frame as the view's: +1 Page Up's, -1 Page Down's, 0 for neither and for both at once. Held together, both holds step at the
// same frame, and the old line (Page Down's -5 whenever it stepped) kept Page Down's alone with Page Up's hold used up: a keyboard lying on both moved the
// view 5 degrees narrower for good, the black at the edges back (the review of the keys' lane, 2026-11-25). Both at once now move nothing; each hold is
// used up all the same (no step again until it's let go).
inline int ViewKeysStep(bool upStep, bool downStep) {
    return upStep == downStep ? 0 : upStep ? 1 : -1;
}

// ---- The right stick's click: a jump, the moment it's pressed with the soldier standing; held a recenter (input.cpp's Sync, once a frame) ----
//
// The user's design (2026-11-05): the click jumps, held it recenters the view with no jump, and a click with the stick pushed over is no jump (pushing
// it down to crouch or go prone, or turning). Until 2026-11-26 the jump went as the click was let go, once it had stayed let go 50 ms, and only for a
// click let go within 0.3 s with the stick under half pushed all through it: a quick click jumped 0.1 to 0.2 s after it went down, while A jumps the
// moment it's pressed (the owner, in the headset: "the way it's fast when you press A to jump ... zero latency or delay. It should be like that with
// the controllers too"). Now, with the soldier standing (ClickIn::standing: stance.h's Standing, on the game's own pose), the jump goes the frame the
// click is first read down, as A's does, through the jump's own mapping (Space held kJumpPressMs):
//   - the stick still at its middle: pushed under kJumpStillMost and moved since the frame before by under kJumpStillPerSecond's share of the frame (a
//     click's own small wobble); pushed over (kJumpStickMost) now or the frame before, no jump (a crouch, prone or a turn held, or coming back);
//   - the stick moving as the click goes down (a push or a flick beginning, or a click's bigger wobble): watched kJumpWatchMs (three frames at 72 Hz):
//     past kJumpStickMost in them, no jump (a push or a flick under way: the slow push the user's design was made for, 2026-11-05); still again, the
//     jump then; still moving at their end, decided as before 2026-11-26, as it's let go;
//   - a fresh click: let go at least kRecenterSlipMs before. A thumb slipping off the click for a moment while it's held to recenter is the same hold:
//     no second jump, and the recenter's wait goes on.
// Crouched or prone (or on the way there, or the head's real crouch holding the game's crouch-and-hold), and in a cutscene's camera, the jump goes as
// the click is let go, as before 2026-11-26: let go within kJumpClickMs, the stick under kJumpStickMost all through it, and stayed let go
// kJumpSettleMs (pushed over meanwhile, none). The game's jump stands a crouched or prone soldier up (ProneToStandOnJump; from a crouch the same), so
// a jump at the press stood him up at every recenter (the review of 32b5f2a): held to recenter there nothing is sent, as the user's design has it,
// and a quick click still stands him up (the flick up does it at once). In a cutscene a quick click still answers a prompt that wants Space (the
// cellar fight's JumpClimb, the interrogation's first), and held it recenters with no key sent.
// So in play, standing, a click held to recenter hops once as it goes down (nothing at the press tells a jump from a hold's start), and the view
// recenters once it has been held kRecenterHoldMs, 1 s (was 1.5; the owner: "maybe like 1 second or 2 seconds, preferably one"), once a hold, a slip of
// the thumb under kRecenterSlipMs not starting the wait again. On a turning trackpad (the Vive's wands) only a click in its middle counts towards the
// recenter, and the push is the trackpad's place as PadTurn reads it (xrprofiles.h: 0 in its middle, so a click there is still).
constexpr unsigned long long kRecenterHoldMs = 1000, kRecenterSlipMs = 200, kJumpPressMs = 150;
constexpr unsigned long long kJumpClickMs = 300, kJumpSettleMs = 50;  // the jump as it's let go: let go within this, then stayed let go this (f1552ad's)
constexpr unsigned long long kJumpWatchMs = 42;                       // a stick moving as the click goes down: watched this long
constexpr float kJumpStickMost = 0.5f;        // the stick pushed further than this at the click: a crouch, prone or a turn, no jump
constexpr float kJumpStillMost = 0.3f;        // ... the jump at the press only with it pushed under this,
constexpr float kJumpStillPerSecond = 3.6f;   // ... and moving under this (0.05 a 72 Hz frame; a slow push for prone is 4 to 10, a flick 20 to 40)
constexpr float kJumpFrameMs = 13.9f;         // a frame's length when none is given (72 Hz)
enum class ClickMode : uint8_t {
    kNone = 0,   // no press under way
    kWatch = 1,  // the stick moving as the click went down: watched
    kLetGo = 2,  // decided as the click is let go
    kDone = 3,   // decided (a jump, or none)
};
enum class ClickWhy : uint8_t {
    kNone = 0,
    kMoving = 1,       // the stick still moving when the watch ended
    kNotStanding = 2,  // crouched or prone (the game's jump would stand him up)
    kCutscene = 3,     // a cutscene's camera
};
struct ClickState {
    bool held = false;               // the click as last read
    bool started = false;            // stepped once (a click held at the first read is no press: no jump)
    bool letGoSeen = false;          // let go after a press at least once
    unsigned long long letGoAt = 0;  // its last let-go (ms)
    float lastPush = 0.0f;           // the stick's push as last read
    unsigned long long holdSince = 0, holdSeenAt = 0;  // the recenter's hold: since when, and its last frame counted
    bool recentered = false;                         // this hold has recentered
    // The press under way: its jump decided at it, watched a moment, or as it's let go.
    ClickMode mode = ClickMode::kNone;
    ClickWhy why = ClickWhy::kNone;  // why it waits for its let-go
    unsigned long long pressAt = 0;
    float most = 0.0f;         // the stick's largest push from the frame before the press on (while it's held)
    bool jumpWaiting = false;  // let go in time: the jump once it has stayed let go kJumpSettleMs
};
struct ClickIn {
    unsigned long long nowMs = 0;
    bool held = false;        // the click read down this frame
    float push = 0.0f;        // the stick's push this frame (0-1; a turning trackpad's place as PadTurn reads it)
    bool forRecenter = true;  // the held click counts towards a recenter (xrprofiles::HeldForRecenter: on a turning trackpad, only in its middle)
    bool cutscene = false;    // a cutscene's camera (input::InCutscene)
    bool standing = true;     // the soldier standing, nothing else asked of it (stance::Standing): false, the jump as the click's let go
    float frameMs = 0.0f;     // this frame's length (the frames' display times apart; 0: kJumpFrameMs)
};
enum class ClickNo : uint8_t {
    kNone = 0,
    kPushed = 1,    // the stick pushed over: at the press or the frame before, while watched, or (decided as it's let go) while it was held or after
    kSlip = 2,      // pressed again under kRecenterSlipMs after a let-go: the same hold
    kHeldFrom = 4,  // held already at the first read
};
struct ClickOut {
    bool jump = false;            // press the jump now (Space for kJumpPressMs)
    bool recenter = false;        // recenter the view now
    ClickNo no = ClickNo::kNone;  // a press that didn't jump, and why (said)
    float push = 0.0f;            // ... the largest push it was refused at
    ClickWhy late = ClickWhy::kNone;  // a jump decided after the press: kNone watched (still again), else as it was let go, and why
    unsigned long long afterMs = 0;   // ... how long after the press
};
// The stick's push as the click takes it: not a number taken as pushed all the way (never a jump).
inline float ClickPush(float push) {
    return push == push ? (push < 0.0f ? 0.0f : push) : 1.0f;
}
inline ClickOut StepClick(ClickState& s, const ClickIn& in) {
    ClickOut out;
    const unsigned long long now = in.nowMs;
    const float push = ClickPush(in.push);
    const float frame = in.frameMs > 0.0f && in.frameMs == in.frameMs ? (in.frameMs < 4.0f ? 4.0f : in.frameMs > 30.0f ? 30.0f : in.frameMs) : kJumpFrameMs;
    const float stillStep = kJumpStillPerSecond * frame / 1000.0f;  // (the most a still stick moves in this frame)
    const float before = s.started ? s.lastPush : push;
    const bool still = push < kJumpStillMost && std::fabs(push - before) < stillStep;
    const auto waitFor = [&](ClickWhy why) {
        s.mode = ClickMode::kLetGo;
        s.why = why;
    };
    if (in.held && !s.held) {
        s.jumpWaiting = false;  // (a jump waiting out its settle: this press is the same hold's, as f1552ad's rule had it)
        if (!s.started) {
            out.no = ClickNo::kHeldFrom;
            s.mode = ClickMode::kDone;
        } else if (s.letGoSeen && now >= s.letGoAt && now - s.letGoAt < kRecenterSlipMs) {
            out.no = ClickNo::kSlip;  // (the press under way goes on: one decided stays so, one decided as it's let go waits for this let-go)
        } else {
            s.pressAt = now;
            s.most = push > before ? push : before;
            s.why = ClickWhy::kNone;
            if (!(s.most < kJumpStickMost)) {
                out.no = ClickNo::kPushed;
                out.push = s.most;
                s.mode = ClickMode::kDone;
            } else if (in.cutscene) {
                waitFor(ClickWhy::kCutscene);
            } else if (!in.standing) {
                waitFor(ClickWhy::kNotStanding);
            } else if (still) {
                out.jump = true;  // the frame it goes down, as A's
                s.mode = ClickMode::kDone;
            } else {
                s.mode = ClickMode::kWatch;
            }
        }
    } else if (s.mode == ClickMode::kWatch) {
        // A moving stick's next frames, the click held or let go: over half, a push or a flick (no jump); still again, the jump; else as it's let go.
        s.most = push > s.most ? push : s.most;
        if (!(push < kJumpStickMost)) {
            out.no = ClickNo::kPushed;
            out.push = s.most;
            s.mode = ClickMode::kDone;
        } else if (in.cutscene) {
            waitFor(ClickWhy::kCutscene);
        } else if (!in.standing) {
            waitFor(ClickWhy::kNotStanding);
        } else if (still) {
            out.jump = true;
            out.afterMs = now >= s.pressAt ? now - s.pressAt : 0;
            s.mode = ClickMode::kDone;
        } else if (now < s.pressAt || now - s.pressAt >= kJumpWatchMs) {
            waitFor(ClickWhy::kMoving);
        }
    } else if (s.mode == ClickMode::kLetGo && in.held) {
        s.most = push > s.most ? push : s.most;
    }
    if (!in.held && s.held) {
        s.letGoAt = now;
        s.letGoSeen = true;
        if (s.mode == ClickMode::kWatch)
            waitFor(ClickWhy::kMoving);  // (let go while watched: as it's let go)
        if (s.mode == ClickMode::kLetGo) {
            // As before 2026-11-26: let go quickly, the stick under half pushed all through it, and not a hold that recentered.
            const bool quick = now >= s.pressAt && now - s.pressAt < kJumpClickMs;
            s.jumpWaiting = quick && s.most < kJumpStickMost && !s.recentered;
            if (quick && !(s.most < kJumpStickMost)) {
                out.no = ClickNo::kPushed;
                out.push = s.most;
            }
            if (!s.jumpWaiting)
                s.mode = ClickMode::kDone;
        }
    }
    if (s.jumpWaiting && !in.held) {
        if (!(push < kJumpStickMost)) {
            s.jumpWaiting = false;  // (pushed over before it went: a crouch, prone or a turn under way)
            s.mode = ClickMode::kDone;
            out.no = ClickNo::kPushed;
            out.push = push;
        } else if (now < s.letGoAt || now - s.letGoAt >= kJumpSettleMs) {
            s.jumpWaiting = false;
            s.mode = ClickMode::kDone;
            out.jump = true;
            out.late = s.why == ClickWhy::kNone ? ClickWhy::kMoving : s.why;
            out.afterMs = now >= s.pressAt ? now - s.pressAt : 0;
        }
    }
    // The recenter: held kRecenterHoldMs, a slip under kRecenterSlipMs not starting it again, once a hold.
    const bool holding = in.held && in.forRecenter;
    if (holding)
        s.holdSeenAt = now;
    if (!holding && (now < s.holdSeenAt || now - s.holdSeenAt > kRecenterSlipMs)) {
        s.holdSince = 0;
        s.recentered = false;
    } else if (holding && !s.holdSince) {
        s.holdSince = now;
    } else if (holding && s.holdSince && !s.recentered && now >= s.holdSince && now - s.holdSince >= kRecenterHoldMs) {
        s.recentered = true;
        out.recenter = true;
    }
    s.held = in.held;
    s.lastPush = push;
    s.started = true;
    return out;
}

}  // namespace inputrules

#pragma once
// Holsters with free hands, worked out on their own so tools/holstertest.cpp runs them without the game: no game calls and no logs (an Event
// says what happened and why, and Say words it, as the pouch's note does). hands.cpp builds the Frame each frame from what it has (the hands, the
// grasp as the last frame left it, the body as the pouch placed it, the game's slots), calls Update before grasp::Update (what the grasp is told:
// stows, stowSide, drawSide, hold; what the game is told: a key) and Settle after it with what the grasp did (the clicks, the sound, the lines,
// a refusal that waited for a take), and keeps the State outside the grasp's Clear. Arrive answers its OnGunChanged: where the game's new weapon
// goes while the gun is put away. Everything by side, [0] the left and [1] the right.
//
// The user's layout (2026-11-05): "Right shoulder should be primary holster. Left shoulder should be secondary holster. Whatever secondary you
// have"; launchers behind the neck, else the left rib cage; grenades on the left chest, C4 and claymores on the right, every gun's ammo at the
// belly; nothing ever drawn at any of them. The places are eggs as the belly's is (ammozone.h: Size 1 at the edge), measured from the pouch's neck
// point in the body's axes (right, up, forward: pouch::BodyFrame), the left the right's mirror:
//   the right shoulder    the primary: the gun in slot 0
//   the left shoulder     the secondary: the gun in slot 1 (a gun taken off the ground that the game puts there too)
//   the back of the neck  every launcher by its kind (the SMAW, the Stinger, the AT4, the Javelin) whatever its slot, and slot 5's gun
//   the left rib cage     the same instead, with holsters.txt "launcher rib"
//   the chests            reserved: they answer nothing yet
// "mirror 1" swaps the shoulders and puts the rib on the right. Either hand puts away and draws, and the hand that draws is the trigger hand.
//   - Put away: the hand holding the game's weapon (by its pistol grip, or by its handguard alone) opens at its place, or at any holster's place
//     ("anyholster 1"), slower than "stowspeed" (faster is a throw). Not with the other hand on its handguard (the hold wins), in a scripted moment,
//     in the game's reload or the grenade launcher's shot, nor while the other hand holds a rocket, a case or a 40 mm round (they ride the gun's own
//     draw): the let-go is then today's. The other hand's magazine falls, or goes back at the belly ("heldmag fall").
//   - Drawn: an empty hand's grip crossing 0.6 from under 0.4 at a place, the hand slower than "draw"'s speed within its seconds of the crossing (a
//     hand closing on its way over the shoulder to the neck draws nothing from the shoulder), where a gun is at its holster: the game's weapon put
//     away (case A: in the hand at once, no key), or another slot's (case B: the game's own switch by its key, the hand keeping the coming gun 1.5 s
//     and the gun in it from its first drawn frame). The user, 2026-11-13: "After a draw, it checks the game really switched guns, presses again if
//     not, and tells you when it failed. That's very important": the key once more at 0.6 s if the game's own slot hasn't moved and no key could do
//     harm then, given up at 1.5 s (KeyAgain). The squeeze is that draw's while it waits (the grasp takes nothing with it), and a pick of
//     another slot calls it off. Nothing is decided on a game slot unread past a moment (a load, a death).
//   - Refused: a double click and a line, only with both hands empty; with the other hand holding the gun the places stay silent. A refusal never
//     stands in for a take: while something lies to be taken, it waits out the take's window first.
//   - A hand not seen (the runtime's own word this frame, from the loss's first frame on: never input's 0.5 s held place) is presumed at the
//     launcher's place it was last seen in or going into (the shoulders too with "unseenshoulders 1"); it may put its gun away there by opening, or
//     take there by a fresh squeeze, within strict limits ("unseen"), and a gun so taken waits put away until the hand is seen again. Any other
//     let-go by a hand not seen drops nothing ("unseendrop 0": a tracking loss never drops a gun, the user's rule).
//   - A 20 ms tick on a hand entering a place where it can act: once an entry, never on the trigger hand while it aims.
//   - "holsters 0" with a gun put away brings it back into the right hand as at a spawn; otherwise it is today's in everything.

#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ammozone.h"

namespace holster {

constexpr int kSlots = 10;
constexpr float kArm = 0.4f, kTake = 0.6f, kLetGo = 0.35f;  // the grip's, as grasp.h's
constexpr int kLetGoFrames = 3;                               // frames of its reading under kLetGo: it has opened
constexpr float kReachWindow = 0.4f;   // s a take's squeeze stays fresh (grasp.h's): a refusal waits it out while something lies to be taken
constexpr float kRetakeAfter = 0.25f;  // s after its own let-go a hand takes nothing (grasp.h's)
constexpr float kPickKeeps = 2.0f;     // s a pick (the D-pad, Y held, X held's pickup) sends the game's new weapon to the right hand
constexpr float kArriveMost = 3.0f;    // s a draw waits at most for the game's new weapon to come (then it comes as the game's own switch)
constexpr float kAskMost = 0.25f;      // s a draw or a put-away asked of the grasp is asked again each frame till it's done (then it lapses)
constexpr float kLeaveSize = 1.1f;     // a hand in a place has left it past this Size (the tick's: an edge doesn't tick twice)
constexpr float kAhead = 0.1f;         // s on: a hand not seen, moving, stays at the place it's in if it's still in it this much later
constexpr float kGuessGrow = 0.2f;     // m: the runtime's guess for a hand not seen within its place grown by this, or the place is given up
constexpr float kSlotStale = 0.5f;     // s the game's own slot may go unread (a moment's gap) before nothing is decided on its last read (a load, a death)
constexpr float kLogNear = 0.5f;       // m from the neck point: "log 1" says a hand's place once a second within it
constexpr float kPi = 3.14159265f;
// The pulses (input::Buzz): a click on a put-away, a draw, a draw asked of the game; a tick on an entry; a refusal's two short pulses.
constexpr float kClickAmp = 0.5f, kClickSeconds = 0.02f;
constexpr float kTickAmp = 0.25f, kTickSeconds = 0.02f;
constexpr float kDoubleAmp = 0.5f, kDoubleSeconds = 0.015f, kDoubleGap = 0.06f;  // the second 60 ms after the first

enum class Place : uint8_t { kNone, kRightShoulder, kLeftShoulder, kNeck, kLeftRib, kRightRib, kLeftChest, kRightChest, kBelly };
constexpr int kPlaces = 9;

// The grasp's holder as the last frame left it (grasp.h's, with its kStowed: put away at its place).
enum class Holder : uint8_t { kHeld, kBarrel, kGlide, kLoose, kStowed };
// A controller this frame, raw (before input's 0.5 s hold): its place tracked and both valid; valid but not tracked (the runtime's own guess);
// neither.
enum class Seen : uint8_t { kNone, kGuessed, kSeen };

enum class Launcher : uint8_t { kNeck, kRib, kOff, kShoulder };
enum class StowSound : uint8_t { kVest, kSettle, kNone };

struct Egg {
    float mid[3] = {};
    float half[3] = {};
};

// build\holsters.txt (Parse; every line optional, no file: every default; read again whole when it changes).
struct Settings {
    bool on = true;                                                // holsters 0: every let-go and squeeze exactly as today
    Launcher launcher = Launcher::kShoulder;                       // launcher shoulder | neck | rib | off (the user, 2026-11-15: the left shoulder)
    // The shoulder and the back of the neck from the user's own reach test (2026-11-13 16:48-16:52, seated, every reach seen by the headset): the tops of
    // the shoulders squeezed 19-22 cm out, 2-3 cm down and 1-5 cm behind the neck point; behind the neck 5 cm left to 3 cm right, 3-12 cm up and 12-19 cm
    // behind it, over either shoulder. (The design's first guesses were 6-7 cm lower and 5 cm further forward.) Sized so every one of those squeezes
    // sits within 0.6 of its place's Size, and the shoulder's front 3 cm short of a left squeeze the user made in play over the left shoulder.
    Egg shoulder{{0.20f, -0.025f, -0.03f}, {0.07f, 0.08f, 0.07f}};  // the right one (middle, half-sizes, m); the left mirrored
    Egg neck{{0.0f, 0.07f, -0.155f}, {0.12f, 0.10f, 0.09f}};
    Egg rib{{-0.19f, -0.30f, -0.03f}, {0.06f, 0.09f, 0.07f}};      // the left one
    float stowSpeed = 1.5f;                                        // m/s: a let-go faster is a throw
    bool anyHolster = true;                                        // 0: only the gun's own place puts it away
    bool otherGunStow = false;                                     // othergun refuse | stow: a draw while the other hand holds the gun
    bool lyingRecall = false;                                      // lying refuse | recall: a draw while the game's weapon lies
    bool twoLaunchersBySlot = false;                               // twolaunchers last | slot: two launchers carried, one in slot 0 or 1 by its slot
    float vanish = 0.0f;                                           // s the open hand rests at the place before the gun goes
    bool unknownGame = false;                                      // unknown refuse | game: a gun the hands don't know drawn the game's way
    bool gameSwitchHand = false;                                   // gameswitch stay | hand: the game switching by itself while put away
    bool unseen = true;                                            // unseen 0: no take or put-away by a hand not seen
    float unseenTake = 1.5f, unseenStow = 4.0f;                    // s not seen at most: a take, a put-away
    float approach = 0.15f, leastSpeed = 0.15f, degrees = 60.0f;   // m outside a place going into it, at least m/s, within degrees of its middle
    float guessMost = 0.30f, above = 0.03f;                        // m the runtime's guess from the last seen place; m over the place's top
    bool unseenShoulders = false;                                  // unseenshoulders 1: the shoulders presumed too
    bool unseenDrop = false;                                       // unseendrop 1: a let-go within input's 0.5 s hold drops, as before
    float drawWindow = 0.10f, drawSpeed = 0.40f;                   // draw: s after the grip's crossing, m/s the hand's most
    StowSound stowSound = StowSound::kVest;                        // stowsound vest | settle | none
    bool stowPanSide = false;                                      // stowpan middle | side
    float buzz = 1.0f;                                             // the pulses' strength: 0 none
    bool mirror = false;
    bool log = false;                                              // log 1: a hand near the neck point said once a second; the quiet refusals said
    bool heldMagRefuse = false;                                    // heldmag fall | refuse: a put-away while the other hand holds its magazine
    float keyAgain = 0.6f, keyGiveUp = 1.5f;                       // keyagain: s to the second key, s to giving up
};

// ---- The places ----

constexpr Egg kLeftChestEgg{{-0.08f, -0.18f, 0.12f}, {0.06f, 0.05f, 0.09f}};  // grenades (reserved); the right chest its mirror

inline Egg Mirrored(const Egg& e) {
    Egg m = e;
    m.mid[0] = -e.mid[0];
    return m;
}

inline Egg EggOf(Place p, const Settings& s) {
    switch (p) {
    case Place::kRightShoulder: return s.shoulder;
    case Place::kLeftShoulder: return Mirrored(s.shoulder);
    case Place::kNeck: return s.neck;
    case Place::kLeftRib: return s.rib;
    case Place::kRightRib: return Mirrored(s.rib);
    case Place::kLeftChest: return kLeftChestEgg;
    case Place::kRightChest: return Mirrored(kLeftChestEgg);
    case Place::kBelly: {
        Egg e;
        for (int i = 0; i < 3; ++i)
            e.mid[i] = ammozone::kBelly[i], e.half[i] = ammozone::kBellyRadii[i];
        return e;
    }
    default: return Egg{};
    }
}

// How far out of an egg a point is, each way by its own size grown by more: 1 its edge, under 1 inside (1e9 for a broken egg or point).
inline float Size(const Egg& e, const float p[3], float more = 0.0f) {
    float sum = 0.0f;
    for (int i = 0; i < 3; ++i) {
        const float r = e.half[i] + more;
        if (!(r > 0.0f))
            return 1e9f;
        const float d = (p[i] - e.mid[i]) / r;
        sum += d * d;
    }
    const float v = std::sqrt(sum);
    return std::isfinite(v) ? v : 1e9f;
}

inline Place PrimaryPlace(const Settings& s) { return s.mirror ? Place::kLeftShoulder : Place::kRightShoulder; }
inline Place SecondaryPlace(const Settings& s) { return s.mirror ? Place::kRightShoulder : Place::kLeftShoulder; }
// The launcher's place. "launcher shoulder" (the default since 2026-11-15, the user: "make sure primaries go on the right and secondaries go
// on the left. So that means the SMAW should go on the left shoulder"): the secondary's shoulder; behind the neck instead (LauncherFallback)
// when a gun of slot 1 has that shoulder (PlacesOf decides it from the kit).
inline Place LauncherPlace(const Settings& s) {
    return s.launcher == Launcher::kShoulder ? SecondaryPlace(s)
           : s.launcher == Launcher::kNeck   ? Place::kNeck
           : s.launcher == Launcher::kRib    ? (s.mirror ? Place::kRightRib : Place::kLeftRib)
                                             : Place::kNone;
}
// Where a launcher goes with "launcher shoulder" when slot 1's gun has the secondary's shoulder: behind the neck; kNone otherwise.
inline Place LauncherFallback(const Settings& s) { return s.launcher == Launcher::kShoulder ? Place::kNeck : Place::kNone; }
// A place that holds guns with these settings: both shoulders, the launcher's, and its fallback.
inline bool Holds(Place p, const Settings& s) {
    return p == Place::kRightShoulder || p == Place::kLeftShoulder || (p != Place::kNone && (p == LauncherPlace(s) || p == LauncherFallback(s)));
}

// The holster place a point (from the neck point, body axes) is in, its egg grown by more: the smaller Size where two meet; kNone. size: its Size.
inline Place PlaceAt(const float p[3], const Settings& s, float* size = nullptr, float more = 0.0f) {
    const Place list[4] = {Place::kRightShoulder, Place::kLeftShoulder, LauncherPlace(s), LauncherFallback(s)};
    Place best = Place::kNone;
    float bestSize = 1e9f;
    for (const Place q : list) {
        if (q == Place::kNone)
            continue;
        const float z = Size(EggOf(q, s), p, more);
        if (z <= 1.0f && z < bestSize)
            best = q, bestSize = z;
    }
    if (size)
        *size = bestSize;
    return best;
}

// The egg nearest a point by its Size, of every place (the chests and the belly too; the rib on its side): for the lines.
inline Place Nearest(const float p[3], const Settings& s, float* size = nullptr) {
    const Place list[7] = {Place::kRightShoulder, Place::kLeftShoulder, Place::kNeck, s.mirror ? Place::kRightRib : Place::kLeftRib,
                           Place::kLeftChest, Place::kRightChest, Place::kBelly};
    Place best = Place::kNone;
    float bestSize = 1e9f;
    for (const Place q : list) {
        const float z = Size(EggOf(q, s), p);
        if (z < bestSize)
            best = q, bestSize = z;
    }
    if (size)
        *size = bestSize;
    return best;
}

inline const char* Name(Place p) {
    switch (p) {
    case Place::kRightShoulder: return "the right shoulder";
    case Place::kLeftShoulder: return "the left shoulder";
    case Place::kNeck: return "the back of the neck";
    case Place::kLeftRib: return "the left rib cage";
    case Place::kRightRib: return "the right rib cage";
    case Place::kLeftChest: return "the left chest";
    case Place::kRightChest: return "the right chest";
    case Place::kBelly: return "the belly";
    default: return "no place";
    }
}
// "at the right shoulder", "behind the neck".
inline const char* AtName(Place p) {
    switch (p) {
    case Place::kRightShoulder: return "at the right shoulder";
    case Place::kLeftShoulder: return "at the left shoulder";
    case Place::kNeck: return "behind the neck";
    case Place::kLeftRib: return "at the left rib cage";
    case Place::kRightRib: return "at the right rib cage";
    case Place::kLeftChest: return "at the left chest";
    case Place::kRightChest: return "at the right chest";
    case Place::kBelly: return "at the belly";
    default: return "nowhere";
    }
}
// "from the right shoulder", "from behind the neck".
inline const char* FromName(Place p) {
    switch (p) {
    case Place::kRightShoulder: return "from the right shoulder";
    case Place::kLeftShoulder: return "from the left shoulder";
    case Place::kNeck: return "from behind the neck";
    case Place::kLeftRib: return "from the left rib cage";
    case Place::kRightRib: return "from the right rib cage";
    default: return "from nowhere";
    }
}

// The put-away sound's pan there with "stowpan side" (sounds::Play's: -1 left to 1 right): the right's places to the right, the left's to the left,
// the neck in the middle. "stowpan middle", the default, keeps it in the middle as the reloads' sounds are (the user, 2026-10-30).
inline float PanOf(Place p) {
    return p == Place::kRightShoulder || p == Place::kRightRib || p == Place::kRightChest ? 0.6f
           : p == Place::kLeftShoulder || p == Place::kLeftRib || p == Place::kLeftChest ? -0.6f
                                                                                         : 0.0f;
}

// ---- The body's axes (pouch::BodyFrame's: right (cos, 0, -sin), up y, forward (-sin, 0, -cos)) ----

inline bool Finite3(const float v[3]) { return std::isfinite(v[0]) && std::isfinite(v[1]) && std::isfinite(v[2]); }
// A place in the room this far out is broken, as one not finite: a float that large has lost the centimetres a place is judged by (a neck 1e30 m
// up put a hand anywhere at the shoulder: holstertest N1).
constexpr float kRoomMost = 1000.0f;
inline bool Sane3(const float v[3]) {
    return Finite3(v) && std::fabs(v[0]) <= kRoomMost && std::fabs(v[1]) <= kRoomMost && std::fabs(v[2]) <= kRoomMost;
}

// A direction in the room into the body's axes.
inline void TurnToBody(const float v[3], float yaw, float out[3]) {
    const float c = std::cos(yaw), s = std::sin(yaw);
    out[0] = v[0] * c - v[2] * s;
    out[1] = v[1];
    out[2] = -v[0] * s - v[2] * c;
}
// A point in the room from the neck point in the body's axes; false when a number isn't finite.
inline bool FromNeck(const float at[3], const float neck[3], float yaw, float out[3]) {
    if (!Sane3(at) || !Sane3(neck) || !std::isfinite(yaw))
        return false;
    const float d[3] = {at[0] - neck[0], at[1] - neck[1], at[2] - neck[2]};
    TurnToBody(d, yaw, out);
    return Finite3(out);
}

// ---- The guns and their places ----

// A slot of the soldier's as hands.cpp reads it (ammo's reads of the slots, recoilguns.h's row by its name, the hands' profile).
struct SlotIn {
    bool carried = false;   // a weapon in it
    bool launcher = false;  // a launcher by its kind: recoilguns.h's kRocket, kMissile or kAt4 (not the M320, which goes with its rifle or by its slot)
    bool known = false;     // the hands know it (hands.cpp's ProfileIdFor gives an id)
};

// The game's key for a slot (input.cpp has 1, 2 and 4): slot 0 key 1, slot 1 key 2, slot 5 key 4; 0 none.
inline int KeyOf(int slot) { return slot == 0 ? 1 : slot == 1 ? 2 : slot == 5 ? 4 : 0; }

// Each slot's place. Launchers by their kind at the launcher's place, whatever their slot ("twolaunchers slot": with two or more carried, one in
// slot 0 or 1 by its slot); every other gun by its slot: 0 the primary's shoulder, 1 the secondary's, 5 the launcher's place; the rest none (the
// M320 under the rifle in 2, the claymore in 4, the M67 in 6, the knife in 7, a pistol in 9: no key reaches them, and the chests are reserved).
inline void PlacesOf(const SlotIn slots[kSlots], const Settings& s, Place out[kSlots]) {
    int launchers = 0;
    for (int i = 0; i < kSlots; ++i)
        launchers += slots[i].carried && slots[i].launcher;
    const auto bySlot = [&](int i) { return !slots[i].launcher || (s.twoLaunchersBySlot && launchers >= 2 && (i == 0 || i == 1)); };
    // With "launcher shoulder", the secondary's shoulder is the launcher's unless slot 1's gun is there by its slot: then behind the neck.
    const bool secondaryTaken = slots[1].carried && bySlot(1);
    const Place launcherPlace = s.launcher == Launcher::kShoulder && secondaryTaken ? LauncherFallback(s) : LauncherPlace(s);
    for (int i = 0; i < kSlots; ++i) {
        out[i] = Place::kNone;
        if (!slots[i].carried)
            continue;
        if (!bySlot(i))
            out[i] = launcherPlace;
        else if (i == 0)
            out[i] = PrimaryPlace(s);
        else if (i == 1)
            out[i] = SecondaryPlace(s);
        else if (i == 5)
            out[i] = launcherPlace;
    }
}

// A gun the soldier carries that isn't the game's weapon is at its place; the game's weapon only while put away: never while it lies, flies into
// a hand or is held. gameSlot: the game's weapon's (-1 not known: then none is).
inline bool AtHolster(int slot, const SlotIn slots[kSlots], int gameSlot, Holder holder) {
    if (slot < 0 || slot >= kSlots || !slots[slot].carried || gameSlot < 0 || gameSlot >= kSlots)
        return false;
    return slot != gameSlot || holder == Holder::kStowed;
}

// The gun a draw at place p gives: one at its holster there; of two, the one in the hands last (heldAt: the clock each slot's gun was last in
// the hands, under 0 never), then slot 5's, then the lowest slot; -1 none.
inline int GunAt(Place p, const SlotIn slots[kSlots], const Place places[kSlots], int gameSlot, Holder holder, const float heldAt[kSlots]) {
    int best = -1;
    for (int i = 0; i < kSlots; ++i) {
        if (p == Place::kNone || places[i] != p || !AtHolster(i, slots, gameSlot, holder))
            continue;
        if (best < 0) {
            best = i;
            continue;
        }
        const float a = heldAt[i], b = heldAt[best];
        if (a > b || (a == b && i == 5 && best != 5))
            best = i;
    }
    return best;
}

// ---- What the frame brings ----

struct HandIn {
    Seen seen = Seen::kNone;            // raw, this frame
    float at[3] = {};                   // its grip point in the room when seen or guessed (the runtime's place: never input's held one)
    bool velocityKnown = false;         // its controller's own speed (room, m/s) given by the runtime ...
    float velocity[3] = {};
    bool own = false, read = false;     // a real controller or a test's hand (the stand-in's isn't: it never takes or puts away); its grip read now
    float squeeze = 0.0f;               // its grip, 0-1
    bool armed = false;                 // the grasp's: squeezed past kTake since it took hold (last frame's Result::armed)
    // What it holds or does, as the last frame left it:
    bool busy = false;      // the grasp's busy: a magazine, the pistol's, the M320's round, the charging handle, the glove held to line it up
    bool magazine = false;  // of those, a rifle magazine (the pouch's, handmag's)
    bool round40 = false;   // of those, the grenade launcher's 40 mm round
    bool rocket = false;    // rocket.cpp's rocket or case in this hand (its kHand, kLine, kSeating, kLocking, kGrabbed, kPulling, kHandSpent)
    bool onGun = false;     // on a gun's handguard or front grip (the grasp's onSupport), the M320's grip, a lever
    bool aims = false;      // the trigger hand with its gun at the eye: never a tick
};

struct Frame {
    float dt = 0.0f;           // seconds (0-0.1)
    bool on = false;           // free hands awake for the gun in hand (the grasp's enabled)
    bool frozen = false;       // a scripted moment, or the switch's frames with no gun drawn (the grasp's frozen)
    bool gameReload = false;   // the game's own reload
    bool m320 = false;         // the grenade launcher's shot or the game's reload of it under way (input::M320ShotUnderway)
    bool body = false;         // the body placed this frame (pouch::BodyFrame, the head tracked): without it, no draw and no put-away
    float neck[3] = {};
    float yaw = 0.0f;
    HandIn hand[2];
    Holder holder = Holder::kHeld;  // the grasp's, as the last frame left it
    int trigger = 1, barrel = -1;
    bool pendingTake = false;  // the grasp's take asked of the game, waiting
    bool anyTake = false;      // something a hand may take this frame (the game's weapon lies, dropped guns offered): a refusal waits for the take
    int gameSlot = -1;         // the game's own current weapon's slot now (ammo's GameSlot): -1 not read, the last read kept
    SlotIn slots[kSlots];
    bool picked = false;       // a pick this frame: the D-pad, Y held, or X held's pickup (the game's R for a dropped gun)
    int pickSlot = -1;         // ... the slot it raises (-1: not known, X held's)
};

// What the grasp did this frame (its Result), for Settle.
struct After {
    int stowed = -1;   // it put the gun away by that hand
    int drawn = -1;    // it drew the gun from its place into that hand
    int letGo = -1;    // that hand let go of the gun (put away, fallen, or onto the other's hold)
    bool spent[2] = {};  // that hand's squeeze took something (a take)
};

// ---- What follows ----

enum class Why : uint8_t {
    kNone,
    kSlotUnread,   // the game's slot not read yet
    kBusy,         // the hand isn't empty
    kMoment,       // a scripted moment, the game's reload, the grenade launcher's shot, a take or a draw waiting
    kOtherHolds,   // the other hand holds the game's weapon
    kLying,        // the game's weapon lies on the ground (or flies into a hand)
    kNothing,      // nothing at that place
    kUnknown,      // the gun there is one the hands don't know yet
    kNoKey,        // no key reaches its slot
    kNoSwitch,     // the game didn't switch to it
    kOtherRocket,  // put-aways: the other hand holds a rocket or a case
    kOtherRound,   // ... the grenade launcher's 40 mm round
    kOtherMag,     // ... its magazine ("heldmag refuse")
    kOpened,       // a take called off: the grip opened before the hand was seen
    kLate,         // ... the hand not seen again in time; a gun come after its draw stopped waiting
    kPicked,       // ... a pick of another slot came first (the D-pad, Y held, X held's pickup): the newer word wins
};

enum class What : uint8_t {
    kNone,
    kStowed,       // put away
    kDrawn,        // drawn into a hand (case A, a pick of its own slot, a hidden take seen again)
    kAsked,        // a case-B draw: its key pressed (or a hidden take of the gun put away: no key)
    kKeyAgain,     // its key pressed once more
    kNoSwitch,     // given up: the game didn't switch
    kRefused,      // a draw refused (why)
    kStowRefused,  // a put-away refused: the let-go was today's (why)
    kTick,         // a hand entered a place where it can act
    kCalledOff,    // a hidden take called off (why)
    kCameAway,     // the game's new weapon kept at its place (why: kNone it switched by itself, kOpened the drawing hand opened, kLate)
    kGameWay,      // a gun the hands don't know asked for the game's way ("unknown game")
    kRecall,       // the lying gun put back at its place ("lying recall")
    kLapsed,       // a draw (why kNothing) or a put-away (kNone) asked of the grasp and never taken within kAskMost: nothing done (not felt)
    kLook,         // "log 1": a hand near the neck point
};

struct Event {
    What what = What::kNone;
    int side = -1;
    Place place = Place::kNone;  // the place it's about (a put-away's: the gun's own place)
    Place at = Place::kNone;     // where the hand was, when another (a put-away at another holster's place)
    int slot = -1;               // the gun's slot
    int other = -1;              // the slot of the gun the other hand holds, or that lies; kKeyAgain: the game's slot then
    Why why = Why::kNone;
    bool quiet = false;          // nothing felt: said only with "log 1"
    bool away = false;           // kTick: the gun can be put away there (else drawn)
    bool kept = false;           // kStowRefused: the gun stays in the hand (a hand not seen drops nothing), else it fell as anywhere
    bool unseen = false;         // by a hand not seen: its record ...
    float unseenFor = 0.0f, lastOff = 0.0f, lastSpeed = 0.0f;  // ... s not seen; m from the place when last seen; m/s then
    int key = 0;                 // the key pressed
    bool twice = false;          // kNoSwitch: its key pressed twice (else no second key: one could have done harm then)
    float since = 0.0f;          // kKeyAgain, kNoSwitch, kDrawn after a hidden take, kCalledOff: s since the squeeze
    float size = 0.0f, from[3] = {};  // kLook: the place's Size and the hand from the neck point
    Seen seen = Seen::kNone;     // kLook
    bool off = false;            // kDrawn: holsters 0 brought it back from its place into the right hand, as at a spawn
};

struct Pulse {
    float amplitude = 0.0f, seconds = 0.0f;  // input::Buzz's; 0: none
};

enum class Sound : uint8_t { kNone, kStow, kDraw };  // kStow: "stowsound"'s cut; kDraw: the gun's own deploy where the game has one, else take.wav

constexpr int kMostEvents = 8;

struct Result {
    // For the grasp, this frame (before its Update):
    bool stows[2] = {};  // a let-go by that hand this frame puts the gun away (grasp's HandIn::stows)
    int stowSide = -1;   // put away now by that hand: a hand not seen opening at its place (grasp's Frame::stowSide)
    int drawSide = -1;   // drawn now from its place into that hand (grasp's Frame::drawSide: kStowed to kHeld, that squeeze spent) ...
    bool drawArmed = true;  // ... armed, as a take (its grip squeezes it); false for a pick's draw (the D-pad, Y held: the grip may be open):
                            // unarmed as at a spawn, so it stays in the hand till that grip has been squeezed and let go
    bool hold[2] = {};   // that hand holds the gun and isn't seen: its let-go mustn't drop it (the grasp told it isn't tracked)
    bool busy[2] = {};   // that hand's squeeze is a draw's that waits for its gun (case B): busy for the grasp (no take or regrip with it). For the
                         // grasp's HandIn::busy only: holsters' own Frame::hand[i].busy stays the modules' (fed back, it would read as a busy hand)
    // For the drawing (skin's HandTargets::stowed besides the grasp's kStowed): the gun drawn nowhere though not put away.
    bool unseenAtPlace = false;  // the hand holding the game's weapon isn't seen and is presumed at a place: it may be putting it away there (as long
                                 // as it stays not seen, the put-away's seconds over or not)
    bool drawWaits = false;      // a draw waits: a case-B key sent and its gun not in the hand yet, or a take by a hand not seen (while the gun
                                 // is put away only: never a gun that is in a hand)
    // For the game:
    int key = -1;          // raise this slot now by its key (input's SelectSlot)
    bool gameWay = false;  // ... the game's way: free hands asleep for it
    bool recall = false;   // after Settle: the lying gun goes to its place now (its copy off the ground, the grasp kStowed)
    // For the player (after Settle):
    Pulse buzz[2];
    Sound sound = Sound::kNone;
    float pan = 0.0f;
    int soundSlot = -1;          // the gun whose sound it is
    bool stillShown[2] = {};     // "vanish": the gun put away by that hand still drawn in it, resting there open
    Event event[kMostEvents];
    int events = 0;
    // Each hand's holster place now (seen or guessed, from the neck point), for the lines.
    Place place[2] = {};
    float size[2] = {1e9f, 1e9f};
};

// ---- The state, kept by hands.cpp outside the grasp's Clear ----

struct Record {        // one controller as seen raw, holsters' own
    bool haveLast = false;   // seen with the body placed
    float lastAt[3] = {};    // its last seen place (room) ...
    float lastFrom[3] = {};  // ... from the neck point then (body axes) ...
    float lastVel[3] = {};   // ... its velocity then (body axes) ...
    float lastSpeed = 0.0f;
    float lastTime = -1e9f;  // ... and the clock then
    bool havePrev = false;   // the seen frame before it (for a step's speed)
    float prevAt[3] = {};
    float prevTime = 0.0f;
    bool seenNow = false;    // seen this frame
    float unseenFor = 0.0f;  // s since the last seen frame
    bool judged = true;      // this loss's place worked out
    Place presumed = Place::kNone;  // presumed at it (fixed at the loss) ...
    bool vetoed = false;     // ... until the runtime's guess said otherwise
    bool gripArmed = false;  // the grip was past kTake at the loss or since (an opening while not seen counts only so)
    float off = 0.0f;        // m from the presumed place when last seen
};

struct Grip {          // one hand's grip by its reading
    bool armTake = false;     // under kArm since its last crossing: the next crossing of kTake is fresh
    float crossedAt = -1e9f;  // the clock at its latest crossing
    bool spent = true;        // that crossing used (or none yet)
    int low = 0;              // frames of its reading under kLetGo
    float letGoAt = -1e9f;    // its last let-go of the gun
};

struct Wait {          // a draw waiting: for the game's switch (case B), or for a hand not seen to be seen again
    bool on = false;
    int side = -1, slot = -1;
    Place place = Place::kNone;
    float since = 0.0f;       // s since the squeeze
    int presses = 0;          // keys pressed (case B)
    bool landed = false;      // the game's slot is the one asked for: no more keys
    bool arrived = false;     // the gun is the game's weapon put away now: into the hand once it keeps it and is seen
    bool hidden = false;      // the hand not seen since the squeeze
    bool keeps = true;        // the hand keeps the coming gun (still squeezed, empty, within keyGiveUp for a seen case B)
    bool gameWay = false;
};

struct Ask {           // a draw or a put-away asked of the grasp, asked again each frame till it's done (Settle) or kAskMost
    int side = -1;
    float since = 0.0f;
    bool armed = true;                      // a draw: by a squeeze (a pick's isn't)
    bool hidden = false;                    // a draw: a hidden take's ...
    float hiddenFor = 0.0f;                 // ... s since its squeeze
    Place planned = Place::kNone, at = Place::kNone;  // a put-away: where it goes, where the hand was
};

struct Refusal {       // a refusal waiting out the take's window (something lay to be taken)
    bool on = false;
    Event event;
    float crossedAt = 0.0f;
    bool recall = false;      // "lying recall": the lying gun goes to its place at the window's end, then the draw is tried again
};

struct State {
    float clock = 0.0f;
    int gameSlot = -1;              // the game's weapon's slot as last read ...
    float slotUnread = 0.0f;        // ... s since: past kSlotStale nothing is decided on it
    float heldAt[kSlots];           // the clock each slot's gun was last in the hands (-1e9 never)
    Record rec[2];
    Grip grip[2];
    Place inPlace[2] = {};          // the place each hand is in for the tick (left past kLeaveSize)
    Wait wait;
    Ask drawAsk, stowAsk;
    Refusal refusal[2];
    float pulseAt[2] = {-1.0f, -1.0f};  // a refusal's second pulse due then (under 0: none)
    Place planned[2] = {};          // where this frame's put-away by that hand would go (Update's, for Settle) ...
    Place plannedAt[2] = {};        // ... and where that hand was
    Why stowWhy[2] = {};            // why a let-go at a place by that hand this frame isn't a put-away (Settle says it if it lets go)
    Place stowWhyAt[2] = {};
    int drawPlanned = -1;           // the hand this frame's drawSide is for (Settle's click, sound, line)
    bool drawWasHidden = false;
    float drawHiddenFor = 0.0f;
    int shownSide = -1;             // "vanish": the hand that put the gun away ...
    Place shownPlace = Place::kNone;
    float shownAt = 0.0f;           // ... and when
    float pickAt = -1e9f;           // the clock at the last pick
    int recallSide = -1;            // "lying recall": the hand whose draw waits for the gun put back
    Place recallPlace = Place::kNone;
    float recallAt = 0.0f;
    float lookAt[2] = {-1e9f, -1e9f};  // "log 1"'s last line for each hand
    State() {
        for (float& v : heldAt)
            v = -1e9f;
    }
};

inline void Clear(State& s) { s = State{}; }

// ---- The second key: what a case-B draw does next, from the game's own slot ----

enum class KeyStep : uint8_t { kWait, kPressAgain, kGiveUp, kLanded };
struct KeyIn {
    float since = 0.0f;   // s since the first key
    int presses = 1;      // keys pressed so far
    int target = -1;      // the slot asked for
    int gameSlot = -1;    // the game's slot as last read (-1 never)
    bool readNow = false; // read this frame: a second key only on a slot read now
    bool harm = false;    // a slot key could do harm now: the grenade launcher's shot or reload, the game's reload, a scripted moment, frozen,
                          // a dropped gun's take asked of the game
};
// Landed once the game's slot is the one asked for. Else a second key once keyAgain has passed, one only, never while a key could do harm (the
// grenade launcher's lesson: key 1 pressed as it fired kept the game from reloading it) and never on a slot not read now; given up at keyGiveUp.
inline KeyStep KeyAgain(const KeyIn& in, const Settings& s) {
    if (in.target >= 0 && in.gameSlot == in.target)
        return KeyStep::kLanded;
    if (!(in.since < s.keyGiveUp))
        return KeyStep::kGiveUp;
    if (in.since >= s.keyAgain && in.presses < 2 && !in.harm && in.readNow && in.gameSlot >= 0)
        return KeyStep::kPressAgain;
    return KeyStep::kWait;
}

// ---- A hand not seen: the place it's presumed at ----

// The place a hand last seen at from (body axes) going at vel (body axes, m/s) is presumed at once it isn't seen: only a place that may be out of
// sight (the launcher's; the shoulders with "unseenshoulders 1"; never the belly or a chest), never one whose top it was more than "above" over (a
// hand coming down from the headset's strap isn't reaching behind the neck). A hand going at least "leastSpeed" is presumed where it's going: a
// place within "approach" of it whose middle is within "degrees" of its way (the straightest first), or else the place it's in if it would still be
// in it kAhead on; a slower hand, the place it's in (the smaller Size first). off: metres from that place's edge (0 inside).
inline Place Presume(const float from[3], const float vel[3], const Settings& s, float* off = nullptr) {
    if (off)
        *off = 0.0f;
    if (!s.unseen || !Finite3(from) || !Finite3(vel))
        return Place::kNone;
    const Place list[4] = {LauncherPlace(s), LauncherFallback(s), s.unseenShoulders ? Place::kRightShoulder : Place::kNone,
                           s.unseenShoulders ? Place::kLeftShoulder : Place::kNone};
    const float speed = std::sqrt(vel[0] * vel[0] + vel[1] * vel[1] + vel[2] * vel[2]);
    const bool moving = speed >= s.leastSpeed && speed > 0.0f;
    const float most = std::cos(s.degrees * kPi / 180.0f);
    Place into = Place::kNone, in = Place::kNone;
    float intoCos = -2.0f, inSize = 1e9f;
    for (const Place q : list) {
        if (q == Place::kNone)
            continue;
        const Egg e = EggOf(q, s);
        if (from[1] > e.mid[1] + e.half[1] + s.above)
            continue;
        const float size = Size(e, from);
        if (moving) {
            const float d[3] = {e.mid[0] - from[0], e.mid[1] - from[1], e.mid[2] - from[2]};
            const float n = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
            // (At its very middle the way to it is no way: the hand is in it, below, not going into it, so it never outruns the place it goes to.)
            const float c = n > 1e-4f ? (d[0] * vel[0] + d[1] * vel[1] + d[2] * vel[2]) / (n * speed) : -2.0f;
            if (Size(e, from, s.approach) <= 1.0f && c >= most && c > intoCos)
                into = q, intoCos = c;
            const float ahead[3] = {from[0] + vel[0] * kAhead, from[1] + vel[1] * kAhead, from[2] + vel[2] * kAhead};
            if (size <= 1.0f && Size(e, ahead) <= 1.0f && size < inSize)
                in = q, inSize = size;
        } else if (size <= 1.0f && size < inSize) {
            in = q, inSize = size;
        }
    }
    const Place p = into != Place::kNone ? into : in;
    if (off && p != Place::kNone) {
        const Egg e = EggOf(p, s);
        const float size = Size(e, from);
        const float d[3] = {from[0] - e.mid[0], from[1] - e.mid[1], from[2] - e.mid[2]};
        const float n = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        *off = size > 1.0f && size < 1e8f ? n * (1.0f - 1.0f / size) : 0.0f;
    }
    return p;
}

// ---- Pieces of the update ----

namespace detail {

inline bool GunInHands(Holder h) { return h == Holder::kHeld || h == Holder::kBarrel || h == Holder::kGlide; }

// That hand has the gun: on its pistol grip (held, or flying into it), on its handguard alone, or the support of a regrip's glide.
inline bool HasGun(const Frame& f, int i) {
    return ((f.holder == Holder::kHeld || f.holder == Holder::kGlide) && f.trigger == i) ||
           ((f.holder == Holder::kBarrel || f.holder == Holder::kGlide) && f.barrel == i);
}
// That hand holds the game's weapon so it may put it away: by its pistol grip, or by its handguard alone.
inline bool HoldsToStow(const Frame& f, int i) {
    return (f.holder == Holder::kHeld && f.trigger == i) || (f.holder == Holder::kBarrel && f.barrel == i);
}
// Empty: no gun, no magazine, round, rocket or case, not on a gun's front, not busy otherwise.
inline bool Empty(const Frame& f, int i) {
    const HandIn& h = f.hand[i];
    return !HasGun(f, i) && !h.busy && !h.magazine && !h.round40 && !h.rocket && !h.onGun;
}
// Holding something a waiting draw gives way to: a gun, a magazine, a round, a rocket or a case, a gun's front (busy aside, which holsters' own
// Result::busy would fill if it came back in).
inline bool HoldsThing(const Frame& f, int i) {
    const HandIn& h = f.hand[i];
    return HasGun(f, i) || h.magazine || h.round40 || h.rocket || h.onGun;
}
// The game's own slot read now or a moment ago (kSlotStale): what holsters decide on.
inline bool SlotKnown(const State& st) { return st.gameSlot >= 0 && st.gameSlot < kSlots && st.slotUnread <= kSlotStale; }

inline void Add(Result& r, const Event& e) {
    if (r.events < kMostEvents)
        r.event[r.events++] = e;
}
inline void Buzz(Result& r, const Settings& s, int side, float amp, float seconds) {
    if (side < 0 || side > 1 || !(s.buzz > 0.0f))
        return;
    const float a = amp * s.buzz < 1.0f ? amp * s.buzz : 1.0f;
    if (r.buzz[side].seconds == 0.0f || a > r.buzz[side].amplitude)
        r.buzz[side].amplitude = a, r.buzz[side].seconds = seconds;
}
inline void Double(Result& r, State& st, const Settings& s, int side) {
    if (side < 0 || side > 1 || !(s.buzz > 0.0f))
        return;
    Buzz(r, s, side, kDoubleAmp, kDoubleSeconds);
    st.pulseAt[side] = st.clock + kDoubleGap;
}

// A draw into that hand asked of the grasp now (and again each frame till it's done: Update's 1b, Settle).
inline void AskDraw(Result& r, State& st, int side, bool armed, bool hidden, float hiddenFor) {
    r.drawSide = side, r.drawArmed = armed;
    st.drawPlanned = side, st.drawWasHidden = hidden, st.drawHiddenFor = hiddenFor;
    st.drawAsk = Ask{};
    st.drawAsk.side = side, st.drawAsk.armed = armed, st.drawAsk.hidden = hidden, st.drawAsk.hiddenFor = hiddenFor;
}
// A put-away by that hand asked of the grasp now (planned its gun's own place, at where the hand is), again each frame till it's done.
inline void AskStow(Result& r, State& st, int side, Place planned, Place at) {
    r.stowSide = side;
    st.planned[side] = planned, st.plannedAt[side] = at;
    st.stowAsk = Ask{};
    st.stowAsk.side = side, st.stowAsk.planned = planned, st.stowAsk.at = at;
}

// Why a put-away by hand i at place at isn't one now (kNone: it is), own the gun's own place: kNothing not a place it goes away at (today's let-go,
// said nowhere), then the moment, the other hand's hold on it, what the other hand holds.
inline Why StowWhy(const Settings& s, const Frame& f, int i, Place at, Place own) {
    if (own == Place::kNone || at == Place::kNone || !(at == own || (s.anyHolster && Holds(at, s))))
        return Why::kNothing;
    if (f.frozen || f.gameReload || f.m320)
        return Why::kMoment;
    const HandIn& o = f.hand[1 - i];
    if (f.holder == Holder::kHeld && o.onGun)
        return Why::kOtherHolds;  // (the hold wins: the gun stays on that hand)
    if (o.rocket)
        return Why::kOtherRocket;
    if (o.round40)
        return Why::kOtherRound;
    if (o.magazine && s.heldMagRefuse)
        return Why::kOtherMag;
    return Why::kNone;
}

}  // namespace detail

// Why a draw at place at by hand i would be refused now (kNone: it would draw): slot the gun it would give, caseA whether that's the game's weapon
// put away (no key), stowOther whether the other hand's gun goes to its place first ("othergun stow").
inline Why DrawWhy(const Settings& s, const Frame& f, const State& st, int i, Place at, int& slot, bool& caseA, bool& stowOther) {
    using namespace detail;
    slot = -1, caseA = false, stowOther = false;
    if (!SlotKnown(st))
        return Why::kSlotUnread;
    if (!Empty(f, i))
        return Why::kBusy;
    if (f.frozen || f.gameReload || f.m320 || f.pendingTake || st.wait.on || st.recallSide >= 0)
        return Why::kMoment;
    Place places[kSlots];
    PlacesOf(f.slots, s, places);
    if (GunInHands(f.holder)) {
        // The other hand holds the game's weapon (this one is empty): one gun in the hands. "othergun stow": it goes to its place first, if it can,
        // and a gun of another slot at this place comes by its key.
        const Place held = places[st.gameSlot];
        if (!s.otherGunStow || !HoldsToStow(f, 1 - i) || StowWhy(s, f, 1 - i, held, held) != Why::kNone)
            return Why::kOtherHolds;
        slot = GunAt(at, f.slots, places, st.gameSlot, Holder::kStowed, st.heldAt);
        if (slot < 0 || slot == st.gameSlot)
            return Why::kOtherHolds;
        if (!f.slots[slot].known && !s.unknownGame)
            return Why::kUnknown;
        if (KeyOf(slot) == 0)
            return Why::kNoKey;
        stowOther = true;
        return Why::kNone;
    }
    if (f.holder == Holder::kLoose)
        return Why::kLying;
    slot = GunAt(at, f.slots, places, st.gameSlot, f.holder, st.heldAt);
    if (slot < 0)
        return Why::kNothing;
    caseA = slot == st.gameSlot && f.holder == Holder::kStowed;
    if (!caseA && !f.slots[slot].known && !s.unknownGame)
        return Why::kUnknown;
    if (!caseA && KeyOf(slot) == 0)
        return Why::kNoKey;
    return Why::kNone;
}

// A pick (the D-pad, Y held) of the put-away gun's own slot: no key (input asks this before pressing one), the gun drawn into the right hand.
inline bool PickIsOwnSlot(int slot, const State& st, Holder holder, const Settings& s) {
    return s.on && holder == Holder::kStowed && slot >= 0 && slot == st.gameSlot && detail::SlotKnown(st);
}

namespace detail {

// A squeeze judged at place at: a draw, or a refusal (said now, or after the take's window if something lies to be taken).
inline void Judge(const Settings& s, const Frame& f, State& st, Result& r, int i, Place at, bool hidden) {
    int slot = -1;
    bool caseA = false, stowOther = false;
    Why why = DrawWhy(s, f, st, i, at, slot, caseA, stowOther);
    if (why == Why::kNone && (r.drawSide >= 0 || r.key >= 0 || r.stowSide >= 0))
        why = Why::kMoment;  // (the other hand's draw or put-away this very frame)
    const Record& rc = st.rec[i];
    Event e;
    e.side = i, e.place = at, e.slot = slot, e.why = why, e.unseen = hidden;
    if (hidden)
        e.unseenFor = rc.unseenFor, e.lastOff = rc.off, e.lastSpeed = rc.lastSpeed;
    if (why == Why::kNone) {
        Place places[kSlots];
        PlacesOf(f.slots, s, places);
        if (stowOther)
            AskStow(r, st, 1 - i, places[st.gameSlot], places[st.gameSlot]);  // (the other hand's gun to its own place now: Settle says it)
        st.wait = Wait{};
        if (caseA) {
            if (!hidden) {
                AskDraw(r, st, i, true, false, 0.0f);
                return;  // (its click, sound and line once the grasp has drawn it: Settle)
            }
            st.wait.on = true, st.wait.side = i, st.wait.slot = slot, st.wait.place = at, st.wait.landed = true, st.wait.arrived = true;
            st.wait.hidden = true;
            e.what = What::kAsked;  // (no key: in the hand once it's seen)
            Add(r, e);
            Buzz(r, s, i, kClickAmp, kClickSeconds);
            return;
        }
        // Case B, or the game's way: the key now; the hand keeps the coming gun.
        r.key = slot;
        e.key = KeyOf(slot);
        st.wait.on = true, st.wait.side = i, st.wait.slot = slot, st.wait.place = at, st.wait.presses = 1, st.wait.hidden = hidden;
        if (!f.slots[slot].known) {
            r.gameWay = true;
            st.wait.gameWay = true, st.wait.keeps = false;
            e.what = What::kGameWay;
        } else {
            e.what = What::kAsked;
        }
        Add(r, e);
        Buzz(r, s, i, kClickAmp, kClickSeconds);
        return;
    }
    e.what = What::kRefused;
    if (why == Why::kOtherHolds || why == Why::kLying)
        e.other = st.gameSlot;
    // Felt only with both hands empty and nothing under way: the other hand holding the gun or anything else (a magazine taken at the belly
    // while the gun is away, a rocket, a case), a busy hand, a moment: silent (a line with "log 1").
    e.quiet = why == Why::kOtherHolds || why == Why::kBusy || why == Why::kMoment || why == Why::kSlotUnread || !Empty(f, 1 - i);
    if (!e.quiet && (f.anyTake || why == Why::kLying)) {
        // Something lies to be taken: a take may answer this squeeze, and a refusal never stands in for it. Said once the take's window is over
        // with nothing taken (Settle).
        st.refusal[i].on = true;
        st.refusal[i].event = e;
        st.refusal[i].crossedAt = st.grip[i].crossedAt;
        st.refusal[i].recall = why == Why::kLying && s.lyingRecall && !hidden;
        return;
    }
    Add(r, e);
    if (!e.quiet)
        Double(r, st, s, i);
}

}  // namespace detail

// ---- The update: before grasp::Update, each frame. r is cleared first. ----

inline void Update(const Settings& s, const Frame& f, State& st, Result& r) {
    using namespace detail;
    r = Result{};
    const float dt = std::isfinite(f.dt) ? (f.dt > 0.0f ? (f.dt < 0.1f ? f.dt : 0.1f) : 0.0f) : 0.0f;
    st.clock += dt;
    if (f.gameSlot >= 0 && f.gameSlot < kSlots)
        st.gameSlot = f.gameSlot, st.slotUnread = 0.0f;
    else
        st.slotUnread += dt;  // (-1: the last read kept a moment; nothing decided on a slot never read, or unread past kSlotStale: SlotKnown)
    st.drawPlanned = -1;
    for (int i = 0; i < 2; ++i)
        st.planned[i] = st.plannedAt[i] = Place::kNone, st.stowWhy[i] = Why::kNone, st.stowWhyAt[i] = Place::kNone;
    if (!s.on || !f.on) {
        // Off, or free hands asleep: today's in everything; what waited is dropped (a key pressed may still bring its gun: Arrive keeps it put away).
        // Holsters turned off with a gun put away: it comes back into the right hand, unarmed as at a spawn (asked each frame till the grasp has
        // it), so "holsters 0" never leaves a gun nobody can draw. (Free hands asleep: the grasp's own Clear puts it there.)
        if (!s.on && f.on && f.holder == Holder::kStowed)
            r.drawSide = 1, r.drawArmed = false, st.drawPlanned = 1, st.drawWasHidden = false, st.drawHiddenFor = 0.0f;
        st.wait = Wait{};
        st.drawAsk = st.stowAsk = Ask{};
        st.refusal[0] = st.refusal[1] = Refusal{};
        st.recallSide = -1;
        st.shownSide = -1;
        for (int i = 0; i < 2; ++i) {
            st.rec[i].seenNow = false, st.rec[i].judged = true, st.rec[i].presumed = Place::kNone;
            st.grip[i].spent = true, st.grip[i].armTake = false;  // (a grip held through it is no fresh squeeze after: it opens first)
        }
        return;
    }
    if (GunInHands(f.holder) && st.gameSlot >= 0)
        st.heldAt[st.gameSlot] = st.clock;
    if (f.picked) {
        st.pickAt = st.clock;
        // A pick of another slot calls off a draw that waits (its second key would undo the pick; the newer word wins). Said, not felt.
        if (st.wait.on && f.pickSlot != st.wait.slot) {
            Event e;
            e.what = What::kCalledOff, e.side = st.wait.side, e.place = st.wait.place, e.slot = st.wait.slot, e.why = Why::kPicked;
            e.since = st.wait.since;
            Add(r, e);
            st.wait = Wait{};
        }
    }
    Place places[kSlots];
    PlacesOf(f.slots, s, places);
    const bool slotKnown = SlotKnown(st);
    const Place own = slotKnown ? places[st.gameSlot] : Place::kNone;
    const bool body = f.body && Sane3(f.neck) && std::isfinite(f.yaw);
    const bool gates = body && slotKnown;

    // 1. Each hand: where it is, its speed, its record and its grip.
    bool seen[2] = {}, placed[2] = {}, speedKnown[2] = {}, openedNow[2] = {};
    float from[2][3] = {}, speed[2] = {};
    for (int i = 0; i < 2; ++i) {
        const HandIn& h = f.hand[i];
        Record& rc = st.rec[i];
        Grip& g = st.grip[i];
        const bool seenBefore = rc.seenNow;
        const bool atOk = Sane3(h.at);
        seen[i] = h.seen == Seen::kSeen && atOk;
        const bool guessed = h.seen == Seen::kGuessed && atOk;
        placed[i] = body && (seen[i] || guessed) && FromNeck(h.at, f.neck, f.yaw, from[i]);
        if (placed[i])
            r.place[i] = PlaceAt(from[i], s, &r.size[i]);
        // Its speed: its controller's own, else the step from the frame before when that was seen too.
        float vel[3] = {};
        if (h.velocityKnown && Finite3(h.velocity)) {
            vel[0] = h.velocity[0], vel[1] = h.velocity[1], vel[2] = h.velocity[2];
            speedKnown[i] = true;
        } else if (seen[i] && seenBefore && rc.lastTime > -1e8f && dt > 0.0f) {
            for (int k = 0; k < 3; ++k)
                vel[k] = (h.at[k] - rc.lastAt[k]) / dt;
            speedKnown[i] = Finite3(vel);
        }
        if (speedKnown[i])
            speed[i] = std::sqrt(vel[0] * vel[0] + vel[1] * vel[1] + vel[2] * vel[2]);
        rc.seenNow = seen[i];
        if (seen[i]) {
            const bool hadLast = rc.lastTime > -1e8f;
            if (hadLast)
                std::memcpy(rc.prevAt, rc.lastAt, sizeof(rc.prevAt)), rc.prevTime = rc.lastTime, rc.havePrev = true;
            std::memcpy(rc.lastAt, h.at, sizeof(rc.lastAt));
            rc.lastTime = st.clock;
            if (!speedKnown[i] && rc.havePrev && rc.lastTime - rc.prevTime > 0.0f && rc.lastTime - rc.prevTime < 0.25f)
                for (int k = 0; k < 3; ++k)
                    vel[k] = (rc.lastAt[k] - rc.prevAt[k]) / (rc.lastTime - rc.prevTime);
            if (placed[i]) {
                std::memcpy(rc.lastFrom, from[i], sizeof(rc.lastFrom));
                TurnToBody(vel, f.yaw, rc.lastVel);
                rc.lastSpeed = std::sqrt(vel[0] * vel[0] + vel[1] * vel[1] + vel[2] * vel[2]);
                if (!Finite3(rc.lastVel) || !std::isfinite(rc.lastSpeed))
                    rc.lastVel[0] = rc.lastVel[1] = rc.lastVel[2] = 0.0f, rc.lastSpeed = 0.0f;
                rc.haveLast = true;
            } else {
                rc.haveLast = false;  // (no body: no place to judge a loss by)
            }
            rc.unseenFor = 0.0f, rc.judged = false, rc.presumed = Place::kNone, rc.vetoed = false, rc.gripArmed = false, rc.off = 0.0f;
        } else {
            rc.unseenFor += dt;
            if (!rc.judged) {
                // The loss's first frame: its place fixed from the last seen frame.
                rc.judged = true;
                rc.presumed = rc.haveLast ? Presume(rc.lastFrom, rc.lastVel, s, &rc.off) : Place::kNone;
                rc.gripArmed = h.armed || (h.read && h.squeeze >= kTake);
            }
            if (h.read && h.squeeze >= kTake)
                rc.gripArmed = true;
            if (guessed && rc.presumed != Place::kNone && !rc.vetoed) {
                // The runtime's guess: within guessMost of the last seen place and inside the place grown, or the place is given up.
                const float d[3] = {h.at[0] - rc.lastAt[0], h.at[1] - rc.lastAt[1], h.at[2] - rc.lastAt[2]};
                const float apart = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
                if (!(apart <= s.guessMost) || !placed[i] || Size(EggOf(rc.presumed, s), from[i], kGuessGrow) > 1.0f)
                    rc.vetoed = true;
            }
        }
        // The grip, by its reading, seen or not (a button is read either way).
        if (h.own && h.read && std::isfinite(h.squeeze)) {
            if (h.squeeze < kArm)
                g.armTake = true;
            if (g.armTake && h.squeeze >= kTake)
                g.crossedAt = st.clock, g.armTake = false, g.spent = false;
            const int lowBefore = g.low;
            g.low = h.squeeze < kLetGo ? (g.low < kLetGoFrames ? g.low + 1 : kLetGoFrames) : 0;
            openedNow[i] = g.low >= kLetGoFrames && lowBefore < kLetGoFrames;  // (the frame it has opened: an opening counts once)
        }
    }

    // 1b. A draw or a put-away asked of the grasp and not done yet (Settle ends it when it is): asked again, till kAskMost.
    if (st.drawAsk.side == 0 || st.drawAsk.side == 1) {
        Ask& q = st.drawAsk;
        const int i = q.side;
        q.since += dt;
        const bool opened = f.hand[i].read && st.grip[i].low >= kLetGoFrames;
        if (f.holder == Holder::kStowed && q.since > kAskMost) {
            Event e;  // (the grasp never took it: frozen all the while, or the hand not its own or not tracked as it counts them)
            e.what = What::kLapsed, e.side = i, e.slot = st.gameSlot, e.why = Why::kNothing, e.since = q.since;
            Add(r, e);
            q = Ask{};
        } else if (f.holder != Holder::kStowed || (q.armed && opened) || !Empty(f, i)) {
            q = Ask{};  // (done, or no longer wanted: let go first, or something else in that hand)
        } else {
            r.drawSide = i, r.drawArmed = q.armed, st.drawPlanned = i, st.drawWasHidden = q.hidden, st.drawHiddenFor = q.hiddenFor;
        }
    }
    if (st.stowAsk.side == 0 || st.stowAsk.side == 1) {
        Ask& q = st.stowAsk;
        const int i = q.side;
        q.since += dt;
        if (HoldsToStow(f, i) && q.since > kAskMost) {
            Event e;
            e.what = What::kLapsed, e.side = i, e.slot = st.gameSlot, e.why = Why::kNone, e.since = q.since, e.place = q.planned;
            Add(r, e);
            q = Ask{};
        } else if (!HoldsToStow(f, i)) {
            q = Ask{};
        } else {
            r.stowSide = i, st.planned[i] = q.planned, st.plannedAt[i] = q.at;
        }
    }

    // 2. Put-aways: a seen hand's let-go at a place (the grasp decides the let-go, stows where it goes), a hand not seen opening at its presumed
    // place (stowSide), and the hold that keeps a hand not seen from dropping its gun.
    for (int i = 0; i < 2; ++i) {
        const HandIn& h = f.hand[i];
        const Record& rc = st.rec[i];
        if (!HasGun(f, i))
            continue;
        if (!seen[i] && h.own && !s.unseenDrop)
            r.hold[i] = true;
        // Drawn nowhere while that hand isn't seen and is presumed at a place (the game would draw its own at the camera meanwhile): for as long as it
        // stays so, past the put-away's seconds too (a gun held behind the neck 5 s would otherwise come out in front of the eyes, the game's own hand
        // on it, as input's held place ran out); seen again, or the runtime's guess elsewhere, it's in the hand.
        if (!seen[i] && h.own && s.unseen && HoldsToStow(f, i) && rc.presumed != Place::kNone && Holds(rc.presumed, s) && !rc.vetoed)
            r.unseenAtPlace = true;
        if (!gates || !HoldsToStow(f, i))
            continue;
        if (seen[i] && placed[i]) {
            const Place at = PlaceAt(from[i], s);
            const Why why = StowWhy(s, f, i, at, own);
            const bool slow = !speedKnown[i] || speed[i] < s.stowSpeed;
            if (why == Why::kNone && slow) {
                r.stows[i] = true;
                st.planned[i] = own, st.plannedAt[i] = at;
            } else if (why == Why::kOtherRocket || why == Why::kOtherRound || why == Why::kOtherMag) {
                st.stowWhy[i] = why, st.stowWhyAt[i] = at;  // (said if it lets go here)
            }
        } else if (!seen[i] && h.own && s.unseen && rc.presumed != Place::kNone && !rc.vetoed && rc.unseenFor <= s.unseenStow && rc.gripArmed &&
                   openedNow[i] && r.stowSide < 0) {
            const Why why = StowWhy(s, f, i, rc.presumed, own);
            if (why == Why::kNone) {
                AskStow(r, st, i, own, rc.presumed);
            } else if (why != Why::kNothing) {
                // Refused: the gun stays in the hand (it isn't seen: nothing is dropped). Felt for what the other hand holds; the hold on the
                // handguard and a moment silent.
                Event e;
                e.what = What::kStowRefused, e.side = i, e.slot = st.gameSlot, e.why = why, e.at = rc.presumed, e.place = own, e.kept = true;
                e.unseen = true, e.unseenFor = rc.unseenFor, e.lastOff = rc.off, e.lastSpeed = rc.lastSpeed;
                e.quiet = why == Why::kOtherHolds || why == Why::kMoment;
                Add(r, e);
                if (!e.quiet)
                    Double(r, st, s, i);
            }
        }
    }

    // 3. A draw waiting: for the game's switch (its second key, giving up), or for a hand not seen to be seen again.
    if (st.wait.on) {
        Wait& w = st.wait;
        const int i = w.side;
        const HandIn& h = f.hand[i];
        w.since += dt;
        if (seen[i] || (h.seen == Seen::kGuessed && Finite3(h.at)))
            w.hidden = false;  // (a place again: seen, or the runtime's own guess; never input's held place)
        const bool opened = h.read && st.grip[i].low >= kLetGoFrames;
        if (w.keeps && (opened || HoldsThing(f, i))) {
            w.keeps = false;
            if (w.arrived && w.hidden) {
                Event e;
                e.what = What::kCalledOff, e.side = i, e.place = w.place, e.slot = w.slot, e.why = Why::kOpened, e.since = w.since;
                Add(r, e);
                w = Wait{};
            }
        }
        if (w.on && !w.landed && !w.gameWay) {
            KeyIn k;
            k.since = w.since, k.presses = w.presses, k.target = w.slot, k.gameSlot = st.gameSlot, k.readNow = f.gameSlot >= 0;
            k.harm = f.m320 || f.gameReload || f.frozen || f.pendingTake;
            const KeyStep step = KeyAgain(k, s);
            if (step == KeyStep::kLanded) {
                w.landed = true;
            } else if (step == KeyStep::kPressAgain) {
                r.key = w.slot;
                ++w.presses;
                Event e;
                e.what = What::kKeyAgain, e.side = i, e.place = w.place, e.slot = w.slot, e.key = KeyOf(w.slot), e.since = w.since;
                e.other = st.gameSlot;
                Add(r, e);
            } else if (step == KeyStep::kGiveUp) {
                Event e;
                e.what = What::kNoSwitch, e.side = i, e.place = w.place, e.slot = w.slot, e.why = Why::kNoSwitch, e.since = w.since;
                e.twice = w.presses >= 2;
                Add(r, e);
                if (w.keeps && Empty(f, 1 - i))
                    Double(r, st, s, i);
                w = Wait{};
            }
        }
        if (w.on && w.gameWay && (w.landed || !(w.since < s.keyGiveUp)))
            w = Wait{};  // (the game's way: its own hands; nothing more to wait for)
        if (w.on && w.arrived) {
            if (w.keeps && !w.hidden && f.holder == Holder::kStowed && st.gameSlot == w.slot && !f.frozen && r.drawSide < 0) {
                AskDraw(r, st, i, true, true, w.since);  // (into the hand that drew it: the hidden take seen again)
                w = Wait{};
            } else if (w.since > s.unseenStow) {
                // Not seen again in time; or seen, but the game's weapon moved on meanwhile (or a scripted moment lasted): it stays at its place,
                // and the wait never outlives the take's window, so no draw is refused for it later.
                if (w.keeps) {
                    Event e;
                    e.what = What::kCalledOff, e.side = i, e.place = w.place, e.slot = w.slot, e.since = w.since;
                    e.why = w.hidden ? Why::kLate : Why::kMoment;
                    e.quiet = !w.hidden;
                    Add(r, e);
                }
                w = Wait{};
            } else if (!w.keeps) {
                w = Wait{};  // (it stays at its place)
            }
        } else if (w.on) {
            // Waiting for the gun: a seen hand keeps it keyGiveUp; the wait itself lasts till it comes (Arrive) or kArriveMost.
            if (w.keeps && !w.hidden && !(w.since < s.keyGiveUp))
                w.keeps = false;
            if (!(w.since < kArriveMost) || (w.hidden && w.since > s.unseenStow)) {
                if (w.hidden && w.keeps) {
                    Event e;
                    e.what = What::kCalledOff, e.side = i, e.place = w.place, e.slot = w.slot, e.why = Why::kLate, e.since = w.since;
                    Add(r, e);
                }
                w = Wait{};
            }
        }
    }

    // 4. Squeezes: a seen hand's at a place within the draw's seconds of its crossing, slow; a hand not seen, its own fresh squeeze at its presumed
    // place within the take's seconds. The right first (a tie goes to it, as the grasp's). Then a pick of the put-away gun's own slot.
    for (int i = 1; i >= 0; --i) {
        const HandIn& h = f.hand[i];
        Grip& g = st.grip[i];
        Record& rc = st.rec[i];
        if (g.spent || !h.own || !h.read || !(h.squeeze >= kTake))
            continue;
        if (!gates) {
            g.spent = true;  // (no body or no slot: nothing drawn by this squeeze)
            continue;
        }
        if (seen[i]) {
            if (st.clock - g.crossedAt > s.drawWindow)
                continue;  // (past the draw's seconds: nothing for a seen hand; lost within kReachWindow of it, the rule below may count it)
            if (!placed[i] || !speedKnown[i] || !(speed[i] < s.drawSpeed))
                continue;
            const Place at = PlaceAt(from[i], s);
            if (at == Place::kNone)
                continue;
            g.spent = true;
            if (st.clock - g.letGoAt <= kRetakeAfter)
                continue;  // (its own let-go just now: not fresh, as the grasp's)
            Judge(s, f, st, r, i, at, false);
        } else {
            if (!s.unseen || rc.presumed == Place::kNone || rc.vetoed || !rc.haveLast)
                continue;
            if (g.crossedAt < rc.lastTime - kReachWindow || rc.unseenFor > s.unseenTake) {
                g.spent = true;  // (crossed long before it was lost and held since, or past the take's seconds)
                continue;
            }
            g.spent = true;
            if (st.clock - g.letGoAt <= kRetakeAfter)
                continue;
            Judge(s, f, st, r, i, rc.presumed, true);
        }
    }
    if (f.picked && PickIsOwnSlot(f.pickSlot, st, f.holder, s) && !f.frozen && r.drawSide < 0 && r.stowSide < 0) {
        AskDraw(r, st, 1, false, false, 0.0f);  // (unarmed: the right grip may be open)
        st.wait = Wait{};
    }
    // "lying recall": once the gun is back at its place, the draw that waited for it.
    if (st.recallSide >= 0) {
        const int i = st.recallSide;
        if (f.holder == Holder::kStowed) {
            st.recallSide = -1;
            if (f.hand[i].read && f.hand[i].squeeze >= kLetGo && gates)
                Judge(s, f, st, r, i, st.recallPlace, false);
        } else if (st.clock - st.recallAt > 0.5f) {
            st.recallSide = -1;
        }
    }

    // 5. The tick: a seen hand entering a place where it can act, once an entry (an edge doesn't tick twice), never the trigger hand while it aims.
    for (int i = 0; i < 2; ++i) {
        const HandIn& h = f.hand[i];
        if (!seen[i] || !placed[i] || !h.own) {
            if (seen[i] && !placed[i])
                st.inPlace[i] = Place::kNone;
            continue;  // (not seen: where it was is kept, so a flicker inside a place doesn't tick again)
        }
        Place now = Place::kNone;
        if (st.inPlace[i] != Place::kNone && Holds(st.inPlace[i], s) && Size(EggOf(st.inPlace[i], s), from[i]) <= kLeaveSize)
            now = st.inPlace[i];
        else
            now = PlaceAt(from[i], s);
        if (now != Place::kNone && now != st.inPlace[i] && gates && h.read && s.buzz > 0.0f) {
            bool can = false;
            const bool away = HoldsToStow(f, i);
            if (away)
                can = now == own && StowWhy(s, f, i, now, own) == Why::kNone && !(h.aims && f.holder == Holder::kHeld && f.trigger == i);
            else if (Empty(f, i)) {
                int slot = -1;
                bool caseA = false, stowOther = false;
                can = DrawWhy(s, f, st, i, now, slot, caseA, stowOther) == Why::kNone;
            }
            if (can) {
                Buzz(r, s, i, kTickAmp, kTickSeconds);
                Event e;
                e.what = What::kTick, e.side = i, e.place = now, e.away = away;
                e.slot = away ? st.gameSlot : GunAt(now, f.slots, places, st.gameSlot, f.holder, st.heldAt);
                Add(r, e);
            }
        }
        st.inPlace[i] = now;
    }

    r.drawWaits = st.wait.on && f.holder == Holder::kStowed;  // (never hides a gun that is in a hand)
    // The squeeze a waiting draw made is that draw's: the grasp takes nothing with it (a dropped gun within reach of the neck or the rib, prone).
    if (st.wait.on && st.wait.keeps && (st.wait.side == 0 || st.wait.side == 1))
        r.busy[st.wait.side] = true;

    // 6. "log 1": a hand near the neck point, once a second each.
    if (s.log) {
        for (int i = 0; i < 2; ++i) {
            if (!placed[i])
                continue;
            const float n = std::sqrt(from[i][0] * from[i][0] + from[i][1] * from[i][1] + from[i][2] * from[i][2]);
            if (n <= kLogNear && st.clock - st.lookAt[i] >= 1.0f) {
                st.lookAt[i] = st.clock;
                Event e;
                e.what = What::kLook, e.side = i, e.quiet = true, e.seen = f.hand[i].seen;
                e.place = Nearest(from[i], s, &e.size);
                std::memcpy(e.from, from[i], sizeof(e.from));
                Add(r, e);
            }
        }
    }
}

// ---- After grasp::Update, the same frame: what the grasp did and what follows (the clicks, the sound, the lines). Adds to r. ----

inline void Settle(const Settings& s, const Frame& f, const After& a, State& st, Result& r) {
    using namespace detail;
    if (!s.on) {
        if (a.drawn == 1 && st.drawPlanned == 1) {  // (holsters turned off with the gun put away: back in the right hand)
            Event e;
            e.what = What::kDrawn, e.side = 1, e.slot = st.gameSlot, e.off = true;
            Add(r, e);
        }
        return;
    }
    if (a.drawn >= 0 && a.drawn == st.drawAsk.side)
        st.drawAsk = Ask{};  // (done)
    if (a.stowed >= 0 && a.stowed == st.stowAsk.side)
        st.stowAsk = Ask{};
    Place places[kSlots];
    PlacesOf(f.slots, s, places);
    if (a.stowed == 0 || a.stowed == 1) {
        const int i = a.stowed;
        const Record& rc = st.rec[i];
        Event e;
        e.what = What::kStowed, e.side = i, e.slot = st.gameSlot;
        e.place = st.planned[i] != Place::kNone ? st.planned[i] : (st.gameSlot >= 0 ? places[st.gameSlot] : Place::kNone);
        e.at = st.plannedAt[i] != e.place ? st.plannedAt[i] : Place::kNone;
        e.unseen = !rc.seenNow;
        if (e.unseen)
            e.unseenFor = rc.unseenFor, e.lastOff = rc.off, e.lastSpeed = rc.lastSpeed;
        Add(r, e);
        Buzz(r, s, i, kClickAmp, kClickSeconds);
        if (s.stowSound != StowSound::kNone) {
            r.sound = Sound::kStow, r.soundSlot = st.gameSlot;
            r.pan = s.stowPanSide ? PanOf(st.plannedAt[i] != Place::kNone ? st.plannedAt[i] : e.place) : 0.0f;
        }
        st.grip[i].letGoAt = st.clock;
        if (s.vanish > 0.0f && !e.unseen)
            st.shownSide = i, st.shownPlace = st.plannedAt[i] != Place::kNone ? st.plannedAt[i] : e.place, st.shownAt = st.clock;
    }
    if ((a.letGo == 0 || a.letGo == 1) && a.letGo != a.stowed) {
        const int i = a.letGo;
        st.grip[i].letGoAt = st.clock;
        if (st.stowWhy[i] != Why::kNone) {
            Event e;
            e.what = What::kStowRefused, e.side = i, e.slot = st.gameSlot, e.why = st.stowWhy[i], e.at = st.stowWhyAt[i];
            e.place = st.gameSlot >= 0 ? places[st.gameSlot] : Place::kNone;
            Add(r, e);
            Double(r, st, s, i);
        }
    }
    if ((a.drawn == 0 || a.drawn == 1) && st.drawPlanned == a.drawn) {
        const int i = a.drawn;
        Event e;
        e.what = What::kDrawn, e.side = i, e.slot = st.gameSlot, e.place = st.gameSlot >= 0 ? places[st.gameSlot] : Place::kNone;
        e.unseen = st.drawWasHidden, e.since = st.drawHiddenFor;
        Add(r, e);
        if (!st.drawWasHidden) {
            Buzz(r, s, i, kClickAmp, kClickSeconds);
            r.sound = Sound::kDraw, r.pan = 0.0f, r.soundSlot = st.gameSlot;
        }
        if (st.gameSlot >= 0)
            st.heldAt[st.gameSlot] = st.clock;
    }
    // A refusal that waited out the take's window: dropped if the grasp took with that squeeze, said once the window is over or nothing lies to
    // be taken any more ("lying recall": the lying gun to its place instead, and the draw tried again).
    for (int i = 0; i < 2; ++i) {
        Refusal& q = st.refusal[i];
        if (!q.on)
            continue;
        // Dropped unsaid: a squeeze took something this frame (this hand's take, or the other's: a gun came into the hands), or the gun that lay
        // isn't lying any more ("lies on the ground" would be false then).
        if (a.spent[0] || a.spent[1] || !f.on || (q.event.why == Why::kLying && f.holder != Holder::kLoose)) {
            q = Refusal{};
            continue;
        }
        const HandIn& h = f.hand[i];
        if (st.clock - q.crossedAt > kReachWindow || !h.read || h.squeeze < kTake || !f.anyTake) {
            if (q.recall && f.holder == Holder::kLoose) {
                r.recall = true;
                st.recallSide = i, st.recallPlace = q.event.place, st.recallAt = st.clock;
                Event c = q.event;
                c.what = What::kRecall;
                Add(r, c);
            } else {
                Add(r, q.event);
                Double(r, st, s, i);
            }
            q = Refusal{};
        }
    }
    // A refusal's second pulse.
    for (int i = 0; i < 2; ++i)
        if (st.pulseAt[i] >= 0.0f && st.clock >= st.pulseAt[i] - 1e-4f && r.buzz[i].seconds == 0.0f) {
            Buzz(r, s, i, kDoubleAmp, kDoubleSeconds);
            st.pulseAt[i] = -1.0f;
        }
    // "vanish": the gun still in the open hand while it rests at its place.
    if (st.shownSide >= 0) {
        const int i = st.shownSide;
        float from[3];
        const bool there = f.body && f.hand[i].seen == Seen::kSeen && FromNeck(f.hand[i].at, f.neck, f.yaw, from) &&
                           Size(EggOf(st.shownPlace, s), from) <= kLeaveSize;
        if (a.stowed == i || (there && st.clock - st.shownAt < s.vanish && f.holder == Holder::kStowed && a.drawn < 0))
            r.stillShown[i] = true;
        else
            st.shownSide = -1;
    }
}

// ---- The game's new weapon while the gun is put away (OnGunChanged's holster branch, before Update) ----

enum class Arrival : uint8_t {
    kToday,   // today's (gunprofile::OnChange): the gun before was in the hands or on the ground, a take, a new life, or holsters off
    kHand,    // into that hand by its pistol grip
    kAtPlace  // kept at its place, put away (the grasp kStowed)
};
struct ArriveIn {
    bool stowed = false;   // the gun before was put away
    bool take = false;     // a dropped gun taken by hand arriving (into the hand that took it)
    bool newLife = false;  // a new life, a checkpoint, a cutscene's end
    int slot = -1;         // the new gun's slot (the game's slot now; -1 not read: the draw waiting's, if one waits)
    bool placed[2] = {};   // each hand with a place now, raw: seen, or the runtime's own guess (never input's held place)
};
struct ArriveOut {
    Arrival arrival = Arrival::kToday;
    int side = -1;
    Event event;           // kCameAway when kept at its place for want of a hand (what = kNone otherwise)
};
inline ArriveOut Arrive(const Settings& s, const ArriveIn& in, State& st) {
    ArriveOut out;
    if (in.slot >= 0 && in.slot < kSlots)
        st.gameSlot = in.slot, st.slotUnread = 0.0f;
    if (!s.on || in.take || in.newLife || !in.stowed) {
        st.wait = Wait{};  // (into a hand by today's rule: nothing waits for it any more, and no second key comes after it)
        return out;
    }
    Wait& w = st.wait;
    if (w.on && !w.gameWay && (w.slot == st.gameSlot || in.slot < 0)) {
        if (in.slot < 0 && w.slot >= 0)
            st.gameSlot = w.slot;
        if (w.keeps && w.side >= 0 && in.placed[w.side]) {
            out.arrival = Arrival::kHand, out.side = w.side;
            st.heldAt[st.gameSlot] = st.clock;
            w = Wait{};
            return out;
        }
        out.arrival = Arrival::kAtPlace;
        if (w.keeps) {
            w.landed = w.arrived = w.hidden = true;  // (the hand has no place now: drawn into it once it has one again)
            return out;
        }
        out.event.what = What::kCameAway, out.event.side = w.side, out.event.slot = st.gameSlot, out.event.place = w.place;
        out.event.why = w.since < s.keyGiveUp ? Why::kOpened : Why::kLate;
        w = Wait{};
        return out;
    }
    if (st.clock - st.pickAt <= kPickKeeps || s.gameSwitchHand) {
        out.arrival = Arrival::kHand, out.side = 1;  // (a pick, X held's pickup among them, or "gameswitch hand": the right hand)
        w = Wait{};  // (a draw of another slot that waited ends: the gun is in a hand now, and a second key would take it away)
        return out;
    }
    out.arrival = Arrival::kAtPlace;
    out.event.what = What::kCameAway, out.event.slot = st.gameSlot, out.event.why = Why::kNone;
    return out;
}

// ---- The lines (hands.cpp logs them; names[slot]: the gun's own name, pickups::Nice's "M416", "SMAW", "AN-94"; null or "": "the gun") ----

namespace detail {
inline void Put(char* out, size_t size, int& n, const char* format, ...) {
    if (n < 0 || static_cast<size_t>(n) + 1 >= size)
        return;
    va_list args;
    va_start(args, format);
    const int k = std::vsnprintf(out + n, size - static_cast<size_t>(n), format, args);
    va_end(args);
    if (k > 0)
        n = static_cast<size_t>(n + k) < size ? n + k : static_cast<int>(size) - 1;
}
}  // namespace detail

inline int Say(const Event& e, const char* const names[kSlots], char* out, size_t size) {
    using detail::Put;
    if (!out || size == 0)
        return 0;
    out[0] = 0;
    int n = 0;
    char a[64], b[64];
    const auto gun = [names](int slot, char* buf, size_t len) {
        const char* g = slot >= 0 && slot < kSlots && names && names[slot] && names[slot][0] ? names[slot] : nullptr;
        if (g)
            std::snprintf(buf, len, "the %s", g);
        else
            std::snprintf(buf, len, "the gun");
        return buf;
    };
    const char* hand = e.side == 0 ? "left" : e.side == 1 ? "right" : "no";
    const char* otherHand = e.side == 0 ? "right" : "left";
    const auto unseenPart = [&]() {
        if (e.unseen)
            Put(out, size, n, " (not seen for %.2f s: last seen %.0f cm from it, going at %.1f m/s)", e.unseenFor, e.lastOff * 100.0f, e.lastSpeed);
    };
    switch (e.what) {
    case What::kStowed:
        Put(out, size, n, "Holster: %s put away %s by the %s hand", gun(e.slot, a, sizeof(a)), AtName(e.place), hand);
        if (e.at != Place::kNone)
            Put(out, size, n, " (let go %s: its own place is %s)", AtName(e.at), Name(e.place));
        unseenPart();
        break;
    case What::kDrawn:
        if (e.off)
            Put(out, size, n, "Holster: holsters off: %s back in the right hand", gun(e.slot, a, sizeof(a)));
        else if (e.unseen)
            Put(out, size, n, "Holster: %s in the %s hand (seen again after %.2f s)", gun(e.slot, a, sizeof(a)), hand, e.since);
        else
            Put(out, size, n, "Holster: %s drawn %s by the %s hand", gun(e.slot, a, sizeof(a)), FromName(e.place), hand);
        break;
    case What::kAsked:
        Put(out, size, n, "Holster: %s asked for %s by the %s hand", gun(e.slot, a, sizeof(a)), FromName(e.place), hand);
        unseenPart();
        if (e.key > 0)
            Put(out, size, n, ": key %d", e.key);
        if (e.unseen)
            Put(out, size, n, "%s in the hand once it's seen", e.key > 0 ? "," : ":");
        break;
    case What::kKeyAgain:
        Put(out, size, n, "Holster: %s: the game's own weapon still in slot %d after %.2f s: key %d again", gun(e.slot, a, sizeof(a)), e.other, e.since,
            e.key);
        break;
    case What::kNoSwitch:
        Put(out, size, n, "Holster: %s not drawn: the game didn't switch to it (%.2f s, %s)", gun(e.slot, a, sizeof(a)), e.since,
            e.twice ? "its key pressed twice" : "no second key: one could have done harm then");
        break;
    case What::kRefused:
        switch (e.why) {
        case Why::kNothing: Put(out, size, n, "Holster: nothing %s", AtName(e.place)); break;
        case Why::kOtherHolds:
            Put(out, size, n, "Holster: nothing drawn %s: the %s hand holds %s", FromName(e.place), otherHand, gun(e.other, b, sizeof(b)));
            break;
        case Why::kLying:
            Put(out, size, n, "Holster: nothing drawn %s: %s lies on the ground: take it first", FromName(e.place), gun(e.other, b, sizeof(b)));
            break;
        case Why::kUnknown:
            Put(out, size, n, "Holster: %s not drawn: the hands don't know it yet (the D-pad or Y gives it the game's way)", gun(e.slot, a, sizeof(a)));
            break;
        case Why::kNoKey: Put(out, size, n, "Holster: %s not drawn: no key reaches slot %d", gun(e.slot, a, sizeof(a)), e.slot); break;
        case Why::kBusy: Put(out, size, n, "Holster: the %s hand's squeeze %s: the hand isn't empty", hand, AtName(e.place)); break;
        case Why::kMoment:
            Put(out, size, n, "Holster: the %s hand's squeeze %s: not now (a scripted moment, a reload, a take or a draw under way)", hand,
                AtName(e.place));
            break;
        case Why::kSlotUnread:
            Put(out, size, n, "Holster: the %s hand's squeeze %s: the game's own weapon not read yet", hand, AtName(e.place));
            break;
        default: Put(out, size, n, "Holster: nothing drawn %s", FromName(e.place)); break;
        }
        unseenPart();
        break;
    case What::kStowRefused:
        Put(out, size, n, "Holster: %s not put away %s: ", gun(e.slot, a, sizeof(a)), AtName(e.at));
        if (e.why == Why::kOtherRocket || e.why == Why::kOtherRound || e.why == Why::kOtherMag)
            Put(out, size, n, "the %s hand holds %s", otherHand,
                e.why == Why::kOtherRocket ? "a rocket or a case" : e.why == Why::kOtherRound ? "a 40 mm round" : "its magazine");
        else if (e.why == Why::kOtherHolds)
            Put(out, size, n, "the %s hand holds it by its front", otherHand);
        else
            Put(out, size, n, "not now (a scripted moment, a reload, the grenade launcher's shot)");
        if (e.kept) {
            unseenPart();
            Put(out, size, n, ": it stays in the hand");
        } else {
            Put(out, size, n, ", so it's let go of as anywhere");
        }
        break;
    case What::kTick:
        Put(out, size, n, "Holster: the %s hand %s: a tick (%s can be %s there)", hand, AtName(e.place), gun(e.slot, a, sizeof(a)),
            e.away ? "put away" : "drawn");
        break;
    case What::kCalledOff:
        Put(out, size, n, "Holster: %s called off after %.2f s: %s (it stays at its place)", gun(e.slot, a, sizeof(a)), e.since,
            e.why == Why::kOpened   ? "the grip opened before the hand was seen"
            : e.why == Why::kPicked ? "a pick of another gun came first"
            : e.why == Why::kMoment ? "the game's weapon moved on meanwhile, or a scripted moment lasted"
                                    : "the hand not seen again in time");
        break;
    case What::kCameAway:
        if (e.why == Why::kOpened)
            Put(out, size, n, "Holster: %s came after the %s hand opened: it is at its place %s", gun(e.slot, a, sizeof(a)), hand, AtName(e.place));
        else if (e.why == Why::kLate)
            Put(out, size, n, "Holster: %s came after the %s hand stopped waiting: it is at its place %s", gun(e.slot, a, sizeof(a)), hand,
                AtName(e.place));
        else
            Put(out, size, n, "Holster: the game switched to %s by itself: it is at its place", gun(e.slot, a, sizeof(a)));
        break;
    case What::kGameWay:
        Put(out, size, n, "Holster: %s asked for the game's way %s by the %s hand: key %d (the hands don't know it yet)", gun(e.slot, a, sizeof(a)),
            FromName(e.place), hand, e.key);
        break;
    case What::kRecall:
        Put(out, size, n, "Holster: %s left the ground for its place (the %s hand squeezed %s)", gun(e.other, b, sizeof(b)), hand, AtName(e.place));
        break;
    case What::kLapsed:
        Put(out, size, n, "Holster: %s not %s by the %s hand: free hands didn't take it in %.2f s (a scripted moment, or the hand not counted as "
            "tracked)", gun(e.slot, a, sizeof(a)), e.why == Why::kNone ? "put away" : "drawn", hand, e.since);
        break;
    case What::kLook:
        Put(out, size, n, "Holster: the %s hand %.0f cm right, %.0f cm up, %.0f cm forward of the neck point: %.2f of %s's size, %s", hand,
            e.from[0] * 100.0f, e.from[1] * 100.0f, e.from[2] * 100.0f, e.size, Name(e.place),
            e.seen == Seen::kSeen ? "seen" : e.seen == Seen::kGuessed ? "the runtime's guess" : "not seen");
        break;
    default: break;
    }
    return n;
}

// ---- build\holsters.txt ----

struct ParseReport {
    int lines = 0, read = 0, refused = 0;
    char why[4][224] = {};  // the first refused lines: "line 3 (stowspeed fast): ..."
};

namespace detail {

inline bool Space(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f'; }
inline bool Same(const char* b, const char* e, const char* word) {
    const size_t n = std::strlen(word);
    if (static_cast<size_t>(e - b) != n)
        return false;
    for (size_t i = 0; i < n; ++i) {
        char c = b[i];
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
        if (c != word[i])
            return false;
    }
    return true;
}
struct Tok {
    const char* p;
    const char* end;
    bool Next(const char*& b, const char*& e) {
        while (p < end && Space(*p))
            ++p;
        if (p >= end)
            return false;
        b = p;
        while (p < end && !Space(*p))
            ++p;
        e = p;
        return true;
    }
};
inline bool Number(const char* b, const char* e, float& v) {
    char buf[32];
    const size_t n = static_cast<size_t>(e - b);
    if (n == 0 || n >= sizeof(buf))
        return false;
    std::memcpy(buf, b, n);
    buf[n] = 0;
    char* stop = nullptr;
    v = std::strtof(buf, &stop);
    return stop == buf + n && std::isfinite(v);
}

}  // namespace detail

// One line of holsters.txt into s: true when read (a blank line or a comment, "#" or "//", reads as nothing: blank). A bad line leaves s as it was
// and says why. Words after a line's values are its comment, as the design's own listing has them.
inline bool ParseLine(const char* begin, const char* end, Settings& s, char* why, size_t whySize, bool* blank = nullptr) {
    using namespace detail;
    if (blank)
        *blank = false;
    if (why && whySize)
        why[0] = 0;
    Tok t{begin, end};
    const char *kb = nullptr, *ke = nullptr;
    if (!t.Next(kb, ke) || *kb == '#' || (ke - kb >= 2 && kb[0] == '/' && kb[1] == '/')) {
        if (blank)
            *blank = true;
        return true;
    }
    const auto no = [&](const char* text) {
        if (why && whySize)
            std::snprintf(why, whySize, "%s", text);
        return false;
    };
    const auto numbers = [&](float* v, int count) {
        for (int i = 0; i < count; ++i) {
            const char *b = nullptr, *e = nullptr;
            if (!t.Next(b, e) || !Number(b, e, v[i]))
                return false;
        }
        return true;
    };
    const auto onOff = [&](bool& to, const char* text) {
        const char *b = nullptr, *e = nullptr;
        if (!t.Next(b, e))
            return no(text);
        if (Same(b, e, "1") || Same(b, e, "on"))
            to = true;
        else if (Same(b, e, "0") || Same(b, e, "off"))
            to = false;
        else
            return no(text);
        return true;
    };
    const auto choice = [&](bool& to, const char* first, const char* second, const char* text) {
        const char *b = nullptr, *e = nullptr;
        if (!t.Next(b, e))
            return no(text);
        if (Same(b, e, first))
            to = false;
        else if (Same(b, e, second))
            to = true;
        else
            return no(text);
        return true;
    };
    const auto within = [](float v, float lo, float hi) { return v >= lo && v <= hi; };
    const auto egg = [&](Egg& to, int side, const char* text) {
        float v[6];
        if (!numbers(v, 6))
            return no(text);
        for (int i = 0; i < 3; ++i)
            if (!within(v[i], -1.0f, 1.0f) || !within(v[3 + i], 0.01f, 0.5f))
                return no(text);
        if ((side > 0 && !(v[0] > 0.0f)) || (side < 0 && !(v[0] < 0.0f)))
            return no(text);
        for (int i = 0; i < 3; ++i)
            to.mid[i] = v[i], to.half[i] = v[3 + i];
        return true;
    };
    Settings n = s;  // (a bad line leaves s as it was)
    bool ok = true;
    if (Same(kb, ke, "holsters")) {
        ok = onOff(n.on, "holsters 1 or 0");
    } else if (Same(kb, ke, "launcher")) {
        const char *b = nullptr, *e = nullptr;
        if (!t.Next(b, e))
            ok = no("launcher shoulder, neck, rib or off");
        else if (Same(b, e, "shoulder"))
            n.launcher = Launcher::kShoulder;
        else if (Same(b, e, "neck"))
            n.launcher = Launcher::kNeck;
        else if (Same(b, e, "rib"))
            n.launcher = Launcher::kRib;
        else if (Same(b, e, "off"))
            n.launcher = Launcher::kOff;
        else
            ok = no("launcher shoulder, neck, rib or off");
    } else if (Same(kb, ke, "shoulder")) {
        ok = egg(n.shoulder, 1, "shoulder: the right one's middle (its x over 0) and half-sizes, six numbers in metres (the middle within 1, half-sizes 0.01 to 0.5)");
    } else if (Same(kb, ke, "neck")) {
        ok = egg(n.neck, 0, "neck: its middle and half-sizes, six numbers in metres (the middle within 1, half-sizes 0.01 to 0.5)");
    } else if (Same(kb, ke, "rib")) {
        ok = egg(n.rib, -1, "rib: the left one's middle (its x under 0) and half-sizes, six numbers in metres (the middle within 1, half-sizes 0.01 to 0.5)");
    } else if (Same(kb, ke, "stowspeed")) {
        float v = 0.0f;
        ok = numbers(&v, 1) && within(v, 0.1f, 20.0f) ? (n.stowSpeed = v, true) : no("stowspeed: m/s, 0.1 to 20");
    } else if (Same(kb, ke, "anyholster")) {
        ok = onOff(n.anyHolster, "anyholster 1 or 0");
    } else if (Same(kb, ke, "othergun")) {
        ok = choice(n.otherGunStow, "refuse", "stow", "othergun refuse or stow");
    } else if (Same(kb, ke, "lying")) {
        ok = choice(n.lyingRecall, "refuse", "recall", "lying refuse or recall");
    } else if (Same(kb, ke, "twolaunchers")) {
        ok = choice(n.twoLaunchersBySlot, "last", "slot", "twolaunchers last or slot");
    } else if (Same(kb, ke, "vanish")) {
        float v = 0.0f;
        ok = numbers(&v, 1) && within(v, 0.0f, 2.0f) ? (n.vanish = v, true) : no("vanish: seconds, 0 to 2");
    } else if (Same(kb, ke, "unknown")) {
        ok = choice(n.unknownGame, "refuse", "game", "unknown refuse or game");
    } else if (Same(kb, ke, "gameswitch")) {
        ok = choice(n.gameSwitchHand, "stay", "hand", "gameswitch stay or hand");
    } else if (Same(kb, ke, "unseen")) {
        // "unseen 0" (none), "unseen 1" (on, every number its default), or seven numbers.
        float v[7] = {};
        int count = 0;
        const char *b = nullptr, *e = nullptr;
        while (count < 7 && t.Next(b, e) && Number(b, e, v[count]))
            ++count;
        if (count == 1 && (v[0] == 0.0f || v[0] == 1.0f)) {
            const Settings d;
            n.unseen = v[0] == 1.0f;
            n.unseenTake = d.unseenTake, n.unseenStow = d.unseenStow, n.approach = d.approach, n.leastSpeed = d.leastSpeed;
            n.degrees = d.degrees, n.guessMost = d.guessMost, n.above = d.above;
        } else if (count == 7 && within(v[0], 0.0f, 3.0f) && within(v[1], 0.0f, 8.0f) && within(v[2], 0.0f, 0.5f) && within(v[3], 0.0f, 2.0f) &&
                   within(v[4], 0.0f, 180.0f) && within(v[5], 0.0f, 1.0f) && within(v[6], -0.2f, 0.3f)) {
            n.unseen = true;
            n.unseenTake = v[0], n.unseenStow = v[1], n.approach = v[2], n.leastSpeed = v[3], n.degrees = v[4], n.guessMost = v[5], n.above = v[6];
        } else {
            ok = no("unseen 0, unseen 1, or seven numbers: take s (0-3), put-away s (0-8), approach m (0-0.5), least m/s (0-2), degrees (0-180), "
                    "the guess's most m (0-1), above m (-0.2 to 0.3)");
        }
    } else if (Same(kb, ke, "unseenshoulders")) {
        ok = onOff(n.unseenShoulders, "unseenshoulders 1 or 0");
    } else if (Same(kb, ke, "unseendrop")) {
        ok = onOff(n.unseenDrop, "unseendrop 1 or 0");
    } else if (Same(kb, ke, "draw")) {
        float v[2] = {};
        ok = numbers(v, 2) && within(v[0], 0.0f, kReachWindow) && within(v[1], 0.05f, 5.0f)
                 ? (n.drawWindow = v[0], n.drawSpeed = v[1], true)
                 : no("draw: seconds after the grip's crossing (0 to 0.4) and the hand's m/s at most (0.05 to 5)");
    } else if (Same(kb, ke, "stowsound")) {
        const char *b = nullptr, *e = nullptr;
        if (!t.Next(b, e))
            ok = no("stowsound vest, settle or none");
        else if (Same(b, e, "vest"))
            n.stowSound = StowSound::kVest;
        else if (Same(b, e, "settle"))
            n.stowSound = StowSound::kSettle;
        else if (Same(b, e, "none"))
            n.stowSound = StowSound::kNone;
        else
            ok = no("stowsound vest, settle or none");
    } else if (Same(kb, ke, "stowpan")) {
        ok = choice(n.stowPanSide, "middle", "side", "stowpan middle or side");
    } else if (Same(kb, ke, "buzz")) {
        float v = 0.0f;
        ok = numbers(&v, 1) && within(v, 0.0f, 2.0f) ? (n.buzz = v, true) : no("buzz: 0 (none), 1, or a strength up to 2");
    } else if (Same(kb, ke, "mirror")) {
        ok = onOff(n.mirror, "mirror 1 or 0");
    } else if (Same(kb, ke, "log")) {
        ok = onOff(n.log, "log 1 or 0");
    } else if (Same(kb, ke, "heldmag")) {
        ok = choice(n.heldMagRefuse, "fall", "refuse", "heldmag fall or refuse");
    } else if (Same(kb, ke, "keyagain")) {
        float v[2] = {};
        ok = numbers(v, 2) && within(v[0], 0.1f, 5.0f) && within(v[1], 0.2f, 10.0f) && v[1] > v[0]
                 ? (n.keyAgain = v[0], n.keyGiveUp = v[1], true)
                 : no("keyagain: seconds to the second key (0.1 to 5) and to giving up (later, at most 10)");
    } else {
        ok = no("not a holsters.txt word");
    }
    if (ok)
        s = n;
    return ok;
}

// The whole file: every default, then each line in turn (the last of a word wins). A first UTF-8 mark is skipped; a line ends at LF, CR LF or CR.
inline Settings Parse(const char* text, size_t length, ParseReport* report = nullptr) {
    Settings s;
    ParseReport rep;
    if (text) {
        const char* p = text;
        const char* end = text + length;
        if (length >= 3 && static_cast<unsigned char>(p[0]) == 0xEF && static_cast<unsigned char>(p[1]) == 0xBB &&
            static_cast<unsigned char>(p[2]) == 0xBF)
            p += 3;
        while (p < end) {
            const char* e = p;
            while (e < end && *e != '\n' && *e != '\r' && *e != 0)
                ++e;
            ++rep.lines;
            char why[160];
            bool blank = false;
            if (ParseLine(p, e, s, why, sizeof(why), &blank)) {
                rep.read += !blank;
            } else {
                if (rep.refused < 4) {
                    char shown[40];
                    const size_t n = static_cast<size_t>(e - p) < sizeof(shown) - 1 ? static_cast<size_t>(e - p) : sizeof(shown) - 1;
                    std::memcpy(shown, p, n);
                    shown[n] = 0;
                    std::snprintf(rep.why[rep.refused], sizeof(rep.why[0]), "line %d (%s): %s", rep.lines, shown, why);
                }
                ++rep.refused;
            }
            if (e >= end || *e == 0)
                break;
            if (*e == '\r' && e + 1 < end && e[1] == '\n')
                ++e;
            p = e + 1;
        }
    }
    if (report)
        *report = rep;
    return s;
}

// ---- The holsters' heading (hands.cpp gives it to Frame::yaw) ----
// The places are measured from the neck by a heading. The pouch's (the belly's) is dragged with the head past 35 degrees and settles on
// it 5 degrees a second; the shoulders sit ~18 cm out from the neck, so that moved them off the real shoulders whenever a seated player
// looked toward one (0.1.4's review: a 60-degree look moved them 25 degrees at once, and a put-away there dropped the gun instead). Here
// a look moves nothing: the heading follows the head only once it has stayed more than kHeadLag away for kHeadDwell s (the whole body
// turned), then all the way at kHeadTurn; past kHeadDrag at once (turning round); a small drift settles at kHeadSettle while the head is
// still. Nothing but the drag while a hand is up by the neck and shoulders (within kHeadNear of the neck point, above kHeadBelow under
// it): a hand at a holster keeps the places where they are. A jump of the head's heading (a recentre) starts it afresh.
constexpr float kHeadDeg = 3.14159265f / 180.0f;
constexpr float kHeadLag = 25.0f * kHeadDeg, kHeadDrag = 100.0f * kHeadDeg, kHeadDwell = 2.5f, kHeadTurn = 90.0f * kHeadDeg,
                kHeadSettle = 2.0f * kHeadDeg, kHeadSteady = 20.0f * kHeadDeg, kHeadJump = 45.0f * kHeadDeg, kHeadNear = 0.40f,
                kHeadBelow = -0.30f;
struct Heading {
    float yaw = 0.0f, head = 0.0f, away = 0.0f;  // radians; away: s the head has been more than kHeadLag off
    bool have = false, follow = false;
};
inline float WrapHeading(float a) { return std::remainder(a, 2.0f * 3.14159265f); }
// This frame's heading from the head's (radians, the room's vertical axis), the body's to start from (the pouch's), the hands in f.
inline float FollowHeading(Heading& h, float bodyYaw, float head, const Frame& f, float dt) {
    const float jump = std::fabs(WrapHeading(head - h.head));
    const float headTurn = dt > 0.0f ? jump / dt : 0.0f;
    h.head = head;
    if (!h.have || jump > kHeadJump || !(dt >= 0.0f) || !std::isfinite(head) || !std::isfinite(bodyYaw)) {
        h.yaw = std::isfinite(bodyYaw) ? bodyYaw : head;
        h.have = std::isfinite(h.yaw);
        h.away = 0.0f;
        h.follow = false;
        return h.yaw;
    }
    bool up = false;
    for (int s = 0; s < 2; ++s) {
        const HandIn& hand = f.hand[s];
        if (hand.seen == Seen::kNone)
            continue;
        const float d[3] = {hand.at[0] - f.neck[0], hand.at[1] - f.neck[1], hand.at[2] - f.neck[2]};
        if (d[1] > kHeadBelow && d[0] * d[0] + d[1] * d[1] + d[2] * d[2] < kHeadNear * kHeadNear)
            up = true;
    }
    const float apart = WrapHeading(head - h.yaw);
    const float off = std::fabs(apart);
    h.away = off > kHeadLag ? h.away + dt : 0.0f;
    if (h.away > kHeadDwell && !up)
        h.follow = true;
    if (off < 2.0f * kHeadDeg)
        h.follow = false;
    float turn = 0.0f;
    if (off > kHeadDrag)
        turn = off - kHeadDrag;
    else if (h.follow && !up)
        turn = std::fmin(off, kHeadTurn * dt);
    else if (!up && off <= kHeadLag && headTurn < kHeadSteady)
        turn = std::fmin(off, kHeadSettle * dt);
    h.yaw = WrapHeading(h.yaw + (apart > 0.0f ? turn : -turn));
    return h.yaw;
}

}  // namespace holster

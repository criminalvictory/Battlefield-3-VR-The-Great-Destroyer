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
//   the right shoulder    the primary
//   the left shoulder     the secondary, and the launchers (2026-11-15) while no secondary hangs there
//   the back of the neck  the launchers instead ("launcher neck", or while a secondary is at the left shoulder)
//   the left rib cage     the launchers instead, with holsters.txt "launcher rib"
//   the chests            reserved: they answer nothing yet
// "mirror 1" swaps the shoulders and puts the rib on the right. Either hand puts away and draws, and the hand that draws is the trigger hand.
// Holsters by hand only (the user, 2026-11-18, "Listen carefully": "Don't let guns get automatically put into holsters if they're picked up from the
// ground.. it needs to be physically placed on the shoulder to lock as a permanent gun primary or secondary unless it is physically removed from the
// holster and dropped"). Each slot's gun has a home (State::home): the role a hand gave it by putting it away (the shoulder it was put on: the right
// the primary's, the left the secondary's, whatever the game's slot; a launcher the launcher's place), or the kit's at a new life (the user,
// 2026-10-31: whatever the player starts with has its holster from the start): the one the memo has for its kind (a death and its checkpoint keep
// each gun's), else its slot's. A gun taken off the ground (a hand's take, the magnet's, the game's own R: Frame::taken) has none till a hand puts it
// on a shoulder, nor one the game gives with no take later on (the roof's AT4 by its kit); a gun on the ground loses its home; one gun a shoulder
// (the launchers' place is shared). Only a gun with a home is at a holster (drawn from there, kept there by the game's switches); one with none that
// isn't in the hands is in the game's slot on no shoulder, the game's way (the D-pad, Y) its only way back (kNoPlace).
// A holstered gun the game takes out of its slot stays on its shoulder (Away; the user's AT4, 2026-11-18: "a smart player would holster their rifle
// before picking up the AT4 ... shoot it, discard it, then pick up their rifle again"). A take off the ground puts the gun of each slot the pickup
// names into that pickup (the game's take, 008AB870: pickrules.h): the take goes ahead, and the holstered gun is remembered there, in that pickup,
// with the rounds it went with; a draw at its shoulder asks the game's own take of that pickup (Result::giveBack: the gun back into its slot and into
// the drawing hand, the slot's gun then into the pickup, where it lay), within the game's reach of it (its check, 008B6380: 2.5 m from the soldier
// and in sight). A script's kit that takes it with no pickup (Operation Swordbreaker's roof AT4 into the M4A1's slot), the gun held or hanging,
// leaves it on its shoulder till the game gives a gun of its kind back into that slot (the level's own kit again, which also ends a record in a
// pickup left behind). Only a plain floor gun's take that couldn't give it back is refused before the game is asked (TakeCheck: a pickup that destroys
// the gun, one that takes every gun, one with more entries than are read), with the double click; the level's own pickups never are (Rock's Stinger
// for the jet): a holstered gun one destroys is gone, as the game does it, said. A shoulder is never blocked for good: a hand's put-away at a shoulder
// an away gun has wins (that gun stays in its pickup on the ground, on no shoulder, said), and a put-away refused at a place (another carried gun has
// that shoulder, what the other hand holds, a moment) keeps the gun in the hand, never dropped (the user, 2026-11-16: "it just keeps falling to the
// ground"). The memo of each kind's holster never crosses levels: a new life's kit is the same level's only while every gun of it with a holster by
// its slot was carried in the life before.
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
constexpr float kStowGraceSize = 1.6f;  // ... while within this Size of it, stowMore added (2026-11-16, the user in the headset: "make it a
                                         // little bit easier to holster your gun": 8 drops, 5 of them 0.4-1.0 s after the tick)
constexpr float kLeaveSize = 1.1f;     // a hand in a place has left it past this Size (the tick's: an edge doesn't tick twice)
constexpr float kAhead = 0.1f;         // s on: a hand not seen, moving, stays at the place it's in if it's still in it this much later
constexpr float kGuessGrow = 0.2f;     // m: the runtime's guess for a hand not seen within its place grown by this, or the place is given up
constexpr float kSlotStale = 0.5f;     // s the game's own slot may go unread (a moment's gap) before nothing is decided on its last read (a load, a death)
constexpr float kLogNear = 0.5f;       // m from the neck point: "log 1" says a hand's place once a second within it
constexpr float kKitSeconds = 10.0f;   // s from a new life's start (Clear) a gun first seen in a slot is the kit's: at its place by its slot
constexpr float kTakeNear = 1.0f;      // s apart a take's mark (Frame::taken) and a new gun in that slot are the same take (either may come first)
constexpr uint32_t kUnknownId = 0xFFFFFFFFu;  // a slot's identity when the reader gives none (SlotIn::id 0): the same gun while it stays carried
constexpr int kMostAway = 6;           // holstered guns the game took out of their slots, kept on their shoulders (Away)
constexpr int kMostMemo = 48;          // guns' kinds remembered through a new life (Memo; the oldest written goes first)
constexpr int kMostCarried = 64;       // kinds carried in a life (State::carried; past it a kind isn't noted: the next life's kit taken as another level's)
constexpr int kMostPending = 6;        // lines noted outside Update (NoteSlots in Arrive, NoteSwap, GiveBackFailed), said at the next Update
constexpr int kNameLen = 24;           // a gun's own name for the lines ("M416")
constexpr float kLostAfter = 3.0f;     // s an away gun's pickup may be missing from the pickups seen (or hold another gun) before it's given up
constexpr float kGiveBackMost = 3.0f;  // s a draw from a shoulder waits at most for its gun from the game's take of its pickup
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
    float stowSpeed = 2.2f;                                        // m/s: a let-go faster is a throw (1.5 until 2026-11-16: a natural release
                                                                   // over the shoulder counted as one, the user's gun fell)
    float stowMore = 0.03f;                                        // m added round a place's egg for a put-away (the tick's own egg inside it)
    float stowGrace = 0.5f;                                        // s a put-away still holds after the hand was last in the place, ...
    bool anyHolster = true;                                        // 0: only a place where the gun stays puts it away (StowTo)
    // othergun refuse | stow and lying refuse | recall are read but answer nothing since 2026-11-18 (a gun goes into a holster only by a hand): a
    // draw while the other hand holds the gun, or while the game's weapon lies, is refused whatever they say.
    bool otherGunStow = false;
    bool lyingRecall = false;
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
    uint32_t id = 0;        // which gun it is (ammo::SlotIdentity's weapon object: the game makes a new one at every swap and new life); 0 not given
    uint32_t kind = 0;      // ... its kind (NameKey of its blueprint's name: the same through a new life; the same gun read afresh keeps it); 0 not given
    char name[kNameLen] = {};  // its own name for the lines ("M416"; hands.cpp's HolsterNameOf), "" not given
};

// A gun's kind from its blueprint's name ("Weapons/M416/M416"), by its letters and digits in any case as recoilguns.h's SameName compares names:
// the same kind through a new life and a checkpoint (where the blueprint's own address may be another). 0 for none.
inline uint32_t NameKey(const char* name) {
    if (!name)
        return 0;
    uint32_t h = 2166136261u;
    bool any = false;
    for (const char* p = name; *p; ++p) {
        char c = *p;
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')))
            continue;
        h ^= static_cast<uint8_t>(c);
        h *= 16777619u;
        any = true;
    }
    return any ? (h ? h : 1u) : 0u;
}

inline void CopyName(char* out, const char* name) {
    size_t n = 0;
    if (name)
        for (; name[n] && n + 1 < static_cast<size_t>(kNameLen); ++n)
            out[n] = name[n];
    out[n] = 0;
}

// The game's key for a slot (input.cpp has 1, 2 and 4): slot 0 key 1, slot 1 key 2, slot 5 key 4; 0 none.
inline int KeyOf(int slot) { return slot == 0 ? 1 : slot == 1 ? 2 : slot == 5 ? 4 : 0; }

// A gun's holster as a role: the primary's shoulder, the secondary's, the launcher's place (which of them it is goes by the settings and by whether
// a secondary hangs at the left shoulder); none: at no holster.
enum class Role : uint8_t { kNone, kPrimary, kSecondary, kLauncher };

// The role a slot's gun has by its kind and its slot (the kit's map): launchers by their kind the launcher's whatever their slot ("twolaunchers
// slot": with two or more carried, one in slot 0 or 1 by its slot); every other gun by its slot: 0 the primary's, 1 the secondary's, 5 the
// launcher's; the rest none (the M320 under the rifle in 2, the claymore in 4, the M67 in 6, the knife in 7, a pistol in 9: no key reaches them, and
// the chests are reserved). A gun with none is never put away.
inline Role KitRole(const SlotIn slots[kSlots], const Settings& s, int i) {
    if (i < 0 || i >= kSlots || !slots[i].carried)
        return Role::kNone;
    int launchers = 0;
    for (int k = 0; k < kSlots; ++k)
        launchers += slots[k].carried && slots[k].launcher;
    const bool bySlot = !slots[i].launcher || (s.twoLaunchersBySlot && launchers >= 2 && (i == 0 || i == 1));
    if (!bySlot)
        return Role::kLauncher;
    return i == 0 ? Role::kPrimary : i == 1 ? Role::kSecondary : i == 5 ? Role::kLauncher : Role::kNone;
}

// A role's place now: the shoulders; the launcher's ("launcher shoulder": the secondary's shoulder, behind the neck instead while a secondary is
// there, secondaryTaken).
inline Place PlaceOfRole(Role r, bool secondaryTaken, const Settings& s) {
    switch (r) {
    case Role::kPrimary: return PrimaryPlace(s);
    case Role::kSecondary: return SecondaryPlace(s);
    case Role::kLauncher: return s.launcher == Launcher::kShoulder && secondaryTaken ? LauncherFallback(s) : LauncherPlace(s);
    default: return Place::kNone;
    }
}

// Each slot's place by the kit's map (a new life's guns, the game's gifts; and where a gun put away "anyholster 0" would stay). With "launcher
// shoulder", the secondary's shoulder is the launcher's unless slot 1's gun is there by its slot: then behind the neck.
inline void PlacesOf(const SlotIn slots[kSlots], const Settings& s, Place out[kSlots]) {
    Role roles[kSlots];
    bool secondaryTaken = false;
    for (int i = 0; i < kSlots; ++i) {
        roles[i] = KitRole(slots, s, i);
        secondaryTaken = secondaryTaken || roles[i] == Role::kSecondary;
    }
    for (int i = 0; i < kSlots; ++i)
        out[i] = PlaceOfRole(roles[i], secondaryTaken, s);
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
    unsigned taken = 0;        // bit s: a take off the ground changed slot s since the last frame (a hand's take or the game's own R: pickups' swaps)
    bool swapsSeen = true;     // the takes are seen (pickups hooked): a new gun with no take's mark is one the game gave
    // Each away gun (State::away, by its index) as the pickups seen now have its pickup (hands.cpp, from pickups' census): 1 there with that gun in
    // it, 0 there holding another, -1 not among them, -2 not known now (no pickups seen: nothing judged); in the game's reach (allowed, and the game's
    // own check of it passed: 2.5 m, in sight), and how far (m).
    int awayFound[kMostAway] = {-2, -2, -2, -2, -2, -2};
    bool awayReach[kMostAway] = {};
    float awayDistance[kMostAway] = {};
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
    kTaken,        // put-aways: that shoulder has another gun (one gun a shoulder; the launchers' place is shared)
    kFar,          // an away gun's draw: its pickup isn't within the game's reach (2.5 m, in sight), or isn't seen near
    kWithGame,     // ... the game took it out of its slot with no pickup (a script's kit): back once the game gives it back
    kGiveBack,     // ... the game's take of its pickup refused or never came (Event::words says what the game said)
    kHidden,       // ... a hand not seen (its gun comes into a seen hand only)
    kGone,         // takes: the pickup destroys the guns of the slots it names (UnspawnOnPickup, ContentIsStatic): a holstered one would be gone
    kAll,          // ... ReplaceAllContent: it would hold every gun carried, and giving one back would give back all of them as they were
    kUnread,       // ... more content entries than are read: what it would take isn't known
    kPart,         // ... it names the grenade launcher's slot alone: a part of the rifle at its shoulder
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
    kNoPlace,      // the game's weapon moved on from a gun with no home: it's in its slot on no shoulder (the D-pad or Y brings it)
    kBackHome,     // the game's own switch took the gun in the hands to its home, where it came from
    kToHand,       // the game's weapon now is a gun with no home while the hands were empty: into the right hand as at a spawn
    kAway,         // a holstered gun the game took out of its slot stays on its shoulder: in the pickup it went into (weak: with the game, no pickup)
    kComeBack,     // ... back in its slot, on its shoulder again (the game's take of its pickup, or the game gave it back)
    kLost,         // ... its pickup gone, or holding another gun: on no shoulder any more
    kDiscard,      // a picked-up gun let go of on the ground went into its slot for a draw (the game draws one gun at a time)
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
    bool noPlace = false;        // kDrawn: it was put away with no home (on no shoulder): back in the right hand, as at a spawn
    // The away guns' (they're no slot's gun now: names[slot] would name another), and what the game said.
    char gun[kNameLen] = {};       // the gun's own name ("M416"), when set: said instead of names[slot]
    char otherGun[kNameLen] = {};  // ... the other gun's (kTaken: the gun on that shoulder; kAway: the gun taken in its place)
    bool weak = false;             // kAway, kComeBack: with the game, no pickup (a script's kit took it, or a take the mod didn't see)
    bool pickupLeft = false;       // kComeBack, the game gave it back: one of its kind was away in a pickup, and stays there on no shoulder
    bool giveBack = false;         // kAsked, kNoSwitch, kRefused: a draw of an away gun, by the game's take of its pickup
    float distance = -1.0f;        // ... its pickup's distance (m; under 0 not known)
    char words[64] = {};           // kRefused kGiveBack: the game's (pickups') reason
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
    // A draw of an away gun (a holstered gun the game put into a pickup): hands.cpp asks the game's own take of that pickup into that hand
    // (pickups::RequestGiveBack, its first blueprint looked up there; blueprint: the away gun's own, which must still be in it); not asked:
    // GiveBackFailed. side -1: none.
    struct GiveBack {
        int side = -1, slot = -1, away = -1;
        uint32_t entity = 0, blueprint = 0;
    } giveBack;
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
    bool giveBack = false;    // an away gun's draw: no key, the game's take of its pickup asked (hands.cpp); its arrival is the take's
    int away = -1;            // ... the away record (State::away)
};

struct Ask {           // a draw or a put-away asked of the grasp, asked again each frame till it's done (Settle) or kAskMost
    int side = -1;
    float since = 0.0f;
    bool armed = true;                      // a draw: by a squeeze (a pick's isn't)
    bool hidden = false;                    // a draw: a hidden take's ...
    float hiddenFor = 0.0f;                 // ... s since its squeeze
    Place planned = Place::kNone, at = Place::kNone;  // a put-away: where it goes, where the hand was ...
    Role role = Role::kNone;                // ... and the role it takes there
};

struct Home {          // one slot's gun as holsters keep it (NoteSlots)
    uint32_t id = 0, kind = 0;  // the gun it's about (SlotIn's; 0 none seen this life)
    Role role = Role::kNone;    // its holster: given by a hand's put-away (or the kit's); kept while it's drawn into a hand, gone once it lies
    float seenAt = -1e9f;       // when that gun was first seen in the slot (State::clock)
    float takenAt = -1e9f;      // when a take off the ground last changed the slot (Frame::taken)
    bool byTake = false;        // that gun came by a take (its mark at its noting, or a frame after)
    char name[kNameLen] = {};
    // The gun before in this slot, a moment after another came (a take's swap told a frame after its new gun was seen still finds what went).
    uint32_t lastKind = 0;
    float lastAt = -1e9f;
    int lastAway = -1;          // the away record made for it then (-1 none)
    bool memoByKit = false;     // the kit's noting of this gun wrote its kind's memo (a take's mark a frame late makes it none: review finding 8)
};

// A holstered gun the game took out of its slot, still on its shoulder (the user, 2026-11-18: a holstered gun stays "unless it is physically removed
// from the holster and dropped"). Strong: in a pickup (entity), drawn back by the game's own take of it. Weak: with the game, no pickup known (a
// script's kit put another gun in its slot; a take the mod didn't see): back once the game gives a gun of its kind into that slot again.
struct Away {
    bool on = false;
    int slot = -1;              // the slot it was in, and goes back into
    uint32_t kind = 0;          // its kind (NameKey)
    Role role = Role::kNone;    // its holster
    uint32_t entity = 0;        // the pickup it's in (0: weak)
    uint32_t blueprint = 0;     // its blueprint (hands.cpp's, for the take's request: it must still be in that pickup)
    char name[kNameLen] = {};
    char instead[kNameLen] = {};  // the gun its take put in its place ("AT4"), for the lines
    float since = 0.0f;         // State::clock when it went
    float heldAt = -1e9f;       // when it was last in the hands (a shared place gives the one held last)
    float missing = 0.0f;       // s its pickup has been missing from the pickups seen, or holding another gun
    float sayAt = 0.0f;         // its line said then (a weak one a moment on: a take's swap may still tell its pickup)
    bool said = false;
};

// A gun's kind and the holster it last had by a hand, or the kit's, or none (picked up off the ground, given by the game): through a new life and a
// checkpoint (the review of 2026-11-18: a gun picked up and carried into a checkpoint was the kit's after a death).
struct Memo {
    uint32_t kind = 0;
    Role role = Role::kNone;
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
    float stowOkAt[2] = {-1e9f, -1e9f};  // the clock that hand could last put away (stowGrace) ...
    Place stowOkPlace[2] = {};      // ... and at which place
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
    Home home[kSlots];              // each slot's gun and its holster (the user, 2026-11-18: a gun goes into one only by a hand)
    Role plannedRole[2] = {};       // the role this frame's put-away by that hand gives (with planned)
    int lastSlot = -1;              // the game's weapon's slot last frame (-1 not known) ...
    Holder lastHolder = Holder::kHeld;  // ... and the grasp's holder then
    int lyingSlot = -1;             // the slot of the gun that lies (the game's weapon as it fell), -1 none
    bool drawNoPlace = false;       // the draw asked now brings a gun put away with no home back into the right hand ...
    bool toHandSaid = false;        // ... said once while it lasts
    float toHandAt = -1e9f;         // ... asked then (again a second on)
    Away away[kMostAway];           // holstered guns the game took out of their slots, still on their shoulders
    Memo memo[kMostMemo];           // each kind's holster through a new life
    int memoNext = 0;
    bool memoChecked = true;        // false from NewLife till its kit is first noted (a kit gun with a holster by its slot: KitCheck)
    bool kitWatch = false;          // from NewLife till the kit's seconds are over: each kit gun noted is checked (KitCheck)
    bool memoForgot = false;        // ... this life's kit was found another level's: the memo went
    // Every kind carried this life (NoteSlots), and the life before's (NewLife moves it there): a new life's kit is that level's while every gun of it
    // with a holster by its slot was carried in the life before (a death and its checkpoint give back what the player had), else another level's
    // (review finding 3: every kit shares the M67 and the knife, so "shares a kind" never told levels apart).
    uint32_t carried[kMostCarried] = {};
    int carriedCount = 0;
    uint32_t carriedBefore[kMostCarried] = {};
    int carriedBeforeCount = 0;
    // A put-away refused at a place (another carried gun has that shoulder, what the other hand holds, a moment): the gun stays in the hand (review
    // finding 1; the user, 2026-11-16: "it just keeps falling to the ground"). Each hand's last such place and when (a let-go just after it keeps it
    // too, as the put-away's grace), why, the gun there, and whether this opening was said.
    float keepAt[2] = {-1e9f, -1e9f};
    Place keepPlace[2] = {};
    Why keepWhy[2] = {};
    int keepTakerSlot[2] = {-1, -1};
    bool keptSaid[2] = {};
    struct Restore {                // an away gun's take seen (NoteSwap), its gun not noted in its slot yet
        uint32_t kind = 0;
        Role role = Role::kNone;
        float at = -1e9f;
        char name[kNameLen] = {};
    } restore[kSlots];
    int comebackSlot = -1;          // an away gun back in its slot just now (Arrive keeps it at its place, the gun before not in the hands) ...
    float comebackAt = -1e9f;
    Event pending[kMostPending];    // lines noted outside Update (NoteSlots in Arrive, NoteSwap, GiveBackFailed): said at its start
    int pendingCount = 0;
    bool doubleDue[2] = {};         // a refusal noted outside Update (GiveBackFailed): its double click at the next Update
    int stowTakerSlot[2] = {-1, -1}, stowTakerAway[2] = {-1, -1};  // kTaken: the gun on that shoulder (a slot's, or an away record)
    State() {
        for (float& v : heldAt)
            v = -1e9f;
    }
};

inline void Clear(State& s) { s = State{}; }

// A new life (a death, a checkpoint, another level: hands.cpp's f.newLife): everything afresh but the memo, so a gun keeps the holster it last had
// through a death and its checkpoint, and one picked up off the ground stays on no shoulder (NoteSlots); and the kinds this life carried, so the new
// life's kit is told the same level's or another's (KitCheck: the memo goes for another level's).
inline void NewLife(State& s) {
    State n;
    for (int i = 0; i < kMostMemo; ++i)
        n.memo[i] = s.memo[i];
    n.memoNext = s.memoNext;
    n.memoChecked = false;
    n.kitWatch = true;
    // (A life that carried nothing, the soldier gone a moment between two of hands.cpp's new lives (ammo's: the soldier unread over 1 s, then the new
    // one), keeps the life before it as it was.)
    const bool none = s.carriedCount <= 0;
    for (int i = 0; i < kMostCarried; ++i)
        n.carriedBefore[i] = none ? s.carriedBefore[i] : i < s.carriedCount ? s.carried[i] : 0u;
    n.carriedBeforeCount = none ? s.carriedBeforeCount : s.carriedCount < kMostCarried ? s.carriedCount : kMostCarried;
    s = n;
}

// Free hands turned off and on (the Insert key): what waited goes; every gun keeps its holster, the away guns stay, the clock runs on (the review of
// 2026-11-18: wiped, every gun carried was the kit's within 10 s, picked-up ones too).
inline void Reset(State& s) {
    State n;
    n.clock = s.clock;
    n.gameSlot = s.gameSlot, n.slotUnread = s.slotUnread, n.lastSlot = s.lastSlot, n.lastHolder = s.lastHolder;
    for (int k = 0; k < kSlots; ++k)
        n.heldAt[k] = s.heldAt[k], n.home[k] = s.home[k];
    for (int i = 0; i < kMostAway; ++i)
        n.away[i] = s.away[i];
    for (int i = 0; i < kMostMemo; ++i)
        n.memo[i] = s.memo[i];
    n.memoNext = s.memoNext, n.memoChecked = s.memoChecked, n.kitWatch = s.kitWatch, n.memoForgot = s.memoForgot;
    for (int i = 0; i < kMostCarried; ++i)
        n.carried[i] = s.carried[i], n.carriedBefore[i] = s.carriedBefore[i];
    n.carriedCount = s.carriedCount, n.carriedBeforeCount = s.carriedBeforeCount;
    s = n;
}

// ---- The memo: each kind's holster through a new life ----

inline const Memo* MemoOf(const State& st, uint32_t kind) {
    if (!kind)
        return nullptr;
    for (const Memo& m : st.memo)
        if (m.kind == kind)
            return &m;
    return nullptr;
}
// That kind's holster now (a hand's put-away, the kit's, or none: picked up, given); always replaces what it had.
inline void Remember(State& st, uint32_t kind, Role role) {
    if (!kind)
        return;
    for (Memo& m : st.memo)
        if (m.kind == kind) {
            m.role = role;
            return;
        }
    Memo& m = st.memo[st.memoNext % kMostMemo];
    st.memoNext = (st.memoNext + 1) % kMostMemo;
    m.kind = kind, m.role = role;
}
// ... only when it has none yet (a take of a gun the memo knows keeps what the memo says: the player's own rifle picked up again).
inline void RememberFirst(State& st, uint32_t kind, Role role) {
    if (!MemoOf(st, kind))
        Remember(st, kind, role);
}
inline void ForgetMemo(State& st) {
    for (Memo& m : st.memo)
        m = Memo{};
    st.memoNext = 0;
}
// A kind carried this life (noted once; past kMostCarried not noted).
inline void NoteCarried(State& st, uint32_t kind) {
    if (!kind)
        return;
    for (int i = 0; i < st.carriedCount && i < kMostCarried; ++i)
        if (st.carried[i] == kind)
            return;
    if (st.carriedCount < kMostCarried)
        st.carried[st.carriedCount++] = kind;
}
// ... in the life before.
inline bool CarriedBefore(const State& st, uint32_t kind) {
    for (int i = 0; i < st.carriedBeforeCount && i < kMostCarried; ++i)
        if (st.carriedBefore[i] == kind)
            return true;
    return false;
}
// A gun of that kind let go of on the ground may go into its slot for a draw of another (the game draws one gun at a time): one with no holster of
// its own by the memo (picked up off the ground, given by the game: the user's AT4, "meant to be thrown away"). The player's own gun (the kit's, or
// one a hand put on a shoulder) stays where it lies: a draw waits till it's taken (kLying).
inline bool Discardable(const State& st, uint32_t kind) {
    const Memo* m = MemoOf(st, kind);  // (a kind not known, or not in the memo: the player's own, as before)
    return m && m->role == Role::kNone;
}

// A line noted outside Update, said at its start.
inline void Queue(State& st, const Event& e) {
    if (st.pendingCount < kMostPending)
        st.pending[st.pendingCount++] = e;
}

// ---- The guns' homes: a gun goes into a holster only by a hand (the user, 2026-11-18) ----

// A slot's gun with a holster was hanging there last frame unless it was the game's weapon in the hands or on the ground (Update keeps the game's
// weapon's slot and the grasp's holder of the frame before). Not known then (unread past kSlotStale): every gun with a holster counts.
inline bool WasHung(const State& st, int k) {
    const bool known = st.lastSlot >= 0 && st.lastSlot < kSlots;
    return !known || k != st.lastSlot || st.lastHolder == Holder::kStowed;
}

// A gun at the secondary's shoulder by its home (not counting slot except), or an away one there in a pickup: the launchers go behind the neck then
// ("launcher shoulder").
inline bool SecondaryTaken(const SlotIn slots[kSlots], const State& st, int except = -1) {
    for (int k = 0; k < kSlots; ++k)
        if (k != except && slots[k].carried && st.home[k].role == Role::kSecondary)
            return true;
    for (const Away& a : st.away)
        if (a.on && a.entity && a.role == Role::kSecondary)
            return true;
    return false;
}

// That shoulder's holster (the primary's, the secondary's) another gun has now: a carried one (not slot except), or (countAway) an away one in a
// pickup. One gun a shoulder (the review of 2026-11-18: two put on the right, the one held last was drawn every time, the other only by the D-pad).
// The launchers' place is shared (the jet run's two launchers); an away gun with the game (weak) doesn't keep its shoulder. A hand's put-away asks
// without the away guns (StowTo): it wins over one, so no shoulder is blocked for good (review finding 1: an away gun left 60 m behind kept its
// shoulder the whole level). taker: the gun there.
inline bool ShoulderTaken(const SlotIn slots[kSlots], const State& st, Role role, int except, int* takerSlot = nullptr, int* takerAway = nullptr,
                          bool countAway = true) {
    if (takerSlot)
        *takerSlot = -1;
    if (takerAway)
        *takerAway = -1;
    if (role != Role::kPrimary && role != Role::kSecondary)
        return false;
    for (int k = 0; k < kSlots; ++k)
        if (k != except && slots[k].carried && st.home[k].role == role) {
            if (takerSlot)
                *takerSlot = k;
            return true;
        }
    for (int a = 0; a < kMostAway && countAway; ++a)
        if (st.away[a].on && st.away[a].entity && st.away[a].role == role) {
            if (takerAway)
                *takerAway = a;
            return true;
        }
    return false;
}

// Each slot's place now, by its gun's home (kNone: none, or no home: on no shoulder).
inline void PlacesNow(const SlotIn slots[kSlots], const State& st, const Settings& s, Place out[kSlots]) {
    const bool taken = SecondaryTaken(slots, st);
    for (int k = 0; k < kSlots; ++k)
        out[k] = slots[k].carried ? PlaceOfRole(st.home[k].role, taken, s) : Place::kNone;
}

// An away gun's place now (kNone: none, or not on).
inline Place AwayPlace(const SlotIn slots[kSlots], const State& st, const Settings& s, int a) {
    if (a < 0 || a >= kMostAway || !st.away[a].on)
        return Place::kNone;
    return PlaceOfRole(st.away[a].role, SecondaryTaken(slots, st), s);
}

// A holstered gun out of its slot, still on its shoulder: its record from its home (with a pickup: strong; none: weak, its line a moment later, as a
// take's swap may still tell its pickup). When all are in use, the oldest weak one goes, else the oldest. Its index.
inline int MakeAway(State& st, int slot, const Home& h, uint32_t entity, uint32_t blueprint) {
    int at = -1;
    for (int a = 0; a < kMostAway && at < 0; ++a)
        if (!st.away[a].on)
            at = a;
    for (int a = 0; a < kMostAway && at < 0; ++a)
        if (!st.away[a].entity && (at < 0 || st.away[a].since < st.away[at].since))
            at = a;
    if (at < 0) {
        at = 0;
        for (int a = 1; a < kMostAway; ++a)
            if (st.away[a].since < st.away[at].since)
                at = a;
    }
    Away& w = st.away[at];
    w = Away{};
    w.on = true, w.slot = slot, w.kind = h.kind, w.role = h.role, w.entity = entity, w.blueprint = blueprint;
    CopyName(w.name, h.name);
    w.since = st.clock;
    w.heldAt = slot >= 0 && slot < kSlots ? st.heldAt[slot] : -1e9f;
    w.sayAt = entity ? st.clock : st.clock + kTakeNear;
    return at;
}

// A take's gun in slot k: an away gun coming back by the game's take of its pickup (NoteSwap's restore, a moment ago) has its holster again, unless
// another gun has that shoulder now (said); any other has none (picked up off the ground: the memo says so of its kind, if it says nothing yet).
inline Role Restored(const Settings& s, const SlotIn slots[kSlots], State& st, int k, uint32_t kind) {
    State::Restore& q = st.restore[k];
    if (q.kind && q.kind == kind && st.clock - q.at <= kTakeNear) {
        Role r = q.role;
        Event e;
        e.what = What::kComeBack, e.slot = k, e.at = PlaceOfRole(r, SecondaryTaken(slots, st, k), s);
        CopyName(e.gun, q.name);
        int takerSlot = -1, takerAway = -1;
        if (ShoulderTaken(slots, st, r, k, &takerSlot, &takerAway)) {
            e.why = Why::kTaken, e.other = takerSlot;
            if (takerAway >= 0)
                CopyName(e.otherGun, st.away[takerAway].name);
            r = Role::kNone;
        }
        e.place = PlaceOfRole(r, SecondaryTaken(slots, st, k), s);
        Queue(st, e);
        q = State::Restore{};
        st.comebackSlot = k, st.comebackAt = st.clock;
        if (r != Role::kNone)
            Remember(st, kind, r);
        return r;
    }
    RememberFirst(st, kind, Role::kNone);
    return Role::kNone;
}

// A gun the game gave into slot k with no take: an away gun of this slot and kind coming back (the level's kit again after a script's: the M4A1 after
// Operation Swordbreaker's roof AT4) has its holster again (said), unless another gun has that shoulder now; any other has none. A weak record first;
// else one in a pickup (the kit gave a fresh one of its kind back while the one away lies in its pickup, the bridge's M249 left behind: review finding
// 1): that record ends, the gun in the pickup stays there on no shoulder (said), the fresh one has the holster.
inline Role ComeBack(const Settings& s, const SlotIn slots[kSlots], State& st, int k, uint32_t kind) {
    int found = -1;
    for (int pass = 0; pass < 2 && found < 0; ++pass)
        for (int a = 0; a < kMostAway && kind && found < 0; ++a) {
            const Away& w = st.away[a];
            if (w.on && w.slot == k && w.kind == kind && (pass == 0 ? !w.entity : w.entity != 0))
                found = a;
        }
    if (found >= 0) {
        Away& w = st.away[found];
        Role r = w.role;
        Event e;
        e.what = What::kComeBack, e.slot = k, e.weak = true, e.pickupLeft = w.entity != 0;
        CopyName(e.gun, w.name);
        CopyName(e.otherGun, w.instead);
        if (st.wait.on && st.wait.giveBack && st.wait.away == found)
            st.wait = Wait{};  // (a draw of it waiting: the game gave its kind back meanwhile)
        w = Away{};
        e.at = PlaceOfRole(r, SecondaryTaken(slots, st, k), s);
        int takerSlot = -1, takerAway = -1;
        if (ShoulderTaken(slots, st, r, k, &takerSlot, &takerAway)) {
            e.why = Why::kTaken, e.other = takerSlot;
            if (takerAway >= 0)
                CopyName(e.otherGun, st.away[takerAway].name);
            r = Role::kNone;
        }
        e.place = PlaceOfRole(r, SecondaryTaken(slots, st, k), s);
        Queue(st, e);
        st.comebackSlot = k, st.comebackAt = st.clock;
        if (r != Role::kNone)
            Remember(st, kind, r);
        return r;
    }
    RememberFirst(st, kind, Role::kNone);
    return Role::kNone;
}

// A new life's kit (from NewLife till the kit's seconds are over, before each frame's noting): the same level's (a death and its checkpoint give back
// what the player had) only while every gun of it with a holster by its slot (KitRole: the rifles of slots 0 and 1, the launchers, slot 5's) was
// carried in the life before; a gun taken off the ground isn't the kit's. Else another level's (review finding 3: a gun picked up in Rock and put on a
// shoulder made Swordbreaker's M4A1 the secondary and its Mk11 none, for good): the memo goes, and each kit gun noted so far has its slot's holster.
inline void KitCheck(const Settings& s, const SlotIn slots[kSlots], unsigned taken, State& st) {
    if (!st.kitWatch)
        return;
    if (st.clock > kKitSeconds) {
        st.kitWatch = false;
        return;
    }
    bool any = false, other = false;
    for (int k = 0; k < kSlots; ++k) {
        const SlotIn& in = slots[k];
        if (!in.carried || !in.kind || KitRole(slots, s, k) == Role::kNone)
            continue;
        const Home& h = st.home[k];
        const uint32_t id = in.id ? in.id : kUnknownId;
        const bool byTake = (id == h.id && h.byTake) || (id != h.id && (((taken >> k) & 1u) || st.clock - h.takenAt <= kTakeNear));
        if (byTake)
            continue;
        any = true;
        other = other || !CarriedBefore(st, in.kind);
    }
    if (!any)
        return;
    st.memoChecked = true;
    if (!other)
        return;
    ForgetMemo(st);
    st.memoForgot = true, st.kitWatch = false;
    // The kit's guns noted so far, each by its slot (one gun a shoulder, the lowest slot first).
    bool kit[kSlots] = {};
    for (int k = 0; k < kSlots; ++k) {
        Home& h = st.home[k];
        kit[k] = slots[k].carried && h.id != 0 && !h.byTake && h.seenAt > -1e8f && h.seenAt <= kKitSeconds;
        if (kit[k])
            h.role = Role::kNone;
    }
    for (int k = 0; k < kSlots; ++k) {
        if (!kit[k])
            continue;
        Home& h = st.home[k];
        const Role want = KitRole(slots, s, k);
        const Role r = ShoulderTaken(slots, st, want, k) ? Role::kNone : want;
        h.role = r;
        h.memoByKit = r == want;
        if (h.memoByKit)
            Remember(st, h.kind, r);
    }
}

// The guns in the slots, each frame before anything is decided on them (Update; Arrive, which comes first): a gun is told by its identity (SlotIn::id,
// kind). A new one in a slot (another identity) has no home, unless it is the kit of a new life (first seen within kKitSeconds of the life's start:
// the holster the memo has for its kind, else its slot's; the user, 2026-10-31: whatever the player starts with has its holster from the start), or an
// away gun of that slot coming back (Restored, ComeBack). A take off the ground (Frame::taken: a hand's, the magnet's, the game's own R) never gives
// one, nor a gun the game gives with no take later on (the roof's AT4 by its kit): its mark and the new gun may come a frame apart, either first
// (kTakeNear). The gun before, with its holster, stays on its shoulder, away: for a take when it was hanging there (NoteSwap tells the pickup it went
// into), for a gift held or hanging (review finding 5: the roof's AT4 by its kit with the M4A1 in the hand). A slot not read a moment, or the same gun
// under a new object (the same kind, no take), keeps what it had; an emptied slot keeps its record (a gun that went and came back is a new object, and
// a new gun). Every kind carried is noted for the next life's KitCheck.
inline void NoteSlots(const Settings& s, const SlotIn slots[kSlots], unsigned taken, bool swapsSeen, State& st) {
    (void)swapsSeen;  // (a gift with the takes not seen is none, as one with them seen: a gun goes on a shoulder only by a hand)
    KitCheck(s, slots, taken, st);
    for (int k = 0; k < kSlots; ++k) {
        Home& h = st.home[k];
        const SlotIn& in = slots[k];
        const uint32_t id = in.id ? in.id : kUnknownId;
        if (in.carried)
            NoteCarried(st, in.kind);
        if ((taken >> k) & 1u) {
            // (The take's gun seen a frame before its swap was told, first noted as a gift: a take's after all. Once: Arrive and Update both hear
            // this frame's marks. A kit's word the noting wrote for its kind is none now: review finding 8.)
            const bool late = h.seenAt > -1e8f && st.clock - h.seenAt <= kTakeNear && !h.byTake && in.carried && id == h.id;
            h.takenAt = st.clock;
            if (late) {
                h.role = Restored(s, slots, st, k, h.kind);
                h.byTake = true;
                if (h.role == Role::kNone && h.memoByKit)
                    Remember(st, h.kind, Role::kNone);
                h.memoByKit = false;
            }
        }
        if (!in.carried)
            continue;
        if (id == h.id)
            continue;
        const bool sameGun = h.id != 0 && in.kind != 0 && in.kind == h.kind && id != kUnknownId && h.id != kUnknownId;
        const bool byTake = st.clock - h.takenAt <= kTakeNear;
        if (sameGun && !byTake) {
            h.id = id, h.seenAt = st.clock;  // (the same gun read afresh: as it was)
            CopyName(h.name, in.name);
            continue;
        }
        // The gun before, with its holster: on its shoulder still, away (a take's: hanging there; a gift's: held or hanging).
        h.lastAway = -1;
        if (s.on && h.id != 0 && h.role != Role::kNone && (WasHung(st, k) || !byTake))
            h.lastAway = MakeAway(st, k, h, 0, 0);
        h.lastKind = h.kind, h.lastAt = st.clock;
        h.id = id, h.kind = in.kind, h.seenAt = st.clock, h.byTake = byTake;
        h.memoByKit = false;
        CopyName(h.name, in.name);
        if (byTake) {
            h.role = Restored(s, slots, st, k, in.kind);  // (picked up from the ground: on no shoulder till a hand puts it on one)
        } else if (st.clock <= kKitSeconds) {
            const Memo* m = MemoOf(st, in.kind);
            const Role want = m ? m->role : KitRole(slots, s, k);
            Role r = want;
            if (ShoulderTaken(slots, st, r, k))
                r = Role::kNone;  // (one gun a shoulder: an older word of the memo's for another kind)
            h.role = r;
            if (!m && r == want) {
                Remember(st, in.kind, r);  // (a none from one gun a shoulder alone isn't its kind's word: nothing written, review finding 3)
                h.memoByKit = true;
            }
        } else {
            h.role = ComeBack(s, slots, st, k, in.kind);  // (the game gave it with no take: none, unless an away gun of this slot is back)
        }
    }
}

// One swap of the soldier's slots by a take off the ground, as pickups saw it (hands.cpp's NoteSwaps, every swap, before Arrive and Update this frame):
// each slot it changed with the kinds that came and went (NameKey; 0 nothing), the pickup (the gun that went is in it now), whether that pickup went
// with the take, and the taken gun's own name.
struct SwapIn {
    int changed = 0;
    int slot[kSlots] = {};
    uint32_t came[kSlots] = {}, went[kSlots] = {};
    uint32_t wentBlueprint[kSlots] = {};
    bool same[kSlots] = {};  // that slot's weapon is the one it had (its firing state and blueprint as before): a take of its rounds alone
    uint32_t entity = 0;
    bool removed = false;
    bool noPlace = false;    // the pickup keeps no place for the guns it replaces (UnspawnOnPickup, ContentIsStatic): they're gone
    bool all = false;        // ReplaceAllContent: it holds a copy of every gun now, and its take would give them all back as they were
    char taken[kNameLen] = {};
};
// What went: a holstered gun stays on its shoulder, away in that pickup (its gun not noted gone yet: from its home; noted a frame before: the weak
// record NoteSlots made gets its pickup); if the pickup went with the take or keeps no place for it, it's gone (said: the level's own pickup, taken
// all the same); one that took every gun can't give it back alone (said, on no shoulder). What came: an away gun from this very pickup is back, its
// holster again once its gun is noted in its slot (Restored: the take's mark comes with this swap). A gun of the same kind for a holstered one is a
// swap like any other (review finding 7: the game's same-gun check refuses only one with the same attachments too); only the same weapon kept (a take
// of its rounds alone) changes nothing. Whatever holsters.txt says but off.
inline void NoteSwap(const Settings& s, const SlotIn slots[kSlots], const SwapIn& in, State& st) {
    (void)slots;
    if (!s.on)
        return;
    const int n = in.changed < kSlots ? (in.changed > 0 ? in.changed : 0) : kSlots;
    for (int i = 0; i < n; ++i) {
        const int k = in.slot[i];
        if (k < 0 || k >= kSlots || in.same[i])
            continue;  // (the same weapon kept: a take of its rounds alone, the gun as it was)
        if (in.came[i] && in.entity) {
            int found = -1;
            for (int a = 0; a < kMostAway; ++a) {
                const Away& w = st.away[a];
                if (w.on && w.entity == in.entity && w.kind == in.came[i] && (found < 0 || w.slot == k))
                    found = a;
            }
            if (found >= 0) {
                Away& w = st.away[found];
                State::Restore& q = st.restore[k];
                q.kind = w.kind, q.role = w.role, q.at = st.clock;
                CopyName(q.name, w.name);
                if (st.wait.on && st.wait.giveBack && st.wait.away == found)
                    st.wait.away = -1;  // (its record goes: the draw's wait ends at the gun's arrival)
                w = Away{};
            }
        }
        if (!in.went[i])
            continue;
        Home& h = st.home[k];
        const bool gone = in.removed || !in.entity || in.noPlace;
        const auto lost = [&](const char* name) {
            Event e;
            e.what = What::kLost, e.slot = k, e.why = gone ? Why::kGone : Why::kAll;
            CopyName(e.gun, name);
            CopyName(e.otherGun, in.taken);
            Queue(st, e);
        };
        if (h.kind == in.went[i] && h.role != Role::kNone && WasHung(st, k)) {
            if (gone || in.all) {
                lost(h.name);
            } else {
                const int a = MakeAway(st, k, h, in.entity, in.wentBlueprint[i]);
                CopyName(st.away[a].instead, in.taken);
            }
            h.role = Role::kNone;  // (the slot's gun is another now: NoteSlots makes no record of its own for this one)
        } else if (h.lastKind == in.went[i] && st.clock - h.lastAt <= kTakeNear && h.lastAway >= 0 && h.lastAway < kMostAway) {
            Away& w = st.away[h.lastAway];
            if (w.on && !w.entity && w.kind == in.went[i] && w.slot == k) {
                if (gone || in.all) {
                    lost(w.name);
                    w = Away{};
                } else {
                    w.entity = in.entity, w.blueprint = in.wentBlueprint[i];
                    CopyName(w.instead, in.taken);
                    if (!w.said)
                        w.sayAt = st.clock;
                }
            }
            h.lastAway = -1;
        }
    }
}

// Where the game's weapon (slot) goes when a hand puts it away at place at (kNone: nowhere, so not a put-away there), and the role it takes. A gun
// with a role by its kind and slot (KitRole: a rifle of slot 0 or 1, a launcher, slot 5's gun) may go; it goes:
//   any but a launcher: on the shoulder the hand put it on, which is then its holster, the right the primary's and the left the secondary's (the
//     user's layout of 2026-11-05) whatever the game's slot (2026-11-18: "it needs to be physically placed on the shoulder to lock as a permanent gun
//     primary or secondary"); let go behind the neck or at the rib cage, on its own shoulder by its slot;
//   a launcher (and slot 5's gun): at the launcher's place, wherever at the holsters it's let go (2026-11-15: "the SMAW should go on the left
//     shoulder"; behind the neck while a secondary hangs at the left shoulder).
// Never on a shoulder another carried gun has (why kTaken, taker the gun there: one gun a shoulder); an away gun's shoulder it takes (Settle: that
// gun stays in its pickup on no shoulder, said; review finding 1). "anyholster 0": only where it stays (the shoulder it's put on, the launcher's place
// itself). Several launchers may share their place (the jet run's two): a draw there gives the one in the hands last.
inline Place StowTo(const Settings& s, const SlotIn slots[kSlots], const State& st, int slot, Place at, Role* role = nullptr, Why* why = nullptr,
                    int* takerSlot = nullptr, int* takerAway = nullptr) {
    if (role)
        *role = Role::kNone;
    if (why)
        *why = Why::kNothing;
    if (takerSlot)
        *takerSlot = -1;
    if (takerAway)
        *takerAway = -1;
    if (slot < 0 || slot >= kSlots || at == Place::kNone || !Holds(at, s))
        return Place::kNone;
    const Role kit = KitRole(slots, s, slot);
    if (kit == Role::kNone)
        return Place::kNone;
    Role r = kit;
    if (kit != Role::kLauncher)
        r = at == PrimaryPlace(s) ? Role::kPrimary : at == SecondaryPlace(s) ? Role::kSecondary : kit;
    const Place to = PlaceOfRole(r, SecondaryTaken(slots, st, slot), s);
    if (to == Place::kNone || (!s.anyHolster && to != at))
        return Place::kNone;
    if (ShoulderTaken(slots, st, r, slot, takerSlot, takerAway, false)) {
        if (why)
            *why = Why::kTaken;
        return Place::kNone;
    }
    if (role)
        *role = r;
    if (why)
        *why = Why::kNone;
    return to;
}

// The place the gun of a slot hangs at now (kNone: not at a holster): carried, with a home, and not the game's weapon in the hands or on the ground.
// The game's weapon not known now (a moment unread past kSlotStale): every gun with a home counts.
inline Place HolsteredAt(const Settings& s, const SlotIn slots[kSlots], const State& st, int slot, Holder holder) {
    if (!s.on || slot < 0 || slot >= kSlots || !slots[slot].carried)
        return Place::kNone;
    Place places[kSlots];
    PlacesNow(slots, st, s, places);
    if (places[slot] == Place::kNone)
        return Place::kNone;
    const bool known = st.gameSlot >= 0 && st.gameSlot < kSlots && st.slotUnread <= kSlotStale;
    return !known || slot != st.gameSlot || holder == Holder::kStowed ? places[slot] : Place::kNone;
}

// A take that would put the guns of these slots into its pickup (pickrules::TakeVictims' bits) takes a holstered one off its slot: the first such
// slot and its place; -1 none.
inline int HolsteredVictim(const Settings& s, const SlotIn slots[kSlots], const State& st, Holder holder, unsigned victims, Place* at = nullptr) {
    for (int k = 0; k < kSlots; ++k) {
        if (!((victims >> k) & 1u))
            continue;
        const Place p = HolsteredAt(s, slots, st, k, holder);
        if (p != Place::kNone) {
            if (at)
                *at = p;
            return k;
        }
    }
    return -1;
}

// A take of a pickup, before the game is asked (hands.cpp: a hand's, the magnet's; X held's R): it goes ahead (kGo: no holstered gun among the guns it
// takes out; kAway: the holstered ones stay on their shoulders, in that pickup: NoteSwap), or is refused (kRefuse, why, the gun's slot and place) when
// a holstered one couldn't be given back: the pickup destroys the guns of the slots it names (gone: UnspawnOnPickup, ContentIsStatic), would hold every
// gun carried and give them all back as they were (all: ReplaceAllContent), has more entries than are read (unread: victims every gun carried), or
// names the grenade launcher's slot (2) and not its rifle's (0) with that rifle at its shoulder (a part of it would go). victims: the slots whose guns
// leave (pickrules::TakeVictims). Refused only for a plain floor gun (a soldier's drop): the level's own pickups (required: the Stinger Rock's jet
// needs, a script's gun) always go ahead, as the game takes them (review finding 6: a picked-up gun on the left shoulder made Rock's ending
// impossible); a holstered gun one destroys or can't give back is then gone (lost, why and its slot and place: hands.cpp says so before, NoteSwap as
// the swap comes).
enum class TakeVerdict : uint8_t { kGo, kAway, kRefuse };
struct TakeOut {
    TakeVerdict verdict = TakeVerdict::kGo;
    int slot = -1;
    Place at = Place::kNone;
    Why why = Why::kNone;
    bool lost = false;  // kAway for a required pickup: the holstered gun of slot (at at) can't stay on its shoulder (why: kGone, kAll, kUnread)
};
inline TakeOut TakeCheck(const Settings& s, const SlotIn slots[kSlots], const State& st, Holder holder, unsigned victims, bool gone, bool all,
                         bool unread, bool required = false) {
    TakeOut o;
    if (!s.on)
        return o;
    if (((victims >> 2) & 1u) && !(victims & 1u)) {
        const Place p = HolsteredAt(s, slots, st, 0, holder);
        if (p != Place::kNone && !required) {
            o.verdict = TakeVerdict::kRefuse, o.slot = 0, o.at = p, o.why = Why::kPart;
            return o;
        }
    }
    for (int k = 0; k < kSlots; ++k) {
        if (!((victims >> k) & 1u))
            continue;
        const Place p = HolsteredAt(s, slots, st, k, holder);
        if (p == Place::kNone)
            continue;
        if (gone || all || unread) {
            const Why why = gone ? Why::kGone : all ? Why::kAll : Why::kUnread;
            if (!required) {
                o.verdict = TakeVerdict::kRefuse, o.slot = k, o.at = p, o.why = why, o.lost = false;
                return o;
            }
            if (!o.lost)
                o.verdict = TakeVerdict::kAway, o.slot = k, o.at = p, o.why = why, o.lost = true;
            continue;
        }
        if (o.verdict == TakeVerdict::kGo)
            o.verdict = TakeVerdict::kAway, o.slot = k, o.at = p;
    }
    return o;
}

// A take refused for a holster (hands.cpp, before the game is asked): the double click's first pulse for that hand, the second 60 ms on (Settle).
inline Pulse RefusePulse(const Settings& s, State& st, int side) {
    Pulse p;
    if (side < 0 || side > 1 || !(s.buzz > 0.0f))
        return p;
    p.amplitude = kDoubleAmp * s.buzz < 1.0f ? kDoubleAmp * s.buzz : 1.0f;
    p.seconds = kDoubleSeconds;
    st.pulseAt[side] = st.clock + kDoubleGap;
    return p;
}

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
    st.drawNoPlace = false;  // (Update's 0b sets it after, for its own)
}
// A put-away by that hand asked of the grasp now (planned where it goes, at where the hand is, role the role it takes), again each frame till it's done.
inline void AskStow(Result& r, State& st, int side, Place planned, Place at, Role role) {
    r.stowSide = side;
    st.planned[side] = planned, st.plannedAt[side] = at, st.plannedRole[side] = role;
    st.stowAsk = Ask{};
    st.stowAsk.side = side, st.stowAsk.planned = planned, st.stowAsk.at = at, st.stowAsk.role = role;
}

// Why a put-away by hand i that would go to place to (StowTo's) isn't one now (kNone: it is): kNothing not a place it goes away at (today's
// let-go, said nowhere), then the moment, the other hand's hold on it, what the other hand holds.
inline Why StowWhy(const Settings& s, const Frame& f, int i, Place to) {
    if (to == Place::kNone)
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

// The gun a draw at place p gives: a carried one at its holster there (GunAt), or an away one in a pickup whose holster is there; of several, the one
// in the hands last; an away one with the game (weak) only when nothing else is there. slot: a carried gun's; away: an away record's.
struct AtPlace {
    int slot = -1, away = -1;
};
inline AtPlace FindAt(const Settings& s, Place p, const SlotIn slots[kSlots], const Place places[kSlots], const State& st, Holder holder) {
    AtPlace out;
    if (p == Place::kNone)
        return out;
    const int c = GunAt(p, slots, places, st.gameSlot, holder, st.heldAt);
    int best = -1;
    for (int a = 0; a < kMostAway; ++a) {
        const Away& w = st.away[a];
        if (!w.on || !w.entity || AwayPlace(slots, st, s, a) != p)
            continue;
        if (best < 0 || w.heldAt > st.away[best].heldAt)
            best = a;
    }
    if (c >= 0 && (best < 0 || st.heldAt[c] >= st.away[best].heldAt)) {
        out.slot = c;
        return out;
    }
    if (best >= 0) {
        out.away = best;
        return out;
    }
    for (int a = 0; a < kMostAway; ++a)
        if (st.away[a].on && !st.away[a].entity && AwayPlace(slots, st, s, a) == p) {
            out.away = a;
            break;
        }
    return out;
}

// The game's weapon on the ground goes into its slot for a draw of another (the game draws one gun at a time), or into a pickup by its slot's give-back:
// one with no holster of its own by the memo (picked up off the ground, given by the game: Discardable). The player's own stays where it lies.
inline bool LyingGoes(const State& st) {
    return st.lyingSlot >= 0 && st.lyingSlot < kSlots && Discardable(st, st.home[st.lyingSlot].kind);
}

// Why a draw at place at by hand i would be refused now (kNone: it would draw): slot the gun it would give, caseA whether that's the game's weapon
// put away (no key), stowOther never (it was "othergun stow"'s: the other hand's gun to its place first, no more since 2026-11-18); away: an away
// gun's record (its draw the game's take of its pickup: kNone only within the game's reach of it, Frame::awayReach), -1 a carried gun's.
inline Why DrawWhy(const Settings& s, const Frame& f, const State& st, int i, Place at, int& slot, bool& caseA, bool& stowOther, int* away = nullptr) {
    using namespace detail;
    slot = -1, caseA = false, stowOther = false;
    if (away)
        *away = -1;
    if (!SlotKnown(st))
        return Why::kSlotUnread;
    if (!Empty(f, i))
        return Why::kBusy;
    if (f.frozen || f.gameReload || f.m320 || f.pendingTake || st.wait.on || st.recallSide >= 0)
        return Why::kMoment;
    Place places[kSlots];
    PlacesNow(f.slots, st, s, places);
    // The other hand holds the game's weapon (this one is empty): one gun in the hands, put away first (the user, 2026-11-16); never put away by
    // itself for a draw (2026-11-18: a gun goes into a holster only by a hand), whatever "othergun" says.
    if (GunInHands(f.holder))
        return Why::kOtherHolds;
    // On the ground: the player's own gun stays there (take it first); a picked-up one let go of goes (the user's AT4: "shoot it, discard it, then
    // pick up their rifle again"), and so does any lying gun a give-back of its own slot puts into the pickup (it stays on the ground there: review
    // finding 4, the AT4 put on the launcher's place a moment before it was fired and thrown).
    const AtPlace g = FindAt(s, at, f.slots, places, st, f.holder);
    const bool lyingInto = g.away >= 0 && st.away[g.away].entity && st.away[g.away].slot == st.lyingSlot;
    if (f.holder == Holder::kLoose && !LyingGoes(st) && !lyingInto)
        return Why::kLying;
    if (g.away >= 0) {
        const Away& w = st.away[g.away];
        slot = w.slot;
        if (away)
            *away = g.away;
        if (!w.entity)
            return Why::kWithGame;
        if (!f.awayReach[g.away])
            return Why::kFar;
        return Why::kNone;
    }
    slot = g.slot;
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
    int slot = -1, away = -1;
    bool caseA = false, stowOther = false;
    Why why = DrawWhy(s, f, st, i, at, slot, caseA, stowOther, &away);
    if (why == Why::kNone && (r.drawSide >= 0 || r.key >= 0 || r.stowSide >= 0 || r.giveBack.side >= 0))
        why = Why::kMoment;  // (the other hand's draw or put-away this very frame)
    if (why == Why::kNone && away >= 0 && hidden)
        why = Why::kHidden;  // (an away gun comes by the game's take into a seen hand only)
    const Record& rc = st.rec[i];
    Event e;
    e.side = i, e.place = at, e.slot = slot, e.why = why, e.unseen = hidden;
    if (hidden)
        e.unseenFor = rc.unseenFor, e.lastOff = rc.off, e.lastSpeed = rc.lastSpeed;
    if (away >= 0) {
        const Away& w = st.away[away];
        e.giveBack = true, e.weak = !w.entity, e.distance = f.awayDistance[away];
        CopyName(e.gun, w.name);
        CopyName(e.otherGun, w.instead);
    }
    if (why == Why::kNone && away >= 0) {
        // An away gun: the game's own take of the pickup it's in, into this hand (hands.cpp asks it: pickups::RequestGiveBack). No key; its arrival
        // is the take's, into this hand; the slot's gun then goes into that pickup.
        const Away& w = st.away[away];
        st.wait = Wait{};
        st.wait.on = true, st.wait.side = i, st.wait.slot = w.slot, st.wait.place = at, st.wait.giveBack = true, st.wait.away = away;
        r.giveBack.side = i, r.giveBack.slot = w.slot, r.giveBack.away = away, r.giveBack.entity = w.entity, r.giveBack.blueprint = w.blueprint;
        e.what = What::kAsked;
        Add(r, e);
        Buzz(r, s, i, kClickAmp, kClickSeconds);
        return;
    }
    if (why == Why::kNone) {
        (void)stowOther;  // (never: no gun goes into a holster but by a hand)
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
        st.refusal[i].recall = false;  // ("lying recall" no more: the gun that lies goes back on a shoulder only by a hand, 2026-11-18)
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
    for (int i = 0; i < 2; ++i) {
        st.planned[i] = st.plannedAt[i] = Place::kNone, st.stowWhy[i] = Why::kNone, st.stowWhyAt[i] = Place::kNone, st.plannedRole[i] = Role::kNone;
        st.stowTakerSlot[i] = st.stowTakerAway[i] = -1;
    }
    // The guns in the slots and their homes (whatever holsters.txt says and free hands awake or not: a take or a gift is noted when it comes), and
    // the game's weapon and the grasp's holder the frame before, for the switch's lines below.
    NoteSlots(s, f.slots, f.taken, f.swapsSeen, st);
    const int slotWas = st.lastSlot;
    const Holder holderWas = st.lastHolder;
    st.lastSlot = SlotKnown(st) ? st.gameSlot : -1;
    st.lastHolder = f.holder;
    // A gun on the ground has no holster any more ("physically removed from the holster and dropped"): picked up again, it goes on a shoulder only
    // by a hand. (The gun that fell: the game's weapon as it fell; the game's switch to another may come a few frames before the grasp hears of it.)
    if (f.on && f.holder == Holder::kLoose) {
        if (holderWas != Holder::kLoose || st.lyingSlot < 0)
            st.lyingSlot = SlotKnown(st) ? st.gameSlot : -1;
        if (st.lyingSlot >= 0)
            st.home[st.lyingSlot].role = Role::kNone;
    } else if (f.holder != Holder::kLoose) {
        st.lyingSlot = -1;
    }
    // The lines noted outside this (Arrive's NoteSlots, NoteSwap, GiveBackFailed), a refusal's double click with them (felt with the other hand
    // empty, as every refusal); then each away gun: said once (a weak one a moment on), and given up once its pickup has been gone, or held another
    // gun, kLostAfter (the pickups seen: Frame::awayFound). Free hands asleep too (a take the game's own R made with the gun drawn the game's way).
    for (int p = 0; p < st.pendingCount; ++p) {
        Event e = st.pending[p];
        if (e.what == What::kRefused && (e.side == 0 || e.side == 1) && st.doubleDue[e.side]) {
            st.doubleDue[e.side] = false;
            e.quiet = !Empty(f, 1 - e.side);
            if (!e.quiet)
                Double(r, st, s, e.side);
        }
        Add(r, e);
    }
    st.pendingCount = 0;
    st.doubleDue[0] = st.doubleDue[1] = false;
    for (int a = 0; a < kMostAway; ++a) {
        Away& w = st.away[a];
        if (!w.on)
            continue;
        if (!w.said && st.clock >= w.sayAt) {
            w.said = true;
            Event e;
            e.what = What::kAway, e.slot = w.slot, e.weak = !w.entity, e.place = AwayPlace(f.slots, st, s, a);
            CopyName(e.gun, w.name);
            CopyName(e.otherGun, w.instead);
            Add(r, e);
        }
        if (!w.entity)
            continue;
        const int found = f.awayFound[a];
        if (found == 1 || found == -2) {
            w.missing = 0.0f;
            continue;
        }
        w.missing += dt;
        if (w.missing > kLostAfter && st.clock - w.since > kLostAfter) {
            Event e;
            e.what = What::kLost, e.slot = w.slot, e.why = found == 0 ? Why::kNothing : Why::kFar, e.place = AwayPlace(f.slots, st, s, a);
            CopyName(e.gun, w.name);
            CopyName(e.otherGun, w.instead);
            Add(r, e);
            if (st.wait.on && st.wait.giveBack && st.wait.away == a)
                st.wait = Wait{};
            w = Away{};
        }
    }
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
    PlacesNow(f.slots, st, s, places);
    const bool slotKnown = SlotKnown(st);
    const bool body = f.body && Sane3(f.neck) && std::isfinite(f.yaw);
    const bool gates = body && slotKnown;

    // 0. The game's weapon moved on from another gun (its own switch: a pick, a take into another slot, a script): that gun is at its home, the
    // gun in the hands taken back there (said); with no home it's in the game's slot on no shoulder (said: the D-pad or Y brings it). Only between
    // two guns a holster can hold (the grenade launcher under the rifle, a grenade or the knife never take the rifle out of the hands).
    if (slotWas >= 0 && slotKnown && st.gameSlot != slotWas && f.slots[slotWas].carried && KitRole(f.slots, s, slotWas) != Role::kNone &&
        KitRole(f.slots, s, st.gameSlot) != Role::kNone) {
        Event e;
        e.slot = slotWas;
        if (places[slotWas] == Place::kNone) {
            e.what = What::kNoPlace;
            Add(r, e);
        } else if (GunInHands(holderWas)) {
            e.what = What::kBackHome, e.place = places[slotWas];
            Add(r, e);
        }
    }
    // 0b. The game's weapon put away with no home (the game made another gun of it, or a state nobody meant): it can't be kept at a place it doesn't
    // have, so it goes into the right hand, unarmed, as at a spawn, once that hand is empty (said once; asked again a second on if it didn't land).
    if (slotKnown && f.holder == Holder::kStowed && places[st.gameSlot] == Place::kNone) {
        if (!f.frozen && !st.wait.on && st.drawAsk.side < 0 && Empty(f, 1) && st.clock - st.toHandAt >= 1.0f) {
            if (!st.toHandSaid) {
                Event e;
                e.what = What::kToHand, e.side = 1, e.slot = st.gameSlot;
                Add(r, e);
                st.toHandSaid = true;
            }
            AskDraw(r, st, 1, false, false, 0.0f);
            st.drawNoPlace = true;
            st.toHandAt = st.clock;
        }
    } else {
        st.toHandSaid = false;
    }

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
            if (h.squeeze >= kTake)
                st.keptSaid[i] = false;  // (squeezed again: its next opening at a refusing place is said again)
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
            e.quiet = st.drawNoPlace;  // (0b asks again a second on: said once, as kToHand)
            Add(r, e);
            q = Ask{};
            st.drawNoPlace = false;
        } else if (f.holder != Holder::kStowed || (q.armed && opened) || !Empty(f, i)) {
            q = Ask{};  // (done, or no longer wanted: let go first, or something else in that hand)
            if (f.holder == Holder::kStowed)
                st.drawNoPlace = false;  // (drawn, its line is Settle's: the flag kept for it)
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
            r.stowSide = i, st.planned[i] = q.planned, st.plannedAt[i] = q.at, st.plannedRole[i] = q.role;
        }
    }

    // 2. Put-aways: a seen hand's let-go at a place (the grasp decides the let-go, stows where it goes), a hand not seen opening at its presumed
    // place (stowSide), and the hold that keeps a hand not seen from dropping its gun. Where it goes and the holster it then has: StowTo (a gun goes
    // on the shoulder the hand puts it on; a launcher at the launcher's place). A put-away refused at a place keeps the gun in the hand.
    for (int i = 0; i < 2; ++i) {
        const HandIn& h = f.hand[i];
        const Record& rc = st.rec[i];
        if (!HasGun(f, i)) {
            st.keepPlace[i] = Place::kNone;
            continue;
        }
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
            const Place at = PlaceAt(from[i], s, nullptr, s.stowMore);
            Role role = Role::kNone;
            Why toWhy = Why::kNothing;
            int takerSlot = -1, takerAway = -1;
            const Place to = StowTo(s, f.slots, st, st.gameSlot, at, &role, &toWhy, &takerSlot, &takerAway);
            const Why why = StowWhy(s, f, i, to);
            const bool slow = !speedKnown[i] || speed[i] < s.stowSpeed;
            Role graceRole = Role::kNone;
            const Place graceTo = st.stowOkPlace[i] != Place::kNone ? StowTo(s, f.slots, st, st.gameSlot, st.stowOkPlace[i], &graceRole) : Place::kNone;
            // A put-away refused here (another carried gun has that shoulder, what the other hand holds, a moment) keeps the gun in the hand: why, where,
            // the gun there.
            Why keep = Why::kNone;
            Place keepAt = Place::kNone, keepTo = Place::kNone;
            int keepTaker = -1;
            const bool frontHeld = f.holder == Holder::kHeld && f.hand[1 - i].onGun;  // (the other hand on its front: a let-go is that hold's, as ever)
            if (why == Why::kNone && slow) {
                r.stows[i] = true;
                st.planned[i] = to, st.plannedAt[i] = at, st.plannedRole[i] = role;
                st.stowOkAt[i] = st.clock, st.stowOkPlace[i] = at;
            } else if (frontHeld) {
                // (Nothing refused: the gun goes onto the other hand's hold.)
            } else if (slow && (toWhy == Why::kTaken || why == Why::kOtherRocket || why == Why::kOtherRound || why == Why::kOtherMag ||
                                why == Why::kMoment)) {
                keep = toWhy == Why::kTaken ? Why::kTaken : why, keepAt = at, keepTo = to, keepTaker = takerSlot;
                st.keepAt[i] = st.clock, st.keepPlace[i] = at, st.keepWhy[i] = keep, st.keepTakerSlot[i] = takerSlot;
                st.stowWhy[i] = keep, st.stowWhyAt[i] = at, st.stowTakerSlot[i] = takerSlot, st.stowTakerAway[i] = takerAway;  // (a let-go anyway: said)
            } else if (toWhy == Why::kTaken) {
                // Another gun has that shoulder (a throw there): no put-away (one gun a shoulder); a let-go here is as anywhere, said with the double
                // click.
                st.stowWhy[i] = Why::kTaken, st.stowWhyAt[i] = at, st.stowTakerSlot[i] = takerSlot, st.stowTakerAway[i] = takerAway;
            } else if (why == Why::kOtherRocket || why == Why::kOtherRound || why == Why::kOtherMag) {
                st.stowWhy[i] = why, st.stowWhyAt[i] = at;  // (said if it lets go here)
            } else if (why == Why::kNothing && slow && st.stowOkPlace[i] != Place::kNone && st.clock - st.stowOkAt[i] <= s.stowGrace &&
                       Size(EggOf(st.stowOkPlace[i], s), from[i], s.stowMore) <= kStowGraceSize && StowWhy(s, f, i, graceTo) == Why::kNone) {
                // Just out of the place a moment after it could put away there, still near it: let go now, it's put away there too.
                r.stows[i] = true;
                st.planned[i] = graceTo, st.plannedAt[i] = st.stowOkPlace[i], st.plannedRole[i] = graceRole;
            } else if (why == Why::kNothing && slow && st.keepPlace[i] != Place::kNone && st.clock - st.keepAt[i] <= s.stowGrace &&
                       Size(EggOf(st.keepPlace[i], s), from[i], s.stowMore) <= kStowGraceSize) {
                // Just out of a refusing place a moment after (the put-away's grace): kept in the hand all the same.
                keep = st.keepWhy[i], keepAt = st.keepPlace[i], keepTaker = st.keepTakerSlot[i];
            }
            // Kept: its let-go here is held (the grasp told the hand isn't tracked: its grip disarmed, so the gun stays in the open hand till it's squeezed
            // again), said once an opening, with the double click (a moment's quiet). Never dropped at a place that won't take it (review finding 1; the
            // user, 2026-11-16: "it just keeps falling to the ground").
            if (keep != Why::kNone && h.read && h.squeeze < kLetGo) {
                r.hold[i] = true;
                if (!st.keptSaid[i]) {
                    st.keptSaid[i] = true;
                    Event e;
                    e.what = What::kStowRefused, e.side = i, e.slot = st.gameSlot, e.why = keep, e.at = keepAt, e.place = keepTo, e.kept = true;
                    e.other = keepTaker;
                    e.quiet = keep == Why::kMoment;
                    Add(r, e);
                    if (!e.quiet)
                        Double(r, st, s, i);
                }
            }
        } else if (!seen[i] && h.own && s.unseen && rc.presumed != Place::kNone && !rc.vetoed && rc.unseenFor <= s.unseenStow && rc.gripArmed &&
                   openedNow[i] && r.stowSide < 0) {
            Role role = Role::kNone;
            Why toWhy = Why::kNothing;
            int takerSlot = -1, takerAway = -1;
            const Place to = StowTo(s, f.slots, st, st.gameSlot, rc.presumed, &role, &toWhy, &takerSlot, &takerAway);
            const Why why = toWhy == Why::kTaken ? Why::kTaken : StowWhy(s, f, i, to);
            if (why == Why::kNone) {
                AskStow(r, st, i, to, rc.presumed, role);
            } else if (why == Why::kTaken) {
                // Another gun has that shoulder: refused, the gun stays in the hand (it isn't seen: nothing is dropped), felt.
                Event e;
                e.what = What::kStowRefused, e.side = i, e.slot = st.gameSlot, e.why = why, e.at = rc.presumed, e.kept = true, e.other = takerSlot;
                if (takerAway >= 0)
                    CopyName(e.otherGun, st.away[takerAway].name);
                e.unseen = true, e.unseenFor = rc.unseenFor, e.lastOff = rc.off, e.lastSpeed = rc.lastSpeed;
                Add(r, e);
                Double(r, st, s, i);
            } else if (why != Why::kNothing) {
                // Refused: the gun stays in the hand (it isn't seen: nothing is dropped). Felt for what the other hand holds; the hold on the
                // handguard and a moment silent.
                Event e;
                e.what = What::kStowRefused, e.side = i, e.slot = st.gameSlot, e.why = why, e.at = rc.presumed, e.place = to, e.kept = true;
                e.unseen = true, e.unseenFor = rc.unseenFor, e.lastOff = rc.off, e.lastSpeed = rc.lastSpeed;
                e.quiet = why == Why::kOtherHolds || why == Why::kMoment;
                Add(r, e);
                if (!e.quiet)
                    Double(r, st, s, i);
            }
        }
    }

    // 3. A draw waiting: for the game's switch (its second key, giving up), or for a hand not seen to be seen again. An away gun's: no key, its gun
    // comes by the take's arrival (Arrive ends the wait), or GiveBackFailed (not asked, or the game said no); its take never landed in kGiveBackMost:
    // given up (said, felt with both hands empty). Landed (its record back in its slot) but not into the hand: its line is pickups' net's.
    if (st.wait.on && st.wait.giveBack) {
        Wait& w = st.wait;
        w.since += dt;
        // (The drawing hand opened, or took something, while its gun was on its way: it doesn't keep it, so the gun comes to its place, never into an
        // open hand that would drop it three frames on: Arrive. Review finding 9.)
        if (w.keeps && w.side >= 0 && w.side <= 1 && ((f.hand[w.side].read && st.grip[w.side].low >= kLetGoFrames) || HoldsThing(f, w.side)))
            w.keeps = false;
        if (!(w.since < kGiveBackMost)) {
            if (w.away >= 0 && w.away < kMostAway && st.away[w.away].on) {
                Event e;
                e.what = What::kNoSwitch, e.side = w.side, e.place = w.place, e.slot = w.slot, e.why = Why::kGiveBack, e.since = w.since;
                e.giveBack = true;
                CopyName(e.gun, st.away[w.away].name);
                CopyName(e.otherGun, st.away[w.away].instead);
                Add(r, e);
                if (Empty(f, 1 - w.side))
                    Double(r, st, s, w.side);
            }
            w = Wait{};
        }
    } else if (st.wait.on) {
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
            int slot = -1, awayGun = -1;
            if (away) {
                // (Only where it would stay: never a place that sends it to another, nor a shoulder another gun has.)
                const Place to = StowTo(s, f.slots, st, st.gameSlot, now);
                can = to == now && StowWhy(s, f, i, to) == Why::kNone && !(h.aims && f.holder == Holder::kHeld && f.trigger == i);
                slot = st.gameSlot;
            } else if (Empty(f, i)) {
                // (An away gun only within the game's reach of its pickup: DrawWhy's kFar otherwise.)
                bool caseA = false, stowOther = false;
                can = DrawWhy(s, f, st, i, now, slot, caseA, stowOther, &awayGun) == Why::kNone;
            }
            if (can) {
                Buzz(r, s, i, kTickAmp, kTickSeconds);
                Event e;
                e.what = What::kTick, e.side = i, e.place = now, e.away = away, e.slot = slot;
                if (awayGun >= 0)
                    CopyName(e.gun, st.away[awayGun].name);
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
    // Put away by a hand: that is the gun's holster now, where the hand put it (StowTo's role), till a hand takes it out and drops it; its kind keeps it
    // through a new life (the memo). An away gun that had that shoulder gives it up, the newer word winning (one gun a shoulder, never blocked for good:
    // review finding 1): it stays where the game has it, in its pickup on the ground or with the game, on no shoulder (said).
    if ((a.stowed == 0 || a.stowed == 1) && st.gameSlot >= 0 && st.gameSlot < kSlots) {
        const Role role = st.plannedRole[a.stowed] != Role::kNone ? st.plannedRole[a.stowed] : KitRole(f.slots, s, st.gameSlot);
        if (role != Role::kNone) {
            st.home[st.gameSlot].role = role;
            Remember(st, st.home[st.gameSlot].kind, role);
            for (int k = 0; k < kMostAway && (role == Role::kPrimary || role == Role::kSecondary); ++k) {
                Away& w = st.away[k];
                if (!w.on || w.role != role)
                    continue;
                Event e;
                e.what = What::kLost, e.side = a.stowed, e.slot = w.slot, e.why = Why::kTaken, e.weak = !w.entity, e.other = st.gameSlot;
                e.place = PlaceOfRole(role, false, s);
                CopyName(e.gun, w.name);
                CopyName(e.otherGun, w.instead);
                Add(r, e);
                if (st.wait.on && st.wait.giveBack && st.wait.away == k)
                    st.wait = Wait{};
                if (w.kind != st.home[st.gameSlot].kind)
                    Remember(st, w.kind, Role::kNone);
                w = Away{};
            }
        }
    }
    Place places[kSlots];
    PlacesNow(f.slots, st, s, places);
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
            if (e.why == Why::kTaken) {
                e.other = st.stowTakerSlot[i];
                if (st.stowTakerAway[i] >= 0 && st.stowTakerAway[i] < kMostAway)
                    CopyName(e.otherGun, st.away[st.stowTakerAway[i]].name);
            }
            Add(r, e);
            Double(r, st, s, i);
        }
    }
    if ((a.drawn == 0 || a.drawn == 1) && st.drawPlanned == a.drawn) {
        const int i = a.drawn;
        Event e;
        e.what = What::kDrawn, e.side = i, e.slot = st.gameSlot, e.place = st.gameSlot >= 0 ? places[st.gameSlot] : Place::kNone;
        e.unseen = st.drawWasHidden, e.since = st.drawHiddenFor;
        e.noPlace = st.drawNoPlace;
        st.drawNoPlace = false;
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
    bool haveSlots = false;  // the soldier's slots as now, for NoteSlots before anything is decided (Arrive comes before this frame's Update) ...
    SlotIn slots[kSlots];
    unsigned taken = 0;      // ... the takes since the last frame (Frame::taken) ...
    bool swapsSeen = true;   // ... and whether takes are seen (Frame::swapsSeen)
    bool lying = false;      // the gun before lay on the ground (the grasp kLoose, its copy there)
};
struct ArriveOut {
    Arrival arrival = Arrival::kToday;
    int side = -1;
    Event event;           // kCameAway when kept at its place for want of a hand; kToHand into the right hand with no home (what = kNone otherwise)
};
inline ArriveOut Arrive(const Settings& s, const ArriveIn& in, State& st) {
    ArriveOut out;
    // (Not a new life's guns: hands.cpp's OnGunChanged comes before its NewLife, and noted in the life before, the new kit's kinds would read as carried
    // before, another level's kit as the same level's. Update notes them, after NewLife; a new life's arrival is today's whatever their homes.)
    if (in.haveSlots && !in.newLife)
        NoteSlots(s, in.slots, in.taken, in.swapsSeen, st);
    if (in.slot >= 0 && in.slot < kSlots)
        st.gameSlot = in.slot, st.slotUnread = 0.0f;
    // A draw from a shoulder (the game's take of the pickup an away gun is in) whose hand opened while its gun came: at its place, put away (said), never
    // into that open hand, which would drop it three frames on (review finding 9). Its holster is back (NoteSlots above: Restored).
    if (s.on && in.take && !in.newLife && st.wait.on && st.wait.giveBack && !st.wait.keeps && st.gameSlot >= 0 && st.gameSlot < kSlots &&
        st.home[st.gameSlot].role != Role::kNone) {
        out.arrival = Arrival::kAtPlace;
        out.event.what = What::kCameAway, out.event.side = st.wait.side, out.event.slot = st.gameSlot, out.event.place = st.wait.place;
        out.event.why = Why::kOpened;
        st.wait = Wait{};
        return out;
    }
    if (!s.on || in.take || in.newLife) {
        st.wait = Wait{};  // (into a hand by today's rule: nothing waits for it any more, and no second key comes after it)
        return out;
    }
    // A holstered gun back in its slot just now (the game gave it back: Restored, ComeBack, said there).
    const bool comeback = st.comebackSlot >= 0 && st.comebackSlot == st.gameSlot && st.clock - st.comebackAt <= kTakeNear &&
                          st.home[st.gameSlot].role != Role::kNone;
    if (in.lying) {
        // The gun before lay on the ground. A draw from a shoulder that waited for this gun comes into the hand that drew it (the gun on the ground a
        // picked-up one let go of: it went into its slot, DrawWhy's LyingGoes; said); a holstered gun the game gave back is at its place (the level's
        // kit after the roof's AT4); else today's.
        Wait& w = st.wait;
        if (w.on && !w.gameWay && !w.giveBack && w.slot == st.gameSlot && w.keeps && w.side >= 0 && in.placed[w.side]) {
            out.arrival = Arrival::kHand, out.side = w.side;
            out.event.what = What::kDiscard, out.event.side = w.side, out.event.slot = st.lyingSlot, out.event.other = st.gameSlot;
            out.event.place = w.place;
            st.heldAt[st.gameSlot] = st.clock;
            w = Wait{};
            return out;
        }
        w = Wait{};
        if (comeback)
            out.arrival = Arrival::kAtPlace;
        return out;
    }
    if (!in.stowed) {
        st.wait = Wait{};
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
    // The game switched by itself to a gun with no home (one taken off the ground and never put on a shoulder, or given since): it can't be kept at
    // a place it doesn't have (it never jumps to a shoulder), so into the right hand as at a spawn.
    if (in.slot >= 0 && in.slot < kSlots && st.home[in.slot].role == Role::kNone && (!in.haveSlots || in.slots[in.slot].carried)) {
        out.arrival = Arrival::kHand, out.side = 1;
        out.event.what = What::kToHand, out.event.side = 1, out.event.slot = in.slot;
        w = Wait{};
        return out;
    }
    out.arrival = Arrival::kAtPlace;
    if (!comeback)  // (a holstered gun the game gave back has said so: kComeBack)
        out.event.what = What::kCameAway, out.event.slot = st.gameSlot, out.event.why = Why::kNone;
    return out;
}

// A draw from a shoulder whose take wasn't asked, or that the game refused (hands.cpp: pickups::RequestGiveBack's words, or the request's outcome):
// its wait ends; said at the next Update (felt then with the other hand empty). The gun stays on its shoulder, in its pickup.
inline void GiveBackFailed(State& st, int side, const char* words, float distance = -1.0f) {
    Wait& w = st.wait;
    if (!w.on || !w.giveBack || w.side != side)
        return;
    Event e;
    e.what = What::kRefused, e.why = Why::kGiveBack, e.side = side, e.place = w.place, e.slot = w.slot, e.giveBack = true, e.distance = distance;
    if (w.away >= 0 && w.away < kMostAway && st.away[w.away].on) {
        CopyName(e.gun, st.away[w.away].name);
        CopyName(e.otherGun, st.away[w.away].instead);
    }
    size_t n = 0;
    if (words)
        for (; words[n] && n + 1 < sizeof(e.words); ++n)
            e.words[n] = words[n];
    e.words[n] = 0;
    Queue(st, e);
    if (side == 0 || side == 1)
        st.doubleDue[side] = true;
    w = Wait{};
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
    char a[64], b[64], c[64];
    // A slot's gun by its name now; an away gun's (no slot's gun now) by its own (Event::gun, Event::otherGun).
    const auto gun = [names](int slot, char* buf, size_t len) {
        const char* g = slot >= 0 && slot < kSlots && names && names[slot] && names[slot][0] ? names[slot] : nullptr;
        if (g)
            std::snprintf(buf, len, "the %s", g);
        else
            std::snprintf(buf, len, "the gun");
        return buf;
    };
    const auto own = [&](char* buf, size_t len) {
        if (e.gun[0])
            std::snprintf(buf, len, "the %s", e.gun);
        else
            gun(e.slot, buf, len);
        return buf;
    };
    const auto other = [&](char* buf, size_t len) {
        if (e.otherGun[0])
            std::snprintf(buf, len, "the %s", e.otherGun);
        else
            gun(e.other, buf, len);
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
        Put(out, size, n, "Holster: %s put away %s by the %s hand", own(a, sizeof(a)), AtName(e.place), hand);
        if (e.at != Place::kNone)
            Put(out, size, n, " (let go %s: its own place is %s)", AtName(e.at), Name(e.place));
        unseenPart();
        break;
    case What::kDrawn:
        if (e.off)
            Put(out, size, n, "Holster: holsters off: %s back in the right hand", own(a, sizeof(a)));
        else if (e.noPlace)
            Put(out, size, n, "Holster: %s in the right hand: it was put away on no shoulder", own(a, sizeof(a)));
        else if (e.unseen)
            Put(out, size, n, "Holster: %s in the %s hand (seen again after %.2f s)", own(a, sizeof(a)), hand, e.since);
        else
            Put(out, size, n, "Holster: %s drawn %s by the %s hand", own(a, sizeof(a)), FromName(e.place), hand);
        break;
    case What::kAsked:
        Put(out, size, n, "Holster: %s asked for %s by the %s hand", own(a, sizeof(a)), FromName(e.place), hand);
        if (e.giveBack) {
            Put(out, size, n, ": the game's own take of the pickup it's in (where %s lay", e.otherGun[0] ? other(b, sizeof(b)) : "the gun taken");
            if (e.distance >= 0.0f)
                Put(out, size, n, ", %.1f m away", e.distance);
            Put(out, size, n, ")");
            break;
        }
        unseenPart();
        if (e.key > 0)
            Put(out, size, n, ": key %d", e.key);
        if (e.unseen)
            Put(out, size, n, "%s in the hand once it's seen", e.key > 0 ? "," : ":");
        break;
    case What::kKeyAgain:
        Put(out, size, n, "Holster: %s: the game's own weapon still in slot %d after %.2f s: key %d again", own(a, sizeof(a)), e.other, e.since,
            e.key);
        break;
    case What::kNoSwitch:
        if (e.giveBack)
            Put(out, size, n, "Holster: %s not given back %s: the game's take of its pickup didn't come in %.1f s (it stays there, in its pickup)",
                own(a, sizeof(a)), FromName(e.place), e.since);
        else
            Put(out, size, n, "Holster: %s not drawn: the game didn't switch to it (%.2f s, %s)", own(a, sizeof(a)), e.since,
                e.twice ? "its key pressed twice" : "no second key: one could have done harm then");
        break;
    case What::kRefused:
        switch (e.why) {
        case Why::kNothing: Put(out, size, n, "Holster: nothing %s", AtName(e.place)); break;
        case Why::kOtherHolds:
            Put(out, size, n, "Holster: nothing drawn %s: the %s hand holds %s", FromName(e.place), otherHand, other(b, sizeof(b)));
            break;
        case Why::kLying:
            Put(out, size, n, "Holster: nothing drawn %s: %s lies on the ground: take it first", FromName(e.place), other(b, sizeof(b)));
            break;
        case Why::kUnknown:
            Put(out, size, n, "Holster: %s not drawn: the hands don't know it yet (the D-pad or Y gives it the game's way)", own(a, sizeof(a)));
            break;
        case Why::kNoKey: Put(out, size, n, "Holster: %s not drawn: no key reaches slot %d", own(a, sizeof(a)), e.slot); break;
        case Why::kBusy: Put(out, size, n, "Holster: the %s hand's squeeze %s: the hand isn't empty", hand, AtName(e.place)); break;
        case Why::kMoment:
            Put(out, size, n, "Holster: the %s hand's squeeze %s: not now (a scripted moment, a reload, a take or a draw under way)", hand,
                AtName(e.place));
            break;
        case Why::kSlotUnread:
            Put(out, size, n, "Holster: the %s hand's squeeze %s: the game's own weapon not read yet", hand, AtName(e.place));
            break;
        case Why::kFar:
            // (The game gives a pickup only within its reach: 2.5 m and in sight, its own check 008B6380. Not moved nearer: review finding 2.)
            if (e.distance >= 0.0f)
                Put(out, size, n, "Holster: %s not drawn %s: the game keeps it in the pickup where %s lay, %.1f m away, and gives it back only within "
                    "2.5 m of it and in sight: go back there and draw again, or take it off the floor", own(a, sizeof(a)), FromName(e.place),
                    e.otherGun[0] ? other(b, sizeof(b)) : "the gun taken", e.distance);
            else
                Put(out, size, n, "Holster: %s not drawn %s: the pickup the game keeps it in (where %s lay) isn't among the pickups seen near: go back "
                    "there and draw again, or take it off the floor", own(a, sizeof(a)), FromName(e.place),
                    e.otherGun[0] ? other(b, sizeof(b)) : "the gun taken");
            break;
        case Why::kWithGame:
            Put(out, size, n, "Holster: %s not drawn %s: the game took it out of its slot with no pickup (a script's kit): it's there again once the "
                "game gives it back", own(a, sizeof(a)), FromName(e.place));
            break;
        case Why::kGiveBack:
            Put(out, size, n, "Holster: %s not given back %s: the game didn't take its pickup (%s): it stays there, in the pickup where %s lay",
                own(a, sizeof(a)), FromName(e.place), e.words[0] ? e.words : "no reason given", e.otherGun[0] ? other(b, sizeof(b)) : "the gun taken");
            break;
        case Why::kHidden:
            Put(out, size, n, "Holster: %s not drawn %s: it comes from its pickup into a hand the headset sees", own(a, sizeof(a)), FromName(e.place));
            break;
        default: Put(out, size, n, "Holster: nothing drawn %s", FromName(e.place)); break;
        }
        unseenPart();
        break;
    case What::kStowRefused:
        Put(out, size, n, "Holster: %s not put away %s: ", own(a, sizeof(a)), AtName(e.at));
        if (e.why == Why::kOtherRocket || e.why == Why::kOtherRound || e.why == Why::kOtherMag)
            Put(out, size, n, "the %s hand holds %s", otherHand,
                e.why == Why::kOtherRocket ? "a rocket or a case" : e.why == Why::kOtherRound ? "a 40 mm round" : "its magazine");
        else if (e.why == Why::kOtherHolds)
            Put(out, size, n, "the %s hand holds it by its front", otherHand);
        else if (e.why == Why::kTaken)
            Put(out, size, n, "%s is there (one gun a shoulder)", other(b, sizeof(b)));
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
        Put(out, size, n, "Holster: the %s hand %s: a tick (%s can be %s there)", hand, AtName(e.place), own(a, sizeof(a)),
            e.away ? "put away" : "drawn");
        break;
    case What::kCalledOff:
        Put(out, size, n, "Holster: %s called off after %.2f s: %s (it stays at its place)", own(a, sizeof(a)), e.since,
            e.why == Why::kOpened   ? "the grip opened before the hand was seen"
            : e.why == Why::kPicked ? "a pick of another gun came first"
            : e.why == Why::kMoment ? "the game's weapon moved on meanwhile, or a scripted moment lasted"
                                    : "the hand not seen again in time");
        break;
    case What::kCameAway:
        if (e.why == Why::kOpened)
            Put(out, size, n, "Holster: %s came after the %s hand opened: it is at its place %s", own(a, sizeof(a)), hand, AtName(e.place));
        else if (e.why == Why::kLate)
            Put(out, size, n, "Holster: %s came after the %s hand stopped waiting: it is at its place %s", own(a, sizeof(a)), hand,
                AtName(e.place));
        else
            Put(out, size, n, "Holster: the game switched to %s by itself: it is at its place", own(a, sizeof(a)));
        break;
    case What::kGameWay:
        Put(out, size, n, "Holster: %s asked for the game's way %s by the %s hand: key %d (the hands don't know it yet)", own(a, sizeof(a)),
            FromName(e.place), hand, e.key);
        break;
    case What::kRecall:
        Put(out, size, n, "Holster: %s left the ground for its place (the %s hand squeezed %s)", other(b, sizeof(b)), hand, AtName(e.place));
        break;
    case What::kLapsed:
        Put(out, size, n, "Holster: %s not %s by the %s hand: free hands didn't take it in %.2f s (a scripted moment, or the hand not counted as "
            "tracked)", own(a, sizeof(a)), e.why == Why::kNone ? "put away" : "drawn", hand, e.since);
        break;
    case What::kLook:
        Put(out, size, n, "Holster: the %s hand %.0f cm right, %.0f cm up, %.0f cm forward of the neck point: %.2f of %s's size, %s", hand,
            e.from[0] * 100.0f, e.from[1] * 100.0f, e.from[2] * 100.0f, e.size, Name(e.place),
            e.seen == Seen::kSeen ? "seen" : e.seen == Seen::kGuessed ? "the runtime's guess" : "not seen");
        break;
    case What::kNoPlace:
        Put(out, size, n, "Holster: %s is on no shoulder (a gun goes on one only by your hand): the game keeps it in its slot %d, the D-pad or Y "
            "brings it into the right hand", own(a, sizeof(a)), e.slot);
        break;
    case What::kBackHome:
        Put(out, size, n, "Holster: %s back %s, where it came from (the game's own switch)", own(a, sizeof(a)), AtName(e.place));
        break;
    case What::kToHand:
        Put(out, size, n, "Holster: the game's weapon is %s, on no shoulder (a gun goes on one only by your hand): into the right hand as at a spawn",
            own(a, sizeof(a)));
        break;
    case What::kAway:
        if (e.weak)
            Put(out, size, n, "Holster: %s stays %s: the game took it out of its slot %d with no pickup (a script's kit, or a take the mod didn't "
                "see): it's there again once the game gives it back", own(a, sizeof(a)), AtName(e.place), e.slot);
        else
            Put(out, size, n, "Holster: %s stays %s: the game put it into the pickup where %s lay (its take, slot %d): a draw there brings it back "
                "with its rounds, within the game's reach of that pickup (2.5 m, in sight)", own(a, sizeof(a)), AtName(e.place),
                e.otherGun[0] ? other(b, sizeof(b)) : "the gun taken", e.slot);
        break;
    case What::kComeBack:
        Put(out, size, n, "Holster: %s back in its slot %d (%s)", own(a, sizeof(a)), e.slot, e.weak ? "the game gave it back" : "the game's take of its pickup");
        if (e.why == Why::kTaken)
            Put(out, size, n, ", but %s has %s now: on no shoulder (one gun a shoulder)", Name(e.at), other(b, sizeof(b)));
        else
            Put(out, size, n, ": its holster %s again", Name(e.place));
        if (e.pickupLeft)
            Put(out, size, n, "; the %s left in the pickup where %s lay stays there, on no shoulder", e.gun[0] ? e.gun : "gun",
                e.otherGun[0] && e.why != Why::kTaken ? other(b, sizeof(b)) : "the gun taken");
        break;
    case What::kLost:
        if (e.why == Why::kGone)
            Put(out, size, n, "Holster: %s went with the pickup the game took (where %s lay: it keeps no place for the gun it replaces): it's gone",
                own(a, sizeof(a)), e.otherGun[0] ? other(b, sizeof(b)) : "the gun taken");
        else if (e.why == Why::kAll)
            Put(out, size, n, "Holster: %s is on no shoulder now: the pickup the game took (where %s lay) took every gun, and can't give it back alone: "
                "it lies there", own(a, sizeof(a)), e.otherGun[0] ? other(b, sizeof(b)) : "the gun taken");
        else if (e.why == Why::kTaken && e.weak)
            Put(out, size, n, "Holster: %s is on no shoulder now: %s was put on %s (one gun a shoulder); when the game gives it back it's on no shoulder",
                own(a, sizeof(a)), gun(e.other, b, sizeof(b)), Name(e.place));
        else if (e.why == Why::kTaken)
            Put(out, size, n, "Holster: %s is on no shoulder now: %s was put on %s (one gun a shoulder); it stays in the pickup where %s lay (take it "
                "off the floor there)", own(a, sizeof(a)), gun(e.other, b, sizeof(b)), Name(e.place), e.otherGun[0] ? other(c, sizeof(c)) : "the gun taken");
        else
            Put(out, size, n, "Holster: %s isn't %s any more: the pickup it was in (where %s lay) %s", own(a, sizeof(a)), AtName(e.place),
                e.otherGun[0] ? other(b, sizeof(b)) : "the gun taken",
                e.why == Why::kNothing ? "holds another gun now" : "is gone from the pickups seen near");
        break;
    case What::kDiscard:
        Put(out, size, n, "Holster: %s drawn %s by the %s hand; %s you let go of went into its slot %d (the game draws one gun at a time): on no "
            "shoulder, the D-pad or Y brings it", other(b, sizeof(b)), FromName(e.place), hand, own(a, sizeof(a)), e.slot);
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

#pragma once
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
// The rules of the game's dropped guns that need no game: tools/picktest.cpp runs them. Matrices are three rows, rotation | translation, unless said.
namespace pickrules {

// The gun's drawn pose (its third-person mesh's model -> the world) from the pickup's transform (16 floats: rows right, up, forward, translation) and its
// gun's PickupSettings, as the client draws it at rest (0084A7C0): turned in its own axes by y, then x, then z (degrees), then moved by the offset in
// the world. A number not finite: false.
bool DrawnPose(const float* gameTransform16, const float* offset3, const float* rotationDegrees3, float* out12);

// A pickup's countdown by the policy. seconds: 0 the game's own, -1 never, N > 0 that long. own: it holds a gun that left the player's slots; then
// ownSeconds is the policy unless it's 0 (-1 never, N). unseen: kept while in view. rate: seconds of its countdown a second as last measured between
// two looks (1 if not known; taken within 1 to 60). raisedOnce: the mod has already moved this pickup's countdown for its present content (the caller
// resets it when the first blueprint changes or +0x10C jumps back up to TimeToLive). True with out when +0x10C should be written.
bool CountdownFor(int seconds, bool own, int ownSeconds, bool unseen, float ttl, float left, float rate, bool inView, bool& raisedOnce, float& out);

// Within 70 degrees of the look and 80 m of the eye.
bool InView(const float* eye3, const float* look3, const float* at3);

// A slot's weapon, its firing state, its blueprint. A new object at a freed one's address with another blueprint is another gun.
struct Triple {
    uint32_t weapon = 0, firing = 0, blueprint = 0;
    bool operator==(const Triple& o) const { return weapon == o.weapon && firing == o.firing && blueprint == o.blueprint; }
};
// The slots whose triple changed (indexes into changed; how many).
int Diff(const Triple* before, const Triple* after, int slots, int* changed);
// A take has landed: any slot's triple changed, or the counts in the taken entry's slot aren't the ones there right before it (loadedBefore as written by
// the hand-over, spareBefore as read then). Only the same triples with both counts the same is "the game swapped nothing".
bool Landed(const Triple* before, const Triple* after, int slots, int slot, int loadedBefore, int spareBefore, int loadedAfter, int spareAfter);
// The one slot in the game's hand by its firing state's +0xA8 word: a weapon there, the word neither 0 (never in hand) nor 3 (put away). None, or more
// than one: -1.
int ActiveSlot(const Triple* slots, const int* stateA8, int count);

// A request's life on the game's thread: 500 ms from when it was asked (GetTickCount64's).
bool Fresh(unsigned long long askedAt, unsigned long long now);

// A pose, or a point, of the world (absolute: the arms' space plus its origin in whole metres) into the room: armsToRoom the inverse of loose's
// roomToArms, origin its Frame::origin. The origin comes off first, so the floats stay small.
void WorldToRoom(const float* armsToRoom12, const int* origin3, const float* world12, float* room12);
void WorldPointToRoom(const float* armsToRoom12, const int* origin3, const float* world3, float* room3);

// An arrival's start when the two meshes' frames aren't known to agree: the gun turned as the hand will hold it (handGunRoom's rotation), the middle of
// its box (model) on centreRoom. A number not finite, or a box with no size: false.
bool StartAtCentre(const float* handGunRoom12, const float* boxMin3, const float* boxMax3, const float* centreRoom3, float* out12);

// ---- What the render thread makes of a swap (hands.cpp's NoteSwaps, ammo.cpp's Swapped): pure, so picktest runs it. ----

// A gun of the player's is forgotten by ammo at a swap only when its own slot is among the swap's, or its firing state is among those that went. A
// swap that leaves a slot alone leaves that slot's gun as it was (its magazine out, the grenade launcher's count).
bool Forgets(const int* slot, const uint32_t* wentFiring, int changed, int ownSlot, uint32_t ownFiring);

// One swap as hands.cpp sees it.
struct SwapSeen {
    // The player's gun lies: a loose body of it, the grasp in the world. Put away at its place (holsters) it doesn't: nothing of it lies to retire, so a
    // take that swaps its slot is answered and due as with a gun in the hands, and nothing is ever swapped away or sent back by the net (picktest T22).
    bool lies = false;
    int lyingSlot = -1;                 // ... the slot it's in, as the game has it
    unsigned long long releasedAt = 0;  // ... when it was let go of (GetTickCount64)
    bool retired = false;               // ... retired already by an earlier swap
    unsigned long long at = 0;          // the swap: when it was made
    int changed = 0;                    // ... its slots, and what each held before
    const int* slot = nullptr;
    const uint32_t* wentBlueprint = nullptr;
    uint32_t takenBlueprint = 0;        // ... its taken entry's blueprint, and the slot that entry goes into
    int takenSlot = -1;
    bool forceSlot = false;             // ... the pickup makes the game select that slot
    bool byHand = false;                // ... a hand's take, the request it answers, the hand that asked
    uint32_t request = 0;
    int side = -1;
    uint32_t waiting = 0;               // the take hands.cpp waits on (0 none)
    uint32_t knownId = 0;               // the profile id free hands will have for the taken gun (0: none)
    bool noRow = false;                 // the guns' table has no row for its name: what it is can't be told before it's drawn
};
// What follows from it.
struct SwapVerdict {
    bool retire = false;        // the gun that lies went into the pickup: retired from now (false if it already was)
    bool answered = false;      // it answers the take waiting: that request is over
    int landedSide = -1;        // ... and its gun is due in that hand,
    uint32_t landedId = 0;      // ... as that profile,
    bool landedAny = false;     // ... or as any profile other than the one in force now (no row for its name)
    bool sameKind = false;      // ... the same gun as the one that lay: no change of gun will come
    bool refused = false;       // the grasp's wait ends now, with nothing in a hand
    bool freedAtOnce = false;   // ... because the game won't select the taken gun (another slot, not forced); its arrival stays due should it come
    bool unknown = false;       // ... because free hands don't know the gun
    bool swappedAway = false;   // the game's own pickup of the same gun as the one that lay: back in the right hand now
};
SwapVerdict OnSwap(const SwapSeen& s);

// The net, each frame after the swaps: a landed take whose gun isn't in the hand kNetMs after it landed is given up; a gun retired kNetMs ago with no
// take due goes back to the right hand. swappedAway is never true unless the gun still lies (raised with a gun in a hand, it would clear the grasp
// every frame).
constexpr unsigned long long kNetMs = 1500;
struct NetVerdict {
    bool landedOver = false;    // the landed take is given up
    bool refused = false;       // the grasp's wait ends
    bool swappedAway = false;   // back in the right hand, as the game has it
    bool retiredOver = false;   // the retired copy went some other way: nothing to give back
};
NetVerdict Net(bool lies, bool retired, unsigned long long retiredAt, int landedSide, unsigned long long landedAt, unsigned long long now);

// The M416's seated tell (skin::RifleSeated: its launcher's parts off their rest) puts free hands to sleep, but not within kTakeQuietMs of a hand's
// take that landed (tookAt, GetTickCount64's; 0 none): the game's switch to the gun taken moves those parts as a seat does, and the tell holds 0.5 s
// after the M416's last draw. Asleep then, the take's arrival was put off and the gun went back to the right hand (2026-11-12, the Stinger).
constexpr unsigned long long kTakeQuietMs = 2000;
bool SeatedSleeps(bool seatedTell, unsigned long long tookAt, unsigned long long now);

// A take's hand-over count goes back into a gun that stayed in the soldier's hands: its slot's triple the same after the take as before it. With
// ReplaceAllContent every slot held is handed over, and the Stinger's take left the M416 in slot 0 with the mod's count in it (one round less than
// the game's: its hidden round), a shot by the count with none fired (2026-11-12).
bool StaysWithSoldier(const Triple* before, const Triple* after, int slots, int slot);
// ... and the count the hand-over wrote into that slot goes back to the one it had (pickups.cpp, after a take): a gun that stayed, its count still the one
// written. The taken entry's slot is left out only when no slot's triple changed (the take landed by its counts alone: the game's write there). A dead
// soldier's AN-94 went into slot 1 (2026-11-13; its pickup preferring slot 0, its entry naming 1 as the take reads it: TakeVictims): the M416 left in
// slot 0 kept the hand-over's count, a round short.
bool GivesCountBack(const Triple* before, const Triple* after, int slots, int slot, int takenSlot, int loadedAfter, int wrote);

// ---- Which of the player's guns a take puts into the pickup: pure, so picktest runs it. ----
//
// Read in the game's take (008AB870, bf3_mem.exe, 2026-11-18): a pickup that is neither UnspawnOnPickup (+0x98), ContentIsStatic (+0x9F) nor
// ReplaceAllContent (+0xA2) first puts, for each of its content entries, the soldier's weapon in the slot that entry names (+0xA8; the list's own
// entry by that index, 00ABAA40) into the pickup (008AAED0, at 008ABA20-008ABA59: the entry's alternative and linked slots go along as data); then
// each entry is given to the soldier by its slot (the give loop, 008ABAE0-008ABC20: 00AC8110 puts the entry's gun in the slot it names, its clip and
// spare rounds set from the entry, 011E68A0 and 011E6B00). ReplaceAllContent instead puts a copy of every one of the soldier's weapons into it
// (008AB94E-008AB99F: 008AAED0 for each slot, the soldier's weapon left where it is) and the give loop still replaces only the slots the entries name:
// the guns that leave the soldier are those of the named slots, every take alike (the review of 2026-11-18; pickruntest R8's own model). Unspawn or
// static puts none in: the weapon in a named slot is gone for good (pickups' keepsNoPlace: never offered to a hand). The swaps seen in the game agree:
// an AN-94 drop naming slot 1 with slot 1 empty took nothing out (2026-11-13 01:21), others naming slot 1 took the AN-94 or the RPK-74M out of it,
// those naming slot 0 the M416 and its M320 (a null entry for slot 2), the M249 the M4A1 (2026-11-18).
constexpr int kMostEntries = 4;  // as pickups::kMostContent
struct TakeEntry {
    bool gun = false;  // a weapon in it (else a null entry: it empties its slot)
    int slot = -1;     // the slot it names
};
struct TakeIn {
    TakeEntry entry[kMostEntries];
    int entries = 0;
    int entriesAll = 0;    // every entry it has (0: as entries); more than are read: which slots it names isn't known
    bool replaceAll = false, unspawnOrStatic = false;
    unsigned carried = 0;  // bit s: the soldier's slot s holds a weapon
};
// More entries than are read: what the take would take out isn't known.
inline bool TakeUnread(const TakeIn& in) { return in.entriesAll > in.entries || in.entries > kMostEntries; }
// The slots whose weapon leaves the soldier by this take, as bits (only carried ones): the slots its entries name; every carried one when they aren't
// all read (not known: taken as the most it could be).
inline unsigned TakeVictims(const TakeIn& in) {
    if (TakeUnread(in))
        return in.carried;
    unsigned out = 0;
    for (int i = 0; i < in.entries && i < kMostEntries; ++i) {
        const int s = in.entry[i].slot;
        if (s >= 0 && s < 32 && ((in.carried >> s) & 1u))
            out |= 1u << s;
    }
    return out;
}

// ---- The slot of the gun in the hands (ammo.cpp's: every count of the gun in hand goes to it, recoil's rounds are counted in it): pure, so picktest
// runs it. ----
//
// The game keeps a gun taken up from the ground in whichever slot its take put it in: a dead soldier's AN-94 went into slot 1 with the player's M416
// left in slot 0 (2026-11-13), and with every count taken from slot 0 the AN-94 ran dry, the game reloaded it by itself, and recoil counted no round.
enum class Drawn : int {
    kNothing = 0,  // nothing in the hands the mod knows: the gun lying or flying, both hands empty, a scripted moment
    kRifle = 1,    // the M416 (the grenade launcher under it too: its own slot is counted apart)
    kHeld = 2,     // any other gun in the hands (skin.cpp's DrawHeldPart), not the Stinger
    kOwnPath = 3,  // the Stinger, the SMAW, the train pistol: their counts are their own paths' (ammo.cpp's StingerFiringState, LauncherFiringState)
};
struct HandSlotIn {
    Drawn drawn = Drawn::kNothing;
    bool known = false;      // the drawn gun's blueprint is known (its row in the guns' table)
    unsigned weapon = 0;     // bit s (slots 0 and 1): that slot holds a weapon with a firing state
    unsigned byName = 0;     // bit s: that slot holds the drawn gun's blueprint, by its name
    unsigned rifleName = 0;  // bit s: that slot holds the M416's blueprint (a test's switch draws another gun under its name; the rule leans on it no more)
    int word[2] = {-1, -1};  // each slot's firing state's +0xA8 word (-1 not read): 0 never in hand, 3 put away, anything else in hand (the M320's, 2026-11-05)
    int landed = -1;         // the slot (0 or 1) the last take this life put its gun into, or the last pick (key 1 or 2) since; -1 none
    int last = 0;            // the last answer
    bool lastSame = false;   // ... and that slot still holds the gun it held then (its weapon, firing state and blueprint)
    int current = -1;        // the game's own current weapon: the slot in its soldier's hands as its weapons keep it (-1 not read)
    unsigned handGun = 0;    // bit s: that slot's gun is one the hands work the magazine of: the guns' table has its first-person body, and it is
                             // no launcher and no pistol (the Stinger, the train pistol and a gun not yet measured stay as they were)
};
struct HandSlotOut {
    int slot = 0;
    const char* why = "";  // the reason, for the log ("by its name" ...)
};
// In order: the Stinger, the SMAW and the train pistol the first slot, as before. The rifle or a held gun: the one of slots 0 and 1 holding its
// blueprint by name, whatever the words say. Two of a kind: the sure words (one slot in hand, the other empty or put away), else the words, else
// where the last take or pick (key 1 or 2) put one, else as it was,
// else the first slot. Neither by name: the sure words; else a gun the table knows is under another's name (a test's switch: the first slot); a gun it
// doesn't know goes where the last take or pick put one, else as it was. Nothing drawn (the headset off, the hands asleep, the first seconds after a
// load): the game's own current weapon when that is one of the first two slots and its gun one the hands work the magazine of (a load put the
// soldier's AN-94 of slot 1 in its hands and the counts stayed with slot 0 until the hands were drawn, 2026-11-13); else as it was while that slot's
// gun is the same, else the first slot.
HandSlotOut HandSlot(const HandSlotIn& in);
// The one of slots 0 and 1 in the game's hand by its words: a weapon there with a word neither 0 nor 3; -1 none or both. Sure: the other slot is empty
// or put away (3), not merely never in hand.
int WordSlot(const HandSlotIn& in, bool sure);

// ---- The game's count of the gun in the hands, a frame (ammo.cpp's Update, in whichever slot the gun is): what the mod does with what the game did.
// Pure, so picktest runs it. ----
//
// The game's count has a round more than the mod says (the hidden round): the trigger stops short, so the game never runs dry and never reloads by
// itself. The game, a frame or two behind the trigger, sometimes fires that one too, and the mouse (as the films fire) passes no stop at all: the M416
// at 1 of 370 fired to 0 and the game reloaded it to 30 of 340 by itself, unseen, as only a reload from one round or more was looked for (2026-11-13).
// The rule (the user, 2026-11-13): whatever gun is in the hands, the game never reloads it by itself; X held asks for its reload, and that stays.
enum class CountSeen : int {
    kNone = 0,     // nothing to do
    kKept = 1,     // the game's reload from one round or more, asked for (X held, weapon.txt "empty", a swap of the game's just now) or on a gun whose
                   // magazine isn't worked by hand: kept, as before (a magazine and the chambered round its size, the hidden round back on top)
    kUndone = 2,   // the game's reload of a gun whose magazine is worked by hand, not asked for, from any count: its count back to what it was (the hidden
                   // round at least), every round it took given back to the spare ones
    kPutBack = 3,  // the hidden round fired (the game's count at 0, spare rounds left, nothing asked): the count back to the hidden round at once, taken
                   // from the spare ones, so the game has nothing to reload; at most kMostPutBacks times a magazine (then the reload is undone instead)
};
constexpr int kReloadJump = 5;  // a reload: the game's count up by this many or more since the last frame, the mod writing nothing
constexpr unsigned long long kReloadAskedMs = 8000;  // R let go this long ago (X held) still asks for the reload: the game's runs on after it
constexpr unsigned long long kSwapAskedMs = 2000;    // a swap of the game's this recent: its counts are the game's take, not a reload
constexpr int kMostPutBacks = 3;  // a magazine's put-backs (the mouse fires each again at once: no stream of rounds from the spare ones)
struct CountIn {
    int last = -1;        // the game's count last frame (-1 not seen yet: a new gun)
    int raw = 0;          // ... now
    int spare = 0;        // its spare rounds now
    int hidden = 0;       // the hidden round in its count (0 or 1)
    bool byHand = false;  // its magazine worked by hand (the M416, a held gun with its magazine by hand), and not a gun of its own path drawn
    bool asked = false;   // its reload asked for: X held (R down, or let go within kReloadAskedMs), weapon.txt "empty", a swap within kSwapAskedMs
    int putBacks = 0;     // put-backs since its count was last 2 or more
};
struct CountOut {
    CountSeen what = CountSeen::kNone;
    int loaded = 0;      // the game's count to write (kUndone, kPutBack; else as it is)
    int spare = 0;       // the spare rounds to write (likewise)
};
CountOut SeeCount(const CountIn& in);

// ---- A magazine taken out by hand (ammo.cpp's Eject): the rounds that stay in the gun, as the mod counts them. Pure, so picktest runs it. ----
//
// A gun with rounds in it keeps one, the chambered round; the rest go back to the spare ones with the magazine. An empty gun keeps one all the
// same, from the spare ones if it has any: the trigger stops at one round left (that one never fires), so a gun the game fired past that stop gets
// its floor back, and a fresh magazine then fires all its rounds, as from a gun run dry the usual way. An empty gun with no spare ones keeps none.
int StaysAtEject(int loaded, int spare);

// ---- A change of the gun in a slot under the gun drawn (a scene's kit, a checkpoint's new soldier, a take): never the drawn gun's rounds, nor its
// data. Pure, so picktest and at4test run it. ----
//
// Operation Swordbreaker's hotel roof (build\designs\at4_measured.md, sections 3 and 11): the prompt's kit puts the AT4 (1 loaded) in slot 0 0.1 s
// after the use, while the M4A1 it took is drawn 0.3 s more; it gives a fresh M4A1 back to slot 0 while the AT4's tube is drawn half a second more.
// On main's DLL both side effects showed: recoil took the AT4's six kick rows out of its data under the M4A1's name for 0.91 s, and the held
// magazine read the AT4's one round as the M4A1 run dry (its chamber "empty" till a magazine and its lever; the kit's M4A1 given back with 31 then
// passed for the game's reload, a gun given back with fewer than 5 rounds more than the one before would have stayed "empty").
//
// The slot's gun by its name (its row in the guns' table: ammo.cpp's RowInSlot) and the gun drawn (its row by its body or its shape) are two guns:
// either not known, they're taken as one (as before). One row a gun: rows compare as pointers.
template <class Row> bool OtherGun(const Row* drawn, const Row* inSlot);
// The held magazine's run-dry rule (hands.cpp: a gun whose magazine the hands work, its chamber empty once its count is down to the last round, till
// a magazine is in and its lever pulled), a frame: a drop is that gun's own rounds only while the slot still holds the very gun counted the frame
// before (its firing state the same) and, by its name, the gun drawn. The name is ammo.cpp's, read at the top of the frame with that slot's firing
// state; the count and the firing state are read later in it (hands::Update, after the headset's waits): a kit's swap between the two (the roof's
// AT4, about a third of the frame) leaves the name a frame behind, and nothing is decided by it till it's read with the firing state counted.
enum class DrySeen : int {
    kNone = 0,      // nothing to do
    kRunDry = 1,    // its count down to the last round, its magazine in: run dry (a new magazine, then the lever)
    kReloaded = 2,  // its count up by more than 4 with nothing seated by hand: the game's own reload: its chamber filled
    kNewGun = 3,    // another gun in the slot (another firing state), by its name the one drawn: its chamber as its count is (DryOut::dry)
    kNotDrawn = 4,  // the slot holds another gun than the one drawn (OtherGun): its count isn't the drawn gun's; nothing read, the chamber as it was
    kWaiting = 5,   // the slot's name wasn't read with the firing state counted now (DryIn::nameWithIt): nothing decided; the count and the firing
                    // state before kept (DryOut::keepFiring), so the next frame's, its name read with it, decides
};
struct DryIn {
    int was = -1;               // its count the frame before (-1: none read, afresh)
    uint32_t firingWas = 0;     // ... of this firing state (0 none)
    int now = 0;                // its count now
    uint32_t firing = 0;        // ... the hands' slot's firing state now
    bool otherGun = false;      // OtherGun(the drawn gun's row, the slot's row)
    bool nameWithIt = true;     // the slot's name (otherGun's) read with this very firing state (ammo's read of the slot this frame holds it)
    bool magOut = false;        // its magazine out (skin's)
    bool seated = false;        // a magazine seated by hand this frame
};
struct DryOut {
    DrySeen what = DrySeen::kNone;
    bool dry = false;           // kNewGun: its chamber empty (its count at the last round or none, its magazine in)
    int was = -1;               // the count to keep for the next frame (-1: afresh)
    bool keepFiring = false;    // kWaiting: the firing state counted before kept for the next frame (not this one's)
};
DryOut SeeDry(const DryIn& in);

// The owner's flow (b) on Operation Swordbreaker's roof (2026-11-18: "they could momentarily drop their rifle on the floor, pick up the AT4, shoot it,
// discard it, then pick up their rifle again"): the AT4 is no pickup but the scene's kit (PlayerAT4 into the M4A1's slot), so the lying rifle's game
// gun goes with it, and nothing of it can be drawn while the game draws the AT4 (no hosting). hands.cpp keeps where it lay (LyingTaken at the change
// of gun to the scene's gift), and when the game's gun of its kind is back in that slot (the level's kit, 1.3-3.3 s after the shot) it lies there
// again (LyingBack), still, as it lay: taken off the floor by a hand as before, never into a hand or onto a shoulder by itself.
struct LyingTakenIn {
    bool gift = false;               // the change of gun is a scene's gift: a prompt used by a hand (UseMade), or X held's R at one (PromptRSince)
    bool lies = false;               // the game's weapon before lay on the ground (the grasp kLoose, its copy there now)
    bool retired = false;            // ... a pickup's swap took it: it's in that pickup, where the game draws it (not this)
    uint32_t lyingId = 0, newId = 0;  // the lying gun's profile and the new gun's (0: not known)
    int lyingSlot = -1, newSlot = -1;  // the slot it lay from (its own), and the new gun's (the game's slot now)
};
bool LyingTaken(const LyingTakenIn& in);
constexpr unsigned long long kLyingKeepsMs = 120000;  // the scene longest from the take to the kit's return: the fire window opens 13.5 s on, 19.5 s open
constexpr float kLyingNear = 6.0f;                    // m from the eye at most to where it lay (the "RPG" state holds the player where they took it)
enum class LyingBackVerdict : int {
    kNone = 0,  // not that gun back: the record waits
    kLie = 1,   // that gun back in its slot by the scene: it lies again where it lay
    kDrop = 2,  // the record is over (why says why): that gun came back another way (a take, a pick of the player's, another slot), it lay too far
                // or where isn't known, too long ago, a new life
};
struct LyingBackIn {
    bool on = false;                 // a record (LyingTaken's)
    uint32_t id = 0;                 // ... its gun's profile
    int slot = -1;                   // ... its slot
    unsigned long long at = 0;       // ... when the scene took it (ms)
    uint32_t newId = 0;              // the gun the game draws now (this change of gun's)
    int newSlot = -1;                // ... its slot
    bool take = false;               // ... a take's arrival (a hand's, the magnet's, X held's pickup, a draw's give-back): its rules, into the hand
    bool picked = false;             // a pick of the player's since the record (the D-pad, Y held): their way, into the right hand
    bool newLife = false;            // a death, a checkpoint, another level: the world it lay in is gone
    unsigned long long now = 0;      // ms
    float distance = -1.0f;          // m from the eye now to where it lay (under 0: not known)
};
struct LyingBackOut {
    LyingBackVerdict verdict = LyingBackVerdict::kNone;
    const char* why = "";
};
LyingBackOut LyingBack(const LyingBackIn& in);

// The seqlock pair (the writer bumps to odd, writes, bumps to even; the reader copies and compares), templated on the payload.
template <class T> struct SeqLock {
    std::atomic<uint32_t> seq{0};
    T value{};
    void Write(const T& v);
    bool Read(T& out, int tries = 4) const;
};

}  // namespace pickrules

// ---- The bodies of the above (inline: this header is all of it). ----
namespace pickrules {

namespace detail {
inline bool Finite(const float* v, int n) {
    for (int i = 0; i < n; ++i)
        if (!std::isfinite(v[i]))
            return false;
    return true;
}
// a times b, both 3x3 (rows), into out (not a or b).
inline void Mul3(const double* a, const double* b, double* out) {
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            out[3 * r + c] = a[3 * r] * b[c] + a[3 * r + 1] * b[3 + c] + a[3 * r + 2] * b[6 + c];
}
}  // namespace detail

// The client's three turns as it builds them (rows, a point times them; cos and sin of degrees x pi / 180): y (00428D30) rows (c 0 -s) (0 1 0)
// (s 0 c), x (00557AA0) rows (1 0 0) (0 c s) (0 -s c), z (00557B50) rows (c s 0) (-s c 0) (0 0 1). Each is put before the transform in turn, so
// the drawn rows are Rz Rx Ry M; the offset is then added to the translation.
inline bool DrawnPose(const float* gameTransform16, const float* offset3, const float* rotationDegrees3, float* out12) {
    if (!gameTransform16 || !offset3 || !rotationDegrees3 || !out12 || !detail::Finite(gameTransform16, 16) || !detail::Finite(offset3, 3) ||
        !detail::Finite(rotationDegrees3, 3))
        return false;
    const double toRadians = 3.14159274101257 / 180.0;  // the game's pi (0206EBE0) over its 180 (0206EBD8)
    const double ax = rotationDegrees3[0] * toRadians, ay = rotationDegrees3[1] * toRadians, az = rotationDegrees3[2] * toRadians;
    const double cx = std::cos(ax), sx = std::sin(ax), cy = std::cos(ay), sy = std::sin(ay), cz = std::cos(az), sz = std::sin(az);
    const double ry[9] = {cy, 0.0, -sy, 0.0, 1.0, 0.0, sy, 0.0, cy};
    const double rx[9] = {1.0, 0.0, 0.0, 0.0, cx, sx, 0.0, -sx, cx};
    const double rz[9] = {cz, sz, 0.0, -sz, cz, 0.0, 0.0, 0.0, 1.0};
    double m[9];
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            m[3 * r + c] = gameTransform16[4 * r + c];
    double a[9], b[9];
    detail::Mul3(ry, m, a);
    detail::Mul3(rx, a, b);
    detail::Mul3(rz, b, a);
    // Its rows are the model's axes in the world: here they're the columns.
    float out[12];
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c)
            out[4 * r + c] = static_cast<float>(a[3 * c + r]);
        out[4 * r + 3] = gameTransform16[12 + r] + offset3[r];
    }
    if (!detail::Finite(out, 12))
        return false;
    std::memcpy(out12, out, sizeof(out));
    return true;
}

inline bool CountdownFor(int seconds, bool own, int ownSeconds, bool unseen, float ttl, float left, float rate, bool inView, bool& raisedOnce,
                         float& out) {
    if (!std::isfinite(ttl) || !std::isfinite(left) || !(ttl > 0.0f))
        return false;
    const float r = std::isfinite(rate) ? std::fmin(std::fmax(rate, 1.0f), 60.0f) : 1.0f;
    const int policy = own && ownSeconds != 0 ? ownSeconds : seconds;
    bool write = false;
    float v = left;
    if (policy > 0 && !raisedOnce) {
        v = std::fmax(0.5f, left + (static_cast<float>(policy) - ttl));
        raisedOnce = true;
        write = true;
    } else if (policy == -1 && left < std::fmax(30.0f, 3.0f * r)) {
        v = ttl;
        write = true;
    }
    const float hold = std::fmax(2.0f, 3.0f * r);
    if (unseen && inView && v < hold) {
        v = hold;
        write = true;
    }
    if (write)
        out = v;
    return write;
}

inline bool InView(const float* eye3, const float* look3, const float* at3) {
    if (!eye3 || !look3 || !at3 || !detail::Finite(eye3, 3) || !detail::Finite(look3, 3) || !detail::Finite(at3, 3))
        return false;
    const double d[3] = {static_cast<double>(at3[0]) - eye3[0], static_cast<double>(at3[1]) - eye3[1], static_cast<double>(at3[2]) - eye3[2]};
    const double distance = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    const double lookLength = std::sqrt(static_cast<double>(look3[0]) * look3[0] + static_cast<double>(look3[1]) * look3[1] +
                                        static_cast<double>(look3[2]) * look3[2]);
    if (distance > 80.0 || lookLength < 1e-6)
        return false;
    if (distance < 1e-6)
        return true;  // at the eye
    const double cosine = (d[0] * look3[0] + d[1] * look3[1] + d[2] * look3[2]) / (distance * lookLength);
    return cosine >= std::cos(70.0 * 3.14159265358979323846 / 180.0);
}

inline int Diff(const Triple* before, const Triple* after, int slots, int* changed) {
    if (!before || !after)
        return 0;
    int n = 0;
    for (int i = 0; i < slots; ++i)
        if (!(before[i] == after[i])) {
            if (changed)
                changed[n] = i;
            ++n;
        }
    return n;
}

inline bool Landed(const Triple* before, const Triple* after, int slots, int slot, int loadedBefore, int spareBefore, int loadedAfter, int spareAfter) {
    if (Diff(before, after, slots, nullptr) > 0)
        return true;
    return slot >= 0 && slot < slots && (loadedAfter != loadedBefore || spareAfter != spareBefore);
}

// (A word not read, below 0, says nothing either way.)
inline int ActiveSlot(const Triple* slots, const int* stateA8, int count) {
    if (!slots || !stateA8)
        return -1;
    int found = -1;
    for (int i = 0; i < count; ++i) {
        if (!slots[i].weapon || stateA8[i] <= 0 || stateA8[i] == 3)
            continue;
        if (found >= 0)
            return -1;
        found = i;
    }
    return found;
}

inline bool Fresh(unsigned long long askedAt, unsigned long long now) {
    return now >= askedAt && now - askedAt <= 500;
}

inline void WorldPointToRoom(const float* armsToRoom12, const int* origin3, const float* world3, float* room3) {
    const double a[3] = {static_cast<double>(world3[0]) - origin3[0], static_cast<double>(world3[1]) - origin3[1],
                         static_cast<double>(world3[2]) - origin3[2]};
    float out[3];
    for (int r = 0; r < 3; ++r)
        out[r] = static_cast<float>(armsToRoom12[4 * r] * a[0] + armsToRoom12[4 * r + 1] * a[1] + armsToRoom12[4 * r + 2] * a[2] + armsToRoom12[4 * r + 3]);
    std::memcpy(room3, out, sizeof(out));
}

inline void WorldToRoom(const float* armsToRoom12, const int* origin3, const float* world12, float* room12) {
    const float t[3] = {world12[3], world12[7], world12[11]};
    float out[12], moved[3];
    WorldPointToRoom(armsToRoom12, origin3, t, moved);
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c)
            out[4 * r + c] = static_cast<float>(static_cast<double>(armsToRoom12[4 * r]) * world12[c] +
                                                static_cast<double>(armsToRoom12[4 * r + 1]) * world12[4 + c] +
                                                static_cast<double>(armsToRoom12[4 * r + 2]) * world12[8 + c]);
        out[4 * r + 3] = moved[r];
    }
    std::memcpy(room12, out, sizeof(out));
}

inline bool StartAtCentre(const float* handGunRoom12, const float* boxMin3, const float* boxMax3, const float* centreRoom3, float* out12) {
    if (!handGunRoom12 || !boxMin3 || !boxMax3 || !centreRoom3 || !out12 || !detail::Finite(handGunRoom12, 12) || !detail::Finite(boxMin3, 3) ||
        !detail::Finite(boxMax3, 3) || !detail::Finite(centreRoom3, 3))
        return false;
    float mid[3];
    for (int i = 0; i < 3; ++i) {
        if (!(boxMax3[i] - boxMin3[i] > 0.0f))
            return false;
        mid[i] = 0.5f * (boxMin3[i] + boxMax3[i]);
    }
    float out[12];
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c)
            out[4 * r + c] = handGunRoom12[4 * r + c];
        out[4 * r + 3] = centreRoom3[r] - (handGunRoom12[4 * r] * mid[0] + handGunRoom12[4 * r + 1] * mid[1] + handGunRoom12[4 * r + 2] * mid[2]);
    }
    if (!detail::Finite(out, 12))
        return false;
    std::memcpy(out12, out, sizeof(out));
    return true;
}

inline bool Forgets(const int* slot, const uint32_t* wentFiring, int changed, int ownSlot, uint32_t ownFiring) {
    for (int i = 0; i < changed; ++i) {
        if (slot && slot[i] == ownSlot)
            return true;
        if (ownFiring && wentFiring && wentFiring[i] == ownFiring)
            return true;
    }
    return false;
}

inline SwapVerdict OnSwap(const SwapSeen& s) {
    SwapVerdict v;
    int at = -1;
    if (s.lies && s.lyingSlot >= 0 && s.slot)
        for (int i = 0; i < s.changed; ++i)
            if (s.slot[i] == s.lyingSlot) {
                at = i;
                break;
            }
    const bool took = s.lies && at >= 0 && s.at >= s.releasedAt;
    v.retire = took && !s.retired;
    v.sameKind = took && s.takenBlueprint != 0 && s.wentBlueprint && s.takenBlueprint == s.wentBlueprint[at];
    v.answered = s.byHand && s.waiting != 0 && s.request == s.waiting;
    if (v.answered) {
        if (s.side != 0 && s.side != 1) {
            v.refused = true;
        } else if (s.knownId) {
            v.landedSide = s.side;
            v.landedId = s.knownId;
        } else if (s.noRow) {
            v.landedSide = s.side;
            v.landedAny = true;
        } else {
            v.refused = true;
            v.unknown = true;
        }
        if (v.landedSide >= 0 && !took && !s.forceSlot) {
            v.refused = true;
            v.freedAtOnce = true;
        }
    } else if (!s.byHand && took && v.sameKind) {
        v.swappedAway = true;
    }
    return v;
}

inline NetVerdict Net(bool lies, bool retired, unsigned long long retiredAt, int landedSide, unsigned long long landedAt, unsigned long long now) {
    NetVerdict v;
    if (landedSide >= 0 && now > landedAt && now - landedAt > kNetMs) {
        v.landedOver = true;
        v.refused = true;
        if (retired && lies)
            v.swappedAway = true;
    }
    if (retired && !lies)
        v.retiredOver = true;
    else if (retired && (landedSide < 0 || v.landedOver) && now > retiredAt && now - retiredAt > kNetMs)
        v.swappedAway = true;
    return v;
}

inline bool SeatedSleeps(bool seatedTell, unsigned long long tookAt, unsigned long long now) {
    return seatedTell && !(tookAt && now >= tookAt && now - tookAt < kTakeQuietMs);
}

inline bool StaysWithSoldier(const Triple* before, const Triple* after, int slots, int slot) {
    return before && after && slot >= 0 && slot < slots && before[slot].weapon && before[slot] == after[slot];
}

inline bool GivesCountBack(const Triple* before, const Triple* after, int slots, int slot, int takenSlot, int loadedAfter, int wrote) {
    return (slot != takenSlot || Diff(before, after, slots, nullptr) > 0) && StaysWithSoldier(before, after, slots, slot) && loadedAfter == wrote;
}

inline int WordSlot(const HandSlotIn& in, bool sure) {
    int found = -1;
    for (int s = 0; s < 2; ++s) {
        if (!((in.weapon >> s) & 1u) || in.word[s] <= 0 || in.word[s] == 3)
            continue;
        if (found >= 0)
            return -1;
        found = s;
    }
    if (found < 0 || !sure)
        return found;
    const int other = 1 - found;
    return !((in.weapon >> other) & 1u) || in.word[other] == 3 ? found : -1;
}

inline HandSlotOut HandSlot(const HandSlotIn& in) {
    auto answer = [](int slot, const char* why) {
        HandSlotOut out;
        out.slot = slot == 1 ? 1 : 0;  // (only the first two: the grenade launcher's 2, the SMAW's 5, the grenades' and the knife's are never the hands')
        out.why = why;
        return out;
    };
    const bool lastOk = in.lastSame && (in.last == 0 || in.last == 1);
    const bool landedOk = (in.landed == 0 || in.landed == 1) && ((in.weapon >> in.landed) & 1u);
    if (in.drawn == Drawn::kOwnPath)
        return answer(0, "the Stinger, the SMAW or the train pistol: the first slot, their counts their own");
    if (in.drawn == Drawn::kRifle || in.drawn == Drawn::kHeld) {
        const unsigned named = in.byName & in.weapon & 3u;
        const int sure = WordSlot(in, true), words = WordSlot(in, false);
        if (named == 1u || named == 2u) {
            // (Never the words over a single name: with the player's M416 in slot 0, a real AN-94 in slot 1 reads as a test's switch beside one
            // would, and words a state late would take its counts to the M416.)
            return answer(named == 1u ? 0 : 1, "by its name");
        }
        if (named == 3u) {
            if (sure >= 0)
                return answer(sure, "two of its kind: by the game's words");
            if (words >= 0)
                return answer(words, "two of its kind: by the game's words (the other never in hand)");
            if (landedOk)
                return answer(in.landed, "two of its kind: the one the last take or pick put there");
            if (lastOk)
                return answer(in.last, "two of its kind: as it was");
            return answer(0, "two of its kind: the first slot");
        }
        if (sure >= 0)
            return answer(sure, "not by its name: by the game's words");
        if (in.known)
            return answer(0, "under another's name (a test's switch): the first slot");
        if (landedOk)
            return answer(in.landed, "a gun the table doesn't know: where the last take or pick put it");
        if (lastOk)
            return answer(in.last, "a gun the table doesn't know: as it was");
        return answer(0, "a gun the table doesn't know: the first slot");
    }
    if ((in.current == 0 || in.current == 1) && ((in.weapon >> in.current) & (in.handGun >> in.current) & 1u))
        return answer(in.current, "nothing drawn: the game's own current weapon");
    if (lastOk)
        return answer(in.last, "nothing drawn: as it was");
    return answer(0, "nothing drawn, that slot's gun not the same: the first slot");
}

inline CountOut SeeCount(const CountIn& in) {
    CountOut out;
    out.loaded = in.raw, out.spare = in.spare;
    if (in.last < 0 || in.raw < 0 || in.spare < 0)
        return out;
    // (Back to what it was, the hidden round at least: fired, then reloaded before it could be put back.) From empty as the mod counts it (nothing but
    // the hidden round), any rise past that is a reload, the last few spare rounds' too (past kMostPutBacks).
    const int back = in.last > in.hidden ? in.last : in.hidden;
    const bool fromEmpty = in.last <= in.hidden && in.raw > back;
    if (in.raw >= in.last + kReloadJump || (fromEmpty && in.byHand && !in.asked)) {
        if (in.byHand && !in.asked) {
            out.what = CountSeen::kUndone;
            out.loaded = back;
            out.spare = in.spare + (in.raw - back);
        } else if (in.last >= 1) {
            out.what = CountSeen::kKept;
        }
        return out;
    }
    if (in.raw == 0 && in.hidden > 0 && in.byHand && !in.asked && in.spare >= in.hidden && in.putBacks < kMostPutBacks) {
        out.what = CountSeen::kPutBack;
        out.loaded = in.hidden;
        out.spare = in.spare - in.hidden;
    }
    return out;
}

inline int StaysAtEject(int loaded, int spare) {
    return loaded >= 1 || spare >= 1 ? 1 : 0;
}

template <class Row> inline bool OtherGun(const Row* drawn, const Row* inSlot) {
    return drawn && inSlot && drawn != inSlot;
}

inline DryOut SeeDry(const DryIn& in) {
    DryOut out;
    if (!in.nameWithIt) {
        out.what = DrySeen::kWaiting;  // (a name not its own decides nothing: neither another gun nor a new one of the kind drawn)
        out.was = in.was;
        out.keepFiring = true;
        return out;
    }
    if (in.otherGun) {
        out.what = DrySeen::kNotDrawn;  // (afresh once the slot's gun is the one drawn again)
        return out;
    }
    out.was = in.now;
    if (in.firingWas && in.firing && in.firing != in.firingWas) {
        out.what = DrySeen::kNewGun;
        out.dry = in.now <= 1 && !in.magOut;
        return out;
    }
    if (in.was > 1 && in.now <= 1 && !in.magOut)
        out.what = DrySeen::kRunDry;
    else if (in.was >= 0 && in.now > in.was + 4 && !in.seated)
        out.what = DrySeen::kReloaded;
    return out;
}

inline bool LyingTaken(const LyingTakenIn& in) {
    return in.gift && in.lies && !in.retired && in.lyingId != 0 && in.newId != 0 && in.newId != in.lyingId && in.lyingSlot >= 0 &&
           in.lyingSlot == in.newSlot;
}

inline LyingBackOut LyingBack(const LyingBackIn& in) {
    LyingBackOut out;
    if (!in.on)
        return out;
    const auto drop = [&out](const char* why) {
        out.verdict = LyingBackVerdict::kDrop, out.why = why;
        return out;
    };
    if (in.newLife)
        return drop("a new life: the world it lay in is gone");
    if (in.now < in.at || in.now - in.at > kLyingKeepsMs)
        return drop("the scene took it too long ago");
    if (in.newId == 0 || in.newId != in.id)
        return out;  // (another gun: the scene's gift itself, or a gun between; the record waits)
    if (in.take)
        return drop("it came back by a take, into the hand that took it");
    if (in.picked)
        return drop("a pick of yours brought it, into the right hand");
    if (in.newSlot != in.slot)
        return drop("the game gave it back into another slot");
    if (!(in.distance >= 0.0f) || in.distance > kLyingNear)
        return drop(in.distance >= 0.0f ? "where it lay is too far now" : "where it lay isn't known now");
    out.verdict = LyingBackVerdict::kLie, out.why = "the scene gave it back";
    return out;
}

// One writer thread; readers on any, never holding it up. Read is false while it writes (after the tries), and before anything was written.
template <class T> inline void SeqLock<T>::Write(const T& v) {
    const uint32_t s = seq.load(std::memory_order_relaxed);
    seq.store(s + 1, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);
    std::memcpy(static_cast<void*>(&value), &v, sizeof(T));
    std::atomic_thread_fence(std::memory_order_release);
    seq.store(s + 2, std::memory_order_release);
}

template <class T> inline bool SeqLock<T>::Read(T& out, int tries) const {
    for (int i = 0; i < tries; ++i) {
        const uint32_t before = seq.load(std::memory_order_acquire);
        if (before == 0)
            return false;
        if (before & 1)
            continue;
        std::memcpy(static_cast<void*>(&out), &value, sizeof(T));
        std::atomic_thread_fence(std::memory_order_acquire);
        if (seq.load(std::memory_order_relaxed) == before)
            return true;
    }
    return false;
}

}  // namespace pickrules

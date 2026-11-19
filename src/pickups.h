#pragma once
#include <cstdint>
// The game's own dropped guns (its pickups: an enemy's gun on the ground, the Stinger the level lays out, the player's own gun once the game has swapped
// it into one), as the game's server keeps them, and a take of one asked for by a hand (the user, 2026-11-06: "whichever hand ends up picking up the gun
// on the floor ... will be the first hand that uses it as a trigger grip"). The game's thread does everything of the game's: its interaction update
// (008B8940) is hooked; after it, for the player's soldier alone, its slots and its own pickup target are read, its pickups walked when something wants
// them, their countdowns kept by the policy, and a take asked for is made with the game's own take (008AB870) after the game's own checks, as the game
// makes it with R held. The render thread only reads what was published and posts requests. Every piece of the game's code it goes through is checked
// byte for byte and by its FNV-1a 64 against bf3_mem.exe first; anything not as read leaves it off for the session, said once (X held still asks the
// game's own pickup). build\pickups.txt (beside the DLL, read again whole when it changes; deleted: every default): off, level N, log N, take,
// seconds N, unseen 0|1, own N, slots 1, glidefrom 0|1, prearm 0|1, unknown 0|1, fire 0|1, half 2. bf3vr.ini [world]: pickups (2 all of it; 1 the
// game's own pickup alone, with X held; 0 off), pickup_seconds, pickup_unseen, pickup_own_seconds.
namespace pickups {

constexpr int kMost = 32;          // pickups in a census (the nearest the soldier)
constexpr int kMostContent = 4;    // content entries kept per pickup
constexpr int kMostSlots = 10;     // the soldier's slots looked at
constexpr int kName = 48;          // a blueprint's name, cut to fit
constexpr int kMostSwaps = 8;      // swaps remembered

enum Kind : uint8_t { kNoKind = 0, kDrop = 1 /* DynamicWeaponPickupEntityData: a soldier's */, kPlaced = 2 /* WeaponUnlockPickupEntityData: the level's */, kOtherKind = 3 };

struct Content {
    char name[kName] = {};      // "Weapons/AN94/AN94"; "" a null entry (that slot emptied)
    uint32_t blueprint = 0;     // (an identity: never followed off the game's thread)
    int slot = -1, altSlot = -1, linkedSlot = -1, clip = -1, spare = -1;
    float offset[3] = {}, rotation[3] = {};  // its gun data's PickupSettings: MeshRenderOffset (m, the world), MeshRenderRotation (degrees x y z)
};

struct Pickup {
    uint32_t entity = 0, data = 0;   // identities (RequestTake names the entity)
    Kind kind = kNoKind;
    bool allowed = false;            // the game's AllowPickup now (its own getter, on its thread, at the walk's last look)
    bool eligible = false;           // the game's own check of it for the player's soldier passed at its last look (the soldier's gates down, 008B6380 true): every
                                     // 0.2 s for those within 3 m while a hand may reach; false otherwise
    bool keepsNoPlace = false;       // UnspawnOnPickup or ContentIsStatic, and a slot its content names holds one of the soldier's weapons: that weapon would be gone
    bool own = false;                // it holds a gun that left the player's slots: kept by pickup_own_seconds
    float world[16] = {};            // its transform as the game keeps it: rows right, up, forward, translation (w)
    float secondsLeft = 0.0f, timeToLive = 0.0f, radius = 0.0f;  // +0x10C; its data's TimeToLive (0: forever) and InteractionRadius
    bool unspawnOnPickup = false, contentIsStatic = false, forceSlot = false, replaceAll = false;
    float distance = 0.0f;           // from the soldier (m)
    int contents = 0;
    int first = -1;                  // the first entry that has a blueprint (-1 none): its identity, its drawn pose, its box, what a swap takes
    Content content[kMostContent];
};

// One of the player's slots, as the game's thread last read it.
struct Slot {
    uint32_t weapon = 0, firing = 0, blueprint = 0;  // its weapon, that one's firing state, its blueprint: a new gun is told by all three
    char name[kName] = {};
    int stateA4 = -1, stateA8 = -1, stateAC = -1;    // its firing state's words (which one says "in hand" is the probe's to settle)
    int loaded = -1, spare = -1;                     // as the game counts
};

// A change of the player's slots seen on the game's thread: a hand's take (byHand), or the game's own (R held: flag 4 went up inside its update).
struct Swap {
    uint32_t serial = 0;             // 1, 2, ... (0 none)
    bool byHand = false;
    uint32_t request = 0;            // byHand: the request it answers
    int side = -1;                   // byHand: the hand that asked (-1 none: pickups.txt "take")
    uint32_t entity = 0;             // the pickup (the game's own: its target, read before its update and again after)
    bool removed = false;            // the pickup went with the take (UnspawnOnPickup, or nothing left in it)
    bool forceSlot = false;          // the pickup's ForceWeaponSlotSelection, read before the take: the game selects the slot the gun went into
    int preferredSlot = -1;          // its PreferredWeaponSlot, read then (said in the log; nothing goes by it yet)
    float world[16] = {};            // where that pickup lay, read before the take
    Content taken;                   // its first entry with a blueprint, before the take: what came to the soldier (its PickupSettings: the drawn pose)
    int changed = 0;
    int slot[kMostSlots] = {};       // the slots that changed (a take that landed by its counts alone: the taken entry's slot, came and went the same)
    char came[kMostSlots][kName] = {}, went[kMostSlots][kName] = {};        // what each one holds now, and what it held ("" nothing)
    uint32_t cameBlueprint[kMostSlots] = {}, wentBlueprint[kMostSlots] = {};  // the same as blueprints (the same kind: equal)
    uint32_t cameFiring[kMostSlots] = {}, wentFiring[kMostSlots] = {};        // their firing states (ammo forgets every one of them)
    unsigned long long at = 0;       // GetTickCount64 on the game's thread
};

// What the game's thread publishes every tick of the player's soldier (small).
struct Tick {
    uint32_t serial = 0;             // the ticks published (0: none yet)
    unsigned long long at = 0;       // GetTickCount64 on the game's thread
    unsigned long thread = 0;        // the game's thread
    uint32_t soldier = 0;            // the player's server soldier
    float soldierAt[3] = {};
    Slot slots[kMostSlots];
    int activeSlot = -1;             // pickrules::ActiveSlot of the slots' +0xA8 words (-1: none, or more than one)
    uint32_t gameTarget = 0, gameTargetBlueprint = 0;  // the pickup the game's own R would take now (its component's target, flag 4 clear); 0 none
    char gameTargetName[kName] = {};
    bool gatesUp = false;            // the game allows no pickup just now (the soldier's +0xA bit clear, or one of its three gates up)
    uint32_t served = 0;             // the last request served
    int servedResult = 0;            // 1 taken (the swap answering it is among the swaps), -1 not taken
    char servedWhy[64] = {};
    uint32_t newestSwap = 0, census = 0;  // the newest swap's serial; the newest census's
    float micros = 0.0f;             // the hook's own time this tick, after the game's update returned (the most in the last second)
    // A scripted use point the game targets now (its component's target is a ServerInteractionEntity): UsePoint's numbers; 0 none.
    uint32_t useEntity = 0;
    int useAction = -1, useKind = 0;
    float useHold = 0.0f, useRadius = 0.0f;
    uint32_t useMaxUses = 0, useUses = 0;
    bool useEnabled = false, useHaveWorld = false;
    float useWorld[16] = {};
    char useSid[kName] = {};
};

// ---- Scripted use points (the game's ServerInteractionEntity: a prompt to press a key near a point, its use the scene's own) ----
// Operation Swordbreaker's AT4 hand-over on the hotel roof is one ("hold X to pickup RPG": the reload key, R, pressed within 2 m; once): its use saves
// a checkpoint and the scene's kit puts the AT4 in slot 0. The game's own update (008B8940, hooked) targets one as it targets a dropped gun; after it,
// on the game's thread, the target is read when it is one (its vtable the use point's, 020BC9F8) and published; a hand's use asked for is made there
// with the game's own use (008A9440(entity, soldier): it enters the soldier among the point's users; the point's own update raises its OnInteraction)
// once every byte of the code it goes through is as read (bf3_mem.exe, 2026-11-18). Otherwise a hand's use is the prompt's own key (hands.cpp).
struct UsePoint {
    uint32_t entity = 0;       // the use point the game targets now (0 none)
    int action = -1;           // its InputAction (its data's +0x90): kUseReload (EIAReload) 15, kUseInteract (EIAInteract) 35 ...
    int kind = 0;              // its InteractionEntityType (+0x98): 0 a prompt; 3 a flag (its own path: no key, no hand)
    float hold = 0.0f;         // its HoldToInteractTime (+0x94, s): 0 a press
    uint32_t maxUses = 0;      // its MaxUses (+0x6C; 0 no limit) ...
    uint32_t uses = 0;         // ... and its uses so far (the entity's +0x94)
    bool enabled = false;      // the entity's enabled bit (its +0xA bit 0)
    float radius = 0.0f;       // its UseWithinRadius (+0x60, m)
    bool haveWorld = false;    // its transform as a pickup's is read (rows: right, up, forward, the point; the world)
    float world[16] = {};
    char sid[kName] = {};      // its InteractionSid's text (+0x9C): the prompt's words' id ("ID_H_INT_SP_EARTHQUAKE_EQUIP_RPG")
    unsigned long long at = 0; // when the game's thread read it
};
constexpr int kUseReload = 15, kUseInteract = 35;  // EIAReload, EIAInteract (the game's own enum, read in the image; usepoint.h's kAction*)
// The use point the game targets now, as its thread last published it (fresh: under 1 s): false for none. Render thread.
bool UseTarget(UsePoint& out);
// Whether the game's use (008A9440) and the code around it are as read, every byte, and it never faulted: a hand's use is then the game's own call.
// Render thread.
bool UseCodeReady();
// The point read (its vtable and class as read) but the use's code not, and the use never faulted: a hand's use is then the prompt's own key (hands.cpp,
// R for the reload's action). False once the game's use faulted: no hand's use of a prompt again this session, by the call or by the key (X held, the
// game's own key, stays the player's). Render thread.
bool UseByKey();
// A hand's use of prompts on: pickups at level 2 (dropped guns by hand) and build\pickups.txt not "use 0". Render thread.
bool UseByHandOn();
// A hand's use of the point it targets: served on the game's next tick if the point is still its target, a prompt a hand uses (usepoint::ByHand: the
// interact or the reload key, a press, 3 m), enabled, not used up, the soldier awake, and the game's code as read and never faulted; then the game's
// own use is called. Its serial; 0 not asked (why: none of that, or another use waits). Render thread.
uint32_t RequestUse(uint32_t entity, int side, const char** why = nullptr);
// What became of a use asked: 0 waiting (not taken by the game's thread yet, 1 s at most; or being served, 5 s at most), 1 used (the game's own use
// made: its uses went up), -1 not (why), as the game's thread said right after it. Never waits on a lock. Render thread.
int UseOutcome(uint32_t serial, const char** why = nullptr);

// The pickups near the soldier, as last walked (only while something wants them).
struct Census {
    uint32_t serial = 0;
    unsigned long long at = 0;
    int total = 0, count = 0;        // walked; kept (the nearest kMost)
    Pickup pickup[kMost];
};

// One a hand may take: its drawn pose where it lies (its third-person mesh's model space into the world, rows rotation | translation), its box there
// (its third-person mesh's, learned from the game per blueprint; a rifle's till then), its entity and first blueprint for RequestTake.
struct Reach {
    uint32_t entity = 0, blueprint = 0;
    char name[kName] = {};
    float drawn[12] = {};
    float boxMin[3] = {}, boxMax[3] = {};
    bool boxLearned = false;
    int slot = -1;                   // the slot its gun goes into
};

// build\pickups.txt's lines for hands.cpp, and bf3vr.ini's level.
struct Tune {
    bool byHand = true;              // level 2 (bf3vr.ini pickups=2, or pickups.txt "level 2"): dropped guns are offered to a hand. False at level 1
    bool glideFromDrawn = false;     // "glidefrom 1": an arrival starts at the pickup's drawn pose (else: turned as the hand holds it, box middle on box middle)
    bool preArm = true;              // "prearm 0": the arriving gun's first frame neither drawn where it lay nor hidden (as before this round)
    bool takeUnknown = true;         // "unknown 0": a gun free hands don't know is not offered to a hand at all (X held still takes it the game's way)
    bool testFire = false;           // "fire 1": the pickup round's trigger pulls are real (hands.cpp's RunFreeTest, behind its own two guards); else left out
    bool testSecondHalf = false;     // "half 2": the pickup round starts at its second half (30 s in): the right hand takes the dropped gun
    bool handUse = true;             // "use 0": no scripted prompt used by a hand's reach and grip (usepoint.h); X held and the game's keys as before
};

// Render thread, each frame (xr_runtime.cpp's OnPresent, right before ammo::Update): the player's soldier and the eye for the game's thread, the policy's
// numbers, build\pickups.txt, the checks once a level is up and the hook right after them, this frame's copies of the tick, the census and the
// swaps (every reader below reads those), each new swap said in the log and told to ammo (ammo::Swapped) before ammo::Update meets the new gun, a
// request nobody served dropped after 1 s, TakeOpen lowered.
void OnFrame();
// hands.cpp, each frame free hands run: a gun lies, a hand's take waits or an arrival is due. The walk runs every tick then (else once a second for the
// policy, a test or the log). Forgotten when not called for a second.
void SetReaching(bool reaching);
// Hooked and checked (the self-test's line).
bool Hooked();
// A take asked for waits on the game's thread (from RequestTake until its outcome is known, 1 s at most): ammo.cpp writes nothing to the player's
// weapons meanwhile.
bool TakeOpen();
// This frame's tick; false if none in the last second. This frame's census; false if none walked in the last second.
bool Now(Tick& out);
bool Near(Census& out);
// The ones a hand may take: allowed, eligible, with a blueprint, not keepsNoPlace, within `within` m of eyeWorld, nearest first, at most `most`.
int Reachable(const float* eyeWorld, float within, Reach* out, int most);
// A take asked by a hand (side 0 left, 1 right; -1 none): the game's own take of that pickup on its thread, within 0.5 s, if it's still there with that
// first blueprint and the game's own checks pass then. The rounds its guns carry into the pickup are the mod's counts (ammo::HandOverCount). Returns
// the request's serial; 0: not asked (why: pickups off or not hooked, the tick stale, a request already waiting, not in the census, keepsNoPlace).
uint32_t RequestTake(uint32_t entity, uint32_t blueprint, int side, const char** why = nullptr);
// What became of a request: 0 waiting, 1 taken (its swap is among the swaps), -1 not taken (why: a short line).
int Outcome(uint32_t request, const char** why = nullptr);
// The next swap after serial `after` (the game's own R's too), oldest first. False if none newer. The newest one's serial (0 none).
bool NextSwap(uint32_t after, Swap& out);
uint32_t NewestSwap();
// The drawn pose of a swap's taken gun where it lay, as Reach::drawn (the world).
void SwapDrawn(const Swap& swap, float* out12);
// The game's own pickup would take something now (input.cpp's X held with nothing in hand): 1 yes (its component's target is a pickup), 2 its target is
// a use point on the reload key (R would use it: the AT4's), enabled and not used up; 0 no, -1 not known (pickups off, not hooked, the tick stale: X
// held does as before).
int GameWouldTake();
// A squeeze that took nothing (hands.cpp's log line): the dropped gun whose place is nearest that point (the world) within `within` m, its name for the
// log, its distance, and why a hand can't take it now: "the game doesn't allow it now", "the game wouldn't let you take it from there", "it would take
// your gun's place for good", "nothing in it"; "" when pickups has nothing against it (hands.cpp then says its own reason). False: none that near in
// this frame's census, or no census.
bool WhyNot(const float* pointWorld, float within, const char** name, const char** why, float* distance);
// The player's slot holding that blueprint (its name compared by letters and digits, any case, as recoilguns.h's SameName); -1 none, or the tick stale.
int SlotOf(const char* blueprintName);
// The slot in the game's hand by its state words: -1 unless build\pickups.txt has "slots 1" (not trusted until the probe has read the words).
int ActiveSlot();
Tune Tuning();
// A blueprint's word for the logs: recoilguns.h's name for it ("AN-94"), else its last path part. Static storage (a ring of 8, so one Log call can hold several); the render thread.
const char* Nice(const char* blueprintName);

}  // namespace pickups

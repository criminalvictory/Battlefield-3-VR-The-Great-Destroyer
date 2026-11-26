// The game's own dropped guns (pickups.h). The game's thread: its interaction update (008B8940) hooked, and for the player's soldier alone, after
// the game's own update, its slots and its own pickup target read, its pickups walked when something wants them, their countdowns kept by the
// policy, a take asked by a hand made with the game's own take (008AB870) after the game's own checks, and every change of its slots told as a
// swap. The render thread: build\pickups.txt, the checks of the game's code and the hook, this frame's copies of what the game's thread published,
// the swaps said and told to ammo, the requests, the log.
// Every address is a VA in bf3_mem.exe (bf3.exe 1.6, not relocated: the game has always loaded at 0x400000), read 2026-11-06 and again 2026-11-12;
// they're all in one table (g_at), so tools/pickruntest.cpp can point them at memory and functions of its own.

#include "pickups.h"

#include <windows.h>
#ifndef BF3VR_PICKUPS_TEST
#include <MinHook.h>  // (tools/pickruntest.cpp gives its own)
#endif

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <mutex>

#include "ammo.h"
#include "camera.h"
#include "config.h"
#include "god.h"
#include "hands.h"
#include "log.h"
#include "loose.h"
#include "pickparts.h"
#include "pickrules.h"
#include "recoilguns.h"
#include "usepoint.h"

namespace pickups {
namespace {

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// The game's.

struct Addresses {
    uintptr_t serverWorld = 0x0236B60C, serverWorldVt = 0x020D60A8, clientWorld = 0x023813A0, clientWorldVt = 0x0213C578;
    uintptr_t pickupVt = 0x020BDAD0, clientPickupVt = 0x020B8DF0, componentVt = 0x020BEAC8, soldierVt = 0x020E1268, firingVt = 0x021AD810;
    uintptr_t dropDataVt = 0x021B0C20, placedDataVt = 0x021B0CE4, pickupType = 0x023D97D0, clientPickupType = 0x023D93AC;
    uintptr_t update = 0x008B8940, take = 0x008AB870, allow = 0x0089B480, check = 0x008B6380;
    uintptr_t codeFrom = 0, codeTo = 0, rdataFrom = 0, rdataTo = 0;  // the image's code and its read-only data, from its section headers (Verify)
    uintptr_t useVt = 0x020BC9F8, use = 0x008A9440;  // a scripted use point's (ServerInteractionEntity's) vtable; the game's use of one (2026-11-18)
};
Addresses g_at;  // the image's (the game has always loaded at 0x400000; footsteps.cpp and ground.cpp use VAs likewise)
constexpr uintptr_t kImageBase = 0x00400000;

// Layouts, as read in the code (the take reads a content entry so at 008AB9D7-008ABBB4; a slot's name so at 00A6D7DB).
constexpr uint32_t kWorldLists = 0x820, kWorldListsEnd = 0x824, kWorldFirstClass = 0x834;  // EntityWorld: {first segment, creator} by classId - first
constexpr uint32_t kSegmentNext = 0x18, kSegmentActive = 0x1C;                              // a segment: +0 begin, +4 end; [0, active) live
constexpr uint32_t kClassIdAt = 0x14;                                                       // a type's class id (u16)
constexpr uint32_t kEntityData = 0x14, kEntityTransform = 0x1C, kEntityFlags = 8;           // bit 0x40000: the indexed transform
constexpr uint32_t kContentBegin = 0x9C, kContentEnd = 0xA0, kSecondsLeft = 0x10C;          // a ServerPickupEntity's
constexpr uint32_t kClientContentBegin = 0xAC, kClientContentEnd = 0xB0, kClientMesh = 0x110;
constexpr uint32_t kPreferredSlot = 0x84, kTimeToLive = 0x88, kRadius = 0x90, kUnspawnOnPickup = 0x98, kForceSlot = 0x9A, kContentIsStatic = 0x9F,
                   kReplaceAll = 0xA2;                                                      // its data's
constexpr uint32_t kEntrySize = 0xC0, kEntrySlot = 0xA8, kEntryAlt = 0xAC, kEntryLinked = 0xB0, kEntryClip = 0xB4, kEntrySpare = 0xB8;
constexpr uint32_t kBlueprintName = 8, kBlueprintData = 0x20, kPickupOffset = 0xF0, kPickupRotation = 0x100;
constexpr uint32_t kSoldierWeapons = 0x4EC, kWeaponSlots = 0x40, kWeaponData = 0x14, kWeaponBlueprint = 0x13C, kWeaponFiring = 0x128;
constexpr uint32_t kFiringLoaded = 0xF4, kFiringSpare = 0xF8, kFiringWords = 0xA4;         // +0xA4, +0xA8, +0xAC
constexpr uint32_t kComponentSoldier = 0x10, kComponentTarget = 0x1C, kComponentFlags = 0x3C;
constexpr uint8_t kFlagTaken = 4;                                                           // a pickup taken with this press (008B9173 sets it)
constexpr uint32_t kSoldierAwake = 0xA;                                                     // its bit 1: the update does anything (008B895C)
constexpr uint32_t kMeshSetMin = 0x10, kMeshSetMax = 0x20, kMeshSetName = 0x58;

// Every piece of the game's code it goes through, as bf3_mem.exe has it: the FNV-1a 64 of its bytes.
struct Code {
    uintptr_t at;
    uint32_t size;
    uint64_t fnv;
    const char* what;
};
constexpr Code kCode[] = {
    {0x008B8940, 2368, 0xFB955A4BA0EED708ull, "its interaction update"},
    {0x008B9280, 95, 0xEEF3B42F2A4BB0D1ull, "the interaction update's caller"},
    {0x008AB870, 1716, 0x8EF7C3BC33821F96ull, "its take"},
    {0x008B6380, 549, 0x7DFD1688EA8E274Eull, "its check of a pickup for a soldier"},
    {0x008992F0, 309, 0xA0482E73683948B3ull, "its same-gun check"},
    {0x0089B480, 58, 0xC1F48076C2C83412ull, "AllowPickup"},
    {0x008A1DD0, 109, 0xFE4486D077E38B64ull, "the pickup's countdown"},
    {0x00894E00, 99, 0xCD6D569E244C0860ull, "the pickup's rest pass"},
    {0x00549A90, 112, 0xE7012094BB48EBE7ull, "the world's lists"},
    {0x00549A40, 78, 0x980991FE1C2A4737ull, "the world's lists' next"},
    {0x0084A7C0, 1150, 0x2872C71DD00A31F2ull, "the client's pose of a dropped gun"},
    {0x0084CC50, 93, 0x6951EB4951803959ull, "the client's pose's caller"},
};
// Byte for byte as well: where it hooks (MinHook's jump goes over the first 6 bytes), the call of it, flag 4 tested and set and the game's take
// called, the finder's call of the check, the update's test of the soldier, the finder's three gates on the soldier.
constexpr uint8_t kUpdateStart[] = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0, 0x81, 0xEC, 0x84, 0x00, 0x00, 0x00, 0x53, 0x56, 0x8B, 0xF1};
constexpr uint8_t kUpdateCall[] = {0x53, 0x8B, 0xCE, 0xE8, 0x9D, 0xF6, 0xFF, 0xFF};
constexpr uint8_t kFlagTested[] = {0xB3, 0x04, 0x84, 0x5E, 0x3C, 0x0F, 0x85, 0x26, 0x01, 0x00, 0x00};
constexpr uint8_t kTakeCall[] = {0x8B, 0x4E, 0x10, 0x08, 0x5E, 0x3C, 0x51, 0x8B, 0xCF, 0xE8, 0xF2, 0x26, 0xFF, 0xFF};
constexpr uint8_t kCheckCall[] = {0x56, 0x8B, 0xCF, 0xE8, 0x56, 0xDB, 0xFF, 0xFF};
constexpr uint8_t kAwakeTest[] = {0xF6, 0x40, 0x0A, 0x01};
constexpr uint8_t kGates[] = {0x8B, 0xF9, 0x8B, 0x47, 0x10, 0x8D, 0x88, 0x04, 0x01, 0x00, 0x00, 0x8B, 0x01, 0x8B, 0x50, 0x10, 0xFF, 0xD2, 0x84,
                              0xC0, 0x0F, 0x85, 0x0C, 0x01, 0x00, 0x00, 0x8B, 0x47, 0x10, 0x8D, 0x88, 0x20, 0x01, 0x00, 0x00, 0x8B, 0x01, 0x8B,
                              0x50, 0x0C, 0xFF, 0xD2, 0x84, 0xC0, 0x0F, 0x85, 0xF4, 0x00, 0x00, 0x00, 0x8B, 0x47, 0x10, 0x8D, 0x88, 0x20, 0x01,
                              0x00, 0x00, 0x8B, 0x01, 0x8B, 0x50, 0x10, 0xFF, 0xD2, 0x84, 0xC0, 0x0F, 0x85, 0xDC, 0x00, 0x00, 0x00};
static_assert(sizeof(kGates) == 74);
struct Bytes {
    uintptr_t at;
    const uint8_t* bytes;
    uint32_t size;
    const char* what;
};
constexpr Bytes kBytes[] = {
    {0x008B8940, kUpdateStart, sizeof(kUpdateStart), "its interaction update's start"},
    {0x008B929B, kUpdateCall, sizeof(kUpdateCall), "the call of its interaction update"},
    {0x008B9146, kFlagTested, sizeof(kFlagTested), "its test of a pickup taken with this press"},
    {0x008B9170, kTakeCall, sizeof(kTakeCall), "its call of the take"},
    {0x008B8822, kCheckCall, sizeof(kCheckCall), "the finder's call of its check of a pickup"},
    {0x008B895C, kAwakeTest, sizeof(kAwakeTest), "the update's test of the soldier"},
    {0x008B85E3, kGates, sizeof(kGates), "the finder's three gates on the soldier"},
};
// The data it goes by: four vtable slots, the three class ids, four floats (their bits).
struct Word {
    uintptr_t at;
    uint32_t value;
    uint32_t size;  // 2 or 4
    const char* what;
};
constexpr Word kWords[] = {
    {0x020BEAC8 + 0xA4, 0x008B9280, 4, "the interaction component's slot 41"},
    {0x020BDAD0 + 0x84, 0x008A1DD0, 4, "the pickup's slot 33 (its update)"},
    {0x020BDAD0 + 0x24, 0x01191130, 4, "the pickup's slot 9 (its transform)"},
    {0x020BDAD0 + 0x54, 0x00891510, 4, "the pickup's slot 21 (its setTransform)"},
    {0x023D97D0 + 0x14, 0x02EA, 2, "the server pickup's class id"},
    {0x023D97D0 + 0x16, 0x02EA, 2, "the server pickup's last class id"},
    {0x023D93AC + 0x14, 0x029C, 2, "the client pickup's class id"},
    {0x020BDDE0, 0x3ECCCCCD, 4, "the hold (0.4 s)"},
    {0x02075424, 0x3E4CCCCD, 4, "the rescan (0.2 s)"},
    {0x0207E6E0, 0x40A00000, 4, "the query sphere (5 m)"},
    {0x0207E940, 0x3F8CCCCD, 4, "the bounds' scale (1.1)"},
};
struct Checks {
    const Code* code;
    int codes;
    const Bytes* bytes;
    int byteRows;
    const Word* words;
    int wordRows;
};
Checks g_checks = {kCode, static_cast<int>(std::size(kCode)), kBytes, static_cast<int>(std::size(kBytes)), kWords, static_cast<int>(std::size(kWords))};

// ---- Scripted use points (pickups.h's UsePoint), read in bf3_mem.exe 2026-11-18 (docs/research/at4.md has the reading) ----
// The update's branch for a ServerInteractionEntity target (its type 023D97A8 by the target's slot 0, its class 0x2EB, one class, its vtable 020BC9F8):
// its kind (data +0x98) 3 takes a ray test and no key; any other: its HoldToInteractTime (data +0x94, through the property system) 0 asks 011BB370
// (its InputAction down now with its level zero last frame: a press), else 011BB350 (down); true: 008A9440(entity, soldier). That use (thiscall, ret
// 4): nothing unless the entity is enabled or its team allows the soldier, the soldier's +0x104 interface answers false at +0x20, its users list has
// room, its list of who may use it allows the soldier (008A8630), its uses are under MaxUses (data +0x6C, 0 none), its DelayBetweenUses has passed and
// the soldier isn't a user already; then the soldier is added to its users, its uses (+0x94) counted, its time (+0x98) kept, its update switched on:
// that update (008A1A90, slot 30) raises OnInteraction once the longest hold reaches HoldToInteractTime, at its next pass for 0. Once used up it
// returns at its own check. Called from inside the hooked update it runs where the game's own call does (008B90FA), on the same thread.
constexpr uint32_t kUseRadius = 0x60, kUseMaxUses = 0x6C, kUseAction = 0x90, kUseHold = 0x94, kUseKind = 0x98, kUseSid = 0x9C;  // its data's
constexpr uint32_t kUseUses = 0x94, kUseEnabledByte = 0xA;  // the entity's: its uses so far; the byte whose bit 0 is "enabled" (its flags' bit 16)
constexpr int kUseFlagKind = 3;                             // IET_CTFFlag: its own path, never a hand's
constexpr Code kUseCode[] = {
    {0x008A9440, 631, 0xCECA9804C85D0826ull, "its use of a use point"},
    {0x008A8630, 272, 0x638BADA8C36B571Bull, "the use point's list of who may use it"},
    {0x008A1A90, 830, 0xBF4CF0439EA83E8Cull, "the use point's update (slot 30)"},
    {0x008A6CF0, 64, 0x8C811FB410477102ull, "the use point's users list: add"},
    {0x011953E0, 68, 0x3930E7A150F5838Full, "an entity's update switch"},
    {0x0118D130, 85, 0x8A8A26F1D3889C46ull, "the game time"},
    {0x00A64ED0, 91, 0x20509B1DC743BD00ull, "a player event made"},
    {0x00545C90, 24, 0x2640302D80421EEEull, "an event sent from an entity"},
    {0x007E3590, 43, 0x1214EB2E7AE082FBull, "a player event let go"},
    {0x00832980, 156, 0x7F5E0058A9646484ull, "a weak pointer from an entity"},
    {0x0082FE50, 72, 0x520A42D75FE702D6ull, "a weak pointer set"},
    {0x008912E0, 10, 0x04748D5ADEA66225ull, "the use point's InputAction"},
    {0x00891390, 10, 0x69EAA6AEC920023Dull, "the use point's InteractionEntityType"},
};
// The update's own test of the target's type, and its key path through its call of the use (as the game makes it: the soldier pushed, the entity in
// ecx); the type's getter; the use's test of the entity's enabled bit.
constexpr uint8_t kUseTypeTest[] = {0x8B, 0x17, 0x8B, 0x02, 0x8B, 0xCF, 0xFF, 0xD0, 0x3D, 0xA8, 0x97, 0x3D, 0x02, 0x0F, 0x85, 0x5E, 0x02, 0x00, 0x00};
constexpr uint8_t kUseKeyPath[] = {0x8B, 0xCF, 0xE8, 0x76, 0x20, 0xFE, 0xFF, 0xD9, 0xEE, 0xD9, 0xC9, 0x8B, 0xCF, 0xDF, 0xE9, 0xDD, 0xD8, 0x9F, 0xF6, 0xC4,
                                   0x44, 0x7A, 0x18, 0xE8, 0x11, 0x82, 0xFD, 0xFF, 0x8B, 0x4D, 0x08, 0x50, 0xE8, 0x98, 0x22, 0x90, 0x00, 0x84, 0xC0, 0x74,
                                   0x23, 0x8B, 0x56, 0x10, 0x52, 0xEB, 0x16, 0xE8, 0xF9, 0x81, 0xFD, 0xFF, 0x8B, 0x4D, 0x08, 0x50, 0xE8, 0x60, 0x22, 0x90,
                                   0x00, 0x84, 0xC0, 0x74, 0x0B, 0x8B, 0x46, 0x10, 0x50, 0x8B, 0xCF, 0xE8, 0x41, 0x03, 0xFF, 0xFF};
constexpr uint8_t kUseGetter[] = {0xB8, 0xA8, 0x97, 0x3D, 0x02, 0xC3};
constexpr uint8_t kUseEnabledTest[] = {0xF6, 0x46, 0x0A, 0x01, 0x75, 0x2B};
static_assert(sizeof(kUseTypeTest) == 19 && sizeof(kUseKeyPath) == 76);
constexpr Bytes kUseBytes[] = {
    {0x008B8EB3, kUseTypeTest, sizeof(kUseTypeTest), "its interaction update's test of a use point"},
    {0x008B90B3, kUseKeyPath, sizeof(kUseKeyPath), "its interaction update's key path to the use"},
    {0x00891220, kUseGetter, sizeof(kUseGetter), "the use point's type"},
    {0x008A944B, kUseEnabledTest, sizeof(kUseEnabledTest), "the use's test of the point's enabled bit"},
};
constexpr Word kUseWords[] = {
    {0x020BC9F8 + 0x00, 0x00891220, 4, "the use point's slot 0 (its type)"},
    {0x020BC9F8 + 0x24, 0x01191130, 4, "the use point's slot 9 (its transform)"},
    {0x020BC9F8 + 0x78, 0x008A1A90, 4, "the use point's slot 30 (its update)"},
    {0x023D97A8 + 0x14, 0x02EB, 2, "the use point's class id"},
    {0x023D97A8 + 0x16, 0x02EB, 2, "the use point's last class id"},
    {0x021C6DD4, 0x01FE5550, 4, "the use point's data's slot 0 (its type)"},
};
// The words alone let the point be read and published (its key then the hand's way); the code and bytes too let the game's own use be called.
Checks g_useChecks = {kUseCode, static_cast<int>(std::size(kUseCode)), kUseBytes, static_cast<int>(std::size(kUseBytes)), kUseWords,
                      static_cast<int>(std::size(kUseWords))};
using UseFn = void(__fastcall*)(void* entity, void* edx, void* soldier);  // 008A9440: thiscall, the soldier on the stack, ret 4

using InteractionFn = void(__fastcall*)(void* component, void* edx, void* input);
using TakeFn = void(__fastcall*)(void* pickup, void* edx, void* soldier);
using AllowFn = bool(__fastcall*)(void* pickup, void* edx);
using CheckFn = bool(__fastcall*)(void* component, void* edx, void* pickup);  // 008B6380
using GateFn = bool(__fastcall*)(void* self, void* edx);                       // the soldier's three gates

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// Its own numbers.

constexpr int kMostWalked = 256;            // pickups looked at in one walk
constexpr int kMostSegments = 64;
constexpr float kEligibleWithin = 3.0f;     // m from the soldier: the game's own check made for those
constexpr float kTakeWithin = 3.5f;         // a sanity bound only
constexpr float kFileTakeWithin = 2.5f;     // pickups.txt "take"
constexpr int kNeverKept = 48;              // "never": the nearest so many kept
constexpr ULONGLONG kLookMs = 200, kPolicyMs = 1000, kRestMs = 5000, kStaleMs = 1000, kUnansweredMs = 1000, kKnownMs = 5000, kOwnMs = 10000;
constexpr int kMostFaults = 3;
// A rifle's box (its third-person mesh's frame) till the game's own is learned: generous.
constexpr float kRifleBoxMin[3] = {-0.05f, -0.16f, -0.30f}, kRifleBoxMax[3] = {0.05f, 0.14f, 0.80f};

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// Shared between the two threads.

// What the game's thread swaps out: the ring of the swaps.
struct Ring {
    uint32_t newest = 0;
    Swap swap[kMostSwaps];
};
// The eye, for "unseen" (the world: the arms' space plus its origin).
struct Eye {
    float eye[3] = {}, look[3] = {};
    unsigned long long at = 0;
};

pickrules::SeqLock<Tick> g_tickLock;
pickrules::SeqLock<Census> g_censusLock;
pickrules::SeqLock<Ring> g_ringLock;
pickrules::SeqLock<Eye> g_eyeLock;

std::atomic<bool> g_live{false}, g_dead{false}, g_takesDead{false};
std::atomic<uint32_t> g_player{0}, g_playerData{0};
std::atomic<int> g_level{2}, g_seconds{0}, g_ownSeconds{-1}, g_logLevel{0};
std::atomic<bool> g_unseen{false}, g_takeWanted{false};
std::atomic<ULONGLONG> g_reachingAt{0};
std::atomic<DWORD> g_gameThread{0};

// Faults (the game's thread notes them, the render thread says them).
std::atomic<int> g_faults{0};
std::atomic<DWORD> g_faultCode{0};
std::atomic<ULONGLONG> g_restUntil{0};
std::atomic<DWORD> g_takeFaultCode{0};
std::atomic<uint32_t> g_badGateTable{0}, g_badGateFunction{0};
// The policy's doing, for the log: raises made (pickup_seconds), and the pickups held in view at the end of their time (pickup_unseen).
std::atomic<uint32_t> g_raised{0};
std::atomic<uint32_t> g_heldEntity[8] = {};
std::atomic<uint32_t> g_heldCount{0};

// A take asked for: under g_askMutex (the render thread's lock_guard; the game's thread only try_locks it).
struct HandOver {
    int slot = -1;
    uint32_t firing = 0;
    int loaded = 0;
};
struct Ask {
    uint32_t serial = 0, entity = 0, blueprint = 0;
    int side = -1;
    unsigned long long at = 0;
    int counts = 0;
    HandOver count[kMostSlots];
    bool done = false, dropped = false;
};
std::mutex g_askMutex;
Ask g_ask;
std::atomic<uint32_t> g_askPosted{0};  // its serial, for the game's thread to pass by cheaply

// A hand's use of a scripted point asked: under g_useMutex, held only to post it, take it or give it up, never across the game's use (the game's
// thread try_locks it, copies the ask, marks it taken and lets go before it calls the game; the render thread's outcome try_locks it too).
struct UseAsk {
    uint32_t serial = 0, entity = 0;
    int side = -1;
    unsigned long long at = 0;
    bool taken = false;    // the game's thread has it (served, or being served now)
    bool dropped = false;  // the render thread gave it up (not taken within kUnansweredMs): the game's thread passes it by
};
std::mutex g_useMutex;
UseAsk g_useAsk;
std::atomic<uint32_t> g_useAskPosted{0};
constexpr ULONGLONG kUseServedMostMs = 5000;  // a use taken by the game's thread whose outcome hasn't come by then: given up (never seen so far)
// What came of the last use served (the game's thread, right after it, whatever the tick: the render thread reads them, never a stale tick): its
// serial last (release), its result (1 used, -1 not), why not (a text of its own, never freed), a fault's code.
std::atomic<uint32_t> g_useDoneSerial{0};
std::atomic<int> g_useDoneResult{0};
std::atomic<const char*> g_useDoneWhy{""};
std::atomic<DWORD> g_useFaultCode{0};
// What the use's checks found (Verify): 0 not looked at yet, -1 not as read (nothing of a use point is read: today's), 1 the point's vtable and class
// as read (it's read and published: a hand's use is its key), 2 the use's code too (a hand's use is the game's own call). g_useDead: the use faulted
// (inside its guard): no hand's use of a prompt again this session, by the call or by the key (the game's own key, X held, is the player's).
std::atomic<int> g_useState{0};
std::atomic<bool> g_useDead{false};

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// The game's thread's own (one thread at a time comes into PlayerTick).

std::atomic<bool> g_inside{false};
InteractionFn g_original = nullptr;
uint32_t g_servedSerial = 0;
int g_servedResult = 0;
char g_servedWhy[64] = {};
uint32_t g_useServedSerial = 0;  // the last use served or passed by (its outcome: g_useDone*)
uint32_t g_lastSoldier = 0, g_lastWorld = 0;
uint32_t g_swapSerial = 0, g_censusSerial = 0;
ULONGLONG g_walkAt = 0, g_lookGatesAt = 0, g_policyAt = 0, g_microsFrom = 0;
bool g_gatesUp = true;
float g_microsMost = 0.0f;
Tick g_tickScratch;
Census g_censusScratch;
Ring g_ringScratch;
Pickup g_walked[kMostWalked];
int g_walkedCount = 0, g_walkedTotal = 0;
bool g_walkedThisTick = false;
LARGE_INTEGER g_qpcPerSecond{};

// Names cached by blueprint (static asset data: a name is never read from a weapon object that a take may have freed).
struct NameOf {
    uint32_t blueprint = 0;
    char name[kName] = {};
};
NameOf g_names[64];
int g_nameNext = 0;

// Each pickup as last looked at: the game's checks of it (at most every 0.2 s), its countdown's rate, the policy's raise.
struct Known {
    uint32_t entity = 0, first = 0;
    ULONGLONG seenAt = 0, lookedAt = 0, leftAt = 0;
    float lastLeft = 0.0f, rate = 1.0f;
    bool allowed = false, eligible = false, raisedOnce = false, heldSaid = false;
};
Known g_known[kMostWalked];

// The pickups a gun of the player's slots went into (pickup_own_seconds).
struct Own {
    uint32_t entity = 0;
    ULONGLONG seenAt = 0;
};
Own g_own[16];

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// Reads and writes, guarded.

bool Read32(uintptr_t at, uint32_t& out) {
    return at && camera::SafeRead(at, &out, sizeof(out));
}
bool ReadInt(uintptr_t at, int& out) {
    return at && camera::SafeRead(at, &out, sizeof(out));
}
bool Read16(uintptr_t at, uint16_t& out) {
    return at && camera::SafeRead(at, &out, sizeof(out));
}
bool Read8(uintptr_t at, uint8_t& out) {
    return at && camera::SafeRead(at, &out, sizeof(out));
}
bool ReadFloats(uintptr_t at, float* out, int n) {
    return at && camera::SafeRead(at, out, sizeof(float) * n);
}
bool Flag(uintptr_t at) {
    uint8_t v = 0;
    return Read8(at, v) && v != 0;
}
// A write the game's memory may refuse: false then, nothing changed.
bool Poke(uintptr_t at, const void* bytes, size_t size) {
    if (!at)
        return false;
    __try {
        std::memcpy(reinterpret_cast<void*>(at), bytes, size);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool Finite(const float* v, int n) {
    for (int i = 0; i < n; ++i)
        if (!std::isfinite(v[i]))
            return false;
    return true;
}

// How long ago `at` was by `now` (0 when it's later: the two threads read the clock at their own moments).
ULONGLONG Age(ULONGLONG now, ULONGLONG at) {
    return now > at ? now - at : 0;
}

float Distance(const float* a, const float* b) {
    const float d[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    return std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
}

uint64_t Fnv(const uint8_t* bytes, size_t size) {
    uint64_t h = 14695981039346656037ull;
    for (size_t i = 0; i < size; ++i) {
        h ^= bytes[i];
        h *= 1099511628211ull;
    }
    return h;
}

bool SameHash(const Code& c) {
    uint8_t now[4096];
    return c.size <= sizeof(now) && camera::SafeRead(c.at, now, c.size) && Fnv(now, c.size) == c.fnv;
}

bool SameBytes(uintptr_t at, const uint8_t* bytes, size_t size) {
    uint8_t now[128];
    return size <= sizeof(now) && camera::SafeRead(at, now, size) && std::memcmp(now, bytes, size) == 0;
}

// A text whose pointer is at `at`, cut to kName - 1 letters ("" when there's none).
void ReadText(uintptr_t at, char* out) {
    out[0] = 0;
    uint32_t text = 0;
    if (!Read32(at, text) || !text)
        return;
    char buffer[kName] = {};
    if (!camera::SafeRead(text, buffer, kName - 1))
        for (int i = 0; i < kName - 1; ++i)
            if (!camera::SafeRead(text + i, &buffer[i], 1)) {
                buffer[i] = 0;
                break;
            }
    buffer[kName - 1] = 0;
    for (int i = 0; buffer[i]; ++i)
        if (static_cast<unsigned char>(buffer[i]) < 0x20 || static_cast<unsigned char>(buffer[i]) > 0x7E)
            buffer[i] = '?';
    std::memcpy(out, buffer, kName);
}

// An entity's transform as the game keeps it (rows right, up, forward, translation): its +0x1C, or with bit 0x40000 of +8 the indexed one.
bool ReadTransform(uint32_t entity, float* out16) {
    uint32_t at = 0, flags = 0;
    if (!Read32(entity + kEntityTransform, at) || !at || !Read32(entity + kEntityFlags, flags))
        return false;
    if (flags & 0x40000) {
        uint8_t index = 0;
        if (!Read8(at + 6, index))
            return false;
        at = at + 0x10 + 0x50 * index;
    }
    float m[16];
    if (!ReadFloats(at, m, 16) || !Finite(m, 16))
        return false;
    std::memcpy(out16, m, sizeof(m));
    return true;
}

// The game's thread: a blueprint's name from the cache.
const char* NameOfBlueprint(uint32_t blueprint) {
    if (!blueprint)
        return "";
    for (const NameOf& n : g_names)
        if (n.blueprint == blueprint)
            return n.name;
    NameOf& n = g_names[g_nameNext];
    g_nameNext = (g_nameNext + 1) % static_cast<int>(std::size(g_names));
    n.blueprint = blueprint;
    ReadText(blueprint + kBlueprintName, n.name);
    return n.name;
}

void CopyName(char* out, const char* name) {
    strncpy_s(out, kName, name ? name : "", _TRUNCATE);
}

// A blueprint's gun data's PickupSettings: how the client turns and moves it where it lies.
void ReadSettings(uint32_t blueprint, Content& c) {
    uint32_t data = 0;
    float offset[3] = {}, rotation[3] = {};
    if (blueprint && Read32(blueprint + kBlueprintData, data) && data && ReadFloats(data + kPickupOffset, offset, 3) &&
        ReadFloats(data + kPickupRotation, rotation, 3) && Finite(offset, 3) && Finite(rotation, 3)) {
        std::memcpy(c.offset, offset, sizeof(offset));
        std::memcpy(c.rotation, rotation, sizeof(rotation));
    }
}

// The content of a pickup (begin and end at those offsets of the entity): up to kMostContent entries, and the first with a blueprint.
void ReadContent(uint32_t entity, uint32_t beginAt, uint32_t endAt, Pickup& p) {
    p.contents = 0;
    p.first = -1;
    uint32_t begin = 0, end = 0;
    if (!Read32(entity + beginAt, begin) || !Read32(entity + endAt, end) || !begin || end < begin || end - begin > kEntrySize * 64)
        return;
    const int all = static_cast<int>((end - begin) / kEntrySize);
    p.contents = std::min(all, kMostContent);
    p.contentsAll = all;
    for (int k = 0; k < p.contents; ++k) {
        Content& c = p.content[k];
        c = Content{};
        const uint32_t q = begin + kEntrySize * k;
        Read32(q, c.blueprint);
        ReadInt(q + kEntrySlot, c.slot);
        ReadInt(q + kEntryAlt, c.altSlot);
        ReadInt(q + kEntryLinked, c.linkedSlot);
        ReadInt(q + kEntryClip, c.clip);
        ReadInt(q + kEntrySpare, c.spare);
        if (c.blueprint) {
            CopyName(c.name, NameOfBlueprint(c.blueprint));
            ReadSettings(c.blueprint, c);
            if (p.first < 0)
                p.first = k;
        }
    }
}

// The component's target: the entity its weak pointer holds (its token's +0 is the entity's +4), whatever it is; 0 none.
uint32_t TargetHeld(uint32_t comp) {
    uint32_t weak = 0, token = 0;
    if (!Read32(comp + kComponentTarget, weak) || !weak || !Read32(weak, token) || !token)
        return 0;
    return token - 4;
}
// ... when it's a pickup.
uint32_t TargetPickup(uint32_t comp) {
    const uint32_t entity = TargetHeld(comp);
    uint32_t vtable = 0;
    return entity && Read32(entity, vtable) && vtable == g_at.pickupVt ? entity : 0;
}

// The player's slots as the game has them now.
void ReadSlots(uint32_t soldier, Slot* slots, pickrules::Triple* triples, bool& list) {
    for (int s = 0; s < kMostSlots; ++s) {
        slots[s] = Slot{};
        triples[s] = pickrules::Triple{};
    }
    uint32_t weapons = 0;
    list = Read32(soldier + kSoldierWeapons, weapons) && weapons;
    if (!list)
        return;
    for (int s = 0; s < kMostSlots; ++s) {
        uint32_t weapon = 0, firing = 0, data = 0, blueprint = 0, vtable = 0;
        if (!Read32(weapons + kWeaponSlots + 4 * s, weapon) || !weapon)
            continue;
        Slot& slot = slots[s];
        slot.weapon = weapon;
        if (Read32(weapon + kWeaponFiring, firing) && firing) {
            slot.firing = firing;
            if (Read32(firing, vtable) && vtable == g_at.firingVt) {
                ReadInt(firing + kFiringLoaded, slot.loaded);
                ReadInt(firing + kFiringSpare, slot.spare);
                ReadInt(firing + kFiringWords, slot.stateA4);
                ReadInt(firing + kFiringWords + 4, slot.stateA8);
                ReadInt(firing + kFiringWords + 8, slot.stateAC);
            }
        }
        if (Read32(weapon + kWeaponData, data) && data && Read32(data + kWeaponBlueprint, blueprint) && blueprint) {
            slot.blueprint = blueprint;
            CopyName(slot.name, NameOfBlueprint(blueprint));
        }
        triples[s] = {slot.weapon, slot.firing, slot.blueprint};
    }
}

// The soldier's place: its transform's translation.
bool SoldierAt(uint32_t soldier, float* out3) {
    float m[16];
    if (!ReadTransform(soldier, m))
        return false;
    out3[0] = m[12], out3[1] = m[13], out3[2] = m[14];
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// The game's own functions, each guarded: 1 true, 0 false, -1 a fault (its code).

int AllowGuarded(uint32_t pickup, DWORD& fault) {
    DWORD code = 0;
    __try {
        return reinterpret_cast<AllowFn>(g_at.allow)(reinterpret_cast<void*>(pickup), nullptr) ? 1 : 0;
    } __except (code = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER) {
        fault = code;
        return -1;
    }
}

int CheckGuarded(uint32_t comp, uint32_t pickup, DWORD& fault) {
    DWORD code = 0;
    __try {
        return reinterpret_cast<CheckFn>(g_at.check)(reinterpret_cast<void*>(comp), nullptr, reinterpret_cast<void*>(pickup)) ? 1 : 0;
    } __except (code = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER) {
        fault = code;
        return -1;
    }
}

int GateGuarded(uintptr_t function, uint32_t self, DWORD& fault) {
    DWORD code = 0;
    __try {
        return reinterpret_cast<GateFn>(function)(reinterpret_cast<void*>(self), nullptr) ? 1 : 0;
    } __except (code = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER) {
        fault = code;
        return -1;
    }
}

bool TakeGuarded(uint32_t pickup, uint32_t soldier, DWORD& fault) {
    DWORD code = 0;
    __try {
        reinterpret_cast<TakeFn>(g_at.take)(reinterpret_cast<void*>(pickup), nullptr, reinterpret_cast<void*>(soldier));
        return true;
    } __except (code = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER) {
        fault = code;
        return false;
    }
}

// The game's use of a scripted point, guarded as its take is.
bool UseGuarded(uint32_t entity, uint32_t soldier, DWORD& fault) {
    DWORD code = 0;
    __try {
        reinterpret_cast<UseFn>(g_at.use)(reinterpret_cast<void*>(entity), nullptr, reinterpret_cast<void*>(soldier));
        return true;
    } __except (code = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER) {
        fault = code;
        return false;
    }
}

// The component's target, when it's a scripted use point (its vtable the use point's, as the update's own type test passes it): its numbers into the
// tick (useEntity 0: none). Only with its vtable and class as read (g_useState 1 or 2).
void ReadUse(uint32_t comp, Tick& t) {
    t.useEntity = 0, t.useAction = -1, t.useKind = 0, t.useHold = 0.0f, t.useRadius = 0.0f, t.useMaxUses = 0, t.useUses = 0;
    t.useEnabled = t.useHaveWorld = false, t.useSid[0] = 0;
    if (g_useState.load(std::memory_order_relaxed) < 1)
        return;
    const uint32_t entity = TargetHeld(comp);
    uint32_t vtable = 0, data = 0;
    if (!entity || !Read32(entity, vtable) || vtable != g_at.useVt || !Read32(entity + kEntityData, data) || !data)
        return;
    int action = -1, kind = 0;
    float hold = 0.0f, radius = 0.0f;
    uint32_t maxUses = 0, uses = 0;
    uint8_t enabled = 0;
    if (!ReadInt(data + kUseAction, action) || !ReadInt(data + kUseKind, kind) || !ReadFloats(data + kUseHold, &hold, 1) ||
        !ReadFloats(data + kUseRadius, &radius, 1) || !Read32(data + kUseMaxUses, maxUses) || !Read32(entity + kUseUses, uses) ||
        !Read8(entity + kUseEnabledByte, enabled))
        return;
    t.useEntity = entity, t.useAction = action, t.useKind = kind, t.useHold = std::isfinite(hold) ? hold : -1.0f;
    t.useRadius = std::isfinite(radius) ? radius : 0.0f, t.useMaxUses = maxUses, t.useUses = uses, t.useEnabled = (enabled & 1) != 0;
    t.useHaveWorld = ReadTransform(entity, t.useWorld);
    ReadText(data + kUseSid, t.useSid);
}

// What came of a use served (the game's thread): for the render thread at once (UseOutcome), its serial last.
void UseServed(uint32_t serial, int result, const char* why, DWORD fault = 0) {
    g_useServedSerial = serial;
    g_useFaultCode.store(fault, std::memory_order_relaxed);
    g_useDoneWhy.store(why ? why : "", std::memory_order_relaxed);
    g_useDoneResult.store(result, std::memory_order_relaxed);
    g_useDoneSerial.store(serial, std::memory_order_release);
}

// A hand's use asked by the render thread (taken off the ask, its lock let go): refused in this order, each with its words; then the game's own use,
// and whether its uses went up.
void ServeUse(const UseAsk& a, uint32_t comp, uint32_t soldier, const Tick& t, ULONGLONG now) {
    const char* why = nullptr;
    uint8_t awake = 0;
    if (!pickrules::Fresh(a.at, now))
        why = "too late";
    else if (g_useState.load(std::memory_order_relaxed) < 2)
        why = "the game's use isn't as read";
    else if (g_useDead.load(std::memory_order_relaxed))
        why = "the game's use faulted earlier";
    else if (!t.useEntity || t.useEntity != a.entity || TargetHeld(comp) != a.entity)
        why = "the game doesn't target it now";
    else if (t.useKind == kUseFlagKind)
        why = "a flag: its own path";
    else if (!usepoint::ByHand(t.useAction, t.useKind, t.useHold, t.useRadius))
        why = "not a prompt a hand uses";
    else if (!t.useEnabled)
        why = "it isn't on";
    else if (t.useMaxUses && t.useUses >= t.useMaxUses)
        why = "used up";
    else if (!Read8(soldier + kSoldierAwake, awake) || !(awake & 1))  // (the game's update does nothing for it then: its target is last seen's)
        why = "the soldier isn't awake";
    if (why) {
        UseServed(a.serial, -1, why);
        return;
    }
    DWORD fault = 0;
    if (!UseGuarded(a.entity, soldier, fault)) {
        g_useDead.store(true, std::memory_order_relaxed);
        UseServed(a.serial, -1, "the game's use faulted", fault ? fault : 1);
        return;
    }
    uint32_t uses = 0;
    const bool counted = Read32(a.entity + kUseUses, uses) && uses > t.useUses;
    UseServed(a.serial, counted ? 1 : -1, counted ? "" : "the game's own checks refused it (a delay, a team, already a user)");
}

// A fault in the walk, the getter, the check or a gate: 5 s without the walk; the third this session leaves it all off.
void Faulted(DWORD code) {
    const int faults = g_faults.load(std::memory_order_relaxed) + 1;
    g_faultCode.store(code, std::memory_order_relaxed);
    if (faults >= kMostFaults)
        g_dead.store(true, std::memory_order_relaxed);
    else
        g_restUntil.store(GetTickCount64() + kRestMs, std::memory_order_relaxed);
    g_faults.store(faults, std::memory_order_release);
}

// The soldier's own reasons for no pickup at all, as the finder asks them (008B85E3): its +0xA bit clear, or one of its three gates up. Each gate's
// table must lie in the game's read-only data and its function in the game's code, or it isn't called (up, and a fault). False: none up.
bool GatesUp(uint32_t soldier) {
    uint8_t awake = 0;
    if (!Read8(soldier + kSoldierAwake, awake) || !(awake & 1))
        return true;
    struct Gate {
        uint32_t at, slot;
    };
    constexpr Gate kGatesOf[] = {{0x104, 0x10}, {0x120, 0x0C}, {0x120, 0x10}};
    for (const Gate& g : kGatesOf) {
        const uint32_t self = soldier + g.at;
        uint32_t table = 0, function = 0;
        if (!Read32(self, table) || !Read32(table + g.slot, function) || table < g_at.rdataFrom || table >= g_at.rdataTo ||
            function < g_at.codeFrom || function >= g_at.codeTo) {
            if (!g_badGateTable.load(std::memory_order_relaxed)) {
                g_badGateFunction.store(function, std::memory_order_relaxed);
                g_badGateTable.store(table ? table : 1, std::memory_order_relaxed);
            }
            Faulted(0);
            return true;
        }
        DWORD fault = 0;
        const int up = GateGuarded(function, self, fault);
        if (up < 0) {
            Faulted(fault);
            return true;
        }
        if (up)
            return true;
    }
    return false;
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// The game's thread: the tables keyed by the game's pointers.

void DropTables() {
    for (NameOf& n : g_names)
        n = NameOf{};
    for (Known& k : g_known)
        k = Known{};
    for (Own& o : g_own)
        o = Own{};
    g_nameNext = 0;
}

Known& KnownOf(uint32_t entity, ULONGLONG now) {
    Known* free = nullptr;
    Known* oldest = &g_known[0];
    for (Known& k : g_known) {
        if (k.entity == entity)
            return k;
        if (!free && (!k.entity || now - k.seenAt > kKnownMs))
            free = &k;
        if (k.seenAt < oldest->seenAt)
            oldest = &k;
    }
    Known& k = free ? *free : *oldest;
    k = Known{};
    k.entity = entity;
    return k;
}

bool IsOwn(uint32_t entity) {
    for (const Own& o : g_own)
        if (o.entity && o.entity == entity)
            return true;
    return false;
}

int OwnCount() {
    int n = 0;
    for (const Own& o : g_own)
        n += o.entity ? 1 : 0;
    return n;
}

// A swap made: a later swap of a pickup that brings a gun out lets it go; one that a slot's gun went into (and wasn't removed) is the player's own.
void NoteOwn(const Swap& s, ULONGLONG now) {
    if (!s.entity)
        return;
    bool came = false, went = false;
    for (int i = 0; i < s.changed; ++i) {
        came = came || s.came[i][0];
        went = went || s.went[i][0];
    }
    if (came)
        for (Own& o : g_own)
            if (o.entity == s.entity)
                o = Own{};
    if (s.removed || !went || IsOwn(s.entity))
        return;
    Own* slot = &g_own[0];
    for (Own& o : g_own)
        if (!o.entity || o.seenAt < slot->seenAt) {
            slot = &o;
            if (!o.entity)
                break;
        }
    slot->entity = s.entity;
    slot->seenAt = now;
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// The game's thread: the walk.

// One pickup as the game keeps it (its data, transform, countdown, content), its distance from the soldier. False: not a pickup to keep.
bool ReadPickup(uint32_t entity, const float* soldierAt, Pickup& p) {
    p = Pickup{};
    p.entity = entity;
    uint32_t data = 0, dataVt = 0;
    if (!Read32(entity + kEntityData, data) || !data || !Read32(data, dataVt) || !ReadTransform(entity, p.world))
        return false;
    p.data = data;
    p.kind = dataVt == g_at.dropDataVt ? kDrop : dataVt == g_at.placedDataVt ? kPlaced : kOtherKind;
    ReadFloats(entity + kSecondsLeft, &p.secondsLeft, 1);
    ReadFloats(data + kTimeToLive, &p.timeToLive, 1);
    ReadFloats(data + kRadius, &p.radius, 1);
    p.unspawnOnPickup = Flag(data + kUnspawnOnPickup);
    p.forceSlot = Flag(data + kForceSlot);
    p.contentIsStatic = Flag(data + kContentIsStatic);
    p.replaceAll = Flag(data + kReplaceAll);
    ReadContent(entity, kContentBegin, kContentEnd, p);
    p.distance = Distance(p.world + 12, soldierAt);
    return std::isfinite(p.distance);
}

// The server world's ServerPickupEntity segments (its class id read at run time), active entries only, at most kMostWalked, into g_walked. With
// `find`, only whether that entity is among them.
bool WalkWorld(const float* soldierAt, uint32_t find, bool& found) {
    found = false;
    g_walkedCount = g_walkedTotal = 0;
    uint32_t world = 0, vtable = 0, lists = 0, listsEnd = 0, segment = 0;
    uint16_t first = 0, cls = 0;
    if (!Read32(g_at.serverWorld, world) || !world || !Read32(world, vtable) || vtable != g_at.serverWorldVt || !Read16(world + kWorldFirstClass, first) ||
        !Read16(g_at.pickupType + kClassIdAt, cls) || cls < first || !Read32(world + kWorldLists, lists) || !Read32(world + kWorldListsEnd, listsEnd))
        return false;
    const uint32_t at = lists + 8u * (cls - first);
    if (!lists || at + 8 > listsEnd || !Read32(at, segment))
        return false;
    for (int guard = 0; segment && guard < kMostSegments; ++guard) {
        uint32_t begin = 0, end = 0, next = 0;
        int active = 0;
        if (!Read32(segment, begin) || !Read32(segment + 4, end) || !ReadInt(segment + kSegmentActive, active) || !Read32(segment + kSegmentNext, next))
            break;
        const int all = end >= begin && end - begin <= 4u * 4096 ? static_cast<int>((end - begin) / 4) : 0;
        const int live = std::min(std::max(active, 0), all);
        for (int i = 0; i < live; ++i) {
            uint32_t entity = 0, entityVt = 0;
            if (!Read32(begin + 4 * i, entity) || !entity || !Read32(entity, entityVt) || entityVt != g_at.pickupVt)
                continue;
            if (find) {
                if (entity == find) {
                    found = true;
                    return true;
                }
                continue;
            }
            if (g_walkedCount >= kMostWalked) {
                ++g_walkedTotal;
                continue;
            }
            if (ReadPickup(entity, soldierAt, g_walked[g_walkedCount])) {
                ++g_walkedCount;
                ++g_walkedTotal;
            }
        }
        segment = next;
    }
    return true;
}

bool WalkGuarded(const float* soldierAt, uint32_t find, bool& found, DWORD& fault) {
    DWORD code = 0;
    __try {
        return WalkWorld(soldierAt, find, found);
    } __except (code = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER) {
        fault = code;
        g_walkedCount = 0;
        return false;
    }
}

// Whether that pickup is still in the world's list (after a take).
bool InWorld(uint32_t entity, const float* soldierAt) {
    bool found = false;
    DWORD fault = 0;
    const int count = g_walkedCount, total = g_walkedTotal;
    const bool walked = WalkGuarded(soldierAt, entity, found, fault);
    g_walkedCount = count, g_walkedTotal = total;
    return !walked || found;  // (not known: taken as there)
}

const Pickup* Walked(uint32_t entity) {
    for (int i = 0; i < g_walkedCount; ++i)
        if (g_walked[i].entity == entity)
            return &g_walked[i];
    return nullptr;
}

// After the walk: the game's checks for those within 3 m (every 0.2 s a pickup), keepsNoPlace, own, the rates. False: a fault (the walk rests).
bool LookAtWalked(uint32_t comp, const Slot* slots, ULONGLONG now) {
    for (int i = 0; i < g_walkedCount; ++i) {
        Pickup& p = g_walked[i];
        Known& k = KnownOf(p.entity, now);
        const uint32_t first = p.first >= 0 ? p.content[p.first].blueprint : 0;
        if (k.first != first) {
            k.first = first;
            k.raisedOnce = false;
            k.heldSaid = false;
        }
        k.seenAt = now;
        // Its countdown's rate between two looks half a second apart or more; back up at its time: the game put new content in it.
        if (!k.leftAt) {
            k.leftAt = now, k.lastLeft = p.secondsLeft;
        } else if (now - k.leftAt >= 500) {
            const float seconds = static_cast<float>(now - k.leftAt) / 1000.0f;
            if (p.secondsLeft > k.lastLeft + 1.0f && p.timeToLive > 0.0f && p.secondsLeft >= p.timeToLive - 1.0f) {
                k.raisedOnce = false;
                k.heldSaid = false;
            } else if (p.secondsLeft < k.lastLeft && std::isfinite(p.secondsLeft)) {
                k.rate = (k.lastLeft - p.secondsLeft) / seconds;
            }
            k.leftAt = now, k.lastLeft = p.secondsLeft;
        }
        if (p.distance <= kEligibleWithin) {
            if (!k.lookedAt || now - k.lookedAt >= kLookMs) {
                k.lookedAt = now;
                DWORD fault = 0;
                const int allowed = AllowGuarded(p.entity, fault);
                if (allowed < 0) {
                    Faulted(fault);
                    return false;
                }
                k.allowed = allowed == 1;
                k.eligible = false;
                if (!g_gatesUp) {
                    const int eligible = CheckGuarded(comp, p.entity, fault);
                    if (eligible < 0) {
                        Faulted(fault);
                        return false;
                    }
                    k.eligible = eligible == 1;
                }
            }
            p.allowed = k.allowed;
            p.eligible = k.eligible && !g_gatesUp;
        } else {
            k.lookedAt = 0;
            p.allowed = p.eligible = false;
        }
        p.keepsNoPlace = false;
        if (p.unspawnOnPickup || p.contentIsStatic)
            for (int c = 0; c < p.contents; ++c)
                if (p.content[c].blueprint && p.content[c].slot >= 0 && p.content[c].slot < kMostSlots && slots[p.content[c].slot].weapon)
                    p.keepsNoPlace = true;
        p.own = IsOwn(p.entity);
        if (p.own)
            for (Own& o : g_own)
                if (o.entity == p.entity)
                    o.seenAt = now;
    }
    // Own pickups not in a walk for 10 s are gone (or the world isn't the one they were in).
    for (Own& o : g_own)
        if (o.entity && now - o.seenAt > kOwnMs)
            o = Own{};
    return true;
}

// The nearest kMost into the census, the player's own always among them.
void KeepNearest(ULONGLONG now) {
    int order[kMostWalked];
    for (int i = 0; i < g_walkedCount; ++i)
        order[i] = i;
    std::sort(order, order + g_walkedCount, [](int a, int b) {
        if (g_walked[a].own != g_walked[b].own)
            return g_walked[a].own;
        return g_walked[a].distance < g_walked[b].distance;
    });
    Census& c = g_censusScratch;
    c.serial = ++g_censusSerial;
    c.at = now;
    c.total = g_walkedTotal;
    c.count = std::min(g_walkedCount, kMost);
    for (int i = 0; i < c.count; ++i)
        c.pickup[i] = g_walked[order[i]];
    std::sort(c.pickup, c.pickup + c.count, [](const Pickup& a, const Pickup& b) { return a.distance < b.distance; });
}

// The policy, once a second, over every pickup of this tick's walk: +0x10C alone is written, a finite value.
void Policy(ULONGLONG now) {
    const int seconds = g_seconds.load(std::memory_order_relaxed), ownSeconds = g_ownSeconds.load(std::memory_order_relaxed);
    const bool unseen = g_unseen.load(std::memory_order_relaxed);
    if (seconds == 0 && !unseen && (ownSeconds == 0 || !OwnCount()))
        return;
    Eye eye;
    const bool haveEye = g_eyeLock.Read(eye) && eye.at && Age(now, eye.at) < kStaleMs;
    // With "never" for all, only the nearest kNeverKept are kept.
    int rank[kMostWalked];
    for (int i = 0; i < g_walkedCount; ++i) {
        rank[i] = 0;
        for (int j = 0; j < g_walkedCount; ++j)
            if (g_walked[j].distance < g_walked[i].distance)
                ++rank[i];
    }
    for (int i = 0; i < g_walkedCount; ++i) {
        const Pickup& p = g_walked[i];
        Known& k = KnownOf(p.entity, now);
        const int policy = seconds == -1 && rank[i] >= kNeverKept ? 0 : seconds;
        const bool inView = haveEye && pickrules::InView(eye.eye, eye.look, p.world + 12);
        float out = 0.0f;
        const bool wasRaised = k.raisedOnce;
        if (!pickrules::CountdownFor(policy, p.own, ownSeconds, unseen, p.timeToLive, p.secondsLeft, k.rate, inView, k.raisedOnce, out) ||
            !std::isfinite(out))
            continue;
        if (!Poke(p.entity + kSecondsLeft, &out, sizeof(out)))
            continue;
        k.lastLeft = out, k.leftAt = now;  // (its own write isn't the game's countdown moving)
        if (!wasRaised && k.raisedOnce && !p.own)
            g_raised.fetch_add(1, std::memory_order_relaxed);
        const float hold = std::fmax(2.0f, 3.0f * std::fmin(std::fmax(k.rate, 1.0f), 60.0f));
        if (unseen && inView && out == hold && !k.heldSaid) {
            k.heldSaid = true;
            const uint32_t n = g_heldCount.load(std::memory_order_relaxed);
            g_heldEntity[n % std::size(g_heldEntity)].store(p.entity, std::memory_order_relaxed);
            g_heldCount.store(n + 1, std::memory_order_release);
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// The game's thread: the swaps.

// What came and went, slot by slot: the slots Diff gives, or (it landed by its counts alone) the taken entry's slot, came and went the same.
void FillSlots(Swap& s, const Slot* before, const Slot* after, const pickrules::Triple* tBefore, const pickrules::Triple* tAfter, int takenSlot) {
    int changed[kMostSlots];
    int n = pickrules::Diff(tBefore, tAfter, kMostSlots, changed);
    if (n == 0 && takenSlot >= 0 && takenSlot < kMostSlots) {
        changed[0] = takenSlot;
        n = 1;
    }
    s.changed = n;
    for (int i = 0; i < n; ++i) {
        const int slot = changed[i];
        s.slot[i] = slot;
        CopyName(s.came[i], after[slot].name);
        CopyName(s.went[i], before[slot].name);
        s.cameBlueprint[i] = after[slot].blueprint;
        s.wentBlueprint[i] = before[slot].blueprint;
        s.cameFiring[i] = after[slot].firing;
        s.wentFiring[i] = before[slot].firing;
    }
}

void AddSwap(const Swap& s, ULONGLONG now) {
    Ring& r = g_ringScratch;
    r.swap[s.serial % kMostSwaps] = s;
    r.newest = s.serial;
    NoteOwn(s, now);
    g_ringLock.Write(r);
}

// A content entry made from a slot's gun (its blueprint's name and PickupSettings: static data).
void ContentOfSlot(const Slot& slot, int index, Content& c) {
    c = Content{};
    c.blueprint = slot.blueprint;
    c.slot = index;
    CopyName(c.name, slot.name);
    ReadSettings(slot.blueprint, c);
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// The game's thread: one tick of the player's soldier.

// What must be read before the game's own update can change it.
struct Before {
    uint8_t flags = 0;
    bool list = false;
    Slot slots[kMostSlots];
    pickrules::Triple triple[kMostSlots];
    uint32_t target = 0;
    bool haveWorld = false, haveTaken = false, forceSlot = false;
    bool keepsNoPlace = false, replaceAll = false;  // the target's UnspawnOnPickup or ContentIsStatic, its ReplaceAllContent (a swap's, for holsters)
    float world[16] = {};
    Content taken;
    int preferredSlot = -1;
};

void ReadBefore(uint32_t comp, uint32_t soldier, Before& b) {
    Read8(comp + kComponentFlags, b.flags);
    ReadSlots(soldier, b.slots, b.triple, b.list);
    b.target = TargetPickup(comp);
    if (!b.target)
        return;
    b.haveWorld = ReadTransform(b.target, b.world);
    Pickup p;
    p.entity = b.target;
    ReadContent(b.target, kContentBegin, kContentEnd, p);
    if (p.first >= 0) {
        b.taken = p.content[p.first];
        b.haveTaken = true;
    }
    uint32_t data = 0;
    if (Read32(b.target + kEntityData, data) && data) {
        b.forceSlot = Flag(data + kForceSlot);
        ReadInt(data + kPreferredSlot, b.preferredSlot);
        b.keepsNoPlace = Flag(data + kUnspawnOnPickup) || Flag(data + kContentIsStatic);
        b.replaceAll = Flag(data + kReplaceAll);
    }
}

// One thread at a time in PlayerTick, so what it holds is its own.
struct Inside {
    const bool in = !g_inside.exchange(true, std::memory_order_acquire);
    ~Inside() {
        if (in)
            g_inside.store(false, std::memory_order_release);
    }
};

void Serve(uint32_t serial, int result, const char* why) {
    g_servedSerial = serial;
    g_servedResult = result;
    strncpy_s(g_servedWhy, why ? why : "", _TRUNCATE);
}

// A take asked by the render thread: refused in this order, each with its words; then the counts handed over, flag 4 set and the game's take.
// mutex held (try_lock): the render thread can't drop it meanwhile.
void ServeAsk(const Ask& a, uint32_t comp, uint32_t soldier, const float* soldierAt, Tick& t, const pickrules::Triple* triples, uint8_t flagsBefore,
              uint8_t flagsAfter, bool ownTake, ULONGLONG now) {
    const char* why = nullptr;
    const Pickup* p = g_walkedThisTick ? Walked(a.entity) : nullptr;
    uint32_t weapons = 0;
    if (!pickrules::Fresh(a.at, now))
        why = "too late";
    else if (!p || (p->first >= 0 && p->content[p->first].blueprint != a.blueprint))
        why = "not there any more";
    else if (!p->allowed)
        why = "the game doesn't allow it now";
    else if (p->first < 0)
        why = "nothing in it";
    else if (p->keepsNoPlace)
        why = "it would take your gun's place for good";
    else if (p->distance > kTakeWithin)
        why = "out of reach";
    else if (!Read32(soldier + kSoldierWeapons, weapons) || !weapons)
        why = "the soldier has no weapons";
    else if ((flagsBefore & kFlagTaken) || (flagsAfter & kFlagTaken) || ownTake)
        why = "the game's own pickup was under way";
    else if (g_takesDead.load(std::memory_order_relaxed))
        why = "the game's take faulted earlier";
    if (!why) {
        g_gatesUp = GatesUp(soldier);
        g_lookGatesAt = now;
        if (g_gatesUp)
            why = "the game allows no pickup just now";
    }
    if (!why) {
        DWORD fault = 0;
        const int ok = CheckGuarded(comp, a.entity, fault);
        if (ok < 0) {
            Faulted(fault);
            why = "the game's check faulted";
        } else if (!ok) {
            why = "it wouldn't let you take it from there now";
        }
    }
    if (why) {
        Serve(a.serial, -1, why);
        return;
    }
    // The counts the mod keeps, into the guns that go into the pickup, right before the take (the old ones kept, to put back if it swaps nothing).
    struct Written {
        uint32_t firing;
        int old;
        int slot, wrote;
    };
    Written written[kMostSlots];
    int writes = 0;
    for (int i = 0; i < a.counts && i < kMostSlots; ++i) {
        const HandOver& h = a.count[i];
        uint32_t vtable = 0;
        int old = 0;
        if (h.slot < 0 || h.slot >= kMostSlots || !triples[h.slot].weapon || triples[h.slot].firing != h.firing || !h.firing ||
            !Read32(h.firing, vtable) || vtable != g_at.firingVt || h.loaded < 0 || h.loaded >= 10000 || !ReadInt(h.firing + kFiringLoaded, old))
            continue;
        if (Poke(h.firing + kFiringLoaded, &h.loaded, sizeof(int)))
            written[writes++] = {h.firing, old, h.slot, h.loaded};
    }
    // What the take is about, read before it (nothing of the pickup is read after it).
    Swap s;
    s.byHand = true;
    s.request = a.serial;
    s.side = a.side;
    s.entity = a.entity;
    if (!ReadTransform(a.entity, s.world))
        std::memcpy(s.world, p->world, sizeof(s.world));
    Pickup now0;
    now0.entity = a.entity;
    ReadContent(a.entity, kContentBegin, kContentEnd, now0);
    s.taken = now0.first >= 0 ? now0.content[now0.first] : p->content[p->first];
    uint32_t data = 0;
    if (Read32(a.entity + kEntityData, data) && data) {
        s.forceSlot = Flag(data + kForceSlot);
        ReadInt(data + kPreferredSlot, s.preferredSlot);
        s.keepsNoPlace = Flag(data + kUnspawnOnPickup) || Flag(data + kContentIsStatic);  // (holsters: a gun it replaces can't stay away in it)
        s.replaceAll = Flag(data + kReplaceAll);
    }
    const int takenSlot = s.taken.slot >= 0 && s.taken.slot < kMostSlots ? s.taken.slot : -1;
    int loadedBefore = -1, spareBefore = -1;
    if (takenSlot >= 0 && t.slots[takenSlot].firing) {
        ReadInt(t.slots[takenSlot].firing + kFiringLoaded, loadedBefore);
        ReadInt(t.slots[takenSlot].firing + kFiringSpare, spareBefore);
    }
    uint8_t flags = 0;
    Read8(comp + kComponentFlags, flags);
    flags |= kFlagTaken;
    Poke(comp + kComponentFlags, &flags, 1);
    DWORD fault = 0;
    if (!TakeGuarded(a.entity, soldier, fault)) {
        g_takesDead.store(true, std::memory_order_relaxed);
        g_takeFaultCode.store(fault ? fault : 1, std::memory_order_relaxed);
        Serve(a.serial, -1, "the game's take faulted");
        return;
    }
    Slot after[kMostSlots];
    pickrules::Triple tAfter[kMostSlots];
    bool list = false;
    ReadSlots(soldier, after, tAfter, list);
    const int loadedAfter = takenSlot >= 0 ? after[takenSlot].loaded : -1, spareAfter = takenSlot >= 0 ? after[takenSlot].spare : -1;
    if (!pickrules::Landed(triples, tAfter, kMostSlots, takenSlot, loadedBefore, spareBefore, loadedAfter, spareAfter)) {
        for (int i = 0; i < writes; ++i)
            Poke(written[i].firing + kFiringLoaded, &written[i].old, sizeof(int));
        Serve(a.serial, -1, "the game swapped nothing");
        return;
    }
    // A gun the take left in the soldier's hands (its slot's triple as before: ReplaceAllContent hands over every slot held; an entry naming a slot
    // whose gun the game left there, putting the taken one in another) gets its count back as it was: the mod's, without its hidden round, read as a
    // shot. Not the taken entry's slot when nothing else changed (a take landed by its counts alone is the game's write there), nor a count the game
    // changed since the hand-over (pickrules::GivesCountBack).
    for (int i = 0; i < writes; ++i) {
        const int k = written[i].slot;
        if (pickrules::GivesCountBack(triples, tAfter, kMostSlots, k, takenSlot, after[k].loaded, written[i].wrote) &&
            Poke(written[i].firing + kFiringLoaded, &written[i].old, sizeof(int)))
            after[k].loaded = written[i].old;
    }
    FillSlots(s, t.slots, after, triples, tAfter, takenSlot);
    s.removed = !InWorld(a.entity, soldierAt);
    s.serial = ++g_swapSerial;
    s.at = now;
    AddSwap(s, now);
    std::memcpy(t.slots, after, sizeof(after));
    Serve(a.serial, 1, "");
}

void PlayerTick(uint32_t comp, uint32_t soldier, const Before& b, LONGLONG beforeTicks) {
    const Inside inside{};
    if (!inside.in)
        return;
    LARGE_INTEGER start{};
    QueryPerformanceCounter(&start);
    const ULONGLONG now = GetTickCount64();
    if (!g_gameThread.load(std::memory_order_relaxed))
        g_gameThread.store(GetCurrentThreadId(), std::memory_order_relaxed);
    // Another soldier, or another world: every table keyed by the game's pointers goes (a blueprint's address can hold another gun now).
    uint32_t world = 0;
    Read32(g_at.serverWorld, world);
    if (soldier != g_lastSoldier || world != g_lastWorld) {
        DropTables();
        g_lastSoldier = soldier, g_lastWorld = world;
    }
    Tick& t = g_tickScratch;
    t.at = now;
    t.thread = GetCurrentThreadId();
    t.soldier = soldier;
    pickrules::Triple triples[kMostSlots];
    bool list = false;
    ReadSlots(soldier, t.slots, triples, list);
    {
        int words[kMostSlots];
        for (int s = 0; s < kMostSlots; ++s)
            words[s] = t.slots[s].stateA8;
        t.activeSlot = pickrules::ActiveSlot(triples, words, kMostSlots);
    }
    float soldierAt[3] = {};
    SoldierAt(soldier, soldierAt);
    std::memcpy(t.soldierAt, soldierAt, sizeof(soldierAt));
    uint8_t flags = 0;
    Read8(comp + kComponentFlags, flags);

    // The game's own take: flag 4 clear before its update and set after it, and landed.
    bool ownTake = false;
    if (!(b.flags & kFlagTaken) && (flags & kFlagTaken) && b.list) {
        const int slot = b.haveTaken && b.taken.slot >= 0 && b.taken.slot < kMostSlots ? b.taken.slot : -1;
        const int loadedBefore = slot >= 0 ? b.slots[slot].loaded : -1, spareBefore = slot >= 0 ? b.slots[slot].spare : -1;
        const int loadedAfter = slot >= 0 ? t.slots[slot].loaded : -1, spareAfter = slot >= 0 ? t.slots[slot].spare : -1;
        if (pickrules::Landed(b.triple, triples, kMostSlots, slot, loadedBefore, spareBefore, loadedAfter, spareAfter)) {
            ownTake = true;
            Swap s;
            s.byHand = false;
            s.forceSlot = b.forceSlot;
            s.preferredSlot = b.preferredSlot;
            FillSlots(s, b.slots, t.slots, b.triple, triples, slot);
            const uint32_t heldNow = TargetHeld(comp);
            const uint32_t afterTarget = TargetPickup(comp);
            s.entity = afterTarget ? afterTarget : b.target;
            s.removed = heldNow == 0;
            if (s.entity == b.target && b.target) {
                std::memcpy(s.world, b.world, sizeof(s.world));
                if (b.haveTaken)
                    s.taken = b.taken;
                s.keepsNoPlace = b.keepsNoPlace, s.replaceAll = b.replaceAll;
            } else {
                ReadTransform(s.entity, s.world);
                for (int i = 0; i < s.changed; ++i)
                    if (s.came[i][0]) {
                        ContentOfSlot(t.slots[s.slot[i]], s.slot[i], s.taken);
                        break;
                    }
                uint32_t data = 0;
                if (s.entity && Read32(s.entity + kEntityData, data) && data) {
                    s.keepsNoPlace = Flag(data + kUnspawnOnPickup) || Flag(data + kContentIsStatic);
                    s.replaceAll = Flag(data + kReplaceAll);
                }
            }
            s.serial = ++g_swapSerial;
            s.at = now;
            AddSwap(s, now);
        }
    }

    // Is the walk wanted: a hand may reach, a request waits, a test (every tick); the policy or the log (once a second).
    const ULONGLONG reachingAt = g_reachingAt.load(std::memory_order_relaxed);
    const bool reaching = reachingAt && Age(now, reachingAt) < kStaleMs;
    const bool asking = g_askPosted.load(std::memory_order_acquire) > g_servedSerial;
    const bool takeLine = g_takeWanted.load(std::memory_order_relaxed);
    const bool everyTick = reaching || asking || takeLine;
    const bool policy = g_seconds.load(std::memory_order_relaxed) != 0 || g_unseen.load(std::memory_order_relaxed) ||
                        (g_ownSeconds.load(std::memory_order_relaxed) != 0 && OwnCount() > 0);
    const bool slowly = policy || g_logLevel.load(std::memory_order_relaxed) > 0;
    const ULONGLONG rest = g_restUntil.load(std::memory_order_relaxed);
    const bool resting = rest && now < rest;
    g_walkedThisTick = false;
    if (!resting && (everyTick || (slowly && now - g_walkAt >= kPolicyMs))) {
        // The gates (D1), every 0.2 s while a hand may reach or a take waits.
        if ((everyTick || !g_lookGatesAt) && now - g_lookGatesAt >= kLookMs) {
            g_gatesUp = GatesUp(soldier);
            g_lookGatesAt = now;
        }
        bool found = false;
        DWORD fault = 0;
        if (!WalkGuarded(soldierAt, 0, found, fault)) {
            if (fault)
                Faulted(fault);
        } else if (LookAtWalked(comp, t.slots, now)) {
            g_walkAt = now;
            g_walkedThisTick = true;
            KeepNearest(now);
            if (now - g_policyAt >= kPolicyMs) {
                g_policyAt = now;
                Policy(now);
            }
        }
    }
    t.gatesUp = g_gatesUp;

    // The pickup the game's own R would take now (the stopgap's gate).
    uint8_t flagsNow = 0;
    Read8(comp + kComponentFlags, flagsNow);
    t.gameTarget = 0, t.gameTargetBlueprint = 0, t.gameTargetName[0] = 0;
    t.gameTargetEntries = 0, t.gameTargetEntriesAll = 0, t.gameTargetReplaceAll = false, t.gameTargetUnspawnOrStatic = false;
    t.gameTargetKind = kNoKind;
    if (!(flagsNow & kFlagTaken))
        if (const uint32_t target = TargetPickup(comp)) {
            Pickup p;
            p.entity = target;
            ReadContent(target, kContentBegin, kContentEnd, p);
            t.gameTarget = target;
            if (p.first >= 0) {
                t.gameTargetBlueprint = p.content[p.first].blueprint;
                CopyName(t.gameTargetName, p.content[p.first].name);
            }
            // (What its take would put into it: holsters keep the game's own R from taking a gun off a shoulder.)
            t.gameTargetEntries = p.contents;
            t.gameTargetEntriesAll = p.contentsAll;
            for (int c = 0; c < p.contents && c < kMostContent; ++c)
                t.gameTargetSlot[c] = p.content[c].slot, t.gameTargetGun[c] = p.content[c].blueprint != 0;
            uint32_t data = 0, dataVt = 0;
            if (Read32(target + kEntityData, data) && data) {
                t.gameTargetReplaceAll = Flag(data + kReplaceAll);
                t.gameTargetUnspawnOrStatic = Flag(data + kUnspawnOnPickup) || Flag(data + kContentIsStatic);
                if (Read32(data, dataVt))
                    t.gameTargetKind = dataVt == g_at.dropDataVt ? kDrop : dataVt == g_at.placedDataVt ? kPlaced : kOtherKind;
            }
        }

    // A scripted use point the game targets now, and a hand's use of it waiting (busy: the next tick), after the game's own update (its own key's use
    // is made or not by now). The ask is taken under its lock, and the lock let go before the game's use is called: the render thread never waits on
    // the game's use (its scene may run then).
    ReadUse(comp, t);
    bool usedNow = false;
    if (g_useAskPosted.load(std::memory_order_acquire) > g_useServedSerial) {
        UseAsk a;
        {
            std::unique_lock lock(g_useMutex, std::try_to_lock);
            if (lock.owns_lock() && g_useAsk.serial > g_useServedSerial) {
                if (g_useAsk.dropped || g_useAsk.taken)
                    g_useServedSerial = g_useAsk.serial;  // (given up by the render thread: passed by)
                else
                    a = g_useAsk, g_useAsk.taken = true;
            }
        }
        if (a.serial) {
            ServeUse(a, comp, soldier, t, GetTickCount64());
            usedNow = true;
        }
    }

    // A request (busy: the next tick). Never in the tick a use was served: a use's scene may change the slots its take was read for.
    if (asking && !usedNow) {
        std::unique_lock lock(g_askMutex, std::try_to_lock);
        if (lock.owns_lock() && g_ask.serial > g_servedSerial && !g_ask.done) {
            if (g_ask.dropped) {
                Serve(g_ask.serial, -1, "not answered in 1 s");  // (the render thread has said it)
            } else {
                const Ask a = g_ask;
                ServeAsk(a, comp, soldier, soldierAt, t, triples, b.flags, flags, ownTake, GetTickCount64());  // (read under the lock: never before its ask)
            }
            g_ask.done = true;
        }
    }

    // Publish.
    t.served = g_servedSerial;
    t.servedResult = g_servedResult;
    strncpy_s(t.servedWhy, g_servedWhy, _TRUNCATE);
    t.newestSwap = g_ringScratch.newest;
    t.census = g_censusScratch.serial;
    if (g_walkedThisTick)
        g_censusLock.Write(g_censusScratch);
    LARGE_INTEGER end{};
    QueryPerformanceCounter(&end);
    if (!g_qpcPerSecond.QuadPart)
        QueryPerformanceFrequency(&g_qpcPerSecond);
    const float micros = static_cast<float>(static_cast<double>(end.QuadPart - start.QuadPart + beforeTicks) * 1e6 /
                                            static_cast<double>(g_qpcPerSecond.QuadPart ? g_qpcPerSecond.QuadPart : 1));
    if (now - g_microsFrom >= 1000) {
        g_microsFrom = now;
        g_microsMost = 0.0f;
    }
    g_microsMost = std::fmax(g_microsMost, micros);
    t.micros = g_microsMost;
    ++t.serial;
    g_tickLock.Write(t);
    pickparts::GameTick(now);  // (nothing unless its hooks are in: build\pickups.txt "parts")
}

// The player's soldier, and the same object the client made it from (a computer-controlled soldier's component costs one read).
bool IsPlayer(uint32_t soldier) {
    uint32_t vtable = 0, data = 0;
    return soldier == g_player.load(std::memory_order_relaxed) && Read32(soldier, vtable) && vtable == g_at.soldierVt &&
           Read32(soldier + 0x14, data) && data && data == g_playerData.load(std::memory_order_relaxed);
}

void __fastcall InteractionHook(void* component, void* edx, void* input) {
    const uint32_t comp = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(component));
    uint32_t soldier = 0;
    const bool player = g_live.load(std::memory_order_relaxed) && Read32(comp + kComponentSoldier, soldier) && soldier && IsPlayer(soldier);
    Before b;  // what must be read before the game's own update can change it
    LARGE_INTEGER t0{}, t1{};
    if (player) {
        QueryPerformanceCounter(&t0);
        ReadBefore(comp, soldier, b);
        QueryPerformanceCounter(&t1);
    }
    g_original(component, edx, input);
    if (player)
        PlayerTick(comp, soldier, b, t1.QuadPart - t0.QuadPart);  // never throws; guarded inside
    else if (!g_live.load(std::memory_order_relaxed))
        pickparts::GameTick(GetTickCount64());  // (pickups off or the player dead: no PlayerTick, but parts still held go at their tick)
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// The render thread's own.

struct FileValues {
    bool exists = false;
    int level = -1;  // -1 not given ("off" is 0)
    int log = 0;
    bool take = false;
    bool haveSeconds = false, haveOwn = false;
    int seconds = 0, own = 0;
    int unseen = -1;  // -1 not given
    bool slots = false;
    int parts = -1;  // -1 not given (pickparts.h: 0 off)
    Tune tune;
};

wchar_t g_testFile[MAX_PATH] = {};  // pickruntest's own pickups.txt (empty: beside the DLL)
FileValues g_file;
bool g_fileRead = false;
FILETIME g_fileWrite{};
ULONGLONG g_fileCheck = 0;
bool g_takePending = false;
ULONGLONG g_takeAskedAt = 0;

bool g_verified = false, g_failed = false, g_hooked = false;
int g_levelNow = -1, g_secondsNow = 0, g_ownNow = -1;
bool g_unseenNow = false, g_policySaid = false;
const char* g_levelFrom = "";

Tick g_tick;
Census g_census;
Ring g_ring;
bool g_haveTick = false, g_haveCensus = false;
uint32_t g_swapsSaid = 0, g_servedSaid = 0;
bool g_firstTickSaid = false;
int g_faultsSaid = 0;
bool g_takeFaultSaid = false, g_badGateSaid = false;
uint32_t g_raisedSaid = 0, g_heldSaid = 0;
ULONGLONG g_lastLog = 0, g_lastRaiseSay = 0, g_lastNames = 0;
uint32_t g_frameSoldier = 0, g_frameWorld = 0;

// The request this side waits on, and the last outcomes.
std::atomic<bool> g_takeOpen{false};
uint32_t g_askSerial = 0, g_waiting = 0;
ULONGLONG g_waitingAt = 0;
struct Answer {
    uint32_t serial = 0;
    int result = 0;
    char why[64] = {};
};
Answer g_outcomes[8];
int g_outcomeNext = 0;

// The boxes learned (by blueprint), and one read once to be read again a second later.
struct Box {
    uint32_t blueprint = 0;
    float min[3] = {}, max[3] = {};
    bool learned = false;
    ULONGLONG readAt = 0;
};
Box g_boxes[32];
ULONGLONG g_lastBoxLook = 0;

// Said once each: names with no row, own pickups, pickups held in view, keepsNoPlace refusals.
uint32_t g_saidOwn[16] = {};
uint32_t g_saidHeld[32] = {};
uint32_t g_saidNoPlace[16] = {};
char g_saidNoRow[32][kName] = {};

bool SaidOnce(uint32_t* list, size_t size, uint32_t value) {
    for (size_t i = 0; i < size; ++i)
        if (list[i] == value)
            return false;
    for (size_t i = 0; i < size; ++i)
        if (!list[i]) {
            list[i] = value;
            return true;
        }
    std::memmove(list, list + 1, sizeof(uint32_t) * (size - 1));
    list[size - 1] = value;
    return true;
}

// Onto the end of a line, never past it.
void Append(char* line, size_t size, int& used, const char* format, ...) {
    if (used < 0 || static_cast<size_t>(used) + 1 >= size)
        return;
    va_list args;
    va_start(args, format);
    const int n = vsnprintf(line + used, size - static_cast<size_t>(used), format, args);
    va_end(args);
    if (n > 0)
        used = std::min(used + n, static_cast<int>(size) - 1);
}

// A name the guns' table has no row for: said once.
void NoRow(const char* name) {
    if (!name || !name[0] || recoilguns::ByName(name))
        return;
    for (const char* said : g_saidNoRow)
        if (std::strcmp(said, name) == 0)
            return;
    for (char* said : g_saidNoRow)
        if (!said[0]) {
            CopyName(said, name);
            break;
        }
    Log("Pickups: no row for %s in the guns' table: free hands can't tell what it is before it's drawn", name);
}

const char* HandName(int side) {
    return side == 0 ? "the left hand" : side == 1 ? "the right hand" : "no hand";
}

const char* FindName(uint32_t entity) {
    for (int i = 0; i < g_census.count; ++i)
        if (g_census.pickup[i].entity == entity && g_census.pickup[i].first >= 0)
            return Nice(g_census.pickup[i].content[g_census.pickup[i].first].name);
    return "dropped gun";
}

// ---- pickups.txt ----

const char* Skip(const char* s) {
    while (*s == ' ' || *s == '\t')
        ++s;
    return s;
}

void Parse(FILE* f, FileValues& v) {
    char text[256];
    while (fgets(text, sizeof(text), f)) {
        const char* line = text;
        if (static_cast<unsigned char>(line[0]) == 0xEF && static_cast<unsigned char>(line[1]) == 0xBB && static_cast<unsigned char>(line[2]) == 0xBF)
            line += 3;
        line = Skip(line);
        int n = 0;
        if (std::strncmp(line, "off", 3) == 0)
            v.level = 0;
        else if (std::strncmp(line, "level", 5) == 0 && sscanf_s(line + 5, "%d", &n) == 1)
            v.level = std::clamp(n, 0, 2);
        else if (std::strncmp(line, "log", 3) == 0 && sscanf_s(line + 3, "%d", &n) == 1)
            v.log = std::clamp(n, 0, 2);
        else if (std::strncmp(line, "take", 4) == 0)
            v.take = true;
        else if (std::strncmp(line, "seconds", 7) == 0 && sscanf_s(line + 7, "%d", &n) == 1)
            v.haveSeconds = true, v.seconds = n < -1 ? -1 : std::min(n, 100000);
        else if (std::strncmp(line, "unseen", 6) == 0 && sscanf_s(line + 6, "%d", &n) == 1)
            v.unseen = n != 0 ? 1 : 0;
        else if (std::strncmp(line, "own", 3) == 0 && sscanf_s(line + 3, "%d", &n) == 1)
            v.haveOwn = true, v.own = n < -1 ? -1 : std::min(n, 100000);
        else if (std::strncmp(line, "slots", 5) == 0 && sscanf_s(line + 5, "%d", &n) == 1)
            v.slots = n == 1;
        else if (std::strncmp(line, "glidefrom", 9) == 0 && sscanf_s(line + 9, "%d", &n) == 1)
            v.tune.glideFromDrawn = n != 0;
        else if (std::strncmp(line, "prearm", 6) == 0 && sscanf_s(line + 6, "%d", &n) == 1)
            v.tune.preArm = n != 0;
        else if (std::strncmp(line, "unknown", 7) == 0 && sscanf_s(line + 7, "%d", &n) == 1)
            v.tune.takeUnknown = n != 0;
        else if (std::strncmp(line, "fire", 4) == 0 && sscanf_s(line + 4, "%d", &n) == 1)
            v.tune.testFire = n != 0;
        else if (std::strncmp(line, "half", 4) == 0 && sscanf_s(line + 4, "%d", &n) == 1)
            v.tune.testSecondHalf = n == 2;
        else if (std::strncmp(line, "parts", 5) == 0 && sscanf_s(line + 5, "%d", &n) == 1)
            v.parts = std::clamp(n, 0, 4);
        else if (std::strncmp(line, "usereach", 8) == 0 && sscanf_s(line + 8, "%d", &n) == 1)  // (before "use", which it begins with)
            v.tune.useReach = n != 0;
        else if (std::strncmp(line, "use", 3) == 0 && sscanf_s(line + 3, "%d", &n) == 1)
            v.tune.handUse = n != 0;
    }
}

void LogFileChanges(const FileValues& was, const FileValues& v, bool all) {
    if ((all && v.log) || (!all && v.log != was.log))
        Log(v.log == 2 ? "Pickups: every dropped gun near in the log each second (log 2)"
                       : v.log == 1 ? "Pickups: the census in the log each second (log 1)" : "Pickups: no census in the log (log 0)");
    if ((all && v.slots) || (!all && v.slots != was.slots))
        Log(v.slots ? "Pickups: the slot in the game's hand trusted by its state words (slots 1)"
                    : "Pickups: the slot in the game's hand not trusted (slots 0)");
    if ((all && v.tune.glideFromDrawn) || (!all && v.tune.glideFromDrawn != was.tune.glideFromDrawn))
        Log(v.tune.glideFromDrawn ? "Pickups: a taken gun flies in from its drawn pose where it lay (glidefrom 1)"
                                  : "Pickups: a taken gun flies in turned as the hand holds it, from the middle of where it lay (glidefrom 0)");
    if ((all && !v.tune.preArm) || (!all && v.tune.preArm != was.tune.preArm))
        Log(v.tune.preArm ? "Pickups: a taken gun's first frame drawn where it lay, or not at all (prearm 1)"
                          : "Pickups: a taken gun's first frame as before (prearm 0)");
    if ((all && !v.tune.takeUnknown) || (!all && v.tune.takeUnknown != was.tune.takeUnknown))
        Log(v.tune.takeUnknown ? "Pickups: a gun free hands don't know is offered to a hand too (unknown 1)"
                               : "Pickups: a gun free hands don't know isn't offered to a hand (unknown 0): X held takes it the game's way");
    if ((all && v.tune.testFire) || (!all && v.tune.testFire != was.tune.testFire))
        Log(v.tune.testFire ? "Pickups: the pickup round's trigger pulls are real rounds (fire 1)" : "Pickups: the pickup round pulls no trigger (fire 0)");
    if ((all && v.tune.testSecondHalf) || (!all && v.tune.testSecondHalf != was.tune.testSecondHalf))
        Log(v.tune.testSecondHalf ? "Pickups: the pickup round starts at its second half (half 2)" : "Pickups: the pickup round starts at its start");
    if ((all && !v.tune.handUse) || (!all && v.tune.handUse != was.tune.handUse))
        Log(v.tune.handUse ? "Pickups: a scripted prompt is used by holding a free hand's grip (use 1)"
                           : "Pickups: no scripted prompt used by a hand (use 0): X held and the game's own keys as before");
    if ((all && v.tune.useReach) || (!all && v.tune.useReach != was.tune.useReach))
        Log(v.tune.useReach ? "Pickups: a scripted prompt is used by reaching for it and holding the grip there (usereach 1, 0.1.14's rule)"
                            : "Pickups: a scripted prompt is used by holding the grip of a free hand anywhere while it shows (usereach 0)");
}

const wchar_t* FilePath() {
    static wchar_t path[MAX_PATH] = {};
    if (g_testFile[0])
        return g_testFile;
    if (!path[0]) {
        HMODULE self = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&FilePath),
                           &self);
        GetModuleFileNameW(self, path, MAX_PATH);
        if (wchar_t* slash = wcsrchr(path, L'\\'))
            wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path), L"pickups.txt");
    }
    return path;
}

void ReadTestFile(ULONGLONG now) {
    if (g_fileRead && now - g_fileCheck < 300)
        return;
    g_fileCheck = now;
    const wchar_t* path = FilePath();
    WIN32_FILE_ATTRIBUTE_DATA info{};
    const bool exists = GetFileAttributesExW(path, GetFileExInfoStandard, &info) != 0;
    if (!exists)
        info.ftLastWriteTime = FILETIME{};
    if (g_fileRead && CompareFileTime(&info.ftLastWriteTime, &g_fileWrite) == 0 && exists == g_file.exists)
        return;
    FILE* f = nullptr;
    if (exists && (_wfopen_s(&f, path, L"r") != 0 || !f))
        return;  // still being written (the open isn't shared): read at the next look
    g_fileWrite = info.ftLastWriteTime;
    FileValues v;
    v.exists = exists;
    if (f) {
        Parse(f, v);
        fclose(f);
    }
    LogFileChanges(g_file, v, !g_fileRead);
    if (v.take) {
        g_takePending = true;
        g_takeAskedAt = now;
    }
    g_file = v;
    g_fileRead = true;
}

// ---- The policy and the level ----

int SecondsOf(float v) {
    if (!std::isfinite(v) || v == 0.0f)
        return 0;
    if (v < 0.0f)
        return -1;
    return static_cast<int>(std::lround(std::fmin(v, 100000.0f)));
}

void SayPolicy() {
    if (g_levelNow == 0) {
        Log(g_file.level == 0 ? "Pickups: off (pickups.txt): nothing of the game's hooked or called; X held asks the game's own pickup as before"
                              : "Pickups: off (bf3vr.ini pickups=0): nothing of the game's hooked or called; X held asks the game's own pickup as before");
        return;
    }
    char line[400];
    int used = 0;
    Append(line, sizeof(line), used, "Pickups: level %d (%s): %s; ", g_levelNow, g_levelFrom,
           g_levelNow >= 2 ? "a hand can take a dropped gun" : "the game's own pickup alone (X held), a hand takes none");
    if (g_secondsNow == 0)
        Append(line, sizeof(line), used, "a dropped gun stays the game's own time");
    else if (g_secondsNow < 0)
        Append(line, sizeof(line), used, "a dropped gun stays for good (the nearest %d)", kNeverKept);
    else
        Append(line, sizeof(line), used, "a dropped gun stays %d s", g_secondsNow);
    Append(line, sizeof(line), used, g_unseenNow ? ", and never goes while you look at it; " : "; ");
    if (g_ownNow < 0)
        Append(line, sizeof(line), used, "your own until you take it");
    else if (g_ownNow == 0)
        Append(line, sizeof(line), used, "your own as any other");
    else
        Append(line, sizeof(line), used, "your own %d s", g_ownNow);
    Log("%s", line);
}

void UpdatePolicy() {
    const config::Settings& c = config::Get();
    const int level = g_file.level >= 0 ? g_file.level : std::clamp(c.pickups, 0, 2);
    const int seconds = g_file.haveSeconds ? g_file.seconds : SecondsOf(c.pickupSeconds);
    const bool unseen = g_file.unseen >= 0 ? g_file.unseen != 0 : c.pickupUnseen;
    const int own = g_file.haveOwn ? g_file.own : SecondsOf(c.pickupOwnSeconds);
    g_level.store(level, std::memory_order_relaxed);
    g_seconds.store(seconds, std::memory_order_relaxed);
    g_unseen.store(unseen, std::memory_order_relaxed);
    g_ownSeconds.store(own, std::memory_order_relaxed);
    g_logLevel.store(g_file.log, std::memory_order_relaxed);
    if (!g_policySaid || level != g_levelNow || seconds != g_secondsNow || unseen != g_unseenNow || own != g_ownNow) {
        g_policySaid = true;
        g_levelNow = level, g_secondsNow = seconds, g_unseenNow = unseen, g_ownNow = own;
        g_levelFrom = g_file.level >= 0 ? "pickups.txt" : "bf3vr.ini";
        SayPolicy();
    }
}

// ---- The checks and the hook ----

void Fail(const char* why) {
    if (g_failed)
        return;
    g_failed = true;
    Log("Pickups: the game's code isn't as read: %s: left off for this session (X held still asks the game's own pickup)", why);
}

// The image's code and read-only data from its own section headers (.text and .rdata), unless given already (pickruntest gives its own).
bool ImageRanges() {
    if (g_at.codeTo && g_at.rdataTo)
        return true;
    uint32_t peAt = 0, signature = 0;
    uint16_t sections = 0, optional = 0;
    if (!Read32(kImageBase + 0x3C, peAt) || peAt > 0x1000 || !Read32(kImageBase + peAt, signature) || signature != 0x00004550 ||
        !Read16(kImageBase + peAt + 6, sections) || !Read16(kImageBase + peAt + 20, optional) || sections > 64)
        return false;
    const uintptr_t table = kImageBase + peAt + 24 + optional;
    for (int i = 0; i < sections; ++i) {
        char name[8] = {};
        uint32_t size = 0, va = 0;
        if (!camera::SafeRead(table + 40 * i, name, sizeof(name)) || !Read32(table + 40 * i + 8, size) || !Read32(table + 40 * i + 12, va))
            return false;
        if (std::memcmp(name, ".text\0\0\0", 8) == 0)
            g_at.codeFrom = kImageBase + va, g_at.codeTo = kImageBase + va + size;
        else if (std::memcmp(name, ".rdata\0\0", 8) == 0)
            g_at.rdataFrom = kImageBase + va, g_at.rdataTo = kImageBase + va + size;
    }
    return g_at.codeTo > g_at.codeFrom && g_at.rdataTo > g_at.rdataFrom;
}

void VerifyUse();

bool Verify() {
    // A level first: the server's world set and as read (the same one odd three times running: off).
    uint32_t world = 0, vtable = 0;
    if (!Read32(g_at.serverWorld, world) || !world)
        return false;
    if (!Read32(world, vtable) || vtable != g_at.serverWorldVt) {
        static uint32_t s_odd = 0;
        static int s_times = 0;
        uint32_t still = 0;
        if (!Read32(g_at.serverWorld, still) || still != world) {
            s_times = 0;
            return false;
        }
        s_times = s_odd == world ? s_times + 1 : 1;
        s_odd = world;
        if (s_times < 3)
            return false;
        char why[160];
        snprintf(why, sizeof(why), "the server's world (its vtable %08X at %08X)", vtable, world);
        Fail(why);
        return false;
    }
    char why[200];
    for (int i = 0; i < g_checks.codes; ++i) {
        const Code& c = g_checks.code[i];
        if (!SameHash(c)) {
            snprintf(why, sizeof(why), "%s (%u bytes at %08X)", c.what, c.size, static_cast<unsigned>(c.at));
            Fail(why);
            return false;
        }
    }
    for (int i = 0; i < g_checks.byteRows; ++i) {
        const Bytes& b = g_checks.bytes[i];
        if (!SameBytes(b.at, b.bytes, b.size)) {
            snprintf(why, sizeof(why), "%s (%u bytes at %08X)", b.what, b.size, static_cast<unsigned>(b.at));
            Fail(why);
            return false;
        }
    }
    for (int i = 0; i < g_checks.wordRows; ++i) {
        const Word& w = g_checks.words[i];
        uint32_t value = 0;
        if (!camera::SafeRead(w.at, &value, w.size) || value != w.value) {
            snprintf(why, sizeof(why), "%s (%u bytes at %08X)", w.what, w.size, static_cast<unsigned>(w.at));
            Fail(why);
            return false;
        }
    }
    if (!ImageRanges()) {
        snprintf(why, sizeof(why), "the image's sections (its headers at %08X)", static_cast<unsigned>(kImageBase));
        Fail(why);
        return false;
    }
    g_verified = true;
    Log("Pickups: the game's code is as read (its interaction update, its take, its checks of a pickup, AllowPickup, the pickup's countdown, the "
        "world's lists, the client's pose of a dropped gun)");
    VerifyUse();
    return true;
}

// Every row of a set of checks as read: its words, and with code its code's hashes and its bytes too; false with the first that isn't, in why.
bool ChecksPass(const Checks& c, bool code, char* why, size_t size) {
    for (int i = 0; i < c.wordRows; ++i) {
        const Word& w = c.words[i];
        uint32_t value = 0;
        if (!camera::SafeRead(w.at, &value, w.size) || value != w.value) {
            snprintf(why, size, "%s (%u bytes at %08X)", w.what, w.size, static_cast<unsigned>(w.at));
            return false;
        }
    }
    for (int i = 0; code && i < c.codes; ++i) {
        const Code& k = c.code[i];
        if (!SameHash(k)) {
            snprintf(why, size, "%s (%u bytes at %08X)", k.what, k.size, static_cast<unsigned>(k.at));
            return false;
        }
    }
    for (int i = 0; code && i < c.byteRows; ++i) {
        const Bytes& b = c.bytes[i];
        if (!SameBytes(b.at, b.bytes, b.size)) {
            snprintf(why, size, "%s (%u bytes at %08X)", b.what, b.size, static_cast<unsigned>(b.at));
            return false;
        }
    }
    return true;
}

// The scripted use points' checks (g_useChecks), once the pickups' own are as read: their words first (the point's vtable and class: a target is read
// and published, a hand's use is the prompt's key), then the use's code and the update's call of it (the game's own use is called). Not as read: said
// once, and pickups go on as before.
void VerifyUse() {
    char why[200];
    if (!ChecksPass(g_useChecks, false, why, sizeof(why))) {
        g_useState.store(-1, std::memory_order_relaxed);
        Log("Pickups: scripted use points (a prompt's key near a point): %s isn't as read: left as the game has them (its own key alone)", why);
        return;
    }
    if (!ChecksPass(g_useChecks, true, why, sizeof(why))) {
        g_useState.store(1, std::memory_order_relaxed);
        Log("Pickups: scripted use points read (the point's type as read), but %s isn't: a hand's use of one presses the prompt's own key", why);
        return;
    }
    g_useState.store(2, std::memory_order_relaxed);
    Log("Pickups: scripted use points: the game's use of one and its call are as read: a hand's use of a prompt is the game's own");
}

bool HookOne() {
    char why[160];
    if (!SameBytes(g_at.update, kUpdateStart, sizeof(kUpdateStart))) {
        snprintf(why, sizeof(why), "its interaction update's start isn't as read any more (at %08X)", static_cast<unsigned>(g_at.update));
        Fail(why);
        return false;
    }
    void* trampoline = nullptr;
    if (MH_CreateHook(reinterpret_cast<void*>(g_at.update), reinterpret_cast<void*>(&InteractionHook), &trampoline) != MH_OK || !trampoline) {
        Fail("its interaction update couldn't be hooked");
        return false;
    }
    g_original = reinterpret_cast<InteractionFn>(trampoline);
    if (MH_EnableHook(reinterpret_cast<void*>(g_at.update)) != MH_OK) {
        MH_RemoveHook(reinterpret_cast<void*>(g_at.update));
        g_original = nullptr;
        Fail("its interaction update's hook couldn't be turned on");
        return false;
    }
    g_hooked = true;
    Log("Pickups: hooked its interaction update");
    return true;
}

// ---- What the render thread says ----

void SayNames(char* line, size_t size, int& used, const char (*names)[kName], const int* slots, int n, bool withSlot) {
    int said = 0;
    for (int i = 0; i < n; ++i) {
        if (!names[i][0])
            continue;
        Append(line, size, used, said ? " and the %s" : "the %s", Nice(names[i]));
        if (withSlot)
            Append(line, size, used, " into slot %d", slots[i]);
        ++said;
    }
    if (!said)
        Append(line, size, used, "nothing");
}

void SaySwap(const Swap& s) {
    char line[600];
    int used = 0;
    if (s.byHand)
        Append(line, sizeof(line), used, "Pickups: the game took it (#%u, %s): ", s.request, HandName(s.side));
    else
        Append(line, sizeof(line), used, "Pickups: the game's own pickup (R held): ");
    SayNames(line, sizeof(line), used, s.came, s.slot, s.changed, true);
    Append(line, sizeof(line), used, "; ");
    SayNames(line, sizeof(line), used, s.went, s.slot, s.changed, false);
    Append(line, sizeof(line), used, s.byHand ? " into its place, where it lay" : " into its place");
    if (s.removed)
        Append(line, sizeof(line), used, ", and it went with the take");
    if (s.forceSlot)
        Append(line, sizeof(line), used, "; its slot forced");
    if (s.preferredSlot >= 0)
        Append(line, sizeof(line), used, " (it prefers slot %d)", s.preferredSlot);
    // (The slot the taken gun's entry names: the game puts the gun of that slot into the pickup and gives its gun there, 008AB870. Said, so a swap
    // read otherwise in a log settles it: the review of 2026-11-18.)
    if (s.taken.slot >= 0)
        Append(line, sizeof(line), used, "; its entry named slot %d", s.taken.slot);
    Log("%s", line);
    NoRow(s.taken.name);
    for (int i = 0; i < s.changed; ++i) {
        NoRow(s.came[i]);
        NoRow(s.went[i]);
    }
    // Your own gun now in that pickup.
    bool went = false;
    for (int i = 0; i < s.changed; ++i)
        went = went || s.went[i][0];
    if (went && !s.removed && g_ownNow != 0 && s.entity && SaidOnce(g_saidOwn, std::size(g_saidOwn), s.entity)) {
        const char* name = "gun";
        for (int i = 0; i < s.changed; ++i)
            if (s.went[i][0]) {
                name = Nice(s.went[i]);
                break;
            }
        if (g_ownNow < 0)
            Log("Pickups: your %s kept where it lies until you take it (pickup_own_seconds)", name);
        else
            Log("Pickups: your %s kept %d s where it lies (pickup_own_seconds)", name, g_ownNow);
    }
}

bool Noted(uint32_t serial) {
    for (const Answer& o : g_outcomes)
        if (o.serial == serial)
            return true;
    return false;
}

void NoteOutcome(uint32_t serial, int result, const char* why) {
    Answer& o = g_outcomes[g_outcomeNext];
    g_outcomeNext = (g_outcomeNext + 1) % static_cast<int>(std::size(g_outcomes));
    o.serial = serial;
    o.result = result;
    strncpy_s(o.why, why ? why : "", _TRUNCATE);
    if (serial == g_waiting) {
        g_waiting = 0;
        g_takeOpen.store(false, std::memory_order_relaxed);
    }
}

void SayCensus(ULONGLONG now) {
    const int level = g_file.log;
    if (!level || now - g_lastLog < 1000 || !g_haveTick)
        return;
    g_lastLog = now;
    char line[700];
    int used = 0;
    const bool census = g_haveCensus && g_census.count > 0;
    if (census) {
        const Pickup& p = g_census.pickup[0];
        Append(line, sizeof(line), used, "Pickups: %d lying near: the nearest the %s %.1f m away, ", g_census.total,
               p.first >= 0 ? Nice(p.content[p.first].name) : "(nothing in it)",
               p.distance);
        if (p.timeToLive > 0.0f)
            Append(line, sizeof(line), used, "%.0f s left of %.0f", p.secondsLeft, p.timeToLive);
        else
            Append(line, sizeof(line), used, "no time of its own");
        Append(line, sizeof(line), used, " (%s%s%s%s)", p.allowed ? "allowed" : "not allowed", p.eligible ? ", eligible" : "", p.own ? ", your own" : "",
               p.keepsNoPlace ? ", keeps no place" : "");
    } else {
        Append(line, sizeof(line), used, "Pickups: none lying near");
    }
    const char* target = g_tick.gameTarget ? (g_tick.gameTargetName[0] ? Nice(g_tick.gameTargetName) : "a pickup") : "none";
    Append(line, sizeof(line), used, "; the game's target: %s", target);
    if (g_tick.activeSlot >= 0)
        Append(line, sizeof(line), used, "; slot %d in hand by its words (", g_tick.activeSlot);
    else
        Append(line, sizeof(line), used, "; no slot in hand by its words (");
    int said = 0;
    for (int s = 0; s < kMostSlots; ++s)
        if (g_tick.slots[s].weapon) {
            const Slot& w = g_tick.slots[s];
            Append(line, sizeof(line), used, "%s%d: %d/%d/%d", said ? ", " : "", s, w.stateA4, w.stateA8, w.stateAC);
            ++said;
        }
    Append(line, sizeof(line), used, "); the tick %.0f us", g_tick.micros);
    Log("%s", line);
    if (level >= 2 && g_haveCensus)
        for (int i = 0; i < g_census.count; ++i) {
            const Pickup& p = g_census.pickup[i];
            Log("Pickups:   %s %.1f m at (%.2f %.2f %.2f): %s, %.0f s left of %.0f, %s%s%s%s%s, %d in it",
                p.first >= 0 ? Nice(p.content[p.first].name) : "(nothing)",
                p.distance, p.world[12], p.world[13], p.world[14], p.kind == kDrop ? "a soldier's" : p.kind == kPlaced ? "the level's" : "another kind",
                p.secondsLeft, p.timeToLive, p.allowed ? "allowed" : "not allowed", p.eligible ? ", eligible" : "", p.own ? ", your own" : "",
                p.keepsNoPlace ? ", keeps no place" : "", p.unspawnOnPickup ? ", goes with its take" : "", p.contents);
        }
}

// The third-person boxes, from the client's pickups (at most once a second while one in the census lacks a box).
const Box* BoxOf(uint32_t blueprint) {
    for (const Box& b : g_boxes)
        if (b.learned && b.blueprint == blueprint)
            return &b;
    return nullptr;
}

bool HasText(const char* text, const char* part) {
    const size_t n = std::strlen(part);
    if (!n)
        return false;
    for (const char* t = text; *t; ++t)
        if (_strnicmp(t, part, n) == 0)
            return true;
    return false;
}

// The census's gun with that blueprint whose box isn't learned yet (its name), or none.
const char* Lacking(uint32_t blueprint) {
    if (!blueprint || BoxOf(blueprint))
        return nullptr;
    for (int i = 0; i < g_census.count; ++i) {
        const Pickup& p = g_census.pickup[i];
        if (p.first >= 0 && p.content[p.first].blueprint == blueprint)
            return p.content[p.first].name;
    }
    return nullptr;
}

// One client pickup's MeshSet for a gun lacking its box: taken when finite, each side 0.005-3 m, its name with "3p" and the gun's last path part
// in it, and the same box read twice a second apart.
void LookAtBox(uint32_t blueprint, const char* gunName, uint32_t set, ULONGLONG now) {
    float lo[3], hi[3];
    char name[kName];
    ReadText(set + kMeshSetName, name);
    if (!ReadFloats(set + kMeshSetMin, lo, 3) || !ReadFloats(set + kMeshSetMax, hi, 3) || !Finite(lo, 3) || !Finite(hi, 3))
        return;
    bool sized = true;
    for (int k = 0; k < 3; ++k)
        sized = sized && hi[k] - lo[k] >= 0.005f && hi[k] - lo[k] <= 3.0f;
    const char* slash = std::strrchr(gunName, '/');
    if (!sized || !HasText(name, "3p") || !HasText(name, slash ? slash + 1 : gunName))
        return;
    Box* slot = nullptr;
    for (Box& b : g_boxes)
        if (b.blueprint == blueprint) {
            slot = &b;
            break;
        }
    if (!slot)
        for (Box& b : g_boxes)
            if (!b.blueprint) {
                slot = &b;
                break;
            }
    if (!slot)
        return;
    if (slot->blueprint == blueprint && slot->readAt && now - slot->readAt >= 1000 && std::memcmp(slot->min, lo, sizeof(lo)) == 0 &&
        std::memcmp(slot->max, hi, sizeof(hi)) == 0) {
        slot->learned = true;
        Log("Pickups: the %s's 3P box (%.2f %.2f %.2f) to (%.2f %.2f %.2f) (its MeshSet %s)", Nice(gunName), lo[0], lo[1], lo[2], hi[0], hi[1], hi[2], name);
    } else {
        slot->blueprint = blueprint;
        std::memcpy(slot->min, lo, sizeof(lo));
        std::memcpy(slot->max, hi, sizeof(hi));
        slot->readAt = now;
    }
}

void LearnBoxes(ULONGLONG now) {
    if (!g_haveCensus || now - g_lastBoxLook < 1000)
        return;
    bool any = false;
    for (int i = 0; i < g_census.count && !any; ++i) {
        const Pickup& p = g_census.pickup[i];
        any = p.first >= 0 && Lacking(p.content[p.first].blueprint);
    }
    if (!any)
        return;
    g_lastBoxLook = now;
    uint32_t world = 0, vtable = 0, lists = 0, listsEnd = 0, segment = 0;
    uint16_t first = 0, cls = 0;
    if (!Read32(g_at.clientWorld, world) || !world || !Read32(world, vtable) || vtable != g_at.clientWorldVt || !Read16(world + kWorldFirstClass, first) ||
        !Read16(g_at.clientPickupType + kClassIdAt, cls) || cls < first || !Read32(world + kWorldLists, lists) || !Read32(world + kWorldListsEnd, listsEnd))
        return;
    const uint32_t at = lists + 8u * (cls - first);
    if (!lists || at + 8 > listsEnd || !Read32(at, segment))
        return;
    uint32_t done[kMost] = {};
    int doneCount = 0;
    for (int guard = 0; segment && guard < kMostSegments; ++guard) {
        uint32_t begin = 0, end = 0, next = 0;
        int active = 0;
        if (!Read32(segment, begin) || !Read32(segment + 4, end) || !ReadInt(segment + kSegmentActive, active) || !Read32(segment + kSegmentNext, next))
            return;
        const int all = end >= begin && end - begin <= 4u * 4096 ? static_cast<int>((end - begin) / 4) : 0;
        for (int i = 0; i < std::min(std::max(active, 0), all); ++i) {
            uint32_t entity = 0, entityVt = 0, contentBegin = 0, contentEnd = 0, blueprint = 0, instance = 0, inner = 0, set = 0;
            if (!Read32(begin + 4 * i, entity) || !entity || !Read32(entity, entityVt) || entityVt != g_at.clientPickupVt ||
                !Read32(entity + kClientContentBegin, contentBegin) || !Read32(entity + kClientContentEnd, contentEnd) || !contentBegin ||
                contentEnd < contentBegin + kEntrySize)
                continue;
            for (uint32_t q = contentBegin; q + kEntrySize <= contentEnd && q < contentBegin + kEntrySize * kMostContent && !blueprint; q += kEntrySize)
                Read32(q, blueprint);
            const char* gunName = Lacking(blueprint);
            if (!gunName || std::find(done, done + doneCount, blueprint) != done + doneCount || !Read32(entity + kClientMesh, instance) || !instance ||
                !Read32(instance, inner) || !inner || !Read32(inner + 0x10, set) || !set)
                continue;
            if (doneCount < kMost)
                done[doneCount++] = blueprint;  // (one of each gun a pass)
            LookAtBox(blueprint, gunName, set, now);
        }
        segment = next;
    }
}

void SayFaults() {
    if (const int faults = g_faults.load(std::memory_order_acquire); faults > g_faultsSaid) {
        g_faultsSaid = faults;
        if (g_dead.load(std::memory_order_relaxed))
            Log("Pickups: the walk faulted (exception %08lX, %d of %d this session): left off for this session (X held still asks the game's own pickup)",
                g_faultCode.load(std::memory_order_relaxed), faults, kMostFaults);
        else
            Log("Pickups: the walk faulted (exception %08lX): no walk for 5 s", g_faultCode.load(std::memory_order_relaxed));
    }
    if (const uint32_t table = g_badGateTable.load(std::memory_order_relaxed); table && !g_badGateSaid) {
        g_badGateSaid = true;
        Log("Pickups: a gate of the soldier's isn't in the game's code (its table %08X, its function %08X): no pickup taken (a fault)", table,
            g_badGateFunction.load(std::memory_order_relaxed));
    }
    if (const DWORD code = g_takeFaultCode.load(std::memory_order_relaxed); code && !g_takeFaultSaid) {
        g_takeFaultSaid = true;
        Log("Pickups: the game's take faulted (exception %08lX): no more takes this session", code);
    }
}

void SayPolicyDoings(ULONGLONG now) {
    const uint32_t raised = g_raised.load(std::memory_order_relaxed);
    if (raised != g_raisedSaid && now - g_lastRaiseSay >= 1000) {
        Log("Pickups: %u dropped guns' time raised to %d s (pickup_seconds)", raised - g_raisedSaid, g_secondsNow);
        g_raisedSaid = raised;
        g_lastRaiseSay = now;
    }
    const uint32_t held = g_heldCount.load(std::memory_order_acquire);
    if (held - g_heldSaid > std::size(g_heldEntity))
        g_heldSaid = held - static_cast<uint32_t>(std::size(g_heldEntity));
    for (; g_heldSaid != held; ++g_heldSaid) {
        const uint32_t entity = g_heldEntity[g_heldSaid % std::size(g_heldEntity)].load(std::memory_order_relaxed);
        if (entity && SaidOnce(g_saidHeld, std::size(g_saidHeld), entity))
            Log("Pickups: the %s kept while in view at the end of its time (pickup_unseen)", FindName(entity));
    }
}

// pickups.txt "take": once per write, with no hand, once a census newer than the line has come (1 s at most).
void FileTake(ULONGLONG now) {
    if (!g_takePending)
        return;
    g_takeWanted.store(true, std::memory_order_relaxed);
    const bool newer = g_haveCensus && g_census.at >= g_takeAskedAt;
    if (!newer && now - g_takeAskedAt < 1000)
        return;
    g_takePending = false;
    g_takeWanted.store(false, std::memory_order_relaxed);
    const Pickup* best = nullptr;
    if (g_haveCensus)
        for (int i = 0; i < g_census.count; ++i) {
            const Pickup& p = g_census.pickup[i];
            if (p.allowed && p.first >= 0 && p.distance <= kFileTakeWithin && (!best || p.distance < best->distance))
                best = &p;
        }
    if (!best) {
        Log("Pickups: take: nothing the game allows within 2.5 m");
        return;
    }
    const char* why = "";
    const uint32_t serial = RequestTake(best->entity, best->content[best->first].blueprint, -1, &why);
    if (serial)
        Log("Pickups: asked the game for the %s %.1f m away (pickups.txt take, #%u)", Nice(best->content[best->first].name), best->distance, serial);
    else
        Log("Pickups: take: the %s not asked for: %s", Nice(best->content[best->first].name), why);
}

}  // namespace

// ---------------------------------------------------------------------------------------------------------------------------------------------------

void OnFrame() {
    const ULONGLONG now = GetTickCount64();
    ReadTestFile(now);
    UpdatePolicy();

    // The player's soldier for the game's thread, and the client's data it was made from.
    const uint32_t player = static_cast<uint32_t>(god::PlayerServerSoldier());
    uint32_t data = 0;
    if (const uintptr_t client = god::PlayerClientSoldier())
        Read32(client + 0x14, data);
    g_player.store(player, std::memory_order_relaxed);
    g_playerData.store(data, std::memory_order_relaxed);
    uint32_t world = 0;
    Read32(g_at.serverWorld, world);
    if (player != g_frameSoldier || world != g_frameWorld) {
        for (Box& b : g_boxes)
            b = Box{};
        g_frameSoldier = player, g_frameWorld = world;
    }

    // The eye for "unseen".
    if (const loose::Frame* f = hands::LooseFrame()) {
        Eye e;
        for (int i = 0; i < 3; ++i) {
            e.eye[i] = f->eye[i] + static_cast<float>(f->origin[i]);
            e.look[i] = f->look[i];
        }
        e.at = now;
        g_eyeLock.Write(e);
    }

    // The checks once a level is up, the hook right after them (not before pickups.txt has been read once: it may say "off").
    if (g_fileRead && g_levelNow != 0 && !g_failed && !g_verified)
        Verify();
    if (g_fileRead && g_levelNow != 0 && g_verified && !g_hooked && !g_failed)
        HookOne();
    g_live.store(g_hooked && g_verified && !g_dead.load(std::memory_order_relaxed) && g_levelNow != 0, std::memory_order_relaxed);
    pickparts::OnFrame(now, g_levelNow, g_file.parts, g_verified);  // a dropped gun's attachments (parts 0 or no line: nothing)
    if (!g_live.load(std::memory_order_relaxed)) {
        g_haveTick = g_haveCensus = false;
        if (g_waiting && now - g_waitingAt > kUnansweredMs) {
            NoteOutcome(g_waiting, -1, "pickups went off");
        }
        if (!g_hooked)
            return;
    }

    // This frame's copies.
    Tick tick;
    g_haveTick = g_tickLock.Read(tick) && Age(now, tick.at) < kStaleMs;
    if (g_haveTick)
        g_tick = tick;
    if (g_haveTick && !g_firstTickSaid) {
        g_firstTickSaid = true;
        Log("Pickups: the game's thread %lu (the render thread %lu)", tick.thread, GetCurrentThreadId());
    }
    if (g_tickLock.seq.load(std::memory_order_relaxed) && (g_tick.census != g_census.serial || !g_haveCensus)) {
        static Census census;  // (the render thread alone: off its stack)
        if (g_censusLock.Read(census))
            g_census = census;
    }
    g_haveCensus = g_census.serial && Age(now, g_census.at) < kStaleMs;
    if (g_haveTick && g_tick.newestSwap != g_ring.newest) {
        static Ring ring;
        if (g_ringLock.Read(ring))
            g_ring = ring;
    }

    // Each new swap, oldest first: said, then told to ammo (before ammo::Update meets the new gun).
    Swap s;
    while (NextSwap(g_swapsSaid, s)) {
        g_swapsSaid = s.serial;
        SaySwap(s);
        ammo::Swapped(s);
    }

    // The request: its outcome (not while this frame's swaps are behind the tick: ammo hears of a swap before TakeOpen goes down), or dropped
    // when nobody serves it.
    if (g_haveTick && g_tick.served != g_servedSaid && g_ring.newest >= g_tick.newestSwap) {
        g_servedSaid = g_tick.served;
        if (!Noted(g_tick.served)) {
            NoteOutcome(g_tick.served, g_tick.servedResult, g_tick.servedWhy);
            if (g_tick.servedResult < 0)
                Log("Pickups: the game didn't take it (#%u): %s", g_tick.served, g_tick.servedWhy);
        }
    }
    if (g_waiting && now - g_waitingAt > kUnansweredMs) {
        bool dropped = false;
        {
            std::lock_guard lock(g_askMutex);
            if (g_ask.serial == g_waiting && !g_ask.done) {
                g_ask.dropped = true;
                dropped = true;
            }
        }
        const uint32_t serial = g_waiting;
        if (dropped || now - g_waitingAt > 2 * kUnansweredMs) {
            NoteOutcome(serial, -1, "not answered in 1 s");
            Log("Pickups: a take (#%u) not answered in 1 s: dropped", serial);
        }
        g_takeOpen.store(false, std::memory_order_relaxed);
    }

    // The census's and the slots' names against the guns' table (once a second), the boxes, the test's take, the log.
    if (now - g_lastNames >= 1000) {
        g_lastNames = now;
        if (g_haveCensus)
            for (int i = 0; i < g_census.count; ++i)
                if (g_census.pickup[i].first >= 0)
                    NoRow(g_census.pickup[i].content[g_census.pickup[i].first].name);
        if (g_haveTick)
            for (const Slot& slot : g_tick.slots)
                NoRow(slot.name);
    }
    LearnBoxes(now);
    FileTake(now);
    SayCensus(now);
    SayFaults();
    SayPolicyDoings(now);
}

void SetReaching(bool reaching) {
    g_reachingAt.store(reaching ? GetTickCount64() : 0, std::memory_order_relaxed);
}

bool Hooked() {
    return g_hooked && g_verified && !g_dead.load(std::memory_order_relaxed);
}

bool TakeOpen() {
    return g_takeOpen.load(std::memory_order_relaxed);
}

bool Now(Tick& out) {
    if (!g_haveTick)
        return false;
    out = g_tick;
    return true;
}

bool Near(Census& out) {
    if (!g_haveCensus)
        return false;
    out = g_census;
    return true;
}

int Reachable(const float* eyeWorld, float within, Reach* out, int most) {
    if (!g_live.load(std::memory_order_relaxed) || !g_haveCensus || !eyeWorld || !out || most <= 0)
        return 0;
    struct Found {
        int index;
        float distance;
    };
    Found found[kMost];
    int n = 0;
    for (int i = 0; i < g_census.count; ++i) {
        const Pickup& p = g_census.pickup[i];
        if (!p.allowed || !p.eligible || p.first < 0 || p.keepsNoPlace)
            continue;
        const float d = Distance(p.world + 12, eyeWorld);
        if (!(d <= within))
            continue;
        found[n++] = {i, d};
    }
    std::sort(found, found + n, [](const Found& a, const Found& b) { return a.distance < b.distance; });
    int kept = 0;
    for (int i = 0; i < n && kept < most; ++i) {
        const Pickup& p = g_census.pickup[found[i].index];
        const Content& c = p.content[p.first];
        Reach& r = out[kept];
        r = Reach{};
        if (!pickrules::DrawnPose(p.world, c.offset, c.rotation, r.drawn))
            continue;
        r.entity = p.entity;
        r.blueprint = c.blueprint;
        CopyName(r.name, c.name);
        r.slot = c.slot;
        r.entries = std::min(p.contents, kMostContent);
        r.entriesAll = p.contentsAll;
        for (int e = 0; e < r.entries; ++e)
            r.entrySlot[e] = p.content[e].slot, r.entryGun[e] = p.content[e].blueprint != 0;
        r.replaceAll = p.replaceAll;
        r.unspawnOrStatic = p.unspawnOnPickup || p.contentIsStatic;
        r.kind = p.kind;
        if (const Box* b = BoxOf(c.blueprint)) {
            std::memcpy(r.boxMin, b->min, sizeof(r.boxMin));
            std::memcpy(r.boxMax, b->max, sizeof(r.boxMax));
            r.boxLearned = true;
        } else {
            std::memcpy(r.boxMin, kRifleBoxMin, sizeof(r.boxMin));
            std::memcpy(r.boxMax, kRifleBoxMax, sizeof(r.boxMax));
        }
        ++kept;
    }
    return kept;
}

uint32_t RequestTake(uint32_t entity, uint32_t blueprint, int side, const char** why) {
    auto no = [why](const char* reason) -> uint32_t {
        if (why)
            *why = reason;
        return 0;
    };
    if (!g_live.load(std::memory_order_relaxed))
        return no(g_levelNow == 0 ? "pickups off" : "not hooked");
    if (side >= 0 && g_levelNow < 2)
        return no("a hand takes none at level 1");
    if (!g_haveTick)
        return no("the tick is stale");
    if (g_waiting)
        return no("a request already waiting");
    const Pickup* p = nullptr;
    for (int i = 0; g_haveCensus && i < g_census.count; ++i)
        if (g_census.pickup[i].entity == entity)
            p = &g_census.pickup[i];
    if (!p || p->first < 0 || p->content[p->first].blueprint != blueprint)
        return no("not in the census");
    if (p->keepsNoPlace) {
        if (SaidOnce(g_saidNoPlace, std::size(g_saidNoPlace), entity))
            Log("Pickups: the %s would take your gun's place for good (the game keeps no place for it): hold X to take it the game's way",
                Nice(p->content[p->first].name));
        return no("it would take your gun's place for good");
    }
    // The rounds the guns that go into it carry: the mod's counts. Every slot the content names that the soldier holds; with ReplaceAllContent,
    // every slot the soldier holds. The first two always: the game may put the taken gun in the one the content doesn't name (a dead soldier's AN-94
    // naming slot 0 went into 1, 2026-11-13), and the gun there would go with its hidden round in it. One that stays gets its count back (ServeAsk).
    Ask a;
    for (int s = 0; s < kMostSlots; ++s) {
        bool named = p->replaceAll || s == 0 || s == 1;
        for (int c = 0; c < p->contents && !named; ++c)
            named = p->content[c].slot == s;
        if (!named || !g_tick.slots[s].weapon)
            continue;
        HandOver h;
        h.slot = s;
        if (ammo::HandOverCount(s, h.firing, h.loaded) && a.counts < kMostSlots)
            a.count[a.counts++] = h;
    }
    a.serial = ++g_askSerial;
    a.entity = entity;
    a.blueprint = blueprint;
    a.side = side;
    a.at = GetTickCount64();
    g_takeOpen.store(true, std::memory_order_relaxed);  // before it's posted: ammo writes nothing to the player's weapons from now
    {
        std::lock_guard lock(g_askMutex);
        g_ask = a;
        g_askPosted.store(a.serial, std::memory_order_release);
    }
    g_waiting = a.serial;
    g_waitingAt = a.at;
    if (why)
        *why = "";
    return a.serial;
}

int Outcome(uint32_t request, const char** why) {
    if (why)
        *why = "";
    if (!request)
        return -1;
    if (request == g_waiting)
        return 0;
    for (const auto& o : g_outcomes)
        if (o.serial == request) {
            if (why)
                *why = o.why;
            return o.result;
        }
    if (why)
        *why = "no such request";
    return -1;
}

bool NextSwap(uint32_t after, Swap& out) {
    const Swap* best = nullptr;
    for (const Swap& s : g_ring.swap)
        if (s.serial > after && (!best || s.serial < best->serial))
            best = &s;
    if (!best)
        return false;
    out = *best;
    return true;
}

uint32_t NewestSwap() {
    return g_ring.newest;
}

void SwapDrawn(const Swap& swap, float* out12) {
    if (!out12)
        return;
    if (pickrules::DrawnPose(swap.world, swap.taken.offset, swap.taken.rotation, out12))
        return;
    // (Not finite: its transform as it is, turned not at all.)
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c)
            out12[4 * r + c] = r == c ? 1.0f : 0.0f;
        out12[4 * r + 3] = std::isfinite(swap.world[12 + r]) ? swap.world[12 + r] : 0.0f;
    }
}

int GameWouldTake() {
    if (!g_live.load(std::memory_order_relaxed) || !g_haveTick)
        return -1;
    if (g_tick.gameTarget)
        return 1;
    // A use point on the reload key, on and not used up (R would use it: the AT4's hand-over), not a flag.
    const Tick& t = g_tick;
    return t.useEntity && t.useAction == kUseReload && t.useKind != kUseFlagKind && t.useEnabled && (!t.useMaxUses || t.useUses < t.useMaxUses) ? 2 : 0;
}

bool UseTarget(UsePoint& out) {
    out = UsePoint{};
    if (!g_live.load(std::memory_order_relaxed) || !g_haveTick || !g_tick.useEntity)
        return false;
    const Tick& t = g_tick;
    out.entity = t.useEntity, out.action = t.useAction, out.kind = t.useKind, out.hold = t.useHold, out.maxUses = t.useMaxUses, out.uses = t.useUses;
    out.enabled = t.useEnabled, out.radius = t.useRadius, out.haveWorld = t.useHaveWorld;
    std::memcpy(out.world, t.useWorld, sizeof(out.world));
    std::memcpy(out.sid, t.useSid, sizeof(out.sid));
    out.at = t.at;
    return true;
}

bool UseCodeReady() {
    return g_useState.load(std::memory_order_relaxed) == 2 && !g_useDead.load(std::memory_order_relaxed);
}

bool UseByKey() {
    return g_useState.load(std::memory_order_relaxed) == 1 && !g_useDead.load(std::memory_order_relaxed);
}

bool UseByHandOn() {
    const Tune t = Tuning();
    return t.byHand && t.handUse;
}

uint32_t RequestUse(uint32_t entity, int side, const char** why) {
    const char* no = nullptr;
    if (!g_live.load(std::memory_order_relaxed) || !g_haveTick)
        no = "pickups off, not hooked, or the game's thread not heard from";
    else if (!UseByHandOn())
        no = "a hand's use of a prompt is off (pickups.txt \"use 0\", or \"level 1\")";
    else if (g_useDead.load(std::memory_order_relaxed))
        no = "the game's use faulted earlier";
    else if (!UseCodeReady())
        no = "the game's use isn't as read";
    else if (!entity || g_tick.useEntity != entity)
        no = "the game doesn't target it now";
    else if (!usepoint::ByHand(g_tick.useAction, g_tick.useKind, g_tick.useHold, g_tick.useRadius))
        no = "not a prompt a hand uses";
    if (no) {
        if (why)
            *why = no;
        return 0;
    }
    std::lock_guard lock(g_useMutex);  // (the game's thread holds it only to take an ask)
    const ULONGLONG now = GetTickCount64();
    const bool open = g_useAsk.serial && !g_useAsk.dropped && g_useDoneSerial.load(std::memory_order_acquire) < g_useAsk.serial;
    if (open && now - g_useAsk.at <= (g_useAsk.taken ? kUseServedMostMs : kUnansweredMs)) {
        if (why)
            *why = "another use waits";
        return 0;
    }
    if (open && !g_useAsk.taken)
        g_useAsk.dropped = true;  // (stale: given up, the game's thread passes it by)
    g_useAsk = UseAsk{g_useAsk.serial + 1, entity, side, now, false, false};
    g_useAskPosted.store(g_useAsk.serial, std::memory_order_release);
    if (why)
        *why = "";
    return g_useAsk.serial;
}

int UseOutcome(uint32_t serial, const char** why) {
    if (why)
        *why = "";
    if (!serial)
        return -1;
    // Served: what the game's thread said right after it (never a stale tick's).
    if (const uint32_t done = g_useDoneSerial.load(std::memory_order_acquire); done >= serial) {
        if (done > serial) {
            if (why)
                *why = "a later use was served";
            return -1;
        }
        const char* said = g_useDoneWhy.load(std::memory_order_relaxed);
        if (const DWORD fault = g_useFaultCode.load(std::memory_order_relaxed); fault && why) {
            static char s_fault[64];
            snprintf(s_fault, sizeof(s_fault), "%s (%08lX)", said, static_cast<unsigned long>(fault));
            said = s_fault;
        }
        if (why)
            *why = said;
        return g_useDoneResult.load(std::memory_order_relaxed);
    }
    // Not yet: taken (being served: wait, kUseServedMostMs at most), or not taken within kUnansweredMs (given up). Never waited on: the game's thread
    // holds the lock only to take it.
    std::unique_lock lock(g_useMutex, std::try_to_lock);
    if (!lock.owns_lock())
        return 0;
    if (g_useAsk.serial != serial) {
        if (why)
            *why = "not answered (another use asked since)";
        return -1;
    }
    const ULONGLONG age = GetTickCount64() - g_useAsk.at;
    if (g_useAsk.dropped || (!g_useAsk.taken && age > kUnansweredMs)) {
        g_useAsk.dropped = true;  // (the game's thread passes it by)
        if (why)
            *why = "not answered in 1 s";
        return -1;
    }
    if (g_useAsk.taken && age > kUseServedMostMs) {
        if (why)
            *why = "taken by the game's thread, its outcome not heard in 5 s";
        return -1;
    }
    return 0;
}

bool GameTargetTake(int& entries, int* entrySlot, bool* entryGun, bool& replaceAll, bool& unspawnOrStatic, const char** name, int* entriesAll, Kind* kind) {
    entries = 0, replaceAll = unspawnOrStatic = false;
    if (name)
        *name = "";
    if (entriesAll)
        *entriesAll = 0;
    if (kind)
        *kind = kNoKind;
    if (!g_live.load(std::memory_order_relaxed) || !g_haveTick || !g_tick.gameTarget || !entrySlot || !entryGun)
        return false;
    entries = std::min(std::max(g_tick.gameTargetEntries, 0), kMostContent);
    for (int e = 0; e < entries; ++e)
        entrySlot[e] = g_tick.gameTargetSlot[e], entryGun[e] = g_tick.gameTargetGun[e];
    replaceAll = g_tick.gameTargetReplaceAll;
    unspawnOrStatic = g_tick.gameTargetUnspawnOrStatic;
    if (entriesAll)
        *entriesAll = g_tick.gameTargetEntriesAll;
    if (kind)
        *kind = g_tick.gameTargetKind;
    if (name)
        *name = g_tick.gameTargetName[0] ? Nice(g_tick.gameTargetName) : "dropped gun";
    return true;
}

int HoldsGun(uint32_t entity, uint32_t blueprint, float* distance, bool* reach) {
    if (distance)
        *distance = -1.0f;
    if (reach)
        *reach = false;
    if (!g_live.load(std::memory_order_relaxed) || !g_haveCensus || !entity)
        return -2;
    for (int i = 0; i < g_census.count; ++i) {
        const Pickup& p = g_census.pickup[i];
        if (p.entity != entity)
            continue;
        if (distance)
            *distance = p.distance;
        bool holds = false;
        for (int c = 0; c < p.contents && !holds; ++c)
            holds = blueprint && p.content[c].blueprint == blueprint;
        if (reach)
            *reach = holds && p.allowed && p.eligible && !p.keepsNoPlace;
        return holds ? 1 : 0;
    }
    return -1;
}

// Only within the game's own reach of that pickup (holsters' kFar otherwise; the review of 907b2b6, finding 2, read in bf3_mem.exe 2026-11-18): its
// check 008B6380 takes the pickup's place from one thing alone, the server entity's transform (vtable +0x58 005390D0 -> +0x24 01191130, the same
// matrix ReadTransform reads) and a fixed box round it (+0x5C 00891490: -0.5 to 0.5, 020839A0 / 0206EF50: its middle the transform's translation),
// within InteractionRadius (00891530: data +0x90) of the interaction component's own point (008AF440), the line to 0.5 m over it clear (00A996E0).
// Moving the pickup to the soldier for the take is not one write: the pickup's own update (008A1DD0 -> 0089FB90) copies its Havok body's pose
// (+0x120, 005C17C0) into that transform every frame and marks it for the client (0118CBD0, 0170F140 bit 1), so a lone write leaves the server's
// pickup where the client doesn't draw it (or is undone a frame on); and inside the take, with the place moved, the pickup raises an event on its
// bus (008ABE9A, 00548EE0: [entity +0x10] vtable +0x18, hash 9285C8F0), whose handlers are the level's data. Not done: the player walks back within
// 2.5 m.
uint32_t RequestGiveBack(uint32_t entity, uint32_t blueprint, int side, const char** why, float* distance) {
    if (distance)
        *distance = -1.0f;
    const int holds = HoldsGun(entity, blueprint, distance);
    if (holds != 1) {
        if (why)
            *why = holds == 0 ? "it holds another gun now" : holds == -1 ? "not in the census" : "no pickups seen now";
        return 0;
    }
    const Pickup* p = nullptr;
    for (int i = 0; i < g_census.count && !p; ++i)
        if (g_census.pickup[i].entity == entity)
            p = &g_census.pickup[i];
    if (!p || p->first < 0) {
        if (why)
            *why = "nothing in it";
        return 0;
    }
    return RequestTake(entity, p->content[p->first].blueprint, side, why);
}

bool WhyNot(const float* pointWorld, float within, const char** name, const char** why, float* distance) {
    if (!g_haveCensus || !pointWorld)
        return false;
    const Pickup* best = nullptr;
    float bestDistance = 0.0f;
    for (int i = 0; i < g_census.count; ++i) {
        const float d = Distance(g_census.pickup[i].world + 12, pointWorld);
        if (d <= within && (!best || d < bestDistance))
            best = &g_census.pickup[i], bestDistance = d;
    }
    if (!best)
        return false;
    const char* reason = !best->allowed ? "the game doesn't allow it now"
                         : best->first < 0 ? "nothing in it"
                         : best->keepsNoPlace ? "it would take your gun's place for good"
                         : !best->eligible ? "the game wouldn't let you take it from there"
                                           : "";
    if (name)
        *name = best->first >= 0 ? Nice(best->content[best->first].name) : "dropped gun";
    if (why)
        *why = reason;
    if (distance)
        *distance = bestDistance;
    return true;
}

int SlotOf(const char* blueprintName) {
    if (!g_haveTick || !blueprintName || !blueprintName[0])
        return -1;
    for (int s = 0; s < kMostSlots; ++s)
        if (g_tick.slots[s].weapon && g_tick.slots[s].name[0] && recoilguns::SameName(g_tick.slots[s].name, blueprintName))
            return s;
    return -1;
}

int ActiveSlot() {
    return g_file.slots && g_haveTick ? g_tick.activeSlot : -1;
}

Tune Tuning() {
    Tune t = g_file.tune;
    t.byHand = g_levelNow >= 2;
    return t;
}

const char* Nice(const char* blueprintName) {
    static char ring[8][kName];
    static int next = 0;
    if (!blueprintName)
        return "";
    char* out = ring[next];
    next = (next + 1) % 8;
    if (const recoilguns::Row* row = recoilguns::ByName(blueprintName))
        CopyName(out, row->name);
    else {
        const char* slash = std::strrchr(blueprintName, '/');
        CopyName(out, slash ? slash + 1 : blueprintName);
    }
    return out;
}

}  // namespace pickups

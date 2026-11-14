// A dropped gun's attachments where it lies (pickparts.h). The game's builder of a client pickup's body (00854B50), its setTransform (00841680), its
// destructor (00857FF0) and the pickup renderer's collector (00797FF0) are hooked once build\pickups.txt has said "parts 1" or more and every piece
// of the game's code this goes through is as read. Inside the builder, on the game's thread: the entry the game draws (011BB9D0's two passes, read),
// its unlocks (the set at entry+4), the gun's sockets for them, each part's mesh made with the game's own calls (the pickup's allocator, 0101AA90,
// 01011D30), placed with 0100E820 on the pose the game just placed its body on (0084CC50) times the part's matrix from the gun's own data. That
// placing only keeps the mesh loaded (0101AA90's instance is an entry of the MeshStreamingManager [023B3E84], not a draw); what draws is the
// collector: after the game's own pass over the pickups it was handed, it's handed once more one stand-in entity per part of each of them, the
// part's instance at +0x110 and at +0x1C a matrix made in that same call from the pose the game draws the body on now (0084A7C0, as 00854960 there
// takes it), so the game's own code draws the parts every frame, on the gun as it is. The game's thread writes what the collector reads into rows
// of its own (a seqlock), and frees an instance only once the row is written without it and no collector's call holds it. The destructor frees
// them first, with the game's own pair (010158A0, 00425930), only while the scene and the allocator are the ones they were made with (else forgotten,
// counted). The render thread reads only: the census in the log, the counts. Every address is a VA in bf3_mem.exe; they're all in one table (g_at),
// so tools/pickpartstest.cpp can point them at memory and functions of its own.

#include "pickparts.h"

#include <windows.h>
#ifndef BF3VR_PICKUPS_TEST
#include <MinHook.h>  // (the tests give their own)
#endif

#include <algorithm>
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <iterator>

#include "camera.h"
#include "log.h"
#include "pickpartsrules.h"

namespace pickparts {
namespace {

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// The game's.

struct Addresses {
    uintptr_t build = 0x00854B50, move = 0x00841680, gone = 0x00857FF0, collect = 0x00797FF0;  // hooked
    uintptr_t pose = 0x0084CC50, alloc = 0x0041BE10, construct = 0x0101AA90, enter = 0x01011D30, place = 0x0100E820, leave = 0x010158A0,
              release = 0x00425930;
    uintptr_t poseOf = 0x0084A7C0;  // the pose from a given entry (0084CC50's second call; the collector's own through 00854960)
    uintptr_t scene = 0x023B3E84;   // the scene the instances are entered into (01011D33)
    uintptr_t lights = 0x023B3F94;  // the light handles' system the collector reads a pickup's +0x166 in (007981AF, 01744260)
    uintptr_t clientWorld = 0x023813A0, clientWorldVt = 0x0213C578, clientPickupVt = 0x020B8DF0, clientPickupType = 0x023D93AC;
    uintptr_t socketObjectRange = 0x023F5098, meshRange = 0x023D1B04;
    Image image;  // the image's code and read-only data, from its section headers (pickpartstest gives its own)
};
Addresses g_at;
constexpr uintptr_t kImageBase = 0x00400000;

// A client pickup's (00854B50, 00856330): its data (UseWeaponMesh at +0x9D, its own mesh at +0x7C), the holder of its allocator and world
// ([[+0x10]+4]: +0x34 the allocator 00545DC0 reads, +0x38 the world 0101AA90 is given), its body instance, its transform.
constexpr uint32_t kPickupData = 0x14, kUseWeaponMesh = 0x9D, kOwnMesh = 0x7C, kPickupHolder = 0x10, kHolderInner = 4, kInnerArena = 0x34,
                   kInnerWorld = 0x38, kBody = 0x110, kTransform = 0x1C, kFlags = 8;
// ... and what slot 47 and the collector read of it besides (00844560, 007981A8): the body shown, hidden in a fly-in, its light's handle.
constexpr uint32_t kShown = 0x164, kHiding = 0x165, kLightHandle = 0x166;
static_assert(kShown == kStandinShown && kHiding == kStandinHiding && kLightHandle == kStandinHandle && kBody == kStandinBody &&
                  kTransform == kStandinTransform && kFlags == kStandinFlags,
              "a stand-in is laid out as a client pickup");
// A mesh instance (0101AA90, 01017DF0): +0 its mesh holder (+0x10 the MeshSet, its name at +0x58), +8 the renderer's mesh, +0x24 its scene handle.
constexpr uint32_t kInstanceSize = 0x2C, kInstanceAlign = 0x10, kInstanceHolder = 0, kHolderSet = 0x10, kSetName = 0x58, kInstanceMesh = 8,
                   kInstanceHandle = 0x24;
constexpr uint16_t kNoHandle = 0xFFFF;
// The client world's lists (as pickups.cpp walks them).
constexpr uint32_t kWorldLists = 0x820, kWorldListsEnd = 0x824, kWorldFirstClass = 0x834, kSegmentNext = 0x18, kSegmentActive = 0x1C, kClassIdAt = 0x14;
constexpr int kMostSegments = 64;

struct Code {
    uintptr_t at;
    uint32_t size;
    uint64_t fnv;
    const char* what;
};
// Every function it hooks or calls, and every one those call that it depends on, as bf3_mem.exe has them: the FNV-1a 64 of their bytes, each
// to its last ret (or its tail jump). 011BB9D0 and 00545DC0 are read the same way by the mod, never called.
constexpr Code kCode[] = {
    {0x00854B50, 446, 0x7E984CE546619ED9ull, "the builder of a dropped gun's body"},
    {0x00857FF0, 167, 0x3605BF7DDF499CBEull, "the client pickup's destructor"},
    {0x00841680, 78, 0x8D41C406BC21B92Full, "the client pickup's setTransform"},
    {0x00856330, 307, 0x72B4A4E6D4B9A2ABull, "the client pickup's init"},
    {0x0101AA90, 80, 0x2F3AACA154CC7468ull, "the mesh instance's constructor"},
    {0x01017DF0, 172, 0x0F820118CCDF9C30ull, "the mesh instance's set-up"},
    {0x004BC8C0, 17, 0xB1DFE21C2F347BBDull, "the mesh instance's base and the release's tail (004BC8D0)"},
    {0x01011D30, 45, 0xB8E9DF625C544EA4ull, "the mesh instance's entry into the scene"},
    {0x0100E820, 54, 0x539B56348069F37Dull, "the mesh instance's placement"},
    {0x010158A0, 109, 0x537B2B4053FC3738ull, "the mesh instance's release"},
    {0x0173C100, 547, 0x56DD037184FDB6D6ull, "the scene's add"},
    {0x0173E410, 161, 0x7B12A442BFF4AF40ull, "the scene's remove"},
    {0x00425930, 64, 0x334E4F6602627EDAull, "the free by pointer"},
    {0x0041BE10, 98, 0x60B49AC5C5316BDAull, "the allocator's aligned allocation"},
    {0x00545DC0, 11, 0x70D3EFC67D5579DEull, "the pickup's allocator"},
    {0x011BB9D0, 209, 0xD6E9EF98B1E86337ull, "the entry the body is drawn from"},
    {0x01235800, 4, 0x1DC13EF6FFD54D40ull, "the blueprint's gun data"},
    {0x0084CC50, 93, 0x6951EB4951803959ull, "the client's pose of a dropped gun"},
    // What those it calls call in turn, one level down (the scene's add and remove above are of this level too).
    {0x01736520, 289, 0x3DAA2C57FE994796ull, "the placement's call into the scene (0100E84E)"},
    {0x004CA210, 95, 0x59AC632C57EDF9EDull, "the set-up's first call (01017E16)"},
    {0x004C1420, 22, 0x51F61410205E1774ull, "the set-up's and the release's reset (01017E1D, 010158FC)"},
    {0x017398C0, 117, 0x7A1DA57DF921C5F0ull, "the set-up's call at 01017E30"},
    {0x00988400, 89, 0x17825FAC6AA990EFull, "the set-up's call at 01017E39"},
    {0x00484030, 1, 0xAF647E4C8602FFD2ull, "the release's call at 010158DB"},
    {0x00503D00, 50, 0xED130D9E754290CEull, "the free's lookup (0042593F)"},
    {0x00418290, 80, 0x1B793DCAA4C63A7Dull, "the free's call at 0042595E"},
    {0x004249A0, 78, 0x9718FE7E0FA3E615ull, "the free's call at 00425966"},
    // The pickup renderer's collector the stand-ins are handed to, and what it calls for them: a client pickup's slots 47 and 23, its draw
    // record's allocation and entry into the frame's lists, the frame's memory, a pickup's light, the composite record's set-up and its parts.
    {0x00797FF0, 2377, 0x2E79C444D3BA4EFAull, "the pickup renderer's collector"},
    {0x00844560, 28, 0xF02A1D7994731934ull, "the client pickup's slot 47 (its body)"},
    {0x008444C0, 160, 0xF007AAC779A75ADDull, "the client pickup's slot 23 (its box)"},
    {0x017A1FA0, 49, 0x4EC7FB1C9F54F6B5ull, "the collector's draw record (007980CD, 00798398, 0079867F)"},
    {0x0179BD00, 302, 0x4EA5149E604AFF11ull, "the draw record's entry into the frame's lists (017A1FC5)"},
    {0x0050F0B0, 150, 0x63FEC8507D6DE0DDull, "the frame's memory (017A1FB1, 01744287, 01015994)"},
    {0x01744260, 92, 0x1B254AC3C9E5C9EFull, "a pickup's light from its handle (007981BA, 0079845C, 00798760)"},
    {0x00793BD0, 87, 0x3D1B8D3FDF8DF437ull, "the composite draw record's set-up (007980DA)"},
    {0x01015950, 260, 0x49A091F60F46EA2Bull, "the composite draw record's parts (0079837B)"},
    {0x0127DE70, 31, 0x32FC9911E876E449ull, "the frame's lists' views (0179BD58)"},
    {0x00854920, 56, 0x55B6B47097D98314ull, "the body's bone count (0079843F)"},
    {0x00854960, 483, 0xE038B750507C5112ull, "the body's bone matrices (00798631)"},
    // The pose from an entry, called on the collector's thread (as 00854960 calls it there), and what it calls.
    {0x0084A7C0, 1150, 0x2872C71DD00A31F2ull, "the client's pose of a dropped gun from its entry"},
    {0x0050D510, 422, 0xB0383413D2197D8Dull, "the pose's blend (0084A846)"},
    {0x00428D30, 180, 0xE800ED5DDC443DB3ull, "the pose's call at 0084A8C8"},
    {0x00557AA0, 176, 0xCFF8C1DB9080AA27ull, "the pose's call at 0084A9F2"},
    {0x00557B50, 183, 0x2187A4195CBCA117ull, "the pose's call at 0084AB1F"},
};
// Byte for byte as well: where it hooks (MinHook's jump goes over the first 5 or 6 bytes), and the content read's call of the builder.
constexpr uint8_t kBuildStart[] = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0, 0x83, 0xEC, 0x44, 0x53, 0x56, 0x8B};
constexpr uint8_t kMoveStart[] = {0x56, 0x57, 0x8B, 0x7C, 0x24, 0x0C, 0x57, 0x8B, 0xF1, 0xE8, 0x32, 0xFB};
constexpr uint8_t kGoneStart[] = {0x53, 0x55, 0x56, 0x8B, 0xF1, 0x8B, 0x9E, 0x10, 0x01, 0x00, 0x00, 0x57};
// The collector's: push ebp / mov ebp,esp / and esp,0FFFFFFF0h / sub esp,84h (four whole instructions, no relative jump among them).
constexpr uint8_t kCollectStart[] = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0, 0x81, 0xEC, 0x84, 0x00, 0x00, 0x00};
constexpr uint8_t kContentCall[] = {0x8B, 0x4F, 0xB4, 0x80, 0xB9, 0x9D, 0x00, 0x00, 0x00, 0x00, 0x74, 0x08, 0x8D, 0x4F, 0xA0, 0xE8, 0x35, 0xBF, 0xFF, 0xFF};
struct Bytes {
    uintptr_t at;
    const uint8_t* bytes;
    uint32_t size;
    const char* what;
};
constexpr Bytes kBytes[] = {
    {0x00854B50, kBuildStart, sizeof(kBuildStart), "the builder's start"},
    {0x00841680, kMoveStart, sizeof(kMoveStart), "setTransform's start"},
    {0x00857FF0, kGoneStart, sizeof(kGoneStart), "the destructor's start"},
    {0x00797FF0, kCollectStart, sizeof(kCollectStart), "the collector's start"},
    {0x00858C07, kContentCall, sizeof(kContentCall), "the content read's call of the builder"},
};
// The data it goes by: the client pickup's vtable slots, the field records of the gun's data (their offsets), the getters' forms, the empty name.
struct Word {
    uintptr_t at;
    uint32_t value;
    uint32_t size;  // 1, 2 or 4
    const char* what;
};
constexpr Word kWords[] = {
    {0x020B8E10, 0x008583A0, 4, "the client pickup's slot 8 (its deleting destructor)"},
    {0x020B8E44, 0x00841680, 4, "the client pickup's slot 21 (setTransform)"},
    {0x020B8E88, 0x00856330, 4, "the client pickup's slot 38 (init)"},
    {0x02437594, 0x01340048, 4, "SoldierWeaponData.Sockets at +0x134"},
    {0x0243706C, 0x00600048, 4, "SoldierWeaponData.WeaponStates at +0x60"},
    {0x0243733C, 0x00100000, 4, "WeaponStateData.Mesh3p at +0x10"},
    {0x02437414, 0x00440048, 4, "WeaponStateData.Mesh3pTransforms at +0x44"},
    {0x02437420, 0x00480048, 4, "WeaponStateData.Mesh3pRigidMeshSocketObjectTransforms at +0x48"},
    {0x02430178, 0x00900000, 4, "SocketData.UnlockAsset at +0x90"},
    {0x02430184, 0x00980000, 4, "SocketData.BoneName at +0x98"},
    {0x024301A8, 0x009C0048, 4, "SocketData.AvailableObjects at +0x9C"},
    {0x024301CC, 0x00A80000, 4, "SocketData.UsesDefaultObject at +0xA8"},
    {0x0243016C, 0x00A90000, 4, "SocketData.Excluded at +0xA9"},
    {0x0243010C, 0x00140000, 4, "WeaponSocketObjectData.Asset3p at +0x14"},
    {0x02430130, 0x00200000, 4, "WeaponRegularSocketObjectData.Transform at +0x20"},
    {0x02435DFC, 0x00400000, 4, "RigidMeshSocketTransform.SocketObject at +0x40"},
    {0x0206A5D0, 0x00, 1, "the empty bone name"},
    {0x020AE204, 0x00797FF0, 4, "the pickup renderer's slot 9 (its collector)"},
    {0x020AE1F0, 0x0078E360, 4, "the pickup renderer's slot 4 (its type)"},
    {0x0078E379, 0x023D93AC, 4, "the pickup renderer's type: the client pickup's (0078E373)"},
    {0x020B8EAC, 0x00844560, 4, "the client pickup's slot 47 (its body)"},
    {0x020B8E4C, 0x008444C0, 4, "the client pickup's slot 23 (its box)"},
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

using BuildFn = void(__fastcall*)(void* pickup, void* edx);
using MoveFn = void(__fastcall*)(void* pickup, void* edx, const float* m);
using GoneFn = void(__fastcall*)(void* pickup, void* edx);
using PoseFn = float*(__fastcall*)(void* pickup, void* edx, float* out);
using AllocFn = void*(__fastcall*)(void* arena, void* edx, uint32_t size, uint32_t align);
using ConstructFn = void*(__fastcall*)(void* memory, void* edx, void* mesh, int one, void* world, void* none);
using EnterFn = void(__fastcall*)(void* instance, void* edx, int zero);
using PlaceFn = void(__fastcall*)(void* instance, void* edx, const float* m, float scale);
using LeaveFn = void(__fastcall*)(void* instance, void* edx);
using ReleaseFn = void(__cdecl*)(void* p);
// The collector (thiscall, ret 10h; it never reads ecx): the elements' count, the elements, a word it doesn't read, the frame's records.
using CollectFn = void(__fastcall*)(void* self, void* edx, uint32_t count, const uint32_t* elements, void* unused, void* records);
// 0084A7C0 (thiscall, ret 8): the pickup, the entry, the 16-byte aligned matrix it fills.
using PoseOfFn = void(__fastcall*)(void* pickup, void* edx, uint32_t entry, float* out);

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// Its own numbers and what it holds.

constexpr int kPerPickup = 8, kMostLive = 128, kMostRecords = 64, kMostMoved = 64, kCatchUpEach = 4, kMostWalked = 64;
constexpr ULONGLONG kCensusMs = 1000, kCatchUpMs = 1000, kCountsMs = 1000, kCountsRepeatMs = 10000;

// One part made: its matrix on the body, its instance, what it was made of, the scene and the allocator it was made with, its MeshSet's kind, and
// whether it's whole (made, loaded, in the scene: only then in the row the collector reads).
struct alignas(16) Held {
    float rel[16];
    uint32_t instance, unlock, object, asset, scene, arena;
    uint32_t kind, ready;
};
// One pickup's parts: the base the game last placed its body on, the body (its instance and MeshSet), the key of what was wanted, the world.
struct alignas(16) Record {
    float base[16];
    Held part[kPerPickup];
    uint32_t pickup, entry, blueprint, body, bodySet, world;
    uint64_t key;
    int count, level;
    DWORD thread;
    ULONGLONG madeAt;
};
static_assert(offsetof(Held, rel) % 16 == 0 && sizeof(Held) % 16 == 0, "a part's matrix 16-byte aligned");
static_assert(offsetof(Record, base) % 16 == 0 && offsetof(Record, part) % 16 == 0 && sizeof(Record) % 16 == 0, "a pickup's matrices 16-byte aligned");
// A pickup moved before it had a record: the matrix the game placed its body on.
struct alignas(16) Moved {
    float m[16];
    uint32_t pickup;
    uint32_t pad[3];
};
static_assert(offsetof(Moved, m) % 16 == 0 && sizeof(Moved) % 16 == 0, "a moved matrix 16-byte aligned");

// Touched only under g_lock, on the game's threads (the render thread never takes it).
alignas(16) Record g_records[kMostRecords];
alignas(16) Moved g_moved[kMostMoved];
int g_movedNext = 0;
CRITICAL_SECTION g_lock;
std::atomic<bool> g_lockMade{false};
ULONGLONG g_catchUpAt = 0;
int g_catchUpNext = 0;  // where in the client world's list the last catch-up pass stopped

// Shared: the level in force, the hooks' state, the counts (the game's threads write, the render thread reads).
std::atomic<int> g_level{0};
std::atomic<bool> g_hooked{false}, g_noMakes{false};
// Set when the switch goes below 1: moves and builds go unseen from then on, so the moved table is emptied at the game's thread's next record work.
std::atomic<bool> g_movedStale{false};
std::atomic<int> g_live{0}, g_pickupsWithParts{0};
std::atomic<uint32_t> g_made{0}, g_freed{0}, g_refused{0}, g_notLoaded{0}, g_noSlot{0}, g_noScene{0}, g_misaligned{0}, g_faults{0},
    g_forgotten{0}, g_builds{0}, g_moves{0}, g_gones{0}, g_caughtUp{0}, g_drained{0}, g_noAlloc{0};
std::atomic<uint32_t> g_mostMicros{0};
std::atomic<DWORD> g_buildThread{0}, g_moveThread{0}, g_goneThread{0}, g_tickThread{0}, g_faultCode{0};
std::atomic<uint32_t> g_goneScene{0}, g_goneArena{0}, g_movedPickups{0};
// A part's mesh not loaded, said once (the render thread says it): the asset.
std::atomic<uint32_t> g_unloadedAsset{0};

BuildFn g_build = nullptr;
MoveFn g_moveOriginal = nullptr;
GoneFn g_goneOriginal = nullptr;
CollectFn g_collectOriginal = nullptr;

// What the collector's calls read and hold. One row per record (the same index), written by the game's thread under g_lock, read with no lock.
static_assert(kPerPickup == kRowParts, "a row holds all of a record's parts");
SharedRow g_rows[kMostRecords];
std::atomic<int> g_rowsLive{0};  // rows with parts (while there are none, a call looks no further)
// Each call's holds, and its stand-ins, their matrices, the pose and the elements (static: the game keeps none of it past the call's return).
CallHolds g_holds[kCallSlots];
struct alignas(16) CallMemory {
    float pose[16];
    float world[kDrawEach][16];
    uint8_t standin[kDrawEach][kStandinSize];
    uint32_t element[kDrawEach][kElementWords];
};
static_assert(offsetof(CallMemory, world) % 16 == 0 && offsetof(CallMemory, standin) % 16 == 0 && sizeof(CallMemory) % 16 == 0,
              "a call's matrices 16-byte aligned");
alignas(16) CallMemory g_callMemory[kCallSlots];
std::atomic<bool> g_drawOn{true};  // false for the session after a fault on the collector's side
std::atomic<uint32_t> g_calls{0}, g_callsDrawn{0}, g_standins{0}, g_mostElements{0}, g_atOnce{0}, g_mostAtOnce{0}, g_noCall{0}, g_rowBusy{0},
    g_overCap{0}, g_notAsMade{0}, g_refusedKind{0}, g_drawFaults{0}, g_heldWaits{0}, g_heldForgotten{0}, g_drawFaultCode{0};
std::atomic<DWORD> g_collectThread[4] = {};
// Each part made, for the census (the render thread says them): a ring the game's thread writes under g_lock, each note's number its version.
struct MadeNote {
    std::atomic<uint32_t> number{0}, pickup{0}, instance{0}, asset{0}, handle{0}, kind{0}, x{0}, y{0}, z{0};
};
MadeNote g_notes[16];
std::atomic<uint32_t> g_notesMade{0};

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// Reads.

bool ReadGame(void*, uint32_t at, void* out, uint32_t size) {
    return camera::SafeRead(at, out, size);
}
const Reader kRead{&ReadGame, nullptr};

uint32_t U32(uint32_t at) {
    uint32_t v = 0;
    kRead.U32(at, v);
    return v;
}

Classes ClassCells() {
    Classes c;
    c.socketObject = static_cast<uint32_t>(g_at.socketObjectRange);
    c.mesh = static_cast<uint32_t>(g_at.meshRange);
    return c;
}

// The allocator and the world the game makes a pickup's body with ([[[pickup+0x10]+4]+0x34] as 00545DC0 reads it, and +0x38: 00854C04-00854C25).
uint32_t ArenaOf(uint32_t pickup) {
    const uint32_t holder = U32(pickup + kPickupHolder);
    const uint32_t inner = holder ? U32(holder + kHolderInner) : 0;
    return inner ? U32(inner + kInnerArena) : 0;
}
uint32_t WorldOf(uint32_t pickup) {
    const uint32_t holder = U32(pickup + kPickupHolder);
    const uint32_t inner = holder ? U32(holder + kHolderInner) : 0;
    return inner ? U32(inner + kInnerWorld) : 0;
}
uint32_t SetOf(uint32_t instance) {
    const uint32_t holder = instance ? U32(instance + kInstanceHolder) : 0;
    return holder ? U32(holder + kHolderSet) : 0;
}
bool UsesWeaponMesh(uint32_t pickup) {
    uint32_t data = 0;
    uint8_t use = 0;
    return kRead.U32(pickup + kPickupData, data) && data && kRead.U8(data + kUseWeaponMesh, use) && use != 0;
}
// The raw transform (its +0x1C, or with bit 0x40000 of +8 the indexed one: 00854C82-00854C99).
bool RawTransform(uint32_t pickup, float* out16) {
    uint32_t at = 0, flags = 0;
    if (!kRead.U32(pickup + kTransform, at) || !at || !kRead.U32(pickup + kFlags, flags))
        return false;
    if (flags & 0x40000) {
        uint8_t index = 0;
        if (!kRead.U8(at + 6, index))
            return false;
        at = at + 0x10 + 0x50 * index;
    }
    return kRead.Bytes(at, out16, 64);
}

bool Aligned(const void* p) {
    return (reinterpret_cast<uintptr_t>(p) & 15) == 0;
}
uint32_t Addr(const void* p) {
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(p));
}
uint32_t Bits(float f) {
    uint32_t v = 0;
    std::memcpy(&v, &f, 4);
    return v;
}
float Float(uint32_t v) {
    float f = 0;
    std::memcpy(&f, &v, 4);
    return f;
}

void Atomic(std::atomic<uint32_t>& a) {
    a.fetch_add(1, std::memory_order_relaxed);
}
void Most(std::atomic<uint32_t>& a, uint32_t v) {
    uint32_t was = a.load(std::memory_order_relaxed);
    while (v > was && !a.compare_exchange_weak(was, v, std::memory_order_relaxed)) {
    }
}
void FirstThread(std::atomic<DWORD>& a) {
    if (!a.load(std::memory_order_relaxed))
        a.store(GetCurrentThreadId(), std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// The game's thread: the records (under g_lock).

Record* RecordOf(uint32_t pickup) {
    for (Record& r : g_records)
        if (r.pickup == pickup)
            return &r;
    return nullptr;
}
Record* NewRecord(uint32_t pickup) {
    for (Record& r : g_records)
        if (!r.pickup) {
            r = Record{};
            r.pickup = pickup;
            return &r;
        }
    return nullptr;
}
int CountWithParts() {
    int n = 0;
    for (const Record& r : g_records)
        n += r.pickup && r.count > 0;
    return n;
}
Moved* MovedOf(uint32_t pickup) {
    for (Moved& m : g_moved)
        if (m.pickup == pickup)
            return &m;
    return nullptr;
}
void ForgetMoved(uint32_t pickup) {
    if (Moved* m = MovedOf(pickup))
        m->pickup = 0;
}
void DropStaleMoved() {
    if (g_movedStale.exchange(false, std::memory_order_relaxed))
        for (Moved& m : g_moved)
            m.pickup = 0;
}

// The game's own release and free of one instance (00854BE2-00854BEF, 00858020-0085802D), guarded.
bool FreeInstance(uint32_t instance, DWORD& code) {
    __try {
        reinterpret_cast<LeaveFn>(g_at.leave)(reinterpret_cast<void*>(instance), nullptr);
        reinterpret_cast<ReleaseFn>(g_at.release)(reinterpret_cast<void*>(instance));
        return true;
    } __except (code = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// The record's row as the collector's calls are to read it: its whole parts of a kind the collector draws from a stand-in, with its pickup, body,
// world and the scene they were made in. A record holding none (or no pickup) gives an empty row. The game's thread, under g_lock.
void Publish(const Record& r) {
    const int at = static_cast<int>(&r - g_records);
    if (at < 0 || at >= kMostRecords)
        return;
    Row row;
    for (int i = 0; r.pickup && i < r.count && row.count < static_cast<uint32_t>(kRowParts); ++i) {
        const Held& h = r.part[i];
        if (!h.instance || !h.ready || !KindDrawn(h.kind))
            continue;
        if (!row.count)
            row.scene = h.scene;
        row.instance[row.count] = h.instance, row.kind[row.count] = h.kind;
        std::memcpy(row.rel[row.count], h.rel, sizeof(h.rel));
        ++row.count;
    }
    if (row.count)
        row.pickup = r.pickup, row.body = r.body, row.world = r.world;  // (a row with no parts names no pickup)
    const bool was = RowPickup(g_rows[at]) != 0;
    WriteRow(g_rows[at], row);
    if (was != (row.count > 0))
        g_rowsLive.fetch_add(row.count > 0 ? 1 : -1, std::memory_order_relaxed);
}

// Before an instance is freed, its row already written without it: no collector's call may hold it still. A call holds one only while the game's
// collector works on its stand-ins (microseconds); one held past 5 ms is let go of unfreed (forgotten, counted), never freed under a call.
uint32_t g_heldWaitMs = 5;  // (the tests lengthen it where a thread of theirs may be slow to let go)
bool Unheld(uint32_t instance) {
    if (!AnyHolds(g_holds, kCallSlots, instance))
        return true;
    Atomic(g_heldWaits);
    LARGE_INTEGER f{}, t0{}, t{};
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&t0);
    for (;;) {
        SwitchToThread();
        if (!AnyHolds(g_holds, kCallSlots, instance))
            return true;
        QueryPerformanceCounter(&t);
        if (!f.QuadPart || (t.QuadPart - t0.QuadPart) * 1000 > static_cast<LONGLONG>(g_heldWaitMs) * f.QuadPart) {
            Atomic(g_heldForgotten);
            return false;
        }
    }
}

// One part let go of: freed while the scene and the allocator are the ones it was made with (arenaNow: the pickup's now, 0 when it can't be known),
// else forgotten. Either way it's no longer held. Whoever calls this has written its row without it already (LetGoAll), or it was never in one.
void LetGo(Held& h, uint32_t arenaNow) {
    if (!h.instance)
        return;
    const uint32_t instance = h.instance;
    const uint32_t sceneNow = U32(static_cast<uint32_t>(g_at.scene));
    const bool same = h.scene && sceneNow == h.scene && arenaNow && arenaNow == h.arena;
    h.instance = 0;
    h.ready = 0;
    g_live.fetch_sub(1, std::memory_order_relaxed);
    if (!same || !Unheld(instance)) {
        Atomic(g_forgotten);
        return;
    }
    DWORD code = 0;
    if (FreeInstance(instance, code))
        Atomic(g_freed);
    else {
        Atomic(g_faults);
        g_faultCode.store(code, std::memory_order_relaxed);
        g_noMakes.store(true, std::memory_order_relaxed);
    }
}

// Every part of a record let go of (the record is emptied first: never freed twice; its row written empty before any is freed), the record kept for
// its base when keep.
void LetGoAll(Record& r, uint32_t arenaNow, bool keep) {
    Held held[kPerPickup];
    const int n = r.count;
    std::memcpy(held, r.part, sizeof(held));
    for (Held& h : r.part)
        h.instance = 0, h.ready = 0;
    const bool had = r.count > 0;
    r.count = 0;
    r.key = 0;
    if (!keep)
        r.pickup = 0;
    Publish(r);
    for (int i = 0; i < n; ++i)
        LetGo(held[i], arenaNow);
    if (had)
        g_pickupsWithParts.fetch_sub(1, std::memory_order_relaxed);
}

// Every part placed on the record's base.
void PlaceAll(Record& r) {
    for (int i = 0; i < r.count; ++i) {
        Held& h = r.part[i];
        if (!h.instance)
            continue;
        alignas(16) float world[16];
        Compose(h.rel, r.base, world);
        if (!Aligned(world) || !Aligned(r.base)) {
            Atomic(g_misaligned);
            continue;
        }
        reinterpret_cast<PlaceFn>(g_at.place)(reinterpret_cast<void*>(h.instance), nullptr, world, 1.0f);
    }
}

// The pose the game places a body on (0084CC50), into an aligned matrix. False when the pointer isn't aligned (never handed on).
bool PoseOf(uint32_t pickup, float* out16) {
    if (!Aligned(out16)) {
        Atomic(g_misaligned);
        return false;
    }
    reinterpret_cast<PoseFn>(g_at.pose)(reinterpret_cast<void*>(pickup), nullptr, out16);
    return true;
}

// A part made whole, for the census: its pickup, instance, scene slot, MeshSet's kind, asset, and where it was first placed.
void Note(uint32_t pickup, const Record& r, const Held& h, uint16_t handle) {
    const uint32_t number = g_notesMade.load(std::memory_order_relaxed) + 1;
    MadeNote& n = g_notes[number % std::size(g_notes)];
    n.number.store(0, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);
    n.pickup.store(pickup, std::memory_order_relaxed), n.instance.store(h.instance, std::memory_order_relaxed);
    n.asset.store(h.asset, std::memory_order_relaxed), n.handle.store(handle, std::memory_order_relaxed), n.kind.store(h.kind, std::memory_order_relaxed);
    n.x.store(Bits(r.base[12]), std::memory_order_relaxed), n.y.store(Bits(r.base[13]), std::memory_order_relaxed);
    n.z.store(Bits(r.base[14]), std::memory_order_relaxed);
    n.number.store(number, std::memory_order_release);
    g_notesMade.store(number, std::memory_order_release);
}

// One part made on the record (sights first; the record holds it from the moment the constructor returns). False: not made (counted why).
bool MakeOne(uint32_t pickup, Record& r, const Part& p) {
    if (g_live.load(std::memory_order_relaxed) >= kMostLive || r.count >= kPerPickup) {
        Atomic(g_refused);
        return false;
    }
    const uint32_t scene = U32(static_cast<uint32_t>(g_at.scene));
    const uint32_t arena = ArenaOf(pickup), world = WorldOf(pickup);
    if (!scene || !arena) {
        Atomic(g_noScene);
        return false;
    }
    void* memory = reinterpret_cast<AllocFn>(g_at.alloc)(reinterpret_cast<void*>(arena), nullptr, kInstanceSize, kInstanceAlign);
    if (!memory) {
        Atomic(g_noAlloc);
        return false;
    }
    void* made = reinterpret_cast<ConstructFn>(g_at.construct)(memory, nullptr, reinterpret_cast<void*>(p.asset), 1, reinterpret_cast<void*>(world),
                                                                nullptr);
    const uint32_t instance = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(made));
    if (!instance)
        return false;
    Held& h = r.part[r.count++];
    std::memcpy(h.rel, p.rel, sizeof(h.rel));
    h.instance = instance, h.unlock = p.unlock, h.object = p.object, h.asset = p.asset, h.scene = scene, h.arena = arena;
    h.kind = 0, h.ready = 0;  // (not in the row until it's whole)
    g_live.fetch_add(1, std::memory_order_relaxed);
    Atomic(g_made);
    if (r.count == 1)  // (counted with the part itself: a fault in a later call leaves the count as LetGoAll will lower it)
        g_pickupsWithParts.fetch_add(1, std::memory_order_relaxed);
    // The renderer has the mesh, and the MeshSet is there; else it goes at once.
    if (!U32(instance + kInstanceMesh) || !SetOf(instance)) {
        Atomic(g_notLoaded);
        g_unloadedAsset.store(p.asset, std::memory_order_relaxed);
        LetGo(h, arena);
        if (--r.count == 0)
            g_pickupsWithParts.fetch_sub(1, std::memory_order_relaxed);
        return false;
    }
    reinterpret_cast<EnterFn>(g_at.enter)(made, nullptr, 0);
    uint16_t handle = kNoHandle;
    kRead.U16(instance + kInstanceHandle, handle);
    if (handle == kNoHandle) {
        Atomic(g_noSlot);
        LetGo(h, arena);
        if (--r.count == 0)
            g_pickupsWithParts.fetch_sub(1, std::memory_order_relaxed);
        return false;
    }
    // Its MeshSet's kind: one the collector draws from a stand-in alone (0 or 2), else it goes at once (1, skinned, would need the pickup's bones).
    uint32_t kind = 0;
    if (!kRead.U32(SetOf(instance), kind) || !KindDrawn(kind)) {
        Atomic(g_refusedKind);
        LetGo(h, arena);
        if (--r.count == 0)
            g_pickupsWithParts.fetch_sub(1, std::memory_order_relaxed);
        return false;
    }
    h.kind = kind, h.ready = 1;
    Note(pickup, r, h, handle);
    return true;
}

// The parts wanted for a pickup at the level in force, made or kept, on base (already in the record). The game's thread, under g_lock.
void Want(uint32_t pickup, Record& r, uint32_t entry, int level) {
    uint32_t blueprint = 0;
    kRead.U32(entry, blueprint);
    uint32_t unlocks[kMostUnlocks];
    const int n = WalkSet(kRead, entry, unlocks, kMostUnlocks);
    static Gun s_gun;  // (the game's thread alone, under g_lock: off its stack)
    int wanted[kPerPickup];
    int count = 0;
    if (n >= 0 && blueprint && ReadGun(kRead, g_at.image, ClassCells(), blueprint, unlocks, n, s_gun)) {
        const bool firstAllowed = r.count > 0 || CountWithParts() == 0;
        count = Wanted(s_gun, level, firstAllowed, wanted, kPerPickup);
    }
    const uint64_t key = count ? WantKey(s_gun, wanted, count, blueprint) ^ static_cast<uint64_t>(level) : 0;
    r.entry = entry, r.blueprint = blueprint, r.level = level;
    if (key == r.key && r.count > 0) {
        PlaceAll(r);
        return;
    }
    if (r.count > 0)
        LetGoAll(r, ArenaOf(pickup), true);
    r.key = key;
    if (!count || g_noMakes.load(std::memory_order_relaxed))
        return;
    for (int i = 0; i < count; ++i)
        MakeOne(pickup, r, s_gun.part[wanted[i]]);
    if (r.count > 0) {
        r.madeAt = GetTickCount64();
        r.thread = GetCurrentThreadId();
        PlaceAll(r);
    }
}

// A world change seen on the game's thread: a record of another world whose pickup no longer reads as that pickup (its vtable, its body) is
// forgotten, nothing of it touched (it may be gone). One that still reads so keeps its parts for its own destructor to free.
void WorldCheck() {
    const uint32_t world = U32(static_cast<uint32_t>(g_at.clientWorld));
    if (!world)
        return;
    for (Record& r : g_records) {
        if (!r.pickup || !r.world || r.world == world)
            continue;
        uint32_t vtable = 0;
        if (!kRead.U32(r.pickup, vtable) || vtable != g_at.clientPickupVt || U32(r.pickup + kBody) != r.body)
            LetGoAll(r, 0, false);
    }
}

// After the game's builder: what it did to its body, then ours the same (00854B50's every return).
void AfterBuild(uint32_t pickup, uint32_t pre, uint32_t preSet, int level, bool caughtUp) {
    Record* r = RecordOf(pickup);
    if (r && r->world && r->world != U32(static_cast<uint32_t>(g_at.clientWorld))) {
        LetGoAll(*r, 0, false);  // a record of another world at this address: forgotten, never freed
        r = nullptr;
    }
    uint32_t vtable = 0;
    if (!kRead.U32(pickup, vtable) || vtable != g_at.clientPickupVt || !UsesWeaponMesh(pickup))
        return;  // the game did nothing either
    if (level < kPartsOne) {
        if (r)
            LetGoAll(*r, ArenaOf(pickup), false), Atomic(g_drained);
        return;
    }
    const uint32_t entry = PickEntry(kRead, pickup + kContentList);
    if (!entry) {  // the game freed its body (00854CE3)
        if (r)
            LetGoAll(*r, ArenaOf(pickup), false);
        return;
    }
    uint32_t data = 0, states = 0, stateCount = 0, blueprint = 0;
    if (!kRead.U32(entry, blueprint) || !kRead.U32(blueprint + kBlueprintData, data) || !data || !kRead.U32(data + kDataStates, states) || !states ||
        !kRead.U32(states - 4, stateCount) || !stateCount)
        return;  // the game left its body as it was (00854B91): ours too
    const uint32_t post = U32(pickup + kBody);
    if (!post) {  // no Mesh3p: the game freed its body (00854BDE-00854BF2)
        if (r)
            LetGoAll(*r, ArenaOf(pickup), false);
        return;
    }
    const uint32_t postSet = SetOf(post);
    const bool rebuilt = Rebuilt(pre, preSet, post, postSet);
    if (!r) {
        if (!(r = NewRecord(pickup))) {
            Atomic(g_refused);
            return;
        }
        alignas(16) float base[16];
        bool have = false;
        if (rebuilt)
            have = PoseOf(pickup, base);
        else if (const Moved* m = MovedOf(pickup))
            std::memcpy(base, m->m, sizeof(base)), have = true;
        else
            have = PoseOf(pickup, base);
        if (!have) {
            r->pickup = 0;
            Publish(*r);
            return;
        }
        std::memcpy(r->base, base, sizeof(base));
    } else if (rebuilt) {
        alignas(16) float base[16];
        if (!PoseOf(pickup, base)) {
            LetGoAll(*r, ArenaOf(pickup), false);
            return;
        }
        std::memcpy(r->base, base, sizeof(base));
    }
    r->body = post, r->bodySet = postSet;
    r->world = U32(static_cast<uint32_t>(g_at.clientWorld));
    const int had = r->count;
    Want(pickup, *r, entry, level);
    if (caughtUp && r->count > had)
        Atomic(g_caughtUp);
    if (!r->count && !r->key)
        r->pickup = 0;  // nothing wanted: no record (a moved matrix stays in the moved table)
    else
        ForgetMoved(pickup);
    Publish(*r);  // its parts, on its body as it is now (until this, a call of the collector finds the old body in the row and hands nothing)
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// The hooks (the game's threads).

struct Locked {
    Locked() { EnterCriticalSection(&g_lock); }
    ~Locked() { LeaveCriticalSection(&g_lock); }
};

int LevelNow() {
    return g_level.load(std::memory_order_relaxed);
}
bool AnyRecord() {
    return g_live.load(std::memory_order_relaxed) > 0 || g_pickupsWithParts.load(std::memory_order_relaxed) > 0;
}

// The builder's work after the game's, guarded (no C++ objects to unwind in here).
void BuildGuarded(uint32_t pickup, uint32_t pre, uint32_t preSet, int level) {
    __try {
        DropStaleMoved();
        WorldCheck();
        AfterBuild(pickup, pre, preSet, level, false);
        // A body made again lies on the pose (00854C4F-00854C65), no longer on the matrix a move gave (0084169F): that row goes, whatever was made.
        const uint32_t post = U32(pickup + kBody);
        if (Rebuilt(pre, preSet, post, SetOf(post)))
            ForgetMoved(pickup);
    } __except (g_faultCode.store(GetExceptionCode(), std::memory_order_relaxed), EXCEPTION_EXECUTE_HANDLER) {
        Atomic(g_faults);
        g_noMakes.store(true, std::memory_order_relaxed);
    }
}

void __fastcall BuildHook(void* pickupPointer, void* edx) {
    const uint32_t pickup = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pickupPointer));
    const int level = LevelNow();
    Atomic(g_builds);
    FirstThread(g_buildThread);
    if (level < kPartsCensus && !AnyRecord()) {  // (from parts 1 on its body is read before and after: a moved row goes when it's made again)
        g_build(pickupPointer, edx);
        return;
    }
    uint32_t pre = 0, preSet = 0;
    kRead.U32(pickup + kBody, pre);
    preSet = SetOf(pre);
    g_build(pickupPointer, edx);
    LARGE_INTEGER t0{}, t1{}, f{};
    QueryPerformanceCounter(&t0);
    {
        const Locked lock;
        BuildGuarded(pickup, pre, preSet, level == kPartsCensus ? kPartsOff : level);
    }
    QueryPerformanceCounter(&t1);
    QueryPerformanceFrequency(&f);
    if (f.QuadPart)
        Most(g_mostMicros, static_cast<uint32_t>((t1.QuadPart - t0.QuadPart) * 1000000 / f.QuadPart));
}

void MoveGuarded(uint32_t pickup, const float* m) {
    __try {
        DropStaleMoved();
        float copy[16];
        if (!kRead.Bytes(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(m)), copy, sizeof(copy)))
            return;
        if (Record* r = RecordOf(pickup)) {
            if (LevelNow() < kPartsOne) {
                LetGoAll(*r, ArenaOf(pickup), false);
                Atomic(g_drained);
                return;
            }
            std::memcpy(r->base, copy, sizeof(copy));  // the game placed its own on the raw matrix (0084169F)
            PlaceAll(*r);
            return;
        }
        if (LevelNow() < kPartsCensus)
            return;  // (below 1 the table isn't kept: it was emptied after the switch went down)
        Moved* row = MovedOf(pickup);
        if (!row) {
            row = &g_moved[g_movedNext];
            g_movedNext = (g_movedNext + 1) % kMostMoved;
            Atomic(g_movedPickups);
        }
        row->pickup = pickup;
        std::memcpy(row->m, copy, sizeof(copy));
    } __except (g_faultCode.store(GetExceptionCode(), std::memory_order_relaxed), EXCEPTION_EXECUTE_HANDLER) {
        Atomic(g_faults);
        g_noMakes.store(true, std::memory_order_relaxed);
    }
}

void __fastcall MoveHook(void* pickupPointer, void* edx, const float* m) {
    Atomic(g_moves);
    FirstThread(g_moveThread);
    g_moveOriginal(pickupPointer, edx, m);
    if (LevelNow() < kPartsCensus && !AnyRecord())
        return;
    const Locked lock;
    MoveGuarded(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pickupPointer)), m);
}

void GoneGuarded(uint32_t pickup) {
    __try {
        g_goneScene.store(U32(static_cast<uint32_t>(g_at.scene)), std::memory_order_relaxed);
        g_goneArena.store(ArenaOf(pickup), std::memory_order_relaxed);
        ForgetMoved(pickup);
        if (Record* r = RecordOf(pickup))
            LetGoAll(*r, ArenaOf(pickup), false);
    } __except (g_faultCode.store(GetExceptionCode(), std::memory_order_relaxed), EXCEPTION_EXECUTE_HANDLER) {
        Atomic(g_faults);
        g_noMakes.store(true, std::memory_order_relaxed);
    }
}

void __fastcall GoneHook(void* pickupPointer, void* edx) {
    Atomic(g_gones);
    FirstThread(g_goneThread);
    {
        const Locked lock;  // (every time once hooked: a record or a moved row never outlives its pickup)
        GoneGuarded(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pickupPointer)));
    }
    g_goneOriginal(pickupPointer, edx);
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// The pickup renderer's collector (the game's render jobs: any thread, several at once). Never g_lock; only the rows, the holds and the counts.

void SeenOn(DWORD thread) {
    for (std::atomic<DWORD>& t : g_collectThread) {
        DWORD was = t.load(std::memory_order_relaxed);
        if (was == thread || (!was && (t.compare_exchange_strong(was, thread, std::memory_order_relaxed) || was == thread)))
            return;
    }
}

int RowOf(uint32_t pickup) {
    for (int i = 0; i < kMostRecords; ++i)
        if (RowPickup(g_rows[i]) == pickup)
            return i;
    return -1;
}

// The stand-ins for one call, in call slot c: for each pickup the call was handed that the game shows, whose row holds parts made for its body as
// it is, in the world and scene there are now: those parts, held first, each on the pose the game draws the body on in this same call (0084A7C0
// from the entry 00854960 would take, guarded as there), the gun's light and its element's views. How many (at most kDrawEach; a pickup whose parts
// don't all fit gets none).
int Fill(int c, uint32_t count, const uint32_t* elements) {
    CallMemory& m = g_callMemory[c];
    CallHolds& holds = g_holds[c];
    const uint32_t world = U32(static_cast<uint32_t>(g_at.clientWorld)), scene = U32(static_cast<uint32_t>(g_at.scene));
    const uint32_t lights = U32(static_cast<uint32_t>(g_at.lights));
    if (!world || !scene || !lights || !Aligned(m.pose))
        return 0;  // (no light system: 01744260 would be called on none)
    int n = 0;
    for (uint32_t i = 0; i < count; ++i) {  // (on past the cap: those that don't fit are counted)
        uint32_t element[kElementWords] = {};
        uint32_t pickup = 0, vtable = 0, body = 0;
        uint8_t shown = 0, hiding = 0;
        if (!kRead.Bytes(Addr(elements + kElementWords * i), element, sizeof(element)) || !(pickup = element[0]) || !kRead.U32(pickup, vtable) ||
            vtable != g_at.clientPickupVt || !kRead.U8(pickup + kShown, shown) || !kRead.U8(pickup + kHiding, hiding) || !BodyShown(shown, hiding) ||
            !kRead.U32(pickup + kBody, body) || !body)
            continue;
        const int at = RowOf(pickup);
        if (at < 0)
            continue;
        Row row;
        uint32_t seq = 0;
        if (!ReadRow(g_rows[at], row, seq)) {
            Atomic(g_rowBusy);
            continue;
        }
        if (row.pickup != pickup || row.body != body || row.world != world || row.scene != scene || !row.count ||
            row.count > static_cast<uint32_t>(kRowParts))
            continue;  // made for another body, world or scene: nothing until the game's thread writes the row again
        if (n + static_cast<int>(row.count) > kDrawEach) {
            Atomic(g_overCap);
            continue;
        }
        if (!HoldRow(holds, n, row.instance, static_cast<int>(row.count), g_rows[at], seq)) {
            Atomic(g_rowBusy);
            continue;
        }
        // Held: none of them can be freed now. Each as it was made all the same (its mesh holder, MeshSet, kind, the renderer's mesh), then the pose.
        bool whole = true;
        for (uint32_t k = 0; k < row.count && whole; ++k) {
            uint32_t holder = 0, set = 0, kind = 0, mesh = 0;
            whole = KindDrawn(row.kind[k]) && kRead.U32(row.instance[k] + kInstanceHolder, holder) && holder && kRead.U32(holder + kHolderSet, set) && set &&
                    kRead.U32(set, kind) && kind == row.kind[k] && kRead.U32(row.instance[k] + kInstanceMesh, mesh) && mesh;
        }
        const uint32_t entry = whole ? PoseEntry(kRead, pickup) : 0;
        if (!entry) {
            if (!whole)
                Atomic(g_notAsMade);
            for (uint32_t k = 0; k < row.count; ++k)
                holds.instance[n + k].store(0, std::memory_order_seq_cst);
            continue;
        }
        reinterpret_cast<PoseOfFn>(g_at.poseOf)(reinterpret_cast<void*>(pickup), nullptr, entry, m.pose);
        uint16_t handle = kNoLight;
        kRead.U16(pickup + kLightHandle, handle);
        handle = LightFor(kRead, lights, handle);
        for (uint32_t k = 0; k < row.count; ++k, ++n) {
            Compose(row.rel[k], m.pose, m.world[n]);
            FillStandin(m.standin[n], vtable, Addr(m.world[n]), row.instance[k], handle);
            m.element[n][0] = Addr(m.standin[n]);
            for (uint32_t w = 1; w < kElementWords; ++w)
                m.element[n][w] = element[w];  // its views' mask (0179BD0D) and the rest, the gun's own
        }
    }
    return n;
}

int FillGuarded(int c, uint32_t count, const uint32_t* elements) {
    __try {
        return Fill(c, count, elements);
    } __except (g_drawFaultCode.store(GetExceptionCode(), std::memory_order_relaxed), EXCEPTION_EXECUTE_HANDLER) {
        Atomic(g_drawFaults);
        g_drawOn.store(false, std::memory_order_relaxed);
        return 0;
    }
}

// The game's own collector once more, on the stand-ins alone (its records go in the same frame's lists as the gun's).
bool DrawGuarded(void* self, void* edx, int c, int n, void* unused, void* records) {
    __try {
        g_collectOriginal(self, edx, static_cast<uint32_t>(n), &g_callMemory[c].element[0][0], unused, records);
        return true;
    } __except (g_drawFaultCode.store(GetExceptionCode(), std::memory_order_relaxed), EXCEPTION_EXECUTE_HANDLER) {
        Atomic(g_drawFaults);
        g_drawOn.store(false, std::memory_order_relaxed);
        return false;
    }
}

void __fastcall CollectHook(void* self, void* edx, uint32_t count, const uint32_t* elements, void* unused, void* records) {
    Most(g_mostAtOnce, g_atOnce.fetch_add(1, std::memory_order_relaxed) + 1);
    g_collectOriginal(self, edx, count, elements, unused, records);  // the game's own pass, untouched
    Atomic(g_calls);
    Most(g_mostElements, count);
    SeenOn(GetCurrentThreadId());
    if (count && elements && LevelNow() >= kPartsOne && g_drawOn.load(std::memory_order_relaxed) && g_rowsLive.load(std::memory_order_relaxed) > 0) {
        const int c = ClaimCall(g_holds, kCallSlots);
        if (c < 0)
            Atomic(g_noCall);
        else {
            const int n = FillGuarded(c, count, elements);
            if (n > 0 && DrawGuarded(self, edx, c, n, unused, records)) {
                Atomic(g_callsDrawn);
                g_standins.fetch_add(static_cast<uint32_t>(n), std::memory_order_relaxed);
            }
            ReleaseCall(g_holds[c]);  // (only now may the game's thread free what this call held)
        }
    }
    g_atOnce.fetch_sub(1, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// The client world's pickups (read; either thread).

int ClientPickups(uint32_t* out, int most) {
    uint32_t world = 0, vtable = 0, lists = 0, listsEnd = 0, segment = 0;
    uint16_t first = 0, cls = 0;
    if (!kRead.U32(static_cast<uint32_t>(g_at.clientWorld), world) || !world || !kRead.U32(world, vtable) || vtable != g_at.clientWorldVt ||
        !kRead.U16(world + kWorldFirstClass, first) || !kRead.U16(static_cast<uint32_t>(g_at.clientPickupType) + kClassIdAt, cls) || cls < first ||
        !kRead.U32(world + kWorldLists, lists) || !kRead.U32(world + kWorldListsEnd, listsEnd))
        return 0;
    const uint32_t at = lists + 8u * (cls - first);
    if (!lists || at + 8 > listsEnd || !kRead.U32(at, segment))
        return 0;
    int n = 0;
    for (int guard = 0; segment && guard < kMostSegments && n < most; ++guard) {
        uint32_t begin = 0, end = 0, next = 0, active = 0;
        if (!kRead.U32(segment, begin) || !kRead.U32(segment + 4, end) || !kRead.U32(segment + kSegmentActive, active) ||
            !kRead.U32(segment + kSegmentNext, next))
            return n;
        const uint32_t all = end >= begin && end - begin <= 4u * 4096 ? (end - begin) / 4 : 0;
        for (uint32_t i = 0; i < std::min(active, all) && n < most; ++i) {
            uint32_t entity = 0, entityVt = 0;
            if (kRead.U32(begin + 4 * i, entity) && entity && kRead.U32(entity, entityVt) && entityVt == g_at.clientPickupVt)
                out[n++] = entity;
        }
        segment = next;
    }
    return n;
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// The game's thread of the player's soldier: the catch-up pass and the drain, only when it's the thread the game builds its pickups on.

void TickGuarded(ULONGLONG now, int level) {
    __try {
        DropStaleMoved();
        WorldCheck();
        if (level < kPartsOne) {
            for (Record& r : g_records)
                if (r.pickup) {
                    uint32_t vtable = 0;
                    const bool alive = kRead.U32(r.pickup, vtable) && vtable == g_at.clientPickupVt && U32(r.pickup + kBody) == r.body;
                    LetGoAll(r, alive ? ArenaOf(r.pickup) : 0, false);
                    Atomic(g_drained);
                }
            return;
        }
        if (now - g_catchUpAt < kCatchUpMs || g_noMakes.load(std::memory_order_relaxed))
            return;
        g_catchUpAt = now;
        uint32_t pickups[kMostWalked];
        const int n = ClientPickups(pickups, kMostWalked);
        int done = 0, i = 0;
        // On from where the last pass stopped: a drop that wants nothing at this level keeps no record, and would else be tried first every time.
        const int from = n > 0 ? g_catchUpNext % n : 0;
        for (; i < n && done < kCatchUpEach; ++i) {
            const uint32_t pickup = pickups[(from + i) % n];
            const Record* r = RecordOf(pickup);
            if ((r && r->level == level) || !UsesWeaponMesh(pickup))
                continue;
            const uint32_t body = U32(pickup + kBody);
            if (!body)
                continue;
            ++done;
            AfterBuild(pickup, body, SetOf(body), level, true);  // not rebuilt: the base the game last used
        }
        g_catchUpNext = n > 0 ? (from + i) % n : 0;
    } __except (g_faultCode.store(GetExceptionCode(), std::memory_order_relaxed), EXCEPTION_EXECUTE_HANDLER) {
        Atomic(g_faults);
        g_noMakes.store(true, std::memory_order_relaxed);
    }
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// The render thread's own.

bool g_verified = false, g_failed = false;
int g_levelSaid = -1;
ULONGLONG g_censusAt = 0, g_countsAt = 0, g_countsSaidAt = 0;
uint32_t g_countsLast[20] = {};
bool g_threadsSaid[4] = {};
bool g_p6Said = false, g_faultSaid = false, g_unloadedSaid = false;
// The collector's threads said, its fault said, the made notes said up to.
DWORD g_collectSaid[4] = {};
bool g_drawFaultSaid = false;
uint32_t g_notesSaid = 0;
// Said once each: (pickup, key) of the census, pickups without UseWeaponMesh.
struct SaidKey {
    uint32_t pickup = 0;
    uint64_t key = 0;
};
SaidKey g_saidCensus[128];
int g_saidNext = 0;
uint32_t g_saidOwnMesh[32] = {};
int g_saidOwnNext = 0;

void Append(char* line, size_t size, int& used, const char* format, ...) {
    if (used < 0 || static_cast<size_t>(used) >= size)
        return;
    va_list args;
    va_start(args, format);
    const int n = vsnprintf(line + used, size - used, format, args);
    va_end(args);
    if (n > 0)
        used = std::min(static_cast<int>(size) - 1, used + n);
}

void Fail(const char* why) {
    if (g_failed)
        return;
    g_failed = true;
    Log("Pickups: parts: the game's code isn't as read: %s: left off for this session", why);
}

uint64_t Fnv(const uint8_t* bytes, size_t size) {
    uint64_t h = 14695981039346656037ull;
    for (size_t i = 0; i < size; ++i) {
        h ^= bytes[i];
        h *= 1099511628211ull;
    }
    return h;
}

bool ImageRanges() {
    if (g_at.image.codeTo && g_at.image.rdataTo)
        return true;
    uint32_t peAt = 0, signature = 0;
    uint16_t sections = 0, optional = 0;
    if (!kRead.U32(kImageBase + 0x3C, peAt) || peAt > 0x1000 || !kRead.U32(kImageBase + peAt, signature) || signature != 0x00004550 ||
        !kRead.U16(kImageBase + peAt + 6, sections) || !kRead.U16(kImageBase + peAt + 20, optional) || sections > 64)
        return false;
    const uint32_t table = kImageBase + peAt + 24 + optional;
    for (int i = 0; i < sections; ++i) {
        char name[8] = {};
        uint32_t size = 0, va = 0;
        if (!kRead.Bytes(table + 40 * i, name, sizeof(name)) || !kRead.U32(table + 40 * i + 8, size) || !kRead.U32(table + 40 * i + 12, va))
            return false;
        if (std::memcmp(name, ".text\0\0\0", 8) == 0)
            g_at.image.codeFrom = kImageBase + va, g_at.image.codeTo = kImageBase + va + size;
        else if (std::memcmp(name, ".rdata\0\0", 8) == 0)
            g_at.image.rdataFrom = kImageBase + va, g_at.image.rdataTo = kImageBase + va + size;
        else if (std::memcmp(name, "ctr\0\0\0\0\0", 8) == 0)
            g_at.image.moreCodeFrom = kImageBase + va, g_at.image.moreCodeTo = kImageBase + va + size;
    }
    return g_at.image.codeTo > g_at.image.codeFrom && g_at.image.rdataTo > g_at.image.rdataFrom;
}

bool Verify() {
    char why[200];
    static uint8_t s_now[4096];  // (the render thread alone)
    for (int i = 0; i < g_checks.codes; ++i) {
        const Code& c = g_checks.code[i];
        if (c.size > sizeof(s_now) || !kRead.Bytes(static_cast<uint32_t>(c.at), s_now, c.size) || Fnv(s_now, c.size) != c.fnv) {
            snprintf(why, sizeof(why), "%s (%u bytes at %08X)", c.what, c.size, static_cast<unsigned>(c.at));
            Fail(why);
            return false;
        }
    }
    for (int i = 0; i < g_checks.byteRows; ++i) {
        const Bytes& b = g_checks.bytes[i];
        if (b.size > sizeof(s_now) || !kRead.Bytes(static_cast<uint32_t>(b.at), s_now, b.size) || std::memcmp(s_now, b.bytes, b.size) != 0) {
            snprintf(why, sizeof(why), "%s (%u bytes at %08X)", b.what, b.size, static_cast<unsigned>(b.at));
            Fail(why);
            return false;
        }
    }
    for (int i = 0; i < g_checks.wordRows; ++i) {
        const Word& w = g_checks.words[i];
        uint32_t value = 0;
        if (!kRead.Bytes(static_cast<uint32_t>(w.at), &value, w.size) || value != w.value) {
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
    Log("Pickups: parts: the game's code is as read (the builder of a dropped gun's body, its setTransform and destructor, the mesh instance's "
        "calls, the scene's add and remove, the allocator, the gun data's fields, the pickup renderer's collector and what it calls, the pose)");
    return true;
}

bool HookAll() {
    const struct {
        uintptr_t at;
        const uint8_t* start;
        uint32_t size;
        void* detour;
        void** original;
        const char* what;
    } hooks[] = {
        {g_at.build, kBuildStart, sizeof(kBuildStart), reinterpret_cast<void*>(&BuildHook), reinterpret_cast<void**>(&g_build), "the builder"},
        {g_at.move, kMoveStart, sizeof(kMoveStart), reinterpret_cast<void*>(&MoveHook), reinterpret_cast<void**>(&g_moveOriginal), "setTransform"},
        {g_at.gone, kGoneStart, sizeof(kGoneStart), reinterpret_cast<void*>(&GoneHook), reinterpret_cast<void**>(&g_goneOriginal), "the destructor"},
        {g_at.collect, kCollectStart, sizeof(kCollectStart), reinterpret_cast<void*>(&CollectHook), reinterpret_cast<void**>(&g_collectOriginal),
         "the collector"},
    };
    char why[160];
    for (const auto& h : hooks) {
        uint8_t now[16] = {};
        if (g_checks.byteRows && (!kRead.Bytes(static_cast<uint32_t>(h.at), now, h.size) || std::memcmp(now, h.start, h.size) != 0)) {
            snprintf(why, sizeof(why), "%s's start isn't as read any more (at %08X)", h.what, static_cast<unsigned>(h.at));
            Fail(why);
            return false;
        }
    }
    if (!g_lockMade.load()) {
        InitializeCriticalSection(&g_lock);
        g_lockMade.store(true);
    }
    int made = 0;
    for (const auto& h : hooks) {
        void* trampoline = nullptr;
        if (MH_CreateHook(reinterpret_cast<void*>(h.at), h.detour, &trampoline) != MH_OK || !trampoline)
            break;
        *h.original = trampoline;
        ++made;
    }
    if (made < static_cast<int>(std::size(hooks))) {
        for (int i = 0; i < made; ++i)
            MH_RemoveHook(reinterpret_cast<void*>(hooks[i].at));
        Fail("its four functions couldn't be hooked");
        return false;
    }
    // Turned on the destructor first, then setTransform, the collector, and the builder last: nothing is made before the hooks that free it and draw
    // it are in, and a hook that won't turn on leaves nothing made (the collector before the builder: it hands nothing while no row holds a part).
    const int order[] = {2, 1, 3, 0};
    static_assert(sizeof(order) / sizeof(order[0]) == sizeof(hooks) / sizeof(hooks[0]), "every hook turned on");
    for (int i : order)
        if (MH_EnableHook(reinterpret_cast<void*>(hooks[i].at)) != MH_OK) {
            for (int k = 0; k < made; ++k)
                MH_RemoveHook(reinterpret_cast<void*>(hooks[k].at));
            Fail("its four hooks couldn't be turned on");
            return false;
        }
    g_hooked.store(true);
    Log("Pickups: parts: hooked the builder of a dropped gun's body, its setTransform, its destructor and the pickup renderer's collector (00797FF0)");
    return true;
}

const char* Short(const char* name) {
    const char* slash = std::strrchr(name, '/');
    return slash ? slash + 1 : name;
}

bool SaidCensus(uint32_t pickup, uint64_t key) {
    for (const SaidKey& s : g_saidCensus)
        if (s.pickup == pickup && s.key == key)
            return true;
    g_saidCensus[g_saidNext] = {pickup, key};
    g_saidNext = (g_saidNext + 1) % static_cast<int>(std::size(g_saidCensus));
    return false;
}
bool SaidOwnMesh(uint32_t pickup) {
    for (uint32_t p : g_saidOwnMesh)
        if (p == pickup)
            return true;
    g_saidOwnMesh[g_saidOwnNext] = pickup;  // (the oldest goes, as the census's own)
    g_saidOwnNext = (g_saidOwnNext + 1) % static_cast<int>(std::size(g_saidOwnMesh));
    return false;
}

// One pickup in the log, once per its content: the entry, its unlocks, each part the 3P data has for them, and what the level in force draws.
void SayPickup(uint32_t pickup, int level) {
    float raw[16] = {};
    const bool haveRaw = RawTransform(pickup, raw);
    if (!UsesWeaponMesh(pickup)) {
        if (SaidOwnMesh(pickup))
            return;
        uint32_t data = 0, own = 0;
        char name[kText] = {};
        if (kRead.U32(pickup + kPickupData, data) && data && kRead.U32(data + kOwnMesh, own) && own)
            kRead.Text(own + kAssetName, name, kText);
        Log("Pickups: parts: a pickup at (%.1f %.1f %.1f) has no UseWeaponMesh: its own mesh %s, placed raw (not drawn with parts)", raw[12], raw[13],
            raw[14], name[0] ? name : "(none)");
        return;
    }
    int pass = 0;
    const uint32_t entry = PickEntry(kRead, pickup + kContentList, &pass);
    uint32_t blueprint = 0;
    if (!entry || !kRead.U32(entry, blueprint) || !blueprint)
        return;
    uint32_t unlocks[kMostUnlocks];
    const char* why = "";
    const int n = WalkSet(kRead, entry, unlocks, kMostUnlocks, &why);
    const uint64_t key = (n >= 0 ? SetKey(unlocks, n) : 1) ^ (static_cast<uint64_t>(blueprint) << 32) ^ static_cast<uint64_t>(level);
    if (SaidCensus(pickup, key))
        return;
    char gun[kText] = {};
    kRead.Text(blueprint + 8, gun, kText);
    static char line[3000];  // (the render thread alone)
    int used = 0;
    Append(line, sizeof(line), used, "Pickups: parts: the %s at (%.1f %.1f %.1f)%s, entry %u (%s): ", Short(gun), raw[12], raw[13], raw[14],
           haveRaw ? "" : " (no transform)", (entry - U32(pickup + kContentList + 4)) / kEntrySize,
           pass == 1 ? "the preferred slot" : "the lowest slot");
    if (n < 0) {
        Append(line, sizeof(line), used, "its unlocks refused: %s", why);
        Log("%s", line);
        return;
    }
    static Gun s_gun;  // (the render thread alone)
    const char* gunWhy = "";
    if (!ReadGun(kRead, g_at.image, ClassCells(), blueprint, unlocks, n, s_gun, &gunWhy)) {
        Append(line, sizeof(line), used, "%d unlocks; %s", n, gunWhy);
        Log("%s", line);
        return;
    }
    Append(line, sizeof(line), used, "%d unlock%s", n, n == 1 ? "" : "s");
    for (int i = 0; i < n; ++i) {
        char name[kText] = {};
        uint32_t kind = 0;
        kRead.Text(unlocks[i] + kUnlockName, name, kText);
        kRead.U32(unlocks[i] + kUnlockKind, kind);
        Append(line, sizeof(line), used, "%s %s (kind %u, %s)", i ? "," : ":", name[0] ? name : "(no name)", kind, KindName(KindOf(name)));
        int parts = 0;
        for (int k = 0; k < s_gun.parts; ++k) {
            const Part& p = s_gun.part[k];
            if (p.unlock != unlocks[i])
                continue;
            ++parts;
            const char* no = NotDrawn(p, level < kPartsOne ? kPartsAll : level);
            Append(line, sizeof(line), used, " -> socket %d (%s, object %d, vtable %08X%s), its 3P asset %s (vtable %08X, class %u) at (%.3f %.3f %.3f) from %s: %s",
                   p.socketIndex, p.bone[0] ? p.bone : "no bone", p.objectIndex, p.objectVt, p.objectOk ? "" : " not a regular socket object",
                   p.asset ? (p.assetName[0] ? p.assetName : "(no name)") : "none", p.assetVt, p.assetClass, p.rel[12], p.rel[13], p.rel[14],
                   p.fromState ? "the state's list" : "the object's own", no[0] ? no : level >= kPartsOne ? "wanted" : "drawable");
        }
        if (!parts)
            Append(line, sizeof(line), used, " -> no socket");
    }
    int bare = 0, bareAssets = 0;
    for (int k = 0; k < s_gun.parts; ++k)
        if (!s_gun.part[k].unlock) {
            ++bare;
            bareAssets += s_gun.part[k].asset != 0;
        }
    Append(line, sizeof(line), used, "; %d socket objects with no unlock (%d with a 3P asset)", bare, bareAssets);
    if (s_gun.otherBones)
        Append(line, sizeof(line), used, "; %d of its unlocks' sockets on another bone", s_gun.otherBones);
    if (s_gun.absentDefaults)
        Append(line, sizeof(line), used, "; %d sockets use a default object, their unlock absent", s_gun.absentDefaults);
    Append(line, sizeof(line), used, "; %d states, %d sockets; the body's Mesh3p vtable %08X class %u; root bone %s", s_gun.stateCount, s_gun.socketCount,
           s_gun.mesh3pVt, s_gun.mesh3pClass, s_gun.rootIdentity ? "identity" : "not identity");
    if (s_gun.haveRoot && !s_gun.rootIdentity) {  // (where a part would be off by, if its matrix is in that bone's frame: the 30 cm still decides)
        const float* m = s_gun.root;
        Append(line, sizeof(line), used, " (at (%.4f %.4f %.4f), its rows (%.3f %.3f %.3f) (%.3f %.3f %.3f) (%.3f %.3f %.3f))", m[12], m[13], m[14], m[0], m[1],
               m[2], m[4], m[5], m[6], m[8], m[9], m[10]);
    }
    Log("%s", line);
}

void Census(ULONGLONG now, int level) {
    if (now - g_censusAt < kCensusMs)
        return;
    g_censusAt = now;
    uint32_t pickups[32];
    const int n = ClientPickups(pickups, 32);
    for (int i = 0; i < n; ++i)
        SayPickup(pickups[i], level);
}

void SayThreads() {
    const char* names[] = {"the builder", "setTransform", "the destructor", "the interaction update (pickups' tick)"};
    std::atomic<DWORD>* threads[] = {&g_buildThread, &g_moveThread, &g_goneThread, &g_tickThread};
    for (int i = 0; i < 4; ++i)
        if (!g_threadsSaid[i] && threads[i]->load(std::memory_order_relaxed)) {
            g_threadsSaid[i] = true;
            Log("Pickups: parts: %s runs on thread %lu (the render thread %lu)", names[i], threads[i]->load(std::memory_order_relaxed),
                GetCurrentThreadId());
        }
    const DWORD build = g_buildThread.load(std::memory_order_relaxed), tick = g_tickThread.load(std::memory_order_relaxed);
    if (!g_p6Said && build && tick) {
        g_p6Said = true;
        Log(build == tick ? "Pickups: parts: the builder and pickups' tick share the game's thread: the catch-up pass and the drain run there"
                          : "Pickups: parts: the builder runs on another thread than pickups' tick: no catch-up pass (parts show on drops built "
                            "from now on) and no drain (parts go with each pickup's own next build, move or end)");
    }
    // The collector's threads (its first four), each once.
    for (int i = 0; i < 4; ++i) {
        const DWORD t = g_collectThread[i].load(std::memory_order_relaxed);
        if (!t || g_collectSaid[i] == t)
            continue;
        g_collectSaid[i] = t;
        Log("Pickups: parts: the pickup renderer's collector runs on thread %lu (the builder's %lu, the render thread %lu)", t, build, GetCurrentThreadId());
    }
}

// Each part made since the last time, once: which pickup, where it was first placed, its instance, scene slot and MeshSet's kind (said here: the
// game's thread only notes them).
void SayMade() {
    const uint32_t made = g_notesMade.load(std::memory_order_acquire);
    if (made - g_notesSaid > std::size(g_notes)) {
        Log("Pickups: parts: %u parts made before these weren't said", made - g_notesSaid - static_cast<uint32_t>(std::size(g_notes)));
        g_notesSaid = made - static_cast<uint32_t>(std::size(g_notes));
    }
    for (; g_notesSaid != made; ) {
        const uint32_t number = ++g_notesSaid;
        const MadeNote& n = g_notes[number % std::size(g_notes)];
        if (n.number.load(std::memory_order_acquire) != number)
            continue;  // (written over already)
        const uint32_t pickup = n.pickup.load(std::memory_order_relaxed), instance = n.instance.load(std::memory_order_relaxed),
                       asset = n.asset.load(std::memory_order_relaxed), handle = n.handle.load(std::memory_order_relaxed),
                       kind = n.kind.load(std::memory_order_relaxed);
        const float x = Float(n.x.load(std::memory_order_relaxed)), y = Float(n.y.load(std::memory_order_relaxed)),
                    z = Float(n.z.load(std::memory_order_relaxed));
        std::atomic_thread_fence(std::memory_order_acquire);
        if (n.number.load(std::memory_order_relaxed) != number)
            continue;
        char name[kText] = {};
        kRead.Text(asset + kAssetName, name, kText);
        Log("Pickups: parts: made %s on the pickup %08X first placed at (%.2f %.2f %.2f): instance %08X, scene slot %u, its MeshSet's kind %u", name[0] ? name : "(no name)",
            pickup, x, y, z, instance, handle, kind);
    }
}

// What the collector did: its calls (those that handed stand-ins), the stand-ins, the most elements in one call and calls at once, what it didn't
// hand and why, its faults, the frees that waited for a call.
void SayCollector() {
    Log("Pickups: parts: the collector: %u calls (%u with stand-ins, %u stand-ins in all), the most %u elements in a call, %u calls at once; not handed: "
        "no call slot %u, a row being written %u, over %d a call %u, a part not as made %u; refused at the make (its MeshSet's kind) %u; faults %u; "
        "frees that waited for a call %u (forgotten after 5 ms %u)",
        g_calls.load(), g_callsDrawn.load(), g_standins.load(), g_mostElements.load(), g_mostAtOnce.load(), g_noCall.load(), g_rowBusy.load(), kDrawEach,
        g_overCap.load(), g_notAsMade.load(), g_refusedKind.load(), g_drawFaults.load(), g_heldWaits.load(), g_heldForgotten.load());
    if (g_drawFaults.load() && !g_drawFaultSaid) {
        g_drawFaultSaid = true;
        Log("Pickups: parts: the collector's stand-ins faulted (%08X): none handed over for the rest of the session", g_drawFaultCode.load());
    }
}

void SayCounts(ULONGLONG now) {
    if (now - g_countsAt < kCountsMs)
        return;
    g_countsAt = now;
    const uint32_t v[20] = {static_cast<uint32_t>(g_live.load()), static_cast<uint32_t>(g_pickupsWithParts.load()), g_made.load(), g_freed.load(),
                            g_refused.load(), g_notLoaded.load(), g_noSlot.load(), g_noScene.load(), g_misaligned.load(), g_forgotten.load(),
                            g_faults.load(), g_builds.load(), g_moves.load(), g_gones.load(), g_caughtUp.load(), g_drained.load(), g_noAlloc.load(),
                            g_mostMicros.load(), g_movedPickups.load(),
                            g_noCall.load() + g_rowBusy.load() + g_overCap.load() + g_notAsMade.load() + g_refusedKind.load() + g_drawFaults.load() +
                                g_heldWaits.load()};
    SayMade();
    bool changed = false;  // (builds, moves, the collector's calls and the time alone don't make a line: they come every frame)
    for (int i = 0; i < 20; ++i)
        changed = changed || (i != 11 && i != 12 && i != 17 && i != 18 && v[i] != g_countsLast[i]);
    if (!changed && (v[0] == 0 || now - g_countsSaidAt < kCountsRepeatMs))
        return;
    std::memcpy(g_countsLast, v, sizeof(v));
    g_countsSaidAt = now;
    Log("Pickups: parts: %u live on %u pickups (made %u, freed %u, refused %u, not loaded %u, no scene slot %u, no scene %u, no memory %u, misaligned "
        "%u, forgotten %u, faults %u); builds %u, moves %u (%u pickups), gone %u, caught up %u, drained %u; the most a build took %u us; the scene "
        "%08X and the allocator %08X at the last end",
        v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[16], v[8], v[9], v[10], v[11], v[12], v[18], v[13], v[14], v[15], v[17],
        g_goneScene.load(), g_goneArena.load());
    SayCollector();
    if (v[10] && !g_faultSaid) {
        g_faultSaid = true;
        Log("Pickups: parts: faulted (%08lX): no more parts made this session; those held are still freed with their pickups", g_faultCode.load());
    }
    if (const uint32_t asset = g_unloadedAsset.load(); asset && !g_unloadedSaid) {
        g_unloadedSaid = true;
        char name[kText] = {};
        kRead.Text(asset + kAssetName, name, kText);
        Log("Pickups: parts: the 3P mesh %s isn't loaded: not drawn (said once)", name[0] ? name : "(no name)");
    }
}

}  // namespace

// ---------------------------------------------------------------------------------------------------------------------------------------------------

void OnFrame(unsigned long long now, int level, int parts, bool pickupsVerified) {
    const int want = level == 0 ? kPartsOff : std::clamp(parts < 0 ? 0 : parts, kPartsOff, kPartsAll);
    g_level.store(want, std::memory_order_relaxed);
    if (want != g_levelSaid && (g_levelSaid >= 0 || want > 0)) {
        static const char* kSays[] = {"off: nothing of the game's hooked, read or called for them", "each drop's unlocks and parts named in the log, "
                                      "nothing drawn", "one sight on one dropped gun", "the sights on every dropped gun", "every part with a picture "
                                      "on every dropped gun, magazines not"};
        Log("Pickups: parts %d (%s): %s%s", want, level == 0 && parts > 0 ? "pickups off" : "pickups.txt", kSays[want],
            want == 0 && g_hooked.load() ? " (the hooks stay, passing through; what is held goes with each pickup)" : "");
    }
    if (want < kPartsCensus && g_levelSaid >= kPartsCensus)
        g_movedStale.store(true, std::memory_order_relaxed);
    g_levelSaid = want;
    if (want == kPartsOff && !g_hooked.load(std::memory_order_relaxed))
        return;
    if (want >= kPartsCensus && pickupsVerified && !g_verified && !g_failed)
        Verify();
    if (want >= kPartsCensus && g_verified && !g_hooked.load() && !g_failed)
        HookAll();
    if (!g_hooked.load(std::memory_order_relaxed))
        return;
    SayThreads();
    if (want >= kPartsCensus)
        Census(now, want);
    SayCounts(now);
}

void GameTick(unsigned long long now) {
    if (!g_hooked.load(std::memory_order_relaxed))
        return;
    FirstThread(g_tickThread);
    const DWORD self = GetCurrentThreadId();
    if (g_buildThread.load(std::memory_order_relaxed) != self)
        return;  // not the thread the game builds its pickups on (or not known yet)
    const int level = LevelNow();
    if (level < kPartsOne && !AnyRecord())
        return;
    const Locked lock;
    TickGuarded(now, level);
}

int Live() {
    return g_live.load(std::memory_order_relaxed);
}

}  // namespace pickparts

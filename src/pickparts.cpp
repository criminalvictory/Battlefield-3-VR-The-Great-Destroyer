// A dropped gun's attachments where it lies (pickparts.h). The game's builder of a client pickup's body (00854B50), its setTransform (00841680) and
// its destructor (00857FF0) are hooked once build\pickups.txt has said "parts 1" or more and every piece of the game's code this goes through is as
// read. Inside the builder, on the game's thread: the entry the game draws (011BB9D0's two passes, read), its unlocks (the set at entry+4), the gun's
// sockets for them, each part's mesh made with the game's own calls (the pickup's allocator, 0101AA90, 01011D30), placed with 0100E820 on the pose
// the game just placed its body on (0084CC50) times the part's matrix from the gun's own data. setTransform moves them with the body; the
// destructor frees them first, with the game's own pair (010158A0, 00425930), only while the scene and the allocator are the ones they were made
// with (else forgotten, counted). The render thread reads only: the census in the log, the counts. Every address is a VA in bf3_mem.exe; they're all
// in one table (g_at), so tools/pickpartstest.cpp can point them at memory and functions of its own.

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
    uintptr_t build = 0x00854B50, move = 0x00841680, gone = 0x00857FF0;  // hooked
    uintptr_t pose = 0x0084CC50, alloc = 0x0041BE10, construct = 0x0101AA90, enter = 0x01011D30, place = 0x0100E820, leave = 0x010158A0,
              release = 0x00425930;
    uintptr_t scene = 0x023B3E84;  // the scene the instances are entered into (01011D33)
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
};
// Byte for byte as well: where it hooks (MinHook's jump goes over the first 5 or 6 bytes), and the content read's call of the builder.
constexpr uint8_t kBuildStart[] = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0, 0x83, 0xEC, 0x44, 0x53, 0x56, 0x8B};
constexpr uint8_t kMoveStart[] = {0x56, 0x57, 0x8B, 0x7C, 0x24, 0x0C, 0x57, 0x8B, 0xF1, 0xE8, 0x32, 0xFB};
constexpr uint8_t kGoneStart[] = {0x53, 0x55, 0x56, 0x8B, 0xF1, 0x8B, 0x9E, 0x10, 0x01, 0x00, 0x00, 0x57};
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

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// Its own numbers and what it holds.

constexpr int kPerPickup = 8, kMostLive = 128, kMostRecords = 64, kMostMoved = 64, kCatchUpEach = 4, kMostWalked = 64;
constexpr ULONGLONG kCensusMs = 1000, kCatchUpMs = 1000, kCountsMs = 1000, kCountsRepeatMs = 10000;

// One part made: its matrix on the body, its instance, what it was made of, the scene and the allocator it was made with.
struct alignas(16) Held {
    float rel[16];
    uint32_t instance, unlock, object, asset, scene, arena;
    uint32_t pad[2];
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

// One part let go of: freed while the scene and the allocator are the ones it was made with (arenaNow: the pickup's now, 0 when it can't be known),
// else forgotten. Either way it's no longer held.
void LetGo(Held& h, uint32_t arenaNow) {
    if (!h.instance)
        return;
    const uint32_t instance = h.instance;
    const uint32_t sceneNow = U32(static_cast<uint32_t>(g_at.scene));
    const bool same = h.scene && sceneNow == h.scene && arenaNow && arenaNow == h.arena;
    h.instance = 0;
    g_live.fetch_sub(1, std::memory_order_relaxed);
    if (!same) {
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

// Every part of a record let go of (the record is emptied first: never freed twice), the record kept for its base when keep.
void LetGoAll(Record& r, uint32_t arenaNow, bool keep) {
    Held held[kPerPickup];
    const int n = r.count;
    std::memcpy(held, r.part, sizeof(held));
    for (Held& h : r.part)
        h.instance = 0;
    const bool had = r.count > 0;
    r.count = 0;
    r.key = 0;
    if (!keep)
        r.pickup = 0;
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
        "calls, the scene's add and remove, the allocator, the gun data's fields)");
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
        Fail("its three functions couldn't be hooked");
        return false;
    }
    // Turned on last to first: the destructor and setTransform before the builder, so nothing is made before the hook that frees it is in.
    for (int i = made - 1; i >= 0; --i)
        if (MH_EnableHook(reinterpret_cast<void*>(hooks[i].at)) != MH_OK) {
            for (int k = 0; k < made; ++k)
                MH_RemoveHook(reinterpret_cast<void*>(hooks[k].at));
            Fail("its three hooks couldn't be turned on");
            return false;
        }
    g_hooked.store(true);
    Log("Pickups: parts: hooked the builder of a dropped gun's body, its setTransform and its destructor");
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
                   p.fromState ? "the state's list" : "the object's own", no[0] ? no : level >= kPartsOne ? "drawn" : "drawable");
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
}

void SayCounts(ULONGLONG now) {
    if (now - g_countsAt < kCountsMs)
        return;
    g_countsAt = now;
    const uint32_t v[20] = {static_cast<uint32_t>(g_live.load()), static_cast<uint32_t>(g_pickupsWithParts.load()), g_made.load(), g_freed.load(),
                            g_refused.load(), g_notLoaded.load(), g_noSlot.load(), g_noScene.load(), g_misaligned.load(), g_forgotten.load(),
                            g_faults.load(), g_builds.load(), g_moves.load(), g_gones.load(), g_caughtUp.load(), g_drained.load(), g_noAlloc.load(),
                            g_mostMicros.load(), g_movedPickups.load(), 0};
    bool changed = false;  // (builds, moves and the time alone don't make a line: moves may come every frame)
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

// A dropped gun's attachments where it lies (src/pickpartsrules.h, and src/pickparts.cpp compiled in here) on made-up memory laid out as the
// game's: content entries with their unlock sets, blueprints, gun data with states and sockets, socket objects and meshes with vtables whose type
// getters are the game's form, client pickups with their allocator and world, a client world with its lists, the scene's cell. The game's
// functions are the test's own (its builder, setTransform and destructor as the trampolines, the pose, the allocator, the mesh instance's calls),
// every hook called directly. Each check prints PASS or FAIL, then the totals; the exit code is the number failed.
//   T1  the walk of an entry's unlocks: empty, one, seven in key order, the root's flag bit, a child at the anchor, a cycle, a size that lies, the
//       ends the anchor names; the layout the game's copy (00855AA0) and insert (008559B0) read
//   T2  the sockets: an unlock named by two sockets, a socket with no unlock, a socket with two objects, an object of another class, no Asset3p,
//       another bone, an excluded socket, a default object with its unlock absent, over 64 sockets, a state count of 0
//   T3  a part's matrix: from the state's list by its object, else the object's own
//   T4  the placement: Compose against the order of 008549D0-00854AD6 on 20 random matrices; a hand case written out
//   T5  the key: the same unlocks in any order; one more changes it
//   T6  the parts' lives: made in the builder on the pose; a rebuild with the same key places only; a new key frees and makes; a move places on the
//       raw matrix, and a same-name build keeps that base; a body rebuilt at the same address with another MeshSet; gone frees all; no entry, no
//       Mesh3p, a state count of 0; another world; another scene or allocator: forgotten, never freed; a fault in a make; not loaded, no scene slot;
//       an asset that isn't a mesh or whose getter isn't the game's form: nothing called; the catch-up pass; the caps; the drain; level 0; the
//       hooks turned on destructor first; the catch-up going on past drops that want nothing; a moved row gone with a body made again (also
//       while off); the count of pickups with parts after a fault
//   T7  the kinds by name, as the drops of 2026-11-13 carried them
//   T8  the entry the game draws (011BB9D0's two passes)
//   T9  alignment: every matrix handed to the game 16-byte aligned; a misaligned one refused, never handed on
//   T10 what each switch value draws; parts 2 one sight on one pickup
//   T11 the real tables against build\bf3_mem.exe when it's there
//   T12 the collector's rules: slot 47's test, the kinds it draws from a stand-in, a stand-in's every byte, the light's handle, the entry the pose is
//       taken from, a row written and read (a row being written refused), a row's holds (the row written meanwhile: none), the call slots
//   T13 the collector hooked (the game's own collector the test's, reading each element as the game's does): parts 1 passes through; parts 3 hands
//       a stand-in per part after the game's own pass, every field of it, its matrix the part's on the pose taken in that same call; the gun hidden,
//       its body rebuilt, another world or scene, no entry, a state count of 0, no light system or none for its handle, another entity; the cap;
//       no call slot; a fault (nothing more for the session); the MeshSet's kinds; a part not as made; a row being written; gone (the row empty
//       before the free); a free while a call holds the part (waits, or forgets it after 5 ms, never frees it under the call); the census lines
//   T14 the game's thread and a render job at once: parts made and freed thousands of times while the collector is called over and over; no
//       stand-in ever handed with a freed instance; a row read whole or not at all

#include <windows.h>

#include <atomic>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <random>
#include <set>
#include <string>
#include <thread>
#include <vector>

#define BF3VR_PICKUPS_TEST
extern "C" {
typedef enum MH_STATUS { MH_OK = 0, MH_ERROR_NOT_CREATED = 9 } MH_STATUS;
MH_STATUS WINAPI MH_CreateHook(LPVOID target, LPVOID detour, LPVOID* original);
MH_STATUS WINAPI MH_EnableHook(LPVOID target);
MH_STATUS WINAPI MH_RemoveHook(LPVOID target);
}
#include "../src/pickparts.cpp"

namespace {

int g_failures = 0, g_counted = 0;
void Check(bool ok, const char* row, const char* what) {
    ++g_counted;
    if (!ok)
        ++g_failures;
    std::printf("%s %s %s\n", ok ? "PASS" : "FAIL", row, what);
}

std::vector<std::string> g_log;
size_t g_logMark = 0;
void Mark() {
    g_logMark = g_log.size();
}
int Said(const char* text) {
    int n = 0;
    for (size_t i = g_logMark; i < g_log.size(); ++i)
        n += g_log[i].find(text) != std::string::npos;
    return n;
}

// ---- Fake memory: one block, every address 32 bits as the game's ----
uint8_t* g_arena = nullptr;
size_t g_used = 0;
constexpr size_t kArena = 8 << 20;
uint32_t Alloc(size_t size) {
    g_used = (g_used + 15) & ~size_t(15);
    const uint32_t at = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(g_arena + g_used));
    std::memset(g_arena + g_used, 0, size);
    g_used += size;
    return at;
}
template <typename T> void Put(uint32_t at, T v) {
    std::memcpy(reinterpret_cast<void*>(static_cast<uintptr_t>(at)), &v, sizeof(T));
}
template <typename T> T Get(uint32_t at) {
    T v;
    std::memcpy(&v, reinterpret_cast<const void*>(static_cast<uintptr_t>(at)), sizeof(T));
    return v;
}
uint32_t Text(const char* s) {
    const uint32_t at = Alloc(std::strlen(s) + 1);
    std::memcpy(reinterpret_cast<void*>(static_cast<uintptr_t>(at)), s, std::strlen(s) + 1);
    return at;
}
// An array with its count at -4 (the game's form).
uint32_t Array(uint32_t count, uint32_t each) {
    const uint32_t at = Alloc(16 + count * each) + 16;
    Put<uint32_t>(at - 4, count);
    return at;
}

bool ReadMemory(void*, uint32_t at, void* out, uint32_t size) {
    __try {
        std::memcpy(out, reinterpret_cast<const void*>(static_cast<uintptr_t>(at)), size);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
const pickparts::Reader kMem{&ReadMemory, nullptr};

// ---- Types, vtables, getters: the read-only data and code of the made-up image ----
uint32_t g_rdataFrom, g_rdataTo, g_codeFrom, g_codeTo;
uint32_t g_rdataNext, g_codeNext;
uint32_t g_socketObjectCell, g_meshCell;
// A type with that class id, its getter (B8 type C3) in the code part, a vtable in the read-only part whose first slot is that getter.
uint32_t VtableOf(uint16_t classId, bool gameForm = true) {
    const uint32_t type = Alloc(0x20);
    Put<uint16_t>(type + 0x14, classId);
    const uint32_t getter = g_codeNext;
    g_codeNext += 8;
    const uint8_t code[6] = {static_cast<uint8_t>(gameForm ? 0xB8 : 0x55), static_cast<uint8_t>(type), static_cast<uint8_t>(type >> 8),
                             static_cast<uint8_t>(type >> 16), static_cast<uint8_t>(type >> 24), 0xC3};
    std::memcpy(reinterpret_cast<void*>(static_cast<uintptr_t>(getter)), code, 6);
    const uint32_t vtable = g_rdataNext;
    g_rdataNext += 16;
    Put<uint32_t>(vtable, getter);
    return vtable;
}
uint32_t g_objectVt, g_meshVt, g_bodyMeshVt, g_otherVt, g_oddGetterVt;

// ---- The made-up image's parts: a section for the read-only data, one for the code, then the heap ----
void MakeImage() {
    g_used = 0;
    g_rdataFrom = Alloc(0x10000), g_rdataTo = g_rdataFrom + 0x10000, g_rdataNext = g_rdataFrom;
    g_codeFrom = Alloc(0x10000), g_codeTo = g_codeFrom + 0x10000, g_codeNext = g_codeFrom;
    g_socketObjectCell = Alloc(4), g_meshCell = Alloc(4);
    Put<uint16_t>(g_socketObjectCell, 0x0421), Put<uint16_t>(g_socketObjectCell + 2, 0x0421);
    Put<uint16_t>(g_meshCell, 0x0922), Put<uint16_t>(g_meshCell + 2, 0x0925);
    g_objectVt = VtableOf(0x0421);
    g_meshVt = VtableOf(0x0923);
    g_bodyMeshVt = VtableOf(0x0924);
    g_otherVt = VtableOf(0x0300);
    g_oddGetterVt = VtableOf(0x0923, false);
}
pickparts::Image TheImage() {
    pickparts::Image i;
    i.codeFrom = g_codeFrom, i.codeTo = g_codeTo, i.rdataFrom = g_rdataFrom, i.rdataTo = g_rdataTo;
    return i;
}
pickparts::Classes TheClasses() {
    pickparts::Classes c;
    c.socketObject = g_socketObjectCell, c.mesh = g_meshCell;
    return c;
}

// ---- Unlocks, sets, entries ----
uint32_t Unlock(const char* name, uint32_t kind = 0) {
    const uint32_t u = Alloc(0x40);
    Put<uint32_t>(u + 8, Text(name));
    Put<uint32_t>(u + 0x1C, kind);
    return u;
}
// A node: +0 the child with the higher keys, +4 the lower, +8 parent | colour, +0xC the value.
uint32_t Node(uint32_t value) {
    const uint32_t n = Alloc(0x10);
    Put<uint32_t>(n + 0xC, value);
    return n;
}
// A balanced tree of these values (sorted by the caller's order as the game's compare would: unsigned), its anchor at entry+8 as 00855AA0 leaves it.
uint32_t Build(const uint32_t* v, int from, int to, uint32_t parent, uint32_t* nodes) {
    if (from >= to)
        return 0;
    const int mid = (from + to) / 2;
    const uint32_t n = Node(v[mid]);
    nodes[mid] = n;
    Put<uint32_t>(n + 8, parent);
    Put<uint32_t>(n + 4, Build(v, from, mid, n, nodes));
    Put<uint32_t>(n + 0, Build(v, mid + 1, to, n, nodes));
    return n;
}
void SetUnlocks(uint32_t entry, std::vector<uint32_t> values, bool flagBit = false) {
    std::sort(values.begin(), values.end());
    const uint32_t anchor = entry + 8;
    if (values.empty()) {
        Put<uint32_t>(entry + 8, anchor), Put<uint32_t>(entry + 0xC, anchor), Put<uint32_t>(entry + 0x10, flagBit ? 1u : 0u), Put<uint32_t>(entry + 0x14, 0);
        return;
    }
    std::vector<uint32_t> nodes(values.size());
    const uint32_t root = Build(values.data(), 0, static_cast<int>(values.size()), anchor, nodes.data());
    Put<uint32_t>(entry + 8, nodes.back());    // the node reached by +0 links
    Put<uint32_t>(entry + 0xC, nodes.front()); // the node reached by +4 links: the begin
    Put<uint32_t>(entry + 0x10, root | (flagBit ? 1u : 0u));
    Put<uint32_t>(entry + 0x14, static_cast<uint32_t>(values.size()));
}

// ---- A gun: its blueprint, data, state, sockets ----
struct SocketSpec {
    uint32_t unlock;
    const char* bone;
    int objects;            // each with an Asset3p unless noAsset
    bool noAsset = false, otherClass = false, excluded = false, usesDefault = false, assetNotMesh = false, assetOddGetter = false;
    float at[3] = {0.0f, 0.079f, 0.416f};
};
struct GunMade {
    uint32_t blueprint = 0, data = 0, states = 0, mesh3p = 0, sockets = 0, rigid = 0;
    std::vector<uint32_t> socket, object, asset;
};
uint32_t Mesh(const char* name, uint32_t vtable) {
    const uint32_t m = Alloc(0x40);
    Put<uint32_t>(m, vtable);
    Put<uint32_t>(m + 8, Text(name));
    return m;
}
GunMade MakeGun(const char* name, const std::vector<SocketSpec>& specs, uint32_t stateCount = 1, bool rigidList = true) {
    GunMade g;
    g.blueprint = Alloc(0x40);
    Put<uint32_t>(g.blueprint + 8, Text(name));
    g.data = Alloc(0x200);
    Put<uint32_t>(g.blueprint + 0x20, g.data);
    g.states = Array(stateCount ? stateCount : 1, 0x5C);
    Put<uint32_t>(g.states - 4, stateCount);
    Put<uint32_t>(g.data + 0x60, g.states);
    g.mesh3p = Mesh("weapons/an94/an94_3p_Mesh", g_bodyMeshVt);
    Put<uint32_t>(g.states + 0x10, g.mesh3p);
    const uint32_t transforms = Array(1, 64);
    const float id[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    std::memcpy(reinterpret_cast<void*>(static_cast<uintptr_t>(transforms)), id, 64);
    Put<uint32_t>(g.states + 0x44, transforms);
    g.sockets = Array(static_cast<uint32_t>(specs.size()), 4);
    Put<uint32_t>(g.data + 0x134, g.sockets);
    int objects = 0;
    for (const SocketSpec& s : specs)
        objects += s.objects;
    g.rigid = rigidList ? Array(static_cast<uint32_t>(objects), 0x50) : 0;
    Put<uint32_t>(g.states + 0x48, g.rigid);
    int k = 0;
    for (size_t i = 0; i < specs.size(); ++i) {
        const SocketSpec& s = specs[i];
        const uint32_t socket = Alloc(0xC0);
        Put<uint32_t>(g.sockets + 4 * static_cast<uint32_t>(i), socket);
        Put<uint32_t>(socket + 0x90, s.unlock);
        Put<uint32_t>(socket + 0x98, s.bone ? Text(s.bone) : 0);
        Put<uint8_t>(socket + 0xA8, s.usesDefault), Put<uint8_t>(socket + 0xA9, s.excluded);
        const uint32_t list = Array(static_cast<uint32_t>(s.objects), 4);
        Put<uint32_t>(socket + 0x9C, list);
        g.socket.push_back(socket);
        for (int o = 0; o < s.objects; ++o) {
            const uint32_t object = Alloc(0x80);
            Put<uint32_t>(object, s.otherClass ? g_otherVt : g_objectVt);
            uint32_t asset = 0;
            if (!s.noAsset)
                asset = Mesh(s.unlock ? "weapons/accessories/pks-07/pks-07_3p_mesh" : "weapons/an94/an94_rail_3p_mesh", s.assetNotMesh ? g_otherVt : s.assetOddGetter ? g_oddGetterVt : g_meshVt);
            Put<uint32_t>(object + 0x14, asset);
            // Its own Transform: identity at (9, 9, 9), so a part placed from it is told apart.
            float own[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 9, 9, 9, 1};
            std::memcpy(reinterpret_cast<void*>(static_cast<uintptr_t>(object + 0x20)), own, 64);
            Put<uint32_t>(list + 4 * o, object);
            if (g.rigid) {
                float m[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, s.at[0], s.at[1], s.at[2] + 0.01f * o, 1};
                std::memcpy(reinterpret_cast<void*>(static_cast<uintptr_t>(g.rigid + 0x50 * k)), m, 64);
                Put<uint32_t>(g.rigid + 0x50 * k + 0x40, object);
            }
            ++k;
            g.object.push_back(object);
            g.asset.push_back(asset);
        }
    }
    return g;
}

// ---- The game's side: client pickups, the client world, the scene, the game's functions ----
constexpr uint32_t kClientPickupVt = 0x72000040, kClientWorldVt = 0x72000020;
uint32_t g_sceneCell, g_clientWorldCell, g_clientWorld, g_clientType, g_segment, g_entities;
uint32_t g_holder, g_inner, g_allocatorObject, g_worldObject;
// The light handles' system ([023B3F94]): on, blocks for the handles 0x000-0x3FF and 0x400-0x7FF, none for 0x800-0xBFF.
uint32_t g_lightsCell, g_lights;
constexpr uint16_t kGunLight = 0x0405, kLightNoBlock = 0x0805;
int g_pickups = 0;

struct Pickup {
    uint32_t at = 0, content = 0, contentData = 0, entries = 0, transform = 0;
};
// A client pickup with one entry (or none) of that gun and those unlocks.
Pickup MakePickup(const GunMade* gun, std::vector<uint32_t> unlocks, float x = 10.0f, bool useWeaponMesh = true) {
    Pickup p;
    p.at = Alloc(0x180);
    Put<uint32_t>(p.at, kClientPickupVt);
    Put<uint32_t>(p.at + 0x10, g_holder);
    const uint32_t data = Alloc(0x100);
    Put<uint8_t>(data + 0x9D, useWeaponMesh ? 1 : 0);
    Put<uint32_t>(p.at + 0x14, data);
    p.transform = Alloc(64);
    float raw[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, 1, 2, 1};
    std::memcpy(reinterpret_cast<void*>(static_cast<uintptr_t>(p.transform)), raw, 64);
    Put<uint32_t>(p.at + 0x1C, p.transform);
    Put<uint16_t>(p.at + 0x166, kGunLight);  // its handle in the light handles' system
    p.contentData = Alloc(0x100);
    Put<uint32_t>(p.contentData + 0x84, 0);  // its PreferredSlot
    Put<uint32_t>(p.at + 0xA8, p.contentData);
    if (gun) {
        p.entries = Alloc(0xC0);
        Put<uint32_t>(p.entries, gun->blueprint);
        Put<uint32_t>(p.entries + 0xA8, 0);
        SetUnlocks(p.entries, unlocks);
        Put<uint32_t>(p.at + 0xAC, p.entries), Put<uint32_t>(p.at + 0xB0, p.entries + 0xC0);
    } else {
        Put<uint32_t>(p.at + 0xAC, 0), Put<uint32_t>(p.at + 0xB0, 0);
    }
    // In the client world's list.
    Put<uint32_t>(g_entities + 4 * g_pickups, p.at);
    ++g_pickups;
    Put<uint32_t>(g_segment + 4, g_entities + 4 * g_pickups);
    Put<uint32_t>(g_segment + 0x1C, static_cast<uint32_t>(g_pickups));
    return p;
}
// The pose the fake 0084CC50 hands out: the pickup's raw transform, turned 90 degrees about y.
void PoseFor(uint32_t pickup, float* out) {
    const uint32_t transform = Get<uint32_t>(pickup + 0x1C);
    const float x = Get<float>(transform + 48), y = Get<float>(transform + 52), z = Get<float>(transform + 56);
    const float m[16] = {0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, x, y, z, 1};
    std::memcpy(out, m, 64);
}

// What the fakes saw.
struct Instance {
    uint32_t at = 0, mesh = 0;
    bool entered = false, freed = false;
    float last[16] = {};
    int places = 0;
};
std::vector<Instance> g_instances;
Instance* InstanceAt(uint32_t at) {
    for (Instance& i : g_instances)
        if (i.at == at && !i.freed)
            return &i;
    return nullptr;
}
int g_poses = 0, g_allocs = 0, g_constructs = 0, g_enters = 0, g_places = 0, g_leaves = 0, g_releases = 0, g_doubleFrees = 0, g_misalignedSeen = 0;
bool g_allocNull = false, g_constructFaults = false, g_enterFaults = false, g_meshUnloaded = false, g_sceneFull = false;
uint32_t g_setKind = 0;  // the kind the made instances' MeshSets have (their +0)
// T14: the game's thread and a render job at once. A freed instance is marked (its +8), and the collector looks for the mark in what it's handed.
bool g_stress = false;
constexpr uint32_t kFreedMark = 0x0DEAD000;
int g_stressSeen = 0, g_stressBad = 0, g_stressOdd = 0;
std::set<uint32_t> g_unloaded;  // meshes the renderer doesn't have
uint16_t g_nextHandle = 1;
// The game's builder, as 00854B50 leaves +0x110: 0 no entry or no Mesh3p, kept for a state count of 0 or the same mesh, else a new instance.
enum BuildMode { kBuildNew, kBuildSame, kBuildNone, kBuildSameAddressOtherSet };
BuildMode g_buildMode = kBuildNew;
int g_builderCalls = 0, g_moveCalls = 0, g_goneCalls = 0;

uint32_t BodyInstance(const char* setName) {
    const uint32_t inst = Alloc(0x2C);
    const uint32_t holder = Alloc(0x20), set = Alloc(0x60);
    Put<uint32_t>(set + 0x58, Text(setName));
    Put<uint32_t>(holder + 0x10, set);
    Put<uint32_t>(inst, holder);
    Put<uint32_t>(inst + 8, 1);
    return inst;
}

void __fastcall FakeBuild(void* pickupPointer, void*) {
    ++g_builderCalls;
    const uint32_t pickup = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pickupPointer));
    switch (g_buildMode) {
    case kBuildNew: Put<uint32_t>(pickup + 0x110, BodyInstance("weapons/an94/an94_3p_Mesh")); break;
    case kBuildSame: break;
    case kBuildNone: Put<uint32_t>(pickup + 0x110, 0); break;
    case kBuildSameAddressOtherSet: {
        const uint32_t body = Get<uint32_t>(pickup + 0x110);
        const uint32_t holder = Alloc(0x20), set = Alloc(0x60);
        Put<uint32_t>(set + 0x58, Text("weapons/m416/m416_3p_Mesh"));
        Put<uint32_t>(holder + 0x10, set);
        Put<uint32_t>(body, holder);
        break;
    }
    }
}
float g_movedTo[16] = {};
void __fastcall FakeMove(void*, void*, const float* m) {
    ++g_moveCalls;
    std::memcpy(g_movedTo, m, 64);
}
void __fastcall FakeGone(void*, void*) {
    ++g_goneCalls;
}
float* __fastcall FakePose(void* pickup, void*, float* out) {
    ++g_poses;
    if (reinterpret_cast<uintptr_t>(out) & 15)
        ++g_misalignedSeen;  // (movaps would fault here)
    PoseFor(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pickup)), out);
    return out;
}
void* __fastcall FakeAlloc(void* arena, void*, uint32_t size, uint32_t align) {
    ++g_allocs;
    if (g_allocNull || static_cast<uint32_t>(reinterpret_cast<uintptr_t>(arena)) != g_allocatorObject || size != 0x2C || align != 0x10)
        return nullptr;
    return reinterpret_cast<void*>(static_cast<uintptr_t>(Alloc(size)));
}
void* __fastcall FakeConstruct(void* memory, void*, void* mesh, int one, void* world, void* none) {
    ++g_constructs;
    const uint32_t inst = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(memory));
    if (g_constructFaults)
        Put<uint32_t>(0, 1);  // an access violation, as a bad mesh would make inside the game's set-up
    if (one != 1 || none || static_cast<uint32_t>(reinterpret_cast<uintptr_t>(world)) != g_worldObject)
        return nullptr;
    const uint32_t m = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(mesh));
    const uint32_t holder = Alloc(0x20), set = Alloc(0x60);
    Put<uint32_t>(set + 0x58, Text("weapons/accessories/pks-07/pks-07_3p_Mesh"));
    Put<uint32_t>(set, g_setKind);
    Put<uint32_t>(holder + 0x10, set);
    Put<uint32_t>(inst, holder);
    Put<uint32_t>(inst + 8, g_unloaded.count(m) || g_meshUnloaded ? 0u : 1u);
    Put<uint16_t>(inst + 0x24, 0xFFFF);
    Instance i;
    i.at = inst, i.mesh = m;
    g_instances.push_back(i);
    return memory;
}
void __fastcall FakeEnter(void* instance, void*, int zero) {
    ++g_enters;
    if (g_enterFaults)
        Put<uint32_t>(0, 1);  // a fault after the constructor returned
    const uint32_t inst = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(instance));
    if (zero == 0 && !g_sceneFull && Get<uint32_t>(g_sceneCell))
        Put<uint16_t>(inst + 0x24, g_nextHandle++);
    if (Instance* i = InstanceAt(inst))
        i->entered = Get<uint16_t>(inst + 0x24) != 0xFFFF;
}
void __fastcall FakePlace(void* instance, void*, const float* m, float scale) {
    ++g_places;
    if (reinterpret_cast<uintptr_t>(m) & 15) {
        ++g_misalignedSeen;
        return;
    }
    if (Instance* i = InstanceAt(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(instance)))) {
        std::memcpy(i->last, m, 64);
        ++i->places;
    }
    (void)scale;
}
void __fastcall FakeLeave(void* instance, void*) {
    ++g_leaves;
    Put<uint16_t>(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(instance)) + 0x24, 0xFFFF);
}
void __cdecl FakeRelease(void* p) {
    ++g_releases;
    Instance* i = InstanceAt(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(p)));
    if (!i)
        ++g_doubleFrees;
    else
        i->freed = true;
    if (g_stress)
        InterlockedExchange(reinterpret_cast<volatile LONG*>(reinterpret_cast<uintptr_t>(p) + 8), static_cast<LONG>(kFreedMark));
}

// 0084A7C0: the pose from an entry (the same pose as the fake 0084CC50's); a null entry would fault in the game's (0084A7D6).
int g_posesOf = 0, g_poseOfNull = 0, g_poseOfMisaligned = 0;
void __fastcall FakePoseOf(void* pickup, void*, uint32_t entry, float* out) {
    ++g_posesOf;
    if (!entry)
        ++g_poseOfNull;
    if (reinterpret_cast<uintptr_t>(out) & 15)
        ++g_poseOfMisaligned;
    PoseFor(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pickup)), out);
}

// The game's collector as the test's: each element read as 00797FF0 reads it (the entity, its views' mask; of the entity its vtable, +8, +0x1C's
// matrix, +0x110 through slot 47's test of +0x164 and +0x165, the word +0x166), every call kept.
struct Seen {
    uint32_t entity = 0, views = 0, more[2] = {}, vtable = 0, flags = 0, instance = 0, matrixAt = 0;
    uint8_t shown = 0, hiding = 0;
    uint16_t handle = 0;
    bool live = false, zeroElsewhere = false;
    float m[16] = {};
};
std::vector<std::vector<Seen>> g_collected;
bool g_collectFaults = false;  // the second pass faults (as a stand-in the game couldn't read would)
uint32_t g_recordsSeen = 0, g_unusedSeen = 0, g_selfSeen = 0;
void __fastcall FakeCollect(void* self, void*, uint32_t count, const uint32_t* elements, void* unused, void* records) {
    if (g_stress) {
        for (uint32_t i = 0; i < count; ++i) {
            const uint32_t entity = elements[4 * i];
            if (Get<uint32_t>(entity) != kClientPickupVt || Get<uint32_t>(entity + 0x14))
                continue;  // (a pickup's +0x14 is its data; a stand-in's is 0)
            const uint32_t inst = Get<uint32_t>(entity + 0x110);
            volatile LONG* mark = reinterpret_cast<volatile LONG*>(static_cast<uintptr_t>(inst) + 8);
            const LONG before = InterlockedCompareExchange(mark, 0, 0);
            for (int spin = 0; spin < 200; ++spin)
                YieldProcessor();
            const LONG after = InterlockedCompareExchange(mark, 0, 0);
            ++g_stressSeen;
            if (before == static_cast<LONG>(kFreedMark) || after == static_cast<LONG>(kFreedMark))
                ++g_stressBad;
            if ((Get<uint32_t>(entity + 0x1C) & 15) || Get<uint32_t>(entity + 8))
                ++g_stressOdd;
        }
        return;
    }
    g_selfSeen = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(self));
    g_unusedSeen = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(unused));
    g_recordsSeen = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(records));
    std::vector<Seen> call;
    for (uint32_t i = 0; i < count; ++i) {
        const uint32_t* e = elements + 4 * i;
        Seen s;
        s.entity = e[0], s.views = e[1], s.more[0] = e[2], s.more[1] = e[3];
        s.vtable = Get<uint32_t>(s.entity);
        s.flags = Get<uint32_t>(s.entity + 8);
        s.shown = Get<uint8_t>(s.entity + 0x164), s.hiding = Get<uint8_t>(s.entity + 0x165);
        s.instance = s.hiding && !s.shown ? 0 : Get<uint32_t>(s.entity + 0x110);
        s.handle = Get<uint16_t>(s.entity + 0x166);
        s.matrixAt = Get<uint32_t>(s.entity + 0x1C);
        if (s.matrixAt)
            std::memcpy(s.m, reinterpret_cast<const void*>(static_cast<uintptr_t>(s.matrixAt)), 64);
        s.live = s.instance && InstanceAt(s.instance) != nullptr;
        // Every other byte of the first 0x180 zero (a stand-in's).
        bool zero = true;
        for (uint32_t at = 0; at < 0x180 && zero; ++at)
            if (at >= 0x164 && at < 0x168)
                continue;
            else if ((at < 4) || (at >= 0x1C && at < 0x20) || (at >= 0x110 && at < 0x114))
                continue;
            else
                zero = Get<uint8_t>(s.entity + at) == 0;
        s.zeroElsewhere = zero;
        call.push_back(s);
    }
    g_collected.push_back(call);
    if (g_collectFaults && g_collected.size() == 2)
        Put<uint32_t>(0, 1);  // an access violation inside the game's collector, on the stand-ins
}

int LiveInstances() {
    int n = 0;
    for (const Instance& i : g_instances)
        n += !i.freed;
    return n;
}
std::vector<uintptr_t> g_enabled;  // the hooks in the order they were turned on

}  // namespace

extern "C" {
MH_STATUS WINAPI MH_CreateHook(LPVOID target, LPVOID, LPVOID* original) {
    const uintptr_t at = reinterpret_cast<uintptr_t>(target);
    if (at == pickparts::g_at.build)
        *original = reinterpret_cast<void*>(&FakeBuild);
    else if (at == pickparts::g_at.move)
        *original = reinterpret_cast<void*>(&FakeMove);
    else if (at == pickparts::g_at.gone)
        *original = reinterpret_cast<void*>(&FakeGone);
    else if (at == pickparts::g_at.collect)
        *original = reinterpret_cast<void*>(&FakeCollect);
    else
        return MH_ERROR_NOT_CREATED;
    return MH_OK;
}
MH_STATUS WINAPI MH_EnableHook(LPVOID target) {
    g_enabled.push_back(reinterpret_cast<uintptr_t>(target));
    return MH_OK;
}
MH_STATUS WINAPI MH_RemoveHook(LPVOID) {
    return MH_OK;
}
}

namespace camera {
bool SafeRead(uintptr_t address, void* out, size_t size) {
    __try {
        std::memcpy(out, reinterpret_cast<const void*>(address), size);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
}  // namespace camera
namespace logx {
void Write(const char* fmt, ...) {
    char line[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);
    g_log.push_back(line);
}
}  // namespace logx

namespace {

template <typename F> uintptr_t Fn(F f) {
    return reinterpret_cast<uintptr_t>(f);
}

// Everything pickparts.cpp keeps, as at a launch, and a scene of the test's own.
void Scene() {
    using namespace pickparts;
    MakeImage();
    for (Record& r : g_records)
        r = Record{};
    for (Moved& m : g_moved)
        m = Moved{};
    g_movedNext = 0;
    g_catchUpAt = 0, g_catchUpNext = 0, g_movedStale = false;
    g_enabled.clear();
    g_level = 0, g_hooked = false, g_noMakes = false, g_live = 0, g_pickupsWithParts = 0;
    for (auto* a : {&g_made, &g_freed, &g_refused, &g_notLoaded, &g_noSlot, &g_noScene, &g_misaligned, &g_faults, &g_forgotten, &g_builds, &g_moves,
                    &g_gones, &g_caughtUp, &g_drained, &g_noAlloc, &g_mostMicros, &g_goneScene, &g_goneArena, &g_movedPickups, &g_unloadedAsset})
        a->store(0);
    for (auto* t : {&g_buildThread, &g_moveThread, &g_goneThread, &g_tickThread, &g_faultCode})
        t->store(0);
    g_build = nullptr, g_moveOriginal = nullptr, g_goneOriginal = nullptr, g_collectOriginal = nullptr;
    // The collector's side.
    for (SharedRow& r : g_rows) {
        r.seq = 0;
        for (auto& w : r.word)
            w = 0;
    }
    g_rowsLive = 0;
    for (CallHolds& h : g_holds) {
        h.busy = 0;
        for (auto& i : h.instance)
            i = 0;
    }
    g_drawOn = true;
    for (auto* a : {&g_calls, &g_callsDrawn, &g_standins, &g_mostElements, &g_atOnce, &g_mostAtOnce, &g_noCall, &g_rowBusy, &g_overCap, &g_notAsMade,
                    &g_refusedKind, &g_drawFaults, &g_heldWaits, &g_heldForgotten, &g_drawFaultCode, &g_notesMade})
        a->store(0);
    for (auto& t : g_collectThread)
        t = 0;
    for (MadeNote& n : g_notes)
        n.number = 0;
    for (DWORD& t : g_collectSaid)
        t = 0;
    g_drawFaultSaid = false, g_notesSaid = 0;
    g_heldWaitMs = 5;
    g_verified = false, g_failed = false, g_levelSaid = -1, g_censusAt = 0, g_countsAt = 0, g_countsSaidAt = 0;
    std::memset(g_countsLast, 0, sizeof(g_countsLast));
    for (bool& b : g_threadsSaid)
        b = false;
    g_p6Said = g_faultSaid = g_unloadedSaid = false;
    for (SaidKey& s : g_saidCensus)
        s = SaidKey{};
    g_saidNext = 0;
    std::memset(g_saidOwnMesh, 0, sizeof(g_saidOwnMesh));
    g_saidOwnNext = 0;
    g_checks = Checks{nullptr, 0, nullptr, 0, nullptr, 0};

    g_at.build = 0x7300B000, g_at.move = 0x7300B010, g_at.gone = 0x7300B020, g_at.collect = 0x7300B030;
    g_at.pose = Fn(&FakePose), g_at.alloc = Fn(&FakeAlloc), g_at.construct = Fn(&FakeConstruct), g_at.enter = Fn(&FakeEnter), g_at.place = Fn(&FakePlace),
    g_at.leave = Fn(&FakeLeave), g_at.release = Fn(&FakeRelease), g_at.poseOf = Fn(&FakePoseOf);
    g_at.image = TheImage();
    g_at.socketObjectRange = g_socketObjectCell, g_at.meshRange = g_meshCell;
    g_at.clientPickupVt = kClientPickupVt, g_at.clientWorldVt = kClientWorldVt;
    g_sceneCell = Alloc(4);
    Put<uint32_t>(g_sceneCell, Alloc(0x40));
    g_at.scene = g_sceneCell;
    // The pickups' allocator and world, and the client world with one segment of pickups.
    g_allocatorObject = Alloc(0x40), g_worldObject = Alloc(0x40);
    g_holder = Alloc(0x10), g_inner = Alloc(0x40);
    Put<uint32_t>(g_holder + 4, g_inner);
    Put<uint32_t>(g_inner + 0x34, g_allocatorObject), Put<uint32_t>(g_inner + 0x38, g_worldObject);
    g_clientWorldCell = Alloc(4), g_clientWorld = Alloc(0x900), g_clientType = Alloc(0x20);
    Put<uint32_t>(g_clientWorldCell, g_clientWorld);
    Put<uint32_t>(g_clientWorld, kClientWorldVt);
    Put<uint16_t>(g_clientWorld + 0x834, 0x100);
    Put<uint16_t>(g_clientType + 0x14, 0x101);
    const uint32_t lists = Alloc(0x20);
    Put<uint32_t>(g_clientWorld + 0x820, lists), Put<uint32_t>(g_clientWorld + 0x824, lists + 0x20);
    g_segment = Alloc(0x20), g_entities = Alloc(4 * 256);
    Put<uint32_t>(lists + 8, g_segment);
    Put<uint32_t>(g_segment, g_entities), Put<uint32_t>(g_segment + 4, g_entities), Put<uint32_t>(g_segment + 0x1C, 0), Put<uint32_t>(g_segment + 0x18, 0);
    g_at.clientWorld = g_clientWorldCell, g_at.clientPickupType = g_clientType;
    g_pickups = 0;
    g_lightsCell = Alloc(4), g_lights = Alloc(0x200);
    Put<uint8_t>(g_lights + 0x144, 1);
    Put<uint32_t>(g_lights + 0, Alloc(0x10)), Put<uint32_t>(g_lights + 4, Alloc(0x10)), Put<uint32_t>(g_lights + 8, 0);
    Put<uint32_t>(g_lightsCell, g_lights);
    g_at.lights = g_lightsCell;

    g_log.clear();
    g_logMark = 0;
    g_instances.clear();
    g_poses = g_allocs = g_constructs = g_enters = g_places = g_leaves = g_releases = g_doubleFrees = g_misalignedSeen = 0;
    g_allocNull = g_constructFaults = g_enterFaults = g_meshUnloaded = g_sceneFull = false;
    g_unloaded.clear();
    g_buildMode = kBuildNew;
    g_builderCalls = g_moveCalls = g_goneCalls = 0;
    g_setKind = 0;
    g_stress = false;
    g_stressSeen = g_stressBad = g_stressOdd = 0;
    g_posesOf = g_poseOfNull = g_poseOfMisaligned = 0;
    g_collected.clear();
    g_collectFaults = false;
}

ULONGLONG g_now = 1000000;
// The switch at a value, a frame of the render thread (the checks and the hooks go in).
void Level(int parts, int pickupsLevel = 2) {
    g_now += 1100;
    pickparts::OnFrame(g_now, pickupsLevel, parts, true);
}
void Build(const Pickup& p) {
    pickparts::BuildHook(reinterpret_cast<void*>(static_cast<uintptr_t>(p.at)), nullptr);
}
void Move(const Pickup& p, const float* m) {
    pickparts::MoveHook(reinterpret_cast<void*>(static_cast<uintptr_t>(p.at)), nullptr, m);
}
// Its end, as the game's: the destructor, then out of the client world's list.
void Gone(const Pickup& p) {
    pickparts::GoneHook(reinterpret_cast<void*>(static_cast<uintptr_t>(p.at)), nullptr);
    for (int i = 0; i < g_pickups; ++i)
        if (Get<uint32_t>(g_entities + 4 * i) == p.at) {
            Put<uint32_t>(g_entities + 4 * i, Get<uint32_t>(g_entities + 4 * (g_pickups - 1)));
            --g_pickups;
            Put<uint32_t>(g_segment + 4, g_entities + 4 * g_pickups);
            Put<uint32_t>(g_segment + 0x1C, static_cast<uint32_t>(g_pickups));
            break;
        }
}
void Tick() {
    g_now += 1100;
    pickparts::GameTick(g_now);
}
// One call of the collector as a render job makes it (017A6683-017A66A0): these entities' elements (their views' mask, two more words), a word it
// doesn't read, the job's records.
constexpr uint32_t kViews = 0x5, kRenderer = 0x74000100, kUnused = 0x74000200, kRecords = 0x74000300;
void Collect(const std::vector<uint32_t>& entities) {
    std::vector<uint32_t> e;
    for (uint32_t at : entities)
        e.insert(e.end(), {at, kViews, 0x11, 0x22});
    if (!g_stress)
        g_collected.clear();
    pickparts::CollectHook(reinterpret_cast<void*>(static_cast<uintptr_t>(kRenderer)), nullptr, static_cast<uint32_t>(entities.size()), e.data(),
                           reinterpret_cast<void*>(static_cast<uintptr_t>(kUnused)), reinterpret_cast<void*>(static_cast<uintptr_t>(kRecords)));
}
// The stand-ins of the last call (its second pass, if it had one).
const std::vector<Seen>& Handed() {
    static const std::vector<Seen> none;
    return g_collected.size() >= 2 ? g_collected[1] : none;
}

bool Near(const float* a, const float* b, int n = 16, float tolerance = 1e-5f) {
    for (int i = 0; i < n; ++i)
        if (std::fabs(a[i] - b[i]) > tolerance)
            return false;
    return true;
}

// The AN-94 of the drops: its own unlock, the Kobra, the PSO-1, NoOptics, AI_Unlock, a foregrip, an extended magazine; sockets as SocketSpec says.
struct An94 {
    uint32_t own, kobra, pso, noOptics, ai, foregrip, mag;
    GunMade gun;
};
An94 MakeAn94(bool rigidList = true) {
    An94 a;
    a.own = Unlock("Weapons/AN94/U_AN94");
    a.kobra = Unlock("Weapons/AN94/U_AN94_Kobra");
    a.pso = Unlock("Weapons/AN94/U_AN94_PSO-1");
    a.noOptics = Unlock("Weapons/Common/NoOptics");
    a.ai = Unlock("Weapons/Common/AI_Unlock");
    a.foregrip = Unlock("Weapons/AN94/U_AN94_Foregrip");
    a.mag = Unlock("Weapons/AN94/U_AN94_ExtendedMag");
    std::vector<SocketSpec> s = {
        {0, "Wep_Root", 1},                      // 0: no unlock (always on)
        {a.kobra, "Wep_Root", 1},                // 1
        {a.pso, "", 1},                          // 2: an empty bone name
        {a.foregrip, "Wep_Root", 1},             // 3
        {a.mag, "Wep_Root", 1},                  // 4
        {a.kobra, "Wep_Grip", 1},                // 5: the Kobra's second socket, on another bone
    };
    s[1].at[0] = 0.0f, s[1].at[1] = 0.08f, s[1].at[2] = 0.30f;
    a.gun = MakeGun("Weapons/AN94/AN94", s, 1, rigidList);
    return a;
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------

void T1() {
    Scene();
    using namespace pickparts;
    uint32_t out[16];
    const char* why = "";
    const uint32_t entry = Alloc(0xC0);
    SetUnlocks(entry, {});
    Check(WalkSet(kMem, entry, out, 16, &why) == 0, "T1", "an empty set (the anchor's self-links, root 0, size 0): none");
    const uint32_t a = Unlock("A");
    SetUnlocks(entry, {a});
    Check(WalkSet(kMem, entry, out, 16) == 1 && out[0] == a, "T1", "one node: one");
    std::vector<uint32_t> seven;
    for (int i = 0; i < 7; ++i)
        seven.push_back(Unlock("U"));
    std::vector<uint32_t> shuffled = {seven[3], seven[6], seven[0], seven[5], seven[1], seven[4], seven[2]};
    SetUnlocks(entry, shuffled);
    bool ordered = WalkSet(kMem, entry, out, 16) == 7;
    for (int i = 0; i < 7 && ordered; ++i)
        ordered = out[i] == seven[i];  // (Unlock hands out rising addresses)
    Check(ordered, "T1", "a balanced seven: all seven, lowest key first (the +4 child first, as the insert's setb sends lower keys to +4)");
    SetUnlocks(entry, shuffled, true);
    Check(WalkSet(kMem, entry, out, 16) == 7, "T1", "the root word's flag bit (the anchor's colour) ignored");
    // A child pointing at the anchor.
    SetUnlocks(entry, {seven[0], seven[1], seven[2]});
    const uint32_t root = Get<uint32_t>(entry + 0x10) & ~1u;
    const uint32_t lower = Get<uint32_t>(root + 4);
    Put<uint32_t>(lower + 4, entry + 8);
    Check(WalkSet(kMem, entry, out, 16, &why) == -1 && std::strstr(why, "anchor"), "T1", "a child pointing at the anchor: refused");
    // A cycle.
    SetUnlocks(entry, {seven[0], seven[1], seven[2]});
    const uint32_t root2 = Get<uint32_t>(entry + 0x10) & ~1u;
    Put<uint32_t>(Get<uint32_t>(root2 + 0), root2);
    Check(WalkSet(kMem, entry, out, 16, &why) == -1, "T1", "a cycle: refused (stops by 64 steps or by its size)");
    SetUnlocks(entry, {seven[0], seven[1], seven[2], seven[3], seven[4]});
    Put<uint32_t>(entry + 0x14, 3);
    Check(WalkSet(kMem, entry, out, 16, &why) == -1 && std::strstr(why, "more nodes"), "T1", "a size of 3 with 5 nodes: refused");
    SetUnlocks(entry, {seven[0], seven[1], seven[2]});
    Put<uint32_t>(entry + 0x14, 17);
    Check(WalkSet(kMem, entry, out, 16, &why) == -1, "T1", "a size over 16: refused");
    SetUnlocks(entry, {seven[0], seven[1], seven[2]});
    Put<uint32_t>(entry + 0xC, Get<uint32_t>(entry + 8));
    Check(WalkSet(kMem, entry, out, 16, &why) == -1 && std::strstr(why, "ends"), "T1", "the begin the anchor names isn't the walk's first: refused");
    SetUnlocks(entry, {seven[0], seven[1], seven[2]});
    Put<uint32_t>(Get<uint32_t>(entry + 0x10) + 0xC, 0);
    Check(WalkSet(kMem, entry, out, 16, &why) == -1, "T1", "a node holding nothing: refused");
    // The layout of a real drop as read in the game on 2026-11-13 (entry DF4F4E80: its nodes inside the entry at +0x38, +0x48, +0x28; three names).
    {
        const uint32_t e = Alloc(0x100);
        const uint32_t kobra = Unlock("Weapons/AN94/U_AN94_Kobra"), own = Unlock("Weapons/AN94/U_AN94"), ai = Unlock("Weapons/Common/AI_Unlock");
        // In key order: kobra < own < ai (their addresses). The root the middle one at +0x48, its +4 child at +0x38, its +0 child at +0x28.
        const uint32_t n38 = e + 0x38, n48 = e + 0x48, n28 = e + 0x28;
        Put<uint32_t>(n48 + 0xC, own), Put<uint32_t>(n48 + 4, n38), Put<uint32_t>(n48 + 0, n28), Put<uint32_t>(n48 + 8, e + 8);
        Put<uint32_t>(n38 + 0xC, kobra), Put<uint32_t>(n38 + 8, n48);
        Put<uint32_t>(n28 + 0xC, ai), Put<uint32_t>(n28 + 8, n48 | 1);
        Put<uint32_t>(e + 8, n28), Put<uint32_t>(e + 0xC, n38), Put<uint32_t>(e + 0x10, n48), Put<uint32_t>(e + 0x14, 3);
        Check(WalkSet(kMem, e, out, 16) == 3 && out[0] == kobra && out[1] == own && out[2] == ai, "T1",
              "a drop's set laid out as a real one was read (its nodes inside the entry): its three unlocks");
    }
}

void T2() {
    Scene();
    using namespace pickparts;
    An94 a = MakeAn94();
    Gun g;
    const char* why = "";
    const uint32_t set[] = {a.own, a.kobra, a.ai};
    Check(ReadGun(kMem, TheImage(), TheClasses(), a.gun.blueprint, set, 3, g, &why), "T2", "the gun's data read");
    int kobraParts = 0, bare = 0, other = 0;
    for (int i = 0; i < g.parts; ++i) {
        kobraParts += g.part[i].unlock == a.kobra;
        bare += g.part[i].unlock == 0;
        other += g.part[i].unlock && g.part[i].unlock != a.kobra;
    }
    Check(kobraParts == 2 && bare == 1 && other == 0 && g.otherBones == 1, "T2",
          "the Kobra named by two sockets (one on another bone, counted), the socket with no unlock in, the absent unlocks' sockets out");
    Check(g.stateCount == 1 && g.socketCount == 6 && g.rootIdentity && g.mesh3p == a.gun.mesh3p && g.mesh3pVt == g_bodyMeshVt && g.mesh3pClass == 0x924,
          "T2", "its state, sockets, the body's Mesh3p with its vtable and class, the root bone's identity");
    const Part* root = nullptr;
    const Part* grip = nullptr;
    for (int i = 0; i < g.parts; ++i)
        if (g.part[i].unlock == a.kobra)
            (g.part[i].rootBone ? root : grip) = &g.part[i];
    Check(root && grip && root->objectOk && root->assetOk && root->kind == kSight && !NotDrawn(*root, kPartsSights)[0] &&
              std::strcmp(NotDrawn(*grip, kPartsSights), "its socket is on another bone") == 0,
          "T2", "the Kobra on Wep_Root drawable; on Wep_Grip not, and why");
    // Two objects, another class, no asset, an excluded socket, a default object with its unlock absent, an odd getter, an asset of another class.
    const uint32_t u1 = Unlock("U_X_RX01"), u2 = Unlock("U_X_ACOG"), u3 = Unlock("U_X_EOTech"), u4 = Unlock("U_X_Silencer"), u5 = Unlock("U_X_M145"),
                   u6 = Unlock("U_X_PKA");
    std::vector<SocketSpec> s = {{u1, "Wep_Root", 2}, {u2, "Wep_Root", 1}, {u3, "Wep_Root", 1}, {u4, "Wep_Root", 1}, {u5, "Wep_Root", 1},
                                 {u6, "Wep_Root", 1}, {Unlock("U_X_Bipod"), "Wep_Root", 1}};
    s[1].otherClass = true, s[2].noAsset = true, s[3].excluded = true, s[4].assetOddGetter = true, s[5].assetNotMesh = true, s[6].usesDefault = true;
    GunMade x = MakeGun("Weapons/X/X", s);
    const uint32_t xs[] = {u1, u2, u3, u4, u5, u6};
    Check(ReadGun(kMem, TheImage(), TheClasses(), x.blueprint, xs, 6, g) && g.parts == 7 && g.absentDefaults == 1, "T2",
          "a socket with two objects gives two; a default object with its unlock absent counted, not read");
    Check(std::strcmp(NotDrawn(g.part[2], kPartsAll), "its object isn't a regular socket object") == 0 && !g.part[2].asset, "T2",
          "an object of another class: skipped, its Asset3p never read");
    Check(std::strcmp(NotDrawn(g.part[3], kPartsAll), "no 3P asset") == 0, "T2", "no Asset3p: skipped");
    Check(std::strcmp(NotDrawn(g.part[4], kPartsAll), "its socket is excluded") == 0, "T2", "an excluded socket: skipped");
    Check(std::strcmp(NotDrawn(g.part[5], kPartsAll), "its 3P asset isn't a mesh") == 0 && g.part[5].assetVt == g_oddGetterVt, "T2",
          "an asset whose getter isn't the game's form (mov eax, type; ret): not a mesh");
    Check(std::strcmp(NotDrawn(g.part[6], kPartsAll), "its 3P asset isn't a mesh") == 0, "T2", "an asset of another class: not a mesh");
    // Over 64 sockets; a state count of 0.
    std::vector<SocketSpec> many(65, SocketSpec{0, "Wep_Root", 1});
    GunMade big = MakeGun("Weapons/Y/Y", many);
    Check(!ReadGun(kMem, TheImage(), TheClasses(), big.blueprint, xs, 0, g, &why) && g.socketsCut, "T2", "over 64 sockets: refused");
    GunMade none = MakeGun("Weapons/Z/Z", {{u1, "Wep_Root", 1}}, 0);
    Put<uint32_t>(none.states + 0x10, 0xDEAD0000);  // a state that must not be read
    Check(ReadGun(kMem, TheImage(), TheClasses(), none.blueprint, xs, 1, g) && g.stateCount == 0 && !g.mesh3p && g.parts == 1 && !g.part[0].fromState &&
              g.part[0].rel[12] == 9.0f,
          "T2", "a state count of 0: no state read (the part's place its object's own)");
}

void T3() {
    Scene();
    using namespace pickparts;
    An94 a = MakeAn94();
    float m[16];
    bool fromState = false;
    Check(RigidFor(kMem, a.gun.states, a.gun.object[1], m, &fromState) && fromState && Near(m + 12, std::vector<float>{0.0f, 0.08f, 0.30f}.data(), 3),
          "T3", "found by its object in the state's list of 0x50 entries");
    const uint32_t stranger = Alloc(0x80);
    float own[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 4, 5, 6, 1};
    std::memcpy(reinterpret_cast<void*>(static_cast<uintptr_t>(stranger + 0x20)), own, 64);
    Check(RigidFor(kMem, a.gun.states, stranger, m, &fromState) && !fromState && Near(m, own), "T3", "not in the list: the object's own Transform");
}

// 008549D0-00854AD6 written out lane by lane.
void ReferenceCompose(const float* M, const float* B, float* out) {
    for (int i = 0; i < 4; ++i)
        for (int k = 0; k < 4; ++k)
            out[4 * i + k] = M[4 * i] * B[k] + M[4 * i + 1] * B[4 + k] + M[4 * i + 2] * B[8 + k] + (i == 3 ? B[12 + k] : 0.0f);
}

void T4() {
    std::mt19937 rng(7);
    std::uniform_real_distribution<float> d(-2.0f, 2.0f);
    bool same = true;
    for (int n = 0; n < 20; ++n) {
        float m[16], b[16], a[16], r[16];
        for (int i = 0; i < 16; ++i)
            m[i] = d(rng), b[i] = d(rng);
        pickparts::Compose(m, b, a);
        ReferenceCompose(m, b, r);
        same = same && Near(a, r, 16, 1e-5f);
    }
    Check(same, "T4", "Compose against 00854960's order on 20 random matrices");
    // A pose turned 90 degrees about y at (1, 2, 3); the part at (0, 0.079, 0.416) on the body.
    const float base[16] = {0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 2, 3, 1};
    const float rel[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0.079f, 0.416f, 1};
    float out[16];
    pickparts::Compose(rel, base, out);
    const float want[16] = {0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1.416f, 2.079f, 3, 1};
    Check(Near(out, want), "T4", "the hand case: (0, 0.079, 0.416) on a pose turned 90 degrees at (1, 2, 3) lies at (1.416, 2.079, 3)");
    float inPlace[16];
    std::memcpy(inPlace, rel, sizeof(rel));
    pickparts::Compose(inPlace, base, inPlace);
    Check(Near(inPlace, want), "T4", "into its own input: the same");
}

void T5() {
    const uint32_t a[] = {0x1000, 0x3000, 0x2000}, b[] = {0x3000, 0x2000, 0x1000}, c[] = {0x1000, 0x3000, 0x2000, 0x4000};
    Check(pickparts::SetKey(a, 3) == pickparts::SetKey(b, 3), "T5", "the same unlocks in another order: the same key");
    Check(pickparts::SetKey(a, 3) != pickparts::SetKey(c, 4) && pickparts::SetKey(a, 3) != pickparts::SetKey(a, 2), "T5", "one more, one less: another key");
}

void T7() {
    using namespace pickparts;
    struct Row {
        const char* name;
        Kind kind;
    };
    const Row rows[] = {{"Weapons/AN94/U_AN94_Kobra", kSight},      {"Weapons/AN94/U_AN94_PSO-1", kSight},   {"Weapons/AN94/U_AN94_PKS-07", kSight},
                        {"Weapons/AN94/U_AN94_PK-AS", kSight},      {"Weapons/AN94/U_AN94_PKA", kSight},     {"Weapons/M416/U_M416_ACOG", kSight},
                        {"Weapons/M416/U_M416_RX01", kSight},       {"Weapons/X/U_X_EOTech", kSight},        {"Weapons/X/U_X_M145", kSight},
                        {"Weapons/X/U_X_IRNV", kSight},             {"Weapons/X/U_X_FLIR", kSight},          {"Weapons/X/U_X_RifleScope", kSight},
                        {"Weapons/X/U_X_Ballistic_20x", kSight},    {"Weapons/Common/NoOptics", kNoPicture}, {"Weapons/Common/AI_Unlock", kNoPicture},
                        {"Weapons/AN94/U_AN94", kOther},            {"Weapons/AN94/U_AN94_Foregrip", kRail}, {"Weapons/M416/U_M416_40mm_HE", kRail},
                        {"Weapons/X/U_X_Bipod", kRail},             {"Weapons/X/U_X_Flashlight", kRail},     {"Weapons/X/U_X_TacticalLight", kRail},
                        {"Weapons/X/U_X_Laser", kRail},             {"Weapons/X/U_X_Silencer", kBarrel},     {"Weapons/X/U_X_FlashSuppressor", kBarrel},
                        {"Weapons/X/U_X_HeavyBarrel", kNoPicture},  {"Weapons/X/U_X_ExtendedMag", kMagazine}, {"Weapons/X/U_X_Flechette", kNoPicture},
                        {"Weapons/X/U_X_Frag", kNoPicture},         {"Weapons/PKP/U_PKP", kOther},           {"", kOther},
                        {"weapons/accessories/pks-07/pks-07_3p_mesh", kSight}};
    int wrong = 0;
    for (const Row& r : rows)
        if (KindOf(r.name) != r.kind) {
            ++wrong;
            std::printf("      %s: %s, not %s\n", r.name, KindName(KindOf(r.name)), KindName(r.kind));
        }
    Check(wrong == 0, "T7", "the kinds by name (the drops' unlocks of 2026-11-13, the campaign's attachments, unknown names \"other\")");
}

void T8() {
    Scene();
    using namespace pickparts;
    const uint32_t data = Alloc(0x100), entries = Alloc(0xC0 * 4), list = Alloc(0x10);
    Put<uint32_t>(list, data), Put<uint32_t>(list + 4, entries), Put<uint32_t>(list + 8, entries + 0xC0 * 4);
    auto entry = [&](int i, uint32_t blueprint, uint32_t slot) {
        Put<uint32_t>(entries + 0xC0 * i, blueprint);
        Put<uint32_t>(entries + 0xC0 * i + 0xA8, slot);
    };
    int pass = 0;
    Put<uint32_t>(data + 0x84, 1);
    entry(0, 0x5000, 3), entry(1, 0, 1), entry(2, 0x5100, 1), entry(3, 0x5200, 0);
    Check(PickEntry(kMem, list, &pass) == entries + 0xC0 * 2 && pass == 1, "T8", "the preferred slot with a blueprint wins, one with none passed over");
    Put<uint32_t>(data + 0x84, 7);
    Check(PickEntry(kMem, list, &pass) == entries + 0xC0 * 3 && pass == 2, "T8", "no match: the lowest slot with a blueprint");
    entry(3, 0x5200, 3);
    Check(PickEntry(kMem, list, &pass) == entries + 0xC0 * 2, "T8", "no match: the lowest slot (1)");
    entry(2, 0x5100, 3);
    entry(1, 0x5300, 3);
    Check(PickEntry(kMem, list, &pass) == entries, "T8", "equal slots: the first");
    for (int i = 0; i < 4; ++i)
        entry(i, 0, i);
    Check(PickEntry(kMem, list, &pass) == 0 && pass == 0, "T8", "no blueprint at all: none");
    Put<uint32_t>(list + 4, 0), Put<uint32_t>(list + 8, 0);
    Check(PickEntry(kMem, list, &pass) == 0, "T8", "no entries: none");
}

// Up and hooked at a level, no rows to check (the test's own image).
void Up(int parts) {
    Level(parts);
}

void T6() {
    using namespace pickparts;
    // Made in the builder on the pose; then a rebuild with the same key places only.
    Scene();
    An94 a = MakeAn94();
    Pickup p = MakePickup(&a.gun, {a.own, a.kobra, a.ai});
    Up(kPartsSights);
    Check(g_hooked && Said("Pickups: parts: hooked the builder") == 1, "T6", "parts 3: hooked");
    Check(g_enabled.size() == 4 && g_enabled[0] == g_at.gone && g_enabled[1] == g_at.move && g_enabled[2] == g_at.collect && g_enabled[3] == g_at.build,
          "T6", "the hooks turned on the destructor first, the collector before the builder, the builder last: nothing made before the hooks that free "
                "it and draw it");
    Build(p);
    float pose[16];
    PoseFor(p.at, pose);
    const float kobraRel[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0.08f, 0.30f, 1};
    const float psoRel[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0.079f, 0.416f, 1};
    float expect[16];
    Compose(kobraRel, pose, expect);
    Check(Live() == 1 && LiveInstances() == 1 && g_instances[0].entered && g_instances[0].mesh == a.gun.asset[1] && Near(g_instances[0].last, expect) &&
              g_builderCalls == 1,
          "T6", "a drop built: its Kobra made in the builder, entered, placed on the pose times its matrix");
    if (g_instances.empty())
        return;
    const int places = g_instances[0].places;
    g_buildMode = kBuildSame;
    Build(p);
    Check(Live() == 1 && g_constructs == 1 && g_instances[0].places == places + 1 && Near(g_instances[0].last, expect), "T6",
          "the same key again: nothing made, placed again on the same base");
    // A new key (the content's unlocks changed: the PSO-1): the old freed, the new made, on the base the game last used.
    SetUnlocks(p.entries, {a.own, a.pso, a.ai});
    Build(p);
    Compose(psoRel, pose, expect);
    Check(Live() == 1 && g_releases == 1 && g_leaves == 1 && g_instances.size() == 2 && g_instances[1].mesh == a.gun.asset[2] &&
              !InstanceAt(g_instances[0].at) && Near(g_instances[1].last, expect),
          "T6", "a new key: the Kobra freed by the game's pair, the PSO-1 made on the same base");
    // A move: on the raw matrix; then a same-name build with other unlocks makes them on that base, not the pose.
    alignas(16) const float raw[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 50, 60, 70, 1};
    Move(p, raw);
    Compose(psoRel, raw, expect);
    Check(g_moveCalls == 1 && Near(g_instances[1].last, expect), "T6", "a move: the game's own first, then ours placed on the raw matrix");
    SetUnlocks(p.entries, {a.own, a.kobra, a.ai});
    const int posesBefore = g_poses;
    Build(p);
    Compose(kobraRel, raw, expect);
    Check(Live() == 1 && g_instances.size() == 3 && Near(g_instances[2].last, expect) && g_poses == posesBefore, "T6",
          "moved, then the same body with other unlocks (00854BCF): made on the raw matrix, the pose not asked");
    // A body rebuilt at the same address with another MeshSet: rebuilt, so the pose again.
    g_buildMode = kBuildSameAddressOtherSet;
    Build(p);
    Compose(kobraRel, pose, expect);
    Check(Live() == 1 && Near(g_instances[2].last, expect) && g_poses == posesBefore + 1, "T6",
          "the same body address naming another MeshSet: rebuilt, placed on the pose");
    // Gone: all freed, once; gone again: nothing.
    const int releases = g_releases;
    Gone(p);
    Check(Live() == 0 && LiveInstances() == 0 && g_releases == releases + 1 && g_goneCalls == 1 && !RecordOf(p.at), "T6",
          "gone: freed before the game's own destructor, the record gone");
    Gone(p);
    Check(g_releases == releases + 1 && g_doubleFrees == 0, "T6", "gone again: nothing freed twice");

    // No entry after the call; no Mesh3p; a state count of 0.
    g_buildMode = kBuildNew;
    Pickup q = MakePickup(&a.gun, {a.own, a.kobra});
    Build(q);
    Put<uint32_t>(q.at + 0xAC, 0), Put<uint32_t>(q.at + 0xB0, 0);
    g_buildMode = kBuildNone;
    Build(q);
    Check(Live() == 0 && LiveInstances() == 0 && !RecordOf(q.at), "T6", "no entry after the call: all freed");
    Put<uint32_t>(q.at + 0xAC, q.entries), Put<uint32_t>(q.at + 0xB0, q.entries + 0xC0);
    g_buildMode = kBuildNew;
    Build(q);
    g_buildMode = kBuildNone;
    Build(q);
    Check(Live() == 0 && LiveInstances() == 0, "T6", "+0x110 0 after the call (no Mesh3p): all freed");
    g_buildMode = kBuildNew;
    Build(q);
    const int placesNow = g_places, releasesNow = g_releases;
    Put<uint32_t>(a.gun.states - 4, 0);
    g_buildMode = kBuildSame;
    SetUnlocks(q.entries, {a.own, a.pso});
    Build(q);
    Check(Live() == 1 && g_places == placesNow && g_releases == releasesNow, "T6", "a WeaponStates count of 0: kept, untouched (the game's body too)");
    Put<uint32_t>(a.gun.states - 4, 1);
    Gone(q);

    // Another world: a pickup that still reads as itself keeps its parts; one that doesn't is forgotten, nothing of it touched.
    g_buildMode = kBuildNew;
    Pickup w = MakePickup(&a.gun, {a.own, a.kobra});
    Build(w);
    const uint32_t newWorld = Alloc(0x900);
    std::memcpy(reinterpret_cast<void*>(static_cast<uintptr_t>(newWorld)), reinterpret_cast<void*>(static_cast<uintptr_t>(g_clientWorld)), 0x900);
    Put<uint32_t>(g_clientWorldCell, newWorld);
    Tick();
    Check(Live() == 1 && RecordOf(w.at), "T6", "another world, the pickup still itself: kept for its destructor");
    Put<uint32_t>(w.at, 0x12345678);  // its memory reused
    const int releasesW = g_releases, leavesW = g_leaves;
    Tick();
    Check(Live() == 0 && g_forgotten == 1 && g_releases == releasesW && g_leaves == leavesW && !RecordOf(w.at), "T6",
          "another world, the pickup gone: forgotten, never freed, nothing of it touched; the render side never forgets");
    Put<uint32_t>(g_clientWorldCell, g_clientWorld);

    // Another scene, another allocator: forgotten, never freed.
    Pickup s1 = MakePickup(&a.gun, {a.own, a.kobra});
    Build(s1);
    const uint32_t scene = Get<uint32_t>(g_sceneCell);
    Put<uint32_t>(g_sceneCell, Alloc(0x40));
    const int releasesS = g_releases;
    Gone(s1);
    Check(Live() == 0 && g_forgotten == 2 && g_releases == releasesS, "T6", "another scene at its end: forgotten, not freed");
    Put<uint32_t>(g_sceneCell, scene);
    Pickup s2 = MakePickup(&a.gun, {a.own, a.kobra});
    Build(s2);
    const uint32_t allocator = Get<uint32_t>(g_inner + 0x34);
    Put<uint32_t>(g_inner + 0x34, Alloc(0x40));
    Gone(s2);
    Check(Live() == 0 && g_forgotten == 3 && g_releases == releasesS, "T6", "another allocator at its end: forgotten, not freed");
    Put<uint32_t>(g_inner + 0x34, allocator);
    const int leftover = LiveInstances();  // (the three forgotten, as the game would still hold them)

    // Not loaded; no scene slot; no memory.
    g_meshUnloaded = true;
    const int enters = g_enters;
    Pickup n1 = MakePickup(&a.gun, {a.own, a.kobra});
    Build(n1);
    Check(Live() == 0 && pickparts::g_notLoaded == 1 && LiveInstances() == leftover && g_enters == enters, "T6",
          "the renderer hasn't the mesh: freed at once, not live, never entered");
    g_meshUnloaded = false;
    g_sceneFull = true;
    Pickup n2 = MakePickup(&a.gun, {a.own, a.kobra});
    Build(n2);
    Check(Live() == 0 && g_noSlot == 1 && LiveInstances() == leftover, "T6", "no scene slot after the entry (+0x24 still 0xFFFF): freed, not live");
    g_sceneFull = false;
    g_allocNull = true;
    const int constructs = g_constructs;
    Pickup n3 = MakePickup(&a.gun, {a.own, a.kobra});
    Build(n3);
    Check(Live() == 0 && g_noAlloc == 1 && g_constructs == constructs, "T6", "no memory from the allocator: counted, nothing constructed");
    g_allocNull = false;
    Gone(n1), Gone(n2), Gone(n3);

    // An asset that isn't a mesh, a getter not of the game's form: no call made.
    {
        const uint32_t u = Unlock("U_X_Kobra");
        std::vector<SocketSpec> specs = {{u, "Wep_Root", 1}, {u, "Wep_Root", 1}};
        specs[0].assetNotMesh = true, specs[1].assetOddGetter = true;
        GunMade odd = MakeGun("Weapons/X/X", specs);
        Pickup o = MakePickup(&odd, {u});
        const int allocs = g_allocs, made = g_constructs;
        Build(o);
        Check(Live() == 0 && g_allocs == allocs && g_constructs == made, "T6", "its asset not a mesh, or its getter not the game's form: nothing called");
        Gone(o);
    }

    // The caps: 128 live at most (8 a pickup by the wanted list).
    {
        std::vector<SocketSpec> specs;
        std::vector<uint32_t> unlocks;
        for (int i = 0; i < 10; ++i) {
            unlocks.push_back(Unlock("U_X_Kobra"));
            specs.push_back({unlocks.back(), "Wep_Root", 1});
        }
        GunMade ten = MakeGun("Weapons/X/X", specs);
        std::vector<Pickup> many;
        for (int i = 0; i < 17; ++i) {
            many.push_back(MakePickup(&ten, unlocks));
            Build(many.back());
        }
        Check(Live() == 128 && g_refused == 8 && LiveInstances() == leftover + 128, "T6", "17 pickups of 10 sights: 8 a pickup, 128 live in all, the rest refused");
        for (const Pickup& m : many)
            Gone(m);
        Check(Live() == 0 && LiveInstances() == leftover, "T6", "all gone: all freed");
    }

    // A fault inside the constructor: counted, no more makes.
    {
        Pickup f1 = MakePickup(&a.gun, {a.own, a.kobra});
        g_constructFaults = true;
        Build(f1);
        g_constructFaults = false;
        Check(g_faults == 1 && g_noMakes && Live() == 0, "T6", "a fault inside the constructor: counted, no more makes");
        Pickup f2 = MakePickup(&a.gun, {a.own, a.kobra});
        const int made = g_constructs;
        Build(f2);
        Check(g_constructs == made && Live() == 0, "T6", "after a fault: nothing made");
        Gone(f1), Gone(f2);
    }
    // A fault after the constructor: the instance is held and freed at its end; frees still run after it.
    Scene();
    a = MakeAn94();
    Up(kPartsSights);
    {
        Pickup f3 = MakePickup(&a.gun, {a.own, a.kobra});
        Pickup f4 = MakePickup(&a.gun, {a.own, a.kobra});
        Build(f3);
        g_enterFaults = true;
        Build(f4);
        g_enterFaults = false;
        Check(g_faults == 1 && Live() == 2 && LiveInstances() == 2, "T6", "a fault after the constructor: its instance already held");
        Gone(f4);
        Gone(f3);
        Check(Live() == 0 && LiveInstances() == 0 && g_doubleFrees == 0, "T6", "both freed at their ends, though no more makes");
        Check(g_pickupsWithParts == 0, "T6", "the pickups with parts counted with the first part: back to 0, not below, after the fault's record went");
    }

    // The catch-up pass: a pickup built before the switch, one moved before it.
    Scene();
    a = MakeAn94();
    Up(kPartsCensus);
    Pickup c1 = MakePickup(&a.gun, {a.own, a.kobra}, 10.0f);
    Pickup c2 = MakePickup(&a.gun, {a.own, a.pso}, 20.0f);
    Build(c1), Build(c2);
    Check(Live() == 0 && g_constructs == 0, "T6", "parts 1: the builds counted, nothing made");
    alignas(16) const float moved[16] = {0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 5, 6, 7, 1};
    Move(c2, moved);
    Level(kPartsSights);
    Tick();
    float c1pose[16], c1want[16], c2want[16];
    PoseFor(c1.at, c1pose);
    Compose(kobraRel, c1pose, c1want);
    Compose(psoRel, moved, c2want);
    Check(Live() == 2 && g_caughtUp == 2 && g_instances.size() == 2 && Near(g_instances[0].last, c1want) && Near(g_instances[1].last, c2want), "T6",
          "the catch-up pass: parts for pickups with no record, on the pose, or on the matrix a move gave");
    // The drain at the switch's going down, on the game's thread.
    Level(kPartsOff);
    Tick();
    Check(Live() == 0 && LiveInstances() == 0 && g_drained == 2, "T6", "parts 0: drained at the next tick on the game's thread");
    Level(kPartsSights);
    Tick();
    Check(Live() == 2, "T6", "parts 3 again: caught up again");
    pickparts::g_buildThread = GetCurrentThreadId() + 1;
    Level(kPartsOff);
    Tick();
    Check(Live() == 2, "T6", "the builder on another thread: no drain from the tick");
    g_buildMode = kBuildSame;
    Build(c1);
    Move(c2, moved);
    Check(Live() == 0 && LiveInstances() == 0, "T6", "... each pickup's own next build or move lets its parts go");
    pickparts::g_buildThread = GetCurrentThreadId();
    Level(kPartsSights, 0);
    Tick();
    g_buildMode = kBuildNew;
    Pickup c3 = MakePickup(&a.gun, {a.own, a.kobra});
    Build(c3);
    Check(pickparts::g_level == 0 && Live() == 0 && LiveInstances() == 0, "T6", "the pickups level 0 with parts 3: nothing made");

    // The catch-up pass, four a second, goes on from where the last stopped: five drops that want nothing first in the list never hold up a
    // scoped one behind them.
    Scene();
    a = MakeAn94();
    Up(kPartsCensus);
    std::vector<Pickup> bare;
    for (int i = 0; i < 5; ++i) {
        bare.push_back(MakePickup(&a.gun, {a.own, a.noOptics, a.foregrip}));
        Build(bare.back());
    }
    Pickup scoped = MakePickup(&a.gun, {a.own, a.kobra}, 30.0f);
    Build(scoped);
    Level(kPartsSights);
    Tick();
    const bool notYet = Live() == 0;
    Tick();
    Check(notYet && Live() == 1 && RecordOf(scoped.at) && RecordOf(scoped.at)->count == 1, "T6",
          "the catch-up pass: a scoped drop behind five that want nothing reached at the second pass");
    for (const Pickup& b : bare)
        Gone(b);
    Gone(scoped);

    // A moved row goes when the game makes the body again (it lies on the pose then): a later catch-up places on the pose, not on the old matrix.
    Scene();
    a = MakeAn94();
    Up(kPartsCensus);
    alignas(16) const float away[16] = {0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 50, 60, 70, 1};
    Pickup r1 = MakePickup(&a.gun, {a.own, a.kobra});
    Build(r1);
    Move(r1, away);
    Build(r1);  // a new body, on the pose
    Level(kPartsSights);
    Tick();
    float r1pose[16], r1want[16];
    PoseFor(r1.at, r1pose);
    Compose(kobraRel, r1pose, r1want);
    Check(Live() == 1 && !g_instances.empty() && Near(g_instances.back().last, r1want), "T6",
          "parts 1: a body made again after a move: its moved row gone, the catch-up places on the pose");
    Gone(r1);
    // ... and while off, when builds and moves go unseen: the table is emptied once the switch went down.
    Scene();
    a = MakeAn94();
    Up(kPartsCensus);
    Pickup r2 = MakePickup(&a.gun, {a.own, a.kobra});
    Build(r2);
    Move(r2, away);
    Level(kPartsOff);
    Build(r2);  // unseen
    Level(kPartsSights);
    Tick();
    float r2pose[16], r2want[16];
    PoseFor(r2.at, r2pose);
    Compose(kobraRel, r2pose, r2want);
    Check(Live() == 1 && !g_instances.empty() && Near(g_instances.back().last, r2want), "T6",
          "a body made again while off: the moved table emptied when the switch went down, the catch-up places on the pose");
    Gone(r2);
}

void T9() {
    using namespace pickparts;
    Scene();
    An94 a = MakeAn94();
    Pickup p = MakePickup(&a.gun, {a.own, a.kobra});
    alignas(16) float buffer[20];
    const int poses = g_poses;
    Check(!PoseOf(p.at, buffer + 1) && g_misaligned == 1 && g_poses == poses, "T9", "a misaligned matrix: refused, counted, never handed to the game");
    Check(PoseOf(p.at, buffer) && g_poses == poses + 1, "T9", "an aligned one handed on");
    Check(alignof(Record) == 16 && alignof(Held) == 16 && offsetof(Record, base) % 16 == 0 && offsetof(Record, part) % 16 == 0 && sizeof(Held) % 16 == 0,
          "T9", "the record's matrices aligned (also asserted when compiled)");
}

void T10() {
    using namespace pickparts;
    Scene();
    An94 a = MakeAn94();
    Gun g;
    int wanted[8];
    const uint32_t kobra[] = {a.own, a.kobra, a.ai}, grip[] = {a.own, a.foregrip, a.noOptics, a.ai, a.mag};
    ReadGun(kMem, TheImage(), TheClasses(), a.gun.blueprint, kobra, 3, g);
    Check(Wanted(g, kPartsCensus, true, wanted, 8) == 0, "T10", "parts 1: nothing drawn");
    Check(Wanted(g, kPartsOne, true, wanted, 8) == 1 && g.part[wanted[0]].unlock == a.kobra && Wanted(g, kPartsOne, false, wanted, 8) == 0, "T10",
          "parts 2: the first sight, only while no other pickup has a part");
    Check(Wanted(g, kPartsSights, true, wanted, 8) == 1 && g.part[wanted[0]].unlock == a.kobra, "T10",
          "parts 3: the Kobra on Wep_Root alone (its socket on Wep_Grip not, the socket with no unlock not)");
    Check(Wanted(g, kPartsAll, true, wanted, 8) == 2 && g.part[wanted[0]].unlock == a.kobra && g.part[wanted[1]].unlock == 0, "T10",
          "parts 4: the sight first, then the socket with no unlock");
    ReadGun(kMem, TheImage(), TheClasses(), a.gun.blueprint, grip, 5, g);
    Check(Wanted(g, kPartsSights, true, wanted, 8) == 0, "T10", "a foregrip and NoOptics at parts 3: none");
    const int n = Wanted(g, kPartsAll, true, wanted, 8);
    bool mag = false, foregrip = false;
    for (int i = 0; i < n; ++i)
        mag = mag || g.part[wanted[i]].unlock == a.mag, foregrip = foregrip || g.part[wanted[i]].unlock == a.foregrip;
    Check(n == 2 && foregrip && !mag, "T10", "parts 4: the foregrip and the bare socket, never the extended magazine");
    const uint32_t kind1 = Unlock("U_AN94_Kobra", 1);
    GunMade k1 = MakeGun("Weapons/X/X", {{kind1, "Wep_Root", 1}});
    const uint32_t s1[] = {kind1};
    ReadGun(kMem, TheImage(), TheClasses(), k1.blueprint, s1, 1, g);
    Check(Wanted(g, kPartsAll, true, wanted, 8) == 0 && std::strcmp(NotDrawn(g.part[0], kPartsAll), "its unlock's kind isn't 0") == 0, "T10",
          "a sight of kind 1: not drawn");

    // In the game's builder: parts 2 with two scoped drops, one part on the first alone.
    Up(kPartsOne);
    Pickup p1 = MakePickup(&a.gun, {a.own, a.kobra}), p2 = MakePickup(&a.gun, {a.own, a.pso});
    Build(p1), Build(p2);
    Check(Live() == 1 && RecordOf(p1.at) && RecordOf(p1.at)->count == 1 && !RecordOf(p2.at), "T10", "parts 2: one sight on the first scoped drop alone");
    Gone(p1);
    Build(p2);
    Check(Live() == 1 && RecordOf(p2.at), "T10", "the first gone: the next scoped drop's build gets it");
    Gone(p2);

    // The census (parts 1): said once per pickup and content, nothing made.
    Scene();
    a = MakeAn94();
    Pickup c = MakePickup(&a.gun, {a.own, a.kobra, a.ai});
    MakePickup(nullptr, {}, 3.0f, false);
    Mark();
    Up(kPartsCensus);
    Level(kPartsCensus);
    Check(Said("Pickups: parts 1 (pickups.txt): each drop's unlocks and parts named in the log, nothing drawn") == 1, "T10", "parts 1 said");
    Check(Said("Pickups: parts: the AN94 at (10.0 1.0 2.0), entry 0 (the preferred slot): 3 unlocks") == 1 &&
              Said("Weapons/AN94/U_AN94_Kobra (kind 0, sight) -> socket 1 (Wep_Root, object 0") == 1 && Said("from the state's list: drawable") == 1 &&
              Said("socket 5 (Wep_Grip") == 1 && Said("its socket is on another bone") == 1 &&
              Said("Weapons/Common/AI_Unlock (kind 0, no picture) -> no socket") == 1 && Said("1 socket objects with no unlock (1 with a 3P asset)") == 1,
          "T10", "the census: the drop's unlocks, each one's sockets and 3P assets, where from, what would be drawn; said once");
    Check(Said("has no UseWeaponMesh: its own mesh (none), placed raw") == 1, "T10", "a pickup without UseWeaponMesh: its path said once");
    Check(g_constructs == 0 && Live() == 0, "T10", "the census makes nothing");
    SetUnlocks(c.entries, {a.own, a.pso, a.ai});
    Level(kPartsCensus);
    Check(Said("U_AN94_PSO-1 (kind 0, sight)") == 1, "T10", "its content changed: said again");
    Check(g_misalignedSeen == 0, "T10", "every matrix the game's functions were handed in all the rows was 16-byte aligned");
    {
        bool fresh = true;
        for (uint32_t i = 0; i < 40; ++i)
            fresh = !SaidOwnMesh(0x10000 + 4 * i) && fresh;
        const bool once = !SaidOwnMesh(0x20000) && !SaidOwnMesh(0x20004);
        Check(fresh && once && SaidOwnMesh(0x20000) && SaidOwnMesh(0x20004), "T10",
              "the pickups without UseWeaponMesh already said, a ring: with it full, two new ones are each said once, not every second");
    }
}

void T12() {
    using namespace pickparts;
    Scene();
    // Slot 47's test (00844560): none only while +0x165 is set and +0x164 isn't.
    Check(BodyShown(0, 0) && BodyShown(1, 0) && !BodyShown(0, 1) && BodyShown(1, 1), "T12", "slot 47's test: hidden only with +0x165 set and +0x164 clear");
    Check(KindDrawn(0) && !KindDrawn(1) && KindDrawn(2) && !KindDrawn(3) && !KindDrawn(0xFFFFFFFF), "T12",
          "the kinds drawn from a stand-in: 0 and 2; 1 (skinned: the pickup's own bones) and any other never");
    // A stand-in: the vtable, the matrix's address, the instance, the handle; every other byte zero.
    {
        uint8_t block[kStandinSize + 8];
        std::memset(block, 0xCC, sizeof(block));
        FillStandin(block, 0x020B8DF0, 0x12345670, 0x0ABCDEF0, 0x0405);
        bool rest = true;
        for (uint32_t i = 0; i < kStandinSize; ++i)
            if (i >= 4 && (i < 0x1C || i >= 0x20) && (i < 0x110 || i >= 0x114) && (i < 0x166 || i >= 0x168))
                rest = rest && block[i] == 0;
        uint32_t vt = 0, m = 0, inst = 0, flags = 1;
        uint16_t h = 0;
        std::memcpy(&vt, block, 4), std::memcpy(&m, block + 0x1C, 4), std::memcpy(&inst, block + 0x110, 4), std::memcpy(&flags, block + 8, 4);
        std::memcpy(&h, block + 0x166, 2);
        Check(rest && vt == 0x020B8DF0 && m == 0x12345670 && inst == 0x0ABCDEF0 && h == 0x0405 && !flags && !block[0x164] && !block[0x165] &&
                  block[kStandinSize] == 0xCC,
              "T12", "a stand-in: the vtable at +0, the matrix at +0x1C, the instance at +0x110, the handle at +0x166, +8 and +0x164/+0x165 0, every "
                     "other byte 0, nothing past 0x180");
    }
    // The light's handle (01744260).
    Check(LightFor(kMem, g_lights, kGunLight) == kGunLight, "T12", "the light: the gun's own handle while its block is there");
    Check(LightFor(kMem, g_lights, kLightNoBlock) == kNoLight, "T12", "the light: none when the system has no block for the handle");
    Check(LightFor(kMem, g_lights, kNoLight) == kNoLight && LightFor(kMem, 0, kGunLight) == kNoLight, "T12", "the light: none for none, none with no system");
    Put<uint8_t>(g_lights + 0x144, 0);
    Check(LightFor(kMem, g_lights, kLightNoBlock) == kLightNoBlock, "T12", "the light: the system off reads no block, so any handle is handed as it is");
    Put<uint8_t>(g_lights + 0x144, 1);
    // The entry the pose is taken from (as 00854960 guards it).
    {
        An94 a = MakeAn94();
        Pickup p = MakePickup(&a.gun, {a.own, a.kobra});
        Check(PoseEntry(kMem, p.at) == p.entries, "T12", "the pose's entry: the one the game draws");
        Put<uint32_t>(a.gun.states - 4, 0);
        Check(PoseEntry(kMem, p.at) == 0, "T12", "the pose's entry: none with a WeaponStates count of 0");
        Put<uint32_t>(a.gun.states - 4, 1);
        Put<uint32_t>(p.entries, 0);
        Check(PoseEntry(kMem, p.at) == 0, "T12", "the pose's entry: none without a blueprint");
        Put<uint32_t>(p.entries, a.gun.blueprint);
        Put<uint32_t>(p.at + 0xAC, 0), Put<uint32_t>(p.at + 0xB0, 0);
        Check(PoseEntry(kMem, p.at) == 0, "T12", "the pose's entry: none with no entries (0084CC50 would hand 0084A7C0 a null one)");
    }
    // A row written and read; one being written isn't read.
    {
        static SharedRow s;
        Row in;
        in.pickup = 0x1000, in.body = 0x2000, in.world = 0x3000, in.scene = 0x4000, in.count = 2;
        in.instance[0] = 0x5000, in.instance[1] = 0x6000, in.kind[1] = 2;
        in.rel[1][13] = 0.079f;
        WriteRow(s, in);
        Row out;
        uint32_t seq = 1;
        Check(ReadRow(s, out, seq) && seq == 2 && std::memcmp(&in, &out, sizeof(Row)) == 0 && RowPickup(s) == 0x1000, "T12",
              "a row written, then read: the same words, its seq even");
        s.seq.store(3);
        Check(!ReadRow(s, out, seq), "T12", "a row being written (its seq odd): no copy");
        s.seq.store(4);
        // Held for a call; the row written since the copy: the holds dropped, nothing handed.
        CallHolds call;
        Check(HoldRow(call, 0, in.instance, 2, s, 4) && AnyHolds(&call, 1, 0x5000) && AnyHolds(&call, 1, 0x6000), "T12",
              "a row's instances held while its seq is the copy's");
        ReleaseCall(call);
        Check(!AnyHolds(&call, 1, 0x5000) && !call.busy, "T12", "a call let go of: nothing held");
        WriteRow(s, in);
        Check(!HoldRow(call, 2, in.instance, 2, s, 4) && !AnyHolds(&call, 1, 0x5000) && !AnyHolds(&call, 1, 0x6000), "T12",
              "a row written after the copy: its instances not held, none handed");
    }
    {
        CallHolds calls[kCallSlots];
        int got = 0;
        for (int i = 0; i < kCallSlots; ++i)
            got += ClaimCall(calls, kCallSlots) == i;
        const bool none = ClaimCall(calls, kCallSlots) == -1;
        ReleaseCall(calls[3]);
        Check(got == kCallSlots && none && ClaimCall(calls, kCallSlots) == 3, "T12", "the call slots: 8 at once, the 9th none, one let go of taken again");
    }
    // A row read whole or not at all, a writer at it on another thread all along.
    {
        static SharedRow s;
        std::atomic<bool> stop{false};
        std::thread writer([&] {
            Row r;
            for (uint32_t g = 1; !stop.load(); ++g) {
                r.pickup = g, r.body = g, r.world = g, r.scene = g, r.count = g;
                for (int k = 0; k < kRowParts; ++k) {
                    r.instance[k] = g, r.kind[k] = g;
                    for (float& f : r.rel[k])
                        f = static_cast<float>(g & 0xFFFF);
                }
                WriteRow(s, r);
            }
        });
        int copies = 0, torn = 0;
        const ULONGLONG until = GetTickCount64() + 300;
        while (GetTickCount64() < until) {
            Row r;
            uint32_t seq = 0;
            if (!ReadRow(s, r, seq))
                continue;
            ++copies;
            bool same = r.body == r.pickup && r.world == r.pickup && r.scene == r.pickup && r.count == r.pickup;
            for (int k = 0; k < kRowParts; ++k) {
                same = same && r.instance[k] == r.pickup && r.kind[k] == r.pickup;
                for (float f : r.rel[k])
                    same = same && f == static_cast<float>(r.pickup & 0xFFFF);
            }
            torn += !same;
        }
        stop = true;
        writer.join();
        char what[160];
        snprintf(what, sizeof(what), "a row written over and over on another thread: %d copies read, none torn (%d)", copies, torn);
        Check(copies > 100 && torn == 0, "T12", what);
    }
}

// The pose the fake 0084A7C0 gives a pickup now, times the part's matrix.
void Expect(uint32_t pickup, const float* rel, float* out) {
    float pose[16];
    PoseFor(pickup, pose);
    pickparts::Compose(rel, pose, out);
}

void T13() {
    using namespace pickparts;
    const float kobraRel[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0.08f, 0.30f, 1};
    const float psoRel[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0.079f, 0.416f, 1};
    // Off: nothing hooked, the collector never seen.
    Scene();
    Level(kPartsOff);
    Check(!g_hooked && g_enabled.empty() && !g_collectOriginal, "T13", "no parts line: nothing hooked, the collector not either (the DLL as before)");
    // Parts 1: hooked, passing through, counted.
    Scene();
    An94 a = MakeAn94();
    Pickup p = MakePickup(&a.gun, {a.own, a.kobra, a.ai});
    Mark();
    Up(kPartsCensus);
    Check(g_hooked && Said("its destructor and the pickup renderer's collector (00797FF0)") == 1, "T13", "parts 1: the collector hooked with the rest");
    Build(p);
    Collect({p.at});
    Check(g_collected.size() == 1 && g_collected[0].size() == 1 && g_collected[0][0].entity == p.at && g_calls == 1 && g_selfSeen == kRenderer &&
              g_unusedSeen == kUnused && g_recordsSeen == kRecords,
          "T13", "parts 1: the game's own pass alone, its renderer, word and records as handed");
    // Parts 3: the Kobra made in the builder; the next call hands its stand-in after the game's own pass.
    Level(kPartsSights);
    Build(p);
    Check(Live() == 1 && g_rowsLive == 1, "T13", "parts 3: the Kobra made, its row written");
    const int posesOf = g_posesOf;
    Collect({p.at});
    float want[16];
    Expect(p.at, kobraRel, want);
    const std::vector<Seen>& h = Handed();
    Check(g_collected.size() == 2 && g_collected[0].size() == 1 && g_collected[0][0].entity == p.at && h.size() == 1, "T13",
          "the game's own pass first, then the stand-in alone");
    if (h.size() == 1) {
        const Seen& s = h[0];
        Check(s.vtable == kClientPickupVt && s.flags == 0 && s.instance == g_instances.back().at && s.live && s.shown == 0 && s.hiding == 0 &&
                  s.handle == kGunLight && s.zeroElsewhere,
              "T13", "the stand-in: the client pickup's vtable, +8 0, the Kobra's live instance at +0x110, +0x164/+0x165 0, the gun's light, the rest 0");
        Check(s.views == kViews && s.more[0] == 0x11 && s.more[1] == 0x22, "T13", "its element: the gun's views' mask and the rest of the gun's");
        Check((s.matrixAt & 15) == 0 && Near(s.m, want) && g_posesOf == posesOf + 1 && g_poseOfNull == 0 && g_poseOfMisaligned == 0, "T13",
              "its matrix: 16-byte aligned, the Kobra's on the pose taken in that same call (0084A7C0, its entry not null, aligned)");
        Check(g_selfSeen == kRenderer && g_unusedSeen == kUnused && g_recordsSeen == kRecords && g_callsDrawn == 1 && g_standins == 1, "T13",
              "the second pass with the same renderer, word and records (the same frame's lists); counted");
    }
    // The gun moves (the drop falls, rolls, settles): the next call's matrix is where it is then.
    Put<float>(p.transform + 48, 25.0f), Put<float>(p.transform + 52, 0.5f);
    Collect({p.at});
    Expect(p.at, kobraRel, want);
    Check(Handed().size() == 1 && Near(Handed()[0].m, want) && want[12] != 0.0f, "T13", "the gun moved without setTransform: the part with it, the next call");
    // Hidden in its fly-in; shown again.
    Put<uint8_t>(p.at + 0x165, 1);
    Collect({p.at});
    const bool hidden = g_collected.size() == 1;
    Put<uint8_t>(p.at + 0x164, 1);
    Collect({p.at});
    const bool both = Handed().size() == 1;
    Put<uint8_t>(p.at + 0x164, 0), Put<uint8_t>(p.at + 0x165, 0);
    Check(hidden && both, "T13", "the gun hidden (+0x165 set, +0x164 clear): its parts too; both set: shown, so are they");
    // Its body made again behind the row's back: nothing until the game's thread writes the row again.
    {
        const uint32_t body = Get<uint32_t>(p.at + 0x110);
        Put<uint32_t>(p.at + 0x110, BodyInstance("weapons/an94/an94_3p_Mesh"));
        Collect({p.at});
        const bool none = g_collected.size() == 1;
        Put<uint32_t>(p.at + 0x110, body);
        Collect({p.at});
        Check(none && Handed().size() == 1, "T13", "another body than the row's: nothing handed; its own again: handed");
    }
    // Another world, another scene.
    {
        const uint32_t world = Get<uint32_t>(g_clientWorldCell), scene = Get<uint32_t>(g_sceneCell);
        Put<uint32_t>(g_clientWorldCell, Alloc(0x900));
        Collect({p.at});
        const bool w = g_collected.size() == 1;
        Put<uint32_t>(g_clientWorldCell, world);
        Put<uint32_t>(g_sceneCell, Alloc(0x40));
        Collect({p.at});
        const bool sc = g_collected.size() == 1;
        Put<uint32_t>(g_sceneCell, scene);
        Check(w && sc, "T13", "another client world or another scene than the parts were made in: nothing handed");
    }
    // No entry, a state count of 0: no pose, nothing handed (0084A7C0 never handed a null entry).
    {
        Put<uint32_t>(p.at + 0xAC, 0), Put<uint32_t>(p.at + 0xB0, 0);
        const int before = g_posesOf;
        Collect({p.at});
        const bool none = g_collected.size() == 1 && g_posesOf == before;
        Put<uint32_t>(p.at + 0xAC, p.entries), Put<uint32_t>(p.at + 0xB0, p.entries + 0xC0);
        Put<uint32_t>(a.gun.states - 4, 0);
        Collect({p.at});
        const bool none2 = g_collected.size() == 1 && g_posesOf == before;
        Put<uint32_t>(a.gun.states - 4, 1);
        Check(none && none2 && g_poseOfNull == 0, "T13", "no entry, or a WeaponStates count of 0: no pose asked, nothing handed");
    }
    // The light: none for a handle without a block; no system: nothing handed at all.
    {
        Put<uint16_t>(p.at + 0x166, kLightNoBlock);
        Collect({p.at});
        const bool unlit = Handed().size() == 1 && Handed()[0].handle == kNoLight;
        Put<uint16_t>(p.at + 0x166, kGunLight);
        Put<uint32_t>(g_lightsCell, 0);
        Collect({p.at});
        const bool none = g_collected.size() == 1;
        Put<uint32_t>(g_lightsCell, g_lights);
        Check(unlit && none, "T13", "a handle with no block: the stand-in's 0xFFFF; no light system: no stand-in at all (01744260 on none)");
    }
    // An element that isn't a client pickup, and one with no row: skipped.
    {
        const uint32_t other = Alloc(0x180);
        Put<uint32_t>(other, 0x72000099);
        Put<uint32_t>(other + 0x110, g_instances.back().at);
        Pickup bare = MakePickup(&a.gun, {a.own, a.noOptics});
        Build(bare);
        Collect({other, bare.at, p.at});
        Check(g_collected.size() == 2 && g_collected[0].size() == 3 && Handed().size() == 1 && Handed()[0].instance == g_instances.back().at, "T13",
              "another entity and a bare drop in the call: only the scoped drop's part handed");
        Gone(bare);
    }
    // Census lines (since parts 1): the made part, the collector's thread, its counts.
    Level(kPartsSights);
    Check(Said("Pickups: parts: made weapons/accessories/pks-07/pks-07_3p_mesh on the pickup") == 1 && Said("its MeshSet's kind 0") == 1 &&
              Said("the pickup renderer's collector runs on thread") == 1 && Said("Pickups: parts: the collector:") >= 1,
          "T13", "the census: the part made once (its pickup, place, instance, scene slot, kind), the collector's thread once, its counts");
    // Below parts 2: nothing handed, though the row is there (until the game's thread drains it).
    Level(kPartsCensus);
    Collect({p.at});
    const bool census = g_collected.size() == 1;
    Level(kPartsSights);
    Collect({p.at});
    Check(census && Handed().size() == 1, "T13", "parts 1: nothing handed; parts 3 again: handed");
    // Gone: the row written empty before the free; the next call hands nothing.
    {
        const uint32_t inst = g_instances.back().at;
        Gone(p);
        Collect({p.at});
        Check(g_collected.size() == 1 && g_rowsLive == 0 && !InstanceAt(inst) && g_heldWaits == 0, "T13", "gone: the row empty, the part freed, nothing handed");
    }

    // The cap: one drop of two sights more than a call holds: all but the last handed whole (the cap's stand-ins), the last not at all.
    Scene();
    a = MakeAn94();
    Up(kPartsSights);
    {
        const int drops = kDrawEach / 2 + 1;
        std::vector<uint32_t> all;
        for (int i = 0; i < drops; ++i) {
            Pickup q = MakePickup(&a.gun, {a.own, a.kobra, a.pso}, 10.0f + i);
            Build(q);
            all.push_back(q.at);
        }
        Collect(all);
        int whole = 0;
        for (uint32_t at : all) {
            int n = 0;
            for (const Seen& s : Handed()) {
                float m[16];
                const bool kobra = (Expect(at, kobraRel, m), Near(s.m, m)), pso = (Expect(at, psoRel, m), Near(s.m, m));
                n += kobra || pso;
            }
            whole += n == 2;
        }
        char what[200];
        snprintf(what, sizeof(what), "%d drops of two parts in one call: %d stand-ins (the cap), %d drops whole, the last none (counted)", drops, kDrawEach,
                 drops - 1);
        Check(Live() == 2 * drops && Handed().size() == static_cast<size_t>(kDrawEach) && whole == drops - 1 && g_overCap == 1, "T13", what);
    }
    // No call slot free: nothing handed, counted.
    {
        Pickup q = MakePickup(&a.gun, {a.own, a.kobra});
        Build(q);
        for (CallHolds& c : g_holds)
            c.busy = 1;
        Collect({q.at});
        const bool none = g_collected.size() == 1 && g_noCall == 1;
        for (CallHolds& c : g_holds)
            c.busy = 0;
        Collect({q.at});
        Check(none && Handed().size() == 1, "T13", "every call slot taken: nothing handed (counted); one free: handed");
    }
    // A row being written: not read, counted.
    {
        Pickup q = MakePickup(&a.gun, {a.own, a.pso}, 40.0f);
        Build(q);
        int at = -1;
        for (int i = 0; i < kMostRecords; ++i)
            at = RowPickup(g_rows[i]) == q.at ? i : at;
        const uint32_t seq = at >= 0 ? g_rows[at].seq.load() : 0;
        if (at >= 0)
            g_rows[at].seq.store(seq + 1);
        Collect({q.at});
        const bool none = g_collected.size() == 1 && g_rowBusy == 1;
        if (at >= 0)
            g_rows[at].seq.store(seq + 2);
        Collect({q.at});
        Check(at >= 0 && none && Handed().size() == 1, "T13", "a row being written: not read (counted); written: handed");
    }
    // A part not as made (the renderer's mesh gone from it): nothing of that drop handed, counted.
    {
        Pickup q = MakePickup(&a.gun, {a.own, a.kobra}, 50.0f);
        Build(q);
        const uint32_t inst = g_instances.back().at;
        Put<uint32_t>(inst + 8, 0);
        Collect({q.at});
        const bool none = g_collected.size() == 1 && g_notAsMade == 1;
        Put<uint32_t>(inst + 8, 1);
        Check(none, "T13", "a part whose instance isn't as made: that drop's not handed (counted)");
    }
    // A fault in the second pass: caught, counted, nothing more handed this session; the game's own passes go on.
    {
        Pickup q = MakePickup(&a.gun, {a.own, a.kobra}, 60.0f);
        Build(q);
        g_collectFaults = true;
        Collect({q.at});
        g_collectFaults = false;
        const bool caught = g_drawFaults == 1 && !g_drawOn;
        Collect({q.at});
        bool released = true;
        for (const CallHolds& c : g_holds) {
            released = released && !c.busy;
            for (const auto& i : c.instance)
                released = released && !i;
        }
        Check(caught && g_collected.size() == 1 && released, "T13", "a fault on the stand-ins: caught, counted, none handed after it, nothing left held");
        Mark();
        g_countsAt = 0;
        Level(kPartsSights);
        Check(Said("the collector's stand-ins faulted") == 1, "T13", "the fault said");
    }

    // The MeshSet's kinds: 1 freed at the make (counted), never in a row; 2 handed; 3 freed.
    Scene();
    a = MakeAn94();
    Up(kPartsSights);
    {
        g_setKind = 1;
        Pickup k1 = MakePickup(&a.gun, {a.own, a.kobra});
        Build(k1);
        const bool one = Live() == 0 && g_refusedKind == 1 && g_rowsLive == 0 && LiveInstances() == 0;
        g_setKind = 3;
        Pickup k3 = MakePickup(&a.gun, {a.own, a.kobra});
        Build(k3);
        const bool three = Live() == 0 && g_refusedKind == 2 && LiveInstances() == 0;
        g_setKind = 2;
        Pickup k2 = MakePickup(&a.gun, {a.own, a.kobra});
        Build(k2);
        Collect({k1.at, k2.at, k3.at});
        g_setKind = 0;
        Check(one && three && Live() == 1 && Handed().size() == 1 && Handed()[0].live, "T13",
              "its MeshSet's kind 1 (skinned) or 3: freed at the make, never handed; kind 2: handed");
    }

    // A free while a call holds the part: the game's thread waits for the call to let go of it, then frees; a hold past 5 ms: forgotten, not freed.
    Scene();
    a = MakeAn94();
    Up(kPartsSights);
    {
        Pickup q = MakePickup(&a.gun, {a.own, a.kobra});
        Build(q);
        const uint32_t inst = g_instances.back().at;
        const int c = ClaimCall(g_holds, kCallSlots);
        g_holds[c].instance[0] = inst;
        const int releases = g_releases;
        const ULONGLONG t0 = GetTickCount64();
        Gone(q);
        const ULONGLONG took = GetTickCount64() - t0;
        Check(g_heldWaits == 1 && g_heldForgotten == 1 && g_forgotten == 1 && g_releases == releases && InstanceAt(inst) && took < 200, "T13",
              "a part held by a call past 5 ms at its free: forgotten, never freed under the call");
        ReleaseCall(g_holds[c]);
        Pickup r = MakePickup(&a.gun, {a.own, a.kobra});
        Build(r);
        const uint32_t inst2 = g_instances.back().at;
        const int c2 = ClaimCall(g_holds, kCallSlots);
        g_holds[c2].instance[3] = inst2;
        g_heldWaitMs = 2000;  // (this test's thread lets go once the free is seen waiting, however slow it's scheduled)
        std::thread job([&] {
            for (int i = 0; i < 2000 && g_heldWaits.load() < 2; ++i)
                Sleep(1);
            ReleaseCall(g_holds[c2]);
        });
        Gone(r);
        job.join();
        g_heldWaitMs = 5;
        Check(g_heldWaits == 2 && g_heldForgotten == 1 && g_releases == releases + 1 && !InstanceAt(inst2), "T13",
              "a part held by a call that lets go of it at once: the free waits, then frees");
    }
}

// The game's thread making and freeing parts, a render job calling the collector all the while.
void T14() {
    using namespace pickparts;
    Scene();
    An94 a = MakeAn94();
    Pickup p = MakePickup(&a.gun, {a.own, a.kobra});
    Up(kPartsSights);
    Build(p);
    g_stress = true;
    g_heldWaitMs = 2000;  // (a free waits for the job's call however slow it's scheduled: none forgotten for being slow)
    std::atomic<bool> stop{false};
    std::atomic<int> calls{0};
    std::thread job([&] {
        while (!stop.load()) {
            Collect({p.at});
            ++calls;
        }
    });
    for (int i = 0; i < 3000; ++i) {
        SetUnlocks(p.entries, {a.own, (i & 1) ? a.pso : a.kobra});  // another sight: the old part freed, a new one made, each build
        Build(p);
        if (i % 50 == 49)
            Gone(p);
        // The job gets at least one call in before the next build (its next call then runs into that build: they still overlap), however busy
        // the machine is.
        const int seen = calls.load();
        for (int spin = 0; spin < 2050 && calls.load() == seen; ++spin)
            Sleep(spin < 2000 ? 0 : 1);
    }
    stop = true;
    job.join();
    g_stress = false;
    g_heldWaitMs = 5;
    char what[240];
    snprintf(what, sizeof(what),
             "3000 builds (%d parts freed) beside %d calls of the collector: %d stand-ins handed, none with a freed instance (%d), none odd (%d); %u frees "
             "waited for a call, none forgotten (%u)",
             g_releases, calls.load(), g_stressSeen, g_stressBad, g_stressOdd, g_heldWaits.load(), g_heldForgotten.load());
    Check(g_stressBad == 0 && g_stressOdd == 0 && g_stressSeen > 100 && g_releases > 1000 && g_heldForgotten == 0 && g_drawFaults == 0, "T14", what);
    Gone(p);
    Check(Live() == 0 && g_rowsLive == 0, "T14", "all gone: nothing live, no row");
}

void T11() {
    wchar_t self[MAX_PATH];
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    std::wstring image = self;
    image = image.substr(0, image.find_last_of(L'\\') + 1) + L"bf3_mem.exe";
    FILE* f = nullptr;
    _wfopen_s(&f, image.c_str(), L"rb");
    if (!f) {
        std::printf("SKIP T11 the real tables against bf3_mem.exe: not beside the test\n");
        return;
    }
    fseek(f, 0, SEEK_END);
    const long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> img(static_cast<size_t>(size));
    fread(img.data(), 1, img.size(), f);
    fclose(f);
    auto at = [&](uintptr_t va, size_t n) -> const uint8_t* {
        return va >= 0x400000 && va - 0x400000 + n <= img.size() ? img.data() + (va - 0x400000) : nullptr;
    };
    int codes = 0, bytes = 0, words = 0;
    for (const pickparts::Code& c : pickparts::kCode)
        if (const uint8_t* p = at(c.at, c.size); p && pickparts::Fnv(p, c.size) == c.fnv)
            ++codes;
        else
            std::printf("      not as read in bf3_mem.exe: %s\n", c.what);
    for (const pickparts::Bytes& b : pickparts::kBytes)
        if (const uint8_t* p = at(b.at, b.size); p && std::memcmp(p, b.bytes, b.size) == 0)
            ++bytes;
        else
            std::printf("      not as read in bf3_mem.exe: %s\n", b.what);
    for (const pickparts::Word& w : pickparts::kWords) {
        uint32_t v = 0;
        if (const uint8_t* p = at(w.at, w.size)) {
            std::memcpy(&v, p, w.size);
            if (v == w.value) {
                ++words;
                continue;
            }
        }
        std::printf("      not as read in bf3_mem.exe: %s\n", w.what);
    }
    char what[200];
    snprintf(what, sizeof(what), "the DLL's own tables against build\\bf3_mem.exe: %d of %d functions, %d of %d byte rows, %d of %d words as read", codes,
             static_cast<int>(std::size(pickparts::kCode)), bytes, static_cast<int>(std::size(pickparts::kBytes)), words,
             static_cast<int>(std::size(pickparts::kWords)));
    Check(codes == static_cast<int>(std::size(pickparts::kCode)) && bytes == static_cast<int>(std::size(pickparts::kBytes)) &&
              words == static_cast<int>(std::size(pickparts::kWords)),
          "T11", what);
    // The getters the class test reads are of that form in the image: WeaponRegularSocketObjectData's and the mesh assets' (their types at 023F5084
    // and 023D1AF0, whose class ids the ranges at 023F5098 and 023D1B04 start with), in the ctr section, each the first slot of a vtable in the
    // read-only data.
    const uint8_t objectGetter[] = {0xB8, 0x84, 0x50, 0x3F, 0x02, 0xC3}, meshGetter[] = {0xB8, 0xF0, 0x1A, 0x3D, 0x02, 0xC3};
    const uint8_t* og = at(0x01FAD150, 6);
    const uint8_t* mg = at(0x01F21440, 6);
    const uint8_t* ov = at(0x021B60FC, 4);
    const uint8_t* mv = at(0x02092B1C, 4);
    uint32_t ovf = 0, mvf = 0;
    if (ov)
        std::memcpy(&ovf, ov, 4);
    if (mv)
        std::memcpy(&mvf, mv, 4);

    Check(og && mg && std::memcmp(og, objectGetter, 6) == 0 && std::memcmp(mg, meshGetter, 6) == 0 && ovf == 0x01FAD150 && mvf == 0x01F21440, "T11",
          "the type getters of the game's form, each named in the read-only data");
    // ... and the section they're in is code: what ImageRanges takes from the headers as the second code range.
    uint32_t peAt = 0, ctrFrom = 0, ctrTo = 0, ctrFlags = 0;
    uint16_t sections = 0, optional = 0;
    if (const uint8_t* h = at(0x40003C, 4))
        std::memcpy(&peAt, h, 4);
    if (const uint8_t* h = at(0x400000 + peAt + 6, 2))
        std::memcpy(&sections, h, 2);
    if (const uint8_t* h = at(0x400000 + peAt + 20, 2))
        std::memcpy(&optional, h, 2);
    for (int i = 0; i < sections && i < 64; ++i)
        if (const uint8_t* s = at(0x400000 + peAt + 24 + optional + 40 * i, 40); s && std::memcmp(s, "ctr\0\0\0\0\0", 8) == 0) {
            uint32_t size = 0, va = 0;
            std::memcpy(&size, s + 8, 4), std::memcpy(&va, s + 12, 4), std::memcpy(&ctrFlags, s + 36, 4);
            ctrFrom = 0x400000 + va, ctrTo = ctrFrom + size;
        }
    Check(ctrFrom <= 0x01F21440 && 0x01FAD150 < ctrTo && (ctrFlags & 0x20000000), "T11", "the getters' section, ctr, is executable code in the headers");
}

}  // namespace

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);  // (a crash keeps every line before it)
    g_arena = static_cast<uint8_t*>(VirtualAlloc(nullptr, kArena, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!g_arena) {
        std::printf("no memory for the test\n");
        return 1;
    }
    void (*rows[])() = {T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14};
    for (auto row : rows)
        row();
    std::printf("%d of %d checks failed\n", g_failures, g_counted);
    return g_failures;
}

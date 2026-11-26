#include "ground.h"

#include <windows.h>
#include <MinHook.h>
#include <intrin.h>
#include <malloc.h>  // _resetstkoflw

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <mutex>

#include "camera.h"
#include "log.h"
#include "stood.h"

#pragma intrinsic(_ReturnAddress)

namespace {

using ground::Hit;
using ground::Source;
using ground::What;

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// The game's, read in bf3_mem.exe (bf3.exe 1.6, not relocated; the floor study and its check, 2026-11-05).

// The client's world, one for each level (0x860 bytes): its pointer is set by its constructor alone (00F93C58); the seven other stores to it all write
// 0, each in one of this world's own destructors once it has put its vtables back (00F897A0, 00F92B93, 00F942B9, 00F942FC, 00FAB71E, 00FABCC0,
// 00FAC062): a pointer that isn't 0 is a whole world. Its vtable, the ray caster it has at +0x840 (whose first slot, 0119A0B0, is its own copy of the
// cast by an interface: it walks the lists itself and never calls the cast), and at +0x844 the lists of what's where that a ray is tested against. The
// client level's update reads its world at +0xC0 (00F9848D).
constexpr uintptr_t kWorldAt = 0x023813A0;
constexpr uint32_t kWorldVtable = 0x0213C578, kCasterVtable = 0x0213C570, kCasterFirst = 0x0119A0B0;
constexpr uint32_t kCasterAt = 0x840, kListsAt = 0x844, kLevelWorldAt = 0xC0;

// The cast: thiscall on the world, ret 1Ch: (a name it never reads, plain, from, to, the hit, the flags, things to leave out or null); al 1 if it hit.
// plain 1 is the plain way (0 adds the client soldier's hit boxes and forces the detail meshes, 011CCC80, 011CCC94). from and to are read with movaps
// (01779639, 01199AFD): 16-byte aligned, w 0.
constexpr uintptr_t kCastAt = 0x01199980;
// The lists' flush: thiscall on them, one byte, ret 4: what's been queued for them put in, under their own lock (+0x20). The client level's update
// (00F98470, ret 14h) has it done every frame inside its physics step (01882690, called from 00F985A2 alone), once the step has waited for the last
// step's jobs (018826AD-01882745) and before it starts the new step's (01882C18-01882DD5): it comes back to 018827AD. With no physics the update flushes
// them itself: back to 00F985CC. Its other callers (a writer once 1023 changes are queued, 0177EF90, and the rest) come back elsewhere. At its very end it
// starts the lists' own query jobs again (0177EE9B, 017740C0: queued on the job manager [022EAD98] at 01774160-01774166) and stops them only at the start
// of the next flush (0177ECA7, 017741E0: the stop at 017741F7, the wait at 01774201-0177420B): the casts here run beside those, as the game's own casts on
// this thread do (readers, by all that's been read of them). With the physics paused ([esi+240h] 0, 01882753) the step returns at 0188276E without a
// flush: nothing's cast then (groundat 0).
constexpr uintptr_t kFlushAt = 0x0177EC70, kStepFlushBack = 0x018827AD, kLevelFlushBack = 0x00F985CC;
constexpr uintptr_t kLevelUpdateAt = 0x00F98470;

// Every piece of the game's code it goes through, as bf3_mem.exe has it: the FNV-1a 64 of its bytes.
struct Code {
    uintptr_t at;
    uint32_t size;
    uint64_t fnv;
    const char* what;
};
constexpr Code kCode[] = {
    {0x01199980, 909, 0x1BDFFAAFDCC36DC2ull, "the ray cast"},
    {0x0177EC70, 629, 0x73EC5A2BD5D211B1ull, "the lists' flush"},
    {0x01882690, 285, 0x0E86617AFF6CC794ull, "the physics step, up to its flush"},
    {0x00F98470, 366, 0x56E985DAD36AF6F9ull, "the client level's update"},
    {0x00F93C10, 84, 0xAB1FAD757A779B4Eull, "the client world's constructor"},
};
// Byte for byte as well: where it hooks (MinHook's jump goes over the first 6 bytes of each), and the two calls of the flush it casts after.
constexpr uint8_t kCastStart[] = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0, 0x81, 0xEC, 0x44, 0x01, 0x00, 0x00, 0x53, 0x56, 0x57, 0x6A};
constexpr uint8_t kFlushStart[] = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0xB8, 0xCC, 0x10, 0x01, 0x00, 0xE8, 0x20, 0xF1, 0xAF, 0xFF};
constexpr uint8_t kLevelStart[] = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0, 0x8B, 0x45, 0x18, 0xD9, 0x45, 0x10, 0x83, 0xEC, 0x4C, 0x56};
// mov ecx,[ebp+10h] / push 1 / call 0177EC70; and mov edx,[esi+0C0h] / mov ecx,[edx+844h] / push 1 / call 0177EC70.
constexpr uint8_t kStepFlushCall[] = {0x8B, 0x4D, 0x10, 0x6A, 0x01, 0xE8, 0xC3, 0xC4, 0xEF, 0xFF};
constexpr uint8_t kLevelFlushCall[] = {0x8B, 0x96, 0xC0, 0x00, 0x00, 0x00, 0x8B, 0x8A, 0x44, 0x08, 0x00, 0x00, 0x6A, 0x01, 0xE8, 0xA4, 0x66, 0x7E, 0x00};
struct Bytes {
    uintptr_t at;
    const uint8_t* bytes;
    uint32_t size;
    const char* what;
};
constexpr Bytes kBytes[] = {
    {kCastAt, kCastStart, sizeof(kCastStart), "the ray cast's start"},
    {kFlushAt, kFlushStart, sizeof(kFlushStart), "the flush's start"},
    {kLevelUpdateAt, kLevelStart, sizeof(kLevelStart), "the level update's start"},
    {kStepFlushBack - sizeof(kStepFlushCall), kStepFlushCall, sizeof(kStepFlushCall), "the physics step's call of the flush"},
    {kLevelFlushBack - sizeof(kLevelFlushCall), kLevelFlushCall, sizeof(kLevelFlushCall), "the level update's own call of the flush"},
};
static_assert(kCode[0].at == kCastAt);  // the census looks at the cast's whole again before each count

// The census's alone (the probe's count of the game's own casts by thread): the caster's own copy of the cast, by its interface (0119A0B0, slot 0 of the
// caster at the world's +0x840): thiscall on the caster, ret 18h: (a name it never reads, from, to, the hit, the flags, things to leave out or null); al 1
// if it hit. It reads the lists at the caster's +4 (0119A0EA) and walks them itself (009F28B0 at 0119A16E). As bf3_mem.exe has it: 0119A0B0-0119A269,
// its two ret 18h at 0119A252 and 0119A266, then int 3s. Looked at whole and byte for byte before each count, as the cast is; not in Verify: its code not
// as read leaves its count alone off, nothing else.
constexpr Code kInterfaceCode = {kCasterFirst, 441, 0xB5B1A3F1C8560EC7ull, "the caster's copy of the cast"};
constexpr uint8_t kInterfaceStart[] = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0, 0x81, 0xEC, 0x34, 0x01, 0x00, 0x00, 0x8B, 0x45, 0x1C, 0x53};

// The hit as the cast fills it (the game sets one up the same way first, 0108E2C0): +0 the point, from + (to - from) * along; +0x10 the normal; +0x20 the
// physics entity hit; +0x24 its material, whose +8 holds the material's index in its low 24 bits (the game reads it so at 00603BA9); +0x28 and +0x2C -1
// to begin with; +0x30 along, 0 at from and 1 at to (2.0 is nothing).
struct alignas(16) Vec4 {
    float v[4];
};
struct alignas(16) GameHit {
    float point[4];
    float normal[4];
    uint32_t entity;
    uint32_t material;
    int32_t part[2];
    float along;
    uint32_t spare[3];
};
static_assert(sizeof(GameHit) == 0x40 && offsetof(GameHit, entity) == 0x20 && offsetof(GameHit, material) == 0x24 && offsetof(GameHit, along) == 0x30);

using CastFn = bool(__fastcall*)(void* world, void* edx, const char* name, int plain, const Vec4* from, const Vec4* to, GameHit* hit, uint32_t flags,
                                 const void* leaveOut);
using InterfaceCastFn = bool(__fastcall*)(void* caster, void* edx, const char* name, const Vec4* from, const Vec4* to, GameHit* hit, uint32_t flags,
                                          const void* leaveOut);
using FlushFn = void(__fastcall*)(void* lists, void* edx, uint32_t now);
using LevelUpdateFn = void(__fastcall*)(void* level, void* edx, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e);

// The flags, each tested by the kind of entity it's about (005D1BF0, 005D1C75, 005C705E, 005CB9FF, 005995BC, 0060C6BE, 005D1511, 005C4FAE): 0x01 the detail
// meshes of the static, moving and breakable ones (what bullets and decals land on), 0x02 no second look at each hit (never set), 0x04 not water, 0x08
// not the terrain, 0x10 not bodies (RagdollEntity), 0x20 not characters (the player's own neither), 0x40 not groups of meshes (GroupPhysicsEntity: they
// may be the level's buildings, never set), 0x80 not trigger volumes. 0xB5: the detail meshes, through water, bodies, characters and trigger volumes. The
// probe's other, 0x34: the coarse meshes, trigger volumes too, as most of the game's own plain casts (its dropped weapons 0x14, the radio altimeter 0x30).
constexpr uint32_t kDefaultFlags = 0xB5, kCoarseFlags = 0x34;

// The physics entities by their vtables (each one's getType names it): what a hit was, and how long a ray that hit it is kept (what doesn't move, long;
// what may move or break, a moment: its ray is cast again when it's asked for after that, so a car driving off leaves nothing on the air).
struct Kind {
    uint32_t vtable;
    What what;
    ULONGLONG keepMs;
    const char* name;
};
constexpr Kind kKinds[] = {
    {0x02082470, ground::kTerrain, 20000, "the terrain"},
    {0x02082810, ground::kStatic, 20000, "a static mesh"},
    {0x020842D8, ground::kGroup, 20000, "a group of meshes"},
    {0x020841B8, ground::kMoving, 400, "something that moves"},
    {0x020810B8, ground::kBreakable, 1000, "something breakable"},
    {0x02082678, ground::kWater, 20000, "water"},
    {0x020822A8, ground::kTrigger, 1000, "a trigger volume"},
    {0x0207FBD8, ground::kCharacter, 200, "a character"},
    {0x02082910, ground::kBody, 1000, "a body"},
};
constexpr ULONGLONG kUnknownKeepMs = 1000, kNothingKeepMs = 1500;
// PhysicsEntity's own vtable, never a live entity's: its constructor (005CA0F0, writing it at 005CA11E) is called only by the static mesh's (005CEDA6,
// which writes its own, 02082810, at 005CEDAD) and the moving one's (0060B8C9, which writes 020841B8 straight after), and its destructor (005C5070) writes
// it back first thing (005C5076) before it lets the body go; nothing else in bf3_mem.exe has it. A hit carrying it is an entity being built or taken
// apart while we cast, the very race the casting point is there to rule out: neither remembered nor answered (kOdd), and said once in the log.
constexpr uint32_t kHalfMadeVtable = 0x02082388;

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// Its own numbers.

// A ray answers for the points within 4 cm of where it was cast across, and no farther (the review: one ray answered for its whole 25 cm column, so by a
// crate's edge a magazine fell through the crate's top and lay inside it, or hung on the air beside it, held firm for up to 20 s); one cast farther off,
// in the point's own column or one beside it, is only carried over (kNear, not firm) till its own comes.
constexpr float kExact = 0.04f;
constexpr float kCell = 0.25f;           // the columns the rays near a point are looked for in (m)
static_assert(kExact < kCell);           // a ray within 4 cm of a point is in its column or one beside it
constexpr int kPerFrame = 6;             // rays cast in one of the game's frames, at most
constexpr int kQueueSize = 32;
constexpr ULONGLONG kStaleMs = 400;      // a ray asked for and not cast by then is let go of (asked again if it's still wanted)
constexpr int kRays = 512;               // rays known
constexpr float kLeastReach = 1.0f, kMostReach = 50.0f;
constexpr float kEyeHeight[3] = {1.6f, 1.0f, 0.3f};  // standing, crouched, prone
constexpr ULONGLONG kFeetEveryMs = 500, kInUseMs = 5000, kCountMs = 10000;
constexpr int kMapSide = 7, kMapCells = kMapSide * kMapSide;
constexpr float kMapStep = 0.5f, kMapReach = 4.0f, kProbeReach = 3.0f;
// The feet's ray's plane is carried this far from where it hit (m); beyond, its height, flat (a slope carried 4 m is 3 m off with a normal's y of 0.8).
constexpr float kFeetSlope = 0.5f;
// A fault in a cast: so long without casting (ms); the third this session leaves it off for the session.
constexpr ULONGLONG kRestMs = 5000;
constexpr int kMostFaults = 3;

// kForSurvey: the eight rays round the soldier when the one down from his eye found no floor at his feet (stood.h), kForSurveyCoarse the eye's own
// again with the probe's coarse flags (for the log alone: whether the coarse meshes see the floor the detail ones don't).
enum Purpose : uint8_t { kForAsker, kForFeet, kForProbe, kForProbeCoarse, kForMap, kForSurvey, kForSurveyCoarse };
enum Result : int8_t { kFaulted = -1, kMissed = 0, kHitIt = 1, kOdd = 2 };

struct Request {
    float from[3];
    float reach;
    uint32_t flags;
    Purpose purpose;
    uint8_t index;    // the map's cell, the survey's ray
    uint32_t serial;  // the probe's, the map's or the survey's round
    ULONGLONG at;
    bool steady;      // the feet's: the soldier steady as it was asked (stood.h), his feet at feet
    float feet;
};

// A ray known: cast straight down from `from`, `reach` metres; what it found. cx, cz: the column it was cast in.
struct Ray {
    bool found;
    int cx, cz;
    float from[3], reach;
    float point[3], normal[3];
    int material;
    What what;
    ULONGLONG at, keep;  // at 0: unused
};

// What one cast found, as the game's thread hands it on.
struct Cast {
    Request asked;
    Result result;
    float point[3], normal[3], along;
    uint32_t entity, vtable, material;  // the hit's entity, its vtable, its material's record
    int index;                          // the material's index (-1)
    What what;
    ULONGLONG keep;
    float micros;
    DWORD fault;     // kFaulted: the exception's code
    bool stackLost;  // kFaulted by a stack overflow whose guard page couldn't be put back
};

std::atomic<bool> g_on{true}, g_wanted{false}, g_dead{false};
std::atomic<int> g_probe{0}, g_where{0};
std::atomic<uint32_t> g_flags{kDefaultFlags};
std::atomic<ULONGLONG> g_lastAsked{0};
std::atomic<int> g_queuedCount{0};  // g_queued, for the game's thread to pass by cheaply

// The threads, for the census (and the log): the level's update (where it casts), the render thread, the eye's, the askers'; and its own cast's
// thread while it casts (the census leaves those out).
std::atomic<DWORD> g_levelThread{0}, g_renderThread{0}, g_eyeThread{0}, g_askThread{0}, g_ourCast{0};

// Faults in a cast (the game's thread notes them, the render thread says them): how many this session, the last one's code, no casts till g_restUntil,
// and a stack overflow whose guard page couldn't be put back. A hit on an entity half built or half taken apart (its address, said once), and the like.
std::atomic<int> g_faults{0};
std::atomic<DWORD> g_faultCode{0};
std::atomic<ULONGLONG> g_restUntil{0};
std::atomic<bool> g_stackLost{false}, g_firstCast{false};
std::atomic<uint32_t> g_oddWorld{0}, g_halfMade{0};

// The render thread's own (OnFrame).
bool g_verified = false, g_flushHooked = false, g_levelHooked = false;
bool g_firstCastSaid = false, g_noEyeSaid = false, g_halfMadeSaid = false;
int g_faultsSaid = 0;
std::atomic<bool> g_censusWanted{false};  // the probe just turned on (weapon.txt's thread)
uint32_t g_oddWorldSaid = 0, g_worldWatched = 0;
ULONGLONG g_countFrom = 0, g_countUntil = 0, g_lastFeet = 0, g_lastProbe = 0, g_lastMap = 0, g_lastTally = 0, g_noEyeSince = 0;
const char* g_countedWhat = "";  // what the count couldn't see, for its line
FlushFn g_flush = nullptr;
LevelUpdateFn g_level = nullptr;
CastFn g_castOriginal = nullptr;
InterfaceCastFn g_interfaceOriginal = nullptr;
LARGE_INTEGER g_ticksPerSecond{};

// The game's thread's own (OnGameThread's, which one thread at a time comes into): what was cast while the lock was busy after, kept to be stored the
// next time it's had, with the generation it was cast for and when it was cast.
std::atomic<bool> g_inside{false};
Cast g_held[kPerFrame];
int g_heldCount = 0;
uint32_t g_heldGeneration = 0;
ULONGLONG g_heldAt = 0;

// The game's own casts counted by thread for the probe's first 10 s: in the client's world and in another (the server's), and how many of them came by
// the caster's interface.
struct Counted {
    std::atomic<DWORD> thread{0};
    std::atomic<uint32_t> client{0}, other{0}, byInterface{0};
};
Counted g_counted[12];
std::atomic<uint32_t> g_countedRest{0}, g_censusWorld{0};
std::atomic<bool> g_counting{false};

// Under g_mutex: the requests, the rays known, the eye, the feet's ray, the probe's and the map's latest, the tally.
std::mutex g_mutex;
Request g_queue[kQueueSize];
int g_queued = 0;
Ray g_rays[kRays];
int g_rayNext = 0;
uint32_t g_generation = 1;  // a new world, or Clear: what's cast for an older one is let go of
float g_eye[3] = {};
int g_stance = 0;
ULONGLONG g_eyeAt = 0;
Ray g_feet{};
uint32_t g_probeSerial = 0, g_probeSaid = 0;
ULONGLONG g_probeAsked = 0;
float g_probeEye[3] = {};
int g_probeStance = 0;
Cast g_probeCast[2]{};
bool g_probeIn[2] = {};
uint32_t g_mapSerial = 0, g_mapSaid = 0;
int g_mapNext = 0;  // the next of its cells to ask for (a few at a time: the queue's short)
ULONGLONG g_mapAsked = 0;
float g_mapEye[3] = {};
Cast g_mapCast[kMapCells]{};
bool g_mapIn[kMapCells] = {};
struct Tally {
    uint32_t casts, hits, odd;
    double micros;
    float most;
    uint32_t answers[6];  // by Source (Planes' kStood counted as well as the answers it was given in place of)
};
Tally g_tally{};
static_assert(ground::kStood < 6);
// The floor the soldier stands on where the rays see none (stood.h): his eye's steadiness, the survey round him, its verdict and the places kept; on
// foot as SetEye last said; the survey's coarse ray (the log's).
stood::Watch g_stood;
bool g_onFoot = false;
Cast g_surveyCoarse{};
bool g_surveyCoarseIn = false;
unsigned g_surveyCoarseSerial = 0;
// The render thread's: when and where the last verdict's line was said.
ULONGLONG g_stoodSaidAt = 0, g_edgeSaidAt = 0;
float g_stoodSaidEye[3] = {};

// ---------------------------------------------------------------------------------------------------------------------------------------------------

bool Read32(uintptr_t at, uint32_t& out) {
    return camera::SafeRead(at, &out, sizeof(out));
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

uint64_t Fnv(const uint8_t* bytes, size_t size) {
    uint64_t h = 14695981039346656037ull;
    for (size_t i = 0; i < size; ++i) {
        h ^= bytes[i];
        h *= 1099511628211ull;
    }
    return h;
}

bool SameHash(const Code& c) {
    uint8_t now[1024];
    return c.size <= sizeof(now) && camera::SafeRead(c.at, now, c.size) && Fnv(now, c.size) == c.fnv;
}

bool SameBytes(uintptr_t at, const uint8_t* bytes, size_t size) {
    uint8_t now[32];
    return size <= sizeof(now) && camera::SafeRead(at, now, size) && std::memcmp(now, bytes, size) == 0;
}

bool Finite3(const float* v) {
    return std::isfinite(v[0]) && std::isfinite(v[1]) && std::isfinite(v[2]);
}

const Kind* KindOf(uint32_t vtable) {
    for (const Kind& k : kKinds)
        if (k.vtable == vtable)
            return &k;
    return nullptr;
}

int CellOf(float v) {
    return static_cast<int>(std::clamp(std::floor(v / kCell), -1e8f, 1e8f));
}

// How long ago a stamp was; 0 for one after now (another thread's, stamped a moment after this one read the time: the review, a ray stored a moment
// ago looked expired then).
ULONGLONG Age(ULONGLONG now, ULONGLONG at) {
    return at > now ? 0 : now - at;
}

// Whether a ray known may still answer.
bool Fresh(const Ray& r, ULONGLONG now) {
    return r.at && Age(now, r.at) <= r.keep;
}

// Whether a ray cast from `from` is the one for the point at: within kExact of it across (x, z).
bool Exact(const float* from, const float* at) {
    const float dx = at[0] - from[0], dz = at[2] - from[2];
    return dx * dx + dz * dz <= kExact * kExact;
}

// Cast in the column (cx, cz) or one beside it: where the rays near a point are looked for (and the ones within 4 cm of it, first).
bool Around(const Ray& r, int cx, int cz) {
    return std::abs(r.cx - cx) <= 1 && std::abs(r.cz - cz) <= 1;
}

// A pointer the game could have given: not in the first 64 KB nor the last, never a 32-bit process's memory. A hit's material is -1 when it has none:
// each candidate's starts as that (017744E2) and is copied on into the hit (01774507-0177452D, 01199CD2); read at +8 it would fault at 7, on the game's
// thread, through every vectored handler first (the review).
bool Pointer(uint32_t p) {
    return p >= 0x10000 && p < 0xFFFF0000u;
}

// The height of a ray's plane right under (x, z); its point's own height if the plane's too steep to carry over.
float PlaneHeight(const Ray& r, float x, float z) {
    if (r.normal[1] < 0.3f)
        return r.point[1];
    return r.point[1] - (r.normal[0] * (x - r.point[0]) + r.normal[2] * (z - r.point[2])) / r.normal[1];
}

// Off for the session, said once. The render thread's.
void Fail(const char* why) {
    if (g_dead.exchange(true))
        return;
    Log("Ground: %s: left off for this session, the soldier's feet answer", why);
}

// Everything known let go of. Under g_mutex.
void ForgetLocked() {
    for (Ray& r : g_rays)
        r.at = 0;
    g_rayNext = 0;
    g_queued = 0;
    g_queuedCount.store(0, std::memory_order_relaxed);
    g_feet = Ray{};
    g_stood.Clear();
    g_surveyCoarseIn = false;
    ++g_generation;
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// The game's thread: the casts.

// The cast itself, guarded: kHitIt, kMissed, or kFaulted (fault the exception's code). Every exception's caught; a stack overflow's leaves the thread
// without its guard page, put back here (_resetstkoflw: in the __except block, never in its filter, the stack's unwound by now), stackLost if it can't
// be (the next overflow would end the game without a word).
Result CastGuarded(uint32_t world, uint32_t flags, const Vec4* from, const Vec4* to, GameHit* hit, DWORD& fault, bool& stackLost) {
    DWORD code = 0;
    __try {
        return reinterpret_cast<CastFn>(kCastAt)(reinterpret_cast<void*>(world), nullptr, "bf3vr ground", 1, from, to, hit, flags, nullptr) ? kHitIt : kMissed;
    } __except (code = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER) {
        fault = code;
        if (code == EXCEPTION_STACK_OVERFLOW && !_resetstkoflw())
            stackLost = true;
        return kFaulted;
    }
}

Cast CastOne(uint32_t world, const Request& q) {
    Cast c{};
    c.asked = q;
    c.index = -1;
    // The cast's from, to and hit on this thread's own stack (the review: as globals, two threads casting would mix their rays), 16-byte aligned (from and
    // to are read with movaps); the hit set up as the game sets one up (0108E2C0): 0, its two parts -1.
    alignas(16) Vec4 from{{q.from[0], q.from[1], q.from[2], 0.0f}};
    alignas(16) Vec4 to{{q.from[0], q.from[1] - q.reach, q.from[2], 0.0f}};
    alignas(16) GameHit hit{};
    hit.part[0] = hit.part[1] = -1;
    LARGE_INTEGER t0, t1;
    QueryPerformanceCounter(&t0);
    g_ourCast.store(GetCurrentThreadId(), std::memory_order_relaxed);
    c.result = CastGuarded(world, q.flags, &from, &to, &hit, c.fault, c.stackLost);
    g_ourCast.store(0, std::memory_order_relaxed);
    QueryPerformanceCounter(&t1);
    if (g_ticksPerSecond.QuadPart > 0)
        c.micros = static_cast<float>(static_cast<double>(t1.QuadPart - t0.QuadPart) * 1e6 / static_cast<double>(g_ticksPerSecond.QuadPart));
    if (c.result != kHitIt)
        return c;
    // What it says it found, looked over: a point and a normal that are numbers, along the ray.
    const GameHit& h = hit;
    const float n = std::sqrt(h.normal[0] * h.normal[0] + h.normal[1] * h.normal[1] + h.normal[2] * h.normal[2]);
    if (!Finite3(h.point) || !std::isfinite(n) || n < 0.5f || n > 1.5f || !std::isfinite(h.along) || h.along < -0.001f || h.along > 1.001f) {
        c.result = kOdd;
        return c;
    }
    for (int i = 0; i < 3; ++i) {
        c.point[i] = h.point[i];
        c.normal[i] = h.normal[i] / n;
    }
    c.along = h.along;
    c.entity = h.entity;
    c.material = h.material;
    if (Pointer(c.entity))
        Read32(c.entity, c.vtable);
    if (c.vtable == kHalfMadeVtable) {
        // An entity half built or half taken apart: nothing of this ray's used (OnFrame says it once).
        g_halfMade.store(c.entity, std::memory_order_relaxed);
        c.result = kOdd;
        return c;
    }
    const Kind* kind = KindOf(c.vtable);
    c.what = kind ? kind->what : ground::kUnknown;
    c.keep = kind ? kind->keepMs : kUnknownKeepMs;
    // Its material's index from its record (+8, the low 24 bits); -1 (no material) and the like never read.
    uint32_t index = 0;
    if (Pointer(c.material) && Read32(c.material + 8, index))
        c.index = static_cast<int>(index & 0xFFFFFF);
    return c;
}

// Up to kPerFrame of the requests, the stale ones let go of: the probe's first, then the feet's, the survey round the soldier's (asked only where the
// feet's found no floor at his feet: a frame or two of them, before what's let go of there has its own far ray back), the loose things', the map's.
// Under g_mutex.
int TakeLocked(Request* out, ULONGLONG now) {
    int kept = 0;
    for (int i = 0; i < g_queued; ++i)
        if (Age(now, g_queue[i].at) <= kStaleMs)
            g_queue[kept++] = g_queue[i];
    g_queued = kept;
    int n = 0;
    for (const Purpose p : {kForProbe, kForProbeCoarse, kForFeet, kForSurvey, kForSurveyCoarse, kForAsker, kForMap}) {
        int left = 0;
        for (int i = 0; i < g_queued; ++i) {
            if (g_queue[i].purpose == p && n < kPerFrame)
                out[n++] = g_queue[i];
            else
                g_queue[left++] = g_queue[i];
        }
        g_queued = left;
    }
    g_queuedCount.store(g_queued, std::memory_order_relaxed);
    return n;
}

// A ray known from now on: in place of one cast within 4 cm of it from about as high (the same place asked again), or the oldest. Under g_mutex.
void RememberLocked(const Ray& ray) {
    Ray* slot = nullptr;
    for (Ray& r : g_rays)
        if (r.at && Exact(r.from, ray.from) && std::fabs(r.from[1] - ray.from[1]) < 0.3f) {
            slot = &r;
            break;
        }
    if (!slot) {
        slot = &g_rays[g_rayNext];
        g_rayNext = (g_rayNext + 1) % kRays;
    }
    *slot = ray;
}

Ray RayOf(const Cast& c, ULONGLONG now) {
    Ray r{};
    r.found = c.result == kHitIt;
    r.cx = CellOf(c.asked.from[0]);
    r.cz = CellOf(c.asked.from[2]);
    std::memcpy(r.from, c.asked.from, sizeof(r.from));
    r.reach = c.asked.reach;
    std::memcpy(r.point, c.point, sizeof(r.point));
    std::memcpy(r.normal, c.normal, sizeof(r.normal));
    r.material = c.index;
    r.what = c.what;
    r.at = now ? now : 1;
    r.keep = r.found ? c.keep : kNothingKeepMs;
    return r;
}

void EnqueueLocked(const float* from, float reach, uint32_t flags, Purpose purpose, uint8_t index, uint32_t serial, ULONGLONG now, bool steady = false,
                   float feet = 0.0f);

// The feet's ray back, steady as it was asked: what it says of the floor under the soldier (stood.h). No floor at his feet and no verdict there yet: the
// eight rays round him asked for (the probe's 3 m each, from his eye's height), and the eye's own again with the coarse flags for the log, if the queue
// has room for all nine (else none: the next miss asks again). Under g_mutex.
void FeetBackLocked(const Cast& c, ULONGLONG now) {
    if (!g_stood.EyeBack(now, c.asked.from, c.asked.steady, c.asked.feet, c.result == kHitIt, c.normal[1], c.point[1]))
        return;
    if (g_queued + stood::kRound + 1 > kQueueSize) {
        g_stood.GiveUp();
        return;
    }
    const stood::Survey& s = g_stood.survey();
    const uint32_t flags = g_flags.load(std::memory_order_relaxed);
    for (int i = 0; i < stood::kRound; ++i) {
        float from[3];
        stood::RoundFrom(s.eye, i, from);
        EnqueueLocked(from, kProbeReach, flags, kForSurvey, static_cast<uint8_t>(i), s.serial, now);
    }
    g_surveyCoarseIn = false;
    g_surveyCoarseSerial = s.serial;
    EnqueueLocked(s.eye, kProbeReach, kCoarseFlags, kForSurveyCoarse, 0, s.serial, now);
}

// A cast's answer put where it's wanted. Under g_mutex.
void StoreLocked(const Cast& c, uint32_t generation, ULONGLONG now) {
    if (c.result != kFaulted) {
        ++g_tally.casts;
        g_tally.hits += c.result == kHitIt;
        g_tally.odd += c.result == kOdd;
        g_tally.micros += c.micros;
        g_tally.most = std::max(g_tally.most, c.micros);
    }
    if (generation != g_generation || c.result == kFaulted)
        return;
    const Purpose p = c.asked.purpose;
    if ((p == kForProbe || p == kForProbeCoarse) && c.asked.serial == g_probeSerial) {
        g_probeCast[p == kForProbe ? 0 : 1] = c;
        g_probeIn[p == kForProbe ? 0 : 1] = true;
    }
    if (p == kForMap && c.asked.serial == g_mapSerial && c.asked.index < kMapCells) {
        g_mapCast[c.asked.index] = c;
        g_mapIn[c.asked.index] = true;
    }
    // The survey round the soldier (stood.h): told to the watch, never remembered (cast from his eye's height round him, as the map's: the review's
    // reason); an odd answer finds no floor of his. Its coarse ray kept for the log.
    if (p == kForSurvey) {
        g_stood.Round(now, c.asked.serial, c.asked.index, c.result == kHitIt, c.normal[1], c.point[1]);
        return;
    }
    if (p == kForSurveyCoarse) {
        if (c.asked.serial == g_surveyCoarseSerial)
            g_surveyCoarse = c, g_surveyCoarseIn = true;
        return;
    }
    // Remembered (and so answering what's asked) unless it's odd, the probe's coarse ray, the map's (cast from the eye's height on its 0.5 m grid, they'd
    // answer for whatever is under them while the probe's on: the review), or cast with flags that are no longer the ones.
    if (c.result == kOdd || p == kForProbeCoarse || p == kForMap || c.asked.flags != g_flags.load(std::memory_order_relaxed))
        return;
    const Ray ray = RayOf(c, now);
    if (p == kForFeet || p == kForProbe)
        g_feet = ray;
    if (p == kForFeet)
        FeetBackLocked(c, now);
    RememberLocked(ray);
}

// One thread at a time in OnGameThread (only ever the level update's: a second one coming in meanwhile would cast nothing), so what it holds is its own.
struct Inside {
    const bool in = !g_inside.exchange(true, std::memory_order_acquire);
    ~Inside() {
        if (in)
            g_inside.store(false, std::memory_order_release);
    }
};

// A cast that faulted. The game guards its own entity ray tests against faults (01774260, its filter at 017742D7) and the cast writes nothing of the
// game's: one is most likely a passing race (the review). 5 s without casting, then on again; the third this session, or a stack whose guard page
// couldn't be put back, leaves it off for the session. OnFrame says each.
void Faulted(const Cast& c) {
    const int faults = g_faults.load(std::memory_order_relaxed) + 1;
    g_faultCode.store(c.fault, std::memory_order_relaxed);
    if (faults >= kMostFaults || c.stackLost) {
        g_stackLost.store(c.stackLost, std::memory_order_relaxed);
        g_dead.store(true, std::memory_order_relaxed);
    } else {
        g_restUntil.store(GetTickCount64() + kRestMs, std::memory_order_relaxed);
    }
    g_faults.store(faults, std::memory_order_release);
}

// Right after the lists are flushed (lists: theirs, groundat 0) or the level's update is over (level: it, groundat 1): what's asked for, cast.
void OnGameThread(uintptr_t lists, uintptr_t level) {
    if (!g_on.load(std::memory_order_relaxed) || g_dead.load(std::memory_order_relaxed))
        return;
    g_levelThread.store(GetCurrentThreadId(), std::memory_order_relaxed);
    const Inside inside{};
    if (!inside.in || (g_queuedCount.load(std::memory_order_relaxed) == 0 && g_heldCount == 0))
        return;
    // Resting after a fault.
    if (const ULONGLONG rest = g_restUntil.load(std::memory_order_relaxed); rest && GetTickCount64() < rest)
        return;
    // The client's world as read, the one whose lists were just flushed or whose level this is.
    uint32_t world = 0, vtable = 0, caster = 0, worldLists = 0;
    if (!Read32(kWorldAt, world) || !world)
        return;
    if (!Read32(world, vtable) || vtable != kWorldVtable || !Read32(world + kCasterAt, caster) || caster != kCasterVtable ||
        !Read32(world + kListsAt, worldLists) || !worldLists) {
        g_oddWorld.store(world, std::memory_order_relaxed);
        return;
    }
    if (lists && lists != worldLists)
        return;
    if (level) {
        uint32_t levelWorld = 0;
        if (!Read32(level + kLevelWorldAt, levelWorld) || levelWorld != world)
            return;
    }
    Request todo[kPerFrame];
    uint32_t generation = 0;
    int n = 0;
    {
        // The lock's never waited for on the game's thread: the render thread holds it in every Down, in SetEye and in OnFrame, and a wait here would be a
        // hitch in the game's frame (the review). Busy, nothing's cast this frame: what's asked for waits a frame.
        std::unique_lock lock(g_mutex, std::try_to_lock);
        if (!lock.owns_lock())
            return;
        for (int i = 0; i < g_heldCount; ++i)
            StoreLocked(g_held[i], g_heldGeneration, g_heldAt);
        g_heldCount = 0;
        generation = g_generation;
        n = TakeLocked(todo, GetTickCount64());
    }
    if (!n)
        return;
    Cast done[kPerFrame];
    int cast = 0;
    while (cast < n) {
        done[cast] = CastOne(world, todo[cast]);
        if (done[cast++].result == kFaulted) {
            Faulted(done[cast - 1]);
            break;
        }
    }
    // Stamped with when they were cast, and stored; the lock busy, held for the next time it's had (the casts before then wait for it).
    const ULONGLONG castAt = GetTickCount64();
    {
        std::unique_lock lock(g_mutex, std::try_to_lock);
        if (lock.owns_lock()) {
            for (int i = 0; i < cast; ++i)
                StoreLocked(done[i], generation, castAt);
        } else {
            std::copy(done, done + cast, g_held);
            g_heldCount = cast;
            g_heldGeneration = generation;
            g_heldAt = castAt;
        }
    }
    g_firstCast.store(true, std::memory_order_relaxed);
}

void __fastcall FlushHook(void* lists, void* edx, uint32_t now) {
    g_flush(lists, edx, now);
    const auto back = reinterpret_cast<uintptr_t>(_ReturnAddress());
    if ((back == kStepFlushBack || back == kLevelFlushBack) && g_where.load(std::memory_order_relaxed) == 0)
        OnGameThread(reinterpret_cast<uintptr_t>(lists), 0);
}

// groundat 1, for a test only: the physics' step has come back without waiting for its new jobs (01882DEA-01882E01), so these casts run beside them;
// quiet only with the physics paused.
void __fastcall LevelHook(void* level, void* edx, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e) {
    g_level(level, edx, a, b, c, d, e);
    if (g_where.load(std::memory_order_relaxed) == 1)
        OnGameThread(0, reinterpret_cast<uintptr_t>(level));
}

// The census: each of the game's own casts counted by its thread, its own left out. On whatever thread the game casts from: atomics only.
void Count(DWORD thread, bool client, bool byInterface) {
    for (Counted& c : g_counted) {
        DWORD seen = c.thread.load(std::memory_order_relaxed);
        if (seen == 0 && c.thread.compare_exchange_strong(seen, thread, std::memory_order_relaxed))
            seen = thread;
        if (seen == thread) {
            (client ? c.client : c.other).fetch_add(1, std::memory_order_relaxed);
            if (byInterface)
                c.byInterface.fetch_add(1, std::memory_order_relaxed);
            return;
        }
    }
    g_countedRest.fetch_add(1, std::memory_order_relaxed);
}

bool __fastcall CastHook(void* world, void* edx, const char* name, int plain, const Vec4* from, const Vec4* to, GameHit* hit, uint32_t flags,
                         const void* leaveOut) {
    if (g_counting.load(std::memory_order_relaxed)) {
        const DWORD thread = GetCurrentThreadId();
        if (g_ourCast.load(std::memory_order_relaxed) != thread)
            Count(thread, reinterpret_cast<uintptr_t>(world) == g_censusWorld.load(std::memory_order_relaxed), false);
    }
    return g_castOriginal(world, edx, name, plain, from, to, hit, flags, leaveOut);
}

// The caster's own copy of the cast, by its interface: the caster is the world's +0x840. Neither calls the other (their calls go to the same helpers of
// the lists'), so nothing's counted twice.
bool __fastcall InterfaceCastHook(void* caster, void* edx, const char* name, const Vec4* from, const Vec4* to, GameHit* hit, uint32_t flags,
                                  const void* leaveOut) {
    if (g_counting.load(std::memory_order_relaxed)) {
        const DWORD thread = GetCurrentThreadId();
        if (g_ourCast.load(std::memory_order_relaxed) != thread)
            Count(thread, reinterpret_cast<uintptr_t>(caster) - kCasterAt == g_censusWorld.load(std::memory_order_relaxed), true);
    }
    return g_interfaceOriginal(caster, edx, name, from, to, hit, flags, leaveOut);
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// The askers' side: what's known.

// Whether a ray cast within 4 cm of a point at `at` answers for it looking howFar down: cast from no lower than it (2 cm let off), and the surface it
// found under it (3 cm into it still counts, as a thing lying there may be; from deeper, the ray it asks for finds the next surface down: callers ask
// from a thing's top or its last place, the review's item 14), or nothing found all the way down as far as asked.
bool Covers(const Ray& r, const float* at, float howFar) {
    if (at[1] > r.from[1] + 0.02f)
        return false;
    if (!r.found)
        return at[1] - howFar >= r.from[1] - r.reach - 0.01f;
    return at[1] >= PlaneHeight(r, at[0], at[2]) - 0.03f;
}

// The newest ray cast within 4 cm of the point that answers for it, found something or not (the review: any ray of its 25 cm column answered, firm).
// Under g_mutex.
const Ray* KnownLocked(const float* at, float howFar, ULONGLONG now) {
    const int cx = CellOf(at[0]), cz = CellOf(at[2]);
    const Ray* best = nullptr;
    for (const Ray& r : g_rays) {
        if (!Around(r, cx, cz) || !Exact(r.from, at) || !Fresh(r, now) || !Covers(r, at, howFar))
            continue;
        if (!best || r.at > best->at)
            best = &r;
    }
    return best;
}

// Meanwhile: the nearest ray cast round it, in its own column or one beside it, whose ground (facing up) is under the point and not far below it, cast
// from about its height. Under g_mutex.
const Ray* NearLocked(const float* at, float howFar, ULONGLONG now) {
    const int cx = CellOf(at[0]), cz = CellOf(at[2]);
    const Ray* best = nullptr;
    float bestDistance = 0.0f;
    for (const Ray& r : g_rays) {
        if (!Around(r, cx, cz) || !r.found || !Fresh(r, now) || r.normal[1] < 0.3f || at[1] > r.from[1] + 0.5f)
            continue;
        const float h = PlaneHeight(r, at[0], at[2]);
        if (at[1] < h - 0.03f || at[1] - h > howFar + 0.25f)
            continue;
        const float dx = at[0] - r.from[0], dz = at[2] - r.from[2], distance = dx * dx + dz * dz;
        if (!best || distance < bestDistance)
            best = &r, bestDistance = distance;
    }
    return best;
}

void FillLocked(Hit& out, const Ray& r, const float* at, Source source) {
    out.point[0] = at[0];
    out.point[1] = PlaneHeight(r, at[0], at[2]);
    out.point[2] = at[2];
    if (r.normal[1] < 0.3f)
        std::memcpy(out.point, r.point, sizeof(out.point));
    std::memcpy(out.normal, r.normal, sizeof(out.normal));
    out.material = r.material;
    out.what = r.what;
    out.source = source;
}

// The soldier's feet: the last ray under the eye while it's fresh and near (its plane within 0.5 m of where it hit; beyond, its height, flat: the review,
// its slope carried 4 m), or the eye less its height for the stance; not for a point well below them. Under g_mutex.
bool FeetLocked(const float* at, ULONGLONG now, Hit& out) {
    if (g_feet.at && g_feet.found && Age(now, g_feet.at) < 3000 && g_feet.normal[1] >= 0.3f) {
        const float dx = at[0] - g_feet.point[0], dz = at[2] - g_feet.point[2], distance = dx * dx + dz * dz;
        const bool sloped = distance <= kFeetSlope * kFeetSlope;
        const float y = sloped ? PlaneHeight(g_feet, at[0], at[2]) : g_feet.point[1];
        if (distance < 16.0f && at[1] >= y - 0.5f) {
            FillLocked(out, g_feet, at, ground::kFeet);
            if (!sloped) {
                out.point[1] = y;
                out.normal[0] = 0.0f, out.normal[1] = 1.0f, out.normal[2] = 0.0f;
            }
            return true;
        }
    }
    if (g_eyeAt && Age(now, g_eyeAt) < 1000) {
        const float y = g_eye[1] - kEyeHeight[g_stance];
        const float dx = at[0] - g_eye[0], dz = at[2] - g_eye[2];
        if (dx * dx + dz * dz < 16.0f && at[1] >= y - 0.5f) {
            out = Hit{};
            out.point[0] = at[0], out.point[1] = y, out.point[2] = at[2];
            out.source = ground::kFeet;
            return true;
        }
    }
    return false;
}

// A ray asked for, unless one that will most likely answer for it is asked for already: within 4 cm of it, from no lower, reaching at least half as far
// below it (what it finds answers for every point above it there; a thing falling asks a little lower each frame). The feet's carry the soldier's
// steadiness as it was asked (stood.h). Under g_mutex.
void EnqueueLocked(const float* from, float reach, uint32_t flags, Purpose purpose, uint8_t index, uint32_t serial, ULONGLONG now, bool steady, float feet) {
    if (purpose == kForAsker)
        for (int i = 0; i < g_queued; ++i) {
            const Request& q = g_queue[i];
            if (q.purpose == kForAsker && Exact(q.from, from) && q.flags == flags && q.from[1] >= from[1] - 0.02f &&
                q.from[1] - q.reach <= from[1] - 0.5f * reach)
                return;
        }
    if (g_queued == kQueueSize) {
        // Full: a loose thing's waits for the next frame (and a survey's, which asks only with room for all of it); the probe's and the feet's make
        // room by letting the oldest loose thing's go.
        if (purpose == kForAsker || purpose == kForMap || purpose == kForSurvey || purpose == kForSurveyCoarse)
            return;
        int oldest = -1;
        for (int i = 0; i < g_queued; ++i)
            if ((g_queue[i].purpose == kForAsker || g_queue[i].purpose == kForMap) && (oldest < 0 || g_queue[i].at < g_queue[oldest].at))
                oldest = i;
        if (oldest < 0)
            return;
        g_queue[oldest] = g_queue[--g_queued];
    }
    Request& q = g_queue[g_queued++];
    std::memcpy(q.from, from, sizeof(q.from));
    q.reach = std::clamp(reach, 0.05f, kMostReach);
    q.flags = flags;
    q.purpose = purpose;
    q.index = index;
    q.serial = serial;
    q.at = now;
    q.steady = steady;
    q.feet = feet;
    g_queuedCount.store(g_queued, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------
// The render thread: the checks, the hooks, what's asked for each frame, the log.

bool Verify() {
    // A level first: before one, the game's code may still be being unpacked, and there's no world to cast in anyway.
    uint32_t world = 0;
    if (!Read32(kWorldAt, world) || !world)
        return false;
    uint32_t vtable = 0, caster = 0, first = 0;
    if (!Read32(world, vtable) || vtable != kWorldVtable || !Read32(world + kCasterAt, caster) || caster != kCasterVtable || !Read32(kCasterVtable, first) ||
        first != kCasterFirst) {
        // A level ending just then (its pointer goes before its world does) isn't a world that isn't as read: only the same one three times running is.
        static uint32_t s_odd = 0;
        static int s_times = 0;
        uint32_t still = 0;
        if (!Read32(kWorldAt, still) || still != world) {
            s_times = 0;
            return false;
        }
        s_times = s_odd == world ? s_times + 1 : 1;
        s_odd = world;
        if (s_times < 3)
            return false;
        char why[200];
        snprintf(why, sizeof(why), "the client's world isn't as read (at %08X: its vtable %08X, its ray caster's %08X, that one's first slot %08X)", world,
                 vtable, caster, first);
        Fail(why);
        return false;
    }
    for (const Code& c : kCode)
        if (!SameHash(c)) {
            char why[160];
            snprintf(why, sizeof(why), "the game's code isn't as read: %s (%u bytes at %08X)", c.what, c.size, static_cast<unsigned>(c.at));
            Fail(why);
            return false;
        }
    for (const Bytes& b : kBytes)
        if (!SameBytes(b.at, b.bytes, b.size)) {
            char why[160];
            snprintf(why, sizeof(why), "the game's code isn't as read: %s (at %08X)", b.what, static_cast<unsigned>(b.at));
            Fail(why);
            return false;
        }
    QueryPerformanceFrequency(&g_ticksPerSecond);
    g_verified = true;
    uint32_t lists = 0;
    Read32(world + kListsAt, lists);
    Log("Ground: the game's code is as read (the ray cast, the lists' flush, the physics step, the level's update, the world's constructor); the client's "
        "world at %08X, its lists at %08X",
        world, lists);
    return true;
}

// One of the game's functions hooked, its first bytes looked at again just before (nothing else of the mod's hooks these). original is set before the
// hook goes on: the game's thread may come through straight away.
bool HookOne(uintptr_t at, const uint8_t* start, size_t size, void* detour, void** original, const char* what) {
    char why[160];
    if (!SameBytes(at, start, size)) {
        snprintf(why, sizeof(why), "%s isn't as read any more (at %08X)", what, static_cast<unsigned>(at));
        Fail(why);
        return false;
    }
    void* trampoline = nullptr;
    if (MH_CreateHook(reinterpret_cast<void*>(at), detour, &trampoline) != MH_OK || !trampoline) {
        snprintf(why, sizeof(why), "%s couldn't be hooked", what);
        Fail(why);
        return false;
    }
    *original = trampoline;
    if (MH_EnableHook(reinterpret_cast<void*>(at)) != MH_OK) {
        MH_RemoveHook(reinterpret_cast<void*>(at));
        *original = nullptr;
        snprintf(why, sizeof(why), "%s's hook couldn't be turned on", what);
        Fail(why);
        return false;
    }
    return true;
}

// The hook where it casts, for groundat 0 or 1 (each put on when it's first wanted; neither taken off: the other's casts are left out).
bool HookWhere() {
    const int where = g_where.load(std::memory_order_relaxed);
    if (where == 0 && !g_flushHooked) {
        if (!HookOne(kFlushAt, kFlushStart, sizeof(kFlushStart), reinterpret_cast<void*>(&FlushHook), reinterpret_cast<void**>(&g_flush), "the lists' flush"))
            return false;
        g_flushHooked = true;
        Log("Ground: casting in the client level's update, right after it has flushed its lists and before its physics' step starts (groundat 0)");
    }
    if (where == 1 && !g_levelHooked) {
        if (!HookOne(kLevelUpdateAt, kLevelStart, sizeof(kLevelStart), reinterpret_cast<void*>(&LevelHook), reinterpret_cast<void**>(&g_level),
                     "the level's update"))
            return false;
        g_levelHooked = true;
        Log("Ground: casting at the end of the client level's update (groundat 1, for a test only: its physics' new step runs in its jobs then, quiet only "
            "with the physics paused)");
    }
    return true;
}

// The census, while the probe's on: the game's own casts counted by thread for 10 s (less if the probe or ground goes off first, the review: it was left
// counting for the session), by the cast and by the caster's own copy of it (0119A0B0: it never calls the cast, so the cast's hook alone could count
// too few, or none). Their hooks are on for the count alone: made the first time, each one's code looked at whole and byte for byte before it goes on,
// taken off after with MH_DisableHook, never MH_RemoveHook (a game thread may be inside a trampoline then: it's never let go of). Either failing leaves
// the other's count, and the rest, as it is.
const char* ThreadLabel(DWORD thread) {
    if (thread && thread == g_levelThread.load(std::memory_order_relaxed))
        return " (the level's update: where these are cast)";
    if (thread && thread == g_renderThread.load(std::memory_order_relaxed))
        return " (the render thread)";
    if (thread && thread == g_eyeThread.load(std::memory_order_relaxed))
        return " (where the eye comes from)";
    return "";
}

// One of the census's hooks. The render thread's own.
struct CensusHook {
    const Code* code;      // its whole function
    const uint8_t* start;  // its first bytes (MinHook's jump goes over the first 6)
    size_t size;
    void* detour;
    void** original;  // set before the hook first goes on: the game's threads may come through straight away
    const char* what;
    bool made = false, on = false, broken = false;
};
CensusHook g_castCensus{&kCode[0], kCastStart, sizeof(kCastStart), reinterpret_cast<void*>(&CastHook), reinterpret_cast<void**>(&g_castOriginal),
                        "the cast"};
CensusHook g_interfaceCensus{&kInterfaceCode, kInterfaceStart, sizeof(kInterfaceStart), reinterpret_cast<void*>(&InterfaceCastHook),
                             reinterpret_cast<void**>(&g_interfaceOriginal), "the caster's copy of the cast"};

// On for a count. Its code is looked at each time (while it's off its bytes are the game's own again); not as read, or not hookable, it's said once and
// never tried again.
bool CensusOn(CensusHook& h) {
    if (h.on)
        return true;
    if (h.broken)
        return false;
    void* const at = reinterpret_cast<void*>(h.code->at);
    const char* failed = nullptr;
    if (!SameBytes(h.code->at, h.start, h.size) || !SameHash(*h.code)) {
        failed = "its code isn't as read";
    } else if (!h.made) {
        void* trampoline = nullptr;
        if (MH_CreateHook(at, h.detour, &trampoline) != MH_OK || !trampoline) {
            failed = "it couldn't be hooked";
        } else {
            *h.original = trampoline;
            h.made = true;
        }
    }
    if (!failed && MH_EnableHook(at) != MH_OK)
        failed = "its hook couldn't be turned on";
    if (failed) {
        h.broken = true;
        Log("Ground: the game's own casts by %s can't be counted (%s, at %08X); the rest goes on", h.what, failed, static_cast<unsigned>(h.code->at));
        return false;
    }
    h.on = true;
    return true;
}

// Off after a count: MH_DisableHook puts the game's own first bytes back; the trampoline stays.
void CensusOff(CensusHook& h) {
    if (!h.on)
        return;
    if (MH_DisableHook(reinterpret_cast<void*>(h.code->at)) != MH_OK) {
        Log("Ground: the census's hook on %s couldn't be turned off: left on, counting nothing (tried again after the next count)", h.what);
        return;
    }
    h.on = false;
}

void StartCount(ULONGLONG now) {
    const bool cast = CensusOn(g_castCensus), byInterface = CensusOn(g_interfaceCensus);
    if (!cast && !byInterface)
        return;
    for (Counted& c : g_counted) {
        c.thread.store(0, std::memory_order_relaxed);
        c.client.store(0, std::memory_order_relaxed);
        c.other.store(0, std::memory_order_relaxed);
        c.byInterface.store(0, std::memory_order_relaxed);
    }
    g_countedRest.store(0, std::memory_order_relaxed);
    g_countFrom = now;
    g_countUntil = now + kCountMs;
    g_countedWhat = cast && byInterface ? ""
                    : cast              ? " (by the cast alone: its copy by the caster's interface can't be counted)"
                                        : " (by the caster's copy alone: the cast itself can't be counted)";
    g_counting.store(true, std::memory_order_relaxed);
    Log("Ground: counting the game's own ray casts by thread for 10 s%s", g_countedWhat);
}

// The count over: at its 10 s, or sooner (why: the probe or ground went off). Its hooks off, and what it counted said.
void EndCount(ULONGLONG now, const char* why) {
    g_counting.store(false, std::memory_order_relaxed);
    CensusOff(g_castCensus);
    CensusOff(g_interfaceCensus);
    char line[1600] = {};
    int used = 0, threads = 0;
    uint32_t all = 0;
    for (const Counted& c : g_counted) {
        const DWORD thread = c.thread.load(std::memory_order_relaxed);
        if (!thread)
            continue;
        const uint32_t client = c.client.load(std::memory_order_relaxed), other = c.other.load(std::memory_order_relaxed),
                       byInterface = c.byInterface.load(std::memory_order_relaxed);
        all += client + other;
        ++threads;
        Append(line, sizeof(line), used, "%s thread %lu%s %u in the client's world, %u in another", threads > 1 ? ";" : "", static_cast<unsigned long>(thread),
               ThreadLabel(thread), client, other);
        if (byInterface)
            Append(line, sizeof(line), used, " (%u of them by the caster's interface)", byInterface);
    }
    const uint32_t rest = g_countedRest.load(std::memory_order_relaxed);
    const double seconds = static_cast<double>(now - g_countFrom) / 1000.0;
    char when[96];
    if (why)
        snprintf(when, sizeof(when), "in %.1f s (%s: the count stopped there)", seconds, why);
    else
        snprintf(when, sizeof(when), "in %.0f s", seconds);
    Log("Ground: %s the game cast %u rays of its own on %d thread%s:%s%s%s; its rays here cast on thread %lu, asked for on %lu, the eye given on %lu, the "
        "render thread %lu",
        when, all + rest, threads, threads == 1 ? "" : "s", threads ? line : " none", rest ? " (and more threads than counted)" : "", g_countedWhat,
        static_cast<unsigned long>(g_levelThread.load(std::memory_order_relaxed)), static_cast<unsigned long>(g_askThread.load(std::memory_order_relaxed)),
        static_cast<unsigned long>(g_eyeThread.load(std::memory_order_relaxed)), static_cast<unsigned long>(g_renderThread.load(std::memory_order_relaxed)));
}

void Census(ULONGLONG now) {
    if (g_counting.load(std::memory_order_relaxed)) {
        if (now >= g_countUntil)
            EndCount(now, nullptr);
        return;
    }
    if (g_censusWanted.exchange(false, std::memory_order_relaxed))
        StartCount(now);
}

// The client's world changing: what was known of the last one let go of.
void WatchWorld() {
    uint32_t world = 0;
    Read32(kWorldAt, world);
    g_censusWorld.store(world, std::memory_order_relaxed);
    if (world == g_worldWatched)
        return;
    const uint32_t was = g_worldWatched;
    g_worldWatched = world;
    {
        std::lock_guard lock(g_mutex);
        ForgetLocked();
    }
    if (!g_verified)
        return;
    if (world) {
        uint32_t lists = 0;
        Read32(world + kListsAt, lists);
        Log("Ground: the client's world at %08X, its lists at %08X%s", world, lists, was ? " (another level)" : "");
    } else {
        Log("Ground: the client's world is gone (the level's ending): what was known of it is let go of");
    }
}

const char* KindName(uint32_t vtable) {
    const Kind* k = KindOf(vtable);
    return k ? k->name : "something else";
}

void SayCast(const char* what, const Cast& c, const float* eye, float reach, const char* more) {
    if (c.result == kMissed) {
        Log("Ground: %s: nothing within %.1f m, %.0f us%s", what, reach, c.micros, more);
    } else if (c.result == kOdd && c.vtable == kHalfMadeVtable) {
        Log("Ground: %s: an entity half built or half taken apart (entity %08X, vtable %08X), not used, %.0f us%s", what, c.entity, c.vtable, c.micros, more);
    } else if (c.result == kOdd) {
        Log("Ground: %s: an odd answer (a point or normal that isn't one), %.0f us%s", what, c.micros, more);
    } else {
        Log("Ground: %s: %.2f m below the eye (y %.2f), normal %.2f %.2f %.2f, material %d (its record at %08X), %s (entity %08X, vtable %08X), along %.3f, "
            "%.0f us%s",
            what, eye[1] - c.point[1], c.point[1], c.normal[0], c.normal[1], c.normal[2], c.index, c.material, KindName(c.vtable), c.entity, c.vtable,
            c.along, c.micros, more);
    }
}

// The probe's two rays as they come in (or a second on, what hasn't).
void SayProbe(ULONGLONG now) {
    Cast cast[2];
    bool in[2];
    float eye[3];
    int stance;
    uint32_t serial;
    {
        std::lock_guard lock(g_mutex);
        serial = g_probeSerial;
        if (!serial || serial == g_probeSaid || (!(g_probeIn[0] && g_probeIn[1]) && now - g_probeAsked < 1000))
            return;
        g_probeSaid = serial;
        cast[0] = g_probeCast[0], cast[1] = g_probeCast[1];
        in[0] = g_probeIn[0], in[1] = g_probeIn[1];
        std::memcpy(eye, g_probeEye, sizeof(eye));
        stance = g_probeStance;
    }
    if (!in[0] && !in[1]) {
        Log("Ground: probe from the eye at %.2f %.2f %.2f: no ray cast within a second (the level's flush hasn't come by: its physics paused? groundat 1, "
            "for a test only, casts at the end of its update instead; or resting after a fault)",
            eye[0], eye[1], eye[2]);
        return;
    }
    char what[96], more[64];
    snprintf(what, sizeof(what), "probe (0x%02X) from the eye at %.2f %.2f %.2f", g_flags.load(std::memory_order_relaxed), eye[0], eye[1], eye[2]);
    snprintf(more, sizeof(more), "; the stance would say %.2f m", kEyeHeight[stance]);
    if (in[0])
        SayCast(what, cast[0], eye, kProbeReach, more);
    snprintf(what, sizeof(what), "probe (0x%02X), the same ray", kCoarseFlags);
    if (in[1])
        SayCast(what, cast[1], eye, kProbeReach, "");
}

// The map round the eye once it's all in (or two seconds on): each cell's ground in cm over the ground under the eye.
void SayMap(ULONGLONG now) {
    Cast cast[kMapCells];
    bool in[kMapCells];
    float eye[3];
    {
        std::lock_guard lock(g_mutex);
        if (!g_mapSerial || g_mapSerial == g_mapSaid)
            return;
        int count = 0;
        for (bool b : g_mapIn)
            count += b;
        if (count < kMapCells && now - g_mapAsked < 3000)
            return;
        g_mapSaid = g_mapSerial;
        std::copy(std::begin(g_mapCast), std::end(g_mapCast), cast);
        std::copy(std::begin(g_mapIn), std::end(g_mapIn), in);
        std::memcpy(eye, g_mapEye, sizeof(eye));
    }
    const int middle = kMapCells / 2;
    const bool base = in[middle] && cast[middle].result == kHitIt;
    const float under = base ? cast[middle].point[1] : eye[1] - kEyeHeight[0];
    Log("Ground: heights round the eye at %.2f %.2f %.2f, in cm over the ground under it (y %.2f%s), %.1f m apart; rows from z %+.1f to %+.1f m, each from x "
        "%+.1f to %+.1f m; -- nothing within %.0f m, ?? not cast",
        eye[0], eye[1], eye[2], under, base ? "" : ", the stance's", kMapStep, -kMapStep * (kMapSide / 2), kMapStep * (kMapSide / 2),
        -kMapStep * (kMapSide / 2), kMapStep * (kMapSide / 2), kMapReach);
    for (int row = 0; row < kMapSide; ++row) {
        char line[200] = {};
        int used = 0;
        for (int column = 0; column < kMapSide; ++column) {
            const Cast& c = cast[row * kMapSide + column];
            if (!in[row * kMapSide + column])
                Append(line, sizeof(line), used, "    ??");
            else if (c.result != kHitIt)
                Append(line, sizeof(line), used, "    --");
            else
                Append(line, sizeof(line), used, " %+5.0f", (c.point[1] - under) * 100.0f);
        }
        Log("Ground:  %s", line);
    }
}

// A survey's verdict round the soldier (stood.h): a floor the rays don't see, kept (said again 3 m on, or 10 s on), or an edge of one they do (once in
// 30 s); the coarse meshes' ray from his eye with the first. The render thread's.
void SayStood(ULONGLONG now, const stood::Verdict& v, const Cast& coarse, bool coarseIn, int prints) {
    char eyeRay[96];
    if (v.eyeFound)
        snprintf(eyeRay, sizeof(eyeRay), "found a floor %.2f m below his feet", v.feet - v.eyeY);
    else
        snprintf(eyeRay, sizeof(eyeRay), "found nothing within %.0f m", kProbeReach);
    if (!v.unseen) {
        if (g_edgeSaidAt && now - g_edgeSaidAt < 30000)
            return;
        g_edgeSaidAt = now;
        Log("Ground: the ray down from the soldier's eye (at %.2f %.2f %.2f) %s, but %d of the %d rays %.1f m round him found his floor (y %.2f): an "
            "edge he stands at, nothing kept",
            v.eye[0], v.eye[1], v.eye[2], eyeRay, v.atFeet, stood::kRound, stood::kRoundAt, v.feet);
        return;
    }
    const float dx = v.eye[0] - g_stoodSaidEye[0], dz = v.eye[2] - g_stoodSaidEye[2];
    if (g_stoodSaidAt && now - g_stoodSaidAt < 10000 && dx * dx + dz * dz < 9.0f)
        return;
    g_stoodSaidAt = now;
    std::memcpy(g_stoodSaidEye, v.eye, sizeof(g_stoodSaidEye));
    Log("Ground: no floor the game's ray cast sees under the soldier (his eye at %.2f %.2f %.2f, his feet at y %.2f): the ray down from his eye %s, %d "
        "of the %d rays %.1f m round him found his floor; where he stands is kept as the floor for what's let go of within %.1f m of it (%d places kept)",
        v.eye[0], v.eye[1], v.eye[2], v.feet, eyeRay, v.atFeet, stood::kRound, stood::kRoundAt, stood::kReach, prints);
    if (coarseIn) {
        char what[96];
        snprintf(what, sizeof(what), "the same ray down from his eye with the coarse meshes (0x%02X)", kCoarseFlags);
        SayCast(what, coarse, v.eye, kProbeReach, "");
    }
}

// Once a second with the probe on, every 10 s otherwise (when anything happened): the rays cast and the answers given.
void SayTally(ULONGLONG now, int probe) {
    if (now - g_lastTally < (probe > 0 ? 1000u : 10000u))
        return;
    const double seconds = g_lastTally ? static_cast<double>(now - g_lastTally) / 1000.0 : 1.0;
    g_lastTally = now;
    Tally t;
    {
        std::lock_guard lock(g_mutex);
        t = g_tally;
        g_tally = Tally{};
    }
    uint32_t answers = 0;
    for (uint32_t a : t.answers)
        answers += a;
    if (!t.casts && !answers)
        return;
    const uint32_t stoodAnswers = t.answers[ground::kStood];
    answers -= stoodAnswers;  // (each given in place of one of the others)
    Log("Ground: in the last %.0f s, %u rays cast on thread %lu (%.0f us each, at most %.0f), %u hit, %u odd; %u answers asked on thread %lu: %u from a ray "
        "here, %u from one beside, %u the feet, %u nothing below, %u not known yet; %u given the floor the soldier stood on in their place (where the rays "
        "see none)",
        seconds, t.casts, static_cast<unsigned long>(g_levelThread.load(std::memory_order_relaxed)), t.casts ? t.micros / t.casts : 0.0,
        static_cast<double>(t.most), t.hits, t.odd, answers, static_cast<unsigned long>(g_askThread.load(std::memory_order_relaxed)),
        t.answers[ground::kRay], t.answers[ground::kNear], t.answers[ground::kFeet], t.answers[ground::kNothing], t.answers[ground::kNone], stoodAnswers);
}

}  // namespace

namespace ground {

bool Down(const float* at, float howFar, Hit& out) {
    out = Hit{};
    if (!at || !Finite3(at) || !std::isfinite(howFar))
        return false;
    howFar = std::clamp(howFar, 0.01f, kMostReach);
    g_wanted.store(true, std::memory_order_relaxed);
    g_askThread.store(GetCurrentThreadId(), std::memory_order_relaxed);
    const bool casting = g_on.load(std::memory_order_relaxed) && !g_dead.load(std::memory_order_relaxed);
    std::lock_guard lock(g_mutex);
    // The time read with the lock held: what the game's thread stores under it is stamped no later than this (the review: read before it, a ray stored a
    // moment ago looked expired, was cast again, and the near rays and the feet passed it by).
    const ULONGLONG now = GetTickCount64();
    g_lastAsked.store(now, std::memory_order_relaxed);
    if (casting) {
        if (const Ray* r = KnownLocked(at, howFar, now)) {
            if (r->found) {
                FillLocked(out, *r, at, kRay);
                ++g_tally.answers[kRay];
                return true;
            }
            out.source = kNothing;
            ++g_tally.answers[kNothing];
            return false;
        }
        EnqueueLocked(at, std::max(howFar, kLeastReach), g_flags.load(std::memory_order_relaxed), kForAsker, 0, 0, now);
        if (const Ray* r = NearLocked(at, howFar, now)) {
            FillLocked(out, *r, at, kNear);
            ++g_tally.answers[kNear];
            return true;
        }
    }
    if (FeetLocked(at, now, out)) {
        ++g_tally.answers[kFeet];
        return true;
    }
    out.source = kNone;
    ++g_tally.answers[kNone];
    return false;
}

int Planes(const float* atArms, const float* origin, Plane* planes, int most) {
    if (!atArms || !origin || !planes || most < 1)
        return 0;
    const float at[3] = {atArms[0] + origin[0], atArms[1] + origin[1], atArms[2] + origin[2]};
    Hit h;
    int n = 0;  // the rays': 1 a floor facing up, 0 none known now (or an underside), -1 nothing as far as 50 m
    bool found = Down(at, 6.0f, h);
    if (!found && h.source == kNothing) {
        found = Down(at, kMostReach, h);
        // Nothing there even 50 m down (off an edge, say): -1, loose's "nothing there", never 0 (the review: hands.cpp then put its guess under it, and a
        // thing dropped there lay in the air at the feet's height).
        if (!found && h.source == kNothing)
            n = -1;
    }
    if (n == 0 && found && h.normal[1] > 0.05f)
        n = 1;
    // The floor the soldier stood on where the rays see none (stood.h), within its reach of the point and under it: in place of the rays' answer when
    // they know nothing, find nothing, or only a guess or a floor further down than his (Swordbreaker's overpass: the embankment 7 m under the deck).
    // A floor of theirs at his height or above it (a crate on the deck) is theirs.
    {
        std::lock_guard lock(g_mutex);
        float y = 0.0f;
        if (g_stood.Under(at, y)) {
            const stood::Rays rays = n < 0                                ? stood::Rays::kNothing
                                     : n == 0 || h.source == kFeet        ? stood::Rays::kNone
                                     : h.source == kRay                   ? stood::Rays::kFloor
                                                                          : stood::Rays::kGuess;
            if (stood::StandsIn(rays, h.point[1], y)) {
                Plane& p = planes[0];
                p.point[0] = atArms[0], p.point[1] = y - origin[1], p.point[2] = atArms[2];
                p.normal[0] = 0.0f, p.normal[1] = 1.0f, p.normal[2] = 0.0f;
                p.material = -1;
                p.what = kUnknown;
                p.source = kStood;
                ++g_tally.answers[kStood];
                return 1;
            }
        }
    }
    if (n != 1)
        return n;
    Plane& p = planes[0];
    for (int i = 0; i < 3; ++i) {
        p.point[i] = h.point[i] - origin[i];
        p.normal[i] = h.normal[i];
    }
    p.material = h.material;
    p.what = h.what;
    p.source = h.source;
    return 1;
}

void SetEye(const float* eye, int stance, bool onFoot, int bodyPose) {
    if (!eye || !Finite3(eye))
        return;
    g_eyeThread.store(GetCurrentThreadId(), std::memory_order_relaxed);
    std::lock_guard lock(g_mutex);
    std::memcpy(g_eye, eye, sizeof(g_eye));
    g_stance = std::clamp(stance, 0, 2);
    g_eyeAt = GetTickCount64();
    g_onFoot = onFoot;
    // (His feet for the floor kept go by his body's own pose alone: a belief's or the head's crouch the game didn't take would put them 0.6 m off.)
    const bool known = bodyPose >= 0 && bodyPose <= 2;
    g_stood.Eye(g_eyeAt, g_eye, known ? bodyPose : 0, onFoot && known);
}

void SetOn(bool on) {
    if (g_on.exchange(on) != on)
        Log("Ground: %s", on ? "on: the game's own ray cast once something asks (ground 1)" : "off: nothing of the game's called, the feet answer (ground 0)");
}

void SetProbe(int level) {
    level = std::clamp(level, 0, 2);
    const int was = g_probe.exchange(level);
    if (was == level)
        return;
    if (level > 0 && was <= 0)
        g_censusWanted.store(true, std::memory_order_relaxed);
    Log("Ground: the probe %s", level == 0 ? "off" : level == 1 ? "on (groundprobe 1: a ray down from the eye each second)"
                                                                : "on (groundprobe 2: a ray down from the eye each second, a map of heights every 5 s)");
}

void SetWhere(int where) {
    where = where == 1 ? 1 : 0;
    if (g_where.exchange(where) != where)
        Log("Ground: casting %s", where == 1 ? "at the end of the level's update (groundat 1, for a test only: quiet only with the physics paused)"
                                             : "right after the level's flush of its lists (groundat 0)");
}

void SetFlags(uint32_t flags) {
    flags &= 0xFF;
    if (g_flags.exchange(flags) == flags)
        return;
    Clear();
    Log("Ground: rays cast with flags 0x%02X (groundflags)", flags);
}

void Clear() {
    std::lock_guard lock(g_mutex);
    ForgetLocked();
}

void OnFrame() {
    g_renderThread.store(GetCurrentThreadId(), std::memory_order_relaxed);
    const ULONGLONG now = GetTickCount64();
    // Each fault in a cast said: 5 s without casting after one; the third this session, or a stack whose guard page couldn't be put back, off for it.
    if (const int faults = g_faults.load(std::memory_order_acquire); faults > g_faultsSaid) {
        g_faultsSaid = faults;
        const unsigned long code = static_cast<unsigned long>(g_faultCode.load(std::memory_order_relaxed));
        if (g_stackLost.load(std::memory_order_relaxed))
            Log("Ground: the game's ray cast overflowed the stack (exception %08lX) and its guard page couldn't be put back: no more rays this session, the "
                "soldier's feet answer",
                code);
        else if (faults >= kMostFaults)
            Log("Ground: the game's ray cast faulted (exception %08lX, %d times this session): no more rays this session, the soldier's feet answer", code,
                faults);
        else
            Log("Ground: the game's ray cast faulted (exception %08lX, %d of %d this session before it's left off): no rays for 5 s, what's known and the "
                "soldier's feet answer meanwhile",
                code, faults, kMostFaults);
    }
    if (const uint32_t entity = g_halfMade.load(std::memory_order_relaxed); entity && !g_halfMadeSaid) {
        g_halfMadeSaid = true;
        Log("Ground: warning: a ray hit an entity half built or half taken apart (entity %08X, vtable %08X: PhysicsEntity's own, there only while one is "
            "being built or taken apart): the game was changing its world while it cast. That ray's not used; said once, however many more",
            entity, kHalfMadeVtable);
    }
    const int probe = g_probe.load(std::memory_order_relaxed);
    // A count of the game's own casts ends at its 10 s, or as soon as the probe or ground goes off, its hooks off with it.
    if (g_counting.load(std::memory_order_relaxed) && (probe <= 0 || !g_on.load(std::memory_order_relaxed) || g_dead.load(std::memory_order_relaxed)))
        EndCount(now, probe <= 0 ? "the probe went off" : "ground's off");
    if (!g_on.load(std::memory_order_relaxed) || g_dead.load(std::memory_order_relaxed))
        return;
    WatchWorld();
    const bool inUse = g_wanted.load(std::memory_order_relaxed) && Age(now, g_lastAsked.load(std::memory_order_relaxed)) < kInUseMs;
    if (!g_verified && ((!inUse && probe <= 0) || !Verify()))
        return;
    if (!HookWhere())
        return;
    if (g_firstCast.load(std::memory_order_relaxed) && !g_firstCastSaid) {
        g_firstCastSaid = true;
        Log("Ground: the first rays cast, on thread %lu", static_cast<unsigned long>(g_levelThread.load(std::memory_order_relaxed)));
    }
    if (const uint32_t odd = g_oddWorld.load(std::memory_order_relaxed); odd && odd != g_oddWorldSaid) {
        g_oddWorldSaid = odd;
        Log("Ground: the client's world at %08X isn't as read when casting: nothing cast in it", odd);
    }
    if (probe > 0)
        Census(now);
    // The feet's ray twice a second while anything asks, and on foot once anything has (hooked: here, by now): the floor under the soldier looked at
    // before anything's let go of on it (stood.h: one the rays don't see is kept from his eye); the probe's each second, its map every 5 s.
    bool eyeFresh = false;
    stood::Verdict verdict;
    bool sayVerdict = false, coarseIn = false;
    Cast coarse{};
    int prints = 0;
    {
        std::lock_guard lock(g_mutex);
        eyeFresh = g_eyeAt && Age(now, g_eyeAt) < 500;
        const uint32_t flags = g_flags.load(std::memory_order_relaxed);
        if (eyeFresh && probe > 0 && now - g_lastProbe >= 1000) {
            g_lastProbe = g_lastFeet = now;
            ++g_probeSerial;
            g_probeAsked = now;
            std::memcpy(g_probeEye, g_eye, sizeof(g_probeEye));
            g_probeStance = g_stance;
            g_probeIn[0] = g_probeIn[1] = false;
            EnqueueLocked(g_eye, kProbeReach, flags, kForProbe, 0, g_probeSerial, now);
            EnqueueLocked(g_eye, kProbeReach, kCoarseFlags, kForProbeCoarse, 0, g_probeSerial, now);
        } else if (eyeFresh && (inUse || g_onFoot) && now - g_lastFeet >= kFeetEveryMs) {
            g_lastFeet = now;
            float feet = 0.0f;
            const bool steady = g_stood.Steady(now, feet);
            EnqueueLocked(g_eye, kProbeReach, flags, kForFeet, 0, 0, now, steady, feet);
        }
        // The survey's time out, and while the floor round him is one the rays don't see, the places he walks to kept; a new verdict to say (one of unseen
        // with its coarse ray, once that's back or 0.3 s on).
        g_stood.Frame(now, eyeFresh ? g_eye : nullptr);
        if (!g_stood.Said()) {
            const stood::Verdict& v = g_stood.verdict();
            const bool coarseBack = g_surveyCoarseIn && g_surveyCoarseSerial == v.serial;
            if (!v.have || !v.unseen || coarseBack || (now > v.at && now - v.at > 300)) {
                g_stood.Say();
                verdict = v;
                sayVerdict = v.have;
                coarse = g_surveyCoarse;
                coarseIn = coarseBack;
                prints = g_stood.Prints();
            }
        }
        if (eyeFresh && probe >= 2 && now - g_lastMap >= 5000 && g_mapSerial == g_mapSaid) {
            g_lastMap = now;
            ++g_mapSerial;
            g_mapAsked = now;
            g_mapNext = 0;
            std::memcpy(g_mapEye, g_eye, sizeof(g_mapEye));
            std::fill(std::begin(g_mapIn), std::end(g_mapIn), false);
        }
        // Its cells a few at a time, leaving the queue room for the rest: the map waits behind everything else (TakeLocked), and has 3 s.
        while (g_mapSerial != g_mapSaid && g_mapNext < kMapCells && g_queued < kQueueSize - 8) {
            const int i = g_mapNext++;
            const float from[3] = {g_mapEye[0] + kMapStep * static_cast<float>(i % kMapSide - kMapSide / 2), g_mapEye[1],
                                   g_mapEye[2] + kMapStep * static_cast<float>(i / kMapSide - kMapSide / 2)};
            EnqueueLocked(from, kMapReach, flags, kForMap, static_cast<uint8_t>(i), g_mapSerial, now);
        }
    }
    // The probe can't start without the eye: said once, 3 s on, for whoever wires it up.
    if (probe > 0 && !eyeFresh) {
        if (!g_noEyeSince)
            g_noEyeSince = now;
        else if (!g_noEyeSaid && now - g_noEyeSince > 3000) {
            g_noEyeSaid = true;
            Log("Ground: the probe waits for the eye (nothing's given it through ground::SetEye for 3 s)");
        }
    } else {
        g_noEyeSince = 0;
    }
    if (sayVerdict)
        SayStood(now, verdict, coarse, coarseIn, prints);
    SayProbe(now);
    SayMap(now);
    SayTally(now, probe);
}

}  // namespace ground

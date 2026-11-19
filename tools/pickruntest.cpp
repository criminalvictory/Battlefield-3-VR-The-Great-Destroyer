// The game's side of its dropped guns (src/pickups.cpp, compiled in here) against fake game memory laid out as the game's (a server world with
// its segments of pickups, their data and content, blueprints with names and PickupSettings, a soldier with ten slots and firing states, its
// interaction component with a target and flags, a client world with a pickup and its MeshSet), the game's functions the test's own (a take that
// swaps the slots as 008AB870 does, AllowPickup, the check, three gates, the interaction update), every call out of it stubbed. The detour is
// called directly. Each check prints PASS or FAIL, then the totals; the exit code is the number failed. Its pickups.txt is in a folder of its own
// under the temp folder, never beside the DLL.
//   R1  Verify: as read: the line, then the hook; a byte changed in a function, a call site, a float: off for the session, said once, nothing
//       hooked; no level: nothing looked at. The real tables against build\bf3_mem.exe when it's there
//   R2  the hook: in right after the checks (unless off); "off" (pickups.txt, bf3vr.ini): never, and the hook once in passes straight through
//   R3  a soldier that isn't the player's (another address; the same address with another data pointer): straight through, nothing published
//   R4  the tick: the slots' triples, names, counts and words; the game's target with flag 4 clear, none with it set; published every tick
//   R5  the walk only when wanted; the nearest kMost kept, nearest first; first skips a null entry; allowed, eligible, keepsNoPlace, own; nothing
//       eligible while a gate is up
//   R6  a hand's request: served in the tick after; the swap's fields; ammo told; TakeOpen up from the ask to the outcome; an unspawning one removed
//   R7  each refusal with its words, in order; nothing written to a count, the take never called
//   R8  the hand-over: the counts written right before the take and only then; a take that changes nothing: "the game swapped nothing", the counts
//       written back; replaceAll: every slot; a take landed by its counts alone; a gun the take leaves in its slot gets its count back (the
//       level's Stinger, 2026-11-12: the M416 kept the mod's count, a shot by it with none fired); an entry naming slot 0 whose gun the game puts
//       into slot 1: the M416 left in slot 0 gets its count back too (a dead soldier's AN-94, 2026-11-13)
//   R9  the game's own take (flag 4's edge inside the update): byHand false, its entity the target read before; the target changed inside: the after one
//   R10 a fault in the take: no more takes, the tick goes on, nothing written back; faults in the walk: 5 s rest, the third dead
//   R11 the policy: nothing written at the defaults; the player's own back up under 30 s; seconds 40 once; unseen 1 held in view, let go out of it;
//       the nearest 48 with "never"
//   R12 a request nobody serves: dropped after 1 s, TakeOpen down, said once, RequestTake asks again
//   R13 Reachable: only allowed, eligible, with a blueprint, not keepsNoPlace, within reach, nearest first; the rifle's box until learned; a client
//       box taken only with "3p" and the gun's last part in its name, read twice a second apart
//   R14 GameWouldTake 1, 0, -1; SlotOf by name, any case; ActiveSlot -1 without "slots 1"
//   R15 pickups.txt: each key; deleted: every default; take once per write
//   R16 no log line the self-test would take for a failure, but the fault lines
//   R17 pickups.txt's "parts" (src/pickparts.cpp, compiled in too): 0 to 4 read, off or level 0 makes it 0, no other key moved by it, no key
//       the start of another, "parts" no part of another nor another of it; no line: never a word of it said in any row
//   R18 a scripted use point (the AT4's hand-over): published; a hand's use served at the next tick with the game's use (a stand-in); its outcome
//       the game's thread's own (a stale tick doesn't lose it); taken and under way: waits, 5 s at most; not taken in 1 s: given up and passed by;
//       each refusal, on either thread; only a prompt a hand uses (the roof's pose prompts, holds, 300 m prompts, jumps: never asked); the soldier
//       asleep; "use 0"; a use and a take never served in one tick; a fault: never called again, nor its key; the checks not as read
// Usage: pickruntest [log]   (log: a failed check shows what was logged since its row began)

#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>
#include <vector>

// MinHook as pickups.cpp calls it: the hook made in the test (the detour is called directly; the game's update is the test's own).
#define BF3VR_PICKUPS_TEST
extern "C" {
typedef enum MH_STATUS { MH_OK = 0, MH_ERROR_NOT_CREATED = 9 } MH_STATUS;
MH_STATUS WINAPI MH_CreateHook(LPVOID target, LPVOID detour, LPVOID* original);
MH_STATUS WINAPI MH_EnableHook(LPVOID target);
MH_STATUS WINAPI MH_RemoveHook(LPVOID target);
}
// The game's clock, the test's.
static ULONGLONG g_now = 1000000;
static ULONGLONG FakeTicks() {
    return g_now;
}
#define GetTickCount64 FakeTicks
#include "../src/pickups.cpp"
#include "../src/pickparts.cpp"
#undef GetTickCount64

namespace {

int g_failures = 0, g_counted = 0;
bool g_showLog = false;  // "pickruntest log": a failure shows the log since the row's mark
void ShowSince();

void Check(bool ok, const char* row, const char* what) {
    ++g_counted;
    if (!ok)
        ++g_failures;
    std::printf("%s %s %s\n", ok ? "PASS" : "FAIL", row, what);
    if (!ok && g_showLog)
        ShowSince();
}

// ---- The log ----
std::vector<std::string> g_log;
size_t g_logMark = 0;
void Mark() {
    g_logMark = g_log.size();
}
// Lines since the mark with that text in them.
int Said(const char* text) {
    int n = 0;
    for (size_t i = g_logMark; i < g_log.size(); ++i)
        if (g_log[i].find(text) != std::string::npos)
            ++n;
    return n;
}
void ShowSince() {
    for (size_t i = g_logMark; i < g_log.size(); ++i)
        std::printf("      log: %s\n", g_log[i].c_str());
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
template <typename F> uint32_t Fn(F f) {
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(f));
}

// Tags the game's vtables are told by (only compared), and the ones read through (the gates').
constexpr uint32_t kServerWorldVt = 0x71000010, kClientWorldVt = 0x71000020, kPickupVt = 0x71000030, kClientPickupVt = 0x71000040,
                   kComponentVt = 0x71000050, kSoldierVt = 0x71000060, kFiringVt = 0x71000070, kDropDataVt = 0x71000080, kPlacedDataVt = 0x71000090;
constexpr uint16_t kFirstClass = 0x100, kPickupClass = 0x2EA, kClientClass = 0x29C;

struct Blueprint {
    uint32_t at = 0;
    const char* name = "";
};
Blueprint g_m416, g_m320, g_an94, g_rpk, g_stinger, g_odd;

struct Entry {
    uint32_t blueprint;
    int slot, linked, clip, spare;
};

// The scene.
uint32_t g_worldCell, g_world, g_lists, g_type, g_segment, g_entities;
uint32_t g_clientWorldCell, g_clientWorld, g_clientLists, g_clientType, g_clientSegment, g_clientEntities;
uint32_t g_soldier, g_soldierData, g_soldierXform, g_weapons, g_client, g_comp, g_token;  // (the component's +0x1C points at g_token: entity + 4)
uint32_t g_gateTable[3], g_updateStart;
uint32_t g_loose = 0;
float g_soldierAt[3] = {1260.0f, 65.0f, -638.0f};

// The game's functions, the test's.
enum TakeMode { kSwap, kNothing, kCountsOnly, kFault, kIntoNext };
TakeMode g_takeMode = kSwap;
int g_takes = 0, g_updates = 0, g_allows = 0, g_checkCalls = 0, g_gateCalls = 0;
bool g_allowFault = false, g_check = true, g_gateUp[3] = {};
std::vector<uint32_t> g_notAllowed;
int g_carried[10] = {};  // the count each outgoing gun had at the take
bool g_gameTakes = false, g_rHeld = false;
uint32_t g_retargetTo = 0;

uint32_t MakeBlueprint(const char* name) {
    const uint32_t bp = Alloc(0x40), data = Alloc(0x120);
    Put<uint32_t>(bp + 8, Text(name));
    Put<uint32_t>(bp + 0x20, data);
    Put<float>(data + 0xF4, 0.025f);   // MeshRenderOffset (0 0.025 0)
    Put<float>(data + 0x108, 88.0f);   // MeshRenderRotation (0 0 88)
    return bp;
}

uint32_t MakeWeapon(uint32_t blueprint, int loaded, int spare, int word) {
    const uint32_t weapon = Alloc(0x140), data = Alloc(0x140), firing = Alloc(0x100);
    Put<uint32_t>(weapon + 0x14, data);
    Put<uint32_t>(data + 0x13C, blueprint);
    Put<uint32_t>(weapon + 0x128, firing);
    Put<uint32_t>(firing, kFiringVt);
    Put<int>(firing + 0xA4, word), Put<int>(firing + 0xA8, word), Put<int>(firing + 0xAC, word);
    Put<int>(firing + 0xF4, loaded), Put<int>(firing + 0xF8, spare);
    return weapon;
}
uint32_t Weapon(int slot) {
    return Get<uint32_t>(g_weapons + 0x40 + 4 * slot);
}
uint32_t Firing(int slot) {
    const uint32_t w = Weapon(slot);
    return w ? Get<uint32_t>(w + 0x128) : 0;
}
uint32_t BlueprintIn(int slot) {
    const uint32_t w = Weapon(slot);
    return w ? Get<uint32_t>(Get<uint32_t>(w + 0x14) + 0x13C) : 0;
}
int Loaded(int slot) {
    return Firing(slot) ? Get<int>(Firing(slot) + 0xF4) : -99;
}
void SetSlot(int slot, uint32_t weapon) {
    Put<uint32_t>(g_weapons + 0x40 + 4 * slot, weapon);
}

uint32_t MakeTransform(const float* at) {
    const uint32_t x = Alloc(64);
    const float m[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, at[0], at[1], at[2], 1};
    std::memcpy(reinterpret_cast<void*>(static_cast<uintptr_t>(x)), m, sizeof(m));
    return x;
}

void SetContent(uint32_t entity, const std::vector<Entry>& entries, uint32_t beginAt = 0x9C, uint32_t endAt = 0xA0) {
    const uint32_t list = Alloc(0xC0 * (entries.size() + 1));
    for (size_t i = 0; i < entries.size(); ++i) {
        const uint32_t q = list + 0xC0 * static_cast<uint32_t>(i);
        Put<uint32_t>(q, entries[i].blueprint);
        Put<int>(q + 0xA8, entries[i].slot), Put<int>(q + 0xAC, -1), Put<int>(q + 0xB0, entries[i].linked);
        Put<int>(q + 0xB4, entries[i].clip), Put<int>(q + 0xB8, entries[i].spare);
    }
    Put<uint32_t>(entity + beginAt, list);
    Put<uint32_t>(entity + endAt, list + 0xC0 * static_cast<uint32_t>(entries.size()));
}

struct PickupSpec {
    float at[3];
    std::vector<Entry> content;
    float ttl = 180.0f, left = 170.0f;
    bool placed = false, unspawn = false, contentStatic = false, replaceAll = false, force = false;
};
uint32_t MakePickup(const PickupSpec& s) {
    const uint32_t e = Alloc(0x140), data = Alloc(0xC0);
    Put<uint32_t>(e, kPickupVt);
    Put<uint32_t>(e + 0x14, data);
    Put<uint32_t>(e + 0x1C, MakeTransform(s.at));
    Put<float>(e + 0x10C, s.left);
    Put<uint32_t>(data, s.placed ? kPlacedDataVt : kDropDataVt);
    Put<int>(data + 0x84, -1);
    Put<float>(data + 0x88, s.ttl);
    Put<float>(data + 0x90, 2.5f);
    Put<uint8_t>(data + 0x98, s.unspawn), Put<uint8_t>(data + 0x9A, s.force), Put<uint8_t>(data + 0x9F, s.contentStatic),
        Put<uint8_t>(data + 0xA2, s.replaceAll);
    SetContent(e, s.content);
    const int active = Get<int>(g_segment + 0x1C);
    Put<uint32_t>(g_entities + 4 * active, e);
    Put<int>(g_segment + 0x1C, active + 1);
    Put<uint32_t>(g_segment + 4, g_entities + 4 * (active + 1));
    return e;
}
void RemovePickup(uint32_t entity) {
    const int active = Get<int>(g_segment + 0x1C);
    for (int i = 0; i < active; ++i)
        if (Get<uint32_t>(g_entities + 4 * i) == entity) {
            for (int j = i; j + 1 < active; ++j)
                Put<uint32_t>(g_entities + 4 * j, Get<uint32_t>(g_entities + 4 * (j + 1)));
            Put<int>(g_segment + 0x1C, active - 1);
            Put<uint32_t>(g_segment + 4, g_entities + 4 * (active - 1));
            return;
        }
}
void ClearPickups() {
    Put<int>(g_segment + 0x1C, 0);
    Put<uint32_t>(g_segment + 4, g_entities);
}
bool InWorldNow(uint32_t entity) {
    const int active = Get<int>(g_segment + 0x1C);
    for (int i = 0; i < active; ++i)
        if (Get<uint32_t>(g_entities + 4 * i) == entity)
            return true;
    return false;
}
float Left(uint32_t entity) {
    return Get<float>(entity + 0x10C);
}
void SetTarget(uint32_t entity) {
    Put<uint32_t>(g_token, entity ? entity + 4 : 0);
}
void SetFlags(uint8_t f) {
    Put<uint8_t>(g_comp + 0x3C, f);
}
uint8_t Flags() {
    return Get<uint8_t>(g_comp + 0x3C);
}

// ---- The game's functions ----

// 008AB870's swap: each content slot's gun into the pickup (unless UnspawnOnPickup, ContentIsStatic or ReplaceAllContent), each entry given into
// its slot as a new object (a null one empties it), the countdown back to its time, the pickup removed when it unspawns or nothing is left.
void __fastcall FakeTake(void* pickupP, void*, void* soldierP) {
    ++g_takes;
    if (g_takeMode == kFault) {
        volatile int* nowhere = nullptr;
        *nowhere = 1;
    }
    const uint32_t e = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pickupP));
    (void)soldierP;
    for (int& c : g_carried)
        c = -99;
    const uint32_t data = Get<uint32_t>(e + 0x14);
    const bool unspawn = Get<uint8_t>(data + 0x98), keep = Get<uint8_t>(data + 0x9F), all = Get<uint8_t>(data + 0xA2);
    const uint32_t begin = Get<uint32_t>(e + 0x9C), end = Get<uint32_t>(e + 0xA0);
    std::vector<Entry> taken, back;
    for (uint32_t q = begin; q < end; q += 0xC0)
        taken.push_back({Get<uint32_t>(q), Get<int>(q + 0xA8), Get<int>(q + 0xB0), Get<int>(q + 0xB4), Get<int>(q + 0xB8)});
    for (const Entry& t : taken)
        if (t.slot >= 0 && t.slot < 10 && Weapon(t.slot))
            g_carried[t.slot] = Loaded(t.slot);
    if (g_takeMode == kNothing)
        return;
    if (g_takeMode == kCountsOnly) {
        Put<int>(Firing(0) + 0xF4, Loaded(0) + 1);
        return;
    }
    if (g_takeMode == kIntoNext) {  // each entry's gun into the slot after the one it names, that one left as it was, the pickup gone
        for (const Entry& t : taken)
            if (t.blueprint && t.slot >= 0 && t.slot + 1 < 10)
                SetSlot(t.slot + 1, MakeWeapon(t.blueprint, t.clip >= 0 ? t.clip : 30, t.spare >= 0 ? t.spare : 90, 2));
        RemovePickup(e);
        return;
    }
    for (const Entry& t : taken)
        if (t.slot >= 0 && t.slot < 10 && Weapon(t.slot)) {
            if (!unspawn && !keep && !all)
                back.push_back({BlueprintIn(t.slot), t.slot, -1, Loaded(t.slot), Get<int>(Firing(t.slot) + 0xF8)});
        }
    for (const Entry& t : taken)
        if (t.slot >= 0 && t.slot < 10)
            SetSlot(t.slot, t.blueprint ? MakeWeapon(t.blueprint, t.clip >= 0 ? t.clip : 30, t.spare >= 0 ? t.spare : 90, 2) : 0);
    if (!keep)
        SetContent(e, back);
    Put<float>(e + 0x10C, Get<float>(data + 0x88));
    if (unspawn || back.empty())
        RemovePickup(e);
}

bool __fastcall FakeAllow(void* pickupP, void*) {
    ++g_allows;
    if (g_allowFault) {
        volatile int* nowhere = nullptr;
        return *nowhere != 0;
    }
    const uint32_t e = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pickupP));
    for (uint32_t n : g_notAllowed)
        if (n == e)
            return false;
    return true;
}
bool __fastcall FakeCheck(void*, void*, void*) {
    ++g_checkCalls;
    return g_check;
}
bool __fastcall FakeGate0(void*, void*) {
    ++g_gateCalls;
    return g_gateUp[0];
}
bool __fastcall FakeGate1(void*, void*) {
    ++g_gateCalls;
    return g_gateUp[1];
}
bool __fastcall FakeGate2(void*, void*) {
    ++g_gateCalls;
    return g_gateUp[2];
}
// The interaction update: flag 4 cleared with R not held; the game's own take with R held (flag 4 set, then the take); the target changed inside
// first when asked.
void __fastcall FakeUpdate(void* comp, void*, void*) {
    ++g_updates;
    if (comp != reinterpret_cast<void*>(static_cast<uintptr_t>(g_comp)))
        return;
    if (g_retargetTo) {
        SetTarget(g_retargetTo);
        g_retargetTo = 0;
    }
    if (!g_gameTakes && !g_rHeld)
        SetFlags(Flags() & ~4);  // (cleared at the next update with R not held, as the game does)
    if (g_gameTakes) {
        g_gameTakes = false;
        const uint32_t token = Get<uint32_t>(g_token);
        if (token) {
            SetFlags(Flags() | 4);
            FakeTake(reinterpret_cast<void*>(static_cast<uintptr_t>(token - 4)), nullptr, reinterpret_cast<void*>(static_cast<uintptr_t>(g_soldier)));
            if (!InWorldNow(token - 4))
                SetTarget(0);
        }
    }
}

int g_hooks = 0, g_enables = 0;

// ---- The DLL's other modules, as pickups.cpp calls them ----
config::Settings g_settings;
loose::Frame g_frame;
bool g_haveFrame = false;
int g_handOver[10];
std::vector<int> g_handOverAsked;
std::vector<pickups::Swap> g_told;

}  // namespace

extern "C" {
MH_STATUS WINAPI MH_CreateHook(LPVOID, LPVOID, LPVOID* original) {
    ++g_hooks;
    *original = reinterpret_cast<LPVOID>(&FakeUpdate);
    return MH_OK;
}
MH_STATUS WINAPI MH_EnableHook(LPVOID) {
    ++g_enables;
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
    char line[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);
    g_log.push_back(line);
}
}  // namespace logx
namespace config {
const Settings& Get() {
    return g_settings;
}
}  // namespace config
namespace god {
uintptr_t PlayerServerSoldier() {
    return g_soldier;
}
uintptr_t PlayerClientSoldier() {
    return g_client;
}
}  // namespace god
namespace hands {
const loose::Frame* LooseFrame() {
    return g_haveFrame ? &g_frame : nullptr;
}
}  // namespace hands
namespace ammo {
bool HandOverCount(int slot, uint32_t& firing, int& loaded) {
    g_handOverAsked.push_back(slot);
    if (slot < 0 || slot >= 10 || g_handOver[slot] < 0 || !Firing(slot))
        return false;
    firing = Firing(slot);
    loaded = g_handOver[slot];
    return true;
}
void Swapped(const pickups::Swap& swap) {
    g_told.push_back(swap);
}
}  // namespace ammo

namespace {

// The test's own checks of the game's code (its fake code, a call site, a float, a class id), as Verify goes by them.
uint8_t* g_fakeCode = nullptr;
uint32_t g_fakeCall = 0, g_fakeFloat = 0, g_fakeClass = 0;
pickups::Code g_tCode[1];
uint8_t g_tCallBytes[8] = {0x53, 0x8B, 0xCE, 0xE8, 0x9D, 0xF6, 0xFF, 0xFF};
pickups::Bytes g_tBytes[1];
pickups::Word g_tWords[2];

std::wstring g_dir;
int g_fileStamp = 0;

void WriteFileText(const char* text) {
    const std::wstring path = g_dir + L"pickups.txt";
    FILE* f = nullptr;
    _wfopen_s(&f, path.c_str(), L"wb");
    if (f) {
        fputs(text, f);
        fclose(f);
    }
    // A time of its own each write (the reader goes by it).
    HANDLE h = CreateFileW(path.c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h != INVALID_HANDLE_VALUE) {
        ULARGE_INTEGER t;
        t.QuadPart = 133000000000000000ull + 10000000ull * static_cast<unsigned long long>(++g_fileStamp);
        FILETIME ft{t.LowPart, t.HighPart};
        SetFileTime(h, nullptr, nullptr, &ft);
        CloseHandle(h);
    }
}
void DeleteTheFile() {
    DeleteFileW((g_dir + L"pickups.txt").c_str());
}

// Everything pickups.cpp keeps, as at a launch.
void ResetPickups() {
    using namespace pickups;
    g_tickLock.seq.store(0), g_tickLock.value = Tick{};
    g_censusLock.seq.store(0), g_censusLock.value = Census{};
    g_ringLock.seq.store(0), g_ringLock.value = Ring{};
    g_eyeLock.seq.store(0), g_eyeLock.value = Eye{};
    g_live = false, g_dead = false, g_takesDead = false;
    g_player = 0, g_playerData = 0, g_level = 2, g_seconds = 0, g_ownSeconds = -1, g_logLevel = 0;
    g_unseen = false, g_takeWanted = false, g_reachingAt = 0, g_gameThread = 0;
    g_faults = 0, g_faultCode = 0, g_restUntil = 0, g_takeFaultCode = 0, g_badGateTable = 0, g_badGateFunction = 0, g_raised = 0;
    for (auto& h : g_heldEntity)
        h = 0;
    g_heldCount = 0;
    g_ask = Ask{};
    g_askPosted = 0;
    g_useAsk = UseAsk{};
    g_useAskPosted = 0, g_useState = 0, g_useDead = false;
    g_useServedSerial = 0;
    g_useDoneSerial = 0, g_useDoneResult = 0, g_useDoneWhy = "", g_useFaultCode = 0;
    g_useChecks = {kUseCode, static_cast<int>(std::size(kUseCode)), kUseBytes, static_cast<int>(std::size(kUseBytes)), kUseWords,
                   static_cast<int>(std::size(kUseWords))};
    g_inside = false;
    g_original = nullptr;
    g_servedSerial = 0, g_servedResult = 0, g_servedWhy[0] = 0;
    g_lastSoldier = 0, g_lastWorld = 0, g_swapSerial = 0, g_censusSerial = 0;
    g_walkAt = 0, g_lookGatesAt = 0, g_policyAt = 0, g_microsFrom = 0;
    g_gatesUp = true;
    g_microsMost = 0.0f;
    g_tickScratch = Tick{}, g_censusScratch = Census{}, g_ringScratch = Ring{};
    g_walkedCount = 0, g_walkedTotal = 0, g_walkedThisTick = false;
    for (auto& n : g_names)
        n = NameOf{};
    g_nameNext = 0;
    for (auto& k : g_known)
        k = Known{};
    for (auto& o : g_own)
        o = Own{};
    g_file = FileValues{}, g_fileRead = false, g_fileWrite = FILETIME{}, g_fileCheck = 0, g_takePending = false, g_takeAskedAt = 0;
    g_verified = false, pickups::g_failed = false, g_hooked = false;
    g_levelNow = -1, g_secondsNow = 0, g_ownNow = -1, g_unseenNow = false, g_policySaid = false, g_levelFrom = "";
    g_tick = Tick{}, g_census = Census{}, g_ring = Ring{};
    g_haveTick = false, g_haveCensus = false, g_swapsSaid = 0, g_servedSaid = 0, g_firstTickSaid = false, g_faultsSaid = 0;
    g_takeFaultSaid = false, g_badGateSaid = false, g_raisedSaid = 0, g_heldSaid = 0, g_lastLog = 0, g_lastRaiseSay = 0, g_lastNames = 0;
    g_frameSoldier = 0, g_frameWorld = 0;
    g_takeOpen = false, g_askSerial = 0, g_waiting = 0, g_waitingAt = 0;
    for (auto& o : g_outcomes)
        o = Answer{};
    g_outcomeNext = 0;
    for (auto& b : g_boxes)
        b = Box{};
    g_lastBoxLook = 0;
    std::memset(g_saidOwn, 0, sizeof(g_saidOwn));
    std::memset(g_saidHeld, 0, sizeof(g_saidHeld));
    std::memset(g_saidNoPlace, 0, sizeof(g_saidNoPlace));
    std::memset(g_saidNoRow, 0, sizeof(g_saidNoRow));
}

// The code range of this test (the gates' functions must lie in it).
void OwnCode(uintptr_t& from, uintptr_t& to) {
    const uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    const IMAGE_SECTION_HEADER* s = IMAGE_FIRST_SECTION(nt);
    for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i)
        if (std::memcmp(s[i].Name, ".text", 5) == 0) {
            from = base + s[i].VirtualAddress;
            to = from + s[i].Misc.VirtualSize;
        }
}

// A fresh scene: the worlds, the player with the M416 (slot 0) and its M320 (slot 2), the component, the checks of the code, every switch at its
// default; nothing yet lying.
void Scene() {
    g_used = 0;
    ResetPickups();
    g_log.clear();
    g_logMark = 0;
    g_settings = config::Settings{};
    g_settings.pickups = 2;  // (on, as the rows need it: the ini's default is 0 now, and R2 tests 0 by itself)
    g_takeMode = kSwap;
    g_takes = g_updates = g_allows = g_checkCalls = g_gateCalls = 0;
    g_allowFault = false, g_check = true, g_gateUp[0] = g_gateUp[1] = g_gateUp[2] = false;
    g_notAllowed.clear();
    g_gameTakes = false, g_rHeld = false, g_retargetTo = 0;
    g_hooks = g_enables = 0;
    g_haveFrame = false;
    for (int& h : g_handOver)
        h = -1;
    g_handOverAsked.clear();
    g_told.clear();
    DeleteTheFile();

    g_m416 = {MakeBlueprint("Weapons/M416/M416"), "M416"};
    g_m320 = {MakeBlueprint("Weapons/Gadgets/M320/M320"), "M320"};
    g_an94 = {MakeBlueprint("Weapons/AN94/AN94"), "AN-94"};
    g_rpk = {MakeBlueprint("Weapons/RPK/RPK"), "RPK-74M"};
    g_stinger = {MakeBlueprint("Weapons/FIM92A/FIM92A_SP_Valley"), "Stinger"};
    g_odd = {MakeBlueprint("Weapons/Odd/Oddgun"), "Oddgun"};

    // The server world: its pickups' segment.
    g_worldCell = Alloc(4), g_world = Alloc(0x900), g_type = Alloc(0x20);
    Put<uint32_t>(g_worldCell, g_world);
    Put<uint32_t>(g_world, kServerWorldVt);
    Put<uint16_t>(g_world + 0x834, kFirstClass);
    Put<uint16_t>(g_type + 0x14, kPickupClass);
    g_lists = Alloc(8 * 0x200);
    Put<uint32_t>(g_world + 0x820, g_lists), Put<uint32_t>(g_world + 0x824, g_lists + 8 * 0x200);
    g_segment = Alloc(0x40), g_entities = Alloc(4 * 400);
    Put<uint32_t>(g_lists + 8 * (kPickupClass - kFirstClass), g_segment);
    Put<uint32_t>(g_segment, g_entities), Put<uint32_t>(g_segment + 4, g_entities);
    // The client world, empty.
    g_clientWorldCell = Alloc(4), g_clientWorld = Alloc(0x900), g_clientType = Alloc(0x20);
    Put<uint32_t>(g_clientWorldCell, g_clientWorld);
    Put<uint32_t>(g_clientWorld, kClientWorldVt);
    Put<uint16_t>(g_clientWorld + 0x834, kFirstClass);
    Put<uint16_t>(g_clientType + 0x14, kClientClass);
    g_clientLists = Alloc(8 * 0x200);
    Put<uint32_t>(g_clientWorld + 0x820, g_clientLists), Put<uint32_t>(g_clientWorld + 0x824, g_clientLists + 8 * 0x200);
    g_clientSegment = Alloc(0x40), g_clientEntities = Alloc(4 * 16);
    Put<uint32_t>(g_clientLists + 8 * (kClientClass - kFirstClass), g_clientSegment);
    Put<uint32_t>(g_clientSegment, g_clientEntities), Put<uint32_t>(g_clientSegment + 4, g_clientEntities);

    // The player.
    g_soldier = Alloc(0x600), g_soldierData = Alloc(0x40), g_client = Alloc(0x40), g_weapons = Alloc(0x80);
    Put<uint32_t>(g_soldier, kSoldierVt);
    Put<uint8_t>(g_soldier + 0xA, 1);
    Put<uint32_t>(g_soldier + 0x14, g_soldierData);
    Put<uint32_t>(g_client + 0x14, g_soldierData);
    g_soldierXform = MakeTransform(g_soldierAt);
    Put<uint32_t>(g_soldier + 0x1C, g_soldierXform);
    Put<uint32_t>(g_soldier + 0x4EC, g_weapons);
    SetSlot(0, MakeWeapon(g_m416.at, 31, 120, 2));
    SetSlot(2, MakeWeapon(g_m320.at, 1, 3, 3));
    // Its three gates.
    for (int i = 0; i < 3; ++i)
        g_gateTable[i] = Alloc(0x20);
    Put<uint32_t>(g_soldier + 0x104, g_gateTable[0]);
    Put<uint32_t>(g_gateTable[0] + 0x10, Fn(&FakeGate0));
    Put<uint32_t>(g_soldier + 0x120, g_gateTable[1]);
    Put<uint32_t>(g_gateTable[1] + 0x0C, Fn(&FakeGate1));
    Put<uint32_t>(g_gateTable[1] + 0x10, Fn(&FakeGate2));
    // Its interaction component.
    g_comp = Alloc(0x60), g_token = Alloc(8);
    Put<uint32_t>(g_comp, kComponentVt);
    Put<uint32_t>(g_comp + 0x10, g_soldier);
    Put<uint32_t>(g_comp + 0x1C, g_token);
    Put<uint32_t>(g_token, 0);

    // The image's code the checks go by: the test's own.
    g_fakeCode = reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(Alloc(256)));
    for (int i = 0; i < 256; ++i)
        g_fakeCode[i] = static_cast<uint8_t>(i * 7 + 3);
    g_tCode[0] = {reinterpret_cast<uintptr_t>(g_fakeCode), 256, pickups::Fnv(g_fakeCode, 256), "its interaction update"};
    g_fakeCall = Alloc(16);
    std::memcpy(reinterpret_cast<void*>(static_cast<uintptr_t>(g_fakeCall)), g_tCallBytes, sizeof(g_tCallBytes));
    g_tBytes[0] = {g_fakeCall, g_tCallBytes, sizeof(g_tCallBytes), "the call of its interaction update"};
    g_fakeFloat = Alloc(4), g_fakeClass = Alloc(4);
    Put<float>(g_fakeFloat, 0.4f);
    Put<uint16_t>(g_fakeClass, kPickupClass);
    g_tWords[0] = {g_fakeFloat, 0x3ECCCCCD, 4, "the hold (0.4 s)"};
    g_tWords[1] = {g_fakeClass, kPickupClass, 2, "the server pickup's class id"};
    pickups::g_checks = {g_tCode, 1, g_tBytes, 1, g_tWords, 2};
    g_updateStart = Alloc(16);
    std::memcpy(reinterpret_cast<void*>(static_cast<uintptr_t>(g_updateStart)), pickups::kUpdateStart, sizeof(pickups::kUpdateStart));

    pickups::Addresses& a = pickups::g_at;
    a = pickups::Addresses{};
    a.serverWorld = g_worldCell, a.serverWorldVt = kServerWorldVt, a.clientWorld = g_clientWorldCell, a.clientWorldVt = kClientWorldVt;
    a.pickupVt = kPickupVt, a.clientPickupVt = kClientPickupVt, a.componentVt = kComponentVt, a.soldierVt = kSoldierVt, a.firingVt = kFiringVt;
    a.dropDataVt = kDropDataVt, a.placedDataVt = kPlacedDataVt, a.pickupType = g_type, a.clientPickupType = g_clientType;
    a.update = g_updateStart, a.take = Fn(&FakeTake), a.allow = Fn(&FakeAllow), a.check = Fn(&FakeCheck);
    OwnCode(a.codeFrom, a.codeTo);
    a.rdataFrom = reinterpret_cast<uintptr_t>(g_arena), a.rdataTo = reinterpret_cast<uintptr_t>(g_arena) + kArena;
}

// One of the game's ticks of the player's component (the detour called as the game would), then one render frame.
void Hook(uint32_t comp = 0) {
    void* c = reinterpret_cast<void*>(static_cast<uintptr_t>(comp ? comp : g_comp));
    if (pickups::g_hooked)
        pickups::InteractionHook(c, nullptr, nullptr);
    else
        FakeUpdate(c, nullptr, nullptr);  // (not hooked: the game's own update alone)
}
void Step(int frames = 1, ULONGLONG ms = 14) {
    for (int i = 0; i < frames; ++i) {
        g_now += ms;
        Hook();
        pickups::OnFrame();
    }
}
// Frames with no tick of the game's.
void FramesOnly(int frames, ULONGLONG ms = 14) {
    for (int i = 0; i < frames; ++i) {
        g_now += ms;
        pickups::OnFrame();
    }
}
// Up and running: verified, hooked, a hand reaching (the walk every tick).
void Up() {
    pickups::OnFrame();
    Step(2);
    pickups::SetReaching(true);
    Step(2);
}
void Reach(int frames = 1) {
    for (int i = 0; i < frames; ++i) {
        pickups::SetReaching(true);
        Step();
    }
}

PickupSpec Drop(float dx, const Blueprint& gun, float dz = 0.0f) {
    PickupSpec s;
    s.at[0] = g_soldierAt[0] + dx, s.at[1] = g_soldierAt[1], s.at[2] = g_soldierAt[2] + dz;
    s.content = {{gun.at, 0, -1, 30, 90}, {0, 2, 0, -1, -1}};
    return s;
}

const pickups::Pickup* InCensus(uint32_t entity) {
    static pickups::Census c;
    if (!pickups::Near(c))
        return nullptr;
    for (int i = 0; i < c.count; ++i)
        if (c.pickup[i].entity == entity)
            return &c.pickup[i];
    return nullptr;
}

// A count set by the test, not the game's countdown: its rate starts again from the next look (1 a second till then).
void Rebase(uint32_t entity) {
    for (auto& k : pickups::g_known)
        if (k.entity == entity)
            k.leftAt = 0, k.rate = 1.0f;
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------

void R1() {
    Scene();
    Put<uint32_t>(g_worldCell, 0);  // no level yet
    Step(5);
    Check(Said("Pickups: the game's code") == 0 && g_hooks == 0, "R1", "no level up: nothing looked at, nothing said, nothing hooked");
    Put<uint32_t>(g_worldCell, g_world);
    Mark();
    Step(3);
    Check(Said("Pickups: the game's code is as read (its interaction update, its take, its checks of a pickup, AllowPickup, the pickup's countdown, "
               "the world's lists, the client's pose of a dropped gun)") == 1,
          "R1", "the code as read: its line, once");
    Check(Said("Pickups: hooked its interaction update") == 1 && g_hooks == 1 && g_enables == 1 && pickups::Hooked(), "R1",
          "hooked right after, once");

    struct Bad {
        const char* what;
        uint32_t at;
        const char* words;
    };
    const Bad bad[] = {{"a byte of a function", 0, "its interaction update (256 bytes at"},
                       {"a byte of a call site", 1, "the call of its interaction update (8 bytes at"},
                       {"a float", 2, "the hold (0.4 s) (4 bytes at"}};
    for (const Bad& b : bad) {
        Scene();
        if (b.at == 0)
            g_fakeCode[100] ^= 1;
        else if (b.at == 1)
            Put<uint8_t>(g_fakeCall + 4, 0x9E);
        else
            Put<float>(g_fakeFloat, 0.5f);
        Step(5);
        char what[200];
        snprintf(what, sizeof(what), "%s changed: \"isn't as read\" said once, naming it; nothing hooked; never live", b.what);
        Check(Said("Pickups: the game's code isn't as read: ") == 1 && Said(b.words) == 1 &&
                  Said(": left off for this session (X held still asks the game's own pickup)") == 1 && g_hooks == 0 && !pickups::Hooked() &&
                  pickups::GameWouldTake() == -1 && g_updates == 5,
              "R1", what);
    }

    // The real tables, against the image they were read from.
    wchar_t self[MAX_PATH];
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    std::wstring image = self;
    image = image.substr(0, image.find_last_of(L'\\') + 1) + L"bf3_mem.exe";
    FILE* f = nullptr;
    _wfopen_s(&f, image.c_str(), L"rb");
    if (!f) {
        std::printf("SKIP R1 the real tables against bf3_mem.exe: not beside the test\n");
        return;
    }
    fseek(f, 0, SEEK_END);
    const long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> img(static_cast<size_t>(size));
    fread(img.data(), 1, img.size(), f);
    fclose(f);
    auto at = [&](uintptr_t va) -> const uint8_t* { return va >= 0x400000 && va - 0x400000 < img.size() ? img.data() + (va - 0x400000) : nullptr; };
    int codes = 0, bytes = 0, words = 0;
    for (const pickups::Code& c : pickups::kCode)
        if (const uint8_t* p = at(c.at); p && c.at - 0x400000 + c.size <= img.size() && pickups::Fnv(p, c.size) == c.fnv)
            ++codes;
        else
            std::printf("      not as read in bf3_mem.exe: %s\n", c.what);
    for (const pickups::Bytes& b : pickups::kBytes)
        if (const uint8_t* p = at(b.at); p && std::memcmp(p, b.bytes, b.size) == 0)
            ++bytes;
        else
            std::printf("      not as read in bf3_mem.exe: %s\n", b.what);
    for (const pickups::Word& w : pickups::kWords) {
        uint32_t v = 0;
        if (const uint8_t* p = at(w.at)) {
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
             static_cast<int>(std::size(pickups::kCode)), bytes, static_cast<int>(std::size(pickups::kBytes)), words, static_cast<int>(std::size(pickups::kWords)));
    Check(codes == static_cast<int>(std::size(pickups::kCode)) && bytes == static_cast<int>(std::size(pickups::kBytes)) &&
              words == static_cast<int>(std::size(pickups::kWords)),
          "R1", what);
}

void R2() {
    Scene();
    pickups::OnFrame();
    Check(g_hooks == 1, "R2", "nothing wanted yet (no gun lying, no test line): hooked anyway, right after the checks: a swap can come before any gun has lain");
    Step(3);
    pickups::SetReaching(true);
    Step(3);
    Check(g_hooks == 1, "R2", "SetReaching(true) later: still the one hook");

    Scene();
    WriteFileText("off\n");
    Step(5);
    pickups::SetReaching(true);
    Step(5);
    Check(Said("Pickups: off (pickups.txt): nothing of the game's hooked or called; X held asks the game's own pickup as before") == 1 && g_hooks == 0 &&
              Said("Pickups: the game's code") == 0 && g_takes == 0 && g_allows == 0 && pickups::GameWouldTake() == -1,
          "R2", "pickups.txt \"off\": nothing looked at, hooked or called, said once; X held as before (-1)");
    // "off" in a file that can't be read yet at the first frames (its writer holds it): nothing looked at or hooked till it's read.
    Scene();
    WriteFileText("off\n");
    {
        const HANDLE held = CreateFileW((g_dir + L"pickups.txt").c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        Step(5);
        const bool none = g_hooks == 0 && Said("Pickups: the game's code") == 0 && Said("Pickups: off (pickups.txt)") == 0;  // (not read yet)
        if (held != INVALID_HANDLE_VALUE)
            CloseHandle(held);
        g_now += 400;
        Step(3);
        Check(held != INVALID_HANDLE_VALUE && none && g_hooks == 0 && Said("Pickups: the game's code") == 0 &&
                  Said("Pickups: off (pickups.txt)") == 1,
              "R2", "pickups.txt \"off\" not readable at the first frames: nothing looked at or hooked meanwhile, then off");
    }
    DeleteTheFile();
    Scene();
    g_settings.pickups = 0;
    Step(5);
    Check(Said("Pickups: off (bf3vr.ini pickups=0)") == 1 && g_hooks == 0, "R2", "bf3vr.ini pickups=0: the same, said so");

    // Once in, "off" makes the hook pass straight through.
    Scene();
    Up();
    const uint32_t serial = pickups::g_tickScratch.serial;
    WriteFileText("off\n");
    g_now += 400;
    pickups::OnFrame();
    const int allows = g_allows, updates = g_updates;
    Step(10);
    Check(pickups::g_tickScratch.serial == serial, "R2", "off once hooked: no tick published");
    Check(g_updates == updates + 10 && g_allows == allows && !pickups::g_live, "R2", "off once hooked: the game's own update still runs, nothing of ours");
    DeleteTheFile();
}

void R3() {
    Scene();
    Up();
    const uint32_t serial = pickups::g_tickScratch.serial;
    // Another soldier's component.
    const uint32_t other = Alloc(0x600), otherComp = Alloc(0x60);
    Put<uint32_t>(other, kSoldierVt);
    Put<uint32_t>(other + 0x14, Alloc(0x10));
    Put<uint32_t>(otherComp + 0x10, other);
    const int updates = g_updates;
    for (int i = 0; i < 5; ++i)
        Hook(otherComp);
    Check(g_updates == updates + 5 && pickups::g_tickScratch.serial == serial, "R3", "another soldier: the game's update, nothing published");
    // The player's address with another data pointer (a new soldier made there).
    Put<uint32_t>(g_soldier + 0x14, Alloc(0x10));
    for (int i = 0; i < 5; ++i)
        Hook();
    Check(g_updates == updates + 10 && pickups::g_tickScratch.serial == serial, "R3", "the same address, another data pointer: straight through");
    Put<uint32_t>(g_soldier + 0x14, g_soldierData);
    Hook();
    Check(pickups::g_tickScratch.serial == serial + 1, "R3", "the player's again: published");
}

void R4() {
    Scene();
    Up();
    pickups::Tick t;
    bool ok = pickups::Now(t);
    Check(ok && t.slots[0].weapon == Weapon(0) && t.slots[0].firing == Firing(0) && t.slots[0].blueprint == g_m416.at &&
              std::strcmp(t.slots[0].name, "Weapons/M416/M416") == 0 && t.slots[0].loaded == 31 && t.slots[0].spare == 120 && t.slots[0].stateA8 == 2 &&
              t.slots[2].blueprint == g_m320.at && t.slots[2].stateA8 == 3 && !t.slots[1].weapon && t.soldier == g_soldier,
          "R4", "the tick: the slots' triples, names, counts and words");
    Check(ok && t.activeSlot == 0 && std::fabs(t.soldierAt[0] - 1260.0f) < 1e-3f, "R4", "the tick: slot 0 in hand by its words; the soldier's place");
    const uint32_t e = MakePickup(Drop(1.0f, g_an94));
    SetTarget(e);
    Step();
    pickups::Now(t);
    Check(t.gameTarget == e && t.gameTargetBlueprint == g_an94.at && std::strcmp(t.gameTargetName, "Weapons/AN94/AN94") == 0 && pickups::GameWouldTake() == 1,
          "R4", "the game's target with flag 4 clear: the AN-94's pickup, GameWouldTake 1");
    SetFlags(4);
    g_rHeld = true;  // (R held: the game keeps flag 4 set)
    Step();
    pickups::Now(t);
    Check(t.gameTarget == 0 && pickups::GameWouldTake() == 0, "R4", "flag 4 set: no target, GameWouldTake 0");
    SetFlags(0);
    g_rHeld = false;
    const uint32_t before = t.serial;
    Step(7);
    pickups::Now(t);
    Check(t.serial == before + 7, "R4", "published every tick");
}

void R5() {
    Scene();
    pickups::OnFrame();
    Step(80);  // over a second, nothing wanting the walk
    pickups::Census c;
    const uint32_t e = MakePickup(Drop(1.0f, g_an94));
    Step(80);
    Check(!pickups::Near(c) && g_allows == 0, "R5", "nothing wanting it: no walk, no census, AllowPickup never called");
    Reach();
    Check(pickups::Near(c) && c.count == 1 && c.pickup[0].entity == e, "R5", "a hand reaching: walked that tick");
    // The nearest kMost of 40, nearest first.
    for (int i = 0; i < 39; ++i)
        MakePickup(Drop(2.0f + 0.5f * static_cast<float>(i), g_rpk, 0.1f));
    Reach();
    pickups::Near(c);
    bool sorted = c.count == pickups::kMost && c.total == 40;
    for (int i = 1; i < c.count; ++i)
        sorted = sorted && c.pickup[i - 1].distance <= c.pickup[i].distance;
    Check(sorted && c.pickup[0].entity == e, "R5", "40 lying: the nearest 32 kept, nearest first, 40 walked");
    ClearPickups();
    PickupSpec nullFirst = Drop(1.5f, g_an94);
    nullFirst.content = {{0, 2, 0, -1, -1}, {g_an94.at, 0, -1, 30, 90}};
    const uint32_t n = MakePickup(nullFirst);
    PickupSpec banned = Drop(1.2f, g_rpk);
    const uint32_t b = MakePickup(banned);
    g_notAllowed = {b};
    PickupSpec crate = Drop(2.0f, g_stinger);
    crate.content = {{g_stinger.at, 0, -1, 1, 3}};  // a gun for slot 0, which holds the M416
    crate.unspawn = true;
    const uint32_t k = MakePickup(crate);
    PickupSpec stinger = Drop(2.2f, g_stinger);
    stinger.content = {{g_stinger.at, 1, -1, 1, 3}};
    stinger.unspawn = true, stinger.placed = true, stinger.ttl = 0.0f;
    const uint32_t st = MakePickup(stinger);
    const uint32_t distant = MakePickup(Drop(5.0f, g_rpk));
    g_now += 300;
    Reach(2);
    const pickups::Pickup* pn = InCensus(n);
    const pickups::Pickup* pb = InCensus(b);
    const pickups::Pickup* pk = InCensus(k);
    const pickups::Pickup* ps = InCensus(st);
    const pickups::Pickup* pf = InCensus(distant);
    Check(pn && pn->first == 1 && pn->contents == 2 && pn->content[1].blueprint == g_an94.at && std::strcmp(pn->content[1].name, "Weapons/AN94/AN94") == 0 &&
              pn->content[1].slot == 0 && std::fabs(pn->content[1].rotation[2] - 88.0f) < 1e-4f && std::fabs(pn->content[1].offset[1] - 0.025f) < 1e-6f,
          "R5", "first skips a null entry; the entry's name, slot and PickupSettings");
    Check(pn && pn->allowed && pn->eligible && pb && !pb->allowed && pf && !pf->allowed && !pf->eligible, "R5",
          "allowed and eligible within 3 m by the game's own getter and check; not allowed: false; 5 m off: neither");
    Check(pk && pk->keepsNoPlace && ps && !ps->keepsNoPlace && ps->kind == pickups::kPlaced && pk->kind == pickups::kDrop && ps->unspawnOnPickup &&
              ps->timeToLive == 0.0f,
          "R5", "keepsNoPlace: an unspawning one for a slot that holds a gun; the Stinger for the empty slot 1: not; kinds and flags");
    Check(pn && !pn->own, "R5", "own: none before a swap");
    g_gateUp[1] = true;
    g_now += 300;
    Reach(2);
    pn = InCensus(n);
    pickups::Tick t;
    pickups::Now(t);
    const float eye[3] = {g_soldierAt[0], g_soldierAt[1] + 1.6f, g_soldierAt[2]};
    pickups::Reach r[4];
    Check(pn && !pn->eligible && pn->allowed && t.gatesUp && pickups::Reachable(eye, 3.0f, r, 4) == 0, "R5",
          "a gate up: nothing eligible, nothing reachable, gatesUp in the tick");
    g_gateUp[1] = false;
    Put<uint8_t>(g_soldier + 0xA, 0);
    g_now += 300;
    Reach(2);
    pickups::Now(t);
    Check(t.gatesUp, "R5", "the soldier's +0xA bit clear: gates up");
    // A gun the guns' table has no row for: said once.
    Put<uint8_t>(g_soldier + 0xA, 1);
    MakePickup(Drop(1.1f, g_odd));
    Mark();
    Reach(160);
    Check(Said("Pickups: no row for Weapons/Odd/Oddgun in the guns' table: free hands can't tell what it is before it's drawn") == 1 &&
              Said("Pickups: no row for") == 1,
          "R5", "a name with no row in the guns' table: said once in 2 s, and none for the guns it has");
}

// The left takes the AN-94 for the M416 with its M320.
void R6() {
    Scene();
    Up();
    const uint32_t e = MakePickup(Drop(1.0f, g_an94));
    const uint32_t m416Firing = Firing(0), m320Firing = Firing(2), m416Weapon = Weapon(0);
    Reach(2);
    const char* why = "";
    const uint32_t request = pickups::RequestTake(e, g_an94.at, 0, &why);
    Check(request != 0 && pickups::TakeOpen() && pickups::Outcome(request) == 0, "R6", "asked: a serial, TakeOpen up, waiting");
    Check(g_takes == 0, "R6", "nothing taken on the render thread");
    Mark();
    pickups::SetReaching(true);
    Step();
    Check(g_takes == 1 && pickups::Outcome(request) == 1 && !pickups::TakeOpen(), "R6", "served in the tick after: taken, TakeOpen down");
    pickups::Swap s;
    const bool got = pickups::NextSwap(0, s);
    Check(got && s.byHand && s.request == request && s.side == 0 && s.entity == e && !s.removed && std::fabs(s.world[12] - 1261.0f) < 1e-3f &&
              s.taken.blueprint == g_an94.at && std::strcmp(s.taken.name, "Weapons/AN94/AN94") == 0 && s.taken.slot == 0,
          "R6", "the swap: by the left hand, its request, the pickup, where it lay (read before), what was taken");
    Check(got && s.changed == 2 && s.slot[0] == 0 && s.slot[1] == 2 && std::strcmp(s.came[0], "Weapons/AN94/AN94") == 0 && s.came[1][0] == 0 &&
              std::strcmp(s.went[0], "Weapons/M416/M416") == 0 && std::strcmp(s.went[1], "Weapons/Gadgets/M320/M320") == 0 &&
              s.wentFiring[0] == m416Firing && s.wentFiring[1] == m320Firing && s.cameFiring[0] == Firing(0) && s.wentBlueprint[0] == g_m416.at &&
              s.cameBlueprint[0] == g_an94.at && Weapon(0) != m416Weapon,
          "R6", "the slots: 0 the AN-94 for the M416, 2 emptied of the M320; their firing states and blueprints");
    Check(g_told.size() == 1 && g_told[0].serial == s.serial && pickups::NewestSwap() == s.serial, "R6", "ammo told of it once, the frame it was seen");
    Check(Said("Pickups: the game took it (#1, the left hand): the AN-94 into slot 0; the M416 and the M320 into its place, where it lay") == 1, "R6",
          "said as the design has it");
    Check(Said("Pickups: your M416 kept where it lies until you take it (pickup_own_seconds)") == 1, "R6", "the player's own: said once");
    float drawn[12];
    pickups::SwapDrawn(s, drawn);
    Check(std::fabs(drawn[7] - (65.0f + 0.025f)) < 1e-4f && std::fabs(drawn[3] - 1261.0f) < 1e-3f, "R6", "SwapDrawn: where it lay, 2.5 cm up");
    pickups::Swap again;
    Check(!pickups::NextSwap(s.serial, again), "R6", "no swap newer");
    Reach(2);
    const pickups::Pickup* own = InCensus(e);
    Check(own && own->own && own->first >= 0 && own->content[own->first].blueprint == g_m416.at, "R6", "that pickup now holds the M416: your own");
    Check(g_told.size() == 1, "R6", "ammo not told again");
    // An unspawning one: removed.
    PickupSpec stinger = Drop(-1.0f, g_stinger);
    stinger.content = {{g_stinger.at, 1, -1, 1, 3}};
    stinger.unspawn = true, stinger.placed = true, stinger.ttl = 0.0f;
    const uint32_t st = MakePickup(stinger);
    Reach(2);
    const uint32_t r2 = pickups::RequestTake(st, g_stinger.at, 1, &why);
    Reach(2);
    pickups::NextSwap(s.serial, s);
    Check(r2 && pickups::Outcome(r2) == 1 && s.removed && s.side == 1 && s.changed == 1 && s.slot[0] == 1 && s.went[0][0] == 0, "R6",
          "the Stinger taken by the right: into slot 1, nothing went, the pickup removed with it");
}

void R7() {
    struct Case {
        const char* words;
        int setup;
    };
    const Case cases[] = {{"too late", 0},
                          {"not there any more", 1},
                          {"the game doesn't allow it now", 2},
                          {"nothing in it", 3},
                          {"it would take your gun's place for good", 4},
                          {"the game doesn't allow it now", 5},  // 4 m off: nothing is allowed past 3 m, so 3.5 m ("out of reach") is a bound only
                          {"the soldier has no weapons", 6},
                          {"the game's own pickup was under way", 7},
                          {"the game allows no pickup just now", 8},
                          {"it wouldn't let you take it from there now", 9}};
    for (const Case& c : cases) {
        Scene();
        Up();
        PickupSpec spec = Drop(1.0f, g_an94);
        if (c.setup == 4) {
            spec.content = {{g_stinger.at, 1, -1, 1, 3}};
            spec.unspawn = true;
        }
        const uint32_t e = MakePickup(spec);
        g_handOver[0] = 17;
        g_handOver[2] = 0;
        Reach(2);
        const uint32_t blueprint = c.setup == 4 ? g_stinger.at : g_an94.at;
        const char* why = "";
        const uint32_t request = pickups::RequestTake(e, blueprint, 0, &why);
        const int loaded0 = Loaded(0), loaded2 = Loaded(2);
        switch (c.setup) {
            case 0: g_now += 600; break;
            case 1: RemovePickup(e); break;
            case 2: g_notAllowed = {e}, g_now += 250; break;
            case 3: SetContent(e, {{0, 2, 0, -1, -1}}); break;
            case 4: SetSlot(1, MakeWeapon(g_rpk.at, 10, 10, 3)); break;
            case 5: {
                float* x = reinterpret_cast<float*>(static_cast<uintptr_t>(Get<uint32_t>(e + 0x1C)));
                x[12] += 3.0f;  // 4 m off
                break;
            }
            case 6: Put<uint32_t>(g_soldier + 0x4EC, 0); break;
            case 7: SetFlags(4); break;
            case 8: g_gateUp[0] = true; break;
            case 9: g_check = false; break;
        }
        Mark();
        pickups::SetReaching(true);
        g_now += 14;
        Hook();
        pickups::OnFrame();
        const char* got = "";
        const int outcome = pickups::Outcome(request, &got);
        char what[200];
        snprintf(what, sizeof(what), "refused: \"%s\" (said \"%s\"); no count written, the take not called", c.words, got);
        Put<uint32_t>(g_soldier + 0x4EC, g_weapons);
        Check(request && outcome == -1 && std::strcmp(got, c.words) == 0 && g_takes == 0 && Loaded(0) == loaded0 && Loaded(2) == loaded2 &&
                  Said("Pickups: the game didn't take it (#1): ") == 1 && !pickups::TakeOpen(),
              "R7", what);
    }
    // RequestTake's own refusals.
    Scene();
    Up();
    PickupSpec crate = Drop(1.0f, g_stinger);
    crate.content = {{g_stinger.at, 0, -1, 1, 3}};
    crate.unspawn = true;
    const uint32_t k = MakePickup(crate);
    Reach(2);
    const char* why = "";
    Mark();
    Check(pickups::RequestTake(k, g_stinger.at, 0, &why) == 0 && std::strcmp(why, "it would take your gun's place for good") == 0 &&
              Said("Pickups: the Stinger would take your gun's place for good (the game keeps no place for it): hold X to take it the game's way") == 1 &&
              !pickups::TakeOpen(),
          "R7", "RequestTake: a pickup that keeps no place refused at once, said");
    Check(pickups::RequestTake(k, g_an94.at, 0, &why) == 0 && std::strcmp(why, "not in the census") == 0 && pickups::RequestTake(0x1234, 0, 0, &why) == 0,
          "R7", "RequestTake: another first blueprint, or not in the census: refused");
    const uint32_t e = MakePickup(Drop(1.5f, g_an94));
    Reach(2);
    const uint32_t first = pickups::RequestTake(e, g_an94.at, 0, &why);
    Check(first && pickups::RequestTake(e, g_an94.at, 1, &why) == 0 && std::strcmp(why, "a request already waiting") == 0, "R7",
          "RequestTake: one waiting already: refused");
}

void R8() {
    Scene();
    Up();
    const uint32_t e = MakePickup(Drop(1.0f, g_an94));
    g_handOver[0] = 17;  // the M416's 18 less its hidden round, say
    g_handOver[2] = 0;   // the M320 the mod counts empty
    Reach(2);
    const char* why = "";
    const uint32_t request = pickups::RequestTake(e, g_an94.at, 0, &why);
    Check(Loaded(0) == 31 && Loaded(2) == 1, "R8", "asking writes nothing");
    Check(g_handOverAsked.size() == 2 && g_handOverAsked[0] == 0 && g_handOverAsked[1] == 2, "R8", "the counts asked of ammo for the content's slots 0 and 2");
    Reach();
    Check(pickups::Outcome(request) == 1 && g_carried[0] == 17 && g_carried[2] == 0, "R8", "the take found the mod's counts in them: 17 and 0");
    // A take that swaps nothing: the counts back.
    Scene();
    Up();
    const uint32_t e2 = MakePickup(Drop(1.0f, g_an94));
    g_handOver[0] = 17, g_handOver[2] = 0;
    g_takeMode = kNothing;
    Reach(2);
    const uint32_t r2 = pickups::RequestTake(e2, g_an94.at, 0, &why);
    Reach();
    const char* got = "";
    Check(r2 && pickups::Outcome(r2, &got) == -1 && std::strcmp(got, "the game swapped nothing") == 0 && g_carried[0] == 17 && Loaded(0) == 31 &&
              Loaded(2) == 1,
          "R8", "a take that changed nothing: \"the game swapped nothing\", the counts written back (31 and 1)");
    // Landed by its counts alone (the same kind at the same addresses).
    Scene();
    Up();
    const uint32_t e3 = MakePickup(Drop(1.0f, g_m416));
    g_takeMode = kCountsOnly;
    Reach(2);
    const uint32_t r3 = pickups::RequestTake(e3, g_m416.at, 1, &why);
    Reach();
    pickups::Swap s;
    Check(r3 && pickups::Outcome(r3) == 1 && pickups::NextSwap(0, s) && s.changed == 1 && s.slot[0] == 0 &&
              std::strcmp(s.came[0], s.went[0]) == 0 && s.came[0][0],
          "R8", "landed by its counts alone: a swap of the taken entry's slot, came and went the same");
    // ReplaceAllContent: every slot the soldier holds.
    Scene();
    Up();
    SetSlot(1, MakeWeapon(g_rpk.at, 50, 100, 3));
    PickupSpec kit = Drop(1.0f, g_an94);
    kit.content = {{g_an94.at, 0, -1, 30, 90}};
    kit.replaceAll = true;
    const uint32_t e4 = MakePickup(kit);
    for (int& h : g_handOver)
        h = 5;
    Reach(2);
    g_handOverAsked.clear();
    pickups::RequestTake(e4, g_an94.at, 0, &why);
    Check(g_handOverAsked.size() == 3 && g_handOverAsked[0] == 0 && g_handOverAsked[1] == 1 && g_handOverAsked[2] == 2, "R8",
          "ReplaceAllContent: the counts of every slot the soldier holds (0, 1, 2)");
    Reach();
    Check(Loaded(0) == 30 && Loaded(1) == 50 && Loaded(2) == 1, "R8",
          "ReplaceAllContent: the slots the take left alone (1, 2) get their counts back (50 and 1); slot 0 the AN-94's own 30");
    // The level's Stinger by a hand (2026-11-12): placed, unspawning, ReplaceAllContent and forced, its content for the empty slot 1. The M416 stays
    // in slot 0: its count as the game had it (31, the mod's 30 and its hidden round), not the mod's, which read as a shot with none fired.
    Scene();
    Up();
    PickupSpec valley = Drop(1.0f, g_stinger);
    valley.content = {{g_stinger.at, 1, -1, 1, 4}};
    valley.placed = valley.unspawn = valley.replaceAll = valley.force = true;
    const uint32_t e5 = MakePickup(valley);
    g_handOver[0] = 30, g_handOver[2] = 1;
    Reach(2);
    const uint32_t r5 = pickups::RequestTake(e5, g_stinger.at, 0, &why);
    Reach();
    Check(r5 && pickups::Outcome(r5) == 1 && BlueprintIn(1) == g_stinger.at, "R8",
          "the Stinger's take: landed, the Stinger into the empty slot 1");
    Check(Loaded(0) == 31 && Loaded(2) == 1, "R8",
          "the Stinger's take: the M416 left in slot 0 and the M320 in slot 2 have their counts as before the take (31 and 1), not the mod's (30)");
    pickups::Tick tick;
    Check(pickups::Now(tick) && tick.slots[0].loaded == 31, "R8", "the Stinger's take: the tick published with the M416's count as the game has it");
    // A dead soldier's AN-94 whose entry names slot 0, put by the game into slot 1 with the M416 left in slot 0 (2026-11-13): the M416's count back
    // as it was (31), not the hand-over's (17, a round short of the mod's once its hidden round is taken off).
    Scene();
    Up();
    const uint32_t e6 = MakePickup(Drop(1.0f, g_an94));
    g_handOver[0] = 17, g_handOver[2] = 0;
    g_takeMode = kIntoNext;
    Reach(2);
    const uint32_t r6 = pickups::RequestTake(e6, g_an94.at, 1, &why);
    Reach();
    Check(r6 && pickups::Outcome(r6) == 1 && BlueprintIn(1) == g_an94.at && BlueprintIn(0) == g_m416.at && g_carried[0] == 17, "R8",
          "an entry naming slot 0, the gun put into slot 1: landed, the M416 still in slot 0 (the take saw the mod's 17 in it)");
    Check(Loaded(0) == 31 && Loaded(2) == 1, "R8", "... the M416 and the M320 left in place have their counts as before the take (31 and 1)");
}

void R9() {
    Scene();
    Up();
    const uint32_t e = MakePickup(Drop(1.0f, g_an94));
    SetTarget(e);
    Step(2);
    Mark();
    g_gameTakes = true;
    Step();
    pickups::Swap s;
    Check(pickups::NextSwap(0, s) && !s.byHand && s.entity == e && s.changed == 2 && std::strcmp(s.came[0], "Weapons/AN94/AN94") == 0 &&
              s.taken.blueprint == g_an94.at && std::fabs(s.world[12] - 1261.0f) < 1e-3f,
          "R9", "the game's own take: a swap, not by hand, its entity the target read before, what it took and where");
    Check(Said("Pickups: the game's own pickup (R held): the AN-94 into slot 0; the M416 and the M320 into its place") == 1 && g_told.size() == 1, "R9",
          "said, ammo told");
    // Flag 4 still set at the next ticks (R still held): no second swap.
    g_rHeld = true;
    Step(3);
    Check(g_told.size() == 1, "R9", "flag 4 held set: no other swap");
    g_rHeld = false;
    // A flag 4 edge with nothing swapped: nothing.
    SetFlags(0);
    Step();
    g_takeMode = kNothing;
    g_gameTakes = true;
    Step(2);
    Check(g_told.size() == 1, "R9", "flag 4's edge with nothing landed: no swap");
    // The target changed inside the call: the after one.
    Scene();
    Up();
    const uint32_t a = MakePickup(Drop(1.0f, g_an94)), b = MakePickup(Drop(1.5f, g_rpk));
    SetTarget(a);
    Step(2);
    g_retargetTo = b;
    g_gameTakes = true;
    Step();
    Check(pickups::NextSwap(0, s) && s.entity == b && std::strcmp(s.came[0], "Weapons/RPK/RPK") == 0 && std::strcmp(s.taken.name, "Weapons/RPK/RPK") == 0 &&
              std::fabs(s.world[12] - 1261.5f) < 1e-3f,
          "R9", "the target changed inside the update: the swap's entity the after one, its place read now, taken from the slot that came");
}

void R10() {
    Scene();
    Up();
    const uint32_t e = MakePickup(Drop(1.0f, g_an94));
    g_handOver[0] = 17;
    g_takeMode = kFault;
    Reach(2);
    Mark();
    const char* why = "";
    const uint32_t r = pickups::RequestTake(e, g_an94.at, 0, &why);
    Reach();
    const char* got = "";
    pickups::Tick t;
    pickups::Now(t);
    const uint32_t serial = t.serial;
    Check(r && pickups::Outcome(r, &got) == -1 && std::strcmp(got, "the game's take faulted") == 0 && Loaded(0) == 17, "R10",
          "a fault in the take: not taken, \"the game's take faulted\", the count left as written (the state isn't known)");
    Check(Said("Pickups: the game's take faulted (exception C0000005): no more takes this session") == 1, "R10", "said once");
    Reach(3);
    pickups::Now(t);
    Check(t.serial == serial + 3, "R10", "the tick goes on");
    g_takeMode = kSwap;
    const uint32_t r2 = pickups::RequestTake(e, g_an94.at, 0, &why);
    Reach();
    Check(r2 && pickups::Outcome(r2, &got) == -1 && std::strcmp(got, "the game's take faulted earlier") == 0 && g_takes == 1, "R10",
          "no more takes this session");

    // Faults in the walk: 5 s rest, the third dead.
    Scene();
    Up();
    MakePickup(Drop(1.0f, g_an94));
    Mark();
    g_allowFault = true;
    Reach(2);
    pickups::Census c;
    Check(Said("Pickups: the walk faulted (exception C0000005): no walk for 5 s") == 1, "R10", "a fault in the getter: the walk rests 5 s, said");
    g_allowFault = false;
    pickups::Now(t);
    const uint32_t s0 = t.serial;
    for (int i = 0; i < 80; ++i)
        Reach();
    pickups::Now(t);
    Check(!pickups::Near(c) && t.serial == s0 + 80, "R10", "resting: no census after a second, the tick still published every tick");
    g_now += 4000;
    Reach(2);
    Check(pickups::Near(c), "R10", "5 s on: walked again");
    g_allowFault = true;
    g_now += 300;
    Reach(2);
    g_now += 5100;
    Reach(2);
    Check(Said("left off for this session") == 1 && !pickups::Hooked() && pickups::GameWouldTake() == -1, "R10",
          "the third fault: off for the session, said; X held as before (-1)");
    const uint32_t s1 = pickups::g_tickScratch.serial;
    Reach(5);
    Check(pickups::g_tickScratch.serial == s1, "R10", "dead: the hook passes straight through");
    // A gate that isn't in the game's code: never called, up, a fault.
    Scene();
    Up();
    MakePickup(Drop(1.0f, g_an94));
    const uint32_t odd = Alloc(0x20);
    Put<uint32_t>(odd + 0x10, 0x00001234);  // a "function" outside the code
    Put<uint32_t>(g_soldier + 0x104, odd);
    Mark();
    const int calls = g_gateCalls;
    g_now += 300;
    Reach(2);
    pickups::Now(t);
    Check(t.gatesUp && g_gateCalls == calls && Said("Pickups: a gate of the soldier's isn't in the game's code (its table") == 1, "R10",
          "a gate's function outside the game's code: not called, gates up, said");
}

void R11() {
    // Every key at its default: nothing written, before any swap.
    Scene();
    Up();
    std::vector<uint32_t> lying;
    for (int i = 0; i < 5; ++i) {
        PickupSpec s = Drop(1.0f + i, g_rpk);
        s.left = 20.0f + i;
        lying.push_back(MakePickup(s));
    }
    WriteFileText("log 1\n");
    for (int i = 0; i < 200; ++i)
        Reach();
    bool untouched = true;
    for (size_t i = 0; i < lying.size(); ++i)
        untouched = untouched && Left(lying[i]) == 20.0f + static_cast<float>(i);
    Check(untouched, "R11", "every key at its default, no swap yet: no countdown written in 3 s of walks");
    DeleteTheFile();
    // The player's own: back up under 30 s (pickup_own_seconds -1).
    Scene();
    Up();
    const uint32_t e = MakePickup(Drop(1.0f, g_an94));
    Reach(2);
    pickups::RequestTake(e, g_an94.at, 0);
    Reach(2);
    Put<float>(e + 0x10C, 25.0f);
    Rebase(e);
    g_now += 1100;
    Reach(2);
    Check(Left(e) == 180.0f, "R11", "the player's own pickup at 25 s: back to its 180");
    Put<float>(e + 0x10C, 31.0f);
    Rebase(e);
    g_now += 1100;
    Reach(2);
    Check(Left(e) == 31.0f, "R11", "... at 31 s: left alone");
    // seconds 40: raised once.
    Scene();
    Up();
    PickupSpec s = Drop(1.0f, g_rpk);
    s.left = 175.0f;
    const uint32_t r = MakePickup(s);
    WriteFileText("seconds 40\n");
    Mark();
    g_now += 400;
    Reach(3);
    g_now += 1100;
    Reach(2);
    const float once = Left(r);
    Put<float>(r + 0x10C, 30.0f);
    g_now += 1100;
    Reach(2);
    g_now += 1100;
    Reach(2);
    Check(std::fabs(once - 35.0f) < 1e-3f && Left(r) == 30.0f, "R11", "seconds 40: 175 of 180 lowered to 35 once, not again");
    Check(Said("Pickups: 1 dropped guns' time raised to 40 s (pickup_seconds)") == 1, "R11", "said");
    // unseen 1: held while in view, let go when not.
    Scene();
    Up();
    PickupSpec u = Drop(2.0f, g_rpk);
    u.left = 1.0f;
    const uint32_t v = MakePickup(u);
    g_haveFrame = true;
    g_frame.origin[0] = 1260, g_frame.origin[1] = 65, g_frame.origin[2] = -638;
    g_frame.eye[0] = 0.0f, g_frame.eye[1] = 1.6f, g_frame.eye[2] = 0.0f;
    g_frame.look[0] = 1.0f, g_frame.look[1] = 0.0f, g_frame.look[2] = 0.0f;  // towards it
    WriteFileText("unseen 1\n");
    Mark();
    g_now += 400;
    Step();  // (the file read)
    Reach(80);  // (frames all along: the eye is the last frame's)
    Check(Left(v) == 3.0f && Said("Pickups: the RPK-74M kept while in view at the end of its time (pickup_unseen)") == 1, "R11",
          "unseen 1, looked at with 1 s left: held at 3 s, said once");
    Put<float>(v + 0x10C, 1.0f);
    Rebase(v);
    g_frame.look[0] = -1.0f;  // away
    Reach(80);
    Check(Left(v) == 1.0f, "R11", "looked away: let go (left as it is)");
    g_haveFrame = false;
    Put<float>(v + 0x10C, 1.0f);
    Reach(80);
    Check(Left(v) == 1.0f, "R11", "no eye at all: nothing held for being seen");
    DeleteTheFile();
    // never: the nearest 48 kept.
    Scene();
    Up();
    std::vector<uint32_t> many;
    for (int i = 0; i < 60; ++i) {
        PickupSpec m = Drop(1.0f + 0.2f * static_cast<float>(i), g_rpk);
        m.left = 10.0f;
        many.push_back(MakePickup(m));
    }
    WriteFileText("seconds -1\n");
    g_now += 400;
    Step();  // (the file read)
    g_now += 1100;
    Reach(3);
    int kept = 0, left = 0;
    for (int i = 0; i < 60; ++i) {
        kept += i < 48 && Left(many[i]) == 180.0f;
        left += i >= 48 && Left(many[i]) == 10.0f;
    }
    char what[160];
    snprintf(what, sizeof(what), "never: the nearest 48 back to 180 (%d), the 12 farther left to the game's own time (%d)", kept, left);
    Check(kept == 48 && left == 12, "R11", what);
    DeleteTheFile();
}

void R12() {
    Scene();
    Up();
    const uint32_t e = MakePickup(Drop(1.0f, g_an94));
    Reach(2);
    Mark();
    const char* why = "";
    const uint32_t r = pickups::RequestTake(e, g_an94.at, 0, &why);
    FramesOnly(60);  // the game paused: 0.84 s
    Check(pickups::TakeOpen() && pickups::Outcome(r) == 0, "R12", "0.84 s unserved: still waiting, TakeOpen up");
    FramesOnly(15);
    const char* got = "";
    Check(pickups::Outcome(r, &got) == -1 && std::strcmp(got, "not answered in 1 s") == 0 && !pickups::TakeOpen() &&
              Said("Pickups: a take (#1) not answered in 1 s: dropped") == 1,
          "R12", "1.05 s: dropped, TakeOpen down, said");
    Reach(3);
    Check(g_takes == 0 && Said("not answered in 1 s") == 1 && Said("the game didn't take it") == 0, "R12",
          "the game's thread back: the dropped one not taken, nothing said twice");
    const uint32_t r2 = pickups::RequestTake(e, g_an94.at, 0, &why);
    Reach();
    Check(r2 == r + 1 && pickups::Outcome(r2) == 1 && g_takes == 1, "R12", "asked again: taken");
}

void R13() {
    Scene();
    Up();
    const uint32_t close = MakePickup(Drop(1.0f, g_an94)), nearer = MakePickup(Drop(0.5f, g_rpk));
    const uint32_t banned = MakePickup(Drop(0.8f, g_an94, 0.3f));
    g_notAllowed = {banned};
    PickupSpec empty = Drop(0.7f, g_an94, -0.3f);
    empty.content = {{0, 2, 0, -1, -1}};
    MakePickup(empty);
    PickupSpec crate = Drop(0.9f, g_stinger, 0.2f);
    crate.content = {{g_stinger.at, 0, -1, 1, 3}};
    crate.unspawn = true;
    MakePickup(crate);
    MakePickup(Drop(2.8f, g_rpk));
    Reach(3);
    const float eye[3] = {g_soldierAt[0], g_soldierAt[1] + 1.6f, g_soldierAt[2]};
    pickups::Reach r[8];
    const int n = pickups::Reachable(eye, 2.5f, r, 8);
    Check(n == 2 && r[0].entity == nearer && r[1].entity == close && r[0].blueprint == g_rpk.at && std::strcmp(r[1].name, "Weapons/AN94/AN94") == 0 &&
              r[1].slot == 0,
          "R13", "Reachable: allowed, eligible, with a blueprint, not keepsNoPlace, within 2.5 m of the eye, nearest first");
    Check(n == 2 && !r[1].boxLearned && r[1].boxMin[1] == -0.16f && r[1].boxMax[2] == 0.80f && std::fabs(r[1].drawn[7] - 65.025f) < 1e-4f &&
              std::fabs(r[1].drawn[3] - 1261.0f) < 1e-3f,
          "R13", "Reachable: the rifle's box till learned; drawn where it lies, 2.5 cm up");
    Check(pickups::Reachable(eye, 2.5f, r, 1) == 1 && r[0].entity == nearer, "R13", "Reachable: at most `most`");
    // The client's pickup of the AN-94, its MeshSet.
    auto client = [&](const char* setName) {
        const uint32_t c = Alloc(0x180), inst = Alloc(0x10), inner = Alloc(0x20), set = Alloc(0x80);
        Put<uint32_t>(c, kClientPickupVt);
        SetContent(c, {{g_an94.at, 0, -1, 30, 90}}, 0xAC, 0xB0);
        Put<uint32_t>(c + 0x110, inst);
        Put<uint32_t>(inst, inner);
        Put<uint32_t>(inner + 0x10, set);
        const float lo[3] = {-0.03f, -0.10f, -0.05f}, hi[3] = {0.03f, 0.12f, 0.95f};
        std::memcpy(reinterpret_cast<void*>(static_cast<uintptr_t>(set + 0x10)), lo, sizeof(lo));
        std::memcpy(reinterpret_cast<void*>(static_cast<uintptr_t>(set + 0x20)), hi, sizeof(hi));
        Put<uint32_t>(set + 0x58, Text(setName));
        Put<uint32_t>(g_clientEntities, c);
        Put<int>(g_clientSegment + 0x1C, 1);
        Put<uint32_t>(g_clientSegment + 4, g_clientEntities + 4);
    };
    client("weapons/an94/an94_Mesh");  // no "3p"
    Mark();
    for (int i = 0; i < 4; ++i) {
        g_now += 1000;
        Reach();
    }
    pickups::Reachable(eye, 2.5f, r, 8);
    Check(!r[1].boxLearned && Said("3P box") == 0, "R13", "a MeshSet without \"3p\" in its name: not taken");
    client("weapons/an94/an94_3p_Mesh");
    g_now += 1000;
    Reach();
    pickups::Reachable(eye, 2.5f, r, 8);
    const bool once = !r[1].boxLearned;
    g_now += 1000;
    Reach();
    pickups::Reachable(eye, 2.5f, r, 8);
    Check(once && r[1].boxLearned && std::fabs(r[1].boxMax[2] - 0.95f) < 1e-6f && r[0].boxMin[1] == -0.16f, "R13",
          "its 3P MeshSet: read once, then again a second later: learned (the RPK still the rifle's box)");
    Check(Said("Pickups: the AN-94's 3P box (-0.03 -0.10 -0.05) to (0.03 0.12 0.95) (its MeshSet weapons/an94/an94_3p_Mesh)") == 1, "R13", "said once");
}

void R14() {
    Scene();
    Check(pickups::GameWouldTake() == -1, "R14", "GameWouldTake before a level: -1");
    Up();
    Check(pickups::GameWouldTake() == 0, "R14", "nothing targeted: 0");
    const uint32_t e = MakePickup(Drop(1.0f, g_an94));
    SetTarget(e);
    Step();
    Check(pickups::GameWouldTake() == 1, "R14", "the AN-94 targeted: 1");
    FramesOnly(80);
    Check(pickups::GameWouldTake() == -1 && pickups::SlotOf("weapons/m416/m416") == -1, "R14", "the tick stale: -1, SlotOf -1");
    Step();
    Check(pickups::SlotOf("weapons/m416/m416") == 0 && pickups::SlotOf("WEAPONS/GADGETS/M320/M320") == 2 && pickups::SlotOf("Weapons/AN94/AN94") == -1,
          "R14", "SlotOf by name, any case; one not carried: -1");
    Check(pickups::ActiveSlot() == -1, "R14", "ActiveSlot without \"slots 1\": -1");
    WriteFileText("slots 1\n");
    g_now += 400;
    Step();
    Check(pickups::ActiveSlot() == 0, "R14", "with \"slots 1\": slot 0 by its words");
    DeleteTheFile();
}

void R15() {
    Scene();
    Up();
    pickups::Tune d = pickups::Tuning();
    Check(d.byHand && !d.glideFromDrawn && d.preArm && d.takeUnknown && !d.testFire && !d.testSecondHalf, "R15", "no file: every default");
    WriteFileText("\xEF\xBB\xBFglidefrom 1\nprearm 0\n  unknown 0\nfire 1\nhalf 2\nlevel 1\nseconds 40\nunseen 1\nown 300\nlog 2\nslots 1\n");
    Mark();
    g_now += 400;
    Step();
    pickups::Tune t = pickups::Tuning();
    Check(!t.byHand && t.glideFromDrawn && !t.preArm && !t.takeUnknown && t.testFire && t.testSecondHalf && pickups::g_seconds == 40 && pickups::g_unseen &&
              pickups::g_ownSeconds == 300 && pickups::g_logLevel == 2 && pickups::g_file.slots,
          "R15", "each key read (a UTF-8 mark and blanks before a key don't hide it)");
    Check(Said("Pickups: level 1 (pickups.txt): the game's own pickup alone (X held), a hand takes none; a dropped gun stays 40 s, and never goes while "
               "you look at it; your own 300 s") == 1,
          "R15", "the policy in force said with where it came from");
    const uint32_t e = MakePickup(Drop(1.0f, g_an94));
    Reach(3);
    const char* why = "";
    Check(pickups::RequestTake(e, g_an94.at, 0, &why) == 0 && std::strcmp(why, "a hand takes none at level 1") == 0, "R15", "level 1: a hand asks nothing");
    DeleteTheFile();
    Mark();
    g_now += 400;
    Step();
    t = pickups::Tuning();
    Check(t.byHand && !t.glideFromDrawn && t.preArm && t.takeUnknown && !t.testFire && pickups::g_seconds == 0 && !pickups::g_unseen &&
              pickups::g_ownSeconds == -1 && pickups::g_logLevel == 0 && !pickups::g_file.slots &&
              Said("Pickups: level 2 (bf3vr.ini): a hand can take a dropped gun; a dropped gun stays the game's own time; your own until you take it") == 1,
          "R15", "deleted: every default again, said");
    // take: once per write.
    Mark();
    WriteFileText("take\n");
    g_now += 400;
    for (int i = 0; i < 30; ++i)
        Step();
    Check(Said("Pickups: asked the game for the AN-94 1.0 m away (pickups.txt take, #1)") == 1 && g_takes == 1, "R15", "take: asked once, taken");
    for (int i = 0; i < 60; ++i)
        Step();
    Check(g_takes == 1, "R15", "take: not again while the file stays");
    const uint32_t f = MakePickup(Drop(-1.2f, g_rpk));
    (void)f;
    WriteFileText("take\n");
    g_now += 400;
    for (int i = 0; i < 30; ++i)
        Step();
    Check(g_takes == 2 && Said("(pickups.txt take, #2)") == 1, "R15", "take: written again, asked again");
    ClearPickups();
    WriteFileText("take\n");
    Mark();
    g_now += 400;
    for (int i = 0; i < 30; ++i)
        Step();
    Check(Said("Pickups: take: nothing the game allows within 2.5 m") == 1, "R15", "take with nothing near: said so");
    DeleteTheFile();
    // log 1: the summary each second, also with none near.
    Scene();
    Up();
    WriteFileText("log 1\n");
    Mark();
    g_now += 400;
    for (int i = 0; i < 80; ++i)
        Step();
    Check(Said("Pickups: none lying near; the game's target: none; slot 0 in hand by its words (0: 2/2/2, 2: 3/3/3); the tick ") >= 1, "R15",
          "log 1 with none near: still said, with the slots' words and the tick's time");
    MakePickup(Drop(1.4f, g_an94));
    Mark();
    for (int i = 0; i < 80; ++i)
        Reach();
    Check(Said("Pickups: 1 lying near: the nearest the AN-94 1.4 m away, 170 s left of 180 (allowed, eligible); the game's target: none") >= 1, "R15",
          "log 1: the nearest, its time, its flags");
    DeleteTheFile();
}

void R16() {
    int bad = 0;
    for (const std::string& line : g_log) {
        std::string lower = line;
        for (char& c : lower)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if ((lower.find("fail") != std::string::npos || lower.find("error") != std::string::npos || lower.find("exception") != std::string::npos) &&
            lower.find("faulted") == std::string::npos) {
            ++bad;
            std::printf("      a bad word: %s\n", line.c_str());
        }
    }
    Check(bad == 0, "R16", "no log line with fail, error or exception but the fault lines (this row's own log)");
}

// Every row's log, for R16: kept across Scene().
std::vector<std::string> g_allLog;
void Keep() {
    g_allLog.insert(g_allLog.end(), g_log.begin(), g_log.end());
}

void R17() {
    int said = 0;
    for (const std::string& line : g_allLog)
        said += line.find("Pickups: parts") != std::string::npos;
    Check(said == 0 && pickparts::g_level == 0 && !pickparts::g_hooked && pickparts::Live() == 0, "R17",
          "no parts line in any row before this one: not a word said, nothing hooked, nothing held");
    Scene();
    Up();
    struct Row {
        const char* text;
        int want;
        const char* what;
    };
    const Row rows[] = {{"parts 3\n", 3, "parts 3"},
                        {"parts 9\n", 4, "parts 9: 4 at most"},
                        {"parts -2\n", 0, "parts -2: 0"},
                        {"parts 0\n", 0, "parts 0"},
                        {"partsXYZ\n", 0, "partsXYZ: no number, not read"},
                        {"off\nparts 3\n", 0, "off with parts 3: off"},
                        {"level 0\nparts 2\n", 0, "level 0 with parts 2: off"},
                        {"parts 1\nlevel 2\n", 1, "parts 1 with level 2"}};
    for (const Row& r : rows) {
        WriteFileText(r.text);
        g_now += 400;
        Step();
        char what[160];
        snprintf(what, sizeof(what), "%s: the level in force %d", r.what, r.want);
        Check(pickparts::g_level == r.want, "R17", what);
    }
    WriteFileText("parts 3\n");
    g_now += 400;
    Step();
    const pickups::Tune t = pickups::Tuning();
    Check(pickups::g_file.level == -1 && pickups::g_file.log == 0 && !pickups::g_file.take && !pickups::g_file.haveSeconds && !pickups::g_file.haveOwn &&
              pickups::g_file.unseen == -1 && !pickups::g_file.slots && t.byHand && !t.glideFromDrawn && t.preArm && t.takeUnknown && !t.testFire &&
              !t.testSecondHalf && pickups::g_file.parts == 3,
          "R17", "parts 3 moves no other key");
    // Every key the reader knows: none begins with another (the reader goes by the line's start); "parts", the newest, holds none and is held by
    // none (a later word must not shadow it or be shadowed by it).
    const char* words[] = {"off", "level", "log", "take", "seconds", "unseen", "own", "slots", "glidefrom", "prearm", "unknown", "fire", "half", "parts",
                           "use"};
    int clashes = 0;
    for (const char* a : words)
        for (const char* b : words)
            if (a != b && (std::strncmp(a, b, std::strlen(b)) == 0 || ((std::strcmp(a, "parts") == 0 || std::strcmp(b, "parts") == 0) && std::strstr(a, b)))) {
                ++clashes;
                std::printf("      \"%s\" holds \"%s\"\n", a, b);
            }
    Check(clashes == 0, "R17", "no key of pickups.txt begins with another; parts holds none and is held by none");
    DeleteTheFile();
    g_now += 400;
    Step();
    Check(pickparts::g_level == 0 && pickups::g_file.parts == -1, "R17", "deleted: parts off again");
}

// ---- R18: a scripted use point (pickups.h's UsePoint: the AT4's hand-over on Operation Swordbreaker's roof) ----
constexpr uint32_t kUseVt = 0x0A0B0C10;  // (a fake vtable: only ever compared)
int g_useCalls = 0;
uint32_t g_useCalledEntity = 0, g_useCalledSoldier = 0;
bool g_useFaults = false, g_useCounts = true;
// The game's use as the test has it: called with the entity in ecx and the soldier on the stack; counts a use as the game's does (or doesn't: its own
// checks refused), or faults.
void __fastcall FakeUse(void* entity, void*, void* soldier) {
    ++g_useCalls;
    const uint32_t e = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(entity));
    g_useCalledEntity = e, g_useCalledSoldier = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(soldier));
    if (g_useFaults)
        RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, 0, nullptr);
    if (g_useCounts)
        Put<uint32_t>(e + 0x94, Get<uint32_t>(e + 0x94) + 1);
}
// A use point laid out as the game's: its vtable, its flags (bit 16 on: its +0xA bit 0), its data (+0x60 radius, +0x6C MaxUses, +0x90 its action,
// +0x94 its hold, +0x98 its kind, +0x9C its prompt's id), its transform (not the indexed kind), its uses (+0x94).
uint32_t MakeUsePoint(int action, int kind, uint32_t maxUses, bool enabled, const float* at, float hold = 0.0f, float radius = 2.0f) {
    const uint32_t e = Alloc(0x120), data = Alloc(0xC0);
    Put<uint32_t>(e, kUseVt);
    Put<uint32_t>(e + 8, enabled ? 0x10000u : 0u);
    Put<uint32_t>(e + 0x14, data);
    Put<uint32_t>(e + 0x1C, MakeTransform(at));
    Put<float>(data + 0x60, radius);
    Put<uint32_t>(data + 0x6C, maxUses);
    Put<int>(data + 0x90, action);
    Put<float>(data + 0x94, hold);
    Put<int>(data + 0x98, kind);
    Put<uint32_t>(data + 0x9C, Text("ID_H_INT_SP_EARTHQUAKE_EQUIP_RPG"));
    return e;
}
// The scene, up, with the use's address and vtable the test's; checks: the use's rows to check (none: all as read).
void UseScene(const pickups::Checks& checks) {
    Scene();
    pickups::g_at.useVt = kUseVt, pickups::g_at.use = Fn(&FakeUse);
    pickups::g_useChecks = checks;
    g_useCalls = 0, g_useCalledEntity = g_useCalledSoldier = 0, g_useFaults = false, g_useCounts = true;
    Mark();
    Up();
}
// Asked, served at the next tick, its outcome then.
int UseNow(uint32_t entity, const char** why, uint32_t* serial = nullptr) {
    const uint32_t s = pickups::RequestUse(entity, 0, why);
    if (serial)
        *serial = s;
    if (!s)
        return -2;
    Step();
    return pickups::UseOutcome(s, why);
}

void R18() {
    const pickups::Checks none = {nullptr, 0, nullptr, 0, nullptr, 0};
    UseScene(none);
    Check(pickups::g_useState == 2 && pickups::UseCodeReady() && Said("the game's use of one and its call are as read") == 1, "R18",
          "the use's checks as read (the test's: none to fail): the game's own use is called");
    const float at[3] = {g_soldierAt[0] + 1.0f, g_soldierAt[1], g_soldierAt[2] + 0.5f};
    const uint32_t e = MakeUsePoint(pickups::kUseReload, 0, 1, true, at);
    SetTarget(e);
    Step();
    pickups::UsePoint up;
    const bool published = pickups::UseTarget(up);
    Check(published && up.entity == e && up.action == 15 && up.kind == 0 && up.hold == 0.0f && up.maxUses == 1 && up.uses == 0 && up.enabled && up.radius == 2.0f &&
              up.haveWorld && std::fabs(up.world[12] - at[0]) < 1e-4f && std::fabs(up.world[14] - at[2]) < 1e-4f &&
              std::strcmp(up.sid, "ID_H_INT_SP_EARTHQUAKE_EQUIP_RPG") == 0 && pickups::GameWouldTake() == 2,
          "R18", "the game's target a use point: published (its action 15, a press, 1 use, on, 2 m, its place, its prompt's id); GameWouldTake 2 (R would use it)");
    const char* why = "";
    uint32_t serial = 0;
    int outcome = UseNow(e, &why, &serial);
    Check(serial && outcome == 1 && g_useCalls == 1 && g_useCalledEntity == e && g_useCalledSoldier == g_soldier && Get<uint32_t>(e + 0x94) == 1, "R18",
          "a use asked: served on the game's thread at its next tick, its use called once (the entity in ecx, the soldier pushed), its uses 1: used");
    Step();
    outcome = UseNow(e, &why);
    Check(pickups::GameWouldTake() == 0 && outcome == -1 && std::strcmp(why, "used up") == 0 && g_useCalls == 1, "R18",
          "used up (1 of 1): GameWouldTake 0; another use refused on the game's thread (used up), never called");

    // Each refusal, never calling the game's use: off, not the target any more, too late (not served within 0.5 s), the soldier asleep (the game's
    // update does nothing for it: its target is last seen's), on the game's thread; a flag, on the render thread (no hand's: never asked).
    struct Case {
        const char* name;
        int kind;
        bool enabled;
        bool retarget, late, asleep, asked;
        const char* why;
    } cases[] = {{"off", 0, false, false, false, false, true, "it isn't on"},
                 {"a flag", 3, true, false, false, false, false, "not a prompt a hand uses"},
                 {"not the target now", 0, true, true, false, false, true, "the game doesn't target it now"},
                 {"too late", 0, true, false, true, false, true, "too late"},
                 {"the soldier asleep", 0, true, false, false, true, true, "the soldier isn't awake"}};
    for (const Case& c : cases) {
        UseScene(none);
        const uint32_t p = MakeUsePoint(pickups::kUseReload, c.kind, 1, c.enabled, at);
        SetTarget(p);
        Step();
        const uint32_t s = pickups::RequestUse(p, 1, &why);
        const std::string askedWhy = why;
        if (c.retarget)
            SetTarget(MakePickup(Drop(1.0f, g_an94)));
        if (c.late)
            g_now += 600;
        if (c.asleep)
            Put<uint8_t>(g_soldier + 0xA, 0);
        Step();
        Put<uint8_t>(g_soldier + 0xA, 1);
        const int o = s ? pickups::UseOutcome(s, &why) : -2;
        const char* said = s ? why : askedWhy.c_str();
        char what[200];
        snprintf(what, sizeof(what), "refused, %s: \"%s\" (%s), the game's use never called", c.name, c.why, c.asked ? "on the game's thread" : "not asked");
        Check((c.asked ? s && o == -1 : s == 0) && std::strcmp(said, c.why) == 0 && g_useCalls == 0 && Get<uint32_t>(p + 0x94) == 0, "R18", what);
    }

    // Only a prompt a hand uses (usepoint::ByHand: the interact or the reload key, a press, the game's own reach 3 m at most). The roof's pose prompts
    // (EIAProne 14; EIAChangePose 13 held 1 s; 100 m round, 8 m off the AT4's), a hold on the interact key, the jet's 300 m prompts, a climb on the
    // jump key: published, never asked, never called.
    struct NotHand {
        const char* name;
        int action;
        float hold, radius;
    } notHand[] = {{"EIAProne, 100 m round", 14, 0.0f, 100.0f}, {"EIAChangePose held 1 s, 100 m round", 13, 1.0f, 100.0f},
                   {"EIAInteract held 1 s", 35, 1.0f, 2.0f}, {"EIAInteract 300 m round", 35, 0.0f, 300.0f}, {"EIAJump, 2 m", 12, 0.0f, 2.0f}};
    for (const NotHand& n : notHand) {
        UseScene(none);
        const uint32_t pt = MakeUsePoint(n.action, 0, 1, true, at, n.hold, n.radius);
        SetTarget(pt);
        Step();
        const bool pub = pickups::UseTarget(up) && up.entity == pt && up.action == n.action;
        const uint32_t s = pickups::RequestUse(pt, 0, &why);
        const std::string notWhy = why;
        Step();
        char what[200];
        snprintf(what, sizeof(what), "not a hand's (%s): published, not asked (\"%s\"), never called", n.name, notWhy.c_str());
        Check(pub && s == 0 && notWhy == "not a prompt a hand uses" && g_useCalls == 0 && pickups::GameWouldTake() == 0, "R18", what);
    }
    // ... and the game's thread checks it again: a reload prompt asked, its data a pose prompt's by the tick that serves it.
    UseScene(none);
    uint32_t q = MakeUsePoint(pickups::kUseReload, 0, 1, true, at);
    SetTarget(q);
    Step();
    uint32_t s1 = pickups::RequestUse(q, 0, &why);
    Put<int>(Get<uint32_t>(q + 0x14) + 0x90, 14);
    Step();
    outcome = pickups::UseOutcome(s1, &why);
    Check(s1 && outcome == -1 && std::strcmp(why, "not a prompt a hand uses") == 0 && g_useCalls == 0, "R18",
          "a point that is no hand's by the tick that serves it: refused on the game's thread too, never called");
    // A door (EIAInteract, a press, 2.3 m: Bank's breach): a hand's; X held's R never takes it (GameWouldTake 0: its key is E).
    UseScene(none);
    q = MakeUsePoint(pickups::kUseInteract, 0, 1, true, at, 0.0f, 2.3f);
    SetTarget(q);
    Step();
    const int doorTake = pickups::GameWouldTake();
    outcome = UseNow(q, &why);
    Check(doorTake == 0 && outcome == 1 && g_useCalls == 1, "R18", "a door's prompt (EIAInteract, 2.3 m): a hand's, used; GameWouldTake 0 (X held's R isn't its key)");

    // The outcome is the game's thread's own word, right after it: a tick gone stale after the use (the game's thread busy a second and more) doesn't
    // make a served use "not answered".
    UseScene(none);
    q = MakeUsePoint(pickups::kUseReload, 0, 1, true, at);
    SetTarget(q);
    Step();
    s1 = pickups::RequestUse(q, 0, &why);
    Step();
    FramesOnly(110, 15);
    const bool stale = !pickups::g_haveTick;
    outcome = pickups::UseOutcome(s1, &why);
    Check(s1 && stale && outcome == 1 && g_useCalls == 1, "R18", "served, then 1.6 s of frames with no tick (the tick stale): still used, as the game's thread said");
    // Taken by the game's thread and its use under way (as it leaves the ask while it calls the game): waiting, another hand's use waits, never "not
    // answered"; 5 s on, given up.
    UseScene(none);
    q = MakeUsePoint(pickups::kUseReload, 0, 1, true, at);
    SetTarget(q);
    Step();
    s1 = pickups::RequestUse(q, 0, &why);
    pickups::g_useAsk.taken = true;
    g_now += 2000;
    const int during = pickups::UseOutcome(s1, &why);
    const uint32_t other = pickups::RequestUse(q, 1, &why);
    const std::string otherWhy = why;
    g_now += 3500;
    const int given = pickups::UseOutcome(s1, &why);
    Check(s1 && during == 0 && other == 0 && otherWhy == "another use waits" && given == -1 && std::strstr(why, "not heard in 5 s") != nullptr, "R18",
          "taken and under way: waiting at 2 s (another hand's use waits), given up at 5.5 s");
    // Not taken within 1 s (no tick of the game's): given up, and passed by when its tick comes, never called.
    UseScene(none);
    q = MakeUsePoint(pickups::kUseReload, 0, 1, true, at);
    SetTarget(q);
    Step();
    s1 = pickups::RequestUse(q, 0, &why);
    FramesOnly(80, 15);
    const int unanswered = pickups::UseOutcome(s1, &why);
    const std::string unansweredWhy = why;
    Step();
    Check(s1 && unanswered == -1 && unansweredWhy == "not answered in 1 s" && g_useCalls == 0 && pickups::g_useServedSerial == s1, "R18",
          "not taken in 1 s: given up on the render thread, passed by on the game's (never called)");

    // pickups.txt "use 0": published (X held's R still uses it: GameWouldTake 2), a hand's use never asked; the file gone: on again.
    UseScene(none);
    WriteFileText("use 0\n");
    g_now += 400;
    q = MakeUsePoint(pickups::kUseReload, 0, 1, true, at);
    SetTarget(q);
    Step();
    const bool pubOff = pickups::UseTarget(up) && up.entity == q;
    const uint32_t offSerial = pickups::RequestUse(q, 0, &why);
    const std::string offWhy = why;
    const bool offOn = pickups::UseByHandOn();
    DeleteTheFile();
    g_now += 400;
    Step();
    Check(pubOff && offSerial == 0 && offWhy.find("use 0") != std::string::npos && !offOn && Said("no scripted prompt used by a hand (use 0)") == 1 &&
              pickups::UseByHandOn() && g_useCalls == 0,
          "R18", "use 0: published, a hand's use never asked, said once; the file gone: on again");

    // A use and a take asked in the same frame: the use served at the next tick, the take the tick after (a use's scene may change the slots the
    // take's hand-over was read for).
    UseScene(none);
    q = MakeUsePoint(pickups::kUseReload, 0, 1, true, at);
    const uint32_t gun = MakePickup(Drop(1.0f, g_an94));
    SetTarget(q);
    Reach(2);
    const int takesBefore = g_takes;
    s1 = pickups::RequestUse(q, 0, &why);
    const uint32_t take = pickups::RequestTake(gun, g_an94.at, 1, &why);
    pickups::SetReaching(true);
    Step();
    const int useFirst = pickups::UseOutcome(s1, &why), takeFirst = pickups::Outcome(take);
    pickups::SetReaching(true);
    Step();
    const int takeThen = pickups::Outcome(take);
    Check(s1 && take && useFirst == 1 && takeFirst == 0 && takeThen == 1 && g_takes == takesBefore + 1, "R18",
          "a use and a take asked together: the use at the next tick, the take not in that tick but the one after");

    // The game's own checks refuse it (its uses don't go up): called once, not used. A fault: dead for the session, said with its code.
    UseScene(none);
    uint32_t p = MakeUsePoint(pickups::kUseReload, 0, 1, true, at);
    SetTarget(p);
    Step();
    g_useCounts = false;
    outcome = UseNow(p, &why);
    Check(outcome == -1 && g_useCalls == 1 && std::strstr(why, "the game's own checks refused it") != nullptr, "R18",
          "its own checks refused it (its uses didn't go up): called once, not used");
    g_useCounts = true, g_useFaults = true;
    outcome = UseNow(p, &why);
    const bool dead = outcome == -1 && std::strstr(why, "faulted (C0000005)") != nullptr && !pickups::UseCodeReady() && !pickups::UseByKey();
    g_useFaults = false;
    uint32_t after = 0;
    const int afterOutcome = UseNow(p, &why, &after);
    Check(dead && after == 0 && afterOutcome == -2 && std::strcmp(why, "the game's use faulted earlier") == 0 && g_useCalls == 2 && pickups::GameWouldTake() == 2,
          "R18", "a fault in the game's use: said with its code; never called again this session, nor its key (UseByKey false); X held's R still the player's");

    // The use's checks not as read: its words (the vtable's slots, its class): nothing published, nothing asked; its code alone: published (its key is a
    // hand's way), a use refused before the game's thread.
    const uint32_t wrongWord = Alloc(4);
    Put<uint32_t>(wrongWord, 7);
    static const pickups::Word kWrongWord[] = {{wrongWord, 8, 4, "the use point's slot 0 (its type)"}};
    UseScene({nullptr, 0, nullptr, 0, kWrongWord, 1});
    p = MakeUsePoint(pickups::kUseReload, 0, 1, true, at);
    SetTarget(p);
    Step();
    Check(pickups::g_useState == -1 && !pickups::UseTarget(up) && pickups::GameWouldTake() == 0 && pickups::RequestUse(p, 0, &why) == 0 &&
              Said("left as the game has them") == 1,
          "R18", "its words not as read: no use point read or published, GameWouldTake 0 (today's), said once");
    static const uint8_t kOdd[] = {1, 2, 3};
    const uint32_t oddAt = Alloc(4);
    Put<uint8_t>(oddAt, 9);
    static const pickups::Bytes kWrongBytes[] = {{oddAt, kOdd, sizeof(kOdd), "its interaction update's key path to the use"}};
    UseScene({nullptr, 0, kWrongBytes, 1, nullptr, 0});
    p = MakeUsePoint(pickups::kUseReload, 0, 1, true, at);
    SetTarget(p);
    Step();
    const uint32_t refusedSerial = pickups::RequestUse(p, 0, &why);
    Check(pickups::g_useState == 1 && pickups::UseTarget(up) && up.entity == p && pickups::GameWouldTake() == 2 && refusedSerial == 0 &&
              std::strcmp(why, "the game's use isn't as read") == 0 && g_useCalls == 0 && pickups::UseByKey() &&
              Said("a hand's use of one presses the prompt's own key") == 1,
          "R18", "its code not as read: published (X held's R and a hand's key still use it: UseByKey), the game's use never asked");
    SetTarget(0);
}

}  // namespace

int main(int argc, char** argv) {
    g_showLog = argc > 1 && std::strcmp(argv[1], "log") == 0;
    g_arena = static_cast<uint8_t*>(VirtualAlloc(nullptr, kArena, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    wchar_t temp[MAX_PATH];
    GetTempPathW(MAX_PATH, temp);
    g_dir = std::wstring(temp) + L"pickruntest_" + std::to_wstring(GetCurrentProcessId()) + L"\\";
    CreateDirectoryW(g_dir.c_str(), nullptr);
    wcscpy_s(pickups::g_testFile, (g_dir + L"pickups.txt").c_str());

    void (*rows[])() = {R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15, R18};
    for (auto row : rows) {
        row();
        Keep();
    }
    R17();
    Keep();
    g_log = g_allLog;
    g_logMark = 0;
    R16();

    DeleteTheFile();
    RemoveDirectoryW(g_dir.c_str());
    std::printf("%d of %d checks failed\n", g_failures, g_counted);
    return g_failures;
}

#include "aim.h"

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "aimmemory.h"
#include "aimrules.h"
#include "ammo.h"
#include "camera.h"
#include "config.h"
#include "god.h"
#include "log.h"
#include "mountseat.h"
#include "pickups.h"
#include "skin.h"

namespace {

constexpr float kPi = 3.14159265f;
constexpr float kRadToDeg = 57.2957795f;
using aimrules::LooksLikeAngle;
using aimrules::VehicleTurn;
using aimrules::YawApart;

// weapon.txt "aimfind 0" (aim::SetFind): the search, mouse aim's last step and the learned pitch limits exactly as they were before the round that
// made the search match the camera at each window, hold mouse aim once a pass and prove a pair as it is after its wait (2026-11-13). On by default.
std::atomic<bool> g_findOn{true};
// Mouse aim steers the game's view while the look isn't written: on, and the game's window in front, as MouseAim asks (Update, each frame). A turn of
// the view by the search (TiltView, which needs the window in front too) is then undone within a few frames, and only throws the rounds fired
// meanwhile 6 degrees off.
std::atomic<bool> g_mouseSteering{false};
// The newest swap of the player's slots (a take: pickups::NewestSwap), as the render thread last saw it (Targets): an address proved before it is
// tried again, but not waited for.
std::atomic<uint32_t> g_swapSerial{0};
// One more at each find (Search): the learned pitch limits are forgotten when it changes (Update).
std::atomic<uint32_t> g_findCount{0};
// The gun in the player's soldier's hands (the game's own current weapon's object: ammo::SlotIdentity of ammo::GameSlot; 0 not read), as the render
// thread last saw it (Targets): each gun reads a look of its own, and what proved itself is kept with the gun it proved with (aimrules::Proven).
std::atomic<uintptr_t> g_weaponKey{0};
// The gun the last proof that said yes counts for (Proves: aimrules::ProvedWith of the gun as it began and as it ended). Search's own thread.
uintptr_t g_provedWith = 0;
// When the look was last lost (GetTickCount64: a new soldier, a load, a cutscene, the look gone from the camera; 0 never): the next search comes
// sooner for a while after (aimrules::SearchAgainMs).
std::atomic<ULONGLONG> g_lostAt{0};

// The player's look angles: a float yaw and, 12 bytes on, the pitch (radians), somewhere within
// 0x200 bytes after an object the client soldier reaches by its fields. Neither the object's kind
// nor the way there stays the same from one load to the next (seen 2026-10-30: two pointers away,
// then three; the block bf3vr.ini's old F3 pointer led to went elsewhere at a checkpoint, the pitch
// was written over a pointer and the player walked backwards). What does: they're exactly the
// angles the game builds the weapon camera from, before the mod turns it (camera.cpp), and they
// keep a nudge and turn that camera with it, where the copies the game rebuilds each frame don't.
// So: the soldier's pointers three deep, the pairs just after each that match the camera, each
// nudged; nothing is written until one passes. Kept while it goes on matching the camera.
constexpr float kNudge = 0.03f;           // radians, undone straight after
constexpr ULONGLONG kSearchAgain = 3000;  // ms between searches while none is found
constexpr ULONGLONG kLevelAgain = 500, kCrowdedAgain = 1000;  // with find, while mouse aim steers: a level view, a crowded one (Search)
// (With find, mouse aim holds the game's view this far off a level or straight-ahead aim, and further off both together: aimrules::HoldOffZeros.)
using aimrules::kLevelOff;
constexpr uint32_t kWindow = 0x200;       // bytes looked through after each pointer

// The look yaw is set outright each frame: where the game's aim was when the angles were found
// (less the hand's yaw then), plus the hand's yaw and the stick's turns since. Adding each frame's
// change instead let anything the game did to the angle pile up, and the picture, which takes the
// hand's yaw back out of it, turned with the gun again (user, 2026-10-30).
bool g_havePrevious = false;
float g_base = 0.0f;     // the look yaw with the hand's yaw at 0 and no stick turn
float g_turned = 0.0f;   // the stick's turns since then (radians, the hand's sense)
float g_target = 0.0f;   // the look yaw written this frame
bool g_haveTarget = false;
// g_target is mouse aim's, a yaw of the game's camera itself: no vehicle's turn (g_yawOffset) to add in LookYaw.
// The search thread sets g_yawOffset as it finds the angles, a moment before Targets hands them over: added to
// mouse aim's target then, the picture's body took a vehicle's turn for a frame and kept it from then on.
bool g_targetIsMouse = false;
// Reset() (a recenter, a mode or source switch): the next start takes the game's own yaw, not the picture's
// body (a recenter turns the hand and the head together, which the game's yaw kept then follows unseen).
bool g_reanchor = false;
// Update's last answer, the look going on: a new soldier then is a load or a respawn, whose facing is taken.
bool g_lookedLast = false;
int g_writes = 0;  // head moves written since the last Probe log
int g_gameMoves = 0;      // frames the game had moved the look yaw away from the last one written
float g_gameMoveMost = 0.0f;

// A scripted moment (a cutscene, a QTE, a vehicle) turns the look yaw itself, by up to 178 degrees a
// frame, and the head's yaw written over it each frame fought it: the view turned the wrong way (the
// user, The Great Destroyer, 2026-10-31: "When Kovac is walking into the room, it's inverted"; the car
// ride at the start of Rock and a Hard Place, the same). Moved by more than kScriptedMove, nothing is
// written until the game has left the yaw alone for kScriptedQuiet: the game's camera is then the
// body and the head turns the picture about it (xr_runtime.cpp keeps the hand's yaw as it was), the
// horizon level. weapon.txt "scripted 0" writes through it as before.
// 10 degrees a frame: the moving train's curves turned it about 2 a frame, which the 2 it was took for a
// scripted moment, stopping the aim 1.5 s at a time (the user's knife fight, 16:21); the scripted turns
// seen are 15-170 degrees.
constexpr float kScriptedMove = 10.0f / kRadToDeg;
// With a gun in hand a jump only counts if the game keeps doing it, this many frames in a row: 51 lone jumps in one run of Rock and a
// Hard Place (the yaw still after each) took the aim from the hand 1.5 s at a time (the user: the rifle "losing its aim", 2026-11-02).
constexpr int kScriptedFrames = 4;
constexpr int kScriptedQuietFrames = 8;
int g_scriptedStrikes = 0, g_scriptedQuiet = 0;
constexpr ULONGLONG kScriptedQuiet = 1500;
std::atomic<bool> g_scriptedOn{true};
bool g_scripted = false;
bool g_scriptedArmed = false;  // it began with a gun in hand: perhaps it wasn't one at all, but the game reading a slot that isn't written
float g_scriptedSeen = 0.0f;   // the look yaw as last read while scripted
ULONGLONG g_scriptedMovedAt = 0;
// How far up and down the game lets the look pitch go, as learned (radians), and the last written.
using aimrules::kSteep;  // about 52 degrees: only writes past this teach the limits
float g_pitchMax = 1.5f, g_pitchMin = -1.5f, g_pitchWritten = 0.0f;
bool g_havePitch = false;
float g_pitchHeld = 0.0f;  // how far those limits held the last pitch written back (radians)
// A learned limit tried again (with find: aimrules::LimitProbe). Render thread.
aimrules::LimitProbe g_probe;

// The looks the slot has held lately, for WeaponDeviation (recoil): each frame's write, and what was found there just before it (the game's own
// change, if it made one). The render thread only. kLookFrames frames' worth: the weapon camera the game last built came from one of them.
struct Look {
    float yaw, pitch;
    uint32_t frame;
};
constexpr int kLooks = 8, kLookFrames = 3;
Look g_looks[kLooks] = {};
int g_lookNext = 0;
uint32_t g_lookFrame = 1000;  // writes so far (from 1000: the empty entries are never among the last frames')
ULONGLONG g_lookAt = 0;       // when the last was written (GetTickCount64; 0 none)
constexpr ULONGLONG kLookFresh = 100;  // ms: older, nothing's being written

void NoteLook(float yaw, float pitch) {
    g_looks[g_lookNext] = {yaw, pitch, g_lookFrame};
    g_lookNext = (g_lookNext + 1) % kLooks;
}

// The look yaws written, for the eye's arm (WrittenYaws; headmove.h): the game's eye stands on the one its last tick read, which the view builder's thread tells from the last few. Kept by
// their number among all the writes, the last kWritten of them, each in one word with that number in its high half, so the other thread reads a whole one whatever is being written
// meanwhile; the newest's number, and the first number that still counts (one past the newest while no look is written).
constexpr uint32_t kWritten = 8;
std::atomic<uint64_t> g_written[kWritten];
std::atomic<uint32_t> g_writtenNewest{0}, g_writtenFrom{1};
std::atomic<ULONGLONG> g_writtenAt{0};    // when the newest was written (GetTickCount64; 0 none)
constexpr ULONGLONG kWrittenFresh = 250;  // ms: older, no frame is being worked at all

void NoteWritten(float yaw) {
    const uint32_t number = g_writtenNewest.load(std::memory_order_relaxed) + 1;
    uint32_t bits = 0;
    std::memcpy(&bits, &yaw, sizeof(bits));
    g_written[number % kWritten].store(static_cast<uint64_t>(number) << 32 | bits, std::memory_order_relaxed);
    g_writtenAt.store(GetTickCount64(), std::memory_order_relaxed);
    g_writtenNewest.store(number, std::memory_order_release);
}

// No look is written (any more): none of those written so far counts.
void DropWritten() {
    g_writtenFrom.store(g_writtenNewest.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
}

std::atomic<uintptr_t> g_yawAddress{0};  // the angles found, or 0 (the pitch is 0xC on)
std::atomic<uintptr_t> g_foundFor{0};    // the client soldier they were looked for from
// The angles come in a row of five slots 0x40 apart: the game reads one and copies it over the others at each of its ticks, and a weapon
// switch changes which (found 2026-11-04, nudging each from outside with the launcher back in hand: only the third kept it and turned the camera,
// the first having been the one at the level's start). Written to the one found alone, after the first switch to the launcher the game's aim
// stayed where it was and the writes were undone 30 times a second: the rifle and the launcher shot where the soldier had last been looking,
// he walked the wrong way, and the value read back still matched, so nothing looked again (the user, 2026-11-04). All of them are written.
using aimrules::kSlotStep;      // 0x40
using aimrules::kSlotsEachWay;  // 36: they aren't always side by side: on two of three loads scanned, the copies were 0x300 to 0x880 from the slot the game read
std::atomic<uintptr_t> g_slots[8];  // the others found with g_yawAddress (four in the row), 0 past the last
std::atomic<bool> g_searching{false};
ULONGLONG g_searchedAt = 0;              // render thread
ULONGLONG g_mismatchSince = 0;           // render thread: when they stopped matching the camera
// The last that proved themselves for a soldier, and how soon to look again. The aim went off for a quarter of a run (283 of 1068 s, the user, 2026-11-04: "it happens on and off"): a
// shake or a scripted moment dropped the angles, and the whole search then took 20 to 93 s to find the very same address again, the mouse steering meanwhile. They're tried first, proved
// again as any are, at once and every kQuickAgain while it can't yet, with the camera held still for it; only after kQuickFor does the whole search come back.
std::atomic<uintptr_t> g_provenAddress{0}, g_provenFor{0};
std::atomic<ULONGLONG> g_searchAgain{kSearchAgain};
std::atomic<ULONGLONG> g_quickSince{0};
constexpr ULONGLONG kQuickAgain = 400, kQuickFor = 15000;
constexpr DWORD kCameraSettles = 60;  // ms: a game tick or two for the game's camera to come to the angles, once mouse aim holds it

bool SafeWrite(uintptr_t address, float value) {
    __try {
        *reinterpret_cast<float*>(address) = value;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// (LooksLikeAngle and YawApart: aimrules.h.)

// How a pair was reached from the soldier: the field followed at each step (its index among the
// node's 0x100), then the pair's place in the window after the last pointer.
struct Route {
    int steps = 0;
    uint16_t field[3] = {};
    uint16_t offset = 0;
};

// Every yaw-and-pitch pair within kWindow after a pointer the soldier's fields lead to, three
// deep, that matches the camera's angles (routes: how each was reached, alike in order).
// fresh (with find): the camera read again just after each window and the near test (aimrules::NearPair, within), so a look turning during the walk
// (360-390 ms, mouse aim steering it after the gun) is still compared with the camera made from it; Matching reads the list again at one instant.
// full: set when the list reached aimrules::kMostNear and the walk stopped adding.
std::vector<uintptr_t> Pairs(uintptr_t soldier, float cameraYaw, float cameraPitch, std::vector<Route>* routes = nullptr,
                            bool anyYaw = false, bool fresh = false, bool* full = nullptr, float within = aimrules::kNear) {
    std::vector<uintptr_t> pairs;
    struct Node {
        uintptr_t at;
        Route route;
    };
    std::vector<Node> level{{soldier, {}}}, next;
    std::unordered_set<uintptr_t> seen{soldier};
    std::unordered_set<uintptr_t> listed;  // (fresh: the near list may hold thousands)
    uint32_t window[kWindow / 4 + 3];
    for (int depth = 0; depth < 3; ++depth) {
        for (const Node& node : level) {
            uint32_t fields[0x100];
            if (!camera::SafeRead(node.at, fields, sizeof(fields)))
                continue;
            for (uint16_t f = 0; f < 0x100; ++f) {
                const uint32_t v = fields[f];
                if (v < 0x10000 || v >= 0xFFFF0000u || (v & 3) || !seen.insert(v).second)
                    continue;
                Route route = node.route;
                route.field[route.steps++] = f;
                if (depth < 2 && seen.size() < 200000)
                    next.push_back({v, route});
                if (!camera::SafeRead(v, window, sizeof(window)))
                    continue;
                if (float y = 0.0f, p = 0.0f; fresh && camera::GameWeaponAngles(y, p))
                    cameraYaw = y, cameraPitch = p;
                for (uint32_t i = 0; i < kWindow / 4; ++i) {
                    float yaw, pitch;
                    std::memcpy(&yaw, &window[i], 4);
                    std::memcpy(&pitch, &window[i + 3], 4);
                    // Any yaw, but not a zero (a null pointer as often as an angle: Proves).
                    const bool match = fresh ? aimrules::NearPair(yaw, pitch, cameraYaw, cameraPitch, anyYaw, within) && listed.insert(v + i * 4).second
                                             : aimrules::StrictPair(yaw, pitch, cameraYaw, cameraPitch, anyYaw) &&
                                                   std::find(pairs.begin(), pairs.end(), v + i * 4) == pairs.end();
                    if (match) {
                        pairs.push_back(v + i * 4);
                        if (routes) {
                            route.offset = static_cast<uint16_t>(i * 4);
                            routes->push_back(route);
                        }
                        if (fresh && pairs.size() >= aimrules::kMostNear) {
                            if (full)
                                *full = true;
                            return pairs;
                        }
                    }
                }
            }
        }
        level.swap(next);
        next.clear();
    }
    return pairs;
}

// The route the look angles were last found by (Search): a new soldier (each load, checkpoint or life)
// is tried by it first, one nudge instead of a whole search (the user, 2026-10-31: the free arms
// waited 5-10 s for the aim after every load, the game's own arms shown meanwhile).
Route g_lastRoute;
bool g_haveLastRoute = false;
// The camera's yaw less the look yaw found (radians): none on foot, a moving vehicle's turn on The Great
// Destroyer's train. Followed slowly as the vehicle turns (Targets).
float g_yawOffset = 0.0f;
// (Under 1.5 degrees of it is none, exactly: aimrules::VehicleTurn.)
constexpr float kLookStill = 0.1f / kRadToDeg;  // radians a frame: the hand as good as still
constexpr int kLookStillFrames = 6;             // and for this long: the camera has caught up with the look

// The pairs matching only the camera's pitch (a vehicle's look yaw is its own, not the camera's).
std::vector<uintptr_t> PitchPairs(uintptr_t soldier, float cameraPitch, std::vector<Route>* routes) {
    return Pairs(soldier, 0.0f, cameraPitch, routes, true);
}

// Kept in aimroute.txt next to the mod's DLL, so the next launch starts with it too.
std::wstring RoutePath() {
    HMODULE self = nullptr;
    wchar_t path[MAX_PATH] = {};
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(&RoutePath), &self) ||
        !GetModuleFileNameW(self, path, MAX_PATH))
        return {};
    std::wstring p = path;
    return p.substr(0, p.find_last_of(L'\\') + 1) + L"aimroute.txt";
}

void SaveRoute() {
    FILE* f = nullptr;
    if (_wfopen_s(&f, RoutePath().c_str(), L"w") || !f)
        return;
    fprintf(f, "%d %u %u %u %u\n", g_lastRoute.steps, g_lastRoute.field[0], g_lastRoute.field[1], g_lastRoute.field[2],
            g_lastRoute.offset);
    fclose(f);
}

void LoadRoute() {
    static bool s_loaded = false;
    if (s_loaded)
        return;
    s_loaded = true;
    FILE* f = nullptr;
    if (_wfopen_s(&f, RoutePath().c_str(), L"r") || !f)
        return;
    unsigned a = 0, b = 0, c = 0, o = 0;
    int steps = 0;
    if (fscanf_s(f, "%d %u %u %u %u", &steps, &a, &b, &c, &o) == 5 && steps >= 1 && steps <= 3 && a < 0x100 && b < 0x100 &&
        c < 0x100 && o < kWindow) {
        g_lastRoute.steps = steps;
        g_lastRoute.field[0] = static_cast<uint16_t>(a), g_lastRoute.field[1] = static_cast<uint16_t>(b);
        g_lastRoute.field[2] = static_cast<uint16_t>(c), g_lastRoute.offset = static_cast<uint16_t>(o);
        g_haveLastRoute = true;
        Log("Aim: the last route to the look angles read from aimroute.txt (%d steps)", steps);
    }
    fclose(f);
}

uintptr_t Follow(uintptr_t soldier, const Route& route) {
    uintptr_t at = soldier;
    for (int s = 0; s < route.steps; ++s) {
        uint32_t v = 0;
        if (!camera::SafeRead(at + route.field[s] * 4, &v, sizeof(v)) || v < 0x10000 || v >= 0xFFFF0000u || (v & 3))
            return 0;
        at = v;
    }
    return at + route.offset;
}

// A load leaves the pitch exactly level until it's written, and level, nothing is nudged (below):
// the game's view is tilted a few degrees by the mouse, the game's own input, so the next search
// can go ahead. Only with the game's window in front (the mouse would move the desktop's otherwise).
void TiltView(bool turn) {
    // Never in a gun's seat (mountseat.h, never today): its mouse turns the turret there (a search begun before the seat, on its own thread, would end by
    // turning the ring and the gun about 6 degrees: the mounted gun's second review).
    if (mountseat::Quiet(mountgun::kLookSearch))
        return;
    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    if (pid != GetCurrentProcessId())
        return;
    INPUT in{};
    in.type = INPUT_MOUSE;
    in.mi.dy = 24;            // about 6 degrees down at the test setup's mouse speed
    in.mi.dx = turn ? 24 : 0;  // and as much to the side, for a yaw that's a zero too
    in.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, &in, sizeof(in));
}

// More pairs than this matching the camera means its angles are ones memory is full of (a yaw or
// pitch near zero): 163 pairs at yaw 0.0085 once, and nudging them crashed the game (2026-10-31).
// Nothing is nudged then; the view is turned a little and searched again.
constexpr size_t kMostPairs = 16;

// Whether a yaw/pitch pair is the player's look: with the camera still (not turning, not firing),
// its yaw nudged one way and its pitch the other, the pair keeps them and the camera's yaw and pitch
// both follow, each its own way; put back, the camera comes back. A yaw nudge alone, checked only
// for the camera moving about as much, took two wrong pairs in one evening: recoil and the smooth
// turn moved the camera during the test, and the mod then wrote where the game wasn't looking (the
// user's shots at the floor went elsewhere, 2026-10-30).
// kOverwritten (with find only; a kNo before): the nudge went in and the game wrote over it (a copy it rebuilds from the slot it reads, or a slot it
// reads no more). Any answer but kYes, kKept and kBusy is taken as kNo wherever it isn't told apart.
enum class Proof { kNo, kKept, kYes, kBusy, kOverwritten };

// How far the camera may drift in the 120 ms before a nudge (radians, 0.35 degrees): on The Great
// Destroyer's moving train its sway always passed the 0.002 it was, and the aim was never found there
// (the user's shots missed, 16:04). The nudge it then has to follow, and come back from, still proves it.
constexpr float kStillMost = 0.006f;

// While a candidate is being nudged (Proves): mouse aim holds its corrections, or they would undo the nudge
// (MouseAim). How long it held them in the current search, and for how many candidates (the search's own
// thread; Search logs them).
std::atomic<bool> g_proving{false};
ULONGLONG g_heldMs = 0;
int g_nudged = 0;
// With find, mouse aim held for a whole pass instead (PassHold): settled once, its pairs read at one instant and every candidate nudged inside it. Each
// candidate held only from the start of its own test, a correction mouse aim sent the frame before landed inside its 120 ms wait and refused the very
// slot the game reads (its swap is only over the value read before the wait), or the camera it was first compared with was a tick behind the look.
std::atomic<bool> g_passHeld{false};
struct Proving {
    const ULONGLONG since = GetTickCount64();
    Proving() { g_proving = true; }
    ~Proving() {
        g_proving = false;
        if (!g_passHeld.load())  // (inside a pass's hold its time is the pass's)
            g_heldMs += GetTickCount64() - since;
        ++g_nudged;
    }
};
struct PassHold {
    const ULONGLONG since = GetTickCount64();
    PassHold() { g_passHeld = true; }
    ~PassHold() {
        g_passHeld = false;
        g_heldMs += GetTickCount64() - since;
    }
};

// Puts value at address only if it still holds expected, bit for bit: false if it doesn't (or can't be
// written).
bool SwapIfUnchanged(uintptr_t address, float expected, float value) {
    LONG was = 0, now = 0;
    std::memcpy(&was, &expected, sizeof(was));
    std::memcpy(&now, &value, sizeof(now));
    __try {
        return InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(address), now, was) == was;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// A nudge undone where it's still there: by the nudge alone, so a turn the game added meanwhile stays; a
// value the game replaced is left as the game made it.
void Undo(uintptr_t address, float nudged, float nudge) {
    float now = 0.0f;
    if (camera::SafeRead(address, &now, sizeof(now)) && std::fabs(now - nudged) < kNudge * 0.3f)
        SwapIfUnchanged(address, now, now - nudge);
}

// fresh (with find): the pair taken as it is after the 120 ms (the same tests again), so the game's own small moves of the look meanwhile (it moved the
// AN-94's written look itself in 22 of 30 seconds of the user's try, by 0.04 to 1.25 degrees) no longer refuse the very slot it reads; a nudge already
// written over after 50 ms is told then (kOverwritten: a copy is rebuilt at the game's next tick, 30 a second) instead of after 150. offset: the camera's
// yaw less the pair's as nudged, read at one moment with the camera still (aimrules::ProofTurn). A yes sets g_provedWith: the gun in hand as it began
// and as it ended, the same, or aimrules::kWeaponUnsure (a switch landing inside it).
Proof Proves(uintptr_t at, bool anyYaw = false, bool fresh = false, float* offset = nullptr) {
    // Never a nudge in a gun's seat (mountseat.h, never today; a search begun before the seat goes on on its own thread): a candidate there is a vehicle's
    // unknown. Busy, as a moving camera: nothing counted as tried.
    if (mountseat::Quiet(mountgun::kLookSearch))
        return Proof::kBusy;
    const Proving proving;
    const uintptr_t weaponAtStart = g_weaponKey.load();
    float yaw = 0.0f, pitch = 0.0f, still[2][2] = {};
    if (!camera::SafeRead(at, &yaw, sizeof(yaw)) || !camera::SafeRead(at + 0xC, &pitch, sizeof(pitch)) ||
        !camera::GameWeaponAngles(still[0][0], still[0][1]) || (!anyYaw && YawApart(yaw, still[0][0]) > 0.02f))
        return Proof::kNo;
    // Only real angles are nudged, never a zero: zeros near the soldier are null pointers and empty lists as
    // often as level views, and the pitch-alone pass (any yaw) let them through. Nudged, the game read the
    // 0.03 as a pointer and crashed (bf3.exe+179AE2 reading 3CF5C28F, 2026-10-31 19:54); the crashes just
    // after the interrogation (lists and nodes that looked freed) were likely the same.
    if (std::fabs(yaw) < 0.02f || std::fabs(pitch) < 0.02f || !LooksLikeAngle(yaw, false) || !LooksLikeAngle(pitch, true))
        return Proof::kNo;
    Sleep(120);
    if (!camera::GameWeaponAngles(still[1][0], still[1][1]) || YawApart(still[1][0], still[0][0]) > kStillMost ||
        std::fabs(still[1][1] - still[0][1]) > kStillMost)
        return Proof::kBusy;
    if (fresh) {
        float now[4] = {};
        if (!camera::SafeRead(at, now, sizeof(now)) || (!anyYaw && YawApart(now[0], still[1][0]) > 0.02f) || std::fabs(now[0]) < 0.02f ||
            std::fabs(now[3]) < 0.02f || !LooksLikeAngle(now[0], false) || !LooksLikeAngle(now[3], true))
            return Proof::kNo;
        yaw = now[0], pitch = now[3];
        if (offset)
            *offset = aimrules::ProofTurn(still[1][0], yaw);
    }
    // The pitch towards level, so the nudge never goes past the game's limits. Each nudge goes only over the
    // value read, and is undone only where it's still there (Undo): put back regardless, as it was, a stale
    // value could land over whatever the game had put there meanwhile.
    const float pitchNudge = pitch > 0.0f ? -kNudge : kNudge;
    if (!SwapIfUnchanged(at, yaw, yaw + kNudge))
        return Proof::kNo;
    if (!SwapIfUnchanged(at + 0xC, pitch, pitch + pitchNudge)) {
        Undo(at, yaw + kNudge, kNudge);
        return Proof::kNo;
    }
    if (fresh) {
        Sleep(50);
        float early[4] = {};
        if (camera::SafeRead(at, early, sizeof(early)) &&
            !(std::fabs(early[0] - (yaw + kNudge)) < kNudge * 0.3f && std::fabs(early[3] - (pitch + pitchNudge)) < kNudge * 0.3f)) {
            Undo(at, yaw + kNudge, kNudge);
            Undo(at + 0xC, pitch + pitchNudge, pitchNudge);
            return Proof::kOverwritten;
        }
        Sleep(100);
    } else {
        Sleep(150);
    }
    float keptYaw = 0.0f, keptPitch = 0.0f, nudged[2] = {};
    const bool kept = camera::SafeRead(at, &keptYaw, sizeof(keptYaw)) &&
                      camera::SafeRead(at + 0xC, &keptPitch, sizeof(keptPitch)) &&
                      std::fabs(keptYaw - (yaw + kNudge)) < kNudge * 0.3f &&
                      std::fabs(keptPitch - (pitch + pitchNudge)) < kNudge * 0.3f;
    // Signed: the camera's yaw up by the nudge, its pitch the pitch nudge's way.
    bool followed = camera::GameWeaponAngles(nudged[0], nudged[1]);
    if (followed) {
        float yawMoved = nudged[0] - still[1][0];
        yawMoved -= 2 * kPi * std::round(yawMoved / (2 * kPi));
        followed = std::fabs(yawMoved - kNudge) < kNudge * 0.3f &&
                   std::fabs((nudged[1] - still[1][1]) - pitchNudge) < kNudge * 0.3f;
    }
    // Undone first, kept or not: one of the two kept while the other was rebuilt stayed nudged before.
    Undo(at, yaw + kNudge, kNudge);
    Undo(at + 0xC, pitch + pitchNudge, pitchNudge);
    if (!kept)
        return fresh ? Proof::kOverwritten : Proof::kNo;
    if (!followed)
        return Proof::kKept;
    Sleep(150);
    float back[2] = {};
    const bool returned = camera::GameWeaponAngles(back[0], back[1]) && YawApart(back[0], still[1][0]) < kNudge * 0.3f &&
                          std::fabs(back[1] - still[1][1]) < kNudge * 0.3f;
    if (returned)
        g_provedWith = aimrules::ProvedWith(weaponAtStart, g_weaponKey.load());
    return returned ? Proof::kYes : Proof::kKept;
}

// A slot of the row as it stands now: a pointer's shape three words before its yaw, the words between them all zero (2026-11-04, in memory: pointer, 0, 0, yaw, 0, 0,
// pitch), two plausible angles. Checked again before each write: this memory is the game's.
bool SlotShaped(uintptr_t at, float& yaw, float& pitch) {
    uint32_t words[7];
    if (!camera::SafeRead(at - 0xC, words, sizeof(words)))
        return false;
    return aimrules::SlotWords(words, yaw, pitch);
}

int SlotCount() {
    int count = 0;
    for (const auto& slot : g_slots)
        count += slot.load(std::memory_order_relaxed) != 0;
    return count;
}

// The walk's near list (Pairs, fresh) read again at one instant, mouse aim held and the camera settled (with find): those that pass the walk's own
// test now (aimrules::StrictPair), with their routes; shaped: how many of them have the slot's shape (SlotShaped), and those first, each kind in the
// walk's order (the slot the game reads has the row's shape, as its copies do; a stray float seldom has, and each one nudged holds mouse aim 270 ms).
std::vector<uintptr_t> Matching(const std::vector<uintptr_t>& nearList, const std::vector<Route>& nearRoutes, float cameraYaw, float cameraPitch,
                                bool anyYaw, std::vector<Route>* routes, int* shaped) {
    std::vector<uintptr_t> pairs, rest;
    std::vector<Route> pairRoutes, restRoutes;
    for (size_t k = 0; k < nearList.size(); ++k) {
        float pair[4] = {}, y = 0.0f, p = 0.0f;
        if (!camera::SafeRead(nearList[k], pair, sizeof(pair)) || !aimrules::StrictPair(pair[0], pair[3], cameraYaw, cameraPitch, anyYaw))
            continue;
        const bool slot = SlotShaped(nearList[k], y, p);
        (slot ? pairs : rest).push_back(nearList[k]);
        (slot ? pairRoutes : restRoutes).push_back(k < nearRoutes.size() ? nearRoutes[k] : Route{});
    }
    if (shaped)
        *shaped = static_cast<int>(pairs.size());
    pairs.insert(pairs.end(), rest.begin(), rest.end());
    if (routes) {
        *routes = std::move(pairRoutes);
        routes->insert(routes->end(), restRoutes.begin(), restRoutes.end());
    }
    return pairs;
}

// With find, what proved itself for the player's soldier (aimrules::Proven: up to four addresses, the take each came after), the route's own record
// (the soldier it was last saved or proven on, its strikes in a row for this soldier), and the pass by the pitch alone's candidates tried lately (gone
// on from in the next search). Search's own thread (one search at a time).
aimrules::Proven g_proven;
struct RouteTrust {
    uintptr_t provenOn = 0;  // 0 after LoadRoute: a new launch's route is a stranger's
    uintptr_t strikesFor = 0;
    int strikes = 0;
    void For(uintptr_t soldier) {
        if (soldier != strikesFor)
            strikesFor = soldier, strikes = 0;
    }
} g_routeTrust;
std::unordered_set<uintptr_t> g_pitchTried;
uintptr_t g_pitchTriedFor = 0;
// With find, the row's places the sweep proved for this soldier and got no proof from (aimrules::SweepChoice leaves them out, so the next search goes
// on to the others; all of them tried, it starts again after aimrules::kSweepRest searches). Search's own thread.
aimrules::SweepTried g_swept;
// With find, the reach's (aimrules::ReachChoice) the same way, and the looks proved in this launch, whatever the soldier, newest first: the reach looks
// about them too (a new life's look lies in the same memory as earlier ones': the user's M4A1 of 2026-11-19 01:15:49, E754614C, 0x276BC0 from the
// M4A1's look of the life three before, E77BCD0C at 00:59:52). Search's own thread.
aimrules::SweepTried g_reachTried;
constexpr int kProvenEver = 8;
uintptr_t g_provenEver[kProvenEver] = {};

void NoteProvenEver(uintptr_t at) {
    int from = kProvenEver - 1;
    for (int k = 0; k < kProvenEver; ++k)
        if (g_provenEver[k] == at) {
            from = k;
            break;
        }
    for (int k = from; k > 0; --k)
        g_provenEver[k] = g_provenEver[k - 1];
    g_provenEver[0] = at;
}

// The reach's look through memory (with find): aimrules::ReachThrough on the game's memory (aimmemory::GameMemory: plain memory in the working set,
// never a thread's stack, 25 ms at most), about the anchors in ReachSpans' order (nearest first), the slow allocations kept for the launch
// (g_reachMemo). Search's own thread.
struct ReachRun {
    std::vector<aimrules::Reached> places;
    int spans = 0;
    aimrules::ReachTotals totals;
    int threadStacks = 0;      // the threads' stacks listed (aimmemory::AddThreadStacks)
    uint64_t listMicros = 0;   // ... in how long
};
aimrules::ReachMemo g_reachMemo;
ReachRun ReachLook(const uintptr_t* anchors, int count, float cameraYaw, float cameraPitch, bool pointerAlone) {
    ReachRun run;
    constexpr int kSpansMost = 256;
    static aimrules::Span s_spans[kSpansMost];
    run.spans = aimrules::ReachSpans(anchors, count, s_spans, kSpansMost, aimrules::kReachBytesMost);
    // Its piece in an allocation of its own (so it's told apart as a whole), kept for the launch.
    static uint32_t* s_piece =
        static_cast<uint32_t*>(VirtualAlloc(nullptr, aimrules::kReachPieceWords * 4, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!s_piece)
        return run;
    aimmemory::GameMemory memory;
    memory.Start();
    memory.piece = s_piece;
    MEMORY_BASIC_INFORMATION mine{}, stack{};
    VirtualQuery(s_piece, &mine, sizeof(mine));
    VirtualQuery(&stack, &stack, sizeof(stack));  // (this thread's stack, whatever the list below gives)
    memory.own.push_back(reinterpret_cast<uintptr_t>(mine.AllocationBase));
    memory.own.push_back(reinterpret_cast<uintptr_t>(stack.AllocationBase));
    const uint64_t listFrom = memory.Micros();
    run.threadStacks = aimmemory::AddThreadStacks(memory.own);
    run.listMicros = memory.Micros() - listFrom;
    run.totals = aimrules::ReachThrough(s_spans, run.spans, memory, g_reachMemo, cameraYaw, cameraPitch, pointerAlone, run.places);
    return run;
}

// A proof's answer in the log's words (Search's lines of its candidates and of the row swept).
// A proof's answer as the rules take it (aimrules::SweepTried).
aimrules::Answer AnswerOf(Proof answer) {
    switch (answer) {
    case Proof::kYes:
        return aimrules::Answer::kYes;
    case Proof::kKept:
        return aimrules::Answer::kKept;
    case Proof::kBusy:
        return aimrules::Answer::kBusy;
    case Proof::kOverwritten:
        return aimrules::Answer::kOverwritten;
    default:
        return aimrules::Answer::kNo;
    }
}

const char* ProofWords(Proof answer) {
    switch (answer) {
    case Proof::kYes:
        return "proved";
    case Proof::kKept:
        return "kept the nudge, the camera didn't follow";
    case Proof::kBusy:
        return "not tried, the camera moving";
    case Proof::kOverwritten:
        return "written over";
    default:
        return "refused";
    }
}

// The other slots of the row the angles proved at (kSlotStep): each with the same two angles and the row's shape. Only those are written. Never on zeros: just before the
// row there's a pointer followed by nulls, which any level look would match, and floats written over a pointer crashed the game once (bf3.exe+179AE2). So with the
// soldier looking level or due north none are taken yet: Targets tries again each second until the row is whole. Adds to those it has.
int FindSlots(uintptr_t found) {
    constexpr float kNotZero = 0.02f;
    float yaw = 0.0f, pitch = 0.0f;
    if (!camera::SafeRead(found, &yaw, sizeof(yaw)) || !camera::SafeRead(found + 0xC, &pitch, sizeof(pitch)) || std::fabs(pitch) < kNotZero || YawApart(yaw, 0.0f) < kNotZero)
        return SlotCount();
    for (int k = -kSlotsEachWay; k <= kSlotsEachWay; ++k) {
        if (!k)
            continue;
        const uintptr_t at = found + static_cast<intptr_t>(k) * static_cast<intptr_t>(kSlotStep);
        bool known = false;
        for (const auto& slot : g_slots)
            known = known || slot.load(std::memory_order_relaxed) == at;
        float slotYaw = 0.0f, slotPitch = 0.0f;
        if (known || !SlotShaped(at, slotYaw, slotPitch) || std::fabs(slotPitch) < kNotZero || YawApart(slotYaw, 0.0f) < kNotZero || YawApart(slotYaw, yaw) >= 0.05f ||
            std::fabs(slotPitch - pitch) >= 0.05f)
            continue;
        for (auto& slot : g_slots)
            if (!slot.load(std::memory_order_relaxed)) {
                slot = at;
                break;
            }
    }
    return SlotCount();
}

// Background thread: the pairs tested in turn; the first that proves to be the player's look
// (Proves) is used.
void Search(uintptr_t soldier) {
    const bool find = g_findOn.load();  // (one answer for the whole search: weapon.txt's "aimfind" may change in the middle of one)
    const ULONGLONG searchStart = GetTickCount64();
    g_heldMs = 0, g_nudged = 0;
    int byPitch = 0;  // candidates nudged by the pitch alone (the "no look angles yet" line doesn't count them)
    float cameraYaw = 0.0f, cameraPitch = 0.0f;
    for (int wait = 0; wait < 20 && !camera::GameWeaponAngles(cameraYaw, cameraPitch); ++wait)  // just after a load
        Sleep(50);
    uintptr_t found = 0;
    size_t candidates = 0;
    int kept = 0;
    // Looking level, the pitch is a zero, which matches every zero after a yaw-like float: nothing
    // is nudged then (nudging stray matches crashed the game once, from outside, 2026-10-30).
    int busy = 0;  // pairs not tried: the camera was moving (turning, firing)
    bool crowded = false;
    LoadRoute();
    // With find: the takes so far (an address proved before the newest is tried again, not waited for), whether mouse aim steers the game's view (a turn
    // of it is then undone at once), what the walk saw, this soldier's records.
    const uint32_t swapAtStart = g_swapSerial.load();
    // The gun in hand as this search began: what it tries first (aimrules::Proven::Order). What it proves is kept with the gun its proof counts for
    // (g_provedWith, set by the proof: a switch may land between this and the proof).
    const uintptr_t weaponAtStart = g_weaponKey.load();
    g_provedWith = aimrules::kWeaponUnsure;
    const bool steering = find && g_mouseSteering.load();
    size_t nearCount = 0;
    bool nearFull = false;
    int shaped = -1;    // the first pass's slot-shaped pairs at the camera's yaw, at one instant (-1: it didn't look)
    int pitchLeft = 0;  // the pass by the pitch alone's candidates left for the next search
    bool pitchSkipped = false;  // ... or the pass not run at all: the look's copies seen at the camera's yaw (aimrules::PitchPassWanted)
    size_t pitchNearCount = 0;  // ... its walk's list
    bool pitchFull = false;
    std::vector<uintptr_t> firstTried;  // the first pass's candidates nudged in this search: not nudged again by the pitch alone
    // With find, for the log: each candidate of the first pass with its answer, and the row swept from its written-over copies (aimrules::SweepWanted).
    struct Tried {
        uintptr_t at;
        bool slot;
        Proof answer;
    };
    std::vector<Tried> tried;
    bool swept = false;  // the row was looked through
    int sweptCopies = 0, sweptTaken = 0, sweptProved = 0, sweptBefore = 0;  // the copies swept from; taken, proved, left out as tried before
    uintptr_t copies0 = 0;                                                  // the first of those copies (for the log)
    aimrules::Swept sweptAt[aimrules::kSweepMost] = {};
    Proof sweptAnswer[aimrules::kSweepMost] = {};
    // With find, for the log: the reach (aimrules::ReachWanted), its anchors and what it read, took and proved; whether it took a pointer alone
    // (aimrules::ReachPointerAlone: its copies, those with the row's shape).
    bool reached = false, reachPointerAlone = false;
    int reachAnchors = 0, reachTaken = 0, reachProved = 0, reachBefore = 0, reachCrowd = 0, reachCopies = 0, reachShapedCopies = 0;
    ReachRun reachRun;
    aimrules::Reached reachAt[aimrules::kSweepMost] = {};
    Proof reachAnswer[aimrules::kSweepMost] = {};
    if (find) {
        g_proven.For(soldier);
        g_routeTrust.For(soldier);
        if (g_pitchTriedFor != soldier)
            g_pitchTriedFor = soldier, g_pitchTried.clear();
        g_swept.For(soldier);
        g_reachTried.For(soldier);
    }
    // With find, a level view (the pitch a zero) while mouse aim steers it: no pair could be nudged (Proves never nudges a zero, the slot the game reads
    // among them), so nothing is held for the quick tries either, and it's looked at again in 0.5 s (below). Their 60 ms hold came first before, at every
    // level search.
    const bool levelNow = steering && camera::GameWeaponAngles(cameraYaw, cameraPitch) && std::fabs(cameraPitch) <= 0.02f;
    // What proved itself for this soldier first, then the last route: its pair matching the camera and proving itself, nothing else to search. With mouse aim holding the camera still
    // for it: read as the hand moved it, the pair and the camera never agreed to the 0.6 degrees asked (they're a game tick apart), and the route failed for a minute at a time.
    const uintptr_t known = g_provenFor.load() == soldier ? g_provenAddress.load() : 0;
    const uintptr_t byRoute = g_haveLastRoute ? Follow(soldier, g_lastRoute) : 0;
    bool likely = known != 0;  // worth waiting for: the known one, or the route's where it leads to a look like the camera's
    if (find) {
        // With find: every address that proved itself for this soldier, those proved with the gun now in hand first, each kind newest first (a gun's from
        // before a take, back in hand with key 1 or 2), then the route's where it leads elsewhere, inside one hold; each counts for waiting by its own
        // record (aimrules::KnownLikely, RouteLikely, StrikesAfter): after a take the old address was retried for 15 s, every 400 ms, the whole search
        // waiting (the user's try, 10:39:41 to 10:39:59), and after a switch the other gun's (the user's play of 2026-11-19, the Mk11's after the switch
        // to the AT4, 01:10:42 to 01:10:57).
        uintptr_t tries[aimrules::kProvenMost + 1] = {};
        int entryOf[aimrules::kProvenMost + 1] = {};  // each try's entry in g_proven (-1 the route's)
        int order[aimrules::kProvenMost] = {};
        int count = g_proven.Order(weaponAtStart, order), routeAt = -1;
        for (int k = 0; k < count; ++k)
            tries[k] = g_proven.entry[order[k]].address, entryOf[k] = order[k];
        const int knownCount = count;
        if (byRoute && std::find(tries, tries + count, byRoute) == tries + count)
            routeAt = count, entryOf[count] = -1, tries[count++] = byRoute;
        likely = false;
        if (count && !levelNow) {
            const PassHold held;
            Sleep(kCameraSettles);
            const bool haveCamera = camera::GameWeaponAngles(cameraYaw, cameraPitch);
            for (int n = 0; n < count && !found; ++n) {
                const bool route = n == routeAt;
                float pair[4] = {}, apart = 1e9f;  // (unread: as far off as can be)
                float offset = 0.0f;  // the vehicle's turn as its proof measures it (Proves)
                aimrules::Answer answer = aimrules::Answer::kNone;
                if (haveCamera && camera::SafeRead(tries[n], pair, sizeof(pair)) && LooksLikeAngle(pair[0], false) && LooksLikeAngle(pair[3], true)) {
                    apart = std::fabs(pair[3] - cameraPitch);
                    if (apart < 0.01f) {
                        switch (Proves(tries[n], true, true, &offset)) {
                        case Proof::kYes:
                            answer = aimrules::Answer::kYes;
                            break;
                        case Proof::kKept:
                            answer = aimrules::Answer::kKept;
                            break;
                        case Proof::kBusy:
                            answer = aimrules::Answer::kBusy;
                            break;
                        case Proof::kOverwritten:
                            answer = aimrules::Answer::kOverwritten;
                            break;
                        default:
                            answer = aimrules::Answer::kNo;
                            break;
                        }
                    } else if (apart < aimrules::kLikelyPitch) {
                        answer = aimrules::Answer::kNearNotMatching;
                    }
                }
                if (route) {
                    g_routeTrust.strikes = aimrules::StrikesAfter(g_routeTrust.strikes, answer, true);
                    likely = likely || aimrules::RouteLikely(g_routeTrust.provenOn == soldier, g_routeTrust.strikes, apart);
                } else {
                    // (The first in the order is the newest proved with the gun in hand, or with none read, the newest: it waits whatever its pitch.)
                    aimrules::Proven::Entry& entry = g_proven.entry[entryOf[n]];
                    entry.strikes = aimrules::StrikesAfter(entry.strikes, answer, false);
                    likely = likely || aimrules::KnownLikely(n == 0, entry.swap != swapAtStart, entry.strikes, apart,
                                                             aimrules::Proven::OtherWeapon(entry.weapon, weaponAtStart));
                }
                if (answer == aimrules::Answer::kYes) {
                    found = tries[n], kept = 1, candidates = 1;
                    // (The still camera less the pair read again, at one moment: the camera read above comes before every try ahead of this one, and
                    // a stick turn during them, sent as it is while the hold lasts, came out as a vehicle's turn on foot.)
                    g_yawOffset = offset;
                    if (route)
                        g_routeTrust.provenOn = soldier;
                    Log("Aim: the look angles found again by %s, at once (%.0f degrees off the camera's yaw)",
                        route ? "the last route" : entryOf[n] == 0 ? "the address that proved itself last" : "an address that proved itself before", g_yawOffset * kRadToDeg);
                }
            }
        } else if (knownCount) {
            // (Level: the first in the order still holds the search back as the newest did, whatever its pitch; not one proved with another gun.)
            const aimrules::Proven::Entry& entry = g_proven.entry[order[0]];
            likely = aimrules::KnownLikely(true, entry.swap != swapAtStart, entry.strikes, 1e9f, aimrules::Proven::OtherWeapon(entry.weapon, weaponAtStart));
        }
    } else if (known || byRoute) {
        const Proving held;
        Sleep(kCameraSettles);
        if (camera::GameWeaponAngles(cameraYaw, cameraPitch)) {
            const uintptr_t tries[2] = {known, byRoute == known ? 0 : byRoute};
            for (const uintptr_t at : tries) {
                float pair[4] = {};
                if (found || !at || !camera::SafeRead(at, pair, sizeof(pair)) || !LooksLikeAngle(pair[0], false) || !LooksLikeAngle(pair[3], true))
                    continue;
                likely = likely || std::fabs(pair[3] - cameraPitch) < 0.05f;
                // Its yaw needn't match the camera's: on a moving vehicle (The Great Destroyer's train) the look
                // yaw is the player's own, the camera's the vehicle's turn added (g_yawOffset).
                if (std::fabs(pair[3] - cameraPitch) < 0.01f && Proves(at, true) == Proof::kYes) {
                    found = at, kept = 1, candidates = 1;
                    g_yawOffset = VehicleTurn(std::remainder(cameraYaw - pair[0], 2.0f * kPi));
                    Log("Aim: the look angles found again by %s, at once (%.0f degrees off the camera's yaw)", at == known ? "the address that proved itself last" : "the last route",
                        g_yawOffset * kRadToDeg);
                }
            }
        }
    }
    // Waiting for the game to leave the camera be, or for a look that isn't level (nothing's nudged at a zero): only those, again in a moment. The whole search nudges a dozen candidates, 5 s
    // at a time; after kQuickFor it comes back, until something's found.
    if (found) {
        g_quickSince = 0;
    } else if (likely) {
        const ULONGLONG since = GetTickCount64();
        ULONGLONG first = g_quickSince.load();
        if (!first) {
            g_quickSince = since;
            first = since;
            Log("Aim: trying the last look angles again every %llu ms", kQuickAgain);
        }
        if (since - first < kQuickFor) {
            g_searchAgain = kQuickAgain;
            g_yawAddress = 0;
            g_foundFor = soldier;
            g_searching = false;
            return;
        }
    }
    // (With find, sooner for a while after the look was lost: aimrules::SearchAgainMs.)
    if (find) {
        const ULONGLONG lost = g_lostAt.load(), now = GetTickCount64();
        g_searchAgain = lost ? aimrules::SearchAgainMs(now > lost ? now - lost : 0) : aimrules::kSearchLater;
    } else {
        g_searchAgain = kSearchAgain;
    }
    if (!found && camera::GameWeaponAngles(cameraYaw, cameraPitch) && std::fabs(cameraPitch) > 0.02f &&
        std::fabs(cameraYaw) > 0.02f) {
        std::vector<Route> routes;
        std::vector<uintptr_t> pairs;
        std::optional<PassHold> held;  // (with find: the whole pass's)
        ULONGLONG extraStart = 0;      // (with find: when the first proof past the first pass's began, the sweep's or the reach's)
        if (!find) {
            pairs = Pairs(soldier, cameraYaw, cameraPitch, &routes);
        } else {
            // With find: the walk compares each window with the camera read just after it, mouse aim steering meanwhile (a pair read 200 ms into the
            // walk was kept only if the look turned under 2.5 degrees a second, and a gun in the headset never stands that still: 48 to 83 a second in
            // the user's try); then, mouse aim held and settled, its list is read again at one instant and nudged inside that one hold.
            std::vector<Route> nearRoutes;
            const std::vector<uintptr_t> nearList = Pairs(soldier, cameraYaw, cameraPitch, &nearRoutes, false, true, &nearFull);
            nearCount = nearList.size();
            if (!nearList.empty()) {
                held.emplace();
                Sleep(kCameraSettles);
                if (camera::GameWeaponAngles(cameraYaw, cameraPitch) && std::fabs(cameraPitch) > 0.02f && std::fabs(cameraYaw) > 0.02f)
                    pairs = Matching(nearList, nearRoutes, cameraYaw, cameraPitch, false, &routes, &shaped);
            }
        }
        candidates = pairs.size();
        crowded = candidates > kMostPairs;
        for (size_t k = 0; k < pairs.size() && !crowded && !found; ++k) {
            if (find)
                firstTried.push_back(pairs[k]);
            const Proof answer = Proves(pairs[k], false, find);
            if (find)
                tried.push_back({pairs[k], shaped >= 0 && k < static_cast<size_t>(shaped), answer});  // (Matching: the slot-shaped first)
            switch (answer) {
            case Proof::kYes:
                found = pairs[k], ++kept;
                if (k < routes.size())
                    g_lastRoute = routes[k], g_haveLastRoute = true, SaveRoute(), g_routeTrust.provenOn = soldier;
                break;
            case Proof::kKept:
                ++kept;
                break;
            case Proof::kBusy:
                ++busy;
                break;
            default:
                break;
            }
        }
        // With find, nothing found and slot-shaped candidates written over (the look's copies, the game rebuilding each from the slot it reads, which
        // the walk didn't reach: 2 of 7 launches at one checkpoint, 2026-11-18, the same three copies every 3 s for the whole launch): the row is looked
        // through from each of them (aimrules::SweepChoice: the row's shape, the camera's angles as read now, no zero, not a candidate already, not
        // swept before for this soldier) and what's there proved as any candidate, inside the same hold, kSweepMost at most. Nothing is written but
        // Proves' nudge, undone as ever; only a proof counts. No route is saved for it (none leads there): g_proven tries it first from here on.
        uintptr_t copies[kMostPairs] = {};
        {
            std::vector<aimrules::Candidate> answered;
            for (const Tried& t : tried)
                answered.push_back({t.at, t.slot, t.answer == Proof::kOverwritten});
            sweptCopies = aimrules::SweepCopies(answered.data(), static_cast<int>(answered.size()), copies, static_cast<int>(kMostPairs));
            copies0 = copies[0];
        }
        if (aimrules::SweepWanted(find, found != 0, crowded, sweptCopies) && held && camera::GameWeaponAngles(cameraYaw, cameraPitch) &&
            std::fabs(cameraPitch) > 0.02f && std::fabs(cameraYaw) > 0.02f) {
            swept = true;
            sweptTaken = aimrules::SweepChoice(
                copies, sweptCopies, cameraYaw, cameraPitch,
                [](uintptr_t at, uint32_t* words) { return camera::SafeRead(at - 0xC, words, 7 * sizeof(uint32_t)); },
                // (A candidate of this pass had its answer above: not taken, and not counted as tried before, so "0 tried before" is the slot out of reach.)
                [&](uintptr_t at) { return std::find(pairs.begin(), pairs.end(), at) != pairs.end(); }, [&](uintptr_t at) { return g_swept.Has(at); }, sweptAt,
                aimrules::kSweepMost, &sweptBefore);
            // (Every one tried and none proved: from the first again only after kSweepRest such searches, none of which holds mouse aim for the sweep.)
            g_swept.Chosen(sweptTaken, sweptBefore);
            // The proofs hold mouse aim on top of the first pass's: no more of them begun past kPitchHoldMost (aimrules::SweepGoesOn), the rest taken
            // again by the next search. (Proves' measure of the vehicle's turn isn't asked for: these lie at the camera's own yaw, so none.)
            const ULONGLONG sweepStart = GetTickCount64();
            for (int n = 0; n < sweptTaken && n < aimrules::kSweepMost && !found && aimrules::SweepGoesOn(GetTickCount64() - sweepStart, n); ++n) {
                if (!n)
                    extraStart = sweepStart;
                sweptAnswer[n] = Proves(sweptAt[n].at, false, true);
                sweptProved = n + 1;
                g_swept.Answered(sweptAt[n].at, AnswerOf(sweptAnswer[n]));
                switch (sweptAnswer[n]) {
                case Proof::kYes:
                    found = sweptAt[n].at, ++kept;
                    break;
                case Proof::kBusy:
                    ++busy;  // (tried again in the next search)
                    break;
                case Proof::kKept:
                    ++kept;
                    break;
                default:
                    break;
                }
            }
        }
        // With find, still nothing: the reach (aimrules.h: ReachWanted, ReachSpans, ReachThrough, ReachChoice), for a look beyond the walk and the sweep
        // (the M249's of 2026-11-18 22:04, 0x1580 from its copies, 43 searches; the user's of 2026-11-19, the M4A1's 1.9 MB and the M249's 3.4 MB from them,
        // a minute and 27). The memory within 16 MB of this search's copies (its candidates and swept places written over) and of the looks proved earlier
        // in this launch, nearest first, 25 ms at most, looked through at each 16-byte line for the look at the camera's angles as read now (a pointer before
        // it alone only where the copies lack the row's shape: aimrules::ReachPointerAlone), mouse aim held and settled (held here if the walk brought
        // nothing to hold for); what it takes is proved as any candidate, the row's shape first, each kind nearest an anchor first, within the sweep's time
        // and kSweepMost a search (the rest, and those tried before, as the sweep's). Nothing is written but Proves' nudge, undone as ever; only a proof
        // counts. No route is saved for it (none leads there): g_proven tries it first from here on.
        if (find && !found && !crowded) {
            uintptr_t anchors[aimrules::kReachAnchors] = {};
            const auto anchor = [&](uintptr_t at) {
                if (at && reachAnchors < aimrules::kReachAnchors && std::find(anchors, anchors + reachAnchors, at) == anchors + reachAnchors)
                    anchors[reachAnchors++] = at;
            };
            for (const Tried& t : tried)
                if (t.answer == Proof::kOverwritten) {
                    anchor(t.at);
                    ++reachCopies, reachShapedCopies += t.slot;
                }
            for (int n = 0; n < sweptProved; ++n)
                if (sweptAnswer[n] == Proof::kOverwritten) {
                    anchor(sweptAt[n].at);
                    ++reachCopies, ++reachShapedCopies;  // (the sweep takes the row's shape alone)
                }
            for (const uintptr_t at : g_provenEver)
                anchor(at);
            reachPointerAlone = aimrules::ReachPointerAlone(reachCopies, reachShapedCopies);
            if (aimrules::ReachWanted(find, found != 0, crowded, reachAnchors)) {
                if (!held) {
                    held.emplace();
                    Sleep(kCameraSettles);
                }
                if (camera::GameWeaponAngles(cameraYaw, cameraPitch) && std::fabs(cameraPitch) > 0.02f && std::fabs(cameraYaw) > 0.02f) {
                    reached = true;
                    reachRun = ReachLook(anchors, reachAnchors, cameraYaw, cameraPitch, reachPointerAlone);
                    const auto sweptHere = [&](uintptr_t at) {
                        for (int n = 0; n < sweptProved; ++n)
                            if (sweptAt[n].at == at)
                                return true;
                        return false;
                    };
                    reachTaken = aimrules::ReachChoice(
                        reachRun.places.data(), static_cast<int>(reachRun.places.size()), anchors, reachAnchors,
                        // (A candidate of this pass, or a place the sweep proved in it, had its answer: not taken, and not counted as tried before.)
                        [&](uintptr_t at) { return std::find(pairs.begin(), pairs.end(), at) != pairs.end() || sweptHere(at); },
                        [&](uintptr_t at) { return g_reachTried.Has(at) || g_swept.Has(at); }, reachAt, aimrules::kSweepMost, &reachBefore, &reachCrowd,
                        reachRun.totals.counts);
                    g_reachTried.Chosen(reachTaken, reachBefore);
                    const ULONGLONG reachStart = extraStart ? extraStart : GetTickCount64();
                    for (int n = 0; n < reachTaken && n < aimrules::kSweepMost && !found &&
                                    aimrules::SweepGoesOn(GetTickCount64() - reachStart, sweptProved + n);
                         ++n) {
                        reachAnswer[n] = Proves(reachAt[n].at, false, true);
                        reachProved = n + 1;
                        g_reachTried.Answered(reachAt[n].at, AnswerOf(reachAnswer[n]));
                        switch (reachAnswer[n]) {
                        case Proof::kYes:
                            found = reachAt[n].at, ++kept;
                            break;
                        case Proof::kBusy:
                            ++busy;  // (tried again in the next search)
                            break;
                        case Proof::kKept:
                            ++kept;
                            break;
                        default:
                            break;
                        }
                    }
                }
            }
        }
        if (found)
            g_yawOffset = 0.0f;
    }
    // Then by the pitch alone (the yaw any): on the moving train the look yaw is the player's own, not the
    // camera's, and no pair matching the camera's ever kept a nudge (the user's shots missed, 16:10). Only
    // with the view clearly up or down (more stray floats match a small pitch; nudging strays crashed the
    // game once) and at most kMostPairs of them.
    if (!found && !crowded && camera::GameWeaponAngles(cameraYaw, cameraPitch) && std::fabs(cameraPitch) > 0.1f && !find) {
        std::vector<Route> routes;
        const std::vector<uintptr_t> pairs = PitchPairs(soldier, cameraPitch, &routes);
        for (size_t k = 0; k < pairs.size() && k < kMostPairs && !found; ++k) {
            float yaw = 0.0f;
            ++byPitch;
            if (Proves(pairs[k], true) == Proof::kYes && camera::SafeRead(pairs[k], &yaw, sizeof(yaw))) {
                found = pairs[k], ++kept;
                g_yawOffset = VehicleTurn(std::remainder(cameraYaw - yaw, 2.0f * kPi));
                if (k < routes.size())
                    g_lastRoute = routes[k], g_haveLastRoute = true, SaveRoute();
                Log("Aim: the look angles found by the pitch alone, %.0f degrees off the camera's yaw (a vehicle's turn)",
                    g_yawOffset * kRadToDeg);
            }
        }
    } else if (!found && !crowded && camera::GameWeaponAngles(cameraYaw, cameraPitch) && std::fabs(cameraPitch) > 0.1f && find &&
               !aimrules::PitchPassWanted(shaped, crowded, true)) {
        pitchSkipped = true;
    } else if (!found && !crowded && camera::GameWeaponAngles(cameraYaw, cameraPitch) && std::fabs(cameraPitch) > 0.1f && find) {
        // With find: as the first pass (the walk against the camera at each window, one hold, the pairs at one instant), not where the first pass saw the
        // look's copies at the camera's own yaw (no vehicle turns it there), the slot-shaped first, those tried lately left for later, at most
        // kPitchHoldMost of mouse aim held in one search (the rest in the next), and the vehicle's turn measured at the proof (aimrules::ProofTurn).
        std::vector<Route> nearRoutes, routes;
        const std::vector<uintptr_t> nearList = Pairs(soldier, 0.0f, cameraPitch, &nearRoutes, true, true, &pitchFull, aimrules::kStrict);
        pitchNearCount = nearList.size();
        if (!nearList.empty()) {
            const PassHold held;
            Sleep(kCameraSettles);
            if (camera::GameWeaponAngles(cameraYaw, cameraPitch) && std::fabs(cameraPitch) > 0.1f) {
                const std::vector<uintptr_t> pairs = Matching(nearList, nearRoutes, cameraYaw, cameraPitch, true, &routes, nullptr);
                std::vector<size_t> order;
                for (int shapedFirst = 1; shapedFirst >= 0; --shapedFirst)
                    for (size_t k = 0; k < pairs.size(); ++k)
                        if (float y = 0.0f, p = 0.0f; SlotShaped(pairs[k], y, p) == (shapedFirst != 0) && !g_pitchTried.count(pairs[k]) &&
                                                       std::find(firstTried.begin(), firstTried.end(), pairs[k]) == firstTried.end())
                            order.push_back(k);
                if (order.empty())
                    g_pitchTried.clear();  // (every one tried: from the first again next time)
                const ULONGLONG passStart = GetTickCount64();
                int nudgedNow = 0;  // this pass's, in this search
                for (size_t n = 0; n < order.size() && !found; ++n) {
                    if (nudgedNow >= static_cast<int>(kMostPairs) || !aimrules::PitchPassGoesOn(GetTickCount64() - passStart, nudgedNow)) {
                        pitchLeft = static_cast<int>(order.size() - n);
                        break;
                    }
                    const size_t k = order[n];
                    float offset = 0.0f;
                    ++byPitch, ++nudgedNow;
                    const Proof answer = Proves(pairs[k], true, true, &offset);
                    if (answer == Proof::kYes) {
                        found = pairs[k], ++kept;
                        g_yawOffset = offset;
                        if (k < routes.size())
                            g_lastRoute = routes[k], g_haveLastRoute = true, SaveRoute(), g_routeTrust.provenOn = soldier;
                        Log("Aim: the look angles found by the pitch alone, %.0f degrees off the camera's yaw (a vehicle's turn)",
                            g_yawOffset * kRadToDeg);
                    } else if (answer != Proof::kBusy) {
                        g_pitchTried.insert(pairs[k]);
                    }
                }
            }
        }
    }
    const bool level = !found && camera::GameWeaponAngles(cameraYaw, cameraPitch) &&
                       (std::fabs(cameraPitch) <= 0.02f || std::fabs(cameraYaw) <= 0.02f);
    // With find, while mouse aim steers the game's view a turn of it is undone within a few frames, so it only threw the rounds fired meanwhile 6 degrees
    // off (19 times in the user's try, some 0.4 s apart): none, and looked at again sooner, a level view in 0.5 s (nothing walked or held), a crowded one
    // in 1 s.
    const bool quiet = steering && (level || crowded);
    if ((level || crowded) && !quiet)
        TiltView(true);
    if (quiet)
        g_searchAgain = level ? kLevelAgain : kCrowdedAgain;
    static uintptr_t s_logged = 1;
    static ULONGLONG s_quietSaid = 0;
    bool sayTook = true;
    if (found && found != s_logged) {
        Log("Aim: the look angles found at %08X (%zu pairs near the soldier matching the camera, %d kept a nudge)",
            static_cast<unsigned>(found), candidates, kept);
        // Its block's seven words as they stand now, from the 16-byte line before its yaw (the reach's: aimrules::ReachKind), to tell in the logs what
        // shape the look the game reads has when no candidate had the row's at that instant (46 of 128 finds of 2026-11-18/19).
        uint32_t words[7] = {};
        if (camera::SafeRead(found - 0xC, words, sizeof(words))) {
            float y = 0.0f, p = 0.0f;
            const bool row = aimrules::SlotWords(words, y, p);
            std::memcpy(&y, &words[3], 4);
            std::memcpy(&p, &words[6], 4);
            Log("Aim: its block from %08X: %08X %08X %08X, yaw %.4f, %08X %08X, pitch %.4f (%s)", static_cast<unsigned>(found - 0xC), words[0], words[1],
                words[2], y, words[4], words[5], p, row ? "the row's shape" : "not the row's shape");
        }
    } else if (level && quiet) {
        sayTook = GetTickCount64() - s_quietSaid >= 3000;  // (every 0.5 s otherwise)
        if (sayTook) {
            s_quietSaid = GetTickCount64();
            Log("Aim: the game's view is exactly level or straight ahead, which can't be told apart; looked at again in 0.5 s, the view not turned "
                "(mouse aim steers it, and would undo a turn at once)");
        }
    } else if (level)
        Log("Aim: the game's view is exactly level or straight ahead, which can't be told apart; turned a little to "
            "look again");
    else if (crowded && quiet)
        Log("Aim: %zu pairs near the soldier match the camera, too many to nudge safely; looked at again in 1 s, the view not turned (mouse aim steers "
            "it)",
            candidates);
    else if (crowded)
        Log("Aim: %zu pairs near the soldier match the camera, too many to nudge safely; turned a little to look again",
            candidates);
    else if (!found)
        Log("Aim: no look angles yet (%zu pairs near the soldier matching the camera, %d kept a nudge, none turned "
            "it, %d not tried with the camera moving); the view follows the head meanwhile",
            candidates, kept, busy);
    s_logged = found;
    // With find, what the lines above leave out, at a search that found nothing and at a sweep: each candidate of the first pass (its address, the
    // slot's shape or not, its answer; the first 8), then the row swept: each place proved, how far from which copy, its answer. The candidates' line
    // and the sweep's "nothing more there" only when they differ from the last said (a launch that never finds the slot said both every 3 s); a find
    // lets them be said again.
    static char s_candidatesSaid[768] = {}, s_nothingSaid[320] = {}, s_reachSaid[320] = {};
    if (found)
        s_candidatesSaid[0] = s_nothingSaid[0] = s_reachSaid[0] = 0;
    if (find && !tried.empty() && (!found || swept || reached)) {
        char line[768] = {};
        int used = 0;
        constexpr size_t kSaid = 8;
        for (size_t k = 0; k < tried.size() && k < kSaid && used >= 0 && used < static_cast<int>(sizeof(line)); ++k)
            used += std::snprintf(line + used, sizeof(line) - used, "%s%08X %s, %s", k ? "; " : "", static_cast<unsigned>(tried[k].at),
                                  tried[k].slot ? "slot-shaped" : "not a slot", ProofWords(tried[k].answer));
        if (tried.size() > kSaid && used >= 0 && used < static_cast<int>(sizeof(line)))
            std::snprintf(line + used, sizeof(line) - used, "; %zu more", tried.size() - kSaid);
        if (std::strcmp(line, s_candidatesSaid) != 0) {
            std::memcpy(s_candidatesSaid, line, sizeof(line));
            Log("Aim: the candidates: %s", line);
        }
    }
    if (swept) {
        char line[768] = {};
        int used = 0;
        const int proved = sweptProved;  // (those past a proof or past the hold's time aren't tried: left for the next search)
        for (int n = 0; n < proved && used >= 0 && used < static_cast<int>(sizeof(line)); ++n) {
            const long distance = sweptAt[n].Distance();
            used += std::snprintf(line + used, sizeof(line) - used, "%s%08X (%c0x%lX from %08X) %s", n ? "; " : "", static_cast<unsigned>(sweptAt[n].at),
                                  distance < 0 ? '-' : '+', static_cast<unsigned long>(std::labs(distance)), static_cast<unsigned>(sweptAt[n].copy),
                                  ProofWords(sweptAnswer[n]));
        }
        if (proved)
            Log("Aim: the row swept from %d written-over cop%s, 0x%X each way: %s%s (%d more left for the next search, %d tried before left out)",
                sweptCopies, sweptCopies == 1 ? "y" : "ies", static_cast<unsigned>(kSlotsEachWay * kSlotStep), line,
                sweptProved && sweptAnswer[sweptProved - 1] == Proof::kYes ? "; no route saved for it" : "", sweptTaken - proved, sweptBefore);
        else {
            char nothing[160] = {};
            if (sweptBefore)
                std::snprintf(nothing, sizeof(nothing), "%d tried before, tried again from the first every %d such searches", sweptBefore, aimrules::kSweepRest);
            else
                std::snprintf(nothing, sizeof(nothing), "0 tried before: the slot the game reads isn't within reach of these copies");
            char said[sizeof(s_nothingSaid)] = {};
            std::snprintf(said, sizeof(said), "%d %08X %s", sweptCopies, static_cast<unsigned>(copies0), nothing);
            if (std::strcmp(said, s_nothingSaid) != 0) {
                std::memcpy(s_nothingSaid, said, sizeof(said));
                Log("Aim: the row swept from %d written-over cop%s, 0x%X each way: nothing more there with the slot's shape at the camera's angles (%s)",
                    sweptCopies, sweptCopies == 1 ? "y" : "ies", static_cast<unsigned>(kSlotsEachWay * kSlotStep), nothing);
            }
        }
    }
    // The reach (with find): what it read, about how many anchors, in how long (microseconds), whether its time cut it short, what it left (pages out of
    // the working set, threads' stacks, memory read slowly); what it found at the camera's angles (the row's shape, a pointer alone, or why no pointer
    // alone was looked for); each place proved, how far from which anchor, its answer. Its "nothing to prove" only when it differs from the last said
    // (the M249's launch would have said it every 3 s); a find lets it be said again. A piece read slowly twice in a row is said each time.
    if (reached) {
        const aimrules::ReachTotals& totals = reachRun.totals;
        if (totals.slowNow)
            Log("Aim: the reach read %d piece%s of memory slowly twice in a row (the last in the allocation at %08X: not plain RAM, whatever the system calls "
                "it): %s left for the rest of this search, and for the launch at a second search so",
                totals.slowNow, totals.slowNow == 1 ? "" : "s", static_cast<unsigned>(totals.slowAt), totals.slowNow == 1 ? "its allocation" : "their allocations");
        char readWords[384] = {};
        {
            int used = std::snprintf(readWords, sizeof(readWords), "%.1f MB about %d anchor%s in %llu us%s (%d threads' stacks listed in %llu us)",
                                     static_cast<double>(totals.bytes) / (1024.0 * 1024.0), reachAnchors, reachAnchors == 1 ? "" : "s", totals.micros,
                                     totals.cut ? ", its time up: the farther memory left" : "", reachRun.threadStacks, reachRun.listMicros);
            if ((totals.notResident || totals.stacks || totals.slowLeft) && used >= 0 && used < static_cast<int>(sizeof(readWords)))
                std::snprintf(readWords + used, sizeof(readWords) - used, ", leaving %.1f MB out of the game's working set, %d thread stack%s, %d region%s read slowly before",
                              static_cast<double>(totals.notResident) / (1024.0 * 1024.0), totals.stacks, totals.stacks == 1 ? "" : "s", totals.slowLeft,
                              totals.slowLeft == 1 ? "" : "s");
        }
        char pointerWords[128] = {};
        if (reachPointerAlone)
            std::snprintf(pointerWords, sizeof(pointerWords), "%d with a pointer before them alone (the copies without the row's shape)", totals.counts[1]);
        else
            std::snprintf(pointerWords, sizeof(pointerWords), "none with a pointer alone looked for (%s)",
                          reachCopies ? "the copies have the row's shape" : "no copies written over in this search");
        char crowd[96] = {};
        if (reachCrowd & 2)
            std::snprintf(crowd, sizeof(crowd), "; too many with the row's shape: none nudged");
        else if (reachCrowd & 1)
            std::snprintf(crowd, sizeof(crowd), "; too many with a pointer alone: those not nudged");
        if (reachProved) {
            char line[768] = {};
            int used = 0;
            for (int n = 0; n < reachProved && used >= 0 && used < static_cast<int>(sizeof(line)); ++n) {
                const long distance = reachAt[n].Distance();
                used += std::snprintf(line + used, sizeof(line) - used, "%s%08X (%c0x%lX from %08X, %s) %s", n ? "; " : "", static_cast<unsigned>(reachAt[n].at),
                                      distance < 0 ? '-' : '+', static_cast<unsigned long>(std::labs(distance)), static_cast<unsigned>(reachAt[n].anchor),
                                      reachAt[n].kind == 2 ? "the row's shape" : "a pointer before it", ProofWords(reachAnswer[n]));
            }
            Log("Aim: the reach looked through %s: %d places at the camera's angles with the row's shape, %s%s; %s%s (%d more left for the next search, %d "
                "tried before left out)",
                readWords, totals.counts[2], pointerWords, crowd, line, reachAnswer[reachProved - 1] == Proof::kYes ? "; no route saved for it" : "",
                reachTaken - reachProved, reachBefore);
        } else {
            char said[sizeof(s_reachSaid)] = {};
            std::snprintf(said, sizeof(said), "%d %d %d %d %d %d %d", totals.counts[2], totals.counts[1], reachPointerAlone, reachBefore, reachCrowd, reachTaken,
                          totals.cut);
            if (std::strcmp(said, s_reachSaid) != 0) {
                std::memcpy(s_reachSaid, said, sizeof(said));
                Log("Aim: the reach looked through %s: %d places at the camera's angles with the row's shape, %s%s; nothing to prove (%d tried before left out%s)",
                    readWords, totals.counts[2], pointerWords, crowd, reachBefore,
                    reachTaken                                   ? ", the rest past the hold's time"
                    : reachBefore || (reachCrowd & 2)            ? ""
                    : totals.cut                                 ? ": the look the game reads isn't in what its time allowed"
                                                                 : ": the look the game reads isn't within 16 MB of them");
            }
        }
    }
    // How long the search took, and how long mouse aim held its corrections for the nudges: most of each search
    // in the interrogation room (2026-10-31: 3.5-4.4 s searches back to back, the view tilted, so the pitch-alone
    // pass nudged up to 16 candidates each time).
    if (!find) {
        Log("Aim: the search took %llu ms; mouse aim held its corrections %llu ms for %d candidates (%d by the pitch "
            "alone)",
            GetTickCount64() - searchStart, g_heldMs, g_nudged, byPitch);
    } else if (sayTook) {
        // (With find: what the walk saw, what the reach read, and what the pass by the pitch alone left for the next search.)
        char tail[384] = {};
        int used = 0;
        if (nearCount)
            used += std::snprintf(tail + used, sizeof(tail) - used, "; %zu near the camera in the walk%s", nearCount, nearFull ? " (its list full)" : "");
        if (shaped >= 0 && used >= 0 && used < static_cast<int>(sizeof(tail)))
            used += std::snprintf(tail + used, sizeof(tail) - used, ", %d of those at one instant slot-shaped", shaped);
        if (reached && used >= 0 && used < static_cast<int>(sizeof(tail)))
            used += std::snprintf(tail + used, sizeof(tail) - used, "; the reach %.1f MB in %llu us%s, %d places, %d proved",
                                  static_cast<double>(reachRun.totals.bytes) / (1024.0 * 1024.0), reachRun.totals.micros, reachRun.totals.cut ? " (its time up)" : "",
                                  reachRun.totals.counts[1] + reachRun.totals.counts[2], reachProved);
        if (pitchNearCount && used >= 0 && used < static_cast<int>(sizeof(tail)))
            used += std::snprintf(tail + used, sizeof(tail) - used, "; %zu by the pitch alone in its walk%s", pitchNearCount, pitchFull ? " (its list full)" : "");
        if (pitchLeft && used >= 0 && used < static_cast<int>(sizeof(tail)))
            std::snprintf(tail + used, sizeof(tail) - used, ", %d of them left for the next search", pitchLeft);
        else if (pitchSkipped && used >= 0 && used < static_cast<int>(sizeof(tail)))
            std::snprintf(tail + used, sizeof(tail) - used, "; none by the pitch alone (the look's copies at the camera's yaw)");
        Log("Aim: the search took %llu ms; mouse aim held its corrections %llu ms for %d candidates (%d by the pitch "
            "alone)%s",
            GetTickCount64() - searchStart, g_heldMs, g_nudged, byPitch, tail);
    }
    for (auto& slot : g_slots)  // this soldier's row, not the last's
        slot = 0;
    if (found) {
        Log("Aim: %d more slots of the look angles found beside them, written with them", FindSlots(found));
        g_provenAddress = found;
        g_provenFor = soldier;
        // (Kept whatever "aimfind" says, so it can change at any time. The take as at this search's start: a find made across one counts from before it.
        // The gun its proof counts for, as the proof began and ended: a switch landing between the search's start and the proof (the user's play of
        // 2026-11-19 01:09:11.87 to 01:09:12.679) is the new gun's; one inside the proof, no gun's for sure: aimrules::ProvedWith.)
        if (g_provedWith == aimrules::kWeaponUnsure)
            Log("Aim: the gun in hand changed while the look angles were proved: kept with no gun (never waited for after a switch)");
        g_proven.Remember(soldier, found, swapAtStart, g_provedWith);
        g_routeTrust.For(soldier);
        g_routeTrust.strikes = 0;
        g_pitchTried.clear();
        g_swept.Found();
        g_reachTried.Found();
        NoteProvenEver(found);
        g_findCount.fetch_add(1);
    }
    g_yawAddress = found;
    g_foundFor = soldier;
    g_searching = false;
}

// Where to write the angles, if they're known for the player's current soldier and still match the
// camera (a vehicle, a cutscene or a load can leave them: after a second of not matching, they're
// looked for again); otherwise a search starts and nothing is written meanwhile.
bool Targets(uintptr_t& yawAddress, uintptr_t& pitchAddress) {
    const uintptr_t soldier = god::PlayerClientSoldier();
    const uintptr_t found = g_yawAddress.load();
    const ULONGLONG now = GetTickCount64();
    g_swapSerial.store(pickups::NewestSwap(), std::memory_order_relaxed);  // (this frame's copy, the render thread's: for the search's records)
    {
        // The gun in hand, the game's own current weapon's object (ammo.h, this frame's read; 0 none read): kept with what the search proves.
        uint32_t weapon = 0, blueprint = 0;
        const int slot = ammo::GameSlot();
        g_weaponKey.store(slot >= 0 && ammo::SlotIdentity(slot, weapon, blueprint) ? weapon : 0, std::memory_order_relaxed);
    }
    float yaw = 0.0f, pitch = 0.0f, cameraYaw = 0.0f, cameraPitch = 0.0f;
    if (soldier && soldier == g_foundFor.load() && found && camera::SafeRead(found, &yaw, sizeof(yaw)) &&
        camera::SafeRead(found + 0xC, &pitch, sizeof(pitch)) && LooksLikeAngle(yaw, false) &&
        LooksLikeAngle(pitch, true)) {
        // Yaw and pitch both (a wrong pair's yaw stayed near the camera's for 16 s by chance while
        // its pitch never did, 2026-10-30).
        // The yaw with the vehicle's turn added (g_yawOffset: none on foot), which is followed slowly so
        // the train's curves keep it matching.
        const bool haveCamera = camera::GameWeaponAngles(cameraYaw, cameraPitch);
        // And what was written, not only what's there: the game copies its own over a slot it doesn't read, so that always matches.
        // (The camera is up to a tick behind the hand, more room for that.)
        const bool followed = !haveCamera || !g_haveTarget || g_targetIsMouse || g_scripted || YawApart(g_target + g_yawOffset, cameraYaw) < 0.35f;
        const bool matching =
            !haveCamera || (followed && YawApart(yaw + g_yawOffset, cameraYaw) < 0.2f && std::fabs(pitch - cameraPitch) < 0.12f);
        // The camera is up to a game tick behind the look while the hand moves, and that isn't a vehicle's turn: only a little of it counts until the look has been still a moment.
        // At 0.05 all the time the picture took it (LookYaw), the whole world stepping 0.05-0.5 degrees a frame as the gun moved, and sliding back after.
        static float s_yawSeen = 0.0f;
        static int s_yawStill = 0;
        s_yawStill = YawApart(yaw, s_yawSeen) < kLookStill ? std::min(s_yawStill + 1, 100) : 0;
        s_yawSeen = yaw;
        if (haveCamera && matching && g_yawOffset != 0.0f)
            g_yawOffset += (s_yawStill >= kLookStillFrames ? 0.05f : 0.005f) * std::remainder(cameraYaw - yaw - g_yawOffset, 2.0f * kPi);
        if (matching)
            g_mismatchSince = 0;
        else if (!g_mismatchSince)
            g_mismatchSince = now;
        if (!g_mismatchSince || now - g_mismatchSince < 1000) {
            yawAddress = found;
            pitchAddress = found + 0xC;
            // The row isn't whole yet (looking level when it was searched, a slot not filled in then): a slot the game may read and nothing writes loses the aim on a weapon switch.
            if (static ULONGLONG s_triedAt = 0; SlotCount() < 4 && now - s_triedAt > 1000) {
                s_triedAt = now;
                const int before = SlotCount(), after = FindSlots(found);
                if (after != before)
                    Log("Aim: %d more slots of the look angles found since, %d in all", after - before, after);
            }
            return true;
        }
        Log("Aim: the look angles at %08X stopped matching the weapon camera; looking again",
            static_cast<unsigned>(found));
        g_yawAddress = 0;
        g_mismatchSince = 0;
        g_lostAt = now;
    }
    if (soldier && !g_searching.load() && (soldier != g_foundFor.load() || now - g_searchedAt > g_searchAgain.load())) {
        if (soldier != g_foundFor.load())
            g_lostAt = now;  // (a new soldier, or the first search after a cutscene: Forget)
        g_searchedAt = now;
        g_searching = true;
        g_yawAddress = 0;
        std::thread(Search, soldier).detach();
    }
    return false;
}

void SetTo(uintptr_t address, float value) {
    if (SafeWrite(address, value))
        ++g_writes;
}


}

namespace aim {

// Mouse aim, while the look angles aren't known (after a load, on The Great Destroyer's moving train,
// where the search failed for minutes and a knife-rushing enemy killed the user before the gun could
// aim, 16:17): the game's view steered with the mouse towards where the hand points, closing half the
// gap each frame from the weapon camera's angles as the game made them. Nothing in memory is searched
// or written, so it works at once. The picture takes the hand's turn out by the yaw aimed for
// (g_target, LookYaw) as it does with the angles written. Off without the game's window in front and
// with weapon.txt "mouseaim 0". While the search nudges a candidate (g_proving) only its corrections
// wait (they would undo the nudge): the aim and the picture go on, and the stick's turn is sent as it is.
std::atomic<bool> g_mouseAimOn{true};
bool g_mouseHave = false;
float g_mouseBase = 0.0f;   // the game yaw with the hand's yaw at 0 and no stick turn
float g_mouseTurned = 0.0f;
constexpr float kMousePixelsPerRadian = 230.0f;  // about 4 a degree (TiltView's 24 for some 6)
float g_mouseCarry = 0.0f;  // during a nudge: the stick's turn not yet sent (a fraction of a count)
bool g_mouseHeld = false;   // this frame's corrections held for a nudge (Nudging)
bool g_mousePaused = false;  // it waited since it last steered: centred again when it steers next
// With find, its last step on each axis (aimrules::MouseStep): the half-gap step alone leaves up to 2 counts (half a degree) of the aim standing.
aimrules::MouseAxis g_mouseAxes[2];

bool MouseAim(float headYaw, float headPitch, float turn, float sign, const float* body, bool lookedLast) {
    g_mouseHeld = false;
    float cameraYaw = 0.0f, cameraPitch = 0.0f;
    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    if (!g_mouseAimOn.load(std::memory_order_relaxed) || pid != GetCurrentProcessId() ||
        !camera::GameWeaponAngles(cameraYaw, cameraPitch)) {
        g_mousePaused = true;
        return false;
    }
    const bool nudging = g_proving.load() || g_passHeld.load();
    // Centred afresh for a new soldier (a respawn, a load), after the look angles were written, and after a pause.
    static uintptr_t s_soldier = 0;
    const uintptr_t soldier = god::PlayerClientSoldier();
    bool newSoldier = false;
    if (soldier != s_soldier) {
        s_soldier = soldier;
        g_mouseHave = false;
        newSoldier = true;
    }
    if (!g_mouseHave || g_mousePaused) {
        // From the picture's body (Update's body): the yaw aimed for takes the hand's turn out of it, so the view
        // goes on where it was and the game's camera turns to the gun. Its centring kept through a pause instead,
        // whatever turned the game's camera meanwhile (the stick, through the mouse thread) was undone and the view
        // jumped with it (The Great Destroyer, 18:31:16 and 18:32:27: 112 and 136 degrees); centred on the game's
        // camera, the hand's turn meanwhile jumped it (16:21). The game's camera only after a recenter, or for a
        // new soldier while the look went on (a load: its facing taken), and never a nudged one.
        const bool fromBody = body && !g_reanchor && !(newSoldier && lookedLast);
        if (nudging && !fromBody)
            return false;  // never centred on a nudged camera: paused, as it always was through a nudge
        g_mouseBase = fromBody ? *body - (1.0f + sign) * headYaw : cameraYaw - sign * headYaw;
        g_mouseTurned = 0.0f;
        g_yawOffset = 0.0f;  // the camera's own yaw here, no vehicle's turn to add
        g_reanchor = false;
        if (!g_mouseHave)
            Log("Aim: the view steered with the mouse until the look angles are found%s",
                fromBody ? " (from the picture's turn)" : "");
        g_mouseHave = true;
        g_mousePaused = false;
    }
    g_mouseTurned += turn;
    const float yaw = g_mouseBase + sign * (headYaw + g_mouseTurned);
    INPUT in{};
    in.type = INPUT_MOUSE;
    in.mi.dwFlags = MOUSEEVENTF_MOVE;
    if (nudging) {
        // A candidate being nudged (Proves): no correction, which would undo the nudge, but the aim and the
        // picture (LookYaw) go on, and the stick's own turn is sent as input.cpp's mouse thread sent it while
        // mouse aim paused. Paused outright (returning false), the picture switched to the game's camera for
        // each nudge and back after, undoing the stick's turn made meanwhile in one frame: the interrogation
        // room's "teleporting" (2026-10-31 18:07, 75-86% of the stick-turning frames during nudges; 112
        // degrees in one frame at 18:31:16, 162 and 180 on the train at 18:42).
        g_mouseCarry += sign * turn * kMousePixelsPerRadian;
        in.mi.dx = static_cast<LONG>(g_mouseCarry);
        g_mouseCarry -= static_cast<float>(in.mi.dx);
        g_mouseHeld = true;
    } else {
        g_mouseCarry = 0.0f;
        float pitch = std::clamp(headPitch, -1.4f, 1.4f);
        // With find, a head held level kept the game's view level, and a level view is never searched (its pitch is among memory's zeros) nor turned
        // (mouse aim would undo it): with the headset on a desk or the stand-in's films nothing else moved it, and the look angles were never found
        // (the self-test of 2026-11-13 16:32, 90 s; the old way, which turned the view, found them in 24 s). While they aren't found, a head within
        // kLevelOff of level has the game's view held kLevelOff under level, out of that band, so the search goes on. The same for a gun straight ahead
        // (the yaw a zero: 97 searches "straight ahead" in 358 s of the shell fix's run of 2026-11-18) and for both small together, where memory is full
        // of pairs like the look (593 of 708 searches "too many to nudge" that day): aimrules::HoldOffZeros. Only the steering: the yaw aimed for
        // (g_target, LookYaw) stays the gun's, so the picture, which takes the camera's own yaw against it, never shows the hold.
        float steerYaw = yaw;
        if (g_findOn.load(std::memory_order_relaxed))
            aimrules::HoldOffZeros(steerYaw, pitch);
        const float yawError = std::remainder(steerYaw - cameraYaw, 2.0f * kPi), pitchError = pitch - cameraPitch;
        // (Half the gap, cut toward zero, as ever; with find, the last step too. A gap not finite sends nothing: its cast was undefined.)
        const bool lastStep = g_findOn.load(std::memory_order_relaxed);
        in.mi.dx = aimrules::MouseStep(yawError, kMousePixelsPerRadian, g_mouseAxes[0], lastStep);
        in.mi.dy = aimrules::MouseStep(-pitchError, kMousePixelsPerRadian, g_mouseAxes[1], lastStep);
    }
    if (in.mi.dx || in.mi.dy)
        SendInput(1, &in, sizeof(in));
    g_target = yaw - 2 * kPi * std::floor(yaw / (2 * kPi));
    g_haveTarget = true;
    g_targetIsMouse = true;
    return true;
}

void SetMouseAim(bool on) {
    if (g_mouseAimOn.exchange(on) != on)
        Log("Aim: mouse aim %s", on ? "on (until the look angles are found)" : "off");
}

void SetFind(bool on) {
    if (g_findOn.exchange(on) != on)
        Log("Aim: the look angles' search %s", on ? "against the camera at each window, a hold a pass, each pair as it is after its wait; mouse aim's last step; "
                                                    "the pitch limits tried again (aimfind 1)"
                                                  : "as before (aimfind 0)");
}

bool Nudging() {
    return g_mouseHeld && g_mouseHave;
}

bool Update(float headYaw, float headPitch, float turn, const float* body) {
    // A frame that writes no look yaw (mouse aim, a scripted moment, the angles not found or not readable) ends the run of written yaws (WrittenYaws).
    struct Written {
        bool one = false;
        ~Written() {
            if (!one)
                DropWritten();
        }
    } written;
    const bool lookedLast = g_lookedLast;
    g_lookedLast = false;
    config::Aim a = config::GetAim();
    if (DWORD pid = 0; g_findOn.load(std::memory_order_relaxed)) {  // (before Targets, which may start a search: what it finds mouse aim doing)
        GetWindowThreadProcessId(GetForegroundWindow(), &pid);
        g_mouseSteering = g_mouseAimOn.load(std::memory_order_relaxed) && pid == GetCurrentProcessId();
    }
    uintptr_t yawAddress = 0, pitchAddress = 0;
    if (!Targets(yawAddress, pitchAddress)) {
        g_havePrevious = false;  // found again: start from there, no jump
        g_haveTarget = false;
        g_havePitch = false;
        // A scripted moment can only be seen ending through the look angles: without them it stayed on and
        // kept the free hands off for 1.5 minutes after the dumpster (the user, 18:33). Over once the quiet has
        // lasted as long as it would have to with them.
        if (g_scripted && GetTickCount64() - g_scriptedMovedAt >= kScriptedQuiet) {
            g_scripted = false;
            Log("Aim: the scripted moment is over (%.1f s without seeing the game turn the look; its angles not found)",
                kScriptedQuiet / 1000.0f);
        }
        g_lookedLast = MouseAim(headYaw, headPitch, turn, a.invertYaw ? 1.0f : -1.0f, body, lookedLast);
        return g_lookedLast;
    }
    g_mouseHave = false;
    // The hand's yaw is OpenXR's (growing as it turns left); the game's look yaw grows turning
    // right: it goes in negated. Measured 2026-10-30 (weapon.txt "viewlog", the user sweeping the
    // gun with the head still): taken as it came, the game aimed the other way from the gun and
    // the picture, taking the hand's turn back out, swung twice the gun's turn ("the camera moves
    // with the gun"). The stick's turn (input::TakeTurn) is OpenXR's way too, left positive: it
    // goes in negated with it (left as it came, the smooth turn ran backwards: user, the same
    // evening). ([aim] invert_yaw undoes both.)
    const float sign = a.invertYaw ? 1.0f : -1.0f;
    float now = 0.0f;
    if (!camera::SafeRead(yawAddress, &now, sizeof(now))) {
        g_haveTarget = false;
        return false;
    }
    const ULONGLONG tick = GetTickCount64();
    if (g_scripted) {
        float moved = now - g_scriptedSeen;
        moved -= 2 * kPi * std::round(moved / (2 * kPi));
        g_scriptedSeen = now;
        if (std::fabs(moved) > 0.0005f) {
            g_scriptedMovedAt = tick;
            ++g_gameMoves;
            g_gameMoveMost = std::max(g_gameMoveMost, std::fabs(moved));
        }
        if (tick - g_scriptedMovedAt < kScriptedQuiet && g_scriptedOn.load(std::memory_order_relaxed))
            return false;
        g_scripted = false;
        g_havePrevious = false;  // from where the game left it, no jump
        Log("Aim: the game left the look yaw alone for %.1f s: the head aims again", kScriptedQuiet / 1000.0f);
        // With a gun in hand it may have been the game reading a slot nothing writes, which from here looks the same: the angles looked for again. (It proves whichever slot the
        // game reads, about 0.4 s by the last route, the mouse aim steering meanwhile, and such moments are rare: with every slot written a weapon switch makes none.)
        if (g_scriptedArmed) {
            g_scriptedArmed = false;
            g_yawAddress = 0;
            g_lostAt = tick;
            g_haveTarget = false;
            g_havePitch = false;
            return false;
        }
    }
    bool yawLeftAlone = false;  // the game left the yaw as last written (to the 0.0005 its own moves are counted by), for the pitch limits
    if (!g_havePrevious) {
        // From the picture's body where there is one (the angles found while mouse aim steered, a scripted moment
        // over, after a cutscene): the yaw written takes the hand's turn out of it, so the view goes on and the
        // game's camera turns to the gun. From the game's own yaw, the view jumped by the hand's turn since the
        // look stopped (18-34 degrees in The Great Destroyer: 18:31:38, 18:32:12, 18:33:27, 18:33:44, 18:34:51).
        // After a recenter, from the game's (Reset).
        g_base = body && !g_reanchor ? *body - g_yawOffset - (1.0f + sign) * headYaw : now - sign * headYaw;
        g_reanchor = false;
        g_turned = 0.0f;
        g_havePrevious = true;
    } else if (g_haveTarget) {
        // Whatever the game did to it since the last write (it should do nothing).
        float moved = now - g_target;
        moved -= 2 * kPi * std::round(moved / (2 * kPi));
        yawLeftAlone = std::fabs(moved) <= 0.0005f;
        if (std::fabs(moved) > 0.0005f) {
            ++g_gameMoves;
            g_gameMoveMost = std::max(g_gameMoveMost, std::fabs(moved));
        }
        const bool armed = skin::RifleInHand() || skin::LauncherInHand() || skin::HeldGunInHand();
        if (std::fabs(moved) <= kScriptedMove) {
            // The game moves it at its own ticks, 30 a second, so jumps come a frame or two apart: they count until it's been quiet a while.
            if (++g_scriptedQuiet >= kScriptedQuietFrames)
                g_scriptedStrikes = 0;
        } else if (g_scriptedQuiet = 0; armed && ++g_scriptedStrikes < kScriptedFrames) {
            static ULONGLONG s_logged = 0;
            if (tick - s_logged > 1000) {
                s_logged = tick;
                Log("Aim: the game jumped the look yaw %.1f degrees in a frame: put back to the hand", std::fabs(moved) * kRadToDeg);
            }
        } else if (g_scriptedOn.load(std::memory_order_relaxed)) {
            g_scriptedStrikes = 0;
            g_scripted = true;
            g_scriptedArmed = armed;
            g_scriptedSeen = now;
            g_scriptedMovedAt = tick;
            g_haveTarget = false;
            g_havePitch = false;
            Log("Aim: the game turned the look yaw itself by %.1f degrees (a scripted moment): the game's camera "
                "leads, the head looks about it",
                std::fabs(moved) * kRadToDeg);
            return false;
        }
    }
    // Yaw: set outright, the stick's turn added to the hand's. The picture follows the yaw written
    // here (camera.cpp: LookYaw gives this, not what the game made of it), so the turn is drawn in
    // step with the frame, whenever the game itself reads it, and the gun can't turn the view.
    g_turned += turn;
    float yaw = g_base + sign * (headYaw + g_turned);
    yaw -= 2 * kPi * std::floor(yaw / (2 * kPi));  // the game keeps it within 0 to 2 pi
    g_target = yaw;
    g_haveTarget = true;
    g_targetIsMouse = false;
    SetTo(yawAddress, yaw);
    NoteWritten(yaw + g_yawOffset);  // (as the camera has it, a vehicle's turn added: what LookYaw gives for this frame's turn)
    written.one = true;
    for (auto& slot : g_slots)
        if (const uintptr_t at = slot.load(std::memory_order_relaxed)) {
            float slotYaw = 0.0f, slotPitch = 0.0f;
            if (SlotShaped(at, slotYaw, slotPitch)) {
                SetTo(at, yaw);
            } else {
                slot = 0;
                Log("Aim: the look angles' slot at %08X isn't one any more: not written", static_cast<unsigned>(at));
            }
        }
    // Pitch: set it outright. Adding changes drifted: BF3 clamps how far the player can look
    // up or down, so whatever went past the clamp was lost and the game ended up aiming low.
    // The stored pitch is the true view pitch (positive = up), so it takes the angle as is. Past
    // that clamp, the game pulled each frame's write back and the gun shook aiming up (user,
    // 2026-10-30): where it pulls a steep write back to is learned as the limit and kept to.
    // (Only steep ones: the game's recoil moves the pitch too.)
    const bool find = g_findOn.load(std::memory_order_relaxed);
    // With find, the limits learned are forgotten at each find of the look angles: one learned for another slot or soldier stayed the whole launch ("above
    // -45.0" from a stand-in film, 2026-11-13 00:53). A real one is learned again at its first pull.
    if (static uint32_t s_findsSeen = 0; find && g_findCount.load() != s_findsSeen) {
        s_findsSeen = g_findCount.load();
        if (g_pitchMax != aimrules::kPitchDefault || g_pitchMin != -aimrules::kPitchDefault)
            Log("Aim: the look pitch's limits learned before forgotten (the look angles found again): a real one is learned again at its first pull");
        g_pitchMax = aimrules::kPitchDefault, g_pitchMin = -aimrules::kPitchDefault;
        g_probe.side = 0;
    }
    if (!g_havePitch)
        g_probe.side = 0;  // (nothing of this look written yet: a try begun before is no more)
    float pitchNow = 0.0f;
    const bool pitchRead = g_havePitch && camera::SafeRead(pitchAddress, &pitchNow, sizeof(pitchNow));
    // With find, a learned limit being tried again (below): pulled back within kProbeMs it stands; held that long, the game leaving the yaw alone all
    // the while, it was none and is dropped. Nothing is learned from those frames.
    bool probed = false;
    if (g_probe.side && pitchRead) {
        probed = true;
        if (const int drop = g_probe.Read(g_pitchWritten, pitchNow, yawLeftAlone, tick)) {
            float& limit = drop > 0 ? g_pitchMax : g_pitchMin;
            Log("Aim: the game let the look pitch go to %.1f degrees, past the limit of %.1f learned before: that limit dropped", pitchNow * 57.29578f,
                limit * 57.29578f);
            limit = drop > 0 ? aimrules::kPitchDefault : -aimrules::kPitchDefault;
        }
    }
    if (const int teaches = pitchRead && !probed ? aimrules::PullTeaches(g_pitchWritten, pitchNow, yawLeftAlone, find, g_pitchMax, g_pitchMin) : 0;
        teaches > 0) {
        g_pitchMax = pitchNow;
        Log("Aim: the game keeps the look pitch below %.1f degrees", pitchNow * 57.29578f);
    } else if (teaches < 0) {
        g_pitchMin = pitchNow;
        Log("Aim: the game keeps the look pitch above %.1f degrees", pitchNow * 57.29578f);
    }
    const float wanted = a.invertPitch ? -headPitch : headPitch;
    float pitch = std::clamp(wanted, g_pitchMin, g_pitchMax);
    // With find, the try: every kProbeEvery while the pitch wanted lies past a learned limit, the write goes past it for up to kProbeMs (a real limit
    // pulls it back at the game's next tick, a moment of the game's own clamp; a false one held the gun 15 degrees off its aim until the next load).
    if (g_probe.Past(wanted, g_pitchMax, g_pitchMin, find && g_havePitch, tick))
        pitch = std::clamp(wanted, -aimrules::kPitchDefault, aimrules::kPitchDefault);
    // The looks for WeaponDeviation: what was there (the pitch only as read just now, else the last written), then this frame's.
    ++g_lookFrame;
    NoteLook(now, pitchRead ? pitchNow : g_pitchWritten);
    NoteLook(yaw, pitch);
    g_lookAt = tick;
    g_pitchHeld = std::isfinite(wanted) ? wanted - pitch : 0.0f;
    SetTo(pitchAddress, pitch);
    for (const auto& slot : g_slots)
        if (const uintptr_t at = slot.load(std::memory_order_relaxed))
            SetTo(at + 0xC, pitch);
    g_pitchWritten = pitch;
    g_havePitch = true;
    g_lookedLast = true;
    return true;
}

bool LastWritten(float& yaw, float& pitch, float* pitchHeld) {
    if (!g_haveTarget || g_targetIsMouse || !g_havePitch || !g_lookAt || GetTickCount64() - g_lookAt > kLookFresh)
        return false;
    yaw = g_target;
    pitch = g_pitchWritten;
    if (pitchHeld)
        *pitchHeld = g_pitchHeld;
    return true;
}

int WrittenYaws(float* yaws, int most, unsigned* counts) {
    const uint32_t newest = g_writtenNewest.load(std::memory_order_acquire), from = g_writtenFrom.load(std::memory_order_relaxed);
    const ULONGLONG at = g_writtenAt.load(std::memory_order_relaxed), tick = GetTickCount64();  // (a write may come between the two: its time is then ahead of the tick)
    if (tick > at && tick - at > kWrittenFresh)
        return 0;
    int count = 0;
    for (uint32_t number = newest; count < most && count < static_cast<int>(kWritten) && number >= from && number > 0; --number) {
        const uint64_t entry = g_written[number % kWritten].load(std::memory_order_relaxed);
        if (static_cast<uint32_t>(entry >> 32) != number)
            break;  // written over since
        const uint32_t bits = static_cast<uint32_t>(entry);
        std::memcpy(&yaws[count], &bits, sizeof(bits));
        if (counts)
            counts[count] = number;
        ++count;
    }
    return count;
}

bool WeaponDeviation(float& yaw, float& pitch) {
    float cameraYaw = 0.0f, cameraPitch = 0.0f;
    if (!camera::GameWeaponAngles(cameraYaw, cameraPitch) || !std::isfinite(cameraYaw) || !std::isfinite(cameraPitch))
        return false;
    // Only a camera built since the last call: the same one again would count twice.
    static uint64_t s_last = ~0ull;
    uint32_t bits[2];
    std::memcpy(&bits[0], &cameraYaw, 4);
    std::memcpy(&bits[1], &cameraPitch, 4);
    const uint64_t key = (static_cast<uint64_t>(bits[0]) << 32) | bits[1];
    if (key == s_last)
        return false;
    s_last = key;
    if (!g_haveTarget || g_targetIsMouse || !g_lookAt || GetTickCount64() - g_lookAt > kLookFresh)
        return false;
    float best = 1e30f, bestYaw = 0.0f, bestPitch = 0.0f;
    for (const Look& look : g_looks) {
        if (g_lookFrame - look.frame >= static_cast<uint32_t>(kLookFrames) || !std::isfinite(look.yaw) || !std::isfinite(look.pitch))
            continue;
        const float dy = std::remainder(cameraYaw - (look.yaw + g_yawOffset), 2.0f * kPi), dp = cameraPitch - look.pitch;
        const float size = dy * dy * std::cos(cameraPitch) * std::cos(cameraPitch) + dp * dp;
        if (size < best)
            best = size, bestYaw = dy, bestPitch = dp;
    }
    if (best >= 1e30f || !std::isfinite(bestYaw) || !std::isfinite(bestPitch))
        return false;
    yaw = bestYaw;
    pitch = bestPitch;
    return true;
}

void Reset() {
    g_havePrevious = false;
    g_haveTarget = false;
    g_mouseHave = false;  // mouse aim centred again too, on the game's camera (g_reanchor)
    g_reanchor = true;
}

bool Scripted() {
    return g_scripted;
}

bool ScriptedArmed() {
    return g_scripted && g_scriptedArmed;
}

void Forget(const char* why) {
    if (g_yawAddress.exchange(0)) {
        if (why)
            Log("Aim: the look angles forgotten (%s; they're looked for again after)", why);
        else
            Log("Aim: the look angles forgotten (a cutscene: the game draws from its own camera, unturned or scripted; "
                "they're looked for again after)");
    }
    g_foundFor = 0;
    g_quickSince = 0;
    g_havePrevious = false;
    g_haveTarget = false;
    g_havePitch = false;
    DropWritten();  // (no look yaw is written through a cutscene: Update isn't called)
    // Mouse aim centred again where the game left the view: kept, it steered from its centring before the
    // load's opening sequence and swung the soldier back 75-80 degrees after it (the 16:27 log).
    g_mouseHave = false;
    // A cutscene ends a scripted moment: left on, it held the free hands off long after (Update's not-found path).
    g_scripted = false;
    g_lookedLast = false;  // the look stopped: a new soldier after the cutscene goes on from the picture
}

void SetScriptedFollow(bool on) {
    if (g_scriptedOn.exchange(on) != on)
        Log("Aim: scripted moments %s", on ? "lead the view (the head looks about them)" : "written over as before");
}

// weapon.txt "aimtest 20": the hand's yaw taken as turned that many degrees more, from 1 s to 3 s
// after, so the view's turn can be checked with the controllers lying still (weapon.txt "viewlog":
// the game's camera turns by it, the picture mustn't).
std::atomic<ULONGLONG> g_testAt{0};
std::atomic<float> g_testAngle{0.0f};

void StartTest(float degrees) {
    g_testAngle.store(degrees * kPi / 180.0f, std::memory_order_relaxed);
    g_testAt.store(GetTickCount64(), std::memory_order_relaxed);
    Log("Aim: test: the hand taken as turned %.0f degrees more from 1 s to 3 s from now", degrees);
}

float TestTurn() {
    const ULONGLONG since = GetTickCount64() - g_testAt.load(std::memory_order_relaxed);
    return since >= 1000 && since < 3000 ? g_testAngle.load(std::memory_order_relaxed) : 0.0f;
}

bool LookYaw(float& radians, float& sign) {
    if (!g_haveTarget)
        return false;
    // As written, whatever the game has done to it since; with a vehicle's turn (g_yawOffset) added, as
    // the camera has it (the picture takes the hand's turn out by the difference). Mouse aim's is the camera's own.
    radians = g_target + (g_targetIsMouse ? 0.0f : g_yawOffset);
    sign = config::GetAim().invertYaw ? 1.0f : -1.0f;  // the hand's yaw goes in negated (Update)
    return true;
}

void Probe() {
    static ULONGLONG lastLog = 0;
    ULONGLONG now = GetTickCount64();
    // weapon.txt "aimtest": the weapon camera (where the game fires) against the yaw written, ten
    // times a second while the test turns the hand (tools/selftest.ps1: it must aim where the gun points).
    static ULONGLONG lastTest = 0;
    if (TestTurn() != 0.0f && now - lastTest >= 100) {
        lastTest = now;
        float written[4] = {}, cameraYaw = 0.0f, cameraPitch = 0.0f;
        const uintptr_t at = g_yawAddress.load();
        if (at && camera::SafeRead(at, written, sizeof(written)) && camera::GameWeaponAngles(cameraYaw, cameraPitch))
            Log("Aim test: written %.4f, weapon camera %.4f", written[0], cameraYaw);
    }
    if (now - lastLog < 1000)
        return;
    lastLog = now;
    const uintptr_t yaw = g_yawAddress.load();
    float angles[4] = {}, cameraYaw = 0.0f, cameraPitch = 0.0f;
    if (yaw && camera::SafeRead(yaw, angles, sizeof(angles)))
        Log("Aim: yaw at %08X = %.4f, pitch %.4f", static_cast<unsigned>(yaw), angles[0], angles[3]);
    if (camera::GameWeaponAngles(cameraYaw, cameraPitch))
        Log("Aim: the weapon camera as the game wrote it: yaw %.4f, pitch %.4f", cameraYaw, cameraPitch);
    if (g_writes)
        Log("Aim: %d head moves written in the last second", g_writes);
    if (g_gameMoves)
        Log("Aim: the game moved the look yaw itself in %d frames (by up to %.2f degrees)", g_gameMoves,
            g_gameMoveMost * 57.29578f);
    g_writes = 0;
    g_gameMoves = 0;
    g_gameMoveMost = 0.0f;
}

void FlipYaw() {
    config::Aim a = config::GetAim();
    a.invertYaw = !a.invertYaw;
    config::SetAim(a);
    Log("Head yaw direction %s", a.invertYaw ? "inverted" : "normal");
}

void FlipPitch() {
    config::Aim a = config::GetAim();
    a.invertPitch = !a.invertPitch;
    config::SetAim(a);
    Log("Head pitch direction %s", a.invertPitch ? "inverted" : "normal");
}

}

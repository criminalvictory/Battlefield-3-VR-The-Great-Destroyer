#include "ammo.h"

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <unordered_map>

#include "camera.h"
#include "config.h"
#include "god.h"
#include "gunprofile.h"
#include "hands.h"
#include "input.h"
#include "inputrules.h"
#include "log.h"
#include "pickups.h"
#include "pickrules.h"
#include "pistolammo.h"
#include "shotgunammo.h"
#include "recoilguns.h"
#include "recoilrun.h"
#include "reload.h"
#include "skin.h"
#include "tubeask.h"

namespace {

// The server's soldier holds its weapons by slot: its +0x4EC, whose +0x40 on are the weapons (a slot empty: 0), each with its firing
// state at +0x128. On Rock and a Hard Place: 0 the M416, 2 the grenade launcher under it, 5 the SMAW, 6 the grenades, 7 the knife.
// In the firing state +0xF4 is the rounds loaded (magazine and chamber), +0xF8 the spare rounds. When its loaded count ran out the game
// reloaded by itself (30 in, 30 fewer spare); the client has objects of the same kind (+0xBC, +0x1D8, +0x580) that follow the server's count.
// Until 2026-11-04 the rifle's was read at +0x4C0, +0x248 and the launcher's at +0x258: not fields of that component (it's 0x130 long at
// most) but past its end, in whatever the game's pool had put after it. Right with the soldiers allocated as at a mission's start, and with
// some later respawns; after others no gun was found, and every reload by hand died with "only the picture changes" (the user's run,
// 23:13: table slot 7 instead of 5). The list is where the train's pistol was found by on 2026-10-31.
constexpr uintptr_t kWeaponHandler = 0x4EC, kWeaponSlots = 0x40, kWeaponFiring = 0x128;
constexpr int kRifleSlot = 0, kLauncherSlot = 5;
constexpr uintptr_t kFiringType = 0x1DAD810;  // vtable, bf3.exe offset
constexpr uintptr_t kLoaded = 0xF4;
constexpr uintptr_t kSpare = 0xF8;

// The rifle reloaded by hand (skin::RifleInHand, Blackburn's M416): its magazine's size. The game's
// own data has it (AmmoConfigData.MagazineCapacity), not found yet (tools/gunammo.ps1 looked two
// pointers deep from the firing state). Learning it from the most rounds seen went wrong: a reload
// on top of the game's own made it 59, then 88 (2026-10-30).
constexpr int kRifleMagazine = 30;

// Per gun (its firing state). The rifle's count, as the game keeps it, has one round more than
// the mod says (hidden): the trigger is let go at one round left, and the game, a frame or two
// behind, sometimes fired that one too; with none it reloaded by itself, a magazine on top of the
// hand's. With the extra one it never runs out.
struct Gun {
    int fullest = 0;      // the most rounds seen in it before the mod wrote its count
    int learned = 0;      // the magazine's size from the game's own reload, 0 not seen
    int hidden = 0;       // the extra round in the game's count
    int last = -1;        // the game's count last frame
    bool written = false;
    bool magOut = false;  // its magazine was out when another was taken in hand
    bool emptied = false;  // weapon.txt "empty" left it at none for the game's own reload (no hidden round till it has reloaded)
    int putBacks = 0;      // its hidden round put back after the game fired it, since its count was last 2 or more (pickrules::SeeCount)
};

uintptr_t g_soldier = 0;  // the server's soldier the guns below belong to
uintptr_t g_firing = 0;   // the gun in hand
uintptr_t g_weapon = 0, g_blueprint = 0;  // its weapon and blueprint: a new gun at an old address is still a new gun
ULONGLONG g_firingSince = 0;  // when the gun in hand last changed (the old rifle is still drawn a while)
std::unordered_map<uintptr_t, Gun> g_guns;

// A gun the game swapped out of the first slot into one of its pickups (Swapped): its magazine out or in, kept by that pickup and the gun's name
// until it comes out of that pickup again, then given to it at its new-gun path (pending). The oldest of 8 replaced.
struct KeptMagazine {
    uint32_t entity = 0;
    char name[pickups::kName] = {};
    bool magOut = false;
    uint32_t swap = 0;    // the swap that kept it (0: none kept here)
};
KeptMagazine g_keptMagazines[8];
int g_keptNext = 0;
bool g_pendingMagOut = false, g_havePendingMagOut = false;
uint32_t g_pendingFiring = 0;  // ... the gun it's for: its firing state as it came out (0: not known), the slot it came into
int g_pendingSlot = 0;
ULONGLONG g_swappedAt = 0;  // the last swap of the game's (Swapped; 0 none): a count up just after it is the game's take, not a reload
bool g_rKeyDown = false;     // R as the keyboard has it (a film's key or a person's, not only the mod's own for X held), last frame
ULONGLONG g_rKeyUpAt = 0;    // ... when it was last let go (0 never)
// X held with a gun in a hand (HeldPress): the game's own reload waited for (inputrules.h's StepPress), the gun and its count at the hold.
inputrules::PressWait g_press;
// ... a magazine pushed home by hand before the game's count rose (Insert): the game's reload of that hold no longer asked for (inputrules.h's StillAsked).
inputrules::HandOver g_handOver;

// What the wait came to this frame (PressFrame): the hold as it was before the step, the count now. Loaded: the reload's picture (the magazine in, and
// the M416's bolt home or a held gun's chamber filled, whichever was drawn at the hold), each said by its own module when it changes; otherwise nothing
// changes.
void PressFrame(inputrules::PressSeen seen, const inputrules::PressWait& hold, int loaded, int spare) {
    const double after = hold.at ? (GetTickCount64() - hold.at) / 1000.0 : 0.0;
    switch (seen) {
    case inputrules::PressSeen::kLoaded:
        Log("Ammo: X held: the game's count rose %.2f s after the hold (%d -> %d rounds loaded, %d spare): its reload drawn now", after, hold.loaded, loaded, spare);
        {
            const inputrules::PressDraw draw = inputrules::DrawAtRise(hold.kind);
            if (draw.magazine)
                skin::InsertMagazine();
            if (draw.bolt)
                reload::ReleaseBolt();  // (reload.cpp's bolt is the M416's alone)
            if (draw.chamber)
                hands::HeldChambered(false);
        }
        break;
    case inputrules::PressSeen::kGaveUp: {
        char count[64] = "its count not read";
        if (loaded >= 0)
            sprintf_s(count, "%d rounds loaded, %d spare", loaded, spare);
        Log("Ammo: X held: the game's count didn't rise in %.1f s (%s): no reload of the game's; the magazine, the bolt and the chamber left as they were",
            after, count);
        break;
    }
    case inputrules::PressSeen::kOtherGun:
        Log("Ammo: X held: another gun in the hands %.2f s after the hold, before the game's count rose: nothing of the reload drawn", after);
        break;
    default:
        break;
    }
}

// The keyboard's R, looked at each frame (Update, on the thread that presents): 0 while it's down, else the ms since it was let go; ~0ULL never.
// It asks for the game's reload as X held does (input::GameReloadSince knows only the mod's own R).
unsigned long long KeyRSince() {
    const bool down = (GetAsyncKeyState('R') & 0x8000) != 0;
    const ULONGLONG now = GetTickCount64();
    if (g_rKeyDown && !down)
        g_rKeyUpAt = now;
    g_rKeyDown = down;
    if (down)
        return 0;
    return g_rKeyUpAt ? now - g_rKeyUpAt : ~0ULL;
}

// A take of the game's waits on its thread (pickups::TakeOpen): it may be freeing these weapons just now, so no count is written. Said once a window.
std::atomic<bool> g_unwrittenSaid{false};
bool TakeOpenNow() {
    if (!pickups::TakeOpen())
        return false;
    if (!g_unwrittenSaid.exchange(true))
        Log("Ammo: a count left unwritten: the game is taking a gun just now (pickups)");
    return true;
}

uintptr_t ReadPointer(uintptr_t address) {
    uint32_t value = 0;
    return camera::SafeRead(address, &value, sizeof(value)) ? value : 0;
}

// Every count the mod writes (CountWrites: recoil takes no drop in a count as shots while this moved).
std::atomic<unsigned> g_countWrites{0};

bool WriteInt(uintptr_t address, int value) {
    if (TakeOpenNow())
        return false;
    g_countWrites.fetch_add(1, std::memory_order_relaxed);
    __try {
        *reinterpret_cast<int*>(address) = value;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// The firing state of the weapon in a slot of the player's soldier, or 0.
uintptr_t FiringInSlot(int slot) {
    const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    const uintptr_t soldier = god::PlayerServerSoldier();
    const uintptr_t handler = soldier ? ReadPointer(soldier + kWeaponHandler) : 0;
    const uintptr_t weapon = handler ? ReadPointer(handler + kWeaponSlots + 4 * slot) : 0;
    const uintptr_t firing = weapon ? ReadPointer(weapon + kWeaponFiring) : 0;
    return firing && ReadPointer(firing) == base + kFiringType ? firing : 0;
}

// The same, with that slot's weapon and its blueprint (the weapon's data at +0x14, its +0x13C, as build\guntools\pickpeek.ps1 reads its name): all
// three tell a new gun. 0 for all three if the slot has no weapon with a firing state; the blueprint 0 if it wasn't read. FiringInList: from the
// soldier's weapons (its +0x4EC) read once for every slot (ReadSlots).
constexpr uintptr_t kWeaponData = 0x14, kWeaponBlueprint = 0x13C;
uintptr_t FiringInList(uintptr_t base, uintptr_t handler, int slot, uintptr_t& weapon, uintptr_t& blueprint) {
    weapon = handler ? ReadPointer(handler + kWeaponSlots + 4 * slot) : 0;
    const uintptr_t firing = weapon ? ReadPointer(weapon + kWeaponFiring) : 0;
    if (!firing || ReadPointer(firing) != base + kFiringType) {
        weapon = 0, blueprint = 0;
        return 0;
    }
    const uintptr_t data = ReadPointer(weapon + kWeaponData);
    blueprint = data ? ReadPointer(data + kWeaponBlueprint) : 0;
    return firing;
}
uintptr_t FiringInSlot(int slot, uintptr_t& weapon, uintptr_t& blueprint) {
    const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    const uintptr_t soldier = god::PlayerServerSoldier();
    return FiringInList(base, soldier ? ReadPointer(soldier + kWeaponHandler) : 0, slot, weapon, blueprint);
}

// The slot of the gun in the hands (pickrules::HandSlot), as worked out at the top of Update each frame: the first (the M416) unless a gun taken up
// from the ground is in the hands from another. Any thread reads it (HandSlot).
std::atomic<int> g_handSlot{0};

// The train pistol with its magazine by hand and its rounds the mod's (pistolammo.h; hands.cpp says so each frame: SetPistolByHand): the pistol as
// the mod counts it, by its firing state, its weapon and its blueprint (firing 0: not counted by the mod, the game's own reload as before). The
// game's count of it is kept one above (its Gun's hidden round). Render thread; the atomics below are its answers for any thread.
struct Pistol {
    uintptr_t firing = 0, weapon = 0, blueprint = 0;
    int slot = 0;             // the soldier's slot it's in
    pistolammo::State state;
    int capacity = 15;        // the rounds a magazine holds (its row's Ammo.MagazineCapacity less the chambered round)
    int lost = 0;             // rounds the slide threw out, not yet out of the game's count
    ULONGLONG seatedAt = 0;   // when a magazine was last seated by hand (0 never): X held before that asks for no reload of the game's
    ULONGLONG goneAt = 0;     // since when it isn't in any slot (0: it is)
};
Pistol g_pistol;
bool g_pistolByHand = false;
std::atomic<bool> g_pistolLive{false}, g_pistolDry{false}, g_pistolLocked{false}, g_pistolMagIn{true};
std::atomic<int> g_pistolSpare{0}, g_pistolLoaded{0};

// The gun whose magazine is worked by hand: the one in the hands' slot, the primary's with the launcher in hand too. Not the train's pistol, which has
// its own path (UpdatePistol with its rounds by hand; otherwise its rounds are the game's alone): it's in the list's first slot there, RoundsInHand
// reads it for the shots' flash.
// A tube shotgun with its shells by hand (shotgunammo.h; hands.cpp says so each frame: SetTubeByHand): the gun as the mod counts it, by its firing state,
// its weapon and its blueprint (firing 0: not counted by the mod). The game's count of it is kept one above (its Gun's hidden round) but while the game's
// own reload is asked for. Render thread; the atomics below are its answers for any thread.
struct Tube {
    uintptr_t firing = 0, weapon = 0, blueprint = 0;
    int slot = 0;             // the soldier's slot it's in
    shotgunammo::State state;
    int capacity = 4;         // the shells its tube holds (its row's Ammo.MagazineCapacity less the chambered one)
    int lost = 0;             // live shells the bolt threw out, not yet out of the game's count
    ULONGLONG goneAt = 0;     // since when it isn't in any slot (0: it is)
    // The asking for the game's own reload (tubeask.h: the reload key only; ended by a shot; the latch "it began on an empty gun"; the times).
    tubeask::State ask;
    // The reload key as this gun has it: down only while it is the gun in the hands, from a press begun there (a key meant for another gun is nothing to it).
    tubeask::Key key;
    const char* name = "shotgun";
};
Tube g_tube;
// The counted shotgun the game swapped out by itself with its hidden round in its count (Swapped: not a hand's take, whose hand-over writes the count
// without it): the pickup it went into (0: that one went with the take, the gun into a new one), its name, its count as the game had it; and the gun
// that came out of it again (its firing state), met as already carrying that round if its count is still that one.
struct TubeWent {
    bool have = false;
    uint32_t entity = 0, swap = 0;
    char name[pickups::kName] = {};
    int raw = 0;
};
TubeWent g_tubeWent;
uint32_t g_tubeCameFiring = 0;
int g_tubeCameRaw = 0;
bool g_tubeByHand = false;
std::atomic<bool> g_tubeLive{false}, g_tubeDry{false}, g_tubeLocked{false}, g_tubeChambered{false}, g_tubePort{false}, g_tubeGameReload{false};
std::atomic<int> g_tubeShells{0}, g_tubeCapacity{4}, g_tubeCount{0}, g_tubeSpare{0}, g_tubeBelly{0}, g_tubeSlotNow{-1};
std::atomic<uintptr_t> g_tubeFiringNow{0};
std::atomic<unsigned> g_tubeLockedShots{0};
std::atomic<bool> g_tubeAskHeld{false};       // X held with it in the hands (input.cpp: TubeAsk)
std::atomic<bool> g_tubeHiddenOut{false};     // its hidden round out of the game's count, as the last look left it
std::atomic<bool> g_tubeEndAsk{false};        // the trigger pulled on its last shell with that round out (input.cpp: TubeEndAsk): the asking ends at the next look

uintptr_t Firing() {
    if (skin::TrainPistolInHand())
        return 0;
    const uintptr_t firing = FiringInSlot(g_handSlot.load(std::memory_order_relaxed));
    // (The pistol's "in hand" drops a frame now and then: its count stays its own. A tube shotgun the mod counts has its own count too.)
    return firing && (firing == g_pistol.firing || firing == g_tube.firing) ? 0 : firing;
}

// The game's own counts.
bool ReadRaw(uintptr_t firing, int& loaded, int& spare) {
    return camera::SafeRead(firing + kLoaded, &loaded, sizeof(loaded)) &&
           camera::SafeRead(firing + kSpare, &spare, sizeof(spare)) && loaded >= 0 && loaded < 10000 &&
           spare >= 0 && spare < 100000;
}

bool g_launcherByHand = false;

// The grenade launcher under the rifle: its slot, and its firing state's word that's 1 with it in hand (3 put away).
constexpr int kM320Slot = 2;
constexpr uintptr_t kInHandState = 0xA8;
std::atomic<int> g_m320State{-1}, g_m320Loaded{0}, g_m320Spare{0};  // -1: none found
std::atomic<ULONGLONG> g_m320UpAt{0};  // when it was last seen in hand
// Its count as the mod keeps it: the game's is kept at 1 (a hidden round, as the SMAW's). Empty in the game's hand, the game won't put it away: its own "switch to the grenade
// launcher" only starts or calls off its reload, key 1 is ignored (seen 2026-11-05), so after a shot the rifle couldn't come back. -1 not known yet (the game's count when first
// seen, and again for a new life). Its own reload asked for (M320GameReload): the game's count left at 0 till a round's in.
int g_m320Rounds = -1;
uintptr_t g_m320Firing = 0;
std::atomic<bool> g_m320GameReload{false};

// The rifle and the launcher under it are each other's "alt deploy" (both AltDeployId 3 in their WeaponFiringData, the firing state's +0x94): switching between them takes its
// AltDeployTime, 0.5 s in the game's data. Made instant (the user: weapon switches instant; with the left hand on the launcher's grip a shot is a switch there and straight back,
// the rifle firing again at once). The game's own data has 0 for it too (the M416's modifier #95, behind an unlock). Written only with every word as read: its DeployTime (0.67
// the rifle's, 0.55 the launcher's), AltDeployId 3, and AltDeployTime 0.5 (or already 0).
constexpr uintptr_t kFiringData = 0x94, kDeployTime = 0x0C, kAltDeployTime = 0x18, kAltDeployId = 0x1C;

bool WriteFloat(uintptr_t address, float value) {
    __try {
        *reinterpret_cast<float*>(address) = value;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void InstantAltDeploy(uintptr_t firing, float deployTime, const char* name, int which) {
    const uint32_t data = firing ? static_cast<uint32_t>(ReadPointer(firing + kFiringData)) : 0;
    float deploy = 0.0f, alt = 0.0f;
    int id = 0;
    if (!data || !camera::SafeRead(data + kDeployTime, &deploy, sizeof(deploy)) || !camera::SafeRead(data + kAltDeployTime, &alt, sizeof(alt)) ||
        !camera::SafeRead(data + kAltDeployId, &id, sizeof(id)))
        return;
    if (deploy != deployTime || id != 3 || alt == 0.0f)
        return;
    static uint32_t s_told[2] = {};
    if (alt != 0.5f) {
        if (s_told[which] != data)
            Log("Ammo: the %s's switch time is %.3f s, not the game's 0.5: left alone", name, alt);
        s_told[which] = data;
        return;
    }
    if (WriteFloat(data + kAltDeployTime, 0.0f))
        Log("Ammo: the switch to the %s made instant (its AltDeployTime 0.5 -> 0, data at %08X)", name, data);
}

// The Stinger's firing state: in the second slot once it's taken up, the first on the test gun. Holds a round at a time (with the mod's hidden round, two).
uintptr_t StingerFiringState() {
    uintptr_t firing = FiringInSlot(1);
    if (!firing)
        firing = FiringInSlot(kRifleSlot);
    int loaded = 0, spare = 0;
    return firing && ReadRaw(firing, loaded, spare) && loaded <= 2 ? firing : 0;
}

// The launcher's firing state. Only one that holds a round at a time (with the mod's hidden round, two): another gun in that place
// mustn't have its count written.
uintptr_t LauncherFiringState() {
    const uintptr_t firing = FiringInSlot(kLauncherSlot);
    int loaded = 0, spare = 0;
    return firing && ReadRaw(firing, loaded, spare) && loaded <= 2 ? firing : 0;
}

// As the mod counts them: without the hidden round.
bool Read(uintptr_t firing, int& loaded, int& spare) {
    if (!ReadRaw(firing, loaded, spare))
        return false;
    auto it = g_guns.find(firing);
    if (it != g_guns.end())
        loaded = std::max(0, loaded - it->second.hidden);
    return true;
}

void Write(uintptr_t firing, int loaded, int spare) {
    if (TakeOpenNow())
        return;
    Gun& gun = g_guns[firing];
    gun.written = true;
    gun.last = loaded + gun.hidden;
    WriteInt(firing + kSpare, spare);
    WriteInt(firing + kLoaded, gun.last);
}

// The held gun's row in the guns' table: by its shape, or by that as a name; null for one the table doesn't know (or none held).
const recoilguns::Row* HeldRow() {
    if (!skin::HeldGunInHand())
        return nullptr;
    const char* shape = skin::HeldShape();
    const recoilguns::Row* row = recoilguns::ByShape(shape);
    return row ? row : recoilguns::ByName(shape);
}

// The magazine's size for the gun in hand. A held gun's at least its row's (Ammo.MagazineCapacity less the chambered round: the M416's and the
// AN-94's 31, 30 a magazine): a gun taken up from a dead soldier comes part empty, and the most rounds seen in it would be a short magazine.
int Capacity() {
    if (skin::RifleInHand() && !skin::HeldGunInHand())
        return kRifleMagazine;
    const recoilguns::Row* row = HeldRow();
    const int table = row && row->magazine > 1 ? row->magazine - 1 : 1;
    auto it = g_guns.find(g_firing);
    if (it == g_guns.end())
        return table;
    return it->second.learned ? it->second.learned : std::max({1, it->second.fullest - 1, table});
}

// ---- The soldier's slots, read here (no hook of the game's needed): the first two are pickrules::HandSlot's inputs, all ten the holsters' ----

constexpr int kSlots = 10;  // the slots looked at, as pickups looks at them
static_assert(kSlots == pickups::kMostSlots, "the soldier's slots as pickups.h counts them");

// A slot's blueprint's name (its +8: "Weapons/AN94/AN94"), read again only when the blueprint there changes; "" if it can't be read. One the guns'
// table has no row for is read again a second on while it stays (a name read while the game was still writing it comes right). With it: its row
// (recoilguns::ByName); whether that gun is one the hands work the magazine of (the table has its first-person body, and it is no launcher and no
// pistol: HandSlot's, slots 0 and 1); whether it is a launcher by its kind, kRocket (the SMAW), kMissile (the Stinger, the Javelin) or kAt4, never
// kM320 (the M320 goes with its rifle, or by its slot): holsters put that one at the launcher's place whatever its slot.
constexpr uintptr_t kBlueprintName = 8;
struct SlotName {
    uintptr_t blueprint = 0;
    char name[pickups::kName] = {};
    const recoilguns::Row* row = nullptr;
    bool handGun = false;
    bool launcher = false;
    ULONGLONG readAt = 0;  // (when the name was read)
};
SlotName g_slotNames[kSlots];

const char* NameIn(int slot, uintptr_t blueprint) {
    SlotName& n = g_slotNames[slot];
    const ULONGLONG now = GetTickCount64();
    if (n.blueprint == blueprint && (n.row || !blueprint || now - n.readAt < 1000))
        return n.name;
    n.blueprint = blueprint;
    n.readAt = now;
    char buffer[pickups::kName] = {};
    const uintptr_t text = blueprint ? ReadPointer(blueprint + kBlueprintName) : 0;
    if (text && !camera::SafeRead(text, buffer, sizeof(buffer) - 1)) {
        std::memset(buffer, 0, sizeof(buffer));
        for (size_t i = 0; i + 1 < sizeof(buffer) && camera::SafeRead(text + i, &buffer[i], 1) && buffer[i]; ++i) {
        }
    }
    buffer[sizeof(buffer) - 1] = 0;
    std::memcpy(n.name, buffer, sizeof(n.name));
    n.row = n.name[0] ? recoilguns::ByName(n.name) : nullptr;
    n.handGun = n.row && n.row->body && !recoil::Launcher(n.row->kind) && n.row->kind != recoil::Kind::kPistol;
    n.launcher = n.row && (n.row->kind == recoil::Kind::kRocket || n.row->kind == recoil::Kind::kMissile || n.row->kind == recoil::Kind::kAt4);
    return n.name;
}

// The ten slots as this frame read them (ReadSlots, in Update past the take's gate): each one's weapon, its firing state and its blueprint (0 for all
// three: no weapon with a firing state there), and its name above, read with them. Every reader of a slot in a frame goes by these: HandSlot's two,
// the check of the game's own current weapon, and the holsters' ten (ammo.h's GameSlot, SlotCarried, LauncherInSlot, NameInSlot, RowInSlot). Through
// a take's window (pickups::TakeOpen) they stay as last read.
struct SlotRead {
    uintptr_t firing = 0, weapon = 0, blueprint = 0;
};
SlotRead g_slots[kSlots];
std::atomic<unsigned> g_carried{0};    // bit s: slot s holds a weapon with a firing state, as last read
std::atomic<unsigned> g_launchers{0};  // bit s: ... a launcher by its kind
std::atomic<int> g_gameSlot{-1};       // the game's own current weapon as this frame read it, for holsters (GameSlot); -1 none read

void ReadSlots() {
    const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    const uintptr_t soldier = god::PlayerServerSoldier();
    const uintptr_t handler = soldier ? ReadPointer(soldier + kWeaponHandler) : 0;
    unsigned carried = 0, launchers = 0;
    for (int s = 0; s < kSlots; ++s) {
        SlotRead& read = g_slots[s];
        read.firing = FiringInList(base, handler, s, read.weapon, read.blueprint);
        if (!read.firing)
            continue;
        carried |= 1u << s;
        NameIn(s, read.blueprint);
        if (g_slotNames[s].launcher)
            launchers |= 1u << s;
    }
    g_carried.store(carried, std::memory_order_relaxed);
    g_launchers.store(launchers, std::memory_order_relaxed);
}

// What the slots hold, said when it changes and then stays so for half a second (a new life fills them over a few frames, a take changes one or
// two), or 3 s after a change if it never settles: each slot with a weapon with a firing state, by the guns' table's name ("0 M416"), "a launcher"
// by its kind, or its blueprint's name whole when the table has no row for it. Nothing said while none holds one (no soldier). At most 8 lines in
// 5 s.
void SaySlots() {
    static unsigned s_carried = 0, s_launchers = 0, s_rows = 0;
    static uintptr_t s_blueprints[kSlots] = {};
    static ULONGLONG s_changedAt = 0, s_owedSince = 0, s_windowAt = 0;
    static bool s_owed = false;
    static int s_inWindow = 0;
    const unsigned carried = g_carried.load(std::memory_order_relaxed), launchers = g_launchers.load(std::memory_order_relaxed);
    unsigned rows = 0;
    uintptr_t blueprints[kSlots] = {};
    for (int s = 0; s < kSlots; ++s) {
        if (!((carried >> s) & 1u))
            continue;
        blueprints[s] = g_slots[s].blueprint;
        if (g_slotNames[s].row)
            rows |= 1u << s;
    }
    const ULONGLONG now = GetTickCount64();
    if (carried != s_carried || launchers != s_launchers || rows != s_rows || std::memcmp(blueprints, s_blueprints, sizeof(blueprints)) != 0) {
        s_carried = carried, s_launchers = launchers, s_rows = rows;
        std::memcpy(s_blueprints, blueprints, sizeof(blueprints));
        s_changedAt = now;
        if (!s_owed)
            s_owedSince = now;
        s_owed = carried != 0;
    }
    if (!s_owed || (now - s_changedAt < 500 && now - s_owedSince < 3000))
        return;
    if (now - s_windowAt > 5000)
        s_windowAt = now, s_inWindow = 0;
    if (s_inWindow >= 8)
        return;
    ++s_inWindow;
    s_owed = false;
    char line[1024] = "";
    size_t used = 0;
    for (int s = 0; s < kSlots && used + 1 < sizeof(line); ++s) {
        if (!((carried >> s) & 1u))
            continue;
        const SlotName& n = g_slotNames[s];
        char name[pickups::kName] = {};  // (a name read wrong may hold anything: only its printable letters said)
        for (size_t i = 0; i + 1 < sizeof(name) && n.name[i]; ++i)
            name[i] = n.name[i] >= 0x20 && n.name[i] < 0x7F ? n.name[i] : '?';
        const int wrote = n.row ? std::snprintf(line + used, sizeof(line) - used, "%s%d %s%s", used ? ", " : "", s, n.row->name,
                                                n.launcher ? " (a launcher)" : "")
                                : std::snprintf(line + used, sizeof(line) - used, "%s%d \"%s\" (not in the guns' table)", used ? ", " : "", s,
                                                name);
        if (wrote < 0)
            break;
        used = std::min(sizeof(line) - 1, used + static_cast<size_t>(wrote));
    }
    Log("Ammo: the soldier's slots: %s", line);
}

// The game's own current weapon: the slot in its soldier's hands. The soldier's weapons (its +0x4EC, set at 00ABFE4E) keep an object at +0x68 (made
// at 011D5880, kept at 00AC0BF7) whose +4 is that slot: their own "the weapon in hand" (00ABA5D0) takes the list's entry by it, and a switch
// (011CFD90) writes the new slot there and the old one at +8. The object is known by either of its two vtables. -1 if it isn't there, can't be read,
// or names no slot of the list. unknownType: the vtable of an object there of neither kind (a level whose weapons keep it otherwise), else 0.
constexpr uintptr_t kSwitching = 0x68, kCurrentSlot = 4;
constexpr uintptr_t kSwitchingTypes[2] = {0x1DACB04, 0x1DACAF4};  // vtables, bf3.exe offsets
int ReadGameSlot(uintptr_t& unknownType) {
    const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    const uintptr_t soldier = god::PlayerServerSoldier();
    const uintptr_t handler = soldier ? ReadPointer(soldier + kWeaponHandler) : 0;
    const uintptr_t switching = handler ? ReadPointer(handler + kSwitching) : 0;
    const uintptr_t type = switching ? ReadPointer(switching) : 0;
    unknownType = 0;
    if (type && type != base + kSwitchingTypes[0] && type != base + kSwitchingTypes[1]) {
        unknownType = type;
        return -1;
    }
    int slot = -1;
    if (!type || !camera::SafeRead(switching + kCurrentSlot, &slot, sizeof(slot)))
        return -1;
    return slot >= 0 && slot < kSlots ? slot : -1;
}

int g_handLanded = -1;      // the slot (0 or 1) the last take put its gun into (Swapped), or the last pick since; -1 none this life
std::atomic<int> g_handPicked{-1};  // the slot (0 or 1) the game was last told to raise (Picked, input's thread), till UpdateHandSlot takes it; -1 none
bool g_handSaid = false;    // the slot said once this life
int g_handWordsSaid = -1;   // the slot the game's words named when they last disagreed with the answer (said once a disagreement; -1 none)

// What's in the hands as skin draws it, in the hands' own order (hands.cpp's profile: a held gun before the rifle, which is still "in hand" 30 frames
// after its last draw).
pickrules::Drawn DrawnNow() {
    if (skin::TrainPistolInHand() || skin::LauncherInHand())
        return pickrules::Drawn::kOwnPath;
    if (skin::HeldGunInHand())
        return gunprofile::RowOf(skin::HeldShape()) == gunprofile::kStingerRow ? pickrules::Drawn::kOwnPath : pickrules::Drawn::kHeld;
    if (skin::RifleInHand())
        return pickrules::Drawn::kRifle;
    return pickrules::Drawn::kNothing;
}

// This frame's slot of the gun in the hands, into g_handSlot; said when it changes (and once a life), and once when the game's sure words name the
// other of the two.
void UpdateHandSlot() {
    pickrules::HandSlotIn in;
    in.drawn = DrawnNow();
    const recoilguns::Row* row = in.drawn == pickrules::Drawn::kRifle   ? recoilguns::ByBody(gunprofile::kRifleId)
                                 : in.drawn == pickrules::Drawn::kHeld ? HeldRow()
                                                                       : nullptr;
    const recoilguns::Row* rifle = recoilguns::ByBody(gunprofile::kRifleId);
    in.known = row != nullptr;
    // (Slots 0 and 1 only, as this frame read them: ReadSlots.)
    for (int s = 0; s < 2; ++s) {
        const uintptr_t firing = g_slots[s].firing;
        if (!firing)
            continue;
        in.weapon |= 1u << s;
        const char* name = g_slotNames[s].name;
        if (g_slotNames[s].handGun)
            in.handGun |= 1u << s;
        if (row && name[0] && recoilguns::SameName(name, row->blueprint))
            in.byName |= 1u << s;
        if (rifle && name[0] && recoilguns::SameName(name, rifle->blueprint))
            in.rifleName |= 1u << s;
        int word = -1;
        if (camera::SafeRead(firing + kInHandState, &word, sizeof(word)))
            in.word[s] = word;
    }
    const int was = g_handSlot.load(std::memory_order_relaxed);
    // (A pick since the last take: two guns of one kind, or one the table doesn't know, go where the game was last told to raise one.)
    if (const int picked = g_handPicked.exchange(-1, std::memory_order_relaxed); picked == 0 || picked == 1)
        g_handLanded = picked;
    in.landed = g_handLanded;
    in.last = was;
    in.lastSame = g_firing && was >= 0 && was < kSlots && g_slots[was].firing == g_firing && g_slots[was].weapon == g_weapon &&
                  g_slots[was].blueprint == g_blueprint;
    uintptr_t unknownType = 0;
    in.current = ReadGameSlot(unknownType);
    // Holsters' answer (GameSlot): this read while that slot holds a weapon with a firing state as this frame read the slots, else none read.
    const bool currentCarried = in.current >= 0 && g_slots[in.current].firing;
    g_gameSlot.store(currentCarried ? in.current : -1, std::memory_order_relaxed);
    // The game's own current weapon, said each time it changes (a key's switch, a script's, a take, a load; none read while there's no soldier), at most
    // 8 lines in 5 s, the rest counted into the next.
    {
        static int s_slotWas = -2;  // (-2: never read)
        static ULONGLONG s_slotWindowAt = 0;
        static int s_slotInWindow = 0, s_slotUnsaid = 0;
        if (in.current != s_slotWas) {
            if (in.current >= 0 || s_slotWas >= 0) {
                const ULONGLONG now = GetTickCount64();
                if (now - s_slotWindowAt > 5000)
                    s_slotWindowAt = now, s_slotInWindow = 0;
                if (s_slotInWindow < 8) {
                    ++s_slotInWindow;
                    char was[24] = "not read yet", more[64] = "";
                    if (s_slotWas >= 0)
                        sprintf_s(was, "%d", s_slotWas);
                    else if (s_slotWas == -1)
                        strcpy_s(was, "none");
                    if (s_slotUnsaid > 0)
                        sprintf_s(more, "; %d changes not said before it", s_slotUnsaid);
                    s_slotUnsaid = 0;
                    if (in.current >= 0)
                        Log("Ammo: the game's own current weapon: slot %d (was %s)%s", in.current, was, more);
                    else
                        Log("Ammo: the game's own current weapon: none read (was %s)%s", was, more);
                } else {
                    ++s_slotUnsaid;
                }
            }
            s_slotWas = in.current;
        }
    }
    // Why holsters have no slot to go by while there is a soldier, said: the read naming a slot with no weapon with a firing state, the same one half
    // a second on end (a moment's mismatch, as the slots fill at a new life, isn't), at most once in 5 s; an object of neither known kind where the
    // slot is kept, the same one 2 s on end (a level whose weapons keep it otherwise: each of 4 such vtables once a session).
    {
        static int s_emptyWas = -1;
        static ULONGLONG s_emptySince = 0, s_emptySaidAt = 0;
        static bool s_emptySaid = false;
        const int empty = in.current >= 0 && !currentCarried ? in.current : -1;
        const ULONGLONG now = GetTickCount64();
        if (empty != s_emptyWas)
            s_emptyWas = empty, s_emptySince = now, s_emptySaid = false;
        if (empty >= 0 && !s_emptySaid && now - s_emptySince > 500 && now - s_emptySaidAt > 5000) {
            s_emptySaid = true, s_emptySaidAt = now;
            Log("Ammo: the game's own current weapon is in slot %d, which holds no weapon with a firing state just now: holsters take it as none "
                "read",
                empty);
        }
        static uintptr_t s_typeWas = 0, s_typesSaid[4] = {};
        static ULONGLONG s_typeSince = 0;
        if (unknownType != s_typeWas)
            s_typeWas = unknownType, s_typeSince = now;
        if (unknownType && now - s_typeSince > 2000 &&
            std::find(std::begin(s_typesSaid), std::end(s_typesSaid), unknownType) == std::end(s_typesSaid)) {
            for (uintptr_t& said : s_typesSaid) {
                if (said)
                    continue;
                said = unknownType;
                const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
                char type[32] = "";
                if (unknownType > base && unknownType - base < 0x4000000)
                    sprintf_s(type, "bf3.exe+%X", static_cast<unsigned>(unknownType - base));
                else
                    sprintf_s(type, "%08X", static_cast<unsigned>(unknownType));
                Log("Ammo: the game's own current weapon can't be read: its soldier's weapons keep an object of vtable %s where it's kept, neither "
                    "of the two known (bf3.exe+%X, +%X): holsters have no slot to go by while it's so",
                    type, static_cast<unsigned>(kSwitchingTypes[0]), static_cast<unsigned>(kSwitchingTypes[1]));
                break;
            }
        }
    }
    const pickrules::HandSlotOut out = pickrules::HandSlot(in);
    g_handSlot.store(out.slot, std::memory_order_relaxed);
    // (At most 8 of these lines in 5 s: a slot that flapped every frame would fill the log.)
    static ULONGLONG s_windowAt = 0;
    static int s_inWindow = 0;
    const ULONGLONG now = GetTickCount64();
    if (now - s_windowAt > 5000)
        s_windowAt = now, s_inWindow = 0;
    if ((out.slot != was || !g_handSaid) && s_inWindow < 8) {
        g_handSaid = true;
        ++s_inWindow;
        Log("Ammo: the gun in the hands is in slot %d: %s (the %s drawn; slot %d before; the game's words in slots 0 and 1: %d, %d; its own current "
            "weapon in slot %d)",
            out.slot, out.why, row ? row->name : in.drawn == pickrules::Drawn::kNothing ? "nothing" : "gun", was, in.word[0], in.word[1], in.current);
    }
    const int sure = in.drawn == pickrules::Drawn::kRifle || in.drawn == pickrules::Drawn::kHeld ? pickrules::WordSlot(in, true) : -1;
    if (sure >= 0 && sure != out.slot) {
        if (g_handWordsSaid != sure && s_inWindow < 8) {
            ++s_inWindow;
            Log("Ammo: the game's words say slot %d is in its hand, the gun in the hands is counted in slot %d (%s; its words %d, %d)", sure, out.slot,
                out.why, in.word[0], in.word[1]);
        }
        g_handWordsSaid = sure;
    } else {
        g_handWordsSaid = -1;
    }
    // The game's own current weapon in the other of the two slots than a gun drawn, a second on end (a switch is over sooner): said once.
    static ULONGLONG s_apartSince = 0;
    static bool s_apartSaid = false;
    if ((in.drawn == pickrules::Drawn::kRifle || in.drawn == pickrules::Drawn::kHeld) && (in.current == 0 || in.current == 1) && in.current != out.slot) {
        if (!s_apartSince)
            s_apartSince = now;
        if (!s_apartSaid && now - s_apartSince > 1000 && s_inWindow < 8) {
            s_apartSaid = true;
            ++s_inWindow;
            Log("Ammo: the game's own current weapon is in slot %d, the gun in the hands is counted in slot %d (%s; the %s drawn)", in.current, out.slot,
                out.why, row ? row->name : "gun");
        }
    } else {
        s_apartSince = 0, s_apartSaid = false;
    }
}

// ---- The train pistol's rounds by hand (pistolammo.h): its count each frame, and what the hands did ----

void PublishPistol() {
    const Pistol& p = g_pistol;
    g_pistolLive.store(p.firing != 0, std::memory_order_relaxed);
    g_pistolDry.store(p.firing && !p.state.chamber, std::memory_order_relaxed);
    g_pistolLocked.store(p.firing && p.state.locked, std::memory_order_relaxed);
    g_pistolMagIn.store(!p.firing || p.state.magIn, std::memory_order_relaxed);
    g_pistolLoaded.store(p.firing ? pistolammo::Total(p.state) : 0, std::memory_order_relaxed);
}

void ForgetPistol() {
    g_pistol = Pistol{};
    PublishPistol();
}

// The pistol in words: "3 in the magazine, one in the chamber".
const char* PistolWords(const pistolammo::State& s, char* out, size_t size) {
    char mag[48];
    if (s.magIn)
        sprintf_s(mag, "%d in the magazine", s.mag);
    else
        strcpy_s(mag, "the magazine out");
    std::snprintf(out, size, "%s, %s%s", mag, s.chamber ? "one in the chamber" : "the chamber empty", s.locked ? ", the slide on its stop" : "");
    return out;
}

// One look at the game's count of it (each frame, and right before and after each thing the hands did): shots, the game's own reload kept or
// undone, the hidden round put back, the hands' rounds written (pistolammo::See). said: the caller says what the hands did itself.
void StepPistol(bool said) {
    Pistol& p = g_pistol;
    int raw = 0, spare = 0;
    if (!p.firing || !ReadRaw(p.firing, raw, spare))
        return;
    Gun& gun = g_guns[p.firing];
    const ULONGLONG now = GetTickCount64();
    const unsigned long long rSince = std::min(input::GameReloadSince(), KeyRSince());
    pistolammo::CountIn in;
    in.last = gun.last;
    in.raw = raw;
    in.spare = spare;
    in.hidden = gun.hidden;
    // (A magazine seated by hand after X was last held: that hold's reload is the hand's now, and the game's, landing after it, is undone.)
    in.asked = (rSince < pickrules::kReloadAskedMs && (!p.seatedAt || rSince <= now - p.seatedAt)) ||
               (g_swappedAt && now - g_swappedAt < pickrules::kSwapAskedMs);
    in.putBacks = gun.putBacks;
    in.lost = p.lost;
    const pistolammo::CountOut out = pistolammo::See(p.state, in);
    bool wrote = false;
    if (out.write && WriteInt(p.firing + kLoaded, out.loaded)) {
        wrote = true;
        p.lost = 0;
        const int rawWas = raw, spareWas = spare;
        raw = out.loaded;
        if (out.spare != spare && WriteInt(p.firing + kSpare, out.spare))
            spare = out.spare;
        if (out.kept) {
            Log("Pistol: the game's own reload (X held): kept, %d rounds loaded (a magazine in, a round chambered), %d spare", raw - gun.hidden, spare);
        } else if (out.undone) {
            Log("Pistol: the game reloaded by itself, not asked for (no X held): undone, %d -> %d rounds as the game counts them, %d -> %d spare",
                rawWas, raw, spareWas, spare);
        } else if (out.putBack) {
            ++gun.putBacks;
            Log("Pistol: the game fired the hidden round (its count at 0): put back at once from the spare ones, %d -> %d spare (%d this magazine)",
                spareWas, spare, gun.putBacks);
        } else if (!said) {
            Log("Pistol: the game's count set to the mod's: %d -> %d rounds as the game counts them, %d -> %d spare", rawWas, raw, spareWas, spare);
        }
    }
    char words[128];
    if (out.fired > 0) {
        if (out.lockedNow)
            Log("Pistol: %d fired, the last round: empty, the slide stays open on its stop (%d spare): the trigger fires nothing till a magazine "
                "is in and the slide home",
                out.fired, spare);
        else if (!p.state.chamber)
            Log("Pistol: the chambered round fired with the magazine out: the chamber empty, the slide home (%d spare)", spare);
        else
            Log("Pistol: %d fired: %s (%d spare)", out.fired, PistolWords(p.state, words, sizeof(words)), spare);
    }
    if (out.hiddenFired && !out.putBack)
        Log("Pistol: the game fired the hidden round (its count at %d): not put back (%s)", raw,
            in.asked            ? "the game's own reload is asked for"
            : spare < 1         ? "no spare rounds"
            : !wrote && out.write ? "not written just now"
                                  : "put back too often: the game's reload, when it comes, is undone");
    if (out.adopted)
        Log("Pistol: the mod's count and the game's (%d, its hidden round in it) apart, and the %d spare rounds short of the difference: the pistol as "
            "the game's count has it now: %s",
            raw, spare, PistolWords(p.state, words, sizeof(words)));
    gun.written = true;
    gun.last = raw;
    if (raw >= 2)
        gun.putBacks = 0;
    // (God mode keeps the spare rounds up, as the rifles': testing the reload by hand over and over emptied them.)
    if (config::GetGod().enabled && spare < 60 && WriteInt(p.firing + kSpare, 300)) {
        Log("Pistol: spare rounds %d -> 300 (god mode keeps them up)", spare);
        spare = 300;
    }
    g_pistolSpare.store(spare, std::memory_order_relaxed);
    PublishPistol();
}

// Each frame (Update, the slots read): the pistol found and counted by the mod once it's in the hand with its rounds by hand; followed in its slot
// from then on (in the hand or not); given back to the game when its rounds are no longer by hand; forgotten once it has left the slots.
void UpdatePistol() {
    Pistol& p = g_pistol;
    const ULONGLONG now = GetTickCount64();
    if (p.firing) {
        int slot = -1;
        for (int s = 0; s < kSlots && slot < 0; ++s)
            if (g_slots[s].firing == p.firing && g_slots[s].weapon == p.weapon &&
                (g_slots[s].blueprint == p.blueprint || !g_slots[s].blueprint || !p.blueprint))
                slot = s;
        if (slot < 0) {
            // (Not the slots unread for a moment: forgotten then and met again, a second hidden round went on top of the first.)
            if (!p.goneAt)
                p.goneAt = now;
            if (now - p.goneAt > 1000) {
                Log("Pistol: no longer in the soldier's slots: its rounds no longer counted");
                g_guns.erase(p.firing);
                ForgetPistol();
            }
            return;
        }
        p.goneAt = 0;
        p.slot = slot;
        if (g_pistolByHand) {
            StepPistol(false);
            return;
        }
        // Its rounds the game's own again (pistol.txt, the hands turned off): the hidden round out of its count; empty, the game reloads it itself.
        int raw = 0, spare = 0;
        if (!ReadRaw(p.firing, raw, spare))
            return;
        Gun& gun = g_guns[p.firing];
        if (gun.hidden && raw >= 1) {
            if (!WriteInt(p.firing + kLoaded, raw - 1))
                return;
            raw -= 1;
        }
        gun.hidden = 0;
        gun.last = raw;
        Log("Pistol: its rounds the game's own again (build\\pistol.txt, or the hands off): %d loaded, %d spare%s", raw, spare,
            raw < 1 ? ": empty, the game reloads it by itself" : "");
        ForgetPistol();
        return;
    }
    if (!g_pistolByHand || !skin::TrainPistolInHand())
        return;
    // The train's pistol is in the list's first slot; the game's own current weapon, if that is a pistol by the guns' table.
    int slot = kRifleSlot;
    const int current = g_gameSlot.load(std::memory_order_relaxed);
    if (current > 0 && current < kSlots && g_slots[current].firing && g_slotNames[current].row &&
        g_slotNames[current].row->kind == recoil::Kind::kPistol)
        slot = current;
    const SlotRead& read = g_slots[slot];
    if (!read.firing)
        return;
    const recoilguns::Row* row = g_slotNames[slot].row;
    if (row && row->kind != recoil::Kind::kPistol) {
        static uintptr_t s_said = 0;
        if (s_said != read.blueprint)
            Log("Pistol: slot %d holds the %s, no pistol: the pistol's rounds aren't counted by the mod (the game's own reload, as before)", slot,
                row->name);
        s_said = read.blueprint;
        return;
    }
    int raw = 0, spare = 0;
    if (!ReadRaw(read.firing, raw, spare))
        return;
    const int capacity = std::max(1, (row ? row : &recoilguns::Default(recoil::Kind::kPistol))->magazine - 1);
    Gun& gun = g_guns[read.firing];
    int total = raw;
    const char* how = "a hidden round on top in the game's count";
    if (gun.hidden) {
        total = std::max(0, raw - 1);
        how = "its hidden round already in the game's count";
    } else if (raw > capacity + 1) {
        // (More than a magazine and the chambered round: a count a checkpoint brought back with the hidden round in it.)
        total = raw - 1;
        gun.hidden = 1;
        how = "the game's count a round above a full pistol: its hidden round already in it";
    } else {
        if (!WriteInt(read.firing + kLoaded, raw + 1))
            return;
        gun.hidden = 1;
        raw += 1;
    }
    gun.written = true;
    gun.last = raw;
    gun.putBacks = 0;
    p = Pistol{};
    p.firing = read.firing, p.weapon = read.weapon, p.blueprint = read.blueprint;
    p.slot = slot;
    p.capacity = capacity;
    p.state = pistolammo::FromCount(total);
    char words[128];
    Log("Pistol: its rounds by hand from now (slot %d, the %s): %d loaded (%s), %d spare; a magazine holds %d; %s, so the game never reloads it by "
        "itself",
        slot, row ? row->name : "pistol, not in the guns' table", total, PistolWords(p.state, words, sizeof(words)), spare, capacity, how);
    g_pistolSpare.store(spare, std::memory_order_relaxed);
    PublishPistol();
}

// ---- A tube shotgun's shells by hand (shotgunammo.h): its count each frame, and what the hands did ----

void PublishTube(int raw, int spare) {
    const Tube& t = g_tube;
    const bool live = t.firing != 0;
    g_tubeLive.store(live, std::memory_order_relaxed);
    g_tubeDry.store(live && (t.state.chamber != shotgunammo::Chamber::kLive || t.state.locked), std::memory_order_relaxed);
    g_tubeLocked.store(live && t.state.locked, std::memory_order_relaxed);
    g_tubeChambered.store(live && t.state.chamber == shotgunammo::Chamber::kLive, std::memory_order_relaxed);
    g_tubePort.store(live && t.state.port, std::memory_order_relaxed);
    g_tubeShells.store(live ? t.state.tube : 0, std::memory_order_relaxed);
    g_tubeCapacity.store(t.capacity, std::memory_order_relaxed);
    g_tubeCount.store(live ? shotgunammo::Total(t.state) : 0, std::memory_order_relaxed);
    g_tubeFiringNow.store(t.firing, std::memory_order_relaxed);
    g_tubeSlotNow.store(live ? t.slot : -1, std::memory_order_relaxed);
    if (live && raw >= 0) {
        g_tubeSpare.store(spare, std::memory_order_relaxed);
        shotgunammo::CountIn in;
        in.raw = raw, in.spare = spare, in.lost = t.lost;
        if (const auto it = g_guns.find(t.firing); it != g_guns.end())
            in.hidden = it->second.hidden;
        g_tubeBelly.store(shotgunammo::Belly(t.state, in), std::memory_order_relaxed);
    } else if (!live) {
        g_tubeSpare.store(0, std::memory_order_relaxed);
        g_tubeBelly.store(0, std::memory_order_relaxed);
        g_tubeGameReload.store(false, std::memory_order_relaxed);
        g_tubeHiddenOut.store(false, std::memory_order_relaxed);
    }
}

void ForgetTube() {
    g_tube = Tube{};
    // (X held as it was forgotten, by a swap, a death: not held for the next one; input.cpp's next hold says so afresh. When it was let go was a
    // global once, g_tubeAskLetGoAt: it is the tube's own now, tubeask::Key, and goes with the tube.)
    g_tubeAskHeld.store(false, std::memory_order_relaxed);
    g_tubeEndAsk.store(false, std::memory_order_relaxed);
    PublishTube(-1, 0);
    hands::TubeForgotten();  // (nothing of it left in a hand, its bolt as the next count has it)
}

// The shotgun in words: "3 in the tube, one in the chamber, the bolt home".
const char* TubeWords(const shotgunammo::State& s, char* out, size_t size) {
    std::snprintf(out, size, "%d in the tube, %s, %s%s", s.tube,
                  s.chamber == shotgunammo::Chamber::kLive ? "one in the chamber" : s.chamber == shotgunammo::Chamber::kSpent ? "a fired hull in the chamber" : "the chamber empty",
                  s.locked ? "the bolt held open" : "the bolt home", s.port ? ", a shell lying in the port" : "");
    return out;
}

// How long ago the game's own reload of this shotgun was last asked for: 0 while its key is down, else the ms since it was let go; ~0 never. The
// key is the tube's own (tubeask::KeySince): X held with the shotgun, the mod's own R or the keyboard's count only while the tube's slot is the
// hands' slot and the shotgun is the gun drawn in the hands, from a press begun there. (Read globally, a reload of another gun with the shotgun
// put away was the shotgun's asking for 3 s: drawn again dry inside them, its hidden round went out and the game reloaded it by itself.)
unsigned long long TubeAskSince(Tube& t, ULONGLONG now) {
    const bool keyDown = g_tubeAskHeld.load(std::memory_order_relaxed) || input::GameReloadSince() == 0 || KeyRSince() == 0;
    const bool inHands = t.slot == g_handSlot.load(std::memory_order_relaxed) && skin::HeldTubeGun() != nullptr && !skin::TrainPistolInHand();
    return tubeask::KeySince(t.key, inHands, keyDown, now);
}

// One look at the game's count of it (each frame, and right before and after each thing the hands did): shots, the game's own reload kept a shell
// a loop or given back, the hidden round out for it and back in, put back, the hands' shells written (shotgunammo::See). said: the caller says what
// the hands did itself.
void StepTube(bool said) {
    Tube& t = g_tube;
    int raw = 0, spare = 0;
    if (!t.firing || !ReadRaw(t.firing, raw, spare))
        return;
    Gun& gun = g_guns[t.firing];
    const ULONGLONG now = GetTickCount64();
    // The asking: X (or the reload key) only; a shot ends it; the latch for a reload begun on an empty gun (tubeask.h).
    const unsigned long long keySince = TubeAskSince(t, now);
    shotgunammo::CountIn in;
    in.last = gun.last;
    in.raw = raw;
    in.spare = spare;
    in.hidden = gun.hidden;
    in.putBacks = gun.putBacks;
    in.lost = t.lost;
    in.capacity = t.capacity;
    if (g_tubeEndAsk.exchange(false, std::memory_order_relaxed) && !gun.hidden) {
        tubeask::End(t.ask, keySince);
        Log("Shotgun: the trigger pulled on its last shell with the hidden round out (the game's own reload asked for): the asking ended, the hidden round back first");
    }
    const bool asked = tubeask::Before(t.ask, keySince, now, shotgunammo::Total(t.state), in);
    const shotgunammo::CountOut out = shotgunammo::See(t.state, in);
    const int rawWas = raw, spareWas = spare;
    bool wrote = false;
    if (!out.write) {
        gun.hidden = out.hidden;
        t.lost = out.lost;
    } else if (WriteInt(t.firing + kLoaded, out.loaded)) {
        wrote = true;
        gun.hidden = out.hidden;
        t.lost = out.lost;
        raw = out.loaded;
        if (out.spare != spare && WriteInt(t.firing + kSpare, out.spare))
            spare = out.spare;
    }
    char words[160];
    if (out.fired > 0 || out.firedTube > 0) {
        if (out.lockedNow) {
            g_tubeLockedShots.fetch_add(1, std::memory_order_relaxed);
            Log("Shotgun: %d fired, the last shell: empty, the bolt held open (%d spare): the trigger fires nothing till a shell is in and the bolt home", out.fired, spare);
        } else if (out.fired > 0) {
            Log("Shotgun: %d fired: %s (%d spare)", out.fired, TubeWords(t.state, words, sizeof(words)), spare);
        }
        if (out.firedTube > 0)
            Log("Shotgun: the game fired %d with nothing chambered (the mouse passes no trigger's stop): each took a shell of the tube's: %s", out.firedTube,
                TubeWords(t.state, words, sizeof(words)));
    }
    if (out.hiddenFired && !out.putBack)
        Log("Shotgun: the game fired the hidden round (its count at %d): not put back (%s)", raw,
            asked                  ? "the game's own reload is asked for: its first shell pays for it"
            : spare < 1            ? "no spare shells"
            : !wrote && out.write  ? "not written just now"
                                   : "put back too often: the game's reload, when it comes, is given back");
    if (out.putBack && wrote) {
        ++gun.putBacks;
        Log("Shotgun: the game fired the hidden round (its count at 0): put back at once from the spare ones, %d -> %d spare (%d since its count was last 2)", spareWas, spare,
            gun.putBacks);
    }
    if (out.hiddenOut && wrote)
        Log("Shotgun: the game's own reload asked for (X held): the hidden round out of its count, %d -> %d as the game counts them (%s; %d spare)", rawWas, raw,
            TubeWords(t.state, words, sizeof(words)), spare);
    if (out.gameLoaded > 0)
        Log("Shotgun: the game's own reload: %d shell%s kept%s%s: %s (%d spare)", out.gameLoaded, out.gameLoaded == 1 ? "" : "s",
            out.gameChambered ? ", the first into the chamber, the bolt home" : "", out.hiddenPaid ? " (its first paid for the hidden round fired before)" : "",
            TubeWords(t.state, words, sizeof(words)), spare);
    if (out.refused > 0)
        Log("Shotgun: the game's own reload: %d shell%s the tube has no room for (it holds %d): given back once its count has stopped rising", out.refused,
            out.refused == 1 ? "" : "s", t.capacity);
    if (out.undone)
        Log("Shotgun: the game reloads by itself, not asked for (no X held): its count %d -> %d: given back once it has stopped rising", in.last, rawWas);
    if (out.back > 0 && wrote)
        Log("Shotgun: %d shell%s of the game's own given back to the spare ones: %d -> %d as the game counts them, %d -> %d spare", out.back, out.back == 1 ? "" : "s", rawWas,
            raw, spareWas, spare);
    if (out.hiddenIn && wrote && !out.back)
        Log("Shotgun: the hidden round on top of the game's count: %d -> %d as the game counts them (%s; %d spare), so the game never reloads it by itself", rawWas, raw,
            TubeWords(t.state, words, sizeof(words)), spare);
    if (out.portBack)
        Log("Shotgun: the shell lying in the port is no more (the game's own reload, or no spare shell left behind it): back with the spare ones");
    if (out.adopted)
        Log("Shotgun: the mod's count and the game's (%d, its hidden round %s) apart, and the %d spare shells short of the difference: the gun as the game's count has "
            "it now: %s",
            raw, gun.hidden ? "in it" : "out", spare, TubeWords(t.state, words, sizeof(words)));
    if (wrote && out.settled && !said)
        Log("Shotgun: the game's count set to the mod's: %d -> %d as the game counts them, %d -> %d spare", rawWas, raw, spareWas, spare);
    gun.written = true;
    gun.last = raw;
    if (raw >= 2)
        gun.putBacks = 0;
    // (The asking's times and its latch, with the hidden round as this look left it.)
    tubeask::After(t.ask, out, asked, keySince, now, gun.hidden);
    g_tubeGameReload.store(asked || out.waiting || !gun.hidden, std::memory_order_relaxed);
    g_tubeHiddenOut.store(!gun.hidden, std::memory_order_relaxed);
    // (God mode keeps the spare shells up, as the rifles': testing the reload by hand over and over emptied them. Not while shells of the game's own wait to
    // be given back: their sum is the count's to settle.)
    if (config::GetGod().enabled && spare < 60 && !out.waiting && WriteInt(t.firing + kSpare, 300)) {
        Log("Shotgun: spare shells %d -> 300 (god mode keeps them up)", spare);
        spare = 300;
    }
    PublishTube(raw, spare);
}

// Each frame (Update, the slots read): the shotgun found and counted by the mod once it's drawn in the hands with its shells by hand; followed in its
// slot from then on (in the hands or not); given back to the game when its shells are no longer by hand; forgotten once it has left the slots.
void UpdateTube() {
    Tube& t = g_tube;
    const ULONGLONG now = GetTickCount64();
    if (t.firing) {
        int slot = -1;
        for (int s = 0; s < kSlots && slot < 0; ++s)
            if (g_slots[s].firing == t.firing && g_slots[s].weapon == t.weapon && (g_slots[s].blueprint == t.blueprint || !g_slots[s].blueprint || !t.blueprint))
                slot = s;
        if (slot < 0) {
            // (Not the slots unread for a moment: forgotten then and met again, a second hidden round went on top of the first.)
            if (!t.goneAt)
                t.goneAt = now;
            if (now - t.goneAt > 1000) {
                Log("Shotgun: no longer in the soldier's slots: its shells no longer counted");
                g_guns.erase(t.firing);
                ForgetTube();
            }
            return;
        }
        t.goneAt = 0;
        t.slot = slot;
        if (g_tubeByHand) {
            StepTube(false);
            return;
        }
        // Its shells the game's own again (weapon.txt "tubebyhand 0", the hands turned off): the hidden round out of its count; empty, the game reloads it.
        int raw = 0, spare = 0;
        if (!ReadRaw(t.firing, raw, spare))
            return;
        Gun& gun = g_guns[t.firing];
        if (gun.hidden && raw >= 1) {
            if (!WriteInt(t.firing + kLoaded, raw - 1))
                return;
            raw -= 1;
        }
        gun.hidden = 0;
        gun.last = raw;
        Log("Shotgun: its shells the game's own again (weapon.txt tubebyhand 0, or the hands off): %d loaded, %d spare%s", raw, spare,
            raw < 1 ? ": empty, the game reloads it by itself" : "");
        ForgetTube();
        return;
    }
    if (!g_tubeByHand || !skin::HeldTubeGun() || skin::TrainPistolInHand())
        return;
    // The gun drawn in the hands is in the hands' slot, and that slot's row is the drawn gun's (not a switch's frame, the slot still the gun before's).
    const int slot = g_handSlot.load(std::memory_order_relaxed);
    if (slot < 0 || slot >= kSlots)
        return;
    const SlotRead& read = g_slots[slot];
    const recoilguns::Row* row = g_slotNames[slot].row;
    const char* shape = skin::HeldShape();
    if (!read.firing || !row || !row->shape || !shape || std::strcmp(row->shape, shape) != 0 || row->kind != recoil::Kind::kShotgun)
        return;
    int raw = 0, spare = 0;
    if (!ReadRaw(read.firing, raw, spare))
        return;
    Gun& gun = g_guns[read.firing];
    // Its tube: its row's magazine less the chambered shell (4); the longer tube's two more when a count says so (7 or 8 seen in it: the game's own
    // capacity with that tube, and its hidden round on top).
    int capacity = std::max(1, row->magazine - 1);
    const int most = std::max(raw, gun.fullest);
    const bool longer = most >= row->magazine + 2 && most <= row->magazine + 3;
    if (longer)
        capacity += 2;
    // (More than the tube and the chambered shell: a count a checkpoint brought back with the hidden round in it. Swapped out by the game with that
    // round in its count and come back out of the pickup with the same count: it carried it, and isn't a shell richer for the trip.)
    const bool carried = g_tubeCameFiring && g_tubeCameFiring == static_cast<uint32_t>(read.firing) && raw == g_tubeCameRaw;
    if (g_tubeCameFiring == static_cast<uint32_t>(read.firing))
        g_tubeCameFiring = 0;
    const tubeask::Met met = tubeask::Meet(raw, capacity, gun.hidden, carried);
    const int total = met.total;
    gun.hidden = met.hidden;
    const char* how = met.how == 1   ? "its hidden round already in the game's count"
                      : met.how == 2 ? "the game's count a shell above a full gun: its hidden round already in it"
                      : met.how == 3 ? "back out of the pickup the game swapped it into with its hidden round in its count: that round already in it"
                                     : "its hidden round goes on top at the next look";
    gun.written = true;
    gun.last = raw;
    gun.putBacks = 0;
    t = Tube{};
    t.firing = read.firing, t.weapon = read.weapon, t.blueprint = read.blueprint;
    t.slot = slot;
    t.capacity = capacity;
    t.name = row->name;
    t.state = shotgunammo::FromCount(total);
    char words[160];
    Log("Shotgun: its rounds by hand from now (slot %d, the %s): %s; the tube holds %d%s, %d spare; %s, so the game never reloads it by itself", slot, row->name,
        TubeWords(t.state, words, sizeof(words)), capacity, longer ? " (the longer tube)" : "", spare, how);
    StepTube(true);
}

}

namespace ammo {

bool Get(int& loaded, int& spare) {
    uintptr_t firing = Firing();
    return firing && Read(firing, loaded, spare);
}

bool SecondaryCarried() {
    return FiringInSlot(1) != 0;
}

int HandSlot() {
    return g_handSlot.load(std::memory_order_relaxed);
}

void Picked(int slot) {
    if (slot == 0 || slot == 1)
        g_handPicked.store(slot, std::memory_order_relaxed);
}

int GameSlot() {
    return g_gameSlot.load(std::memory_order_relaxed);
}

bool SlotCarried(int slot) {
    return slot >= 0 && slot < kSlots && ((g_carried.load(std::memory_order_relaxed) >> slot) & 1u) != 0;
}

bool LauncherInSlot(int slot) {
    return slot >= 0 && slot < kSlots && ((g_launchers.load(std::memory_order_relaxed) >> slot) & 1u) != 0;
}

const char* NameInSlot(int slot) {
    return SlotCarried(slot) ? g_slotNames[slot].name : "";
}

const recoilguns::Row* RowInSlot(int slot) {
    return SlotCarried(slot) ? g_slotNames[slot].row : nullptr;
}

bool RoundsInHand(int& loaded) {
    int spare = 0;
    if (Get(loaded, spare))
        return true;
    // A tube shotgun the mod counts, in the hands' slot (its count is its own: Firing leaves it out).
    if (const uintptr_t tube = g_tubeFiringNow.load(std::memory_order_relaxed);
        tube && g_tubeSlotNow.load(std::memory_order_relaxed) == g_handSlot.load(std::memory_order_relaxed) && !skin::TrainPistolInHand() && skin::HeldTubeGun())
        return ReadRaw(tube, loaded, spare);
    // The train's pistol (docs/research/pistol-manual-reload.md): the list's first slot, or the one the mod counts it in.
    const uintptr_t firing = FiringInSlot(g_pistol.firing ? g_pistol.slot : kRifleSlot);
    return firing && ReadRaw(firing, loaded, spare);
}

void Update() {
    // Recoil's kick taken out of a gun's data goes back once the mod stops drawing it (free hands off, the headset not rendering): looked at each
    // Present, whatever is drawn.
    recoilrun::Watch();
    if (pickups::TakeOpen()) {
        // (The game's thread may be freeing these weapons: nothing read, nothing written, for a second at most. The slots stay as last read; the
        // game's own current weapon is none read meanwhile, so holsters bridge a moment on their last read and then decide nothing on it.)
        g_gameSlot.store(-1, std::memory_order_relaxed);
        return;
    }
    g_unwrittenSaid = false;
    uintptr_t soldier = god::PlayerServerSoldier();
    // A new life or a checkpoint: new guns, magazines in. Not the soldier unread for a moment: the guns forgotten then, a second hidden round went on top of the first.
    static ULONGLONG s_goneAt = 0;
    if (soldier)
        s_goneAt = 0;
    else if (g_soldier && !s_goneAt)
        s_goneAt = GetTickCount64();
    if (soldier ? soldier != g_soldier : g_soldier && GetTickCount64() - s_goneAt > 1000) {
        g_soldier = soldier;
        g_guns.clear();
        ForgetPistol();  // (met again below as the new life has it, its magazine in)
        ForgetTube();    // (the shotgun alike: met again as the new life's count has it)
        g_tubeWent = TubeWent{};
        g_tubeCameFiring = 0;
        skin::InsertMagazine();
        reload::ReleaseBolt();
        hands::HeldChambered(true);
        // (The hands' slot from the first again, its names read afresh; a magazine kept out for a gun of the last life given to none.)
        g_handSlot = 0;
        g_handLanded = -1;
        g_handPicked = -1;
        g_handSaid = false;
        for (SlotName& n : g_slotNames)
            n = SlotName{};
        g_havePendingMagOut = false;
        g_press = inputrules::PressWait{};  // (a hold of the life before waits for nothing)
        g_handOver = inputrules::HandOver{};
    }
    ReadSlots();
    SaySlots();
    UpdateHandSlot();
    // The train pistol with its rounds by hand (pistolammo.h): nothing done unless hands.cpp says so (SetPistolByHand) or it was counted so before.
    UpdatePistol();
    // A tube shotgun with its shells by hand (shotgunammo.h): likewise, once it's drawn in the hands.
    UpdateTube();
    // The launcher loaded by hand gets its hidden round: with its count at none the game reloads it by itself.
    if (g_launcherByHand && skin::LauncherInHand()) {
        if (const uintptr_t launcher = LauncherFiringState()) {
            int count = 0, rockets = 0;
            Gun& gun = g_guns[launcher];
            if (ReadRaw(launcher, count, rockets) && !gun.hidden && count >= 1) {
                gun.hidden = 1;
                gun.written = true;
                gun.fullest = 1;
                // It holds one: two already has the hidden round in it (a count a checkpoint brought back).
                if (count < 2) {
                    count += 1;
                    WriteInt(launcher + kLoaded, count);
                }
                Log("Ammo: the launcher, loaded by hand: %d rocket in it, %d spare", count - 1, rockets);
            }
            gun.last = count;
        }
    }
    // The Stinger, reloaded by the hand (flap.cpp) gets its hidden round the same way: with its count at none the game reloads it by itself.
    if (skin::HeldAerial()) {
        if (const uintptr_t stinger = StingerFiringState()) {
            int count = 0, missiles = 0;
            Gun& gun = g_guns[stinger];
            if (ReadRaw(stinger, count, missiles) && !gun.hidden && count >= 1) {
                gun.hidden = 1;
                gun.written = true;
                gun.fullest = 1;
                if (count < 2) {
                    count += 1;
                    WriteInt(stinger + kLoaded, count);
                }
                Log("Ammo: the Stinger, reloaded by hand: %d in it, %d spare", count - 1, missiles);
            }
            gun.last = count;
        }
    }
    // The launcher under the rifle: a gun of a round at a time in its slot (another level may have something else there).
    {
        const uintptr_t m320 = FiringInSlot(kM320Slot);
        int loaded = 0, rounds = 0, state = 0;
        const bool found = m320 && ReadRaw(m320, loaded, rounds) && loaded <= 1 && camera::SafeRead(m320 + kInHandState, &state, sizeof(state));
        // Its state (+0xA8): 0 before it's ever been in hand, 3 put away; in hand everything else: 1 coming up (the switch from the rifle), 2 ready, 5 firing, 10 and 11
        // reloading (seen 2026-11-05: picked, fired, the game's own reload, ready: 1, 5, 10, 11, 2).
        const int now = found ? state : -1;
        // (A study read +0xA4 as the state now, +0xA8 the one before it and +0xAC the next: +0xA8 reads on after a put-away. Both logged beside it till that's settled in
        // the game, the review 2026-11-05.)
        int stateA4 = -1, stateAC = -1;
        if (found) {
            camera::SafeRead(m320 + 0xA4, &stateA4, sizeof(stateA4));
            camera::SafeRead(m320 + 0xAC, &stateAC, sizeof(stateAC));
        }
        static int s_a4 = -2, s_ac = -2;
        if (const int was = g_m320State.exchange(now); was != now || stateA4 != s_a4 || stateAC != s_ac)
            Log("Ammo: the launcher under the rifle %s (its state %d; +0xA4 %d, +0xAC %d; %d loaded, %d spare)", now > 0 && now != 3 ? "in hand" : now >= 0 ? "not in hand" : "not found",
                now, stateA4, stateAC, loaded, rounds);
        s_a4 = stateA4, s_ac = stateAC;
        if (now > 0 && now != 3)
            g_m320UpAt = GetTickCount64();
        if (found) {
            if (m320 != g_m320Firing || g_m320Rounds < 0) {
                g_m320Firing = m320;
                g_m320Rounds = loaded;
                g_m320GameReload = false;
            }
            if (g_m320GameReload.load(std::memory_order_relaxed)) {
                if (loaded >= 1) {
                    g_m320Rounds = 1;
                    g_m320GameReload = false;
                    Log("Ammo: the launcher under the rifle reloaded by the game (%d spare)", rounds);
                }
            } else if (loaded == 0) {
                // A shot (or the game emptied it): one less as the mod counts it, the game's back to 1.
                g_m320Rounds = std::max(0, g_m320Rounds - 1);
                if (WriteInt(m320 + kLoaded, 1))
                    loaded = 1;
                Log("Ammo: the launcher under the rifle fired: %d in it as the mod counts, the game's count kept at 1 (%d spare)", g_m320Rounds, rounds);
            }
        }
        g_m320Loaded = found ? std::max(0, g_m320Rounds) : 0;
        g_m320Spare = found ? rounds : 0;
        // Its switch with the rifle instant, both ways (each checked as read before it's written).
        if (found) {
            InstantAltDeploy(m320, 0.55f, "launcher under the rifle", 0);
            InstantAltDeploy(FiringInSlot(kRifleSlot), 0.67f, "rifle from the launcher under it", 1);
            // (The M416 back in the hands from slot 1, taken up again with another gun in its place: its own data checked the same way.)
            if (g_handSlot.load(std::memory_order_relaxed) == 1 && DrawnNow() == pickrules::Drawn::kRifle)
                InstantAltDeploy(FiringInSlot(1), 0.67f, "rifle from the launcher under it", 1);
        }
    }
    uintptr_t firing = Firing();
    int raw = 0, spare = 0;
    if (!firing || !ReadRaw(firing, raw, spare)) {
        // (No gun's count this frame: a held press's wait only times out.)
        const inputrules::PressWait hold = g_press;
        PressFrame(inputrules::StepPress(g_press, GetTickCount64(), g_press.gun, false), hold, -1, -1);
        return;
    }
    // Its weapon and blueprint too: another gun at the same address (a swap freed one and the game put the next there) is a new gun. Read again with
    // another firing state (it changed between the two reads): taken as they were.
    uintptr_t weapon = 0, blueprint = 0;
    if (FiringInSlot(g_handSlot.load(std::memory_order_relaxed), weapon, blueprint) != firing)
        weapon = g_weapon, blueprint = g_blueprint;
    const bool newGun = firing != g_firing || weapon != g_weapon || blueprint != g_blueprint;
    if (newGun && firing == g_firing)
        g_guns.erase(firing);
    Gun& gun = g_guns[firing];
    if (newGun) {
        Log("Ammo: the gun in hand has %d rounds loaded, %d spare", raw - gun.hidden, spare);
        g_firingSince = GetTickCount64();
        // A magazine out is that gun's: the next has its own in (the M416's out, a dead enemy's gun taken up), and it's still out when this one comes back.
        // (Another gun at the same address: the old one's record is gone.)
        if (const auto was = g_guns.find(g_firing); firing != g_firing && was != g_guns.end())
            was->second.magOut = skin::MagazineOut();
        // Out of a pickup the game had swapped it into: as it went in (Swapped), once the gun that came out of it is the one in the hands.
        if (g_havePendingMagOut && (!g_pendingFiring || g_pendingFiring == static_cast<uint32_t>(firing) ||
                                    g_pendingSlot == g_handSlot.load(std::memory_order_relaxed))) {
            gun.magOut = g_pendingMagOut;
            g_havePendingMagOut = false;
        }
        skin::SetMagazineOut(gun.magOut);
    }
    g_firing = firing;
    g_weapon = weapon;
    g_blueprint = blueprint;
    // The game's own count, by the mod's rule (pickrules::SeeCount). Its reload asked for (R held on X): its count up by a magazine without the mod
    // writing it; a magazine and the chambered round its size, and the hidden round goes back on top. Not asked for, with the magazine by hand: undone,
    // from any count (the game never reloads it by itself). Its hidden round fired: put back at once, from the spare ones. (Here, on the thread that
    // presents the frame, after the game's own count changed: the game may have started its reload already; the undo covers that.)
    pickrules::CountIn seen;
    seen.last = gun.last;
    seen.raw = raw;
    seen.spare = spare;
    seen.hidden = gun.hidden;
    seen.byHand = (skin::RifleInHand() || skin::HeldByHand()) && DrawnNow() != pickrules::Drawn::kOwnPath;
    const unsigned long long rSince = std::min(input::GameReloadSince(), KeyRSince());  // (the keyboard's looked at every frame)
    const bool asked = gun.emptied || rSince < pickrules::kReloadAskedMs ||
                       (g_swappedAt && GetTickCount64() - g_swappedAt < pickrules::kSwapAskedMs);
    // (A magazine pushed home by hand while X held's wait ran: that hold's reload is the hand's now, and the game's, landing after it, is undone.)
    seen.asked = inputrules::StillAsked(g_handOver, asked, newGun ? 0 : static_cast<uint64_t>(firing), rSince);
    const bool overHand = asked && !seen.asked;
    seen.putBacks = gun.putBacks;
    const pickrules::CountOut count = pickrules::SeeCount(seen);
    if (count.what == pickrules::CountSeen::kKept) {
        gun.learned = raw - 1;
        if (gun.hidden) {
            WriteInt(firing + kLoaded, raw + gun.hidden);
            raw += gun.hidden;
        }
        Log("Ammo: the game reloaded by itself: %d rounds loaded (a magazine holds %d)", raw - gun.hidden,
            gun.learned);
        skin::InsertMagazine();  // it put one in, whatever the hand did meanwhile
    } else if (count.what == pickrules::CountSeen::kUndone) {
        // (The count first: back at its hidden round before the spare ones come back, the game's next look finds nothing to reload.)
        if (!WriteInt(firing + kLoaded, count.loaded) || !WriteInt(firing + kSpare, count.spare))
            return;  // (not written: the same jump looked at again next frame)
        Log("Ammo: the game reloaded by itself, %s: undone, %d -> %d rounds as the game counts them, %d -> %d spare (slot %d)",
            overHand ? "on top of a magazine pushed home by hand while X held waited for it" : "not asked for (no X held)", raw, count.loaded, spare,
            count.spare, g_handSlot.load(std::memory_order_relaxed));
        raw = count.loaded;
        spare = count.spare;
    } else if (count.what == pickrules::CountSeen::kPutBack) {
        if (WriteInt(firing + kLoaded, count.loaded) && WriteInt(firing + kSpare, count.spare)) {
            ++gun.putBacks;
            Log("Ammo: the game fired the hidden round (its count at 0): put back at once from the spare ones, %d -> %d spare (slot %d, %d this magazine)",
                spare, count.spare, g_handSlot.load(std::memory_order_relaxed), gun.putBacks);
            raw = count.loaded;
            spare = count.spare;
        }
    }
    // X held (HeldPress): the reload drawn once the game's own count of this gun rises, by any number of rounds (the mod's own writes leave its last count
    // as they wrote it, so they're no rise; nor is a reload undone or a hidden round put back).
    {
        const bool gameRose = seen.last >= 0 && seen.raw > seen.last && count.what != pickrules::CountSeen::kUndone &&
                              count.what != pickrules::CountSeen::kPutBack;
        const inputrules::PressWait hold = g_press;
        PressFrame(inputrules::StepPress(g_press, GetTickCount64(), newGun ? 0 : static_cast<uint64_t>(firing), gameRose), hold, std::max(0, raw - gun.hidden),
                   spare);
    }
    if (raw >= 2)
        gun.putBacks = 0;
    if (!gun.written)
        gun.fullest = std::max(gun.fullest, raw);
    // The rifle reloaded by hand gets its hidden round, as does any other gun with its magazine by hand. One taken up empty from the ground too, with
    // spare rounds to reload by hand (the mod's count stays 0, so the trigger stays stopped): with none the game would reload it by itself. Not after
    // weapon.txt "empty", which leaves it for the game's own reload.
    if (raw >= 1)
        gun.emptied = false;
    if (!gun.hidden && (skin::RifleInHand() || skin::HeldByHand()) && (raw >= 1 || (spare > 0 && !gun.emptied))) {
        if (raw < 1)
            Log("Ammo: the gun in the hands is empty (%d spare): a hidden round in it, so the game doesn't reload it by itself", spare);
        gun.hidden = 1;
        gun.written = true;
        raw += 1;
        WriteInt(firing + kLoaded, raw);
    }
    gun.last = raw;
    // With god mode on, the spare rounds don't run out: testing the reloads by hand over and over
    // emptied them, and with none left the gun stayed locked back (2026-10-30).
    if (config::GetGod().enabled && spare < 60) {
        WriteInt(firing + kSpare, 300);
        Log("Ammo: spare rounds %d -> 300 (god mode keeps them up)", spare);
    }
}

void Eject() {
    uintptr_t firing = Firing();
    int loaded = 0, spare = 0;
    if (!firing || !Read(firing, loaded, spare)) {
        Log("Magazine: the gun's rounds weren't found; only the picture changes");
        return;
    }
    // One round stays (the chambered one, or the one an empty gun keeps), from the spare if the game fired it (pickrules::StaysAtEject).
    const int kept = pickrules::StaysAtEject(loaded, spare);
    Write(firing, kept, spare + loaded - kept);
    if (loaded < 1 && kept)
        Log("Magazine: out; the gun was empty: one round from the spare ones left in it, as the trigger's stop keeps (%d spare)", spare - kept);
    else
        Log("Magazine: out; %d rounds back to the spare ones, %d left in the chamber", loaded - kept, kept);
}

void Insert() {
    uintptr_t firing = Firing();
    int loaded = 0, spare = 0;
    if (!firing || !Read(firing, loaded, spare)) {
        Log("Magazine: the gun's rounds weren't found; only the picture changes");
        return;
    }
    int take = std::min(Capacity(), spare);
    Write(firing, loaded + take, spare - take);
    Log("Magazine: in; %d rounds loaded, %d spare (a magazine holds %d)", loaded + take, spare - take, Capacity());
    // (A held press still waiting for the game's reload: the hand loaded it first. The game's own reload, if it still comes, would land on top of the
    // hand's magazine: undone then, its rounds back to the spare ones.)
    if (g_press.at) {
        Log("Ammo: X held: a magazine pushed home by hand %.2f s after the hold, before the game's count rose: its wait ends (the game's reload, if it "
            "comes now, is undone)",
            (GetTickCount64() - g_press.at) / 1000.0);
        g_handOver = inputrules::HandOver{};
        g_handOver.gun = g_press.gun;
        g_press = inputrules::PressWait{};
    }
}

void HeldPress() {
    // Only the guns whose rounds are counted here (the M416, a held gun): the train pistol, the launchers and the Stinger have their own at the hold.
    const pickrules::Drawn drawn = DrawnNow();
    const uintptr_t firing = Firing();
    int loaded = 0, spare = 0;
    if ((drawn != pickrules::Drawn::kRifle && drawn != pickrules::Drawn::kHeld) || !firing || !Read(firing, loaded, spare))
        return;
    g_handOver = inputrules::HandOver{};  // (a new hold asks for the game's reload again)
    g_press.at = GetTickCount64();
    g_press.gun = static_cast<uint64_t>(firing);
    g_press.loaded = loaded;
    g_press.kind = drawn == pickrules::Drawn::kRifle ? inputrules::kPressRifle : inputrules::kPressHeld;
    Log("Ammo: X held: the game's own reload asked for (%d rounds loaded, %d spare): the magazine, the bolt and the chamber wait for its count to rise", loaded,
        spare);
}

int MagazineSize() {
    return Capacity();
}

void SetPistolByHand(bool on) {
    g_pistolByHand = on;
}

bool PistolLive() {
    return g_pistolLive.load(std::memory_order_relaxed);
}

bool PistolDry() {
    return g_pistolDry.load(std::memory_order_relaxed);
}

bool PistolLocked() {
    return g_pistolLocked.load(std::memory_order_relaxed);
}

bool PistolMagIn() {
    return g_pistolMagIn.load(std::memory_order_relaxed);
}

int PistolSpare() {
    return g_pistolSpare.load(std::memory_order_relaxed);
}

int PistolLoaded() {
    return g_pistolLoaded.load(std::memory_order_relaxed);
}

bool PistolEject() {
    Pistol& p = g_pistol;
    if (!p.firing)
        return false;
    StepPistol(false);  // (a shot the game made since the last look comes first)
    const int back = pistolammo::Eject(p.state);
    if (back < 0) {
        Log("Pistol: the magazine out by hand: none was in as the mod counts it");
        return true;
    }
    StepPistol(true);
    Log("Pistol: the magazine out by hand: %d round%s back to the spare ones with it, %s%s; %d spare", back, back == 1 ? "" : "s",
        p.state.chamber ? "one left in the chamber" : "the chamber empty", p.state.locked ? ", the slide on its stop" : "",
        g_pistolSpare.load(std::memory_order_relaxed));
    return true;
}

bool PistolInsert() {
    Pistol& p = g_pistol;
    if (!p.firing)
        return false;
    StepPistol(false);
    const int take = pistolammo::Insert(p.state, p.capacity, g_pistolSpare.load(std::memory_order_relaxed));
    if (take < 0) {
        Log("Pistol: a magazine seated by hand: one was in already as the mod counts it");
        return true;
    }
    p.seatedAt = GetTickCount64();
    StepPistol(true);
    Log("Pistol: a magazine seated by hand: %d round%s in it from the spare ones (it holds %d), %s; %d loaded, %d spare", take, take == 1 ? "" : "s",
        p.capacity,
        p.state.chamber  ? "one in the chamber"
        : p.state.locked ? "the chamber empty, the slide on its stop: the trigger's next pull, or a tug of the slide, sends it home"
                         : "the chamber empty, the slide home: racked, it chambers a round",
        pistolammo::Total(p.state), g_pistolSpare.load(std::memory_order_relaxed));
    return true;
}

pistolammo::Racked PistolRack(bool byTrigger) {
    Pistol& p = g_pistol;
    pistolammo::Racked r;
    if (!p.firing)
        return r;
    StepPistol(false);
    r = pistolammo::Rack(p.state);
    if (r.threw)
        ++p.lost;
    StepPistol(true);
    char words[128];
    Log("Pistol: the slide %s: %s%s; %s (%d spare)",
        byTrigger     ? (r.wasLocked ? "sent home off its stop by the trigger" : "cycled by the trigger")
        : r.wasLocked ? "tugged off its stop by hand"
                      : "racked by hand",
        r.threw ? "the chambered round thrown out (lost), " : "",
        r.chambered ? "a round chambered from the magazine"
        : r.locked  ? "an empty magazine in: the stop holds it open"
                    : "no magazine in: home on an empty chamber",
        PistolWords(p.state, words, sizeof(words)), g_pistolSpare.load(std::memory_order_relaxed));
    return r;
}

pistolammo::Pull PistolPull(bool slideByHand) {
    Pistol& p = g_pistol;
    if (!p.firing)
        return pistolammo::Pull::kFires;
    StepPistol(false);
    const pistolammo::Pull pull = pistolammo::OnPull(p.state, slideByHand);
    if (pull == pistolammo::Pull::kSendsHome || pull == pistolammo::Pull::kRacks)
        PistolRack(true);
    else if (pull == pistolammo::Pull::kClick)
        Log("Pistol: the trigger pulled on an empty chamber: nothing fires (%s; %d spare)",
            !p.state.magIn    ? "the magazine is out: seat a fresh one"
            : p.state.mag < 1 ? "the magazine is empty: X drops it, a fresh one from the belly"
                              : "rounds in the magazine, the slide home: rack it by hand",
            g_pistolSpare.load(std::memory_order_relaxed));
    return pull;
}

void SetTubeByHand(bool on) {
    g_tubeByHand = on;
}

bool TubeLive() {
    return g_tubeLive.load(std::memory_order_relaxed);
}

bool TubeDry() {
    return g_tubeDry.load(std::memory_order_relaxed);
}

bool TubeLocked() {
    return g_tubeLocked.load(std::memory_order_relaxed);
}

bool TubeChambered() {
    return g_tubeChambered.load(std::memory_order_relaxed);
}

bool TubePort() {
    return g_tubePort.load(std::memory_order_relaxed);
}

int TubeShells() {
    return g_tubeShells.load(std::memory_order_relaxed);
}

int TubeCapacity() {
    return g_tubeCapacity.load(std::memory_order_relaxed);
}

int TubeCount() {
    return g_tubeCount.load(std::memory_order_relaxed);
}

int TubeSpare() {
    return g_tubeSpare.load(std::memory_order_relaxed);
}

int TubeBelly() {
    return g_tubeBelly.load(std::memory_order_relaxed);
}

shotgunammo::Take TubeCanTake(int inHand) {
    const Tube& t = g_tube;
    if (!t.firing)
        return shotgunammo::Take::kYes;
    return shotgunammo::CanTake(t.state, t.capacity, g_tubeBelly.load(std::memory_order_relaxed), inHand);
}

bool TubeGameReload() {
    return g_tubeGameReload.load(std::memory_order_relaxed);
}

bool TubeHiddenOut() {
    return g_tubeLive.load(std::memory_order_relaxed) && g_tubeHiddenOut.load(std::memory_order_relaxed);
}

void TubeEndAsk() {
    g_tubeEndAsk.store(true, std::memory_order_relaxed);
}

unsigned TubeLockedShots() {
    return g_tubeLockedShots.load(std::memory_order_relaxed);
}

void TubeAsk(bool held) {
    g_tubeAskHeld.store(held, std::memory_order_relaxed);  // (when it was let go is the tube's own to keep: tubeask::KeySince)
}

shotgunammo::Load TubeLoadShell() {
    Tube& t = g_tube;
    if (!t.firing)
        return shotgunammo::Load::kNoShell;
    StepTube(false);  // (a shot the game made since the last look comes first)
    if (g_tubeGameReload.load(std::memory_order_relaxed)) {
        // (Shells of the game's own are landing or waiting to be given back: the hand's shell waits with them, shotgunammo.h.)
        Log("Shotgun: a shell pushed home by hand while the game's own reload runs: not counted (it stays a spare one)");
        return shotgunammo::Load::kNoShell;
    }
    const shotgunammo::Load load = shotgunammo::LoadTube(t.state, t.capacity, g_tubeBelly.load(std::memory_order_relaxed));
    StepTube(true);
    char words[160];
    if (load == shotgunammo::Load::kIn)
        Log("Shotgun: a shell home in the tube: %d of %d (%s), %d spare", t.state.tube, t.capacity, TubeWords(t.state, words, sizeof(words)),
            g_tubeSpare.load(std::memory_order_relaxed));
    else
        Log("Shotgun: a shell pushed home by hand not counted: %s", load == shotgunammo::Load::kFull ? "the tube is full" : "no shell left at the belly for it");
    return load;
}

shotgunammo::Load TubePortLoad() {
    Tube& t = g_tube;
    if (!t.firing)
        return shotgunammo::Load::kNoShell;
    StepTube(false);
    const shotgunammo::Load load = shotgunammo::PortLoad(t.state, g_tubeBelly.load(std::memory_order_relaxed));
    StepTube(true);
    if (load == shotgunammo::Load::kIn)
        Log("Shotgun: a shell dropped into the ejection port: it lies on the carrier (counted when the bolt takes it; %d spare)", g_tubeSpare.load(std::memory_order_relaxed));
    else
        Log("Shotgun: a shell at the ejection port not laid in: %s",
            load == shotgunammo::Load::kBoltHome ? "the bolt isn't held open" : load == shotgunammo::Load::kTaken ? "one lies there already, or the chamber isn't empty"
                                                                                                                  : "no shell left at the belly for it");
    return load;
}

shotgunammo::Bolt TubeBoltHome() {
    Tube& t = g_tube;
    shotgunammo::Bolt b;
    if (!t.firing)
        return b;
    StepTube(false);
    b = shotgunammo::BoltHome(t.state);
    StepTube(true);
    char words[160];
    if (b.moved)
        Log("Shotgun: the bolt home off its hold: %s; %s (%d spare)",
            b.fromPort ? "the port's shell chambered" : b.fromTube ? "one from the tube chambered" : "on an empty chamber", TubeWords(t.state, words, sizeof(words)),
            g_tubeSpare.load(std::memory_order_relaxed));
    return b;
}

shotgunammo::Bolt TubeRack() {
    Tube& t = g_tube;
    shotgunammo::Bolt b;
    if (!t.firing)
        return b;
    StepTube(false);
    b = shotgunammo::Rack(t.state);
    if (b.threw)
        ++t.lost;
    StepTube(true);
    char words[160];
    Log("Shotgun: the handle drawn back and let go: %s%s; %s (%d spare)",
        b.wasLocked ? "the bolt home off its hold, " : b.threw ? "racked, a live shell thrown out (lost), " : "racked, ",
        b.fromPort ? "the port's shell chambered" : b.fromTube ? "one from the tube chambered" : b.locked ? "the tube empty: the bolt held open" : "on an empty chamber",
        TubeWords(t.state, words, sizeof(words)), g_tubeSpare.load(std::memory_order_relaxed));
    return b;
}

shotgunammo::Pull TubePull(bool handleHeld) {
    Tube& t = g_tube;
    if (!t.firing)
        return shotgunammo::Pull::kFires;
    StepTube(false);
    const shotgunammo::Pull pull = shotgunammo::Pulled(t.state, handleHeld);
    if (pull != shotgunammo::Pull::kFires)
        Log("Shotgun: the trigger didn't fire: %s (%d in the tube, %d spare)%s",
            handleHeld       ? "the handle is held"
            : t.state.locked ? "the bolt is held open"
                             : "nothing in the chamber",
            t.state.tube, g_tubeSpare.load(std::memory_order_relaxed), pull == shotgunammo::Pull::kClick ? ": a click" : "");
    PublishTube(-1, 0);
    return pull;
}

bool LauncherGet(int& loaded, int& spare) {
    const uintptr_t firing = LauncherFiringState();
    return firing && Read(firing, loaded, spare);
}

void LauncherInsert() {
    const uintptr_t firing = LauncherFiringState();
    int loaded = 0, spare = 0;
    if (!firing || !Read(firing, loaded, spare)) {
        Log("Launcher: its rockets weren't found; only the picture changes");
        return;
    }
    if (spare < 1) {
        Log("Launcher: no rockets left to load");
        return;
    }
    Write(firing, loaded + 1, spare - 1);
    Log("Launcher: a rocket in; %d loaded, %d spare", loaded + 1, spare - 1);
}

bool LauncherAtNone() {
    const uintptr_t firing = LauncherFiringState();
    int count = 0, rockets = 0;
    if (!firing || !ReadRaw(firing, count, rockets) || count != 0)
        return false;
    const auto it = g_guns.find(firing);
    return it == g_guns.end() || !it->second.hidden;
}

void LauncherGameReload() {
    const uintptr_t firing = LauncherFiringState();
    int loaded = 0, spare = 0;
    if (!firing || !Read(firing, loaded, spare))
        return;
    Gun& gun = g_guns[firing];
    gun.hidden = 0;
    Write(firing, loaded, spare);
    Log("Launcher: the game's own reload (%d loaded, %d spare)", loaded, spare);
}

void SetLauncherByHand(bool on) {
    g_launcherByHand = on;
}

bool StingerGet(int& loaded, int& spare, unsigned& which) {
    const uintptr_t firing = StingerFiringState();
    which = static_cast<unsigned>(firing);
    return firing && Read(firing, loaded, spare);
}

void StingerLoad() {
    const uintptr_t firing = StingerFiringState();
    int loaded = 0, spare = 0;
    if (!firing || !Read(firing, loaded, spare) || spare < 1)
        return;
    Write(firing, loaded + 1, spare - 1);
    Log("Ammo: the Stinger: %d in it, %d spare", loaded + 1, spare - 1);
}

void StingerOut() {
    const uintptr_t firing = StingerFiringState();
    if (!firing)
        return;
    // Nothing left: its true count, and the game goes back to the rifle by itself.
    Gun& gun = g_guns[firing];
    gun.hidden = 0;
    Write(firing, 0, 0);
    Log("Ammo: the Stinger's last missile gone");
}

bool RifleFiringInHand() {
    // Just switched (to the launcher, or another gun while the rifle is still drawn half a second):
    // not yet, so a magazine in the hand goes with the switch.
    if (!skin::RifleInHand() || !g_firing || g_firing != Firing() || GetTickCount64() - g_firingSince < 1500)
        return false;
    // The launcher under it is seen with a round or two at most; the rifle's own with a magazine.
    auto it = g_guns.find(g_firing);
    return it != g_guns.end() && std::max(it->second.fullest, it->second.learned) > 2;
}

bool M320Up() {
    const int state = g_m320State.load(std::memory_order_relaxed);
    return state > 0 && state != 3;
}

int M320State() {
    return g_m320State.load(std::memory_order_relaxed);
}

bool M320Carried() {
    return g_m320State.load(std::memory_order_relaxed) >= 0;
}

bool M320UpOrRecent() {
    return M320Up() || GetTickCount64() - g_m320UpAt.load(std::memory_order_relaxed) < 1500;
}

bool M320GameReload() {
    const uintptr_t firing = FiringInSlot(kM320Slot);
    int loaded = 0, spare = 0;
    if (!firing || firing != g_m320Firing || !ReadRaw(firing, loaded, spare) || loaded > 1 || spare < 1)
        return false;
    g_m320GameReload = true;
    WriteInt(firing + kLoaded, 0);
    Log("Ammo: the launcher under the rifle left empty for the game's own reload (%d spare)", spare);
    return true;
}

void M320LoadByHand() {
    const uintptr_t firing = FiringInSlot(kM320Slot);
    int loaded = 0, spare = 0;
    if (!firing || firing != g_m320Firing || !ReadRaw(firing, loaded, spare) || loaded > 1 || spare < 1 || g_m320Rounds >= 1)  // (one in it already: none taken)
        return;
    g_m320Rounds = 1;
    WriteInt(firing + kSpare, spare - 1);
    Log("Ammo: a round into the launcher under the rifle by hand (%d spare)", spare - 1);
}

void M320Unload() {
    const uintptr_t firing = FiringInSlot(kM320Slot);
    int loaded = 0, spare = 0;
    if (!firing || firing != g_m320Firing || !ReadRaw(firing, loaded, spare) || loaded > 1 || g_m320Rounds < 1)
        return;
    g_m320Rounds = 0;
    WriteInt(firing + kSpare, spare + 1);
    Log("Ammo: a live round out of the launcher under the rifle, back to the spare ones (%d spare)", spare + 1);
}

int M320Loaded() {
    return g_m320Loaded.load(std::memory_order_relaxed);
}

int M320Spare() {
    return g_m320Spare.load(std::memory_order_relaxed);
}

uintptr_t FiringOf(int slot) {
    return slot >= 0 && slot < 8 ? FiringInSlot(slot) : 0;
}

bool RoundsInSlot(int slot, int& loaded) {
    const uintptr_t firing = FiringOf(slot);
    int spare = 0;
    return firing && ReadRaw(firing, loaded, spare);
}

unsigned CountWrites() {
    return g_countWrites.load(std::memory_order_relaxed);
}

bool HandOverCount(int slot, uint32_t& firing, int& loaded) {
    firing = 0;
    const uintptr_t state = slot >= 0 && slot < pickups::kMostSlots ? FiringInSlot(slot) : 0;
    if (!state)
        return false;
    firing = static_cast<uint32_t>(state);
    // The grenade launcher under the rifle: the mod's own count (the game's is kept at 1).
    if (slot == kM320Slot && state == g_m320Firing && g_m320Rounds >= 0) {
        loaded = g_m320Rounds;
        return true;
    }
    int raw = 0, spare = 0;
    if (!ReadRaw(state, raw, spare))
        return false;
    const auto it = g_guns.find(state);
    loaded = std::max(0, raw - (it != g_guns.end() ? it->second.hidden : 0));
    return true;
}

void Swapped(const pickups::Swap& swap) {
    g_swappedAt = GetTickCount64();
    const int changed = std::clamp(swap.changed, 0, pickups::kMostSlots);
    const int handSlot = g_handSlot.load(std::memory_order_relaxed);
    // The gun that went out of the first two slots into the pickup: its magazine out or in, kept by that pickup and its name (the gun in the hands'
    // is skin's state; the other's as it was left when another was taken in hand, if it was ever met). None when the pickup went with the take.
    for (int k = 0; k < changed; ++k) {
        if ((swap.slot[k] != 0 && swap.slot[k] != 1) || !swap.went[k][0] || swap.removed)
            continue;
        bool magOut = false;
        if (swap.slot[k] == handSlot) {
            magOut = skin::MagazineOut();
        } else if (const auto it = g_guns.find(swap.wentFiring[k]); swap.wentFiring[k] && it != g_guns.end()) {
            magOut = it->second.magOut;
        } else {
            continue;
        }
        KeptMagazine& kept = g_keptMagazines[g_keptNext];
        g_keptNext = (g_keptNext + 1) % static_cast<int>(std::size(g_keptMagazines));
        kept.entity = swap.entity;
        std::memcpy(kept.name, swap.went[k], sizeof(kept.name));
        kept.name[sizeof(kept.name) - 1] = 0;
        kept.magOut = magOut;
        kept.swap = swap.serial ? swap.serial : ~0u;
    }
    // Where the take put its gun (the slot of the first two that came to hold the taken entry's name; else the one of them it changed): the tie-breaker
    // for the hands' slot.
    {
        int landed = -1, changedOfTwo = 0, onlyOfTwo = -1;
        for (int k = 0; k < changed; ++k) {
            if ((swap.slot[k] != 0 && swap.slot[k] != 1) || !swap.came[k][0])
                continue;
            ++changedOfTwo, onlyOfTwo = swap.slot[k];
            if (landed < 0 && swap.taken.name[0] && recoilguns::SameName(swap.came[k], swap.taken.name))
                landed = swap.slot[k];
        }
        if (landed < 0 && changedOfTwo == 1)
            landed = onlyOfTwo;
        if (landed >= 0)
            g_handLanded = landed, g_handPicked = -1;  // (a pick made while the take was open came before it)
    }
    // The shotgun the mod counts, swapped out by the game itself with its hidden round in its count: the pickup carries that round (a hand's take
    // wrote its count without it: HandOverCount). Before its record goes below.
    for (int k = 0; k < changed && g_tube.firing && !swap.byHand; ++k) {
        if (swap.wentFiring[k] != static_cast<uint32_t>(g_tube.firing) || !swap.went[k][0])
            continue;
        const auto it = g_guns.find(g_tube.firing);
        if (it == g_guns.end() || !it->second.hidden || it->second.last < 1)
            continue;
        g_tubeWent = TubeWent{};
        g_tubeWent.have = true;
        g_tubeWent.entity = swap.removed ? 0 : swap.entity;
        g_tubeWent.swap = swap.serial ? swap.serial : ~0u;
        std::memcpy(g_tubeWent.name, swap.went[k], sizeof(g_tubeWent.name));
        g_tubeWent.name[sizeof(g_tubeWent.name) - 1] = 0;
        g_tubeWent.raw = it->second.last;
        Log("Shotgun: swapped out by the game with its hidden round in its count (%d as the game counts): taken back with that count, it's met as carrying it",
            g_tubeWent.raw);
    }
    // ... and come back out of that pickup (never by this same swap): its new firing state is the one that carries it.
    for (int k = 0; k < changed && g_tubeWent.have; ++k) {
        if (!swap.came[k][0] || !swap.cameFiring[k] || g_tubeWent.swap == (swap.serial ? swap.serial : ~0u) ||
            (g_tubeWent.entity && g_tubeWent.entity != swap.entity) || std::strncmp(g_tubeWent.name, swap.came[k], sizeof(g_tubeWent.name)) != 0)
            continue;
        g_tubeCameFiring = swap.cameFiring[k];
        g_tubeCameRaw = g_tubeWent.raw;
        g_tubeWent = TubeWent{};
    }
    // Every firing state in it, those that went and those that came: their objects are new, or freed.
    int forgotten = 0;
    for (int k = 0; k < changed; ++k) {
        if (swap.wentFiring[k])
            forgotten += static_cast<int>(g_guns.erase(swap.wentFiring[k]));
        if (swap.cameFiring[k])
            forgotten += static_cast<int>(g_guns.erase(swap.cameFiring[k]));
    }
    // The train pistol the mod counts: gone into the pickup, or its firing state another gun's now (its record went with the others above).
    for (int k = 0; k < changed && g_pistol.firing; ++k)
        if (swap.wentFiring[k] == static_cast<uint32_t>(g_pistol.firing) || swap.cameFiring[k] == static_cast<uint32_t>(g_pistol.firing))
            ForgetPistol();
    // The shotgun the mod counts, alike (the gun, its times and its latch: a swap asks for nothing).
    for (int k = 0; k < changed && g_tube.firing; ++k)
        if (swap.wentFiring[k] == static_cast<uint32_t>(g_tube.firing) || swap.cameFiring[k] == static_cast<uint32_t>(g_tube.firing))
            ForgetTube();
    // The gun in hand, and the grenade launcher's count, only when the swap is theirs: the new gun's path then runs once for what is there now, and the
    // grenade launcher's count is taken from the game's when it's next seen (the count the hand-over wrote).
    const bool rifle = pickrules::Forgets(swap.slot, swap.wentFiring, changed, handSlot, static_cast<uint32_t>(g_firing));
    const bool m320 = pickrules::Forgets(swap.slot, swap.wentFiring, changed, kM320Slot, static_cast<uint32_t>(g_m320Firing));
    if (rifle)
        g_firing = 0, g_weapon = 0, g_blueprint = 0;
    if (m320) {
        g_m320Firing = 0;
        g_m320Rounds = -1;
        g_m320GameReload = false;
    }
    // A gun that comes out of a pickup it went into: its magazine as it went in, given to it at its new-gun path (never one kept by this same swap): that
    // gun's, or the next new one in the slot it came into.
    bool given = false;
    for (int k = 0; k < changed && !given; ++k) {
        if ((swap.slot[k] != 0 && swap.slot[k] != 1) || !swap.came[k][0])
            continue;
        for (KeptMagazine& kept : g_keptMagazines) {
            if (!kept.swap || kept.swap == (swap.serial ? swap.serial : ~0u) || kept.entity != swap.entity ||
                std::strncmp(kept.name, swap.came[k], sizeof(kept.name)) != 0)
                continue;
            g_pendingMagOut = kept.magOut;
            g_havePendingMagOut = true;
            g_pendingFiring = swap.cameFiring[k];
            g_pendingSlot = swap.slot[k];
            kept = KeptMagazine{};
            given = true;
            break;
        }
    }
    Log("Ammo: a swap of the game's (%u, %s): %d slot%s changed, %d gun%s forgotten; the gun in hand %s, the launcher under the rifle %s%s", swap.serial,
        swap.byHand ? "a hand's take" : "its own pickup", changed, changed == 1 ? "" : "s", forgotten, forgotten == 1 ? "" : "s",
        rifle ? "new from now" : "as it was", m320 ? "counted afresh" : "as it was",
        given ? (g_pendingMagOut ? "; its magazine out again, as it went in" : "; its magazine in, as it went in") : "");
}

void Refill() {
    // The launcher's own, with it in hand.
    if (skin::LauncherInHand()) {
        if (const uintptr_t launcher = LauncherFiringState()) {
            int loaded = 0, spare = 0;
            if (Read(launcher, loaded, spare)) {
                Write(launcher, loaded, 30);
                Log("Ammo: spare rockets %d -> 30 (weapon.txt \"refill\"), %d loaded", spare, loaded);
                return;
            }
        }
    }
    uintptr_t firing = Firing();
    int loaded = 0, spare = 0;
    if (!firing || !Read(firing, loaded, spare)) {
        Log("Ammo: the gun's rounds weren't found; no refill");
        return;
    }
    Write(firing, loaded, 300);
    Log("Ammo: spare rounds %d -> 300 (weapon.txt \"refill\"), %d loaded", spare, loaded);
}

void Empty() {
    uintptr_t firing = Firing();
    int loaded = 0, spare = 0;
    if (!firing || !Read(firing, loaded, spare)) {
        Log("Ammo: the gun's rounds weren't found; not emptied");
        return;
    }
    // The game's reload is wanted here, so the gun gets no hidden round until it has reloaded.
    Gun& gun = g_guns[firing];
    gun.hidden = 0;
    gun.emptied = true;
    Write(firing, 0, spare + loaded);
    Log("Ammo: %d rounds in the gun -> 0, to the spare ones (weapon.txt \"empty\")", loaded);
}

}

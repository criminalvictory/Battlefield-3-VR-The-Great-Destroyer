#include "ammo.h"

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <unordered_map>

#include "camera.h"
#include "config.h"
#include "god.h"
#include "hands.h"
#include "log.h"
#include "reload.h"
#include "skin.h"

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
};

uintptr_t g_soldier = 0;  // the server's soldier the guns below belong to
uintptr_t g_firing = 0;   // the gun in hand
ULONGLONG g_firingSince = 0;  // when the gun in hand last changed (the old rifle is still drawn a while)
std::unordered_map<uintptr_t, Gun> g_guns;

uintptr_t ReadPointer(uintptr_t address) {
    uint32_t value = 0;
    return camera::SafeRead(address, &value, sizeof(value)) ? value : 0;
}

bool WriteInt(uintptr_t address, int value) {
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

// The gun whose magazine is worked by hand: the primary's, with the launcher in hand too. Not the train's pistol, which has its own
// rehearsal and whose rounds don't change (it's in the list's first slot there: RoundsInHand reads it for the shots' flash).
uintptr_t Firing() {
    return skin::TrainPistolInHand() ? 0 : FiringInSlot(kRifleSlot);
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
    Gun& gun = g_guns[firing];
    gun.written = true;
    gun.last = loaded + gun.hidden;
    WriteInt(firing + kSpare, spare);
    WriteInt(firing + kLoaded, gun.last);
}

// The magazine's size for the gun in hand.
int Capacity() {
    if (skin::RifleInHand())
        return kRifleMagazine;
    auto it = g_guns.find(g_firing);
    if (it == g_guns.end())
        return 1;
    return it->second.learned ? it->second.learned : std::max(1, it->second.fullest - 1);
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

bool RoundsInHand(int& loaded) {
    int spare = 0;
    if (Get(loaded, spare))
        return true;
    // The train's pistol (docs/research/pistol-manual-reload.md).
    const uintptr_t firing = FiringInSlot(kRifleSlot);
    return firing && ReadRaw(firing, loaded, spare);
}

void Update() {
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
        skin::InsertMagazine();
        reload::ReleaseBolt();
        hands::HeldChambered(true);
    }
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
        }
    }
    uintptr_t firing = Firing();
    int raw = 0, spare = 0;
    if (!firing || !ReadRaw(firing, raw, spare))
        return;
    Gun& gun = g_guns[firing];
    if (firing != g_firing) {
        Log("Ammo: the gun in hand has %d rounds loaded, %d spare", raw - gun.hidden, spare);
        g_firingSince = GetTickCount64();
        // A magazine out is that gun's: the next has its own in (the M416's out, a dead enemy's gun taken up), and it's still out when this one comes back.
        if (const auto was = g_guns.find(g_firing); was != g_guns.end())
            was->second.magOut = skin::MagazineOut();
        skin::SetMagazineOut(gun.magOut);
    }
    g_firing = firing;
    // The game's own reload (R held on X): its count up by a magazine without the mod writing it.
    // A magazine and the chambered round: its size, and the hidden round goes back on top.
    if (gun.last >= 1 && raw >= gun.last + 5) {
        gun.learned = raw - 1;
        if (gun.hidden) {
            WriteInt(firing + kLoaded, raw + gun.hidden);
            raw += gun.hidden;
        }
        Log("Ammo: the game reloaded by itself: %d rounds loaded (a magazine holds %d)", raw - gun.hidden,
            gun.learned);
        skin::InsertMagazine();  // it put one in, whatever the hand did meanwhile
    }
    if (!gun.written)
        gun.fullest = std::max(gun.fullest, raw);
    // The rifle reloaded by hand gets its hidden round, as does any other gun with its magazine by hand.
    if (!gun.hidden && (skin::RifleInHand() || skin::HeldByHand()) && raw >= 1) {
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
    // One round stays (the chambered one, or the one an empty gun keeps), from the spare if the
    // game fired it.
    int kept = std::min(1, loaded + spare);
    Write(firing, kept, spare + loaded - kept);
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
}

int MagazineSize() {
    return Capacity();
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
    Write(firing, 0, spare + loaded);
    Log("Ammo: %d rounds in the gun -> 0, to the spare ones (weapon.txt \"empty\")", loaded);
}

}

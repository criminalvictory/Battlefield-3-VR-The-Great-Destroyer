#include "mountseat.h"

#include <windows.h>

#include <atomic>
#include <cstdio>
#include <cstring>

#include "camera.h"
#include "god.h"
#include "log.h"
#include "seatread.h"

namespace {

// Step 1's two parts that need the game, not built yet: the seat's own view (the body fixed to the vehicle, the eye at the gunner's place, the game's
// camera as it made it kept for the steering: docs/research/mounted_gun.md 4.2, 4.6) and the steering's outputs (the pad's right stick, the mouse: each
// frame on the gun mountgun::SeatSteer with the rows from mountgun::AxisRowsOf, the player's invert key read from the profile; a guard's stop there puts
// the seat back to today's behaviour at the next SeatStep, said below). Until they are, no seat is ever a gun's, whatever its rows say
// (mountgun::SeatRules::built): with the hands steering, the view following the game's camera would turn the room with the gun and the gun with the room.
constexpr bool kBuilt = false;
// The steering's way (docs/research/mounted_gun.md 4.6: the mouse by default, the review's fourth finding; R5 decides).
constexpr uint32_t kWay = mountgun::kNeedMouse;

std::atomic<int> g_state{static_cast<int>(mountgun::State::kOnFoot)};
mountgun::Seat g_seat;
seatread::TypeCache g_types;
enum class Proved { kNotYet, kYes, kNo };
Proved g_proved = Proved::kNotYet;
uint32_t g_base = 0;
bool g_saidOff = false;
bool g_missSaid[8] = {};
char g_name[64] = {};
int g_seenSaid = -2;
mountgun::Fire g_fire;
bool g_holding[2] = {};

// Every read of the game's memory for the seat: guarded (camera::SafeRead), never the first 64 KB.
struct Reader {
    bool operator()(uint32_t address, void* out, size_t size) const {
        return address >= 0x10000 && address <= 0xFFFFFFFFu - static_cast<uint32_t>(size) && camera::SafeRead(address, out, size);
    }
};

// Once a run, in the first level: why the gunner's seats are as they were.
void SayOff() {
    const char* from = nullptr;
    if (const char* missing = seatread::FirstUnmeasured(seatread::kLayout, &from))
        Log("Mounted gun: the gunner's seats work as before: the seat isn't read (%s not measured yet: %s)", missing, from);
    for (int i = 0; i < mountgun::kGunCount; ++i) {
        int row = -1;
        mountgun::RowFault fault = mountgun::RowFault::kNone;
        if (!mountgun::Ready(mountgun::kGuns[i], mountgun::kNeedMatch | mountgun::kNeedView | mountgun::kNeedSteer | kWay, &row, &fault) && row >= 0)
            Log("Mounted gun: %s is not steered by the hands: %s %s (%s; docs\\research\\mounted_gun.md 6)", mountgun::kGuns[i].name,
                mountgun::kRowKinds[row].name, mountgun::RowFaultName(fault), mountgun::kRowKinds[row].from);
    }
    if (!kBuilt)
        Log("Mounted gun: the seat's own view and the steering's outputs aren't built yet (step 1): no seat is a gun's this run");
}

}  // namespace

namespace mountseat {

void Frame() {
    // The reader off for the run (unmeasured, or not borne out) and said so: nothing more of the game is read here, the seat stays on foot (the second
    // review's nit: the player chain had been read every frame after it).
    if (g_proved == Proved::kNo && g_saidOff)
        return;
    const uintptr_t soldier = god::PlayerClientSoldier();
    if (!g_saidOff && soldier) {
        g_saidOff = true;
        SayOff();
    }
    if (g_proved == Proved::kNotYet && soldier) {
        g_base = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)));
        const seatread::Proofs p = seatread::Prove(Reader(), g_base, seatread::kLayout);
        g_proved = p.ok ? Proved::kYes : Proved::kNo;
        if (p.ok)
            Log("Mounted gun: the seat is read from the game (the local player's attached controllable +0x%X, its entry +0x%X, a %s's data +0x%X; %d pieces "
                "of code borne out)",
                seatread::kLayout.attached.value, seatread::kLayout.entry.value, seatread::kLayout.vehicleType.value, seatread::kLayout.data.value,
                seatread::kLayout.codeCount);
        else if (!p.unmeasured)
            Log("Mounted gun: %s isn't as bf3.exe 1.6.0.0 has it (looked for at %08X): the seat isn't read this run, the gunner's seats work as before",
                p.failed, p.at);
    }
    mountgun::SeatRead read;
    if (g_proved == Proved::kYes) {
        const seatread::Seat s = seatread::Read(Reader(), g_base, seatread::kLayout, g_types);
        read.known = s.ok;
        read.vehicle = s.vehicle;
        read.entry = s.entry;
        if (s.vehicle) {
            std::memcpy(g_name, s.name, sizeof(g_name));
            read.name = g_name;
        }
        const int m = static_cast<int>(s.miss);
        if (!s.ok && s.miss != seatread::Miss::kNoPlayer && m >= 0 && m < 8 && !g_missSaid[m]) {
            g_missSaid[m] = true;
            Log("Mounted gun: the seat not read: %s (said once a run): taken as no gun's seat", seatread::MissName(s.miss));
        }
    }
    // (The game's camera as it made it: the seat's view keeps it, not built yet; the climb-in's end waits on it, and no seat gets that far today.)
    const mountgun::CameraNow camera{};
    mountgun::SeatRules rules;
    rules.way = kWay;
    rules.built = kBuilt;
    const double now = static_cast<double>(GetTickCount64()) / 1000.0;
    mountgun::SeatStep(g_seat, read, camera, now, mountgun::kGuns, mountgun::kGunCount, rules);
    if (g_seat.changed || (read.vehicle && g_seat.seen != g_seenSaid)) {
        if (g_seat.state == mountgun::State::kOtherSeat && g_seat.stopped != mountgun::SteerStop::kNone)
            Log("Mounted gun: the steering stopped on the %s: %s: %s's seat works as before till it's left", g_seat.stoppedAxis == 1 ? "pitch" : "yaw",
                mountgun::SteerStopName(g_seat.stopped), g_seat.seen >= 0 ? mountgun::kGuns[g_seat.seen].name : "the gun");
        else if (g_seat.state == mountgun::State::kOtherSeat && g_seat.seen >= 0 && g_seat.missing >= 0)
            Log("Mounted gun: %s's seat (entry %d of %s), kept as before: %s %s", mountgun::kGuns[g_seat.seen].name, read.entry, read.name ? read.name : "?",
                mountgun::kRowKinds[g_seat.missing].name, mountgun::RowFaultName(g_seat.fault));
        else if (g_seat.state == mountgun::State::kOtherSeat && g_seat.seen >= 0)
            Log("Mounted gun: %s's seat (entry %d of %s), kept as before: the seat's view and outputs not built", mountgun::kGuns[g_seat.seen].name, read.entry,
                read.name ? read.name : "?");
        else if (g_seat.state == mountgun::State::kOtherSeat)
            Log("Mounted gun: a vehicle's seat (entry %d of %s): no gun's, as before", read.entry, read.name ? read.name : "?");
        else
            Log("Mounted gun: %s%s%s", mountgun::StateName(g_seat.state), g_seat.gun >= 0 ? ": " : "", g_seat.gun >= 0 ? mountgun::kGuns[g_seat.gun].name : "");
        g_seenSaid = read.vehicle ? g_seat.seen : -2;
    }
    g_state.store(static_cast<int>(g_seat.state), std::memory_order_relaxed);
}

mountgun::State State() {
    return static_cast<mountgun::State>(g_state.load(std::memory_order_relaxed));
}

bool Seated() {
    return mountgun::InSeat(State());
}

bool Quiet(mountgun::What w) {
    return mountgun::Quiet(State(), w);
}

void NoteStickTurn(float radians) {
    // The body's orbit about the ring's axis belongs to the seat's view (mountgun::OrbitTurn), not built yet: the turn goes nowhere meanwhile (never to the
    // mouse, which turns the ring here).
    (void)radians;
}

bool FireDown(const float trigger[2], const float squeeze[2]) {
    for (int i = 0; i < 2; ++i) {
        const float s = squeeze[i];
        g_holding[i] = s > (g_holding[i] ? 0.4f : 0.6f);
    }
    return mountgun::FireStep(g_fire, trigger, g_holding);
}

}  // namespace mountseat

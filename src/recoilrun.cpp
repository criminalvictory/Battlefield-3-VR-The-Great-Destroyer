// build\recoil.txt and the game's side of recoil for every gun (design 6). The gun in hand's own per-shot kick is read from its own data (its
// firing state's WeaponFiringData, then its GunSwayData: in each of the six stance and zoom rows, the degrees up a round and the sideways pick)
// and taken out of there: each row kept by the data's address and put back exactly as it was on "off", "game 1", a gun the mod no longer draws,
// and unload. The game's weapon camera is then the look the mod writes, so the drawn gun's own kick (recoil.h) moves its sights, the scope's
// picture and the bullets together. Its rounds: the slot's own count going down (never a count the mod wrote), the grenade launcher's serial,
// the SMAW's and the Stinger's as hands.cpp sees them, and test rounds. Then the buzz, the scope picture's slack, the one-hand scatter, the
// check that the game's own kick really is out of its camera, and the logs. Render thread (Step, Read, Watch), and the unload.

#include "recoilrun.h"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iterator>

#include "aim.h"
#include "ammo.h"
#include "camera.h"
#include "config.h"
#include "hands.h"
#include "input.h"
#include "log.h"
#include "pickrules.h"
#include "recoilguns.h"
#include "scopeview.h"
#include "skin.h"

namespace recoilrun {
namespace {

using recoil::kDegree;

// ---- The game's own layout (its type descriptors, the 1.6 data; matches what ammo.cpp and skin.cpp's ClearRifleSpread already rely on) ----
constexpr uintptr_t kFiringData = 0x94;                                       // the firing state -> its WeaponFiringData (ammo.cpp's)
// WeaponFiringData: FiringFunctionData*, GunSwayData*, the rumble's Low/High/Duration. The sway is at +0x20, read in the game (2026-11-11: +0x24, the
// type descriptors' reading, holds SupportDelayStand, 0.3: every gun's data "not readable" and its kick left in); +0x24 and +0x28 are the support delays.
constexpr uintptr_t kPrimaryFire = 0x08, kWeaponSway = 0x20, kRumble = 0x2C;
constexpr uintptr_t kSpeedZ = 0x88, kPellets = 0xB0, kRate = 0x120, kRateBurst = 0x124, kMagazine = 0x15C;  // FiringFunctionData
constexpr int kBlocks = 6;
constexpr uintptr_t kRecoilRows[kBlocks] = {0x068, 0x0F4, 0x140, 0x18C, 0x1D8, 0x224};  // GunSwayData: Stand, Crouch, Prone x NoZoom, Zoom
// Each GunSwayRecoilData: RecoilAmplitudeMax, IncPerShot, HorizontalMin, HorizontalMax, HorizontalRecoilAmplitudeMax, DecreaseFactor.
enum Field { kCap, kUp, kSideMin, kSideMax, kSideCap, kDecrease, kFields };
constexpr uintptr_t kDeviationScales = 0x30C;                                  // four floats: 1 for every gun (a read of 1 confirms the layout)
constexpr uintptr_t kShootingScale = 0x31C, kFirstShot = 0x320, kCameraRecoil = 0x324;  // CameraRecoilData*: its spring at +0x08, damping +0x0C
constexpr uintptr_t kSpring = 0x08;
constexpr size_t kSwaySize = 0x328;

constexpr int kChangeFrames = 3;     // a change of gun, or a gun not held, counts once it has lasted this many frames
constexpr double kTriggerLately = 0.3;  // seconds: a drop in the count is shots only with the fire button down this lately
constexpr double kMaintain = 2.0;    // seconds between the looks at each gun's data (a checkpoint can put the kick back)
constexpr double kUnwatched = 1.0;   // seconds without a Step before every kick taken out goes back (free hands off, the headset not rendering)
// ...and as many pictures (Watch runs at each): one picture held up for seconds (a hitch, a film's burst written to disk) isn't a second
// of the gun not drawn (the holsters' F12 film, 2026-11-15: put back on the first picture after each burst).
constexpr int kUnwatchedPictures = 30;
constexpr double kM320Holds = 0.6;   // seconds after the grenade launcher's round its own numbers step the kick
// The check that the game's kick is out (CheckRun): a string counts with kCheckLeast of its rounds leveled, its first kCheckFirst at most (fewer on
// a gun whose own kick would reach its cap sooner), and goes on while the next round comes within kCheckGaps of the gun's own gaps (its count is read
// a frame or a tick late). A verdict once the strings' rounds are spread kCheckLever about their strings' middles (one string of six rounds): climbing
// when the camera rises between kCheckShare and kCheckTwice of the gun's own degrees up a round (a gun the game kicks less than kCheckUpLeast a round
// has nothing to look for). A string whose level moves more than kCheckJump times the gun's own degrees up a round between two neighbouring rounds
// is a shake, not a kick: it is never judged. kCheckRun verdicts running one way switch the drift off, or on again.
constexpr int kCheckLeast = 5, kCheckFirst = 10, kCheckRun = 3;
constexpr float kCheckGaps = 1.75f, kCheckLever = 17.5f, kCheckShare = 0.5f, kCheckTwice = 2.0f, kCheckUpLeast = 0.05f, kCheckJump = 3.5f;

double Clock() {
    static LARGE_INTEGER s_frequency{};
    if (!s_frequency.QuadPart)
        QueryPerformanceFrequency(&s_frequency);
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return static_cast<double>(now.QuadPart) / static_cast<double>(s_frequency.QuadPart);
}

uintptr_t Pointer(uintptr_t at) {
    uint32_t value = 0;
    return at && camera::SafeRead(at, &value, sizeof(value)) ? value : 0;
}

bool WriteFloat(uintptr_t address, float value) {
    __try {
        *reinterpret_cast<volatile float*>(address) = value;
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

float Degrees(float radians) {
    return std::remainder(radians, 2.0f * 3.14159265f) / kDegree;
}

// ---- build\recoil.txt ----
struct GunScale {
    char name[32];
    float scale;
};
struct Values {
    bool on = true;
    bool game = false;                     // "game 1": the game's own kick left in its data, the mod's drift off
    bool drift = true, kick = true, buzz = true;
    float driftScale = 1.0f, kickScale = 1.0f, buzzScale = 1.0f, buzzHz = 0.0f;
    float oneRise = 5.0f, oneBack = 2.5f, pistolOne = 1.5f;
    bool eye = true, mirror = true;
    float sideSign = 1.0f, lift = 0.04f;
    bool oneHandSpread = true, release = false;
    float driftDamping = 0.0f;
    GunScale guns[16] = {};
    int gunCount = 0;
    float logSeconds = 0.0f;               // "log N"
    int test = 0, testRounds = 0;          // "test": 1 single, 2 burst, 3 auto
    float testMs = 1000.0f;
};

Values g_values;
bool g_read = false;

const char* Skip(const char* s) {
    while (*s == ' ' || *s == '\t')
        ++s;
    return s;
}

// The rest of the line after its key, or null if the line isn't that key (a key is a whole word: "drift" isn't "driftdamping").
const char* After(const char* line, const char* key) {
    const size_t n = std::strlen(key);
    if (std::strncmp(line, key, n) != 0)
        return nullptr;
    const char c = line[n];
    return c == '\0' || c == ' ' || c == '\t' || c == '\r' || c == '\n' ? Skip(line + n) : nullptr;
}

float Clamp(float v, float lo, float hi, float otherwise) {
    return std::isfinite(v) ? std::fmin(std::fmax(v, lo), hi) : otherwise;
}

// "word 0|1 [strength [hz]]": off or on, its strength and (the buzz) its hertz after; any other first number is the strength itself, on, its hertz
// after it ("kick 1.3", "kick 0.7"; a negative one is off). Every number read as a number: "kick 1.3" was once on at 0.3, "kick 0.7" off.
void Switch(const char* rest, bool& on, float* strength, float* hz) {
    float n[3] = {};
    const int got = sscanf_s(rest, "%f %f %f", &n[0], &n[1], &n[2]);
    if (got < 1 || !std::isfinite(n[0]))
        return;
    on = n[0] > 0.0f;
    const int at = n[0] == 0.0f || n[0] == 1.0f ? 1 : 0;  // where its strength is
    if (at == 0 && !on)
        return;
    if (got > at && strength)
        *strength = Clamp(n[at], 0.0f, 5.0f, 1.0f);
    if (got > at + 1 && hz)
        *hz = Clamp(n[at + 1], 0.0f, 500.0f, 0.0f);
}

void Parse(FILE* f, Values& v) {
    char text[512];
    while (fgets(text, sizeof(text), f)) {
        const char* line = text;
        if (static_cast<unsigned char>(line[0]) == 0xEF && static_cast<unsigned char>(line[1]) == 0xBB && static_cast<unsigned char>(line[2]) == 0xBF)
            line += 3;  // (PowerShell's UTF-8 mark)
        line = Skip(line);
        const char* rest = nullptr;
        int n = 0;
        float a = 0.0f, b = 0.0f;
        if ((rest = After(line, "off")))
            v.on = false;
        else if ((rest = After(line, "game")) && sscanf_s(rest, "%d", &n) == 1)
            v.game = n != 0;
        else if ((rest = After(line, "drift")))
            Switch(rest, v.drift, &v.driftScale, nullptr);
        else if ((rest = After(line, "kick")))
            Switch(rest, v.kick, &v.kickScale, nullptr);
        else if ((rest = After(line, "buzz")))
            Switch(rest, v.buzz, &v.buzzScale, &v.buzzHz);
        else if ((rest = After(line, "onehand"))) {
            const int got = sscanf_s(rest, "%f %f", &a, &b);
            if (got >= 1)
                v.oneRise = Clamp(a, 1.0f, 20.0f, 5.0f);
            if (got >= 2)
                v.oneBack = Clamp(b, 1.0f, 10.0f, 2.5f);
        } else if ((rest = After(line, "onehandspread")) && sscanf_s(rest, "%d", &n) == 1)
            v.oneHandSpread = n != 0;
        else if ((rest = After(line, "pistol")) && sscanf_s(rest, "%f", &a) == 1)
            v.pistolOne = Clamp(a, 0.5f, 5.0f, 1.5f);
        else if ((rest = After(line, "eye")) && sscanf_s(rest, "%d", &n) == 1)
            v.eye = n != 0;
        else if ((rest = After(line, "mirror")) && sscanf_s(rest, "%d", &n) == 1)
            v.mirror = n != 0;
        else if ((rest = After(line, "sidesign")) && sscanf_s(rest, "%f", &a) == 1)
            v.sideSign = std::isfinite(a) && a < 0.0f ? -1.0f : 1.0f;
        else if ((rest = After(line, "lift")) && sscanf_s(rest, "%f", &a) == 1)
            v.lift = Clamp(a, 0.0f, 0.1f, 0.04f);
        else if ((rest = After(line, "release")) && sscanf_s(rest, "%d", &n) == 1)
            v.release = n != 0;
        else if ((rest = After(line, "driftdamping")) && sscanf_s(rest, "%f", &a) == 1)
            v.driftDamping = Clamp(a, 0.0f, 2.0f, 0.0f);
        else if ((rest = After(line, "log")) && sscanf_s(rest, "%f", &a) == 1)
            v.logSeconds = Clamp(a, 0.0f, 600.0f, 0.0f);
        else if ((rest = After(line, "gun"))) {
            // Its name as table 4.2 has it (spaces and all), then its scale.
            char name[64] = {};
            strncpy_s(name, rest, _TRUNCATE);
            size_t end = std::strlen(name);
            while (end && (name[end - 1] == ' ' || name[end - 1] == '\t' || name[end - 1] == '\r' || name[end - 1] == '\n'))
                name[--end] = '\0';
            char* last = std::strrchr(name, ' ');
            if (!last)
                last = std::strrchr(name, '\t');
            if (last && sscanf_s(last + 1, "%f", &a) == 1 && v.gunCount < static_cast<int>(std::size(v.guns))) {
                *last = '\0';
                end = std::strlen(name);
                while (end && (name[end - 1] == ' ' || name[end - 1] == '\t'))
                    name[--end] = '\0';
                if (end) {
                    GunScale& g = v.guns[v.gunCount++];
                    strncpy_s(g.name, name, _TRUNCATE);
                    g.scale = Clamp(a, 0.0f, 5.0f, 1.0f);
                }
            }
        } else if ((rest = After(line, "test"))) {
            char kind[16] = {};
            int rounds = 0;
            float ms = 1000.0f;
            const int got = sscanf_s(rest, "%15s %d %f", kind, static_cast<unsigned>(sizeof(kind)), &rounds, &ms);
            const int which = got < 2 ? 0 : std::strcmp(kind, "single") == 0 ? 1 : std::strcmp(kind, "burst") == 0 ? 2 : std::strcmp(kind, "auto") == 0 ? 3 : 0;
            if (which) {
                v.test = which;
                v.testRounds = std::min(std::max(rounds, 1), 100);
                v.testMs = got >= 3 ? Clamp(ms, 20.0f, 5000.0f, 1000.0f) : 1000.0f;
            }
        }
    }
}

// The switches in one line: "on (drift 1.0, kick 1.0, buzz 1.0; one hand 5 x 2.5; the eye 1, mirror 1, sideways +1)", and what isn't a default.
void Describe(const Values& v, char* out, size_t size) {
    const Values d;
    int n = sprintf_s(out, size, "on (drift %.1f, kick %.1f, buzz %.1f; one hand %g x %g; the eye %d, mirror %d, sideways %+d)",
                      v.drift ? v.driftScale : 0.0f, v.kick ? v.kickScale : 0.0f, v.buzz ? v.buzzScale : 0.0f, v.oneRise, v.oneBack, v.eye ? 1 : 0,
                      v.mirror ? 1 : 0, v.sideSign < 0.0f ? -1 : 1);
    auto more = [&](const char* format, auto... args) {
        if (n > 0 && static_cast<size_t>(n) < size)
            n += sprintf_s(out + n, size - n, format, args...);
    };
    if (v.pistolOne != d.pistolOne)
        more("; a pistol in one hand x%g", v.pistolOne);
    if (v.lift != d.lift)
        more("; the launchers' lift at most %.0f mm", v.lift * 1000.0f);
    if (v.buzzHz != d.buzzHz)
        more("; the buzz at %.0f Hz", v.buzzHz);
    if (!v.oneHandSpread)
        more("; one hand keeps the zoomed scatter (onehandspread 0)");
    if (v.release)
        more("; the flip after held fire (release 1)");
    if (v.driftDamping > 0.0f)
        more("; the drift's spring damped %.2f", v.driftDamping);
    for (int i = 0; i < v.gunCount; ++i) {
        const recoilguns::Row* row = recoilguns::ByName(v.guns[i].name);
        more("; the %s x%g", row ? row->name : v.guns[i].name, v.guns[i].scale);
    }
}

void LogChanges(const Values& was, const Values& v, bool all) {
    if (!v.on) {
        if (all || was.on)
            Log("Recoil: off (recoil.txt): the game's own kick in every gun's data, nothing kicks or buzzes");
        return;
    }
    char now[512], before[512];
    Describe(v, now, sizeof(now));
    Describe(was, before, sizeof(before));
    if (all || !was.on || std::strcmp(now, before) != 0)
        Log("Recoil: %s", now);
    if ((all && v.game) || (!all && v.game != was.game))
        Log(v.game ? "Recoil: the game's own kick left in its data (game 1): the mod's drift off, its kick and buzz as set"
                   : "Recoil: the game's own kick taken out of its data again (game 0): the mod's drift on");
    for (int i = 0; i < v.gunCount; ++i)
        if (!recoilguns::ByName(v.guns[i].name))
            Log("Recoil: no gun called \"%s\" in the table (recoil.txt's gun line): it scales nothing", v.guns[i].name);
}

// ---- Each gun's six rows, kept by its data's address: taken out (up and sideways 0) and put back exactly ----
struct Saved {
    uintptr_t sway = 0;
    float game[kBlocks][kFields] = {};  // its rows as the game had them
    bool out = false;                   // their up and sideways written 0 by the mod (every write taken)
    bool told = false;                  // its first take-out logged (once a gun a level)
    bool refused = false;               // a write refused taking it out: what was written put back, never tried again (its kick left in, no drift)
    char name[32] = {};
};
constexpr int kSavedMost = 32;
Saved g_saved[kSavedMost];

// A row as the game has it, bit for bit.
bool RowAsGame(const float* now, const float* game) {
    return std::memcmp(now, game, sizeof(float) * kFields) == 0;
}
// A row as the mod left it: its caps and recovery the game's, its up and sideways each 0 or the game's, not all three the game's.
bool RowTaken(const float* now, const float* game) {
    if (!(now[kCap] == game[kCap] && now[kSideCap] == game[kSideCap] && now[kDecrease] == game[kDecrease]) || RowAsGame(now, game))
        return false;
    for (const int k : {kUp, kSideMin, kSideMax})
        if (!(now[k] == 0.0f || now[k] == game[k]))
            return false;
    return true;
}

struct Reading {
    int game = 0, taken = 0;  // rows as the game has them, as the mod left them
    bool Whole() const { return game + taken == kBlocks; }
};
Reading RowsNow(const Saved& s) {
    Reading r;
    for (int b = 0; b < kBlocks; ++b) {
        float now[kFields];
        if (!camera::SafeRead(s.sway + kRecoilRows[b], now, sizeof(now)))
            return Reading{};
        if (RowAsGame(now, s.game[b]))
            ++r.game;
        else if (RowTaken(now, s.game[b]))
            ++r.taken;
    }
    return r;
}

int PutBack(Saved& s);

// Up and sideways written 0 in every row that still reads as the game had it; nothing at all unless every row is the game's or the mod's already.
// A write refused: every row written put back as the game had it, and never tried again, so the game's kick stays whole and the mod's drift off
// (never part of the one with the other). True with its kick out.
bool TakeOut(Saved& s) {
    if (s.refused || !RowsNow(s).Whole())
        return false;
    for (int b = 0; b < kBlocks; ++b) {
        const uintptr_t at = s.sway + kRecoilRows[b];
        float now[kFields];
        if (!camera::SafeRead(at, now, sizeof(now)) || !RowAsGame(now, s.game[b]))
            continue;
        if (!(WriteFloat(at + 4 * kUp, 0.0f) && WriteFloat(at + 4 * kSideMin, 0.0f) && WriteFloat(at + 4 * kSideMax, 0.0f))) {
            PutBack(s);
            s.refused = true;
            return false;
        }
    }
    s.out = true;
    return true;
}

// Every row that reads as the mod left it back as the game had it (a row that reads as anything else is left alone: a load put other data
// there). Each float tried on its own (one refused never keeps the others from going back). How many rows were written whole.
int PutBack(Saved& s) {
    int rows = 0;
    for (int b = 0; b < kBlocks; ++b) {
        const uintptr_t at = s.sway + kRecoilRows[b];
        float now[kFields];
        if (!camera::SafeRead(at, now, sizeof(now)) || !RowTaken(now, s.game[b]))
            continue;
        const bool high = WriteFloat(at + 4 * kSideMax, s.game[b][kSideMax]);
        const bool low = WriteFloat(at + 4 * kSideMin, s.game[b][kSideMin]);
        const bool up = WriteFloat(at + 4 * kUp, s.game[b][kUp]);
        if (high && low && up)
            ++rows;
    }
    s.out = false;
    return rows;
}

Saved* Find(uintptr_t sway) {
    if (sway)
        for (Saved& s : g_saved)
            if (s.sway == sway)
                return &s;
    return nullptr;
}

Saved* Keep(uintptr_t sway) {
    for (Saved& s : g_saved)
        if (!s.sway) {
            s = Saved{};
            s.sway = sway;
            return &s;
        }
    for (Saved& s : g_saved)  // full: one whose kick the mod hasn't taken out
        if (!s.out) {
            s = Saved{};
            s.sway = sway;
            return &s;
        }
    return nullptr;
}

bool AnyOut() {
    for (const Saved& s : g_saved)
        if (s.sway && s.out)
            return true;
    return false;
}

void PutBackAll(const char* why) {
    for (Saved& s : g_saved)
        if (s.sway && s.out) {
            const int rows = PutBack(s);
            Log("Recoil: the %s's data put back as the game had it (%d rows)%s%s", s.name, rows, why ? ": " : "", why ? why : "");
        }
}

// Back at unload, as at "off" (the DLL is never unloaded while the game runs today: this is for when it is).
struct BackAtUnload {
    ~BackAtUnload() {
        for (Saved& s : g_saved)
            if (s.sway && s.out)
                PutBack(s);
    }
} g_backAtUnload;

// ---- The gun's own data as it is now ----
struct Live {
    uintptr_t firing = 0, data = 0, fire = 0, sway = 0;
    bool haveSway = false, haveFire = false, haveRumble = false, haveCamera = false;
    float rows[kBlocks][kFields] = {};
    float deviation[4] = {}, firstShot = 0.0f, shootingScale = 0.0f;
    float spring = 0.0f, damping = 0.0f;  // its camera spring (none: 0)
    float rate = 0.0f, rateBurst = 0.0f, speed = 0.0f;
    int pellets = 0, magazine = 0;
    float rumble[3] = {};
};

void ReadLive(int slot, Live& l) {
    l = Live{};
    l.firing = ammo::FiringOf(slot);
    l.data = l.firing ? Pointer(l.firing + kFiringData) : 0;
    if (!l.data)
        return;
    l.fire = Pointer(l.data + kPrimaryFire);
    l.sway = Pointer(l.data + kWeaponSway);
    float rumble[3] = {};
    if (camera::SafeRead(l.data + kRumble, rumble, sizeof(rumble)) && rumble[0] >= 0.0f && rumble[0] <= 1.0f && rumble[1] >= 0.0f && rumble[1] <= 1.0f &&
        rumble[2] >= 0.0f && rumble[2] <= 2.0f) {
        std::memcpy(l.rumble, rumble, sizeof(rumble));
        l.haveRumble = true;
    }
    if (l.fire)
        l.haveFire = camera::SafeRead(l.fire + kRate, &l.rate, sizeof(l.rate)) && camera::SafeRead(l.fire + kRateBurst, &l.rateBurst, sizeof(l.rateBurst)) &&
                     camera::SafeRead(l.fire + kSpeedZ, &l.speed, sizeof(l.speed)) && camera::SafeRead(l.fire + kPellets, &l.pellets, sizeof(l.pellets)) &&
                     camera::SafeRead(l.fire + kMagazine, &l.magazine, sizeof(l.magazine));
    if (!l.sway)
        return;
    float all[kSwaySize / 4];
    if (!camera::SafeRead(l.sway, all, sizeof(all)))
        return;
    for (int b = 0; b < kBlocks; ++b)
        for (int k = 0; k < kFields; ++k)
            l.rows[b][k] = all[kRecoilRows[b] / 4 + k];
    for (int i = 0; i < 4; ++i)
        l.deviation[i] = all[kDeviationScales / 4 + i];
    l.shootingScale = all[kShootingScale / 4];
    l.firstShot = all[kFirstShot / 4];
    uint32_t cameraData = 0;
    std::memcpy(&cameraData, &all[kCameraRecoil / 4], sizeof(cameraData));
    float spring[2] = {};
    if (cameraData && camera::SafeRead(cameraData + kSpring, spring, sizeof(spring))) {
        l.spring = spring[0], l.damping = spring[1];
        l.haveCamera = true;
    }
    l.haveSway = true;
}

// Which check the data doesn't pass, or null: it reads as a gun's sway (5.2). Its up and sideways may be the mod's 0.
const char* Check(const Live& l) {
    auto in = [](float v, float lo, float hi) { return v >= lo && v <= hi; };  // (never for a NaN)
    if (!l.haveSway)
        return "not readable";
    for (const auto& row : l.rows) {
        if (!in(row[kCap], 1.0f, 60.0f) || !in(row[kSideCap], 1.0f, 60.0f))
            return "a cap out of range";
        if (!in(row[kUp], 0.0f, 10.0f))
            return "the degrees up a round out of range";
        if (!in(row[kSideMin], -5.0f, 5.0f) || !in(row[kSideMax], -5.0f, 5.0f) || row[kSideMin] > row[kSideMax])
            return "the sideways pick out of range";
        if (!in(row[kDecrease], 0.5f, 100.0f))
            return "the recovery out of range";
    }
    if (!in(l.firstShot, 0.1f, 5.0f) || !in(l.shootingScale, 0.0f, 2.0f))
        return "the first-shot or firing scale out of range";
    for (const float scale : l.deviation)
        if (scale != 1.0f)
            return "a deviation scale not 1";
    if (!l.haveFire || !in(l.rate, 10.0f, 2000.0f))
        return "its rate of fire out of range";
    if (l.haveCamera && (!in(l.spring, 0.0f, 5000.0f) || !in(l.damping, 0.0f, 5.0f)))
        return "its camera spring out of range";
    return nullptr;
}

// ---- The gun in hand ----
struct Key {
    int pose = 0;
    uint32_t id = 0;
    int slot = -1;
    bool operator==(const Key& o) const { return pose == o.pose && id == o.id && slot == o.slot; }
};
struct Held {
    Key key;
    uintptr_t firing = 0, data = 0;       // its firing state and data as set up
    const recoilguns::Row* row = nullptr; // its row in the table (static)
    const char* name = "gun";             // the table's name (static)
    recoil::Gun gun;                      // its numbers (the drift's row for the stance put in each frame)
    float rows[kBlocks][kFields] = {};    // the game's own six rows (as saved, or the table's)
    uintptr_t sway = 0;                   // its data's sway, kept in g_saved: its kick out of it while the mod draws it
    bool noSway = false;                  // its data has no gun sway (a soldier's): the game kicks it none, the drift is the mod's alone
    bool driftOff = false;                // the drift off for it: its kick left in the game (its data unread, another gun's, a write refused)
    bool climbs = false;                  // the drift off for it by the check: the game's own kick seen climbing in its camera (on again when it no longer does)
    bool otherInSlot = false;             // its slot holds another gun by its name (a change of gun under way): its data and its count not this gun's
    const recoilguns::Row* slotRow = nullptr;  // its slot's gun by its name as set up (null: not known; a name known later or another sets it up again)
    float rateBurst = 0.0f;
};
Held g_gun, g_m320;

const char* M320Name() {
    // The grenade launcher under the M416 is the 40 mm's own data: its row by name, the standalone one's numbers if the table has only that.
    for (const char* name : {"M320 (under the M416)", "M320 (M416)", "M320"})
        if (recoilguns::ByName(name))
            return name;
    return nullptr;
}

recoil::Kind Guess(const Key& key, const Live& l, bool m320) {
    if (key.pose == 3)
        return recoil::Kind::kPistol;
    if (m320)
        return recoil::Kind::kM320;
    if (key.slot == 5)
        return recoil::Kind::kRocket;
    if (key.slot == 1 && (!l.haveFire || l.magazine <= 1))  // (the Stinger's slot; a gun taken up from the ground may be there too, with a magazine)
        return recoil::Kind::kMissile;
    if (l.haveFire && l.pellets > 1)
        return recoil::Kind::kShotgun;
    if (l.haveFire && l.rate > 0.0f && l.rate < 400.0f)
        return recoil::Kind::kMarksman;
    return recoil::Kind::kRifle;
}

bool SameSignature(const recoilguns::Row& a, const recoilguns::Row& b) {
    if (a.rate != b.rate || a.speed != b.speed || a.magazine != b.magazine || a.pellets != b.pellets)
        return false;
    for (int i = 0; i < 8; ++i)
        if (a.sig[i] != b.sig[i])
            return false;
    return true;
}

// A sway's own numbers (Row::sig: the standing hip row, the first shot, the firing scale, the camera spring) the same, each within the table's 1e-4.
bool SameSway(const float* a, const float* b) {
    for (int i = 0; i < 8; ++i)
        if (!(std::fabs(a[i] - b[i]) <= 1e-4f))
            return false;
    return true;
}

// Whether the gun's kick is wanted out of its data now (not "off", not "game 1").
bool KickWantedOut() {
    return g_values.on && !g_values.game;
}

// A gun's line said once for its data (a weapon switch back to it doesn't say it again): its name and why are static words.
bool Once(const char* name, const char* why, uintptr_t data) {
    struct Told {
        const char* name;
        const char* why;
        uintptr_t data;
    };
    static Told s_told[32] = {};
    static int s_next = 0;
    for (const Told& t : s_told)
        if (t.name == name && t.why == why && t.data == data)
            return false;
    s_told[s_next] = {name, why, data};
    s_next = (s_next + 1) % static_cast<int>(std::size(s_told));
    return true;
}

// The gun in that slot set up: its data read and checked, its kick taken out of it (kept to put back), its table row, its numbers.
void SetUp(Held& h, const Key& key, const char* shape, int slot, bool m320) {
    h = Held{};
    h.key = key;
    Live l;
    ReadLive(slot, l);
    h.firing = l.firing;
    h.data = l.data;
    h.rateBurst = l.haveFire && l.rateBurst >= 10.0f && l.rateBurst <= 3000.0f ? l.rateBurst : 0.0f;
    // Its row: by what's drawn (its body, its shape), by its data's numbers, by name for the grenade launcher under the M416.
    const recoilguns::Row* byBody = m320 ? nullptr : recoilguns::ByBody(key.id);
    const recoilguns::Row* byShape = m320 ? nullptr : recoilguns::ByShape(shape);
    const recoilguns::Row* byName = m320 ? recoilguns::ByName(M320Name()) : nullptr;
    const recoilguns::Row* bySig = nullptr;
    // The slot's own gun by its name (ammo.cpp's: its blueprint's row in the table) against the gun drawn: two guns (pickrules::OtherGun) is a change
    // of gun under way, and the slot's data is never touched under the drawn gun's name (Operation Swordbreaker's roof, at4_measured.md section 11:
    // the kit's AT4 in slot 0 0.1 s after the prompt while the M4A1 is drawn 0.3 s more, and its six kick rows taken out as the M4A1's for 0.91 s;
    // the kit's M4A1 back in it while the AT4's tube is still drawn). Its data is left as it is and set up again once what's drawn is the slot's
    // own gun; meanwhile the drift is off and the slot's count is no round of the drawn gun's. (Data its numbers know (bySig, below) never came in
    // the game: its +0x15C isn't the magazine. Rows with the same numbers are one gun's data, as below. Not the train pistol's: its key's slot is 0
    // whichever slot ammo.cpp counts it in, as before.)
    const recoilguns::Row* bySlot = m320 || key.pose == 3 ? nullptr : ammo::RowInSlot(slot);
    const recoilguns::Row* drawnRow = byBody ? byBody : byShape;
    h.slotRow = bySlot;
    h.otherInSlot = pickrules::OtherGun(drawnRow, bySlot) && !SameSignature(*drawnRow, *bySlot);
    const char* bad = nullptr;
    Saved* kept = nullptr;
    float sig[8] = {};  // its data's own numbers (Row::sig's), with kept
    if (!l.data)
        bad = "its data not found";
    else if (h.otherInSlot)
        bad = "its slot holds another gun by its name";
    else if (!l.sway)
        h.noSway = true;
    else
        bad = Check(l);
    if (l.data && l.sway && !bad) {
        kept = Find(l.sway);
        if (kept && !RowsNow(*kept).Whole())
            kept->sway = 0, kept = nullptr;  // (something else at that address now)
        if (!kept) {
            bool zeros = true;
            for (const auto& row : l.rows)
                zeros = zeros && row[kUp] == 0.0f && row[kSideMin] == 0.0f && row[kSideMax] == 0.0f;
            if (zeros) {
                // Already 0 by an earlier run of the mod (never put back): the table's numbers, kept to put back, only where its caps and recovery
                // agree with the data's in every row (another gun's numbers are never written into it).
                const recoilguns::Row* from = byName ? byName : byBody ? byBody : byShape;
                for (int b = 0; from && b < kBlocks; ++b)
                    if (l.rows[b][kCap] != from->capUp || l.rows[b][kSideCap] != from->capSide || l.rows[b][kDecrease] != from->decrease[b / 2][b % 2])
                        from = nullptr;
                if (from && (kept = Keep(l.sway))) {
                    for (int b = 0; b < kBlocks; ++b) {
                        std::memcpy(kept->game[b], l.rows[b], sizeof(kept->game[b]));
                        kept->game[b][kUp] = from->up[b / 2][b % 2], kept->game[b][kSideMin] = from->sideMin[b / 2][b % 2];
                        kept->game[b][kSideMax] = from->sideMax[b / 2][b % 2];
                    }
                    kept->out = true;
                    Log("Recoil: the %s's kick was already out of its data (an earlier run): the table's numbers kept to put back", from->name);
                } else {
                    bad = "its kick already out of it, its own numbers not known";
                }
            } else if ((kept = Keep(l.sway))) {
                for (int b = 0; b < kBlocks; ++b)
                    std::memcpy(kept->game[b], l.rows[b], sizeof(kept->game[b]));
            } else {
                bad = "no room to keep its rows";
            }
        }
        if (kept) {
            std::memcpy(h.rows, kept->game, sizeof(h.rows));
            h.sway = l.sway;
            const float own[8] = {h.rows[0][kUp], h.rows[0][kSideMin], h.rows[0][kSideMax], h.rows[0][kDecrease], l.firstShot, l.shootingScale,
                                  l.haveCamera ? l.spring : 0.0f, l.haveCamera ? l.damping : 0.0f};
            std::memcpy(sig, own, sizeof(sig));
            bySig = recoilguns::BySignature(l.rate, l.speed, l.magazine, l.pellets, sig);
        }
    }
    // The drawn gun and the data's numbers name other guns (the slot holding another gun's data), or the grenade launcher's slot holds data whose sway
    // isn't the 40 mm's own (the M320's row's numbers; the launchers share them): nothing written, its kick left in.
    const bool not40mm = m320 && kept && !(byName && SameSway(byName->sig, sig));
    if (kept && ((byBody && bySig && byBody != bySig && !SameSignature(*byBody, *bySig)) || not40mm)) {
        bad = "another gun's numbers in that slot";
        if (!kept->out)
            kept->sway = 0;  // (never taken out: forgotten)
        kept = nullptr;
        h.sway = 0;
    }
    h.row = byName ? byName : byBody ? byBody : bySig ? bySig : byShape;
    const char* by = byName ? "by name" : byBody ? "its body" : bySig ? "its data" : byShape ? "its shape" : "the class default";
    const recoil::Kind kind = h.row ? h.row->kind : Guess(key, l, m320);
    h.name = h.row ? h.row->name : "gun";  // (a gun not known: "the gun" in the logs, its class's numbers)
    if (!m320 && Once(h.name, "set up", l.data))
        Log("Recoil: the %s set up from slot %d (pose %d, body %u): %s; by its body %s, by its data %s, by its shape %s, the slot's by its name %s", h.name, slot,
            key.pose, key.id, by, byBody ? byBody->name : "none", bySig ? bySig->name : "none", byShape ? byShape->name : "none",
            bySlot ? bySlot->name : "none");
    if (!h.row)
        h.row = &recoilguns::Default(kind);
    h.gun.kind = key.pose == 3 ? recoil::Kind::kPistol : m320 ? recoil::Kind::kM320 : kind == recoil::Kind::kNone ? Guess(key, l, m320) : kind;
    h.gun.kick = h.row->kick;
    h.gun.rate = l.haveFire && l.rate >= 10.0f && l.rate <= 3000.0f ? l.rate : h.row->rate;
    h.gun.pairGap = h.row->pairGap;
    h.gun.burst = -1;  // (the mode in force isn't known: the first two rounds of a pull show it)
    if (l.haveRumble)
        std::memcpy(h.gun.rumble, l.rumble, sizeof(h.gun.rumble));
    else
        std::memcpy(h.gun.rumble, h.row->rumble, sizeof(h.gun.rumble));
    recoil::Drift& d = h.gun.drift;
    if (h.sway) {
        d.capUp = h.rows[0][kCap], d.capSide = h.rows[0][kSideCap];
        d.firstShot = l.firstShot, d.shootingScale = l.shootingScale;
        d.spring = l.haveCamera ? l.spring : 0.0f, d.damping = l.haveCamera ? l.damping : 0.0f;
    } else {
        // The table's, for a gun whose data has no gun sway (or can't be read).
        for (int b = 0; b < kBlocks; ++b) {
            h.rows[b][kCap] = h.row->capUp, h.rows[b][kSideCap] = h.row->capSide;
            h.rows[b][kUp] = h.row->up[b / 2][b % 2], h.rows[b][kSideMin] = h.row->sideMin[b / 2][b % 2];
            h.rows[b][kSideMax] = h.row->sideMax[b / 2][b % 2], h.rows[b][kDecrease] = h.row->decrease[b / 2][b % 2];
        }
        d.capUp = h.row->capUp, d.capSide = h.row->capSide;
        d.firstShot = h.row->sig[4], d.shootingScale = h.row->sig[5], d.spring = h.row->sig[6], d.damping = h.row->sig[7];
    }
    // Its kick out of its data (unless "off" or "game 1"), and the line once a gun a level.
    if (h.otherInSlot) {
        h.driftOff = true;
        if (Once(h.name, bad, l.data))
            Log("Recoil: the %s drawn while slot %d holds the %s (a change of gun under way): nothing of that gun's data touched, set up again once "
                "it's drawn; the drift off meanwhile", h.name, slot, bySlot->name);
    } else if (bad) {
        h.driftOff = true;
        if (Once(h.name, bad, l.data))
            Log("Recoil: the %s's data didn't read as gun sway (%s): its kick left in the game, the drift off for it", h.name, bad);
    } else if (h.noSway) {
        if (Once(h.name, by, l.data))
            Log("Recoil: the %s has no gun sway of its own (a soldier's data): its numbers from the table (%s)", h.name, by);
    } else if (kept) {
        strncpy_s(kept->name, h.name, _TRUNCATE);
        if (KickWantedOut() && !TakeOut(*kept)) {
            // A write refused (what was written put back): the game's kick whole, so the mod's drift stays off for it.
            h.driftOff = true;
            if (Once(h.name, "a write refused", l.data))
                Log("Recoil: the %s's kick left in its data (a write refused, what was written put back as the game had it): the drift off for it", h.name);
        } else if (KickWantedOut() && !kept->told) {
            kept->told = true;
            const float* r = h.rows[0];
            Log("Recoil: the %s's own kick taken out of the game's data: %.2f up a shot (x%.2g the first), sideways %.2f..%.2f, back %.0f a second (x%.2f "
                "firing), its camera spring %.0f/%.2f; %d rows (data %08X, its standing block at %08X; its row %s)%s",
                h.name, r[kUp], d.firstShot, r[kSideMin], r[kSideMax], r[kDecrease], d.shootingScale, d.spring, d.damping, kBlocks,
                static_cast<unsigned>(l.sway), static_cast<unsigned>(l.sway + 8), by, m320 ? " (the grenade launcher under the rifle)" : "");
        }
    }
}

// The drift's row for this stance: [0] the hip, [1] at the eye.
void Stance(Held& h, int stance) {
    const int s = stance >= 0 && stance <= 2 ? stance : 0;
    recoil::Drift& d = h.gun.drift;
    for (int z = 0; z < 2; ++z) {
        const float* row = h.rows[s * 2 + z];
        d.up[z] = row[kUp], d.sideMin[z] = row[kSideMin], d.sideMax[z] = row[kSideMax], d.decrease[z] = row[kDecrease];
    }
}

// The sight's own ZoomedKickbackFactor from the gun's row (table 4.4): by which sight it is, else by its power.
float EyeFactor(const recoilguns::Row* row, const skin::SightNow& s) {
    if (!row)
        return 1.0f;
    int column = 0;  // irons, Kobra, PK-AS, PK-A, PSO-1, PKS-07, ACOG, another 6x+ scope
    switch (s.sight) {
    case 0: column = 4; break;
    case 1: column = 5; break;
    case 2: column = 3; break;
    case 3: column = 2; break;
    case 4: column = 1; break;
    case 5: column = 2; break;  // the EoTech: the holographic sight's column, the PK-AS's
    case 6: column = 7; break;  // the rifle scope (the Mk11's, 6.84x): the 6x+ scopes' column
    case 7: column = 6; break;  // the ACOG on a held gun (the bridge M249's): the ACOG's column, as the M416's
    case 10: column = 6; break;
    default: column = !(s.power > 1.5f) ? 0 : s.power <= 2.5f ? 2 : s.power <= 5.0f ? 6 : 7; break;
    }
    const float factor = row->eye[column];
    return std::isfinite(factor) && factor > 0.0f && factor <= 2.0f ? factor : 1.0f;
}

// Whether the drift is wanted for that gun and the game's own kick is out of its data (or none in it): on, but for the check of its camera.
bool DriftWanted(const Held& h) {
    if (!g_values.drift || g_values.game || h.driftOff)
        return false;
    if (!h.sway)
        return h.noSway;
    const Saved* s = Find(h.sway);
    return s && s->out;
}

// Whether the drift is on for that gun: wanted, the game's own kick out of its data (or none in it) and not seen climbing in its camera, never both.
bool DriftOn(const Held& h) {
    return DriftWanted(h) && !h.climbs;
}

// recoil.txt's "gun <name> <scale>" for that gun's row (its name, blueprint or shape, any case: the table's ByName).
float GunScaleOf(const recoilguns::Row* row) {
    for (int i = 0; row && i < g_values.gunCount; ++i)
        if (recoilguns::ByName(g_values.guns[i].name) == row)
            return g_values.guns[i].scale;
    return 1.0f;
}

// ---- The frame's state ----
recoil::State g_state;
recoil::Output g_out;
recoil::Hold g_lastHold;
bool g_lastActive = false;
bool g_first = true, g_clear = false;
double g_stepAt = 0.0;         // the last Step (Clock)
int g_watchesSinceStep = 0;     // the pictures (Watch) since the last Step
uint32_t g_frame = 0;
Key g_change;                  // a change of gun waiting out kChangeFrames
int g_changeFrames = 0;
int g_notHeld = 0;             // frames the gun hasn't been held by its grip
bool g_atEye = false;
int g_lastLoaded = -1;         // the slot's count last frame (-1 not known)
unsigned g_lastWrites = 0, g_lastM320 = 0;
double g_triggerAt = -1e9, g_m320At = -1e9, g_maintainAt = 0.0;
double g_lateAt = -1.0;        // a launcher's lift pulse due then (Clock), its amplitude, length and hands
float g_late = 0.0f, g_lateFor = 0.0f;
int g_lateSides[2] = {-1, -1};
double g_logUntil = 0.0;       // "log N"
bool g_testArmed = false;      // "test": once per write of the file
struct TestRun {
    int kind = 0, left = 0;
    double gap = 0.0, next = 0.0;
} g_test;

// Once a second while firing: the line.
struct Second {
    double start = -1.0;
    int rounds = 0;
    float up = 0.0f, drift = 0.0f, kick = 0.0f, side = 0.0f, back = 0.0f;
    float camUp[64] = {}, camSide[64] = {};
    int cams = 0;
} g_second;
// The check that the game's kick is out (design 6, 5.2). Left in its data, the game's own kick shows in its weapon camera as a climb: at every round
// of a string the camera stands the gun's own degrees up higher above the look it was built from, nothing coming back until the string ends (the M416's
// 0.26 a round: 10 rounds or more in a second stood 4.65 degrees above, 2026-11-11). Nothing else that parts the camera from the look grows with the
// rounds: the game's own move of it at each shot (the M416 pitched 52 degrees down, its kick out and the mod's off: 1.4 degrees to the side and 0.4 to
// 0.65 up, most of it gone in 0.3 s), its shakes with no round fired (degrees either way for a second, in a fight), the look read a frame or a tick
// late while the kick turns the gun, the aim held at its clamp, a steep look's wide yaw. Judged by how far the camera stood from the look (its median over 10 rounds, 0.5
// degrees at most), a gun's drift was switched off seven times in two days with its kick out of its data all the while (its rows are read every 2 s),
// the user's own M416 among them (2026-11-13). So it goes by the climb alone, in pitch: each round's level (the median height of the cameras built
// from its frame until the next round, or for one of the gun's gaps), a straight line through a string's levels against its rounds' numbers, the
// strings pooled each about its own middle (a height that lasts a string adds nothing). Single shots and bursts under kCheckLeast rounds are never
// judged (the kick, were it in, does not pile up in them), nor rounds fired with one hand (Step).
struct CheckRun {
    float heights[12] = {};  // the cameras built since the last round: each one's height above the look it was built from (degrees)
    int count = 0;
    float since = 1e9f;      // seconds since that round, by the frames' own lengths
    int number = 0;          // the string: its rounds so far (0: none going on),
    int leveled = 0;         // ... those with a level, their numbers and levels summed,
    float sx = 0.0f, sy = 0.0f, sxx = 0.0f, sxy = 0.0f;
    bool taken = false;      // ... and its first rounds already among the verdict's
    float last = 0.0f, lastAt = 0.0f;  // ... its last level and that round's number,
    bool shaken = false;     // ... and whether two of its neighbouring levels lie more than `most` a round apart (a shake, not a kick: never judged)
    float most = 0.0f;       // the gun's own degrees up a round, kCheckJump times over
    float lever = 0.0f, rise = 0.0f;  // the strings since the last verdict: their rounds' spread, and their levels' rise with it
    int rounds = 0, strings = 0;
    int climbing = 0, level = 0;      // verdicts running each way
    bool told = false;
} g_check;
double g_lastRoundAt = -1e9;   // the main gun's last round (Clock)

float Median(float* v, int n) {
    if (n <= 0)
        return 0.0f;
    std::nth_element(v, v + n / 2, v + n);
    return v[n / 2];
}

// The round the cameras belong to is over: its level (two cameras at least) into its string's sums, while it's among the string's first `first`.
void LevelRound(CheckRun& c, int first) {
    if (c.count >= 2 && c.number >= 1 && c.number <= first && !c.taken) {
        const float x = static_cast<float>(c.number), y = Median(c.heights, c.count);
        if (c.leveled > 0 && !(std::fabs(y - c.last) <= c.most * (x - c.lastAt)))
            c.shaken = true;
        c.last = y, c.lastAt = x;
        ++c.leveled, c.sx += x, c.sy += y, c.sxx += x * x, c.sxy += x * y;
    }
    c.count = 0;
}

// The string's leveled rounds into the verdict's sums, about their own middle; once a string.
void TakeString(CheckRun& c) {
    if (!c.taken && !c.shaken && c.leveled >= kCheckLeast) {
        const float n = static_cast<float>(c.leveled);
        c.lever += c.sxx - c.sx * c.sx / n;
        c.rise += c.sxy - c.sx * c.sy / n;
        c.rounds += c.leveled, ++c.strings;
    }
    c.taken = true;
}

// The string is over (or dropped: the gun not checked this frame): the next round begins another.
void EndString(CheckRun& c, int first, bool counts) {
    if (counts) {
        LevelRound(c, first);
        TakeString(c);
    }
    c.count = 0, c.since = 1e9f, c.number = 0, c.leveled = 0, c.sx = c.sy = c.sxx = c.sxy = 0.0f, c.taken = false, c.shaken = false;
}

// This frame of the check: `fired` rounds of the gun counted now, the camera's height above its look if one was built since the last frame (have),
// the frame's length, the gun's own seconds between rounds, how many of a string's first rounds count, the most its levels may move a round. True
// with the strings' rise a round when there is enough for a verdict (its sums then begin afresh; rounds and strings what it was made of).
bool CheckStep(CheckRun& c, int fired, bool have, float height, float seconds, float gap, int first, float most, float& slope, int& rounds,
               int& strings) {
    c.most = most;
    c.since += std::isfinite(seconds) && seconds > 0.0f ? seconds : 0.0f;
    if (c.number > 0 && c.since > kCheckGaps * gap)
        EndString(c, first, true);
    if (fired > 0) {
        if (c.number > 0) {
            LevelRound(c, first);
            if (c.number >= first)
                TakeString(c);  // (a long string: judged on its first rounds, while it goes on)
        }
        c.number += fired;
        c.since = 0.0f;
    }
    if (have && std::isfinite(height) && c.number > 0 && c.since <= gap && c.count < static_cast<int>(std::size(c.heights)))
        c.heights[c.count++] = height;
    if (!(c.lever >= kCheckLever))
        return false;
    slope = c.rise / c.lever, rounds = c.rounds, strings = c.strings;
    c.lever = c.rise = 0.0f, c.rounds = c.strings = 0;
    return true;
}

void LogCleared(const char* why) {
    static double s_at = -1e9;
    const double now = Clock();
    if (now - s_at < 2.0)
        return;
    s_at = now;
    Log("Recoil: cleared (%s)", why);
}

void Adopt(const Key& key, const char* shape) {
    if (g_lastActive)
        LogCleared("another gun");
    g_gun = Held{};
    g_m320 = Held{};
    if (key.pose)
        SetUp(g_gun, key, shape, key.slot, false);
    if (key.pose == 1 && ammo::M320Carried())
        SetUp(g_m320, key, nullptr, 2, true);
    g_lastLoaded = -1;
    g_lastWrites = ammo::CountWrites();
    g_check = CheckRun{};
    g_second = Second{};
    g_test = TestRun{};
}

// Every kMaintain: each gun's rows as they are now. The one drawn (and its grenade launcher) out, any other back; a kick the game put back (a
// checkpoint) taken out again; rows that aren't there any more forgotten.
void Maintain(double now, bool drawn) {
    if (now - g_maintainAt < kMaintain)
        return;
    g_maintainAt = now;
    // The grenade launcher under the rifle, met after the rifle was set up.
    if (drawn && g_gun.key.pose == 1 && !g_m320.key.pose && ammo::M320Carried())
        SetUp(g_m320, g_gun.key, nullptr, 2, true);
    for (Saved& s : g_saved) {
        if (!s.sway)
            continue;
        const bool inUse = drawn && (s.sway == g_gun.sway || s.sway == g_m320.sway);
        const Reading r = RowsNow(s);
        if (!r.Whole()) {
            if (s.out)
                Log("Recoil: the %s's rows aren't at %08X any more (a load): forgotten", s.name, static_cast<unsigned>(s.sway));
            if (inUse)
                g_gun.firing = 0;  // (set up again: Step finds it changed)
            s = Saved{};
            continue;
        }
        if (inUse && KickWantedOut()) {
            if (r.game > 0 && !s.refused) {
                if (TakeOut(s))
                    Log("Recoil: the %s's kick was back in its data (%d rows: a checkpoint?): taken out again", s.name, r.game);
                else
                    Log("Recoil: the %s's kick was back in its data (%d rows: a checkpoint?) and a write was refused taking it out again: left in, the "
                        "drift off for it", s.name, r.game);
            }
        } else if (s.out) {
            const int rows = PutBack(s);
            Log("Recoil: the %s's data put back as the game had it (%d rows): not drawn by the mod now", s.name, rows);
        }
    }
}

void Buzz(int side, float amplitude, float seconds) {
    if (side < 0 || side > 1 || !(amplitude > 0.0f) || !(seconds > 0.0f))
        return;
    // The call's cost, said once (up to 30 a second in automatic fire).
    static int s_timed = 0;
    const double before = s_timed < 3 ? Clock() : 0.0;
    input::Buzz(side, std::fmin(amplitude, 1.0f), seconds, g_values.buzzHz);
    if (s_timed < 3 && ++s_timed == 3)
        Log("Recoil: a buzz takes %.3f ms", (Clock() - before) * 1000.0);
}

}  // namespace

void Read() {
    static FILETIME lastWrite{};
    static ULONGLONG lastCheck = 0;
    const ULONGLONG now = GetTickCount64();
    if (g_read && now - lastCheck < 300)
        return;
    lastCheck = now;
    wchar_t path[MAX_PATH];
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&Read), &self);
    GetModuleFileNameW(self, path, MAX_PATH);
    if (wchar_t* slash = wcsrchr(path, L'\\'))
        wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path), L"recoil.txt");
    WIN32_FILE_ATTRIBUTE_DATA info{};
    const bool exists = GetFileAttributesExW(path, GetFileExInfoStandard, &info) != 0;
    if (g_read && CompareFileTime(&info.ftLastWriteTime, &lastWrite) == 0)
        return;
    FILE* f = nullptr;
    if (exists && (_wfopen_s(&f, path, L"r") != 0 || !f))
        return;  // still being written (the open isn't shared): read at the next check
    lastWrite = info.ftLastWriteTime;
    Values v;  // every default, then the file's lines
    if (f) {
        Parse(f, v);
        fclose(f);
    }
    LogChanges(g_values, v, !g_read);
    const bool wasOut = KickWantedOut();
    g_values = v;
    g_read = true;
    // Once per write of the file: its test and its log.
    g_testArmed = v.test != 0;
    if (!v.test)
        g_test = TestRun{};
    g_logUntil = v.logSeconds > 0.0f ? Clock() + v.logSeconds : 0.0;
    // "off" or "game 1": every kick taken out back at once; on again, the gun drawn's out at once (the drift waits for it). Either way the check of
    // the game's camera begins its strings and verdicts afresh.
    if (wasOut != KickWantedOut())
        g_check = CheckRun{};
    if (wasOut && !KickWantedOut())
        PutBackAll(v.on ? "game 1" : "off");
    else if (!wasOut && KickWantedOut() && g_gun.key.pose)
        for (Held* h : {&g_gun, &g_m320})
            if (Saved* s = Find(h->sway))
                TakeOut(*s);
}

bool On() {
    return g_values.on;
}

void Clear() {
    if (g_lastActive)
        LogCleared("a new life, or free hands switched");
    g_clear = true;
    g_notHeld = kChangeFrames;
}

void Watch() {
    Read();
    const double now = Clock();
    // A gun put away in a holster isn't drawn by the mod, yet it stays the mod's: its data stays as the mod set it, and nothing is
    // set up afresh, until it's drawn again (the holsters' films, 2026-11-15: put back a second after every put-away before).
    if (g_watchesSinceStep < kUnwatchedPictures + 1)
        ++g_watchesSinceStep;
    const bool unwatched = g_stepAt > 0.0 && now - g_stepAt > kUnwatched && g_watchesSinceStep > kUnwatchedPictures && !hands::GunStowed();
    if (unwatched || !KickWantedOut()) {
        if (AnyOut())
            PutBackAll(KickWantedOut() ? "not drawn by the mod for a second" : g_values.on ? "game 1" : "off");
        if (unwatched)
            g_gun = Held{}, g_m320 = Held{};  // (set up afresh when Step runs again)
    }
}

const recoil::Output& Step(const In& in) {
    const double now = Clock();
    const float since = g_stepAt > 0.0 ? static_cast<float>(now - g_stepAt) : 0.0f;
    g_stepAt = now;
    g_watchesSinceStep = 0;
    ++g_frame;
    g_out = recoil::Output{};
    recoil::Frame f;
    f.dt = std::isfinite(in.dt) && in.dt > 0.0f ? in.dt : std::fmin(since, recoil::kMostStep);
    f.gap = std::fmax(std::isfinite(in.gap) && in.gap > 0.0f ? in.gap : 0.0f, since);  // (the real gap: hands.cpp's or this clock's, the longer)
    if (g_first)
        g_state.rng = static_cast<uint32_t>(static_cast<uint64_t>(now * 1e6)) | 1u;  // its random numbers, seeded once a run (kept through every clear)
    const bool button = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;  // (the headset's trigger comes as the left mouse button: input.cpp)
    if (button)
        g_triggerAt = now;

    // ---- Recoil off: nothing kicks or buzzes; a kick still drawn eases out ----
    if (!g_values.on) {
        if (g_lastActive) {
            recoil::Hold hold;
            hold.stillDrawn = in.pose != 0;
            recoil::Tune tune;
            g_out = recoil::Step(g_gun.gun, hold, f, tune, g_state);
        }
        g_first = false;
        g_lastActive = g_out.active;
        g_lastLoaded = -1;  // (rounds fired meanwhile are never taken as shots once it's on again)
        g_lastM320 = input::M320Fired();
        g_test = TestRun{};
        scopeview::SetKickSlack(0.0f);
        skin::HoldSpread(false);
        return g_out;
    }

    // ---- 1. The gun: what's drawn, its firing state and data (a change counts once it has lasted kChangeFrames) ----
    const Key key = in.pose >= 1 && in.pose <= 3 ? Key{in.pose, in.id, in.slot} : Key{};
    const uintptr_t firing = key.pose && key.slot >= 0 ? ammo::FiringOf(key.slot) : 0;
    const uintptr_t data = firing ? Pointer(firing + kFiringData) : 0;
    // (Its slot's gun by its name known, and another than as set up: set up again, so a name read a moment late is never one taken for the drawn gun's.)
    const recoilguns::Row* slotRow = key.pose == 1 || key.pose == 2 ? ammo::RowInSlot(key.slot) : nullptr;
    const bool changed = !(key == g_gun.key) || (firing && firing != g_gun.firing) || (data && data != g_gun.data) || (slotRow && slotRow != g_gun.slotRow);
    bool newGun = g_first || g_clear;
    if (!changed) {
        g_changeFrames = 0;
    } else {
        if (!g_changeFrames || !(g_change == key))
            g_change = key, g_changeFrames = 0;
        if (++g_changeFrames >= kChangeFrames || !g_gun.key.pose || g_first) {
            Adopt(key, in.shape);
            g_changeFrames = 0;
            newGun = true;
        }
    }
    g_first = false;
    g_clear = false;
    const bool drawn = g_gun.key.pose != 0;
    Maintain(now, drawn);
    // A change to another gun (another pose or id) still waiting out its frames: what's drawn now isn't the gun the kick is of.
    const bool another = !(key == g_gun.key) && (key.pose != g_gun.key.pose || key.id != g_gun.key.id);

    // ---- 2. The hold (a gun not held keeps its kick a few frames: a frame's glitch never snaps it) ----
    const bool heldNow = in.held && drawn && key == g_gun.key;
    recoil::Hold hold;
    hold.hands = recoil::Safe(in.hands, 0.0f, 1.0f, 0.0f);
    hold.leftTrigger = in.leftTrigger;
    // A front grip buzzes as one only on the grenade launcher under the rifle and on a launcher's own (never a held rifle's handguard).
    hold.frontGrip = in.frontGrip && (key.pose == 1 || recoil::Launcher(g_gun.gun.kind));
    if (heldNow) {
        g_notHeld = 0;
        hold.held = true;
    } else {
        g_notHeld = std::min(g_notHeld + 1, 1000);
        if (g_notHeld < kChangeFrames && g_lastHold.held && !newGun) {
            hold = g_lastHold;  // (the last frame's hold and no rounds: its kick goes on as it was, should the hold come back next frame)
        } else {
            hold.held = false;
            hold.stillDrawn = in.pose != 0;  // still in a hand (by its front alone, a scripted moment): the clear eases out
            if (g_lastActive && g_notHeld == kChangeFrames)
                LogCleared(in.pose ? "not held by its grip" : "let go");
        }
    }

    // ---- 3. The eye at the sight: the nearer eye within 2.5 cm of the sights' line (4 cm to leave, or while the scope's picture shows), along it
    // from 20 cm behind the game's zoomed eye to 6 cm in front of it, or on to just short of the sight's own glass (its rear, irons): the game's
    // zoomed eye sits 16-50 cm behind a scope's glass (the ACOG's 20), and the scope's picture comes on only within about 12 cm of it ----
    skin::SightNow sight;
    const bool haveSight = drawn && recoil::Long(g_gun.gun.kind) && !recoil::Launcher(g_gun.gun.kind) && skin::Sight(sight) &&
                           (!sight.id || sight.id == key.id) && std::isfinite(sight.up) && Finite(sight.line, 3) && std::isfinite(sight.eyeZ);
    bool atEye = false;
    if (haveSight) {
        f.glassZ = sight.glassZ, f.haveGlass = sight.haveGlass && std::isfinite(sight.glassZ);
        f.rearZ = sight.rearZ, f.haveRear = std::isfinite(sight.rearZ) && sight.rearZ != 0.0f;
        const bool guarded = f.haveGlass || f.haveRear;  // the glass, else the gun's nearest on the line: what the kick must keep off the eye
        const float guardZ = f.haveGlass ? f.glassZ : f.rearZ;
        const float up = sight.up * kDegree;
        const float along[3] = {0.0f, -std::sin(up), std::cos(up)};
        int best = -1;
        float bestOff = 1e9f;
        for (int e = 0; e < 2; ++e) {
            if (!Finite(in.eyes[e], 3))
                continue;
            float v[3];
            for (int i = 0; i < 3; ++i)
                v[i] = in.eyes[e][i] - sight.line[i];
            const float a = v[0] * along[0] + v[1] * along[1] + v[2] * along[2];
            float off = 0.0f;
            for (int i = 0; i < 3; ++i)
                off += (v[i] - a * along[i]) * (v[i] - a * along[i]);
            off = std::sqrt(off);
            if (off < bestOff)
                best = e, bestOff = off;
        }
        if (best >= 0) {
            std::memcpy(f.eyeAt, in.eyes[best], sizeof(f.eyeAt));
            const float z = in.eyes[best][2];
            const float limit = g_atEye || scopeview::Showing() ? 0.04f : 0.025f;
            const float front = guarded ? std::fmax(sight.eyeZ + 0.06f, guardZ - 0.005f) : sight.eyeZ + 0.06f;
            atEye = z >= sight.eyeZ - 0.20f && z <= front && bestOff < limit;
            // The eye given to the kick's guard (the glass kept 1.5 cm off it, any hold) only near the sights' line: within 5 cm of it and not past the
            // glass (or the rear) by more than 2 cm. At the hip or pointing up the eye is far off the line and the glass can never reach it; the guard
            // goes by the eye's place along the gun alone, so given there it pushed the gun forward and the kick back was gone.
            f.haveEye = atEye || (bestOff < 0.05f && (!guarded || z <= guardZ + 0.02f));
        }
    }
    g_atEye = atEye;
    if (hold.held && heldNow)
        hold.eye = atEye;
    if (Finite(in.pivot, 3))
        std::memcpy(f.pivot, in.pivot, sizeof(f.pivot));

    // ---- 4. The rounds ----
    // The gun's own, its slot's count going down: the M416's and a held gun's slot (0, or 1 for a gun the game put there), not the launchers' (the mod
    // writes their counts: the SMAW's 5, the Stinger's 1 is a missile's). Not while the grenade launcher under it is in the game's hand; once it's put
    // away (instant), the rifle's rounds kick at once: slot 0's count is the rifle's alone, and every count the mod writes (the launcher's own among
    // them) moves CountWrites.
    int rounds = 0;
    {
        int loaded = -1;
        // (Not while its slot holds another gun by its name: a change of gun under way, that gun's count isn't this one's.)
        const bool counted = drawn && (g_gun.key.slot == 0 || g_gun.key.slot == 1) && !recoil::Launcher(g_gun.gun.kind) && !g_gun.otherInSlot &&
                             ammo::RoundsInSlot(g_gun.key.slot, loaded);
        const unsigned writes = ammo::CountWrites();
        if (counted && g_lastLoaded >= 0 && writes == g_lastWrites && heldNow && !ammo::M320Up() && now - g_triggerAt <= kTriggerLately) {
            const int fired = g_lastLoaded - loaded;
            rounds = fired >= 1 && fired <= 3 ? fired : 0;  // (more is a magazine between frames, not shots)
        }
        g_lastLoaded = counted ? loaded : -1;
        g_lastWrites = writes;
    }
    int m320Rounds = 0;  // the grenade launcher under the rifle's
    {
        const unsigned fired = input::M320Fired();
        const unsigned more = fired - g_lastM320;
        g_lastM320 = fired;
        if (more >= 1 && more <= 3 && heldNow && key.pose == 1 && !newGun) {
            if (!g_m320.key.pose)
                SetUp(g_m320, key, nullptr, 2, true);
            m320Rounds = static_cast<int>(more);
            g_m320At = now;
        }
    }
    const int extra = heldNow ? std::min(std::max(in.extraShots, 0), 3) : 0;
    // Test rounds: no bullet leaves, only the kick, the buzz and the log (with the gun held).
    int testRounds = 0;
    bool testTrigger = false;
    if (g_testArmed && heldNow) {
        g_testArmed = false;
        const float rate = g_values.test == 2 && g_gun.rateBurst > 0.0f ? g_gun.rateBurst : g_gun.gun.rate;
        g_test.kind = g_values.test;
        g_test.left = g_values.testRounds;
        g_test.gap = g_test.kind == 1 ? g_values.testMs / 1000.0 : 60.0 / std::max(rate, 10.0f);
        g_test.next = now;
        if (g_test.kind == 1)
            Log("Recoil: test single %d, %.0f ms apart (the %s)", g_test.left, g_values.testMs, g_gun.name);
        else
            Log("Recoil: test %s %d at %.0f a minute (the %s)", g_test.kind == 2 ? "burst" : "auto", g_test.left, rate, g_gun.name);
    }
    if (g_test.left > 0) {
        if (!heldNow) {
            g_test = TestRun{};
            Log("Recoil: test stopped (the gun not held)");
        } else {
            while (g_test.left > 0 && now >= g_test.next && testRounds < 3)
                ++testRounds, --g_test.left, g_test.next += g_test.gap;
            testTrigger = g_test.kind == 1 ? testRounds > 0 : true;
        }
    }
    const int shots = rounds + m320Rounds + extra + testRounds;
    if (rounds + extra > 0)
        g_lastRoundAt = now;  // (real rounds only: a test's leave the game's camera alone)
    f.shots = shots;
    f.trigger = button || testTrigger;
    f.newGun = newGun;

    // ---- 5. The numbers for this frame: the grenade launcher's for its own round and a moment after, the gun's otherwise ----
    const bool m320Now = g_m320.key.pose && key.pose == 1 && now - g_m320At < kM320Holds;
    Held& h = m320Now ? g_m320 : g_gun;
    Stance(h, in.stance);
    recoil::Gun gun = h.gun;
    gun.eyeFactor = m320Now || !haveSight ? 1.0f : EyeFactor(h.row, sight);  // (the grenade launcher takes its own 1, not the ACOG's)
    recoil::Tune tune;
    tune.drift = DriftOn(h) ? g_values.driftScale : 0.0f;
    tune.kick = g_values.kick ? g_values.kickScale : 0.0f;
    tune.buzz = g_values.buzz ? g_values.buzzScale : 0.0f;
    tune.oneRise = g_values.oneRise, tune.oneBack = g_values.oneBack, tune.pistolOne = g_values.pistolOne;
    tune.eyePivot = g_values.eye, tune.mirror = g_values.mirror, tune.sideSign = g_values.sideSign, tune.liftMost = g_values.lift;
    tune.gun = GunScaleOf(h.row);
    tune.buzzHz = g_values.buzzHz;
    tune.release = g_values.release;
    tune.oneHandSpread = g_values.oneHandSpread;
    tune.driftDamping = g_values.driftDamping;
    if (!drawn)
        gun.kind = recoil::Kind::kNone;  // (nothing drawn: everything clears)

    // ---- 6. This frame's kick ----
    if (g_lastActive && f.gap > recoil::kGap)
        LogCleared("a long frame");
    g_out = recoil::Step(gun, hold, f, tune, g_state);
    if (!Finite(g_out.k, 12) || !Finite(g_out.q, 4) || !std::isfinite(g_out.slack)) {
        static double s_at = -1e9;
        if (now - s_at > 1.0) {
            s_at = now;
            Log("Recoil: a kick came out with a number not finite: none drawn, the state cleared");
        }
        g_state = recoil::State{};
        g_out = recoil::Output{};
    }
    if (hold.held && heldNow)
        g_lastHold = hold;
    g_lastActive = g_out.active;
    // Another gun drawn while its change waits out its frames: the kick's state goes on (should the change flip back, it's where it would be), but none
    // of it is drawn on a gun it isn't of (a switch leaves nothing kicked: the critic's 10); the scatter's hold as it was.
    if (another) {
        const bool spread = g_out.hipSpread;
        g_out = recoil::Output{};
        g_out.hipSpread = spread;
    }

    // ---- 7. Its buzz, the scope's slack, the scatter for the hold ----
    {
        const int trigger = in.leftTrigger ? 0 : 1;
        int first = trigger, second = in.support >= 0 && in.support != trigger ? in.support : -1;
        if (m320Rounds > 0) {  // the grenade launcher's round: the hand on its grip first, the one on the rifle's after
            const int onIt = hands::M320HeldBy();
            if (onIt >= 0)
                second = onIt == trigger ? -1 : trigger, first = onIt;
        }
        Buzz(first, g_out.pulse[0], g_out.pulseFor[0]);
        Buzz(second, g_out.pulse[1], g_out.pulseFor[1]);
        if (g_out.late > 0.0f && g_out.lateFor > 0.0f) {
            g_lateAt = now + std::fmax(g_out.lateIn, 0.0f), g_late = g_out.late, g_lateFor = g_out.lateFor;
            g_lateSides[0] = first, g_lateSides[1] = second;
        }
        if (g_lateAt >= 0.0 && now >= g_lateAt) {
            if (heldNow)
                for (const int side : g_lateSides)
                    Buzz(side, g_late, g_lateFor);
            g_lateAt = -1.0;
        }
    }
    scopeview::SetKickSlack(g_out.slack);
    skin::HoldSpread(g_out.hipSpread);

    // ---- 8. The check that the game's own kick is out of its camera, and the logs ----
    float devYaw = 0.0f, devPitch = 0.0f;
    const bool firing2 = now - g_lastRoundAt < 0.3;
    const bool dev = aim::WeaponDeviation(devYaw, devPitch) && std::isfinite(devYaw) && std::isfinite(devPitch);
    if (dev && firing2 && !m320Now) {
        if (g_second.cams < static_cast<int>(std::size(g_second.camUp)))
            g_second.camUp[g_second.cams] = devPitch / kDegree, g_second.camSide[g_second.cams] = devYaw / kDegree, ++g_second.cams;
    }
    // The gun's own degrees up a round as the game has them for this stance (its hip row and its aimed one: the same on most guns), and how many of
    // a string's first rounds would stay under its cap. The check runs while the drift is wanted and the kick out of the gun's own data, on or off
    // by the check itself: a gun whose camera no longer climbs gets its drift back. Two hands only: with one hand the kick throws the look degrees
    // a frame, and the camera's look is told from the last frames' by which is nearest, so a climb could pass for one of them. With "game 1" and
    // "log" (a film's: the game's kick left in its data on purpose) the camera is watched the same way and each verdict said, nothing switched.
    const int stance = in.stance >= 0 && in.stance <= 2 ? in.stance : 0;
    const float upHip = g_gun.rows[stance * 2][kUp], upEye = g_gun.rows[stance * 2 + 1][kUp], capUp = g_gun.rows[stance * 2][kCap];
    const float upLeast = std::fmin(upHip, upEye), upMost = std::fmax(upHip, upEye);
    const int first = upMost > 0.0f && capUp > 0.0f ? static_cast<int>(std::fmin(capUp / upMost, static_cast<float>(kCheckFirst))) : 0;
    const bool logging = g_logUntil > 0.0 && now < g_logUntil, wanted = DriftWanted(g_gun);
    const bool watched = !wanted && logging && g_values.game && g_values.drift && !g_gun.driftOff;
    if (!m320Now && g_gun.sway && (wanted || watched) && hold.hands >= 0.5f && upLeast >= kCheckUpLeast && first >= kCheckLeast) {
        float slope = 0.0f;
        int counted = 0, strings = 0;
        const bool verdict =
            CheckStep(g_check, rounds + extra, dev, devPitch / kDegree, f.gap, 60.0f / std::fmax(g_gun.gun.rate, 10.0f), first, kCheckJump * upMost, slope,
                       counted, strings);
        const bool climbs = verdict && slope >= kCheckShare * upLeast && slope <= kCheckTwice * upMost;
        if (verdict && watched) {
            Log("Recoil: a verdict on the %s's camera: it climbs %.2f degrees a round over %d rounds in %d strings (the game's own kick for it %.2f a "
                "round): %s (game 1: its kick left in on purpose, nothing switched)",
                g_gun.name, slope, counted, strings, upMost, climbs ? "climbing" : "not climbing");
        } else if (verdict) {
            g_check.climbing = climbs ? g_check.climbing + 1 : 0;
            g_check.level = climbs ? 0 : g_check.level + 1;
            if (logging)  // ("log N": every verdict)
                Log("Recoil: a verdict on the %s's camera: it climbs %.2f degrees a round over %d rounds in %d strings (the game's own kick for it %.2f "
                    "a round): %s, %d running",
                    g_gun.name, slope, counted, strings, upMost, climbs ? "climbing" : "not climbing", climbs ? g_check.climbing : g_check.level);
            if (!g_gun.climbs && g_check.climbing >= kCheckRun) {
                g_gun.climbs = true;
                Log("Recoil: the weapon camera climbs %.2f degrees a round above the written look in the %s's strings (the game's own kick for it %.2f a "
                    "round; %d verdicts running, the last over %d rounds in %d strings): the game's kick still in its data: the drift off for the %s",
                    slope, g_gun.name, upMost, g_check.climbing, counted, strings, g_gun.name);
            } else if (g_gun.climbs && g_check.level >= kCheckRun) {
                g_gun.climbs = false;
                Log("Recoil: the weapon camera no longer climbs with the %s's rounds (%.2f degrees a round over %d rounds in %d strings, %d verdicts "
                    "running; the game's own kick for it %.2f a round): its kick is out of the game's camera: the drift on again",
                    g_gun.name, slope, counted, strings, g_check.level, upMost);
            } else if (!climbs && !g_gun.climbs && !g_check.told) {
                g_check.told = true;
                Log("Recoil: the %s's kick is out of the game's camera: it climbs %.2f degrees a round above the look it was built from over %d rounds "
                    "in %d strings (the game's own kick for it %.2f a round)",
                    g_gun.name, slope, counted, strings, upMost);
            }
        }
    } else if (g_check.number) {
        EndString(g_check, first, false);  // (not checked this frame: the string under way is dropped)
    }
    // Once a second while firing.
    if (shots > 0 && g_second.start < 0.0)
        g_second.start = now;
    if (g_second.start >= 0.0) {
        g_second.rounds += shots;
        g_second.up = std::fmax(g_second.up, g_out.pitch);
        g_second.drift = std::fmax(g_second.drift, g_out.drift[0]);
        g_second.kick = std::fmax(g_second.kick, g_out.pitch - g_out.drift[0]);
        if (std::fabs(g_out.yaw) > std::fabs(g_second.side))
            g_second.side = g_out.yaw;
        g_second.back = std::fmax(g_second.back, g_out.back);
        if (now - g_second.start >= 1.0) {
            char seen[96] = "the weapon camera not compared (no look written)";
            if (g_second.cams > 0) {
                const float up = Median(g_second.camUp, g_second.cams), side = Median(g_second.camSide, g_second.cams);
                sprintf_s(seen, "the weapon camera %.2f degrees %s the written look, %.2f to the side", std::fabs(up), up < 0.0f ? "below" : "above", std::fabs(side));
            }
            // (Its slot at the end: tools/recoilfit.ps1 and the film scripts read the gun's name up to the comma after it.)
            Log("Recoil: %d rounds in the last second (the %s, %s, at the eye %.1f): the drawn gun up %.1f degrees at most (drift %.1f, kick %.1f), %.1f %s, "
                "%.1f cm back; %s; its rounds counted in slot %d",
                g_second.rounds, h.name, hold.hands >= 0.5f ? "two hands" : "one hand", g_state.eye, g_second.up, g_second.drift, g_second.kick,
                std::fabs(g_second.side), g_second.side < 0.0f ? "left" : "right", g_second.back * 100.0f, seen, g_gun.key.slot);
            g_second = Second{};
        }
    }
    // The aim's own limits: the clamp to the head (xr_runtime.cpp's max_aim_offset) and the game's own pitch limits, while it kicks.
    if (g_out.active && heldNow) {
        static double s_clampAt = -1e9, s_pitchAt = -1e9;
        if (in.heldPose && Finite(in.heldPose, 12) && now - s_clampAt > 1.0) {
            const float up = (haveSight ? sight.up : 0.0f) * kDegree;
            const float line[3] = {0.0f, -std::sin(up), std::cos(up)};
            float turn[9], kicked[3], v[3];
            recoil::QMatrix(g_out.q, turn);
            for (int i = 0; i < 3; ++i)
                kicked[i] = turn[i * 3] * line[0] + turn[i * 3 + 1] * line[1] + turn[i * 3 + 2] * line[2];
            for (int i = 0; i < 3; ++i)
                v[i] = in.heldPose[i * 4] * kicked[0] + in.heldPose[i * 4 + 1] * kicked[1] + in.heldPose[i * 4 + 2] * kicked[2];
            const float yawOff = std::atan2(v[0], -v[2]) / kDegree, pitchOff = std::asin(std::fmax(std::fmin(v[1], 1.0f), -1.0f)) / kDegree;
            const float limit = config::Get().maxAimOffsetDeg, off = std::fmax(std::fabs(yawOff), std::fabs(pitchOff));
            if (std::isfinite(off) && off > limit) {
                s_clampAt = now;
                Log("Recoil: the aim clamped at %.0f degrees from the head: the drawn gun %.0f degrees off it", limit, off);
            }
        }
        float lookYaw = 0.0f, lookPitch = 0.0f, pitchHeld = 0.0f;
        if (now - s_pitchAt > 1.0 && aim::LastWritten(lookYaw, lookPitch, &pitchHeld) && std::fabs(pitchHeld) > 0.5f * kDegree) {
            s_pitchAt = now;
            Log("Recoil: the game's own pitch limit held the look %.1f degrees short of the drawn gun", std::fabs(pitchHeld) / kDegree);
        }
    }
    // "log N": a line a frame (P2's input).
    if (g_logUntil > 0.0 && now < g_logUntil) {
        char look[48] = "- -", cam[48] = "- -";
        float lookYaw = 0.0f, lookPitch = 0.0f, camYaw = 0.0f, camPitch = 0.0f;
        if (aim::LastWritten(lookYaw, lookPitch))
            sprintf_s(look, "%.3f %.3f", Degrees(lookYaw), lookPitch / kDegree);
        if (camera::GameWeaponAngles(camYaw, camPitch))
            sprintf_s(cam, "%.3f %.3f", Degrees(camYaw), camPitch / kDegree);
        Log("Recoil: f %u shots %d hold %.2f eye %.2f drift %.3f %.3f kick %.3f %.3f %.3f back %.2f lift %.2f look %s camera %s", g_frame, shots, hold.hands,
            g_state.eye, g_out.drift[0], g_out.drift[1], g_out.pitch - g_out.drift[0], g_out.yaw - g_out.drift[1], g_out.roll, g_out.back * 100.0f,
            g_out.lift * 100.0f, look, cam);
    }
    return g_out;
}

}  // namespace recoilrun

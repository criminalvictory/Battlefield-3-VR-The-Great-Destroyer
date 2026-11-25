// Recoil's game side (src/recoilrun.cpp, compiled in here) against fake game memory laid out as the game's own data (a firing state, its
// WeaponFiringData, FiringFunctionData, GunSwayData and camera spring), every call out of it stubbed, without the game. Each check prints PASS or
// FAIL, then the totals; the exit code is the number failed. Its recoil.txt is in a folder of its own under the temp folder, never beside the DLL.
//   1  the M416 drawn: the identity before its first round; its six rows' up and sideways 0, caps and recovery the game's; the take-out line; the
//      drift on; its row the M416's
//   2  off: its data the game's bit for bit, no kick, no slack, two hands' scatter; the file deleted: out again at once
//   3  game 1: the data the game's, the drift off; game 0: out again, the drift on
//   4  recoil.txt's numbers read as numbers: kick 1.3 on at 1.3, kick 0.7 on at 0.7 (once off), kick 1 1.3, kick 0, kick -1, a lone 1, the buzz's
//      strength and hertz both ways
//   5  the shots by the count: a round kicks; a drop with a count the mod wrote, the button up 0.35 s, a magazine's 10, the grenade launcher in the
//      game's hand, the gun not held: none; the launcher put away a moment ago: the rifle's round kicks at once
//   6  Watch: no Step for a second: put back; Step again: out again
//   7  a checkpoint puts the kick back: out again at the next look, said
//   8  another gun drawn: its kick out, the M416's back at the next look, its own row
//   9  the slot holding another gun's data, data that doesn't read as gun sway, a NaN in a row: nothing written; no gun sway: the table's, the drift on
//  10  a write refused half way through taking a kick out: what was written put back (the data the game's bit for bit), the drift off, never tried
//      again; its kick in the hands still there
//  11  the grenade launcher under the rifle: both kicks out, its round's 40 mm kick; its slot holding data whose sway isn't the 40 mm's: nothing
//      written there, said, the launcher's drift off
//  12  test rounds; "log", a gun's own scale, a name not known
//  13  the eye at the ACOG: eased in, 3 cm off the line still (4 cm to leave), 5 cm off out, 3 cm not again, the scope's picture on 3 cm in; the eye
//      3 cm behind the ACOG's own glass (where its picture shows): at the eye; 1 cm in front of the glass, or at the hip: not
//  14  the eye far off the sights' line (the gun at the hip, pointed up from the chest; one hand and two): every frame's kick as with no sight known,
//      bit for bit, the trigger hand's place never more than 5 mm forward, the kick back there; one hand at the eye 2 cm behind the glass: the eye
//      never nearer the glass than as held
//  15  a switch: nothing kicked drawn on another gun while its change waits out its frames, the kick where it was when the change flips back; nothing
//      drawn for 3 frames: gone
//  16  nothing not finite from broken inputs
//  17  the clears: the gun gone, a new life
//  20  (run here, before 18 and 19, which look at all before them) a gun taken up into slot 1 with the M416 left in slot 0: set up from its own slot,
//      its round kicks and buzzes in the trigger's hand, slot 0's drop and a written drop none, the line a second with its slot; the Stinger in slot
//      1: no round by its count, its missile by the extra shot; a gun the table doesn't know in slot 1: a rifle, not a missile; the M416 again as before
//  21  (also before 18 and 19) the check of the game's camera, the weapon camera's height above its look given frame by frame: the M416 as a film had
//      it (five strings of six, the camera 2.5 degrees to the side, no climb), the AN-94's two-round bursts, the user's M416 (a string of 13 and one
//      of 4, the camera 0.41 above and 0.7 either way), shakes (1.6 degrees up from a string's third round on, a sound string after each), a rise far
//      past the gun's own kick: the drift on; the camera 0.26 higher at every round of three strings: off at the third, said, the kick in the hands
//      still there; still climbing: off; no longer: on again at the third verdict, said; a long string judged on its first ten; the gun set up again:
//      afresh; rounds 1.9 gaps apart no string; rounds with one hand and a gun whose own kick reaches its cap in four rounds never judged; with
//      recoil.txt's log on every verdict said; with its "game 1" too (the kick left in on purpose) the camera watched, nothing switched
//  22  (also before 18 and 19) a change of gun under way, as Operation Swordbreaker's AT4 kit on the roof: the M4A1 drawn with the AT4 in its slot
//      by its name: the AT4's rows never taken out under the M4A1's name, said, the drift off, its count no round; the M4A1's own put back; the AT4
//      drawn: its own; a fresh M4A1 under the AT4's tube: untouched, the AT4's put back; the M4A1 drawn again; a slot's name read late: set up again
//      and put back; no name known: the drawn gun's, as before
//  18  unload: every kick taken out back, bit for bit
//  19  no log line the self-test would take for a failure
// Usage: recoilruntest

#include <windows.h>

#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>
#include <vector>

// The fire button and the DLL's own path, as recoilrun reads them: the button the test's; the path in a folder of the test's own, so the recoil.txt
// it reads and writes is never the one beside the DLL.
static SHORT g_key = 0;
static SHORT FakeKeyState(int) { return g_key; }
static std::wstring g_dir;
static DWORD FakeModuleFileName(HMODULE, LPWSTR out, DWORD size) {
    const std::wstring path = g_dir + L"recoilruntest.exe";
    if (!out || !size)
        return 0;
    wcsncpy_s(out, size, path.c_str(), _TRUNCATE);
    return static_cast<DWORD>(wcslen(out));
}
#define GetAsyncKeyState FakeKeyState
#define GetModuleFileNameW FakeModuleFileName
#include "../src/recoilrun.cpp"
#undef GetModuleFileNameW
#undef GetAsyncKeyState

namespace {

// ---- Fake game memory (x86: every pointer in it 32 bits, as the game's) ----
struct FakeCamera { uint8_t pad[8]; float spring, damping, threshold; uint8_t rest[4]; };
struct FakeSway { uint8_t bytes[0x328]; };
struct FakeFire { uint8_t bytes[0x210]; };
struct FakeData { uint8_t bytes[0x44]; };
struct FakeFiring { uint8_t bytes[0x130]; };

template <typename T> void Put(void* base, size_t at, T v) { std::memcpy(static_cast<uint8_t*>(base) + at, &v, sizeof(T)); }
template <typename T> T Get(const void* base, size_t at) { T v; std::memcpy(&v, static_cast<const uint8_t*>(base) + at, sizeof(T)); return v; }
uint32_t Address(const void* p) { return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(p)); }

struct FakeGun {
    FakeCamera camera{};
    FakeSway sway{};
    FakeFire fire{};
    FakeData data{};
    FakeFiring firing{};
    void Make(float up, float hmin, float hmax, float decrease, float first, float shooting, float rate, float speed, int magazine, bool withSway = true) {
        std::memset(this, 0, sizeof(*this));
        camera.spring = 250.0f, camera.damping = 0.35f;
        for (int b = 0; b < 6; ++b) {
            const size_t at = recoilrun::kRecoilRows[b];
            Put(&sway, at + 0, 10.0f);
            Put(&sway, at + 4, up);
            Put(&sway, at + 8, hmin);
            Put(&sway, at + 12, hmax);
            Put(&sway, at + 16, 10.0f);
            Put(&sway, at + 20, decrease);
        }
        for (int i = 0; i < 4; ++i)
            Put(&sway, 0x30C + 4 * i, 1.0f);
        Put(&sway, 0x31C, shooting);
        Put(&sway, 0x320, first);
        Put(&sway, 0x324, Address(&camera));
        Put(&fire, 0x120, rate);
        Put(&fire, 0x124, rate);
        Put(&fire, 0x88, speed);
        Put(&fire, 0xB0, 1);
        Put(&fire, 0x15C, magazine);
        Put(&data, 0x08, Address(&fire));
        SwayAt(withSway ? &sway : nullptr);
        Put(&data, 0x24, 0.3f);  // SupportDelayStand and Prone, as in the game (the sway was once read here: 0.3 isn't a pointer)
        Put(&data, 0x28, 0.3f);
        Put(&data, 0x2C, 0.2f);
        Put(&data, 0x30, 1.0f);
        Put(&data, 0x34, 0.12f);
        Put(&firing, 0x94, Address(&data));
        Put(&firing, 0xF4, 30);
    }
    void SwayAt(const FakeSway* at) { Put(&data, 0x20, at ? Address(at) : 0u); }  // (WeaponFiringData's GunSwayData*, as read in the game)
    int Count() const { return Get<int>(&firing, 0xF4); }
    void SetCount(int n) { Put(&firing, 0xF4, n); }
};
float Row(const FakeSway& s, int b, int k) { return Get<float>(&s, recoilrun::kRecoilRows[b] + 4 * k); }

FakeGun* g_slot[8] = {};
const recoilguns::Row* g_slotRow[8] = {};  // each slot's gun by its name (ammo::RowInSlot): none known unless a check sets it
unsigned g_writes = 0, g_m320Fired = 0;
bool g_m320Carried = false, g_m320Up = false, g_m320Recent = false, g_showing = false, g_haveSight = false;
skin::SightNow g_sight;
std::vector<std::string> g_log;
std::vector<int> g_buzzSides;  // the hand of each buzz sent (input::Buzz)
float g_slack = -1.0f;
int g_spread = -1;
float g_cameraUp = 0.0f, g_cameraAside = 0.0f;  // the weapon camera off the look it was built from, as aim would give it (degrees: above it, its yaw apart)

}  // namespace

// ---- Everything recoilrun calls out to ----
namespace camera {
bool SafeRead(uintptr_t address, void* out, size_t size) {
    __try {
        std::memcpy(out, reinterpret_cast<const void*>(address), size);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
bool GameWeaponAngles(float& yaw, float& pitch) {
    yaw = 0.1f, pitch = 0.05f;
    return true;
}
}  // namespace camera
namespace ammo {
uintptr_t FiringOf(int slot) { return slot >= 0 && slot < 8 && g_slot[slot] ? reinterpret_cast<uintptr_t>(&g_slot[slot]->firing) : 0; }
bool RoundsInSlot(int slot, int& loaded) {
    if (!FiringOf(slot))
        return false;
    loaded = g_slot[slot]->Count();
    return true;
}
unsigned CountWrites() { return g_writes; }
const recoilguns::Row* RowInSlot(int slot) { return slot >= 0 && slot < 8 && g_slot[slot] ? g_slotRow[slot] : nullptr; }
bool M320Carried() { return g_m320Carried; }
bool M320Up() { return g_m320Up; }
bool M320UpOrRecent() { return g_m320Up || g_m320Recent; }  // (never to be asked by recoil: the rifle's rounds right after a grenade kick)
}  // namespace ammo
namespace input {
void Buzz(int side, float, float, float) { g_buzzSides.push_back(side); }
unsigned M320Fired() { return g_m320Fired; }
}  // namespace input
namespace skin {
bool Sight(SightNow& out) {
    out = g_sight;
    return g_haveSight;
}
void HoldSpread(bool oneHand) { g_spread = oneHand ? 1 : 0; }
}  // namespace skin
namespace scopeview {
void SetKickSlack(float radians) { g_slack = radians; }
bool Showing() { return g_showing; }
}  // namespace scopeview
namespace aim {
bool WeaponDeviation(float& yaw, float& pitch) {
    yaw = g_cameraAside * recoil::kDegree, pitch = g_cameraUp * recoil::kDegree;
    return true;
}
bool LastWritten(float& yaw, float& pitch, float* held) {
    yaw = 0.1f, pitch = 0.05f;
    if (held)
        *held = 0.0f;
    return true;
}
}  // namespace aim
namespace hands {
int M320HeldBy() { return 0; }
bool g_testStowed = false;  // (a gun put away in a holster: the guard leaves its kick alone)
bool GunStowed() { return g_testStowed; }
}  // namespace hands
namespace config {
const Settings& Get() {
    static Settings s;
    return s;
}
}  // namespace config
namespace logx {
void Write(const char* format, ...) {
    char text[2048];
    va_list args;
    va_start(args, format);
    std::vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    g_log.push_back(text);
}
}  // namespace logx

namespace {

int g_failed = 0, g_passed = 0;
void Expect(bool ok, const char* what, const char* format = nullptr, ...) {
    char numbers[400] = "";
    if (format) {
        va_list args;
        va_start(args, format);
        std::vsnprintf(numbers, sizeof(numbers), format, args);
        va_end(args);
    }
    std::printf("%s  %s%s%s\n", ok ? "PASS" : "FAIL", what, *numbers ? " | " : "", numbers);
    (ok ? g_passed : g_failed)++;
}
bool Logged(const char* part, size_t from = 0) {
    for (size_t i = from; i < g_log.size(); ++i)
        if (g_log[i].find(part) != std::string::npos)
            return true;
    return false;
}
std::wstring g_file;
void WriteRecoilTxt(const char* text) {
    Sleep(320);  // (Read looks every 300 ms)
    if (!text) {
        DeleteFileW(g_file.c_str());
    } else {
        FILE* f = nullptr;
        if (_wfopen_s(&f, g_file.c_str(), L"w") == 0 && f) {
            std::fputs(text, f);
            std::fclose(f);
        }
    }
    recoilrun::Read();
}
recoilrun::In Hold(int pose = 1, uint32_t id = 12768, int slot = 0) {
    recoilrun::In in;
    in.dt = 1.0f / 72.0f, in.gap = 1.0f / 72.0f;
    in.pose = pose, in.id = id, in.slot = slot;
    in.held = true;
    in.hands = 1.0f;
    in.support = 0;
    in.pivot[0] = -0.063f, in.pivot[1] = -0.091f, in.pivot[2] = 0.224f;
    in.eyes[0][0] = -0.3f, in.eyes[0][1] = 0.3f, in.eyes[0][2] = -0.3f;
    in.eyes[1][0] = -0.24f, in.eyes[1][1] = 0.3f, in.eyes[1][2] = -0.3f;
    return in;
}
// Frames as fast as they come: the kick is stepped by each one's dt (1/72 s), never the clock. What goes by the clock waits for it where it's
// tested (the button's 0.3 s, Watch's second, the test rounds, the grenade launcher's numbers for 0.6 s after its round).
recoil::Output Frames(const recoilrun::In& in, int n) {
    recoil::Output out;
    for (int i = 0; i < n; ++i)
        out = recoilrun::Step(in);
    return out;
}
bool Identity(const recoil::Output& o) {
    const recoil::Output none;
    return !o.active && std::memcmp(o.k, none.k, sizeof(none.k)) == 0 && std::memcmp(o.q, none.q, sizeof(none.q)) == 0 && o.slack == 0.0f;
}
bool RowsAre(const FakeSway& s, float up, float hmin, float hmax) {
    for (int b = 0; b < 6; ++b)
        if (Row(s, b, 1) != up || Row(s, b, 2) != hmin || Row(s, b, 3) != hmax || Row(s, b, 0) != 10.0f || Row(s, b, 4) != 10.0f)
            return false;
    return true;
}
bool Same(const FakeSway& a, const FakeSway& b) { return std::memcmp(&a, &b, sizeof(a)) == 0; }

// The M416's ACOG as skin has it: its line at the muzzle (0, 0.109, 0.78), 0.29 degrees below z, the game's zoomed eye at z 0.148, its glass 0.349.
void Acog() {
    g_sight = skin::SightNow{};
    g_sight.line[0] = 0.0f, g_sight.line[1] = 0.109f, g_sight.line[2] = 0.78f;
    g_sight.up = 0.29f, g_sight.eyeZ = 0.148f, g_sight.power = 3.44f, g_sight.sight = 10;
    g_sight.glassZ = 0.349f, g_sight.haveGlass = true, g_sight.rearZ = 0.349f, g_sight.id = 12768;
}
// The sights' line's height at z (model).
float LineY(float z) { return g_sight.line[1] + (g_sight.line[2] - z) * std::tan(g_sight.up * 3.14159265f / 180.0f); }
// Both eyes: the nearer at `eye`, the other 6.4 cm to its left.
void Eyes(recoilrun::In& in, float x, float y, float z) {
    in.eyes[0][0] = x, in.eyes[0][1] = y, in.eyes[0][2] = z;
    in.eyes[1][0] = x + 0.064f, in.eyes[1][1] = y, in.eyes[1][2] = z;
}

// A string of rounds by the count (one every `every` frames, the button down), from a clear with these random numbers, and a second after.
struct Run {
    std::vector<float> backs, pitches;
    float backMost = 0.0f;        // the kick's back (m)
    float forwardMost = -1.0f;    // the trigger hand's place on the gun carried forward along the gun as the hands hold it (m)
    float eyeMost = -1e9f;        // the nearer eye's z in the kicked gun (m)
};
Run Fire(const recoilrun::In& in, FakeGun& gun, int rounds, int every, uint32_t seed) {
    gun.SetCount(30);
    recoilrun::Clear();
    Frames(in, 1);
    recoilrun::g_state.rng = seed;
    g_key = static_cast<SHORT>(0x8000);
    Frames(in, 2);
    Run r;
    int left = rounds;
    for (int i = 0; i < rounds * every + 72; ++i) {
        if (left > 0 && i % every == 0)
            gun.SetCount(gun.Count() - 1), --left;
        const recoil::Output o = recoilrun::Step(in);
        const float* k = o.k;
        const float* p = in.pivot;
        const float z = k[8] * p[0] + k[9] * p[1] + k[10] * p[2] + k[11];
        const float d[3] = {in.eyes[0][0] - k[3], in.eyes[0][1] - k[7], in.eyes[0][2] - k[11]};
        r.forwardMost = std::fmax(r.forwardMost, z - p[2]);
        r.eyeMost = std::fmax(r.eyeMost, k[2] * d[0] + k[6] * d[1] + k[10] * d[2]);
        r.backMost = std::fmax(r.backMost, o.back);
        r.backs.push_back(o.back);
        r.pitches.push_back(o.pitch);
    }
    g_key = 0;
    return r;
}

// A string of rounds by the count (one every `every` frames, the button down) with the weapon camera's height above its look given for every frame
// (degrees: by the round's number in the string from 1 and the frame's number since that round from 0), then `rest` frames with the camera on its
// look (a string is over 1.75 of the gun's own gaps after its last round).
template <class Height> void String(const recoilrun::In& in, FakeGun& gun, int rounds, int every, int rest, Height height) {
    gun.SetCount(rounds + 5);
    g_key = static_cast<SHORT>(0x8000);
    g_cameraUp = 0.0f;
    Frames(in, 1);
    for (int r = 1; r <= rounds; ++r) {
        gun.SetCount(gun.Count() - 1);
        for (int f = 0; f < every; ++f) {
            g_cameraUp = height(r, f);
            recoilrun::Step(in);
        }
    }
    g_cameraUp = 0.0f;
    Frames(in, rest);
    g_key = 0;
}

}  // namespace

int main() {
    // Its own folder for recoil.txt (and nothing else), one a run: two at once never read each other's.
    wchar_t temp[MAX_PATH] = {};
    if (!GetTempPathW(MAX_PATH, temp))
        return 1;
    g_dir = std::wstring(temp) + L"bf3vr_recoilruntest_" + std::to_wstring(GetCurrentProcessId()) + L"\\";
    CreateDirectoryW(g_dir.c_str(), nullptr);
    g_file = g_dir + L"recoil.txt";
    DeleteFileW(g_file.c_str());

    static FakeGun m416, m416b, m320, ak, odd, nan, bare, pistol;
    m416.Make(0.26f, -0.3f, 0.1f, 18.0f, 2.0f, 0.1f, 750.0f, 600.0f, 31);
    m416b.Make(0.26f, -0.3f, 0.1f, 18.0f, 2.0f, 0.1f, 750.0f, 600.0f, 31);
    m320.Make(2.5f, -1.0f, -0.5f, 15.0f, 1.0f, 1.0f, 250.0f, 45.0f, 1);
    ak.Make(0.28f, -0.3f, 0.2f, 18.0f, 1.5f, 0.1f, 650.0f, 600.0f, 31);
    pistol.Make(0.7f, -0.1f, 0.1f, 20.0f, 1.0f, 0.2f, 400.0f, 380.0f, 1);
    const FakeSway original = m416.sway, m320Original = m320.sway, akOriginal = ak.sway;
    g_slot[0] = &m416;

    std::printf("-- 1. the M416 drawn: its own kick out of its data, bit for bit kept\n");
    recoilrun::Read();
    recoilrun::In in = Hold();
    const recoil::Output first = Frames(in, 2);
    Expect(Identity(first), "1  before the first round: the identity, bit for bit");
    Expect(RowsAre(m416.sway, 0.0f, 0.0f, 0.0f), "1a its six rows' up and sideways 0, caps and recovery the game's");
    Expect(Logged("the M416's own kick taken out of the game's data: 0.26 up a shot"), "1b the take-out line, its numbers the data's");
    Expect(recoilrun::DriftOn(recoilrun::g_gun), "1c the drift on (its kick out)");
    Expect(recoilrun::g_gun.row && std::strcmp(recoilrun::g_gun.row->name, "M416") == 0, "1d its row the M416's");

    std::printf("-- 2. off: put back bit for bit; on: out again\n");
    WriteRecoilTxt("off\n");
    Expect(Same(m416.sway, original), "2a off: the data the game's, bit for bit");
    const recoil::Output offOut = Frames(in, 3);
    Expect(Identity(offOut) && g_slack == 0.0f && g_spread == 0, "2b off: no kick, no slack, two hands' scatter");
    WriteRecoilTxt(nullptr);
    Expect(RowsAre(m416.sway, 0.0f, 0.0f, 0.0f), "2c the file deleted (on): out again at once");

    std::printf("-- 3. game 1: put back, the drift off; game 0: out again\n");
    WriteRecoilTxt("game 1\n");
    Expect(Same(m416.sway, original), "3a game 1: the data the game's");
    Expect(!recoilrun::DriftOn(recoilrun::g_gun), "3b game 1: the drift off");
    WriteRecoilTxt("game 0\n");
    Expect(RowsAre(m416.sway, 0.0f, 0.0f, 0.0f) && recoilrun::DriftOn(recoilrun::g_gun), "3c game 0: out again, the drift on");

    std::printf("-- 4. recoil.txt's numbers read as numbers\n");
    {
        const recoilrun::Values& v = recoilrun::g_values;
        WriteRecoilTxt("kick 1.3\n");
        Expect(v.kick && v.kickScale == 1.3f, "4a kick 1.3: on at 1.3 (once on at 0.3)", "on %d, %.2f", v.kick, v.kickScale);
        WriteRecoilTxt("kick 0.7\n");
        Expect(v.kick && v.kickScale == 0.7f, "4b kick 0.7: on at 0.7 (once off)", "on %d, %.2f", v.kick, v.kickScale);
        WriteRecoilTxt("kick 1 1.3\ndrift 1 0.5\n");
        Expect(v.kick && v.kickScale == 1.3f && v.drift && v.driftScale == 0.5f, "4c kick 1 1.3, drift 1 0.5: on, at their strengths");
        WriteRecoilTxt("kick 0\ndrift -1\n");
        Expect(!v.kick && !v.drift, "4d kick 0, drift -1: off");
        WriteRecoilTxt("kick 1\n");
        Expect(v.kick && v.kickScale == 1.0f, "4e a lone 1: on at its default");
        WriteRecoilTxt("buzz 0.8 180\n");
        Expect(v.buzz && v.buzzScale == 0.8f && v.buzzHz == 180.0f, "4f buzz 0.8 180: on at 0.8, 180 Hz");
        WriteRecoilTxt("buzz 1 0.6 90\n");
        Expect(v.buzz && v.buzzScale == 0.6f && v.buzzHz == 90.0f, "4g buzz 1 0.6 90: on at 0.6, 90 Hz");
        WriteRecoilTxt("kick 9\n");
        Expect(v.kick && v.kickScale == 5.0f, "4h kick 9: on, held to the most (5)");
        WriteRecoilTxt(nullptr);
        Expect(v.kick && v.kickScale == 1.0f && v.drift && v.buzz && v.buzzHz == 0.0f, "4i the file deleted: every default");
    }

    std::printf("-- 5. shots by the count\n");
    g_key = static_cast<SHORT>(0x8000);
    m416.SetCount(30);
    Frames(in, 2);
    m416.SetCount(29);
    recoilrun::Step(in);
    const recoil::Output kicked = Frames(in, 4);
    Expect(kicked.active && kicked.pitch > 0.5f, "5a one round: the gun kicked up", "%.2f degrees", kicked.pitch);
    Frames(in, 90);
    m416.SetCount(27);
    ++g_writes;  // (a count the mod wrote)
    recoilrun::Step(in);
    const recoil::Output quiet = Frames(in, 1);
    Expect(quiet.pitch < 0.5f, "5b a drop with a count written: no shot");
    Frames(in, 30);
    g_key = 0;
    Sleep(350);
    m416.SetCount(26);
    const recoil::Output noButton = Frames(in, 4);
    Expect(noButton.pitch < 0.5f, "5c a drop with the button up for 0.35 s: no shot");
    g_key = static_cast<SHORT>(0x8000);
    Frames(in, 2);
    m416.SetCount(16);
    const recoil::Output magazine = Frames(in, 4);
    Expect(magazine.pitch < 0.5f, "5d a drop of 10 (a magazine): no shot");
    g_m320Up = true;
    m416.SetCount(15);
    const recoil::Output m320up = Frames(in, 4);
    Expect(m320up.pitch < 0.5f, "5e the grenade launcher in the game's hand: no rifle shot taken");
    g_m320Up = false;
    recoilrun::In loose = in;
    loose.held = false;
    Frames(loose, 5);
    m416.SetCount(14);
    Frames(loose, 2);
    const recoil::Output notHeld = Frames(in, 3);
    Expect(notHeld.pitch < 0.5f, "5f a drop while not held: no shot");
    Frames(in, 90);
    g_m320Recent = true;  // (put away a moment ago: in the game's hand no more)
    m416.SetCount(13);
    recoilrun::Step(in);
    const recoil::Output afterGrenade = Frames(in, 4);
    Expect(afterGrenade.active && afterGrenade.pitch > 0.5f, "5g the grenade launcher just put away: the rifle's round kicks at once",
           "%.2f degrees", afterGrenade.pitch);
    g_m320Recent = false;
    g_key = 0;

    std::printf("-- 6. Watch: no Step for a second of pictures: put back; one picture held up a second isn't one; Step again: out again\n");
    Sleep(1100);
    recoilrun::Watch();
    Expect(RowsAre(m416.sway, 0.0f, 0.0f, 0.0f), "6a0 one picture held up for a second (a hitch, a film's burst written): the kick stays out");
    for (int i = 0; i < 30; ++i)
        recoilrun::Watch();
    Expect(Same(m416.sway, original), "6a unwatched a second of pictures: the data the game's");
    Frames(in, 2);
    Expect(RowsAre(m416.sway, 0.0f, 0.0f, 0.0f), "6b drawn again: out again");

    std::printf("-- 7. a checkpoint puts the kick back: out again at the next look\n");
    m416.sway = original;
    recoilrun::g_maintainAt = 0.0;
    size_t mark = g_log.size();
    Frames(in, 1);
    Expect(RowsAre(m416.sway, 0.0f, 0.0f, 0.0f) && Logged("was back in its data", mark), "7a taken out again, said");

    std::printf("-- 8. another gun drawn: its kick out, the M416's back at the next look\n");
    g_slot[0] = &ak;
    in.id = 21723;
    in.shape = "ak74";
    Frames(in, 4);
    Expect(RowsAre(ak.sway, 0.0f, 0.0f, 0.0f), "8a the AK-74M's out");
    recoilrun::g_maintainAt = 0.0;
    Frames(in, 1);
    Expect(Same(m416.sway, original), "8b the M416's back as the game had it (not drawn)");
    Expect(recoilrun::g_gun.row && std::strcmp(recoilrun::g_gun.row->name, "AK-74M") == 0, "8c its row the AK-74M's");

    std::printf("-- 9. data not the drawn gun's, or not gun sway: nothing written\n");
    g_slot[0] = &m416;  // the M416's data under the AK's body
    mark = g_log.size();
    Frames(in, 4);
    Expect(Same(m416.sway, original) && Logged("another gun's numbers in that slot", mark), "9a another gun's data in the slot: left alone, said");
    Expect(!recoilrun::DriftOn(recoilrun::g_gun), "9b the drift off for it");
    in.id = 12768, in.shape = nullptr;
    Frames(in, 4);
    odd.Make(0.26f, -0.3f, 0.1f, 18.0f, 2.0f, 0.1f, 750.0f, 600.0f, 31);
    Put(&odd.sway, 0x30C, 0.9f);  // a deviation scale not 1
    const FakeSway oddOriginal = odd.sway;
    g_slot[0] = &odd;
    in.id = 12769;
    Frames(in, 4);
    Expect(Same(odd.sway, oddOriginal) && Logged("a deviation scale not 1"), "9c a deviation scale not 1: left alone, said");
    nan.Make(0.26f, -0.3f, 0.1f, 18.0f, 2.0f, 0.1f, 750.0f, 600.0f, 31);
    Put(&nan.sway, recoilrun::kRecoilRows[3] + 4, std::nanf(""));
    const FakeSway nanOriginal = nan.sway;
    g_slot[0] = &nan;
    in.id = 12770;
    Frames(in, 4);
    Expect(Same(nan.sway, nanOriginal), "9d a NaN in a row: left alone");
    bare.Make(0.26f, -0.3f, 0.1f, 18.0f, 2.0f, 0.1f, 750.0f, 600.0f, 31, false);
    g_slot[0] = &bare;
    in.id = 21723, in.shape = "ak74";
    Frames(in, 4);
    Expect(recoilrun::g_gun.noSway && recoilrun::DriftOn(recoilrun::g_gun) && Logged("no gun sway of its own"), "9e no gun sway: the table's, the drift on");
    in.id = 12768, in.shape = nullptr;

    std::printf("-- 10. a write refused half way through taking a kick out\n");
    {
        // Its rows from the third on on a page that can't be written: the first two are written, then a write is refused.
        static FakeGun refusing;
        refusing.Make(0.26f, -0.3f, 0.1f, 18.0f, 2.0f, 0.1f, 750.0f, 600.0f, 31);
        uint8_t* pages = static_cast<uint8_t*>(VirtualAlloc(nullptr, 0x2000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        FakeSway* sway = pages ? reinterpret_cast<FakeSway*>(pages + 0x1000 - recoilrun::kRecoilRows[2]) : nullptr;
        DWORD was = 0;
        if (sway) {
            std::memcpy(sway, &refusing.sway, sizeof(FakeSway));
            refusing.SwayAt(sway);
        }
        const bool locked = sway && VirtualProtect(pages + 0x1000, 0x1000, PAGE_READONLY, &was);
        const FakeSway refusingOriginal = refusing.sway;
        g_slot[0] = &refusing;
        mark = g_log.size();
        Frames(in, 4);
        Expect(locked && Same(*sway, refusingOriginal), "10a the two rows written put back: the data the game's, bit for bit");
        Expect(locked && !recoilrun::DriftOn(recoilrun::g_gun) && recoilrun::g_gun.driftOff && Logged("a write refused", mark),
               "10b the drift off for it, said");
        mark = g_log.size();
        recoilrun::g_maintainAt = 0.0;
        Frames(in, 2);
        Expect(locked && Same(*sway, refusingOriginal) && !Logged("taken out again", mark) && !Logged("a write refused", mark),
               "10c the next look: never tried again, nothing said again");
        g_key = static_cast<SHORT>(0x8000);
        refusing.SetCount(30);
        Frames(in, 2);
        refusing.SetCount(29);
        recoilrun::Step(in);
        const recoil::Output still = Frames(in, 4);
        g_key = 0;
        Expect(still.active && still.pitch > 0.5f && std::fabs(still.drift[0]) < 1e-6f, "10d its kick in the hands still there, none of the drift",
               "%.2f degrees, drift %.3f", still.pitch, still.drift[0]);
        Frames(in, 90);
        g_slot[0] = &m416;
        Frames(in, 4);
        if (locked)
            VirtualProtect(pages + 0x1000, 0x1000, was, &was);
        // (the pages are kept: the table of kept guns may still name them)
    }

    std::printf("-- 11. the grenade launcher under the rifle\n");
    g_slot[2] = &m320;
    g_m320Carried = true;
    g_slot[0] = &m416b;  // (a new firing state for the same gun: set up again, the launcher with it)
    Frames(in, 4);
    g_slot[0] = &m416;
    Frames(in, 4);
    Expect(RowsAre(m320.sway, 0.0f, 0.0f, 0.0f) && RowsAre(m416.sway, 0.0f, 0.0f, 0.0f), "11a both out");
    Frames(in, 60);
    ++g_m320Fired;
    const recoil::Output m320Kick = Frames(in, 5);
    Expect(m320Kick.active && m320Kick.pitch > 2.0f, "11b its round: the 40 mm's kick (over 2 degrees)", "%.2f degrees", m320Kick.pitch);
    Frames(in, 80);
    Sleep(700);  // (its numbers step the kick for 0.6 s by the clock)
    Frames(in, 2);
    // The launcher's slot holding a gun whose sway isn't the 40 mm's.
    const FakeSway pistolOriginal = pistol.sway;
    g_slot[2] = &pistol;
    g_slot[0] = &m416b;
    mark = g_log.size();
    Frames(in, 4);
    Expect(Same(pistol.sway, pistolOriginal) && Logged("another gun's numbers in that slot", mark) && !recoilrun::DriftOn(recoilrun::g_m320),
           "11c its slot holding other data: nothing written there, said, its drift off");
    g_slot[2] = &m320;
    g_slot[0] = &m416;
    Frames(in, 4);
    Expect(RowsAre(m320.sway, 0.0f, 0.0f, 0.0f) && recoilrun::DriftOn(recoilrun::g_m320), "11d the launcher again: out again");

    std::printf("-- 12. test rounds, the log, a gun's own scale\n");
    mark = g_log.size();
    WriteRecoilTxt("test auto 5\n");
    for (int i = 0; i < 60; ++i) {
        recoilrun::Step(in);
        Sleep(5);
    }
    Expect(Logged("Recoil: test auto 5 at 750 a minute", mark), "12a the test's line");
    WriteRecoilTxt(nullptr);
    mark = g_log.size();
    WriteRecoilTxt("log 1\ngun m416 1.5\ngun no such gun 2\n");
    Frames(in, 3);
    Expect(Logged("Recoil: f ", mark) && Logged(" shots 0 hold 1.00 eye ", mark) && Logged(" look 5.730 2.865 camera 5.730 2.865", mark),
           "12b a line a frame, its fields");
    Expect(Logged("the M416 x1.5", mark) && Logged("no gun called \"no such gun\"", mark), "12c the scale said, the unknown name said");
    Expect(recoilrun::GunScaleOf(recoilrun::g_gun.row) == 1.5f, "12d the M416's scale 1.5 (by its blueprint's name, any case)");
    WriteRecoilTxt(nullptr);

    std::printf("-- 13. the eye at the ACOG\n");
    g_haveSight = true;
    Acog();
    g_sight.glassZ = g_sight.rearZ = 0.2f;  // (the glass near the game's zoomed eye: the window as it always was)
    Eyes(in, 0.0f, 0.112f, 0.148f);
    Frames(in, 20);
    Expect(recoilrun::g_atEye && recoilrun::g_state.eye > 0.9f, "13a at the eye, eased in");
    Eyes(in, 0.0f, 0.142f, 0.148f);
    Frames(in, 2);
    Expect(recoilrun::g_atEye, "13b 3 cm off the line: still (4 cm to leave)");
    Eyes(in, 0.0f, 0.162f, 0.148f);
    Frames(in, 2);
    Expect(!recoilrun::g_atEye, "13c 5 cm off: left");
    Eyes(in, 0.0f, 0.142f, 0.148f);
    Frames(in, 2);
    Expect(!recoilrun::g_atEye, "13d back to 3 cm: not again (2.5 cm to come in)");
    g_showing = true;
    Frames(in, 2);
    Expect(recoilrun::g_atEye, "13e with the scope's picture on: 3 cm is at the eye");
    g_showing = false;
    Acog();
    Eyes(in, 0.0f, LineY(0.319f), 0.319f);
    Frames(in, 20);
    Expect(recoilrun::g_atEye && recoilrun::g_state.eye > 0.9f, "13f the eye 3 cm behind the ACOG's own glass (its picture on there): at the eye");
    g_showing = true;
    Frames(in, 2);
    Expect(recoilrun::g_atEye, "13g ... with the scope's picture on");
    g_showing = false;
    Eyes(in, 0.0f, LineY(0.359f), 0.359f);
    Frames(in, 2);
    Expect(!recoilrun::g_atEye, "13h 1 cm in front of the glass: not");
    Eyes(in, 0.15f, 0.50f, 0.28f);
    Frames(in, 2);
    Expect(!recoilrun::g_atEye, "13i the gun at the hip: not");
    Frames(in, 40);

    std::printf("-- 14. the eye far off the sights' line: the kick as with no sight known\n");
    {
        struct Case {
            const char* what;
            float eye[3];
            float hands;
        };
        const Case cases[] = {
            {"14a two hands, the gun at the hip", {0.15f, 0.50f, 0.28f}, 1.0f},
            {"14b one hand, the gun at the hip", {0.15f, 0.50f, 0.28f}, 0.0f},
            {"14c two hands, pointed up from the chest", {0.15f, 0.35f, 0.45f}, 1.0f},
            {"14d one hand, pointed up from the chest", {0.15f, 0.35f, 0.45f}, 0.0f},
        };
        for (const Case& c : cases) {
            recoilrun::In at = in;
            at.hands = c.hands, at.support = c.hands > 0.5f ? 0 : -1;
            Eyes(at, c.eye[0], c.eye[1], c.eye[2]);
            Acog();
            g_haveSight = true;
            const Run with = Fire(at, m416, 10, 6, 777u);
            g_haveSight = false;
            const Run without = Fire(at, m416, 10, 6, 777u);
            Expect(with.backs == without.backs && with.pitches == without.pitches && with.forwardMost <= recoil::kReboundMost + 1e-5f &&
                       with.backMost >= 0.01f,
                   c.what, "every frame as with no sight: %s; the hand's place %+.1f mm forward at most; back %.1f cm (%.1f with no sight)",
                   with.backs == without.backs && with.pitches == without.pitches ? "yes" : "no", with.forwardMost * 1000.0f, with.backMost * 100.0f,
                   without.backMost * 100.0f);
        }
        // One hand with the eye at the sight, 2 cm behind the glass: the guard holds (the eye never nearer the glass than as held, nor 1.5 cm).
        recoilrun::In close = in;
        close.hands = 0.0f, close.support = -1;
        Acog();
        g_haveSight = true;
        Eyes(close, 0.0f, LineY(0.329f), 0.329f);
        const Run guarded = Fire(close, m416, 10, 6, 777u);
        const float limit = std::fmax(g_sight.glassZ - recoil::kEyeGlassLeast, 0.329f);
        Expect(guarded.eyeMost <= limit + 1e-4f, "14e one hand at the eye, 2 cm behind the glass: never nearer it than as held",
               "the eye at z %.4f at most in the kicked gun (%.4f)", guarded.eyeMost, limit);
        g_haveSight = false;
        Eyes(in, -0.3f, 0.3f, -0.3f);
        Frames(in, 40);
    }

    std::printf("-- 15. a switch\n");
    {
        g_key = static_cast<SHORT>(0x8000);
        m416.SetCount(30);
        Frames(in, 2);
        m416.SetCount(29);
        Frames(in, 3);
        recoilrun::In other = in;
        other.pose = 2, other.id = 21723, other.shape = "ak74";
        const recoil::Output o1 = recoilrun::Step(other);
        const recoil::Output o2 = recoilrun::Step(other);
        const recoil::Output back = recoilrun::Step(in);
        Expect(Identity(o1) && Identity(o2), "15a another gun drawn while its change waits: nothing kicked drawn on it");
        Expect(back.active && back.pitch > 0.3f, "15b the change flipped back: the kick where it was", "%.2f degrees", back.pitch);
        Frames(in, 3);
        recoilrun::In none = in;
        none.pose = 0, none.held = false;
        const recoil::Output n1 = recoilrun::Step(none);
        const recoil::Output n2 = recoilrun::Step(none);
        const recoil::Output n3 = recoilrun::Step(none);
        const recoil::Output again = recoilrun::Step(in);
        g_key = 0;
        Expect(Identity(n1) && Identity(n2) && Identity(n3) && Identity(again), "15c nothing drawn for 3 frames: gone, and the gun drawn again clear");
        Frames(in, 4);
    }

    std::printf("-- 16. nothing not finite from broken inputs\n");
    {
        recoilrun::In broken = in;
        broken.hands = std::nanf("");
        broken.pivot[1] = std::nanf("");
        broken.eyes[0][2] = std::nanf("");
        broken.dt = std::nanf("");
        broken.gap = -5.0f;
        g_key = static_cast<SHORT>(0x8000);
        m416.SetCount(20);
        Frames(broken, 2);
        m416.SetCount(18);
        const recoil::Output b = Frames(broken, 3);
        bool finite = std::isfinite(b.slack);
        for (const float v : b.k)
            finite = finite && std::isfinite(v);
        Expect(finite, "16a finite");
        g_key = 0;
    }

    std::printf("-- 17. the clears\n");
    Frames(in, 200);
    m416.SetCount(30);
    g_key = static_cast<SHORT>(0x8000);
    Frames(in, 2);
    m416.SetCount(29);
    Frames(in, 3);
    recoilrun::In gone = in;
    gone.pose = 0, gone.held = false;
    const recoil::Output away = Frames(gone, 4);
    Expect(Identity(away), "17a the gun gone: the identity");
    recoilrun::Clear();
    const recoil::Output fresh = Frames(in, 1);
    Expect(Identity(fresh), "17b a new life: the identity");
    g_key = 0;

    std::printf("-- 20. a gun taken up from the ground into slot 1, the M416 left in slot 0\n");
    {
        static FakeGun an94, stinger, other;
        an94.Make(0.3f, -0.2f, 0.25f, 16.0f, 1.5f, 0.1f, 600.0f, 900.0f, 31);
        stinger.Make(0.5f, -0.1f, 0.1f, 10.0f, 1.0f, 1.0f, 60.0f, 50.0f, 1);
        other.Make(0.31f, -0.2f, 0.3f, 15.0f, 1.4f, 0.1f, 700.0f, 880.0f, 31);
        g_slot[0] = &m416, g_slot[1] = &an94;
        g_m320Carried = false;
        recoilrun::In held = Hold(2, 16188, 1);
        held.shape = "an94";
        size_t from = g_log.size();
        Frames(held, 4);
        Expect(recoilrun::g_gun.key.slot == 1 && recoilrun::g_gun.row && std::strcmp(recoilrun::g_gun.row->name, "AN-94") == 0 &&
                   recoilrun::g_gun.firing == reinterpret_cast<uintptr_t>(&an94.firing) && Logged("the AN-94 set up from slot 1 (pose 2, body 16188)", from),
               "20a the AN-94 in slot 1: set up from its own slot, its row, the set-up line");
        Expect(RowsAre(an94.sway, 0.0f, 0.0f, 0.0f), "20b its own kick out of its own data (slot 1's)");
        g_key = static_cast<SHORT>(0x8000);
        an94.SetCount(30);
        Frames(held, 2);
        g_buzzSides.clear();
        an94.SetCount(29);
        recoilrun::Step(held);
        const recoil::Output round = Frames(held, 4);
        bool rightBuzz = false;
        for (int side : g_buzzSides)
            rightBuzz = rightBuzz || side == 1;
        Expect(round.active && round.pitch > 0.5f && rightBuzz, "20c its round (slot 1's count down): kicked, buzzed in the trigger's hand",
               "%.2f degrees, %zu buzzes", round.pitch, g_buzzSides.size());
        Frames(held, 90);
        Sleep(60);  // (a launcher's late pulse goes by the clock: any due is sent before the next look)
        Frames(held, 2);
        g_buzzSides.clear();
        m416.SetCount(m416.Count() - 1);
        const recoil::Output slot0 = Frames(held, 4);
        Expect(slot0.pitch < 0.5f && g_buzzSides.empty(), "20d slot 0's count down with the AN-94 drawn: no round");
        an94.SetCount(26);
        ++g_writes;
        recoilrun::Step(held);
        const recoil::Output written = Frames(held, 1);
        Expect(written.pitch < 0.5f, "20e a drop in slot 1 with a count written (a hand's reload): no round");
        Frames(held, 60);
        an94.SetCount(25);
        Frames(held, 2);
        Sleep(1050);
        from = g_log.size();
        Frames(held, 2);
        Expect(Logged("rounds in the last second (the AN-94, two hands", from) && Logged("; its rounds counted in slot 1", from),
               "20f the line a second: the gun's name where the film scripts read it, its slot at the end");
        Frames(held, 90);
        // The Stinger in slot 1: its count is the mod's (never a round), its missile by hands.cpp's extra shot.
        g_slot[1] = &stinger;
        recoilrun::In aerial = Hold(2, 20298, 1);
        aerial.shape = "fim92";
        Frames(aerial, 4);
        stinger.SetCount(2);
        Frames(aerial, 2);
        Sleep(60);
        Frames(aerial, 2);
        g_buzzSides.clear();
        stinger.SetCount(1);
        Frames(aerial, 4);
        Expect(recoil::Launcher(recoilrun::g_gun.gun.kind) && g_buzzSides.empty(), "20g the Stinger in slot 1: its count down is no round");
        recoilrun::In fired = aerial;
        fired.extraShots = 1;
        recoilrun::Step(fired);
        Frames(aerial, 30);
        Expect(!g_buzzSides.empty(), "20h ... its missile told by hands.cpp: buzzed", "%zu buzzes", g_buzzSides.size());
        Frames(aerial, 90);
        // A gun in slot 1 the table doesn't know (no body, no shape, its data no row's): a rifle by its data's magazine, not a missile; its rounds count.
        g_slot[1] = &other;
        recoilrun::In unknown = Hold(2, 777, 1);
        Frames(unknown, 4);
        other.SetCount(30);
        Frames(unknown, 2);
        other.SetCount(29);
        recoilrun::Step(unknown);
        const recoil::Output otherRound = Frames(unknown, 4);
        Expect(recoilrun::g_gun.gun.kind == recoil::Kind::kRifle && otherRound.active && otherRound.pitch > 0.3f,
               "20i a gun the table doesn't know in slot 1, a 31-round magazine: a rifle, its round kicks", "%.2f degrees", otherRound.pitch);
        Frames(unknown, 90);
        // The M416 again, in slot 0: as before.
        g_slot[1] = nullptr;
        Frames(in, 4);
        m416.SetCount(30);
        Frames(in, 2);
        m416.SetCount(29);
        recoilrun::Step(in);
        const recoil::Output again = Frames(in, 4);
        Expect(recoilrun::g_gun.key.slot == 0 && again.active && again.pitch > 0.5f, "20j the M416 again in slot 0: its round kicks as before",
               "%.2f degrees", again.pitch);
        g_key = 0;
        Frames(in, 200);
    }

    std::printf("-- 21. the check of the game's camera: only a climb with the rounds, by about the gun's own kick, switches the drift off\n");
    {
        auto afresh = [] {
            recoilrun::g_check = recoilrun::CheckRun{};
            recoilrun::g_gun.climbs = false;
        };
        // Each round's level as a film of the M416 gave them (2026-11-13, the look 56 degrees down, its kick out of its data): five strings of six.
        static const float kFilm[5][6] = {{0.10f, 0.01f, -0.40f, 0.02f, 0.23f, -0.03f},
                                          {0.29f, 0.29f, -0.25f, 0.04f, 0.20f, 0.09f},
                                          {0.29f, 0.26f, -0.15f, 0.20f, 0.28f, 0.15f},
                                          {0.28f, 0.17f, -0.19f, 0.19f, 0.28f, 0.10f},
                                          {0.24f, 0.22f, -0.12f, 0.14f, 0.25f, 0.06f}};
        auto shake = [](int f) { return f % 2 ? 0.4f : -0.4f; };  // (the camera off its look either way from frame to frame)
        g_slot[0] = &m416, g_slot[1] = nullptr;
        Frames(in, 4);
        afresh();
        // (a) The M416's film: its camera 2.5 degrees of yaw to the side at every shot (once enough to switch the drift off), no climb.
        size_t from = g_log.size();
        g_cameraAside = 2.5f;
        for (int s = 0; s < 5; ++s)
            String(in, m416, 6, 6, 14, [&](int r, int f) { return kFilm[s][r - 1] + shake(f); });
        Expect(recoilrun::DriftOn(recoilrun::g_gun) && recoilrun::g_check.level == 5 && recoilrun::g_check.climbing == 0 &&
                   Logged("the M416's kick is out of the game's camera: it climbs ", from) && !Logged("the drift off for", from),
               "21a the M416 as filmed (five strings of six, the camera 2.5 degrees to the side, no climb): five verdicts, the drift on, said once",
               "%d verdicts not climbing, %d climbing", recoilrun::g_check.level, recoilrun::g_check.climbing);
        // (b) The AN-94's film: two-round bursts, the second round's level 0.2 over the first's (as filmed), the camera to the side: never judged.
        {
            static FakeGun burst;
            burst.Make(0.3f, -0.2f, 0.25f, 16.0f, 1.5f, 0.1f, 600.0f, 900.0f, 31);
            g_slot[1] = &burst;
            recoilrun::In held = Hold(2, 16188, 1);
            held.shape = "an94";
            Frames(held, 4);
            from = g_log.size();
            for (int s = 0; s < 7; ++s)
                String(held, burst, 2, 2, 16, [&](int r, int f) { return (r == 1 ? 0.13f : 0.33f) + shake(f); });
            Expect(recoilrun::DriftOn(recoilrun::g_gun) && recoilrun::g_check.level == 0 && recoilrun::g_check.climbing == 0 &&
                       recoilrun::g_check.strings == 0 && !Logged("the drift off for", from),
                   "21b the AN-94 as filmed (seven bursts of two, the camera to the side): no string long enough, no verdict, the drift on");
            g_slot[1] = nullptr;
            Frames(in, 4);
        }
        // (c) The user's M416 (2026-11-13 01:03, a level look, the aim at its clamp): a string of 13 with the camera 0.08 below its look, one of 4
        // with it 0.41 above, 0.7 either way from frame to frame: no climb.
        afresh();
        from = g_log.size();
        g_cameraAside = 0.02f;
        String(in, m416, 13, 6, 14, [&](int, int f) { return -0.08f + (f % 2 ? 0.7f : -0.7f); });
        String(in, m416, 4, 6, 14, [&](int, int f) { return 0.41f + (f % 2 ? 0.7f : -0.7f); });
        String(in, m416, 1, 6, 14, [&](int, int f) { return 0.41f + (f % 2 ? 0.7f : -0.7f); });
        Expect(recoilrun::DriftOn(recoilrun::g_gun) && recoilrun::g_check.level == 1 && recoilrun::g_check.climbing == 0 && !Logged("the drift off for", from),
               "21c the user's M416 (a string of 13 and one of 4, the camera 0.41 above its look and 0.7 either way): one verdict, no climb, the drift on",
               "%d verdicts not climbing", recoilrun::g_check.level);
        // (d) Shakes: the camera 1.6 degrees above its look from the third round of a string of eight on (its own line 0.23 a round, inside the
        // M416's band), three strings running: each a shake (two neighbouring levels 1.6 apart), never judged; three sound strings after them.
        afresh();
        from = g_log.size();
        bool never = true;
        for (int s = 0; s < 3; ++s) {
            String(in, m416, 8, 6, 14, [&](int r, int f) { return (r >= 3 ? 1.6f : 0.0f) + shake(f); });
            never = never && recoilrun::g_check.climbing == 0 && recoilrun::g_check.level == 0 && recoilrun::DriftOn(recoilrun::g_gun);
        }
        for (int s = 0; s < 3; ++s)
            String(in, m416, 6, 6, 14, [&](int r, int f) { return kFilm[s][r - 1] + shake(f); });
        Expect(never && recoilrun::g_check.level == 3 && recoilrun::g_check.climbing == 0 && recoilrun::DriftOn(recoilrun::g_gun) &&
                   !Logged("the drift off for", from),
               "21d three shakes running (the camera 1.6 degrees up from a string's third round on): never judged, the drift on; three sound strings "
               "after: three verdicts, not climbing",
               "%d verdicts not climbing, %d climbing", recoilrun::g_check.level, recoilrun::g_check.climbing);
        // (e) The camera rising far faster than the gun's own 0.26 a round: not its kick. 0.7 degrees a round (judged: not climbing); and a string as
        // another film had it (2026-11-13 03:10, the M416, its kick out: thrown 1.9 and 3.6 degrees up at its fifth and sixth rounds, its own line
        // 0.63 a round: a shake, never judged), three times running.
        afresh();
        from = g_log.size();
        for (int s = 0; s < 3; ++s)
            String(in, m416, 6, 6, 14, [&](int r, int f) { return 0.7f * r + shake(f); });
        static const float kThrown[6] = {0.23f, 0.22f, 0.02f, 0.16f, 1.90f, 3.63f};
        for (int s = 0; s < 3; ++s)
            String(in, m416, 6, 6, 14, [&](int r, int f) { return kThrown[r - 1] + shake(f); });
        Expect(recoilrun::DriftOn(recoilrun::g_gun) && recoilrun::g_check.climbing == 0 && recoilrun::g_check.level == 3 && !Logged("the drift off for", from),
               "21e the camera rising 0.7 degrees a round in three strings: not the game's kick (0.26); then thrown up at a string's end as filmed in "
               "three: never judged; the drift on",
               "%d verdicts not climbing", recoilrun::g_check.level);
        // (f) The game's kick in its data: the camera 0.26 higher at every round (0.52 the first), on top of the film's levels.
        afresh();
        from = g_log.size();
        auto kicked = [&](int s) { return [&, s](int r, int f) { return 0.26f * (r + 1) + kFilm[s % 5][(r - 1) % 6] + shake(f); }; };
        String(in, m416, 6, 6, 14, kicked(0));
        const bool afterOne = recoilrun::DriftOn(recoilrun::g_gun) && recoilrun::g_check.climbing == 1;
        String(in, m416, 6, 6, 14, kicked(1));
        const bool afterTwo = recoilrun::DriftOn(recoilrun::g_gun) && recoilrun::g_check.climbing == 2;
        String(in, m416, 6, 6, 14, kicked(2));
        Expect(afterOne && afterTwo && !recoilrun::DriftOn(recoilrun::g_gun) && recoilrun::g_gun.climbs &&
                   Logged("Recoil: the weapon camera climbs 0.2", from) && Logged("the game's kick still in its data: the drift off for the M416", from),
               "21f the game's kick in its data (the camera 0.26 higher at every round of three strings of six): on after one and two, off at the third, said",
               "after one %d, after two %d, climbing %d", afterOne, afterTwo, recoilrun::g_check.climbing);
        Frames(in, 200);  // (the drift drawn before it went off settles)
        g_key = static_cast<SHORT>(0x8000);
        m416.SetCount(30);
        Frames(in, 2);
        m416.SetCount(29);
        recoilrun::Step(in);
        const recoil::Output undrifted = Frames(in, 4);
        g_key = 0;
        Expect(undrifted.active && undrifted.pitch > 0.5f && std::fabs(undrifted.drift[0]) < 1e-6f, "21g ... its kick in the hands still there, none of the drift",
               "%.2f degrees, drift %.3f", undrifted.pitch, undrifted.drift[0]);
        Frames(in, 90);
        // (h) Still climbing with the drift off: off; no longer climbing: on again at the third verdict.
        from = g_log.size();
        for (int s = 0; s < 3; ++s)
            String(in, m416, 6, 6, 14, kicked(s));
        const bool stillOff = !recoilrun::DriftOn(recoilrun::g_gun) && !Logged("the drift on again", from);
        String(in, m416, 6, 6, 14, [&](int r, int f) { return kFilm[0][r - 1] + shake(f); });
        String(in, m416, 6, 6, 14, [&](int r, int f) { return kFilm[1][r - 1] + shake(f); });
        const bool offAtTwo = !recoilrun::DriftOn(recoilrun::g_gun);
        String(in, m416, 6, 6, 14, [&](int r, int f) { return kFilm[2][r - 1] + shake(f); });
        Expect(stillOff && offAtTwo && recoilrun::DriftOn(recoilrun::g_gun) && Logged("no longer climbs with the M416's rounds", from) &&
                   Logged("the drift on again", from) && !Logged("the drift off for", from),
               "21h the kick still in: off; the camera no longer climbing: off after two verdicts, on again at the third, said");
        // (i) A long string is judged on its first ten rounds while it goes on; another gun and back: judged afresh.
        afresh();
        String(in, m416, 30, 6, 14, [&](int r, int f) { return 0.26f * (r < 11 ? r + 1 : 11) + shake(f); });
        const bool oneVerdict = recoilrun::g_check.climbing == 1 && recoilrun::DriftOn(recoilrun::g_gun);
        String(in, m416, 30, 6, 14, [&](int r, int f) { return 0.26f * (r < 11 ? r + 1 : 11) + shake(f); });
        String(in, m416, 30, 6, 14, [&](int r, int f) { return 0.26f * (r < 11 ? r + 1 : 11) + shake(f); });
        const bool offLong = !recoilrun::DriftOn(recoilrun::g_gun);
        g_slot[0] = &m416b;  // (another firing state: the gun set up again)
        Frames(in, 4);
        g_slot[0] = &m416;
        Frames(in, 4);
        Expect(oneVerdict && offLong && recoilrun::DriftOn(recoilrun::g_gun) && recoilrun::g_check.climbing == 0,
               "21i a string of 30 climbing over its first ten: one verdict; three such: off; the gun set up again: the drift on, judged afresh");
        // (j) Rounds 1.9 gaps apart are no string; rounds fired with one hand are never judged; nor a gun whose own kick would reach its cap in under
        // five rounds.
        afresh();
        String(in, m416, 12, 11, 14, [&](int r, int f) { return 0.26f * (r + 1) + shake(f); });
        bool apart = recoilrun::g_check.strings == 0 && recoilrun::g_check.climbing == 0;
        recoilrun::In oneHand = in;
        oneHand.hands = 0.0f, oneHand.support = -1;
        for (int s = 0; s < 3; ++s)
            String(oneHand, m416, 6, 6, 14, kicked(s));
        apart = apart && recoilrun::g_check.strings == 0 && recoilrun::g_check.climbing == 0 && recoilrun::g_check.level == 0 &&
                recoilrun::DriftOn(recoilrun::g_gun);
        static FakeGun heavy;
        heavy.Make(3.0f, -0.3f, 0.3f, 18.0f, 1.0f, 0.1f, 750.0f, 600.0f, 31);
        g_slot[0] = &heavy;
        recoilrun::In odd2 = in;
        odd2.id = 778;
        Frames(odd2, 4);
        for (int s = 0; s < 3; ++s)
            String(odd2, heavy, 6, 6, 14, [&](int r, int f) { return 3.0f * r + shake(f); });
        Expect(apart && RowsAre(heavy.sway, 0.0f, 0.0f, 0.0f) && recoilrun::DriftOn(recoilrun::g_gun) && recoilrun::g_check.strings == 0 &&
                   recoilrun::g_check.climbing == 0,
               "21j rounds 1.9 gaps apart: no string; three climbing strings with one hand: not judged; a gun kicked 3 degrees a round by the game (its "
               "cap in four rounds): never judged");
        g_slot[0] = &m416;
        g_cameraAside = 0.0f;
        Frames(in, 4);
        Frames(in, 200);
        // (k) recoil.txt's "log": every verdict said.
        afresh();
        WriteRecoilTxt("log 3\n");
        from = g_log.size();
        String(in, m416, 6, 6, 14, [&](int r, int f) { return kFilm[0][r - 1] + shake(f); });
        Expect(Logged("Recoil: a verdict on the M416's camera: it climbs 0.01 degrees a round over 6 rounds in 1 strings (the game's own kick for it 0.26 a round): "
                      "not climbing, 1 running",
                      from),
               "21k with recoil.txt's log on: each verdict said, its numbers");
        WriteRecoilTxt(nullptr);
        Frames(in, 200);
        // (l) recoil.txt's "game 1" with its log (a film's: the game's kick left in its data on purpose): the camera watched, each verdict said,
        // nothing switched; the file deleted: the kick out again, the drift on.
        afresh();
        WriteRecoilTxt("game 1\nlog 3\n");
        from = g_log.size();
        for (int s = 0; s < 4; ++s)
            String(in, m416, 6, 6, 14, kicked(s));
        const bool watchedOnly = Logged("a round): climbing (game 1: its kick left in on purpose, nothing switched)", from) && !recoilrun::g_gun.climbs &&
                                 recoilrun::g_check.climbing == 0 && !Logged("the drift off for", from) && Same(m416.sway, original);
        WriteRecoilTxt(nullptr);
        Frames(in, 4);
        Expect(watchedOnly && RowsAre(m416.sway, 0.0f, 0.0f, 0.0f) && recoilrun::DriftOn(recoilrun::g_gun),
               "21l game 1 with the log on (the kick left in on purpose), four climbing strings: each verdict said, nothing switched; game 0: the drift on");
        Frames(in, 200);
    }

    std::printf("-- 22. a change of gun under way (Operation Swordbreaker's AT4 kit): the slot's gun never touched under the drawn gun's name\n");
    {
        // As on the roof (build\designs\at4_measured.md, sections 3 and 11): the M4A1 drawn from slot 0; the kit puts the AT4 there (its numbers: 4 up a
        // shot, -1..1, back 4, no camera spring) while the M4A1 is drawn 0.3 s more; the AT4 drawn; the kit gives a fresh M4A1 back to slot 0 while the
        // AT4's tube is drawn half a second more; the M4A1 drawn. Each data's +0x15C 5, as the game's (never its magazine: no row by its numbers).
        static FakeGun m4a1, at4, m4a1Back;
        m4a1.Make(0.26f, -0.4f, 0.1f, 18.0f, 2.8f, 0.1f, 800.0f, 900.0f, 5);
        at4.Make(4.0f, -1.0f, 1.0f, 4.0f, 1.0f, 0.1f, 350.0f, 40.0f, 5);
        Put(&at4.sway, 0x324, 0u);  // (no CameraRecoilData)
        m4a1Back.Make(0.26f, -0.4f, 0.1f, 18.0f, 2.8f, 0.1f, 800.0f, 900.0f, 5);
        const FakeSway m4a1Original = m4a1.sway, at4Original = at4.sway, backOriginal = m4a1Back.sway;
        const recoilguns::Row* m4a1Row = recoilguns::ByName("M4A1");
        const recoilguns::Row* at4Row = recoilguns::ByName("AT4");
        g_slot[0] = &m4a1, g_slotRow[0] = m4a1Row;
        recoilrun::In rifle = Hold(2, 22194, 0);
        rifle.shape = "m4a1";
        Frames(rifle, 6);
        Expect(RowsAre(m4a1.sway, 0.0f, 0.0f, 0.0f) && recoilrun::g_gun.row == m4a1Row && !recoilrun::g_gun.otherInSlot,
               "22a the M4A1 drawn from slot 0, its name the slot's: its own kick out, its row");
        // The kit: the AT4 in slot 0 under the M4A1 still drawn.
        size_t from = g_log.size();
        g_slot[0] = &at4, g_slotRow[0] = at4Row;
        Frames(rifle, 6);
        Expect(Same(at4.sway, at4Original) && recoilrun::g_gun.otherInSlot && Logged("the M4A1 drawn while slot 0 holds the AT4", from) &&
                   !Logged("the M4A1's own kick taken out", from),
               "22b the AT4 in slot 0 with the M4A1 still drawn: its six rows as the game has them, bit for bit, never taken out under the M4A1's name; said");
        Expect(!recoilrun::DriftOn(recoilrun::g_gun), "22c ... the drift off meanwhile");
        g_key = static_cast<SHORT>(0x8000);
        at4.SetCount(1);
        Frames(rifle, 2);
        at4.SetCount(0);
        const recoil::Output noRound = Frames(rifle, 4);
        g_key = 0;
        Expect(noRound.pitch < 0.5f, "22d ... the slot's count down: no round of the M4A1's", "%.2f degrees", noRound.pitch);
        recoilrun::g_maintainAt = 0.0;
        Frames(rifle, 1);
        Expect(Same(m4a1.sway, m4a1Original), "22e the M4A1's own data put back as the game had it (no gun the mod draws has it now)");
        // The AT4 drawn: its own.
        recoilrun::In tube = Hold(2, 15987, 0);
        tube.shape = "at4";
        Frames(tube, 6);
        Expect(RowsAre(at4.sway, 0.0f, 0.0f, 0.0f) && recoilrun::g_gun.row == at4Row && !recoilrun::g_gun.otherInSlot,
               "22f the AT4 drawn: its own kick out of its own data, its row the AT4's");
        // The kit gives an M4A1 back under the AT4's tube still drawn.
        from = g_log.size();
        g_slot[0] = &m4a1Back, g_slotRow[0] = m4a1Row;
        Frames(tube, 6);
        recoilrun::g_maintainAt = 0.0;
        Frames(tube, 1);
        Expect(Same(m4a1Back.sway, backOriginal) && Logged("the AT4 drawn while slot 0 holds the M4A1", from), "22g a fresh M4A1 in slot 0 under the AT4's tube: its data untouched, said");
        Expect(Same(at4.sway, at4Original), "22h ... the AT4's data put back as the game had it");
        Frames(rifle, 6);
        Expect(RowsAre(m4a1Back.sway, 0.0f, 0.0f, 0.0f) && recoilrun::g_gun.row == m4a1Row, "22i the M4A1 drawn again: the new one's kick out, its row");
        // The slot's name read a moment late: unknown as the gun is set up (as before), known three frames on as another gun: set up again, its data
        // put back at the next look.
        static FakeGun late;
        late.Make(4.0f, -1.0f, 1.0f, 4.0f, 1.0f, 0.1f, 350.0f, 40.0f, 5);
        Put(&late.sway, 0x324, 0u);
        const FakeSway lateOriginal = late.sway;
        g_slot[0] = &late, g_slotRow[0] = nullptr;
        Frames(rifle, 6);
        g_slotRow[0] = at4Row;
        Frames(rifle, 6);
        recoilrun::g_maintainAt = 0.0;
        Frames(rifle, 1);
        Expect(Same(late.sway, lateOriginal) && recoilrun::g_gun.otherInSlot, "22j the slot's name known late as the AT4's: set up again, its rows put back as the game had them");
        // Neither known (no name read): as before, the drawn gun's.
        g_slotRow[0] = nullptr;
        g_slot[0] = &m4a1;
        Frames(rifle, 6);
        Expect(RowsAre(m4a1.sway, 0.0f, 0.0f, 0.0f) && !recoilrun::g_gun.otherInSlot, "22k no name known for the slot: the drawn gun's data, as before");
        // The M416 again, for 18 and 19.
        g_slot[0] = &m416;
        Frames(in, 6);
        Frames(in, 200);
    }

    std::printf("-- 18. unload: every kick taken out back\n");
    for (recoilrun::Saved& s : recoilrun::g_saved)
        if (s.sway && s.out)
            recoilrun::PutBack(s);
    Expect(Same(m416.sway, original) && Same(m320.sway, m320Original) && Same(ak.sway, akOriginal),
           "18a the M416's, the 40 mm's and the AK's data the game's, bit for bit");

    std::printf("-- 19. no log line the self-test would take for a failure\n");
    bool clean = true;
    for (const std::string& line : g_log) {
        std::string lower = line;
        for (char& c : lower)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (lower.find("fail") != std::string::npos || lower.find("error") != std::string::npos || lower.find("exception") != std::string::npos)
            clean = false, std::printf("  the line: %s\n", line.c_str());
    }
    Expect(clean, "19a none");
    DeleteFileW(g_file.c_str());
    RemoveDirectoryW(g_dir.c_str());
    std::printf("%d checks, %d failed\n", g_passed + g_failed, g_failed);
    return g_failed;
}

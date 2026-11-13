// The charging handle and the bolt release by either hand (src/reload.cpp, as hands.cpp runs it), on scripted frames
// at 72 Hz, without the game: reload.cpp's calls into it are stubbed here (the rifle in hand, its magazine, the rounds
// in it, the last shot, the sounds, the log) and its clock is the test's (reload::TestClock). The gun is held still in
// the view (the head's space: x right, y up, z back), turned 25/-12/8 degrees (yaw/pitch/roll) so its matrices are
// tried off their axes; each palm point is placed on the gun (its model space: x left, y up, z to the muzzle) and
// carried into the view by that pose. reload.h's numbers: the palm's point with the glove on the handle's T,
// (0.035 0.14 0.135) for the left hand and (-0.035 0.14 0.135) for the right; the bolt release at (0.03 0.01 0.37) on
// the receiver's left side for both (the gun is never mirrored). reload.cpp's: a fresh press takes the handle within
// 9 cm (trigger) or 7 cm (grip); 25 cm off, both presses under 0.4 or the palm withdrawn lets it go; drawn 4.5 cm or
// more it chambers a round; home in 0.06 s; a slap is a touch arriving on the receiver's left side (x over 5 mm)
// within 7 cm of the release with the palm moving at over 0.05 m/s or, without the gun's shape, a hand at over
// 0.7 m/s within 10 cm of it.
//   r1  the left alone against tools/reloadtest_old.inc (reload.cpp as it was, on the same clock), frame for frame:
//       HandleBack, CoverAngle, BoltBack, HandleHeld, every log line and every sound, each copy's recorded apart. One
//       script three times: through UpdateHands with no right hand, through Update, and through UpdateHands with the
//       right controller pressed meanwhile and its palm withheld (the right on the pistol grip). Its paths: a trigger
//       grab 3 cm off and a 7 cm pull; a grip grab 5 cm off; presses 10 cm (trigger) and 8 cm (grip) off, refused; a
//       trigger held as the hand comes; the grip held and the trigger's own press; let go 26 cm off, by the palm
//       withdrawn and by the gun's pose withdrawn; run dry (2 to 1 on shots), a pull with no rounds, the magazine
//       changed and a pull chambering; a slap with the shape (the touch arriving at x 12 mm, 3 cm from the release,
//       the palm at 0.1 m/s); a touch resting there as the bolt locks; shapeless passes at 0.6 m/s 5 cm off and at
//       0.8 m/s 8 cm off; shots moving the cover; weapon.txt's pull; the game's own reload; the rifle out of the hand
//       and the count unknown. Every line the script should bring is counted; and every held gun's cover (StepCover)
//       against the reference's
//   r2  the right at its own point: by its trigger 8 cm off and by its grip 6 cm off, 7 cm pulls chambering, told as
//       the right hand's; its trigger 10 cm off and its grip 8 cm off, refused
//   r3  one hand at a time: either holding, the other's press takes nothing; presses in one frame: the left's; the
//       holder judged by its own presses and palm only
//   r4  the right's slap: its touch on the receiver's left side chambers; on its right side, under it or 7.2 cm off,
//       nothing; the magazine out, nothing to chamber; a hand resting isn't a slap; nothing while the left holds the
//       charging handle; without the shape, a quick hand
//   r5  the holder's palm withdrawn (either hand): let go, falling home in 0.06 s, chambered when drawn past 4.5 cm
//   r6  a trigger (or grip) held as the right's palm appears at its point takes nothing until it's let go under 0.6
//       and pressed again
//   r7  NearHandle by side: the right's point and its reaches; the left's as before on random palms
//   r8  TestLockBack: locked back with one line and the cover open; a right slap chambers; again while back: nothing
//   r9  broken inputs (palms, touches, presses and the gun's pose not numbers, or infinite): nothing taken or slapped,
//       a hand holding the handle lets it go, every output a number; TestClock not a number: the real clock
//   r10 both hands slap in one frame: one chamber, one 'bolt'
//   r11 a held gun drawn while the rifle is still drawn lately: its count locks no bolt on the rifle; the rifle's own run dry as ever
// Usage: reloadtest [-v]   (-v prints each line as it's said: r1's from the reference's first run of the script)
// The exit code is the number of failed checks.

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <string>
#include <vector>

#include "../src/ammo.h"
#include "../src/effects.h"
#include "../src/log.h"
#include "../src/reload.h"
#include "../src/skin.h"
#include "../src/sounds.h"

namespace {

// ---- The game as reload.cpp asks it (stubbed below), the test's clock, and what each copy says ----
struct Game {
    bool rifleInHand = true;  // skin::RifleInHand
    bool heldGun = false;     // skin::HeldGunInHand: another gun drawn since the rifle (its count the gun in hand's)
    bool ammoKnown = true;    // ammo::Get finds the soldier and the gun
    bool magOut = false;      // skin::MagazineOut
    int loaded = 30, spare = 90;
};
Game g_game;
unsigned long long g_lastShot = 0;  // effects::LastShot: when the last shot was seen, 0 before the first; a new value is a new shot
double g_clock = 1000.0;            // the test's clock (s), both copies' Now(): 1/72 s a frame

struct Record {  // what one copy said: its log lines and the sounds it played, in order
    std::vector<std::string> lines, sounds;
};
Record* g_record = nullptr;  // where the stubs write now (nullptr: nowhere)
Record* g_echo = nullptr;    // with -v: lines written to this one are printed as well
bool g_verbose = false;
double g_shownFrom = 0.0;    // with -v: lines are printed with the time since this

}  // namespace

// ---- reload.cpp's calls into the game, stubbed: both copies are told the same ----
namespace skin {
bool RifleInHand() {
    return g_game.rifleInHand;
}
bool HeldGunInHand() {
    return g_game.heldGun;
}
bool MagazineOut() {
    return g_game.magOut;
}
}  // namespace skin
namespace ammo {
bool Get(int& loaded, int& spare) {
    if (!g_game.ammoKnown)
        return false;
    loaded = g_game.loaded;
    spare = g_game.spare;
    return true;
}
}  // namespace ammo
namespace effects {
ULONGLONG LastShot() {
    return g_lastShot;
}
}  // namespace effects
namespace sounds {
void Play(const char* name, float pan) {
    if (!g_record)
        return;
    std::string what = name;
    if (pan != 0.0f) {
        char text[32];
        std::snprintf(text, sizeof(text), " (pan %.2f)", pan);
        what += text;
    }
    g_record->sounds.push_back(what);
}
}  // namespace sounds
namespace logx {
void Write(const char* fmt, ...) {
    char text[1024];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);
    if (!g_record)
        return;
    g_record->lines.push_back(text);
    if (g_verbose && g_record == g_echo)
        std::printf("        %7.3f s  %s\n", g_clock - g_shownFrom, text);
}
}  // namespace logx

// The reference's clock (its Now()): the one the new copy is given by reload::TestClock.
namespace reloadtest {
double Now() {
    return g_clock;
}
}  // namespace reloadtest

#include "reloadtest_old.inc"

namespace {

namespace before = old_reload::reload;  // reload.cpp as it was

constexpr double kFrame = 1.0 / 72.0;
constexpr float kPi = 3.14159265f;
const float* const kGrip[2] = {reload::kHandleGripOnGun[0], reload::kHandleGripOnGun[1]};  // each hand's point on the T
const float* const kRelease = reload::kBoltReleaseOnGun;
// A glove's touch on the receiver's left side 3 cm from the release (x 12 mm), and a palm point below it to come from.
constexpr float kOnRelease[3] = {0.012f, 0.01f, 0.346f};
constexpr float kBelowRelease[3] = {0.09f, -0.06f, 0.33f};
constexpr float kRightOnRelease[3] = {0.015f, 0.01f, 0.36f};  // r1: the right glove resting on the release, its palm withheld

int g_checks = 0, g_failed = 0;
Record g_said;  // r2-r10: what the new copy has said since Forget

void Check(bool ok, const char* format, ...) {
    ++g_checks;
    if (!ok)
        ++g_failed;
    char text[800];
    va_list args;
    va_start(args, format);
    std::vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    std::printf("  %s %s\n", ok ? "ok  " : "FAIL", text);
    if (!ok) {  // what was said meanwhile
        for (const std::string& line : g_said.lines)
            std::printf("         said: %s\n", line.c_str());
        for (const std::string& sound : g_said.sounds)
            std::printf("         played: %s\n", sound.c_str());
    }
}

bool Same(float a, float b) {  // bit for bit
    return std::memcmp(&a, &b, sizeof(a)) == 0;
}

uint32_t g_seed = 20261106u;
float Random() {  // 0-1, the same numbers every run (xorshift)
    g_seed ^= g_seed << 13;
    g_seed ^= g_seed >> 17;
    g_seed ^= g_seed << 5;
    return static_cast<float>(g_seed >> 8) / 16777216.0f;
}

// ---- The gun in the view: its model space into the view's (rows: rotation | translation) ----
float g_gun[12];

void Multiply3(const float* a, const float* b, float* out) {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            out[i * 3 + j] = a[i * 3] * b[j] + a[i * 3 + 1] * b[3 + j] + a[i * 3 + 2] * b[6 + j];
}

void MakeGun(float yaw, float pitch, float roll, const float* at, float* pose) {  // turned about up, then right, then back
    const float y = yaw * kPi / 180.0f, p = pitch * kPi / 180.0f, r = roll * kPi / 180.0f;
    const float aboutUp[9] = {std::cos(y), 0.0f, std::sin(y), 0.0f, 1.0f, 0.0f, -std::sin(y), 0.0f, std::cos(y)};
    const float aboutRight[9] = {1.0f, 0.0f, 0.0f, 0.0f, std::cos(p), -std::sin(p), 0.0f, std::sin(p), std::cos(p)};
    const float aboutBack[9] = {std::cos(r), -std::sin(r), 0.0f, std::sin(r), std::cos(r), 0.0f, 0.0f, 0.0f, 1.0f};
    float t[9], turn[9];
    Multiply3(aboutUp, aboutRight, t);
    Multiply3(t, aboutBack, turn);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            pose[i * 4 + j] = turn[i * 3 + j];
        pose[i * 4 + 3] = at[i];
    }
}

void ToView(const float* pose, const float* p, float* out) {  // a point on the gun into the view
    float r[3];
    for (int i = 0; i < 3; ++i)
        r[i] = pose[i * 4] * p[0] + pose[i * 4 + 1] * p[1] + pose[i * 4 + 2] * p[2] + pose[i * 4 + 3];
    std::memcpy(out, r, sizeof(r));
}

void Tick() {  // the next frame's time, for both copies
    g_clock += kFrame;
    reload::TestClock(g_clock);
}

int Count(const std::vector<std::string>& lines, const char* text, bool whole) {  // lines that are text (or begin with it)
    int n = 0;
    for (const std::string& line : lines)
        n += whole ? line == text : line.compare(0, std::strlen(text), text) == 0;
    return n;
}

// ==== r1: the left alone, frame for frame against the reference ====

struct Frame {  // one frame of r1's script: the left hand as hands.cpp gives it, and the game
    bool gun = true;    // the gun's pose given (none: the launcher or a pistol in the hands)
    bool palm = true;   // the left palm given (none: not tracked, or busy)
    float at[3] = {};   // the palm on the gun
    float grip = 0.0f, trigger = 0.0f;
    bool touching = false;
    float touch[3] = {};  // where the glove touches the gun
    bool shape = true;    // the gun's shape known
    Game game;
    bool shot = false;         // a shot this frame (effects::LastShot moves on)
    bool releaseBolt = false;  // the game's own reload this frame (ReleaseBolt, before the update)
    bool setPull = false;      // weapon.txt's "pull" given this frame (TestPull, before the update)
    float pull = -1.0f;
};

struct Script {
    std::vector<Frame> frames;
    Frame f;  // the next frame, carried on from the last
    void Add(int n = 1) {
        for (int k = 0; k < n; ++k) {
            frames.push_back(f);
            f.shot = f.releaseBolt = f.setPull = false;  // a frame's events happen once
        }
    }
    void Move(const float* to, int n) {  // the palm in a straight line to there over n frames
        float from[3];
        std::memcpy(from, f.at, sizeof(from));
        for (int k = 1; k <= n; ++k) {
            for (int i = 0; i < 3; ++i)
                f.at[i] = from[i] + (to[i] - from[i]) * static_cast<float>(k) / static_cast<float>(n);
            Add();
        }
    }
    void MoveTo(const float* base, float dx, float dy, float dz, int n) {  // to base + (dx dy dz)
        const float to[3] = {base[0] + dx, base[1] + dy, base[2] + dz};
        Move(to, n);
    }
    void Shoot(int loaded, int n) {  // a shot leaving this many rounds in the gun, n frames in all
        f.game.loaded = loaded;
        f.shot = true;
        Add(n);
    }
    void NewMagazine() {  // out (none counted), then a full one in
        f.game.magOut = true;
        f.game.loaded = 0;
        Add(10);
        f.game.magOut = false;
        f.game.loaded = 30;
        Add(5);
    }
};

std::vector<Frame> R1Script() {
    Script s;
    const float* g0 = kGrip[0];
    const float away[3] = {0.15f, 0.02f, 0.10f};  // the hand off the gun's left side
    std::memcpy(s.f.at, away, sizeof(away));
    s.Add(10);
    // A trigger grab 3 cm off and a 7 cm pull, let go (the bolt home: it goes home with its sound, nothing to chamber).
    s.MoveTo(g0, 0.03f, 0.0f, 0.0f, 20);
    s.f.trigger = 1.0f;
    s.Add();
    s.MoveTo(g0, 0.03f, 0.0f, -0.075f, 15);
    s.Add(3);
    s.f.trigger = 0.3f;
    s.Add();
    s.f.trigger = 0.0f;
    s.Add(8);
    // A grip grab 5 cm off, drawn 3 cm, let go.
    s.MoveTo(g0, 0.0f, 0.05f, 0.0f, 10);
    s.f.grip = 1.0f;
    s.Add();
    s.MoveTo(g0, 0.0f, 0.05f, -0.03f, 6);
    s.f.grip = 0.0f;
    s.Add(8);
    // A trigger press 10 cm off and a grip press 8 cm off: refused.
    s.MoveTo(g0, 0.0f, 0.0f, 0.10f, 8);
    s.f.trigger = 1.0f;
    s.Add(3);
    s.f.trigger = 0.0f;
    s.Add(2);
    s.MoveTo(g0, 0.08f, 0.0f, 0.0f, 8);
    s.f.grip = 1.0f;
    s.Add(3);
    s.f.grip = 0.0f;
    s.Add(2);
    // The trigger pressed 20 cm off and held as the hand comes on to the handle: nothing, until it's let go to 0.5 and
    // pressed again.
    s.MoveTo(g0, 0.2f, 0.0f, 0.0f, 6);
    s.f.trigger = 1.0f;
    s.Add(2);
    s.MoveTo(g0, 0.01f, 0.0f, 0.0f, 12);
    s.Add(3);
    s.f.trigger = 0.5f;
    s.Add();
    s.f.trigger = 1.0f;
    s.Add();
    s.f.trigger = 0.0f;
    s.Add(8);
    // The grip pressed 20 cm off and held on to 8 cm off (the trigger's reach, not the grip's): the trigger's own press
    // takes it, the grip holds it on after the trigger's let go.
    s.MoveTo(g0, 0.2f, 0.0f, 0.0f, 6);
    s.f.grip = 1.0f;
    s.Add(2);
    s.MoveTo(g0, 0.08f, 0.0f, 0.0f, 8);
    s.Add(2);
    s.f.trigger = 1.0f;
    s.Add();
    s.f.trigger = 0.0f;
    s.Add(4);
    s.f.grip = 0.2f;
    s.Add();
    s.f.grip = 0.0f;
    s.Add(8);
    // Let go 26 cm off: taken 2 cm off, the hand going away 4 cm a frame.
    s.MoveTo(g0, 0.02f, 0.0f, 0.0f, 8);
    s.f.trigger = 1.0f;
    s.Add();
    s.MoveTo(g0, 0.30f, 0.0f, 0.0f, 7);
    s.f.trigger = 0.0f;
    s.Add(8);
    // The palm withdrawn while it holds it 5 cm back: let go (the bolt home: its sound); back with the trigger still held,
    // nothing.
    s.MoveTo(g0, 0.0f, 0.0f, 0.02f, 8);
    s.f.trigger = 1.0f;
    s.Add();
    s.MoveTo(g0, 0.0f, 0.0f, -0.03f, 10);
    s.f.palm = false;
    s.Add(3);
    s.f.palm = true;
    s.Add(3);
    s.f.trigger = 0.0f;
    s.Add(3);
    // The gun's pose withdrawn while it's held 3 cm back: let go.
    s.f.trigger = 1.0f;
    s.Add();
    s.MoveTo(g0, 0.0f, 0.0f, -0.06f, 4);
    s.f.gun = false;
    s.Add(3);
    s.f.gun = true;
    s.Add(2);
    s.f.trigger = 0.0f;
    s.Add(8);
    // Run dry (the count 2 to 1 on shots): locked back. Drawn 5 cm with the one round the gun keeps: nothing to chamber;
    // the magazine changed, drawn 5 cm: chambered.
    s.Move(away, 10);
    s.Shoot(3, 5);
    s.Shoot(2, 5);
    s.Shoot(1, 10);
    s.MoveTo(g0, 0.0f, 0.01f, 0.0f, 12);
    s.f.trigger = 1.0f;
    s.Add();
    s.MoveTo(g0, 0.0f, 0.01f, -0.05f, 10);
    s.f.trigger = 0.0f;
    s.Add(8);
    s.f.game.magOut = true;
    s.f.game.loaded = 0;
    s.Add(20);
    s.f.game.magOut = false;
    s.f.game.loaded = 30;
    s.Add(10);
    s.f.trigger = 1.0f;
    s.Add();
    s.MoveTo(g0, 0.0f, 0.01f, -0.10f, 10);
    s.f.trigger = 0.0f;
    s.Add(8);
    // A slap with the shape: locked back, a fresh magazine in; the palm coming up at 0.1 m/s, the glove's touch arriving
    // on the receiver's left side (x 12 mm) 3 cm from the release: chambered. Then resting there.
    s.Move(away, 10);
    s.Shoot(2, 5);
    s.Shoot(1, 5);
    s.NewMagazine();
    s.Move(kBelowRelease, 10);
    s.Add(2);
    for (int k = 0; k <= 10; ++k) {
        s.f.at[1] += 0.1f / 72.0f;
        if (k == 10) {
            s.f.touching = true;
            std::memcpy(s.f.touch, kOnRelease, sizeof(kOnRelease));
        }
        s.Add();
    }
    s.Add(5);
    s.f.touching = false;
    s.Add(3);
    // The glove resting on the release as the bolt locks back, and jiggling on it: no slap. Lifted off and brought back on
    // at 0.36 m/s with the one round left: nothing to chamber; again with a fresh magazine: chambered.
    s.f.touching = true;
    s.Add(10);
    s.Shoot(3, 5);
    s.Shoot(2, 5);
    s.Shoot(1, 10);
    for (int k = 0; k < 6; ++k) {
        s.f.at[2] += k % 2 ? 0.002f : -0.002f;
        s.Add();
    }
    const float rest[3] = {s.f.at[0], s.f.at[1], s.f.at[2]};
    for (int round = 0; round < 2; ++round) {
        s.f.touching = false;
        s.MoveTo(rest, 0.03f, 0.0f, 0.0f, 6);
        s.MoveTo(rest, 0.005f, 0.0f, 0.0f, 5);
        s.f.touching = true;
        s.Move(rest, 1);
        s.Add(3);
        s.f.touching = false;
        s.Add(3);
        if (round == 0)
            s.NewMagazine();
    }
    s.Move(away, 10);
    // Without the shape: locked back, a fresh magazine in; a pass along the receiver at 0.6 m/s 5 cm off (too slow), then
    // round well clear of it, a pass at 0.8 m/s 8 cm off at the nearest: chambered.
    s.Shoot(2, 5);
    s.Shoot(1, 5);
    s.NewMagazine();
    s.MoveTo(kRelease, 0.05f, 0.0f, -0.25f, 10);
    s.f.shape = false;
    for (int k = 0; k < 60; ++k) {
        s.f.at[2] += 0.6f / 72.0f;
        s.Add();
    }
    s.MoveTo(kRelease, 0.30f, 0.0f, 0.25f, 15);
    s.MoveTo(kRelease, 0.30f, 0.0f, -0.25f, 15);
    s.MoveTo(kRelease, 0.08f, 0.0f, -0.25f, 15);
    for (int k = 0; k < 50; ++k) {
        s.f.at[2] += 0.8f / 72.0f;
        s.Add();
    }
    s.f.shape = true;
    s.Move(away, 10);
    // Shots moving the cover: twelve 5 frames apart, then six on six frames running.
    for (int k = 0; k < 12; ++k)
        s.Shoot(s.f.game.loaded - 1, 5);
    for (int k = 0; k < 6; ++k)
        s.Shoot(s.f.game.loaded - 1, 1);
    s.Add(10);
    // weapon.txt's pull held at 3 cm, then none; run dry and the game's own reload; again with the bolt home: nothing said.
    s.f.setPull = true;
    s.f.pull = 0.03f;
    s.Add(10);
    s.f.setPull = true;
    s.f.pull = -1.0f;
    s.Add(8);
    s.f.game.loaded = 2;
    s.Add(2);
    s.f.game.loaded = 1;
    s.Add(3);
    s.f.releaseBolt = true;
    s.Add();
    s.f.game.loaded = 30;
    s.Add(5);
    s.f.releaseBolt = true;
    s.Add(3);
    // One round showing with the rifle out of the hand, then with the count unknown: not run dry; then 2 to 1 with the
    // rifle in hand: run dry, and the game's own reload. It ends as it began: the bolt home, nothing held.
    s.f.game.rifleInHand = false;
    s.f.game.loaded = 1;
    s.Add(10);
    s.f.game.rifleInHand = true;
    s.f.game.loaded = 30;
    s.Add(3);
    s.f.game.ammoKnown = false;
    s.f.game.loaded = 1;
    s.Add(5);
    s.f.game.ammoKnown = true;
    s.f.game.loaded = 30;
    s.Add(3);
    s.f.game.loaded = 2;
    s.Add(2);
    s.f.game.loaded = 1;
    s.Add(5);
    s.f.releaseBolt = true;
    s.Add();
    s.f.game.loaded = 30;
    s.Add(10);
    return s.frames;
}

// The three ways the script is run: through UpdateHands with no right hand; through Update; through UpdateHands with
// the right controller's grip and trigger pressed meanwhile and its glove now and then on the release, its palm
// withheld (hands.cpp's right on the pistol grip).
const char* const kWays[3] = {"through UpdateHands with no right hand", "through Update",
                              "through UpdateHands, the right controller pressed meanwhile, its palm withheld"};

struct Run {
    int frames = 0, differ = 0;
    Record before, after;  // the reference's and the new copy's, all the run
};

Run RunScript(const std::vector<Frame>& script, int way) {
    Run run;
    const double start = g_clock;
    for (size_t i = 0; i < script.size(); ++i) {
        const Frame& f = script[i];
        Tick();
        g_game = f.game;
        if (f.shot)
            g_lastShot += 75;
        float view[3];
        ToView(g_gun, f.at, view);
        const float* gun = f.gun ? g_gun : nullptr;
        const float* palm = f.palm ? view : nullptr;
        const float* touch = f.touching ? f.touch : nullptr;
        Record was, now;
        g_echo = way == 0 ? &was : nullptr;
        g_shownFrom = start;
        // The game's own calls before the frame's update, each copy's own.
        g_record = &now;
        if (f.releaseBolt)
            reload::ReleaseBolt();
        if (f.setPull)
            reload::TestPull(f.pull);
        g_record = &was;
        if (f.releaseBolt)
            before::ReleaseBolt();
        if (f.setPull)
            before::TestPull(f.pull);
        // The frame: the new copy, then the reference.
        g_record = &now;
        if (way == 1) {
            reload::Update(gun, palm, f.grip, f.trigger, touch, f.shape);
        } else {
            reload::Hand hands[2] = {{palm, f.grip, f.trigger, touch}, {}};
            if (way == 2) {
                const int k = static_cast<int>(i);
                hands[1].grip = k % 97 < 90 ? 1.0f : 0.2f;
                hands[1].trigger = k % 11 < 4 ? 1.0f : (k % 11 == 4 ? 0.5f : 0.0f);
                hands[1].touch = k % 37 < 5 ? kRightOnRelease : nullptr;
            }
            reload::UpdateHands(gun, hands, f.shape);
        }
        g_record = &was;
        before::Update(gun, palm, f.grip, f.trigger, touch, f.shape);
        g_record = nullptr;
        g_echo = nullptr;
        // The same in every way.
        const bool held = before::HandleHeld();
        const bool same = Same(reload::HandleBack(), before::HandleBack()) && Same(reload::CoverAngle(), before::CoverAngle()) &&
                          reload::BoltBack() == before::BoltBack() && reload::HandleHeld() == held &&
                          reload::HandleSide() == (held ? 0 : -1) && now.lines == was.lines && now.sounds == was.sounds;
        ++run.frames;
        if (!same && ++run.differ <= 3) {
            std::printf("    frame %zu (%.3f s): the handle %.6f, was %.6f; the cover %.6f, was %.6f; the bolt back %d, was %d; held %d "
                        "(side %d), was %d\n",
                        i, g_clock - start, reload::HandleBack(), before::HandleBack(), reload::CoverAngle(), before::CoverAngle(),
                        reload::BoltBack(), before::BoltBack(), reload::HandleHeld(), reload::HandleSide(), held);
            for (const std::string& line : now.lines)
                std::printf("      said: %s\n", line.c_str());
            for (const std::string& line : was.lines)
                std::printf("      was said: %s\n", line.c_str());
            for (const std::string& sound : now.sounds)
                std::printf("      played: %s\n", sound.c_str());
            for (const std::string& sound : was.sounds)
                std::printf("      was played: %s\n", sound.c_str());
        }
        run.before.lines.insert(run.before.lines.end(), was.lines.begin(), was.lines.end());
        run.before.sounds.insert(run.before.sounds.end(), was.sounds.begin(), was.sounds.end());
        run.after.lines.insert(run.after.lines.end(), now.lines.begin(), now.lines.end());
        run.after.sounds.insert(run.after.sounds.end(), now.sounds.begin(), now.sounds.end());
    }
    return run;
}

void R1() {
    std::printf("r1 the left alone, frame for frame against reload.cpp as it was (tools/reloadtest_old.inc)\n");
    const std::vector<Frame> script = R1Script();
    Run runs[3];
    for (int way = 0; way < 3; ++way) {
        runs[way] = RunScript(script, way);
        const Run& r = runs[way];
        Check(r.differ == 0 && r.after.lines == r.before.lines && r.after.sounds == r.before.sounds,
              "%s: %d frames, %d differ (HandleBack, CoverAngle, BoltBack, HandleHeld, the lines, the sounds); %zu lines, %zu sounds",
              kWays[way], r.frames, r.differ, r.before.lines.size(), r.before.sounds.size());
    }
    // The script did what it says: every line it should bring, in the reference's own log, and nothing else.
    struct Expected {
        const char* text;
        bool whole;  // the whole line, or how it begins
        int count;
    };
    const Expected expected[] = {
        {"Reload: left trigger pressed, the palm ", false, 10},
        {"Reload: left grip pressed, the palm ", false, 3},
        {"Reload: charging handle grabbed", true, 9},
        {"Reload: charging handle let go, pulled 70 mm", true, 1},
        {"Reload: charging handle let go, pulled 50 mm", true, 3},
        {"Reload: charging handle let go, pulled 30 mm", true, 2},
        {"Reload: charging handle let go, pulled 0 mm", true, 3},
        {"Reload: the magazine ran dry; the bolt is locked back", true, 6},
        {"Reload: charging handle pulled with no rounds to chamber", true, 1},
        {"Reload: charging handle pulled: a round is chambered", true, 1},
        {"Reload: bolt release slapped (the glove touched it) with no rounds to chamber", true, 1},
        {"Reload: bolt release slapped (the glove touched it): a round is chambered", true, 2},
        {"Reload: bolt release slapped: a round is chambered", true, 1},
        {"Reload: the charging handle held drawn (weapon.txt pull)", true, 1},
        {"Reload: no test pull", true, 1},
        {"Reload: the game's own reload chambers a round", true, 2},
    };
    const Record& said = runs[0].before;
    bool all = true;
    int counted = 0;
    for (const Expected& e : expected) {
        const int n = Count(said.lines, e.text, e.whole);
        counted += n;
        if (n != e.count) {
            all = false;
            std::printf("    \"%s\": %d, not %d\n", e.text, n, e.count);
        }
    }
    const int levers = Count(said.sounds, "lever", true), bolts = Count(said.sounds, "bolt", true);
    Check(all && counted == static_cast<int>(said.lines.size()) && levers == 6 && bolts == 6 && said.sounds.size() == 12,
          "the script's paths all taken (the reference's own log): %zu lines, each as many times as the script brings it; %d "
          "'lever', %d 'bolt'",
          said.lines.size(), levers, bolts);
    Check(runs[1].before.lines == said.lines && runs[2].before.lines == said.lines && runs[1].before.sounds == said.sounds &&
              runs[2].before.sounds == said.sounds,
          "each run of the script said the same (it ends as it begins)");
    // Every held gun's cover (StepCover, which reads the clock too) against the reference's: shots, a bolt back, a handle
    // drawn, frames of 0, 1 and 3 frames' length, quick and not.
    reload::Cover mine;
    before::Cover theirs;
    int differ = 0;
    for (int k = 0; k < 4000; ++k) {
        Tick();
        if (Random() < 0.12f)
            g_lastShot += 75;
        const bool back = (k / 170) % 3 == 1;
        const float handle = (k / 230) % 2 ? 0.075f * Random() : 0.0f;
        const float dt = k % 61 == 0 ? 0.0f : static_cast<float>(kFrame) * (k % 13 == 0 ? 3.0f : 1.0f);
        const bool quick = (k / 1000) % 2 == 1;
        const float a = reload::StepCover(mine, dt, back, handle, quick), b = before::StepCover(theirs, dt, back, handle, quick);
        if (!Same(a, b) || !Same(mine.open, theirs.open) || mine.byHandle != theirs.byHandle || mine.boltMovedAt != theirs.boltMovedAt ||
            mine.shotSeen != theirs.shotSeen)
            ++differ;
    }
    Check(differ == 0, "every held gun's cover (StepCover, quick and not) as before: 4000 frames, %d differ", differ);
}

// ==== r2-r10: the new copy alone ====

struct Side {  // one hand as a row has it
    bool palm = false;  // its palm given
    float at[3] = {};   // its palm on the gun
    float grip = 0.0f, trigger = 0.0f;
    bool touching = false;
    float touch[3] = {};  // where its glove touches the gun
};
Side g_hand[2];
bool g_shape = true;
int g_notNumbers = 0;  // frames with HandleBack or CoverAngle not a number

void StepWith(const float* gun, const reload::Hand hands[2], bool shape) {
    Tick();
    reload::UpdateHands(gun, hands, shape);
    if (!std::isfinite(reload::HandleBack()) || !std::isfinite(reload::CoverAngle()))
        ++g_notNumbers;
}

void Step(int n = 1) {
    for (int k = 0; k < n; ++k) {
        float view[2][3];
        reload::Hand hands[2];
        for (int s = 0; s < 2; ++s) {
            const Side& h = g_hand[s];
            ToView(g_gun, h.at, view[s]);
            hands[s].palm = h.palm ? view[s] : nullptr;
            hands[s].grip = h.grip;
            hands[s].trigger = h.trigger;
            hands[s].touch = h.touching ? h.touch : nullptr;
        }
        StepWith(g_gun, hands, g_shape);
    }
}

void Put(Side& hand, const float* base, float dx, float dy, float dz) {  // its palm at base + (dx dy dz) on the gun
    hand.at[0] = base[0] + dx;
    hand.at[1] = base[1] + dy;
    hand.at[2] = base[2] + dz;
}

void Glide(Side& hand, float dx, float dy, float dz, int n) {  // its palm moved by (dx dy dz) over n frames
    const float from[3] = {hand.at[0], hand.at[1], hand.at[2]};
    const float by[3] = {dx, dy, dz};
    for (int k = 1; k <= n; ++k) {
        for (int i = 0; i < 3; ++i)
            hand.at[i] = from[i] + by[i] * static_cast<float>(k) / static_cast<float>(n);
        Step();
    }
}

void Forget() {
    g_said = Record{};
}

int Said(const char* line) {
    return Count(g_said.lines, line, true);
}
int SaidStart(const char* start) {
    return Count(g_said.lines, start, false);
}
int SaidAny(const char* part) {
    int n = 0;
    for (const std::string& line : g_said.lines)
        n += line.find(part) != std::string::npos;
    return n;
}
bool Lines(std::initializer_list<const char*> want) {  // exactly these lines since Forget
    if (g_said.lines.size() != want.size())
        return false;
    size_t i = 0;
    for (const char* line : want)
        if (g_said.lines[i++] != line)
            return false;
    return true;
}
bool Sounds(std::initializer_list<const char*> want) {  // exactly these sounds since Forget
    if (g_said.sounds.size() != want.size())
        return false;
    size_t i = 0;
    for (const char* sound : want)
        if (g_said.sounds[i++] != sound)
            return false;
    return true;
}

// A row's start: the hands off, the bolt and the handle home, the cover shut, the game as at the start, nothing said.
// From here on, all the new copy says goes to g_said, between frames too (TestLockBack, ReleaseBolt).
void Fresh() {
    for (Side& hand : g_hand)
        hand = Side{};
    g_shape = true;
    g_game = Game{};
    g_record = &g_said;
    if (reload::BoltBack())
        reload::ReleaseBolt();
    Step(12);
    Forget();
    g_echo = &g_said;
    g_shownFrom = g_clock;
    if (reload::HandleSide() != -1 || reload::HandleBack() != 0.0f || reload::CoverAngle() != 0.0f || reload::BoltBack())
        Check(false, "the row's start: the handle held by %d and %.4f m back, the cover %.3f, the bolt back %d", reload::HandleSide(),
              reload::HandleBack(), reload::CoverAngle(), reload::BoltBack());
}

// A hand's palm coming up under the release at speed (m/s) for 10 frames, its glove's touch arriving at touch as it
// moves on (the arrival is the last frame stepped); LiftOff takes the touch away.
void SlapIn(int side, const float* touch, float speed) {
    Side& hand = g_hand[side];
    hand.palm = true;
    hand.touching = false;
    std::memcpy(hand.at, kBelowRelease, sizeof(hand.at));
    Step(3);
    for (int k = 0; k <= 10; ++k) {
        hand.at[1] += speed / 72.0f;
        if (k == 10) {
            hand.touching = true;
            std::memcpy(hand.touch, touch, sizeof(hand.touch));
        }
        Step();
    }
}
void LiftOff(int side) {
    g_hand[side].touching = false;
    Step(3);
}

// Without the gun's shape: a hand's palm passing along the receiver at speed (m/s), (offX, offY) off the release at the
// nearest. The frame of the pass a round was chambered in (-1: none); the shape's back after, the palm withdrawn.
int PassBy(int side, float offX, float offY, float speed) {
    Side& hand = g_hand[side];
    hand.palm = true;
    hand.touching = false;
    Put(hand, kRelease, offX, offY, -0.25f);
    Step(2);
    g_shape = false;
    int chambered = -1;
    for (int k = 0; k < 50; ++k) {
        hand.at[2] += speed / 72.0f;
        const bool wasBack = reload::BoltBack();
        Step();
        if (chambered < 0 && wasBack && !reload::BoltBack())
            chambered = k;
    }
    g_shape = true;
    hand.palm = false;
    Step(2);
    return chambered;
}

void R2() {
    std::printf("r2 the right hand at its own point on the T: grabbed, drawn, chambered\n");
    Fresh();
    Side& r = g_hand[1];
    r.palm = true;
    Put(r, kGrip[1], -0.048f, 0.064f, 0.0f);  // 8 cm off it: out to the gun's right, and up
    Step(3);
    reload::TestLockBack();
    Forget();
    r.trigger = 1.0f;
    Step();
    Check(reload::HandleSide() == 1 && reload::HandleHeld() &&
              SaidStart("Reload: right trigger pressed, the palm 80 mm from the charging handle's grip point (") == 1 &&
              Said("Reload: charging handle grabbed by the right hand") == 1 && g_said.lines.size() == 2,
          "its trigger pressed 8 cm off its point: told, and grabbed by the right hand (HandleSide %d)", reload::HandleSide());
    Glide(r, 0.0f, 0.0f, -0.075f, 15);
    Check(reload::HandleBack() == 0.07f && Sounds({"lever"}), "drawn back 7.5 cm: the handle at its stop (%.2f mm), one 'lever'",
          reload::HandleBack() * 1000.0f);
    Forget();
    r.trigger = 0.0f;
    Step();
    Check(!reload::BoltBack() && reload::HandleSide() == -1 &&
              Lines({"Reload: charging handle pulled by the right hand: a round is chambered",
                     "Reload: charging handle let go by the right hand, pulled 70 mm"}) &&
              Sounds({"bolt"}),
          "let go: a round chambered and the let go, told as the right hand's; the bolt's sound");
    Step(5);
    Check(reload::HandleBack() == 0.0f, "the handle home 5 frames on");
    // By its grip, 6 cm off.
    Put(r, kGrip[1], 0.0f, 0.06f, 0.0f);
    Step(3);
    reload::TestLockBack();
    Forget();
    r.grip = 1.0f;
    Step();
    Check(reload::HandleSide() == 1 && SaidStart("Reload: right grip pressed, the palm 60 mm from the charging handle's grip point (") == 1 &&
              Said("Reload: charging handle grabbed by the right hand") == 1,
          "its grip pressed 6 cm off: told, and grabbed by the right hand");
    Glide(r, 0.0f, 0.0f, -0.07f, 14);
    Forget();
    r.grip = 0.0f;
    Step();
    Check(!reload::BoltBack() && reload::HandleSide() == -1 &&
              Lines({"Reload: charging handle pulled by the right hand: a round is chambered",
                     "Reload: charging handle let go by the right hand, pulled 70 mm"}) &&
              Sounds({"bolt"}),
          "drawn 7 cm and let go: chambered, told as the right hand's");
    Step(6);
    // Refused: its trigger 10 cm off, its grip 8 cm off.
    Put(r, kGrip[1], 0.0f, 0.0f, 0.10f);
    Step(3);
    Forget();
    r.trigger = 1.0f;
    Step(3);
    r.trigger = 0.0f;
    Step(2);
    Put(r, kGrip[1], -0.08f, 0.0f, 0.0f);
    Step(3);
    r.grip = 1.0f;
    Step(3);
    r.grip = 0.0f;
    Step(2);
    Check(reload::HandleSide() == -1 && SaidStart("Reload: right trigger pressed, the palm 100 mm") == 1 &&
              SaidStart("Reload: right grip pressed, the palm 80 mm") == 1 && SaidAny("grabbed") == 0 && g_said.lines.size() == 2,
          "its trigger 10 cm off and its grip 8 cm off: told, refused");
}

void R3() {
    std::printf("r3 one hand at a time\n");
    Fresh();
    Side& l = g_hand[0];
    Side& r = g_hand[1];
    l.palm = r.palm = true;
    Put(l, kGrip[0], 0.02f, 0.0f, 0.0f);
    Put(r, kGrip[1], -0.02f, 0.0f, 0.0f);
    Step(3);
    l.trigger = 1.0f;
    Step();
    Forget();
    r.trigger = 1.0f;
    Step();
    Check(reload::HandleSide() == 0 && SaidStart("Reload: right trigger pressed") == 1 && SaidAny("grabbed") == 0,
          "the left holding it, the right's trigger pressed 2 cm off its point: told, nothing taken");
    for (int k = 0; k < 6; ++k) {
        l.at[2] -= 0.005f;
        r.at[2] -= 0.01f;
        Step();
    }
    Check(std::fabs(reload::HandleBack() - 0.03f) < 1e-4f, "the left drawing it 3 cm and the right's palm going 6: the handle %.2f mm back (the left's)",
          reload::HandleBack() * 1000.0f);
    Forget();
    l.trigger = 0.0f;
    Step();
    Check(reload::HandleSide() == -1 && Lines({"Reload: charging handle let go, pulled 30 mm"}),
          "the left lets its trigger go with the right's still held: let go (the holder's own presses)");
    Step(3);
    Check(reload::HandleSide() == -1, "the right's trigger held on: nothing taken (no fresh press)");
    r.trigger = 0.5f;
    Step();
    Forget();
    r.trigger = 1.0f;
    Step();
    Check(reload::HandleSide() == 1 && Said("Reload: charging handle grabbed by the right hand") == 1,
          "the right's let go to 0.5 and pressed: the right's");
    // The reverse.
    Put(l, kGrip[0], 0.0f, 0.0f, 0.0f);
    Step();
    Forget();
    l.trigger = 1.0f;
    Step();
    Check(reload::HandleSide() == 1 && SaidStart("Reload: left trigger pressed") == 1 && SaidAny("grabbed") == 0,
          "the right holding it, the left's trigger pressed on its point: told, nothing taken");
    l.trigger = r.trigger = 0.0f;
    Step(8);
    // Presses in one frame: the left's.
    Put(l, kGrip[0], 0.02f, 0.0f, 0.0f);
    Put(r, kGrip[1], -0.02f, 0.0f, 0.0f);
    Step(2);
    Forget();
    l.trigger = r.trigger = 1.0f;
    Step();
    Check(reload::HandleSide() == 0 && SaidStart("Reload: left trigger pressed") == 1 && SaidStart("Reload: right trigger pressed") == 1 &&
              Said("Reload: charging handle grabbed") == 1 && SaidAny("grabbed by the right hand") == 0,
          "both triggers pressed in one frame by their points: both told, the left takes it");
    // The right's press in the very frame the left lets go takes nothing (a grab is looked for before a let go); its next
    // press does.
    r.trigger = 0.0f;
    Step();
    Forget();
    l.trigger = 0.0f;
    r.trigger = 1.0f;
    Step();
    Check(reload::HandleSide() == -1 && Said("Reload: charging handle let go, pulled 0 mm") == 1 && SaidAny("grabbed") == 0,
          "the right pressed in the frame the left lets go: let go, nothing taken");
    r.trigger = 0.5f;
    Step();
    r.trigger = 1.0f;
    Step();
    Check(reload::HandleSide() == 1, "pressed again: the right's");
    // The holder's own palm: the other hand far off or withdrawn lets nothing go.
    Put(l, kGrip[0], 0.5f, 0.0f, 0.0f);
    Step(3);
    const bool farOff = reload::HandleSide() == 1;
    l.palm = false;
    Step(3);
    Check(farOff && reload::HandleSide() == 1, "the left's palm 50 cm off, then withdrawn: the right still holds it");
    r.trigger = 0.0f;
    Step(8);
}

void R4() {
    std::printf("r4 the right hand slaps the bolt release\n");
    Fresh();
    reload::TestLockBack();
    Forget();
    SlapIn(1, kOnRelease, 0.1f);
    Check(!reload::BoltBack() &&
              Lines({"Reload: bolt release slapped by the right hand (the glove touched it): a round is chambered"}) && Sounds({"bolt"}),
          "its touch arriving on the receiver's left side (x 12 mm, 3 cm from the release), the palm at 0.1 m/s: chambered, told as the "
          "right hand's");
    LiftOff(1);
    // On the receiver's right side, under it, 7.2 cm off: nothing.
    const float rightSide[3] = {-0.02f, 0.01f, 0.37f}, under[3] = {0.004f, 0.01f, 0.37f}, off[3] = {0.012f, 0.01f, 0.30f};
    const float* const misses[3] = {rightSide, under, off};
    const char* const missed[3] = {"on the receiver's right side (x -20 mm)", "under it (x 4 mm)", "7.2 cm from the release"};
    for (int m = 0; m < 3; ++m) {
        reload::TestLockBack();
        Forget();
        SlapIn(1, misses[m], 0.1f);
        LiftOff(1);
        Check(reload::BoltBack() && SaidAny("slapped") == 0 && g_said.sounds.empty(), "its touch arriving %s: nothing", missed[m]);
    }
    // The magazine out: nothing to chamber.
    g_game.magOut = true;
    g_game.loaded = 0;
    Forget();
    SlapIn(1, kOnRelease, 0.1f);
    LiftOff(1);
    Check(reload::BoltBack() &&
              Lines({"Reload: bolt release slapped by the right hand (the glove touched it) with no rounds to chamber"}) &&
              g_said.sounds.empty(),
          "the magazine out: nothing to chamber, the bolt stays back, no sound");
    g_game.magOut = false;
    g_game.loaded = 30;
    Step(3);
    // A hand resting: the touch arriving with the palm still isn't a slap.
    Forget();
    SlapIn(1, kOnRelease, 0.0f);
    LiftOff(1);
    Check(reload::BoltBack() && SaidAny("slapped") == 0, "its touch arriving with the palm still: no slap");
    // While the left holds the charging handle: nothing.
    Side& l = g_hand[0];
    l.palm = true;
    Put(l, kGrip[0], 0.0f, 0.0f, 0.0f);
    Step(2);
    l.trigger = 1.0f;
    Step();
    Forget();
    SlapIn(1, kOnRelease, 0.1f);
    const bool nothing = reload::BoltBack() && SaidAny("slapped") == 0 && reload::HandleSide() == 0;
    LiftOff(1);
    l.trigger = 0.0f;
    Step(8);
    l.palm = false;
    Step();
    Check(nothing, "the left holding the charging handle: the right's slap chambers nothing");
    // Without the shape: a quick hand by it.
    reload::TestLockBack();
    Forget();
    const int at = PassBy(1, 0.08f, 0.0f, 0.8f);
    Check(at >= 0 && !reload::BoltBack() && Lines({"Reload: bolt release slapped by the right hand: a round is chambered"}) &&
              Sounds({"bolt"}),
          "without the gun's shape, its palm passing 8 cm off at 0.8 m/s: chambered (the pass's frame %d), told as the right hand's", at);
}

void R5() {
    std::printf("r5 the holder's palm withdrawn\n");
    for (int side = 1; side >= 0; --side) {
        const bool right = side == 1;
        const char* const name = right ? "the right" : "the left";
        Fresh();
        Side& h = g_hand[side];
        h.palm = true;
        Put(h, kGrip[side], 0.0f, 0.01f, 0.0f);
        Step(3);
        reload::TestLockBack();
        h.trigger = 1.0f;
        Step();
        Glide(h, 0.0f, 0.0f, -0.05f, 10);
        const float drawn = reload::HandleBack();
        Forget();
        h.palm = false;  // its trigger still held
        Step();
        const bool told = right ? Lines({"Reload: charging handle pulled by the right hand: a round is chambered",
                                         "Reload: charging handle let go by the right hand, pulled 50 mm"})
                                : Lines({"Reload: charging handle pulled: a round is chambered", "Reload: charging handle let go, pulled 50 mm"});
        Check(reload::HandleSide() == -1 && !reload::BoltBack() && told && Sounds({"bolt"}) && reload::HandleBack() == drawn,
              "%s drawing it 5 cm, its palm withdrawn (the trigger still held): let go, chambered, the handle still %.2f mm back that frame",
              name, reload::HandleBack() * 1000.0f);
        float last = drawn;
        bool falling = true;
        int home = -1;
        for (int k = 1; k <= 8; ++k) {
            Step();
            const float back = reload::HandleBack();
            if (back > last || (back == last && back > 0.0f))
                falling = false;
            if (back == 0.0f && home < 0)
                home = k;
            last = back;
        }
        Check(falling && home == 5, "sprung home: nearer every frame, home %d frames on (%.1f ms: the first frame 0.06 s after)", home,
              home * 1000.0 / 72.0);
        // Drawn 4 cm only.
        h.palm = true;
        h.trigger = 0.0f;
        Put(h, kGrip[side], 0.0f, 0.01f, 0.0f);
        Step(3);
        reload::TestLockBack();
        h.trigger = 1.0f;
        Step();
        Glide(h, 0.0f, 0.0f, -0.04f, 8);
        Forget();
        h.palm = false;
        Step();
        Check(reload::HandleSide() == -1 && reload::BoltBack() &&
                  Lines({right ? "Reload: charging handle let go by the right hand, pulled 40 mm" : "Reload: charging handle let go, pulled 40 mm"}) &&
                  g_said.sounds.empty(),
              "%s drawing it 4 cm, its palm withdrawn: let go, nothing chambered, no sound", name);
        h.trigger = 0.0f;
        Step(8);
    }
}

void R6() {
    std::printf("r6 a press already held as the palm appears at the handle\n");
    Fresh();
    Side& r = g_hand[1];
    std::memcpy(r.at, kGrip[1], sizeof(r.at));
    r.trigger = 1.0f;
    Step(10);  // held with its palm withheld (busy, or not the hand at work)
    r.palm = true;
    Step(10);  // its palm appearing on its point, the trigger still held
    Check(reload::HandleSide() == -1 && g_said.lines.empty(), "its trigger held as its palm appears on its point: nothing taken, nothing told");
    r.trigger = 0.7f;
    Step(3);
    Check(reload::HandleSide() == -1, "eased to 0.7, still over 0.6: nothing");
    r.trigger = 0.55f;
    Step();
    r.trigger = 1.0f;
    Step();
    Check(reload::HandleSide() == 1 && Said("Reload: charging handle grabbed by the right hand") == 1, "let go to 0.55 and pressed: grabbed");
    r.trigger = 0.0f;
    Step(8);
    // Its grip the same way.
    r.palm = false;
    r.grip = 1.0f;
    Step(10);
    r.palm = true;
    Step(10);
    Check(reload::HandleSide() == -1, "its grip held as its palm appears: nothing taken");
    r.grip = 0.3f;
    Step();
    r.grip = 1.0f;
    Step();
    Check(reload::HandleSide() == 1, "its grip let go and pressed: grabbed");
    r.grip = 0.0f;
    Step(8);
}

void R7() {
    std::printf("r7 NearHandle by side\n");
    float v[3];
    ToView(g_gun, kGrip[1], v);
    Check(reload::NearHandle(g_gun, v, false, 1) && reload::NearHandle(g_gun, v, true, 1), "on the right's point: near for its trigger and its grip");
    // Off it along (0.6 0.8 0): to the gun's left and up.
    float at[5][3];
    const float distances[5] = {0.089f, 0.091f, 0.069f, 0.071f, 0.10f};
    for (int k = 0; k < 5; ++k) {
        const float p[3] = {kGrip[1][0] + 0.6f * distances[k], kGrip[1][1] + 0.8f * distances[k], kGrip[1][2]};
        ToView(g_gun, p, at[k]);
    }
    Check(reload::NearHandle(g_gun, at[0], false, 1) && !reload::NearHandle(g_gun, at[1], false, 1), "its trigger's reach: 8.9 cm near, 9.1 cm not");
    Check(reload::NearHandle(g_gun, at[2], true, 1) && !reload::NearHandle(g_gun, at[3], true, 1), "its grip's reach: 6.9 cm near, 7.1 cm not");
    Check(!reload::NearHandle(g_gun, at[4], false, 1) && !reload::NearHandle(g_gun, at[4], true, 1), "10 cm off: neither");
    // Its own point, not the left's: 3 cm to the gun's right of it (10 cm from the left's).
    float beyond[3];
    const float p[3] = {kGrip[1][0] - 0.03f, kGrip[1][1], kGrip[1][2]};
    ToView(g_gun, p, beyond);
    Check(reload::NearHandle(g_gun, beyond, false, 1) && !reload::NearHandle(g_gun, beyond, false, 0) && !reload::NearHandle(g_gun, beyond, false),
          "3 cm to the gun's right of the right's point: the right's, not the left's");
    // The left's as before: palms within 12 cm of its point each way, by trigger and by grip; a side out of range is the left's.
    int differ = 0, nearOnes = 0;
    for (int k = 0; k < 20000; ++k) {
        float q[3], w[3];
        for (int i = 0; i < 3; ++i)
            q[i] = kGrip[0][i] + (Random() * 2.0f - 1.0f) * 0.12f;
        ToView(g_gun, q, w);
        for (int byGrip = 0; byGrip < 2; ++byGrip) {
            const bool was = before::NearHandle(g_gun, w, byGrip != 0);
            nearOnes += was;
            if (reload::NearHandle(g_gun, w, byGrip != 0) != was || reload::NearHandle(g_gun, w, byGrip != 0, 0) != was ||
                reload::NearHandle(g_gun, w, byGrip != 0, -1) != was || reload::NearHandle(g_gun, w, byGrip != 0, 2) != was)
                ++differ;
        }
    }
    Check(differ == 0 && nearOnes > 2000 && nearOnes < 38000,
          "the left's (side 0, by default, or out of range) as before on 20000 palms by trigger and by grip: %d differ (%d near)", differ, nearOnes);
    const float nan = std::numeric_limits<float>::quiet_NaN(), broken[3] = {nan, nan, nan};
    Check(!reload::NearHandle(nullptr, v, false, 1) && !reload::NearHandle(g_gun, nullptr, true, 1) && !reload::NearHandle(g_gun, broken, false, 1) &&
              !reload::NearHandle(g_gun, broken, true, 0),
          "no gun, no palm, or a palm not a number: not near");
}

void R8() {
    std::printf("r8 TestLockBack\n");
    Fresh();
    reload::TestLockBack();
    Check(reload::BoltBack() && Lines({"Reload: the bolt locked back (a test: a free hands film round)"}), "the bolt home: locked back, one line");
    reload::TestLockBack();
    Check(reload::BoltBack() && g_said.lines.size() == 1, "again while it's back: nothing said");
    Step(3);
    Check(reload::CoverAngle() > 2.79f, "the port's cover open with the bolt back (%.2f radians)", reload::CoverAngle());
    Forget();
    SlapIn(1, kOnRelease, 0.1f);
    Check(!reload::BoltBack() &&
              Lines({"Reload: bolt release slapped by the right hand (the glove touched it): a round is chambered"}) && Sounds({"bolt"}),
          "a right slap: chambered");
    LiftOff(1);
    Step(10);
    Check(reload::CoverAngle() == 0.0f, "the cover shut again with the bolt home");
    Forget();
    reload::TestLockBack();
    Check(reload::BoltBack() && g_said.lines.size() == 1, "home and locked back again: told again");
    reload::ReleaseBolt();
}

void R9() {
    std::printf("r9 broken inputs\n");
    Fresh();
    const int notNumbers = g_notNumbers;
    const float nan = std::numeric_limits<float>::quiet_NaN(), inf = std::numeric_limits<float>::infinity();
    float at[2][3];
    ToView(g_gun, kGrip[0], at[0]);
    ToView(g_gun, kGrip[1], at[1]);
    const float nanPalm[3] = {nan, nan, nan}, partLeft[3] = {at[0][0], at[0][1], nan}, partRight[3] = {at[1][0], nan, at[1][2]};
    const float infPalm[3] = {inf, 0.0f, 0.0f}, negInf[3] = {at[1][0], at[1][1], -inf};
    float nanGun[12], farGun[12];
    for (float& x : nanGun)
        x = nan;
    std::memcpy(farGun, g_gun, sizeof(farGun));
    farGun[3] = inf;
    struct Trial {
        const float* gun;
        const float* palm[2];
    };
    const Trial trials[5] = {{g_gun, {nanPalm, nanPalm}}, {g_gun, {partLeft, partRight}}, {g_gun, {infPalm, negInf}},
                             {nanGun, {at[0], at[1]}}, {farGun, {at[0], at[1]}}};
    int taken = 0;
    for (const Trial& t : trials) {
        for (int press = 0; press < 2; ++press) {
            const float v = press ? 1.0f : 0.0f;
            const reload::Hand hands[2] = {{t.palm[0], v, v, nullptr}, {t.palm[1], v, v, nullptr}};
            StepWith(t.gun, hands, true);
        }
        taken += reload::HandleSide() >= 0;
        const reload::Hand off[2] = {};
        StepWith(t.gun, off, true);
    }
    Check(taken == 0 && g_said.lines.empty(),
          "fresh presses at both points with the palms not numbers, partly, or infinite, or the gun's pose not numbers or infinitely far: "
          "nothing told, nothing taken");
    // Presses not numbers, or infinite: no press; a real one right after counts.
    const reload::Hand nanPresses[2] = {{at[0], nan, nan, nullptr}, {at[1], inf, -inf, nullptr}};
    StepWith(g_gun, nanPresses, true);
    StepWith(g_gun, nanPresses, true);
    Check(reload::HandleSide() == -1 && g_said.lines.empty(), "presses not numbers or infinite at both points: no press");
    const reload::Hand press[2] = {{}, {at[1], 0.0f, 1.0f, nullptr}};
    StepWith(g_gun, press, true);
    Check(reload::HandleSide() == 1, "a real press right after: it counts (taken as let go before)");
    // Holding it: its palm not a number a frame lets it go as if withdrawn, the handle springing home from where it was.
    Side& r = g_hand[1];
    r.palm = true;
    std::memcpy(r.at, kGrip[1], sizeof(r.at));
    r.trigger = 1.0f;
    Step();
    Glide(r, 0.0f, 0.0f, -0.03f, 6);
    Forget();
    {
        const reload::Hand hands[2] = {{}, {nanPalm, 0.0f, 1.0f, nullptr}};
        StepWith(g_gun, hands, true);
    }
    Check(reload::HandleSide() == -1 && Lines({"Reload: charging handle let go by the right hand, pulled 30 mm"}) && reload::HandleBack() > 0.029f,
          "holding it 3 cm back, its palm not a number: let go as if withdrawn, the handle %.2f mm back that frame, springing home",
          reload::HandleBack() * 1000.0f);
    Step(8);  // its trigger still held: not taken again
    Check(reload::HandleSide() == -1 && reload::HandleBack() == 0.0f, "home, and not taken again with its trigger held on");
    r.trigger = 0.0f;
    Step(2);
    // Holding it: its presses not numbers let it go.
    r.trigger = 1.0f;
    Step();
    const bool held = reload::HandleSide() == 1;
    Forget();
    {
        float view[3];
        ToView(g_gun, r.at, view);
        const reload::Hand hands[2] = {{}, {view, nan, nan, nullptr}};
        StepWith(g_gun, hands, true);
    }
    Check(held && reload::HandleSide() == -1 && SaidStart("Reload: charging handle let go by the right hand") == 1, "holding it, its presses not numbers: let go");
    r.trigger = 0.0f;
    Step(2);
    // Holding it: the gun's pose not numbers lets it go.
    r.trigger = 1.0f;
    Step();
    const bool heldAgain = reload::HandleSide() == 1;
    Forget();
    {
        float view[3];
        ToView(g_gun, r.at, view);
        const reload::Hand hands[2] = {{}, {view, 0.0f, 1.0f, nullptr}};
        StepWith(nanGun, hands, true);
    }
    Check(heldAgain && reload::HandleSide() == -1 && SaidStart("Reload: charging handle let go by the right hand") == 1,
          "holding it, the gun's pose not numbers: let go");
    r.trigger = 0.0f;
    r.palm = false;
    Step(8);
    // Touches not numbers or infinite: no slap; a real touch after: slapped (nothing left stuck).
    reload::TestLockBack();
    const float nanTouch[3] = {nan, nan, nan}, partTouch[3] = {0.012f, nan, 0.346f}, infTouch[3] = {inf, 0.01f, 0.37f};
    const float* const touches[3] = {nanTouch, partTouch, infTouch};
    Forget();
    for (const float* touch : touches) {
        SlapIn(1, touch, 0.1f);
        LiftOff(1);
    }
    Check(reload::BoltBack() && SaidAny("slapped") == 0, "touches not numbers, partly, or infinite, arriving with the palm at 0.1 m/s: no slap");
    SlapIn(1, kOnRelease, 0.1f);
    LiftOff(1);
    Check(!reload::BoltBack() && Said("Reload: bolt release slapped by the right hand (the glove touched it): a round is chambered") == 1,
          "then a real touch arriving: slapped");
    // Without the shape, a palm not a number by the release: no slap.
    reload::TestLockBack();
    Forget();
    for (int k = 0; k < 4; ++k) {
        const reload::Hand hands[2] = {{nanPalm, 0.0f, 0.0f, nullptr}, {k % 2 ? nanPalm : infPalm, 0.0f, 0.0f, nullptr}};
        StepWith(g_gun, hands, false);
    }
    Check(reload::BoltBack() && SaidAny("slapped") == 0, "without the shape, palms not numbers or infinite: no slap");
    reload::ReleaseBolt();
    Check(g_notNumbers == notNumbers, "every frame's HandleBack and CoverAngle a number (%d not)", g_notNumbers - notNumbers);
    // TestClock: its time read (seen in a cover's step, which notes when the bolt last moved); not a number, infinite or
    // below 0: the real clock.
    reload::Cover probe;
    reload::TestClock(1234.5);
    reload::StepCover(probe, 0.0f, true, 0.0f);
    const double atTest = probe.boltMovedAt;
    reload::TestClock(std::numeric_limits<double>::quiet_NaN());
    reload::StepCover(probe, 0.0f, true, 0.0f);
    const double atNan = probe.boltMovedAt;
    reload::TestClock(std::numeric_limits<double>::infinity());
    reload::StepCover(probe, 0.0f, true, 0.0f);
    const double atInf = probe.boltMovedAt;
    reload::TestClock(-1.0);
    reload::StepCover(probe, 0.0f, true, 0.0f);
    const double atReal = probe.boltMovedAt;
    reload::TestClock(g_clock);  // the test's clock again
    Check(atTest == 1234.5 && std::isfinite(atNan) && atNan != 1234.5 && std::fabs(atInf - atNan) < 5.0 && std::fabs(atReal - atNan) < 5.0,
          "TestClock: its time read (%.1f s); not a number, infinite or below 0: the real clock (%.3f s, %.3f s, %.3f s)", atTest, atNan, atInf,
          atReal);
}

void R10() {
    std::printf("r10 both hands slap in one frame\n");
    // With the shape: both touches arriving in one frame; then the right's alone the same way (it slaps in that frame too).
    const float rightOn[3] = {0.015f, 0.02f, 0.36f};
    for (int run = 0; run < 2; ++run) {
        Fresh();
        reload::TestLockBack();
        Side& l = g_hand[0];
        Side& r = g_hand[1];
        l.palm = run == 0;
        r.palm = true;
        Put(l, kBelowRelease, 0.0f, 0.0f, 0.0f);
        Put(r, kBelowRelease, -0.02f, -0.01f, 0.03f);
        Step(3);
        Forget();
        bool backBefore = false;
        for (int k = 0; k <= 10; ++k) {
            l.at[1] += 0.1f / 72.0f;
            r.at[1] += 0.12f / 72.0f;
            if (k == 10) {
                backBefore = reload::BoltBack();
                l.touching = run == 0;
                r.touching = true;
                std::memcpy(l.touch, kOnRelease, sizeof(l.touch));
                std::memcpy(r.touch, rightOn, sizeof(r.touch));
            }
            Step();
        }
        if (run == 0)
            Check(backBefore && !reload::BoltBack() &&
                      Lines({"Reload: bolt release slapped (the glove touched it): a round is chambered"}) && Sounds({"bolt"}),
                  "both gloves' touches arriving on the release in one frame: one chamber (the left's), one 'bolt'");
        else
            Check(backBefore && !reload::BoltBack() &&
                      Lines({"Reload: bolt release slapped by the right hand (the glove touched it): a round is chambered"}),
                  "the right's alone the same way: it slaps in that same frame");
        l.touching = r.touching = false;
        Step(3);
    }
    // Without the shape: both palms passing 8 cm off at 0.8 m/s in step (the left beside it, the right above it); then the
    // right's alone the same way, chambering in the same frame of the pass.
    int frames[2] = {-1, -1};
    for (int run = 0; run < 2; ++run) {
        Fresh();
        reload::TestLockBack();
        Side& l = g_hand[0];
        Side& r = g_hand[1];
        l.palm = run == 0;
        r.palm = true;
        Put(l, kRelease, 0.08f, 0.0f, -0.25f);
        Put(r, kRelease, 0.0f, 0.08f, -0.25f);
        Step(2);
        Forget();
        g_shape = false;
        for (int k = 0; k < 50; ++k) {
            l.at[2] += 0.8f / 72.0f;
            r.at[2] += 0.8f / 72.0f;
            const bool wasBack = reload::BoltBack();
            Step();
            if (frames[run] < 0 && wasBack && !reload::BoltBack())
                frames[run] = k;
        }
        g_shape = true;
        if (run == 0)
            Check(!reload::BoltBack() && Lines({"Reload: bolt release slapped: a round is chambered"}) && Sounds({"bolt"}),
                  "without the shape, both palms passing 8 cm off at 0.8 m/s: one chamber (the left's), one 'bolt'");
        else
            Check(frames[1] >= 0 && frames[1] == frames[0] && Lines({"Reload: bolt release slapped by the right hand: a round is chambered"}),
                  "the right's alone the same way: chambered in the same frame of the pass (%d, %d)", frames[0], frames[1]);
        l.palm = r.palm = false;
        Step(2);
    }
}

void R11() {
    std::printf("r11 a held gun drawn while the rifle is still drawn lately (a gun taken up from the ground: its own slot)\n");
    Fresh();
    g_game.loaded = 20;
    Step(3);
    g_game.heldGun = true;  // the gun in hand's count is the held gun's now, at 1
    g_game.loaded = 1;
    Step(5);
    Check(!reload::BoltBack() && SaidAny("ran dry") == 0, "the held gun's 1 with the rifle drawn lately: no bolt locked back on the rifle");
    g_game.heldGun = false;  // the rifle again, its own count
    g_game.loaded = 20;
    Step(3);
    Check(!reload::BoltBack() && SaidAny("ran dry") == 0, "the rifle again with its own 20: nothing");
    g_game.loaded = 1;
    Step(2);
    Check(reload::BoltBack() && Said("Reload: the magazine ran dry; the bolt is locked back") == 1, "the rifle's own run dry: locked back, as ever");
    reload::ReleaseBolt();
    Step(2);
}

}  // namespace

int main(int argc, char** argv) {
    g_verbose = argc > 1 && std::strcmp(argv[1], "-v") == 0;
    const float at[3] = {0.12f, -0.25f, -0.45f};
    MakeGun(25.0f, -12.0f, 8.0f, at, g_gun);
    reload::TestClock(g_clock);
    std::printf("reloadtest: src/reload.cpp at 72 Hz, its calls into the game stubbed. The palm's points on the T (%.3f %.3f %.3f) and "
                "(%.3f %.3f %.3f), the bolt release at (%.2f %.2f %.2f); the gun turned 25/-12/8 degrees in the view.\n",
                kGrip[0][0], kGrip[0][1], kGrip[0][2], kGrip[1][0], kGrip[1][1], kGrip[1][2], kRelease[0], kRelease[1], kRelease[2]);
    R1();
    R2();
    R3();
    R4();
    R5();
    R6();
    R7();
    R8();
    R9();
    R10();
    R11();
    g_echo = nullptr;
    Check(g_notNumbers == 0, "r2-r10: every frame's HandleBack and CoverAngle a number (%d frames not)", g_notNumbers);
    {  // What a frame costs: both hands by the handle and the release, nothing said.
        Fresh();
        float view[2][3];
        ToView(g_gun, kGrip[0], view[0]);
        ToView(g_gun, kBelowRelease, view[1]);
        const reload::Hand hands[2] = {{view[0], 0.2f, 0.1f, nullptr}, {view[1], 0.3f, 0.0f, kOnRelease}};
        g_record = nullptr;
        LARGE_INTEGER frequency, start, end;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start);
        constexpr int kFrames = 200000;
        for (int k = 0; k < kFrames; ++k) {
            Tick();
            reload::UpdateHands(g_gun, hands, true);
        }
        QueryPerformanceCounter(&end);
        std::printf("info UpdateHands with both hands given: %.3f microseconds a frame\n",
                    static_cast<double>(end.QuadPart - start.QuadPart) * 1e6 / static_cast<double>(frequency.QuadPart) / kFrames);
    }
    std::printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed;
}

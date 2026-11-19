// A magazine in a hand drawn after a death, on every gun the hands reload: input.cpp's stance at a new soldier and its Sprinting (src/inputrules.h), the game's
// palms at rest on a held gun (src/heldpalms.h, skin.cpp's NoteHeldPalms) and the magazine's seat (src/seatrest.h's Take and Change, skin.cpp's DrawHeldPart and
// RigFor), header only, as the DLL runs them, without the game. Each check prints PASS or FAIL, then the totals; the exit code is the number failed.
//
// The owner's Operation Swordbreaker, 2026-11-19 (build\logs\bf3vr_1119_userplay_mk11.log, 00:56:40-01:05:00): the right stick held down at 00:59:18 (prone), killed at
// 00:59:34. The new soldier at 00:59:51 stood, but the mod's stance stayed prone, so input::Sprinting said the game's hands moved on its gun for the whole of that life:
// the game's palms on the Mk11 drawn at 01:00:01 were never taken at rest ("palms not yet" each second for 38 s), its magazine's seat (forgotten at the change of gun)
// was never taken again, and from 01:00:02.8 to 01:00:34.1, through every magazine change of that life, a magazine out by hand or from the belly was drawn nowhere
// ("Magazine: drawn in the left hand" not said once from 00:59:50.9 to 01:01:10.9; five let go of fell nowhere). The next life was right: in that death's camera the
// stick was flicked down (01:02:05, "crouched, from prone"), so its soldier wasn't held prone.
//   m1  the session replayed frame by frame at 72 Hz with the rules as they are: in every life every frame with a magazine in a hand draws it, from the first frame
//       of each pull and each fresh one from the belly (the Mk11's after the first death above all)
//   m2  ... with the rules as they were (copied below, as of 7e99b44): the replay is the log's. The Mk11's palms never known in the life after the first death and
//       none of its magazine's frames drawn there; the windows of 10 s with the magazine drawn are exactly the log's "drawn in the left hand" lines; each draw's
//       palms known as long after it as the log says (1.4 to 1.6 s); the M4A1's first pull at 01:01:10.4 and the Mk11's at 01:02:29.4 drawn only once they were
//   m3  a new soldier stands: the stance kept prone through the first death and crouched through the second and third (flicked in their cameras) is standing at
//       each new soldier and said once, the game's palms taken at rest 1.5 s after each change of gun in that life (at once for the same gun drawn again); one
//       soldier's stance is never undone by itself
//   m4  every gun the hands reload a magazine of (skin.cpp's kHeldKinds with a HeldMagBones: 13 guns): drawn the first time prone, or with the sprint held, a magazine
//       pulled a second on is drawn from its first frame (its seat from the magazine at rest on the frame); a pull 0.8 s after a first draw standing too; a gun
//       drawn again after another (prone) or after a death has its own seat from its first frame; thirteen guns in turn, each pull drawn with its own gun's seat
//   m5  what was right stays so: bare arms' seat as it was; the game's own reload (its palms known and moving) takes none; with gloves the magazine at rest never
//       replaces a seat known, and one moving on the frame never gives one; the game's palms' rule, the stance's blend, a new soldier's rule, the seats kept;
//       while the stance or a sprint moves the game's hands the palms give no seat (a seat known kept, one not known taken at rest)
//   m6  the review of 2026-11-19: the Mk11's palms known standing, then the game's own reload (X held 0.6 s) while the stance or a sprint moves the game's hands
//       (prone; a sprint's blend), so their moving is never told, cut by a change of gun or a death 0.6 s in; the Mk11 drawn again (prone still, or the new
//       soldier standing, or after the sprint) and its magazine pulled 0.2 s on. With the branch's rule before the review (a359c3c, copied below) the palms took
//       the magazine on its way as the seat each frame, and that seat was kept: drawn 7 to 12 cm off; as they are, drawn by its own seat from the first frame
// Usage: magseattest. Reads and writes nothing.

#include "../src/heldpalms.h"
#include "../src/inputrules.h"
#include "../src/seatrest.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

int g_checks = 0, g_failed = 0;
void Check(bool ok, const char* what) {
    ++g_checks;
    g_failed += !ok;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
}

// "hh:mm:ss.mmm" of the log: its ms of the day.
uint64_t At(const char* t) {
    int h = 0, m = 0, s = 0, ms = 0;
    std::sscanf(t, "%d:%d:%d.%d", &h, &m, &s, &ms);
    return ((static_cast<uint64_t>(h) * 60 + m) * 60 + s) * 1000 + ms;
}
std::string Clock(uint64_t ms) {
    char b[32];
    std::snprintf(b, sizeof(b), "%02d:%02d:%02d.%03d", static_cast<int>(ms / 3600000), static_cast<int>(ms / 60000 % 60), static_cast<int>(ms / 1000 % 60),
                  static_cast<int>(ms % 1000));
    return b;
}

// ---- The rules as they were before 2026-11-19 (input.cpp's UpdateStance and skin.cpp's RigFor and DrawHeldPart at 7e99b44), copied: m2 replays the log with them ----
namespace before {
// The stance as the stick last put it, whatever soldier it was put for.
bool StanceForSoldier(unsigned& seen, unsigned soldier, inputrules::Stance&, bool&) {
    seen = soldier;
    return false;
}
// A change of gun forgot the seats.
void Change(seatrest::Kept&, seatrest::Seats& now, uint32_t, uint32_t) {
    now = seatrest::Seats{};
}
// With the game's palms known, taken whenever they rest on the gun; without them only with bare arms, from the magazine at rest on the frame. (Whether the
// stance or a sprint moved the game's hands wasn't asked.)
seatrest::Took Take(seatrest::Seats& s, int i, seatrest::Watch& watch, const float* on, bool palmsKnown, bool gameMoving, bool, bool bareArms, uint64_t now) {
    if (palmsKnown) {
        if (gameMoving)
            return seatrest::Took::kNone;
        std::memcpy(s.seat[i], on, sizeof(s.seat[i]));
        s.have[i] = true;
        return seatrest::Took::kByPalms;
    }
    if (!bareArms || !seatrest::Rests(watch, on, now))
        return seatrest::Took::kNone;
    std::memcpy(s.seat[i], on, sizeof(s.seat[i]));
    s.have[i] = true;
    return seatrest::Took::kAtRest;
}
}  // namespace before

// ---- The seat's rule of a359c3c, before its review (the stance as it is, the seats kept), copied: m6 shows the review's case with it ----
namespace branch {
// With the game's palms known, taken whenever they aren't seen moving, whatever moved the game's hands; without them at rest, with gloves only while none is known.
seatrest::Took Take(seatrest::Seats& s, int i, seatrest::Watch& watch, const float* on, bool palmsKnown, bool gameMoving, bool, bool bareArms, uint64_t now) {
    if (palmsKnown) {
        if (gameMoving)
            return seatrest::Took::kNone;
        std::memcpy(s.seat[i], on, sizeof(s.seat[i]));
        s.have[i] = true;
        return seatrest::Took::kByPalms;
    }
    const bool rests = seatrest::Rests(watch, on, now);
    if (!seatrest::Learn(false, false, bareArms, rests, s.have[i]))
        return seatrest::Took::kNone;
    std::memcpy(s.seat[i], on, sizeof(s.seat[i]));
    s.have[i] = true;
    return seatrest::Took::kAtRest;
}
}  // namespace branch

struct Rules {
    const char* name;
    bool (*stanceForSoldier)(unsigned&, unsigned, inputrules::Stance&, bool&);
    void (*change)(seatrest::Kept&, seatrest::Seats&, uint32_t, uint32_t);
    // (the draw's seat i, its watch, its bone on the frame, the palms known, moving, the stance or a sprint moving the game's hands, bare arms, the time)
    seatrest::Took (*take)(seatrest::Seats&, int, seatrest::Watch&, const float*, bool, bool, bool, bool, uint64_t);
};
const Rules kNow{"as they are", inputrules::StanceForSoldier, seatrest::Change, seatrest::Take};
const Rules kBefore{"as they were", before::StanceForSoldier, before::Change, before::Take};
const Rules kBranch{"as on the branch before its review", inputrules::StanceForSoldier, seatrest::Change, branch::Take};

// ---- The guns: skin.cpp's kHeldKinds with a magazine by hand (their colour pass's index counts), each its own seat on its frame and its palms ----
struct Gun {
    uint32_t kind;
    const char* name;
};
constexpr Gun kGuns[] = {
    {24492, "RPK-74M"}, {16188, "AN-94"}, {21585, "AEK-971"}, {21723, "AK-74M"}, {23064, "G3A3"}, {15237, "KH2002"}, {17013, "M16A4"},
    {22194, "M4A1"},   {27588, "M240"},  {17991, "AKS-74u"}, {19314, "UMP-45"}, {12456, "Mk11 Mod 0"}, {24291, "M249"},
};
constexpr uint32_t kM4a1 = 22194, kMk11 = 12456;
const char* NameOf(uint32_t kind) {
    for (const Gun& g : kGuns)
        if (g.kind == kind)
            return g.name;
    return "?";
}
// The magazine's bone of a seat draw on the gun's frame, seated: the game's own, rigid with the frame whatever the game does with the gun (but its own reload):
// each gun and each seat its own (the colour pass's, the rounds', the depth pass's two).
void SeatOn(uint32_t kind, int i, float* on) {
    const float a = 0.01f * static_cast<float>(kind % 97) + 0.1f * static_cast<float>(i);
    const float c = std::cos(a), s = std::sin(a);
    const float m[12] = {1, 0, 0, 0.0f, 0, c, -s, -0.03f - 0.01f * static_cast<float>(i) - 0.0001f * static_cast<float>(kind % 89), 0, s, c, 0.40f + 0.001f * static_cast<float>(kind % 61)};
    std::memcpy(on, m, sizeof(m));
}
// Where the game's palms rest on it (the log's for the M4A1 and the Mk11, the others about there).
void PalmsAtRest(uint32_t kind, float* right, float* left) {
    const float r[3] = {-0.054f, -0.114f, kind == kMk11 ? 0.254f : 0.255f}, l[3] = {0.012f, -0.041f, kind == kMk11 ? 0.611f : 0.612f};
    std::memcpy(right, r, sizeof(r));
    std::memcpy(left, l, sizeof(l));
    if (kind != kM4a1 && kind != kMk11)
        right[2] += 0.0001f * static_cast<float>(kind % 50), left[2] += 0.0002f * static_cast<float>(kind % 40);
}

// ---- The world the rules see, a frame at a time ----
constexpr uint64_t kRaiseMs = 550;      // the game's raise after its switch: its hands come onto the gun (each draw's palms known 1.4-1.6 s on, in the log)
constexpr uint64_t kGameBlendMs = 1000;  // the game's hands move on its gun this long after its stance changes or its sprint
constexpr double kFrameMs = 1000.0 / 72.0;
using inputrules::Stance;

struct World {
    const Rules* rules = &kNow;
    // The game's own soldier.
    unsigned soldier = 0;              // god::SoldierSerial
    Stance gameStance = Stance::kStand;
    uint64_t gameStanceAt = 0, sprintAt = 0;
    uint32_t kind = 0;                 // the gun the game draws (0: none)
    bool drawn = false, stowed = false;  // drawn at all; put away (drawn nowhere by the mod)
    uint64_t raiseAt = 0;              // its last raise (the game's switch to it)
    bool raising = false;
    bool mag = false;                  // a magazine in a hand
    bool bare = false;                 // bare arms drawn (Semper Fidelis's levels)
    bool gameReload = false;           // the game's own reload plays: its magazine moves on the frame, its hands off their rest
    // input.cpp's.
    Stance stance = Stance::kStand;
    bool crouchOn = false, prone = false;
    uint64_t stanceChangedAt = 0;
    unsigned stanceSoldier = 0;
    int stanceSaid = 0;
    // skin.cpp's.
    uint32_t rigKind = 0;
    heldpalms::State palms;
    seatrest::Seats seats;
    seatrest::Kept kept;
    seatrest::Watch watch[2 * seatrest::kSeats];
    // What was seen.
    uint64_t palmsKnownAt = 0;         // when the palms were taken for the gun drawn (0: not since its draw)
    uint64_t drawAt = 0;               // its draw into a hand
    long magFrames = 0, magDrawn = 0;  // frames with a magazine in a hand; of them drawn whole (every seat draw of both eyes)
    uint64_t firstMiss = 0, lastMiss = 0;
    long misses = 0;
    bool seatWrong = false;            // a magazine drawn with a seat not its gun's
    bool tookAtRest = false;
};

void SetStance(World& w, Stance s, uint64_t now) {  // input.cpp's SetStance
    if (s != w.stance)
        w.stanceChangedAt = now;
    w.stance = s;
    w.prone = s == Stance::kProne;
}
void GameStance(World& w, Stance s, uint64_t now) {
    if (s != w.gameStance)
        w.gameStanceAt = now;
    w.gameStance = s;
}

// The game's palms on the gun's frame now: at rest; coming onto it in a raise; moving on it prone, sprinting, or in a change of stance, or in its own reload.
void GamePalms(const World& w, uint64_t now, float* right, float* left) {
    PalmsAtRest(w.kind, right, left);
    float off = 0.0f;
    if (w.raising && now - w.raiseAt < kRaiseMs)
        off += 0.10f * (1.0f - static_cast<float>(now - w.raiseAt) / static_cast<float>(kRaiseMs));
    if (w.gameStance == Stance::kProne || (w.gameStanceAt && now - w.gameStanceAt < kGameBlendMs) || (w.sprintAt && now - w.sprintAt < kGameBlendMs) || w.gameReload)
        off += 0.04f + 0.04f * std::sin(6.2831853f * static_cast<float>(now % 1250) / 1250.0f);
    right[1] -= off, left[1] -= off;
    left[2] -= 0.5f * off;
}

// One frame: input.cpp's Sync (its stance), then skin.cpp's draws of the gun (RigFor, the gloves' palms, each eye's four seat draws).
void Frame(World& w, uint64_t now) {
    if (w.rules->stanceForSoldier(w.stanceSoldier, w.soldier, w.stance, w.crouchOn)) {
        w.prone = false;
        ++w.stanceSaid;
    }
    if (w.sprintAt && now - w.sprintAt < 200) {  // UpdateStance: a sprint stands the soldier up
        w.crouchOn = false;
        SetStance(w, Stance::kStand, now);
    }
    const bool sprinting = inputrules::HandsMovedByStance(now, w.sprintAt, w.prone, w.stanceChangedAt);
    if (!w.kind || !w.drawn)
        return;
    if (w.kind != w.rigKind) {  // RigFor
        w.rules->change(w.kept, w.seats, w.rigKind, w.kind);
        w.rigKind = w.kind;
        heldpalms::Forget(w.palms);
        for (seatrest::Watch& watch : w.watch)
            watch = seatrest::Watch{};
    }
    if (!w.stowed) {  // (put away, the gun's frame isn't taken: no palms are learned, no seat; one in a hand is drawn by the seat known)
        float right[3], left[3];
        GamePalms(w, now, right, left);
        const bool was = w.palms.known;
        heldpalms::Note(w.palms, right, left, now, sprinting || w.bare);  // (bare arms: no gloves' draw, the palms never noted)
        if (w.bare)
            heldpalms::Forget(w.palms);
        if (!was && w.palms.known && !w.palmsKnownAt)
            w.palmsKnownAt = now;
    }
    bool whole = true, right = true;
    for (int eye = 0; eye < 2; ++eye)
        for (int i = 0; i < seatrest::kSeats; ++i) {
            float on[12];
            SeatOn(w.kind, i, on);
            if (w.gameReload) {  // its own reload: the magazine on its way out and back, never still
                const float t = static_cast<float>(now % 2000) / 2000.0f;
                on[7] -= 0.15f * std::sin(3.14159265f * t);
                on[11] += 0.03f * t;
            }
            if (!w.stowed) {
                const seatrest::Took took = w.rules->take(w.seats, i, w.watch[i == 1 ? seatrest::kSeats + 1 : i], on, w.palms.known, w.palms.moving, sprinting, w.bare, now);
                w.tookAtRest = w.tookAtRest || took == seatrest::Took::kAtRest;
            }
            if (w.mag) {
                whole = whole && w.seats.have[i];
                float want[12];
                SeatOn(w.kind, i, want);
                if (w.seats.have[i] && std::memcmp(want, w.seats.seat[i], sizeof(want)) != 0)
                    right = false;
            }
        }
    if (!w.mag)
        return;
    ++w.magFrames;
    if (whole)
        ++w.magDrawn;
    else {
        if (!w.misses)
            w.firstMiss = now;
        w.lastMiss = now;
        ++w.misses;
    }
    if (!right)
        w.seatWrong = true;
}

// Frames from one time to another (from included, to not).
void Run(World& w, uint64_t from, uint64_t to) {
    for (double t = static_cast<double>(from); t < static_cast<double>(to); t += kFrameMs)
        Frame(w, static_cast<uint64_t>(t));
}

// ---- The owner's session (the log's lines; what wasn't said, a stand-up the next flick's "from standing" shows, at a time between) ----
enum class Ev : uint8_t {
    kSoldier,  // a new soldier (God mode: "the server's soldier is at ...")
    kSwitch,   // the game's switch to this gun into a hand (a key, a pick, a new soldier's first): its raise
    kDrawn,    // into a hand from its place with no switch of the game's (the same gun)
    kAway,     // put away at a shoulder (drawn nowhere)
    kNoGun,    // nothing drawn (a death's camera over, the level reloading)
    kStick,    // the right stick's stance (Input: the right stick ...), the game's soldier with it
    kSprint,   // a sprint or a jump (not said): stood up
    kMagOut,   // a magazine into a hand: pulled out of the gun, or a fresh one at the belly
    kMagIn,    // out of the hand: home, put back at the belly, let go of
};
struct Event {
    const char* at;
    Ev ev;
    uint32_t arg;  // a gun; a stance; a soldier's serial
};
constexpr uint32_t kStand = 0, kCrouch = 1, kProne = 2;
constexpr Event kSession[] = {
    {"00:56:40.000", Ev::kSoldier, 1},  // (the 22:20 launch's soldier: the headset on at 00:56:40)
    {"00:56:49.709", Ev::kSwitch, kM4a1},
    {"00:57:13.562", Ev::kAway, 0},
    {"00:57:14.938", Ev::kSwitch, kMk11},  // key 2
    {"00:57:27.990", Ev::kAway, 0},
    {"00:57:32.071", Ev::kSwitch, kM4a1},  // key 1
    {"00:57:48.427", Ev::kStick, kCrouch},
    {"00:57:56.992", Ev::kMagOut, 0},  // ejected by its button (falling loose), a fresh one at the belly
    {"00:57:57.666", Ev::kMagIn, 0},   // rocked home
    {"00:58:20.000", Ev::kSprint, 0},
    {"00:58:26.174", Ev::kAway, 0},
    {"00:58:28.368", Ev::kSwitch, kMk11},
    {"00:58:34.232", Ev::kStick, kCrouch},
    {"00:58:40.000", Ev::kSprint, 0},
    {"00:58:50.380", Ev::kMagOut, 0},  // pulled out by hand
    {"00:58:51.813", Ev::kMagIn, 0},
    {"00:58:56.188", Ev::kStick, kCrouch},
    {"00:58:56.600", Ev::kSprint, 0},
    {"00:58:57.038", Ev::kStick, kCrouch},
    {"00:59:18.307", Ev::kStick, kProne},  // held down: prone, from crouched
    // 00:59:34.6 killed; the death's camera draws the Mk11 on in the hands
    {"00:59:40.342", Ev::kMagOut, 0},
    {"00:59:41.514", Ev::kMagIn, 0},
    {"00:59:42.152", Ev::kNoGun, 0},
    {"00:59:51.552", Ev::kSoldier, 2},
    {"00:59:51.832", Ev::kSwitch, kM4a1},
    {"00:59:55.983", Ev::kAway, 0},
    {"01:00:01.441", Ev::kSwitch, kMk11},  // key 2
    {"01:00:02.848", Ev::kMagOut, 0},      // pulled out by hand (11 back)
    {"01:00:03.655", Ev::kMagIn, 0},       // rocked home
    {"01:00:03.809", Ev::kMagOut, 0},      // pulled out
    {"01:00:04.673", Ev::kMagIn, 0},       // put back at the belly
    {"01:00:04.881", Ev::kMagOut, 0},      // a fresh one at the belly
    {"01:00:05.659", Ev::kMagIn, 0},       // let go of (fell nowhere)
    {"01:00:09.229", Ev::kMagOut, 0},
    {"01:00:09.646", Ev::kMagIn, 0},
    {"01:00:09.812", Ev::kMagOut, 0},
    {"01:00:10.271", Ev::kMagIn, 0},
    {"01:00:10.438", Ev::kMagOut, 0},
    {"01:00:11.187", Ev::kMagIn, 0},
    {"01:00:11.409", Ev::kMagOut, 0},
    {"01:00:12.048", Ev::kMagIn, 0},
    {"01:00:14.746", Ev::kMagOut, 0},
    {"01:00:15.308", Ev::kMagIn, 0},
    {"01:00:15.489", Ev::kMagOut, 0},
    {"01:00:16.074", Ev::kMagIn, 0},
    {"01:00:18.061", Ev::kAway, 0},
    {"01:00:19.506", Ev::kDrawn, kMk11},  // drawn from the left shoulder, no key
    {"01:00:20.812", Ev::kMagOut, 0},
    {"01:00:21.243", Ev::kMagIn, 0},
    {"01:00:21.410", Ev::kMagOut, 0},
    {"01:00:21.882", Ev::kMagIn, 0},
    {"01:00:23.479", Ev::kMagOut, 0},
    {"01:00:23.838", Ev::kMagIn, 0},
    {"01:00:25.548", Ev::kMagOut, 0},
    {"01:00:26.493", Ev::kMagIn, 0},
    {"01:00:26.771", Ev::kMagOut, 0},
    {"01:00:27.326", Ev::kMagIn, 0},
    {"01:00:28.451", Ev::kMagOut, 0},
    {"01:00:29.048", Ev::kMagIn, 0},
    {"01:00:29.938", Ev::kMagOut, 0},
    {"01:00:30.340", Ev::kMagIn, 0},
    {"01:00:31.701", Ev::kMagOut, 0},
    {"01:00:32.909", Ev::kMagIn, 0},
    {"01:00:33.174", Ev::kMagOut, 0},
    {"01:00:34.118", Ev::kMagIn, 0},
    {"01:00:39.898", Ev::kAway, 0},
    {"01:00:45.000", Ev::kSprint, 0},  // (between the Mk11's last "palms not yet" and 01:00:53's "from standing")
    {"01:00:53.282", Ev::kStick, kCrouch},
    {"01:01:07.530", Ev::kStick, kStand},
    {"01:01:09.228", Ev::kSwitch, kM4a1},  // key 1
    {"01:01:10.362", Ev::kMagOut, 0},
    {"01:01:11.519", Ev::kMagIn, 0},
    {"01:01:12.265", Ev::kMagOut, 0},
    {"01:01:15.180", Ev::kStick, kCrouch},
    {"01:01:21.290", Ev::kMagIn, 0},
    {"01:01:22.392", Ev::kMagOut, 0},
    {"01:01:23.349", Ev::kMagIn, 0},
    {"01:01:52.744", Ev::kStick, kProne},
    // 01:02:04.7 killed (prone); in its camera the stick flicked down
    {"01:02:05.197", Ev::kStick, kCrouch},
    {"01:02:12.256", Ev::kNoGun, 0},
    {"01:02:21.592", Ev::kSoldier, 3},
    {"01:02:21.841", Ev::kSwitch, kM4a1},
    {"01:02:26.742", Ev::kAway, 0},
    {"01:02:27.995", Ev::kSwitch, kMk11},  // key 2
    {"01:02:29.415", Ev::kMagOut, 0},
    {"01:02:35.765", Ev::kMagIn, 0},
    {"01:02:49.511", Ev::kMagOut, 0},  // run dry: pulled out (0 back)
    {"01:02:50.433", Ev::kMagIn, 0},   // let go of, falling (loose)
    {"01:02:50.993", Ev::kMagOut, 0},  // a fresh one at the belly
    {"01:02:51.812", Ev::kMagIn, 0},
    {"01:02:52.500", Ev::kSprint, 0},
    {"01:02:54.176", Ev::kStick, kCrouch},
    {"01:03:05.000", Ev::kSprint, 0},
    {"01:03:11.292", Ev::kStick, kCrouch},
    {"01:03:14.838", Ev::kStick, kStand},
    {"01:03:17.824", Ev::kStick, kCrouch},
    {"01:03:32.418", Ev::kStick, kProne},
    {"01:04:09.000", Ev::kAway, 0},
    {"01:04:15.000", Ev::kSprint, 0},
    {"01:04:24.402", Ev::kSwitch, kM4a1},  // X held: the game's pick, into the right hand
    {"01:04:33.687", Ev::kAway, 0},
    // 01:04:41.8 killed; in its camera the stick flicked down
    {"01:04:41.930", Ev::kStick, kCrouch},
    {"01:04:44.479", Ev::kNoGun, 0},
    {"01:04:57.912", Ev::kSoldier, 4},
    {"01:04:58.000", Ev::kSwitch, kM4a1},
};
constexpr uint64_t kSessionEnd = 3900000;  // 01:05:00.000: the controllers gone from 01:05:01

// The log's "Magazine: drawn in the left hand N times in the last 10 s" lines in the owner's part: each says its 10 s had a magazine drawn in a hand; every other 10 s had none.
constexpr const char* kLogDrawnLines[] = {"00:58:00.881", "00:58:50.883", "00:59:00.892", "00:59:40.900", "00:59:50.915", "01:01:10.911",
                                         "01:01:20.912", "01:01:30.914", "01:02:30.918", "01:02:40.916", "01:02:50.915", "01:03:00.920"};
// The draws into a hand and when the log said their palms rest on them ("Held gun: the game's palms rest on it at ..."); 0: never while it was in the hand.
struct PalmsSaid {
    const char* draw;
    const char* palms;
};
constexpr PalmsSaid kLogPalms[] = {
    {"00:57:14.938", "00:57:16.496"}, {"00:57:32.071", "00:57:33.459"}, {"00:58:28.368", "00:58:29.959"}, {"00:59:51.832", nullptr},
    {"01:00:01.441", nullptr},         {"01:01:09.228", "01:01:10.661"}, {"01:02:27.995", "01:02:29.599"},
};

struct SessionOut {
    std::vector<uint64_t> windowsDrawn;  // the 10 s windows (their ends) with any frame of the magazine drawn
    struct Draw {
        uint64_t at = 0, palms = 0;
        uint32_t kind = 0;
    };
    std::vector<Draw> draws;  // each draw into a hand: when its palms were taken (0 never, while drawn)
    struct Life {
        long magFrames = 0, magDrawn = 0, misses = 0;
        uint64_t firstMiss = 0, lastMiss = 0;
    };
    Life lives[5];  // by the soldier's serial
    struct Pull {
        uint64_t at = 0;
        uint32_t kind = 0;
        long frames = 0, drawn = 0;
        bool firstFrameDrawn = false;
        uint64_t drawnFrom = 0;
    };
    std::vector<Pull> pulls;
    int stanceSaid = 0;
    std::vector<uint64_t> saidAt;
    bool seatWrong = false;
    bool movedBeforeFirstFlick = false;
};

SessionOut Replay(const Rules& rules) {
    SessionOut out;
    World w;
    w.rules = &rules;
    const uint64_t start = At(kSession[0].at);
    uint64_t windowEnd = At("00:56:50.900");
    long windowDrawn = 0;
    size_t next = 0;
    double t = static_cast<double>(start);
    SessionOut::Pull* pull = nullptr;
    long drawnBefore = 0;
    int lastSaid = 0;
    while (t < static_cast<double>(kSessionEnd)) {
        const uint64_t now = static_cast<uint64_t>(t);
        while (next < std::size(kSession) && At(kSession[next].at) <= now) {
            const Event& e = kSession[next++];
            const uint64_t at = At(e.at);
            switch (e.ev) {
            case Ev::kSoldier:
                w.soldier = e.arg;
                GameStance(w, Stance::kStand, at);  // the game makes him standing
                w.gameStanceAt = 0;
                break;
            case Ev::kSwitch:
            case Ev::kDrawn:
                if (e.ev == Ev::kSwitch || e.arg != w.kind)
                    w.raiseAt = at, w.raising = true;
                w.kind = e.arg;
                w.drawn = true;
                w.stowed = false;
                w.drawAt = at;
                w.palmsKnownAt = w.palms.known && w.rigKind == e.arg ? at : 0;  // (the same gun again: its palms still known)
                out.draws.push_back({at, w.palmsKnownAt, e.arg});
                break;
            case Ev::kAway:
                w.stowed = true;
                break;
            case Ev::kNoGun:
                w.drawn = false;
                break;
            case Ev::kStick: {
                const Stance s = e.arg == kProne ? Stance::kProne : e.arg == kCrouch ? Stance::kCrouch : Stance::kStand;
                if (e.arg == kCrouch)
                    w.crouchOn = true;
                else if (e.arg == kStand)
                    w.crouchOn = false;
                SetStance(w, s, at);
                GameStance(w, s, at);
                break;
            }
            case Ev::kSprint:
                w.sprintAt = at;
                GameStance(w, Stance::kStand, at);
                break;
            case Ev::kMagOut:
                w.mag = true;
                out.pulls.push_back({at, w.kind});
                pull = &out.pulls.back();
                drawnBefore = w.magDrawn;
                break;
            case Ev::kMagIn:
                w.mag = false;
                pull = nullptr;
                break;
            }
        }
        const long framesBefore = w.magFrames, drawnBeforeFrame = w.magDrawn, missesBefore = w.misses;
        Frame(w, now);
        if (w.stanceSaid != lastSaid) {
            lastSaid = w.stanceSaid;
            out.saidAt.push_back(now);
        }
        if (!out.draws.empty() && w.palmsKnownAt && !out.draws.back().palms)
            out.draws.back().palms = w.palmsKnownAt;
        if (w.magFrames != framesBefore) {
            SessionOut::Life& life = out.lives[std::min<unsigned>(w.soldier, 4)];
            ++life.magFrames;
            const bool drawnNow = w.magDrawn != drawnBeforeFrame;
            if (drawnNow)
                ++life.magDrawn, ++windowDrawn;
            if (w.misses != missesBefore) {
                if (!life.misses)
                    life.firstMiss = now;
                life.lastMiss = now;
                ++life.misses;
            }
            if (pull) {
                if (!pull->frames)
                    pull->firstFrameDrawn = drawnNow;
                ++pull->frames;
                if (drawnNow) {
                    if (!pull->drawn)
                        pull->drawnFrom = now;
                    ++pull->drawn;
                }
            }
        }
        (void)drawnBefore;
        t += kFrameMs;
        if (static_cast<uint64_t>(t) >= windowEnd) {
            if (windowDrawn)
                out.windowsDrawn.push_back(windowEnd);
            windowDrawn = 0;
            windowEnd += 10000;
        }
    }
    out.stanceSaid = w.stanceSaid;
    out.seatWrong = w.seatWrong;
    return out;
}

// A gun's own small run (m4): a fresh world (nothing known yet), its soldier as asked.
struct Small {
    World w;
    uint64_t now = 100000;
    void Go(uint64_t ms) {
        Run(w, now, now + ms);
        now += ms;
    }
    void Switch(uint32_t kind) {
        w.kind = kind, w.drawn = true, w.stowed = false, w.raiseAt = now, w.raising = true, w.palmsKnownAt = 0;
    }
    // A pull held this long: its frames, how many drawn, whether its first was.
    struct Pulled {
        long frames = 0, drawn = 0;
        bool first = false;
    };
    Pulled Pull(uint64_t ms) {
        Pulled p;
        w.mag = true;
        const long f0 = w.magFrames, d0 = w.magDrawn;
        Frame(w, now);
        p.first = w.magDrawn > d0;
        now += 14;
        Go(ms);
        p.frames = w.magFrames - f0, p.drawn = w.magDrawn - d0;
        w.mag = false;
        return p;
    }
};

// m: how far apart two places on the frame are (3x4s, rows).
float Apart(const float* a, const float* b) {
    const float x = a[3] - b[3], y = a[7] - b[7], z = a[11] - b[11];
    return std::sqrt(x * x + y * y + z * z);
}

// m6, the review of 2026-11-19: the game's own reload (X held 0.6 s) while the stance or a sprint moves the game's hands, cut 0.6 s in by a change of gun or
// a death; the Mk11 drawn again and its magazine pulled 0.2 s on.
enum class Cut { kProne, kDeath, kSprint };
struct CutOut {
    bool known = false;   // the Mk11's palms and its colour pass's seat known before it, its own
    float off = -1.0f;    // m: the seat the pull is drawn by from the Mk11's own (-1: none known)
    bool first = false;   // the pull drawn from its first frame
    bool all = false;     // every frame of it
    bool wrong = false;   // any frame of it drawn by a seat not the gun's own
    bool prone = false;   // the mod's stance prone at the pull
};
CutOut CutReload(const Rules& rules, Cut how) {
    CutOut out;
    Small s;
    s.w.rules = &rules;
    s.w.soldier = 1, s.w.stanceSoldier = 1;
    s.Switch(kMk11);
    s.Go(3000);  // standing: its palms known 1.55 s on, its seats by them
    float own[12];
    SeatOn(kMk11, 0, own);
    out.known = s.w.palms.known && s.w.seats.have[0] && Apart(s.w.seats.seat[0], own) < 1e-6f;
    if (how == Cut::kSprint) {
        s.w.sprintAt = s.now;  // a sprint just let go of: the game's hands blend back for a second
        GameStance(s.w, Stance::kStand, s.now);
        s.Go(100);
    } else {
        SetStance(s.w, Stance::kProne, s.now), GameStance(s.w, Stance::kProne, s.now);  // the right stick held down
        s.Go(3000);
    }
    s.w.gameReload = true;  // X held: the game's own reload, its magazine on its way out (12 cm down 0.6 s in)
    s.Go(600);
    s.w.gameReload = false;  // (cut with the gun, or with the soldier)
    if (how == Cut::kDeath) {
        s.w.drawn = false;  // killed; the next soldier 15 s on, standing
        s.Go(15000);
        s.w.soldier = 2;
        GameStance(s.w, Stance::kStand, s.now), s.w.gameStanceAt = 0;
    }
    s.Switch(kM4a1);  // another gun (the new soldier's first, as the owner's)
    s.Go(4000);
    s.Switch(kMk11);
    s.Go(200);
    if (s.w.seats.have[0])
        out.off = Apart(s.w.seats.seat[0], own);
    out.prone = s.w.stance == Stance::kProne;
    const bool wrongBefore = s.w.seatWrong;
    const Small::Pulled p = s.Pull(1000);
    out.first = p.first, out.all = p.frames > 0 && p.drawn == p.frames, out.wrong = s.w.seatWrong && !wrongBefore;
    return out;
}

}  // namespace

int main() {
    char what[900];

    // ---- m2: the replay with the rules as they were is the log ----
    const SessionOut was = Replay(kBefore);
    {
        long mk11Frames = 0, mk11Drawn = 0;
        int mk11Pulls = 0;
        for (const SessionOut::Pull& p : was.pulls)
            if (p.kind == kMk11 && p.at >= At("01:00:01.000") && p.at < At("01:00:40.000"))
                mk11Frames += p.frames, mk11Drawn += p.drawn, ++mk11Pulls;
        std::snprintf(what, sizeof(what),
                      "m2 as they were: the life after the first death: the Mk11's %d magazines in a hand (01:00:02.848-01:00:34.118, %.1f s) drawn in %ld of %ld frames (the log: never \"drawn in the left hand\" "
                      "from 00:59:50.9 to 01:01:10.9)",
                      mk11Pulls, mk11Frames / 72.0, mk11Drawn, mk11Frames);
        Check(mk11Pulls == 18 && mk11Frames > 800 && mk11Drawn == 0, what);
        // Its 10 s windows with a magazine drawn: exactly the log's lines.
        std::vector<uint64_t> logged;
        for (const char* at : kLogDrawnLines)
            logged.push_back(At(at));
        bool same = was.windowsDrawn.size() == logged.size();
        std::string list;
        for (size_t i = 0; i < was.windowsDrawn.size(); ++i) {
            list += (i ? " " : "") + Clock(was.windowsDrawn[i]).substr(0, 10);
            same = same && i < logged.size() && (was.windowsDrawn[i] > logged[i] ? was.windowsDrawn[i] - logged[i] : logged[i] - was.windowsDrawn[i]) < 100;
        }
        std::snprintf(what, sizeof(what), "m2 as they were: the 10 s windows with a magazine drawn in a hand are the log's %zu \"drawn in the left hand\" lines, no other: %s", logged.size(), list.c_str());
        Check(same, what);
        // Each draw's palms.
        bool palmsLikeLog = true;
        std::string seen;
        for (const PalmsSaid& p : kLogPalms) {
            const uint64_t draw = At(p.draw);
            const SessionOut::Draw* d = nullptr;
            for (const SessionOut::Draw& x : was.draws)
                if (x.at == draw)
                    d = &x;
            if (!d) {
                palmsLikeLog = false;
                continue;
            }
            char one[96];
            std::snprintf(one, sizeof(one), " %s %s: %s (the log %s);", NameOf(d->kind), p.draw, d->palms ? (std::to_string((d->palms - draw) / 10 / 100.0).substr(0, 4) + " s").c_str() : "never",
                          p.palms ? (std::to_string((At(p.palms) - draw) / 10 / 100.0).substr(0, 4) + " s").c_str() : "never");
            seen += one;
            if (!p.palms)
                palmsLikeLog = palmsLikeLog && !d->palms;
            else {
                const long logMs = static_cast<long>(At(p.palms) - draw), ms = d->palms ? static_cast<long>(d->palms - draw) : -1;
                palmsLikeLog = palmsLikeLog && ms >= 0 && std::labs(ms - logMs) <= 300;
            }
        }
        std::snprintf(what, sizeof(what), "m2 as they were: each draw's palms taken as long after it as the log says (within 0.3 s), never in the life after the first death:%s", seen.c_str());
        Check(palmsLikeLog, what);
        // The two pulls just before their palms: drawn only from then.
        const SessionOut::Pull* m4 = nullptr;
        const SessionOut::Pull* mk = nullptr;
        for (const SessionOut::Pull& p : was.pulls) {
            if (p.at == At("01:01:10.362"))
                m4 = &p;
            if (p.at == At("01:02:29.415"))
                mk = &p;
        }
        std::snprintf(what, sizeof(what), "m2 as they were: the M4A1's pull at 01:01:10.362 drawn from %s (the log: its palms 01:01:10.661), the Mk11's at 01:02:29.415 from %s (01:02:29.599)",
                      m4 && m4->drawn ? Clock(m4->drawnFrom).c_str() : "never", mk && mk->drawn ? Clock(mk->drawnFrom).c_str() : "never");
        Check(m4 && mk && !m4->firstFrameDrawn && !mk->firstFrameDrawn && m4->drawn && mk->drawn && m4->drawnFrom > At("01:01:10.500") && mk->drawnFrom > At("01:02:29.450"), what);
        Check(was.lives[1].misses == 0 && was.lives[1].magFrames > 100, "m2 as they were: the first life's magazines (the M4A1's fresh one, the Mk11's, the death's camera's) drawn in every frame");
        Check(was.stanceSaid == 0, "m2 as they were: no new soldier ever stood the stance up");
    }

    // ---- m1: the replay with the rules as they are ----
    const SessionOut now = Replay(kNow);
    {
        long frames = 0, drawn = 0;
        std::string lives;
        for (int s = 1; s <= 4; ++s) {
            const SessionOut::Life& l = now.lives[s];
            frames += l.magFrames, drawn += l.magDrawn;
            char one[160];
            std::snprintf(one, sizeof(one), " life %d: %ld of %ld%s%s%s;", s, l.magDrawn, l.magFrames, l.misses ? " (missed " : "", l.misses ? (Clock(l.firstMiss) + "-" + Clock(l.lastMiss)).c_str() : "",
                          l.misses ? ")" : "");
            lives += one;
        }
        std::snprintf(what, sizeof(what), "m1 as they are: every frame with a magazine in a hand draws it: %ld of %ld;%s", drawn, frames, lives.c_str());
        Check(frames > 2000 && drawn == frames, what);
        long mk11Frames = 0, mk11Drawn = 0;
        bool firstFrames = true;
        int pulls = 0;
        for (const SessionOut::Pull& p : now.pulls) {
            firstFrames = firstFrames && p.firstFrameDrawn;
            if (p.kind == kMk11 && p.at >= At("01:00:01.000") && p.at < At("01:00:40.000"))
                mk11Frames += p.frames, mk11Drawn += p.drawn, ++pulls;
        }
        std::snprintf(what, sizeof(what), "m1 as they are: the Mk11 after the first death: its %d magazines in a hand from 01:00:02.848 to 01:00:34.118 drawn in %ld of %ld frames", pulls, mk11Drawn, mk11Frames);
        Check(pulls == 18 && mk11Frames > 800 && mk11Drawn == mk11Frames, what);
        std::snprintf(what, sizeof(what), "m1 as they are: each of the session's %zu magazines into a hand drawn from its first frame", now.pulls.size());
        Check(firstFrames && now.pulls.size() == 27, what);
        std::vector<uint64_t> logged;
        for (const char* at : kLogDrawnLines)
            logged.push_back(At(at));
        size_t more = 0;
        for (uint64_t end : now.windowsDrawn) {
            bool inLog = false;
            for (uint64_t l : logged)
                inLog = inLog || (end > l ? end - l : l - end) < 100;
            more += !inLog;
        }
        std::snprintf(what, sizeof(what), "m1 as they are: the magazine drawn in %zu windows of 10 s, the log's %zu and %zu more (the four of 01:00:02-01:00:34)", now.windowsDrawn.size(), logged.size(), more);
        Check(more == 4 && now.windowsDrawn.size() == logged.size() + 4, what);
        Check(!now.seatWrong && !was.seatWrong, "m1 every magazine drawn in a hand is drawn by its own gun's seat (none of another's)");
    }

    // ---- m3: a new soldier stands ----
    {
        std::string said;
        for (uint64_t at : now.saidAt)
            said += " " + Clock(at);
        std::snprintf(what, sizeof(what), "m3 the stance back to standing at each new soldier it wasn't standing at, said once each: 3 (prone at 00:59:51, crouched at 01:02:21 and 01:04:57):%s", said.c_str());
        Check(now.stanceSaid == 3 && now.saidAt.size() == 3 && now.saidAt[0] >= At("00:59:51.552") && now.saidAt[0] < At("00:59:51.600") && now.saidAt[1] >= At("01:02:21.592") &&
                  now.saidAt[1] < At("01:02:21.640") && now.saidAt[2] >= At("01:04:57.912") && now.saidAt[2] < At("01:04:57.960"),
              what);
        bool palms = true;
        std::string seen;
        for (const SessionOut::Draw& d : now.draws) {
            if (d.at < At("00:59:51.000") || d.at > At("01:00:40.000"))
                continue;
            seen += " " + std::string(NameOf(d.kind)) + " " + Clock(d.at) + ": " + (d.palms ? std::to_string((d.palms - d.at) / 10 / 100.0).substr(0, 4) + " s" : "never") + ";";
            palms = palms && d.palms && d.palms - d.at <= 2000;
        }
        std::snprintf(what, sizeof(what), "m3 the life after the first death: every draw's palms known within 2 s of it, as in every other life (the same gun drawn again: at once):%s", seen.c_str());
        Check(palms, what);
        // The rule alone: the stick's stance stays for its soldier; a new one stands; the first soldier, standing, says nothing.
        unsigned seen1 = 0;
        Stance st = Stance::kStand;
        bool crouch = false;
        const bool first = inputrules::StanceForSoldier(seen1, 1, st, crouch);
        st = Stance::kProne;
        const bool same = inputrules::StanceForSoldier(seen1, 1, st, crouch);
        const bool stays = st == Stance::kProne;
        const bool next = inputrules::StanceForSoldier(seen1, 2, st, crouch);
        const bool stood = st == Stance::kStand && !crouch;
        st = Stance::kCrouch, crouch = true;
        const bool third = inputrules::StanceForSoldier(seen1, 3, st, crouch);
        const bool again = inputrules::StanceForSoldier(seen1, 3, st, crouch);
        Check(!first && !same && stays && next && stood && third && st == Stance::kStand && !crouch && !again,
              "m3 the rule: a stance the stick put stays for its soldier (prone, frame after frame); a new soldier stands with the crouch toggle off, said once; the first soldier, standing, says nothing");
        unsigned seen2 = 0;
        Stance s2 = Stance::kStand;
        bool c2 = true;
        Check(inputrules::StanceForSoldier(seen2, 5, s2, c2) && !c2, "m3 standing with the game's crouch toggle left on: a new soldier turns it off too, said");
    }

    // ---- m4: every gun ----
    {
        int prone = 0, sprint = 0, early = 0, back = 0, death = 0;
        std::string bad;
        for (const Gun& g : kGuns) {
            // Prone the whole time, the gun drawn the first time: a pull a second on.
            {
                Small s;
                SetStance(s.w, Stance::kProne, s.now), GameStance(s.w, Stance::kProne, s.now), s.w.stanceChangedAt = 0, s.w.gameStanceAt = 0;
                s.w.soldier = 1, s.w.stanceSoldier = 1;
                s.Go(3000);
                s.Switch(g.kind);
                s.Go(1000);
                const Small::Pulled p = s.Pull(2000);
                if (p.first && p.drawn == p.frames && !s.w.palms.known && !s.w.seatWrong)
                    ++prone;
                else
                    bad += std::string(" ") + g.name + " prone (" + std::to_string(p.drawn) + "/" + std::to_string(p.frames) + ")";
                // Another gun, and this one again, prone: its own seat from its first frame.
                const uint32_t other = g.kind == kM4a1 ? kMk11 : kM4a1;
                s.Switch(other);
                s.Go(1500);
                s.Pull(500);
                s.Switch(g.kind);
                const Small::Pulled q = s.Pull(1000);
                if (q.first && q.drawn == q.frames && !s.w.seatWrong)
                    ++back;
                else
                    bad += std::string(" ") + g.name + " again (" + std::to_string(q.drawn) + "/" + std::to_string(q.frames) + ")";
                // Killed prone, the new soldier (standing) with another gun in his hands first (the owner's: the M4A1, put away 4 s on), then this one: a pull 0.2 s
                // after its draw.
                s.w.drawn = false;
                s.Go(15000);
                s.w.soldier = 2;
                GameStance(s.w, Stance::kStand, s.now), s.w.gameStanceAt = 0;
                s.Switch(other);
                s.Go(4000);
                s.Switch(g.kind);
                s.Go(200);
                const Small::Pulled d = s.Pull(1500);
                if (d.first && d.drawn == d.frames && s.w.stance == Stance::kStand && s.w.palms.known && !s.w.seatWrong)
                    ++death;
                else
                    bad += std::string(" ") + g.name + " after a death (" + std::to_string(d.drawn) + "/" + std::to_string(d.frames) + ")";
            }
            // The sprint held (the left stick's click), the gun drawn the first time: a pull a second on.
            {
                Small s;
                s.w.soldier = 1, s.w.stanceSoldier = 1;
                s.Switch(g.kind);
                bool all = true, first = false;
                long frames = 0;
                for (int k = 0; k < 72 * 3; ++k) {
                    s.w.sprintAt = s.now;  // held: seen each frame
                    if (k == 72)
                        s.w.mag = true;
                    const long d0 = s.w.magDrawn, f0 = s.w.magFrames;
                    Frame(s.w, s.now);
                    if (s.w.magFrames > f0) {
                        if (!frames)
                            first = s.w.magDrawn > d0;
                        ++frames;
                        all = all && s.w.magDrawn > d0;
                    }
                    s.now += 14;
                }
                if (first && all && frames > 100 && !s.w.palms.known)
                    ++sprint;
                else
                    bad += std::string(" ") + g.name + " sprinting";
            }
            // Standing, the gun drawn the first time: a pull 0.8 s on (before its palms are known).
            {
                Small s;
                s.w.soldier = 1, s.w.stanceSoldier = 1;
                s.Switch(g.kind);
                s.Go(800);
                const bool palmsThen = s.w.palms.known;
                const Small::Pulled p = s.Pull(1500);
                if (!palmsThen && p.first && p.drawn == p.frames && s.w.palms.known)
                    ++early;
                else
                    bad += std::string(" ") + g.name + " standing early (" + std::to_string(p.drawn) + "/" + std::to_string(p.frames) + ")";
            }
        }
        const int n = static_cast<int>(std::size(kGuns));
        std::snprintf(what, sizeof(what), "m4 every gun (%d): drawn the first time prone, a magazine pulled a second on drawn from its first frame, its palms still not known: %d of %d", n, prone, n);
        Check(prone == n, what);
        std::snprintf(what, sizeof(what), "m4 every gun: drawn the first time with the sprint held, its magazine drawn from its first frame: %d of %d", sprint, n);
        Check(sprint == n, what);
        std::snprintf(what, sizeof(what), "m4 every gun: standing, a pull 0.8 s after its first draw (its palms not yet known) drawn from its first frame: %d of %d", early, n);
        Check(early == n, what);
        std::snprintf(what, sizeof(what), "m4 every gun: drawn again after another, prone, its own seat from the pull's first frame: %d of %d", back, n);
        Check(back == n, what);
        std::snprintf(what, sizeof(what),
                      "m4 every gun: killed prone, the new soldier's other gun first (4 s), then this one: a pull 0.2 s after its draw drawn from its first frame, the stance standing, "
                      "its palms known by the pull's end: %d of %d%s%s",
                      death, n, bad.empty() ? "" : "; wrong:", bad.c_str());
        Check(death == n, what);
        // Thirteen guns in turn, prone: each pull drawn with its own gun's seat.
        Small s;
        SetStance(s.w, Stance::kProne, s.now), GameStance(s.w, Stance::kProne, s.now), s.w.stanceChangedAt = 0, s.w.gameStanceAt = 0;
        s.w.soldier = 1, s.w.stanceSoldier = 1;
        bool all = true;
        for (int round = 0; round < 2; ++round)
            for (const Gun& g : kGuns) {
                s.Switch(g.kind);
                s.Go(round == 0 ? 700 : 20);
                const Small::Pulled p = s.Pull(300);
                all = all && p.first && p.drawn == p.frames;
            }
        Check(all && !s.w.seatWrong, "m4 thirteen guns in turn, prone, twice round: every pull drawn from its first frame with its own gun's seat (the second round 20 ms after each draw)");
    }

    // ---- m5: what was right stays so ----
    {
        float seat[12], other[12];
        SeatOn(kMk11, 0, seat);
        SeatOn(kMk11, 0, other);
        other[7] -= 0.02f;
        // Bare arms: no palms ever; the magazine at rest gives the seat, and a new rest a new one (as before).
        {
            seatrest::Seats s;
            seatrest::Watch w;
            bool firstTaken = false, secondTaken = false;
            for (uint64_t t = 0; t <= 600; t += 14)
                firstTaken = seatrest::Take(s, 0, w, seat, false, false, false, true, t) == seatrest::Took::kAtRest || firstTaken;
            for (uint64_t t = 614; t <= 1300; t += 14)
                secondTaken = seatrest::Take(s, 0, w, other, false, false, false, true, t) == seatrest::Took::kAtRest || secondTaken;
            Check(firstTaken && secondTaken && std::memcmp(s.seat[0], other, sizeof(other)) == 0, "m5 bare arms: the magazine at rest on the frame gives its seat, a new rest a new one (as it was)");
        }
        // The game's own reload: its palms known and moving: nothing taken, the seat known kept.
        {
            seatrest::Seats s;
            seatrest::Watch w;
            seatrest::Take(s, 0, w, seat, true, false, false, false, 0);
            const seatrest::Took took = seatrest::Take(s, 0, w, other, true, true, false, false, 14);
            const seatrest::Took tookMoved = seatrest::Take(s, 0, w, other, true, true, true, false, 28);
            Check(took == seatrest::Took::kNone && tookMoved == seatrest::Took::kNone && std::memcmp(s.seat[0], seat, sizeof(seat)) == 0,
                  "m5 the game's own reload (its palms known, moving): no seat taken, the one known kept (the stance or a sprint moving its hands too)");
            const seatrest::Took byPalms = seatrest::Take(s, 0, w, other, true, false, false, false, 42);
            Check(byPalms == seatrest::Took::kByPalms && std::memcmp(s.seat[0], other, sizeof(other)) == 0, "m5 the palms known and at rest: the seat taken from every draw, as it was");
        }
        // The palms known while the stance or a sprint moves the game's hands (their moving not told): none from the palms; the seat known kept, whatever the
        // magazine does (on its way, or at rest elsewhere for 2 s); one not known taken at rest (still 499 ms no, 500 ms yes).
        {
            seatrest::Seats s;
            seatrest::Watch w;
            std::memcpy(s.seat[0], seat, sizeof(seat)), s.have[0] = true;
            bool taken = false;
            for (uint64_t t = 0; t <= 2000; t += 14) {
                float on[12];
                std::memcpy(on, seat, sizeof(on));
                on[7] -= 0.15f * std::sin(3.14159265f * static_cast<float>(t) / 2000.0f);  // on its way out and back, as the game's own reload's
                taken = seatrest::Take(s, 0, w, on, true, false, true, false, t) != seatrest::Took::kNone || taken;
            }
            for (uint64_t t = 2014; t <= 4100; t += 14)
                taken = seatrest::Take(s, 0, w, other, true, false, true, false, t) != seatrest::Took::kNone || taken;
            const bool kept = !taken && std::memcmp(s.seat[0], seat, sizeof(seat)) == 0;
            seatrest::Seats n;
            seatrest::Watch v;
            bool at499 = false;
            for (uint64_t t = 10000; t <= 10499; ++t)
                at499 = seatrest::Take(n, 0, v, seat, true, false, true, false, t) != seatrest::Took::kNone || at499;
            const bool at500 = seatrest::Take(n, 0, v, seat, true, false, true, false, 10500) == seatrest::Took::kAtRest;
            Check(kept && !at499 && at500 && std::memcmp(n.seat[0], seat, sizeof(seat)) == 0,
                  "m5 the palms known, the stance or a sprint moving the game's hands: no seat from them (the magazine on its way 2 s, then at rest 2 cm off for 2 s: the one known "
                  "kept); one not known taken at rest (499 ms no, 500 ms yes)");
        }
        // Gloves, no palms: the rest never replaces a seat known.
        {
            seatrest::Seats s;
            seatrest::Watch w;
            std::memcpy(s.seat[0], seat, sizeof(seat)), s.have[0] = true;
            bool taken = false;
            for (uint64_t t = 0; t <= 2000; t += 14)
                taken = seatrest::Take(s, 0, w, other, false, false, false, false, t) != seatrest::Took::kNone || taken;
            Check(!taken && std::memcmp(s.seat[0], seat, sizeof(seat)) == 0, "m5 gloves, the palms not known: a magazine at rest elsewhere for 2 s never replaces the seat known");
        }
        // Gloves, no palms, none known: the game's own reload moving it never gives one; still for 499 ms no, for 500 ms yes.
        {
            World w;
            w.kind = kMk11, w.drawn = true, w.gameReload = true, w.soldier = 1, w.stanceSoldier = 1;
            Run(w, 1000, 5000);
            const bool none = !w.seats.have[0] && !w.seats.have[1] && !w.seats.have[2] && !w.seats.have[3];
            seatrest::Seats s;
            seatrest::Watch v;
            bool at499 = false;
            for (uint64_t t = 10000; t <= 10499; t += 1)
                at499 = seatrest::Take(s, 0, v, seat, false, false, false, false, t) != seatrest::Took::kNone || at499;
            const bool at500 = seatrest::Take(s, 0, v, seat, false, false, false, false, 10500) == seatrest::Took::kAtRest;
            Check(none && !at499 && at500 && s.have[0], "m5 gloves, the palms not known, no seat known: a magazine moving on the frame (the game's own reload, 4 s) gives none; still 499 ms no, 500 ms its seat");
        }
        // The game's palms: their rule as it was.
        {
            heldpalms::State p;
            const float r[3] = {-0.054f, -0.114f, 0.254f}, l[3] = {0.012f, -0.041f, 0.611f};
            bool early = false;
            for (uint64_t t = 5000; t <= 6000; t += 10)  // (a clock as GetTickCount64's: never 0)
                early = heldpalms::Note(p, r, l, t, false) || early;
            const bool at1010 = heldpalms::Note(p, r, l, 6010, false);
            Check(!early && at1010 && p.known, "m5 the game's palms: taken once still 1 s and a frame (not at 1000 ms)");
            heldpalms::State q;
            bool sprinted = false;
            for (uint64_t t = 5000; t <= 10000; t += 10)
                sprinted = heldpalms::Note(q, r, l, t, true) || sprinted;
            Check(!sprinted && !q.known, "m5 the game's palms: never taken while the stance or a sprint moves the game's hands (5 s)");
            float rr[3] = {r[0], r[1] - 0.07f, r[2]};
            heldpalms::Note(p, rr, l, 6100, false);
            const bool notYet = !p.moving;
            heldpalms::Note(p, rr, l, 6159, false);
            const bool at59 = p.moving;
            heldpalms::Note(p, rr, l, 6160, false);
            const bool at60 = p.moving;
            heldpalms::Note(p, r, l, 6170, false);
            Check(notYet && !at59 && at60 && !p.moving, "m5 the game's palms: moving once 6 cm off for 60 ms (not 59), back within 2 cm: not");
            float r2[3] = {r[0] + 0.03f, r[1], r[2]};
            bool retaken = false;
            uint64_t retakenAt = 0;
            for (uint64_t t = 7000; t <= 12000 && !retaken; t += 10)
                if ((retaken = heldpalms::Note(p, r2, l, t, false)))
                    retakenAt = t;
            Check(retaken && retakenAt > 11000 && p.palm[0][0] == r2[0], "m5 the game's palms: a new rest 3 cm on taken after 4 s there, as it was");
            heldpalms::Forget(p);
            Check(!p.known && !p.moving && !p.trySince, "m5 the game's palms: another gun knows none of them");
        }
        // The stance's blend.
        Check(inputrules::HandsMovedByStance(10999, 10000, false, 0) && !inputrules::HandsMovedByStance(11000, 10000, false, 0) && inputrules::HandsMovedByStance(50000, 0, true, 0) &&
                  inputrules::HandsMovedByStance(21499, 0, false, 20000) && !inputrules::HandsMovedByStance(21500, 0, false, 20000) && !inputrules::HandsMovedByStance(50000, 0, false, 0),
              "m5 the game's hands moved by a sprint 999 ms on (not 1000), prone always, a stance change 1499 ms on (not 1500); standing still never");
        // The seats kept.
        {
            seatrest::Kept k;
            seatrest::Seats now;
            float a[12], b[12];
            SeatOn(kMk11, 0, a), SeatOn(kM4a1, 0, b);
            seatrest::Change(k, now, 0, kMk11);
            const bool none = !now.have[0];
            std::memcpy(now.seat[0], a, sizeof(a)), now.have[0] = true;
            seatrest::Change(k, now, kMk11, kM4a1);
            const bool notMk11 = !now.have[0];
            std::memcpy(now.seat[0], b, sizeof(b)), now.have[0] = true;
            seatrest::Change(k, now, kM4a1, kMk11);
            const bool mk11Back = now.have[0] && std::memcmp(now.seat[0], a, sizeof(a)) == 0 && !now.have[1];
            now.have[0] = false;  // (none known of it this time: what it had is kept)
            std::memcpy(now.seat[1], b, sizeof(b)), now.have[1] = true;
            seatrest::Change(k, now, kMk11, 0);
            const bool launcher = !now.have[0] && !now.have[1];
            seatrest::Change(k, now, 0, kMk11);
            const bool merged = now.have[0] && std::memcmp(now.seat[0], a, sizeof(a)) == 0 && now.have[1] && std::memcmp(now.seat[1], b, sizeof(b)) == 0;
            seatrest::Change(k, now, kMk11, kM4a1);
            const bool m4Back = now.have[0] && std::memcmp(now.seat[0], b, sizeof(b)) == 0;
            Check(none && notMk11 && mk11Back && launcher && merged && m4Back,
                  "m5 the seats kept: a gun never seen has none, another gun's never come with it, each comes back with its own, the launcher (0) has none, one not known this time keeps what it had");
            bool full = true;
            seatrest::Kept many;
            seatrest::Seats in;
            uint32_t from = 0;
            for (uint32_t kind = 1000; kind < 1000 + 3 * seatrest::kKinds; ++kind) {
                seatrest::Change(many, in, from, kind);
                full = full && !in.have[0];
                float m[12];
                SeatOn(kind, 0, m);
                std::memcpy(in.seat[0], m, sizeof(m)), in.have[0] = true;
                from = kind;
            }
            const uint32_t last = from;
            seatrest::Change(many, in, from, last - 1);
            float want[12];
            SeatOn(last - 1, 0, want);
            Check(full && in.have[0] && std::memcmp(in.seat[0], want, sizeof(want)) == 0 && many.count == seatrest::kKinds,
                  "m5 the seats kept: three times as many guns as kept, never another's; the last ones come back with their own");
        }
    }

    // ---- m6: the game's own reload while the stance or a sprint moves the game's hands, cut by a change of gun or a death (the review of 2026-11-19) ----
    {
        const char* const kHow[] = {"prone, a change of gun in the middle of it, the Mk11 drawn again prone", "killed prone in the middle of it, the next soldier's M4A1 first, then the Mk11",
                                    "begun in a sprint's blend, a change of gun in the middle of it, the Mk11 drawn again standing"};
        const Cut kCuts[] = {Cut::kProne, Cut::kDeath, Cut::kSprint};
        std::string was, is;
        bool reproduced = true, right = true;
        for (int c = 0; c < 3; ++c) {
            const CutOut b = CutReload(kBranch, kCuts[c]), n = CutReload(kNow, kCuts[c]);
            char one[200];
            std::snprintf(one, sizeof(one), " %s: %.0f mm off%s;", kHow[c], b.off * 1000.0f, b.wrong ? ", drawn by it" : "");
            was += one;
            std::snprintf(one, sizeof(one), " %s: %.1f mm off, drawn from the pull's first frame %s, every frame %s%s;", kHow[c], n.off * 1000.0f, n.first ? "yes" : "no", n.all ? "yes" : "no",
                          n.prone ? " (prone)" : "");
            is += one;
            reproduced = reproduced && b.known && b.off > 0.05f && b.wrong;
            right = right && n.known && n.off >= 0.0f && n.off < 1e-6f && n.first && n.all && !n.wrong && n.prone == (kCuts[c] == Cut::kProne);
        }
        std::snprintf(what, sizeof(what), "m6 with the branch's rule before its review (a359c3c): the seat the magazine in the hand is drawn by, from the Mk11's own:%s", was.c_str());
        Check(reproduced, what);
        std::snprintf(what, sizeof(what), "m6 as they are: the Mk11's own seat kept through the game's own reload, the magazine in the hand drawn by it:%s", is.c_str());
        Check(right, what);
    }

    std::printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed;
}

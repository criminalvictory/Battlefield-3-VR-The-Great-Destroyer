#pragma once
// The player's soldier's real pose (standing, crouched, prone) as the game keeps it (bf3.exe 1.6.0.0, read in build\bf3_mem.exe 2026-11-19, the
// stance study after the owner's "if I flick up, I crouch"). Until now the mod only believed the stance (what its own key presses should have done):
// a sprint press that didn't stand the soldier up, a death, a checkpoint that lays the soldier prone, and the belief and the game parted, after which
// the game's crouch toggle went the wrong way.
//
// Where it is. ClientSoldierEntity (0x3A0 bytes by its type info; its vtable 0x0214BD40, slot 0 getType 0x010C2A80 -> "ClientSoldierEntity") keeps its
// CharacterPhysicsEntity (0x220 bytes; vtable 0x0207FBD8, slot 0 0x00586F50 -> "CharacterPhysicsEntity") at +0x164: the soldier's constructor stores it
// there (0x010D9FD5), and the client's movement passes the soldier's +0xF0 part to the soldier's input update (0x011F8860), which takes its physics from
// that part's +0x74 (0xF0 + 0x74 = 0x164) and reads +0x164 again straight after (0x01051800). (The server's soldier keeps its own at +0x194: 0x00AC0A68.)
// In the physics:
//   +0x7C the current pose (CharacterPoseType: 0 Stand, 1 Crouch, 2 Prone, by the enum's type info): what its body is in. Its getter 0x00587440.
//   +0x80 the pose it is changing to, the same as +0x7C when no change is under way. The game's own getPose (0x00587450) returns it, and the
//         soldier's pose input (0x011F7050) decides every toggle from it: a change sets it at once (setPose 0x00591D30 writes it at 0x00591E6F).
//   +0x84 the change's time left (seconds, a float): at 0 the change ends and +0x7C takes +0x80 (0x005913E0); a change refused at its end (no room)
//         puts +0x80 back to +0x7C.
//   +0x90 three bytes, the poses allowed now (CharacterPoseConstraintsData: StandPose, CrouchPose, PronePose; all 1 from the constructor 0x005FF1E0):
//         setPose refuses a pose whose byte is 0 (0x005FF1F0), and new constraints move the soldier to the first allowed pose (0x00592100). A scripted
//         moment's lock on the stance comes this way (or by the game not reading the keys at all, which a press that changes nothing shows).
//   +0xB4 its CharacterPhysicsData (vtable 0x0207E564, getType 0x01F02830 -> "CharacterPhysicsData"), whose +0x4C AllowPoseChangeDuringTransition
//         (by its type info) setPose reads (0x00591D63): when it's 0 a press during a change is ignored.
//   The constructor (0x00592E47) writes the vtable and puts +0x7C and +0x80 at 0 (standing): a new soldier stands until the game says otherwise.
// What the game's keys do (the pose input 0x011F7050, with InputConfigurationAsset's actions by its type info: TogglePoseAction +0x18, CrouchAction +0x1C,
// CrouchAndHoldAction +0x20; PC: X the crouch toggle, Z EIAProne, Left Ctrl crouch-and-hold), each from +0x80 at the press:
//   X (CrouchAction): crouched -> standing; standing or prone -> crouched. (Not while Left Ctrl, crouch-and-hold, is down; and not to crouched while
//     the sprint is wanted or the water is 0.8 m deep.)
//   Z (EIAProne): prone -> standing; standing or crouched -> prone (on the ground, in water under 0.1 m).
//   There are no toggle states of their own: every press acts on the pose the soldier has. (The mod's old "out of prone by Z, back to what X last left"
//   was never the game's: from a crouch, prone, then up, Z alone stands, so its Z-then-X crouched again.)
// What stands the soldier up by itself (the movement 0x011F8860, InputConfigurationAsset StandAutomaticallyIfSprinting +0x34, ProneToStandOnJump +0x35):
//   the sprint wanted (the sprint held and the forward throttle over 0.5, or already sprinting) with the soldier not standing; a jump from prone. A sprint
//   press standing still does nothing to the pose.
//
// Every constant is borne out at startup by the game's code bytes and type names (Prove); one that isn't turns the reader off for the run (the stick's
// stance then goes by the mod's belief, stance.h's fallback). Each frame: the soldier's vtable, its physics' and its data's checked, the poses 0-2, the
// time finite. Plain heap and image memory only, nothing the game mapped from the graphics card (AMD-safe). Pure: memory is read through the caller's
// reader (input.cpp's guarded one; a made-up image in tools/stancetest.cpp; bf3_mem.exe there too), never directly.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace poseread {

// ------------------------------------------------- the constants (bf3.exe's image base taken off) -------------------------------------------------
constexpr uint32_t kSoldierVtable = 0x1D4BD40;  // ClientSoldierEntity's
constexpr uint32_t kPhysicsVtable = 0x1C7FBD8;  // CharacterPhysicsEntity's
constexpr uint32_t kDataVtable = 0x1C7E564;     // CharacterPhysicsData's
// The layouts.
constexpr uint32_t kSoldierPhysics = 0x164;  // ClientSoldierEntity -> its CharacterPhysicsEntity
constexpr uint32_t kCurrent = 0x7C, kTarget = 0x80, kTimer = 0x84, kAllowed = 0x90, kData = 0xB4;
constexpr uint32_t kDataChangeInTransition = 0x4C;  // CharacterPhysicsData's AllowPoseChangeDuringTransition (a byte)
// The code that bears them out.
constexpr uint32_t kPhysicsCtor = 0x192E47;      // its constructor: the vtable, +0x60..+0x74 cleared, +0x7C and +0x80 standing, +0x84 0.0
constexpr uint32_t kGetPose = 0x187450;          // its getPose: +0x80
constexpr uint32_t kGetCurrentPose = 0x187440;   // its getCurrentPose: +0x7C
constexpr uint32_t kSetPose = 0x191D30;          // setPose(pose, ...): +0x7C and +0x80 against the pose asked for, +0xB4's +0x4C
constexpr uint32_t kSetPoseAllowed = 0x191DAA;   // ... the constraints at +0x90 asked
constexpr uint32_t kSetPoseWrite = 0x191E6F;     // ... +0x80 written with the pose asked for
constexpr uint32_t kAllowedTest = 0x1FF1F0;      // the constraints' test: the byte at +pose
constexpr uint32_t kSoldierMove = 0xC517F1;      // the client soldier's movement: its +0xF0 part handed on, then +0x164 read
constexpr uint32_t kMovePhysics = 0xDF886E;      // the soldier's input update: the physics from its part's +0x74
constexpr uint32_t kSoldierCtor = 0xCD9FD5;      // the client soldier's constructor storing its physics at +0x164
constexpr uint32_t kInputGetPose = 0xDF705F;     // the pose input: its toggles from getPose (+0x80)
constexpr uint32_t kInputCrouch = 0xDF713A;      // ... CrouchAction (the mapping's +0x0C) pressed
constexpr uint32_t kInputCrouchRule = 0xDF7163;  // ... crouched: up (to standing); else down (to crouched)
constexpr uint32_t kInputProne = 0xDF7175;       // ... EIAProne (14) pressed, on the ground: prone -> standing; else prone

constexpr const char* kPhysicsCtorBytes =
    "C7 06 ?? ?? ?? ?? 89 5E 60 89 5E 64 89 5E 68 89 5E 6C 89 5E 74 89 5E 7C 89 9E 80 00 00 00 F3 0F 11 86 84 00 00 00";
constexpr const char* kGetPoseBytes = "8B 81 80 00 00 00 C3";
constexpr const char* kGetCurrentPoseBytes = "8B 41 7C C3";
constexpr const char* kSetPoseBytes =
    "56 8B F1 8B 46 7C 57 8B 7C 24 0C 3B F8 75 16 39 86 80 00 00 00 75 0E 80 7C 24 10 00 75 07 5F B0 01 5E C2 10 00 80 7C 24 14 00 75 17 80 7C 24 10 "
    "00 75 10 8B 8E B4 00 00 00 80 79 4C 00";
constexpr const char* kSetPoseAllowedBytes = "57 8D 8E 90 00 00 00 E8";
constexpr const char* kSetPoseWriteBytes = "89 BE 80 00 00 00";
constexpr const char* kAllowedTestBytes = "8B 44 24 04 8A 04 08 C2 04 00";
constexpr const char* kSoldierMoveBytes = "8D 83 F0 00 00 00 50 8D 4E 10 E8 ?? ?? ?? ?? 8B 9B 64 01 00 00";
constexpr const char* kMovePhysicsBytes = "8B 5E 74";
constexpr const char* kSoldierCtorBytes = "89 86 64 01 00 00";
constexpr const char* kInputGetPoseBytes = "8B CF E8 EA 03 39 FF";  // (call 0x00587450, getPose)
constexpr const char* kInputCrouchBytes = "8B 46 0C 8B 4C 24 20 50 E8 ?? ?? ?? ?? 84 C0 74 26";
constexpr const char* kInputCrouchRuleBytes = "83 FD 01 0F 84 A4 00 00 00 E9 77 FF FF FF";
constexpr const char* kInputProneBytes = "6A 0E 8B CE E8 ?? ?? ?? ?? 84 C0 74 5E 8B CF E8 ?? ?? ?? ?? 85 C0 75 53 83 FD 02 74 7E";

enum PoseType : int { kStand = 0, kCrouch = 1, kProne = 2 };  // CharacterPoseType
inline const char* PoseName(int p) {
    return p == kStand ? "standing" : p == kCrouch ? "crouched" : p == kProne ? "prone" : "?";
}

// ------------------------------------------------------------- reading the image -------------------------------------------------------------

// A pattern's bytes at an address ("8B 54 ?? 04": ?? any byte; at most 96 bytes).
template <class Reader>
bool Matches(const Reader& read, uint32_t at, const char* pattern) {
    uint8_t want[96], mask[96], got[96];
    int n = 0;
    const auto hex = [](char c) -> int {
        return c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10 : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
    };
    for (const char* p = pattern; *p;) {
        if (*p == ' ') {
            ++p;
            continue;
        }
        if (n == 96 || !p[1])
            return false;
        if (p[0] == '?' && p[1] == '?') {
            want[n] = 0, mask[n++] = 0;
        } else {
            const int hi = hex(p[0]), lo = hex(p[1]);
            if (hi < 0 || lo < 0)
                return false;
            want[n] = static_cast<uint8_t>(hi * 16 + lo), mask[n++] = 0xFF;
        }
        p += 2;
    }
    if (!n || !read(at, got, static_cast<size_t>(n)))
        return false;
    for (int i = 0; i < n; ++i)
        if ((got[i] & mask[i]) != want[i])
            return false;
    return true;
}

template <class Reader>
uint32_t Word(const Reader& read, uint32_t at, bool* ok = nullptr) {
    uint32_t v = 0;
    const bool got = read(at, &v, 4);
    if (ok)
        *ok = got;
    return got ? v : 0;
}

// The name of the type a vtable's getType gives (its slot 0: mov eax, <ClassInfo>; ret; [ClassInfo] its ClassInfoData, whose +0 is the name),
// compared with want (letters up to its end, read a piece at a time).
template <class Reader>
bool TypeNamed(const Reader& read, uint32_t vtable, const char* want) {
    uint32_t getType = 0, info = 0, data = 0, text = 0;
    uint8_t code[6] = {};
    if (!read(vtable, &getType, 4) || !read(getType, code, 6) || code[0] != 0xB8 || code[5] != 0xC3)
        return false;
    std::memcpy(&info, code + 1, 4);
    if (!read(info, &data, 4) || !read(data, &text, 4))
        return false;
    const size_t n = std::strlen(want) + 1;  // (its end too: "ClientSoldierEntityData" isn't "ClientSoldierEntity")
    char got[64] = {};
    if (n > sizeof(got) || !read(text, got, n))
        return false;
    return std::memcmp(got, want, n) == 0;
}

// ------------------------------------------------------------------ the proofs ------------------------------------------------------------------

constexpr int kProofs = 16;
inline const char* ProofName(int i) {
    static const char* const kNames[kProofs] = {
        "ClientSoldierEntity's vtable (by its type's name)",
        "CharacterPhysicsEntity's vtable (by its type's name)",
        "CharacterPhysicsData's vtable (by its type's name)",
        "the physics' constructor (its vtable, +0x7C and +0x80 standing, +0x84 a float)",
        "the physics' getPose (+0x80)",
        "the physics' getCurrentPose (+0x7C)",
        "setPose's start (+0x7C and +0x80 against the pose asked for, the data at +0xB4 and its +0x4C)",
        "setPose's constraints (+0x90)",
        "setPose's write of +0x80",
        "the constraints' test (a byte a pose)",
        "the client soldier's movement (its +0xF0 part handed on, +0x164 read after)",
        "the soldier's input update (the physics at its part's +0x74)",
        "the client soldier's constructor (its physics stored at +0x164)",
        "the pose input's toggles from getPose",
        "the pose input's crouch toggle (crouched: up; else down)",
        "the pose input's prone toggle (prone: up; else down)",
    };
    return i >= 0 && i < kProofs ? kNames[i] : "?";
}

struct Proofs {
    bool ok = false;
    int failed = -1;  // the first proof not borne out (ProofName)
    uint32_t at = 0;  // where it was looked for
};

template <class Reader>
bool ProveOne(const Reader& read, uint32_t base, int i, uint32_t* at) {
    uint32_t where = 0;
    const auto named = [&](uint32_t vtable, const char* want) {
        where = vtable;
        return TypeNamed(read, vtable, want);
    };
    const auto bytes = [&](uint32_t code, const char* pattern) {
        where = code;
        return Matches(read, code, pattern);
    };
    bool ok = false;
    switch (i) {
    case 0: ok = named(base + kSoldierVtable, "ClientSoldierEntity"); break;
    case 1: ok = named(base + kPhysicsVtable, "CharacterPhysicsEntity"); break;
    case 2: ok = named(base + kDataVtable, "CharacterPhysicsData"); break;
    case 3: {
        where = base + kPhysicsCtor;
        bool got = false;
        ok = Matches(read, where, kPhysicsCtorBytes) && Word(read, where + 2, &got) == base + kPhysicsVtable && got;
        break;
    }
    case 4: ok = bytes(base + kGetPose, kGetPoseBytes); break;
    case 5: ok = bytes(base + kGetCurrentPose, kGetCurrentPoseBytes); break;
    case 6: ok = bytes(base + kSetPose, kSetPoseBytes); break;
    case 7: ok = bytes(base + kSetPoseAllowed, kSetPoseAllowedBytes); break;
    case 8: ok = bytes(base + kSetPoseWrite, kSetPoseWriteBytes); break;
    case 9: ok = bytes(base + kAllowedTest, kAllowedTestBytes); break;
    case 10: ok = bytes(base + kSoldierMove, kSoldierMoveBytes); break;
    case 11: ok = bytes(base + kMovePhysics, kMovePhysicsBytes); break;
    case 12: ok = bytes(base + kSoldierCtor, kSoldierCtorBytes); break;
    case 13: ok = bytes(base + kInputGetPose, kInputGetPoseBytes); break;
    case 14: ok = bytes(base + kInputCrouch, kInputCrouchBytes) && bytes(base + kInputCrouchRule, kInputCrouchRuleBytes); break;
    case 15: ok = bytes(base + kInputProne, kInputProneBytes); break;
    default: break;
    }
    if (at)
        *at = where;
    return ok;
}

// Every constant above, in order; the first not borne out named.
template <class Reader>
Proofs Prove(const Reader& read, uint32_t base) {
    Proofs p;
    for (int i = 0; i < kProofs; ++i) {
        uint32_t at = 0;
        if (!ProveOne(read, base, i, &at)) {
            p.failed = i;
            p.at = at;
            return p;
        }
    }
    p.ok = true;
    return p;
}

// ------------------------------------------------------------------ a frame's read ------------------------------------------------------------------

// Why a frame's read gave nothing (said once a run each, by the caller).
enum class Miss : uint8_t {
    kNone,
    kNoSoldier,    // no soldier of the player's (a menu, a load, a death)
    kSoldierType,  // the player's controlled object isn't a ClientSoldierEntity (a vehicle?), or not readable
    kNoPhysics,    // its physics not there, or not a CharacterPhysicsEntity
    kValues,       // the poses not 0-2, or the time not a time
    kData,         // its data not a CharacterPhysicsData
};
inline const char* MissName(Miss m) {
    switch (m) {
    case Miss::kNone: return "read";
    case Miss::kNoSoldier: return "no soldier of the player's";
    case Miss::kSoldierType: return "the player's soldier pointer isn't a ClientSoldierEntity";
    case Miss::kNoPhysics: return "the soldier's +0x164 isn't a CharacterPhysicsEntity";
    case Miss::kValues: return "the physics' poses aren't poses";
    case Miss::kData: return "the physics' data isn't a CharacterPhysicsData";
    }
    return "?";
}

struct Pose {
    bool ok = false;
    Miss miss = Miss::kNoSoldier;
    int current = kStand;  // +0x7C
    int target = kStand;   // +0x80: the pose the game's toggles act on
    float timer = 0.0f;    // +0x84
    bool allowed[3] = {true, true, true};  // +0x90..+0x92
    bool changeInTransition = true;        // the data's AllowPoseChangeDuringTransition
    uint32_t soldier = 0, physics = 0;
};

// One frame's read of the soldier at `soldier` (god.cpp's chain: [[[0x2380B58]+0x30]+0xBC]+0x3D8), every object checked by its vtable first.
template <class Reader>
Pose Read(const Reader& read, uint32_t base, uint32_t soldier) {
    Pose p;
    p.soldier = soldier;
    if (!soldier)
        return p;
    bool got = false;
    if (Word(read, soldier, &got) != base + kSoldierVtable || !got) {
        p.miss = Miss::kSoldierType;
        return p;
    }
    const uint32_t physics = Word(read, soldier + kSoldierPhysics, &got);
    if (!got || !physics || Word(read, physics, &got) != base + kPhysicsVtable || !got) {
        p.miss = Miss::kNoPhysics;
        return p;
    }
    p.physics = physics;
    uint8_t block[0x18] = {};  // +0x7C..+0x93
    if (!read(physics + kCurrent, block, sizeof(block))) {
        p.miss = Miss::kNoPhysics;
        return p;
    }
    int32_t current = 0, target = 0;
    float timer = 0.0f;
    std::memcpy(&current, block + (kCurrent - kCurrent), 4);
    std::memcpy(&target, block + (kTarget - kCurrent), 4);
    std::memcpy(&timer, block + (kTimer - kCurrent), 4);
    if (current < 0 || current > 2 || target < 0 || target > 2 || !std::isfinite(timer) || timer < -1.0f || timer > 30.0f) {
        p.miss = Miss::kValues;
        return p;
    }
    const uint32_t data = Word(read, physics + kData, &got);
    uint8_t change = 0;
    if (!got || !data || Word(read, data, &got) != base + kDataVtable || !got || !read(data + kDataChangeInTransition, &change, 1)) {
        p.miss = Miss::kData;
        return p;
    }
    p.current = current;
    p.target = target;
    p.timer = timer;
    for (int i = 0; i < 3; ++i)
        p.allowed[i] = block[kAllowed - kCurrent + i] != 0;
    p.changeInTransition = change != 0;
    p.ok = true;
    p.miss = Miss::kNone;
    return p;
}

// The pose's changes as the log says them (input.cpp's "Stance: the game's pose: crouched (was standing)"), from every read of one soldier: each new
// target (+0x80) with the one said before it, and whether that one was never reached: the soldier's own pose (+0x7C) never it in any read while it was
// the target (a change refused at its end, +0x80 put back; another asked for before it ended), and the pose the soldier stayed in meanwhile (the last read
// before the new target). The game sets +0x80 at a key's press and +0x7C as the change ends; a change made at once (a checkpoint's new soldier laid prone,
// the change's end and a new press between two reads) sets both, and the one before was reached all the same. The line had gone by the new read's own
// +0x7C, so every change seen with both already set said the one before was never reached and the soldier stayed in the NEW pose ("[16:48:44.860] Stance:
// the game's pose: standing (was prone, never reached: the soldier stayed standing)", build\logs\bf3vr_1125_p114b_part1.log, and 16:27:54 to 16:28:01).
struct Said {
    int target = -1;       // the target last said (-1: nothing said for this soldier yet)
    bool reached = false;  // the soldier's pose has been that target in a read since it was said
    int current = -1;      // the soldier's pose at the last read
};
struct Change {
    bool say = false;           // a new target: its line
    int target = kStand;        // the new target
    int was = kStand;           // the one said before it ...
    bool neverReached = false;  // ... never the soldier's pose while it was the target ...
    int stayed = kStand;        // ... the soldier staying in this meanwhile
};
// A soldier's first read (its line is its own: "a soldier's first read").
inline void SaidFirst(Said& s, int current, int target) {
    s.target = target;
    s.reached = current == target;
    s.current = current;
}
// Every read after it, of the same soldier.
inline Change SaidNext(Said& s, int current, int target) {
    Change c;
    if (s.target < 0) {
        SaidFirst(s, current, target);
        return c;
    }
    if (current == s.target)  // (this read's pose the old target: reached, its change's end and the new press between two reads)
        s.reached = true;
    if (target != s.target) {
        c.say = true;
        c.target = target;
        c.was = s.target;
        c.neverReached = !s.reached;
        c.stayed = s.current >= 0 ? s.current : current;
        s.target = target;
        s.reached = current == target;
    }
    s.current = current;
    return c;
}

}  // namespace poseread

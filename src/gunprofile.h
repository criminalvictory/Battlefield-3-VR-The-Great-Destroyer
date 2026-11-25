#pragma once
// One profile for whatever gun is in hand, for free hands on every gun (held by either hand, by its pistol grip or its front alone, let go of, thrown,
// lying, picked up): which of skin.cpp's poses it drives, where the game's own palms rest on it, its sights' up, its body once it's let go of, its knocks
// and its log words. Pure, header only: hands.cpp makes it once a frame from what skin.cpp knows of the gun in hand (Make), and tools/profiletest.cpp runs
// it without the game.
//   - Nothing is measured by hand: the numbers are those today's code already uses for that gun (skin.cpp's kHeldKinds palms, the game's own palms at
//     rest; the SMAW's grip and front grip; the M416's GunGrip). The M416's profile is today's numbers bit for bit, and its log words ("rifle",
//     "handguard") keep its lines as they were.
//   - The left hand's places are the right's mirrored across the gun's x = 0 plane: every grip was measured within 1.7 mm of it, so the mirrored palm
//     lands on the same grip from its other side.
//   - kNone is dormant: free hands off for it, today's code (the right hand holds it, nothing drops). So is anything not known well enough: a gun with no
//     body yet, a held gun with no id, a number not finite, a box with no size, two guns at once (a switch's frame), and with freehands.txt's "guns m416"
//     every gun but the M416.

#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstring>

#include "loose.h"

namespace gunprofile {

enum class Pose : uint8_t { kNone, kRifle, kLauncher };              // kRifle: HandTargets::gunPose (the M416); kLauncher: launcherPose (held guns, the SMAW)
enum class Sound : uint8_t { kMetal = 0, kPolymer = 1, kHeavy = 2 };  // gunbodies::Sound's numbers
constexpr uint32_t kRifleId = 12768, kSmawId = 3414;                 // their colour draws' index counts (skin.cpp kWeapon, kLauncherA)

struct Profile {
    Pose pose = Pose::kNone;
    uint32_t id = 0;                       // which gun: another id is another gun (back in the right hand)
    const char* name = "gun";              // the logs' word ("rifle" for the M416: its lines as before)
    const char* front = "handguard";       // where the support hand holds it, for the logs
    float anchor[2][3] = {};               // [1] where the game's right palm sits on it (model), [0] that mirrored
    float support[2][3] = {};              // a launcher-pose gun's front: [0] the game's left palm, [1] that mirrored
    float up = 0.0f;                       // degrees its sights' line lies below its z
    float right = 0.0f;                    // ... and right of its z (toward its -x): the AT4's 0.68 (its sights 6 cm left of its tube, set to meet
                                           // it); every other gun 0, and with 0 nothing that turns a gun by its up changes at all
    const loose::Spec* body = nullptr;     // its loose body
    float boxMin[3] = {}, boxMax[3] = {};  // its hull's box (a pickup's reach)
    Sound sound = Sound::kMetal;
    int slot = 0;         // the game's weapon slot it's in (0 the M416 and the held rifles, 1 the Stinger, 5 the SMAW): the pickups' lying slot
    int attachments = 0;  // of its body's hull points, how many are its attachments' (the profile's log line)
};

// What hands.cpp knows of the gun in hand this frame, from skin.cpp's reads.
struct In {
    bool rifle = false, launcher = false, held = false;  // exactly one, or none: kNone
    uint32_t heldId = 0;                                 // a held gun's colour draw's index count
    const char* heldShape = nullptr;                     // its shape (skin.cpp's HeldKind::shape: "an94", "fim92" ...)
    float rifleAnchor[3] = {}, rifleUp = 0.0f;           // the M416: where the game's right palm sits on it, its sights' up
    float launcherGrip[3] = {}, launcherFront[3] = {}, launcherUp = 0.0f;  // the SMAW or a held gun: the game's right and left palms on it, its up
    float launcherRight = 0.0f;                                            // ... and its sights' line right of its z (skin.cpp's HeldKind::right)
    const loose::Spec* body = nullptr;                   // its loose body (none yet: kNone)
    float bodyMin[3] = {}, bodyMax[3] = {};              // its hull's box
    int bodySound = 0;                                   // gunbodies::Sound
    int bodyAttachments = 0;                             // how many of the body's hull points are its attachments'
    bool everyGun = true;                                // freehands.txt "guns m416": false
};

// A point across the gun's x = 0 plane (in and out may be the same).
inline void MirrorPoint(const float in[3], float out[3]) {
    const float x = in[0];
    out[0] = -x;
    out[1] = in[1];
    out[2] = in[2];
}

// Not a NaN or an infinity (no comparison with a NaN holds): std::isfinite's answer, without the runtime library's call for each number.
inline bool Finite(float v) {
    return std::fabs(v) <= FLT_MAX;
}
inline bool FiniteAll(const float* v, int n) {
    for (int i = 0; i < n; ++i)
        if (!Finite(v[i]))
            return false;
    return true;
}

// The held guns known by name: each one's shape (skin.cpp's HeldKind::shape) and its word in the logs.
inline constexpr const char* kNames[][2] = {{"an94", "AN-94"}, {"rpk", "RPK-74M"}, {"aek971", "AEK-971"}, {"ak74", "AK-74M"}, {"g3a3", "G3A3"},
                                            {"kh2002", "KH2002"}, {"m16a4", "M16A4"}, {"m4a1", "M4A1"}, {"m240", "M240"}, {"fim92", "Stinger"},
                                            {"aks74u", "AKS-74u"}, {"ump45", "UMP-45"},  // (new guns after the Stinger: its row stays 9)
                                            {"m1014", "M1014"}, {"mk11", "Mk11"}, {"m249", "M249"}, {"at4", "AT4"},
                                            {"saiga12k", "Saiga 12K"}, {"pkp", "PKP"}};  // (after the AT4's: its row stays 15)
constexpr int kStingerRow = 9;  // the Stinger's row: held by its front grip, in slot 1
constexpr int kAt4Row = 15;     // the AT4's (Operation Swordbreaker's, given by the scene in the M4A1's slot 0): held by its folding front grip, its slot
                                // found by its name (hands.cpp's SlotOfProfile goes by name for every launcher)

// A held gun's row in kNames by its shape, -1 for one not known (or none).
inline int RowOf(const char* shape) {
    if (shape)
        for (int i = 0; i < static_cast<int>(sizeof(kNames) / sizeof(kNames[0])); ++i)
            if (kNames[i][0][0] == shape[0] && std::strcmp(kNames[i][0], shape) == 0)
                return i;
    return -1;
}

// A gun's turn so its sights look along the aim (hands.cpp's every place that turns a gun by its up): raised by up (its sights' line that many degrees
// under its z) and, for a gun whose sights' line also lies right of its z (the AT4's, 0.68 degrees: its sights 6 cm left of its tube, set to meet its
// line), turned that much about its y first: raise(up) . turn(right), row-major. With right 0 (every other gun) the raise alone, as every place built it
// before, bit for bit.
inline void SightsTurn(float upDegrees, float rightDegrees, float* out) {
    const float up = upDegrees * 3.14159265f / 180.0f;
    const float raise[9] = {1, 0, 0, 0, std::cos(up), std::sin(up), 0, -std::sin(up), std::cos(up)};
    if (rightDegrees == 0.0f || !(std::fabs(rightDegrees) <= FLT_MAX)) {
        std::memcpy(out, raise, sizeof(raise));
        return;
    }
    const float a = rightDegrees * 3.14159265f / 180.0f;
    const float turn[9] = {std::cos(a), 0, std::sin(a), 0, 1, 0, -std::sin(a), 0, std::cos(a)};  // its sights' line (toward -x by a) onto its z
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            out[i * 3 + j] = raise[i * 3] * turn[j] + raise[i * 3 + 1] * turn[3 + j] + raise[i * 3 + 2] * turn[6 + j];
}

// A held gun's name in the logs, by its shape: "gun" for one not known (or none).
inline const char* NameOf(const char* shape) {
    const int row = RowOf(shape);
    return row >= 0 ? kNames[row][1] : "gun";
}

// The profile of the gun in hand, or kNone (dormant). The M416 keeps its numbers whatever "guns" says; the SMAW and a held gun need every gun on, and
// a held gun its id. Every pointer in it is to static data (the body, the words): nothing points into In.
inline Profile Make(const In& in) {
    Profile p;  // (the one object every path returns: built where the caller keeps it, not copied out)
    const int kinds = (in.rifle ? 1 : 0) + (in.launcher ? 1 : 0) + (in.held ? 1 : 0);
    if (kinds != 1 || !in.body || (!in.rifle && (!in.everyGun || (in.held && !in.heldId))))
        return p;
    if (in.rifle) {
        p.pose = Pose::kRifle, p.id = kRifleId, p.name = "rifle", p.front = "handguard";
        std::memcpy(p.anchor[1], in.rifleAnchor, sizeof(p.anchor[1]));
        p.up = in.rifleUp;
    } else {
        const int row = in.held ? RowOf(in.heldShape) : -1;
        const bool stinger = row == kStingerRow, at4 = row == kAt4Row;
        p.pose = Pose::kLauncher;
        p.id = in.launcher ? kSmawId : in.heldId;
        p.name = in.launcher ? "launcher" : row >= 0 ? kNames[row][1] : "gun";
        p.front = in.launcher || stinger || at4 ? "front grip" : "handguard";
        p.slot = in.launcher ? 5 : stinger ? 1 : 0;  // (the AT4's 0 is Swordbreaker's kit's; its slot is found by its name all the same)
        std::memcpy(p.anchor[1], in.launcherGrip, sizeof(p.anchor[1]));
        std::memcpy(p.support[0], in.launcherFront, sizeof(p.support[0]));
        MirrorPoint(p.support[0], p.support[1]);
        p.up = in.launcherUp;
        p.right = in.held ? in.launcherRight : 0.0f;
    }
    MirrorPoint(p.anchor[1], p.anchor[0]);
    p.body = in.body;
    std::memcpy(p.boxMin, in.bodyMin, sizeof(p.boxMin));
    std::memcpy(p.boxMax, in.bodyMax, sizeof(p.boxMax));
    p.sound = in.bodySound == 1 ? Sound::kPolymer : in.bodySound == 2 ? Sound::kHeavy : Sound::kMetal;
    p.attachments = in.bodyAttachments;
    bool ok = FiniteAll(&p.anchor[0][0], 6) && FiniteAll(&p.support[0][0], 6) && Finite(p.up) && Finite(p.right) && FiniteAll(p.boxMin, 3) &&
              FiniteAll(p.boxMax, 3);
    for (int i = 0; i < 3; ++i)
        ok = ok && p.boxMin[i] < p.boxMax[i];  // (a box with no size: a pickup's reach would be measured to a point)
    if (!ok)
        p = Profile{};
    return p;
}

// A change of gun (another id) while free hands are on, before the grasp runs: which hand the new gun goes into.
//   kReturn  the gun before wasn't held by a pistol grip (it lay, flew into a hand from the ground, or was held by its front alone): the grasp gets this
//            one frame off, so it's returned to the right hand and what lay is cleaned up
//   kLeft    the left held the gun before by its pistol grip in the last frame free hands were on, under kKeepSideMs before, it's seen now, and it isn't
//            a new life: the new gun stays in the left (its trigger fires once it's been let go, as after a take)
//   kRight   anything else: the right hand, as at a spawn (today's after a switch)
// lastHeldSide: the pistol-grip hand in the last frame free hands were on (-1 none); sinceHeldMs: how long ago that frame was.
enum class Change : uint8_t { kRight, kLeft, kReturn };
constexpr uint64_t kKeepSideMs = 2000;
inline Change OnChange(bool byPistolGrip, int lastHeldSide, uint64_t sinceHeldMs, bool leftSeen, bool newLife) {
    if (!byPistolGrip)
        return Change::kReturn;
    return lastHeldSide == 0 && sinceHeldMs < kKeepSideMs && leftSeen && !newLife ? Change::kLeft : Change::kRight;
}

}  // namespace gunprofile

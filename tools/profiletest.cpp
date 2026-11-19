// The profile of whatever gun is in hand (src/gunprofile.h, as hands.cpp makes it once a frame), without the game. Each gun's row is built from the
// numbers hands.cpp gives it (skin.cpp's kHeldKinds palms and ups, the SMAW's grip and front grip, the M416's GunGrip and RifleUp, a body and its
// hull's box), and every field of what comes back is checked against the same numbers worked out here:
//   p1  the M416: today's numbers bit for bit: anchor[1] the GunGrip given, anchor[0] it mirrored as hands.cpp mirrors it (-x), up the RifleUp given,
//       its body gunbody::kM416 and its hull's box (kM416HullMin/Max), "rifle", "handguard", id 12768, slot 0, metal, no front numbers; with
//       another body given (the M416 without its M320) only the body differs
//   p2  the AN-94 from its palms: the right's anchor and the left's front, each with its mirror, "AN-94", "handguard", metal (no gun's knock is
//       plastic), id 16188, slot 0, its attachments' count; and (p2b) every held gun the same way, the Stinger alone with a "front grip"
//   p3  the Stinger: "Stinger", "front grip", heavy, slot 1
//   p4  the SMAW: kLauncher, id 3414, "launcher", "front grip", heavy, slot 5
//   p5  "guns m416" (everyGun false): the M416 bit for bit as with every gun; the SMAW and every held gun dormant
//   p6  dormant (every field a default Profile's) for: no body; a held gun with no id; a number not finite (NaN, either infinity) in anything the
//       profile uses; a box with no size or inside out; two guns at once, three, none. A number not finite in what it doesn't use changes nothing.
//   p7  NameOf for every shape, "gun" for none, an unknown or a near miss; a held gun with no shape held as "gun"; MirrorPoint twice is the
//       identity bit for bit (in place too); the words point at static text, never into what was given; Finite gives std::isfinite's answer
//   p8  a million Makes in under 50 ms (the best of five runs)
//   p9  a change of gun (OnChange): the left stays the trigger hand across a 0.5 s gap, not across 2.5 s (2 s the edge), not from a hold by
//       the front alone, lying or flying (those go back: kReturn), not at a new life, not with the left unseen; the right is never kept by it
// Usage: profiletest
// The exit code is the number of failed checks.

#include <windows.h>

#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <utility>

#include "../src/gunbody.h"
#include "../src/gunprofile.h"

namespace {

using gunprofile::In;
using gunprofile::Pose;
using gunprofile::Profile;
using gunprofile::Sound;

int g_failed = 0, g_counted = 0;

void Report(const char* id, const char* what, bool pass, const char* format, ...) {
    char numbers[1536];
    va_list args;
    va_start(args, format);
    std::vsnprintf(numbers, sizeof(numbers), format, args);
    va_end(args);
    std::printf("%-3s %-5s %s | %s\n", id, pass ? "PASS" : "FAIL", what, numbers);
    ++g_counted;
    g_failed += !pass;
}

// ---- What hands.cpp gives Make ----
// The M416: the game's right palm on it this run (skin::GunGrip) and its sights' up (skin::RifleUp).
constexpr float kGunGrip[3] = {-0.063f, -0.091f, 0.224f};
constexpr float kRifleUp = 0.2865f;
// The SMAW: its rear grip (the game's right palm), its front grip (the left's), its up (skin::LauncherUp). hands.cpp sets these every frame,
// whatever is in hand.
constexpr float kSmawGrip[3] = {-0.026f, -0.141f, 0.061f}, kSmawFront[3] = {0.039f, -0.073f, 0.315f};
constexpr float kSmawUp = 0.7f;

// Every held gun as skin.cpp's kHeldKinds has it (its colour draw's index count, its up, the game's palms at rest), its body's knock as the bodies'
// table gives it (metal; heavy for the M240 and the Stinger), and what its profile should say.
struct HeldRow {
    const char* shape;
    uint32_t id;
    float up;
    float rightPalm[3], leftPalm[3];
    int sound;
    const char* name;
    const char* front;
    int slot;
    float right = 0.0f;  // its sights' line right of its z (skin.cpp's HeldKind::right: the AT4's alone)
};
constexpr HeldRow kHeld[] = {
    {"an94", 16188, 0.0f, {-0.023f, -0.059f, 0.192f}, {0.002f, -0.018f, 0.561f}, 0, "AN-94", "handguard", 0},
    {"rpk", 24492, 0.0f, {-0.032f, -0.080f, 0.192f}, {-0.001f, -0.029f, 0.585f}, 0, "RPK-74M", "handguard", 0},
    {"aek971", 21585, 0.21f, {-0.024f, -0.079f, 0.208f}, {0.004f, -0.031f, 0.551f}, 0, "AEK-971", "handguard", 0},
    {"ak74", 21723, 0.03f, {-0.032f, -0.080f, 0.193f}, {0.001f, -0.031f, 0.548f}, 0, "AK-74M", "handguard", 0},
    {"g3a3", 23064, 0.48f, {-0.034f, -0.082f, 0.241f}, {0.020f, -0.007f, 0.616f}, 0, "G3A3", "handguard", 0},
    {"kh2002", 15237, -0.47f, {-0.025f, -0.099f, 0.234f}, {0.016f, -0.059f, 0.447f}, 0, "KH2002", "handguard", 0},
    {"m16a4", 17013, 0.0f, {-0.054f, -0.123f, 0.256f}, {0.014f, -0.041f, 0.636f}, 0, "M16A4", "handguard", 0},
    {"m4a1", 22194, 0.0f, {-0.054f, -0.114f, 0.255f}, {0.012f, -0.041f, 0.612f}, 0, "M4A1", "handguard", 0},
    {"m240", 27588, 0.22f, {-0.023f, -0.115f, 0.196f}, {0.018f, -0.065f, 0.678f}, 2, "M240", "handguard", 0},
    {"fim92", 20298, 0.0f, {-0.046f, -0.132f, 0.593f}, {0.025f, -0.146f, 0.993f}, 2, "Stinger", "front grip", 1},
    // Semper Fidelis's two (2026-11-18), after the Stinger: its row stays 9.
    {"aks74u", 17991, -0.42f, {-0.031f, -0.080f, 0.193f}, {0.003f, -0.035f, 0.517f}, 0, "AKS-74u", "handguard", 0},
    {"ump45", 19314, 0.05f, {-0.053f, -0.066f, 0.223f}, {0.004f, -0.026f, 0.538f}, 0, "UMP-45", "handguard", 0},
    {"m1014", 13377, 0.0f, {-0.038f, -0.030f, 0.217f}, {-0.017f, 0.020f, 0.660f}, 0, "M1014", "handguard", 0},
    // Operation Swordbreaker's Mk11 (2026-11-18).
    {"mk11", 12456, 0.0f, {-0.054f, -0.114f, 0.254f}, {0.012f, -0.041f, 0.612f}, 0, "Mk11", "handguard", 0},
    // Operation Swordbreaker's AT4 (2026-11-18): held by its folding front grip, its sights' line 0.47 under and 0.68 right of its z; its body's knock
    // the hollow plastic's (a fibreglass tube); its slot 0 the kit's (found by its name in the game: hands.cpp's SlotOfProfile).
    {"at4", 15987, 0.47f, {-0.091f, 0.023f, 0.409f}, {0.037f, -0.047f, 0.883f}, 1, "AT4", "front grip", 0, 0.68f},
};
constexpr int kHeldCount = static_cast<int>(sizeof(kHeld) / sizeof(kHeld[0]));
constexpr int kAn94 = 0, kM240 = 8, kStinger = 9;

// Their bodies here: each its own (only which one, and its hull's box, reach the profile), the SMAW's last.
loose::Spec g_body[kHeldCount + 1];
float g_hullMin[kHeldCount + 1][3], g_hullMax[kHeldCount + 1][3];
char g_bodyName[kHeldCount + 1][24];
// The M416 without its M320 (a later level's): another body, the same box.
loose::Spec g_m416Bare = gunbody::kM416;

void MakeBodies() {
    for (int k = 0; k <= kHeldCount; ++k) {
        loose::Spec s{};
        std::snprintf(g_bodyName[k], sizeof(g_bodyName[k]), "%sgun", k < kHeldCount ? kHeld[k].shape : "smaw");
        s.name = g_bodyName[k];
        const float f = static_cast<float>(k);
        const float boxMin[3] = {-0.035f - 0.001f * f, -0.20f, -0.22f}, boxMax[3] = {0.035f, 0.08f + 0.001f * f, 0.85f + 0.01f * f};
        std::memcpy(s.boxMin, boxMin, sizeof(boxMin));
        std::memcpy(s.boxMax, boxMax, sizeof(boxMax));
        s.massEmpty = 3.0f + f;
        s.capacity = 30;
        s.bounce = 1.0f;
        s.keep = true;
        g_body[k] = s;
        const float hullMin[3] = {-0.05f - 0.001f * f, -0.30f, -0.25f}, hullMax[3] = {0.05f, 0.12f + 0.001f * f, 0.95f + 0.01f * f};
        std::memcpy(g_hullMin[k], hullMin, sizeof(hullMin));
        std::memcpy(g_hullMax[k], hullMax, sizeof(hullMax));
    }
}

void Copy3(float* out, const float* in) {
    std::memcpy(out, in, 3 * sizeof(float));
}

// What every frame gives whatever is in hand (the M416's anchor and up, the SMAW's numbers unless a held gun's palms replace them).
In Base() {
    In in;
    Copy3(in.rifleAnchor, kGunGrip);
    in.rifleUp = kRifleUp;
    Copy3(in.launcherGrip, kSmawGrip);
    Copy3(in.launcherFront, kSmawFront);
    in.launcherUp = kSmawUp;
    return in;
}
In RifleIn() {
    In in = Base();
    in.rifle = true;
    in.body = &gunbody::kM416;
    Copy3(in.bodyMin, gunbody::kM416HullMin);
    Copy3(in.bodyMax, gunbody::kM416HullMax);
    in.bodySound = 0;
    return in;
}
In SmawIn() {
    In in = Base();
    in.launcher = true;
    in.body = &g_body[kHeldCount];
    Copy3(in.bodyMin, g_hullMin[kHeldCount]);
    Copy3(in.bodyMax, g_hullMax[kHeldCount]);
    in.bodySound = 2;
    return in;
}
In HeldIn(int k) {
    const HeldRow& r = kHeld[k];
    In in = Base();
    in.held = true;
    in.heldId = r.id;
    in.heldShape = r.shape;
    Copy3(in.launcherGrip, r.rightPalm);
    Copy3(in.launcherFront, r.leftPalm);
    in.launcherUp = r.up;
    in.launcherRight = r.right;
    in.body = &g_body[k];
    Copy3(in.bodyMin, g_hullMin[k]);
    Copy3(in.bodyMax, g_hullMax[k]);
    in.bodySound = r.sound;
    return in;
}

// ---- What should come back, worked out here ----
void Mirrored(const float* in, float* out) {
    out[0] = -in[0], out[1] = in[1], out[2] = in[2];
}
Profile WantRifle(const loose::Spec* body) {
    Profile w;
    w.pose = Pose::kRifle, w.id = 12768, w.name = "rifle", w.front = "handguard";
    Copy3(w.anchor[1], kGunGrip);
    Mirrored(kGunGrip, w.anchor[0]);  // (hands.cpp's own: -anchor[0], anchor[1], anchor[2])
    w.up = kRifleUp;
    w.body = body;
    Copy3(w.boxMin, gunbody::kM416HullMin);
    Copy3(w.boxMax, gunbody::kM416HullMax);
    w.sound = Sound::kMetal, w.slot = 0;
    return w;
}
Profile WantSmaw() {
    Profile w;
    w.pose = Pose::kLauncher, w.id = 3414, w.name = "launcher", w.front = "front grip";
    Copy3(w.anchor[1], kSmawGrip);
    Mirrored(kSmawGrip, w.anchor[0]);
    Copy3(w.support[0], kSmawFront);
    Mirrored(kSmawFront, w.support[1]);
    w.up = kSmawUp;
    w.body = &g_body[kHeldCount];
    Copy3(w.boxMin, g_hullMin[kHeldCount]);
    Copy3(w.boxMax, g_hullMax[kHeldCount]);
    w.sound = Sound::kHeavy, w.slot = 5;
    return w;
}
Profile WantHeld(int k) {
    const HeldRow& r = kHeld[k];
    Profile w;
    w.pose = Pose::kLauncher, w.id = r.id, w.name = r.name, w.front = r.front;
    Copy3(w.anchor[1], r.rightPalm);
    Mirrored(r.rightPalm, w.anchor[0]);
    Copy3(w.support[0], r.leftPalm);
    Mirrored(r.leftPalm, w.support[1]);
    w.up = r.up;
    w.right = r.right;
    w.body = &g_body[k];
    Copy3(w.boxMin, g_hullMin[k]);
    Copy3(w.boxMax, g_hullMax[k]);
    w.sound = r.sound == 2 ? Sound::kHeavy : r.sound == 1 ? Sound::kPolymer : Sound::kMetal, w.slot = r.slot;
    return w;
}

// Bit for bit (a -0 isn't a 0; a NaN is its own bits).
bool Same(const float* a, const float* b, int n) {
    return std::memcmp(a, b, sizeof(float) * n) == 0;
}
bool SameText(const char* a, const char* b) {
    return a && b && std::strcmp(a, b) == 0;
}
const char* PoseName(Pose p) {
    return p == Pose::kNone ? "kNone" : p == Pose::kRifle ? "kRifle" : p == Pose::kLauncher ? "kLauncher" : "?";
}
const char* SoundName(Sound s) {
    return s == Sound::kMetal ? "metal" : s == Sound::kPolymer ? "polymer" : s == Sound::kHeavy ? "heavy" : "?";
}

// The fields of got that aren't want's, named (empty: none).
struct Diff {
    char text[1024] = {};
    int count = 0;
    void Add(bool same, const char* field) {
        if (same)
            return;
        const size_t used = std::strlen(text);
        std::snprintf(text + used, sizeof(text) - used, "%s%s", count ? ", " : "", field);
        ++count;
    }
};
Diff Compare(const Profile& got, const Profile& want) {
    Diff d;
    d.Add(got.pose == want.pose, "pose");
    d.Add(got.id == want.id, "id");
    d.Add(SameText(got.name, want.name), "name");
    d.Add(SameText(got.front, want.front), "front");
    d.Add(Same(got.anchor[1], want.anchor[1], 3), "anchor[1]");
    d.Add(Same(got.anchor[0], want.anchor[0], 3), "anchor[0]");
    d.Add(Same(got.support[0], want.support[0], 3), "support[0]");
    d.Add(Same(got.support[1], want.support[1], 3), "support[1]");
    d.Add(Same(&got.up, &want.up, 1), "up");
    d.Add(Same(&got.right, &want.right, 1), "right");
    d.Add(got.body == want.body, "body");
    d.Add(Same(got.boxMin, want.boxMin, 3), "boxMin");
    d.Add(Same(got.boxMax, want.boxMax, 3), "boxMax");
    d.Add(got.sound == want.sound, "sound");
    d.Add(got.slot == want.slot, "slot");
    d.Add(got.attachments == want.attachments, "attachments");
    return d;
}
bool Dormant(const Profile& p) {
    return Compare(p, Profile{}).count == 0;
}
// One line of what a profile says.
const char* Describe(const Profile& p) {
    static char text[4][256];
    static int at = 0;
    char* out = text[at++ & 3];
    std::snprintf(out, sizeof(text[0]), "%s id %u \"%s\" \"%s\" slot %d %s, anchor[1] (%.3f %.3f %.3f) [0] (%.3f %.3f %.3f), up %.4f, body %s",
                  PoseName(p.pose), p.id, p.name ? p.name : "(null)", p.front ? p.front : "(null)", p.slot, SoundName(p.sound), p.anchor[1][0],
                  p.anchor[1][1], p.anchor[1][2], p.anchor[0][0], p.anchor[0][1], p.anchor[0][2], p.up,
                  p.body && p.body->name ? p.body->name : "none");
    return out;
}

// ---- 1. The M416 ----
void CheckP1() {
    const Profile p = gunprofile::Make(RifleIn());
    const Diff d = Compare(p, WantRifle(&gunbody::kM416));
    const Profile none;
    const bool zeroFront = Same(&p.support[0][0], &none.support[0][0], 6);
    Report("p1", "the M416: today's numbers bit for bit", d.count == 0 && zeroFront && p.id == gunprofile::kRifleId,
           d.count ? "off: %s" : "%s; its front numbers all 0 (Support's own line holds it)", d.count ? d.text : Describe(p));
    In bare = RifleIn();
    bare.body = &g_m416Bare;
    const Profile q = gunprofile::Make(bare);
    const Diff e = Compare(q, WantRifle(&g_m416Bare));
    Report("p1b", "the M416 with another body (without its M320): only the body differs", e.count == 0,
           e.count ? "off: %s" : "body %s at another place, the rest as p1", e.count ? e.text : q.body->name);
}

// ---- 2. The AN-94, and every held gun ----
void CheckP2() {
    In in = HeldIn(kAn94);
    in.bodyAttachments = 3;  // (a sight's points in its hull)
    const Profile p = gunprofile::Make(in);
    Profile want = WantHeld(kAn94);
    want.attachments = 3;
    const Diff d = Compare(p, want);
    const bool mirrors = p.anchor[0][0] == -p.anchor[1][0] && p.support[1][0] == -p.support[0][0] && p.anchor[0][1] == p.anchor[1][1] &&
                         p.anchor[0][2] == p.anchor[1][2] && p.support[1][1] == p.support[0][1] && p.support[1][2] == p.support[0][2];
    Report("p2", "the AN-94 from its palms, each mirrored", d.count == 0 && mirrors && p.sound == Sound::kMetal,
           d.count ? "off: %s" : "%s; front [0] (%.3f %.3f %.3f) [1] (%.3f %.3f %.3f), %d attachment points", d.count ? d.text : Describe(p),
           p.support[0][0], p.support[0][1], p.support[0][2], p.support[1][0], p.support[1][1], p.support[1][2], p.attachments);
    int good = 0;
    char bad[512] = {};
    for (int k = 0; k < kHeldCount; ++k) {
        const Diff e = Compare(gunprofile::Make(HeldIn(k)), WantHeld(k));
        if (e.count == 0)
            ++good;
        else {
            const size_t used = std::strlen(bad);
            std::snprintf(bad + used, sizeof(bad) - used, "%s%s: %s", used ? "; " : "", kHeld[k].shape, e.text);
        }
    }
    Report("p2b", "every held gun from its palms (the Stinger alone a front grip)", good == kHeldCount, good == kHeldCount ? "%d of %d, e.g. %s" : "%d of %d; %s",
           good, kHeldCount, good == kHeldCount ? Describe(gunprofile::Make(HeldIn(kM240))) : bad);
}

// ---- 3. The Stinger ----
void CheckP3() {
    const Profile p = gunprofile::Make(HeldIn(kStinger));
    const Diff d = Compare(p, WantHeld(kStinger));
    Report("p3", "the Stinger: its front grip, heavy, slot 1",
           d.count == 0 && SameText(p.name, "Stinger") && SameText(p.front, "front grip") && p.sound == Sound::kHeavy && p.slot == 1,
           d.count ? "off: %s" : "%s", d.count ? d.text : Describe(p));
    // The AT4 (Operation Swordbreaker's): its front grip, its sights' line right of its z carried (every other gun's right 0), a fibreglass tube's knock.
    const int at4 = kHeldCount - 1;
    const Profile a = gunprofile::Make(HeldIn(at4));
    const Diff da = Compare(a, WantHeld(at4));
    bool othersZero = true;
    for (int k = 0; k < kHeldCount - 1; ++k)
        othersZero = othersZero && gunprofile::Make(HeldIn(k)).right == 0.0f;
    Report("p3b", "the AT4: its front grip, its sights' line 0.68 degrees right of its z (every other gun's 0), a polymer knock, row 14",
           da.count == 0 && SameText(a.name, "AT4") && SameText(a.front, "front grip") && a.right == 0.68f && a.up == 0.47f && a.sound == Sound::kPolymer &&
               othersZero && gunprofile::RowOf("at4") == gunprofile::kAt4Row && gunprofile::kAt4Row == 14,
           da.count ? "off: %s" : "%s, right %.2f", da.count ? da.text : Describe(a), a.right);
}

// ---- 4. The SMAW ----
void CheckP4() {
    const Profile p = gunprofile::Make(SmawIn());
    const Diff d = Compare(p, WantSmaw());
    Report("p4", "the SMAW: id 3414, the launcher, its front grip, heavy, slot 5",
           d.count == 0 && p.id == gunprofile::kSmawId && SameText(p.name, "launcher") && SameText(p.front, "front grip") && p.sound == Sound::kHeavy &&
               p.slot == 5,
           d.count ? "off: %s" : "%s", d.count ? d.text : Describe(p));
}

// ---- 5. "guns m416" ----
void CheckP5() {
    In rifle = RifleIn();
    rifle.everyGun = false;
    const Diff d = Compare(gunprofile::Make(rifle), gunprofile::Make(RifleIn()));
    int dormant = 0;
    In smaw = SmawIn();
    smaw.everyGun = false;
    dormant += Dormant(gunprofile::Make(smaw));
    for (int k = 0; k < kHeldCount; ++k) {
        In held = HeldIn(k);
        held.everyGun = false;
        dormant += Dormant(gunprofile::Make(held));
    }
    Report("p5", "guns m416: the M416 as before, every other gun dormant", d.count == 0 && dormant == kHeldCount + 1,
           "the M416 %s; %d of %d others dormant", d.count ? d.text : "bit for bit the same", dormant, kHeldCount + 1);
}

// ---- 6. Dormant ----
void CheckP6() {
    const float nan = std::numeric_limits<float>::quiet_NaN(), inf = std::numeric_limits<float>::infinity();
    const float bad[3] = {nan, inf, -inf};
    // a. No body.
    {
        In a = RifleIn(), b = SmawIn(), c = HeldIn(kAn94);
        a.body = b.body = c.body = nullptr;
        const int n = Dormant(gunprofile::Make(a)) + Dormant(gunprofile::Make(b)) + Dormant(gunprofile::Make(c));
        Report("p6a", "no body: dormant", n == 3, "%d of 3 (the M416, the SMAW, the AN-94)", n);
    }
    // b. A held gun with no id.
    {
        In a = HeldIn(kAn94), b = HeldIn(kStinger);
        a.heldId = b.heldId = 0;
        const int n = Dormant(gunprofile::Make(a)) + Dormant(gunprofile::Make(b));
        Report("p6b", "a held gun with no id: dormant", n == 2, "%d of 2", n);
    }
    // c. A number not finite in what the profile uses: each in turn, each of NaN and both infinities.
    {
        int cases = 0, dormant = 0;
        char first[128] = {};
        auto run = [&](In& in, float* field, const char* what) {  // (field is one of in's)
            for (float b : bad) {
                *field = b;
                ++cases;
                if (Dormant(gunprofile::Make(in)))
                    ++dormant;
                else if (!first[0])
                    std::snprintf(first, sizeof(first), "%s = %g kept", what, b);
            }
        };
        for (int i = 0; i < 3; ++i) {
            {
                In in = RifleIn();
                run(in, &in.rifleAnchor[i], "the M416's anchor");
            }
            for (const In& base : {RifleIn(), SmawIn(), HeldIn(kAn94)}) {
                In a = base, b = base;
                run(a, &a.bodyMin[i], "a box's min");
                run(b, &b.bodyMax[i], "a box's max");
            }
            for (const In& base : {SmawIn(), HeldIn(kAn94), HeldIn(kStinger)}) {
                In a = base, b = base;
                run(a, &a.launcherGrip[i], "a launcher-pose grip");
                run(b, &b.launcherFront[i], "a launcher-pose front");
            }
        }
        {
            In a = RifleIn();
            run(a, &a.rifleUp, "the M416's up");
            In b = SmawIn();
            run(b, &b.launcherUp, "the SMAW's up");
            In c = HeldIn(kM240);
            run(c, &c.launcherUp, "the M240's up");
        }
        Report("p6c", "a number not finite in what it uses: dormant", dormant == cases, dormant == cases ? "%d of %d" : "%d of %d; %s", dormant, cases,
               first);
    }
    // d. A number not finite in what it doesn't use: nothing changes (the M416 ignores the launcher's numbers, a held gun the M416's).
    {
        int cases = 0, same = 0;
        const Profile rifle = gunprofile::Make(RifleIn()), smaw = gunprofile::Make(SmawIn()), an94 = gunprofile::Make(HeldIn(kAn94));
        for (float b : bad) {
            In r = RifleIn();
            for (int i = 0; i < 3; ++i)
                r.launcherGrip[i] = r.launcherFront[i] = b;
            r.launcherUp = b;
            In s = SmawIn(), h = HeldIn(kAn94);
            for (int i = 0; i < 3; ++i)
                s.rifleAnchor[i] = h.rifleAnchor[i] = b;
            s.rifleUp = h.rifleUp = b;
            cases += 3;
            same += Compare(gunprofile::Make(r), rifle).count == 0;
            same += Compare(gunprofile::Make(s), smaw).count == 0;
            same += Compare(gunprofile::Make(h), an94).count == 0;
        }
        Report("p6d", "a number not finite in what it doesn't use: nothing changes", same == cases, "%d of %d the same bit for bit", same, cases);
    }
    // e. A box with no size, or inside out, along any axis.
    {
        int cases = 0, dormant = 0;
        for (const In& base : {RifleIn(), SmawIn(), HeldIn(kAn94)})
            for (int i = 0; i < 3; ++i) {
                In flat = base, out = base;
                flat.bodyMax[i] = flat.bodyMin[i];
                std::swap(out.bodyMin[i], out.bodyMax[i]);
                cases += 2;
                dormant += Dormant(gunprofile::Make(flat)) + Dormant(gunprofile::Make(out));
            }
        Report("p6e", "a box with no size or inside out: dormant", dormant == cases, "%d of %d", dormant, cases);
    }
    // f. Two guns at once (a switch's frame), all three, none.
    {
        In rl = RifleIn(), rh = RifleIn(), lh = SmawIn(), all = RifleIn(), none = RifleIn();
        rl.launcher = true;
        rh.held = true, rh.heldId = kHeld[kAn94].id, rh.heldShape = kHeld[kAn94].shape;
        lh.held = true, lh.heldId = kHeld[kAn94].id, lh.heldShape = kHeld[kAn94].shape;
        all.launcher = all.held = true, all.heldId = kHeld[kAn94].id;
        none.rifle = false;
        const int n = Dormant(gunprofile::Make(rl)) + Dormant(gunprofile::Make(rh)) + Dormant(gunprofile::Make(lh)) + Dormant(gunprofile::Make(all)) +
                      Dormant(gunprofile::Make(none)) + Dormant(gunprofile::Make(In{}));
        Report("p6f", "two guns at once, three, none: dormant", n == 6, "%d of 6", n);
    }
    // g. What dormant says in the logs (never a null word).
    {
        const Profile p;
        Report("p6g", "a dormant profile's words", SameText(p.name, "gun") && SameText(p.front, "handguard") && p.body == nullptr && p.id == 0,
               "\"%s\" \"%s\", id %u, no body", p.name, p.front, p.id);
    }
}

// ---- 7. Names, mirrors, static words ----
void CheckP7() {
    int named = 0;
    for (const HeldRow& r : kHeld)
        named += SameText(gunprofile::NameOf(r.shape), r.name);
    const char* misses[] = {nullptr, "", "m416", "smaw", "AN94", "an9", "an944", "fim92 ", "knife"};
    int gun = 0;
    for (const char* m : misses)
        gun += SameText(gunprofile::NameOf(m), "gun");
    const int missCount = static_cast<int>(sizeof(misses) / sizeof(misses[0]));
    Report("p7a", "NameOf every shape; \"gun\" for anything else", named == kHeldCount && gun == missCount, "%d of %d named, %d of %d \"gun\"", named,
           kHeldCount, gun, missCount);

    // A held gun with no shape known (a gun learned later): held as any other, its words "gun" and "handguard".
    In in = HeldIn(kAn94);
    in.heldShape = nullptr;
    const Profile p = gunprofile::Make(in);
    Profile want = WantHeld(kAn94);
    want.name = "gun";
    const Diff d = Compare(p, want);
    Report("p7b", "a held gun with no shape: held, as \"gun\" by its \"handguard\"", d.count == 0, d.count ? "off: %s" : "%s", d.count ? d.text : Describe(p));

    // MirrorPoint twice: the identity, bit for bit, -0, the smallest and largest numbers and a NaN among them; in place the same.
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float points[][3] = {{0.0f, -0.0f, 0.0f},    {-0.0f, 0.0f, -0.0f},  {1e-45f, -1e-45f, 1e-38f}, {-3.4e38f, 3.4e38f, -1.0f},
                               {-0.063f, -0.091f, 0.224f}, {0.025f, -0.146f, 0.993f}, {nan, 0.5f, -nan}};
    int twice = 0, inPlace = 0, once = 0;
    for (const auto& pt : points) {
        float a[3], b[3], c[3];
        gunprofile::MirrorPoint(pt, a);
        gunprofile::MirrorPoint(a, b);
        twice += Same(b, pt, 3);
        float neg = -pt[0];
        once += Same(&a[0], &neg, 1) && Same(&a[1], &pt[1], 2);
        Copy3(c, pt);
        gunprofile::MirrorPoint(c, c);
        inPlace += Same(c, a, 3);
        gunprofile::MirrorPoint(c, c);
        inPlace += Same(c, pt, 3);
    }
    const int pointCount = static_cast<int>(sizeof(points) / sizeof(points[0]));
    Report("p7c", "MirrorPoint: x negated alone; twice the identity, in place too", twice == pointCount && once == pointCount && inPlace == 2 * pointCount,
           "%d of %d once, %d of %d twice, %d of %d in place", once, pointCount, twice, pointCount, inPlace, 2 * pointCount);

    // The words point at static text: the shape given from a buffer that's then written over leaves them as they were.
    char shape[16] = "an94", unknown[16] = "zz9";
    In fromBuffer = HeldIn(kAn94), fromUnknown = HeldIn(kAn94);
    fromBuffer.heldShape = shape, fromUnknown.heldShape = unknown;
    const Profile a = gunprofile::Make(fromBuffer), b = gunprofile::Make(fromUnknown);
    std::memset(shape, 'x', sizeof(shape) - 1);
    std::memset(unknown, 'x', sizeof(unknown) - 1);
    auto outside = [](const char* word, const char* buffer) { return word < buffer || word >= buffer + 16; };
    const bool ok = SameText(a.name, "AN-94") && SameText(a.front, "handguard") && SameText(b.name, "gun") && outside(a.name, shape) &&
                    outside(a.front, shape) && outside(b.name, unknown) && outside(b.front, unknown);
    Report("p7d", "the words are static text, never what was given", ok, "\"%s\" \"%s\" and \"%s\" \"%s\" after the shapes' buffers were written over", a.name,
           a.front, b.name, b.front);

    // Finite (the exponent's bits) gives std::isfinite's answer: every 65521st bit pattern, and the edges (zeros, the smallest and largest numbers,
    // both infinities, quiet and signalling NaNs of either sign).
    const uint32_t edges[] = {0x00000000u, 0x80000000u, 0x00000001u, 0x807fffffu, 0x00800000u, 0x7f7fffffu, 0xff7fffffu, 0x7f800000u,
                              0xff800000u, 0x7fc00000u, 0xffc00000u, 0x7f800001u, 0xff800001u, 0x7fbfffffu, 0x3f800000u, 0xbf800000u};
    int checked = 0, agree = 0;
    uint32_t firstOff = 0;
    auto check = [&](uint32_t bits) {
        float v;
        std::memcpy(&v, &bits, sizeof(v));
        ++checked;
        if (gunprofile::Finite(v) == static_cast<bool>(std::isfinite(v)))
            ++agree;
        else if (!firstOff)
            firstOff = bits | 1u;
    };
    for (uint32_t e : edges)
        check(e);
    for (uint64_t bits = 0; bits <= 0xffffffffull; bits += 65521)
        check(static_cast<uint32_t>(bits));
    Report("p7e", "Finite as std::isfinite", agree == checked, agree == checked ? "%d of %d bit patterns" : "%d of %d; first off near %08x", agree, checked,
           firstOff);
}

// ---- 8. The time ----
// The best of five runs of a million (the self-test runs this beside the game: a run the system took the processor from is left out).
void CheckP8() {
    const In ins[8] = {RifleIn(), SmawIn(), HeldIn(kAn94), HeldIn(kStinger), HeldIn(kM240), HeldIn(5), RifleIn(), HeldIn(3)};
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    constexpr int kMakes = 1000000, kRuns = 5;
    double best = 1e9, worst = 0.0;
    for (int run = 0; run < kRuns; ++run) {
        LARGE_INTEGER a, b;
        uint64_t ids = 0;
        double sum = 0.0;
        QueryPerformanceCounter(&a);
        for (int n = 0; n < kMakes; ++n) {
            const Profile p = gunprofile::Make(ins[n & 7]);
            ids += p.id + static_cast<uint32_t>(p.slot) + (p.name[0] == 'S');
            sum += p.anchor[0][0] + p.support[1][2] + p.boxMax[2] + p.up;
        }
        QueryPerformanceCounter(&b);
        static volatile uint64_t s_ids;  // (what was made is used, so none of it is left out)
        static volatile double s_sum;
        s_ids = ids, s_sum = sum;
        const double ms = static_cast<double>(b.QuadPart - a.QuadPart) * 1000.0 / static_cast<double>(frequency.QuadPart);
        best = ms < best ? ms : best;
        worst = ms > worst ? ms : worst;
    }
    Report("p8", "a million Makes", best < 50.0, "%.1f ms at best of %d runs (< 50; the worst %.1f): %.0f ns a Make", best, kRuns, worst, best * 1e6 / kMakes);
}

// ---- 9. A change of gun ----
void CheckP9() {
    using gunprofile::Change;
    using gunprofile::OnChange;
    struct Case {
        const char* what;
        bool byPistolGrip;
        int lastHeldSide;
        uint64_t sinceMs;
        bool leftSeen, newLife;
        Change want;
    };
    const uint64_t never = static_cast<uint64_t>(-1) - 1000;  // (the time since a last-held time of 0, long ago)
    const Case cases[] = {
        {"the left, 0.5 s before", true, 0, 500, true, false, Change::kLeft},
        {"the left, the frame before", true, 0, 14, true, false, Change::kLeft},
        {"the left, 1.999 s before", true, 0, 1999, true, false, Change::kLeft},
        {"the left, 2 s before", true, 0, 2000, true, false, Change::kRight},
        {"the left, 2.5 s before", true, 0, 2500, true, false, Change::kRight},
        {"the left, never", true, 0, never, true, false, Change::kRight},
        {"the left by its front alone (kBarrel), 0.5 s before", false, 0, 500, true, false, Change::kReturn},
        {"lying or flying, the left's 0.5 s before", false, 0, 500, true, false, Change::kReturn},
        {"the left at a new life", true, 0, 500, true, true, Change::kRight},
        {"the left unseen", true, 0, 500, false, false, Change::kRight},
        {"the right, 0.5 s before", true, 1, 500, true, false, Change::kRight},
        {"nobody by its pistol grip", true, -1, 500, true, false, Change::kRight},
        {"by its front alone, the right's before", false, 1, 500, true, false, Change::kReturn},
    };
    int good = 0;
    char bad[512] = {};
    for (const Case& c : cases) {
        const Change got = OnChange(c.byPistolGrip, c.lastHeldSide, c.sinceMs, c.leftSeen, c.newLife);
        if (got == c.want)
            ++good;
        else {
            const size_t used = std::strlen(bad);
            std::snprintf(bad + used, sizeof(bad) - used, "%s%s: %d", used ? "; " : "", c.what, static_cast<int>(got));
        }
    }
    const int count = static_cast<int>(sizeof(cases) / sizeof(cases[0]));
    Report("p9", "a change of gun: the left kept as the trigger hand only as it should be", good == count,
           good == count ? "%d of %d: kept across 0.5 s, not 2 s or 2.5 s, from a front hold or lying (returned), at a new life or unseen" : "%d of %d; %s",
           good, count, bad);
}

}  // namespace

int main() {
    MakeBodies();
    std::printf("profiletest: src/gunprofile.h, the profile of whatever gun is in hand: the M416 (id %u), the SMAW (id %u) and %d held guns.\n",
                gunprofile::kRifleId, gunprofile::kSmawId, kHeldCount);
    CheckP1();
    CheckP2();
    CheckP3();
    CheckP4();
    CheckP5();
    CheckP6();
    CheckP7();
    CheckP8();
    CheckP9();
    std::printf("%d of %d checks failed\n", g_failed, g_counted);
    return g_failed;
}

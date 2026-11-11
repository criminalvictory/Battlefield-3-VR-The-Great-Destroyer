// A gun's magazine and lever worked by the left hand (src/handmag.cpp) on scripted frames at 72 Hz, with the AN-94's numbers and the hold fitted to it: what goes home by itself and from where, what takes the
// seated magazine and what is left for the front grip, what it takes to knock an old one out, and the lever. Then free hands: what the hand has when the gun leaves the hands (Frame::dropHeld, m1-m4), and the
// squeeze the regrip leaves alone (Claims, m6, on the KH2002 and against Update itself). Prints PASS or FAIL per check; the exit code is the failures.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>

#include "../src/handmag.h"
#include "../src/handpose.h"

#include "../src/pouch.h"

namespace magposes {
#include "../src/magposes/an94.h"
#include "../src/magposes/an94_lever.h"
#include "../src/magposes/kh2002.h"
#include "../src/magposes/kh2002_lever.h"
#include "../src/magposes/m4a1.h"
#include "../src/magposes/m4a1_lever.h"
#include "../src/magposes/m240.h"
#include "../src/magposes/m240_lid.h"
#include "../src/magposes/m240_belt.h"
}

namespace {

constexpr float kDt = 1.0f / 72.0f;
constexpr float kPi = 3.14159265f;
constexpr float kHandInGlove[3] = {0.5363f, 1.0976f, 0.2092f};
const handmag::Gun kGun{{0.0f, 0.033f, 0.386f}, {0.0f, -0.075f, 0.410f}, {0.0f, -0.012f, 0.340f}, {0.0f, 0.005f, 0.424f}, -0.05f, {-0.034f, 0.068f, 0.495f}, 0.113f, 25.0f * kPi / 180.0f, {0.0f, -0.008f, 0.010f}, magposes::kAn94Hold, magposes::kAn94Fingers, false, false, {{0.023f, 0.348f}, {-0.065f, 0.365f}, {-0.152f, 0.404f}}, magposes::kAn94LeverHold, magposes::kAn94LeverFingers};
const handmag::Gun kM4{{0.0f, 0.016f, 0.451f}, {0.0f, -0.133f, 0.461f}, {0.0f, -0.029f, 0.407f}, {0.0f, -0.012f, 0.487f}, -0.077f, {0.0f, 0.05f, 0.292f}, 0.08f, 0.0f, {0.0f, 0.0f, 0.0f}, magposes::kM4a1Hold, magposes::kM4a1Fingers, true, false, {}, magposes::kM4a1LeverHold, magposes::kM4a1LeverFingers, true, {0.03f, -0.015f, 0.406f}};
const handmag::BeltGun kBelt{{0.068f, 0.568f}, 103.0f * kPi / 180.0f, {0.048f, 0.087f, 0.335f}, magposes::kM240LidHold, magposes::kM240LidFingers, magposes::kM240BeltHanging, magposes::kM240BeltLaid, {0.4884f, 1.0245f, 0.2538f}, magposes::kBeltPinchFingers, {0.012f, 0.0535f, 0.519f}, {0.085f, -0.035f, 0.52f},
    {{-0.001f, -0.0445f, 0.025f, -0.0445f}, {0.0f, -0.0625f, 0.027f, -0.0625f}, {0.0f, -0.0765f, 0.024f, -0.0765f}, {0.005f, -0.079f, 0.005f, -0.105f}, {-0.003f, -0.088f, -0.003f, -0.143f}}, 0.308f};
const handmag::Gun kM240{{0.0f, -0.022f, 0.508f}, {0.095f, -0.09f, 0.50f}, {0.0f, -0.067f, 0.438f}, {0.0f, -0.05f, 0.57f}, -0.05f, {-0.062f, -0.002f, 0.442f}, 0.129f, 0.0f, {0.0f, 0.0f, 0.0f}, magposes::kM240Hold, magposes::kM240Fingers, false, true, {}, nullptr, nullptr, false, {}, 0.0f, {}, 0.0f, &kBelt};
constexpr float kFrontGrip[3] = {0.002f, -0.018f, 0.561f};
// The gun's frame in the view's space: anywhere, turned a little; a test doesn't care which.
const float kGunPose[12] = {0.9848f, 0.0f, 0.1736f, 0.05f, 0.0f, 1.0f, 0.0f, -0.2f, -0.1736f, 0.0f, 0.9848f, -0.6f};

int g_failures = 0;
void Check(bool ok, const char* what) {
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        ++g_failures;
}

void Compose(const float* a, const float* b, float* out) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j)
            out[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        out[i * 4 + 3] += a[i * 4 + 3];
    }
}

void Identity(float* m) {
    const float id[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    std::memcpy(m, id, sizeof(id));
}

// The glove as it would hold a magazine with its top moved by this on the gun from seated, turned by this about x.
void GloveWithMagazine(const float* off, float tilt, float* glove) {
    const float c = std::cos(tilt), s = std::sin(tilt);
    float mag[12] = {1, 0, 0, 0, 0, c, s, 0, 0, -s, c, 0};
    // Its top where it's wanted.
    const float* top = kGun.top;
    const float turned[3] = {top[0], c * top[1] + s * top[2], -s * top[1] + c * top[2]};
    for (int i = 0; i < 3; ++i)
        mag[i * 4 + 3] = top[i] + off[i] - turned[i];
    float inView[12];
    Compose(kGunPose, mag, inView);
    handmag::GloveFor(kGun, inView, glove);
}

// The glove with its hand's middle at this place on the gun, its palm up as on a handguard (the game's own: palm (-0.17 0.88 -0.44), fingers (-0.45 0.33 0.83)), or sideways as on a magazine.
void GloveAt(const float* place, bool palmUp, float* glove) {
    // The gloves' bind space into the gun's: rows from the game's hold on the handguard, and from the fitted hold on the magazine.
    float inGun[12];
    if (palmUp) {
        constexpr float along[3] = {0.437f, -0.544f, 0.716f}, across[3] = {0.633f, -0.380f, -0.675f}, palm[3] = {-0.639f, -0.748f, -0.179f};
        constexpr float wantAlong[3] = {-0.45f, 0.33f, 0.83f}, wantPalm[3] = {-0.17f, 0.88f, -0.44f}, wantAcross[3] = {-0.88f, -0.34f, -0.34f};
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                inGun[i * 4 + j] = wantAlong[i] * along[j] + wantPalm[i] * palm[j] + wantAcross[i] * across[j];
    } else {
        Identity(inGun);
        float hold[12];
        handmag::HoldFor(kGun, hold);
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                inGun[i * 4 + j] = hold[j * 4 + i];
    }
    float at[3];
    for (int i = 0; i < 3; ++i)
        at[i] = inGun[i * 4] * kHandInGlove[0] + inGun[i * 4 + 1] * kHandInGlove[1] + inGun[i * 4 + 2] * kHandInGlove[2];
    for (int i = 0; i < 3; ++i)
        inGun[i * 4 + 3] = place[i] - at[i];
    Compose(kGunPose, inGun, glove);
}

handmag::Frame Base() {
    handmag::Frame f;
    f.dt = kDt;
    f.gun = &kGun;
    f.gunTracked = true;
    std::memcpy(f.gunPose, kGunPose, sizeof(f.gunPose));
    std::memcpy(f.frontGrip, kFrontGrip, sizeof(f.frontGrip));
    f.left = true;
    f.spare = true;
    return f;
}

// Everything let go of, and the gun gone a frame: nothing carries over from the last check.
void Reset() {
    handmag::Clear();
    handmag::Frame f = Base();
    f.gun = nullptr;
    handmag::Update(f);
    f = Base();
    handmag::Update(f);
}

// A fresh magazine taken with the hand here.
handmag::Result Take(float* glove, bool wellEmpty) {
    handmag::Frame f = Base();
    f.wellEmpty = wellEmpty;
    std::memcpy(f.glove, glove, sizeof(f.glove));
    f.squeeze = 0.0f;
    handmag::Update(f);
    f.squeeze = 1.0f;
    f.shoulder = true;
    return handmag::Update(f);
}

// ---- For free hands' checks ----

// The KH2002 (skin.cpp's kKh2002Mag): a bullpup with a button, its magazine behind the pistol grip. Where the game's left palm holds it at the front, and its
// right palm on the pistol grip mirrored: the left's pistol grip, 9.6 cm from the magazine's middle.
const handmag::Gun kKh2002{{0.0f, -0.010f, 0.159f}, {0.0f, -0.060f, 0.150f}, {0.0f, -0.055f, 0.123f}, {0.0f, -0.038f, 0.187f}, -0.05f, {0.0f, 0.058f, 0.392f}, 0.085f, 0.0f, {0.0f, 0.0f, 0.0f}, magposes::kKh2002Hold, magposes::kKh2002Fingers, true, false, {}, magposes::kKh2002LeverHold, magposes::kKh2002LeverFingers};
constexpr float kKhFrontGrip[3] = {0.016f, -0.059f, 0.447f};
constexpr float kKhLeftPistolGrip[3] = {0.025f, -0.099f, 0.234f};
// The other guns' front grips (the game's left palms, skin.cpp kHeldKinds).
constexpr float kM4FrontGrip[3] = {0.012f, -0.041f, 0.612f};
constexpr float kM240FrontGrip[3] = {0.018f, -0.065f, 0.678f};
// hands.cpp's grip point on the left glove (g_leftAnchor): the palm it passes Claims.
constexpr float kLeftAnchor[3] = {0.502f, 1.146f, 0.144f};

void Palm(const float* glove, float* palm) {
    for (int i = 0; i < 3; ++i)
        palm[i] = glove[i * 4] * kLeftAnchor[0] + glove[i * 4 + 1] * kLeftAnchor[1] + glove[i * 4 + 2] * kLeftAnchor[2] + glove[i * 4 + 3];
}

// The gloves' bind space into the gun's frame as this gun's fitted hold has the hand on its seated magazine (the palm on its side).
void HoldTurn(const handmag::Gun& gun, float* turn) {
    float hold[12];
    handmag::HoldFor(gun, hold);
    Identity(turn);
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            turn[i * 4 + j] = hold[j * 4 + i];
}

// The glove turned by "turn" (the gloves' bind space into the gun's frame), its hand's middle at this place on the gun.
void GloveTurned(const float* turn, const float* place, float* glove) {
    float inGun[12];
    std::memcpy(inGun, turn, sizeof(inGun));
    for (int i = 0; i < 3; ++i)
        inGun[i * 4 + 3] = place[i] - (turn[i * 4] * kHandInGlove[0] + turn[i * 4 + 1] * kHandInGlove[1] + turn[i * 4 + 2] * kHandInGlove[2]);
    Compose(kGunPose, inGun, glove);
}

// A frame of this gun with its own front grip.
handmag::Frame BaseOf(const handmag::Gun& gun, const float* frontGrip) {
    handmag::Frame f = Base();
    f.gun = &gun;
    std::memcpy(f.frontGrip, frontGrip, sizeof(f.frontGrip));
    return f;
}

// The M240's lid's back end with the lid turned up by this much, on the frame.
void LidAt(float angle, float* out) {
    const float c = std::cos(angle), s = std::sin(angle), y = kBelt.lidGrab[1] - kBelt.lidHinge[0], z = kBelt.lidGrab[2] - kBelt.lidHinge[1];
    out[0] = kBelt.lidGrab[0];
    out[1] = kBelt.lidHinge[0] + c * y - s * z;
    out[2] = kBelt.lidHinge[1] + s * y + c * z;
}

// The glove with the M240's belt pinch (its thumb and pointer) at this place on the frame, as its fitted hold on the box is turned.
void GloveWithPinch(float x, float y, float z, float* out) {
    float hold[12], inGun[12], p[3];
    handmag::HoldFor(kM240, hold);
    for (int a = 0; a < 3; ++a)
        for (int c = 0; c < 3; ++c)
            inGun[a * 4 + c] = hold[c * 4 + a];
    for (int a = 0; a < 3; ++a)
        inGun[a * 4 + 3] = -(inGun[a * 4] * hold[3] + inGun[a * 4 + 1] * hold[7] + inGun[a * 4 + 2] * hold[11]);
    for (int a = 0; a < 3; ++a)
        p[a] = inGun[a * 4] * kBelt.pinch[0] + inGun[a * 4 + 1] * kBelt.pinch[1] + inGun[a * 4 + 2] * kBelt.pinch[2] + inGun[a * 4 + 3];
    inGun[3] += x - p[0], inGun[7] += y - p[1], inGun[11] += z - p[2];
    Compose(kGunPose, inGun, out);
}

// The M240 with its lid up: taken at its back end and lifted till it goes on by itself, then let go of. The frame left with the M240's front grip.
bool M240LidUp(handmag::Frame& f) {
    Reset();
    handmag::BeltReset();
    f = BaseOf(kM240, kM240FrontGrip);
    handmag::Update(f);  // the change of gun
    float at[3];
    LidAt(0.0f, at);
    GloveAt(at, false, f.glove);
    handmag::Update(f);
    f.squeeze = 1.0f;
    const bool taken = handmag::Update(f).lidTaken;
    bool up = false;
    for (int i = 1; i < 60 && !up; ++i) {
        LidAt(std::min(0.02f * i, 0.8f), at);
        GloveAt(at, false, f.glove);
        up = handmag::Update(f).lidUp;
    }
    f.squeeze = 0.0f;
    handmag::Update(f);
    return taken && up;
}

// ... then its box off by the hand and a new one in (as the magnet puts one in): its belt hanging out of the box, to be laid by hand.
bool M240BeltHanging(handmag::Frame& f) {
    if (!M240LidUp(f))
        return false;
    const float onBox[3] = {0.13f, -0.09f, 0.50f};
    GloveAt(onBox, false, f.glove);
    handmag::Update(f);
    f.squeeze = 1.0f;
    const bool grabbed = handmag::Update(f).grabbed;
    bool out = false;
    for (int i = 1; i < 40 && !out; ++i) {
        const float down[3] = {onBox[0], onBox[1] - 0.004f * i, onBox[2]};
        GloveAt(down, false, f.glove);
        out = handmag::Update(f).pulledOut;
    }
    f.wellEmpty = true;  // (the game told: the box is out)
    f.squeeze = 0.0f;
    handmag::Update(f);  // let go of: it falls
    f.wellEmpty = false;  // a new one in
    const handmag::Result r = handmag::Update(f);
    return grabbed && out && r.beltDrawn && !r.beltReady;
}

// A small fixed sequence of numbers 0-1 (the same each run).
struct Rng {
    uint32_t s;
    float Next() {
        s = s * 1664525u + 1013904223u;
        return static_cast<float>(s >> 8) * (1.0f / 16777216.0f);
    }
};

// A turn every way alike (a unit quaternion from the unit ball), as a 3x4 with no translation.
void RandomTurn(Rng& g, float* m) {
    float q[4], n = 0.0f;
    do {
        for (float& v : q)
            v = g.Next() * 2.0f - 1.0f;
        n = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
    } while (n > 1.0f || n < 1e-4f);
    n = std::sqrt(n);
    const float x = q[0] / n, y = q[1] / n, z = q[2] / n, w = q[3] / n;
    const float r[12] = {1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w), 0.0f, 2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w), 0.0f,
                         2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y), 0.0f};
    std::memcpy(m, r, sizeof(r));
}

// Something of the gun taken by a squeeze this frame (not a fresh one from over the shoulder).
bool TookOnGun(const handmag::Result& r) {
    return r.grabbed || r.leverTaken || r.lidTaken || r.beltTaken;
}

}  // namespace

int main() {
    // ---- The magnet ----
    {
        struct Case {
            const char* name;
            float off[3], tilt;
            bool home;
        };
        const Case cases[] = {
            {"from below, near the mouth", {0.0f, -0.12f, 0.0f}, 0.0f, true},
            {"from below and behind, turned", {0.0f, -0.09f, -0.05f}, 0.5f, true},
            {"from the side, level with the receiver's underside", {0.08f, -0.05f, 0.0f}, 0.3f, true},
            {"from the far side", {-0.08f, -0.05f, 0.0f}, -0.3f, true},
            {"up inside the receiver (was a clip through)", {0.0f, 0.02f, 0.0f}, 0.2f, true},
            {"from above the mouth, in the gun's way", {0.0f, -0.02f, 0.03f}, 0.9f, true},
            {"turned a long way", {0.0f, -0.08f, 0.0f}, 1.9f, true},
            {"too far off yet", {0.0f, -0.20f, 0.0f}, 0.0f, false},
            {"too far to the side yet", {0.14f, -0.05f, 0.0f}, 0.0f, false},
        };
        for (const Case& c : cases) {
            Reset();
            float glove[12];
            GloveWithMagazine(c.off, c.tilt, glove);
            handmag::Frame f = Base();
            f.wellEmpty = true;
            std::memcpy(f.glove, glove, sizeof(f.glove));
            f.squeeze = 1.0f;
            f.shoulder = true;
            handmag::Update(f);
            f.shoulder = false;
            int frames = 0;
            bool seated = false, caught = false;
            for (; frames < 30 && !seated; ++frames) {
                const handmag::Result r = handmag::Update(f);
                caught = caught || r.caught;
                seated = seated || r.seated;
                if (r.seated)
                    break;
            }
            char what[160];
            std::snprintf(what, sizeof(what), "magnet: %s: %s (%d frames)", c.name, seated ? "home" : "stays in the hand", frames);
            Check(seated == c.home, what);
            if (c.home)
                Check(frames <= 13, "  ... going home in 13 frames at most");
        }
        // Let go of on the way: it still goes home.
        Reset();
        float glove[12];
        const float off[3] = {0.0f, -0.12f, 0.0f};
        GloveWithMagazine(off, 0.0f, glove);
        handmag::Frame f = Base();
        f.wellEmpty = true;
        std::memcpy(f.glove, glove, sizeof(f.glove));
        f.squeeze = 1.0f;
        f.shoulder = true;
        handmag::Update(f);
        f.shoulder = false;
        handmag::Update(f);
        handmag::Update(f);
        f.squeeze = 0.0f;
        bool seated = false;
        for (int i = 0; i < 20 && !seated; ++i)
            seated = handmag::Update(f).seated;
        Check(seated, "magnet: let go of on its way, it still goes home");
    }
    // ---- The gun or the magazine ----
    {
        struct Case {
            const char* name;
            float at[3];
            bool palmUp, takes;
        };
        const Case cases[] = {
            {"the fitted hold's place, palm on it", {0.050f, -0.088f, 0.391f}, false, true},
            {"on its other side, palm on it", {-0.050f, -0.088f, 0.400f}, false, true},
            {"low on it, palm up", {0.020f, -0.120f, 0.410f}, true, true},
            {"the game's own hold on the handguard", {0.002f, -0.016f, 0.562f}, true, false},
            {"the handguard's back end, palm up", {0.010f, -0.040f, 0.490f}, true, false},
            {"just forward of the magazine, palm sideways", {0.030f, -0.050f, 0.480f}, false, false},
            {"up at the well, palm up", {0.020f, -0.040f, 0.420f}, true, false},
            {"under the handguard, well forward", {0.020f, -0.070f, 0.540f}, true, false},
        };
        for (const Case& c : cases) {
            Reset();
            float glove[12];
            GloveAt(c.at, c.palmUp, glove);
            handmag::Frame f = Base();
            std::memcpy(f.glove, glove, sizeof(f.glove));
            handmag::Update(f);
            f.squeeze = 1.0f;
            const handmag::Result r = handmag::Update(f);
            char what[160];
            std::snprintf(what, sizeof(what), "squeeze: %s: %s", c.name, r.grabbed ? "takes the magazine" : "left for the gun");
            Check(r.grabbed == c.takes, what);
        }
        // Taken, then the hand goes along the gun to the handguard: it's not pulled out, and the squeeze goes on to the front grip.
        Reset();
        const float at[3] = {0.050f, -0.088f, 0.391f};
        float glove[12];
        GloveAt(at, false, glove);
        handmag::Frame f = Base();
        std::memcpy(f.glove, glove, sizeof(f.glove));
        handmag::Update(f);
        f.squeeze = 1.0f;
        handmag::Update(f);
        bool out = false, toGrip = false;
        for (int i = 0; i < 40 && !toGrip; ++i) {
            const float along[3] = {at[0], at[1], at[2] + 0.004f * i};
            GloveAt(along, false, f.glove);
            const handmag::Result r = handmag::Update(f);
            out = out || r.pulledOut;
            toGrip = toGrip || r.toGrip;
        }
        Check(!out && toGrip, "taken, the hand on along the gun: stays in, the squeeze goes to the front grip");
        // Taken, then up: the same.
        Reset();
        GloveAt(at, false, glove);
        std::memcpy(f.glove, glove, sizeof(f.glove));
        f.squeeze = 0.0f;
        handmag::Update(f);
        f.squeeze = 1.0f;
        handmag::Update(f);
        out = toGrip = false;
        for (int i = 0; i < 30 && !toGrip; ++i) {
            const float up[3] = {at[0], at[1] + 0.003f * i, at[2]};
            GloveAt(up, false, f.glove);
            const handmag::Result r = handmag::Update(f);
            out = out || r.pulledOut;
            toGrip = toGrip || r.toGrip;
        }
        Check(!out && toGrip, "taken, the hand on up: stays in, the squeeze goes to the front grip");
        // Taken, then sideways, a little: nothing yet. Then down: out.
        Reset();
        GloveAt(at, false, glove);
        std::memcpy(f.glove, glove, sizeof(f.glove));
        f.squeeze = 0.0f;
        handmag::Update(f);
        f.squeeze = 1.0f;
        handmag::Update(f);
        out = false;
        for (int i = 0; i < 20; ++i) {
            const float side[3] = {at[0] + 0.004f * i, at[1], at[2]};
            GloveAt(side, false, f.glove);
            out = out || handmag::Update(f).pulledOut;
        }
        Check(!out, "taken, the hand 8 cm to the side: not pulled out");
        int downFrames = 0;
        for (int i = 0; i < 40 && !out; ++i, ++downFrames) {
            const float down[3] = {at[0] + 0.076f, at[1] - 0.004f * i, at[2]};
            GloveAt(down, false, f.glove);
            out = handmag::Update(f).pulledOut;
        }
        Check(out, "... then pulled down: out");
    }
    // ---- The knock ----
    {
        // The fresh one's front corner sweeping forward along the gun at this speed, from 12 cm behind the old one's back edge at the height where it touches.
        struct Sweep {
            bool flicked = false, touched = false;
            float at = 0.0f;
            float drawnPast = 0.0f;
        };
        auto sweep = [](float across, float height, float speed, float tilt, float hold) {
            Reset();
            float glove[12];
            // The front corner is hook's place on the magazine; the top moved by "off" moves it the same with no tilt.
            float off[3] = {across, height, -0.14f};
            GloveWithMagazine(off, tilt, glove);
            Take(glove, false);
            handmag::Frame f = Base();
            f.squeeze = 1.0f;
            std::memcpy(f.glove, glove, sizeof(f.glove));
            for (int i = 0; i < 12; ++i)  // brought here from the shoulder first
                handmag::Update(f);
            Sweep s;
            for (int i = 0; i < 400; ++i) {
                off[2] += speed * kDt;
                if (off[2] > 0.12f + hold)
                    break;
                GloveWithMagazine(off, tilt, f.glove);
                const handmag::Result r = handmag::Update(f);
                if (r.flicked) {
                    s.flicked = true;
                    s.at = off[2];
                    break;
                }
                if (r.touched)
                    s.touched = true;
                if (r.mag) {
                    // How far the drawn magazine is behind where the hand has it.
                    float wanted[12], back[12];
                    float inView[12];
                    (void)wanted, (void)back, (void)inView;
                    const float d = (f.glove[3] - r.glove[3]) * (r.gloveBlend > 0.5f ? 1.0f : 0.0f);
                    s.drawnPast = std::fabs(d) > s.drawnPast ? std::fabs(d) : s.drawnPast;
                }
            }
            return s;
        };
        char what[160];
        for (float speed : {0.2f, 0.5f, 0.8f, 1.1f}) {
            const Sweep s = sweep(0.0f, -0.06f, speed, 0.0f, 0.0f);
            std::snprintf(what, sizeof(what), "knock: touching it at %.1f m/s: no flick, stops against it", speed);
            Check(!s.flicked && s.touched, what);
        }
        for (float speed : {1.6f, 2.2f, 3.0f}) {
            const Sweep s = sweep(0.0f, -0.06f, speed, 0.0f, 0.0f);
            std::snprintf(what, sizeof(what), "knock: at %.1f m/s, touching: knocked out", speed);
            Check(s.flicked, what);
        }
        // Coming up from under it as it goes forward: its way crosses the back edge between two frames.
        {
            Reset();
            float glove[12];
            float off[3] = {0.0f, -0.22f, -0.20f};
            GloveWithMagazine(off, 0.0f, glove);
            Take(glove, false);
            handmag::Frame f = Base();
            f.squeeze = 1.0f;
            std::memcpy(f.glove, glove, sizeof(f.glove));
            for (int i = 0; i < 12; ++i)
                handmag::Update(f);
            bool flicked = false;
            for (int i = 0; i < 30 && !flicked; ++i) {
                off[1] += 2.5f * kDt;
                off[2] += 2.5f * kDt;
                GloveWithMagazine(off, 0.0f, f.glove);
                flicked = handmag::Update(f).flicked;
            }
            Check(flicked, "knock: swung up and forward at 3.5 m/s: knocked out, however it came");
        }
        // Not touching it: past its side, at any speed.
        for (float speed : {0.5f, 2.0f, 4.0f}) {
            const Sweep s = sweep(0.06f, -0.06f, speed, 0.0f, 0.0f);
            std::snprintf(what, sizeof(what), "knock: past its side at %.1f m/s, not touching: nothing", speed);
            Check(!s.flicked && !s.touched, what);
        }
        // Under its bottom.
        {
            const Sweep s = sweep(0.0f, -0.25f, 2.5f, 0.0f, 0.0f);
            Check(!s.flicked && !s.touched, "knock: under its bottom at 2.5 m/s: nothing");
        }
        // Waved near it: fast, but not forward into it.
        {
            Reset();
            float glove[12];
            float off[3] = {0.08f, -0.06f, -0.05f};
            GloveWithMagazine(off, 0.0f, glove);
            Take(glove, false);
            handmag::Frame f = Base();
            f.squeeze = 1.0f;
            bool flicked = false;
            for (int i = 0; i < 40; ++i) {
                off[0] = 0.08f - 0.0417f * (i % 4);  // across, 3 m/s
                off[1] = -0.06f + 0.02f * (i % 3);
                GloveWithMagazine(off, 0.0f, f.glove);
                flicked = flicked || handmag::Update(f).flicked;
            }
            Check(!flicked, "knock: waved across behind it, fast: nothing");
        }
        // Coming from in front of it, back into it: nothing.
        {
            const float speed = -2.0f;
            Reset();
            float glove[12];
            float off[3] = {0.0f, -0.06f, 0.20f};
            GloveWithMagazine(off, 0.0f, glove);
            Take(glove, false);
            handmag::Frame f = Base();
            f.squeeze = 1.0f;
            bool flicked = false;
            for (int i = 0; i < 40; ++i) {
                off[2] += speed * kDt;
                GloveWithMagazine(off, 0.0f, f.glove);
                flicked = flicked || handmag::Update(f).flicked;
            }
            Check(!flicked, "knock: from in front of it, back through it: nothing");
        }
        // After a real knock it goes home by itself.
        {
            Reset();
            float glove[12];
            float off[3] = {0.0f, -0.06f, -0.14f};
            GloveWithMagazine(off, 0.0f, glove);
            Take(glove, false);
            handmag::Frame f = Base();
            f.squeeze = 1.0f;
            bool flicked = false, seated = false;
            int after = 0;
            for (int i = 0; i < 100 && !seated; ++i) {
                if (!flicked) {
                    off[2] += 2.0f * kDt;
                    GloveWithMagazine(off, 0.0f, f.glove);
                } else {
                    ++after;
                }
                f.wellEmpty = flicked;
                const handmag::Result r = handmag::Update(f);
                flicked = flicked || r.flicked;
                seated = seated || r.seated;
            }
            Check(flicked && seated && after < 20, "knock: then home by itself, snappy");
        }
        // A tracking gap isn't a speed.
        {
            Reset();
            float glove[12];
            float off[3] = {0.0f, -0.06f, -0.06f};
            GloveWithMagazine(off, 0.0f, glove);
            Take(glove, false);
            handmag::Frame f = Base();
            f.squeeze = 1.0f;
            for (int i = 0; i < 8; ++i) {
                GloveWithMagazine(off, 0.0f, f.glove);
                handmag::Update(f);
            }
            f.left = false;
            handmag::Update(f);
            handmag::Update(f);
            f.left = true;
            off[2] = 0.02f;
            GloveWithMagazine(off, 0.0f, f.glove);
            bool flicked = handmag::Update(f).flicked;
            for (int i = 0; i < 4; ++i)
                flicked = flicked || handmag::Update(f).flicked;
            Check(!flicked, "knock: a tracking gap, the hand further on: no flick");
        }
    }
    // ---- The lever ----
    {
        Reset();
        const float at[3] = {-0.075f, 0.09f, 0.54f};
        float glove[12];
        GloveAt(at, false, glove);
        handmag::Frame f = Base();
        std::memcpy(f.glove, glove, sizeof(f.glove));
        handmag::Update(f);
        f.trigger = 1.0f;
        handmag::Result r = handmag::Update(f);
        Check(r.leverTaken, "lever: the trigger near it takes it");
        Check(r.spentSqueeze == false, "lever: the grip isn't spent");
        r = handmag::Update(f);
        Check(r.fist && r.leverFingers == magposes::kAn94LeverFingers, "lever: the fingers as the game's hand has them");
        // Drawn back with the hand.
        float last = 0.0f;
        for (int i = 0; i < 30; ++i) {
            const float back[3] = {at[0], at[1], at[2] - 0.004f * i};
            GloveAt(back, false, f.glove);
            r = handmag::Update(f);
            last = r.leverBack;
        }
        Check(std::fabs(last - 0.113f) < 0.002f, "lever: back as far as it goes");
        f.trigger = 0.0f;
        r = handmag::Update(f);
        Check(r.leverLet && r.charged, "lever: let go of, a round chambered");
        // The grip takes it as well (the user, 2026-11-05), held by it, let go of by letting go of both.
        Reset();
        GloveAt(at, false, glove);
        std::memcpy(f.glove, glove, sizeof(f.glove));
        f.trigger = 0.0f;
        f.squeeze = 0.0f;
        handmag::Update(f);
        f.squeeze = 1.0f;
        r = handmag::Update(f);
        Check(r.leverTaken, "lever: the grip takes it too");
        f.trigger = 1.0f;
        f.squeeze = 0.0f;
        r = handmag::Update(f);
        Check(!r.leverLet, "lever: from the grip to the trigger: still held");
        f.trigger = 0.0f;
        r = handmag::Update(f);
        Check(r.leverLet, "lever: both let go of: let go");
        // But on the handguard, near the lever, the grip is the front grip's; the trigger there reaches the lever.
        Reset();
        const float onGuard[3] = {kFrontGrip[0], kFrontGrip[1] + 0.02f, kFrontGrip[2] - 0.02f};
        GloveAt(onGuard, true, glove);
        std::memcpy(f.glove, glove, sizeof(f.glove));
        f.squeeze = 0.0f;
        handmag::Update(f);
        f.squeeze = 1.0f;
        r = handmag::Update(f);
        Check(!r.leverTaken && !r.grabbed, "lever: the grip on the handguard stays the front grip's");
    }
    // ---- A magazine by the trigger too ----
    {
        Reset();
        const float at[3] = {0.050f, -0.088f, 0.391f};
        float glove[12];
        GloveAt(at, false, glove);
        handmag::Frame f = Base();
        std::memcpy(f.glove, glove, sizeof(f.glove));
        handmag::Update(f);
        f.trigger = 1.0f;
        const handmag::Result r = handmag::Update(f);
        Check(r.grabbed, "magazine: the trigger takes it too");
        Reset();
        const float off[3] = {0.0f, -0.3f, 0.0f};
        GloveWithMagazine(off, 0.0f, glove);
        f = Base();
        f.wellEmpty = true;
        std::memcpy(f.glove, glove, sizeof(f.glove));
        handmag::Update(f);
        f.trigger = 1.0f;
        f.shoulder = true;
        bool took = handmag::Update(f).took;
        Check(took, "magazine: a fresh one over the shoulder by the trigger");
        f.shoulder = false;
        handmag::Result held = handmag::Update(f);
        Check(held.holding, "magazine: held by the trigger");
        f.trigger = 0.0f;
        held = handmag::Update(f);
        Check(held.letGo, "magazine: the trigger let go of: it falls");
    }
    // ---- A gun with a button: its magazine by the hand as well, straight down the well ----
    {
        Reset();
        // The hand's middle on the M4's magazine as its hold has it.
        float hold[12];
        handmag::HoldFor(kM4, hold);
        float toGunTurn[12];
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j)
                toGunTurn[i * 4 + j] = hold[j * 4 + i];
        }
        for (int i = 0; i < 3; ++i)
            toGunTurn[i * 4 + 3] = -(toGunTurn[i * 4] * hold[3] + toGunTurn[i * 4 + 1] * hold[7] + toGunTurn[i * 4 + 2] * hold[11]);
        handmag::Frame f = Base();
        f.gun = &kM4;
        auto gloveDown = [&](float down, float* out) {
            float at[12];
            std::memcpy(at, toGunTurn, sizeof(at));
            at[7] -= down;
            Compose(kGunPose, at, out);
        };
        handmag::Update(f);  // the change of gun
        gloveDown(0.0f, f.glove);
        handmag::Update(f);
        f.squeeze = 1.0f;
        handmag::Result r = handmag::Update(f);
        Check(r.grabbed, "button: the hand takes the seated magazine");
        bool out = false, straight = true;
        for (int i = 1; i < 60 && !out; ++i) {
            const float down = 0.003f * i;
            gloveDown(down, f.glove);
            r = handmag::Update(f);
            out = r.pulledOut;
            if (!out && r.mag) {
                // It only ever comes straight down: its top on the well's line.
                float inverse[12];
                for (int a = 0; a < 3; ++a)
                    for (int b = 0; b < 3; ++b)
                        inverse[a * 4 + b] = kGunPose[b * 4 + a];
                float rel[3];
                for (int a = 0; a < 3; ++a) {
                    const float d[3] = {r.magPose[3] - kGunPose[3], r.magPose[7] - kGunPose[7], r.magPose[11] - kGunPose[11]};
                    rel[a] = inverse[a * 4] * d[0] + inverse[a * 4 + 1] * d[1] + inverse[a * 4 + 2] * d[2];
                }
                if (std::fabs(rel[0]) > 1e-3f || std::fabs(rel[2]) > 1e-3f)
                    straight = false;
            }
            if (out)
                Check(down >= 0.077f && down < 0.095f, "button: free when its top is out of the well");
        }
        Check(out && straight, "button: drawn straight down and out");
        // Back in: the magnet, as any other.
        f.wellEmpty = true;
        gloveDown(0.25f, f.glove);
        handmag::Update(f);
        gloveDown(0.12f, f.glove);
        bool seated = false;
        for (int i = 0; i < 30 && !seated; ++i)
            seated = handmag::Update(f).seated;
        Check(seated, "button: a magazine near the well goes home");
    }
    // ---- A belt fed gun: lid, box, belt, lid ----
    {
        Reset();
        handmag::BeltReset();
        handmag::Frame f = Base();
        f.gun = &kM240;
        handmag::Update(f);  // the change of gun
        auto lidAt = [&](float angle, float* out) {
            const float c = std::cos(angle), s = std::sin(angle), y = kBelt.lidGrab[1] - kBelt.lidHinge[0], z = kBelt.lidGrab[2] - kBelt.lidHinge[1];
            out[0] = kBelt.lidGrab[0];
            out[1] = kBelt.lidHinge[0] + c * y - s * z;
            out[2] = kBelt.lidHinge[1] + s * y + c * z;
        };
        // The box isn't to be had with the lid shut.
        const float onBox[3] = {0.13f, -0.09f, 0.50f};
        GloveAt(onBox, false, f.glove);
        handmag::Update(f);
        f.trigger = 1.0f;
        handmag::Result r = handmag::Update(f);
        Check(!r.grabbed, "belt fed: with the lid shut the box stays");
        f.trigger = 0.0f;
        handmag::Update(f);
        // The lid: taken at its back end, up with the hand; a little of the way, and it goes the rest by itself.
        float at[3];
        lidAt(0.0f, at);
        GloveAt(at, false, f.glove);
        handmag::Update(f);
        f.squeeze = 1.0f;
        r = handmag::Update(f);
        Check(r.lidTaken, "belt fed: the hand takes the lid");
        Check(!r.beltReady || r.lid == 0.0f, "belt fed: it's a lid, not a box");
        bool up = false, flown = false;
        for (int i = 1; i < 60 && !up; ++i) {
            lidAt(std::min(0.02f * i, 0.8f), at);
            GloveAt(at, false, f.glove);
            r = handmag::Update(f);
            flown = flown || (r.lid > 0.5f * kBelt.lidOpen);
            up = r.lidUp;
        }
        Check(up && flown, "belt fed: lifted a third of the way the lid goes up by itself, and stays");
        Check(!r.beltReady, "belt fed: with the lid up it won't fire");
        f.squeeze = 0.0f;
        handmag::Update(f);
        // The box off, with the lid up.
        GloveAt(onBox, false, f.glove);
        handmag::Update(f);
        f.squeeze = 1.0f;
        r = handmag::Update(f);
        Check(r.grabbed, "belt fed: with the lid up the box comes off");
        bool out = false;
        for (int i = 1; i < 40 && !out; ++i) {
            const float down[3] = {onBox[0], onBox[1] - 0.004f * i, onBox[2]};
            GloveAt(down, false, f.glove);
            out = handmag::Update(f).pulledOut;
        }
        Check(out, "belt fed: pulled off");
        f.wellEmpty = true;
        f.squeeze = 0.0f;
        r = handmag::Update(f);
        Check(!r.beltDrawn && !r.beltReady, "belt fed: no box, no belt");
        // A new one home by itself: its belt hanging.
        float hold[12], toGun[12];
        handmag::HoldFor(kM240, hold);
        for (int a = 0; a < 3; ++a) {
            for (int c = 0; c < 3; ++c)
                toGun[a * 4 + c] = hold[c * 4 + a];
            toGun[a * 4 + 3] = 0.0f;
        }
        for (int a = 0; a < 3; ++a)
            toGun[a * 4 + 3] = -(toGun[a * 4] * hold[3] + toGun[a * 4 + 1] * hold[7] + toGun[a * 4 + 2] * hold[11]);
        float faraway[12], under[12];
        std::memcpy(faraway, toGun, sizeof(faraway));
        faraway[7] -= 0.4f;
        std::memcpy(under, toGun, sizeof(under));
        under[7] += kM240.mouth - 0.05f;
        Compose(kGunPose, faraway, f.glove);
        f.squeeze = 1.0f;
        f.shoulder = true;
        handmag::Update(f);
        f.shoulder = false;
        for (int i = 0; i < 4; ++i)
            handmag::Update(f);
        Compose(kGunPose, under, f.glove);
        bool seated = false;
        for (int i = 0; i < 30 && !seated; ++i)
            seated = handmag::Update(f).seated;
        Check(seated, "belt fed: a new box goes home by itself");
        f.wellEmpty = false;
        f.squeeze = 0.0f;
        r = handmag::Update(f);
        Check(r.beltDrawn && !r.beltReady, "belt fed: its belt hanging, not in the feed: it won't fire");
        // The belt by the thumb and the pointer: its end where the hand has it, freely.
        auto gloveWithPinch = [&](float x, float y, float z, float* out) {
            float inGun[12], p[3];
            std::memcpy(inGun, toGun, sizeof(inGun));
            for (int a = 0; a < 3; ++a)
                p[a] = inGun[a * 4] * kBelt.pinch[0] + inGun[a * 4 + 1] * kBelt.pinch[1] + inGun[a * 4 + 2] * kBelt.pinch[2] + inGun[a * 4 + 3];
            inGun[3] += x - p[0], inGun[7] += y - p[1], inGun[11] += z - p[2];
            Compose(kGunPose, inGun, out);
        };
        gloveWithPinch(0.197f, -0.05f, 0.514f, f.glove);
        handmag::Update(f);
        f.trigger = 1.0f;
        r = handmag::Update(f);
        Check(r.beltTaken, "belt fed: the pinch takes the belt where it hangs");
        Check(r.leverFingers == nullptr || r.leverFingers == magposes::kBeltPinchFingers, "belt fed: as a pinch");
        // Off to the side, away from the feed: it's in the hand, the hand's where it is, it isn't in the feed.
        for (int i = 0; i < 8; ++i) {
            gloveWithPinch(0.22f, 0.06f + 0.01f * i, 0.46f, f.glove);
            r = handmag::Update(f);
        }
        Check(r.beltDrawn && !r.beltLaid && r.gloveBlend == 0.0f && r.leverFingers == magposes::kBeltPinchFingers, "belt fed: held anywhere, the hand free, the fingers pinching");
        // Its end is where the pinch is: the first piece's near end on the frame.
        const float near0[3] = {kBelt.pieces[0][0], kBelt.pieces[0][1], kBelt.pieceZ};
        float tip[3];
        for (int a = 0; a < 3; ++a)
            tip[a] = r.beltBones[0][a * 4] * near0[0] + r.beltBones[0][a * 4 + 1] * near0[1] + r.beltBones[0][a * 4 + 2] * near0[2] + r.beltBones[0][a * 4 + 3];
        const float want[3] = {0.22f, 0.13f, 0.46f};
        Check(std::hypot(tip[0] - want[0], tip[1] - want[1], tip[2] - want[2]) < 0.02f || std::hypot(tip[0] - kBelt.anchor[0], tip[1] - kBelt.anchor[1]) > 0.15f, "belt fed: the belt's end where the hand has it, or as far as it goes");
        // Let go of: back to hanging.
        f.trigger = 0.0f;
        r = handmag::Update(f);
        Check(r.beltLet, "belt fed: let go of, it hangs again");
        // Near the feed the feed takes it from the hand.
        gloveWithPinch(0.197f, -0.05f, 0.514f, f.glove);
        handmag::Update(f);
        f.trigger = 1.0f;
        handmag::Update(f);
        bool laid = false;
        int frames = 0;
        for (int i = 1; i < 60 && !laid; ++i, ++frames) {
            const float s = std::min(i / 20.0f, 1.0f);
            gloveWithPinch(0.197f + (0.05f - 0.197f) * s, -0.05f + (0.06f + 0.05f) * s, 0.514f, f.glove);
            laid = handmag::Update(f).beltLaid;
        }
        Check(laid, "belt fed: near the feed it goes in by itself");
        f.trigger = 0.0f;
        r = handmag::Update(f);
        Check(!r.beltDrawn && !r.beltReady, "belt fed: lying as the game has it; the lid's still up");
        // The lid shut: taken up there, brought down a little of the way, it goes the rest.
        lidAt(kBelt.lidOpen, at);
        GloveAt(at, false, f.glove);
        handmag::Update(f);
        f.squeeze = 1.0f;
        r = handmag::Update(f);
        Check(r.lidTaken, "belt fed: the hand takes the lid up there");
        bool shut = false;
        for (int i = 1; i < 60 && !shut; ++i) {
            lidAt(kBelt.lidOpen - std::min(0.03f * i, 0.9f), at);
            GloveAt(at, false, f.glove);
            shut = handmag::Update(f).lidShut;
        }
        Check(shut, "belt fed: brought down it shuts by itself");
        f.squeeze = 0.0f;
        r = handmag::Update(f);
        Check(r.beltReady, "belt fed: belt in, lid shut: it can fire");
        handmag::BeltReset();
    }
    // ---- The M416 (pouch.cpp): its magazine by the hand as well ----
    {
        pouch::Clear();
        // The seated magazine's hold, the hand where it is with it home.
        float seated[12], gunPose[12];
        std::memcpy(gunPose, kGunPose, sizeof(gunPose));
        pouch::GloveFor(gunPose, seated);
        pouch::Frame f;
        f.dt = kDt;
        f.left = true;
        f.gun = true;
        f.gunTracked = true;
        std::memcpy(f.gunPose, gunPose, sizeof(f.gunPose));
        f.head.orientation.w = 1.0f;
        f.leftGrip = {0.0f, -2.0f, 0.0f};  // far from the shoulder
        f.spare = true;
        auto gloveDown = [&](float down) {
            std::memcpy(f.glove, seated, sizeof(f.glove));
            for (int i = 0; i < 3; ++i)
                f.glove[i * 4 + 3] -= gunPose[i * 4 + 1] * down;
        };
        // Lower the hand to the magazine's middle: the hold has it there.
        gloveDown(0.0f);
        f.squeeze = 0.0f;
        pouch::Update(f);
        f.squeeze = 1.0f;
        pouch::Result r = pouch::Update(f);
        Check(r.grabbed, "M416: the hand takes the seated magazine");
        gloveDown(0.04f);
        r = pouch::Update(f);
        Check(r.hideSeated && r.mag && !r.pulledOut, "M416: part way down the well: the one in the hand drawn, the gun's own not");
        gloveDown(0.10f);
        r = pouch::Update(f);
        Check(r.pulledOut, "M416: out of the well: free");
        // Not caught again at once.
        f.wellEmpty = true;
        bool caught = false;
        for (int i = 0; i < 5; ++i)
            caught = caught || pouch::Update(f).caught;
        Check(!caught, "M416: not caught again straight away");
        // Let go of before it's out: it stays in.
        pouch::Clear();
        f.wellEmpty = false;
        f.squeeze = 0.0f;
        gloveDown(0.0f);
        pouch::Update(f);
        f.squeeze = 1.0f;
        pouch::Update(f);
        f.squeeze = 0.0f;
        r = pouch::Update(f);
        Check(!r.pulledOut && !r.letGo && !r.hideSeated, "M416: let go of before it's out: it stays in");
        // Going up instead: the handguard's.
        pouch::Clear();
        gloveDown(0.0f);
        pouch::Update(f);
        f.squeeze = 1.0f;
        pouch::Update(f);
        gloveDown(-0.04f);
        r = pouch::Update(f);
        Check(r.toGrip, "M416: the hand goes on up: the squeeze goes to the handguard");
        pouch::Clear();
    }
    // ---- Free hands: the gun leaves the hands with something in the left (Frame::dropHeld: loose has it, or it flies into a hand from the ground; the gun not
    // seen then, as hands.cpp feeds it). Every check above has dropHeld false: today's paths (m5). ----
    {
        char what[200];
        const auto finite3 = [](const float* v) { return std::isfinite(v[0]) && std::isfinite(v[1]) && std::isfinite(v[2]); };
        const float away[3] = {0.0f, -0.3f, 0.0f};
        // A fresh one taken over the shoulder and held a few frames, the hand carried across the room at 1 m/s.
        const auto carry = [&](handmag::Frame& f, int frames) {
            Reset();
            f = Base();
            GloveWithMagazine(away, 0.0f, f.glove);
            handmag::Update(f);
            f.squeeze = 1.0f;
            f.shoulder = true;
            bool held = handmag::Update(f).took;
            f.shoulder = false;
            for (int i = 0; i < frames; ++i) {
                f.gripRoom[0] += kDt;
                held = held && handmag::Update(f).holding;
            }
            return held;
        };
        // m1: let go of at the hand's own speed over its last three frames, this one's included (carried at 1 m/s, then 4 m/s this frame: 2 m/s), never
        // cancelled; then nothing for 3 s while the gun lies, a fresh squeeze over the shoulder included.
        handmag::Frame f;
        bool held = carry(f, 6);
        f.gripRoom[0] += 4.0f * kDt;
        f.dropHeld = true;
        f.gunTracked = false;
        handmag::Result r = handmag::Update(f);
        Check(held && r.letGo && !r.putBack && !r.cancelled && !r.holding && !r.mag && !handmag::Holding(), "m1: a fresh one in the hand, the gun let go of and loose has it: let go of, never cancelled");
        std::snprintf(what, sizeof(what), "m1: ... falling at the hand's own speed (%.3f %.3f %.3f m/s; its last three frames 1, 1 and 4 m/s across)", r.dropVelocity[0], r.dropVelocity[1], r.dropVelocity[2]);
        Check(std::fabs(r.dropVelocity[0] - 2.0f) < 1e-3f && std::fabs(r.dropVelocity[1]) < 1e-3f && std::fabs(r.dropVelocity[2]) < 1e-3f, what);
        bool quiet = true;
        for (int i = 0; i < 216; ++i) {
            f.gripRoom[0] += kDt;
            f.squeeze = i == 100 ? 0.0f : 1.0f;  // opened, and squeezed again over the shoulder
            f.shoulder = i > 100;
            const handmag::Result q = handmag::Update(f);
            quiet = quiet && !q.letGo && !q.putBack && !q.cancelled && !q.mag && !q.holding && !q.took && !q.grabbed && !q.seated && !q.leverTaken && *q.note == '\0';
        }
        Check(quiet && !handmag::Holding(), "m1: then 3 s with the gun lying: nothing more, nothing taken (a fresh squeeze over the shoulder included)");
        // The gun back in a hand with that squeeze still held and the hand now on the seated magazine: it was spent while the gun lay (the shoulder's
        // "reached" came and went), so it takes nothing.
        const float onSeated[3] = {0.050f, -0.088f, 0.391f};
        GloveAt(onSeated, false, f.glove);
        f.shoulder = false;
        f.dropHeld = false;
        f.gunTracked = true;
        r = handmag::Update(f);
        Check(!r.grabbed && !r.took && !handmag::Holding(), "m1: ... the gun back in a hand, that squeeze still held, the hand on the seated one: spent while the gun lay, nothing taken");
        // With the release frame between (the gun not seen yet, loose not had it a frame): held through it, then let go of, never cancelled.
        held = carry(f, 6);
        f.gunTracked = false;
        f.gripRoom[0] += kDt;
        const handmag::Result release = handmag::Update(f);
        f.dropHeld = true;
        f.gripRoom[0] += kDt;
        r = handmag::Update(f);
        Check(held && release.holding && !release.letGo && !release.cancelled && r.letGo && !r.cancelled && finite3(r.dropVelocity), "m1: with the release frame between: held through it, then let go of, never cancelled");
        // At the shoulder: back there.
        held = carry(f, 3);
        f.atShoulder = true;
        f.dropHeld = true;
        f.gunTracked = false;
        r = handmag::Update(f);
        Check(held && r.putBack && !r.letGo && !r.cancelled && !handmag::Holding(), "m1: the hand at the shoulder: put back there, never cancelled");
        // The hand not seen that frame: let go of all the same.
        held = carry(f, 3);
        f.left = false;
        f.dropHeld = true;
        f.gunTracked = false;
        r = handmag::Update(f);
        Check(held && r.letGo && !r.cancelled && finite3(r.dropVelocity) && !handmag::Holding(), "m1: the hand not seen that frame: let go of all the same, never cancelled");
        // m1b: the frame KeepTheRifle undoes (loose had no room: the gun back in the hand, dropHeld never comes): a frame with the gun not seen, then seen
        // again: held all along, never let go of or cancelled, drawn in the glove's hold again. Undone before handmag hears of it: simply held.
        held = carry(f, 3);
        f.gunTracked = false;
        const handmag::Result undone = handmag::Update(f);
        f.gunTracked = true;
        r = handmag::Update(f);
        float hold[12], want[12];
        handmag::HoldFor(kGun, hold);
        Compose(f.glove, hold, want);
        float most = 0.0f;
        for (int i = 0; i < 12; ++i)
            most = std::max(most, std::fabs(r.magPose[i] - want[i]));
        Check(held && undone.holding && !undone.letGo && !undone.cancelled && r.holding && r.mag && !r.letGo && !r.cancelled && most < 1e-5f,
              "m1b: the release undone (the gun not seen a frame, then back in the hand): held all along, drawn in the hand again");
        bool same = true;
        for (int i = 0; i < 10; ++i) {
            r = handmag::Update(f);
            same = same && r.holding && r.mag && !r.letGo && !r.cancelled && !r.putBack;
        }
        Check(same, "m1b: ... undone within the frame (the gun seen all along): just held");

        // m2: the seated one taken and rocking out: it stays in (no pull out, nothing cancelled, the gun's own drawn again), the glove straight back at the hand
        // (the gun not seen); the hand going on down takes nothing out. With the gun seen that frame, the glove eases off it instead.
        for (int seen = 0; seen < 2; ++seen) {
            Reset();
            const float at[3] = {0.050f, -0.088f, 0.391f};
            f = Base();
            GloveAt(at, false, f.glove);
            handmag::Update(f);
            f.squeeze = 1.0f;
            const bool grabbed = handmag::Update(f).grabbed;
            bool rocking = false;
            for (int i = 1; i <= 5; ++i) {
                const float down[3] = {at[0], at[1] - 0.004f * i, at[2]};
                GloveAt(down, false, f.glove);
                const handmag::Result q = handmag::Update(f);
                rocking = q.hideSeated && q.mag && !q.pulledOut;
            }
            f.dropHeld = true;
            f.gunTracked = seen == 1;
            r = handmag::Update(f);
            const bool stays = grabbed && rocking && !r.pulledOut && !r.cancelled && !r.hideSeated && !r.mag && !r.letGo && !r.holding && !handmag::Holding();
            // (Not seen that frame: nothing left to ease from either, should the gun be seen the next.)
            bool stale = false;
            if (seen == 0) {
                f.gunTracked = true;
                stale = handmag::Update(f).gloveBlend > 0.0f;
                f.gunTracked = false;
            }
            bool out = stale;
            for (int i = 6; i < 40; ++i) {
                const float down[3] = {at[0], at[1] - 0.004f * i, at[2]};
                GloveAt(down, false, f.glove);
                const handmag::Result q = handmag::Update(f);
                out = out || q.pulledOut || q.cancelled || q.mag || q.hideSeated || q.grabbed || q.flicked;
            }
            if (seen == 0)
                Check(stays && r.gloveBlend == 0.0f && !out, "m2: the seated one rocking out as the gun's let go of: it stays in, nothing cancelled, the glove back at the hand; the hand on down takes nothing");
            else
                Check(stays && r.gloveBlend > 0.5f && !out, "m2: ... the gun seen that frame: the glove eases off the magazine from where it was drawn");
        }

        // m3: the lever drawn all the way, the gun let go of: let go of, it springs home and chambers a round, nothing cancelled, the fist open. Drawn 2 cm: home
        // without one.
        for (int far = 1; far >= 0; --far) {
            Reset();
            const float at[3] = {-0.075f, 0.09f, 0.54f};
            f = Base();
            GloveAt(at, false, f.glove);
            handmag::Update(f);
            f.trigger = 1.0f;
            const bool taken = handmag::Update(f).leverTaken;
            for (int i = 0; i < (far ? 30 : 6); ++i) {
                const float back[3] = {at[0], at[1], at[2] - 0.004f * i};
                GloveAt(back, false, f.glove);
                r = handmag::Update(f);
            }
            const float drawn = r.leverBack;
            f.dropHeld = true;
            f.gunTracked = false;
            r = handmag::Update(f);
            const bool let = taken && r.leverLet && !r.cancelled && !r.fist && std::fabs(r.leverBack - drawn) < 1e-6f && r.charged == (far == 1) && r.gloveBlend == 0.0f;
            float back = r.leverBack;
            bool falling = true, home = false, again = false, springs = false;
            for (int i = 0; i < 10; ++i) {
                const handmag::Result q = handmag::Update(f);
                if (i == 0)
                    springs = q.leverBack > 0.0f && q.leverBack < drawn;  // (on its way home, not there at once)
                falling = falling && q.leverBack <= back;
                back = q.leverBack;
                home = home || q.leverBack == 0.0f;
                again = again || q.leverLet || q.leverTaken || q.fist || q.cancelled;
            }
            std::snprintf(what, sizeof(what), "m3: the lever drawn %.0f mm as the gun's let go of: let go of, %s, nothing cancelled; it springs home", drawn * 1000.0f,
                          far ? "a round chambered" : "no round");
            Check(let && drawn > 0.01f && springs && falling && home && !again, what);
        }

        // m4: a magazine going home by itself as the gun's let go of: let go of where it's drawn, with no speed of its own (the hand moving), never seated,
        // nothing cancelled. With the gun seen that frame, the glove eases off it instead of jumping back. The hand at the shoulder then: back there.
        for (int seen = 0; seen < 3; ++seen) {
            Reset();
            const float near[3] = {0.0f, -0.12f, 0.0f};
            f = Base();
            f.wellEmpty = true;
            GloveWithMagazine(near, 0.0f, f.glove);
            f.squeeze = 1.0f;
            f.shoulder = true;
            handmag::Update(f);
            f.shoulder = false;
            bool caught = false;
            for (int i = 0; i < 10 && !caught; ++i) {
                f.gripRoom[0] += kDt;
                caught = handmag::Update(f).caught;
            }
            f.gripRoom[0] += kDt;
            f.dropHeld = true;
            f.gunTracked = seen == 1;
            f.atShoulder = seen == 2;
            r = handmag::Update(f);
            bool seated = r.seated;
            for (int i = 0; i < 30; ++i) {
                const handmag::Result q = handmag::Update(f);
                seated = seated || q.seated || q.mag || q.cancelled || q.caught || q.letGo || q.putBack;
            }
            const bool still = r.dropVelocity[0] == 0.0f && r.dropVelocity[1] == 0.0f && r.dropVelocity[2] == 0.0f;
            if (seen == 0)
                Check(caught && r.letGo && !r.putBack && !r.cancelled && still && !seated && r.gloveBlend == 0.0f && !handmag::Holding(), "m4: on its way home as the gun's let go of: let go of, no speed of its own, never seated, nothing cancelled");
            else if (seen == 1)
                Check(caught && r.letGo && !r.cancelled && still && !seated && r.gloveBlend > 0.5f, "m4: ... the gun seen that frame: the glove eases off it");
            else
                Check(caught && r.putBack && !r.letGo && !r.cancelled && !seated && !handmag::Holding(), "m4: ... the hand at the shoulder then: put back there, never seated, nothing cancelled");
        }

        // The M240's lid lifted part way as the gun's let go of: it goes back down by itself, as when let go of; nothing cancelled, the fist open.
        {
            Reset();
            handmag::BeltReset();
            f = BaseOf(kM240, kM240FrontGrip);
            handmag::Update(f);
            float at[3];
            LidAt(0.0f, at);
            GloveAt(at, false, f.glove);
            handmag::Update(f);
            f.squeeze = 1.0f;
            const bool taken = handmag::Update(f).lidTaken;
            for (int i = 1; i <= 8; ++i) {
                LidAt(0.03f * i, at);
                GloveAt(at, false, f.glove);
                r = handmag::Update(f);
            }
            const float lifted = r.lid;
            f.dropHeld = true;
            f.gunTracked = false;
            r = handmag::Update(f);
            const bool let = taken && lifted > 0.2f && !r.cancelled && !r.lidUp && !r.fist && !handmag::Holding();
            bool shut = false, up = false;
            for (int i = 0; i < 20; ++i) {
                const handmag::Result q = handmag::Update(f);
                shut = shut || q.lidShut;
                up = up || q.lidUp || q.lidTaken || q.cancelled;
            }
            r = handmag::Update(f);
            std::snprintf(what, sizeof(what), "m3: the M240's lid lifted %.2f rad as the gun's let go of: back down by itself, nothing cancelled", lifted);
            Check(let && shut && !up && r.lid == 0.0f, what);
            handmag::BeltReset();
        }
        // The M240's belt in the hand as the gun's let go of: let go of, it hangs from its box again; brought to the feed with the trigger held, it's never laid.
        {
            const bool hanging = M240BeltHanging(f);
            GloveWithPinch(0.197f, -0.05f, 0.514f, f.glove);
            handmag::Update(f);
            f.trigger = 1.0f;
            const bool taken = handmag::Update(f).beltTaken;
            for (int i = 0; i < 8; ++i) {
                GloveWithPinch(0.22f, 0.06f + 0.01f * i, 0.46f, f.glove);
                r = handmag::Update(f);
            }
            const bool inHand = r.beltDrawn && r.leverFingers == magposes::kBeltPinchFingers;
            f.dropHeld = true;
            f.gunTracked = false;
            r = handmag::Update(f);
            bool asHanging = r.beltDrawn;
            for (int k = 0; k < 5; ++k)
                for (int j = 0; j < 12; ++j)
                    asHanging = asHanging && r.beltBones[k][j] == kBelt.hanging[k][j];
            const bool let = r.beltLet && !r.beltLaid && !r.cancelled && r.leverFingers == nullptr && !handmag::Holding();
            bool laid = false;
            for (int i = 1; i < 40; ++i) {
                const float s = std::min(i / 20.0f, 1.0f);
                GloveWithPinch(0.197f + (0.05f - 0.197f) * s, -0.05f + (0.06f + 0.05f) * s, 0.514f, f.glove);
                const handmag::Result q = handmag::Update(f);
                laid = laid || q.beltLaid || q.beltTaken || !q.beltDrawn || q.cancelled;
            }
            Check(hanging && taken && inHand && let && asHanging && !laid, "m3: the M240's belt in the hand as the gun's let go of: let go of, hanging from its box again, never laid");
            handmag::BeltReset();
        }
    }
    // ---- m6: the claim (handmag::Claims, for FreeHands): a fresh squeeze of the left that the regrip leaves alone, exactly where Update would take something ----
    {
        char what[200];
        float turn[12], glove[12], palm[3];
        HoldTurn(kKh2002, turn);
        // The KH2002, held by the right's front grip alone: the left on its seated magazine is 9.6 cm from the left's pistol grip, inside the regrip's reach.
        Reset();
        handmag::Frame f = BaseOf(kKh2002, kKhFrontGrip);
        GloveTurned(turn, kKh2002.grab, f.glove);
        handmag::Update(f);  // (fed it, its well in)
        Palm(f.glove, palm);
        Check(handmag::Claims(kKh2002, f.gunPose, f.glove, palm), "m6: the KH2002, the hand's middle at its seated magazine's grab point: claimed");
        float seated[12];
        handmag::GloveFor(kKh2002, kGunPose, seated);
        Palm(seated, palm);
        Check(handmag::Claims(kKh2002, f.gunPose, seated, palm), "m6: ... the hand as its fitted hold has it on the seated one: claimed");
        const float ways[6][3] = {{0.2f, 0.0f, 0.0f}, {-0.2f, 0.0f, 0.0f}, {0.0f, 0.2f, 0.0f}, {0.0f, -0.2f, 0.0f}, {0.0f, 0.0f, 0.2f}, {0.0f, 0.0f, -0.2f}};
        int claimedOff = 0;
        for (const auto& w : ways) {
            const float at[3] = {kKh2002.grab[0] + w[0], kKh2002.grab[1] + w[1], kKh2002.grab[2] + w[2]};
            GloveTurned(turn, at, glove);
            Palm(glove, palm);
            claimedOff += handmag::Claims(kKh2002, f.gunPose, glove, palm) ? 1 : 0;
        }
        Check(claimedOff == 0, "m6: ... 20 cm off it, any of six ways: not claimed");
        GloveTurned(turn, kKhLeftPistolGrip, glove);
        Palm(glove, palm);
        bool atGrip = handmag::Claims(kKh2002, f.gunPose, glove, palm);
        GloveAt(kKhLeftPistolGrip, true, glove);
        Palm(glove, palm);
        atGrip = atGrip || handmag::Claims(kKh2002, f.gunPose, glove, palm);
        Check(!atGrip, "m6: ... the hand's middle at the left's pistol grip, palm on its side or up: not claimed (the regrip's)");
        f.wellEmpty = true;
        handmag::Update(f);
        Palm(f.glove, palm);
        Check(!handmag::Claims(kKh2002, f.gunPose, f.glove, palm), "m6: ... the well empty: not claimed");
        f.wellEmpty = false;
        handmag::Update(f);  // (one in again)
        // A squeeze there: Update takes it, as claimed. While the hand has it the squeeze is the module's, wherever the glove is; let go of, it stays in.
        const bool claimed = handmag::Claims(kKh2002, f.gunPose, f.glove, palm);
        f.squeeze = 1.0f;
        handmag::Result r = handmag::Update(f);
        const float farAway[3] = {kKh2002.grab[0] + 0.3f, kKh2002.grab[1], kKh2002.grab[2]};
        GloveTurned(turn, farAway, glove);
        Palm(glove, palm);
        const bool busy = handmag::Claims(kKh2002, f.gunPose, glove, palm);
        Check(claimed && r.grabbed && busy, "m6: ... a squeeze there: Update takes the seated one; while held the squeeze stays the module's");
        f.squeeze = 0.0f;
        r = handmag::Update(f);
        Check(!r.pulledOut && !handmag::Holding() && !handmag::Claims(kKh2002, f.gunPose, glove, palm), "m6: ... let go of: it stays in, nothing claimed 30 cm off");
        // Only for the gun Update was fed: the AN-94's magazine isn't claimed with the KH2002 fed; fed the AN-94, it is.
        float an94Turn[12], an94Glove[12];
        HoldTurn(kGun, an94Turn);
        GloveTurned(an94Turn, kGun.grab, an94Glove);
        Palm(an94Glove, palm);
        const bool otherGun = handmag::Claims(kGun, kGunPose, an94Glove, palm);
        handmag::Frame a = Base();
        std::memcpy(a.glove, an94Glove, sizeof(a.glove));
        handmag::Update(a);
        const bool fedNow = handmag::Claims(kGun, kGunPose, an94Glove, palm);
        a.gun = nullptr;
        handmag::Update(a);
        const bool noGun = handmag::Claims(kGun, kGunPose, an94Glove, palm);
        Check(!otherGun && fedNow && !noGun, "m6: only for the gun Update was last fed: another gun, or none fed, not claimed");
        // Numbers that aren't: not claimed; no palm given: as with one.
        Reset();
        f = BaseOf(kKh2002, kKhFrontGrip);
        GloveTurned(turn, kKh2002.grab, f.glove);
        handmag::Update(f);
        Palm(f.glove, palm);
        const float nan = std::numeric_limits<float>::quiet_NaN(), inf = std::numeric_limits<float>::infinity();
        float badPose[12], badGlove[12];
        std::memcpy(badPose, f.gunPose, sizeof(badPose));
        std::memcpy(badGlove, f.glove, sizeof(badGlove));
        badPose[5] = nan;
        badGlove[7] = inf;
        const float badPalm[3] = {palm[0], nan, palm[2]};
        Check(handmag::Claims(kKh2002, f.gunPose, f.glove, palm) && handmag::Claims(kKh2002, f.gunPose, f.glove, nullptr) && !handmag::Claims(kKh2002, badPose, f.glove, palm) &&
                  !handmag::Claims(kKh2002, f.gunPose, badGlove, palm) && !handmag::Claims(kKh2002, f.gunPose, f.glove, badPalm) && !handmag::Claims(kKh2002, nullptr, f.glove, palm) &&
                  !handmag::Claims(kKh2002, f.gunPose, nullptr, palm),
              "m6: a pose, glove or palm with a number not finite (or none): not claimed; no palm given: as with one");
        // Pure: a take, a pull out, a let go, a fresh one over the shoulder and its way home, frame for frame the same with Claims asked before every frame.
        const auto run = [&](bool ask, std::vector<handmag::Result>& out) {
            Reset();
            handmag::Frame g = BaseOf(kKh2002, kKhFrontGrip);
            for (int i = 0; i < 72; ++i) {
                const float d = i < 2 ? 0.0f : i < 32 ? 0.004f * (i - 2) : i < 36 ? 0.12f : i < 41 ? 0.3f : std::max(0.3f - 0.01f * (i - 41), 0.1f);
                const float off[12] = {1, 0, 0, 0, 0, 1, 0, -d, 0, 0, 1, 0};
                float inView[12];
                Compose(kGunPose, off, inView);
                handmag::GloveFor(kKh2002, inView, g.glove);
                g.squeeze = (i >= 2 && i < 35) || i >= 40 ? 1.0f : 0.0f;
                g.wellEmpty = i >= 32;
                g.shoulder = i == 40;
                g.gripRoom[1] = -d;
                if (ask) {
                    float p[3];
                    Palm(g.glove, p);
                    (void)handmag::Claims(kKh2002, g.gunPose, g.glove, p);
                    (void)handmag::Claims(kGun, g.gunPose, g.glove, p);
                }
                out.push_back(handmag::Update(g));
            }
        };
        std::vector<handmag::Result> plain, asked;
        run(false, plain);
        run(true, asked);
        int differ = 0, events = 0;
        for (size_t i = 0; i < plain.size(); ++i) {
            const handmag::Result &x = plain[i], &y = asked[i];
            const bool flags = x.mag == y.mag && x.hideSeated == y.hideSeated && x.holding == y.holding && x.grabbed == y.grabbed && x.pulledOut == y.pulledOut &&
                               x.took == y.took && x.caught == y.caught && x.seated == y.seated && x.letGo == y.letGo && x.putBack == y.putBack && x.cancelled == y.cancelled &&
                               x.spentSqueeze == y.spentSqueeze && x.toGrip == y.toGrip;
            const bool numbers = std::memcmp(x.magPose, y.magPose, sizeof(x.magPose)) == 0 && std::memcmp(x.glove, y.glove, sizeof(x.glove)) == 0 &&
                                 x.gloveBlend == y.gloveBlend && std::memcmp(x.dropVelocity, y.dropVelocity, sizeof(x.dropVelocity)) == 0;
            differ += flags && numbers ? 0 : 1;
            events += x.grabbed + x.pulledOut + x.letGo + x.took + x.caught + x.seated;
        }
        std::snprintf(what, sizeof(what), "m6: pure: %zu frames (%d events: taken, out, let go, a fresh one, caught, home) the same with Claims asked every frame: %d differ", plain.size(), events, differ);
        Check(differ == 0 && events == 6, what);
        // Against Update itself: gloves turned every way, the hand's middle anywhere within 15 cm of what can be taken, on four guns (the M240 with its lid
        // shut, its lid up, its belt hanging), the well in and out: claimed exactly where a fresh squeeze of the grip takes something.
        const auto agree = [&](const char* name, const handmag::Gun& gun, const float* front, int m240, int samples, uint32_t seed) {
            Rng g{seed};
            int wrong = 0, takes = 0, tried = 0;
            for (int empty = 0; empty < 2; ++empty)
                for (int n = 0; n < samples; ++n) {
                    handmag::Frame s;
                    bool ready = true;
                    if (m240 == 2)
                        ready = M240LidUp(s);
                    else if (m240 == 3)
                        ready = M240BeltHanging(s);
                    else {
                        Reset();
                        if (m240 == 1)
                            handmag::BeltReset();
                        s = BaseOf(gun, front);
                    }
                    float centres[5][3];
                    int count = 0;
                    std::memcpy(centres[count++], gun.grab, sizeof(centres[0]));
                    std::memcpy(centres[count++], gun.lever, sizeof(centres[0]));
                    std::memcpy(centres[count++], front, sizeof(centres[0]));
                    if (gun.belt) {
                        LidAt(m240 >= 2 ? kBelt.lidOpen : 0.0f, centres[count++]);
                        const float tip[3] = {0.197f, -0.05f, 0.514f};
                        std::memcpy(centres[count++], tip, sizeof(centres[0]));
                    }
                    const float* c = centres[std::min(static_cast<int>(g.Next() * count), count - 1)];
                    float at[3], t[12];
                    for (int i = 0; i < 3; ++i)
                        at[i] = c[i] + (g.Next() * 2.0f - 1.0f) * 0.15f;
                    RandomTurn(g, t);
                    GloveTurned(t, at, s.glove);
                    std::memcpy(s.frontGrip, front, sizeof(s.frontGrip));
                    s.wellEmpty = empty == 1;
                    s.squeeze = 0.0f, s.trigger = 0.0f;
                    s.shoulder = false, s.atShoulder = false;
                    handmag::Update(s);
                    float p[3];
                    Palm(s.glove, p);
                    const bool says = handmag::Claims(gun, s.gunPose, s.glove, p);
                    s.squeeze = 1.0f;
                    const handmag::Result taken = handmag::Update(s);
                    const bool took = TookOnGun(taken);
                    ++tried;
                    takes += took ? 1 : 0;
                    if (!ready || says != took) {
                        if (wrong < 3)
                            std::printf("  %s: the hand at (%.3f %.3f %.3f), the well %s: Claims %d, Update took %d (magazine %d, lever %d, lid %d, belt %d)%s\n", name, at[0], at[1], at[2],
                                        empty ? "empty" : "in", says, took, taken.grabbed, taken.leverTaken, taken.lidTaken, taken.beltTaken, ready ? "" : " (not set up)");
                        ++wrong;
                    }
                }
            std::snprintf(what, sizeof(what), "m6: Claims against Update on the %s: %d gloves, %d of them taking something: %d disagree", name, tried, takes, wrong);
            Check(wrong == 0 && takes * 20 >= tried, what);
        };
        agree("KH2002", kKh2002, kKhFrontGrip, 0, 2000, 11u);
        agree("AN-94", kGun, kFrontGrip, 0, 2000, 12u);
        agree("M4A1", kM4, kM4FrontGrip, 0, 2000, 13u);
        agree("M240, its lid shut", kM240, kM240FrontGrip, 1, 1000, 14u);
        agree("M240, its lid up", kM240, kM240FrontGrip, 2, 400, 15u);
        agree("M240, its belt hanging", kM240, kM240FrontGrip, 3, 400, 16u);
        handmag::BeltReset();
        Reset();
    }
    std::printf("%d failures\n", g_failures);
    return g_failures;
}

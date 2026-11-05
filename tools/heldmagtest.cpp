// A gun's magazine and lever worked by the left hand (src/handmag.cpp) on scripted frames at 72 Hz, with the AN-94's numbers and the hold fitted to it: what goes home by itself and from where, what takes the
// seated magazine and what is left for the front grip, what it takes to knock an old one out, and the lever. Prints PASS or FAIL per check; the exit code is the failures.

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "../src/handmag.h"
#include "../src/handpose.h"

#include "../src/pouch.h"

namespace magposes {
#include "../src/magposes/an94.h"
#include "../src/magposes/an94_lever.h"
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
    std::printf("%d failures\n", g_failures);
    return g_failures;
}

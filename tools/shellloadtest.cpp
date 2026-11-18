// A tube shotgun's shells by hand (src/shellload.cpp) without the game: scripted hands at 72 Hz on a gun held still in front of the head, with the M1014's
// places (shellload::kM1014) and the fitted hold (src/shellposes.h). Each check prints PASS or FAIL, then the totals; the exit code is the number failed.
//   T1  a fresh squeeze at the belly takes a shell (in the hand, in the pinch); a hand that arrives squeezed takes none; none with no shells left or a
//       full gun (why is said); a busy hand takes none; let go of at the belly it goes back; anywhere else it falls, at the hand's speed (3 m/s at most)
//   T2  its nose brought to the loading port's mouth goes onto the way in; held still there it stays where it is (no magnet); pushed by the hand it
//       moves with the hand; past the latch it goes home by itself: loaded once, the hand empty; the glove rides it
//   T3  pulled back out before the latch it's in the hand again, and goes in again only once it has been taken 6 cm away; the wrong way round it never
//       goes in; with the tube full it knocks once and stays in the hand; with the game's own reload asked for it stays in the hand; let go of on the
//       way it falls from the port
//   T4  the ejection port: only with the bolt held open on an empty chamber (the count's word), the shell nose forward: it lies there, the hand empty;
//       with the bolt home it stays in the hand
//   T5  the release: a hand arriving on the button with the bolt held open sends it home; hovering in slowly doesn't; not with the bolt home; not for
//       0.3 s after that hand laid a shell into the port; once a visit
//   T6  the handle: taken at the knob and drawn back the bolt follows the hand; drawn most of its way and let go: racked; a short pull isn't, and the
//       bolt goes home in under 0.1 s; held back the gun is out of battery; held open, a tug and let go sends it home, let go without one doesn't
//   T7  the bolt drawn back while the count says held open, home 0.07 s after it says home
//   T8  either hand: the hand holding the gun takes nothing; with the gun in the left hand the right takes a shell, loads it and racks the handle
//   T9  out of use under 0.2 s everything is kept; longer the shell falls; a hand unseen keeps its shell a second where it was, then it falls
//   T10 numbers not finite (a glove, the gun's pose, the frame's length) give nothing not finite and take nothing
// Usage: shellloadtest

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "../src/ammozone.h"
#include "../src/shellload.h"
#include "../src/shellposes.h"

namespace {

using namespace shellload;

int g_failed = 0, g_counted = 0;

void Check(bool ok, const char* row, const char* format, ...) {
    char what[512];
    va_list args;
    va_start(args, format);
    std::vsnprintf(what, sizeof(what), format, args);
    va_end(args);
    ++g_counted;
    if (!ok)
        ++g_failed;
    std::printf("%s %s %s\n", ok ? "PASS" : "FAIL", row, what);
}

constexpr float kDt = 1.0f / 72.0f;
const Gun& kGun = kM1014;

void Compose(const float* a, const float* b, float* out) {
    float r[12];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            r[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        r[i * 4 + 3] = a[i * 4] * b[3] + a[i * 4 + 1] * b[7] + a[i * 4 + 2] * b[11] + a[i * 4 + 3];
    }
    std::memcpy(out, r, sizeof(r));
}
void Transform(const float* m, const float* p, float* out) {
    const float r[3] = {m[0] * p[0] + m[1] * p[1] + m[2] * p[2] + m[3], m[4] * p[0] + m[5] * p[1] + m[6] * p[2] + m[7], m[8] * p[0] + m[9] * p[1] + m[10] * p[2] + m[11]};
    std::memcpy(out, r, sizeof(r));
}
float Distance(const float* a, const float* b) {
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}
bool Finite(const float* v, int n) {
    for (int i = 0; i < n; ++i)
        if (!std::isfinite(v[i]))
            return false;
    return true;
}

// The gun held still: its muzzle away from the eye (the view's -z), its left the view's left, its receiver half a metre out and a little down. The head
// at the room's origin looking along -z (the view is the room); the neck where pouch.cpp has it, the body heading the same way.
const float kGunPose[12] = {-1, 0, 0, 0.15f, 0, 1, 0, -0.30f, 0, 0, -1, -0.05f};
const float kIdentity[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
const float kNeck[3] = {0.0f, -0.075f, 0.08f};
// A point of the room at the belly's middle, and one well away from the belly and the gun.
const float kBellyRoom[3] = {kNeck[0] + ammozone::kBelly[0], kNeck[1] + ammozone::kBelly[1], kNeck[2] - ammozone::kBelly[2]};
const float kAway[3] = {-0.30f, -0.20f, -0.25f};

// The scene: what the count says, and each hand.
struct Scene {
    Frame f;
    Scene() {
        f.dt = kDt;
        f.active = true;
        f.gun = &kGun;
        std::memcpy(f.gunPose, kGunPose, sizeof(kGunPose));
        std::memcpy(f.headPose, kIdentity, sizeof(kIdentity));
        std::memcpy(f.neck, kNeck, sizeof(kNeck));
        f.haveBody = true;
        f.gunHand = 1;
        f.take = Take::kYes;
        f.canLoad = true;
        for (int s = 0; s < 2; ++s)
            HandAtPoint(s, kAway, 0.0f);
    }
    // A hand with its middle at a point of the view, its glove unturned.
    void HandAtPoint(int s, const float* at, float squeeze) {
        HandIn& h = f.hand[s];
        h.tracked = true;
        const float bind[3] = {s == 1 ? -kHandInGlove[0] : kHandInGlove[0], kHandInGlove[1], kHandInGlove[2]};
        const float glove[12] = {1, 0, 0, at[0] - bind[0], 0, 1, 0, at[1] - bind[1], 0, 0, 1, at[2] - bind[2]};
        std::memcpy(h.glove, glove, sizeof(glove));
        std::memcpy(h.gripRoom, at, 3 * sizeof(float));
        h.squeeze = squeeze;
        h.touches = 0;
    }
    // A hand holding a shell whose bone is at shellOnGun on the gun's frame.
    void HandWithShell(int s, const float* shellOnGun, float squeeze) {
        HandIn& h = f.hand[s];
        float view[12];
        Compose(kGunPose, shellOnGun, view);
        GloveFor(s, view, h.glove);
        h.tracked = true;
        h.squeeze = squeeze;
        const float bind[3] = {s == 1 ? -kHandInGlove[0] : kHandInGlove[0], kHandInGlove[1], kHandInGlove[2]};
        Transform(h.glove, bind, h.gripRoom);
        h.touches = 0;
    }
    // A hand with its middle at a point of the gun's frame.
    void HandOnGun(int s, const float* onGun, float squeeze) {
        float view[3];
        Transform(kGunPose, onGun, view);
        HandAtPoint(s, view, squeeze);
    }
    Result Step() { return Update(f); }
    Result Steps(int n) {
        Result r;
        for (int i = 0; i < n; ++i)
            r = Update(f);
        return r;
    }
};

void Start() {
    Clear();
    SetHold(shellposes::kShellInGlove, shellposes::kFingers);
}

// The hand at work takes a shell at the belly: let go, then squeezed there.
Result TakeShell(Scene& sc, int s) {
    sc.HandAtPoint(s, kBellyRoom, 0.0f);
    sc.Step();
    sc.HandAtPoint(s, kBellyRoom, 1.0f);
    return sc.Step();
}

// A shell's bone on the gun moved by a shift (its turn kept).
void Shifted(const float* on, float x, float y, float z, float* out) {
    std::memcpy(out, on, 12 * sizeof(float));
    out[3] += x, out[7] += y, out[11] += z;
}

void T1() {
    Start();
    {
        Scene sc;
        const Result r = TakeShell(sc, 0);
        Check(r.took && r.handShell && r.holding && r.spentSqueeze && r.side == 0 && r.fingers == shellposes::kFingers && r.fingersShare > 0.99f, "T1a",
              "a fresh squeeze of the left hand at the belly takes a shell: took %d, in the hand %d, the pinch in by %.2f", r.took, r.handShell, r.fingersShare);
        float base[3], want[3], view[12];
        Compose(sc.f.hand[0].glove, shellposes::kShellInGlove, view);
        Transform(view, kGun.baseOnBone, want);
        Transform(r.shellView, kGun.baseOnBone, base);
        Check(Distance(base, want) < 1e-4f, "T1b", "it's drawn in the glove as the hold has it (its base %.1f mm off)", Distance(base, want) * 1000);
        // Let go of at the belly: back.
        sc.HandAtPoint(0, kBellyRoom, 0.0f);
        const Result back = sc.Step();
        Check(back.putBack && !back.letGo && !back.handShell && !Holding(), "T1c", "let go of at the belly it goes back (put back %d, falls %d)", back.putBack, back.letGo);
    }
    Start();
    {
        Scene sc;
        sc.HandAtPoint(0, kBellyRoom, 1.0f);  // arrives squeezed
        const Result r = sc.Steps(5);
        Check(!r.took && !Holding(), "T1d", "a hand that arrives at the belly already squeezed takes none");
    }
    Start();
    {
        Scene sc;
        sc.f.take = Take::kNoShells;
        const Result none = TakeShell(sc, 0);
        const bool said = none.squeezed && std::strcmp(none.note, "no shells left") == 0;
        sc.HandAtPoint(0, kBellyRoom, 0.0f);
        sc.Step();
        sc.f.take = Take::kFull;
        sc.HandAtPoint(0, kBellyRoom, 1.0f);
        const Result full = sc.Step();
        Check(!none.took && said && !full.took && full.squeezed && std::strcmp(full.note, "the tube is full") == 0 && !Holding(), "T1e",
              "none with no shells left (\"%s\") or a full gun (\"%s\")", none.note, full.note);
        sc.f.take = Take::kYes;
        sc.HandAtPoint(0, kBellyRoom, 0.0f);
        sc.Step();
        sc.f.hand[0].busy = true;
        sc.HandAtPoint(0, kBellyRoom, 1.0f);
        const Result busy = sc.Step();
        Check(!busy.took && !Holding(), "T1f", "a hand busy with something else takes none");
    }
    Start();
    {
        Scene sc;
        TakeShell(sc, 0);
        // Carried away from the belly at 5 m/s to the left, then let go of.
        float at[3] = {kBellyRoom[0], kBellyRoom[1] + 0.25f, kBellyRoom[2] - 0.2f};
        for (int i = 0; i < 6; ++i) {
            at[0] -= 5.0f * kDt;
            sc.HandAtPoint(0, at, 1.0f);
            sc.Step();
        }
        at[0] -= 5.0f * kDt;
        sc.HandAtPoint(0, at, 0.0f);
        const Result r = sc.Step();
        const float speed = std::sqrt(r.dropVelocityRoom[0] * r.dropVelocityRoom[0] + r.dropVelocityRoom[1] * r.dropVelocityRoom[1] + r.dropVelocityRoom[2] * r.dropVelocityRoom[2]);
        Check(r.letGo && r.dropFromHand && !r.putBack && !Holding() && r.dropVelocityRoom[0] < -2.5f && speed <= 3.001f && Finite(r.dropView, 12), "T1g",
              "let go of away from the belly it falls, at the hand's speed capped at 3 m/s (%.2f m/s, %.2f to the right)", speed, r.dropVelocityRoom[0]);
    }
}

// A shell taken and brought to the loading port's mouth (the way's start); the result of the frame it arrives.
Result ToMouth(Scene& sc, int s) {
    TakeShell(sc, s);
    float on[12], off[12];
    ShellOnPath(kGun, 0.0f, on);
    Shifted(on, 0.0f, -0.12f, -0.05f, off);  // from below and behind it
    sc.HandWithShell(s, off, 1.0f);
    sc.Step();
    sc.HandWithShell(s, on, 1.0f);
    return sc.Step();
}

void T2() {
    Start();
    Scene sc;
    const Result in = ToMouth(sc, 0);
    Check(in.entered && in.lineShell && !in.handShell && in.holding, "T2a", "its nose at the loading port's mouth: onto the way in (entered %d, drawn on the gun %d)", in.entered,
          in.lineShell);
    // Held still: no magnet.
    const Result still = sc.Steps(36);
    float base0[3], baseNow[3], on[12];
    ShellOnPath(kGun, 0.0f, on);
    Transform(on, kGun.baseOnBone, base0);
    Transform(still.shellOnGun, kGun.baseOnBone, baseNow);
    Check(still.lineShell && !still.loaded && !still.caught && Distance(base0, baseNow) < 0.001f, "T2b",
          "held still at the mouth for half a second it stays where it is (%.1f mm on)", Distance(base0, baseNow) * 1000);
    Check(still.gloveBlend > 0.99f && still.side == 0, "T2c", "the glove rides it (on the gun by %.2f)", still.gloveBlend);
    // Pushed by the hand, 0.25 m/s along the way.
    const float length = PathLength(kGun);
    bool followed = true, loadedEarly = false;
    float u = 0.0f;
    Result r;
    int caughtAt = -1, loadedAt = -1, loadedTimes = 0;
    for (int i = 0; i < 80; ++i) {
        u = std::fmin(u + 0.25f * kDt, length - kGun.latch - 0.002f);
        ShellOnPath(kGun, u, on);
        sc.HandWithShell(0, on, 1.0f);
        r = sc.Step();
        float want[3], got[3];
        Transform(on, kGun.baseOnBone, want);
        Transform(r.shellOnGun, kGun.baseOnBone, got);
        followed = followed && r.lineShell && Distance(want, got) < 0.0015f;
        loadedEarly = loadedEarly || r.loaded || r.caught;
    }
    Check(followed && !loadedEarly, "T2d", "pushed by the hand to 2 mm short of the latch it moves with the hand (within 1.5 mm) and isn't counted");
    for (int i = 0; i < 30; ++i) {
        u = std::fmin(u + 0.25f * kDt, length);
        ShellOnPath(kGun, u, on);
        sc.HandWithShell(0, on, 1.0f);
        r = sc.Step();
        if (r.caught && caughtAt < 0)
            caughtAt = i;
        if (r.loaded) {
            ++loadedTimes;
            if (loadedAt < 0)
                loadedAt = i;
        }
    }
    Check(caughtAt >= 0 && loadedAt > caughtAt && loadedTimes == 1 && !Holding() && !r.lineShell && !r.handShell, "T2e",
          "past the latch it goes home by itself: caught at frame %d, loaded at %d, once (%d), the hand empty", caughtAt, loadedAt, loadedTimes);
    const Result after = sc.Steps(20);
    Check(after.gloveBlend < 0.01f && after.fingersShare < 0.05f, "T2f", "the glove and its fingers ease back to the hand (%.2f, %.2f)", after.gloveBlend, after.fingersShare);
}

void T3() {
    Start();
    {
        Scene sc;
        ToMouth(sc, 0);
        float on[12], back[12];
        ShellOnPath(kGun, 0.02f, on);
        sc.HandWithShell(0, on, 1.0f);
        sc.Steps(3);
        // Pulled back out: 3 cm back past the start, along the way's own line there.
        ShellOnPath(kGun, 0.0f, on);
        Shifted(on, 0.0f, -0.03f * 0.93f, -0.03f * 0.36f, back);
        sc.HandWithShell(0, back, 1.0f);
        const Result out = sc.Step();
        Check(out.pulledOut && out.handShell && !out.lineShell && Holding(), "T3a", "pulled back out before the latch it's in the hand again (pulled out %d)", out.pulledOut);
        // Straight back to the mouth: not in again.
        sc.HandWithShell(0, on, 1.0f);
        const Result again = sc.Steps(4);
        Check(!again.lineShell && again.handShell, "T3b", "brought straight back to the mouth it doesn't go in again");
        // Taken 8 cm away, then back: in.
        Shifted(on, 0.0f, -0.08f, 0.0f, back);
        sc.HandWithShell(0, back, 1.0f);
        sc.Step();
        sc.HandWithShell(0, on, 1.0f);
        const Result in = sc.Step();
        Check(in.entered && in.lineShell, "T3c", "taken 8 cm away and brought back it goes in");
    }
    Start();
    {
        Scene sc;
        TakeShell(sc, 0);
        // The wrong way round: its base where its nose would be, its nose pointing back down out of the port.
        float on[12], flipped[12];
        ShellOnPath(kGun, 0.0f, on);
        const float turn[12] = {1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0};  // half a turn about x
        Compose(on, turn, flipped);
        float nose[3], wantNose[3];
        Transform(on, kGun.noseOnBone, wantNose);
        Transform(flipped, kGun.noseOnBone, nose);
        flipped[3] += wantNose[0] - nose[0], flipped[7] += wantNose[1] - nose[1], flipped[11] += wantNose[2] - nose[2];
        sc.HandWithShell(0, flipped, 1.0f);
        bool entered = false;
        Result r;
        for (int i = 0; i < 5; ++i) {
            r = sc.Step();
            entered = entered || r.entered || r.lineShell;
        }
        Check(!entered && r.handShell, "T3d", "the wrong way round (its nose at the mouth, pointing back out) it never goes in");
    }
    Start();
    {
        Scene sc;
        sc.f.canLoad = false, sc.f.tubeFull = true;
        const Result knock = ToMouth(sc, 0);
        const Result held = sc.Steps(10);
        Check(knock.fullKnock && !knock.entered && !held.fullKnock && held.handShell && !held.lineShell, "T3e",
              "the tube full: it stops at the lifter with one knock and stays in the hand");
    }
    Start();
    {
        Scene sc;
        sc.f.gameReload = true;
        const Result r = ToMouth(sc, 0);
        Check(!r.entered && !r.fullKnock && r.handShell, "T3f", "the game's own reload asked for: it stays in the hand");
    }
    Start();
    {
        Scene sc;
        ToMouth(sc, 0);
        float on[12];
        ShellOnPath(kGun, 0.04f, on);
        sc.HandWithShell(0, on, 1.0f);
        sc.Steps(4);
        sc.HandWithShell(0, on, 0.0f);
        const Result r = sc.Step();
        float want[12];
        Compose(kGunPose, on, want);
        Check(r.letGo && !r.dropFromHand && !r.loaded && !Holding() && Distance(&r.dropView[3], &want[3]) < 0.02f, "T3g",
              "let go of part way up it falls from the port, not counted (falls %d, from the hand %d)", r.letGo, r.dropFromHand);
    }
}

void T4() {
    Start();
    Scene sc;
    TakeShell(sc, 0);
    float port[12], above[12];
    ShellInPort(kGun, port);
    Shifted(port, -0.02f, 0.02f, 0.0f, above);  // just outside the port's opening, on the gun's right, a little up
    sc.HandWithShell(0, above, 1.0f);
    const Result home = sc.Steps(5);
    Check(!home.portDrop && home.handShell, "T4a", "with the bolt home a shell at the ejection port stays in the hand");
    sc.f.portOpen = true, sc.f.locked = true;
    const Result drop = sc.Step();
    Check(drop.portDrop && !drop.handShell && !Holding() && !drop.letGo, "T4b", "with the bolt held open on an empty chamber it's laid into the port: the hand empty");
    // Nose backwards: not laid in.
    Start();
    Scene sc2;
    sc2.f.portOpen = true, sc2.f.locked = true;
    TakeShell(sc2, 0);
    const float turn[12] = {-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0};  // half a turn about y
    float flipped[12], m0[3], m1[3];
    Compose(above, turn, flipped);
    const float mid[3] = {(kGun.baseOnBone[0] + kGun.noseOnBone[0]) / 2, (kGun.baseOnBone[1] + kGun.noseOnBone[1]) / 2, (kGun.baseOnBone[2] + kGun.noseOnBone[2]) / 2};
    Transform(above, mid, m0);
    Transform(flipped, mid, m1);
    flipped[3] += m0[0] - m1[0], flipped[7] += m0[1] - m1[1], flipped[11] += m0[2] - m1[2];
    sc2.HandWithShell(0, flipped, 1.0f);
    const Result back = sc2.Steps(5);
    Check(!back.portDrop && back.handShell, "T4c", "its nose towards the shooter it isn't laid in");
}

void T5() {
    const float off[3] = {kGun.release[0] - 0.10f, kGun.release[1], kGun.release[2]};  // 10 cm out to the gun's right of the button
    // Arriving on the button at 0.5 m/s with the bolt held open.
    Start();
    {
        Scene sc;
        sc.f.locked = true;
        float at[3] = {off[0], off[1], off[2]};
        int pressed = 0;
        for (int i = 0; i < 20; ++i) {
            at[0] = std::fmin(at[0] + 0.5f * kDt, kGun.release[0] - 0.005f);
            sc.HandOnGun(0, at, 0.0f);
            pressed += sc.Step().released ? 1 : 0;
        }
        const Result rest = sc.Steps(30);
        Check(pressed == 1 && !rest.released, "T5a", "a hand arriving on the release at 0.5 m/s with the bolt held open presses it, once (%d)", pressed);
        // Off it and on again: again.
        sc.HandOnGun(0, off, 0.0f);
        sc.Steps(3);
        int again = 0;
        for (int i = 0; i < 20; ++i) {
            at[0] = i == 0 ? off[0] : std::fmin(at[0] + 0.5f * kDt, kGun.release[0] - 0.005f);
            sc.HandOnGun(0, at, 0.0f);
            again += sc.Step().released ? 1 : 0;
        }
        Check(again == 1, "T5b", "off it and on again presses it again (%d)", again);
    }
    // Hovering in slowly: no.
    Start();
    {
        Scene sc;
        sc.f.locked = true;
        float at[3] = {kGun.release[0] - 0.05f, off[1], off[2]};
        int pressed = 0;
        for (int i = 0; i < 200; ++i) {
            at[0] = std::fmin(at[0] + 0.02f * kDt, kGun.release[0] - 0.005f);
            sc.HandOnGun(0, at, 0.0f);
            pressed += sc.Step().released ? 1 : 0;
        }
        Check(pressed == 0, "T5c", "a hand drifting onto it at 2 cm/s doesn't press it (%d)", pressed);
        // A poke while on it: 2 cm out and back in at 0.5 m/s.
        for (int i = 0; i < 3; ++i) {
            at[0] -= 0.5f * kDt;
            sc.HandOnGun(0, at, 0.0f);
            pressed += sc.Step().released ? 1 : 0;
        }
        for (int i = 0; i < 3; ++i) {
            at[0] += 0.5f * kDt;
            sc.HandOnGun(0, at, 0.0f);
            pressed += sc.Step().released ? 1 : 0;
        }
        Check(pressed == 1, "T5d", "a poke of the hand on it presses it (%d)", pressed);
    }
    // The bolt home: nothing.
    Start();
    {
        Scene sc;
        float at[3] = {off[0], off[1], off[2]};
        int pressed = 0;
        for (int i = 0; i < 20; ++i) {
            at[0] = std::fmin(at[0] + 0.5f * kDt, kGun.release[0] - 0.005f);
            sc.HandOnGun(0, at, 0.0f);
            pressed += sc.Step().released ? 1 : 0;
        }
        Check(pressed == 0, "T5e", "with the bolt home the release does nothing");
    }
    // Right after a port drop by the same hand: not for 0.3 s.
    Start();
    {
        Scene sc;
        sc.f.portOpen = true, sc.f.locked = true;
        TakeShell(sc, 0);
        float port[12], above[12];
        ShellInPort(kGun, port);
        Shifted(port, -0.02f, 0.02f, 0.0f, above);
        sc.HandWithShell(0, above, 1.0f);
        const Result drop = sc.Step();
        sc.f.portOpen = false;  // (the count: a shell lies there now)
        // The hand's own points at the button at once, moving.
        int early = 0, late = 0;
        float at[3] = {kGun.release[0] - 0.04f, kGun.release[1], kGun.release[2]};
        for (int i = 0; i < 18; ++i) {  // 0.25 s
            at[0] = std::fmin(at[0] + 0.5f * kDt, kGun.release[0] - 0.005f);
            sc.HandOnGun(0, at, 0.0f);
            early += sc.Step().released ? 1 : 0;
        }
        sc.Steps(8);
        for (int i = 0; i < 6; ++i) {  // off and on again
            at[0] = i < 3 ? at[0] - 0.5f * kDt * 4 : at[0] + 0.5f * kDt * 4;
            sc.HandOnGun(0, at, 0.0f);
            late += sc.Step().released ? 1 : 0;
        }
        Check(drop.portDrop && early == 0 && late == 1, "T5f", "not for 0.3 s after that hand laid a shell into the port (%d then), then it does (%d)", early, late);
    }
}

// The hand at work brought to the handle's knob (the bolt back by `back`) and squeezed there.
Result TakeHandle(Scene& sc, int s, float back) {
    const float knob[3] = {kGun.handle[0] - 0.02f, kGun.handle[1], kGun.handle[2] - back};
    sc.HandOnGun(s, knob, 0.0f);
    sc.Step();
    sc.HandOnGun(s, knob, 1.0f);
    return sc.Step();
}

void T6() {
    Start();
    {
        Scene sc;
        const Result take = TakeHandle(sc, 0, 0.0f);
        Check(take.handleTaken && take.spentSqueeze && Holding() && !take.took, "T6a", "a fresh squeeze at the knob takes the handle");
        bool follows = true, held = false;
        float at[3] = {kGun.handle[0] - 0.02f, kGun.handle[1], kGun.handle[2]};
        Result r;
        for (int i = 0; i < 24; ++i) {  // drawn back at 0.25 m/s: 6 cm in 0.24 s, then on against its stop
            at[2] -= 0.25f * kDt;
            sc.HandOnGun(0, at, 1.0f);
            r = sc.Step();
            const float want = std::fmin(kGun.handle[2] - at[2], kGun.travel + 0.004f);
            follows = follows && std::fabs(r.boltBack - want) < 0.001f;
            held = held || r.handleHeld;
        }
        Check(follows && held && r.fist && r.gloveBlend > 0.99f, "T6b", "drawn back, the bolt follows the hand to its stop (%.1f mm), the gun out of battery, the glove on it", r.boltBack * 1000);
        sc.HandOnGun(0, at, 0.0f);
        const Result let = sc.Step();
        Check(let.handleLet && let.racked && !let.tugged && let.handlePulled >= kGun.travel - 0.001f && !Holding(), "T6c", "drawn all its way and let go: racked (drawn %.0f mm)",
              let.handlePulled * 1000);
        // The count's answer: a shell fed, the bolt home.
        const Result home = sc.Steps(10);
        Check(home.boltBack == 0.0f && !home.handleHeld, "T6d", "the bolt home again within 10 frames");
    }
    Start();
    {
        Scene sc;
        TakeHandle(sc, 0, 0.0f);
        float at[3] = {kGun.handle[0] - 0.02f, kGun.handle[1], kGun.handle[2] - 0.03f};
        sc.HandOnGun(0, at, 1.0f);
        const Result part = sc.Steps(3);
        sc.HandOnGun(0, at, 0.0f);
        const Result let = sc.Step();
        const Result soon = sc.Steps(7);  // 0.1 s
        Check(std::fabs(part.boltBack - 0.03f) < 0.001f && let.handleLet && !let.racked && soon.boltBack == 0.0f, "T6e",
              "drawn 3 cm and let go: not racked, the bolt home in under 0.1 s (%.1f mm then)", soon.boltBack * 1000);
    }
    Start();
    {
        Scene sc;
        sc.f.locked = true;
        sc.Steps(2);
        const Result take = TakeHandle(sc, 0, kGun.travel);
        float at[3] = {kGun.handle[0] - 0.02f, kGun.handle[1], kGun.handle[2] - kGun.travel - 0.01f};
        sc.HandOnGun(0, at, 1.0f);
        const Result tug = sc.Steps(2);
        sc.HandOnGun(0, at, 0.0f);
        const Result let = sc.Step();
        Check(take.handleTaken && tug.handleHeld && let.handleLet && let.tugged && !let.racked, "T6f", "held open: the handle tugged 1 cm further and let go sends the bolt home");
        // Let go without a tug: nothing.
        sc.Steps(3);
        TakeHandle(sc, 0, kGun.travel);
        at[2] = kGun.handle[2] - kGun.travel;
        sc.HandOnGun(0, at, 0.0f);
        const Result plain = sc.Step();
        const Result stays = sc.Steps(10);
        Check(plain.handleLet && !plain.tugged && !plain.racked && std::fabs(stays.boltBack - kGun.travel) < 1e-5f, "T6g", "let go without a tug it stays held open");
    }
}

void T7() {
    Start();
    Scene sc;
    sc.f.locked = true;
    const Result open = sc.Steps(2);
    sc.f.locked = false;
    const Result one = sc.Step();
    const Result five = sc.Steps(5);
    Check(std::fabs(open.boltBack - kGun.travel) < 1e-6f && one.boltBack < kGun.travel && one.boltBack > 0.0f && five.boltBack == 0.0f && open.active, "T7",
          "the bolt drawn back while held open (%.0f mm), on its way a frame after the count says home (%.0f mm), home within 6 frames", open.boltBack * 1000, one.boltBack * 1000);
}

void T8() {
    Start();
    {
        Scene sc;
        const Result r = TakeShell(sc, 1);  // the right holds the gun
        Check(!r.took && !Holding(), "T8a", "the hand holding the gun takes nothing at the belly");
    }
    Start();
    {
        Scene sc;
        sc.f.gunHand = 0;
        const Result in = ToMouth(sc, 1);
        const float length = PathLength(kGun);
        float u = 0.0f, on[12];
        int loaded = 0;
        Result r;
        bool rides = true;
        for (int i = 0; i < 60; ++i) {
            u = std::fmin(u + 0.25f * kDt, length);
            ShellOnPath(kGun, u, on);
            sc.HandWithShell(1, on, 1.0f);
            r = sc.Step();
            loaded += r.loaded ? 1 : 0;
            if (r.lineShell) {
                // The glove on the gun holds the shell where it's drawn (the right's hold: the left's flipped).
                float gloveView[12], shellView[12], want[12], a[3], b[3];
                Compose(kGunPose, r.gloveOnGun, gloveView);
                float mirrored[12];
                std::memcpy(mirrored, shellposes::kShellInGlove, sizeof(mirrored));
                mirrored[1] = -mirrored[1], mirrored[2] = -mirrored[2], mirrored[3] = -mirrored[3], mirrored[4] = -mirrored[4], mirrored[8] = -mirrored[8];
                Compose(gloveView, mirrored, shellView);
                Compose(kGunPose, r.shellOnGun, want);
                Transform(shellView, kGun.noseOnBone, a);
                Transform(want, kGun.noseOnBone, b);
                rides = rides && Distance(a, b) < 1e-3f && r.side == 1;
            }
        }
        Check(in.entered && in.side == 1 && loaded == 1 && rides && !Holding(), "T8b",
              "the gun in the left hand: the right takes a shell and pushes it home (entered %d, loaded %d), its glove's hold the left's flipped", in.entered, loaded);
        const Result take = TakeHandle(sc, 1, 0.0f);
        float at[3] = {kGun.handle[0] - 0.02f, kGun.handle[1], kGun.handle[2] - kGun.travel};
        sc.HandOnGun(1, at, 1.0f);
        sc.Steps(3);
        sc.HandOnGun(1, at, 0.0f);
        const Result let = sc.Step();
        Check(take.handleTaken && take.side == 1 && let.racked, "T8c", "... and racks the handle");
    }
}

void T9() {
    Start();
    {
        Scene sc;
        TakeShell(sc, 0);
        sc.HandAtPoint(0, kAway, 1.0f);
        sc.Step();
        sc.f.active = false;
        const Result brief = sc.Steps(10);  // 0.14 s
        sc.f.active = true;
        const Result back = sc.Step();
        Check(!brief.letGo && back.handShell && Holding(), "T9a", "out of use for 0.14 s the shell is kept");
        sc.f.active = false;
        bool fell = false;
        Result r;
        for (int i = 0; i < 20; ++i) {
            r = sc.Step();
            fell = fell || (r.letGo && Finite(r.dropView, 12));
        }
        Check(fell && !Holding(), "T9b", "out of use for longer it falls");
    }
    Start();
    {
        Scene sc;
        TakeShell(sc, 0);
        sc.HandAtPoint(0, kAway, 1.0f);
        const Result seen = sc.Step();
        sc.f.hand[0].tracked = false;
        const Result unseen = sc.Steps(60);  // 0.83 s
        float a[3], b[3];
        Transform(seen.shellView, kGun.baseOnBone, a);
        Transform(unseen.shellView, kGun.baseOnBone, b);
        bool fell = false;
        for (int i = 0; i < 20; ++i)
            fell = fell || sc.Step().letGo;
        Check(unseen.handShell && Distance(a, b) < 1e-4f && fell && !Holding(), "T9c", "a hand unseen keeps its shell where it was for a second, then it falls");
    }
}

void T10() {
    Start();
    Scene sc;
    TakeShell(sc, 0);
    sc.f.hand[0].glove[3] = std::nanf("");
    Result r = sc.Steps(3);
    bool finite = Finite(r.shellView, 12) && Finite(r.gloveOnGun, 12) && Finite(&r.boltBack, 1) && Finite(r.shellOnGun, 12) && Finite(r.dropView, 12);
    sc.HandAtPoint(0, kAway, 1.0f);
    sc.f.gunPose[5] = std::nanf("");
    r = sc.Steps(3);
    finite = finite && Finite(r.shellView, 12) && Finite(r.gloveOnGun, 12) && Finite(&r.boltBack, 1) && !r.entered && !r.portDrop && !r.released && !r.handleTaken;
    std::memcpy(sc.f.gunPose, kGunPose, sizeof(kGunPose));
    sc.f.dt = std::nanf("");
    r = sc.Steps(3);
    finite = finite && Finite(r.shellView, 12) && Finite(&r.boltBack, 1) && Finite(&r.gloveBlend, 1) && Finite(&r.fingersShare, 1);
    Check(finite && Holding(), "T10", "a glove, the gun's pose or the frame's length not finite: nothing not finite comes out, the shell stays in the hand");
}

}  // namespace

int main() {
    T1();
    T2();
    T3();
    T4();
    T5();
    T6();
    T7();
    T8();
    T9();
    T10();
    std::printf("%d of %d checks failed\n", g_failed, g_counted);
    return g_failed;
}

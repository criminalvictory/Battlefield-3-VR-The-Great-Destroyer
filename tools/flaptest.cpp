// The Stinger reloaded by the left hand (src/flap.cpp, as hands.cpp runs it), on scripted frames at 72 Hz, without the game: fired, its aerial folds by
// itself; the hand takes the folded aerial on the tube's right and lifts it, and it springs open with the next missile ready; let go of before it's up,
// or the hand off the way, it stays folded. Free hands: the Stinger let go of mid-lift (it falls, flies into a hand, or the hand holding it is lost) lets
// go of the lift and it stays folded, the glove easing back from where it was; nothing is taken off a Stinger no hand holds.
// The view is the head's own space (x right, y up, z back). The Stinger is held still in it, pointing ahead (its frame x left, y up, z to its muzzle),
// turned a little off the axes. The tables are the game's own (src/magposes/fim92_flap.h).
// Usage: flaptest   The exit code is the number of failed checks.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "../src/flap.h"
#include "../src/magposes.h"

namespace {

constexpr float kDt = 1.0f / 72.0f;
constexpr float kHandInGlove[3] = {0.5363f, 1.0976f, 0.2092f};  // the hand's middle in the gloves' bind space, as flap.cpp has it

int g_failed = 0, g_checks = 0;

void Check(bool ok, const char* what) {
    ++g_checks;
    if (!ok) {
        ++g_failed;
        std::printf("FAIL: %s\n", what);
    }
}

const flap::Aerial kAerial{magposes::kFim92FoldKeys,  magposes::kFim92LiftKeys,  magposes::kFim92SpringKeys,
                           magposes::kFim92FoldBones, magposes::kFim92LiftHand,  magposes::kFim92LiftGlove,
                           &magposes::kFim92LiftFingers[0][0][0], magposes::kFim92LiftBones, magposes::kFim92SpringBones};

void Transform(const float* m, const float* p, float* out) {
    for (int i = 0; i < 3; ++i)
        out[i] = m[i * 4] * p[0] + m[i * 4 + 1] * p[1] + m[i * 4 + 2] * p[2] + m[i * 4 + 3];
}

float Distance(const float* a, const float* b) {
    const float d[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    return std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
}

float Apart12(const float* a, const float* b) {
    float most = 0.0f;
    for (int i = 0; i < 12; ++i)
        most = std::fmax(most, std::fabs(a[i] - b[i]));
    return most;
}

bool Finite(const float* m, int n) {
    for (int i = 0; i < n; ++i)
        if (!std::isfinite(m[i]))
            return false;
    return true;
}

// The base frame: the Stinger ahead and a little to the right, on the right shoulder, ready; the left hand down, open.
flap::Frame Base() {
    flap::Frame f;
    f.dt = kDt;
    f.aerial = &kAerial;
    f.gunTracked = true;
    // 10 degrees round the view's up: x left, y up, z to the muzzle, in the view's space.
    const float c = std::cos(0.17f), s = std::sin(0.17f);
    const float pose[12] = {-c, 0, -s, 0.15f,  //
                            0,  1, 0,  -0.25f,  //
                            s,  0, -c, 0.25f};
    std::memcpy(f.gunPose, pose, sizeof(pose));
    f.left = true;
    const float glove[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    std::memcpy(f.glove, glove, sizeof(glove));
    f.squeeze = f.trigger = 0.0f;
    f.handFree = true;
    f.shot = false;
    f.spare = true;
    return f;
}

// The hand's middle at this place on the Stinger's frame (its turn stays).
void PutHandOnFrame(flap::Frame& f, const float* p) {
    float at[3], g[12], middle[3];
    Transform(f.gunPose, p, at);
    std::memcpy(g, f.glove, sizeof(g));
    g[3] = g[7] = g[11] = 0.0f;
    Transform(g, kHandInGlove, middle);
    f.glove[3] = at[0] - middle[0], f.glove[7] = at[1] - middle[1], f.glove[11] = at[2] - middle[2];
}

// The hand on the lift's way, this far along it (keys).
void PutHandAlong(flap::Frame& f, float along) {
    const int k = std::min(static_cast<int>(along), kAerial.liftKeys - 2);
    const float t = along - k;
    float p[3];
    for (int i = 0; i < 3; ++i)
        p[i] = kAerial.liftHand[k][i] + (kAerial.liftHand[k + 1][i] - kAerial.liftHand[k][i]) * t;
    PutHandOnFrame(f, p);
}

flap::Result Run(const flap::Frame& f, float seconds) {
    flap::Result r;
    const int frames = static_cast<int>(seconds / kDt + 0.5f);
    for (int i = 0; i < frames; ++i)
        r = flap::Update(f);
    return r;
}

// Fired with another to come: the aerial folded by itself. True if it said so once.
bool ToFolded(flap::Frame& f) {
    flap::Reset();
    f.squeeze = f.trigger = 0.0f;
    f.shot = true;
    flap::Update(f);
    f.shot = false;
    int folds = 0;
    for (int i = 0; i < 100; ++i)
        folds += flap::Update(f).folded ? 1 : 0;
    return folds == 1;
}

// The hand at the lift's start, the grip squeezed (one frame open, then shut).
flap::Result Take(flap::Frame& f) {
    PutHandAlong(f, 0.0f);
    f.squeeze = 0.0f;
    flap::Update(f);
    f.squeeze = 1.0f;
    return flap::Update(f);
}

// Taken and lifted this far along the way (keys), a tenth of a key a frame.
flap::Result LiftTo(flap::Frame& f, float along) {
    flap::Result r = Take(f);
    for (float a = 0.1f; a <= along + 1e-4f; a += 0.1f) {
        PutHandAlong(f, a);
        r = flap::Update(f);
    }
    return r;
}

}  // namespace

int main() {
    // ---- Every path as before, a hand holding the Stinger ----
    {  // f2a. Ready: open as the game has it, the trigger free; a squeeze on the aerial takes nothing.
        flap::Frame f = Base();
        flap::Reset();
        flap::Result r = Run(f, 0.5f);
        Check(!r.empty && !r.panels && !r.holding, "ready: open, the trigger free");
        r = Take(f);
        Check(!r.taken && !r.holding && !r.spentSqueeze, "ready: a squeeze on it takes nothing");
    }
    {  // f2b. Fired with none to come: it stays open.
        flap::Frame f = Base();
        flap::Reset();
        f.shot = true;
        f.spare = false;
        flap::Result r = flap::Update(f);
        f.shot = false;
        r = Run(f, 2.0f);
        Check(!r.empty && !r.panels && !r.folded, "fired, none to come: it stays open");
    }
    {  // f2c. Fired: empty at once; folding a little after, by itself, told once; folded it waits, however long.
        flap::Frame f = Base();
        flap::Reset();
        f.shot = true;
        flap::Result r = flap::Update(f);
        f.shot = false;
        Check(r.empty && !r.panels, "fired: empty at once, not folding yet");
        r = Run(f, 0.30f);
        Check(r.empty && !r.panels, "0.3 s on: not folding yet");
        r = Run(f, 0.15f);
        Check(r.empty && r.panels, "0.45 s on: folding");
        int folds = 0;
        for (int i = 0; i < 72; ++i) {
            r = flap::Update(f);
            folds += r.folded ? 1 : 0;
        }
        Check(folds == 1 && r.panels && r.empty, "folded, told once");
        float folded[4][12];
        std::memcpy(folded, r.bones, sizeof(folded));
        r = Run(f, 5.0f);
        Check(r.panels && r.empty && Apart12(&r.bones[0][0], &folded[0][0]) < 1e-6f, "5 s on: still folded, the trigger off");
        Check(Apart12(&r.bones[3][0], &kAerial.foldBones[kAerial.foldKeys - 1][3][0]) < 1e-6f, "folded as the game's last key");
    }
    {  // f2d. Taken only by the hand at the lift's start, free, with the grip or the trigger; lifted, it springs open three quarters of the way up,
       // told once; the glove on the aerial, then easing back over three frames.
        flap::Frame f = Base();
        Check(ToFolded(f), "(folded)");
        float far[3] = {kAerial.liftHand[0][0], kAerial.liftHand[0][1] + 0.20f, kAerial.liftHand[0][2]};
        PutHandOnFrame(f, far);
        f.squeeze = 0.0f;
        flap::Update(f);
        f.squeeze = 1.0f;
        flap::Result r = flap::Update(f);
        Check(!r.taken && !r.holding, "20 cm from it: not taken");
        f = Base();
        ToFolded(f);
        f.handFree = false;
        r = Take(f);
        Check(!r.taken && !r.holding, "the hand on the front grip: not taken");
        f = Base();
        ToFolded(f);
        f.left = false;
        r = Take(f);
        Check(!r.taken && !r.holding, "no left hand: not taken");
        f = Base();
        ToFolded(f);
        PutHandAlong(f, 0.0f);
        f.trigger = 0.0f;
        flap::Update(f);
        f.trigger = 1.0f;
        r = flap::Update(f);
        Check(r.taken && r.spentSqueeze, "taken with the trigger");
        r = flap::Update(f);
        Check(r.holding, "and held");
        f = Base();
        ToFolded(f);
        r = Take(f);
        Check(r.taken && r.spentSqueeze && r.panels, "taken with the grip");
        int opens = 0, held = 0;
        bool fingers = true, onGlove = true;
        for (float a = 0.1f; a <= 5.0f + 1e-4f; a += 0.1f) {
            PutHandAlong(f, a);
            r = flap::Update(f);
            opens += r.opened ? 1 : 0;
            if (r.holding) {
                ++held;
                fingers = fingers && r.fingers;
                onGlove = onGlove && Finite(r.glove, 12) && r.gloveBlend > 0.0f && r.panels;
            }
        }
        Check(held > 30 && fingers && onGlove, "lifting: the glove on the aerial, the game's fingers, its panels going up");
        for (int i = 0; i < 20; ++i)
            opens += flap::Update(f).opened ? 1 : 0;
        r = flap::Update(f);
        Check(opens == 1, "lifted three quarters of the way: open, told once");
        Check(!r.empty && !r.panels && !r.holding, "open: ready, the trigger free, as the game has it");
        // The glove's way back: three frames from the aerial.
        flap::Frame g = Base();
        ToFolded(g);
        r = LiftTo(g, 2.0f);
        Check(r.holding && r.gloveBlend == 1.0f, "(lifting, the glove all on it)");
        g.squeeze = 0.0f;
        float blends[3];
        for (float& b : blends) {
            r = flap::Update(g);
            b = r.gloveBlend;
        }
        Check(std::fabs(blends[0] - 0.65f) < 1e-5f && std::fabs(blends[1] - 0.30f) < 1e-5f && blends[2] == 0.0f, "let go of: the glove back over three frames");
    }
    {  // f2e. Let go of before it's up: it stays folded, and needs a new squeeze; the hand well off the way: let go of; it only goes up.
        flap::Frame f = Base();
        ToFolded(f);
        flap::Result r = LiftTo(f, 2.0f);
        Check(r.holding, "(lifting)");
        f.squeeze = 0.0f;
        r = flap::Update(f);
        Check(!r.holding && r.panels && r.empty, "let go of: folded again");
        Check(Apart12(&r.bones[3][0], &kAerial.foldBones[kAerial.foldKeys - 1][3][0]) < 1e-6f, "as it was folded");
        f.squeeze = 1.0f;
        PutHandAlong(f, 0.0f);
        r = flap::Update(f);
        Check(r.taken, "a new squeeze: taken again");
        // Off the way.
        f = Base();
        ToFolded(f);
        LiftTo(f, 1.0f);
        const float away[3] = {kAerial.liftHand[1][0], kAerial.liftHand[1][1] + 0.70f, kAerial.liftHand[1][2]};
        PutHandOnFrame(f, away);
        r = flap::Update(f);
        Check(!r.holding && r.panels && r.empty, "the hand 70 cm off the way: let go of, folded");
        // Up only: the hand back down the way doesn't lower it.
        f = Base();
        ToFolded(f);
        r = LiftTo(f, 3.0f);
        float up[4][12];
        std::memcpy(up, r.bones, sizeof(up));
        PutHandAlong(f, 1.0f);
        r = flap::Update(f);
        Check(r.holding && Apart12(&r.bones[0][0], &up[0][0]) < 1e-6f, "back down the way: it stays as high as it went");
        // Put away with the hand on it: folded when it's back.
        f.aerial = nullptr;
        r = flap::Update(f);
        Check(!r.holding && !r.panels, "put away: nothing of it");
        f.aerial = &kAerial;
        f.squeeze = 0.0f;
        r = flap::Update(f);
        Check(!r.holding && r.panels && r.empty && r.gloveBlend == 0.0f, "back: folded, the glove with the hand");
        // A new life or the game's own reload: ready.
        flap::Reset();
        r = flap::Update(f);
        Check(!r.panels && !r.empty, "reset: ready");
    }
    // ---- Free hands: the Stinger let go of ----
    {  // f1a. Let go of mid-lift (no hand holds it, so none tracks it): the lift lets go and it stays folded, whatever the hand does after; the glove eases
       // back over three frames from where it was on the aerial; never open.
        for (int which = 0; which < 3; ++which) {  // let go of as hands.cpp has it, then each sign alone
            flap::Frame f = Base();
            ToFolded(f);
            flap::Result r = LiftTo(f, 2.0f);
            Check(r.holding && r.gloveBlend == 1.0f, "f1a: (lifting)");
            float lifted[12];
            std::memcpy(lifted, r.glove, sizeof(lifted));
            f.held = which == 2;
            f.gunTracked = which == 1;
            r = flap::Update(f);
            Check(!r.holding && !r.fingers && r.panels && r.empty, "f1a: the Stinger let go of mid-lift: the lift lets go, folded");
            Check(Apart12(&r.bones[3][0], &kAerial.foldBones[kAerial.foldKeys - 1][3][0]) < 1e-6f, "f1a: as it was folded");
            Check(std::fabs(r.gloveBlend - 0.65f) < 1e-5f && Apart12(r.glove, lifted) < 1e-5f, "f1a: the glove easing back from where it was");
            r = flap::Update(f);
            Check(std::fabs(r.gloveBlend - 0.30f) < 1e-5f && Apart12(r.glove, lifted) < 1e-5f, "f1a: (its second frame)");
            r = flap::Update(f);
            Check(r.gloveBlend == 0.0f, "f1a: back with the hand");
            bool opened = false, holding = false;
            for (float a = 2.0f; a <= 6.0f + 1e-4f; a += 0.1f) {  // the hand going on up the way, the grip still squeezed
                PutHandAlong(f, a);
                r = flap::Update(f);
                opened |= r.opened, holding |= r.holding;
            }
            Check(!opened && !holding && r.panels && r.empty, "f1a: the hand going on up: nothing, still folded");
        }
    }
    {  // f1b. No hand holds it: a fresh squeeze at the lift's start takes nothing (with its pose known, or not); held again, a fresh squeeze takes it, the
       // lift opens it as ever.
        flap::Frame f = Base();
        ToFolded(f);
        f.held = false;
        flap::Result r = Take(f);
        Check(!r.taken && !r.holding && !r.spentSqueeze, "f1b: lying (its pose known): a squeeze on its aerial takes nothing");
        f.gunTracked = false;
        r = Take(f);
        Check(!r.taken && !r.holding, "f1b: lying: nothing taken");
        r = Run(f, 1.0f);
        Check(r.panels && r.empty && !r.holding, "f1b: and it waits, folded");
        f.held = true;
        f.gunTracked = true;
        r = flap::Update(f);
        Check(!r.taken, "f1b: held again, the squeeze still held: nothing");
        r = Take(f);
        Check(r.taken, "f1b: a fresh squeeze: taken");
        int opens = 0;
        for (float a = 0.1f; a <= 5.0f + 1e-4f; a += 0.1f) {
            PutHandAlong(f, a);
            opens += flap::Update(f).opened ? 1 : 0;
        }
        for (int i = 0; i < 20; ++i)
            opens += flap::Update(f).opened ? 1 : 0;
        Check(opens == 1, "f1b: lifted: open");
    }
    {  // f1c. The Stinger let go of with its pose gone (the hand that held it lost: nothing to place it by): the glove still eases back from where it was,
       // a rigid hand, not pulled to the eye. Held all along, as here, this is also the Stinger in the right hand with its controller lost, in any mode: the
       // lift is let go of as before (its zero pose put the hand far off the way); before, the glove was drawn shrunk and pulled to the eye for two frames.
        flap::Frame f = Base();
        ToFolded(f);
        flap::Result r = LiftTo(f, 3.0f);
        float lifted[12];
        std::memcpy(lifted, r.glove, sizeof(lifted));
        f.gunTracked = false;
        std::memset(f.gunPose, 0, sizeof(f.gunPose));
        r = flap::Update(f);
        Check(!r.holding && r.panels, "f1c: lost: let go of, folded");
        Check(r.gloveBlend > 0.0f && Apart12(r.glove, lifted) < 1e-5f, "f1c: the glove eases back from where it was");
        float at[3];
        Transform(r.glove, kHandInGlove, at);
        const float eye[3] = {};
        Check(Distance(at, eye) > 0.3f, "f1c: not at the eye");
    }
    {  // f1d. Fired, then let go of before it's folded: it folds by itself where it lies, told once; nothing takes it there.
        flap::Frame f = Base();
        flap::Reset();
        f.shot = true;
        flap::Update(f);
        f.shot = false;
        f.held = false;
        f.gunTracked = false;
        int folds = 0;
        flap::Result r;
        for (int i = 0; i < 100; ++i) {
            r = flap::Update(f);
            folds += r.folded ? 1 : 0;
        }
        Check(folds == 1 && r.panels && r.empty, "f1d: lying: it folds by itself, told once");
        r = Take(f);
        Check(!r.taken, "f1d: and nothing takes it");
    }
    {  // f1e. Broken inputs: a gun pose not finite, untracked, doesn't stick it or the glove.
        flap::Frame f = Base();
        ToFolded(f);
        flap::Result r = LiftTo(f, 2.0f);
        float lifted[12];
        std::memcpy(lifted, r.glove, sizeof(lifted));
        f.gunTracked = false;
        f.held = false;
        for (float& v : f.gunPose)
            v = std::nanf("");
        r = flap::Update(f);
        Check(!r.holding && Finite(r.glove, 12) && Apart12(r.glove, lifted) < 1e-5f, "f1e: a broken pose, untracked: let go of, the glove as it was");
        f = Base();
        ToFolded(f);
        r = Take(f);
        Check(r.taken, "f1e: after it, it still works");
    }
    std::printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed;
}

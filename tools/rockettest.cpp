// The launcher loaded by the left hand (src/rocket.cpp, as hands.cpp runs it), on scripted frames at 72 Hz, without the game: the spent case staying
// on, the hand taking it, the wrist or a pull turning it open, pulled off and dropped; the squeeze over the shoulder and the zone, whichever way
// the hand is turned; the catch at the launcher's back by position alone, the push and the wait, the wrist or a wait turning it shut; letting go, the
// launcher or the hand lost, the game's own reload, tracking jumps, broken inputs.
// The room is y up, metres: the head level at (0 1.2 0) faces -z. The view is the head's own space (x right, y up, z back). The launcher is held still
// in it, pointing ahead (its frame x left, y up, z to its muzzle), turned a little off the axes.
// Usage: rockettest   The exit code is the number of failed checks.

#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>

#include "../src/rocket.h"

namespace {

constexpr float kDt = 1.0f / 72.0f;
constexpr float kMiddle[3] = {0.537f, 1.098f, 0.207f};  // the hand's middle in the gloves' bind space (handpose.cpp)
constexpr float kAcross[3] = {0.633f, -0.380f, -0.675f};
constexpr float kPalm[3] = {-0.639f, -0.748f, -0.179f};

int g_failed = 0, g_checks = 0;

void Check(bool ok, const char* what) {
    ++g_checks;
    if (!ok) {
        ++g_failed;
        std::printf("FAIL: %s\n", what);
    }
}

bool Is(const rocket::Result& r, const char* state) {
    return std::strcmp(r.state, state) == 0;
}

void Identity(float* m) {
    const float i[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    std::memcpy(m, i, sizeof(i));
}

void Transform(const float* m, const float* p, float* out) {
    for (int i = 0; i < 3; ++i)
        out[i] = m[i * 4] * p[0] + m[i * 4 + 1] * p[1] + m[i * 4 + 2] * p[2] + m[i * 4 + 3];
}

void Turn(const float* m, const float* v, float* out) {
    for (int i = 0; i < 3; ++i)
        out[i] = m[i * 4] * v[0] + m[i * 4 + 1] * v[1] + m[i * 4 + 2] * v[2];
}

void Compose(const float* a, const float* b, float* out) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j)
            out[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        out[i * 4 + 3] += a[i * 4 + 3];
    }
}

float Distance(const float* a, const float* b) {
    const float d[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    return std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
}

float Dot(const float* a, const float* b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// The base frame: launcher ahead and to the right, loaded, the hand down by the belt, open.
rocket::Frame Base() {
    rocket::Frame f;
    f.dt = kDt;
    f.launcher = true;
    // A little yawed: x left, y up, z to the muzzle, in the view's space; 10 degrees round the view's up.
    const float c = std::cos(0.17f), s = std::sin(0.17f);
    const float pose[12] = {-c, 0, -s, 0.15f,  //
                            0,  1, 0,  -0.20f,  //
                            s,  0, -c, -0.40f};
    std::memcpy(f.launcherPose, pose, sizeof(pose));
    Identity(f.headPose);
    f.headPose[7] = 1.2f;
    f.left = true;
    Identity(f.glove);
    f.squeeze = 0.0f;
    f.neck[0] = 0.0f, f.neck[1] = 1.2f - 0.075f, f.neck[2] = 0.08f;
    f.yaw = 0.0f;
    f.haveBody = true;
    f.loaded = true;
    f.spare = true;
    return f;
}

// The hand's middle at this place in the view's space (its turn stays).
void PutHand(rocket::Frame& f, float x, float y, float z) {
    float at[3];
    float g[12];
    std::memcpy(g, f.glove, sizeof(g));
    g[3] = g[7] = g[11] = 0.0f;
    Transform(g, kMiddle, at);
    f.glove[3] = x - at[0], f.glove[7] = y - at[1], f.glove[11] = z - at[2];
}

void PutHandOnFrame(rocket::Frame& f, float x, float y, float z) {
    const float p[3] = {x, y, z};
    float at[3];
    Transform(f.launcherPose, p, at);
    PutHand(f, at[0], at[1], at[2]);
}

// The wrist rolled by this much about the launcher's line, about the hand's middle.
void RollWrist(rocket::Frame& f, float angle) {
    float at[3];
    Transform(f.glove, kMiddle, at);
    const float axis[3] = {f.launcherPose[2], f.launcherPose[6], f.launcherPose[10]};
    const float c = std::cos(angle), s = std::sin(angle), u = 1.0f - c;
    const float r[12] = {u * axis[0] * axis[0] + c,           u * axis[0] * axis[1] - s * axis[2], u * axis[0] * axis[2] + s * axis[1], 0,
                         u * axis[0] * axis[1] + s * axis[2], u * axis[1] * axis[1] + c,           u * axis[1] * axis[2] - s * axis[0], 0,
                         u * axis[0] * axis[2] - s * axis[1], u * axis[1] * axis[2] + s * axis[0], u * axis[2] * axis[2] + c,           0};
    float turned[12];
    Compose(r, f.glove, turned);
    std::memcpy(f.glove, turned, sizeof(turned));
    PutHand(f, at[0], at[1], at[2]);
}

rocket::Result Run(rocket::Frame f, float seconds) {
    rocket::Result r;
    const int frames = static_cast<int>(seconds / kDt + 0.5f);
    for (int i = 0; i < frames; ++i)
        r = rocket::Update(f);
    return r;
}

// Squeezed on the spot (one frame open, then shut).
rocket::Result Squeeze(rocket::Frame& f) {
    f.squeeze = 0.0f;
    rocket::Update(f);
    f.squeeze = 1.0f;
    return rocket::Update(f);
}

// Fired: the spent one on.
void ToSpent(rocket::Frame& f) {
    rocket::Clear();
    f.loaded = true;
    f.squeeze = 0.0f;
    Run(f, 0.1f);
    f.loaded = false;
    Run(f, 0.1f);
}

// The hand on the spent case, its grip taken.
rocket::Result Grab(rocket::Frame& f) {
    PutHandOnFrame(f, 0.05f, 0.02f, -0.45f);
    return Squeeze(f);
}

// Fired and the spent one pulled off and dropped: nothing on it.
void ToEmpty(rocket::Frame& f) {
    ToSpent(f);
    Grab(f);
    for (int i = 0; i < 200; ++i) {  // pulled straight back
        float hand[3];
        Transform(f.glove, kMiddle, hand);
        for (int k = 0; k < 3; ++k)
            hand[k] -= f.launcherPose[k * 4 + 2] * 0.4f * kDt;
        PutHand(f, hand[0], hand[1], hand[2]);
        if (rocket::Update(f).freed)
            break;
    }
    f.squeeze = 0.0f;
    Run(f, 2.0f);
    PutHand(f, -0.3f, -0.1f, -0.5f);
}

// Squeezes at the shoulder.
rocket::Result TakeAtShoulder(rocket::Frame& f) {
    PutHand(f, -0.15f, 0.05f, 0.12f);  // left of the head, a little above the eyes, behind them
    return Squeeze(f);
}

// The case's bone is at its place at rest on the launcher to within this (m).
float FromHome(const rocket::Frame& f, const float* pose) {
    float rel[12], home[12], a[3], b[3];
    rocket::CaseOnLine(0.0f, rel);
    Compose(f.launcherPose, rel, home);
    const float p[3] = {0.0f, -0.0935f, -0.16f};
    Transform(home, p, a);
    Transform(pose, p, b);
    return Distance(a, b);
}

}  // namespace

// The hand half a turn about its palm: the case's front the other way in it, the branch of MakeHold that turns it end for end (the review, 2026-11-04: every other test's hand kept it one way).
void HalfTurnAboutPalm(rocket::Frame& f) {
    float at[3], axis[3];
    Transform(f.glove, kMiddle, at);
    Turn(f.glove, kPalm, axis);
    const float n = std::sqrt(Dot(axis, axis));
    for (float& v : axis)
        v /= n;
    float turned[12];
    for (int i = 0; i < 3; ++i) {
        // A half turn about a unit axis: 2 a a^T - 1.
        const float r[3] = {2.0f * axis[i] * axis[0] - (i == 0), 2.0f * axis[i] * axis[1] - (i == 1), 2.0f * axis[i] * axis[2] - (i == 2)};
        for (int j = 0; j < 3; ++j)
            turned[i * 4 + j] = r[0] * f.glove[j] + r[1] * f.glove[4 + j] + r[2] * f.glove[8 + j];
        turned[i * 4 + 3] = 0.0f;
    }
    std::memcpy(f.glove, turned, sizeof(turned));
    PutHand(f, at[0], at[1], at[2]);
}

int main() {
    rocket::SetMagnet(0.20f, 0.11f, 0.17f, 0.30f, 0.30f);
    {  // 1. Loaded: as the game has it, the trigger free.
        rocket::Frame f = Base();
        rocket::Clear();
        rocket::Result r = Run(f, 0.5f);
        Check(r.show == rocket::Show::kGame, "loaded: drawn where the game has it");
        Check(!r.empty, "loaded: the trigger's free");
        Check(!r.holding, "loaded: the hand is free");
        f.squeeze = 1.0f;
        PutHandOnFrame(f, 0.05f, 0.02f, -0.45f);
        r = Run(f, 0.1f);
        Check(!r.grabbed && !r.holding, "loaded: a squeeze on the case takes nothing");
    }
    {  // 2. Fired: the spent case stays on. It doesn't come off by itself, however long.
        rocket::Frame f = Base();
        rocket::Clear();
        Run(f, 0.1f);
        f.loaded = false;
        rocket::Result r = rocket::Update(f);
        Check(r.spent, "fired: spent");
        Check(r.empty, "fired: empty at once");
        Check(r.show == rocket::Show::kGame, "fired: the case stays");
        r = Run(f, 5.0f);
        Check(r.show == rocket::Show::kGame && Is(r, "spent"), "five seconds on: still on, as the game has it");
        Check(r.empty, "and the trigger's off");
    }
    {  // 3. Taken by the hand near it, anywhere along the part out of the launcher, however the hand's turned; not from further away.
        rocket::Frame f = Base();
        ToSpent(f);
        PutHandOnFrame(f, 0.30f, 0.30f, -0.45f);
        rocket::Result r = Squeeze(f);
        Check(!r.grabbed && r.squeezed, "squeezed well away from it: nothing");
        Check(std::strcmp(r.note, "not on the spent case") == 0, "and says so");
        float along[3] = {-0.25f, -0.45f, -0.68f};
        for (float z : along) {
            rocket::Frame g = Base();
            ToSpent(g);
            PutHandOnFrame(g, 0.05f, 0.05f, z);
            r = Squeeze(g);
            Check(r.grabbed && r.holding, "taken anywhere along it");
            Check(r.empty, "still empty in the hand");
        }
        for (float turn = 0.0f; turn < 6.2f; turn += 0.7f) {
            rocket::Frame g = Base();
            RollWrist(g, turn);
            ToSpent(g);
            r = Grab(g);
            Check(r.grabbed, "taken however the hand's turned");
            r = rocket::Update(g);
            // Held the way the hand already is: the glove drawn no more than a right angle from it, and on the case.
            float real[3], drawn[3];
            for (int i = 0; i < 12; ++i)
                r = rocket::Update(g);
            Turn(g.glove, kAcross, real);
            Turn(r.glove, kAcross, drawn);
            Check(Dot(real, drawn) > 0.0f, "whichever way round suits the hand");
            Turn(g.glove, kPalm, real);
            Turn(r.glove, kPalm, drawn);
            Check(Dot(real, drawn) > 0.3f, "the palm the way it faces");
        }
        // Let go of at once: back as it was.
        rocket::Frame g = Base();
        ToSpent(g);
        Grab(g);
        g.squeeze = 0.0f;
        r = Run(g, 0.4f);
        Check(Is(r, "spent") && r.show == rocket::Show::kGame && !r.holding, "let go of: as it was");
    }
    {  // 4. A roll of the wrist turns it open, either way; a small one only part way.
        for (float sign : {1.0f, -1.0f}) {
            rocket::Frame f = Base();
            ToSpent(f);
            Grab(f);
            RollWrist(f, sign * 0.10f);
            rocket::Result r = Run(f, 0.1f);
            Check(Is(r, "turning open"), "a small roll: still turning");
            RollWrist(f, sign * 0.15f);
            r = rocket::Update(f);
            Check(r.unlocked, "rolled 14 degrees: open");
            Check(Is(r, "coming off"), "and it comes off now");
        }
    }
    {  // 5. Or a pull alone: it turns open as it starts to come, drawn twisted, and off along the line, in the hand at the end.
        rocket::Frame f = Base();
        ToSpent(f);
        Grab(f);
        rocket::Result r;
        bool unlocked = false, freed = false;
        float pulled = 0.0f;
        float last[3] = {};
        bool have = false;
        float most = 0.0f;
        for (int i = 0; i < 200 && !freed; ++i) {
            float hand[3];
            Transform(f.glove, kMiddle, hand);
            for (int k = 0; k < 3; ++k)
                hand[k] -= f.launcherPose[k * 4 + 2] * 0.4f * kDt;
            PutHand(f, hand[0], hand[1], hand[2]);
            pulled += 0.4f * kDt;
            r = rocket::Update(f);
            unlocked |= r.unlocked;
            freed |= r.freed;
            if (r.unlocked)
                Check(pulled < 0.04f, "open within the first few cm of the pull");
            if (r.show == rocket::Show::kAt) {
                const float p[3] = {0.0f, -0.0935f, -0.16f};
                float now[3];
                Transform(r.pose, p, now);
                if (have)
                    most = std::fmax(most, Distance(now, last));
                std::memcpy(last, now, sizeof(now));
                have = true;
            }
        }
        std::printf("info: the most the spent case moves in a frame coming off: %.1f cm\n", most * 100.0f);
        Check(unlocked && freed, "pulled alone: open, then off");
        Check(most < 0.03f, "it doesn't jump on the way off");
        Check(pulled < 0.30f, "off after a natural pull");
        Check(Is(r, "spent in the hand") && r.holding, "in the hand");
        Check(r.empty, "still empty");
        // Carried, then let go of: it falls and it's gone; nothing on the launcher.
        r = Run(f, 0.3f);
        Check(r.show == rocket::Show::kAt && r.holding, "carried");
        f.squeeze = 0.0f;
        r = rocket::Update(f);
        Check(r.letGo && !r.holding, "let go of: it falls");
        r = Run(f, 2.0f);
        Check(r.show == rocket::Show::kNone && Is(r, "empty"), "and it's gone");
    }
    {  // 6. Let go of once it's open, part way off: it goes on off by itself; before it's open: back on.
        rocket::Frame f = Base();
        ToSpent(f);
        Grab(f);
        RollWrist(f, 0.3f);
        rocket::Update(f);
        f.squeeze = 0.0f;
        rocket::Result r = rocket::Update(f);
        Check(Is(r, "sliding off"), "open, let go of: it goes on off");
        r = Run(f, 2.5f);
        Check(Is(r, "empty") && r.show == rocket::Show::kNone, "and it's gone");
        rocket::Frame g = Base();
        ToSpent(g);
        Grab(g);
        RollWrist(g, 0.08f);
        rocket::Update(g);
        g.squeeze = 0.0f;
        r = Run(g, 0.4f);
        Check(Is(r, "spent") && r.show == rocket::Show::kGame, "part way turned, let go of: back shut");
    }
    {  // 7. A tracking jump doesn't pull it off.
        rocket::Frame f = Base();
        ToSpent(f);
        Grab(f);
        RollWrist(f, 0.3f);
        rocket::Update(f);
        rocket::Frame jump = f;
        float hand[3];
        Transform(f.glove, kMiddle, hand);
        PutHand(jump, hand[0], hand[1], hand[2] + 0.8f);
        rocket::Result r = rocket::Update(jump);
        Check(!r.freed, "a one-frame jump: not off");
        r = rocket::Update(f);
        Check(Is(r, "coming off"), "still on the way off");
    }
    {  // 8. A squeeze over the shoulder with the spent one still on: nothing; with it off, a new one, whichever way the hand's turned, its
       // front the way the launcher points, never to be flipped.
        rocket::Frame f = Base();
        ToSpent(f);
        rocket::Result r = TakeAtShoulder(f);
        Check(!r.took && !r.grabbed, "the spent one still on: none taken");
        for (float turn = 0.0f; turn < 6.2f; turn += 0.7f) {
            rocket::Frame g = Base();
            ToEmpty(g);
            RollWrist(g, turn);
            const float acrossTurn = turn;
            (void)acrossTurn;
            r = TakeAtShoulder(g);
            Check(r.took && r.holding, "over the shoulder: taken");
            Check(r.show == rocket::Show::kAt, "drawn in the hand");
            float front[3];
            const float z[3] = {0, 0, 1};
            Turn(r.pose, z, front);
            const float lz[3] = {g.launcherPose[2], g.launcherPose[6], g.launcherPose[10]};
            Check(Dot(front, lz) >= -0.001f, "its front the launcher's way, however the hand's turned");
        }
        rocket::Frame g = Base();
        ToEmpty(g);
        PutHand(g, -0.2f, -0.5f, -0.3f);
        r = Squeeze(g);
        Check(!r.took && std::strcmp(r.note, "not over the shoulder") == 0, "in front of the belly: not taken, and says so");
        g = Base();
        ToEmpty(g);
        g.spare = false;
        r = TakeAtShoulder(g);
        Check(!r.took && std::strcmp(r.note, "no rockets left") == 0, "none left: not taken, and says so");
        // Squeezed long before: not; just before: taken.
        g = Base();
        ToEmpty(g);
        PutHand(g, -0.2f, -0.5f, -0.3f);
        Squeeze(g);
        Run(g, 0.6f);
        PutHand(g, -0.15f, 0.05f, 0.12f);
        r = Run(g, 0.2f);
        Check(!r.holding, "a squeeze held long from elsewhere takes nothing");
        g = Base();
        ToEmpty(g);
        PutHand(g, -0.2f, -0.5f, -0.3f);
        Squeeze(g);
        Run(g, 0.2f);
        PutHand(g, -0.15f, 0.05f, 0.12f);
        r = rocket::Update(g);
        Check(r.took, "squeezed just before getting there: taken");
        // The other shoulder.
        g = Base();
        ToEmpty(g);
        PutHand(g, 0.20f, 0.03f, 0.05f);
        r = Squeeze(g);
        Check(r.took, "the other shoulder too");
    }
    {  // 9. Brought near the launcher's back whichever way the hand's turned: caught, by position alone, on the line, on by itself, turned shut by itself;
       // told once; the hand on it.
        for (float turn = 0.0f; turn < 6.2f; turn += 0.9f) {
            rocket::Frame f = Base();
            ToEmpty(f);
            RollWrist(f, turn);
            TakeAtShoulder(f);
            Run(f, 0.1f);
            PutHandOnFrame(f, 0.25f, 0.25f, -0.30f);  // not near yet
            rocket::Result r = rocket::Update(f);
            bool caught = r.caught;
            PutHandOnFrame(f, 0.10f, 0.15f, -0.25f);
            int seats = 0, homings = 0, frames = 0;
            float last[3] = {};
            bool have = false;
            float most = 0.0f;
            bool twisted = false;
            for (; frames < 300 && !seats; ++frames) {
                r = rocket::Update(f);
                caught |= r.caught;
                seats += r.seated;
                homings += r.homing;
                Check(r.show == rocket::Show::kAt, "drawn all the way on");
                Check(r.empty == !r.seated, "empty until it's shut");
                if (Is(r, "going on")) {
                    float toFrame[12], rel[12];
                    for (int i = 0; i < 3; ++i) {
                        for (int j = 0; j < 3; ++j)
                            toFrame[i * 4 + j] = f.launcherPose[j * 4 + i];
                        toFrame[i * 4 + 3] = -(f.launcherPose[3] * f.launcherPose[i] + f.launcherPose[7] * f.launcherPose[4 + i] + f.launcherPose[11] * f.launcherPose[8 + i]);
                    }
                    Compose(toFrame, r.pose, rel);
                    twisted = rel[1] > 0.3f;  // its y's lean towards the frame's x
                }
                const float p[3] = {0.0f, -0.0935f, -0.16f};
                float now[3];
                Transform(r.pose, p, now);
                if (have)
                    most = std::fmax(most, Distance(now, last));
                std::memcpy(last, now, sizeof(now));
                have = true;
            }
            Check(caught, "near the launcher's back: caught, whichever way round");
            Check(twisted, "it comes on twisted open");
            Check(seats == 1, "turned shut by itself");
            Check(homings == 1, "its sound once");
            Check(frames * kDt < 1.5f, "soon");
            Check(most < 0.20f, "the glide doesn't jump, even from the wrong way round");
            Check(!r.empty, "the seat's frame isn't empty");
            f.loaded = true;
            bool wasGame = false;
            float settled[12] = {};
            for (int i = 0; i < 40; ++i) {
                r = rocket::Update(f);
                if (r.show == rocket::Show::kAt)
                    std::memcpy(settled, r.pose, sizeof(settled));
                else
                    wasGame = true;
                Check(!r.holding, "settling: the hand's free");
            }
            Check(wasGame, "then as the game has it");
            Check(FromHome(f, settled) < 0.003f, "settled exactly at its place at rest");
            r = Run(f, 1.0f);
            Check(r.gloveBlend == 0.0f, "the glove back with the hand");
        }
    }
    {  // 10. A roll of the wrist turns it shut sooner than the wait; with the wait off, only the wrist.
        rocket::SetMagnet(0.20f, 0.11f, 0.17f, 0.30f, -1.0f);
        rocket::Frame f = Base();
        ToEmpty(f);
        TakeAtShoulder(f);
        Run(f, 0.1f);
        PutHandOnFrame(f, 0.10f, 0.15f, -0.25f);
        rocket::Result r;
        for (int i = 0; i < 200; ++i) {
            r = rocket::Update(f);
            if (Is(r, "turning shut"))
                break;
        }
        Check(Is(r, "turning shut"), "on, to be turned shut");
        r = Run(f, 2.0f);
        Check(Is(r, "turning shut") && r.empty, "not turned: it waits, not loaded");
        RollWrist(f, 0.10f);
        r = rocket::Update(f);
        Check(Is(r, "turning shut") && !r.seated, "a small roll: part way");
        RollWrist(f, 0.15f);
        r = rocket::Update(f);
        Check(r.seated, "rolled 14 degrees: shut");
        rocket::SetMagnet(0.20f, 0.11f, 0.17f, 0.30f, 0.30f);
    }
    {  // 11. Pushed: without the wait it goes only as far as it's pushed, and on near the end.
        rocket::SetMagnet(0.20f, 0.11f, 0.17f, -1.0f, 0.30f);
        rocket::Frame f = Base();
        ToEmpty(f);
        TakeAtShoulder(f);
        Run(f, 0.1f);
        PutHandOnFrame(f, 0.10f, 0.15f, -0.25f);
        rocket::Result r = rocket::Update(f);
        Check(r.caught, "caught, for the push");
        r = Run(f, 1.0f);
        Check(!r.seated && Is(r, "on the line"), "not pushed: it waits");
        float hand[3];
        Transform(f.glove, kMiddle, hand);
        bool on = false;
        for (int i = 0; i < 200 && !on; ++i) {
            for (int k = 0; k < 3; ++k)
                hand[k] += f.launcherPose[k * 4 + 2] * 0.20f * kDt;
            PutHand(f, hand[0], hand[1], hand[2]);
            r = rocket::Update(f);
            on |= r.homing;
        }
        Check(on, "pushed on");
        rocket::SetMagnet(0.20f, 0.11f, 0.17f, 0.30f, 0.30f);
    }
    {  // 12. Pulled off the line: back in the hand; caught again after being away.
        rocket::SetMagnet(0.20f, 0.11f, 0.17f, -1.0f, 0.30f);
        rocket::Frame f = Base();
        ToEmpty(f);
        TakeAtShoulder(f);
        Run(f, 0.1f);
        PutHandOnFrame(f, 0.10f, 0.15f, -0.25f);
        rocket::Update(f);
        Run(f, 0.4f);
        PutHandOnFrame(f, 0.70f, 0.60f, -0.55f);
        bool off = false;
        rocket::Result r;
        for (int i = 0; i < 10; ++i) {
            r = rocket::Update(f);
            off |= r.offLine;
        }
        Check(off, "pulled away: off the line");
        Check(Is(r, "in the hand"), "back in the hand");
        PutHandOnFrame(f, 0.10f, 0.15f, -0.25f);
        r = rocket::Update(f);
        Check(r.caught, "well away and back: caught again");
        rocket::SetMagnet(0.20f, 0.11f, 0.17f, 0.30f, 0.30f);
    }
    {  // 13. A tracking jump on the line doesn't throw it on, nor off.
        rocket::SetMagnet(0.20f, 0.11f, 0.17f, -1.0f, 0.30f);
        rocket::Frame f = Base();
        ToEmpty(f);
        TakeAtShoulder(f);
        Run(f, 0.1f);
        PutHandOnFrame(f, 0.10f, 0.15f, -0.25f);
        rocket::Update(f);
        Run(f, 0.4f);
        float hand[3];
        Transform(f.glove, kMiddle, hand);
        rocket::Frame jump = f;
        for (int k = 0; k < 3; ++k)
            hand[k] += f.launcherPose[k * 4 + 2] * 0.5f;
        PutHand(jump, hand[0], hand[1], hand[2]);
        rocket::Result r = rocket::Update(jump);
        Check(!r.homing && !r.offLine, "a one-frame jump: nothing");
        r = rocket::Update(f);
        Check(!r.homing && !r.offLine && Is(r, "on the line"), "still on the line");
        rocket::SetMagnet(0.20f, 0.11f, 0.17f, 0.30f, 0.30f);
    }
    {  // 14. Let go of a new one in the air: it falls and the launcher's still empty, ready for another; at the shoulder: back.
        rocket::Frame f = Base();
        ToEmpty(f);
        TakeAtShoulder(f);
        PutHand(f, -0.3f, -0.1f, -0.5f);
        Run(f, 0.1f);
        f.squeeze = 0.0f;
        rocket::Result r = rocket::Update(f);
        Check(r.letGo && !r.seated, "let go of in front: it falls");
        r = Run(f, 2.0f);
        Check(r.show == rocket::Show::kNone && r.empty, "and it's gone");
        rocket::Frame g = Base();
        ToEmpty(g);
        TakeAtShoulder(g);
        Run(g, 0.1f);
        g.squeeze = 0.0f;
        r = rocket::Update(g);
        Check(r.putBack && !r.letGo, "let go of at the shoulder: back");
        Check(r.show == rocket::Show::kNone, "nothing falls");
    }
    {  // 15. The launcher put away: a new one in the hand goes back; it comes back empty; the spent one stays on; a loaded one loaded.
        rocket::Frame f = Base();
        ToEmpty(f);
        TakeAtShoulder(f);
        Run(f, 0.1f);
        f.launcher = false;
        rocket::Result r = rocket::Update(f);
        Check(r.cancelled, "the launcher went: gone from the hand");
        f.launcher = true;
        r = Run(f, 0.1f);
        Check(r.show == rocket::Show::kNone && r.empty, "back: still empty");
        rocket::Frame g = Base();
        ToSpent(g);
        Grab(g);
        g.launcher = false;
        Run(g, 0.2f);
        g.launcher = true;
        g.squeeze = 0.0f;
        r = Run(g, 0.1f);
        Check(Is(r, "spent") && r.show == rocket::Show::kGame, "a spent one, away and back: still on");
        g = Base();
        rocket::Clear();
        Run(g, 0.1f);
        g.launcher = false;
        Run(g, 0.5f);
        g.launcher = true;
        r = Run(g, 0.1f);
        Check(r.show == rocket::Show::kGame && !r.empty, "loaded, away and back: loaded");
    }
    {  // 16. The hand lost: kept a moment, then let go; back in time, kept.
        rocket::Frame f = Base();
        ToEmpty(f);
        TakeAtShoulder(f);
        PutHand(f, -0.3f, -0.1f, -0.5f);
        Run(f, 0.1f);
        f.left = false;
        rocket::Result r = Run(f, 0.5f);
        Check(r.holding, "lost half a second: kept");
        f.left = true;
        r = Run(f, 0.1f);
        Check(r.holding, "back: still held");
        f.left = false;
        bool gone = false;
        for (int i = 0; i < 100; ++i) {
            r = rocket::Update(f);
            gone |= r.cancelled;
        }
        Check(gone && !r.holding, "lost longer: gone");
    }
    {  // 17. The game's own reload: its animation has the case; the hand is let go; loaded after, as the game says.
        rocket::Frame f = Base();
        ToSpent(f);
        Grab(f);
        Run(f, 0.1f);
        f.gameReload = true;
        rocket::Result r = rocket::Update(f);
        Check(r.cancelled && !r.holding, "the game's reload: gone from the hand");
        Check(r.show == rocket::Show::kGame, "the case the game's");
        r = Run(f, 2.0f);
        Check(r.show == rocket::Show::kGame && r.empty, "all through it");
        f.gameReload = false;
        f.loaded = true;
        r = rocket::Update(f);
        Check(r.show == rocket::Show::kGame && !r.empty && !r.spent, "then loaded");
    }
    {  // 18. A rocket in it without the hand (a checkpoint), at any point.
        rocket::Frame f = Base();
        ToSpent(f);
        f.loaded = true;
        rocket::Result r = rocket::Update(f);
        Check(r.show == rocket::Show::kGame && !r.empty, "spent, a checkpoint: loaded");
        rocket::Frame g = Base();
        ToEmpty(g);
        TakeAtShoulder(g);
        g.loaded = true;
        r = rocket::Update(g);
        Check(r.cancelled && !r.holding, "with one in the hand: gone");
    }
    {  // 19. Broken inputs don't stick it.
        rocket::Frame f = Base();
        ToEmpty(f);
        f.squeeze = std::nanf("");
        f.dt = std::nanf("");
        Run(f, 0.1f);
        f = Base();
        f.loaded = false;
        rocket::Result r = TakeAtShoulder(f);
        Check(r.took, "after broken inputs it still works");
    }
    {
        // The other way round in the hand: the branch of MakeHold that turns the case end for end. Still rigid, the palm as it faced, the same way round as the hand.
        rocket::Frame f = Base();
        HalfTurnAboutPalm(f);
        ToSpent(f);
        rocket::Result r = Grab(f);
        Check(r.grabbed, "the spent case taken with the hand turned end for end");
        for (int i = 0; i < 12; ++i)
            r = rocket::Update(f);
        const float x[3] = {r.glove[0], r.glove[4], r.glove[8]}, y[3] = {r.glove[1], r.glove[5], r.glove[9]}, z[3] = {r.glove[2], r.glove[6], r.glove[10]};
        Check(std::fabs(Dot(x, x) - 1.0f) < 1e-3f && std::fabs(Dot(y, y) - 1.0f) < 1e-3f && std::fabs(Dot(z, z) - 1.0f) < 1e-3f, "its glove still unit axes");
        Check(std::fabs(Dot(x, y)) < 1e-3f && std::fabs(Dot(y, z)) < 1e-3f && std::fabs(Dot(x, z)) < 1e-3f, "at right angles");
        const float cross[3] = {x[1] * y[2] - x[2] * y[1], x[2] * y[0] - x[0] * y[2], x[0] * y[1] - x[1] * y[0]};
        Check(Dot(cross, z) > 0.99f, "and not mirrored");
        float real[3], drawn[3];
        Turn(f.glove, kAcross, real);
        Turn(r.glove, kAcross, drawn);
        Check(Dot(real, drawn) > 0.0f, "the same way round as the hand");
        Turn(f.glove, kPalm, real);
        Turn(r.glove, kPalm, drawn);
        Check(Dot(real, drawn) > 0.5f, "the palm facing the way it faced");
    }
    {
        // Coming off, a frame of the hand lost: it keeps it a moment (kLost).
        rocket::Frame f = Base();
        ToSpent(f);
        PutHandOnFrame(f, 0.05f, 0.05f, -0.45f);
        rocket::Result r = Squeeze(f);
        Check(r.grabbed, "taken, to pull");
        for (float z = -0.45f; z > -0.52f; z -= 0.005f) {
            PutHandOnFrame(f, 0.05f, 0.05f, z);
            r = rocket::Update(f);
        }
        Check(Is(r, "coming off"), "open and coming off");
        f.left = false;
        for (int i = 0; i < 5; ++i)
            r = rocket::Update(f);
        Check(Is(r, "coming off") && r.holding, "the hand lost for a moment: it stays in it");
        f.left = true;
        r = rocket::Update(f);
        Check(Is(r, "coming off"), "and goes on");
    }
    std::printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed;
}

// The right stick's crouch and prone (src/stance.h, header only, as input.cpp runs it) on the game's own pose (src/poseread.h, its reader, here on a
// memory image made up in the test), without the game: the mod's frames at 72 Hz, the game's side at its own 30 Hz by its rules as read in bf3.exe
// 1.6.0.0 (the pose input 0x011F7050, setPose 0x00591D30, the change's end 0x005913E0, the sprint's stand-up 0x011F8860, the jump's 0x011F6FF0).
// Each run three ways: the old stance (input.cpp's before 2026-11-19, copied here: the mod's belief alone, a sprint press resetting it, Z then X out of
// prone), the new with the game's pose read, the new without it (its belief and the game's own stand-ups).
//   1  the owner's sequence, 2026-11-19 00:58 and 01:03 ("if I flick up, I crouch"): crouched, a sprint press standing still, flicked down, up, down
//   2  a death while prone: the new soldier stands; flicked down and up
//   3  the RPG checkpoint's new soldier prone: flicked up (without the read: unknown till a flick down)
//   4  prone from a crouch, then up (the old Z then X crouched again); prone from a crouch, then down to a crouch
//   5  the game's own stand-up: the sprint with the stick forward stands it, the sprint standing still doesn't; a jump from prone
//   6  a change during a change the game doesn't take (its data's AllowPoseChangeDuringTransition 0): the new waits for its end
//   7  a scripted lock: a pose the game doesn't allow (its constraints) never pressed for; the game not reading the keys: pressed twice, given up,
//      the stance the game's; without the read, nothing sent in a scripted moment
//   8  no room to stand (prone under something): Z refused, given up, still prone
//   9  the head's real crouch (Left Ctrl): held steady, never on and off; a flick lets it go first; begun only from standing
//   10 a fist fight's prompt: nothing pressed; a cutscene: nothing pressed
//   11 the rules: every pose to every pose by KeyFor's one key (the game's AfterKey)
//   12 the reader on a made-up image: a soldier read; every check failing alone, named; never a fault
//   13 the proofs: on a made-up image each broken alone; against the real code (build\bf3_mem.exe beside the test, SKIP without it)
//   14 the time (said last)
//   15 the review of 7b43fdf (2026-11-19): held down then up at once, out of prone then down at once (without the read the next key waits for the
//      game's change to end, kProneGap: it was refused in it and the belief parted); a scripted moment's one press a gesture, never two; a scripted
//      moment just after going prone (the game's own turn of the look): read, up still stands the soldier; the head's crouch named when a flick up
//      does nothing; a change under way when a fist fight's prompt opens: given up, never pressed after the prompt
//   16 the log's line for each change of the game's pose (poseread.h's SaidNext, input.cpp's "Stance: the game's pose: ..."): "never reached" only for
//      a target the soldier's pose never was (refused at its end, another asked for before it ended), never for one read with the soldier already in
//      the new pose (the proof of 2026-11-25: "standing (was prone, never reached: the soldier stayed standing)"); the old line shown saying it
// Usage: stancetest
// The exit code is the number of failed checks.

#include <windows.h>

#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "../src/poseread.h"
#include "../src/stance.h"

namespace {

int g_failed = 0, g_counted = 0, g_skipped = 0;

void Report(const char* id, const char* what, bool pass, const char* format, ...) {
    char numbers[1024];
    va_list args;
    va_start(args, format);
    std::vsnprintf(numbers, sizeof(numbers), format, args);
    va_end(args);
    std::printf("%-4s %-5s %s | %s\n", id, pass ? "PASS" : "FAIL", what, numbers);
    ++g_counted;
    g_failed += !pass;
}

void Skip(const char* id, const char* what) {
    std::printf("%-4s SKIP  %s\n", id, what);
    ++g_skipped;
}

constexpr uint32_t kBase = 0x400000;
const char* P(int pose) {
    return stance::PoseName(pose);
}

// ------------------------------------------------------------------ a memory image ------------------------------------------------------------------

struct Image {
    std::map<uint32_t, std::vector<uint8_t>> pages;
    mutable int reads = 0;
    uint8_t* Page(uint32_t at) {
        std::vector<uint8_t>& p = pages[at >> 12];
        if (p.empty())
            p.assign(4096, 0);
        return p.data();
    }
    void Put(uint32_t at, const void* data, size_t n) {
        const uint8_t* b = static_cast<const uint8_t*>(data);
        for (size_t i = 0; i < n; ++i)
            Page(at + static_cast<uint32_t>(i))[(at + i) & 4095] = b[i];
    }
    void W32(uint32_t at, uint32_t v) { Put(at, &v, 4); }
    void W8(uint32_t at, uint8_t v) { Put(at, &v, 1); }
    void F32(uint32_t at, float v) { Put(at, &v, 4); }
    void Drop(uint32_t at) { pages.erase(at >> 12); }
    bool operator()(uint32_t at, void* out, size_t n) const {
        ++reads;
        uint8_t* o = static_cast<uint8_t*>(out);
        while (n) {
            const auto it = pages.find(at >> 12);
            if (it == pages.end())
                return false;
            const size_t off = at & 4095, take = n < 4096 - off ? n : 4096 - off;
            std::memcpy(o, it->second.data() + off, take);
            o += take;
            at += static_cast<uint32_t>(take);
            n -= take;
        }
        return true;
    }
};

// ------------------------------------------------------------- the game's side -------------------------------------------------------------

// The soldier's pose as bf3.exe 1.6.0.0's code keeps and changes it, its keys read at its own ticks (30 a second), written into the image where the
// reader finds it (the soldier's +0x164 its physics: +0x7C current, +0x80 changing to, +0x84 the time left, +0x90 the poses allowed, +0xB4 its data).
struct Game {
    int current = stance::kStand, target = stance::kStand;
    float timer = 0.0f;
    bool allowed[3] = {true, true, true};
    bool changeInTransition = false;  // (the soldier's data's: not known for the real one; both are run)
    bool room = true;                 // room to stand and crouch here (false: under something low, prone alone fits)
    bool onGround = true;
    bool keysRead = true;             // false: a scripted moment's lock, the soldier's keys not read
    bool alive = true;
    // The keyboard now (the mod's sends) and as the game saw it at its last tick.
    bool kx = false, kz = false, kctrl = false, kjump = false, ksprint = false;
    float kforward = 0.0f;
    bool sx = false, sz = false, sctrl = false, sjump = false;
    unsigned long long nextTick = 0;
    int xPresses = 0, zPresses = 0, ctrlEdges = 0, refused = 0;
    // Where it lives in the image.
    uint32_t soldier = 0xE5D79910u, physics = 0xE6100000u, data = 0xE6200000u;
    unsigned serial = 1;

    static float Time(int from, int to) { return from == stance::kProne || to == stance::kProne ? 1.0f : 0.3f; }
    bool Room(int pose) const { return room || pose == stance::kProne; }
    // setPose (0x00591D30): pose, a2 (again: even if it's in it), a3 (at once), check (the room for it).
    bool SetPose(int pose, bool a2, bool a3, bool check) {
        if (pose == current && target == current && !a2)
            return true;
        const bool during = a3 || a2 || changeInTransition;
        if (current != target && !during)
            return target == pose;
        if (!allowed[pose] || (check && !Room(pose))) {
            ++refused;
            return false;
        }
        target = pose;
        timer = a3 ? 0.0f : Time(current, pose);
        if (a3)
            current = target;
        return true;
    }
    // 0x005912E0: the target straight to a pose (the crouch-and-hold let go).
    void SetTarget(int pose) {
        if (!allowed[pose])
            return;
        target = pose;
        timer = Time(current, pose);
    }
    void Spawn(int pose, uint32_t newSoldier) {
        current = target = pose;
        timer = 0.0f;
        soldier = newSoldier;
        physics += 0x1000;
        data += 0x1000;
        ++serial;
        alive = true;
    }
    void Tick(float dt) {
        // The change's end (0x005913E0): refused there (no room), back to where it was.
        if (current != target) {
            timer -= dt;
            if (timer <= 0.0f) {
                timer = 0.0f;
                if (!Room(target) || !allowed[target])
                    target = current;
                else
                    current = target;
            }
        }
        const bool ctrlPressed = kctrl && !sctrl, ctrlLetGo = !kctrl && sctrl;
        const bool xPressed = kx && !sx, zPressed = kz && !sz, jumpPressed = kjump && !sjump;
        if (kctrl != sctrl)
            ++ctrlEdges;
        xPresses += xPressed;
        zPresses += zPressed;
        sx = kx, sz = kz, sctrl = kctrl, sjump = kjump;
        if (!keysRead || !alive)
            return;
        const bool wantSprint = ksprint && kforward > 0.5f;
        // The jump (0x011F6FF0): from prone, up (ProneToStandOnJump); from a crouch the jump stands it as well.
        if (jumpPressed && current == target && current != stance::kStand)
            SetPose(stance::kStand, false, false, true);
        // The pose input (0x011F7050), its order: crouch-and-hold pressed, let go; the crouch toggle; the prone toggle.
        const bool blocked = wantSprint;  // (no crouching while the sprint is wanted; deep water the same)
        if (ctrlPressed) {
            if (target != stance::kCrouch && !blocked)
                SetPose(stance::kCrouch, false, false, true);
        } else if (ctrlLetGo) {
            if (target != stance::kProne)
                SetTarget(stance::kStand);
        } else if (xPressed && !kctrl) {
            if (target == stance::kCrouch)
                SetPose(stance::kStand, false, false, true);
            else if (!blocked)
                SetPose(stance::kCrouch, false, false, true);
        } else if (zPressed && onGround) {
            if (target == stance::kProne)
                SetPose(stance::kStand, false, false, true);
            else
                SetPose(stance::kProne, false, false, false);
        }
        // The sprint's stand-up (0x011F8860: StandAutomaticallyIfSprinting, the soldier not standing).
        if (wantSprint && current != stance::kStand)
            SetPose(stance::kStand, false, false, true);
    }
    void Write(Image& m) const {
        m.W32(soldier, kBase + poseread::kSoldierVtable);
        m.W32(soldier + poseread::kSoldierPhysics, physics);
        m.W32(physics, kBase + poseread::kPhysicsVtable);
        m.W32(physics + poseread::kCurrent, static_cast<uint32_t>(current));
        m.W32(physics + poseread::kTarget, static_cast<uint32_t>(target));
        m.F32(physics + poseread::kTimer, timer);
        for (int i = 0; i < 3; ++i)
            m.W8(physics + poseread::kAllowed + i, allowed[i] ? 1 : 0);
        m.W32(physics + poseread::kData, data);
        m.W32(data, kBase + poseread::kDataVtable);
        m.W8(data + poseread::kDataChangeInTransition, changeInTransition ? 1 : 0);
    }
};

// ------------------------------------------------------------- the old stance -------------------------------------------------------------

// input.cpp's UpdateStance as it was (git 7e99b44), with the mapping loop's parts of it: the belief, g_crouchOn, a sprint held in the last 200 ms
// standing it, a jump held standing it, Z then X out of prone, the head's crouch only with the belief standing.
struct OldStance {
    int stance = stance::kStand;
    bool crouchOn = false;
    unsigned long long downSince = 0, upSince = 0, sprintAt = 0;
    bool downUsed = false;
    int key = 0, next = 0;
    unsigned long long keyUntil = 0, nextAt = 0;
    void Tap(Game& g, int k, int then, unsigned long long now) {
        if (key)
            Set(g, key, false);
        Set(g, k, true);
        key = k;
        keyUntil = now + 300;
        next = then;
        nextAt = keyUntil + 150;
    }
    static void Set(Game& g, int k, bool down) { (k == stance::kKeyX ? g.kx : g.kz) = down; }
    void Loop(bool sprint, bool jump, unsigned long long now) {
        if (sprint)
            sprintAt = now;
        if (jump && stance != stance::kStand) {
            crouchOn = false;
            stance = stance::kStand;
        }
    }
    bool HeadCrouch(bool wanted) const { return wanted && stance == stance::kStand; }
    void Update(Game& g, float x, float y, unsigned long long now) {
        if (key && now >= keyUntil) {
            Set(g, key, false);
            key = 0;
        }
        if (!key && next && now >= nextAt) {
            const int n = next;
            Tap(g, n, 0, now);
        }
        if (now - sprintAt < 200) {
            crouchOn = false;
            stance = stance::kStand;
        }
        const bool straight = std::fabs(x) < 0.35f * std::fabs(y);
        const bool down = y < -0.8f && straight, up = y > 0.8f && straight;
        if (down && !downSince) {
            downSince = now;
            downUsed = false;
        }
        if (downSince && !downUsed && down && now - downSince >= 450) {
            downUsed = true;
            if (stance != stance::kProne) {
                Tap(g, stance::kKeyZ, 0, now);
                stance = stance::kProne;
            }
        }
        if (downSince && y > -0.5f) {
            if (!downUsed && stance != stance::kCrouch) {
                if (stance == stance::kProne)
                    Tap(g, stance::kKeyZ, crouchOn ? 0 : stance::kKeyX, now);
                else
                    Tap(g, stance::kKeyX, 0, now);
                crouchOn = true;
                stance = stance::kCrouch;
            }
            downSince = 0;
        }
        if (up && !upSince) {
            upSince = now;
            if (stance != stance::kStand) {
                if (stance == stance::kProne)
                    Tap(g, stance::kKeyZ, crouchOn ? stance::kKeyX : 0, now);
                else
                    Tap(g, stance::kKeyX, 0, now);
                crouchOn = false;
                stance = stance::kStand;
            }
        }
        if (upSince && y < 0.5f)
            upSince = 0;
    }
};

// ------------------------------------------------------------- a run -------------------------------------------------------------

enum class Way { kOld, kRead, kNoRead };
const char* WayName(Way w) {
    return w == Way::kOld ? "the old stance" : w == Way::kRead ? "the new, the game's pose read" : "the new without the read";
}

struct Run {
    Way way;
    Game g;
    Image m;
    stance::State s;
    OldStance old;
    unsigned long long now = 1000000;
    // The controllers.
    float stickX = 0.0f, stickY = 0.0f;
    bool sprint = false, jump = false, cutscene = false, turn = false, fightGuard = false, headWanted = false;
    float forward = 0.0f;
    // What the mod did.
    int heldUps = 0;
    std::vector<stance::Event> events;
    std::string poses;  // the game's pose after each gesture, as letters (S C P)

    explicit Run(Way w, bool changeInTransition = false) : way(w) {
        g.changeInTransition = changeInTransition;
        g.Write(m);
        g.nextTick = now;
    }
    stance::Read ReadNow() {
        stance::Read r;
        if (way != Way::kRead)
            return r;
        const poseread::Pose p = poseread::Read(m, kBase, g.alive ? g.soldier : 0);
        r.ok = p.ok;
        r.current = p.current;
        r.target = p.target;
        r.timer = p.timer;
        for (int i = 0; i < 3; ++i)
            r.allowed[i] = p.allowed[i];
        r.changeInTransition = p.changeInTransition;
        return r;
    }
    // One of the mod's frames (14 ms), the game's ticks due in it first.
    void Frame() {
        now += 14;
        while (g.nextTick <= now) {
            g.ksprint = sprint;
            g.kjump = jump;
            g.kforward = forward;
            g.Tick(1.0f / 30.0f);
            g.Write(m);
            g.nextTick += 33;
        }
        if (way == Way::kOld) {
            old.Loop(sprint, jump, now);
            g.kctrl = old.HeadCrouch(headWanted);
            old.Update(g, stickX, stickY, now);
            return;
        }
        // input.cpp's order: the mapping loop's crouch-and-hold first (from the state as the last frame left it), then UpdateStance.
        g.kctrl = headWanted && (g.kctrl ? stance::HeadMayHold(s) : stance::HeadMayStart(s));
        stance::Frame f;
        f.now = now;
        f.stickX = stickX;
        f.stickY = stickY;
        f.read = ReadNow();
        f.soldier = g.alive ? g.soldier : 0;
        f.noSoldier = way == Way::kRead && !g.alive;  // (the reader borne out, the soldier pointer 0; unknown game: the belief goes on)
        f.serial = g.serial;
        f.sprint = sprint;
        f.forward = forward;
        f.jump = jump;
        f.cutscene = cutscene;
        f.turn = turn;
        f.fightGuard = fightGuard;
        f.headHold = g.kctrl;
        const stance::Out o = stance::Step(s, f);
        if (o.release)
            (o.release == stance::kKeyX ? g.kx : g.kz) = false;
        if (o.press)
            (o.press == stance::kKeyX ? g.kx : g.kz) = true;
        heldUps += o.heldUp;
        for (int i = 0; i < o.events; ++i)
            events.push_back(o.event[i]);
        if (watch)
            watch(*this);
    }
    // Called after each of the mod's frames (row 16: the log's lines of the game's pose, as input.cpp's WatchPose reads it every frame).
    std::function<void(Run&)> watch;
    void Wait(unsigned long long ms) {
        const unsigned long long until = now + ms;
        while (now < until)
            Frame();
    }
    void Stick(float y, unsigned long long ms) {
        stickY = y;
        Wait(ms);
        stickY = 0.0f;
    }
    // The gestures as a thumb makes them, then the time for the game's change (a prone one takes a second).
    void FlickDown() { Stick(-1.0f, 150); Wait(1500); Note(); }
    void HoldDown() { Stick(-1.0f, 700); Wait(1500); Note(); }
    void FlickUp() { Stick(1.0f, 150); Wait(1500); Note(); }
    void Note() { poses += g.target == stance::kStand ? 'S' : g.target == stance::kCrouch ? 'C' : 'P'; }
    void SprintClick() {
        sprint = true;
        Wait(100);
        sprint = false;
        Wait(300);
    }
    int Count(stance::Ev what) const {
        int n = 0;
        for (const stance::Event& e : events)
            n += e.what == what;
        return n;
    }
    // The stance as the mod takes it.
    int Taken() const { return way == Way::kOld ? old.stance : s.pose; }
};

// What a run did to the game, as the poses after each gesture against those wanted ("CSC" ...).
bool Matches(const Run& r, const char* want) {
    return r.poses == want;
}

// ------------------------------------------------------------- 1-10 the runs -------------------------------------------------------------

void OwnersSequence() {
    // 00:58:34 crouched; 00:58:56 "flicked down: crouched, from standing" again after a sprint press; then the inverted toggles of 01:03:11-01:03:17.
    for (Way w : {Way::kOld, Way::kRead, Way::kNoRead}) {
        Run r(w);
        r.FlickDown();    // crouched
        r.SprintClick();  // the left stick's click standing still: the game stays crouched
        r.FlickDown();    // still crouched
        r.FlickUp();      // standing
        r.FlickDown();    // crouched
        char what[160];
        std::snprintf(what, sizeof(what), "the owner's sequence (crouch, a sprint press standing still, down, up, down), %s", WayName(w));
        if (w == Way::kOld)
            Report("1a", "the old stance goes wrong as in the headset: down stands, up crouches", r.poses == "CSCS",
                   "the game's pose after each gesture %s (wanted CCSC; the belief after the sprint press: standing, the soldier crouched)", r.poses.c_str());
        else
            Report(w == Way::kRead ? "1b" : "1c", what, Matches(r, "CCSC") && r.g.current == stance::kCrouch && r.Taken() == stance::kCrouch && r.g.xPresses == 3,
                   "the game's pose after each gesture %s (wanted CCSC), the mod's %s; X pressed %d times (wanted 3), Z %d", r.poses.c_str(), P(r.Taken()),
                   r.g.xPresses, r.g.zPresses);
    }
}

void DeathWhileProne() {
    for (Way w : {Way::kOld, Way::kRead, Way::kNoRead}) {
        Run r(w);
        r.FlickDown();  // crouched
        r.HoldDown();   // prone
        // 01:02:04 "out of the level": the soldier dies, a moment without one (01:02:05 "flicked down: crouched, from prone" then), the new soldier standing.
        r.g.alive = false;
        r.Wait(300);
        const int xBefore = r.g.xPresses, zBefore = r.g.zPresses;
        r.Stick(-1.0f, 150);
        r.Wait(1500);
        const int keysInDeath = r.g.xPresses - xBefore + r.g.zPresses - zBefore;
        r.g.Spawn(stance::kStand, 0xE5C08480u);
        r.Wait(1500);
        r.FlickDown();  // crouched
        r.FlickUp();    // standing
        if (w == Way::kOld)
            Report("2a", "the old stance through a death while prone: its down in the death (Z, from a belief of prone) taken as crouched, so the new soldier's down "
                         "does nothing and its up crouches it",
                   r.poses == "CPSC", "the game's pose after each gesture %s (wanted CPCS)", r.poses.c_str());
        else
            // (With the read, a frame may come between the new soldier's pointer and its physics read: taken as standing then, as a new soldier is.)
            Report(w == Way::kRead ? "2b" : "2c",
                   w == Way::kRead ? "a death while prone, the game's pose read: no key while there's no soldier, the new soldier stands, down and up right"
                                   : "a death while prone without the read: the new soldier taken as standing, down and up right",
                   Matches(r, "CPCS") && r.Count(stance::Ev::kStood) == 1 && r.g.zPresses == 1 && (w == Way::kNoRead || (keysInDeath == 0 && r.Count(stance::Ev::kNoSoldier) == 1)),
                   "the game's pose after each gesture %s (wanted CPCS); taken as standing for a new soldier %d time(s); Z %d (wanted 1: no prone for the new one); "
                   "keys with no soldier %d, said %d",
                   r.poses.c_str(), r.Count(stance::Ev::kStood), r.g.zPresses, keysInDeath, r.Count(stance::Ev::kNoSoldier));
    }
}

void CheckpointProne() {
    for (Way w : {Way::kOld, Way::kRead, Way::kNoRead}) {
        Run r(w);
        r.Wait(500);
        r.g.Spawn(stance::kProne, 0xE5D520A0u);  // the RPG checkpoint lays the new soldier prone
        r.Wait(1500);
        r.FlickUp();
        r.FlickDown();
        r.FlickUp();
        if (w == Way::kOld)
            Report("3a", "the old stance at a checkpoint that lays the soldier prone: up does nothing", r.poses[0] == 'P',
                   "the game's pose after each gesture %s", r.poses.c_str());
        else if (w == Way::kRead)
            Report("3b", "the checkpoint's prone soldier, the game's pose read: up stands it, down crouches, up stands", Matches(r, "SCS"),
                   "the game's pose after each gesture %s (wanted SCS)", r.poses.c_str());
        else
            Report("3c", "... without the read: up does nothing (prone not known), down crouches it (X from prone), and from there right", Matches(r, "PCS"),
                   "the game's pose after each gesture %s (wanted PCS: the fallback's known limit at such a checkpoint)", r.poses.c_str());
    }
}

void ProneFromCrouch() {
    // Both ways the soldier's data may have it: the old's X after its Z lands in the game's change out of prone, lost (0) or taken (1).
    for (bool during : {false, true}) {
        for (Way w : {Way::kOld, Way::kRead, Way::kNoRead}) {
            Run r(w, during);
            r.FlickDown();  // crouched
            r.HoldDown();   // prone
            r.FlickUp();    // standing (the old: Z stands, then its X crouches if the game takes it in the change)
            r.FlickDown();  // crouched
            r.HoldDown();   // prone
            r.FlickDown();  // crouched (the old: Z alone, standing)
            char id[8];
            std::snprintf(id, sizeof(id), "4%c", "abcdef"[(during ? 3 : 0) + (w == Way::kOld ? 0 : w == Way::kRead ? 1 : 2)]);
            char what[200];
            if (w == Way::kOld)
                std::snprintf(what, sizeof(what), "the old stance out of a prone that came from a crouch (a change during a change %s): %s", during ? "taken" : "lost",
                              during ? "up crouches, then down stands, and down from prone stands" : "down from prone stands");
            else
                std::snprintf(what, sizeof(what), "prone from a crouch, then up and down, %s (a change during a change %s): one key each", WayName(w),
                              during ? "taken" : "lost");
            if (w == Way::kOld)
                Report(id, what, r.poses == (during ? "CPCSPS" : "CPSCPS"), "the game's pose after each gesture %s (wanted CPSCPC)", r.poses.c_str());
            else
                Report(id, what, Matches(r, "CPSCPC") && r.g.xPresses == 3 && r.g.zPresses == 3,
                       "the game's pose after each gesture %s (wanted CPSCPC); X %d, Z %d (wanted 3 and 3)", r.poses.c_str(), r.g.xPresses, r.g.zPresses);
        }
    }
}

void GameStandUps() {
    for (Way w : {Way::kRead, Way::kNoRead}) {
        Run r(w);
        r.FlickDown();  // crouched
        r.sprint = true;
        r.forward = 1.0f;  // sprinting forward: the game stands it
        r.Wait(1000);
        r.sprint = false;
        r.forward = 0.0f;
        r.Wait(500);
        const int afterSprint = r.g.target, takenAfterSprint = r.Taken();
        r.FlickDown();  // crouched again
        r.HoldDown();   // prone
        r.SprintClick();  // standing still: nothing
        const int afterClick = r.g.target, takenAfterClick = r.Taken();
        r.jump = true;  // a jump from prone stands it
        r.Wait(150);
        r.jump = false;
        r.Wait(1500);
        const int afterJump = r.g.target, takenAfterJump = r.Taken();
        r.FlickDown();
        Report(w == Way::kRead ? "5a" : "5b",
               w == Way::kRead ? "the game's own stand-ups, read: the sprint forward stands it, a sprint press prone doesn't, a jump from prone does"
                               : "... without the read: taken into the belief only as the game makes them (the stick forward over half, the jump)",
               afterSprint == stance::kStand && takenAfterSprint == stance::kStand && afterClick == stance::kProne && takenAfterClick == stance::kProne &&
                   afterJump == stance::kStand && takenAfterJump == stance::kStand && r.poses == "CCPC",
               "after the sprint forward the game %s, the mod %s; after a sprint press prone %s / %s; after the jump %s / %s; gestures %s (wanted CCPC)",
               P(afterSprint), P(takenAfterSprint), P(afterClick), P(takenAfterClick), P(afterJump), P(takenAfterJump), r.poses.c_str());
    }
}

void ChangeDuringChange() {
    for (Way w : {Way::kOld, Way::kRead}) {
        Run r(w, false);
        r.Stick(-1.0f, 700);  // held down: prone (a second's change)
        r.Wait(300);
        r.Stick(1.0f, 150);   // up while the change to prone is still under way
        r.Wait(2500);
        r.Note();
        if (w == Way::kOld)
            Report("6a", "the old stance: up during the game's change to prone is lost (the game takes no change then), the belief standing", r.poses == "P" && r.Taken() == stance::kStand,
                   "the game %s, the belief %s", r.poses.c_str(), P(r.Taken()));
        else
            Report("6b", "up during the game's change to prone, the game's pose read: its end awaited, then Z: standing", r.poses == "S" && r.g.zPresses == 2,
                   "the game %s (wanted S), Z pressed %d times (wanted 2)", r.poses.c_str(), r.g.zPresses);
    }
    {
        Run r(Way::kRead, true);  // a soldier whose data takes a change during a change: pressed at once
        r.Stick(-1.0f, 700);
        r.Stick(1.0f, 150);
        r.Wait(2500);
        r.Note();
        Report("6c", "... with the data's AllowPoseChangeDuringTransition 1: pressed without waiting, standing", r.poses == "S", "the game %s (wanted S)", r.poses.c_str());
    }
}

void ScriptedLocks() {
    {
        Run r(Way::kRead);
        r.g.allowed[stance::kCrouch] = false;  // the level's constraints: no crouching
        r.FlickDown();
        const int xLocked = r.g.xPresses;
        r.g.allowed[stance::kCrouch] = true;
        r.FlickDown();
        Report("7a", "a pose the game doesn't allow now never pressed for (said), and once allowed: crouched", r.poses == "SC" && xLocked == 0 && r.Count(stance::Ev::kNotAllowed) == 1,
               "the game %s (wanted SC), X while locked %d (wanted 0), said %d", r.poses.c_str(), xLocked, r.Count(stance::Ev::kNotAllowed));
    }
    {
        Run r(Way::kRead);
        r.g.keysRead = false;  // the RPG state: the soldier's keys not read
        r.FlickDown();
        const int xLocked = r.g.xPresses, notTaken = r.Count(stance::Ev::kNotTaken);
        const int taken = r.Taken();
        r.g.keysRead = true;
        r.FlickDown();
        Report("7b", "the game not reading the keys, read: X pressed twice, not taken, given up; the stance stays the game's; after the lock: crouched",
               r.poses == "SC" && xLocked == 2 && notTaken == 2 && taken == stance::kStand,
               "the game %s (wanted SC), X while locked %d (wanted 2), not-taken lines %d, the mod's stance then %s", r.poses.c_str(), xLocked, notTaken, P(taken));
    }
    {
        Run r(Way::kNoRead);
        r.g.keysRead = false;
        r.turn = true;  // the game turning the look itself
        r.FlickDown();
        const int xLocked = r.g.xPresses, taken = r.Taken();
        r.turn = false;
        r.g.keysRead = true;
        r.FlickDown();
        Report("7c", "... without the read, in a scripted moment: nothing sent, the belief unchanged; after it: crouched",
               r.poses == "SC" && xLocked == 0 && taken == stance::kStand && r.Count(stance::Ev::kScripted) == 1,
               "the game %s (wanted SC), X then %d (wanted 0), the belief then %s, said %d", r.poses.c_str(), xLocked, P(taken), r.Count(stance::Ev::kScripted));
    }
    {
        Run r(Way::kOld);
        r.g.keysRead = false;
        r.FlickDown();
        r.g.keysRead = true;
        r.FlickUp();
        Report("7d", "the old stance after a lock that ignored X: its belief crouched, so up crouches the standing soldier", r.poses == "SC",
               "the game %s (wanted SS)", r.poses.c_str());
    }
}

void NoRoom() {
    Run r(Way::kRead);
    r.HoldDown();  // prone
    r.g.room = false;  // crawled under something low
    r.FlickUp();
    Report("8", "no room to stand: Z refused by the game, pressed again once, given up; still prone, the mod's stance prone",
           r.poses == "PP" && r.g.zPresses == 3 && r.Count(stance::Ev::kNotTaken) == 2 && r.Taken() == stance::kProne,
           "the game %s (wanted PP), Z %d (wanted 3), not taken %d", r.poses.c_str(), r.g.zPresses, r.Count(stance::Ev::kNotTaken));
}

void HeadCrouch() {
    for (Way w : {Way::kRead, Way::kNoRead}) {
        Run r(w);
        r.headWanted = true;  // the head down: Left Ctrl held, the game crouched
        r.Wait(3000);
        const int edgesHeld = r.g.ctrlEdges, crouchedByHead = r.g.target;
        r.FlickDown();  // the stick's crouch: Left Ctrl let go, the game stands, X crouches
        const int edgesAfter = r.g.ctrlEdges;
        r.Wait(2000);    // the head still down: Left Ctrl not taken again (crouched by the stick)
        const int edgesLater = r.g.ctrlEdges;
        r.headWanted = false;
        r.Wait(500);
        r.FlickUp();
        r.headWanted = true;  // down again, standing: the head's crouch again
        r.Wait(1000);
        const int headAgain = r.g.target;
        Report(w == Way::kRead ? "9a" : "9b",
               w == Way::kRead ? "the head's real crouch, read: Left Ctrl held steady (never on and off), a flick lets it go and the game crouches by X, "
                                 "not taken again while the stick's crouch stands, again once standing"
                               : "... without the read: the same",
               crouchedByHead == stance::kCrouch && edgesHeld == 1 && edgesAfter == 2 && edgesLater == 2 && r.poses == "CS" && headAgain == stance::kCrouch,
               "crouched by the head %s, Left Ctrl's changes held %d (wanted 1), after the flick %d (wanted 2), 2 s later %d (wanted 2); gestures %s (wanted CS); "
               "the head again %s",
               P(crouchedByHead), edgesHeld, edgesAfter, edgesLater, r.poses.c_str(), P(headAgain));
    }
}

void GuardAndCutscene() {
    {
        Run r(Way::kRead);
        r.fightGuard = true;
        r.FlickDown();
        r.fightGuard = false;
        r.FlickDown();
        Report("10a", "a fist fight's prompt up: the gesture left undone (said by the caller), after it crouched", r.poses == "SC" && r.heldUps >= 1,
               "the game %s (wanted SC), held up %d", r.poses.c_str(), r.heldUps);
    }
    {
        Run r(Way::kRead);
        r.cutscene = true;
        r.FlickDown();
        r.HoldDown();
        r.cutscene = false;
        r.FlickDown();
        Report("10b", "a cutscene's camera: nothing pressed (said once), after it crouched", r.poses == "SSC" && r.Count(stance::Ev::kScripted) == 1,
               "the game %s (wanted SSC), said %d", r.poses.c_str(), r.Count(stance::Ev::kScripted));
    }
}

void Rules() {
    int right = 0, total = 0;
    for (int from = 0; from < 3; ++from)
        for (int want = 0; want < 3; ++want, ++total)
            right += stance::AfterKey(stance::KeyFor(from, want), from) == want && (from == want) == (stance::KeyFor(from, want) == stance::kNoKey);
    Report("11", "the rules: every pose to every pose by KeyFor's one key, as the game's AfterKey has it (X stand<->crouch and prone->crouch, Z into and out of prone)",
           right == total, "%d of %d", right, total);
}

// ------------------------------------------------------------- 12 the reader -------------------------------------------------------------

void Reader() {
    Game g;
    g.current = stance::kCrouch;
    g.target = stance::kProne;
    g.timer = 0.6f;
    g.allowed[2] = false;
    g.changeInTransition = true;
    Image m;
    g.Write(m);
    const poseread::Pose p = poseread::Read(m, kBase, g.soldier);
    Report("12a", "the reader on a soldier: the pose it's in, the one it changes to, the time left, the poses allowed, the data's rule", p.ok && p.current == 1 &&
           p.target == 2 && std::fabs(p.timer - 0.6f) < 1e-6f && p.allowed[0] && p.allowed[1] && !p.allowed[2] && p.changeInTransition && p.physics == g.physics,
           "ok %d, %s -> %s, %.2f s, allowed %d %d %d, change in a change %d", p.ok, P(p.current), P(p.target), p.timer, p.allowed[0], p.allowed[1], p.allowed[2],
           p.changeInTransition);
    struct Case {
        const char* what;
        poseread::Miss want;
        void (*breakIt)(Image&, const Game&);
    };
    const Case cases[] = {
        {"no soldier", poseread::Miss::kNoSoldier, nullptr},
        {"the soldier's vtable another's (a vehicle)", poseread::Miss::kSoldierType, [](Image& i, const Game& gg) { i.W32(gg.soldier, kBase + 0x1D4BD48); }},
        {"the soldier not readable", poseread::Miss::kSoldierType, [](Image& i, const Game& gg) { i.Drop(gg.soldier); }},
        {"no physics", poseread::Miss::kNoPhysics, [](Image& i, const Game& gg) { i.W32(gg.soldier + poseread::kSoldierPhysics, 0); }},
        {"the physics' vtable another's", poseread::Miss::kNoPhysics, [](Image& i, const Game& gg) { i.W32(gg.physics, kBase + poseread::kDataVtable); }},
        {"the physics not readable", poseread::Miss::kNoPhysics, [](Image& i, const Game& gg) { i.Drop(gg.physics); }},
        {"the current pose 5", poseread::Miss::kValues, [](Image& i, const Game& gg) { i.W32(gg.physics + poseread::kCurrent, 5); }},
        {"the target -1", poseread::Miss::kValues, [](Image& i, const Game& gg) { i.W32(gg.physics + poseread::kTarget, 0xFFFFFFFFu); }},
        {"the time not a number", poseread::Miss::kValues, [](Image& i, const Game& gg) { i.F32(gg.physics + poseread::kTimer, std::nanf("")); }},
        {"the time 1e6 s", poseread::Miss::kValues, [](Image& i, const Game& gg) { i.F32(gg.physics + poseread::kTimer, 1e6f); }},
        {"no data", poseread::Miss::kData, [](Image& i, const Game& gg) { i.W32(gg.physics + poseread::kData, 0); }},
        {"the data's vtable another's", poseread::Miss::kData, [](Image& i, const Game& gg) { i.W32(gg.data, kBase + poseread::kPhysicsVtable); }},
        {"the data not readable", poseread::Miss::kData, [](Image& i, const Game& gg) { i.Drop(gg.data); }},
    };
    int right = 0;
    char wrong[512] = "";
    for (const Case& c : cases) {
        Image mm;
        g.Write(mm);
        if (c.breakIt)
            c.breakIt(mm, g);
        const poseread::Pose q = poseread::Read(mm, kBase, c.breakIt ? g.soldier : 0);
        if (!q.ok && q.miss == c.want)
            ++right;
        else
            std::snprintf(wrong + std::strlen(wrong), sizeof(wrong) - std::strlen(wrong), " [%s: %s]", c.what, poseread::MissName(q.miss));
    }
    Report("12b", "the reader: every check failing alone, named (no soldier, a vehicle's vtable, no physics, not poses, a time not a time, no data, unreadable)",
           right == static_cast<int>(sizeof(cases) / sizeof(cases[0])), "%d of %zu%s", right, sizeof(cases) / sizeof(cases[0]), wrong);
}

// ------------------------------------------------------------- 13 the proofs -------------------------------------------------------------

std::vector<uint8_t> Bytes(const char* pattern, uint8_t fill = 0xCC) {
    std::vector<uint8_t> b;
    for (const char* p = pattern; *p;) {
        if (*p == ' ') {
            ++p;
            continue;
        }
        if (p[0] == '?') {
            b.push_back(fill);
        } else {
            unsigned v = 0;
            std::sscanf(p, "%2x", &v);
            b.push_back(static_cast<uint8_t>(v));
        }
        p += 2;
    }
    return b;
}

// Everything the 16 proofs read, made up where bf3.exe 1.6.0.0 has it. names: each vtable's name's address.
void ProofImage(Image& m, std::map<uint32_t, uint32_t>& names) {
    uint32_t fn = kBase + 0x1E80000, info = kBase + 0x2010000;
    const auto named = [&](uint32_t vtable, const char* name) {
        m.W32(vtable, fn);
        const uint8_t code[6] = {0xB8, static_cast<uint8_t>(info), static_cast<uint8_t>(info >> 8), static_cast<uint8_t>(info >> 16),
                                 static_cast<uint8_t>(info >> 24), 0xC3};
        m.Put(fn, code, 6);
        m.W32(info, info + 0x10);
        m.W32(info + 0x10, info + 0x20);
        m.Put(info + 0x20, name, std::strlen(name) + 1);
        names[vtable] = info + 0x20;
        fn += 0x10;
        info += 0x80;
    };
    const auto code = [&](uint32_t rva, const char* pattern) {
        const std::vector<uint8_t> b = Bytes(pattern);
        m.Put(kBase + rva, b.data(), b.size());
    };
    named(kBase + poseread::kSoldierVtable, "ClientSoldierEntity");
    named(kBase + poseread::kPhysicsVtable, "CharacterPhysicsEntity");
    named(kBase + poseread::kDataVtable, "CharacterPhysicsData");
    code(poseread::kPhysicsCtor, poseread::kPhysicsCtorBytes);
    m.W32(kBase + poseread::kPhysicsCtor + 2, kBase + poseread::kPhysicsVtable);
    code(poseread::kGetPose, poseread::kGetPoseBytes);
    code(poseread::kGetCurrentPose, poseread::kGetCurrentPoseBytes);
    code(poseread::kSetPose, poseread::kSetPoseBytes);
    code(poseread::kSetPoseAllowed, poseread::kSetPoseAllowedBytes);
    code(poseread::kSetPoseWrite, poseread::kSetPoseWriteBytes);
    code(poseread::kAllowedTest, poseread::kAllowedTestBytes);
    code(poseread::kSoldierMove, poseread::kSoldierMoveBytes);
    code(poseread::kMovePhysics, poseread::kMovePhysicsBytes);
    code(poseread::kSoldierCtor, poseread::kSoldierCtorBytes);
    code(poseread::kInputGetPose, poseread::kInputGetPoseBytes);
    code(poseread::kInputCrouch, poseread::kInputCrouchBytes);
    code(poseread::kInputCrouchRule, poseread::kInputCrouchRuleBytes);
    code(poseread::kInputProne, poseread::kInputProneBytes);
}

// Proof i not borne out by one change: a type's name, a byte of its code, the constructor's vtable.
void BreakProof(Image& m, int i, std::map<uint32_t, uint32_t>& names) {
    const auto name = [&](uint32_t vtable) { m.W8(names[vtable] + 1, 'X'); };
    const auto flip = [&](uint32_t rva, uint32_t offset) {
        uint8_t b = 0;
        m(kBase + rva + offset, &b, 1);
        m.W8(kBase + rva + offset, static_cast<uint8_t>(b ^ 0x01));
    };
    switch (i) {
    case 0: name(kBase + poseread::kSoldierVtable); break;
    case 1: name(kBase + poseread::kPhysicsVtable); break;
    case 2: name(kBase + poseread::kDataVtable); break;
    case 3: m.W32(kBase + poseread::kPhysicsCtor + 2, kBase + poseread::kPhysicsVtable + 8); break;
    case 4: flip(poseread::kGetPose, 2); break;            // (+0x81)
    case 5: flip(poseread::kGetCurrentPose, 2); break;     // (+0x7D)
    case 6: flip(poseread::kSetPose, 5); break;            // (+0x7D)
    case 7: flip(poseread::kSetPoseAllowed, 3); break;     // (+0x91)
    case 8: flip(poseread::kSetPoseWrite, 2); break;       // (+0x81)
    case 9: flip(poseread::kAllowedTest, 6); break;
    case 10: flip(poseread::kSoldierMove, 17); break;      // (+0x165)
    case 11: flip(poseread::kMovePhysics, 2); break;       // (+0x75)
    case 12: flip(poseread::kSoldierCtor, 2); break;
    case 13: flip(poseread::kInputGetPose, 3); break;      // (another call)
    case 14: flip(poseread::kInputCrouchRule, 2); break;   // (cmp ebp, 0)
    case 15: flip(poseread::kInputProne, 26); break;       // (cmp ebp, 3)
    default: break;
    }
}

struct FileImage {
    std::vector<uint8_t> bytes;
    bool operator()(uint32_t at, void* out, size_t n) const {
        if (at < kBase || at - kBase > bytes.size() || n > bytes.size() - (at - kBase))
            return false;
        std::memcpy(out, bytes.data() + (at - kBase), n);
        return true;
    }
};

bool LoadFile(const std::wstring& path, std::vector<uint8_t>& out) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"rb") != 0 || !f)
        return false;
    std::fseek(f, 0, SEEK_END);
    const long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    out.resize(size > 0 ? static_cast<size_t>(size) : 0);
    const size_t got = out.empty() ? 0 : std::fread(out.data(), 1, out.size(), f);
    std::fclose(f);
    return got == out.size() && !out.empty();
}

std::wstring OwnFolder() {
    wchar_t self[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    std::wstring s = self;
    return s.substr(0, s.find_last_of(L"\\/") + 1);
}

void Proofs() {
    {
        Image m;
        std::map<uint32_t, uint32_t> names;
        ProofImage(m, names);
        const poseread::Proofs all = poseread::Prove(m, kBase);
        int alone = 0;
        char wrong[256] = "";
        for (int i = 0; i < poseread::kProofs; ++i) {
            Image b;
            std::map<uint32_t, uint32_t> n;
            ProofImage(b, n);
            BreakProof(b, i, n);
            const poseread::Proofs p = poseread::Prove(b, kBase);
            if (!p.ok && p.failed == i)
                ++alone;
            else
                std::snprintf(wrong + std::strlen(wrong), sizeof(wrong) - std::strlen(wrong), " %d (%s)", i, p.ok ? "borne out" : "another failed");
        }
        Report("13a", "the proofs on a made-up image: all borne out; each broken alone fails alone, named", all.ok && alone == poseread::kProofs,
               "all %s; %d of %d broken alone found%s", all.ok ? "borne out" : poseread::ProofName(all.failed), alone, poseread::kProofs, wrong);
    }
    FileImage real;
    if (LoadFile(OwnFolder() + L"bf3_mem.exe", real.bytes) && real.bytes.size() >= 0x2000000) {
        int borne = 0;
        char notBorne[512] = "";
        for (int i = 0; i < poseread::kProofs; ++i) {
            uint32_t at = 0;
            if (poseread::ProveOne(real, kBase, i, &at))
                ++borne;
            else
                std::snprintf(notBorne + std::strlen(notBorne), sizeof(notBorne) - std::strlen(notBorne), " %d %s (%08X)", i, poseread::ProofName(i), at);
        }
        const poseread::Proofs p = poseread::Prove(real, kBase);
        Report("13b", "the real code (build\\bf3_mem.exe, bf3.exe 1.6.0.0 decrypted): every one of the reader's constants borne out", p.ok && borne == poseread::kProofs,
               "%d of %d borne out%s", borne, poseread::kProofs, notBorne);
    } else {
        Skip("13b", "no build\\bf3_mem.exe beside the test: the constants against the real code not checked");
    }
}

// ------------------------------------------------------------- 15 the review's cases -------------------------------------------------------------

// The review of 7b43fdf (2026-11-19), on the game's model: a quick gesture after a key into or out of prone, a scripted moment's one press, the
// head's crouch named when a flick up does nothing, a change under way when a fight's prompt opens.
void ReviewCases() {
    // 15a, 15b: held down (Z at 0.45 s), flicked up 0.12 s after it (the owner's 01:10:10.848 and .969), the soldier's data taking no change during a
    // change (a second's change into prone): without the read the next key went 0.45 s after the first, into the change, and was refused (the soldier
    // prone, the belief standing: the toggles backwards again); now it waits for the change's end (kProneGap).
    for (Way w : {Way::kRead, Way::kNoRead}) {
        Run r(w, false);
        r.Stick(-1.0f, 470);  // (the hold's Z at 450 ms)
        r.Wait(100);
        r.Stick(1.0f, 120);
        r.Wait(3000);
        r.Note();
        Report(w == Way::kRead ? "15a" : "15b",
               w == Way::kRead ? "held down, then up 0.12 s after the prone's Z (a soldier taking no change during a change), read: standing"
                               : "... without the read: the next key waits for the change into prone to end (kProneGap): standing, the belief the game's",
               r.poses == "S" && r.g.current == stance::kStand && r.Taken() == stance::kStand && r.g.zPresses == 2,
               "the game %s (wanted S), the mod's %s, Z %d (wanted 2)", r.poses.c_str(), P(r.Taken()), r.g.zPresses);
    }
    // 15c, 15d: out of prone by a flick up (Z, a second's change), then flicked down 0.3 s after it: crouched, by X once the change has ended.
    for (Way w : {Way::kRead, Way::kNoRead}) {
        Run r(w, false);
        r.HoldDown();  // prone
        r.Stick(1.0f, 120);
        r.Wait(300);
        r.Stick(-1.0f, 120);
        r.Wait(3000);
        r.Note();
        Report(w == Way::kRead ? "15c" : "15d",
               w == Way::kRead ? "out of prone by a flick up, then down 0.3 s after it, read: X once the game's change has ended, crouched"
                               : "... without the read: the same by kProneGap, crouched, the belief the game's",
               r.poses == "PC" && r.g.current == stance::kCrouch && r.Taken() == stance::kCrouch && r.g.xPresses == 1 && r.g.zPresses == 2,
               "the game %s (wanted PC), the mod's %s; X %d (wanted 1), Z %d (wanted 2)", r.poses.c_str(), P(r.Taken()), r.g.xPresses, r.g.zPresses);
    }
    // 15e: read, a scripted moment whose game takes no keys (the game turning the look itself; its keys perhaps its own then, a quick-time event's Z):
    // one press a gesture, given up, never pressed twice (the review: two a gesture went); after the moment crouched by one X.
    {
        Run r(Way::kRead);
        r.g.keysRead = false;
        r.turn = true;
        r.FlickDown();
        r.HoldDown();
        const int xIn = r.g.xPresses, zIn = r.g.zPresses;
        int scriptedGiveUps = 0;
        for (const stance::Event& e : r.events)
            scriptedGiveUps += e.what == stance::Ev::kNotTaken && e.d == 2;
        r.turn = false;
        r.g.keysRead = true;
        r.FlickDown();
        Report("15e", "a scripted moment that takes no keys, read: one press a gesture, given up (said), never pressed twice; after it crouched",
               r.poses == "SSC" && xIn == 1 && zIn == 1 && scriptedGiveUps == 2 && r.g.xPresses == 2,
               "the game %s (wanted SSC), X %d and Z %d in the moment (wanted 1 and 1), given up there %d (wanted 2), X in all %d (wanted 2)",
               r.poses.c_str(), xIn, zIn, scriptedGiveUps, r.g.xPresses);
    }
    // 15f, 15g: a scripted moment just after going prone (the owner's log: 00:59:18.307 Z, 00:59:19.175 "the game turned the look yaw itself by 18.0
    // degrees", the game's own turn of the look as the soldier went down): read, a flick up in it still stands the soldier (its first press goes);
    // without the read nothing is sent in a scripted moment (the belief would part if the game took none), so up stands it once the moment is over.
    for (Way w : {Way::kRead, Way::kNoRead}) {
        Run r(w);
        r.HoldDown();  // prone
        r.turn = true;
        r.FlickUp();
        r.turn = false;
        r.FlickUp();
        if (w == Way::kRead)
            Report("15f", "a scripted moment just after going prone (the game's own turn of the look), read: a flick up in it still stands the soldier",
                   r.poses == "PSS" && r.g.zPresses == 2, "the game %s (wanted PSS), Z %d (wanted 2)", r.poses.c_str(), r.g.zPresses);
        else
            Report("15g", "... without the read: nothing sent in the moment (said once), the belief prone; after it up stands",
                   r.poses == "PPS" && r.Count(stance::Ev::kScripted) == 1 && r.Taken() == stance::kStand && r.g.zPresses == 2,
                   "the game %s (wanted PPS), said %d, the mod's %s, Z %d (wanted 2)", r.poses.c_str(), r.Count(stance::Ev::kScripted), P(r.Taken()),
                   r.g.zPresses);
    }
    // 15h, 15i: the head's real crouch holding the game's crouch-and-hold, a flick up: nothing pressed, and its line names the head's crouch (the review:
    // it said "already standing (the game's pose)" with the game crouched).
    for (Way w : {Way::kRead, Way::kNoRead}) {
        Run r(w);
        r.headWanted = true;
        r.Wait(2000);
        r.FlickUp();
        int named = 0, poseSaid = -1;
        for (const stance::Event& e : r.events)
            if (e.what == stance::Ev::kAlready && e.c == 1) {
                ++named;
                poseSaid = e.d;
            }
        const int wantSaid = w == Way::kRead ? stance::kCrouch : stance::kStand;  // (the game's pose, read; the belief, which the head's crouch leaves)
        Report(w == Way::kRead ? "15h" : "15i",
               w == Way::kRead ? "the head's crouch holding, a flick up, read: nothing pressed, its line names the head's crouch and the game's pose crouched"
                               : "... without the read: nothing pressed, the head's crouch named",
               r.poses == "C" && r.g.xPresses == 0 && r.g.zPresses == 0 && named == 1 && poseSaid == wantSaid,
               "the game %s (wanted C), X %d Z %d (wanted 0 0), named %d (wanted 1), the pose said %s (wanted %s)", r.poses.c_str(), r.g.xPresses,
               r.g.zPresses, named, P(poseSaid), P(wantSaid));
    }
    // 15j: a change under way (its press not taken in time) when a fist fight's prompt opens, read: given up then (the caller's "left undone" line), never
    // pressed after the prompt (the review: it was pressed once the prompt was down; without the read it was dropped already).
    {
        Run r(Way::kRead);
        r.g.keysRead = false;
        r.Stick(-1.0f, 100);  // flicked down: X
        r.Wait(200);
        r.fightGuard = true;  // the prompt opens before the 0.7 s of the press's taking are up
        r.Wait(1500);
        const int xDuring = r.g.xPresses;
        r.fightGuard = false;
        r.g.keysRead = true;  // (a press after the prompt would be taken: none must come)
        r.Wait(2500);
        r.Note();
        int guardGiveUps = 0;
        for (const stance::Event& e : r.events)
            guardGiveUps += e.what == stance::Ev::kNotTaken && e.d == 3;
        Report("15j", "a change under way when a fist fight's prompt opens, read: given up (said), never pressed after the prompt",
               r.poses == "S" && xDuring == 1 && r.g.xPresses == 1 && guardGiveUps == 1 && r.heldUps >= 1,
               "the game %s (wanted S), X by the prompt's end %d, in all %d (wanted 1 and 1), given up for the prompt %d (wanted 1), held up %d",
               r.poses.c_str(), xDuring, r.g.xPresses, guardGiveUps, r.heldUps);
    }
}

// ------------------------------------------------------------- 16 the log's lines -------------------------------------------------------------

// input.cpp's line for each change of the game's pose ("Stance: the game's pose: crouched (was standing)", poseread.h's SaidFirst and SaidNext, fed every
// read), and the line as it was before 2026-11-25: "never reached: the soldier stayed" by the new read's own +0x7C, which a change made at once has set
// to the new pose already.
struct PoseLog {
    poseread::Said said;
    int oldTarget = -1;
    std::vector<std::string> lines, oldLines;
    void Read(int current, int target) {
        char line[160];
        if (said.target < 0) {
            poseread::SaidFirst(said, current, target);
            oldTarget = target;
            return;
        }
        if (const poseread::Change c = poseread::SaidNext(said, current, target); c.say) {
            std::snprintf(line, sizeof(line), "%s (was %s%s%s)", P(c.target), P(c.was), c.neverReached ? ", never reached: the soldier stayed " : "",
                          c.neverReached ? P(c.stayed) : "");
            lines.push_back(line);
        }
        if (target != oldTarget) {
            std::snprintf(line, sizeof(line), "%s (was %s%s%s)", P(target), P(oldTarget), current != oldTarget ? ", never reached: the soldier stayed " : "",
                          current != oldTarget ? P(current) : "");
            oldLines.push_back(line);
            oldTarget = target;
        }
    }
    void Watch(Run& r) {  // every mod frame, as WatchPose: the soldier's pose read from the image
        const poseread::Pose p = poseread::Read(r.m, kBase, r.g.soldier);
        if (p.ok)
            Read(p.current, p.target);
    }
    static std::string Joined(const std::vector<std::string>& v) {
        std::string s;
        for (const std::string& l : v)
            s += (s.empty() ? "" : " | ") + l;
        return s.empty() ? "none" : s;
    }
};

void PoseLines() {
    using stance::kCrouch, stance::kProne, stance::kStand;
    {
        // The proof's (build\logs\bf3vr_1125_p114b_part1.log 16:27:54 to 16:28:01, 16:47:44 and 16:48:44): each change read with +0x7C and +0x80 both set
        // already (made at once, or ended between two reads): reached, every one.
        PoseLog log;
        const int reads[][2] = {{kStand, kStand}, {kStand, kStand}, {kProne, kProne}, {kProne, kProne}, {kCrouch, kCrouch}, {kStand, kStand}, {kStand, kStand},
                                {kProne, kProne}};
        for (const auto& r : reads)
            log.Read(r[0], r[1]);
        const std::string want = "prone (was standing) | crouched (was prone) | standing (was crouched) | prone (was standing)";
        Report("16a", "a change read with the soldier already in it (made at once: a checkpoint's prone soldier; ended between two reads): the one before was "
               "reached, no \"never reached\"", PoseLog::Joined(log.lines) == want, "%s", PoseLog::Joined(log.lines).c_str());
        Report("16b", "... the line as it was said the one before was never reached and the soldier stayed in the new pose (the proof's \"standing (was prone, "
               "never reached: the soldier stayed standing)\")",
               PoseLog::Joined(log.oldLines) != want && log.oldLines.size() == 4 && log.oldLines[3] == "prone (was standing, never reached: the soldier stayed prone)",
               "%s", PoseLog::Joined(log.oldLines).c_str());
    }
    {
        // The game's own pace (its keys at its 30 Hz, +0x80 at the press, +0x7C at the change's end), read every mod frame: four changes, each reached.
        Run r(Way::kRead);
        PoseLog log;
        r.watch = [&log](Run& run) { log.Watch(run); };
        r.FlickDown();
        r.FlickUp();
        r.HoldDown();
        r.FlickUp();
        const std::string want = "crouched (was standing) | standing (was crouched) | prone (was standing) | standing (was prone)";
        Report("16c", "the game's own changes read every frame (the target first, the soldier's pose as the change ends): each reached", PoseLog::Joined(log.lines) == want && r.poses == "CSPS",
               "%s; the game %s (wanted CSPS)", PoseLog::Joined(log.lines).c_str(), r.poses.c_str());
    }
    {
        // Refused at its end (no room: the game puts +0x80 back to +0x7C): up from prone, something low over the soldier as it rises.
        Run r(Way::kRead);
        PoseLog log;
        r.watch = [&log](Run& run) { log.Watch(run); };
        r.HoldDown();
        r.Stick(1.0f, 150);
        r.Wait(200);
        r.g.room = false;  // (under something low now: the change to standing refused at its end)
        r.Wait(2500);
        const bool refused = !log.lines.empty() && log.lines.back() == "prone (was standing, never reached: the soldier stayed prone)";
        Report("16d", "a change refused at its end (no room to stand): the line says that target was never reached, the soldier staying prone", refused && log.lines.size() >= 3 &&
               log.lines[0] == "prone (was standing)" && log.lines[1] == "standing (was prone)", "%s", PoseLog::Joined(log.lines).c_str());
    }
    {
        // A change during a change (the data's AllowPoseChangeDuringTransition 1): held down, then up before the change to prone ends: prone never reached.
        Run r(Way::kRead, true);
        PoseLog log;
        r.watch = [&log](Run& run) { log.Watch(run); };
        r.Stick(-1.0f, 700);
        r.Stick(1.0f, 150);
        r.Wait(2500);
        const std::string want = "prone (was standing) | standing (was prone, never reached: the soldier stayed standing)";
        Report("16e", "another change asked for before the last ended: that target never reached, the soldier staying where it was", PoseLog::Joined(log.lines) == want, "%s",
               PoseLog::Joined(log.lines).c_str());
    }
}

}  // namespace

int main() {
    const auto start = std::chrono::steady_clock::now();
    OwnersSequence();
    DeathWhileProne();
    CheckpointProne();
    ProneFromCrouch();
    GameStandUps();
    ChangeDuringChange();
    ScriptedLocks();
    NoRoom();
    HeadCrouch();
    GuardAndCutscene();
    Rules();
    Reader();
    Proofs();
    ReviewCases();
    PoseLines();
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    Report("14", "the time", ms < 20000.0, "%.0f ms", ms);
    std::printf("%d checks, %d failed, %d skipped\n", g_counted, g_failed, g_skipped);
    return g_failed;
}

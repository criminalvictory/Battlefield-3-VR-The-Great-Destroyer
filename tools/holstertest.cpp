// Holsters (src/holster.h, header only, as hands.cpp runs it) without the game: scripted frames at 72 Hz with a stand-in for the grasp (grasp.h's
// let-go, its kStowed and its draw as the holster design gives them; grasptest's own rows prove the real one), and the body as the pouch places it
// (src/pouch.cpp, linked) for the head's turns.
//   Z1-Z6   the places: no point of a 1 cm and a 5 mm grid in two (the shoulders as judged, the head cut out; where a shoulder meets the neck the
//           nearer has it), none in one and the belly grown by 0.05, every bottom above the magnet column's gate, the smaller Size where two meet,
//           none of the user's own squeezes in play inside one, mirror 1
//   M1-M12  a slot, its kind and whether the hands know it to a place: the levels' kits, two launchers, "launcher off"
//   S1-S10  a put-away by a seen hand: its own place, another's, fast at a place (never a throw there) and a throw just outside one, the other
//           hand's hold, the handguard alone, the moments, past the grace, 2 cm out, the grace, what the other hand holds (a rocket, a case, a 40
//           mm round, its magazine), the sound's side, "vanish"
//   D1-D22  a draw by a seen hand: case A by either hand, case B and its key, the grip let go first, no switch, the other hand holding the gun, the
//           gun lying (and a take first, by either hand), a gun the hands don't know, a grip held through, the moments, a busy hand, the picks, a
//           switch nobody asked for, a new life, a quick grab going back and the reach over the shoulder to the neck, refusals felt only with both
//           hands empty, nothing there, its own
//           let-go just now, a pick while a draw waits, the waiting draw's squeeze kept from the grasp, the game's own switch while a draw waits
//   K1-K5   the second key: its rule row by row, harm at 0.6 s, a switch after the second key; the game's slot unread a moment or longer
//   U1-U25  a hand not seen: the design's rows, U12 and U14 with the grip opening at 0.3 s too (within input's held place), U22 the reach over the
//           shoulder to the neck (from its very middle too), the stand-in's hand, a refused opening said once, a grip held through free hands asleep
//   B1-B3   small head turns leave the places, the places follow the body's turn, no head tracking: no draw, no put-away
//   W1-W3   a draw then a fast let-go: at the place, put away; whipped out of it, a throw (the grace below the throw's speed)
//   R1-R4   a draw or a put-away asked of the grasp again till it's done (it lapses after 0.25 s); the gun drawn nowhere while its hand isn't seen
//           at a place, and while a draw waits; a hidden take's wait never outlives its window
//   T1-T4   the tick on entering a place where the hand can act
//   P1-P7   holsters.txt: holsters 0 (and with a gun put away), no file, each line, bad lines, unseen 0, launcher off, the line endings
//   L1      the lines' words
//   N1-N2   broken inputs; the time
//   HB1-HB19 holsters by hand only (the user, 2026-11-18): the kit's at a new life; a take off the ground (raised by the game or not) never at a
//           shoulder, nor the gun that lay; a picked-up gun put on the left or the right shoulder by the hand: held there, drawn from there (its key
//           its slot's); taken out and dropped: no holster; the game's swap of a held gun (to no shoulder, or back where it came from); the game's
//           own switch to a gun with none, and one put away with none: into the right hand; the launchers' rule; a take that would put a holstered
//           gun into its pickup found, its double click; a gun by its identity; a take's mark either side of its gun; anyholster 0; the tick at
//           either shoulder; othergun and lying read, answering nothing; a new life; the lines; the grenade launcher's shot and a grenade say nothing
//   HA1-HA18 a holstered gun the game takes out of its slot stays on its shoulder (the user's AT4, 2026-11-18), against the test's game (a take puts
//           the gun of the slot its entry names into the pickup with its rounds, its own into that slot): the user's flow (a), a pickup's AT4 taken
//           with the rifle holstered, fired, thrown, the rifle drawn back from the shoulder by the game's take of its pickup with its own rounds, put
//           back; flow (b), the rifle dropped, the AT4 taken, thrown, the rifle picked up again off the floor with its rounds; Swordbreaker's roof AT4
//           by its kit (the M4A1 with the game, back at its shoulder with the level's kit); the review's RB (a new life keeps a picked-up gun on no
//           shoulder; another level's kit by its slots), RC and an away gun (one gun a shoulder); a draw that can't come (far, refused, never
//           answered, the other hand holding the gun); a pickup gone or holding another gun; TakeCheck's verdicts and Rock's Stinger; one pickup, two
//           guns of a slot back and forth; a picked-up gun let go of going into its slot for a draw; Reset; a swap told a frame late; weak records and
//           taken shoulders; the lines (by the scene's names); HA7/HA8/HA11b/HA12 as the review of 907b2b6 was settled (below)
//   HS1-HS5 the review of 907b2b6 settled (2026-11-18): kept in the hand at a refusing shoulder (the grace, a throw, carried away open, let go after);
//           the level's kit ends a record in a pickup; the memo by level (a death, a life between with nothing, a kit gun never carried a frame late, a
//           take in the kit's seconds); a draw whose hand opened before the game's take landed (at its place); the AT4 branch's merge (the roof's AT4
//           by a use, marked a take's: holstered, and flow (b); HS5c with the M4A1 in the other hand: a take's mark with no swap is no pickup's)
//   X1-X8b  the review's own scenarios (its revtest.cpp): a picked-up gun and the next level's kit (twice); the AT4 on the launcher's place before
//           it's thrown; a same-kind swap of a holstered gun; the roof's AT4 with the M4A1 held; a take told a frame late in the kit's seconds; the
//           D-pad with a gun away (info); a new life with the world kept (info); the bridge's M249 with the M4A1 holstered, and on to the roof
//   V1-V15b the first review of ef2cbb6's scenarios (its rvtest.cpp): the roof's M4A1 with the game through a death (V1; V1b its record's ends); two
//           floor guns in a row; two guns sharing slot 0; the away rifle taken back off the floor by X held, its swap in time and a frame late (V4,
//           V4b: into the hand); Rock's Stinger with a picked-up gun on the left shoulder; the bridge's M249 on the right then the roof (info); the
//           SMAW at the left shoulder with a far AT4 away there (V7), and a near one (V7b); a take of the gun in the hand in time and late (V8); a level
//           told by its kit alone (info); a level change mid-flow; a death mid-flow; kept in the hand twice; the weapon buttons mid-flow; a picked-up
//           gun through a death; one gun a shoulder through a death (V15, V15b)
//   Y9      the second review's R9: the draw back's take told a frame after its gun is seen in its slot
//   HT1-HT8 the AT4 branch merged (2026-11-19), the owner's two AT4 flows end to end with both together: its use of the roof's prompt by reach and grip
//           (src/usepoint.h) on holsters' frames, its gift as hands.cpp gives it (the slot marked a take's, the take's Arrive, into the hand that used
//           it), the scene's kit and its give-back: (a) holstered, thrown, drawn early (not back yet), at its place, drawn by hand; the AT4 still in the
//           hand at the give-back (at its place all the same; a pick: today's); the M4A1 held in the other hand at the tube: refused (the owner's
//           decision: "Rifle dropped on the floor first"), the use once it's put on a shoulder or let go of; (b) dropped, its lying gun recorded
//           (src/pickrules.h's LyingTaken), the kit's M4A1 laid again where it lay (LyingBack), the left's Mk11 refused while it lies (HT4c the
//           control: holsters' lying gun read afresh), taken off the floor, holstered by hand; through a death at the "RPG" checkpoint (HT7); X held's
//           R at the prompt for both; nothing into a hand between a use and its gift (HT8); hands.cpp's lines word for word
//   TN1-TN8 the owner's night of 2026-11-19 and the shoulders made a lot bigger for it ("just swing it to my shoulders and easily let go"): the
//           night's 17 squeezes at the shoulders on their own paths (7 missed then: all drawn now), its falls by a shoulder as near as the log
//           places them, 135 fast swings to each shoulder (its own hand and across), the swing's end come early, the guards (both hands on the
//           gun, aiming, held out, the headset, the back of the head, the throat, the chests, the belly), the sizes holsters.txt's, the places
//           apart, the new lines; with main's AT4 merged, a fired AT4 thrown away at the shoulder its firing hand sits on (TN8)
//   TN9-TN14 the two reviews of c6dabab, settled: both hands on the gun, the grip let go of first at the shoulder (the handoff); by the handguard
//           alone a gun goes to its own shoulder; a toss is no swing, nor a swing onto the other shoulder; a gun let go of beside the head is put
//           away (the head's cut for an empty hand only); a hand not seen presumed at a shoulder only within the old egg; the fired AT4 spent while
//           drawn, through the kit's give-back
//   H1-H9   the holsters' own heading (FollowHeading): a seated look moves nothing (a long one too, the hands resting in front), a hand at a holster
//           holds the places, the whole body's turn is followed (while aiming too: the aiming hand and the one on its front hold nothing), turning
//           round, a recentre, a small drift; hands that say nothing
//   K..     every forced change of gun and every kit a script gives in the six missions left (docs\research\missions\, PLAN.md A13), as their surveys
//           read the levels, with the guns the hands know today and (k) every gun held: Uprising's knife alone and the stab's AKS-74u, the Parking
//           Lot's load (KU1-KU2); Guillotine's mortar and the SP_Bank kit, the bridge's M4A1 under the fade (held, put away, hanging, after a floor
//           gun), a death after, the bathroom's knife, the Javelin and the NoJavelin kit (KG1-KG7); Comrades' wire cut (KC1); Night Shift's knife,
//           the game's own SelectWeapon2 and slot 1, the claymore kit (KN1-KN3); Kaffarov's drogue kit, the SUV's MP-443, the pool house's M82A3
//           and the AEK-971's pickup, weapon change locked, the pool and Blackburn (KK1-KK5); The Great Destroyer's spawn kit, the jammed door, the
//           kick's two AKS-74u, a death after, the last door, the sewer, the climb and the cuffs (KD1-KD5); a take told a frame late, a kit's gun for
//           that frame (KX1); the lines (KL1). Whatever the scene: one gun a shoulder, gear never at a holster, no record of a gun while one of its kind
//           is in its slot, none a kit replaced past its window. Since the lane's settle (2026-11-25) these run "kitguns holster" (a script's kit's gun
//           the kit's holster: an opt-in), and an away gun in its pickup keeps its shoulder from a kit's gun (KG4k (e))
//   KO1-KO5 the same scenes with the default, "kitguns none" (as the owner played Semper Fidelis, The Great Destroyer and Swordbreaker): a script's
//           kit's gun on no shoulder till placed, the gun it replaced gone with the kit all the same (no ghost "with the game" at its shoulder)
//   KR1-KR3 the lane's review settled, each setting: the memo a late take's mark puts back (#1), a picked-up gun through a death's spawn-kit frames
//           (#2), a floor gun at a shoulder the roof's AT4 kit took, gone with the start kit's give-back (#3)
//   KR4     the recheck's F1, each setting: a kit of gear alone (Guillotine's mortar in slot 0) is a level's kit still, so a holster word of the level
//           before never carries in (an 870 put on the left shoulder in Uprising, the SP_Bank kit's 870 after a death in Guillotine)
// Usage: holstertest
// The exit code is the number of failed checks.

#include <windows.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "../src/holster.h"
#include "../src/pickrules.h"
#include "../src/pouch.h"
#include "../src/usepoint.h"

using namespace holster;
// The checks below were written for launchers behind the neck, the default until 2026-11-15: they pin it. The shoulder default's own checks
// (L1-L4, the user: "primaries go on the right and secondaries go on the left") use the default.
Settings NeckSet() {
    Settings s;
    s.launcher = Launcher::kNeck;
    return s;
}

namespace {

int g_failed = 0, g_counted = 0;

void Report(const char* id, const char* what, bool pass, const char* format, ...) {
    char numbers[1536];
    va_list args;
    va_start(args, format);
    std::vsnprintf(numbers, sizeof(numbers), format, args);
    va_end(args);
    std::printf("%-5s %-5s %s | %s\n", id, pass ? "PASS" : "FAIL", what, numbers);
    ++g_counted;
    g_failed += !pass;
}

constexpr float kDt = 1.0f / 72.0f;
constexpr float kPi_ = 3.14159265f;
constexpr float kNaN = std::numeric_limits<float>::quiet_NaN();
constexpr float kInf = std::numeric_limits<float>::infinity();

// Places' middles and other points from the neck point (right, up, forward; metres).
constexpr float kRS[3] = {0.20f, -0.025f, -0.03f};  // the places' middles, as holster.h's defaults (the reach test's)
constexpr float kLS[3] = {-0.20f, -0.025f, -0.03f};
constexpr float kNeckAt[3] = {0.0f, 0.07f, -0.155f};
constexpr float kRibAt[3] = {-0.19f, -0.30f, -0.03f};
constexpr float kFront[3] = {0.20f, -0.30f, 0.40f};  // the hands' rest in front: no place
constexpr float kBellyAt[3] = {0.0f, -0.43f, 0.13f};
constexpr float kStill[3] = {0.0f, 0.0f, 0.0f};

const char* const kNames[kSlots] = {"M416", "Stinger", "M320", nullptr, nullptr, "SMAW", "M67", "Knife", nullptr, nullptr};

struct Script {  // one hand as the test drives it
    float pos[3] = {kFront[0], kFront[1], kFront[2]};  // from the neck point, body axes
    float vel[3] = {};                                 // body axes, m/s (its controller's own)
    bool velKnown = true;
    Seen seen = Seen::kSeen;
    float guess[3] = {};  // the runtime's guess (body axes) while kGuessed
    float squeeze = 0.0f;
    bool own = true, read = true, magazine = false, round40 = false, rocket = false, onGun = false, aims = false, busy = false;
};

struct Buzzed {
    float at;
    int side;
    float amp, seconds;
};

struct Sim {
    Settings set = NeckSet();
    State st;
    Script hand[2];
    // The test's grasp.
    Holder holder = Holder::kHeld;
    int trigger = 1, barrel = -1;
    bool armed[2] = {};
    int low[2] = {};
    float lostFor[2] = {};  // input's view: s with no place at all (its 0.5 s held place)
    int takeBy = -1;        // the grasp takes the lying gun into that hand this frame (its squeeze spent)
    int deaf = 0;           // frames the grasp ignores holsters' stowSide and drawSide (as frozen, or a hand it doesn't count as tracked)
    int unseenAtPlaceFrames = 0, drawWaitsFrames = 0;
    // The game.
    int gameSlot = 0;
    bool slotRead = true;
    SlotIn slots[kSlots];
    bool on = true, frozen = false, gameReload = false, m320 = false, body = true, pendingTake = false, anyTake = false;
    float neck[3] = {0.0f, 1.2f, 0.0f};
    float yaw = 0.0f;
    bool picked = false;
    int pickSlot = -1;
    unsigned taken = 0;      // takes off the ground since the last frame (Frame::taken), cleared after each frame
    bool swapsSeen = true;   // Frame::swapsSeen
    bool drawnOther = false; // Frame::drawnOther: a change of gun under way (the game's slot holds another gun than the one drawn)
    bool spentTube = false;  // Frame::spentTube: the gun in the hands a fired AT4
    // The away guns' pickups as the census has them (Frame::awayFound, awayReach, awayDistance), and the test's game (HA's World): before each frame
    // (its takes, what the pickups hold), and after Update (hands.cpp's ask of a draw's give-back).
    int awayFound[kMostAway] = {-2, -2, -2, -2, -2, -2};
    bool awayReach[kMostAway] = {};
    float awayDistance[kMostAway] = {};
    void (*before)(Sim&) = nullptr;
    void (*after)(Sim&) = nullptr;
    void* world = nullptr;
    // What happened.
    float time = 0.0f;
    Result r;
    After a;
    std::vector<Event> events;
    std::vector<std::pair<float, int>> keys;  // (time, slot)
    std::vector<Buzzed> buzzes;
    std::vector<std::pair<float, Sound>> sounds;
    float lastPan = 0.0f;
    int stows = 0, draws = 0, falls = 0;
    int stowedBy = -1, drawnBy = -1, fellBy = -1;
    float stowedAt = -1.0f, drawnAt = -1.0f, fellAt = -1.0f;
    bool everHold[2] = {}, everStows[2] = {};
    int busyFrames[2] = {};
    int stowSides = 0, drawSides = 0;
    int shownFrames = 0;

    void ToRoom(const float b[3], float out[3]) const {
        const float c = std::cos(yaw), s = std::sin(yaw);
        const float right[3] = {c, 0.0f, -s}, forward[3] = {-s, 0.0f, -c};
        for (int k = 0; k < 3; ++k)
            out[k] = neck[k] + right[k] * b[0] + (k == 1 ? b[1] : 0.0f) + forward[k] * b[2];
    }
    void VelToRoom(const float b[3], float out[3]) const {
        const float c = std::cos(yaw), s = std::sin(yaw);
        const float right[3] = {c, 0.0f, -s}, forward[3] = {-s, 0.0f, -c};
        for (int k = 0; k < 3; ++k)
            out[k] = right[k] * b[0] + (k == 1 ? b[1] : 0.0f) + forward[k] * b[2];
    }
    Frame Build(float dt) const {
        Frame f;
        f.dt = dt, f.on = on, f.frozen = frozen, f.gameReload = gameReload, f.m320 = m320, f.body = body;
        std::memcpy(f.neck, neck, sizeof(f.neck));
        f.yaw = yaw;
        for (int i = 0; i < 2; ++i) {
            const Script& h = hand[i];
            HandIn& in = f.hand[i];
            in.seen = h.seen;
            if (h.seen == Seen::kSeen)
                ToRoom(h.pos, in.at);
            else if (h.seen == Seen::kGuessed)
                ToRoom(h.guess, in.at);
            in.velocityKnown = h.velKnown && h.seen == Seen::kSeen;
            if (in.velocityKnown)
                VelToRoom(h.vel, in.velocity);
            in.own = h.own, in.read = h.read, in.squeeze = h.squeeze, in.armed = armed[i];
            in.busy = h.busy || h.magazine || h.round40, in.magazine = h.magazine, in.round40 = h.round40, in.rocket = h.rocket;
            in.onGun = h.onGun, in.aims = h.aims;
        }
        f.holder = holder, f.trigger = trigger, f.barrel = barrel, f.pendingTake = pendingTake;
        f.anyTake = anyTake || holder == Holder::kLoose;
        f.gameSlot = slotRead ? gameSlot : -1;
        for (int i = 0; i < kSlots; ++i)
            f.slots[i] = slots[i];
        f.picked = picked, f.pickSlot = pickSlot;
        f.taken = taken, f.swapsSeen = swapsSeen, f.drawnOther = drawnOther, f.spentTube = spentTube;
        for (int a = 0; a < kMostAway; ++a)
            f.awayFound[a] = awayFound[a], f.awayReach[a] = awayReach[a], f.awayDistance[a] = awayDistance[a];
        return f;
    }
    // grasp.h's let-go and holds as the holster design has them: stows sends a let-go to kStowed unless the other hand holds the handguard; stowSide
    // puts it away now; drawSide draws it from kStowed into that hand, armed; hold tells it the hand isn't tracked (no let-go, disarmed).
    void Grasp(const Frame& f) {
        a = After{};
        bool tracked[2];
        for (int i = 0; i < 2; ++i) {
            lostFor[i] = hand[i].seen == Seen::kNone ? lostFor[i] + f.dt : 0.0f;
            tracked[i] = !r.hold[i] && (hand[i].seen != Seen::kNone || lostFor[i] < 0.5f);
        }
        const auto live = [&](int i) { return hand[i].own && hand[i].read && tracked[i]; };
        const bool hears = deaf <= 0;
        if (deaf > 0)
            --deaf;
        if (hears && r.stowSide >= 0 && ((holder == Holder::kHeld && trigger == r.stowSide) || (holder == Holder::kBarrel && barrel == r.stowSide))) {
            a.stowed = a.letGo = r.stowSide;
            holder = Holder::kStowed, trigger = barrel = -1;
            armed[0] = armed[1] = false, low[0] = low[1] = 0;
            ++stows, stowedBy = a.stowed, stowedAt = time;
            return;
        }
        if (hears && r.drawSide >= 0 && holder == Holder::kStowed && !frozen) {
            const int t = r.drawSide;
            holder = Holder::kHeld, trigger = t, barrel = -1;
            armed[t] = r.drawArmed, low[t] = 0;
            a.drawn = t, a.spent[t] = true;
            ++draws, drawnBy = t, drawnAt = time;
            return;
        }
        if (holder == Holder::kHeld && (trigger == 0 || trigger == 1)) {
            const int t = trigger;
            if (!live(t) || gameReload || frozen) {
                armed[t] = false, low[t] = 0;
            } else {
                if (hand[t].squeeze >= kTake)
                    armed[t] = true;
                low[t] = armed[t] && hand[t].squeeze < kLetGo ? std::min(low[t] + 1, 3) : 0;
            }
            if (!frozen && low[t] >= 3) {
                a.letGo = t;
                armed[t] = false, low[t] = 0;
                const int o = 1 - t;
                const bool support = hand[o].onGun && tracked[o];
                if (r.stows[t] && !support) {
                    holder = Holder::kStowed, trigger = -1, a.stowed = t;
                    ++stows, stowedBy = t, stowedAt = time;
                } else if (support) {
                    holder = Holder::kBarrel, barrel = o, trigger = -1;
                } else {
                    holder = Holder::kLoose, trigger = -1;
                    ++falls, fellBy = t, fellAt = time;
                }
            }
        } else if (holder == Holder::kBarrel && (barrel == 0 || barrel == 1)) {
            const int b = barrel;
            if (!frozen && !hand[b].onGun && !r.hold[b]) {  // (hold: hands.cpp keeps that hand on the front, as the grasp counts it)
                a.letGo = b;
                if (r.stows[b]) {
                    holder = Holder::kStowed, barrel = -1, a.stowed = b;
                    ++stows, stowedBy = b, stowedAt = time;
                } else {
                    holder = Holder::kLoose, barrel = -1;
                    ++falls, fellBy = b, fellAt = time;
                }
            }
        } else if (holder == Holder::kLoose && takeBy >= 0 && !frozen) {
            holder = Holder::kHeld, trigger = takeBy, barrel = -1;
            armed[takeBy] = true;
            a.spent[takeBy] = true;
        }
    }
    void Step(float dt = kDt) {
        if (before)
            before(*this);
        const Frame f = Build(dt);
        Update(set, f, st, r);
        if (after)
            after(*this);
        for (int i = 0; i < 2; ++i) {
            everHold[i] = everHold[i] || r.hold[i];
            everStows[i] = everStows[i] || r.stows[i];
            busyFrames[i] += r.busy[i];
        }
        stowSides += r.stowSide >= 0;
        drawSides += r.drawSide >= 0;
        unseenAtPlaceFrames += r.unseenAtPlace;
        drawWaitsFrames += r.drawWaits;
        if (r.key >= 0)
            keys.push_back({time, r.key});
        Grasp(f);
        Settle(set, f, a, st, r);
        for (int k = 0; k < r.events; ++k)
            events.push_back(r.event[k]);
        for (int i = 0; i < 2; ++i)
            if (r.buzz[i].seconds > 0.0f)
                buzzes.push_back({time, i, r.buzz[i].amplitude, r.buzz[i].seconds});
        if (r.sound != Sound::kNone)
            sounds.push_back({time, r.sound}), lastPan = r.pan;
        shownFrames += r.stillShown[0] || r.stillShown[1];
        time += dt;
        picked = false, pickSlot = -1, takeBy = -1, taken = 0;
    }
    void Run(float seconds) {
        const int n = static_cast<int>(std::lround(seconds / kDt));
        for (int k = 0; k < n; ++k)
            Step();
    }
    void Move(int i, const float p[3], float vx = 0.0f, float vy = 0.0f, float vz = 0.0f) {
        std::memcpy(hand[i].pos, p, sizeof(hand[i].pos));
        hand[i].vel[0] = vx, hand[i].vel[1] = vy, hand[i].vel[2] = vz;
    }
    void MoveTo(int i, float x, float y, float z, float vx = 0.0f, float vy = 0.0f, float vz = 0.0f) {
        const float p[3] = {x, y, z};
        Move(i, p, vx, vy, vz);
    }
    // The game's new weapon drawn (OnGunChanged): Arrive, and the grasp as hands.cpp leaves it.
    ArriveOut Switch(int slot, bool take = false, bool newLife = false) {
        ArriveIn in;
        in.stowed = holder == Holder::kStowed, in.take = take, in.newLife = newLife, in.lying = holder == Holder::kLoose;
        in.slot = slotRead ? slot : -1;
        for (int i = 0; i < 2; ++i)
            in.placed[i] = hand[i].seen != Seen::kNone;
        in.haveSlots = true, in.taken = taken, in.swapsSeen = swapsSeen;  // (hands.cpp's HolsterArriveSlots: the slots as now, the takes not yet told)
        for (int i = 0; i < kSlots; ++i)
            in.slots[i] = slots[i];
        gameSlot = slot;
        const ArriveOut o = Arrive(set, in, st);
        if (o.arrival == Arrival::kHand)
            holder = Holder::kHeld, trigger = o.side, barrel = -1, armed[0] = armed[1] = false;
        else if (o.arrival == Arrival::kAtPlace)
            holder = Holder::kStowed, trigger = barrel = -1;
        else
            holder = Holder::kHeld, trigger = 1, barrel = -1, armed[0] = armed[1] = false;  // (today's: the right hand)
        if (o.event.what != What::kNone)
            events.push_back(o.event);
        return o;
    }
    int Count(What w, int side = -2) const {
        int n = 0;
        for (const Event& e : events)
            n += e.what == w && (side == -2 || e.side == side);
        return n;
    }
    int Said() const {  // lines other than the ticks' and "log 1"'s
        int n = 0;
        for (const Event& e : events)
            n += e.what != What::kTick && e.what != What::kLook;
        return n;
    }
    const Event* Find(What w, int side = -2) const {
        for (const Event& e : events)
            if (e.what == w && (side == -2 || e.side == side))
                return &e;
        return nullptr;
    }
    int Pulses(int side, float amp = -1.0f, float after = -1.0f) const {
        int n = 0;
        for (const Buzzed& b : buzzes)
            n += b.side == side && (amp < 0.0f || std::fabs(b.amp - amp) < 1e-4f) && b.at >= after;
        return n;
    }
    int Keys(int slot = -1) const {
        int n = 0;
        for (const auto& k : keys)
            n += slot < 0 || k.second == slot;
        return n;
    }
};

// A failed row's events, said as hands.cpp would say them.
void Dump(const Sim& s, bool failed) {
    if (!failed)
        return;
    for (const Event& e : s.events) {
        char line[320];
        Say(e, kNames, line, sizeof(line));
        std::printf("        %s%s\n", line, e.quiet ? " (quiet)" : "");
    }
}

// Rock and a Hard Place's kit: the M416 (slot 0) with its M320 (slot 2), the SMAW (5), the M67 (6), the knife (7).
void Rock(Sim& s) {
    for (SlotIn& v : s.slots)
        v = SlotIn{};
    s.slots[0] = {true, false, true};
    s.slots[2] = {true, false, true};
    s.slots[5] = {true, true, true};
    s.slots[6] = {true, false, false};
    s.slots[7] = {true, false, false};
}
// Operation Swordbreaker's: the M4A1 (0), the Mk11 (1, not known to the hands), the M67 (6).
void Swordbreaker(Sim& s) {
    for (SlotIn& v : s.slots)
        v = SlotIn{};
    s.slots[0] = {true, false, true};
    s.slots[1] = {true, false, false};
    s.slots[6] = {true, false, false};
}
// The M416 put away, both hands empty at rest.
Sim StowedRock() {
    Sim s;
    Rock(s);
    s.holder = Holder::kStowed, s.trigger = -1;
    s.Run(0.1f);
    return s;
}
// The M416 in the right hand, armed and squeezed.
Sim HeldRock() {
    Sim s;
    Rock(s);
    s.holder = Holder::kHeld, s.trigger = 1, s.armed[1] = true;
    s.hand[1].squeeze = 1.0f;
    s.Run(0.1f);
    return s;
}

// ---- Z: the places ----

void Places() {
    const Settings s = NeckSet();
    const Place eggs[7] = {Place::kRightShoulder, Place::kLeftShoulder, Place::kNeck, Place::kLeftRib, Place::kRightRib, Place::kLeftChest,
                           Place::kRightChest};
    for (int pass = 0; pass < 2; ++pass) {
        // The shoulders as they're judged (SizeFor: the head's egg cut out of them, nothing inside the chests' or the belly's); the other eggs whole.
        // Since 2026-11-19 the shoulders' big eggs may meet the neck's at their back corners: there the nearer by its Size has the point (PlaceAt).
        const int step = pass == 0 ? 10 : 5;  // millimetres
        long long points = 0, inTwo = 0, inBelly = 0, shoulderNeck = 0, wrongNearer = 0, shoulderInHead = 0, shoulderPoints = 0;
        for (int x = -400; x <= 400; x += step)
            for (int y = -700; y <= 200; y += step)
                for (int z = -350; z <= 450; z += step) {
                    const float p[3] = {x / 1000.0f, y / 1000.0f, z / 1000.0f};
                    int in = 0;
                    bool shoulder = false, neck = false;
                    for (const Place q : eggs) {
                        const bool inside = (IsShoulder(q) ? SizeFor(q, p, s) : Size(EggOf(q, s), p)) <= 1.0f;
                        in += inside;
                        shoulder = shoulder || (inside && IsShoulder(q));
                        neck = neck || (inside && q == Place::kNeck);
                    }
                    ++points;
                    shoulderPoints += shoulder;
                    shoulderInHead += shoulder && InHead(p, s);
                    if (shoulder && neck && in == 2) {
                        // (Where a shoulder and the neck meet: the place judged there is the one with the smaller Size.)
                        ++shoulderNeck;
                        const float zs = std::min(SizeFor(Place::kRightShoulder, p, s), SizeFor(Place::kLeftShoulder, p, s)),
                                    zn = SizeFor(Place::kNeck, p, s);
                        const Place got = PlaceAt(p, s);
                        wrongNearer += got != (zs < zn ? (p[0] > 0.0f ? Place::kRightShoulder : Place::kLeftShoulder) : Place::kNeck);
                    } else {
                        inTwo += in >= 2;
                    }
                    inBelly += in >= 1 && ammozone::Inside(p, ammozone::kPutBackMore);
                }
        Report(pass == 0 ? "Z1" : "Z1b",
               pass == 0 ? "no point of a 1 cm grid in two places (the shoulders as judged), but where a shoulder meets the neck: the nearer has it"
                         : "no point of a 5 mm grid in two places (the shoulders as judged), but where a shoulder meets the neck: the nearer has it",
               inTwo == 0 && wrongNearer == 0 && shoulderInHead == 0 && shoulderPoints > 0,
               "%lld points from -0.40 to 0.40 right, -0.70 to 0.20 up, -0.35 to 0.45 forward; %lld in two of the shoulders, the neck, the ribs and the "
               "chests otherwise; %lld where a shoulder meets the neck, %lld of them not the nearer's; %lld shoulder points, %lld in the head", points, inTwo,
               shoulderNeck, wrongNearer, shoulderPoints, shoulderInHead);
        Report(pass == 0 ? "Z2" : "Z2b", "none in a place and in the belly grown by 0.05 (where a let-go puts a magazine back)", inBelly == 0, "%lld",
               inBelly);
    }
    float lowest = 1.0f;
    Place low = Place::kNone;
    for (const Place q : eggs) {
        const Egg e = EggOf(q, s);
        if (e.mid[1] - e.half[1] < lowest)
            lowest = e.mid[1] - e.half[1], low = q;
    }
    Report("Z3", "every place's bottom above the magnet column's gate (0.50 m under the neck point): the column never meets a place",
           lowest > -0.50f, "the lowest: %s at %.2f (the sphere has no gate: a take goes before a place, D7)", Name(low), lowest);
    {
        // Two places that meet (holsters.txt can make them): the smaller Size wins; the defaults grown by the approach margin too.
        Settings big = NeckSet();
        big.shoulder = Egg{{0.10f, 0.02f, -0.08f}, {0.10f, 0.10f, 0.10f}};
        const float p[3] = {0.06f, 0.04f, -0.12f};
        const float zs = Size(EggOf(Place::kRightShoulder, big), p), zn = Size(EggOf(Place::kNeck, big), p);
        float z = 0.0f;
        const Place got = PlaceAt(p, big, &z);
        const Place want = zs < zn ? Place::kRightShoulder : Place::kNeck;
        const float q[3] = {0.11f, -0.07f, -0.035f};
        const float gs = Size(EggOf(Place::kRightShoulder, s), q, 0.15f), gn = Size(EggOf(Place::kNeck, s), q, 0.15f);
        float zg = 0.0f;
        const Place grown = PlaceAt(q, s, &zg, 0.15f);
        Report("Z4", "the smaller Size wins where two places meet", got == want && zs <= 1.0f && zn <= 1.0f && z == std::min(zs, zn) &&
                                                                       gs <= 1.0f && gn <= 1.0f && grown == (gs < gn ? Place::kRightShoulder : Place::kNeck),
               "a shoulder grown over the neck: %.2f and %.2f, %s; the defaults grown 0.15: %.2f and %.2f, %s", zs, zn, Name(got), gs, gn, Name(grown));
    }
    {
        // The user's own squeezes in play (the pouch's, the grenade launcher's and the launcher's lines in six logs of 2026-11-12 and 13): their
        // gun work in front and their belly; none may fall in a place.
        static const signed char kUser[][3] = {
            {-31, -30, 34}, {-8, -26, 37}, {-27, -21, 28}, {-23, -21, 27}, {-11, -48, 8}, {-5, -33, 21}, {-33, -26, 31}, {-35, -24, 33}, {-17, -9, 32},
            {-18, -45, 13}, {-13, -49, 10}, {-26, -9, 39}, {3, -16, 38}, {-4, -47, 18}, {-9, -14, 44}, {3, -48, 12}, {0, -17, 42}, {2, -18, 36},
            {-5, -45, 16}, {-12, -14, 44}, {-16, -22, 45}, {-42, -46, 16}, {-11, -17, 27}, {-10, -34, 20}, {-4, -29, 18}, {-18, -18, 17}, {-21, -44, 20},
            {-32, -44, 22}, {-8, -20, 33}, {-31, -21, 36}, {0, -44, 20}, {-4, -6, 45}, {2, -46, 18}, {-3, -11, 41}, {-3, -29, 36}, {-5, -25, 33},
            {14, -6, 36}, {14, -3, 34}, {-4, -45, 19}, {-6, -47, 20}, {-8, -18, 50}, {13, -8, 33}, {14, -7, 31}, {-9, -46, 19}, {-11, -47, 18},
            {-19, -16, 49}, {20, -7, 30}, {21, -6, 28}, {-12, -46, 23}, {-10, -44, 23}, {-9, -47, 19}, {-12, -48, 19}, {-17, -20, 45}, {22, -5, 29},
            {22, -4, 27}, {-22, -47, 8}, {-21, -46, 10}, {-20, -45, 6}, {-19, -44, 8}, {-14, -48, 10}, {-16, -49, 10}, {-26, -16, 42}, {15, -6, 30},
            {16, -5, 28}, {-20, -49, 10}, {-18, -44, 13}, {-15, -47, 11}, {-17, -48, 10}, {-15, -48, 12}, {-17, -49, 12}, {-31, -23, 37}, {-21, -35, 31},
            {-21, -29, 39}, {-6, -47, 17}, {-13, -8, 42}, {-9, -47, 16}, {-24, -20, 42}, {-17, -26, 39}, {-7, -50, 18}, {-14, -30, 43}, {-20, -37, 36},
            {-4, -23, 92}, {2, -23, 92}, {7, -24, 59}, {0, -43, 13}, {3, 1, 46}, {-27, -26, 30}, {4, -51, 22}, {-12, -31, 34}, {1, -53, 19}, {-16, -40, 34},
            {-3, -53, 21}, {-33, 7, 19}, {-9, -51, 21}, {-10, -52, 20}, {-8, -54, 19}, {-11, -55, 19}, {-6, -53, 18}, {-11, -51, 18}, {-9, -51, 20},
            {-6, -20, 32}, {-10, -15, 34}, {-11, -15, 31}, {-30, -23, 36}, {-29, -23, 37}, {-2, -51, 22}, {-17, -4, 26}, {-4, -53, 18}, {-17, -30, 33},
            {-19, -23, 42}, {-3, -52, 20}, {-4, -22, 34}, {1, -26, 30}, {1, -26, 31}, {-27, -28, 34}, {-26, -28, 34}, {-14, 2, 29}, {3, -55, 22},
            {-4, -52, 16}, {-1, -32, 22}, {-26, -17, 35}, {-4, -55, 23}, {-4, -55, 18}, {-21, -22, 43}, {2, -52, 21}, {-14, -25, 42}, {-13, -25, 42},
            {2, -51, 22}, {-9, -24, 40}, {5, -53, 21}, {-4, -22, 42}, {-3, -52, 23}, {3, -56, 16}, {-12, -26, 40}, {-4, -53, 22}, {-21, -27, 32},
            {-1, -54, 23}, {0, -54, 19}, {-11, -30, 45}, {4, -53, 22}, {-10, -27, 43}, {3, -58, 23}, {-1, -60, 20}, {0, -59, 24}, {1, -54, 18},
            {-6, -13, 41}, {4, -56, 24}, {4, -60, 19}, {4, -57, 18}, {5, -23, 31}, {-12, -56, 21}, {-10, -55, 19}, {-5, -59, 19}, {-7, -60, 22},
            {-6, -56, 16}, {-3, -24, 29}, {-11, -16, 31}, {-28, -24, 29}, {-28, -23, 29}, {3, -51, 23}, {3, -56, 18}, {2, -16, 28}, {-30, -27, 29},
            {-8, -9, 27}, {-11, -21, 32}, {-29, -51, 7}, {-14, -64, -13}, {-22, -30, 27}, {-6, -19, 30}, {-24, -24, 44}, {-24, -25, 44}, {-1, -51, 21},
            {-20, -32, 47}, {-7, -20, 27}, {-14, -36, 18}, {-10, -47, 19}, {-7, -26, 27}, {-5, -52, 16}, {-9, -49, 17}, {-4, -25, 25}, {-14, -34, 17},
            {-7, -49, 19}, {-7, -33, 20}, {-12, -49, 17}, {-9, -22, 26}, {-12, -25, 24}, {-5, -48, 20}, {2, -16, 25}, {-2, -20, 26}, {11, -50, 20},
            {9, -18, 28}, {6, -31, 20}, {2, -35, 23}, {-14, -53, 17}, {-7, -52, 17}, {-7, -21, 24}, {-11, -20, 27}, {-31, -66, 0}, {-2, 12, 35},
            {-14, -57, 29}, {-37, -58, 8}, {-36, -58, 5}, {-1, -89, 34}, {1, -89, 34}, {6, -61, 15}, {-34, -19, 35}, {-34, -20, 36}, {5, -43, 23},
            {12, -19, 29}, {1, -16, 49}, {2, -16, 49}, {-5, -45, 26}, {-4, -43, 17}, {7, -16, 31}, {-25, -29, 44}, {-3, -42, 25}, {1, -41, 14},
            {-30, -20, 38}, {-6, -24, 28}, {-14, -41, 17}, {-9, -16, 26}, {-21, -42, 12}, {-12, -41, 7}, {-12, -15, 24}, {-14, -21, 24}, {-32, -28, 42},
            {-11, -41, 21}, {0, -15, 28}, {-4, -16, 27}, {-10, -43, 26}, {-9, -34, 18}, {-4, -17, 25}, {-27, -31, 36}, {-14, -27, 26}, {-18, -42, 36},
            {-7, -39, 19}, {0, -20, 26}, {-23, -34, 39}, {-5, -40, 20}, {-2, -30, 19}, {3, -19, 26}, {-11, -34, 19}, {-10, -34, 19}, {-6, -30, 19},
            {-12, -34, 16}, {-4, -21, 23}, {-15, -46, 25}, {-6, -43, 22}, {-4, -30, 20}, {-3, -29, 18}, {-12, -45, 24}, {-25, -13, 33}, {-21, -19, 39},
            {-7, -21, 34}, {-4, -45, 10}, {0, -48, 7}, {-9, -45, 17}, {-15, -18, 39}, {-34, -19, 41}, {7, -12, 32}, {5, -13, 31}, {-2, -45, 12},
            {-9, -9, 45}, {-1, -9, 43}, {4, -46, 10}, {-6, -10, 49}, {-4, -16, 45}, {-9, -26, 48}, {2, -48, 10}, {4, -12, 43}, {7, -30, 45}, {3, -45, 10},
            {11, -21, 40}, {9, -23, 43}, {-1, -49, 11}, {8, -18, 41}, {1, -29, 47}, {2, -45, 11}, {9, -18, 41}, {2, -26, 45}, {-2, -49, 10}, {7, -16, 38},
            {-17, -26, 41}, {-2, -46, 12}, {1, -15, 43}, {-16, -33, 44}, {3, -46, 12}, {14, -16, 35}, {17, -21, 40}, {-5, -48, 14}, {-18, -47, 22},
            {-27, -28, 33}, {-4, -47, 14}, {4, -14, 36}, {-12, -21, 47}, {-27, -31, 33}, {-16, -26, 41}, {-5, -47, 13}, {3, -15, 35}, {-8, -22, 46},
            {-20, -29, 41}, {-9, -28, 42}, {-3, -46, 13}, {1, -8, 40}, {-10, -16, 53}, {1, -46, 10}, {7, -20, 34}, {-3, -24, 45}, {-1, -43, 14},
            {4, -10, 40}, {-6, -19, 42}, {-2, -44, 13}, {4, -14, 34}, {-1, -21, 44}, {2, -45, 13}, {8, -17, 35}, {-5, -22, 42}, {0, -47, 15}, {3, -21, 29},
            {4, -19, 37}, {-1, -45, 12}, {0, -25, 34}, {-1, -40, 36}, {-17, -25, 42}, {-5, -43, 15}, {-1, -20, 33}, {-2, -20, 41}, {-2, -45, 14},
            {3, -20, 35}, {5, -24, 41}, {-4, -43, 15}, {13, -19, 35}, {0, -26, 44}, {-7, -48, 16}, {3, -21, 33}, {-3, -29, 41}, {-4, -44, 13}, {-2, -22, 31},
            {-10, -27, 36}, {-11, -47, 12}, {-35, -25, 31}, {-3, -20, 36}, {-1, -19, 35}, {-14, -24, 38}, {-10, -26, 40}, {-25, -13, 36}, {1, -17, 37},
            {-11, -22, 46}, {-1, -45, 15}, {8, -18, 38}, {6, -24, 45}, {-3, -46, 14}, {-3, -28, 14}, {-5, -27, 46}, {-3, -45, 15}, {3, -12, 41},
            {-4, -15, 49}, {-4, -45, 14}, {-3, -14, 37}, {-7, -22, 45}, {1, -21, 36}, {0, -27, 45}, {0, -45, 15}, {11, -17, 34}, {8, -18, 39}, {12, -33, 42},
            {-1, -42, 14}, {5, -21, 33}, {2, -20, 41}, {-7, -28, 50}, {-5, -45, 14}, {0, -17, 37}, {-9, -30, 40}, {0, -44, 13}, {13, -15, 39},
            {-15, -17, 43}, {-1, -43, 15}, {-2, -19, 36}, {-12, -25, 39}, {3, -30, 13}, {3, -28, 17}, {3, -29, 18}, {1, -19, 27}, {0, -20, 29}, {1, -33, 22},
            {-3, -20, 24}, {-4, -22, 26}, {0, -22, 26}, {-1, -22, 25}, {-1, -24, 22}, {2, -23, 23}, {1, -28, 15}, {-2, -27, 13}, {-1, -32, 20}, {0, -27, 13},
            {-2, -28, 20}, {-2, -29, 19}, {4, -40, 12}, {4, -37, 13}, {2, -37, 13}, {3, -37, 12}, {-5, -43, 10}, {2, -27, 12}, {3, -45, 12}, {-1, -34, 22},
            {-1, -30, 20}, {1, -43, 13}, {2, -44, 14}, {-4, -57, 51}, {-12, -7, 38}, {-28, -38, 27}, {-4, -46, 12}, {-5, -30, 19}, {-21, -38, 35},
            {-18, -36, 37}, {-8, -47, 29}, {-13, -46, 36}, {-15, -43, 37}, {15, -28, 24}, {13, -21, 22}, {13, -21, 25}, {7, -25, 25}, {-5, -32, 20},
            {12, -20, 21}, {9, -24, 20}, {-16, -36, 33}, {0, -46, 15}, {5, -20, 22}, {-12, -28, 35}, {-6, -32, 35}, {-19, -27, 29}, {-22, -30, 29},
            {-4, -48, 17}, {-11, -19, 43}, {6, -45, 17}, {7, -12, 29}, {-6, -12, 43}, {-8, -45, 17}, {3, -13, 26}, {-10, -13, 44}, {-3, -46, 18},
            {9, -13, 22}, {1, -18, 46}, {0, -51, 16}, {8, -10, 43}, {6, -16, 44}, {-7, -47, 15}, {5, -9, 28}, {-6, 0, 47}, {-3, -47, 19}, {9, -14, 22},
            {13, -5, 45}, {-8, -48, 18}, {4, -16, 22}, {-16, -29, 39}, {-10, -48, 15}, {-15, -21, 41}, {-5, -30, 18}, {-20, -28, 40}, {-4, -30, 25},
            {3, -48, 24}, {-11, -22, 31}, {-16, 0, 35},
        };
        // (One squeeze of that list, 17 cm left, 1 cm up, 3 cm forward, is at the top of the left shoulder, 2 cm from where the right hand drew the Mk11
        // there on 2026-11-19 (17 left, 0, 1): the shoulders made big that night have it, as they must; it's TN1's now, not play's.)
        const int n = static_cast<int>(sizeof(kUser) / sizeof(kUser[0]));
        int inside = 0, putAway = 0;
        float nearest = 1e9f;
        Place nearestPlace = Place::kNone;
        for (int k = 0; k < n; ++k) {
            const float p[3] = {kUser[k][0] / 100.0f, kUser[k][1] / 100.0f, kUser[k][2] / 100.0f};
            for (const Place q : eggs) {
                const float z = IsShoulder(q) ? SizeFor(q, p, s) : Size(EggOf(q, s), p);
                inside += z <= 1.0f;
                if (z < nearest)
                    nearest = z, nearestPlace = q;
            }
            putAway += PlaceAt(p, s, nullptr, s.stowMore) != Place::kNone;
        }
        Report("Z5", "none of the user's own squeezes in play inside a place, nor in a place grown for a put-away", n > 400 && inside == 0 && putAway == 0,
               "%d squeezes; %d inside, %d in a put-away's; the nearest %.2f of %s's size", n, inside, putAway, nearest, Name(nearestPlace));
        // The reach test's logged squeezes, each within 0.6 of its own place's Size: the user's own, 2026-11-13 16:48-16:53, seated, every one seen by
        // the headset ("Tracking: the ... grip squeezed at ...", the neck point's right, up, forward). The chests' and the rib's squeezes aren't rows: the
        // chests are reserved and the launcher's place is the neck (their squeezes landed 5 cm under the chests and 8 cm in front of the rib).
        struct Reach {
            Place place;
            float at[3];
        };
        static const Reach kReach[] = {
            // the back of the neck, the right hand over the right shoulder (step 1)
            {Place::kNeck, {0.03f, 0.07f, -0.16f}}, {Place::kNeck, {-0.01f, 0.09f, -0.16f}}, {Place::kNeck, {0.03f, 0.09f, -0.16f}},
            {Place::kNeck, {0.02f, 0.12f, -0.15f}}, {Place::kNeck, {0.02f, 0.11f, -0.12f}}, {Place::kNeck, {-0.01f, 0.03f, -0.15f}},
            {Place::kNeck, {0.02f, 0.04f, -0.17f}}, {Place::kNeck, {-0.02f, 0.05f, -0.16f}}, {Place::kNeck, {0.0f, 0.03f, -0.17f}},
            // ... the left hand over the left shoulder (step 2)
            {Place::kNeck, {-0.02f, 0.04f, -0.19f}}, {Place::kNeck, {-0.05f, 0.04f, -0.17f}}, {Place::kNeck, {-0.04f, 0.06f, -0.15f}},
            {Place::kNeck, {0.01f, 0.04f, -0.18f}}, {Place::kNeck, {-0.01f, 0.08f, -0.17f}}, {Place::kNeck, {0.02f, 0.08f, -0.17f}},
            // the tops of the shoulders, each by its own hand (step 4)
            {Place::kRightShoulder, {0.22f, -0.03f, -0.05f}}, {Place::kRightShoulder, {0.20f, -0.02f, -0.05f}},
            {Place::kLeftShoulder, {-0.20f, -0.03f, -0.01f}}, {Place::kLeftShoulder, {-0.19f, -0.02f, -0.01f}},
            // the belly, each hand (step 7)
            {Place::kBelly, {-0.05f, -0.45f, 0.14f}}, {Place::kBelly, {0.05f, -0.41f, 0.14f}},
        };
        int rows = 0, off = 0;
        for (const Reach& q : kReach) {
            if (q.place == Place::kNone)
                continue;
            ++rows;
            off += Size(EggOf(q.place, s), q.at) > 0.6f;
        }
        Report("Z5b", "the reach test's logged squeezes within 0.6 of their places' Size", off == 0 && rows > 0,
               "%d rows, %d outside 0.6", rows, off);
    }
    {
        Settings m = NeckSet();
        m.mirror = true;
        m.launcher = Launcher::kRib;
        SlotIn slots[kSlots];
        slots[0] = {true, false, true}, slots[1] = {true, false, true}, slots[5] = {true, true, true};
        Place places[kSlots];
        PlacesOf(slots, m, places);
        Report("Z6", "mirror 1: the primary on the left shoulder, the secondary on the right, the launcher's rib on the right",
               places[0] == Place::kLeftShoulder && places[1] == Place::kRightShoulder && places[5] == Place::kRightRib &&
                   PlaceAt(kRS, m) == Place::kRightShoulder,
               "%s, %s, %s (the eggs themselves don't move: Z1 counts both ribs)", Name(places[0]), Name(places[1]), Name(places[5]));
    }
}

// ---- M: the map ----

void Map() {
    const Settings s = NeckSet();
    Place p[kSlots];
    Sim rock;
    Rock(rock);
    PlacesOf(rock.slots, s, p);
    Report("M1", "Rock and a Hard Place's start: the M416 at the right shoulder, the SMAW behind the neck, 2, 6, 7 none",
           p[0] == Place::kRightShoulder && p[5] == Place::kNeck && p[2] == Place::kNone && p[6] == Place::kNone && p[7] == Place::kNone, "%s, %s, %s, %s, %s",
           Name(p[0]), Name(p[5]), Name(p[2]), Name(p[6]), Name(p[7]));
    {
        // The default since 2026-11-15, "launcher shoulder" (the user: "make sure primaries go on the right and secondaries go on the left. So
        // that means the SMAW should go on the left shoulder").
        const Settings d;
        Place q[kSlots];
        PlacesOf(rock.slots, d, q);
        Report("L1", "the default: Rock and a Hard Place's M416 at the right shoulder, the SMAW at the left shoulder (the secondary's)",
               q[0] == Place::kRightShoulder && q[5] == Place::kLeftShoulder, "%s, %s", Name(q[0]), Name(q[5]));
        Sim an;
        Rock(an);
        an.slots[1] = {true, false, true};  // a gun of its own in slot 1 (an AN-94 taken up): it has the secondary's shoulder
        PlacesOf(an.slots, d, q);
        Report("L2", "the default with a gun in slot 1: that gun at the left shoulder, the SMAW behind the neck instead",
               q[0] == Place::kRightShoulder && q[1] == Place::kLeftShoulder && q[5] == Place::kNeck, "%s, %s, %s", Name(q[0]), Name(q[1]), Name(q[5]));
        Sim st;
        Rock(st);
        st.slots[1] = {true, true, true};  // the Stinger taken up: slot 1, a launcher (by its kind, not its slot)
        PlacesOf(st.slots, d, q);
        Report("L3", "the default with two launchers (the Stinger in slot 1, the SMAW in 5): both at the left shoulder, as both were behind the neck",
               q[1] == Place::kLeftShoulder && q[5] == Place::kLeftShoulder, "%s, %s", Name(q[1]), Name(q[5]));
        const float neckMid[3] = {d.neck.mid[0], d.neck.mid[1], d.neck.mid[2]};
        Report("L4", "the default: the neck's place still holds guns (the fallback) and is found there; the shoulders as before",
               Holds(Place::kNeck, d) && Holds(Place::kLeftShoulder, d) && Holds(Place::kRightShoulder, d) && PlaceAt(neckMid, d) == Place::kNeck,
               "holds the neck %d, found at its middle: %s", Holds(Place::kNeck, d) ? 1 : 0, Name(PlaceAt(neckMid, d)));
    }
    {
        Sim st = StowedRock();
        st.slots[1] = {true, true, true};  // the Stinger taken up: slot 1, a launcher
        PlacesOf(st.slots, s, p);
        float held[kSlots];
        for (float& v : held)
            v = -1e9f;
        const int none = GunAt(Place::kNeck, st.slots, p, 0, Holder::kStowed, held);
        held[1] = 5.0f, held[5] = 3.0f;
        const int stinger = GunAt(Place::kNeck, st.slots, p, 0, Holder::kStowed, held);
        held[5] = 7.0f;
        const int smaw = GunAt(Place::kNeck, st.slots, p, 0, Holder::kStowed, held);
        Report("M2", "after the Stinger: both launchers behind the neck, a draw there gives the one in the hands last (none yet: slot 5's)",
               p[1] == Place::kNeck && p[5] == Place::kNeck && none == 5 && stinger == 1 && smaw == 5, "%s; none %d, the Stinger last %d, the SMAW last %d",
               Name(p[1]), none, stinger, smaw);
    }
    {
        Sim sb;
        Swordbreaker(sb);
        sb.holder = Holder::kStowed, sb.trigger = -1;
        sb.Run(0.1f);
        PlacesOf(sb.slots, s, p);
        sb.MoveTo(0, kLS[0], kLS[1], kLS[2]);
        sb.Run(0.1f);
        sb.hand[0].squeeze = 1.0f;
        sb.Run(0.6f);
        const Event* e = sb.Find(What::kRefused, 0);
        Report("M3", "Swordbreaker: the M4A1 at the right shoulder, the Mk11 at the left, refused (the hands don't know it)",
               p[0] == Place::kRightShoulder && p[1] == Place::kLeftShoulder && e && e->why == Why::kUnknown && sb.Keys() == 0 && sb.draws == 0,
               "%s, %s; %s", Name(p[0]), Name(p[1]), e ? "refused" : "not refused");
    }
    {
        SlotIn at4[kSlots];
        at4[0] = {true, true, false};
        at4[1] = {true, false, false};
        PlacesOf(at4, s, p);
        Report("M4", "the AT4 in slot 0: behind the neck (by its kind)", p[0] == Place::kNeck, "%s", Name(p[0]));
        SlotIn jav[kSlots];
        jav[5] = {true, true, false};
        PlacesOf(jav, s, p);
        const Place in5 = p[5];
        jav[5] = SlotIn{}, jav[0] = {true, true, false};
        PlacesOf(jav, s, p);
        Report("M5", "the Javelin in slot 5 and in slot 0: behind the neck", in5 == Place::kNeck && p[0] == Place::kNeck, "%s, %s", Name(in5), Name(p[0]));
        SlotIn an[kSlots];
        an[0] = {true, false, true}, an[1] = {true, false, true}, an[5] = {true, true, true};
        PlacesOf(an, s, p);
        Report("M6", "an AN-94 taken into slot 1: the left shoulder", p[1] == Place::kLeftShoulder, "%s", Name(p[1]));
        Settings m = NeckSet();
        m.mirror = true;
        PlacesOf(an, m, p);
        const Place m0 = p[0], m1 = p[1], m5 = p[5];
        m.launcher = Launcher::kRib;
        PlacesOf(an, m, p);
        Report("M7", "mirror 1: slot 0 at the left shoulder, slot 1 at the right, the launcher behind the neck or at the right rib cage",
               m0 == Place::kLeftShoulder && m1 == Place::kRightShoulder && m5 == Place::kNeck && p[5] == Place::kRightRib, "%s, %s, %s, %s", Name(m0),
               Name(m1), Name(m5), Name(p[5]));
        SlotIn odd[kSlots];
        odd[9] = {true, false, true}, odd[4] = {true, false, false}, odd[3] = {true, false, true};
        PlacesOf(odd, s, p);
        Report("M8", "slot 9 (Kaffarov's pistol), slot 4 (the claymore), slot 3: no place", p[9] == Place::kNone && p[4] == Place::kNone && p[3] == Place::kNone,
               "%s, %s, %s", Name(p[9]), Name(p[4]), Name(p[3]));
        SlotIn unknown[kSlots];
        unknown[5] = {true, false, false}, unknown[0] = {true, false, false};
        PlacesOf(unknown, s, p);
        Report("M9", "a gun of no known kind in slot 5: behind the neck; in slot 0: the right shoulder", p[5] == Place::kNeck && p[0] == Place::kRightShoulder,
               "%s, %s", Name(p[5]), Name(p[0]));
    }
    {
        Sim ly;
        Rock(ly);
        ly.holder = Holder::kLoose, ly.trigger = -1;
        ly.Run(0.1f);
        PlacesOf(ly.slots, s, p);
        const bool m416 = AtHolster(0, ly.slots, 0, Holder::kLoose), smaw = AtHolster(5, ly.slots, 0, Holder::kLoose);
        ly.MoveTo(1, kNeckAt[0], kNeckAt[1], kNeckAt[2]);
        ly.Run(0.1f);
        ly.hand[1].squeeze = 1.0f;
        ly.Run(0.6f);
        const Event* e = ly.Find(What::kRefused, 1);
        Report("M10", "the game's weapon lying: not at its place, the others at theirs, a draw refused", !m416 && smaw && e && e->why == Why::kLying &&
                                                                                                           ly.Keys() == 0 && ly.draws == 0,
               "the M416 %s, the SMAW %s; %s", m416 ? "at its place" : "not", smaw ? "at its place" : "not", e ? "refused" : "not refused");
    }
    {
        Settings two = NeckSet();
        two.twoLaunchersBySlot = true;
        SlotIn both[kSlots];
        both[0] = {true, false, true}, both[1] = {true, true, true}, both[5] = {true, true, true};
        PlacesOf(both, two, p);
        const Place stinger = p[1], smaw = p[5];
        both[5] = SlotIn{};
        PlacesOf(both, two, p);
        Report("M11", "twolaunchers slot: with both, the Stinger at the left shoulder and the SMAW behind the neck; alone, the Stinger behind the neck",
               stinger == Place::kLeftShoulder && smaw == Place::kNeck && p[1] == Place::kNeck, "%s, %s; alone %s", Name(stinger), Name(smaw), Name(p[1]));
        Settings off = NeckSet();
        off.launcher = Launcher::kOff;
        both[5] = {true, true, true};
        PlacesOf(both, off, p);
        SlotIn five[kSlots];
        five[5] = {true, false, true};
        Place q[kSlots];
        PlacesOf(five, off, q);
        Report("M12", "launcher off: no launcher has a place, nor slot 5's gun", p[1] == Place::kNone && p[5] == Place::kNone && q[5] == Place::kNone &&
                                                                                       p[0] == Place::kRightShoulder,
               "%s, %s, %s", Name(p[1]), Name(p[5]), Name(q[5]));
    }
}

// ---- S: putting away (seen) ----

void PutAway() {
    {
        Sim s = HeldRock();
        s.MoveTo(1, kRS[0], kRS[1], kRS[2]);
        s.Run(0.1f);
        const float at = s.time;
        s.hand[1].squeeze = 0.0f;
        s.Run(0.1f);
        const Event* e = s.Find(What::kStowed, 1);
        Report("S1", "a slow let-go at its own place: put away", s.stows == 1 && s.holder == Holder::kStowed && s.falls == 0 && e &&
                                                                     e->place == Place::kRightShoulder && e->at == Place::kNone,
               "put away %d, fell %d, %s", s.stows, s.falls, e ? AtName(e->place) : "no line");
        Report("S1b", "its click on that hand, the vest's sound in the middle", s.Pulses(1, kClickAmp, at) == 1 && s.sounds.size() == 1 &&
                                                                                    s.sounds[0].second == Sound::kStow && s.lastPan == 0.0f,
               "%d clicks, %d sounds, pan %.2f", s.Pulses(1, kClickAmp, at), static_cast<int>(s.sounds.size()), s.lastPan);
    }
    for (int any = 1; any >= 0; --any) {
        Sim s = HeldRock();
        s.set.anyHolster = any == 1;
        s.MoveTo(1, kNeckAt[0], kNeckAt[1], kNeckAt[2]);
        s.Run(0.1f);
        s.hand[1].squeeze = 0.0f;
        s.Run(0.1f);
        const Event* e = s.Find(What::kStowed, 1);
        if (any)
            Report("S2", "let go at another holster's place, anyholster 1: to its own place", s.stows == 1 && e && e->place == Place::kRightShoulder &&
                                                                                                  e->at == Place::kNeck,
                   "put away %d (%s, let go %s)", s.stows, e ? Name(e->place) : "-", e ? AtName(e->at) : "-");
        else
            Report("S2b", "the same with anyholster 0: it falls", s.stows == 0 && s.falls == 1 && s.holder == Holder::kLoose, "put away %d, fell %d", s.stows,
                   s.falls);
    }
    {
        // (2026-11-19, the owner: "I should just be able to just swing it to my shoulders and easily let go and it'll holster": at a place a let-go is
        // never a throw. Until then a let-go there faster than 2.2 m/s was one, and the gun fell.)
        const float speeds[3] = {2.5f, 4.0f, -3.0f};
        int stowed = 0, fell = 0;
        for (const float v : speeds) {
            Sim s = HeldRock();
            s.MoveTo(1, kRS[0], kRS[1], kRS[2], v, 0.0f, 0.0f);
            s.Run(0.1f);
            s.hand[1].squeeze = 0.0f;
            s.Run(0.1f);
            stowed += s.stows, fell += s.falls;
        }
        Report("S3", "let go at its place at 2.5 and 4 m/s going out, 3 m/s going in: put away each time, never a throw at a place (2026-11-19)",
               stowed == 3 && fell == 0, "put away %d, fell %d", stowed, fell);
        // A throw: outside the places, moving away from them. Just outside the right shoulder's put-away egg (never in it), going away at 2.5 m/s: it
        // falls (and its line says where); the same at rest there: it falls, not thrown.
        const Egg e = Settings{}.shoulder;
        int thrownFell = 0, restFell = 0, thrownLines = 0, restLines = 0;
        bool thrownSaid = false;
        for (int k = 0; k < 2; ++k) {
            Sim s = HeldRock();
            s.MoveTo(1, e.mid[0] + e.half[0] + s.set.stowMore + 0.04f, e.mid[1], e.mid[2], k == 0 ? 2.5f : 0.0f, 0.0f, 0.0f);
            s.Run(0.1f);
            s.hand[1].squeeze = 0.0f;
            s.Run(0.1f);
            const Event* f = s.Find(What::kFell, 1);
            (k == 0 ? thrownFell : restFell) = s.falls * (s.stows == 0);
            (k == 0 ? thrownLines : restLines) = s.Count(What::kFell, 1);
            if (k == 0)
                thrownSaid = f && f->thrown && f->place == Place::kRightShoulder && f->out > 0.03f && f->out < 0.05f;
            else
                thrownSaid = thrownSaid && f && !f->thrown;
        }
        Report("S3b", "let go 4 cm outside the right shoulder's put-away egg going away at 2.5 m/s: a throw, it falls; at rest there: it falls; each said "
                      "with where the hand was",
               thrownFell == 1 && restFell == 1 && thrownLines == 1 && restLines == 1 && thrownSaid, "thrown: fell %d (%d lines); at rest: fell %d (%d lines)",
               thrownFell, thrownLines, restFell, restLines);
    }
    {
        Sim s = HeldRock();
        s.hand[0].onGun = true;  // the left on the handguard
        s.MoveTo(1, kRS[0], kRS[1], kRS[2]);
        s.Run(0.1f);
        s.hand[1].squeeze = 0.0f;
        s.Run(0.1f);
        Report("S4", "the other hand on the handguard: the hold wins", s.stows == 0 && s.falls == 0 && s.holder == Holder::kBarrel && s.barrel == 0 &&
                                                                           !s.everStows[1],
               "holder %d, barrel %d, put away %d", static_cast<int>(s.holder), s.barrel, s.stows);
    }
    {
        Sim s;
        Rock(s);
        s.holder = Holder::kBarrel, s.trigger = -1, s.barrel = 0;
        s.hand[0].onGun = true, s.hand[0].squeeze = 1.0f;
        s.MoveTo(0, kRS[0], kRS[1], kRS[2]);
        s.Run(0.1f);
        s.hand[0].onGun = false, s.hand[0].squeeze = 0.0f;  // (the hand opens: off the handguard)
        s.Run(0.05f);
        Report("S5", "by the hand on the handguard alone: put away", s.stows == 1 && s.stowedBy == 0 && s.holder == Holder::kStowed, "put away %d by %d",
               s.stows, s.stowedBy);
    }
    {
        int stowsTrue = 0, stowed = 0;
        const char* names[3] = {"the grenade launcher's shot", "a scripted moment", "the game's reload"};
        char said[160] = "";
        for (int k = 0; k < 3; ++k) {
            Sim s = HeldRock();
            s.MoveTo(1, kRS[0], kRS[1], kRS[2]);
            s.Run(0.1f);
            s.m320 = k == 0, s.frozen = k == 1, s.gameReload = k == 2;
            s.everStows[1] = false;
            s.Run(0.05f);
            s.hand[1].squeeze = 0.0f;
            s.Run(0.1f);
            stowsTrue += s.everStows[1];
            stowed += s.stows;
            std::snprintf(said + std::strlen(said), sizeof(said) - std::strlen(said), "%s%s: %s", k ? "; " : "", names[k],
                          s.falls ? "fell" : s.holder == Holder::kHeld ? "still held" : "?");
        }
        Report("S6", "the grenade launcher's shot under way, a scripted moment, the game's reload: as today, never put away", stowsTrue == 0 && stowed == 0,
               "%s", said);
    }
    {
        // Past the put-away's egg and its grace's 10 cm (31 cm out from the reach test's top of the shoulder, 30 from the egg's middle): it falls.
        Sim s = HeldRock();
        s.MoveTo(1, kRS[0] + 0.31f, kRS[1], kRS[2]);
        s.Run(0.1f);
        s.hand[1].squeeze = 0.0f;
        s.Run(0.1f);
        Report("S7", "let go 31 cm out from its place's top (past the put-away's egg and its grace's 10 cm): it falls", s.stows == 0 && s.falls == 1,
               "put away %d, fell %d", s.stows, s.falls);
    }
    {
        // (2026-11-16, the user: "make it a little bit easier to holster your gun": the put-away's egg 3 cm bigger than the tick's.)
        const Egg e = Settings{}.shoulder;
        Sim s = HeldRock();
        s.MoveTo(1, e.mid[0] + e.half[0] + 0.02f, e.mid[1], e.mid[2]);
        s.Run(0.1f);
        s.hand[1].squeeze = 0.0f;
        s.Run(0.1f);
        Report("S7e", "let go 2 cm outside the tick's egg: put away (stowmore's 3 cm)", s.stows == 1 && s.falls == 0 && PlaceAt(s.hand[1].pos, s.set) ==
                                                                                                                                 Place::kNone,
               "put away %d, fell %d", s.stows, s.falls);
    }
    {
        // The grace (stowgrace's 0.6 s and 10 cm; 0.5 s and Size 1.6 of the old egg until 2026-11-19): in the place, then 6 cm out of its put-away egg
        // and let go 0.2 s and 0.55 s after (put away), 0.8 s after (it falls); and 0.2 s after going away at 2.5 m/s (a throw: it falls).
        const Egg e = Settings{}.shoulder;
        const float out = e.mid[0] + e.half[0] + Settings{}.stowMore + 0.06f;
        const float waits[4] = {0.2f, 0.55f, 0.8f, 0.2f};
        int stowed[4] = {}, fell[4] = {};
        for (int k = 0; k < 4; ++k) {
            Sim s = HeldRock();
            s.MoveTo(1, kRS[0], kRS[1], kRS[2]);
            s.Run(0.2f);
            s.MoveTo(1, out, e.mid[1], e.mid[2], k == 3 ? 2.5f : 0.0f, 0.0f, 0.0f);
            s.Run(waits[k]);
            s.hand[1].squeeze = 0.0f;
            s.Run(0.1f);
            stowed[k] = s.stows, fell[k] = s.falls;
        }
        Report("S7g", "in its place, then 6 cm out of it: let go 0.2 s and 0.55 s after, put away (the grace); 0.8 s after, it falls; 0.2 s after going "
                      "away at 2.5 m/s, it falls (a throw)",
               stowed[0] == 1 && fell[0] == 0 && stowed[1] == 1 && fell[1] == 0 && stowed[2] == 0 && fell[2] == 1 && stowed[3] == 0 && fell[3] == 1,
               "0.2 s: put away %d, fell %d; 0.55 s: %d, %d; 0.8 s: %d, %d; thrown: %d, %d", stowed[0], fell[0], stowed[1], fell[1], stowed[2], fell[2],
               stowed[3], fell[3]);
    }
    {
        const char* what[4] = {"a rocket or a case", "a 40 mm round", "its magazine", "its magazine (heldmag refuse)"};
        char said[256] = "";
        bool ok = true;
        for (int k = 0; k < 4; ++k) {
            Sim s = HeldRock();
            s.set.heldMagRefuse = k == 3;
            s.hand[0].rocket = k == 0, s.hand[0].round40 = k == 1, s.hand[0].magazine = k >= 2;
            s.MoveTo(1, kRS[0], kRS[1], kRS[2]);
            s.Run(0.1f);
            const float at = s.time;
            s.hand[1].squeeze = 0.0f;
            s.Run(0.2f);
            const bool refused = k != 2;
            const Event* e = s.Find(What::kStowRefused, 1);
            const bool good = refused ? (s.stows == 0 && s.falls == 0 && s.holder == Holder::kHeld && s.trigger == 1 && e && e->kept &&
                                         s.Count(What::kStowRefused, 1) == 1 && s.Pulses(1, kDoubleAmp, at) == 2)
                                      : (s.stows == 1 && !e);
            ok = ok && good;
            std::snprintf(said + std::strlen(said), sizeof(said) - std::strlen(said), "%s%s: %s", k ? "; " : "", what[k],
                          s.stows ? "put away" : e && s.holder == Holder::kHeld ? "refused, kept in the hand, a double click" : s.falls ? "fell" : "?");
        }
        Report("S8", "the other hand holding a rocket, a case or a 40 mm round: refused, the gun stays in the hand (said once, the double click: never "
                     "dropped at a place, review finding 1); its magazine: put away (it falls)", ok, "%s", said);
    }
    {
        float pans[3] = {};
        int sounds[3] = {};
        for (int k = 0; k < 3; ++k) {
            Sim s = HeldRock();
            s.set.stowPanSide = k >= 1;
            s.set.stowSound = k == 2 ? StowSound::kNone : StowSound::kVest;
            s.MoveTo(1, k == 1 ? kLS[0] : kRS[0], kRS[1], kRS[2]);
            s.Run(0.1f);
            s.hand[1].squeeze = 0.0f;
            s.Run(0.1f);
            pans[k] = s.lastPan, sounds[k] = static_cast<int>(s.sounds.size());
        }
        Sim r = HeldRock();
        r.set.stowPanSide = true;
        r.MoveTo(1, kRS[0], kRS[1], kRS[2]);
        r.Run(0.1f);
        r.hand[1].squeeze = 0.0f;
        r.Run(0.1f);
        Report("S9", "the put-away's sound in the middle by default; stowpan side: the place's side; stowsound none: none",
               pans[0] == 0.0f && sounds[0] == 1 && pans[1] < 0.0f && r.lastPan > 0.0f && sounds[2] == 0, "middle %.1f; the left shoulder %.1f, the right %.1f; none: %d",
               pans[0], pans[1], r.lastPan, sounds[2]);
    }
    {
        int frames[3] = {};
        for (int k = 0; k < 3; ++k) {
            Sim s = HeldRock();
            s.set.vanish = k == 0 ? 0.0f : 0.15f;
            s.MoveTo(1, kRS[0], kRS[1], kRS[2]);
            s.Run(0.1f);
            s.hand[1].squeeze = 0.0f;
            s.Run(0.06f);  // (the let-go: 3 frames)
            if (k == 2)
                s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
            s.Run(0.4f);
            frames[k] = s.shownFrames;
        }
        Report("S10", "vanish 0.15: the gun kept in the open hand 0.15 s at its place, gone at once if the hand leaves; vanish 0: never",
               frames[0] == 0 && frames[1] >= 10 && frames[1] <= 12 && frames[2] <= 2, "%d, %d, %d frames", frames[0], frames[1], frames[2]);
    }
}

// ---- D: drawing (seen) ----

void Draw() {
    for (int side = 1; side >= 0; --side) {
        Sim s = StowedRock();
        s.MoveTo(side, kRS[0], kRS[1], kRS[2]);
        s.Run(0.1f);
        const float at = s.time;
        s.hand[side].squeeze = 1.0f;
        s.Run(0.1f);
        const Event* e = s.Find(What::kDrawn, side);
        Report(side ? "D1" : "D2", side ? "case A by the right: in the hand at once, no key" : "case A by the left: the left is the trigger hand",
               s.draws == 1 && s.drawnBy == side && s.holder == Holder::kHeld && s.trigger == side && s.Keys() == 0 && e &&
                   s.Pulses(side, kClickAmp, at) == 1 && !s.sounds.empty() && s.sounds.back().second == Sound::kDraw,
               "drawn %d by %d, keys %d, %s", s.draws, s.drawnBy, s.Keys(), e ? "said" : "not said");
    }
    {
        Sim s = StowedRock();
        s.MoveTo(1, kNeckAt[0], kNeckAt[1], kNeckAt[2]);
        s.Run(0.1f);
        const float at = s.time;
        s.hand[1].squeeze = 1.0f;
        s.Step();
        const bool keyAtOnce = s.Keys(5) == 1 && s.keys[0].first <= at + 0.05f;
        s.Run(0.3f);
        s.gameSlot = 5;  // (the game's slot moves)
        s.Run(0.2f);
        const bool waited = s.holder == Holder::kStowed && s.draws == 0 && s.drawSides == 0;  // (the M416 never put in the hand meanwhile)
        const ArriveOut o = s.Switch(5);
        s.Run(0.1f);
        Report("D3", "case B behind the neck: key 4 at once, the hand keeps the coming gun, the SMAW in it from its first frame", keyAtOnce && waited &&
                                                                                                                                    o.arrival == Arrival::kHand && o.side == 1 &&
                                                                                                                                    s.holder == Holder::kHeld && s.trigger == 1 &&
                                                                                                                                    s.Count(What::kAsked, 1) == 1 &&
                                                                                                                                    s.Pulses(1, kClickAmp, at) >= 1 &&
                                                                                                                                    s.Keys() == 1,
               "key at %.2f s, waited %d, arrival %d into %d, keys %d", s.keys.empty() ? -1.0f : s.keys[0].first - at, waited, static_cast<int>(o.arrival),
               o.side, s.Keys());
    }
    {
        Sim s = StowedRock();
        s.MoveTo(1, kNeckAt[0], kNeckAt[1], kNeckAt[2]);
        s.Run(0.1f);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.25f);
        s.hand[1].squeeze = 0.0f;  // let go first
        s.Run(0.1f);
        s.gameSlot = 5;
        s.Run(0.1f);
        const ArriveOut o = s.Switch(5);
        s.Run(0.1f);
        Report("D4", "the grip let go before the gun came: it comes put away, the hand empty", o.arrival == Arrival::kAtPlace && s.holder == Holder::kStowed &&
                                                                                                    o.event.what == What::kCameAway && o.event.why == Why::kOpened,
               "arrival %d, holder %d", static_cast<int>(o.arrival), static_cast<int>(s.holder));
    }
    {
        Sim s = StowedRock();
        s.MoveTo(1, kNeckAt[0], kNeckAt[1], kNeckAt[2]);
        s.Run(0.1f);
        const float at = s.time;
        s.hand[1].squeeze = 1.0f;
        s.Run(2.0f);
        const Event* again = s.Find(What::kKeyAgain, 1);
        const Event* gave = s.Find(What::kNoSwitch, 1);
        const float second = s.keys.size() >= 2 ? s.keys[1].first - at : -1.0f;
        // The double click: two pulses 60 ms apart at the give-up.
        std::vector<float> doubles;
        for (const Buzzed& b : s.buzzes)
            if (b.side == 1 && std::fabs(b.amp - kDoubleAmp) < 1e-4f && std::fabs(b.seconds - kDoubleSeconds) < 1e-4f)
                doubles.push_back(b.at - at);
        Report("D5", "no switch: the key again at 0.6 s, given up at 1.5 s with the double click and a line", s.Keys(5) == 2 && second > 0.58f &&
                                                                                                                 second < 0.64f && again && gave && gave->twice &&
                                                                                                                 doubles.size() == 2 && doubles[0] > 1.48f &&
                                                                                                                 doubles[0] < 1.53f &&
                                                                                                                 std::fabs(doubles[1] - doubles[0] - kDoubleGap) < 0.02f &&
                                                                                                                 s.holder == Holder::kStowed,
               "keys %d (the second at %.2f s), given up %s, pulses at %.2f and %.2f s", s.Keys(5), second, gave ? "yes" : "no",
               doubles.size() > 0 ? doubles[0] : -1.0f, doubles.size() > 1 ? doubles[1] : -1.0f);
    }
    {
        Sim s = HeldRock();
        s.MoveTo(0, kNeckAt[0], kNeckAt[1], kNeckAt[2]);
        s.Run(0.1f);
        const float at = s.time;
        s.hand[0].squeeze = 1.0f;
        s.Run(0.6f);
        const Event* e = s.Find(What::kRefused, 0);
        Report("D6", "the other hand holds the gun: nothing drawn, silent (a line with log 1)", e && e->why == Why::kOtherHolds && e->quiet && s.Keys() == 0 &&
                                                                                                     s.Pulses(0, -1.0f, at) == 0 && s.holder == Holder::kHeld,
               "%s, %d pulses on the left", e ? "refused quietly" : "no line", s.Pulses(0, -1.0f, at));
    }
    for (int take = 0; take < 2; ++take) {
        Sim s;
        Rock(s);
        s.holder = Holder::kLoose, s.trigger = -1;
        s.Run(0.1f);
        s.MoveTo(1, kNeckAt[0], kNeckAt[1], kNeckAt[2]);
        s.Run(0.1f);
        const float at = s.time;
        s.hand[1].squeeze = 1.0f;
        s.Run(0.2f);
        const bool notYet = s.Count(What::kRefused, 1) == 0 && s.Pulses(1, -1.0f, at) == 0;
        if (take)
            s.takeBy = 1;  // (the magnet took the gun with this squeeze)
        s.Run(0.5f);
        const Event* e = s.Find(What::kRefused, 1);
        if (!take)
            Report("D7", "the gun lying: refused once the take's window is over, the double click", notYet && e && e->why == Why::kLying &&
                                                                                                    s.Pulses(1, kDoubleAmp, at) == 2,
                   "nothing in the window %d, %s", notYet, e ? "then refused" : "never refused");
        else
            Report("D7b", "the gun lying and the magnet takes it with that squeeze: no refusal stands in for the take", notYet && !e &&
                                                                                                                         s.Pulses(1, kDoubleAmp, at) == 0 &&
                                                                                                                         s.holder == Holder::kHeld,
                   "refusals %d, holder %d", s.Count(What::kRefused, 1), static_cast<int>(s.holder));
    }
    {
        // The right squeezes behind the neck while the gun lies; within the take's window the left takes it (the magnet), or it leaves the ground
        // another way (the game's own pickup): "it lies on the ground" would be false, so nothing is said or felt for the right.
        int lines[2] = {}, pulses[2] = {};
        for (int k = 0; k < 2; ++k) {
            Sim s;
            Rock(s);
            s.holder = Holder::kLoose, s.trigger = -1;
            s.Run(0.1f);
            s.MoveTo(1, kNeckAt[0], kNeckAt[1], kNeckAt[2]);
            s.Run(0.1f);
            const float at = s.time;
            s.hand[1].squeeze = 1.0f;
            s.Run(0.15f);
            if (k == 0)
                s.takeBy = 0;  // (the left's squeeze took it)
            else
                s.holder = Holder::kHeld, s.trigger = 1, s.armed[1] = false;  // (taken up without a hand's squeeze: X held's pickup, as today)
            s.Run(0.6f);
            lines[k] = s.Count(What::kRefused, 1), pulses[k] = s.Pulses(1, kDoubleAmp, at);
        }
        Report("D7c", "the gun lying, a squeeze behind the neck, then the other hand takes it (or it leaves the ground otherwise): no refusal said or felt",
               lines[0] == 0 && pulses[0] == 0 && lines[1] == 0 && pulses[1] == 0, "the left's take: %d lines, %d pulses; picked up otherwise: %d, %d",
               lines[0], pulses[0], lines[1], pulses[1]);
    }
    {
        Sim s;
        Swordbreaker(s);
        s.holder = Holder::kStowed, s.trigger = -1;
        s.Run(0.1f);
        s.MoveTo(0, kLS[0], kLS[1], kLS[2]);
        s.Run(0.1f);
        const float at = s.time;
        s.hand[0].squeeze = 1.0f;
        s.Run(0.3f);
        const Event* e = s.Find(What::kRefused, 0);
        Report("D8", "the Mk11 the hands don't know: refused, the double click, no key", e && e->why == Why::kUnknown && s.Pulses(0, kDoubleAmp, at) == 2 &&
                                                                                             s.Keys() == 0,
               "%s", e ? "refused" : "not refused");
    }
    {
        Sim s = StowedRock();
        s.hand[1].squeeze = 1.0f;  // crossed in front
        s.Run(0.1f);
        for (int k = 0; k <= 20; ++k) {  // moved slowly into the shoulder, held
            const float u = k / 20.0f;
            s.MoveTo(1, kFront[0] + (kRS[0] - kFront[0]) * u, kFront[1] + (kRS[1] - kFront[1]) * u, kFront[2] + (kRS[2] - kFront[2]) * u, 0.1f, 0.0f, 0.0f);
            s.Step();
        }
        s.Run(0.5f);
        Report("D9", "a grip held while moving into the place: nothing", s.draws == 0 && s.Keys() == 0 && s.Said() == 0, "drawn %d, %d lines",
               s.draws, s.Said());
    }
    {
        const char* what[4] = {"frozen", "the game's reload", "the grenade launcher's shot", "a take waiting"};
        bool ok[4] = {};
        int draws[4] = {}, pulses[4] = {};
        for (int k = 0; k < 4; ++k) {
            Sim s = StowedRock();
            s.frozen = k == 0, s.gameReload = k == 1, s.m320 = k == 2, s.pendingTake = k == 3;
            s.MoveTo(1, kRS[0], kRS[1], kRS[2]);
            s.Run(0.1f);
            const float at = s.time;
            s.hand[1].squeeze = 1.0f;
            s.Run(0.5f);
            const Event* e = s.Find(What::kRefused, 1);
            draws[k] = s.draws, pulses[k] = s.Pulses(1, kDoubleAmp, at);
            ok[k] = s.draws == 0 && s.Keys() == 0 && pulses[k] == 0 && (!e || e->quiet);
        }
        Report("D10", "a scripted moment (frozen): nothing drawn, nothing felt", ok[0], "%s: drawn %d, %d pulses", what[0], draws[0], pulses[0]);
        Report("D11", "the game's reload under way: nothing drawn, nothing felt", ok[1], "%s: drawn %d, %d pulses", what[1], draws[1], pulses[1]);
        Report("D11b", "the grenade launcher's shot under way, a take waiting: nothing drawn, nothing felt", ok[2] && ok[3], "%s: drawn %d; %s: drawn %d",
               what[2], draws[2], what[3], draws[3]);
    }
    {
        const char* what[4] = {"a magazine", "a 40 mm round", "a rocket or a case", "on the charging handle"};
        char said[200] = "";
        bool ok = true;
        for (int k = 0; k < 4; ++k) {
            Sim s = StowedRock();
            s.hand[1].magazine = k == 0, s.hand[1].round40 = k == 1, s.hand[1].rocket = k == 2, s.hand[1].busy = k == 3;
            s.MoveTo(1, kRS[0], kRS[1], kRS[2]);
            s.Run(0.1f);
            s.hand[1].squeeze = 0.0f;
            s.Step();
            s.hand[1].squeeze = 1.0f;
            s.Run(0.5f);
            ok = ok && s.draws == 0 && s.Keys() == 0 && s.Pulses(1, kDoubleAmp) == 0;
            std::snprintf(said + std::strlen(said), sizeof(said) - std::strlen(said), "%s%s: %s", k ? "; " : "", what[k], s.draws ? "drawn" : "nothing");
        }
        Report("D12", "a hand that isn't empty (a magazine, a 40 mm round, a rocket or a case, the charging handle): nothing", ok, "%s", said);
    }
    {
        Sim s = StowedRock();
        s.picked = true, s.pickSlot = 5;  // the D-pad up: input's key 4
        s.Step();
        s.Run(0.5f);
        s.gameSlot = 5;
        const ArriveOut o = s.Switch(5);
        Report("D13", "a pick while put away (the D-pad up: the SMAW): into the right hand", o.arrival == Arrival::kHand && o.side == 1 && s.Keys() == 0,
               "arrival %d into %d", static_cast<int>(o.arrival), o.side);
        Sim own = StowedRock();
        const bool asked = PickIsOwnSlot(0, own.st, own.holder, own.set) && !PickIsOwnSlot(5, own.st, own.holder, own.set);
        own.picked = true, own.pickSlot = 0;  // the D-pad down: the M416's own slot
        own.Step();
        own.Run(0.5f);  // (the right grip open all the while: unarmed, it stays in the hand)
        Sim x = StowedRock();
        x.picked = true, x.pickSlot = -1;  // X held's pickup (the game's R)
        x.Step();
        x.Run(1.0f);
        const ArriveOut ox = x.Switch(1);
        Report("D13b", "a pick of the put-away gun's own slot: no key, into the right hand; X held's pickup counts as a pick",
               asked && own.draws == 1 && own.drawnBy == 1 && own.Keys() == 0 && own.holder == Holder::kHeld && own.trigger == 1 && ox.arrival == Arrival::kHand &&
                   ox.side == 1,
               "own slot asked %d, drawn %d by %d; X held: arrival %d into %d", asked, own.draws, own.drawnBy, static_cast<int>(ox.arrival), ox.side);
    }
    {
        Sim s = StowedRock();
        s.Run(3.0f);
        const ArriveOut o = s.Switch(5);
        Report("D14", "the game switching by itself while put away: the new gun at its place", o.arrival == Arrival::kAtPlace && s.holder == Holder::kStowed &&
                                                                                                    o.event.what == What::kCameAway && o.event.why == Why::kNone,
               "arrival %d", static_cast<int>(o.arrival));
        Settings hand = NeckSet();
        hand.gameSwitchHand = true;
        Sim h = StowedRock();
        h.set = hand;
        h.Run(3.0f);
        const ArriveOut oh = h.Switch(5);
        Sim n = StowedRock();
        const ArriveOut on = n.Switch(0, false, true);
        Report("D15", "a new life: today's (the right hand); gameswitch hand: into the right hand", on.arrival == Arrival::kToday &&
                                                                                                        oh.arrival == Arrival::kHand && oh.side == 1,
               "new life %d, gameswitch hand %d", static_cast<int>(on.arrival), static_cast<int>(oh.arrival));
    }
    {
        // A quick grab (2026-11-19, the owner: "you can just quickly draw your gun"): a grip closing in the shoulder with the hand already going back at
        // 0.8 m/s draws from it (until then nothing: the hand had to be under 0.4 m/s). The SMAW behind the neck: this hand isn't going into its place.
        Sim s = StowedRock();
        s.MoveTo(1, kRS[0], kRS[1], kRS[2] - 0.08f, 0.0f, 0.0f, -0.8f);  // 8 cm behind its top, going back at 0.8 m/s
        s.Step();
        s.hand[1].squeeze = 1.0f;
        for (int k = 0; k < 10; ++k) {
            s.hand[1].pos[2] -= 0.8f * kDt;
            s.Step();
        }
        s.Run(0.4f);
        Report("D16", "a grip closing in the shoulder at 0.8 m/s going back: drawn from it (a quick grab)", s.draws == 1 && s.drawnBy == 1 && s.Keys() == 0 &&
                                                                                                              s.holder == Holder::kHeld,
               "drawn %d by %d, keys %d", s.draws, s.drawnBy, s.Keys());
    }
    {
        // The reach over the right shoulder to the back of the neck (the SMAW there, the M416 at the shoulder), seen all the way at 1 m/s, the grip
        // closing as it passes the shoulder: the shoulder's M416 never, the SMAW asked for (key 4) as it gets to the neck's place. Stopped over the
        // shoulder instead: the M416 there.
        const float from[3] = {0.20f, 0.03f, -0.02f};
        int keys5[2] = {}, drawn[2] = {}, keysOther[2] = {};
        for (int k = 0; k < 2; ++k) {
            Sim s = StowedRock();
            s.Move(1, from);
            s.Run(0.1f);
            const float d[3] = {kNeckAt[0] - from[0], kNeckAt[1] - from[1], kNeckAt[2] - from[2]};
            const float len = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
            const float v[3] = {d[0] / len, d[1] / len, d[2] / len};  // 1 m/s
            for (int f = 0; f < 30; ++f) {
                const float u = std::min(1.0f, f * kDt / len);
                const bool moving = k == 0 && u < 1.0f;
                s.MoveTo(1, from[0] + (k == 0 ? d[0] * u : 0.0f), from[1] + (k == 0 ? d[1] * u : 0.0f), from[2] + (k == 0 ? d[2] * u : 0.0f),
                         moving ? v[0] : 0.0f, moving ? v[1] : 0.0f, moving ? v[2] : 0.0f);
                s.hand[1].squeeze = f >= 2 ? 1.0f : 0.0f;  // (closing on the second frame, still over the shoulder)
                s.Step();
            }
            s.Run(0.3f);
            keys5[k] = s.Keys(5), drawn[k] = s.draws, keysOther[k] = s.Keys() - s.Keys(5);
        }
        Report("D16b", "a grip closing over the shoulder on its way into the neck's place at 1 m/s: the SMAW asked for there, never the shoulder's M416; "
                       "stopped over the shoulder: the M416 drawn",
               keys5[0] == 1 && drawn[0] == 0 && keysOther[0] == 0 && keys5[1] == 0 && drawn[1] == 1 && keysOther[1] == 0,
               "going: key 4 %d, drawn %d, other keys %d; stopped: key 4 %d, drawn %d", keys5[0], drawn[0], keysOther[0], keys5[1], drawn[1]);
    }
    {
        // Refusals felt only with both hands empty: with the other hand holding the gun every squeeze at a place is silent.
        Sim s = HeldRock();
        const float* where[3] = {kRS, kLS, kNeckAt};
        const float at = s.time;
        for (const float* p : where) {
            s.MoveTo(0, p[0], p[1], p[2]);
            s.Run(0.1f);
            s.hand[0].squeeze = 1.0f;
            s.Run(0.5f);
            s.hand[0].squeeze = 0.0f;
            s.Run(0.1f);
        }
        Report("D17", "the other hand holding the gun: no pulse at any place", s.Pulses(0, -1.0f, at) == 0 && s.Count(What::kRefused, 0) == 3,
               "%d pulses, %d quiet lines", s.Pulses(0, -1.0f, at), s.Count(What::kRefused, 0));
    }
    {
        Sim s = StowedRock();
        s.MoveTo(0, kLS[0], kLS[1], kLS[2]);
        s.Run(0.1f);
        const float at = s.time;
        s.hand[0].squeeze = 1.0f;
        s.Run(0.3f);
        const Event* e = s.Find(What::kRefused, 0);
        char line[256] = "";
        if (e)
            Say(*e, kNames, line, sizeof(line));
        Report("D18", "nothing at the left shoulder: the double click and its line", e && e->why == Why::kNothing && s.Pulses(0, kDoubleAmp, at) == 2 &&
                                                                                         std::strcmp(line, "Holster: nothing at the left shoulder") == 0,
               "\"%s\"", line);
    }
    {
        // The double click only with both hands empty: the gun put away, the right holding a magazine taken at the belly (or a rocket), the left
        // squeezes at the empty left shoulder: said with "log 1" only, nothing felt.
        int pulses[2] = {}, quiet[2] = {};
        for (int k = 0; k < 2; ++k) {
            Sim s = StowedRock();
            s.hand[1].magazine = k == 0, s.hand[1].rocket = k == 1;
            s.MoveTo(0, kLS[0], kLS[1], kLS[2]);
            s.Run(0.1f);
            const float at = s.time;
            s.hand[0].squeeze = 1.0f;
            s.Run(0.3f);
            const Event* e = s.Find(What::kRefused, 0);
            pulses[k] = s.Pulses(0, -1.0f, at), quiet[k] = e && e->why == Why::kNothing && e->quiet;
        }
        Report("D18b", "the other hand holding a magazine or a rocket (the gun put away): a refusal at a place is silent (a line with log 1)",
               pulses[0] == 0 && quiet[0] && pulses[1] == 0 && quiet[1], "a magazine: %d pulses, quiet %d; a rocket: %d pulses, quiet %d", pulses[0], quiet[0],
               pulses[1], quiet[1]);
    }
    {
        Sim s = HeldRock();
        s.MoveTo(1, kRS[0], kRS[1], kRS[2]);
        s.Run(0.1f);
        s.hand[1].squeeze = 0.0f;
        s.Run(0.05f);
        const int stowed = s.stows;
        s.Run(0.05f);
        s.hand[1].squeeze = 1.0f;  // within 0.25 s of its own let-go
        s.Run(0.3f);
        const int early = s.draws;
        s.hand[1].squeeze = 0.0f;
        s.Run(0.2f);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.2f);
        Report("D19", "a squeeze within 0.25 s of its own put-away draws nothing; later it does", stowed == 1 && early == 0 && s.draws == 1, "put away %d, "
               "drawn early %d, drawn later %d", stowed, early, s.draws);
    }
    {
        // A pick of another slot while a case-B draw waits: the draw is called off (said), its second key never comes against the pick. The D-pad
        // down (the M416 put away: its own slot) draws it into the right hand; X held's pickup (no slot) calls it off too.
        Sim s = StowedRock();
        s.MoveTo(1, kNeckAt[0], kNeckAt[1], kNeckAt[2]);
        s.Run(0.1f);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.3f);  // (key 4 sent, the game hasn't switched)
        s.picked = true, s.pickSlot = 0;
        s.Step();
        s.Run(1.5f);
        const Event* off = s.Find(What::kCalledOff, 1);
        Sim x = StowedRock();
        x.MoveTo(1, kNeckAt[0], kNeckAt[1], kNeckAt[2]);
        x.Run(0.1f);
        x.hand[1].squeeze = 1.0f;
        x.Run(0.3f);
        x.picked = true, x.pickSlot = -1;
        x.Step();
        x.Run(1.5f);
        Report("D20", "a pick of another slot while a draw waits: called off, no second key, nothing given up; the D-pad's own slot drawn into the right",
               s.Keys(5) == 1 && !s.Find(What::kKeyAgain) && !s.Find(What::kNoSwitch) && off && off->why == Why::kPicked && s.draws == 1 &&
                   s.drawnBy == 1 && x.Keys(5) == 1 && !x.Find(What::kKeyAgain) && !x.Find(What::kNoSwitch) && x.Count(What::kCalledOff, 1) == 1,
               "the D-pad: keys %d, %s, drawn %d by %d; X held: keys %d, %d called off", s.Keys(5), off ? "called off" : "not called off", s.draws,
               s.drawnBy, x.Keys(5), x.Count(What::kCalledOff, 1));
    }
    {
        // The squeeze of a case-B draw is that draw's while it waits: the grasp is told the hand is busy (no dropped gun taken with it), from the
        // squeeze till the gun comes; never for the other hand, never for a case-A draw, never once the grip opens.
        Sim s = StowedRock();
        s.MoveTo(1, kNeckAt[0], kNeckAt[1], kNeckAt[2]);
        s.Run(0.1f);
        s.hand[1].squeeze = 1.0f;
        s.Step();
        const int first = s.busyFrames[1];
        s.Run(0.4f);
        const int waiting = s.busyFrames[1];
        s.gameSlot = 5;
        s.Switch(5);
        s.Run(0.3f);
        const int after = s.busyFrames[1] - waiting;
        Sim a = StowedRock();
        a.MoveTo(1, kRS[0], kRS[1], kRS[2]);
        a.Run(0.1f);
        a.hand[1].squeeze = 1.0f;
        a.Run(0.4f);
        Sim o = StowedRock();
        o.MoveTo(1, kNeckAt[0], kNeckAt[1], kNeckAt[2]);
        o.Run(0.1f);
        o.hand[1].squeeze = 1.0f;
        o.Run(0.2f);
        o.hand[1].squeeze = 0.0f;
        const int beforeOpen = o.busyFrames[1];
        o.Run(0.3f);
        // "gameswitch hand": the game switches by itself to another gun while a case-B draw waits: it comes into the right hand and is drawn there
        // (nothing hides a gun in a hand), the draw that waited ends (no second key to take it away again).
        Sim g = StowedRock();
        g.set.gameSwitchHand = true;
        g.slots[1] = {true, false, true};  // (an AN-94 in slot 1)
        g.MoveTo(1, kNeckAt[0], kNeckAt[1], kNeckAt[2]);
        g.Run(0.1f);
        g.hand[1].squeeze = 1.0f;
        g.Run(0.2f);
        g.gameSlot = 1;
        const ArriveOut og = g.Switch(1);
        const int hiddenBefore = g.drawWaitsFrames;
        g.Run(1.5f);
        Report("D21", "a case-B draw's hand busy for the grasp from its squeeze till its gun comes; not the other hand, not case A, not once it opens",
               first == 1 && waiting >= 28 && after == 0 && s.busyFrames[0] == 0 && a.busyFrames[1] == 0 && a.draws == 1 && o.busyFrames[1] - beforeOpen <= 3 &&
                   o.busyFrames[0] == 0,
               "on the squeeze %d, waiting %d frames, after the gun %d; case A %d; opened: %d more", first, waiting, after, a.busyFrames[1],
               o.busyFrames[1] - beforeOpen);
        Report("D22", "gameswitch hand, the game's own switch to another gun while a draw waits: into the right hand, drawn, the draw ended (no second key)",
               og.arrival == Arrival::kHand && og.side == 1 && g.drawWaitsFrames == hiddenBefore && g.Keys(5) == 1 && !g.Find(What::kNoSwitch) &&
                   !g.st.wait.on,
               "arrival %d into %d, hidden %d frames after, keys %d", static_cast<int>(og.arrival), og.side, g.drawWaitsFrames - hiddenBefore, g.Keys(5));
    }
}

// ---- K: the second key ----

void Keys() {
    const Settings s = NeckSet();
    KeyIn k;
    k.target = 5, k.gameSlot = 0, k.readNow = true;
    const auto at = [&](float t, int presses, bool harm = false, bool readNow = true, int slot = 0) {
        KeyIn q = k;
        q.since = t, q.presses = presses, q.harm = harm, q.readNow = readNow, q.gameSlot = slot;
        return KeyAgain(q, s);
    };
    const bool rows = at(0.1f, 1, false, true, 5) == KeyStep::kLanded && at(0.59f, 1) == KeyStep::kWait && at(0.6f, 1) == KeyStep::kPressAgain &&
                      at(0.6f, 1, true) == KeyStep::kWait && at(0.6f, 1, false, false) == KeyStep::kWait && at(0.6f, 1, false, true, -1) == KeyStep::kWait &&
                      at(1.0f, 2) == KeyStep::kWait && at(1.49f, 2) == KeyStep::kWait && at(1.5f, 2) == KeyStep::kGiveUp &&
                      at(1.5f, 1, true) == KeyStep::kGiveUp && at(1.2f, 2, false, true, 5) == KeyStep::kLanded && at(kNaN, 1) == KeyStep::kGiveUp;
    Report("K1", "the second key's rule: landed, wait, once at 0.6 s, never with harm or a slot not read now, given up at 1.5 s", rows,
           "landed %d, 0.59 %d, 0.6 %d, harm %d, unread %d, never read %d, 1.49 %d, 1.5 %d", static_cast<int>(at(0.1f, 1, false, true, 5)),
           static_cast<int>(at(0.59f, 1)), static_cast<int>(at(0.6f, 1)), static_cast<int>(at(0.6f, 1, true)), static_cast<int>(at(0.6f, 1, false, false)),
           static_cast<int>(at(0.6f, 1, false, true, -1)), static_cast<int>(at(1.49f, 2)), static_cast<int>(at(1.5f, 2)));
    const char* names[5] = {"the grenade launcher's shot", "the game's reload", "a scripted moment", "the slot not read", "a take asked of the game"};
    char said[320] = "";
    bool ok = true;
    for (int h = 0; h < 5; ++h) {
        Sim t = StowedRock();
        t.MoveTo(1, kNeckAt[0], kNeckAt[1], kNeckAt[2]);
        t.Run(0.1f);
        t.hand[1].squeeze = 1.0f;
        t.Step();
        t.Run(0.45f);
        t.m320 = h == 0, t.gameReload = h == 1, t.frozen = h == 2, t.slotRead = h != 3, t.pendingTake = h == 4;
        t.Run(1.2f);
        const Event* gave = t.Find(What::kNoSwitch, 1);
        ok = ok && t.Keys(5) == 1 && gave && !gave->twice;
        std::snprintf(said + std::strlen(said), sizeof(said) - std::strlen(said), "%s%s: %d keys%s", h ? "; " : "", names[h], t.Keys(5),
                      gave ? ", given up" : "");
    }
    Report("K2", "harm at 0.6 s (the grenade launcher, the game's reload, a scripted moment, a dropped gun's take asked) or the slot not read: no "
           "second key, given up at 1.5 s", ok, "%s", said);
    {
        Sim t = StowedRock();
        t.MoveTo(1, kNeckAt[0], kNeckAt[1], kNeckAt[2]);
        t.Run(0.1f);
        t.hand[1].squeeze = 1.0f;
        t.Run(0.75f);  // (the second key at 0.6 s)
        t.gameSlot = 5;
        t.Run(0.2f);
        const ArriveOut o = t.Switch(5);
        t.Run(1.0f);
        Report("K3", "the game switched after the second key: drawn, never given up", t.Keys(5) == 2 && o.arrival == Arrival::kHand && !t.Find(What::kNoSwitch),
               "keys %d, arrival %d", t.Keys(5), static_cast<int>(o.arrival));
    }
    {
        Settings q = NeckSet();
        q.keyAgain = 0.3f, q.keyGiveUp = 0.9f;
        Sim t = StowedRock();
        t.set = q;
        t.MoveTo(1, kNeckAt[0], kNeckAt[1], kNeckAt[2]);
        t.Run(0.1f);
        const float at = t.time;
        t.hand[1].squeeze = 1.0f;
        t.Run(1.2f);
        const Event* gave = t.Find(What::kNoSwitch, 1);
        Report("K4", "keyagain 0.3 0.9: the second key at 0.3 s, given up at 0.9 s", t.Keys(5) == 2 && t.keys[1].first - at > 0.28f &&
                                                                                       t.keys[1].first - at < 0.34f && gave && gave->since > 0.88f && gave->since < 0.93f,
               "the second at %.2f s, given up at %.2f s", t.keys.size() > 1 ? t.keys[1].first - at : -1.0f, gave ? gave->since : -1.0f);
    }
    {
        // The game's own slot unread (-1: no soldier, a load): a moment's gap (0.3 s) is bridged by the last read (a put-away as ever); past 0.5 s
        // nothing is decided on it (the let-go at the shoulder is today's: it falls; a squeeze at a place draws nothing, and no pick counts as the
        // put-away gun's own); read again, all as before.
        Sim gap = HeldRock();
        gap.MoveTo(1, kRS[0], kRS[1], kRS[2]);
        gap.slotRead = false;
        gap.Run(0.3f);
        gap.hand[1].squeeze = 0.0f;
        gap.Run(0.1f);
        Sim gone = HeldRock();
        gone.MoveTo(1, kRS[0], kRS[1], kRS[2]);
        gone.slotRead = false;
        gone.Run(0.6f);
        gone.everStows[1] = false;
        gone.hand[1].squeeze = 0.0f;
        gone.Run(0.1f);
        Sim draw = StowedRock();
        draw.slotRead = false;
        draw.MoveTo(0, kRS[0], kRS[1], kRS[2]);
        draw.Run(0.6f);
        const bool ownPick = PickIsOwnSlot(0, draw.st, draw.holder, draw.set);
        draw.hand[0].squeeze = 1.0f;
        draw.Run(0.3f);
        const int unread = draw.draws + draw.Keys();
        draw.slotRead = true;
        draw.hand[0].squeeze = 0.0f;
        draw.Run(0.1f);
        draw.hand[0].squeeze = 1.0f;
        draw.Run(0.2f);
        Report("K5", "the game's slot unread 0.3 s: decided on its last read; past 0.5 s: nothing decided (today's let-go, no draw, no own pick); read again: "
               "as before", gap.stows == 1 && gone.stows == 0 && gone.falls == 1 && !gone.everStows[1] && unread == 0 && !ownPick && draw.draws == 1 &&
                   draw.drawnBy == 0,
               "a gap: put away %d; unread 0.6 s: put away %d, fell %d; a squeeze then: %d, own pick %d; read again: drawn %d", gap.stows, gone.stows,
               gone.falls, unread, ownPick, draw.draws);
    }
}

// ---- U: a hand not seen ----

// The right hand last seen at p (body axes) going at v, then not seen; after `wait` s its grip squeezes (fresh). The M416 put away (Rock).
Sim Unseen(const float p[3], const float v[3], float wait, Seen lost = Seen::kNone, const Settings* set = nullptr) {
    Sim s;
    Rock(s);
    if (set)
        s.set = *set;
    s.holder = Holder::kStowed, s.trigger = -1;
    s.Run(0.1f);
    s.Move(1, p, v[0], v[1], v[2]);
    s.Run(0.05f);
    s.hand[1].seen = lost;
    if (lost == Seen::kGuessed)
        std::memcpy(s.hand[1].guess, p, sizeof(s.hand[1].guess));
    s.Run(wait);
    s.hand[1].squeeze = 1.0f;
    s.Run(0.1f);
    return s;
}

void NotSeen() {
    const float still[3] = {0.0f, 0.0f, 0.0f};
    {
        Sim s = Unseen(kNeckAt, still, 0.5f);
        const Event* e = s.Find(What::kAsked, 1);
        Report("U1", "last seen inside the neck's place, not seen 0.5 s, a fresh squeeze: a take (hidden), key 4", s.Keys(5) == 1 && e && e->unseen &&
                                                                                                                     s.draws == 0 && s.holder == Holder::kStowed,
               "keys %d, %s", s.Keys(5), e ? "asked, not seen" : "nothing");
    }
    // 10 cm to the right of the neck's place, going in 25 degrees off the way to its middle.
    const float outside[3] = {0.13f + 0.10f, kNeckAt[1], kNeckAt[2]};
    const float c25 = std::cos(25.0f * kPi_ / 180.0f), s25 = std::sin(25.0f * kPi_ / 180.0f);
    const float in25[3] = {-0.6f * c25, 0.0f, 0.6f * s25};
    const float c150 = std::cos(150.0f * kPi_ / 180.0f), s150 = std::sin(150.0f * kPi_ / 180.0f);
    const float away[3] = {-0.6f * c150, 0.0f, 0.6f * s150};
    const float slow[3] = {-0.10f * c25, 0.0f, 0.10f * s25};
    {
        Sim a = Unseen(outside, in25, 0.3f);
        Report("U2", "last seen 10 cm outside, going in at 0.6 m/s 25 degrees off: a take", a.Keys(5) == 1, "keys %d", a.Keys(5));
        Sim b = Unseen(outside, away, 0.3f);
        Report("U3", "the same going away (150 degrees): nothing", b.Keys() == 0 && b.Said() == 0, "keys %d, %d lines", b.Keys(),
               b.Said());
        Sim c = Unseen(outside, slow, 0.3f);
        Report("U4", "the same at 0.10 m/s: nothing", c.Keys() == 0 && c.Said() == 0, "keys %d", c.Keys());
        const float distant[3] = {0.13f + 0.20f, kNeckAt[1], kNeckAt[2]};
        Sim d = Unseen(distant, in25, 0.3f);
        Report("U5", "last seen 20 cm outside going in: nothing", d.Keys() == 0 && d.Said() == 0, "keys %d", d.Keys());
        Sim e = Unseen(kNeckAt, still, 1.6f);
        Report("U6", "the squeeze at 1.6 s: nothing", e.Keys() == 0 && e.Said() == 0, "keys %d", e.Keys());
        Dump(e, !(e.Keys() == 0 && e.Said() == 0));
    }
    {
        Sim s;
        Rock(s);
        s.holder = Holder::kStowed, s.trigger = -1;
        s.Run(0.1f);
        s.hand[1].squeeze = 1.0f;  // crossed in front ...
        s.Run(0.6f);               // ... held
        s.Move(1, kNeckAt);
        s.Step();                  // seen behind the neck one frame, 0.6 s after the crossing
        s.hand[1].seen = Seen::kNone;
        s.Run(0.5f);
        Report("U7", "the grip crossed 0.6 s before the last seen moment and held since: nothing", s.Keys() == 0 && s.draws == 0, "keys %d", s.Keys());
    }
    {
        // U8: the place empty; its launcher the other hand's; the game's weapon lying; its gun unknown: refused, nothing drawn.
        char said[256] = "";
        bool ok = true;
        for (int k = 0; k < 4; ++k) {
            Sim s;
            Rock(s);
            if (k == 0)
                s.slots[5] = SlotIn{};
            if (k == 3)
                s.slots[5] = {true, true, false};  // (a launcher the hands don't know)
            s.holder = k == 1 ? Holder::kHeld : k == 2 ? Holder::kLoose : Holder::kStowed;
            s.trigger = k == 1 ? 0 : -1;
            s.gameSlot = k == 1 ? 5 : 0;
            if (k == 1)
                s.armed[0] = true, s.hand[0].squeeze = 1.0f;
            s.Run(0.1f);
            s.Move(1, kNeckAt);
            s.Run(0.05f);
            const float at = s.time;
            s.hand[1].seen = Seen::kNone;
            s.Run(0.3f);
            s.hand[1].squeeze = 1.0f;
            s.Run(0.6f);
            const Event* e = s.Find(What::kRefused, 1);
            const Why want = k == 0 ? Why::kNothing : k == 1 ? Why::kOtherHolds : k == 2 ? Why::kLying : Why::kUnknown;
            const int pulses = s.Pulses(1, kDoubleAmp, at);
            const bool good = e && e->why == want && e->unseen && s.Keys() == 0 && s.draws == 0 && pulses == (k == 1 ? 0 : 2);
            ok = ok && good;
            std::snprintf(said + std::strlen(said), sizeof(said) - std::strlen(said), "%s%d: %s, %d pulses", k ? "; " : "", k, e ? "refused" : "nothing", pulses);
        }
        Report("U8", "the place empty, its launcher in the other hand (silent), the game's weapon lying, its gun unknown: refused, nothing drawn", ok, "%s",
               said);
    }
    {
        Sim s;
        Rock(s);
        s.holder = Holder::kStowed, s.trigger = -1;
        s.hand[1].magazine = true;
        s.Run(0.1f);
        s.Move(1, kNeckAt);
        s.Run(0.05f);
        s.hand[1].seen = Seen::kNone;
        s.Run(0.3f);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.5f);
        Report("U9", "the hand holds a magazine: nothing", s.Keys() == 0 && s.draws == 0 && s.Pulses(1, kDoubleAmp) == 0, "keys %d", s.Keys());
        Sim t = HeldRock();
        t.MoveTo(0, kFront[0], kFront[1], kFront[2]);
        t.trigger = 1;
        t.Move(0, kNeckAt);
        t.Run(0.05f);
        t.hand[0].seen = Seen::kNone;
        t.Run(0.3f);
        t.hand[0].squeeze = 1.0f;
        t.Run(0.5f);
        const Event* e = t.Find(What::kRefused, 0);
        Report("U10", "the other hand holds the game's weapon: refused, silent", e && e->why == Why::kOtherHolds && t.Keys() == 0 && t.Pulses(0) == 0,
               "%s", e ? "refused" : "nothing");
    }
    {
        // The runtime's guess: 35 cm from the last seen place / 25 cm away and inside the place grown.
        const float last[3] = {0.10f, kNeckAt[1], kNeckAt[2]};
        const float in[3] = {-0.3f, 0.0f, 0.0f};
        Sim a;
        Rock(a);
        a.holder = Holder::kStowed, a.trigger = -1;
        a.Run(0.1f);
        a.Move(1, last, in[0], in[1], in[2]);
        a.Run(0.05f);
        a.hand[1].seen = Seen::kGuessed;
        a.MoveTo(1, last[0], last[1], last[2]);
        a.hand[1].guess[0] = last[0] - 0.35f, a.hand[1].guess[1] = last[1], a.hand[1].guess[2] = last[2];
        a.Run(0.2f);
        a.hand[1].squeeze = 1.0f;
        a.Run(0.3f);
        Sim b;
        Rock(b);
        b.holder = Holder::kStowed, b.trigger = -1;
        b.Run(0.1f);
        b.Move(1, last, in[0], in[1], in[2]);
        b.Run(0.05f);
        b.hand[1].seen = Seen::kGuessed;
        b.hand[1].guess[0] = last[0] - 0.25f, b.hand[1].guess[1] = last[1], b.hand[1].guess[2] = last[2];
        b.Run(0.2f);
        b.hand[1].squeeze = 1.0f;
        b.Run(0.3f);
        b.gameSlot = 5;
        b.Run(0.2f);
        const ArriveOut o = b.Switch(5);
        Report("U11", "the runtime's guess 35 cm from the last seen place: nothing; 25 cm and inside the place grown: a take, in the hand (a place again)",
               a.Keys() == 0 && b.Keys(5) == 1 && o.arrival == Arrival::kHand && o.side == 1, "35 cm: keys %d; 25 cm: keys %d, arrival %d", a.Keys(), b.Keys(5),
               static_cast<int>(o.arrival));
    }
    {
        // Holding the SMAW, last seen going into the neck's place; the grip opens at 2.0 s / 0.3 s / 4.1 s.
        const float edge[3] = {0.13f + 0.05f, kNeckAt[1], kNeckAt[2]};
        const float in[3] = {-0.6f, 0.0f, 0.0f};
        const float opens[3] = {2.0f, 0.3f, 4.1f};
        int stowed[3] = {}, held[3] = {}, holds[3] = {}, fell[3] = {};
        for (int k = 0; k < 3; ++k) {
            Sim s;
            Rock(s);
            s.gameSlot = 5, s.holder = Holder::kHeld, s.trigger = 1, s.armed[1] = true;
            s.hand[1].squeeze = 1.0f;
            s.Run(0.1f);
            s.Move(1, edge, in[0], in[1], in[2]);
            s.Run(0.05f);
            s.hand[1].seen = Seen::kNone;
            s.Run(opens[k]);
            s.hand[1].squeeze = 0.0f;
            s.Run(0.2f);
            stowed[k] = s.stows, held[k] = s.holder == Holder::kHeld, holds[k] = s.everHold[1], fell[k] = s.falls;
        }
        Report("U12", "holding the SMAW, last seen going into the neck's place, the grip opens at 2.0 s: put away", stowed[0] == 1 && fell[0] == 0,
               "put away %d, fell %d", stowed[0], fell[0]);
        Report("U12b", "the same opening at 0.3 s (within input's held place): put away, never dropped", stowed[1] == 1 && fell[1] == 0, "put away %d, fell %d",
               stowed[1], fell[1]);
        Report("U13", "the same at 4.1 s: nothing: still held, not dropped", stowed[2] == 0 && held[2] && holds[2] && fell[2] == 0, "put away %d, held %d, "
               "fell %d", stowed[2], held[2], fell[2]);
    }
    {
        // The same hand kept not seen 6 s with its grip closed, then seen in front: the gun drawn nowhere every one of those frames (past the put-away's
        // 4 s too: input's held place long gone, the game would draw its own at the camera), in the hand again from the frame it's seen; never put away
        // or dropped. Going to the lap instead: never drawn nowhere.
        const float edge[3] = {0.13f + 0.05f, kNeckAt[1], kNeckAt[2]};
        const float in[3] = {-0.6f, 0.0f, 0.0f};
        const float lap[3] = {0.10f, -0.35f, 0.30f};
        const float down[3] = {0.0f, -0.6f, 0.0f};
        int hidden[2] = {}, after[2] = {}, put[2] = {}, fell[2] = {};
        bool held[2] = {};
        const int frames = static_cast<int>(std::lround(6.0f / kDt));
        for (int k = 0; k < 2; ++k) {
            Sim s;
            Rock(s);
            s.gameSlot = 5, s.holder = Holder::kHeld, s.trigger = 1, s.armed[1] = true;
            s.hand[1].squeeze = 1.0f;
            s.Run(0.1f);
            s.Move(1, k == 0 ? edge : lap, k == 0 ? in[0] : down[0], k == 0 ? in[1] : down[1], k == 0 ? in[2] : down[2]);
            s.Run(0.05f);
            s.hand[1].seen = Seen::kNone;
            s.Run(6.0f);
            hidden[k] = s.unseenAtPlaceFrames;
            s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
            s.hand[1].seen = Seen::kSeen;
            s.Run(0.2f);
            after[k] = s.unseenAtPlaceFrames - hidden[k], put[k] = s.stows, fell[k] = s.falls, held[k] = s.holder == Holder::kHeld && s.trigger == 1;
        }
        Report("U13b", "held not seen 6 s at the neck's place: drawn nowhere all the while (past the put-away's 4 s), in the hand once seen; the lap: never",
               hidden[0] >= frames - 1 && after[0] == 0 && held[0] && put[0] == 0 && fell[0] == 0 && hidden[1] == 0 && held[1] && fell[1] == 0,
               "the neck: %d of %d frames drawn nowhere, %d after it was seen, held %d, put away %d, fell %d; the lap: %d frames, held %d, fell %d", hidden[0],
               frames, after[0], held[0] ? 1 : 0, put[0], fell[0], hidden[1], held[1] ? 1 : 0, fell[1]);
    }
    {
        const float lap[3] = {0.10f, -0.35f, 0.30f};
        const float down[3] = {0.0f, -0.6f, 0.0f};
        const float opens[3] = {2.0f, 0.3f, 0.3f};
        int stowed[3] = {}, held[3] = {}, fell[3] = {};
        for (int k = 0; k < 3; ++k) {
            Sim s;
            Rock(s);
            s.set.unseenDrop = k == 2;
            s.gameSlot = 5, s.holder = Holder::kHeld, s.trigger = 1, s.armed[1] = true;
            s.hand[1].squeeze = 1.0f;
            s.Run(0.1f);
            s.Move(1, lap, down[0], down[1], down[2]);
            s.Run(0.05f);
            s.hand[1].seen = Seen::kNone;
            s.Run(opens[k]);
            s.hand[1].squeeze = 0.0f;
            s.Run(0.2f);
            stowed[k] = s.stows, held[k] = s.holder == Holder::kHeld, fell[k] = s.falls;
        }
        Report("U14", "holding the SMAW, last seen going down to the lap, the grip opens at 2.0 s: not put away, not dropped", stowed[0] == 0 && held[0] &&
                                                                                                                                fell[0] == 0,
               "put away %d, held %d, fell %d", stowed[0], held[0], fell[0]);
        Report("U14b", "the same opening at 0.3 s (input still holds the hand's place): not dropped; unseendrop 1: today's drop", stowed[1] == 0 && held[1] &&
                                                                                                                                     fell[1] == 0 && fell[2] == 1,
               "held %d, fell %d; unseendrop 1: fell %d", held[1], fell[1], fell[2]);
    }
    {
        // A hidden take, case B: the gun comes while the hand has no place; then a place again at 0.8 s / the grip opens first.
        Sim a = Unseen(kNeckAt, still, 0.2f);
        a.Run(0.2f);
        a.gameSlot = 5;
        a.Run(0.1f);
        const ArriveOut oa = a.Switch(5);
        a.Run(0.15f);
        const bool waited = a.holder == Holder::kStowed && a.draws == 0;
        a.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        a.hand[1].seen = Seen::kSeen;
        a.Run(0.1f);
        const Event* d = a.Find(What::kDrawn, 1);
        // Case A: the SMAW put away behind the neck, taken back unseen.
        Sim b;
        Rock(b);
        b.gameSlot = 5, b.holder = Holder::kStowed, b.trigger = -1;
        b.Run(0.1f);
        b.Move(1, kNeckAt);
        b.Run(0.05f);
        b.hand[1].seen = Seen::kNone;
        b.Run(0.2f);
        b.hand[1].squeeze = 1.0f;
        b.Run(0.3f);
        const bool hidden = b.holder == Holder::kStowed && b.Count(What::kAsked, 1) == 1 && b.Keys() == 0;
        b.hand[1].squeeze = 0.0f;  // opens before it's seen
        b.Run(0.2f);
        const Event* off = b.Find(What::kCalledOff, 1);
        Report("U15", "a hidden take: in the hand once it has a place again; the grip opening first calls it off (at its place)",
               oa.arrival == Arrival::kAtPlace && waited && a.draws == 1 && a.drawnBy == 1 && d && d->unseen && hidden && off && off->why == Why::kOpened &&
                   b.holder == Holder::kStowed && b.draws == 0,
               "case B: kept put away %d, drawn %d; case A: hidden %d, %s", waited, a.draws, hidden, off ? "called off" : "not called off");
        Sim c = Unseen(kNeckAt, still, 0.2f);
        c.Run(0.2f);
        c.hand[1].squeeze = 0.0f;
        c.Run(0.1f);
        c.gameSlot = 5;
        const ArriveOut oc = c.Switch(5);
        Report("U15b", "case B with the grip opened before the gun came: it comes put away", oc.arrival == Arrival::kAtPlace && oc.event.what == What::kCameAway &&
                                                                                                 c.holder == Holder::kStowed,
               "arrival %d", static_cast<int>(oc.arrival));
        // No place again for 4 s: case A (the SMAW put away behind the neck), and case B with its switch come while the hand had no place.
        Sim e;
        Rock(e);
        e.gameSlot = 5, e.holder = Holder::kStowed, e.trigger = -1;
        e.Run(0.1f);
        e.Move(1, kNeckAt);
        e.Run(0.05f);
        e.hand[1].seen = Seen::kNone;
        e.Run(0.2f);
        e.hand[1].squeeze = 1.0f;
        e.Run(4.5f);
        const Event* late = e.Find(What::kCalledOff, 1);
        Sim g = Unseen(kNeckAt, still, 0.2f);
        g.Run(0.2f);
        g.gameSlot = 5;
        g.Run(0.1f);
        g.Switch(5);
        g.Run(4.5f);
        const Event* lateB = g.Find(What::kCalledOff, 1);
        Report("U15c", "a hidden take with no place again for 4 s: called off, the gun at its place (case A; case B with its switch come)",
               late && late->why == Why::kLate && e.holder == Holder::kStowed && e.draws == 0 && lateB && lateB->why == Why::kLate &&
                   g.holder == Holder::kStowed && g.draws == 0,
               "case A %s after %.2f s; case B %s", late ? "called off" : "not called off", late ? late->since : -1.0f, lateB ? "called off" : "not called off");
        Dump(e, !late);
        Dump(g, !lateB);
    }
    {
        char said[128] = "";
        bool ok = true;
        for (int k = 0; k < 2; ++k) {
            Sim s;
            Rock(s);
            s.holder = Holder::kStowed, s.trigger = -1;
            s.Run(0.1f);
            s.Move(1, kNeckAt);
            s.Run(0.05f);
            s.hand[1].seen = Seen::kNone;
            s.frozen = k == 0, s.gameReload = k == 1;
            s.Run(0.3f);
            s.hand[1].squeeze = 1.0f;
            s.Run(0.5f);
            ok = ok && s.Keys() == 0 && s.draws == 0 && s.Pulses(1, kDoubleAmp) == 0;
            std::snprintf(said + std::strlen(said), sizeof(said) - std::strlen(said), "%s%s: keys %d", k ? "; " : "", k ? "the game's reload" : "scripted",
                          s.Keys());
        }
        Report("U16", "a scripted moment; the game's reload: nothing", ok, "%s", said);
    }
    {
        Sim s;
        Rock(s);
        s.holder = Holder::kStowed, s.trigger = -1;
        s.Run(0.1f);
        s.Move(1, kNeckAt);
        s.Run(0.05f);
        s.hand[1].seen = Seen::kNone;
        s.Run(0.2f);
        s.hand[0].squeeze = 1.0f;  // the other hand, in front
        s.Run(0.5f);
        Report("U17", "the other hand squeezes in front while this one isn't seen: nothing for this one's place", s.Keys() == 0 && s.draws == 0 && s.Said() == 0,
               "keys %d, %d lines", s.Keys(), s.Said());
        Dump(s, s.Said() != 0);
    }
    {
        // Flickering: seen one frame at a time, the record follows the last seen frame.
        Sim a;
        Rock(a);
        a.holder = Holder::kStowed, a.trigger = -1;
        a.Run(0.1f);
        for (int k = 0; k < 6; ++k) {
            a.Move(1, (k % 2) ? kNeckAt : kFront);
            a.hand[1].seen = Seen::kSeen;
            a.Step();
            a.hand[1].seen = Seen::kNone;
            a.Run(0.05f);
        }
        a.hand[1].squeeze = 1.0f;  // last seen behind the neck
        a.Run(0.3f);
        Sim b;
        Rock(b);
        b.holder = Holder::kStowed, b.trigger = -1;
        b.Run(0.1f);
        for (int k = 0; k < 6; ++k) {
            b.Move(1, (k % 2) ? kFront : kNeckAt);
            b.hand[1].seen = Seen::kSeen;
            b.Step();
            b.hand[1].seen = Seen::kNone;
            b.Run(0.05f);
        }
        b.hand[1].squeeze = 1.0f;  // last seen in front
        b.Run(0.3f);
        Sim c;
        Rock(c);
        c.holder = Holder::kStowed, c.trigger = -1;
        c.Run(0.1f);
        c.Move(1, kNeckAt);
        c.Step();
        c.hand[1].seen = Seen::kNone;
        c.Run(0.05f);
        c.hand[1].seen = Seen::kSeen;
        c.hand[1].squeeze = 1.0f;  // on a seen frame inside the place: judged as seen
        c.Step();
        c.hand[1].seen = Seen::kNone;
        c.Run(0.3f);
        const Event* ec = c.Find(What::kAsked, 1);
        Report("U18", "flickering: the record follows the last seen frame; a squeeze on a seen frame is judged as seen", a.Keys(5) == 1 && b.Keys() == 0 &&
                                                                                                                             ec && !ec->unseen,
               "last behind the neck: keys %d; last in front: keys %d; on a seen frame: %s", a.Keys(5), b.Keys(),
               ec ? (ec->unseen ? "as not seen" : "as seen") : "nothing");
    }
    {
        Sim s;
        Rock(s);
        s.holder = Holder::kStowed, s.trigger = -1;
        s.Run(0.1f);
        s.Move(1, kNeckAt);
        s.Run(0.05f);
        s.hand[1].seen = Seen::kNone;
        s.Run(0.3f);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.hand[1].seen = Seen::kSeen;  // seen again in front
        s.Run(0.1f);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.4f);
        Report("U19", "a seen hand: only where it is counts, never its record", s.Keys() == 0 && s.draws == 0, "keys %d", s.Keys());
    }
    {
        const float toBelly[3] = {0.0f, -0.6f, 0.0f};
        const float aboveBelly[3] = {kBellyAt[0], kBellyAt[1] + 0.20f, kBellyAt[2]};
        Sim a = Unseen(aboveBelly, toBelly, 0.3f);
        Report("U20", "last seen going into the belly, a squeeze not seen: nothing (the belly is never presumed)", a.Keys() == 0 && a.Said() == 0, "keys %d",
               a.Keys());
        const float high[3] = {kNeckAt[0], 0.17f + 0.06f, kNeckAt[2]};  // (the neck's top: its middle 0.07 and its half-height 0.10)
        const float downIn[3] = {0.0f, -0.6f, 0.0f};
        Sim b = Unseen(high, downIn, 0.3f);
        Report("U21", "last seen 6 cm above the neck's place's top, going down into it: nothing", b.Keys() == 0 && b.Said() == 0, "keys %d", b.Keys());
    }
    {
        // The reach over the right shoulder: last seen inside it at 0.6 m/s going into the neck's place, lost, a squeeze at 0.3 s: the launcher or
        // nothing, never the M416 (at the shoulder, put away).
        const float at[3] = {0.18f, -0.02f, -0.03f};  // (inside the shoulder, toward the neck)
        float way[3] = {kNeckAt[0] - at[0], kNeckAt[1] - at[1], kNeckAt[2] - at[2]};
        const float n = std::sqrt(way[0] * way[0] + way[1] * way[1] + way[2] * way[2]);
        for (float& v : way)
            v *= 0.6f / n;
        Sim a = Unseen(at, way, 0.3f);
        Settings both = NeckSet();
        both.unseenShoulders = true;
        Sim b = Unseen(at, way, 0.3f, Seen::kNone, &both);
        Report("U22", "the reach over the shoulder to the neck, lost: the SMAW asked or nothing, never the M416 (with the shoulders presumed too)",
               a.draws == 0 && a.drawSides == 0 && a.Keys(0) == 0 && b.draws == 0 && b.drawSides == 0 && b.Keys(0) == 0 && a.Keys(5) == 1 && b.Keys(5) == 1,
               "the shoulders not presumed: keys for the SMAW %d, the M416 drawn %d; presumed: %d, %d", a.Keys(5), a.draws, b.Keys(5), b.draws);
        // The same from the shoulder's very middle (no way to its middle from there), the shoulders presumed: going into the neck's place still wins.
        // At 1.5 m/s, the reach test's own speed over the shoulder (its paths: 5 to 11 cm a 0.05 s step on the way to the neck).
        float mid[3] = {kNeckAt[0] - kRS[0], kNeckAt[1] - kRS[1], kNeckAt[2] - kRS[2]};
        const float m = std::sqrt(mid[0] * mid[0] + mid[1] * mid[1] + mid[2] * mid[2]);
        for (float& v : mid)
            v *= 1.5f / m;
        Sim c = Unseen(kRS, mid, 0.3f, Seen::kNone, &both);
        float off = 0.0f;
        const Place p = Presume(kRS, mid, both, &off);
        Report("U22b", "from the right shoulder's very middle going into the neck's place, the shoulders presumed: the neck's, never the shoulder's gun",
               p == Place::kNeck && c.draws == 0 && c.drawSides == 0 && c.Keys(5) == 1, "presumed %s; the M416 drawn %d, keys for the SMAW %d", Name(p),
               c.draws, c.Keys(5));
    }
    {
        // The stand-in's right (no controller of its own: own false) never held untracked, never judged.
        Sim s;
        Rock(s);
        s.gameSlot = 5, s.holder = Holder::kHeld, s.trigger = 1, s.armed[1] = true, s.hand[1].squeeze = 1.0f, s.hand[1].own = false;
        s.Run(0.1f);
        s.Move(1, kNeckAt);
        s.Run(0.05f);
        s.hand[1].seen = Seen::kNone;
        s.Run(0.3f);
        s.hand[1].squeeze = 0.0f;
        s.Run(0.3f);
        Report("U23", "the stand-in's hand not seen: never held untracked, never put away", !s.everHold[1] && s.stows == 0 && s.stowSides == 0 && s.Said() == 0,
               "held untracked %d, put away %d", s.everHold[1], s.stows);
    }
    {
        // Holding the SMAW, not seen behind the neck, opening while the left holds a rocket: refused, kept, said and felt once; with the left on its
        // front grip: kept, silent.
        int kept[2] = {}, lines[2] = {}, pulses[2] = {}, stowed[2] = {};
        for (int k = 0; k < 2; ++k) {
            Sim s;
            Rock(s);
            s.gameSlot = 5, s.holder = Holder::kHeld, s.trigger = 1, s.armed[1] = true, s.hand[1].squeeze = 1.0f;
            s.hand[0].rocket = k == 0, s.hand[0].onGun = k == 1;
            s.Run(0.1f);
            s.Move(1, kNeckAt);
            s.Run(0.05f);
            s.hand[1].seen = Seen::kNone;
            s.Run(0.3f);
            const float at = s.time;
            s.hand[1].squeeze = 0.0f;
            s.Run(1.0f);  // (the grip stays open)
            kept[k] = s.holder == Holder::kHeld, lines[k] = s.Count(What::kStowRefused, 1), pulses[k] = s.Pulses(1, kDoubleAmp, at), stowed[k] = s.stows;
            Dump(s, !(kept[k] && lines[k] == 1));
        }
        Report("U24", "a hand not seen opening at its place while the other holds a rocket: kept, said and felt once; the other on its front grip: kept, "
               "silent", kept[0] && lines[0] == 1 && pulses[0] == 2 && stowed[0] == 0 && kept[1] && lines[1] == 1 && pulses[1] == 0 && stowed[1] == 0,
               "the rocket: kept %d, %d lines, %d pulses; the front grip: kept %d, %d lines, %d pulses", kept[0], lines[0], pulses[0], kept[1], lines[1],
               pulses[1]);
    }
    {
        // A grip held through free hands asleep (a cutscene) is no fresh squeeze when they wake.
        Sim s = StowedRock();
        s.MoveTo(1, kRS[0], kRS[1], kRS[2]);
        s.Run(0.1f);
        s.on = false;
        s.Run(0.2f);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.3f);
        s.on = true;
        s.Run(0.5f);
        const int held = s.draws;
        s.hand[1].squeeze = 0.0f;
        s.Run(0.1f);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.2f);
        Report("U25", "a grip held through free hands asleep draws nothing on waking; squeezed again, it does", held == 0 && s.draws == 1,
               "held through %d, squeezed again %d", held, s.draws);
    }
}

// ---- B: head turns and the body's turn (the pouch's body); no head tracking ----

void HeadTurns() {
    pouch::Clear();
    // The head 7.5 cm above and 8 cm in front of the neck point, looking at -z; turned by yaw (radians, left positive).
    const auto head = [](float yaw) {
        XrPosef p{};
        p.orientation.x = 0.0f, p.orientation.y = std::sin(yaw / 2.0f), p.orientation.z = 0.0f, p.orientation.w = std::cos(yaw / 2.0f);
        p.position.x = 0.0f, p.position.y = 1.275f, p.position.z = -0.08f;
        return p;
    };
    const auto body = [&](float yaw, float dt, float neck[3], float& bodyYaw) {
        pouch::Frame f;
        f.dt = dt;
        f.head = head(yaw);
        f.left = false;
        pouch::Update(f);
        return pouch::BodyFrame(neck, bodyYaw);
    };
    // The right hand fixed in the room at the right shoulder's middle as the body faced at the start.
    float neck[3] = {}, yaw = 0.0f;
    body(0.0f, kDt, neck, yaw);
    body(0.0f, kDt, neck, yaw);
    Sim probe;
    std::memcpy(probe.neck, neck, sizeof(probe.neck));
    probe.yaw = yaw;
    float handRoom[3];
    probe.ToRoom(kRS, handRoom);
    const Settings s = NeckSet();
    float most = 0.0f;
    bool always = true;
    float t = 0.0f;
    for (int k = 0; k < 72; ++k, t += kDt) {
        // 25 degrees left and back in a second: quick turns (100 degrees a second), held a moment each way.
        const float u = t < 0.25f ? t / 0.25f : t < 0.5f ? 1.0f : t < 0.75f ? 1.0f - (t - 0.5f) / 0.25f : 0.0f;
        const float y = 25.0f * kPi_ / 180.0f * u;
        body(y, kDt, neck, yaw);
        float from[3];
        FromNeck(handRoom, neck, yaw, from);
        float z = 0.0f;
        always = always && PlaceAt(from, s, &z) == Place::kRightShoulder;
        most = std::max(most, z);
    }
    Report("B1", "small head turns (25 degrees and back) leave the places where they are", always && most < 0.6f,
           "the hand fixed in the room stays at the right shoulder, at most %.2f of its size (the neck point turns with the head: a few cm)", most);
    // The head turned 90 degrees over a second (not a recentre: no jump over 45 degrees in a frame) and held 3 s: the body follows, past 35 degrees
    // at once, then settling towards the head.
    for (int k = 1; k <= 72; ++k)
        body(90.0f * kPi_ / 180.0f * k / 72.0f, kDt, neck, yaw);
    for (int k = 0; k < 3 * 72; ++k)
        body(90.0f * kPi_ / 180.0f, kDt, neck, yaw);
    float from[3];
    FromNeck(handRoom, neck, yaw, from);
    const Place old = PlaceAt(from, s);
    Sim turned;
    std::memcpy(turned.neck, neck, sizeof(turned.neck));
    turned.yaw = yaw;
    float newRoom[3], fromNew[3];
    turned.ToRoom(kRS, newRoom);
    FromNeck(newRoom, neck, yaw, fromNew);
    Report("B2", "the places follow once the body turns", old != Place::kRightShoulder && PlaceAt(fromNew, s) == Place::kRightShoulder &&
                                                               yaw > 55.0f * kPi_ / 180.0f,
           "the body turned %.0f degrees after 3 s; the old room point at %s, the turned one at %s", yaw * 180.0f / kPi_, Name(old), Name(PlaceAt(fromNew, s)));
    // No head tracking: no draw, no put-away.
    Sim d = StowedRock();
    d.body = false;
    d.MoveTo(1, kRS[0], kRS[1], kRS[2]);
    d.Run(0.1f);
    d.hand[1].squeeze = 1.0f;
    d.Run(0.5f);
    Sim p = HeldRock();
    p.body = false;
    p.MoveTo(1, kRS[0], kRS[1], kRS[2]);
    p.Run(0.1f);
    p.hand[1].squeeze = 0.0f;
    p.Run(0.1f);
    Sim u;  // a hand not seen opening at its presumed place, the head not tracked
    Rock(u);
    u.gameSlot = 5, u.holder = Holder::kHeld, u.trigger = 1, u.armed[1] = true, u.hand[1].squeeze = 1.0f;
    u.Run(0.1f);
    u.Move(1, kNeckAt);
    u.Run(0.05f);
    u.hand[1].seen = Seen::kNone;
    u.body = false;
    u.Run(0.3f);
    u.hand[1].squeeze = 0.0f;
    u.Run(0.2f);
    Report("B3", "no head tracking: no draw and no put-away (a hand not seen keeps its gun)", d.draws == 0 && d.Keys() == 0 && p.stows == 0 &&
                                                                                                  !p.everStows[1] && u.stows == 0 && u.falls == 0 &&
                                                                                                  u.holder == Holder::kHeld,
           "drawn %d, keys %d, put away %d and %d, fell %d", d.draws, d.Keys(), p.stows, u.stows, u.falls);
}

// ---- W: a draw then a fast let-go ----

void FastLetGo() {
    // At the place (2026-11-19): a let-go there is never a throw, whatever the hand's own speed.
    const float speeds[4] = {2.5f, 0.0f, 1.4f, 2.4f};
    int stowed[4] = {}, fell[4] = {}, drawn[4] = {};
    for (int k = 0; k < 4; ++k) {
        Sim s = StowedRock();
        s.MoveTo(1, kRS[0], kRS[1], kRS[2]);
        s.Run(0.1f);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.05f);
        drawn[k] = s.draws;
        s.hand[1].vel[0] = speeds[k];  // whipped (still at the place for these frames: its own speed only)
        s.hand[1].squeeze = 0.0f;
        s.Run(0.1f);
        stowed[k] = s.stows, fell[k] = s.falls;
    }
    Report("W1", "a draw then at once a let-go at the place at 2.5 m/s: put away again (never a throw at a place, 2026-11-19)",
           drawn[0] == 1 && stowed[0] == 1 && fell[0] == 0, "drawn %d, put away %d, fell %d", drawn[0], stowed[0], fell[0]);
    Report("W2", "a draw then at once a let-go at rest at the place: put away again", drawn[1] == 1 && stowed[1] == 1 && fell[1] == 0, "put away %d",
           stowed[1]);
    // Drawn and whipped out of the place for real (the hand moving out to the side), let go 5 cm past the put-away's egg, still within the grace: at 1.4
    // m/s away, put away (the grace); at 2.4 and 3.5 m/s away, thrown (it falls).
    const float away[3] = {1.4f, 2.4f, 3.5f};
    int stowedOut[3] = {}, fellOut[3] = {};
    const Egg e = Settings{}.shoulder;
    for (int k = 0; k < 3; ++k) {
        Sim s = StowedRock();
        s.MoveTo(1, kRS[0], kRS[1], kRS[2]);
        s.Run(0.1f);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.05f);
        const float edge = e.mid[0] + e.half[0] + s.set.stowMore + 0.05f;
        while (s.hand[1].pos[0] < edge) {
            s.MoveTo(1, std::min(edge, s.hand[1].pos[0] + away[k] * kDt), kRS[1], kRS[2], away[k], 0.0f, 0.0f);
            s.Step();
        }
        s.hand[1].squeeze = 0.0f;
        s.Run(0.1f);
        stowedOut[k] = s.stows, fellOut[k] = s.falls;
    }
    Report("W3", "drawn, whipped out to the side and let go 5 cm past the put-away's egg: at 1.4 m/s put away (the grace); at 2.4 and 3.5 m/s thrown",
           stowedOut[0] == 1 && fellOut[0] == 0 && stowedOut[1] == 0 && fellOut[1] == 1 && stowedOut[2] == 0 && fellOut[2] == 1,
           "1.4: put away %d, fell %d; 2.4: %d, %d; 3.5: %d, %d", stowedOut[0], fellOut[0], stowedOut[1], fellOut[1], stowedOut[2], fellOut[2]);
}

// ---- R: asked of the grasp again till it's done; drawn nowhere meanwhile ----

void Asked() {
    {
        Sim s = StowedRock();
        s.MoveTo(1, kRS[0], kRS[1], kRS[2]);
        s.Run(0.1f);
        const float at = s.time;
        s.deaf = 2;  // (the grasp doesn't take it these frames)
        s.hand[1].squeeze = 1.0f;
        s.Run(0.2f);
        const float late = s.drawnAt - at;
        Sim n = StowedRock();
        n.MoveTo(1, kRS[0], kRS[1], kRS[2]);
        n.Run(0.1f);
        n.deaf = 40;  // (never within kAskMost)
        n.hand[1].squeeze = 1.0f;
        n.Run(0.8f);
        Report("R1", "a draw the grasp doesn't take at once is asked again and taken, said and clicked once; never taken, it lapses after 0.25 s with a line",
               s.draws == 1 && late > 0.02f && late < 0.06f && s.Count(What::kDrawn, 1) == 1 && s.Pulses(1, kClickAmp, at) == 1 && n.draws == 0 &&
                   n.drawSides >= 15 && n.drawSides <= 20 && n.Said() == 1 && n.Count(What::kLapsed, 1) == 1 && n.Pulses(1, kDoubleAmp) == 0 &&
                   n.holder == Holder::kStowed,
               "taken %.3f s after the squeeze, %d lines, %d clicks; never taken: asked %d frames, %d lines", late, s.Count(What::kDrawn, 1),
               s.Pulses(1, kClickAmp, at), n.drawSides, n.Said());
    }
    {
        const float edge[3] = {0.13f + 0.05f, kNeckAt[1], kNeckAt[2]};
        Sim s;
        Rock(s);
        s.gameSlot = 5, s.holder = Holder::kHeld, s.trigger = 1, s.armed[1] = true, s.hand[1].squeeze = 1.0f;
        s.Run(0.1f);
        s.Move(1, edge, -0.6f, 0.0f, 0.0f);
        s.Run(0.05f);
        s.hand[1].seen = Seen::kNone;
        s.Run(0.3f);
        const int atPlace = s.unseenAtPlaceFrames;
        s.deaf = 3;
        s.hand[1].squeeze = 0.0f;
        s.Run(0.2f);
        Sim lap;
        Rock(lap);
        lap.gameSlot = 5, lap.holder = Holder::kHeld, lap.trigger = 1, lap.armed[1] = true, lap.hand[1].squeeze = 1.0f;
        lap.Run(0.1f);
        lap.MoveTo(1, 0.10f, -0.35f, 0.30f, 0.0f, -0.6f, 0.0f);
        lap.Run(0.05f);
        lap.hand[1].seen = Seen::kNone;
        lap.Run(0.5f);
        Report("R2", "a put-away by a hand not seen asked again till the grasp takes it; the gun drawn nowhere while that hand is presumed at a place",
               s.stows == 1 && s.Count(What::kStowed, 1) == 1 && atPlace >= 20 && lap.unseenAtPlaceFrames == 0 && lap.everHold[1],
               "put away %d (%d lines); drawn nowhere %d frames before the opening; going to the lap: %d frames", s.stows, s.Count(What::kStowed, 1),
               atPlace, lap.unseenAtPlaceFrames);
    }
    {
        Sim s = StowedRock();
        s.MoveTo(1, kNeckAt[0], kNeckAt[1], kNeckAt[2]);
        s.Run(0.1f);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.4f);
        const int waiting = s.drawWaitsFrames;
        s.gameSlot = 5;
        s.Switch(5);
        const int before = s.drawWaitsFrames;
        s.Run(0.3f);
        Report("R3", "a case-B draw waits (drawn nowhere) from its key till its gun is in the hand", waiting >= 25 && s.drawWaitsFrames == before &&
                                                                                                       s.holder == Holder::kHeld,
               "%d frames waiting, %d after the gun came", waiting, s.drawWaitsFrames - before);
    }
    {
        // A hidden take of the SMAW put away (case A), then the game switches by itself to the M416 before the hand is seen; seen again with the grip
        // held, the SMAW is no longer the game's weapon: nothing drawn, and the wait ends with the take's window (said with log 1), so it never
        // refuses a later draw: the left then draws the M416 from the right shoulder.
        Sim s;
        Rock(s);
        s.gameSlot = 5, s.holder = Holder::kStowed, s.trigger = -1;
        s.Run(0.1f);
        s.Move(1, kNeckAt);
        s.Run(0.05f);
        s.hand[1].seen = Seen::kNone;
        s.Run(0.2f);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.2f);
        const bool asked = s.Count(What::kAsked, 1) == 1 && s.st.wait.on;
        s.Switch(0);  // (the game's own switch, nobody's)
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.hand[1].seen = Seen::kSeen;  // seen again, the grip still held
        s.Run(1.0f);
        const bool stillWaits = s.st.wait.on && s.draws == 0;
        s.Run(3.0f);
        const Event* end = s.Find(What::kCalledOff, 1);
        const bool ended = !s.st.wait.on && s.draws == 0 && end && end->why == Why::kMoment && end->quiet;
        s.MoveTo(0, kRS[0], kRS[1], kRS[2]);
        s.Run(0.1f);
        s.hand[0].squeeze = 1.0f;
        s.Run(0.2f);
        Report("R4", "a hidden take whose gun stopped being the game's weapon: never drawn, its wait ends with the take's window, a later draw answered",
               asked && stillWaits && ended && s.draws == 1 && s.drawnBy == 0 && s.holder == Holder::kHeld && s.trigger == 0,
               "asked %d, waiting at 1 s %d, ended by 4 s %d, then drawn %d by %d", asked, stillWaits, ended, s.draws, s.drawnBy);
    }
}

// ---- T: the tick ----

int Ticks(const Sim& s, int side) {
    int n = 0;
    for (const Buzzed& b : s.buzzes)
        n += b.side == side && std::fabs(b.amp - kTickAmp) < 1e-4f && std::fabs(b.seconds - kTickSeconds) < 1e-4f;
    return n;
}

void Tick() {
    {
        Sim s = StowedRock();
        s.Move(1, kRS);
        s.Step();
        const int first = Ticks(s, 1), lines = s.Count(What::kTick, 1);
        s.Run(1.0f);
        const int stay = Ticks(s, 1);
        const float hx = Settings{}.shoulder.half[0];
        s.MoveTo(1, kRS[0] + hx * 1.05f, kRS[1], kRS[2]);  // the edge, 1.05 of its size
        s.Run(0.1f);
        s.Move(1, kRS);
        s.Run(0.1f);
        const int edge = Ticks(s, 1);
        s.MoveTo(1, kRS[0] + hx * 1.2f, kRS[1], kRS[2]);  // out
        s.Run(0.1f);
        s.Move(1, kRS);
        s.Run(0.1f);
        const int again = Ticks(s, 1);
        Report("T1", "an empty hand entering the place of a gun at its holster: one 20 ms tick and its line; staying in, or the edge: none; out and in: again",
               first == 1 && lines == 1 && stay == 1 && edge == 1 && again == 2, "entry %d (%d line), staying %d, the edge %d, out and in %d", first, lines, stay,
               edge, again);
    }
    {
        Sim s = HeldRock();
        s.Move(1, kRS);
        s.Step();
        const Event* e = s.Find(What::kTick, 1);
        Sim a = HeldRock();
        a.hand[1].aims = true;
        a.Move(1, kRS);
        a.Run(0.2f);
        Report("T2", "the hand holding the gun entering its own place: a tick; the trigger hand while it aims: never", Ticks(s, 1) == 1 && e && e->away &&
                                                                                                                        Ticks(a, 1) == 0,
               "%d; aiming %d", Ticks(s, 1), Ticks(a, 1));
    }
    {
        char said[256] = "";
        bool ok = true;
        const char* what[6] = {"nothing at the left shoulder", "the Mk11 (unknown)", "a busy hand", "the other hand holding the gun",
                               "the gun at another gun's place", "buzz 0"};
        for (int k = 0; k < 6; ++k) {
            Sim s = k == 3 || k == 4 ? HeldRock() : StowedRock();
            if (k == 1) {
                Swordbreaker(s);
                s.Run(0.05f);
            }
            if (k == 5)
                s.set.buzz = 0.0f;
            const int side = k == 3 ? 0 : 1;
            s.hand[side].magazine = k == 2;
            const float* where = k == 0 || k == 1 ? kLS : k == 3 || k == 4 ? kNeckAt : kRS;
            s.Move(side, where);
            s.Run(0.3f);
            ok = ok && Ticks(s, side) == 0;
            std::snprintf(said + std::strlen(said), sizeof(said) - std::strlen(said), "%s%s %d", k ? "; " : "", what[k], Ticks(s, side));
        }
        Report("T3", "a hand that can't act there never ticks", ok, "%s", said);
    }
    {
        Sim s = StowedRock();
        s.hand[1].seen = Seen::kNone;
        s.Move(1, kRS);
        s.Run(0.3f);
        const int unseen = Ticks(s, 1);
        Sim f = StowedRock();
        f.Move(1, kRS);
        f.Step();
        for (int k = 0; k < 10; ++k) {
            f.hand[1].seen = k % 2 ? Seen::kSeen : Seen::kNone;
            f.Step();
        }
        Report("T4", "a hand not seen never ticks; a flicker inside a place doesn't tick again", unseen == 0 && Ticks(f, 1) == 1, "not seen %d, flicker %d",
               unseen, Ticks(f, 1));
    }
}

// ---- P: holsters.txt ----

bool SameSettings(const Settings& a, const Settings& b) {
    const auto egg = [](const Egg& x, const Egg& y) {
        for (int i = 0; i < 3; ++i)
            if (x.mid[i] != y.mid[i] || x.half[i] != y.half[i])
                return false;
        return true;
    };
    return a.on == b.on && a.launcher == b.launcher && egg(a.shoulder, b.shoulder) && egg(a.neck, b.neck) && egg(a.rib, b.rib) && egg(a.head, b.head) &&
           a.keepOut == b.keepOut && a.stowMore == b.stowMore && a.stowGrace == b.stowGrace && a.graceMore == b.graceMore && a.swingReach == b.swingReach &&
           a.swingSpeed == b.swingSpeed && a.swingDegrees == b.swingDegrees && a.stowSpeed == b.stowSpeed &&
           a.anyHolster == b.anyHolster && a.otherGunStow == b.otherGunStow && a.lyingRecall == b.lyingRecall && a.twoLaunchersBySlot == b.twoLaunchersBySlot &&
           a.vanish == b.vanish && a.unknownGame == b.unknownGame && a.gameSwitchHand == b.gameSwitchHand && a.unseen == b.unseen &&
           a.unseenTake == b.unseenTake && a.unseenStow == b.unseenStow && a.approach == b.approach && a.leastSpeed == b.leastSpeed && a.degrees == b.degrees &&
           a.guessMost == b.guessMost && a.above == b.above && a.unseenShoulders == b.unseenShoulders && a.unseenDrop == b.unseenDrop &&
           a.drawWindow == b.drawWindow && a.drawSpeed == b.drawSpeed && a.stowSound == b.stowSound && a.stowPanSide == b.stowPanSide && a.buzz == b.buzz &&
           a.mirror == b.mirror && a.log == b.log && a.heldMagRefuse == b.heldMagRefuse && a.keyAgain == b.keyAgain && a.keyGiveUp == b.keyGiveUp &&
           a.kitHolster == b.kitHolster;
}

Settings ParseText(const std::string& text, ParseReport* rep = nullptr) { return Parse(text.data(), text.size(), rep); }

void File() {
    {
        // holsters 0: every decision none, the grasp's own let-go and squeezes exactly as today.
        Settings off = NeckSet();
        off.on = false;
        Sim a = HeldRock();
        a.set = off;
        a.MoveTo(1, kRS[0], kRS[1], kRS[2]);
        a.Run(0.1f);
        a.hand[1].squeeze = 0.0f;
        a.Run(0.1f);
        Sim b;  // (the gun lying: a squeeze at a place takes nothing and says nothing)
        Rock(b);
        b.set = off;
        b.holder = Holder::kLoose, b.trigger = -1;
        b.Run(0.1f);
        b.MoveTo(1, kNeckAt[0], kNeckAt[1], kNeckAt[2]);
        b.Run(0.1f);
        b.hand[1].squeeze = 1.0f;
        b.Run(0.6f);
        Sim c;  // (a hand lost behind the neck squeezing, the gun lying: nothing; a gun put away with holsters 0 is P1b's)
        Rock(c);
        c.set = off;
        c.holder = Holder::kLoose, c.trigger = -1;
        c.Run(0.1f);
        c.Move(1, kNeckAt);
        c.Run(0.05f);
        c.hand[1].seen = Seen::kNone;
        c.Run(0.3f);
        c.hand[1].squeeze = 1.0f;
        c.Run(0.3f);
        Sim d;
        Rock(d);
        d.set = off;
        d.gameSlot = 5, d.holder = Holder::kHeld, d.trigger = 1, d.armed[1] = true, d.hand[1].squeeze = 1.0f;
        d.Run(0.1f);
        d.Move(1, kNeckAt);
        d.Run(0.05f);
        d.hand[1].seen = Seen::kNone;
        d.Run(0.2f);
        d.hand[1].squeeze = 0.0f;
        d.Run(0.2f);
        Sim e = StowedRock();
        e.set = off;
        const ArriveOut o = e.Switch(5);
        const bool none = a.stows == 0 && a.falls == 1 && !a.everStows[1] && b.draws == 0 && b.Keys() == 0 && c.Keys() == 0 && d.stows == 0 && !d.everHold[1] &&
                          a.events.empty() && b.events.empty() && c.events.empty() && d.events.empty() && a.buzzes.empty() && b.buzzes.empty() &&
                          o.arrival == Arrival::kToday && d.stowSides == 0;
        Report("P1", "holsters 0: none for every decision (a let-go falls, a squeeze takes nothing, a hand not seen as today, today's switch)", none,
               "fell %d, drawn %d, keys %d, put away %d, held untracked %d, lines %d, pulses %d", a.falls, b.draws, c.Keys(), d.stows, d.everHold[1],
               static_cast<int>(a.events.size() + b.events.size() + c.events.size() + d.events.size()),
               static_cast<int>(a.buzzes.size() + b.buzzes.size()));
        // Holsters turned off while the gun is put away: back in the right hand at once, unarmed as at a spawn (its open grip doesn't drop it),
        // one line, no key, nothing felt; then nothing more.
        Sim w = StowedRock();
        w.set = off;
        w.Run(0.3f);
        const bool back = w.draws == 1 && w.drawnBy == 1 && w.holder == Holder::kHeld && w.trigger == 1 && w.falls == 0 && w.Keys() == 0 && w.buzzes.empty();
        w.MoveTo(0, kRS[0], kRS[1], kRS[2]);
        w.Run(0.1f);
        w.hand[0].squeeze = 1.0f;
        w.Run(0.3f);
        const Event* e2 = w.Find(What::kDrawn, 1);
        char line[160] = "";
        if (e2)
            Say(*e2, kNames, line, sizeof(line));
        Report("P1b", "holsters 0 with the gun put away: back in the right hand as at a spawn, said once; nothing else", back && w.draws == 1 &&
                                                                                                                             w.Said() == 1 && e2 && e2->off &&
                                                                                                                             std::strcmp(line, "Holster: holsters off: the M416 back in the right hand") == 0,
               "drawn %d by %d, fell %d, keys %d, pulses %d; \"%s\"", w.draws, w.drawnBy, w.falls, w.Keys(), static_cast<int>(w.buzzes.size()), line);
    }
    {
        ParseReport r1, r2;
        const Settings none = Parse(nullptr, 0, &r1), empty = ParseText("", &r2);
        Report("P2", "no file, an empty file: every default", SameSettings(none, Settings{}) && SameSettings(empty, Settings{}) && r1.refused == 0 &&
                                                                  r2.refused == 0,
               "%d and %d lines refused", r1.refused, r2.refused);
    }
    {
        // Every word with a value not its default.
        const std::string all =
            "holsters 1\nlauncher rib\nshoulder 0.18 -0.10 0.05 0.09 0.07 0.10\nneck 0.01 -0.05 -0.12 0.12 0.11 0.06\nrib -0.20 -0.31 -0.02 0.05 0.08 0.06\n"
            "head 0.01 0.11 0.03 0.14 0.12 0.16\nkeepout 0.03\nstowmore 0.04\nstowgrace 0.7 0.12\nswing 0.2 0.6 50\n"
            "stowspeed 1.2\nanyholster 0\nothergun stow\nlying recall\ntwolaunchers slot\nvanish 0.15\nunknown game\ngameswitch hand\n"
            "unseen 1.2 3.0 0.10 0.20 45 0.25 0.02\nunseenshoulders 1\nunseendrop 1\ndraw 0.15 0.5\nstowsound settle\nstowpan side\nbuzz 0.5\nmirror 1\n"
            "log 1\nheldmag refuse\nkeyagain 0.5 1.2\nkitguns holster\n";
        ParseReport rep;
        const Settings v = ParseText(all, &rep);
        const bool ok = rep.refused == 0 && rep.read == 30 && v.on && v.launcher == Launcher::kRib && v.shoulder.mid[0] == 0.18f && v.shoulder.half[2] == 0.10f &&
                        v.neck.mid[0] == 0.01f && v.neck.half[1] == 0.11f && v.rib.mid[0] == -0.20f && v.rib.half[0] == 0.05f && v.head.mid[0] == 0.01f &&
                        v.head.half[2] == 0.16f && v.keepOut == 0.03f && v.stowMore == 0.04f && v.stowGrace == 0.7f && v.graceMore == 0.12f &&
                        v.swingReach == 0.2f && v.swingSpeed == 0.6f && v.swingDegrees == 50.0f && v.stowSpeed == 1.2f &&
                        !v.anyHolster && v.otherGunStow && v.lyingRecall && v.twoLaunchersBySlot && v.vanish == 0.15f && v.unknownGame && v.gameSwitchHand &&
                        v.unseen && v.unseenTake == 1.2f && v.unseenStow == 3.0f && v.approach == 0.10f && v.leastSpeed == 0.20f && v.degrees == 45.0f &&
                        v.guessMost == 0.25f && v.above == 0.02f && v.unseenShoulders && v.unseenDrop && v.drawWindow == 0.15f && v.drawSpeed == 0.5f &&
                        v.stowSound == StowSound::kSettle && v.stowPanSide && v.buzz == 0.5f && v.mirror && v.log && v.heldMagRefuse && v.keyAgain == 0.5f &&
                        v.keyGiveUp == 1.2f && v.kitHolster;
        // The design's own listing, its explanations after the values: every default, nothing refused.
        const std::string listing =
            "holsters 1                         0: every let-go and squeeze exactly as today\n"
            "launcher shoulder                  shoulder | neck | rib | off (the user, 2026-11-15)\n"
            "shoulder 0.21 -0.01 -0.02 0.15 0.15 0.17    the right one: middle, half-sizes (m); the left mirrored (2026-11-19: a lot bigger)\n"
            "neck 0 0.07 -0.155 0.12 0.10 0.09\n"
            "rib -0.19 -0.30 -0.03 0.06 0.09 0.07        the left one\n"
            "head 0 0.12 0.02 0.15 0.13 0.17             no shoulder's place inside it\n"
            "keepout 0.02                       m every place keeps clear of the chests and the belly\n"
            "stowmore 0.03                      m added round a place for a put-away\n"
            "stowgrace 0.6 0.10                 s and m: a let-go just out of a place a moment after\n"
            "swing 0.15 0.5 60                  m short of a place, m/s toward it, degrees: the swing's end; \"swing 0\": none\n"
            "stowspeed 2.2                      m/s: outside the places, faster away from one is a throw\n"
            "anyholster 1                       0: only the gun's own place puts it away\n"
            "othergun refuse                    refuse | stow\n"
            "lying refuse                       refuse | recall\n"
            "twolaunchers last                  last | slot\n"
            "vanish 0                           s at rest open at a place before the gun goes\n"
            "unknown refuse                     refuse | game\n"
            "gameswitch stay                    stay | hand\n"
            "unseen 1.5 4.0 0.15 0.15 60 0.30 0.03     take s, put-away s, approach m, least m/s, degrees, the guess's most m, above m; \"unseen 0\": none\n"
            "stowsound vest                     vest | settle | none\n"
            "buzz 1                             0: no clicks\n"
            "mirror 0                           1: the primary on the left shoulder, the launcher's rib on the right\n"
            "log 0                              1: once a second while a hand is within 0.5 m of the neck point: its place, the place's Size, seen or not\n"
            "kitguns none                       holster: a gun a script's kit gives later on has the kit's holster (else on no shoulder till placed)\n";
        ParseReport lr;
        const Settings l = ParseText(listing, &lr);
        Report("P3", "every word read; the design's own listing with its explanations: every default, nothing refused",
               ok && SameSettings(l, Settings{}) && lr.refused == 0 && lr.read == 24, "%d read, %d refused; the listing: %d read, %d refused", rep.read,
               rep.refused, lr.read, lr.refused);
    }
    {
        struct Bad {
            const char* line;
            const char* says;
        };
        const Bad bad[] = {{"stowspeed fast", "stowspeed: m/s"},     {"launcher back", "launcher shoulder, neck, rib or off"},
                           {"shoulder -0.17 -0.09 0.04 0.08 0.08 0.09", "its x over 0"}, {"rib 0.19 -0.30 -0.03 0.06 0.09 0.07", "its x under 0"},
                           {"unseen 1.5", "unseen 0, unseen 1, or seven numbers"},       {"holsters 2", "holsters 1 or 0"},
                           {"frobnicate 1", "not a holsters.txt word"},                  {"neck 0 0 0 0 0 0", "half-sizes 0.01 to 0.5"},
                           {"draw 0.5 0.4", "0 to 0.4"},                                 {"keyagain 1.5 0.6", "giving up (later"},
                           {"buzz 3", "a strength up to 2"},                             {"vanish -1", "0 to 2"},
                           {"stowspeed 1.5x", "stowspeed: m/s"},                         {"othergun maybe", "refuse or stow"},
                           {"swing 0.5 1 30", "swing 0, or three numbers"},              {"swing 0.1 1", "swing 0, or three numbers"},
                           {"stowgrace 3 0.1", "stowgrace: s"},                          {"stowmore 0.3", "stowmore: m"},
                           {"keepout -0.01", "keepout: m"},                              {"head 0 0 0 0 0 0", "half-sizes 0.01 to 0.5"},
                           {"kitguns maybe", "kitguns none or holster"}};
        bool ok = true;
        char first[256] = "";
        for (const Bad& b : bad) {
            Settings s;
            s.stowSpeed = 1.3f;  // (a value from an earlier line: kept)
            char why[200] = "";
            const bool read = ParseLine(b.line, b.line + std::strlen(b.line), s, why, sizeof(why));
            Settings want;
            want.stowSpeed = 1.3f;
            const bool good = !read && std::strstr(why, b.says) && SameSettings(s, want);
            if (!good && !first[0])
                std::snprintf(first, sizeof(first), "\"%s\": %s", b.line, why);
            ok = ok && good;
        }
        ParseReport rep;
        ParseText("stowspeed 1.2\nfrobnicate 1\nmirror 1\n", &rep);
        Report("P4", "a bad line refused with its reason, the setting left as it was; the file's report names the line", ok && rep.refused == 1 &&
                                                                                                                             std::strstr(rep.why[0], "line 2 (frobnicate 1)"),
               "%d bad lines; %s; \"%s\"", static_cast<int>(sizeof(bad) / sizeof(bad[0])), first[0] ? first : "each refused", rep.why[0]);
    }
    {
        const Settings no = ParseText("unseen 0\n");
        Sim a = Unseen(kNeckAt, kStill, 0.3f, Seen::kNone, &no);
        Sim b;
        Rock(b);
        b.set = no;
        b.gameSlot = 5, b.holder = Holder::kHeld, b.trigger = 1, b.armed[1] = true, b.hand[1].squeeze = 1.0f;
        b.Run(0.1f);
        b.Move(1, kNeckAt);
        b.Run(0.05f);
        b.hand[1].seen = Seen::kNone;
        b.Run(1.0f);
        b.hand[1].squeeze = 0.0f;
        b.Run(0.2f);
        const Settings yes = ParseText("unseen 0\nunseen 1\n");
        Report("P5", "unseen 0: no take and no put-away by a hand not seen (and still no drop); unseen 1: back on with its numbers", !no.unseen && a.Keys() == 0 &&
                                                                                                                                         b.stows == 0 && b.falls == 0 &&
                                                                                                                                         b.holder == Holder::kHeld &&
                                                                                                                                         yes.unseen && yes.unseenTake == 1.5f,
               "keys %d, put away %d, fell %d", a.Keys(), b.stows, b.falls);
    }
    {
        const Settings off = ParseText("launcher off\n");
        Sim s = StowedRock();
        s.set = off;
        s.MoveTo(1, kNeckAt[0], kNeckAt[1], kNeckAt[2]);
        s.Run(0.1f);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.5f);
        Sim t;
        Rock(t);
        t.set = off;
        t.gameSlot = 5, t.holder = Holder::kHeld, t.trigger = 1, t.armed[1] = true, t.hand[1].squeeze = 1.0f;
        t.Run(0.1f);
        t.MoveTo(1, kNeckAt[0], kNeckAt[1], kNeckAt[2]);
        t.Run(0.1f);
        t.hand[1].squeeze = 0.0f;
        t.Run(0.1f);
        Report("P6", "launcher off: behind the neck is no place (nothing drawn there, the SMAW let go there falls)", off.launcher == Launcher::kOff && s.Keys() == 0 &&
                                                                                                                         s.events.empty() && t.stows == 0 && t.falls == 1,
               "keys %d, lines %d, put away %d, fell %d", s.Keys(), static_cast<int>(s.events.size()), t.stows, t.falls);
    }
    {
        const std::string lf = "# holsters\nlauncher rib\n\nstowspeed 1.0\n// a comment\nstowspeed 1.25\nmirror 1   the lefty's\n";
        std::string crlf, cr;
        for (const char ch : lf) {
            if (ch == '\n')
                crlf += "\r\n", cr += "\r";
            else
                crlf += ch, cr += ch;
        }
        const std::string bom = "\xEF\xBB\xBF" + crlf;
        ParseReport a, b, c, d;
        const Settings sa = ParseText(lf, &a), sb = ParseText(crlf, &b), sc = ParseText(cr, &c), sd = ParseText(bom, &d);
        Report("P7", "LF, CR LF, CR and a first UTF-8 mark read the same; comments and blank lines read as nothing; the last of a word wins",
               SameSettings(sa, sb) && SameSettings(sa, sc) && SameSettings(sa, sd) && a.refused + b.refused + c.refused + d.refused == 0 &&
                   sa.launcher == Launcher::kRib && sa.stowSpeed == 1.25f && sa.mirror && a.read == 4 && b.read == 4 && c.read == 4 && d.read == 4,
               "read %d %d %d %d, refused %d; stowspeed %.2f", a.read, b.read, c.read, d.read, a.refused + b.refused + c.refused + d.refused, sa.stowSpeed);
    }
}

// ---- L: the lines ----

void Lines() {
    struct Want {
        Event e;
        const char* text;
    };
    std::vector<Want> want;
    Event e;
    e.what = What::kStowed, e.side = 1, e.slot = 0, e.place = Place::kRightShoulder;
    want.push_back({e, "Holster: the M416 put away at the right shoulder by the right hand"});
    e.at = Place::kNeck;
    want.push_back({e, "Holster: the M416 put away at the right shoulder by the right hand (let go behind the neck: its own place is the right shoulder)"});
    e = Event{};
    e.what = What::kStowed, e.side = 1, e.slot = 5, e.place = Place::kNeck, e.unseen = true, e.unseenFor = 0.42f, e.lastOff = 0.09f, e.lastSpeed = 0.6f;
    want.push_back({e, "Holster: the SMAW put away behind the neck by the right hand (not seen for 0.42 s: last seen 9 cm from it, going at 0.6 m/s)"});
    e = Event{};
    e.what = What::kAsked, e.side = 1, e.slot = 5, e.place = Place::kNeck, e.key = 4;
    want.push_back({e, "Holster: the SMAW asked for from behind the neck by the right hand: key 4"});
    e.unseen = true, e.unseenFor = 0.42f, e.lastOff = 0.09f, e.lastSpeed = 0.6f;
    want.push_back({e, "Holster: the SMAW asked for from behind the neck by the right hand (not seen for 0.42 s: last seen 9 cm from it, going at 0.6 m/s): "
                       "key 4, in the hand once it's seen"});
    e = Event{};
    e.what = What::kNoSwitch, e.slot = 5, e.side = 1, e.since = 1.5f, e.twice = true;
    want.push_back({e, "Holster: the SMAW not drawn: the game didn't switch to it (1.50 s, its key pressed twice)"});
    e = Event{};
    e.what = What::kRefused, e.side = 0, e.slot = 5, e.other = 0, e.why = Why::kOtherHolds, e.place = Place::kNeck;
    want.push_back({e, "Holster: nothing drawn from behind the neck: the right hand holds the M416"});
    e.why = Why::kLying;
    want.push_back({e, "Holster: nothing drawn from behind the neck: the M416 lies on the ground: take it first"});
    e = Event{};
    e.what = What::kRefused, e.side = 0, e.slot = 1, e.why = Why::kUnknown, e.place = Place::kLeftShoulder;
    const char* sb[kSlots] = {"M4A1", "Mk11", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    char line[320];
    Say(e, sb, line, sizeof(line));
    const bool mk11 = std::strcmp(line, "Holster: the Mk11 not drawn: the hands don't know it yet (the D-pad or Y gives it the game's way)") == 0;
    e = Event{};
    e.what = What::kCameAway, e.slot = 5, e.why = Why::kNone;
    want.push_back({e, "Holster: the game switched to the SMAW by itself: it is at its place"});
    e = Event{};
    e.what = What::kStowRefused, e.side = 1, e.slot = 5, e.at = Place::kNeck, e.why = Why::kOtherRocket;
    want.push_back({e, "Holster: the SMAW not put away behind the neck: the left hand holds a rocket or a case, so it's let go of as anywhere"});
    e.kept = true, e.unseen = true, e.unseenFor = 0.30f, e.lastOff = 0.0f, e.lastSpeed = 0.0f;
    want.push_back({e, "Holster: the SMAW not put away behind the neck: the left hand holds a rocket or a case (not seen for 0.30 s: last seen 0 cm from "
                       "it, going at 0.0 m/s): it stays in the hand"});
    e = Event{};
    e.what = What::kTick, e.side = 0, e.slot = 0, e.place = Place::kRightShoulder;
    want.push_back({e, "Holster: the left hand at the right shoulder: a tick (the M416 can be drawn there)"});
    e = Event{};
    e.what = What::kCalledOff, e.side = 1, e.slot = 5, e.place = Place::kNeck, e.why = Why::kPicked, e.since = 0.3f;
    want.push_back({e, "Holster: the SMAW called off after 0.30 s: a pick of another gun came first (it stays at its place)"});
    e = Event{};
    e.what = What::kDrawn, e.side = 1, e.slot = 0, e.place = Place::kRightShoulder, e.off = true;
    want.push_back({e, "Holster: holsters off: the M416 back in the right hand"});
    int bad = 0;
    char first[400] = "";
    for (const Want& w : want) {
        Say(w.e, kNames, line, sizeof(line));
        if (std::strcmp(line, w.text) != 0) {
            if (!bad)
                std::snprintf(first, sizeof(first), "\"%s\" for \"%s\"", line, w.text);
            ++bad;
        }
    }
    // Every kind says something; a short buffer is cut, never overrun.
    int empty = 0;
    for (int k = 1; k <= static_cast<int>(What::kLook); ++k) {
        Event x;
        x.what = static_cast<What>(k), x.side = 1, x.slot = 0, x.place = Place::kRightShoulder;
        if (Say(x, kNames, line, sizeof(line)) <= 0 || std::strncmp(line, "Holster: ", 9) != 0)
            ++empty;
    }
    char tiny[12];
    std::memset(tiny, 'x', sizeof(tiny));
    Say(want[0].e, kNames, tiny, 8);
    const bool cut = tiny[7] == 0 && tiny[8] == 'x' && std::strlen(tiny) == 7;
    Report("L1", "the lines' words (the design's), every kind of event said, a short buffer cut cleanly", bad == 0 && mk11 && empty == 0 && cut,
           "%d wrong%s%s; the Mk11's %s; %d kinds said nothing; cut %d", bad, bad ? ": " : "", first, mk11 ? "right" : line, empty, cut);
}

// ---- N: broken inputs, the time ----

void Broken() {
    const float bad[4] = {kNaN, kInf, -kInf, 1e30f};
    int decisions = 0, frames = 0;
    char firstBad[160] = "";
    bool finite = true;
    for (const float v : bad) {
        for (int k = 0; k < 6; ++k) {
            if (std::isfinite(v) && (k == 2 || k == 5))
                continue;  // (a heading of 1e30 is an angle like any: the test's hand is placed by it too)
            Sim s = k < 3 ? StowedRock() : HeldRock();
            s.MoveTo(1, kRS[0], kRS[1], kRS[2]);
            s.Run(0.1f);
            if (k == 0 || k == 3)
                s.hand[1].pos[0] = v;
            if (k == 1 || k == 4)
                s.neck[1] = v;
            if (k == 2 || k == 5)
                s.yaw = v;
            s.everStows[1] = false;
            const int draws = s.draws, stows = s.stows;
            s.hand[1].squeeze = k < 3 ? 1.0f : 0.0f;
            for (int n = 0; n < 30; ++n) {
                s.Step(n % 3 == 0 ? v : kDt);
                ++frames;
                for (int i = 0; i < 2; ++i)
                    finite = finite && std::isfinite(s.r.buzz[i].amplitude) && std::isfinite(s.r.buzz[i].seconds) && std::isfinite(s.r.size[i]);
                finite = finite && std::isfinite(s.st.clock);
            }
            const int d = (s.draws - draws) + (s.stows - stows) + s.everStows[1] + s.Keys();
            if (d > 0 && !firstBad[0])
                std::snprintf(firstBad, sizeof(firstBad), "; the first: %g in %s (draws %d, stows %d, ever stowed %d, keys %d)", v,
                              k == 0 || k == 3 ? "the hand" : k == 1 || k == 4 ? "the neck" : "the heading", s.draws - draws, s.stows - stows,
                              static_cast<int>(s.everStows[1]), s.Keys());
            decisions += d;
        }
    }
    // A squeeze not finite, a velocity not finite.
    Sim q = StowedRock();
    q.MoveTo(1, kRS[0], kRS[1], kRS[2]);
    q.Run(0.1f);
    q.hand[1].squeeze = kNaN;
    q.hand[1].vel[0] = kNaN;
    q.Run(0.3f);
    Report("N1", "numbers not finite (the hand, the neck, the body's heading, the frame's time, the grip, the speed): nothing decided, nothing not finite out",
           decisions == 0 && finite && q.draws == 0 && q.Keys() == 0, "%d frames, %d decisions%s", frames, decisions, firstBad);
    // The time: a frame's Update and Settle.
    Sim t = StowedRock();
    t.MoveTo(1, kRS[0], kRS[1], kRS[2]);
    const Frame f = t.Build(kDt);
    State st;
    Result r;
    After a;
    const int n = 200000;
    const auto start = std::chrono::steady_clock::now();
    for (int k = 0; k < n; ++k) {
        Update(t.set, f, st, r);
        Settle(t.set, f, a, st, r);
    }
    const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count() / n;
    Report("N2", "a frame's Update and Settle in well under a microsecond or two", us < 5.0, "%.3f us a frame", us);
}

}  // namespace

// The holsters' own heading (holster.h FollowHeading, 0.1.5): a seated look towards a shoulder doesn't move the places; a hand up at the
// shoulders holds them; a turn of the whole body is followed, turning round at once; a recentre starts afresh; a small drift settles. The reviews of
// c6dabab: the aiming hand and the hand on the gun's front hold nothing (a standing player's turn while aiming is followed); a seated look held long
// with the hands resting in front of the body moves nothing (the hands turn with a body's turn, H3's).
void OwnHeading() {
    constexpr float kDt = 1.0f / 72.0f, kDeg = 3.14159265f / 180.0f;
    const holster::Settings set;
    // The hands in the room from the neck point (0, 1.2, 0) by body axes turned by handsYaw (radians, as the head's): low in front, on the lap; the
    // right up at its shoulder (up); or aiming (aim: the right at the eye with its gun there, the left on its front).
    struct Pose {
        bool up = false, aim = false, aimFlags = true;
        float handsYaw = 0.0f;
    };
    const auto frame = [](const Pose& p) {
        holster::Frame f;
        f.body = true;
        f.neck[0] = 0.0f, f.neck[1] = 1.2f, f.neck[2] = 0.0f;
        const float c = std::cos(p.handsYaw), sn = std::sin(p.handsYaw);
        for (int s = 0; s < 2; ++s) {
            float b[3] = {s ? 0.25f : -0.25f, -0.45f, 0.30f};  // low in front, on the lap
            if (p.up && s == 1)
                b[0] = 0.18f, b[1] = 0.05f, b[2] = -0.05f;  // the right at the right shoulder
            if (p.aim)
                s == 1 ? (b[0] = 0.10f, b[1] = -0.14f, b[2] = 0.30f) : (b[0] = -0.02f, b[1] = -0.12f, b[2] = 0.50f);  // at the eye; on its front
            f.hand[s].seen = holster::Seen::kSeen;
            f.hand[s].at[0] = f.neck[0] + c * b[0] - sn * b[2];
            f.hand[s].at[1] = f.neck[1] + b[1];
            f.hand[s].at[2] = f.neck[2] - sn * b[0] - c * b[2];
            f.hand[s].aims = p.aim && p.aimFlags && s == 1;
            f.hand[s].onGun = p.aim && p.aimFlags && s == 0;
        }
        return f;
    };
    // The head from head0 to head1 (degrees) in turn s, then held, total s in all (the hands turned with it when the whole body turns: bodyTurn); the
    // heading at the end (degrees).
    const auto run = [&](holster::Heading& h, float head0, float head1, float turn, float total, Pose pose, float body, bool bodyTurn = false) {
        float yaw = 0.0f;
        for (float t = kDt; t <= total + 1e-4f; t += kDt) {
            const float k = turn > 0.0f ? std::fmin(t / turn, 1.0f) : 1.0f;
            const float head = (head0 + (head1 - head0) * k) * kDeg;
            if (bodyTurn)
                pose.handsYaw = head;
            yaw = holster::FollowHeading(h, body * kDeg, head, frame(pose), kDt, set);
        }
        return yaw / kDeg;
    };
    const Pose low, up{true}, aim{false, true}, aimUnflagged{false, true, false};
    {
        holster::Heading h;
        const float y = run(h, 0.0f, 60.0f, 0.4f, 2.4f, low, 0.0f);
        Report("H1", "a seated look 60 degrees towards the right shoulder for 2 s, the hands on the lap: the places don't move", std::fabs(y) < 1.0f,
               "the heading %.1f degrees", y);
    }
    {
        holster::Heading h;
        const float y = run(h, 0.0f, 70.0f, 0.4f, 6.0f, up, 0.0f);
        Report("H2", "a look 70 degrees held 6 s with the right hand up at its shoulder: held where they were", std::fabs(y) < 1.0f,
               "the heading %.1f degrees", y);
    }
    {
        holster::Heading h;
        const float y = run(h, 0.0f, 90.0f, 0.5f, 5.0f, low, 0.0f, true);
        Report("H3", "the whole body turned 90 degrees and held, the hands low (turned with it): followed within 4 s of the turn",
               std::fabs(y - 90.0f) < 2.0f, "the heading %.1f degrees", y);
    }
    {
        holster::Heading h;
        const float y = run(h, 0.0f, 180.0f, 0.5f, 0.5f, low, 0.0f);
        Report("H4", "turning round (180 degrees in 0.5 s): dragged at once to within 100 degrees", y >= 79.0f, "the heading %.1f degrees at the turn's end", y);
    }
    {
        holster::Heading h;
        run(h, 0.0f, 0.0f, 0.0f, 0.5f, low, 0.0f);
        const float y = run(h, 90.0f, 90.0f, 0.0f, kDt, low, 90.0f);
        Report("H5", "a recentre (the head's heading jumps 90 degrees in a frame): started afresh from the body's", std::fabs(y - 90.0f) < 0.5f,
               "the heading %.1f degrees", y);
    }
    {
        holster::Heading h;
        const float y = run(h, 0.0f, 10.0f, 0.2f, 6.0f, low, 0.0f);
        Report("H6", "a 10-degree drift held still 6 s: settled on it (2 degrees a second)", std::fabs(y - 10.0f) < 0.5f, "the heading %.1f degrees", y);
    }
    {
        // H7: the second review's seated look: 70 degrees aside held 4 s and 8 s with the hands resting on the lap, then back: nothing moved (it
        // followed the look after 2.5 s, and for 2.5 s after looking back the places were 70 degrees off).
        holster::Heading h;
        const float y4 = run(h, 0.0f, 70.0f, 0.4f, 4.4f, low, 0.0f);
        const float back = run(h, 70.0f, 0.0f, 0.4f, 0.6f, low, 0.0f);
        holster::Heading g;
        const float y8 = run(g, 0.0f, -70.0f, 0.4f, 8.4f, low, 0.0f);
        Report("H7", "a seated look 70 degrees aside held 4 s (and 8 s the other way) with the hands resting on the lap: the places don't move; looked "
                     "back: where they were",
               std::fabs(y4) < 1.0f && std::fabs(back) < 1.0f && std::fabs(y8) < 1.0f, "the heading %.1f degrees after 4 s, %.1f just after looking "
                                                                                       "back; %.1f after 8 s", y4, back, y8);
    }
    {
        // H8: the first review's standing turn while aiming (the right at the eye with its gun there, the left on its front, both turned with the
        // body): followed (the aiming hand was within 40 cm of the neck point and held the places for good); 45 and 90 degrees, either way.
        float worst = 0.0f;
        for (const float a : {45.0f, -45.0f, 90.0f, -90.0f}) {
            holster::Heading h;
            const float y = run(h, 0.0f, a, 0.5f, 5.0f, aim, 0.0f, true);
            worst = std::fmax(worst, std::fabs(y - a));
        }
        // ... the same hands not flagged (the gun held up there but not at the eye, nor a hand on its front): followed too, judged where the hands
        // say the body faces (as the stale heading had them, the right one sat in the right shoulder's place and held the places for good).
        float worstUnflagged = 0.0f;
        for (const float a : {90.0f, -90.0f}) {
            holster::Heading h;
            const float y = run(h, 0.0f, a, 0.5f, 5.0f, aimUnflagged, 0.0f, true);
            worstUnflagged = std::fmax(worstUnflagged, std::fabs(y - a));
        }
        Report("H8", "standing, the whole body turned 45 and 90 degrees either way while aiming (the gun at the eye, the left on its front): followed "
                     "within 4 s; the same hands not flagged as aiming nor on its front, 90 degrees either way: followed too",
               worst < 2.0f && worstUnflagged < 2.0f, "worst %.1f degrees off the turn; not flagged: %.1f", worst, worstUnflagged);
    }
    {
        // H9: the hands say nothing hanging at the sides, or with one not seen: a look held past the dwell is followed, as before.
        holster::Heading h;
        float yaw = 0.0f;
        for (float t = kDt; t <= 5.0f + 1e-4f; t += kDt) {
            holster::Frame f = frame(low);
            f.hand[0].seen = holster::Seen::kNone;
            yaw = holster::FollowHeading(h, 0.0f, std::fmin(t / 0.4f, 1.0f) * 70.0f * kDeg, f, kDt, set);
        }
        holster::Heading g;
        float yaw2 = 0.0f;
        for (float t = kDt; t <= 5.0f + 1e-4f; t += kDt) {
            holster::Frame f = frame(low);
            for (int s = 0; s < 2; ++s)
                f.hand[s].at[0] = s ? 0.22f : -0.22f, f.hand[s].at[1] = 0.62f, f.hand[s].at[2] = 0.02f;  // hanging at the sides
            yaw2 = holster::FollowHeading(g, 0.0f, std::fmin(t / 0.4f, 1.0f) * 70.0f * kDeg, f, kDt, set);
        }
        Report("H9", "a look 70 degrees held 5 s with a hand not seen, or with the arms hanging at the sides: followed as before (the hands say nothing)",
               std::fabs(yaw / kDeg - 70.0f) < 2.0f && std::fabs(yaw2 / kDeg - 70.0f) < 2.0f, "the heading %.1f and %.1f degrees", yaw / kDeg,
               yaw2 / kDeg);
    }
}

// ---- HB: holsters by hand only (the user, 2026-11-18: "Don't let guns get automatically put into holsters if they're picked up from the ground..
// it needs to be physically placed on the shoulder to lock as a permanent gun primary or secondary unless it is physically removed from the holster
// and dropped") ----

constexpr uint32_t kAn94Kind = 0x94, kAkKind = 0xA4, kM416Kind = 0x416;

// A gun the game put in a slot by a take off the ground (a hand's, the magnet's, the game's own R): the slot's new gun and the take's mark.
void Took(Sim& s, int slot, uint32_t id, uint32_t kind, bool launcher = false, bool known = true) {
    s.slots[slot] = SlotIn{};
    s.slots[slot].carried = true, s.slots[slot].launcher = launcher, s.slots[slot].known = known, s.slots[slot].id = id, s.slots[slot].kind = kind;
    s.taken |= 1u << slot;
}
// A gun the game gave in a slot with no take (a script, a kit): the slot's new gun alone.
void Gave(Sim& s, int slot, uint32_t id, uint32_t kind, bool launcher = false, bool known = true) {
    s.slots[slot] = SlotIn{};
    s.slots[slot].carried = true, s.slots[slot].launcher = launcher, s.slots[slot].known = known, s.slots[slot].id = id, s.slots[slot].kind = kind;
}
Place PlaceNow(const Sim& s, int slot) {
    Place p[kSlots];
    PlacesNow(s.slots, s.st, s.set, p);
    return p[slot];
}
Place Hung(const Sim& s, int slot) { return HolsteredAt(s.set, s.slots, s.st, slot, s.holder); }
const char* RoleName(Role r) {
    return r == Role::kPrimary ? "primary" : r == Role::kSecondary ? "secondary" : r == Role::kLauncher ? "launcher" : "none";
}
// Rock and a Hard Place at the defaults (the launchers at the left shoulder), the M416 known by its identity, in the right hand, armed and squeezed.
Sim HandRock() {
    Sim s;
    s.set = Settings{};
    Rock(s);
    s.slots[0].id = 0x4160, s.slots[0].kind = kM416Kind;
    s.holder = Holder::kHeld, s.trigger = 1, s.armed[1] = true;
    s.hand[1].squeeze = 1.0f;
    s.Run(0.1f);
    return s;
}
// That hand lets go where it is (its grip open 0.1 s).
void LetGo(Sim& s, int side) {
    s.hand[side].squeeze = 0.0f;
    s.Run(0.1f);
}
// The M416 dropped in front, the right hand takes an AN-94 off the ground: the game puts it in slot 1 (empty: nothing of the player's goes into the
// pickup) and raises it, into the right hand; the grip squeezed again (armed).
Sim An94InRight() {
    Sim s = HandRock();
    s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
    s.Run(0.1f);
    LetGo(s, 1);
    s.Run(0.5f);
    Took(s, 1, 0x9401, kAn94Kind);
    s.Switch(1, true);  // (the take's arrival: into the hand that took it, today's)
    s.Step();
    s.hand[1].squeeze = 1.0f;
    s.armed[1] = true;
    s.Run(0.2f);
    return s;
}

void HandOnly() {
    {
        Sim s = HandRock();
        Report("HB1", "a new life's kit has its holsters from the start: the M416 the primary's (in the hand: at none now), the SMAW the launcher's at "
                      "the left shoulder; the M320, the M67, the knife none",
               s.st.home[0].role == Role::kPrimary && s.st.home[5].role == Role::kLauncher && s.st.home[2].role == Role::kNone &&
                   s.st.home[6].role == Role::kNone && PlaceNow(s, 0) == Place::kRightShoulder && PlaceNow(s, 5) == Place::kLeftShoulder &&
                   Hung(s, 0) == Place::kNone && Hung(s, 5) == Place::kLeftShoulder,
               "the M416 %s (%s), the SMAW %s (%s), the M320 %s", RoleName(s.st.home[0].role), Name(PlaceNow(s, 0)), RoleName(s.st.home[5].role),
               Name(Hung(s, 5)), RoleName(s.st.home[2].role));
    }
    {
        // Dropped, then another gun off the ground into the empty slot 1, raised by the game: neither is at a holster.
        Sim s = HandRock();
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.1f);
        LetGo(s, 1);
        const bool fell = s.holder == Holder::kLoose && s.falls == 1;
        const Role dropped = s.st.home[0].role;
        s.Run(0.5f);
        Took(s, 1, 0x9401, kAn94Kind);
        const ArriveOut o = s.Switch(1, true);
        s.Run(0.3f);
        const Event* e = s.Find(What::kNoPlace);
        char line[320] = "";
        if (e)
            Say(*e, kNames, line, sizeof(line));
        Report("HB2", "a take off the ground never holsters: the M416 dropped (its holster gone), an AN-94 taken into slot 1 and raised: in the right "
                      "hand, at no shoulder; the M416 on no shoulder either (said), no key",
               fell && dropped == Role::kNone && o.arrival == Arrival::kToday && s.holder == Holder::kHeld && s.trigger == 1 &&
                   s.st.home[1].role == Role::kNone && PlaceNow(s, 1) == Place::kNone && PlaceNow(s, 0) == Place::kNone && Hung(s, 0) == Place::kNone &&
                   e && e->slot == 0 && s.Count(What::kNoPlace) == 1 && s.Keys() == 0,
               "fell %d, the M416 %s, the AN-94 %s; \"%s\"", fell, RoleName(s.st.home[0].role), RoleName(s.st.home[1].role), line);
    }
    {
        // The M416 put away; an AN-94 taken that the game doesn't raise (another slot, not forced): it isn't at the left shoulder (2026-11-17's
        // "the gun in slot 1 at the secondary's shoulder" is gone): a squeeze there draws the SMAW (key 4), never the AN-94 (key 2).
        Sim s = HandRock();
        s.MoveTo(1, kRS[0], kRS[1], kRS[2]);
        s.Run(0.1f);
        LetGo(s, 1);
        const bool away = s.holder == Holder::kStowed;
        Took(s, 1, 0x9401, kAn94Kind);
        s.Run(0.5f);
        const Place an94 = Hung(s, 1), smawAt = PlaceNow(s, 5);
        s.MoveTo(0, kLS[0], kLS[1], kLS[2]);
        s.Run(0.1f);
        s.hand[0].squeeze = 1.0f;
        s.Run(0.3f);
        Report("HB3", "an AN-94 the game put in slot 1 without raising it: at no shoulder; the left shoulder keeps the SMAW (key 4), never the AN-94",
               away && an94 == Place::kNone && s.st.home[1].role == Role::kNone && smawAt == Place::kLeftShoulder && s.Keys(5) == 1 && s.Keys(1) == 0,
               "put away %d, the AN-94 %s, the SMAW %s, keys 4: %d, 2: %d", away, Name(an94), Name(smawAt), s.Keys(5), s.Keys(1));
    }
    {
        // The picked-up AN-94 put at the left shoulder by the hand: its holster now (the secondary's); the SMAW goes behind the neck (a secondary
        // hangs at the left shoulder: 2026-11-15's rule); drawn again there by the same hand.
        Sim s = An94InRight();
        s.MoveTo(1, kLS[0], kLS[1], kLS[2]);
        s.Run(0.1f);
        LetGo(s, 1);
        const Event* e = s.Find(What::kStowed, 1);
        const bool locked = s.holder == Holder::kStowed && s.st.home[1].role == Role::kSecondary && PlaceNow(s, 1) == Place::kLeftShoulder &&
                            Hung(s, 1) == Place::kLeftShoulder && PlaceNow(s, 5) == Place::kNeck && e && e->place == Place::kLeftShoulder &&
                            e->at == Place::kNone;
        s.Run(0.3f);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.2f);
        Report("HB4", "a picked-up gun put on the left shoulder by the hand: held there as the secondary (the SMAW behind the neck), drawn from there "
                      "again (in the hand at once, no key)",
               locked && s.holder == Holder::kHeld && s.trigger == 1 && s.Keys() == 0 && s.draws == 1,
               "%s, put away %s, the SMAW %s; drawn %d, keys %d", RoleName(s.st.home[1].role), e ? AtName(e->place) : "-", Name(PlaceNow(s, 5)), s.draws,
               s.Keys());
    }
    {
        // ... put on the RIGHT shoulder instead: the primary, whatever the game's slot (1): drawn from there with slot 1's key, never key 1.
        Sim s = An94InRight();
        s.MoveTo(1, kRS[0], kRS[1], kRS[2]);
        s.Run(0.1f);
        LetGo(s, 1);
        const Event* e = s.Find(What::kStowed, 1);
        const bool locked = s.st.home[1].role == Role::kPrimary && PlaceNow(s, 1) == Place::kRightShoulder && PlaceNow(s, 5) == Place::kLeftShoulder &&
                            e && e->place == Place::kRightShoulder && e->at == Place::kNone;
        // The SMAW drawn by the left at the left shoulder (key 4) and put back there: the AN-94 is then another slot's gun at the right shoulder.
        s.Run(0.3f);
        s.MoveTo(0, kLS[0], kLS[1], kLS[2]);
        s.Run(0.1f);
        s.hand[0].squeeze = 1.0f;
        s.Run(0.2f);
        s.gameSlot = 5;
        s.Switch(5);
        s.armed[0] = true;
        s.Run(0.2f);
        LetGo(s, 0);
        const bool smawBack = s.holder == Holder::kStowed && s.st.home[5].role == Role::kLauncher;
        s.Run(0.3f);
        s.MoveTo(1, kRS[0], kRS[1], kRS[2]);
        s.Run(0.1f);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.2f);
        Report("HB5", "a picked-up gun put on the right shoulder: the primary there whatever its slot; with the SMAW the game's weapon, a squeeze at the "
                      "right shoulder sends slot 1's key (2), never the M416's (1)",
               locked && smawBack && s.Keys(1) == 1 && s.Keys(0) == 0,
               "%s at %s; the SMAW back %d; keys 2: %d, 1: %d", RoleName(s.st.home[1].role), Name(PlaceNow(s, 1)), smawBack, s.Keys(1), s.Keys(0));
    }
    {
        // A holstered gun taken out and dropped: its shoulder is free (nothing there); picked up again it's still at none; put back, the primary again.
        Sim s = HandRock();
        s.MoveTo(1, kRS[0], kRS[1], kRS[2]);
        s.Run(0.1f);
        LetGo(s, 1);
        s.Run(0.3f);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.2f);
        const bool drawn = s.holder == Holder::kHeld && s.st.home[0].role == Role::kPrimary;  // (in the hand: its holster kept for the game's switches)
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.1f);
        LetGo(s, 1);
        const Role lying = s.st.home[0].role;
        const Place at = PlaceNow(s, 0);
        s.Run(0.3f);
        s.hand[1].squeeze = 1.0f;
        s.takeBy = 1;  // (the magnet takes it back)
        s.Run(0.2f);
        const Role taken = s.st.home[0].role;
        s.MoveTo(1, kRS[0], kRS[1], kRS[2]);
        s.Run(0.1f);
        LetGo(s, 1);
        Report("HB6", "a holstered gun taken out by the hand and dropped: no holster any more (nothing at its shoulder), none either once taken off "
                      "the ground; put back on the shoulder by the hand, the primary again",
               drawn && lying == Role::kNone && at == Place::kNone && taken == Role::kNone && s.holder == Holder::kStowed &&
                   s.st.home[0].role == Role::kPrimary,
               "drawn %d, dropped: %s (%s), taken up: %s, put back: %s", drawn, RoleName(lying), Name(at), RoleName(taken), RoleName(s.st.home[0].role));
    }
    {
        // The game's own switch with a gun in the hands (a pick): one with no holster goes to none (said: in its slot on no shoulder, the D-pad or Y
        // brings it); one that came from a shoulder goes back there (said).
        Sim a = An94InRight();
        a.events.clear();  // (the M416's own "on no shoulder" was said at the take)
        a.picked = true, a.pickSlot = 0;
        a.Step();
        a.gameSlot = 0;
        a.Switch(0);
        a.Run(0.2f);
        const Event* none = a.Find(What::kNoPlace);
        Sim b = HandRock();
        b.picked = true, b.pickSlot = 5;
        b.Step();
        b.gameSlot = 5;
        b.Switch(5);
        b.Run(0.2f);
        const Event* home = b.Find(What::kBackHome);
        Report("HB7", "the game's swap of a held gun: the picked-up AN-94 to no shoulder (said), the kit's M416 back at the right shoulder (said)",
               none && none->slot == 1 && PlaceNow(a, 1) == Place::kNone && Hung(a, 1) == Place::kNone && home && home->slot == 0 &&
                   home->place == Place::kRightShoulder && Hung(b, 0) == Place::kRightShoulder,
               "the AN-94 %s (%s), the M416 %s (%s)", none ? "said" : "not said", Name(Hung(a, 1)), home ? "said" : "not said", Name(Hung(b, 0)));
    }
    {
        // The game switching by itself, while put away, to a gun with no holster: into the right hand, never kept at a place it doesn't have.
        Sim s = HandRock();
        s.MoveTo(1, kRS[0], kRS[1], kRS[2]);
        s.Run(0.1f);
        LetGo(s, 1);
        Took(s, 1, 0x9401, kAn94Kind);
        s.Run(3.0f);  // (past a pick's seconds)
        s.gameSlot = 1;
        const ArriveOut o = s.Switch(1);
        char line[320] = "";
        Say(o.event, kNames, line, sizeof(line));
        Report("HB8", "the game's own switch, put away, to a gun on no shoulder: into the right hand (said), not at a place",
               o.arrival == Arrival::kHand && o.side == 1 && o.event.what == What::kToHand && s.holder == Holder::kHeld, "arrival %d into %d; \"%s\"",
               static_cast<int>(o.arrival), o.side, line);
    }
    {
        // The game's weapon put away but with no holster (here: the game's own R put an AN-94 into the put-away M416's slot): it can't stay put away
        // at no place: into the right hand, unarmed, as at a spawn; said once.
        Sim s = HandRock();
        s.MoveTo(1, kRS[0], kRS[1], kRS[2]);
        s.Run(0.1f);
        LetGo(s, 1);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.4f);
        Took(s, 0, 0x9402, kAn94Kind);
        s.Run(0.3f);
        const Event* d = s.Find(What::kDrawn, 1);
        Report("HB9", "the game's weapon put away with no holster: into the right hand, unarmed, as at a spawn (said once: kToHand, then drawn)",
               s.holder == Holder::kHeld && s.trigger == 1 && !s.armed[1] && s.Count(What::kToHand) == 1 && d && d->noPlace,
               "holder %d by %d, armed %d, said %d, drawn %s", static_cast<int>(s.holder), s.trigger, s.armed[1], s.Count(What::kToHand),
               d && d->noPlace ? "from no shoulder" : "-");
    }
    {
        // The launchers keep their place: the SMAW let go at the right shoulder goes to the left (its place); with a secondary at the left shoulder,
        // the SMAW let go there goes behind the neck.
        Sim a;
        a.set = Settings{};
        Rock(a);
        a.gameSlot = 5, a.holder = Holder::kHeld, a.trigger = 1, a.armed[1] = true, a.hand[1].squeeze = 1.0f;
        a.Run(0.1f);
        a.MoveTo(1, kRS[0], kRS[1], kRS[2]);
        a.Run(0.1f);
        LetGo(a, 1);
        const Event* ea = a.Find(What::kStowed, 1);
        Sim b;
        b.set = Settings{};
        Rock(b);
        b.slots[1] = {true, false, true};  // (a secondary of the kit: its holster the left shoulder)
        b.gameSlot = 5, b.holder = Holder::kHeld, b.trigger = 1, b.armed[1] = true, b.hand[1].squeeze = 1.0f;
        b.Run(0.1f);
        b.MoveTo(1, kLS[0], kLS[1], kLS[2]);
        b.Run(0.1f);
        LetGo(b, 1);
        const Event* eb = b.Find(What::kStowed, 1);
        Report("HB10", "the launchers' rule kept: the SMAW let go at the right shoulder is put away at the left (its place); with a secondary at the left "
                       "shoulder, let go there, behind the neck",
               ea && ea->place == Place::kLeftShoulder && ea->at == Place::kRightShoulder && a.st.home[5].role == Role::kLauncher && eb &&
                   eb->place == Place::kNeck && eb->at == Place::kLeftShoulder,
               "%s (let go %s); %s (let go %s)", ea ? Name(ea->place) : "-", ea ? AtName(ea->at) : "-", eb ? Name(eb->place) : "-", eb ? AtName(eb->at) : "-");
    }
    {
        // HolsteredVictim: Operation Swordbreaker's two guns at their shoulders, a take naming either slot takes one off its shoulder: refused; the
        // M4A1 dropped first: a take naming its slot 0 is fine (the gun on the ground goes into the pickup), one naming slot 1 still isn't (the Mk11
        // hangs at the left shoulder); the game's slot unread: every gun with a holster counts; holsters 0: none.
        Sim s;
        s.set = Settings{};
        Swordbreaker(s);
        s.holder = Holder::kStowed, s.trigger = -1;
        s.Run(0.1f);
        Place at0 = Place::kNone, at1 = Place::kNone;
        const int both0 = HolsteredVictim(s.set, s.slots, s.st, s.holder, 1u << 0, &at0), both1 = HolsteredVictim(s.set, s.slots, s.st, s.holder, 1u << 1, &at1);
        Sim d;
        d.set = Settings{};
        Swordbreaker(d);
        d.holder = Holder::kHeld, d.trigger = 1, d.armed[1] = true, d.hand[1].squeeze = 1.0f;
        d.Run(0.1f);
        d.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        d.Run(0.1f);
        LetGo(d, 1);
        const int drop0 = HolsteredVictim(d.set, d.slots, d.st, d.holder, 1u << 0), drop1 = HolsteredVictim(d.set, d.slots, d.st, d.holder, 1u << 1);
        Sim u = s;
        u.slotRead = false;
        u.Run(0.6f);
        const int unread = HolsteredVictim(u.set, u.slots, u.st, u.holder, 1u << 0);
        Settings off = s.set;
        off.on = false;
        const int none = HolsteredVictim(off, s.slots, s.st, s.holder, 3u);
        Report("HB11", "a take that would put a holstered gun into its pickup is found (refused by hands.cpp): either of Swordbreaker's shoulders; the "
                       "dropped M4A1's slot fine, the Mk11's still not; unread: every holstered gun; holsters 0: none",
               both0 == 0 && at0 == Place::kRightShoulder && both1 == 1 && at1 == Place::kLeftShoulder && drop0 == -1 && drop1 == 1 && unread == 0 &&
                   none == -1,
               "both at theirs: %d (%s), %d (%s); dropped: %d, %d; unread %d; off %d", both0, Name(at0), both1, Name(at1), drop0, drop1, unread, none);
        // The double click for that refusal: the first pulse now, the second 60 ms on (Settle's).
        Sim p = s;
        const float from = p.time;
        const Pulse first = RefusePulse(p.set, p.st, 1);
        p.Run(0.2f);
        std::vector<float> seconds;
        for (const Buzzed& b : p.buzzes)
            if (b.side == 1 && b.at >= from && std::fabs(b.amp - kDoubleAmp) < 1e-4f)
                seconds.push_back(b.at - from);
        Report("HB11b", "a refused take's double click: the first pulse at once, the second 60 ms on",
               std::fabs(first.amplitude - kDoubleAmp) < 1e-4f && std::fabs(first.seconds - kDoubleSeconds) < 1e-4f && seconds.size() == 1 &&
                   seconds[0] > 0.045f && seconds[0] < 0.08f,
               "first %.2f for %.3f s, the second at %.3f s", first.amplitude, first.seconds, seconds.empty() ? -1.0f : seconds[0]);
    }
    {
        // A gun told by its identity: a moment unread keeps its holster; the same gun under a new object (no take) too. A gun the game gave after the
        // kit's seconds: none (a launcher: the roof's AT4 by its kit, meant to be thrown away; a rifle into the empty slot 1, a script's kit's gun:
        // "kitguns none", the default), and none while the takes aren't seen (it may be a take); with "kitguns holster" the rifle is the kit's (KitSwap:
        // the missions' surveys of 2026-11-19, the A13 lane), the launcher none still; within the kit's seconds: the kit's.
        Sim s = HandRock();
        s.MoveTo(1, kRS[0], kRS[1], kRS[2]);
        s.Run(0.1f);
        LetGo(s, 1);
        s.slots[0].carried = false;
        s.Run(0.2f);
        s.slots[0].carried = true;
        s.Run(0.1f);
        const Role gap = s.st.home[0].role;
        s.Run(10.5f);
        s.slots[0].id = 0x4161;  // (the same blueprint, a new object: read afresh)
        s.Run(0.1f);
        const Role afresh = s.st.home[0].role;
        Gave(s, 1, 0xA401, kAkKind);
        s.Run(0.1f);
        const Role gift = s.st.home[1].role;
        Sim n = HandRock();
        n.Run(10.5f);
        n.swapsSeen = false;
        Gave(n, 1, 0xA401, kAkKind);
        n.Run(0.1f);
        const Role unseenGift = n.st.home[1].role;
        Sim k = HandRock();
        Gave(k, 1, 0xA401, kAkKind);
        k.Run(0.1f);
        Role launcherGift[2] = {}, rifleGift = Role::kNone;
        for (int kitGuns = 0; kitGuns < 2; ++kitGuns) {
            Sim l = HandRock();
            if (kitGuns)
                l.set.kitHolster = true;  // (else the default)
            l.Run(10.5f);
            Gave(l, 1, 0xA742, 0xA74, true, true);  // (an AT4, a launcher, by a script)
            l.Run(0.1f);
            launcherGift[kitGuns] = l.st.home[1].role;
            if (kitGuns) {
                Sim h = HandRock();
                h.set.kitHolster = true;
                h.Run(10.5f);
                Gave(h, 1, 0xA401, kAkKind);
                h.Run(0.1f);
                rifleGift = h.st.home[1].role;
            }
        }
        Report("HB12", "a gun by its identity: unread a moment, kept; the same gun read afresh, kept; a gift after the kit's 10 s: none (the "
                       "review of 2026-11-18: the roof's AT4 by its kit went behind the neck; a script's kit's rifle, \"kitguns none\"), as with the takes "
                       "not seen; within them: the kit's. With \"kitguns holster\": the rifle the kit's (the secondary), the launcher none still",
               gap == Role::kPrimary && afresh == Role::kPrimary && gift == Role::kNone && unseenGift == Role::kNone && k.st.home[1].role == Role::kSecondary &&
                   launcherGift[0] == Role::kNone && launcherGift[1] == Role::kNone && rifleGift == Role::kSecondary,
               "gap %s, afresh %s, gift %s, takes unseen %s, kit %s; a launcher given %s / %s, a rifle given with kitguns holster %s", RoleName(gap),
               RoleName(afresh), RoleName(gift), RoleName(unseenGift), RoleName(k.st.home[1].role), RoleName(launcherGift[0]), RoleName(launcherGift[1]),
               RoleName(rifleGift));
    }
    {
        // A take's mark and its new gun a frame apart, either first: no holster; a mark 1.5 s before a gun: not that take's (a gift: within a new
        // life's 10 s, the kit's place, as here; later, none either: HB12).
        Sim a = HandRock();
        a.taken = 1u << 1;
        a.Step();
        Gave(a, 1, 0x9401, kAn94Kind);
        a.Run(0.1f);
        Sim b = HandRock();
        Gave(b, 1, 0x9401, kAn94Kind);
        b.Step();
        const Role before = b.st.home[1].role;
        b.taken = 1u << 1;
        b.Run(0.1f);
        Sim c = HandRock();
        c.taken = 1u << 1;
        c.Step();
        c.Run(1.5f);
        Gave(c, 1, 0x9401, kAn94Kind);
        c.Run(0.1f);
        Report("HB13", "a take's mark a frame before its gun, or after (the gun first noted as a gift): no holster; a mark 1.5 s before: a gift's (in "
                       "a new life's 10 s: the kit's place)",
               a.st.home[1].role == Role::kNone && before == Role::kSecondary && b.st.home[1].role == Role::kNone && c.st.home[1].role == Role::kSecondary,
               "mark first %s; gun first %s then %s; long before %s", RoleName(a.st.home[1].role), RoleName(before), RoleName(b.st.home[1].role),
               RoleName(c.st.home[1].role));
    }
    {
        // anyholster 0: only where the gun stays: the M416 put on the left shoulder stays there (the secondary); let go behind the neck it falls.
        Sim a = HandRock();
        a.set.anyHolster = false;
        a.MoveTo(1, kLS[0], kLS[1], kLS[2]);
        a.Run(0.1f);
        LetGo(a, 1);
        Sim b = HandRock();
        b.set.anyHolster = false;
        b.set.launcher = Launcher::kNeck;
        b.MoveTo(1, kNeckAt[0], kNeckAt[1], kNeckAt[2]);
        b.Run(0.1f);
        LetGo(b, 1);
        Report("HB14", "anyholster 0: a gun put away only where it stays: the M416 on the left shoulder (the secondary there); behind the neck, it falls",
               a.stows == 1 && a.st.home[0].role == Role::kSecondary && PlaceNow(a, 0) == Place::kLeftShoulder && b.stows == 0 && b.falls == 1,
               "left: put away %d (%s); neck: put away %d, fell %d", a.stows, RoleName(a.st.home[0].role), b.stows, b.falls);
    }
    {
        // The tick for the hand holding a gun: at either shoulder (it can be put away there, and stays there); never at a place that sends it
        // elsewhere (behind the neck: its own shoulder).
        Sim a = HandRock();
        a.Move(1, kLS);
        a.Run(0.1f);
        Sim b = HandRock();
        b.set.launcher = Launcher::kNeck;
        b.Move(1, kNeckAt);
        b.Run(0.1f);
        const Event* e = a.Find(What::kTick, 1);
        Report("HB15", "the tick for the hand holding the M416: at the left shoulder too (it would stay there), not behind the neck (it'd go elsewhere)",
               Ticks(a, 1) == 1 && e && e->away && Ticks(b, 1) == 0, "left %d, the neck %d", Ticks(a, 1), Ticks(b, 1));
    }
    {
        // "othergun stow" and "lying recall" read, answering nothing: a squeeze at a place while the other hand holds the gun, or while it lies, never
        // puts a gun into a holster by itself.
        Sim a = HandRock();
        a.set.otherGunStow = true;
        a.MoveTo(0, kLS[0], kLS[1], kLS[2]);
        a.Run(0.1f);
        a.hand[0].squeeze = 1.0f;
        a.Run(0.4f);
        const Event* ea = a.Find(What::kRefused, 0);
        Sim b;
        b.set = Settings{};
        b.set.lyingRecall = true;
        Rock(b);
        b.holder = Holder::kLoose, b.trigger = -1;
        b.Run(0.1f);
        b.MoveTo(1, kRS[0], kRS[1], kRS[2]);
        b.Run(0.1f);
        b.hand[1].squeeze = 1.0f;
        b.Run(0.6f);
        const Event* eb = b.Find(What::kRefused, 1);
        Report("HB16", "othergun stow, lying recall: read, but no gun is put away by itself (refused: the other hand holds it; it lies)",
               ea && ea->why == Why::kOtherHolds && a.stowSides == 0 && a.holder == Holder::kHeld && eb && eb->why == Why::kLying && !b.Find(What::kRecall) &&
                   b.holder == Holder::kLoose,
               "%s; %s, recalled %d", ea ? "refused" : "-", eb ? "refused" : "-", b.Find(What::kRecall) ? 1 : 0);
    }
    {
        // A new life: every gun's holster afresh, the kit's (the M416 dropped before is the primary again).
        Sim s = HandRock();
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.1f);
        LetGo(s, 1);
        const Role dropped = s.st.home[0].role;
        Clear(s.st);
        s.holder = Holder::kHeld, s.trigger = 1, s.armed[1] = false;  // (back in the right hand, given)
        s.slots[0].id = 0x4170;  // (a new life's objects)
        s.Run(0.1f);
        Report("HB17", "a new life: the kit's holsters again (the M416 dropped before: the primary)", dropped == Role::kNone && s.st.home[0].role == Role::kPrimary &&
                                                                                                     PlaceNow(s, 0) == Place::kRightShoulder,
               "dropped %s, after %s", RoleName(dropped), RoleName(s.st.home[0].role));
    }
    {
        // The grenade launcher's shot (the game's weapon slot 2 a moment, then back) and a grenade (slot 6): the M416 never leaves the hands, nothing
        // said of shoulders.
        Sim s = HandRock();
        s.gameSlot = 2;
        s.Run(0.3f);
        s.gameSlot = 0;
        s.Run(0.3f);
        s.gameSlot = 6;
        s.Run(0.3f);
        s.gameSlot = 0;
        s.Run(0.3f);
        Report("HB19", "the grenade launcher's shot and a grenade (the game's weapon slot 2 or 6 a moment): no line of shoulders, the M416's holster kept",
               s.Count(What::kBackHome) == 0 && s.Count(What::kNoPlace) == 0 && s.Count(What::kToHand) == 0 && s.st.home[0].role == Role::kPrimary,
               "back home %d, no place %d, to hand %d", s.Count(What::kBackHome), s.Count(What::kNoPlace), s.Count(What::kToHand));
    }
    {
        // The lines.
        char a[320], b[320], c[320], d[320];
        Event e;
        e.what = What::kNoPlace, e.slot = 0;
        Say(e, kNames, a, sizeof(a));
        e = Event{};
        e.what = What::kBackHome, e.slot = 0, e.place = Place::kRightShoulder;
        Say(e, kNames, b, sizeof(b));
        e = Event{};
        e.what = What::kToHand, e.slot = 5, e.side = 1;
        Say(e, kNames, c, sizeof(c));
        e = Event{};
        e.what = What::kDrawn, e.slot = 5, e.side = 1, e.noPlace = true;
        Say(e, kNames, d, sizeof(d));
        Report("HB18", "the lines of holsters by hand only",
               std::strcmp(a, "Holster: the M416 is on no shoulder (a gun goes on one only by your hand): the game keeps it in its slot 0, the D-pad or Y brings it "
                              "into the right hand") == 0 &&
                   std::strcmp(b, "Holster: the M416 back at the right shoulder, where it came from (the game's own switch)") == 0 &&
                   std::strcmp(c, "Holster: the game's weapon is the SMAW, on no shoulder (a gun goes on one only by your hand): into the right hand as at a "
                                  "spawn") == 0 &&
                   std::strcmp(d, "Holster: the SMAW in the right hand: it was put away on no shoulder") == 0,
               "\"%s\" | \"%s\" | \"%s\" | \"%s\"", a, b, c, d);
    }
}

// ---- HA: a holstered gun the game takes out of its slot stays on its shoulder (the user's AT4, 2026-11-18: "a smart player would holster their
// rifle before picking up the AT4, but they could momentarily drop their rifle on the floor, pick up the AT4, shoot it, discard it, then pick up their
// rifle again") ----

constexpr uint32_t kAt4Kind = 0xA74, kM4Kind = 0x4A1, kMk11Kind = 0x111, kSmawKind = 0x5A3, kM320Kind = 0x320;
constexpr uint32_t kBlueprintOf = 0x100000;  // the test's blueprints: its kind and this (hands.cpp's are the game's addresses, its kinds by name)

// The test's game: the pickups on the ground (one gun each, naming one slot), the soldier's rounds by slot, and how it answers a take.
struct World {
    struct Gun {
        uint32_t kind = 0;  // 0: nothing
        int loaded = 0, spare = 0;
        bool launcher = false;
        char name[kNameLen] = {};
    };
    struct Pick {
        uint32_t entity = 0;
        float distance = 1.0f;  // from the soldier (m): the game's check takes it within 2.5
        bool gone = false;
        int slot = -1;  // the slot its entry names
        Gun gun;
    };
    Pick pick[4];
    int picks = 0;
    int loaded[kSlots] = {}, spare[kSlots] = {};
    uint32_t nextId = 0x7000;
    bool refuse = false, never = false;  // the game says no to the next give-back / never answers it
    const char* refuseWhy = "the game doesn't allow it now";
    int servePick = -1, serveSide = -1;  // a give-back asked, served on the game's thread before the next frame ...
    int late = 0, serveIn = 0;           // ... or this many frames later
    int asked = 0, served = 0;
    int AddPick(uint32_t entity, float distance, int slot, uint32_t kind, int loaded, int spare, bool launcher, const char* name) {
        Pick& p = pick[picks];
        p = Pick{};
        p.entity = entity, p.distance = distance, p.slot = slot;
        p.gun.kind = kind, p.gun.loaded = loaded, p.gun.spare = spare, p.gun.launcher = launcher;
        CopyName(p.gun.name, name);
        return picks++;
    }
};
World& WorldOf(Sim& s) { return *static_cast<World*>(s.world); }

void Named(SlotIn& v, uint32_t id, uint32_t kind, const char* name) {
    v.id = id, v.kind = kind;
    CopyName(v.name, name);
}

// The test's game takes pickup pi for the soldier (a hand's take, the magnet's, the game's own R, or a draw's give-back), as pickrules.h reads
// 008AB870: the gun of the slot its entry names goes into it with its rounds, its gun into that slot with the pickup's rounds. hands.cpp's NoteSwaps
// tells holsters first (NoteSwap, the take's mark), then the game raises it in its slot and the take's arrival brings it into that hand (-1: none).
void TakeOf(Sim& s, World& w, int pi, int side) {
    World::Pick& p = w.pick[pi];
    const int k = p.slot;
    SwapIn in;
    in.changed = 1, in.slot[0] = k, in.entity = p.entity;
    World::Gun went;
    if (s.slots[k].carried) {
        went.kind = s.slots[k].kind, went.loaded = w.loaded[k], went.spare = w.spare[k], went.launcher = s.slots[k].launcher;
        CopyName(went.name, s.slots[k].name);
        in.went[0] = went.kind, in.wentBlueprint[0] = went.kind + kBlueprintOf;
    }
    in.came[0] = p.gun.kind;
    CopyName(in.taken, p.gun.name);
    in.removed = went.kind == 0;  // (nothing went into it: it goes with its take)
    NoteSwap(s.set, s.slots, in, s.st);
    SlotIn& v = s.slots[k];
    v = SlotIn{};
    v.carried = true, v.launcher = p.gun.launcher, v.known = true, v.id = w.nextId++, v.kind = p.gun.kind;
    CopyName(v.name, p.gun.name);
    w.loaded[k] = p.gun.loaded, w.spare[k] = p.gun.spare;
    s.taken |= 1u << k;
    if (went.kind)
        p.gun = went;
    else
        p.gone = true;
    ++w.served;
    if (side == 0 || side == 1) {
        s.gameSlot = k;
        const ArriveOut o = s.Switch(k, true);
        if (o.arrival != Arrival::kAtPlace) {  // (at its place: hands.cpp puts it away there, no arrival into the hand)
            s.holder = Holder::kHeld, s.trigger = side, s.barrel = -1;
            s.armed[side] = true;  // (the grasp's arrival is a take)
        }
    }
}

// Before each frame: a give-back asked last frame served (the game's take), and each away gun's pickup as the census would have it.
void WorldBefore(Sim& s) {
    World& w = WorldOf(s);
    if (w.servePick >= 0 && w.serveIn > 0) {
        --w.serveIn;
    } else if (w.servePick >= 0) {
        const int pi = w.servePick, side = w.serveSide;
        w.servePick = -1;
        TakeOf(s, w, pi, side);
    }
    for (int a = 0; a < kMostAway; ++a) {
        s.awayFound[a] = -2, s.awayReach[a] = false, s.awayDistance[a] = 0.0f;
        const Away& r = s.st.away[a];
        if (!r.on || !r.entity)
            continue;
        s.awayFound[a] = -1;
        for (int i = 0; i < w.picks; ++i) {
            const World::Pick& p = w.pick[i];
            if (p.entity != r.entity || p.gone)
                continue;
            const bool holds = p.gun.kind && p.gun.kind + kBlueprintOf == r.blueprint;
            s.awayFound[a] = holds ? 1 : 0;
            s.awayDistance[a] = p.distance;
            s.awayReach[a] = holds && p.distance <= 2.5f;
        }
    }
}
// After Update: hands.cpp asks the game's take of a draw's pickup (pickups::RequestGiveBack: the census holds the gun), served before the next frame;
// refused at once (GiveBackFailed) when it isn't there, holds another gun, is out of the game's reach, or the game says no; or never answered.
void WorldAfter(Sim& s) {
    World& w = WorldOf(s);
    if (s.r.giveBack.side < 0)
        return;
    ++w.asked;
    int pi = -1;
    for (int i = 0; i < w.picks; ++i)
        if (w.pick[i].entity == s.r.giveBack.entity && !w.pick[i].gone)
            pi = i;
    const bool holds = pi >= 0 && w.pick[pi].gun.kind + kBlueprintOf == s.r.giveBack.blueprint;
    if (!holds) {
        GiveBackFailed(s.st, s.r.giveBack.side, pi < 0 ? "not in the census" : "it holds another gun now", pi >= 0 ? w.pick[pi].distance : -1.0f);
        return;
    }
    if (w.refuse || w.pick[pi].distance > 2.5f) {
        GiveBackFailed(s.st, s.r.giveBack.side, w.refuse ? w.refuseWhy : "it wouldn't let you take it from there now", w.pick[pi].distance);
        return;
    }
    if (w.never)
        return;
    w.servePick = pi, w.serveSide = s.r.giveBack.side, w.serveIn = w.late;
}

// Rock and a Hard Place's kit at the defaults, every gun known by its kind and name (as hands.cpp's HolsterSlots has them), the M416 in the right hand,
// armed and squeezed; the test's game in w.
Sim RockWorld(World& w) {
    Sim s;
    s.set = Settings{};
    Rock(s);
    Named(s.slots[0], 0x4160, kM416Kind, "M416");
    Named(s.slots[2], 0x3200, kM320Kind, "M320");
    Named(s.slots[5], 0x5A30, kSmawKind, "SMAW");
    Named(s.slots[6], 0x6700, 0x67, "M67");
    Named(s.slots[7], 0x7700, 0x77, "Knife");
    s.world = &w, s.before = WorldBefore, s.after = WorldAfter;
    s.holder = Holder::kHeld, s.trigger = 1, s.armed[1] = true;
    s.hand[1].squeeze = 1.0f;
    s.Run(0.1f);
    return s;
}
// Operation Swordbreaker's: the M4A1 (0), the Mk11 (1), the M67 (6), the M4A1 in the right hand.
Sim SwordWorld(World& w) {
    Sim s;
    s.set = Settings{};
    Swordbreaker(s);
    Named(s.slots[0], 0x4A10, kM4Kind, "M4A1");
    s.slots[1].known = true;
    Named(s.slots[1], 0x1110, kMk11Kind, "Mk11");
    Named(s.slots[6], 0x6760, 0x67, "M67");
    s.world = &w, s.before = WorldBefore, s.after = WorldAfter;
    s.holder = Holder::kHeld, s.trigger = 1, s.armed[1] = true;
    s.hand[1].squeeze = 1.0f;
    s.Run(0.1f);
    return s;
}
// That hand puts the gun it holds on that place (from in front, a slow let-go there).
void PutOn(Sim& s, int side, const float at[3]) {
    s.Move(side, at);
    s.Run(0.1f);
    LetGo(s, side);
    s.Run(0.2f);
}
// That empty hand squeezes at that place (seen, still), holding the squeeze.
void SqueezeAt(Sim& s, int side, const float at[3], float hold = 0.3f) {
    s.hand[side].squeeze = 0.0f;
    s.Move(side, at);
    s.Run(0.15f);
    s.hand[side].squeeze = 1.0f;
    s.Run(hold);
}
int AwayCount(const Sim& s) {
    int n = 0;
    for (const Away& a : s.st.away)
        n += a.on;
    return n;
}
const Away* AwayOf(const Sim& s, uint32_t kind) {
    for (const Away& a : s.st.away)
        if (a.on && a.kind == kind)
            return &a;
    return nullptr;
}
std::string Line(const Event* e) {
    if (!e)
        return "-";
    char line[320];
    Say(*e, kNames, line, sizeof(line));
    return line;
}
// ... with the names the slots have now (hands.cpp's HolsterNames, from the slots: the review's finding 11, a fixed table named the AN-94 the Stinger).
std::string LineIn(const Sim& s, const Event* e) {
    if (!e)
        return "-";
    const char* names[kSlots];
    for (int k = 0; k < kSlots; ++k)
        names[k] = s.slots[k].carried && s.slots[k].name[0] ? s.slots[k].name : kNames[k];
    char line[400];
    Say(*e, names, line, sizeof(line));
    return line;
}

// An event found, kept (the Sim's list grows with the frames after, and may move): one of a few slots.
const Event* Kept(const Event* e, int at) {
    static Event kept[4];
    if (!e)
        return nullptr;
    kept[at] = *e;
    return &kept[at];
}

void AwayRows() {
    {
        // The user's flow (a), a pickup's AT4: the rifle put on the right shoulder by the hand; the AT4 taken off the floor (the game puts the gun of the
        // slot it names, the M416's 0, into its pickup: the take goes ahead, the M416 stays on its shoulder, in that pickup, with its rounds); fired,
        // thrown away; a draw at the right shoulder: the game's own take of that pickup gives the M416 back into slot 0 with its own rounds, into the hand
        // that drew it, and the AT4 on the ground goes into the pickup; put back on the shoulder, it's there again.
        World w;
        Sim s = RockWorld(w);
        w.loaded[0] = 27, w.spare[0] = 120;
        const int at4 = w.AddPick(0xE1, 1.2f, 0, kAt4Kind, 1, 0, true, "AT4");
        PutOn(s, 1, kRS);
        const bool holstered = s.holder == Holder::kStowed && s.st.home[0].role == Role::kPrimary;
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.3f);
        const TakeOut check = TakeCheck(s.set, s.slots, s.st, s.holder, 1u << 0, false, false, false);
        TakeOf(s, w, at4, 1);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.3f);
        const Away* away = AwayOf(s, kM416Kind);
        const bool kept = away && away->slot == 0 && away->role == Role::kPrimary && away->entity == 0xE1 && w.pick[at4].gun.kind == kM416Kind &&
                          w.pick[at4].gun.loaded == 27 && w.pick[at4].gun.spare == 120 && s.slots[0].kind == kAt4Kind &&
                          s.st.home[0].role == Role::kNone && s.holder == Holder::kHeld && s.trigger == 1;
        const Event* said = s.Find(What::kAway);
        Report("HA1", "the user's AT4 (a pickup's), flow (a) 1: the M416 holstered, the AT4 taken: the take goes ahead (not refused), the M416 stays at the "
                      "right shoulder, in the AT4's pickup with its rounds (27 + 120), said once; the AT4 in the hand that took it, on no shoulder",
               holstered && check.verdict == TakeVerdict::kAway && check.slot == 0 && check.at == Place::kRightShoulder && kept && said &&
                   s.Count(What::kAway) == 1 && !said->weak && AwayPlace(s.slots, s.st, s.set, 0) == Place::kRightShoulder,
               "holstered %d; verdict %d (slot %d %s); away %s; the pickup %X %d+%d; \"%s\"", holstered, static_cast<int>(check.verdict), check.slot,
               Name(check.at), away ? Name(PlaceOfRole(away->role, false, s.set)) : "-", w.pick[at4].gun.kind, w.pick[at4].gun.loaded,
               w.pick[at4].gun.spare, Line(said).c_str());
        // Fired (nothing for holsters), then thrown away in front: it lies, a picked-up gun (the memo: no holster of its own).
        LetGo(s, 1);
        s.Run(0.5f);
        const bool thrown = s.holder == Holder::kLoose && Discardable(s.st, kAt4Kind) && !Discardable(s.st, kM416Kind);
        // The left hand squeezes at the right shoulder.
        const int ticksBefore = Ticks(s, 0);
        SqueezeAt(s, 0, kRS);
        const Event* asked = s.Find(What::kAsked, 0);
        const Event* back = s.Find(What::kComeBack);
        Report("HA2", "flow (a) 2: the AT4 fired and thrown away; a draw at the right shoulder (a tick there first): the game's own take of the M416's "
                      "pickup, the M416 back in slot 0 with its own rounds, in the hand that drew it, its holster the right shoulder; the AT4 in that "
                      "pickup on the ground; nothing away any more",
               thrown && Ticks(s, 0) == ticksBefore + 1 && asked && asked->giveBack && w.asked == 1 && w.served == 2 && s.slots[0].kind == kM416Kind &&
                   w.loaded[0] == 27 && w.spare[0] == 120 && s.holder == Holder::kHeld && s.trigger == 0 && s.st.home[0].role == Role::kPrimary &&
                   AwayCount(s) == 0 && w.pick[at4].gun.kind == kAt4Kind && back && !back->weak && back->place == Place::kRightShoulder && s.Keys() == 0,
               "thrown %d; ticks %d; asked %d served %d; slot 0 %X with %d+%d, holder %d by %d, %s; away %d; the pickup %X; \"%s\" | \"%s\"", thrown,
               Ticks(s, 0) - ticksBefore, w.asked, w.served, s.slots[0].kind, w.loaded[0], w.spare[0], static_cast<int>(s.holder), s.trigger,
               RoleName(s.st.home[0].role), AwayCount(s), w.pick[at4].gun.kind, Line(asked).c_str(), Line(back).c_str());
        // Put back on the right shoulder by the left hand: its holster, as before the AT4.
        s.armed[0] = true;
        PutOn(s, 0, kRS);
        Report("HA3", "flow (a) 3: the M416 put back on the right shoulder by the hand: held there, the primary", s.holder == Holder::kStowed &&
                                                                                                              s.st.home[0].role == Role::kPrimary,
               "holder %d, %s", static_cast<int>(s.holder), RoleName(s.st.home[0].role));
    }
    {
        // The user's flow (b): the rifle dropped on the floor (no holster any more, but the player's own: it stays where it lies); the AT4 taken off the
        // floor: the game puts the M416 lying in slot 0 into the AT4's pickup, where it stays on the ground (nothing away: it wasn't holstered); fired,
        // thrown; the rifle picked up again off the floor (the AT4 into its pickup), with its own rounds, in the hand, on no shoulder till a hand puts it
        // on one.
        World w;
        Sim s = RockWorld(w);
        w.loaded[0] = 22, w.spare[0] = 90;
        const int at4 = w.AddPick(0xE2, 1.0f, 0, kAt4Kind, 1, 0, true, "AT4");
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.1f);
        LetGo(s, 1);
        s.Run(0.5f);
        const bool dropped = s.holder == Holder::kLoose && s.st.home[0].role == Role::kNone;
        const TakeOut check = TakeCheck(s.set, s.slots, s.st, s.holder, 1u << 0, false, false, false);
        TakeOf(s, w, at4, 0);
        s.hand[0].squeeze = 1.0f;
        s.Run(0.3f);
        const bool swapped = s.slots[0].kind == kAt4Kind && w.pick[at4].gun.kind == kM416Kind && w.pick[at4].gun.loaded == 22 && AwayCount(s) == 0 &&
                             s.Count(What::kAway) == 0 && s.holder == Holder::kHeld && s.trigger == 0;
        LetGo(s, 0);
        s.Run(0.5f);
        // The right shoulder holds nothing now (the M416 lies in the pickup, where the AT4 lay): a squeeze there draws nothing, asks nothing.
        SqueezeAt(s, 1, kRS, 0.6f);
        const Event* nothing = Kept(s.Find(What::kRefused, 1), 0);
        s.hand[1].squeeze = 0.0f;
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.3f);
        const TakeOut again = TakeCheck(s.set, s.slots, s.st, s.holder, 1u << 0, false, false, false);
        TakeOf(s, w, at4, 1);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.3f);
        const bool picked = s.slots[0].kind == kM416Kind && w.loaded[0] == 22 && w.spare[0] == 90 && w.pick[at4].gun.kind == kAt4Kind &&
                            s.holder == Holder::kHeld && s.trigger == 1 && s.st.home[0].role == Role::kNone;
        PutOn(s, 1, kRS);
        Report("HA4", "the user's flow (b): the rifle dropped, the AT4 taken (the lying M416 into its pickup: nothing refused, nothing away), fired, "
                      "thrown; nothing at the right shoulder meanwhile; the rifle picked up again off the floor with its own rounds (22 + 90), on no "
                      "shoulder; put on the right shoulder by the hand: the primary again",
               dropped && check.verdict == TakeVerdict::kGo && swapped && nothing && nothing->why == Why::kNothing && w.asked == 0 &&
                   again.verdict == TakeVerdict::kGo && picked && s.holder == Holder::kStowed && s.st.home[0].role == Role::kPrimary,
               "dropped %d; verdicts %d %d; swapped %d; \"%s\", asked %d; picked up %d (%d+%d, %s); put back %s", dropped,
               static_cast<int>(check.verdict), static_cast<int>(again.verdict), swapped, Line(nothing).c_str(), w.asked, picked, w.loaded[0],
               w.spare[0], RoleName(s.st.home[0].role), RoleName(s.st.home[0].role));
    }
    {
        // Operation Swordbreaker's roof AT4 (a script's kit, no pickup: PlayerAT4 puts the M136 into slot 0 in place of the M4A1), the M4A1 holstered
        // first: the AT4 has no holster (the review's RA: it went behind the neck by itself), the M4A1 stays at the right shoulder with the game (said);
        // a draw there meanwhile: not back yet (said, felt), never key 1; the AT4 fired and thrown; the level's kit again (a fresh M4A1 into slot 0, no
        // take: the review's RD): back at the right shoulder, put away there (said), drawn from there as ever.
        World w;
        Sim s = SwordWorld(w);
        PutOn(s, 1, kRS);
        const bool holstered = s.holder == Holder::kStowed && s.st.home[0].role == Role::kPrimary && s.st.home[1].role == Role::kSecondary;
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(20.0f);
        Gave(s, 0, 0xA740, kAt4Kind, true, true);
        CopyName(s.slots[0].name, "AT4");
        const ArriveOut o = s.Switch(0);
        s.hand[1].squeeze = 1.0f;
        s.armed[1] = true;
        s.Run(1.2f);  // (a record with the game said a moment on: a take's swap may still tell its pickup)
        const Away* away = AwayOf(s, kM4Kind);
        const Event* said = Kept(s.Find(What::kAway), 1);
        const bool given = o.arrival == Arrival::kHand && o.side == 1 && s.st.home[0].role == Role::kNone && away && !away->entity &&
                           away->role == Role::kPrimary && said && said->weak;
        LetGo(s, 1);
        s.Run(0.5f);
        SqueezeAt(s, 0, kRS, 0.6f);
        const Event* notBack = Kept(s.Find(What::kRefused, 0), 2);
        s.hand[0].squeeze = 0.0f;
        s.MoveTo(0, kFront[0], kFront[1], kFront[2]);
        s.Run(0.3f);
        SqueezeAt(s, 1, kNeckAt, 0.6f);  // (behind the neck: nothing there, the AT4 never holstered)
        s.hand[1].squeeze = 0.0f;
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.3f);
        Gave(s, 0, 0x4A11, kM4Kind);
        CopyName(s.slots[0].name, "M4A1");
        const ArriveOut k = s.Switch(0);
        s.Run(0.3f);
        const Event* back = Kept(s.Find(What::kComeBack), 3);
        const bool atPlace = k.arrival == Arrival::kAtPlace && s.holder == Holder::kStowed && s.st.home[0].role == Role::kPrimary && AwayCount(s) == 0 &&
                             back && back->weak && back->place == Place::kRightShoulder;
        SqueezeAt(s, 1, kRS);
        Report("HA5", "Swordbreaker's roof AT4 by its kit, the M4A1 holstered: the AT4 on no shoulder, in the right hand; the M4A1 stays at the right "
                      "shoulder with the game (said); a draw there: not back yet (felt), no key; the level's kit again: the M4A1 back at its place (said), "
                      "then drawn from there",
               holstered && given && notBack && notBack->why == Why::kWithGame && s.Pulses(0, kDoubleAmp) >= 2 && s.Keys() == 0 && atPlace &&
                   s.holder == Holder::kHeld && s.trigger == 1 && s.draws >= 1,
               "holstered %d; given %d (arrival %d, AT4 %s, M4A1 away %s); \"%s\"; \"%s\"; back %d (\"%s\"); drawn %d, keys %d", holstered, given,
               static_cast<int>(o.arrival), RoleName(s.st.home[0].role), away ? RoleName(away->role) : "-", Line(said).c_str(), Line(notBack).c_str(),
               atPlace, Line(back).c_str(), s.draws, s.Keys());
    }
    {
        // The review's RB: a gun taken off the ground and never put on a shoulder stays on none after a death and its checkpoint (NewLife keeps the memo);
        // the kit's guns keep theirs. Another level's kit (no kind the memo knows): every gun's holster by its slot again.
        World w;
        Sim s = RockWorld(w);
        PutOn(s, 1, kRS);
        Took(s, 1, 0x9401, kAn94Kind);
        CopyName(s.slots[1].name, "AN-94");
        s.Run(0.5f);
        const Role before = s.st.home[1].role;
        NewLife(s.st);
        for (int k = 0; k < kSlots; ++k)
            if (s.slots[k].carried)
                s.slots[k].id += 0x100000;
        s.holder = Holder::kHeld, s.trigger = 1, s.gameSlot = 0;
        s.Run(0.5f);
        const bool kept = s.st.home[1].role == Role::kNone && s.st.home[0].role == Role::kPrimary && s.st.home[5].role == Role::kLauncher &&
                          PlaceNow(s, 5) == Place::kLeftShoulder;
        Sim n = s;
        NewLife(n.st);
        for (SlotIn& v : n.slots)
            v = SlotIn{};
        n.slots[0] = {true, false, true};
        Named(n.slots[0], 0x4A20, kM4Kind, "M4A1");
        n.slots[1] = {true, false, true};
        Named(n.slots[1], 0x1120, kMk11Kind, "Mk11");
        n.Run(0.3f);
        Report("HA6", "the review's RB: a picked-up AN-94 never put on a shoulder is on none after a death (its checkpoint's kit), the M416 still the "
                      "primary, the SMAW the launcher's; another level's kit (nothing the memo knows): each by its slot",
               before == Role::kNone && kept && n.st.home[0].role == Role::kPrimary && n.st.home[1].role == Role::kSecondary,
               "before %s; after: the AN-94 %s, the M416 %s, the SMAW %s (%s); another level: %s, %s", RoleName(before), RoleName(s.st.home[1].role),
               RoleName(s.st.home[0].role), RoleName(s.st.home[5].role), Name(PlaceNow(s, 5)), RoleName(n.st.home[0].role),
               RoleName(n.st.home[1].role));
    }
    {
        // The review's RC: one gun a shoulder. The M416 on the right shoulder; an AN-94 taken into the right hand: no tick at the right shoulder; let go
        // there it stays in the hand (said once, the double click: never dropped, review finding 1), on no shoulder; squeezed again and put on the left:
        // the secondary (the SMAW behind the neck).
        World w;
        Sim s = RockWorld(w);
        PutOn(s, 1, kRS);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.5f);
        Took(s, 1, 0x9401, kAn94Kind);
        CopyName(s.slots[1].name, "AN-94");
        s.Switch(1, true);
        s.Step();
        s.hand[1].squeeze = 1.0f;
        s.armed[1] = true;
        s.Run(0.3f);
        const int ticks = Ticks(s, 1);
        s.Move(1, kRS);
        s.Run(0.15f);
        const int ticked = Ticks(s, 1) - ticks;
        const float from = s.time;
        LetGo(s, 1);
        s.Run(0.4f);  // (the grip stays open there)
        const Event* refused = Kept(s.Find(What::kStowRefused, 1), 0);
        const bool kept = s.holder == Holder::kHeld && s.trigger == 1 && s.falls == 0 && s.st.home[1].role == Role::kNone &&
                          s.st.home[0].role == Role::kPrimary && s.everHold[1];
        const int pulses = s.Pulses(1, kDoubleAmp, from), said = s.Count(What::kStowRefused, 1);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.2f);
        PutOn(s, 1, kLS);
        Report("HA7", "the review's RC: one gun a shoulder: the AN-94 at the M416's right shoulder: no tick, a let-go there keeps it in the hand (said "
                      "once, the double click), on no shoulder; squeezed again and put on the left shoulder: the secondary there, the SMAW behind the neck",
               ticked == 0 && refused && refused->why == Why::kTaken && refused->kept && kept && pulses == 2 && said == 1 && s.holder == Holder::kStowed &&
                   s.st.home[1].role == Role::kSecondary && PlaceNow(s, 5) == Place::kNeck && s.falls == 0,
               "ticks %d; \"%s\"; kept %d, pulses %d, said %d; then %s, the SMAW %s", ticked, LineIn(s, refused).c_str(), kept, pulses, said,
               RoleName(s.st.home[1].role), Name(PlaceNow(s, 5)));
    }
    {
        // An away gun gives its shoulder up to a put-away (review finding 1: a shoulder never blocked for good): the M416 swapped into an AN-94's pickup
        // (naming slot 0) is away at the right shoulder; the AN-94 there ticks and is put away, the primary; the M416 on no shoulder (said: it stays in
        // its pickup on the ground, which still holds it), its record gone; a draw at the right shoulder draws the AN-94, nothing asked of the game.
        World w;
        Sim s = RockWorld(w);
        const int an = w.AddPick(0xE3, 1.0f, 0, kAn94Kind, 30, 90, false, "AN-94");
        PutOn(s, 1, kRS);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.3f);
        TakeOf(s, w, an, 1);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.3f);
        const bool away = AwayOf(s, kM416Kind) != nullptr;
        const int ticks = Ticks(s, 1);
        s.Move(1, kRS);
        s.Run(0.15f);
        const int ticked = Ticks(s, 1) - ticks;
        LetGo(s, 1);
        s.Run(0.2f);
        const Event* gave = Kept(s.Find(What::kLost), 0);
        const bool won = s.holder == Holder::kStowed && s.st.home[0].role == Role::kPrimary && !AwayOf(s, kM416Kind) && w.pick[an].gun.kind == kM416Kind &&
                         gave && gave->why == Why::kTaken && !gave->weak && s.Count(What::kStowRefused) == 0 && s.falls == 0;
        const Memo* memo = MemoOf(s.st, kM416Kind);
        SqueezeAt(s, 0, kRS);
        Report("HA8", "an away gun gives its shoulder up: the AN-94 put on the right shoulder where the M416 is away in its pickup: a tick, put away "
                      "there (the primary); the M416 on no shoulder (said), still in its pickup; drawn there again: the AN-94, nothing asked of the game",
               away && ticked == 1 && won && memo && memo->role == Role::kNone && s.holder == Holder::kHeld && s.trigger == 0 && w.asked == 0 &&
                   s.slots[0].kind == kAn94Kind &&
                   LineIn(s, gave) == "Holster: the M416 is on no shoulder now: the AN-94 was put on the right shoulder (one gun a shoulder); it stays in "
                                      "the pickup where the AN-94 lay (take it off the floor there)",
               "away %d, ticks %d, won %d, the M416's memo %s; drawn: holder %d by %d, asked %d; \"%s\"", away, ticked, won, memo ? RoleName(memo->role) : "-",
               static_cast<int>(s.holder), s.trigger, w.asked, LineIn(s, gave).c_str());
    }
    {
        // A draw of an away gun that can't come: its pickup 4 m away (no tick at its shoulder; said with how distant, felt; nothing asked); the game says no
        // (said with its words at the next frame, felt); its take never answered (given up at 3 s, said); the other hand holding the gun (silent).
        World w;
        Sim s = RockWorld(w);
        const int at4 = w.AddPick(0xE4, 1.2f, 0, kAt4Kind, 1, 0, true, "AT4");
        PutOn(s, 1, kRS);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.3f);
        TakeOf(s, w, at4, 1);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.3f);
        LetGo(s, 1);
        s.Run(0.5f);
        Sim distant = s;
        World wd = w;
        distant.world = &wd;
        wd.pick[at4].distance = 4.0f;
        distant.Run(0.1f);
        const int ticks = Ticks(distant, 0);
        const float from = distant.time;
        SqueezeAt(distant, 0, kRS, 0.6f);
        const Event* tooFar = distant.Find(What::kRefused, 0);
        const bool farOk = Ticks(distant, 0) == ticks && tooFar && tooFar->why == Why::kFar && std::fabs(tooFar->distance - 4.0f) < 0.01f &&
                           distant.Pulses(0, kDoubleAmp, from) == 2 && wd.asked == 0 && AwayOf(distant, kM416Kind);
        Sim no = s;
        World wn = w;
        no.world = &wn;
        wn.refuse = true;
        const float noFrom = no.time;
        SqueezeAt(no, 0, kRS, 0.3f);
        const Event* said = no.Find(What::kRefused, 0);
        const bool noOk = said && said->why == Why::kGiveBack && std::strcmp(said->words, "the game doesn't allow it now") == 0 && wn.asked == 1 &&
                          wn.served == 1 && no.Pulses(0, kDoubleAmp, noFrom) == 3 && AwayOf(no, kM416Kind) && !no.st.wait.on;  // (the click, then the double)
        Sim never = s;
        World wv = w;
        never.world = &wv;
        wv.never = true;
        SqueezeAt(never, 0, kRS, 3.3f);
        const Event* late = never.Find(What::kNoSwitch, 0);
        const bool lateOk = late && late->giveBack && !never.st.wait.on && AwayOf(never, kM416Kind) && wv.asked == 1;
        Sim held = s;
        World wh = w;
        held.world = &wh;
        held.hand[1].squeeze = 1.0f;
        held.takeBy = 1;  // (the AT4 back in the right hand)
        held.Run(0.2f);
        SqueezeAt(held, 0, kRS, 0.3f);
        const Event* holds = held.Find(What::kRefused, 0);
        const bool heldOk = holds && holds->why == Why::kOtherHolds && holds->quiet && wh.asked == 0;
        Report("HA9", "an away gun's draw that can't come: 4 m from its pickup (no tick, said with how distant, felt, not asked); the game says no (its "
                      "words, felt); never answered (3 s, said); the other hand holding the AT4 (silent): the M416 stays at its shoulder each time",
               farOk && noOk && lateOk && heldOk, "distant %d (\"%s\"); no %d (\"%s\"); never %d (\"%s\"); held %d", farOk, Line(tooFar).c_str(), noOk,
               Line(said).c_str(), lateOk, Line(late).c_str(), heldOk);
    }
    {
        // An away gun's pickup gone (3 s missing from the pickups seen: said, the shoulder free), or holding another gun now (said); a take whose pickup
        // went with it (nothing could stay in it: said at once, no record).
        World w;
        Sim s = RockWorld(w);
        const int at4 = w.AddPick(0xE5, 1.2f, 0, kAt4Kind, 1, 0, true, "AT4");
        PutOn(s, 1, kRS);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.3f);
        TakeOf(s, w, at4, 1);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.5f);
        Sim gone = s;
        World wg = w;
        gone.world = &wg;
        wg.pick[at4].gone = true;
        gone.Run(2.0f);
        const bool stillAt2 = AwayOf(gone, kM416Kind) != nullptr;
        gone.Run(2.0f);
        const Event* lost = gone.Find(What::kLost);
        Sim other = s;
        World wo = w;
        other.world = &wo;
        wo.pick[at4].gun.kind = kAkKind;
        other.Run(4.0f);
        const Event* holdsOther = other.Find(What::kLost);
        Sim with = s;
        SwapIn in;
        in.changed = 1, in.slot[0] = 5, in.came[0] = 0x777, in.went[0] = kSmawKind, in.wentBlueprint[0] = kSmawKind + kBlueprintOf, in.entity = 0xE6,
        in.removed = true;
        CopyName(in.taken, "RPG-7");
        with.st.lastSlot = 0;  // (the AT4 in the hands: the SMAW hangs at its place)
        NoteSwap(with.set, with.slots, in, with.st);
        with.Step();
        const Event* went = with.Find(What::kLost);
        Report("HA10", "an away gun's pickup gone 3 s (said, nothing at the shoulder), or holding another gun (said); a holstered gun whose take's pickup "
                       "went with it: said at once, no record",
               stillAt2 && lost && lost->why == Why::kFar && !AwayOf(gone, kM416Kind) && holdsOther && holdsOther->why == Why::kNothing && went &&
                   went->why == Why::kGone && !AwayOf(with, kSmawKind),
               "at 2 s %d; \"%s\"; \"%s\"; \"%s\"", stillAt2, Line(lost).c_str(), Line(holdsOther).c_str(), Line(went).c_str());
    }
    {
        // TakeCheck: what goes ahead and what is refused before the game is asked (the M416 holstered at the right shoulder, the SMAW at the left).
        World w;
        Sim s = RockWorld(w);
        PutOn(s, 1, kRS);
        const auto v = [&](unsigned victims, bool gone, bool all, bool unread) {
            return TakeCheck(s.set, s.slots, s.st, s.holder, victims, gone, all, unread);
        };
        const TakeOut away = v(1u << 0, false, false, false), launcher = v(1u << 5, false, false, false), both = v((1u << 0) | (1u << 2), false, false, false);
        const TakeOut gone = v(1u << 0, true, false, false), all = v(1u << 0, false, true, false), unread = v(0x3E7u, false, false, true);
        const TakeOut part = v(1u << 2, false, false, false), empty = v(1u << 1, false, false, false), gift = v(1u << 1, true, false, false);
        Settings off = s.set;
        off.on = false;
        const TakeOut none = TakeCheck(off, s.slots, s.st, s.holder, 1u << 0, true, false, false);
        Sim d = RockWorld(w);
        d.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        d.Run(0.1f);
        LetGo(d, 1);
        const TakeOut lying = TakeCheck(d.set, d.slots, d.st, d.holder, 1u << 0, true, false, false);
        Report("HA11", "TakeCheck: a holstered gun among the guns a take takes out goes away (the M416, the SMAW, the M416 with its M320); refused when "
                       "it couldn't be given back (the pickup destroys it, takes every gun, has entries not read, or takes the M416's M320 alone); an "
                       "empty slot or a gun on the ground: ahead; holsters 0: ahead",
               away.verdict == TakeVerdict::kAway && away.slot == 0 && away.at == Place::kRightShoulder && launcher.verdict == TakeVerdict::kAway &&
                   launcher.at == Place::kLeftShoulder && both.verdict == TakeVerdict::kAway && gone.verdict == TakeVerdict::kRefuse &&
                   gone.why == Why::kGone && all.verdict == TakeVerdict::kRefuse && all.why == Why::kAll && unread.verdict == TakeVerdict::kRefuse &&
                   unread.why == Why::kUnread && part.verdict == TakeVerdict::kRefuse && part.why == Why::kPart && empty.verdict == TakeVerdict::kGo &&
                   gift.verdict == TakeVerdict::kGo && none.verdict == TakeVerdict::kGo && lying.verdict == TakeVerdict::kGo && !away.lost && !gone.lost,
               "away %d %s, launcher %d %s, both %d; gone %d, all %d, unread %d, part %d; empty %d, the Stinger's slot empty %d; off %d; lying %d",
               static_cast<int>(away.verdict), Name(away.at), static_cast<int>(launcher.verdict), Name(launcher.at), static_cast<int>(both.verdict),
               static_cast<int>(gone.verdict), static_cast<int>(all.verdict), static_cast<int>(unread.verdict), static_cast<int>(part.verdict),
               static_cast<int>(empty.verdict), static_cast<int>(gift.verdict), static_cast<int>(none.verdict), static_cast<int>(lying.verdict));
        // The level's own pickups (required: not a soldier's drop) are never refused (review finding 6): the same takes go ahead, the holstered gun
        // marked lost (gone, every gun, entries not read); the M320 alone goes ahead and nothing is lost; a plain one that keeps it goes away as before.
        const auto q = [&](unsigned victims, bool gone, bool all, bool unread) {
            return TakeCheck(s.set, s.slots, s.st, s.holder, victims, gone, all, unread, true);
        };
        const TakeOut rGone = q(1u << 0, true, false, false), rAll = q(1u << 0, false, true, false), rUnread = q(0x3E7u, false, false, true);
        const TakeOut rPart = q(1u << 2, false, false, false), rAway = q(1u << 0, false, false, false), rEmpty = q(1u << 1, true, false, false);
        Report("HA11b", "TakeCheck for the level's own pickups: never refused; a holstered gun it destroys, can't give back alone or may take is lost (its "
                        "slot, place and why for the line); the M320 alone, ahead; one that keeps it, away; an empty slot, ahead",
               rGone.verdict == TakeVerdict::kAway && rGone.lost && rGone.why == Why::kGone && rGone.slot == 0 && rGone.at == Place::kRightShoulder &&
                   rAll.verdict == TakeVerdict::kAway && rAll.lost && rAll.why == Why::kAll && rUnread.verdict == TakeVerdict::kAway && rUnread.lost &&
                   rUnread.why == Why::kUnread && rPart.verdict == TakeVerdict::kGo && !rPart.lost && rAway.verdict == TakeVerdict::kAway && !rAway.lost &&
                   rEmpty.verdict == TakeVerdict::kGo,
               "gone %d lost %d, all %d lost %d, unread %d lost %d, part %d, away %d lost %d, empty %d", static_cast<int>(rGone.verdict), rGone.lost,
               static_cast<int>(rAll.verdict), rAll.lost, static_cast<int>(rUnread.verdict), rUnread.lost, static_cast<int>(rPart.verdict),
               static_cast<int>(rAway.verdict), rAway.lost, static_cast<int>(rEmpty.verdict));
    }
    {
        // Rock's ending: the Stinger (UnspawnOnPickup, slot 1) with a picked-up AN-94 put on the left shoulder: refused (it would be gone), as the first
        // build; the AN-94 taken out and dropped first: ahead.
        World w;
        Sim s = RockWorld(w);
        PutOn(s, 1, kRS);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.5f);
        Took(s, 1, 0x9401, kAn94Kind);
        CopyName(s.slots[1].name, "AN-94");
        s.Switch(1, true);
        s.Step();
        s.hand[1].squeeze = 1.0f;
        s.armed[1] = true;
        s.Run(0.3f);
        PutOn(s, 1, kLS);
        const bool onLeft = s.st.home[1].role == Role::kSecondary;
        const TakeOut plain = TakeCheck(s.set, s.slots, s.st, s.holder, 1u << 1, true, false, false);
        const TakeOut stinger = TakeCheck(s.set, s.slots, s.st, s.holder, 1u << 1, true, false, false, true);
        // X held's R: the game takes it (UnspawnOnPickup: the pickup goes, the AN-94 with it, nothing put in): said, nothing away; the Stinger in slot 1
        // on no shoulder; the SMAW back at the left shoulder (no secondary there now).
        constexpr uint32_t kStingerKind = 0x5719;
        SwapIn in;
        in.changed = 1, in.slot[0] = 1, in.came[0] = kStingerKind, in.went[0] = kAn94Kind, in.wentBlueprint[0] = kAn94Kind + kBlueprintOf;
        in.entity = 0xEA, in.removed = true, in.noPlace = true;
        CopyName(in.taken, "Stinger");
        NoteSwap(s.set, s.slots, in, s.st);
        Took(s, 1, 0x5791, kStingerKind, true, true);
        CopyName(s.slots[1].name, "Stinger");
        s.gameSlot = 1;
        s.Switch(1, true);
        s.Run(0.3f);
        const Event* gone = Kept(s.Find(What::kLost), 0);
        Report("HA12", "Rock's Stinger (the level's own: it destroys the gun of the slot it names, 1) with the picked-up AN-94 on the left shoulder: never "
                       "refused (review finding 6: the jet needs it), the AN-94 lost; taken: the AN-94 gone (said), nothing away, the Stinger on no "
                       "shoulder, the SMAW at the left shoulder; a soldier's drop that would destroy it: still refused",
               onLeft && plain.verdict == TakeVerdict::kRefuse && plain.why == Why::kGone && stinger.verdict == TakeVerdict::kAway && stinger.lost &&
                   stinger.why == Why::kGone && stinger.slot == 1 && stinger.at == Place::kLeftShoulder && gone && gone->why == Why::kGone &&
                   AwayCount(s) == 0 && s.st.home[1].role == Role::kNone && PlaceNow(s, 1) == Place::kNone && PlaceNow(s, 5) == Place::kLeftShoulder &&
                   LineIn(s, gone) == "Holster: the AN-94 went with the pickup the game took (where the Stinger lay: it keeps no place for the gun it "
                                      "replaces): it's gone",
               "on the left %d; plain %d; the Stinger %d lost %d slot %d %s; \"%s\"; away %d, the Stinger %s, the SMAW %s", onLeft,
               static_cast<int>(plain.verdict), static_cast<int>(stinger.verdict), stinger.lost, stinger.slot, Name(stinger.at), LineIn(s, gone).c_str(),
               AwayCount(s), RoleName(s.st.home[1].role), Name(PlaceNow(s, 5)));
    }
    {
        // Two guns sharing one slot through one pickup: the AT4 put away at the launcher's place, the M416 drawn back from the right shoulder (the AT4,
        // holstered, goes into the pickup and stays at its place, away in turn); the M416 put away, the AT4 drawn back from the left shoulder (held
        // last there: before the SMAW), the M416 away again.
        World w;
        Sim s = RockWorld(w);
        const int at4 = w.AddPick(0xE7, 1.0f, 0, kAt4Kind, 1, 2, true, "AT4");
        w.loaded[0] = 30, w.spare[0] = 150;
        PutOn(s, 1, kRS);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.3f);
        TakeOf(s, w, at4, 1);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.3f);
        PutOn(s, 1, kLS);
        const bool at4Away = s.holder == Holder::kStowed && s.st.home[0].role == Role::kLauncher && AwayOf(s, kM416Kind);
        SqueezeAt(s, 1, kRS);
        const Away* a4 = AwayOf(s, kAt4Kind);
        const bool m416Back = s.slots[0].kind == kM416Kind && s.st.home[0].role == Role::kPrimary && w.loaded[0] == 30 && a4 &&
                              a4->role == Role::kLauncher && a4->entity == 0xE7 && !AwayOf(s, kM416Kind) && s.holder == Holder::kHeld &&
                              AwayPlace(s.slots, s.st, s.set, static_cast<int>(a4 - s.st.away)) == Place::kLeftShoulder;
        PutOn(s, 1, kRS);
        SqueezeAt(s, 1, kLS);
        const Away* m4 = AwayOf(s, kM416Kind);
        const bool at4Back = s.slots[0].kind == kAt4Kind && s.st.home[0].role == Role::kLauncher && w.loaded[0] == 1 && w.spare[0] == 2 && m4 &&
                             m4->role == Role::kPrimary && s.holder == Holder::kHeld && s.Keys(5) == 0;
        Report("HA13", "one pickup, two guns of one slot: the AT4 put away, the M416 drawn back from the right shoulder (the AT4 away at the left in "
                       "turn), the M416 put away, the AT4 drawn back from the left (held there last, before the SMAW): each with its own rounds",
               at4Away && m416Back && at4Back, "AT4 put away %d; M416 back %d (%d rounds); AT4 back %d (%d+%d), SMAW key %d", at4Away, m416Back,
               w.loaded[0], at4Back, w.loaded[0], w.spare[0], s.Keys(5));
    }
    {
        // A picked-up gun let go of on the ground goes into its slot for a draw of another (the game draws one gun at a time: said); the player's own gun
        // lying stays there (a draw is refused till it's taken: D7, HB16).
        World w;
        Sim s = RockWorld(w);
        PutOn(s, 1, kRS);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.5f);
        Took(s, 1, 0x9401, kAn94Kind);
        CopyName(s.slots[1].name, "AN-94");
        s.Switch(1, true);
        s.Step();
        s.hand[1].squeeze = 1.0f;
        s.armed[1] = true;
        s.Run(0.3f);
        LetGo(s, 1);
        s.Run(0.5f);
        const bool lies = s.holder == Holder::kLoose && Discardable(s.st, kAn94Kind);
        SqueezeAt(s, 0, kRS, 0.2f);
        const int keys = s.Keys(0);
        s.gameSlot = 0;
        const ArriveOut o = s.Switch(0);
        s.Run(0.2f);
        Report("HA14", "a picked-up gun let go of lies: a draw at the right shoulder sends the M416's key, the AN-94 goes into its slot (said: on no "
                       "shoulder), the M416 into the hand that drew it",
               lies && keys == 1 && o.arrival == Arrival::kHand && o.side == 0 && o.event.what == What::kDiscard && s.holder == Holder::kHeld &&
                   s.trigger == 0 && s.st.home[0].role == Role::kPrimary,
               "lies %d, key 1: %d; arrival %d into %d; \"%s\"", lies, keys, static_cast<int>(o.arrival), o.side, Line(&o.event).c_str());
    }
    {
        // Free hands turned off and on (Reset): every gun keeps its holster, the picked-up one none, the clock on (no kit window again); Clear (a new
        // level's start) would give the kit's.
        World w;
        Sim s = RockWorld(w);
        PutOn(s, 1, kRS);
        Took(s, 1, 0x9401, kAn94Kind);
        CopyName(s.slots[1].name, "AN-94");
        s.Run(11.0f);
        Reset(s.st);
        s.holder = Holder::kHeld, s.trigger = 1;
        s.Run(0.5f);
        Report("HA15", "free hands turned off and on (Reset, the Insert key): the M416 the primary still, the picked-up AN-94 none (the review: a wipe made "
                       "it the kit's)",
               s.st.home[0].role == Role::kPrimary && s.st.home[1].role == Role::kNone && s.st.clock > kKitSeconds,
               "the M416 %s, the AN-94 %s, clock %.1f", RoleName(s.st.home[0].role), RoleName(s.st.home[1].role), s.st.clock);
    }
    {
        // The take's swap told a frame after its new gun was seen (the slot read first): the weak record NoteSlots made gets its pickup, said once as in
        // a pickup; the new gun on no shoulder (the take's mark a frame late).
        World w;
        Sim s = RockWorld(w);
        PutOn(s, 1, kRS);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(11.0f);
        Gave(s, 0, 0x9402, kAn94Kind);
        CopyName(s.slots[0].name, "AN-94");
        s.Step();
        const Away* early = AwayOf(s, kM416Kind);
        const bool weakFirst = early && !early->entity && s.Count(What::kAway) == 0;
        SwapIn in;
        in.changed = 1, in.slot[0] = 0, in.came[0] = kAn94Kind, in.went[0] = kM416Kind, in.wentBlueprint[0] = kM416Kind + kBlueprintOf, in.entity = 0xE8;
        CopyName(in.taken, "AN-94");
        NoteSwap(s.set, s.slots, in, s.st);
        s.taken = 1u << 0;
        s.Run(0.3f);
        const Away* a = AwayOf(s, kM416Kind);
        const Event* said = s.Find(What::kAway);
        Report("HA16", "a take's swap told a frame after its new gun: the M416's record gets its pickup, said once (in the pickup, not \"with the "
                       "game\"); the AN-94 on no shoulder",
               weakFirst && a && a->entity == 0xE8 && s.Count(What::kAway) == 1 && said && !said->weak && s.st.home[0].role == Role::kNone,
               "weak first %d; entity %X; said %d (%s); the AN-94 %s", weakFirst, a ? a->entity : 0, s.Count(What::kAway), said && said->weak ? "weak" : "in a pickup",
               RoleName(s.st.home[0].role));
    }
    {
        // An away gun coming back while another gun has its shoulder: back in its slot on no shoulder (said). A weak one's shoulder taken by a hand meanwhile
        // gives it up (the newer word wins).
        World w;
        Sim s = RockWorld(w);
        const int an = w.AddPick(0xE9, 1.0f, 0, kAn94Kind, 30, 60, false, "AN-94");
        PutOn(s, 1, kRS);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.3f);
        TakeOf(s, w, an, 1);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.3f);
        // (the AN-94 put on the left shoulder; then a second gun, from slot 1, taken and put on... the right is the M416's while it's away: refused)
        PutOn(s, 1, kLS);
        const bool anLeft = s.st.home[0].role == Role::kSecondary && AwayOf(s, kM416Kind);
        World wk;
        Sim k = SwordWorld(wk);
        PutOn(k, 1, kRS);
        k.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        k.Run(20.0f);
        Gave(k, 0, 0xA741, kAt4Kind, true, true);
        k.Switch(0);
        k.hand[1].squeeze = 1.0f;
        k.armed[1] = true;
        k.Run(0.5f);
        const bool weak = AwayOf(k, kM4Kind) && !AwayOf(k, kM4Kind)->entity;
        // the AN-94 off a pickup into slot 1? (Swordbreaker's Mk11 is there): instead the Mk11 drawn and put on the right shoulder by the hand
        LetGo(k, 1);
        k.Run(0.5f);
        SqueezeAt(k, 1, kLS);
        k.gameSlot = 1;
        k.Switch(1);
        k.armed[1] = true;
        k.Run(0.2f);
        PutOn(k, 1, kRS);
        const bool mk11Right = k.st.home[1].role == Role::kPrimary && !AwayOf(k, kM4Kind);
        Gave(k, 0, 0x4A12, kM4Kind);
        k.Switch(0);
        k.Run(0.3f);
        Report("HA17", "one gun a shoulder for guns coming back: the M416 away keeps the right shoulder (the AN-94 goes on the left); a weak record "
                       "(Swordbreaker's M4A1 with the game) gives its shoulder up to the Mk11 put there by the hand, and the M4A1 back from the kit is "
                       "on no shoulder",
               anLeft && weak && mk11Right && k.st.home[0].role == Role::kNone && k.st.home[1].role == Role::kPrimary,
               "AN-94 left %d; weak %d; Mk11 right %d; the M4A1 back: %s", anLeft, weak, mk11Right, RoleName(k.st.home[0].role));
    }
    {
        // The lines, with the names the slots have in their scenes (the review's finding 11: the fixed table's slot 1 is the Stinger, so the AN-94's
        // lines read "the Stinger" and a wrong name wasn't caught): Rock with an AN-94 picked up into slot 1.
        const char* rock[kSlots] = {"M416", "AN-94", "M320", nullptr, nullptr, "SMAW", "M67", "Knife", nullptr, nullptr};
        const auto said = [&](const Event& x) {
            char line[400];
            Say(x, rock, line, sizeof(line));
            return std::string(line);
        };
        Event e;
        e.what = What::kAway, e.slot = 0, e.place = Place::kRightShoulder;
        CopyName(e.gun, "M416");
        CopyName(e.otherGun, "AT4");
        const std::string strong = said(e);
        e.weak = true;
        CopyName(e.gun, "M4A1");
        const std::string weak = said(e);
        Event c;
        c.what = What::kComeBack, c.slot = 0, c.place = Place::kRightShoulder;
        CopyName(c.gun, "M416");
        const std::string back = said(c);
        Event l;
        l.what = What::kLost, l.slot = 0, l.why = Why::kFar, l.place = Place::kRightShoulder;
        CopyName(l.gun, "M416");
        CopyName(l.otherGun, "AT4");
        const std::string lost = said(l);
        Event d;
        d.what = What::kDiscard, d.side = 0, d.slot = 1, d.other = 0, d.place = Place::kRightShoulder;
        const std::string discard = said(d);
        Event t;
        t.what = What::kStowRefused, t.side = 1, t.slot = 1, t.why = Why::kTaken, t.at = Place::kRightShoulder, t.other = 0;
        const std::string taken = said(t);
        t.kept = true;
        const std::string kept = said(t);
        Event f;
        f.what = What::kRefused, f.side = 0, f.why = Why::kFar, f.place = Place::kRightShoulder, f.distance = 4.0f, f.giveBack = true;
        CopyName(f.gun, "M416");
        CopyName(f.otherGun, "AT4");
        const std::string distant = said(f);
        f.distance = -1.0f;
        const std::string unseenFar = said(f);
        Event a;
        a.what = What::kAsked, a.side = 0, a.place = Place::kRightShoulder, a.giveBack = true, a.distance = 1.2f;
        CopyName(a.gun, "M416");
        CopyName(a.otherGun, "AT4");
        const std::string asked = said(a);
        Event g;
        g.what = What::kLost, g.slot = 0, g.why = Why::kTaken, g.place = Place::kRightShoulder, g.other = 1;
        CopyName(g.gun, "M416");
        CopyName(g.otherGun, "AT4");
        const std::string gave = said(g);
        g.weak = true;
        CopyName(g.gun, "M4A1");
        const std::string gaveWeak = said(g);
        Event x;
        x.what = What::kLost, x.slot = 0, x.why = Why::kAll;
        CopyName(x.gun, "M416");
        CopyName(x.otherGun, "AK-74M");
        const std::string all = said(x);
        Event k;
        k.what = What::kComeBack, k.slot = 0, k.place = Place::kRightShoulder, k.weak = true, k.pickupLeft = true;
        CopyName(k.gun, "M4A1");
        CopyName(k.otherGun, "M249");
        const std::string kit = said(k);
        Event o;
        o.what = What::kCameAway, o.side = 0, o.slot = 0, o.place = Place::kRightShoulder, o.why = Why::kOpened;
        const std::string opened = said(o);
        const std::string want[] = {
            "Holster: the M416 stays at the right shoulder: the game put it into the pickup where the AT4 lay (its take, slot 0): a draw there brings it "
            "back with its rounds, within the game's reach of that pickup (2.5 m, in sight)",
            "Holster: the M4A1 stays at the right shoulder: the game took it out of its slot 0 with no pickup (a script's kit, or a take the mod didn't "
            "see): it's there again once the game gives it back",
            "Holster: the M416 back in its slot 0 (the game's take of its pickup): its holster the right shoulder again",
            "Holster: the M416 isn't at the right shoulder any more: the pickup it was in (where the AT4 lay) is gone from the pickups seen near",
            "Holster: the M416 drawn from the right shoulder by the left hand; the AN-94 you let go of went into its slot 1 (the game draws one gun at a "
            "time): on no shoulder, the D-pad or Y brings it",
            "Holster: the AN-94 not put away at the right shoulder: the M416 is there (one gun a shoulder), so it's let go of as anywhere",
            "Holster: the AN-94 not put away at the right shoulder: the M416 is there (one gun a shoulder): it stays in the hand",
            "Holster: the M416 not drawn from the right shoulder: the game keeps it in the pickup where the AT4 lay, 4.0 m away, and gives it back only "
            "within 2.5 m of it and in sight: go back there and draw again, or take it off the floor",
            "Holster: the M416 not drawn from the right shoulder: the pickup the game keeps it in (where the AT4 lay) isn't among the pickups seen near: go "
            "back there and draw again, or take it off the floor",
            "Holster: the M416 asked for from the right shoulder by the left hand: the game's own take of the pickup it's in (where the AT4 lay, 1.2 m "
            "away)",
            "Holster: the M416 is on no shoulder now: the AN-94 was put on the right shoulder (one gun a shoulder); it stays in the pickup where the AT4 "
            "lay (take it off the floor there)",
            "Holster: the M4A1 is on no shoulder now: the AN-94 was put on the right shoulder (one gun a shoulder); when the game gives it back it's on no "
            "shoulder",
            "Holster: the M416 is on no shoulder now: the pickup the game took (where the AK-74M lay) took every gun, and can't give it back alone: it "
            "lies there",
            "Holster: the M4A1 back in its slot 0 (the game gave it back): its holster the right shoulder again; the M4A1 left in the pickup where the "
            "M249 lay stays there, on no shoulder",
            "Holster: the M416 came after the left hand opened: it is at its place at the right shoulder"};
        const std::string got[] = {strong, weak, back, lost, discard, taken, kept, distant, unseenFar, asked, gave, gaveWeak, all, kit, opened};
        int wrong = 0;
        std::string first;
        for (size_t i = 0; i < std::size(want); ++i)
            if (got[i] != want[i]) {
                if (!wrong)
                    first = "\"" + got[i] + "\" for \"" + want[i] + "\"";
                ++wrong;
            }
        Report("HA18", "the lines of a holstered gun the game takes out of its slot, by the scene's names (an AN-94 in slot 1)", wrong == 0,
               "%d wrong%s%s", wrong, wrong ? ": " : "", first.c_str());
    }
}

// ---- HS: the review of 907b2b6 settled (2026-11-18): a shoulder never blocked for good, kept in the hand, the memo by level, the level's own pickups
// never refused, a draw whose hand opened, the AT4 branch's merge ----

// Rock at the defaults with the M416 on the right shoulder, an AN-94 taken into the right hand (slot 1), armed and squeezed.
Sim An94Taken(World& w) {
    Sim s = RockWorld(w);
    PutOn(s, 1, kRS);
    s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
    s.Run(0.5f);
    Took(s, 1, 0x9401, kAn94Kind);
    CopyName(s.slots[1].name, "AN-94");
    s.Switch(1, true);
    s.Step();
    s.hand[1].squeeze = 1.0f;
    s.armed[1] = true;
    s.Run(0.3f);
    return s;
}

void SettleRows() {
    {
        // Kept in the hand at a refusing shoulder, its edges: let go 6 cm out of its put-away egg a moment after it was in it (the put-away's grace): kept
        // too; let go there at 2.5 m/s: kept as well (never a throw at a place, 2026-11-19; it fell until then), but 6 cm out going away at 2.5 m/s: a
        // throw, it falls; kept, then carried to the front with the grip open: still in the hand (disarmed, as at a spawn); squeezed and let go there:
        // it falls as anywhere.
        World w;
        const Sim base = An94Taken(w);
        const Egg e = Settings{}.shoulder;
        const float out = e.mid[0] + e.half[0] + Settings{}.stowMore + 0.06f;
        Sim g = base;
        g.Move(1, kRS);
        g.Run(0.2f);
        g.MoveTo(1, out, e.mid[1], e.mid[2]);
        g.Run(0.2f);
        LetGo(g, 1);
        g.Run(0.3f);
        const bool grace = g.holder == Holder::kHeld && g.falls == 0 && g.Count(What::kStowRefused, 1) == 1;
        Sim t = base;
        t.Move(1, kRS);
        t.Run(0.2f);
        t.MoveTo(1, kRS[0], kRS[1], kRS[2], 2.5f, 0.0f, 0.0f);
        t.Run(0.05f);
        LetGo(t, 1);
        Sim to = base;
        to.Move(1, kRS);
        to.Run(0.2f);
        to.MoveTo(1, out, e.mid[1], e.mid[2], 2.5f, 0.0f, 0.0f);
        to.Run(0.05f);
        LetGo(to, 1);
        const bool thrown = t.falls == 0 && t.holder == Holder::kHeld && t.Count(What::kStowRefused, 1) == 1 && to.falls == 1 &&
                            to.holder == Holder::kLoose;
        Sim c = base;
        c.Move(1, kRS);
        c.Run(0.2f);
        LetGo(c, 1);
        c.Run(0.2f);
        c.Move(1, kFront);
        c.Run(0.5f);
        const bool carried = c.holder == Holder::kHeld && c.trigger == 1 && c.falls == 0;
        c.hand[1].squeeze = 1.0f;
        c.Run(0.2f);
        LetGo(c, 1);
        const bool dropped = c.holder == Holder::kLoose && c.falls == 1;
        Report("HS1", "kept in the hand at a refusing shoulder: let go just out of it a moment after (the grace): kept; let go there at 2.5 m/s: kept, "
                      "thrown just out of it: falls; kept, carried away with the grip open: still in the hand; squeezed and let go in front: falls",
               grace && thrown && carried && dropped, "grace %d (%d lines), thrown %d, carried %d, dropped %d", grace, g.Count(What::kStowRefused, 1), thrown,
               carried, dropped);
        // The other hand on its handguard there: the let-go is that hold's, as ever (held by its front, nothing kept, nothing said).
        Sim o = base;
        o.hand[0].onGun = true;
        o.Move(1, kRS);
        o.Run(0.2f);
        LetGo(o, 1);
        o.Run(0.2f);
        // A hand not seen opening at a shoulder an away gun has: put away there too (the put-away wins), the away gun on no shoulder.
        World wu;
        Sim u = RockWorld(wu);
        u.set.unseenShoulders = true;
        const int an = wu.AddPick(0xED, 1.0f, 0, kAn94Kind, 30, 90, false, "AN-94");
        PutOn(u, 1, kRS);
        u.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        u.Run(0.3f);
        TakeOf(u, wu, an, 1);
        u.hand[1].squeeze = 1.0f;
        u.Run(0.3f);
        u.Move(1, kRS);
        u.Run(0.2f);
        u.hand[1].seen = Seen::kNone;
        u.Run(0.1f);
        u.hand[1].squeeze = 0.0f;
        u.Run(0.3f);
        Report("HS1b", "the other hand on its handguard at a refusing shoulder: held by its front (nothing kept, nothing said); a hand not seen opening at a "
                       "shoulder an away gun has: put away there, the away gun on no shoulder",
               o.holder == Holder::kBarrel && o.barrel == 0 && o.falls == 0 && o.Count(What::kStowRefused) == 0 && u.holder == Holder::kStowed &&
                   u.st.home[0].role == Role::kPrimary && !AwayOf(u, kM416Kind) && u.Count(What::kLost) == 1,
               "front: holder %d by %d, refusals %d; not seen: holder %d, %s, away %d", static_cast<int>(o.holder), o.barrel, o.Count(What::kStowRefused),
               static_cast<int>(u.holder), RoleName(u.st.home[0].role), AwayCount(u));
    }
    {
        // ComeBack ends a record in a pickup (review finding 1): Swordbreaker, the M4A1 holstered, away in the M249's pickup at the bridge 200 m back; the
        // level's kit gives a fresh M4A1 into slot 0 (no take): its holster the right shoulder again, nothing away, at its place; said once the take's
        // window is over (the one in the pickup stays there: no take of it came, the second review of ef2cbb6's #1); the M249 on no shoulder.
        World w;
        Sim s = SwordWorld(w);
        const int m249 = w.AddPick(0xEB, 1.0f, 0, 0x249, 100, 200, false, "M249");
        PutOn(s, 1, kRS);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(20.0f);
        TakeOf(s, w, m249, 1);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.5f);
        w.pick[m249].distance = 200.0f;
        s.Run(1.0f);
        const bool away = AwayOf(s, kM4Kind) && AwayOf(s, kM4Kind)->entity == 0xEB;
        LetGo(s, 1);
        s.Run(0.5f);
        Gave(s, 0, 0x4A16, kM4Kind);
        CopyName(s.slots[0].name, "M4A1");
        const ArriveOut o = s.Switch(0);
        s.Run(0.3f);
        const bool notYet = s.Find(What::kComeBack) == nullptr && s.st.home[0].role == Role::kPrimary && AwayCount(s) == 0;
        s.Run(0.9f);
        const Event* back = Kept(s.Find(What::kComeBack), 0);
        Report("HS2", "the level's kit gives a fresh M4A1 back while the old one is away in a pickup 200 m behind: its holster again, at its place, nothing "
                      "away; said once the take's window is over (the one in the pickup stays there)",
               away && o.arrival == Arrival::kAtPlace && s.holder == Holder::kStowed && s.st.home[0].role == Role::kPrimary && AwayCount(s) == 0 && back &&
                   back->pickupLeft && back->place == Place::kRightShoulder && notYet && s.Count(What::kComeBack) == 1,
               "away %d; arrival %d, %s; away now %d; not said at 0.3 s %d; \"%s\"", away, static_cast<int>(o.arrival), RoleName(s.st.home[0].role),
               AwayCount(s), notYet, LineIn(s, back).c_str());
    }
    {
        // The memo by level (review finding 3). (a) Rock, the M416 put on the left shoulder (its memo the secondary); a death: the kit again, every gun of
        // it with a holster by its slot carried before: the memo kept (the M416 the secondary). (b) The same with a life between that carried nothing
        // (the soldier gone a moment, two new lives): kept. (c) A kit gun of a kind never carried, noted a frame after the others: another level's, the
        // memo gone, the M416 noted first by its slot again (the primary). (d) A take in the kit's seconds isn't the kit's: the memo kept.
        World w;
        Sim a = RockWorld(w);
        PutOn(a, 1, kLS);
        const bool left = a.st.home[0].role == Role::kSecondary;
        Sim b = a;
        const auto again = [](Sim& s, uint32_t add) {
            NewLife(s.st);
            for (int k = 0; k < kSlots; ++k)
                if (s.slots[k].carried)
                    s.slots[k].id += add;
            s.holder = Holder::kHeld, s.trigger = 1, s.gameSlot = 0;
            s.Run(0.5f);
        };
        again(a, 0x100000);
        const Role death = a.st.home[0].role;
        NewLife(b.st);
        SlotIn kept[kSlots];
        for (int k = 0; k < kSlots; ++k)
            kept[k] = b.slots[k], b.slots[k] = SlotIn{};
        b.slotRead = false;
        b.Run(0.5f);
        for (int k = 0; k < kSlots; ++k)
            b.slots[k] = kept[k];
        b.slotRead = true;
        again(b, 0x200000);
        const Role gap = b.st.home[0].role;
        // (c): the M416 alone the first frame, an AK-74M in slot 1 the next.
        Sim c = RockWorld(w);
        PutOn(c, 1, kLS);
        NewLife(c.st);
        SlotIn ak = SlotIn{};
        ak.carried = true, ak.known = true;
        Named(ak, 0xA471, kAkKind, "AK-74M");
        for (int k = 0; k < kSlots; ++k)
            if (c.slots[k].carried)
                c.slots[k].id += 0x100000;
        c.holder = Holder::kHeld, c.trigger = 1, c.gameSlot = 0;
        c.Step();
        const Role first = c.st.home[0].role;
        c.slots[1] = ak;
        c.Run(0.3f);
        // (d): a death's kit, then an AN-94 never carried taken into slot 1 within the kit's seconds.
        Sim d = RockWorld(w);
        PutOn(d, 1, kLS);
        again(d, 0x300000);
        Took(d, 1, 0x9411, kAn94Kind);
        CopyName(d.slots[1].name, "AN-94");
        d.Run(0.3f);
        // (e) hands.cpp's order at a new life: the new level's first gun arrives (OnGunChanged, Arrive with newLife) before its NewLife: not noted in the
        // life before, so the level is told all the same (X1b's: an M4A1 put on the left shoulder in Rock, then Swordbreaker's kit).
        World we;
        Sim e = RockWorld(we);
        PutOn(e, 1, kRS);
        e.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        e.Run(0.3f);
        const int m4 = we.AddPick(0xEE, 1.0f, 1, kM4Kind, 30, 90, false, "M4A1");
        TakeOf(e, we, m4, 1);
        e.hand[1].squeeze = 1.0f;
        e.Run(0.5f);
        PutOn(e, 1, kLS);
        Swordbreaker(e);
        Named(e.slots[0], 0x4A31, kM4Kind, "M4A1");
        e.slots[1].known = true;
        Named(e.slots[1], 0x1131, kMk11Kind, "Mk11");
        Named(e.slots[6], 0x6791, 0x67, "M67");
        e.Switch(0, false, true);  // (before NewLife: hands.cpp's OnGunChanged at the new life's first frame)
        NewLife(e.st);
        e.holder = Holder::kHeld, e.trigger = 1;
        e.Run(0.5f);
        const bool order = e.st.memoForgot && e.st.home[0].role == Role::kPrimary && e.st.home[1].role == Role::kSecondary;
        Report("HS3", "the memo by level: a death's kit (all carried before) keeps it; a life between with nothing carried keeps the one before; a kit gun "
                      "never carried, a frame late: another level's, the memo gone, each kit gun by its slot; a take in the kit's seconds isn't the kit's; "
                      "the new level's gun arriving before its new life is begun: another level's all the same",
               order &&
               left && death == Role::kSecondary && gap == Role::kSecondary && first == Role::kSecondary && c.st.memoForgot &&
                   c.st.home[0].role == Role::kPrimary && c.st.home[1].role == Role::kSecondary && !d.st.memoForgot && d.st.home[0].role == Role::kSecondary &&
                   d.st.home[1].role == Role::kNone,
               "left %d; a death %s; a gap %s; a late kit gun: first %s, then %s and %s (forgot %d); a take in the kit's seconds: %s, %s (forgot %d); "
               "arriving before its new life: %s, %s (forgot %d)",
               left, RoleName(death), RoleName(gap), RoleName(first), RoleName(c.st.home[0].role), RoleName(c.st.home[1].role), c.st.memoForgot,
               RoleName(d.st.home[0].role), RoleName(d.st.home[1].role), d.st.memoForgot, RoleName(e.st.home[0].role), RoleName(e.st.home[1].role),
               e.st.memoForgot);
    }
    {
        // A draw from a shoulder whose hand opens while its gun comes (review finding 9): the game's take of the M416's pickup lands 10 frames after the
        // ask, the left grip open by then: the M416 at its place, put away (said), its holster back, never in that open hand (it would fall three frames
        // on); with the grip held: in the hand.
        bool atPlace = false, inHand = false;
        std::string line;
        for (int k = 0; k < 2; ++k) {
            World w;
            Sim s = RockWorld(w);
            const int at4 = w.AddPick(0xEC, 1.2f, 0, kAt4Kind, 1, 0, true, "AT4");
            PutOn(s, 1, kRS);
            s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
            s.Run(0.3f);
            TakeOf(s, w, at4, 1);
            s.hand[1].squeeze = 1.0f;
            s.Run(0.3f);
            LetGo(s, 1);
            s.Run(0.5f);
            w.late = 10;
            s.hand[0].squeeze = 0.0f;
            s.Move(0, kRS);
            s.Run(0.15f);
            s.hand[0].squeeze = 1.0f;
            s.Run(2 * kDt);
            if (k == 0)
                s.hand[0].squeeze = 0.0f;  // (open before its gun comes)
            s.Run(0.4f);
            const Event* came = s.Find(What::kCameAway, 0);
            if (k == 0) {
                atPlace = w.asked == 1 && w.served == 2 && s.slots[0].kind == kM416Kind && s.holder == Holder::kStowed && s.st.home[0].role == Role::kPrimary &&
                          s.falls <= 1 && came && came->why == Why::kOpened && AwayCount(s) == 0;
                line = LineIn(s, came);
            } else {
                inHand = w.asked == 1 && s.slots[0].kind == kM416Kind && s.holder == Holder::kHeld && s.trigger == 0 && !came;
            }
        }
        Report("HS4", "a draw from a shoulder, the hand open when the game's take lands: at its place, put away (said), its holster back; the grip held: "
                      "in the hand", atPlace && inHand, "at its place %d (\"%s\"), in the hand %d", atPlace, line.c_str(), inHand);
    }
    {
        // The AT4 branch's merge: its use of the roof's prompt marks the AT4's slot as a take's. (a) The M4A1 holstered: the AT4 on no shoulder, in the
        // hand that used it; the M4A1 with the game at the right shoulder (a weak record: per gun, not per slot); the level's kit gives it back: at its
        // place. (b) The owner's flow (b) there: the M4A1 dropped first: nothing away, the kit's M4A1 back on no shoulder, into the right hand (today's:
        // its copy lying where it was dropped is the hosting's to keep).
        World wa;
        Sim a = SwordWorld(wa);
        PutOn(a, 1, kRS);
        a.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        a.Run(20.0f);
        Took(a, 0, 0xA780, kAt4Kind, true, true);
        CopyName(a.slots[0].name, "AT4");
        a.Switch(0, true);
        a.hand[1].squeeze = 1.0f;
        a.armed[1] = true;
        a.Run(1.5f);
        const Away* weak = AwayOf(a, kM4Kind);
        const bool roof = a.st.home[0].role == Role::kNone && weak && !weak->entity && weak->role == Role::kPrimary;
        LetGo(a, 1);
        a.Run(0.5f);
        Gave(a, 0, 0x4A17, kM4Kind);
        CopyName(a.slots[0].name, "M4A1");
        const ArriveOut ka = a.Switch(0);
        a.Run(0.3f);
        const bool backA = ka.arrival == Arrival::kAtPlace && a.holder == Holder::kStowed && a.st.home[0].role == Role::kPrimary && AwayCount(a) == 0;
        World wb;
        Sim b = SwordWorld(wb);
        b.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        b.Run(20.0f);
        LetGo(b, 1);
        b.Run(0.5f);
        const bool dropped = b.holder == Holder::kLoose && b.st.home[0].role == Role::kNone;
        Took(b, 0, 0xA781, kAt4Kind, true, true);
        CopyName(b.slots[0].name, "AT4");
        b.Switch(0, true);
        b.hand[0].squeeze = 1.0f;
        b.armed[0] = true;
        b.holder = Holder::kHeld, b.trigger = 0;
        b.Run(1.5f);
        const bool none = AwayCount(b) == 0 && b.st.home[0].role == Role::kNone;
        LetGo(b, 0);
        b.Run(0.5f);
        Gave(b, 0, 0x4A18, kM4Kind);
        CopyName(b.slots[0].name, "M4A1");
        const ArriveOut kb = b.Switch(0);
        b.Run(0.3f);
        const bool backB = kb.arrival == Arrival::kToday && b.holder == Holder::kHeld && b.trigger == 1 && b.st.home[0].role == Role::kNone;
        Report("HS5", "the AT4 branch's merge (the roof's AT4 by a use, marked a take's): holstered M4A1: with the game at its shoulder, back at its place "
                      "with the kit; dropped first (flow b): nothing away, the kit's M4A1 on no shoulder, into the right hand",
               roof && backA && dropped && none && backB, "holstered: roof %d, back %d (arrival %d); dropped %d, none %d, back %d (arrival %d)", roof, backA,
               static_cast<int>(ka.arrival), dropped, none, backB, static_cast<int>(kb.arrival));
        // (c) The prompt used by the left hand with the M4A1 in the right (the reach and grip of the AT4 branch: the scene's kit replaces it in slot 0 with
        // no pickup, its use marked a take's and no swap told): never dropped, so with the game at its shoulder as the gift's (X4); the kit's M4A1 back at
        // its place once the AT4 was thrown.
        World wc;
        Sim c = SwordWorld(wc);
        c.Run(20.0f);
        const bool heldFirst = c.holder == Holder::kHeld && c.trigger == 1 && c.st.home[0].role == Role::kPrimary;
        Took(c, 0, 0xA782, kAt4Kind, true, true);
        CopyName(c.slots[0].name, "AT4");
        c.Switch(0, true);
        c.holder = Holder::kHeld, c.trigger = 0, c.barrel = -1;
        c.hand[0].squeeze = 1.0f, c.armed[0] = true;
        c.hand[1].squeeze = 0.0f, c.armed[1] = false;
        c.Run(1.5f);
        const Away* wk = AwayOf(c, kM4Kind);
        const bool withGame = wk && !wk->entity && wk->role == Role::kPrimary && wk->held && c.st.home[0].role == Role::kNone;
        LetGo(c, 0);
        c.Run(0.5f);
        Gave(c, 0, 0x4A19, kM4Kind);
        CopyName(c.slots[0].name, "M4A1");
        const ArriveOut kc = c.Switch(0);
        c.Run(0.3f);
        const bool backC = kc.arrival == Arrival::kAtPlace && c.holder == Holder::kStowed && c.st.home[0].role == Role::kPrimary && AwayCount(c) == 0;
        Report("HS5c", "the AT4 branch's merge: its prompt used by the left hand with the M4A1 in the right (a take's mark, no swap: no pickup): the M4A1 "
                       "with the game at its shoulder; the kit's M4A1 back at its place",
               heldFirst && withGame && backC, "held first %d; with the game %d; back %d (arrival %d, %s)", heldFirst, withGame, backC,
               static_cast<int>(kc.arrival), RoleName(c.st.home[0].role));
    }
}

// ---- X: the review's own scenarios of 907b2b6 (2026-11-18, its revtest.cpp), as it wrote them ----

void ReviewRows() {
    const auto R = [](Role r) { return RoleName(r); };
    {
        // X1: a gun picked up in one level (never put on a shoulder) is the next level's kit gun: the memo survived, because every kit shares the M67 (and
        // the knife) with the memo, so "another level" was never seen.
        World w;
        Sim s = RockWorld(w);
        PutOn(s, 1, kRS);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.3f);
        const int m4 = w.AddPick(0xF1, 1.0f, 1, kM4Kind, 30, 90, false, "M4A1");
        TakeOf(s, w, m4, 1);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.5f);
        const Role inRock = s.st.home[1].role;
        // Another level (Operation Swordbreaker from Replay Missions): a new life, its kit.
        NewLife(s.st);
        Swordbreaker(s);
        Named(s.slots[0], 0x4A30, kM4Kind, "M4A1");
        s.slots[1].known = true;
        Named(s.slots[1], 0x1130, kMk11Kind, "Mk11");
        Named(s.slots[6], 0x6790, 0x67, "M67");
        s.holder = Holder::kHeld, s.trigger = 1, s.gameSlot = 0;
        s.Run(0.5f);
        Report("X1", "a gun picked up in Rock (M4A1, never placed) is Swordbreaker's kit gun: the kit's M4A1 should be the primary, the Mk11 the secondary",
               s.st.home[0].role == Role::kPrimary && s.st.home[1].role == Role::kSecondary,
               "in Rock the picked-up M4A1 %s; Swordbreaker's kit: the M4A1 %s, the Mk11 %s (memo checked %d)", R(inRock), R(s.st.home[0].role),
               R(s.st.home[1].role), s.st.memoChecked);
    }
    {
        // X1b: ... and one put on the left shoulder by hand in Rock: Swordbreaker's M4A1 on the left, its Mk11 on none.
        World w;
        Sim s = RockWorld(w);
        PutOn(s, 1, kRS);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.3f);
        const int m4 = w.AddPick(0xF1, 1.0f, 1, kM4Kind, 30, 90, false, "M4A1");
        TakeOf(s, w, m4, 1);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.5f);
        PutOn(s, 1, kLS);
        const Role inRock = s.st.home[1].role;
        NewLife(s.st);
        Swordbreaker(s);
        Named(s.slots[0], 0x4A30, kM4Kind, "M4A1");
        s.slots[1].known = true;
        Named(s.slots[1], 0x1130, kMk11Kind, "Mk11");
        Named(s.slots[6], 0x6790, 0x67, "M67");
        s.holder = Holder::kHeld, s.trigger = 1, s.gameSlot = 0;
        s.Run(0.5f);
        const Role m4Role = s.st.home[0].role, mk11Role = s.st.home[1].role;
        // A third life on Swordbreaker: the Mk11's "none" was then in the memo for good.
        NewLife(s.st);
        for (int k = 0; k < kSlots; ++k)
            if (s.slots[k].carried)
                s.slots[k].id += 0x100000;
        s.Run(0.5f);
        Report("X1b", "a picked-up M4A1 put on the left shoulder in Rock: Swordbreaker's kit should still be the M4A1 primary, the Mk11 secondary",
               m4Role == Role::kPrimary && mk11Role == Role::kSecondary && s.st.home[0].role == Role::kPrimary && s.st.home[1].role == Role::kSecondary,
               "in Rock the M4A1 %s; Swordbreaker: the M4A1 %s, the Mk11 %s; after a death there: the M4A1 %s, the Mk11 %s", R(inRock), R(m4Role),
               R(mk11Role), R(s.st.home[0].role), R(s.st.home[1].role));
    }
    {
        // X2: flow (a) with the AT4 put on the launcher's place once before it's fired: thrown away, the rifle's draw was refused ("take it first").
        World w;
        Sim s = RockWorld(w);
        w.loaded[0] = 27, w.spare[0] = 120;
        const int at4 = w.AddPick(0xF2, 1.2f, 0, kAt4Kind, 1, 0, true, "AT4");
        PutOn(s, 1, kRS);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.3f);
        TakeOf(s, w, at4, 1);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.3f);
        PutOn(s, 1, kLS);  // (the AT4 on the launcher's place a moment, to climb, to reload the other gun...)
        const bool stowed = s.holder == Holder::kStowed && s.st.home[0].role == Role::kLauncher;
        SqueezeAt(s, 1, kLS);  // (drawn again, case A)
        const bool drawn = s.holder == Holder::kHeld && s.trigger == 1;
        s.Move(1, kFront);
        s.Run(0.2f);
        LetGo(s, 1);  // (fired, thrown away)
        s.Run(0.5f);
        const bool lies = s.holder == Holder::kLoose;
        const int askedBefore = w.asked;
        SqueezeAt(s, 0, kRS, 0.8f);
        const Event* refused = s.Find(What::kRefused, 0);
        Report("X2", "the user's AT4, flow (a), the AT4 put on the launcher's place once before it's fired and thrown: the rifle should still be drawn back "
                     "from the right shoulder",
               stowed && drawn && lies && w.asked == askedBefore + 1 && s.slots[0].kind == kM416Kind,
               "stowed %d drawn %d lies %d; asked %d; slot 0 %X; discardable %d; \"%s\"", stowed, drawn, lies, w.asked - askedBefore, s.slots[0].kind,
               Discardable(s.st, kAt4Kind), Line(refused).c_str());
    }
    {
        // X3: the holstered M416 swapped for another M416 off the ground (the same kind came and went): no record in that pickup, a weak one "with no
        // pickup" instead, which a draw could never bring back.
        World w;
        Sim s = RockWorld(w);
        w.loaded[0] = 27, w.spare[0] = 120;
        const int m416 = w.AddPick(0xF3, 1.0f, 0, kM416Kind, 30, 60, false, "M416");
        PutOn(s, 1, kRS);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.3f);
        TakeOf(s, w, m416, 1);
        s.hand[1].squeeze = 1.0f;
        s.Run(1.5f);
        const Away* a = AwayOf(s, kM416Kind);
        const bool strong = a && a->entity == 0xF3;
        const Event* said = Kept(s.Find(What::kAway), 0);
        // The new M416 thrown away, a draw at the right shoulder.
        LetGo(s, 1);
        s.Run(0.5f);
        SqueezeAt(s, 0, kRS, 0.8f);
        const Event* refused = Kept(s.Find(What::kRefused, 0), 1);
        Report("X3", "the holstered M416 swapped for another M416 off the ground: it should be in that pickup (drawn back, or on no shoulder), never a "
                     "record 'with no pickup' that refuses every draw",
               strong && !(refused && refused->why == Why::kWithGame) && w.asked == 1 && s.st.home[0].role == Role::kPrimary && w.loaded[0] == 27,
               "away %s; \"%s\"; \"%s\"; asked %d, the M416 %s with %d", strong ? "in the pickup" : a ? "weak" : "none", Line(said).c_str(),
               Line(refused).c_str(), w.asked, R(s.st.home[0].role), w.loaded[0]);
    }
    {
        // X4: Swordbreaker's roof AT4 by its kit with the M4A1 in the hand (the prompt pressed with the rifle held, the vanilla way): the kit's M4A1 came
        // back on no shoulder though it was never dropped.
        World w;
        Sim s = SwordWorld(w);
        PutOn(s, 1, kRS);
        SqueezeAt(s, 1, kRS);  // (drawn again: in the hand)
        s.Run(20.0f);
        const bool inHand = s.holder == Holder::kHeld && s.st.home[0].role == Role::kPrimary;
        Gave(s, 0, 0xA750, kAt4Kind, true, true);
        CopyName(s.slots[0].name, "AT4");
        s.Switch(0);
        s.hand[1].squeeze = 1.0f;
        s.armed[1] = true;
        s.Run(1.5f);
        const Away* away = AwayOf(s, kM4Kind);
        const bool weak = away && !away->entity && away->role == Role::kPrimary;
        LetGo(s, 1);
        s.Run(0.5f);
        Gave(s, 0, 0x4A13, kM4Kind);
        CopyName(s.slots[0].name, "M4A1");
        const ArriveOut o = s.Switch(0);
        s.Run(0.3f);
        Report("X4", "Swordbreaker's roof AT4 by its kit with the M4A1 in the hand: the kit's M4A1 given back should keep its right shoulder (never dropped)",
               inHand && weak && s.st.home[0].role == Role::kPrimary && o.arrival == Arrival::kAtPlace,
               "in the hand first %d; away %s; the M4A1 back: %s (arrival %d)", inHand, weak ? "with the game" : "none", R(s.st.home[0].role),
               static_cast<int>(o.arrival));
    }
    {
        // X5: a take within a life's first 10 s whose swap is told a frame late: the AT4 was the kit's launcher in the memo, so thrown away it refused
        // the rifle's draw.
        World w;
        Sim s = RockWorld(w);
        PutOn(s, 1, kRS);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(1.0f);
        Gave(s, 0, 0xA760, kAt4Kind, true, true);
        CopyName(s.slots[0].name, "AT4");
        s.Step();
        SwapIn in;
        in.changed = 1, in.slot[0] = 0, in.came[0] = kAt4Kind, in.went[0] = kM416Kind, in.wentBlueprint[0] = kM416Kind + kBlueprintOf, in.entity = 0xF5;
        CopyName(in.taken, "AT4");
        NoteSwap(s.set, s.slots, in, s.st);
        s.taken = 1u << 0;
        s.Step();
        w.AddPick(0xF5, 1.0f, 0, kM416Kind, 27, 120, false, "M416");
        s.Switch(0, true);
        s.holder = Holder::kHeld, s.trigger = 1, s.armed[1] = true;
        s.hand[1].squeeze = 1.0f;
        s.Run(0.3f);
        const Role role = s.st.home[0].role;
        const Memo* m = MemoOf(s.st, kAt4Kind);
        LetGo(s, 1);
        s.Run(0.5f);
        SqueezeAt(s, 0, kRS, 0.8f);
        const Event* refused = s.Find(What::kRefused, 0);
        Report("X5", "a take in a life's first 10 s told a frame late: the AT4 none in the memo too (thrown away, the rifle drawn back)",
               role == Role::kNone && m && m->role == Role::kNone && w.asked == 1, "the AT4's home %s, memo %s; asked %d; \"%s\"", R(role),
               m ? R(m->role) : "-", w.asked, Line(refused).c_str());
    }
    {
        // X6: the away gun and the D-pad: the M416 away, the AT4 put away; D-pad down (a pick of slot 0) gives the AT4, not the rifle (the weapon
        // buttons can't reach an away gun: the README says so now). Info.
        World w;
        Sim s = RockWorld(w);
        const int at4 = w.AddPick(0xF6, 1.2f, 0, kAt4Kind, 1, 0, true, "AT4");
        PutOn(s, 1, kRS);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.3f);
        TakeOf(s, w, at4, 1);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.3f);
        PutOn(s, 1, kLS);  // the AT4 put away (launcher's place)
        const size_t before = s.events.size();
        s.picked = true, s.pickSlot = 0;  // D-pad down: slot 0
        s.Step();
        s.Run(0.3f);
        int said = 0;
        for (size_t i = before; i < s.events.size(); ++i)
            said += s.events[i].what != What::kTick;
        Report("X6", "info: D-pad down with the M416 away and the AT4 put away in slot 0: what comes", true, "holder %d trigger %d slot 0 %X; lines %d",
               static_cast<int>(s.holder), s.trigger, s.slots[0].kind, said);
    }
    {
        // X7: a new life the mod sees while the world stays (the soldier unread over 1 s: a ride, a cutscene): every away record goes; the M416 still lies
        // in its pickup; taken back off the floor it's on no shoulder. Info.
        World w;
        Sim s = RockWorld(w);
        const int at4 = w.AddPick(0xF7, 1.2f, 0, kAt4Kind, 1, 0, true, "AT4");
        PutOn(s, 1, kRS);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.3f);
        TakeOf(s, w, at4, 1);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.3f);
        NewLife(s.st);  // (same world, same guns, new weapon objects not even made)
        s.Run(0.5f);
        LetGo(s, 1);
        s.Run(0.5f);
        TakeOf(s, w, at4, 1);  // the M416 off the floor
        s.hand[1].squeeze = 1.0f;
        s.Run(0.3f);
        Report("X7", "info: a new life seen with the world kept (soldier unread over 1 s): the away M416 taken back off the floor", true,
               "away records %d; the M416 %s (memo %s)", AwayCount(s), R(s.st.home[0].role),
               MemoOf(s.st, kM416Kind) ? R(MemoOf(s.st, kM416Kind)->role) : "-");
    }
    {
        // X8: Swordbreaker's bridge: the M4A1 holstered, the M249 taken (it names slot 0): the M4A1 away in the M249's pickup, left behind as the player
        // walks on (kept for good: the player's own pickup); the M249 could then go on no shoulder at all, and every try dropped it.
        World w;
        Sim s = SwordWorld(w);
        w.loaded[0] = 30, w.spare[0] = 90;
        constexpr uint32_t kM249Kind = 0x249;
        const int m249 = w.AddPick(0xF8, 1.0f, 0, kM249Kind, 100, 200, false, "M249");
        PutOn(s, 1, kRS);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(20.0f);
        TakeOf(s, w, m249, 1);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.5f);
        const Away* a = AwayOf(s, kM4Kind);
        const bool strong = a && a->entity;
        w.pick[m249].distance = 60.0f;  // (the bridge fought over, the player 60 m on)
        s.Run(5.0f);
        const bool stillAway = AwayOf(s, kM4Kind) != nullptr;
        const int fallsBefore = s.falls;
        PutOn(s, 1, kRS);
        const bool fellRight = s.holder == Holder::kLoose;
        const bool rightIsIts = s.holder == Holder::kStowed && s.st.home[0].role == Role::kPrimary && AwayOf(s, kM4Kind) == nullptr;
        const Event* right = Kept(s.Find(What::kStowRefused, 1), 0);
        s.hand[1].squeeze = 1.0f;
        s.takeBy = 1;
        s.Run(0.2f);
        PutOn(s, 1, kLS);
        const bool fellLeft = s.holder == Holder::kLoose;
        s.hand[1].squeeze = 1.0f;
        s.takeBy = 1;
        s.Run(0.2f);
        PutOn(s, 1, kNeckAt);
        const bool fellNeck = s.holder == Holder::kLoose;
        Report("X8", "the M4A1 away in the M249's pickup, left 60 m behind: the M249 should be holstered somewhere (or kept in the hand), not dropped at every "
                     "shoulder",
               !(fellRight && fellLeft && fellNeck) && rightIsIts && s.falls == fallsBefore,
               "away %s (kept %d after 5 s at 60 m); right shoulder: fell %d, its own now %d (\"%s\"); left: fell %d; neck: fell %d; falls %d",
               strong ? "strong" : "none", stillAway, fellRight, rightIsIts, Line(right).c_str(), fellLeft, fellNeck, s.falls - fallsBefore);
    }
    {
        // X8b: ... the walk on to the roof: the kit's AT4 into slot 0 (the M249 there), then the level's kit gives a fresh M4A1 back: the old M4A1's
        // record at the bridge kept the right shoulder: the kit's M4A1 on no shoulder, couldn't be put there, and a draw there said "far".
        World w;
        Sim s = SwordWorld(w);
        constexpr uint32_t kM249Kind = 0x249;
        const int m249 = w.AddPick(0xF9, 1.0f, 0, kM249Kind, 100, 200, false, "M249");
        PutOn(s, 1, kRS);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(20.0f);
        TakeOf(s, w, m249, 1);
        s.hand[1].squeeze = 1.0f;
        s.Run(0.5f);
        w.pick[m249].distance = 200.0f;
        s.Run(2.0f);
        // The roof: the M249 in the right hand, the kit's AT4 into slot 0.
        Gave(s, 0, 0xA770, kAt4Kind, true, true);
        CopyName(s.slots[0].name, "AT4");
        s.Switch(0);
        s.hand[1].squeeze = 1.0f;
        s.armed[1] = true;
        s.Run(1.5f);
        LetGo(s, 1);
        s.Run(0.5f);
        // After the scene: the level's kit gives the M4A1 back.
        Gave(s, 0, 0x4A14, kM4Kind);
        CopyName(s.slots[0].name, "M4A1");
        s.Switch(0);
        s.Run(0.5f);
        const Role back = s.st.home[0].role;
        const Away* old = AwayOf(s, kM4Kind);
        // The kit's M4A1 in the right hand, put on the right shoulder.
        s.holder = Holder::kHeld, s.trigger = 1, s.armed[1] = true;
        s.hand[1].squeeze = 1.0f;
        s.Run(0.2f);
        PutOn(s, 1, kRS);
        const bool fell = s.holder == Holder::kLoose;
        Report("X8b", "the M249 at the bridge with the M4A1 holstered, then the roof's AT4 and the level's kit's M4A1: the kit's M4A1 should have (or be "
                      "allowed) the right shoulder",
               (back == Role::kPrimary || !fell) && back == Role::kPrimary && !old && !fell,
               "the kit's M4A1 %s; the old one's record %s (entity %X); put on the right shoulder: fell %d", R(back),
               old ? (old->entity ? "strong" : "weak") : "none", old ? old->entity : 0, fell);
    }
}

// ---- V, Y: the two reviews of ef2cbb6 (2026-11-18): the first's scenarios (its rvtest.cpp, V1-V15b) and the second's (its myrev.cpp, R7-R9 here Y7-Y9),
// as they wrote them, the rows they found failing checked against the settled rules ----

namespace second {

const char* R(Role r) { return RoleName(r); }
constexpr uint32_t kStingerKind = 0xF192, kM249Kind = 0x249, kM16Kind = 0x16A4;

// Every place a gun of that kind is: the soldier's slots and the pickups on the ground (a lying gun is still in its slot, the game's way).
int Copies(const Sim& s, const World& w, uint32_t kind) {
    int n = 0;
    for (int k = 0; k < kSlots; ++k)
        n += s.slots[k].carried && s.slots[k].kind == kind;
    for (int i = 0; i < w.picks; ++i)
        n += !w.pick[i].gone && w.pick[i].gun.kind == kind;
    return n;
}
// The last event of that kind (and side) from index from on; how many.
const Event* Last(const Sim& s, What what, int side = -2, size_t from = 0) {
    const Event* e = nullptr;
    for (size_t i = from; i < s.events.size(); ++i)
        if (s.events[i].what == what && (side == -2 || s.events[i].side == side))
            e = &s.events[i];
    return e;
}
int CountFrom(const Sim& s, What what, size_t from, bool (*also)(const Event&) = nullptr) {
    int n = 0;
    for (size_t i = from; i < s.events.size(); ++i)
        n += s.events[i].what == what && (!also || also(s.events[i]));
    return n;
}
void Front(Sim& s, int side, float seconds = 0.3f) {
    s.MoveTo(side, kFront[0], kFront[1], kFront[2]);
    s.Run(seconds);
}
void Squeeze(Sim& s, int side) {
    s.hand[side].squeeze = 1.0f;
    s.armed[side] = true;
}
void NewKit(Sim& s, uint32_t idAdd) {  // (a new life's weapon objects: new identities, the same kinds)
    for (int k = 0; k < kSlots; ++k)
        if (s.slots[k].carried)
            s.slots[k].id += idAdd;
}
// X held by a gun on the floor: R held 1.2 s (a pick each frame).
void HoldX(Sim& s) {
    for (int k = 0; k < 86; ++k) {
        s.picked = true, s.pickSlot = -1;
        s.Step();
    }
}

void Rows() {
    {
        // V1 (the first review's #2): Swordbreaker's roof AT4, the M4A1 holstered (with the game at the right shoulder), a miss: a death, the "RPG"
        // checkpoint (the AT4 in slot 0, the Mk11, the M67), the hit; the level's kit gives the M4A1 back: never dropped, its right shoulder whatever
        // the timing (15 s or 5 s after the respawn), the AT4 a gift as today or a take as the AT4 branch marks it; without a death as before.
        const char* name[4] = {"gift, back 15 s after the respawn", "take (AT4 branch), back 15 s after", "gift, back 5 s after", "no death (control)"};
        Role got[4] = {};
        bool weakBefore[4] = {}, atPlace[4] = {};
        int saidAway[4] = {};
        std::string line[4];
        for (int v = 0; v < 4; ++v) {
            World w;
            Sim s = SwordWorld(w);
            PutOn(s, 1, kRS);
            Front(s, 1, 20.0f);
            if (v == 1) {
                Took(s, 0, 0xA790, kAt4Kind, true, true);
                CopyName(s.slots[0].name, "AT4");
                s.Switch(0, true);
            } else {
                Gave(s, 0, 0xA790, kAt4Kind, true, true);
                CopyName(s.slots[0].name, "AT4");
                s.Switch(0);
            }
            Squeeze(s, 1);
            s.Run(1.5f);
            const Away* a = AwayOf(s, kM4Kind);
            weakBefore[v] = a && !a->entity && a->role == Role::kPrimary;
            if (v != 3) {
                NewLife(s.st);
                Swordbreaker(s);
                Gave(s, 0, 0xA791, kAt4Kind, true, true);
                CopyName(s.slots[0].name, "AT4");
                s.slots[1].known = true;
                Named(s.slots[1], 0x1112, kMk11Kind, "Mk11");
                Named(s.slots[6], 0x6762, 0x67, "M67");
                s.holder = Holder::kHeld, s.trigger = 1, s.gameSlot = 0;
                Squeeze(s, 1);
                s.Run(v == 2 ? 5.0f : 15.0f);
            }
            LetGo(s, 1);  // (fired and thrown)
            s.Run(0.5f);
            const size_t from = s.events.size();
            Gave(s, 0, 0x4A19, kM4Kind);
            CopyName(s.slots[0].name, "M4A1");
            const ArriveOut o = s.Switch(0);
            s.Run(0.3f);
            got[v] = s.st.home[0].role;
            atPlace[v] = o.arrival == Arrival::kAtPlace && s.holder == Holder::kStowed;
            line[v] = LineIn(s, Last(s, What::kComeBack, -2, from));
            saidAway[v] = s.Count(What::kAway);  // (its "stays at the right shoulder" said once, in the life it went)
        }
        Report("V1", "Swordbreaker's roof AT4 with the M4A1 holstered, a miss (a death, the RPG checkpoint), then the hit and the kit's M4A1: still the right "
                     "shoulder's (never dropped), at its place, whatever the timing, the AT4 a gift or a take",
               weakBefore[0] && weakBefore[1] && got[0] == Role::kPrimary && got[1] == Role::kPrimary && got[2] == Role::kPrimary &&
                   got[3] == Role::kPrimary && atPlace[0] && atPlace[1] && atPlace[2] && atPlace[3] && saidAway[0] == 1 && saidAway[1] == 1 &&
                   saidAway[2] == 1,
               "%s: %s at its place %d (weak before %d, said %d) | %s: %s %d | %s: %s %d | %s: %s %d (\"%s\" | \"%s\")", name[0], R(got[0]), atPlace[0],
               weakBefore[0], saidAway[0], name[1], R(got[1]), atPlace[1], name[2], R(got[2]), atPlace[2], name[3], R(got[3]), atPlace[3],
               line[0].c_str(), line[2].c_str());
    }
    {
        // V1b: the record with the game through a new life ends as it should: a death whose checkpoint kit has the M4A1 again (before the roof: the kit's
        // M4A1 noted, its record gone, the primary by its memo); another level's kit (Rock's, from Replay Missions): gone, nothing said of it.
        World w;
        Sim s = SwordWorld(w);
        PutOn(s, 1, kRS);
        Front(s, 1, 20.0f);
        Gave(s, 0, 0xA792, kAt4Kind, true, true);
        CopyName(s.slots[0].name, "AT4");
        s.Switch(0);
        Squeeze(s, 1);
        s.Run(1.5f);
        Sim b = s;
        NewLife(s.st);
        Named(s.slots[0], 0x4A1B, kM4Kind, "M4A1");
        s.slots[0].launcher = false;
        NewKit(s, 0x100000);
        s.holder = Holder::kHeld, s.trigger = 1, s.gameSlot = 0;
        s.Run(1.0f);
        const bool kitBack = s.st.home[0].role == Role::kPrimary && s.st.home[1].role == Role::kSecondary && AwayCount(s) == 0;
        NewLife(b.st);
        Rock(b);
        Named(b.slots[0], 0x4162, kM416Kind, "M416");
        Named(b.slots[2], 0x3202, kM320Kind, "M320");
        Named(b.slots[5], 0x5A32, kSmawKind, "SMAW");
        Named(b.slots[6], 0x6702, 0x67, "M67");
        Named(b.slots[7], 0x7702, 0x77, "Knife");
        b.holder = Holder::kHeld, b.trigger = 1, b.gameSlot = 0;
        b.Run(1.0f);
        const bool level = b.st.memoForgot && AwayCount(b) == 0 && b.st.home[0].role == Role::kPrimary && b.st.home[5].role == Role::kLauncher;
        Report("V1b", "the M4A1 with the game through a new life: a checkpoint whose kit has it again ends its record (the primary by its memo); another "
                      "level's kit drops it",
               kitBack && level, "the checkpoint's kit: the M4A1 %s, the Mk11 %s, away %d; another level: forgot %d, away %d, the M416 %s, the SMAW %s",
               R(s.st.home[0].role), R(s.st.home[1].role), AwayCount(s), b.st.memoForgot, AwayCount(b), R(b.st.home[0].role), R(b.st.home[5].role));
    }
    {
        // V2: flow (a) twice in a row, two floor guns: the M416 stays away in the first pickup through the second take, drawn back with its rounds.
        World w;
        Sim s = RockWorld(w);
        w.loaded[0] = 27, w.spare[0] = 120;
        const int an = w.AddPick(0xD1, 1.0f, 0, kAn94Kind, 30, 90, false, "AN-94");
        const int ak = w.AddPick(0xD2, 1.5f, 0, kAkKind, 30, 60, false, "AK-74M");
        PutOn(s, 1, kRS);
        Front(s, 1);
        TakeOf(s, w, an, 1);
        Squeeze(s, 1);
        s.Run(0.3f);
        LetGo(s, 1);
        s.Run(0.5f);
        TakeOf(s, w, ak, 1);
        Squeeze(s, 1);
        s.Run(0.3f);
        const Away* a = AwayOf(s, kM416Kind);
        const bool stillAway = a && a->entity == 0xD1;
        LetGo(s, 1);
        s.Run(0.5f);
        SqueezeAt(s, 0, kRS, 0.8f);
        const bool back = s.slots[0].kind == kM416Kind && w.loaded[0] == 27 && w.spare[0] == 120 && s.holder == Holder::kHeld && s.trigger == 0 &&
                          s.st.home[0].role == Role::kPrimary && AwayCount(s) == 0;
        const bool places = w.pick[an].gun.kind == kAkKind && w.pick[ak].gun.kind == kAn94Kind;
        Report("V2", "two floor guns in a row (flow a twice): the M416 stays away in the first pickup through the second take, drawn back with its rounds; "
                     "each gun in one place",
               stillAway && back && places && Copies(s, w, kM416Kind) == 1 && Copies(s, w, kAn94Kind) == 1 && Copies(s, w, kAkKind) == 1,
               "away through the 2nd take %d; back %d; P1 %X P2 %X; copies M416 %d AN-94 %d AK %d", stillAway, back, w.pick[an].gun.kind,
               w.pick[ak].gun.kind, Copies(s, w, kM416Kind), Copies(s, w, kAn94Kind), Copies(s, w, kAkKind));
    }
    {
        // V3: two guns sharing slot 0 by two pickups, one on each shoulder by hand: each drawn back at its own shoulder in turn, each in one place.
        World w;
        Sim s = RockWorld(w);
        w.loaded[0] = 27, w.spare[0] = 120;
        const int an = w.AddPick(0xD3, 1.0f, 0, kAn94Kind, 30, 90, false, "AN-94");
        const int ak = w.AddPick(0xD4, 1.4f, 0, kAkKind, 30, 60, false, "AK-74M");
        PutOn(s, 1, kRS);
        Front(s, 1);
        TakeOf(s, w, an, 1);
        Squeeze(s, 1);
        s.Run(0.3f);
        PutOn(s, 1, kLS);
        const bool anLeft = s.holder == Holder::kStowed && s.st.home[0].role == Role::kSecondary;
        Front(s, 1);
        TakeOf(s, w, ak, 1);
        Squeeze(s, 1);
        s.Run(0.3f);
        const Away* aAn = AwayOf(s, kAn94Kind);
        const bool anAway = aAn && aAn->entity == 0xD4 && aAn->role == Role::kSecondary && AwayCount(s) == 2;
        LetGo(s, 1);
        s.Run(0.5f);
        SqueezeAt(s, 0, kLS, 0.8f);
        const bool anBack = s.slots[0].kind == kAn94Kind && s.holder == Holder::kHeld && s.trigger == 0 && s.st.home[0].role == Role::kSecondary;
        s.armed[0] = true;
        PutOn(s, 0, kLS);
        s.hand[0].squeeze = 0.0f;
        Front(s, 0);
        SqueezeAt(s, 1, kRS, 0.8f);
        const bool m416Back = s.slots[0].kind == kM416Kind && w.loaded[0] == 27 && s.st.home[0].role == Role::kPrimary && s.holder == Holder::kHeld &&
                              s.trigger == 1;
        const Away* aAn2 = AwayOf(s, kAn94Kind);
        const bool anAway2 = aAn2 && aAn2->entity == 0xD3 && aAn2->role == Role::kSecondary && AwayCount(s) == 1;
        Report("V3", "two guns sharing slot 0 through two pickups (one on each shoulder by hand): each drawn back at its own shoulder in turn, each gun in "
                     "one place",
               anLeft && anAway && anBack && m416Back && anAway2 && Copies(s, w, kM416Kind) == 1 && Copies(s, w, kAn94Kind) == 1 &&
                   Copies(s, w, kAkKind) == 1,
               "AN-94 left %d, away in P2 %d, back %d; M416 back %d; AN-94 away in P1 %d", anLeft, anAway, anBack, m416Back, anAway2);
    }
    {
        // V4 (the first review's #1): flow (a), the rifle taken back off the floor with X held (the game's own R) instead of a draw: picked up from the
        // ground, so into the hand (as a hand's take is), on its shoulder still (it was never dropped), never put away there by itself. V4b: the same take
        // with its swap told a frame after its gun is seen in slot 0 (the game's own take publishes its swap after it: the second review's #1, past the kit's
        // seconds): the same, said once as the take's, never "left in the pickup".
        Arrival arrival[2] = {Arrival::kAtPlace, Arrival::kAtPlace};
        Holder holder[2] = {Holder::kStowed, Holder::kStowed};
        Role role[2] = {};
        int away[2] = {}, takeLines[2] = {}, leftLines[2] = {};
        std::string line[2];
        for (int v = 0; v < 2; ++v) {
            World w;
            Sim s = RockWorld(w);
            w.loaded[0] = 27, w.spare[0] = 120;
            const int at4 = w.AddPick(0xD5, 1.2f, 0, kAt4Kind, 1, 0, true, "AT4");
            PutOn(s, 1, kRS);
            Front(s, 1, v == 1 ? 10.5f : 0.3f);
            TakeOf(s, w, at4, 1);
            Squeeze(s, 1);
            s.Run(0.3f);
            LetGo(s, 1);
            s.Run(0.5f);  // (the AT4 fired and thrown: it lies)
            HoldX(s);
            const size_t from = s.events.size();
            ArriveOut o;
            if (v == 0) {
                TakeOf(s, w, at4, -1);
                o = s.Switch(0);
            } else {
                Gave(s, 0, 0x4169, kM416Kind);
                CopyName(s.slots[0].name, "M416");
                o = s.Switch(0);
                s.picked = true;
                s.Step();
                SwapIn in;
                in.changed = 1, in.slot[0] = 0, in.entity = 0xD5;
                in.came[0] = kM416Kind, in.went[0] = kAt4Kind, in.wentBlueprint[0] = kAt4Kind + kBlueprintOf;
                CopyName(in.taken, "M416");
                NoteSwap(s.set, s.slots, in, s.st);
                s.taken |= 1u;
                World::Gun g;
                g.kind = kAt4Kind, g.loaded = 0, g.launcher = true;
                CopyName(g.name, "AT4");
                w.pick[at4].gun = g;
            }
            s.picked = true;
            s.Step();
            s.Run(1.5f);
            arrival[v] = o.arrival, holder[v] = s.holder, role[v] = s.st.home[0].role, away[v] = AwayCount(s);
            takeLines[v] = CountFrom(s, What::kComeBack, from, [](const Event& e) { return !e.weak; });
            leftLines[v] = CountFrom(s, What::kComeBack, from, [](const Event& e) { return e.pickupLeft; });
            line[v] = LineIn(s, Last(s, What::kComeBack, -2, from));
        }
        Report("V4", "flow (a): the M416 taken back off the floor by X held (the game's R, the AT4 lying): into the hand, as a hand's take, still the "
                     "primary; never put away at the shoulder by itself",
               arrival[0] != Arrival::kAtPlace && holder[0] == Holder::kHeld && role[0] == Role::kPrimary && away[0] == 0 && takeLines[0] == 1,
               "arrival %d, holder %d, %s, away %d (\"%s\")", static_cast<int>(arrival[0]), static_cast<int>(holder[0]), R(role[0]), away[0], line[0].c_str());
        Report("V4b", "... its swap told a frame after the M416 is seen in slot 0 (past the kit's seconds): the same (into the hand, the primary), said once "
                      "as the take's, never \"left in the pickup\"",
               arrival[1] != Arrival::kAtPlace && holder[1] == Holder::kHeld && role[1] == Role::kPrimary && away[1] == 0 && takeLines[1] == 1 &&
                   leftLines[1] == 0,
               "arrival %d, holder %d, %s, away %d; take's lines %d, \"left in the pickup\" %d (\"%s\")", static_cast<int>(arrival[1]),
               static_cast<int>(holder[1]), R(role[1]), away[1], takeLines[1], leftLines[1], line[1].c_str());
    }
    {
        // V5: Rock's Stinger (the level's own pickup, naming slot 1, keeping no place for the gun it replaces) with a picked-up AN-94 on the left shoulder:
        // the take goes ahead (the jet), the AN-94 gone (said); the Stinger then put on the left shoulder and drawn from there.
        World w;
        Sim s = RockWorld(w);
        PutOn(s, 1, kRS);
        Front(s, 1);
        const int an = w.AddPick(0xD6, 1.0f, 1, kAn94Kind, 30, 90, false, "AN-94");
        TakeOf(s, w, an, 1);
        Squeeze(s, 1);
        s.Run(0.3f);
        PutOn(s, 1, kLS);
        const bool placed = s.st.home[1].role == Role::kSecondary && s.holder == Holder::kStowed && s.gameSlot == 1;
        Front(s, 1);
        const TakeOut req = TakeCheck(s.set, s.slots, s.st, s.holder, 1u << 1, true, false, false, true);
        const TakeOut plain = TakeCheck(s.set, s.slots, s.st, s.holder, 1u << 1, true, false, false, false);
        SwapIn in;
        in.changed = 1, in.slot[0] = 1, in.came[0] = kStingerKind, in.went[0] = kAn94Kind, in.wentBlueprint[0] = kAn94Kind + kBlueprintOf;
        in.entity = 0xD7, in.removed = true, in.noPlace = true;
        CopyName(in.taken, "Stinger");
        const size_t from = s.events.size();
        NoteSwap(s.set, s.slots, in, s.st);
        Took(s, 1, 0xF1920, kStingerKind, true, true);
        CopyName(s.slots[1].name, "Stinger");
        s.picked = true, s.pickSlot = -1;
        s.Switch(1);
        s.Step();
        Squeeze(s, 1);
        s.Run(0.3f);
        const Event* lost = Kept(Last(s, What::kLost, -2, from), 0);  // (kept: the list grows with the frames after)
        const bool goneSaid = lost && lost->why == Why::kGone;
        const bool inHand = s.holder == Holder::kHeld && s.st.home[1].role == Role::kNone;
        PutOn(s, 1, kLS);
        const bool on = s.holder == Holder::kStowed && s.st.home[1].role == Role::kLauncher;
        SqueezeAt(s, 0, kLS, 0.3f);
        const bool drawn = s.holder == Holder::kHeld && s.trigger == 0;
        Report("V5", "Rock's Stinger with a picked-up AN-94 on the left shoulder: the take goes ahead (a plain floor gun's refused), the AN-94 gone (said), "
                     "the Stinger on no shoulder in the hand, then put on the left shoulder and drawn from there",
               placed && req.verdict == TakeVerdict::kAway && req.lost && plain.verdict == TakeVerdict::kRefuse && goneSaid && inHand && on && drawn,
               "placed %d; required %d lost %d; plain %d; \"%s\"; in hand %d; on %d; drawn %d", placed, static_cast<int>(req.verdict), req.lost,
               static_cast<int>(plain.verdict), LineIn(s, lost).c_str(), inHand, on, drawn);
    }
    {
        // V6 (info: the first review's #6 and the second's #3, low, kept): the bridge's M249 put on the right shoulder (the M4A1 there on no shoulder,
        // in its pickup), the roof's AT4 by its kit (the M249 with the game), the kit's M4A1 back: on no shoulder, into the right hand (a gun the game
        // gives goes on a shoulder only by a hand: it isn't the M249); the M249's record with the game stays till a hand puts a gun on that shoulder
        // (it never keeps a shoulder: a put-away there wins, and ends it).
        World w;
        Sim s = SwordWorld(w);
        const int m249 = w.AddPick(0xD8, 1.0f, 0, kM249Kind, 100, 200, false, "M249");
        PutOn(s, 1, kRS);
        Front(s, 1, 20.0f);
        TakeOf(s, w, m249, 1);
        Squeeze(s, 1);
        s.Run(0.5f);
        PutOn(s, 1, kRS);
        const bool onRight = s.holder == Holder::kStowed && s.st.home[0].role == Role::kPrimary && !AwayOf(s, kM4Kind);
        w.pick[m249].distance = 200.0f;
        Front(s, 1, 2.0f);
        Gave(s, 0, 0xA7A0, kAt4Kind, true, true);
        CopyName(s.slots[0].name, "AT4");
        s.Switch(0);
        Squeeze(s, 1);
        s.Run(1.5f);
        LetGo(s, 1);
        s.Run(0.5f);
        Gave(s, 0, 0x4A1A, kM4Kind);
        CopyName(s.slots[0].name, "M4A1");
        const ArriveOut ok = s.Switch(0);
        s.Run(1.2f);
        const Role m4 = s.st.home[0].role;
        const bool staleWeak = AwayOf(s, kM249Kind) != nullptr;
        s.holder = Holder::kHeld, s.trigger = 1, s.armed[1] = true;
        s.hand[1].squeeze = 1.0f;
        s.Run(0.2f);
        PutOn(s, 1, kRS);
        const bool put = s.holder == Holder::kStowed && s.st.home[0].role == Role::kPrimary && !AwayOf(s, kM249Kind);
        Report("V6", "info: the bridge's M249 on the right shoulder, the roof's AT4, the kit's M4A1 back: on no shoulder (into the right hand); the M249's "
                     "record with the game till a gun is put on that shoulder, which wins",
               onRight && m4 == Role::kNone && ok.arrival != Arrival::kAtPlace && put,
               "the M249 on the right %d; the kit's M4A1 %s (arrival %d); the M249's record %s; the M4A1 put on the right: %d", onRight, R(m4),
               static_cast<int>(ok.arrival), staleWeak ? "with the game still" : "gone", put);
    }
    {
        // V7 (the first review's #4, the second's #2): the fired AT4 put on the launchers' place (the left shoulder, the SMAW's too), the M416 drawn back
        // (the hung AT4 into the pickup: away there); 10 m on, a squeeze at the left shoulder: the SMAW (key 4), never refused for the far AT4. V7b: within
        // the game's reach of that pickup, the one in the hands last (the AT4, by the game's take of its pickup), nothing lost.
        bool ok[2] = {};
        std::string line[2];
        for (int v = 0; v < 2; ++v) {
            World w;
            Sim s = RockWorld(w);
            w.loaded[0] = 27, w.spare[0] = 120;
            const int at4 = w.AddPick(0xD9, 1.2f, 0, kAt4Kind, 1, 0, true, "AT4");
            PutOn(s, 1, kRS);
            Front(s, 1);
            TakeOf(s, w, at4, 1);
            Squeeze(s, 1);
            s.Run(0.3f);
            PutOn(s, 1, kLS);  // (the fired AT4 put on the launchers' place, not thrown)
            const bool at4On = s.holder == Holder::kStowed && s.st.home[0].role == Role::kLauncher;
            Front(s, 1);
            SqueezeAt(s, 1, kRS, 0.8f);  // (the M416 drawn back: the hung AT4 into the pickup)
            const bool m416Back = s.slots[0].kind == kM416Kind && s.holder == Holder::kHeld && s.trigger == 1;
            const Away* a = AwayOf(s, kAt4Kind);
            const bool at4Away = a && a->entity == 0xD9 && a->role == Role::kLauncher;
            Squeeze(s, 1);
            s.Run(0.2f);
            PutOn(s, 1, kRS);
            if (v == 0)
                w.pick[at4].distance = 10.0f;  // (walked on)
            s.hand[1].squeeze = 0.0f;
            Front(s, 1, 1.0f);
            const int keys = s.Keys(5), asked = w.asked;
            const size_t from = s.events.size();
            SqueezeAt(s, 0, kLS, 0.4f);  // (within the second key's 0.6 s)
            const Event* refused = Last(s, What::kRefused, 0, from);
            line[v] = LineIn(s, refused);
            if (v == 0)
                ok[0] = at4On && m416Back && at4Away && s.Keys(5) == keys + 1 && !refused && w.asked == asked;
            else
                ok[1] = at4On && m416Back && at4Away && s.Keys(5) == keys && w.asked == asked + 1 && s.slots[0].kind == kAt4Kind &&
                        AwayOf(s, kM416Kind) && AwayOf(s, kM416Kind)->role == Role::kPrimary && Copies(s, w, kM416Kind) == 1 &&
                        Copies(s, w, kAt4Kind) == 1;
        }
        Report("V7", "the SMAW at the left shoulder with a fired AT4 away there in a pickup 10 m back: a draw at the left gives the SMAW (key 4), never "
                     "refused for the far AT4",
               ok[0], "\"%s\"", line[0].c_str());
        Report("V7b", "... within the game's reach of that pickup: the one in the hands last, the AT4, by the game's take of its pickup (the M416 into it, "
                      "away at the right shoulder), each gun in one place",
               ok[1], "\"%s\"", line[1].c_str());
    }
    {
        // V8 (the first review's #5, the second's #4): a take of the gun in the hand (the game's R with the M416 held: X held by a floor AN-94), told in
        // time or a frame late: the same, no record (it went from the hand into the pickup, on the floor: the gun in the hands a take swaps out isn't
        // holstered). V8b: the roof's kit taking it from the hand with no pickup still keeps it with the game (X4, a script's: never dropped).
        bool awayInTime = true, awayLate = true;
        std::string line;
        for (int v = 0; v < 2; ++v) {
            World w;
            Sim s = RockWorld(w);
            Front(s, 1, 12.0f);
            if (v == 0) {
                const int an = w.AddPick(0xDA, 1.0f, 0, kAn94Kind, 30, 90, false, "AN-94");
                TakeOf(s, w, an, -1);
                s.Switch(0);
                s.Run(1.5f);
                awayInTime = AwayOf(s, kM416Kind) != nullptr;
            } else {
                Gave(s, 0, 0x9402, kAn94Kind);
                CopyName(s.slots[0].name, "AN-94");
                s.Switch(0);
                s.Step();
                SwapIn in;
                in.changed = 1, in.slot[0] = 0, in.came[0] = kAn94Kind, in.went[0] = kM416Kind, in.wentBlueprint[0] = kM416Kind + kBlueprintOf;
                in.entity = 0xDB;
                CopyName(in.taken, "AN-94");
                NoteSwap(s.set, s.slots, in, s.st);
                s.taken = 1u << 0;
                s.Run(1.5f);
                const Away* a = AwayOf(s, kM416Kind);
                awayLate = a != nullptr;
                line = a ? (a->entity ? "strong" : "weak") : "none";
            }
        }
        Report("V8", "a take of the gun in the hand (the game's R with the M416 held), told in time or a frame late: the same, no record (it went from the "
                     "hand into the pickup)",
               !awayInTime && !awayLate, "in time: away %d; a frame late: away %d (%s)", awayInTime, awayLate, line.c_str());
    }
    {
        // V9 (info: the first review's #7, low, kept): another level whose kit is one rifle of a kind picked up (never placed) in the level before is
        // taken for the same level (every gun of it with a holster by its slot was carried): its rifle on no shoulder. Telling levels apart for sure
        // needs the level's own name, which the mod doesn't read yet.
        World w;
        Sim s = RockWorld(w);
        PutOn(s, 1, kRS);
        Front(s, 1);
        const int m16 = w.AddPick(0xDC, 1.0f, 0, kM16Kind, 30, 90, false, "M16A4");
        TakeOf(s, w, m16, 1);
        Squeeze(s, 1);
        s.Run(0.5f);
        NewLife(s.st);
        for (SlotIn& v : s.slots)
            v = SlotIn{};
        s.slotRead = false;
        s.Run(15.0f);
        NewLife(s.st);
        s.slots[0].carried = true, s.slots[0].known = true;
        Named(s.slots[0], 0x16A40, kM16Kind, "M16A4");
        s.slots[6].carried = true;
        Named(s.slots[6], 0x6770, 0x67, "M67");
        s.slotRead = true, s.gameSlot = 0, s.holder = Holder::kHeld, s.trigger = 1;
        s.Run(1.0f);
        Report("V9", "info: a new level whose kit is one rifle of a kind picked up (never placed) in the level before: taken for the same level", true,
               "the kit's M16A4 %s (memo forgot %d)", R(s.st.home[0].role), s.st.memoForgot);
    }
    {
        // V10: a level change mid-flow with a long load (two new lives, the empty one over 10 s): Swordbreaker's kit by its slots, nothing away.
        World w;
        Sim s = RockWorld(w);
        PutOn(s, 1, kLS);
        Front(s, 1);
        const int an = w.AddPick(0xDD, 1.0f, 0, kAn94Kind, 30, 90, false, "AN-94");
        TakeOf(s, w, an, 1);
        Squeeze(s, 1);
        s.Run(0.5f);
        NewLife(s.st);
        for (SlotIn& v : s.slots)
            v = SlotIn{};
        s.slotRead = false;
        s.Run(15.0f);
        NewLife(s.st);
        Swordbreaker(s);
        Named(s.slots[0], 0x4A40, kM4Kind, "M4A1");
        s.slots[1].known = true;
        Named(s.slots[1], 0x1140, kMk11Kind, "Mk11");
        Named(s.slots[6], 0x6740, 0x67, "M67");
        s.slotRead = true, s.gameSlot = 0, s.holder = Holder::kHeld, s.trigger = 1;
        s.Run(1.0f);
        Report("V10", "a level change mid-flow (a long load): Swordbreaker's kit by its slots, nothing away",
               s.st.home[0].role == Role::kPrimary && s.st.home[1].role == Role::kSecondary && AwayCount(s) == 0, "the M4A1 %s, the Mk11 %s, away %d, "
               "forgot %d", R(s.st.home[0].role), R(s.st.home[1].role), AwayCount(s), s.st.memoForgot);
    }
    {
        // V11: a death mid-flow (a), the checkpoint before the take: the kit's M416 the primary, nothing away, the AT4's memo none.
        World w;
        Sim s = RockWorld(w);
        const int at4 = w.AddPick(0xDE, 1.2f, 0, kAt4Kind, 1, 0, true, "AT4");
        PutOn(s, 1, kRS);
        Front(s, 1);
        TakeOf(s, w, at4, 1);
        Squeeze(s, 1);
        s.Run(0.5f);
        NewLife(s.st);
        Rock(s);
        Named(s.slots[0], 0x4161, kM416Kind, "M416");
        Named(s.slots[2], 0x3201, kM320Kind, "M320");
        Named(s.slots[5], 0x5A31, kSmawKind, "SMAW");
        Named(s.slots[6], 0x6701, 0x67, "M67");
        Named(s.slots[7], 0x7701, 0x77, "Knife");
        s.gameSlot = 0, s.holder = Holder::kHeld, s.trigger = 1;
        s.Run(1.0f);
        const Memo* m = MemoOf(s.st, kAt4Kind);
        Report("V11", "a death mid-flow (a), the checkpoint before the take: the kit's M416 the primary, nothing away, the AT4's memo none",
               s.st.home[0].role == Role::kPrimary && AwayCount(s) == 0 && m && m->role == Role::kNone, "the M416 %s, away %d, AT4 memo %s",
               R(s.st.home[0].role), AwayCount(s), m ? R(m->role) : "-");
    }
    {
        // V12: kept in the hand at a refusing shoulder twice running (never falls), then put away at the free shoulder.
        World w;
        Sim s = An94Taken(w);
        s.Move(1, kRS);
        s.Run(0.2f);
        LetGo(s, 1);
        s.Run(0.5f);
        const bool kept1 = s.holder == Holder::kHeld && s.falls == 0;
        Squeeze(s, 1);
        s.Run(0.2f);
        LetGo(s, 1);
        s.Run(0.5f);
        const bool kept2 = s.holder == Holder::kHeld && s.falls == 0;
        Squeeze(s, 1);
        s.Run(0.2f);
        PutOn(s, 1, kLS);
        const bool left = s.holder == Holder::kStowed && s.st.home[1].role == Role::kSecondary;
        Report("V12", "kept in the hand at a refusing shoulder twice running (never falls), then put away at the free shoulder", kept1 && kept2 && left,
               "kept %d %d, refusals said %d, falls %d; left %d", kept1, kept2, s.Count(What::kStowRefused, 1), s.falls, left);
    }
    {
        // V13: the weapon buttons mid-flow (a): Y held to the SMAW with the AT4 thrown, the SMAW put away; the M416 still drawn back at the right
        // shoulder, the AT4 into its pickup.
        World w;
        Sim s = RockWorld(w);
        w.loaded[0] = 27, w.spare[0] = 120;
        const int at4 = w.AddPick(0xDF, 1.2f, 0, kAt4Kind, 1, 0, true, "AT4");
        PutOn(s, 1, kRS);
        Front(s, 1);
        TakeOf(s, w, at4, 1);
        Squeeze(s, 1);
        s.Run(0.3f);
        LetGo(s, 1);
        s.Run(0.5f);
        s.picked = true, s.pickSlot = 5;
        s.Step();
        s.Switch(5);
        Squeeze(s, 1);
        s.Run(0.3f);
        const bool smaw = s.holder == Holder::kHeld && s.gameSlot == 5;
        const bool stillAway = AwayOf(s, kM416Kind) != nullptr;
        PutOn(s, 1, kLS);
        const bool smawAway = s.holder == Holder::kStowed && s.st.home[5].role == Role::kLauncher;
        Front(s, 1);
        SqueezeAt(s, 0, kRS, 0.8f);
        const bool back = s.slots[0].kind == kM416Kind && s.holder == Holder::kHeld && s.trigger == 0 && s.st.home[0].role == Role::kPrimary &&
                          w.pick[at4].gun.kind == kAt4Kind;
        Report("V13", "the weapon buttons mid-flow (a): Y held to the SMAW with the AT4 thrown, the SMAW put away; the M416 still drawn back at the right "
                      "shoulder, the AT4 into its pickup",
               smaw && stillAway && smawAway && back && Copies(s, w, kAt4Kind) == 1 && Copies(s, w, kM416Kind) == 1,
               "SMAW %d; away %d; SMAW put away %d; back %d; copies AT4 %d M416 %d", smaw, stillAway, smawAway, back, Copies(s, w, kAt4Kind),
               Copies(s, w, kM416Kind));
    }
    {
        // V14: a picked-up AN-94 through a death: put on the left shoulder, the secondary after; never placed, none.
        Role placedRole = Role::kNone, unplacedRole = Role::kPrimary;
        for (int v = 0; v < 2; ++v) {
            World w;
            Sim s = An94Taken(w);
            if (v == 0)
                PutOn(s, 1, kLS);
            NewLife(s.st);
            NewKit(s, 0x100000);
            s.gameSlot = 0, s.holder = Holder::kHeld, s.trigger = 1;
            s.Run(1.0f);
            (v == 0 ? placedRole : unplacedRole) = s.st.home[1].role;
        }
        Report("V14", "a picked-up AN-94 through a death: put on the left shoulder, the secondary after; never placed, none",
               placedRole == Role::kSecondary && unplacedRole == Role::kNone, "placed: %s; never placed: %s", R(placedRole), R(unplacedRole));
    }
    {
        // V15 (the first review's #3): one gun a shoulder through a death too: the kit's M416 (the right's) dropped, an AN-94 taken into slot 1 and put on
        // the RIGHT shoulder by hand (the newest word for it), the M416 taken back off the ground and never placed; a death whose checkpoint has both: the
        // AN-94 the primary (placed), the M416 on no shoulder (dropped). V15b: the M416 left on the floor (the game gives it back at the checkpoint).
        Role an[2] = {}, m416[2] = {};
        bool anRight[2] = {};
        for (int v = 0; v < 2; ++v) {
            World w;
            Sim s = RockWorld(w);
            Front(s, 1, 0.2f);
            LetGo(s, 1);
            s.Run(0.5f);
            const int pick = w.AddPick(0xE0, 1.0f, 1, kAn94Kind, 30, 90, false, "AN-94");
            TakeOf(s, w, pick, 1);
            Squeeze(s, 1);
            s.Run(0.3f);
            PutOn(s, 1, kRS);
            anRight[v] = s.holder == Holder::kStowed && s.st.home[1].role == Role::kPrimary;
            if (v == 0) {
                Front(s, 1);
                s.holder = Holder::kLoose, s.trigger = -1;  // (the game's weapon the M416 again, lying)
                s.gameSlot = 0;
                s.Run(0.2f);
                s.takeBy = 1;
                Squeeze(s, 1);
                s.Run(0.3f);
            }
            NewLife(s.st);
            NewKit(s, 0x100000);
            s.gameSlot = 0, s.holder = Holder::kHeld, s.trigger = 1;
            s.Run(1.0f);
            an[v] = s.st.home[1].role, m416[v] = s.st.home[0].role;
        }
        Report("V15", "the kit's M416 dropped, an AN-94 put on the right shoulder by hand, the M416 taken back and never placed; a death: the AN-94 the "
                      "primary, the M416 none",
               anRight[0] && an[0] == Role::kPrimary && m416[0] == Role::kNone, "AN-94 right %d; after the death: the AN-94 %s, the M416 %s", anRight[0],
               R(an[0]), R(m416[0]));
        Report("V15b", "... the M416 left on the floor (the checkpoint gives it back): the AN-94 the primary, the M416 none",
               anRight[1] && an[1] == Role::kPrimary && m416[1] == Role::kNone, "AN-94 right %d; after the death: the AN-94 %s, the M416 %s", anRight[1],
               R(an[1]), R(m416[1]));
    }
    {
        // Y9 (the second review's #1, its R9): flow (a)'s draw back, the game's take of the M416's pickup with its swap told a frame after the M416 is
        // seen in slot 0: the primary, as with the swap first (HA2), said once as the take's, never "left in the pickup", nothing away.
        World w;
        Sim s = RockWorld(w);
        const int at4 = w.AddPick(0xE9, 1.0f, 0, kAt4Kind, 1, 0, true, "AT4");
        PutOn(s, 1, kRS);
        Front(s, 1, 10.3f);
        TakeOf(s, w, at4, 1);
        Squeeze(s, 1);
        s.Run(1.5f);
        LetGo(s, 1);
        s.Run(0.5f);
        w.never = true;  // (the test's game: the give-back served by hand below)
        SqueezeAt(s, 0, kRS, 0.1f);
        const bool asked = w.asked == 1;
        const size_t from = s.events.size();
        Gave(s, 0, 0x4169, kM416Kind);
        CopyName(s.slots[0].name, "M416");
        s.gameSlot = 0;
        const ArriveOut o = s.Switch(0, true);
        if (o.arrival != Arrival::kAtPlace)
            s.holder = Holder::kHeld, s.trigger = 0, s.barrel = -1, s.armed[0] = true;
        s.Step();
        SwapIn in;
        in.changed = 1, in.slot[0] = 0, in.entity = 0xE9;
        in.came[0] = kM416Kind, in.went[0] = kAt4Kind, in.wentBlueprint[0] = kAt4Kind + kBlueprintOf;
        CopyName(in.taken, "M416");
        NoteSwap(s.set, s.slots, in, s.st);
        s.taken |= 1u;
        s.Run(1.5f);
        const int takeLines = CountFrom(s, What::kComeBack, from, [](const Event& e) { return !e.weak; });
        const int leftLines = CountFrom(s, What::kComeBack, from, [](const Event& e) { return e.pickupLeft; });
        Report("Y9", "the draw back's take told a frame after its gun is seen in slot 0: the primary, as with the swap first; said once as the take's, "
                     "never \"left in the pickup\"; nothing away",
               asked && s.st.home[0].role == Role::kPrimary && takeLines == 1 && leftLines == 0 && AwayCount(s) == 0 && s.holder == Holder::kHeld,
               "asked %d; the M416 %s; take's lines %d, \"left in the pickup\" %d; away %d; holder %d (\"%s\")", asked, R(s.st.home[0].role), takeLines,
               leftLines, AwayCount(s), static_cast<int>(s.holder), LineIn(s, Last(s, What::kComeBack, -2, from)).c_str());
    }
}

}  // namespace second

// ---- TN: the owner's night of 2026-11-19 (build\logs\bf3vr_1119_userplay_holster.log, from 00:56:40: Operation Swordbreaker, the M4A1 and the Mk11)
// and the places made bigger for it: "My only issue is just making sure that the holsters were big enough so you wouldn't have to precisely find the
// area ... I should just be able to just swing it to my shoulders and easily let go and it'll holster ... you can just quickly draw your gun". ----

namespace tonight {

// Swordbreaker's kit as played that night: the M4A1 (slot 0, the right shoulder), the Mk11 (1, the left; the hands know it since 2026-11-18), the
// M67; the gun in slot stowed put away, both hands empty at rest in front. Defaults (launcher shoulder: no launcher carried, nothing at the neck).
Sim Kit(int stowed) {
    Sim s;
    s.set = Settings{};
    Swordbreaker(s);
    s.slots[1].known = true;
    s.gameSlot = stowed;
    s.holder = Holder::kStowed, s.trigger = -1;
    s.MoveTo(0, -kFront[0], kFront[1], kFront[2]);
    s.Run(0.1f);
    return s;
}
// The same with the gun of slot held in that hand (armed, squeezed), the other at rest.
Sim KitHeld(int slot, int side) {
    Sim s = Kit(slot);
    s.holder = Holder::kHeld, s.trigger = side, s.armed[side] = true;
    s.hand[side].squeeze = 1.0f;
    s.MoveTo(side, side == 1 ? 0.15f : -0.15f, -0.35f, 0.22f);  // the low ready by the belly (the night's own: 9-18 cm out, 29-39 down, 18-29 forward)
    s.Run(0.2f);
    return s;
}

// A squeeze as the log has it: the hand's place every 0.05 s from the squeeze on ("Tracking: the ... hand's path while squeezed"; cm right, up, forward
// of the neck point), whether it drew that night, the gun put away then.
struct Squeeze {
    const char* at;
    int side;
    bool drew;
    int stowed;
    int n;
    signed char path[12][3];
};
const Squeeze kNight[] = {
    {"00:57:14.605", 0, true, 0, 7, {{-25, 0, -3}, {-22, 0, 1}, {-22, 0, 8}, {-21, -1, 13}, {-20, -4, 23}, {-17, -13, 31}, {-16, -21, 34}}},
    {"00:57:31.788", 1, true, 1, 6, {{27, -2, -1}, {25, -3, -1}, {24, -2, 6}, {24, -3, 10}, {23, -5, 16}, {18, -19, 25}}},
    {"00:58:26.913", 1, false, 0, 8, {{-18, -3, 7}, {-18, -5, 10}, {-16, -9, 17}, {-15, -12, 20}, {-8, -20, 26}, {2, -30, 29}, {6, -33, 30}, {15, -39, 27}}},
    {"00:58:28.056", 1, true, 0, 7, {{-20, 0, 1}, {-19, -3, 3}, {-19, -5, 8}, {-18, -8, 16}, {-14, -12, 22}, {-8, -18, 28}, {-2, -24, 29}}},
    {"00:59:56.999", 1, false, 0, 8, {{-19, -6, 7}, {-17, -7, 8}, {-16, -8, 11}, {-14, -9, 14}, {-9, -13, 21}, {-2, -18, 27}, {5, -23, 30}, {12, -29, 30}}},
    {"00:59:58.252", 1, false, 0, 8, {{-18, -2, 6}, {-18, -3, 7}, {-18, -3, 7}, {-18, -3, 8}, {-17, -2, 9}, {-15, -3, 13}, {-14, -5, 18}, {-9, -9, 25}}},
    {"00:59:59.686", 1, false, 0, 11,
     {{-19, -3, 6}, {-18, -3, 6}, {-17, -3, 5}, {-17, -3, 5}, {-16, -3, 4}, {-15, -3, 4}, {-15, -2, 4}, {-14, -3, 4}, {-15, -3, 6}, {-14, -4, 12}, {-12, -8, 20}}},
    {"01:00:01.136", 1, true, 0, 7, {{-19, -1, 2}, {-19, -1, 3}, {-19, -2, 3}, {-19, -1, 2}, {-19, -1, 2}, {-19, -1, 3}, {-19, -1, 4}}},
    {"01:00:19.506", 1, true, 1, 1, {{-19, -3, 1}}},
    {"01:01:08.852", 1, true, 1, 8, {{21, -3, 2}, {21, -3, 1}, {20, -3, 4}, {20, -3, 8}, {18, -7, 14}, {16, -16, 21}, {15, -24, 24}, {15, -31, 24}}},
    {"01:02:27.702", 1, true, 0, 6, {{-20, -2, 2}, {-19, -3, 4}, {-19, -3, 7}, {-19, -4, 11}, {-16, -7, 17}, {-11, -12, 24}}},
    {"01:09:10.613", 1, false, 0, 8, {{-18, -5, 8}, {-17, -6, 8}, {-16, -7, 10}, {-14, -9, 13}, {-10, -12, 19}, {-4, -14, 24}, {1, -16, 27}, {9, -17, 29}}},
    {"01:09:12.025", 1, true, 0, 7, {{-19, -2, 3}, {-18, -2, 4}, {-18, -2, 6}, {-18, -2, 7}, {-17, -2, 9}, {-16, -3, 12}, {-14, -3, 16}}},
    {"01:11:57.672", 1, false, 0, 8, {{-19, -3, 7}, {-17, -3, 9}, {-16, -4, 13}, {-14, -6, 18}, {-10, -9, 24}, {-3, -13, 29}, {3, -17, 32}, {8, -22, 33}}},
    {"01:11:58.925", 1, false, 0, 8, {{-17, 0, 3}, {-18, 0, 3}, {-18, 0, 3}, {-18, 0, 4}, {-18, 0, 6}, {-17, -1, 9}, {-16, -2, 13}, {-13, -6, 21}}},
    {"01:12:00.774", 1, true, 0, 8, {{-17, 0, 1}, {-17, 0, 1}, {-17, 0, 0}, {-17, 0, 0}, {-18, 0, 1}, {-18, 0, 0}, {-17, 0, 2}, {-16, -1, 8}}},
    {"01:14:49.082", 1, true, 1, 7, {{23, 1, -3}, {22, 3, 0}, {22, 5, 4}, {21, 4, 10}, {19, -1, 18}, {17, -10, 27}, {15, -19, 30}}},
};

// The hand at its path's place t s after the squeeze (between its samples), and its speed there (m/s, body axes); still past its end.
void PathAt(const Squeeze& q, float t, float p[3], float v[3]) {
    const float k = t / 0.05f;
    int a = static_cast<int>(std::floor(k));
    if (a >= q.n - 1 || q.n == 1) {
        for (int i = 0; i < 3; ++i)
            p[i] = q.path[q.n - 1][i] / 100.0f, v[i] = 0.0f;
        return;
    }
    if (a < 0)
        a = 0;
    const float u = k - a;
    for (int i = 0; i < 3; ++i) {
        const float x0 = q.path[a][i] / 100.0f, x1 = q.path[a + 1][i] / 100.0f;
        p[i] = x0 + (x1 - x0) * u;
        v[i] = (x1 - x0) / 0.05f;
    }
}

// A swing of the gun in that hand from where it is to `to` over `seconds` (its speed a bell, 1.9 times the mean at the middle), the grip opening at
// `opens` of the way (1: as it gets there; past 1 it rests there first); the frames after it at rest there. Put away or fell.
void Swing(Sim& s, int side, const float to[3], float seconds, float opens) {
    const float from[3] = {s.hand[side].pos[0], s.hand[side].pos[1], s.hand[side].pos[2]};
    const int frames = std::max(2, static_cast<int>(std::lround(seconds / kDt)));
    const int openAt = static_cast<int>(std::lround(opens * frames));
    for (int f = 1; f <= frames + 20; ++f) {
        const float u = std::min(1.0f, static_cast<float>(f) / frames);
        const float e = u * u * (3.0f - 2.0f * u);                                // smoothstep: its speed 6u(1-u) of the mean
        const float speed = f <= frames ? 6.0f * u * (1.0f - u) / seconds : 0.0f;  // (d e / d t)
        float p[3], v[3];
        for (int i = 0; i < 3; ++i)
            p[i] = from[i] + (to[i] - from[i]) * e, v[i] = (to[i] - from[i]) * speed;
        s.MoveTo(side, p[0], p[1], p[2], v[0], v[1], v[2]);
        if (f >= openAt)
            s.hand[side].squeeze = 0.0f;
        s.Step();
        if (s.holder != Holder::kHeld)
            break;
    }
    s.Run(0.1f);
}

}  // namespace tonight

void Tonight() {
    using namespace tonight;
    const Egg old{{0.20f, -0.025f, -0.03f}, {0.07f, 0.08f, 0.07f}};  // the right shoulder until 2026-11-19 (the left its mirror)
    {
        // TN1: every squeeze at a shoulder that night replayed on its own path (the gun at that shoulder known, both hands empty): seven missed then
        // (the right hand across at the left shoulder, 3-8 cm in front of the neck point where the old egg's front was 4 cm; two of them already
        // pulling away at 0.5-0.7 m/s), ten drew. Each draws now: the Mk11 at the left (key 2, or in the hand at once when it was the gun put away),
        // the M4A1 at the right (key 1).
        int now = 0, missed = 0, missedNow = 0, wrong = 0;
        float oldMin = 9.0f, oldMax = 0.0f, newMost = 0.0f, fastest = 0.0f;
        char first[160] = "";
        for (const Squeeze& q : kNight) {
            Sim s = Kit(q.stowed);
            const int other = 1 - q.side;
            s.MoveTo(other, other == 1 ? kFront[0] : -kFront[0], kFront[1], kFront[2]);
            float p[3], v[3];
            PathAt(q, 0.0f, p, v);
            s.Move(q.side, p);
            s.hand[q.side].squeeze = 0.0f;
            s.Run(0.3f);
            const float at = s.time;
            for (int f = 0; f < 43; ++f) {  // (0.6 s)
                PathAt(q, f * kDt, p, v);
                s.MoveTo(q.side, p[0], p[1], p[2], v[0], v[1], v[2]);
                s.hand[q.side].squeeze = 1.0f;
                s.Step();
            }
            s.Run(0.3f);
            const bool left = q.path[0][0] < 0;
            const int slot = left ? 1 : 0;
            const Place want = left ? Place::kLeftShoulder : Place::kRightShoulder;
            // (Case B: its key at once; the test's game doesn't switch by itself, so the second key 0.6 s on is the draw's own check, not another draw.)
            const Event* asked = s.Find(What::kAsked, q.side);
            const bool drew = slot == q.stowed ? (s.draws == 1 && s.drawnBy == q.side && s.Keys() == 0)
                                               : (!s.keys.empty() && s.keys[0].second == slot && s.keys[0].first - at < 0.15f && s.Keys(slot) == s.Keys() &&
                                                  s.Count(What::kAsked) == 1 && asked && asked->place == want && asked->slot == slot);
            now += drew;
            const float p0[3] = {q.path[0][0] / 100.0f, q.path[0][1] / 100.0f, q.path[0][2] / 100.0f};
            const Egg oldSide = left ? Mirrored(old) : old;
            if (!q.drew) {
                ++missed, missedNow += drew;
                oldMin = std::min(oldMin, Size(oldSide, p0)), oldMax = std::max(oldMax, Size(oldSide, p0));
                if (q.n > 1) {
                    const float d[3] = {static_cast<float>(q.path[1][0] - q.path[0][0]), static_cast<float>(q.path[1][1] - q.path[0][1]),
                                        static_cast<float>(q.path[1][2] - q.path[0][2])};
                    fastest = std::max(fastest, std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]) / 100.0f / 0.05f);
                }
            }
            newMost = std::max(newMost, SizeFor(want, p0, s.set));
            if (!drew && !first[0]) {
                std::snprintf(first, sizeof(first), "; the first not drawn: %s (drawn %d, keys %d)", q.at, s.draws, s.Keys());
                ++wrong;
            }
        }
        const int count = static_cast<int>(sizeof(kNight) / sizeof(kNight[0]));
        Report("TN1", "that night's 17 squeezes at the shoulders replayed on their own paths: the 7 that missed (outside the old eggs, 2 pulling away) "
                      "and the 10 that drew all draw now, each its shoulder's gun",
               now == count && missed == 7 && missedNow == 7 && wrong == 0 && newMost < 0.7f,
               "%d of %d drawn; the 7 missed then: %.2f-%.2f of the old egg's size (1 its edge), up to %.1f m/s; now all within %.2f of the new%s", now,
               count, missed ? oldMin : 0.0f, oldMax, fastest, newMost, first);
    }
    {
        // TN2: the let-gos that fell by a shoulder that night, as near as the log can place them (no hand's place is logged at a let-go: the gun's own
        // place a second before, and where it was caught): the M4A1 let go in front of the right shoulder on its way up (01:08:15.666, caught at the
        // belly 0.26 s on), the Mk11 held across at the left shoulder by the right hand (01:14:42.896, the eye 17 cm to its right 0.76 s before: the
        // misses' place, 7 cm in front of the neck point), the M249 swung from the eye to the right shoulder (01:21:33.446, at the eye 0.3 s before,
        // 107 degrees off the look as it fell) faster than the old throw's 2.2 m/s. Each fell then (outside the old egg and its 3 cm, or a "throw");
        // each is put away now.
        int stowed = 0, fell = 0;
        bool oldFell[3] = {};
        {
            Sim s = KitHeld(0, 1);
            const float to[3] = {0.18f, -0.03f, 0.09f};
            oldFell[0] = Size(old, to, 0.03f) > 1.0f;
            Swing(s, 1, to, 0.3f, 1.0f);
            stowed += s.stows, fell += s.falls;
        }
        {
            Sim s = KitHeld(1, 1);
            const float to[3] = {-0.18f, -0.03f, 0.07f};
            oldFell[1] = Size(Mirrored(old), to, 0.03f) > 1.0f;
            Swing(s, 1, to, 0.45f, 1.5f);  // (there, then opened at rest)
            stowed += s.stows, fell += s.falls;
        }
        {
            Sim s = KitHeld(0, 1);
            s.MoveTo(1, 0.06f, 0.0f, 0.33f);  // the gun at the eye
            s.Run(0.2f);
            const float to[3] = {0.21f, -0.02f, -0.02f};
            Swing(s, 1, to, 0.2f, 0.5f);  // (let go two thirds of the way in, still going fast)
            const Event* e = s.Find(What::kStowed, 1);
            oldFell[2] = e && e->moving >= 2.2f;  // (a let-go that fast was a throw until then, wherever it was)
            stowed += s.stows, fell += s.falls;
        }
        Report("TN2", "that night's falls by a shoulder, replayed as near as the log places them (in front of the right shoulder; across at the left; "
                      "swung in from the eye, let go at 2.6 m/s): each fell then, each is put away now",
               stowed == 3 && fell == 0 && oldFell[0] && oldFell[1] && oldFell[2], "put away %d, fell %d; outside the old egg or thrown then: %d %d %d", stowed,
               fell, oldFell[0], oldFell[1], oldFell[2]);
    }
    {
        // TN3: fast swings to each shoulder by its own hand and across (from the low ready by the belly to the top, over it, behind it, out to the
        // side and in front of it, in 0.15-0.40 s: up to 4 m/s at the middle), the grip opening 3/4 of the way, as it gets there and resting there:
        // every one put away (none thrown, none fallen).
        const float ends[5][3] = {{0.21f, -0.02f, -0.03f}, {0.20f, 0.10f, -0.08f}, {0.20f, 0.0f, -0.16f}, {0.34f, -0.02f, -0.02f}, {0.17f, -0.05f, 0.12f}};
        const float times[3] = {0.15f, 0.25f, 0.40f};
        const float opens[3] = {0.75f, 1.0f, 1.3f};
        int tries = 0, stowed = 0, fell = 0, wrongPlace = 0;
        float fastest = 0.0f;
        char first[200] = "";
        for (int who = 0; who < 3; ++who)  // the right hand to the right (the M4A1), the left to the left (the Mk11), the right across to the left (the Mk11)
            for (const auto& end : ends)
                for (const float t : times)
                    for (const float o : opens) {
                        const int side = who == 1 ? 0 : 1, slot = who == 0 ? 0 : 1;
                        const float sign = who == 0 ? 1.0f : -1.0f;
                        Sim s = KitHeld(slot, side);
                        const float to[3] = {end[0] * sign, end[1], end[2]};
                        const float d[3] = {to[0] - s.hand[side].pos[0], to[1] - s.hand[side].pos[1], to[2] - s.hand[side].pos[2]};
                        fastest = std::max(fastest, 1.5f * std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]) / t);
                        Swing(s, side, to, t, o);
                        ++tries;
                        stowed += s.stows, fell += s.falls;
                        const Event* e = s.Find(What::kStowed, side);
                        const Place want = slot == 0 ? Place::kRightShoulder : Place::kLeftShoulder;
                        if (s.stows != 1 || !e || e->place != want) {
                            ++wrongPlace;
                            if (!first[0])
                                std::snprintf(first, sizeof(first), "; the first not put away: hand %d to (%.2f %.2f %.2f) in %.2f s opening at %.2f: put away "
                                              "%d, fell %d", side, to[0], to[1], to[2], t, o, s.stows, s.falls);
                        }
                    }
        Report("TN3", "fast swings to each shoulder (its own hand and across; to its top, over, behind, out to the side, in front; 0.15-0.40 s), let go "
                      "3/4 of the way, arriving or resting: every one put away at its gun's shoulder",
               stowed == tries && fell == 0 && wrongPlace == 0, "%d swings (up to %.1f m/s), put away %d, fell %d%s", tries, fastest, stowed, fell, first);
    }
    {
        // TN3b: the swing's end come early: let go 8 cm short of the right shoulder's put-away egg while still moving in at 1.5 m/s: put away (said as
        // swung in); the same hand at rest there, or moving across it, or out of it: it falls (said with where it was).
        const Egg e = Settings{}.shoulder;
        const float shortAt[3] = {e.mid[0], e.mid[1] - e.half[1] - Settings{}.stowMore - 0.08f, e.mid[2]};  // 8 cm under it
        const float ways[4][3] = {{0.0f, 1.5f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.5f, 0.0f, 0.0f}, {0.0f, -1.5f, 0.0f}};
        int stowed[4] = {}, fell[4] = {}, said[4] = {};
        bool swungLine = false;
        for (int k = 0; k < 4; ++k) {
            Sim s = KitHeld(0, 1);
            s.MoveTo(1, shortAt[0], shortAt[1], shortAt[2], ways[k][0], ways[k][1], ways[k][2]);
            s.Run(0.1f);
            s.hand[1].squeeze = 0.0f;
            s.Run(0.1f);
            stowed[k] = s.stows, fell[k] = s.falls, said[k] = s.Count(What::kFell, 1);
            if (k == 0) {
                const Event* st = s.Find(What::kStowed, 1);
                char line[256] = "";
                if (st)
                    Say(*st, kNames, line, sizeof(line));
                swungLine = st && st->how == StowHow::kSwing && std::strstr(line, "swung in: let go 8 cm short of its place at 1.5 m/s");
            }
        }
        Report("TN3b", "let go 8 cm under the shoulder's put-away egg moving up into it at 1.5 m/s: put away (swung in); at rest, across or away there: it "
                       "falls, said with where",
               stowed[0] == 1 && fell[0] == 0 && swungLine && stowed[1] == 0 && fell[1] == 1 && stowed[2] == 0 && fell[2] == 1 && stowed[3] == 0 &&
                   fell[3] == 1 && said[1] == 1 && said[2] == 1 && said[3] == 1,
               "moving in: put away %d (line %d); at rest: fell %d; across: fell %d; away: fell %d; their lines %d %d %d", stowed[0], swungLine, fell[1], fell[2],
               fell[3], said[1], said[2], said[3]);
    }
    {
        // TN4: the guards. (a) Both hands on the gun shouldered (the left on its handguard), the right lets go of the pistol grip in front of the right
        // shoulder and at its top: held by the left's front, never put away. (b) The gun at the eye (aiming) moving toward the shoulder, let go 12 cm
        // short of it: no swing (it falls). (c) Let go in front of the shoulder held out to aim (35 cm forward) going forward at 3 m/s: a throw.
        int putA = 0, frontA = 0;
        for (int k = 0; k < 2; ++k) {
            Sim s = KitHeld(0, 1);
            s.hand[0].onGun = true;
            s.MoveTo(1, k == 0 ? 0.16f : 0.21f, k == 0 ? -0.08f : -0.02f, k == 0 ? 0.10f : -0.03f, 0.0f, 2.0f, -1.0f);
            s.Run(0.1f);
            s.hand[1].squeeze = 0.0f;
            s.Run(0.1f);
            putA += s.stows + s.everStows[1];
            frontA += s.holder == Holder::kBarrel && s.barrel == 0 && s.falls == 0;
        }
        const Egg e = Settings{}.shoulder;
        Sim b = KitHeld(0, 1);
        b.hand[1].aims = true;
        b.MoveTo(1, e.mid[0], e.mid[1] - e.half[1] - Settings{}.stowMore - 0.12f, e.mid[2], 0.0f, 1.5f, 0.0f);
        b.Run(0.1f);
        b.hand[1].squeeze = 0.0f;
        b.Run(0.1f);
        Sim c = KitHeld(0, 1);
        c.MoveTo(1, 0.18f, -0.10f, 0.35f, 0.0f, 0.5f, 3.0f);
        c.Run(0.1f);
        c.hand[1].squeeze = 0.0f;
        c.Run(0.1f);
        Report("TN4", "both hands on the gun: the trigger hand's let-go at the shoulder is the other's hold, never put away; the gun at the eye moving "
                      "in: no swing; let go held out to aim going forward: falls",
               putA == 0 && frontA == 2 && b.stows == 0 && b.falls == 1 && c.stows == 0 && c.falls == 1,
               "both hands: put away %d, held by the front %d of 2; aiming: put away %d, fell %d; held out: put away %d, fell %d", putA, frontA, b.stows,
               b.falls, c.stows, c.falls);
    }
    {
        // TN4b: an empty hand's fresh squeeze, both hands empty, where no gun is to be drawn: at the side of the headset (13 cm out, 8 up, 5 forward:
        // inside the head's egg, by the shoulders), at the back of the head (nothing behind the neck: no launcher carried), in front of the throat,
        // at either chest, at the belly: nothing drawn, no key. And over the shoulder beside the head (15 cm out, 4 up: a put-away's place that night,
        // 01:02:26): drawn.
        const float spots[6][3] = {{0.13f, 0.08f, 0.05f}, {0.0f, 0.10f, -0.12f}, {0.0f, -0.06f, 0.10f}, {0.08f, -0.18f, 0.12f}, {-0.08f, -0.18f, 0.12f},
                                   {0.0f, -0.43f, 0.13f}};
        int drawn = 0, keys = 0;
        for (const auto& p : spots) {
            Sim s = Kit(0);
            s.MoveTo(0, -kFront[0], kFront[1], kFront[2]);
            SqueezeAt(s, 1, p, 0.4f);
            drawn += s.draws, keys += s.Keys();
        }
        const float overAt[3] = {0.15f, 0.04f, -0.02f};
        Sim o = Kit(1);
        SqueezeAt(o, 1, overAt, 0.4f);
        const bool over = o.Keys(0) == 1;
        const Settings d;
        Report("TN4b", "a fresh squeeze with both hands empty at the side of the headset, the back of the head, the throat, the chests, the belly: nothing "
                       "drawn; over the shoulder beside the head (15 cm out, 4 up): the M4A1 asked for",
               drawn == 0 && keys == 0 && over && InHead(spots[0], d) && PlaceAt(spots[0], d) == Place::kNone && PlaceAt(overAt, d) == Place::kRightShoulder,
               "drawn %d, keys %d; over the shoulder: key 1 %d; the headset's side in the head's egg %d", drawn, keys, o.Keys(0), InHead(spots[0], d));
    }
    {
        // TN5: the sizes are holsters.txt's: the old shoulder ("shoulder 0.20 -0.025 -0.03 0.07 0.08 0.07") misses the night's 7 again where they were
        // outside it; "swing 0" puts nothing away short of a place; "stowgrace 0 0" nothing just out of it.
        Settings oldSet;
        oldSet.shoulder = old;
        int drawnOld = 0;
        for (const Squeeze& q : kNight) {
            if (q.drew)
                continue;
            Sim s = Kit(q.stowed);
            s.set = oldSet;
            float p[3], v[3];
            PathAt(q, 0.0f, p, v);
            s.Move(q.side, p);
            s.Run(0.3f);
            for (int f = 0; f < 43; ++f) {
                PathAt(q, f * kDt, p, v);
                s.MoveTo(q.side, p[0], p[1], p[2], v[0], v[1], v[2]);
                s.hand[q.side].squeeze = 1.0f;
                s.Step();
            }
            s.Run(0.3f);
            drawnOld += s.Keys() + s.draws > 0;
        }
        const Egg e = Settings{}.shoulder;
        Settings noSwing = ParseText("swing 0\n"), noGrace = ParseText("stowgrace 0 0\n");
        Sim a = KitHeld(0, 1);
        a.set = noSwing;
        a.MoveTo(1, e.mid[0], e.mid[1] - e.half[1] - Settings{}.stowMore - 0.12f, e.mid[2], 0.0f, 1.5f, 0.0f);
        a.Run(0.1f);
        LetGo(a, 1);
        Sim g = KitHeld(0, 1);
        g.set = noGrace;
        g.MoveTo(1, kRS[0], kRS[1], kRS[2]);
        g.Run(0.2f);
        g.MoveTo(1, e.mid[0] + e.half[0] + Settings{}.stowMore + 0.06f, e.mid[1], e.mid[2]);
        g.Run(0.2f);
        LetGo(g, 1);
        Report("TN5", "the sizes are holsters.txt's: the old shoulder misses the night's 7 again where they lay outside it (the quick grab draws those it "
                      "had inside); swing 0: none short of a place; stowgrace 0 0: none just out of it",
               drawnOld <= 2 && a.stows == 0 && a.falls == 1 && g.stows == 0 && g.falls == 1,
               "the old egg: %d of 7 drawn; swing 0: put away %d, fell %d; stowgrace 0 0: put away %d, fell %d", drawnOld, a.stows, a.falls, g.stows, g.falls);
    }
    {
        // TN6: the neck's place where it holds a launcher (the AT4 behind the neck with the Mk11 at the left shoulder: the default "launcher shoulder")
        // is unchanged, and the shoulders don't take its middle; the shoulders never meet (their put-away eggs keep 3 cm off the middle line, the
        // draw's 6 cm, a let-go's as much without the head's cut); the head's egg never a shoulder's for an empty hand's squeeze (a gun let go of there
        // is put away: TN12); none of the chests' or the belly's points (their eggs grown 2 cm, the belly's 5 more) in any place, even grown for the
        // swing.
        const Settings d;
        const float neckMid[3] = {d.neck.mid[0], d.neck.mid[1], d.neck.mid[2]};
        int middle = 0, chest = 0, headShoulder = 0;
        for (int y = -30; y <= 20; ++y)
            for (int z = -30; z <= 30; ++z)
                for (int x = -2; x <= 2; ++x) {
                    const float p[3] = {x / 100.0f, y / 100.0f, z / 100.0f};
                    middle += IsShoulder(PlaceAt(p, d, nullptr, d.stowMore)) + IsShoulder(PlaceAt(p, d, nullptr, d.stowMore, false));
                }
        for (int x = -40; x <= 40; ++x)
            for (int y = -65; y <= 25; ++y)
                for (int z = -35; z <= 45; ++z) {
                    const float p[3] = {x / 100.0f, y / 100.0f, z / 100.0f};
                    const Place q = PlaceAt(p, d, nullptr, d.stowMore + d.swingReach, false);
                    chest += q != Place::kNone && KeptOut(p, d);
                    headShoulder += IsShoulder(PlaceAt(p, d)) && InHead(p, d);
                }
        Report("TN6", "the neck's middle is the neck's; no shoulder's put-away within 3 cm of the middle line (with the head's cut or without); no "
                      "shoulder in the head's egg for a squeeze; no place (grown for the swing) in the chests' or the belly's",
               PlaceAt(neckMid, d) == Place::kNeck && middle == 0 && chest == 0 && headShoulder == 0, "the neck's middle: %s; middle %d, chests and belly %d, head %d",
               Name(PlaceAt(neckMid, d)), middle, chest, headShoulder);
    }
    {
        // TN7: the new lines' words: a put-away by the grace and by the swing (a put-away at its place says what it always said), a let-go that fell
        // by a place (where the hand was, how fast, thrown or not).
        struct Want {
            Event e;
            const char* text;
        };
        Want want[4];
        want[0].e.what = What::kStowed, want[0].e.side = 1, want[0].e.slot = 0, want[0].e.place = Place::kRightShoulder, want[0].e.how = StowHow::kGrace;
        want[0].e.out = 0.06f, want[0].e.since = 0.2f, want[0].e.moving = 0.4f;
        want[0].text = "Holster: the M416 put away at the right shoulder by the right hand (let go 6 cm out of its place 0.20 s after it was in it, at 0.4 m/s)";
        want[1].e.what = What::kStowed, want[1].e.side = 0, want[1].e.slot = 0, want[1].e.place = Place::kRightShoulder, want[1].e.how = StowHow::kSwing;
        want[1].e.out = 0.08f, want[1].e.moving = 1.5f;
        want[1].text = "Holster: the M416 put away at the right shoulder by the left hand (swung in: let go 8 cm short of its place at 1.5 m/s)";
        want[2].e.what = What::kFell, want[2].e.side = 1, want[2].e.slot = 0, want[2].e.place = Place::kRightShoulder, want[2].e.out = 0.04f;
        want[2].e.from[0] = 0.40f, want[2].e.from[1] = -0.01f, want[2].e.from[2] = -0.02f, want[2].e.moving = 2.5f, want[2].e.awayFrom = 2.5f;
        want[2].e.thrown = true;
        want[2].text = "Holster: the M416 let go 4 cm out of the right shoulder's place by the right hand (40 cm right, -1 cm up, -2 cm forward of the neck "
                       "point; 2.5 m/s, 2.5 away from it): thrown, it falls";
        want[3].e.what = What::kFell, want[3].e.side = 1, want[3].e.slot = 5, want[3].e.place = Place::kNeck, want[3].e.out = 0.0f;
        want[3].e.from[0] = 0.0f, want[3].e.from[1] = 0.07f, want[3].e.from[2] = -0.15f;
        want[3].text = "Holster: the SMAW let go behind the neck by the right hand (0 cm right, 7 cm up, -15 cm forward of the neck point; 0.0 m/s, 0.0 away "
                       "from it): it falls";
        int bad = 0;
        char first[400] = "";
        for (const Want& w : want) {
            char line[400];
            Say(w.e, kNames, line, sizeof(line));
            if (std::strcmp(line, w.text) != 0) {
                if (!bad)
                    std::snprintf(first, sizeof(first), ": \"%s\"", line);
                ++bad;
            }
        }
        // A put-away at its place says what it always did (the films' lines end there).
        Event in;
        in.what = What::kStowed, in.side = 1, in.slot = 0, in.place = Place::kRightShoulder, in.moving = 3.0f;
        char line[200];
        Say(in, kNames, line, sizeof(line));
        Report("TN7", "the lines: put away by the grace and by the swing, said so; at its place, as always; a let-go that fell by a place, with where",
               bad == 0 && std::strcmp(line, "Holster: the M416 put away at the right shoulder by the right hand") == 0, "%d wrong%s; at its place: \"%s\"", bad,
               first, line);
    }
    {
        // TN8: a fired AT4 is thrown away wherever it's let go of, never put away (the user, 2026-11-18: "once it's shot, it's MEANT to be thrown away
        // and discarded"; the README: "One shot; then open your hand to throw it away"). With its sights at the right eye its firing hand sits on the
        // right shoulder (skin.cpp's AT4 row: the right palm 15 cm right of the zoomed eye, 11 under it, 8 behind it; the eye 3 cm right, 7.5 up and 8
        // in front of the neck point): about 18 cm right, 3 down, 0 forward, inside the shoulder's place (the old egg's too), where a let-go is never a
        // throw since 2026-11-19. The roof's kit: the AT4 in slot 0 in the right hand, the Mk11 at the left shoulder (the launcher's place behind the
        // neck). Let go (a) there with its sights at the eye, at rest; (b) out to the side 6 cm past its put-away egg a moment after it was there (the
        // grace's); (c) swung up toward it from 12 cm under its put-away egg at 1.5 m/s (the swing's): fired, it falls each time, nothing kept in the
        // hand; with its rocket still in it (the control) the same three put it away (behind the neck). And (d) no tick for it behind the neck, its
        // own place, where the control's comes.
        const float aimed[3] = {0.18f, -0.03f, 0.0f};
        const Egg e = Settings{}.shoulder;
        const float more = Settings{}.stowMore;
        const auto at4 = [](bool spent) {
            Sim s;
            s.set = Settings{};
            Swordbreaker(s);
            Gave(s, 0, 0xA7F0, kAt4Kind, true, true);
            CopyName(s.slots[0].name, "AT4");
            s.slots[1].known = true;
            Named(s.slots[1], 0x1110, kMk11Kind, "Mk11");
            s.gameSlot = 0;
            s.holder = Holder::kHeld, s.trigger = 1, s.armed[1] = true;
            s.hand[1].squeeze = 1.0f;
            s.MoveTo(0, -kFront[0], kFront[1], kFront[2]);
            s.MoveTo(1, 0.15f, -0.35f, 0.22f);  // (the low ready)
            s.Run(0.3f);
            s.spentTube = spent;
            return s;
        };
        int put[2][3] = {}, fell[2][3] = {}, ticks[2] = {}, kept[2] = {};
        Place went = Place::kNone;
        for (int spent = 0; spent < 2; ++spent) {
            Sim a = at4(spent != 0);
            a.hand[1].aims = true;
            a.Move(1, aimed);
            a.Run(0.3f);
            LetGo(a, 1);
            a.Run(0.2f);
            put[spent][0] = a.stows, fell[spent][0] = a.falls;
            ticks[spent] += a.Pulses(1, kTickAmp);
            kept[spent] += a.Count(What::kStowRefused, 1);
            if (!spent)
                went = Hung(a, 0);
            Sim b = at4(spent != 0);
            b.Move(1, aimed);
            b.Run(0.3f);
            b.MoveTo(1, e.mid[0] + e.half[0] + more + 0.06f, e.mid[1], e.mid[2], 0.3f, 0.0f, 0.0f);
            b.Step();
            LetGo(b, 1);
            b.Run(0.2f);
            put[spent][1] = b.stows, fell[spent][1] = b.falls;
            ticks[spent] += b.Pulses(1, kTickAmp);
            kept[spent] += b.Count(What::kStowRefused, 1);
            Sim c = at4(spent != 0);
            c.MoveTo(1, e.mid[0], e.mid[1] - e.half[1] - more - 0.12f, e.mid[2], 0.0f, 1.5f, 0.0f);
            c.Run(0.1f);
            LetGo(c, 1);
            c.Run(0.2f);
            put[spent][2] = c.stows, fell[spent][2] = c.falls;
            ticks[spent] += c.Pulses(1, kTickAmp);
            kept[spent] += c.Count(What::kStowRefused, 1);
            // (d) taken behind the neck, its own place (a tick there says it can be put away; at the shoulder it goes there all the same, unticked).
            Sim n = at4(spent != 0);
            n.Move(1, kNeckAt);
            n.Run(0.3f);
            ticks[spent] += n.Pulses(1, kTickAmp);
        }
        const Settings d;
        Settings oldSet;
        oldSet.shoulder = old;
        const bool inside = PlaceAt(aimed, d, nullptr, d.stowMore) == Place::kRightShoulder && PlaceAt(aimed, oldSet, nullptr, oldSet.stowMore) == Place::kRightShoulder;
        const bool control = put[0][0] == 1 && put[0][1] == 1 && put[0][2] == 1 && fell[0][0] + fell[0][1] + fell[0][2] == 0 && ticks[0] >= 1;
        const bool thrown = fell[1][0] == 1 && fell[1][1] == 1 && fell[1][2] == 1 && put[1][0] + put[1][1] + put[1][2] == 0 && ticks[1] == 0 && kept[1] == 0;
        Report("TN8", "a fired AT4 let go of at the right shoulder with its sights at the eye (its firing hand's place, in the shoulder's place, the old one's "
                      "too), just out of it, or swung in toward it: thrown, it falls, nothing kept; no tick behind the neck; with its rocket in it: put away "
                      "each time, ticked behind the neck",
               inside && control && thrown,
               "the aimed hand in the place %d; fired: fell %d %d %d, put away %d %d %d, ticks %d, kept %d; not fired: put away %d %d %d (at %s), ticks %d",
               inside, fell[1][0], fell[1][1], fell[1][2], put[1][0], put[1][1], put[1][2], ticks[1], kept[1], put[0][0], put[0][1], put[0][2], Name(went),
               ticks[0]);
    }
    // ---- TN9-TN14: the two reviews of c6dabab, settled ----
    const float top[3] = {0.21f, -0.02f, -0.02f};  // the right shoulder's top, as the gun's grip ends a swing there
    {
        // TN9 (the first review's #1): both hands on the gun swung to the right shoulder, the pistol grip let go of first (the gun onto the left's hold
        // on its front), the left opening 0.06-0.5 s later wherever it ended (before the face, across the chest, by the left shoulder where the Mk11
        // hangs, above the head): put away at the right shoulder by the left hand each time, said as the handoff (it fell from 0.06 s on, or was kept
        // at the Mk11's shoulder). Not: the left still holding past the grace (0.6 s), then opening in front; the grip let go of in front, at no
        // place; the left throwing it away from the shoulder at 3 m/s.
        const float fronts[4][3] = {{0.09f, 0.16f, 0.17f}, {-0.12f, -0.20f, 0.22f}, {-0.20f, -0.02f, 0.12f}, {0.16f, 0.32f, 0.02f}};
        const float lags[4] = {0.06f, 0.10f, 0.25f, 0.50f};
        const auto both = [&](const float grip[3], const float front[3]) {
            Sim s = tonight::KitHeld(0, 1);
            s.hand[0].onGun = true, s.hand[0].squeeze = 1.0f;
            s.Move(0, front);
            s.Move(1, grip);
            s.Run(0.3f);
            s.hand[1].squeeze = 0.0f;  // the grip let go of: onto the left's front
            s.Run(0.1f);
            return s;
        };
        int tries = 0, put = 0, said = 0, kept = 0;
        char first[200] = "";
        for (const auto& fr : fronts)
            for (const float lag : lags) {
                Sim s = both(top, fr);
                const bool onFront = s.holder == Holder::kBarrel && s.barrel == 0;
                s.Run(std::max(0.0f, lag - 0.1f));
                s.hand[0].onGun = false, s.hand[0].squeeze = 0.0f;  // the left opens where it is
                s.Run(0.2f);
                ++tries;
                const Event* e = s.Find(What::kStowed, 0);
                const bool ok = onFront && s.stows == 1 && s.falls == 0 && e && e->place == Place::kRightShoulder && e->how == StowHow::kHandoff &&
                                s.st.home[0].role == Role::kPrimary;
                put += ok;
                kept += s.Count(What::kStowRefused);
                if (ok && !said) {
                    char line[256] = "";
                    Say(*e, kNames, line, sizeof(line));
                    said = std::strstr(line, "put away at the right shoulder by the left hand (the right hand let go of its grip there") != nullptr;
                }
                if (!ok && !first[0])
                    std::snprintf(first, sizeof(first), "; the first not: the left at (%.2f %.2f %.2f) %.2f s on: put away %d, fell %d", fr[0], fr[1], fr[2],
                                  lag, s.stows, s.falls);
            }
        // The controls.
        Sim late = both(top, fronts[0]);
        late.Run(0.8f);
        late.hand[0].onGun = false, late.hand[0].squeeze = 0.0f;
        late.Run(0.2f);
        const float inFront[3] = {0.18f, -0.30f, 0.18f};
        Sim away = both(inFront, fronts[0]);
        away.hand[0].onGun = false, away.hand[0].squeeze = 0.0f;
        away.Run(0.2f);
        Sim flung = both(top, fronts[0]);
        flung.MoveTo(0, fronts[0][0], fronts[0][1], fronts[0][2], -0.5f, 0.5f, 3.0f);  // (forward and away from the shoulder at 3 m/s)
        flung.hand[0].onGun = false, flung.hand[0].squeeze = 0.0f;
        flung.Run(0.2f);
        Report("TN9", "both hands on the gun at the right shoulder, the pistol grip let go of first: the left's let-go 0.06-0.5 s on, before the face, across "
                      "the chest, by the left shoulder or above the head, puts it away at the right shoulder (said); not past the grace, nor with the grip "
                      "let go of at no place, nor thrown away from there",
               put == tries && said && kept == 0 && late.stows == 0 && late.falls == 1 && away.stows == 0 && away.falls == 1 && flung.stows == 0 &&
                   flung.falls == 1,
               "%d of %d put away (line %d, refusals %d); past the grace: put away %d, fell %d; the grip let go of in front: put away %d, fell %d; thrown: "
               "put away %d, fell %d%s", put, tries, said, kept, late.stows, late.falls, away.stows, away.falls, flung.stows, flung.falls, first);
    }
    {
        // TN10 (the second review's #6): Rock (the SMAW at the left shoulder), the M416 held by the left hand's handguard alone (the grip let go of at
        // the low ready, at no place), let go in front of the left shoulder: put away at its own, the right, still the primary, the SMAW where it was
        // (it went onto the left shoulder as the secondary, the SMAW behind the neck; until 2026-11-19 it fell there). Only the hand on its grip
        // moves it: put on the left shoulder by the right hand, it's the secondary and the SMAW goes behind the neck, as ever.
        const float lefts[2][3] = {{-0.15f, 0.0f, 0.14f}, {-0.18f, -0.05f, 0.10f}};
        int ownShoulder = 0;
        Place smaw[2] = {};
        for (int k = 0; k < 2; ++k) {
            Sim s = HeldRock();
            s.set = Settings{};
            s.MoveTo(1, 0.15f, -0.35f, 0.22f);
            s.Run(0.5f);
            s.hand[0].onGun = true, s.hand[0].squeeze = 1.0f;
            s.Move(0, lefts[k]);
            s.MoveTo(1, 0.18f, -0.30f, 0.18f);
            s.Run(0.3f);
            s.hand[1].squeeze = 0.0f;
            s.Run(0.3f);
            const bool front = s.holder == Holder::kBarrel && s.barrel == 0;
            s.hand[0].onGun = false, s.hand[0].squeeze = 0.0f;
            s.Run(0.3f);
            const Event* e = s.Find(What::kStowed, 0);
            ownShoulder += front && s.stows == 1 && e && e->place == Place::kRightShoulder && s.st.home[0].role == Role::kPrimary;
            smaw[k] = PlaceNow(s, 5);
        }
        Sim g = HeldRock();
        g.set = Settings{};
        g.MoveTo(1, 0.15f, -0.35f, 0.22f);
        g.Run(0.5f);
        PutOn(g, 1, kLS);
        Report("TN10", "the M416 by the left hand's handguard alone, let go in front of the left shoulder (Rock): put away at its own, the right, still the "
                       "primary, the SMAW still at the left; put on the left shoulder by the hand on its grip: the secondary, the SMAW behind the neck",
               ownShoulder == 2 && smaw[0] == Place::kLeftShoulder && smaw[1] == Place::kLeftShoulder && g.stows == 1 &&
                   g.st.home[0].role == Role::kSecondary && PlaceNow(g, 5) == Place::kNeck,
               "its own shoulder %d of 2 (the SMAW at %s, %s); by the grip at the left: %s, the SMAW %s", ownShoulder, Name(smaw[0]), Name(smaw[1]),
               RoleName(g.st.home[0].role), Name(PlaceNow(g, 5)));
    }
    {
        // TN11 (the second review's #1): a toss across the chest to the left at 2.5 m/s is no swing's end: it falls, on Rock (it went onto the left
        // shoulder, the SMAW behind the neck) and on Swordbreaker (it was kept in the hand at the Mk11's shoulder, a double click); a swing at the
        // throw's speed or more ends short of a place as a toss (falls); and a swing never moves a gun onto the other shoulder than its own: the M416
        // swung at 1.5 m/s toward the left shoulder, let go 12 cm short of it, falls (toward its own it's put away: TN3b).
        int tossFell = 0, tossPut = 0, tossKept = 0;
        for (int kit = 0; kit < 2; ++kit) {
            Sim s = kit == 0 ? HeldRock() : tonight::KitHeld(0, 1);
            if (kit == 0) {
                s.set = Settings{};
                s.MoveTo(1, 0.15f, -0.35f, 0.22f);
                s.Run(0.5f);
            }
            s.MoveTo(1, 0.0f, -0.08f, 0.20f, -2.5f, 0.3f, -0.5f);
            s.Step();
            LetGo(s, 1);
            s.Run(0.3f);
            tossFell += s.falls, tossPut += s.stows, tossKept += s.Count(What::kStowRefused);
        }
        const Egg e = Settings{}.shoulder;
        const float more = Settings{}.stowMore;
        const float under[3] = {e.mid[0], e.mid[1] - e.half[1] - more - 0.08f, e.mid[2]};
        Sim fast = tonight::KitHeld(0, 1);
        fast.Move(1, under, 0.0f, 2.5f, 0.0f);  // (8 cm under the right shoulder's put-away egg, going up into it at 2.5 m/s)
        fast.Run(0.1f);
        LetGo(fast, 1);
        Sim other = HeldRock();
        other.set = Settings{};
        other.MoveTo(1, 0.15f, -0.35f, 0.22f);
        other.Run(0.5f);
        const float shortLeft[3] = {-e.mid[0], e.mid[1] - e.half[1] - more - 0.12f, e.mid[2]};
        other.Move(1, shortLeft, 0.0f, 1.5f, 0.0f);  // (12 cm under the left shoulder's put-away egg, going up into it at 1.5 m/s)
        other.Run(0.1f);
        LetGo(other, 1);
        Report("TN11", "a toss across the chest to the left at 2.5 m/s (Rock, Swordbreaker): falls, nothing put away or kept; going into the right shoulder "
                       "at 2.5 m/s let go 8 cm short: a toss, falls; the M416 swung at 1.5 m/s toward the left shoulder, 12 cm short: falls (no swing "
                       "onto the other shoulder)",
               tossFell == 2 && tossPut == 0 && tossKept == 0 && fast.stows == 0 && fast.falls == 1 && other.stows == 0 && other.falls == 1 &&
                   PlaceNow(other, 5) == Place::kLeftShoulder,
               "tossed: fell %d, put away %d, refusals %d; fast short of the right: put away %d, fell %d; toward the left: put away %d, fell %d (the SMAW at %s)",
               tossFell, tossPut, tossKept, fast.stows, fast.falls, other.stows, other.falls, Name(PlaceNow(other, 5)));
    }
    {
        // TN12 (both reviews, #3): the head's egg is cut out of the shoulders for an empty hand's squeeze and its tick only: the M4A1 swung up from the
        // low ready to beside the head (12 cm out 6 up; 13 out 10 up 5 behind; 11 out 12 up 8 behind: in the head's egg) and let go there arriving, or
        // held there a second and let go: put away at the right shoulder (it fell); an empty hand's fresh squeeze at each, both hands empty: nothing.
        const float spots[3][3] = {{0.12f, 0.06f, 0.0f}, {0.13f, 0.10f, -0.05f}, {0.11f, 0.12f, -0.08f}};
        int swung = 0, rested = 0, inHead = 0, drawn = 0;
        const Settings d;
        for (const auto& p : spots) {
            inHead += InHead(p, d);
            Sim a = tonight::KitHeld(0, 1);
            tonight::Swing(a, 1, p, 0.3f, 1.0f);
            swung += a.stows == 1 && a.falls == 0 && Hung(a, 0) == Place::kRightShoulder;
            Sim b = tonight::KitHeld(0, 1);
            b.Move(1, p);
            b.Run(1.0f);
            LetGo(b, 1);
            b.Run(0.1f);
            rested += b.stows == 1 && b.falls == 0;
            Sim c = tonight::Kit(0);
            SqueezeAt(c, 1, p, 0.4f);
            drawn += c.draws + c.Keys();
        }
        Report("TN12", "the M4A1 let go beside the head (in the head's egg), swung there or held there: put away at the right shoulder; an empty hand's "
                       "squeeze at the same spots: nothing drawn",
               inHead == 3 && swung == 3 && rested == 3 && drawn == 0, "in the head's egg %d of 3; swung: put away %d of 3; held there: %d of 3; drawn %d",
               inHead, swung, rested, drawn);
    }
    {
        // TN13 (the second review's #5): a hand not seen is presumed at a shoulder only within its egg until 2026-11-19 too (kUnseenShoulder), so a hand
        // lost round the big left shoulder (the launchers' by default) isn't: Swordbreaker, both guns at their shoulders, the left hand lost beside
        // the left temple or in front of the left shoulder, then squeezed: nothing (it pressed key 2 for the Mk11); lost at the old egg's place (17
        // out, 3 up): key 2, as ever. The rifle held by the left hand's handguard alone, lost in front of the left shoulder 1 s: drawn all the while
        // (it was drawn nowhere the whole loss); lost at the old egg's middle: drawn nowhere meanwhile, as ever.
        const float spots[3][3] = {{-0.16f, 0.06f, 0.06f}, {-0.25f, -0.05f, 0.12f}, {-0.17f, 0.03f, 0.0f}};
        int keys[3] = {};
        for (int k = 0; k < 3; ++k) {
            Sim s = tonight::Kit(0);
            s.Move(0, spots[k]);
            s.Run(0.3f);
            s.hand[0].seen = Seen::kNone;
            s.Run(0.2f);
            s.hand[0].squeeze = 1.0f;
            s.Run(0.4f);
            keys[k] = s.Keys(1);
        }
        const float lost[2][3] = {{-0.20f, -0.05f, 0.10f}, {kLS[0], kLS[1], kLS[2]}};
        int hidden[2] = {};
        for (int k = 0; k < 2; ++k) {
            Sim s = tonight::KitHeld(0, 1);
            s.hand[0].onGun = true, s.hand[0].squeeze = 1.0f;
            s.Move(0, lost[k]);
            s.Run(0.3f);
            s.hand[1].squeeze = 0.0f;
            s.Run(0.3f);
            s.hand[0].seen = Seen::kNone;
            const int before = s.unseenAtPlaceFrames;
            s.Run(1.0f);
            hidden[k] = s.unseenAtPlaceFrames - before;
        }
        Report("TN13", "a hand not seen round the big left shoulder isn't presumed there: lost beside the left temple or in front of the shoulder and "
                       "squeezed: no key; at the old egg's place: key 2; the rifle by its handguard lost in front of the shoulder: drawn all the while; at "
                       "the old egg's middle: drawn nowhere meanwhile",
               keys[0] == 0 && keys[1] == 0 && keys[2] >= 1 && hidden[0] == 0 && hidden[1] > 60, "keys %d %d, at the old egg %d; drawn nowhere %d frames "
                                                                                                "(in front), %d (the old middle)", keys[0], keys[1], keys[2],
               hidden[0], hidden[1]);
    }
    {
        // TN14 (the second review's #2): the fired AT4 stays spent while it's drawn (TubeSpent, as hands.cpp keeps hf.spentTube): its count read none,
        // then unread (the kit gave the M4A1 back into its slot: no slot holds an AT4), still spent; no longer drawn: forgotten; a fresh one read with
        // its rocket: not spent; a fresh one drawn before its count is read: not spent. And holsters with it: the roof's give-back window (the M4A1 in
        // slot 0, the spent tube still drawn: drawnOther) let go at the right shoulder at rest, thrown out at 2.5 m/s or moving down: falls each time,
        // nothing kept in the open hand (it was kept, as a change of gun under way, with At4Spent false).
        TubeLatch t;
        const bool r0 = TubeSpent(t, true, true, 1);
        const bool r1 = TubeSpent(t, true, true, 0);
        const bool r2 = TubeSpent(t, true, false, 0);
        const bool r3 = TubeSpent(t, false, false, 0);
        const bool r4 = TubeSpent(t, true, false, 0);
        const bool r5 = TubeSpent(t, true, true, 1);
        const bool latch = !r0 && r1 && r2 && !r3 && !r4 && !r5;
        const float aimed[3] = {0.18f, -0.03f, 0.0f};
        const float vels[3][3] = {{0.0f, 0.0f, 0.0f}, {2.5f, 0.5f, 0.0f}, {0.0f, -1.0f, 0.3f}};
        int fell[2] = {}, keptIn[2] = {};
        for (int old = 0; old < 2; ++old)
            for (const auto& v : vels) {
                Sim s;
                s.set = Settings{};
                Swordbreaker(s);
                Gave(s, 0, 0xA7F0, kAt4Kind, true, true);
                CopyName(s.slots[0].name, "AT4");
                Named(s.slots[1], 0x1110, kMk11Kind, "Mk11");
                s.gameSlot = 0;
                s.holder = Holder::kHeld, s.trigger = 1, s.armed[1] = true;
                s.hand[1].squeeze = 1.0f;
                s.MoveTo(0, -kFront[0], kFront[1], kFront[2]);
                s.MoveTo(1, 0.15f, -0.35f, 0.22f);
                s.Run(0.3f);
                TubeLatch tube;
                TubeSpent(tube, true, true, 1);
                s.spentTube = TubeSpent(tube, true, true, 0);  // (fired: its count none)
                s.hand[1].aims = true;
                s.Move(1, aimed);
                s.Run(1.5f);
                Gave(s, 0, 0x4A2F, kM4Kind);  // (the kit's give-back: the M4A1 into slot 0, no take; the spent tube still drawn)
                CopyName(s.slots[0].name, "M4A1");
                s.spentTube = old ? false : TubeSpent(tube, true, false, 0);  // (old: hands.cpp's At4Spent before the fix, the AT4 in no slot)
                s.drawnOther = true;
                s.Run(0.2f);
                s.MoveTo(1, aimed[0], aimed[1], aimed[2], v[0], v[1], v[2]);
                LetGo(s, 1);
                s.Run(0.2f);
                fell[old] += s.falls;
                keptIn[old] += s.holder == Holder::kHeld;
            }
        Report("TN14", "the fired AT4 stays spent while drawn (TubeSpent: read none, then unread, then not drawn, a fresh one); let go at the right "
                       "shoulder in the kit's give-back window (at rest, thrown, moving down): falls each time (with At4Spent false, as before: kept in "
                       "the hand)",
               latch && fell[0] == 3 && keptIn[0] == 0 && keptIn[1] == 3, "the latch %d (%d %d %d %d %d %d); spent: fell %d of 3, kept %d; as before: kept %d "
                                                                          "of 3", latch, r0, r1, r2, r3, r4, r5, fell[0], keptIn[0], keptIn[1]);
    }
}

// ---- HT: the AT4 branch merged with holsters by hand (2026-11-19): the owner's two AT4 flows end to end with both together. The AT4 branch's use of
// the roof's prompt by reach and grip (usepoint.h) run on holsters' own frames, its gift as hands.cpp's OnGunChanged gives it (the slot marked a
// take's, the take's Arrive, the grasp's arrival into the hand that used it), the scene's kit and its give-back, holsters' Arrive and Update, and
// hands.cpp's flow (b) record (pickrules::LyingTaken, LyingBack); hands.cpp's own lines for each word for word (no test compiles hands.cpp). ----
namespace merge {

// The roof's prompt (its point on the floor, 1.15 m under the neck point, a little right and ahead: the AT4 lies about 0.65 m off it) and a hand
// lowered to the tube, from the neck point in the body's axes.
constexpr float kPromptAt[3] = {0.25f, -1.15f, 0.60f};
constexpr float kAtTube[3] = {0.30f, -1.05f, 0.55f};
constexpr uint32_t kAt4Id = 0xA7F0, kM4Back = 0x4A2F;

struct UseRun {
    int use = -1;     // the hand whose use was asked
    int ticks = 0;    // that hand's ticks entering the reach
    int yields = 0;   // frames a hand's squeeze was holsters' (UseYields' first rule)
    int refused = 0;  // that hand's refusals (a gun in a hand: usepoint's Out::refused)
    int doubles = 0;  // that hand's pulses of the double click's (two a refusal)
};
// The AT4 branch's use of the prompt (usepoint.h) with holsters' frames together, as FreeHands runs them: after holster::Update each frame (the Sim's
// Step), UseFrame's frame: the prompt's point and its own disc (the AT4's 1.0 m), a prompt that gives a gun (the reload key's), a gun in a hand
// (GunInAHand as the Sim has it: by its grip or its front, flying in, a take or a draw waiting), each hand's place and squeeze, empty (UseFrame's: no
// gun by its grip or front in it, nothing carried), yield (UseYields' first rule: the hand at one of holsters' places, busy with a draw that waits,
// drawing or putting away this frame). That hand goes to at, squeezes once there and holds (till its use, else the seconds out).
UseRun UseByHand(Sim& s, usepoint::State& us, int side, const float at[3], float seconds) {
    UseRun out;
    s.hand[side].squeeze = 0.0f;
    s.Move(side, at);
    const int frames = static_cast<int>(std::lround(seconds / kDt));
    for (int k = 0; k < frames && out.use < 0; ++k) {
        if (k == 12)
            s.hand[side].squeeze = 1.0f;
        s.Step();
        usepoint::Frame uf;
        uf.dt = kDt;
        uf.point.on = true;
        s.ToRoom(kPromptAt, uf.point.at);
        uf.point.disc = 1.0f;
        uf.point.gives = true;
        uf.gunHeld = s.holder == Holder::kHeld || s.holder == Holder::kBarrel || s.holder == Holder::kGlide || s.pendingTake || s.st.wait.on;
        for (int i = 0; i < 2; ++i) {
            usepoint::Hand& u = uf.hand[i];
            u.tracked = s.hand[i].seen == Seen::kSeen && s.hand[i].own && s.hand[i].read;
            s.ToRoom(s.hand[i].pos, u.at);
            u.squeeze = s.hand[i].squeeze;
            const bool holdsGun = ((s.holder == Holder::kHeld || s.holder == Holder::kGlide) && s.trigger == i) || (s.holder == Holder::kBarrel && s.barrel == i);
            u.empty = !holdsGun && !s.hand[i].busy && !s.hand[i].onGun;
            u.yield = s.r.place[i] != Place::kNone || s.r.busy[i] || s.r.drawSide == i || s.r.stowSide == i;
            out.yields += u.yield;
        }
        usepoint::Out uo;
        usepoint::Update(uf, us, uo);
        out.ticks += uo.entered[side];
        out.refused += uo.refused[side];
        out.doubles += uo.pulse[side].seconds > 0.0f && uo.pulse[side].amplitude == usepoint::kDoubleAmp;
        if (uo.use >= 0)
            out.use = uo.use;
    }
    return out;
}
// The game's use served, then the scene's kit (PlayerAT4): the AT4 into slot 0 for the M4A1 (a new object, 1 + 4), the Mk11 kept; its counts and its
// slot 0.1 s after the press, the gun before still drawn till 0.4 s and nothing drawn till 0.7 s (at4_measured.md section 3): lag seconds of frames with
// the slot's new gun and no mark (no pickup's swap: nothing marked by NoteSwaps), hands.cpp's change of gun under way (Frame::drawnOther) the while;
// then its first draw (OnGunChanged). By a hand's use (hands.cpp's gift block within kUseGiftMs): the slot marked a take's, the take's Arrive, the AT4
// into the hand that used it (the grasp's arrival). By X held's R (no use recorded): no mark, the game's own switch's Arrive.
constexpr float kTakeLag = 0.6f;
constexpr float kFireWait = 14.0f;  // s from the AT4's draw to its shot: the scene's fire trigger opens 13.5 s after the take (section 5)
ArriveOut Kit(Sim& s, World& w, int side, bool byUse, float lag = kTakeLag, int* drawsMeanwhile = nullptr) {
    Gave(s, 0, kAt4Id, kAt4Kind, true, true);
    CopyName(s.slots[0].name, "AT4");
    w.loaded[0] = 1, w.spare[0] = 4;
    const int draws = s.draws;
    s.drawnOther = true;
    s.Run(lag);
    s.drawnOther = false;
    if (drawsMeanwhile)
        *drawsMeanwhile = s.draws - draws;
    if (byUse)
        s.taken |= 1u;  // (the gift's mark, with its Arrive: Took's)
    const ArriveOut o = s.Switch(0, byUse);
    if (byUse) {
        s.holder = Holder::kHeld, s.trigger = side, s.barrel = -1;
        s.armed[side] = s.hand[side].squeeze >= kTake;  // (grasp.h's Frame::arriveUnarmed: armed only by a grip squeezed then)
    }
    s.Run(kFireWait);  // (in the hand till the scene's fire trigger opens: fired then)
    return o;
}
// The level's kit again after the shot (HotelDestruction, 1.3-3.3 s on): a fresh M4A1 into slot 0 with the kit's counts (31 + 372), no take; noted
// with the slots (holsters' Update, every frame) lag seconds before its first draw changes the gun in hand (at4_measured.md section 8: 0.92 s and 1.28 s
// after the counts, the spent AT4 drawn, then nothing, meanwhile), a change of gun under way the while. The frames between are run here (meanwhile, if
// given, after half of them: the spent AT4 thrown then); its draw (OnGunChanged's Arrive) is the caller's.
constexpr float kDrawLag = 1.3f;
void KitBack(Sim& s, World& w, float lag = kDrawLag, void (*meanwhile)(Sim&) = nullptr) {
    Gave(s, 0, kM4Back, kM4Kind);
    CopyName(s.slots[0].name, "M4A1");
    w.loaded[0] = 31, w.spare[0] = 372;
    s.drawnOther = true;
    s.Run(lag / 2);
    if (meanwhile)
        meanwhile(s);
    s.Run(lag / 2);
    s.drawnOther = false;
}
// hands.cpp's flow (b) record at the gift (OnGunChanged: LyingTaken's inputs as it gives them; the profiles' ids are the kinds here).
pickrules::LyingTakenIn TakenIn(const Sim& s, bool gift) {
    pickrules::LyingTakenIn in;
    in.gift = gift;
    in.lies = s.holder == Holder::kLoose;
    in.lyingId = kM4Kind, in.newId = kAt4Kind;
    in.lyingSlot = 0, in.newSlot = 0;
    return in;
}
// ... and at the kit's give-back (LyingBack's), the record taken at `at` (ms), now `now`, where it lay `distance` m from the eye.
pickrules::LyingBackIn BackIn(unsigned long long at, unsigned long long now, float distance) {
    pickrules::LyingBackIn in;
    in.on = true, in.id = kM4Kind, in.slot = 0, in.at = at;
    in.newId = kM4Kind, in.newSlot = 0;
    in.now = now, in.distance = distance;
    return in;
}
// The left hand lets go of what it holds where it is (the spent AT4 thrown).
void ThrowLeft(Sim& s) { LetGo(s, 0); }
// The draw of the gun at that place by that hand: case A (the game's weapon put away: in the hand at once, no key).
bool DrawnCaseA(Sim& s, int side, const float at[3]) {
    const int keys = s.Keys();
    SqueezeAt(s, side, at);
    return s.holder == Holder::kHeld && s.trigger == side && s.Keys() == keys;
}

void Rows() {
    // HT1 flow (a): the M4A1 put on the right shoulder by hand; the left hand reaches to the tube and holds the grip (the use, by the AT4 branch's
    // reach and grip, the holsters' frame each frame: no holster place under the hand); the kit's AT4 in slot 0 0.6 s before its draw (nothing drawn
    // into the right hand meanwhile: no put-away gun with no home), then into the left hand, on no shoulder; the M4A1 with the game at its shoulder
    // (weak); fired; thrown before the kit gives it back, or in the 1.3 s between the kit's counts and its draw; a draw at the right shoulder before:
    // not back yet (felt, no key); the kit's M4A1: at its place; drawn by the right hand at its shoulder (case A, no key), the kit's counts.
    for (int inGap = 0; inGap < 2; ++inGap) {
        World w;
        Sim s = SwordWorld(w);
        s.Run(20.0f);
        PutOn(s, 1, kRS);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.2f);
        const bool holstered = s.holder == Holder::kStowed && s.st.home[0].role == Role::kPrimary;
        usepoint::State us;
        const UseRun u = UseByHand(s, us, 0, kAtTube, 1.5f);
        int meanwhile = -1;
        const size_t takeFrom = s.events.size();
        const ArriveOut take = Kit(s, w, 0, true, kTakeLag, &meanwhile);
        s.Run(0.5f);
        const Away* away = AwayOf(s, kM4Kind);
        const Role at4Home = s.st.home[0].role;
        const bool gift = u.use == 0 && u.ticks == 1 && u.yields == 0 && meanwhile == 0 && second::CountFrom(s, What::kToHand, takeFrom) == 0 &&
                          take.arrival == Arrival::kToday && s.holder == Holder::kHeld && s.trigger == 0 && at4Home == Role::kNone && away && !away->entity &&
                          away->role == Role::kPrimary;
        s.Move(0, kFront);
        s.Run(0.1f);
        bool notYet = true;
        std::string earlyLine = "-";  // (the lines copied as said: s.events grows, and a pointer into it goes stale)
        if (!inGap) {
            LetGo(s, 0);
            s.Run(0.4f);
            const size_t from = s.events.size();
            SqueezeAt(s, 1, kRS);
            s.hand[1].squeeze = 0.0f;
            s.Run(0.2f);
            const Event* early = second::Last(s, What::kRefused, 1, from);
            notYet = early && early->why == Why::kWithGame && s.Keys() == 0 && s.holder == Holder::kLoose;
            earlyLine = LineIn(s, early);
            s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
            s.Run(0.3f);
        }
        const bool thrownFirst = inGap || s.holder == Holder::kLoose;
        KitBack(s, w, kDrawLag, inGap ? ThrowLeft : nullptr);
        const bool thrown = s.holder == Holder::kLoose;
        const ArriveOut back = s.Switch(0);
        s.Run(0.3f);
        const Event* said = second::Last(s, What::kComeBack);
        const bool atPlace = back.arrival == Arrival::kAtPlace && s.holder == Holder::kStowed && s.st.home[0].role == Role::kPrimary && AwayCount(s) == 0 &&
                             said && said->weak && said->place == Place::kRightShoulder;
        const std::string saidLine = LineIn(s, said);
        const bool drawn = DrawnCaseA(s, 1, kRS) && w.loaded[0] == 31 && w.spare[0] == 372;
        Report(inGap ? "HT1b" : "HT1",
               inGap ? "flow (a), the spent AT4 thrown between the kit's counts and its M4A1's draw (a change of gun under way): the M4A1 at its place "
                       "all the same (the thrown tube no home of slot 0's), drawn by hand there"
                     : "flow (a) both together: the M4A1 holstered by hand; the AT4 taken by reach and grip (the left, on holsters' frames: no place under "
                       "it); its kit's 0.6 s before its draw (nothing into the right hand meanwhile); in the left hand, on no shoulder; the M4A1 with the "
                       "game at its shoulder; fired, thrown; drawn early: not back yet (no key); the kit's M4A1 (1.3 s before its draw) at its place; "
                       "drawn by hand there (case A) with the kit's counts",
               holstered && gift && thrownFirst && thrown && notYet && atPlace && drawn,
               "holstered %d; use %d (ticks %d, holsters' squeezes %d), drawn meanwhile %d, arrival %d, the AT4's home %s, M4A1 %s; thrown %d; early \"%s\"; "
               "back %d (arrival %d, \"%s\"); drawn %d (%d + %d)",
               holstered, u.use, u.ticks, u.yields, meanwhile, static_cast<int>(take.arrival), RoleName(at4Home), away ? "with the game" : "none", thrown,
               earlyLine.c_str(), atPlace, static_cast<int>(back.arrival), saidLine.c_str(), drawn, w.loaded[0], w.spare[0]);
    }
    // HT2 flow (a) with the spent AT4 still in the hand as the kit gives the M4A1 back (1.3-3.3 s after the shot: a player who hasn't thrown it yet):
    // the M4A1 at its place all the same, never into a hand by itself (the AT4 goes with the game's switch, the hands empty); drawn by hand. HT2b: a pick
    // of the player's just before (the D-pad): today's, into the right hand. (On main's Arrive alone HT2 failed: kToday, the M4A1 into the hand.)
    {
        World w;
        Sim s = SwordWorld(w);
        s.Run(20.0f);
        PutOn(s, 1, kRS);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.2f);
        usepoint::State us;
        const UseRun u = UseByHand(s, us, 0, kAtTube, 1.5f);
        Kit(s, w, 0, true);
        s.Move(0, kFront);
        s.Run(2.0f);
        const bool held = s.holder == Holder::kHeld && s.trigger == 0;
        KitBack(s, w);
        const ArriveOut back = s.Switch(0);
        s.Run(0.3f);
        const bool atPlace = back.arrival == Arrival::kAtPlace && s.holder == Holder::kStowed && s.st.home[0].role == Role::kPrimary && AwayCount(s) == 0 &&
                             s.falls == 0;
        s.hand[0].squeeze = 0.0f;
        s.Run(0.2f);
        const bool drawn = DrawnCaseA(s, 0, kRS);
        World w2;
        Sim p = SwordWorld(w2);
        p.Run(20.0f);
        PutOn(p, 1, kRS);
        p.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        p.Run(0.2f);
        usepoint::State us2;
        UseByHand(p, us2, 0, kAtTube, 1.5f);
        Kit(p, w2, 0, true);
        p.Run(2.0f);
        p.picked = true, p.pickSlot = 0;
        p.Step();
        KitBack(p, w2);
        const ArriveOut picked = p.Switch(0);
        const bool today = picked.arrival == Arrival::kToday && p.holder == Holder::kHeld && p.st.home[0].role == Role::kPrimary;
        Report("HT2", "flow (a), the spent AT4 still in the hand at the kit's give-back: the M4A1 at its right shoulder all the same (the AT4 gone with the "
                      "game's switch, nothing dropped), drawn by hand (case A); HT2b a pick just before: today's (into the right hand, still its own shoulder's)",
               u.use == 0 && held && atPlace && drawn && today, "use %d; held %d; arrival %d, holder %d, home %s, away %d, falls %d; drawn %d; picked: arrival %d",
               u.use, held, static_cast<int>(back.arrival), static_cast<int>(s.holder), RoleName(s.st.home[0].role), AwayCount(s), s.falls, drawn,
               static_cast<int>(picked.arrival));
    }
    // HT3 the owner's decision (2026-11-19, asked whether the AT4 is taken with the rifle still in the other hand: "Rifle dropped on the floor first",
    // then "Better to physically drop the rifle and pick up the AT"): the M4A1 held in the right hand (the kit's, with its holster; HT3b one with no
    // holster, taken back off the floor), the left's reach and grip at the tube is refused: no tick, no use asked, the double click (two pulses), nothing
    // with the game, the M4A1 still in the right with the home it had, no key. Put on the right shoulder by hand (HT3), or let go of on the floor (HT3b),
    // the left's squeeze there again is the use (the tick on the bar's lifting, the hand still in reach). Never a gun the kit takes from a hand. (The rule
    // before: a gun in the other hand no bar, the kit took the M4A1 from the right.)
    {
        bool ok[2] = {};
        char what[2][260] = {};
        for (int holstered = 1; holstered >= 0; --holstered) {
            World w;
            Sim s = SwordWorld(w);
            s.Run(20.0f);
            if (!holstered) {  // (let go of on the floor and taken back off it: no holster)
                s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
                LetGo(s, 1);
                s.Run(0.5f);
                s.hand[1].squeeze = 1.0f;
                s.takeBy = 1;
                s.Run(0.3f);
            }
            const Role home = holstered ? Role::kPrimary : Role::kNone;
            const bool inRight = s.holder == Holder::kHeld && s.trigger == 1 && s.st.home[0].role == home;
            usepoint::State us;
            const UseRun u = UseByHand(s, us, 0, kAtTube, 1.0f);
            s.hand[0].squeeze = 0.0f;
            s.Run(0.2f);
            const bool refused = u.use < 0 && u.refused == 1 && u.ticks == 0 && u.doubles == 2 && s.holder == Holder::kHeld && s.trigger == 1 &&
                                 AwayCount(s) == 0 && s.st.home[0].role == home && s.Keys() == 0;
            if (holstered) {
                PutOn(s, 1, kRS);
            } else {
                LetGo(s, 1);
                s.Run(0.5f);
            }
            s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
            s.Run(0.2f);
            const bool emptied = holstered ? s.holder == Holder::kStowed && s.st.home[0].role == Role::kPrimary : s.holder == Holder::kLoose;
            const UseRun again = UseByHand(s, us, 0, kAtTube, 1.5f);
            const bool used = again.use == 0 && again.refused == 0 && again.ticks == 1;
            ok[holstered] = inRight && refused && emptied && used;
            std::snprintf(what[holstered], sizeof(what[holstered]), "in the right %d; refused %d (use %d, refusals %d, ticks %d, double pulses %d, away %d, "
                          "home %s, keys %d); %s %d; then the use %d (ticks %d)", inRight, refused, u.use, u.refused, u.ticks, u.doubles, AwayCount(s),
                          RoleName(s.st.home[0].role), s.Keys(), holstered ? "put on the right shoulder" : "let go of on the floor", emptied, again.use,
                          again.ticks);
        }
        Report("HT3", "the M4A1 in the right hand at the tube (with its holster; HT3b one with none): the left's reach and grip refused (no tick, no use, "
                      "the double click, nothing with the game, no key); put on the right shoulder, or let go of on the floor: the same squeeze the use",
               ok[0] && ok[1], "holstered: %s | no holster: %s", what[1], what[0]);
    }
    // HT4 flow (b): the M4A1 let go of on the floor (no holster any more: it lies, the player's own); the right hand takes the AT4 by reach and grip
    // (hands.cpp's record: a scene's gift taking the slot of the gun that lies, LyingTaken); nothing away; the AT4 in the right; fired, thrown; the kit's
    // M4A1 back in slot 0: LyingBack lies it again where it lay (hands.cpp lays it down still: the grasp kLoose, no Arrive, holsters' lying gun read
    // afresh), the thrown AT4 gone with the game's switch; holsters: lying, the M4A1 itself (its identity and kind), on no shoulder, nothing at the right
    // shoulder (a squeeze there draws nothing, no key), the left shoulder's Mk11 not drawn while the player's own lies (refused: it lies on the ground,
    // no key: the reviews of e403947); taken off the floor by hand: in the hand, on no shoulder; put on the right shoulder by hand: its holster again.
    // HT4b: the AT4 still in the hand at the give-back: the same.
    {
        bool ok[2] = {};
        char what[2][300] = {};
        for (int thrown = 1; thrown >= 0; --thrown) {
            World w;
            Sim s = SwordWorld(w);
            s.Run(20.0f);
            LetGo(s, 1);
            s.Run(0.5f);
            const bool dropped = s.holder == Holder::kLoose && s.st.home[0].role == Role::kNone;
            usepoint::State us;
            s.hand[1].squeeze = 0.0f;
            const UseRun u = UseByHand(s, us, 1, kAtTube, 1.5f);
            const bool recorded = pickrules::LyingTaken(TakenIn(s, true));
            const unsigned long long tookAt = 100000;
            Kit(s, w, 1, true);
            s.Run(0.5f);
            const bool gift = u.use == 1 && s.holder == Holder::kHeld && s.trigger == 1 && AwayCount(s) == 0 && s.st.home[0].role == Role::kNone;
            s.Move(1, kFront);
            s.Run(0.1f);
            if (thrown) {
                LetGo(s, 1);
                s.Run(0.4f);
            } else {
                s.Run(1.5f);
            }
            KitBack(s, w);
            const pickrules::LyingBackOut lb = pickrules::LyingBack(BackIn(tookAt, tookAt + 16000, 0.9f));
            if (lb.verdict == pickrules::LyingBackVerdict::kLie) {
                s.holder = Holder::kLoose, s.trigger = s.barrel = -1, s.armed[0] = s.armed[1] = false;  // (hands.cpp's lay-down: no Arrive)
                s.st.lyingSlot = -1;  // (... and holsters' lying gun read afresh)
            }
            s.hand[1].squeeze = 0.0f;
            s.Run(0.5f);
            const bool ownKind = s.st.lyingKind == kM4Kind;
            const bool lies = s.holder == Holder::kLoose && s.st.home[0].role == Role::kNone && AwayCount(s) == 0 && s.st.lyingSlot == 0 && ownKind;
            const int keys = s.Keys();
            SqueezeAt(s, 1, kRS);
            s.hand[1].squeeze = 0.0f;
            s.Run(0.2f);
            const bool nothingThere = s.holder == Holder::kLoose && s.Keys() == keys;
            const size_t fromLeft = s.events.size();
            SqueezeAt(s, 1, kLS);
            s.hand[1].squeeze = 0.0f;
            s.Run(0.3f);
            const Event* lyingFirst = second::Last(s, What::kRefused, 1, fromLeft);
            const bool leftRefused = s.holder == Holder::kLoose && s.Keys() == keys && lyingFirst && lyingFirst->why == Why::kLying;
            const std::string leftLine = LineIn(s, lyingFirst);
            s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
            s.Run(0.2f);
            s.hand[1].squeeze = 1.0f;
            s.takeBy = 1;
            s.Run(0.3f);
            const bool taken = s.holder == Holder::kHeld && s.trigger == 1 && s.st.home[0].role == Role::kNone;
            PutOn(s, 1, kRS);
            const bool again = s.holder == Holder::kStowed && s.st.home[0].role == Role::kPrimary;
            ok[thrown] = dropped && recorded && gift && lb.verdict == pickrules::LyingBackVerdict::kLie && lies && nothingThere && leftRefused && taken &&
                         again;
            std::snprintf(what[thrown], sizeof(what[thrown]), "dropped %d, recorded %d, gift %d, back %d (%s), lies %d (its kind %s), nothing at the right "
                          "shoulder %d, the left's Mk11 refused %d (\"%s\"), taken %d, holstered again %d", dropped, recorded, gift, static_cast<int>(lb.verdict),
                          lb.why, lies, ownKind ? "the M4A1's" : "another's", nothingThere, leftRefused, leftLine.c_str(), taken, again);
        }
        Report("HT4", "flow (b) both together: the M4A1 dropped; the AT4 by reach and grip (its slot's lying gun recorded); fired, thrown (HT4b: still in "
                      "the hand); the kit's M4A1 lies again where it lay, on no shoulder, nothing at the right shoulder, the left's Mk11 refused while it "
                      "lies; taken off the floor by hand; holstered by hand",
               ok[0] && ok[1], "thrown: %s; still in the hand: %s", what[1], what[0]);
    }
    // HT4c the control for HT4's left shoulder (the reviews of e403947, their second finding): the tube thrown before the give-back, the M4A1 laid down
    // with holsters' lying gun not read afresh (the code before) still counts as the thrown AT4 (a picked-up gun let go of: it may go), so the left's
    // squeeze draws the Mk11 by key 2 and the game's switch takes the player's own M4A1 off the floor; read afresh (hands.cpp now), refused, no key.
    {
        int keys[2] = {};
        bool lying[2] = {};
        for (int reRead = 0; reRead < 2; ++reRead) {
            World w;
            Sim s = SwordWorld(w);
            s.Run(20.0f);
            LetGo(s, 1);
            s.Run(0.5f);
            usepoint::State us;
            UseByHand(s, us, 1, kAtTube, 1.5f);
            Kit(s, w, 1, true);
            s.Move(1, kFront);
            s.Run(0.1f);
            LetGo(s, 1);
            s.Run(0.4f);
            KitBack(s, w);
            s.holder = Holder::kLoose, s.trigger = s.barrel = -1, s.armed[0] = s.armed[1] = false;
            if (reRead)
                s.st.lyingSlot = -1;
            s.Run(0.5f);
            const int before = s.Keys(1);
            SqueezeAt(s, 1, kLS);
            s.hand[1].squeeze = 0.0f;
            s.Run(0.3f);
            keys[reRead] = s.Keys(1) - before;
            lying[reRead] = s.holder == Holder::kLoose && s.st.lyingKind == kM4Kind;
        }
        Report("HT4c", "the control: the tube thrown before the give-back, the M4A1 laid down: holsters' lying gun not read afresh (the code before), the "
                       "left's squeeze sends key 2 (the M4A1 would go off the floor); read afresh, nothing sent, the M4A1's own kind lying",
               keys[0] == 1 && keys[1] == 0 && lying[1], "not read afresh: %d key(s) for slot 1 (lying kind the M4A1's %d); read afresh: %d key(s), the "
                                                          "M4A1's kind lying %d",
               keys[0], lying[0], keys[1], lying[1]);
    }
    // HT5 X held's R at the prompt instead (the game's own key, no use recorded): (a) the M4A1 holstered: X held's pick every frame brings the AT4 into
    // the right hand (Arrive kHand), the M4A1 with the game at its shoulder, back at its place with the kit, drawn by hand; (b) the M4A1 lying: the gift
    // told by the prompt's R (PromptRSince), its lying gun recorded; the AT4 the game's own switch's (the right hand); thrown; laid again where it lay.
    {
        World w;
        Sim s = SwordWorld(w);
        s.Run(20.0f);
        PutOn(s, 1, kRS);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        for (int k = 0; k < 8; ++k) {
            s.picked = true, s.pickSlot = -1;  // (X held's R for the prompt: a pick of the user's every frame it's held)
            s.Step();
        }
        const ArriveOut take = Kit(s, w, 1, false);
        s.Run(0.5f);
        const Away* away = AwayOf(s, kM4Kind);
        const bool inRight = take.arrival == Arrival::kHand && take.side == 1 && s.holder == Holder::kHeld && s.trigger == 1 && away && !away->entity &&
                             s.st.home[0].role == Role::kNone;
        s.hand[1].squeeze = 1.0f;  // (a pick's arrival is unarmed till its grip is squeezed: then thrown)
        s.Run(0.2f);
        LetGo(s, 1);
        s.Run(0.4f);
        const bool thrownA = s.holder == Holder::kLoose;
        s.Run(2.6f);
        KitBack(s, w);
        const ArriveOut back = s.Switch(0);
        s.Run(0.3f);
        const bool backA = thrownA && back.arrival == Arrival::kAtPlace && s.st.home[0].role == Role::kPrimary && DrawnCaseA(s, 1, kRS);
        World wb;
        Sim b = SwordWorld(wb);
        b.Run(20.0f);
        LetGo(b, 1);
        b.Run(0.5f);
        const bool recorded = pickrules::LyingTaken(TakenIn(b, true)) && !pickrules::LyingTaken(TakenIn(b, false));
        for (int k = 0; k < 8; ++k) {
            b.picked = true, b.pickSlot = -1;
            b.Step();
        }
        const ArriveOut tb = Kit(b, wb, 1, false);
        b.Run(0.5f);
        const bool gameWay = tb.arrival == Arrival::kToday && b.holder == Holder::kHeld && AwayCount(b) == 0;
        b.hand[1].squeeze = 1.0f;
        b.Run(0.2f);
        LetGo(b, 1);
        b.Run(0.4f);
        const bool thrownB = b.holder == Holder::kLoose;
        b.Run(0.6f);
        KitBack(b, wb);
        const bool lie = thrownB && pickrules::LyingBack(BackIn(5000, 25000, 1.1f)).verdict == pickrules::LyingBackVerdict::kLie;
        Report("HT5", "X held's R at the prompt instead: (a) holstered: the AT4 into the right hand, the M4A1 with the game, back at its place, drawn by "
                      "hand; (b) lying: recorded by the prompt's R alone (never with no gift), the AT4 the game's switch's, laid again where it lay",
               inRight && backA && recorded && gameWay && lie, "(a) in the right %d, back and drawn %d (arrival %d); (b) recorded %d, the game's way %d, laid %d",
               inRight, backA, static_cast<int>(back.arrival), recorded, gameWay, lie);
    }
    // HT7 flow (b) through a death at the "RPG" checkpoint (the review of e403947's third finding: a miss kills, and the game loads the checkpoint with
    // the AT4 in slot 0; the helper died 4 times there): the M4A1 dropped, the AT4 taken by reach and grip (its lying gun recorded), a miss, a death; the
    // new life's kit (the AT4, the Mk11, the M67; hands.cpp keeps the record, its time from the new life); the hit 16 s into the life, thrown; the
    // kit's M4A1 back 18 s in: it lies where it lay (LyingBack), on no shoulder, the left's Mk11 refused while it lies; taken off the floor, holstered
    // by hand. HT7b: the record's gun in a new life's kit seconds (a load of an earlier save: the kit's M4A1 at its spawn) is that life's, as at a spawn;
    // another level's kit ends it; the AT4 in the new life's kit leaves it waiting. (The rule before: every new life ended it, the M4A1 into the hand.)
    {
        World w;
        Sim s = SwordWorld(w);
        s.Run(20.0f);
        LetGo(s, 1);
        s.Run(0.5f);
        usepoint::State us;
        const UseRun u = UseByHand(s, us, 1, kAtTube, 1.5f);
        const bool recorded = pickrules::LyingTaken(TakenIn(s, true));
        Kit(s, w, 1, true);
        // A miss: a death at the fire window's end, the "RPG" checkpoint loaded (hands.cpp: the record's time from the new life).
        const unsigned long long lifeAt = 100000 + 35000;
        NewLife(s.st);
        Swordbreaker(s);
        Gave(s, 0, 0xA791, kAt4Kind, true, true);
        CopyName(s.slots[0].name, "AT4");
        s.slots[1].known = true;
        Named(s.slots[1], 0x1112, kMk11Kind, "Mk11");
        Named(s.slots[6], 0x6762, 0x67, "M67");
        s.holder = Holder::kHeld, s.trigger = 1, s.gameSlot = 0;
        second::Squeeze(s, 1);
        pickrules::LyingBackIn kit = BackIn(lifeAt, lifeAt, 0.9f);
        kit.newId = kAt4Kind, kit.newLife = true, kit.lifeAt = lifeAt;  // (the new life's kit: the AT4 again)
        const bool waits = pickrules::LyingBack(kit).verdict == pickrules::LyingBackVerdict::kNone;
        s.Run(16.0f);
        LetGo(s, 1);  // (fired, hit, thrown)
        s.Run(0.4f);
        KitBack(s, w);
        pickrules::LyingBackIn backIn = BackIn(lifeAt, lifeAt + 18000, 1.0f);
        backIn.lifeAt = lifeAt;
        const pickrules::LyingBackOut back = pickrules::LyingBack(backIn);
        if (back.verdict == pickrules::LyingBackVerdict::kLie) {
            s.holder = Holder::kLoose, s.trigger = s.barrel = -1, s.armed[0] = s.armed[1] = false;  // (hands.cpp's lay-down)
            s.st.lyingSlot = -1;
        }
        s.hand[1].squeeze = 0.0f;
        s.Run(0.5f);
        const bool lies = s.holder == Holder::kLoose && s.st.home[0].role == Role::kNone && AwayCount(s) == 0 && s.st.lyingKind == kM4Kind &&
                          !s.st.memoForgot;
        const int keys = s.Keys();
        const size_t from = s.events.size();
        SqueezeAt(s, 1, kLS);
        s.hand[1].squeeze = 0.0f;
        s.Run(0.3f);
        const Event* refusal = second::Last(s, What::kRefused, 1, from);
        const bool leftRefused = s.holder == Holder::kLoose && s.Keys() == keys && refusal && refusal->why == Why::kLying;
        const std::string refusalLine = LineIn(s, refusal);
        s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
        s.Run(0.2f);
        s.hand[1].squeeze = 1.0f;
        s.takeBy = 1;
        s.Run(0.3f);
        const bool taken = s.holder == Holder::kHeld && s.trigger == 1 && s.st.home[0].role == Role::kNone;
        PutOn(s, 1, kRS);
        const bool again = s.holder == Holder::kStowed && s.st.home[0].role == Role::kPrimary;
        Report("HT7", "flow (b) through a death at the \"RPG\" checkpoint: the record kept (the new life's AT4 leaves it waiting), the kit's M4A1 after the "
                      "hit lies where it lay, on no shoulder, the left's Mk11 refused while it lies; taken off the floor, holstered by hand",
               u.use == 1 && recorded && waits && back.verdict == pickrules::LyingBackVerdict::kLie && lies && leftRefused && taken && again,
               "use %d, recorded %d; the new life's AT4: waits %d; back %d (%s); lies %d (memo forgot %d); the left's \"%s\" %d; taken %d, holstered %d", u.use,
               recorded, waits, static_cast<int>(back.verdict), back.why, lies, s.st.memoForgot, refusalLine.c_str(), leftRefused, taken, again);
        pickrules::LyingBackIn spawn = BackIn(lifeAt, lifeAt + 4000, 1.0f), first = BackIn(lifeAt, lifeAt, 1.0f), level = BackIn(lifeAt, lifeAt + 18000, 1.0f);
        spawn.lifeAt = first.lifeAt = level.lifeAt = lifeAt;
        first.newLife = true;
        level.otherLevel = true;
        const pickrules::LyingBackOut a = pickrules::LyingBack(spawn), b = pickrules::LyingBack(first), c = pickrules::LyingBack(level);
        Report("HT7b", "the record's gun in a new life's kit (its first frame, or 4 s in: a load of an earlier save) is that life's, as at a spawn; another "
                       "level's kit ends it",
               a.verdict == pickrules::LyingBackVerdict::kDrop && std::strcmp(a.why, "a new life's kit gave it, as at a spawn") == 0 &&
                   b.verdict == pickrules::LyingBackVerdict::kDrop && std::strcmp(b.why, a.why) == 0 && c.verdict == pickrules::LyingBackVerdict::kDrop &&
                   std::strcmp(c.why, "another level's kit: the world it lay in is gone") == 0,
               "4 s in: %d (%s); its first frame: %d (%s); another level: %d (%s)", static_cast<int>(a.verdict), a.why, static_cast<int>(b.verdict), b.why,
               static_cast<int>(c.verdict), c.why);
    }
    // HT8 nothing comes into a hand between a use and its gift (hands.cpp's GiftOnItsWay: holsters' Frame::pendingTake, the grasp's hands busy; the
    // review of e403947's third finding, a rifle in a hand in the 0.6 s before the AT4's draw): flow (a), the left's use made, the right squeezing at the
    // right shoulder before the kit's swap: no draw, no key (a moment), the M4A1 still put away; the AT4 into the left, the M4A1 with the game at its
    // shoulder. The control without it: the M4A1 drawn into the right (case A) as the kit came for slot 0.
    {
        bool stayed[2] = {}, withGame[2] = {};
        for (int guard = 0; guard < 2; ++guard) {
            World w;
            Sim s = SwordWorld(w);
            s.Run(20.0f);
            PutOn(s, 1, kRS);
            s.MoveTo(1, kFront[0], kFront[1], kFront[2]);
            s.Run(0.2f);
            usepoint::State us;
            UseByHand(s, us, 0, kAtTube, 1.5f);
            s.pendingTake = guard != 0;  // (the gift on its way: the use made, its AT4 not drawn yet)
            SqueezeAt(s, 1, kRS);
            s.pendingTake = false;
            stayed[guard] = s.holder == Holder::kStowed && s.draws == 0 && s.Keys() == 0;
            s.hand[1].squeeze = 0.0f;
            Kit(s, w, 0, true);
            const Away* a = AwayOf(s, kM4Kind);
            withGame[guard] = a && !a->entity && a->role == Role::kPrimary && s.holder == Holder::kHeld && s.trigger == 0;
        }
        Report("HT8", "a squeeze at the right shoulder between the left's use and its gift: nothing drawn, no key, the M4A1 put away, then with the game at "
                      "its shoulder as the AT4 comes into the left; the control without the gift's way: drawn into the right",
               stayed[1] && withGame[1] && !stayed[0], "with the gift's way: stayed %d, with the game %d; without: stayed %d", stayed[1], withGame[1], stayed[0]);
    }
    // HT6 hands.cpp's lines for all of it, word for word: the gift's take mark, UseYields' rule, the record's inputs and the give-back's, the lay-down
    // with no Arrive, its still start in StepLoose; holster.h's Arrive rule for a gun given back while the hands hold another.
    {
        std::string hands, holster;
        for (auto [path, out] : {std::pair<const char*, std::string*>{"src\\hands.cpp", &hands}, {"src\\holster.h", &holster}}) {
            FILE* f = nullptr;
            if (fopen_s(&f, path, "rb") == 0 && f) {
                char buf[65536];
                size_t n;
                while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
                    out->append(buf, n);
                fclose(f);
            }
            out->erase(std::remove(out->begin(), out->end(), '\r'), out->end());
        }
        const char* const handsLines[] = {
            "            if (g_pick.landedSlot >= 0 && g_pick.landedSlot < holster::kSlots)\n                g_holsterTaken |= 1u << g_pick.landedSlot;",
            "    if (hr.place[side] != holster::Place::kNone || hr.busy[side] || hr.drawSide == side || hr.stowSide == side)\n        return true;",
            "        lt.gift = (g_useMade.side >= 0 && g_useMade.gift && nowMs - g_useMade.at <= kUseGiftMs) || input::PromptRSince() <= kUseGiftMs;",
            "        lt.lies = g_grasp.holder == grasp::Holder::kLoose && g_gunLoose.id != 0 && g_gunLoose.have;",
            "        lt.lyingId = g_profileWas, lt.newId = p.id;\n        lt.lyingSlot = g_gunLoose.slot, lt.newSlot = ammo::GameSlot();",
            "        lb.newId = p.id, lb.newSlot = ammo::GameSlot();\n        lb.take = g_pick.landedSide >= 0 && PickArrives(p.id);",
            "            g_grasp.holder = grasp::Holder::kLoose;\n            g_grasp.trigger = -1;",
            "            g_gunRelease.want = g_gunRelease.still = true;",
            "            g_lyingGone = LyingGone{};\n            return false;\n        }\n    }\n    // A gun taken off the ground by hand (pickups.h)",
            "                       : g_gunRelease.still        ? loose::Place(*body, kLooseGun, bone, g_gunRelease.rounds, f)",
            "                bone[i * 4 + 3] += static_cast<float>(g_gunRelease.stillOrigin[i] - f.origin[i]);",
            // (The reviews of e403947: the use barred with a gun in a hand (HT3), holsters' lying gun read afresh at the lay-down (HT4, HT4c), the
            // record through a new life on its level (HT7), nothing into a hand while the gift is on its way (HT8), the gift unarmed in an open hand.)
            "    uf.point.gives = uf.point.on && up.action == pickups::kUseReload;\n    uf.gunHeld = GunInAHand(nullptr, 0);",
            "            g_holster.lyingSlot = -1;\n            g_gunRelease = GunRelease{};\n            g_gunRelease.want = g_gunRelease.still = true;",
            "            g_lyingGone.at = g_lyingGone.lifeAt = GetTickCount64();",
            "        lb.lifeAt = g_lyingGone.lifeAt;\n        lb.otherLevel = g_lyingGone.lifeAt && g_holster.memoForgot;",
            "    if (g_lyingGone.on && g_lyingGone.lifeAt && g_holster.memoForgot) {",
            "    const bool giftWay = GiftOnItsWay(GetTickCount64());\n    hf.pendingTake = g_grasp.pendingSide >= 0 || giftWay;",
            "        h.busy = h.busy || hr.busy[s] || uo.spent[s] || giftWay;",
            "            f.arriveSide = a, f.arriveMagnet = g_pick.magnet, f.arriveUnarmed = g_pick.byUse;",
        };
        int found = 0;
        for (const char* l : handsLines)
            found += hands.find(l) != std::string::npos;
        const bool rule = holster.find("    if (comeback && !st.comebackByTake && !in.stowed && !in.lying && st.clock - st.pickAt > kPickKeeps) {\n        st.wait = "
                                       "Wait{};\n        out.arrival = Arrival::kAtPlace;\n        return out;\n    }") != std::string::npos;
        if (hands.empty() || holster.empty())
            Report("HT6", "hands.cpp's and holster.h's lines (run from the project's folder)", false, "src\\hands.cpp or src\\holster.h not read");
        else
            Report("HT6", "hands.cpp's lines the rows above stand for, word for word (the gift's mark, UseYields, the record, the give-back, the still start; "
                          "the use barred with a gun in a hand, the lying gun read afresh, the record through a new life, the gift's way, the gift unarmed), "
                          "and holster.h's rule for a gun given back while the hands hold another",
                   found == static_cast<int>(std::size(handsLines)) && rule, "%d of %d of hands.cpp's; holster.h's rule %d", found,
                   static_cast<int>(std::size(handsLines)), rule);
    }
}

}  // namespace merge

// ---- K: every forced change of gun and every kit a script gives in the six missions left (docs\research\missions\, PLAN.md A13), against holsters by
// hand. Each scene as its survey reads the level (nothing of it run in the game): with the guns the hands know today, and (where it differs) with every
// gun held, as once the missions' guns are built. Free hands as hands.cpp has them: awake only with a gun the hands know as the game's weapon (a knife, a
// mortar, a gun the game draws its own way: asleep, the grasp cleared to the right hand); a scripted moment's arrival today's. What must hold whatever the
// scene (Audit): one gun a shoulder, gear never at a holster, no record of a gun while one of its kind is in its slot (two of it), none of a gun a kit
// replaced past the take's window. ----
namespace kits {

using second::CountFrom;
using second::Last;

// The K rows run with "kitguns holster" (the A13 lane's KitSwap: a script's kit's gun has the kit's holster, an opt-in since its settle); the KO rows
// with the default, "kitguns none" (such a gun on no shoulder till a hand puts it on one, as the owner played the finished missions).
bool g_kitHolster = true;
Settings KitSet() {
    Settings s;  // (the KO rows: the defaults as they are)
    if (g_kitHolster)
        s.kitHolster = true;
    return s;
}

constexpr uint32_t kKnife = 0xC01, kMortar = 0xC02, kM67 = 0x67, kMachete = 0xC03, kCuffs = 0xC04, kClaymore = 0xC05;
constexpr uint32_t kAks = 0x1A74, kScar = 0x5CA, k870 = 0x870, kAsVal = 0xA5A, kM40 = 0x40A5, kMp7 = 0x7A7, kPp2000 = 0x2000, kAek = 0xAE9,
                   kSaiga = 0x5A1, kMp443 = 0x443, kM82 = 0x82A3, kM9 = 0x9A9, kM1014 = 0x1014, kSg553 = 0x553, kJavelin = 0x148, kG3 = 0x63A3;

// A gun in a slot with no take (a kit's, or a new life's), gear (no gun), nothing.
void Gun(Sim& s, int slot, uint32_t id, uint32_t kind, const char* name, bool known, bool launcher = false) {
    SlotIn& v = s.slots[slot];
    v = SlotIn{};
    v.carried = true, v.launcher = launcher, v.known = known, v.id = id, v.kind = kind;
    CopyName(v.name, name);
}
void Gear(Sim& s, int slot, uint32_t id, uint32_t kind, const char* name) {
    Gun(s, slot, id, kind, name, false);
    s.slots[slot].gear = true;
}
void Gone(Sim& s, int slot) { s.slots[slot] = SlotIn{}; }
void NoSlots(Sim& s) {
    for (SlotIn& v : s.slots)
        v = SlotIn{};
}
void Hook(Sim& s, World& w) { s.world = &w, s.before = WorldBefore, s.after = WorldAfter; }
// hands.cpp's free hands: awake only with a gun the hands know as the game's weapon (FreeHands' enabled); asleep, the grasp cleared to the right hand
// as at a spawn, nothing squeezed.
void Hands(Sim& s) {
    const bool known = s.gameSlot >= 0 && s.gameSlot < kSlots && s.slots[s.gameSlot].carried && s.slots[s.gameSlot].known;
    s.on = known;
    if (!known) {
        s.holder = Holder::kHeld, s.trigger = 1, s.barrel = -1, s.armed[0] = s.armed[1] = false;
        s.hand[0].squeeze = s.hand[1].squeeze = 0.0f;
    }
}
// The game's weapon now that slot's: free hands as hands.cpp has them and, awake with a change of gun (arrive), its arrival (OnGunChanged's Arrive; scripted:
// a scripted moment's or a cutscene's end, today's).
ArriveOut Draws(Sim& s, int slot, bool scripted = false, bool arrive = true) {
    s.gameSlot = slot;
    Hands(s);
    ArriveOut o;
    if (s.on && arrive)
        o = s.Switch(slot, false, scripted);
    return o;
}
// That hand's grip squeezed and held (armed, as the grasp has it past 0.6).
void Grip(Sim& s, int side) {
    s.hand[side].squeeze = 1.0f;
    s.Run(0.1f);
}
// The right hand takes pickup pi off the floor (a hand's take, the magnet's, or X held's R with the gun in the hands): the game's take (TakeOf), into
// that hand by the take's arrival, its grip squeezing (armed: it holds on); free hands awake with the gun it took (a held gun).
void TakeUp(Sim& s, World& w, int pi) {
    TakeOf(s, w, pi, 1);
    s.hand[1].squeeze = 1.0f;
    s.on = s.slots[s.gameSlot].known;
}
// What must hold whatever the scene; "" when it does.
std::string Audit(const Sim& s) {
    char why[256] = "";
    int primary = 0, secondary = 0;
    for (int k = 0; k < kSlots; ++k) {
        const SlotIn& v = s.slots[k];
        if (!v.carried)
            continue;
        const Role r = s.st.home[k].role;
        if (v.gear && r != Role::kNone) {
            std::snprintf(why, sizeof(why), "the %s (gear, slot %d) has a holster (%s)", v.name, k, RoleName(r));
            return why;
        }
        primary += r == Role::kPrimary, secondary += r == Role::kSecondary;
    }
    for (const Away& a : s.st.away) {
        if (!a.on)
            continue;
        if (a.kitGone && s.st.clock - a.since > kTakeNear + 0.05f) {
            std::snprintf(why, sizeof(why), "the %s a kit replaced still recorded %.1f s on", a.name, s.st.clock - a.since);
            return why;
        }
        if (!a.kitGone && a.slot >= 0 && a.slot < kSlots && s.slots[a.slot].carried && s.slots[a.slot].kind == a.kind) {
            std::snprintf(why, sizeof(why), "the %s recorded %s while one is in its slot %d: two of it", a.name, a.entity ? "in a pickup" : "with the game",
                          a.slot);
            return why;
        }
        if (a.entity)
            primary += a.role == Role::kPrimary, secondary += a.role == Role::kSecondary;
    }
    if (primary > 1 || secondary > 1)
        std::snprintf(why, sizeof(why), "%d guns at the right shoulder, %d at the left (one gun a shoulder)", primary, secondary);
    return why;
}
// The lines said from index from on (ticks aside), as hands.cpp would say them: for a failed row.
std::string Said(const Sim& s, size_t from = 0) {
    std::string out;
    for (size_t i = from; i < s.events.size(); ++i)
        if (s.events[i].what != What::kTick && s.events[i].what != What::kLook)
            out += "\n        " + LineIn(s, &s.events[i]);
    return out;
}
int Shoulders(const Sim& s, size_t from) {  // lines of shoulders after a forced switch: none wanted where nothing changed hands
    return CountFrom(s, What::kBackHome, from) + CountFrom(s, What::kNoPlace, from) + CountFrom(s, What::kToHand, from) +
           CountFrom(s, What::kAway, from) + CountFrom(s, What::kLost, from);
}

// ---- Uprising (SP_Earthquake2) ----

// The level's start: the knife alone in slot 0 (UsHumanSpSoldier_SP_Earthquake2_Knife), free hands asleep; 40 s of crawl; the stab's kit (the bazaar
// door's first-person kill: UsHumanSpSoldier_SP_Earthquake2_Silenced, the AKS-74u into slot 0, the M67 into 6, the knife into 7), its first draw at the
// scene's end; past the take's window. knife: the knife's holster before the stab.
Sim Stabbed(World* w, Role* knife = nullptr, size_t* kitFrom = nullptr) {
    Sim s;
    s.set = KitSet();
    if (w)
        Hook(s, *w);
    NoSlots(s);
    Gear(s, 0, 0xC010, kKnife, "Knife");
    Draws(s, 0);
    s.Run(0.5f);
    if (knife)
        *knife = s.st.home[0].role;
    s.Run(40.0f);
    if (kitFrom)
        *kitFrom = s.events.size();
    s.frozen = true;
    Gun(s, 0, 0x17A0, kAks, "AKS-74u", true);
    Gear(s, 6, 0x6710, kM67, "M67");
    Gear(s, 7, 0xC011, kKnife, "Knife");
    s.Step();
    Draws(s, 0, true);
    s.Run(0.5f);
    s.frozen = false;
    s.Run(1.5f);
    return s;
}

void Uprising() {
    {
        Role knife = Role::kNone;
        size_t from = 0;
        Sim s = Stabbed(nullptr, &knife, &from);
        const Event* kit = Kept(Last(s, What::kKit, -2, from), 0);
        const bool given = knife == Role::kNone && s.holder == Holder::kHeld && s.trigger == 1 && s.st.home[0].role == Role::kPrimary &&
                           PlaceNow(s, 0) == Place::kRightShoulder && s.st.home[6].role == Role::kNone && s.st.home[7].role == Role::kNone &&
                           AwayCount(s) == 0 && CountFrom(s, What::kKit, from) == 1 && kit && kit->slot == 0 && kit->place == Place::kRightShoulder &&
                           !kit->otherGun[0] && CountFrom(s, What::kAway, from) == 0;
        Grip(s, 1);
        PutOn(s, 1, kRS);
        const bool holstered = s.holder == Holder::kStowed && s.st.home[0].role == Role::kPrimary;
        // The school's breach: the game's switch to slot 0 (already the AKS-74u's) under the first-person kick: nothing drawn, no key.
        const int keys = s.Keys(), draws = s.draws;
        s.frozen = true;
        s.gameSlot = 0;
        s.Run(2.0f);
        s.frozen = false;
        s.Run(0.3f);
        const bool breach = s.holder == Holder::kStowed && s.Keys() == keys && s.draws == draws;
        const bool back = merge::DrawnCaseA(s, 1, kRS);
        const std::string audit = Audit(s);
        const bool pass = given && holstered && breach && back && audit.empty();
        Report("KU1", "Uprising: the knife alone in slot 0 at the start is at no holster (gear); the stab's kit gives the AKS-74u: the kit's, the right "
                      "shoulder's (said once its take's window is over, the knife never named as gone), nothing away, in the right hand at the scene's "
                      "end; put on the right shoulder by hand; the school's breach (the game's slot 0 under a kick) draws nothing; drawn back",
               pass, "the knife %s; the AKS-74u %s at %s, away %d, kit lines %d (\"%s\"); holstered %d, breach %d, drawn back %d; %s", RoleName(knife),
               RoleName(s.st.home[0].role), Name(PlaceNow(s, 0)), AwayCount(s), CountFrom(s, What::kKit, from), LineIn(s, kit).c_str(), holstered, breach,
               back, audit.c_str());
        if (!pass)
            std::printf("%s\n", Said(s).c_str());
    }
    {
        // A death, then the Parking Lot's load (the School's alike): its save's guns, then its script's Silenced kit again with every other gun removed.
        // The AKS-74u swapped for a G3A3 off the floor before (the AKS-74u, holstered, into the G3A3's pickup; the G3A3 put on the right shoulder by
        // hand: the AKS-74u on no shoulder, its kind's memo none): the kit's AKS-74u has the G3A3's right shoulder, nothing away, the G3A3 gone with the
        // kit, never said. The control: the save had the AKS-74u, the kit gives it again: as it was.
        World w;
        Sim s = Stabbed(&w);
        const int g3 = w.AddPick(0xE31, 1.0f, 0, kG3, 20, 80, false, "G3A3");
        Grip(s, 1);
        PutOn(s, 1, kRS);
        second::Front(s, 1);
        TakeUp(s, w, g3);
        s.Run(0.3f);
        const Away* aks = AwayOf(s, kAks);
        const bool away = aks && aks->entity == 0xE31 && aks->role == Role::kPrimary;
        Grip(s, 1);
        PutOn(s, 1, kRS);
        const bool g3Right = s.holder == Holder::kStowed && s.st.home[0].role == Role::kPrimary && AwayCount(s) == 0;
        const Memo* memo = MemoOf(s.st, kAks);
        const bool aksNone = memo && memo->role == Role::kNone;
        NewLife(s.st);
        NoSlots(s);
        Gun(s, 0, 0x63A9, kG3, "G3A3", true);
        Gear(s, 6, 0x6711, kM67, "M67");
        Gear(s, 7, 0xC013, kKnife, "Knife");
        s.holder = Holder::kHeld, s.trigger = 1, s.gameSlot = 0, s.on = true;
        s.Run(0.3f);
        const Role saved = s.st.home[0].role;
        const size_t from = s.events.size();
        Gun(s, 0, 0x17A9, kAks, "AKS-74u", true);
        Gear(s, 6, 0x6712, kM67, "M67");
        Gear(s, 7, 0xC014, kKnife, "Knife");
        s.Run(1.5f);
        const std::string audit = Audit(s);
        Sim c = Stabbed(nullptr);
        Grip(c, 1);
        PutOn(c, 1, kRS);
        NewLife(c.st);
        NoSlots(c);
        Gun(c, 0, 0x17AA, kAks, "AKS-74u", true);
        Gear(c, 7, 0xC015, kKnife, "Knife");
        c.holder = Holder::kHeld, c.trigger = 1, c.gameSlot = 0;
        c.Run(0.3f);
        Gun(c, 0, 0x17AB, kAks, "AKS-74u", true);
        c.Run(1.5f);
        const bool pass = away && g3Right && aksNone && saved == Role::kPrimary && s.st.home[0].role == Role::kPrimary &&
                          PlaceNow(s, 0) == Place::kRightShoulder && AwayCount(s) == 0 && CountFrom(s, What::kAway, from) == 0 &&
                          CountFrom(s, What::kLost, from) == 0 && audit.empty() && c.st.home[0].role == Role::kPrimary && Audit(c).empty();
        Report("KU2", "Uprising, a death and the Parking Lot's (or the School's) load: its save's G3A3 (put on the right shoulder by hand, the AKS-74u on "
                      "none by then), then its script's Silenced kit with every other gun removed: the kit's AKS-74u has the G3A3's right shoulder, nothing "
                      "away, nothing said of the G3A3; the control, a save with the AKS-74u: the right shoulder's as it was",
               pass, "the AKS-74u away in the G3A3's pickup %d; the G3A3 right %d; the AKS-74u's memo none %d; at the load: the G3A3 %s, then the AKS-74u %s "
                     "at %s, away %d; the control %s; %s",
               away, g3Right, aksNone, RoleName(saved), RoleName(s.st.home[0].role), Name(PlaceNow(s, 0)), AwayCount(s), RoleName(c.st.home[0].role),
               audit.c_str());
        if (!pass)
            std::printf("%s\n", Said(s, from).c_str());
    }
}

// ---- Operation Guillotine (SP_Bank) ----

// The level's start: the mortar alone in slot 0 (StartKitMortar, RemoveAllExistingWeapons), free hands asleep; 90 s of trench scene and carry; then the
// SP_Bank kit 11 s after MOUNT MORTAR, in the DeployMortar scene: the SCAR-H (0) with its ACOG, the 870 MCS (1), the M67 (6), the knife (7); its first
// draw at the scene's end; past the take's window. all: the SCAR-H and the 870 held guns (they are not yet: the game draws them its own way).
Sim Mortar(World* w, bool all, Role* mortar = nullptr, size_t* kitFrom = nullptr) {
    Sim s;
    s.set = KitSet();
    if (w)
        Hook(s, *w);
    NoSlots(s);
    Gear(s, 0, 0x2240, kMortar, "M224 mortar");
    Draws(s, 0);
    s.Run(0.5f);
    if (mortar)
        *mortar = s.st.home[0].role;
    s.Run(90.0f);
    if (kitFrom)
        *kitFrom = s.events.size();
    s.frozen = true;
    Gun(s, 0, 0x5CA0, kScar, "SCAR-H", all);
    Gun(s, 1, 0x8700, k870, "870 MCS", all);
    Gear(s, 6, 0x6720, kM67, "M67");
    Gear(s, 7, 0xC012, kKnife, "Knife");
    s.Step();
    Draws(s, 0, true);
    s.Run(0.5f);
    s.frozen = false;
    s.Run(1.5f);
    return s;
}
// The bridge's end: the rift's climb, its scene, the fade, the M4A1 kit (UsHumanSpSoldier_SP_005: slot 0 alone, RemoveAllExistingWeapons False); its
// first draw at the scene's end if slot 0's gun was the game's weapon (drawn), past the take's window.
void Bridge(Sim& s, bool drawn = true) {
    s.frozen = true;
    s.Run(3.0f);
    Gun(s, 0, 0x4A30, kM4Kind, "M4A1", true);
    s.Step();
    if (drawn)
        Draws(s, 0, true);
    s.Run(0.5f);
    s.frozen = false;
    s.Run(1.5f);
}

void Guillotine() {
    for (int all = 0; all < 2; ++all) {
        Role mortar = Role::kNone;
        size_t from = 0;
        Sim s = Mortar(nullptr, all != 0, &mortar, &from);
        const Event* scar = nullptr;
        const Event* m870 = nullptr;
        for (size_t i = from; i < s.events.size(); ++i)
            if (s.events[i].what == What::kKit)
                (s.events[i].slot == 0 ? scar : m870) = &s.events[i];
        bool given = mortar == Role::kNone && s.st.home[0].role == Role::kPrimary && s.st.home[1].role == Role::kSecondary &&
                     PlaceNow(s, 0) == Place::kRightShoulder && PlaceNow(s, 1) == Place::kLeftShoulder && s.st.home[6].role == Role::kNone &&
                     s.st.home[7].role == Role::kNone && AwayCount(s) == 0 && CountFrom(s, What::kKit, from) == 2 && scar && !scar->otherGun[0] &&
                     scar->place == Place::kRightShoulder && m870 && m870->place == Place::kLeftShoulder && CountFrom(s, What::kAway, from) == 0;
        const std::string scarLine = LineIn(s, scar), line870 = LineIn(s, m870);
        bool use = false;
        if (all) {
            // Put on the right shoulder; the 870 drawn at the left (key 2, the game's switch, into the hand that drew).
            Grip(s, 1);
            PutOn(s, 1, kRS);
            SqueezeAt(s, 1, kLS);
            const bool key = s.Keys(1) == 1;
            const ArriveOut o = Draws(s, 1);
            use = key && o.arrival == Arrival::kHand && o.side == 1 && s.holder == Holder::kHeld && s.trigger == 1;
        } else {
            // Free hands asleep with the SCAR-H the game's way: a squeeze at the left shoulder does nothing (the D-pad or Y gives the 870).
            SqueezeAt(s, 1, kLS);
            use = s.Keys() == 0 && s.draws == 0;
        }
        const std::string audit = Audit(s);
        const bool pass = given && use && audit.empty();
        Report(all ? "KG1k" : "KG1", all ? "Guillotine with every gun held: as KG1; the SCAR-H put on the right shoulder, the 870 drawn at the left (key 2), "
                                           "into the hand that drew"
                                         : "Guillotine: the mortar alone in slot 0 at the start is at no holster (gear); the SP_Bank kit in the mortar's scene: "
                                           "the SCAR-H the kit's, the right shoulder's, the 870 the left's (said once each, nothing of the mortar), nothing "
                                           "away; free hands asleep with them (the game's own guns): a squeeze at a shoulder does nothing",
               pass, "the mortar %s; the SCAR-H %s, the 870 %s, away %d; \"%s\" | \"%s\"; the hands %d; %s", RoleName(mortar), RoleName(s.st.home[0].role),
               RoleName(s.st.home[1].role), AwayCount(s), scarLine.c_str(), line870.c_str(), use, audit.c_str());
        if (!pass)
            std::printf("%s\n", Said(s, from).c_str());
    }
    for (int all = 0; all < 2; ++all) {
        // The bridge's M4A1 kit with the SCAR-H the game's weapon (all: in the right hand; today: drawn the game's way, free hands asleep): the M4A1 has
        // the SCAR-H's right shoulder, in the right hand at the scene's end; the 870 the left's still; nothing away, the SCAR-H gone with the kit (said:
        // "in place of the SCAR-H").
        Sim s = Mortar(nullptr, all != 0);
        if (all)
            Grip(s, 1);
        const size_t from = s.events.size();
        Bridge(s);
        const Event* kit = Kept(Last(s, What::kKit, -2, from), 0);
        const std::string audit = Audit(s);
        const bool pass = s.st.home[0].role == Role::kPrimary && s.holder == Holder::kHeld && s.trigger == 1 && s.st.home[1].role == Role::kSecondary &&
                          AwayCount(s) == 0 && CountFrom(s, What::kKit, from) == 1 && kit && std::strcmp(kit->otherGun, "SCAR-H") == 0 &&
                          kit->place == Place::kRightShoulder && CountFrom(s, What::kAway, from) == 0 && audit.empty();
        Report(all ? "KG2k" : "KG2", "Guillotine, the bridge's end: the M4A1 kit under the fade with the SCAR-H the game's weapon: the M4A1 has its right "
                                     "shoulder, in the right hand at the scene's end, the 870 still the left's; nothing away (the SCAR-H gone with the kit)",
               pass, "the M4A1 %s, holder %d by %d, the 870 %s, away %d; \"%s\"; %s", RoleName(s.st.home[0].role), static_cast<int>(s.holder), s.trigger,
               RoleName(s.st.home[1].role), AwayCount(s), LineIn(s, kit).c_str(), audit.c_str());
        if (!pass)
            std::printf("%s\n", Said(s, from).c_str());
    }
    {
        // Every gun held: (b) the SCAR-H put away at the right shoulder, the kit under the fade (its draw a scene's end's: into the right hand): the M4A1
        // the right shoulder's, nothing away; (c) the 870 in the hands and the SCAR-H hanging at the right shoulder: the M4A1 hangs there (the game's
        // weapon unchanged, no arrival), a squeeze there with the 870 held refused quietly, the 870 put back at the left, the M4A1 drawn at the right
        // (key 1, into the hand that drew).
        Sim b = Mortar(nullptr, true);
        Grip(b, 1);
        PutOn(b, 1, kRS);
        const bool stowed = b.holder == Holder::kStowed;
        Bridge(b);
        const bool bOk = stowed && b.st.home[0].role == Role::kPrimary && b.holder == Holder::kHeld && AwayCount(b) == 0 && Audit(b).empty();
        Sim c = Mortar(nullptr, true);
        Grip(c, 1);
        PutOn(c, 1, kRS);
        SqueezeAt(c, 1, kLS);
        Draws(c, 1);
        Grip(c, 1);
        const bool in870 = c.holder == Holder::kHeld && c.trigger == 1 && c.gameSlot == 1;
        Bridge(c, false);
        const bool hangs = c.st.home[0].role == Role::kPrimary && Hung(c, 0) == Place::kRightShoulder && AwayCount(c) == 0;
        const size_t from = c.events.size();
        SqueezeAt(c, 0, kRS);
        c.hand[0].squeeze = 0.0f;
        const Event* refused = Last(c, What::kRefused, 0, from);
        const bool quiet = refused && refused->why == Why::kOtherHolds && refused->quiet && c.Keys(0) == 0;
        c.Run(0.2f);
        PutOn(c, 1, kLS);
        SqueezeAt(c, 1, kRS);
        const bool key1 = c.Keys(0) == 1;
        const ArriveOut o = Draws(c, 0);
        const bool drawn = key1 && o.arrival == Arrival::kHand && o.side == 1 && c.holder == Holder::kHeld;
        const std::string audit = Audit(c);
        const bool pass = bOk && in870 && hangs && quiet && drawn && audit.empty();
        Report("KG3k", "Guillotine with every gun held, the bridge's kit: (b) the SCAR-H put away: the M4A1 the right shoulder's, in the right hand at the "
                       "scene's end; (c) the 870 in the hands, the SCAR-H hanging: the M4A1 hangs at the right shoulder, a squeeze there refused quietly while "
                       "the 870 is held, then drawn there (key 1) into the hand that drew",
               pass, "(b) %d; (c) the 870 in hand %d, the M4A1 %s hangs %s, refused quietly %d, drawn %d; %s", bOk, in870, RoleName(c.st.home[0].role),
               Name(Hung(c, 0)), quiet, drawn, audit.c_str());
        if (!pass)
            std::printf("%s\n", Said(c).c_str());
    }
    for (int all = 0; all < 2; ++all) {
        // (d) The SCAR-H the game's weapon when an AEK-971 is taken off the floor (it names slot 0: the SCAR-H into its pickup, held: no record); the AEK-971
        // put on the right shoulder by hand; the bridge's kit: the M4A1 has the AEK-971's right shoulder, the AEK-971 gone with the kit. (e) every gun held,
        // the SCAR-H put away at the right shoulder when the AEK-971 is taken (away in its pickup, the right shoulder's), the AEK-971 never placed: the kit's
        // M4A1 on no shoulder, the SCAR-H's in its pickup still (the user, 2026-11-18: a holstered gun stays till a hand takes it out and drops it; the
        // A13 lane's settle: until then the kit's gun took it); the M4A1 put there by hand wins (the SCAR-H in its pickup on no shoulder, said).
        World w;
        Sim d = Mortar(&w, all != 0);
        const int aek = w.AddPick(0xEA1, 1.0f, 0, kAek, 30, 90, false, "AEK-971");
        TakeUp(d, w, aek);
        Grip(d, 1);
        PutOn(d, 1, kRS);
        const bool placed = d.holder == Holder::kStowed && d.st.home[0].role == Role::kPrimary && AwayCount(d) == 0;
        const size_t from = d.events.size();
        Bridge(d);
        const Event* kit = Kept(Last(d, What::kKit, -2, from), 0);
        const bool dOk = placed && d.st.home[0].role == Role::kPrimary && AwayCount(d) == 0 && kit && std::strcmp(kit->otherGun, "AEK-971") == 0 &&
                         Audit(d).empty();
        bool eOk = true;
        std::string kitLine = "-", lost = "-";
        if (all) {
            World we;
            Sim e = Mortar(&we, true);
            const int aek2 = we.AddPick(0xEA2, 1.0f, 0, kAek, 30, 90, false, "AEK-971");
            Grip(e, 1);
            PutOn(e, 1, kRS);
            second::Front(e, 1);
            TakeUp(e, we, aek2);
            e.Run(0.3f);
            const Away* a = AwayOf(e, kScar);
            const bool scarAway = a && a->entity == 0xEA2 && a->role == Role::kPrimary;
            const size_t eFrom = e.events.size();
            Bridge(e);
            const Event* k = Kept(Last(e, What::kKit, -2, eFrom), 1);
            kitLine = LineIn(e, k);
            const Away* still = AwayOf(e, kScar);
            const bool kept = e.st.home[0].role == Role::kNone && still && still->entity == 0xEA2 && still->role == Role::kPrimary && k &&
                              k->why == Why::kTaken && std::strcmp(k->takerGun, "SCAR-H") == 0 && CountFrom(e, What::kLost, eFrom) == 0 && Audit(e).empty();
            Grip(e, 1);
            const size_t pFrom = e.events.size();
            PutOn(e, 1, kRS);
            const Event* l = Kept(Last(e, What::kLost, -2, pFrom), 2);
            lost = LineIn(e, l);
            eOk = scarAway && kept && e.holder == Holder::kStowed && e.st.home[0].role == Role::kPrimary && AwayCount(e) == 0 && l &&
                  l->why == Why::kTaken && Audit(e).empty();
        }
        Report(all ? "KG4k" : "KG4", all ? "Guillotine with every gun held, the bridge's kit after a floor gun: (d) as KG4; (e) the SCAR-H away in the "
                                           "AEK-971's pickup at the right shoulder, the AEK-971 never placed: the kit's M4A1 on no shoulder (said), the "
                                           "SCAR-H's still; put there by hand, the M4A1 has it (the SCAR-H on none, said)"
                                         : "Guillotine, the bridge's kit after a floor gun: (d) the AEK-971 taken with the SCAR-H the game's weapon (no "
                                           "record), put on the right shoulder by hand: the kit's M4A1 has its right shoulder, the AEK-971 gone with the kit",
               dOk && eOk, "(d) placed %d, the M4A1 %s, away %d (\"%s\"); (e) %d (\"%s\" | \"%s\")", placed, RoleName(d.st.home[0].role), AwayCount(d),
               LineIn(d, kit).c_str(), eOk, kitLine.c_str(), lost.c_str());
        if (!(dOk && eOk))
            std::printf("%s\n", Said(d).c_str());
    }
    {
        // A death after the bridge: a later start point's kits in its first seconds, SP_Bank then SP_005: the M4A1 the right shoulder's, the 870 the
        // left's, nothing away, nothing said of the SCAR-H; a start point before the bridge (SP_Bank alone): the SCAR-H the right's, the 870 the left's.
        Sim s = Mortar(nullptr, false);
        Bridge(s);
        Sim e = s;
        NewLife(s.st);
        NoSlots(s);
        Gun(s, 0, 0x5CA1, kScar, "SCAR-H", false);
        Gun(s, 1, 0x8701, k870, "870 MCS", false);
        Gear(s, 6, 0x6721, kM67, "M67");
        Gear(s, 7, 0xC016, kKnife, "Knife");
        Draws(s, 0);
        s.Run(0.3f);
        const size_t from = s.events.size();
        Gun(s, 0, 0x4A31, kM4Kind, "M4A1", true);
        Draws(s, 0, true);
        s.Run(1.5f);
        const bool later = s.st.home[0].role == Role::kPrimary && s.st.home[1].role == Role::kSecondary && AwayCount(s) == 0 &&
                           CountFrom(s, What::kAway, from) == 0 && Audit(s).empty();
        NewLife(e.st);
        NoSlots(e);
        Gun(e, 0, 0x5CA2, kScar, "SCAR-H", false);
        Gun(e, 1, 0x8702, k870, "870 MCS", false);
        Gear(e, 7, 0xC017, kKnife, "Knife");
        Draws(e, 0);
        e.Run(1.5f);
        const bool earlier = e.st.home[0].role == Role::kPrimary && e.st.home[1].role == Role::kSecondary && Audit(e).empty();
        Report("KG5", "Guillotine, a death: a start point after the bridge (SP_Bank then SP_005 in its first seconds): the M4A1 the right shoulder's, the "
                      "870 the left's, nothing away or said of the SCAR-H; one before the bridge: the SCAR-H the right's, the 870 the left's",
               later && earlier, "after: the M4A1 %s, the 870 %s, away %d; before: the SCAR-H %s, the 870 %s", RoleName(s.st.home[0].role),
               RoleName(s.st.home[1].role), AwayCount(s), RoleName(e.st.home[0].role), RoleName(e.st.home[1].role));
    }
    {
        // The bathroom: the knife forced into the hand (slot 7) for the fist fight, free hands asleep through it. The M4A1 held: the D-pad after (no change of
        // gun for the hands: the M4A1 was the last they knew), in the right hand, nothing said of shoulders; put away before: the game's own switch back
        // (no pick) leaves it at its place (hands.cpp: "still put away at its place"), drawn there.
        bool ok[2] = {};
        std::string why[2];
        for (int v = 0; v < 2; ++v) {
            Sim s = Mortar(nullptr, false);
            Bridge(s);
            Grip(s, 1);
            if (v == 1)
                PutOn(s, 1, kRS);
            const size_t from = s.events.size();
            Draws(s, 7);
            s.frozen = true;
            s.Run(13.0f);
            s.frozen = false;
            s.Run(1.0f);
            if (v == 0) {
                s.picked = true, s.pickSlot = 0;
                s.Step();
                Draws(s, 0, false, false);
                s.Run(0.5f);
                ok[v] = s.holder == Holder::kHeld && s.trigger == 1;
            } else {
                Draws(s, 0, false, false);
                s.holder = Holder::kStowed, s.trigger = -1;  // (hands.cpp's g_sleptStowedId: the same gun, no pick)
                s.Run(0.3f);
                ok[v] = merge::DrawnCaseA(s, 1, kRS);
            }
            ok[v] = ok[v] && s.st.home[0].role == Role::kPrimary && s.st.home[1].role == Role::kSecondary && s.st.home[7].role == Role::kNone &&
                    Shoulders(s, from) == 0 && Audit(s).empty();
            why[v] = Audit(s) + Said(s, from);
        }
        Report("KG6", "Guillotine's bathroom: the knife forced (slot 7) with the M4A1 held, then the D-pad: in the right hand; put away before, the game's "
                      "switch back: at its place, drawn there; the M4A1 the right shoulder's, the 870 the left's, nothing said of shoulders",
               ok[0] && ok[1], "held %d, put away %d%s%s", ok[0], ok[1], ok[0] ? "" : why[0].c_str(), ok[1] ? "" : why[1].c_str());
    }
    for (int all = 0; all < 2; ++all) {
        // The Javelin (the plaza's dead Javelin team: the level's own pickup naming slot 5, empty: nothing goes into it), the M4A1 put away at the right
        // shoulder: the Javelin into the right hand, on no shoulder (all: put away by hand behind the neck, the 870 at the left shoulder); the three tanks;
        // UsHumanSpSoldier_SP_Bank_NoJavelin empties slot 5 with the Javelin put away: nothing drawn from the empty slot, the game's switch to the M4A1
        // keeps it at its place, the hands empty (today: the Javelin drawn the game's way, the M4A1 back in the right hand); nothing lost or away.
        World w;
        Sim s = Mortar(&w, all != 0);
        Bridge(s);
        Grip(s, 1);
        PutOn(s, 1, kRS);
        const int jav = w.AddPick(0xF1, 1.0f, 5, kJavelin, 1, 2, true, "Javelin");
        TakeUp(s, w, jav);
        s.slots[5].known = all != 0;
        Hands(s);
        s.Run(0.3f);
        const bool taken = s.slots[5].kind == kJavelin && s.st.home[5].role == Role::kNone;
        bool placed = true;
        if (all) {
            Grip(s, 1);
            PutOn(s, 1, kNeckAt);
            placed = s.holder == Holder::kStowed && s.st.home[5].role == Role::kLauncher && PlaceNow(s, 5) == Place::kNeck;
        }
        s.Run(60.0f);
        const int draws = s.draws;
        const size_t from = s.events.size();
        Gone(s, 5);
        s.Run(0.3f);
        const bool nothing = s.draws == draws && CountFrom(s, What::kToHand, from) == 0;
        const ArriveOut o = Draws(s, 0, false, all != 0);
        s.Run(0.5f);
        const bool after = all ? o.arrival == Arrival::kAtPlace && s.holder == Holder::kStowed : s.holder == Holder::kHeld && s.trigger == 1;
        const std::string audit = Audit(s);
        const bool pass = taken && placed && nothing && after && s.st.home[0].role == Role::kPrimary && s.st.home[1].role == Role::kSecondary &&
                          PlaceNow(s, 5) == Place::kNone && AwayCount(s) == 0 && audit.empty();
        Report(all ? "KG7k" : "KG7", all ? "Guillotine with every gun held, the Javelin: put away by hand behind the neck (the 870 at the left shoulder); the "
                                           "NoJavelin kit empties its slot: nothing drawn from the empty slot, the game's switch keeps the M4A1 at its place"
                                         : "Guillotine, the Javelin (the level's pickup into the empty slot 5, the M4A1 put away): on no shoulder, drawn the "
                                           "game's way; the NoJavelin kit empties its slot: the M4A1 back in the right hand, nothing lost or away",
               pass, "taken %d, placed %d, nothing from the empty slot %d, after %d (arrival %d, holder %d); the M4A1 %s, the 870 %s; %s", taken, placed,
               nothing, after, static_cast<int>(o.arrival), static_cast<int>(s.holder), RoleName(s.st.home[0].role), RoleName(s.st.home[1].role),
               audit.c_str());
        if (!pass)
            std::printf("%s\n", Said(s, from).c_str());
    }
}

// ---- Comrades (SP_Paris) ----

void Comrades() {
    for (int all = 0; all < 2; ++all) {
        // SP_Dima_Sp_06 at a new life: the AS Val (slot 0) the right shoulder's, the AKS-74u (1) the left's, the M67 and the machete (7) none. The wire cut:
        // the game forces slot 7 (the machete) for the cut, then slot 0 (the AS Val: "whatever the player held, the game puts the AS Val back"). With the
        // AKS-74u in the hands, and put away: it stays the left shoulder's, hanging there after; nothing said of shoulders; nothing away.
        bool ok[2] = {};
        std::string why[2];
        Role asVal = Role::kNone, aks = Role::kNone;
        for (int v = 0; v < 2; ++v) {
            Sim s;
            s.set = KitSet();
            NoSlots(s);
            Gun(s, 0, 0xA5A0, kAsVal, "AS Val", all != 0);
            Gun(s, 1, 0x17B0, kAks, "AKS-74u", true);
            Gear(s, 6, 0x6730, kM67, "M67");
            Gear(s, 7, 0xC030, kMachete, "Machete");
            Draws(s, 0);
            s.Run(0.5f);
            asVal = s.st.home[0].role, aks = s.st.home[1].role;
            s.Run(20.0f);
            s.picked = true, s.pickSlot = 1;
            s.Step();
            Draws(s, 1);
            Grip(s, 1);
            if (v == 1)
                PutOn(s, 1, kLS);
            const size_t from = s.events.size();
            s.frozen = true;
            Draws(s, 7);
            s.Run(4.7f);
            Draws(s, 0, true);
            s.Run(0.5f);
            s.frozen = false;
            s.Run(1.0f);
            ok[v] = asVal == Role::kPrimary && aks == Role::kSecondary && s.st.home[7].role == Role::kNone && s.st.home[1].role == Role::kSecondary &&
                    Hung(s, 1) == Place::kLeftShoulder && AwayCount(s) == 0 && Shoulders(s, from) == 0 && Audit(s).empty();
            why[v] = Audit(s) + Said(s, from);
        }
        Report(all ? "KC1k" : "KC1", "Comrades: the kit (the AS Val the right shoulder's, the AKS-74u the left's, the machete none); the wire cut forces "
                                     "the machete, then the AS Val: the AKS-74u held or put away stays the left shoulder's, hanging there; nothing said, nothing away",
               ok[0] && ok[1], "the AS Val %s, the AKS-74u %s; held %d, put away %d%s%s", RoleName(asVal), RoleName(aks), ok[0], ok[1],
               ok[0] ? "" : why[0].c_str(), ok[1] ? "" : why[1].c_str());
    }
}

// ---- Night Shift (SP_Sniper) ----

Sim Sniper(bool all) {
    Sim s;
    s.set = KitSet();
    NoSlots(s);
    Gun(s, 0, 0x40A0, kM40, "M40A5", all);
    Gun(s, 1, 0x7A70, kMp7, "MP7", all);
    Gear(s, 7, 0xC040, kKnife, "Knife");
    Draws(s, 0);
    s.Run(0.5f);
    s.Run(30.0f);
    if (all)
        Grip(s, 1);
    return s;
}

void NightShift() {
    for (int all = 0; all < 2; ++all) {
        // The kit: the M40A5 the right shoulder's, the MP7 the left's, the knife none. The rocket apartment: the knife forced (slot 7) with the M40A5 held,
        // the 13 s struggle, then the MP7 by the D-pad for the escape: the M40A5 hangs at the right shoulder, the MP7 in the right hand (all), nothing said.
        Sim s = Sniper(all != 0);
        const bool kit = s.st.home[0].role == Role::kPrimary && s.st.home[1].role == Role::kSecondary && s.st.home[7].role == Role::kNone;
        const size_t from = s.events.size();
        Draws(s, 7);
        s.frozen = true;
        s.Run(13.0f);
        s.frozen = false;
        s.picked = true, s.pickSlot = 1;
        s.Step();
        const ArriveOut o = Draws(s, 1);
        s.Run(0.5f);
        const bool hands = all ? o.arrival != Arrival::kAtPlace && s.holder == Holder::kHeld && s.trigger == 1 : true;
        const std::string audit = Audit(s);
        const bool pass = kit && hands && Hung(s, 0) == Place::kRightShoulder && s.st.home[1].role == Role::kSecondary && Shoulders(s, from) == 0 &&
                          audit.empty();
        Report(all ? "KN1k" : "KN1", "Night Shift: the kit (the M40A5 the right shoulder's, the MP7 the left's, the knife none); the knife forced in the rocket "
                                     "apartment with the M40A5 held, then the MP7 by the D-pad: the M40A5 hangs at the right shoulder; nothing said of shoulders",
               pass, "kit %d, the MP7 into the hand %d, the M40A5 hangs %s; %s", kit, hands, Name(Hung(s, 0)), audit.c_str());
        if (!pass)
            std::printf("%s\n", Said(s, from).c_str());
    }
    {
        // Every gun held: the sewer's SelectWeapon2 (the game's own key, the M40A5 not the MP7) and the SUV's crash (the game's slot 1): twice. The M40A5
        // in the hands: back at the right shoulder (said), the MP7 into the right hand; the M40A5 put away: the MP7 at its place (the game switched by
        // itself), the hands empty, drawn at the left (in the hand at once, no key).
        bool ok[2] = {};
        std::string why[2];
        for (int v = 0; v < 2; ++v) {
            Sim s = Sniper(true);
            if (v == 1)
                PutOn(s, 1, kRS);
            const size_t from = s.events.size();
            const ArriveOut o = Draws(s, 1);
            s.Run(0.5f);
            if (v == 0) {
                const Event* home = Last(s, What::kBackHome, -2, from);
                ok[v] = o.arrival == Arrival::kToday && s.holder == Holder::kHeld && s.trigger == 1 && home && home->slot == 0 &&
                        home->place == Place::kRightShoulder && Hung(s, 0) == Place::kRightShoulder;
            } else {
                ok[v] = o.arrival == Arrival::kAtPlace && o.event.what == What::kCameAway && s.holder == Holder::kStowed && Hung(s, 1) == Place::kLeftShoulder &&
                        merge::DrawnCaseA(s, 1, kLS);
            }
            ok[v] = ok[v] && s.st.home[0].role == Role::kPrimary && s.st.home[1].role == Role::kSecondary && AwayCount(s) == 0 && Audit(s).empty();
            why[v] = Audit(s) + Said(s, from);
        }
        Report("KN2k", "Night Shift with every gun held, the game's own switch to the MP7 (the sewer's SelectWeapon2, the SUV's crash): the M40A5 held: back "
                       "at the right shoulder (said), the MP7 into the right hand; put away: the MP7 at its place, drawn at the left",
               ok[0] && ok[1], "held %d, put away %d%s%s", ok[0], ok[1], ok[0] ? "" : why[0].c_str(), ok[1] ? "" : why[1].c_str());
    }
    for (int all = 0; all < 2; ++all) {
        // The mall: UsHumanSpSoldier_SP_Sniper_Claymores at its save (the claymore added in slot 4 and selected) and again at a load (a new life): the
        // claymore at no holster (gear), the M40A5 and the MP7 their shoulders' still; nothing away or said.
        Sim s = Sniper(all != 0);
        const size_t from = s.events.size();
        Gear(s, 4, 0xC050, kClaymore, "M18 Claymore");
        Draws(s, 4);
        s.Run(2.0f);
        const bool save = s.st.home[4].role == Role::kNone && s.st.home[0].role == Role::kPrimary && s.st.home[1].role == Role::kSecondary &&
                          Hung(s, 0) == Place::kRightShoulder && AwayCount(s) == 0 && CountFrom(s, What::kKit, from) == 0 && Shoulders(s, from) == 0;
        NewLife(s.st);
        NoSlots(s);
        Gun(s, 0, 0x40A1, kM40, "M40A5", all != 0);
        Gun(s, 1, 0x7A71, kMp7, "MP7", all != 0);
        Gear(s, 7, 0xC041, kKnife, "Knife");
        Gear(s, 4, 0xC051, kClaymore, "M18 Claymore");
        Draws(s, 4);
        s.Run(1.5f);
        const bool load = s.st.home[4].role == Role::kNone && s.st.home[0].role == Role::kPrimary && s.st.home[1].role == Role::kSecondary && Audit(s).empty();
        Report(all ? "KN3k" : "KN3", "Night Shift's mall: the claymore kit at its save (slot 4 added and selected) and at a load: the claymore at no holster, "
                                     "the M40A5 and the MP7 their shoulders' still, nothing away or said",
               save && load, "the save %d, the load %d%s", save, load, save && load ? "" : Said(s, from).c_str());
    }
}

// ---- Kaffarov (SP_Villa) ----

// The HALO kit at a new life (SP_Dima_Sp_10_HALO: the PP-2000 (0), the machete (7)), the plane, the fall, the landing's guards; then the drogue's kit by
// script (SP_Dima_Sp_10: the AEK-971 (0) with PK-A and its light, the Saiga 12K (1), the M67 (6), the MP-443 (9), the machete), no scene; past the
// take's window. stowed: the PP-2000 put away at the right shoulder before (all).
Sim Drogue(World* w, bool all, bool stowed, size_t* kitFrom = nullptr, Role* pp = nullptr, ArriveOut* arrival = nullptr) {
    Sim s;
    s.set = KitSet();
    if (w)
        Hook(s, *w);
    NoSlots(s);
    Gun(s, 0, 0x2000A, kPp2000, "PP-2000", all);
    Gear(s, 7, 0xC031, kMachete, "Machete");
    Draws(s, 0);
    s.Run(0.5f);
    if (pp)
        *pp = s.st.home[0].role;
    s.Run(100.0f);
    if (all)
        Grip(s, 1);
    if (all && stowed)
        PutOn(s, 1, kRS);
    if (kitFrom)
        *kitFrom = s.events.size();
    Gun(s, 0, 0xAE90, kAek, "AEK-971", true);
    Gun(s, 1, 0x5A10, kSaiga, "Saiga 12K", all);
    Gear(s, 6, 0x6740, kM67, "M67");
    Gun(s, 9, 0x4430, kMp443, "MP-443", false);
    s.Step();
    s.gameSlot = 0;
    const bool slept = !s.on;
    Hands(s);
    if (s.on && slept)
        s.holder = Holder::kHeld, s.trigger = 1;  // (awake with the AEK-971 from the PP-2000 the game's way: the grasp's clear, the right hand)
    const ArriveOut o = s.on ? s.Switch(0) : ArriveOut{};
    if (arrival)
        *arrival = o;
    s.Run(1.5f);
    return s;
}

void Kaffarov() {
    for (int all = 0; all < 2; ++all) {
        // The drogue's kit: the AEK-971 has the PP-2000's right shoulder, the Saiga 12K the left's, the M67, the MP-443 (slot 9) and the machete none;
        // nothing away (the PP-2000 gone with the kit, named so); the PP-2000 in the hands: the AEK-971 into the right hand; (all) put away: at its place,
        // drawn there.
        bool ok[2] = {};
        std::string why[2];
        for (int v = 0; v < 1 + all; ++v) {
            size_t from = 0;
            Role pp = Role::kNone;
            ArriveOut o;
            Sim s = Drogue(nullptr, all != 0, v == 1, &from, &pp, &o);
            const Event* aek = nullptr;
            for (size_t i = from; i < s.events.size(); ++i)
                if (s.events[i].what == What::kKit && s.events[i].slot == 0)
                    aek = &s.events[i];
            ok[v] = pp == Role::kPrimary && s.st.home[0].role == Role::kPrimary && s.st.home[1].role == Role::kSecondary && s.st.home[6].role == Role::kNone &&
                    s.st.home[7].role == Role::kNone && s.st.home[9].role == Role::kNone && AwayCount(s) == 0 && CountFrom(s, What::kAway, from) == 0 &&
                    CountFrom(s, What::kKit, from) == 2 && aek && std::strcmp(aek->otherGun, "PP-2000") == 0 && Audit(s).empty();
            if (v == 0)
                ok[v] = ok[v] && s.holder == Holder::kHeld && s.trigger == 1;
            else
                ok[v] = ok[v] && o.arrival == Arrival::kAtPlace && s.holder == Holder::kStowed && merge::DrawnCaseA(s, 1, kRS);
            why[v] = Audit(s) + Said(s, from);
        }
        Report(all ? "KK1k" : "KK1", all ? "Kaffarov with every gun held, the drogue's kit: as KK1; the PP-2000 put away before: the AEK-971 at its place "
                                           "(the right shoulder), drawn there"
                                         : "Kaffarov, the drogue's kit by script: the AEK-971 has the PP-2000's right shoulder (the PP-2000 gone with the kit, "
                                           "never at a shoulder after), the Saiga 12K the left's, the M67, the MP-443 and the machete none; in the right hand",
               ok[0] && (!all || ok[1]), "in the hands %d, put away %d%s%s", ok[0], ok[1], ok[0] ? "" : why[0].c_str(),
               !all || ok[1] ? "" : why[1].c_str());
    }
    {
        // The SUV's ride: the scene sets the game's weapon to the MP-443 (slot 9) and back, twice, the AEK-971 held before: free hands asleep in between,
        // the AEK-971 in the right hand after (the same gun the hands knew: no arrival); its right shoulder kept, the Saiga's left; nothing said.
        Sim s = Drogue(nullptr, false, false);
        Grip(s, 1);
        const size_t from = s.events.size();
        s.frozen = true;
        for (int twice = 0; twice < 2; ++twice) {
            Draws(s, 9);
            s.Run(3.0f);
            Draws(s, 0, false, false);
            s.Run(3.0f);
        }
        s.frozen = false;
        s.Run(0.5f);
        const std::string audit = Audit(s);
        const bool pass = s.holder == Holder::kHeld && s.trigger == 1 && s.st.home[0].role == Role::kPrimary && s.st.home[1].role == Role::kSecondary &&
                          s.st.home[9].role == Role::kNone && Shoulders(s, from) == 0 && audit.empty();
        Report("KK2", "Kaffarov's SUV ride: the scene's MP-443 (slot 9) twice and back, the AEK-971 held before: in the right hand after, its right shoulder "
                      "kept, the Saiga 12K's left; nothing said of shoulders",
               pass, "holder %d by %d, the AEK-971 %s, the Saiga %s; %s", static_cast<int>(s.holder), s.trigger, RoleName(s.st.home[0].role),
               RoleName(s.st.home[1].role), audit.c_str());
        if (!pass)
            std::printf("%s\n", Said(s, from).c_str());
    }
    for (int all = 0; all < 2; ++all) {
        // The pool house: SP_Dima_Sp_10_Sniper by a zone at the door (the M82A3 into slot 0 for the AEK-971), no scene: the M82A3 has the AEK-971's right
        // shoulder, the AEK-971 gone with the kit (nothing away). The disarm fight, the overwatch; then the pickup that gives the AEK-971 back (it names slot
        // 0). The M82A3 in the hands: it goes into that pickup (no record); (all) put away at the right shoulder: away in that pickup at the right
        // shoulder, the AEK-971 into the hand that took it, on no shoulder; put on the right shoulder by hand: the M82A3 on no shoulder (said). One AEK-971
        // either way, never two.
        bool ok[2] = {};
        std::string why[2];
        for (int v = 0; v < 1 + all; ++v) {
            World w;
            Sim s = Drogue(&w, all != 0, false);
            Grip(s, 1);
            if (v == 1)
                PutOn(s, 1, kRS);
            const size_t from = s.events.size();
            Gun(s, 0, 0x82A0, kM82, "M82A3", all != 0);
            s.Step();
            if (v == 0) {
                Draws(s, 0);
            } else {
                s.gameSlot = 0;
                Hands(s);
                if (s.on)
                    s.Switch(0);
            }
            s.Run(1.5f);
            const Event* kit = Kept(Last(s, What::kKit, -2, from), 2);
            const bool sniper = s.st.home[0].role == Role::kPrimary && AwayCount(s) == 0 && kit && std::strcmp(kit->otherGun, "AEK-971") == 0 &&
                                (v == 0 || s.holder == Holder::kStowed);
            s.frozen = true;
            s.Run(10.0f);
            s.frozen = false;
            s.Run(60.0f);
            const int give = w.AddPick(0xEA5, 1.0f, 0, kAek, 30, 120, false, "AEK-971");
            if (!s.on)
                s.holder = Holder::kHeld, s.trigger = 1;
            TakeUp(s, w, give);
            s.Run(0.3f);
            const Away* m82 = AwayOf(s, kM82);
            const bool took = s.st.home[0].role == Role::kNone && s.slots[0].kind == kAek && w.pick[give].gun.kind == kM82 &&
                              (v == 0 ? m82 == nullptr : m82 && m82->entity == 0xEA5 && m82->role == Role::kPrimary);
            Grip(s, 1);
            PutOn(s, 1, kRS);
            const Event* lost = Last(s, What::kLost, -2, from);
            const bool placed = s.holder == Holder::kStowed && s.st.home[0].role == Role::kPrimary && AwayCount(s) == 0 &&
                                (v == 0 || (lost && lost->why == Why::kTaken && !lost->kit)) && second::Copies(s, w, kAek) == 1;
            ok[v] = sniper && took && placed && Audit(s).empty();
            why[v] = Audit(s) + Said(s, from);
        }
        Report(all ? "KK3k" : "KK3", all ? "Kaffarov with every gun held, the pool house: as KK3; the M82A3 put away when the AEK-971's pickup is taken: away "
                                           "in it at the right shoulder, on no shoulder once the AEK-971 is put there (said)"
                                         : "Kaffarov's pool house: the sniper kit by a zone (the M82A3 for the AEK-971): the M82A3 has the right shoulder, "
                                           "nothing away; the AEK-971's pickup taken with the M82A3 in the hands: it goes into the pickup, the AEK-971 on no "
                                           "shoulder till put there by hand; one AEK-971",
               ok[0] && (!all || ok[1]), "in the hands %d, put away %d%s%s", ok[0], ok[1], ok[0] ? "" : why[0].c_str(), !all || ok[1] ? "" : why[1].c_str());
    }
    for (int all = 0; all < 2; ++all) {
        // The villa's main room: the game refuses a change of weapon till the player's shot. The AEK-971 put away and drawn back (in the hand at once, no
        // key); a squeeze at the left shoulder: (all) the Saiga's key, the game never switches: the key again at 0.6 s, given up at 1.5 s (said, felt),
        // the hand empty; (today) the Saiga drawn the game's way: refused (the hands don't know it); both guns at their places, nothing lost.
        Sim s = Drogue(nullptr, all != 0, false);
        Grip(s, 1);
        PutOn(s, 1, kRS);
        const bool back = merge::DrawnCaseA(s, 1, kRS);
        Grip(s, 1);
        PutOn(s, 1, kRS);
        const size_t from = s.events.size();
        const int keys = s.Keys();
        const float at = s.time;
        SqueezeAt(s, 0, kLS, 2.0f);
        const Event* e = all ? Last(s, What::kNoSwitch, 0, from) : Last(s, What::kRefused, 0, from);
        const bool refused = all ? e && e->twice && s.Keys(1) == 2 && s.Keys() == keys + 2 : e && e->why == Why::kUnknown && s.Keys() == keys;
        int doubles = 0;  // (the refusal's double click: two short pulses, felt)
        for (const Buzzed& b : s.buzzes)
            doubles += b.side == 0 && b.at >= at && std::fabs(b.seconds - kDoubleSeconds) < 1e-4f;
        const std::string audit = Audit(s);
        const bool pass = back && refused && s.holder == Holder::kStowed && s.gameSlot == 0 && Hung(s, 0) == Place::kRightShoulder &&
                          Hung(s, 1) == Place::kLeftShoulder && doubles == 2 && audit.empty();
        Report(all ? "KK4k" : "KK4", all ? "Kaffarov's main room with every gun held (weapon change locked): the AEK-971 put away and drawn back, no key; the "
                                           "Saiga 12K's key twice, never answered: given up (said, felt), both guns at their places"
                                         : "Kaffarov's main room (weapon change locked): the AEK-971 put away and drawn back, no key; a squeeze at the left "
                                           "shoulder refused (the Saiga 12K drawn the game's way), felt; both guns at their places",
               pass, "drawn back %d, at the left: %s; the AEK-971 %s, the Saiga %s; %s", back, LineIn(s, e).c_str(), Name(Hung(s, 0)), Name(Hung(s, 1)),
               audit.c_str());
        if (!pass)
            std::printf("%s\n", Said(s, from).c_str());
    }
    {
        // The pool: SP_Emperor_Beatup takes every gun (the slots empty): nothing at a shoulder, nothing away; Blackburn's part (a new soldier, a new life,
        // UsHumanSpSoldier_SP_Villa_Blackburn: the M416 with its ACOG, slot 0, the M320 drawn with no slot of its own): another level's kit by its
        // slots, the M416 the right shoulder's.
        Sim s = Drogue(nullptr, false, false);
        const size_t from = s.events.size();
        for (int k = 0; k < kSlots; ++k)
            Gone(s, k);
        s.frozen = true;
        s.Run(5.0f);
        bool none = AwayCount(s) == 0 && Shoulders(s, from) == 0;
        for (int k = 0; k < kSlots; ++k)
            none = none && PlaceNow(s, k) == Place::kNone;
        NewLife(s.st);
        Gun(s, 0, 0x4169, kM416Kind, "M416", true);
        s.frozen = false;
        Draws(s, 0, true);
        s.Run(1.5f);
        const bool blackburn = s.st.home[0].role == Role::kPrimary && s.st.memoForgot && Audit(s).empty();
        Report("KK5", "Kaffarov's pool: every gun taken, nothing at a shoulder or away; Blackburn's part (a new life, the M416 alone): another level's kit, "
                      "the M416 the right shoulder's",
               none && blackburn, "none %d, Blackburn %d (the M416 %s, memo forgot %d)", none, blackburn, RoleName(s.st.home[0].role), s.st.memoForgot);
    }
}

// ---- The Great Destroyer (SP_Finale) ----

// The spawn kit at a new life (UsHumanSpSoldier_SP_Finale: the M9 in slot 0 behind the cuffs, the handcuffs in 7, selected by the script): free hands
// asleep (the cuffs; the M9 the train pistol's own rig after); the escape, the street; the train jump's kit again (the M9 and the cuffs given back, new
// objects of the same kinds); on the train with the M9 drawn (the train pistol: free hands asleep).
Sim Escape(World* w) {
    Sim s;
    s.set = KitSet();
    if (w)
        Hook(s, *w);
    NoSlots(s);
    Gun(s, 0, 0x9A90, kM9, "M9", false);
    Gear(s, 7, 0xC040, kCuffs, "Handcuffs");
    Draws(s, 7);
    s.Run(0.5f);
    s.Run(60.0f);
    s.frozen = true;
    Gun(s, 0, 0x9A91, kM9, "M9", false);
    Gear(s, 7, 0xC041, kCuffs, "Handcuffs");
    s.Run(2.0f);
    s.frozen = false;
    Draws(s, 0);
    s.Run(5.0f);
    return s;
}
// The kick won: _AKs74u_Only (an AKS-74u in slot 0 and another in slot 1, the knife in 7, the cuffs gone) in the kick's scene, its first draw at its end;
// the jammed door's scene and the kick's guard before it (a take's window is long over: a gun a kit gives within 1 s of a take in its slot is the take's).
void Kick(Sim& s) {
    s.Run(3.0f);
    s.frozen = true;
    s.Run(1.0f);
    Gun(s, 0, 0x17C0, kAks, "AKS-74u", true);
    Gun(s, 1, 0x17C1, kAks, "AKS-74u", true);
    Gear(s, 7, 0xC042, kKnife, "Knife");
    s.Step();
    Draws(s, 0, true);
    s.Run(0.5f);
    s.frozen = false;
    s.Run(1.5f);
}

void Destroyer() {
    {
        // The spawn kit: the M9 the right shoulder's by its slot (the train pistol's: free hands asleep with it), the cuffs none; the train jump's kit gives
        // both back: read afresh, as they were, nothing said. The jammed door's M1014 (the level's pickup naming slot 0): (a) taken with the M9 in the
        // hands: the M9 into the pickup (no record), the M1014 in the right hand, on no shoulder; (b) not taken: the _Shotgun kit forces one into slot 0:
        // the kit's, the M9's right shoulder.
        World w;
        Sim a = Escape(&w);
        const bool spawn = a.st.home[0].role == Role::kPrimary && a.st.home[7].role == Role::kNone && CountFrom(a, What::kKit, 0) == 0 &&
                           CountFrom(a, What::kAway, 0) == 0;
        const int door = w.AddPick(0xD10, 1.0f, 0, kM1014, 7, 25, false, "M1014");
        TakeUp(a, w, door);
        a.Run(0.5f);
        const bool taken = a.st.home[0].role == Role::kNone && a.slots[0].kind == kM1014 && AwayCount(a) == 0 && w.pick[door].gun.kind == kM9 &&
                           a.holder == Holder::kHeld && a.trigger == 1;
        Sim b = Escape(nullptr);
        const size_t from = b.events.size();
        Gun(b, 0, 0x1014B, kM1014, "M1014", true);
        b.Step();
        Draws(b, 0, true);
        b.Run(1.5f);
        const Event* kit = Kept(Last(b, What::kKit, -2, from), 0);
        const bool forced = b.st.home[0].role == Role::kPrimary && AwayCount(b) == 0 && kit && std::strcmp(kit->otherGun, "M9") == 0;
        Report("KD1", "The Great Destroyer: the spawn kit (the M9 the right shoulder's by its slot, the cuffs none), the train jump's kit again (as it was); "
                      "the jammed door's M1014 taken with the M9 in the hands (the M9 into the pickup, the M1014 on no shoulder), or forced by the _Shotgun "
                      "kit (the kit's: the M9's right shoulder)",
               spawn && taken && forced && Audit(a).empty() && Audit(b).empty(), "spawn %d, taken %d (the M1014 %s), forced %d (\"%s\")", spawn, taken,
               RoleName(a.st.home[0].role), forced, LineIn(b, kit).c_str());
    }
    {
        // The kick won: _AKs74u_Only gives an AKS-74u in slot 0 and another in slot 1. (a) the M1014 (taken off the floor, never placed) in the hands: slot
        // 0's the right shoulder's (its slot's), slot 1's the left's, slot 0's into the right hand; (b) the M1014 put on the right shoulder by hand: slot
        // 0's has its right shoulder; (c) the M1014 put on the LEFT shoulder: slot 0's has the left, slot 1's on no shoulder (one gun a shoulder, said).
        // Nothing away; the M1014 gone with the kit.
        bool ok[3] = {};
        std::string why[3];
        for (int v = 0; v < 3; ++v) {
            World w;
            Sim s = Escape(&w);
            const int door = w.AddPick(0xD11, 1.0f, 0, kM1014, 7, 25, false, "M1014");
            TakeUp(s, w, door);
            Grip(s, 1);
            if (v == 1)
                PutOn(s, 1, kRS);
            if (v == 2)
                PutOn(s, 1, kLS);
            const size_t from = s.events.size();
            Kick(s);
            const Role want0 = v == 2 ? Role::kSecondary : Role::kPrimary, want1 = v == 2 ? Role::kNone : Role::kSecondary;
            int taken = 0;
            for (size_t i = from; i < s.events.size(); ++i)
                taken += s.events[i].what == What::kKit && s.events[i].why == Why::kTaken;
            ok[v] = s.st.home[0].role == want0 && s.st.home[1].role == want1 && s.holder == Holder::kHeld && s.trigger == 1 && AwayCount(s) == 0 &&
                    CountFrom(s, What::kKit, from) == 2 && taken == (v == 2 ? 1 : 0) && CountFrom(s, What::kAway, from) == 0 && Audit(s).empty();
            why[v] = Audit(s) + Said(s, from);
        }
        Report("KD2", "The Great Destroyer's kick: _AKs74u_Only (two AKS-74u, slots 0 and 1): the M1014 in the hands: the right shoulder's and the left's; "
                      "the M1014 on the right shoulder: slot 0's has it; on the left: slot 0's has the left, slot 1's none (said); nothing away",
               ok[0] && ok[1] && ok[2], "in the hands %d, right %d, left %d%s%s%s", ok[0], ok[1], ok[2], ok[0] ? "" : why[0].c_str(),
               ok[1] ? "" : why[1].c_str(), ok[2] ? "" : why[2].c_str());
    }
    {
        // A death after the kick (TrainrideRoof, SaveBeforeSolomon: their kit the two AKS-74u): each by its slot, slot 0's the right shoulder's, slot 1's
        // the left's (the memo, one word a kind, can't tell them apart).
        World w;
        Sim s = Escape(&w);
        const int door = w.AddPick(0xD12, 1.0f, 0, kM1014, 7, 25, false, "M1014");
        TakeUp(s, w, door);
        Kick(s);
        NewLife(s.st);
        NoSlots(s);
        Gun(s, 0, 0x17C8, kAks, "AKS-74u", true);
        Gun(s, 1, 0x17C9, kAks, "AKS-74u", true);
        Gear(s, 7, 0xC043, kKnife, "Knife");
        Draws(s, 0, true);
        s.Run(1.5f);
        Report("KD3", "The Great Destroyer, a death after the kick: its checkpoint's two AKS-74u each by its slot (the right shoulder's, the left's)",
               s.st.home[0].role == Role::kPrimary && s.st.home[1].role == Role::kSecondary && Audit(s).empty(), "slot 0 %s, slot 1 %s; %s",
               RoleName(s.st.home[0].role), RoleName(s.st.home[1].role), Audit(s).c_str());
    }
    {
        // OPEN at the last door forces slot 1 (the other AKS-74u: one profile, no change of gun for the hands, no arrival). Slot 0's in the hands: back at the
        // right shoulder (said), the hands hold slot 1's now. Slot 0's put away: slot 1's is the game's weapon at the left shoulder, both at their places;
        // a draw at the right (key 1) lands with no arrival: into the hand that drew; put away again, the D-pad's pick of the other: into the right hand.
        World w;
        Sim s = Escape(&w);
        const int door = w.AddPick(0xD13, 1.0f, 0, kM1014, 7, 25, false, "M1014");
        TakeUp(s, w, door);
        Kick(s);
        Grip(s, 1);
        Sim h = s;
        size_t from = h.events.size();
        h.gameSlot = 1;
        h.Run(0.5f);
        const Event* home = Last(h, What::kBackHome, -2, from);
        const bool held = home && home->slot == 0 && home->place == Place::kRightShoulder && h.holder == Holder::kHeld && Hung(h, 0) == Place::kRightShoulder;
        PutOn(s, 1, kRS);
        from = s.events.size();
        s.gameSlot = 1;
        s.Run(0.5f);
        const bool both = s.holder == Holder::kStowed && Hung(s, 1) == Place::kLeftShoulder && Hung(s, 0) == Place::kRightShoulder;
        SqueezeAt(s, 0, kRS);
        const bool key = s.Keys(0) == 1;
        s.gameSlot = 0;  // (the game's switch to slot 0: no arrival, the same profile)
        s.Run(0.3f);
        const bool drawn = key && s.holder == Holder::kHeld && s.trigger == 0;
        s.hand[0].squeeze = 1.0f;
        PutOn(s, 0, kRS);
        s.picked = true, s.pickSlot = 1;
        s.Step();
        s.gameSlot = 1;
        s.Run(0.3f);
        const bool pick = s.holder == Holder::kHeld && s.trigger == 1 && !s.armed[1];
        const std::string audit = Audit(s);
        const bool pass = held && both && drawn && pick && audit.empty();
        Report("KD4", "The Great Destroyer's last door (OPEN forces slot 1, the other AKS-74u): held, slot 0's back at the right shoulder (said); put away, "
                      "both at their places; a draw at the right (key 1) with no arrival into the hand that drew; the D-pad's pick of the other into the right hand",
               pass, "held %d, both at their places %d, drawn %d, picked %d; %s", held, both, drawn, pick, audit.c_str());
        if (!pass)
            std::printf("%s\n", Said(s, from).c_str());
    }
    {
        // PENNSAVE1 after the crash: UsHumanSpSoldier_SP_Finale_SIG (the SG 553 into slot 0, the knife into 7, slot 1 emptied) in the wake's scene: the SG
        // 553 has slot 0's AKS-74u's right shoulder (said), slot 1's AKS-74u gone (no place, no record); an MP7 taken off the floor into slot 1 later:
        // on no shoulder, and no record of the AKS-74u long gone from it. Then MANHOLESAVE's climb: CustomizeSoldierGunOnly (the M9 alone in slot 0,
        // the MP7 put on the left shoulder gone): the M9 has the right shoulder, nothing else at one; the Bossfight's cuffs alone in slot 0: no
        // holster, nothing recorded of the M9.
        World w;
        Sim s = Escape(&w);
        const int door = w.AddPick(0xD14, 1.0f, 0, kM1014, 7, 25, false, "M1014");
        TakeUp(s, w, door);
        Kick(s);
        size_t from = s.events.size();
        s.frozen = true;
        s.Run(10.0f);
        Gun(s, 0, 0x5530, kSg553, "SG 553", false);
        Gone(s, 1);
        Gear(s, 7, 0xC044, kKnife, "Knife");
        s.Step();
        Draws(s, 0, true);
        s.Run(1.0f);
        s.frozen = false;
        s.Run(1.0f);
        const Event* kit = Kept(Last(s, What::kKit, -2, from), 0);
        const bool sewer = s.st.home[0].role == Role::kPrimary && PlaceNow(s, 1) == Place::kNone && AwayCount(s) == 0 && kit &&
                           std::strcmp(kit->otherGun, "AKS-74u") == 0 && CountFrom(s, What::kAway, from) == 0;
        s.Run(30.0f);
        from = s.events.size();
        const int mp7 = w.AddPick(0x7A5, 1.0f, 1, kMp7, 40, 120, false, "MP7");
        TakeUp(s, w, mp7);
        s.slots[1].known = true;
        s.Run(1.5f);
        const bool noGhost = s.st.home[1].role == Role::kNone && AwayCount(s) == 0 && CountFrom(s, What::kAway, from) == 0;
        Grip(s, 1);
        PutOn(s, 1, kLS);
        const bool mp7Left = s.st.home[1].role == Role::kSecondary;
        s.Run(30.0f);
        from = s.events.size();
        s.frozen = true;
        s.Run(2.0f);
        Gun(s, 0, 0x9A92, kM9, "M9", false);
        Gone(s, 1);
        Gone(s, 7);
        s.Step();
        Draws(s, 0, true);
        s.Run(1.5f);
        s.frozen = false;
        bool alone = s.st.home[0].role == Role::kPrimary && AwayCount(s) == 0 && CountFrom(s, What::kAway, from) == 0;
        for (int k = 1; k < kSlots; ++k)
            alone = alone && PlaceNow(s, k) == Place::kNone;
        s.Run(60.0f);
        from = s.events.size();
        s.frozen = true;
        Gear(s, 0, 0xC045, kCuffs, "Handcuffs");
        s.Step();
        Draws(s, 0);
        s.Run(1.5f);
        const bool cuffs = s.st.home[0].role == Role::kNone && AwayCount(s) == 0 && CountFrom(s, What::kKit, from) == 0 && Shoulders(s, from) == 0;
        const std::string audit = Audit(s);
        const bool pass = sewer && noGhost && mp7Left && alone && cuffs && audit.empty();
        Report("KD5", "The Great Destroyer's sewer, climb and Times Square: the SIG kit (the SG 553 has slot 0's right shoulder, slot 1's AKS-74u gone, no "
                      "record); an MP7 into the emptied slot 1: on no shoulder, no record of the AKS-74u; the climb's M9 alone: the right shoulder's, the MP7 "
                      "put on the left gone with the kit; the cuffs alone: no holster, nothing recorded",
               pass, "sewer %d (\"%s\"), no ghost %d, the MP7 left %d, the M9 alone %d, the cuffs %d; %s", sewer, LineIn(s, kit).c_str(), noGhost, mp7Left,
               alone, cuffs, audit.c_str());
        if (!pass)
            std::printf("%s\n", Said(s).c_str());
    }
}

// ---- The lines of a kit's gun, word for word ----

void KitLines() {
    char a[400], b[400], c[400], d[400], g[400], h[400];
    const char* names[kSlots] = {"M4A1", "870 MCS", nullptr, nullptr, nullptr, nullptr, "M67", "Knife", nullptr, nullptr};
    Event e;
    e.what = What::kKit, e.slot = 0, e.place = e.at = Place::kRightShoulder;
    CopyName(e.gun, "M4A1");
    CopyName(e.otherGun, "SCAR-H");
    Say(e, names, a, sizeof(a));
    e = Event{};
    e.what = What::kKit, e.slot = 1, e.place = e.at = Place::kLeftShoulder;
    CopyName(e.gun, "870 MCS");
    Say(e, names, b, sizeof(b));
    const char* twins[kSlots] = {"AKS-74u", "AKS-74u", nullptr, nullptr, nullptr, nullptr, nullptr, "Knife", nullptr, nullptr};
    e = Event{};
    e.what = What::kKit, e.slot = 1, e.at = Place::kLeftShoulder, e.place = Place::kNone, e.why = Why::kTaken, e.other = 0;
    CopyName(e.gun, "AKS-74u");
    Say(e, twins, c, sizeof(c));
    // (an away gun in its pickup keeps its shoulder: the kit's gun on none, naming it)
    e = Event{};
    e.what = What::kKit, e.slot = 0, e.at = Place::kRightShoulder, e.place = Place::kNone, e.why = Why::kTaken, e.other = -1;
    CopyName(e.gun, "M4A1");
    CopyName(e.otherGun, "AEK-971");
    CopyName(e.takerGun, "SCAR-H");
    Say(e, names, d, sizeof(d));
    // ("kitguns none", the default: the gun before named, gone with the kit; the new one on no shoulder)
    const char* kick[kSlots] = {"AKS-74u", "AKS-74u", nullptr, nullptr, nullptr, nullptr, nullptr, "Knife", nullptr, nullptr};
    e = Event{};
    e.what = What::kKit, e.slot = 0;
    CopyName(e.gun, "AKS-74u");
    CopyName(e.otherGun, "M9");
    Say(e, kick, g, sizeof(g));
    // (a gun given back in a launcher's place: another gun the game held from that slot at a shoulder went with the kit)
    e = Event{};
    e.what = What::kLost, e.slot = 0, e.why = Why::kGone, e.weak = true, e.kit = true;
    CopyName(e.gun, "M249");
    CopyName(e.otherGun, "M4A1");
    Say(e, names, h, sizeof(h));
    Report("KL1", "the lines of a script's kit's gun (holster.h's Say), and of a gun gone with a kit's give-back",
           std::strcmp(a, "Holster: the game's kit gave the M4A1 (slot 0) in place of the SCAR-H, gone with it: its holster the right shoulder") == 0 &&
               std::strcmp(b, "Holster: the game's kit gave the 870 MCS (slot 1): its holster the left shoulder") == 0 &&
               std::strcmp(c, "Holster: the game's kit gave the AKS-74u (slot 1): on no shoulder (the left shoulder has the AKS-74u: one gun a shoulder)") == 0 &&
               std::strcmp(d, "Holster: the game's kit gave the M4A1 (slot 0) in place of the AEK-971, gone with it: on no shoulder (the right shoulder has "
                              "the SCAR-H, in its pickup: one gun a shoulder)") == 0 &&
               std::strcmp(g, "Holster: the game's kit gave the AKS-74u (slot 0) in place of the M9, gone with it: on no shoulder") == 0 &&
               std::strcmp(h, "Holster: the M249 went with the game's kit (the M4A1 given back in its slot 0): it's gone, its shoulder free") == 0,
           "\"%s\" | \"%s\" | \"%s\" | \"%s\" | \"%s\" | \"%s\"", a, b, c, d, g, h);
}

// A take of a gun never carried before (no launcher: as a kit's gun would look), its swap told a frame after its gun is seen in its slot (first noted as a
// kit's gift): the holstered M416's record made at the gift (unsaid, kit-gone) gets the take's pickup when the swap comes, as before the kit's rule; the
// AN-94 on no shoulder, its kind's memo none, no kit's line said. (Its "with the game" record never said either, the M416 drawn back from its shoulder.)
void LateTake() {
    World w;
    Sim s = RockWorld(w);
    if (g_kitHolster)
        s.set.kitHolster = true;  // (else the default)
    PutOn(s, 1, kRS);
    second::Front(s, 1, 11.0f);
    const size_t from = s.events.size();
    Gave(s, 0, 0x9403, kAn94Kind);
    CopyName(s.slots[0].name, "AN-94");
    s.Step();
    const Away* first = AwayOf(s, kM416Kind);
    // (a kit's gun for a frame: with "kitguns holster" the M416's shoulder, with "kitguns none" none)
    const bool gone = first && first->kitGone && s.st.home[0].role == (g_kitHolster ? Role::kPrimary : Role::kNone);
    SwapIn in;
    in.changed = 1, in.slot[0] = 0, in.came[0] = kAn94Kind, in.went[0] = kM416Kind, in.wentBlueprint[0] = kM416Kind + kBlueprintOf, in.entity = 0xEC;
    CopyName(in.taken, "AN-94");
    NoteSwap(s.set, s.slots, in, s.st);
    s.taken = 1u << 0;
    s.Run(1.5f);
    const Away* a = AwayOf(s, kM416Kind);
    const Memo* m = MemoOf(s.st, kAn94Kind);
    const Event* said = Kept(Last(s, What::kAway, -2, from), 3);
    const std::string audit = Audit(s);
    const bool pass = gone && a && a->entity == 0xEC && !a->kitGone && a->role == Role::kPrimary && s.st.home[0].role == Role::kNone &&
                      CountFrom(s, What::kKit, from) == 0 && CountFrom(s, What::kAway, from) == 1 && said && !said->weak && m && m->role == Role::kNone &&
                      audit.empty();
    Report(g_kitHolster ? "KX1" : "KX1o", g_kitHolster ? "a take of a gun never carried before told a frame after its gun (first a kit's for a frame): "
                                                         "the holstered M416 away in its pickup after all (said once, as in a pickup), the AN-94 on no "
                                                         "shoulder, its memo none, no kit's line"
                                                       : "as KX1 with \"kitguns none\" (the kit's gun on none for its frame)",
           pass, "a kit's for a frame %d; the M416 %s%s; the AN-94 %s; kit lines %d; \"%s\"; %s", gone, a ? (a->entity ? "in a pickup" : "with the game") : "-",
           a && a->kitGone ? " (kit-gone)" : "", RoleName(s.st.home[0].role), CountFrom(s, What::kKit, from), LineIn(s, said).c_str(), audit.c_str());
    if (!pass)
        std::printf("%s\n", Said(s, from).c_str());
}

// ---- KO: the scenes with the default, "kitguns none" (the A13 lane's settle, 2026-11-25): a script's kit's gun on no shoulder till a hand puts it on
// one, as the owner played Semper Fidelis, The Great Destroyer and Swordbreaker (main's rule since 2026-11-18); the gun it replaced gone with the kit all
// the same: no ghost (until the A13 lane a record "with the game" stayed at its shoulder, refusing every draw there and said gone at the next put-away),
// gear at none, the kit's line naming what went. ----

int Refusals(const Sim& s, size_t from, Why why) {
    int n = 0;
    for (size_t i = from; i < s.events.size(); ++i)
        n += s.events[i].what == What::kRefused && s.events[i].why == why;
    return n;
}

void Owners() {
    const bool was = g_kitHolster;
    g_kitHolster = false;
    {
        // Uprising: the stab's AKS-74u on no shoulder (the knife never at one, nothing named gone: it was gear), in the right hand at the scene's end;
        // put on the right shoulder by hand, drawn back. The Parking Lot's load after a G3A3 took the AKS-74u's right shoulder (KU2's): the kit's
        // AKS-74u on no shoulder (its kind's none), the G3A3 gone with the kit (no record); the control, a save with the AKS-74u on the right: the right's.
        size_t from = 0;
        Role knife = Role::kNone;
        Sim s = Stabbed(nullptr, &knife, &from);
        const Event* kit = Kept(Last(s, What::kKit, -2, from), 0);
        const std::string line = LineIn(s, kit);
        const bool given = knife == Role::kNone && s.st.home[0].role == Role::kNone && s.st.home[7].role == Role::kNone && s.holder == Holder::kHeld &&
                           s.trigger == 1 && AwayCount(s) == 0 && CountFrom(s, What::kKit, from) == 1 && kit && kit->place == Place::kNone &&
                           !kit->otherGun[0] && Audit(s).empty();
        Grip(s, 1);
        PutOn(s, 1, kRS);
        const bool placed = s.holder == Holder::kStowed && s.st.home[0].role == Role::kPrimary;
        const bool back = merge::DrawnCaseA(s, 1, kRS);
        World w;
        Sim p = Stabbed(&w);
        const int g3 = w.AddPick(0xE33, 1.0f, 0, kG3, 20, 80, false, "G3A3");
        Grip(p, 1);
        PutOn(p, 1, kRS);
        second::Front(p, 1);
        TakeUp(p, w, g3);
        p.Run(0.3f);
        Grip(p, 1);
        PutOn(p, 1, kRS);
        NewLife(p.st);
        NoSlots(p);
        Gun(p, 0, 0x63AB, kG3, "G3A3", true);
        Gear(p, 6, 0x6714, kM67, "M67");
        Gear(p, 7, 0xC019, kKnife, "Knife");
        p.holder = Holder::kHeld, p.trigger = 1, p.gameSlot = 0, p.on = true;
        p.Run(0.3f);
        const Role saved = p.st.home[0].role;
        Gun(p, 0, 0x17AD, kAks, "AKS-74u", true);
        p.Run(1.5f);
        Sim c = Stabbed(nullptr);
        Grip(c, 1);
        PutOn(c, 1, kRS);
        NewLife(c.st);
        NoSlots(c);
        Gun(c, 0, 0x17AE, kAks, "AKS-74u", true);
        Gear(c, 7, 0xC01A, kKnife, "Knife");
        c.holder = Holder::kHeld, c.trigger = 1, c.gameSlot = 0;
        c.Run(0.3f);
        Gun(c, 0, 0x17AF, kAks, "AKS-74u", true);
        c.Run(1.5f);
        const bool load = saved == Role::kPrimary && p.st.home[0].role == Role::kNone && AwayCount(p) == 0 && Audit(p).empty() &&
                          c.st.home[0].role == Role::kPrimary && Audit(c).empty();
        Report("KO1", "Uprising, \"kitguns none\": the stab's AKS-74u on no shoulder (the knife none, nothing named gone), in the right hand; put on the "
                      "right shoulder by hand and drawn back; the Parking Lot's load after a G3A3 took its shoulder: on none, the G3A3 gone with the kit; "
                      "the control: the right's",
               given && placed && back && load, "given %d (\"%s\"), placed %d, drawn back %d; the load: the G3A3 %s, then the AKS-74u %s, away %d, %s; the "
                                                "control %s",
               given, line.c_str(), placed, back, RoleName(saved), RoleName(p.st.home[0].role), AwayCount(p), Audit(p).c_str(), RoleName(c.st.home[0].role));
    }
    {
        // Guillotine: the SP_Bank kit's SCAR-H and 870 on no shoulder (the mortar none); the SCAR-H put on the right shoulder by hand; the bridge's M4A1
        // under the fade: on none, the SCAR-H gone with it (named, no record), in the right hand at the scene's end; put on the right shoulder: no line of
        // a SCAR-H losing it (no ghost).
        size_t from = 0;
        Role mortar = Role::kNone;
        Sim s = Mortar(nullptr, true, &mortar, &from);
        const bool kit = mortar == Role::kNone && s.st.home[0].role == Role::kNone && s.st.home[1].role == Role::kNone && CountFrom(s, What::kKit, from) == 2 &&
                         AwayCount(s) == 0 && Audit(s).empty();
        Grip(s, 1);
        PutOn(s, 1, kRS);
        const bool placed = s.holder == Holder::kStowed && s.st.home[0].role == Role::kPrimary;
        const size_t bFrom = s.events.size();
        Bridge(s);
        const Event* m4 = Kept(Last(s, What::kKit, -2, bFrom), 0);
        const std::string line = LineIn(s, m4);
        const bool bridge = s.st.home[0].role == Role::kNone && s.holder == Holder::kHeld && s.trigger == 1 && AwayCount(s) == 0 && m4 &&
                            std::strcmp(m4->otherGun, "SCAR-H") == 0 && m4->place == Place::kNone && CountFrom(s, What::kAway, bFrom) == 0 && Audit(s).empty();
        Grip(s, 1);
        const size_t pFrom = s.events.size();
        PutOn(s, 1, kRS);
        const bool again = s.holder == Holder::kStowed && s.st.home[0].role == Role::kPrimary && CountFrom(s, What::kLost, pFrom) == 0 && Audit(s).empty();
        Report("KO2", "Guillotine, \"kitguns none\": the SP_Bank kit's SCAR-H and 870 on no shoulder; the SCAR-H put on the right shoulder by hand, the "
                      "bridge's M4A1: on none, the SCAR-H gone with it (named, no record), in the right hand; put on the right shoulder: nothing of the "
                      "SCAR-H said",
               kit && placed && bridge && again, "the kit %d, placed %d, the bridge %d (\"%s\"), put on the right %d%s", kit, placed, bridge, line.c_str(),
               again, kit && placed && bridge && again ? "" : Said(s, from).c_str());
    }
    {
        // Kaffarov, the drogue's kit with the PP-2000 put away at the right shoulder: the AEK-971 and the Saiga 12K on no shoulder, the PP-2000 gone with
        // the kit (named, no record), the AEK-971 into the right hand (the game's weapon put away with no home); the pool house's M82A3 for the AEK-971 put
        // on the right shoulder by hand: on none, the AEK-971 gone with it.
        size_t from = 0;
        Role pp = Role::kNone;
        Sim s = Drogue(nullptr, true, true, &from, &pp);
        const Event* aek = nullptr;
        for (size_t i = from; i < s.events.size(); ++i)
            if (s.events[i].what == What::kKit && s.events[i].slot == 0)
                aek = &s.events[i];
        const std::string line = LineIn(s, aek);
        const bool drogue = pp == Role::kPrimary && s.st.home[0].role == Role::kNone && s.st.home[1].role == Role::kNone && s.st.home[9].role == Role::kNone &&
                            AwayCount(s) == 0 && CountFrom(s, What::kAway, from) == 0 && aek && std::strcmp(aek->otherGun, "PP-2000") == 0 &&
                            s.holder == Holder::kHeld && s.trigger == 1 && Audit(s).empty();
        Grip(s, 1);
        PutOn(s, 1, kRS);
        const size_t pFrom = s.events.size();
        Gun(s, 0, 0x82A1, kM82, "M82A3", true);
        s.Step();
        s.gameSlot = 0;
        s.Switch(0);
        s.Run(1.5f);
        const Event* m82 = Kept(Last(s, What::kKit, -2, pFrom), 1);
        const bool pool = s.st.home[0].role == Role::kNone && AwayCount(s) == 0 && m82 && std::strcmp(m82->otherGun, "AEK-971") == 0 &&
                          CountFrom(s, What::kAway, pFrom) == 0 && Audit(s).empty();
        Report("KO3", "Kaffarov, \"kitguns none\": the drogue's kit with the PP-2000 put away at the right shoulder: the AEK-971 and the Saiga 12K on no "
                      "shoulder, the PP-2000 gone with the kit (named), the AEK-971 in the right hand; the pool house's M82A3 for the AEK-971 put on the "
                      "right shoulder: on none, the AEK-971 gone with it",
               drogue && pool, "the drogue %d (\"%s\", holder %d by %d), the pool house %d (\"%s\")%s", drogue, line.c_str(), static_cast<int>(s.holder),
               s.trigger, pool, LineIn(s, m82).c_str(), drogue && pool ? "" : Said(s, from).c_str());
    }
    {
        // Semper Fidelis's and The Great Destroyer's kick as the owner played it: the M9 the spawn kit's right shoulder's in slot 0, the jammed door's
        // M1014 taken into the empty slot 1 (Semper Fidelis: "its slot forced"), the kick's two AKS-74u: both on no shoulder, the M9 gone with the kit
        // (named, no record); slot 0's put on the right shoulder by hand: nothing said of the M9 (the ghost's "on no shoulder now" until the A13 lane),
        // drawn back. The forced _Shotgun M1014 for the M9 (not taken at the door): on none, the M9 gone. A death after the kick: both on none. The
        // sewer's SG 553 and the climb's M9 (KD5's): on none; the MP7 into the emptied slot 1: on none, no record of the AKS-74u gone from it.
        World w;
        Sim s = Escape(&w);
        const bool spawn = s.st.home[0].role == Role::kPrimary;
        const int door = w.AddPick(0xD16, 1.0f, 1, kM1014, 7, 25, false, "M1014");
        TakeUp(s, w, door);
        s.Run(0.5f);
        const bool taken = s.slots[1].kind == kM1014 && s.st.home[1].role == Role::kNone && s.st.home[0].role == Role::kPrimary && s.holder == Holder::kHeld;
        const size_t from = s.events.size();
        Kick(s);
        const Event* k0 = nullptr;
        const Event* k1 = nullptr;
        for (size_t i = from; i < s.events.size(); ++i)
            if (s.events[i].what == What::kKit)
                (s.events[i].slot == 0 ? k0 : k1) = &s.events[i];
        const std::string line0 = LineIn(s, k0), line1 = LineIn(s, k1);
        const bool kick = s.st.home[0].role == Role::kNone && s.st.home[1].role == Role::kNone && AwayCount(s) == 0 && CountFrom(s, What::kAway, from) == 0 &&
                          k0 && std::strcmp(k0->otherGun, "M9") == 0 && k0->place == Place::kNone && k1 && std::strcmp(k1->otherGun, "M1014") == 0 &&
                          k1->place == Place::kNone && s.holder == Holder::kHeld && s.trigger == 1 && Audit(s).empty();
        Grip(s, 1);
        const size_t pFrom = s.events.size();
        PutOn(s, 1, kRS);
        const bool placed = s.holder == Holder::kStowed && s.st.home[0].role == Role::kPrimary && CountFrom(s, What::kLost, pFrom) == 0;
        const bool back = merge::DrawnCaseA(s, 1, kRS) && Refusals(s, from, Why::kWithGame) == 0;
        Sim b = Escape(nullptr);
        const size_t bFrom = b.events.size();
        Gun(b, 0, 0x1014C, kM1014, "M1014", true);
        b.Step();
        Draws(b, 0, true);
        b.Run(1.5f);
        const Event* forced = Kept(Last(b, What::kKit, -2, bFrom), 0);
        const bool shotgun = b.st.home[0].role == Role::kNone && AwayCount(b) == 0 && forced && std::strcmp(forced->otherGun, "M9") == 0 && Audit(b).empty();
        World dw;
        Sim d = Escape(&dw);
        const int door2 = dw.AddPick(0xD17, 1.0f, 0, kM1014, 7, 25, false, "M1014");
        TakeUp(d, dw, door2);
        Kick(d);
        NewLife(d.st);
        NoSlots(d);
        Gun(d, 0, 0x17CA, kAks, "AKS-74u", true);
        Gun(d, 1, 0x17CB, kAks, "AKS-74u", true);
        Gear(d, 7, 0xC046, kKnife, "Knife");
        Draws(d, 0, true);
        d.Run(1.5f);
        const bool death = d.st.home[0].role == Role::kNone && d.st.home[1].role == Role::kNone && Audit(d).empty();
        // The sewer and the climb.
        d.Run(12.0f);
        d.frozen = true;
        Gun(d, 0, 0x5531, kSg553, "SG 553", false);
        Gone(d, 1);
        Gear(d, 7, 0xC047, kKnife, "Knife");
        d.Step();
        Draws(d, 0, true);
        d.Run(1.0f);
        d.frozen = false;
        d.Run(31.0f);
        const bool sewer = d.st.home[0].role == Role::kNone && PlaceNow(d, 1) == Place::kNone && AwayCount(d) == 0 && Audit(d).empty();
        const size_t mFrom = d.events.size();
        const int mp7 = dw.AddPick(0x7A6, 1.0f, 1, kMp7, 40, 120, false, "MP7");
        TakeUp(d, dw, mp7);
        d.slots[1].known = true;
        d.Run(1.5f);
        const bool noGhost = d.st.home[1].role == Role::kNone && AwayCount(d) == 0 && CountFrom(d, What::kAway, mFrom) == 0;
        d.Run(30.0f);
        d.frozen = true;
        Gun(d, 0, 0x9A93, kM9, "M9", false);
        Gone(d, 1);
        Gone(d, 7);
        d.Step();
        Draws(d, 0, true);
        d.Run(1.5f);
        d.frozen = false;
        const bool climb = d.st.home[0].role == Role::kNone && AwayCount(d) == 0 && Audit(d).empty();
        Report("KO4", "Semper Fidelis's and The Great Destroyer's kick, \"kitguns none\": both AKS-74u on no shoulder, the M9 and the M1014 gone with "
                      "the kit (named, no record); slot 0's put on the right shoulder: nothing said of the M9, drawn back, never a refusal \"with the "
                      "game\"; the forced M1014: on none, the M9 gone; a death after: both on none; the sewer's SG 553, the MP7 into the emptied slot 1, "
                      "the climb's M9: on none, no record",
               spawn && taken && kick && placed && back && shotgun && death && sewer && noGhost && climb,
               "spawn %d, the M1014 taken %d, the kick %d (\"%s\" | \"%s\"), put on the right %d, drawn back %d; forced %d (\"%s\"); a death %d (%s, %s); "
               "the sewer %d, the MP7 %d, the climb %d",
               spawn, taken, kick, line0.c_str(), line1.c_str(), placed, back, shotgun, LineIn(b, forced).c_str(), death, RoleName(d.st.home[0].role),
               RoleName(d.st.home[1].role), sewer, noGhost, climb);
    }
    {
        // Swordbreaker: an AN-94 off the floor into slot 1 with the Mk11 drawn in the hands (into its pickup, no record), never placed; the roof's start
        // kit back later (HotelDestruction: the Mk11 a new object in slot 1 for the AN-94, no take): the Mk11 on no shoulder, as the owner played it (the
        // A13 lane's review, ADV6: "kitguns holster" gives it the left).
        bool ok[2] = {};
        Role mk[2] = {};
        for (int kitGuns = 0; kitGuns < 2; ++kitGuns) {
            World w;
            Sim s = SwordWorld(w);
            if (kitGuns)
                s.set.kitHolster = true;  // (else the default)
            s.Run(11.0f);
            s.picked = true, s.pickSlot = 1;
            s.Step();
            s.Switch(1);
            s.hand[1].squeeze = 1.0f;
            s.Run(0.5f);
            const int an = w.AddPick(0xE92, 1.0f, 1, kAn94Kind, 30, 90, false, "AN-94");
            TakeOf(s, w, an, 1);
            s.hand[1].squeeze = 1.0f;
            s.Run(2.0f);
            Gave(s, 1, 0x1119, kMk11Kind, false, true);
            CopyName(s.slots[1].name, "Mk11");
            s.Run(2.0f);
            mk[kitGuns] = s.st.home[1].role;
            ok[kitGuns] = mk[kitGuns] == (kitGuns ? Role::kSecondary : Role::kNone) && AwayCount(s) == 0 && Audit(s).empty();
        }
        Report("KO5", "Swordbreaker's start kit after the roof with an AN-94 off the floor in the Mk11's slot: the Mk11 on no shoulder (\"kitguns none\", "
                      "as played; \"kitguns holster\": the left), nothing recorded",
               ok[0] && ok[1], "the Mk11 %s / %s", RoleName(mk[0]), RoleName(mk[1]));
    }
    g_kitHolster = was;
}

// ---- KR: the A13 lane's review, settled (2026-11-25), each with "kitguns none" and "kitguns holster" ----

// Rock: an AK-74M off the floor by the right hand (the M416 in it into the pickup), never placed; a death: the checkpoint's load shows the spawn kit's
// M416 in slot 0 for tick frames (the AT4 notes: "for one tick it holds the start kit's M4A1"), then the save's AK-74M (no take: the game's restore).
Role PickedThroughDeath(bool kitGuns, int tick) {
    World w;
    Sim s = RockWorld(w);
    if (kitGuns)
        s.set.kitHolster = true;  // (else the default)
    s.Run(11.0f);
    const int ak = w.AddPick(0xE51, 1.0f, 0, kAkKind, 30, 90, false, "AK-74M");
    TakeOf(s, w, ak, 1);
    s.hand[1].squeeze = 1.0f;
    s.Run(0.5f);
    NewLife(s.st);
    Rock(s);
    Named(s.slots[0], 0x24160, kM416Kind, "M416");
    Named(s.slots[2], 0x23200, kM320Kind, "M320");
    Named(s.slots[5], 0x25A30, kSmawKind, "SMAW");
    Named(s.slots[6], 0x26700, 0x67, "M67");
    Named(s.slots[7], 0x27700, 0x77, "Knife");
    s.gameSlot = 0, s.holder = Holder::kHeld, s.trigger = 1;
    for (int k = 0; k < tick; ++k)
        s.Step();
    s.slots[0] = SlotIn{};
    s.slots[0].carried = true, s.slots[0].known = true;
    Named(s.slots[0], 0x2A401, kAkKind, "AK-74M");
    s.Run(1.5f);
    return s.st.home[0].role;
}
// Swordbreaker: an AN-94 off the floor into slot 1 (the Mk11 there, hanging: into the pickup, on its shoulder there), never placed; a death: the spawn
// kit's frames (the M4A1 and the Mk11), then the save's AN-94 in slot 1.
Role SlotOneThroughDeath(bool kitGuns, int tick) {
    World w;
    Sim s = SwordWorld(w);
    if (kitGuns)
        s.set.kitHolster = true;  // (else the default)
    s.Run(11.0f);
    const int an = w.AddPick(0xE61, 1.0f, 1, kAn94Kind, 30, 90, false, "AN-94");
    TakeOf(s, w, an, 1);
    s.hand[1].squeeze = 1.0f;
    s.Run(0.5f);
    NewLife(s.st);
    Swordbreaker(s);
    Named(s.slots[0], 0x34A10, kM4Kind, "M4A1");
    s.slots[1].known = true;
    Named(s.slots[1], 0x31110, kMk11Kind, "Mk11");
    Named(s.slots[6], 0x36760, 0x67, "M67");
    s.gameSlot = 0, s.holder = Holder::kHeld, s.trigger = 1;
    for (int k = 0; k < tick; ++k)
        s.Step();
    s.slots[1] = SlotIn{};
    s.slots[1].carried = true, s.slots[1].known = true;
    Named(s.slots[1], 0x39401, kAn94Kind, "AN-94");
    s.Run(1.5f);
    return s.st.home[1].role;
}
// The M416 put on the LEFT shoulder by hand (its memo the secondary's) and drawn back; an AN-94 taken off the floor with it in the hands (it goes into
// that pickup, no record); taken back off the floor from that pickup, the take's swap told a frame after its gun is seen in slot 0 (a kit's gun for that
// frame). Its memo before and after, and its shoulder after a death whose kit has it (the level's own).
void LateRetake(bool kitGuns, Role* memoBefore, Role* memoAfter, Role* afterDeath) {
    World w;
    Sim s = RockWorld(w);
    if (kitGuns)
        s.set.kitHolster = true;  // (else the default)
    s.Run(11.0f);
    PutOn(s, 1, kLS);
    SqueezeAt(s, 1, kLS);
    s.Run(0.3f);
    const Memo* m0 = MemoOf(s.st, kM416Kind);
    *memoBefore = m0 ? m0->role : Role::kPrimary;
    const int an = w.AddPick(0xE71, 1.0f, 0, kAn94Kind, 30, 90, false, "AN-94");
    TakeOf(s, w, an, 1);
    s.hand[1].squeeze = 1.0f;
    s.Run(2.0f);
    s.slots[0] = SlotIn{};
    s.slots[0].carried = true, s.slots[0].known = true;
    Named(s.slots[0], 0x4169, kM416Kind, "M416");
    s.Step();
    SwapIn in;
    in.changed = 1, in.slot[0] = 0, in.came[0] = kM416Kind, in.went[0] = kAn94Kind, in.wentBlueprint[0] = kAn94Kind + kBlueprintOf, in.entity = 0xE71;
    CopyName(in.taken, "M416");
    NoteSwap(s.set, s.slots, in, s.st);
    s.taken = 1u << 0;
    s.Run(1.5f);
    const Memo* m1 = MemoOf(s.st, kM416Kind);
    *memoAfter = m1 ? m1->role : Role::kPrimary;
    NewLife(s.st);
    Rock(s);
    Named(s.slots[0], 0x44160, kM416Kind, "M416");
    Named(s.slots[2], 0x43200, kM320Kind, "M320");
    Named(s.slots[5], 0x45A30, kSmawKind, "SMAW");
    Named(s.slots[6], 0x46700, 0x67, "M67");
    Named(s.slots[7], 0x47700, 0x77, "Knife");
    s.gameSlot = 0, s.holder = Holder::kHeld, s.trigger = 1;
    s.Run(1.5f);
    *afterDeath = s.st.home[0].role;
}
// Swordbreaker: an M249 off the floor into slot 0 (the M4A1 in the hands into its pickup, no record), put on the right shoulder by hand; the roof's AT4 by
// its kit into slot 0, into the right hand (the M249 hanging: with the game at its shoulder, said); the AT4 thrown, a squeeze at the right shoulder with
// both hands empty (not back yet: refused, felt); the start kit back (HotelDestruction: the M4A1 into slot 0 in the AT4's place, into the right hand, the
// Mk11 a new object). After: the M249's record, its line; the M4A1 let go of (it lies), a squeeze at the right shoulder with both hands empty; the M4A1
// taken up again and put there by hand.
struct RoofGhost {
    bool recordBefore = false, refusedBefore = false, recordAfter = true, saidGone = false, refusedAfter = true, placed = false;
    int lostAtPlace = -1;
    std::string line;
};
RoofGhost GhostAfterRoof(bool kitGuns, bool marked) {
    RoofGhost g;
    World w;
    Sim s = SwordWorld(w);
    if (kitGuns)
        s.set.kitHolster = true;  // (else the default)
    s.Run(11.0f);
    const int mg = w.AddPick(0xE81, 1.0f, 0, 0x249, 100, 200, false, "M249");
    TakeOf(s, w, mg, 1);
    s.hand[1].squeeze = 1.0f;
    s.Run(0.3f);
    PutOn(s, 1, kRS);
    s.Run(1.0f);
    Gave(s, 0, 0xA7F1, kAt4Kind, true, true);
    CopyName(s.slots[0].name, "AT4");
    if (marked)
        s.taken |= 1u << 0;  // (the gift as hands.cpp gives it after the roof's prompt is used by reach: its slot marked a take's, no swap told)
    s.Switch(0);
    s.hand[1].squeeze = 1.0f;
    s.armed[1] = true;
    s.Run(1.5f);
    const Away* a = AwayOf(s, 0x249);
    g.recordBefore = a && !a->entity && a->role == Role::kPrimary;
    LetGo(s, 1);  // (the AT4 thrown away)
    s.Run(0.5f);
    size_t from = s.events.size();
    SqueezeAt(s, 0, kRS, 0.6f);
    g.refusedBefore = Refusals(s, from, Why::kWithGame) == 1;
    s.hand[0].squeeze = 0.0f;
    s.MoveTo(0, kFront[0], kFront[1], kFront[2]);
    s.Run(15.0f);
    from = s.events.size();
    Gave(s, 0, 0x4A3F, kM4Kind, false, true);
    CopyName(s.slots[0].name, "M4A1");
    s.slots[1].id = 0x1119;
    s.Switch(0);
    s.hand[1].squeeze = 1.0f;
    s.armed[1] = true;
    s.Run(1.5f);
    const Away* b = AwayOf(s, 0x249);
    g.recordAfter = b != nullptr;
    const Event* gone = Kept(Last(s, What::kLost, -2, from), 0);
    g.saidGone = gone && gone->why == Why::kGone && gone->kit && std::strcmp(gone->gun, "M249") == 0;
    g.line = LineIn(s, gone);
    LetGo(s, 1);  // (the M4A1 let go of: it lies, both hands empty)
    s.Run(0.3f);
    from = s.events.size();
    SqueezeAt(s, 0, kRS, 0.6f);
    g.refusedAfter = Refusals(s, from, Why::kWithGame) != 0;
    s.hand[0].squeeze = 0.0f;
    s.MoveTo(0, kFront[0], kFront[1], kFront[2]);
    s.Run(0.3f);
    s.hand[1].squeeze = 1.0f;
    s.takeBy = 1;  // (the right hand takes it up again)
    s.Step();
    s.Run(0.3f);
    from = s.events.size();
    PutOn(s, 1, kRS);
    g.placed = s.holder == Holder::kStowed && s.st.home[0].role == Role::kPrimary;
    g.lostAtPlace = CountFrom(s, What::kLost, from);
    return g;
}
// The recheck's F1: Uprising, then Operation Guillotine in one session. Uprising's stab (KU1's), an 870 off the floor into slot 1 by the right hand, put on
// the left shoulder by hand (its kind's memo the left); Guillotine's start kit, the mortar alone in slot 0 (gear: at no holster, yet that level's kit by
// its slot): another level's, the memo gone; a death in the mortar's scene, the mortar alone again: the same level's; the SP_Bank kit's SCAR-H and 870 in
// the mortar's scene, past the kit's seconds (Mortar's); a death, the SP_Bank kit at its checkpoint. A holster word never crosses levels.
struct LevelCross {
    Role placed = Role::kNone, memo = Role::kNone, mortar = Role::kPrimary, scar = Role::kLauncher, m870 = Role::kLauncher, scarDeath = Role::kLauncher,
         m870Death = Role::kLauncher;
    bool forgot = false, memoGone = false, sameLevel = false;
    std::string audit;
};
LevelCross UprisingThenGuillotine(bool kitGuns) {
    LevelCross o;
    World w;
    const bool was = g_kitHolster;
    g_kitHolster = kitGuns;  // (Stabbed's settings)
    Sim s = Stabbed(&w);
    g_kitHolster = was;
    const int p = w.AddPick(0x870E, 1.0f, 1, k870, 4, 20, false, "870");
    Grip(s, 1);
    TakeUp(s, w, p);
    s.Run(0.3f);
    PutOn(s, 1, kLS);
    o.placed = s.st.home[1].role;
    const Memo* m = MemoOf(s.st, k870);
    o.memo = m ? m->role : Role::kNone;
    // Guillotine: the mortar alone (its first noting tells the level).
    NewLife(s.st);
    NoSlots(s);
    Gear(s, 0, 0x2241, kMortar, "M224 mortar");
    Draws(s, 0);
    s.Run(0.5f);
    o.mortar = s.st.home[0].role;
    o.forgot = s.st.memoForgot;
    o.memoGone = MemoOf(s.st, k870) == nullptr;
    s.Run(12.0f);
    // A death in the mortar's scene: the mortar alone again, the same level's (nothing forgotten).
    NewLife(s.st);
    Gear(s, 0, 0x2242, kMortar, "M224 mortar");
    Draws(s, 0);
    s.Run(12.0f);
    o.sameLevel = !s.st.memoForgot && s.st.home[0].role == Role::kNone;
    s.Run(78.0f);
    // The SP_Bank kit in the mortar's scene.
    s.frozen = true;
    Gun(s, 0, 0x5CA2, kScar, "SCAR-H", false);
    Gun(s, 1, 0x8702, k870, "870 MCS", false);
    Gear(s, 6, 0x6722, kM67, "M67");
    Gear(s, 7, 0xC013, kKnife, "Knife");
    s.Step();
    Draws(s, 0, true);
    s.Run(0.5f);
    s.frozen = false;
    s.Run(1.5f);
    o.scar = s.st.home[0].role, o.m870 = s.st.home[1].role;
    // A death: the SP_Bank kit at its checkpoint.
    NewLife(s.st);
    NoSlots(s);
    Gun(s, 0, 0x5CA3, kScar, "SCAR-H", false);
    Gun(s, 1, 0x8703, k870, "870 MCS", false);
    Gear(s, 6, 0x6723, kM67, "M67");
    Gear(s, 7, 0xC014, kKnife, "Knife");
    Draws(s, 0);
    s.Run(3.0f);
    o.scarDeath = s.st.home[0].role, o.m870Death = s.st.home[1].role;
    o.audit = Audit(s);
    return o;
}

void Review() {
    for (int kitGuns = 0; kitGuns < 2; ++kitGuns) {
        const char* const set = kitGuns ? "kitguns holster" : "kitguns none";
        {
            Role before = Role::kNone, after = Role::kNone, death = Role::kNone;
            LateRetake(kitGuns != 0, &before, &after, &death);
            Report(kitGuns ? "KR1k" : "KR1", "the review's #1: the player's M416, put on the left shoulder by hand, taken back off the floor with the take's "
                                             "mark a frame late (a kit's gun for that frame): its memo kept (the left), the left shoulder after a death",
                   before == Role::kSecondary && after == Role::kSecondary && death == Role::kSecondary, "%s: the memo before %s, after %s; after a death %s",
                   set, RoleName(before), RoleName(after), RoleName(death));
        }
        {
            const Role rock[3] = {PickedThroughDeath(kitGuns != 0, 0), PickedThroughDeath(kitGuns != 0, 1), PickedThroughDeath(kitGuns != 0, 10)};
            const Role sword[3] = {SlotOneThroughDeath(kitGuns != 0, 0), SlotOneThroughDeath(kitGuns != 0, 1), SlotOneThroughDeath(kitGuns != 0, 10)};
            bool none = true;
            for (int i = 0; i < 3; ++i)
                none = none && rock[i] == Role::kNone && sword[i] == Role::kNone;
            Report(kitGuns ? "KR2k" : "KR2", "the review's #2: a gun picked up and never placed (Rock's AK-74M in slot 0, Swordbreaker's AN-94 in slot 1) "
                                             "through a death whose load shows the spawn kit's gun there first for 0, 1 or 10 frames: on no shoulder each",
                   none, "%s: the AK-74M %s %s %s; the AN-94 %s %s %s", set, RoleName(rock[0]), RoleName(rock[1]), RoleName(rock[2]), RoleName(sword[0]),
                   RoleName(sword[1]), RoleName(sword[2]));
        }
        {
            bool ok = true;
            std::string why;
            for (int marked = 0; marked < 2; ++marked) {
                const RoofGhost g = GhostAfterRoof(kitGuns != 0, marked != 0);
                const bool pass = g.recordBefore && g.refusedBefore && !g.recordAfter && g.saidGone && !g.refusedAfter && g.placed && g.lostAtPlace == 0;
                ok = ok && pass;
                char part[600];
                std::snprintf(part, sizeof(part), "%s%s: with the game %d, refused then %d; after: recorded %d, \"%s\", refused %d, the M4A1 put on the right "
                                                  "%d (lost lines %d)",
                              why.empty() ? "" : "; ", marked ? "the gift marked a take's" : "a plain gift", g.recordBefore, g.refusedBefore, g.recordAfter,
                              g.line.c_str(), g.refusedAfter, g.placed, g.lostAtPlace);
                why += part;
            }
            Report(kitGuns ? "KR3k" : "KR3", "the review's #3: Swordbreaker, a floor M249 put on the right shoulder, the roof's AT4 by its kit (a plain gift, "
                                             "or marked a take's as the prompt's use gives it; the M249 with the game, a draw there not back yet), the start "
                                             "kit back after: the M249 gone with the kit (said), a squeeze at the right shoulder never refused \"with the "
                                             "game\" again, the M4A1 put there with nothing of the M249 said",
                   ok, "%s: %s", set, why.c_str());
        }
        {
            const LevelCross o = UprisingThenGuillotine(kitGuns != 0);
            const bool kit = kitGuns ? o.scar == Role::kPrimary && o.m870 == Role::kSecondary && o.scarDeath == Role::kPrimary &&
                                           o.m870Death == Role::kSecondary
                                     : o.scar == Role::kNone && o.m870 == Role::kNone && o.scarDeath == Role::kNone && o.m870Death == Role::kNone;
            Report(kitGuns ? "KR4k" : "KR4", "the recheck's F1: Uprising, then Operation Guillotine in one session: an 870 off Uprising's floor put on the "
                                             "left shoulder by hand; Guillotine's start kit, the mortar alone in slot 0 (gear, at no holster), another level's: "
                                             "the memo gone (a death in the mortar's scene the same level's); the SP_Bank kit's SCAR-H and 870 as the setting "
                                             "gives them, after a death too, never by Uprising's word",
                   o.placed == Role::kSecondary && o.memo == Role::kSecondary && o.mortar == Role::kNone && o.forgot && o.memoGone && o.sameLevel && kit &&
                       o.audit.empty(),
                   "%s: the 870 in Uprising %s (memo %s); the mortar %s, memo forgot %d (the 870's word gone %d), a death there the same level's %d; the "
                   "SP_Bank kit: the SCAR-H %s, the 870 %s; after a death: the SCAR-H %s, the 870 %s; %s",
                   set, RoleName(o.placed), RoleName(o.memo), RoleName(o.mortar), o.forgot, o.memoGone, o.sameLevel, RoleName(o.scar), RoleName(o.m870),
                   RoleName(o.scarDeath), RoleName(o.m870Death), o.audit.c_str());
        }
    }
}

void Rows() {
    Uprising();
    Guillotine();
    Comrades();
    NightShift();
    Kaffarov();
    Destroyer();
    LateTake();
    g_kitHolster = false;
    LateTake();
    g_kitHolster = true;
    KitLines();
    Owners();
    Review();
}

}  // namespace kits

int main() {
    Places();
    Map();
    PutAway();
    Draw();
    Keys();
    NotSeen();
    HeadTurns();
    OwnHeading();
    FastLetGo();
    Asked();
    Tick();
    File();
    Lines();
    Broken();
    HandOnly();
    AwayRows();
    SettleRows();
    ReviewRows();
    second::Rows();
    merge::Rows();
    Tonight();
    kits::Rows();
    std::printf("%d checks, %d failed\n", g_counted, g_failed);
    return g_failed;
}

#pragma once
// The free hands' film rounds (build\freehands.txt "test N", with nobody in the headset): what each hand
// does at a time into the round, [0] left and [1] right. hands.cpp turns each Where into a grip pose and
// drives the hands with it (the right while the stand-in stands in, the left while the real left isn't
// tracked); tools/grasptest.cpp runs the same rounds through the hold machine (grasp.h). Pure, header only.
//   test 1 (26 s) every hand-over: the right lets go (it falls), the left picks it up and fires, the right
//          takes the handguard, the left lets go (held by the right alone), the left takes the pistol grip
//          back, lets go (it falls), the right picks it up, the left takes the handguard, the right lets go
//          (held by the left alone), the right takes the pistol grip back.
//   test 2 (50 s) the throw: the right lets go moving forward and up with a spin; it lies past the 30 s a
//          magazine lies; the right picks it up.
//   test 3 (12 s) the free right glove relaxed, pointing, a fist and half closed, the left beside it doing
//          the same (its mirror twin), then the right picks the rifle up.
// The times things are expected (grasptest checks them within 2 frames at 72 Hz; the let-gos 3 frames
// after the grip opens):
//   1: 1.04 released by the right; 5.0 picked up by the left; 5.12 arrived; 9.04 the left lets go, held by
//      the right on the handguard; 11.0 the left regrips; 11.12 arrived; 14.04 released by the left; 16.0
//      picked up by the right; 16.12 arrived; 21.04 the right lets go, held by the left; 23.0 the right
//      regrips; 23.12 arrived. (hands.cpp's Support takes and lets go of the handguard at 7.4 and 13.0 by
//      the right, at 19.0 and 24.0 by the left.)
//   2: 1.04 released by the right; 45.0 picked up by the right; 45.12 arrived.
//   3: 0.54 released by the right; 9.0 picked up by the right; 9.12 arrived.

#include <cmath>
#include <cstdint>

namespace freetest {

enum class Where : uint8_t {
    kNone,     // not moved by the round
    kStandIn,  // the stand-in's right grip
    kMirror,   // that grip mirrored about the head (head space p (-x, y, z), q (x, -y, -z, w))
    kGuard,    // the glove at the game's hold on the handguard of the gun as drawn (its side's), the controller
               // pose behind it through the line-up offset
    kGrip,     // the grip point at its side's pistol-grip point of the gun as drawn, keeping its last turn
    kFront,    // 30 cm out, 12 cm to its side, 15 cm down (head space), the stand-in grip's turn, mirrored for the left
};
// kGuard and kGrip are worked out at a step's first frame and then held head-relative.

struct Step {
    Where where = Where::kNone;
    float squeeze = 0.0f, trigger = 0.0f, thumb = 0.0f;  // 0-1; thumb 1: on the stick
    bool forceTake = false;                               // taken whatever the squeeze's history or the distance
    bool sensed = false;
    float velocity[3] = {}, spin[3] = {};  // head axes, m/s and rad/s (the throw)
};

constexpr float kRound[4] = {0.0f, 26.0f, 50.0f, 12.0f};  // each test's round (seconds); 0: no such test

inline void At(int test, float t, Step out[2]);  // t: s into the round (hands.cpp takes fmod)

inline void At(int test, float t, Step out[2]) {
    out[0] = Step{};
    out[1] = Step{};
    if (!(t >= 0.0f))  // before the round or not a number: nothing moves
        return;
    Step& l = out[0];
    Step& r = out[1];
    const auto in = [t](float from, float to) { return t >= from && t < to; };
    const auto set = [](Step& s, Where where, float squeeze, bool forceTake = false) {
        s.where = where;
        s.squeeze = squeeze;
        s.forceTake = forceTake;
    };
    if (test == 1) {
        if (in(0.0f, 1.0f))
            set(r, Where::kStandIn, 1.0f);
        else if (in(1.0f, 7.0f))
            set(r, Where::kStandIn, 0.0f);  // lets go: it falls
        else if (in(7.0f, 7.4f))
            set(r, Where::kGuard, 0.0f);
        else if (in(7.4f, 13.0f))
            set(r, Where::kGuard, 1.0f);  // on the handguard of the rifle in the left hand
        else if (in(13.0f, 16.0f))
            set(r, Where::kStandIn, 0.0f);
        else if (in(16.0f, 16.2f))
            set(r, Where::kStandIn, 1.0f, true);  // picks it up
        else if (in(16.2f, 21.0f))
            set(r, Where::kStandIn, 1.0f);
        else if (in(21.0f, 23.0f))
            set(r, Where::kStandIn, 0.0f);  // lets go: held by the left on the handguard
        else if (in(23.0f, 23.2f))
            set(r, Where::kGrip, 1.0f, true);  // takes the pistol grip back
        else if (in(23.2f, 26.0f))
            set(r, Where::kGrip, 1.0f);
        if (in(5.0f, 5.2f))
            set(l, Where::kMirror, 1.0f, true);  // picks it up
        else if (in(5.2f, 9.0f)) {
            set(l, Where::kMirror, 1.0f);
            if (in(5.5f, 6.5f))
                l.trigger = 1.0f - 2.0f * std::fabs(t - 6.0f);  // the left trigger pulled and let go: it fires
        } else if (in(9.0f, 11.0f))
            set(l, Where::kMirror, 0.0f);  // lets go: held by the right on the handguard
        else if (in(11.0f, 11.2f))
            set(l, Where::kGrip, 1.0f, true);  // takes the pistol grip back
        else if (in(11.2f, 14.0f))
            set(l, Where::kGrip, 1.0f);
        else if (in(14.0f, 16.0f))
            set(l, Where::kGrip, 0.0f);  // lets go: it falls
        else if (in(18.0f, 19.0f))
            set(l, Where::kGuard, 0.0f);
        else if (in(19.0f, 24.0f))
            set(l, Where::kGuard, 1.0f);  // on the handguard of the rifle in the right hand
    } else if (test == 2) {
        if (in(0.0f, 1.0f))
            set(r, Where::kStandIn, 1.0f);
        else if (in(1.0f, 1.15f)) {
            set(r, Where::kStandIn, 0.0f);  // lets go, thrown forward and up with a spin
            r.sensed = true;
            r.velocity[0] = 0.3f, r.velocity[1] = 1.5f, r.velocity[2] = -3.0f;
            r.spin[0] = 6.0f;
        } else if (in(1.15f, 45.0f))
            set(r, Where::kStandIn, 0.0f);
        else if (in(45.0f, 45.2f))
            set(r, Where::kStandIn, 1.0f, true);  // picks it up
        else if (in(45.2f, 50.0f))
            set(r, Where::kStandIn, 1.0f);
    } else if (test == 3) {
        if (in(0.0f, 0.5f))
            set(r, Where::kStandIn, 1.0f);
        else if (in(0.5f, 1.0f))
            set(r, Where::kStandIn, 0.0f);  // lets go: it falls
        else if (in(1.0f, 3.0f))
            set(r, Where::kFront, 0.0f);  // relaxed
        else if (in(3.0f, 5.0f)) {
            set(r, Where::kFront, 1.0f);  // a point
            r.thumb = 1.0f;
        } else if (in(5.0f, 7.0f)) {
            set(r, Where::kFront, 1.0f);  // a fist
            r.trigger = 1.0f;
            r.thumb = 1.0f;
        } else if (in(7.0f, 9.0f)) {
            set(r, Where::kFront, 0.5f);  // half closed
            r.trigger = 0.5f;
        } else if (in(9.0f, 9.2f))
            set(r, Where::kStandIn, 1.0f, true);  // picks it up
        else if (in(9.2f, 12.0f))
            set(r, Where::kStandIn, 1.0f);
        if (in(1.0f, 9.0f)) {  // the left beside it with the right's fingers
            l.where = Where::kFront;
            l.squeeze = r.squeeze;
            l.trigger = r.trigger;
            l.thumb = r.thumb;
        }
    }
}

}  // namespace freetest

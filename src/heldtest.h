#pragma once
// weapon.txt "heldtest 3" and "heldtest 4" (and the UMP-45's "heldtest 9"): hands.cpp's scripted left hand on a held gun's lever or charging handle, with
// nobody in the headset, for the films: 3 reaching over the top, 4 from under it. Its path and its wrist, on their own so tools/leverfingerstest.cpp
// draws the same hand through handmag.cpp and the fitted hands as hands.cpp does: over and under, either hand, every gun with a lever. Pure, header only.
//
// The wrist is a turn, square in each of its axes and in the gloves' own. The proof of 2026-11-25 (Operation Swordbreaker's Mk11 Mod 0, heldtest 4:
// build\survey\p114b_mk11_under_zoom.png, frames 7 and 8, build\logs\bf3vr_1125_p114b_part1.log 16:35:58-16:36:02) drew the left glove's fingers as
// long striped rods up past the scope: heldtest 4's palm, (-0.217, 0.976, 0) in the gun's axes, was 65 degrees from its fingers, (-0.976, 0.217, 0),
// not 90 (its x the wrong way round), so the glove's matrix was a shear, not a turn: 1.19 times as long along one diagonal and 0.76 along the other, its
// determinant 0.82. Every finger drawn on it was sheared with it, its bones 0.89 to 1.19 of their length, the most along the way they curl: as the
// lever was let go of, the hand palm up at the back of the receiver, its fingers open and pointing up towards the eye, they showed as long thin rods.
// The fitted hand on the handle is a turn, so the hand on the handle itself drew right, over and under; the scripted hand drew wrong wherever it was
// drawn on its own (coming in, and from the let-go on). Real hands never were: a controller's glove is a turn. The palm is (0.217, 0.976, 0), square
// to the fingers. A hand from under a T handle (the M16A4's, the M4A1's, the KH2002's, the Mk11's) still takes the two-finger pull over the top as it
// closes on the handle, the owner's rule for the M4 family (2026-11-05: an under and over pull for "almost all the guns. Except for the M4s... they have
// a specific pull"); the side levers' from under are their own fitted hands (handmag.h's leverUnderHold).

#include <cmath>

namespace heldtest {

struct Key {
    float time;    // s into the test
    float off[3];  // the hand's middle from where it holds the seated magazine, on the gun's frame
};

// 3's and 4's path. toLever: the lever's knob from where the hand holds the seated magazine; rest: the hand's place at rest from there. Reaching over the
// top (3) it comes down from 20 cm above the knob, from under it (4) straight from its rest: onto the knob at 1 s, drawn back 10 cm from 1.5 to 2.3 s,
// held there to 2.8 s, back the way it came by 3.2 s, at rest by 3.6 s. Its trigger (LeverTrigger) from 1.3 to 2.6 s: taken at 1.3, let go of at 2.6.
constexpr int kLeverKeys = 8;
inline void LeverKeys(bool overTheTop, const float toLever[3], const float rest[3], Key out[kLeverKeys]) {
    const float from[3] = {overTheTop ? toLever[0] : rest[0], overTheTop ? toLever[1] + 0.2f : rest[1], overTheTop ? toLever[2] : rest[2]};
    const float times[kLeverKeys] = {0.0f, 0.5f, 1.0f, 1.5f, 2.3f, 2.8f, 3.2f, 3.6f};
    const float* at[kLeverKeys] = {rest, from, toLever, toLever, nullptr, nullptr, from, rest};
    for (int k = 0; k < kLeverKeys; ++k) {
        out[k].time = times[k];
        for (int i = 0; i < 3; ++i)
            out[k].off[i] = at[k] ? at[k][i] : toLever[i] - (i == 2 ? 0.10f : 0.0f);  // (4 and 5: drawn back along the gun)
    }
}
inline bool LeverTrigger(float t) {
    return t >= 1.3f && t < 2.6f;
}

// Where keys have the hand t into the test: between two keys eased in and out (smoothstep), the last key's after the last (hands.cpp's interpolation).
inline void KeyAt(const Key* keys, int count, float t, float off[3]) {
    for (int i = 0; i < 3; ++i)
        off[i] = keys[count - 1].off[i];
    for (int k = 0; k + 1 < count; ++k)
        if (t >= keys[k].time && t < keys[k + 1].time) {
            float s = (t - keys[k].time) / (keys[k + 1].time - keys[k].time);
            s = s * s * (3.0f - 2.0f * s);
            for (int i = 0; i < 3; ++i)
                off[i] = keys[k].off[i] + (keys[k + 1].off[i] - keys[k].off[i]) * s;
            return;
        }
}

namespace detail {
// a's direction, and b made square to it, both of unit length; c = a x b: a right-handed set of axes.
inline void Square(const float* a, const float* b, float* x, float* y, float* z) {
    const float la = std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
    for (int i = 0; i < 3; ++i)
        x[i] = a[i] / la;
    const float d = b[0] * x[0] + b[1] * x[1] + b[2] * x[2];
    for (int i = 0; i < 3; ++i)
        y[i] = b[i] - d * x[i];
    const float ly = std::sqrt(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
    for (int i = 0; i < 3; ++i)
        y[i] /= ly;
    z[0] = x[1] * y[2] - x[2] * y[1];
    z[1] = x[2] * y[0] - x[0] * y[2];
    z[2] = x[0] * y[1] - x[1] * y[0];
}
}  // namespace detail

// The wrist as a real hand has it on a lever, in the gun's axes (x left, y up, z forward): over the top (fromBelow false) the palm down, the fingers over
// to the gun's right and a little forward, the thumb towards the shooter; from under it the palm up, the fingers to the gun's right and a little up. A
// handle on the gun's left (the G3's, the UMP-45's) is reached from its own side either way: the palm towards the gun, the fingers forward. out (3x3,
// rows): the gloves' bind space into the gun's frame, a turn: the hand's own axes there (along its fingers, out of its palm, from its thumb to its little
// finger: handpose.cpp's kAlong, kPalm, kAcross, made square) onto these, made square.
inline void LeverWrist(bool onItsLeft, bool fromBelow, float out[9]) {
    constexpr float kBindAlong[3] = {0.437f, -0.544f, 0.716f}, kBindPalm[3] = {-0.639f, -0.748f, -0.179f};
    const float along[3] = {onItsLeft ? 0.2f : fromBelow ? -0.976f : -0.954f, onItsLeft ? 0.0f : fromBelow ? 0.217f : 0.0f, onItsLeft ? 0.98f : fromBelow ? 0.0f : 0.3f};
    const float palm[3] = {onItsLeft ? -0.98f : fromBelow ? 0.217f : 0.0f, onItsLeft ? 0.0f : fromBelow ? 0.976f : -1.0f, onItsLeft ? 0.2f : 0.0f};
    float b[3][3], g[3][3];
    detail::Square(kBindAlong, kBindPalm, b[0], b[1], b[2]);
    detail::Square(along, palm, g[0], g[1], g[2]);
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            out[i * 3 + j] = g[0][i] * b[0][j] + g[1][i] * b[1][j] + g[2][i] * b[2][j];
}

}  // namespace heldtest

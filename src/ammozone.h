#pragma once

#include <cmath>

// Where the held gun's ammo is taken, for every gun and launcher on every level: with the left grip, at the belly button (the user, 2026-11-05:
// "let's switch the ammo to be on the belly button, stomach area. Okay? So whatever gun you're holding will give you that ammo when you reach for that area. That includes the
// launcher, like this SMAW"). Over the left shoulder until then (2026-11-04), where the secondary's holster goes now; the pelvis before that ("a very awkward position").
// Nothing tracks the body, so it's placed from the headset: from the neck point (pouch.cpp: 7.5 cm below and 8 cm behind the eyes) in the body's axes (right, up, forward; the
// body's heading, pouch::BodyFrame). Seated, the navel is some 40-48 cm below the neck point and a little in front of it, the controller's grip point a few cm off the belly.
// Generous, as it's reached without looking: an egg 36 cm across the body, 32 cm high, but only 24 cm deep, so it stops short of the gun's handguard and magazine well (held
// 35 cm and more out in front, seated). The rest of the body (the user, same day): grenades (and deployables later) on the left chest, C4 and claymores on the right chest, the
// primary's holster on the right shoulder, the secondary's on the left.
namespace ammozone {

constexpr float kBelly[3] = {0.0f, -0.43f, 0.13f};          // its middle from the neck point: right, up, forward (metres)
constexpr float kBellyRadii[3] = {0.18f, 0.16f, 0.12f};     // half its size across, up and down, forward and back
constexpr float kPutBackMore = 0.05f;                        // let go of within this more than the zone: back, not dropped

// How far out a point from the neck is, each way by the zone's own size: 1 its edge, under 1 inside.
inline float Size(const float* fromNeck, float more = 0.0f) {
    float sum = 0.0f;
    for (int i = 0; i < 3; ++i) {
        const float d = (fromNeck[i] - kBelly[i]) / (kBellyRadii[i] + more);
        sum += d * d;
    }
    return std::sqrt(sum);
}
inline bool Inside(const float* fromNeck, float more = 0.0f) {
    return Size(fromNeck, more) <= 1.0f;
}

}

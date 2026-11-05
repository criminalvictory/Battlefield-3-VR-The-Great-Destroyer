#pragma once

// Where all ammo is taken, for every gun and launcher on every level: with the left grip, over the left shoulder (the user,
// 2026-11-04: "anything in terms of ammo has to be the left shoulder"; the pelvis was "a very awkward position"). Nothing tracks the
// shoulders and it's reached without looking, so it's generous: within 60 cm of the neck point, not more than 12 cm below it or 12 cm
// in front of it, on the body's heading (pouch::BodyFrame). The rockets' first (rocket.cpp, "feels right"); magazines the same (pouch.cpp).
namespace ammozone {

constexpr float kBelow = 0.12f, kForward = 0.12f, kReach = 0.60f;  // metres
constexpr float kPutBackMore = 0.05f;                              // let go of within this more than the zone: back, not dropped

}

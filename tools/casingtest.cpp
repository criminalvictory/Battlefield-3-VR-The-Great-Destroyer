// The mod's own casings' pure parts (src/casings.cpp), without the game: a round's throw out of the port as the game's emitter numbers have it, in
// the room and the arms' space; the shots told by the count of rounds; the placements as the game's casing draw takes them, packed by kind; the
// casing's shape read from a mesh's vertices; halves read as floats. Then 1000 seeded throws flown by loose.cpp over flat ground from a gun 1.4 m up:
// where they land, that they never go into the ground, that they come to rest, lying on their side; what a step costs.
// The room is y up, metres: the gun 1.4 m up points along -z (its frame x left, y up, z to its muzzle), so its right is the room's +x and its back +z.
// casings.cpp's game calls are stubbed here (never called by these parts).
// Usage: casingtest   The exit code is the number of failed checks.

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "../src/casings.h"
#include "../src/loose.h"

// ---- casings.cpp's calls into the game: never made by what's tested here.
struct ID3D11Resource;
namespace skin {
bool GameGunPort(float (&world)[3]) {
    world[0] = world[1] = world[2] = 0.0f;
    return false;
}
int ViewBufferKind(ID3D11Resource*) {
    return 0;
}
const uint8_t* LatestViewConstants(ID3D11Resource*& resource) {
    resource = nullptr;
    return nullptr;
}
}  // namespace skin
namespace ammo {
bool RoundsInHand(int& loaded) {
    loaded = -1;
    return false;
}
bool M320UpOrRecent() {
    return false;
}
}  // namespace ammo
namespace scopeview {
std::atomic<bool> g_drawing{false};
}
namespace camera {
bool SafeRead(uintptr_t, void* out, size_t size) {
    std::memset(out, 0, size);
    return false;
}
}  // namespace camera
namespace logx {
void Write(const char*, ...) {}
}  // namespace logx

namespace {

int g_failed = 0, g_checks = 0;

void Check(bool ok, const char* what) {
    ++g_checks;
    if (!ok) {
        ++g_failed;
        std::printf("FAIL: %s\n", what);
    }
}

constexpr float kPi = 3.14159265f;

float Length3(const float* v) {
    return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}
float Dot3(const float* a, const float* b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
void Transform(const float* m, const float* p, float* out) {
    for (int i = 0; i < 3; ++i)
        out[i] = m[i * 4] * p[0] + m[i * 4 + 1] * p[1] + m[i * 4 + 2] * p[2] + m[i * 4 + 3];
}
void RotateRows(const float* m, const float* v, float* out) {  // m's 3x3 (in a 3x4) times v
    for (int i = 0; i < 3; ++i)
        out[i] = m[i * 4] * v[0] + m[i * 4 + 1] * v[1] + m[i * 4 + 2] * v[2];
}
void RotateRowsBack(const float* m, const float* v, float* out) {  // its transpose times v
    for (int i = 0; i < 3; ++i)
        out[i] = m[i] * v[0] + m[4 + i] * v[1] + m[8 + i] * v[2];
}
float Det3(const float* m) {  // of a 3x4's 3x3
    return m[0] * (m[5] * m[10] - m[6] * m[9]) - m[1] * (m[4] * m[10] - m[6] * m[8]) + m[2] * (m[4] * m[9] - m[5] * m[8]);
}
bool Orthonormal(const float* m) {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            const float d = m[i * 4] * m[j * 4] + m[i * 4 + 1] * m[j * 4 + 1] + m[i * 4 + 2] * m[j * 4 + 2];
            if (std::fabs(d - (i == j ? 1.0f : 0.0f)) > 1e-4f)
                return false;
        }
    return Det3(m) > 0.0f;
}

// The gun 1.4 m up pointing along the room's -z: its x (left) the room's -x, its y up, its z the room's -z.
constexpr float kGunRoom[12] = {-1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.4f, 0.0f, 0.0f, -1.0f, 0.0f};
// The room into the arms' space: turned 30 degrees about y and moved, as a head turned and walked would have it.
float g_roomToArms[12];
void MakeRoomToArms() {
    const float c = std::cos(30.0f * kPi / 180.0f), s = std::sin(30.0f * kPi / 180.0f);
    const float m[12] = {c, 0.0f, s, 5.0f, 0.0f, 1.0f, 0.0f, -0.25f, -s, 0.0f, c, -3.0f};
    std::memcpy(g_roomToArms, m, sizeof(m));
}

// ---- the throw
void TestThrow() {
    const casings::Emitter* m416 = casings::EmitterFor("m416");
    Check(m416 && m416->kind == casings::k556, "the M416 throws 5.56 casings");
    Check(casings::EmitterFor("smaw") == nullptr && casings::EmitterFor("fim92") == nullptr && casings::EmitterFor(nullptr) == nullptr,
          "the launchers, the Stinger and nothing throw none");
    Check(casings::EmitterFor("ak74") && casings::EmitterFor("ak74")->kind == casings::k545, "the AK-74M throws 5.45");
    Check(casings::EmitterFor("m9") && casings::EmitterFor("m9")->kind == casings::k9, "the train's pistol throws 9 mm");
    Check(casings::IsLooseCasing(casings::kLooseKind) && casings::IsLooseCasing(casings::kLooseKind + 3) && !casings::IsLooseCasing(4) &&
              !casings::IsLooseCasing(casings::kLooseKind + 4),
          "loose's kinds 10 to 13 are casings, skin.cpp's 1 to 4 aren't");
    if (!m416)
        return;
    float portRoom[3];
    Transform(kGunRoom, m416->port, portRoom);
    double speedSum = 0.0;
    float speedLow = 1e9f, speedHigh = 0.0f, upLow = 90.0f, upHigh = -90.0f, backLow = 1.0f, backHigh = -1.0f, spinLow = 1e9f, spinHigh = 0.0f;
    bool placed = true, along = true, square = true, unitAxis = true, axisDown = true, right = true, kinds = true;
    for (uint32_t seed = 1; seed <= 1000; ++seed) {
        casings::Throw t;
        t.gun = m416;
        std::memcpy(t.gunRoom, kGunRoom, sizeof(t.gunRoom));
        std::memcpy(t.roomToArms, g_roomToArms, sizeof(t.roomToArms));
        t.seed = seed;
        casings::Start s;
        if (!casings::ShotStart(t, s)) {
            Check(false, "a throw with good numbers starts");
            return;
        }
        kinds = kinds && s.spec && std::strcmp(s.spec->name, "556") == 0 && s.kind == casings::kLooseKind + casings::k556 && !s.zoomed;
        // Back into the room: its bone (the arms' less the room's move), its speed and spin.
        float bone[12];
        {
            float back[12] = {};
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    back[i * 4 + j] = g_roomToArms[j * 4 + i];
            for (int i = 0; i < 3; ++i)
                back[i * 4 + 3] = -(back[i * 4] * g_roomToArms[3] + back[i * 4 + 1] * g_roomToArms[7] + back[i * 4 + 2] * g_roomToArms[11]);
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 4; ++j)
                    bone[i * 4 + j] = back[i * 4] * s.bone[j] + back[i * 4 + 1] * s.bone[4 + j] + back[i * 4 + 2] * s.bone[8 + j];
                bone[i * 4 + 3] += back[i * 4 + 3];
            }
        }
        square = square && Orthonormal(bone);
        const float length = s.spec->ringAt[s.spec->rings - 1] - s.spec->ringAt[0];
        const float middleOwn[3] = {0.0f, 0.0f, s.spec->ringAt[0] + 0.5f * length};
        float middle[3];
        Transform(bone, middleOwn, middle);
        const float off[3] = {middle[0] - portRoom[0], middle[1] - portRoom[1], middle[2] - portRoom[2]};
        placed = placed && Length3(off) < 1e-4f;
        const float mouth[3] = {bone[2], bone[6], bone[10]};  // its +z in the room: the gun's z, the room's -z
        along = along && std::fabs(mouth[2] + 1.0f) < 1e-4f;
        float v[3], vRoom[3], onGun[3];
        std::memcpy(v, s.velocity, sizeof(v));
        RotateRowsBack(g_roomToArms, v, vRoom);
        RotateRowsBack(kGunRoom, vRoom, onGun);
        const float speed = Length3(onGun);
        speedSum += speed;
        speedLow = std::min(speedLow, speed), speedHigh = std::max(speedHigh, speed);
        const float up = std::asin(onGun[1] / speed) * 180.0f / kPi, back = onGun[2] / speed;
        upLow = std::min(upLow, up), upHigh = std::max(upHigh, up);
        backLow = std::min(backLow, back), backHigh = std::max(backHigh, back);
        right = right && onGun[0] < 0.0f && vRoom[0] > 0.0f;
        float a[3], aRoom[3], aGun[3];
        std::memcpy(a, s.axis, sizeof(a));
        unitAxis = unitAxis && std::fabs(Length3(a) - 1.0f) < 1e-4f;
        RotateRowsBack(g_roomToArms, a, aRoom);
        RotateRowsBack(kGunRoom, aRoom, aGun);
        axisDown = axisDown && aGun[1] < -0.89f;  // (up to 0.35 off it either way: 1 / sqrt(1.245))
        spinLow = std::min(spinLow, s.spin), spinHigh = std::max(spinHigh, s.spin);
    }
    std::printf("M416 throws: %.2f to %.2f m/s (%.2f on average), %.1f to %.1f degrees up, back %.3f to %.3f of the way, spin %.2f to %.2f rad/s\n", speedLow,
                speedHigh, speedSum / 1000.0, upLow, upHigh, backLow, backHigh, spinLow, spinHigh);
    Check(kinds, "a 5.56 casing, loose's kind 10, not zoomed");
    Check(square, "its bone is a turn (orthonormal, right-handed)");
    Check(placed, "its middle is at the gun's port as drawn");
    Check(along, "it starts along the bore, its mouth forward");
    Check(speedLow >= 1.5f - 1e-3f && speedHigh <= 3.0f + 1e-3f && std::fabs(speedSum / 1000.0 - 2.25) < 0.1, "its speed is the emitter's random, 1.5 to 3 m/s");
    // The emitter's box made unit: up from asin(0.8 / 1.315) to asin(1 / 1.296), back from 0.2 / 1.418 to 0.3 / 1.170.
    Check(upLow >= 37.0f && upHigh <= 51.0f, "it goes up 37 to 51 degrees");
    Check(backLow >= -0.26f && backHigh <= -0.14f, "a little back");
    Check(right, "out to the right (the room's +x for a gun along -z)");
    Check(unitAxis && axisDown, "it spins about the gun's down, give or take");
    Check(spinLow >= 1.0f - 1e-4f && spinHigh <= 4.0f + 1e-4f, "its spin the emitter's, 1 to 4 rad/s");

    // The eye just behind the port: the game's zoom turn, 9 degrees about y: thrown less back. Far from it: not.
    {
        casings::Throw t;
        t.gun = m416;
        std::memcpy(t.gunRoom, kGunRoom, sizeof(t.gunRoom));
        std::memcpy(t.roomToArms, g_roomToArms, sizeof(t.roomToArms));
        t.seed = 77;
        casings::Start plain, zoomed, offEye;
        casings::ShotStart(t, plain);
        t.haveEye = true;
        const float eyeOnGun[3] = {m416->port[0] + 0.02f, m416->port[1] + 0.03f, m416->port[2] - 0.08f};
        Transform(kGunRoom, eyeOnGun, t.eyeRoom);
        casings::ShotStart(t, zoomed);
        const float offOnGun[3] = {0.0f, 0.1f, -0.3f};
        Transform(kGunRoom, offOnGun, t.eyeRoom);
        casings::ShotStart(t, offEye);
        float a[3], b[3], pa[3], pb[3];
        RotateRowsBack(g_roomToArms, plain.velocity, pa);
        RotateRowsBack(kGunRoom, pa, a);
        RotateRowsBack(g_roomToArms, zoomed.velocity, pb);
        RotateRowsBack(kGunRoom, pb, b);
        // Turned about the gun's y: its bearing across the gun (x and z) 9 degrees on, its rise and its speed as they were.
        const float turned = (std::atan2(-b[0], b[2]) - std::atan2(-a[0], a[2])) * 180.0f / kPi;
        Check(zoomed.zoomed && !plain.zoomed && !offEye.zoomed, "the eye at the sights turns it, the eye 30 cm off doesn't");
        Check(b[2] > a[2] && std::fabs(std::fabs(turned) - 9.0f) < 0.05f && std::fabs(a[1] - b[1]) < 1e-4f && std::fabs(Length3(a) - Length3(b)) < 1e-4f,
              "zoomed it goes 9 degrees further forward about the gun's y, as fast");
    }
    // The pistol's flat throw; the M240's down.
    {
        casings::Throw t;
        t.gun = casings::EmitterFor("m9");
        std::memcpy(t.gunRoom, kGunRoom, sizeof(t.gunRoom));
        std::memcpy(t.roomToArms, g_roomToArms, sizeof(t.roomToArms));
        bool flat = true, down = true;
        for (uint32_t seed = 1; seed <= 200; ++seed) {
            t.seed = seed;
            t.gun = casings::EmitterFor("m9");
            casings::Start s;
            float r[3], g[3];
            flat = flat && casings::ShotStart(t, s) && std::strcmp(s.spec->name, "9mm") == 0;
            RotateRowsBack(g_roomToArms, s.velocity, r);
            RotateRowsBack(kGunRoom, r, g);
            flat = flat && std::fabs(g[1]) < 1e-4f && g[0] < 0.0f;
            t.gun = casings::EmitterFor("m240");
            down = down && casings::ShotStart(t, s) && std::strcmp(s.spec->name, "762") == 0;
            RotateRowsBack(g_roomToArms, s.velocity, r);
            RotateRowsBack(kGunRoom, r, g);
            down = down && g[1] < -0.3f;
        }
        Check(flat, "the pistol's casings go out flat to the right");
        Check(down, "the M240's go down (its effect turned 90 degrees about its z)");
    }
    // Broken numbers start nothing.
    {
        casings::Throw t;
        t.gun = m416;
        std::memcpy(t.gunRoom, kGunRoom, sizeof(t.gunRoom));
        std::memcpy(t.roomToArms, g_roomToArms, sizeof(t.roomToArms));
        t.gunRoom[7] = std::nanf("");
        casings::Start s;
        Check(!casings::ShotStart(t, s), "a broken gun starts nothing");
        t.gunRoom[7] = 1.4f;
        t.gun = nullptr;
        Check(!casings::ShotStart(t, s), "no emitter starts nothing");
        t.gun = m416;
        const float zero[12] = {};
        std::memcpy(t.gunRoom, zero, sizeof(t.gunRoom));
        Check(!casings::ShotStart(t, s), "a gun with no axes starts nothing");
    }
}

// ---- the shots
void TestRounds() {
    Check(casings::RoundsFired(30, 29, true) == 1, "one round down: one shot");
    Check(casings::RoundsFired(30, 28, true) == 2, "two in a frame: two");
    Check(casings::RoundsFired(30, 27, true) == 3, "three: three");
    Check(casings::RoundsFired(30, 25, true) == 0, "five: a magazine let go of, not shots");
    Check(casings::RoundsFired(30, 29, false) == 0, "no trigger: no shot (a magazine out with two left)");
    Check(casings::RoundsFired(-1, 29, true) == 0 && casings::RoundsFired(30, -1, true) == 0, "a count not known: none");
    Check(casings::RoundsFired(29, 30, true) == 0 && casings::RoundsFired(30, 30, true) == 0, "up or the same: none");
}

// ---- the placements
void TestPack() {
    // A bone turned 40 degrees about a slanted axis, and a fit turning y into z, scaled and moved.
    const float axis[3] = {0.3f, 0.8f, -0.52f};
    const float l = Length3(axis), n[3] = {axis[0] / l, axis[1] / l, axis[2] / l};
    const float a = 40.0f * kPi / 180.0f, c = std::cos(a), s = std::sin(a), t = 1.0f - c;
    const float bone[12] = {t * n[0] * n[0] + c, t * n[0] * n[1] - s * n[2], t * n[0] * n[2] + s * n[1], 0.25f,
                            t * n[0] * n[1] + s * n[2], t * n[1] * n[1] + c, t * n[1] * n[2] - s * n[0], -1.1f,
                            t * n[0] * n[2] - s * n[1], t * n[1] * n[2] + s * n[0], t * n[2] * n[2] + c, 0.6f};
    const float fit[12] = {1.5f, 0.0f, 0.0f, -0.003f, 0.0f, 0.0f, -1.5f, 0.001f, 0.0f, 1.5f, 0.0f, -0.02f};
    const int origin[3] = {1000, -20, 3000};
    float rows[12];
    casings::PackInstance(bone, origin, fit, rows);
    bool same = true;
    const float points[3][3] = {{0.0f, 0.0f, 0.0f}, {0.01f, -0.02f, 0.03f}, {-0.004f, 0.045f, 0.002f}};
    for (const float* p : points) {
        float spec[3], arms[3], world[3];
        Transform(fit, p, spec);
        Transform(bone, spec, arms);
        Transform(rows, p, world);
        for (int i = 0; i < 3; ++i)  // (a float 3 km out is good to a quarter of a millimetre, as the game's own)
            same = same && std::fabs(world[i] - (arms[i] + static_cast<float>(origin[i]))) < 5e-4f;
    }
    Check(same, "a placement takes the mesh's vertices through its fit and its bone into the world");

    // By kind: a magazine (1), 5.56 (10), 9 mm (13), 5.56, a 5.45 whose fit isn't known (11), a broken 5.56.
    loose::Pose poses[6];
    const int kindsOf[6] = {1, 10, 13, 10, 11, 10};
    for (int i = 0; i < 6; ++i) {
        poses[i].id = i + 1;
        poses[i].kind = kindsOf[i];
        const float b[12] = {1, 0, 0, static_cast<float>(i), 0, 1, 0, 0, 0, 0, 1, 0};
        std::memcpy(poses[i].bone, b, sizeof(b));
    }
    poses[5].bone[7] = std::nanf("");
    float fits[casings::kKinds][12];
    const float identity[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    for (auto& f : fits)
        std::memcpy(f, identity, sizeof(identity));
    const bool known[casings::kKinds] = {true, false, true, true};
    const int zero[3] = {};
    float packed[12 * 8] = {};
    int first[casings::kKinds], count[casings::kKinds];
    const int all = casings::PackAll(poses, 6, zero, fits, known, packed, 8, first, count);
    Check(all == 3 && count[casings::k556] == 2 && count[casings::k545] == 0 && count[casings::k762] == 0 && count[casings::k9] == 1 &&
              first[casings::k556] == 0 && first[casings::k9] == 2,
          "packed by kind: the 5.56s first, the 9 mm after them; no magazine, no unknown fit, no broken one");
    Check(packed[3] == 1.0f && packed[12 + 3] == 3.0f && packed[24 + 3] == 2.0f, "each in its kind's place, in the order loose gave them");
    const int fewer = casings::PackAll(poses, 6, zero, fits, known, packed, 2, first, count);
    Check(fewer == 2 && count[casings::k556] == 2 && count[casings::k9] == 0, "no more than room for");
}

// ---- the mesh's fit
// A 5.56 casing as a mesh of rings (8 vertices round), its base at the top (y 0.02), its mouth down along -y, its middle off the axes.
std::vector<float> CasingMesh(int axis, bool baseAtMax, float scale, const float* rings, const float* radii, int ringCount) {
    std::vector<float> places;
    const float centre[3] = {0.003f, 0.02f, -0.001f};
    for (int r = 0; r < ringCount; ++r)
        for (int k = 0; k < 8; ++k) {
            const float angle = 2.0f * kPi * k / 8.0f;
            float p[3];
            const int u = (axis + 1) % 3, w = (axis + 2) % 3;
            p[axis] = centre[axis] + (baseAtMax ? -rings[r] : rings[r]) / scale;
            p[u] = centre[u] + radii[r] * std::cos(angle) / scale;
            p[w] = centre[w] + radii[r] * std::sin(angle) / scale;
            places.insert(places.end(), p, p + 3);
        }
    return places;
}

void TestFit() {
    const float rings[5] = {0.0f, 0.001f, 0.036f, 0.040f, 0.0447f}, radii[5] = {0.0048f, 0.0048f, 0.0045f, 0.0032f, 0.0032f};
    for (int axis = 0; axis < 3; ++axis)
        for (int high = 0; high < 2; ++high)
            for (float scale : {1.0f, 2.0f}) {
                const std::vector<float> mesh = CasingMesh(axis, high != 0, scale, rings, radii, 5);
                casings::MeshFit fit;
                const bool ok = casings::FitMesh(mesh.data(), static_cast<int>(mesh.size() / 3), scale, fit);
                char what[160];
                std::snprintf(what, sizeof(what), "a 5.56 casing along %c from its %s end, drawn %.0f times, is read", "xyz"[axis], high ? "highest" : "lowest", scale);
                Check(ok && fit.axis == axis && fit.baseAtMax == (high != 0) && !fit.ambiguous, what);
                if (!ok)
                    continue;
                // Its base's middle to 0, its mouth's to 44.7 mm along +z, a rim vertex 4.8 mm out; a turn, scaled.
                const float centre[3] = {0.003f, 0.02f, -0.001f};
                float base[3] = {centre[0], centre[1], centre[2]}, mouthMiddle[3] = {centre[0], centre[1], centre[2]};
                mouthMiddle[axis] += (high ? -0.0447f : 0.0447f) / scale;
                float b[3], m[3], rim[3];
                Transform(fit.specFromMesh, base, b);
                Transform(fit.specFromMesh, mouthMiddle, m);
                Transform(fit.specFromMesh, mesh.data(), rim);
                std::snprintf(what, sizeof(what), "... and fitted: base at 0, mouth 44.7 mm along +z, rim 4.8 mm out (along %c, %s, x%.0f)", "xyz"[axis],
                              high ? "highest" : "lowest", scale);
                Check(Length3(b) < 1e-5f && std::fabs(m[0]) < 1e-5f && std::fabs(m[1]) < 1e-5f && std::fabs(m[2] - 0.0447f) < 1e-5f &&
                          std::fabs(std::sqrt(rim[0] * rim[0] + rim[1] * rim[1]) - 0.0048f) < 1e-5f && std::fabs(rim[2]) < 1e-5f &&
                          std::fabs(fit.length - 0.0447f) < 1e-5f && std::fabs(fit.baseRadius - 0.0048f) < 1e-5f && std::fabs(fit.mouthRadius - 0.0032f) < 1e-5f,
                      what);
                float unscaled[12];
                for (int i = 0; i < 12; ++i)
                    unscaled[i] = fit.specFromMesh[i] / scale;
                Check(Orthonormal(unscaled), "its fit's turn is a turn (right-handed)");
            }
    // A straight one (the 9 mm's, rim and mouth alike): its ends taken as they come, the lowest for its base.
    {
        const float straight[3] = {0.0f, 0.010f, 0.0192f}, same[3] = {0.0049f, 0.0049f, 0.0049f};
        const std::vector<float> mesh = CasingMesh(2, true, 1.0f, straight, same, 3);
        casings::MeshFit fit;
        Check(casings::FitMesh(mesh.data(), static_cast<int>(mesh.size() / 3), 1.0f, fit) && fit.ambiguous && !fit.baseAtMax, "a straight casing: its ends alike");
    }
    // Not a casing: a flat box (a belt link), nothing, one too long, garbage.
    {
        std::vector<float> box;
        for (int i = 0; i < 8; ++i) {
            const float p[3] = {(i & 1) ? 0.011f : -0.011f, (i & 2) ? 0.0075f : -0.0075f, (i & 4) ? 0.004f : -0.004f};
            box.insert(box.end(), p, p + 3);
        }
        casings::MeshFit fit;
        Check(!casings::FitMesh(box.data(), 8, 1.0f, fit), "a flat box isn't a casing");
        Check(!casings::FitMesh(box.data(), 0, 1.0f, fit) && !casings::FitMesh(nullptr, 8, 1.0f, fit), "nothing isn't one");
        const std::vector<float> big = CasingMesh(1, false, 0.1f, rings, radii, 5);  // 45 cm drawn
        Check(!casings::FitMesh(big.data(), static_cast<int>(big.size() / 3), 1.0f, fit), "45 cm long isn't one");
        std::vector<float> garbage;
        for (int i = 0; i < 40 * 3; ++i)
            garbage.push_back(static_cast<float>((i * 7919) % 1000) * 1e3f);
        Check(!casings::FitMesh(garbage.data(), 40, 1.0f, fit), "garbage isn't one");
        garbage[5] = std::nanf("");
        Check(!casings::FitMesh(garbage.data(), 40, 1.0f, fit), "a NaN isn't one");
    }
}

void TestHalves() {
    Check(casings::HalfToFloat(0x3C00) == 1.0f && casings::HalfToFloat(0xC000) == -2.0f && casings::HalfToFloat(0x0000) == 0.0f, "halves: 1, -2, 0");
    Check(std::fabs(casings::HalfToFloat(0x3555) - 0.33325195f) < 1e-7f && casings::HalfToFloat(0x7BFF) == 65504.0f, "halves: a third, the largest");
    Check(casings::HalfToFloat(0x0001) == std::ldexp(1.0f, -24) && std::isinf(casings::HalfToFloat(0x7C00)) && std::isnan(casings::HalfToFloat(0x7E00)),
          "halves: the smallest, infinity, NaN");
    Check(std::fabs(casings::HalfToFloat(0x2266) - 0.012497f) < 2e-5f, "halves: 12.5 mm");
}

// ---- flown by loose.cpp
struct Ground {
    int surface;
};
int Flat(void* user, const float* at, loose::Plane* planes, int most) {
    if (most < 1)
        return 0;
    const Ground* g = static_cast<const Ground*>(user);
    planes[0].point[0] = at[0], planes[0].point[1] = 0.0f, planes[0].point[2] = at[2];
    planes[0].normal[0] = 0.0f, planes[0].normal[1] = 1.0f, planes[0].normal[2] = 0.0f;
    planes[0].surface = g->surface;
    planes[0].firm = true;
    return 1;
}

// The casing's lowest point (its rings' lowest), its middle, and how level it lies (its axis' rise), from its pose.
void Lowest(const loose::Spec& spec, const float* bone, float& lowest, float* middle, float& rise) {
    const float axis[3] = {bone[2], bone[6], bone[10]};
    const float across = std::sqrt(std::max(0.0f, 1.0f - axis[1] * axis[1]));
    lowest = 1e9f;
    for (int i = 0; i < spec.rings; ++i) {
        const float at[3] = {0.0f, 0.0f, spec.ringAt[i]};
        float c[3];
        Transform(bone, at, c);
        lowest = std::min(lowest, c[1] - spec.ringRadius[i] * across);
    }
    const float half[3] = {0.0f, 0.0f, 0.5f * (spec.ringAt[0] + spec.ringAt[spec.rings - 1])};
    Transform(bone, half, middle);
    rise = std::fabs(axis[1]);
}

struct Flight {
    bool landed = false, rested = false;
    float landAt = 0.0f, restAt = 0.0f, right = 0.0f, back = 0.0f, away = 0.0f, deepest = 0.0f, bounce = 0.0f, rise = 1.0f;
};

void Fly(int surface, int throws, std::vector<Flight>& out, double& stepMicros) {
    const casings::Emitter* m416 = casings::EmitterFor("m416");
    Ground ground{surface};
    loose::Frame f;
    f.dt = 1.0f / 72.0f;
    const float identity[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    std::memcpy(f.roomToArms, identity, sizeof(identity));
    f.gun = true;
    std::memcpy(f.gunRoom, kGunRoom, sizeof(f.gunRoom));
    f.eye[0] = 0.0f, f.eye[1] = 1.6f, f.eye[2] = 0.1f;
    f.look[0] = 0.0f, f.look[1] = -0.5f, f.look[2] = -0.866f;
    f.floor = &Flat;
    f.floorUser = &ground;
    float portRoom[3];
    Transform(kGunRoom, m416->port, portRoom);
    LARGE_INTEGER frequency, t0, t1;
    QueryPerformanceFrequency(&frequency);
    int64_t ticks = 0, steps = 0;
    for (int n = 0; n < throws; ++n) {
        loose::Clear();
        for (int k = 0; k < 6; ++k)  // the gun held still a few frames: its own speed nothing
            loose::Step(f);
        casings::Throw t;
        t.gun = m416;
        std::memcpy(t.gunRoom, f.gunRoom, sizeof(t.gunRoom));
        std::memcpy(t.roomToArms, f.roomToArms, sizeof(t.roomToArms));
        t.seed = static_cast<uint32_t>(n + 1) * 2654435761u;
        casings::Start s;
        Flight flight;
        if (!casings::ShotStart(t, s)) {
            out.push_back(flight);
            continue;
        }
        const int id = loose::Flick(*s.spec, s.kind, s.bone, 0, s.velocity, s.axis, s.spin, f);
        float lastLowest = 1e9f;
        bool rising = false;
        for (int frame = 1; frame <= 216; ++frame) {  // 3 s
            QueryPerformanceCounter(&t0);
            const loose::Result& r = loose::Step(f);
            QueryPerformanceCounter(&t1);
            ticks += t1.QuadPart - t0.QuadPart, ++steps;
            const loose::Pose* p = nullptr;
            for (int i = 0; i < r.poses; ++i)
                if (r.pose[i].id == id)
                    p = &r.pose[i];
            if (!p)
                break;
            float lowest, middle[3], rise;
            Lowest(*s.spec, p->bone, lowest, middle, rise);
            const float seconds = frame / 72.0f;
            // Its first touch: loose's own first landing (between frames: by the frame's end it may be back up), or found lying.
            const float* touched = nullptr;
            for (int h = 0; h < r.hits; ++h)
                if (r.hit[h].id == id && r.hit[h].first)
                    touched = r.hit[h].at;
            if (!touched && lowest < 0.002f)
                touched = middle;
            if (!flight.landed && touched) {
                flight.landed = true;
                flight.landAt = seconds;
                flight.right = touched[0] - portRoom[0];
                flight.back = touched[2] - portRoom[2];
                flight.away = std::sqrt(flight.right * flight.right + flight.back * flight.back);
            }
            if (flight.landed) {
                flight.deepest = std::max(flight.deepest, -lowest);
                // Its first bounce: how high its lowest point gets once it's left the ground again, until it comes back.
                if (lowest > lastLowest + 1e-5f)
                    rising = true;
                if (rising && flight.rested == false)
                    flight.bounce = std::max(flight.bounce, lowest);
            }
            lastLowest = lowest;
            if (p->resting && !flight.rested) {
                flight.rested = true;
                flight.restAt = seconds;
            }
            flight.rise = rise;
        }
        out.push_back(flight);
    }
    stepMicros = steps ? 1e6 * static_cast<double>(ticks) / static_cast<double>(frequency.QuadPart) / static_cast<double>(steps) : 0.0;
}

float Quantile(std::vector<float> v, float q) {
    if (v.empty())
        return 0.0f;
    std::sort(v.begin(), v.end());
    return v[std::min(v.size() - 1, static_cast<size_t>(q * static_cast<float>(v.size() - 1) + 0.5f))];
}

void TestFlight() {
    for (int surface : {loose::kDirt, loose::kTarmac}) {
        const char* name = surface == loose::kDirt ? "dirt" : "tarmac";
        std::vector<Flight> flights;
        double micros = 0.0;
        Fly(surface, surface == loose::kDirt ? 1000 : 300, flights, micros);
        std::vector<float> away, landAt, restAt, deepest, bounce, rise;
        int landed = 0, right = 0, back = 0, rested = 0, side = 0;
        for (const Flight& fl : flights) {
            if (!fl.landed)
                continue;
            ++landed;
            right += fl.right > 0.0f;
            back += fl.back > -0.05f;
            away.push_back(fl.away), landAt.push_back(fl.landAt), deepest.push_back(fl.deepest), bounce.push_back(fl.bounce);
            if (fl.rested) {
                ++rested;
                restAt.push_back(fl.restAt);
                rise.push_back(fl.rise);
                side += fl.rise < 0.2f;
            }
        }
        const float n = static_cast<float>(flights.size());
        std::printf("On %s: %d of %zu landed, %.0f%% to the right, %.0f%% not in front; %.2f to %.2f m away (median %.2f), first touching at %.2f to %.2f s; "
                    "%.2f mm into the ground at the most; first bounce %.1f cm (median), %.1f at the most; at rest %d, by %.2f s for 99%%, %d on their side; "
                    "a step %.1f us\n",
                    name, landed, flights.size(), 100.0f * right / std::max(landed, 1), 100.0f * back / std::max(landed, 1), Quantile(away, 0.0f),
                    Quantile(away, 1.0f), Quantile(away, 0.5f), Quantile(landAt, 0.0f), Quantile(landAt, 1.0f), 1000.0f * Quantile(deepest, 1.0f),
                    100.0f * Quantile(bounce, 0.5f), 100.0f * Quantile(bounce, 1.0f), rested, Quantile(restAt, 0.99f), side, micros);
        char what[160];
        std::snprintf(what, sizeof(what), "on %s every casing lands, to the right, not in front of the port", name);
        Check(landed == static_cast<int>(n) && right == landed && back == landed, what);
        std::snprintf(what, sizeof(what), "on %s they land 0.3 to 3 m from the port, first touching 0.3 to 1.1 s on", name);
        Check(Quantile(away, 0.0f) >= 0.3f && Quantile(away, 1.0f) <= 3.0f && Quantile(landAt, 0.0f) >= 0.3f && Quantile(landAt, 1.0f) <= 1.1f, what);
        std::snprintf(what, sizeof(what), "on %s never more than 0.6 mm into the ground (loose's slop is 0.5)", name);
        Check(Quantile(deepest, 1.0f) <= 0.0006f, what);
        std::snprintf(what, sizeof(what), "on %s nearly all at rest in 3 s, lying on their side", name);
        Check(rested >= static_cast<int>(0.95f * n) && side >= static_cast<int>(0.95f * static_cast<float>(rested)), what);
    }
    // What a step costs: 50 in the air at once, then 192 at rest.
    {
        const casings::Emitter* m416 = casings::EmitterFor("m416");
        Ground ground{loose::kDirt};
        loose::Frame f;
        f.dt = 1.0f / 72.0f;
        const float identity[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
        std::memcpy(f.roomToArms, identity, sizeof(identity));
        f.gun = true;
        std::memcpy(f.gunRoom, kGunRoom, sizeof(f.gunRoom));
        f.eye[1] = 1.6f, f.look[2] = -1.0f;
        f.floor = &Flat;
        f.floorUser = &ground;
        loose::Clear();
        LARGE_INTEGER frequency, t0, t1;
        QueryPerformanceFrequency(&frequency);
        auto throwSome = [&](int from, int to) {
            for (int n = from; n < to; ++n) {
                casings::Throw t;
                t.gun = m416;
                std::memcpy(t.gunRoom, f.gunRoom, sizeof(t.gunRoom));
                std::memcpy(t.roomToArms, f.roomToArms, sizeof(t.roomToArms));
                t.seed = static_cast<uint32_t>(n) + 5000u;
                casings::Start s;
                if (casings::ShotStart(t, s))
                    loose::Flick(*s.spec, s.kind, s.bone, 0, s.velocity, s.axis, s.spin, f);
            }
        };
        double mean = 0.0;
        auto worstStep = [&](int steps) {
            double worst = 0.0, sum = 0.0;
            for (int k = 0; k < steps; ++k) {
                QueryPerformanceCounter(&t0);
                loose::Step(f);
                QueryPerformanceCounter(&t1);
                const double us = 1e6 * static_cast<double>(t1.QuadPart - t0.QuadPart) / static_cast<double>(frequency.QuadPart);
                worst = std::max(worst, us), sum += us;
            }
            mean = sum / steps;
            return worst;
        };
        throwSome(0, 50);
        loose::Step(f);
        const double flying = worstStep(72);  // 50 in the air, then landing
        const double flyingMean = mean;
        throwSome(50, loose::kCasings);
        for (int k = 0; k < 72 * 8; ++k)
            loose::Step(f);
        const int lying = loose::Resting();
        const double resting = worstStep(72);
        std::printf("A step with 50 casings thrown at once, their first second: %.0f us on average, %.0f at the most; with %d at rest: %.0f us on average, %.0f at "
                    "the most\n",
                    flyingMean, flying, lying, mean, resting);
        Check(lying >= static_cast<int>(0.95f * loose::kCasings), "a full pool comes to rest");
        loose::Clear();
    }
}

}  // namespace

int main() {
    MakeRoomToArms();
    TestThrow();
    TestRounds();
    TestPack();
    TestFit();
    TestHalves();
    TestFlight();
    std::printf("casingtest: %d of %d checks failed\n", g_failed, g_checks);
    return g_failed;
}

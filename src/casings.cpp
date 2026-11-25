// The mod's own spent casings (casings.h): loose.cpp's small rigid bodies, drawn with the game's own casing draw.
//
// How the game draws its own: each casing draw is a DrawIndexedInstanced of its mesh (stream 0, 32-byte vertices) with each casing's placement in
// stream 1 (48 bytes: three rows, rotation | its place in the world), in every pass: both eyes' pictures of the world (their G-buffers, three or more
// targets) and the sun's cascades (effects.cpp:292-305; the census, 2026-10-30, 2026-11-02: stride 32, stencil 2). Drawn in an eye's
// picture it has that eye's own view constants. So the mod's are that draw again, its states kept, its placements the mod's:
//   - Kept (KeepDraw): one of the game's casing draws in an eye's picture, every 10 s: its input layout, topology, index buffer and mesh, the draw's
//     count, start and base; its vertex and pixel shaders, their constant buffers (copied on the graphics card as they were for that draw, but the
//     view's, each pass's own), textures and samplers; its rasterizer, blend and depth-stencil states. Every one referenced, let go of in a new world.
//     If it tests depth EQUAL without writing (a depth pass drew it first), the depth pass's draw of it is kept as well and drawn first.
//   - Armed (NoteClear): an eye's depth target (the kept draw's size, samples and format) cleared, depth and stencil, on a context, with casings in view.
//   - Hosted (Host): just before that context's first draw of the pass on that depth (three or more targets, as the kept draw's; depth-tested; not
//     first person), the casings of each kind in one instanced draw with the kept states and that pass's own view, everything touched put back as it
//     was. Once for each arming, and the same eye's view never twice within 8 ms. If the casings are in view 2 s and no pass is drawn into, an arming
//     goes to whichever context draws into that depth first (the clear and the pass recorded on different threads); 2 s more, and the game's own come
//     back for 20 s.
//   - Their placements (SetLoose): from loose's poses once a frame, each through its mesh's fit (read back from the mesh's own vertices once, stamped:
//     FitMesh), uploaded on the context that draws them, just before.
//   - The game's own (HideGameCasings): from the trigger to 1.7 s after the last shot, the game's casing draws of the gun in hand's kind drawn with the
//     ones within 3 m of the game's ejection port folded to a point (design D: what effects.cpp's compute shader would do with its "own" flag).
// Nothing kept from or drawn into the view through a scope (scopeview::Drawing). Logs only from Update (the render thread at Present); the draw hooks
// count.
#include "casings.h"

#include <windows.h>
#include <d3dcompiler.h>

#include <algorithm>
#include <atomic>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <mutex>
#include <vector>

#include "ammo.h"
#include "camera.h"
#include "log.h"
#include "loose.h"
#include "scopeview.h"
#include "skin.h"

namespace casings {
std::atomic<int> g_armed{0};
}

namespace {

using casings::Emitter;
using casings::Kind;
using casings::kKinds;

// ---------------------------------------------------------------- small maths (3x4 rows: rotation | translation; 3x3 row-major)
constexpr float kPi = 3.14159265f;
constexpr float kDegree = kPi / 180.0f;

bool Finite(const float* p, int n) {
    for (int i = 0; i < n; ++i)
        if (!std::isfinite(p[i]))
            return false;
    return true;
}
float Length3(const float* v) {
    return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}
bool Normalize3(float* v) {
    const float l = Length3(v);
    if (!(l > 1e-9f))
        return false;
    v[0] /= l, v[1] /= l, v[2] /= l;
    return true;
}
void Cross3(const float* a, const float* b, float* out) {
    const float x = a[1] * b[2] - a[2] * b[1], y = a[2] * b[0] - a[0] * b[2], z = a[0] * b[1] - a[1] * b[0];
    out[0] = x, out[1] = y, out[2] = z;
}
float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}
// out = m v (m 3x3).
void Rotate3(const float* m, const float* v, float* out) {
    const float x = m[0] * v[0] + m[1] * v[1] + m[2] * v[2], y = m[3] * v[0] + m[4] * v[1] + m[5] * v[2], z = m[6] * v[0] + m[7] * v[1] + m[8] * v[2];
    out[0] = x, out[1] = y, out[2] = z;
}
void Mul3(const float* a, const float* b, float* out) {
    float o[9];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            o[i * 3 + j] = a[i * 3] * b[j] + a[i * 3 + 1] * b[3 + j] + a[i * 3 + 2] * b[6 + j];
    std::memcpy(out, o, sizeof(o));
}
void RotationOf(const float* rows, float* m) {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            m[i * 3 + j] = rows[i * 4 + j];
}
// Affine 3x4s: out = a b (skin.cpp's and hands.cpp's Compose).
void Compose(const float* a, const float* b, float* out) {
    float o[12];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j)
            o[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        o[i * 4 + 3] += a[i * 4 + 3];
    }
    std::memcpy(out, o, sizeof(o));
}
void Transform(const float* m, const float* p, float* out) {
    const float x = m[0] * p[0] + m[1] * p[1] + m[2] * p[2] + m[3], y = m[4] * p[0] + m[5] * p[1] + m[6] * p[2] + m[7],
                z = m[8] * p[0] + m[9] * p[1] + m[10] * p[2] + m[11];
    out[0] = x, out[1] = y, out[2] = z;
}
// A bone's axes (its rotation's columns) made unit and square, x kept: rows as a 3x3. False: broken.
bool SquareAxes(const float* rows, float* m) {
    float x[3] = {rows[0], rows[4], rows[8]}, y[3] = {rows[1], rows[5], rows[9]}, z[3];
    if (!Normalize3(x))
        return false;
    Cross3(x, y, z);
    if (!Normalize3(z))
        return false;
    Cross3(z, x, y);
    for (int i = 0; i < 3; ++i)
        m[i * 3] = x[i], m[i * 3 + 1] = y[i], m[i * 3 + 2] = z[i];
    return true;
}
// A turn of degrees about x, y and z, as the game's LinearTransform from its angles is taken to be: about z first, then x, then y (yaw, pitch and roll;
// one axis at a time it makes no difference, and only the M240's second effect has two).
void TurnOf(const float* degrees, float* m) {
    const float cx = std::cos(degrees[0] * kDegree), sx = std::sin(degrees[0] * kDegree);
    const float cy = std::cos(degrees[1] * kDegree), sy = std::sin(degrees[1] * kDegree);
    const float cz = std::cos(degrees[2] * kDegree), sz = std::sin(degrees[2] * kDegree);
    const float rx[9] = {1, 0, 0, 0, cx, -sx, 0, sx, cx}, ry[9] = {cy, 0, sy, 0, 1, 0, -sy, 0, cy}, rz[9] = {cz, -sz, 0, sz, cz, 0, 0, 0, 1};
    float yx[9];
    Mul3(ry, rx, yx);
    Mul3(yx, rz, m);
}

// A small seeded random (xorshift32), 0 to 1.
struct Random {
    uint32_t state;
    explicit Random(uint32_t seed) : state(seed * 2654435761u + 0x9E3779B9u) {
        if (!state)
            state = 1;
        Next();
    }
    float Next() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<float>(state >> 8) * (1.0f / 16777216.0f);
    }
};

// ---------------------------------------------------------------- the game's numbers
// The first-person shell effects, from the public Venice EBX dump of the game's data (the multiplayer build; the singleplayer files share these
// effects but weren't read: weapon.txt "flashscan" in the game is to settle them), read in the design study of 2026-11-05:
//   - The M416: FX/Weapons/Shells/FX_ShellEjection_M at its FireEffectData offset (-0.020 0.034 0.40), turned 9 degrees about y zoomed
//     (UseZoomRotation). Its casing emitter: SpawnDirection's random x -1 to -0.8, y 0.8 to 1, z -0.3 to -0.2 (out to the right, up, a little
//     back, on the gun's x left, y up, z forward); SpawnSpeed 2.0 with a random 1.5 to 3.0; SpawnRotationSpeed 2.0 with a random 1 to 4; its age
//     0.75 s. The M16A4 the same effect at (-0.013 0.026 0.44); the AK-74M FX_ShellEjection_M_Russian at (-0.012 0.035 0.435), a random 1 to 3, 1.5 s;
//     the G3A3 FX_ShellEjection_762 at (-0.02 0.062 0.468), 1 to 3, 1.5 s; the M240's casings FX_ShellEjection_762 at (0 -0.02 0.60) turned (0 -15 90),
//     thrown down (its links are another mesh, no loose shape: left to the game); the train's pistol's _S flat, x -1 to -0.6, y 0, z -0.5 to -0.3,
//     1 to 2.5, 0.75 s.
//   - Each random brackets its emitter's own value (1.5 to 3 round 2.0, 1 to 4 round 2.0, 1 to 2.5 round 2.0), as a value randomised round itself is:
//     so the random alone is the speed here (1.5 to 3 m/s for the M416: landing 0.7 to 1.7 m away from 1.4 m up), not 2 times it (3 to 6 m/s);
//     effects.cpp:300-301 saw "about 3 m/s". weapon.txt "casingsspeed 2" is the other reading. Spins likewise (1 to 4 rad/s, a gentle tumble).
//   - Not in the dump's study: the other AK family's, the KH2002's and the M4A1's, taken as their kind's, at their port as skin.cpp's kHeldKinds has
//     it (skin.cpp:7631-7647); the directions of _M_Russian and _762, taken as _M's.
// Ports on the gun's frame bone: the M416's model space (its body bone's: skin.cpp:3604-3605, 9061), the held guns' frames (the model's axes:
// skin.cpp:7584), the pistol's frame (skin.cpp:9061, where its casings first show, 2026-11-02).
constexpr Emitter kEmitters[] = {
    {"m416", casings::k556, {-0.020f, 0.034f, 0.40f}, {0.0f, 0.0f, 0.0f}, {0.0f, 9.0f, 0.0f}, {-1.0f, 0.8f, -0.3f}, {-0.8f, 1.0f, -0.2f}, 1.5f, 3.0f, 1.0f, 4.0f, 0.75f},
    {"m16a4", casings::k556, {-0.013f, 0.026f, 0.44f}, {0.0f, 0.0f, 0.0f}, {0.0f, 9.0f, 0.0f}, {-1.0f, 0.8f, -0.3f}, {-0.8f, 1.0f, -0.2f}, 1.5f, 3.0f, 1.0f, 4.0f, 0.75f},
    {"m4a1", casings::k556, {-0.018f, 0.02f, 0.43f}, {0.0f, 0.0f, 0.0f}, {0.0f, 9.0f, 0.0f}, {-1.0f, 0.8f, -0.3f}, {-0.8f, 1.0f, -0.2f}, 1.5f, 3.0f, 1.0f, 4.0f, 0.75f},
    {"kh2002", casings::k556, {-0.027f, 0.012f, 0.15f}, {0.0f, 0.0f, 0.0f}, {0.0f, 9.0f, 0.0f}, {-1.0f, 0.8f, -0.3f}, {-0.8f, 1.0f, -0.2f}, 1.5f, 3.0f, 1.0f, 4.0f, 0.75f},
    {"ak74", casings::k545, {-0.012f, 0.035f, 0.435f}, {0.0f, 0.0f, 0.0f}, {0.0f, 9.0f, 0.0f}, {-1.0f, 0.8f, -0.3f}, {-0.8f, 1.0f, -0.2f}, 1.0f, 3.0f, 1.0f, 4.0f, 1.5f},
    {"an94", casings::k545, {-0.017f, 0.06f, 0.33f}, {0.0f, 0.0f, 0.0f}, {0.0f, 9.0f, 0.0f}, {-1.0f, 0.8f, -0.3f}, {-0.8f, 1.0f, -0.2f}, 1.0f, 3.0f, 1.0f, 4.0f, 1.5f},
    {"aek971", casings::k545, {-0.017f, 0.05f, 0.36f}, {0.0f, 0.0f, 0.0f}, {0.0f, 9.0f, 0.0f}, {-1.0f, 0.8f, -0.3f}, {-0.8f, 1.0f, -0.2f}, 1.0f, 3.0f, 1.0f, 4.0f, 1.5f},
    {"rpk", casings::k545, {-0.015f, 0.05f, 0.30f}, {0.0f, 0.0f, 0.0f}, {0.0f, 9.0f, 0.0f}, {-1.0f, 0.8f, -0.3f}, {-0.8f, 1.0f, -0.2f}, 1.0f, 3.0f, 1.0f, 4.0f, 1.5f},
    {"g3a3", casings::k762, {-0.02f, 0.062f, 0.468f}, {0.0f, 0.0f, 0.0f}, {0.0f, 9.0f, 0.0f}, {-1.0f, 0.8f, -0.3f}, {-0.8f, 1.0f, -0.2f}, 1.0f, 3.0f, 1.0f, 4.0f, 1.5f},
    {"m240", casings::k762, {0.0f, -0.02f, 0.60f}, {0.0f, -15.0f, 90.0f}, {0.0f, -15.0f, 90.0f}, {-1.0f, 0.8f, -0.3f}, {-0.8f, 1.0f, -0.2f}, 1.0f, 3.0f, 1.0f, 4.0f, 1.5f},
    {"m9", casings::k9, {-0.01f, 0.04f, 0.13f}, {0.0f, 0.0f, 0.0f}, {0.0f, 9.0f, 0.0f}, {-1.0f, 0.0f, -0.5f}, {-0.6f, 0.0f, -0.3f}, 1.0f, 2.5f, 1.0f, 4.0f, 0.75f},
    // The AKS-74u (the dump, 2026-11-18): FX_ShellEjection_M_Russian, the AK-74M's own effect, at (-0.0142 0.0394 0.4108) with no turn.
    {"aks74u", casings::k545, {-0.014f, 0.039f, 0.411f}, {0.0f, 0.0f, 0.0f}, {0.0f, 9.0f, 0.0f}, {-1.0f, 0.8f, -0.3f}, {-0.8f, 1.0f, -0.2f}, 1.0f, 3.0f, 1.0f, 4.0f, 1.5f},
    // The UMP-45 (the dump, 2026-11-18): FX_ShellEjection_45cal at (-0.02 0.034 0.42), turned 45 degrees about y, 60 zoomed (UseZoomRotation). The effect's own
    // emitter wasn't read: its throw taken as the pistol's _S, and its brass as the 9 mm's, the nearest the table has (a .45's case is 2 mm wider).
    {"ump45", casings::k9, {-0.02f, 0.034f, 0.42f}, {0.0f, 45.0f, 0.0f}, {0.0f, 60.0f, 0.0f}, {-1.0f, 0.0f, -0.5f}, {-0.6f, 0.0f, -0.3f}, 1.0f, 2.5f, 1.0f, 4.0f, 0.75f},
    // The Mk11 (the dump, 2026-11-18): FX_ShellEjection_762, the G3A3's own effect, at (-0.0203 0.0232 0.4458) with no turn and no zoom turn (UseZoomRotation false).
    {"mk11", casings::k762, {-0.020f, 0.023f, 0.446f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {-1.0f, 0.8f, -0.3f}, {-0.8f, 1.0f, -0.2f}, 1.0f, 3.0f, 1.0f, 4.0f, 1.5f},
    // The M249 (the dump, 2026-11-18): FX_ShellEjection_M, the M416's own effect, at (-0.0304 0.0419 0.4217) with no turn, 9 degrees about y zoomed
    // (UseZoomRotation), so the M416's throw. (Its links, FX_BeltLinkEjection_M at (-0.035 0.042 0.42), are the game's own effect: no loose shape.)
    {"m249", casings::k556, {-0.030f, 0.042f, 0.422f}, {0.0f, 0.0f, 0.0f}, {0.0f, 9.0f, 0.0f}, {-1.0f, 0.8f, -0.3f}, {-0.8f, 1.0f, -0.2f}, 1.5f, 3.0f, 1.0f, 4.0f, 0.75f},
    // The PKP (the dump, Weapons/Pecheneg/Pecheneg): FX_ShellEjection_762, the G3A3's own effect, at (0.0204 0.0094 0.4796) on its LEFT, turned (10.6 -1.2 -155)
    // and not again zoomed (UseZoomRotation false): the _762's throw out to the right and up, turned 155 degrees about z, goes out to its left and a little
    // down, as a PKM's cases go. 7.62 brass (its 54R case is 1 mm longer: the table's nearest). (Its links, FX_BeltLinkEjection_M at (0.0225 0.0691 0.5582),
    // are the game's own effect: no loose shape.)
    {"pkp", casings::k762, {0.020f, 0.009f, 0.480f}, {10.568f, -1.214f, -155.0f}, {10.568f, -1.214f, -155.0f}, {-1.0f, 0.8f, -0.3f}, {-0.8f, 1.0f, -0.2f}, 1.0f, 3.0f, 1.0f, 4.0f, 1.5f},
};
// loose.cpp's shapes for each kind (loose.cpp:382-389).
constexpr const char* kSpecNames[kKinds] = {"556", "545", "762", "9mm"};
constexpr const char* kKindNames[kKinds] = {"5.56", "5.45", "7.62", "9 mm"};

// The eye at the sights: this near the port and behind it (or level with it), the effect's zoom turn.
constexpr float kEyeNearPort = 0.15f, kEyeBehindPort = 0.03f;
// The spin's axis: the gun's down (a casing turning about its extractor, its mouth swinging out to the right), up to this much off it either way.
constexpr float kSpinSpread = 0.7f;

std::atomic<float> g_speedScale{1.0f}, g_spinScale{1.0f};

}  // namespace

// ---------------------------------------------------------------- the throw

namespace casings {

const Emitter* EmitterFor(const char* shape) {
    if (!shape)
        return nullptr;
    for (const Emitter& e : kEmitters)
        if (std::strcmp(e.shape, shape) == 0)
            return &e;
    return nullptr;
}

bool ShotStart(const Throw& t, Start& out) {
    out = Start{};
    if (!t.gun || !Finite(t.gunRoom, 12) || !Finite(t.roomToArms, 12) || (t.haveEye && !Finite(t.eyeRoom, 3)))
        return false;
    const Emitter& e = *t.gun;
    if (e.kind < 0 || e.kind >= kKinds)
        return false;
    const loose::Spec* spec = loose::SpecFor(kSpecNames[e.kind]);
    if (!spec || spec->rings < 2 || spec->rings > 4)
        return false;
    const float* port = t.port ? t.port : e.port;
    if (!Finite(port, 3))
        return false;
    float gun[9], toArms[9];
    if (!SquareAxes(t.gunRoom, gun))
        return false;
    RotationOf(t.roomToArms, toArms);
    // The eye at the sights: the game turns its casings away from the face zoomed; with free hands the game never zooms, so the eye just behind
    // the port does it.
    float portRoom[3];
    Transform(t.gunRoom, port, portRoom);
    bool zoomed = false;
    if (t.haveEye) {
        const float d[3] = {t.eyeRoom[0] - portRoom[0], t.eyeRoom[1] - portRoom[1], t.eyeRoom[2] - portRoom[2]};
        const float onGun[3] = {gun[0] * d[0] + gun[3] * d[1] + gun[6] * d[2], gun[1] * d[0] + gun[4] * d[1] + gun[7] * d[2],
                                gun[2] * d[0] + gun[5] * d[1] + gun[8] * d[2]};
        zoomed = Length3(onGun) < kEyeNearPort && onGun[2] < kEyeBehindPort;
    }
    float turn[9];
    TurnOf(zoomed ? e.zoomTurn : e.turn, turn);
    Random random(t.seed);
    float way[3] = {Lerp(e.dirMin[0], e.dirMax[0], random.Next()), Lerp(e.dirMin[1], e.dirMax[1], random.Next()),
                    Lerp(e.dirMin[2], e.dirMax[2], random.Next())};
    if (!Normalize3(way))
        return false;
    float axis[3] = {(random.Next() - 0.5f) * kSpinSpread, -1.0f, (random.Next() - 0.5f) * kSpinSpread};
    Normalize3(axis);
    const float speed = Lerp(e.speedMin, e.speedMax, random.Next()) * g_speedScale.load(std::memory_order_relaxed);
    const float spin = Lerp(e.spinMin, e.spinMax, random.Next()) * g_spinScale.load(std::memory_order_relaxed);
    // The effect's axes on the gun, the gun's in the room, the room's in the arms' space.
    float onGun[3], room[3];
    Rotate3(turn, way, onGun);
    Rotate3(gun, onGun, room);
    Rotate3(toArms, room, out.velocity);
    for (float& v : out.velocity)
        v *= speed;
    Rotate3(turn, axis, onGun);
    Rotate3(gun, onGun, room);
    Rotate3(toArms, room, out.axis);
    Normalize3(out.axis);
    // In the chamber it lies along the bore, its mouth forward (the casing's +z the gun's +z): its middle at the port.
    const float middle = spec->ringAt[0] + 0.5f * (spec->ringAt[spec->rings - 1] - spec->ringAt[0]);
    const float onFrame[12] = {1.0f, 0.0f, 0.0f, port[0], 0.0f, 1.0f, 0.0f, port[1], 0.0f, 0.0f, 1.0f, port[2] - middle};
    float inRoom[12];
    Compose(t.gunRoom, onFrame, inRoom);
    Compose(t.roomToArms, inRoom, out.bone);
    out.spec = spec;
    out.kind = kLooseKind + e.kind;
    out.spin = spin;
    out.zoomed = zoomed;
    return Finite(out.bone, 12) && Finite(out.velocity, 3) && Finite(out.axis, 3) && std::isfinite(out.spin);
}

int RoundsFired(int before, int now, bool triggerHeldLately) {
    if (before < 0 || now < 0 || !triggerHeldLately)
        return 0;
    const int fired = before - now;
    return fired >= 1 && fired <= 3 ? fired : 0;
}

void PackInstance(const float* bone, const int* origin, const float* specFromMesh, float* rows) {
    Compose(bone, specFromMesh, rows);
    rows[3] += static_cast<float>(origin[0]);
    rows[7] += static_cast<float>(origin[1]);
    rows[11] += static_cast<float>(origin[2]);
}

int PackAll(const loose::Pose* poses, int poseCount, const int* origin, const float (*specFromMesh)[12], const bool* fitKnown, float* rows, int most,
            int* first, int* count) {
    for (int k = 0; k < kKinds; ++k)
        first[k] = count[k] = 0;
    if (!poses || !origin || !specFromMesh || !fitKnown || !rows || most <= 0)
        return 0;
    for (int p = 0; p < poseCount; ++p) {
        const int k = poses[p].kind - kLooseKind;
        if (IsLooseCasing(poses[p].kind) && fitKnown[k] && Finite(poses[p].bone, 12))
            ++count[k];
    }
    int at = 0;
    for (int k = 0; k < kKinds; ++k) {
        first[k] = at;
        count[k] = std::min(count[k], most - at);
        at += count[k];
    }
    int placed[kKinds] = {};
    for (int p = 0; p < poseCount; ++p) {
        const int k = poses[p].kind - kLooseKind;
        if (!IsLooseCasing(poses[p].kind) || !fitKnown[k] || !Finite(poses[p].bone, 12) || placed[k] >= count[k])
            continue;
        PackInstance(poses[p].bone, origin, specFromMesh[k], rows + static_cast<size_t>(first[k] + placed[k]) * 12);
        ++placed[k];
    }
    return at;
}

bool FitMesh(const float* places, int count, float scale, MeshFit& out) {
    out = MeshFit{};
    if (!places || count < 6 || !(scale > 0.0f) || !std::isfinite(scale))
        return false;
    float lo[3] = {1e30f, 1e30f, 1e30f}, hi[3] = {-1e30f, -1e30f, -1e30f};
    for (int v = 0; v < count; ++v) {
        const float* p = places + v * 3;
        if (!Finite(p, 3))
            return false;
        for (int i = 0; i < 3; ++i)
            lo[i] = std::min(lo[i], p[i]), hi[i] = std::max(hi[i], p[i]);
    }
    const float extent[3] = {hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2]};
    int a = 0;
    for (int i = 1; i < 3; ++i)
        if (extent[i] > extent[a])
            a = i;
    const int u = (a + 1) % 3, w = (a + 2) % 3;
    // A casing's shape: 1.5 to 10 times as long as it's wide, round across (its two widths alike), 8 mm to 15 cm long as drawn. A wrong guess at
    // where the places are in a vertex gives nothing like it.
    const float length = extent[a] * scale, wide = std::max(extent[u], extent[w]), narrow = std::min(extent[u], extent[w]);
    if (!(length >= 0.008f && length <= 0.15f) || !(narrow > 0.0f) || wide / narrow > 1.6f || extent[a] / wide < 1.5f || extent[a] / wide > 10.0f)
        return false;
    const float cu = 0.5f * (lo[u] + hi[u]), cw = 0.5f * (lo[w] + hi[w]);
    // Its base the wider end (the rim: 4.8 mm a 5.56's, its neck 3.2): the widest within an eighth of its length of each end (the 5.56's shoulder,
    // 4.5 mm, is 8.7 mm from its mouth: a fifth would reach it).
    float rLow = 0.0f, rHigh = 0.0f;
    for (int v = 0; v < count; ++v) {
        const float* p = places + v * 3;
        const float r = std::sqrt((p[u] - cu) * (p[u] - cu) + (p[w] - cw) * (p[w] - cw));
        if (p[a] <= lo[a] + 0.12f * extent[a])
            rLow = std::max(rLow, r);
        if (p[a] >= hi[a] - 0.12f * extent[a])
            rHigh = std::max(rHigh, r);
    }
    out.ambiguous = std::fabs(rHigh - rLow) < 0.04f * std::max(rHigh, rLow);
    out.baseAtMax = !out.ambiguous && rHigh > rLow;
    out.axis = a;
    out.scale = scale;
    out.length = length;
    out.baseRadius = (out.baseAtMax ? rHigh : rLow) * scale;
    out.mouthRadius = (out.baseAtMax ? rLow : rHigh) * scale;
    // The casing's axes in the mesh's: its z from the base to the mouth along that axis, its x along the next.
    float z[3] = {}, x[3] = {}, y[3];
    z[a] = out.baseAtMax ? -1.0f : 1.0f;
    x[u] = 1.0f;
    Cross3(z, x, y);
    float base[3];
    base[a] = out.baseAtMax ? hi[a] : lo[a];
    base[u] = cu, base[w] = cw;
    const float* row[3] = {x, y, z};
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c)
            out.specFromMesh[r * 4 + c] = scale * row[r][c];
        out.specFromMesh[r * 4 + 3] = -scale * (row[r][0] * base[0] + row[r][1] * base[1] + row[r][2] * base[2]);
    }
    return true;
}

float HalfToFloat(uint16_t half) {
    const uint32_t exponent = (half >> 10) & 0x1Fu, mantissa = half & 0x3FFu;
    float value;
    if (exponent == 0)
        value = std::ldexp(static_cast<float>(mantissa), -24);
    else if (exponent == 31)
        value = mantissa ? std::nanf("") : HUGE_VALF;
    else
        value = std::ldexp(static_cast<float>(mantissa | 0x400u), static_cast<int>(exponent) - 25);
    return (half & 0x8000u) ? -value : value;
}

void SetThrow(float speedScale, float spinScale) {
    speedScale = std::isfinite(speedScale) ? std::clamp(speedScale, 0.1f, 4.0f) : 1.0f;
    spinScale = std::isfinite(spinScale) ? std::clamp(spinScale, 0.0f, 20.0f) : 1.0f;
    const float wasSpeed = g_speedScale.exchange(speedScale), wasSpin = g_spinScale.exchange(spinScale);
    if (wasSpeed != speedScale || wasSpin != spinScale)
        Log("Casings: thrown at %.2f times the game's speed, spinning %.2f times its spin", speedScale, spinScale);
}

}  // namespace casings

// ---------------------------------------------------------------- in the game

namespace {

using casings::DrawIndexedInstancedFn;
using casings::MeshFit;

constexpr UINT kFirstPersonStencil = 32;   // the first-person pass's draws' stencil reference has this (skin.cpp's kFirstPersonStencil; the census)
constexpr UINT kInstanceStride = 48;       // a casing's placement in stream 1 (effects.cpp:305)
constexpr UINT kViewBytes = 352;           // the per-view constants (viewturn.h's kPerViewBytes): each pass's own
constexpr UINT kEyeAt = 320;               // ... the eye in them (bytes)
constexpr UINT kPerDrawBytes = 4096;       // the game's per-draw block (skin.cpp's kGloveUploadSize): its own for each draw
constexpr int kCbSlots = D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
constexpr int kVsSrvs = 8, kVsSamplers = 4;
constexpr int kPsSrvs = 16, kPsSamplers = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;
constexpr int kVbSlots = 4;
constexpr int kTargets = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
constexpr int kMostInstances = loose::kCasings;
constexpr ULONGLONG kRefreshMs = 10000;    // a kept draw kept again this often while the game draws casings
constexpr ULONGLONG kKeepRetryMs = 1000;   // ... and after one that couldn't be kept
constexpr uint32_t kMostLooks = 256;       // ... and after so many of its draws that weren't in an eye's picture (a look each, about 1 us)
constexpr ULONGLONG kHideMs = 1700;        // the game's own hidden this long after the trigger or a shot (its 1.5 s ones too)
constexpr float kHideReach = 3.0f;         // m from the game's ejection port (its effect's CullDistance)
constexpr UINT kHideMost = 64;             // instances in one of its casing draws (effects.cpp's kCasingsMost)
constexpr ULONGLONG kTriggerMs = 300;      // a shot counts only with the trigger held this lately
constexpr ULONGLONG kGunFreshMs = 250;     // TakeShots' gun counts this long
constexpr int kArms = 4;                   // contexts armed at once
constexpr uint32_t kArmFrames = 2;         // an arming not hosted in this many frames goes
constexpr uint32_t kArmDraws = 256;        // ... nor in this many draws with its targets on its depth (a full look each, about 1 us)
constexpr uint32_t kArmWaits = 64;         // ... nor after this many of those that hadn't the view bound or were of a view just drawn into
constexpr double kRepeatSeconds = 0.008;   // the same eye's view hosted again this soon: the same pass (a second clear in it)
constexpr float kRepeatMetres = 0.002f;
constexpr float kFirstPersonReach = 5.0f;  // a view whose eye is this near the origin is the arms' space's, not the world's
constexpr int kLearnVertices = 256;        // the mesh's vertices read back, at most
constexpr UINT kLearnStamp = 16;           // bytes of the readback's own stamp before them
constexpr uint32_t kLearnWait = 3, kLearnStall = 120;  // frames
constexpr int kLearnTries = 6;
constexpr ULONGLONG kWatchMs = 2000, kOffMs = 20000;  // casings in view this long and never hosted: the mod's off this long
constexpr uintptr_t kWorldAt = 0x023813A0;  // the client's world, one for each level (ground.cpp:36)
constexpr float kIdentity[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};

std::atomic<int> g_mode{0};
std::atomic<UINT> g_counts[kKinds] = {234, 0, 0, 114};  // the game's casing draw of each kind: its index count (effects.cpp:302-304)
std::atomic<bool> g_countChanged[kKinds] = {};
std::atomic<bool> g_drawable[kKinds] = {};
std::atomic<ULONGLONG> g_keptAt[kKinds] = {}, g_depthKeptAt[kKinds] = {};
std::atomic<bool> g_wantDepth[kKinds] = {};
std::atomic<uint32_t> g_looks[kKinds] = {};
std::atomic<int> g_gunKind{-1};
std::atomic<ULONGLONG> g_gunAt{0}, g_triggerAt{0}, g_shotAt{0};
std::atomic<int> g_shotsThisFrame{0};
std::atomic<bool> g_wantArm{false};
std::atomic<bool> g_releaseWanted{false};
std::atomic<uint32_t> g_frame{0};
std::atomic<int> g_mostPasses{6};
std::atomic<bool> g_logOn{true};
std::atomic<ULONGLONG> g_offUntil{0};

// What became of things, for the once-a-second line.
enum Why {
    kWhyNothing,       // armed, nothing to draw by then
    kWhyNotDrawable,   // no kind of those in view drawable
    kWhyPass,          // the pass's targets or depth weren't the kept draw's
    kWhyView,          // the pass had no view constants where the kept draw had its
    kWhyRepeat,        // the same eye's view drawn into just now
    kWhyMap,           // the placements couldn't be uploaded
    kWhyPasses,        // more passes in a frame than casingspasses
    kWhyLong,          // armed, and its context drew on and on with no pass of an eye's
    kWhyOld,           // armed, and nothing came in two frames
    kWhys
};
constexpr const char* kWhyNames[kWhys] = {"nothing to draw", "no kind drawable", "another kind of pass", "no view constants", "that eye again",
                                          "no upload", "too many passes", "no pass in its draws", "no pass in two frames"};
std::atomic<uint32_t> g_why[kWhys] = {};
std::atomic<uint32_t> g_hostedPasses{0}, g_hostedDraws{0}, g_passesThisFrame{0}, g_hidden{0}, g_hideNot{0}, g_thrown{0};
std::atomic<uint32_t> g_viewSame{0}, g_viewOther{0};
std::atomic<int64_t> g_hostTicks{0}, g_hostWorst{0};

int KindOfCount(UINT count) {
    for (int k = 0; k < kKinds; ++k)
        if (UINT c = g_counts[k].load(std::memory_order_relaxed); c && c == count)
            return k;
    return -1;
}

template <typename T>
void SafeRelease(T*& p) {
    if (p) {
        p->Release();
        p = nullptr;
    }
}

int64_t Ticks() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart;
}
double TicksPerSecond() {
    static const double frequency = [] {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        return static_cast<double>(f.QuadPart);
    }();
    return frequency;
}

// ---------------------------------------------------------------- what's kept of a draw
// How a constant buffer of the kept draw is bound again: as nothing (none was), the pass's own (the view's), the pass's own if it has the same one
// bound there, or the copy.
enum CbRule : uint8_t { kCbNone, kCbView, kCbShared, kCbCopy };
constexpr const char* kCbRuleNames[] = {"none", "view", "shared", "copy"};
struct Cb {
    ID3D11Buffer* original = nullptr;  // the game's, as bound (referenced: compared with the pass's own, and its place never taken by another)
    ID3D11Buffer* copy = nullptr;      // the mod's, copied from it at the draw (none for an immutable one: the original is bound)
    UINT size = 0;
    D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
    CbRule rule = kCbNone;
};

struct KeptState {
    bool have = false;
    ID3D11InputLayout* layout = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ID3D11Buffer* ib = nullptr;
    DXGI_FORMAT ibFormat = DXGI_FORMAT_UNKNOWN;
    UINT ibOffset = 0;
    ID3D11Buffer* vb[kVbSlots] = {};  // slot 1, the game's placements, isn't kept: the mod's go there
    UINT stride[kVbSlots] = {}, offset[kVbSlots] = {};
    UINT count = 0, start = 0;
    INT base = 0;
    ID3D11VertexShader* vs = nullptr;
    Cb vsCb[kCbSlots];
    ID3D11ShaderResourceView* vsSrv[kVsSrvs] = {};
    ID3D11SamplerState* vsSampler[kVsSamplers] = {};
    ID3D11PixelShader* ps = nullptr;
    Cb psCb[kCbSlots];
    ID3D11ShaderResourceView* psSrv[kPsSrvs] = {};
    ID3D11SamplerState* psSampler[kPsSamplers] = {};
    ID3D11RasterizerState* raster = nullptr;
    ID3D11BlendState* blend = nullptr;
    FLOAT blendFactor[4] = {};
    UINT sampleMask = 0xFFFFFFFFu;
    ID3D11DepthStencilState* depth = nullptr;
    UINT stencilRef = 0;
    bool depthEqual = false;  // tests EQUAL, writes nothing: a depth pass drew it first
    UINT psTextures = 0;

    void Release() {
        SafeRelease(layout);
        SafeRelease(ib);
        for (ID3D11Buffer*& b : vb)
            SafeRelease(b);
        SafeRelease(vs);
        SafeRelease(ps);
        for (Cb* cbs : {vsCb, psCb})
            for (int i = 0; i < kCbSlots; ++i) {
                SafeRelease(cbs[i].original);
                SafeRelease(cbs[i].copy);
                cbs[i] = Cb{};
            }
        for (ID3D11ShaderResourceView*& v : vsSrv)
            SafeRelease(v);
        for (ID3D11ShaderResourceView*& v : psSrv)
            SafeRelease(v);
        for (ID3D11SamplerState*& s : vsSampler)
            SafeRelease(s);
        for (ID3D11SamplerState*& s : psSampler)
            SafeRelease(s);
        SafeRelease(raster);
        SafeRelease(blend);
        SafeRelease(depth);
        *this = KeptState{};
    }
};

// The kind of pass a draw is in: its targets and its depth.
struct Pass {
    UINT targets = 0;  // bound from slot 0 on
    DXGI_FORMAT format[kTargets] = {};
    UINT width = 0, height = 0, samples = 0;  // the depth target's
    DXGI_FORMAT depthFormat = DXGI_FORMAT_UNKNOWN;
    bool haveDepth = false;
    ID3D11Resource* depthTexture = nullptr;  // not referenced: compared only
    UINT targetWidth = 0, targetHeight = 0;  // the first target's
    bool SameAs(const Pass& o) const {
        if (targets != o.targets || !haveDepth || !o.haveDepth || width != o.width || height != o.height || samples != o.samples ||
            depthFormat != o.depthFormat)
            return false;
        for (UINT i = 0; i < targets; ++i)
            if (format[i] != o.format[i])
                return false;
        return true;
    }
};

D3D11_TEXTURE2D_DESC TextureDesc(ID3D11Resource* resource, bool& ok) {
    D3D11_TEXTURE2D_DESC desc{};
    ok = false;
    if (!resource)
        return desc;
    D3D11_RESOURCE_DIMENSION dimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    resource->GetType(&dimension);
    if (dimension != D3D11_RESOURCE_DIMENSION_TEXTURE2D)
        return desc;
    static_cast<ID3D11Texture2D*>(resource)->GetDesc(&desc);
    ok = true;
    return desc;
}

void LookAtPass(ID3D11DeviceContext* ctx, Pass& out) {
    out = Pass{};
    ID3D11RenderTargetView* rtvs[kTargets] = {};
    ID3D11DepthStencilView* dsv = nullptr;
    ctx->OMGetRenderTargets(kTargets, rtvs, &dsv);
    bool gap = false;
    for (int i = 0; i < kTargets; ++i) {
        if (!rtvs[i]) {
            gap = true;
            continue;
        }
        if (!gap) {
            D3D11_RENDER_TARGET_VIEW_DESC d{};
            rtvs[i]->GetDesc(&d);
            out.format[out.targets++] = d.Format;
            if (i == 0) {
                ID3D11Resource* resource = nullptr;
                rtvs[i]->GetResource(&resource);
                bool ok = false;
                const D3D11_TEXTURE2D_DESC t = TextureDesc(resource, ok);
                if (ok)
                    out.targetWidth = t.Width, out.targetHeight = t.Height;
                SafeRelease(resource);
            }
        }
        rtvs[i]->Release();
    }
    if (dsv) {
        ID3D11Resource* resource = nullptr;
        dsv->GetResource(&resource);
        bool ok = false;
        const D3D11_TEXTURE2D_DESC t = TextureDesc(resource, ok);
        if (ok) {
            out.haveDepth = true;
            out.width = t.Width, out.height = t.Height, out.samples = t.SampleDesc.Count, out.depthFormat = t.Format;
            out.depthTexture = resource;
        }
        SafeRelease(resource);
        dsv->Release();
    }
}

// Whether the draw now writes any colour to its targets (a depth pass's draw with the targets left bound writes none).
bool WritesColour(ID3D11DeviceContext* ctx, UINT targets) {
    ID3D11BlendState* blend = nullptr;
    FLOAT factor[4] = {};
    UINT mask = 0;
    ctx->OMGetBlendState(&blend, factor, &mask);
    if (!blend)
        return true;  // the default state writes everything
    D3D11_BLEND_DESC d{};
    blend->GetDesc(&d);
    blend->Release();
    bool writes = false;
    for (UINT i = 0; i < std::min<UINT>(std::max<UINT>(targets, 1u), kTargets); ++i)
        writes = writes || (d.IndependentBlendEnable ? d.RenderTarget[i] : d.RenderTarget[0]).RenderTargetWriteMask != 0;
    return writes;
}

// Why a draw couldn't be kept, for the log.
enum KeepWhy { kKeptFine, kKeepNoStream, kKeepDynamicMesh, kKeepStages, kKeepShaders, kKeepDepthUnwritten, kKeepCopy, kKeepNoView, kKeepWhys };
constexpr const char* kKeepWhyNames[kKeepWhys] = {"kept", "its placements not in a 48-byte stream 1", "its mesh in a buffer rewritten each frame",
                                                  "a hull, domain or geometry shader with it", "no vertex or pixel shader",
                                                  "its depth neither tested nor written", "its constants couldn't be copied",
                                                  "no view constants among its vertex shader's (352 bytes): kept, it would show where it was kept from"};

// Its constant buffers: each referenced, and but the view's, copied as they are now (into the last kept copy of that size, if there is one: shared,
// so a keep that fails halfway leaves the last one whole).
bool KeepCbs(ID3D11DeviceContext* ctx, ID3D11Device* device, ID3D11Buffer* const* bound, Cb* out, const Cb* old) {
    for (int i = 0; i < kCbSlots; ++i) {
        ID3D11Buffer* b = bound[i];
        if (!b)
            continue;
        D3D11_BUFFER_DESC d{};
        b->GetDesc(&d);
        Cb& c = out[i];
        c.original = b;
        b->AddRef();
        c.size = d.ByteWidth;
        c.usage = d.Usage;
        c.rule = d.ByteWidth == kViewBytes || skin::ViewBufferKind(b) != 0 ? kCbView : d.ByteWidth >= kPerDrawBytes ? kCbCopy : kCbShared;
        if (c.rule == kCbView || d.Usage == D3D11_USAGE_IMMUTABLE)
            continue;
        if (old && old[i].copy && old[i].size == d.ByteWidth) {
            c.copy = old[i].copy;
            c.copy->AddRef();
        } else {
            D3D11_BUFFER_DESC copy{d.ByteWidth, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0};
            if (FAILED(device->CreateBuffer(&copy, nullptr, &c.copy)))
                return false;
        }
        ctx->CopyResource(c.copy, b);
    }
    return true;
}

// Everything the draw now on ctx is drawn with but its targets, viewports and placements. old: what was kept of it before (its copies are reused).
KeepWhy Capture(ID3D11DeviceContext* ctx, ID3D11Device* device, UINT count, UINT start, INT base, bool colour, KeptState& out, const KeptState& old) {
    out = KeptState{};
    ID3D11Buffer* vbs[kVbSlots] = {};
    UINT strides[kVbSlots] = {}, offsets[kVbSlots] = {};
    ctx->IAGetVertexBuffers(0, kVbSlots, vbs, strides, offsets);
    KeepWhy why = kKeptFine;
    D3D11_BUFFER_DESC d{};
    if (!vbs[0] || !vbs[1] || strides[1] != kInstanceStride)
        why = kKeepNoStream;
    for (int i = 0; i < kVbSlots && why == kKeptFine; ++i)
        if (i != 1 && vbs[i]) {
            vbs[i]->GetDesc(&d);
            if (d.Usage == D3D11_USAGE_DYNAMIC)
                why = kKeepDynamicMesh;
        }
    for (int i = 0; i < kVbSlots; ++i) {
        if (i != 1 && why == kKeptFine) {
            out.vb[i] = vbs[i];  // (Get's reference kept)
            out.stride[i] = strides[i], out.offset[i] = offsets[i];
        } else {
            SafeRelease(vbs[i]);
        }
    }
    if (why != kKeptFine)
        return why;
    ctx->IAGetIndexBuffer(&out.ib, &out.ibFormat, &out.ibOffset);
    if (!out.ib)
        return kKeepNoStream;
    out.ib->GetDesc(&d);
    if (d.Usage == D3D11_USAGE_DYNAMIC)
        return kKeepDynamicMesh;
    ctx->IAGetInputLayout(&out.layout);
    ctx->IAGetPrimitiveTopology(&out.topology);
    {
        ID3D11HullShader* hs = nullptr;
        ID3D11DomainShader* ds = nullptr;
        ID3D11GeometryShader* gs = nullptr;
        ctx->HSGetShader(&hs, nullptr, nullptr);
        ctx->DSGetShader(&ds, nullptr, nullptr);
        ctx->GSGetShader(&gs, nullptr, nullptr);
        const bool stages = hs || ds || gs;
        SafeRelease(hs);
        SafeRelease(ds);
        SafeRelease(gs);
        if (stages)
            return kKeepStages;
    }
    ctx->VSGetShader(&out.vs, nullptr, nullptr);
    ctx->PSGetShader(&out.ps, nullptr, nullptr);
    if (!out.layout || !out.vs || (colour && !out.ps))
        return kKeepShaders;
    ID3D11Buffer* bound[kCbSlots] = {};
    ctx->VSGetConstantBuffers(0, kCbSlots, bound);
    bool copied = KeepCbs(ctx, device, bound, out.vsCb, old.vsCb);
    for (ID3D11Buffer*& b : bound)
        SafeRelease(b);
    ctx->PSGetConstantBuffers(0, kCbSlots, bound);
    copied = KeepCbs(ctx, device, bound, out.psCb, old.psCb) && copied;
    for (ID3D11Buffer*& b : bound)
        SafeRelease(b);
    if (!copied)
        return kKeepCopy;
    // Each pass's own view goes where the kept draw had its: with none found, a copy of the one it was kept with would be drawn every pass.
    if (std::none_of(std::begin(out.vsCb), std::end(out.vsCb), [](const Cb& c) { return c.rule == kCbView; }))
        return kKeepNoView;
    ctx->VSGetShaderResources(0, kVsSrvs, out.vsSrv);
    ctx->VSGetSamplers(0, kVsSamplers, out.vsSampler);
    ctx->PSGetShaderResources(0, kPsSrvs, out.psSrv);
    ctx->PSGetSamplers(0, kPsSamplers, out.psSampler);
    for (ID3D11ShaderResourceView* v : out.psSrv)
        out.psTextures += v != nullptr;
    ctx->RSGetState(&out.raster);
    ctx->OMGetBlendState(&out.blend, out.blendFactor, &out.sampleMask);
    ctx->OMGetDepthStencilState(&out.depth, &out.stencilRef);
    D3D11_DEPTH_STENCIL_DESC depth{};
    if (out.depth)
        out.depth->GetDesc(&depth);
    else
        depth.DepthEnable = TRUE, depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL, depth.DepthFunc = D3D11_COMPARISON_LESS;  // (the default state)
    const bool writes = depth.DepthEnable && depth.DepthWriteMask == D3D11_DEPTH_WRITE_MASK_ALL;
    out.depthEqual = depth.DepthEnable && depth.DepthWriteMask == D3D11_DEPTH_WRITE_MASK_ZERO && depth.DepthFunc == D3D11_COMPARISON_EQUAL;
    // Drawn at the start of the pass, it must write its depth, or what's drawn after it there covers it (EQUAL: its depth pass's draw first).
    if (!writes && !(colour && out.depthEqual))
        return kKeepDepthUnwritten;
    out.count = count, out.start = start, out.base = base;
    out.have = true;
    return kKeptFine;
}

// The mesh's vertices read back once (the instance, its indices, its vertices), to fit the casing's shape to it: copied into gpu on the draw's own
// context after a stamp of its own, then on the immediate one into cpu and mapped when it's there. Another stamp there: the copy hadn't run yet.
enum LearnStep { kLearnNone, kLearnCopied, kLearnStaged, kLearnDone, kLearnFailed };
struct Learn {
    int step = kLearnNone;
    uint32_t frame = 0;
    int tries = 0;
    ID3D11Buffer* gpu = nullptr;
    ID3D11Buffer* cpu = nullptr;
    UINT size = 0;
    uint32_t stamp = 0;  // this readback's own, at its start
    UINT indexSize = 2, count = 0, stride = 0, vertices = 0, verticesAt = 0;
    bool said = false;
    char note[200] = "";  // how it was read, or why not
};

struct KindState {
    KeptState colour, depth;
    Pass pass;
    bool fitKnown = false;
    MeshFit fit;
    float specFromMesh[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    Learn learn;
    // For the log (Update): kept anew, read, refused (and the refused draw's vertex shader's constant buffers, slot:bytes).
    bool keptNews = false, fitNews = false;
    KeepWhy refused = kKeptFine;
    bool refusedNews = false;
    char refusedBuffers[160] = "";
};

// A kept draw's constant buffers as the log has them: " 0:4096 copy 2:352 view" (rules too).
void DescribeCbs(const Cb* cbs, char* text, size_t size, bool rules) {
    int used = 0;
    text[0] = 0;
    for (int i = 0; i < kCbSlots && used >= 0 && static_cast<size_t>(used) + 24 < size; ++i)
        if (cbs[i].original)
            used += std::snprintf(text + used, size - used, rules ? " %d:%u %s" : " %d:%u", i, cbs[i].size, kCbRuleNames[cbs[i].rule]);
}

std::mutex g_mutex;  // all below; the arms' table has its own (taken inside this one, never the other way round)
KindState g_kinds[kKinds];
// The placements to draw, each kind's together, as SetLoose last made them.
float g_rows[kMostInstances * 12] = {};
int g_first[kKinds] = {}, g_count[kKinds] = {};
int g_instanceCount = 0;
int g_flying = 0, g_lying = 0, g_inView = 0;
// Each context's own buffer of placements (a buffer can be drawn from on any context; one each keeps their uploads apart).
struct ContextBuffer {
    ID3D11DeviceContext* ctx = nullptr;
    ID3D11Buffer* buffer = nullptr;
};
ContextBuffer g_contextBuffers[8];
ID3D11BlendState* g_noColour = nullptr;  // the depth draw's: nothing written to the targets
struct Hosted {
    float eye[3] = {};
    double at = -1.0;
};
Hosted g_recent[4];
int g_recentNext = 0;

// The arms. ctx is read without the lock by the draw hooks' look (ArmedSlow).
struct Arm {
    std::atomic<ID3D11DeviceContext*> ctx{nullptr};
    ID3D11Resource* texture = nullptr;  // not referenced: compared only
    uint32_t frame = 0, draws = 0;
    uint32_t waits = 0;                 // draws on its depth with its targets that weren't the pass's start (first person, no view): each a look
};
std::mutex g_armMutex;
Arm g_arms[kArms];
// The eye's depth target as the kept draw had it: what an arming clear has to be.
struct ArmDepth {
    bool have = false;
    UINT width = 0, height = 0, samples = 0;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
} g_armDepth;  // under g_armMutex

// An arming for whichever context draws into that depth first: the clear on one context, the pass's draws recorded on another (BF3 records on several
// threads). weapon.txt "casingsanycontext 1", or by itself once armed contexts never drew their passes.
ID3D11DeviceContext* const kAnyContext = reinterpret_cast<ID3D11DeviceContext*>(static_cast<uintptr_t>(1));
std::atomic<bool> g_anyContext{false};

void DisarmLocked(Arm& arm) {
    if (arm.ctx.exchange(nullptr))
        casings::g_armed.fetch_sub(1, std::memory_order_relaxed);
    arm.texture = nullptr;
    arm.draws = arm.waits = 0;
}

bool ArmsContext(const Arm& arm, ID3D11DeviceContext* ctx) {
    ID3D11DeviceContext* armed = arm.ctx.load(std::memory_order_relaxed);
    return armed && (armed == ctx || armed == kAnyContext);
}

// The arming a draw on ctx into that depth target goes with, or none.
Arm* ArmForLocked(ID3D11DeviceContext* ctx, ID3D11Resource* texture) {
    for (Arm& a : g_arms)
        if (ArmsContext(a, ctx) && a.texture == texture)
            return &a;
    return nullptr;
}

// A draw of the armed pass's kind that isn't where the casings go (still armed): only so many, each costing a look.
void Wait(ID3D11DeviceContext* ctx, ID3D11Resource* texture, Why why) {
    std::lock_guard lock(g_armMutex);
    if (Arm* arm = ArmForLocked(ctx, texture); arm && ++arm->waits > kArmWaits) {
        DisarmLocked(*arm);
        g_why[why].fetch_add(1, std::memory_order_relaxed);
    }
}

// Taken by this draw: false if another has taken it meanwhile.
bool Claim(ID3D11DeviceContext* ctx, ID3D11Resource* texture) {
    std::lock_guard lock(g_armMutex);
    Arm* arm = ArmForLocked(ctx, texture);
    if (!arm)
        return false;
    DisarmLocked(*arm);
    return true;
}

void DisarmAll() {
    std::lock_guard lock(g_armMutex);
    for (Arm& arm : g_arms)
        DisarmLocked(arm);
}

void ReleaseKind(KindState& s) {
    s.colour.Release();
    s.depth.Release();
    s.pass = Pass{};
    s.fitKnown = false;
    s.fit = MeshFit{};
    std::memcpy(s.specFromMesh, kIdentity, sizeof(kIdentity));
    // (Its readback buffers are the device's, kept for the next world.)
    s.learn.step = kLearnNone;
    s.learn.tries = 0;
    s.learn.said = false;
    s.learn.note[0] = 0;
    s.keptNews = s.fitNews = s.refusedNews = false;
    s.refused = kKeptFine;
}

void ClearLocked() {
    for (int k = 0; k < kKinds; ++k) {
        ReleaseKind(g_kinds[k]);
        g_keptAt[k] = 0;
        g_depthKeptAt[k] = 0;
        g_wantDepth[k] = false;
        g_drawable[k] = false;
    }
    g_instanceCount = 0;
    for (int k = 0; k < kKinds; ++k)
        g_first[k] = g_count[k] = 0;
    g_flying = g_lying = g_inView = 0;
    for (Hosted& h : g_recent)
        h = Hosted{};
    {
        std::lock_guard arms(g_armMutex);
        g_armDepth = ArmDepth{};
    }
}

// The readback of the kept draw's mesh, on its own context, now. Under g_mutex.
void StartLearn(ID3D11DeviceContext* ctx, ID3D11Device* device, KindState& s, UINT first) {
    Learn& l = s.learn;
    const KeptState& k = s.colour;
    if (!k.vb[0] || !k.ib || !k.stride[0] || k.base < 0)
        return;
    D3D11_BUFFER_DESC vbDesc{}, ibDesc{};
    k.vb[0]->GetDesc(&vbDesc);
    k.ib->GetDesc(&ibDesc);
    const UINT indexSize = k.ibFormat == DXGI_FORMAT_R32_UINT ? 4u : 2u;
    const UINT stride = k.stride[0];
    const UINT64 vertexFrom = static_cast<UINT64>(k.offset[0]) + static_cast<UINT64>(k.base) * stride;
    const UINT64 indexFrom = static_cast<UINT64>(k.ibOffset) + static_cast<UINT64>(k.start) * indexSize;
    if (vertexFrom >= vbDesc.ByteWidth || indexFrom + static_cast<UINT64>(k.count) * indexSize > ibDesc.ByteWidth || k.count > 4096)
        return;
    const UINT vertices = static_cast<UINT>(std::min<UINT64>(kLearnVertices, (vbDesc.ByteWidth - vertexFrom) / stride));
    ID3D11Buffer* instances = nullptr;
    UINT instanceStride = 0, instanceOffset = 0;
    ctx->IAGetVertexBuffers(1, 1, &instances, &instanceStride, &instanceOffset);
    if (!instances || !vertices) {
        SafeRelease(instances);
        return;
    }
    D3D11_BUFFER_DESC instanceDesc{};
    instances->GetDesc(&instanceDesc);
    const UINT64 instanceFrom = static_cast<UINT64>(instanceOffset) + static_cast<UINT64>(first) * kInstanceStride;
    const UINT indexBytes = (k.count * indexSize + 15u) & ~15u;
    const UINT size = kLearnStamp + kInstanceStride + indexBytes + vertices * stride;
    if (instanceFrom + kInstanceStride > instanceDesc.ByteWidth) {
        instances->Release();
        return;
    }
    if (!l.gpu || l.size < size) {
        SafeRelease(l.gpu);
        SafeRelease(l.cpu);
        D3D11_BUFFER_DESC gpu{size, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0};
        D3D11_BUFFER_DESC cpu{size, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ, 0, 0};
        if (FAILED(device->CreateBuffer(&gpu, nullptr, &l.gpu)) || FAILED(device->CreateBuffer(&cpu, nullptr, &l.cpu))) {
            SafeRelease(l.gpu);
            SafeRelease(l.cpu);
            instances->Release();
            ++l.tries;
            std::snprintf(l.note, sizeof(l.note), "no buffers to read it into");
            return;
        }
        l.size = size;
    }
    // Stamped first (the whole buffer: a part of one on a deferred context isn't to be trusted), then copied into: read back with another stamp, this
    // copy hadn't run yet.
    l.stamp = l.stamp * 2654435761u + 0x6A09E667u;
    std::vector<uint8_t> stamped(l.size, 0xFF);
    const uint32_t stamp[4] = {l.stamp, ~l.stamp, 0x43415349u, 0x4E475321u};
    std::memcpy(stamped.data(), stamp, sizeof(stamp));
    ctx->UpdateSubresource(l.gpu, 0, nullptr, stamped.data(), 0, 0);
    const D3D11_BOX instanceBox{static_cast<UINT>(instanceFrom), 0, 0, static_cast<UINT>(instanceFrom) + kInstanceStride, 1, 1};
    const D3D11_BOX indexBox{static_cast<UINT>(indexFrom), 0, 0, static_cast<UINT>(indexFrom) + k.count * indexSize, 1, 1};
    const D3D11_BOX vertexBox{static_cast<UINT>(vertexFrom), 0, 0, static_cast<UINT>(vertexFrom) + vertices * stride, 1, 1};
    ctx->CopySubresourceRegion(l.gpu, 0, kLearnStamp, 0, 0, instances, 0, &instanceBox);
    ctx->CopySubresourceRegion(l.gpu, 0, kLearnStamp + kInstanceStride, 0, 0, k.ib, 0, &indexBox);
    ctx->CopySubresourceRegion(l.gpu, 0, kLearnStamp + kInstanceStride + indexBytes, 0, 0, k.vb[0], 0, &vertexBox);
    instances->Release();
    l.indexSize = indexSize, l.count = k.count, l.stride = stride, l.vertices = vertices, l.verticesAt = kLearnStamp + kInstanceStride + indexBytes;
    l.step = kLearnCopied;
    l.frame = g_frame.load(std::memory_order_relaxed);
}

// The readback there: the game's instance's scale, the vertices its indices use, their places read as three floats or four halves at the vertex's
// start, whichever gives a casing's shape. Under g_mutex.
void ReadLearn(KindState& s, const uint8_t* data) {
    Learn& l = s.learn;
    uint32_t stamp[2];
    std::memcpy(stamp, data, sizeof(stamp));
    float instance[12];
    std::memcpy(instance, data + kLearnStamp, sizeof(instance));
    bool unread = stamp[0] != l.stamp || stamp[1] != ~l.stamp;
    for (int i = 0; i < 12 && !unread; ++i)
        unread = std::isnan(instance[i]);
    if (unread) {  // the copy hadn't run yet: again with the next keep
        l.step = kLearnNone;
        ++l.tries;
        std::snprintf(l.note, sizeof(l.note), "its copy hadn't run when read");
        return;
    }
    float scale = 0.0f;
    for (int r = 0; r < 3; ++r)
        scale += Length3(instance + r * 4) / 3.0f;
    const bool scaleRead = scale > 0.05f && scale < 20.0f;
    if (!scaleRead)
        scale = 1.0f;
    std::vector<uint8_t> used(l.vertices, 0);
    UINT most = 0;
    for (UINT i = 0; i < l.count; ++i) {
        UINT index = 0;
        std::memcpy(&index, data + kLearnStamp + kInstanceStride + i * l.indexSize, l.indexSize);
        most = std::max(most, index);
        if (index < l.vertices)
            used[index] = 1;
    }
    if (most >= l.vertices) {
        l.step = kLearnFailed;
        std::snprintf(l.note, sizeof(l.note), "its indices go to vertex %u, past the %u read", most, l.vertices);
        return;
    }
    std::vector<float> places;
    const char* how = nullptr;
    for (int way = 0; way < 2 && !how; ++way) {
        if ((way == 0 && l.stride < 12) || (way == 1 && l.stride < 8))
            continue;
        places.clear();
        for (UINT v = 0; v < l.vertices; ++v) {
            if (!used[v])
                continue;
            const uint8_t* at = data + l.verticesAt + v * l.stride;
            float p[3];
            if (way == 0) {
                std::memcpy(p, at, sizeof(p));
            } else {
                uint16_t h[3];
                std::memcpy(h, at, sizeof(h));
                for (int i = 0; i < 3; ++i)
                    p[i] = casings::HalfToFloat(h[i]);
            }
            places.insert(places.end(), p, p + 3);
        }
        MeshFit fit;
        if (casings::FitMesh(places.data(), static_cast<int>(places.size() / 3), scale, fit)) {
            how = way == 0 ? "three floats" : "four halves";
            s.fit = fit;
        }
    }
    if (!how) {
        uint32_t words[8] = {};
        std::memcpy(words, data + l.verticesAt, std::min<UINT>(l.stride, sizeof(words)));
        l.step = kLearnFailed;
        std::snprintf(l.note, sizeof(l.note), "no casing's shape in its vertices (stride %u, the first %08X %08X %08X %08X %08X %08X %08X %08X)", l.stride,
                      words[0], words[1], words[2], words[3], words[4], words[5], words[6], words[7]);
        return;
    }
    std::memcpy(s.specFromMesh, s.fit.specFromMesh, sizeof(s.specFromMesh));
    s.fitKnown = true;
    s.fitNews = true;
    l.step = kLearnDone;
    std::snprintf(l.note, sizeof(l.note), "%s at its vertex's start%s", how, scaleRead ? "" : ", the instance's scale unread (1)");
}

// Steps the readbacks on the immediate context. Under g_mutex.
void StepLearn(uint32_t frame) {
    for (KindState& s : g_kinds) {
        Learn& l = s.learn;
        if ((l.step != kLearnCopied && l.step != kLearnStaged) || !l.gpu || !l.cpu)
            continue;
        ID3D11Device* device = nullptr;
        l.gpu->GetDevice(&device);
        ID3D11DeviceContext* immediate = nullptr;
        if (device)
            device->GetImmediateContext(&immediate);
        if (immediate && l.step == kLearnCopied && frame - l.frame >= kLearnWait) {
            immediate->CopyResource(l.cpu, l.gpu);
            l.step = kLearnStaged;
            l.frame = frame;
        } else if (immediate && l.step == kLearnStaged) {
            D3D11_MAPPED_SUBRESOURCE mapped{};
            const HRESULT hr = immediate->Map(l.cpu, 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &mapped);
            if (hr == DXGI_ERROR_WAS_STILL_DRAWING) {
                if (frame - l.frame > kLearnStall)
                    l.step = kLearnNone, ++l.tries, std::snprintf(l.note, sizeof(l.note), "the graphics card never gave it back");
            } else if (SUCCEEDED(hr)) {
                ReadLearn(s, static_cast<const uint8_t*>(mapped.pData));
                immediate->Unmap(l.cpu, 0);
            } else {
                l.step = kLearnNone, ++l.tries;
                std::snprintf(l.note, sizeof(l.note), "it couldn't be mapped (%08lX)", static_cast<unsigned long>(hr));
            }
        }
        SafeRelease(immediate);
        SafeRelease(device);
        if (l.step == kLearnNone && l.tries >= kLearnTries)
            l.step = kLearnFailed;
    }
}

// The draw's own buffer of placements on this context. Under g_mutex.
ID3D11Buffer* PlacementsFor(ID3D11DeviceContext* ctx) {
    for (ContextBuffer& c : g_contextBuffers)
        if (c.ctx == ctx && c.buffer)
            return c.buffer;
    for (ContextBuffer& c : g_contextBuffers) {
        if (c.buffer)
            continue;
        ID3D11Device* device = nullptr;
        ctx->GetDevice(&device);
        if (!device)
            return nullptr;
        D3D11_BUFFER_DESC desc{kMostInstances * kInstanceStride, D3D11_USAGE_DYNAMIC, D3D11_BIND_VERTEX_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0};
        const HRESULT hr = device->CreateBuffer(&desc, nullptr, &c.buffer);
        device->Release();
        if (FAILED(hr)) {
            c.buffer = nullptr;
            return nullptr;
        }
        c.ctx = ctx;
        return c.buffer;
    }
    return nullptr;
}

// Everything Host changes, as it was, put back afterwards (its targets, viewports, scissors, compute stage and stream output it never touches).
struct Saved {
    ID3D11InputLayout* layout = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ID3D11Buffer* ib = nullptr;
    DXGI_FORMAT ibFormat = DXGI_FORMAT_UNKNOWN;
    UINT ibOffset = 0;
    ID3D11Buffer* vb[kVbSlots] = {};
    UINT stride[kVbSlots] = {}, offset[kVbSlots] = {};
    ID3D11VertexShader* vs = nullptr;
    ID3D11Buffer* vsCb[kCbSlots] = {};
    ID3D11ShaderResourceView* vsSrv[kVsSrvs] = {};
    ID3D11SamplerState* vsSampler[kVsSamplers] = {};
    ID3D11HullShader* hs = nullptr;
    ID3D11DomainShader* ds = nullptr;
    ID3D11GeometryShader* gs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11Buffer* psCb[kCbSlots] = {};
    ID3D11ShaderResourceView* psSrv[kPsSrvs] = {};
    ID3D11SamplerState* psSampler[kPsSamplers] = {};
    ID3D11RasterizerState* raster = nullptr;
    ID3D11BlendState* blend = nullptr;
    FLOAT blendFactor[4] = {};
    UINT sampleMask = 0xFFFFFFFFu;
    ID3D11DepthStencilState* depth = nullptr;
    UINT stencilRef = 0;
    ID3D11Predicate* predicate = nullptr;
    BOOL predicateValue = FALSE;

    void Save(ID3D11DeviceContext* c) {
        c->IAGetInputLayout(&layout);
        c->IAGetPrimitiveTopology(&topology);
        c->IAGetIndexBuffer(&ib, &ibFormat, &ibOffset);
        c->IAGetVertexBuffers(0, kVbSlots, vb, stride, offset);
        c->VSGetShader(&vs, nullptr, nullptr);
        c->VSGetConstantBuffers(0, kCbSlots, vsCb);
        c->VSGetShaderResources(0, kVsSrvs, vsSrv);
        c->VSGetSamplers(0, kVsSamplers, vsSampler);
        c->HSGetShader(&hs, nullptr, nullptr);
        c->DSGetShader(&ds, nullptr, nullptr);
        c->GSGetShader(&gs, nullptr, nullptr);
        c->PSGetShader(&ps, nullptr, nullptr);
        c->PSGetConstantBuffers(0, kCbSlots, psCb);
        c->PSGetShaderResources(0, kPsSrvs, psSrv);
        c->PSGetSamplers(0, kPsSamplers, psSampler);
        c->RSGetState(&raster);
        c->OMGetBlendState(&blend, blendFactor, &sampleMask);
        c->OMGetDepthStencilState(&depth, &stencilRef);
        c->GetPredication(&predicate, &predicateValue);
    }
    void Restore(ID3D11DeviceContext* c) {
        c->IASetInputLayout(layout);
        c->IASetPrimitiveTopology(topology);
        c->IASetIndexBuffer(ib, ibFormat, ibOffset);
        c->IASetVertexBuffers(0, kVbSlots, vb, stride, offset);
        c->VSSetShader(vs, nullptr, 0);
        c->VSSetConstantBuffers(0, kCbSlots, vsCb);
        c->VSSetShaderResources(0, kVsSrvs, vsSrv);
        c->VSSetSamplers(0, kVsSamplers, vsSampler);
        c->HSSetShader(hs, nullptr, 0);
        c->DSSetShader(ds, nullptr, 0);
        c->GSSetShader(gs, nullptr, 0);
        c->PSSetShader(ps, nullptr, 0);
        c->PSSetConstantBuffers(0, kCbSlots, psCb);
        c->PSSetShaderResources(0, kPsSrvs, psSrv);
        c->PSSetSamplers(0, kPsSamplers, psSampler);
        c->RSSetState(raster);
        c->OMSetBlendState(blend, blendFactor, sampleMask);
        c->OMSetDepthStencilState(depth, stencilRef);
        c->SetPredication(predicate, predicateValue);
        SafeRelease(layout);
        SafeRelease(ib);
        for (ID3D11Buffer*& b : vb)
            SafeRelease(b);
        SafeRelease(vs);
        for (ID3D11Buffer*& b : vsCb)
            SafeRelease(b);
        for (ID3D11ShaderResourceView*& v : vsSrv)
            SafeRelease(v);
        for (ID3D11SamplerState*& s : vsSampler)
            SafeRelease(s);
        SafeRelease(hs);
        SafeRelease(ds);
        SafeRelease(gs);
        SafeRelease(ps);
        for (ID3D11Buffer*& b : psCb)
            SafeRelease(b);
        for (ID3D11ShaderResourceView*& v : psSrv)
            SafeRelease(v);
        for (ID3D11SamplerState*& s : psSampler)
            SafeRelease(s);
        SafeRelease(raster);
        SafeRelease(blend);
        SafeRelease(depth);
        SafeRelease(predicate);
    }
};

// The kept constant buffers as they're bound again: the pass's own for the view; for one the pass has bound there too, the pass's (its constants as
// they are now); else the copy.
void PickCbs(const Cb* kept, ID3D11Buffer* const* host, ID3D11Buffer** out) {
    for (int i = 0; i < kCbSlots; ++i) {
        const Cb& c = kept[i];
        ID3D11Buffer* own = c.copy ? c.copy : c.original;
        switch (c.rule) {
        case kCbView: out[i] = host[i]; break;
        case kCbShared: out[i] = host[i] && host[i] == c.original ? host[i] : own; break;
        case kCbCopy: out[i] = own; break;
        default: out[i] = nullptr; break;
        }
    }
}

// The pass has its own view constants wherever the kept draw had its.
bool ViewsThere(const KeptState& s, ID3D11Buffer* const* hostVs, ID3D11Buffer* const* hostPs) {
    for (int pass = 0; pass < 2; ++pass) {
        const Cb* kept = pass ? s.psCb : s.vsCb;
        ID3D11Buffer* const* host = pass ? hostPs : hostVs;
        for (int i = 0; i < kCbSlots; ++i) {
            if (kept[i].rule != kCbView)
                continue;
            if (!host[i])
                return false;
            D3D11_BUFFER_DESC d{};
            host[i]->GetDesc(&d);
            if (d.ByteWidth != kept[i].size)
                return false;
        }
    }
    return true;
}

void Bind(ID3D11DeviceContext* ctx, const KeptState& s, ID3D11Buffer* const* hostVs, ID3D11Buffer* const* hostPs, ID3D11Buffer* placements,
          ID3D11BlendState* blend) {
    ctx->IASetInputLayout(s.layout);
    ctx->IASetPrimitiveTopology(s.topology);
    ctx->IASetIndexBuffer(s.ib, s.ibFormat, s.ibOffset);
    ID3D11Buffer* vbs[kVbSlots];
    UINT strides[kVbSlots], offsets[kVbSlots];
    for (int i = 0; i < kVbSlots; ++i)
        vbs[i] = s.vb[i], strides[i] = s.stride[i], offsets[i] = s.offset[i];
    vbs[1] = placements, strides[1] = kInstanceStride, offsets[1] = 0;
    ctx->IASetVertexBuffers(0, kVbSlots, vbs, strides, offsets);
    ID3D11Buffer* cbs[kCbSlots];
    ctx->VSSetShader(s.vs, nullptr, 0);
    PickCbs(s.vsCb, hostVs, cbs);
    ctx->VSSetConstantBuffers(0, kCbSlots, cbs);
    ctx->VSSetShaderResources(0, kVsSrvs, s.vsSrv);
    ctx->VSSetSamplers(0, kVsSamplers, s.vsSampler);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->PSSetShader(s.ps, nullptr, 0);
    PickCbs(s.psCb, hostPs, cbs);
    ctx->PSSetConstantBuffers(0, kCbSlots, cbs);
    ctx->PSSetShaderResources(0, kPsSrvs, s.psSrv);
    ctx->PSSetSamplers(0, kPsSamplers, s.psSampler);
    ctx->RSSetState(s.raster);
    ctx->OMSetBlendState(blend ? blend : s.blend, s.blendFactor, s.sampleMask);
    ctx->OMSetDepthStencilState(s.depth, s.stencilRef);
}

// ---------------------------------------------------------------- the game's own, hidden (design D)
constexpr char kHideShader[] = R"(
cbuffer Hide : register(b0) { float4 port; uint count, pad0, pad1, pad2; };
RWByteAddressBuffer dst : register(u0);
[numthreads(16, 1, 1)] void Main(uint3 id : SV_DispatchThreadID) {
    if (id.x >= count)
        return;
    const uint at = id.x * 48;
    const float3 p = float3(asfloat(dst.Load(at + 12)), asfloat(dst.Load(at + 28)), asfloat(dst.Load(at + 44)));
    // Near the game's own gun: the player's, which the mod throws its own in place of. All its vertices on one point: nothing drawn.
    if (distance(p, port.xyz) < port.w) {
        dst.Store4(at, uint4(0, 0, 0, 0));
        dst.Store4(at + 16, uint4(0, 0, 0, 0));
        dst.Store4(at + 32, uint4(0, 0, 0, 0));
    }
}
)";
std::mutex g_hideMutex;  // the shader made once
ID3D11ComputeShader* g_hideShader = nullptr;
bool g_hideFailed = false;
// Each thread's own (the draws come from several, each with its own context).
struct HideBuffers {
    ID3D11Buffer* out = nullptr;
    ID3D11UnorderedAccessView* view = nullptr;
    ID3D11Buffer* constants = nullptr;
};
thread_local HideBuffers t_hide;

bool EnsureHide(ID3D11Device* device) {
    {
        std::lock_guard lock(g_hideMutex);
        if (!g_hideShader && !g_hideFailed) {
            ID3DBlob* code = nullptr;
            ID3DBlob* errors = nullptr;
            const bool ok = SUCCEEDED(D3DCompile(kHideShader, sizeof(kHideShader) - 1, "casings hidden", nullptr, nullptr, "Main", "cs_5_0", 0, 0, &code, &errors)) &&
                            SUCCEEDED(device->CreateComputeShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &g_hideShader));
            SafeRelease(errors);
            SafeRelease(code);
            g_hideFailed = !ok;
        }
        if (!g_hideShader)
            return false;
    }
    if (t_hide.view)
        return true;
    D3D11_BUFFER_DESC out{kHideMost * kInstanceStride, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_UNORDERED_ACCESS, 0,
                          D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS, 0};
    D3D11_BUFFER_DESC constants{32, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0};
    D3D11_UNORDERED_ACCESS_VIEW_DESC uv{};
    uv.Format = DXGI_FORMAT_R32_TYPELESS;
    uv.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uv.Buffer.NumElements = kHideMost * kInstanceStride / 4;
    uv.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
    if (SUCCEEDED(device->CreateBuffer(&out, nullptr, &t_hide.out)) && SUCCEEDED(device->CreateBuffer(&constants, nullptr, &t_hide.constants)) &&
        SUCCEEDED(device->CreateUnorderedAccessView(t_hide.out, &uv, &t_hide.view)))
        return true;
    SafeRelease(t_hide.view);
    SafeRelease(t_hide.out);
    SafeRelease(t_hide.constants);
    return false;
}

}  // namespace

namespace casings {

// ---------------------------------------------------------------- settings

void SetMode(int mode) {
    const int was = g_mode.exchange(mode);
    if (was == mode)
        return;
    if (mode != 2) {
        g_wantArm = false;
        g_releaseWanted = true;  // let go of on the render thread (Update)
        for (std::atomic<bool>& d : g_drawable)
            d = false;
    }
    Log(mode == 2 ? "Casings: the mod's own (weapon.txt \"casings 2\"): thrown from the port of the gun in hand, lying a minute, drawn as the game's own"
                  : "Casings: the mod's own off (weapon.txt \"casings %d\")",
        mode);
}

int Mode() {
    return g_mode.load(std::memory_order_relaxed);
}

void SetMeshCount(int kind, unsigned indexCount) {
    if (kind < 0 || kind >= kKinds)
        return;
    if (g_counts[kind].exchange(indexCount) != indexCount) {
        g_countChanged[kind] = true;
        Log("Casings: the game's %s casing draw has %u indices%s", kKindNames[kind], indexCount, indexCount ? "" : " (none: the game's own)");
    }
}

void SetPasses(int most) {
    most = std::clamp(most, 1, 16);
    if (g_mostPasses.exchange(most) != most)
        Log("Casings: drawn into %d eye passes a frame at most", most);
}

void SetLog(bool on) {
    g_logOn = on;
}

void SetAnyContext(bool on) {
    if (g_anyContext.exchange(on) != on)
        Log("Casings: armed for %s", on ? "any context's draws into an eye's depth" : "the draws of the context that cleared it");
}

bool Drawable(int kind) {
    return kind >= 0 && kind < kKinds && g_drawable[kind].load(std::memory_order_relaxed);
}

void Clear() {
    std::lock_guard lock(g_mutex);
    ClearLocked();
    DisarmAll();
    g_wantArm = false;
    g_offUntil = 0;
}

// ---------------------------------------------------------------- once a frame

int TakeShots(const Emitter* gun) {
    const int shots = g_shotsThisFrame.exchange(0);
    if (g_mode.load(std::memory_order_relaxed) != 2)
        return 0;
    g_gunKind = gun ? static_cast<int>(gun->kind) : -1;
    g_gunAt = GetTickCount64();
    if (!gun || !Drawable(gun->kind) || shots <= 0)
        return 0;
    g_thrown.fetch_add(static_cast<uint32_t>(shots), std::memory_order_relaxed);
    return std::min(shots, 3);
}

void SetLoose(const loose::Result& r, const loose::Frame& f) {
    if (g_mode.load(std::memory_order_relaxed) != 2)
        return;
    float look[3] = {f.look[0], f.look[1], f.look[2]};
    const bool haveLook = Normalize3(look);
    std::lock_guard lock(g_mutex);
    float fits[kKinds][12];
    bool known[kKinds];
    for (int k = 0; k < kKinds; ++k) {
        std::memcpy(fits[k], g_kinds[k].specFromMesh, sizeof(fits[k]));
        known[k] = g_kinds[k].fitKnown && g_drawable[k].load(std::memory_order_relaxed);
    }
    g_instanceCount = PackAll(r.pose, r.poses, f.origin, fits, known, g_rows, kMostInstances, g_first, g_count);
    g_flying = g_lying = g_inView = 0;
    for (int p = 0; p < r.poses; ++p) {
        const loose::Pose& pose = r.pose[p];
        if (!IsLooseCasing(pose.kind))
            continue;
        ++(pose.resting ? g_lying : g_flying);
        if (!known[pose.kind - kLooseKind])
            continue;
        const float to[3] = {pose.bone[3] - f.eye[0], pose.bone[7] - f.eye[1], pose.bone[11] - f.eye[2]};
        const float distance = Length3(to);
        if (distance < 0.5f || (haveLook && distance < 60.0f && to[0] * look[0] + to[1] * look[1] + to[2] * look[2] > f.seenCos * distance))
            ++g_inView;
    }
    g_wantArm = g_instanceCount > 0 && g_inView > 0;
}

void Update() {
    const uint32_t frame = g_frame.fetch_add(1, std::memory_order_relaxed) + 1;
    const ULONGLONG now = GetTickCount64();
    const int mode = g_mode.load(std::memory_order_relaxed);
    const uint32_t hostedLastFrame = g_passesThisFrame.exchange(0, std::memory_order_relaxed);
    // A new world: all of the last one's let go of (its meshes, textures and shaders, and the casings: thrown there).
    static uint32_t s_world = 0;
    static bool s_haveWorld = false;
    uint32_t world = 0;
    camera::SafeRead(kWorldAt, &world, sizeof(world));
    if (s_haveWorld && world != s_world) {
        Clear();
        if (mode == 2)
            Log("Casings: another world (%08X): what was kept of the last let go of", world);
    }
    s_world = world, s_haveWorld = true;
    if (g_releaseWanted.exchange(false))
        Clear();
    for (int k = 0; k < kKinds; ++k)
        if (g_countChanged[k].exchange(false)) {
            std::lock_guard lock(g_mutex);
            ReleaseKind(g_kinds[k]);
            g_keptAt[k] = 0, g_depthKeptAt[k] = 0, g_wantDepth[k] = false, g_drawable[k] = false;
        }
    // The arms not hosted in two frames go.
    {
        std::lock_guard lock(g_armMutex);
        for (Arm& arm : g_arms)
            if (arm.ctx.load(std::memory_order_relaxed) && frame - arm.frame > kArmFrames) {
                DisarmLocked(arm);
                g_why[kWhyOld].fetch_add(1, std::memory_order_relaxed);
            }
    }
    static int s_loaded = -1;
    if (mode != 2) {
        g_shotsThisFrame = 0;
        s_loaded = -1;
        return;
    }
    // The shots: the rounds in the gun going down, with the trigger (the headset's comes as the left mouse button: input.cpp) held lately; not
    // the grenade launcher's.
    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
        g_triggerAt = now;
    int loaded = -1;
    if (!ammo::RoundsInHand(loaded))
        loaded = -1;
    const int fired = ammo::M320UpOrRecent() ? 0 : RoundsFired(s_loaded, loaded, now - g_triggerAt.load(std::memory_order_relaxed) <= kTriggerMs);
    s_loaded = loaded;
    g_shotsThisFrame = fired;
    if (fired)
        g_shotAt = now;
    // What can be drawn; the readbacks; what's to be said.
    struct Said {
        char text[600];
    };
    std::vector<Said> said;
    bool anyDrawable = false;
    // Firing a gun whose casings aren't drawable for 3 s: why, once (till they are).
    static ULONGLONG s_undrawnSince = 0;
    static bool s_undrawnSaid = false;
    const int gunKind = g_gunKind.load(std::memory_order_relaxed);
    {
        std::lock_guard lock(g_mutex);
        StepLearn(frame);
        for (int k = 0; k < kKinds; ++k) {
            KindState& s = g_kinds[k];
            const bool can = s.colour.have && s.fitKnown && (!s.colour.depthEqual || s.depth.have) && now >= g_offUntil.load(std::memory_order_relaxed);
            g_drawable[k] = can;
            anyDrawable = anyDrawable || can;
            const UINT count = g_counts[k].load(std::memory_order_relaxed);
            if (k == gunKind) {
                if (can) {
                    s_undrawnSince = 0, s_undrawnSaid = false;
                } else if (fired && !s_undrawnSince) {
                    s_undrawnSince = now;
                } else if (s_undrawnSince && now - s_undrawnSince > 3000 && !s_undrawnSaid) {
                    s_undrawnSaid = true;
                    Said line;
                    char why[300];
                    if (!count)
                        std::snprintf(why, sizeof(why), "its casing draw's index count isn't known yet (weapon.txt \"flashsurvey\", then \"casingsmesh %d <count>\")", k);
                    else if (!s.colour.have && s.refused != kKeptFine)
                        std::snprintf(why, sizeof(why), "its %u-index draw not kept: %s", count, kKeepWhyNames[s.refused]);
                    else if (!s.colour.have)
                        std::snprintf(why, sizeof(why), "no %u-index draw seen in an eye's picture (three or more targets, not first person)", count);
                    else if (!s.fitKnown)
                        std::snprintf(why, sizeof(why), "its mesh not read yet (%s)", s.learn.note[0] ? s.learn.note : "reading");
                    else if (s.colour.depthEqual && !s.depth.have)
                        std::snprintf(why, sizeof(why), "its draw tests depth EQUAL and its depth pass's draw isn't kept yet");
                    else
                        std::snprintf(why, sizeof(why), "off a while: the eyes' passes weren't found");
                    std::snprintf(line.text, sizeof(line.text), "Casings: firing for 3 s and none of the mod's %s casings drawn (the game's are): %s", kKindNames[k], why);
                    said.push_back(line);
                }
            }
            if (s.keptNews) {
                s.keptNews = false;
                Said line;
                char vs[200] = "", ps[200] = "";
                DescribeCbs(s.colour.vsCb, vs, sizeof(vs), true);
                DescribeCbs(s.colour.psCb, ps, sizeof(ps), true);
                std::snprintf(line.text, sizeof(line.text),
                              "Casings: the game's %u-index casing draw kept (the %s): %u targets on a depth of %ux%u (%u samples, format %d), stencil %u, depth %s%s; vertex "
                              "shader's constants%s; pixel shader's%s; %u textures",
                              count, kKindNames[k], s.pass.targets, s.pass.width, s.pass.height, s.pass.samples, static_cast<int>(s.pass.depthFormat),
                              s.colour.stencilRef, s.colour.depthEqual ? "EQUAL after its depth pass" : "written",
                              s.colour.depthEqual ? (s.depth.have ? " (its depth pass's draw kept too)" : " (its depth pass's draw not kept yet)") : "",
                              vs[0] ? vs : " none", ps[0] ? ps : " none", s.colour.psTextures);
                said.push_back(line);
            }
            if (s.refusedNews) {
                s.refusedNews = false;
                Said line;
                std::snprintf(line.text, sizeof(line.text), "Casings: the game's %u-index casing draw not kept: %s (its vertex shader's constants%s)", count,
                              kKeepWhyNames[s.refused], s.refusedBuffers[0] ? s.refusedBuffers : " none");
                said.push_back(line);
            }
            if (s.fitNews) {
                s.fitNews = false;
                const loose::Spec* spec = loose::SpecFor(kSpecNames[k]);
                const float specLength = spec ? 1000.0f * (spec->ringAt[spec->rings - 1] - spec->ringAt[0]) : 0.0f;
                Said line;
                std::snprintf(line.text, sizeof(line.text),
                              "Casings: the %s's mesh read (%s): %.1f mm long along its %c from its %s end%s, its base %.1f mm across, its mouth %.1f; the game draws it "
                              "%.2f times; the mod's casing is %.1f mm",
                              kKindNames[k], s.learn.note, s.fit.length * 1000.0f, "xyz"[s.fit.axis], s.fit.baseAtMax ? "highest" : "lowest",
                              s.fit.ambiguous ? " (its ends alike)" : "", 2000.0f * s.fit.baseRadius, 2000.0f * s.fit.mouthRadius, s.fit.scale, specLength);
                said.push_back(line);
            }
            if (s.learn.step == kLearnFailed && !s.learn.said) {
                s.learn.said = true;
                Said line;
                std::snprintf(line.text, sizeof(line.text), "Casings: the %s's mesh couldn't be read (%s): the mod's casings of it aren't drawn, the game's are", kKindNames[k],
                              s.learn.note);
                said.push_back(line);
            }
        }
    }
    if (!anyDrawable)
        g_wantArm = false;
    // Casings in view a while and no pass drawn into: something here isn't as the game draws, so the game's own come back for a while.
    static ULONGLONG s_wantSince = 0, s_lastHosted = 0;
    if (hostedLastFrame > 0)
        s_lastHosted = now;
    if (g_wantArm.load(std::memory_order_relaxed)) {
        if (!s_wantSince)
            s_wantSince = now;
    } else {
        s_wantSince = 0;
    }
    if (s_wantSince && now - s_wantSince > kWatchMs && now - s_lastHosted > kWatchMs && now >= g_offUntil.load(std::memory_order_relaxed)) {
        char reasons[400] = "";
        int used = 0;
        for (int w = 0; w < kWhys && used < 380; ++w)
            if (const uint32_t n = g_why[w].load(std::memory_order_relaxed))
                used += std::snprintf(reasons + used, sizeof(reasons) - used, " %u %s;", n, kWhyNames[w]);
        s_wantSince = 0;
        if (!g_anyContext.exchange(true)) {
            // First: the clears may be on one context and the pass recorded on another.
            Log("Casings: in view %.1f s and no eye pass drawn into: armed for any context's draws from now (since the last line:%s)", kWatchMs / 1000.0,
                used ? reasons : " no arming at all");
        } else {
            g_offUntil = now + kOffMs;
            for (std::atomic<bool>& d : g_drawable)
                d = false;
            g_wantArm = false;
            Log("Casings: in view %.1f s and no eye pass drawn into: the game's own back for %.0f s (since the last line:%s)", kWatchMs / 1000.0, kOffMs / 1000.0,
                used ? reasons : " no arming at all");
        }
    }
    for (const Said& s : said)
        Log("%s", s.text);
    // Once a second while anything's happening: two eye passes a frame drawn into is right (one: an eye without them).
    static ULONGLONG s_lineAt = 0;
    static uint32_t s_lineFrame = 0;
    if (now - s_lineAt >= 1000) {
        s_lineAt = now;
        const uint32_t frames = frame - s_lineFrame;
        s_lineFrame = frame;
        const uint32_t thrown = g_thrown.exchange(0), passes = g_hostedPasses.exchange(0), draws = g_hostedDraws.exchange(0), hidden = g_hidden.exchange(0),
                       hideNot = g_hideNot.exchange(0), viewSame = g_viewSame.exchange(0), viewOther = g_viewOther.exchange(0);
        const int64_t ticks = g_hostTicks.exchange(0), worst = g_hostWorst.exchange(0);
        char reasons[400] = "";
        int used = 0;
        for (int w = 0; w < kWhys && used < 380; ++w)
            if (const uint32_t n = g_why[w].exchange(0))
                used += std::snprintf(reasons + used, sizeof(reasons) - used, " %u %s;", n, kWhyNames[w]);
        int flying, lying, inView, drawn;
        {
            std::lock_guard lock(g_mutex);
            flying = g_flying, lying = g_lying, inView = g_inView, drawn = g_instanceCount;
        }
        if (g_logOn.load(std::memory_order_relaxed) && (thrown || passes || hidden || hideNot || used || flying || lying))
            Log("Casings: last second %u thrown; %d flying, %d lying, %d drawn, %d in view; drawn into %u eye passes in %u frames (%u draws, %.1f us each on average, "
                "%.1f at the worst; the pass's view in the kept draw's own buffer %u times, another %u); %u of the game's casing draws hidden (%u not: no "
                "buffers)%s%s",
                thrown, flying, lying, drawn, inView, passes, frames, draws, passes ? 1e6 * static_cast<double>(ticks) / TicksPerSecond() / passes : 0.0,
                1e6 * static_cast<double>(worst) / TicksPerSecond(), viewSame, viewOther, hidden, hideNot, used ? "; not drawn into:" : "", reasons);
    }
}

// ---------------------------------------------------------------- the draw hooks

void NoteClear(ID3D11DeviceContext* ctx, ID3D11DepthStencilView* dsv, UINT flags) {
    constexpr UINT kBoth = D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL;
    if (g_mode.load(std::memory_order_relaxed) != 2 || !g_wantArm.load(std::memory_order_relaxed) || !ctx || !dsv || (flags & kBoth) != kBoth ||
        scopeview::Drawing())
        return;
    ID3D11Resource* resource = nullptr;
    dsv->GetResource(&resource);
    bool ok = false;
    const D3D11_TEXTURE2D_DESC desc = TextureDesc(resource, ok);
    if (ok) {
        std::lock_guard lock(g_armMutex);
        if (g_armDepth.have && desc.Width == g_armDepth.width && desc.Height == g_armDepth.height && desc.SampleDesc.Count == g_armDepth.samples &&
            desc.Format == g_armDepth.format) {
            // Its own context's arming again (a clear for its next pass); any context's, one for each clear (both eyes may be cleared first).
            const bool any = g_anyContext.load(std::memory_order_relaxed);
            ID3D11DeviceContext* const armedBy = any ? kAnyContext : ctx;
            Arm* arm = nullptr;
            for (Arm& a : g_arms)
                if (!any && a.ctx.load(std::memory_order_relaxed) == ctx)
                    arm = &a;
            for (Arm& a : g_arms)
                if (!arm && !a.ctx.load(std::memory_order_relaxed))
                    arm = &a;
            if (!arm)  // all taken: the oldest goes
                for (Arm& a : g_arms)
                    if (!arm || a.frame < arm->frame)
                        arm = &a;
            arm->texture = resource;
            arm->frame = g_frame.load(std::memory_order_relaxed);
            arm->draws = arm->waits = 0;
            if (!arm->ctx.exchange(armedBy))
                g_armed.fetch_add(1, std::memory_order_relaxed);
        }
    }
    SafeRelease(resource);
}

bool ArmedSlow(ID3D11DeviceContext* ctx) {
    for (const Arm& arm : g_arms)
        if (ArmsContext(arm, ctx))
            return true;
    return false;
}

void Host(ID3D11DeviceContext* ctx, const DrawIndexedInstancedFn* original) {
    if (!ctx || !original || scopeview::Drawing())
        return;
    // Quickly first, with no lock (draws on other threads come here too while any context is armed): a depth pass's draws (no targets) before the
    // eye's pass wait without a look at anything else (an arming that never finds its pass goes in two frames: Update).
    {
        ID3D11RenderTargetView* third[3] = {};
        ctx->OMGetRenderTargets(3, third, nullptr);
        const bool three = third[2] != nullptr;
        for (ID3D11RenderTargetView*& v : third)
            SafeRelease(v);
        if (!three)
            return;
    }
    // One of the eye's pass's own draws: depth-tested, not first person (those may come first in its pass: still quickly), on an armed depth. Any
    // other waits (still armed); one of its pass's that hasn't the view bound, or is of a view just drawn into, waits so many times.
    {
        ID3D11DepthStencilState* state = nullptr;
        UINT ref = 0;
        ctx->OMGetDepthStencilState(&state, &ref);
        D3D11_DEPTH_STENCIL_DESC d{};
        if (state)
            state->GetDesc(&d);
        else
            d.DepthEnable = TRUE, d.DepthFunc = D3D11_COMPARISON_LESS;
        SafeRelease(state);
        if ((ref & kFirstPersonStencil) || !d.DepthEnable || d.DepthFunc == D3D11_COMPARISON_ALWAYS || d.DepthFunc == D3D11_COMPARISON_NEVER)
            return;
    }
    Pass pass;
    LookAtPass(ctx, pass);
    if (!pass.haveDepth || pass.targets < 3)
        return;
    {
        std::lock_guard lock(g_armMutex);
        Arm* arm = ArmForLocked(ctx, pass.depthTexture);
        if (!arm)
            return;
        if (++arm->draws > kArmDraws) {  // (draws of its kind of pass on its depth: each a look)
            DisarmLocked(*arm);
            g_why[kWhyLong].fetch_add(1, std::memory_order_relaxed);
            return;
        }
    }
    const int64_t began = Ticks();
    ID3D11Buffer* hostVs[kCbSlots] = {};
    ID3D11Buffer* hostPs[kCbSlots] = {};
    ctx->VSGetConstantBuffers(0, kCbSlots, hostVs);
    ctx->PSGetConstantBuffers(0, kCbSlots, hostPs);
    auto releaseHost = [&] {
        for (ID3D11Buffer*& b : hostVs)
            SafeRelease(b);
        for (ID3D11Buffer*& b : hostPs)
            SafeRelease(b);
    };
    std::lock_guard lock(g_mutex);
    // Each kind to draw here: drawable, some to draw, kept in this kind of pass, its view constants there.
    int kinds[kKinds];
    int n = 0;
    bool anyDrawable = false, anyHere = false, viewMissing = false;
    for (int k = 0; k < kKinds; ++k) {
        const KindState& s = g_kinds[k];
        if (!g_count[k] || !g_drawable[k].load(std::memory_order_relaxed) || !s.colour.have || (s.colour.depthEqual && !s.depth.have))
            continue;
        anyDrawable = true;
        if (!s.pass.SameAs(pass))
            continue;
        anyHere = true;
        if (!ViewsThere(s.colour, hostVs, hostPs) || (s.colour.depthEqual && !ViewsThere(s.depth, hostVs, hostPs))) {
            viewMissing = true;
            continue;
        }
        kinds[n++] = k;
    }
    if (viewMissing && !n) {  // this draw hasn't the view bound: the next may (still armed)
        releaseHost();
        Wait(ctx, pass.depthTexture, kWhyView);
        return;
    }
    bool claimed = false;
    auto refuse = [&](Why why) {  // this pass isn't drawn into: its arming goes
        g_why[why].fetch_add(1, std::memory_order_relaxed);
        releaseHost();
        if (!claimed)
            Claim(ctx, pass.depthTexture);
    };
    if (!g_instanceCount)
        return refuse(kWhyNothing);
    if (!n)
        return refuse(!anyDrawable ? kWhyNotDrawable : anyHere ? kWhyView : kWhyPass);
    if (g_passesThisFrame.load(std::memory_order_relaxed) >= static_cast<uint32_t>(g_mostPasses.load(std::memory_order_relaxed)))
        return refuse(kWhyPasses);
    // The same eye's view drawn into just now: a draw of the pass already drawn into (a second clear in it, or both eyes' armed first), by its view
    // constants as this thread last uploaded them. It waits: the other eye's pass may take the arming.
    const double nowSeconds = static_cast<double>(began) / TicksPerSecond();
    float eye[3] = {};
    bool haveEye = false;
    {
        int viewSlot = -1;
        for (int i = 0; i < kCbSlots && viewSlot < 0; ++i)
            if (g_kinds[kinds[0]].colour.vsCb[i].rule == kCbView)
                viewSlot = i;
        if (viewSlot >= 0)  // (for the log: whether the pass's view is in the kept draw's own buffer: the game's one for them all)
            (hostVs[viewSlot] == g_kinds[kinds[0]].colour.vsCb[viewSlot].original ? g_viewSame : g_viewOther).fetch_add(1, std::memory_order_relaxed);
        ID3D11Resource* uploaded = nullptr;
        const uint8_t* view = viewSlot >= 0 ? skin::LatestViewConstants(uploaded) : nullptr;
        if (view && uploaded && uploaded == hostVs[viewSlot]) {
            std::memcpy(eye, view + kEyeAt, sizeof(eye));
            haveEye = Finite(eye, 3) && Length3(eye) > kFirstPersonReach;
        }
        if (haveEye)
            for (const Hosted& h : g_recent) {
                const float d[3] = {eye[0] - h.eye[0], eye[1] - h.eye[1], eye[2] - h.eye[2]};
                if (h.at >= 0.0 && nowSeconds - h.at < kRepeatSeconds && Length3(d) < kRepeatMetres) {
                    releaseHost();
                    Wait(ctx, pass.depthTexture, kWhyRepeat);
                    return;
                }
            }
    }
    // From here this pass is drawn into now or not at all (another thread may have taken its arming meanwhile).
    if (!Claim(ctx, pass.depthTexture)) {
        releaseHost();
        return;
    }
    claimed = true;
    // The placements, uploaded on this context.
    ID3D11Buffer* placements = PlacementsFor(ctx);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (!placements || FAILED(ctx->Map(placements, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        return refuse(kWhyMap);
    std::memcpy(mapped.pData, g_rows, static_cast<size_t>(g_instanceCount) * kInstanceStride);
    ctx->Unmap(placements, 0);
    if (std::any_of(kinds, kinds + n, [](int k) { return g_kinds[k].colour.depthEqual; }) && !g_noColour) {
        ID3D11Device* device = nullptr;
        ctx->GetDevice(&device);
        D3D11_BLEND_DESC none{};
        none.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE, none.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO, none.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        none.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE, none.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO, none.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        none.RenderTarget[0].RenderTargetWriteMask = 0;
        if (device && FAILED(device->CreateBlendState(&none, &g_noColour)))
            g_noColour = nullptr;
        SafeRelease(device);
    }
    const DrawIndexedInstancedFn draw = ctx->GetType() == D3D11_DEVICE_CONTEXT_DEFERRED && original[1] ? original[1] : original[0];
    if (!draw)
        return refuse(kWhyMap);
    Saved saved;
    saved.Save(ctx);
    ctx->SetPredication(nullptr, FALSE);
    uint32_t draws = 0;
    for (int i = 0; i < n; ++i) {
        const int k = kinds[i];
        const KindState& s = g_kinds[k];
        const UINT instances = static_cast<UINT>(g_count[k]), first = static_cast<UINT>(g_first[k]);
        if (s.colour.depthEqual) {
            if (!g_noColour)
                continue;
            Bind(ctx, s.depth, hostVs, hostPs, placements, g_noColour);
            draw(ctx, s.depth.count, instances, s.depth.start, s.depth.base, first);
            ++draws;
        }
        Bind(ctx, s.colour, hostVs, hostPs, placements, nullptr);
        draw(ctx, s.colour.count, instances, s.colour.start, s.colour.base, first);
        ++draws;
    }
    saved.Restore(ctx);
    releaseHost();
    if (haveEye) {
        Hosted& h = g_recent[g_recentNext];
        g_recentNext = (g_recentNext + 1) % static_cast<int>(std::size(g_recent));
        std::memcpy(h.eye, eye, sizeof(eye));
        h.at = nowSeconds;
    }
    g_passesThisFrame.fetch_add(1, std::memory_order_relaxed);
    g_hostedPasses.fetch_add(1, std::memory_order_relaxed);
    g_hostedDraws.fetch_add(draws, std::memory_order_relaxed);
    const int64_t took = Ticks() - began;
    g_hostTicks.fetch_add(took, std::memory_order_relaxed);
    for (int64_t worst = g_hostWorst.load(std::memory_order_relaxed); took > worst && !g_hostWorst.compare_exchange_weak(worst, took);) {
    }
}

void KeepDraw(ID3D11DeviceContext* ctx, UINT count, UINT instances, UINT start, INT base, UINT first) {
    if (g_mode.load(std::memory_order_relaxed) != 2 || !ctx || !instances)
        return;
    const int k = KindOfCount(count);
    if (k < 0)
        return;
    const ULONGLONG now = GetTickCount64();
    const bool colourDue = now - g_keptAt[k].load(std::memory_order_relaxed) >= kRefreshMs;
    const bool depthDue = g_wantDepth[k].load(std::memory_order_relaxed) && now - g_depthKeptAt[k].load(std::memory_order_relaxed) >= kRefreshMs;
    if ((!colourDue && !depthDue) || scopeview::Drawing())
        return;
    // In an eye's picture: not first person, three or more targets (its G-buffer) on a depth target the size of the first; or, for a draw that tests
    // EQUAL, its depth pass's draw: no targets, on a depth like the kept draw's.
    {
        ID3D11DepthStencilState* state = nullptr;
        UINT ref = 0;
        ctx->OMGetDepthStencilState(&state, &ref);
        SafeRelease(state);
        if (ref & kFirstPersonStencil)
            return;
    }
    Pass pass;
    LookAtPass(ctx, pass);
    const bool colours = pass.targets >= 3 && WritesColour(ctx, pass.targets);
    const bool colour = colourDue && colours && pass.haveDepth && pass.width >= 512 && pass.height >= 512 && pass.width == pass.targetWidth &&
                        pass.height == pass.targetHeight;
    const bool depthOnly = depthDue && pass.haveDepth && !colours && (pass.targets == 0 || pass.targets >= 3);
    if (!colour && !depthOnly) {
        // The sun's cascades' draws of it, and the like: so many looks, then a rest (a draw in an eye's picture comes among the first of them).
        if (g_looks[k].fetch_add(1, std::memory_order_relaxed) >= kMostLooks) {
            g_looks[k] = 0;
            if (colourDue)
                g_keptAt[k] = now - kRefreshMs + kKeepRetryMs;
            if (depthDue)
                g_depthKeptAt[k] = now - kRefreshMs + kKeepRetryMs;
        }
        return;
    }
    g_looks[k] = 0;
    ID3D11Device* device = nullptr;
    ctx->GetDevice(&device);
    if (!device)
        return;
    std::lock_guard lock(g_mutex);
    KindState& s = g_kinds[k];
    if (colour) {
        KeptState fresh;
        const KeepWhy why = Capture(ctx, device, count, start, base, true, fresh, s.colour);
        if (why != kKeptFine) {
            if (s.refused != why) {
                s.refused = why, s.refusedNews = true;
                DescribeCbs(fresh.vsCb, s.refusedBuffers, sizeof(s.refusedBuffers), false);
            }
            fresh.Release();
            g_keptAt[k] = now - kRefreshMs + kKeepRetryMs;
        } else {
            const bool news = !s.colour.have || !s.pass.SameAs(pass) || s.colour.depthEqual != fresh.depthEqual;
            s.colour.Release();
            s.colour = fresh;  // (its references go with it)
            fresh = KeptState{};
            s.pass = pass;
            s.pass.depthTexture = nullptr;
            s.refused = kKeptFine;
            s.keptNews = s.keptNews || news;
            g_keptAt[k] = now;
            g_wantDepth[k] = s.colour.depthEqual;
            {
                std::lock_guard arms(g_armMutex);
                g_armDepth.have = true;
                g_armDepth.width = pass.width, g_armDepth.height = pass.height, g_armDepth.samples = pass.samples, g_armDepth.format = pass.depthFormat;
            }
            if (!s.fitKnown && s.learn.step == kLearnNone && s.learn.tries < kLearnTries)
                StartLearn(ctx, device, s, first);
        }
    } else if (s.colour.have && s.pass.haveDepth && pass.width == s.pass.width && pass.height == s.pass.height && pass.samples == s.pass.samples &&
               pass.depthFormat == s.pass.depthFormat) {
        KeptState fresh;
        if (Capture(ctx, device, count, start, base, false, fresh, s.depth) == kKeptFine) {
            const bool news = !s.depth.have;
            s.depth.Release();
            s.depth = fresh;
            fresh = KeptState{};
            s.keptNews = s.keptNews || news;
            g_depthKeptAt[k] = now;
        } else {
            fresh.Release();
            g_depthKeptAt[k] = now - kRefreshMs + kKeepRetryMs;
        }
    }
    device->Release();
}

bool HideGameCasings(ID3D11DeviceContext* ctx, UINT count, UINT instances, UINT first, const std::function<void(UINT)>& draw) {
    if (g_mode.load(std::memory_order_relaxed) != 2 || !ctx || !instances || instances > kHideMost)
        return false;
    const int k = KindOfCount(count);
    if (k < 0 || !g_drawable[k].load(std::memory_order_relaxed))
        return false;
    const ULONGLONG now = GetTickCount64();
    if (g_gunKind.load(std::memory_order_relaxed) != k || now - g_gunAt.load(std::memory_order_relaxed) > kGunFreshMs ||
        now - std::max(g_triggerAt.load(std::memory_order_relaxed), g_shotAt.load(std::memory_order_relaxed)) >= kHideMs)
        return false;
    float port[3];
    if (!skin::GameGunPort(port))
        return false;  // the game's gun not known: where the game has them (effects.cpp's move)
    ID3D11Buffer* stream = nullptr;
    UINT stride = 0, offset = 0;
    ctx->IAGetVertexBuffers(1, 1, &stream, &stride, &offset);
    if (!stream || stride != kInstanceStride) {
        SafeRelease(stream);
        return false;
    }
    D3D11_BUFFER_DESC streamDesc{};
    stream->GetDesc(&streamDesc);
    const UINT64 left = static_cast<UINT64>(offset) + static_cast<UINT64>(first) * kInstanceStride;
    if (left + static_cast<UINT64>(instances) * kInstanceStride > streamDesc.ByteWidth) {
        stream->Release();
        return false;
    }
    ID3D11Device* device = nullptr;
    ctx->GetDevice(&device);
    const bool ready = device && EnsureHide(device);
    SafeRelease(device);
    if (!ready) {
        stream->Release();
        g_hideNot.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    struct {
        float port[4];
        UINT count, pad[3];
    } constants{{port[0], port[1], port[2], kHideReach}, instances, {0, 0, 0}};
    ctx->UpdateSubresource(t_hide.constants, 0, nullptr, &constants, 0, 0);
    const D3D11_BOX box{static_cast<UINT>(left), 0, 0, static_cast<UINT>(left) + instances * kInstanceStride, 1, 1};
    ctx->CopySubresourceRegion(t_hide.out, 0, 0, 0, 0, stream, 0, &box);
    // The compute stage as the game had it, put back afterwards.
    ID3D11ComputeShader* cs = nullptr;
    ID3D11UnorderedAccessView* uav = nullptr;
    ID3D11Buffer* cb = nullptr;
    ctx->CSGetShader(&cs, nullptr, nullptr);
    ctx->CSGetUnorderedAccessViews(0, 1, &uav);
    ctx->CSGetConstantBuffers(0, 1, &cb);
    const UINT keepCount = UINT_MAX;  // an append buffer's counter left as it is
    ctx->CSSetShader(g_hideShader, nullptr, 0);
    ctx->CSSetUnorderedAccessViews(0, 1, &t_hide.view, &keepCount);
    ctx->CSSetConstantBuffers(0, 1, &t_hide.constants);
    ctx->Dispatch((instances + 15) / 16, 1, 1);
    ID3D11UnorderedAccessView* noUav = nullptr;
    ctx->CSSetUnorderedAccessViews(0, 1, &noUav, &keepCount);  // before it's read as a vertex buffer
    ctx->CSSetShader(cs, nullptr, 0);
    ctx->CSSetUnorderedAccessViews(0, 1, &uav, &keepCount);
    ctx->CSSetConstantBuffers(0, 1, &cb);
    SafeRelease(cs);
    SafeRelease(uav);
    SafeRelease(cb);
    const UINT zero = 0;
    ctx->IASetVertexBuffers(1, 1, &t_hide.out, &stride, &zero);
    draw(0);
    ctx->IASetVertexBuffers(1, 1, &stream, &stride, &offset);
    stream->Release();
    g_hidden.fetch_add(1, std::memory_order_relaxed);
    return true;
}

}  // namespace casings

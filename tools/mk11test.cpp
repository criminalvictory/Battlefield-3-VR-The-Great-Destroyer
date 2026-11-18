// The Mk11 Mod 0 and its rifle scope (Operation Swordbreaker), held to what was measured, without the game: skin.cpp's rows for it (copied here, as
// the other tests keep theirs) against the mesh dumps and the recordings they were read from (build\mesh_12456_2214_*, 1902_0, 312_1902, 12642_156,
// 156_0, 480_0, 558_480; build\survey\MK11_partial.bin, MK11_empty.bin, MK11_dry.bin), and the tables written from them (src/scopemarks.h's
// kRifleScope, src/recoilguns.h's row, src/gunbodies.h's body and attachment, src/shaperecipes.h's recipe).
//   k1  the tables: one row for it in each (its recoil row its body draw and shape, 10 and one in the chamber, 550 m/s; its body, its scope's corner
//       points, its shape's recipe), every number finite
//   k2  the mesh (SKIP without the dumps): its magazine where the row has it (its box, its top, the well's mouth), its T handle's ears at the lever's
//       place, the bolt catch's paddle at the release on the receiver's left, its muzzle, its port under its cover, the piece of two bones
//       (1902/0: the frame's plate and the magazine's sleeve)
//   k3  the scope on the gun (SKIP without the dumps): its glass's eyepiece ring where the sight's row has it (its middle, its radius), the objective
//       further on; the zoomed eye on the eyepiece's own line, 11.4 cm behind it; the power the game's zoom; the scope's bone on the frame puts that
//       line at the row's sights' line and the eye at the row's eye; the lens at the game's own eye big enough for the second renderer's picture
//       (80 pixels' radius and more) and how far back that holds
//   k4  the reticle (SKIP without the dumps): every triangle of kRifleScope made again here from the dumps, to 1e-4 of slope; the cross centred on
//       the line; four thick bars and sixteen dots, evenly spaced, the same on each arm; the bars' inner ends inside what the lens shows
//   k5  the recordings (SKIP without them): the rounds' draw's bone 0, the piece's bone 1 and the depth pass's bones 1 and 2 are the magazine in every
//       frame of every recording, the depth pass's bone 0 the frame, its trigger (3) and cover (4) never the magazine; the palms at rest the row's;
//       the magazine straight down the well in its own reload (no rock, no slide along the gun while its top is in the well); the empty reload's
//       hand at the bolt catch; the kick of each of its eleven rounds as the recoil row has it
// Usage: mk11test [<folder with the mesh dumps> [<folder with the recordings>]] (build and build\survey by default, from the project's folder). Reads only.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "../src/gunbodies.h"
#include "../src/handpose.h"
#include "../src/recoilguns.h"
#include "../src/scopemarks.h"
#include "../src/shaperecipes.h"

namespace {

int g_checks = 0, g_failed = 0, g_skipped = 0;
void Check(bool ok, const char* what) {
    ++g_checks;
    g_failed += !ok;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
}
void Skip(const char* what) {
    ++g_skipped;
    std::printf("SKIP %s\n", what);
}

// ---- skin.cpp's rows for it, copied (kMk11Mag, kMk11Bones, kHeldKinds, kHeldEyes, kHeldSights) ----
constexpr int kBody[2] = {12456, 2214}, kDepth[2] = {14670, 14670}, kRounds[2] = {312, 1902}, kPiece[2] = {1902, 0};
constexpr int kFrameBone = 0, kTriggerBone = 1, kCoverBone = 2, kMagBone = 3;
constexpr int kDepthMag[2] = {1, 2}, kDepthTrigger = 3, kDepthCover = 4, kRoundsBone = 0, kPieceBone = 1;
constexpr float kTop[3] = {0.0f, -0.026f, 0.450f}, kGrab[3] = {0.0f, -0.090f, 0.450f}, kHook[3] = {0.0f, -0.054f, 0.492f};
constexpr float kMouth = -0.032f;
constexpr float kLever[3] = {0.0f, 0.051f, 0.274f}, kRelease[3] = {0.024f, 0.005f, 0.401f};
constexpr float kBoxMin[3] = {-0.015f, -0.189f, 0.023f}, kBoxMax[3] = {0.013f, -0.093f, 0.107f};
constexpr float kSights[3] = {0.0f, 0.0922f, 1.004f}, kMuzzle[3] = {0.0f, 0.026f, 1.004f}, kPort[3] = {-0.020f, 0.023f, 0.446f};
constexpr float kRightPalm[3] = {-0.054f, -0.114f, 0.254f}, kLeftPalm[3] = {0.012f, -0.041f, 0.612f};
constexpr float kEyeZ = 0.198f, kRearZ = 0.307f;
// The rifle scope's row.
constexpr int kScopeBody[2] = {12642, 156}, kScopeGlass[2] = {156, 0};
constexpr float kEyepiece[3] = {0.0f, 0.0370f, -0.1296f};
constexpr float kLensRadius = 0.019f, kPower = 6.84f;
// What the game said zoomed (build\survey\MK11_zoom.txt): the view in the scope's bone 0, its FOV, the first person's projection; unzoomed 55.
constexpr float kZoomedEye[3] = {0.0002f, 0.0369f, -0.2437f};
constexpr float kZoomedFov = 8.7f, kUnzoomedFov = 55.0f, kWeaponProjection = 7.596f;
// The line the marks are laid out from (tools\scopemarks.ps1's Eye): through the cross's own middle, as far back as the zoomed view.
constexpr float kMarksEye[3] = {0.0f, 0.0370f, -0.2437f};
// The scope's bone 0 on the gun's frame: in every frame of the recordings (k5); the draw scan's one view said (0.000 0.056 0.442) to its three decimals.
constexpr float kScopeOnFrame[3] = {0.0f, 0.0552f, 0.4417f};
// The gloves' draw and the palms' points in it (skin.cpp's kArmDraws[0]; kRightPalm on the right's root bone, kLeftPalm on the left's palm bone:
// tools/holdpic.cpp has them so), and each left finger's last bone (thumb ... little: handpose.cpp's kFinger and kDepth).
constexpr int kGloves[2] = {13755, 15051};
constexpr int kRightRoot = 37, kLeftPalmBone = 1;
constexpr float kRightPalmPoint[3] = {-0.505f, 1.129f, 0.150f}, kLeftPalmPoint[3] = {0.537f, 1.098f, 0.207f};
constexpr int kTipBone[5] = {2, 15, 22, 10, 3};

constexpr float kPi = 3.14159265f;

float Half(uint16_t h) {
    const int s = (h >> 15) & 1, e = (h >> 10) & 0x1F, m = h & 0x3FF;
    const float v = e == 0 ? std::ldexp(m / 1024.0f, -14) : std::ldexp(1 + m / 1024.0f, e - 15);
    return s ? -v : v;
}
std::vector<uint8_t> ReadAll(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
// 3x4, rows.
void Compose(const float* a, const float* b, float* out) {
    float r[12];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            r[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        r[i * 4 + 3] = a[i * 4] * b[3] + a[i * 4 + 1] * b[7] + a[i * 4 + 2] * b[11] + a[i * 4 + 3];
    }
    std::memcpy(out, r, sizeof(r));
}
void InvertRigid(const float* m, float* out) {
    float r[12];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            r[i * 4 + j] = m[j * 4 + i];
    for (int i = 0; i < 3; ++i)
        r[i * 4 + 3] = -(r[i * 4] * m[3] + r[i * 4 + 1] * m[7] + r[i * 4 + 2] * m[11]);
    std::memcpy(out, r, sizeof(r));
}
void Apply(const float* m, const float* p, float* o) {
    float r[3];
    for (int k = 0; k < 3; ++k)
        r[k] = m[k * 4] * p[0] + m[k * 4 + 1] * p[1] + m[k * 4 + 2] * p[2] + m[k * 4 + 3];
    std::memcpy(o, r, sizeof(r));
}
float Dist(const float* a, const float* b) {
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}
float Apart(const float* a, const float* b) {  // two bones' places
    return std::sqrt((a[3] - b[3]) * (a[3] - b[3]) + (a[7] - b[7]) * (a[7] - b[7]) + (a[11] - b[11]) * (a[11] - b[11]));
}

// A mesh dump's draw: each vertex once, posed by the dump's own bones into the space of bone `frame` (or left in its own vertices' space: frame -1),
// with the bone it goes with. Stride 48: a half position at 0, four bones at 8, four weights at 12; stride 32 (a sight's): rigid on bone 0.
struct Vertex {
    float at[3];    // posed
    float own[3];   // as stored
    int bone;
    int index;      // where the draw first names it
};
struct Dump {
    bool read = false;
    unsigned stride = 0, offset = 0;
    std::vector<uint8_t> vb, ib, bones;
    std::vector<Vertex> vertices;
    std::vector<int> order;  // the draw's indices as places in `vertices`
};
Dump Load(const std::string& dir, int count, int start, int frame) {
    Dump d;
    const std::string name = dir + "\\mesh_" + std::to_string(count) + "_" + std::to_string(start) + "_";
    d.vb = ReadAll(name + "vb0.bin"), d.ib = ReadAll(name + "ib.bin"), d.bones = ReadAll(name + "bones.bin");
    std::ifstream text(name + "dump.txt");
    std::string word;
    while (text >> word)
        if (word == "vb") {
            int slot = -1;
            std::string a, b;
            unsigned stride = 0, offset = 0;
            if (text >> slot >> a >> stride >> b >> offset && slot == 0)
                d.stride = stride, d.offset = offset;
            break;
        }
    if (d.vb.empty() || d.ib.size() < static_cast<size_t>(start + count) * 2 || d.bones.size() < 48 || (d.stride != 48 && d.stride != 32))
        return d;
    const auto* b = reinterpret_cast<const float*>(d.bones.data());
    const int boneCount = static_cast<int>(d.bones.size() / 48);
    float inverse[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    if (frame >= 0 && frame < boneCount)
        InvertRigid(b + frame * 12, inverse);
    std::vector<int> place(65536, -1);
    for (int i = start; i < start + count; ++i) {
        uint16_t v;
        std::memcpy(&v, &d.ib[2 * static_cast<size_t>(i)], 2);
        if (place[v] < 0) {
            const size_t o = d.offset + static_cast<size_t>(v) * d.stride;
            if (o + d.stride > d.vb.size())
                return d;
            uint16_t h[3];
            std::memcpy(h, &d.vb[o], 6);
            Vertex x{};
            x.own[0] = Half(h[0]), x.own[1] = Half(h[1]), x.own[2] = Half(h[2]);
            x.index = i;
            int bone = 0;
            if (d.stride == 48) {
                int most = -1;
                for (int s = 0; s < 4; ++s)
                    if (d.vb[o + 12 + s] > most)
                        most = d.vb[o + 12 + s], bone = d.vb[o + 8 + s];
            }
            if (bone >= boneCount)
                return d;
            x.bone = bone;
            if (frame >= 0) {
                float posed[3];
                Apply(b + bone * 12, x.own, posed);
                Apply(inverse, posed, x.at);
            } else {
                std::memcpy(x.at, x.own, sizeof(x.at));
            }
            place[v] = static_cast<int>(d.vertices.size());
            d.vertices.push_back(x);
        }
        d.order.push_back(place[v]);
    }
    d.read = true;
    return d;
}
struct Box {
    float lo[3] = {1e9f, 1e9f, 1e9f}, hi[3] = {-1e9f, -1e9f, -1e9f};
    int count = 0;
    void Add(const float* p) {
        for (int k = 0; k < 3; ++k)
            lo[k] = std::min(lo[k], p[k]), hi[k] = std::max(hi[k], p[k]);
        ++count;
    }
};

// A recording (weapon.txt "allrec"): records of 8 ints (the draw's count, the frame's serial, .., its start, the rounds loaded, .., ms, bones) and
// that many bones, in the view's space of the eye the draw was for.
struct Record {
    int count, start, ms, frame;
    std::vector<float> bones;
};
template <typename Fn>
bool Replay(const std::string& path, Fn&& each) {
    FILE* f = nullptr;
    if (fopen_s(&f, path.c_str(), "rb") || !f)
        return false;
    int head[8], first = -1;
    Record r;
    while (fread(head, 4, 8, f) == 8) {
        const int n = head[7];
        if (n <= 0 || n > 4096)
            break;
        r.bones.resize(static_cast<size_t>(n) * 12);
        if (fread(r.bones.data(), 48, n, f) != static_cast<size_t>(n))
            break;
        if (first < 0)
            first = head[6];
        r.count = head[0], r.start = head[3], r.ms = head[6] - first, r.frame = head[1];
        each(r);
    }
    fclose(f);
    return first >= 0;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "build", survey = argc > 2 ? argv[2] : "build\\survey";
    char what[900];

    // ---- k1: the tables ----
    {
        const recoilguns::Row* row = recoilguns::ByName("Weapons/MK11/Mk11");
        const bool known = row && row->body == static_cast<uint32_t>(kBody[0]) && row->shape && !std::strcmp(row->shape, "mk11") && recoilguns::ByBody(kBody[0]) == row &&
                           recoilguns::ByShape("mk11") == row;
        std::snprintf(what, sizeof(what), "k1 its recoil row: \"%s\", its body draw %u, its shape %s; found by its blueprint, its body and its shape alike", row ? row->name : "none",
                      row ? row->body : 0, row && row->shape ? row->shape : "none");
        Check(known, what);
        if (row) {
            std::snprintf(what, sizeof(what), "k1 ... a marksman's rifle, %d in its magazine (10 and the one in the chamber), %.0f m/s, %.0f a minute; its buzz %.1f %.1f for %.2f s",
                          row->magazine, row->speed, row->rate, row->rumble[0], row->rumble[1], row->rumble[2]);
            Check(row->kind == recoil::Kind::kMarksman && row->magazine == 11 && row->speed == 550.0f && row->rate == 260.0f && row->rumble[0] > 0.0f && row->rumble[2] > 0.0f, what);
            std::snprintf(what, sizeof(what), "k1 ... its kick its own recording's: %.2f cm back, a rise of %.2f degrees (with the drift's answer its recorded 2.19), no held fire's push (single fire: %.2f)",
                          row->kick.back.peak * 100.0f, row->kick.rise.peak, row->kick.heldShare);
            Check(std::fabs(row->kick.back.peak - 0.0343f) < 1e-5f && row->kick.rise.peak > 0.5f && row->kick.rise.peak < 2.19f && row->kick.heldShare == 0.0f, what);
            Check(&recoilguns::Default(recoil::Kind::kMarksman) == row, "k1 ... still the marksman rifles' own row for a gun known only by its kind");
            int same = 0;
            for (const recoilguns::Row& other : recoilguns::kRows)
                same += other.speed == 550.0f;
            std::snprintf(what, sizeof(what), "k1 ... 550 m/s is its alone among the %zu rows (%d): its bullets' start can't be another gun's", std::size(recoilguns::kRows), same);
            Check(same == 1, what);
        }
        const gunbodies::Body* body = gunbodies::For("mk11");
        std::snprintf(what, sizeof(what), "k1 its body for free hands and the holster: %s, %.1f kg, %d hull points, %d rounds of %.0f g", body && body->spec ? body->spec->name : "none",
                      body && body->spec ? body->spec->massEmpty : 0.0f, body && body->spec ? body->spec->hullPoints : 0, body && body->spec ? body->spec->capacity : 0,
                      body && body->spec ? body->spec->massRound * 1000.0f : 0.0f);
        Check(body && body->spec && !std::strcmp(body->spec->name, "mk11gun") && body->spec->keep && body->spec->hullPoints >= 4 && body->spec->capacity == 10, what);
        const gunbodies::Attach* scope = gunbodies::AttachFor(kScopeBody[0], kScopeBody[1], 0);
        std::snprintf(what, sizeof(what), "k1 its scope an attachment of the body (%d corner points)", scope ? scope->points : 0);
        Check(scope && scope->points == 8, what);
        const shaperecipes::Recipe* recipe = shaperecipes::Find("mk11");
        const std::string arguments = recipe ? shaperecipes::Arguments(*recipe) : "";
        std::snprintf(what, sizeof(what), "k1 its collision shape's recipe: gunshape ... 0.003 %s (its body from 2214, its frame bone 0, its magazine left out)", arguments.c_str());
        Check(recipe && arguments == "12456@2214 0 3" && recipe->draws[0].vbOffset == 24048 && recipe->drawCount == 1, what);
        const float numbers[] = {kTop[0], kTop[1], kTop[2], kGrab[1], kHook[2], kMouth, kLever[1], kLever[2], kRelease[0], kRelease[2], kSights[1], kMuzzle[1], kPort[0], kEyeZ, kRearZ,
                                 kEyepiece[1], kEyepiece[2], kLensRadius, kPower};
        Check(std::all_of(std::begin(numbers), std::end(numbers), [](float f) { return std::isfinite(f); }) && kMouth < 0.0f && kRearZ > kEyeZ, "k1 its rows' numbers finite, the rail's rear ahead of the eye");
    }

    // ---- k2: the mesh ----
    const Dump body = Load(dir, kBody[0], kBody[1], kFrameBone);
    if (!body.read) {
        Skip("k2 no dump of its body (build\\mesh_12456_2214_*): its mesh");
    } else {
        Box frame, mag, magOwn, trigger, cover;
        for (const Vertex& v : body.vertices) {
            (v.bone == kMagBone ? mag : v.bone == kTriggerBone ? trigger : v.bone == kCoverBone ? cover : frame).Add(v.at);
            if (v.bone == kMagBone)
                magOwn.Add(v.own);
        }
        std::snprintf(what, sizeof(what), "k2 its body: %d vertices on the frame (bone 0), %d the trigger's, %d the cover's, %d the magazine's; %.3f m long", frame.count, trigger.count,
                      cover.count, mag.count, frame.hi[2] - frame.lo[2]);
        Check(frame.count > 4000 && trigger.count > 50 && cover.count > 50 && mag.count > 100 && frame.hi[2] - frame.lo[2] > 0.95f && frame.hi[2] - frame.lo[2] < 1.1f, what);
        // The magazine: its box in its own vertices' space; its top, its middle below the well and its front corner on the frame.
        float boxOff = 0.0f;
        for (int k = 0; k < 3; ++k)
            boxOff = std::max({boxOff, std::fabs(magOwn.lo[k] - kBoxMin[k]), std::fabs(magOwn.hi[k] - kBoxMax[k])});
        // The receiver's underside over the well: the frame's lowest between the magazine's back and front, in the middle of its width.
        float under = 1e9f;
        for (const Vertex& v : body.vertices)
            if (v.bone == kFrameBone && v.at[2] > mag.lo[2] + 0.005f && v.at[2] < mag.hi[2] - 0.005f && std::fabs(v.at[0]) < 0.02f)
                under = std::min(under, v.at[1]);
        std::snprintf(what, sizeof(what),
                      "k2 its magazine: its box %.1f mm off the row's at most; on the frame y %.3f to %.3f, z %.3f to %.3f (the row's top (0 %.3f %.3f), its front corner z %.3f); the receiver's "
                      "underside over the well y %.3f: %.1f cm of it in the well (the row's mouth %.1f)",
                      boxOff * 1000.0f, mag.lo[1], mag.hi[1], mag.lo[2], mag.hi[2], kTop[1], kTop[2], kHook[2], under, (mag.hi[1] - under) * 100.0f, -kMouth * 100.0f);
        Check(boxOff < 0.001f && std::fabs(mag.hi[1] - kTop[1]) < 0.001f && std::fabs((mag.lo[2] + mag.hi[2]) / 2 - kTop[2]) < 0.001f && std::fabs(mag.hi[2] - kHook[2]) < 0.001f &&
                  std::fabs((under - mag.hi[1]) - kMouth) < 0.001f && std::fabs((under + mag.lo[1]) / 2 - kGrab[1]) < 0.001f && std::fabs(mag.lo[0] + 0.015f) < 0.001f &&
                  std::fabs(mag.hi[0] - 0.013f) < 0.001f,
              what);
        // A straight box: its back and front edges stand upright (no curve to rock about).
        float backTop = 1e9f, backBottom = 1e9f;
        for (const Vertex& v : body.vertices)
            if (v.bone == kMagBone) {
                if (v.at[1] > mag.hi[1] - 0.012f)
                    backTop = std::min(backTop, v.at[2]);
                if (v.at[1] < mag.lo[1] + 0.012f)
                    backBottom = std::min(backBottom, v.at[2]);
            }
        std::snprintf(what, sizeof(what), "k2 ... a straight box: its back edge at z %.3f at its top and %.3f at its bottom (straight in: no rock)", backTop, backBottom);
        Check(std::fabs(backTop - backBottom) < 0.003f, what);
        // The T handle's ears: the frame's widest at the top of the receiver's back end.
        Box ears[2];
        for (const Vertex& v : body.vertices)
            if (v.bone == kFrameBone && v.at[1] > 0.044f && v.at[1] < 0.062f && v.at[2] > 0.262f && v.at[2] < 0.285f && std::fabs(v.at[0]) > 0.0165f)
                ears[v.at[0] > 0.0f].Add(v.at);
        const bool haveEars = ears[0].count > 3 && ears[1].count > 3;
        const float earY = haveEars ? (std::min(ears[0].lo[1], ears[1].lo[1]) + std::max(ears[0].hi[1], ears[1].hi[1])) / 2 : 0.0f;
        const float earZ = haveEars ? (std::min(ears[0].lo[2], ears[1].lo[2]) + std::max(ears[0].hi[2], ears[1].hi[2])) / 2 : 0.0f;
        std::snprintf(what, sizeof(what), "k2 its T handle's ears: out to x %.3f and %.3f, their middle (0 %.3f %.3f); the row's lever (0 %.3f %.3f)", haveEars ? ears[0].lo[0] : 0.0f,
                      haveEars ? ears[1].hi[0] : 0.0f, earY, earZ, kLever[1], kLever[2]);
        Check(haveEars && ears[0].lo[0] < -0.018f && ears[1].hi[0] > 0.018f && std::fabs(earY - kLever[1]) < 0.003f && std::fabs(earZ - kLever[2]) < 0.003f, what);
        // The bolt catch: the paddle standing off the receiver's left side behind the magazine's back edge.
        Box paddle;
        for (const Vertex& v : body.vertices)
            if (v.bone == kFrameBone && v.at[0] > 0.0215f && v.at[2] > 0.385f && v.at[2] < 0.41f && v.at[1] > -0.03f && v.at[1] < 0.03f)
                paddle.Add(v.at);
        // ... and nothing of the receiver further out there within 3 cm (so a palm coming in from the left meets it first).
        float wider = 0.0f;
        for (const Vertex& v : body.vertices)
            if (v.bone == kFrameBone && std::fabs(v.at[1] - kRelease[1]) < 0.02f && std::fabs(v.at[2] - kRelease[2]) < 0.03f)
                wider = std::max(wider, v.at[0]);
        const float paddleMid[3] = {paddle.hi[0], (paddle.lo[1] + paddle.hi[1]) / 2, (paddle.lo[2] + paddle.hi[2]) / 2};
        std::snprintf(what, sizeof(what),
                      "k2 the bolt catch: a paddle on its left out to x %.3f, y %.3f to %.3f, z %.3f to %.3f (%d vertices), %.1f mm from the row's release (%.3f %.3f %.3f); the furthest left of the "
                      "receiver round it x %.3f",
                      paddle.hi[0], paddle.lo[1], paddle.hi[1], paddle.lo[2], paddle.hi[2], paddle.count, paddle.count ? Dist(paddleMid, kRelease) * 1000.0f : 0.0f, kRelease[0], kRelease[1],
                      kRelease[2], wider);
        Check(paddle.count >= 4 && Dist(paddleMid, kRelease) < 0.006f && wider <= paddle.hi[0] + 0.0005f && mag.lo[2] - kRelease[2] > 0.0f && mag.lo[2] - kRelease[2] < 0.02f, what);
        // Its muzzle, and its port under its cover on the right.
        Box tip;
        for (const Vertex& v : body.vertices)
            if (v.bone == kFrameBone && v.at[2] > frame.hi[2] - 0.001f)
                tip.Add(v.at);
        const float tipMid[3] = {(tip.lo[0] + tip.hi[0]) / 2, (tip.lo[1] + tip.hi[1]) / 2, tip.hi[2]};
        std::snprintf(what, sizeof(what), "k2 its muzzle's middle (%.3f %.3f %.3f), the row's (0 %.3f %.3f); its port's cover x %.3f to %.3f, y %.3f to %.3f, z %.3f to %.3f, the port (%.3f %.3f %.3f) at it",
                      tipMid[0], tipMid[1], tipMid[2], kMuzzle[1], kMuzzle[2], cover.lo[0], cover.hi[0], cover.lo[1], cover.hi[1], cover.lo[2], cover.hi[2], kPort[0], kPort[1], kPort[2]);
        Check(Dist(tipMid, kMuzzle) < 0.002f && std::fabs(kSights[2] - kMuzzle[2]) < 1e-6f && kPort[0] >= cover.lo[0] - 0.001f && kPort[0] <= cover.hi[0] + 0.001f &&
                  kPort[1] >= cover.lo[1] - 0.001f && kPort[1] <= cover.hi[1] + 0.001f && kPort[2] >= cover.lo[2] && kPort[2] <= cover.hi[2] && cover.hi[0] < 0.0f,
              what);
        // The piece of two bones and the rounds.
        const Dump piece = Load(dir, kPiece[0], kPiece[1], 0), rounds = Load(dir, kRounds[0], kRounds[1], -1);
        if (!piece.read || !rounds.read) {
            Skip("k2 no dump of its piece (mesh_1902_0_*) or its rounds (mesh_312_1902_*)");
        } else {
            Box plate, sleeve, sleeveOwn, round;
            int others = 0;
            for (const Vertex& v : piece.vertices) {
                if (v.bone == 0)
                    plate.Add(v.at);
                else if (v.bone == kPieceBone)
                    sleeve.Add(v.at), sleeveOwn.Add(v.own);
                else
                    ++others;
            }
            for (const Vertex& v : rounds.vertices)
                round.Add(v.own), others += v.bone != kRoundsBone;
            std::snprintf(what, sizeof(what),
                          "k2 its piece of two bones: %d vertices on its bone 0 (the frame: a plate at the stock's neck, z %.3f to %.3f) and %d on its bone 1 (the magazine: a sleeve round its "
                          "bottom, y %.3f to %.3f, z %.3f to %.3f on the frame), none on another; its rounds %d vertices on their bone 0, inside the magazine's box in its own space",
                          plate.count, plate.lo[2], plate.hi[2], sleeve.count, sleeve.lo[1], sleeve.hi[1], sleeve.lo[2], sleeve.hi[2], round.count);
            bool inside = round.count > 0;
            for (int k = 0; k < 3; ++k)
                inside = inside && round.lo[k] >= kBoxMin[k] - 0.002f && round.hi[k] <= kBoxMax[k] + 0.002f;
            Check(plate.count > 100 && sleeve.count > 50 && others == 0 && plate.hi[2] < 0.26f && sleeve.lo[1] >= mag.lo[1] - 0.002f && sleeve.hi[1] < under &&
                      sleeve.lo[2] > mag.lo[2] - 0.01f && sleeve.hi[2] < mag.hi[2] + 0.01f && inside,
                  what);
        }
    }

    // ---- k3: the scope on the gun ----
    const Dump glass = Load(dir, kScopeGlass[0], kScopeGlass[1], -1), scopeBody = Load(dir, kScopeBody[0], kScopeBody[1], -1);
    if (!glass.read || !scopeBody.read || !body.read) {
        Skip("k3 no dump of its scope (build\\mesh_12642_156_*, mesh_156_0_*): the scope on the gun");
    } else {
        // The glass's two lenses: the vertices at its back end (the eyepiece) and at its front (the objective).
        Box all;
        for (const Vertex& v : glass.vertices)
            all.Add(v.own);
        Box eyepiece, objective;
        for (const Vertex& v : glass.vertices)
            (v.own[2] < (all.lo[2] + all.hi[2]) / 2 ? eyepiece : objective).Add(v.own);
        const float middle[3] = {(eyepiece.lo[0] + eyepiece.hi[0]) / 2, (eyepiece.lo[1] + eyepiece.hi[1]) / 2, (eyepiece.lo[2] + eyepiece.hi[2]) / 2};
        const float radius = ((eyepiece.hi[0] - eyepiece.lo[0]) + (eyepiece.hi[1] - eyepiece.lo[1])) / 4;
        const float objectiveRadius = ((objective.hi[0] - objective.lo[0]) + (objective.hi[1] - objective.lo[1])) / 4;
        std::snprintf(what, sizeof(what),
                      "k3 its glass (156/0): the eyepiece %d vertices at z %.4f round (%.4f %.4f), %.1f mm in radius; the objective %d at z %.3f to %.3f, %.1f mm; the row's eyepiece (%.4f %.4f "
                      "%.4f), %.1f mm",
                      eyepiece.count, middle[2], middle[0], middle[1], radius * 1000.0f, objective.count, objective.lo[2], objective.hi[2], objectiveRadius * 1000.0f, kEyepiece[0], kEyepiece[1],
                      kEyepiece[2], kLensRadius * 1000.0f);
        Check(eyepiece.count >= 12 && objective.count >= 12 && eyepiece.hi[2] - eyepiece.lo[2] < 0.001f && Dist(middle, kEyepiece) < 0.0008f && std::fabs(radius - kLensRadius) < 0.0006f &&
                  objective.lo[2] > 0.1f && objectiveRadius > radius,
              what);
        // The zoomed eye on the eyepiece's line; the power.
        const float offLine = std::hypot(kZoomedEye[0] - kEyepiece[0], kZoomedEye[1] - kEyepiece[1]), relief = kEyepiece[2] - kZoomedEye[2];
        const float power = std::tan(kUnzoomedFov / 2 * kPi / 180.0f) / std::tan(kZoomedFov / 2 * kPi / 180.0f);
        std::snprintf(what, sizeof(what), "k3 the game's zoomed eye %.2f mm off the eyepiece's line, %.1f cm behind the glass; its zoom tan(%.2f) / tan(%.2f) = %.2f, the row's power %.2f", offLine * 1000.0f,
                      relief * 100.0f, kUnzoomedFov / 2, kZoomedFov / 2, power, kPower);
        Check(offLine < 0.0005f && relief > 0.08f && relief < 0.15f && std::fabs(power - kPower) < 0.01f, what);
        // The scope's bone 0 on the gun's frame (kScopeOnFrame: the draw scan's, held to the recordings in k5; the two dumps are of different moments,
        // the gun swaying between them), its axes the frame's: so the line and the eye on the frame.
        const float lensOnFrame[3] = {kScopeOnFrame[0] + kEyepiece[0], kScopeOnFrame[1] + kEyepiece[1], kScopeOnFrame[2] + kEyepiece[2]};
        const float eyeOnFrame[3] = {kScopeOnFrame[0] + kZoomedEye[0], kScopeOnFrame[1] + kZoomedEye[1], kScopeOnFrame[2] + kZoomedEye[2]};
        std::snprintf(what, sizeof(what),
                      "k3 with the scope's bone 0 on the frame at (%.4f %.4f %.4f): the eyepiece at (%.4f %.4f %.4f), the game's eye at (%.4f %.4f %.4f); the row's sights' line y %.4f, its eye "
                      "z %.3f, the rail's rear z %.3f",
                      kScopeOnFrame[0], kScopeOnFrame[1], kScopeOnFrame[2], lensOnFrame[0], lensOnFrame[1], lensOnFrame[2], eyeOnFrame[0], eyeOnFrame[1], eyeOnFrame[2], kSights[1], kEyeZ, kRearZ);
        Check(std::fabs(lensOnFrame[0]) < 0.001f && std::fabs(lensOnFrame[1] - kSights[1]) < 0.001f && std::fabs(eyeOnFrame[1] - kSights[1]) < 0.001f &&
                  std::fabs(eyeOnFrame[2] - kEyeZ) < 0.002f && lensOnFrame[2] > kRearZ && lensOnFrame[2] - kRearZ < 0.02f && kSights[0] == 0.0f,
              what);
        // The scope's body round the glass: the eyepiece is inside its tube (so the body gets a window), and nothing of the gun's own stands on the
        // line between the eye and the glass.
        Box tube;
        for (const Vertex& v : scopeBody.vertices)
            tube.Add(v.own);
        int onLine = 0;
        for (const Vertex& v : body.vertices)
            if (v.at[2] > eyeOnFrame[2] && v.at[2] < lensOnFrame[2] && std::hypot(v.at[0] - lensOnFrame[0], v.at[1] - lensOnFrame[1]) < kLensRadius)
                ++onLine;
        std::snprintf(what, sizeof(what), "k3 the scope's tube z %.3f to %.3f in its bone: the eyepiece %.1f mm inside its back end; %d vertices of the gun between the eye and the glass within the lens's radius",
                      tube.lo[2], tube.hi[2], (kEyepiece[2] - tube.lo[2]) * 1000.0f, onLine);
        Check(tube.lo[2] < kEyepiece[2] && kEyepiece[2] - tube.lo[2] < 0.03f && tube.hi[2] > objective.lo[2] && onLine == 0, what);
        // The lens in the picture: scopeview draws its own picture once the lens is 80 pixels in radius. An eye's picture 2160 high at the wanted 105.9
        // degrees (the owner's: build\survey\MK11_unzoomed.txt "wanted 105.9 degrees"): 815 pixels to a unit of slope.
        const float focal = 1080.0f / std::tan(105.9f / 2 * kPi / 180.0f);
        const float atEye = kLensRadius / relief * focal, farthest = kLensRadius * focal / 80.0f;
        std::snprintf(what, sizeof(what), "k3 the lens at the game's eye: %.0f pixels in radius in a 2160-high eye at 105.9 degrees (80 and more: the second renderer's own picture); 80 pixels still at %.0f cm behind the glass",
                      atEye, farthest * 100.0f);
        Check(atEye >= 80.0f * 1.25f && farthest > relief, what);
    }

    // ---- k4: the reticle ----
    {
        const scopemarks::Marks& marks = scopemarks::kRifleScope;
        const Dump dark = Load(dir, 480, 0, -1), bright = Load(dir, 558, 480, -1);
        const float scale = kWeaponProjection * std::tan(kZoomedFov / 2 * kPi / 180.0f);  // the weapon's projection over the world's
        std::snprintf(what, sizeof(what), "k4 kRifleScope: %d dark, %d bright, black, reaching %.4f of slope", marks.darkCount, marks.brightCount, marks.reach);
        Check(marks.darkCount == 480 && marks.brightCount == 558 && marks.dark && marks.bright && marks.color[0] == 0.0f && marks.color[1] == 0.0f && marks.color[2] == 0.0f &&
                  marks.reach > 0.05f && marks.reach < 0.09f,
              what);
        if (!dark.read || !bright.read || dark.stride != 32 || bright.stride != 32) {
            Skip("k4 no dump of its zoomed marks (build\\mesh_480_0_*, mesh_558_480_*): the table against them");
        } else {
            // Made again: each index's vertex, its slope from the zoomed eye, its place on the sheet (two halves 28 bytes in).
            float worst = 0.0f, worstUv = 0.0f, reach = 0.0f;
            const auto again = [&](const Dump& d, const scopemarks::Vertex* table, int count) {
                for (int i = 0; i < count && i < static_cast<int>(d.order.size()); ++i) {
                    const Vertex& v = d.vertices[d.order[i]];
                    const float depth = v.own[2] - kMarksEye[2];
                    const float x = (v.own[0] - kMarksEye[0]) / depth * scale, y = (v.own[1] - kMarksEye[1]) / depth * scale;
                    worst = std::max({worst, std::fabs(x - table[i].x), std::fabs(y - table[i].y)});
                    reach = std::max({reach, std::fabs(x), std::fabs(y)});
                    // (the vertex's own bytes again for its place on the sheet)
                    uint16_t index;
                    std::memcpy(&index, &d.ib[2 * (static_cast<size_t>(&d == &dark ? 0 : 480) + i)], 2);
                    uint16_t uv[2];
                    std::memcpy(uv, &d.vb[d.offset + static_cast<size_t>(index) * d.stride + 28], 4);
                    worstUv = std::max({worstUv, std::fabs(Half(uv[0]) - table[i].u), std::fabs(Half(uv[1]) - table[i].v)});
                }
            };
            again(dark, marks.dark, marks.darkCount);
            again(bright, marks.bright, marks.brightCount);
            std::snprintf(what, sizeof(what), "k4 ... made again from the dumps (the eye (%.4f %.4f %.4f), the weapon's projection %.3f over the world's 1/tan(%.2f)): %.5f of slope off at most, %.5f on the sheet; reaching %.4f",
                          kMarksEye[0], kMarksEye[1], kMarksEye[2], kWeaponProjection, kZoomedFov / 2, worst, worstUv, reach);
            Check(worst < 1e-4f && worstUv < 1e-3f && std::fabs(reach - marks.reach) < 2e-4f, what);
        }
        // The cross: every bright triangle lies along one of the two axes through the line's middle. Along each arm: the thin line, then dots (short
        // stretches where it's thicker), then a thick bar to the edge.
        float lo[2] = {1e9f, 1e9f}, hi[2] = {-1e9f, -1e9f};
        struct Stretch {
            float from, to;
        };
        std::vector<Stretch> thick[4];  // along +x, -x, +y, -y: where a triangle stands more than half a milliradian off the axis
        int offAxes = 0;
        for (int i = 0; i + 2 < marks.brightCount; i += 3) {
            float tLo[2] = {1e9f, 1e9f}, tHi[2] = {-1e9f, -1e9f};
            for (int k = 0; k < 3; ++k) {
                const float p[2] = {marks.bright[i + k].x, marks.bright[i + k].y};
                for (int a = 0; a < 2; ++a)
                    tLo[a] = std::min(tLo[a], p[a]), tHi[a] = std::max(tHi[a], p[a]), lo[a] = std::min(lo[a], p[a]), hi[a] = std::max(hi[a], p[a]);
            }
            const float cx = (tLo[0] + tHi[0]) / 2, cy = (tLo[1] + tHi[1]) / 2;
            const bool alongX = std::fabs(cy) < 0.0012f && std::fabs(cx) > 0.0012f, alongY = std::fabs(cx) < 0.0012f && std::fabs(cy) > 0.0012f;
            if (!alongX && !alongY) {
                offAxes += std::fabs(cx) > 0.0012f || std::fabs(cy) > 0.0012f;  // (the middle's own piece is on both)
                continue;
            }
            const int axis = alongX ? 0 : 1, across = 1 - axis, arm = axis * 2 + ((alongX ? cx : cy) > 0 ? 0 : 1);
            if (std::max(std::fabs(tLo[across]), std::fabs(tHi[across])) > 0.0005f)
                thick[arm].push_back({std::min(std::fabs(tLo[axis]), std::fabs(tHi[axis])), std::max(std::fabs(tLo[axis]), std::fabs(tHi[axis]))});
        }
        std::vector<float> dots[4];  // each dot's distance from the middle
        float barFrom[4] = {1e9f, 1e9f, 1e9f, 1e9f}, barTo[4] = {};
        for (int arm = 0; arm < 4; ++arm) {
            std::sort(thick[arm].begin(), thick[arm].end(), [](const Stretch& a, const Stretch& b) { return a.from < b.from; });
            for (size_t i = 0; i < thick[arm].size();) {
                Stretch s = thick[arm][i++];
                while (i < thick[arm].size() && thick[arm][i].from < s.to + 0.0005f)
                    s.to = std::max(s.to, thick[arm][i++].to);
                if (s.to - s.from < 0.003f)
                    dots[arm].push_back((s.from + s.to) / 2);
                else
                    barFrom[arm] = std::min(barFrom[arm], s.from), barTo[arm] = std::max(barTo[arm], s.to);
            }
        }
        bool even = true, alike = true;
        float spacing = 0.0f;
        for (const auto& arm : dots) {
            if (arm.size() != dots[0].size() || arm.size() < 2) {
                alike = false;
                continue;
            }
            for (size_t i = 0; i < arm.size(); ++i) {
                alike = alike && std::fabs(arm[i] - dots[0][i]) < 0.0012f;
                if (i)
                    even = even && std::fabs((arm[i] - arm[i - 1]) - (arm[1] - arm[0])) < 0.0004f;
            }
            spacing = arm[1] - arm[0];
        }
        const float shows = 0.30f / kPower;  // scopezoom's lens goes dark from 0.30 of apparent slope
        std::snprintf(what, sizeof(what),
                      "k4 the cross: %d triangles, all on its two axes (%d off them), x %.4f to %.4f, y %.4f to %.4f (centred); %zu, %zu, %zu, %zu dots on its four arms, %.4f of slope apart "
                      "(%.1f mrad), the same on each; thick bars from %.4f, %.4f, %.4f, %.4f out to the edge (the lens shows to %.4f)",
                      marks.brightCount / 3, offAxes, lo[0], hi[0], lo[1], hi[1], dots[0].size(), dots[1].size(), dots[2].size(), dots[3].size(), spacing, spacing * 1000.0f, barFrom[0],
                      barFrom[1], barFrom[2], barFrom[3], shows);
        Check(marks.brightCount == 558 && offAxes == 0 && std::fabs(lo[0] + hi[0]) < 0.002f && std::fabs(lo[1] + hi[1]) < 0.002f && alike && even && dots[0].size() == 4 &&
                  *std::max_element(barFrom, barFrom + 4) < shows && *std::min_element(barFrom, barFrom + 4) > dots[0].back() && *std::min_element(barTo, barTo + 4) > 0.07f,
              what);
    }

    // ---- k5: the recordings ----
    for (const char* which : {"partial", "empty", "dry"}) {
        const std::string path = survey + "\\MK11_" + which + ".bin";
        long bodies = 0, depths = 0, roundsSeen = 0, pieces = 0, strays = 0, scopes = 0;
        float worstDepthMag = 0.0f, worstDepthTrigger = 0.0f, worstDepthCover = 0.0f, worstRounds = 0.0f, worstPiece = 0.0f, worstEyes = 0.0f;
        float magOut = 0.0f;                    // the furthest the magazine was from its seat (m)
        float seat[12];                         // its seat on the frame (the first frame's)
        bool haveSeat = false, comingBack = false;
        float outSide = 0.0f, outTurn = 0.0f;   // on its way out, while its top is still in the well: the furthest it slid across or along the gun, and turned
        float inTurn = 0.0f;                    // on its way back in: the most it's turned within 7 cm of home
        float coverMoved = 0.0f, cover0[12];
        bool haveCover = false;
        float palms[2][3] = {};
        bool havePalms = false;
        // The scope's bone 0 on the frame: where, and how far it ever is from where it first was (rigid on the gun).
        float scopeOn[12] = {}, scopeSpread = 0.0f, scopeTurned = 0.0f;
        bool haveScope = false;
        // The kick: the frame in the view (each frame's first body draw: one eye's) against where it rested before each round (a step of more than 0.8
        // degrees up in one frame begins one).
        struct Shot {
            int ms;
            float back, rise;
            int backMs, riseMs;
        };
        std::vector<Shot> shots;
        float rest[12], last[12];
        bool haveLast = false, inShot = false;
        // The hand at the bolt catch: the left palm's and each left fingertip's nearest to the release, and when.
        float nearestCatch = 1e9f;
        int nearestCatchMs = -1;
        float nearestTip[5] = {1e9f, 1e9f, 1e9f, 1e9f, 1e9f}, nearestTipSide[5] = {};
        int nearestTipMs[5] = {-1, -1, -1, -1, -1};
        float joints[5][4][3];
        handpose::Joints(joints);

        // One frame's draws (each eye's): the body's frame and its bones on it, then every other draw of the gun against them.
        const auto frameOf = [&](const std::vector<Record>& group) {
            const Record* first = nullptr;
            std::vector<const Record*> eyes;
            for (const Record& r : group)
                if (r.count == kBody[0] && r.start == kBody[1] && static_cast<int>(r.bones.size() / 12) > kMagBone)
                    eyes.push_back(&r);
            if (eyes.empty())
                return;
            first = eyes.front();
            bodies += static_cast<long>(eyes.size());
            const float* frame = &first->bones[kFrameBone * 12];
            float inverse[12], magOn[12], triggerOn[12], coverOn[12];
            InvertRigid(frame, inverse);
            Compose(inverse, &first->bones[kMagBone * 12], magOn);
            Compose(inverse, &first->bones[kTriggerBone * 12], triggerOn);
            Compose(inverse, &first->bones[kCoverBone * 12], coverOn);
            // (The other eye's draw: the same bones on its own frame.)
            for (const Record* other : eyes) {
                float otherInverse[12], on[12];
                InvertRigid(&other->bones[kFrameBone * 12], otherInverse);
                Compose(otherInverse, &other->bones[kMagBone * 12], on);
                worstEyes = std::max(worstEyes, Apart(on, magOn));
            }
            if (!haveSeat)
                std::memcpy(seat, magOn, sizeof(seat)), haveSeat = true;
            if (!haveCover)
                std::memcpy(cover0, coverOn, sizeof(cover0)), haveCover = true;
            coverMoved = std::max(coverMoved, Apart(coverOn, cover0));
            const float d[3] = {magOn[3] - seat[3], magOn[7] - seat[7], magOn[11] - seat[11]};
            const float away = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
            magOut = std::max(magOut, away);
            comingBack = comingBack || (magOut > 0.2f && away < magOut - 0.05f);
            const float turn = std::acos(std::clamp(magOn[1] * seat[1] + magOn[5] * seat[5] + magOn[9] * seat[9], -1.0f, 1.0f)) * 180.0f / kPi;
            if (!comingBack && d[1] < -0.002f && d[1] > kMouth)
                outSide = std::max(outSide, std::hypot(d[0], d[2])), outTurn = std::max(outTurn, turn);
            if (comingBack && away < 0.07f)
                inTurn = std::max(inTurn, turn);
            // The kick.
            if (haveLast) {
                float lastInverse[12], moved[12];
                InvertRigid(inShot ? rest : last, lastInverse);
                Compose(lastInverse, frame, moved);  // the frame now in the frame it rested in
                const float pitch = std::atan2(moved[6], moved[10]) * 180.0f / kPi;  // its z's rise
                if (!inShot && pitch > 0.8f) {
                    inShot = true;
                    std::memcpy(rest, last, sizeof(rest));
                    shots.push_back({first->ms, 0.0f, 0.0f, 0, 0});
                } else if (inShot && first->ms - shots.back().ms > 600) {
                    inShot = false;
                }
                if (inShot) {
                    Shot& s = shots.back();
                    // (Against the frame at rest, not the one before.)
                    InvertRigid(rest, lastInverse);
                    Compose(lastInverse, frame, moved);
                    const float rise = std::atan2(moved[6], moved[10]) * 180.0f / kPi;
                    if (-moved[11] > s.back)
                        s.back = -moved[11], s.backMs = first->ms - s.ms;
                    if (rise > s.rise)
                        s.rise = rise, s.riseMs = first->ms - s.ms;
                }
            }
            std::memcpy(last, frame, sizeof(last));
            haveLast = true;
            // The frame of the body draw of the same eye as another draw: the one that draw's own bone is nearest (the eyes' views are 6 cm apart).
            const auto eyeOf = [&](const float* bone, const float* expected) {
                const Record* best = eyes.front();
                float least = 1e9f;
                for (const Record* e : eyes) {
                    float inv[12], on[12];
                    InvertRigid(&e->bones[kFrameBone * 12], inv);
                    Compose(inv, bone, on);
                    const float at[3] = {on[3], on[7], on[11]};
                    if (const float off = Dist(at, expected); off < least)
                        least = off, best = e;
                }
                return best;
            };
            for (const Record& r : group) {
                const int n = static_cast<int>(r.bones.size() / 12);
                if (r.count == kDepth[0] && r.start == kDepth[1] && n > kDepthCover) {
                    // The depth pass: its own bone 0 its frame; the others on it as the body's are on the body's frame.
                    ++depths;
                    float inv[12], on[12];
                    InvertRigid(&r.bones[0], inv);
                    for (int k = 0; k < 2; ++k) {
                        Compose(inv, &r.bones[kDepthMag[k] * 12], on);
                        worstDepthMag = std::max(worstDepthMag, Apart(on, magOn));
                    }
                    Compose(inv, &r.bones[kDepthTrigger * 12], on);
                    worstDepthTrigger = std::max(worstDepthTrigger, Apart(on, triggerOn));
                    Compose(inv, &r.bones[kDepthCover * 12], on);
                    worstDepthCover = std::max(worstDepthCover, Apart(on, coverOn));
                } else if (r.count == kRounds[0] && r.start == kRounds[1] && n > kRoundsBone) {
                    // The rounds: placed by the body's frame of the same eye (the draw has no frame bone of its own).
                    const float magAt[3] = {magOn[3], magOn[7], magOn[11]};
                    const Record* eye = eyeOf(&r.bones[kRoundsBone * 12], magAt);
                    float inv[12], on[12];
                    InvertRigid(&eye->bones[kFrameBone * 12], inv);
                    Compose(inv, &r.bones[kRoundsBone * 12], on);
                    if (Apart(on, magOn) > 0.05f) {
                        ++strays;  // (another draw of as many indices: not this gun's)
                    } else {
                        ++roundsSeen;
                        worstRounds = std::max(worstRounds, Apart(on, magOn));
                    }
                } else if (r.count == kPiece[0] && r.start == kPiece[1] && n > kPieceBone) {
                    // The piece: its bone 0 the frame's, its bone 1 on it as the magazine is. (A draw of the HUD's text can have as many indices: its
                    // "bones" are nowhere near a rigid pair, and the mod never takes it: not the player's own palette, not the first person's stencil.)
                    float inv[12], on[12];
                    InvertRigid(&r.bones[0], inv);
                    Compose(inv, &r.bones[kPieceBone * 12], on);
                    const float zero[3] = {};
                    const Record* eye = eyeOf(&r.bones[0], zero);
                    if (Apart(&r.bones[0], &eye->bones[kFrameBone * 12]) > 0.001f) {
                        ++strays;
                    } else {
                        ++pieces;
                        worstPiece = std::max(worstPiece, Apart(on, magOn));
                    }
                } else if (r.count == kScopeBody[0] && r.start == kScopeBody[1] && n > 0) {
                    const Record* eye = eyeOf(&r.bones[0], kScopeOnFrame);
                    float inv[12], on[12];
                    InvertRigid(&eye->bones[kFrameBone * 12], inv);
                    Compose(inv, &r.bones[0], on);
                    ++scopes;
                    if (!haveScope)
                        std::memcpy(scopeOn, on, sizeof(scopeOn)), haveScope = true;
                    scopeSpread = std::max(scopeSpread, Apart(on, scopeOn));
                    for (int i = 0; i < 3; ++i)
                        for (int j = 0; j < 3; ++j)
                            scopeTurned = std::max(scopeTurned, std::fabs(on[i * 4 + j] - (i == j ? 1.0f : 0.0f)));
                } else if (r.count == kGloves[0] && r.start == kGloves[1] && n > kRightRoot) {
                    // The gloves: a palm on the gun is skin.cpp's kRightPalm on the right's root bone, kLeftPalm on the left's palm bone (the row's palms
                    // are these at rest); a fingertip the end of its last bone. Their eye: the one that has the right palm at the grip, where it stays.
                    float palmBone[12], palmAt[3];
                    std::memcpy(palmBone, &r.bones[kRightRoot * 12], sizeof(palmBone));
                    Apply(palmBone, kRightPalmPoint, palmAt);
                    palmBone[3] = palmAt[0], palmBone[7] = palmAt[1], palmBone[11] = palmAt[2];
                    const Record* eye = eyeOf(palmBone, kRightPalm);
                    float inv[12], on[12], at[2][3];
                    InvertRigid(&eye->bones[kFrameBone * 12], inv);
                    Compose(inv, &r.bones[kRightRoot * 12], on);
                    Apply(on, kRightPalmPoint, at[0]);
                    Compose(inv, &r.bones[kLeftPalmBone * 12], on);
                    Apply(on, kLeftPalmPoint, at[1]);
                    if (!havePalms)
                        std::memcpy(palms, at, sizeof(palms)), havePalms = true;
                    if (magOut > 0.2f) {  // after the magazine's been out: the end of the empty reload
                        if (const float off = Dist(at[1], kRelease); off < nearestCatch)
                            nearestCatch = off, nearestCatchMs = r.ms;
                        for (int finger = 0; finger < 5; ++finger) {
                            float tip[3];
                            Compose(inv, &r.bones[kTipBone[finger] * 12], on);
                            Apply(on, joints[finger][3], tip);
                            if (const float off = Dist(tip, kRelease); off < nearestTip[finger])
                                nearestTip[finger] = off, nearestTipMs[finger] = r.ms, nearestTipSide[finger] = tip[0];
                        }
                    }
                }
            }
        };
        std::vector<Record> group;
        const bool read = Replay(path, [&](const Record& r) {
            if (!group.empty() && group.front().frame != r.frame) {
                frameOf(group);
                group.clear();
            }
            group.push_back(r);
        });
        if (!group.empty())
            frameOf(group);
        if (!read || !bodies) {
            std::snprintf(what, sizeof(what), "k5 %s: no recording", path.c_str());
            Skip(what);
            continue;
        }
        std::snprintf(what, sizeof(what),
                      "k5 MK11_%s: %ld body draws, %ld depth, %ld rounds, %ld of the piece (%ld other draws of as many indices): on its own frame the depth pass's bones 1 and 2 are the magazine "
                      "(%.2f mm), its 3 the trigger (%.2f), its 4 the cover (%.2f); the rounds' bone 0 is the magazine (%.2f mm); the piece's bone 1, on its bone 0, the magazine (%.2f); the two "
                      "eyes' draws alike (%.2f mm); the magazine out to %.0f cm",
                      which, bodies, depths, roundsSeen, pieces, strays, worstDepthMag * 1000.0f, worstDepthTrigger * 1000.0f, worstDepthCover * 1000.0f, worstRounds * 1000.0f,
                      worstPiece * 1000.0f, worstEyes * 1000.0f, magOut * 100.0f);
        Check(depths > 100 && roundsSeen > 100 && pieces > 100 && strays * 50 < pieces && worstDepthMag < 0.0005f && worstDepthTrigger < 0.0005f && worstDepthCover < 0.0005f &&
                  worstRounds < 0.0005f && worstPiece < 0.0005f && worstEyes < 0.0005f && magOut > 0.3f,
              what);
        std::snprintf(what, sizeof(what),
                      "k5 ... its own reload: the magazine straight down the well (%.1f mm aside and %.1f degrees turned at most while its top is in it), the fresh one up with a lean of %.0f "
                      "degrees at most on its way (the M4A1's own reload: 13); its port's cover never moves (%.2f mm); the palms at rest right (%.3f %.3f %.3f) left (%.3f %.3f %.3f), the row's to "
                      "%.1f mm",
                      outSide * 1000.0f, outTurn, inTurn, coverMoved * 1000.0f, palms[0][0], palms[0][1], palms[0][2], palms[1][0], palms[1][1], palms[1][2],
                      std::max(Dist(palms[0], kRightPalm), Dist(palms[1], kLeftPalm)) * 1000.0f);
        Check(outSide < 0.002f && outTurn < 1.5f && inTurn < 16.0f && coverMoved < 0.0005f && havePalms && Dist(palms[0], kRightPalm) < 0.003f && Dist(palms[1], kLeftPalm) < 0.003f, what);
        // The scope on the gun: rigid, its axes the frame's; so the eyepiece and the game's zoomed eye on the frame.
        float lensOnFrame[3] = {}, eyeOnFrame[3] = {};
        if (haveScope) {
            Apply(scopeOn, kEyepiece, lensOnFrame);
            Apply(scopeOn, kZoomedEye, eyeOnFrame);
        }
        std::snprintf(what, sizeof(what),
                      "k5 ... its scope's bone 0 on the frame at (%.4f %.4f %.4f) in %ld draws, never %.2f mm from there, its axes the frame's (%.4f off at most): the eyepiece at (%.4f %.4f "
                      "%.4f), the game's zoomed eye at (%.4f %.4f %.4f); the row's sights' line y %.4f, its eye z %.3f, the rail's rear z %.3f",
                      scopeOn[3], scopeOn[7], scopeOn[11], scopes, scopeSpread * 1000.0f, scopeTurned, lensOnFrame[0], lensOnFrame[1], lensOnFrame[2], eyeOnFrame[0], eyeOnFrame[1],
                      eyeOnFrame[2], kSights[1], kEyeZ, kRearZ);
        const float scopeAt[3] = {scopeOn[3], scopeOn[7], scopeOn[11]};
        Check(haveScope && scopes > 100 && scopeSpread < 0.0005f && scopeTurned < 0.002f && Dist(scopeAt, kScopeOnFrame) < 0.001f && std::fabs(lensOnFrame[0]) < 0.001f && std::fabs(lensOnFrame[1] - kSights[1]) < 0.001f &&
                  std::fabs(eyeOnFrame[1] - kSights[1]) < 0.001f && std::fabs(eyeOnFrame[2] - kEyeZ) < 0.002f && lensOnFrame[2] > kRearZ && lensOnFrame[2] - kRearZ < 0.02f && kSights[0] == 0.0f,
              what);
        if (!std::strcmp(which, "partial"))
            continue;
        // The empty reload (the dry recording ends with one too): the hand goes to the catch on the left of the receiver, and a fingertip to the catch itself.
        static const char* const kFingerNames[5] = {"thumb", "index", "middle", "ring", "little"};
        int nearest = 0;
        for (int finger = 1; finger < 5; ++finger)
            if (nearestTip[finger] < nearestTip[nearest])
                nearest = finger;
        std::snprintf(what, sizeof(what),
                      "k5 ... the empty reload ends at the bolt catch: the left palm's middle comes to %.1f cm of the row's release (at %d ms), and the %s's tip to %.1f cm of it (at %d ms, at x %.3f: "
                      "on the gun's left); the other tips %.1f, %.1f, %.1f, %.1f cm at their nearest: a press of the thumb, the hand under the receiver",
                      nearestCatch * 100.0f, nearestCatchMs, kFingerNames[nearest], nearestTip[nearest] * 100.0f, nearestTipMs[nearest], nearestTipSide[nearest],
                      nearestTip[(nearest + 1) % 5] * 100.0f, nearestTip[(nearest + 2) % 5] * 100.0f, nearestTip[(nearest + 3) % 5] * 100.0f, nearestTip[(nearest + 4) % 5] * 100.0f);
        Check(nearestCatch < 0.16f && nearestCatchMs > 0 && nearest == 0 && nearestTip[nearest] < 0.035f && nearestTipSide[nearest] > 0.0f, what);
        if (std::strcmp(which, "dry"))
            continue;
        // Its rounds, fired one by one until it ran dry (anything else found is the reload's own move: not 3 to 4 cm back within 150 ms).
        const recoilguns::Row* row = recoilguns::ByBody(kBody[0]);
        int fired = 0;
        float backLo = 1e9f, backHi = 0.0f, riseLo = 1e9f, riseHi = 0.0f;
        for (const Shot& s : shots) {
            if (s.back < 0.03f || s.back > 0.04f || s.backMs > 150)
                continue;
            ++fired;
            backLo = std::min(backLo, s.back), backHi = std::max(backHi, s.back);
            riseLo = std::min(riseLo, s.rise), riseHi = std::max(riseHi, s.rise);
        }
        const Shot first = shots.empty() ? Shot{} : shots.front();
        std::snprintf(what, sizeof(what),
                      "k5 ... its kick: %d rounds (10 and the one in the chamber), the first %.2f cm back at %d ms and %.2f degrees up at %d ms; all of them %.2f to %.2f cm back, %.2f to %.2f "
                      "degrees up; the row's back %.2f cm, its recorded rise 2.19",
                      fired, first.back * 100.0f, first.backMs, first.rise, first.riseMs, backLo * 100.0f, backHi * 100.0f, riseLo, riseHi, row ? row->kick.back.peak * 100.0f : 0.0f);
        Check(fired == 11 && row && std::fabs(first.back - row->kick.back.peak) < 0.0005f && std::fabs(first.rise - 2.19f) < 0.05f && backHi - backLo < 0.002f && riseLo > 1.5f &&
                  riseHi < 2.4f && first.backMs >= 60 && first.backMs <= 125 && first.riseMs >= 100 && first.riseMs <= 175,
              what);
    }

    std::printf("%d checks, %d failed, %d skipped\n", g_checks, g_failed, g_skipped);
    return g_failed;
}

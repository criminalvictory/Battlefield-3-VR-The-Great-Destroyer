// The bridge M249's ACOG (Operation Swordbreaker's overpass), held to what was measured, without the game: src/acog.h (skin.cpp builds its row of kHeldSights
// from it), m249rows::kSightMount (the feed cover's mount, which lidride::OnMount rides), scopemarks::kAcog (its marks), gunbodies' kAttach row (its corners
// on a dropped gun), aimrules::HeldShotLine (its bullets' line), against the dumps and the recordings of 2026-11-19 (build\survey\M249ACOG_*: its six draws
// unzoomed, its zoomed model, the game's own reload with it on), the 11-05 dumps scopemarks::kAcog was made from (build\mesh_516_6957_*, mesh_12_0_*,
// mesh_6705_12_*) and the bare M249's recordings (build\survey\M249_partial.bin, M249_empty.bin).
//   a1  the tables: its draws the M416's ACOG's; its glass, radius and power; its marks kAcog (516 dark, 12 bright, red); its corners an attachment; the
//       M249's cover carrying what's on it, its mount the row's; no other gun's cover with a mount; every number finite
//   a2  its meshes (SKIP without the dumps): its body's buffers the M416's ACOG's byte for byte, its glass, chevron and fibre in the same buffers; the glass a
//       flat disc where the row has its eyepiece, of its radius; the game's own chevron on that line; the fibre on top; the eyepiece inside the body's back end;
//       its attachment's points on its body
//   a3  its zoomed model and marks (SKIP without the dumps): today's zoomed dumps the 11-05 ones byte for byte; kAcog made again from them as
//       tools\scopemarks.ps1 makes it, to 1e-4 of slope; the power the game's zoom; on the M249 the game's own zoomed eye (the survey's) sees the chevron's
//       tip within 0.8 mrad of where kAcog puts it, the marks at most 4% larger
//   a4  the mount (SKIP without the recording): through the game's own reload (the cover up 102.6 degrees and shut) each of its six draws' bone 0 on the
//       cover's bone at the mount, the cover's axes, in every frame: lidride::OnMount takes every one, never the cover's own bone nor another of the gun's;
//       at rest on the frame where the row says; the cover lifted by lidride::Turn takes the mount where the game's own reload has the sight at its most open
//   a5  the bare M249 (SKIP without its recordings): no draw of its two recordings on the mount in any frame (with the cover up by hand every draw as before)
//   a6  its line: the lens along the gun's sights' line (0.78 degrees), the game's own zoomed view and aim through it within 0.2 degrees of that (0.59 to
//       0.66: in the hand the gun keeps its irons' raise, the ACOG's picture, marks and bullets all along it); its bullets' start
//       through its glass on that line; the chevron's tip over it within 1.5 mrad, as on the M416 (where the owner's rounds landed on the tip); the irons'
//       line under it; the lens at the game's zoomed eye and where the second renderer's picture comes on
//   a7  lifted by hand, each draw framed as DrawHeldPart frames it (SKIP without the recording; the review of dd042df: a4 pairs a draw with whichever
//       body's cover puts it nearest the mount, so it couldn't see this): the frame of the last body or depth pass drawn before it, the last body's cover;
//       its glass, chevron and fibre drawn after the see-through pass's own first-person view, never the body's, tried by lidride::Tried as a held sight's
//       own (by the view alone, none of them: they stayed shut while the body went up) and on the mount in every record; with the game's cover at rest
//       (whenever a hand lifts it) all six tried and on the mount; no other draw after the shadow with a bone on the mount tried
// Usage: m249acogtest [<folder with the mesh dumps> [<folder with the recordings>]] (build and build\survey by default, from the project's folder). Reads only.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../src/acog.h"
#include "../src/aimrules.h"
#include "../src/gunbodies.h"
#include "../src/handmag.h"
#include "../src/lidride.h"
#include "../src/m249rows.h"
#include "../src/scopemarks.h"

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

constexpr float kPi = 3.14159265f;
constexpr float kIdentity[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};

// ---- What the game said zoomed on the M249 with it (build\survey\M249ACOG_zoom2.txt, 2026-11-19 01:55: the right mouse button with BF3 in front, nobody in
// the headset): the view in the ACOG's bone 0 (every one of its zoomed draws' glassdump line), its FOV; the first person's projection zoomed; the held gun's
// lines of the same seconds: the view and the aim on the M249's frame. ----
constexpr float kZoomedEye[3] = {0.0f, 0.0351f, -0.2607f}, kZoomedForward[3] = {-0.0008f, -0.0115f, 0.9999f};
constexpr float kZoomedFov = 17.2f, kUnzoomedFov = 55.0f, kWeaponProjection = 7.596f;
constexpr float kFrameEye[3] = {0.0f, 0.110f, 0.100f};
constexpr float kFrameForwardY[3] = {-0.0115f, -0.0103f, -0.0106f}, kFrameAimY[3] = {-0.0111f, -0.0109f, -0.0109f};  // (each z 0.9999)
// tools\scopemarks.ps1's Acog row (the M416's zoomed eye in its ACOG's bone 0, 2026-11-05): what scopemarks::kAcog was made from.
constexpr float kMarksEye[3] = {0.0f, 0.0317f, -0.2674f};
constexpr float kMarksWorld = 6.612f;
// The six draws as the M416's kGunPartDraws has them (skin.cpp: its static_assert holds the held row to those).
constexpr unsigned kM416Acog[6][2] = {{9273, 12}, {9273, 9594}, {72, 9285}, {12, 0}, {129, 9357}, {108, 9486}};
// The eye's picture: 2160 pixels high at the wanted 105.9 degrees (the owner's: build\survey\M249ACOG_zoom2.txt "wanted 105.9 degrees").
constexpr float kEyeHalfHeight = 1080.0f, kEyeFov = 105.9f, kPicturePixels = 80.0f;  // scopeview draws its own picture from a lens 80 pixels in radius

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
float Dist(const float* a, const float* b) {
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}
float Apart(const float* a, const float* b) {  // two bones' places
    return std::sqrt((a[3] - b[3]) * (a[3] - b[3]) + (a[7] - b[7]) * (a[7] - b[7]) + (a[11] - b[11]) * (a[11] - b[11]));
}
float AxesOff(const float* a, const float* b) {  // the most any part of their axes differs
    float most = 0.0f;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            most = std::max(most, std::fabs(a[i * 4 + j] - b[i * 4 + j]));
    return most;
}
float TurnBetween(const float* a, const float* b) {  // degrees
    float trace = 0.0f;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            trace += a[i * 4 + j] * b[i * 4 + j];
    return std::acos(std::clamp((trace - 1.0f) * 0.5f, -1.0f, 1.0f)) * 180.0f / kPi;
}

// A rigid draw's dump (a sight's: stride 32, a half position at 0, its place on the sheet two halves 28 bytes in, all on bone 0), in its own bone 0's space:
// each index's vertex in order, and each vertex once.
struct Point {
    float at[3];
    float uv[2];
    int vertex;
};
struct Dump {
    bool read = false;
    unsigned stride = 0, offset = 0;
    std::vector<uint8_t> vb, ib, bones;
    std::vector<Point> order;   // each index's vertex
    std::vector<Point> unique;  // each vertex once
};
Dump Load(const std::string& prefix, unsigned count, unsigned start) {
    Dump d;
    d.vb = ReadAll(prefix + "vb0.bin"), d.ib = ReadAll(prefix + "ib.bin"), d.bones = ReadAll(prefix + "bones.bin");
    std::ifstream text(prefix + "dump.txt");
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
    if (d.vb.empty() || d.ib.size() < static_cast<size_t>(start + count) * 2 || d.stride != 32)
        return d;
    std::vector<int> seen(65536, 0);
    for (unsigned i = start; i < start + count; ++i) {
        uint16_t v;
        std::memcpy(&v, &d.ib[2 * static_cast<size_t>(i)], 2);
        const size_t o = d.offset + static_cast<size_t>(v) * d.stride;
        if (o + d.stride > d.vb.size())
            return d;
        uint16_t h[3], uv[2];
        std::memcpy(h, &d.vb[o], 6);
        std::memcpy(uv, &d.vb[o + 28], 4);
        const Point p{{Half(h[0]), Half(h[1]), Half(h[2])}, {Half(uv[0]), Half(uv[1])}, v};
        d.order.push_back(p);
        if (!seen[v]) {
            seen[v] = 1;
            d.unique.push_back(p);
        }
    }
    d.read = true;
    return d;
}
std::string Mesh(const std::string& dir, unsigned count, unsigned start) {
    return dir + "\\mesh_" + std::to_string(count) + "_" + std::to_string(start) + "_";
}
bool SameBytes(const std::string& a, const std::string& b) {
    const std::vector<uint8_t> x = ReadAll(a), y = ReadAll(b);
    return !x.empty() && x == y;
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

// A recording (weapon.txt "allrec"): records of 8 ints (the draw's count, the frame's serial, the eye, its start, the rounds loaded, the spare, ms, bones) and
// that many bones; each frame's records together, in the order they were drawn.
struct Record {
    int count, start, ms, frame;
    std::vector<float> bones;
    const float* Bone(int b) const { return &bones[static_cast<size_t>(b) * 12]; }
    int Bones() const { return static_cast<int>(bones.size() / 12); }
};
template <typename Fn>
bool Frames(const std::string& path, Fn&& each) {
    FILE* f = nullptr;
    if (fopen_s(&f, path.c_str(), "rb") || !f)
        return false;
    int head[8], first = -1;
    std::vector<Record> group;
    while (fread(head, 4, 8, f) == 8) {
        const int n = head[7];
        if (n <= 0 || n > 4096)
            break;
        Record r;
        r.bones.resize(static_cast<size_t>(n) * 12);
        if (fread(r.bones.data(), 48, n, f) != static_cast<size_t>(n))
            break;
        if (first < 0)
            first = head[6];
        r.count = head[0], r.start = head[3], r.ms = head[6] - first, r.frame = head[1];
        if (!group.empty() && group.front().frame != r.frame) {
            each(group);
            group.clear();
        }
        group.push_back(std::move(r));
    }
    if (!group.empty())
        each(group);
    fclose(f);
    return first >= 0;
}

// The M249's own draws (m249rows.h): its body (frame bone 3, cover bone 1), its depth pass (frame 8, cover 6), its belt, its box; the gloves.
bool GunsOwn(const Record& r) {
    return (r.count == static_cast<int>(m249rows::kBody) && r.start == static_cast<int>(m249rows::kBodyStart)) ||
           (r.count == static_cast<int>(m249rows::kDepth) && r.start == static_cast<int>(m249rows::kDepth)) ||
           (r.count == static_cast<int>(m249rows::kBeltCount) && r.start == static_cast<int>(m249rows::kBeltStart)) ||
           (r.count == static_cast<int>(m249rows::kBoxCount) && r.start == static_cast<int>(m249rows::kBoxStart)) || (r.count == 13755 && r.start == 15051);
}
bool IsBody(const Record& r) {
    return r.count == static_cast<int>(m249rows::kBody) && r.start == static_cast<int>(m249rows::kBodyStart) && r.Bones() > m249rows::kFrameBone;
}
int AcogDrawOf(const Record& r) {  // which of the six (kM416Acog's order), -1 none
    for (int i = 0; i < 6; ++i)
        if (r.count == static_cast<int>(kM416Acog[i][0]) && r.start == static_cast<int>(kM416Acog[i][1]))
            return i;
    return -1;
}

// A point's slope from an eye looking along forward (x left, y up, the view's own up the bone's y made square to forward), in the view's own terms.
void SlopeFrom(const float* eye, const float* forward, const float* p, float* out) {
    float f[3] = {forward[0], forward[1], forward[2]};
    const float fl = std::sqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
    for (float& c : f)
        c /= fl;
    float u[3] = {-f[1] * f[0], 1.0f - f[1] * f[1], -f[1] * f[2]};
    const float ul = std::sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
    for (float& c : u)
        c /= ul;
    const float l[3] = {u[1] * f[2] - u[2] * f[1], u[2] * f[0] - u[0] * f[2], u[0] * f[1] - u[1] * f[0]};
    const float d[3] = {p[0] - eye[0], p[1] - eye[1], p[2] - eye[2]};
    const float along = d[0] * f[0] + d[1] * f[1] + d[2] * f[2];
    out[0] = (d[0] * l[0] + d[1] * l[1] + d[2] * l[2]) / along;
    out[1] = (d[0] * u[0] + d[1] * u[1] + d[2] * u[2]) / along;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "build", survey = argc > 2 ? argv[2] : "build\\survey";
    char what[1200];
    const handmag::BeltGun& belt = m249rows::kBelt;
    const float* mount = m249rows::kSightMount;
    const float scale = kWeaponProjection / kMarksWorld;  // the weapon's projection over the world's (scopemarks.ps1's)
    const float worldProjection = 1.0f / std::tan(kZoomedFov / 2 * kPi / 180.0f);

    // ---- a1: the tables ----
    {
        bool keys = true;
        const unsigned* own[6] = {acog::kBody, acog::kDepth, acog::kGlass, acog::kChevron, acog::kFibre[0], acog::kFibre[1]};
        for (int i = 0; i < 6; ++i)
            keys = keys && own[i][0] == kM416Acog[i][0] && own[i][1] == kM416Acog[i][1];
        std::snprintf(what, sizeof(what), "a1 its draws the M416's ACOG's six (body %u/%u, depth %u/%u, glass %u/%u, chevron %u/%u, fibre %u/%u and %u/%u)", acog::kBody[0], acog::kBody[1],
                      acog::kDepth[0], acog::kDepth[1], acog::kGlass[0], acog::kGlass[1], acog::kChevron[0], acog::kChevron[1], acog::kFibre[0][0], acog::kFibre[0][1], acog::kFibre[1][0],
                      acog::kFibre[1][1]);
        Check(keys, what);
        const float numbers[] = {acog::kEyepiece[0], acog::kEyepiece[1], acog::kEyepiece[2], acog::kRadius, acog::kPower, mount[0], mount[1], mount[2]};
        std::snprintf(what, sizeof(what), "a1 its eyepiece (%.4f %.4f %.4f), %.1f mm in radius, %.2f times; the cover's mount (%.5f %.5f %.5f): every number finite", acog::kEyepiece[0],
                      acog::kEyepiece[1], acog::kEyepiece[2], acog::kRadius * 1000.0f, acog::kPower, mount[0], mount[1], mount[2]);
        Check(std::all_of(std::begin(numbers), std::end(numbers), [](float f) { return std::isfinite(f); }) && acog::kRadius > 0.005f && acog::kPower > 1.5f, what);
        const scopemarks::Marks& marks = scopemarks::kAcog;
        std::snprintf(what, sizeof(what), "a1 its marks scopemarks::kAcog: %d dark, %d bright, colour (%.0f %.0f %.0f), reaching %.4f of slope", marks.darkCount, marks.brightCount, marks.color[0],
                      marks.color[1], marks.color[2], marks.reach);
        Check(marks.darkCount == 516 && marks.brightCount == 12 && marks.dark && marks.bright && marks.color[0] == 1.0f && marks.color[1] == 0.0f && marks.color[2] == 0.0f, what);
        const gunbodies::Attach* attach = gunbodies::AttachFor(acog::kBody[0], acog::kBody[1], 0);
        std::snprintf(what, sizeof(what), "a1 its corners an attachment of the gun it's on (gunbodies::kAttach, gunbodyfit's): %d points", attach ? attach->points : 0);
        Check(attach && attach->points == 8, what);
        const handmag::BeltGun plain{};
        std::snprintf(what, sizeof(what), "a1 the M249's cover carries what's on it %d, its mount the row's %d; a cover by default (the M240's) carries nothing, no mount %d", belt.carries,
                      belt.mount == m249rows::kSightMount, !plain.carries && !plain.mount);
        Check(belt.carries && belt.mount == m249rows::kSightMount && !plain.carries && !plain.mount, what);
    }

    // ---- a2: its meshes (today's dumps, build\survey\M249ACOG_mesh_*, beside the M416's of 11-05: M249ACOG_prev_mesh) ----
    const std::string today = survey + "\\M249ACOG_mesh_";
    Dump body = Load(today + "9273_12_", acog::kBody[0], acog::kBody[1]), glass = Load(today + "72_9285_", acog::kGlass[0], acog::kGlass[1]);
    Dump chevron = Load(today + "12_0_", acog::kChevron[0], acog::kChevron[1]);
    Dump fibre[2] = {Load(today + "129_9357_", acog::kFibre[0][0], acog::kFibre[0][1]), Load(today + "108_9486_", acog::kFibre[1][0], acog::kFibre[1][1])};
    if (!body.read || !glass.read || !chevron.read || !fibre[0].read || !fibre[1].read) {
        Skip("a2 no dumps of its six draws (build\\survey\\M249ACOG_mesh_*): its meshes");
    } else {
        const std::string prev = survey + "\\M249ACOG_prev_mesh\\mesh_";
        const bool m416 = SameBytes(today + "9273_12_vb0.bin", prev + "9273_12_vb0.bin") && SameBytes(today + "9273_12_ib.bin", prev + "9273_12_ib.bin") &&
                          SameBytes(today + "9273_9594_vb0.bin", prev + "9273_9594_vb0.bin") && SameBytes(today + "9273_9594_ib.bin", prev + "9273_9594_ib.bin");
        bool shared = true;
        for (const char* part : {"72_9285_", "12_0_", "129_9357_", "108_9486_"})
            shared = shared && SameBytes(today + part + std::string("vb0.bin"), today + "9273_12_vb0.bin") && SameBytes(today + part + std::string("ib.bin"), today + "9273_12_ib.bin");
        std::snprintf(what, sizeof(what), "a2 its body and depth pass the M416's ACOG's byte for byte (the 11-05 dumps) %d; its glass, chevron and both fibres drawn from the body's very buffers %d",
                      m416, shared);
        Check(m416 && shared, what);
        // The glass: its middle vertex and its rim.
        Box all;
        for (const Point& p : glass.unique)
            all.Add(p.at);
        const float boxMid[2] = {(all.lo[0] + all.hi[0]) / 2, (all.lo[1] + all.hi[1]) / 2};
        const Point* middle = &glass.unique.front();
        for (const Point& p : glass.unique)
            if (std::hypot(p.at[0] - boxMid[0], p.at[1] - boxMid[1]) < std::hypot(middle->at[0] - boxMid[0], middle->at[1] - boxMid[1]))
                middle = &p;
        float rimLeast = 1e9f, rimMost = 0.0f;
        for (const Point& p : glass.unique)
            if (&p != middle) {
                const float r = std::hypot(p.at[0] - middle->at[0], p.at[1] - middle->at[1]);
                rimLeast = std::min(rimLeast, r), rimMost = std::max(rimMost, r);
            }
        std::snprintf(what, sizeof(what),
                      "a2 its glass (72/9285): %zu vertices flat at z %.4f (%.2f mm deep), its middle vertex (%.5f %.5f), its rim %.2f to %.2f mm round it; the row's eyepiece (%.4f %.4f %.4f) "
                      "%.2f mm off it, its radius %.1f mm",
                      glass.unique.size(), middle->at[2], (all.hi[2] - all.lo[2]) * 1000.0f, middle->at[0], middle->at[1], rimLeast * 1000.0f, rimMost * 1000.0f, acog::kEyepiece[0], acog::kEyepiece[1],
                      acog::kEyepiece[2], Dist(middle->at, acog::kEyepiece) * 1000.0f, acog::kRadius * 1000.0f);
        Check(glass.unique.size() == 25 && all.hi[2] - all.lo[2] < 0.0001f && Dist(middle->at, acog::kEyepiece) < 0.0003f && rimLeast > acog::kRadius - 0.0002f &&
                  rimMost < acog::kRadius + 0.0002f,
              what);
        // The game's own chevron (drawn unzoomed, at the sight): its tip on the row's line, ahead of the glass.
        const Point* tip = &chevron.unique.front();
        for (const Point& p : chevron.unique)
            if (p.at[1] > tip->at[1])
                tip = &p;
        std::snprintf(what, sizeof(what), "a2 the game's own chevron (12/0, %zu vertices) at z %.4f, %.1f cm ahead of the glass, its tip (%.5f %.5f): %.2f mm off the row's line",
                      chevron.unique.size(), tip->at[2], (tip->at[2] - acog::kEyepiece[2]) * 100.0f, tip->at[0], tip->at[1],
                      std::hypot(tip->at[0] - acog::kEyepiece[0], tip->at[1] - acog::kEyepiece[1]) * 1000.0f);
        Check(chevron.unique.size() == 6 && tip->at[2] > acog::kEyepiece[2] + 0.05f && std::hypot(tip->at[0] - acog::kEyepiece[0], tip->at[1] - acog::kEyepiece[1]) < 0.0005f, what);
        // The body round the eyepiece; the fibre on its top.
        Box hull, top[2];
        for (const Point& p : body.unique)
            hull.Add(p.at);
        for (int i = 0; i < 2; ++i)
            for (const Point& p : fibre[i].unique)
                top[i].Add(p.at);
        std::snprintf(what, sizeof(what),
                      "a2 its body x %.4f to %.4f, y %.4f to %.4f, z %.4f to %.4f (%zu vertices): the eyepiece %.1f mm inside its back end; its fibre on top: y %.4f to %.4f and %.4f to %.4f, "
                      "z %.4f to %.4f",
                      hull.lo[0], hull.hi[0], hull.lo[1], hull.hi[1], hull.lo[2], hull.hi[2], body.unique.size(), (acog::kEyepiece[2] - hull.lo[2]) * 1000.0f, top[0].lo[1], top[0].hi[1],
                      top[1].lo[1], top[1].hi[1], std::min(top[0].lo[2], top[1].lo[2]), std::max(top[0].hi[2], top[1].hi[2]));
        Check(hull.lo[2] < acog::kEyepiece[2] && acog::kEyepiece[2] - hull.lo[2] < 0.01f && top[0].lo[1] > acog::kEyepiece[1] + acog::kRadius && top[1].lo[1] > acog::kEyepiece[1] + acog::kRadius &&
                  top[0].hi[1] <= hull.hi[1] + 0.001f && top[1].hi[1] <= hull.hi[1] + 0.001f,
              what);
        // Its attachment's points (gunbodyfit's): each the vertex of its body furthest toward one of its box's eight corners, in the box's own proportions.
        if (const gunbodies::Attach* attach = gunbodies::AttachFor(acog::kBody[0], acog::kBody[1], 0)) {
            float mid[3], half[3];
            for (int k = 0; k < 3; ++k)
                mid[k] = 0.5f * (hull.lo[k] + hull.hi[k]), half[k] = std::max(0.5f * (hull.hi[k] - hull.lo[k]), 1e-4f);
            int matched = 0;
            for (int i = 0; i < attach->points; ++i) {
                bool corner = false;
                for (int c = 0; c < 8 && !corner; ++c) {
                    const float way[3] = {c & 1 ? 1.0f : -1.0f, c & 2 ? 1.0f : -1.0f, c & 4 ? 1.0f : -1.0f};
                    const Point* best = nullptr;
                    float bestAlong = -1e30f;
                    for (const Point& p : body.unique) {
                        float along = 0.0f;
                        for (int k = 0; k < 3; ++k)
                            along += (p.at[k] - mid[k]) / half[k] * way[k];
                        if (along > bestAlong)
                            bestAlong = along, best = &p;
                    }
                    corner = best && Dist(best->at, attach->point[i]) < 0.0001f;
                }
                matched += corner;
            }
            std::snprintf(what, sizeof(what), "a2 its attachment's %d points: %d of them its body's vertex furthest toward a corner of its box (gunbodyfit's rule)", attach->points, matched);
            Check(attach->points == 8 && matched == attach->points, what);
        }
    }

    // ---- a3: its zoomed model and marks ----
    {
        const std::string zoomed = survey + "\\M249ACOG_zoomed_mesh_";
        bool same = true;
        for (const char* part : {"516_6957_", "12_0_", "6705_12_"})
            same = same && SameBytes(zoomed + part + std::string("vb0.bin"), dir + "\\mesh_" + part + "vb0.bin") &&
                   SameBytes(zoomed + part + std::string("ib.bin"), dir + "\\mesh_" + part + "ib.bin");
        const Dump dark = Load(Mesh(dir, 516, 6957), 516, 6957), bright = Load(Mesh(dir, 12, 0), 12, 0);
        if (!dark.read || !bright.read) {
            Skip("a3 no dumps of its zoomed marks (build\\mesh_516_6957_*, mesh_12_0_*): kAcog against them");
        } else {
            std::snprintf(what, sizeof(what), "a3 on the M249 the game swaps in the M416's zoomed ACOG byte for byte (survey M249ACOG_zoomed_mesh_516_6957, 12_0, 6705_12 against build\\mesh_*: "
                                              "the 11-05 dumps kAcog was made from) %d", same);
            Check(same, what);
            // kAcog made again (scopemarks.ps1's Part: each index's vertex, its slope from the Acog row's eye times the weapon's projection over the world's).
            const scopemarks::Marks& marks = scopemarks::kAcog;
            float worst = 0.0f, worstUv = 0.0f, reach = 0.0f;
            const auto again = [&](const Dump& d, const scopemarks::Vertex* table, int count) {
                for (int i = 0; i < count && i < static_cast<int>(d.order.size()); ++i) {
                    const Point& p = d.order[i];
                    const float depthAway = p.at[2] - kMarksEye[2];
                    const float x = (p.at[0] - kMarksEye[0]) / depthAway * scale, y = (p.at[1] - kMarksEye[1]) / depthAway * scale;
                    worst = std::max({worst, std::fabs(x - table[i].x), std::fabs(y - table[i].y)});
                    worstUv = std::max({worstUv, std::fabs(p.uv[0] - table[i].u), std::fabs(p.uv[1] - table[i].v)});
                    reach = std::max({reach, std::fabs(x), std::fabs(y)});
                }
            };
            again(dark, marks.dark, marks.darkCount);
            again(bright, marks.bright, marks.brightCount);
            std::snprintf(what, sizeof(what),
                          "a3 kAcog made again from the dumps (the eye (%.4f %.4f %.4f), the weapon's projection %.3f over the world's %.3f): %.6f of slope off at most, %.5f on the sheet; reaching "
                          "%.4f (the table's %.4f)",
                          kMarksEye[0], kMarksEye[1], kMarksEye[2], kWeaponProjection, kMarksWorld, worst, worstUv, reach, marks.reach);
            Check(dark.order.size() == 516 && bright.order.size() == 12 && worst < 1e-4f && worstUv < 1e-3f && std::fabs(reach - marks.reach) < 2e-4f, what);
            // The power: the game's own zoom.
            const float power = std::tan(kUnzoomedFov / 2 * kPi / 180.0f) / std::tan(kZoomedFov / 2 * kPi / 180.0f);
            std::snprintf(what, sizeof(what), "a3 the game zooms it from %.1f to %.1f degrees on the M249: tan(%.2f) / tan(%.2f) = %.3f, the row's power %.2f; the world's projection %.3f (scopemarks.ps1's %.3f), "
                                              "the first person's %.3f (FOV %.2f)",
                          kUnzoomedFov, kZoomedFov, kUnzoomedFov / 2, kZoomedFov / 2, power, acog::kPower, worldProjection, kMarksWorld, kWeaponProjection,
                          2.0f * std::atan(1.0f / kWeaponProjection) * 180.0f / kPi);
            Check(std::fabs(power - acog::kPower) < 0.01f && std::fabs(worldProjection - kMarksWorld) < 0.001f && std::fabs(2.0f * std::atan(1.0f / kWeaponProjection) * 180.0f / kPi - 15.0f) < 0.01f,
                  what);
            // The M249's own zoomed eye (the survey's): where it sees the chevron's tip against its own forward, in the world's terms, and the marks' size, against kAcog.
            const Point* tipPoint = &bright.order.front();
            int tipIndex = 0;
            for (int i = 0; i < static_cast<int>(bright.order.size()); ++i)
                if (bright.order[i].at[1] > tipPoint->at[1])
                    tipPoint = &bright.order[i], tipIndex = i;
            float seen[2];
            SlopeFrom(kZoomedEye, kZoomedForward, tipPoint->at, seen);
            const float tip249[2] = {seen[0] * scale, seen[1] * scale}, tipTable[2] = {marks.bright[tipIndex].x, marks.bright[tipIndex].y};
            std::snprintf(what, sizeof(what),
                          "a3 the chevron's tip: from the M249's own zoomed eye (%.4f %.4f %.4f), looking (%.4f %.4f %.4f), at (%.2f %.2f) mrad from its middle; kAcog has it at (%.2f %.2f): %.2f mrad "
                          "up and %.2f across apart (the sample looked %.2f mrad aside: the four on the frame -0.5 to 0.9, its sway)",
                          kZoomedEye[0], kZoomedEye[1], kZoomedEye[2], kZoomedForward[0], kZoomedForward[1], kZoomedForward[2], tip249[0] * 1000.0f, tip249[1] * 1000.0f, tipTable[0] * 1000.0f,
                          tipTable[1] * 1000.0f, std::fabs(tip249[1] - tipTable[1]) * 1000.0f, std::fabs(tip249[0] - tipTable[0]) * 1000.0f, -kZoomedForward[0] * 1000.0f);
            Check(std::fabs(tip249[1] - tipTable[1]) < 0.0008f && std::fabs(tip249[0] - tipTable[0]) < 0.0012f, what);
            // The marks' size: the dark marks' span across, from the M249's eye and in kAcog.
            float spanEye = 0.0f, spanTable = 0.0f;
            {
                float lo249 = 1e9f, hi249 = -1e9f, loTable = 1e9f, hiTable = -1e9f;
                for (int i = 0; i < marks.darkCount && i < static_cast<int>(dark.order.size()); ++i) {
                    float s[2];
                    SlopeFrom(kZoomedEye, kZoomedForward, dark.order[i].at, s);
                    lo249 = std::min(lo249, s[0] * scale), hi249 = std::max(hi249, s[0] * scale);
                    loTable = std::min(loTable, marks.dark[i].x), hiTable = std::max(hiTable, marks.dark[i].x);
                }
                spanEye = hi249 - lo249, spanTable = hiTable - loTable;
            }
            std::snprintf(what, sizeof(what), "a3 its marks across: %.2f mrad from the M249's own zoomed eye (6.7 mm nearer them than the M416's), %.2f in kAcog: %.1f%% larger on the M249",
                          spanEye * 1000.0f, spanTable * 1000.0f, (spanEye / spanTable - 1.0f) * 100.0f);
            Check(spanTable > 0.0f && spanEye / spanTable > 1.0f && spanEye / spanTable < 1.04f, what);
        }
        // The survey's own line, where it is: the view in the zoomed model's bone 0 as the table above has it.
        std::ifstream text(survey + "\\M249ACOG_zoom2.txt");
        const std::string all{std::istreambuf_iterator<char>(text), std::istreambuf_iterator<char>()};
        if (all.empty()) {
            Skip("a3 no build\\survey\\M249ACOG_zoom2.txt: the zoomed eye as the survey said it");
        } else {
            const size_t at = all.find("Glass 516/6957: the view in its bone 0: eye (");
            float e[3] = {}, f[3] = {}, fov = 0.0f;
            const bool read = at != std::string::npos &&
                              std::sscanf(all.c_str() + at, "Glass 516/6957: the view in its bone 0: eye (%f %f %f), forward (%f %f %f), the game's FOV %f", &e[0], &e[1], &e[2], &f[0], &f[1], &f[2], &fov) == 7;
            const bool projection = all.find("Glass 2_516_6957: the view's projection 7.596: a FOV of 15.00") != std::string::npos;
            std::snprintf(what, sizeof(what), "a3 the survey's zoomed view in the ACOG's bone: eye (%.4f %.4f %.4f), forward (%.4f %.4f %.4f), FOV %.2f; the first person's projection 7.596 %d",
                          e[0], e[1], e[2], f[0], f[1], f[2], fov, projection);
            Check(read && Dist(e, kZoomedEye) < 0.00006f && Dist(f, kZoomedForward) < 0.00006f && std::fabs(fov - kZoomedFov) < 0.01f && projection, what);
        }
    }

    // ---- a4: the mount (the game's own reload with it on, build\survey\M249ACOG_reload.bin) ----
    {
        long frames = 0, onMount[6] = {}, records[6] = {}, coverRides = 0, gunBones = 0, gunOnMount = 0;
        float worstPlace = 0.0f, worstAxes = 0.0f, coverMost = 0.0f, restWorst = 0.0f;
        float coverRest[12] = {}, sightRest[12] = {}, coverOpen[12] = {}, sightOpen[12] = {};
        bool haveRest = false, haveSightRest = false;
        std::map<std::pair<int, int>, std::vector<int>> strays;  // other draws with a bone on the mount: their frames
        const bool any = Frames(survey + "\\M249ACOG_reload.bin", [&](const std::vector<Record>& group) {
            std::vector<const Record*> bodies;
            for (const Record& r : group)
                if (IsBody(r))
                    bodies.push_back(&r);
            if (bodies.empty())
                return;
            ++frames;
            // Each body's cover on its frame (the eyes' bodies), and the gun's own bones against the mount.
            std::vector<std::pair<std::vector<float>, std::vector<float>>> covers;  // (frame's inverse, cover on it)
            for (const Record* b : bodies) {
                std::vector<float> inv(12), cover(12);
                InvertRigid(b->Bone(m249rows::kFrameBone), inv.data());
                Compose(inv.data(), b->Bone(m249rows::kCoverBone), cover.data());
                coverRides += lidride::Rides(cover.data(), cover.data()) && !lidride::OnMount(belt, cover.data(), cover.data());
                for (int k = 0; k <= m249rows::kFrameBone; ++k) {
                    float on[12];
                    Compose(inv.data(), b->Bone(k), on);
                    ++gunBones;
                    gunOnMount += lidride::OnMount(belt, cover.data(), on);
                }
                covers.push_back({std::move(inv), std::move(cover)});
            }
            const std::vector<float>& cover0 = covers.front().second;
            if (!haveRest) {
                std::memcpy(coverRest, cover0.data(), sizeof(coverRest));
                haveRest = true;
            }
            float restInverse[12], moved[12];
            InvertRigid(coverRest, restInverse);
            Compose(cover0.data(), restInverse, moved);
            const float angle = std::atan2(moved[9], moved[5]) * 180.0f / kPi;
            for (const Record& r : group) {
                const int which = AcogDrawOf(r);
                if (which < 0) {
                    if (GunsOwn(r))
                        continue;
                    for (const auto& [inv, cover] : covers)
                        for (int k = 0; k < std::min(r.Bones(), 85); ++k) {
                            float on[12];
                            Compose(inv.data(), r.Bone(k), on);
                            if (lidride::OnMount(belt, cover.data(), on)) {
                                std::vector<int>& seenIn = strays[{r.count, r.start}];
                                if (seenIn.empty() || seenIn.back() != r.frame)
                                    seenIn.push_back(r.frame);
                            }
                        }
                    continue;
                }
                ++records[which];
                // Its bone 0 against the cover of the eye it was drawn for: of the bodies' covers, the one it's nearest the mount on.
                float bestPlace = 1e9f, bestAxes = 1e9f, bestOn[12] = {}, bestCover[12] = {};
                bool took = false;
                for (const auto& [inv, cover] : covers) {
                    float on[12], inCover[12], coverInverse[12];
                    Compose(inv.data(), r.Bone(0), on);
                    InvertRigid(cover.data(), coverInverse);
                    Compose(coverInverse, on, inCover);
                    const float place = std::sqrt((inCover[3] - mount[0]) * (inCover[3] - mount[0]) + (inCover[7] - mount[1]) * (inCover[7] - mount[1]) +
                                                  (inCover[11] - mount[2]) * (inCover[11] - mount[2]));
                    if (place < bestPlace) {
                        bestPlace = place, bestAxes = AxesOff(inCover, kIdentity);
                        took = lidride::OnMount(belt, cover.data(), on);
                        std::memcpy(bestOn, on, sizeof(bestOn));
                        std::memcpy(bestCover, cover.data(), sizeof(bestCover));
                    }
                }
                worstPlace = std::max(worstPlace, bestPlace), worstAxes = std::max(worstAxes, bestAxes);
                onMount[which] += took;
                if (which == 0 && bestPlace < 1e8f) {
                    if (!haveSightRest)
                        std::memcpy(sightRest, bestOn, sizeof(sightRest)), haveSightRest = true;
                    else if (angle < 0.5f)
                        restWorst = std::max(restWorst, Apart(bestOn, sightRest));
                    if (angle > coverMost) {
                        coverMost = angle;
                        std::memcpy(coverOpen, bestCover, sizeof(coverOpen));
                        std::memcpy(sightOpen, bestOn, sizeof(sightOpen));
                    }
                }
            }
        });
        if (!any || !frames) {
            Skip("a4 no recording of the game's own reload with it on (build\\survey\\M249ACOG_reload.bin): its mount");
        } else {
            bool every = true;
            for (int i = 0; i < 6; ++i)
                every = every && records[i] >= 2 * frames && onMount[i] == records[i];
            std::snprintf(what, sizeof(what),
                          "a4 through the game's own reload (%ld frames, the cover up to %.1f degrees and shut): its six draws' bone 0 on the cover's mount (%.5f %.5f %.5f), the cover's axes, in every "
                          "one of their records (%ld %ld %ld %ld %ld %ld of %ld %ld %ld %ld %ld %ld): %.4f mm and %.6f of an axis off at most; lidride::OnMount takes every one",
                          frames, coverMost, mount[0], mount[1], mount[2], onMount[0], onMount[1], onMount[2], onMount[3], onMount[4], onMount[5], records[0], records[1], records[2], records[3],
                          records[4], records[5], worstPlace * 1000.0f, worstAxes);
            Check(every && worstPlace < 0.00001f && worstAxes < 0.0001f && coverMost > 100.0f, what);
            std::snprintf(what, sizeof(what), "a4 the gun's own bones (its handle, cover, carrying handle, frame: %ld) never on the mount (%ld); the cover's own bone is lidride::Rides', not the mount's (%ld of %ld frames' bodies)",
                          gunBones, gunOnMount, coverRides, gunBones / 4);
            Check(gunOnMount == 0 && coverRides == gunBones / 4, what);
            std::snprintf(what, sizeof(what), "a4 at rest on the frame: the cover's bone (%.4f %.4f %.4f), the frame's axes (%.6f off); the sight (%.4f %.4f %.4f), the frame's axes (%.6f off), %.3f mm the most "
                                              "it's off that with the cover shut",
                          coverRest[3], coverRest[7], coverRest[11], AxesOff(coverRest, kIdentity), sightRest[3], sightRest[7], sightRest[11], AxesOff(sightRest, kIdentity), restWorst * 1000.0f);
            const float onMountRest[12] = {1, 0, 0, mount[0], 0, 1, 0, mount[1], 0, 0, 1, mount[2]};
            float expect[12];
            Compose(coverRest, onMountRest, expect);
            Check(std::fabs(sightRest[3]) < 0.0001f && std::fabs(sightRest[7] - 0.0749f) < 0.0001f && std::fabs(sightRest[11] - 0.3605f) < 0.0001f && Apart(sightRest, expect) < 0.00001f &&
                      AxesOff(sightRest, kIdentity) < 0.0001f && AxesOff(coverRest, kIdentity) < 0.0001f && restWorst < 0.0001f,
                  what);
            // The cover lifted by hand (lidride::Turn to the row's 102.6 degrees) takes the mount where the game's own reload has the sight at its most open.
            float turn[12], lifted[12], liftedSight[12];
            lidride::Turn(belt, belt.lidOpen, turn);
            Compose(turn, coverRest, lifted);
            const float onMountPose[12] = {1, 0, 0, mount[0], 0, 1, 0, mount[1], 0, 0, 1, mount[2]};
            Compose(lifted, onMountPose, liftedSight);
            std::snprintf(what, sizeof(what),
                          "a4 lifted by hand to %.1f degrees (lidride::Turn): the sight at (%.4f %.4f %.4f) on the frame, the game's own reload's at its most open (%.1f degrees) (%.4f %.4f %.4f): %.2f mm "
                          "and %.2f degrees apart; it goes where the game takes it",
                          belt.lidOpen * 180.0f / kPi, liftedSight[3], liftedSight[7], liftedSight[11], coverMost, sightOpen[3], sightOpen[7], sightOpen[11], Apart(liftedSight, sightOpen) * 1000.0f,
                          TurnBetween(liftedSight, sightOpen));
            Check(Apart(liftedSight, sightOpen) < 0.0005f && TurnBetween(liftedSight, sightOpen) < 0.2f && lidride::OnMount(belt, lifted, liftedSight) &&
                      lidride::OnMount(belt, coverOpen, sightOpen),
                  what);
            // Other draws: a bone on the mount only in a stray frame or two (another draw's palette left over), never as the sight's are.
            std::string list;
            size_t mostFrames = 0;
            for (const auto& [key, seenIn] : strays) {
                list += (list.empty() ? "" : ", ") + std::to_string(key.first) + "/" + std::to_string(key.second) + " (" + std::to_string(seenIn.size()) + ")";
                mostFrames = std::max(mostFrames, seenIn.size());
            }
            std::snprintf(what, sizeof(what), "a4 any other draw with a bone on the mount: %s; at most %zu frames of %ld (palettes left over: what's on the mount goes up with the cover too)",
                          list.empty() ? "none" : list.c_str(), mostFrames, frames);
            Check(mostFrames <= 5, what);
        }
    }

    // ---- a5: the bare M249 (its recordings of 2026-11-18 in the garage: no ACOG on it) ----
    {
        long frames = 0, draws = 0, onMount = 0;
        bool any = false;
        for (const char* which : {"partial", "empty"}) {
            any = Frames(survey + "\\M249_" + which + ".bin", [&](const std::vector<Record>& group) {
                std::vector<const Record*> bodies;
                for (const Record& r : group)
                    if (IsBody(r))
                        bodies.push_back(&r);
                if (bodies.empty())
                    return;
                ++frames;
                for (const Record* b : bodies) {
                    float inv[12], cover[12];
                    InvertRigid(b->Bone(m249rows::kFrameBone), inv);
                    Compose(inv, b->Bone(m249rows::kCoverBone), cover);
                    for (const Record& r : group) {
                        if (GunsOwn(r))
                            continue;
                        ++draws;
                        for (int k = 0; k < std::min(r.Bones(), 85); ++k) {
                            float on[12];
                            Compose(inv, r.Bone(k), on);
                            onMount += lidride::OnMount(belt, cover, on);
                        }
                    }
                }
            }) || any;
        }
        if (!any) {
            Skip("a5 no recordings of the bare M249 (build\\survey\\M249_partial.bin, M249_empty.bin)");
        } else {
            std::snprintf(what, sizeof(what), "a5 the bare M249's recordings (%ld frames, %ld draws not its own, every bone): %ld on the cover's mount: with the cover up by hand every draw as before",
                          frames, draws, onMount);
            Check(frames > 2000 && onMount == 0, what);
        }
    }

    // ---- a6: its line ----
    {
        // Its glass on the M249's frame at rest (the mount's place on the cover at rest: a4's), the lens's line through it falling as the gun's sights' line
        // (skin.cpp: ScopeLens and ScopeWindow with tan(kUp) for a row with gunsLine), its bullets' start (aimrules::HeldShotLine) on that line at the muzzle.
        const float sightAt[3] = {0.0f, 0.0749f, 0.3605f};
        const float glassOn[3] = {sightAt[0] + acog::kEyepiece[0], sightAt[1] + acog::kEyepiece[1], sightAt[2] + acog::kEyepiece[2]};
        const float up = m249rows::kUp, fall = std::tan(up * kPi / 180.0f);
        float start[3];
        aimrules::HeldShotLine(m249rows::kSights, up, glassOn, start);
        const float lineY = glassOn[1] - fall * (start[2] - glassOn[2]);
        float irons[3];
        aimrules::HeldShotLine(m249rows::kSights, up, nullptr, irons);
        std::snprintf(what, sizeof(what),
                      "a6 its glass on the frame (%.4f %.4f %.4f); the lens's line through it falling %.2f degrees (the gun's sights'); its bullets start at (%.4f %.4f %.4f), %.4f mm off that line, "
                      "%.1f cm over the irons' start (%.3f %.3f %.3f): on its line, not the irons'",
                      glassOn[0], glassOn[1], glassOn[2], up, start[0], start[1], start[2], std::fabs(start[1] - lineY) * 1000.0f, (start[1] - irons[1]) * 100.0f, irons[0], irons[1], irons[2]);
        Check(std::fabs(start[1] - lineY) < 1e-5f && start[0] == glassOn[0] && start[2] == m249rows::kSights[2] && start[1] - irons[1] > 0.01f, what);
        // The game's own zoomed aim through it on the M249's frame against the gun's sights' line.
        float least = 1e9f, most = -1e9f;
        for (const float y : kFrameForwardY)
            least = std::min(least, std::atan2(-y, 0.9999f) * 180.0f / kPi), most = std::max(most, std::atan2(-y, 0.9999f) * 180.0f / kPi);
        for (const float y : kFrameAimY)
            least = std::min(least, std::atan2(-y, 0.9999f) * 180.0f / kPi), most = std::max(most, std::atan2(-y, 0.9999f) * 180.0f / kPi);
        const float inBone = std::atan2(-kZoomedForward[1], kZoomedForward[2]) * 180.0f / kPi;
        std::snprintf(what, sizeof(what), "a6 the game's own zoomed view and aim through it on the frame %.2f to %.2f degrees under z (%.2f in the ACOG's bone, whose axes are the frame's); the lens's %.2f: "
                                          "%.2f degrees at most apart",
                      least, most, inBone, up, std::max(std::fabs(up - least), std::fabs(up - most)));
        Check(std::max(std::fabs(up - least), std::fabs(up - most)) < 0.2f && std::fabs(inBone - up) < 0.2f, what);
        // The chevron's tip against the line (kAcog's slopes are from it): where its bullets go, as on the M416.
        const scopemarks::Marks& marks = scopemarks::kAcog;
        float tip[2] = {0.0f, -1e9f};
        for (int i = 0; i < marks.brightCount; ++i)
            if (marks.bright[i].y > tip[1])
                tip[0] = marks.bright[i].x, tip[1] = marks.bright[i].y;
        std::snprintf(what, sizeof(what), "a6 the chevron's tip %.2f mrad over its line and %.2f across (as on the M416, its rounds on the tip: the owner, 2026-11-04): %.1f cm over at 100 m",
                      tip[1] * 1000.0f, tip[0] * 1000.0f, tip[1] * 10000.0f);
        Check(tip[1] > 0.0f && tip[1] < 0.0015f && std::fabs(tip[0]) < 0.0002f, what);
        // The lens in the picture: at the game's own zoomed eye, and where the second renderer's picture comes on (80 pixels in radius).
        const float focal = kEyeHalfHeight / std::tan(kEyeFov / 2 * kPi / 180.0f);
        const float relief = Dist(kFrameEye, glassOn), atEye = acog::kRadius / relief * focal, within = acog::kRadius * focal / kPicturePixels;
        std::snprintf(what, sizeof(what), "a6 the lens at the game's zoomed eye (%.1f cm behind the glass): %.0f pixels in radius in a 2160-high eye at %.1f degrees; the second renderer's picture within "
                                          "%.1f cm of the glass (%.0f pixels), as the M416's ACOG's",
                      relief * 100.0f, atEye, kEyeFov, within * 100.0f, kPicturePixels);
        Check(relief > 0.15f && relief < 0.25f && atEye > 30.0f && atEye < kPicturePixels && within > 0.10f && within < 0.13f, what);
    }

    // ---- a7: lifted by hand, each draw framed as DrawHeldPart frames it (the game's own reload with it on, build\survey\M249ACOG_reload.bin) ----
    {
        // The draws in the order the game drew them: each framed by the last body or depth pass drawn before it (DrawHeldPart's g_launcherFrameOrig: the
        // body's frame bone 3, the depth pass's 8), against the last body's cover (g_heldLidOnFrame: its bone 1 on that frame), as skin.cpp has them. The
        // recording has no views: a draw after the player's shadow (the shadow views' draws, 23952/1728 the first of them) since the last body or depth
        // pass comes after the see-through pass's own first-person view (build\logs\bf3vr_1119_acog.log: "first-person views a frame: 6", each eye's
        // picture order F6 ... F24 ... F67, the last after the shadow views), never in the body's (skin.cpp's exact false). The game's cover at rest (as
        // whenever a hand lifts it: the game never moves it then): the last body's and the one this draw is framed by. In a hand: never drawn as kept.
        constexpr int kShadowCount = 23952, kShadowStart = 1728;
        long frames = 0, records[6] = {}, later[6] = {}, tried[6] = {}, byView[6] = {}, took[6] = {}, restRecords[6] = {}, restTook[6] = {}, straysTried = 0;
        float worst[6] = {}, restWorst[6] = {};
        float frameInverse[12] = {}, lidOn[12] = {}, framedCover[12] = {}, coverRest[12] = {};
        bool haveFrame = false, haveLid = false, haveRest = false, laterView = false;
        int sightFrame = -1000;  // the frame its body or depth pass was last drawn in (skin.cpp's g_heldSightFrame)
        std::map<std::pair<int, int>, std::vector<int>> strays;  // other draws after the shadow with a bone on the mount: their frames
        const auto mountOff = [&](const float* lid, const float* on) {
            float d = 0.0f;
            for (int i = 0; i < 3; ++i) {
                const float at = lid[i * 4] * mount[0] + lid[i * 4 + 1] * mount[1] + lid[i * 4 + 2] * mount[2] + lid[i * 4 + 3];
                d += (on[i * 4 + 3] - at) * (on[i * 4 + 3] - at);
            }
            return std::sqrt(d);
        };
        const bool any = Frames(survey + "\\M249ACOG_reload.bin", [&](const std::vector<Record>& group) {
            ++frames;
            for (const Record& r : group) {
                const bool body = IsBody(r);
                const bool depthPass = r.count == static_cast<int>(m249rows::kDepth) && r.start == static_cast<int>(m249rows::kDepth) && r.Bones() > m249rows::kDepthFrame;
                if (body || depthPass) {
                    InvertRigid(r.Bone(body ? m249rows::kFrameBone : m249rows::kDepthFrame), frameInverse);
                    Compose(frameInverse, r.Bone(body ? m249rows::kCoverBone : m249rows::kDepthCover), framedCover);
                    if (body) {
                        std::memcpy(lidOn, framedCover, sizeof(lidOn));
                        haveLid = true;
                        if (!haveRest)
                            std::memcpy(coverRest, framedCover, sizeof(coverRest)), haveRest = true;
                    }
                    haveFrame = true;
                    laterView = false;
                    continue;
                }
                if (r.count == kShadowCount && r.start == kShadowStart)
                    laterView = true;
                if (!haveFrame || !haveLid)
                    continue;
                const int which = AcogDrawOf(r);
                if (which == 0 || which == 1)
                    sightFrame = r.frame;
                // skin.cpp's HeldSightsOwn: its glass, chevron or fibre, its body or depth pass drawn in the last two frames.
                const bool sightsOwn = which >= 2 && r.frame - sightFrame <= 2;
                const bool tryIt = lidride::Tried(!laterView, false, sightsOwn);
                if (which < 0) {
                    if (!laterView || GunsOwn(r))
                        continue;
                    bool onMount = false;
                    for (int k = 0; k < std::min(r.Bones(), 85) && !onMount; ++k) {
                        float on[12];
                        Compose(frameInverse, r.Bone(k), on);
                        onMount = lidride::OnMount(belt, lidOn, on);
                    }
                    if (onMount) {
                        std::vector<int>& seenIn = strays[{r.count, r.start}];
                        if (seenIn.empty() || seenIn.back() != r.frame)
                            seenIn.push_back(r.frame);
                        straysTried += tryIt;
                    }
                    continue;
                }
                float on[12];
                Compose(frameInverse, r.Bone(0), on);
                const bool taken = tryIt && lidride::OnMount(belt, lidOn, on);
                const float off = mountOff(lidOn, on);
                ++records[which];
                later[which] += laterView;
                tried[which] += tryIt;
                byView[which] += lidride::Tried(!laterView, false, false);
                took[which] += taken;
                worst[which] = std::max(worst[which], off);
                if (lidride::Rides(coverRest, lidOn) && lidride::Rides(coverRest, framedCover)) {
                    ++restRecords[which];
                    restTook[which] += taken;
                    restWorst[which] = std::max(restWorst[which], off);
                }
            }
        });
        if (!any || !frames) {
            Skip("a7 no recording of the game's own reload with it on (build\\survey\\M249ACOG_reload.bin): its draws framed as DrawHeldPart frames them");
        } else {
            std::snprintf(what, sizeof(what),
                          "a7 its glass, chevron and fibre drawn after the player's shadow since the last body or depth pass in every record (%ld %ld %ld %ld of %ld %ld %ld %ld): after the "
                          "see-through pass's own first-person view, never in the body's; its body and depth pass never (%ld %ld of %ld %ld): in the body's view",
                          later[2], later[3], later[4], later[5], records[2], records[3], records[4], records[5], later[0], later[1], records[0], records[1]);
            bool ok = later[0] == 0 && later[1] == 0 && records[0] >= 2 * frames - 1 && records[1] >= 2 * frames - 1;
            for (int i = 2; i < 6; ++i)
                ok = ok && records[i] >= 2 * frames - 1 && later[i] == records[i];
            Check(ok, what);
            std::snprintf(what, sizeof(what),
                          "a7 lidride::Tried tries every record of the six (%ld %ld %ld %ld %ld %ld): its body and depth pass by the view, its glass, chevron and fibre as a held sight's own; "
                          "by the view alone (as dd042df had it) none of those four (%ld %ld %ld %ld): they stayed shut while the body went up",
                          tried[0], tried[1], tried[2], tried[3], tried[4], tried[5], byView[2], byView[3], byView[4], byView[5]);
            ok = true;
            for (int i = 0; i < 6; ++i)
                ok = ok && tried[i] == records[i] && (i < 2 || byView[i] == 0);
            Check(ok, what);
            float restMost = 0.0f, seeMost = 0.0f;
            ok = true;
            for (int i = 0; i < 6; ++i) {
                restMost = std::max(restMost, restWorst[i]);
                ok = ok && restRecords[i] >= frames / 2 && restTook[i] == restRecords[i];
                if (i >= 2) {
                    seeMost = std::max(seeMost, worst[i]);
                    ok = ok && took[i] == records[i];
                }
            }
            std::snprintf(what, sizeof(what),
                          "a7 framed as DrawHeldPart frames them, the game's cover at rest (%ld %ld %ld %ld %ld %ld records of %ld frames): every one tried and on the mount (%ld %ld %ld %ld %ld %ld), "
                          "%.4f mm off at most: lifted by hand, all six go up with the cover; its glass, chevron and fibre all through the reload too, by the same eye's body (%ld %ld %ld %ld, %.4f mm)",
                          restRecords[0], restRecords[1], restRecords[2], restRecords[3], restRecords[4], restRecords[5], frames, restTook[0], restTook[1], restTook[2], restTook[3], restTook[4],
                          restTook[5], restMost * 1000.0f, took[2], took[3], took[4], took[5], seeMost * 1000.0f);
            Check(ok && restMost < lidride::kSamePlace, what);
            std::string list;
            size_t mostFrames = 0;
            for (const auto& [key, seenIn] : strays) {
                list += (list.empty() ? "" : ", ") + std::to_string(key.first) + "/" + std::to_string(key.second) + " (" + std::to_string(seenIn.size()) + ")";
                mostFrames = std::max(mostFrames, seenIn.size());
            }
            std::snprintf(what, sizeof(what), "a7 other draws after the shadow with a bone on the mount: %s, at most %zu frames of %ld; none a held sight's own, none tried (%ld): as the game draws them",
                          list.empty() ? "none" : list.c_str(), mostFrames, frames, straysTried);
            Check(straysTried == 0, what);
        }
    }

    std::printf("%d checks, %d failed, %d skipped\n", g_checks, g_failed, g_skipped);
    return g_failed;
}

// The handles drawn apart with the hand (src/handleruns.h, header only, as skin.cpp's DrawHeldPart draws them: the Mk11's T handle, which has no bone of its
// own), held to the mesh dump they were read from (build\mesh_12456_2214_*: the Mk11 Mod 0's colour draw, whose buffers hold its depth pass too), without
// the game:
//   h1  the table: each row's runs in order and apart inside its passes' draws, its stem's top a run of two triangles inside one of its runs, its box the
//       right way round; For() finds each row by its gun and no other held gun (skin.cpp's kHeldKinds' body counts, copied)
//   h2  Gaps: each pass's draw less its runs, with its runs, is the draw, every index once, in order; with a cover's run in it too (the M16A4's way); a run
//       outside the draw, overlapping another or empty refused (-1: drawn whole, as before)
//   h3  the shaft's faces (ShaftFace): for pulls of 1 mm to 8 cm each takes the stem's top onto its own side of the box, from the pulled stem's front back
//       home; the top's normal (0 1 0) onto the face's outward one at unit length; its winding kept (a determinant above 0); its tangent along z still along
//       z; neighbouring faces meet along their edges (a closed box); nothing without a pull
//   h4  the colour pass (SKIP without the dump): every index of the runs on the frame's bone alone, inside the T's box; none of their corners shared with the
//       rest of the body (no crack where it comes apart); each part of the body the runs touch wholly in them, and no part of the body lying wholly inside
//       the T's box left out of them; the hand's place on the lever (skin.cpp's kMk11Mag, copied) on the T
//   h5  the depth pass (SKIP without the dump): its vertices after the colour pass's, 16 bytes each, as many as its indices name; its runs' triangles the
//       colour runs' by their corners, each once and the same way round; nothing else of the depth pass on the handle's corners; all on its bone 0 alone
//   h6  the shaft (SKIP without the dump): the stem's top two triangles in both passes, the same corners the same way round, facing up, its corners the row's
//       box; their vertices' normals (0 1 0) and tangents along z (the stretch keeps both); the stem's last slice inside the box; the faces made from the
//       real triangles: the box from the pulled stem home, its back end the pulled stem's front
// Usage: handleruntest [<folder with the mesh dumps>] (build by default, from the project's folder). Reads only.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "../src/handleruns.h"

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

// ---- skin.cpp's rows, copied ----
// kHeldKinds' body counts (the colour draw's indices), each held gun's: the RPK-74M, AN-94, AEK-971, AK-74M, G3A3, KH2002, M16A4, M4A1, M240, the Stinger,
// AKS-74u, UMP-45, M1014, Mk11 Mod 0, M249.
constexpr uint32_t kHeldBodies[] = {24492, 16188, 21585, 21723, 23064, 15237, 17013, 22194, 27588, 20298, 17991, 19314, 13377, 12456, 24291};
// The Mk11's: its colour draw and its depth pass (kHeldKinds), its frame bone in both; its lever's knob where the hand takes it, its travel (kMk11Mag).
constexpr uint32_t kMk11Body = 12456, kMk11BodyStart = 2214, kMk11Depth = 14670;
constexpr int kMk11Frame = 0;
constexpr float kMk11Lever[3] = {0.0f, 0.051f, 0.274f}, kMk11Travel = 0.08f;
// The M16A4's cover, a run of its body (kM16Bones): to cut along with a handle's.
constexpr handleruns::Run kM16Cover = {2949, 204};
// The T handle's box on the frame (docs/research/mk11.md: its ears to x +-2 cm, y 0.046 to 0.0565, z 0.269 to 0.278; the latch on to z 0.2878), 0.5 mm wider.
constexpr float kTLo[3] = {-0.0205f, 0.0457f, 0.2685f}, kTHi[3] = {0.0205f, 0.0570f, 0.2885f};

float Half(uint16_t h) {
    const int s = (h >> 15) & 1, e = (h >> 10) & 0x1F, m = h & 0x3FF;
    const float v = e == 0 ? std::ldexp(m / 1024.0f, -14) : std::ldexp(1 + m / 1024.0f, e - 15);
    return s ? -v : v;
}
std::vector<uint8_t> ReadAll(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
bool Inside(const float* p, const float* lo, const float* hi) {
    return p[0] >= lo[0] && p[0] <= hi[0] && p[1] >= lo[1] && p[1] <= hi[1] && p[2] >= lo[2] && p[2] <= hi[2];
}
void Apply(const float* m, const float* p, float* o) {
    float r[3];
    for (int k = 0; k < 3; ++k)
        r[k] = m[k * 4] * p[0] + m[k * 4 + 1] * p[1] + m[k * 4 + 2] * p[2] + m[k * 4 + 3];
    std::memcpy(o, r, sizeof(r));
}
void Turn(const float* m, const float* v, float* o) {  // the 3x3 alone
    for (int k = 0; k < 3; ++k)
        o[k] = m[k * 4] * v[0] + m[k * 4 + 1] * v[1] + m[k * 4 + 2] * v[2];
}
float Det(const float* m) {
    return m[0] * (m[5] * m[10] - m[6] * m[9]) - m[1] * (m[4] * m[10] - m[6] * m[8]) + m[2] * (m[4] * m[9] - m[5] * m[8]);
}
void Cross(const float* a, const float* b, float* o) {
    o[0] = a[1] * b[2] - a[2] * b[1], o[1] = a[2] * b[0] - a[0] * b[2], o[2] = a[0] * b[1] - a[1] * b[0];
}
float Length(const float* v) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }

// The dump's buffers: the colour draw's vertex at the offset its draw bound (stride 48: a half position at 0, four bones at 8, four weights at 12, the
// normal's four halves at 16, the tangent's at 24); the depth pass's after them (stride 16: a half position at 0, four bones at 8, four weights at 12).
struct Dump {
    bool read = false;
    std::vector<uint8_t> vb;
    std::vector<uint16_t> ib;
    size_t colourAt = 0, depthAt = 0;
    int colourVertices = 0, depthVertices = 0;
    using Key = std::tuple<uint16_t, uint16_t, uint16_t>;  // a corner by its position's bits
    Key ColourKey(uint16_t v) const {
        uint16_t h[3];
        std::memcpy(h, &vb[colourAt + v * 48u], 6);
        return {h[0], h[1], h[2]};
    }
    Key DepthKey(uint16_t v) const {
        uint16_t h[3];
        std::memcpy(h, &vb[depthAt + v * 16u], 6);
        return {h[0], h[1], h[2]};
    }
    void ColourAt(uint16_t v, float* p) const {
        uint16_t h[3];
        std::memcpy(h, &vb[colourAt + v * 48u], 6);
        p[0] = Half(h[0]), p[1] = Half(h[1]), p[2] = Half(h[2]);
    }
    void DepthAt(uint16_t v, float* p) const {
        uint16_t h[3];
        std::memcpy(h, &vb[depthAt + v * 16u], 6);
        p[0] = Half(h[0]), p[1] = Half(h[1]), p[2] = Half(h[2]);
    }
    void ColourHalves(uint16_t v, size_t at, float* q) const {  // four halves at that byte of the vertex
        uint16_t h[4];
        std::memcpy(h, &vb[colourAt + v * 48u + at], 8);
        for (int k = 0; k < 4; ++k)
            q[k] = Half(h[k]);
    }
    // On that bone alone: all four bones it, the weight on the last slot whole (the game's way for a rigid part).
    bool ColourOnBone(uint16_t v, int bone) const {
        const uint8_t* o = &vb[colourAt + v * 48u];
        return o[8] == bone && o[9] == bone && o[10] == bone && o[11] == bone && o[12] + o[13] + o[14] == 0 && o[15] == 255;
    }
    bool DepthOnBone(uint16_t v, int bone) const {
        const uint8_t* o = &vb[depthAt + v * 16u];
        return o[8] == bone && o[9] == bone && o[10] == bone && o[11] == bone && o[12] + o[13] + o[14] == 0 && o[15] == 255;
    }
};
Dump Load(const std::string& dir) {
    Dump d;
    const std::string name = dir + "\\mesh_" + std::to_string(kMk11Body) + "_" + std::to_string(kMk11BodyStart) + "_";
    d.vb = ReadAll(name + "vb0.bin");
    const std::vector<uint8_t> ib = ReadAll(name + "ib.bin");
    std::ifstream text(name + "dump.txt");
    std::string word;
    unsigned stride = 0, offset = 0;
    while (text >> word)
        if (word == "vb") {
            int slot = -1;
            std::string a, b;
            if (text >> slot >> a >> stride >> b >> offset && slot == 0)
                break;
            stride = 0;
        }
    if (d.vb.empty() || ib.size() < (kMk11Depth + kMk11Depth) * 2u || stride != 48)
        return d;
    d.ib.resize(ib.size() / 2);
    std::memcpy(d.ib.data(), ib.data(), d.ib.size() * 2);
    d.colourAt = offset;
    int most = -1;
    for (uint32_t i = kMk11BodyStart; i < kMk11BodyStart + kMk11Body; ++i)
        most = std::max(most, static_cast<int>(d.ib[i]));
    d.colourVertices = most + 1;
    d.depthAt = d.colourAt + static_cast<size_t>(d.colourVertices) * 48u;
    most = -1;
    for (uint32_t i = kMk11Depth; i < 2 * kMk11Depth; ++i)
        most = std::max(most, static_cast<int>(d.ib[i]));
    d.depthVertices = most + 1;
    d.read = d.colourAt + static_cast<size_t>(d.colourVertices) * 48u <= d.vb.size();
    return d;
}

bool InRuns(uint32_t i, const handleruns::Pass& pass) {
    for (int r = 0; r < pass.runCount; ++r)
        if (i >= pass.runs[r].start && i < pass.runs[r].start + pass.runs[r].count)
            return true;
    return false;
}

// A triangle by its three corners' keys, starting from the least (its winding kept).
using Tri = std::tuple<Dump::Key, Dump::Key, Dump::Key>;
Tri Turned(const Dump::Key& a, const Dump::Key& b, const Dump::Key& c) {
    if (b < a && b < c)
        return {b, c, a};
    if (c < a && c < b)
        return {c, a, b};
    return {a, b, c};
}

}  // namespace

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? argv[1] : "build";
    char what[900];
    const handleruns::Handle* mk11 = handleruns::For(kMk11Body);

    // ---- h1: the table ----
    for (const handleruns::Handle& h : handleruns::kHandles) {
        bool ok = h.name && h.kind && handleruns::For(h.kind) == &h;
        for (const handleruns::Pass* pass : {&h.colour, &h.depth}) {
            ok = ok && pass->runCount >= 1 && pass->runCount <= handleruns::kMaxRuns && pass->drawCount;
            for (int r = 0; r < pass->runCount; ++r) {
                const handleruns::Run& run = pass->runs[r];
                ok = ok && run.count && run.count % 3 == 0 && run.start % 3 == pass->drawStart % 3 && run.start >= pass->drawStart &&
                     run.start + run.count <= pass->drawStart + pass->drawCount && (r == 0 || run.start >= pass->runs[r - 1].start + pass->runs[r - 1].count);
            }
            bool inRun = false;
            for (int r = 0; r < pass->runCount; ++r)
                inRun = inRun || (pass->shaftTop.start >= pass->runs[r].start && pass->shaftTop.start + pass->shaftTop.count <= pass->runs[r].start + pass->runs[r].count);
            ok = ok && (pass->shaftTop.count == 0 || (pass->shaftTop.count == 6 && inRun));
        }
        ok = ok && h.right < h.left && h.bottom < h.top && h.back < h.front && h.left - h.right < 0.05f && h.top - h.bottom < 0.05f && h.front - h.back < 0.05f;
        std::snprintf(what, sizeof(what), "h1 %s (%u): runs in order and apart inside its passes' draws (colour %u from %u, depth %u from %u), its stem's top a run of two triangles "
                      "inside one, its box (%.4f %.4f) (%.4f %.4f) (%.4f %.4f) the right way round",
                      h.name ? h.name : "?", h.kind, h.colour.drawCount, h.colour.drawStart, h.depth.drawCount, h.depth.drawStart, h.right, h.left, h.bottom, h.top, h.back, h.front);
        Check(ok, what);
    }
    {
        int found = 0, others = 0;
        for (const uint32_t body : kHeldBodies)
            (handleruns::For(body) ? found : others) += 1;
        std::snprintf(what, sizeof(what), "h1 For(): the Mk11's row by its body (%u), no row for the other %d held guns (every one drawn as before), none for 0", kMk11Body, others);
        Check(mk11 && mk11->kind == kMk11Body && found == 1 && others == static_cast<int>(std::size(kHeldBodies)) - 1 && !handleruns::For(0), what);
    }
    if (!mk11) {
        std::printf("%d checks, %d failed, %d skipped\n", g_checks, g_failed + 1, g_skipped);
        return 1;
    }
    std::snprintf(what, sizeof(what), "h1 the Mk11's: colour 12456 from 2214 (kHeldKinds), 834 from 5499; depth 14670 from 14670, 654 from 19068 and 180 from 29004; its stem's top 6 from 6072 "
                  "and from 19101");
    Check(mk11->colour.drawStart == kMk11BodyStart && mk11->colour.drawCount == kMk11Body && mk11->depth.drawStart == kMk11Depth && mk11->depth.drawCount == kMk11Depth &&
              mk11->colour.runCount == 1 && mk11->colour.runs[0].start == 5499 && mk11->colour.runs[0].count == 834 && mk11->depth.runCount == 2 &&
              mk11->depth.runs[0].start == 19068 && mk11->depth.runs[0].count == 654 && mk11->depth.runs[1].start == 29004 && mk11->depth.runs[1].count == 180 &&
              mk11->colour.shaftTop.start == 6072 && mk11->colour.shaftTop.count == 6 && mk11->depth.shaftTop.start == 19101 && mk11->depth.shaftTop.count == 6,
          what);

    // ---- h2: Gaps ----
    {
        bool ok = true;
        for (const handleruns::Pass* pass : {&mk11->colour, &mk11->depth}) {
            handleruns::Run gaps[handleruns::kMaxRuns + 2];
            const int n = handleruns::Gaps(pass->drawStart, pass->drawCount, pass->runs, pass->runCount, gaps);
            std::vector<int> seen(pass->drawCount, 0);
            ok = ok && n >= 1 && n <= pass->runCount + 1;
            uint32_t was = 0;
            for (int i = 0; ok && i < n; ++i) {
                ok = gaps[i].count && gaps[i].start >= pass->drawStart && gaps[i].start + gaps[i].count <= pass->drawStart + pass->drawCount && gaps[i].start >= was;
                was = gaps[i].start + gaps[i].count;
                for (uint32_t k = gaps[i].start; ok && k < gaps[i].start + gaps[i].count; ++k)
                    ++seen[k - pass->drawStart];
            }
            for (int r = 0; ok && r < pass->runCount; ++r)
                for (uint32_t k = pass->runs[r].start; k < pass->runs[r].start + pass->runs[r].count; ++k)
                    ++seen[k - pass->drawStart];
            ok = ok && std::all_of(seen.begin(), seen.end(), [](int s) { return s == 1; });
        }
        Check(ok, "h2 each pass's draw less its runs, with its runs, is the draw: every index once, the pieces in order (colour: 2 pieces round 1 run; depth: 3 round 2)");
    }
    {
        // With a cover's run among them, given out of order; the colour draw of 12456 from 2214 standing for any.
        const handleruns::Run cuts[3] = {{6153, 180}, {2949, 204}, {5499, 654}};
        handleruns::Run gaps[handleruns::kMaxRuns + 2];
        const int n = handleruns::Gaps(2214, 12456, cuts, 3, gaps);
        const bool ok = n == 3 && gaps[0].start == 2214 && gaps[0].count == 2949 - 2214 && gaps[1].start == 2949 + 204 && gaps[1].count == 5499 - (2949 + 204) &&
                        gaps[2].start == 6333 && gaps[2].count == 14670 - 6333;
        std::snprintf(what, sizeof(what), "h2 with a cover's run too (the M16A4's way, %u from %u), and the runs given out of order: the pieces between them, in order (%d)", kM16Cover.count,
                      kM16Cover.start, n);
        Check(ok, what);
    }
    {
        handleruns::Run gaps[handleruns::kMaxRuns + 2];
        const handleruns::Run outside[1] = {{14667, 6}}, overlapping[2] = {{5499, 834}, {6000, 30}}, empty[1] = {{5499, 0}}, before[1] = {{2211, 6}}, whole[1] = {{2214, 12456}};
        const bool ok = handleruns::Gaps(2214, 12456, outside, 1, gaps) == -1 && handleruns::Gaps(2214, 12456, overlapping, 2, gaps) == -1 &&
                        handleruns::Gaps(2214, 12456, empty, 1, gaps) == -1 && handleruns::Gaps(2214, 12456, before, 1, gaps) == -1 &&
                        handleruns::Gaps(2214, 12456, whole, 1, gaps) == 0 && handleruns::Gaps(2214, 12456, whole, 0, gaps) == 1 && gaps[0].start == 2214 && gaps[0].count == 12456 &&
                        handleruns::Gaps(2214, 12456, whole, handleruns::kMaxRuns + 2, gaps) == -1;
        Check(ok, "h2 a run past the draw's end or before its start, overlapping another or empty: refused (-1, the draw drawn whole); all of it: nothing left; none: the draw itself");
    }

    // ---- h3: the shaft's faces ----
    {
        const handleruns::Handle& h = *mk11;
        const float outward[4][3] = {{0, 1, 0}, {0, -1, 0}, {-1, 0, 0}, {1, 0, 0}};  // top, underside, right (x right, the gun's right: -x), left
        bool placed = true, normals = true, winding = true, tangents = true, closed = true;
        float worst = 0.0f;
        for (const float pull : {0.001f, 0.01f, 0.04f, kMk11Travel}) {
            float m[4][12];
            for (int f = 0; f < 4; ++f)
                placed = handleruns::ShaftFace(h, f, pull, m[f]) && placed;
            if (!placed)
                break;
            // The top's four corners (x right/left, y top, z back/front) onto each face.
            const float corners[4][3] = {{h.right, h.top, h.back}, {h.left, h.top, h.back}, {h.right, h.top, h.front}, {h.left, h.top, h.front}};
            for (int f = 0; f < 4; ++f) {
                float lo[3] = {1e9f, 1e9f, 1e9f}, hi[3] = {-1e9f, -1e9f, -1e9f};
                for (const auto& c : corners) {
                    float p[3];
                    Apply(m[f], c, p);
                    for (int k = 0; k < 3; ++k)
                        lo[k] = std::min(lo[k], p[k]), hi[k] = std::max(hi[k], p[k]);
                    // The top's back edge to the pulled stem's front, its front edge home.
                    const float zWant = c[2] == h.back ? h.front - pull : h.front;
                    worst = std::max(worst, std::fabs(p[2] - zWant));
                }
                // Its side of the box, across it all.
                const float side[4][4] = {{h.right, h.left, h.top, h.top}, {h.right, h.left, h.bottom, h.bottom}, {h.right, h.right, h.bottom, h.top}, {h.left, h.left, h.bottom, h.top}};
                for (int k = 0; k < 2; ++k)
                    worst = std::max({worst, std::fabs(lo[k] - side[f][2 * k]), std::fabs(hi[k] - side[f][2 * k + 1])});
                float n[3], t[3];
                const float up[3] = {0, 1, 0}, along[3] = {0, 0, 1};
                Turn(m[f], up, n);
                Turn(m[f], along, t);
                normals = normals && std::fabs(n[0] - outward[f][0]) < 1e-6f && std::fabs(n[1] - outward[f][1]) < 1e-6f && std::fabs(n[2] - outward[f][2]) < 1e-6f;
                winding = winding && Det(m[f]) > 0.0f;
                tangents = tangents && std::fabs(t[0]) < 1e-9f && std::fabs(t[1]) < 1e-9f && t[2] > 0.0f;
            }
            // Neighbouring faces' edges: the top's right edge is the right side's top edge, and so round.
            const auto edge = [&](int f, float x, float y, float* a, float* b) {
                const float pa[3] = {x, h.top, h.back}, pb[3] = {x, h.top, h.front};
                Apply(m[f], pa, a), Apply(m[f], pb, b);
                (void)y;
            };
            const int pairs[4][2] = {{0, 2}, {0, 3}, {1, 2}, {1, 3}};
            for (const auto& pr : pairs) {
                // Each face's edges are the images of the top's two edges (x right, x left): one of the first face's meets one of the second's.
                float a0[3], a1[3], b0[3], b1[3], c0[3], c1[3], d0[3], d1[3];
                edge(pr[0], h.right, 0, a0, a1), edge(pr[0], h.left, 0, b0, b1);
                edge(pr[1], h.right, 0, c0, c1), edge(pr[1], h.left, 0, d0, d1);
                const auto gap = [](const float* p, const float* q, const float* r, const float* s) {
                    return std::max(std::sqrt((p[0] - r[0]) * (p[0] - r[0]) + (p[1] - r[1]) * (p[1] - r[1]) + (p[2] - r[2]) * (p[2] - r[2])),
                                    std::sqrt((q[0] - s[0]) * (q[0] - s[0]) + (q[1] - s[1]) * (q[1] - s[1]) + (q[2] - s[2]) * (q[2] - s[2])));
                };
                const float best = std::min({gap(a0, a1, c0, c1), gap(a0, a1, d0, d1), gap(b0, b1, c0, c1), gap(b0, b1, d0, d1)});
                closed = closed && best < 1e-6f;
            }
        }
        float none[12];
        const bool nothing = !handleruns::ShaftFace(h, 0, 0.0f, none) && !handleruns::ShaftFace(h, 0, -0.01f, none) && !handleruns::ShaftFace(h, 4, 0.04f, none) &&
                             !handleruns::ShaftFace(h, -1, 0.04f, none) && !handleruns::ShaftFace(h, 0, std::nanf(""), none);
        std::snprintf(what, sizeof(what), "h3 the shaft's four faces, pulled 1 mm, 1 cm, 4 cm and 8 cm: each the stem's top taken onto its side of the box from the pulled stem's front home (worst %.2g m)",
                      worst);
        Check(placed && worst < 2e-6f, what);
        Check(normals, "h3 ... the top's normal (0 1 0) taken onto each face's outward one, at unit length (top up, underside down, right side -x, left side +x)");
        Check(winding, "h3 ... each keeps the top's winding (its determinant above 0): no face turned inside out, culled as the game culls the top");
        Check(tangents, "h3 ... the top's tangent along z still along z (the stretch scales it, its direction kept)");
        Check(closed, "h3 ... neighbouring faces meet along their edges: a closed box, no slit between them");
        Check(nothing, "h3 ... nothing with no pull, a pull back the other way, no number, or a face it hasn't");
    }

    // ---- h4 to h6: the mesh ----
    const Dump d = Load(dir);
    if (!d.read) {
        Skip("h4 the colour pass: no dump of the Mk11's colour draw (mesh_12456_2214_*)");
        Skip("h5 the depth pass: likewise");
        Skip("h6 the shaft: likewise");
    } else {
        const handleruns::Pass& colour = mk11->colour;
        const handleruns::Pass& depth = mk11->depth;
        // ---- h4: the colour pass ----
        std::set<Dump::Key> handleCorners, restCorners;
        bool onFrame = true, inBox = true;
        int runIndices = 0;
        float lo[3] = {1e9f, 1e9f, 1e9f}, hi[3] = {-1e9f, -1e9f, -1e9f};
        for (uint32_t i = colour.drawStart; i < colour.drawStart + colour.drawCount; ++i) {
            const uint16_t v = d.ib[i];
            if (InRuns(i, colour)) {
                ++runIndices;
                handleCorners.insert(d.ColourKey(v));
                onFrame = onFrame && d.ColourOnBone(v, kMk11Frame);
                float p[3];
                d.ColourAt(v, p);
                inBox = inBox && Inside(p, kTLo, kTHi);
                for (int k = 0; k < 3; ++k)
                    lo[k] = std::min(lo[k], p[k]), hi[k] = std::max(hi[k], p[k]);
            } else {
                restCorners.insert(d.ColourKey(v));
            }
        }
        std::snprintf(what, sizeof(what), "h4 the colour runs: %d indices, every one on the frame's bone alone (bone %d, all its weight), inside the T's box: x %.4f to %.4f, y %.4f to %.4f, "
                      "z %.4f to %.4f", runIndices, kMk11Frame, lo[0], hi[0], lo[1], hi[1], lo[2], hi[2]);
        Check(runIndices == 834 && onFrame && inBox, what);
        int shared = 0;
        for (const Dump::Key& k : handleCorners)
            shared += restCorners.count(k) ? 1 : 0;
        std::snprintf(what, sizeof(what), "h4 ... none of its %zu corners shared with the rest of the body (%d shared): nothing cracks where it comes apart", handleCorners.size(), shared);
        Check(!handleCorners.empty() && shared == 0, what);
        // The body's parts: joined by a shared vertex or a shared corner's place.
        {
            const int n = d.colourVertices;
            std::vector<int> parent(static_cast<size_t>(n));
            for (int v = 0; v < n; ++v)
                parent[v] = v;
            const auto find = [&](int v) {
                while (parent[v] != v)
                    v = parent[v] = parent[parent[v]];
                return v;
            };
            const auto join = [&](int a, int b) { parent[find(a)] = find(b); };
            std::map<Dump::Key, int> firstAt;
            for (uint32_t i = colour.drawStart; i + 2 < colour.drawStart + colour.drawCount; i += 3) {
                join(d.ib[i], d.ib[i + 1]), join(d.ib[i + 1], d.ib[i + 2]);
                for (int c = 0; c < 3; ++c) {
                    const auto [at, fresh] = firstAt.emplace(d.ColourKey(d.ib[i + c]), d.ib[i + c]);
                    if (!fresh)
                        join(at->second, d.ib[i + c]);
                }
            }
            // Each part: its triangles in the runs and out of them, and whether all of its corners lie in the T's box.
            std::map<int, std::tuple<int, int, bool>> parts;
            for (uint32_t i = colour.drawStart; i + 2 < colour.drawStart + colour.drawCount; i += 3) {
                auto& [in, out, boxed] = parts.try_emplace(find(d.ib[i]), 0, 0, true).first->second;
                (InRuns(i, colour) ? in : out) += 1;
                for (int c = 0; c < 3; ++c) {
                    float p[3];
                    d.ColourAt(d.ib[i + c], p);
                    boxed = boxed && Inside(p, kTLo, kTHi);
                }
            }
            int touched = 0, split = 0, leftOut = 0;
            for (const auto& [root, part] : parts) {
                const auto [in, out, boxed] = part;
                touched += in > 0;
                split += in > 0 && out > 0;
                leftOut += out > 0 && boxed;
            }
            std::snprintf(what, sizeof(what), "h4 ... the body's %zu parts: %d in the runs (the T with its stem, the latch), each wholly (%d split); none lying wholly in the T's box left out (%d)",
                          parts.size(), touched, split, leftOut);
            Check(touched == 2 && split == 0 && leftOut == 0, what);
        }
        Check(Inside(kMk11Lever, kTLo, kTHi), "h4 the hand's place on the lever (kMk11Mag's lever, (0 0.051 0.274)) on the T: the hand stays on what it draws back");

        // ---- h5: the depth pass ----
        const size_t depthEnd = d.depthAt + static_cast<size_t>(d.depthVertices) * 16u;
        const bool depthRead = depthEnd == d.vb.size();
        std::snprintf(what, sizeof(what), "h5 the depth pass's vertices after the colour's (%d of 48 bytes from %zu): %d of 16 bytes from %zu, the buffer's end (%zu bytes)", d.colourVertices,
                      d.colourAt, d.depthVertices, d.depthAt, d.vb.size());
        Check(depthRead, what);
        if (depthRead) {
            std::multiset<Tri> colourTris, depthTris;
            for (int r = 0; r < colour.runCount; ++r)
                for (uint32_t i = colour.runs[r].start; i + 2 < colour.runs[r].start + colour.runs[r].count; i += 3)
                    colourTris.insert(Turned(d.ColourKey(d.ib[i]), d.ColourKey(d.ib[i + 1]), d.ColourKey(d.ib[i + 2])));
            bool onBone = true;
            int strays = 0;
            for (uint32_t i = depth.drawStart; i + 2 < depth.drawStart + depth.drawCount; i += 3) {
                const Tri t = Turned(d.DepthKey(d.ib[i]), d.DepthKey(d.ib[i + 1]), d.DepthKey(d.ib[i + 2]));
                if (InRuns(i, depth)) {
                    depthTris.insert(t);
                    for (int c = 0; c < 3; ++c)
                        onBone = onBone && d.DepthOnBone(d.ib[i + c], 0);
                } else {
                    for (int c = 0; c < 3; ++c)
                        strays += handleCorners.count(d.DepthKey(d.ib[i + c])) ? 1 : 0;
                }
            }
            std::snprintf(what, sizeof(what), "h5 its runs' %zu triangles the colour runs' %zu by their corners, each once and the same way round", depthTris.size(), colourTris.size());
            Check(!colourTris.empty() && depthTris == colourTris, what);
            std::snprintf(what, sizeof(what), "h5 ... nothing else of the depth pass on the handle's corners (%d corners): nothing of it left at home", strays);
            Check(strays == 0, what);
            Check(onBone, "h5 ... every index of them on its bone 0 alone, the frame's in this pass (kHeldKinds: no depth frame of its own)");
        } else {
            Skip("h5 its runs against the colour's: the depth pass's vertices not where they were looked for");
        }

        // ---- h6: the shaft ----
        {
            const handleruns::Handle& h = *mk11;
            const handleruns::Run top = colour.shaftTop, depthTop = depth.shaftTop;
            std::multiset<Tri> colourTop, depthTopTris;
            bool corners = true, faceUp = true, shading = true;
            for (uint32_t i = top.start; i + 2 < top.start + top.count; i += 3) {
                float p[3][3];
                for (int c = 0; c < 3; ++c) {
                    d.ColourAt(d.ib[i + c], p[c]);
                    corners = corners && (p[c][0] == h.right || p[c][0] == h.left) && p[c][1] == h.top && (p[c][2] == h.back || p[c][2] == h.front);
                    float n[4], t[4];
                    d.ColourHalves(d.ib[i + c], 16, n);
                    d.ColourHalves(d.ib[i + c], 24, t);
                    shading = shading && std::fabs(n[0]) < 0.01f && std::fabs(n[1] - 1.0f) < 0.01f && std::fabs(n[2]) < 0.01f && std::fabs(t[0]) < 0.01f && std::fabs(t[1]) < 0.01f &&
                              std::fabs(t[2] - 1.0f) < 0.01f && t[3] == 1.0f;
                }
                const float e1[3] = {p[1][0] - p[0][0], p[1][1] - p[0][1], p[1][2] - p[0][2]}, e2[3] = {p[2][0] - p[0][0], p[2][1] - p[0][1], p[2][2] - p[0][2]};
                float n[3];
                Cross(e1, e2, n);
                faceUp = faceUp && n[1] > 0.0f && std::fabs(n[0]) < 1e-9f && std::fabs(n[2]) < 1e-9f;
                colourTop.insert(Turned(d.ColourKey(d.ib[i]), d.ColourKey(d.ib[i + 1]), d.ColourKey(d.ib[i + 2])));
            }
            for (uint32_t i = depthTop.start; depthRead && i + 2 < depthTop.start + depthTop.count; i += 3)
                depthTopTris.insert(Turned(d.DepthKey(d.ib[i]), d.DepthKey(d.ib[i + 1]), d.DepthKey(d.ib[i + 2])));
            // Its area: the whole rectangle, the two triangles not one over the other.
            float area = 0.0f;
            for (uint32_t i = top.start; i + 2 < top.start + top.count; i += 3) {
                float p[3][3];
                for (int c = 0; c < 3; ++c)
                    d.ColourAt(d.ib[i + c], p[c]);
                const float e1[3] = {p[1][0] - p[0][0], p[1][1] - p[0][1], p[1][2] - p[0][2]}, e2[3] = {p[2][0] - p[0][0], p[2][1] - p[0][1], p[2][2] - p[0][2]};
                float n[3];
                Cross(e1, e2, n);
                area += 0.5f * Length(n);
            }
            const float want = (h.left - h.right) * (h.front - h.back);
            std::snprintf(what, sizeof(what), "h6 the stem's top: two triangles, corners on the row's box (x %.4f and %.4f, y %.4f, z %.4f and %.4f), facing up, the whole rectangle (%.3g of %.3g m2)",
                          h.right, h.left, h.top, h.back, h.front, area, want);
            Check(top.count == 6 && corners && faceUp && std::fabs(area - want) < 1e-9f, what);
            Check(shading, "h6 ... its vertices' normals (0 1 0) and tangents along z (handedness 1): square to z and along it, the stretch keeps both directions");
            if (depthRead)
                Check(depthTop.count == 6 && !colourTop.empty() && colourTop == depthTopTris, "h6 ... the depth pass's two the same corners, the same way round");
            else
                Skip("h6 ... the depth pass's two: its vertices not where they were looked for");
            // The stem's last slice (the T's vertices from z back to z front) inside the box: the shaft is the stem's own cross-section.
            float worst = 0.0f;
            int slice = 0;
            for (uint32_t i = colour.runs[0].start; i < colour.runs[0].start + colour.runs[0].count; ++i) {
                float p[3];
                d.ColourAt(d.ib[i], p);
                if (p[2] < h.back || p[2] > h.front || p[0] > h.left + 0.0003f)  // (the latch's tip, on its left, isn't the stem's)
                    continue;
                ++slice;
                worst = std::max({worst, h.right - p[0], p[0] - h.left, h.bottom - p[1], p[1] - h.top});
            }
            std::snprintf(what, sizeof(what), "h6 the stem's last slice (z %.4f to %.4f, %d corners of the T) inside the shaft's box to %.2f mm: the shaft is the stem's own cross-section",
                          h.back, h.front, slice, worst * 1000.0f);
            Check(slice > 0 && worst < 0.0003f, what);
            // The real triangles through the faces, pulled the whole way: on the box from the pulled stem's front home.
            bool onBox = true;
            for (int f = 0; f < handleruns::kShaftFaces; ++f) {
                float m[12];
                if (!handleruns::ShaftFace(h, f, kMk11Travel, m)) {
                    onBox = false;
                    break;
                }
                for (uint32_t i = top.start; i < top.start + top.count; ++i) {
                    float p[3], q[3];
                    d.ColourAt(d.ib[i], p);
                    Apply(m, p, q);
                    const bool along = std::fabs(q[2] - (p[2] == h.back ? h.front - kMk11Travel : h.front)) < 1e-6f;
                    const bool across = f == 0   ? std::fabs(q[1] - h.top) < 1e-6f
                                        : f == 1 ? std::fabs(q[1] - h.bottom) < 1e-6f
                                        : f == 2 ? std::fabs(q[0] - h.right) < 1e-6f
                                                 : std::fabs(q[0] - h.left) < 1e-6f;
                    const bool within = q[0] > h.right - 1e-6f && q[0] < h.left + 1e-6f && q[1] > h.bottom - 1e-6f && q[1] < h.top + 1e-6f;
                    onBox = onBox && along && across && within;
                }
            }
            std::snprintf(what, sizeof(what), "h6 the real two triangles through each face, the handle drawn back its whole %.0f mm: on that face of the box, from the pulled stem's front "
                          "(z %.4f) home (z %.4f)", kMk11Travel * 1000.0f, h.front - kMk11Travel, h.front);
            Check(onBox, what);
        }
    }

    std::printf("%d checks, %d failed, %d skipped\n", g_checks, g_failed, g_skipped);
    return g_failed ? 1 : 0;
}

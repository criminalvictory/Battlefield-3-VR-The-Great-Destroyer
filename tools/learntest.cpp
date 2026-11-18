// Guns never seen before, learned (src/gunlearn.cpp) without the game: on the mesh dumps in build\ (mesh_<count>_<start>_*.bin) and on a software
// graphics card (WARP), checks l1-l13:
//   l1  each of the 11 held guns' colour draws: its frame bone, the bone most of its vertices go with, is skin.cpp's table's;
//   l2  the parts parked away from the gun left out: the AN-94's spare magazine (bone 5), the AEK-971's (bone 3), nothing else;
//   l3  the learner's own read of each draw, from the raw buffers as they come back, gives the body tools/gunbodyfit.exe wrote into src/gunbodies.h
//       within 1 mm; where gunbodyfit added parts drawn apart (the M16A4's rounds, the M240's tape and belt), which the learner doesn't see, the body
//       of its body draw alone as gunbodyfit poses and fits it (that pose itself checked against gunbodies.h on every gun with none added);
//   l4  the checks before a vertex is read: stride 48 and the launcher's depth pass's 16 taken; 32-bit indices read as the 16-bit ones; a scrambled
//       buffer, broken weights, an unknown index format or stride, an index past the vertices, a draw past its index buffer, a base below 0, bones
//       past the palette, vertices past their buffer, nothing at all refused; never a byte read outside the buffers (each ends at a page that
//       can't be read); what its mesh is refused for good, what the palette drawn with it did (its frame bone not rigid) only till the next launch;
//   l5  the depth pass's bone: the equal matrix found, none, the first of two, the tolerance, numbers that aren't; on a real depth pass too;
//   l6  gun-sized: a grenade's 0.25 m and 0.29 m refused, 0.30 m and 0.5 m taken, a mesh longer across or up than along refused, broken numbers;
//   l7  a saved gun and a refused mesh round-trip to the bit; broken lines refused;
//   l8  the watch: the every-frame colour body picked over a bigger draw drawn half the frames and over its own bigger depth pass; not when it's
//       drawn 26 of 30 frames, at 27; a draw skipped; small draws never; the window closing at its 30th frame;
//   l9  a row published 10000 times while 8 threads read it: never a torn row; a thread holding the hands' lock first (skin.cpp's order) while
//       OnFrame and NoteDraw run: no deadlock;
//   l10 the M240: its body 27588@2364 picked over its belt 2304@0 and its depth pass, and learned;
//   l11 a 0.32 m gun taken, a 0.29 m one refused;
//   l12 the whole learner on WARP: watched, read back with DO_NOT_WAIT, fitted, its depth pass found in the same views, its palms, saved; switched
//       off no row found, on again the same row; at the next launch held again once its key's checked, on its colour draw (a depth-only pass of the
//       same indices from another vertex buffer doesn't drop it); a changed key learned again; a mesh refused never read again; a frame bone not
//       rigid at the capture refused till the next launch only; no room in memory for the copy (on the thread that presents) or for the fit (on its
//       own thread) taken without an exception, refused till the next launch; a graphics card refusing every read back: 3 tries, then left alone,
//       the key check likewise; a buffer over 4 MB never read back; the test gun never saved, learned anew once switched off and on; nothing in
//       flight kept when it's switched off; no frame's work over 16 ms;
//   l13 under a narrow bottom: a magazine wider than its receiver (bodyfit leaves its corners where they are) would stand the gun on it: refused
//       till the next launch; one no wider, its bottom on the keel: taken.
// Usage: learntest [<folder with the mesh dumps>] (the exe's own folder by default). Prints PASS, FAIL or SKIP per check, with what was measured;
// the exit code is the failures. Writes only a file of its own in the temp folder (its process's number in its name: two runs at once never share
// it), never beside the game's files (the log isn't opened).

#include "../src/bodyfit.h"  // before any Windows header: those make the words near and far into nothing

#include <windows.h>
#include <d3d11.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <mutex>
#include <new>
#include <string>
#include <thread>
#include <vector>

#include "meshdump.h"
#include "../src/gunbodies.h"
#include "../src/gunlearn.h"
#include "../src/loose.h"

// ---------------------------------------------------------------- no room in memory, on demand
// This program's own operator new: while g_refuseFrom is set, a block that big or bigger is refused on the main thread (where OnFrame runs: the
// learner's copies of what came back) or off it (the fit's own thread), as a 32-bit game with no room left refuses it.
namespace {
std::atomic<size_t> g_refuseFrom{0};
std::atomic<bool> g_refuseOnMain{true};
std::atomic<int> g_refusedBlocks{0};
DWORD g_mainThread = 0;
}  // namespace

void* operator new(size_t bytes) {
    const size_t from = g_refuseFrom.load(std::memory_order_relaxed);
    if (from && bytes >= from && (GetCurrentThreadId() == g_mainThread) == g_refuseOnMain.load(std::memory_order_relaxed)) {
        g_refusedBlocks.fetch_add(1, std::memory_order_relaxed);
        throw std::bad_alloc();
    }
    if (void* p = std::malloc(bytes ? bytes : 1))
        return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept {
    std::free(p);
}
void operator delete(void* p, size_t) noexcept {
    std::free(p);
}

namespace {

int g_failures = 0, g_checks = 0;
void Check(bool ok, const char* what) {
    ++g_checks;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        ++g_failures;
}
template <typename... Args>
void CheckF(bool ok, const char* format, Args... args) {
    char what[600];
    std::snprintf(what, sizeof(what), format, args...);
    Check(ok, what);
}
template <typename... Args>
void Skip(const char* format, Args... args) {
    char what[600];
    std::snprintf(what, sizeof(what), format, args...);
    std::printf("SKIP %s\n", what);
}
template <typename... Args>
void Info(const char* format, Args... args) {
    char what[600];
    std::snprintf(what, sizeof(what), format, args...);
    std::printf("     %s\n", what);
}

std::string g_dir;
std::wstring g_store;

// The 11 held guns: skin.cpp kHeldKinds' colour draws, frame bones and palms, and the parts parked away from each.
struct GunRow {
    const char* shape;
    uint32_t count, start;
    int frame;
    int parked;  // the bone left out, -1 none
    float rightPalm[3], leftPalm[3];
    uint32_t depthIndices, depthStart;  // its depth pass (as skin.cpp's table: start 0 is "as many in as it has")
    int depthFrame;
};
const GunRow kGuns[] = {
    {"rpk", 24492, 0, 3, -1, {-0.032f, -0.080f, 0.192f}, {-0.001f, -0.029f, 0.585f}, 24840, 0, -1},
    {"an94", 16188, 0, 0, 5, {-0.023f, -0.059f, 0.192f}, {0.002f, -0.018f, 0.561f}, 16590, 0, -1},
    {"aek971", 21585, 0, 4, 3, {-0.024f, -0.079f, 0.208f}, {0.004f, -0.031f, 0.551f}, 21933, 0, -1},
    {"ak74", 21723, 0, 2, -1, {-0.032f, -0.080f, 0.193f}, {0.001f, -0.031f, 0.548f}, 22995, 0, -1},
    {"g3a3", 23064, 0, 0, -1, {-0.034f, -0.082f, 0.241f}, {0.020f, -0.007f, 0.616f}, 23064, 0, -1},
    {"kh2002", 15237, 0, 3, -1, {-0.025f, -0.099f, 0.234f}, {0.016f, -0.059f, 0.447f}, 15549, 0, -1},
    {"m16a4", 17013, 312, 4, -1, {-0.054f, -0.123f, 0.256f}, {0.014f, -0.041f, 0.636f}, 18675, 0, -1},
    {"m4a1", 22194, 312, 2, -1, {-0.054f, -0.114f, 0.255f}, {0.012f, -0.041f, 0.612f}, 22506, 0, 3},
    {"m240", 27588, 2364, 3, -1, {-0.023f, -0.115f, 0.196f}, {0.018f, -0.065f, 0.678f}, 29952, 0, 9},
    {"fim92", 20298, 354, 0, -1, {-0.046f, -0.132f, 0.593f}, {0.025f, -0.146f, 0.993f}, 20298, 20652, -1},
    // Semper Fidelis's two (2026-11-18).
    {"aks74u", 17991, 0, 2, -1, {-0.031f, -0.080f, 0.193f}, {0.003f, -0.035f, 0.517f}, 19269, 0, -1},
    {"ump45", 19314, 234, 2, -1, {-0.053f, -0.066f, 0.223f}, {0.004f, -0.026f, 0.538f}, 19638, 0, 3},
    {"m1014", 13377, 0, 3, -1, {-0.038f, -0.030f, 0.217f}, {-0.017f, 0.020f, 0.660f}, 14151, 0, -1},
    // Operation Swordbreaker's Mk11 (2026-11-18): its body 2214 indices in, after its piece of two bones and its rounds.
    {"mk11", 12456, 2214, 0, -1, {-0.054f, -0.114f, 0.254f}, {0.012f, -0.041f, 0.612f}, 14670, 0, -1},
};
constexpr uint32_t kPaletteBones = 60;  // skin.cpp's: what a draw's palette is passed with

// A dump: its buffers whole, as saved, and how the draw had them bound (dump.txt).
struct Dump {
    bool ok = false;
    uint32_t count = 0, start = 0;
    int base = 0;
    uint32_t ibFormat = 0, ibOffset = 0, stride = 0, vbOffset = 0;
    std::vector<uint8_t> ib, vb, bones;
};
Dump LoadDump(uint32_t count, uint32_t start) {
    Dump d;
    const std::string name = g_dir + "\\mesh_" + std::to_string(count) + "_" + std::to_string(start) + "_";
    d.ib = meshdump::ReadAll(name + "ib.bin");
    d.vb = meshdump::ReadAll(name + "vb0.bin");
    d.bones = meshdump::ReadAll(name + "bones.bin");
    std::ifstream text(name + "dump.txt");
    std::string word;
    bool haveVb = false;
    while (text >> word) {
        if (word == "draw") {
            text >> d.count >> d.start >> d.base;
        } else if (word == "ib") {
            std::string f, o;
            text >> f >> d.ibFormat >> o >> d.ibOffset;
        } else if (word == "vb") {
            int slot = -1;
            std::string s, o;
            uint32_t stride = 0, offset = 0;
            if (text >> slot >> s >> stride >> o >> offset && slot == 0 && !haveVb)
                d.stride = stride, d.vbOffset = offset, haveVb = true;
        }
    }
    d.ok = !d.ib.empty() && !d.vb.empty() && d.bones.size() >= 48 * kPaletteBones && haveVb && d.count == count && d.start == start;
    return d;
}
gunlearn::Capture CaptureOf(const Dump& d, uint32_t bones = kPaletteBones) {
    gunlearn::Capture c;
    c.ib = d.ib.data(), c.ibBytes = static_cast<uint32_t>(d.ib.size()), c.ibOffset = d.ibOffset, c.ibFormat = d.ibFormat;
    c.vb = d.vb.data(), c.vbBytes = static_cast<uint32_t>(d.vb.size()), c.vbOffset = d.vbOffset, c.stride = d.stride;
    c.count = d.count, c.start = d.start, c.base = d.base;
    c.palette = reinterpret_cast<const float*>(d.bones.data());
    c.paletteBones = std::min<uint32_t>(bones, static_cast<uint32_t>(d.bones.size() / 48));
    return c;
}
const float* Palette(const Dump& d) {
    return reinterpret_cast<const float*>(d.bones.data());
}

float Distance(const float* a, const float* b) {
    const float d[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    return std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
}
bool SameFit(const gunlearn::Fitted& a, const gunlearn::Fitted& b) {
    return a.ok == b.ok && a.frameBone == b.frameBone && a.hullPoints == b.hullPoints && std::memcmp(a.hull, b.hull, sizeof(a.hull)) == 0 &&
           std::memcmp(a.boxMin, b.boxMin, sizeof(a.boxMin)) == 0 && std::memcmp(a.boxMax, b.boxMax, sizeof(a.boxMax)) == 0 &&
           std::memcmp(a.centre, b.centre, sizeof(a.centre)) == 0 && a.used == b.used;
}

// A copy that ends at a page nothing may read: one byte read past its end stops the program there.
struct Guarded {
    uint8_t* base = nullptr;
    uint8_t* data = nullptr;
    Guarded(const void* from, size_t bytes) {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        const size_t page = si.dwPageSize, body = (bytes + page - 1) / page * page + (bytes ? 0 : page);
        base = static_cast<uint8_t*>(VirtualAlloc(nullptr, body + page, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
        DWORD old = 0;
        VirtualProtect(base + body, page, PAGE_NOACCESS, &old);
        data = base + body - bytes;
        if (bytes)
            std::memcpy(data, from, bytes);
    }
    ~Guarded() { VirtualFree(base, 0, MEM_RELEASE); }
    Guarded(const Guarded&) = delete;
    Guarded& operator=(const Guarded&) = delete;
};
// The fit on guarded copies; read: whether it stayed inside them.
bool FitInside(const gunlearn::Capture& c, gunlearn::Fitted& f, bool& stayed) {
    __try {
        stayed = true;
        return gunlearn::FitCapture(c, f);
    } __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        stayed = false;
        return false;
    }
}
bool FitGuarded(const gunlearn::Capture& c, gunlearn::Fitted& f, bool& stayed) {
    Guarded ib(c.ib, c.ibBytes), vb(c.vb, c.vbBytes), palette(c.palette, size_t(std::min<uint32_t>(c.paletteBones, gunlearn::kMostBones)) * 48);
    gunlearn::Capture g = c;
    g.ib = c.ib ? ib.data : nullptr, g.vb = c.vb ? vb.data : nullptr, g.palette = c.palette ? reinterpret_cast<const float*>(palette.data) : nullptr;
    return FitInside(g, f, stayed);
}

// A synthetic gun: a box x by y by z (m) about (0, 0, z/2 - 0.1), its vertices on its faces, every one on bone 0, stride 48 as the colour passes'.
uint16_t ToHalf(float v) {
    uint32_t x;
    std::memcpy(&x, &v, 4);
    const uint32_t sign = (x >> 16) & 0x8000;
    const int e = static_cast<int>((x >> 23) & 0xFF) - 127 + 15;
    const uint32_t m = x & 0x7FFFFF;
    if (e <= 0)
        return static_cast<uint16_t>(sign);
    if (e >= 31)
        return static_cast<uint16_t>(sign | 0x7C00);
    return static_cast<uint16_t>(sign | (uint32_t(e) << 10) | (m >> 13));
}
struct Synthetic {
    std::vector<uint8_t> ib, vb;
    float palette[2][12] = {{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0}, {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0}};
    uint32_t bones = 1, count = 0;
};
Synthetic MakeBox(float x, float y, float z, int triangles) {
    Synthetic s;
    uint32_t seed = 12345;
    auto next = [&] {
        seed = seed * 1664525u + 1013904223u;
        return static_cast<float>((seed >> 8) & 0xFFFF) / 65535.0f;
    };
    std::vector<float> p;
    const float lo[3] = {-x / 2, -y / 2, -0.1f}, hi[3] = {x / 2, y / 2, z - 0.1f};
    for (int c = 0; c < 8; ++c)
        p.insert(p.end(), {c & 1 ? hi[0] : lo[0], c & 2 ? hi[1] : lo[1], c & 4 ? hi[2] : lo[2]});
    for (int i = 0; i < 400; ++i) {
        const int axis = static_cast<int>(next() * 2.999f), side = next() < 0.5f ? 0 : 1;
        float q[3];
        for (int k = 0; k < 3; ++k)
            q[k] = lo[k] + (hi[k] - lo[k]) * next();
        q[axis] = side ? hi[axis] : lo[axis];
        p.insert(p.end(), q, q + 3);
    }
    const size_t n = p.size() / 3;
    s.vb.assign(n * 48, 0);
    for (size_t v = 0; v < n; ++v) {
        uint8_t* o = &s.vb[v * 48];
        const uint16_t h[4] = {ToHalf(p[v * 3]), ToHalf(p[v * 3 + 1]), ToHalf(p[v * 3 + 2]), ToHalf(1.0f)};
        std::memcpy(o, h, 8);
        o[12] = 255;  // all its weight on bone 0
    }
    for (int t = 0; t < triangles; ++t)
        for (int c = 0; c < 3; ++c) {
            const uint16_t v = static_cast<uint16_t>(std::min<size_t>(n - 1, static_cast<size_t>(next() * static_cast<float>(n))));
            s.ib.push_back(static_cast<uint8_t>(v & 0xFF)), s.ib.push_back(static_cast<uint8_t>(v >> 8));
        }
    s.count = static_cast<uint32_t>(triangles * 3);
    return s;
}
// A synthetic gun of two boxes (lo, hi in m), each its vertices on its faces and its own bone (0 and 1, the palette both as they are), its
// triangles random within each box: a receiver (its bone the core: more of its vertices) and a magazine under it.
Synthetic MakeTwoBoxes(const float (&lo0)[3], const float (&hi0)[3], const float (&lo1)[3], const float (&hi1)[3], int trianglesEach) {
    Synthetic s;
    s.bones = 2;
    uint32_t seed = 777;
    auto next = [&] {
        seed = seed * 1664525u + 1013904223u;
        return static_cast<float>((seed >> 8) & 0xFFFF) / 65535.0f;
    };
    const float* los[2] = {lo0, lo1};
    const float* his[2] = {hi0, hi1};
    const int points[2] = {500, 200};
    for (int box = 0; box < 2; ++box) {
        const float *lo = los[box], *hi = his[box];
        const size_t first = s.vb.size() / 48;
        std::vector<float> p;
        for (int c = 0; c < 8; ++c)
            p.insert(p.end(), {c & 1 ? hi[0] : lo[0], c & 2 ? hi[1] : lo[1], c & 4 ? hi[2] : lo[2]});
        for (int i = 0; i < points[box]; ++i) {
            const int axis = static_cast<int>(next() * 2.999f), side = next() < 0.5f ? 0 : 1;
            float q[3];
            for (int k = 0; k < 3; ++k)
                q[k] = lo[k] + (hi[k] - lo[k]) * next();
            q[axis] = side ? hi[axis] : lo[axis];
            p.insert(p.end(), q, q + 3);
        }
        const size_t n = p.size() / 3;
        s.vb.resize((first + n) * 48, 0);
        for (size_t v = 0; v < n; ++v) {
            uint8_t* o = &s.vb[(first + v) * 48];
            const uint16_t h[4] = {ToHalf(p[v * 3]), ToHalf(p[v * 3 + 1]), ToHalf(p[v * 3 + 2]), ToHalf(1.0f)};
            std::memcpy(o, h, 8);
            o[8] = static_cast<uint8_t>(box);
            o[12] = 255;  // all its weight on its box's bone
        }
        for (int t = 0; t < trianglesEach; ++t)
            for (int c = 0; c < 3; ++c) {
                const uint16_t v = static_cast<uint16_t>(first + std::min<size_t>(n - 1, static_cast<size_t>(next() * static_cast<float>(n))));
                s.ib.push_back(static_cast<uint8_t>(v & 0xFF)), s.ib.push_back(static_cast<uint8_t>(v >> 8));
            }
    }
    s.count = static_cast<uint32_t>(s.ib.size() / 2);
    return s;
}
gunlearn::Capture CaptureOf(const Synthetic& s) {
    gunlearn::Capture c;
    c.ib = s.ib.data(), c.ibBytes = static_cast<uint32_t>(s.ib.size()), c.ibFormat = 57;
    c.vb = s.vb.data(), c.vbBytes = static_cast<uint32_t>(s.vb.size()), c.stride = 48;
    c.count = s.count;
    c.palette = &s.palette[0][0], c.paletteBones = s.bones;
    return c;
}

// A saved gun from a fit, as the learner makes one.
gunlearn::Saved SavedOf(const gunlearn::Fitted& f, uint32_t count, uint32_t start) {
    gunlearn::Saved s;
    gunlearn::Row& r = s.row;
    r.bodyIndices = count, r.bodyStart = start, r.frameBone = f.frameBone;
    r.depthIndices = 16590, r.depthStart = 16590, r.depthFrame = 0;
    r.palms = true;
    const float right[3] = {-0.023f, -0.059f, 0.192f}, left[3] = {0.002f, -0.018f, 0.561f};
    std::memcpy(r.rightPalm, right, sizeof(right));
    std::memcpy(r.leftPalm, left, sizeof(left));
    std::memcpy(r.muzzle, f.muzzle, sizeof(r.muzzle));
    std::memcpy(r.sights, f.sights, sizeof(r.sights));
    std::memcpy(r.port, f.port, sizeof(r.port));
    std::memcpy(r.hullMin, f.hullMin, sizeof(r.hullMin));
    std::memcpy(r.hullMax, f.hullMax, sizeof(r.hullMax));
    r.vertices = f.vertices, r.indexHash = f.indexHash;
    std::memcpy(s.boxMin, f.boxMin, sizeof(s.boxMin));
    std::memcpy(s.boxMax, f.boxMax, sizeof(s.boxMax));
    std::memcpy(s.centre, f.centre, sizeof(s.centre));
    s.keelX = f.keelX, s.mass = 3.5f;
    s.hullPoints = f.hullPoints;
    std::memcpy(s.hull, f.hull, sizeof(s.hull));
    return s;
}
bool SameSaved(const gunlearn::Saved& a, const gunlearn::Saved& b) {
    const gunlearn::Row &x = a.row, &y = b.row;
    auto same = [](const float* p, const float* q, int n) { return std::memcmp(p, q, sizeof(float) * n) == 0; };
    return x.bodyIndices == y.bodyIndices && x.bodyStart == y.bodyStart && x.depthIndices == y.depthIndices && x.depthStart == y.depthStart &&
           x.frameBone == y.frameBone && x.depthFrame == y.depthFrame && x.palms == y.palms && same(x.rightPalm, y.rightPalm, 3) &&
           same(x.leftPalm, y.leftPalm, 3) && x.sightsLearned == y.sightsLearned && same(&x.up, &y.up, 1) && same(x.sights, y.sights, 3) &&
           same(x.muzzle, y.muzzle, 3) && same(x.port, y.port, 3) && same(x.hullMin, y.hullMin, 3) && same(x.hullMax, y.hullMax, 3) &&
           x.vertices == y.vertices && x.indexHash == y.indexHash && same(a.boxMin, b.boxMin, 3) && same(a.boxMax, b.boxMax, 3) &&
           same(a.centre, b.centre, 3) && same(&a.keelX, &b.keelX, 1) && same(&a.mass, &b.mass, 1) && a.hullPoints == b.hullPoints &&
           same(&a.hull[0][0], &b.hull[0][0], 3 * a.hullPoints);
}

std::string ReadText(const std::wstring& path) {
    std::ifstream f(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
void WriteText(const std::wstring& path, const std::string& text) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    f << text;
}
std::vector<std::string> Lines(const std::string& text, const char* starting) {
    std::vector<std::string> out;
    size_t at = 0;
    while (at < text.size()) {
        size_t end = text.find('\n', at);
        if (end == std::string::npos)
            end = text.size();
        std::string line = text.substr(at, end - at);
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.rfind(starting, 0) == 0)
            out.push_back(line);
        at = end + 1;
    }
    return out;
}
// The index-th number after a word in a saved line, changed by `by` (a key at its top goes the other way).
std::string Bump(const std::string& line, const char* word, int index, long long by) {
    const std::string first = std::string(word) + " ", inside = " " + first;
    size_t at = 0;
    if (line.rfind(first, 0) == 0) {
        at = first.size();
    } else {
        at = line.find(inside);
        if (at == std::string::npos)
            return line;
        at += inside.size();
    }
    for (int i = 0; i < index; ++i)
        at = line.find(' ', at) + 1;
    size_t end = line.find(' ', at);
    if (end == std::string::npos)
        end = line.size();
    const long long v = std::stoll(line.substr(at, end - at));
    const long long changed = v + by > 4294967295LL ? v - by : v + by;
    return line.substr(0, at) + std::to_string(changed) + line.substr(end);
}

// ---------------------------------------------------------------- the body draw alone, as tools/gunbodyfit.cpp poses and fits it
// (gunbodyfit's own steps, copied: a check of the learner's must not run the learner's code.)
void Compose(const float* a, const float* b, float* out) {
    float r[12];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j)
            r[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        r[i * 4 + 3] += a[i * 4 + 3];
    }
    std::memcpy(out, r, sizeof(r));
}
void InvertRigid(const float* m, float* inv) {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            inv[i * 4 + j] = m[j * 4 + i];
    for (int i = 0; i < 3; ++i)
        inv[i * 4 + 3] = -(inv[i * 4] * m[3] + inv[i * 4 + 1] * m[7] + inv[i * 4 + 2] * m[11]);
}
int IndexAt(const Dump& d, uint32_t i) {
    if (d.ibFormat == 42) {
        uint32_t v;
        std::memcpy(&v, &d.ib[d.ibOffset + size_t(i) * 4], 4);
        return static_cast<int>(v) + d.base;
    }
    uint16_t v;
    std::memcpy(&v, &d.ib[d.ibOffset + size_t(i) * 2], 2);
    return v + d.base;
}
// The body draw alone posed as gunbodyfit poses it (FramePalette: every bone of the dump carried onto the table's frame bone; AddSkinned: each
// vertex its weights' blend, numbered as the triangle list first names it, one past its buffer, on a bone past the palette or not a number left out
// with its triangles) and fitted with its options (bodyfit's own): the body gunbodyfit writes for a gun with no parts drawn apart added.
bodyfit::Result BodyAlone(const Dump& d, int frame) {
    bodyfit::Result none;
    none.why = "its dump can't be posed";
    const int bones = static_cast<int>(d.bones.size() / 48);
    const size_t width = d.ibFormat == 42 ? 4 : d.ibFormat == 57 ? 2 : 0;
    if (!d.ok || frame < 0 || frame >= bones || (d.stride != 48 && d.stride != 16) || d.vb.size() <= d.vbOffset || !width ||
        d.ib.size() < d.ibOffset + width * (size_t(d.start) + d.count))
        return none;
    const float* palette = Palette(d);
    float toFrame[12];
    InvertRigid(palette + size_t(frame) * 12, toFrame);
    std::vector<float> onFrame(size_t(bones) * 12);
    for (int b = 0; b < bones; ++b)
        Compose(toFrame, palette + size_t(b) * 12, &onFrame[size_t(b) * 12]);
    const int n = static_cast<int>((d.vb.size() - d.vbOffset) / d.stride);
    std::vector<int> index(size_t(n), -1), bone, tris;
    std::vector<float> pos;
    for (uint32_t i = d.start; i + 2 < d.start + d.count; i += 3) {
        int corner[3] = {-1, -1, -1};
        bool keep = true;
        for (int k = 0; k < 3; ++k) {
            const int v = IndexAt(d, i + k);
            if (v < 0 || v >= n) {
                keep = false;
                continue;
            }
            if (index[size_t(v)] == -1) {
                const uint8_t* o = &d.vb[d.vbOffset + size_t(v) * d.stride];
                uint16_t h[3];
                std::memcpy(h, o, 6);
                const float p[3] = {meshdump::Half(h[0]), meshdump::Half(h[1]), meshdump::Half(h[2])};
                float sum[3] = {};
                int weights = 0, main = -1, mainWeight = -1;
                bool ok = std::isfinite(p[0]) && std::isfinite(p[1]) && std::isfinite(p[2]);
                for (int s = 0; s < 4 && ok; ++s) {
                    const int b = o[8 + s], w = o[12 + s];
                    weights += w;
                    if (!w)
                        continue;
                    if (b >= bones) {
                        ok = false;
                        break;
                    }
                    float q[3];
                    meshdump::Apply(&onFrame[size_t(b) * 12], p, q);
                    for (int c = 0; c < 3; ++c)
                        sum[c] += q[c] * static_cast<float>(w);
                    if (w > mainWeight)
                        mainWeight = w, main = b;
                }
                if (!ok || weights <= 0 || !std::isfinite(sum[0]) || !std::isfinite(sum[1]) || !std::isfinite(sum[2])) {
                    index[size_t(v)] = -2;
                } else {
                    index[size_t(v)] = static_cast<int>(bone.size());
                    for (int c = 0; c < 3; ++c)
                        pos.push_back(sum[c] / static_cast<float>(weights));
                    bone.push_back(main);
                }
            }
            if (index[size_t(v)] < 0)
                keep = false;
            corner[k] = index[size_t(v)];
        }
        if (keep)
            tris.insert(tris.end(), corner, corner + 3);
    }
    if (bone.empty() || tris.empty())
        return none;
    return bodyfit::Fit(pos.data(), bone.data(), static_cast<int>(bone.size()), tris.data(), static_cast<int>(tris.size() / 3));
}
// A body's numbers wherever they are: the learner's fit, bodyfit's result, gunbodies.h's row.
struct Body {
    const float *boxMin, *boxMax, *hullMin, *hullMax;
    const float (*hull)[3];
    int points;
};
Body BodyOf(const gunlearn::Fitted& f) {
    return {f.boxMin, f.boxMax, f.hullMin, f.hullMax, f.hull, f.hullPoints};
}
Body BodyOf(const bodyfit::Result& r) {
    return {r.boxMin, r.boxMax, r.hullMin, r.hullMax, r.hull, r.hullPoints};
}
Body BodyOf(const gunbodies::Body& b) {
    return {b.spec->boxMin, b.spec->boxMax, b.hullMin, b.hullMax, b.spec->hull, b.spec->hullPoints};
}
// How far apart two bodies are (m): their loose boxes, their hulls' boxes, their hull points one by one; endless with not as many points.
float BodyOff(const Body& a, const Body& b) {
    if (a.points != b.points)
        return HUGE_VALF;
    float off = 0.0f;
    for (int k = 0; k < 3; ++k)
        off = std::max({off, std::fabs(a.boxMin[k] - b.boxMin[k]), std::fabs(a.boxMax[k] - b.boxMax[k]), std::fabs(a.hullMin[k] - b.hullMin[k]),
                        std::fabs(a.hullMax[k] - b.hullMax[k])});
    for (int k = 0; k < a.points; ++k)
        off = std::max(off, Distance(a.hull[k], b.hull[k]));
    return off;
}

// ---------------------------------------------------------------- l1-l4: the fit on the dumps
void FitChecks(Dump (&dumps)[std::size(kGuns)], gunlearn::Fitted (&fits)[std::size(kGuns)]) {
    for (size_t i = 0; i < std::size(kGuns); ++i) {
        const GunRow& g = kGuns[i];
        dumps[i] = LoadDump(g.count, g.start);
        if (!dumps[i].ok) {
            Skip("l1 %s: no dump mesh_%u_%u_* in %s", g.shape, g.count, g.start, g_dir.c_str());
            continue;
        }
        const auto t0 = std::chrono::steady_clock::now();
        const bool ok = gunlearn::FitCapture(CaptureOf(dumps[i]), fits[i]);
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        CheckF(ok && fits[i].frameBone == g.frame, "l1 %s (%u@%u): frame bone %d, the table's %d (%d of %u vertices)%s%s; read and fitted in %.1f ms", g.shape,
               g.count, g.start, fits[i].frameBone, g.frame, fits[i].frameVertices, fits[i].used, ok ? "" : ", refused: ", ok ? "" : fits[i].why, ms);
    }
    for (size_t i = 0; i < std::size(kGuns); ++i) {
        const GunRow& g = kGuns[i];
        if (!fits[i].ok)
            continue;
        const gunlearn::Fitted& f = fits[i];
        std::string out;
        for (int k = 0; k < f.leftOutCount; ++k)
            out += (out.empty() ? "" : " ") + std::to_string(f.leftOut[k]);
        const bool right = g.parked < 0 ? f.leftOutCount == 0 : f.leftOutCount == 1 && f.leftOut[0] == g.parked;
        CheckF(right, "l2 %s: left out [%s], wanted [%s]", g.shape, out.c_str(), g.parked < 0 ? "" : std::to_string(g.parked).c_str());
    }
    for (size_t i = 0; i < std::size(kGuns); ++i)
        if (fits[i].ok)
            Info("%s: kept %.3f m long (z %.3f to %.3f), %.3f across, %.3f high; middle (%.3f %.3f %.3f), keel x %.4f, %d hull points; muzzle (0 %.3f %.3f), "
                 "sights' line until learned (0 %.3f %.3f)",
                 kGuns[i].shape, fits[i].keptMax[2] - fits[i].keptMin[2], fits[i].keptMin[2], fits[i].keptMax[2], fits[i].keptMax[0] - fits[i].keptMin[0],
                 fits[i].keptMax[1] - fits[i].keptMin[1], fits[i].centre[0], fits[i].centre[1], fits[i].centre[2], fits[i].keelX, fits[i].hullPoints,
                 fits[i].muzzle[1], fits[i].muzzle[2], fits[i].sights[1], fits[i].sights[2]);
    // l3: the body tools/gunbodyfit.exe wrote into src/gunbodies.h from the same dump. gunbodyfit also adds a gun's parts drawn apart, those its own
    // table names, when a dump of them from the same buffer is here; the learner fits its body draw alone, so such a gun is held to
    // its body draw alone posed and fitted as gunbodyfit does (BodyAlone), and that pose is held to gunbodies.h on every gun with no part added.
    struct Part {
        const char* shape;
        uint32_t count, start;
        const char* what;
    };
    const Part parts[] = {{"m16a4", 312, 0, "its rounds 312@0"}, {"m240", 60, 2304, "its box's tape 60@2304"},
                          {"m240", 2304, 0, "its belt 2304@0"}, {"aks74u", 894, 18063, "its magazine's other piece 894@18063"},
                          {"aks74u", 312, 18957, "its rounds 312@18957"}, {"ump45", 234, 0, "its rounds 234@0"},
                          {"mk11", 1902, 0, "its stock's plate and its magazine's sleeve 1902@0"}, {"mk11", 312, 1902, "its rounds 312@1902"}};  // (gunbodyfit's kGuns' parts)
    for (size_t i = 0; i < std::size(kGuns); ++i) {
        const GunRow& g = kGuns[i];
        if (!fits[i].ok)
            continue;
        const gunbodies::Body* b = gunbodies::For(g.shape);
        if (!b || !b->spec) {
            Skip("l3 %s: src/gunbodies.h has no fitted body for it yet", g.shape);
            continue;
        }
        // (A dump of another buffer is another gun's: the M4A1's rounds and the M16A4's share their numbers.)
        std::string added;
        for (const Part& p : parts)
            if (std::strcmp(p.shape, g.shape) == 0 &&
                meshdump::ReadAll(g_dir + "\\mesh_" + std::to_string(p.count) + "_" + std::to_string(p.start) + "_vb0.bin").size() == dumps[i].vb.size())
                added += (added.empty() ? "" : ", ") + std::string(p.what);
        const bodyfit::Result alone = BodyAlone(dumps[i], g.frame);
        if (!alone.ok) {
            Check(false, ("l3 " + std::string(g.shape) + ": its body draw alone, posed as gunbodyfit does, not fitted: " + alone.why).c_str());
            continue;
        }
        const float learnedToAlone = BodyOff(BodyOf(fits[i]), BodyOf(alone)), aloneToHeader = BodyOff(BodyOf(alone), BodyOf(*b)),
                    learnedToHeader = BodyOff(BodyOf(fits[i]), BodyOf(*b));
        if (!added.empty())
            CheckF(learnedToAlone <= 0.001f, "l3 %s: the learner's body against its body draw alone as gunbodyfit poses and fits it (gunbodies.h's %s has %s "
                   "added): %d and %d hull points, %.3f mm apart at most", g.shape, b->spec->name, added.c_str(), fits[i].hullPoints, alone.hullPoints,
                   learnedToAlone * 1000.0f);
        else
            CheckF(learnedToHeader <= 0.001f && aloneToHeader <= 0.001f && learnedToAlone <= 0.001f,
                   "l3 %s: the learner's body against gunbodies.h's %s: %d and %d hull points, %.3f mm apart at most (its body draw alone as gunbodyfit "
                   "poses it: %.3f mm from gunbodies.h's, %.3f from the learner's)", g.shape, b->spec->name, fits[i].hullPoints, b->spec->hullPoints,
                   learnedToHeader * 1000.0f, aloneToHeader * 1000.0f, learnedToAlone * 1000.0f);
    }
}

void LayoutChecks(const Dump& an94, const gunlearn::Fitted& an94Fit) {
    if (!an94.ok || !an94Fit.ok) {
        Skip("l4: no AN-94 dump");
        return;
    }
    const gunlearn::Capture base = CaptureOf(an94);
    // want: taken; refused for what its mesh is (kept in heldguns.txt for good); or refused for what the palette drawn with it did that once (till
    // the next launch only).
    enum Want { kTaken, kForGood, kTillNext };
    auto run = [&](const char* what, const gunlearn::Capture& c, Want want, gunlearn::Fitted* outFit = nullptr) {
        gunlearn::Fitted f;
        bool stayed = false;
        const bool ok = FitGuarded(c, f, stayed);
        const bool right = want == kTaken ? ok : !ok && f.forGood == (want == kForGood);
        CheckF(stayed && right, "l4 %s: %s%s%s", what, !stayed ? "READ OUTSIDE ITS BUFFERS" : ok ? "taken" : "refused: ", ok || !stayed ? "" : f.why,
               ok || !stayed ? "" : f.forGood ? " (for good)" : " (till the next launch)");
        if (outFit)
            *outFit = f;
        return ok;
    };
    gunlearn::Fitted same;
    run("the AN-94 as dumped (stride 48), its buffers ending at an unreadable page", base, kTaken, &same);
    Check(SameFit(same, an94Fit), "l4 the AN-94 guarded fitted the same as unguarded");
    // The launcher's depth pass: stride 16, its whole launcher on one palette, the tube most weighted.
    if (const Dump smaw = LoadDump(21756, 21756); smaw.ok) {
        gunlearn::Fitted f;
        run("the launcher's depth pass (stride 16)", CaptureOf(smaw), kTaken, &f);
        CheckF(f.frameBone == 2, "l4 the launcher's depth pass: frame bone %d (its tube, 2), %d of %u vertices", f.frameBone, f.frameVertices, f.used);
    } else {
        Skip("l4: no dump of the launcher's depth pass (mesh_21756_21756_*)");
    }
    // 32-bit indices: the same draw's read the same.
    {
        std::vector<uint8_t> wide(an94.ib.size() * 2);
        for (size_t i = 0; i < an94.ib.size() / 2; ++i) {
            uint16_t v;
            std::memcpy(&v, &an94.ib[i * 2], 2);
            const uint32_t w = v;
            std::memcpy(&wide[i * 4], &w, 4);
        }
        gunlearn::Capture c = base;
        c.ib = wide.data(), c.ibBytes = static_cast<uint32_t>(wide.size()), c.ibFormat = 42;
        gunlearn::Fitted f;
        run("32-bit indices", c, kTaken, &f);
        CheckF(SameFit(f, an94Fit), "l4 32-bit indices: the same body as the 16-bit ones (%d hull points)", f.hullPoints);
    }
    // A scrambled vertex buffer; one with its weights broken (its bones fine).
    {
        std::vector<uint8_t> noise(an94.vb.size());
        uint32_t seed = 99;
        for (uint8_t& b : noise)
            seed = seed * 1103515245u + 12345u, b = static_cast<uint8_t>(seed >> 16);
        gunlearn::Capture c = base;
        c.vb = noise.data();
        run("a scrambled vertex buffer", c, kForGood);
        std::vector<uint8_t> weights = an94.vb;
        for (size_t v = 0; v + 48 <= weights.size(); v += 48)
            weights[v + 12] = 10, weights[v + 13] = 10, weights[v + 14] = 10, weights[v + 15] = 10;
        c.vb = weights.data();
        run("weights summing 40", c, kForGood);
    }
    {
        gunlearn::Capture c = base;
        c.ibFormat = 41;
        run("an index format of 41 (R32_FLOAT)", c, kForGood);
        c.ibFormat = 0;
        run("no index format", c, kForGood);
    }
    {
        gunlearn::Capture c = base;
        c.stride = 32;
        run("a stride of 32", c, kForGood);
    }
    // An index past the vertices: it would read the unreadable page if it weren't checked first.
    {
        std::vector<uint8_t> ib = an94.ib;
        const uint32_t room = (static_cast<uint32_t>(an94.vb.size()) - an94.vbOffset) / an94.stride;
        const uint16_t past = static_cast<uint16_t>(room);
        std::memcpy(&ib[(an94.start + 3000) * 2], &past, 2);
        gunlearn::Capture c = base;
        c.ib = ib.data();
        CheckF(room < 65536, "l4 the AN-94's vertices from its offset: %u", room);
        run("an index one past its vertices", c, kForGood);
        const uint16_t last = static_cast<uint16_t>(room - 1);
        std::memcpy(&ib[(an94.start + 3000) * 2], &last, 2);
        run("an index on its last vertex", c, kTaken);
    }
    {
        gunlearn::Capture c = base;
        c.count = (c.ibBytes / 2 - c.start) + 3;
        run("a draw past its index buffer's end", c, kForGood);
        c = base;
        c.start = c.ibBytes / 2;
        run("a draw starting at its index buffer's end", c, kForGood);
        c = base;
        c.ibOffset = c.ibBytes - 2;
        run("an index offset at its buffer's end", c, kForGood);
    }
    {
        uint32_t lowest = 0xFFFFFFFFu;
        for (uint32_t i = 0; i < an94.count; ++i) {
            uint16_t v;
            std::memcpy(&v, &an94.ib[(an94.start + i) * 2], 2);
            lowest = std::min<uint32_t>(lowest, v);
        }
        gunlearn::Capture c = base;
        c.base = -static_cast<int>(lowest) - 1;
        char what[96];
        std::snprintf(what, sizeof(what), "a base of %d (its lowest index, %u, below the buffer)", c.base, lowest);
        run(what, c, kForGood);
        c.base = 1 << 20;
        run("a base of 2^20", c, kForGood);
    }
    {
        gunlearn::Capture c = base;
        c.paletteBones = 2;
        run("a palette of 2 bones (its vertices name bones to 5)", c, kTillNext);
    }
    {
        gunlearn::Capture c = base;
        c.vbOffset = c.vbBytes;
        run("vertices from its buffer's end", c, kForGood);
        c = base;
        c.vbBytes = 40;
        run("a vertex buffer shorter than a vertex", c, kForGood);
    }
    {
        gunlearn::Capture c = base;
        c.count = 3001;
        run("not whole triangles", c, kForGood);
        c = gunlearn::Capture{};
        run("nothing read back", c, kTillNext);
    }
    // What the palette drawn with it did that once: its frame bone (0) not rigid (its turn doubled), or not numbers. Another capture may be fine.
    {
        std::vector<float> odd(Palette(an94), Palette(an94) + size_t(base.paletteBones) * 12);
        for (int k = 0; k < 12; ++k)
            if (k % 4 != 3)
                odd[size_t(k)] *= 2.0f;
        gunlearn::Capture c = base;
        c.palette = odd.data();
        run("its frame bone not rigid in the palette drawn with it (its turn doubled)", c, kTillNext);
        odd[0] = std::nanf("");
        run("its frame bone's matrix not numbers", c, kTillNext);
    }
}

// ---------------------------------------------------------------- l5-l8: the pure steps
void Identity(float* m) {
    const float id[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    std::memcpy(m, id, sizeof(id));
}
void MatchChecks() {
    float palette[64][12];
    for (int b = 0; b < 64; ++b) {
        Identity(palette[b]);
        palette[b][3] = 0.1f * static_cast<float>(b), palette[b][7] = 1.8f, palette[b][11] = -0.01f * static_cast<float>(b);
    }
    float frame[12];
    std::memcpy(frame, palette[9], sizeof(frame));
    CheckF(gunlearn::MatchBone(frame, &palette[0][0], 60) == 9, "l5 the frame's matrix found at bone %d (9)", gunlearn::MatchBone(frame, &palette[0][0], 60));
    float none[12];
    std::memcpy(none, frame, sizeof(none));
    none[7] += 0.5f;
    CheckF(gunlearn::MatchBone(none, &palette[0][0], 60) == -1, "l5 no bone equal: %d (-1)", gunlearn::MatchBone(none, &palette[0][0], 60));
    std::memcpy(palette[4], frame, sizeof(frame));
    CheckF(gunlearn::MatchBone(frame, &palette[0][0], 60) == 4, "l5 two equal (4 and 9): the first, %d", gunlearn::MatchBone(frame, &palette[0][0], 60));
    float within[12], beyond[12];  // (windows.h takes the words near and far)
    std::memcpy(within, frame, sizeof(within));
    std::memcpy(beyond, frame, sizeof(beyond));
    within[5] += 0.5e-5f, beyond[5] += 3e-5f;
    CheckF(gunlearn::MatchBone(within, &palette[0][0], 60) == 4 && gunlearn::MatchBone(beyond, &palette[0][0], 60) == -1,
           "l5 within 1e-5 equal (%d), 3e-5 off not (%d)", gunlearn::MatchBone(within, &palette[0][0], 60), gunlearn::MatchBone(beyond, &palette[0][0], 60));
    float nan[12];
    std::memcpy(nan, frame, sizeof(nan));
    nan[0] = std::nanf("");
    CheckF(gunlearn::MatchBone(nan, &palette[0][0], 60) == -1, "l5 a frame with a number that isn't: %d (-1)", gunlearn::MatchBone(nan, &palette[0][0], 60));
    CheckF(gunlearn::MatchBone(frame, &palette[0][0], 4) == -1, "l5 only the bones given looked at (4: bones 0-3): %d", gunlearn::MatchBone(frame, &palette[0][0], 4));
    CheckF(gunlearn::MatchBone(frame, nullptr, 60) == -1 && gunlearn::MatchBone(nullptr, &palette[0][0], 60) == -1, "l5 nothing given: -1");
    // A real depth pass (the M16A4's): its bones 0 and 1 hold one matrix; the first of them.
    if (const Dump m16 = LoadDump(18675, 18675); m16.ok) {
        const float* p = Palette(m16);
        CheckF(gunlearn::MatchBone(p + 12, p, kPaletteBones) == 0 && gunlearn::MatchBone(p + 4 * 12, p, kPaletteBones) == 4,
               "l5 the M16A4's depth pass: its bone 1's matrix first at %d (0, its twin), bone 4's at %d", gunlearn::MatchBone(p + 12, p, kPaletteBones),
               gunlearn::MatchBone(p + 4 * 12, p, kPaletteBones));
    } else {
        Skip("l5: no dump of the M16A4's depth pass (mesh_18675_18675_*)");
    }
}

void SizeChecks() {
    struct Case {
        const char* what;
        float lo[3], hi[3];
        bool gun;
    };
    const Case cases[] = {
        {"a grenade, 0.25 m", {-0.04f, -0.04f, 0.0f}, {0.04f, 0.04f, 0.25f}, false},
        {"0.29 m", {-0.02f, -0.08f, 0.0f}, {0.02f, 0.05f, 0.29f}, false},
        {"0.30 m", {-0.02f, -0.08f, 0.0f}, {0.02f, 0.05f, 0.30f}, true},
        {"a rifle, 0.5 m", {-0.03f, -0.2f, -0.1f}, {0.03f, 0.1f, 0.4f}, true},
        {"longer across than along (0.6 m across, 0.4 along)", {-0.3f, -0.05f, 0.0f}, {0.3f, 0.05f, 0.4f}, false},
        {"higher than long (0.5 m up, 0.4 along)", {-0.03f, -0.25f, 0.0f}, {0.03f, 0.25f, 0.4f}, false},
        {"a number that isn't", {-0.03f, -0.2f, std::nanf("")}, {0.03f, 0.1f, 0.4f}, false},
    };
    for (const Case& c : cases)
        CheckF(gunlearn::GunSized(c.lo, c.hi) == c.gun, "l6 %s: %s", c.what, gunlearn::GunSized(c.lo, c.hi) ? "a gun" : "not a gun");
    CheckF(!gunlearn::GunSized(nullptr, nullptr), "l6 nothing given: not a gun");
    const Synthetic grenade = MakeBox(0.07f, 0.07f, 0.25f, 700);
    gunlearn::Fitted f;
    const bool ok = gunlearn::FitCapture(CaptureOf(grenade), f);
    CheckF(!ok, "l6 a grenade-sized mesh (0.25 m) read: %s %s", ok ? "taken" : "refused:", ok ? "" : f.why);
}

void RowChecks(const gunlearn::Fitted& an94Fit) {
    gunlearn::Saved s;
    if (an94Fit.ok) {
        s = SavedOf(an94Fit, 16188, 0);
    } else {
        s.row.bodyIndices = 16188, s.row.frameBone = 0, s.row.depthIndices = 16590, s.row.depthStart = 16590, s.row.depthFrame = 0, s.row.vertices = 7187;
        s.row.palms = true;  // (a saved gun always has them)
        s.boxMax[0] = s.boxMax[1] = s.boxMax[2] = 0.5f, s.mass = 3.5f, s.hullPoints = 4;
    }
    // Numbers that are awkward to write: a third, a tenth, negative zero, tiny, and the sights learned.
    s.row.up = 1.0f / 3.0f, s.row.sightsLearned = true;
    s.row.sights[0] = -0.0f, s.row.sights[1] = 0.1f, s.row.sights[2] = 1e-7f;
    s.row.indexHash = 0xFFFFFFFFu;
    s.keelX = 0.006f;
    const std::string line = gunlearn::FormatRow(s);
    gunlearn::Saved back;
    const bool parsed = gunlearn::ParseRow(line.c_str(), back);
    back.row.body = s.row.body;
    CheckF(parsed && SameSaved(s, back) && gunlearn::FormatRow(back) == line, "l7 a saved gun read back to the bit (%zu characters)", line.size());
    CheckF(parsed && back.row.palms && !back.row.ready && !back.row.body, "l7 read back: its palms known, not ready (its key's checked first), no body yet");
    // Broken lines.
    struct Broken {
        const char* what;
        std::string line;
    };
    std::vector<Broken> broken = {
        {"cut short", line.substr(0, line.size() / 2)},
        {"a word more", line + " more"},
        {"no key", line.substr(0, line.find(" key")) + line.substr(line.find(" frame"))},
        {"frame bone 64", Bump(line, "frame", 0, 64 - s.row.frameBone)},
        {"depth bone -1", Bump(line, "depth", 2, -1 - s.row.depthFrame)},
        {"1999 indices", Bump(line, "gun", 0, 1999 - 16188)},
        {"16189 indices (not whole triangles)", Bump(line, "gun", 0, 1)},
        {"23 hull points", Bump(line, "hull", 0, 23 - s.hullPoints)},
        {"3 hull points", Bump(line, "hull", 0, 3 - s.hullPoints)},
    };
    {
        std::string nan = line;
        const size_t at = nan.find(" mass ") + 6;
        nan = nan.substr(0, at) + "nan" + nan.substr(nan.find(' ', at));
        broken.push_back({"its mass not a number", nan});
        std::string inf = line;
        const size_t up = inf.find(" up ") + 4;
        inf = inf.substr(0, up) + "inf" + inf.substr(inf.find(' ', up));
        broken.push_back({"its up infinite", inf});
        std::string zero = line;
        zero = zero.substr(0, at) + "0" + zero.substr(zero.find(' ', at));
        broken.push_back({"its mass 0", zero});
        std::string box = line;
        const size_t b = box.find(" box ") + 5;
        box = box.substr(0, b) + "0.9" + box.substr(box.find(' ', b));  // its box's least x past its most
        broken.push_back({"its box inside out", box});
    }
    for (const Broken& b : broken) {
        gunlearn::Saved x;
        CheckF(!gunlearn::ParseRow(b.line.c_str(), x), "l7 a line %s: refused", b.what);
    }
    gunlearn::Refused r;
    r.count = 3135, r.start = 0, r.vertices = 1200;
    std::snprintf(r.why, sizeof(r.why), "too short for a gun");
    gunlearn::Refused rb;
    const std::string refused = gunlearn::FormatRefused(r);
    CheckF(gunlearn::ParseRefused(refused.c_str(), rb) && rb.count == r.count && rb.start == r.start && rb.vertices == r.vertices &&
               std::strcmp(rb.why, r.why) == 0,
           "l7 a refused mesh read back: \"%s\"", refused.c_str());
    gunlearn::Refused z;
    CheckF(!gunlearn::ParseRefused("refused 0 0 10 why", z) && !gunlearn::ParseRefused("refused 3135 0 0 why", z) && !gunlearn::ParseRefused("refused x", z) &&
               !gunlearn::ParseRow(refused.c_str(), back) && !gunlearn::ParseRefused(line.c_str(), z),
           "l7 refused lines with no count or vertices, and the two kinds of line, told apart");
}

void WatchChecks() {
    // The AN-94's colour body every frame, its depth pass (bigger) every frame, a bigger colour draw half the frames, a small part every frame.
    gunlearn::Watch w;
    int picked = -2;
    uint32_t frame = 100;
    for (int k = 0; k <= gunlearn::kWindow; ++k, ++frame) {
        if (w.Closes(frame)) {
            picked = w.Pick(nullptr, 0);
            break;
        }
        w.Note(16590, 16590, 0, frame, false);
        w.Note(16188, 0, 0, frame, true);
        if (k % 2 == 0)
            w.Note(30000, 0, 0, frame, true);
        w.Note(312, 16278, 0, frame, true);
        w.Note(16188, 0, 0, frame, true);  // a second view in the same frame: still one frame
    }
    CheckF(picked >= 0 && w.seen[picked].count == 16188 && w.seen[picked].start == 0 && w.seen[picked].frames == 30,
           "l8 the body picked: %u@%u, drawn in %u of 30 frames (over its depth pass 16590 every frame and 30000 half the frames)",
           picked >= 0 ? w.seen[picked].count : 0, picked >= 0 ? w.seen[picked].start : 0, picked >= 0 ? w.seen[picked].frames : 0);
    CheckF(!w.Closes(100 + 29) && w.Closes(100 + 30), "l8 the window closes at its 30th frame after it opened, not before");
    // 26 of 30 frames: none; 27: picked.
    for (int seenIn : {26, 27}) {
        gunlearn::Watch v;
        for (uint32_t f = 0; f < 30; ++f)
            if (static_cast<int>(f) < seenIn)
                v.Note(16188, 0, 0, 1000 + f, true);
            else
                v.Note(5000, 0, 0, 1000 + f, true);
        const int p = v.Pick(nullptr, 0);
        CheckF(seenIn == 26 ? p < 0 || v.seen[p].count != 16188 : p >= 0 && v.seen[p].count == 16188, "l8 drawn in %d of 30 frames: %s", seenIn,
               p >= 0 && v.seen[p].count == 16188 ? "picked" : "not picked");
    }
    // Skipped (learned already, or refused): the next largest.
    {
        gunlearn::Watch v;
        for (uint32_t f = 0; f < 30; ++f)
            v.Note(16188, 0, 0, f, true), v.Note(10599, 72, 0, f, true), v.Note(1500, 0, 0, f, true);
        const uint32_t skip[1][2] = {{16188, 0}};
        const int p = v.Pick(skip, 1);
        CheckF(p >= 0 && v.seen[p].count == 10599, "l8 the body skipped: the next largest, %u (draws under 2000 indices never)", p >= 0 ? v.seen[p].count : 0);
        const uint32_t both[2][2] = {{16188, 0}, {10599, 72}};
        CheckF(v.Pick(both, 2) < 0, "l8 both skipped: none (the 1500-index draw is a part)");
    }
    // Depth-only draws never: a window of the depth pass alone picks nothing.
    {
        gunlearn::Watch v;
        for (uint32_t f = 0; f < 30; ++f)
            v.Note(16590, 16590, 0, f, false);
        CheckF(v.Pick(nullptr, 0) < 0, "l8 a depth pass alone (no render targets): never the body");
    }
    // The frame count wraps.
    {
        gunlearn::Watch v;
        const uint32_t from = 0xFFFFFFF0u;
        for (uint32_t f = 0; f < 30; ++f)
            v.Note(16188, 0, 0, from + f, true);
        const int p = v.Pick(nullptr, 0);
        CheckF(p >= 0 && v.seen[p].frames == 30 && v.Closes(from + 30) && !v.Closes(from + 29), "l8 across the frame count's wrap: drawn in %u of 30, the "
               "window closing at its 30th", p >= 0 ? v.seen[p].frames : 0);
    }
}

// ---------------------------------------------------------------- l9: threads
void ThreadChecks(const gunlearn::Fitted& an94Fit) {
    if (!an94Fit.ok) {
        Skip("l9: no AN-94 fit to publish");
        return;
    }
    gunlearn::Forget();
    WriteText(g_store, gunlearn::FormatRow(SavedOf(an94Fit, 16188, 0)) + "\n");
    gunlearn::Set(1, 0);
    for (int k = 0; k < 400 && !gunlearn::Loaded(); ++k)
        gunlearn::OnFrame(nullptr, nullptr), Sleep(5);  // the file read, on a thread of its own
    const gunlearn::Row* first = gunlearn::ByBody(16188);
    Check(first && !first->ready && first->body, "l9 the gun saved before is a row (not ready: its key's not checked) with its body");
    if (!first)
        return;
    // Palms k: right (a, -a, 0.5 + a), left that plus 0.25 each way, a = 2 mm * (k % 1000): read whole, they agree.
    auto palms = [](int k, float* right, float* left) {
        const float a = 0.002f * static_cast<float>(k % 1000);
        right[0] = a, right[1] = -a, right[2] = 0.5f + a;
        left[0] = a + 0.25f, left[1] = -a + 0.25f, left[2] = 0.75f + a;
    };
    auto whole = [](const gunlearn::Row& r) {
        const float a = r.rightPalm[0];
        return std::fabs(r.rightPalm[1] + a) < 1e-5f && std::fabs(r.rightPalm[2] - 0.5f - a) < 1e-5f && std::fabs(r.leftPalm[0] - a - 0.25f) < 1e-5f &&
               std::fabs(r.leftPalm[1] + a - 0.25f) < 1e-5f && std::fabs(r.leftPalm[2] - 0.75f - a) < 1e-5f && r.bodyIndices == 16188 && r.frameBone == 0 &&
               r.depthIndices == 16590 && r.body;
    };
    {
        float right[3], left[3];
        palms(0, right, left);
        gunlearn::NotePalms(16188, right, left);
    }
    std::atomic<bool> stop{false};
    std::atomic<int> done{0};
    std::atomic<long long> reads{0}, torn{0}, published{0};
    std::mutex hands;  // skin.cpp's g_handsMutex, as it's taken: first, then gunlearn's inside NotePalms
    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t)
        threads.emplace_back([&, t] {
            long long n = 0, bad = 0;
            while (!stop.load()) {
                // As skin.cpp reads a learned gun (HeldKindNow, DrawHeldPart's recognition): its row, by its body and by its draws.
                const gunlearn::Row* r = (t & 1) ? gunlearn::ByBody(16188) : gunlearn::Find(16188, 0);
                const gunlearn::Row* d = gunlearn::Find(16590, 16590);
                if (!r || !d || !whole(*r) || !whole(*d))
                    ++bad;
                ++n;
            }
            reads += n, torn += bad;
            ++done;
        });
    threads.emplace_back([&] {
        float right[3], left[3];
        for (int k = 1; k <= 10000; ++k) {
            palms(k, right, left);
            gunlearn::NotePalms(16188, right, left);
            ++published;
        }
        ++done;
    });
    threads.emplace_back([&] {
        float right[3], left[3];
        for (int k = 0; !stop.load(); ++k) {
            std::lock_guard lock(hands);
            palms(500 + k, right, left);
            gunlearn::NotePalms(16188, right, left);
            (void)gunlearn::ByBody(16188);
        }
        ++done;
    });
    threads.emplace_back([&] {
        float palette[64][12];
        for (auto& m : palette)
            Identity(m);
        for (uint32_t frame = 0; !stop.load(); ++frame) {
            gunlearn::NoteDraw(nullptr, 16188, 0, 0, &palette[0][0], 60, frame, frame);
            gunlearn::NoteDraw(nullptr, 16590, 16590, 0, &palette[0][0], 60, frame, frame);
            gunlearn::NoteDraw(nullptr, 9000, 0, 0, &palette[0][0], 60, frame, frame);
            gunlearn::OnFrame(nullptr, nullptr);
        }
        ++done;
    });
    // The writer's 10000, then a little more of everything; all of it within 60 s, or it's stuck.
    const auto began = std::chrono::steady_clock::now();
    while (published.load() < 10000 && std::chrono::steady_clock::now() - began < std::chrono::seconds(60))
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    stop = true;
    while (done.load() < static_cast<int>(threads.size()) && std::chrono::steady_clock::now() - began < std::chrono::seconds(60))
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    if (done.load() < static_cast<int>(threads.size())) {
        Check(false, "l9 the threads finished: STUCK (a deadlock): stopping here");
        std::fflush(stdout);
        ExitProcess(static_cast<UINT>(g_failures));
    }
    for (std::thread& t : threads)
        t.join();
    CheckF(torn.load() == 0 && reads.load() > 1000, "l9 8 threads read the row %lld times while it was published %lld times and more: %lld torn",
           reads.load(), published.load(), torn.load());
    Check(true, "l9 a thread holding the hands' lock first, then the learner's (NotePalms), with OnFrame and NoteDraw running: no deadlock");
    gunlearn::Set(0, 0);
    gunlearn::OnFrame(nullptr, nullptr);
    gunlearn::Forget();
}

void M240Checks(const Dump& m240, const gunlearn::Fitted& m240Fit) {
    gunlearn::Watch w;
    int picked = -1;
    for (uint32_t f = 0; f <= 30; ++f) {
        if (w.Closes(500 + f)) {
            picked = w.Pick(nullptr, 0);
            break;
        }
        w.Note(29952, 29952, 0, 500 + f, false);  // its depth pass, bigger, no render targets
        w.Note(2304, 0, 0, 500 + f, true);        // its belt, first in the buffer
        w.Note(60, 2304, 0, 500 + f, true);       // its box's tape
        w.Note(27588, 2364, 0, 500 + f, true);    // its body, from 2364
    }
    CheckF(picked >= 0 && w.seen[picked].count == 27588 && w.seen[picked].start == 2364, "l10 the M240: %u@%u picked (its body, over its belt 2304@0)",
           picked >= 0 ? w.seen[picked].count : 0, picked >= 0 ? w.seen[picked].start : 0);
    if (!m240.ok) {
        Skip("l10: no M240 dump (mesh_27588_2364_*)");
        return;
    }
    CheckF(m240Fit.ok && m240Fit.frameBone == 3, "l10 the M240 learned from its body draw: frame bone %d (3), %.2f m long, %d hull points", m240Fit.frameBone,
           m240Fit.keptMax[2] - m240Fit.keptMin[2], m240Fit.hullPoints);
}

void ShortGunChecks() {
    for (const float length : {0.32f, 0.29f}) {
        const Synthetic gun = MakeBox(0.04f, 0.10f, length, 700);
        gunlearn::Fitted f;
        const bool ok = gunlearn::FitCapture(CaptureOf(gun), f);
        CheckF(ok == (length >= 0.30f), "l11 a %.2f m gun: %s%s (kept %.3f m long)", length, ok ? "taken" : "refused: ", ok ? "" : f.why,
               f.keptMax[2] - f.keptMin[2]);
    }
}

// ---------------------------------------------------------------- l13: a narrow bottom stood upright
void UprightChecks() {
    // A receiver 2 cm wide, 10 cm high and 50 cm long with a magazine under it 15 cm deep: 3.6 cm wide (more than 5 mm a side wider than the
    // receiver: bodyfit keeps its bottom's corners where they are), then 2 cm (no wider: its bottom goes onto the keel). Both bottoms are under
    // 4 cm wide: neither gun may ever stand.
    const float receiverLo[3] = {-0.01f, -0.05f, -0.1f}, receiverHi[3] = {0.01f, 0.05f, 0.4f};
    for (const float half : {0.018f, 0.010f}) {
        const float magazineLo[3] = {-half, -0.20f, 0.10f}, magazineHi[3] = {half, -0.05f, 0.16f};
        const Synthetic gun = MakeTwoBoxes(receiverLo, receiverHi, magazineLo, magazineHi, 700);
        gunlearn::Fitted f;
        const bool ok = gunlearn::FitCapture(CaptureOf(gun), f);
        const bool wide = half > 0.015f;
        const bool right = wide ? !ok && !f.forGood && std::strstr(f.why, "stand") : ok;
        CheckF(right, "l13 a magazine %.1f cm wide under a 2 cm receiver (a narrow bottom): %s%s%s", half * 200.0f,
               ok ? "taken, upright on its keel alone" : "refused: ", ok ? "" : f.why, ok ? "" : f.forGood ? " (for good)" : " (till the next launch)");
    }
}

// ---------------------------------------------------------------- l12: the learner on a software graphics card
using CreateDeviceFn = HRESULT(WINAPI*)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL*, UINT, UINT, ID3D11Device**, D3D_FEATURE_LEVEL*,
                                        ID3D11DeviceContext**);
struct Card {
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    ID3D11RenderTargetView* target = nullptr;
};
bool MakeCard(Card& c) {
    HMODULE lib = LoadLibraryW(L"d3d11.dll");
    const auto create = lib ? reinterpret_cast<CreateDeviceFn>(GetProcAddress(lib, "D3D11CreateDevice")) : nullptr;
    if (!create || FAILED(create(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &c.device, nullptr, &c.ctx)))
        return false;
    D3D11_TEXTURE2D_DESC td{4, 4, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, {1, 0}, D3D11_USAGE_DEFAULT, D3D11_BIND_RENDER_TARGET, 0, 0};
    ID3D11Texture2D* tex = nullptr;
    if (FAILED(c.device->CreateTexture2D(&td, nullptr, &tex)))
        return false;
    const bool ok = SUCCEEDED(c.device->CreateRenderTargetView(tex, nullptr, &c.target));
    tex->Release();
    return ok;
}
ID3D11Buffer* MakeBuffer(ID3D11Device* device, const std::vector<uint8_t>& data, UINT bind) {
    D3D11_BUFFER_DESC bd{static_cast<UINT>(data.size()), D3D11_USAGE_DEFAULT, bind, 0, 0, 0};
    D3D11_SUBRESOURCE_DATA sd{data.data(), 0, 0};
    ID3D11Buffer* b = nullptr;
    return SUCCEEDED(device->CreateBuffer(&bd, &sd, &b)) ? b : nullptr;
}
ULONG Refs(IUnknown* u) {
    u->AddRef();
    return u->Release();
}

struct Scene {
    Card card;
    ID3D11Buffer* ib = nullptr;
    ID3D11Buffer* vb = nullptr;
    uint32_t count = 0, start = 0, depthCount = 0, depthStart = 0;
    const float* palette = nullptr;
    std::vector<float> depthPalette;
    uint32_t frame = 1000, view = 5000;
    double slowest = 0.0;  // ms: OnFrame's
    bool depth = true;
    void Bind(bool colour, UINT stride) {
        ID3D11DeviceContext* c = card.ctx;
        c->IASetIndexBuffer(ib, DXGI_FORMAT_R16_UINT, 0);
        const UINT offset = 0;
        c->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        if (colour)
            c->OMSetRenderTargets(1, &card.target, nullptr);
        else
            c->OMSetRenderTargets(0, nullptr, nullptr);
    }
    // One frame as the game draws the gun: its depth pass first (no render targets), then its colour body, in one view; then the frame's end.
    void Frame(bool end = true) {
        ++frame, ++view;
        if (depth && depthCount) {
            Bind(false, 16);
            gunlearn::NoteDraw(card.ctx, depthCount, depthStart, 0, depthPalette.data(), kPaletteBones, frame, view);
        }
        Bind(true, 48);
        gunlearn::NoteDraw(card.ctx, count, start, 0, palette, kPaletteBones, frame, view);
        if (end)
            End();
    }
    void End() {
        const auto t0 = std::chrono::steady_clock::now();
        gunlearn::OnFrame(card.device, card.ctx);
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        if (ms > slowest)
            slowest = ms, slowestAt = frame;
        if (ms > 8.0)
            Info("l12 frame %u's OnFrame took %.1f ms", frame, ms);
    }
    uint32_t slowestAt = 0;
    // Frames' ends (no draws) until done() or two seconds: for what runs on threads of its own (the fit, the file).
    template <typename Done>
    bool Await(Done done) {
        for (int k = 0; k < 400; ++k) {
            End();
            if (done())
                return true;
            Sleep(5);
        }
        return false;
    }
    // Frames until the gun's row is ready (its palms noted once its depth is known, as the rig would at rest), at most `most`.
    int UntilReady(int most, const float* right, const float* left) {
        for (int k = 0; k < most; ++k) {
            Frame();
            if (const gunlearn::Row* r = gunlearn::Find(count, start)) {
                if (r->ready)
                    return k + 1;
                if (r->depthIndices && !r->palms)
                    gunlearn::NotePalms(count, right, left);
            }
            Sleep(1);
        }
        return -1;
    }
};

// A graphics card refusing every readback (as a lost device does): the learner's Map (gunlearn::SetMap) failing, and its Unmap.
std::atomic<int> g_failedMaps{0};
HRESULT STDMETHODCALLTYPE FailedMap(ID3D11DeviceContext*, ID3D11Resource*, UINT, D3D11_MAP, UINT, D3D11_MAPPED_SUBRESOURCE*) {
    g_failedMaps.fetch_add(1);
    return DXGI_ERROR_DEVICE_REMOVED;
}
void STDMETHODCALLTYPE NoUnmap(ID3D11DeviceContext*, ID3D11Resource*, UINT) {}

void CardChecks(const Dump& an94, const gunlearn::Fitted& an94Fit) {
    if (!an94.ok || !an94Fit.ok) {
        Skip("l12: no AN-94 dump");
        return;
    }
    Scene s;
    if (!MakeCard(s.card)) {
        Skip("l12: no software graphics card (WARP) here");
        return;
    }
    s.ib = MakeBuffer(s.card.device, an94.ib, D3D11_BIND_INDEX_BUFFER);
    s.vb = MakeBuffer(s.card.device, an94.vb, D3D11_BIND_VERTEX_BUFFER);
    if (!s.ib || !s.vb) {
        Skip("l12: the buffers couldn't be made");
        return;
    }
    s.count = 16188, s.start = 0, s.depthCount = 16590, s.depthStart = 16590;  // its depth pass: the same index buffer, after the body
    s.palette = Palette(an94);
    // The depth pass's palette: the same bones in another order, the frame (the colour pass's bone 0) at 9.
    s.depthPalette.assign(64 * 12, 0.0f);
    for (int b = 0; b < 64; ++b)
        std::memcpy(&s.depthPalette[size_t(b) * 12], s.palette + size_t((b + 64 - 9) % 64) * 12, 48);
    const float right[3] = {-0.023f, -0.059f, 0.192f}, left[3] = {0.002f, -0.018f, 0.561f};
    DeleteFileW(g_store.c_str());
    gunlearn::Forget();
    gunlearn::Set(1, 0);
    const uint32_t readsBefore = gunlearn::Readbacks();
    const int frames = s.UntilReady(400, right, left);
    const gunlearn::Row* r = gunlearn::Find(16188, 0);
    CheckF(frames > 0 && r && r->ready, "l12 a gun never seen: watched, read back, fitted, its depth pass, its palms: held after %d frames", frames);
    if (!r || !r->ready)
        return;
    CheckF(gunlearn::Readbacks() == readsBefore + 1, "l12 its mesh read back once (%u)", gunlearn::Readbacks() - readsBefore);
    CheckF(r->frameBone == 0 && r->bodyStart == 0 && r->depthIndices == 16590 && r->depthStart == 16590 && r->depthFrame == 9,
           "l12 frame bone %d (0), depth %u from %u bone %d (16590 from 16590, bone 9: the frame's matrix there in each view)", r->frameBone, r->depthIndices,
           r->depthStart, r->depthFrame);
    CheckF(r->vertices == an94Fit.vertices && r->indexHash == an94Fit.indexHash && Distance(r->rightPalm, right) == 0.0f && Distance(r->leftPalm, left) == 0.0f,
           "l12 its key (%u vertices, hash %08x) and palms as given", r->vertices, r->indexHash);
    bool bodySame = r->body && r->body->hullPoints == an94Fit.hullPoints && std::memcmp(r->body->boxMin, an94Fit.boxMin, sizeof(an94Fit.boxMin)) == 0 &&
                    std::memcmp(r->body->boxMax, an94Fit.boxMax, sizeof(an94Fit.boxMax)) == 0 && r->body->keep;
    for (int i = 0; bodySame && i < an94Fit.hullPoints; ++i)
        bodySame = std::memcmp(r->body->hull[i], an94Fit.hull[i], sizeof(float) * 3) == 0;
    CheckF(bodySame && r->body->name && std::strcmp(r->body->name, "learned16188gun") == 0, "l12 its loose body: the fit's from the dump to the bit, \"%s\", "
           "kept till taken", r->body ? r->body->name : "");
    CheckF(gunlearn::Find(16590, 16590) == r && gunlearn::ByBody(16188) == r, "l12 found by its depth pass and by its body too");
    // Switched off ("learnguns 0"): no row found, by its draws or its body (the gun the game's own again, as at once as the switch); on again, its
    // row back as it was, held, nothing read back again.
    {
        const uint32_t readsAt = gunlearn::Readbacks();
        gunlearn::Set(0, 0);
        const bool goneAtOnce = !gunlearn::Find(16188, 0) && !gunlearn::Find(16590, 16590) && !gunlearn::ByBody(16188);
        for (int k = 0; k < 5; ++k)
            s.Frame();
        const bool goneAfter = !gunlearn::Find(16188, 0) && !gunlearn::Find(16590, 16590) && !gunlearn::ByBody(16188);
        gunlearn::Set(1, 0);
        s.End();
        const gunlearn::Row* back = gunlearn::Find(16188, 0);
        CheckF(goneAtOnce && goneAfter && back && back->ready && back->depthFrame == 9 && gunlearn::ByBody(16188) == back && gunlearn::Readbacks() == readsAt,
               "l12 switched off: no row found (the gun the game's own); on again: its row back, held, not read back again");
        r = back ? back : r;
    }
    // Saved (on a thread of its own).
    std::vector<std::string> guns;
    s.Await([&] { return !(guns = Lines(ReadText(g_store), "gun ")).empty(); });
    gunlearn::Saved saved;
    CheckF(guns.size() == 1 && gunlearn::ParseRow(guns[0].c_str(), saved) && saved.row.depthFrame == 9 && saved.row.vertices == r->vertices,
           "l12 saved: %zu gun lines in the file, read back", guns.size());
    if (guns.empty())
        return;
    // The next launch: held again once its key's checked (no mesh read back).
    gunlearn::Forget();
    s.Await([] { return gunlearn::Loaded(); });
    r = gunlearn::Find(16188, 0);
    Check(r && !r->ready, "l12 the next launch: the gun saved is a row, not held till its key's checked");
    const uint32_t readsThen = gunlearn::Readbacks();
    const int checked = s.UntilReady(20, right, left);
    r = gunlearn::Find(16188, 0);
    CheckF(checked > 0 && r && r->ready && gunlearn::Readbacks() == readsThen && r->depthFrame == 9,
           "l12 its key checked (a 4 KB readback of its first indices): held after %d frames, its mesh not read again", checked);
    // Its own indices drawn depth-only before its colour draw, from another vertex layout (stride 16: another vertex count): its key is its
    // colour draw's, so it waits for that one (not dropped and learned again).
    {
        gunlearn::Forget();
        WriteText(g_store, guns[0] + "\n");
        s.Await([] { return gunlearn::Loaded(); });
        const uint32_t depthCount = s.depthCount, depthStart = s.depthStart, readsAt = gunlearn::Readbacks();
        s.depthCount = s.count, s.depthStart = s.start;
        const int held = s.UntilReady(20, right, left);
        s.depthCount = depthCount, s.depthStart = depthStart;
        CheckF(held > 0 && gunlearn::Readbacks() == readsAt,
               "l12 its own indices drawn depth-only first (stride 16: %u vertices, not %u): its key checked on its colour draw, held after %d frames, "
               "not learned again", static_cast<uint32_t>(an94.vb.size() / 16), r ? r->vertices : 0, held);
    }
    // A changed key: learned again.
    for (const char* change : {"hash", "vertices"}) {
        gunlearn::Forget();
        WriteText(g_store, std::strcmp(change, "hash") == 0 ? Bump(guns[0], "key", 1, 1) : Bump(guns[0], "key", 0, 1));
        s.Await([] { return gunlearn::Loaded(); });
        const gunlearn::Row* loaded = gunlearn::Find(16188, 0);
        const gunlearn::Row* after = loaded;
        int dropped = 0;
        for (; dropped < 60 && after; ++dropped) {
            s.Frame();
            after = gunlearn::Find(16188, 0);
            Sleep(1);
        }
        Info("l12 its saved %s changed: let go after %d frames", change, dropped);
        const uint32_t readsAt = gunlearn::Readbacks();
        const int again = s.UntilReady(400, right, left);
        CheckF(loaded && !after && again > 0 && gunlearn::Readbacks() == readsAt + 1, "l12 its saved %s not this mesh's: not held, learned again (held after %d "
               "more frames)", change, again);
    }
    // A mesh refused (a grenade's size) is never read back again, at this launch or the next.
    {
        const Synthetic grenade = MakeBox(0.07f, 0.07f, 0.25f, 700);
        ID3D11Buffer* gib = MakeBuffer(s.card.device, grenade.ib, D3D11_BIND_INDEX_BUFFER);
        ID3D11Buffer* gvb = MakeBuffer(s.card.device, grenade.vb, D3D11_BIND_VERTEX_BUFFER);
        std::swap(s.ib, gib), std::swap(s.vb, gvb);
        const uint32_t count = s.count;
        const float* palette = s.palette;
        s.count = grenade.count, s.depth = false;
        float identity[64 * 12] = {};
        for (int b = 0; b < 64; ++b)
            Identity(&identity[b * 12]);
        s.palette = identity;
        gunlearn::Forget();
        DeleteFileW(g_store.c_str());
        s.Await([] { return gunlearn::Loaded(); });
        const uint32_t readsAt = gunlearn::Readbacks();
        for (int k = 0; k < 80; ++k)
            s.Frame(), Sleep(1);
        const uint32_t firstReads = gunlearn::Readbacks() - readsAt;
        std::vector<std::string> refused;
        s.Await([&] { return !(refused = Lines(ReadText(g_store), "refused ")).empty(); });
        gunlearn::Forget();
        s.Await([] { return gunlearn::Loaded(); });
        for (int k = 0; k < 80; ++k)
            s.Frame(), Sleep(1);
        CheckF(firstReads == 1 && refused.size() == 1 && gunlearn::Readbacks() - readsAt == 1 && !gunlearn::Find(s.count, 0),
               "l12 a grenade-sized mesh: read back %u time, refused (\"%s\"), and at the next launch never read again (%u in all)", firstReads,
               refused.empty() ? "" : refused[0].c_str(), gunlearn::Readbacks() - readsAt);
        std::swap(s.ib, gib), std::swap(s.vb, gvb);
        gib->Release(), gvb->Release();
        s.count = count, s.palette = palette, s.depth = true;
    }
    // Its frame bone not rigid in the palette it was drawn with at the capture (its turn doubled): what the palette did that once, not what its mesh
    // is. Refused till the next launch only: never kept in heldguns.txt, read back again at the next launch.
    {
        std::vector<float> odd(s.palette, s.palette + size_t(kPaletteBones) * 12);
        for (int k = 0; k < 12; ++k)
            if (k % 4 != 3)
                odd[size_t(k)] *= 2.0f;
        const float* palette = s.palette;
        s.palette = odd.data();
        gunlearn::Forget();
        DeleteFileW(g_store.c_str());
        s.Await([] { return gunlearn::Loaded(); });
        const uint32_t readsAt = gunlearn::Readbacks();
        for (int k = 0; k < 80; ++k)
            s.Frame();  // (copied at its window's end, on this thread: the fit after on its own)
        const uint32_t first = gunlearn::Readbacks() - readsAt;
        for (int k = 0; k < 30; ++k)
            s.End(), Sleep(5);  // (its copy back, the fit done, and a save, were one wanted, written meanwhile)
        const std::string text = ReadText(g_store);
        const bool keptNone = Lines(text, "refused ").empty() && Lines(text, "gun ").empty();
        gunlearn::Forget();
        s.Await([] { return gunlearn::Loaded(); });
        for (int k = 0; k < 80; ++k)
            s.Frame();
        const uint32_t all = gunlearn::Readbacks() - readsAt;
        s.palette = palette;
        CheckF(first == 1 && keptNone && all == 2 && !gunlearn::Find(16188, 0), "l12 its frame bone not rigid in the palette drawn with it: read back %u time, "
               "refused, nothing kept in heldguns.txt; read back again at the next launch (%u in all)", first, all);
    }
    // No room in memory (32 bits): the vertex buffer's copy refused on the thread that presents (344976 bytes), then the fit's vectors on its own
    // thread (60 KB and more). No exception into the frame or out of the thread; refused till the next launch, nothing kept; learned at the next.
    for (const bool onMain : {true, false}) {
        gunlearn::Forget();
        DeleteFileW(g_store.c_str());
        s.Await([] { return gunlearn::Loaded(); });
        const uint32_t readsAt = gunlearn::Readbacks();
        const int blocksAt = g_refusedBlocks.load();
        g_refuseOnMain = onMain;
        g_refuseFrom = onMain ? 300000 : 60000;
        for (int k = 0; k < 80; ++k)
            s.Frame();
        for (int k = 0; k < 400 && g_refusedBlocks.load() == blocksAt; ++k)
            s.End(), Sleep(5);  // (its copy come back from the graphics card, the fit's thread begun)
        for (int k = 0; k < 4; ++k)
            s.End(), Sleep(5);  // (the refusal taken)
        g_refuseFrom = 0;
        const int blocks = g_refusedBlocks.load() - blocksAt;
        const uint32_t first = gunlearn::Readbacks() - readsAt;
        for (int k = 0; k < 80; ++k)
            s.Frame();
        const uint32_t still = gunlearn::Readbacks() - readsAt;
        for (int k = 0; k < 20; ++k)
            s.End(), Sleep(5);  // (a save, were one wanted, written meanwhile)
        const bool keptNone = Lines(ReadText(g_store), "refused ").empty();
        const bool notLearned = !gunlearn::Find(16188, 0);
        gunlearn::Forget();
        s.Await([] { return gunlearn::Loaded(); });
        const int again = s.UntilReady(400, right, left);
        CheckF(blocks > 0 && first == 1 && still == 1 && keptNone && notLearned && again > 0, "l12 no room in memory for %s (%d blocks refused): no exception, "
               "read back %u time and not learned this launch (%u after 80 frames more), nothing kept; learned at the next launch after %d frames",
               onMain ? "the vertex buffer's copy, on the thread that presents" : "the fit's vectors, on its own thread", blocks, first, still, again);
    }
    // The graphics card refusing every readback (its Map failing): the mesh copied 3 times, then left alone till the next launch, never copied over
    // and over; a saved gun's key check likewise (3 tries, then not held till the next launch, where it is).
    {
        gunlearn::Forget();
        DeleteFileW(g_store.c_str());
        s.Await([] { return gunlearn::Loaded(); });
        const uint32_t readsAt = gunlearn::Readbacks();
        gunlearn::SetMap(reinterpret_cast<void*>(&FailedMap), reinterpret_cast<void*>(&NoUnmap));
        for (int k = 0; k < 300; ++k)
            s.Frame();
        const uint32_t copies = gunlearn::Readbacks() - readsAt;
        gunlearn::SetMap(nullptr, nullptr);
        for (int k = 0; k < 80; ++k)
            s.Frame();
        const uint32_t after = gunlearn::Readbacks() - readsAt;
        CheckF(copies == 3 && after == 3 && !gunlearn::Find(16188, 0),
               "l12 every readback refused by the graphics card: the mesh copied %u times in 300 frames (3), then left alone (%u after 80 frames more with "
               "the card well)", copies, after);
        gunlearn::Forget();
        WriteText(g_store, guns[0] + "\n");
        s.Await([] { return gunlearn::Loaded(); });
        const int mapsAt = g_failedMaps.load();
        gunlearn::SetMap(reinterpret_cast<void*>(&FailedMap), reinterpret_cast<void*>(&NoUnmap));
        for (int k = 0; k < 60; ++k)
            s.Frame();
        const int keyMaps = g_failedMaps.load() - mapsAt;
        gunlearn::SetMap(nullptr, nullptr);
        const int late = s.UntilReady(20, right, left);
        gunlearn::Forget();
        s.Await([] { return gunlearn::Loaded(); });
        const int next = s.UntilReady(20, right, left);
        CheckF(keyMaps == 3 && late < 0 && next > 0, "l12 a saved gun's key check refused by the graphics card: tried %d times in 60 frames (3), then not held "
               "this launch with the card well (%s); held at the next after %d frames", keyMaps, late < 0 ? "left alone" : "HELD", next);
    }
    // A buffer over 4 MB (the M240's 660528 bytes are the biggest gun's): never read back, refused for what it is.
    {
        std::vector<uint8_t> big(size_t(4u << 20) + 64, 0);
        std::memcpy(big.data(), an94.ib.data(), an94.ib.size());
        ID3D11Buffer* bigIb = MakeBuffer(s.card.device, big, D3D11_BIND_INDEX_BUFFER);
        if (!bigIb) {
            Skip("l12: no 4 MB buffer could be made");
        } else {
            std::swap(s.ib, bigIb);
            gunlearn::Forget();
            DeleteFileW(g_store.c_str());
            s.Await([] { return gunlearn::Loaded(); });
            const uint32_t readsAt = gunlearn::Readbacks();
            for (int k = 0; k < 80; ++k)
                s.Frame();
            std::vector<std::string> refused;
            s.Await([&] { return !(refused = Lines(ReadText(g_store), "refused ")).empty(); });
            CheckF(gunlearn::Readbacks() == readsAt && refused.size() == 1 && refused[0].find("too big") != std::string::npos && !gunlearn::Find(16188, 0),
                   "l12 its index buffer %u bytes: never read back, refused for good (\"%s\")", static_cast<uint32_t>(big.size()),
                   refused.empty() ? "" : refused[0].c_str());
            std::swap(s.ib, bigIb);
            bigIb->Release();
        }
    }
    // The test gun ("learnguns test 16188"): learned as new, checked, never saved; forgotten when the test is.
    {
        gunlearn::Forget();
        DeleteFileW(g_store.c_str());
        gunlearn::Set(1, 16188);
        gunlearn::Check c;
        c.bodyIndices = 16188, c.depthIndices = 16590, c.frameBone = 0, c.shape = "an94";
        std::memcpy(c.rightPalm, right, sizeof(right));
        std::memcpy(c.leftPalm, left, sizeof(left));
        gunlearn::SetCheck(c);
        const int test = s.UntilReady(400, right, left);
        Sleep(1100);
        s.End();
        const bool savedNone = Lines(ReadText(g_store), "gun ").empty();
        // Switched off and on again with the same test: the test was over, so it's learned anew (not its row from before).
        gunlearn::Set(0, 0);
        s.End();
        gunlearn::Set(1, 16188);
        s.End();
        const bool forgotten = !gunlearn::Find(16188, 0);
        const uint32_t readsAt = gunlearn::Readbacks();
        const int anew = s.UntilReady(400, right, left);
        CheckF(forgotten && anew > 0 && gunlearn::Readbacks() == readsAt + 1,
               "l12 the test gun switched off and on with the same test: forgotten, learned anew (held after %d frames)", anew);
        gunlearn::Set(1, 0);
        s.End();
        CheckF(test > 0 && savedNone && !gunlearn::Find(16188, 0), "l12 the test gun: held after %d frames, never saved, forgotten when the test is", test);
    }
    // Switched off with its buffers kept for a copy: they're let go.
    {
        gunlearn::Forget();
        DeleteFileW(g_store.c_str());
        gunlearn::Set(1, 0);
        s.Bind(true, 48);
        const ULONG ibRefs = Refs(s.ib), vbRefs = Refs(s.vb);
        s.End();
        for (int k = 0; k < 31; ++k)
            s.Frame(false);  // drawn, no frame's end: the buffers kept, not copied yet
        s.Bind(true, 48);
        const ULONG ibKept = Refs(s.ib), vbKept = Refs(s.vb);
        gunlearn::Set(0, 0);
        s.End();
        s.Bind(true, 48);
        const ULONG ibAfter = Refs(s.ib), vbAfter = Refs(s.vb);
        const uint32_t reads = gunlearn::Readbacks();
        for (int k = 0; k < 40; ++k)
            s.Frame();
        CheckF(ibKept == ibRefs + 1 && vbKept == vbRefs + 1 && ibAfter == ibRefs && vbAfter == vbRefs && gunlearn::Readbacks() == reads && !gunlearn::Find(16188, 0),
               "l12 switched off with a mesh about to be copied: its buffers let go (references %lu, %lu kept, %lu after), nothing more done", ibRefs, ibKept,
               ibAfter);
    }
    CheckF(s.slowest < 16.0, "l12 the slowest frame's work (OnFrame): %.2f ms (under 16)", s.slowest);
    gunlearn::Forget();
    s.ib->Release(), s.vb->Release();
    s.card.target->Release(), s.card.ctx->Release(), s.card.device->Release();
}

}  // namespace

int main(int argc, char** argv) {
    g_mainThread = GetCurrentThreadId();
    // Never beside the game's files: the learner's file goes to the temp folder before anything else runs, its name this run's own (a second run
    // at the same time, the self-test's beside someone's, has its own file).
    wchar_t temp[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, temp);
    g_store = std::wstring(temp) + L"learntest_heldguns_" + std::to_wstring(GetCurrentProcessId()) + L".txt";
    gunlearn::SetStore(g_store.c_str());
    DeleteFileW(g_store.c_str());
    if (argc > 1) {
        g_dir = argv[1];
    } else {
        char self[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, self, MAX_PATH);
        g_dir = self;
        g_dir = g_dir.substr(0, g_dir.find_last_of("\\/"));
    }
    std::printf("Guns learned (offline), the dumps in %s\n", g_dir.c_str());
    Dump dumps[std::size(kGuns)];
    gunlearn::Fitted fits[std::size(kGuns)];
    FitChecks(dumps, fits);
    LayoutChecks(dumps[1], fits[1]);
    MatchChecks();
    SizeChecks();
    RowChecks(fits[1]);
    WatchChecks();
    ThreadChecks(fits[1]);
    M240Checks(dumps[8], fits[8]);
    ShortGunChecks();
    UprightChecks();
    CardChecks(dumps[1], fits[1]);
    gunlearn::Set(0, 0);
    gunlearn::OnFrame(nullptr, nullptr);
    gunlearn::Forget();  // (waits for a write of the file under way)
    DeleteFileW(g_store.c_str());
    DeleteFileW((g_store + L".new").c_str());
    std::printf("%d of %d checks passed\n", g_checks - g_failures, g_checks);
    return g_failures;
}

// Guns never seen before (gunlearn.h): a gun with no table row watched as the game draws it, its body draw found, its mesh read back once, its
// frame bone, its depth pass, its body and the game's palms on it learned; kept in heldguns.txt beside the DLL.
// The readback, as weapon.txt "dump" saves a mesh (skin.cpp SaveBuffer) but never waiting on the graphics card: the buffers are copied in one
// frame and mapped with DO_NOT_WAIT in the frames after (a plain map waits for everything queued: a dropped frame or two), given up after 2 s.
// Nothing is read from them until the index format, the draw's place in its index buffer and every index's place in its vertex buffer are checked.
#include "gunlearn.h"

#include "bodyfit.h"  // before any Windows header: those make the words near and far into nothing

#include <d3d11.h>

#include <algorithm>
#include <atomic>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "gunbodies.h"
#include "log.h"
#include "loose.h"

namespace gunlearn {
namespace {

constexpr uint32_t kR16 = DXGI_FORMAT_R16_UINT, kR32 = DXGI_FORMAT_R32_UINT;
constexpr uint32_t kStrides[] = {48, 16};          // the vertex layouts known: the colour passes' and the depth passes' (meshdump's)
constexpr int kWeightLeast = 250, kWeightMost = 260;
constexpr float kPositionMost = 2.0f;              // m: a vertex's own place, before its bones
constexpr uint32_t kBufferMost = 4u << 20;         // bytes: a buffer bigger isn't read back (some 6 times the M240's 660528, the guns' biggest)
constexpr uint32_t kHashBytes = 4096;
constexpr ULONGLONG kCaptureWait = 2000, kMapWait = 2000;  // ms: for the body's next draw; for the graphics card
constexpr int kGiveUps = 3;                        // readbacks given up on before that draw is left alone this launch
constexpr int kViews = 4, kViewDraws = 24, kDepthViews = 3;
constexpr float kRigid = 0.01f;                    // a frame bone's rows unit and square within this (gunbodyfit's)
constexpr float kMuzzleBand = 0.02f;               // m: the points this near its front give the muzzle's height
constexpr float kSightsLow = 0.05f, kSightsHigh = 0.11f;  // m: its sights' line until learned: the top of its receiver, kept within the known guns'
constexpr float kPort[3] = {-0.017f, 0.05f, 0.25f};       // skin.cpp's kHeldUnknown's, until filmed
constexpr float kMass = 3.5f, kRoundMass = 0.012f;        // kg: a rifle's (the mass hardly matters to loose)
constexpr int kCapacity = 30;
constexpr float kPalmSame = 0.001f;                // m: palms this near the last noted aren't news
constexpr float kPalmsTest = 0.003f, kBodyTest = 0.001f, kUpTest = 0.05f, kSightsTest = 0.002f;
constexpr float kSightEye = 0.0005f, kSightTurnCos = 0.99999994f;  // steady: the eye within 0.5 mm, the look within 0.02 degrees
constexpr ULONGLONG kSightSteady = 500, kSaveEvery = 1000, kDepthSlow = 10000;
constexpr float kPi = 3.14159265f, kDegree = kPi / 180.0f;
// Upright under a narrow bottom (tools/bodytest.cpp b6's rule): rolled up to this either way and pitched up to that in these steps, it touches on its
// keel alone (its lowest hull points within kTouch of each other all on x = keelX), its middle at least kKeelBeside beside the keel.
constexpr int kUprightRoll = 10, kUprightPitch = 15, kUprightPitchStep = 5;
constexpr float kTouch = 0.0001f, kKeelBeside = 0.0039f;
constexpr bool kForGood = true, kThisLaunch = false;  // a refusal kept in heldguns.txt, or till the next launch

using MapFn = HRESULT(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, UINT, D3D11_MAP, UINT, D3D11_MAPPED_SUBRESOURCE*);
using UnmapFn = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, UINT);

// ---------------------------------------------------------------- small math (tools/gunbodyfit.cpp's: a learned gun gets the body gunbodyfit fits)
inline float Half(uint16_t h) {
    int s = (h >> 15) & 1, e = (h >> 10) & 0x1F, m = h & 0x3FF;
    float v = e == 0 ? std::ldexp(m / 1024.0f, -14) : std::ldexp(1 + m / 1024.0f, e - 15);
    return s ? -v : v;
}
// m: 3 rows of rotation | translation.
inline void Apply(const float* m, const float* p, float* o) {
    for (int k = 0; k < 3; ++k)
        o[k] = m[k * 4] * p[0] + m[k * 4 + 1] * p[1] + m[k * 4 + 2] * p[2] + m[k * 4 + 3];
}
// a then b (b's rows into a's space), as tools/gunbodyfit.cpp composes them.
inline void Compose(const float* a, const float* b, float* out) {
    float r[12];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j)
            r[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        r[i * 4 + 3] += a[i * 4 + 3];
    }
    std::memcpy(out, r, sizeof(r));
}
inline void InvertRigid(const float* m, float* inv) {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            inv[i * 4 + j] = m[j * 4 + i];
    for (int i = 0; i < 3; ++i)
        inv[i * 4 + 3] = -(inv[i * 4] * m[3] + inv[i * 4 + 1] * m[7] + inv[i * 4 + 2] * m[11]);
}
inline bool Finite(const float* v, int n) {
    for (int i = 0; i < n; ++i)
        if (!std::isfinite(v[i]))
            return false;
    return true;
}
// A rotation (rows of length 1, square to each other, within kRigid) with a translation.
inline bool Rigid(const float* m) {
    if (!Finite(m, 12))
        return false;
    for (int i = 0; i < 3; ++i)
        for (int j = i; j < 3; ++j) {
            const float d = m[i * 4] * m[j * 4] + m[i * 4 + 1] * m[j * 4 + 1] + m[i * 4 + 2] * m[j * 4 + 2];
            if (std::fabs(d - (i == j ? 1.0f : 0.0f)) > kRigid)
                return false;
        }
    return true;
}
inline float Distance(const float* a, const float* b) {
    const float d[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    return std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
}
inline uint32_t ReadIndex(const uint8_t* at, uint32_t i, uint32_t width) {
    if (width == 2) {
        uint16_t v;
        std::memcpy(&v, at + size_t(i) * 2, 2);
        return v;
    }
    uint32_t v;
    std::memcpy(&v, at + size_t(i) * 4, 4);
    return v;
}
inline uint32_t IndexWidth(uint32_t format) {
    return format == kR16 ? 2 : format == kR32 ? 4 : 0;
}
inline bool StrideKnown(uint32_t stride) {
    return std::find(std::begin(kStrides), std::end(kStrides), stride) != std::end(kStrides);
}
// The draw's indices inside its index buffer.
inline bool DrawInside(uint32_t ibOffset, uint32_t ibBytes, uint32_t start, uint32_t count, uint32_t width) {
    return width && uint64_t(ibOffset) + (uint64_t(start) + count) * width <= ibBytes;
}
// Stood upright, rolled up to kUprightRoll degrees either way and pitched up to kUprightPitch, the body touches on its keel alone (its lowest hull
// points all on x = keelX), its middle beside the keel: it always tips over (tools/bodytest.cpp b6's rule, some 150 tilts of 22 points, once a gun).
bool TipsOver(const bodyfit::Result& b) {
    if (!(std::fabs(b.centre[0] - b.keelX) >= kKeelBeside))
        return false;
    for (int roll = -kUprightRoll; roll <= kUprightRoll; ++roll)
        for (int pitch = -kUprightPitch; pitch <= kUprightPitch; pitch += kUprightPitchStep) {
            const float r = static_cast<float>(roll) * kDegree, p = static_cast<float>(pitch) * kDegree;
            const float down[3] = {std::sin(r) * std::cos(p), -std::cos(r) * std::cos(p), std::sin(p)};  // the ground's way in its own axes
            float most = -1e9f;
            for (int i = 0; i < b.hullPoints; ++i)
                most = std::max(most, b.hull[i][0] * down[0] + b.hull[i][1] * down[1] + b.hull[i][2] * down[2]);
            for (int i = 0; i < b.hullPoints; ++i)
                if (b.hull[i][0] * down[0] + b.hull[i][1] * down[1] + b.hull[i][2] * down[2] > most - kTouch && b.hull[i][0] != b.keelX)
                    return false;
        }
    return true;
}

// ---------------------------------------------------------------- the line's words and numbers (locale-free, exact)
void Put(std::string& s, float v) {
    char buf[32];
    const auto r = std::to_chars(buf, buf + sizeof(buf), v);
    s.push_back(' ');
    s.append(buf, r.ptr);
}
void Put(std::string& s, uint32_t v) {
    char buf[16];
    const auto r = std::to_chars(buf, buf + sizeof(buf), v);
    s.push_back(' ');
    s.append(buf, r.ptr);
}
void Put(std::string& s, int v) {
    char buf[16];
    const auto r = std::to_chars(buf, buf + sizeof(buf), v);
    s.push_back(' ');
    s.append(buf, r.ptr);
}
void Put(std::string& s, const char* word) {
    s.push_back(' ');
    s.append(word);
}
void Put(std::string& s, const float* v, int n) {
    for (int i = 0; i < n; ++i)
        Put(s, v[i]);
}
struct Reader {
    const char* p;
    const char* end;
    void Skip() {
        while (p < end && (*p == ' ' || *p == '\t'))
            ++p;
    }
    bool Word(const char* w) {
        Skip();
        const size_t n = std::strlen(w);
        if (size_t(end - p) < n || std::strncmp(p, w, n) != 0 || (p + n < end && p[n] != ' ' && p[n] != '\t'))
            return false;
        p += n;
        return true;
    }
    template <typename T>
    bool Num(T& v) {
        Skip();
        const auto r = std::from_chars(p, end, v);
        if (r.ec != std::errc() || (r.ptr < end && *r.ptr != ' ' && *r.ptr != '\t'))
            return false;
        p = r.ptr;
        return true;
    }
    bool Nums(float* v, int n) {
        for (int i = 0; i < n; ++i)
            if (!Num(v[i]) || !std::isfinite(v[i]))
                return false;
        return true;
    }
    bool Done() {
        Skip();
        return p >= end;
    }
};
bool Within(const float* v, int n, float most) {
    for (int i = 0; i < n; ++i)
        if (!std::isfinite(v[i]) || std::fabs(v[i]) > most)
            return false;
    return true;
}

// ---------------------------------------------------------------- the published rows
// One table, never changed once published and never freed (Find and ByBody read it with no lock; a row they give stays valid for good).
struct Table {
    std::vector<Row> rows;
};
std::atomic<const Table*> g_table{nullptr};

std::atomic<int> g_mode{0};
std::atomic<uint32_t> g_test{0};
std::atomic<bool> g_sightLearning{false};
std::atomic<MapFn> g_map{nullptr};
std::atomic<UnmapFn> g_unmap{nullptr};
std::atomic<bool> g_busy{false};  // anything to put away when it's switched off
std::atomic<uint32_t> g_readbacks{0};

// ---------------------------------------------------------------- everything else: under g_mutex
std::mutex g_mutex;

// A learned gun: its row as last published and its body's numbers (for the file).
struct Gun {
    Saved saved;
    int base = 0;
    bool verified = false;  // its mesh's key checked this launch (a gun learned now is)
    bool fromFile = false;  // learned at another launch
    bool test = false;      // the test gun ("learnguns test"): checked against its table row, never saved
    bool sightsThisLaunch = false;
    ULONGLONG since = 0;    // its row first published (its depth pass hunted since)
    bool depthSlowTold = false;
    int frameVertices = 0;  // of its vertices, how many its frame bone has (the log's)
    uint32_t used = 0;
};
std::vector<Gun> g_guns;
std::vector<Refused> g_refused;
std::vector<std::pair<uint32_t, uint32_t>> g_skip;  // draws left alone this launch (refused, or their readback given up on)
struct GiveUp {
    uint32_t count, start;
    int times;
};
std::vector<GiveUp> g_giveUps;
// Each learned body's loose::Spec, its hull and its name: never freed, never moved (a deque's elements stay where they are as it grows).
struct HullPoints {
    float p[kMostHull][3];
};
std::deque<loose::Spec> g_specs;
std::deque<HullPoints> g_hulls;
std::deque<std::string> g_names;

Watch g_watch;
Check g_check;
bool g_haveCheck = false;
uint32_t g_testWas = 0;
bool g_loaded = false, g_loadDone = false, g_saveWanted = false;  // the file read begun, and done
ULONGLONG g_savedAt = 0;
std::wstring g_store;
uint64_t g_generation = 0;  // a Forget or a switch off: work begun before it is dropped

// The readback: one draw at a time.
enum class Stage { kWatch, kCapture, kCopy, kMap, kFit, kVerifyCopy, kVerifyMap };  // kFit: on a thread of its own
struct Job {
    Stage stage = Stage::kWatch;
    uint32_t count = 0, start = 0;
    int base = 0;
    int seen = 0;  // frames of its window it was drawn in (for the log)
    ULONGLONG since = 0;
    // the buffers as bound (AddRef'd), and how
    ID3D11Buffer* ib = nullptr;
    ID3D11Buffer* vb = nullptr;
    uint32_t ibBytes = 0, ibOffset = 0, ibFormat = 0, vbBytes = 0, vbOffset = 0, stride = 0, vertices = 0;
    std::vector<float> palette;
    uint32_t paletteBones = 0;
    // their copies, and what's come back of them
    ID3D11Buffer* ibCopy = nullptr;
    ID3D11Buffer* vbCopy = nullptr;
    std::vector<uint8_t> ibData, vbData;
    bool ibBack = false;
    // a saved gun's key checked: its first indices
    uint32_t hashFrom = 0, hashBytes = 0;
};
Job g_job;

// The depth pass's hunt, for the gun in hand whose depth isn't known yet: in each view, its frame's matrix and the depth-only draws' palettes.
struct ViewDraw {
    uint32_t count = 0, start = 0, bones = 0;
    bool fits = false;  // inside its index buffer
    float palette[kMostBones][12];
};
struct ViewSlot {
    bool used = false, haveFrame = false;
    uint32_t view = 0;
    float frame[12] = {};
    int draws = 0;
    ViewDraw draw[kViewDraws];
};
struct Hunt {
    bool active = false;
    uint32_t count = 0, start = 0;
    int frameBone = -1;
    uint32_t latestView = 0;
    ViewSlot slot[kViews];
    // the views matched so far: the depth draw and every bone of it that held the frame's matrix in all of them
    int matched = 0;
    uint32_t depthCount = 0, depthStart = 0;
    uint64_t bones = 0;
};
Hunt g_hunt;

ULONGLONG Now() {
    return GetTickCount64();
}

void Publish() {
    auto* t = new Table;
    t->rows.reserve(g_guns.size());
    for (const Gun& g : g_guns)
        t->rows.push_back(g.saved.row);
    g_table.store(t, std::memory_order_release);  // the one before is never freed: a reader may still hold its rows
}

Gun* GunByBody(uint32_t count, uint32_t start) {
    for (Gun& g : g_guns)
        if (g.saved.row.bodyIndices == count && g.saved.row.bodyStart == start)
            return &g;
    return nullptr;
}
Gun* GunByCount(uint32_t count) {
    for (Gun& g : g_guns)
        if (g.saved.row.bodyIndices == count)
            return &g;
    return nullptr;
}
bool IsDepthOfAGun(uint32_t count, uint32_t start) {
    for (const Gun& g : g_guns)
        if (g.saved.row.depthIndices && g.saved.row.depthIndices == count && g.saved.row.depthStart == start)
            return true;
    return false;
}
bool Skipped(uint32_t count, uint32_t start) {
    return std::find(g_skip.begin(), g_skip.end(), std::make_pair(count, start)) != g_skip.end();
}
const Refused* RefusedBefore(uint32_t count, uint32_t start, uint32_t vertices) {
    for (const Refused& r : g_refused)
        if (r.count == count && r.start == start && r.vertices == vertices)
            return &r;
    return nullptr;
}

// The spec loose holds the gun by, made from its numbers; stays where it is for good.
const loose::Spec* MakeSpec(const Saved& s) {
    g_names.push_back("learned" + std::to_string(s.row.bodyIndices) + "gun");
    HullPoints& hull = g_hulls.emplace_back();
    const int n = std::clamp(s.hullPoints, 0, kMostHull);
    std::memcpy(hull.p, s.hull, sizeof(float) * 3 * n);
    loose::Spec& spec = g_specs.emplace_back();
    spec.name = g_names.back().c_str();
    std::memcpy(spec.boxMin, s.boxMin, sizeof(spec.boxMin));
    std::memcpy(spec.boxMax, s.boxMax, sizeof(spec.boxMax));
    spec.sweep = 0.0f, spec.sag = 0.0f;
    spec.massEmpty = s.mass, spec.massRound = kRoundMass, spec.capacity = kCapacity, spec.bounce = 1.0f;
    spec.keep = true;
    spec.hull = hull.p;
    spec.hullPoints = n;
    return &spec;
}

void ReleaseJob(Job& j) {
    for (ID3D11Buffer** b : {&j.ib, &j.vb, &j.ibCopy, &j.vbCopy})
        if (*b) {
            (*b)->Release();
            *b = nullptr;
        }
}
// Back to watching (the caller puts the buffers away: ReleaseJob, outside the lock if it can).
void EndJob() {
    g_job.stage = Stage::kWatch;
    g_job.palette.clear();
    g_job.palette.shrink_to_fit();
    g_job.ibData.clear();
    g_job.ibData.shrink_to_fit();
    g_job.vbData.clear();
    g_job.vbData.shrink_to_fit();
    g_job.ibBack = false;
    g_watch = Watch{};
}

void Refuse(uint32_t count, uint32_t start, uint32_t vertices, const char* why, bool forGood) {
    if (!Skipped(count, start))
        g_skip.emplace_back(count, start);
    const bool test = count == g_test.load(std::memory_order_relaxed);
    if (forGood && !test && !RefusedBefore(count, start, vertices)) {
        Refused r;
        r.count = count, r.start = start, r.vertices = vertices;
        std::snprintf(r.why, sizeof(r.why), "%s", why);
        g_refused.push_back(r);
        g_saveWanted = true;
    }
    if (test)
        Log("Held gun: learned the test gun %u: FAIL: its draw %u from %u was refused: %s", count, count, start, why);
    else if (forGood)
        Log("Held gun: draw %u from %u isn't a gun to learn: %s (kept in heldguns.txt: never read again)", count, start, why);
    else
        Log("Held gun: draw %u from %u not learned: %s: left alone till the next launch", count, start, why);
}

// A readback of the draw that failed (how: what the graphics card did): each counts, so a draw failing again and again is left alone this launch
// after kGiveUps, never copied over and over.
void GaveUp(uint32_t count, uint32_t start, const char* what, const char* how) {
    auto it = std::find_if(g_giveUps.begin(), g_giveUps.end(), [&](const GiveUp& g) { return g.count == count && g.start == start; });
    if (it == g_giveUps.end())
        it = g_giveUps.insert(g_giveUps.end(), GiveUp{count, start, 0});
    ++it->times;
    Log("Held gun: %s of draw %u from %u %s%s", what, count, start, how, it->times >= kGiveUps ? ": left alone till the next launch" : ": tried again later");
    if (it->times >= kGiveUps && !Skipped(count, start))
        g_skip.emplace_back(count, start);
}
constexpr const char* kNoCopy = "couldn't be copied (no room on the graphics card)";
constexpr const char* kTooLong = "didn't come back from the graphics card in 2 s";
// "couldn't be read back (<the error>)", for GaveUp.
struct MapFailed {
    char text[48];
    explicit MapFailed(HRESULT hr) { std::snprintf(text, sizeof(text), "couldn't be read back (%08lx)", static_cast<unsigned long>(hr)); }
};

// The test gun's numbers against its table row.
void TestAgainstTable(const Gun& g) {
    if (!g_haveCheck || g_check.bodyIndices != g.saved.row.bodyIndices) {
        Log("Held gun: learned the test gun %u: no table row to check it with", g.saved.row.bodyIndices);
        return;
    }
    const Row& r = g.saved.row;
    const Check& c = g_check;
    const int frameWanted = c.frameBone;
    const uint32_t depthStartWanted = c.depthStart ? c.depthStart : c.depthIndices;  // the table's: 0 is "as many in as it has"
    const int depthBoneWanted = c.depthFrame >= 0 ? c.depthFrame : c.frameBone;
    const bool frameOk = r.frameBone == frameWanted && r.bodyStart == c.bodyStart;
    const bool depthOk = r.depthIndices == c.depthIndices && r.depthStart == depthStartWanted && r.depthFrame == depthBoneWanted;
    const float dr = Distance(r.rightPalm, c.rightPalm), dl = Distance(r.leftPalm, c.leftPalm);
    const bool palmsOk = r.palms && dr <= kPalmsTest && dl <= kPalmsTest;
    char body[160];
    const gunbodies::Body* known = gunbodies::For(c.shape);
    if (!known || !known->spec) {
        std::snprintf(body, sizeof(body), "body: SKIP (no fitted body for \"%s\" in gunbodies.h)", c.shape ? c.shape : "");
    } else {
        const loose::Spec& k = *known->spec;
        float off = 0.0f;
        for (int i = 0; i < 3; ++i) {
            off = std::max({off, std::fabs(g.saved.boxMin[i] - k.boxMin[i]), std::fabs(g.saved.boxMax[i] - k.boxMax[i]),
                            std::fabs(r.hullMin[i] - known->hullMin[i]), std::fabs(r.hullMax[i] - known->hullMax[i])});
        }
        bool samePoints = g.saved.hullPoints == k.hullPoints;
        for (int i = 0; samePoints && i < k.hullPoints; ++i)
            off = std::max(off, Distance(g.saved.hull[i], k.hull[i]));
        const bool same = samePoints && off <= kBodyTest;
        std::snprintf(body, sizeof(body), "body within %.1f mm of %s%s %s%s", off * 1000.0f, k.name, samePoints ? "" : " (not as many hull points)",
                      same ? "PASS" : "FAIL", same ? "" : " (unless gunbodyfit added its parts drawn apart, which the learner doesn't see)");
    }
    Log("Held gun: learned the test gun %u: frame bone %d (the table's %d) %s; depth %u from %u bone %d (the table's %u from %u bone %d) %s; palms %.0f "
        "and %.0f mm off %s; %s",
        r.bodyIndices, r.frameBone, frameWanted, frameOk ? "PASS" : "FAIL", r.depthIndices, r.depthStart, r.depthFrame, c.depthIndices, depthStartWanted,
        depthBoneWanted, depthOk ? "PASS" : "FAIL", dr * 1000.0f, dl * 1000.0f, palmsOk ? "PASS" : "FAIL", body);
}

// Everything known: the hands hold it now.
void CheckReady(Gun& g) {
    Row& r = g.saved.row;
    if (r.ready || !g.verified || r.frameBone < 0 || !r.depthIndices || r.depthFrame < 0 || !r.palms || !r.body)
        return;
    r.ready = true;
    if (g.fromFile) {
        Log("Held gun: %u indices from %u learned before: its mesh the same, the hands hold it now", r.bodyIndices, r.bodyStart);
    } else {
        Log("Held gun: a new gun learned: %u indices from %u, frame bone %d (%d of %u vertices), its depth %u from %u bone %d, its body %.2f m long (%d "
            "hull points), its palms right (%.3f %.3f %.3f) left (%.3f %.3f %.3f): the hands hold it now",
            r.bodyIndices, r.bodyStart, r.frameBone, g.frameVertices, g.used, r.depthIndices, r.depthStart, r.depthFrame, r.hullMax[2] - r.hullMin[2],
            g.saved.hullPoints, r.rightPalm[0], r.rightPalm[1], r.rightPalm[2], r.leftPalm[0], r.leftPalm[1], r.leftPalm[2]);
    }
    if (g.test)
        TestAgainstTable(g);
    else
        g_saveWanted = true;
}

void DropSaved(uint32_t count, uint32_t start, const char* why) {
    const auto it = std::find_if(g_guns.begin(), g_guns.end(), [&](const Gun& g) { return g.saved.row.bodyIndices == count && g.saved.row.bodyStart == start; });
    if (it == g_guns.end())
        return;
    Log("Held gun: %u indices from %u learned before, but its mesh isn't the same now (%s): learned again", count, start, why);
    g_guns.erase(it);
    if (g_hunt.count == count && g_hunt.start == start)
        g_hunt.active = false;
    g_saveWanted = true;
    Publish();
}

// ---------------------------------------------------------------- the depth pass's hunt
ViewSlot& SlotFor(uint32_t view) {
    ViewSlot* oldest = &g_hunt.slot[0];
    for (ViewSlot& s : g_hunt.slot) {
        if (s.used && s.view == view)
            return s;
        if (!s.used)
            oldest = &s;
        else if (oldest->used && static_cast<int32_t>(s.view - oldest->view) < 0)
            oldest = &s;
    }
    oldest->used = true;
    oldest->haveFrame = false;
    oldest->view = view;
    oldest->draws = 0;
    return *oldest;
}

void HuntFor(const Gun& g) {
    if (g_hunt.active && g_hunt.count == g.saved.row.bodyIndices && g_hunt.start == g.saved.row.bodyStart)
        return;
    g_hunt.active = true;
    g_hunt.count = g.saved.row.bodyIndices, g_hunt.start = g.saved.row.bodyStart;
    g_hunt.frameBone = g.saved.row.frameBone;
    g_hunt.latestView = 0;
    for (ViewSlot& s : g_hunt.slot)
        s.used = false;
    g_hunt.matched = 0;
    g_hunt.bones = 0;
}

// This view's frame matrix, from the body's own draw.
void HuntBody(const float* palette, uint32_t paletteBones, uint32_t view) {
    if (!palette || g_hunt.frameBone < 0 || static_cast<uint32_t>(g_hunt.frameBone) >= std::min<uint32_t>(paletteBones, kMostBones))
        return;
    const float* frame = palette + g_hunt.frameBone * 12;
    if (!Rigid(frame))
        return;
    ViewSlot& s = SlotFor(view);
    std::memcpy(s.frame, frame, sizeof(s.frame));
    s.haveFrame = true;
    if (static_cast<int32_t>(view - g_hunt.latestView) > 0 || !g_hunt.latestView)
        g_hunt.latestView = view;
}

// A depth-only draw in this view, kept to be matched once the view's done.
void HuntCandidate(ID3D11DeviceContext* ctx, uint32_t count, uint32_t start, const float* palette, uint32_t paletteBones, uint32_t view) {
    if (!palette || !paletteBones)
        return;
    ViewSlot& s = SlotFor(view);
    if (s.draws >= kViewDraws)
        return;
    ViewDraw& d = s.draw[s.draws++];
    d.count = count, d.start = start;
    d.bones = std::min<uint32_t>(paletteBones, kMostBones);
    std::memcpy(d.palette, palette, sizeof(float) * 12 * d.bones);
    d.fits = false;
    if (ctx) {
        ID3D11Buffer* ib = nullptr;
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        UINT offset = 0;
        ctx->IAGetIndexBuffer(&ib, &format, &offset);
        if (ib) {
            D3D11_BUFFER_DESC desc{};
            ib->GetDesc(&desc);
            d.fits = DrawInside(offset, desc.ByteWidth, start, count, IndexWidth(format));
            ib->Release();
        }
    }
    if (static_cast<int32_t>(view - g_hunt.latestView) > 0 || !g_hunt.latestView)
        g_hunt.latestView = view;
}

// Each view done (one newer has come): the largest depth-only draw whose palette holds the frame's matrix, and every bone of it that does. Found
// in kDepthViews views, the same draw each time: its frame bone is the lowest that held the matrix in every one of them (the first of two equal).
void HuntMatch() {
    if (!g_hunt.active)
        return;
    for (ViewSlot& s : g_hunt.slot) {
        if (!s.used || s.view == g_hunt.latestView)
            continue;
        const ViewDraw* best = nullptr;
        uint64_t bestBones = 0;
        if (s.haveFrame)
            for (int i = 0; i < s.draws; ++i) {
                const ViewDraw& d = s.draw[i];
                if (!d.fits || d.count < kBodyLeast || (d.count == g_hunt.count && d.start == g_hunt.start))
                    continue;
                uint64_t bones = 0;
                for (uint32_t from = 0; from < d.bones;) {
                    const int b = MatchBone(s.frame, d.palette[from], d.bones - from);
                    if (b < 0)
                        break;
                    bones |= uint64_t(1) << (from + b);
                    from += b + 1;
                }
                if (bones && (!best || d.count > best->count))
                    best = &d, bestBones = bones;
            }
        s.used = false;
        if (!best)
            continue;
        if (g_hunt.matched && (best->count != g_hunt.depthCount || best->start != g_hunt.depthStart || !(g_hunt.bones & bestBones))) {
            g_hunt.matched = 0;  // another draw, or other bones: start over from this view
        }
        if (!g_hunt.matched) {
            g_hunt.depthCount = best->count, g_hunt.depthStart = best->start;
            g_hunt.bones = bestBones;
        } else {
            g_hunt.bones &= bestBones;
        }
        if (++g_hunt.matched < kDepthViews)
            continue;
        Gun* g = GunByBody(g_hunt.count, g_hunt.start);
        g_hunt.active = false;
        if (!g)
            return;
        int bone = 0;
        while (bone < kMostBones && !(g_hunt.bones & (uint64_t(1) << bone)))
            ++bone;
        Row& r = g->saved.row;
        r.depthIndices = g_hunt.depthCount, r.depthStart = g_hunt.depthStart, r.depthFrame = bone;
        Log("Held gun: %u's depth pass found: %u indices from %u, its frame on bone %d", r.bodyIndices, r.depthIndices, r.depthStart, r.depthFrame);
        CheckReady(*g);
        Publish();
        return;
    }
}

// ---------------------------------------------------------------- the store
std::wstring StorePath() {
    if (!g_store.empty())
        return g_store;
    wchar_t path[MAX_PATH] = {};
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&StorePath),
                       &self);
    GetModuleFileNameW(self, path, MAX_PATH);
    if (wchar_t* slash = wcsrchr(path, L'\\'))
        slash[1] = 0;
    return std::wstring(path) + L"heldguns.txt";
}

// The file read and written on threads of their own (a disk, or something scanning it, can take a frame or more): never in a frame's work. One
// write at a time, never an older text over a newer one; Forget waits for the one under way and drops any older still to come.
std::mutex g_fileMutex;
uint64_t g_written = 0;            // the newest text written (under g_fileMutex)
std::atomic<uint64_t> g_texts{0};  // texts made to be written
uint64_t g_forgets = 0;            // (under g_mutex) a load begun before a Forget is dropped

void LoadNow(const std::wstring& path, uint64_t forgets);
void Write(const std::wstring& path, const std::string& text, uint64_t number);

template <typename Work>
void OnItsOwn(const Work& work) {
    try {
        std::thread(work).detach();  // (a copy: if no thread can be had, the work is still whole here)
    } catch (...) {
        work();  // (no thread to be had: here, once)
    }
}

// Once, the first frame it's on: the guns learned at other launches (not held till their mesh's key is checked) and the meshes refused.
void Load() {
    std::wstring path;
    uint64_t forgets = 0;
    {
        std::lock_guard lock(g_mutex);
        if (g_loaded)
            return;
        g_loaded = true;
        path = StorePath();
        forgets = g_forgets;
    }
    OnItsOwn([path, forgets] { LoadNow(path, forgets); });
}

void LoadNow(const std::wstring& path, uint64_t forgets) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"rb") != 0 || !f) {
        std::lock_guard lock(g_mutex);
        if (forgets == g_forgets)
            g_loadDone = true;  // (none yet)
        return;
    }
    std::vector<Saved> guns;
    std::vector<Refused> refused;
    int broken = 0;
    char line[8192];
    while (std::fgets(line, sizeof(line), f)) {
        size_t n = std::strlen(line);
        while (n && (line[n - 1] == '\n' || line[n - 1] == '\r'))
            line[--n] = 0;
        if (!n || line[0] == '#')
            continue;
        Saved s;
        Refused r;
        if (ParseRow(line, s))
            guns.push_back(s);
        else if (ParseRefused(line, r))
            refused.push_back(r);
        else
            ++broken;
    }
    std::fclose(f);
    std::lock_guard lock(g_mutex);
    if (forgets != g_forgets)
        return;
    int taken = 0;
    for (Saved& s : guns) {
        if (GunByBody(s.row.bodyIndices, s.row.bodyStart))
            continue;
        s.row.body = MakeSpec(s);
        s.row.ready = false;
        Gun g;
        g.saved = s;
        g.fromFile = true;
        g_guns.push_back(g);
        ++taken;
    }
    for (const Refused& r : refused)
        if (!RefusedBefore(r.count, r.start, r.vertices))
            g_refused.push_back(r);
    g_loadDone = true;
    Publish();
    Log("Held gun: %d guns learned before and %zu meshes refused before, from heldguns.txt%s", taken, refused.size(),
        broken ? " (some lines not read: broken)" : "");
}

void Save() {
    std::string text;
    std::wstring path;
    {
        std::lock_guard lock(g_mutex);
        if (!g_saveWanted || !g_loadDone || Now() - g_savedAt < kSaveEvery)
            return;  // (never before the file's own guns are read: they'd be written over)
        g_saveWanted = false;
        g_savedAt = Now();
        path = StorePath();
        text = "# Guns learned by the mod (weapon.txt \"learnguns 1\"), one a line, and meshes it won't read again. Written by it: delete a line to learn "
               "that gun again.\n";
        for (const Gun& g : g_guns)
            if (!g.test && (g.saved.row.ready || g.fromFile))
                text += FormatRow(g.saved) + "\n";
        for (const Refused& r : g_refused)
            text += FormatRefused(r) + "\n";
    }
    const uint64_t number = ++g_texts;
    OnItsOwn([path, text = std::move(text), number] { Write(path, text, number); });
}

// Written whole beside, then put in its place: read at any moment, the file is the old one or the new one.
void Write(const std::wstring& path, const std::string& text, uint64_t number) {
    std::lock_guard file(g_fileMutex);
    if (number <= g_written)
        return;
    g_written = number;
    const std::wstring fresh = path + L".new";
    FILE* f = nullptr;
    bool ok = _wfopen_s(&f, fresh.c_str(), L"wb") == 0 && f;
    if (ok) {
        ok = std::fwrite(text.data(), 1, text.size(), f) == text.size();
        ok = std::fclose(f) == 0 && ok;
    }
    ok = ok && MoveFileExW(fresh.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING);
    if (static bool s_told = false; !ok && !s_told) {
        s_told = true;
        Log("Held gun: heldguns.txt couldn't be written");
    }
}

// ---------------------------------------------------------------- the readback (OnFrame's thread, the lock not held)
HRESULT MapRead(ID3D11DeviceContext* imm, ID3D11Buffer* buffer, D3D11_MAPPED_SUBRESOURCE* mapped) {
    if (MapFn map = g_map.load(std::memory_order_relaxed))
        return map(imm, buffer, 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, mapped);
    return imm->Map(buffer, 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, mapped);
}
void UnmapRead(ID3D11DeviceContext* imm, ID3D11Buffer* buffer) {
    if (UnmapFn unmap = g_unmap.load(std::memory_order_relaxed))
        unmap(imm, buffer, 0);
    else
        imm->Unmap(buffer, 0);
}
ID3D11Buffer* Staging(ID3D11Device* device, uint32_t bytes) {
    D3D11_BUFFER_DESC desc{bytes, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ, 0, 0};
    ID3D11Buffer* b = nullptr;
    return SUCCEEDED(device->CreateBuffer(&desc, nullptr, &b)) ? b : nullptr;
}

// A mesh read back, fitted on a thread of its own: the copies it owns, and the draw (its pointers into those copies).
struct FitWork {
    uint64_t generation = 0;
    std::vector<uint8_t> ib, vb;
    std::vector<float> palette;
    Capture capture;
    uint32_t vertices = 0;
};

// The fit, the lock not held; its result taken under it, unless it was put away meanwhile (Forget, switched off).
void FitNow(const FitWork& w) {
    Fitted f;
    const bool ok = FitCapture(w.capture, f);
    const Capture& c = w.capture;
    std::lock_guard lock(g_mutex);
    if (w.generation != g_generation || g_job.stage != Stage::kFit)
        return;
    EndJob();
    if (!ok) {
        Refuse(c.count, c.start, w.vertices, f.why, f.forGood);
        return;
    }
    Gun g;
    Row& r = g.saved.row;
    r.bodyIndices = c.count, r.bodyStart = c.start, r.frameBone = f.frameBone;
    std::memcpy(r.muzzle, f.muzzle, sizeof(r.muzzle));
    std::memcpy(r.sights, f.sights, sizeof(r.sights));
    std::memcpy(r.port, f.port, sizeof(r.port));
    std::memcpy(r.hullMin, f.hullMin, sizeof(r.hullMin));
    std::memcpy(r.hullMax, f.hullMax, sizeof(r.hullMax));
    r.vertices = f.vertices, r.indexHash = f.indexHash;
    std::memcpy(g.saved.boxMin, f.boxMin, sizeof(g.saved.boxMin));
    std::memcpy(g.saved.boxMax, f.boxMax, sizeof(g.saved.boxMax));
    std::memcpy(g.saved.centre, f.centre, sizeof(g.saved.centre));
    g.saved.keelX = f.keelX, g.saved.mass = kMass;
    g.saved.hullPoints = f.hullPoints;
    std::memcpy(g.saved.hull, f.hull, sizeof(g.saved.hull));
    r.body = MakeSpec(g.saved);
    g.base = c.base;
    g.frameVertices = f.frameVertices, g.used = f.used;
    g.verified = true;
    g.test = c.count == g_test.load(std::memory_order_relaxed);
    g.since = Now();
    if (Gun* old = GunByBody(c.count, c.start))
        *old = g;
    else
        g_guns.push_back(g);
    Publish();
    Log("Held gun: draw %u from %u's mesh read: frame bone %d (%d of %u vertices), its body %.2f m long (%d hull points)%s: its depth pass and the "
        "game's palms on it next",
        c.count, c.start, f.frameBone, f.frameVertices, f.used, f.hullMax[2] - f.hullMin[2], f.hullPoints,
        f.leftOutCount ? ", parts parked away from it left out" : "");
}
// On a thread of its own (with none to be had, in the frame): no room in memory (32 bits) for the fit's vectors or its row refuses the gun till the
// next launch. Never an exception out of here: out of a thread of its own it would end the game, in the frame it would go into the game's Present.
void Fit(const FitWork& w) {
    try {
        FitNow(w);
    } catch (const std::exception&) {
        try {
            std::lock_guard lock(g_mutex);
            if (w.generation != g_generation)
                return;
            if (g_job.stage == Stage::kFit)
                EndJob();
            Refuse(w.capture.count, w.capture.start, w.vertices, "no room in memory to fit its mesh", kThisLaunch);
        } catch (...) {
        }
    }
}

// The mapped bytes kept; false with no room for them in memory (32 bits): never an exception into the game's Present.
bool Keep(const void* mapped, uint32_t bytes, std::vector<uint8_t>& out) {
    try {
        out.assign(static_cast<const uint8_t*>(mapped), static_cast<const uint8_t*>(mapped) + bytes);
        return true;
    } catch (const std::bad_alloc&) {
        std::vector<uint8_t>().swap(out);
        return false;
    }
}
constexpr const char* kNoMemory = "no room in memory to read its mesh back";

// What the job's stage needs of the graphics card, this frame.
void Readback(ID3D11Device* device, ID3D11DeviceContext* imm) {
    Stage stage;
    uint64_t generation;
    {
        std::lock_guard lock(g_mutex);
        stage = g_job.stage;
        generation = g_generation;
        if (stage == Stage::kCapture && Now() - g_job.since > kCaptureWait) {
            EndJob();  // put away, or not drawn again: watched again
            return;
        }
    }
    if (!device || !imm || (stage != Stage::kCopy && stage != Stage::kMap && stage != Stage::kVerifyCopy && stage != Stage::kVerifyMap))
        return;
    // From here the job is this thread's: NoteDraw leaves it alone in these stages, and only this thread (Forget, a switch off) ends it.
    Job& j = g_job;
    auto finish = [&](auto&& underLock) {
        std::lock_guard lock(g_mutex);
        if (generation != g_generation)
            return;
        underLock();
        ReleaseJob(j);
        EndJob();
    };
    switch (stage) {
    case Stage::kCopy: {
        j.ibCopy = Staging(device, j.ibBytes);
        j.vbCopy = Staging(device, j.vbBytes);
        if (!j.ibCopy || !j.vbCopy) {
            finish([&] { GaveUp(j.count, j.start, "the mesh", kNoCopy); });
            return;
        }
        imm->CopyResource(j.ibCopy, j.ib);
        imm->CopyResource(j.vbCopy, j.vb);
        g_readbacks.fetch_add(1, std::memory_order_relaxed);
        j.ib->Release(), j.ib = nullptr;
        j.vb->Release(), j.vb = nullptr;
        std::lock_guard lock(g_mutex);
        if (generation == g_generation)
            j.stage = Stage::kMap, j.since = Now();
        return;
    }
    case Stage::kMap: {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (!j.ibBack) {
            const HRESULT hr = MapRead(imm, j.ibCopy, &mapped);
            if (SUCCEEDED(hr) && mapped.pData) {
                const bool kept = Keep(mapped.pData, j.ibBytes, j.ibData);
                UnmapRead(imm, j.ibCopy);
                j.ibCopy->Release(), j.ibCopy = nullptr;
                if (!kept) {
                    finish([&] { Refuse(j.count, j.start, j.vertices, kNoMemory, kThisLaunch); });
                    return;
                }
                j.ibBack = true;
            } else if (hr != DXGI_ERROR_WAS_STILL_DRAWING) {
                finish([&] { GaveUp(j.count, j.start, "the mesh", MapFailed(hr).text); });
                return;
            }
        }
        if (j.ibBack) {
            const HRESULT hr = MapRead(imm, j.vbCopy, &mapped);
            if (SUCCEEDED(hr) && mapped.pData) {
                const bool kept = Keep(mapped.pData, j.vbBytes, j.vbData);
                UnmapRead(imm, j.vbCopy);
                j.vbCopy->Release(), j.vbCopy = nullptr;
                // The fit from the copies, on a thread of its own: some 20-30 ms for a rifle, never in a frame.
                std::shared_ptr<FitWork> work;
                if (kept) {
                    try {
                        work = std::make_shared<FitWork>();
                    } catch (const std::bad_alloc&) {
                    }
                }
                if (!work) {
                    finish([&] { Refuse(j.count, j.start, j.vertices, kNoMemory, kThisLaunch); });
                    return;
                }
                work->generation = generation;
                work->ib = std::move(j.ibData), work->vb = std::move(j.vbData), work->palette = std::move(j.palette);
                Capture& c = work->capture;
                c.ib = work->ib.data(), c.ibBytes = j.ibBytes, c.ibOffset = j.ibOffset, c.ibFormat = j.ibFormat;
                c.vb = work->vb.data(), c.vbBytes = j.vbBytes, c.vbOffset = j.vbOffset, c.stride = j.stride;
                c.count = j.count, c.start = j.start, c.base = j.base;
                c.palette = work->palette.data(), c.paletteBones = j.paletteBones;
                work->vertices = j.vertices;
                {
                    std::lock_guard lock(g_mutex);
                    if (generation != g_generation)
                        return;
                    j.stage = Stage::kFit;
                }
                OnItsOwn([work] { Fit(*work); });
                return;
            }
            if (hr != DXGI_ERROR_WAS_STILL_DRAWING) {
                finish([&] { GaveUp(j.count, j.start, "the mesh", MapFailed(hr).text); });
                return;
            }
        }
        if (Now() - j.since > kMapWait)
            finish([&] { GaveUp(j.count, j.start, "the mesh", kTooLong); });
        return;
    }
    case Stage::kVerifyCopy: {
        j.ibCopy = Staging(device, j.hashBytes);
        if (!j.ibCopy) {
            finish([&] { GaveUp(j.count, j.start, "the key check", kNoCopy); });
            return;
        }
        const D3D11_BOX box{j.hashFrom, 0, 0, j.hashFrom + j.hashBytes, 1, 1};
        imm->CopySubresourceRegion(j.ibCopy, 0, 0, 0, 0, j.ib, 0, &box);
        j.ib->Release(), j.ib = nullptr;
        std::lock_guard lock(g_mutex);
        if (generation == g_generation)
            j.stage = Stage::kVerifyMap, j.since = Now();
        return;
    }
    case Stage::kVerifyMap: {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        const HRESULT hr = MapRead(imm, j.ibCopy, &mapped);
        if (SUCCEEDED(hr) && mapped.pData) {
            const uint32_t hash = IndexHash(static_cast<const uint8_t*>(mapped.pData), j.hashBytes);
            UnmapRead(imm, j.ibCopy);
            finish([&] {
                Gun* g = GunByBody(j.count, j.start);
                if (!g)
                    return;
                if (hash != g->saved.row.indexHash) {
                    DropSaved(j.count, j.start, "its indices aren't");
                    return;
                }
                g->verified = true;
                CheckReady(*g);
                Publish();
            });
            return;
        }
        if (hr != DXGI_ERROR_WAS_STILL_DRAWING)
            finish([&] { GaveUp(j.count, j.start, "the key check", MapFailed(hr).text); });
        else if (Now() - j.since > kMapWait)
            finish([&] { GaveUp(j.count, j.start, "the key check", kTooLong); });
        return;
    }
    default:
        return;
    }
}

// Switched off, or forgotten: nothing in flight kept (the lock held; the buffers put away by the caller after).
Job TakeJob() {
    Job taken;
    std::swap(taken.ib, g_job.ib);
    std::swap(taken.vb, g_job.vb);
    std::swap(taken.ibCopy, g_job.ibCopy);
    std::swap(taken.vbCopy, g_job.vbCopy);
    EndJob();
    g_hunt.active = false;
    ++g_generation;
    return taken;
}

// Another test gun, none, or switched off (the test over): the test gun learned before is forgotten (never saved), learned anew at its next test
// (the lock held).
void TestNow(uint32_t test) {
    if (test == g_testWas)
        return;
    const size_t before = g_guns.size();
    g_guns.erase(std::remove_if(g_guns.begin(), g_guns.end(), [&](const Gun& g) { return g.test; }), g_guns.end());
    if (g_guns.size() != before)
        Publish();
    g_skip.erase(std::remove_if(g_skip.begin(), g_skip.end(), [&](const auto& k) { return k.first == g_testWas || k.first == test; }), g_skip.end());
    g_testWas = test;
}

}  // namespace

// ---------------------------------------------------------------- the pure steps
uint32_t IndexHash(const uint8_t* indices, uint32_t bytes) {
    uint32_t h = 2166136261u;
    if (!indices)
        return h;
    bytes = std::min(bytes, kHashBytes);
    for (uint32_t i = 0; i < bytes; ++i) {
        h ^= indices[i];
        h *= 16777619u;
    }
    return h;
}

int MatchBone(const float* frame, const float* palette, uint32_t bones, float tolerance) {
    if (!frame || !palette)
        return -1;
    bones = std::min<uint32_t>(bones, kMostBones);
    for (uint32_t b = 0; b < bones; ++b) {
        const float* m = palette + b * 12;
        bool same = true;
        for (int k = 0; k < 12 && same; ++k)
            same = std::fabs(m[k] - frame[k]) <= tolerance;  // (never with a number that isn't)
        if (same)
            return static_cast<int>(b);
    }
    return -1;
}

bool Watch::Closes(uint32_t frame) const {
    return open && static_cast<int32_t>(frame - windowStart) >= kWindow;
}
void Watch::Note(uint32_t count, uint32_t start, int base, uint32_t frame, bool colour) {
    if (!open)
        Next(frame);
    for (int i = 0; i < seenCount; ++i) {
        Seen& s = seen[i];
        if (s.count == count && s.start == start && s.base == base) {
            if (static_cast<int32_t>(frame - s.lastFrame) > 0)
                ++s.frames, s.lastFrame = frame;
            s.colour = s.colour || colour;
            return;
        }
    }
    if (seenCount < kMost)
        seen[seenCount++] = Seen{count, start, base, 1, frame, colour};
}
int Watch::Pick(const uint32_t (*skip)[2], int skips) const {
    int best = -1;
    for (int i = 0; i < seenCount; ++i) {
        const Seen& s = seen[i];
        if (!s.colour || s.count < kBodyLeast || s.frames < static_cast<uint32_t>(kWindowSeen))
            continue;
        bool skipped = false;
        for (int k = 0; k < skips && !skipped; ++k)
            skipped = skip[k][0] == s.count && skip[k][1] == s.start;
        if (!skipped && (best < 0 || s.count > seen[best].count))
            best = i;
    }
    return best;
}
void Watch::Next(uint32_t frame) {
    open = true;
    windowStart = frame;
    seenCount = 0;
}

bool GunSized(const float* keptMin, const float* keptMax) {
    if (!keptMin || !keptMax || !Finite(keptMin, 3) || !Finite(keptMax, 3))
        return false;
    const float x = keptMax[0] - keptMin[0], y = keptMax[1] - keptMin[1], z = keptMax[2] - keptMin[2];
    return z >= kGunLeast && z >= x && z >= y;
}

bool FitCapture(const Capture& c, Fitted& out) {
    out = Fitted{};
    // forGood: refused for what its mesh is (its layout, its buffers, its size); else for what the palette drawn with it that once did to it.
    auto refuse = [&](const char* why, bool forGood) {
        out.ok = false;
        out.why = why;
        out.forGood = forGood;
        return false;
    };
    if (!c.ib || !c.vb || !c.palette || !c.paletteBones)
        return refuse("nothing read back", kThisLaunch);
    const uint32_t width = IndexWidth(c.ibFormat);
    if (!width)
        return refuse("its index format isn't 16 or 32 bits", kForGood);
    if (!StrideKnown(c.stride))
        return refuse("a vertex layout it doesn't know (its stride)", kForGood);
    if (c.count < 3 || c.count % 3)
        return refuse("not whole triangles", kForGood);
    if (!DrawInside(c.ibOffset, c.ibBytes, c.start, c.count, width))
        return refuse("the draw isn't inside its index buffer", kForGood);
    if (c.vbOffset >= c.vbBytes || c.vbBytes - c.vbOffset < c.stride)
        return refuse("its vertices aren't inside their buffer", kForGood);
    const uint32_t room = (c.vbBytes - c.vbOffset) / c.stride;  // vertices from the offset on
    out.vertices = c.vbBytes / c.stride;
    const uint8_t* first = c.ib + c.ibOffset + size_t(c.start) * width;
    out.indexHash = IndexHash(first, static_cast<uint32_t>(std::min<uint64_t>(uint64_t(c.count) * width, kHashBytes)));
    const uint32_t bones = std::min<uint32_t>(c.paletteBones, kMostBones);
    // Every index with the base inside the vertex buffer, before any vertex is read.
    int64_t lo = INT64_MAX, hi = -1;
    for (uint32_t i = 0; i < c.count; ++i) {
        const int64_t v = int64_t(ReadIndex(first, i, width)) + c.base;
        if (v < 0 || v >= room)
            return refuse("an index past its vertices", kForGood);
        lo = std::min(lo, v), hi = std::max(hi, v);
    }
    // The vertices as the triangle list first names them, as tools/gunbodyfit.exe numbers them: the positions are half floats, so two vertices often
    // tie for the furthest out, and the first in this order is the one both fits take. The layout: half positions at 0, four bones at 8, four weights
    // at 12 (meshdump's). One with no weight, a place that isn't a number or a bone past the palette is left out with its triangles.
    struct Raw {
        float p[3];
        uint8_t bone[4], weight[4];
        int weights;
    };
    std::vector<int> number(size_t(hi - lo + 1), -1);  // -1 not seen yet, -2 left out
    std::vector<Raw> raw;
    std::vector<int> mainBone;
    int seen = 0, offWeights = 0, farPlaces = 0, broken = 0;
    for (uint32_t i = 0; i < c.count; ++i) {
        const int64_t v = int64_t(ReadIndex(first, i, width)) + c.base;
        int& num = number[size_t(v - lo)];
        if (num != -1)
            continue;
        ++seen;
        const uint8_t* o = c.vb + c.vbOffset + size_t(v) * c.stride;  // inside: v < room
        Raw r;
        uint16_t h[3];
        std::memcpy(h, o, 6);
        r.p[0] = Half(h[0]), r.p[1] = Half(h[1]), r.p[2] = Half(h[2]);
        std::memcpy(r.bone, o + 8, 4);
        std::memcpy(r.weight, o + 12, 4);
        bool ok = Finite(r.p, 3);
        int weights = 0, heaviestWeight = -1, heaviest = -1;
        for (int s = 0; s < 4 && ok; ++s) {
            const int b = r.bone[s], w = r.weight[s];
            weights += w;
            if (!w)
                continue;
            if (b >= static_cast<int>(bones)) {
                ok = false;
                break;
            }
            if (w > heaviestWeight)
                heaviestWeight = w, heaviest = b;
        }
        offWeights += weights < kWeightLeast || weights > kWeightMost ? 1 : 0;
        farPlaces += Within(r.p, 3, kPositionMost) ? 0 : 1;
        if (!ok || weights <= 0) {
            num = -2, ++broken;
            continue;
        }
        r.weights = weights;
        num = static_cast<int>(raw.size());
        raw.push_back(r);
        mainBone.push_back(heaviest);
    }
    if (!seen || offWeights * 100 > seen)
        return refuse("a vertex layout it doesn't know (its weights)", kForGood);
    if (broken * 100 > seen)
        return refuse("its vertices name bones past its palette, or places that aren't numbers", kThisLaunch);  // (the palette's bones as uploaded)
    if (farPlaces * 100 > seen)
        return refuse("a vertex layout it doesn't know (its places)", kForGood);
    const int n = static_cast<int>(raw.size());
    out.used = static_cast<uint32_t>(n);
    std::vector<int> tris;
    tris.reserve(c.count);
    for (uint32_t i = 0; i + 2 < c.count; i += 3) {
        int corner[3];
        bool keep = true;
        for (int k = 0; k < 3; ++k)
            corner[k] = number[size_t(int64_t(ReadIndex(first, i + k, width)) + c.base - lo)], keep = keep && corner[k] >= 0;
        if (keep)
            tris.insert(tris.end(), corner, corner + 3);
    }
    // Its frame bone: the bone most vertices go with; rigid.
    const int frame = bodyfit::MostWeighted(mainBone.data(), n);
    if (frame < 0 || frame >= static_cast<int>(bones))
        return refuse("no frame bone", kThisLaunch);
    const float* frameMatrix = c.palette + frame * 12;
    if (!Rigid(frameMatrix))
        return refuse("its frame bone isn't rigid", kThisLaunch);
    out.frameBone = frame;
    out.frameVertices = static_cast<int>(std::count(mainBone.begin(), mainBone.end(), frame));
    // Posed into the frame's space by the palette carried onto the frame (gunbodyfit's FramePalette and AddSkinned, step for step).
    float toFrame[12];
    InvertRigid(frameMatrix, toFrame);
    std::vector<float> onFrame(size_t(bones) * 12);
    for (uint32_t b = 0; b < bones; ++b)
        Compose(toFrame, c.palette + b * 12, &onFrame[size_t(b) * 12]);
    std::vector<float> points(size_t(n) * 3, std::nanf(""));
    for (int v = 0; v < n; ++v) {
        const Raw& r = raw[size_t(v)];
        float sum[3] = {};
        for (int s = 0; s < 4; ++s) {
            const int w = r.weight[s];
            if (!w)
                continue;
            float q[3];
            Apply(&onFrame[size_t(r.bone[s]) * 12], r.p, q);
            for (int k = 0; k < 3; ++k)
                sum[k] += q[k] * static_cast<float>(w);
        }
        if (!Finite(sum, 3)) {
            mainBone[size_t(v)] = -1;  // (its triangles aren't read by the fit)
            continue;
        }
        for (int k = 0; k < 3; ++k)
            points[size_t(v) * 3 + k] = sum[k] / static_cast<float>(r.weights);
    }
    const bodyfit::Result body = bodyfit::Fit(points.data(), mainBone.data(), n, tris.data(), static_cast<int>(tris.size() / 3));
    if (!body.ok)
        return refuse(body.why, kThisLaunch);  // (its points as the palette posed them: out of reach, or not numbers)
    std::memcpy(out.keptMin, body.hullMin, sizeof(out.keptMin));
    std::memcpy(out.keptMax, body.hullMax, sizeof(out.keptMax));
    if (body.hullMax[2] - body.hullMin[2] < kGunLeast)
        return refuse("too short for a gun", kForGood);
    if (!GunSized(body.hullMin, body.hullMax))
        return refuse("not shaped like a gun (not longest along its z)", kForGood);
    // Under a narrow bottom bodyfit puts on the keel only what's no wider than the core there: a bottom wider than a slim core (a magazine wider
    // than its receiver) keeps its own corners, and the gun could stand on them. Every known gun's body touches upright on its keel alone (bodytest
    // b6); a learned one that wouldn't is refused (as its magazine was posed that once), never left standing on the ground.
    if (body.footprint < bodyfit::Options{}.standWidth && !TipsOver(body))
        return refuse("its body would stand on its narrow bottom (upright it touches off its keel)", kThisLaunch);
    std::memcpy(out.boxMin, body.boxMin, sizeof(out.boxMin));
    std::memcpy(out.boxMax, body.boxMax, sizeof(out.boxMax));
    std::memcpy(out.hullMin, body.hullMin, sizeof(out.hullMin));
    std::memcpy(out.hullMax, body.hullMax, sizeof(out.hullMax));
    std::memcpy(out.centre, body.centre, sizeof(out.centre));
    out.keelX = body.keelX;
    out.hullPoints = std::min(body.hullPoints, kMostHull);
    std::memcpy(out.hull, body.hull, sizeof(float) * 3 * out.hullPoints);
    out.leftOutCount = std::min(body.leftOutCount, kMostBones);
    std::memcpy(out.leftOut, body.leftOut, sizeof(int) * out.leftOutCount);
    // Its muzzle: at its front, as high as the points near its front are on average. Its sights' line until it's learned: the top of its receiver
    // (its frame bone's highest), at the muzzle.
    const float front = body.hullMax[2];
    double ySum = 0.0;
    int yCount = 0;
    for (int v = 0; v < n; ++v) {
        const int b = mainBone[size_t(v)];
        if (b < 0 || std::find(body.leftOut, body.leftOut + body.leftOutCount, b) != body.leftOut + body.leftOutCount)
            continue;
        const float* p = &points[size_t(v) * 3];
        if (std::isfinite(p[1]) && p[2] >= front - kMuzzleBand && p[2] <= front)
            ySum += p[1], ++yCount;
    }
    out.muzzle[0] = 0.0f, out.muzzle[1] = yCount ? static_cast<float>(ySum / yCount) : 0.5f * (body.hullMin[1] + body.hullMax[1]), out.muzzle[2] = front;
    out.sights[0] = 0.0f, out.sights[1] = std::clamp(body.coreMax[1], kSightsLow, kSightsHigh), out.sights[2] = front;
    std::memcpy(out.port, kPort, sizeof(out.port));
    if (!Finite(out.muzzle, 3) || !Finite(out.sights, 3) || !Finite(&out.hull[0][0], out.hullPoints * 3))
        return refuse("numbers not finite", kThisLaunch);
    out.ok = true;
    out.why = "";
    return true;
}

std::string FormatRow(const Saved& s) {
    const Row& r = s.row;
    std::string t = "gun";
    Put(t, r.bodyIndices), Put(t, r.bodyStart);
    Put(t, "key"), Put(t, r.vertices), Put(t, r.indexHash);
    Put(t, "frame"), Put(t, r.frameBone);
    Put(t, "depth"), Put(t, r.depthIndices), Put(t, r.depthStart), Put(t, r.depthFrame);
    Put(t, "palms"), Put(t, r.rightPalm, 3), Put(t, r.leftPalm, 3);
    Put(t, "sights"), Put(t, r.sightsLearned ? 1 : 0), Put(t, "up"), Put(t, r.up), Put(t, "at"), Put(t, r.sights, 3);
    Put(t, "muzzle"), Put(t, r.muzzle, 3), Put(t, "port"), Put(t, r.port, 3);
    Put(t, "box"), Put(t, s.boxMin, 3), Put(t, s.boxMax, 3);
    Put(t, "hullbox"), Put(t, r.hullMin, 3), Put(t, r.hullMax, 3);
    Put(t, "middle"), Put(t, s.centre, 3), Put(t, "keel"), Put(t, s.keelX), Put(t, "mass"), Put(t, s.mass);
    Put(t, "hull"), Put(t, s.hullPoints);
    for (int i = 0; i < std::clamp(s.hullPoints, 0, kMostHull); ++i)
        Put(t, s.hull[i], 3);
    return t;
}

bool ParseRow(const char* line, Saved& out) {
    if (!line)
        return false;
    Saved s;
    Row& r = s.row;
    Reader in{line, line + std::strlen(line)};
    int sights = 0;
    const bool read = in.Word("gun") && in.Num(r.bodyIndices) && in.Num(r.bodyStart) && in.Word("key") && in.Num(r.vertices) && in.Num(r.indexHash) &&
                      in.Word("frame") && in.Num(r.frameBone) && in.Word("depth") && in.Num(r.depthIndices) && in.Num(r.depthStart) &&
                      in.Num(r.depthFrame) && in.Word("palms") && in.Nums(r.rightPalm, 3) && in.Nums(r.leftPalm, 3) && in.Word("sights") &&
                      in.Num(sights) && in.Word("up") && in.Num(r.up) && in.Word("at") && in.Nums(r.sights, 3) && in.Word("muzzle") &&
                      in.Nums(r.muzzle, 3) && in.Word("port") && in.Nums(r.port, 3) && in.Word("box") && in.Nums(s.boxMin, 3) &&
                      in.Nums(s.boxMax, 3) && in.Word("hullbox") && in.Nums(r.hullMin, 3) && in.Nums(r.hullMax, 3) && in.Word("middle") &&
                      in.Nums(s.centre, 3) && in.Word("keel") && in.Num(s.keelX) && in.Word("mass") && in.Num(s.mass) && in.Word("hull") &&
                      in.Num(s.hullPoints) && s.hullPoints >= 4 && s.hullPoints <= kMostHull;
    if (!read)
        return false;
    for (int i = 0; i < s.hullPoints; ++i)
        if (!in.Nums(s.hull[i], 3))
            return false;
    if (!in.Done())
        return false;
    // Every number in its range: a line written by hand, or broken, is refused whole.
    const bool sane = r.bodyIndices >= kBodyLeast && r.bodyIndices % 3 == 0 && r.vertices > 0 && r.frameBone >= 0 && r.frameBone < kMostBones &&
                      r.depthIndices >= kBodyLeast && r.depthFrame >= 0 && r.depthFrame < kMostBones && (sights == 0 || sights == 1) &&
                      std::isfinite(r.up) && std::fabs(r.up) <= 10.0f && Within(r.rightPalm, 3, 3.0f) && Within(r.leftPalm, 3, 3.0f) &&
                      Within(r.sights, 3, 3.0f) && Within(r.muzzle, 3, 3.0f) && Within(r.port, 3, 3.0f) && Within(s.boxMin, 3, 3.0f) &&
                      Within(s.boxMax, 3, 3.0f) && Within(r.hullMin, 3, 3.0f) && Within(r.hullMax, 3, 3.0f) && Within(s.centre, 3, 3.0f) &&
                      std::isfinite(s.keelX) && std::fabs(s.keelX) <= 1.0f && std::isfinite(s.mass) && s.mass > 0.0f && s.mass <= 100.0f &&
                      Within(&s.hull[0][0], s.hullPoints * 3, 3.0f);
    if (!sane)
        return false;
    for (int k = 0; k < 3; ++k)
        if (s.boxMin[k] >= s.boxMax[k] || r.hullMin[k] > r.hullMax[k])
            return false;
    r.sightsLearned = sights == 1;
    r.palms = true;
    r.body = nullptr;
    r.ready = false;
    out = s;
    return true;
}

std::string FormatRefused(const Refused& r) {
    std::string t = "refused";
    Put(t, r.count), Put(t, r.start), Put(t, r.vertices);
    std::string why(r.why, strnlen(r.why, sizeof(r.why)));
    for (char& ch : why)
        if (ch == '\n' || ch == '\r' || ch == '\t')
            ch = ' ';
    if (!why.empty())
        Put(t, why.c_str());
    return t;
}

bool ParseRefused(const char* line, Refused& out) {
    if (!line)
        return false;
    Refused r;
    Reader in{line, line + std::strlen(line)};
    if (!in.Word("refused") || !in.Num(r.count) || !in.Num(r.start) || !in.Num(r.vertices) || !r.count || !r.vertices)
        return false;
    in.Skip();
    std::snprintf(r.why, sizeof(r.why), "%.*s", static_cast<int>(in.end - in.p), in.p);
    out = r;
    return true;
}

// ---------------------------------------------------------------- the runtime
bool On() {
    return g_mode.load(std::memory_order_relaxed) != 0;
}

void Set(int mode, uint32_t testBody) {
    testBody = mode ? testBody : 0;
    const int was = g_mode.exchange(mode, std::memory_order_relaxed);
    const uint32_t testWas = g_test.exchange(testBody, std::memory_order_relaxed);
    if (was == mode && testWas == testBody)
        return;
    if (!mode)
        Log("Held gun: guns never seen: not learned (learnguns 0)");
    else if (testBody)
        Log("Held gun: guns never seen: learned, and the known gun %u learned as new to check it (learnguns test %u)", testBody, testBody);
    else
        Log("Held gun: guns never seen: learned (learnguns 1)");
}

uint32_t TestBody() {
    return g_test.load(std::memory_order_relaxed);
}

void SetCheck(const Check& c) {
    std::lock_guard lock(g_mutex);
    g_check = c;
    g_haveCheck = true;
}

void NoteDraw(ID3D11DeviceContext* ctx, uint32_t count, uint32_t start, int base, const float* palette, uint32_t paletteBones, uint32_t frame, uint32_t view) {
    if (!g_mode.load(std::memory_order_relaxed) || count < kBodyLeast)
        return;
    g_busy.store(true, std::memory_order_relaxed);
    // A colour draw has render targets; the depth pass, which has more indices than the body, has none.
    bool colour = false;
    if (ctx) {
        ID3D11RenderTargetView* targets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
        ctx->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, targets, nullptr);
        for (ID3D11RenderTargetView* t : targets)
            if (t) {
                colour = true;
                t->Release();
            }
    }
    std::lock_guard lock(g_mutex);
    if (!g_mode.load(std::memory_order_relaxed))
        return;  // switched off meanwhile: nothing taken (OnFrame has put away what was)
    // A gun learned: its key checked if it's from another launch, its depth pass hunted if it isn't known yet.
    if (Gun* g = GunByBody(count, start)) {
        if (!g->verified) {
            // Its key is its colour draw's (the capture's): another pass of the same indices may draw them from another vertex buffer. A key check
            // given up on (GaveUp) leaves it alone till the next launch.
            if (g_job.stage != Stage::kWatch || !colour || Skipped(count, start))
                return;
            ID3D11Buffer* ib = nullptr;
            DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
            UINT ibOffset = 0;
            ctx->IAGetIndexBuffer(&ib, &format, &ibOffset);
            ID3D11Buffer* vb = nullptr;
            UINT stride = 0, vbOffset = 0;
            ctx->IAGetVertexBuffers(0, 1, &vb, &stride, &vbOffset);
            D3D11_BUFFER_DESC id{}, vd{};
            if (ib)
                ib->GetDesc(&id);
            if (vb)
                vb->GetDesc(&vd);
            const uint32_t width = IndexWidth(format);
            const uint32_t vertices = vb && stride ? vd.ByteWidth / stride : 0;
            const bool bound = ib && vb && width;
            if (vb)
                vb->Release();
            // Its vertex count at once; its first indices from the frame's end on.
            const char* differs = !bound ? nullptr : vertices != g->saved.row.vertices ? "its vertex count isn't"
                                                     : !DrawInside(ibOffset, id.ByteWidth, start, count, width) ? "its indices aren't where they were"
                                                                                                                : nullptr;
            if (!bound || differs) {
                if (ib)
                    ib->Release();
                if (differs)
                    DropSaved(count, start, differs);
                return;
            }
            g_job.stage = Stage::kVerifyCopy;
            g_job.count = count, g_job.start = start, g_job.base = base;
            g_job.ib = ib;
            g_job.hashFrom = ibOffset + start * width;
            g_job.hashBytes = std::min(count * width, kHashBytes);
            g_job.since = Now();
            return;
        }
        if (!g->saved.row.depthIndices) {
            HuntFor(*g);
            HuntBody(palette, paletteBones, view);
            if (!g->depthSlowTold && Now() - g->since > kDepthSlow) {
                g->depthSlowTold = true;
                Log("Held gun: %u's depth pass isn't found yet", count);
            }
        }
        return;
    }
    if (IsDepthOfAGun(count, start))
        return;
    if (g_hunt.active && !colour)
        HuntCandidate(ctx, count, start, palette, paletteBones, view);
    if (g_job.stage == Stage::kWatch) {
        if (g_watch.Closes(frame)) {
            uint32_t skip[64][2];
            int skips = 0;
            for (const Gun& g : g_guns)
                if (skips < 64)
                    skip[skips][0] = g.saved.row.bodyIndices, skip[skips][1] = g.saved.row.bodyStart, ++skips;
            for (const auto& [c, s] : g_skip)
                if (skips < 64)
                    skip[skips][0] = c, skip[skips][1] = s, ++skips;
            if (const int i = g_watch.Pick(skip, skips); i >= 0) {
                const Watch::Seen& s = g_watch.seen[i];
                g_job.stage = Stage::kCapture;
                g_job.count = s.count, g_job.start = s.start, g_job.base = s.base;
                g_job.seen = static_cast<int>(s.frames);
                g_job.since = Now();
                Log("Held gun: a gun the mod doesn't know in hand: draw %u from %u, drawn in %d of %d frames: its mesh read next", s.count, s.start, g_job.seen,
                    kWindow);
            }
            g_watch.Next(frame);
        }
        if (g_job.stage == Stage::kWatch)
            g_watch.Note(count, start, base, frame, colour);
    }
    // Its next draw: its buffers kept as bound, to be copied at the frame's end.
    if (g_job.stage == Stage::kCapture && count == g_job.count && start == g_job.start && base == g_job.base && colour && ctx) {
        ID3D11Buffer* ib = nullptr;
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        UINT ibOffset = 0;
        ctx->IAGetIndexBuffer(&ib, &format, &ibOffset);
        ID3D11Buffer* vb = nullptr;
        UINT stride = 0, vbOffset = 0;
        ctx->IAGetVertexBuffers(0, 1, &vb, &stride, &vbOffset);
        D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
        ctx->IAGetPrimitiveTopology(&topology);
        D3D11_BUFFER_DESC id{}, vd{};
        if (ib)
            ib->GetDesc(&id);
        if (vb)
            vb->GetDesc(&vd);
        const uint32_t width = IndexWidth(format);
        const uint32_t vertices = vb && stride ? vd.ByteWidth / stride : 0;
        const char* why = nullptr;
        if (!ib || !vb)
            why = "no buffers bound";
        else if (topology != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
            why = "not a list of triangles";
        else if (!width)
            why = "its index format isn't 16 or 32 bits";
        else if (!StrideKnown(stride))
            why = "a vertex layout it doesn't know (its stride)";
        else if (id.ByteWidth > kBufferMost || vd.ByteWidth > kBufferMost)
            why = "its buffers are too big to read back";
        else if (!DrawInside(ibOffset, id.ByteWidth, start, count, width))
            why = "the draw isn't inside its index buffer";
        else if (vbOffset >= vd.ByteWidth)
            why = "its vertices aren't inside their buffer";
        else if (!palette || !paletteBones)
            why = "no palette";
        const Refused* before = why ? nullptr : RefusedBefore(count, start, vertices);
        if (why || before) {
            // (Kept for good only what its mesh is, never a moment's state: no buffers or no palette bound.)
            const bool forGood = ib && vb && palette && paletteBones;
            if (ib)
                ib->Release();
            if (vb)
                vb->Release();
            if (before) {
                if (!Skipped(count, start))
                    g_skip.emplace_back(count, start);
                Log("Held gun: draw %u from %u was refused before (%s): not read again", count, start, before->why);
            } else {
                Refuse(count, start, vertices, why, forGood);
            }
            EndJob();
            return;
        }
        g_job.ib = ib, g_job.vb = vb;
        g_job.ibBytes = id.ByteWidth, g_job.ibOffset = ibOffset, g_job.ibFormat = static_cast<uint32_t>(format);
        g_job.vbBytes = vd.ByteWidth, g_job.vbOffset = vbOffset, g_job.stride = stride, g_job.vertices = vertices;
        g_job.paletteBones = std::min<uint32_t>(paletteBones, kMostBones);
        g_job.palette.assign(palette, palette + size_t(g_job.paletteBones) * 12);
        g_job.stage = Stage::kCopy;
        g_job.since = Now();
    }
}

void OnFrame(ID3D11Device* device, ID3D11DeviceContext* immediate) {
    if (!g_mode.load(std::memory_order_relaxed)) {
        // Switched off: nothing kept in flight, the test over (Find and ByBody give nothing meanwhile).
        if (g_busy.exchange(false, std::memory_order_relaxed)) {
            Job taken;
            {
                std::lock_guard lock(g_mutex);
                taken = TakeJob();
                TestNow(0);
            }
            ReleaseJob(taken);
        }
        return;
    }
    g_busy.store(true, std::memory_order_relaxed);
    Load();
    {
        std::lock_guard lock(g_mutex);
        TestNow(g_test.load(std::memory_order_relaxed));
        HuntMatch();
    }
    Readback(device, immediate);
    Save();
}

// Switched off (learnguns 0), none: every gun learned is the game's own again, its rows kept for when it's on again.
const Row* Find(uint32_t count, uint32_t start) {
    if (!g_mode.load(std::memory_order_relaxed))
        return nullptr;
    const Table* t = g_table.load(std::memory_order_acquire);
    if (!t)
        return nullptr;
    for (const Row& r : t->rows)
        if ((r.bodyIndices == count && r.bodyStart == start) || (r.depthIndices && r.depthIndices == count && r.depthStart == start))
            return &r;
    return nullptr;
}

const Row* ByBody(uint32_t bodyIndices) {
    if (!g_mode.load(std::memory_order_relaxed))
        return nullptr;
    const Table* t = g_table.load(std::memory_order_acquire);
    if (!t)
        return nullptr;
    for (const Row& r : t->rows)
        if (r.bodyIndices == bodyIndices)
            return &r;
    return nullptr;
}

void NotePalms(uint32_t bodyIndices, const float* right, const float* left) {
    if (!right || !left || !Within(right, 3, 3.0f) || !Within(left, 3, 3.0f))
        return;
    std::lock_guard lock(g_mutex);
    Gun* g = GunByCount(bodyIndices);
    // Once the hands hold it its palms stay as they are: the gun never jumps in the hands.
    if (!g || g->saved.row.ready)
        return;
    Row& r = g->saved.row;
    if (r.palms && Distance(r.rightPalm, right) < kPalmSame && Distance(r.leftPalm, left) < kPalmSame)
        return;
    std::memcpy(r.rightPalm, right, sizeof(r.rightPalm));
    std::memcpy(r.leftPalm, left, sizeof(r.leftPalm));
    r.palms = true;
    CheckReady(*g);
    Publish();
}

void SetSightLearning(bool on) {
    g_sightLearning.store(on, std::memory_order_relaxed);
}

void NoteSights(uint32_t bodyIndices, const float* eyeOnFrame, const float* forwardOnFrame) {
    if (!g_sightLearning.load(std::memory_order_relaxed) || !eyeOnFrame || !forwardOnFrame || !Within(eyeOnFrame, 3, 3.0f) || !Within(forwardOnFrame, 3, 2.0f))
        return;
    const float length = std::sqrt(forwardOnFrame[0] * forwardOnFrame[0] + forwardOnFrame[1] * forwardOnFrame[1] + forwardOnFrame[2] * forwardOnFrame[2]);
    if (!(length > 1e-6f))
        return;
    const float forward[3] = {forwardOnFrame[0] / length, forwardOnFrame[1] / length, forwardOnFrame[2] / length};
    if (forward[2] < 0.9f)
        return;  // not looking along it
    static float s_eye[3], s_forward[3];
    static ULONGLONG s_since = 0;
    static uint32_t s_gun = 0;
    std::lock_guard lock(g_mutex);
    Gun* g = GunByCount(bodyIndices);
    if (!g || g->saved.row.frameBone < 0 || g->sightsThisLaunch)
        return;
    const ULONGLONG now = Now();
    const float turn = s_forward[0] * forward[0] + s_forward[1] * forward[1] + s_forward[2] * forward[2];
    if (s_gun != bodyIndices || !s_since || Distance(s_eye, eyeOnFrame) > kSightEye || turn < kSightTurnCos) {
        s_gun = bodyIndices;
        std::memcpy(s_eye, eyeOnFrame, sizeof(s_eye));
        std::memcpy(s_forward, forward, sizeof(s_forward));
        s_since = now;
        return;
    }
    if (now - s_since < kSightSteady)
        return;
    Row& r = g->saved.row;
    r.up = std::atan2(-forward[1], forward[2]) * 180.0f / kPi;
    const float along = (r.muzzle[2] - eyeOnFrame[2]) / forward[2];
    for (int k = 0; k < 3; ++k)
        r.sights[k] = eyeOnFrame[k] + forward[k] * along;
    r.sightsLearned = true;
    g->sightsThisLaunch = true;
    s_since = 0;
    Log("Held gun: %u's sights learned: %.2f degrees under its z, the line at its muzzle (%.3f %.3f %.3f)", bodyIndices, r.up, r.sights[0], r.sights[1],
        r.sights[2]);
    if (g->test && g_haveCheck && g_check.bodyIndices == bodyIndices) {
        const bool upOk = std::fabs(r.up - g_check.up) <= kUpTest, lineOk = Distance(r.sights, g_check.sights) <= kSightsTest;
        Log("Held gun: the test gun %u's sights: up %.2f (the table's %.2f) %s; the line (%.3f %.3f %.3f) (the table's (%.3f %.3f %.3f)) %s", bodyIndices, r.up,
            g_check.up, upOk ? "PASS" : "FAIL", r.sights[0], r.sights[1], r.sights[2], g_check.sights[0], g_check.sights[1], g_check.sights[2],
            lineOk ? "PASS" : "FAIL");
    }
    if (r.ready && !g->test)
        g_saveWanted = true;
    Publish();
}

void SetMap(void* map, void* unmap) {
    g_map.store(reinterpret_cast<MapFn>(map), std::memory_order_relaxed);
    g_unmap.store(reinterpret_cast<UnmapFn>(unmap), std::memory_order_relaxed);
}

void SetStore(const wchar_t* path) {
    std::lock_guard lock(g_mutex);
    g_store = path ? path : L"";
}

void Forget() {
    Job taken;
    {
        std::lock_guard lock(g_mutex);
        taken = TakeJob();
        g_guns.clear();
        g_refused.clear();
        g_skip.clear();
        g_giveUps.clear();
        g_haveCheck = false;
        g_testWas = 0;
        g_loaded = false;
        g_loadDone = false;
        g_saveWanted = false;
        g_savedAt = 0;
        ++g_forgets;  // a load under way is dropped
        Publish();
    }
    ReleaseJob(taken);
    // The write under way finished, and none made before this one written after it.
    std::lock_guard file(g_fileMutex);
    g_written = std::max(g_written, g_texts.load());
}

uint32_t Readbacks() {
    return g_readbacks.load(std::memory_order_relaxed);
}

bool Loaded() {
    std::lock_guard lock(g_mutex);
    return g_loadDone;
}

}  // namespace gunlearn

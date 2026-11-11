#pragma once
// A gun's body for loose.h (free hands: a gun let go of lies as a rigid body), fitted from its own first-person mesh. tools/gunbodyfit.cpp fits
// every known gun's offline into src/gunbodies.h; a gun never seen is fitted the same way when it's learned. Pure: no game calls and no logs, so
// the tools and the tests run it without the game.
// Every vertex comes posed into the gun's frame bone's space (x to its left, y up, z to the muzzle; metres), with its bone of most weight:
//   1. the core bone: the bone most vertices go with (every held gun's frame bone; the SMAW's tube);
//   2. parked parts out: a bone whose vertices' middle lies more than parkedBeyond outside the core's box is left out whole (the AN-94's spare
//      magazine, parked 1.09 m behind the gun), never a list per gun;
//   3. every other vertex kept: the seated magazine, the lever, the cover, the lid, the box, the aerial, where the mesh has them at rest;
//   4. its middle (where loose puts its weight): the kept triangles' area-weighted middle;
//   5. its box for loose (its weight's spread and its turn): the core's size about that middle, so loose's middle is the middle;
//   6. where it touches the ground, at most 22 of its mesh's own vertices (loose's most), each within merge of one kept before it merged into
//      it, in the order of how much each matters: under a narrow bottom the keel's back, middle and front and the two ends (the furthest along
//      the 26 ways out of the kept box, in its own proportions); under a wide one the faces it may stand and lie on; then the gap points: while
//      its mesh reaches more than gapStop past the hull any way, the vertex furthest the worst way, the ways it comes to rest on counting more.
//      (The 26 ways alone left it 1.5-3 cm into the ground on its sights or its levers, and the M240's stock 8 cm; with a side band and the top's
//      points first, Options::band, 6-14 mm on five guns.) Those that only made up less than restGap are "the rest";
//   7. the keel: under a narrow bottom, the points along it no wider than the core is there go onto one line along z (x = keelX), the middle
//      at least keelNear beside it, so stood upright it always tips over, as gunbody.h's M416 does; each such vertex is also held
//      keelRaise above its own place (a guard), so rolled onto its side the gun stops where that vertex stops it. A bottom 4 cm wide or more
//      (the M240's box, the Stinger's gripstock) has none: the gun really stands on it.
// Not allocation-free (marks per vertex), so it's called once per gun, never per frame.
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace bodyfit {

constexpr int kMostHull = 22, kMostBones = 64;
constexpr int kWays = 1024;  // the ways over the sphere it's held against its mesh
constexpr int kMostRests = 24;
// gunLength > 0: refuse a gun whose kept mesh is shorter along z (a knife, a grenade); keelWide: how much wider than the core's own width a
// bottom point may be and still go onto the keel; band: the side band and the top's points before the gap points (off: the gap points
// find what they would, in fewer points); gapStop: the gap points stop once the mesh reaches no more than this past them any way; restGap: a gap
// point that made up less is "the rest"; keelRaise: how far above its own place a bottom vertex the keel narrows is held as a guard;
// standWidth: a bottom (footprint) at least this wide has no keel: it may stand on it; restWeight: how much more a gap counts within
// restCone degrees of a way it comes to rest on; restFace: how near its lowest a vertex is of a face it rests on; restFacePoints: how many
// points those faces get at most (a wide bottom's).
struct Options { float parkedBeyond = 0.30f, keelBand = 0.25f, keelOffset = 0.006f, keelNear = 0.004f, merge = 0.015f, gunLength = 0.0f, keelWide = 0.005f;
                 bool band = false; float gapStop = 0.002f, restGap = 0.01f, keelRaise = 0.005f, standWidth = 0.04f, restWeight = 2.0f,
                 restCone = 25.0f, restFace = 0.003f; int restFacePoints = 6; };
// What each hull point is, for the reports and the pictures (Result::what).
enum What : int { kKeelBack, kKeelMiddle, kKeelFront, kEndBack, kEndFront, kBottomWide, kSideBand, kTop, kGap, kGuard, kExtra, kFace };
inline const char* WhatName(int what) {
    static constexpr const char* kNames[] = {"keel back", "keel middle", "keel front", "back end", "front end", "bottom, wide", "side band", "top", "gap",
                                             "guard", "extra", "rest face"};
    return what >= 0 && what < static_cast<int>(sizeof(kNames) / sizeof(kNames[0])) ? kNames[what] : "?";
}
struct Result {
    bool ok = false;
    const char* why = "";
    int coreBone = -1, coreVertices = 0, keptVertices = 0;
    float coreMin[3] = {}, coreMax[3] = {}, hullMin[3] = {}, hullMax[3] = {};
    float centre[3] = {};                 // the kept triangles' area-weighted middle
    float boxMin[3] = {}, boxMax[3] = {}; // the core's size about the middle (loose::Spec's box: its com is the middle)
    float keelX = 0.0f;
    int hullPoints = 0;
    float hull[kMostHull][3] = {};
    int leftOut[kMostBones] = {}, leftOutCount = 0;
    int restFrom = 0;                     // hull[restFrom..] is "the rest": the points an attachment's go before (WithExtra)
    int what[kMostHull] = {};             // each hull point's What
    float footprint = 0.0f;               // the kept vertices within 1 cm of the lowest: how wide they are in x (under standWidth: it never stands)
    float gap = 0.0f;                     // how far its mesh still reaches past its hull, the worst way (as the keel and its guards have it; m)
    int rests = 0;                        // the ways it comes to rest on (down, in its own axes)
    float rest[kMostRests][3] = {};
};

// The bone most vertices go with (-1 with none); a tie goes to the lower bone.
inline int MostWeighted(const int* bone, int vertices) {
    int count[kMostBones] = {};
    for (int i = 0; i < vertices; ++i)
        if (bone[i] >= 0 && bone[i] < kMostBones)
            ++count[bone[i]];
    int best = -1;
    for (int b = 0; b < kMostBones; ++b)
        if (count[b] > 0 && (best < 0 || count[b] > count[best]))
            best = b;
    return best;
}

namespace detail {
inline bool Usable(const float* p) {
    for (int k = 0; k < 3; ++k)
        if (!std::isfinite(p[k]) || std::fabs(p[k]) > 10.0f)
            return false;
    return true;
}
inline float Distance2(const float* a, const float* b) {
    const float d[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    return d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
}
// The distance from p to a box (0 inside).
inline float OutsideBox(const float* lo, const float* hi, const float* p) {
    float s = 0.0f;
    for (int k = 0; k < 3; ++k) {
        const float e = std::max(std::max(lo[k] - p[k], 0.0f), p[k] - hi[k]);
        s += e * e;
    }
    return std::sqrt(s);
}
// Adds p unless one already in within merge (that one stays: the earlier kept), while there's room. Whether it was added.
inline bool Add(float (*out)[3], int* what, int& n, const float* p, int kind, float merge) {
    if (n >= kMostHull)
        return false;
    for (int i = 0; i < n; ++i)
        if (Distance2(out[i], p) <= merge * merge)
            return false;
    std::memcpy(out[n], p, sizeof(out[n]));
    if (what)
        what[n] = kind;
    ++n;
    return true;
}
}  // namespace detail

inline Result Fit(const float* points, const int* bone, int vertices, const int* tris, int triangles, const Options& o = {}) {
    Result r;
    if (!points || !bone || !tris || vertices <= 0 || triangles <= 0) {
        r.why = "no mesh";
        return r;
    }
    // The triangles that can be read: every corner a vertex there, with a place that's a number.
    std::vector<unsigned char> used(static_cast<size_t>(vertices), 0), kept(static_cast<size_t>(vertices), 0);
    auto good = [&](int t) {
        for (int c = 0; c < 3; ++c) {
            const int v = tris[t * 3 + c];
            if (v < 0 || v >= vertices || !detail::Usable(points + v * 3))
                return false;
        }
        return true;
    };
    int readable = 0;
    for (int t = 0; t < triangles; ++t)
        if (good(t)) {
            ++readable;
            for (int c = 0; c < 3; ++c)
                used[tris[t * 3 + c]] = 1;
        }
    if (!readable) {
        r.why = "no triangle can be read";
        return r;
    }
    // 1. The core bone, and its box.
    int count[kMostBones] = {};
    for (int v = 0; v < vertices; ++v)
        if (used[v] && bone[v] >= 0 && bone[v] < kMostBones)
            ++count[bone[v]];
    for (int b = 0; b < kMostBones; ++b)
        if (count[b] > 0 && (r.coreBone < 0 || count[b] > count[r.coreBone]))
            r.coreBone = b;
    if (r.coreBone < 0) {
        r.why = "no vertex has a bone";
        return r;
    }
    r.coreVertices = count[r.coreBone];
    for (int k = 0; k < 3; ++k)
        r.coreMin[k] = 1e9f, r.coreMax[k] = -1e9f;
    for (int v = 0; v < vertices; ++v)
        if (used[v] && bone[v] == r.coreBone)
            for (int k = 0; k < 3; ++k)
                r.coreMin[k] = std::min(r.coreMin[k], points[v * 3 + k]), r.coreMax[k] = std::max(r.coreMax[k], points[v * 3 + k]);
    // 2. Parked parts out: by each bone's vertices' middle.
    bool out[kMostBones] = {};
    {
        double sum[kMostBones][3] = {};
        for (int v = 0; v < vertices; ++v)
            if (used[v] && bone[v] >= 0 && bone[v] < kMostBones)
                for (int k = 0; k < 3; ++k)
                    sum[bone[v]][k] += points[v * 3 + k];
        for (int b = 0; b < kMostBones; ++b) {
            if (b == r.coreBone || !count[b])
                continue;
            const float middle[3] = {static_cast<float>(sum[b][0] / count[b]), static_cast<float>(sum[b][1] / count[b]),
                                     static_cast<float>(sum[b][2] / count[b])};
            if (detail::OutsideBox(r.coreMin, r.coreMax, middle) > o.parkedBeyond) {
                out[b] = true;
                r.leftOut[r.leftOutCount++] = b;
            }
        }
    }
    // 3. Kept: every readable triangle with no corner on a bone left out; its vertices.
    auto keptTri = [&](int t) {
        if (!good(t))
            return false;
        for (int c = 0; c < 3; ++c) {
            const int b = bone[tris[t * 3 + c]];
            if (b >= 0 && b < kMostBones && out[b])
                return false;
        }
        return true;
    };
    // 4. The middle: the kept triangles' area-weighted middle.
    double area = 0.0, middle[3] = {};
    for (int t = 0; t < triangles; ++t) {
        if (!keptTri(t))
            continue;
        const float* a = points + tris[t * 3] * 3;
        const float* b = points + tris[t * 3 + 1] * 3;
        const float* c = points + tris[t * 3 + 2] * 3;
        const double e1[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]}, e2[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
        const double n[3] = {e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]};
        const double s = 0.5 * std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        for (int k = 0; k < 3; ++k)
            middle[k] += s * (a[k] + b[k] + c[k]) / 3.0;
        area += s;
        for (int k = 0; k < 3; ++k)
            kept[tris[t * 3 + k]] = 1;
    }
    for (int k = 0; k < 3; ++k)
        r.hullMin[k] = 1e9f, r.hullMax[k] = -1e9f;
    double mean[3] = {};
    for (int v = 0; v < vertices; ++v) {
        if (!kept[v])
            continue;
        ++r.keptVertices;
        for (int k = 0; k < 3; ++k) {
            r.hullMin[k] = std::min(r.hullMin[k], points[v * 3 + k]), r.hullMax[k] = std::max(r.hullMax[k], points[v * 3 + k]);
            mean[k] += points[v * 3 + k];
        }
    }
    if (r.keptVertices < 4) {
        r.why = "too little of it kept";
        return r;
    }
    if (o.gunLength > 0.0f && r.hullMax[2] - r.hullMin[2] < o.gunLength) {
        r.why = "shorter than a gun";
        return r;
    }
    for (int k = 0; k < 3; ++k)
        r.centre[k] = area > 1e-12 ? static_cast<float>(middle[k] / area) : static_cast<float>(mean[k] / r.keptVertices);
    // 5. Loose's box: the core's size about the middle (at least 2 mm each way, for a sane turn).
    for (int k = 0; k < 3; ++k) {
        const float half = std::max(0.5f * (r.coreMax[k] - r.coreMin[k]), 0.001f);
        r.boxMin[k] = r.centre[k] - half, r.boxMax[k] = r.centre[k] + half;
    }
    // The footprint: how wide the kept vertices within 1 cm of the lowest are.
    {
        float lo = 1e9f, hi = -1e9f;
        for (int v = 0; v < vertices; ++v)
            if (kept[v] && points[v * 3 + 1] <= r.hullMin[1] + 0.01f)
                lo = std::min(lo, points[v * 3]), hi = std::max(hi, points[v * 3]);
        r.footprint = hi >= lo ? hi - lo : 0.0f;
    }
    // 6. Where it can touch: the furthest along each of the 26 ways out of the kept box (in its own proportions); along each side a band at the
    //    middle's height, the furthest out in each third of its length; and the furthest along each of many ways spread over the sphere (kWays),
    //    those within 45 degrees of straight down being its bottom.
    float mid[3], half[3];
    for (int k = 0; k < 3; ++k)
        mid[k] = 0.5f * (r.hullMin[k] + r.hullMax[k]), half[k] = std::max(0.5f * (r.hullMax[k] - r.hullMin[k]), 1e-4f);
    int best[27];
    float bestScore[27];
    for (int d = 0; d < 27; ++d)
        best[d] = -1, bestScore[d] = -1e30f;
    for (int v = 0; v < vertices; ++v) {
        if (!kept[v])
            continue;
        const float u[3] = {(points[v * 3] - mid[0]) / half[0], (points[v * 3 + 1] - mid[1]) / half[1], (points[v * 3 + 2] - mid[2]) / half[2]};
        for (int d = 0; d < 27; ++d) {
            if (d == 13)
                continue;  // (0 0 0)
            const float score = u[0] * (d / 9 - 1) + u[1] * (d / 3 % 3 - 1) + u[2] * (d % 3 - 1);
            if (score > bestScore[d])
                bestScore[d] = score, best[d] = v;
        }
    }
    auto way = [](int dx, int dy, int dz) { return (dx + 1) * 9 + (dy + 1) * 3 + (dz + 1); };
    const float height = r.hullMax[1] - r.hullMin[1], length = r.hullMax[2] - r.hullMin[2];
    int band[2][3];
    for (auto& s : band)
        for (int& b : s)
            b = -1;
    for (int v = 0; v < vertices; ++v) {
        if (!kept[v] || std::fabs(points[v * 3 + 1] - r.centre[1]) > 0.25f * height)
            continue;
        const int third = std::clamp(static_cast<int>((points[v * 3 + 2] - r.hullMin[2]) / std::max(length, 1e-4f) * 3.0f), 0, 2);
        for (int s = 0; s < 2; ++s) {
            const float sign = s == 0 ? 1.0f : -1.0f;
            int& b = band[s][third];
            if (b < 0 || sign * points[v * 3] > sign * points[b * 3])
                b = v;
        }
    }
    std::vector<float> ways(static_cast<size_t>(kWays) * 3), reach(kWays, -1e30f);
    std::vector<int> furthest(kWays, -1);
    std::vector<unsigned char> bottom(static_cast<size_t>(vertices), 0);
    for (int i = 0; i < kWays; ++i) {
        float* w = &ways[static_cast<size_t>(i) * 3];
        const float y = 1.0f - 2.0f * (static_cast<float>(i) + 0.5f) / kWays, s = std::sqrt(std::max(0.0f, 1.0f - y * y));
        const float a = 2.39996323f * static_cast<float>(i);  // the golden angle
        w[0] = s * std::cos(a), w[1] = y, w[2] = s * std::sin(a);
        for (int v = 0; v < vertices; ++v)
            if (kept[v]) {
                const float along = points[v * 3] * w[0] + points[v * 3 + 1] * w[1] + points[v * 3 + 2] * w[2];
                if (along > reach[i])
                    reach[i] = along, furthest[i] = v;
            }
        if (w[1] < -0.7071f && furthest[i] >= 0)
            bottom[furthest[i]] = 1;
    }
    // 7. The keel: its x, and the core's own width at a height (its vertices within half the keel band about it, else its lowest that far up: a
    //    magazine or a grip below the core is no wider than the core's bottom).
    r.keelX = std::fabs(r.centre[0]) >= o.keelNear ? 0.0f : r.centre[0] + o.keelOffset;
    const float keelTop = r.hullMin[1] + o.keelBand * height, reachY = std::max(0.5f * o.keelBand * height, 0.01f);
    float coreBottom = 1e9f;
    for (int v = 0; v < vertices; ++v)
        if (kept[v] && bone[v] == r.coreBone)
            coreBottom = std::min(coreBottom, points[v * 3 + 1]);
    auto coreWidth = [&](float y, float& lo, float& hi) {
        const float at = std::max(y, coreBottom);
        lo = 1e9f, hi = -1e9f;
        for (int v = 0; v < vertices; ++v)
            if (kept[v] && bone[v] == r.coreBone && std::fabs(points[v * 3 + 1] - at) <= reachY)
                lo = std::min(lo, points[v * 3]), hi = std::max(hi, points[v * 3]);
        return hi >= lo;
    };
    // A vertex's point on the hull: on its bottom (or low, within the keel band) and no wider than the core there, it goes onto the keel;
    // only under a narrow bottom (a footprint under standWidth): a wide one really stands on it, and keeps its corners (the Stinger's gripstock).
    const bool narrow = r.footprint < o.standWidth;
    auto place = [&](int v, bool fromBelow, float* q) {
        std::memcpy(q, points + v * 3, sizeof(float) * 3);
        if (!narrow || (!fromBelow && !bottom[v] && q[1] > keelTop))
            return false;
        float lo, hi;
        if (!coreWidth(q[1], lo, hi) || q[0] < lo - o.keelWide || q[0] > hi + o.keelWide)
            return false;
        q[0] = r.keelX;
        return true;
    };
    // 8. In the order of how much each matters, a point within merge of one before it merged into it. Under a narrow bottom: the keel's back,
    //    middle and front and the two ends, its first points. (A wide one needs no keel, and the 26 ways' points spent 9 of the Stinger's 22 on
    //    its box's corners: the gap points choose them all.) With band, the side band and the top's back, middle and front. Then the gap points:
    //    while its mesh reaches more than gapStop past the hull any way, the vertex furthest the worst way (on the keel, or its guard with its
    //    point on the keel under it). Those that only made up less than restGap are "the rest": an attachment's points go before them (WithExtra).
    float hullOut[kMostHull][3];
    int whatOut[kMostHull];
    int n = 0;
    auto put = [&](int v, int kind, bool fromBelow) {
        if (v < 0)
            return;
        float q[3];
        place(v, fromBelow, q);
        detail::Add(hullOut, whatOut, n, q, kind, o.merge);
    };
    if (narrow) {
        put(best[way(0, -1, -1)], kKeelBack, true);
        put(best[way(0, -1, 0)], kKeelMiddle, true);
        put(best[way(0, -1, 1)], kKeelFront, true);
        put(best[way(0, 0, -1)], kEndBack, false);
        put(best[way(0, 0, 1)], kEndFront, false);
        for (int dx : {-1, 1})
            for (int dz : {0, -1, 1}) {
                const int v = best[way(dx, -1, dz)];
                float q[3];
                if (v >= 0 && !place(v, true, q))
                    detail::Add(hullOut, whatOut, n, q, kBottomWide, o.merge);
            }
    }
    if (o.band) {
        for (int t = 0; t < 3; ++t)
            for (int s = 0; s < 2; ++s)
                put(band[s][t], kSideBand, false);
        put(best[way(0, 1, -1)], kTop, false);
        put(best[way(0, 1, 0)], kTop, false);
        put(best[way(0, 1, 1)], kTop, false);
    }
    // What each way's held to: the furthest vertex; one that goes onto the keel, either that point on the keel or the vertex itself raised by
    // keelRaise, whichever reaches further that way. The raised one is a guard: stood upright the keel touches first and the gun tips, but
    // lying on that side it holds the gun where the vertex would (with the keel alone a gun rolled onto its side went on rolling toward its
    // bottom, its magazine's corners 1.5-2 cm into the ground). A guard goes in with its own point on the keel: one without it could sit
    // below the keel's line (the G3's grip under the line from its heel to its magazine) and stand the gun on it.
    std::vector<float> target(kWays, -1e30f), make(static_cast<size_t>(kWays) * 3, 0.0f), under(static_cast<size_t>(kWays) * 3, 0.0f);
    std::vector<int> makeKind(kWays, kGap);
    for (int i = 0; i < kWays; ++i) {
        if (furthest[i] < 0)
            continue;
        const float* w = &ways[static_cast<size_t>(i) * 3];
        float* m = &make[static_cast<size_t>(i) * 3];
        float q[3];
        if (place(furthest[i], false, q)) {
            const float guard[3] = {points[furthest[i] * 3], points[furthest[i] * 3 + 1] + o.keelRaise, points[furthest[i] * 3 + 2]};
            const float onKeel = q[0] * w[0] + q[1] * w[1] + q[2] * w[2], raised = guard[0] * w[0] + guard[1] * w[1] + guard[2] * w[2];
            std::memcpy(m, onKeel >= raised ? q : guard, sizeof(float) * 3);
            std::memcpy(&under[static_cast<size_t>(i) * 3], q, sizeof(q));
            target[i] = std::max(onKeel, raised);
            makeKind[i] = onKeel >= raised ? kGap : kGuard;
        } else {
            std::memcpy(m, points + furthest[i] * 3, sizeof(float) * 3);
            target[i] = reach[i];
        }
    }
    std::vector<float> hullReach(kWays, -1e30f);
    std::vector<unsigned char> done(kWays, 0);
    auto reachOf = [&](const float* p) {
        for (int i = 0; i < kWays; ++i) {
            const float* w = &ways[static_cast<size_t>(i) * 3];
            hullReach[i] = std::max(hullReach[i], p[0] * w[0] + p[1] * w[1] + p[2] * w[2]);
        }
    };
    auto merged = [&](const float* q) {  // (never "near": a Windows header makes that word nothing)
        for (int j = 0; j < n; ++j)
            if (detail::Distance2(hullOut[j], q) <= o.merge * o.merge)
                return true;
        return false;
    };
    for (int i = 0; i < n; ++i)
        reachOf(hullOut[i]);
    // The ways it comes to rest on: its middle lower than any way within 12 degrees round them. A gap within restCone of one counts restWeight
    // times: a gun lying sinks where its hull's short there, and elsewhere only for a moment as it rolls (the Stinger, short of points, came to
    // rest on its folded aerial with a corner of it 1.3 cm in).
    std::vector<float> weight(kWays, 1.0f);
    {
        std::vector<float> lying(kWays, 1e30f);  // its middle's height lying that way
        for (int i = 0; i < kWays; ++i)
            if (furthest[i] >= 0) {
                const float* w = &ways[static_cast<size_t>(i) * 3];
                lying[i] = reach[i] - (r.centre[0] * w[0] + r.centre[1] * w[1] + r.centre[2] * w[2]);
            }
        auto cosine = [&](int a, int b) {
            const float* u = &ways[static_cast<size_t>(a) * 3];
            const float* v = &ways[static_cast<size_t>(b) * 3];
            return u[0] * v[0] + u[1] * v[1] + u[2] * v[2];
        };
        const float around = std::cos(12.0f * 0.0174533f), cone = std::cos(o.restCone * 0.0174533f);
        for (int i = 0; i < kWays; ++i) {
            if (furthest[i] < 0)
                continue;
            bool lowest = true;
            for (int j = 0; j < kWays && lowest; ++j)
                lowest = j == i || cosine(i, j) < around || lying[j] >= lying[i];
            if (!lowest)
                continue;
            if (r.rests < kMostRests)
                std::memcpy(r.rest[r.rests++], &ways[static_cast<size_t>(i) * 3], sizeof(r.rest[0]));
            for (int j = 0; j < kWays; ++j)
                if (cosine(i, j) >= cone)
                    weight[j] = o.restWeight;
        }
    }
    // A wide bottom (no keel, no guards: points to spare): its bottom, which it may stand on (a footprint 4 cm wide or more; the Stinger's
    // balances within 10 degrees, too few to be found among the ways round it), then each way it comes to rest on, the lowest-lying first: it
    // lies on a face of its mesh there, its vertices within restFace of its lowest that way. Where the hull has fewer than three points on that
    // face it gets the face's outermost along its length and across it, at most restFacePoints in all: on two points it rocks on an edge
    // (loose wants a second still for that, three times a flat one's), the M240 dropped on its box's side rolled over it, and the Stinger
    // stood on one point of its gripstock with the rest of it 2.6 cm in.
    if (!narrow) {
        constexpr float kDown[3] = {0.0f, -1.0f, 0.0f};
        int order[kMostRests + 1], rests = 0;
        float lyingOf[kMostRests + 1];
        for (int k = 0; k <= r.rests; ++k) {
            const float* d = k < r.rests ? r.rest[k] : kDown;
            if (k < r.rests && d[1] < -0.978f)
                continue;  // (its bottom: below)
            float lowest = -1e30f;
            for (int p = 0; p < vertices; ++p)
                if (kept[p])
                    lowest = std::max(lowest, points[p * 3] * d[0] + points[p * 3 + 1] * d[1] + points[p * 3 + 2] * d[2]);
            lyingOf[k] = k < r.rests ? lowest - (r.centre[0] * d[0] + r.centre[1] * d[1] + r.centre[2] * d[2]) : -1e30f;  // (its bottom first)
            order[rests++] = k;
        }
        std::sort(order, order + rests, [&](int a, int b) { return lyingOf[a] < lyingOf[b]; });
        int added = 0;
        for (int step = 0; step < rests && n < kMostHull && added < o.restFacePoints; ++step) {
            const float* d = order[step] < r.rests ? r.rest[order[step]] : kDown;
            float lowest = -1e30f;
            for (int p = 0; p < vertices; ++p)
                if (kept[p])
                    lowest = std::max(lowest, points[p * 3] * d[0] + points[p * 3 + 1] * d[1] + points[p * 3 + 2] * d[2]);
            auto onFace = [&](const float* q) { return q[0] * d[0] + q[1] * d[1] + q[2] * d[2] >= lowest - o.restFace; };
            int have = 0;
            for (int j = 0; j < n; ++j)
                have += onFace(hullOut[j]);
            if (have >= 3)
                continue;
            float u[3] = {-d[2] * d[0], -d[2] * d[1], 1.0f - d[2] * d[2]};  // its length flattened onto the face
            float size = std::sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
            if (size < 0.1f)
                u[0] = 1.0f - d[0] * d[0], u[1] = -d[0] * d[1], u[2] = -d[0] * d[2], size = std::sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
            for (float& c : u)
                c /= size;
            const float v[3] = {d[1] * u[2] - d[2] * u[1], d[2] * u[0] - d[0] * u[2], d[0] * u[1] - d[1] * u[0]};
            int most[4] = {-1, -1, -1, -1};
            float mostAlong[4] = {-1e30f, -1e30f, -1e30f, -1e30f};
            for (int p = 0; p < vertices; ++p) {
                if (!kept[p] || !onFace(points + p * 3))
                    continue;
                float onKeel[3];
                if (place(p, false, onKeel))
                    continue;
                const float* q = points + p * 3;
                const float along[4] = {q[0] * u[0] + q[1] * u[1] + q[2] * u[2], -(q[0] * u[0] + q[1] * u[1] + q[2] * u[2]),
                                        q[0] * v[0] + q[1] * v[1] + q[2] * v[2], -(q[0] * v[0] + q[1] * v[1] + q[2] * v[2])};
                for (int e = 0; e < 4; ++e)
                    if (along[e] > mostAlong[e])
                        mostAlong[e] = along[e], most[e] = p;
            }
            for (int e = 0; e < 4 && have < 3 && n < kMostHull && added < o.restFacePoints; ++e)
                if (most[e] >= 0 && detail::Add(hullOut, whatOut, n, points + most[e] * 3, kFace, o.merge)) {
                    reachOf(points + most[e] * 3);
                    ++have, ++added;
                }
        }
    }
    r.restFrom = -1;
    while (n < kMostHull) {
        int worst = -1;
        float gap = o.gapStop, worstWeighed = 0.0f;
        for (int i = 0; i < kWays; ++i)
            if (!done[i] && furthest[i] >= 0 && target[i] - hullReach[i] > o.gapStop && (target[i] - hullReach[i]) * weight[i] > worstWeighed)
                gap = target[i] - hullReach[i], worstWeighed = gap * weight[i], worst = i;
        if (worst < 0)
            break;
        const float* q = &make[static_cast<size_t>(worst) * 3];
        const float* keel = &under[static_cast<size_t>(worst) * 3];
        const bool withKeel = makeKind[worst] == kGuard && !merged(keel);
        if (merged(q) || (withKeel && n + 2 > kMostHull)) {
            done[worst] = 1;  // (one kept within merge of it: as near as it gets; or no room for a guard and its keel)
            continue;
        }
        if (gap < o.restGap && r.restFrom < 0)
            r.restFrom = n;
        if (withKeel) {
            detail::Add(hullOut, whatOut, n, keel, kGap, o.merge);
            reachOf(keel);
        }
        // A guard within merge of its own keel point, just put in, isn't: that point is as near as it gets that way, and the reach stays its own.
        if (detail::Add(hullOut, whatOut, n, q, makeKind[worst], o.merge))
            reachOf(q);
        else
            done[worst] = 1;
    }
    if (r.restFrom < 0)
        r.restFrom = n;
    // How far its mesh still reaches past its hull, the worst way (as the keel and its guards have it).
    for (int i = 0; i < kWays; ++i)
        if (furthest[i] >= 0)
            r.gap = std::max(r.gap, target[i] - hullReach[i]);
    r.hullPoints = n;
    for (int i = 0; i < n; ++i) {
        std::memcpy(r.hull[i], hullOut[i], sizeof(r.hull[i]));
        r.what[i] = whatOut[i];
    }
    // Numbers that aren't refuse it whole.
    for (int k = 0; k < 3; ++k)
        if (!std::isfinite(r.centre[k]) || !std::isfinite(r.boxMin[k]) || !std::isfinite(r.boxMax[k]) || !std::isfinite(r.hullMin[k]) ||
            !std::isfinite(r.hullMax[k])) {
            r.why = "numbers not finite";
            return r;
        }
    if (r.hullPoints < 4) {
        r.why = "fewer than 4 hull points";
        return r;
    }
    r.ok = true;
    return r;
}

// The hull's points before restFrom, then each extra point not within o.merge of one kept, then the hull's rest, at most kMostHull: how many.
inline int WithExtra(const float (*hull)[3], int hullPoints, int restFrom, const float (*extra)[3], int extraPoints, float (*out)[3], const Options& o = {}) {
    if (!out)
        return 0;
    int n = 0;
    hullPoints = hull ? std::clamp(hullPoints, 0, kMostHull) : 0;
    restFrom = std::clamp(restFrom, 0, hullPoints);
    auto finite = [](const float* p) { return std::isfinite(p[0]) && std::isfinite(p[1]) && std::isfinite(p[2]); };
    for (int i = 0; i < restFrom && n < kMostHull; ++i)
        if (finite(hull[i]))
            std::memcpy(out[n++], hull[i], sizeof(out[0]));
    for (int i = 0; extra && i < extraPoints; ++i)
        if (finite(extra[i]))
            detail::Add(out, nullptr, n, extra[i], kExtra, o.merge);
    for (int i = restFrom; i < hullPoints; ++i)
        if (finite(hull[i]))
            detail::Add(out, nullptr, n, hull[i], kGap, o.merge);
    return n;
}

}  // namespace bodyfit

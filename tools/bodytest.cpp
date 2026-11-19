// Every gun's body when it's let go of (src/gunbodies.h, fitted by tools/gunbodyfit.cpp from the game's own meshes), moved by src/loose.cpp on
// scripted frames at 72 Hz, without the game; gunbody.h's M416 (and its bare twin, without its M320) as the reference rows. For each:
//   b1 dropped from 1 m near upright (rolled 2 degrees either way): at rest within 3 s, not left standing (a narrow bottom: its footprint under
//      4 cm wide; a wide one, the M240's box, may stand on it: at rest within 4 s), never a hull point 2 mm into the ground;
//   b2 dropped on either side: at rest within 3 s on that side, never a hull point 2 mm in;
//   b3 near upright every way (5 cm, 30 cm and 1 m up; pitched -20 to 20 degrees; rolled -6 to 6): none left standing, all at rest within 4 s;
//      (the M1014 came to rest propped on its grip's bottom and its magazine tube's end, its muzzle down 9 to 15 degrees, in 8 of 195: loose.cpp
//      tips a gun stopped top up on a base under 4 cm across, so none stays so: no gun has an exemption here);
//   b4 thrown 200 ways (loosetest's g2 throws): none left standing, all at rest within 4 s (a wide bottom within 5: the M240, 12.5 kg,
//      see-saws on its box's edge for 4.3 s after one throw in 200; a tube within 5: the AT4 rolls on its venturi's ring for 4.3 s after one),
//      never a hull point 2 mm in, under 40 us a frame;
//   b5 lying 120 s looked away: still there, never told gone;
//   b6 the fit's own rules: its hull points in its hull box; a narrow bottom stood upright (rolled up to 10 degrees, pitched up to 15) touches
//      on its keel alone, its middle at least 4 mm beside it (a tube, the AT4, has no keel: its back is its venturi's ring); at most 22 points;
//      its box and mass sane; its name a gun's;
//   b7 the SMAW with its case and without, dropped nose up (80 degrees) from 0.5 m: it tips over, at rest within 4 s, not on its end;
//   b8 the AN-94 with a PSO-1 on its left (its points merged in, bodyfit::WithExtra): dropped on either side, none of the sight's points 1 cm in.
// At rest in b1, b2, b3 and b7 every one of its mesh's outermost vertices (build\bodytest_points.bin, written by gunbodyfit) must be less than
// 1 cm into the ground: a body's hull points only touch it by construction. A fitted body without them in the file (no file, or its points from
// another mesh than the header's: another hull box or middle) has those skipped, never passed: one check fails, naming it, and the last line
// counts the mesh checks skipped.
// Usage: bodytest [points file] (run from the project's folder: build\bodytest_points.bin). Prints PASS or FAIL per check; the exit code is the
// failures.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../src/bodyfit.h"
#include "../src/gunbodies.h"
#include "../src/gunbody.h"
#include "../src/loose.h"

namespace {

constexpr float kDt = 1.0f / 72.0f;
constexpr float kPi = 3.14159265f;
constexpr float kDegree = kPi / 180.0f;
constexpr int kGunKind = 5;  // hands.cpp's kLooseGun

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

// The ground: level at 0, the game's own (firm), dirt.
int GroundFn(void*, const float*, loose::Plane* planes, int most) {
    if (most < 1)
        return 0;
    planes[0] = loose::Plane{};
    planes[0].normal[1] = 1.0f;
    planes[0].surface = loose::kDirt;
    planes[0].firm = true;
    return 1;
}

void Identity(float* m) {
    const float id[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    std::memcpy(m, id, sizeof(id));
}
// A turn about the unit axis (row-major 3x4, no move).
void Turn(float* m, const float* a, float angle) {
    const float c = std::cos(angle), s = std::sin(angle), u = 1.0f - c;
    const float r[12] = {u * a[0] * a[0] + c,        u * a[0] * a[1] - s * a[2], u * a[0] * a[2] + s * a[1], 0,
                         u * a[0] * a[1] + s * a[2], u * a[1] * a[1] + c,        u * a[1] * a[2] - s * a[0], 0,
                         u * a[0] * a[2] - s * a[1], u * a[1] * a[2] + s * a[0], u * a[2] * a[2] + c,        0};
    std::memcpy(m, r, sizeof(r));
}
void Compose(const float* a, const float* b, float* out) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j)
            out[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        out[i * 4 + 3] += a[i * 4 + 3];
    }
}
void Apply(const float* m, const float* p, float* out) {
    for (int i = 0; i < 3; ++i)
        out[i] = m[i * 4] * p[0] + m[i * 4 + 1] * p[1] + m[i * 4 + 2] * p[2] + m[i * 4 + 3];
}
float Length(const float* v) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }

loose::Frame Base() {
    loose::Frame f;
    f.dt = kDt;
    Identity(f.roomToArms);
    f.floor = GroundFn;
    f.eye[1] = 1.6f;
    f.look[2] = 1.0f;  // looking ahead along +z
    return f;
}

// How far its lowest point is above the ground (negative: into it): of a set of points in its own space, posed by bone.
float Lowest(const float (*points)[3], int count, const float* bone, int* which = nullptr) {
    float low = 1e9f;
    for (int i = 0; i < count; ++i) {
        float p[3];
        Apply(bone, points[i], p);
        if (p[1] < low && which)
            *which = i;
        low = std::fmin(low, p[1]);
    }
    return low;
}
float LowestHull(const loose::Spec& s, const float* bone) { return Lowest(s.hull, s.hullPoints, bone); }

// A small steady noise, the same every run (loosetest's).
unsigned g_noise = 12345u;
float Noise() {
    g_noise = g_noise * 1664525u + 1013904223u;
    return static_cast<float>((g_noise >> 8) & 0xFFFF) / 65535.0f * 2.0f - 1.0f;
}

// The gun turned by turn (3x4), its lowest hull point height above the ground.
void Place(const loose::Spec& s, const float* turn, float height, float* bone) {
    std::memcpy(bone, turn, sizeof(float) * 12);
    bone[3] = bone[7] = bone[11] = 0.0f;
    bone[7] = height - LowestHull(s, bone);
}
// Rolled about its barrel (its z) by roll radians.
void Rolled(const loose::Spec& s, float roll, float height, float* bone) {
    const float along[3] = {0.0f, 0.0f, 1.0f};
    float turn[12];
    Turn(turn, along, roll);
    Place(s, turn, height, bone);
}
bool AllFinite(const float* p, int n) {
    for (int i = 0; i < n; ++i)
        if (!std::isfinite(p[i]))
            return false;
    return true;
}

// Its mesh's outermost vertices, as gunbodyfit wrote them, with the hull box and the middle of the mesh they came from (the header's, when
// they're its).
struct Outer {
    std::string shape;
    float footprint = 0.0f;
    float hullMin[3] = {}, hullMax[3] = {}, middle[3] = {};
    std::vector<float> points;
};
std::vector<Outer> g_outer;
bool g_haveOuter = false;
const char* g_outerWhy = "has no bodies";  // (why there are none)
void ReadOuter(const char* path) {
    FILE* f = nullptr;
    if (fopen_s(&f, path, "rb") || !f) {
        g_outerWhy = "isn't there";
        return;
    }
    char magic[4] = {};
    int bodies = 0;
    if (std::fread(magic, 1, 4, f) != 4 || std::memcmp(magic, "BTP3", 4) != 0) {
        g_outerWhy = std::memcmp(magic, "BTP", 3) == 0 ? "is an older gunbodyfit's (nothing in it ties it to the header)" : "isn't gunbodyfit's";
    } else if (std::fread(&bodies, sizeof(int), 1, f) == 1 && bodies > 0 && bodies < 256) {
        for (int b = 0; b < bodies; ++b) {
            char shape[17] = {};
            int points = 0;
            Outer o;
            if (std::fread(shape, 1, 16, f) != 16 || std::fread(&points, sizeof(int), 1, f) != 1 || points < 0 || points > 4096 ||
                std::fread(&o.footprint, sizeof(float), 1, f) != 1 || std::fread(o.hullMin, sizeof(float), 3, f) != 3 ||
                std::fread(o.hullMax, sizeof(float), 3, f) != 3 || std::fread(o.middle, sizeof(float), 3, f) != 3 || !std::isfinite(o.footprint) ||
                !AllFinite(o.hullMin, 3) || !AllFinite(o.hullMax, 3) || !AllFinite(o.middle, 3))
                break;
            o.shape = shape;
            o.points.resize(static_cast<size_t>(points) * 3);
            if (std::fread(o.points.data(), sizeof(float), o.points.size(), f) != o.points.size() || !AllFinite(o.points.data(), static_cast<int>(o.points.size())))
                break;
            g_outer.push_back(o);
        }
        g_haveOuter = !g_outer.empty();
    }
    std::fclose(f);
}

// One body under test.
struct Row {
    const char* shape;
    const loose::Spec* spec;
    const float* hullMin;
    const float* hullMax;
    int restFrom;
    const Outer* outer;  // its mesh's outermost vertices, or none (the M416's)
    float footprint;     // how wide its bottom is (its mesh's, else its hull's)
    bool narrow;         // under 4 cm: it must never stand
    bool fitted = false; // one of gunbodies.h's: its mesh checks are skipped without its points (the M416's rows have none: made by hand)
    bool tube = false;   // a tube whose back is a ring (the AT4's venturi, 15.6 cm across): no keel (b6's keel rule is a flat bottom's), and rolled
                         // it may take a wide bottom's 5 s to settle (b4); it never stands all the same (narrow)
};

// What a run saw.
struct Seen {
    float lowest = 1e9f;   // its lowest hull point above the ground over the run (negative: into it)
    float restAt = -1.0f;
    float bone[12] = {};
    float outerAtRest = 1e9f;  // its mesh's outermost vertices at rest: the lowest
    float outerAt[3] = {};     // ... and where that one is on it
    bool finite = true;
};
Seen Run(const Row& row, int id, float seconds, const loose::Frame* look = nullptr) {
    Seen s;
    float time = 0.0f;
    const int frames = static_cast<int>(seconds / kDt + 0.5f);
    for (int i = 0; i < frames; ++i) {
        time += kDt;
        const loose::Result& r = loose::Step(look ? *look : Base());
        for (int k = 0; k < r.poses; ++k) {
            const loose::Pose& p = r.pose[k];
            if (p.id != id)
                continue;
            s.finite &= AllFinite(p.bone, 12);
            s.lowest = std::fmin(s.lowest, LowestHull(*row.spec, p.bone));
            if (p.resting && s.restAt < 0.0f)
                s.restAt = time;
            std::memcpy(s.bone, p.bone, sizeof(s.bone));
        }
    }
    if (row.outer && s.restAt > 0.0f) {
        int which = 0;
        s.outerAtRest = Lowest(reinterpret_cast<const float(*)[3]>(row.outer->points.data()), static_cast<int>(row.outer->points.size() / 3), s.bone, &which);
        std::memcpy(s.outerAt, &row.outer->points[static_cast<size_t>(which) * 3], sizeof(s.outerAt));
    }
    return s;
}
// Standing: its top still up, or on an end.
bool Standing(const float* bone) { return bone[5] > 0.7f || std::fabs(bone[6]) > 0.8f; }
// The mesh checks of fitted bodies skipped for want of their points: never passed, counted in the last line.
int g_meshSkipped = 0;
// Its mesh at rest: less than 1 cm in. Without its outermost vertices true, a fitted body's counted as skipped (once per check: called before it).
bool OuterOk(const Row& row, const Seen& s) {
    if (!row.outer) {
        g_meshSkipped += row.fitted;
        return true;
    }
    return s.outerAtRest > -0.01f;
}
std::string OuterText(const Row& row, const Seen& s) {
    char text[160];
    if (!row.outer)
        std::snprintf(text, sizeof(text), "%s", row.fitted ? "its mesh SKIPPED (no points for it)" : "its mesh not checked (none for it)");
    else
        std::snprintf(text, sizeof(text), "its mesh at rest %.1f mm in at most (its vertex at %.3f %.3f %.3f)", std::fmax(0.0f, -s.outerAtRest) * 1000.0f, s.outerAt[0],
                      s.outerAt[1], s.outerAt[2]);
    return text;
}

void TestRow(const Row& row) {
    const loose::Spec& spec = *row.spec;
    const int rounds = spec.capacity;
    const float z[3] = {0, 0, 1}, x[3] = {1, 0, 0};
    std::printf("---- %s (%s): %d hull points, the rest from %d, its bottom %.1f cm wide (%s)\n", row.shape, spec.name, spec.hullPoints, row.restFrom,
                row.footprint * 100.0f, row.narrow ? "narrow: it never stands" : "wide: it may stand on it");

    // b1: near upright, rolled 2 degrees either way.
    for (float roll : {2.0f, -2.0f}) {
        loose::Clear();
        float bone[12];
        Rolled(spec, roll * kDegree, 1.0f, bone);
        const int id = loose::ReleaseBy(spec, kGunKind, bone, rounds, Base(), 1);
        const Seen s = Run(row, id, 4.0f);
        const bool standing = Standing(s.bone);
        const float within = row.narrow ? 3.0f : 4.0f;
        const bool meshOk = OuterOk(row, s);
        CheckF(id && s.finite && s.restAt > 0.0f && s.restAt < within && (!row.narrow || !standing) && s.lowest > -0.002f && meshOk,
               "b1 %s dropped from 1 m near upright, rolled %.0f degrees onto its %s: at rest at %.2f s (within %.0f), %s (its top %.2f of up), never a hull point "
               "2 mm in (deepest %.2f mm); %s",
               row.shape, std::fabs(roll), roll > 0.0f ? "right" : "left", s.restAt, within, standing ? "STANDING" : "not standing", s.bone[5],
               std::fmax(0.0f, -s.lowest) * 1000.0f, OuterText(row, s).c_str());
    }
    // b2: on either side.
    for (float roll : {90.0f, -90.0f}) {
        loose::Clear();
        float bone[12];
        Rolled(spec, roll * kDegree, 1.0f, bone);
        const int id = loose::ReleaseBy(spec, kGunKind, bone, rounds, Base(), 1);
        const Seen s = Run(row, id, 4.0f);
        const float across = s.bone[4];  // its x (its left) against up: + lying on its right side, - on its left
        const bool thatSide = (roll > 0.0f ? across : -across) > 0.5f;
        const bool meshOk = OuterOk(row, s);
        CheckF(id && s.finite && s.restAt > 0.0f && s.restAt < 3.0f && thatSide && s.lowest > -0.002f && meshOk,
               "b2 %s dropped from 1 m on its %s side: at rest at %.2f s (within 3), %s (its x %.2f of up), never a hull point 2 mm in (deepest %.2f mm); %s",
               row.shape, roll > 0.0f ? "right" : "left", s.restAt, thatSide ? "on that side" : "NOT on that side", across, std::fmax(0.0f, -s.lowest) * 1000.0f,
               OuterText(row, s).c_str());
    }
    // b3: near upright every way.
    {
        int runs = 0, standing = 0, unrested = 0, deep = 0;
        float deepest = 0.0f, outerDeepest = 0.0f, where[3] = {};
        for (float height : {0.05f, 0.3f, 1.0f})
            for (float pitch : {-20.0f, -5.0f, 0.0f, 5.0f, 20.0f})
                for (int roll = -6; roll <= 6; ++roll) {
                    loose::Clear();
                    float rolled[12], pitched[12], turn[12], bone[12];
                    Turn(rolled, z, static_cast<float>(roll) * kDegree);
                    Turn(pitched, x, pitch * kDegree);
                    Compose(pitched, rolled, turn);
                    Place(spec, turn, height, bone);
                    const int id = loose::ReleaseBy(spec, kGunKind, bone, rounds, Base(), 1);
                    const Seen s = Run(row, id, 4.0f);
                    ++runs;
                    standing += Standing(s.bone);
                    unrested += s.restAt < 0.0f;
                    deepest = std::fmin(deepest, s.lowest);
                    if (row.outer && s.restAt > 0.0f) {
                        if (s.outerAtRest < outerDeepest)
                            outerDeepest = s.outerAtRest, std::memcpy(where, s.outerAt, sizeof(where));
                        deep += s.outerAtRest <= -0.01f;
                    }
                }
        char outer[160];
        if (row.outer)
            std::snprintf(outer, sizeof(outer), "%d with its mesh 1 cm in at rest (deepest %.1f mm, its vertex at %.3f %.3f %.3f)", deep, -outerDeepest * 1000.0f, where[0],
                          where[1], where[2]);
        else
            std::snprintf(outer, sizeof(outer), "%s", row.fitted ? "its mesh SKIPPED (no points for it)" : "its mesh not checked");
        g_meshSkipped += !row.outer && row.fitted;
        CheckF((!row.narrow || standing == 0) && unrested == 0 && deepest > -0.002f && deep == 0,
               "b3 %s dropped %d ways near upright: %d left standing%s, %d not at rest after 4 s, the deepest hull point %.2f mm in; %s", row.shape, runs, standing,
               row.narrow ? "" : " (it may)", unrested, -deepest * 1000.0f, outer);
    }
    // b4: thrown 200 ways (loosetest's g2 throws, the same each run).
    {
        g_noise = 777u;
        const float within = row.narrow && !row.tube ? 4.0f : 5.0f;
        int standing = 0, late = 0;
        float latest = 0.0f, deepest = 1e9f, latestUp[3] = {};
        int latestThrow = -1;
        double micros = 0.0;
        long long moving = 0;
        for (int n = 0; n < 200; ++n) {
            loose::Clear();
            float axis[3] = {Noise(), Noise(), Noise()};
            const float l = std::fmax(Length(axis), 1e-3f);
            for (float& a : axis)
                a /= l;
            float turn[12], bone[12];
            Turn(turn, axis, kPi * (Noise() + 1.0f));
            Place(spec, turn, 0.3f + 0.6f * (Noise() + 1.0f), bone);
            loose::Frame f = Base();
            loose::HandAt& h = f.hands[1];
            h.tracked = h.sensed = true;
            Identity(h.room);
            h.room[7] = bone[7];
            for (int i = 0; i < 3; ++i)
                h.velocity[i] = 1.5f * Noise(), h.spin[i] = 6.0f * Noise();
            const int id = loose::ReleaseBy(spec, kGunKind, bone, rounds, f, 1);
            float restAt = -1.0f, time = 0.0f, last[12] = {};
            for (int i = 0; i < static_cast<int>(6.0f / kDt); ++i) {
                time += kDt;
                const auto before = std::chrono::steady_clock::now();
                const loose::Result& r = loose::Step(Base());
                const double took = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - before).count();
                for (int k = 0; k < r.poses; ++k)
                    if (r.pose[k].id == id) {
                        std::memcpy(last, r.pose[k].bone, sizeof(last));
                        deepest = std::fmin(deepest, LowestHull(spec, last));
                        if (r.pose[k].resting && restAt < 0.0f)
                            restAt = time;
                        if (!r.pose[k].resting)
                            micros += took, ++moving;
                    }
            }
            standing += Standing(last);
            late += restAt < 0.0f || restAt > within;
            if ((restAt < 0.0f ? 99.0f : restAt) > latest)
                latest = restAt < 0.0f ? 99.0f : restAt, latestThrow = n, latestUp[0] = last[4], latestUp[1] = last[5], latestUp[2] = last[6];
        }
        const double each = moving ? micros / static_cast<double>(moving) : 0.0;
        CheckF((!row.narrow || standing == 0) && late == 0 && deepest > -0.002f && each < 40.0,
               "b4 %s thrown 200 ways: %d left standing%s, %d not at rest within %.0f s (the latest %.2f s, throw %d, at rest with (%.2f %.2f %.2f) of its "
               "axes up), the deepest hull point %.2f mm in; %.1f us a frame while it moves",
               row.shape, standing, row.narrow ? "" : " (it may)", late, within, latest, latestThrow, latestUp[0], latestUp[1], latestUp[2], -deepest * 1000.0f, each);
    }
    // b5: lying 120 s, looked away.
    {
        loose::Clear();
        float bone[12];
        Rolled(spec, kPi / 2, 0.2f, bone);
        const int id = loose::ReleaseBy(spec, kGunKind, bone, rounds, Base(), 1);
        bool told = false, finite = true;
        loose::Frame look = Base();
        look.look[0] = 0.0f, look.look[1] = 1.0f, look.look[2] = 0.0f;  // looking up, away from it
        for (int i = 0; i < static_cast<int>(120.0f / kDt); ++i) {
            const loose::Result& r = loose::Step(look);
            for (int k = 0; k < r.gone; ++k)
                told |= r.goneId[k] == id;
            for (int k = 0; k < r.poses; ++k)
                if (r.pose[k].id == id)
                    finite &= AllFinite(r.pose[k].bone, 12);
        }
        CheckF(id && !told && loose::Has(id) && finite, "b5 %s lying 120 s, looked away: still there (%s), never told gone, its numbers fine", row.shape,
               loose::Has(id) ? "yes" : "no");
    }
    // b6: the fit's own rules.
    {
        bool inBox = true, finite = AllFinite(spec.boxMin, 3) && AllFinite(spec.boxMax, 3);
        float low = 1e9f;
        for (int i = 0; i < spec.hullPoints; ++i) {
            for (int k = 0; k < 3; ++k)
                inBox &= spec.hull[i][k] >= row.hullMin[k] - 0.001f && spec.hull[i][k] <= row.hullMax[k] + 0.001f;
            finite &= AllFinite(spec.hull[i], 3);
            low = std::fmin(low, spec.hull[i][1]);
        }
        // Its keel: the hull's lowest point's x. A narrow bottom stood upright, rolled up to 10 degrees and pitched up to 15, touches with its keel's
        // points alone (its guards, the bottom's own corners held 5 mm above the keel, only once it's rolled further: one 2 cm out at 14 degrees).
        float keelX = 0.0f;
        for (int i = 0; i < spec.hullPoints; ++i)
            if (spec.hull[i][1] == low)
                keelX = spec.hull[i][0];
        bool onKeel = true;
        int offRoll = 0, offPitch = 0, off = -1;
        for (int roll = -10; roll <= 10 && row.narrow && !row.tube; ++roll)
            for (int pitch = -15; pitch <= 15; pitch += 5) {
                const float r = static_cast<float>(roll) * kDegree, p = static_cast<float>(pitch) * kDegree;
                const float down[3] = {std::sin(r) * std::cos(p), -std::cos(r) * std::cos(p), std::sin(p)};  // the ground's way in its own axes
                float most = -1e9f;
                for (int i = 0; i < spec.hullPoints; ++i)
                    most = std::fmax(most, spec.hull[i][0] * down[0] + spec.hull[i][1] * down[1] + spec.hull[i][2] * down[2]);
                for (int i = 0; i < spec.hullPoints; ++i)
                    if (spec.hull[i][0] * down[0] + spec.hull[i][1] * down[1] + spec.hull[i][2] * down[2] > most - 0.0001f && spec.hull[i][0] != keelX) {
                        if (onKeel)
                            offRoll = roll, offPitch = pitch, off = i;
                        onKeel = false;
                    }
            }
        const float middleX = 0.5f * (spec.boxMin[0] + spec.boxMax[0]);
        const bool beside = !row.narrow || row.tube || std::fabs(middleX - keelX) >= 0.0039f;  // (a wide bottom has no keel, nor a tube's ring)
        bool sized = true;
        for (int k = 0; k < 3; ++k)
            sized &= spec.boxMax[k] > spec.boxMin[k] && row.hullMax[k] > row.hullMin[k];
        const size_t name = std::strlen(spec.name);
        const bool gunName = name > 3 && std::strcmp(spec.name + name - 3, "gun") == 0;
        CheckF(inBox && finite && onKeel && beside && spec.hullPoints >= 4 && spec.hullPoints <= bodyfit::kMostHull && sized && spec.massEmpty > 0.0f &&
                   spec.keep && gunName && row.restFrom >= 0 && row.restFrom <= spec.hullPoints,
               "b6 %s's fit: its hull points in its hull box (%s), %s, its middle %.1f mm beside its keel at x %.4f (at least 4), %d points (4-22), its box %s, "
               "%.2f kg, kept, named \"%s\", the rest from %d",
               row.shape, inBox ? "yes" : "NO",
               row.tube ? "a tube: its back a ring, no keel" : row.narrow ? (onKeel ? std::string("upright it touches on its keel alone")
                                    : "upright it touches OFF its keel (rolled " + std::to_string(offRoll) + ", pitched " + std::to_string(offPitch) +
                                          ": its point " + std::to_string(off) + ")").c_str()
                          : "its bottom wide",
               std::fabs(middleX - keelX) * 1000.0f, keelX, spec.hullPoints, sized && finite ? "sane" : "NOT sane", spec.massEmpty, spec.name, row.restFrom);
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    const char* pointsPath = argc > 1 ? argv[1] : "build\\bodytest_points.bin";
    ReadOuter(pointsPath);
    if (!g_haveOuter)
        std::printf("(%s %s: the checks of each mesh's outermost vertices are skipped, never passed: run gunbodyfit build src\\gunbodies.h)\n", pointsPath,
                    g_outerWhy);

    std::vector<Row> rows;
    auto footprintOf = [](const loose::Spec& s) {  // its hull's bottom, within 1 cm of its lowest point: how wide
        float low = 1e9f, lo = 1e9f, hi = -1e9f;
        for (int i = 0; i < s.hullPoints; ++i)
            low = std::fmin(low, s.hull[i][1]);
        for (int i = 0; i < s.hullPoints; ++i)
            if (s.hull[i][1] < low + 0.01f)
                lo = std::fmin(lo, s.hull[i][0]), hi = std::fmax(hi, s.hull[i][0]);
        return hi >= lo ? hi - lo : 0.0f;
    };
    rows.push_back({"m416", &gunbody::kM416, gunbody::kM416HullMin, gunbody::kM416HullMax, gunbody::kM416.hullPoints, nullptr, footprintOf(gunbody::kM416), true});
    rows.push_back({"m416 bare", &gunbodies::kM416Bare, gunbody::kM416HullMin, gunbody::kM416HullMax, gunbodies::kM416Bare.hullPoints, nullptr,
                    footprintOf(gunbodies::kM416Bare), true});
    int bodies = 0;
    std::string without;  // the fitted bodies whose mesh can't be checked
    for (const gunbodies::Body& b : gunbodies::kBodies) {
        if (!b.spec)
            continue;
        ++bodies;
        const Outer* outer = nullptr;
        for (const Outer& o : g_outer)
            if (o.shape == b.shape)
                outer = &o;
        // Its points are its header's only if they came from the same kept mesh: its hull box, and its middle (its Spec's box's: every kept
        // triangle moves it, the fit's choices don't), to the header's 4 decimals. Else another fit's (a mesh dumped again, a part added): not used.
        bool another = false;
        for (int k = 0; outer && k < 3; ++k)
            another |= std::fabs(outer->hullMin[k] - b.hullMin[k]) > 0.0002f || std::fabs(outer->hullMax[k] - b.hullMax[k]) > 0.0002f ||
                       std::fabs(outer->middle[k] - 0.5f * (b.spec->boxMin[k] + b.spec->boxMax[k])) > 0.0002f;
        if (!outer || another)
            without += (without.empty() ? "" : ", ") + std::string(b.shape) + (another ? " (its points another fit's)" : "");
        if (another)
            outer = nullptr;
        const float footprint = outer ? outer->footprint : footprintOf(*b.spec);
        rows.push_back({b.shape, b.spec, b.hullMin, b.hullMax, b.restFrom, outer, footprint, footprint < 0.04f, true, std::strcmp(b.shape, "at4") == 0});
    }
    CheckF(bodies >= 12, "the bodies fitted: %d (every held gun, the Stinger, the SMAW with its case and without: 12)", bodies);
    CheckF(without.empty(), "every fitted body's outermost vertices in %s for b1, b2, b3 and b7: %s", pointsPath,
           without.empty() ? "yes" : ("NONE for " + without + ": their mesh checks are skipped, never passed (run gunbodyfit build src\\gunbodies.h)").c_str());
    for (const Row& row : rows)
        TestRow(row);

    // b7: the SMAW with its case and without, dropped nose up from 0.5 m.
    for (const char* shape : {"smaw", "smaw_bare"}) {
        const gunbodies::Body* b = gunbodies::For(shape);
        const Row* row = nullptr;
        for (const Row& r : rows)
            if (std::strcmp(r.shape, shape) == 0)
                row = &r;
        if (!b || !row) {
            CheckF(false, "b7 %s: no body", shape);
            continue;
        }
        loose::Clear();
        const float x[3] = {1, 0, 0};
        float turn[12], bone[12];
        Turn(turn, x, -80.0f * kDegree);  // its z (its muzzle) up
        Place(*b->spec, turn, 0.5f, bone);
        const int id = loose::ReleaseBy(*b->spec, kGunKind, bone, 1, Base(), 1);
        const Seen s = Run(*row, id, 5.0f);
        const bool onEnd = std::fabs(s.bone[6]) > 0.8f;
        const bool meshOk = OuterOk(*row, s);
        CheckF(id && s.finite && s.restAt > 0.0f && s.restAt < 4.0f && !onEnd && s.lowest > -0.002f && meshOk,
               "b7 %s dropped nose up (80 degrees) from 0.5 m: it tips over, at rest at %.2f s (within 4), %s (its z %.2f of up), never a hull point 2 mm in "
               "(deepest %.2f mm); %s",
               shape, s.restAt, onEnd ? "ON ITS END" : "not on its end", s.bone[6], std::fmax(0.0f, -s.lowest) * 1000.0f, OuterText(*row, s).c_str());
    }

    // b8: the AN-94 with a PSO-1 on its left, over its receiver (its bone 0 on the frame at (0.045 0.055 0.35), as a side mount has it): its points
    // merged in before the rest; dropped on either side, none of the sight's points 1 cm in (the sight sticks out 5 cm past the receiver's side).
    {
        const gunbodies::Body* an94 = gunbodies::For("an94");
        const gunbodies::Attach* pso = gunbodies::AttachFor(10599, 72, 0);
        if (!an94 || !pso) {
            CheckF(false, "b8 the AN-94 with a PSO-1: %s", an94 ? "no PSO-1 in kAttach" : "no AN-94 body");
        } else {
            constexpr float kMount[3] = {0.045f, 0.055f, 0.35f};
            float sight[8][3];
            for (int i = 0; i < pso->points; ++i)
                for (int k = 0; k < 3; ++k)
                    sight[i][k] = pso->point[i][k] + kMount[k];
            static float hull[bodyfit::kMostHull][3];
            const int n = bodyfit::WithExtra(an94->spec->hull, an94->spec->hullPoints, an94->restFrom, sight, pso->points, hull);
            int merged = 0;
            for (int i = 0; i < pso->points; ++i)
                for (int j = 0; j < n; ++j)
                    merged += std::memcmp(hull[j], sight[i], sizeof(hull[j])) == 0;
            loose::Spec spec = *an94->spec;
            spec.hull = hull, spec.hullPoints = n;
            float hullMin[3], hullMax[3];
            for (int k = 0; k < 3; ++k) {
                hullMin[k] = an94->hullMin[k], hullMax[k] = an94->hullMax[k];
                for (int i = 0; i < pso->points; ++i)
                    hullMin[k] = std::fmin(hullMin[k], sight[i][k]), hullMax[k] = std::fmax(hullMax[k], sight[i][k]);
            }
            const Row row{"an94 with a PSO-1", &spec, hullMin, hullMax, an94->restFrom, nullptr, 0.025f, true};
            for (float roll : {90.0f, -90.0f}) {
                loose::Clear();
                float bone[12];
                Rolled(spec, roll * kDegree, 1.0f, bone);
                const int id = loose::ReleaseBy(spec, kGunKind, bone, 30, Base(), 1);
                const Seen s = Run(row, id, 4.0f);
                const float sightLow = Lowest(sight, pso->points, s.bone);
                CheckF(id && s.finite && s.restAt > 0.0f && n <= bodyfit::kMostHull && merged >= 1 && sightLow > -0.01f && s.lowest > -0.002f,
                       "b8 the AN-94 with a PSO-1 on its left (%d of its 8 points in a hull of %d), dropped on its %s side: at rest at %.2f s, the sight's lowest "
                       "point %.1f mm %s the ground (less than 10 in), its hull's deepest %.2f mm in",
                       merged, n, roll > 0.0f ? "right" : "left", s.restAt, std::fabs(sightLow) * 1000.0f, sightLow < 0.0f ? "into" : "above",
                       std::fmax(0.0f, -s.lowest) * 1000.0f);
            }
            // (And without its points: how far the sight would go in, for the record.)
            loose::Clear();
            float bone[12];
            Rolled(*an94->spec, -90.0f * kDegree, 1.0f, bone);
            const int id = loose::ReleaseBy(*an94->spec, kGunKind, bone, 30, Base(), 1);
            const Row plain{"an94", an94->spec, an94->hullMin, an94->hullMax, an94->restFrom, nullptr, 0.025f, true};
            const Seen s = Run(plain, id, 4.0f);
            std::printf("(without the sight's points, on its left side its sight's lowest point is %.1f mm into the ground)\n",
                        std::fmax(0.0f, -Lowest(sight, pso->points, s.bone)) * 1000.0f);
        }
    }

    std::printf("%d checks, %d failed, %d mesh checks skipped (no points)\n", g_checks, g_failures, g_meshSkipped);
    return g_failures;
}

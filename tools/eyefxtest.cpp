// Each eye's own passes without the game (src/eyefx.h and src/markermath.h, header only, as overlay.cpp, shaders.cpp, skyrays.cpp and markers.cpp run them):
//   e1  the HUD's disparity: 29 px at 2 m for 64 mm on the owner's 2400 px over 105.88 degrees, 39 at 1.5 m, 19 at 3 m; nothing asked, broken numbers: 0
//   e2  each eye's share in whole pixels; capped; held against a hair's change of the eye distance
//   e3  the HUD's viewport and scissor placed: unchanged at k 1 and no shift; scaled about the eye's middle; moved; a window's viewport alike; the clip
//   e4  the scale kept within [0.5, 1]; the depth of field's two composites by their checksums
//   m1  the markers' matrix taken apart and put back: cameras turned every way, far from the origin, 30-120 degrees, off-centre windows, three depth
//       mappings, a scale, both layouts: the layout, axes, eye, angles and depth mapping found again
//   m2  what isn't a camera's view-projection refused: an orthographic one, a sheared one, a broken one, garbage
//   m3  the axes against a camera's rows: the game camera's own learned (+1 +1 +1), a flipped up learned (-1), a camera 5 degrees off refused at 3
//   m4  rebuilt for the head: what is straight ahead of the head in the middle of the picture, the eyes' half angle at its edge, above it up, the depth
//       mapping kept, the layout kept, a flipped up kept flipped; the game's matrix rebuilt for the game's own camera is the game's matrix
//   m5  the heading both ways round, the angle's wrap
//   s1  the sky's shaders known by their bytecode (compiled here, shader models 4 and 5): the sky's taken, five near misses refused; every truncation and
//       a thousand broken copies read without a fault
//   s2  the game's own (a folder's ps\*.dxbc, build by default: SKIP without it): the sky's taken (78425db3e8d75475 among them), the depth of field's,
//       the clear middle and the damage tint not
//   s3  the mod's sky vertex shader on a software graphics card (WARP): its outputs as the sky's shader reads them (SV_Position in register 0, the ray in
//       TEXCOORD0's x, y, z in register 1); each pixel's ray along its own line to 0.005 degrees, centred and off-centre (0.2 out, 0.15 down); the corners
//       normalised one by one (the bend the mod's rays don't have) shown off-centre and not centred
//   c1  the code the hooks need, as the game has it (bf3_mem.exe in that folder: SKIP without it): the icons' update and the heading, their first bytes,
//       their returns, the heading's game camera and its two negations
//   t   the time
// Usage: eyefxtest [<folder with ps\ and bf3_mem.exe>]   (build by default, from the project's folder; the exit code is the number of failed checks)

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#include "../src/eyefx.h"
#include "../src/markermath.h"

namespace {

int g_failed = 0, g_counted = 0, g_skipped = 0;

void Report(const char* id, const char* what, bool pass, const char* format, ...) {
    char numbers[1024];
    va_list args;
    va_start(args, format);
    std::vsnprintf(numbers, sizeof(numbers), format, args);
    va_end(args);
    std::printf("%-4s %-5s %s | %s\n", id, pass ? "PASS" : "FAIL", what, numbers);
    ++g_counted;
    g_failed += !pass;
}

void Skip(const char* id, const char* what) {
    std::printf("%-4s SKIP  %s\n", id, what);
    ++g_skipped;
}

constexpr double kPi = 3.14159265358979323846;
constexpr float kOwnersFov = 1.84795f;  // 2 atan(1200 / 906.2271): the owner's 2400 px at 2160's sharpness over 100 degrees (105.88)

// ---- e: the HUD ----
void Hud() {
    const float at2 = eyefx::HudDisparity(0.064f, 2400.0f, kOwnersFov, 2.0f);
    const float at15 = eyefx::HudDisparity(0.064f, 2400.0f, kOwnersFov, 1.5f);
    const float at3 = eyefx::HudDisparity(0.064f, 2400.0f, kOwnersFov, 3.0f);
    Report("e1a", "the HUD's disparity on the owner's picture: 29 px at 2 m, 39 at 1.5, 19 at 3 (the study's numbers)",
           std::fabs(at2 - 29.0f) < 0.1f && std::fabs(at15 - 38.67f) < 0.1f && std::fabs(at3 - 19.33f) < 0.1f, "%.2f %.2f %.2f px", at2, at15, at3);
    const float none[] = {eyefx::HudDisparity(0.064f, 2400.0f, kOwnersFov, 0.0f), eyefx::HudDisparity(0.064f, 2400.0f, kOwnersFov, -1.0f),
                          eyefx::HudDisparity(0.064f, 2400.0f, kOwnersFov, NAN), eyefx::HudDisparity(0.0f, 2400.0f, kOwnersFov, 2.0f),
                          eyefx::HudDisparity(NAN, 2400.0f, kOwnersFov, 2.0f), eyefx::HudDisparity(0.064f, 0.0f, kOwnersFov, 2.0f),
                          eyefx::HudDisparity(0.064f, 2400.0f, 0.0f, 2.0f), eyefx::HudDisparity(0.064f, 2400.0f, 3.2f, 2.0f),
                          eyefx::HudDisparity(0.5f, 2400.0f, kOwnersFov, 2.0f)};
    bool allNone = true;
    for (float v : none)
        allNone = allNone && v == 0.0f;
    const float nearest = eyefx::HudDisparity(0.064f, 2400.0f, kOwnersFov, 0.1f), farthest = eyefx::HudDisparity(0.064f, 2400.0f, kOwnersFov, 1e6f);
    Report("e1b", "no depth asked (0, below 0, NaN), no eye distance, no picture, an angle out of reason: 0; nearer than 0.5 m held at 0.5, past 100 m at 100",
           allNone && std::fabs(nearest - eyefx::HudDisparity(0.064f, 2400.0f, kOwnersFov, 0.5f)) < 1e-3f &&
               std::fabs(farthest - eyefx::HudDisparity(0.064f, 2400.0f, kOwnersFov, 100.0f)) < 1e-4f,
           "%s; at 0.1 m %.1f px, at 1e6 m %.3f px", allNone ? "all 0" : "NOT all 0", nearest, farthest);
    Report("e2", "each eye's share in whole pixels (29.0 -> 14 or 15, 40 -> 20, 0 -> 0, NaN -> 0, 1e6 -> 256)",
           (eyefx::HudShift(at2) == 14 || eyefx::HudShift(at2) == 15) && eyefx::HudShift(40.0f) == 20 && eyefx::HudShift(0.0f) == 0 && eyefx::HudShift(NAN) == 0 &&
               eyefx::HudShift(1e6f) == eyefx::kMostShift && eyefx::HudShift(-3.0f) == 0,
           "%d %d %d", eyefx::HudShift(at2), eyefx::HudShift(40.0f), eyefx::HudShift(1e6f));
    // Held against a hair's change: the owner's 28.999 px read as 29.002 and back stays where it was; a real change (to 1.5 m) moves at once.
    {
        int kept = -1;
        std::string seen;
        const float reads[] = {at2, 29.002f, 28.996f, 29.004f, 28.998f, at15, at15 + 0.01f, at2};
        for (float d : reads) {
            kept = eyefx::HudShiftKept(d, kept);
            seen += std::to_string(kept) + " ";
        }
        const int first = eyefx::HudShift(at2);
        bool held = true;
        int k = -1;
        for (int i = 0; i < 5; ++i)
            held = held && (k = eyefx::HudShiftKept(reads[i], i ? k : -1)) == first;
        Report("e2b", "each eye's share held against a hair's change (28.999 / 29.002 / 28.996 px: one number), moved at once by a real one (1.5 m: 19)",
               held && eyefx::HudShiftKept(at15, first) == 19 && eyefx::HudShiftKept(0.0f, 15) == 0 && eyefx::HudShiftKept(at2, -1) == first, "%s", seen.c_str());
    }
    const eyefx::Viewport eye{0, 0, 2400, 2400}, window{0, 0, 4800, 2400};
    const eyefx::Viewport same = eyefx::Place(eye, 2400.0f, 1.0f, 0.0f), moved = eyefx::Place(eye, 2400.0f, 1.0f, 15.0f),
                          scaled = eyefx::Place(eye, 2400.0f, 0.8f, 0.0f), both = eyefx::Place(eye, 2400.0f, 0.8f, -15.0f),
                          wide = eyefx::Place(window, 2400.0f, 0.8f, 2385.0f);
    Report("e3a", "the viewport: as it is at k 1 and no shift; moved 15; 0.8 about the eye's middle (240, 240, 1920 square); both; the window's alike (240, 3840 wide, +2385)",
           same.x == 0 && same.y == 0 && same.w == 2400 && same.h == 2400 && moved.x == 15 && moved.w == 2400 && std::fabs(scaled.x - 240) < 1e-3f &&
               std::fabs(scaled.y - 240) < 1e-3f && std::fabs(scaled.w - 1920) < 1e-3f && std::fabs(both.x - 225) < 1e-3f && std::fabs(wide.x - 2625) < 1e-3f &&
               std::fabs(wide.w - 3840) < 1e-3f,
           "moved %.0f; scaled %.1f %.1f %.1f; both %.1f; window %.1f %.1f", moved.x, scaled.x, scaled.y, scaled.w, both.x, wide.x, wide.w);
    const eyefx::Rect r{100, 200, 300, 400};
    const eyefx::Rect same2 = eyefx::PlaceRect(r, 2400.0f, 1.0f, 0.0f), half = eyefx::PlaceRect({1, 1, 3, 3}, 4.0f, 0.5f, 0.0f);
    const eyefx::Rect clipped = eyefx::Intersect(eyefx::PlaceRect({2350, 0, 2400, 10}, 2400.0f, 1.0f, 15.0f), {0, -1000, 2400, 1000});
    const eyefx::Rect empty = eyefx::Intersect({0, 0, 10, 10}, {20, 20, 30, 30});
    Report("e3b", "the scissor: as it is unscaled; scaled outward to whole pixels; clipped to its eye (2365..2400); nothing shared, nothing drawn",
           same2.left == 100 && same2.top == 200 && same2.right == 300 && same2.bottom == 400 && half.left == 1 && half.right == 3 && clipped.left == 2365 &&
               clipped.right == 2400 && empty.right <= empty.left && empty.bottom <= empty.top,
           "scaled %ld..%ld; clipped %ld..%ld; empty %ld..%ld", half.left, half.right, clipped.left, clipped.right, empty.left, empty.right);
    Report("e4", "the scale within [0.5, 1] (1.2 -> 1, 0.3 -> 0.5, NaN -> 1); the depth of field's two composites by checksum, nothing else",
           eyefx::HudScale(1.2f) == 1.0f && eyefx::HudScale(0.3f) == 0.5f && eyefx::HudScale(NAN) == 1.0f && eyefx::HudScale(0.8f) == 0.8f &&
               eyefx::IsDofComposite(0x94a5f6c1e17c947dull) && eyefx::IsDofComposite(0x2b2ca40a51aa1b4dull) && !eyefx::IsDofComposite(0x5553fe3fbb6528bbull) &&
               !eyefx::IsDofComposite(0),
           "ok");
}

// ---- m: the markers' matrix ----
struct Cam {
    double right[3], up[3], back[3], eye[3];
};
Cam MakeCam(double yaw, double pitch, double roll, const double eye[3]) {
    // The game's rows: the camera looks along -back; yaw about up (y), pitch about right, roll about the line ahead.
    const double cy = std::cos(yaw), sy = std::sin(yaw), cp = std::cos(pitch), sp = std::sin(pitch), cr = std::cos(roll), sr = std::sin(roll);
    const double ahead[3] = {-sy * cp, sp, -cy * cp};
    const double flatRight[3] = {cy, 0.0, -sy};
    double upv[3] = {flatRight[1] * ahead[2] - flatRight[2] * ahead[1], flatRight[2] * ahead[0] - flatRight[0] * ahead[2], flatRight[0] * ahead[1] - flatRight[1] * ahead[0]};
    Cam c{};
    for (int i = 0; i < 3; ++i) {
        c.right[i] = cr * flatRight[i] + sr * upv[i];
        c.up[i] = -sr * flatRight[i] + cr * upv[i];
        c.back[i] = -ahead[i];
        c.eye[i] = eye[i];
    }
    return c;
}
// A D3D-style view-projection with the game's window offset (views.cpp): clip = (sx x + cx d, sy y + cy d, a d + b, d), d the distance ahead.
void Vp(const Cam& c, double fovY, double aspect, double offX, double offY, double a, double b, double k, markermath::Layout layout, float out[16]) {
    const double sy = 1.0 / std::tan(0.5 * fovY), sx = sy / aspect;
    markermath::Camera m{};
    for (int i = 0; i < 3; ++i) {
        m.r[i] = float(c.right[i]);
        m.u[i] = float(c.up[i]);
        m.l[i] = float(-c.back[i]);
        m.e[i] = float(c.eye[i]);
    }
    // (A window moved by off of its width: its middle at 2 off of the half-width's tangent, the picture's -1..1 shifted by -2 off.)
    markermath::Projection p{float(sx), float(sy), float(-2.0 * offX), float(-2.0 * offY), float(a), float(b), float(k)};
    markermath::Encode(m, p, layout, out);
}

void Markers() {
    std::mt19937 rng(20261125);
    std::uniform_real_distribution<double> angle(-kPi, kPi), pitch(-1.4, 1.4), place(-3000.0, 3000.0), fov(30.0 * kPi / 180, 120.0 * kPi / 180),
        aspect(0.5, 2.5), off(-0.3, 0.3);
    int tried = 0, found = 0;
    double worstAxis = 0, worstEye = 0, worstScale = 0, worstResidual = 0;
    for (int n = 0; n < 2000; ++n) {
        const double eye[3] = {place(rng), place(rng) * 0.1, place(rng)};
        const Cam c = MakeCam(angle(rng), pitch(rng), n % 3 == 0 ? angle(rng) * 0.2 : 0.0, eye);
        const double f = fov(rng), asp = aspect(rng), ox = n % 2 ? off(rng) : 0.0, oy = n % 2 ? off(rng) : 0.0;
        const double nearZ = 0.06, farZ = 10000.0;
        double a = farZ / (farZ - nearZ), b = -nearZ * farZ / (farZ - nearZ);  // standard
        if (n % 4 == 1)
            a = -nearZ / (farZ - nearZ), b = nearZ * farZ / (farZ - nearZ);  // reversed
        else if (n % 4 == 2)
            a = 1.0, b = -nearZ;  // infinite far
        const double k = n % 5 == 0 ? 2.5 : 1.0;
        const markermath::Layout layout = n % 2 ? markermath::kColumnVector : markermath::kRowVector;
        float m[16];
        Vp(c, f, asp, ox, oy, a, b, k, layout, m);
        ++tried;
        const markermath::Decoded d = markermath::Decode(m);
        if (!d.ok || d.layout != layout) {
            if (std::getenv("EYEFX_DEBUG")) {
                const markermath::Decoded own = markermath::DecodeAs(m, layout);
                std::printf("  m1 case %d: %s (own layout: ok %d residual %.2e square %.2e handed %.4f; fov %.1f aspect %.2f off %.2f %.2f a %.3g b %.3g k %.1f "
                            "layout %d eye %.0f %.0f %.0f)\n",
                            n, d.ok ? "the other layout taken" : "refused", own.ok, own.residual, own.square, own.handed, f * 180 / kPi, asp, ox, oy, a, b, k,
                            int(layout), eye[0], eye[1], eye[2]);
            }
            continue;
        }
        const float right[3] = {float(c.right[0]), float(c.right[1]), float(c.right[2])};
        const float up[3] = {float(c.up[0]), float(c.up[1]), float(c.up[2])};
        const float ahead[3] = {float(-c.back[0]), float(-c.back[1]), float(-c.back[2])};
        const double axis = std::max({markermath::AngleDeg(d.cam.r, right), markermath::AngleDeg(d.cam.u, up), markermath::AngleDeg(d.cam.l, ahead)});
        double de = 0;
        for (int i = 0; i < 3; ++i)
            de = std::max(de, std::fabs(d.cam.e[i] - eye[i]));
        const double sy = 1.0 / std::tan(0.5 * f);
        // (The depth's constant comes back through the eye's distance along the line ahead, kilometres here: to a float's share of that.)
        double el = 0;
        for (int i = 0; i < 3; ++i)
            el += eye[i] * -c.back[i];
        const double bTolerance = 4e-7 * (std::fabs(a) * std::fabs(el) + std::fabs(b)) + 1e-9;
        const double scale = std::max({std::fabs(d.proj.sy / sy - 1.0), std::fabs(d.proj.sx / (sy / asp) - 1.0), std::fabs(d.proj.a / a - 1.0),
                                       std::fabs(d.proj.k / k - 1.0), std::fabs(d.proj.cx + 2.0 * ox) + std::fabs(d.proj.cy + 2.0 * oy),
                                       1e-3 * std::fabs(d.proj.b - b) / bTolerance});
        worstAxis = std::max(worstAxis, axis);
        worstEye = std::max(worstEye, de);
        worstScale = std::max(worstScale, scale);
        worstResidual = std::max(worstResidual, double(d.residual));
        if (axis < 0.01 && de < 0.01 && scale < 1e-3)
            ++found;
        else if (std::getenv("EYEFX_DEBUG"))
            std::printf("  m1 case %d: axis %.4f eye %.4f scale %.2e (fov %.1f aspect %.2f off %.2f %.2f a %.3g b %.3g k %.1f layout %d)\n", n, axis, de, scale,
                        f * 180 / kPi, asp, ox, oy, a, b, k, int(layout));
    }
    Report("m1", "2000 view-projections taken apart and put back (every turn, 3 km from the origin, 30-120 degrees, off-centre, three depth mappings, a scale, both layouts)",
           found == tried, "%d of %d found again; worst axis %.5f deg, eye %.4f m, numbers %.1e, residual %.1e", found, tried, worstAxis, worstEye, worstScale,
           worstResidual);
    // m2: refusals. (An orthographic matrix with a constant in its last row reads as a "camera" the other way round, at nearly 180 degrees: refused by its
    // angle.)
    float ortho[16] = {0.01f, 0, 0, 0, 0, 0.01f, 0, 0, 0, 0, 0.001f, 0, 0, 0, 0.5f, 1};
    float sheared[16];
    {
        const double eye[3] = {10, 2, -30};
        Vp(MakeCam(0.4, 0.1, 0, eye), 1.0, 1.0, 0, 0, 1.0, -0.06, 1.0, markermath::kRowVector, sheared);
        sheared[4] += 0.3f;  // the up axis leaning into the right
    }
    float broken[16];
    std::memcpy(broken, sheared, sizeof(broken));
    broken[5] = NAN;
    int garbageTaken = 0;
    std::uniform_real_distribution<float> any(-5.0f, 5.0f);
    for (int n = 0; n < 1000; ++n) {
        float g[16];
        for (float& v : g)
            v = any(rng);
        garbageTaken += markermath::Decode(g).ok;
    }
    Report("m2", "refused: an orthographic matrix, a sheared one, one with a NaN, 1000 random ones",
           !markermath::Decode(ortho).ok && !markermath::Decode(sheared).ok && !markermath::Decode(broken).ok && garbageTaken == 0, "%d of 1000 random taken",
           garbageTaken);
    // m3: the axes against the rows.
    {
        const double eye[3] = {120, 35, -340};
        const Cam game = MakeCam(0.7, -0.2, 0.0, eye);
        float m[16];
        Vp(game, 0.96, 2.0, 0, 0, 1.0, -0.06, 1.0, markermath::kRowVector, m);
        const markermath::Decoded d = markermath::Decode(m);
        markermath::Rows rows{};
        for (int i = 0; i < 3; ++i)
            rows.right[i] = float(game.right[i]), rows.up[i] = float(game.up[i]), rows.back[i] = float(game.back[i]), rows.eye[i] = float(game.eye[i]);
        markermath::Signs s;
        float worst = 0;
        const bool own = markermath::Learn(d.cam, rows, 3.0f, s, &worst);
        // A matrix with its up the other way (a picture whose y goes down).
        float flipped[16];
        std::memcpy(flipped, m, sizeof(m));
        for (int i = 0; i < 4; ++i)
            flipped[i * 4 + 1] = -flipped[i * 4 + 1];
        markermath::Signs sf;
        const bool flip = markermath::Learn(markermath::Decode(flipped).cam, rows, 3.0f, sf);
        const Cam off = MakeCam(0.7 + 5.0 * kPi / 180, -0.2, 0.0, eye);
        markermath::Rows offRows = rows;
        for (int i = 0; i < 3; ++i)
            offRows.right[i] = float(off.right[i]), offRows.up[i] = float(off.up[i]), offRows.back[i] = float(off.back[i]);
        markermath::Signs so;
        float offWorst = 0;
        const bool offTaken = markermath::Learn(d.cam, offRows, 3.0f, so, &offWorst);
        Report("m3", "the game camera's own matrix: +1 +1 +1 within 0.01 deg; its up reversed: -1 for up; 5 degrees off: refused at 3",
               own && s.r > 0 && s.u > 0 && s.l > 0 && worst < 0.01f && flip && sf.r > 0 && sf.u < 0 && sf.l > 0 && !offTaken && offWorst > 4.9f,
               "own %+.0f %+.0f %+.0f (%.4f deg); flipped up %+.0f; 5 off: %s (%.2f deg)", s.r, s.u, s.l, worst, sf.u, offTaken ? "TAKEN" : "refused", offWorst);
    }
    // m4: rebuilt for the head.
    {
        const double gameEye[3] = {120, 35, -340}, headEye[3] = {120.03, 35.1, -340.05};
        const Cam game = MakeCam(0.5, -0.1, 0.0, gameEye), head = MakeCam(0.0, 0.2, 0.05, headEye);
        const double nearZ = 0.06, farZ = 10000.0, a = farZ / (farZ - nearZ), b = -nearZ * farZ / (farZ - nearZ);
        bool all = true;
        char detail[512] = "";
        for (int layoutIndex = 0; layoutIndex < 2; ++layoutIndex)
            for (int flipUp = 0; flipUp < 2; ++flipUp) {
                const markermath::Layout layout = layoutIndex ? markermath::kColumnVector : markermath::kRowVector;
                float m[16];
                Vp(game, 55.0 * kPi / 180, 2.0, 0, 0, a, b, 1.0, layout, m);
                if (flipUp)
                    for (int i = 0; i < 4; ++i)
                        (layout == markermath::kRowVector ? m[i * 4 + 1] : m[4 + i]) *= -1.0f;
                const markermath::Decoded d = markermath::Decode(m);
                markermath::Rows gameRows{}, headRows{};
                for (int i = 0; i < 3; ++i) {
                    gameRows.right[i] = float(game.right[i]), gameRows.up[i] = float(game.up[i]), gameRows.back[i] = float(game.back[i]), gameRows.eye[i] = float(game.eye[i]);
                    headRows.right[i] = float(head.right[i]), headRows.up[i] = float(head.up[i]), headRows.back[i] = float(head.back[i]), headRows.eye[i] = float(head.eye[i]);
                }
                markermath::Signs s;
                const bool learned = markermath::Learn(d.cam, gameRows, 3.0f, s);
                const float tanHalf = std::tan(0.5f * kOwnersFov);
                float rebuilt[16];
                const bool made = learned && markermath::Rebuild(d, s, headRows, tanHalf, tanHalf, rebuilt);
                // Straight ahead of the head, 50 m: the picture's middle. At the eyes' half angle to its right: the edge (x 1). Above it: y up (or down when flipped).
                float ahead[3], edge[3], above[3];
                for (int i = 0; i < 3; ++i) {
                    ahead[i] = float(head.eye[i] - 50.0 * head.back[i]);
                    edge[i] = float(head.eye[i] - 50.0 * head.back[i] + 50.0 * tanHalf * head.right[i]);
                    above[i] = float(head.eye[i] - 50.0 * head.back[i] + 10.0 * head.up[i]);
                }
                float x0 = 9, y0 = 9, w0 = 0, x1 = 9, y1 = 9, w1 = 0, x2 = 9, y2 = 9, w2 = 0;
                const bool seen = made && markermath::Project(rebuilt, layout, ahead, x0, y0, w0) && markermath::Project(rebuilt, layout, edge, x1, y1, w1) &&
                                  markermath::Project(rebuilt, layout, above, x2, y2, w2);
                // The depth mapping kept: z / w of the point ahead is a + b / 50.
                float clip[4] = {};
                if (made)
                    for (int j = 0; j < 4; ++j) {
                        float v[3], t;
                        markermath::Component(rebuilt, layout, j, v, t);
                        clip[j] = markermath::Dot(v, ahead) + t;
                    }
                const double depth = clip[3] != 0 ? clip[2] / clip[3] : 9;
                const bool ok = seen && std::fabs(x0) < 1e-3f && std::fabs(y0) < 1e-3f && std::fabs(x1 - 1.0f) < 1e-3f && std::fabs(y1) < 1e-3f &&
                                (flipUp ? y2 < -0.1f : y2 > 0.1f) && std::fabs(depth - (a + b / 50.0)) < 1e-5 && markermath::Decode(rebuilt).layout == layout;
                // The game's matrix rebuilt for the game's own camera at the game's own angle is the game's matrix.
                float again[16];
                const float gameTanY = std::tan(0.5f * 55.0f * float(kPi) / 180.0f);
                const bool same = learned && markermath::Rebuild(d, s, gameRows, gameTanY * 2.0f, gameTanY, again);
                float worst = 0;
                for (int j = 0; j < 16; ++j)
                    worst = std::max(worst, std::fabs(again[j] - m[j]) / (std::fabs(m[j]) + 1.0f));
                all = all && ok && same && worst < 1e-4f;
                std::snprintf(detail + std::strlen(detail), sizeof(detail) - std::strlen(detail), "%s%s: middle %.4f %.4f, edge %.4f, above %.3f, again %.1e; ",
                              layoutIndex ? "column" : "row", flipUp ? " flipped" : "", x0, y0, x1, y2, worst);
            }
        Report("m4", "rebuilt for the head (the eyes' 105.88 degrees, square): its middle, its edge, up, the depth mapping and the layout kept; the game's own given back",
               all, "%s", detail);
    }
    // m5: the heading.
    {
        const double eye[3] = {0, 0, 0};
        const Cam c = MakeCam(0.6, 0.0, 0.0, eye);
        float rows[9];
        for (int i = 0; i < 3; ++i)
            rows[i] = float(c.right[i]), rows[3 + i] = float(c.up[i]), rows[6 + i] = float(c.back[i]);
        const float way0 = markermath::Heading(rows, 0), way1 = markermath::Heading(rows, 1);
        const float x = -rows[6], z = -rows[8];
        Report("m5", "the heading worked out both ways round (atan2(-x, -z), atan2(-z, -x)); the angle's wrap into (-pi, pi]",
               std::fabs(way0 - std::atan2(x, z)) < 1e-6f && std::fabs(way1 - std::atan2(z, x)) < 1e-6f &&
                   std::fabs(markermath::AngleDiff(3.1f, -3.1f) - (6.2f - 6.28318531f)) < 1e-5f && std::fabs(markermath::AngleDiff(0.2f, 0.1f) - 0.1f) < 1e-6f,
               "%.4f %.4f", way0, way1);
    }
}

// ---- s: the sky ----
std::vector<uint8_t> Compile(const char* source, const char* entry, const char* profile) {
    ID3DBlob* code = nullptr;
    ID3DBlob* errors = nullptr;
    std::vector<uint8_t> out;
    if (SUCCEEDED(D3DCompile(source, std::strlen(source), "test", nullptr, nullptr, entry, profile, 0, 0, &code, &errors)) && code)
        out.assign(static_cast<const uint8_t*>(code->GetBufferPointer()), static_cast<const uint8_t*>(code->GetBufferPointer()) + code->GetBufferSize());
    if (errors) {
        std::printf("  (compile: %s)\n", static_cast<const char*>(errors->GetBufferPointer()));
        errors->Release();
    }
    if (code)
        code->Release();
    return out;
}

// A sky-like pixel shader with one thing changed (none: the sky's own shape).
std::string SkySource(int change) {
    std::string s;
    s += change == 1 ? "float4 g_pad[20];\n" : "float4 g_pad[19];\n";  // 1: everything 16 bytes later
    s += "float4x4 g_invViewProjMatrix;\nfloat3 g_camPos;\nfloat2 g_screenSizeInv;\n";
    s += change == 2 ? "Texture2D g_otherTexture;\n" : "Texture2D g_skyGradientTexture;\n";
    s += "SamplerState g_skyGradientSampler;\n";
    const char* tex = change == 2 ? "g_otherTexture" : "g_skyGradientTexture";
    if (change == 3)
        s += "float4 PS(float4 pos : SV_Position, float3 ray : TEXCOORD0, float2 more : TEXCOORD1) : SV_Target {\n";
    else if (change == 4)
        s += "float4 PS(float4 pos : SV_Position, float3 ray : TEXCOORD1) : SV_Target {\n";
    else if (change == 5)
        s += "float4 PS(float4 pos : SV_Position, float2 ray2 : TEXCOORD0) : SV_Target { float3 ray = float3(ray2, 1);\n";
    else
        s += "float4 PS(float4 pos : SV_Position, float3 ray : TEXCOORD0) : SV_Target {\n";
    s += "    float3 d = normalize(ray);\n";
    s += "    float4 w = mul(float4(pos.xy * g_screenSizeInv, 1, 1), g_invViewProjMatrix);\n";
    s += std::string("    return ") + tex + ".Sample(g_skyGradientSampler, d.xy) + w + float4(g_camPos, 0) + g_pad[0]" + (change == 3 ? " + more.x" : "") + ";\n}\n";
    return s;
}

// A signature chunk's elements: name, index, register, mask.
struct SigElement {
    std::string name;
    uint32_t index, reg;
    uint8_t mask;
};
std::vector<SigElement> Signature(const std::vector<uint8_t>& code, const char* chunk) {
    std::vector<SigElement> out;
    const uint8_t* data = nullptr;
    size_t size = 0;
    if (!eyefx::detail::FindChunk(code.data(), code.size(), chunk, data, size))
        return out;
    uint32_t n = 0;
    eyefx::detail::Read32(data, size, 0, n);
    for (uint32_t i = 0; i < n && 8 + 24 * (i + 1) <= size; ++i) {
        uint32_t name = 0, index = 0, reg = 0;
        eyefx::detail::Read32(data, size, 8 + 24 * i, name);
        eyefx::detail::Read32(data, size, 8 + 24 * i + 4, index);
        eyefx::detail::Read32(data, size, 8 + 24 * i + 16, reg);
        out.push_back({name < size ? std::string(reinterpret_cast<const char*>(data + name)) : std::string(), index, reg, data[8 + 24 * i + 20]});
    }
    return out;
}

void SkyShaders() {
    const std::vector<uint8_t> sky5 = Compile(SkySource(0).c_str(), "PS", "ps_5_0"), sky4 = Compile(SkySource(0).c_str(), "PS", "ps_4_0");
    if (sky5.empty() || sky4.empty()) {
        Skip("s1", "the shader compiler isn't here");
        return;
    }
    bool missesRefused = true;
    char misses[200] = "";
    for (int change = 1; change <= 5; ++change) {
        const std::vector<uint8_t> c = Compile(SkySource(change).c_str(), "PS", "ps_5_0");
        const bool taken = !c.empty() && eyefx::IsSkyShader(c.data(), c.size());
        missesRefused = missesRefused && !c.empty() && !taken;
        std::snprintf(misses + std::strlen(misses), sizeof(misses) - std::strlen(misses), " %d:%s", change, c.empty() ? "no code" : taken ? "TAKEN" : "refused");
    }
    // Every truncation, a thousand copies with a byte broken: read without a fault (a fault would end the test here).
    int truncatedTaken = 0;
    for (size_t n = 0; n < sky5.size(); ++n)
        truncatedTaken += eyefx::IsSkyShader(sky5.data(), n);
    std::mt19937 rng(7);
    int brokenRead = 0;
    for (int n = 0; n < 1000; ++n) {
        std::vector<uint8_t> b = sky5;
        b[rng() % b.size()] = static_cast<uint8_t>(rng());
        b[rng() % b.size()] = static_cast<uint8_t>(rng());
        eyefx::IsSkyShader(b.data(), b.size());
        ++brokenRead;
    }
    const char junk[] = "DXBC this isn't a shader at all, nor near one";
    Report("s1", "the sky's shape taken (shader models 5 and 4); five near misses refused; every truncation refused; broken copies and junk read without a fault",
           eyefx::IsSkyShader(sky5.data(), sky5.size()) && eyefx::IsSkyShader(sky4.data(), sky4.size()) && missesRefused && truncatedTaken == 0 &&
               brokenRead == 1000 && !eyefx::IsSkyShader(junk, sizeof(junk)) && !eyefx::IsSkyShader(nullptr, 100),
           "misses:%s; truncations taken %d of %zu", misses, truncatedTaken, sky5.size());
}

void GameShaders(const std::string& folder) {
    const std::string dir = folder + "\\ps\\";
    WIN32_FIND_DATAA f{};
    HANDLE h = FindFirstFileA((dir + "*.dxbc").c_str(), &f);
    if (h == INVALID_HANDLE_VALUE) {
        Skip("s2", "no ps\\*.dxbc in the folder (the game's dumped shaders): not checked");
        return;
    }
    int files = 0, taken = 0;
    bool sky = false, distance = false, clear = false, tint = false;
    do {
        FILE* file = nullptr;
        if (fopen_s(&file, (dir + f.cFileName).c_str(), "rb") != 0 || !file)
            continue;
        std::vector<uint8_t> b;
        uint8_t chunk[65536];
        size_t n;
        while ((n = fread(chunk, 1, sizeof(chunk), file)) > 0)
            b.insert(b.end(), chunk, chunk + n);
        fclose(file);
        ++files;
        const bool is = eyefx::IsSkyShader(b.data(), b.size());
        taken += is;
        const std::string name = f.cFileName;
        if (name.rfind("78425db3e8d75475", 0) == 0)
            sky = is;
        if (name.rfind("94a5f6c1e17c947d", 0) == 0)
            distance = !is;
        if (name.rfind("2b2ca40a51aa1b4d", 0) == 0)
            clear = !is;
        if (name.rfind("5553fe3fbb6528bb", 0) == 0)
            tint = !is;
    } while (FindNextFileA(h, &f));
    FindClose(h);
    Report("s2", "the game's own shaders: the sky's taken (111 in the 4251 dumped by 2026-11-25), 78425db3e8d75475 among them; the depth of field's, the clear middle, the tint not",
           taken >= 100 && sky && distance && clear && tint, "%d of %d taken; the sky's %s; the others %s", taken, files, sky ? "taken" : "NOT taken",
           distance && clear && tint ? "refused" : "NOT all refused");
}

// Column-vector 4x4 (m[row][col]) helpers, double.
void Mul(const double a[4][4], const double b[4][4], double out[4][4]) {
    double r[4][4];
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) {
            r[i][j] = 0;
            for (int k = 0; k < 4; ++k)
                r[i][j] += a[i][k] * b[k][j];
        }
    std::memcpy(out, r, sizeof(r));
}
bool Invert(const double m[4][4], double out[4][4]) {
    double a[4][8];
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 8; ++j)
            a[i][j] = j < 4 ? m[i][j] : (j - 4 == i ? 1.0 : 0.0);
    for (int c = 0; c < 4; ++c) {
        int p = c;
        for (int r = c + 1; r < 4; ++r)
            if (std::fabs(a[r][c]) > std::fabs(a[p][c]))
                p = r;
        if (std::fabs(a[p][c]) < 1e-15)
            return false;
        for (int j = 0; j < 8; ++j)
            std::swap(a[c][j], a[p][j]);
        const double d = a[c][c];
        for (int j = 0; j < 8; ++j)
            a[c][j] /= d;
        for (int r = 0; r < 4; ++r)
            if (r != c) {
                const double f = a[r][c];
                for (int j = 0; j < 8; ++j)
                    a[r][j] -= f * a[c][j];
            }
    }
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            out[i][j] = a[i][j + 4];
    return true;
}

struct SkyCase {
    const char* name;
    double offX, offY;  // the window's move (shares of its width and height, to the right and up), as views.cpp's offcentre gives it
};

void SkyVertexShader() {
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &ctx))) {
        Skip("s3", "no software graphics card (WARP) here");
        return;
    }
    const std::vector<uint8_t> vsCode = Compile(eyefx::kSkyRaysVs, "VS", "vs_4_0");
    const char psSource[] = "float4 PS(float4 pos : SV_Position, float3 ray : TEXCOORD0) : SV_Target { return float4(ray, 1); }";
    const std::vector<uint8_t> psCode = Compile(psSource, "PS", "ps_4_0");
    const std::vector<SigElement> out = Signature(vsCode, "OSGN"), in = Signature(Compile(SkySource(0).c_str(), "PS", "ps_5_0"), "ISGN");
    const bool signature = out.size() == 2 && out[0].name == "SV_Position" && out[0].reg == 0 && out[1].name == "TEXCOORD" && out[1].index == 0 &&
                           out[1].reg == 1 && (out[1].mask & 7) == 7 && in.size() == 2 && in[1].name == "TEXCOORD" && in[1].reg == out[1].reg;
    Report("s3a", "the mod's sky vertex shader's outputs as the sky's shader reads them: SV_Position in register 0, the ray in TEXCOORD0's x, y, z in register 1",
           signature, "%zu outputs: %s r%u, %s%u r%u mask %x", out.size(), out.size() > 0 ? out[0].name.c_str() : "-", out.size() > 0 ? out[0].reg : 0,
           out.size() > 1 ? out[1].name.c_str() : "-", out.size() > 1 ? out[1].index : 0, out.size() > 1 ? out[1].reg : 0, out.size() > 1 ? out[1].mask : 0);
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    if (vsCode.empty() || psCode.empty() || FAILED(device->CreateVertexShader(vsCode.data(), vsCode.size(), nullptr, &vs)) ||
        FAILED(device->CreatePixelShader(psCode.data(), psCode.size(), nullptr, &ps))) {
        Report("s3b", "the mod's sky vertex shader made", false, "not made");
        return;
    }
    constexpr UINT kSize = 64;
    D3D11_TEXTURE2D_DESC td{};
    td.Width = td.Height = kSize;
    td.MipLevels = td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    td.SampleDesc.Count = 1;
    td.BindFlags = D3D11_BIND_RENDER_TARGET;
    ID3D11Texture2D* target = nullptr;
    ID3D11RenderTargetView* rtv = nullptr;
    D3D11_TEXTURE2D_DESC sd = td;
    sd.BindFlags = 0;
    sd.Usage = D3D11_USAGE_STAGING;
    sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Texture2D* staging = nullptr;
    device->CreateTexture2D(&td, nullptr, &target);
    device->CreateRenderTargetView(target, nullptr, &rtv);
    device->CreateTexture2D(&sd, nullptr, &staging);
    D3D11_BUFFER_DESC bd{};
    bd.ByteWidth = 25 * 16;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    ID3D11Buffer* constants = nullptr;
    device->CreateBuffer(&bd, nullptr, &constants);
    D3D11_BUFFER_DESC ed = bd;
    ed.ByteWidth = 16;
    ID3D11Buffer* eyeBuffer = nullptr;
    device->CreateBuffer(&ed, nullptr, &eyeBuffer);
    const SkyCase cases[] = {{"centred", 0.0, 0.0}, {"off-centre (0.2 out, 0.15 down)", -0.2, -0.15}};
    char detail[600] = "";
    bool all = true;
    double bend[2] = {}, twinWorst = 0;
    for (int ci = 0; ci < 2; ++ci) {
        const SkyCase& sc = cases[ci];
        // The eye: 105.88 degrees, square, the game's 6 cm near plane, 10 km far, its window moved as views.cpp's offcentre moves it (left = -w + 2 w x off.x ...).
        const double camPos[3] = {1520.5, 42.25, -2875.75};
        const Cam c = MakeCam(0.83, 0.21, 0.04, camPos);
        const double n = 0.06, f = 10000.0, h = n * std::tan(0.5 * kOwnersFov), w = h;
        const double l = -w + 2 * w * sc.offX, r = l + 2 * w, t = h + 2 * h * sc.offY, b = t - 2 * h;
        // The world to the view (column vectors): rows right, up, back, and the eye.
        double view[4][4] = {};
        for (int i = 0; i < 3; ++i) {
            view[0][i] = c.right[i];
            view[1][i] = c.up[i];
            view[2][i] = c.back[i];
        }
        for (int i = 0; i < 3; ++i)
            view[i][3] = -(view[i][0] * camPos[0] + view[i][1] * camPos[1] + view[i][2] * camPos[2]);
        view[3][3] = 1;
        // D3D's off-centre right-handed projection, depth 0 at the near plane, 1 at the far.
        double proj[4][4] = {};
        proj[0][0] = 2 * n / (r - l);
        proj[0][2] = (r + l) / (r - l);
        proj[1][1] = 2 * n / (t - b);
        proj[1][2] = (t + b) / (t - b);
        proj[2][2] = f / (n - f);
        proj[2][3] = n * f / (n - f);
        proj[3][2] = -1;
        double vp[4][4], inv[4][4];
        Mul(proj, view, vp);
        Invert(vp, inv);
        float data[25][4] = {};
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                data[19 + i][j] = float(inv[i][j]);  // its rows as the game's shader dots them
        for (int i = 0; i < 3; ++i)
            data[23][i] = float(camPos[i]);
        ctx->UpdateSubresource(constants, 0, nullptr, data, 0, 0);
        // The eye's numbers as skyrays.cpp writes them: the half angle's tangent top to bottom, the window's move to the right and up.
        const float eyeNumbers[4] = {std::tan(0.5f * kOwnersFov), float(sc.offX), float(sc.offY), 0.0f};
        ctx->UpdateSubresource(eyeBuffer, 0, nullptr, eyeNumbers, 0, 0);
        ID3D11Buffer* both[2] = {constants, eyeBuffer};
        // The CPU's twin of the shader (eyefx.h SkyRay) on the same numbers, at the picture's middle, against the middle's own line.
        float rows[4][4], cam[3] = {float(camPos[0]), float(camPos[1]), float(camPos[2])}, twin[3];
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                rows[i][j] = data[19 + i][j];
        eyefx::SkyRay(rows, cam, eyeNumbers, 0.0f, 0.0f, twin);
        float middleLine[3];
        for (int i = 0; i < 3; ++i)
            middleLine[i] = float(0.5 * (l + r) * c.right[i] + 0.5 * (t + b) * c.up[i] - n * c.back[i]);
        twinWorst = std::max(twinWorst, double(markermath::AngleDeg(twin, middleLine)));
        const D3D11_VIEWPORT v{0, 0, float(kSize), float(kSize), 0, 1};
        ctx->RSSetViewports(1, &v);
        ctx->RSSetState(nullptr);  // the default culls the back: the corners turn as the front (clockwise on the screen)
        ctx->OMSetRenderTargets(1, &rtv, nullptr);
        const float clear[4] = {0, 0, 0, 0};
        ctx->ClearRenderTargetView(rtv, clear);
        ctx->IASetInputLayout(nullptr);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader(vs, nullptr, 0);
        ctx->VSSetConstantBuffers(0, 2, both);
        ctx->PSSetShader(ps, nullptr, 0);
        ctx->Draw(6, 0);
        ctx->CopyResource(staging, target);
        D3D11_MAPPED_SUBRESOURCE m{};
        double worst = 0;
        int covered = 0;
        if (SUCCEEDED(ctx->Map(staging, 0, D3D11_MAP_READ, 0, &m))) {
            for (UINT y = 0; y < kSize; ++y)
                for (UINT x = 0; x < kSize; ++x) {
                    const float* p = reinterpret_cast<const float*>(static_cast<const uint8_t*>(m.pData) + y * m.RowPitch) + 4 * x;
                    if (p[3] != 1.0f)
                        continue;
                    ++covered;
                    // The pixel's own line: through its centre on the near plane, in the world.
                    const double sx = (x + 0.5) / kSize, sy = (y + 0.5) / kSize;
                    const double X = l + sx * (r - l), Y = t - sy * (t - b);
                    double d[3];
                    for (int i = 0; i < 3; ++i)
                        d[i] = X * c.right[i] + Y * c.up[i] - n * c.back[i];
                    const float dir[3] = {float(d[0]), float(d[1]), float(d[2])};
                    worst = std::max(worst, double(markermath::AngleDeg(p, dir)));
                }
            ctx->Unmap(staging, 0);
        }
        // The corners normalised one by one and the picture's middle between them (the two triangles meet on the diagonal: the middle is the mean of the
        // two corners on it): how far that is from the middle's own line.
        double corner[4][3];
        const double cx[4] = {l, r, l, r}, cy[4] = {t, t, b, b};
        for (int k = 0; k < 4; ++k) {
            double len = 0;
            for (int i = 0; i < 3; ++i) {
                corner[k][i] = cx[k] * c.right[i] + cy[k] * c.up[i] - n * c.back[i];
                len += corner[k][i] * corner[k][i];
            }
            for (int i = 0; i < 3; ++i)
                corner[k][i] /= std::sqrt(len);
        }
        float mid[3], own[3];
        for (int i = 0; i < 3; ++i) {
            mid[i] = float(0.5 * (corner[1][i] + corner[2][i]));
            own[i] = float(0.5 * (l + r) * c.right[i] + 0.5 * (t + b) * c.up[i] - n * c.back[i]);
        }
        bend[ci] = markermath::AngleDeg(mid, own);
        all = all && covered == int(kSize * kSize) && worst < 0.005;
        std::snprintf(detail + std::strlen(detail), sizeof(detail) - std::strlen(detail), "%s: %d of %u pixels, worst %.5f deg, corners normalised %.4f deg off at the middle; ",
                      sc.name, covered, kSize * kSize, worst, bend[ci]);
    }
    Report("s3b", "every pixel's ray along its own line to 0.005 degrees, centred and off-centre, the camera 3 km from the world's middle (the game's constants as its own shader reads them)",
           all && twinWorst < 0.005, "%sthe CPU's twin at the middle %.5f deg", detail, twinWorst);
    Report("s3c", "why: corners normalised one by one bend the middle off-centre (more than 0.05 degrees here) and not centred", bend[0] < 1e-3 && bend[1] > 0.05,
           "centred %.5f deg, off-centre %.4f deg", bend[0], bend[1]);
    for (IUnknown* u : std::initializer_list<IUnknown*>{eyeBuffer, constants, staging, rtv, target, ps, vs, ctx, device})
        if (u)
            u->Release();
}

// ---- c: the code the hooks need ----
void GameCode(const std::string& folder) {
    FILE* f = nullptr;
    std::vector<uint8_t> image;
    if (fopen_s(&f, (folder + "\\bf3_mem.exe").c_str(), "rb") == 0 && f) {
        uint8_t chunk[1 << 16];
        size_t n;
        while ((n = fread(chunk, 1, sizeof(chunk), f)) > 0)
            image.insert(image.end(), chunk, chunk + n);
        fclose(f);
    }
    if (image.size() < 0x2000000) {
        Skip("c1", "no bf3_mem.exe in the folder (bf3.exe 1.6.0.0 decrypted): the hooked code not checked");
        return;
    }
    const auto at = [&](unsigned va, const uint8_t* bytes, size_t n) {
        const size_t o = va - 0x400000u;
        return o + n <= image.size() && std::memcmp(&image[o], bytes, n) == 0;
    };
    namespace code = markermath::code;
    const uint8_t retIcons[] = {0xC2, 0x0C, 0x00}, ret[] = {0xC3}, camera[] = {0x8B, 0x0D, 0xDC, 0xAA, 0x38, 0x02}, fchs[] = {0xD9, 0xE0},
                  atan[] = {0xE8};  // (a call: to 0x0127DE44, the arctangent's import)
    const bool icons = at(code::kIconUpdate, code::kIconUpdateStart, sizeof(code::kIconUpdateStart)) && at(code::kIconUpdateRet, retIcons, 3);
    const bool heading = at(code::kHeading, code::kHeadingStart, sizeof(code::kHeadingStart)) && at(code::kHeadingRet, ret, 1) && at(0x0093C248, camera, 6) &&
                         at(0x0093C25C, fchs, 2) && at(0x0093C262, fchs, 2) && at(0x0093C264, atan, 1);
    // The call's target: 0x0093C264 + 5 + rel32 = 0x0127DE44.
    int32_t rel = 0;
    std::memcpy(&rel, &image[0x0093C265 - 0x400000], 4);
    const unsigned target = 0x0093C264u + 5u + static_cast<unsigned>(rel);
    Report("c1", "the icons' update (first bytes, RET 0Ch) and the heading (first bytes, RET, the game camera [0x0238AADC], two negations, the arctangent's call) as read",
           icons && heading && target == 0x0127DE44u, "icons %s, heading %s, its call to %08X", icons ? "as read" : "NOT as read", heading ? "as read" : "NOT as read",
           target);
}

}  // namespace

int main(int argc, char** argv) {
    SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
    const auto start = std::chrono::steady_clock::now();
    const std::string folder = argc > 1 ? argv[1] : "build";
    Hud();
    Markers();
    SkyShaders();
    GameShaders(folder);
    SkyVertexShader();
    GameCode(folder);
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    Report("t", "the time", ms < 60000.0, "%.0f ms", ms);
    std::printf("%d checks, %d failed, %d skipped\n", g_counted, g_failed, g_skipped);
    return g_failed;
}

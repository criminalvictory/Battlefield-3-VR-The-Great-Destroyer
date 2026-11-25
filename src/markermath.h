#pragma once
// The world markers' view-projection (header only, as markers.cpp runs it; tools/eyefxtest.cpp tests it without the game).
//
// The game places every 3D icon of the HUD (squadmates' names, objectives, a pickup's tier icon) from one 4x4 matrix handed to the icons' per-frame update
// (0x0094AB90, bf3.exe 1.6.0.0: its first argument, copied and used for every icon). Most likely the game camera's: in VR the gun's aim at the game's own
// angle, so a marker sits on its target only while the gun points where the head looks. Which camera it is, and in which layout, is read here from the
// matrix itself, without assuming the game's conventions:
//
//   a camera: right r, up u, ahead l (unit, square to each other), its eye e; a world point p seen at x = (p-e).r, y = (p-e).u, d = (p-e).l;
//   a projection: clip = k (sx x + cx d, sy y + cy d, a d + b, d)   (cx, cy: a window off its middle; a, b: the depth's mapping; k: an overall scale)
//   a layout: clip = [p 1] M with M's rows in memory (kRowVector: component j's vector down column j, its constant in row 3), or clip = M [p 1]
//             (kColumnVector: along row j, its constant last in the row).
//
// Decode takes a matrix apart into those and puts it back together: a matrix it does not give back to a part in ten thousand is not one of these (an
// orthographic one, a broken one) and is never rebuilt. Rebuild makes the same kind of matrix, in the same layout, with the same depth mapping, for another
// camera (the head's: the eyes' midpoint, their turn) and another angle (the eyes' own), the axes signed as the game's were found to be (Learn).

#include <cmath>
#include <cstring>

namespace markermath {

enum Layout { kRowVector, kColumnVector };

struct Camera {
    float r[3], u[3], l[3];  // right, up, ahead (unit)
    float e[3];              // the eye
};
struct Projection {
    float sx, sy, cx, cy, a, b, k;
};
struct Decoded {
    bool ok = false;
    Layout layout = kRowVector;
    Camera cam{};
    Projection proj{};
    float residual = 1e9f;  // the largest difference put back together, against the matrix's own size
    float square = 1e9f;    // the largest |dot| between two of the axes
    float handed = 0.0f;    // (r x u) . l: +1 or -1
};

inline float Dot(const float* a, const float* b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
inline float Length(const float* a) { return std::sqrt(Dot(a, a)); }
inline void Cross(const float* a, const float* b, float* out) {
    const float c[3] = {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
    std::memcpy(out, c, sizeof(c));
}
// The angle between two directions, degrees (0 when either has no length): from the cross and dot products, as fine near 0 as near 90 (an arccosine of a
// float near 1 can't say less than 0.02 degrees).
inline float AngleDeg(const float* a, const float* b) {
    const double ax = a[0], ay = a[1], az = a[2], bx = b[0], by = b[1], bz = b[2];
    const double cx = ay * bz - az * by, cy = az * bx - ax * bz, cz = ax * by - ay * bx;
    const double sine = std::sqrt(cx * cx + cy * cy + cz * cz), cosine = ax * bx + ay * by + az * bz;
    if (!(sine > 0.0) && !(cosine != 0.0))
        return 0.0f;
    return static_cast<float>(std::atan2(sine, cosine) * 57.29577951308232);
}

// Component j's vector (3) and its constant, in a layout.
inline void Component(const float* m, Layout layout, int j, float* v, float& t) {
    for (int i = 0; i < 3; ++i)
        v[i] = layout == kRowVector ? m[i * 4 + j] : m[j * 4 + i];
    t = layout == kRowVector ? m[12 + j] : m[j * 4 + 3];
}
inline void SetComponent(float* m, Layout layout, int j, const float* v, float t) {
    for (int i = 0; i < 3; ++i)
        (layout == kRowVector ? m[i * 4 + j] : m[j * 4 + i]) = v[i];
    (layout == kRowVector ? m[12 + j] : m[j * 4 + 3]) = t;
}

inline void Encode(const Camera& c, const Projection& p, Layout layout, float* m) {
    float v[4][3];
    for (int i = 0; i < 3; ++i) {
        v[0][i] = p.k * (p.sx * c.r[i] + p.cx * c.l[i]);
        v[1][i] = p.k * (p.sy * c.u[i] + p.cy * c.l[i]);
        v[2][i] = p.k * p.a * c.l[i];
        v[3][i] = p.k * c.l[i];
    }
    const float el = Dot(c.e, c.l);
    SetComponent(m, layout, 0, v[0], -Dot(v[0], c.e));
    SetComponent(m, layout, 1, v[1], -Dot(v[1], c.e));
    SetComponent(m, layout, 2, v[2], -p.k * p.a * el + p.k * p.b);
    SetComponent(m, layout, 3, v[3], -p.k * el);
}

// One layout's reading of a matrix.
inline Decoded DecodeAs(const float* m, Layout layout) {
    Decoded d;
    d.layout = layout;
    float v[4][3], t[4];
    for (int j = 0; j < 4; ++j)
        Component(m, layout, j, v[j], t[j]);
    for (int j = 0; j < 16; ++j)
        if (!std::isfinite(m[j]))
            return d;
    const float k = Length(v[3]);
    if (!(k > 1e-6f))
        return d;
    Camera& c = d.cam;
    Projection& p = d.proj;
    p.k = k;
    for (int i = 0; i < 3; ++i)
        c.l[i] = v[3][i] / k;
    float w[3];
    for (int i = 0; i < 3; ++i)
        w[i] = v[0][i] / k;
    p.cx = Dot(w, c.l);
    for (int i = 0; i < 3; ++i)
        w[i] -= p.cx * c.l[i];
    p.sx = Length(w);
    if (!(p.sx > 1e-6f))
        return d;
    for (int i = 0; i < 3; ++i)
        c.r[i] = w[i] / p.sx;
    for (int i = 0; i < 3; ++i)
        w[i] = v[1][i] / k;
    p.cy = Dot(w, c.l);
    for (int i = 0; i < 3; ++i)
        w[i] -= p.cy * c.l[i];
    p.sy = Length(w);
    if (!(p.sy > 1e-6f))
        return d;
    for (int i = 0; i < 3; ++i)
        c.u[i] = w[i] / p.sy;
    for (int i = 0; i < 3; ++i)
        w[i] = v[2][i] / k;
    p.a = Dot(w, c.l);
    const float el = -t[3] / k;
    const float er = (-t[0] / k - p.cx * el) / p.sx;
    const float eu = (-t[1] / k - p.cy * el) / p.sy;
    for (int i = 0; i < 3; ++i)
        c.e[i] = er * c.r[i] + eu * c.u[i] + el * c.l[i];
    p.b = t[2] / k + p.a * el;
    d.square = std::fmax(std::fabs(Dot(c.r, c.u)), std::fmax(std::fabs(Dot(c.r, c.l)), std::fabs(Dot(c.u, c.l))));
    float ru[3];
    Cross(c.r, c.u, ru);
    d.handed = Dot(ru, c.l);
    float back[16];
    Encode(c, p, layout, back);
    float size = 0.0f;
    for (int j = 0; j < 16; ++j)
        size = std::fmax(size, std::fabs(m[j]));
    float worst = 0.0f;
    for (int j = 0; j < 16; ++j)
        worst = std::fmax(worst, std::fabs(back[j] - m[j]) / (std::fabs(m[j]) + 1e-3f * size + 1e-12f));
    d.residual = worst;
    // A picture's angle within reason too (about 6 to 175 degrees): an orthographic matrix with a constant in its last row reads as a "camera" the other way
    // round, with an angle of nearly 180 degrees.
    const bool angle = p.sx >= 0.05f && p.sx <= 20.0f && p.sy >= 0.05f && p.sy <= 20.0f;
    d.ok = d.residual < 1e-4f && d.square < 1e-3f && std::fabs(std::fabs(d.handed) - 1.0f) < 1e-3f && angle;
    return d;
}

// The layout that reads the matrix as a camera and a projection; of two that both do (a camera at the world's origin), the one with the smaller residual.
inline Decoded Decode(const float* m) {
    const Decoded a = DecodeAs(m, kRowVector), b = DecodeAs(m, kColumnVector);
    if (a.ok && (!b.ok || a.residual <= b.residual))
        return a;
    if (b.ok)
        return b;
    return a.residual <= b.residual ? a : b;
}

// A camera as the game's rows have it: right, up, back (it looks along -back), its eye.
struct Rows {
    float right[3], up[3], back[3], eye[3];
};
// The decoded axes against a camera's rows: +1 or -1 each (r = sr right, u = su up, l = sl (-back)) when all three lie within maxDeg of one of the camera's
// own (either way); false otherwise.
struct Signs {
    float r = 1.0f, u = 1.0f, l = 1.0f;
};
inline bool Learn(const Camera& c, const Rows& rows, float maxDeg, Signs& out, float* worstDeg = nullptr) {
    const float ahead[3] = {-rows.back[0], -rows.back[1], -rows.back[2]};
    const float* own[3] = {rows.right, rows.up, ahead};
    const float* got[3] = {c.r, c.u, c.l};
    float s[3], worst = 0.0f;
    for (int i = 0; i < 3; ++i) {
        const float dot = Dot(got[i], own[i]) / (Length(own[i]) + 1e-12f);
        s[i] = dot >= 0.0f ? 1.0f : -1.0f;
        const float negated[3] = {-own[i][0], -own[i][1], -own[i][2]};
        worst = std::fmax(worst, AngleDeg(got[i], s[i] > 0.0f ? own[i] : negated));
    }
    if (worstDeg)
        *worstDeg = worst;
    if (!(worst <= maxDeg))
        return false;
    out.r = s[0], out.u = s[1], out.l = s[2];
    return true;
}

// The same kind of matrix (the decoded layout, scale and depth mapping) for another camera's rows and eye, seen through a centred picture with these half
// angles' tangents across and up. False if the rows aren't a camera (not unit and square within a part in a thousand) or the tangents are out of reason.
inline bool Rebuild(const Decoded& from, const Signs& signs, const Rows& rows, float tanHalfX, float tanHalfY, float* out) {
    if (!from.ok || !(tanHalfX > 0.05f && tanHalfX < 20.0f) || !(tanHalfY > 0.05f && tanHalfY < 20.0f))
        return false;
    Camera c{};
    for (int i = 0; i < 3; ++i) {
        c.r[i] = signs.r * rows.right[i];
        c.u[i] = signs.u * rows.up[i];
        c.l[i] = -signs.l * rows.back[i];
        c.e[i] = rows.eye[i];
    }
    for (const float* a : {c.r, c.u, c.l})
        if (std::fabs(Length(a) - 1.0f) > 1e-3f)
            return false;
    if (std::fabs(Dot(c.r, c.u)) > 1e-3f || std::fabs(Dot(c.r, c.l)) > 1e-3f || std::fabs(Dot(c.u, c.l)) > 1e-3f)
        return false;
    for (float v : c.e)
        if (!std::isfinite(v))
            return false;
    Projection p = from.proj;
    p.sx = 1.0f / tanHalfX;
    p.sy = 1.0f / tanHalfY;
    p.cx = 0.0f;
    p.cy = 0.0f;
    Encode(c, p, from.layout, out);
    return true;
}

// The game's heading (0x0093C230) as worked out from a camera's rows (right, up, back): atan2 of its back row's x and z, both negated, in one order or the
// other (which the game's x87 helper takes first is learned by markers.cpp against what the game returns: way 0 atan2(-x, -z), way 1 atan2(-z, -x)).
inline float Heading(const float* rows, int way) {
    const float x = -rows[6], z = -rows[8];
    return way == 0 ? std::atan2(x, z) : std::atan2(z, x);
}
// The difference of two angles, radians, into (-pi, pi].
inline float AngleDiff(float a, float b) {
    float d = std::fmod(a - b, 6.28318531f);
    if (d > 3.14159265f)
        d -= 6.28318531f;
    if (d <= -3.14159265f)
        d += 6.28318531f;
    return d;
}

// The code markers.cpp hooks, as read in bf3.exe 1.6.0.0 (build\bf3_mem.exe; tools/eyefxtest.cpp checks them there): each hooked only where its first bytes
// are these.
namespace code {
// The 3D icons' per-frame update: push ebp; mov ebp,esp; and esp,-16; sub esp,0C4h; push ebx; mov ebx,ecx. __thiscall, three arguments (RET 0Ch at
// 0x0094AD36): the view-projection (copied with rep movs at 0x0094AC25, its pointer handed on to each icon's placing at 0x0094AC92), and two more.
constexpr unsigned kIconUpdate = 0x0094AB90;
constexpr unsigned char kIconUpdateStart[] = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0, 0x81, 0xEC, 0xC4, 0x00, 0x00, 0x00, 0x53, 0x8B, 0xD9};
constexpr unsigned kIconUpdateRet = 0x0094AD36;  // C2 0C 00
// The heading: push ebp; mov ebp,esp; and esp,-16; sub esp,50h; call 0x00938E30 (true: 0 returned, fldz). No arguments, its answer in st0 (plain RET at
// 0x0093C274); the game camera [0x0238AADC] through 0x01098B20 (its world transform), the back row's x and z negated (fchs at 0x0093C25C, 0x0093C262) into
// the arctangent's import (0x0127DE44).
constexpr unsigned kHeading = 0x0093C230;
constexpr unsigned char kHeadingStart[] = {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0, 0x83, 0xEC, 0x50, 0xE8};
constexpr unsigned kHeadingRet = 0x0093C274;  // C3
}  // namespace code

// Where a world point comes out through a matrix (x, y from -1 to 1 across the picture, and its w): false behind the eye.
inline bool Project(const float* m, Layout layout, const float* p, float& x, float& y, float& w) {
    float clip[4];
    for (int j = 0; j < 4; ++j) {
        float v[3], t;
        Component(m, layout, j, v, t);
        clip[j] = Dot(v, p) + t;
    }
    w = clip[3];
    if (!(w > 1e-6f))
        return false;
    x = clip[0] / w;
    y = clip[1] / w;
    return true;
}

}  // namespace markermath

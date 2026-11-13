// The head replay's motions and its meters' math (headtest.h): no game, no OpenXR, so tools/headtesttest.cpp runs all of it.

#include "headtest.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace headtest {

namespace {

constexpr double kDeg = kPi / 180.0;

const char* Skip(const char* s) {
    while (*s == ' ' || *s == '\t')
        ++s;
    return s;
}

// The rest of the line after its key, or null if the line isn't that key (a key is a whole word).
const char* After(const char* line, const char* key) {
    const size_t n = std::strlen(key);
    if (std::strncmp(line, key, n) != 0)
        return nullptr;
    const char c = line[n];
    return c == '\0' || c == ' ' || c == '\t' ? Skip(line + n) : nullptr;
}

double Kept(double v, double lo, double hi, double otherwise) {
    return std::isfinite(v) ? std::min(std::max(v, lo), hi) : otherwise;
}

// A turn about one axis (0 x, 1 y, 2 z).
Turn About(int axis, double degrees) {
    const double half = degrees * kDeg / 2.0;
    Turn q{0.0, 0.0, 0.0, std::cos(half)};
    (axis == 0 ? q.x : axis == 1 ? q.y : q.z) = std::sin(half);
    return q;
}

Turn Unit(const Turn& q) {
    const double n = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    return n > 0.0 ? Turn{q.x / n, q.y / n, q.z / n, q.w / n} : Turn{};
}

// From a to b the short way, that share of the way.
Turn Part(const Turn& a, Turn b, double share) {
    double dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    if (dot < 0.0) {
        b = {-b.x, -b.y, -b.z, -b.w};
        dot = -dot;
    }
    double wa = 1.0 - share, wb = share;
    if (dot < 0.9999995) {
        const double angle = std::acos(dot), s = std::sin(angle);
        wa = std::sin((1.0 - share) * angle) / s;
        wb = std::sin(share * angle) / s;
    }
    return Unit({a.x * wa + b.x * wb, a.y * wa + b.y * wb, a.z * wa + b.z * wb, a.w * wa + b.w * wb});
}

// The noise: the same numbers for the same frame, run after run.
uint32_t Mix(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

// About a normal number (mean 0, spread 1): four even ones summed.
double Normal(uint32_t& n) {
    double sum = 0.0;
    for (int i = 0; i < 4; ++i)
        sum += Mix(++n) / 4294967296.0;
    return (sum - 2.0) * 1.7320508075688772;
}

double Mixed(double a, double b, double share) {
    return a + (b - a) * share;
}

void Trimmed(const char* from, std::string& out) {
    out = Skip(from);
    while (!out.empty() && (out.back() == ' ' || out.back() == '\t' || out.back() == '\r' || out.back() == '\n'))
        out.pop_back();
    if (out.size() >= 2 && out.front() == '"' && out.back() == '"')
        out = out.substr(1, out.size() - 2);
}

}  // namespace

Turn Mul(const Turn& a, const Turn& b) {
    return {a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y, a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x, a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
            a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}

Turn Inverse(const Turn& q) {
    return {-q.x, -q.y, -q.z, q.w};
}

void Rotate(const Turn& q, const double* v, double* out) {
    const double t[3] = {2.0 * (q.y * v[2] - q.z * v[1]), 2.0 * (q.z * v[0] - q.x * v[2]), 2.0 * (q.x * v[1] - q.y * v[0])};
    const double r[3] = {v[0] + q.w * t[0] + (q.y * t[2] - q.z * t[1]), v[1] + q.w * t[1] + (q.z * t[0] - q.x * t[2]), v[2] + q.w * t[2] + (q.x * t[1] - q.y * t[0])};
    out[0] = r[0], out[1] = r[1], out[2] = r[2];
}

void ToVector(const Turn& q, double* v) {
    const double sign = q.w < 0.0 ? -1.0 : 1.0;
    const double s = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z);
    const double k = s > 1e-12 ? 2.0 * std::atan2(s, sign * q.w) / s : 2.0;
    v[0] = sign * q.x * k, v[1] = sign * q.y * k, v[2] = sign * q.z * k;
}

Turn FromVector(const double* v) {
    const double angle = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (angle < 1e-12)
        return Unit({v[0] / 2.0, v[1] / 2.0, v[2] / 2.0, 1.0});
    const double s = std::sin(angle / 2.0) / angle;
    return {v[0] * s, v[1] * s, v[2] * s, std::cos(angle / 2.0)};
}

Turn FromAngles(const Angles& a) {
    return Unit(Mul(Mul(About(1, a.yaw), About(0, a.pitch)), About(2, a.roll)));
}

namespace {

// From the three axes (each the head's own axis as the room has it): the right's x, y and z, the up's y, the back's x, y and z.
Angles AxesAngles(double rightX, double rightY, double rightZ, double upY, double backX, double backY, double backZ) {
    Angles a;
    a.pitch = std::asin(std::min(std::max(-backY, -1.0), 1.0)) / kDeg;
    if (backX * backX + backZ * backZ > 1e-12) {
        a.yaw = std::atan2(backX, backZ) / kDeg;
        a.roll = std::atan2(rightY, upY) / kDeg;
    } else {
        // Straight up or down: the yaw and the roll are one turn; all of it the yaw's.
        a.yaw = std::atan2(-rightZ, rightX) / kDeg;
        a.roll = 0.0;
    }
    return a;
}

}  // namespace

Angles ToAngles(const Turn& q) {
    const double x = q.x, y = q.y, z = q.z, w = q.w;
    return AxesAngles(1.0 - 2.0 * (y * y + z * z), 2.0 * (x * y + w * z), 2.0 * (x * z - w * y), 1.0 - 2.0 * (x * x + z * z), 2.0 * (x * z + w * y), 2.0 * (y * z - w * x),
                      1.0 - 2.0 * (x * x + y * y));
}

Angles RowsAngles(const float* rows) {
    return AxesAngles(rows[0], rows[1], rows[2], rows[4], rows[6], rows[7], rows[8]);
}

Turn RowsTurn(const float* rows) {
    // The rows are the head's axes: the columns of its turn's matrix.
    const double m00 = rows[0], m10 = rows[1], m20 = rows[2], m01 = rows[3], m11 = rows[4], m21 = rows[5], m02 = rows[6], m12 = rows[7], m22 = rows[8];
    const double trace = m00 + m11 + m22;
    Turn q;
    if (trace > 0.0) {
        const double s = std::sqrt(trace + 1.0) * 2.0;
        q = {(m21 - m12) / s, (m02 - m20) / s, (m10 - m01) / s, 0.25 * s};
    } else if (m00 > m11 && m00 > m22) {
        const double s = std::sqrt(1.0 + m00 - m11 - m22) * 2.0;
        q = {0.25 * s, (m01 + m10) / s, (m02 + m20) / s, (m21 - m12) / s};
    } else if (m11 > m22) {
        const double s = std::sqrt(1.0 + m11 - m00 - m22) * 2.0;
        q = {(m01 + m10) / s, 0.25 * s, (m12 + m21) / s, (m02 - m20) / s};
    } else {
        const double s = std::sqrt(1.0 + m22 - m00 - m11) * 2.0;
        q = {(m02 + m20) / s, (m12 + m21) / s, 0.25 * s, (m10 - m01) / s};
    }
    return Unit(q);
}

double Between(const Turn& a, const Turn& b) {
    const double dot = std::fabs(a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w);
    const Turn d = Mul(Inverse(a), b);
    // (By the sine and the cosine both: the cosine alone can't tell a hundredth of a degree from none.)
    return 2.0 * std::atan2(std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z), std::min(dot, 1.0)) / kDeg;
}

Angles Slide(const Turn& picture, const Turn& head) {
    return ToAngles(Mul(Inverse(picture), head));
}

Config Parse(const char* text) {
    Config c;
    if (!text)
        return c;
    for (const char* p = text; *p;) {
        char line[512];
        size_t n = 0;
        while (*p && *p != '\n') {
            if (n + 1 < sizeof(line) && *p != '\r')
                line[n++] = *p;
            ++p;
        }
        if (*p == '\n')
            ++p;
        line[n] = '\0';
        const char* s = Skip(line);
        if (!*s || *s == '#')
            continue;
        const char* rest = nullptr;
        double a = 0.0, b = 0.0, d = 0.0;
        char way[16] = {};
        if (After(s, "off"))
            return Config{};  // whatever else the file says
        if ((rest = After(s, "shake"))) {
            const int got = sscanf_s(rest, "%15s %lf %lf %lf", way, static_cast<unsigned>(sizeof(way)), &a, &b, &d);
            const bool yaw = std::strcmp(way, "yaw") == 0, pitch = std::strcmp(way, "pitch") == 0, both = std::strcmp(way, "both") == 0;
            if ((got >= 3 && (yaw || pitch)) || (got >= 4 && both)) {
                c.kind = Kind::kShake;
                c.yaw = yaw || both ? Kept(a, -180.0, 180.0, 0.0) : 0.0;
                c.pitch = pitch ? Kept(a, -85.0, 85.0, 0.0) : both ? Kept(b, -85.0, 85.0, 0.0) : 0.0;
                c.hz = Kept(both ? d : b, 0.01, 20.0, 1.0);
            }
        } else if ((rest = After(s, "burst"))) {
            const int got = sscanf_s(rest, "%15s %lf %lf %lf", way, static_cast<unsigned>(sizeof(way)), &a, &b, &d);
            const bool yaw = std::strcmp(way, "yaw") == 0, pitch = std::strcmp(way, "pitch") == 0;
            if (got >= 3 && (yaw || pitch)) {
                c.kind = Kind::kBurst;
                c.burstPitch = pitch;
                c.burst = Kept(a, pitch ? -85.0 : -180.0, pitch ? 85.0 : 180.0, 0.0);
                c.swingMs = Kept(b, 10.0, 5000.0, 150.0);
                c.pauseMs = got >= 4 ? Kept(d, 0.0, 10000.0, 300.0) : 300.0;
            }
        } else if ((rest = After(s, "track"))) {
            Trimmed(rest, c.trackFile);
            if (!c.trackFile.empty())
                c.kind = Kind::kTrack;
        } else if ((rest = After(s, "located"))) {
            Trimmed(rest, c.locatedFile);
        } else if ((rest = After(s, "neck"))) {
            if (sscanf_s(rest, "%lf %lf", &a, &b) == 2) {
                c.neckForward = Kept(a, -0.5, 0.5, 0.10);
                c.neckUp = Kept(b, -0.5, 0.5, 0.08);
            }
        } else if ((rest = After(s, "predict"))) {
            if (sscanf_s(rest, "%lf", &a) == 1)
                c.predictMs = Kept(a, 0.0, 200.0, 0.0);
        } else if ((rest = After(s, "noise"))) {
            const int got = sscanf_s(rest, "%lf %lf", &a, &b);
            if (got >= 1)
                c.noiseDegrees = Kept(a, 0.0, 5.0, 0.0);
            c.noiseMm = got >= 2 ? Kept(b, 0.0, 50.0, 0.0) : 0.0;
        } else if ((rest = After(s, "noisefast"))) {
            const int got = sscanf_s(rest, "%lf %lf %lf", &a, &b, &d);
            if (got >= 1)
                c.noiseFast = Kept(a, 0.0, 5.0, 0.0);
            if (got >= 3) {
                c.noiseFrom = Kept(b, 0.0, 2000.0, 30.0);
                c.noiseTo = std::max(Kept(d, 0.0, 2000.0, 150.0), c.noiseFrom + 1.0);
            }
        } else if ((rest = After(s, "rate"))) {
            const int got = sscanf_s(rest, "%lf %lf", &a, &b);
            c.rateLateMs = got >= 1 ? Kept(a, -50.0, 200.0, 0.0) : 0.0;
            c.rateNoise = got >= 2 ? Kept(b, 0.0, 200.0, 0.0) : 0.0;
            // ("own" as a word of its own, after the numbers.)
            c.rateOwn = false;
            for (const char* w = rest; (w = std::strstr(w, "own")) != nullptr; w += 3)
                if ((w == rest || w[-1] == ' ' || w[-1] == '\t') && (w[3] == '\0' || w[3] == ' ' || w[3] == '\t'))
                    c.rateOwn = true;
        } else if ((rest = After(s, "level"))) {
            c.level = true;
            c.levelPitch = sscanf_s(rest, "%lf", &a) == 1 ? Kept(a, -80.0, 80.0, 0.0) : 0.0;
        } else if ((rest = After(s, "log"))) {
            if (sscanf_s(rest, "%lf", &a) == 1)
                c.logSeconds = Kept(a, 0.0, 3600.0, 0.0);
        } else if ((rest = After(s, "seconds"))) {
            if (sscanf_s(rest, "%lf", &a) == 1)
                c.seconds = Kept(a, 0.1, 3600.0, 60.0);
        }
    }
    return c;
}

namespace {

// A file's rows "ms yaw pitch [roll x y z]" in order of time, timed from the first.
void ReadRows(const char* text, std::vector<Row>& rows) {
    rows.clear();
    double first = 0.0;
    for (const char* p = text ? text : ""; *p;) {
        char line[256];
        size_t n = 0;
        while (*p && *p != '\n') {
            if (n + 1 < sizeof(line) && *p != '\r')
                line[n++] = *p;
            ++p;
        }
        if (*p == '\n')
            ++p;
        line[n] = '\0';
        const char* s = Skip(line);
        if (!*s || *s == '#')
            continue;
        double v[7] = {};
        const int got = sscanf_s(s, "%lf %lf %lf %lf %lf %lf %lf", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6]);
        bool good = got >= 3;
        for (int i = 0; i < got && i < 7; ++i)
            good = good && std::isfinite(v[i]);
        if (!good)
            continue;
        if (rows.empty())
            first = v[0];
        Row r;
        r.ms = v[0] - first;
        if (!rows.empty() && !(r.ms > rows.back().ms))
            continue;  // (its rows in order of time)
        // The yaw and the roll go on from the row before the short way round, without their jumps at +-180: the curve through the rows has no leap in it.
        r.yaw = rows.empty() ? v[1] : rows.back().yaw + std::remainder(v[1] - rows.back().yaw, 360.0);
        r.pitch = Kept(v[2], -89.0, 89.0, 0.0);
        r.roll = got < 4 ? 0.0 : rows.empty() ? v[3] : rows.back().roll + std::remainder(v[3] - rows.back().roll, 360.0);
        r.placed = got >= 7;
        if (r.placed)
            r.x = Kept(v[4], -2.0, 2.0, 0.0), r.y = Kept(v[5], -2.0, 2.0, 0.0), r.z = Kept(v[6], -2.0, 2.0, 0.0);
        rows.push_back(r);
        if (rows.size() >= 400000)
            break;
    }
}

// The track between its rows, ms from its first row (inside them): a cubic through each row with the slope its two neighbours give (at either end, the slope to
// the one neighbour), so neither the angle nor its rate steps at a row.
struct Along {
    const Row* a = nullptr;
    const Row* b = nullptr;
    double h = 1.0, k[4] = {};  // the stretch's length, and each end's share and each end's slope's
    double Of(double Row::*what, const std::vector<Row>& rows) const {
        const size_t i = static_cast<size_t>(a - rows.data());
        const double slopeA = i > 0 ? (b->*what - (a - 1)->*what) / (b->ms - (a - 1)->ms) : (b->*what - a->*what) / h;
        const double slopeB = i + 2 < rows.size() ? ((b + 1)->*what - a->*what) / ((b + 1)->ms - a->ms) : (b->*what - a->*what) / h;
        return k[0] * (a->*what) + k[1] * h * slopeA + k[2] * (b->*what) + k[3] * h * slopeB;
    }
};

Along AlongRows(const std::vector<Row>& rows, double ms) {
    Along s;
    const auto next = std::upper_bound(rows.begin() + 1, rows.end() - 1, ms, [](double at, const Row& r) { return at < r.ms; });
    s.b = &*next;
    s.a = s.b - 1;
    s.h = s.b->ms - s.a->ms;
    const double u = std::min(std::max((ms - s.a->ms) / s.h, 0.0), 1.0), u2 = u * u, u3 = u2 * u;
    s.k[0] = 2.0 * u3 - 3.0 * u2 + 1.0, s.k[1] = u3 - 2.0 * u2 + u, s.k[2] = -2.0 * u3 + 3.0 * u2, s.k[3] = u3 - u2;
    return s;
}

}  // namespace

bool ParseTrack(const char* text, Config& c) {
    ReadRows(text, c.track);
    c.trackMs = 0.0;
    c.locatedOff.clear();  // (they were another track's)
    if (c.track.size() < 2) {
        c.track.clear();
        c.kind = Kind::kNone;
        return false;
    }
    c.kind = Kind::kTrack;
    c.trackMs = c.track.back().ms + kEase * 1000.0;
    return true;
}

bool ParseLocated(const char* text, Config& c) {
    c.locatedOff.clear();
    if (c.kind != Kind::kTrack || c.track.size() < 2)
        return false;
    std::vector<Row> rows;
    ReadRows(text, rows);
    // Each row as how far it is off the track at its own time (the yaw the short way round).
    for (const Row& r : rows) {
        if (r.ms > c.track.back().ms)
            break;
        const Along s = AlongRows(c.track, r.ms);
        Row off;
        off.ms = r.ms;
        off.yaw = std::remainder(r.yaw - s.Of(&Row::yaw, c.track), 360.0);
        off.pitch = r.pitch - s.Of(&Row::pitch, c.track);
        c.locatedOff.push_back(off);
    }
    if (c.locatedOff.size() < 2)
        c.locatedOff.clear();
    return !c.locatedOff.empty();
}

std::string Describe(const Config& c) {
    char text[900];
    int n = 0;
    auto more = [&](const char* format, auto... args) {
        if (n >= 0 && static_cast<size_t>(n) < sizeof(text))
            n += std::snprintf(text + n, sizeof(text) - n, format, args...);
    };
    if (c.kind == Kind::kShake) {
        const double fastest = 2.0 * kPi * c.hz * std::sqrt(c.yaw * c.yaw + c.pitch * c.pitch);
        if (c.yaw != 0.0 && c.pitch != 0.0)
            more("a sine in yaw and pitch, %.1f and %.1f degrees each way, %.2f a second", c.yaw, c.pitch, c.hz);
        else
            more("a sine in %s, %.1f degrees each way, %.2f a second", c.pitch != 0.0 ? "pitch" : "yaw", c.pitch != 0.0 ? c.pitch : c.yaw, c.hz);
        more(" (at its fastest %.0f degrees a second)", fastest);
    } else if (c.kind == Kind::kBurst) {
        more("bursts in %s: %.1f degrees in %.0f ms and back, %.0f ms between (at its fastest %.0f degrees a second)", c.burstPitch ? "pitch" : "yaw", c.burst, c.swingMs,
             c.pauseMs, kPi / 2.0 * std::fabs(c.burst) / (c.swingMs * 1e-3));
    } else if (c.kind == Kind::kTrack) {
        more("the track %s: %zu rows over %.1f s, looped", c.trackFile.c_str(), c.track.size(), c.track.empty() ? 0.0 : c.track.back().ms * 1e-3);
    } else {
        more("no motion");
    }
    more("; for %.1f s; the eyes %.3f m ahead of and %.3f m above their pivot", c.seconds, c.neckForward, c.neckUp);
    const bool located = c.kind == Kind::kTrack && !c.locatedOff.empty();
    if (located)
        more("; the located head as %s has it (%zu rows: each frame off the true head by the nearest row's own error)", c.locatedFile.c_str(), c.locatedOff.size());
    else if (c.predictMs > 0.0)
        more("; the located head carried on %.1f ms by its speed (predict)", c.predictMs);
    if (!located && c.noiseFast > 0.0)
        more("; the located head off by %.3f degrees an axis up to %.0f degrees a second, growing to %.3f at %.0f, and by %.2f mm (noise)", c.noiseDegrees, c.noiseFrom, c.noiseFast,
             c.noiseTo, c.noiseMm);
    else if ((!located && c.noiseDegrees > 0.0) || c.noiseMm > 0.0)
        more("; the located head off by %.3f degrees and %.2f mm an axis (noise)", located ? 0.0 : c.noiseDegrees, c.noiseMm);
    if (c.rateLateMs != 0.0 || c.rateNoise > 0.0 || c.rateOwn)
        more("; the turn rate given as it was %.1f ms before, off by %.1f degrees a second an axis%s (rate)", c.rateLateMs, c.rateNoise,
             c.rateOwn ? ", as the head itself has it" : "");
    if (c.level)
        more("; level at %.1f degrees of pitch", c.levelPitch);
    if (c.logSeconds > 0.0)
        more("; a line a frame for %.0f s", c.logSeconds);
    return text;
}

Offsets OffsetsAt(const Config& c, double t) {
    Offsets o;
    if (c.kind == Kind::kShake) {
        const double s = std::sin(2.0 * kPi * c.hz * t);
        o.yaw = c.yaw * s;
        o.pitch = c.pitch * s;
    } else if (c.kind == Kind::kBurst) {
        const double swing = c.swingMs * 1e-3, pause = c.pauseMs * 1e-3, whole = 2.0 * (swing + pause);
        double u = t - kEase, share = 0.0;
        if (u > 0.0 && swing > 0.0) {
            u = std::fmod(u, whole);
            if (u < swing)
                share = 0.5 - 0.5 * std::cos(kPi * u / swing);
            else if (u < swing + pause)
                share = 1.0;
            else if (u < 2.0 * swing + pause)
                share = 0.5 + 0.5 * std::cos(kPi * (u - swing - pause) / swing);
        }
        (c.burstPitch ? o.pitch : o.yaw) = c.burst * share;
    } else if (c.kind == Kind::kTrack && c.track.size() >= 2 && c.trackMs > 0.0) {
        const double last = c.track.back().ms;
        const double ms = std::fmod(std::max(t, 0.0) * 1000.0, c.trackMs);
        if (ms >= last) {
            // From the last row back to the first, eased, the short way round.
            const Row& a = c.track.back();
            const Row& b = c.track.front();
            const double share = c.trackMs > last ? 0.5 - 0.5 * std::cos(kPi * (ms - last) / (c.trackMs - last)) : 1.0;
            o.yaw = a.yaw + std::remainder(b.yaw - a.yaw, 360.0) * share;
            o.pitch = Mixed(a.pitch, b.pitch, share);
            o.roll = a.roll + std::remainder(b.roll - a.roll, 360.0) * share;
            o.placed = a.placed && b.placed;
            if (o.placed)
                o.x = Mixed(a.x, b.x, share), o.y = Mixed(a.y, b.y, share), o.z = Mixed(a.z, b.z, share);
        } else {
            // Between two rows, on the curve through them.
            const Along s = AlongRows(c.track, ms);
            o.yaw = s.Of(&Row::yaw, c.track);
            o.pitch = std::min(std::max(s.Of(&Row::pitch, c.track), -89.0), 89.0);
            o.roll = s.Of(&Row::roll, c.track);
            o.placed = s.a->placed && s.b->placed;
            if (o.placed)
                o.x = s.Of(&Row::x, c.track), o.y = s.Of(&Row::y, c.track), o.z = s.Of(&Row::z, c.track);
        }
    }
    return o;
}

double Envelope(const Config& c, double t) {
    if (!(t > 0.0))
        return 0.0;
    const double in = t < kEase ? 0.5 - 0.5 * std::cos(kPi * t / kEase) : 1.0;
    double out = 1.0;
    if (t > c.seconds)
        out = t < c.seconds + kEase ? 0.5 + 0.5 * std::cos(kPi * (t - c.seconds) / kEase) : 0.0;
    return in * out;
}

bool Over(const Config& c, double t) {
    return t >= c.seconds + kEase;
}

namespace {

// The head with the motion's offsets in it, that share of the way from the head as it was.
Pose PoseOf(const Config& c, const Pose& base, const Offsets& o, double share) {
    const Turn from = c.level ? FromAngles({ToAngles(base.turn).yaw, c.levelPitch, 0.0}) : base.turn;
    Pose full;
    // The yaw about the room's up, the pitch and the roll about the head's own axes: a shaken head, a nodding one.
    full.turn = Unit(Mul(Mul(About(1, o.yaw), from), Mul(About(0, o.pitch), About(2, o.roll))));
    if (o.placed) {
        full.place[0] = base.place[0] + o.x, full.place[1] = base.place[1] + o.y, full.place[2] = base.place[2] + o.z;
    } else {
        // The eyes ahead of and above a pivot that stays where it is.
        const double neck[3] = {0.0, c.neckUp, -c.neckForward};
        double was[3], now[3];
        Rotate(from, neck, was);
        Rotate(full.turn, neck, now);
        for (int i = 0; i < 3; ++i)
            full.place[i] = base.place[i] - was[i] + now[i];
    }
    if (share >= 1.0)
        return full;
    Pose p;
    p.turn = Part(base.turn, full.turn, share);
    for (int i = 0; i < 3; ++i)
        p.place[i] = Mixed(base.place[i], full.place[i], share);
    return p;
}

// The true head's turn rate at t: radians a second, a vector along the turn's axis as the room has it (over half a millisecond either side).
void TrueRate(const Config& c, const Pose& base, double t, double* out) {
    const double h = 0.0005;
    ToVector(Mul(HeadAt(c, base, t + h).turn, Inverse(HeadAt(c, base, t - h).turn)), out);
    for (int i = 0; i < 3; ++i)
        out[i] /= 2.0 * h;
}

// How much the located head's turn is off an axis at t ("noise", growing with the true head's speed under "noisefast"), degrees.
double TurnNoise(const Config& c, const Pose& base, double t) {
    if (c.noiseFast <= 0.0)
        return c.noiseDegrees;
    double rate[3];
    TrueRate(c, base, t, rate);
    const double speed = std::sqrt(rate[0] * rate[0] + rate[1] * rate[1] + rate[2] * rate[2]) / kDeg;
    const double share = std::min(std::max((speed - c.noiseFrom) / (c.noiseTo - c.noiseFrom), 0.0), 1.0);
    return Mixed(c.noiseDegrees, c.noiseFast, share);
}

}  // namespace

Pose HeadAt(const Config& c, const Pose& base, double t) {
    const double share = Envelope(c, t);
    if (share <= 0.0 || !c.Moves())
        return base;
    return PoseOf(c, base, OffsetsAt(c, t), share);
}

Pose LocatedAt(const Config& c, const Pose& base, double t, uint32_t frame) {
    Pose p;
    const bool rows = c.kind == Kind::kTrack && !c.locatedOff.empty();
    if (rows) {
        // The true head off by the located rows' own error: the row nearest in time, whole (none on the loop's way back to its start).
        const double share = Envelope(c, t);
        if (share <= 0.0) {
            p = base;
        } else {
            Offsets o = OffsetsAt(c, t);
            const double ms = std::fmod(std::max(t, 0.0) * 1000.0, c.trackMs);
            const auto next = std::lower_bound(c.locatedOff.begin(), c.locatedOff.end(), ms, [](const Row& r, double at) { return r.ms < at; });
            const Row* near = next == c.locatedOff.end() ? &c.locatedOff.back() : &*next;
            if (next != c.locatedOff.begin() && next != c.locatedOff.end() && ms - (next - 1)->ms <= next->ms - ms)
                near = &*(next - 1);
            const Row& last = c.locatedOff.back();
            if (ms <= last.ms + 0.5 * (last.ms - (&last - 1)->ms)) {
                o.yaw += near->yaw;
                o.pitch = std::min(std::max(o.pitch + near->pitch, -89.0), 89.0);
            }
            p = PoseOf(c, base, o, share);
        }
    } else if (c.predictMs > 0.0) {
        // From the true head that long before, carried on by its speed then (a half millisecond either side of it).
        const double ahead = c.predictMs * 1e-3, h = 0.0005;
        const Pose then = HeadAt(c, base, t - ahead), a = HeadAt(c, base, t - ahead - h), b = HeadAt(c, base, t - ahead + h);
        double turned[3];
        ToVector(Mul(b.turn, Inverse(a.turn)), turned);
        for (double& v : turned)
            v *= ahead / (2.0 * h);
        p.turn = Unit(Mul(FromVector(turned), then.turn));
        for (int i = 0; i < 3; ++i)
            p.place[i] = then.place[i] + (b.place[i] - a.place[i]) * ahead / (2.0 * h);
    } else {
        p = HeadAt(c, base, t);
    }
    // (The located rows are the located turn already: no noise on top of them.)
    const double turnNoise = rows ? 0.0 : TurnNoise(c, base, t);
    if (turnNoise > 0.0 || c.noiseMm > 0.0) {
        uint32_t n = Mix(frame * 2654435761u + 12345u);
        double v[3];
        for (double& part : v)
            part = Normal(n) * turnNoise * kDeg;
        p.turn = Unit(Mul(FromVector(v), p.turn));
        for (double& part : p.place)
            part += Normal(n) * c.noiseMm * 1e-3;
    }
    return p;
}

void RateAt(const Config& c, const Pose& base, double t, uint32_t frame, double* out) {
    const double at = t - c.rateLateMs * 1e-3;
    TrueRate(c, base, at, out);
    if (c.rateNoise > 0.0) {
        uint32_t n = Mix(frame * 2246822519u + 777u);
        for (int i = 0; i < 3; ++i)
            out[i] += Normal(n) * c.rateNoise * kDeg;
    }
    if (c.rateOwn)
        Rotate(Inverse(HeadAt(c, base, at).turn), out, out);
}

void RateFilter::NoteAge(uint32_t drawnAge) {
    const int now = static_cast<int>(std::min<uint32_t>(std::max<uint32_t>(drawnAge, 1u), 3u));
    if (now == ageSeen) {
        ++ageRun;
    } else {
        ageSeen = now;
        ageRun = 1;
    }
    if (ageRun >= kAgeAgreed)
        age = ageSeen;
}

Turn RateFilter::Step(const Turn& located, const double* rate, double seconds, double hz, double lead, bool fresh, double* apart) {
    if (fresh || !have || Between(located, stage[1]) > 40.0) {
        stage[0] = stage[1] = located;
        haveRate = false;
    } else if (seconds > 0.0) {
        // Carried by the turn the head made since the last frame: the mean of the rate then and the rate now, for that long. (This frame's rate alone is half
        // a frame ahead of that turn, and where the head speeds up or slows the carried turn would run ahead of it by that much each frame.)
        double by[3];
        for (int i = 0; i < 3; ++i)
            by[i] = (haveRate ? 0.5 * (rateWas[i] + rate[i]) : rate[i]) * seconds;
        const Turn step = FromVector(by);
        // Then pulled towards the located turn, twice over: what the rate has wrong never builds up.
        const double share = hz > 0.0 ? 1.0 - std::exp(-2.0 * kPi * hz * seconds) : 1.0;
        stage[0] = Part(Unit(Mul(step, stage[0])), located, share);
        stage[1] = Part(Unit(Mul(step, stage[1])), stage[0], share);
    }
    have = haveRate = true;
    for (int i = 0; i < 3; ++i)
        rateWas[i] = rate[i];
    if (apart)
        *apart = Between(stage[1], located);
    // Carried on for the picture's age by the same rate (35 degrees at the most, as the fixed lead).
    double on[3] = {rate[0] * lead, rate[1] * lead, rate[2] * lead};
    const double angle = std::sqrt(on[0] * on[0] + on[1] * on[1] + on[2] * on[2]);
    if (angle > 0.6)
        for (double& v : on)
            v *= 0.6 / angle;
    return angle > 1e-9 ? Unit(Mul(FromVector(on), stage[1])) : stage[1];
}

void RateFilter::Follow(const Turn& turn) {
    stage[0] = stage[1] = turn;
    have = true;
    haveRate = false;
}

void RateFit::Add(const Turn& located, const double* rate, double t) {
    if (count == kKept) {
        for (int i = 1; i < kKept; ++i)
            kept[i - 1] = kept[i];
        --count;
    }
    Sample& s = kept[count++];
    s.t = t;
    s.turn = located;
    s.stepped = false;
    for (int i = 0; i < 3; ++i)
        s.rate[i] = rate[i];
    if (count >= 2) {
        const Sample& was = kept[count - 2];
        const double dt = t - was.t;
        if (!(dt > 0.004 && dt < 0.03)) {
            kept[0] = kept[count - 1];  // a gap: no turning is told across it
            count = 1;
            return;
        }
        ToVector(Mul(located, Inverse(was.turn)), kept[count - 1].turned);
        for (double& v : kept[count - 1].turned)
            v /= dt;
        kept[count - 1].stepped = true;
    }
    // The frame three back is judged: the rates of the frames up to three after it are there, for a rate given late.
    const int j = count - 4;
    if (j < 2 || !kept[j].stepped)
        return;
    const Sample& mid = kept[j];
    const double size = mid.turned[0] * mid.turned[0] + mid.turned[1] * mid.turned[1] + mid.turned[2] * mid.turned[2];
    if (size < (30.0 * kDeg) * (30.0 * kDeg))
        return;
    // Its turning is the head's rate at the middle of its frame; a rate given so many ms late is the rate given that much after.
    const double at = mid.t - 0.5 * (mid.t - kept[j - 1].t);
    double offs[2][kLags];
    for (int l = 0; l < kLags; ++l) {
        const double x = at + kLagMs[l] * 1e-3;
        if (x < kept[0].t || x > kept[count - 1].t)
            return;
        int a = 0;
        while (a + 2 < count && kept[a + 1].t <= x)
            ++a;
        const Sample& from = kept[a];
        const Sample& to = kept[a + 1];
        const double share = (x - from.t) / (to.t - from.t);
        for (int own = 0; own < 2; ++own) {
            double ra[3] = {from.rate[0], from.rate[1], from.rate[2]}, rb[3] = {to.rate[0], to.rate[1], to.rate[2]};
            if (own) {
                Rotate(from.turn, ra, ra);
                Rotate(to.turn, rb, rb);
            }
            double sum = 0.0;
            for (int i = 0; i < 3; ++i) {
                const double d = mid.turned[i] - Mixed(ra[i], rb[i], share);
                sum += d * d;
            }
            offs[own][l] = sum;
        }
    }
    total += size;
    ++frames;
    for (int own = 0; own < 2; ++own)
        for (int l = 0; l < kLags; ++l)
            off[own][l] += offs[own][l];
}

bool RateFit::Judge() {
    if (frames < 24)
        return false;
    // Not the head's rate: at its best it leaves more than half of the head's turning unexplained (none at all leaves all of it).
    for (int o = 0; o < 2; ++o) {
        int best = 0;
        for (int l = 1; l < kLags; ++l)
            if (off[o][l] < off[o][best])
                best = l;
        fits[o] = off[o][best] <= 0.5 * total;
        bestMs[o] = kLagMs[best];
    }
    told = true;
    return true;
}

int RateFit::Line(char* out, size_t size, bool own) {
    if (!Judge())
        return 0;
    int best[2] = {0, 0};
    for (int o = 0; o < 2; ++o)
        for (int l = 1; l < kLags; ++l)
            if (off[o][l] < off[o][best[o]])
                best[o] = l;
    auto rms = [&](double sum) { return std::sqrt(sum / frames) / kDeg; };
    const int n = std::snprintf(out, size,
                                "Head rate fit: over %d frames of the head turning more than 30 degrees a second (%.0f on the whole), the rate given is %.0f degrees a second "
                                "off the located head's own turning as it comes and %.0f at its best, as the head's rate of %.0f ms before; taken as the head itself has it, "
                                "%.0f at its best (%.0f ms before)%s",
                                frames, rms(total), rms(off[0][1]), rms(off[0][best[0]]), kLagMs[best[0]], rms(off[1][best[1]]), kLagMs[best[1]],
                                fits[own ? 1 : 0] ? "" : ": as it is used it is NOT the head's rate");
    total = 0.0;
    frames = 0;
    for (auto& way : off)
        for (double& v : way)
            v = 0.0;
    return n;
}

bool RatePose::Step(const Turn& located, const double* rate, bool own, bool makeUp, double t, double seconds, double hz, double period, double aheadMs, uint32_t drawnAge, bool fresh,
                    const Turn& beside, Turn& out) {
    ++frames;
    filter.NoteAge(drawnAge);
    if (fresh)
        fit.Restart(), kept = 0;
    double asRoom[3] = {};
    bool usable = rate != nullptr && std::isfinite(rate[0]) && std::isfinite(rate[1]) && std::isfinite(rate[2]);
    if (usable) {
        fit.Add(located, rate, t);
        if (own)
            Rotate(located, rate, asRoom);
        else
            asRoom[0] = rate[0], asRoom[1] = rate[1], asRoom[2] = rate[2];
    } else {
        fit.Restart();
        kept = 0;
        ++noRate;
    }
    // Used only once it has shown itself the head's rate (the first verdict comes with 24 frames of turning), and while it stays so.
    if (usable && !fit.told)
        fit.Judge();
    if (usable && !fit.told) {
        usable = false;
        ++notYet;
    } else if (usable && !fit.fits[own ? 1 : 0]) {
        usable = false;
        ++notHeads;
    }
    // Its own change since two frames before, for a rate given late; and how uneven it is itself: the rate of two frames ago off the curve through the two before
    // it and the two after (none of either across a gap in the frames).
    if (kept > 0 && !(t - timeWas[kRates - 1] > 0.004 && t - timeWas[kRates - 1] < 0.03))
        kept = 0;
    double change[3] = {};
    const bool haveChange = kept >= 2;
    if (haveChange)
        for (int i = 0; i < 3; ++i)
            change[i] = (asRoom[i] - rateWas[kRates - 2][i]) / (t - timeWas[kRates - 2]);
    if (rate != nullptr && std::isfinite(asRoom[0] + asRoom[1] + asRoom[2])) {
        if (kept == kRates) {
            double sum = 0.0;
            for (int i = 0; i < 3; ++i) {
                const double offCurve = OffCurve(rateWas[0][i], rateWas[1][i], rateWas[2][i], rateWas[3][i], asRoom[i]);
                sum += offCurve * offCurve;
            }
            unevenSum += std::sqrt(sum) / kDeg;
            ++unevenOf;
        }
        for (int k = 0; k + 1 < kRates; ++k) {
            for (int i = 0; i < 3; ++i)
                rateWas[k][i] = rateWas[k + 1][i];
            timeWas[k] = timeWas[k + 1];
        }
        for (int i = 0; i < 3; ++i)
            rateWas[kRates - 1][i] = asRoom[i];
        timeWas[kRates - 1] = t;
        kept = std::min(kept + 1, kRates);
    }
    if (!usable) {
        // The turn of the filter beside it this frame, and on from there once the rate can be used again: no jump then.
        filter.Follow(beside);
        back = true;
        size = 0.0;
        return false;
    }
    double used[3] = {asRoom[0], asRoom[1], asRoom[2]};
    if (makeUp && haveChange && lateMs > 0.0)
        for (int i = 0; i < 3; ++i)
            used[i] += change[i] * lateMs * 1e-3;
    // All but none of it while the head is all but still: its noise would come through the lead.
    const double speed2 = used[0] * used[0] + used[1] * used[1] + used[2] * used[2], knee = kKnee * kDeg;
    const double share = speed2 / (speed2 + knee * knee);
    for (double& v : used)
        v *= share;
    size = std::sqrt(speed2) * share / kDeg;
    const double lead = std::max(0.0, filter.age * period - aheadMs * 1e-3);
    double apart = 0.0;
    out = filter.Step(located, used, seconds, hz, lead, fresh, &apart);
    back = false;
    sizeSum += size, sizeMost = std::max(sizeMost, size);
    apartSum += apart, apartWorst = std::max(apartWorst, apart);
    return true;
}

int RatePose::Line(char* out, size_t size_, char* fitOut, size_t fitSize, bool own, bool makeUp) {
    if (fitOut && fitSize > 0) {
        fitOut[0] = '\0';
        if (fit.Line(fitOut, fitSize, own) > 0) {
            // How late the rate is given, as the fit has it: half way to what this second said (a rate given early counts as on time).
            const double now = std::max(0.0, fit.bestMs[own ? 1 : 0]);
            lateMs = lateKnown ? 0.5 * (lateMs + now) : now;
            lateKnown = true;
        }
    }
    if (frames <= 0)
        return 0;
    const int used = frames - noRate - notHeads - notYet;
    char late[96] = "";
    if (makeUp)
        std::snprintf(late, sizeof(late), "; the rate carried on %.0f ms by its own change (as late as it is given)", lateMs);
    const int n = std::snprintf(out, size_,
                                "Head rate: the head's turn carried by the rate given (taken as %s has it): %.0f degrees a second on average, %.0f at the most; the carried turn "
                                "%.2f degrees from the located one on average, %.2f at the worst; led %d frame%s; %d of %d frames fell back to the filter beside it (%d with no "
                                "rate given, %d with a rate that is not the head's, %d before the rate had shown itself the head's); the rate itself %.1f degrees a second off "
                                "its own curve on average (a head's own is under 1 in a hard shake: more is the rate's noise)%s",
                                own ? "the head itself" : "the room", used > 0 ? sizeSum / used : 0.0, sizeMost, used > 0 ? apartSum / used : 0.0, apartWorst, filter.age,
                                filter.age == 1 ? "" : "s", noRate + notHeads + notYet, frames, noRate, notHeads, notYet, unevenOf > 0 ? unevenSum / unevenOf : 0.0, late);
    frames = noRate = notHeads = notYet = unevenOf = 0;
    sizeSum = sizeMost = apartSum = apartWorst = unevenSum = 0.0;
    return n;
}

double OffCurve(double a, double b, double c, double d, double e) {
    return std::fabs((-a + 4.0 * b - 6.0 * c + 4.0 * d - e) / 6.0);
}

int FrameLine(const Frame& f, char* out, size_t size) {
    static const char* const kHow[] = {"named", "guessed", "lag", "newest"};
    int n = 0;
    auto more = [&](const char* format, auto... args) {
        if (n >= 0 && static_cast<size_t>(n) < size)
            n += std::snprintf(out + n, size - n, format, args...);
    };
    auto head = [&](const char* name, const Pose& p) {
        const Angles a = ToAngles(p.turn);
        more(" | %s %.4f %.4f %.4f at %.2f %.2f %.2f", name, a.yaw, a.pitch, a.roll, p.place[0] * 1000.0, p.place[1] * 1000.0, p.place[2] * 1000.0);
    };
    more("Head test: t %.3f", f.ms);
    head("true", f.truth);
    head("located", f.located);
    head("filtered", f.filtered);
    head("picture", f.picture);
    more(" age %u %s", f.age, kHow[f.how >= kNamed && f.how <= kNewest ? f.how : kNewest]);
    if (f.haveCamera) {
        more(" | camera %s %.4f %.4f %.4f at %.5f %.5f %.5f", f.cameraOwn ? "own" : "pose", f.camera.yaw, f.camera.pitch, f.camera.roll, f.cameraPlace[0], f.cameraPlace[1],
             f.cameraPlace[2]);
        more(" | game %.4f %.4f %.4f at %.5f %.5f %.5f", f.game.yaw, f.game.pitch, f.game.roll, f.gamePlace[0], f.gamePlace[1], f.gamePlace[2]);
        more(" | move %.2f %.2f %.2f | look %d %.4f hand %.4f | pose %u build %u", f.moveMm[0], f.moveMm[1], f.moveMm[2], f.look ? 1 : 0, f.lookYaw, f.handYaw, f.pose, f.build);
    } else {
        more(" | camera none | pose %u", f.pose);
    }
    static const char* const kBy[] = {"fixed", "adapt", "rate", "fallback"};
    const Angles s = ToAngles(f.smooth);
    more(" | smooth %.4f %.4f %.4f | by %s %.1f", s.yaw, s.pitch, s.roll, kBy[f.by >= kByFixed && f.by <= kByFallback ? f.by : kByFixed], f.rate);
    // The world's turn, where the picture had main-view uploads to go by: by the build most of them carried, by its last before the first eye's picture, by its first.
    if (f.world >= 0.0) {
        more(" | world %.4f", f.world);
        if (f.worldLast >= 0.0)
            more(" last %.4f", f.worldLast);
        else
            more(" last -");
        if (f.worldFirst >= 0.0)
            more(" first %.4f", f.worldFirst);
        else
            more(" first -");
        if (f.mixed)
            more(" mixed");
    }
    return n;
}

void Second::Add(const Angles& slide, double speed, uint32_t age, bool named) {
    ++frames;
    ++ages[!named ? 3 : age <= 1 ? 0 : age == 2 ? 1 : 2];
    yawSum += std::fabs(slide.yaw), pitchSum += std::fabs(slide.pitch);
    yawWorst = std::max(yawWorst, std::fabs(slide.yaw)), pitchWorst = std::max(pitchWorst, std::fabs(slide.pitch));
    fastest = std::max(fastest, speed);
}

int Second::Line(char* out, size_t size) const {
    const double n = frames > 0 ? frames : 1;
    return std::snprintf(out, size,
                         "Head test: a second: the slide (the true head against the picture's pose) in yaw %.2f degrees on average, %.2f at the worst, in pitch %.2f and %.2f; "
                         "the head up to %.0f degrees a second; %d pictures, their poses 1 frame old %d, 2 %d, older %d, not named %d",
                         yawSum / n, yawWorst, pitchSum / n, pitchWorst, fastest, frames, ages[0], ages[1], ages[2], ages[3]);
}

void Naming::Add(uint32_t carried) {
    if (!carried)
        return;
    const int at = static_cast<int>(carried % kBuilds);
    if (build[at] != carried) {
        build[at] = carried;
        count[at][0] = count[at][1] = count[at][2] = 0;
    }
    ++count[at][eyes < 2 ? eyes : 2];
    if (!first)
        first = carried;
    last = carried;
}

void Naming::EyePicture() {
    if (eyes == 0)
        lastBeforeEyes = last;
    if (eyes < 2)
        ++eyes;
}

uint32_t Naming::Of(uint32_t carried) const {
    return Of(carried, 0) + Of(carried, 1) + Of(carried, 2);
}

uint32_t Naming::Of(uint32_t carried, int stage) const {
    const int at = static_cast<int>(carried % kBuilds);
    return carried && build[at] == carried && stage >= 0 && stage < 3 ? count[at][stage] : 0;
}

uint32_t Naming::All() const {
    uint32_t all = 0;
    for (int at = 0; at < kBuilds; ++at)
        if (build[at])
            all += count[at][0] + count[at][1] + count[at][2];
    return all;
}

int Naming::Builds() const {
    int builds = 0;
    for (int at = 0; at < kBuilds; ++at)
        builds += build[at] && Of(build[at]) > 0 ? 1 : 0;
    return builds;
}

uint32_t Naming::Most() const {
    uint32_t most = 0, of = 0;
    for (int at = 0; at < kBuilds; ++at) {
        const uint32_t n = build[at] ? Of(build[at]) : 0;
        if (n > of || (n == of && n > 0 && build[at] > most))
            most = build[at], of = n;
    }
    return most;
}

uint32_t Naming::Named(int rule, uint32_t byFirst) const {
    if (!first)
        return byFirst;  // (nothing to go by: no main view uploaded)
    if (rule == 1)
        return eyes > 0 && lastBeforeEyes ? lastBeforeEyes : last;
    if (rule == 2)
        return Most();
    return byFirst;
}

Turn CameraOfPose(const float* buildRows, const Turn& buildHead, const Turn& pose) {
    return Unit(Mul(Mul(RowsTurn(buildRows), Inverse(buildHead)), pose));
}

double CameraApart(const float* rows, const Turn& camera) {
    return Between(RowsTurn(rows), camera);
}

void WorldSecond::Add(double turn, double byLast, bool marked, double byFirst, bool mixedBuilds, bool hands) {
    ++frames;
    sum += turn;
    worst = std::max(worst, turn);
    over += turn > kOver ? 1 : 0;
    if (byLast >= 0.0) {
        ++lastOf;
        lastSum += byLast;
        lastWorst = std::max(lastWorst, byLast);
        unmarked += marked ? 0 : 1;
    }
    if (byFirst >= 0.0) {
        ++firstOf;
        firstSum += byFirst;
        firstWorst = std::max(firstWorst, byFirst);
    }
    mixed += mixedBuilds ? 1 : 0;
    handsOff += hands ? 1 : 0;
}

void WorldSecond::AddNone() {
    ++none;
}

int WorldSecond::Line(char* out, size_t size) const {
    if (frames <= 0 && none <= 0)
        return 0;
    if (frames <= 0)
        return std::snprintf(out, size, "World turn: none of the second's %d pictures had a main-view upload to go by (no view turned to the head, or none uploaded)", none);
    char last[96] = "", without[80] = "", hands[120] = "";
    if (unmarked > 0)
        std::snprintf(last, sizeof(last), " (%d of them by the frame's last: that moment not marked)", unmarked);
    if (none > 0)
        std::snprintf(without, sizeof(without), "; %d more with no main-view upload to go by", none);
    if (handsOff > 0)
        std::snprintf(hands, sizeof(hands), "; the gun and arms placed for another view build than the picture was named by in %d", handsOff);
    return std::snprintf(out, size,
                         "World turn: the headset is told a pose %.2f degrees from the cameras the picture was drawn with on average, %.2f at the worst; %d frames over %.2f "
                         "(of %d; by the view build most of each picture's main-view uploads carried); by its last upload before the first eye's picture went out %.2f and "
                         "%.2f%s, by its first %.2f and %.2f; %d pictures with main-view uploads of more than one view build%s%s",
                         sum / frames, worst, over, kOver, frames, lastOf > 0 ? lastSum / lastOf : 0.0, lastWorst, last, firstOf > 0 ? firstSum / firstOf : 0.0, firstWorst, mixed,
                         without, hands);
}

void EdgeSecond::Add(const Angles& slide, double size, double speed) {
    ++frames;
    fastest = std::max(fastest, speed);
    if (!(speed > kTurning) || turning >= kMost)
        return;
    whole[turning] = static_cast<float>(std::fabs(size));
    yaw[turning] = static_cast<float>(std::fabs(slide.yaw));
    pitch[turning] = static_cast<float>(std::fabs(slide.pitch));
    ++turning;
}

int EdgeSecond::Line(char* out, size_t size) {
    if (turning < kLeast)
        return 0;
    double stats[3][3];  // the whole, the yaw, the pitch: mean, 95%, worst
    float* const rows[3] = {whole, yaw, pitch};
    for (int k = 0; k < 3; ++k) {
        std::sort(rows[k], rows[k] + turning);
        double sum = 0.0;
        for (int i = 0; i < turning; ++i)
            sum += rows[k][i];
        stats[k][0] = sum / turning;
        stats[k][1] = rows[k][std::min(turning - 1, std::max(0, static_cast<int>(std::ceil(0.95 * turning)) - 1))];
        stats[k][2] = rows[k][turning - 1];
    }
    return std::snprintf(out, size,
                         "Edge: the picture %.2f degrees from the head as located for the frame it is shown in on average, %.2f at 95%%, %.2f at the worst; in yaw %.2f, %.2f "
                         "and %.2f, in pitch %.2f, %.2f and %.2f; over the %d of the second's %d frames in which the head turned more than %.0f degrees a second (up to %.0f)",
                         stats[0][0], stats[0][1], stats[0][2], stats[1][0], stats[1][1], stats[1][2], stats[2][0], stats[2][1], stats[2][2], turning, frames, kTurning, fastest);
}

}

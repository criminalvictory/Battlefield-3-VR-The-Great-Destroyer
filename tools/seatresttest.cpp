// A held gun's magazine's seat with bare arms (src/seatrest.h, header only, as skin.cpp's DrawHeldPart runs it) without the game. Each check prints PASS,
// FAIL or SKIP, then the totals; the exit code is the number failed.
//   s1  with gloves and the palms known nothing changes: for every combination, Learn is "the game's hands aren't moving the gun", whatever the bare
//       arms or the magazine's rest say; without palms and without bare arms (gloves, the palms not yet at rest) the magazine at rest on the frame, only
//       while no seat is known (before 2026-11-19 never: tools\magseattest.cpp)
//   s2  the magazine's rest on the frame: still for 499 ms no, for 500 ms yes; half a millimetre and a bit starts it over; a slow drift (0.1 mm a
//       frame, 7 mm a second) never rests; a turn of a quarter of a degree starts it over; not drawn for over 250 ms starts it over; numbers that
//       aren't; two draws' watches don't share
//   s3  the game's own reloads recorded on the train (weapon.txt "reloadrec" with "drawscan": build\survey\<gun>_empty.bin, _partial.bin, _dry.bin;
//       SKIP where there's none), every seat draw of each replayed as DrawHeldPart sees it (the body's and the depth pass's bones on their own frame,
//       the rounds' and the AK's other piece on the last body's or depth pass's frame): a seat is taken within the first second; every seat taken
//       from then on, through the reload and after it, is that same seat to half a millimetre (never one from a magazine on its way); while the
//       magazine is off its seat none is taken; the longest the magazine lay still on the frame off its seat (to be half of kRestMs at most).
//       Also the Mk11's (Operation Swordbreaker) and the M4A1's own reloads under the gloves' rule (the palms not yet at rest, no seat known: the
//       magazine at rest gives it, once): its seats within the first second, once each, the gun's first recording's; its reload of the gun empty, with
//       the seats known, takes none, though its magazine lies still off its seat there for 0.8 and 1.4 s (with none known, its first rest is 7.3 mm
//       off: why the gloves' rest is only for a seat not known, and each gun keeps its own)
// Usage: seatresttest [<folder with the recordings>] (build\survey by default, from the project's folder). Reads only.

#include "../src/seatrest.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

int g_checks = 0, g_failed = 0;
void Check(bool ok, const char* what) {
    ++g_checks;
    g_failed += !ok;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
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
bool Invert(const float* m, float* out) {
    const double a = m[0], b = m[1], c = m[2], d = m[4], e = m[5], f = m[6], g = m[8], h = m[9], k = m[10];
    const double det = a * (e * k - f * h) - b * (d * k - f * g) + c * (d * h - e * g);
    if (!(std::fabs(det) > 1e-12))
        return false;
    const double inv[9] = {(e * k - f * h) / det, (c * h - b * k) / det, (b * f - c * e) / det, (f * g - d * k) / det, (a * k - c * g) / det,
                           (c * d - a * f) / det, (d * h - e * g) / det, (b * g - a * h) / det, (a * e - b * d) / det};
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            out[i * 4 + j] = static_cast<float>(inv[i * 3 + j]);
        out[i * 4 + 3] = static_cast<float>(-(inv[i * 3] * m[3] + inv[i * 3 + 1] * m[7] + inv[i * 3 + 2] * m[11]));
    }
    return true;
}
float Apart(const float* a, const float* b) {
    return std::sqrt((a[3] - b[3]) * (a[3] - b[3]) + (a[7] - b[7]) * (a[7] - b[7]) + (a[11] - b[11]) * (a[11] - b[11]));
}

// skin.cpp's rows for the two guns of the train (kHeldKinds, HeldMagBones), and the Mk11's and the M4A1's (gloves).
struct Gun {
    const char* name;
    int body[2], bodyFrame, bodyMag;
    int depth[2], depthFrame, depthMag[2];
    int rounds[2], roundsBone;
    int part[2];       // the AK's other piece, on its own palette's bone partBone (0 0: none)
    int partBone = 0;  // (the Mk11's piece has the frame's plate on its bone 0, the magazine's sleeve on 1)
    bool gloves = false;  // a level with gloves: the seat from the magazine at rest only while none is known
};
constexpr Gun kGuns[] = {
    {"AKS74u", {17991, 0}, 2, 3, {19269, 19269}, 2, {3, 4}, {312, 18957}, 0, {894, 18063}},
    {"UMP45", {19314, 234}, 2, 0, {19638, 19638}, 3, {1, 0}, {234, 0}, 0, {0, 0}},
    {"MK11", {12456, 2214}, 0, 3, {14670, 14670}, 0, {1, 2}, {312, 1902}, 0, {1902, 0}, 1, true},
    {"M4A1", {22194, 312}, 2, 3, {22506, 22506}, 3, {0, 4}, {312, 0}, 0, {0, 0}, 0, true},
};
constexpr int kSeats = 4;  // skin.cpp's kMagBones: the colour pass's, the rounds', the depth pass's two

struct Replay {
    bool read = false;
    long records = 0, seatDraws = 0;
    int firstAt[2 * kSeats];          // ms: each watch's first seat taken (-1 never)
    float worst = 0.0f;               // m: the furthest a seat taken was from that watch's first
    long taken = 0, takenOff = 0;     // seats taken; of them with the magazine over 1 mm off its first seat
    int offFrom = -1, offUntil = -1;  // ms: the body's magazine first and last over 1 mm off its seat
    int stillOff = 0;                 // ms: the longest it lay still on the frame while off its seat
    float first[2 * kSeats][12] = {};  // each watch's first seat
    float worstOf[2 * kSeats] = {};
};

// known: seats known from the start (another recording's first ones, as a gun drawn again has its own kept), or none.
Replay Run(const Gun& gun, const std::string& path, const Replay* known = nullptr) {
    Replay out;
    std::fill(std::begin(out.firstAt), std::end(out.firstAt), -1);
    if (known)
        for (int i = 0; i < 2 * kSeats; ++i)
            if (known->firstAt[i] >= 0) {
                std::memcpy(out.first[i], known->first[i], sizeof(out.first[i]));
                out.firstAt[i] = 0;
            }
    FILE* f = nullptr;
    if (fopen_s(&f, path.c_str(), "rb") != 0 || !f)
        return out;
    out.read = true;
    seatrest::Watch watch[2 * kSeats];
    auto& first = out.first;
    float frame[12] = {};
    bool haveFrame = false;
    // The body's magazine, watched plainly: how long it's lain where it is.
    float lay[12] = {};
    int laySince = 0;
    bool haveLay = false;
    std::vector<float> bones;
    for (;;) {
        int32_t head[8];
        if (fread(head, sizeof(head), 1, f) != 1 || head[7] <= 0 || head[7] > 64)
            break;
        bones.resize(static_cast<size_t>(head[7]) * 12);
        if (fread(bones.data(), 48, static_cast<size_t>(head[7]), f) != static_cast<size_t>(head[7]))
            break;
        ++out.records;
        const int count = head[0], start = head[3], ms = head[6];
        const bool colour = count == gun.body[0] && start == gun.body[1], depth = count == gun.depth[0] && start == gun.depth[1];
        int seatBone[2] = {-1, -1}, seatIndex[2] = {-1, -1};
        if (colour) {
            std::memcpy(frame, bones.data() + gun.bodyFrame * 12, sizeof(frame));
            haveFrame = true;
            seatBone[0] = gun.bodyMag, seatIndex[0] = 0;
        } else if (depth) {
            std::memcpy(frame, bones.data() + gun.depthFrame * 12, sizeof(frame));
            haveFrame = true;
            seatBone[0] = gun.depthMag[0], seatIndex[0] = 2;
            seatBone[1] = gun.depthMag[1], seatIndex[1] = 3;
        } else if (count == gun.rounds[0] && start == gun.rounds[1]) {
            seatBone[0] = gun.roundsBone, seatIndex[0] = 1;
        } else if (gun.part[0] && count == gun.part[0] && start == gun.part[1]) {
            seatBone[0] = gun.partBone, seatIndex[0] = 0;
        } else {
            continue;
        }
        float inverse[12];
        if (!haveFrame || !Invert(frame, inverse))
            continue;
        for (int s = 0; s < 2; ++s) {
            if (seatBone[s] < 0 || seatBone[s] >= head[7])
                continue;
            const float* m = bones.data() + seatBone[s] * 12;
            if (m[0] * m[0] + m[4] * m[4] + m[8] * m[8] < 0.25f)  // the game's "hidden"
                continue;
            ++out.seatDraws;
            float on[12];
            Compose(inverse, m, on);
            const int slot = colour || depth ? seatIndex[s] : kSeats + seatIndex[s];
            const bool have = out.firstAt[slot] >= 0;
            const bool off = have && Apart(on, first[slot]) > 0.001f;
            if (colour) {
                if (off) {
                    if (out.offFrom < 0)
                        out.offFrom = ms;
                    out.offUntil = ms;
                    if (!haveLay || Apart(on, lay) > seatrest::kSame)
                        std::memcpy(lay, on, sizeof(lay)), laySince = ms, haveLay = true;
                    out.stillOff = std::max(out.stillOff, ms - laySince);
                } else {
                    haveLay = false;
                }
            }
            if (!seatrest::Learn(false, false, !gun.gloves, seatrest::Rests(watch[slot], on, static_cast<uint64_t>(ms)), have))
                continue;
            ++out.taken;
            if (!have) {
                std::memcpy(first[slot], on, sizeof(on));
                out.firstAt[slot] = ms;
            } else {
                out.worst = std::max(out.worst, Apart(on, first[slot]));
                out.worstOf[slot] = std::max(out.worstOf[slot], Apart(on, first[slot]));
                out.takenOff += off;
            }
        }
    }
    fclose(f);
    return out;
}

}  // namespace

int main(int argc, char** argv) {
    char what[640];

    // ---- s1 ----
    {
        bool gloves = true, none = true;
        for (int bits = 0; bits < 32; ++bits) {
            const bool known = bits & 1, moving = bits & 2, bare = bits & 4, rests = bits & 8, seat = bits & 16;
            const bool before = known && !moving;  // skin.cpp's rule before bare arms
            if (known)
                gloves = gloves && seatrest::Learn(known, moving, bare, rests, seat) == before;
            // (Without the palms and without bare arms, before 2026-11-19 never: a magazine out of a gun first drawn prone or sprinting, or after a death
            // prone, was drawn nowhere in the hand. Now as bare arms', but only while no seat is known: tools\magseattest.cpp.)
            if (!known && !bare)
                none = none && seatrest::Learn(known, moving, bare, rests, seat) == (rests && !seat);
        }
        Check(gloves, "s1 with the game's palms known (gloves): the seat is taken exactly when it was before (all 16 combinations)");
        Check(none, "s1 no palms and no bare arms (gloves, the palms not yet at rest): only at rest on the frame, and only while no seat is known (8 combinations)");
        Check(seatrest::Learn(false, false, true, true, false) && !seatrest::Learn(false, false, true, false, false) && seatrest::Learn(false, true, true, true, false) &&
                  seatrest::Learn(false, false, true, true, true),
              "s1 bare arms, no palms: taken when the magazine rests on the frame, not before (a seat known or not)");
    }

    // ---- s2 ----
    {
        const float seat[12] = {1, 0, 0, 0.0f, 0, 1, 0, -0.05f, 0, 0, 1, 0.43f};
        seatrest::Watch w;
        // Drawn every 10 ms from one time to another: whether it rested at any of them.
        const auto drawn = [](seatrest::Watch& watch, const float* on, uint64_t from, uint64_t to) {
            bool any = false;
            for (uint64_t at = from; at <= to; at += 10)
                any = seatrest::Rests(watch, on, at) || any;
            return any;
        };
        const bool early = drawn(w, seat, 1000, 1490);
        const bool at499 = seatrest::Rests(w, seat, 1499), at500 = seatrest::Rests(w, seat, 1500);
        Check(!early && !at499 && at500, "s2 still on the frame: not at 499 ms, at 500 ms");
        float moved[12];
        std::memcpy(moved, seat, sizeof(moved));
        moved[7] -= 0.0006f;
        const bool restarted = !seatrest::Rests(w, moved, 1510) && !drawn(w, moved, 1520, 2000) && !seatrest::Rests(w, moved, 2009) && seatrest::Rests(w, moved, 2010);
        Check(restarted, "s2 0.6 mm from where it was: it starts over, and rests there 500 ms on");
        moved[7] -= 0.0004f;
        Check(seatrest::Rests(w, moved, 2020), "s2 0.4 mm on: still the same rest");
        w = seatrest::Watch{};
        bool drifted = false;
        std::memcpy(moved, seat, sizeof(moved));
        for (uint64_t now = 0; now < 5000; now += 14) {
            moved[7] -= 0.0001f;
            drifted = drifted || seatrest::Rests(w, moved, now);
        }
        Check(!drifted, "s2 a slow drift (0.1 mm a frame, 7 mm a second, for 5 s): never a rest");
        w = seatrest::Watch{};
        drawn(w, seat, 0, 600);
        float turned[12];
        std::memcpy(turned, seat, sizeof(turned));
        const float a = 0.25f * 3.14159265f / 180.0f;
        turned[5] = std::cos(a), turned[6] = -std::sin(a), turned[9] = std::sin(a), turned[10] = std::cos(a);
        Check(seatrest::Rests(w, seat, 610) && !seatrest::Rests(w, turned, 620), "s2 turned a quarter of a degree in place: it starts over");
        w = seatrest::Watch{};
        drawn(w, seat, 0, 600);
        const bool gapShort = seatrest::Rests(w, seat, 850), gapLong = seatrest::Rests(w, seat, 1101);
        Check(gapShort && !gapLong && !drawn(w, seat, 1111, 1591) && !seatrest::Rests(w, seat, 1600) && seatrest::Rests(w, seat, 1601),
              "s2 not drawn for 250 ms: still its rest; for 251 ms: it starts over");
        float broken[12];
        std::memcpy(broken, seat, sizeof(broken));
        broken[3] = std::nanf("");
        const bool refused = !seatrest::Rests(w, broken, 1611);
        broken[3] = INFINITY;
        Check(refused && !seatrest::Rests(w, broken, 1621) && !seatrest::Rests(w, seat, 1631) && !drawn(w, seat, 1641, 2121) && !seatrest::Rests(w, seat, 2130) && seatrest::Rests(w, seat, 2131),
              "s2 numbers that aren't: no rest, and it starts over after them");
        seatrest::Watch one, other;
        std::memcpy(moved, seat, sizeof(moved));
        moved[11] += 0.2f;
        bool both = true;
        for (uint64_t now = 0; now <= 500; now += 10)
            both = seatrest::Rests(one, seat, now) & seatrest::Rests(other, moved, now);
        Check(both, "s2 two draws, each its own watch: both rest though 20 cm apart");
    }

    // ---- s3 ----
    {
        const std::string folder = argc > 1 ? argv[1] : "build\\survey";
        for (const Gun& gun : kGuns) {
            Replay kept;  // this gun's first recording with every seat: the others' seats are its
            for (const char* which : {"partial", "dry", "empty"}) {
                const std::string path = folder + "\\" + gun.name + "_" + which + ".bin";
                // (With gloves, the empty one is replayed with the gun's seats known: in the game's own reload of the gun empty (its bolt held back, as a
                // gun in the mod's hands never is: the hidden round) its magazine lies still off its seat on the frame for over a second, and its first
                // rest is 7.3 mm off the seat. With a seat known, as a gun drawn again has its own, no rest replaces it.)
                const bool emptyKnown = gun.gloves && kept.read && std::strcmp(which, "empty") == 0;
                const Replay r = emptyKnown ? Run(gun, path, &kept) : Run(gun, path);
                if (!r.read) {
                    std::printf("SKIP s3 %s: no recording\n", path.c_str());
                    continue;
                }
                int slots = 0, latest = -1;
                for (int at : r.firstAt)
                    if (at >= 0)
                        ++slots, latest = std::max(latest, at);
                const int wanted = gun.part[0] ? 5 : 4;  // the body's, the rounds', the depth pass's two, the AK's other piece
                if (gun.gloves) {
                    // The gloves' rule: a seat from the magazine at rest only while none is known (the palms not yet at rest: a first draw, prone, a sprint).
                    float apart = 0.0f;
                    for (int i = 0; i < 2 * kSeats; ++i)
                        if (r.firstAt[i] > 0 && kept.read && kept.firstAt[i] >= 0)
                            apart = std::max(apart, Apart(r.first[i], kept.first[i]));
                    if (emptyKnown) {
                        std::snprintf(what, sizeof(what),
                                      "s3 %s_%s with the gun's seats known (its first recording's, as a gun drawn again has them): %ld seat draws, none taken (%ld); in the game's own reload "
                                      "of it empty its magazine lay still off its seat for up to %d ms, and was off it %d-%d ms",
                                      gun.name, which, r.seatDraws, r.taken, r.stillOff, r.offFrom, r.offUntil);
                        Check(r.seatDraws > 100 && r.taken == 0, what);
                        continue;
                    }
                    std::snprintf(what, sizeof(what),
                                  "s3 %s_%s, gloves, no seat known at its start: %ld records, %ld seat draws: %d of %d seats taken from the magazine at rest, the last first at %d ms, %ld "
                                  "taken in all (once each)%s%.2f mm from the gun's first recording's; then its own reload laid the magazine still off its seat for up to %d ms (no seat taken: known)",
                                  gun.name, which, r.records, r.seatDraws, slots, wanted, latest, r.taken, kept.read ? ", " : " (its first recording) ", kept.read ? apart * 1000.0f : 0.0f,
                                  r.stillOff);
                    Check(slots == wanted && latest >= 0 && latest < 1000 && r.taken == slots && apart < 0.0005f, what);
                    if (slots == wanted && !kept.read)
                        kept = r;
                    continue;
                }
                std::snprintf(what, sizeof(what),
                              "s3 %s_%s: %ld records, %ld seat draws: %d of %d seats taken, the last of them first at %d ms; %ld taken in all, the furthest %.2f mm from its first; the magazine off "
                              "its seat %d-%d ms, %ld taken meanwhile; still on the frame off its seat for %d ms at most (the rest asked: %d)",
                              gun.name, which, r.records, r.seatDraws, slots, wanted, latest, r.taken, r.worst * 1000.0f, r.offFrom, r.offUntil, r.takenOff, r.stillOff,
                              static_cast<int>(seatrest::kRestMs));
                if (argc > 2)
                    for (int i = 0; i < 2 * kSeats; ++i)
                        if (r.firstAt[i] >= 0)
                            std::printf("     watch %d: first at %d ms, at (%.4f %.4f %.4f), the furthest %.2f mm\n", i, r.firstAt[i], r.first[i][3], r.first[i][7], r.first[i][11],
                                        r.worstOf[i] * 1000.0f);
                // A recording begun with the reload already under way (the magazine never seen seated before it): its first seat is taken after the reload, and is checked
                // against the gun's other recordings' below.
                const bool begunInReload = r.offFrom < 0;
                Check(slots == wanted && latest >= 0 && r.worst < 0.0005f && r.takenOff == 0 && r.stillOff * 2 <= static_cast<int>(seatrest::kRestMs) &&
                          (begunInReload || (latest < 1000 && r.offUntil > r.offFrom)),
                      what);
                if (slots != wanted)
                    continue;
                if (!kept.read) {
                    kept = r;
                    continue;
                }
                float apart = 0.0f;
                for (int i = 0; i < 2 * kSeats; ++i)
                    if (r.firstAt[i] >= 0 && kept.firstAt[i] >= 0)
                        apart = std::max(apart, Apart(r.first[i], kept.first[i]));
                std::snprintf(what, sizeof(what), "s3 %s_%s: its seats are the gun's first recording's: %.2f mm apart at most%s", gun.name, which, apart * 1000.0f,
                              begunInReload ? " (begun in the reload: taken after it)" : "");
                Check(apart < 0.0005f, what);
            }
        }
    }

    std::printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed;
}

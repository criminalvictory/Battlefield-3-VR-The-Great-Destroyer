#include "markers.h"

#include <windows.h>
#include <MinHook.h>

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "camera.h"
#include "log.h"
#include "markermath.h"
#include "views.h"

namespace {

using IconUpdateFn = void(__fastcall*)(void* self, void* edx, const float* viewProj, void* second, void* third);
using HeadingFn = float(__fastcall*)(void* ecx, void* edx);
IconUpdateFn g_iconUpdate = nullptr;
HeadingFn g_heading = nullptr;
std::atomic<int> g_iconMode{0}, g_headingMode{0};

constexpr float kDegrees = 57.2957795f;
constexpr ULONGLONG kFresh = 500;     // a build older than this is not the head's now
constexpr ULONGLONG kSayEvery = 10000;
constexpr int kProven = 5;            // readings in a row before the game's way is taken as known

markermath::Rows RowsOf(const float* rows, const float* eye) {
    markermath::Rows r{};
    std::memcpy(r.right, rows, 3 * sizeof(float));
    std::memcpy(r.up, rows + 3, 3 * sizeof(float));
    std::memcpy(r.back, rows + 6, 3 * sizeof(float));
    std::memcpy(r.eye, eye, 3 * sizeof(float));
    return r;
}

// ---- The icons' matrix ----
// Touched only by the hook (the game calls it from one thread, once a frame).
struct IconState {
    markermath::Run asGame;  // readings in a row as the game camera's (a stale build's neither counts nor breaks it)
    bool known = false;
    markermath::Signs signs;
    ULONGLONG said = 0;
    uint32_t calls = 0, replaced = 0, unread = 0, stale = 0;
} g_icon;

void SayIcons(const markermath::Decoded& d, const views::BuiltCameras& c, bool have, int mode) {
    if (!d.ok) {
        Log("World markers: the icons' matrix isn't a camera's as read (residual %.1e, axes square to %.1e, %s layout tried first)%s", d.residual, d.square,
            d.layout == markermath::kRowVector ? "row-vector" : "column-vector", mode >= 2 ? ": left to the game" : "");
        return;
    }
    // A stale build's game camera rows are the head's from the build before (views.h rowsStale): nothing compared with them.
    const bool gameRows = have && !c.rowsStale;
    const markermath::Rows game = RowsOf(c.gameRows, c.gameEye), head = RowsOf(c.headRows, c.headEye);
    const float gameAhead[3] = {-game.back[0], -game.back[1], -game.back[2]}, headAhead[3] = {-head.back[0], -head.back[1], -head.back[2]};
    float toGame[3], toHead[3];
    for (int i = 0; i < 3; ++i)
        toGame[i] = d.cam.e[i] - game.eye[i], toHead[i] = d.cam.e[i] - head.eye[i];
    markermath::Signs s;
    float worst = 0.0f;
    const bool asGame = gameRows && markermath::Learn(d.cam, game, 3.0f, s, &worst);
    char status[200];
    if (mode < 2)
        std::snprintf(status, sizeof(status), "left to the game (worldicons 1)");
    else if (!g_icon.known)
        std::snprintf(status, sizeof(status), "not replaced yet: read as the game camera's %d of %d times in a row", g_icon.asGame.inRow, kProven);
    else if (c.offCentre)
        std::snprintf(status, sizeof(status), "left to the game while the pictures are off-centre");
    else
        std::snprintf(status, sizeof(status), "the head's in its place: %u of %u since the last line", g_icon.replaced, g_icon.calls);
    char stale[240] = "";
    if (g_icon.stale)
        std::snprintf(stale, sizeof(stale), "; %u of %u readings since the last line on builds whose game camera rows were the mod's own from the build before (not "
                      "compared, the run kept)", g_icon.stale, g_icon.calls);
    Log("World markers: the icons' matrix (%s layout, read back to %.1e) looks %.1f degrees from the game camera's line, %.1f from the head's; "
        "%.1f degrees across, %.1f up (the game's camera %.1f up, aspect %.3f; the eyes' %.1f, aspect %.3f), off its middle %.3f %.3f; its eye %.2f m from "
        "the game camera's, %.2f from the head's; its axes %s; %s%s",
        d.layout == markermath::kRowVector ? "row-vector" : "column-vector", d.residual, gameRows ? markermath::AngleDeg(d.cam.l, gameAhead) : -1.0f,
        have ? markermath::AngleDeg(d.cam.l, headAhead) : -1.0f, 2.0f * std::atan(1.0f / d.proj.sx) * kDegrees, 2.0f * std::atan(1.0f / d.proj.sy) * kDegrees,
        c.gameFov * kDegrees, c.gameAspect, c.eyeFov * kDegrees, c.eyeAspect, d.proj.cx, d.proj.cy, have ? markermath::Length(toGame) : -1.0f,
        have ? markermath::Length(toHead) : -1.0f,
        !have      ? "(no cameras built lately)"
        : !gameRows ? "not compared (this build's game camera rows were the mod's own from the build before)"
        : asGame   ? (s.r > 0 && s.u > 0 && s.l > 0 ? "the game camera's right, up and ahead" : "the game camera's, one or more reversed")
                   : "not the game camera's (it moved, or it's another camera)",
        status, stale);
    g_icon.calls = 0;
    g_icon.replaced = 0;
    g_icon.stale = 0;
}

void __fastcall IconUpdateHook(void* self, void* edx, const float* viewProj, void* second, void* third) {
    const int mode = g_iconMode.load(std::memory_order_relaxed);
    float m[16];
    if (mode <= 0 || !viewProj || !camera::SafeRead(reinterpret_cast<uintptr_t>(viewProj), m, sizeof(m))) {
        g_iconUpdate(self, edx, viewProj, second, third);
        return;
    }
    const markermath::Decoded d = markermath::Decode(m);
    views::BuiltCameras c;
    const ULONGLONG now = GetTickCount64();
    const bool have = views::LastCameras(c) && now - c.at < kFresh;
    ++g_icon.calls;
    // The game camera's own: its axes within 3 degrees of the game camera's rows, its eye within 3 m of the game camera's place; a stale build's rows (the
    // head's) neither count nor break the run.
    markermath::Signs s;
    const markermath::Reading reading = have ? markermath::IconReading(d, RowsOf(c.gameRows, c.gameEye), c.rowsStale, 3.0f, 3.0f, s) : markermath::kNoReading;
    if (have && d.ok && c.rowsStale)
        ++g_icon.stale;
    if (g_icon.asGame.Add(reading, kProven) && !g_icon.known) {
        g_icon.known = true;
        g_icon.signs = s;
        Log("World markers: the icons' matrix read as the game camera's %d times in a row (axes right %+.0f, up %+.0f, ahead %+.0f)%s", kProven, s.r, s.u, s.l,
            mode >= 2 ? ": the head's goes in its place from here" : "");
    }
    if (!d.ok)
        ++g_icon.unread;
    if (!g_icon.said || now - g_icon.said >= kSayEvery) {
        g_icon.said = now;
        SayIcons(d, c, have, mode);
    }
    if (mode >= 2 && g_icon.known && d.ok && have && !c.offCentre) {
        // The head's matrix, on a 16-byte boundary as a matrix of the game's is (the icons' code may load it whole with aligned loads; thread-local storage
        // in a DLL loaded late isn't promised more than 8 on 32-bit Windows, so the place is aligned by hand).
        thread_local float t_room[20];
        float* const head = reinterpret_cast<float*>((reinterpret_cast<uintptr_t>(t_room) + 15) & ~static_cast<uintptr_t>(15));
        const float tanY = std::tan(0.5f * c.eyeFov);
        if (markermath::Rebuild(d, g_icon.signs, RowsOf(c.headRows, c.headEye), tanY * (c.eyeAspect > 0.0f ? c.eyeAspect : 1.0f), tanY, head)) {
            ++g_icon.replaced;
            g_iconUpdate(self, edx, head, second, third);
            return;
        }
    }
    g_iconUpdate(self, edx, viewProj, second, third);
}

// ---- The heading ----
struct HeadingState {
    markermath::Run matched[2];  // each way's readings in a row (a stale build's neither counts nor breaks them)
    int way = -1;  // the game's way of working it out, once proven
    ULONGLONG said = 0;
    uint32_t calls = 0, replaced = 0, stale = 0;
    float lastGame = 0.0f, lastOurs[2] = {}, lastHead = 0.0f;
};
HeadingState g_head;
SRWLOCK g_headLock = SRWLOCK_INIT;  // (the helper may be called from more than one thread)

float __fastcall HeadingHook(void* ecx, void* edx) {
    const float game = g_heading(ecx, edx);
    const int mode = g_headingMode.load(std::memory_order_relaxed);
    if (mode <= 0 || game == 0.0f)
        return game;  // (0: its own way out, nothing worked out)
    views::BuiltCameras c;
    const ULONGLONG now = GetTickCount64();
    if (!views::LastCameras(c) || now - c.at >= kFresh)
        return game;
    float answer = game;
    AcquireSRWLockExclusive(&g_headLock);
    ++g_head.calls;
    if (c.rowsStale)
        ++g_head.stale;  // (this build's game camera rows were the head's from the build before: nothing learned from them)
    for (int w = 0; w < 2; ++w) {
        g_head.lastOurs[w] = markermath::Heading(c.gameRows, w);
        if (g_head.matched[w].Add(markermath::HeadingReading(game, c.gameRows, w, c.rowsStale, 1.0f), kProven) && g_head.way < 0) {
            g_head.way = w;
            Log("Heading: the game's heading worked out from its camera's rows as atan2(%s) %d times in a row%s", w == 0 ? "-x, -z" : "-z, -x", kProven,
                mode >= 2 ? ": the head's goes in its place from here" : "");
        }
    }
    g_head.lastGame = game;
    const float head = g_head.way >= 0 ? markermath::Heading(c.headRows, g_head.way) : 0.0f;
    g_head.lastHead = head;
    if (mode >= 2 && g_head.way >= 0) {
        answer = head;
        ++g_head.replaced;
    }
    if (!g_head.said || now - g_head.said >= kSayEvery) {
        g_head.said = now;
        char stale[240] = "";
        if (g_head.stale)
            std::snprintf(stale, sizeof(stale), "; %u of %u calls since the last line on builds whose game camera rows were the mod's own from the build before "
                          "(not compared, the runs kept)", g_head.stale, g_head.calls);
        Log("Heading: the game's %.1f degrees; from its camera's rows %.1f (atan2(-x, -z)) or %.1f (atan2(-z, -x))%s; the head's %s%.1f; %s%s", game * kDegrees,
            g_head.lastOurs[0] * kDegrees, g_head.lastOurs[1] * kDegrees, c.rowsStale ? " (this build's rows: the head's, not compared)" : "",
            g_head.way >= 0 ? "" : "(not worked out yet) ", head * kDegrees,
            mode < 2 ? "left to the game (hudheading 1)"
            : g_head.way < 0 ? "left to the game till its way is proven"
                             : "the head's given",
            stale);
        g_head.calls = g_head.replaced = g_head.stale = 0;
    }
    ReleaseSRWLockExclusive(&g_headLock);
    return answer;
}

// ---- The hooks ----
struct HookOne {
    unsigned at;
    const unsigned char* start;
    size_t size;
    void* detour;
    void** original;
    const char* name;
    bool done = false;
    ULONGLONG firstTry = 0;
    bool saidMissing = false;
};

bool SameCode(unsigned at, const unsigned char* bytes, size_t size) {
    unsigned char now[16] = {};
    return size <= sizeof(now) && camera::SafeRead(at, now, size) && std::memcmp(now, bytes, size) == 0;
}

void Try(HookOne& h) {
    if (h.done)
        return;
    const ULONGLONG now = GetTickCount64();
    if (!h.firstTry)
        h.firstTry = now;
    if (!SameCode(h.at, h.start, h.size)) {
        // Not there yet (bf3.exe decrypts itself at start-up), or another version of the game: said once, then looked at again quietly.
        if (now - h.firstTry > 30000 && !h.saidMissing) {
            h.saidMissing = true;
            Log("%s: the game's code at %08X isn't as read in bf3.exe 1.6.0.0: not hooked", h.name, h.at);
        }
        return;
    }
    h.done = true;
    void* target = reinterpret_cast<void*>(static_cast<uintptr_t>(h.at));
    if (MH_CreateHook(target, h.detour, h.original) != MH_OK || MH_EnableHook(target) != MH_OK) {
        Log("%s: could not hook %08X", h.name, h.at);
        *h.original = nullptr;
        return;
    }
    Log("%s: hooked (%08X)", h.name, h.at);
}

HookOne g_iconHook{markermath::code::kIconUpdate, markermath::code::kIconUpdateStart, sizeof(markermath::code::kIconUpdateStart),
                   reinterpret_cast<void*>(&IconUpdateHook), reinterpret_cast<void**>(&g_iconUpdate), "World markers"};
HookOne g_headingHook{markermath::code::kHeading, markermath::code::kHeadingStart, sizeof(markermath::code::kHeadingStart),
                      reinterpret_cast<void*>(&HeadingHook), reinterpret_cast<void**>(&g_heading), "Heading"};

}  // namespace

namespace markers {

void Update() {
    const int icons = g_iconMode.load(std::memory_order_relaxed), heading = g_headingMode.load(std::memory_order_relaxed);
    if (icons > 0)
        Try(g_iconHook);
    if (heading > 0)
        Try(g_headingHook);
    // The heading is the icons' code's own (markermath.h kHeadingCallers, the review of 2e323f2): the head's matrix with the gun's heading in the same
    // routine is a mix, said once each time the pair comes to it (from here, after both words of a weapon.txt read are set).
    static int s_saidPair = -1;
    const int pair = icons * 3 + heading;
    if (pair != s_saidPair) {
        s_saidPair = pair;
        if (icons >= 2 && heading < 2)
            Log("World markers: the head's matrix goes to the icons' code while the heading it works out with stays the gun's (hudheading %d): film them "
                "with hudheading 2 as well", heading);
    }
}

void SetIcons(int mode) {
    mode = mode < 0 ? 0 : (mode > 2 ? 2 : mode);
    if (g_iconMode.exchange(mode) != mode)
        Log("World markers: %s", mode == 0   ? "left to the game, nothing read (worldicons 0)"
                                 : mode == 1 ? "the icons' matrix read and said every 10 s, nothing changed (worldicons 1)"
                                             : "the head's matrix in the icons' place once theirs is read as the game camera's (worldicons 2)");
}

void SetHeading(int mode) {
    mode = mode < 0 ? 0 : (mode > 2 ? 2 : mode);
    if (g_headingMode.exchange(mode) != mode)
        Log("Heading: %s", mode == 0   ? "left to the game, nothing read (hudheading 0)"
                           : mode == 1 ? "the game's heading compared with its camera's rows every 10 s, nothing changed (hudheading 1)"
                                       : "the head's heading in the game's place once its way is proven (hudheading 2)");
}

}  // namespace markers

#include "launchreport.h"

#include <windows.h>

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <string>

#include "log.h"

namespace launchreport {
namespace {

struct StepInfo {
    const char* waitingFor;  // "still waiting for ..."
    int lateAfterS;          // seconds after the step before it, before it counts as late
    const char* tryThis;
};

const StepInfo kInfo[kStepCount] = {
    {"the mod to go into the campaign", 0,
     "close any overlay or recording program that hooks the game (they can block the mod), then start the game again with "
     "Battlelog's PLAY CAMPAIGN (never multiplayer or co-op)."},
    {"the game's first picture", 60,
     "close any overlay or recording program that hooks the game, then start the game again."},
    {"the VR runtime and the headset", 30,
     "start SteamVR before the game and check it shows the headset as connected. Quest 3 or 2: connect through Steam Link, "
     "Virtual Desktop (in its SteamVR mode) or Quest Link/Air Link, with SteamVR running."},
    {"the VR session", 20, "close other VR apps, restart SteamVR, then start the game again."},
    {"SteamVR to start the session", 30,
     "put the headset on (its sensor must see your face), and close SteamVR Home or any other VR app holding the headset."},
    {"the first picture in the headset", 20, "restart SteamVR and the game; if it happens again, send this log."},
    {"the headset to give the game focus", 60,
     "close SteamVR's dashboard (the menu button) and look into the headset, or take it off and put it on once."},
};

// Static storage: all zero before anything runs.
std::atomic<ULONGLONG> g_begin;
std::atomic<ULONGLONG> g_okAt[kStepCount];
std::atomic<bool> g_failed[kStepCount];
std::atomic<ULONGLONG> g_lastDraw;
std::atomic<int> g_where;
std::mutex g_noteMutex;
std::string g_note[kStepCount];

double Seconds(ULONGLONG from, ULONGLONG to) { return to > from ? static_cast<double>(to - from) / 1000.0 : 0.0; }

const char* WhereWords(int where) {
    switch (where) {
    case kWaitingForHeadset:
        return "the mod is waiting for the headset to ask for its next picture (xrWaitFrame): SteamVR isn't taking pictures";
    case kHandingOver:
        return "the mod is handing a picture to SteamVR (xrEndFrame) and SteamVR hasn't taken it";
    default:
        return "the game itself hasn't come back with a new picture: a loading screen, or the game stuck";
    }
}

// Once a second: a step that's late gets its PROBLEM line (at its time, and again at four times it); once every step is
// there, one line says the launch worked. From the game's first picture on, a gap in its pictures is told at 15 s, 60 s
// and 120 s (the last as a PROBLEM), and its end too.
DWORD WINAPI Watch(LPVOID) {
    int toldLate[kStepCount] = {};
    bool toldAll = false;
    int gapTold = 0;  // 0 none, 1 at 15 s, 2 at 60 s, 3 at 120 s
    for (;;) {
        Sleep(1000);
        const ULONGLONG now = GetTickCount64();
        int next = -1;
        for (int s = 0; s < kStepCount; ++s)
            if (!g_okAt[s].load()) {
                next = s;
                break;
            }
        if (next < 0) {
            if (!toldAll) {
                toldAll = true;
                Log("LAUNCH: all %d steps OK, %.1f s after the mod went in: the VR launch worked. From here on, LAUNCH WATCH lines "
                    "tell if the game stops handing over new pictures for a while.",
                    static_cast<int>(kStepCount), Seconds(g_begin.load(), g_okAt[kStepCount - 1].load()));
            }
        } else if (next > 0 && !g_failed[next].load() && !g_failed[next - 1].load()) {
            const double waited = Seconds(g_okAt[next - 1].load(), now);
            const int late = kInfo[next].lateAfterS;
            const int level = waited >= late * 4.0 ? 2 : waited >= late ? 1 : 0;
            if (level > toldLate[next]) {
                toldLate[next] = level;
                std::string note;
                {
                    std::lock_guard lock(g_noteMutex);
                    note = g_note[next];
                }
                Log("LAUNCH %d/%d PROBLEM: still waiting for %s, %.0f s after step %d%s%s. What to try: %s", next + 1,
                    static_cast<int>(kStepCount), kInfo[next].waitingFor, waited, next, note.empty() ? "" : ": ", note.c_str(),
                    kInfo[next].tryThis);
            }
        }

        const ULONGLONG last = g_lastDraw.load();
        if (!last)
            continue;
        const double gap = Seconds(last, now);
        if (gap < 15.0) {
            if (gapTold) {
                Log("LAUNCH WATCH: the game hands over new pictures again (the gap was at least %d s)", gapTold == 1 ? 15 : gapTold == 2 ? 60 : 120);
                gapTold = 0;
            }
            continue;
        }
        const int level = gap >= 120.0 ? 3 : gap >= 60.0 ? 2 : 1;
        if (level <= gapTold)
            continue;
        gapTold = level;
        const char* where = WhereWords(g_where.load());
        int reached = 0;
        while (reached < kStepCount && g_okAt[reached].load())
            ++reached;
        if (level < 3)
            Log("LAUNCH WATCH: no new picture from the game for %.0f s (launch steps OK so far: %d of %d); %s", gap, reached,
                static_cast<int>(kStepCount), where);
        else
            Log("LAUNCH PROBLEM: no new picture from the game for %.0f s: it looks frozen (launch steps OK so far: %d of %d); %s. "
                "Please send build\\bf3vr.log and build\\injector.log before you start the game again (the next start replaces "
                "bf3vr.log).",
                gap, reached, static_cast<int>(kStepCount), where);
    }
}

void Format(char* out, size_t size, const char* fmt, va_list args) {
    vsnprintf(out, size, fmt, args);
    out[size - 1] = 0;
}

}  // namespace

void Begin() {
    ULONGLONG expected = 0;
    if (!g_begin.compare_exchange_strong(expected, GetTickCount64()))
        return;
    Log("LAUNCH: the launch report: each step below says OK or PROBLEM; LAUNCH lines are the ones to look for");
    if (HANDLE t = CreateThread(nullptr, 0, Watch, nullptr, 0, nullptr))
        CloseHandle(t);
}

void Ok(Step step, const char* fmt, ...) {
    if (static_cast<unsigned>(step) >= kStepCount || g_okAt[step].load(std::memory_order_relaxed))
        return;
    ULONGLONG expected = 0;
    const ULONGLONG now = GetTickCount64();
    if (!g_okAt[step].compare_exchange_strong(expected, now))
        return;
    char what[512];
    va_list args;
    va_start(args, fmt);
    Format(what, sizeof(what), fmt, args);
    va_end(args);
    Log("LAUNCH %d/%d OK: %s (%.1f s after the mod went in)", step + 1, static_cast<int>(kStepCount), what,
        Seconds(g_begin.load(), now));
}

void Problem(Step step, const char* fmt, ...) {
    if (static_cast<unsigned>(step) >= kStepCount)
        return;
    bool expected = false;
    if (!g_failed[step].compare_exchange_strong(expected, true))
        return;
    char what[512];
    va_list args;
    va_start(args, fmt);
    Format(what, sizeof(what), fmt, args);
    va_end(args);
    Log("LAUNCH %d/%d PROBLEM: %s. What to try: %s", step + 1, static_cast<int>(kStepCount), what, kInfo[step].tryThis);
}

void Note(Step step, const char* fmt, ...) {
    if (static_cast<unsigned>(step) >= kStepCount || g_okAt[step].load(std::memory_order_relaxed))
        return;
    char what[512];
    va_list args;
    va_start(args, fmt);
    Format(what, sizeof(what), fmt, args);
    va_end(args);
    std::lock_guard lock(g_noteMutex);
    g_note[step] = what;
}

void GameDrew() {
    g_lastDraw.store(GetTickCount64(), std::memory_order_relaxed);
    if (!g_okAt[kGameDraws].load(std::memory_order_relaxed))
        Ok(kGameDraws, "the game draws its pictures, and the mod sees them");
}

void HeadsetFrame() {
    if (!g_okAt[kFirstFrame].load(std::memory_order_relaxed))
        Ok(kFirstFrame, "the first picture went to the headset");
}

void Inside(Where where) { g_where.store(where, std::memory_order_relaxed); }

}  // namespace launchreport

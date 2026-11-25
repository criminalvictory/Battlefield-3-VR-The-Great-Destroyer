#include "launchreport.h"

#include <windows.h>
#include <d3d11.h>
#include <share.h>

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <string>
#include <vector>

#include "conflicts.h"
#include "log.h"

namespace launchreport {
namespace {

struct StepInfo {
    const char* waitingFor;  // "still waiting for ..."
    int lateAfterS;          // seconds after the step before it, before it counts as late (0: not timed)
    const char* tryThis;
};

const StepInfo kInfo[kStepCount] = {
    {"the mod to go into the campaign", 0,
     "close any overlay or recording program that hooks the game (they can block the mod), then start the game again with "
     "Battlelog's PLAY CAMPAIGN (never multiplayer or co-op)."},
    {"the game's first picture", 60, "close any overlay or recording program that hooks the game, then start the game again."},
    {"the VR runtime and the headset", 30,
     "start SteamVR before the game and check it shows the headset as connected. Quest 3 or 2: connect through Steam Link, "
     "Virtual Desktop (in its SteamVR mode) or Quest Link/Air Link, with SteamVR running."},
    {"the VR session", 20, "close other VR apps, restart SteamVR, then start the game again."},
    {"SteamVR to start the session", 30,
     "put the headset on (its sensor must see your face), and close SteamVR Home or any other VR app holding the headset."},
    {"the first picture in the headset", 20, "restart SteamVR and the game; if it happens again, send this log."},
    {"the headset to give the game focus", 60,
     "close SteamVR's dashboard (the menu button) and look into the headset, or take it off and put it on once."},
    {"a level", 0,
     "if the game's window on the monitor shows the level but the headset doesn't, send this log; if the window is black too, "
     "the game itself isn't drawing the level: restart the game, or try another mission."},
};

// A picture counts as black when even its brightest sampled pixel is under this (of 255): a dark level (2026-11-14's test: the
// forest at dawn averaged 6) still has brighter pixels somewhere, a black one has none.
constexpr float kBlack = 8.0f;

// Static storage: all zero before anything runs.
std::atomic<ULONGLONG> g_begin;
std::atomic<ULONGLONG> g_okAt[kStepCount];
std::atomic<bool> g_failed[kStepCount];
std::atomic<ULONGLONG> g_lastDraw;
std::atomic<int> g_where;
std::atomic<int> g_framesThisSecond;
std::atomic<bool> g_inLevel;
std::atomic<float> g_brightness;     // the last reading's average, 0-255; below 0: none possible (the picture's format or samples)
std::atomic<float> g_brightest;      // the last reading's brightest pixel, 0-255
std::atomic<ULONGLONG> g_brightTakenAt;  // when the last reading's picture was copied
std::mutex g_noteMutex;
std::string g_note[kStepCount];
std::mutex g_fileMutex;
FILE* g_file = nullptr;  // build\launch.log: the LAUNCH lines alone

double Seconds(ULONGLONG from, ULONGLONG to) { return to > from ? static_cast<double>(to - from) / 1000.0 : 0.0; }

// One LAUNCH line: into bf3vr.log, and into launch.log with its time.
void Say(const char* fmt, ...) {
    char text[1400];
    va_list args;
    va_start(args, fmt);
    vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);
    text[sizeof(text) - 1] = 0;
    Log("%s", text);
    std::lock_guard lock(g_fileMutex);
    if (!g_file)
        return;
    SYSTEMTIME t;
    GetLocalTime(&t);
    fprintf(g_file, "[%02d:%02d:%02d.%03d] %s\n", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds, text);
    fflush(g_file);
}

// VDXR (Virtual Desktop's own runtime, Start BF3 VR (VDXR).bat): the steps that name SteamVR say what to try with it instead.
std::atomic<bool> g_vdxr{false};
const char* const kVdxrTry[kStepCount] = {
    nullptr,
    nullptr,
    "in the Virtual Desktop Streamer on this PC set the OpenXR runtime to VDXR (or Automatic), connect Virtual Desktop on the "
    "headset to this PC, and keep SteamVR closed; or start with Start BF3 VR.bat to play through SteamVR.",
    "close SteamVR and other VR apps, reconnect Virtual Desktop on the headset, then start the game again.",
    "put the headset on with Virtual Desktop connected to this PC, and keep SteamVR closed.",
    "reconnect Virtual Desktop and start the game again; if it happens again, send this log.",
    "close Virtual Desktop's menu on the headset and look into it, or take the headset off and put it on once.",
    nullptr,
};
const char* TryThis(int step) { return g_vdxr && kVdxrTry[step] ? kVdxrTry[step] : kInfo[step].tryThis; }
const char* WaitingFor(int step) { return g_vdxr && step == kRunning ? "the VR runtime to start the session" : kInfo[step].waitingFor; }

const char* WhereWords(int where) {
    switch (where) {
    case kWaitingForHeadset:
        return g_vdxr ? "the mod is waiting for the headset to ask for its next picture (xrWaitFrame): Virtual Desktop isn't taking pictures"
                      : "the mod is waiting for the headset to ask for its next picture (xrWaitFrame): SteamVR isn't taking pictures";
    case kHandingOver:
        return g_vdxr ? "the mod is handing a picture to Virtual Desktop (xrEndFrame) and it hasn't taken it"
                      : "the mod is handing a picture to SteamVR (xrEndFrame) and SteamVR hasn't taken it";
    default:
        return "the game itself hasn't come back with a new picture: a loading screen, or the game stuck";
    }
}

int Reached() {
    int reached = 0;
    while (reached < kStepCount && g_okAt[reached].load())
        ++reached;
    return reached;
}

// Once a second: a late step gets its PROBLEM line (at its time, and again at four times it); steps 1-7 all there is "VR is
// up"; the level's comings and goings, its picture (black or not) and the pictures a second the headset gets; all 8 there
// is "the launch worked". From the game's first picture on, a gap in its pictures is told at 15 s, 60 s and 120 s (the last
// as a PROBLEM), and its end too.
DWORD WINAPI Watch(LPVOID) {
    int toldLate[kStepCount] = {};
    bool toldUp = false, toldAll = false;
    int gapTold = 0;  // 0 none, 1 at 15 s, 2 at 60 s, 3 at 120 s
    bool wasInLevel = false, levelPictureTold = false, levelBlackTold = false, levelNoFramesTold = false;
    ULONGLONG levelSince = 0;
    int zeroFrameSeconds = 0;
    for (;;) {
        Sleep(1000);
        const ULONGLONG now = GetTickCount64();
        const int perSecond = g_framesThisSecond.exchange(0);

        int next = -1;
        for (int s = 0; s < kStepCount; ++s)
            if (!g_okAt[s].load()) {
                next = s;
                break;
            }
        if (next == kInLevel && !toldUp) {
            toldUp = true;
            Say("LAUNCH: VR is up: steps 1-7 OK, %.1f s after the mod went in. Step 8 comes when a level is on (the headset getting "
                "its picture)",
                Seconds(g_begin.load(), g_okAt[kFocused].load()));
        }
        if (next < 0 && !toldAll) {
            toldAll = true;
            Say("LAUNCH: all %d steps OK: the VR launch worked, and the level is in the headset. From here on, LAUNCH WATCH and "
                "LAUNCH LEVEL lines tell if pictures stop or a level comes up black",
                static_cast<int>(kStepCount));
        }
        if (next > 0 && next < kInLevel && !g_failed[next].load() && !g_failed[next - 1].load() && kInfo[next].lateAfterS) {
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
                Say("LAUNCH %d/%d PROBLEM: still waiting for %s, %.0f s after step %d%s%s. What to try: %s", next + 1,
                    static_cast<int>(kStepCount), WaitingFor(next), waited, next, note.empty() ? "" : ": ", note.c_str(),
                    TryThis(next));
            }
        }

        // The level (the player's soldier there): its picture, and the pictures the headset gets.
        const bool inLevel = g_inLevel.load();
        if (inLevel && !wasInLevel) {
            levelSince = now;
            levelPictureTold = levelBlackTold = levelNoFramesTold = false;
            zeroFrameSeconds = 0;
            Say("LAUNCH LEVEL: a level is on (the player is in it); the headset gets %d pictures a second", perSecond);
        } else if (!inLevel && wasInLevel) {
            Say("LAUNCH LEVEL: out of the level (a load, a cutscene, a death or the menu), after %.0f s", Seconds(levelSince, now));
        }
        wasInLevel = inLevel;
        if (inLevel) {
            zeroFrameSeconds = perSecond ? 0 : zeroFrameSeconds + 1;
            const float bright = g_brightness.load(), brightest = g_brightest.load();
            const bool fresh = g_brightTakenAt.load() >= levelSince;  // a reading of this level's picture
            const double inFor = Seconds(levelSince, now);
            if (!levelPictureTold && fresh && perSecond > 0 && (brightest >= kBlack || bright < 0.0f)) {
                levelPictureTold = true;
                char what[200];
                if (bright >= 0.0f)
                    sprintf_s(what, "brightness %.0f of 255, its brightest pixel %.0f", bright, brightest);
                else
                    sprintf_s(what, "its brightness can't be read on this PC's picture format");
                if (!g_okAt[kInLevel].load())
                    Ok(kInLevel, "in the level, and the headset gets the game's picture (%s, %d pictures a second)", what, perSecond);
                else
                    Say("LAUNCH LEVEL: the headset gets the level's picture (%s, %d pictures a second)", what, perSecond);
            }
            if (!levelBlackTold && fresh && bright >= 0.0f && brightest < kBlack && inFor >= 20.0 && !levelPictureTold) {
                levelBlackTold = true;
                Say("LAUNCH %d/%d PROBLEM: in the level for %.0f s, but the game's picture stays black (brightness %.1f of 255, its "
                    "brightest pixel %.0f; the headset gets %d pictures a second). What to try: %s",
                    static_cast<int>(kInLevel) + 1, static_cast<int>(kStepCount), inFor, bright, brightest, perSecond,
                    TryThis(kInLevel));
            }
            if (!levelNoFramesTold && zeroFrameSeconds >= 10) {
                levelNoFramesTold = true;
                const char* menu = g_vdxr ? "Virtual Desktop's menu" : "SteamVR's dashboard";
                Say("LAUNCH %d/%d PROBLEM: in the level, but the headset has had no pictures for %d s: it isn't asking for them "
                    "(asleep, off your head, or %s open), or the mod can't hand them over. What to try: put the "
                    "headset on and close %s; if it stays black, send this log",
                    static_cast<int>(kInLevel) + 1, static_cast<int>(kStepCount), zeroFrameSeconds, menu, menu);
            }
            if (levelNoFramesTold && perSecond > 0) {
                levelNoFramesTold = false;
                Say("LAUNCH LEVEL: the headset gets pictures again (%d a second)", perSecond);
            }
        }

        // The game's pictures: a gap told at 15 s, 60 s and 120 s, and its end.
        const ULONGLONG last = g_lastDraw.load();
        if (!last)
            continue;
        const double gap = Seconds(last, now);
        if (gap < 15.0) {
            if (gapTold) {
                Say("LAUNCH WATCH: the game hands over new pictures again (the gap was at least %d s)",
                    gapTold == 1 ? 15 : gapTold == 2 ? 60 : 120);
                gapTold = 0;
            }
            continue;
        }
        const int level = gap >= 120.0 ? 3 : gap >= 60.0 ? 2 : 1;
        if (level <= gapTold)
            continue;
        gapTold = level;
        if (level < 3)
            Say("LAUNCH WATCH: no new picture from the game for %.0f s (launch steps OK so far: %d of %d); %s", gap, Reached(),
                static_cast<int>(kStepCount), WhereWords(g_where.load()));
        else
            Say("LAUNCH PROBLEM: no new picture from the game for %.0f s: it looks frozen (launch steps OK so far: %d of %d); %s. "
                "Please send build\\launch.log, build\\bf3vr.log and build\\injector.log before you start the game again (the next "
                "start replaces them).",
                gap, Reached(), static_cast<int>(kStepCount), WhereWords(g_where.load()));
    }
}

void Format(char* out, size_t size, const char* fmt, va_list args) {
    vsnprintf(out, size, fmt, args);
    out[size - 1] = 0;
}

// What a pixel of the picture's format holds: 1 eight bits a colour (RGBA or BGRA), 2 ten bits (R10G10B10A2), 0 can't read.
int FormatKind(DXGI_FORMAT f) {
    switch (f) {
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return 1;
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
        return 2;
    default:
        return 0;
    }
}

// Other programs loading themselves into the game from its folder (conflicts.h: ReShade, 3DMigoto, DXVK, a proxy dxgi.dll...): a LAUNCH PROBLEM line
// each, with what to do; one line saying none were. Once, as the mod starts. The folder is bf3.exe's; its names are only listed (nothing opened).
void GameFolderHooks() {
    wchar_t exe[MAX_PATH] = {};
    if (!GetModuleFileNameW(nullptr, exe, MAX_PATH))
        return;
    std::wstring folder(exe);
    folder = folder.substr(0, folder.find_last_of(L"\\/") + 1);
    std::vector<conflicts::Entry> entries;
    WIN32_FIND_DATAW d{};
    HANDLE find = FindFirstFileW((folder + L"*").c_str(), &d);
    if (find == INVALID_HANDLE_VALUE)
        return;
    do {
        if (!wcscmp(d.cFileName, L".") || !wcscmp(d.cFileName, L".."))
            continue;
        char name[MAX_PATH * 2] = {};
        if (!WideCharToMultiByte(CP_UTF8, 0, d.cFileName, -1, name, sizeof(name), nullptr, nullptr))
            continue;
        entries.push_back({name, (d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0});
    } while (FindNextFileW(find, &d) && entries.size() < 4096);
    FindClose(find);
    const std::vector<conflicts::Finding> found = conflicts::Find(entries);
    for (const conflicts::Finding& f : found)
        Say("LAUNCH PROBLEM: %s in the game's folder (%s): %s", f.what.c_str(), f.files.c_str(), f.todo.c_str());
    if (found.empty())
        Say("LAUNCH: no other program's DLL in the game's folder (ReShade, 3DMigoto, DXVK, a proxy dxgi.dll and the like): nothing there to fight "
            "the mod");
}

constexpr UINT kPatch = 8, kGrid = 3, kSide = kPatch * kGrid;  // 9 patches of 8x8 pixels, into a 24x24 staging picture

// The sampled pixels' average brightness and their brightest, both 0-255.
float Mean(const D3D11_MAPPED_SUBRESOURCE& m, int kind, float& brightest) {
    double sum = 0.0, most = 0.0;
    for (UINT y = 0; y < kSide; ++y) {
        const uint32_t* row = reinterpret_cast<const uint32_t*>(static_cast<const unsigned char*>(m.pData) + y * m.RowPitch);
        for (UINT x = 0; x < kSide; ++x) {
            const uint32_t v = row[x];
            const double b = kind == 1 ? ((v & 0xFF) + ((v >> 8) & 0xFF) + ((v >> 16) & 0xFF)) / 3.0
                                       : ((v & 0x3FF) + ((v >> 10) & 0x3FF) + ((v >> 20) & 0x3FF)) / 12.0;  // 0-1023 to 0-255
            sum += b;
            if (b > most)
                most = b;
        }
    }
    brightest = static_cast<float>(most);
    return static_cast<float>(sum / (kSide * kSide));
}

}  // namespace

void Begin() {
    ULONGLONG expected = 0;
    if (!g_begin.compare_exchange_strong(expected, GetTickCount64()))
        return;
    g_brightness.store(-1.0f);
    HMODULE self = nullptr;
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(&Begin), &self)) {
        wchar_t path[MAX_PATH];
        if (GetModuleFileNameW(self, path, MAX_PATH)) {
            std::wstring p(path);
            p = p.substr(0, p.find_last_of(L"\\/") + 1) + L"launch.log";
            std::lock_guard lock(g_fileMutex);
            g_file = _wfsopen(p.c_str(), L"w", _SH_DENYNO);  // shared, so it can be read while the game runs
        }
    }
    Say("LAUNCH: the launch report (also alone in build\\launch.log): each step below says OK or PROBLEM; 8 steps, the last "
        "one a level in the headset");
    GameFolderHooks();
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
    char what[600];
    va_list args;
    va_start(args, fmt);
    Format(what, sizeof(what), fmt, args);
    va_end(args);
    Say("LAUNCH %d/%d OK: %s (%.1f s after the mod went in)", step + 1, static_cast<int>(kStepCount), what,
        Seconds(g_begin.load(), now));
}

void Problem(Step step, const char* fmt, ...) {
    if (static_cast<unsigned>(step) >= kStepCount)
        return;
    bool expected = false;
    if (!g_failed[step].compare_exchange_strong(expected, true))
        return;
    char what[600];
    va_list args;
    va_start(args, fmt);
    Format(what, sizeof(what), fmt, args);
    va_end(args);
    Say("LAUNCH %d/%d PROBLEM: %s. What to try: %s", step + 1, static_cast<int>(kStepCount), what, TryThis(step));
}

void Note(Step step, const char* fmt, ...) {
    if (static_cast<unsigned>(step) >= kStepCount || g_okAt[step].load(std::memory_order_relaxed))
        return;
    char what[600];
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
    g_framesThisSecond.fetch_add(1, std::memory_order_relaxed);
    if (!g_okAt[kFirstFrame].load(std::memory_order_relaxed))
        Ok(kFirstFrame, "the first picture went to the headset");
}

void Inside(Where where) { g_where.store(where, std::memory_order_relaxed); }

void Warn(const char* fmt, ...) {
    char what[600];
    va_list args;
    va_start(args, fmt);
    Format(what, sizeof(what), fmt, args);
    va_end(args);
    Say("LAUNCH PROBLEM: %s", what);
}

void Line(const char* fmt, ...) {
    char what[600];
    va_list args;
    va_start(args, fmt);
    Format(what, sizeof(what), fmt, args);
    va_end(args);
    Say("LAUNCH: %s", what);
}

void Vdxr() { g_vdxr = true; }
bool IsVdxr() { return g_vdxr; }

void Picture(ID3D11Texture2D* back, bool inLevel) {
    g_inLevel.store(inLevel, std::memory_order_relaxed);
    static ULONGLONG s_next = 0;
    static ID3D11Texture2D* s_staging = nullptr;
    static DXGI_FORMAT s_format = DXGI_FORMAT_UNKNOWN;
    static int s_kind = 0;
    static bool s_pending = false;
    static ULONGLONG s_copiedAt = 0;
    const ULONGLONG now = GetTickCount64();
    if (!back || now < s_next)
        return;
    s_next = now + 1000;
    ID3D11Device* device = nullptr;
    back->GetDevice(&device);
    if (!device)
        return;
    ID3D11DeviceContext* ctx = nullptr;
    device->GetImmediateContext(&ctx);
    if (!ctx) {
        device->Release();
        return;
    }
    // Last time's copy, read back without waiting for the GPU.
    if (s_pending && s_staging) {
        D3D11_MAPPED_SUBRESOURCE m{};
        const HRESULT hr = ctx->Map(s_staging, 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &m);
        if (hr == DXGI_ERROR_WAS_STILL_DRAWING) {
            s_next = now + 100;  // not done yet: try again soon
            ctx->Release();
            device->Release();
            return;
        }
        if (SUCCEEDED(hr)) {
            float brightest = 0.0f;
            g_brightness.store(Mean(m, s_kind, brightest));
            g_brightest.store(brightest);
            g_brightTakenAt.store(s_copiedAt);
            ctx->Unmap(s_staging, 0);
        }
        s_pending = false;
    }
    D3D11_TEXTURE2D_DESC desc{};
    back->GetDesc(&desc);
    const int kind = FormatKind(desc.Format);
    if (kind && desc.SampleDesc.Count == 1 && desc.Width >= 64 && desc.Height >= 64) {
        if (!s_staging || s_format != desc.Format) {
            if (s_staging)
                s_staging->Release();
            s_staging = nullptr;
            D3D11_TEXTURE2D_DESC sd{};
            sd.Width = sd.Height = kSide;
            sd.MipLevels = sd.ArraySize = 1;
            sd.Format = desc.Format;
            sd.SampleDesc.Count = 1;
            sd.Usage = D3D11_USAGE_STAGING;
            sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            if (FAILED(device->CreateTexture2D(&sd, nullptr, &s_staging)))
                s_staging = nullptr;
            s_format = desc.Format;
        }
        if (s_staging) {
            for (UINT gy = 0; gy < kGrid; ++gy)
                for (UINT gx = 0; gx < kGrid; ++gx) {
                    const UINT sx = desc.Width * (1 + 2 * gx) / (2 * kGrid) - kPatch / 2;
                    const UINT sy = desc.Height * (1 + 2 * gy) / (2 * kGrid) - kPatch / 2;
                    const D3D11_BOX box{sx, sy, 0, sx + kPatch, sy + kPatch, 1};
                    ctx->CopySubresourceRegion(s_staging, 0, gx * kPatch, gy * kPatch, 0, back, 0, &box);
                }
            s_kind = kind;
            s_pending = true;
            s_copiedAt = now;
        }
    } else {
        g_brightness.store(-1.0f);  // no reading on this picture (its format, or its samples)
        g_brightTakenAt.store(now);
    }
    ctx->Release();
    device->Release();
}

}  // namespace launchreport

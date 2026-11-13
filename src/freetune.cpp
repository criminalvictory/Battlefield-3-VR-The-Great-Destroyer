// build\freehands.txt, read as hands.cpp reads pouch.txt: checked every 300 ms, read whole when its
// time changes, every key matched at the line's start. A missing file is every default.
#include "freetune.h"

#include <windows.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <iterator>

#include "freetest.h"
#include "log.h"

namespace freetune {
namespace {

struct Values {
    bool on = true;
    int test = 0;
    int unknownTest = 0;  // a "test N" with no such round (it runs none)
    bool rightPose = true;
    bool lineUp = false;
    float shift[3] = {};
    float width = 1.0f, slim = 1.0f;
    float glide = 0.12f;
    float pick = 0.25f, regrip = 0.15f;
    bool everyGun = true;
    bool gunsUnknown = false;  // a "guns" line with neither m416 nor all (every gun stays on)
    bool twoSided = false;
};

Values g_now;
bool g_read = false;  // the first read logs the whole state, later ones what changed

float Clamp(float v, float lo, float hi, float otherwise) {
    return std::isfinite(v) ? std::fmin(std::fmax(v, lo), hi) : otherwise;
}

const char* Skip(const char* s) {
    while (*s == ' ' || *s == '\t')
        ++s;
    return s;
}

void Parse(FILE* f, Values& v) {
    char text[512];
    while (fgets(text, sizeof(text), f)) {
        // A UTF-8 mark (PowerShell writes one) or blanks before the key don't hide it.
        const char* line = text;
        if (static_cast<unsigned char>(line[0]) == 0xEF && static_cast<unsigned char>(line[1]) == 0xBB &&
            static_cast<unsigned char>(line[2]) == 0xBF)
            line += 3;
        line = Skip(line);
        float a[3] = {};
        int n = 0;
        if (std::strncmp(line, "off", 3) == 0)
            v.on = false;
        else if (std::strncmp(line, "test", 4) == 0 && sscanf_s(line + 4, "%d", &n) == 1) {
            const bool known = n > 0 && n < static_cast<int>(std::size(freetest::kRound)) && freetest::kRound[n] > 0.0f;
            v.test = known ? n : 0;
            v.unknownTest = known || n == 0 ? 0 : n;
        } else if (std::strncmp(line, "rightpose", 9) == 0 && sscanf_s(line + 9, "%d", &n) == 1)
            v.rightPose = n != 0;
        else if (std::strncmp(line, "rightwidth", 10) == 0) {
            const int got = sscanf_s(line + 10, "%f %f", &a[0], &a[1]);
            if (got >= 1)
                v.width = Clamp(a[0], 0.8f, 1.5f, 1.0f);
            if (got >= 2)
                v.slim = Clamp(a[1], 0.5f, 1.5f, 1.0f);
        } else if (std::strncmp(line, "lefthold", 8) == 0) {
            const char* rest = Skip(line + 8);
            if (std::strncmp(rest, "lineup", 6) == 0) {
                v.lineUp = true;
                v.shift[0] = v.shift[1] = v.shift[2] = 0.0f;
            } else if (sscanf_s(rest, "%f %f %f", &a[0], &a[1], &a[2]) == 3) {
                v.lineUp = false;
                for (int i = 0; i < 3; ++i)
                    v.shift[i] = Clamp(a[i], -0.3f, 0.3f, 0.0f);
            }
        } else if (std::strncmp(line, "glide", 5) == 0 && sscanf_s(line + 5, "%f", &a[0]) == 1)
            v.glide = Clamp(a[0], 0.02f, 1.0f, 0.12f);
        else if (std::strncmp(line, "reach", 5) == 0) {
            const int got = sscanf_s(line + 5, "%f %f", &a[0], &a[1]);
            if (got >= 1)
                v.pick = Clamp(a[0], 0.02f, 1.0f, 0.25f);
            if (got >= 2)
                v.regrip = Clamp(a[1], 0.02f, 0.5f, 0.15f);
        } else if (std::strncmp(line, "guns", 4) == 0) {
            const char* rest = Skip(line + 4);  // (m416 or M416 alike)
            v.gunsUnknown = false;
            if (_strnicmp(rest, "m416", 4) == 0)
                v.everyGun = false;
            else if (_strnicmp(rest, "all", 3) == 0)
                v.everyGun = true;
            else
                v.gunsUnknown = true;
        } else if (std::strncmp(line, "twosided", 8) == 0 && sscanf_s(line + 8, "%d", &n) == 1)
            v.twoSided = n != 0;
    }
}

// Every line that differs from before (all: the first read, everything that matters).
void LogChanges(const Values& was, const Values& v, bool all) {
    const Values d;
    if (all || v.on != was.on)
        Log(v.on ? "Free hands: on" : "Free hands: off (freehands.txt): the right hand holds the rifle as before");
    if (v.unknownTest && (all || v.unknownTest != was.unknownTest))
        Log("Free hands: no test %d (1 to 10 in freehands.txt): none runs", v.unknownTest);
    if (all || v.test != was.test) {
        if (v.test)
            Log("Free hands: test %d on", v.test);
        else
            Log("Free hands: tests off");
    }
    if ((all && !v.rightPose) || (!all && v.rightPose != was.rightPose))
        Log(v.rightPose ? "Free hands: the free right glove its own, the left's poses flipped (rightpose 1)"
                        : "Free hands: the free right glove the game's grip at its controller (rightpose 0)");
    const bool holdDefault = !v.lineUp && !v.shift[0] && !v.shift[1] && !v.shift[2];
    if ((all && !holdDefault) ||
        (!all && (v.lineUp != was.lineUp || std::memcmp(v.shift, was.shift, sizeof(v.shift)) != 0))) {
        if (v.lineUp)
            Log("Free hands: the rifle in the left hand where the left glove already is (lefthold lineup)");
        else if (holdDefault)
            Log("Free hands: the rifle in the left hand the mirror of the right's hold");
        else
            Log("Free hands: the rifle in the left hand shifted (%.3f %.3f %.3f) m in the left grip's axes", v.shift[0],
                v.shift[1], v.shift[2]);
    }
    if ((all && (v.width != d.width || v.slim != d.slim)) || (!all && (v.width != was.width || v.slim != was.slim)))
        Log("Free hands: the free right glove %.2f wide, its fingers keeping %.2f of their width (rightwidth)", v.width,
            v.slim);
    if ((all && v.glide != d.glide) || (!all && v.glide != was.glide))
        Log("Free hands: a pickup or a regrip glides into the hand in %.2f s", v.glide);
    if ((all && (v.pick != d.pick || v.regrip != d.regrip)) || (!all && (v.pick != was.pick || v.regrip != was.regrip)))
        Log("Free hands: a hand picks the rifle up within %.0f cm of it, regrips it within %.0f cm of its pistol grip",
            v.pick * 100.0f, v.regrip * 100.0f);
    // (Never "Free hands: on" or "off" at their start: the films wait for those words. The last "Free hands: guns:" line is the state.)
    if (v.gunsUnknown && (all || !was.gunsUnknown))
        Log("Free hands: \"guns\" takes m416 or all: every gun stays on");
    if (all || v.everyGun != was.everyGun)
        Log(v.everyGun ? "Free hands: guns: every gun"
                       : "Free hands: guns: the M416 alone (guns m416), every other gun in the right hand as before");
    if ((all && v.twoSided) || (!all && v.twoSided != was.twoSided))
        Log(v.twoSided ? "Free hands: a gun lying drawn with both faces (twosided 1)"
                       : "Free hands: a gun lying drawn with its own faces only (twosided 0)");
}

}  // namespace

void Read() {
    static FILETIME lastWrite{};
    static ULONGLONG lastCheck = 0;
    const ULONGLONG now = GetTickCount64();
    if (now - lastCheck < 300)
        return;
    lastCheck = now;
    wchar_t path[MAX_PATH];
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&Read), &self);
    GetModuleFileNameW(self, path, MAX_PATH);
    if (wchar_t* slash = wcsrchr(path, L'\\'))
        wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path), L"freehands.txt");
    WIN32_FILE_ATTRIBUTE_DATA info{};
    const bool exists = GetFileAttributesExW(path, GetFileExInfoStandard, &info) != 0;
    if (g_read && CompareFileTime(&info.ftLastWriteTime, &lastWrite) == 0)
        return;
    FILE* f = nullptr;
    if (exists && (_wfopen_s(&f, path, L"r") != 0 || !f))
        return;  // still being written (the open isn't shared): read at the next check
    lastWrite = info.ftLastWriteTime;
    Values v;  // every default, then the file's lines
    if (f) {
        Parse(f, v);
        fclose(f);
    }
    LogChanges(g_now, v, !g_read);
    g_now = v;
    g_read = true;
}

bool On() { return g_now.on; }

int Test() { return g_now.test; }

bool RightPose() { return g_now.rightPose; }

bool LeftHold(float shift[3]) {
    for (int i = 0; i < 3; ++i)
        shift[i] = g_now.shift[i];
    return g_now.lineUp;
}

void RightWidth(float& width, float& slim) {
    width = g_now.width;
    slim = g_now.slim;
}

float Glide() { return g_now.glide; }

void Reach(float& pick, float& regrip) {
    pick = g_now.pick;
    regrip = g_now.regrip;
}

bool EveryGun() { return g_now.everyGun; }

bool TwoSided() { return g_now.twoSided; }

}  // namespace freetune

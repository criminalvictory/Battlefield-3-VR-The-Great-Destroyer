// The VR runtime's own recenter followed (src/spacechange.h, header only, as xr_runtime.cpp's PollEvents and RenderFrame run it) on scripted frames
// at 72 Hz, without a runtime.
//   1  LOCAL changed with a time a few frames on: the mod's recenter on the first frame shown at or after that time, once
//   2  LOCAL changed with no time (0): the next frame
//   3  VIEW, STAGE and any other space: nothing, ever
//   4  a time never reached (the runtime's clock off): taken 2 s of display time later at most
//   5  a display clock gone back: taken after 180 frames at most
//   6  two changes before either was taken: one recenter, at the second's time; a change after one taken: another
//   7  no event: never due
//   8  a runtime that keeps sending them (six in a second): the sixth and every one after not followed; six a minute apart: all followed
// Usage: spacechangetest
// The exit code is the number of failed checks.

#include <cstdarg>
#include <cstdint>
#include <cstdio>

#include "../src/spacechange.h"

namespace {

int g_failed = 0, g_counted = 0;

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

constexpr int64_t kFrame = 13888889;  // ns: 72 Hz
// The caller's clock (ms) for each event: a minute apart unless a case says otherwise.
int64_t g_clock = 1000000;
constexpr int64_t kStart = 5000000000000LL;

// Frames from the event on (the event polled before frame 0): the frame the recenter came on (-1 never) and how many came.
struct Run {
    int dueAt = -1, dues = 0;
};

Run Frames(spacechange::State& s, int frames, int64_t firstDisplay, int64_t step) {
    Run r;
    for (int f = 0; f < frames; ++f)
        if (spacechange::Due(s, firstDisplay + step * f)) {
            ++r.dues;
            if (r.dueAt < 0)
                r.dueAt = f;
        }
    return r;
}

}  // namespace

int main() {
    // 1: the change takes 3 frames on.
    {
        spacechange::State s;
        const bool ours = spacechange::Note(s, spacechange::kLocal, kStart + 3 * kFrame, g_clock += 60000);
        const Run r = Frames(s, 300, kStart, kFrame);
        Report("1", "LOCAL changed, its time 3 frames on: recentered on that frame, once", ours && r.dueAt == 3 && r.dues == 1, "frame %d, %d times", r.dueAt,
               r.dues);
    }
    // 2: no time given.
    {
        spacechange::State s;
        spacechange::Note(s, spacechange::kLocal, 0, g_clock += 60000);
        const Run r = Frames(s, 50, kStart, kFrame);
        Report("2", "LOCAL changed with no time: the next frame, once", r.dueAt == 0 && r.dues == 1, "frame %d, %d times", r.dueAt, r.dues);
    }
    // 3: other spaces.
    {
        spacechange::State s;
        const bool view = spacechange::Note(s, spacechange::kView, kStart, g_clock += 60000);
        const bool stage = spacechange::Note(s, spacechange::kStage, kStart, g_clock += 60000);
        const bool floor = spacechange::Note(s, 1000426000, kStart, g_clock += 60000);  // (LOCAL_FLOOR's, an extension's: not one the mod uses)
        const Run r = Frames(s, 500, kStart, kFrame);
        Report("3", "VIEW, STAGE and LOCAL_FLOOR changed: none the mod's, never a recenter", !view && !stage && !floor && r.dues == 0 && s.ignored == 3 &&
               s.count == 0, "%d recenters, %d ignored", r.dues, s.ignored);
    }
    // 4: a time an hour on.
    {
        spacechange::State s;
        spacechange::Note(s, spacechange::kLocal, kStart + 3600LL * 1000000000LL, g_clock += 60000);
        const Run r = Frames(s, 400, kStart, kFrame);
        const int frames2s = static_cast<int>(spacechange::kWaitMost / kFrame) + 1;
        Report("4", "a time never reached (an hour on): taken 2 s of display time on, once", r.dueAt >= frames2s - 1 && r.dueAt <= frames2s + 1 && r.dues == 1,
               "frame %d (2 s is about %d), %d times", r.dueAt, frames2s, r.dues);
    }
    // 5: the display clock going back each frame (a broken clock): taken after 180 frames at most.
    {
        spacechange::State s;
        spacechange::Note(s, spacechange::kLocal, kStart + 10 * kFrame, g_clock += 60000);
        const Run r = Frames(s, 400, kStart, -kFrame);
        Report("5", "a display clock gone back: taken after 180 frames at most, once", r.dueAt == spacechange::kFramesMost - 1 && r.dues == 1,
               "frame %d, %d times", r.dueAt, r.dues);
    }
    // 6: two changes before the first's time came: one recenter, at the second's; then a third after: another.
    {
        spacechange::State s;
        spacechange::Note(s, spacechange::kLocal, kStart + 5 * kFrame, g_clock += 60000);
        const bool before = spacechange::Due(s, kStart);  // (frame 0: not yet)
        spacechange::Note(s, spacechange::kLocal, kStart + 8 * kFrame, g_clock += 60000);
        const Run r = Frames(s, 50, kStart + kFrame, kFrame);  // (frames 1 on)
        spacechange::Note(s, spacechange::kLocal, 0, g_clock += 60000);
        const Run again = Frames(s, 10, kStart + 51 * kFrame, kFrame);
        Report("6", "two changes before either was taken: one recenter at the second's time; another change after: another", !before && r.dues == 1 &&
               r.dueAt + 1 == 8 && again.dues == 1 && s.count == 3, "first at frame %d (%d times), the third's %d times; %d changes", r.dueAt + 1, r.dues,
               again.dues, s.count);
    }
    // 7: nothing noted.
    {
        spacechange::State s;
        const Run r = Frames(s, 1000, kStart, kFrame);
        Report("7", "no event: never a recenter", r.dues == 0, "%d", r.dues);
    }
    // 8: six LOCAL changes in a second, each followed by a frame: five recenters, the sixth flooded, a seventh a minute on not followed either;
    // six a minute apart: six recenters.
    {
        spacechange::State s;
        int dues = 0, taken = 0;
        int64_t display = kStart;
        for (int i = 0; i < 6; ++i) {
            taken += spacechange::Note(s, spacechange::kLocal, 0, 5000000 + 150 * i);
            dues += spacechange::Due(s, display += kFrame);
        }
        const bool later = spacechange::Note(s, spacechange::kLocal, 0, 5000000 + 60000);
        dues += spacechange::Due(s, display += kFrame);
        spacechange::State calm;
        int calmDues = 0;
        for (int i = 0; i < 6; ++i) {
            spacechange::Note(calm, spacechange::kLocal, 0, 6000000 + 60000 * i);
            calmDues += spacechange::Due(calm, display += kFrame);
        }
        Report("8", "six changes in a second: five followed, then none (said); six a minute apart: all six followed", taken == 5 && dues == 5 && !later &&
               s.flooded && calmDues == 6 && !calm.flooded, "%d followed, %d recenters, flooded %d; a minute apart: %d recenters", taken, dues, s.flooded,
               calmDues);
    }
    std::printf("\n%d checks, %d failed\n", g_counted, g_failed);
    return g_failed;
}

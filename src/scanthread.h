#pragma once
// A scan of the game's memory for a gun's data, on a thread of its own (skin.cpp's ClearRifleSpread, FindStinger, ClearLauncherSpread and FindAt4: all
// of the game's read-write pages, 0.6-1.1 s of a core each; StartScan starts one at a time and counts it). Its walk of that memory runs at the lowest
// priority Windows gives a thread for its processor, its memory and its disk (THREAD_MODE_BACKGROUND_BEGIN, from the scan's first line), and only the
// walk: Foreground() as soon as it's over, before any line is logged, any lock is taken or anything is written. The log's lock is held through its
// fflush, which at the background's disk priority took whole timer ticks (15 ms at the 90th percentile of a scan's closing lines, 20 at most, 77 with
// the processor busy), and a frame's thread logging meanwhile, or reading the scatter's rows under the hands' lock (KindDataThere), waited for it: one or
// two late frames at a scan's end (the review of d32a39b, finding 6). Windows, no game (tools/scantest.cpp).

#include <windows.h>

#include <atomic>

struct ScanThread {
    // running: StartScan's count, one less when the scan is over (the guard's end, its thread's last line).
    explicit ScanThread(std::atomic<int>& running) : running_(running) {
        background_ = SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN) || GetLastError() == ERROR_THREAD_MODE_ALREADY_BACKGROUND;
    }
    ScanThread(const ScanThread&) = delete;
    ScanThread& operator=(const ScanThread&) = delete;
    ~ScanThread() {
        Foreground();
        running_.fetch_sub(1, std::memory_order_acq_rel);
    }
    // The walk over, or none to make: the thread's own priorities again (once: a second time is nothing).
    void Foreground() {
        if (background_)
            SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_END);
        background_ = false;
    }
    bool Background() const { return background_; }

private:
    std::atomic<int>& running_;
    bool background_ = false;
};

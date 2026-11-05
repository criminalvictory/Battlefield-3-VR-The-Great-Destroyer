#include "gputime.h"

#include <windows.h>

#include <atomic>
#include <cstring>

#include "log.h"

namespace gputime {

namespace {

constexpr int kSlots = 6;  // frames the card can be behind before one goes untimed

struct Slot {
    ID3D11Query* disjoint = nullptr;
    ID3D11Query* stamps[kMarks] = {};
    bool used[kMarks] = {};
    bool pending = false;
};

Slot g_slots[kSlots];
int g_next = 0;
std::atomic<Slot*> g_open{nullptr};  // this frame's
ID3D11DeviceContext* g_context = nullptr;
bool g_tried = false, g_ok = false;

double g_stretch[kMarks] = {};  // by the mark ending it
double g_whole = 0.0, g_worst = 0.0;
int g_frames = 0;
ULONGLONG g_lastLog = 0;

bool Create(IDXGISwapChain* swap) {
    if (g_tried)
        return g_ok;
    g_tried = true;
    ID3D11Device* device = nullptr;
    if (FAILED(swap->GetDevice(IID_PPV_ARGS(&device))) || !device)
        return false;
    const D3D11_QUERY_DESC disjoint{D3D11_QUERY_TIMESTAMP_DISJOINT, 0}, stamp{D3D11_QUERY_TIMESTAMP, 0};
    g_ok = true;
    for (Slot& s : g_slots) {
        g_ok = g_ok && SUCCEEDED(device->CreateQuery(&disjoint, &s.disjoint));
        for (ID3D11Query*& q : s.stamps)
            g_ok = g_ok && SUCCEEDED(device->CreateQuery(&stamp, &q));
    }
    if (g_ok)
        device->GetImmediateContext(&g_context);  // held: the game's device lives as long as the game
    else
        Log("Graphics card timing: no timestamps on this device");
    device->Release();
    return g_ok;
}

void Collect() {
    for (Slot& s : g_slots) {
        if (!s.pending)
            continue;
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT d;
        if (g_context->GetData(s.disjoint, &d, sizeof(d), D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK)
            continue;
        UINT64 times[kMarks] = {};
        bool ready = true;
        for (int m = 0; m < kMarks && ready; ++m)
            ready = !s.used[m] || g_context->GetData(s.stamps[m], &times[m], sizeof(times[m]), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK;
        if (!ready)
            continue;
        s.pending = false;
        if (d.Disjoint || !d.Frequency)
            continue;
        const double toMs = 1000.0 / static_cast<double>(d.Frequency);
        UINT64 last = times[kStart];
        for (int m = kStart + 1; m < kMarks; ++m) {
            if (!s.used[m] || times[m] < last)
                continue;
            g_stretch[m] += static_cast<double>(times[m] - last) * toMs;
            last = times[m];
        }
        const double whole = static_cast<double>(last - times[kStart]) * toMs;
        g_whole += whole;
        if (whole > g_worst)
            g_worst = whole;
        ++g_frames;
    }
    const ULONGLONG now = GetTickCount64();
    if (now - g_lastLog < 1000 || !g_frames)
        return;
    const double n = g_frames;
    Log("Graphics card timing (ms per frame): left eye %.2f, right eye %.2f, its effects %.2f, HUD %.2f, orange key %.2f, copy to the "
        "headset %.2f, stream view %.2f; whole %.2f, worst %.2f; %d frames",
        g_stretch[kLeftEye] / n, g_stretch[kRightEye] / n, g_stretch[kHudStart] / n, g_stretch[kHudEnd] / n, g_stretch[kKeyDone] / n,
        g_stretch[kSubmitted] / n, g_stretch[kEnd] / n, g_whole / n, g_worst, g_frames);
    std::memset(g_stretch, 0, sizeof(g_stretch));
    g_whole = g_worst = 0.0;
    g_frames = 0;
    g_lastLog = now;
}

}

void FrameEnd() {
    Slot* s = g_open.exchange(nullptr, std::memory_order_acq_rel);
    if (!s)
        return;
    s->used[kEnd] = true;
    g_context->End(s->stamps[kEnd]);
    g_context->End(s->disjoint);
    s->pending = true;
    g_next = (g_next + 1) % kSlots;
}

void FrameStart(IDXGISwapChain* swap) {
    if (!Create(swap))
        return;
    Collect();
    Slot& s = g_slots[g_next];
    if (s.pending)
        return;  // the card this far behind: this frame goes untimed
    std::memset(s.used, 0, sizeof(s.used));
    s.used[kStart] = true;
    g_context->Begin(s.disjoint);
    g_context->End(s.stamps[kStart]);
    g_open.store(&s, std::memory_order_release);
}

void Stamp(Mark mark, ID3D11DeviceContext* context) {
    Slot* s = g_open.load(std::memory_order_acquire);
    if (!s || s->used[mark])
        return;
    s->used[mark] = true;
    (context ? context : g_context)->End(s->stamps[mark]);
}

}

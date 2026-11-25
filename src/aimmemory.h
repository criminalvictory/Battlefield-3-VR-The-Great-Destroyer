#pragma once

// The game's memory as the look angles' reach reads it (aimrules::ReachThrough: aim.cpp's ReachLook), header only, so tools/aimsweeptest.cpp runs the
// very same calls on its own memory: VirtualQuery; the working set (K32QueryWorkingSetEx: a page out of it is never read, so the reach pages nothing of
// the game's back in and makes no page that was never touched); reads under a guard; QueryPerformanceCounter's microseconds; every thread's stack
// left out (AddThreadStacks). Windows only; the search's own thread.

#include <windows.h>
#include <psapi.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

#include "aimrules.h"

namespace aimmemory {

// A read that a page freed meanwhile turns into false (camera::SafeRead's way).
inline bool GuardedRead(uintptr_t address, void* out, size_t size) {
    __try {
        std::memcpy(out, reinterpret_cast<const void*>(address), size);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Every thread's stack in this process, as its allocation's base, added to out: the reach never reads one (a thread's live locals: a nudge there could
// land on a pointer a thread waiting meanwhile still holds; a stack committed whole, as CreateThread commits a stack asked for at the reserve's size,
// has no guard page to be told by). The threads as camera.cpp's GameThreads takes them (NtGetNextThread: a Toolhelp snapshot lists every thread on the
// PC), each one's own TEB by NtQueryInformationThread (ThreadBasicInformation: a 32-bit process's own 32-bit TEB), its stack's top from the TEB's
// first words (NT_TIB: StackBase, StackLimit), its allocation by VirtualQuery. A thread whose stack can't be read this way is left out of the list (the
// guard page still tells most stacks: ReachThrough). Returns how many it added.
inline int AddThreadStacks(std::vector<uintptr_t>& out) {
    using NextThread = LONG(NTAPI*)(HANDLE, HANDLE, ACCESS_MASK, ULONG, ULONG, PHANDLE);
    using QueryThread = LONG(NTAPI*)(HANDLE, ULONG, PVOID, ULONG, PULONG);
    static const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    static const auto next = ntdll ? reinterpret_cast<NextThread>(GetProcAddress(ntdll, "NtGetNextThread")) : nullptr;
    static const auto query = ntdll ? reinterpret_cast<QueryThread>(GetProcAddress(ntdll, "NtQueryInformationThread")) : nullptr;
    if (!next || !query)
        return 0;
    struct BasicInformation {  // THREAD_BASIC_INFORMATION
        LONG exitStatus;
        PVOID teb;
        HANDLE process, thread;  // CLIENT_ID
        ULONG_PTR affinity;
        LONG priority, basePriority;
    };
    int added = 0;
    HANDLE thread = nullptr, following = nullptr;
    for (int n = 0; n < 4096 && next(GetCurrentProcess(), thread, THREAD_QUERY_LIMITED_INFORMATION, 0, 0, &following) >= 0; ++n) {
        if (thread)
            CloseHandle(thread);
        thread = following;
        BasicInformation info{};
        uintptr_t tib[3] = {};  // ExceptionList, StackBase, StackLimit
        if (query(thread, 0 /* ThreadBasicInformation */, &info, sizeof(info), nullptr) < 0 || !info.teb ||
            !GuardedRead(reinterpret_cast<uintptr_t>(info.teb), tib, sizeof(tib)) || tib[1] < 0x10000 + 16 || tib[2] >= tib[1] || tib[1] - tib[2] > 0x10000000)
            continue;
        MEMORY_BASIC_INFORMATION m{};
        if (VirtualQuery(reinterpret_cast<LPCVOID>(tib[1] - 16), &m, sizeof(m)) == sizeof(m) && m.AllocationBase && m.State == MEM_COMMIT &&
            m.Type == MEM_PRIVATE) {
            out.push_back(reinterpret_cast<uintptr_t>(m.AllocationBase));
            ++added;
        }
    }
    if (thread)
        CloseHandle(thread);
    return added;
}

// The memory aimrules::ReachThrough reads. piece: kReachPieceWords words of an allocation of its own; own: allocations never read (that piece's,
// every thread's stack); Start() takes the clock's frequency and K32QueryWorkingSetEx.
struct GameMemory {
    using WorkingSetEx = BOOL(WINAPI*)(HANDLE, PVOID, DWORD);
    uint32_t* piece = nullptr;
    std::vector<uintptr_t> own;
    WorkingSetEx workingSet = nullptr;
    LARGE_INTEGER frequency{};

    void Start() {
        static const auto s_workingSet = reinterpret_cast<WorkingSetEx>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "K32QueryWorkingSetEx"));
        workingSet = s_workingSet;
        QueryPerformanceFrequency(&frequency);
    }
    uint32_t* Piece() const { return piece; }
    bool Query(uintptr_t at, aimrules::Region& r) const {
        MEMORY_BASIC_INFORMATION info{};
        if (VirtualQuery(reinterpret_cast<LPCVOID>(at), &info, sizeof(info)) != sizeof(info))
            return false;
        r.base = reinterpret_cast<uintptr_t>(info.BaseAddress);
        r.end = r.base + info.RegionSize < r.base ? ~static_cast<uintptr_t>(0) : r.base + info.RegionSize;  // (the last region's end)
        r.allocation = reinterpret_cast<uintptr_t>(info.AllocationBase);
        r.state = info.State, r.type = info.Type, r.protect = info.Protect;
        return true;
    }
    bool Own(uintptr_t allocation) const { return std::find(own.begin(), own.end(), allocation) != own.end(); }
    bool Resident(uintptr_t page, int count, uint8_t* valid) const {
        PSAPI_WORKING_SET_EX_INFORMATION info[24] = {};
        if (!workingSet || count <= 0 || count > 24)
            return false;
        for (int k = 0; k < count; ++k)
            info[k].VirtualAddress = reinterpret_cast<PVOID>(page + static_cast<uintptr_t>(k) * 0x1000);
        if (!workingSet(GetCurrentProcess(), info, static_cast<DWORD>(count * sizeof(info[0]))))
            return false;
        for (int k = 0; k < count; ++k)
            valid[k] = info[k].VirtualAttributes.Valid ? 1 : 0;
        return true;
    }
    bool Read(uintptr_t from, uint32_t* to, size_t words) const { return GuardedRead(from, to, words * 4); }
    uint64_t Micros() const {
        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        const uint64_t ticks = static_cast<uint64_t>(now.QuadPart), each = static_cast<uint64_t>(frequency.QuadPart > 0 ? frequency.QuadPart : 1);
        return ticks / each * 1000000 + ticks % each * 1000000 / each;
    }
};

// Whether a page is in this process's working set now (tools: that the reach left one out of it as it was).
inline bool InWorkingSet(uintptr_t page) {
    GameMemory m;
    m.Start();
    uint8_t valid = 0;
    return m.Resident(page & ~static_cast<uintptr_t>(0xFFF), 1, &valid) && valid;
}

}  // namespace aimmemory

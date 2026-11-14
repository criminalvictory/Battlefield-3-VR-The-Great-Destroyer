#pragma once

// Another process's command line, read from outside it (injector/main.cpp, tools/campaigntest.cpp): the way the injector
// tells the campaign from a multiplayer or co-op start before it loads anything (src/campaign.h). For a 32-bit process from a
// 32-bit one: out of its 32-bit PEB (the process parameters' CommandLine), or, if that read fails, by asking the system for it
// (ProcessCommandLineInformation, Windows 8.1 and later). Needs PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ.

#include <windows.h>

#include <string>
#include <vector>

namespace cmdline {

namespace detail {

using QueryInformation = LONG(WINAPI*)(HANDLE, ULONG, PVOID, ULONG, PULONG);

inline QueryInformation Query() {
    static const QueryInformation query =
        reinterpret_cast<QueryInformation>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQueryInformationProcess"));
    return query;
}

constexpr ULONG kProcessBasicInformation = 0, kProcessWow64Information = 26, kProcessCommandLineInformation = 60;
constexpr LONG kInfoLengthMismatch = static_cast<LONG>(0xC0000004L);

// The 32-bit layouts: the PEB's process parameters at +10h, their CommandLine (a UNICODE_STRING: length in bytes, the
// largest length, the buffer) at +40h.
constexpr DWORD kPebParameters = 0x10, kParametersCommandLine = 0x40;

inline bool Read(HANDLE process, DWORD address, void* out, SIZE_T size) {
    SIZE_T got = 0;
    return address && ReadProcessMemory(process, reinterpret_cast<LPCVOID>(static_cast<ULONG_PTR>(address)), out, size, &got) && got == size;
}

}  // namespace detail

// Out of the process's 32-bit PEB. Only from a 32-bit build (a 64-bit one would need the other layout); error: why not.
inline bool ReadByPeb(HANDLE process, std::wstring& line, DWORD& error) {
    line.clear();
    error = 0;
#if defined(_WIN64)
    error = ERROR_NOT_SUPPORTED;
    return false;
#else
    const detail::QueryInformation query = detail::Query();
    if (!query) {
        error = ERROR_PROC_NOT_FOUND;
        return false;
    }
    // On 64-bit Windows a 32-bit process has its own 32-bit PEB, which ProcessWow64Information gives; on 32-bit Windows the
    // only PEB is the basic information's.
    BOOL wow64 = FALSE;
    if (!IsWow64Process(process, &wow64)) {
        error = GetLastError();
        return false;
    }
    DWORD peb = 0;
    if (wow64) {
        ULONG_PTR peb32 = 0;
        if (query(process, detail::kProcessWow64Information, &peb32, sizeof(peb32), nullptr) < 0 || !peb32) {
            error = ERROR_INVALID_DATA;
            return false;
        }
        peb = static_cast<DWORD>(peb32);
    } else {
        struct {
            LONG exitStatus;
            DWORD peb;
            ULONG_PTR affinity;
            LONG priority;
            ULONG_PTR id, parent;
        } basic{};
        if (query(process, detail::kProcessBasicInformation, &basic, sizeof(basic), nullptr) < 0 || !basic.peb) {
            error = ERROR_INVALID_DATA;
            return false;
        }
        peb = basic.peb;
    }
    DWORD parameters = 0;
    struct {
        USHORT length, largest;
        DWORD buffer;
    } text{};
    if (!detail::Read(process, peb + detail::kPebParameters, &parameters, sizeof(parameters)) ||
        !detail::Read(process, parameters + detail::kParametersCommandLine, &text, sizeof(text))) {
        error = GetLastError() ? GetLastError() : ERROR_PARTIAL_COPY;
        return false;
    }
    if ((text.length & 1) || text.length > text.largest || (text.length && !text.buffer)) {
        error = ERROR_INVALID_DATA;
        return false;
    }
    line.resize(text.length / sizeof(wchar_t));
    if (text.length && !detail::Read(process, text.buffer, line.data(), text.length)) {
        error = GetLastError() ? GetLastError() : ERROR_PARTIAL_COPY;
        line.clear();
        return false;
    }
    return true;
#endif
}

// Asked of the system (Windows 8.1 and later). error: why not.
inline bool ReadByQuery(HANDLE process, std::wstring& line, DWORD& error) {
    line.clear();
    error = 0;
    const detail::QueryInformation query = detail::Query();
    if (!query) {
        error = ERROR_PROC_NOT_FOUND;
        return false;
    }
    // What comes back: the text's header (a UNICODE_STRING in this build's own layout), the text after it in the same buffer.
    struct Text {
        USHORT length, largest;
        const wchar_t* buffer;
    };
    ULONG size = 0;
    LONG status = query(process, detail::kProcessCommandLineInformation, nullptr, 0, &size);
    if (status != detail::kInfoLengthMismatch || size < sizeof(Text) || size > (1u << 20)) {
        error = status < 0 && status != detail::kInfoLengthMismatch ? static_cast<DWORD>(status) : ERROR_INVALID_DATA;
        return false;
    }
    std::vector<BYTE> buffer(size);
    status = query(process, detail::kProcessCommandLineInformation, buffer.data(), size, &size);
    if (status < 0) {
        error = static_cast<DWORD>(status);
        return false;
    }
    const auto* text = reinterpret_cast<const Text*>(buffer.data());
    const auto* begin = reinterpret_cast<const BYTE*>(text->buffer);
    // (A header in another layout than this build's puts its pointer elsewhere: refused here, never read past the buffer.)
    if ((text->length & 1) || (text->length && (!begin || begin < buffer.data() || begin + text->length > buffer.data() + buffer.size()))) {
        error = ERROR_INVALID_DATA;
        return false;
    }
    line.assign(text->buffer ? text->buffer : L"", text->length / sizeof(wchar_t));
    return true;
}

// The command line, by the PEB first, then by asking. how: which way it was read ("PEB", "query"), for the log.
inline bool ReadCommandLine(HANDLE process, std::wstring& line, DWORD& error, const char** how = nullptr) {
    if (ReadByPeb(process, line, error)) {
        if (how)
            *how = "PEB";
        return true;
    }
    const DWORD pebError = error;
    if (ReadByQuery(process, line, error)) {
        if (how)
            *how = "query";
        return true;
    }
    if (!error)
        error = pebError;
    return false;
}

}  // namespace cmdline

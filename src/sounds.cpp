#include "sounds.h"

#include <windows.h>
#include <audiopolicy.h>
#include <mmdeviceapi.h>
#include <xaudio2.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "log.h"

namespace {

struct Clip {
    WAVEFORMATEX format{};
    std::shared_ptr<std::vector<BYTE>> data;  // kept by each voice playing it, past a reload
    FILETIME written{};
};

struct Voice {
    IXAudio2SourceVoice* voice;
    std::shared_ptr<std::vector<BYTE>> data;
};

// An output as Windows names it: its id and its name ("Headphones (Oculus Virtual Audio Device)").
struct Output {
    std::wstring id;
    std::string name;
};

std::mutex g_mutex;  // the queue
std::condition_variable g_wake;
std::deque<std::pair<std::string, float>> g_queue;  // what to play, and where
std::atomic<void (*)(const char*)> g_say{nullptr};  // the launch report's line (Begin)
// The rest is the sounds' thread's own.
IXAudio2* g_audio = nullptr;
IXAudio2MasteringVoice* g_master = nullptr;
IMMDeviceEnumerator* g_enumerator = nullptr;
UINT32 g_outputs = 2;
std::wstring g_device;     // the output the sounds play on (its id); empty: XAudio2's own default
ULONGLONG g_begun = 0;     // when the sounds' thread started
ULONGLONG g_lookedAt = 0;  // when it last looked where the game plays
ULONGLONG g_failedAt = 0;  // when the sounds last couldn't start (tried again 10 s later)
int g_failures = 0;
int g_said = 0;            // the launch report's lines said
std::map<std::string, Clip> g_clips;
std::vector<Voice> g_voices;  // playing or done, destroyed once done

// XAudio2's word that its output went away (unplugged, a headset's link dropped): the sounds start again.
struct Engine : IXAudio2EngineCallback {
    std::atomic<bool> lost{false};
    void STDMETHODCALLTYPE OnProcessingPassStart() override {}
    void STDMETHODCALLTYPE OnProcessingPassEnd() override {}
    void STDMETHODCALLTYPE OnCriticalError(HRESULT) override { lost = true; }
} g_engine;

std::wstring ModFolder() {
    wchar_t path[MAX_PATH];
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&ModFolder), &self);
    GetModuleFileNameW(self, path, MAX_PATH);
    std::wstring p = path;
    p.resize(p.find_last_of(L'\\') + 1);
    return p;
}

std::wstring ClipPath(const char* name) {
    std::wstring p = ModFolder() + L"sounds\\";
    for (const char* c = name; *c; ++c)
        p += static_cast<wchar_t>(*c);
    return p + L".wav";
}

std::string Utf8(const wchar_t* text) {
    std::string out;
    const int n = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (n > 1) {
        out.resize(n - 1);
        WideCharToMultiByte(CP_UTF8, 0, text, -1, out.data(), n, nullptr, nullptr);
    }
    return out;
}

// A plain PCM wave file (what capture cut writes).
bool Load(const std::wstring& path, Clip& clip) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"rb") || !f)
        return false;
    std::vector<BYTE> all;
    BYTE buffer[65536];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), f)) > 0)
        all.insert(all.end(), buffer, buffer + n);
    fclose(f);
    if (all.size() < 12 || std::memcmp(all.data(), "RIFF", 4) || std::memcmp(all.data() + 8, "WAVE", 4))
        return false;
    bool haveFormat = false;
    for (size_t at = 12; at + 8 <= all.size();) {
        DWORD size = 0;
        std::memcpy(&size, all.data() + at + 4, 4);
        size = static_cast<DWORD>(std::min<size_t>(size, all.size() - (at + 8)));
        if (!std::memcmp(all.data() + at, "fmt ", 4) && size >= 16) {
            std::memcpy(&clip.format, all.data() + at + 8, 16);
            clip.format.cbSize = 0;
            haveFormat = clip.format.wFormatTag == WAVE_FORMAT_PCM;
        } else if (!std::memcmp(all.data() + at, "data", 4) && haveFormat) {
            clip.data = std::make_shared<std::vector<BYTE>>(all.data() + at + 8, all.data() + at + 8 + size);
            return !clip.data->empty();
        }
        at += 8 + size + (size & 1);
    }
    return false;
}

std::string NameOf(IMMDevice* device) {
    // PKEY_Device_FriendlyName.
    static const PROPERTYKEY kName = {{0xa45c254e, 0xdf1c, 0x4efd, {0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0}}, 14};
    std::string name;
    IPropertyStore* props = nullptr;
    if (FAILED(device->OpenPropertyStore(STGM_READ, &props)))
        return name;
    PROPVARIANT v;
    PropVariantInit(&v);
    if (SUCCEEDED(props->GetValue(kName, &v)) && v.vt == VT_LPWSTR && v.pwszVal)
        name = Utf8(v.pwszVal);
    PropVariantClear(&v);
    props->Release();
    return name;
}

// Windows' default output for games and media (eConsole).
Output DefaultOutput() {
    Output out;
    IMMDevice* endpoint = nullptr;
    LPWSTR id = nullptr;
    if (g_enumerator && SUCCEEDED(g_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &endpoint)) &&
        SUCCEEDED(endpoint->GetId(&id))) {
        out.id = id;
        CoTaskMemFree(id);
        out.name = NameOf(endpoint);
    }
    if (endpoint)
        endpoint->Release();
    return out;
}

// Where the game plays: the outputs with this process's sound running on them (the game's, and once they play the mod's
// own sounds'), Windows' default output for games first among them. (The mod's sounds went to Windows' default output for
// games as it was at their first play; where the game played elsewhere they weren't heard: a Quest player's video,
// 2026-11-17.)
std::vector<Output> Playing(const std::wstring& defaultId) {
    std::vector<Output> found;
    IMMDeviceCollection* list = nullptr;
    if (!g_enumerator || FAILED(g_enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &list)))
        return found;
    UINT count = 0;
    list->GetCount(&count);
    const DWORD self = GetCurrentProcessId();
    for (UINT i = 0; i < count; ++i) {
        IMMDevice* device = nullptr;
        if (FAILED(list->Item(i, &device)))
            continue;
        bool ours = false;
        IAudioSessionManager2* manager = nullptr;
        IAudioSessionEnumerator* sessions = nullptr;
        if (SUCCEEDED(device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr,
                                       reinterpret_cast<void**>(&manager))) &&
            SUCCEEDED(manager->GetSessionEnumerator(&sessions))) {
            int n = 0;
            sessions->GetCount(&n);
            for (int k = 0; k < n && !ours; ++k) {
                IAudioSessionControl* session = nullptr;
                if (FAILED(sessions->GetSession(k, &session)))
                    continue;
                AudioSessionState state = AudioSessionStateInactive;
                IAudioSessionControl2* session2 = nullptr;
                if (SUCCEEDED(session->GetState(&state)) && state == AudioSessionStateActive &&
                    SUCCEEDED(session->QueryInterface(__uuidof(IAudioSessionControl2),
                                                      reinterpret_cast<void**>(&session2)))) {
                    DWORD pid = 0;
                    ours = session2->GetProcessId(&pid) == S_OK && pid == self;
                    session2->Release();
                }
                session->Release();
            }
        }
        if (sessions)
            sessions->Release();
        if (manager)
            manager->Release();
        LPWSTR id = nullptr;
        if (ours && SUCCEEDED(device->GetId(&id))) {
            Output out{id, NameOf(device)};
            CoTaskMemFree(id);
            if (out.id == defaultId)
                found.insert(found.begin(), std::move(out));
            else
                found.push_back(std::move(out));
        }
        device->Release();
    }
    list->Release();
    return found;
}

std::string Shown(const Output& o) { return o.name.empty() ? std::string("an output with no name") : o.name; }

// The sounds' output let go (the voices, the engine): the next play, or the next look, starts them again.
void Stop() {
    for (Voice& v : g_voices)
        v.voice->DestroyVoice();
    g_voices.clear();
    if (g_master) {
        g_master->DestroyVoice();
        g_master = nullptr;
    }
    if (g_audio) {
        g_audio->UnregisterForCallbacks(&g_engine);
        g_audio->Release();
        g_audio = nullptr;
    }
    g_engine.lost = false;
}

// On the sounds' own thread, which has COM (XAudio2 finds the output through it) in the
// multithreaded model: on a game thread, whatever model the game chose there, the voices started
// but nothing was heard (2026-10-30). On `want`, or else where the game plays, or else Windows'
// default output for games (the game's sound not found).
bool Start(const Output* want = nullptr) {
    if (g_audio)
        return true;
    if (g_failures > 0 && GetTickCount64() - g_failedAt < 10000)
        return false;  // tried again 10 s after it failed
    auto failed = [](const char* what, HRESULT hr) {
        if (++g_failures <= 3)
            Log("Sounds: %s (%08lX)%s", what, static_cast<unsigned long>(hr), g_failures == 3 ? "; not logged again" : "");
        g_failedAt = GetTickCount64();
        return false;
    };
    HRESULT hr = XAudio2Create(&g_audio, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr)) {
        g_audio = nullptr;
        return failed("XAudio2 didn't start", hr);
    }
    Output where;
    const char* why = "XAudio2's default output";
    if (want) {
        where = *want;
        why = "where the game plays";
    } else {
        const Output fallback = DefaultOutput();
        const std::vector<Output> playing = Playing(fallback.id);
        if (!playing.empty()) {
            where = playing.front();
            why = "where the game plays";
        } else if (!fallback.id.empty()) {
            where = fallback;
            why = "Windows' default output for games (the game's own sound not found)";
        }
    }
    hr = g_audio->CreateMasteringVoice(&g_master, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE, 0,
                                       where.id.empty() ? nullptr : where.id.c_str());
    if (FAILED(hr)) {
        g_audio->Release();
        g_audio = nullptr;
        return failed("no output for XAudio2", hr);
    }
    g_engine.lost = false;
    g_audio->RegisterForCallbacks(&g_engine);
    g_device = where.id;
    g_lookedAt = GetTickCount64();
    XAUDIO2_VOICE_DETAILS details{};
    g_master->GetVoiceDetails(&details);
    g_outputs = details.InputChannels;
    g_failures = 0;
    const std::string on = where.id.empty() ? std::string(why) : Shown(where) + ", " + why;
    Log("Sounds: ready (%u output channels, %u Hz) on %s", g_outputs, details.InputSampleRate, on.c_str());
    if (auto say = g_say.load(); say && g_said < 4) {
        ++g_said;
        char line[512];
        std::snprintf(line, sizeof(line), "the mod's sounds play on %s", on.c_str());
        say(line);
    }
    return true;
}

// With nothing to play (every 2 s): before the sounds first play (from Begin), they start as soon as the game's own sound is
// found, or after a minute on Windows' default output for games, so the launch report says where; after, every 5 s, they
// follow the game's sound to another output, and start again if theirs went away.
void Follow() {
    if (g_audio && g_engine.lost) {
        Log("Sounds: their output went away (XAudio2's critical error): started again");
        Stop();
    }
    const ULONGLONG now = GetTickCount64();
    if (g_audio && now - g_lookedAt < 5000)
        return;
    g_lookedAt = now;
    const std::vector<Output> playing = Playing(DefaultOutput().id);
    if (!g_audio) {
        if (!playing.empty())
            Start(&playing.front());
        else if (now - g_begun > 60000)
            Start();
        return;
    }
    // Started: an output other than theirs with this process's sound on it is the game's.
    for (const Output& o : playing) {
        if (o.id != g_device) {
            Log("Sounds: the game plays on %s: the mod's sounds move there", Shown(o).c_str());
            Stop();
            Start(&o);
            break;
        }
    }
}

void PlayNow(const char* name, float pan) {
    if (g_audio && g_engine.lost) {
        Log("Sounds: their output went away (XAudio2's critical error): started again");
        Stop();
    }
    if (!Start())
        return;
    // Done voices go first.
    for (auto it = g_voices.begin(); it != g_voices.end();) {
        XAUDIO2_VOICE_STATE state{};
        it->voice->GetState(&state);
        if (state.BuffersQueued == 0) {
            it->voice->DestroyVoice();
            it = g_voices.erase(it);
        } else {
            ++it;
        }
    }
    const std::wstring path = ClipPath(name);
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &info))
        return;
    Clip& clip = g_clips[name];
    if (CompareFileTime(&info.ftLastWriteTime, &clip.written) != 0) {
        clip = Clip{};
        if (!Load(path, clip)) {
            Log("Sounds: %s.wav can't be read (16-bit PCM wanted)", name);
            return;
        }
        clip.written = info.ftLastWriteTime;
        Log("Sounds: %s.wav loaded, %.2f s", name,
            static_cast<double>(clip.data->size()) / clip.format.nAvgBytesPerSec);
    }
    if (!clip.data || clip.data->empty() || g_voices.size() > 16)
        return;
    IXAudio2SourceVoice* voice = nullptr;
    if (HRESULT hr = g_audio->CreateSourceVoice(&voice, &clip.format); FAILED(hr)) {
        Log("Sounds: %s can't play (%08lX)", name, static_cast<unsigned long>(hr));
        return;
    }
    // Equal-power pan of the clip (mono or each channel the same) between the first two outputs,
    // as loud in the middle as the game played it (the clip is its two channels averaged).
    const float angle = (std::clamp(pan, -1.0f, 1.0f) + 1.0f) * 0.25f * 3.14159265f;
    constexpr float kMiddle = 1.41421356f;
    const UINT32 inputs = clip.format.nChannels;
    std::vector<float> matrix(inputs * g_outputs, 0.0f);
    for (UINT32 i = 0; i < inputs; ++i) {
        matrix[0 * inputs + i] = kMiddle * std::cos(angle) / inputs;
        if (g_outputs > 1)
            matrix[1 * inputs + i] = kMiddle * std::sin(angle) / inputs;
    }
    voice->SetOutputMatrix(g_master, inputs, g_outputs, matrix.data());
    XAUDIO2_BUFFER buffer{};
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.AudioBytes = static_cast<UINT32>(clip.data->size());
    buffer.pAudioData = clip.data->data();
    if (FAILED(voice->SubmitSourceBuffer(&buffer)) || FAILED(voice->Start())) {
        Log("Sounds: %s didn't start", name);
        voice->DestroyVoice();
        return;
    }
    g_voices.push_back({voice, clip.data});
    // The first few plays: is the engine running it? (In the game nothing was heard, 2026-10-30.)
    static int s_checks = 6;
    if (s_checks > 0) {
        --s_checks;
        Sleep(60);
        XAUDIO2_VOICE_STATE state{};
        voice->GetState(&state);
        XAUDIO2_PERFORMANCE_DATA perf{};
        g_audio->GetPerformanceData(&perf);
        Log("Sounds: %s after 60 ms: %llu samples played, %u voices active, latency %u samples, %u glitches", name,
            static_cast<unsigned long long>(state.SamplesPlayed), perf.ActiveSourceVoiceCount,
            perf.CurrentLatencyInSamples, perf.GlitchesSinceEngineStarted);
    }
}

// The sounds' thread: plays what Play asked for, in order; between plays it keeps them where the game plays.
void Worker() {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                     reinterpret_cast<void**>(&g_enumerator));
    g_begun = GetTickCount64();
    for (;;) {
        std::pair<std::string, float> next;
        bool play = false;
        {
            std::unique_lock lock(g_mutex);
            play = g_wake.wait_for(lock, std::chrono::seconds(2), [] { return !g_queue.empty(); });
            if (play) {
                next = std::move(g_queue.front());
                g_queue.pop_front();
            }
        }
        if (play)
            PlayNow(next.first.c_str(), next.second);
        else
            Follow();
    }
}

void Run() {
    static std::once_flag started;
    std::call_once(started, [] { std::thread(Worker).detach(); });
}

}

namespace sounds {

bool Have(const char* name) {
    WIN32_FILE_ATTRIBUTE_DATA info;
    return GetFileAttributesExW(ClipPath(name).c_str(), GetFileExInfoStandard, &info) != 0;
}

void Play(const char* name, float pan) {
    Run();
    {
        std::lock_guard lock(g_mutex);
        if (g_queue.size() < 16)
            g_queue.emplace_back(name, pan);
    }
    g_wake.notify_one();
}

void Begin(void (*say)(const char*)) {
    g_say = say;
    Run();
}

int Count() {
    WIN32_FIND_DATAW found;
    const HANDLE h = FindFirstFileW((ModFolder() + L"sounds\\*.wav").c_str(), &found);
    if (h == INVALID_HANDLE_VALUE)
        return 0;
    int n = 0;
    do {
        if (!(found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            ++n;
    } while (FindNextFileW(h, &found));
    FindClose(h);
    return n;
}

}

namespace {

// build\gamesounds.exe started hidden at idle priority with these arguments, its words into `out` (a file, or a pipe's
// writing end: the only handle handed to it, none of the game's own). Its process, or null.
HANDLE RunCutter(const std::wstring& arguments, HANDLE out, DWORD& error) {
    const std::wstring dir = ModFolder();
    const std::wstring exe = dir + L"gamesounds.exe";
    std::wstring command = L"\"" + exe + L"\" " + arguments;
    SIZE_T size = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
    std::vector<BYTE> attributes(size ? size : 1);
    auto list = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());
    const bool handOver = out != INVALID_HANDLE_VALUE && size && InitializeProcThreadAttributeList(list, 1, 0, &size) &&
                          UpdateProcThreadAttribute(list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, &out, sizeof(out), nullptr,
                                                    nullptr);
    STARTUPINFOEXW start{};
    start.StartupInfo.cb = handOver ? sizeof(STARTUPINFOEXW) : sizeof(STARTUPINFOW);
    if (handOver) {
        start.lpAttributeList = list;
        start.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
        start.StartupInfo.hStdOutput = out;
        start.StartupInfo.hStdError = out;
    }
    PROCESS_INFORMATION process{};
    const BOOL ok = CreateProcessW(exe.c_str(), command.data(), nullptr, nullptr, handOver,
                                   CREATE_NO_WINDOW | IDLE_PRIORITY_CLASS | (handOver ? EXTENDED_STARTUPINFO_PRESENT : 0),
                                   nullptr, dir.c_str(), &start.StartupInfo, &process);
    error = GetLastError();
    if (handOver)
        DeleteProcThreadAttributeList(list);
    if (!ok)
        return nullptr;
    CloseHandle(process.hThread);
    return process.hProcess;
}

// The name of every clip the mod plays, as build\gamesounds.exe --list says them (its own table: the game isn't read).
// Empty when it couldn't be asked (`error` then says why, 0 for a gamesounds.exe that knows no --list).
std::vector<std::string> CutterList(DWORD& error) {
    std::vector<std::string> names;
    error = 0;
    SECURITY_ATTRIBUTES inherit{sizeof(inherit), nullptr, TRUE};
    HANDLE read = nullptr, write = nullptr;
    if (!CreatePipe(&read, &write, &inherit, 1 << 16)) {
        error = GetLastError();
        return names;
    }
    SetHandleInformation(read, HANDLE_FLAG_INHERIT, 0);
    const HANDLE process = RunCutter(L"--list", write, error);
    CloseHandle(write);
    if (!process) {
        CloseHandle(read);
        return names;
    }
    error = 0;
    std::string all;
    char buffer[4096];
    DWORD n = 0;
    while (all.size() < (1u << 20) && ReadFile(read, buffer, sizeof(buffer), &n, nullptr) && n)
        all.append(buffer, n);
    CloseHandle(read);
    DWORD code = 1;
    if (WaitForSingleObject(process, 30000) != WAIT_OBJECT_0)
        TerminateProcess(process, 1);
    GetExitCodeProcess(process, &code);
    CloseHandle(process);
    if (code != 0)
        return names;  // (an older gamesounds.exe goes on to look for the game: not a list)
    for (size_t at = 0; at < all.size();) {
        size_t end = all.find('\n', at);
        if (end == std::string::npos)
            end = all.size();
        std::string name = all.substr(at, end - at);
        while (!name.empty() && (name.back() == '\r' || name.back() == ' '))
            name.pop_back();
        // A clip's name and nothing else: letters, digits, _ and - (anything else isn't --list's answer).
        const bool plain = !name.empty() && name.size() < 64 && std::all_of(name.begin(), name.end(), [](char c) {
            return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
        });
        if (!plain)
            return {};
        names.push_back(std::move(name));
        at = end + 1;
    }
    return names;
}

// A clip's file is there (and more than a wave file's header: gamesounds --missing's own rule).
bool There(const std::string& name) {
    WIN32_FILE_ATTRIBUTE_DATA info;
    return GetFileAttributesExW(ClipPath(name.c_str()).c_str(), GetFileExInfoStandard, &info) &&
           !(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (info.nFileSizeHigh || info.nFileSizeLow > 44);
}

// "a, b, c and 4 more".
std::string Named(const std::vector<std::string>& names) {
    std::string out;
    const size_t shown = std::min<size_t>(names.size(), 8);
    for (size_t i = 0; i < shown; ++i)
        out += (i ? ", " : "") + names[i];
    if (names.size() > shown)
        out += " and " + std::to_string(names.size() - shown) + " more";
    return out;
}

void Tell(void (*to)(const char*), const char* format, ...) {
    char text[1024];
    va_list args;
    va_start(args, format);
    std::vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    if (to)
        to(text);  // (the launch report's line: in the mod's log too)
    else
        Log("Sounds: %s", text);
}

void MakeMissingNow(void (*line)(const char*), void (*warn)(const char*), std::wstring game) {
    const std::wstring dir = ModFolder();
    const int have = sounds::Count();
    if (GetFileAttributesW((dir + L"gamesounds.exe").c_str()) == INVALID_FILE_ATTRIBUTES) {
        if (have > 0)
            Tell(line, "the mod's sounds: %d in build\\sounds (build\\gamesounds.exe, which makes a newer version's from your "
                       "game, isn't there: not checked for new ones)", have);
        else
            Tell(warn, "the mod's sounds aren't there (build\\sounds is empty) and build\\gamesounds.exe, which makes them from "
                       "your game, is missing (an antivirus may have removed it): the reloads by hand are silent. Unzip the "
                       "release again");
        return;
    }
    DWORD error = 0;
    const std::vector<std::string> all = CutterList(error);
    std::vector<std::string> missing;
    for (const std::string& name : all)
        if (!There(name))
            missing.push_back(name);
    if (all.empty() && error) {
        if (have > 0)
            Tell(line, "the mod's sounds: %d in build\\sounds (build\\gamesounds.exe couldn't be started to check for a newer "
                       "version's, error %lu: an antivirus may block it)", have, error);
        else
            Tell(warn, "the mod's sounds aren't there (build\\sounds is empty) and build\\gamesounds.exe couldn't be started to "
                       "make them (error %lu: an antivirus may block it): the reloads by hand are silent", error);
        return;
    }
    // (No list from a gamesounds.exe older than the DLL: the rule it was made for, all of them when there are none.)
    const bool whole = all.empty() ? have == 0 : missing.size() == all.size();
    if (!whole && missing.empty()) {
        if (all.empty())
            Tell(line, "the mod's sounds: %d in build\\sounds", have);
        else
            Tell(line, "the mod's sounds: all %d in build\\sounds", static_cast<int>(all.size()));
        return;
    }
    // Its words go to build\gamesounds.log.
    SECURITY_ATTRIBUTES inherit{sizeof(inherit), nullptr, TRUE};
    const HANDLE log = CreateFileW((dir + L"gamesounds.log").c_str(), GENERIC_WRITE, FILE_SHARE_READ, &inherit, CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL, nullptr);
    const HANDLE process = RunCutter(std::wstring(all.empty() ? L"--mod" : L"--missing") + L" --game \"" + game + L"\"", log, error);
    if (log != INVALID_HANDLE_VALUE)
        CloseHandle(log);
    if (!process) {
        Tell(warn, "%s and build\\gamesounds.exe couldn't be started to make them (error %lu: an antivirus may block it): %s",
             whole ? "the mod's sounds aren't there (build\\sounds is empty)"
                   : (std::to_string(missing.size()) + " of the mod's sounds aren't in build\\sounds yet (" + Named(missing) + ")").c_str(),
             error, whole ? "the reloads by hand are silent" : "those are silent, the rest play");
        return;
    }
    if (whole)
        Tell(line, "the mod's sounds weren't made yet (the game wasn't started with Start BF3 VR.bat?): build\\gamesounds.exe is "
                   "making them now from your game's own files in %s (about a minute; build\\gamesounds.log); until then the "
                   "reloads by hand are silent", Utf8(game.c_str()).c_str());
    else
        Tell(line, "the mod's sounds: %d of %d aren't in build\\sounds yet, new with this version (%s): build\\gamesounds.exe is "
                   "making only those now from your game's own files in %s (a few seconds to half a minute; "
                   "build\\gamesounds.log); the %d there are left as they are, and until then only the new ones are silent",
             static_cast<int>(missing.size()), static_cast<int>(all.size()), Named(missing).c_str(), Utf8(game.c_str()).c_str(),
             static_cast<int>(all.size() - missing.size()));
    const ULONGLONG began = GetTickCount64();
    const bool ended = WaitForSingleObject(process, 600000) == WAIT_OBJECT_0;
    if (!ended)
        TerminateProcess(process, 1);
    DWORD code = 1;
    GetExitCodeProcess(process, &code);
    CloseHandle(process);
    const double took = static_cast<double>(GetTickCount64() - began) / 1000.0;
    if (all.empty()) {
        const int now = sounds::Count();
        if (ended && code == 0)
            Tell(line, "the mod's sounds: %d made from your game's own files in %.0f s", now, took);
        else
            Tell(warn, "the mod's sounds couldn't all be made (%d made; build\\gamesounds.log says why): the reloads by hand "
                       "without one are silent", now);
        return;
    }
    std::vector<std::string> made, still;
    for (const std::string& name : missing)
        (There(name) ? made : still).push_back(name);
    if (!made.empty())
        Tell(line, "the mod's sounds: %d made from your game's own files in %.0f s (%s); %d of %d in build\\sounds now",
             static_cast<int>(made.size()), took, Named(made).c_str(), static_cast<int>(all.size() - still.size()),
             static_cast<int>(all.size()));
    if (!still.empty())
        Tell(warn, "%d of the mod's sounds couldn't be made (%s; %s; build\\gamesounds.log says why): those are silent. This is "
                   "tried again at the next start",
             static_cast<int>(still.size()), Named(still).c_str(),
             ended ? ("build\\gamesounds.exe ended with " + std::to_string(code)).c_str() : "stopped after 10 minutes");
}

}

namespace sounds {

void MakeMissing(void (*line)(const char*), void (*warn)(const char*), const wchar_t* gameDir) {
    std::wstring game;
    if (gameDir) {
        game = gameDir;
    } else {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        game = path;
        game.resize(game.find_last_of(L'\\'));
    }
    std::thread(MakeMissingNow, line, warn, std::move(game)).detach();
}

}

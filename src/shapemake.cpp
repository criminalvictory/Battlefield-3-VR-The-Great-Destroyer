#include "shapemake.h"

#include <windows.h>

#include <d3d11.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <mutex>
#include <system_error>
#include <thread>

#include "gunshape.h"
#include "log.h"

namespace shapemake {

std::atomic<uint64_t> g_wantDraw{0};
std::atomic<bool> g_frameWork{false};

namespace {

using shaperecipes::Draw;
using shaperecipes::Recipe;

using MapFn = HRESULT(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, UINT, D3D11_MAP, UINT, D3D11_MAPPED_SUBRESOURCE*);
using UnmapFn = void(STDMETHODCALLTYPE*)(ID3D11DeviceContext*, ID3D11Resource*, UINT);

ULONGLONG g_settle = 1500;      // ms after the gun's first asked for before its mesh is taken: its raise is over
ULONGLONG g_drawWait = 20000;   // ms of the gun held for its draw to come, as known and at rest
ULONGLONG g_retry = 60000;      // ms before a gun whose draw didn't come so is waited for again
constexpr ULONGLONG kAskEvery = 3000;  // ms between two asks for the gun in hand while it has no shape
std::wstring g_toolGuns, g_toolExe;  // tools: set before anything runs
std::mutex g_lineMutex;
std::string g_lastLine;
int g_lines = 0;
constexpr ULONGLONG kMapWait = 2000;    // ms for the graphics card to hand a copy back
constexpr size_t kPaletteMost = 16384;  // bytes of a palette kept (the game's uploads are 4096)

// kLater: its draw didn't come as known while it was held, or it was put away first; nothing was read back, and it may be wanted again.
enum class State { kQueued, kLater, kTried, kMade, kTaken };
enum class Step { kDraw, kCopy, kMap, kWorking };
enum class End { kAway, kWaited, kFailed };  // how a job ends without a shape: the gun put away; its draw waited for in vain; something failed

struct Known {
    State state = State::kQueued;
    ULONGLONG again = 0;  // kLater: not before this
    bool said = false;    // its line about the wait has been logged: once a launch
};

struct Job {
    const Recipe* recipe = nullptr;
    Step step = Step::kDraw;
    int draw = 0;             // which of the recipe's draws is next
    ULONGLONG asked = 0;      // when it was wanted
    ULONGLONG since = 0;      // when this step began
    ULONGLONG started = 0;    // when its first draw was taken
    int wrong = 0;            // draws of its counts that weren't its mesh
    int unrest = 0;           // draws of its mesh with a kept bone not where it is at rest
    float unrestBy = 0.0f;    // the last one's: how far (m), which bone
    int unrestBone = -1;
    int unbound = 0, based = 0;  // draws of its counts without the player's own whole palette bound, and with a base vertex
    uint32_t wrongIb = 0, wrongVb = 0, wrongStride = 0, wrongFormat = 0, wrongOffset = 0;
    ID3D11Buffer* ib = nullptr;  // the game's, held from the draw to the copy
    ID3D11Buffer* vb = nullptr;
    ID3D11Buffer* ibCopy = nullptr;  // staging
    ID3D11Buffer* vbCopy = nullptr;
    Capture capture;
};

std::mutex g_mutex;
std::map<std::string, Known> g_states;  // by name: nothing that failed is tried twice in a launch
std::deque<const Recipe*> g_queue;      // wanted while another was being made
Job g_job;                              // the one being made (recipe null: none)
std::atomic<int> g_news{0};             // shapes made that Ready hasn't told of yet
bool g_noExe = false;                   // gunshape.exe isn't there: said once
bool g_exeThere = false;                // ... it is: looked for once
std::atomic<const char*> g_holding{nullptr};  // the shape's name of the gun in hand (hands.cpp)
std::atomic<const char*> g_askedFor{nullptr};  // the last gun asked for by Holding, and when
std::atomic<ULONGLONG> g_askedAt{0};

std::wstring ModFolder() {
    wchar_t path[MAX_PATH];
    HMODULE self = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&ModFolder), &self);
    GetModuleFileNameW(self, path, MAX_PATH);
    if (wchar_t* slash = wcsrchr(path, L'\\'))
        slash[1] = 0;
    return path;
}

std::wstring Wide(const std::string& s) {
    return std::wstring(s.begin(), s.end());  // names and arguments: plain ASCII
}

uint64_t Key(const Draw& d) {
    return (static_cast<uint64_t>(d.count) << 32) | d.start;
}

void Release(ID3D11Buffer*& b) {
    if (b)
        b->Release();
    b = nullptr;
}

std::wstring GunsFolder() {
    return g_toolGuns.empty() ? ModFolder() + L"guns\\" : g_toolGuns;
}
std::wstring ExePath() {
    return g_toolExe.empty() ? ModFolder() + L"gunshape.exe" : g_toolExe;
}

void Say(const std::string& line) {
    Log("%s", line.c_str());
    std::lock_guard lock(g_lineMutex);
    g_lastLine = line;
    ++g_lines;
}

std::string Line(const Recipe& r, const char* why) {
    return std::string("Gun shape: ") + r.name + "'s shape couldn't be made (" + why + "): the hand goes through this gun, as before";
}

// Why a gun has no shape, from a thread of the game's: no memory for the line itself is not let out.
void SayWhy(const Recipe& r, const char* why) noexcept {
    try {
        Say(Line(r, why));
    } catch (...) {
    }
}

// The next of the queue becomes the job. Under g_mutex.
void Begin() {
    g_job = Job{};
    if (g_queue.empty()) {
        g_wantDraw.store(0, std::memory_order_relaxed);
        g_frameWork.store(false, std::memory_order_relaxed);
        return;
    }
    g_job.recipe = g_queue.front();
    g_queue.pop_front();
    g_job.asked = g_job.since = GetTickCount64();
    g_wantDraw.store(Key(g_job.recipe->draws[0]), std::memory_order_relaxed);
    g_frameWork.store(true, std::memory_order_relaxed);
}

// The job's over without a shape. Under g_mutex.
//   kAway: the gun was put away before its draw was taken: nothing is said, and it's wanted again the next time it's held.
//   kWaited: its draw didn't come as known in all the time it was held: why is said once a launch, and it's waited for again after g_retry
//     (nothing was read back or made: waiting again costs a comparison a draw).
//   kFailed: something was tried and failed: why is said, and it isn't tried again this launch.
void GiveUp(End end, const char* why) {
    const Recipe* r = g_job.recipe;
    Release(g_job.ib), Release(g_job.vb), Release(g_job.ibCopy), Release(g_job.vbCopy);
    if (r) {
        Known& k = g_states[r->name];
        if (end == End::kFailed) {
            k.state = State::kTried;
            SayWhy(*r, why);
        } else {
            k.state = State::kLater;
            k.again = GetTickCount64() + (end == End::kWaited ? g_retry : 0);
            if (end == End::kWaited && !k.said) {
                k.said = true;
                SayWhy(*r, why);
            }
        }
    }
    Begin();
}

// The mod's own thread. Nothing thrown leaves it (the game is a 32-bit process near its limit: no memory is the likely one): the gun then
// has no shape this launch, as when anything else fails.
void Work(const Recipe* recipe, Capture capture, ULONGLONG started) {
    bool ok = false;
    try {
        static std::once_flag swept;
        const std::wstring guns = GunsFolder();
        std::call_once(swept, [&] { Sweep(guns); });
        Result made = Make(*recipe, capture, guns, ExePath());
        if (made.ok && !gunshape::Reload(recipe->name)) {
            made.ok = false;
            made.line = Line(*recipe, "the new file didn't load");
        }
        if (made.ok) {
            char text[160];
            std::snprintf(text, sizeof(text), "Gun shape: %s's shape made on this PC from the game's own mesh in %.1f s", recipe->name,
                          static_cast<double>(GetTickCount64() - started) / 1000.0);
            made.line = text;
        }
        Say(made.line);
        ok = made.ok;
    } catch (...) {
        ok = false;
        try {
            Say(Line(*recipe, "no memory left for the making"));
        } catch (...) {
        }
    }
    std::lock_guard lock(g_mutex);
    try {
        g_states[recipe->name].state = ok ? State::kMade : State::kTried;
        if (ok)
            g_news.fetch_add(1, std::memory_order_relaxed);
    } catch (...) {
    }
    Begin();  // (the next of the queue: never left waiting behind this one)
}

bool WriteAll(const std::wstring& path, const void* data, size_t bytes) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"wb") || !f)
        return false;
    const bool ok = fwrite(data, 1, bytes, f) == bytes;
    return fclose(f) == 0 && ok;
}

// A folder's files and, one level down, its folders' files; then the folders.
void RemoveFolder(const std::wstring& dir, int depth = 0) {
    WIN32_FIND_DATAW found;
    const HANDLE h = FindFirstFileW((dir + L"*").c_str(), &found);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!wcscmp(found.cFileName, L".") || !wcscmp(found.cFileName, L".."))
                continue;
            if (found.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
                continue;  // (a link is never followed)
            if (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (depth < 1)
                    RemoveFolder(dir + found.cFileName + L"\\", depth + 1);
            } else {
                DeleteFileW((dir + found.cFileName).c_str());
            }
        } while (FindNextFileW(h, &found));
        FindClose(h);
    }
    RemoveDirectoryW(dir.substr(0, dir.size() - 1).c_str());
}

}

void Sweep(const std::wstring& gunsFolder) {
    RemoveFolder(gunsFolder + L"make\\");
    WIN32_FIND_DATAW found;
    const HANDLE h = FindFirstFileW((gunsFolder + L"*.bin.tmp").c_str(), &found);
    if (h == INVALID_HANDLE_VALUE)
        return;
    do {
        if (!(found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            DeleteFileW((gunsFolder + found.cFileName).c_str());
    } while (FindNextFileW(h, &found));
    FindClose(h);
}

std::string Refused(const std::wstring& file, const Recipe& recipe) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, file.c_str(), L"rb") || !f)
        return "gunshape.exe wrote no file";
    char magic[4] = {};
    int32_t version = 0, n[3] = {};
    float origin[3] = {}, cell = 0.0f;
    const bool head = fread(magic, 1, 4, f) == 4 && fread(&version, 4, 1, f) == 1 && fread(n, 4, 3, f) == 3 && fread(origin, 4, 3, f) == 3 &&
                      fread(&cell, 4, 1, f) == 1;
    _fseeki64(f, 0, SEEK_END);
    const long long size = _ftelli64(f);
    fclose(f);
    if (!head || std::memcmp(magic, "GSDF", 4) || version != 2)
        return "what gunshape.exe wrote isn't a version-2 shape";
    char text[160];
    for (int k = 0; k < 3; ++k)
        if (std::abs(n[k] - recipe.cells[k]) > shaperecipes::kCellsApart) {
            std::snprintf(text, sizeof(text), "it came out %d x %d x %d cells, not this gun's %d x %d x %d: another pose or mesh", n[0], n[1], n[2],
                          recipe.cells[0], recipe.cells[1], recipe.cells[2]);
            return text;
        }
    if (!(std::fabs(cell - shaperecipes::kCell) < 1e-6f) || size != 36 + 2ll * n[0] * n[1] * n[2])
        return "what gunshape.exe wrote isn't whole";
    for (float o : origin)
        if (!(std::fabs(o) < 5.0f))
            return "what gunshape.exe wrote is out of place";
    return {};
}

Result Make(const Recipe& recipe, const Capture& capture, const std::wstring& gunsFolder, const std::wstring& exe, unsigned long timeoutMs) {
    Result out;
    auto fail = [&](const std::string& why) {
        out.ok = false;
        out.line = Line(recipe, why.c_str());
        return out;
    };
    const std::wstring name = Wide(recipe.name);
    const std::wstring make = gunsFolder + L"make\\", dir = make + name + L"\\";
    const std::wstring tmp = gunsFolder + name + L".bin.tmp", bin = gunsFolder + name + L".bin";
    struct Tidy {  // the dump and the .tmp never stay
        const std::wstring &dir, &make, &tmp;
        ~Tidy() {
            RemoveFolder(dir);
            RemoveDirectoryW(make.substr(0, make.size() - 1).c_str());  // (only if empty)
            DeleteFileW(tmp.c_str());
        }
    } tidy{dir, make, tmp};
    if (static_cast<int>(capture.draws.size()) != recipe.drawCount)
        return fail("its mesh wasn't all read");
    if (GetFileAttributesW(exe.c_str()) == INVALID_FILE_ATTRIBUTES)
        return fail("build\\gunshape.exe isn't there");
    RemoveFolder(dir);
    DeleteFileW(tmp.c_str());
    CreateDirectoryW(gunsFolder.substr(0, gunsFolder.size() - 1).c_str(), nullptr);
    CreateDirectoryW(make.substr(0, make.size() - 1).c_str(), nullptr);
    // (A folder still there after RemoveFolder, an earlier launch's helper not yet out of it: used as it is; what can't be written says so.)
    if (!CreateDirectoryW(dir.substr(0, dir.size() - 1).c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
        return fail("the folder guns\\make couldn't be written");
    // The dump, as weapon.txt "dump" writes one (skin.cpp WriteDump): tools/meshdump.h reads these.
    for (const CapturedDraw& d : capture.draws) {
        const std::wstring prefix = dir + Wide(shaperecipes::FilePrefix(Draw{d.count, d.start}));
        char text[256];
        const int length = std::snprintf(text, sizeof(text), "draw %u %u %d topology %d\nib format %u offset %u size %zu saved\nvb 0 stride %u offset %u size %zu saved\n",
                                         d.count, d.start, d.base, d.topology, d.ibFormat, d.ibOffset, d.ib.size(), d.stride, d.vbOffset, d.vb.size());
        if (!WriteAll(prefix + L"dump.txt", text, static_cast<size_t>(length)) || !WriteAll(prefix + L"ib.bin", d.ib.data(), d.ib.size()) ||
            !WriteAll(prefix + L"vb0.bin", d.vb.data(), d.vb.size()) || !WriteAll(prefix + L"bones.bin", d.bones.data(), d.bones.size()))
            return fail("its mesh couldn't be written to guns\\make (the disk full?)");
    }
    // gunshape.exe from the dump's own folder, its paths relative: a folder name outside the PC's code page never reaches its narrow arguments.
    std::wstring command = L"\"" + exe + L"\" . \"..\\..\\" + name + L".bin.tmp\" 0.003 " + Wide(shaperecipes::Arguments(recipe));
    // Its words go to guns\gunshape.log; only that file is handed to it, none of the game's own handles (as sounds.cpp starts gamesounds.exe).
    SECURITY_ATTRIBUTES inherit{sizeof(inherit), nullptr, TRUE};
    HANDLE log = CreateFileW((gunsFolder + L"gunshape.log").c_str(), GENERIC_WRITE, FILE_SHARE_READ, &inherit, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    SIZE_T size = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
    std::vector<BYTE> attributes(size ? size : 1);
    auto list = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());
    const bool handOver = log != INVALID_HANDLE_VALUE && size && InitializeProcThreadAttributeList(list, 1, 0, &size) &&
                          UpdateProcThreadAttribute(list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, &log, sizeof(log), nullptr, nullptr);
    STARTUPINFOEXW start{};
    start.StartupInfo.cb = handOver ? sizeof(STARTUPINFOEXW) : sizeof(STARTUPINFOW);
    if (handOver) {
        start.lpAttributeList = list;
        start.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
        start.StartupInfo.hStdOutput = log;
        start.StartupInfo.hStdError = log;
    }
    // The helper ends with the game: closed or crashed while it runs, the game's handle to this job goes and Windows ends the helper with it,
    // so none is left writing into guns\ at the next launch (nor keeps Steam showing the game as running). No job to be had: as before.
    struct Helper {
        HANDLE job = nullptr;
        ~Helper() {
            if (job)
                CloseHandle(job);
        }
    } helper;
    helper.job = CreateJobObjectW(nullptr, nullptr);
    if (helper.job) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
        limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!SetInformationJobObject(helper.job, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) {
            CloseHandle(helper.job);
            helper.job = nullptr;
        }
    }
    PROCESS_INFORMATION process{};
    const BOOL ok = CreateProcessW(exe.c_str(), command.data(), nullptr, nullptr, handOver,
                                   CREATE_NO_WINDOW | IDLE_PRIORITY_CLASS | CREATE_SUSPENDED | (handOver ? EXTENDED_STARTUPINFO_PRESENT : 0), nullptr,
                                   dir.c_str(), &start.StartupInfo, &process);
    const DWORD error = GetLastError();
    if (handOver)
        DeleteProcThreadAttributeList(list);
    if (log != INVALID_HANDLE_VALUE)
        CloseHandle(log);
    if (!ok) {
        char text[96];
        std::snprintf(text, sizeof(text), "build\\gunshape.exe couldn't be started, error %lu: an antivirus?", error);
        return fail(text);
    }
    if (helper.job)
        AssignProcessToJobObject(helper.job, process.hProcess);  // (refused: it runs on without one)
    ResumeThread(process.hThread);
    CloseHandle(process.hThread);
    DWORD code = 1;
    const DWORD waited = WaitForSingleObject(process.hProcess, timeoutMs);
    if (waited != WAIT_OBJECT_0) {
        TerminateProcess(process.hProcess, 1);
        WaitForSingleObject(process.hProcess, 5000);
        CloseHandle(process.hProcess);
        return fail("build\\gunshape.exe took too long and was stopped");
    }
    GetExitCodeProcess(process.hProcess, &code);
    CloseHandle(process.hProcess);
    if (code != 0) {
        char text[96];
        std::snprintf(text, sizeof(text), "build\\gunshape.exe gave up, code %lu; guns\\gunshape.log says why", code);
        return fail(text);
    }
    if (const std::string why = Refused(tmp, recipe); !why.empty())
        return fail(why);
    if (!MoveFileExW(tmp.c_str(), bin.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        return fail("the new file couldn't be put in guns\\");
    out.ok = true;
    out.line = std::string("Gun shape: ") + recipe.name + "'s shape made on this PC from the game's own mesh";
    return out;
}

void ForTools(const wchar_t* gunsFolder, const wchar_t* exe, unsigned long settleMs, unsigned long drawWaitMs, unsigned long retryMs) {
    g_toolGuns = gunsFolder ? gunsFolder : L"";
    g_toolExe = exe ? exe : L"";
    g_settle = settleMs;
    g_drawWait = drawWaitMs;
    g_retry = retryMs;
}

std::string LastLine() {
    std::lock_guard lock(g_lineMutex);
    return g_lastLine;
}

int Lines() {
    std::lock_guard lock(g_lineMutex);
    return g_lines;
}

void Want(const char* name) {
    if (!name)
        return;
    std::lock_guard lock(g_mutex);
    try {
        const auto known = g_states.find(name);
        if (known != g_states.end() && (known->second.state != State::kLater || GetTickCount64() < known->second.again))
            return;
        const Recipe* recipe = shaperecipes::Find(name);
        if (!recipe) {
            g_states[name].state = State::kTried;  // (gunshape.cpp has said its file isn't there)
            return;
        }
        if (!g_exeThere && GetFileAttributesW(ExePath().c_str()) == INVALID_FILE_ATTRIBUTES) {
            g_states[name].state = State::kTried;
            if (!g_noExe)
                Say("Gun shape: build\\gunshape.exe isn't there, so no gun's shape can be made on this PC: the hands go through the guns, as before");
            g_noExe = true;
            return;
        }
        g_exeThere = true;
        g_queue.push_back(recipe);
        g_states[name].state = State::kQueued;
        if (!g_job.recipe)
            Begin();
    } catch (...) {  // (no memory for its name: asked again in a few seconds)
    }
}

void Holding(const char* name, bool hasShape) {
    const char* before = g_holding.exchange(name, std::memory_order_relaxed);
    if (!name || hasShape)
        return;
    const ULONGLONG now = GetTickCount64();
    if (before == name && g_askedFor.load(std::memory_order_relaxed) == name && now - g_askedAt.load(std::memory_order_relaxed) < kAskEvery)
        return;
    g_askedFor.store(name, std::memory_order_relaxed);
    g_askedAt.store(now, std::memory_order_relaxed);
    Want(name);
}

bool Ready(const char* name) {
    if (!name || g_news.load(std::memory_order_relaxed) <= 0)
        return false;
    std::lock_guard lock(g_mutex);
    const auto it = g_states.find(name);
    if (it == g_states.end() || it->second.state != State::kMade)
        return false;
    it->second.state = State::kTaken;
    g_news.fetch_sub(1, std::memory_order_relaxed);
    return true;
}

void NoteDraw(ID3D11DeviceContext* ctx, uint32_t count, uint32_t start, int base, const uint8_t* palette, size_t paletteBytes) {
    if (!ctx)
        return;
    std::unique_lock lock(g_mutex, std::try_to_lock);  // (a draw never waits: the next one will do)
    if (!lock.owns_lock())
        return;
    Job& j = g_job;
    if (!j.recipe || j.step != Step::kDraw)
        return;
    const Draw& d = j.recipe->draws[j.draw];
    const ULONGLONG now = GetTickCount64();
    if (d.count != count || d.start != start || now - j.asked < g_settle)
        return;
    // Not the player's own palette, whole and bound for this draw (skin.cpp), or short of a bone a vertex names (tools/meshdump.h would read past
    // it): not this draw, a later one.
    if (!palette || paletteBytes < 48u * static_cast<size_t>(d.bones)) {
        ++j.unbound;
        return;
    }
    if (base != 0) {
        ++j.based;
        return;
    }
    ID3D11Buffer* vb = nullptr;
    ID3D11Buffer* ib = nullptr;
    UINT stride = 0, offset = 0, ibOffset = 0;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ctx->IAGetVertexBuffers(0, 1, &vb, &stride, &offset);
    ctx->IAGetIndexBuffer(&ib, &format, &ibOffset);
    ctx->IAGetPrimitiveTopology(&topology);
    D3D11_BUFFER_DESC id{}, vd{};
    if (ib)
        ib->GetDesc(&id);
    if (vb)
        vb->GetDesc(&vd);
    // Its mesh as known, or it's another model with the same counts (or a layout tools/meshdump.h doesn't read): left alone.
    if (!ib || !vb || id.ByteWidth != d.ibBytes || vd.ByteWidth != d.vbBytes || stride != shaperecipes::kStride || offset != d.vbOffset ||
        format != DXGI_FORMAT_R16_UINT || ibOffset != 0 || topology != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) {
        ++j.wrong;
        j.wrongIb = id.ByteWidth, j.wrongVb = vd.ByteWidth, j.wrongStride = stride, j.wrongFormat = static_cast<uint32_t>(format), j.wrongOffset = offset;
        Release(ib), Release(vb);
        return;
    }
    // Its mesh; but only as the shape is of it, every kept bone where it is with the gun at rest in the hands: mid-raise, a handle pulled, a
    // cover or a lid open, a bolt back, it would be kept in the shape for good. Not now: a later draw.
    int worst = -1;
    const float apart = shaperecipes::Apart(d, j.draw == 0 ? j.recipe->frameBone : d.ownBone, palette, static_cast<int>(paletteBytes / 48), &worst);
    if (!(apart <= shaperecipes::kRestApart)) {
        ++j.unrest;
        j.unrestBy = apart, j.unrestBone = worst;
        Release(ib), Release(vb);
        return;
    }
    CapturedDraw c;
    c.count = count, c.start = start, c.base = base, c.topology = static_cast<int>(topology);
    c.ibFormat = static_cast<uint32_t>(format), c.ibOffset = ibOffset, c.stride = stride, c.vbOffset = offset;
    try {
        c.bones.assign(palette, palette + (paletteBytes < kPaletteMost ? paletteBytes : kPaletteMost));
        j.capture.draws.push_back(std::move(c));
    } catch (const std::bad_alloc&) {
        Release(ib), Release(vb);
        return;
    }
    j.ib = ib, j.vb = vb;
    if (!j.started)
        j.started = now;
    j.step = Step::kCopy;
    j.since = now;
    g_wantDraw.store(0, std::memory_order_relaxed);
}

void OnFrameWork(void* map, void* unmap) {
    std::unique_lock lock(g_mutex, std::try_to_lock);  // (the frame never waits either)
    if (!lock.owns_lock())
        return;
    Job& j = g_job;
    if (!j.recipe || j.step == Step::kWorking)
        return;
    const ULONGLONG now = GetTickCount64();
    if (j.step == Step::kDraw) {
        // Put away before its draw was taken: let go at once (the next in the queue isn't kept waiting, and another model with its counts
        // isn't taken for a wrong mesh), and wanted again when it's held again.
        const char* held = g_holding.load(std::memory_order_relaxed);
        if (!held || std::strcmp(held, j.recipe->name)) {
            GiveUp(End::kAway, nullptr);
            return;
        }
        if (now - j.since < g_drawWait)
            return;
        // Held all that time and never drawn as known: said once, and waited for again later.
        const Draw& d = j.recipe->draws[j.draw];
        const double seconds = static_cast<double>(g_drawWait) / 1000.0;
        char text[320];
        if (j.unrest)
            std::snprintf(text, sizeof(text), "in %.0f s of holding it its parts weren't once where they are at rest: %d draws, the last with bone %d %.1f mm off; "
                          "waited for again", seconds, j.unrest, j.unrestBone, j.unrestBy < 10.0f ? static_cast<double>(j.unrestBy) * 1000.0 : 9999.0);
        else if (j.wrong)
            std::snprintf(text, sizeof(text), "its draw %u/%u wasn't the mesh known: %d seen, the last with an index buffer of %u bytes, a vertex buffer of %u from %u, "
                          "stride %u, index format %u; waited for again", d.count, d.start, j.wrong, j.wrongIb, j.wrongVb, j.wrongOffset, j.wrongStride, j.wrongFormat);
        else if (j.unbound || j.based)
            std::snprintf(text, sizeof(text), "in %.0f s of holding it its draw %u/%u never came as the player's own: %d without the player's bones bound, %d with a "
                          "base vertex; waited for again", seconds, d.count, d.start, j.unbound, j.based);
        else
            std::snprintf(text, sizeof(text), "in %.0f s of holding it its %s %u/%u wasn't drawn; waited for again", seconds, j.draw > 0 ? "second part" : "draw",
                          d.count, d.start);
        GiveUp(End::kWaited, text);
        return;
    }
    ID3D11Buffer* any = j.ib ? j.ib : j.ibCopy ? j.ibCopy : j.vbCopy;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* imm = nullptr;
    if (any)
        any->GetDevice(&device);
    if (device)
        device->GetImmediateContext(&imm);
    struct Let {
        ID3D11Device*& device;
        ID3D11DeviceContext*& imm;
        ~Let() {
            if (imm)
                imm->Release();
            if (device)
                device->Release();
        }
    } let{device, imm};
    if (!imm) {
        GiveUp(End::kFailed, "the graphics card's context couldn't be had");
        return;
    }
    if (j.step == Step::kCopy) {
        auto staging = [&](ID3D11Buffer* from) -> ID3D11Buffer* {
            D3D11_BUFFER_DESC d{};
            from->GetDesc(&d);
            const D3D11_BUFFER_DESC sd{d.ByteWidth, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ, 0, 0};
            ID3D11Buffer* b = nullptr;
            return SUCCEEDED(device->CreateBuffer(&sd, nullptr, &b)) ? b : nullptr;
        };
        j.ibCopy = staging(j.ib);
        j.vbCopy = staging(j.vb);
        if (!j.ibCopy || !j.vbCopy) {
            GiveUp(End::kFailed, "the graphics card had no room for a copy of its mesh");
            return;
        }
        imm->CopyResource(j.ibCopy, j.ib);
        imm->CopyResource(j.vbCopy, j.vb);
        Release(j.ib), Release(j.vb);
        j.step = Step::kMap;
        j.since = now;
        return;
    }
    // One buffer a frame, and only if the graphics card has it ready.
    ID3D11Buffer*& copy = j.ibCopy ? j.ibCopy : j.vbCopy;
    std::vector<uint8_t>& into = j.ibCopy ? j.capture.draws.back().ib : j.capture.draws.back().vb;
    D3D11_BUFFER_DESC d{};
    copy->GetDesc(&d);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    const HRESULT hr = map ? reinterpret_cast<MapFn>(map)(imm, copy, 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &mapped)
                           : imm->Map(copy, 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &mapped);
    if (hr == DXGI_ERROR_WAS_STILL_DRAWING) {
        if (now - j.since > kMapWait)
            GiveUp(End::kFailed, "the graphics card didn't hand its mesh back in 2 s");
        return;
    }
    if (FAILED(hr) || !mapped.pData) {
        if (SUCCEEDED(hr))
            unmap ? reinterpret_cast<UnmapFn>(unmap)(imm, copy, 0) : imm->Unmap(copy, 0);
        GiveUp(End::kFailed, "the copy of its mesh couldn't be read");
        return;
    }
    bool room = true;
    try {
        into.assign(static_cast<const uint8_t*>(mapped.pData), static_cast<const uint8_t*>(mapped.pData) + d.ByteWidth);
    } catch (const std::bad_alloc&) {
        room = false;
    }
    unmap ? reinterpret_cast<UnmapFn>(unmap)(imm, copy, 0) : imm->Unmap(copy, 0);
    Release(copy);
    if (!room) {
        GiveUp(End::kFailed, "no memory left for its mesh");
        return;
    }
    if (j.ibCopy || j.vbCopy) {  // the other buffer: the next frame
        j.since = now;
        return;
    }
    if (++j.draw < j.recipe->drawCount) {
        j.step = Step::kDraw;
        j.since = now;
        g_wantDraw.store(Key(j.recipe->draws[j.draw]), std::memory_order_relaxed);
        return;
    }
    // All read: the rest is a thread's of the mod's own.
    j.step = Step::kWorking;
    g_frameWork.store(false, std::memory_order_relaxed);
    try {
        std::thread(Work, j.recipe, std::move(j.capture), j.started).detach();
    } catch (...) {
        GiveUp(End::kFailed, "no thread could be started");
    }
}

}

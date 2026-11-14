// The frame's memory (src/framemem.h's logic and src/framemem.cpp, compiled in here) against a made-up shader system, settings and block laid out as the game's, with the game's code bytes
// served as they were read (or not) and every call out of it stubbed: MinHook, the game's allocator, the system's memory. Without the game. Each check prints PASS or FAIL, then the totals;
// the exit code is the number failed.
//   1  the sizes to try (64, then 48, then 32) and the room the game is left
//   2  the meter and the net: a frame's peak, the scope's share guessed and measured, 85 percent to the byte, the second's numbers
//   3  the game's code not as read (each piece in turn, one byte off or unreadable): nothing hooked, nothing read, said once; frame_memory_mb=0: nothing hooked, the block still readable
//   4  hooked: the settings' side first; a hook that can't be made or switched on: both taken away again
//   5  the first frame's end: the block made 64 MB in place before the game's own code runs (start, size, not owned; the rest as it was), the settings told; the game's first buffer given
//      back to its own allocator at the second frame's end, once; nothing more in the frames after
//   6  the once-a-second line, and the warnings past three quarters and past the whole
//   7  no piece of 64 MB: 48; the address space tight: 32; nothing to be had: the block bit for bit as the game made it, not tried again
//   8  settings waiting with the game's own 16 MB: given the block's size before the game takes them, the block kept (the same game code without the mod's hook throws it away); settings
//      that already agree, and a block that isn't the mod's: not touched
//   9  the game making its block anew (as a new shader system would): the mod's buffer given back a frame later, the new block made bigger; the game's first buffer still waiting then
//  10  no block, a block not the game's make, a block as big as asked already, a block that can't be read (the fault caught, the game's own code still run)
//  11  Used, the scope's share and the net through the calls scopeview makes, the crash record's line
//  12  the system's own memory once for real: 64 MB taken, written at both ends, given back
//  13  no log line the self-test would take for a failure; the time a frame's end takes
// Usage: framememtest

#include <windows.h>
#include <MinHook.h>

#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
#include <string>
#include <vector>

#include "../src/framemem.cpp"

namespace {

using framemem::Block;
using framemem::kMb;

int g_passed = 0, g_failed = 0;
void Check(bool ok, const char* fmt, ...) {
    char text[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);
    printf("  %s  %s\n", ok ? "PASS" : "FAIL", text);
    ++(ok ? g_passed : g_failed);
}

uint32_t Address(const void* p) { return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(p)); }
template <typename T> void Put(void* base, size_t at, T v) { std::memcpy(static_cast<uint8_t*>(base) + at, &v, sizeof(T)); }
template <typename T> T Get(const void* base, size_t at) { T v; std::memcpy(&v, static_cast<const uint8_t*>(base) + at, sizeof(T)); return v; }

// ---- The log ----
std::vector<std::string> g_lines;
int Said(const char* part) {
    int n = 0;
    for (const std::string& line : g_lines)
        n += line.find(part) != std::string::npos;
    return n;
}

// ---- The settings file's switch ----
config::Settings g_config;

// ---- The game's code as SafeRead serves it: as read, one piece a byte off, or one piece unreadable ----
int g_oddPiece = -1;  // which of the pieces (the table's slot counts as one more) isn't as read; -1 none
bool g_oddUnreadable = false;
constexpr int kPieces = static_cast<int>(sizeof(framemem::kCode) / sizeof(framemem::kCode[0]));
uint32_t g_holder = 0;  // what the shader system's holder holds

// ---- MinHook ----
struct Hooking {
    std::vector<std::string> events;
    MH_STATUS createEnd = MH_OK, createSettings = MH_OK, enableEnd = MH_OK, enableSettings = MH_OK;
};
Hooking g_hooking;

// ---- The system's memory and the game's allocator ----
struct Memory {
    uint32_t freeMb = 2400, largestMb = 900, spaceAsked = 0;
    uint32_t refuseAboveMb = 0xFFFFFFFFu;  // the system has no piece bigger than this
    std::vector<uint32_t> takes;           // every size asked for, bytes
    std::vector<uint32_t> taken, given;    // buffers handed out, and those given back
    std::vector<std::pair<uint32_t, uint32_t>> gameFrees;  // (allocator, buffer)
    uint32_t next = 0x30000000u;
};
Memory g_memory;
void* FakeTake(uint32_t bytes) {
    g_memory.takes.push_back(bytes);
    if (bytes / kMb > g_memory.refuseAboveMb)
        return nullptr;
    const uint32_t at = g_memory.next;
    g_memory.next += 0x08000000u;
    g_memory.taken.push_back(at);
    return framemem::At<void>(at);
}
void FakeGiveBack(void* buffer) { g_memory.given.push_back(Address(buffer)); }
void FakeGameFree(uint32_t allocator, uint32_t buffer) { g_memory.gameFrees.push_back({allocator, buffer}); }
void FakeSpace(uint32_t* freeMb, uint32_t* largestMb) {
    ++g_memory.spaceAsked;
    *freeMb = g_memory.freeMb;
    *largestMb = g_memory.largestMb;
}

// ---- The game: a shader system (0x170 bytes), its settings in use and waiting (0x120 each), its block (0x20) ----
constexpr uint32_t kAllocator = 0x022E8BE0u, kFirstBuffer = 0xF61B0000u, kName = 0x02097410u;
struct Game {
    alignas(16) uint8_t system[0x170] = {};
    alignas(16) uint8_t inUse[0x120] = {};
    alignas(16) uint8_t waiting[0x120] = {};
    struct Slot {
        Block block;
        uint32_t tail;
    } blocks[8] = {};
    int made = 0;
    // What the game's own code saw and did.
    int endCalls = 0, settingsCalls = 0, kept = 0, remade = 0;
    Block seen{};  // the block as the game's end of frame found it
    uint32_t nextBuffer = 0xF51B0000u;

    Block* Make(uint32_t start, uint32_t size, uint32_t allocator = kAllocator) {
        Block* b = &blocks[made++].block;
        *b = Block{start, size, 16, 1, 0, 0, allocator, kName};
        return b;
    }
    void Use(Block* b) { Put<uint32_t>(system, framemem::kBlock, b ? Address(b) : 0); }
    Block* Now() { return framemem::At<Block>(Get<uint32_t>(system, framemem::kBlock)); }
    void Post(uint32_t size) {  // settings posted: a copy of those in use with this size for the block, waiting to be taken
        std::memcpy(waiting, inUse, sizeof(waiting));
        Put<uint32_t>(waiting, framemem::kSettingsSize, size);
        Put<uint32_t>(system, framemem::kWaiting, Address(waiting));
    }
    void Start(uint32_t size = 16 * kMb) {
        Put<uint32_t>(inUse, framemem::kSettingsSize, size);
        Put<uint32_t>(inUse, 0x9C, 4u);
        Put<uint32_t>(system, framemem::kSettings, Address(inUse));
        Put<uint32_t>(system, framemem::kWaiting, 0u);
        Use(Make(kFirstBuffer, size));
    }
};
std::unique_ptr<Game> g_game;

// The game's end of frame as far as it matters: the block emptied, its argument returned.
void* __fastcall GameEndFrame(uint8_t* self, void*, void* out) {
    ++g_game->endCalls;
    const uint32_t at = Get<uint32_t>(self, framemem::kBlock);
    if (at >= 0x10000) {
        Block* b = framemem::At<Block>(at);
        g_game->seen = *b;
        b->used = 0;
    }
    return out;
}

// The game's taking of waiting settings: copied over those in use; a block whose size isn't theirs thrown away (its buffer back to its allocator when it owns it) and made anew at theirs.
void __fastcall GameSettings(uint8_t* self, void*) {
    ++g_game->settingsCalls;
    const uint32_t waiting = Get<uint32_t>(self, framemem::kWaiting), inUse = Get<uint32_t>(self, framemem::kSettings);
    if (!waiting || waiting == inUse)
        return;
    std::memcpy(framemem::At<void>(inUse), framemem::At<void>(waiting), 0x120);
    Put<uint32_t>(self, framemem::kWaiting, 0u);
    Block* b = framemem::At<Block>(Get<uint32_t>(self, framemem::kBlock));
    const uint32_t size = Get<uint32_t>(framemem::At<void>(inUse), framemem::kSettingsSize);
    if (b && b->size == size) {
        ++g_game->kept;
        return;
    }
    ++g_game->remade;
    if (b && b->owned)
        FakeGameFree(b->allocator, b->start);
    g_game->nextBuffer -= 0x01000000u;
    Put<uint32_t>(self, framemem::kBlock, Address(g_game->Make(g_game->nextBuffer, size)));
}

void Fresh(int mb) {
    std::destroy_at(&framemem::g_keeper);
    new (&framemem::g_keeper) framemem::Keeper();
    std::destroy_at(&framemem::g_meter);
    new (&framemem::g_meter) framemem::Meter();
    framemem::g_wantedMb = 0;
    framemem::g_layout = false;
    framemem::g_stopped = false;
    framemem::g_settled = false;
    framemem::g_saidOdd = false;
    framemem::g_tries = 0;
    framemem::g_lastLine = 0;
    framemem::g_endFrame = nullptr;
    framemem::g_settings = nullptr;
    framemem::g_calls = {FakeTake, FakeGiveBack, FakeGameFree, FakeSpace};
    g_config.frameMemoryMb = mb;
    g_lines.clear();
    g_hooking = Hooking();
    g_memory = Memory();
    g_oddPiece = -1;
    g_oddUnreadable = false;
    g_game = std::make_unique<Game>();
    g_game->Start();
    g_holder = Address(g_game->system);
}

// A level under way with the hooks on (the first per-frame call makes them).
void Hooked(int mb = 64) {
    Fresh(mb);
    framemem::OnFrame();
}

// A frame: what it took of the block, then the game's end of frame through the mod's hook.
void* g_out = nullptr;
void* Frame(uint32_t used) {
    if (Block* b = g_game->Now(); Address(b) >= 0x10000)
        b->used = used;
    return framemem::EndFrameHook(g_game->system, nullptr, &g_out);
}

// The game's free faulting inside code that isn't the mod's (the system's own copy, in ntdll: outside this test's module whichever C runtime it's built with, now
// that the C runtime is built in), reading address 16, and a frame that says whether that fault came out of the mod's hook.
void* (*volatile g_copy)(void*, const void*, size_t) =
    reinterpret_cast<void* (*)(void*, const void*, size_t)>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "memmove"));
void FaultyGameFree(uint32_t, uint32_t) {
    char to[64];
    g_copy(to, reinterpret_cast<const void*>(static_cast<uintptr_t>(16)), sizeof(to));
}
bool FrameFaults(uint32_t used) {
    __try {
        Frame(used);
        return false;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return true;
    }
}

}  // namespace

// ---- What framemem.cpp calls out to ----
void logx::Open(HMODULE) {}
void logx::Write(const char* fmt, ...) {
    char text[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);
    g_lines.emplace_back(text);
}

const config::Settings& config::Get() { return g_config; }

bool camera::SafeRead(uintptr_t address, void* out, size_t size) {
    for (int i = 0; i < kPieces; ++i) {
        const framemem::Code& code = framemem::kCode[i];
        if (address == code.at && size <= code.size) {
            if (i == g_oddPiece && g_oddUnreadable)
                return false;
            std::memcpy(out, code.bytes, size);
            if (i == g_oddPiece)
                static_cast<uint8_t*>(out)[size - 1] ^= 0x01;
            return true;
        }
    }
    if (address == framemem::kEndFrameSlot && size == 4) {
        const uint32_t slot = g_oddPiece == kPieces ? 0x0127D910u : static_cast<uint32_t>(framemem::kEndFrameAt);
        std::memcpy(out, &slot, 4);
        return !(g_oddPiece == kPieces && g_oddUnreadable);
    }
    if (address == framemem::kHolder && size == 4) {
        std::memcpy(out, &g_holder, 4);
        return true;
    }
    if (address < 0x10000)
        return false;
    __try {
        std::memcpy(out, reinterpret_cast<const void*>(address), size);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

MH_STATUS WINAPI MH_CreateHook(LPVOID target, LPVOID, LPVOID* original) {
    const bool end = reinterpret_cast<uintptr_t>(target) == framemem::kEndFrameAt;
    g_hooking.events.push_back(end ? "create end" : "create settings");
    const MH_STATUS status = end ? g_hooking.createEnd : g_hooking.createSettings;
    if (status == MH_OK)
        *original = end ? reinterpret_cast<void*>(&GameEndFrame) : reinterpret_cast<void*>(&GameSettings);
    return status;
}
MH_STATUS WINAPI MH_EnableHook(LPVOID target) {
    const bool end = reinterpret_cast<uintptr_t>(target) == framemem::kEndFrameAt;
    g_hooking.events.push_back(end ? "enable end" : "enable settings");
    return end ? g_hooking.enableEnd : g_hooking.enableSettings;
}
MH_STATUS WINAPI MH_DisableHook(LPVOID target) {
    g_hooking.events.push_back(reinterpret_cast<uintptr_t>(target) == framemem::kEndFrameAt ? "disable end" : "disable settings");
    return MH_OK;
}
MH_STATUS WINAPI MH_RemoveHook(LPVOID target) {
    g_hooking.events.push_back(reinterpret_cast<uintptr_t>(target) == framemem::kEndFrameAt ? "remove end" : "remove settings");
    return MH_OK;
}

namespace {

std::string Events() {
    std::string all;
    for (const std::string& e : g_hooking.events)
        all += (all.empty() ? "" : ", ") + e;
    return all;
}

void Sizes() {
    printf("1  the sizes to try and the room the game is left\n");
    const auto list = [](uint32_t wanted, uint32_t have) {
        uint32_t out[4] = {};
        const int n = framemem::Sizes(wanted, have, out);
        std::string text;
        for (int i = 0; i < n; ++i)
            text += (i ? " " : "") + std::to_string(out[i]);
        return text;
    };
    Check(list(64, 16 * kMb) == "64 48 32", "64 asked of a 16 MB block: %s", list(64, 16 * kMb).c_str());
    Check(list(64, 40 * kMb) == "64 48", "64 asked of a 40 MB block: %s", list(64, 40 * kMb).c_str());
    Check(list(40, 16 * kMb) == "40 32", "40 asked: %s", list(40, 16 * kMb).c_str());
    Check(list(128, 16 * kMb) == "128 64 48 32", "128 asked: %s", list(128, 16 * kMb).c_str());
    Check(list(256, 16 * kMb) == "256 64 48 32", "256 asked: %s", list(256, 16 * kMb).c_str());
    Check(list(32, 16 * kMb) == "32", "32 asked: %s", list(32, 16 * kMb).c_str());
    Check(list(16, 16 * kMb).empty() && list(8, 16 * kMb).empty() && list(0, 16 * kMb).empty() && list(64, 64 * kMb).empty(), "16, 8 and 0 asked of 16 MB, 64 of 64: none");
    Check(framemem::Fits(64, 2400, 900), "64 MB fits with 2400 MB free, the largest piece 900");
    Check(!framemem::Fits(64, 319, 900) && framemem::Fits(64, 320, 900), "64 MB needs 320 MB free (itself and 256 besides)");
    Check(!framemem::Fits(64, 2400, 127) && framemem::Fits(64, 2400, 128), "64 MB needs a piece of 128 MB (itself and 64 to spare)");
    Check(framemem::Fits(32, 330, 100) && !framemem::Fits(48, 330, 100), "330 MB free, the largest piece 100: 32 MB fits, 48 doesn't");
}

void MeterAndNet() {
    printf("2  the meter and the net\n");
    framemem::Meter m;
    Check(m.RoomForScope() && m.Size() == 0, "nothing measured yet: room");
    m.EndOfFrame(11 * kMb, 16 * kMb);
    Check(m.LastPeak() == 11 * kMb && m.LastWithout() == 11 * kMb && m.ScopeShare() == 2 * kMb, "a frame of 11 MB, no scope yet: its share guessed 2 MB");
    Check(m.RoomForScope(), "11 + 2 of 16 MB (81 percent): room");
    m.EndOfFrame(12 * kMb, 16 * kMb);
    Check(!m.RoomForScope(), "12 + 2 of 16 MB (87.5 percent): none");
    const uint32_t share = 1363149;  // 1.3 MB
    m.NoteScope(share);
    m.EndOfFrame(12 * kMb + share, 16 * kMb);
    Check(m.LastWithout() == 12 * kMb && m.ScopeShare() == share && m.RoomForScope(), "the scope measured at 1.3 MB in a frame of 13.3: without it 12.0, with it 83 percent: room");
    m.EndOfFrame(12 * kMb + kMb / 2, 16 * kMb);
    Check(m.LastWithout() == 12 * kMb + kMb / 2 && m.ScopeShare() == share && !m.RoomForScope(), "the next frame 12.5 MB with no scope in it: 12.5 + 1.3 (86 percent): none");
    m.SizeNow(64 * kMb);
    Check(m.RoomForScope(), "the same in a 64 MB block: room");
    const uint32_t edge = static_cast<uint32_t>(static_cast<uint64_t>(64 * kMb) * 85 / 100) - share;
    m.EndOfFrame(edge, 64 * kMb);
    const bool at = m.RoomForScope();
    m.EndOfFrame(edge + 1, 64 * kMb);
    Check(at && !m.RoomForScope(), "85 percent to the byte: room at it, none a byte past");
    m.EndOfFrame(65 * kMb, 64 * kMb);
    const framemem::Meter::Second s = m.TakeSecond();
    Check(s.frames == 7 && s.peak == 65 * kMb && s.size == 64 * kMb && s.scope == share && s.over == 1, "the second: %u frames, peak %.1f MB, the scope %.1f MB, %u past the whole", s.frames,
          s.peak / double(kMb), s.scope / double(kMb), s.over);
    const framemem::Meter::Second next = m.TakeSecond();
    Check(next.frames == 0 && next.peak == 0 && next.scope == 0 && next.over == 0 && next.size == 64 * kMb, "the next second starts empty");
    m.NoteScope(0);
    Check(m.ScopeShare() == share, "a share of 0 (not read) doesn't replace the last one measured");
}

void NotAsRead() {
    printf("3  the game's code not as read; frame_memory_mb=0\n");
    int hooked = 0, read = 0, said = 0, crashLine = 0;
    for (int piece = 0; piece <= kPieces; ++piece)
        for (int unreadable = 0; unreadable < 2; ++unreadable) {
            Fresh(64);
            g_oddPiece = piece;
            g_oddUnreadable = unreadable != 0;
            g_game->Now()->used = 5 * kMb;
            for (int frame = 0; frame < 2400; ++frame)
                framemem::OnFrame();
            hooked += !g_hooking.events.empty();
            read += framemem::Used() != 0;
            said += Said("the game's code isn't as it was read") == 1 && Said("Frame memory:") == 1;
            framemem::LogForCrash();
            crashLine += Said("CRASH? frame memory: not read") == 1;
        }
    const int cases = (kPieces + 1) * 2;
    Check(hooked == 0, "%d pieces of the game's code, each in turn a byte off or unreadable: nothing hooked in any of the %d cases", kPieces + 1, cases);
    Check(read == 0, "the block never read");
    Check(said == cases, "said once in each, after half a minute of frames, and nothing else (%d of %d)", said, cases);
    Check(crashLine == cases, "the crash record says it wasn't read (%d of %d)", crashLine, cases);

    Fresh(0);
    g_game->Now()->used = 5 * kMb;
    for (int frame = 0; frame < 100; ++frame)
        framemem::OnFrame();
    Check(g_hooking.events.empty() && Said("left as the game has it ([world] frame_memory_mb=0)") == 1 && g_lines.size() == 1, "frame_memory_mb=0: nothing hooked, said once");
    Check(framemem::Used() == 5 * kMb && framemem::RoomForScope(), "the block's use still read (5 MB); nothing measured, so the net never bites");
}

void Hooks() {
    printf("4  hooked\n");
    Hooked();
    Check(Events() == "create settings, create end, enable settings, enable end", "the hooks: %s", Events().c_str());
    Check(Said("Frame memory: hooked: the block is made 64 MB at the next frame's end") == 1 && framemem::g_endFrame == &GameEndFrame && framemem::g_settings == &GameSettings, "said, with the size");
    for (int frame = 0; frame < 200; ++frame)
        framemem::OnFrame();
    Check(g_hooking.events.size() == 4, "not made again in the frames after");

    Fresh(64);
    g_hooking.createEnd = MH_ERROR_MEMORY_ALLOC;
    framemem::OnFrame();
    Check(Events() == "create settings, create end, remove settings" && Said("could not hook the shader system's end of frame") == 1, "the end of frame's hook can't be made: %s", Events().c_str());
    Fresh(64);
    g_hooking.createSettings = MH_ERROR_MEMORY_ALLOC;
    framemem::OnFrame();
    Check(Events() == "create settings" && Said("could not hook the taking of waiting settings") == 1, "the settings' hook can't be made: %s", Events().c_str());
    Fresh(64);
    g_hooking.enableEnd = MH_ERROR_MEMORY_PROTECT;
    framemem::OnFrame();
    Check(Events() == "create settings, create end, enable settings, enable end, disable settings, remove settings, remove end" && Said("could not switch its hooks on") == 1,
          "the end of frame's hook can't be switched on: %s", Events().c_str());
    Fresh(64);
    g_hooking.enableSettings = MH_ERROR_MEMORY_PROTECT;
    framemem::OnFrame();
    Check(Events() == "create settings, create end, enable settings, disable settings, remove settings, remove end", "the settings' hook can't be switched on: the end of frame's never is (%s)",
          Events().c_str());
}

void FirstFrames() {
    printf("5  the first frames\n");
    Hooked();
    Block* b = g_game->Now();
    const uint32_t first = static_cast<uint32_t>(11.2 * kMb);
    void* back = Frame(first);
    Check(back == &g_out && g_game->endCalls == 1, "the game's own end of frame run once, its return handed on");
    Check(g_game->seen.start == 0x30000000u && g_game->seen.size == 64 * kMb && g_game->seen.owned == 0 && g_game->seen.used == first,
          "the game's own code found the block already changed (start %08X, %u MB, owned %u) and still holding the frame's %.1f MB", g_game->seen.start, g_game->seen.size / kMb,
          g_game->seen.owned, g_game->seen.used / double(kMb));
    Check(b == g_game->Now() && b->start == 0x30000000u && b->size == 64 * kMb && b->owned == 0 && b->used == 0, "the same block object: a buffer of the mod's, 64 MB, not owned, emptied");
    Check(b->alignment == 16 && b->allocator == kAllocator && b->name == kName && b->spare == 0, "its alignment, allocator and name as they were");
    Check(Get<uint32_t>(g_game->inUse, framemem::kSettingsSize) == 64 * kMb && Get<uint32_t>(g_game->inUse, 0x9C) == 4, "the settings in use say 64 MB, nothing else of them touched");
    Check(g_memory.takes.size() == 1 && g_memory.takes[0] == 64 * kMb && g_memory.spaceAsked == 1, "one buffer of 64 MB asked of the system");
    Check(g_memory.gameFrees.empty(), "the game's first buffer not given back yet");
    Check(Said("the block made 64 MB in place (it was 16 MB at F61B0000; now at 30000000") == 1, "said");
    Frame(20 * kMb);
    Check(g_memory.gameFrees.size() == 1 && g_memory.gameFrees[0].first == kAllocator && g_memory.gameFrees[0].second == kFirstBuffer,
          "the second frame's end: the game's first buffer (F61B0000) given back to its allocator (022E8BE0)");
    Check(Said("the game's first buffer given back to its allocator") == 1, "said");
    const size_t lines = g_lines.size();
    for (int frame = 0; frame < 500; ++frame)
        Frame(20 * kMb);
    Check(g_memory.gameFrees.size() == 1 && g_memory.takes.size() == 1 && g_memory.given.empty() && g_memory.spaceAsked == 1 && g_lines.size() == lines && g_game->endCalls == 502,
          "500 frames more: nothing asked, nothing given back, nothing said, the game's own code run each time");
    Check(b->start == 0x30000000u && b->size == 64 * kMb && b->owned == 0, "the block as the mod left it");

    Hooked();
    g_game->Post(16 * kMb);
    Frame(kMb);
    Check(Get<uint32_t>(g_game->waiting, framemem::kSettingsSize) == 16 * kMb && Get<uint32_t>(g_game->inUse, framemem::kSettingsSize) == 64 * kMb,
          "settings waiting at that moment are not written from the render side (they are told when they're taken)");
    framemem::SettingsHook(g_game->system, nullptr);
    Check(g_game->kept == 1 && g_game->remade == 0 && g_game->Now()->size == 64 * kMb, "taken at the next frame's start: the block kept");

    // Another shader system than the one the game's holder names (the keeper knows one block).
    Hooked();
    Game other;
    other.Start();
    Block* theirs = other.Now();
    theirs->used = 9 * kMb;
    back = framemem::EndFrameHook(other.system, nullptr, &g_out);
    other.Post(32 * kMb);
    framemem::SettingsHook(other.system, nullptr);
    Check(back == &g_out && g_game->endCalls == 1 && g_game->settingsCalls == 1 && theirs->size == 16 * kMb && theirs->owned == 1 && theirs->start == kFirstBuffer && g_memory.takes.empty() &&
              Get<uint32_t>(other.inUse, framemem::kSettingsSize) == 32 * kMb && g_game->remade == 1 && !framemem::g_meter.Size() && g_lines.size() == 1,
          "a shader system that isn't the one the game's holder names: the game's own code run (its settings and its remaking its own), nothing of the mod's done, measured or said");
}

void Lines() {
    printf("6  the once-a-second line and the warnings\n");
    Hooked();
    Frame(static_cast<uint32_t>(11.2 * kMb));
    framemem::NoteScopeShare(static_cast<uint32_t>(1.4 * kMb));
    Frame(static_cast<uint32_t>(11.2 * kMb));
    framemem::g_lastLine = 0;
    framemem::OnFrame();
    Check(Said("Frame memory: peak 11.2 of 64 MB (the scope's view 1.4)") == 1 && Said("warning") == 0, "\"Frame memory: peak 11.2 of 64 MB (the scope's view 1.4)\", no warning");
    framemem::g_lastLine = 0;
    framemem::OnFrame();
    Check(Said("Frame memory: peak") == 1, "no frames in a second: no line");
    Frame(48 * kMb);
    framemem::g_lastLine = 0;
    framemem::OnFrame();
    Check(Said("Frame memory: peak 48.0 of 64 MB (the scope's view 0.0)") == 1 && Said("warning") == 0, "48 of 64 MB (75 percent to the byte): no warning, the scope's view 0.0 when not dispatched");
    Frame(48 * kMb + 1);
    framemem::g_lastLine = 0;
    framemem::OnFrame();
    Check(Said("Frame memory: warning: a frame took 48.0 of 64 MB, more than three quarters of the block") == 1, "a byte more: the warning");
    Frame(65 * kMb);
    Frame(70 * kMb);
    framemem::g_lastLine = 0;
    framemem::OnFrame();
    Check(Said("Frame memory: peak 70.0 of 64 MB") == 1 && Said("Frame memory: warning: 2 frames asked for more than the whole block (70.0 of 64 MB at the most)") == 1,
          "two frames past the whole block: said, with the count");
    framemem::OnFrame();
    framemem::OnFrame();
    Check(Said("Frame memory: peak") == 4, "a second not yet over: no line");
}

void Fallbacks() {
    printf("7  no piece of 64 MB\n");
    Hooked();
    g_memory.refuseAboveMb = 48;
    Frame(kMb);
    Block* b = g_game->Now();
    Check(b->size == 48 * kMb && b->owned == 0 && b->start == 0x30000000u && g_memory.takes.size() == 2 && Get<uint32_t>(g_game->inUse, framemem::kSettingsSize) == 48 * kMb,
          "the system has no piece of 64 MB: 48 taken, the settings say 48");
    Check(Said("the system gave no piece of 64 MB") == 1 && Said("the block made 48 MB in place") == 1, "both said");

    Hooked();
    g_memory.freeMb = 330;
    g_memory.largestMb = 100;
    Frame(kMb);
    b = g_game->Now();
    Check(b->size == 32 * kMb && g_memory.takes.size() == 1 && g_memory.takes[0] == 32 * kMb, "330 MB free, the largest piece 100: 64 and 48 never asked for, 32 taken");
    Check(Said("64 MB not taken: the address space has 330 MB free and its largest piece is 100 MB") == 1 && Said("48 MB not taken") == 1 && Said("the block made 32 MB in place") == 1, "each said");

    Hooked();
    g_memory.refuseAboveMb = 0;
    const Block before = *g_game->Now();
    uint8_t settingsBefore[0x120];
    std::memcpy(settingsBefore, g_game->inUse, sizeof(settingsBefore));
    Frame(3 * kMb);
    Block after = *g_game->Now();
    after.used = before.used;
    Check(std::memcmp(&before, &after, sizeof(before)) == 0 && std::memcmp(settingsBefore, g_game->inUse, sizeof(settingsBefore)) == 0 && g_game->endCalls == 1,
          "nothing to be had: the block and the settings bit for bit as the game made them, the game's own code run");
    Check(g_memory.takes.size() == 3 && Said("the system gave no piece of") == 3 && Said("nothing bigger to be had: the block stays the game's own 16 MB") == 1, "64, 48 and 32 asked for, each said, then left");
    for (int frame = 0; frame < 300; ++frame)
        Frame(3 * kMb);
    Check(g_memory.takes.size() == 3 && g_memory.gameFrees.empty() && g_memory.given.empty() && g_memory.spaceAsked == 1, "300 frames more: not tried again, nothing given back");
    Frame(static_cast<uint32_t>(12.5 * kMb));
    framemem::g_lastLine = 0;
    framemem::OnFrame();
    Check(Said("Frame memory: peak 12.5 of 16 MB (the scope's view 0.0)") == 1 && Said("warning: a frame took 12.5 of 16 MB") == 1, "still measured: 12.5 of 16 MB, warned of");
    Check(!framemem::RoomForScope(), "and the net bites: 12.5 + 2 of 16 MB");
}

void Settings() {
    printf("8  settings waiting to be taken\n");
    Hooked();
    Frame(kMb);
    Frame(kMb);
    Block* b = g_game->Now();
    g_game->Post(16 * kMb);
    framemem::SettingsHook(g_game->system, nullptr);
    Check(g_game->settingsCalls == 1 && g_game->kept == 1 && g_game->remade == 0 && g_game->Now() == b && b->size == 64 * kMb && b->start == 0x30000000u,
          "settings came with the game's own 16 MB: the game's code kept the block");
    Check(Get<uint32_t>(g_game->inUse, framemem::kSettingsSize) == 64 * kMb && Said("new shader settings came with 16 MB for the block") == 1, "the settings in use say 64 MB after; said");
    g_game->Post(64 * kMb);
    const size_t lines = g_lines.size();
    framemem::SettingsHook(g_game->system, nullptr);
    Check(g_game->kept == 2 && g_lines.size() == lines, "settings that agree already: nothing changed, nothing said");
    framemem::SettingsHook(g_game->system, nullptr);
    Check(g_game->settingsCalls == 3 && g_game->kept == 2 && g_game->remade == 0, "none waiting: the game's own code run, nothing else");
    // The same game code without the mod's hook: the trap the hook is for.
    g_game->Post(16 * kMb);
    GameSettings(g_game->system, nullptr);
    Check(g_game->remade == 1 && g_game->Now() != b && g_game->Now()->size == 16 * kMb && g_memory.gameFrees.size() == 1,
          "the same without the mod's hook: the game throws the block away and makes one of 16 MB (and never frees the mod's buffer: the block didn't own it)");

    Hooked();
    g_memory.refuseAboveMb = 0;
    Frame(kMb);  // nothing to be had: the block is the game's own
    b = g_game->Now();
    g_game->Post(32 * kMb);
    framemem::SettingsHook(g_game->system, nullptr);
    Check(g_game->remade == 1 && Get<uint32_t>(g_game->inUse, framemem::kSettingsSize) == 32 * kMb && Said("new shader settings") == 0 && g_memory.gameFrees.size() == 1 &&
              g_memory.gameFrees[0].second == kFirstBuffer,
          "a block that isn't the mod's: the settings and the game's remaking left to the game");
    g_memory.refuseAboveMb = 0xFFFFFFFFu;
    Frame(kMb);
    Check(g_game->Now() != b && g_game->Now()->size == 64 * kMb && g_game->Now()->owned == 0, "the game's new block (32 MB) made 64 at the next frame's end");
}

void Remade() {
    printf("9  the game making its block anew\n");
    Hooked();
    Frame(kMb);
    Frame(kMb);
    Block* first = g_game->Now();
    Block* second = g_game->Make(0xF41B0000u, 16 * kMb);
    g_game->Use(second);
    Frame(kMb);
    Check(second->size == 64 * kMb && second->start == 0x38000000u && second->owned == 0 && g_memory.takes.size() == 2, "a new block of 16 MB in the shader system: made 64 MB at its first frame's end");
    Check(g_memory.given.empty() && Said("the game has made its block anew") == 1, "the mod's first buffer not given back yet; said");
    Check(first->start == 0x30000000u && first->size == 64 * kMb, "the old block object not written to");
    Frame(kMb);
    Check(g_memory.given.size() == 1 && g_memory.given[0] == 0x30000000u && Said("the mod's buffer of the block that went given back to the system") == 1,
          "a frame later: the mod's first buffer given back to the system, once");
    Check(g_memory.gameFrees.size() == 2 && g_memory.gameFrees[1].first == kAllocator && g_memory.gameFrees[1].second == 0xF41B0000u, "and the new block's first buffer back to the game's allocator");
    for (int frame = 0; frame < 100; ++frame)
        Frame(kMb);
    Check(g_memory.given.size() == 1 && g_memory.gameFrees.size() == 2 && g_memory.takes.size() == 2, "100 frames more: nothing else");

    // The same block object given another buffer (remade at the same address).
    Hooked();
    Frame(kMb);
    Frame(kMb);
    first = g_game->Now();
    *first = Block{0xF31B0000u, 16 * kMb, 16, 1, 0, 0, kAllocator, kName};
    Frame(kMb);
    Check(first->size == 64 * kMb && first->start == 0x38000000u && first->owned == 0 && Said("the game has made its block anew") == 1, "the same object with a buffer of the game's again: made 64 MB again");
    Frame(kMb);
    Check(g_memory.given.size() == 1 && g_memory.given[0] == 0x30000000u, "the mod's first buffer given back a frame later");

    // Made anew before the game's first buffer has gone back: by the same allocator, and by another.
    Hooked();
    Frame(kMb);
    g_game->Use(g_game->Make(0xF41B0000u, 16 * kMb));
    Frame(kMb);
    Check(g_memory.gameFrees.size() == 1 && g_memory.gameFrees[0].second == kFirstBuffer && g_game->Now()->size == 64 * kMb,
          "made anew within a frame of the change: the game's first buffer still given back (the new block names the same allocator)");
    Hooked();
    Frame(kMb);
    g_game->Use(g_game->Make(0xF41B0000u, 16 * kMb, 0x022E9110u));
    Frame(kMb);
    Check(g_memory.gameFrees.empty() && Said("the game's first buffer left where it is") == 1, "the new block names another allocator: the game's first buffer left, said");
    Frame(kMb);
    Check(g_memory.gameFrees.size() == 1 && g_memory.gameFrees[0].first == 0x022E9110u && g_memory.gameFrees[0].second == 0xF41B0000u, "the new block's own first buffer back to its own allocator");
}

void OddBlocks() {
    printf("10 no block, a block not the game's make, big enough already, unreadable\n");
    Hooked();
    g_game->Use(nullptr);
    void* back = Frame(0);
    Check(back == &g_out && g_game->endCalls == 1 && g_memory.takes.empty() && g_lines.size() == 1, "no block: the game's own code run, nothing else");

    Hooked();
    g_game->Now()->owned = 0;
    Frame(kMb);
    Check(g_memory.takes.empty() && g_game->Now()->size == 16 * kMb && Said("the block isn't as the game makes one (buffer F61B0000, owned 0, allocator 022E8BE0): left alone") == 1,
          "a block that doesn't own its buffer: left alone, said");
    for (int frame = 0; frame < 50; ++frame)
        Frame(kMb);
    Check(Said("isn't as the game makes one") == 1 && g_memory.takes.empty(), "once");
    Hooked();
    g_game->Now()->allocator = 0;
    Frame(kMb);
    Check(g_memory.takes.empty() && Said("isn't as the game makes one") == 1, "a block with no allocator: left alone");

    Hooked(16);
    Frame(kMb);
    for (int frame = 0; frame < 50; ++frame)
        Frame(kMb);
    Check(g_memory.takes.empty() && g_memory.spaceAsked == 0 && Said("the game's block is 16 MB, no smaller than the 16 asked for: left as it is") == 1, "frame_memory_mb=16: left as it is, said once");
    Hooked(64);
    g_game->Now()->size = 96 * kMb;
    Frame(kMb);
    Check(g_memory.takes.empty() && g_game->Now()->size == 96 * kMb && g_game->Now()->owned == 1, "a block of 96 MB: never made smaller");

    Hooked();
    Put<uint32_t>(g_game->system, framemem::kBlock, 0x10u);
    back = Frame(0);
    Check(back == &g_out && g_game->endCalls == 1 && Said("a fault reading the game's block at a frame's end: left alone from here on") == 1, "a block that can't be read: caught, the game's own code run");
    g_game->Use(&g_game->blocks[0].block);
    for (int frame = 0; frame < 50; ++frame)
        Frame(kMb);
    Check(g_game->endCalls == 51 && g_memory.takes.empty() && g_game->Now()->size == 16 * kMb && g_lines.size() == 2, "from there on the frame's end is the game's alone");

    Hooked();
    Frame(kMb);
    framemem::g_calls.gameFree = FaultyGameFree;
    const bool out = FrameFaults(kMb);
    Check(out && !framemem::g_stopped.load() && Said("a fault") == 0, "a fault inside code that isn't the mod's (the game's free): not swallowed, it goes on to the crash record");
}

void Calls() {
    printf("11 the calls scopeview and the crash record make\n");
    Hooked();
    Frame(10 * kMb);
    g_game->Now()->used = 3 * kMb;
    Check(framemem::Used() == 3 * kMb, "Used: the block's use now (3 MB)");
    const uint32_t before = framemem::Used();
    g_game->Now()->used += static_cast<uint32_t>(1.4 * kMb);
    framemem::NoteScopeShare(framemem::Used() - before);
    Frame(40 * kMb);
    const framemem::Reading r = framemem::Last();
    Check(framemem::RoomForScope() && r.whole == 64.0f && std::fabs(r.scope - 1.4f) < 0.001f && std::fabs(r.eyes - 38.6f) < 0.001f, "a frame of 40 MB with the scope's 1.4 in it: the eyes' %.1f, the scope's %.1f, of %.0f MB: room",
          r.eyes, r.scope, r.whole);
    Frame(54 * kMb);
    Check(!framemem::RoomForScope(), "the eyes alone at 54 of 64 MB: 54 + 1.4 is past 85 percent (54.4): no room");
    Frame(52 * kMb);
    Check(framemem::RoomForScope(), "52 + 1.4: room again");
    g_game->Now()->used = 7 * kMb;
    framemem::LogForCrash();
    Check(Said("CRASH? frame memory: 7340032 of 67108864 bytes handed out so far this frame (7.0 of 64 MB); the frame before took 52.0 MB") == 1, "the crash record: \"%s\"", g_lines.back().c_str());
    g_game->Now()->used = 64 * kMb + 4096;
    framemem::LogForCrash();
    Check(Said("more than there is, so requests were being refused") == 1, "past the whole block: it says so");
    g_holder = 0;
    framemem::LogForCrash();
    Check(Said("CRASH? frame memory: not read") == 1 && framemem::Used() == 0, "no shader system: not read");
}

void RealMemory() {
    printf("12 the system's own memory, for real\n");
    uint32_t freeMb = 0, largestMb = 0;
    framemem::Space(&freeMb, &largestMb);
    Check(freeMb > 512 && largestMb >= 128 && largestMb <= freeMb && freeMb < 4096, "this process's address space: %u MB free, the largest piece %u MB", freeMb, largestMb);
    auto* buffer = static_cast<uint8_t*>(framemem::Take(64 * kMb));
    bool written = false;
    if (buffer) {
        buffer[0] = 0x5A;
        buffer[64 * kMb - 1] = 0xA5;
        written = buffer[0] == 0x5A && buffer[64 * kMb - 1] == 0xA5 && buffer[32 * kMb] == 0;
    }
    Check(buffer && written && (Address(buffer) & 0xFFFF) == 0, "64 MB taken at %08X, written at both ends", Address(buffer));
    uint32_t freeAfter = 0, largestAfter = 0;
    framemem::Space(&freeAfter, &largestAfter);
    Check(freeAfter + 64 == freeMb || freeAfter + 65 == freeMb || freeAfter + 63 == freeMb, "the address space 64 MB smaller (%u MB free)", freeAfter);
    if (buffer)
        framemem::GiveBack(buffer);
    MEMORY_BASIC_INFORMATION region{};
    VirtualQuery(buffer, &region, sizeof(region));
    Check(buffer && region.State == MEM_FREE, "given back: free again");
}

void LogAndTime() {
    printf("13 the log's words; the time\n");
    // Every line the scenarios above can write, gathered again.
    std::vector<std::string> all;
    const auto gather = [&all] { all.insert(all.end(), g_lines.begin(), g_lines.end()); };
    Hooked();
    Frame(kMb);
    Frame(kMb);
    g_game->Post(16 * kMb);
    framemem::SettingsHook(g_game->system, nullptr);
    g_game->Use(g_game->Make(0xF41B0000u, 16 * kMb, 0x022E9110u));
    Frame(70 * kMb);
    Frame(60 * kMb);
    framemem::g_lastLine = 0;
    framemem::OnFrame();
    framemem::LogForCrash();
    gather();
    Hooked();
    g_memory.refuseAboveMb = 0;
    g_memory.freeMb = 330;
    Frame(13 * kMb);
    framemem::g_lastLine = 0;
    framemem::OnFrame();
    gather();
    Hooked();
    g_game->Now()->owned = 0;
    Frame(kMb);
    Put<uint32_t>(g_game->system, framemem::kBlock, 0x10u);
    Frame(kMb);
    gather();
    Fresh(0);
    framemem::OnFrame();
    gather();
    Fresh(64);
    g_oddPiece = 3;
    for (int frame = 0; frame < 2400; ++frame)
        framemem::OnFrame();
    gather();
    for (int trouble = 0; trouble < 3; ++trouble) {
        Fresh(64);
        (trouble == 0 ? g_hooking.createEnd : trouble == 1 ? g_hooking.createSettings : g_hooking.enableEnd) = MH_ERROR_MEMORY_ALLOC;
        framemem::OnFrame();
        gather();
    }
    int bad = 0;
    for (const std::string& line : all) {
        std::string lower = line;
        for (char& c : lower)
            c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        if (lower.find("fail") != std::string::npos || lower.find("error") != std::string::npos || lower.find("exception") != std::string::npos || lower.find("had no room") != std::string::npos) {
            ++bad;
            printf("      | %s\n", line.c_str());
        }
    }
    Check(bad == 0 && all.size() >= 20, "%u lines of every kind: none with a word the self-test takes for a failure", static_cast<unsigned>(all.size()));

    Hooked();
    Frame(kMb);
    Frame(kMb);
    LARGE_INTEGER frequency, t0, t1;
    QueryPerformanceFrequency(&frequency);
    const int rounds = 2000000;
    QueryPerformanceCounter(&t0);
    for (int i = 0; i < rounds; ++i)
        Frame(20 * kMb);
    QueryPerformanceCounter(&t1);
    const double ns = static_cast<double>(t1.QuadPart - t0.QuadPart) * 1e9 / static_cast<double>(frequency.QuadPart) / rounds;
    Check(ns < 2000.0, "a frame's end: %.0f ns of the mod's (with the made-up game's own code)", ns);
}

}  // namespace

int main() {
    Sizes();
    MeterAndNet();
    NotAsRead();
    Hooks();
    FirstFrames();
    Lines();
    Fallbacks();
    Settings();
    Remade();
    OddBlocks();
    Calls();
    RealMemory();
    LogAndTime();
    printf("%d passed, %d failed\n", g_passed, g_failed);
    return g_failed;
}

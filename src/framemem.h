#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>

// The game's memory for one frame: a single block ("shaderFrame", 16 MB as shipped: ShaderSystemSettings.FrameMemoryBufferSize) out of which everything drawn in a frame takes its draw lists, its
// matrices and its bones, piece by piece, and which is emptied once a frame. Nothing in the game looks at what a request brings back. It was sized for one view; the mod draws two eyes out of it,
// and a third view while a scope is up. Full, the next soldier's bones were copied to address 0 and the game stopped (2026-11-12: a crowd of soldiers, a scope raised). So the block is made
// bigger in place, what each frame takes of it is measured, and the scope's view is left out of a frame that would crowd it.
// bf3vr.ini [world] frame_memory_mb: the block's size in MB (64). 0: the game's own 16, nothing hooked and nothing measured.
namespace framemem {

// Once a frame on the render thread: hooks the game once its code is there (and is as it was read), and writes the once-a-second line "Frame memory: peak X of Y MB (the scope's view Z)".
void OnFrame();
// The bytes handed out of the block so far this frame; 0 when it can't be read. Any thread.
uint32_t Used();
// The scope's second dispatch took this many bytes: the block's use after its two waits less its use before it. The dispatching thread.
void NoteScopeShare(uint32_t bytes);
// The net: false when the last frame's use without the scope's view, plus what that view last took (2 MB while not yet measured), passes 85 percent of the block. The scope's dispatch is then left
// out of this frame (its lens shows the eye's own picture for it). True while nothing has been measured, and once nothing is measured any more (a fault at a frame's end).
bool RoomForScope();
// The numbers that answer came from, in MB, for the line that says so.
struct Reading {
    float eyes, scope, whole;
};
Reading Last();
// The crash record's line: the block's use and size as they stand (guarded reads), and what the frame before took.
void LogForCrash();

// ---- The logic, with nothing of the game's in it: the offline test runs it on a made-up shader system, settings and block. ----

constexpr uint32_t kMb = 1024 * 1024;

// The block as the game's constructor leaves it: 0x1C bytes of a 0x20-byte object, every field 32 bits wide as in the game.
struct Block {
    uint32_t start;      // +00 the buffer
    uint32_t size;       // +04
    uint32_t alignment;  // +08 of every piece (16)
    uint16_t owned;      // +0C the buffer is the allocator's below, and goes back to it with the block
    uint16_t spare;
    uint32_t used;       // +10 handed out so far this frame: every request adds to it, a refused one too
    uint32_t allocator;  // +14
    uint32_t name;       // +18
};
static_assert(sizeof(Block) == 0x1C, "the game's own layout");

// In the shader system: its settings in use, the settings waiting to be taken, the block. In a settings object: the block's size.
constexpr size_t kSettings = 0x58, kWaiting = 0x5C, kBlock = 0x60, kSettingsSize = 0x24;

template <class T>
T* At(uint32_t address) {
    return reinterpret_cast<T*>(static_cast<uintptr_t>(address));
}
inline uint32_t Word(const uint8_t* at) {
    uint32_t v;
    std::memcpy(&v, at, sizeof(v));
    return v;
}
inline void PutWord(uint32_t address, uint32_t v) { std::memcpy(At<uint8_t>(address), &v, sizeof(v)); }

// The sizes to try, in MB, largest first: the one asked for, then 64, 48 and 32 where they're below it; only those bigger than the block is. Returns how many.
inline int Sizes(uint32_t wantedMb, uint32_t haveBytes, uint32_t out[4]) {
    int n = 0;
    for (const uint32_t mb : {wantedMb, 64u, 48u, 32u})
        if (mb && mb <= wantedMb && (n == 0 || mb < out[n - 1]) && static_cast<uint64_t>(mb) * kMb > haveBytes)
            out[n++] = mb;
    return n;
}

// Whether a buffer of this size may be taken with the game left room of its own: a free piece of the address space that big with 64 MB to spare, and 256 MB free besides. (The game answers a
// request its own heap cannot meet by stopping, with no way back: its room is not to be used up.)
inline bool Fits(uint32_t mb, uint32_t freeMb, uint32_t largestMb) { return largestMb >= mb + 64 && freeMb >= mb + 256; }

inline void Raise(std::atomic<uint32_t>& a, uint32_t v) {
    uint32_t was = a.load(std::memory_order_relaxed);
    while (v > was && !a.compare_exchange_weak(was, v, std::memory_order_relaxed)) {
    }
}

// What each frame took of the block. The frame's end (the render side) writes; the scope's dispatch (the client side, always after the frame before has ended) and the once-a-second line read.
class Meter {
public:
    static constexpr uint32_t kScopeGuess = 2 * kMb;  // the scope's view's share while it hasn't been measured

    // Just before the game empties the block: what's been handed out is the whole frame's.
    void EndOfFrame(uint32_t used, uint32_t size) {
        const uint32_t scope = scopeThisFrame_.exchange(0, std::memory_order_relaxed);
        lastPeak_.store(used, std::memory_order_relaxed);
        lastWithout_.store(used - std::min(used, scope), std::memory_order_relaxed);
        size_.store(size, std::memory_order_relaxed);
        frames_.fetch_add(1, std::memory_order_relaxed);
        Raise(peak_, used);
        if (used > size)
            over_.fetch_add(1, std::memory_order_relaxed);
    }
    // The block's size from here on (it may just have been made bigger).
    void SizeNow(uint32_t size) { size_.store(size, std::memory_order_relaxed); }
    void NoteScope(uint32_t bytes) {
        scopeThisFrame_.fetch_add(bytes, std::memory_order_relaxed);
        if (bytes)
            scopeLast_.store(bytes, std::memory_order_relaxed);
        Raise(scope_, bytes);
    }
    bool RoomForScope() const {
        const uint32_t size = size_.load(std::memory_order_relaxed);
        return !size || static_cast<uint64_t>(lastWithout_.load(std::memory_order_relaxed)) + ScopeShare() <= static_cast<uint64_t>(size) * 85 / 100;
    }
    uint32_t ScopeShare() const {
        const uint32_t last = scopeLast_.load(std::memory_order_relaxed);
        return last ? last : kScopeGuess;
    }
    uint32_t LastWithout() const { return lastWithout_.load(std::memory_order_relaxed); }
    uint32_t LastPeak() const { return lastPeak_.load(std::memory_order_relaxed); }
    uint32_t Size() const { return size_.load(std::memory_order_relaxed); }

    // The second's: its frames, the largest a frame took, the largest the scope's view took (0: not dispatched), the frames that asked for more than there is. Taking them starts the next second.
    struct Second {
        uint32_t frames, peak, size, scope, over;
    };
    Second TakeSecond() {
        Second s;
        s.frames = frames_.exchange(0, std::memory_order_relaxed);
        s.peak = peak_.exchange(0, std::memory_order_relaxed);
        s.scope = scope_.exchange(0, std::memory_order_relaxed);
        s.over = over_.exchange(0, std::memory_order_relaxed);
        s.size = size_.load(std::memory_order_relaxed);
        return s;
    }

private:
    std::atomic<uint32_t> size_{0}, lastPeak_{0}, lastWithout_{0}, scopeThisFrame_{0}, scopeLast_{0};
    std::atomic<uint32_t> frames_{0}, peak_{0}, scope_{0}, over_{0};
};

// What the keeper asks of the system and of the game (the test's are made up).
struct Calls {
    void* (*take)(uint32_t bytes);                          // a buffer of the mod's own; null when the system has no piece that big
    void (*giveBack)(void* buffer);                         // one of those, back
    void (*gameFree)(uint32_t allocator, uint32_t buffer);  // the game's own free of a buffer of its allocator's
    void (*space)(uint32_t* freeMb, uint32_t* largestMb);   // the address space still free, and its largest piece
};

// What a frame's end did, for the log.
struct Report {
    uint32_t enlargedTo = 0, was = 0;         // bytes: the block's new size and the one it had, when it was made bigger this frame ("was" also with "enough" and "gaveUp")
    uint32_t newStart = 0, oldStart = 0;
    uint32_t passed[4] = {}, passedCount = 0;  // the sizes (MB) passed over on the way
    bool noPiece[4] = {};                     // of each: true, the system gave none; false, taking it would have left the game too little room
    uint32_t freeMb = 0, largestMb = 0;       // the address space when it was looked at
    bool gaveUp = false;                      // nothing bigger to be had: the block stays as the game made it, and isn't tried again
    bool enough = false;                      // the block is as big as asked already
    bool strange = false;                     // the block isn't as the game makes one (no buffer of its allocator's): left alone
    bool oldFreed = false, oldLeft = false;   // the game's first buffer, a frame after the change: given back to its allocator, or left (no live block names that allocator)
    bool remade = false;                      // the block the mod made bigger is gone, or holds another buffer: the game has made it anew
    bool ownFreed = false;                    // the mod's buffer of a block that went, given back to the system
};

// Makes the block bigger in place, at the one moment nothing is in it: the shader system's end of frame, before the game's own code empties it. Every draw list of the frame has been drawn by
// then, and the next frame's dispatch has not begun (it waits for this one's render to end). The block object stays the same (the world renderers keep its address); it gets a buffer of the
// mod's own and stops owning it, so the game never hands it to its allocator; the game's first buffer goes back to that allocator a frame later, when nothing can still be reading it.
class Keeper {
public:
    Report EndOfFrame(uint8_t* shaderSystem, uint32_t wantedMb, const Calls& calls) {
        Report r;
        const uint32_t blockAt = Word(shaderSystem + kBlock);
        auto* block = At<Block>(blockAt);
        // A buffer of the mod's whose block went, a frame ago: back to the system.
        if (orphan_) {
            calls.giveBack(At<void>(orphan_));
            orphan_ = 0;
            r.ownFreed = true;
        }
        if (!block)
            return r;
        // The game's first buffer, swapped out a frame ago: back to its allocator, as the block's own destructor would have given it. Only through an allocator a live block still names.
        if (oldBuffer_) {
            if (block->allocator == oldAllocator_) {
                calls.gameFree(oldAllocator_, oldBuffer_);
                r.oldFreed = true;
            } else {
                r.oldLeft = true;
            }
            oldBuffer_ = oldAllocator_ = 0;
        }
        // The block the mod made bigger gone, or holding another buffer: the game has made its block anew (it does when settings come whose size isn't the block's, and with a new
        // shader system). The mod's buffer goes back a frame from now; the new block is made bigger below.
        if (const uint32_t mine = buffer_.load(std::memory_order_relaxed); mine && (blockAt != block_.load(std::memory_order_relaxed) || block->start != mine)) {
            orphan_ = mine;
            block_.store(0, std::memory_order_relaxed);
            buffer_.store(0, std::memory_order_relaxed);
            r.remade = true;
        }
        if (!wantedMb || buffer_.load(std::memory_order_relaxed) || (blockAt == leftBlock_ && block->start == leftStart_))
            return r;
        r.was = block->size;
        uint32_t sizes[4];
        const int count = Sizes(wantedMb, block->size, sizes);
        if (!count || !block->start || !block->owned || !block->allocator) {
            leftBlock_ = blockAt;
            leftStart_ = block->start;
            (count ? r.strange : r.enough) = true;
            return r;
        }
        calls.space(&r.freeMb, &r.largestMb);
        void* taken = nullptr;
        uint32_t bytes = 0;
        for (int i = 0; i < count && !taken; ++i) {
            const bool fits = Fits(sizes[i], r.freeMb, r.largestMb);
            if (fits) {
                bytes = sizes[i] * kMb;
                taken = calls.take(bytes);
            }
            if (!taken) {
                r.passed[r.passedCount] = sizes[i];
                r.noPiece[r.passedCount++] = fits;
            }
        }
        if (!taken) {
            leftBlock_ = blockAt;
            leftStart_ = block->start;
            r.gaveUp = true;
            return r;
        }
        oldBuffer_ = r.oldStart = block->start;
        oldAllocator_ = block->allocator;
        const auto start = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(taken));
        // The start before the size: a request is checked against the size and then given a place from the start, so one that came in between would be checked against the old size and placed in
        // the new buffer, or checked and placed by the new: never placed past a buffer's end. Through a volatile pointer, so that the stores are made, and in this order: written plainly the
        // compiler is free to turn them round (it had put "owned" first), and a size stored before its start would let such a request be placed past the old buffer's end.
        volatile Block* const live = block;
        live->start = start;
        live->size = bytes;
        live->owned = 0;
        // The settings in use say the same size: whoever copies them to post a change carries it, and the game throws a block away whose size isn't its settings' when it takes new ones.
        // (Settings waiting are told when they're taken, at the frame's start on the client side: BeforeSettings. From here, the render side, the store could land on settings just replaced.)
        if (const uint32_t settings = Word(shaderSystem + kSettings))
            PutWord(settings + static_cast<uint32_t>(kSettingsSize), bytes);
        block_.store(blockAt, std::memory_order_relaxed);
        buffer_.store(start, std::memory_order_relaxed);
        r.enlargedTo = bytes;
        r.newStart = start;
        return r;
    }

    // Before the game takes settings that were waiting (the frame's start, the client side): they're copied over the ones in use, and a block whose size isn't theirs is thrown away and made anew
    // at theirs, while the world renderers go on holding the old one's address. So settings that come with another size are given the block's, and the block stays. Only for a block the mod made
    // bigger. True when they were changed; the size they came with in "came".
    bool BeforeSettings(uint8_t* shaderSystem, uint32_t* came) {
        const uint32_t waiting = Word(shaderSystem + kWaiting), blockAt = Word(shaderSystem + kBlock);
        if (!waiting || waiting == Word(shaderSystem + kSettings) || !blockAt || blockAt != block_.load(std::memory_order_relaxed))
            return false;
        const Block* block = At<Block>(blockAt);
        const uint32_t theirs = Word(At<uint8_t>(waiting + static_cast<uint32_t>(kSettingsSize)));
        if (block->start != buffer_.load(std::memory_order_relaxed) || theirs == block->size)
            return false;
        PutWord(waiting + static_cast<uint32_t>(kSettingsSize), block->size);
        *came = theirs;
        return true;
    }

    // The mod's buffer in the block now (0: none).
    uint32_t Buffer() const { return buffer_.load(std::memory_order_relaxed); }

private:
    std::atomic<uint32_t> block_{0}, buffer_{0};  // the block made bigger, and the mod's buffer in it (read by the settings' side too)
    uint32_t oldBuffer_ = 0, oldAllocator_ = 0;   // the game's first buffer, waiting a frame
    uint32_t orphan_ = 0;                         // a buffer of the mod's whose block went, waiting a frame
    uint32_t leftBlock_ = 0, leftStart_ = 0;      // a block left as it is (big enough, not the game's make, or nothing bigger to be had): not looked at again while it's the same
};

}

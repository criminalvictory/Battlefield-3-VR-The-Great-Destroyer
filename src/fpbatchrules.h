#pragma once

// The first-person batches probe (weapon.txt "fpbatchlog 1"; fpbatch.cpp runs it, tools\fpbatchtest.cpp tests these rules without the
// game). Read-only research for the floating ACOG and the dark magazine copy: does the game draw the held gun, its attachments and the
// arms as separate batches, each with a camera of its own, that the mod could move whole? The plan: docs\research\fp_batches.md.
//
// What was read in bf3.exe 1.6.0.0 (build\bf3_mem.exe, not relocated), 2026-11-25:
//   0x006BBFC0  a camera block's transform set (thiscall: the block in ECX, a pointer to 16 floats on the stack; ret 4). It compares the
//               first three columns of each of the four rows with the block's own (cmpeqps, movmskps & 7) and, if any differs, copies all
//               16 (rep movsd) and ORs 9 into the block's +0x80 (its matrices worked out again). 24 callers.
//   0x010181B0  the first-person batch's camera (ret 10h: the batch, the frame's view list, the view's entry, the draw state). EBX holds the
//               batch from its sixth instruction (0x010181BD) to the end. For each sub-view (the loop at 0x01018310: [esp+10h] its number,
//               [esp+1Ch] the last one's) a camera block on its stack ([esp+170h]) is made from the view's main camera (ViewDesc +0x30,
//               0x006C0810); with the batch's flag 1, where the view allows it, the batch's own FOV (+0xB4, 0x006BBE00) and the view's
//               first-person camera (ViewDesc +0x1080, which the view builder copies from its source's +0xD90) are set on it: the transform at
//               0x010183B5, returning to 0x010183BA. Then, without flag 4 and with the batch's origin (+0x60) more than about 3 cm from 0 (its
//               x*x + y*y + z*z over the float 0.001 at 0x0207FCFC: worked out at 0x01018215 into the byte [esp+0Eh], tested at 0x010183C3),
//               the block's transform less the origin is set on it (0x0101841D, returning to 0x01018422): the batch is drawn in its own space
//               about its origin. So the view's own first-person camera stays in the block, and the batch's per-view block is made from it,
//               only with flag 4 or an origin within those 3 cm; otherwise the transform less the origin is set over it at once (BlockKeeps).
//               Each sub-view's matrices go to a 0x160-byte block; the first is the draw state's per-view block ([state+0x3A0]+0x20).
//   0x0101ACD0  its one caller (the call at 0x0101AD74, returning to 0x0101AD79): each view entry of the frame's list (0x18 bytes: +4 the
//               ViewDesc, +8 a kind, +0xC how many batches, +0x10 their pointers), each of its batches.
// The batch as 0x010181B0 reads it: +0x10 its transform (four rows of four), +0x60 its origin, +0x78 / +0x7C its meshes (8 bytes each: the
// mesh, a word) and how many, +0x80 / +0x84 a second list taken instead in some views (the byte at 0x02386160 set), +0x88 its parameters,
// +0xA0 a bounding sphere (its draw distance and level of detail), +0xB4 its FOV in degrees (-1: the view's own), +0xBC its flags: 1 the
// view's first-person camera and the batch's FOV, 2 its meshes drawn through 0x017326C0 with +0x70 as a count (instanced, it seems), 4 kept
// at the view's camera, never made relative to its origin (its first mesh drawn with its own bones composed with the transform), 0x10 drawn
// even in a view that otherwise leaves a flag-1 batch out.
// A ViewDesc's +4 names it ("Root", "SecondaryRoot": the second eye; the view builder writes them at 0x01018E8C and 0x01019275).

#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <type_traits>

namespace fpbatch {

// ---- The game's code ----

constexpr uint32_t kSetter = 0x006BBFC0;              // the camera block's transform set
constexpr uint32_t kBatchCamera = 0x010181B0;         // the first-person batch's camera
constexpr uint32_t kRelReturn = 0x01018422;           // after the set of the transform made relative to the batch's origin
constexpr uint32_t kAbsReturn = 0x010183BA;           // after the set of the view's own first-person camera
constexpr uint32_t kBatchCameraReturn = 0x0101AD79;   // where 0x010181B0 returns to in its one caller

// Every place the probe goes by, as read: the hook is made only if each is so in the running game (code bytes, as the decrypted image has
// them; the setter whole, so nothing else has patched it; the one number the origin is tested against).
struct Code {
    uint32_t at;
    const char* bytes;  // hex, a space between bytes
    const char* what;
};
inline constexpr Code kCode[] = {
    {0x006BBFC0,
     "55 8B EC 83 E4 F0 83 EC 08 8B C1 0F 28 48 30 0F 28 50 20 0F 28 18 56 8B 75 08 0F 28 46 30 0F C2 C1 00 0F 28 4E 20 0F C2 CA 00 0F 28 "
     "50 10 0F 54 C1 0F 28 4E 10 0F C2 CA 00 0F 28 16 0F C2 D3 00 0F 54 CA 0F 54 C1 0F 50 C8 83 E1 07 57 80 F9 07 74 10 B9 10 00 00 00 8B "
     "F8 F3 A5 83 88 80 00 00 00 09 5F 5E 8B E5 5D C2 04 00",
     "the transform set, whole: rows compared, 16 dwords copied, +0x80 |= 9, ret 4"},
    {0x010181B0, "55 8B EC 83 E4 F0 81 EC 64 0B 00 00 53 8B 5D 08 F6 83 BC 00 00 00 01 56 8B 75 10 57",
     "the batch camera's start: EBX = its first argument, the flags at +0xBC, ESI = the view's entry"},
    {0x01018215,
     "0F 28 43 60 0F 59 C0 0F 28 D0 0F 28 C8 0F C6 C8 55 0F C6 D0 00 0F C6 C0 AA 0F 58 CA 0F 58 C8 0F 2F 0D FC FC 07 02 C6 44 24 0E 01 77 05 "
     "C6 44 24 0E 00",
     "the batch's origin at +0x60: [esp+0Eh] set when its x*x + y*y + z*z is over the float at 0x0207FCFC"},
    {0x0207FCFC, "6F 12 83 3A", "that float: 0.001"},
    {0x01018247, "8B 46 08", "the view entry's kind at +8"},
    {0x01018274, "89 7C 24 20", "the last sub-view's number at [esp+1Ch]"},
    {0x010182FA, "C7 44 24 10 00 00 00 00", "the sub-view's number at [esp+10h]"},
    {0x0101832E, "F3 0F 10 83 B4 00 00 00", "the batch's FOV at +0xB4"},
    {0x010183A4, "8B 56 04 81 C2 80 10 00 00 52 8D 8C 24 74 01 00 00 E8 06 3C 6A FF F6 83 BC 00 00 00 04 75 5F",
     "the view's first-person camera (ViewDesc +0x1080) set, returning to 0x010183BA; flag 4"},
    {0x010183C3, "80 7C 24 0E 00 74 58", "without [esp+0Eh] (the origin within about 3 cm of 0) the transform less the origin isn't set"},
    {0x010183CA,
     "66 0F 6F 84 24 70 01 00 00 0F 28 4B 60 66 0F 7F 44 24 20 66 0F 6F 84 24 80 01 00 00 66 0F 7F 44 24 30 66 0F 6F 84 24 90 01 00 00 66 0F "
     "7F 44 24 40 0F 28 84 24 A0 01 00 00 66 0F 7F 44 24 50 8D 44 24 20 0F 5C C1 50 8D 8C 24 74 01 00 00 0F 29 44 24 54 E8 9E 3B 6A FF 83 7C "
     "24 1C 02",
     "the transform less the origin set from [esp+20h], returning to 0x01018422"},
    {0x010188D0, "8B 93 80 00 00 00", "the second list at +0x80"},
    {0x0101891D, "3B B3 84 00 00 00", "its count at +0x84"},
    {0x01018942, "8B 53 78", "the meshes at +0x78"},
    {0x0101898C, "3B 73 7C", "their count at +0x7C"},
    {0x0101AD74, "E8 37 D4 FF FF", "the one call of the batch camera, returning to 0x0101AD79"},
    {0x01018E8C, "C7 43 04 34 40 14 02", "a ViewDesc's name at +4 (\"Root\")"},
    {0x01019275, "C7 47 04 24 40 14 02", "... (\"SecondaryRoot\")"},
    {0x01018FFE, "8D BB 80 10 00 00 B9 10 00 00 00 F3 A5", "a ViewDesc's first-person camera at +0x1080, 16 dwords"},
};
constexpr int kCodeCount = static_cast<int>(sizeof(kCode) / sizeof(kCode[0]));
constexpr size_t kCodeMost = 128;  // the longest entry's bytes

// "55 8B ..." into bytes: how many, 0 if it isn't such a list or longer than most.
inline size_t ParseHex(const char* text, uint8_t* out, size_t most) {
    size_t n = 0;
    const auto digit = [](char c) -> int {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        return -1;
    };
    for (const char* p = text; *p;) {
        if (*p == ' ') {
            ++p;
            continue;
        }
        const int hi = digit(p[0]), lo = hi >= 0 ? digit(p[1]) : -1;
        if (hi < 0 || lo < 0 || (p[2] != ' ' && p[2] != 0) || n >= most)
            return 0;
        out[n++] = static_cast<uint8_t>(hi << 4 | lo);
        p += 2;
    }
    return n;
}

// Whether every place of kCode reads so, by read(at, out, size) (guarded); the first that doesn't, in failed.
template <class Read>
bool CodeAsRead(Read&& read, int* failed = nullptr) {
    for (int i = 0; i < kCodeCount; ++i) {
        uint8_t want[kCodeMost], now[kCodeMost];
        const size_t n = ParseHex(kCode[i].bytes, want, sizeof(want));
        if (!n || !read(kCode[i].at, now, n) || std::memcmp(want, now, n) != 0) {
            if (failed)
                *failed = i;
            return false;
        }
    }
    return true;
}

// ---- One call of the setter, as the hook takes it ----

// From the stack as the setter is entered (its return address at +0): in the batch camera's frame, [esp+10h] the sub-view's number,
// [esp+1Ch] the last one's, [esp+20h] the transform less the origin, [esp+170h] the camera block; one push and the call's return before.
constexpr uint32_t kEntryToSub = 0x18, kEntryToSubLast = 0x24, kEntryToRelMatrix = 0x28, kEntryToCamera = 0x178;
// From the batch camera's EBP: its return address, the batch, the view's entry.
constexpr uint32_t kFrameReturn = 4, kFrameBatch = 8, kFrameViewEntry = 0x10;
// A view entry: its ViewDesc, its kind. A ViewDesc: its name, its first-person camera.
constexpr uint32_t kEntryView = 4, kEntryKind = 8, kViewName = 4, kViewFirstPerson = 0x1080;
// The batch.
constexpr uint32_t kTransform = 0x10, kOrigin = 0x60, kMeshes = 0x78, kMeshCount = 0x7C, kSecond = 0x80, kSecondCount = 0x84, kFov = 0xB4,
                   kFlags = 0xBC;
constexpr uint32_t kHeadBytes = 0xC0;  // the batch's first bytes kept: everything 0x010181B0 reads of it
constexpr int kEntries = 6;            // a list's first entries kept
constexpr float kOriginNear = 1e-3f;   // the game's (0x0207FCFC): an origin whose x*x + y*y + z*z isn't over this is never made relative

struct Call {
    uint32_t batch = 0;
    bool abs = false;  // the view's own first-person camera (kAbsReturn); else the transform less the origin (kRelReturn)
    uint8_t head[kHeadBytes] = {};
    uint32_t entries[2][kEntries][2] = {};  // the meshes' (+0x78) and the second list's (+0x80) first entries
    uint32_t entriesRead[2] = {};           // how many of them were read
    float matrix[16] = {};                  // what is set
    uint32_t viewEntry = 0, view = 0, viewKind = 0;
    char viewName[16] = {};
    uint32_t sub = 0, subLast = 0;
};

inline uint32_t Word(const uint8_t* head, uint32_t at) {
    uint32_t v = 0;
    std::memcpy(&v, head + at, sizeof(v));
    return v;
}
inline float Float(const uint8_t* head, uint32_t at) {
    float v = 0.0f;
    std::memcpy(&v, head + at, sizeof(v));
    return v;
}

// Whether the camera block keeps what this call sets, so that the batch's per-view block is made from it: the transform less the origin
// always (nothing is set after it); the view's own first-person camera only with flag 4 or an origin within about 3 cm of 0, as the game
// tests at 0x010183BA and 0x010183C3 (its sums in its order; an origin that isn't a number never over the 0.001). Otherwise the transform
// less the origin is set over it straight after, and the view's own camera is never uploaded for that batch: every flag-1 batch of the view
// sets that same camera, so a batch noted with it would be taken for another's upload (the review, 2026-11-25).
inline bool BlockKeeps(const Call& c) {
    if (!c.abs || (c.head[kFlags] & 4) != 0)
        return true;
    const float x = Float(c.head, kOrigin), y = Float(c.head, kOrigin + 4), z = Float(c.head, kOrigin + 8);
    return !((y * y + x * x) + z * z > kOriginNear);
}

// A NUL-ended run of printable characters, at least least long with two letters in it, within the n bytes: copied into out (cut to its
// size); its length, 0 if not.
inline size_t LooksLikeName(const uint8_t* bytes, size_t n, char* out, size_t outSize, size_t least = 4) {
    size_t length = 0, letters = 0;
    while (length < n && bytes[length] != 0) {
        const uint8_t c = bytes[length];
        if (c < 0x20 || c > 0x7E)
            return 0;
        if ((c | 0x20) >= 'a' && (c | 0x20) <= 'z')
            ++letters;
        ++length;
    }
    if (length == n || length < least || letters < 2)
        return 0;
    if (out && outSize) {
        const size_t kept = length < outSize - 1 ? length : outSize - 1;
        std::memcpy(out, bytes, kept);
        out[kept] = 0;
    }
    return length;
}

// Page protections the name search reads (fpbatch.cpp's Readable, once a second on the frame's thread): committed memory read the ordinary,
// cached way. Write-combined or uncached pages (memory the game maps for writing to the graphics card: AMD's with Smart Access Memory) and
// guard pages are never read by it. The hook's own reads (the batch camera's frame, the view entry, the ViewDesc and its name, the batch and
// its lists' first entries) aren't checked page by page: they are guarded (a fault is a false), and are the game's own structures on the
// processor's side, each found from the one before as the batch camera reads it.
inline bool ReadableProtect(uint32_t protect) {
    constexpr uint32_t kRead = 0x02 | 0x04 | 0x08 | 0x20 | 0x40 | 0x80;  // PAGE_READONLY, READWRITE, WRITECOPY, EXECUTE_READ, EXECUTE_READWRITE, EXECUTE_WRITECOPY
    constexpr uint32_t kNever = 0x100 | 0x200 | 0x400;                  // PAGE_GUARD, PAGE_NOCACHE, PAGE_WRITECOMBINE
    return (protect & kRead) != 0 && (protect & kNever) == 0;
}

// Why a call wasn't taken.
enum Fault { kTaken, kNotRead, kNotItsCaller, kNotItsBatch, kNotItsCamera, kNotItsMatrix, kFaults };
inline const char* FaultName(int f) {
    static const char* const names[kFaults] = {"taken", "memory not read", "not the batch camera's frame", "EBX not its batch",
                                               "ECX not its camera block", "not the transform it sets"};
    return f >= 0 && f < kFaults ? names[f] : "?";
}

// One call of the setter returning to kRelReturn or kAbsReturn, from its registers (ECX the block, the stack's argument the matrix, EBX,
// EBP) and the stack as it was entered: checked to be the batch camera's frame as read before anything of it is believed, then the batch's
// first bytes, its lists' first entries, the view's entry, kind and name, the sub-view and the matrix read by read(at, out, size), which is
// guarded (a fault is a false, never a crash).
template <class Read>
Fault ReadCall(Read&& read, uint32_t returnAt, uint32_t camera, uint32_t matrix, uint32_t ebx, uint32_t ebp, uint32_t entry, Call& c) {
    uint32_t frame[5] = {};
    if (!read(ebp, frame, sizeof(frame)))
        return kNotRead;
    if (frame[kFrameReturn / 4] != kBatchCameraReturn)
        return kNotItsCaller;
    if (!ebx || frame[kFrameBatch / 4] != ebx)
        return kNotItsBatch;
    if (camera != entry + kEntryToCamera)
        return kNotItsCamera;
    c = Call{};
    c.batch = ebx;
    c.abs = returnAt == kAbsReturn;
    c.viewEntry = frame[kFrameViewEntry / 4];
    uint32_t entryWords[2] = {};
    if (!read(c.viewEntry + kEntryView, entryWords, sizeof(entryWords)))
        return kNotRead;
    c.view = entryWords[0];
    c.viewKind = entryWords[1];
    if (matrix != (c.abs ? c.view + kViewFirstPerson : entry + kEntryToRelMatrix))
        return kNotItsMatrix;
    if (!read(matrix, c.matrix, sizeof(c.matrix)) || !read(ebx, c.head, sizeof(c.head)))
        return kNotRead;
    uint32_t subs[4] = {};
    if (read(entry + kEntryToSub, subs, sizeof(subs))) {
        c.sub = subs[0];
        c.subLast = subs[(kEntryToSubLast - kEntryToSub) / 4];
    }
    const uint32_t lists[2][2] = {{kMeshes, kMeshCount}, {kSecond, kSecondCount}};
    for (int l = 0; l < 2; ++l) {
        const uint32_t at = Word(c.head, lists[l][0]), count = Word(c.head, lists[l][1]);
        const uint32_t n = count < static_cast<uint32_t>(kEntries) ? count : static_cast<uint32_t>(kEntries);
        if (at && n && read(at, c.entries[l], n * 8))
            c.entriesRead[l] = n;
    }
    uint32_t name = 0;
    uint8_t text[16] = {};
    if (c.view && read(c.view + kViewName, &name, sizeof(name)) && name && read(name, text, sizeof(text) - 1))
        LooksLikeName(text, sizeof(text), c.viewName, sizeof(c.viewName), 2);
    return kTaken;
}

// ---- A second's books ----

constexpr int kRecords = 32;      // batches kept a second (a bit each in what a per-view upload matched: MatchView)
constexpr int kCameras = 8;       // a batch's last cameras kept: what's drawn can run a frame or two behind the cameras set
constexpr int kDraws = 16;        // a batch's different draws kept a second
constexpr int kViews = 8;         // the views a batch was seen in, told apart by name and kind (one with no name by its ViewDesc and kind)
constexpr int kViewEntries = 4;   // a view's different view entries kept (more: the frame's view list made again each frame, it seems)
constexpr float kEyeNear = 1e-3f;   // metres: a per-view upload's eye on a batch's camera
constexpr float kRowsNear = 2e-3f;  // its rows on the camera's rows, number by number
constexpr float kArmsReach = 5.0f;  // metres from the origin: a view in the arms' space (viewturn::kBodyReach's)
static_assert(kRecords <= 32, "a record a bit of a uint32_t");

// The batches 0 to n - 1, a bit each.
inline uint32_t Bits(int n) {
    return n >= 32 ? 0xFFFFFFFFu : n <= 0 ? 0u : (1u << n) - 1u;
}

struct Draw {
    uint32_t count = 0, start = 0;
    int32_t base = 0;
    uint32_t times = 0;
    uint8_t stencil = 0;
    char kind = 0;        // skin.cpp's word for it (fpbatch.h NoteDraw)
    bool shared = false;  // drawn after an upload whose camera another kind of batch had too: whose it is, not known
};

// A view a batch was seen in.
struct ViewSeen {
    char name[16] = {};
    uint32_t kind = 0, desc = 0;  // its kind; its ViewDesc (the first seen)
    uint32_t entries[kViewEntries] = {};
    int entryCount = 0;
    bool entriesMore = false;  // more different view entries than kViewEntries
    uint32_t subs = 0;         // its sub-views
};

struct Record {
    Call call;  // its latest
    float cams[kCameras][16] = {};  // the cameras its per-view blocks were made from (BlockKeeps), the last kCameras
    int camNext = 0, camCount = 0;
    uint32_t relCalls = 0, absCalls = 0;
    // The view's own first-person camera just before the transform less the origin, in the same view and sub-view: their difference is the
    // origin, if all is read right. Pairs seen, the farthest off (metres).
    float absPos[3] = {};
    uint32_t absEntry = 0, absSub = 0;
    bool absLast = false;
    uint32_t pairs = 0;
    float pairOff = 0.0f;
    ViewSeen views[kViews];
    int viewCount = 0;
    uint32_t viewsMore = 0;  // calls in views not kept (kViews different ones already)
    Draw draws[kDraws];
    int drawCount = 0;
    uint32_t drawsMore = 0;      // times of draws not kept (kDraws different ones already)
    uint32_t uploads = 0;        // per-view uploads that were its camera's, no other kind of batch's
    uint32_t sharedUploads = 0;  // per-view uploads that were its camera's and another kind of batch's too
};

struct Second {
    uint32_t generation = 1;  // tags of another second's books are never applied
    Record records[kRecords];
    int count = 0;
    uint32_t calls = 0, relCalls = 0, absCalls = 0, notKept = 0;
    uint32_t faults[kFaults] = {};
    uint32_t uploads = 0, notViews = 0, matched = 0, ambiguous = 0, rowsOff = 0, missArms = 0, missFar = 0;
    float missEye[3] = {};
    bool haveMiss = false;
    uint32_t draws = 0, drawsShared = 0, drawsNotBound = 0;
    // 80-byte view-projections in the world on a thread whose draws a batch tagged (fpbatch.h NoteWorldView): before its first draw (the
    // tag kept), after (the tag ended); draws after one ended it, still with the batch's per-view block bound (counted, nobody's).
    uint32_t worldBefore = 0, worldAfter = 0, drawsAfterWorld = 0;
};
// (Emptied with memset: every number 0 but the generation, and no big copy on the stack of the game's threads.)
static_assert(std::is_trivially_copyable<Second>::value && std::is_trivially_copyable<Record>::value, "the books emptied by memset");

// The books emptied for the next second (a new generation).
inline void Reset(Second& s) {
    const uint32_t next = s.generation + 1 ? s.generation + 1 : 1;
    std::memset(&s, 0, sizeof(s));
    s.generation = next;
}

inline void NoteFault(Second& s, Fault f) {
    ++s.calls;
    if (f > kTaken && f < kFaults)
        ++s.faults[f];
}

// Two views alike: by name and kind; with no name, by ViewDesc and kind.
inline bool SameView(const ViewSeen& v, const char* name, uint32_t kind, uint32_t desc) {
    if (v.kind != kind)
        return false;
    if (v.name[0] || name[0])
        return std::strncmp(v.name, name, sizeof(v.name)) == 0;
    return v.desc == desc;
}

// A view entry noted as one of the view's (different ones up to kViewEntries; more, marked).
inline void NoteEntry(ViewSeen& v, uint32_t entry) {
    for (int e = 0; e < v.entryCount; ++e)
        if (v.entries[e] == entry)
            return;
    if (v.entryCount < kViewEntries)
        v.entries[v.entryCount++] = entry;
    else
        v.entriesMore = true;
}

// A call taken (ReadCall's): its batch found or added (kRecords at most a second; past that only counted).
inline Record* NoteCall(Second& s, const Call& c) {
    ++s.calls;
    ++(c.abs ? s.absCalls : s.relCalls);
    Record* r = nullptr;
    for (int i = 0; i < s.count && !r; ++i)
        if (s.records[i].call.batch == c.batch)
            r = &s.records[i];
    if (!r) {
        if (s.count >= kRecords) {
            ++s.notKept;
            return nullptr;
        }
        r = &s.records[s.count++];
        std::memset(r, 0, sizeof(*r));
    }
    r->call = c;
    ++(c.abs ? r->absCalls : r->relCalls);
    if (BlockKeeps(c)) {
        std::memcpy(r->cams[r->camNext], c.matrix, sizeof(c.matrix));
        r->camNext = (r->camNext + 1) % kCameras;
        if (r->camCount < kCameras)
            ++r->camCount;
    }
    ViewSeen* seen = nullptr;
    for (int v = 0; v < r->viewCount && !seen; ++v)
        if (SameView(r->views[v], c.viewName, c.viewKind, c.view))
            seen = &r->views[v];
    if (!seen && r->viewCount < kViews) {
        seen = &r->views[r->viewCount++];
        std::memcpy(seen->name, c.viewName, sizeof(seen->name));
        seen->name[sizeof(seen->name) - 1] = 0;
        seen->kind = c.viewKind;
        seen->desc = c.view;
    }
    if (seen) {
        NoteEntry(*seen, c.viewEntry);
        if (c.subLast < 64 && c.subLast + 1 > seen->subs)
            seen->subs = c.subLast + 1;
    } else {
        ++r->viewsMore;
    }
    if (c.abs) {
        std::memcpy(r->absPos, c.matrix + 12, sizeof(r->absPos));
        r->absEntry = c.viewEntry;
        r->absSub = c.sub;
        r->absLast = true;
    } else {
        if (r->absLast && r->absEntry == c.viewEntry && r->absSub == c.sub) {
            float off = 0.0f;
            for (int i = 0; i < 3; ++i) {
                const float d = std::fabs(r->absPos[i] - Float(c.head, kOrigin + 4 * i) - c.matrix[12 + i]);
                off = d > off || !(d == d) ? d : off;  // (NaN kept as the most off)
            }
            ++r->pairs;
            if (off > r->pairOff || !(off == off))
                r->pairOff = off;
        }
        r->absLast = false;
    }
    return r;
}

// Two batches of one kind: the same flags and the same meshes in both lists (a batch made again each frame shows as many pointers of one
// kind). Which of the two calls each was last noted with doesn't count: a flag-1 batch's view's own camera and its transform less the
// origin come one after the other.
inline bool SameMeshes(const Record& a, const Record& b) {
    if (a.call.head[kFlags] != b.call.head[kFlags])
        return false;
    for (int l = 0; l < 2; ++l) {
        if (a.call.entriesRead[l] != b.call.entriesRead[l] ||
            Word(a.call.head, l ? kSecondCount : kMeshCount) != Word(b.call.head, l ? kSecondCount : kMeshCount))
            return false;
        for (uint32_t e = 0; e < a.call.entriesRead[l]; ++e)
            if (a.call.entries[l][e][0] != b.call.entries[l][e][0])
                return false;
    }
    return true;
}

// A per-view upload read as a view (viewturn::ReadPerView: its camera's rows right, up, back and its eye): the batches whose camera it is
// (one of each batch's last kCameras, the eye within kEyeNear), a bit each (records[i]: 1 << i), 0 none. Batches of one kind (SameMeshes)
// are one: the first of them stands for it. One bit: that batch's upload, its draws its own. More: batches of different kinds had that
// camera (the same origin, or flag 4 in one view), and which of them the upload was isn't known: counted as shared for each, and the draws
// after it marked shared, never given to the first.
inline uint32_t MatchView(Second& s, const float* rows, const double* eye) {
    ++s.uploads;
    uint32_t found = 0;
    int first = -1, kinds = 0;
    bool rowsAlike = false;
    for (int i = 0; i < s.count; ++i) {
        const Record& r = s.records[i];
        int k = 0;
        for (; k < r.camCount; ++k) {
            const float* m = r.cams[k];
            if (std::fabs(m[12] - eye[0]) < kEyeNear && std::fabs(m[13] - eye[1]) < kEyeNear && std::fabs(m[14] - eye[2]) < kEyeNear)
                break;
        }
        if (k == r.camCount)
            continue;
        bool kindFound = false;
        for (int j = 0; j < i && !kindFound; ++j)
            kindFound = (found >> j & 1u) && SameMeshes(s.records[j], r);
        if (kindFound)
            continue;
        found |= 1u << i;
        ++kinds;
        if (first < 0) {
            first = i;
            const float* m = r.cams[k];
            bool alike = true;
            for (int row = 0; row < 3 && alike; ++row)
                for (int col = 0; col < 3 && alike; ++col)
                    alike = std::fabs(m[4 * row + col] - rows[3 * row + col]) < kRowsNear;
            rowsAlike = alike;
        }
    }
    if (!found) {
        const bool arms = std::fabs(eye[0]) < kArmsReach && std::fabs(eye[1]) < kArmsReach && std::fabs(eye[2]) < kArmsReach;
        ++(arms ? s.missArms : s.missFar);
        if (arms && !s.haveMiss) {
            for (int i = 0; i < 3; ++i)
                s.missEye[i] = static_cast<float>(eye[i]);
            s.haveMiss = true;
        }
        return 0;
    }
    ++s.matched;
    if (!rowsAlike)
        ++s.rowsOff;
    if (kinds > 1) {
        ++s.ambiguous;
        for (int i = 0; i < s.count; ++i)
            if (found >> i & 1u)
                ++s.records[i].sharedUploads;
    } else {
        ++s.records[first].uploads;
    }
    return found;
}

// A draw made with a per-view block bound (fpbatch.cpp checks that), after an upload MatchView found these batches' camera in (its bits):
// kept for its one batch as its own, or for each of them as shared; kDraws different ones a batch at most.
inline void NoteDraw(Second& s, uint32_t batches, uint32_t count, uint32_t start, int32_t base, uint8_t stencil, char kind) {
    const uint32_t kept = batches & Bits(s.count);
    if (!kept)
        return;
    const bool shared = (kept & (kept - 1u)) != 0;
    ++s.draws;
    if (shared)
        ++s.drawsShared;
    for (int i = 0; i < s.count; ++i) {
        if (!(kept >> i & 1u))
            continue;
        Record& r = s.records[i];
        int d = 0;
        while (d < r.drawCount && !(r.draws[d].count == count && r.draws[d].start == start && r.draws[d].base == base &&
                                    r.draws[d].stencil == stencil && r.draws[d].shared == shared))
            ++d;
        if (d < r.drawCount) {
            ++r.draws[d].times;
            continue;
        }
        if (r.drawCount >= kDraws) {
            ++r.drawsMore;
            continue;
        }
        Draw& n = r.draws[r.drawCount++];
        n.count = count, n.start = start, n.base = base, n.times = 1, n.stencil = stencil, n.kind = kind, n.shared = shared;
    }
}

// ---- A thread's draws ----
// fpbatch.cpp keeps one of these for each thread (its draws go on the thread its uploads did) and goes by these rules alone, under the lock.

// The batches whose camera the thread's last per-view upload was (MatchView's bits) in the books of that generation, the buffer it went into
// (only compared), and the draws counted with it since. ended: an 80-byte view-projection in the world came after its draws, and the draws
// still finding the buffer bound are only counted, nobody's.
struct Tag {
    uint32_t generation = 0;  // 0: none
    uint32_t batches = 0;
    const void* buffer = nullptr;
    uint32_t draws = 0;
    bool ended = false;
};

// A 352-byte per-view upload (its rows and eye as viewturn::ReadPerView reads them; nullptr if it doesn't read as a view): the tag it leaves
// on its thread, none if it's no batch's camera. Whatever tag the thread had ends.
inline Tag TagView(Second& s, const void* buffer, const float* rows, const double* eye) {
    Tag t;
    if (!rows || !eye) {
        ++s.uploads;
        ++s.notViews;
    } else if (const uint32_t batches = MatchView(s, rows, eye)) {
        t.generation = s.generation;
        t.batches = batches;
        t.buffer = buffer;
    }
    return t;
}

// An 80-byte upload that reads as a view-projection in the world (viewturn::ReadViewProjection) on a tagged thread. After the batch's draws
// it ends the tag. Before its first draw it may be the batch's own (viewturn.h: the body's views come with one each; the order the game
// uploads them in isn't known yet), so it's counted and the tag kept. A tag of another second's books ends.
inline void TagWorldView(Second& s, Tag& t) {
    if (!t.generation || t.ended)
        return;
    if (t.generation != s.generation) {
        t = Tag{};
        return;
    }
    if (t.draws) {
        ++s.worldAfter;
        t.ended = true;
    } else {
        ++s.worldBefore;
    }
}

// A draw on a tagged thread; bound: whether the tag's buffer is bound to a vertex-shader slot (fpbatch.cpp asks the context). Bound, it's
// kept for its batches (NoteDraw), or after an 80-byte view-projection ended the tag only counted. Not bound, the tag ends: nothing after it
// is the batch's (a binding left over from before is no longer taken for one). A tag of another second's books ends.
inline void TagDraw(Second& s, Tag& t, bool bound, uint32_t count, uint32_t start, int32_t base, uint8_t stencil, char kind) {
    if (!t.generation)
        return;
    if (t.generation != s.generation) {
        t = Tag{};
        return;
    }
    if (!bound) {
        if (!t.ended)
            ++s.drawsNotBound;
        t = Tag{};
        return;
    }
    if (t.ended) {
        ++s.drawsAfterWorld;
        return;
    }
    NoteDraw(s, t.batches, count, start, base, stencil, kind);
    ++t.draws;
}

// ---- The log ----

// Appends to out (size bytes, kept ended) as printf does.
inline void Append(char* out, size_t size, const char* fmt, ...) {
    const size_t used = std::strlen(out);
    if (used + 1 >= size)
        return;
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(out + used, size - used, fmt, args);
    va_end(args);
}

// The second's first line.
inline void Summary(const Second& s, char* out, size_t size) {
    out[0] = 0;
    uint32_t faults = 0;
    for (int f = kTaken + 1; f < kFaults; ++f)
        faults += s.faults[f];
    Append(out, size,
           "FP batches: last second %u calls of the camera set from the batch camera (%u of the transform made relative to the batch's origin, %u of "
           "the view's own first-person camera, %u not in the frame as read",
           s.calls, s.relCalls, s.absCalls, faults);
    for (int f = kTaken + 1; f < kFaults; ++f)
        if (s.faults[f])
            Append(out, size, "; %s %u", FaultName(f), s.faults[f]);
    Append(out, size, "), %d batch%s kept", s.count, s.count == 1 ? "" : "es");
    if (s.notKept)
        Append(out, size, " (%u calls of more not)", s.notKept);
    Append(out, size,
           "; per-view uploads %u: %u a batch's camera (%u of them more than one kind of batch's, their draws shared: whose, not known; %u with other "
           "rows), %u no batch's in the arms' space",
           s.uploads, s.matched, s.ambiguous, s.rowsOff, s.missArms);
    if (s.haveMiss)
        Append(out, size, " (one with its eye at %.4f %.4f %.4f)", s.missEye[0], s.missEye[1], s.missEye[2]);
    Append(out, size, ", %u farther, %u not read as views; %u draws made with a batch's per-view block (%u of them shared), %u ending a tag with it not bound",
           s.missFar, s.notViews, s.draws, s.drawsShared, s.drawsNotBound);
    Append(out, size,
           "; 80-byte view-projections in the world after a batch's upload: %u before its first draw (the tag kept), %u after its draws (the tag ended: %u "
           "draws after it still with the block bound, not counted)",
           s.worldBefore, s.worldAfter, s.drawsAfterWorld);
}

// A line for the batches alike to records[first] (SameMeshes): the first's numbers, everything counted over all of them. name(mesh, text,
// size) puts what is known of a mesh's name into text ("" none).
template <class Name>
void Line(const Second& s, int first, const bool* inGroup, Name&& name, char* out, size_t size) {
    out[0] = 0;
    const Record& r = s.records[first];
    const Call& c = r.call;
    int pointers = 0;
    uint32_t rel = 0, abs = 0, uploads = 0, sharedUploads = 0, pairs = 0, viewsMore = 0;
    float pairOff = 0.0f;
    // Its views over all of the group's batches, by name and kind.
    ViewSeen views[kViews];
    int viewCount = 0;
    for (int i = 0; i < s.count; ++i) {
        if (!inGroup[i])
            continue;
        const Record& g = s.records[i];
        ++pointers;
        rel += g.relCalls, abs += g.absCalls, uploads += g.uploads, sharedUploads += g.sharedUploads, pairs += g.pairs, viewsMore += g.viewsMore;
        if (g.pairOff > pairOff || !(g.pairOff == g.pairOff))
            pairOff = g.pairOff;
        for (int v = 0; v < g.viewCount; ++v) {
            const ViewSeen& from = g.views[v];
            int m = 0;
            while (m < viewCount && !SameView(views[m], from.name, from.kind, from.desc))
                ++m;
            if (m == viewCount) {
                if (viewCount < kViews)
                    views[viewCount++] = from;
                else
                    ++viewsMore;
                continue;
            }
            ViewSeen& to = views[m];
            for (int e = 0; e < from.entryCount; ++e)
                NoteEntry(to, from.entries[e]);
            to.entriesMore = to.entriesMore || from.entriesMore;
            if (from.subs > to.subs)
                to.subs = from.subs;
        }
    }
    Append(out, size, "FP batch %08X", c.batch);
    if (pointers > 1)
        Append(out, size, " (and %d more batch pointers with the same flags and meshes this second)", pointers - 1);
    Append(out, size, ": flags 0x%02X, FOV %.2f; in view%s", c.head[kFlags], Float(c.head, kFov), viewCount == 1 ? "" : "s");
    for (int v = 0; v < viewCount; ++v) {
        const ViewSeen& w = views[v];
        Append(out, size, "%s ", v ? "," : "");
        if (w.name[0])
            Append(out, size, "%s (kind %u", w.name, w.kind);
        else
            Append(out, size, "? (ViewDesc %08X, kind %u", w.desc, w.kind);
        Append(out, size, ", %u sub-view%s, ", w.subs, w.subs == 1 ? "" : "s");
        if (w.entriesMore)
            Append(out, size, "more than %d view entries)", kViewEntries);
        else
            Append(out, size, "%d view entr%s)", w.entryCount, w.entryCount == 1 ? "y" : "ies");
    }
    if (!viewCount)
        Append(out, size, " none kept");
    if (viewsMore)
        Append(out, size, " (calls in more views not kept: %u)", viewsMore);
    Append(out, size, "; origin (%.3f %.3f %.3f), its transform's last row (%.3f %.3f %.3f)", Float(c.head, kOrigin), Float(c.head, kOrigin + 4),
           Float(c.head, kOrigin + 8), Float(c.head, kTransform + 48), Float(c.head, kTransform + 52), Float(c.head, kTransform + 56));
    Append(out, size, "; its camera %s at (%.4f %.4f %.4f), forward (%.3f %.3f %.3f)", c.abs ? "(the view's own)" : "less the origin", c.matrix[12],
           c.matrix[13], c.matrix[14], -c.matrix[8], -c.matrix[9], -c.matrix[10]);
    Append(out, size, "; sets: %u less the origin, %u the view's own", rel, abs);
    if (pairs)
        Append(out, size, " (%u in pairs: the view's own less the origin at most %.4f m off the other)", pairs, pairOff);
    for (int l = 0; l < 2; ++l) {
        const uint32_t count = Word(c.head, l ? kSecondCount : kMeshCount);
        Append(out, size, "; %s (+0x%02X) %u", l ? "second list" : "meshes", l ? kSecond : kMeshes, count);
        for (uint32_t e = 0; e < c.entriesRead[l]; ++e) {
            char text[96] = "";
            name(c.entries[l][e][0], text, sizeof(text));
            Append(out, size, "%s %08X/%X%s%s", e ? "," : ":", c.entries[l][e][0], c.entries[l][e][1], text[0] ? " " : "", text);
        }
        if (count > c.entriesRead[l])
            Append(out, size, "%s", c.entriesRead[l] ? ", ..." : " (not read)");
    }
    Append(out, size, "; per-view uploads its camera's %u", uploads);
    if (sharedUploads)
        Append(out, size, " (and %u whose camera another kind of batch had too)", sharedUploads);
    // Every draw of the group's batches, merged by kind: its own, then those shared with another kind of batch.
    Draw merged[kDraws * 4];
    int mergedCount = 0;
    uint32_t more = 0;
    for (int i = 0; i < s.count; ++i) {
        if (!inGroup[i])
            continue;
        more += s.records[i].drawsMore;
        for (int k = 0; k < s.records[i].drawCount; ++k) {
            const Draw& d = s.records[i].draws[k];
            int m = 0;
            while (m < mergedCount && !(merged[m].count == d.count && merged[m].start == d.start && merged[m].base == d.base &&
                                        merged[m].stencil == d.stencil && merged[m].shared == d.shared))
                ++m;
            if (m < mergedCount)
                merged[m].times += d.times;
            else if (mergedCount < kDraws * 4)
                merged[mergedCount++] = d;
            else
                more += d.times;
        }
    }
    for (int part = 0; part < 2; ++part) {
        const bool shared = part == 1;
        int shown = 0;
        for (int m = 0; m < mergedCount; ++m) {
            const Draw& d = merged[m];
            if (d.shared != shared)
                continue;
            if (!shown)
                Append(out, size, "%s:", shared ? "; draws after an upload whose camera another kind of batch had too (whose, not known)" : "; draws");
            Append(out, size, "%s %u/%u", shown ? "," : "", d.count, d.start);
            if (d.base)
                Append(out, size, "/%d", d.base);
            if (d.kind)
                Append(out, size, " %c", d.kind);
            Append(out, size, " x%u s%u", d.times, d.stencil);
            ++shown;
        }
        if (!shown && !shared)
            Append(out, size, "; draws: none of its own seen");
    }
    if (more)
        Append(out, size, " (%u more not kept)", more);
}

}  // namespace fpbatch

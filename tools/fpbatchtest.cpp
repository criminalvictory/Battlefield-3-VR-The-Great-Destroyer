// The first-person batches probe (weapon.txt "fpbatchlog 1"; src/fpbatchrules.h and the hook's stub src/fpbatchstub.h, header only, as
// fpbatch.cpp runs them), without the game.
//   1  the game's code: every place the probe goes by parses; against build\bf3_mem.exe (beside this program, SKIP without it) each is as
//      read; the calls made from them: 0x0101841D and 0x010183B5 call the setter and return to kRelReturn and kAbsReturn, 0x0101AD74 calls
//      the batch camera and returns to kBatchCameraReturn; the setter's one way out is ret 4 (one argument on the stack, as the stub takes it)
//   2  a call read from a made-up stack, frame, view entry, ViewDesc and batch: the transform less the origin and the view's own camera,
//      everything as put there; each way of not being the batch camera's frame refused alone (its return, EBX, ECX, the matrix, memory not
//      there); a list longer than kept; a view with no name
//   3  the books: the origin's pair check (the view's own camera less the origin on the other: 0 off; an origin read wrong: off by as much),
//      views and sub-views, kRecords batches and the calls of one more counted, not kept; a fault counted as a call; views told apart by name
//      and kind, not by their entry (a list made each frame), one with no name by its ViewDesc
//   4  per-view uploads: a batch's camera exactly, 0.5 mm off, 5 mm off; its rows off; two kinds of batch with one camera (shared, never the
//      first's), a third of the first's kind (one with it), one kind's two pointers alone (its own); a camera of frames before, and one of
//      more than kCameras sets before not; an eye in the arms' space or farther; the cameras a batch's per-view block is made from (the view's
//      own only with flag 4 or an origin within 3 cm): a flag-4 batch's upload after a far-origin flag-1 batch's calls is its alone
//   5  draws: kept by kind with their times, kDraws different ones, then the rest counted; a draw after a shared upload kept for each, shared
//   6  the log: two batches alike in one line (their views merged), another kind in its own; the line's numbers and names, its own draws and
//      the shared ones apart; the summary's numbers
//   7  names and pages: LooksLikeName; the pages read and never read (write-combined, uncached, guard, no access)
//   8  the stub (32-bit): FpSetterHook on a stand-in of the setter: the batch camera's two calls noted with ECX, the matrix, EBX, EBP and the
//      stack as the setter was entered, the stand-in run after with ECX, its argument, EAX and EDX as they came; any other call straight on,
//      nothing noted; EBX, ESI, EDI and EBP as they were, the stack as it was after ret 4
//   9  the time: a call taken into the books and an upload matched against full books, each well under the frame's budget; a call passed by
//  10  a thread's tag (fpbatch.cpp goes by these rules alone): its batch's draws while its buffer is bound, ended by the first draw without it
//      (a binding left over after is nobody's); an 80-byte view-projection before the first draw counted and the tag kept, one after the
//      draws ending it (draws still bound after it only counted); a per-view upload of no batch's, or no view, ending it; a tag of the last
//      second's books nobody's, ended; a shared upload's draws shared
// Usage: fpbatchtest
// The exit code is the number of failed checks.

#include <windows.h>

#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "../src/fpbatchrules.h"
#include "../src/fpbatchstub.h"

namespace {

int g_failed = 0, g_counted = 0, g_skipped = 0;

void Report(const char* id, const char* what, bool pass, const char* format, ...) {
    char numbers[2048];
    va_list args;
    va_start(args, format);
    std::vsnprintf(numbers, sizeof(numbers), format, args);
    va_end(args);
    std::printf("%-4s %-5s %s | %s\n", id, pass ? "PASS" : "FAIL", what, numbers);
    ++g_counted;
    g_failed += !pass;
}

void Skip(const char* id, const char* what) {
    std::printf("%-4s SKIP  %s\n", id, what);
    ++g_skipped;
}

// ------------------------------------------------------------------ a memory image ------------------------------------------------------------------

struct Image {
    std::map<uint32_t, std::vector<uint8_t>> pages;
    uint8_t* Page(uint32_t at) {
        std::vector<uint8_t>& p = pages[at >> 12];
        if (p.empty())
            p.assign(4096, 0);
        return p.data();
    }
    void Put(uint32_t at, const void* data, size_t n) {
        const uint8_t* b = static_cast<const uint8_t*>(data);
        for (size_t i = 0; i < n; ++i)
            Page(at + static_cast<uint32_t>(i))[(at + i) & 4095] = b[i];
    }
    void W32(uint32_t at, uint32_t v) { Put(at, &v, 4); }
    void F32(uint32_t at, float v) { Put(at, &v, 4); }
    void Drop(uint32_t at) { pages.erase(at >> 12); }
    bool operator()(uint32_t at, void* out, size_t n) const {
        uint8_t* o = static_cast<uint8_t*>(out);
        while (n) {
            const auto it = pages.find(at >> 12);
            if (it == pages.end())
                return false;
            const size_t off = at & 4095, take = n < 4096 - off ? n : 4096 - off;
            std::memcpy(o, it->second.data() + off, take);
            o += take;
            at += static_cast<uint32_t>(take);
            n -= take;
        }
        return true;
    }
};

std::wstring OwnFolder() {
    wchar_t self[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    std::wstring s = self;
    return s.substr(0, s.find_last_of(L"\\/") + 1);
}

bool LoadFile(const std::wstring& path, std::vector<uint8_t>& out) {
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"rb") != 0 || !f)
        return false;
    std::fseek(f, 0, SEEK_END);
    const long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    out.resize(size > 0 ? static_cast<size_t>(size) : 0);
    const size_t got = out.empty() ? 0 : std::fread(out.data(), 1, out.size(), f);
    std::fclose(f);
    return got == out.size() && !out.empty();
}

// ------------------------------------------------------------------ 1 the game's code ------------------------------------------------------------------

constexpr uint32_t kBase = 0x400000;  // bf3_mem.exe: the decrypted image saved as it lies in memory (a VA less this is its offset)

void Code() {
    int parsed = 0;
    char bad[256] = "";
    for (int i = 0; i < fpbatch::kCodeCount; ++i) {
        uint8_t b[fpbatch::kCodeMost];
        if (fpbatch::ParseHex(fpbatch::kCode[i].bytes, b, sizeof(b)))
            ++parsed;
        else
            std::snprintf(bad + std::strlen(bad), sizeof(bad) - std::strlen(bad), " %08X", fpbatch::kCode[i].at);
    }
    uint8_t probe[4];
    const bool refuses = !fpbatch::ParseHex("55 8", probe, sizeof(probe)) && !fpbatch::ParseHex("55 8G", probe, sizeof(probe)) &&
                         !fpbatch::ParseHex("55 8B EC 83 E4", probe, sizeof(probe)) && fpbatch::ParseHex("55 8b", probe, sizeof(probe)) == 2;
    Report("1a", "every place the probe goes by parses (and a list that isn't one is refused)", parsed == fpbatch::kCodeCount && refuses, "%d of %d%s%s",
           parsed, fpbatch::kCodeCount, bad, refuses ? "" : "; a bad list taken");

    std::vector<uint8_t> image;
    if (!LoadFile(OwnFolder() + L"bf3_mem.exe", image) || image.size() < 0x2000000) {
        Skip("1b", "no build\\bf3_mem.exe beside the test: the places against the real code not checked");
        return;
    }
    const auto read = [&](uint32_t at, void* out, size_t n) {
        if (at < kBase || at - kBase + n > image.size())
            return false;
        std::memcpy(out, image.data() + (at - kBase), n);
        return true;
    };
    int failed = -1;
    const bool all = fpbatch::CodeAsRead(read, &failed);
    Report("1b", "against the real code (build\\bf3_mem.exe, bf3.exe 1.6.0.0 decrypted): every place as read", all, "%s%s",
           all ? "all as read" : "not as read at ", all ? "" : fpbatch::kCode[failed].what);
    // A call's target and where it returns to, from its E8 and rel32.
    const auto call = [&](uint32_t at, uint32_t& target) {
        uint8_t b[5] = {};
        if (!read(at, b, 5) || b[0] != 0xE8)
            return false;
        int32_t rel = 0;
        std::memcpy(&rel, b + 1, 4);
        target = at + 5 + static_cast<uint32_t>(rel);
        return true;
    };
    uint32_t relTarget = 0, absTarget = 0, cameraTarget = 0;
    const bool calls = call(0x0101841D, relTarget) && call(0x010183B5, absTarget) && call(0x0101AD74, cameraTarget);
    Report("1c", "the calls made from them: the setter from 0x0101841D and 0x010183B5, returning to kRelReturn and kAbsReturn; the batch camera from "
                 "0x0101AD74, returning to kBatchCameraReturn",
           calls && relTarget == fpbatch::kSetter && absTarget == fpbatch::kSetter && cameraTarget == fpbatch::kBatchCamera &&
               0x0101841D + 5 == fpbatch::kRelReturn && 0x010183B5 + 5 == fpbatch::kAbsReturn && 0x0101AD74 + 5 == fpbatch::kBatchCameraReturn,
           "targets %08X %08X %08X", relTarget, absTarget, cameraTarget);
    // The setter's code from its start to its first int3 padding: one ret, a ret 4 (C2 04 00), at its end.
    uint8_t setter[0x70] = {};
    read(fpbatch::kSetter, setter, sizeof(setter));
    int rets = 0, at = -1;
    for (int i = 0; i + 2 < static_cast<int>(sizeof(setter)) && setter[i] != 0xCC; ++i)
        if (setter[i] == 0xC2 && setter[i + 1] == 0x04 && setter[i + 2] == 0x00)
            ++rets, at = i;
    Report("1d", "the setter's one way out is ret 4 (one argument on the stack: the stub's), at its end", rets == 1 && at == 0x67 && setter[0x6A] == 0xCC,
           "%d ret 4 at +0x%X", rets, at);
}

// ------------------------------------------------------------------ 2 a call read ------------------------------------------------------------------

// A made-up call of the setter from the batch camera: the stack as entered, the batch camera's frame, its view's entry, the ViewDesc, the batch,
// its meshes.
constexpr uint32_t kEntry = 0x001FF000, kFrame = 0x00200000, kBatch = 0x00300000, kViewEntryAt = 0x00320000, kViewAt = 0x00500000,
                   kNameAt = 0x00600000, kMeshList = 0x00700000;
const float kOrigin[3] = {1200.5f, 85.25f, -431.75f};

void MatrixAt(float* m, float x, float y, float z) {
    const float rows[16] = {0.8f, 0.0f, -0.6f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.6f, 0.0f, 0.8f, 0.0f, x, y, z, 1.0f};
    std::memcpy(m, rows, sizeof(rows));
}

struct Made {
    Image m;
    float abs[16], rel[16];
    Made() {
        m.W32(kFrame + 4, fpbatch::kBatchCameraReturn);
        m.W32(kFrame + 8, kBatch);
        m.W32(kFrame + 0x0C, 0x00310000);
        m.W32(kFrame + 0x10, kViewEntryAt);
        m.W32(kViewEntryAt + 4, kViewAt);
        m.W32(kViewEntryAt + 8, 1);
        m.W32(kViewAt + 4, kNameAt);
        m.Put(kNameAt, "SecondaryRoot\0Root", 19);
        MatrixAt(abs, kOrigin[0] + 0.25f, kOrigin[1] + 1.6f, kOrigin[2] - 0.1f);
        m.Put(kViewAt + fpbatch::kViewFirstPerson, abs, sizeof(abs));
        std::memcpy(rel, abs, sizeof(rel));
        for (int i = 0; i < 3; ++i)
            rel[12 + i] = abs[12 + i] - kOrigin[i];
        m.Put(kEntry + fpbatch::kEntryToRelMatrix, rel, sizeof(rel));
        m.W32(kEntry, fpbatch::kRelReturn);
        m.W32(kEntry + fpbatch::kEntryToSub, 1);
        m.W32(kEntry + fpbatch::kEntryToSubLast, 1);
        for (int i = 0; i < 16; ++i)
            m.F32(kBatch + fpbatch::kTransform + 4 * i, i % 5 == 0 ? 1.0f : 0.0f);
        for (int i = 0; i < 3; ++i)
            m.F32(kBatch + fpbatch::kOrigin + 4 * i, kOrigin[i]);
        m.W32(kBatch + fpbatch::kMeshes, kMeshList);
        m.W32(kBatch + fpbatch::kMeshCount, 8);
        m.W32(kBatch + fpbatch::kSecond, 0);
        m.W32(kBatch + fpbatch::kSecondCount, 0);
        m.F32(kBatch + fpbatch::kFov, 55.0f);
        m.W32(kBatch + fpbatch::kFlags, 0x05);
        for (uint32_t e = 0; e < 8; ++e) {
            m.W32(kMeshList + 8 * e, 0x00800000 + 0x100 * e);
            m.W32(kMeshList + 8 * e + 4, e);
        }
    }
    fpbatch::Fault Rel(fpbatch::Call& c, uint32_t ebx = kBatch, uint32_t camera = kEntry + fpbatch::kEntryToCamera) const {
        return fpbatch::ReadCall(m, fpbatch::kRelReturn, camera, kEntry + fpbatch::kEntryToRelMatrix, ebx, kFrame, kEntry, c);
    }
    fpbatch::Fault Abs(fpbatch::Call& c) const {
        return fpbatch::ReadCall(m, fpbatch::kAbsReturn, kEntry + fpbatch::kEntryToCamera, kViewAt + fpbatch::kViewFirstPerson, kBatch, kFrame, kEntry, c);
    }
};

bool Same16(const float* a, const float* b) {
    return std::memcmp(a, b, 64) == 0;
}

void Calls() {
    {
        Made made;
        fpbatch::Call c;
        const fpbatch::Fault f = made.Rel(c);
        bool entries = c.entriesRead[0] == fpbatch::kEntries && c.entriesRead[1] == 0;
        for (int e = 0; e < fpbatch::kEntries; ++e)
            entries = entries && c.entries[0][e][0] == 0x00800000u + 0x100u * e && c.entries[0][e][1] == static_cast<uint32_t>(e);
        Report("2a", "the transform less the origin: read whole (the batch, its flags, FOV and origin, the matrix, the view's entry, kind and name, the "
                     "sub-view, the meshes' first kEntries of 8)",
               f == fpbatch::kTaken && c.batch == kBatch && !c.abs && Same16(c.matrix, made.rel) && c.head[fpbatch::kFlags] == 0x05 &&
                   fpbatch::Float(c.head, fpbatch::kFov) == 55.0f && fpbatch::Float(c.head, fpbatch::kOrigin + 4) == kOrigin[1] && c.viewEntry == kViewEntryAt &&
                   c.view == kViewAt && c.viewKind == 1 && std::strcmp(c.viewName, "SecondaryRoot") == 0 && c.sub == 1 && c.subLast == 1 && entries &&
                   fpbatch::Word(c.head, fpbatch::kMeshCount) == 8,
               "%s; batch %08X, view %08X '%s' kind %u, sub %u of %u, meshes read %u", fpbatch::FaultName(f), c.batch, c.view, c.viewName, c.viewKind, c.sub,
               c.subLast + 1, c.entriesRead[0]);
    }
    {
        Made made;
        fpbatch::Call c;
        const fpbatch::Fault f = made.Abs(c);
        Report("2b", "the view's own first-person camera (ViewDesc +0x1080): read whole", f == fpbatch::kTaken && c.abs && Same16(c.matrix, made.abs),
               "%s, at (%.3f %.3f %.3f)", fpbatch::FaultName(f), c.matrix[12], c.matrix[13], c.matrix[14]);
    }
    {
        // Each way of not being the batch camera's frame, alone.
        Made a, b, d;
        fpbatch::Call c;
        a.m.W32(kFrame + 4, fpbatch::kBatchCameraReturn + 1);
        const fpbatch::Fault caller = a.Rel(c);
        const fpbatch::Fault batch = b.Rel(c, kBatch + 0x40);
        const fpbatch::Fault zero = b.Rel(c, 0);
        const fpbatch::Fault camera = b.Rel(c, kBatch, kEntry + fpbatch::kEntryToCamera + 4);
        const fpbatch::Fault matrix = fpbatch::ReadCall(b.m, fpbatch::kRelReturn, kEntry + fpbatch::kEntryToCamera, kEntry + 0x20, kBatch, kFrame, kEntry, c);
        const fpbatch::Fault absMatrix =
            fpbatch::ReadCall(b.m, fpbatch::kAbsReturn, kEntry + fpbatch::kEntryToCamera, kEntry + fpbatch::kEntryToRelMatrix, kBatch, kFrame, kEntry, c);
        d.m.Drop(kFrame);
        const fpbatch::Fault gone = d.Rel(c);
        Made e;
        e.m.Drop(kBatch);
        const fpbatch::Fault batchGone = e.Rel(c);
        Report("2c", "each way of not being the batch camera's frame refused alone: its return, EBX (another, 0), ECX, the matrix (either call), memory "
                     "not there",
               caller == fpbatch::kNotItsCaller && batch == fpbatch::kNotItsBatch && zero == fpbatch::kNotItsBatch && camera == fpbatch::kNotItsCamera &&
                   matrix == fpbatch::kNotItsMatrix && absMatrix == fpbatch::kNotItsMatrix && gone == fpbatch::kNotRead && batchGone == fpbatch::kNotRead,
               "%s, %s, %s, %s, %s, %s, %s, %s", fpbatch::FaultName(caller), fpbatch::FaultName(batch), fpbatch::FaultName(zero), fpbatch::FaultName(camera),
               fpbatch::FaultName(matrix), fpbatch::FaultName(absMatrix), fpbatch::FaultName(gone), fpbatch::FaultName(batchGone));
    }
    {
        // Lists: one not there, one of none; a view with no readable name.
        Made made;
        made.m.W32(kBatch + fpbatch::kMeshes, 0x00900000);  // nothing there
        made.m.W32(kBatch + fpbatch::kSecond, kMeshList);
        made.m.W32(kBatch + fpbatch::kSecondCount, 2);
        made.m.W32(kViewAt + 4, 0x00A00000);
        made.m.Put(0x00A00000, "\x01\x02\x03", 4);
        fpbatch::Call c;
        const fpbatch::Fault f = made.Rel(c);
        Report("2d", "a list not there (none read, its count kept), the second list's two, a view whose name isn't one", f == fpbatch::kTaken &&
                   c.entriesRead[0] == 0 && fpbatch::Word(c.head, fpbatch::kMeshCount) == 8 && c.entriesRead[1] == 2 && c.viewName[0] == 0,
               "%s; read %u and %u, name '%s'", fpbatch::FaultName(f), c.entriesRead[0], c.entriesRead[1], c.viewName);
    }
}

// ------------------------------------------------------------------ 3 the books ------------------------------------------------------------------

// A call as ReadCall takes it: by default a flag-1 batch at kOrigin (far from 0: the view's own camera, then the transform less the origin).
fpbatch::Call CallAt(uint32_t batch, bool abs, float x, float y, float z, uint32_t viewEntry = kViewEntryAt, uint32_t sub = 0, uint8_t flags = 0x01) {
    fpbatch::Call c;
    c.batch = batch;
    c.abs = abs;
    MatrixAt(c.matrix, x, y, z);
    for (int i = 0; i < 3; ++i)
        std::memcpy(c.head + fpbatch::kOrigin + 4 * i, &kOrigin[i], 4);
    c.head[fpbatch::kFlags] = flags;
    c.viewEntry = viewEntry;
    c.view = kViewAt;
    std::snprintf(c.viewName, sizeof(c.viewName), "%s", viewEntry == kViewEntryAt ? "Root" : "SecondaryRoot");
    c.sub = sub;
    c.subLast = 1;
    return c;
}

void SetOrigin(fpbatch::Call& c, float x, float y, float z) {
    const float o[3] = {x, y, z};
    std::memcpy(c.head + fpbatch::kOrigin, o, sizeof(o));
}

fpbatch::Second g_s, g_t;  // (big: kept off the stack)

void Books() {
    {
        fpbatch::Reset(g_s);
        const float at[3] = {kOrigin[0] + 0.3f, kOrigin[1] + 1.55f, kOrigin[2] + 0.2f};
        fpbatch::NoteCall(g_s, CallAt(kBatch, true, at[0], at[1], at[2]));
        fpbatch::NoteCall(g_s, CallAt(kBatch, false, at[0] - kOrigin[0], at[1] - kOrigin[1], at[2] - kOrigin[2]));
        const fpbatch::Record& r = g_s.records[0];
        // (Its cameras: the transform less the origin alone; the view's own, set over at once, never its per-view block's.)
        const bool right = g_s.count == 1 && r.pairs == 1 && r.pairOff < 1e-4f && r.relCalls == 1 && r.absCalls == 1 && r.camCount == 1 &&
                           std::fabs(r.cams[0][12] - 0.3f) < 1e-3f;
        // An origin read wrong by 2 cm along y: off by 2 cm.
        fpbatch::NoteCall(g_s, CallAt(kBatch, true, at[0], at[1], at[2], kViewEntryAt, 1));
        fpbatch::Call wrong = CallAt(kBatch, false, at[0] - kOrigin[0], at[1] - kOrigin[1], at[2] - kOrigin[2], kViewEntryAt, 1);
        const float shifted = kOrigin[1] + 0.02f;
        std::memcpy(wrong.head + fpbatch::kOrigin + 4, &shifted, 4);
        fpbatch::NoteCall(g_s, wrong);
        // A pair across views or sub-views is none.
        fpbatch::NoteCall(g_s, CallAt(kBatch, true, at[0], at[1], at[2], kViewEntryAt, 0));
        fpbatch::NoteCall(g_s, CallAt(kBatch, false, 0.3f, 1.55f, 0.2f, kViewEntryAt + 0x18, 0));
        Report("3a", "the pair check: the view's own camera less the origin on the other, 0 off; an origin read 2 cm wrong, 2 cm off; no pair across "
                     "views; its one camera the transform less the origin (the view's own, set over it, never its per-view block's)",
               right && r.pairs == 2 && std::fabs(r.pairOff - 0.02f) < 2e-4f, "pairs %u, at most %.4f m off (first %s; %d cameras kept)", r.pairs, r.pairOff,
               right ? "0 off" : "WRONG", r.camCount);
        Report("3b", "its views and sub-views kept: Root (2 sub-views, 1 entry), SecondaryRoot (its entry)",
               r.viewCount == 2 && std::strcmp(r.views[0].name, "Root") == 0 && r.views[0].subs == 2 && r.views[0].entryCount == 1 &&
                   std::strcmp(r.views[1].name, "SecondaryRoot") == 0 && r.views[1].entryCount == 1 && r.views[1].entries[0] == kViewEntryAt + 0x18,
               "%d views: %s (%u, %d entries), %s (%u, %d entries)", r.viewCount, r.views[0].name, r.views[0].subs, r.views[0].entryCount, r.views[1].name,
               r.views[1].subs, r.views[1].entryCount);
    }
    {
        // Views told apart by name and kind, not by their entry: Root and SecondaryRoot from a new entry each frame (the list made again) for six
        // frames, a view of another name and kind (a shadow's, say) seen once after, and one with no name, by its ViewDesc.
        fpbatch::Reset(g_s);
        for (int frame = 0; frame < 6; ++frame) {
            fpbatch::Call root = CallAt(kBatch, false, 0.3f, 1.55f, 0.2f, kViewEntryAt + 0x1000 * frame);
            std::snprintf(root.viewName, sizeof(root.viewName), "Root");
            fpbatch::NoteCall(g_s, root);
            fpbatch::Call second = CallAt(kBatch, false, 0.3f, 1.55f, 0.2f, kViewEntryAt + 0x1000 * frame + 0x18);
            fpbatch::NoteCall(g_s, second);
        }
        fpbatch::Call shadow = CallAt(kBatch, false, 0.3f, 1.55f, 0.2f, 0x00330000);
        std::snprintf(shadow.viewName, sizeof(shadow.viewName), "ShadowSlice0");
        shadow.viewKind = 3;
        fpbatch::NoteCall(g_s, shadow);
        fpbatch::Call unnamed = CallAt(kBatch, false, 0.3f, 1.55f, 0.2f, 0x00340000);
        unnamed.viewName[0] = 0;
        unnamed.view = 0x00550000;
        fpbatch::NoteCall(g_s, unnamed);
        fpbatch::NoteCall(g_s, unnamed);
        const fpbatch::Record& r = g_s.records[0];
        const bool four = r.viewCount == 4 && std::strcmp(r.views[0].name, "Root") == 0 && r.views[0].entriesMore &&
                          r.views[0].entryCount == fpbatch::kViewEntries && std::strcmp(r.views[1].name, "SecondaryRoot") == 0 && r.views[1].entriesMore &&
                          std::strcmp(r.views[2].name, "ShadowSlice0") == 0 && r.views[2].kind == 3 && r.views[2].entryCount == 1 && r.views[3].name[0] == 0 &&
                          r.views[3].desc == 0x00550000 && r.views[3].entryCount == 1 && r.viewsMore == 0;
        static char line[8192];
        bool group[fpbatch::kRecords] = {true};
        fpbatch::Line(g_s, 0, group, [](uint32_t, char* text, size_t) { text[0] = 0; }, line, sizeof(line));
        const bool said = std::strstr(line, "in views Root (kind 0, 2 sub-views, more than 4 view entries), SecondaryRoot (kind 0, 2 sub-views, more than 4 "
                                            "view entries), ShadowSlice0 (kind 3, 2 sub-views, 1 view entry), ? (ViewDesc 00550000, kind 0, 2 sub-views, 1 view "
                                            "entry);") != nullptr;
        Report("3e", "views by name and kind, not entry: Root and SecondaryRoot from a new entry each of 6 frames one view each (more than kViewEntries "
                     "entries), another view of its own kind kept beside them (never crowded out), one with no name by its ViewDesc",
               four && said, "%d views, %u calls in more; %s", r.viewCount, r.viewsMore, line);
    }
    {
        fpbatch::Reset(g_s);
        for (int b = 0; b < fpbatch::kRecords + 1; ++b)
            fpbatch::NoteCall(g_s, CallAt(kBatch + 0x1000 * b, false, 0.1f * b, 1.6f, 0.0f));
        fpbatch::NoteCall(g_s, CallAt(kBatch + 0x1000 * fpbatch::kRecords, false, 0.0f, 1.6f, 0.0f));
        fpbatch::NoteFault(g_s, fpbatch::kNotItsBatch);
        Report("3c", "kRecords batches kept; the calls of one more counted, not kept; a fault counted as a call",
               g_s.count == fpbatch::kRecords && g_s.notKept == 2 && g_s.calls == fpbatch::kRecords + 3 && g_s.faults[fpbatch::kNotItsBatch] == 1,
               "%d kept, %u not, %u calls", g_s.count, g_s.notKept, g_s.calls);
        const uint32_t was = g_s.generation;
        fpbatch::Reset(g_s);
        Report("3d", "the books emptied for the next second: a new generation", g_s.count == 0 && g_s.calls == 0 && g_s.generation == was + 1, "generation %u",
               g_s.generation);
    }
}

// ------------------------------------------------------------------ 4 per-view uploads ------------------------------------------------------------------

void Rows(const float* m, float* rows) {
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            rows[3 * r + c] = m[4 * r + c];
}

void Uploads() {
    fpbatch::Reset(g_s);
    fpbatch::NoteCall(g_s, CallAt(kBatch, false, 0.30f, 1.55f, 0.20f));
    fpbatch::NoteCall(g_s, CallAt(kBatch + 0x1000, false, 0.10f, 1.40f, -0.50f, kViewEntryAt, 0, 0x11));
    float rows[9];
    Rows(g_s.records[0].call.matrix, rows);
    const double exact[3] = {0.30, 1.55, 0.20}, halfMm[3] = {0.3005, 1.55, 0.20}, off[3] = {0.305, 1.55, 0.20}, other[3] = {0.10, 1.40, -0.50};
    const uint32_t a = fpbatch::MatchView(g_s, rows, exact), b = fpbatch::MatchView(g_s, rows, halfMm), c = fpbatch::MatchView(g_s, rows, off),
                   d = fpbatch::MatchView(g_s, rows, other);
    Report("4a", "a batch's camera exactly and 0.5 mm off: its; 5 mm off: none's (in the arms' space, kept as the one shown); the other batch's: the "
                 "other's",
           a == 1u && b == 1u && c == 0 && d == 2u && g_s.matched == 3 && g_s.missArms == 1 && g_s.haveMiss && std::fabs(g_s.missEye[0] - 0.305f) < 1e-6f &&
               g_s.records[0].uploads == 2 && g_s.records[1].uploads == 1 && g_s.ambiguous == 0,
           "%X %X %X %X; matched %u, missed %u", a, b, c, d, g_s.matched, g_s.missArms);
    float turned[9];
    std::memcpy(turned, rows, sizeof(turned));
    turned[0] = 0.79f, turned[2] = -0.61f;
    const uint32_t e = fpbatch::MatchView(g_s, turned, exact);
    Report("4b", "its eye with other rows: its, counted as rows off", e == 1u && g_s.rowsOff == 1, "%X, rows off %u", e, g_s.rowsOff);
    // Another kind of batch (flag 4) with that camera too: whose an upload of it is isn't known. Shared by both, never the first's alone.
    fpbatch::NoteCall(g_s, CallAt(kBatch + 0x2000, true, 0.30f, 1.55f, 0.20f, kViewEntryAt, 0, 0x05));
    const uint32_t f = fpbatch::MatchView(g_s, rows, exact);
    const bool shared = f == (1u | 4u) && g_s.ambiguous == 1 && g_s.records[0].sharedUploads == 1 && g_s.records[2].sharedUploads == 1 &&
                        g_s.records[0].uploads == 3 && g_s.records[2].uploads == 0;
    Report("4c", "two kinds of batch with one camera: shared by both (a bit each, counted for each as shared), never the first's alone", shared,
           "%X, ambiguous %u, shared %u %u, own %u %u", f, g_s.ambiguous, g_s.records[0].sharedUploads, g_s.records[2].sharedUploads, g_s.records[0].uploads,
           g_s.records[2].uploads);
    // A fourth pointer of the first's kind (made again the next frame, the head still) with that camera: one with the first, no third share.
    fpbatch::NoteCall(g_s, CallAt(kBatch + 0x3000, false, 0.30f, 1.55f, 0.20f));
    const uint32_t g = fpbatch::MatchView(g_s, rows, exact);
    // One kind's two pointers alone with one camera: its own upload, the first standing for the kind.
    fpbatch::Reset(g_t);
    fpbatch::NoteCall(g_t, CallAt(kBatch, false, 0.30f, 1.55f, 0.20f));
    fpbatch::NoteCall(g_t, CallAt(kBatch + 0x1000, false, 0.30f, 1.55f, 0.20f));
    const uint32_t h = fpbatch::MatchView(g_t, rows, exact);
    Report("4d", "a batch of a kind already found is one with it: with another kind there, still the two shared; one kind's two pointers alone, its own",
           g == (1u | 4u) && g_s.records[3].sharedUploads == 0 && g_s.ambiguous == 2 && h == 1u && g_t.ambiguous == 0 && g_t.records[0].uploads == 1 &&
               g_t.records[1].uploads == 0,
           "%X (fourth shared %u), then %X alone (ambiguous %u)", g, g_s.records[3].sharedUploads, h, g_t.ambiguous);
    // A camera of frames before: still its, until kCameras more are set.
    fpbatch::Reset(g_t);
    for (int k = 0; k < fpbatch::kCameras; ++k)
        fpbatch::NoteCall(g_t, CallAt(kBatch, false, 0.30f + 0.01f * k, 1.55f, 0.20f));
    const uint32_t i = fpbatch::MatchView(g_t, rows, exact);
    fpbatch::NoteCall(g_t, CallAt(kBatch, false, 0.9f, 1.55f, 0.20f));
    const uint32_t j = fpbatch::MatchView(g_t, rows, exact);
    const double second[3] = {0.31, 1.55, 0.20};
    const uint32_t k = fpbatch::MatchView(g_t, rows, second);
    Report("4e", "a camera of frames before: its while among its last kCameras, none's once kCameras more were set", i == 1u && j == 0 && k == 1u,
           "%X, then %X (the next %X)", i, j, k);
    const double distant[3] = {1200.0, 85.0, -431.0};
    const uint32_t missFar = g_t.missFar;
    const uint32_t l = fpbatch::MatchView(g_t, rows, distant);
    Report("4f", "an eye far from the origin, no batch's: counted farther, not in the arms' space", l == 0 && g_t.missFar == missFar + 1, "%X, farther %u", l,
           g_t.missFar);
    {
        // The cameras a per-view block is made from (the review, 2026-11-25): a far-origin flag-1 batch noted first (the view's own camera, then the
        // transform less the origin over it), then a flag-4 batch kept at the view's own camera, and a flag-1 batch with its origin within 3 cm.
        // An upload at the view's own camera is the flag-4 batch's alone, never the first's; at the first's camera less the origin, the first's.
        fpbatch::Reset(g_t);
        const float view[3] = {kOrigin[0] + 0.25f, kOrigin[1] + 1.6f, kOrigin[2] - 0.1f};
        fpbatch::NoteCall(g_t, CallAt(kBatch, true, view[0], view[1], view[2]));
        fpbatch::NoteCall(g_t, CallAt(kBatch, false, view[0] - kOrigin[0], view[1] - kOrigin[1], view[2] - kOrigin[2]));
        fpbatch::NoteCall(g_t, CallAt(kBatch + 0x1000, true, view[0], view[1], view[2], kViewEntryAt, 0, 0x05));
        const double atView[3] = {view[0], view[1], view[2]}, lessOrigin[3] = {view[0] - kOrigin[0], view[1] - kOrigin[1], view[2] - kOrigin[2]};
        const uint32_t m = fpbatch::MatchView(g_t, rows, atView), n = fpbatch::MatchView(g_t, rows, lessOrigin);
        Report("4g", "a flag-4 batch's upload at the view's own camera after a far-origin flag-1 batch's two calls: the flag-4 batch's alone, not shared "
                     "(the flag-1 batch's view's own camera never its block's); at the flag-1 batch's camera less its origin, its",
               m == 2u && n == 1u && g_t.ambiguous == 0 && g_t.records[0].camCount == 1 && g_t.records[1].camCount == 1 && g_t.records[1].uploads == 1 &&
                   g_t.records[0].uploads == 1,
               "%X, %X; ambiguous %u, cameras kept %d and %d", m, n, g_t.ambiguous, g_t.records[0].camCount, g_t.records[1].camCount);
        // The game's own test (0x010183BA, 0x010183C3, the float 0.001 at 0x0207FCFC): the view's own camera kept with flag 4, or an origin within
        // about 3 cm of 0 (x*x + y*y + z*z not over 0.001); not at 3.5 cm; the transform less the origin always; an origin not a number kept (the
        // game's test is never over it).
        fpbatch::Call keep = CallAt(kBatch, true, 0.0f, 1.6f, 0.0f);
        const bool farOrigin = !fpbatch::BlockKeeps(keep);
        keep.head[fpbatch::kFlags] = 0x05;
        const bool flag4 = fpbatch::BlockKeeps(keep);
        keep.head[fpbatch::kFlags] = 0x01;
        SetOrigin(keep, 0.02f, 0.01f, -0.01f);  // 0.0006
        const bool within = fpbatch::BlockKeeps(keep);
        SetOrigin(keep, 0.03f, 0.0f, 0.02f);  // 0.0013 (3.6 cm)
        const bool past = !fpbatch::BlockKeeps(keep);
        SetOrigin(keep, std::nanf(""), 0.0f, 0.0f);
        const bool notNumber = fpbatch::BlockKeeps(keep);
        fpbatch::Call rel = CallAt(kBatch, false, 0.0f, 1.6f, 0.0f);
        const bool lessIt = fpbatch::BlockKeeps(rel);
        Report("4h", "what the block keeps, as the game tests it: the view's own camera with flag 4 or an origin within 3 cm (and one not a number), not "
                     "with a far origin or one at 3.6 cm; the transform less the origin always",
               farOrigin && flag4 && within && past && notNumber && lessIt, "far %d, flag 4 %d, 2.4 cm %d, 3.6 cm %d, NaN %d, less the origin %d", farOrigin,
               flag4, within, past, notNumber, lessIt);
    }
}

// ------------------------------------------------------------------ 5 draws ------------------------------------------------------------------

void Draws() {
    fpbatch::Reset(g_s);
    fpbatch::NoteCall(g_s, CallAt(kBatch, false, 0.30f, 1.55f, 0.20f));
    for (int k = 0; k < 3; ++k) {
        fpbatch::NoteDraw(g_s, 1u, 12768, 0, 0, 32, 'W');
        fpbatch::NoteDraw(g_s, 1u, 9273, 12, 0, 32, 'P');
    }
    fpbatch::NoteDraw(g_s, 1u, 12768, 0, 0, 0, 'W');  // the same draw with another stencil: its own
    fpbatch::NoteDraw(g_s, 1u << 5, 1, 2, 3, 32, 'A');  // a batch not kept: nothing
    const fpbatch::Record& r = g_s.records[0];
    Report("5a", "draws kept by kind with their times and stencil; one for a batch not kept, nothing",
           r.drawCount == 3 && r.draws[0].times == 3 && r.draws[0].kind == 'W' && r.draws[1].count == 9273 && r.draws[1].times == 3 && r.draws[2].stencil == 0 &&
               g_s.draws == 7 && g_s.drawsShared == 0 && !r.draws[0].shared,
           "%d kinds, %u draws", r.drawCount, g_s.draws);
    for (uint32_t k = 0; k < 40; ++k)
        fpbatch::NoteDraw(g_s, 1u, 100 + k, 0, 0, 32, 0);
    Report("5b", "kDraws different ones kept, the rest counted", r.drawCount == fpbatch::kDraws && r.drawsMore == 40 - (fpbatch::kDraws - 3),
           "%d kept, %u more", r.drawCount, r.drawsMore);
    // After an upload two kinds of batch had the camera of: kept for each, marked shared, apart from the same draw as its own; counted once.
    fpbatch::Reset(g_s);
    fpbatch::NoteCall(g_s, CallAt(kBatch, false, 0.30f, 1.55f, 0.20f));
    fpbatch::NoteCall(g_s, CallAt(kBatch + 0x1000, true, 0.30f, 1.55f, 0.20f, kViewEntryAt, 0, 0x05));
    fpbatch::NoteDraw(g_s, 1u, 12768, 0, 0, 32, 'W');
    fpbatch::NoteDraw(g_s, 3u, 12768, 0, 0, 32, 'W');
    fpbatch::NoteDraw(g_s, 3u, 13755, 15051, 0, 32, 'A');
    const fpbatch::Record &a = g_s.records[0], &b = g_s.records[1];
    Report("5c", "a draw after a shared upload: kept for each batch, marked shared, apart from the same draw as its own; counted once",
           a.drawCount == 3 && !a.draws[0].shared && a.draws[1].shared && a.draws[1].count == 12768 && a.draws[2].shared && b.drawCount == 2 &&
               b.draws[0].shared && b.draws[1].kind == 'A' && g_s.draws == 3 && g_s.drawsShared == 2,
           "%d and %d kept; %u draws, %u shared", a.drawCount, b.drawCount, g_s.draws, g_s.drawsShared);
}

// ------------------------------------------------------------------ 6 the log ------------------------------------------------------------------

void Names(uint32_t mesh, char* text, size_t size) {
    if (mesh == 0x00800000)
        std::snprintf(text, size, "'Weapons/M416/M416_1P' (+0x10>+0x8)");
    else
        text[0] = 0;
}

void TheLog() {
    fpbatch::Reset(g_s);
    // Two batches alike (made again each frame: the second from another entry of Root, last noted with the view's own camera), a third of
    // another kind (flag 4: kept at the view's own camera).
    for (int b = 0; b < 3; ++b) {
        fpbatch::Call c = CallAt(kBatch + 0x1000 * b, b == 2, 0.30f, 1.55f, 0.20f, kViewEntryAt, 0, b == 2 ? 0x05 : 0x01);
        c.entriesRead[0] = 2;
        std::memcpy(c.head + fpbatch::kMeshCount, "\x02\0\0\0", 4);
        c.entries[0][0][0] = 0x00800000, c.entries[0][1][0] = 0x00800100;
        if (b == 2)
            c.entries[0][0][0] = 0x00900000;
        const float fov = 55.0f;
        std::memcpy(c.head + fpbatch::kFov, &fov, 4);
        fpbatch::NoteCall(g_s, c);
        if (b == 1) {
            c.abs = true;
            c.viewEntry = kViewEntryAt + 0x30;
            fpbatch::NoteCall(g_s, c);
        }
    }
    fpbatch::NoteDraw(g_s, 1u, 12768, 0, 0, 32, 'W');
    fpbatch::NoteDraw(g_s, 2u, 12768, 0, 0, 32, 'W');
    fpbatch::NoteDraw(g_s, 2u, 13755, 15051, 0, 32, 'A');
    fpbatch::NoteDraw(g_s, 1u | 4u, 9273, 12, 0, 32, 'P');  // after an upload the first and the third kind both had the camera of
    g_s.records[0].sharedUploads = 1, g_s.records[2].sharedUploads = 1;
    bool group[fpbatch::kRecords] = {}, other[fpbatch::kRecords] = {};
    for (int i = 0; i < g_s.count; ++i)
        (fpbatch::SameMeshes(g_s.records[0], g_s.records[i]) ? group : other)[i] = true;
    static char line[8192];
    fpbatch::Line(g_s, 0, group, Names, line, sizeof(line));
    const bool merged = std::strstr(line, "and 1 more batch pointers") && std::strstr(line, "; draws: 12768/0 W x2 s32, 13755/15051 A x1 s32;") &&
                        std::strstr(line, "; draws after an upload whose camera another kind of batch had too (whose, not known): 9273/12 P x1 s32") &&
                        std::strstr(line, "per-view uploads its camera's 0 (and 1 whose camera another kind of batch had too)") &&
                        std::strstr(line, "'Weapons/M416/M416_1P' (+0x10>+0x8)") && std::strstr(line, "flags 0x01") && std::strstr(line, "FOV 55.00") &&
                        std::strstr(line, "origin (1200.500 85.250 -431.750)") && std::strstr(line, "in view Root (kind 0, 2 sub-views, 2 view entries);") &&
                        std::strstr(line, "meshes (+0x78) 2: 00800000/0 'Weapons") && std::strstr(line, "second list (+0x80) 0");
    Report("6a", "two batches alike in one line (one last noted with the view's own camera): the pointers counted, the draws merged (its own, then the "
                 "shared apart), the names, flags, FOV, origin, views (their entries merged) and lists",
           other[2] && !other[0] && !other[1] && merged, "%s", line);
    fpbatch::Line(g_s, 2, other, Names, line, sizeof(line));
    Report("6b", "another kind in its own line: none of its own, the shared one", std::strstr(line, "flags 0x05") && !std::strstr(line, "more batch pointers") &&
               std::strstr(line, "; draws: none of its own seen; draws after an upload whose camera another kind of batch had too (whose, not known): 9273/12 P "
                                 "x1 s32") &&
               std::strstr(line, "its camera (the view's own)"),
           "%s", line);
    fpbatch::NoteFault(g_s, fpbatch::kNotItsCamera);
    const float rows[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
    const double eye[3] = {0.5, 1.0, 0.25};
    fpbatch::MatchView(g_s, rows, eye);
    g_s.ambiguous = 7, g_s.drawsNotBound = 5, g_s.worldBefore = 11, g_s.worldAfter = 13, g_s.drawsAfterWorld = 17;
    fpbatch::Summary(g_s, line, sizeof(line));
    Report("6c", "the summary's numbers", std::strstr(line, "last second 5 calls") && std::strstr(line, "2 of the transform made relative") &&
               std::strstr(line, "2 of the view's own first-person camera") && std::strstr(line, "1 not in the frame as read; ECX not its camera block 1") &&
               std::strstr(line, "3 batches kept") && std::strstr(line, "per-view uploads 1: 0 a batch's camera (7 of them more than one kind of batch's") &&
               std::strstr(line, "eye at 0.5000 1.0000 0.2500") && std::strstr(line, "4 draws made with a batch's per-view block (1 of them shared), 5 ending a tag") &&
               std::strstr(line, "11 before its first draw (the tag kept), 13 after its draws (the tag ended: 17 draws after it"),
           "%s", line);
}

// ------------------------------------------------------------------ 7 names and pages ------------------------------------------------------------------

void NamesAndPages() {
    char out[32];
    const auto name = [&](const char* bytes, size_t n, size_t least = 4) {
        return fpbatch::LooksLikeName(reinterpret_cast<const uint8_t*>(bytes), n, out, sizeof(out), least);
    };
    const bool whole = name("Weapons/M416/M416_1P_Mesh\0", 27) == 25 && std::strcmp(out, "Weapons/M416/M416_1P_Mesh") == 0;
    char small[8];
    const bool cut = fpbatch::LooksLikeName(reinterpret_cast<const uint8_t*>("Weapons/M416\0"), 13, small, sizeof(small)) == 12 && std::strcmp(small, "Weapons") == 0;
    const bool yes = whole && cut && name("Root\0", 5, 2) == 4;
    const bool no = !name("ab\0", 3) && !name("1234\0", 5) && !name("Weap\x01on\0", 8) && !name("Weapons", 7) && !name("\0", 1);
    Report("7a", "names: printable, NUL-ended within, two letters at least (cut to the room given); a short, numeric, broken or unended run none",
           yes && no, "%s, %s", yes ? "taken" : "NOT taken", no ? "refused" : "NOT refused");
    const bool reads = fpbatch::ReadableProtect(PAGE_READWRITE) && fpbatch::ReadableProtect(PAGE_READONLY) && fpbatch::ReadableProtect(PAGE_EXECUTE_READ) &&
                       fpbatch::ReadableProtect(PAGE_WRITECOPY);
    const bool never = !fpbatch::ReadableProtect(PAGE_READWRITE | PAGE_WRITECOMBINE) && !fpbatch::ReadableProtect(PAGE_READWRITE | PAGE_NOCACHE) &&
                       !fpbatch::ReadableProtect(PAGE_READWRITE | PAGE_GUARD) && !fpbatch::ReadableProtect(PAGE_NOACCESS) && !fpbatch::ReadableProtect(0);
    Report("7b", "pages: ordinary readable ones read; write-combined, uncached, guard and no-access never", reads && never, "%s, %s", reads ? "read" : "NOT read",
           never ? "never" : "READ");
}

// ------------------------------------------------------------------ 8 the stub ------------------------------------------------------------------

#if defined(_M_IX86)
// The registers handed in, the ones seen after, the stand-in's view of its call.
uint32_t g_inEax, g_inEcx, g_inEdx, g_inEbx, g_inEsi, g_inEdi, g_inEbp, g_arg;
uint32_t g_outEbx, g_outEsi, g_outEdi, g_outEbp, g_espBefore, g_espAfter;
uint32_t g_standEax, g_standEcx, g_standEdx, g_standArg, g_standRuns;

// A stand-in for the game's setter: as it is (16 dwords copied from its argument into ECX's block, 9 ORed into +0x80, ret 4), noting its
// registers as they came.
__declspec(naked) void StandInSetter() {
    __asm {
        mov dword ptr [g_standEax], eax
        mov dword ptr [g_standEcx], ecx
        mov dword ptr [g_standEdx], edx
        mov eax, dword ptr [esp + 4]
        mov dword ptr [g_standArg], eax
        inc dword ptr [g_standRuns]
        push esi
        push edi
        mov esi, eax
        mov edi, ecx
        push ecx
        mov ecx, 16
        rep movsd
        pop ecx
        or dword ptr [ecx + 80h], 9
        pop edi
        pop esi
        ret 4
    }
}

// Three callers alike, each its own call of the stub: the batch camera's two calls (their returns are put in g_fpRelReturn and g_fpAbsReturn)
// and another. Each says which it is first, so the linker can't fold them into one (a Release build folds identical functions).
uint32_t g_caller = 0;
__declspec(naked) void CallerRel() {
    __asm {
        mov dword ptr [g_caller], 1
        pushad
        mov dword ptr [g_espBefore], esp
        mov ebx, dword ptr [g_inEbx]
        mov esi, dword ptr [g_inEsi]
        mov edi, dword ptr [g_inEdi]
        mov ebp, dword ptr [g_inEbp]
        mov eax, dword ptr [g_inEax]
        mov ecx, dword ptr [g_inEcx]
        mov edx, dword ptr [g_inEdx]
        push dword ptr [g_arg]
        call FpSetterHook
        mov dword ptr [g_outEbx], ebx
        mov dword ptr [g_outEsi], esi
        mov dword ptr [g_outEdi], edi
        mov dword ptr [g_outEbp], ebp
        mov dword ptr [g_espAfter], esp
        popad
        ret
    }
}
__declspec(naked) void CallerAbs() {
    __asm {
        mov dword ptr [g_caller], 2
        pushad
        mov dword ptr [g_espBefore], esp
        mov ebx, dword ptr [g_inEbx]
        mov esi, dword ptr [g_inEsi]
        mov edi, dword ptr [g_inEdi]
        mov ebp, dword ptr [g_inEbp]
        mov eax, dword ptr [g_inEax]
        mov ecx, dword ptr [g_inEcx]
        mov edx, dword ptr [g_inEdx]
        push dword ptr [g_arg]
        call FpSetterHook
        mov dword ptr [g_outEbx], ebx
        mov dword ptr [g_outEsi], esi
        mov dword ptr [g_outEdi], edi
        mov dword ptr [g_outEbp], ebp
        mov dword ptr [g_espAfter], esp
        popad
        ret
    }
}
__declspec(naked) void CallerOther() {
    __asm {
        mov dword ptr [g_caller], 3
        pushad
        mov dword ptr [g_espBefore], esp
        mov ebx, dword ptr [g_inEbx]
        mov esi, dword ptr [g_inEsi]
        mov edi, dword ptr [g_inEdi]
        mov ebp, dword ptr [g_inEbp]
        mov eax, dword ptr [g_inEax]
        mov ecx, dword ptr [g_inEcx]
        mov edx, dword ptr [g_inEdx]
        push dword ptr [g_arg]
        call FpSetterHook
        mov dword ptr [g_outEbx], ebx
        mov dword ptr [g_outEsi], esi
        mov dword ptr [g_outEdi], edi
        mov dword ptr [g_outEbp], ebp
        mov dword ptr [g_espAfter], esp
        popad
        ret
    }
}

// Where a caller's call of the stub returns to: found in its code (an incremental link's jump to it followed).
uintptr_t CallReturn(void (*fn)()) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(fn);
    if (p[0] == 0xE9) {
        int32_t rel = 0;
        std::memcpy(&rel, p + 1, 4);
        p = p + 5 + rel;
    }
    const uintptr_t hook = reinterpret_cast<uintptr_t>(&FpSetterHook);
    for (int i = 0; i < 128; ++i) {
        int32_t rel = 0;
        std::memcpy(&rel, p + i + 1, 4);
        if (p[i] == 0xE8 && reinterpret_cast<uintptr_t>(p + i + 5) + static_cast<uintptr_t>(rel) == hook)
            return reinterpret_cast<uintptr_t>(p + i + 5);
    }
    return 0;
}
#endif

}  // namespace

// The stub's call (fpbatchstub.h: the includer's): what it was handed, and the return address on the stack as the setter was entered.
namespace {
uint32_t g_noted = 0, g_notedReturn = 0;
uintptr_t g_notedArgs[5] = {};
bool g_spoil = false;  // the call made to use the registers the stub must keep for the setter (EAX, ECX, EDX) as its own

void __stdcall FpNoteCall(const uint8_t* camera, const float* matrix, uintptr_t ebx, uintptr_t ebp, uintptr_t entry) {
    ++g_noted;
    g_notedArgs[0] = reinterpret_cast<uintptr_t>(camera);
    g_notedArgs[1] = reinterpret_cast<uintptr_t>(matrix);
    g_notedArgs[2] = ebx;
    g_notedArgs[3] = ebp;
    g_notedArgs[4] = entry;
    g_notedReturn = *reinterpret_cast<const uint32_t*>(entry);
#if defined(_M_IX86)
    if (g_spoil) {
        __asm {
            mov eax, 0x0BADF00D
            mov ecx, 0x0BADF00D
            mov edx, 0x0BADF00D
        }
    }
#endif
}
}  // namespace

namespace {

void Stub() {
#if defined(_M_IX86)
    g_fpSetterOriginal = reinterpret_cast<void*>(&StandInSetter);
    g_fpRelReturn = CallReturn(&CallerRel);
    g_fpAbsReturn = CallReturn(&CallerAbs);
    const uintptr_t otherReturn = CallReturn(&CallerOther);
    Report("8a", "the three callers' calls of the stub found (where each returns to, three places)", g_fpRelReturn && g_fpAbsReturn && otherReturn &&
               g_fpRelReturn != g_fpAbsReturn && otherReturn != g_fpRelReturn && otherReturn != g_fpAbsReturn,
           "%08X %08X %08X", static_cast<unsigned>(g_fpRelReturn), static_cast<unsigned>(g_fpAbsReturn), static_cast<unsigned>(otherReturn));
    alignas(16) static float block[64];
    alignas(16) static float matrix[16];
    for (int i = 0; i < 16; ++i)
        matrix[i] = 0.5f + i;
    const auto run = [&](void (*caller)(), bool spoil) {
        std::memset(block, 0, sizeof(block));
        g_inEax = 0xAAAA0001u, g_inEcx = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(block)), g_inEdx = 0xDDDD0002u;
        g_inEbx = 0xB0B0B0B0u, g_inEsi = 0x51515151u, g_inEdi = 0xD1D1D1D1u, g_inEbp = 0xEBEBEB00u;
        g_arg = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(matrix));
        g_standRuns = 0, g_standEax = g_standEcx = g_standEdx = g_standArg = 0;
        g_spoil = spoil;
        caller();
        g_spoil = false;
    };
    const auto kept = [&] {
        uint32_t stale = 0;
        std::memcpy(&stale, reinterpret_cast<const uint8_t*>(block) + 0x80, 4);
        return g_standRuns == 1 && g_standEcx == g_inEcx && g_standArg == g_arg && g_standEax == g_inEax && g_standEdx == g_inEdx &&
               std::memcmp(block, matrix, 64) == 0 && stale == 9 && g_outEbx == g_inEbx && g_outEsi == g_inEsi && g_outEdi == g_inEdi &&
               g_outEbp == g_inEbp && g_espAfter == g_espBefore;
    };
    for (int k = 0; k < 2; ++k) {
        const bool abs = k == 1;
        const uint32_t noted = g_noted;
        run(abs ? &CallerAbs : &CallerRel, true);
        const bool note = g_noted == noted + 1 && g_notedArgs[0] == g_inEcx && g_notedArgs[1] == g_arg && g_notedArgs[2] == g_inEbx &&
                          g_notedArgs[3] == g_inEbp && g_notedArgs[4] == g_espBefore - 8 && g_notedReturn == (abs ? g_fpAbsReturn : g_fpRelReturn);
        Report(abs ? "8c" : "8b",
               abs ? "the view's own camera's call: noted alike, then the setter as it came"
                   : "the transform less the origin's call: noted with ECX, the matrix, EBX, EBP and the stack as entered; then the stand-in setter with ECX, "
                     "its argument, EAX and EDX as they came (the note spoiling them); EBX, ESI, EDI, EBP and the stack after ret 4 as before",
               note && kept(), "noted %u (camera %08X, matrix %08X, EBX %08X, EBP %08X, entry %08X less the caller's %08X, return %08X); the setter ran %u "
               "(ECX %08X, argument %08X, EAX %08X, EDX %08X); after: EBX %08X ESI %08X EDI %08X EBP %08X, the stack %+d",
               g_noted - noted, static_cast<unsigned>(g_notedArgs[0]), static_cast<unsigned>(g_notedArgs[1]), static_cast<unsigned>(g_notedArgs[2]),
               static_cast<unsigned>(g_notedArgs[3]), static_cast<unsigned>(g_notedArgs[4]), g_espBefore, g_notedReturn, g_standRuns, g_standEcx, g_standArg,
               g_standEax, g_standEdx, g_outEbx, g_outEsi, g_outEdi, g_outEbp, static_cast<int>(g_espAfter - g_espBefore));
    }
    {
        const uint32_t noted = g_noted;
        run(&CallerOther, false);
        Report("8d", "any other call: straight on into the setter, nothing noted, everything kept", g_noted == noted && kept(),
               "noted %u; the setter ran %u (ECX %08X, EAX %08X, EDX %08X); the stack %+d", g_noted - noted, g_standRuns, g_standEcx, g_standEax, g_standEdx,
               static_cast<int>(g_espAfter - g_espBefore));
    }
    {
        // With neither call wanted (0 0, as before the hook is made): nothing noted.
        const uintptr_t rel = g_fpRelReturn, abs = g_fpAbsReturn;
        g_fpRelReturn = g_fpAbsReturn = 0;
        const uint32_t noted = g_noted;
        run(&CallerRel, false);
        g_fpRelReturn = rel, g_fpAbsReturn = abs;
        Report("8e", "neither call wanted: the batch camera's own passes straight on too", g_noted == noted && kept(), "noted %u", g_noted - noted);
    }
#else
    Skip("8", "the stub is the 32-bit build's only");
#endif
}

// ------------------------------------------------------------------ 9 the time ------------------------------------------------------------------

void Time() {
    // A call taken into the books, as the hook does on the game's thread (the read's work done by memcpy from a flat copy).
    fpbatch::Reset(g_s);
    fpbatch::Call c = CallAt(kBatch, false, 0.3f, 1.55f, 0.2f);
    constexpr int kRuns = 200000;
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < kRuns; ++i) {
        c.batch = kBatch + 0x1000 * (i % 8);
        c.matrix[12] = 0.3f + 1e-6f * i;
        fpbatch::NoteCall(g_s, c);
    }
    const double callUs = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count() / kRuns;
    // An upload matched against full books (kRecords batches, kCameras each), none its: the most work.
    fpbatch::Reset(g_s);
    for (int b = 0; b < fpbatch::kRecords; ++b)
        for (int k = 0; k < fpbatch::kCameras; ++k)
            fpbatch::NoteCall(g_s, CallAt(kBatch + 0x1000 * b, false, 0.1f * b, 1.6f + 0.01f * k, 0.0f));
    float rows[9];
    Rows(g_s.records[0].call.matrix, rows);
    const double eye[3] = {9.0, 9.0, 9.0};
    start = std::chrono::steady_clock::now();
    for (int i = 0; i < kRuns; ++i)
        fpbatch::MatchView(g_s, rows, eye);
    const double matchUs = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count() / kRuns;
    double passNs = 0.0;
#if defined(_M_IX86)
    // A call the stub passes by (every other caller of the setter, while the probe is on), against the stand-in called straight.
    g_inEcx = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&g_s.records[0].cams[0][0])), g_arg = g_inEcx;
    start = std::chrono::steady_clock::now();
    for (int i = 0; i < kRuns; ++i)
        CallerOther();
    passNs = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - start).count() / kRuns;
#endif
    Report("9", "the time: a call taken, an upload matched against full books (the hook's and the draw hook's work while on), a call passed by",
           callUs < 5.0 && matchUs < 20.0 && passNs < 1000.0, "%.3f us a call, %.3f us a match, %.1f ns a call passed by (its caller's work too)", callUs, matchUs,
           passNs);
}

// ------------------------------------------------------------------ 10 a thread's tag ------------------------------------------------------------------

void Tags() {
    float rows[9];
    {
        const fpbatch::Call c = CallAt(kBatch, false, 0.30f, 1.55f, 0.20f);
        Rows(c.matrix, rows);
    }
    const double eye[3] = {0.30, 1.55, 0.20}, nobody[3] = {0.9, 1.2, 0.4};
    static int buffer = 0, otherBuffer = 0;  // (only their places: the buffers' stand-ins)
    {
        // Its upload, two of its draws with its buffer bound, then one without it: the tag ends there; a draw after with the buffer bound again
        // (a binding left over) is nobody's.
        fpbatch::Reset(g_t);
        fpbatch::NoteCall(g_t, CallAt(kBatch, false, 0.30f, 1.55f, 0.20f));
        fpbatch::Tag t = fpbatch::TagView(g_t, &buffer, rows, eye);
        const bool tagged = t.generation == g_t.generation && t.batches == 1u && t.buffer == &buffer;
        fpbatch::TagDraw(g_t, t, true, 12768, 0, 0, 32, 'W');
        fpbatch::TagDraw(g_t, t, true, 9273, 12, 0, 32, 'P');
        fpbatch::TagDraw(g_t, t, false, 6, 0, 0, 0, 0);
        const bool ended = t.generation == 0;
        fpbatch::TagDraw(g_t, t, true, 4, 0, 0, 0, 0);
        Report("10a", "a batch's upload tags the thread's draws: its two while its buffer is bound; the first without it ends the tag, a draw after "
                      "it with the buffer bound again nobody's",
               tagged && ended && g_t.records[0].drawCount == 2 && g_t.draws == 2 && g_t.drawsNotBound == 1,
               "tagged %d, ended %d; its draws %d, %u in all, %u ending a tag", tagged, ended, g_t.records[0].drawCount, g_t.draws, g_t.drawsNotBound);
    }
    {
        // An 80-byte view-projection before the first draw: counted, the tag kept, the draw its; one after: the tag ended (a second not counted
        // again), a draw still finding the buffer bound counted as nobody's, the first without it ending the watch unseen, nothing after.
        fpbatch::Reset(g_t);
        fpbatch::NoteCall(g_t, CallAt(kBatch, false, 0.30f, 1.55f, 0.20f));
        fpbatch::Tag t = fpbatch::TagView(g_t, &buffer, rows, eye);
        fpbatch::TagWorldView(g_t, t);
        const bool keptBefore = t.generation != 0 && !t.ended && g_t.worldBefore == 1;
        fpbatch::TagDraw(g_t, t, true, 12768, 0, 0, 32, 'W');
        fpbatch::TagWorldView(g_t, t);
        const bool endedAfter = t.generation != 0 && t.ended && g_t.worldAfter == 1;
        fpbatch::TagWorldView(g_t, t);
        fpbatch::TagDraw(g_t, t, true, 36, 0, 0, 0, 0);
        fpbatch::TagDraw(g_t, t, false, 6, 0, 0, 0, 0);
        fpbatch::TagDraw(g_t, t, true, 36, 0, 0, 0, 0);
        Report("10b", "an 80-byte view-projection before the batch's first draw: counted, the tag kept; after its draws: the tag ended, draws still "
                      "bound only counted (nobody's), the first unbound one ending that too",
               keptBefore && endedAfter && g_t.worldBefore == 1 && g_t.worldAfter == 1 && g_t.drawsAfterWorld == 1 && g_t.drawsNotBound == 0 &&
                   t.generation == 0 && g_t.records[0].drawCount == 1 && g_t.draws == 1,
               "kept before %d, ended after %d; before %u, after %u, draws after %u, ending a tag %u; its draws %d", keptBefore, endedAfter,
               g_t.worldBefore, g_t.worldAfter, g_t.drawsAfterWorld, g_t.drawsNotBound, g_t.records[0].drawCount);
    }
    {
        // A per-view upload that's no batch's camera, or no view at all, ends the tag.
        fpbatch::Reset(g_t);
        fpbatch::NoteCall(g_t, CallAt(kBatch, false, 0.30f, 1.55f, 0.20f));
        fpbatch::Tag t = fpbatch::TagView(g_t, &buffer, rows, eye);
        t = fpbatch::TagView(g_t, &otherBuffer, rows, nobody);
        const bool nobodysEnds = t.generation == 0;
        t = fpbatch::TagView(g_t, &buffer, rows, eye);
        t = fpbatch::TagView(g_t, &otherBuffer, nullptr, nullptr);
        const bool notViewEnds = t.generation == 0 && g_t.notViews == 1;
        fpbatch::TagDraw(g_t, t, true, 12768, 0, 0, 32, 'W');
        Report("10c", "a per-view upload of no batch's camera, or not a view, ends the tag: the draw after it nobody's",
               nobodysEnds && notViewEnds && g_t.draws == 0, "no batch's %d, not a view %d; %u draws", nobodysEnds, notViewEnds, g_t.draws);
    }
    {
        // A tag of the last second's books: its draw and its view-projection nobody's, and each ends it.
        fpbatch::Reset(g_t);
        fpbatch::NoteCall(g_t, CallAt(kBatch, false, 0.30f, 1.55f, 0.20f));
        fpbatch::Tag t = fpbatch::TagView(g_t, &buffer, rows, eye);
        fpbatch::Tag stale = t;
        fpbatch::Reset(g_t);
        fpbatch::NoteCall(g_t, CallAt(kBatch, false, 0.30f, 1.55f, 0.20f));
        fpbatch::TagDraw(g_t, t, true, 12768, 0, 0, 32, 'W');
        fpbatch::TagWorldView(g_t, stale);
        Report("10d", "a tag of the last second's books: its draw and its view-projection nobody's, and each ends it",
               t.generation == 0 && stale.generation == 0 && g_t.draws == 0 && g_t.worldBefore == 0 && g_t.records[0].drawCount == 0,
               "%u draws, %u view-projections", g_t.draws, g_t.worldBefore);
    }
    {
        // A shared upload's tag: its draws kept for both kinds of batch, shared.
        fpbatch::Reset(g_t);
        fpbatch::NoteCall(g_t, CallAt(kBatch, false, 0.30f, 1.55f, 0.20f));
        fpbatch::NoteCall(g_t, CallAt(kBatch + 0x1000, true, 0.30f, 1.55f, 0.20f, kViewEntryAt, 0, 0x05));
        fpbatch::Tag t = fpbatch::TagView(g_t, &buffer, rows, eye);
        fpbatch::TagDraw(g_t, t, true, 13755, 15051, 0, 32, 'A');
        Report("10e", "a shared upload's draws: kept for both kinds of batch, shared",
               t.batches == 3u && g_t.drawsShared == 1 && g_t.records[0].drawCount == 1 && g_t.records[0].draws[0].shared && g_t.records[1].drawCount == 1 &&
                   g_t.records[1].draws[0].shared,
               "batches %X, %u shared", t.batches, g_t.drawsShared);
    }
}

}  // namespace

int main() {
    const auto start = std::chrono::steady_clock::now();
    Code();
    Calls();
    Books();
    Uploads();
    Draws();
    TheLog();
    NamesAndPages();
    Stub();
    Tags();
    Time();
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::printf("%d checks, %d failed, %d skipped (%.0f ms)\n", g_counted, g_failed, g_skipped, ms);
    return g_failed;
}

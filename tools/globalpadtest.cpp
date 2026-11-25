// The game's pad read with its window in the background (src/globalpad.h, header only, as pad.cpp's KeepGlobalInput runs it) without the game.
//   1  the proofs on a memory image made up as bf3.exe 1.6.0.0 has it: all borne out; each broken alone fails alone, named; at another base too
//   2  a look at the setting: the game not keeping its ClientSettings yet, a pointer that isn't one, a byte that isn't a bool, off (written), on
//   3  against the real code (build\bf3_mem.exe beside the test, SKIP without it): every proof borne out; UseGlobalGamePadInput among ClientSettings'
//      own fields (by walking them); the import the first three places call is USER32's GetFocus (the fourth asks no focus) (the original import table); the setting off in the
//      dump's own ClientSettings can't be read there (heap, not in the image): the look says so, never a fault
//   4  the time a look takes
// Usage: globalpadtest
// The exit code is the number of failed checks.

#include <windows.h>

#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "../src/globalpad.h"

namespace {

int g_failed = 0, g_counted = 0, g_skipped = 0;

void Report(const char* id, const char* what, bool pass, const char* format, ...) {
    char numbers[1024];
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

constexpr uint32_t kBase = 0x400000;

// ------------------------------------------------------------------ a memory image ------------------------------------------------------------------

struct Image {
    std::map<uint32_t, std::vector<uint8_t>> pages;
    mutable int reads = 0;
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
    void W8(uint32_t at, uint8_t v) { Put(at, &v, 1); }
    void Text(uint32_t at, const char* s) { Put(at, s, std::strlen(s) + 1); }
    void Drop(uint32_t at) { pages.erase(at >> 12); }
    bool operator()(uint32_t at, void* out, size_t n) const {
        ++reads;
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

// Bytes from a pattern ("A1 ?? 85": ?? as 0).
std::vector<uint8_t> Bytes(const char* pattern) {
    std::vector<uint8_t> out;
    for (const char* p = pattern; *p;) {
        if (*p == ' ') {
            ++p;
            continue;
        }
        out.push_back(p[0] == '?' ? 0 : static_cast<uint8_t>(std::strtoul(std::string(p, 2).c_str(), nullptr, 16)));
        p += 2;
    }
    return out;
}

// Where the made-up image keeps what the constants don't place: the bool type's data and name, the class's name, getType's code (RVAs).
constexpr uint32_t kBoolData = 0x1FC22B0, kBoolName = 0x1C10000, kClassName = 0x1DB9A98, kGetType = 0x1BB9800;

// The image as bf3.exe 1.6.0.0 has it, at base.
void Build(Image& m, uint32_t base) {
    using namespace globalpad;
    // The field record and its type.
    m.W32(base + kFieldRecord, base + kFieldName);
    m.W32(base + kFieldRecord + 4, kRecordFlags);
    m.W32(base + kFieldRecord + 8, base + kBoolType);
    m.Text(base + kFieldName, "UseGlobalGamePadInput");
    m.W32(base + kBoolType, base + kBoolData);
    m.W32(base + kBoolData, base + kBoolName);
    m.Text(base + kBoolName, "Boolean");
    // ClientSettings' type: its info, its data (name, flags and size, field count, fields).
    m.W32(base + kClassInfo, base + kClassData);
    m.W32(base + kClassData, base + kClassName);
    m.Text(base + kClassName, "ClientSettings");
    m.W32(base + kClassData + 4, 0x009C0035);
    m.W32(base + kClassData + 0x0C, 0x00004504);
    m.W32(base + kClassData + 0x18, base + kFields);
    // Its vtable and getType.
    m.W32(base + kVtable, base + kGetType);
    const uint8_t getType[6] = {0xB8, 0, 0, 0, 0, 0xC3};
    m.Put(base + kGetType, getType, 6);
    m.W32(base + kGetType + 1, base + kClassInfo);
    // The four places.
    for (const Site& s : kSite) {
        const uint32_t at = base + s.at;
        const std::vector<uint8_t> b = Bytes(s.bytes);
        m.Put(at, b.data(), b.size());
        m.W32(at + s.global1, base + kSettingsGlobal);
        m.W32(at + s.global2, base + kSettingsGlobal);
        m.W32(at + s.manager, base + kSettingsManager);
        m.W32(at + s.info, base + kClassInfo);
        m.W32(at + s.call + 1, base + kGetter - (at + s.call + 5));
        if (s.focus)
            m.W32(at + s.focus, base + kGetFocusSlot);
    }
}

// Proof i broken alone.
void Break(Image& m, uint32_t base, int i) {
    using namespace globalpad;
    switch (i) {
    case 0: m.W32(base + kFieldRecord + 4, 0x008B0000); break;   // (another offset)
    case 1: m.W32(base + kClassData + 0x18, base + kFields + 12); break;  // (the fields elsewhere)
    case 2: m.W8(base + kGetType, 0xB9); break;                  // (getType not mov eax)
    case 3: m.W8(base + kSite[0].at + kSite[0].test + 2, 0x89); break;  // (another byte tested)
    case 4: m.W32(base + kSite[1].at + kSite[1].focus, base + kGetFocusSlot + 4); break;  // (another import)
    case 5: m.W32(base + kSite[2].at + kSite[2].call + 1, 0x1234); break;  // (another getter)
    case 6: m.W32(base + kSite[3].at + kSite[3].info, base + kClassInfo + 4); break;  // (another type asked for)
    default: break;
    }
}

void Proofs() {
    for (uint32_t base : {kBase, 0x10000000u}) {
        Image m;
        Build(m, base);
        const globalpad::Proofs all = globalpad::Prove(m, base);
        int alone = 0;
        char wrong[256] = "";
        for (int i = 0; i < globalpad::kProofs; ++i) {
            Image b;
            Build(b, base);
            Break(b, base, i);
            const globalpad::Proofs p = globalpad::Prove(b, base);
            if (!p.ok && p.failed == i)
                ++alone;
            else
                std::snprintf(wrong + std::strlen(wrong), sizeof(wrong) - std::strlen(wrong), " %d (%s)", i, p.ok ? "borne out" : "another failed");
        }
        Report(base == kBase ? "1a" : "1b",
               base == kBase ? "the proofs on a made-up image: all borne out; each broken alone fails alone, named"
                             : "... the image at another base (0x10000000): the same, every constant from the base",
               all.ok && alone == globalpad::kProofs, "all %s; %d of %d broken alone found%s", all.ok ? "borne out" : globalpad::ProofName(all.failed), alone,
               globalpad::kProofs, wrong);
    }
    // An image with nothing in it, and one whose pages end in the middle of a site: no proof, never a fault.
    Image empty;
    Image cut;
    Build(cut, kBase);
    cut.Drop(kBase + globalpad::kSite[3].at + 40);
    const globalpad::Proofs e = globalpad::Prove(empty, kBase), c = globalpad::Prove(cut, kBase);
    Report("1c", "nothing there, or a site's page not there: not borne out, the first such named", !e.ok && e.failed == 0 && !c.ok && c.failed == 6,
           "empty: %s; cut: %s", globalpad::ProofName(e.failed), globalpad::ProofName(c.failed));
}

void Looks() {
    using namespace globalpad;
    constexpr uint32_t kObject = 0xE5A00000u;
    Image m;
    Build(m, kBase);
    // Not kept yet.
    const Look notYet = Read(m, kBase);
    // Kept, but not a ClientSettings.
    m.W32(kBase + kSettingsGlobal, kObject);
    m.W32(kObject, kBase + kVtable + 8);
    const Look wrongType = Read(m, kBase);
    // A ClientSettings whose byte isn't a bool.
    m.W32(kObject, kBase + kVtable);
    m.W8(kObject + kField, 7);
    const Look notBool = Read(m, kBase);
    // Off: to be written.
    m.W8(kObject + kField, 0);
    const Look off = Read(m, kBase);
    // On.
    m.W8(kObject + kField, 1);
    const Look on = Read(m, kBase);
    // The object's page gone.
    m.Drop(kObject);
    const Look gone = Read(m, kBase);
    Report("2a", "not kept yet: nothing to do, looked at again", !notYet.ok && notYet.miss == Miss::kNotYet && !Wants(notYet), "%s", MissName(notYet.miss));
    Report("2b", "a pointer that isn't a ClientSettings (its vtable), or not readable: left alone", !wrongType.ok && wrongType.miss == Miss::kType &&
           !Wants(wrongType) && !gone.ok && gone.miss == Miss::kType, "%s; gone: %s", MissName(wrongType.miss), MissName(gone.miss));
    Report("2c", "a byte that isn't 0 or 1: left alone", !notBool.ok && notBool.miss == Miss::kValue && !Wants(notBool), "%s", MissName(notBool.miss));
    Report("2d", "off: written, at the object's +0x8A", off.ok && !off.on && Wants(off) && off.at == kObject + kField, "at %08X", off.at);
    Report("2e", "on: nothing written", on.ok && on.on && !Wants(on), "%s", on.on ? "on" : "off");
}

// ------------------------------------------------------------------ the real code ------------------------------------------------------------------

struct FileImage {
    std::vector<uint8_t> bytes;
    bool operator()(uint32_t at, void* out, size_t n) const {
        if (at < kBase || at - kBase > bytes.size() || n > bytes.size() - (at - kBase))
            return false;
        std::memcpy(out, bytes.data() + (at - kBase), n);
        return true;
    }
};

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

std::wstring OwnFolder() {
    wchar_t self[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    std::wstring s = self;
    return s.substr(0, s.find_last_of(L"\\/") + 1);
}

std::string TextAt(const FileImage& m, uint32_t at) {
    char t[80] = {};
    if (!m(at, t, sizeof(t) - 1)) {
        for (size_t n = 1; n < sizeof(t) - 1; ++n)
            if (!m(at, t, n))
                break;
    }
    t[sizeof(t) - 1] = 0;
    return t;
}

// The original import table in the image's .idata: the DLL and the name of the import whose slot is at slotVa.
bool ImportAt(const FileImage& m, uint32_t slotVa, std::string& dll, std::string& name) {
    const uint8_t* b = m.bytes.data();
    if (m.bytes.size() < 0x400)
        return false;
    uint32_t pe = 0;
    std::memcpy(&pe, b + 0x3C, 4);
    uint16_t sections = 0, optional = 0;
    std::memcpy(&sections, b + pe + 6, 2);
    std::memcpy(&optional, b + pe + 20, 2);
    uint32_t idataVa = 0, idataSize = 0;
    for (uint16_t s = 0; s < sections; ++s) {
        const uint8_t* h = b + pe + 24 + optional + 40 * s;
        if (std::memcmp(h, ".idata", 6) == 0) {
            std::memcpy(&idataSize, h + 8, 4);
            std::memcpy(&idataVa, h + 12, 4);
        }
    }
    if (!idataVa)
        return false;
    for (uint32_t at = kBase + idataVa; at + 20 <= kBase + idataVa + idataSize; at += 4) {
        uint32_t d[5] = {};
        if (!m(at, d, sizeof(d)) || !d[0] || !d[3] || !d[4] || d[0] > 0x3000000 || d[3] > 0x3000000 || d[4] > 0x3000000)
            continue;
        const std::string lib = TextAt(m, kBase + d[3]);
        if (lib.size() < 5 || _stricmp(lib.c_str() + lib.size() - 4, ".dll") != 0)
            continue;
        for (uint32_t i = 0; i < 2000; ++i) {
            uint32_t thunk = 0;
            if (!m(kBase + d[0] + 4 * i, &thunk, 4) || !thunk)
                break;
            if (kBase + d[4] + 4 * i != slotVa)
                continue;
            if (thunk & 0x80000000u)
                return false;
            dll = lib;
            name = TextAt(m, kBase + thunk + 2);
            return true;
        }
    }
    return false;
}

void Real() {
    FileImage real;
    if (!LoadFile(OwnFolder() + L"bf3_mem.exe", real.bytes) || real.bytes.size() < 0x2000000) {
        Skip("3a", "no build\\bf3_mem.exe beside the test: the constants against the real code not checked");
        return;
    }
    int borne = 0;
    char notBorne[512] = "";
    for (int i = 0; i < globalpad::kProofs; ++i) {
        uint32_t at = 0;
        if (globalpad::ProveOne(real, kBase, i, &at))
            ++borne;
        else
            std::snprintf(notBorne + std::strlen(notBorne), sizeof(notBorne) - std::strlen(notBorne), " %d %s (%08X)", i, globalpad::ProofName(i), at);
    }
    const globalpad::Proofs p = globalpad::Prove(real, kBase);
    Report("3a", "the real code (build\\bf3_mem.exe, bf3.exe 1.6.0.0 decrypted): every constant borne out (the field record, ClientSettings' type and "
                 "vtable, the four places)",
           p.ok && borne == globalpad::kProofs, "%d of %d borne out%s", borne, globalpad::kProofs, notBorne);
    // ClientSettings' fields walked: UseGlobalGamePadInput among them, at the record the constants name, a Boolean at +0x8A.
    uint32_t data = 0, fields = 0;
    uint8_t count = 0;
    real(kBase + globalpad::kClassInfo, &data, 4);
    real(data + 0x18, &fields, 4);
    real(data + 0x0D, &count, 1);
    int index = -1, named = 0;
    uint32_t record = 0, flags = 0;
    for (int i = 0; i < count; ++i) {
        uint32_t r[3] = {};
        if (!real(fields + 12 * i, r, sizeof(r)))
            break;
        named += !TextAt(real, r[0]).empty();
        if (TextAt(real, r[0]) == "UseGlobalGamePadInput") {
            index = i;
            record = fields + 12 * i;
            flags = r[1];
        }
    }
    Report("3b", "ClientSettings' own fields walked: UseGlobalGamePadInput one of them, the record the constants name, at +0x8A",
           index >= 0 && record == kBase + globalpad::kFieldRecord && (flags >> 16) == globalpad::kField && named == count,
           "%d fields, all named: %s; it is the %dth at %08X, +0x%X", count, named == count ? "yes" : "no", index + 1, record, flags >> 16);
    std::string dll, name;
    const bool found = ImportAt(real, kBase + globalpad::kGetFocusSlot, dll, name);
    Report("3c", "the import the first three places call (FF 15 [023BDECC]; the fourth asks no focus) is USER32's GetFocus (the original import table)",
           found && _stricmp(dll.c_str(), "USER32.dll") == 0 && name == "GetFocus", "%s!%s", found ? dll.c_str() : "?", found ? name.c_str() : "?");
    // The dump's own ClientSettings pointer is a heap address, not in the image: the look can't read the object (a type miss), never a fault.
    const globalpad::Look l = globalpad::Read(real, kBase);
    Report("3d", "a look on the image alone (its ClientSettings is on the heap, not in it): not read, nothing to write", !l.ok && !globalpad::Wants(l),
           "%s (the pointer %08X)", globalpad::MissName(l.miss), l.settings);
}

void Time() {
    Image m;
    Build(m, kBase);
    m.W32(kBase + globalpad::kSettingsGlobal, 0xE5A00000u);
    m.W32(0xE5A00000u, kBase + globalpad::kVtable);
    const auto start = std::chrono::steady_clock::now();
    int on = 0;
    for (int i = 0; i < 10000; ++i)
        on += globalpad::Read(m, kBase).ok;
    const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count() / 10000.0;
    const auto proveStart = std::chrono::steady_clock::now();
    const bool proved = globalpad::Prove(m, kBase).ok;
    const double proveUs = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - proveStart).count();
    Report("4", "the time: a look (once a second), the proofs (once a run)", on == 10000 && proved && us < 50.0, "a look %.2f us (here, on a map of pages: "
           "the game's own reads are plain), the proofs %.0f us", us, proveUs);
}

}  // namespace

int main() {
    Proofs();
    Looks();
    Real();
    Time();
    std::printf("\n%d checks, %d failed, %d skipped\n", g_counted, g_failed, g_skipped);
    return g_failed;
}

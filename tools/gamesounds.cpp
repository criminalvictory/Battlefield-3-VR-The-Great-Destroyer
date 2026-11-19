// gamesounds: reads the game's own sound assets out of its data files, nothing recorded: the catalogues (cas.cat), the
// superbundles (.toc/.sb), the EBX assets in the CAS archives, each SoundWaveAsset's name and chunks and which weapon
// names them. It only reads the game's folder, at background priority, one read at a time, and writes under build\gamesounds
// (and with --sounds the clips it names into build\sounds).
//
//   gamesounds                  index everything: index.txt, weapons.txt, guns.txt, refs.txt, types.txt, codecs.txt, missing.txt, tocs.txt
//   gamesounds --all            the same, reading every EBX in the game instead of the sound and weapon names
//   gamesounds --dump <text>    also write ebx\<name>.txt, a readable dump of every asset whose name holds the text
//   gamesounds --raw <text>     also write raw\<name>.sps, the whole audio chunks of the waves whose name holds the text, and raw\walk.txt
//   gamesounds --wav <text>     decode the waves whose name holds the text to wav\<name flattened>.wav, 16-bit PCM (more than one --wav is fine);
//                               a wave with several variations writes one file each, _v0, _v1...; a variation's segments are played in order
//   gamesounds --reloads        decode the reload waves of every gun of the first mission (the list below, by exact name)
//   gamesounds --check <text>   decode and measure as --wav does, without writing any wave: wav\check.txt
//   gamesounds --study          final\study.txt: every 10 ms level of the reload waves, each rise, what of a long reload is also in the short one, where the liked clips came from
//   gamesounds --trace          final\traces.txt: how each gun's lever and magazine bones move in the survey recordings
//   gamesounds --final          final\<part>_<shape>.wav and final\report.txt, from the table g_cuts: the parts of each gun's reload at the liked clips' loudness
//   gamesounds --gain <dB>      with --final, another gain than kFinalGainDb (never with --sounds: build\sounds holds every clip at the table's gain)
//   gamesounds --sounds <name>  also write the clip of g_cuts that the mod plays by that name into build\sounds\<name>.wav, the folder beside this
//                               program (the name is the cut's part and shape, or its part alone for a "generic" one: take, stow); nothing else
//                               there is touched, and a file already holding the same bytes is not written again (more than one --sounds is fine;
//                               exit code 2 when one of them fails)
//   gamesounds --verify         final\verify.txt: each clip's cut correlated with build\survey\lever_kh2002.wav and lever_m240.wav (identity only; nothing of them goes into a clip)
//   gamesounds --mod            every clip the mod plays (each row of g_cuts and g_modClips) into the sounds folder beside this program, and nothing else
//                               written anywhere: a player's first start makes the mod's sounds from their own copy of the game (tools\launch_vr.ps1);
//                               exit code 2 when a clip failed, 3 when the game's folder wasn't found
//   gamesounds --missing        as --mod, but only the clips that aren't in the sounds folder yet (a player who updates the mod in place has the
//                               older version's clips: only a new gun's are made, none of those there is made again or touched); with none missing
//                               it says so and ends at once, the game's files not read at all. The same exit codes
//   gamesounds --list           the name of every clip the mod plays (--mod's list), one a line, and nothing else: the game isn't looked for or read
//   gamesounds --fit <file.wav> <wave>   where in the game's wave (a name as in g_cuts) a clip already cut lies: its place, the correlation and the
//                               gain from the wave to the clip (more than one --fit is fine; nothing written)
//   gamesounds --game <folder>  the game's folder (default: found as the EA app and Steam installed it: the registry, then Steam's libraries)
// Every decode prints its numbers, also into wav\numbers.txt: see Measure and the columns written at the top of that file.
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

// ---------------------------------------------------------------------------------------------------------- small things
struct Sha {
    u8 b[20]{};
    bool operator==(const Sha& o) const { return memcmp(b, o.b, 20) == 0; }
    bool operator<(const Sha& o) const { return memcmp(b, o.b, 20) < 0; }
};
struct ShaHash {
    size_t operator()(const Sha& s) const { u32 v; memcpy(&v, s.b, 4); return v; }
};
struct Guid {
    u8 b[16]{};
    bool operator==(const Guid& o) const { return memcmp(b, o.b, 16) == 0; }
    bool operator<(const Guid& o) const { return memcmp(b, o.b, 16) < 0; }
    bool zero() const { for (u8 c : b) if (c) return false; return true; }
};
struct GuidHash {
    size_t operator()(const Guid& s) const { u32 v; memcpy(&v, s.b + 4, 4); return v; }
};
static std::string hexOf(const u8* p, size_t n, bool spaces = false) {
    static const char* h = "0123456789abcdef";
    std::string s;
    for (size_t i = 0; i < n; ++i) { s += h[p[i] >> 4]; s += h[p[i] & 15]; if (spaces && i + 1 < n) s += ' '; }
    return s;
}
// Printed the way the game's tools print them: the first three groups read little-endian.
static std::string guidText(const Guid& g) {
    static const int order[16] = {3, 2, 1, 0, 5, 4, 7, 6, 8, 9, 10, 11, 12, 13, 14, 15};
    static const char* h = "0123456789abcdef";
    std::string s;
    for (int i = 0; i < 16; ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) s += '-';
        u8 c = g.b[order[i]];
        s += h[c >> 4]; s += h[c & 15];
    }
    return s;
}
static std::string lower(std::string s) {
    for (char& c : s) if (c >= 'A' && c <= 'Z') c = char(c + 32);
    return s;
}
static bool holds(const std::string& s, const char* what) { return s.find(what) != std::string::npos; }
// A path to print, in UTF-8 (path::string() throws on a letter the system's code page lacks: a player's folder may have one).
static std::string shown(const std::filesystem::path& p) {
    const std::wstring w = p.wstring();
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), int(w.size()), nullptr, 0, nullptr, nullptr);
    std::string s(size_t(n > 0 ? n : 0), '\0');
    if (n > 0) WideCharToMultiByte(CP_UTF8, 0, w.data(), int(w.size()), s.data(), n, nullptr, nullptr);
    return s;
}
static bool isAlnum(char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'); }
// The word on its own: "m9" in "weapons/m9/m9_fire", not in "m93r" or "am9".
static bool hasWord(const std::string& low, const std::string& word) {
    for (size_t at = low.find(word); at != std::string::npos; at = low.find(word, at + 1)) {
        bool before = at == 0 || !isAlnum(low[at - 1]);
        bool after = at + word.size() >= low.size() || !isAlnum(low[at + word.size()]);
        if (before && after) return true;
    }
    return false;
}
static u32 be32(const u8* p) { return (u32(p[0]) << 24) | (u32(p[1]) << 16) | (u32(p[2]) << 8) | p[3]; }
static u16 be16(const u8* p) { return u16((p[0] << 8) | p[1]); }
static u32 le32(const u8* p) { u32 v; memcpy(&v, p, 4); return v; }
static u16 le16(const u8* p) { u16 v; memcpy(&v, p, 2); return v; }

// ------------------------------------------------------------------------------------------------------- reading files
static std::vector<u8> readAll(const fs::path& p) {
    HANDLE h = CreateFileW(p.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                           FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (h == INVALID_HANDLE_VALUE) throw std::runtime_error("cannot open " + p.string());
    LARGE_INTEGER sz{};
    GetFileSizeEx(h, &sz);
    std::vector<u8> v(size_t(sz.QuadPart));
    size_t got = 0;
    while (got < v.size()) {
        DWORD n = 0, want = DWORD(std::min<size_t>(v.size() - got, 1u << 20));
        if (!ReadFile(h, v.data() + got, want, &n, nullptr) || !n) break;
        got += n;
    }
    CloseHandle(h);
    v.resize(got);
    return v;
}
static bool readFilePart(const fs::path& p, u32 offset, u64 want, std::vector<u8>& out) {
    HANDLE h = CreateFileW(p.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER at; at.QuadPart = offset;
    out.resize(size_t(want));
    DWORD n = 0;
    bool ok = SetFilePointerEx(h, at, nullptr, FILE_BEGIN) && ReadFile(h, out.data(), DWORD(want), &n, nullptr);
    CloseHandle(h);
    out.resize(ok ? n : 0);
    return ok && n;
}
// The tocs and the catalogues are lightly scrambled: a 0x22C-byte header holding a 257-byte table.
static std::vector<u8> unscramble(std::vector<u8> v) {
    if (v.size() < 0x22C || v[0] != 0x00 || v[1] != 0xD1 || v[2] != 0xCE || (v[3] != 0x00 && v[3] != 0x01)) return v;
    const u8* table = v.data() + 0x128;
    std::vector<u8> o(v.size() - 0x22C);
    for (size_t i = 0; i < o.size(); ++i) o[i] = u8(v[0x22C + i] ^ table[i % 257] ^ 0x7B);
    return o;
}

// ------------------------------------------------------------------------------------------- the tagged dictionary format
struct Node {
    u8 type = 0;
    const char* name = "";
    const u8* data = nullptr;
    u32 len = 0;
    std::vector<Node> kids;
    const Node* get(const char* key) const {
        for (const Node& k : kids) if (!strcmp(k.name, key)) return &k;
        return nullptr;
    }
    long long asInt() const {
        if (type == 8) { int v; memcpy(&v, data, 4); return v; }
        if (type == 9 || type == 5) { long long v; memcpy(&v, data, 8); return v; }
        if (type == 6) return data[0];
        return 0;
    }
    std::string str() const {
        if (type != 7 || !data) return {};
        u32 n = len;
        while (n && data[n - 1] == 0) --n;
        return std::string((const char*)data, n);
    }
};
static u32 leb128(const u8*& p, const u8* e) {
    u32 v = 0; int sh = 0;
    while (p < e) {
        u8 c = *p++;
        v |= u32(c & 0x7F) << sh;
        if (!(c & 0x80)) return v;
        sh += 7;
        if (sh > 35) break;
    }
    throw std::runtime_error("bad length");
}
// Returns false for the 00 that ends a list or dictionary.
static bool readNode(const u8*& p, const u8* e, Node& n) {
    if (p >= e) throw std::runtime_error("past the end");
    u8 tag = *p++;
    if (!tag) return false;
    n.type = tag & 0x1F;
    if (!(tag & 0x80)) {
        n.name = (const char*)p;
        while (p < e && *p) ++p;
        if (p >= e) throw std::runtime_error("name past the end");
        ++p;
    }
    auto fixed = [&](u32 size) {
        if (p + size > e) throw std::runtime_error("value past the end");
        n.data = p; n.len = size; p += size;
    };
    switch (n.type) {
    case 1:
    case 2: {
        u32 size = leb128(p, e);
        const u8* end = p + size;
        if (end > e) throw std::runtime_error("list past the end");
        while (p < end) {
            Node k;
            if (!readNode(p, end, k)) break;
            n.kids.push_back(std::move(k));
        }
        p = end;
        break;
    }
    case 5: fixed(8); break;
    case 6: fixed(1); break;
    case 7:
    case 0x13: { u32 size = leb128(p, e); fixed(size); break; }
    case 8: fixed(4); break;
    case 9: fixed(8); break;
    case 0xF: fixed(16); break;
    case 0x10: fixed(20); break;
    default: {
        char m[64];
        sprintf_s(m, "unknown type %02x", tag);
        throw std::runtime_error(m);
    }
    }
    return true;
}

// ----------------------------------------------------------------------------------------------------------- inflate
// A plain DEFLATE reader. A cut-short input (we read only the head of a chunk) ends it quietly with what came out so far.
struct Inflater {
    const u8* in; size_t n; size_t pos = 0; u32 buf = 0; int cnt = 0;
    std::vector<u8>& out; size_t limit;
    struct EndOfInput {};
    struct Huff { short count[16]; short symbol[288]; };
    Inflater(const u8* p, size_t size, std::vector<u8>& o, size_t lim) : in(p), n(size), out(o), limit(lim) {}
    int bits(int need) {
        u32 v = buf;
        while (cnt < need) {
            if (pos >= n) throw EndOfInput{};
            v |= u32(in[pos++]) << cnt;
            cnt += 8;
        }
        buf = need == 32 ? 0 : v >> need;
        cnt -= need;
        return int(v & ((1u << need) - 1));
    }
    int decode(const Huff& h) {
        int code = 0, first = 0, index = 0;
        for (int len = 1; len <= 15; ++len) {
            code |= bits(1);
            int count = h.count[len];
            if (code - count < first) return h.symbol[index + (code - first)];
            index += count; first += count; first <<= 1; code <<= 1;
        }
        throw std::runtime_error("bad huffman code");
    }
    static void build(Huff& h, const short* length, int count) {
        short offs[16];
        for (int i = 0; i <= 15; ++i) h.count[i] = 0;
        for (int i = 0; i < count; ++i) h.count[length[i]]++;
        offs[1] = 0;
        for (int i = 1; i < 15; ++i) offs[i + 1] = short(offs[i] + h.count[i]);
        for (int i = 0; i < count; ++i) if (length[i]) h.symbol[offs[length[i]]++] = short(i);
    }
    bool codes(const Huff& lit, const Huff& dist) {
        static const short lens[29] = {3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
        static const short lext[29] = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
        static const short dists[30] = {1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
        static const short dext[30] = {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};
        for (;;) {
            if (out.size() >= limit) return false;
            int sym = decode(lit);
            if (sym < 256) out.push_back(u8(sym));
            else if (sym == 256) return true;
            else {
                sym -= 257;
                if (sym >= 29) throw std::runtime_error("bad length code");
                int len = lens[sym] + bits(lext[sym]);
                int ds = decode(dist);
                if (ds >= 30) throw std::runtime_error("bad distance code");
                size_t d = size_t(dists[ds]) + bits(dext[ds]);
                if (d > out.size()) throw std::runtime_error("distance too far");
                for (int i = 0; i < len; ++i) out.push_back(out[out.size() - d]);
            }
        }
    }
    void run() {
        try {
            for (;;) {
                int last = bits(1), type = bits(2);
                if (type == 0) {
                    buf = 0; cnt = 0;
                    if (pos + 4 > n) throw EndOfInput{};
                    size_t len = size_t(in[pos]) | (size_t(in[pos + 1]) << 8);
                    pos += 4;
                    for (size_t i = 0; i < len; ++i) {
                        if (pos >= n) throw EndOfInput{};
                        out.push_back(in[pos++]);
                    }
                } else if (type == 1) {
                    static Huff lit, dist; static bool made = false;
                    if (!made) {
                        short l[320]; int s = 0;
                        for (; s < 144; ++s) l[s] = 8;
                        for (; s < 256; ++s) l[s] = 9;
                        for (; s < 280; ++s) l[s] = 7;
                        for (; s < 288; ++s) l[s] = 8;
                        build(lit, l, 288);
                        for (s = 0; s < 30; ++s) l[s] = 5;
                        build(dist, l, 30);
                        made = true;
                    }
                    if (!codes(lit, dist)) return;
                } else if (type == 2) {
                    static const short order[19] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
                    int nlen = bits(5) + 257, ndist = bits(5) + 1, ncode = bits(4) + 4, idx = 0;
                    short l[320] = {};
                    for (; idx < ncode; ++idx) l[order[idx]] = short(bits(3));
                    for (; idx < 19; ++idx) l[order[idx]] = 0;
                    Huff lens; build(lens, l, 19);
                    idx = 0;
                    while (idx < nlen + ndist) {
                        int sym = decode(lens);
                        if (sym < 16) l[idx++] = short(sym);
                        else {
                            int prev = 0, rep;
                            if (sym == 16) { if (!idx) throw std::runtime_error("no previous length"); prev = l[idx - 1]; rep = 3 + bits(2); }
                            else if (sym == 17) rep = 3 + bits(3);
                            else rep = 11 + bits(7);
                            if (idx + rep > nlen + ndist) throw std::runtime_error("too many lengths");
                            while (rep--) l[idx++] = short(prev);
                        }
                    }
                    Huff lit, dist;
                    build(lit, l, nlen);
                    build(dist, l + nlen, ndist);
                    if (!codes(lit, dist)) return;
                } else throw std::runtime_error("bad deflate block");
                if (last) return;
            }
        } catch (const EndOfInput&) {}
    }
};

// ----------------------------------------------------------------------------------------------- the catalogue and CAS
struct Root {
    fs::path data;
    std::string label;
    std::map<u32, HANDLE> cas;
    HANDLE casFile(u32 no) {
        auto it = cas.find(no);
        if (it != cas.end()) return it->second;
        char nm[32];
        sprintf_s(nm, "cas_%02u.cas", no);
        HANDLE h = CreateFileW((data / nm).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr);
        cas[no] = h;
        return h;
    }
};
struct CatEntry { u32 root, cas, offset, size; };
static std::vector<Root> g_roots;
static std::unordered_map<Sha, CatEntry, ShaHash> g_cat;

static size_t loadCatalogue(u32 rootIdx) {
    fs::path p = g_roots[rootIdx].data / "cas.cat";
    if (!fs::exists(p)) return 0;
    std::vector<u8> v = unscramble(readAll(p));
    if (v.size() < 16 || memcmp(v.data(), "NyanNyanNyanNyan", 16)) {
        printf("%s: does not start with the expected text: %s\n", p.string().c_str(), hexOf(v.data(), std::min<size_t>(v.size(), 32), true).c_str());
        return 0;
    }
    size_t count = 0;
    for (size_t at = 16; at + 32 <= v.size(); at += 32) {
        Sha s; memcpy(s.b, v.data() + at, 20);
        CatEntry e{rootIdx, le32(v.data() + at + 28), le32(v.data() + at + 20), le32(v.data() + at + 24)};
        g_cat.emplace(s, e);
        ++count;
    }
    return count;
}
static bool readRaw(const CatEntry& e, u32 want, std::vector<u8>& out) {
    HANDLE h = g_roots[e.root].casFile(e.cas);
    if (h == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER at; at.QuadPart = e.offset;
    if (!SetFilePointerEx(h, at, nullptr, FILE_BEGIN)) return false;
    out.resize(std::min(e.size, want));
    size_t got = 0;
    while (got < out.size()) {
        DWORD n = 0;
        if (!ReadFile(h, out.data() + got, DWORD(out.size() - got), &n, nullptr) || !n) break;
        got += n;
    }
    out.resize(got);
    return got > 0;
}
// A payload is a row of blocks: big-endian u32 size out, u16 type, u16 size stored.
static std::map<u32, u64> g_blockTypes;
static void unblock(const std::vector<u8>& raw, std::vector<u8>& out, size_t limit) {
    size_t at = 0;
    while (at + 8 <= raw.size() && out.size() < limit) {
        u32 full = be32(&raw[at]);
        u16 type = be16(&raw[at + 4]);
        u32 stored = be16(&raw[at + 6]);
        at += 8;
        g_blockTypes[type]++;
        if (type == 0x0270) {
            if (at + stored > raw.size()) stored = u32(raw.size() - at);
            if (stored > 2) {
                Inflater inf(raw.data() + at + 2, stored - 2, out, std::min(limit, out.size() + full));
                inf.run();
            }
            at += stored;
        } else if (type == 0x0070 || type == 0x0071 || type == 0x0000) {
            u32 take = stored ? stored : full;
            if (at + take > raw.size()) take = u32(raw.size() - at);
            out.insert(out.end(), raw.begin() + at, raw.begin() + at + take);
            at += take;
        } else {
            char m[64];
            sprintf_s(m, "block type %04x", type);
            throw std::runtime_error(m);
        }
    }
}

// Some payloads stand bare in the archive: an EBX's own magic at the start says so.
static void payloadOf(const std::vector<u8>& raw, std::vector<u8>& out, size_t limit) {
    if (raw.size() >= 4 && raw[0] == 0xCE && raw[1] == 0xD1 && raw[2] == 0xB2 && raw[3] == 0x0F) { out = raw; return; }
    unblock(raw, out, limit);
}

// ------------------------------------------------------------------------------------------------------ the superbundles
struct EbxRef {
    std::string name;
    Sha sha;
    u32 size = 0;
    bool patched = false;
    std::vector<u32> bundles;
};
struct ChunkRef {
    Sha sha;
    bool patched = false;
    long long logical = 0;
    bool inToc = false;
    fs::path sb;           // set when the chunk has no sha1: it lies in this sb
    u32 sbOff = 0, sbSize = 0;
    std::vector<u32> bundles;
};
static std::vector<std::string> g_bundleNames;
static std::map<std::string, EbxRef> g_ebx;
static std::unordered_map<Guid, ChunkRef, GuidHash> g_chunks;
struct TocStat { std::string key, root; size_t bundles = 0, chunks = 0; long long cas = -1; size_t baseFlag = 0, deltaFlag = 0, noSha = 0; size_t sbBundles = 0; std::string note; };
static std::vector<TocStat> g_tocStats;
static size_t g_noncasChunks = 0, g_ebxNoSha = 0, g_nameConflicts = 0;

static void readBundle(const Node& b, const std::string& where, bool patched, std::set<std::string>& seen) {
    std::string name;
    if (const Node* p = b.get("path")) name = p->str();
    if (name.empty()) if (const Node* p = b.get("id")) name = p->str();
    std::string low = lower(name);
    seen.insert(low);
    u32 idx = u32(g_bundleNames.size());
    g_bundleNames.push_back(where + " " + name);
    if (const Node* list = b.get("ebx")) {
        for (const Node& k : list->kids) {
            const Node* nm = k.get("name");
            const Node* sh = k.get("sha1");
            if (!nm) continue;
            if (!sh || sh->len != 20) { ++g_ebxNoSha; continue; }
            std::string lowName = lower(nm->str());
            auto it = g_ebx.find(lowName);
            Sha sha; memcpy(sha.b, sh->data, 20);
            if (it == g_ebx.end()) {
                EbxRef r;
                r.name = nm->str();
                r.sha = sha;
                r.patched = patched;
                if (const Node* s = k.get("originalSize")) r.size = u32(s->asInt());
                else if (const Node* s2 = k.get("size")) r.size = u32(s2->asInt());
                it = g_ebx.emplace(lowName, std::move(r)).first;
            } else if (!(it->second.sha == sha)) {
                ++g_nameConflicts;
            }
            it->second.bundles.push_back(idx);
        }
    }
    if (const Node* list = b.get("chunks")) {
        for (const Node& k : list->kids) {
            const Node* id = k.get("id");
            const Node* sh = k.get("sha1");
            if (!id || id->len != 16) continue;
            if (!sh || sh->len != 20) { ++g_noncasChunks; continue; }
            Guid g; memcpy(g.b, id->data, 16);
            ChunkRef r;
            memcpy(r.sha.b, sh->data, 20);
            r.patched = patched;
            if (const Node* s = k.get("logicalSize")) r.logical = s->asInt();
            auto it = g_chunks.emplace(g, r).first;
            it->second.bundles.push_back(idx);
        }
    }
}
// One superbundle in one root. skip holds the bundles already taken from a newer root.
static void readSuperbundle(const fs::path& toc, const std::string& key, u32 rootIdx, std::set<std::string>& seen) {
    TocStat st;
    st.key = key;
    st.root = g_roots[rootIdx].label;
    bool patched = rootIdx == 0 && g_roots.size() > 1;
    std::vector<u8> tv = unscramble(readAll(toc));
    if (tv.empty()) { st.note = "empty toc"; g_tocStats.push_back(st); return; }
    const u8* p = tv.data();
    Node root;
    try {
        readNode(p, tv.data() + tv.size(), root);
    } catch (const std::exception& e) {
        st.note = std::string("toc unreadable: ") + e.what() + " first bytes " + hexOf(tv.data(), std::min<size_t>(tv.size(), 16), true);
        g_tocStats.push_back(st);
        return;
    }
    if (const Node* c = root.get("cas")) st.cas = c->asInt();
    if (const Node* list = root.get("chunks")) {
        st.chunks = list->kids.size();
        for (const Node& k : list->kids) {
            const Node* id = k.get("id");
            const Node* sh = k.get("sha1");
            if (!id || id->len != 16) continue;
            Guid g; memcpy(g.b, id->data, 16);
            ChunkRef r;
            r.patched = patched;
            r.inToc = true;
            if (!sh || sh->len != 20) {
                ++st.noSha;
                const Node* off = k.get("offset");
                const Node* size = k.get("size");
                if (!off || !size) { ++g_noncasChunks; continue; }
                r.sb = toc; r.sb.replace_extension(".sb");
                r.sbOff = u32(off->asInt());
                r.sbSize = u32(size->asInt());
                g_chunks.emplace(g, std::move(r));
                continue;
            }
            memcpy(r.sha.b, sh->data, 20);
            g_chunks.emplace(g, r);
        }
    }
    const Node* bundles = root.get("bundles");
    if (bundles) {
        st.bundles = bundles->kids.size();
        for (const Node& k : bundles->kids) {
            if (k.get("base")) ++st.baseFlag;
            if (k.get("delta")) ++st.deltaFlag;
        }
    }
    fs::path sbPath = toc; sbPath.replace_extension(".sb");
    if (bundles && !bundles->kids.empty() && fs::exists(sbPath)) {
        u64 sbSize = fs::file_size(sbPath);
        if (sbSize > (64u << 20)) {
            st.note = "sb too large, left out (a DLC chunk file)";
        } else {
            std::vector<u8> sb = readAll(sbPath);
            std::string where = key;
            try {
                // A toc entry says where each bundle sits in the sb; some sb files wrap them all in one dictionary.
                for (const Node& k : bundles->kids) {
                    if (k.get("base")) continue;
                    const Node* off = k.get("offset");
                    const Node* id = k.get("id");
                    if (!off) continue;
                    long long at = off->asInt();
                    if (at < 0 || size_t(at) >= sb.size()) continue;
                    std::string idLow = id ? lower(id->str()) : std::string();
                    if (seen.count(idLow)) continue;
                    const u8* q = sb.data() + at;
                    Node b;
                    u8 first = *q;
                    if ((first & 0x1F) != 2) { st.note = "bundle is not a dictionary: starts " + hexOf(q, 16, true); continue; }
                    if (!readNode(q, sb.data() + sb.size(), b)) continue;
                    ++st.sbBundles;
                    readBundle(b, where, patched, seen);
                    seen.insert(idLow);
                }
            } catch (const std::exception& e) {
                st.note += std::string("sb problem: ") + e.what();
            }
            // Anything the toc did not point at (the sb's own list).
            if (!st.sbBundles && !sb.empty() && (sb[0] & 0x1F) == 2) {
                try {
                    const u8* q = sb.data();
                    Node whole;
                    readNode(q, sb.data() + sb.size(), whole);
                    if (const Node* list = whole.get("bundles")) {
                        for (const Node& b : list->kids) {
                            std::string nm = b.get("path") ? lower(b.get("path")->str()) : std::string();
                            if (seen.count(nm)) continue;
                            readBundle(b, where, patched, seen);
                            ++st.sbBundles;
                        }
                    }
                } catch (const std::exception& e) {
                    st.note += std::string(" sb list problem: ") + e.what();
                }
            }
        }
    }
    g_tocStats.push_back(st);
}

// --------------------------------------------------------------------------------------------------------------- EBX
struct FieldDesc { u32 nameHash; u16 type, ref; u32 offset, secondary; };
struct ComplexDesc { u32 nameHash, fieldStart; u8 numField, alignment; u16 type, size, secondary; };
struct ArrayRep { u32 offset, reps, complex; };
struct Instance { u32 complex; size_t offset; Guid guid; bool hasGuid; };

static u32 keyHash(const char* s) {
    u32 h = 5381;
    for (; *s; ++s) h = (h * 33) ^ u32(u8(*s));
    return h;
}
enum { T_VOID = 0, T_STRUCT = 2, T_CLASS = 3, T_ARRAY = 4, T_STRING = 7, T_ENUM = 8, T_FILEREF = 9, T_BOOL = 0xA, T_I8 = 0xB, T_U8 = 0xC, T_I16 = 0xD, T_U16 = 0xE,
       T_I32 = 0xF, T_U32 = 0x10, T_I64 = 0x11, T_U64 = 0x12, T_F32 = 0x13, T_F64 = 0x14, T_GUID = 0x15, T_SHA1 = 0x16, T_RESREF = 0x17 };

struct Ebx {
    const u8* d = nullptr; size_t n = 0;
    u32 absStr = 0, lenStrEof = 0, numGuid = 0, lenString = 0, numArr = 0, lenPayload = 0;
    u32 numInst = 0, numComplex = 0, numField = 0, lenName = 0;
    size_t endOfInstances = 0;
    Guid fileGuid, primary;
    std::vector<std::pair<Guid, Guid>> ext;
    std::unordered_map<u32, std::string> keys;
    std::vector<FieldDesc> fields;
    std::vector<ComplexDesc> complexes;
    std::vector<std::pair<u32, u32>> instReps;
    std::vector<ArrayRep> arrays;
    std::vector<Instance> instances;
    size_t payload = 0, arrayStart = 0;

    const char* key(u32 h) const { auto it = keys.find(h); return it == keys.end() ? "?" : it->second.c_str(); }
    const char* typeName(u32 c) const { return c < complexes.size() ? key(complexes[c].nameHash) : "?"; }
    u32 getU32(size_t at) const { return at + 4 <= n ? le32(d + at) : 0; }

    // This game's form: eleven u32 after the magic, then the file's GUID and its primary instance's GUID.
    bool parse(const u8* data, size_t size) {
        d = data; n = size;
        if (n < 80 || d[0] != 0xCE || d[1] != 0xD1 || d[2] != 0xB2 || d[3] != 0x0F) return false;
        absStr = le32(d + 4); lenStrEof = le32(d + 8); numGuid = le32(d + 12);
        numInst = le32(d + 20); numComplex = le32(d + 24); numField = le32(d + 28); lenName = le32(d + 32);
        lenString = le32(d + 36); numArr = le32(d + 40); lenPayload = le32(d + 44);
        memcpy(fileGuid.b, d + 48, 16);
        memcpy(primary.b, d + 64, 16);
        size_t at = 80;
        if (at + size_t(numGuid) * 32 > n) return false;
        ext.resize(numGuid);
        for (u32 i = 0; i < numGuid; ++i, at += 32) { memcpy(ext[i].first.b, d + at, 16); memcpy(ext[i].second.b, d + at + 16, 16); }
        if (at + lenName > n) return false;
        for (size_t p = at, e = at + lenName; p < e;) {
            size_t q = p;
            while (q < e && d[q]) ++q;
            if (q > p) {
                std::string s((const char*)d + p, q - p);
                keys[keyHash(s.c_str())] = s;
            }
            p = q + 1;
        }
        at += lenName;
        if (at + size_t(numField) * 16 + size_t(numComplex) * 16 + size_t(numInst) * 12 > n) return false;
        fields.resize(numField);
        for (u32 i = 0; i < numField; ++i, at += 16)
            fields[i] = {le32(d + at), le16(d + at + 4), le16(d + at + 6), le32(d + at + 8), le32(d + at + 12)};
        complexes.resize(numComplex);
        for (u32 i = 0; i < numComplex; ++i, at += 16)
            complexes[i] = {le32(d + at), le32(d + at + 4), d[at + 8], d[at + 9], le16(d + at + 10), le16(d + at + 12), le16(d + at + 14)};
        instReps.resize(numInst);
        for (u32 i = 0; i < numInst; ++i, at += 12) instReps[i] = {le32(d + at + 4), le32(d + at + 8)};  // zero, repetitions, type
        while (at % 16) ++at;
        if (at + size_t(numArr) * 12 > n) return false;
        arrays.resize(numArr);
        for (u32 i = 0; i < numArr; ++i, at += 12) arrays[i] = {le32(d + at), le32(d + at + 4), le32(d + at + 8)};
        payload = size_t(absStr) + lenString;
        arrayStart = payload + lenPayload;
        if (payload > n) return false;
        // Every instance has its GUID in front.
        size_t pos = payload;
        for (u32 r = 0; r < numInst; ++r) {
            u32 reps = instReps[r].first, complex = instReps[r].second;
            if (complex >= numComplex) return false;
            const ComplexDesc& c = complexes[complex];
            for (u32 i = 0; i < reps; ++i) {
                Instance inst{complex, 0, {}, true};
                if (pos + 16 > n) return false;
                memcpy(inst.guid.b, d + pos, 16);
                pos += 16;
                inst.offset = pos;
                instances.push_back(inst);
                pos += c.size;
                if (pos > n) return false;
            }
        }
        endOfInstances = pos;
        return true;
    }
    static u32 shiftOf(const ComplexDesc&, bool) { return 0; }
    // Type name of a field's array/struct, for the rest.
    u32 fieldSize(const FieldDesc& f) const {
        switch ((f.type >> 4) & 0x1F) {
        case T_STRUCT: return f.ref < complexes.size() ? complexes[f.ref].size : 0;
        case T_BOOL: case T_I8: case T_U8: return 1;
        case T_I16: case T_U16: return 2;
        case T_I64: case T_U64: case T_F64: case T_RESREF: return 8;
        case T_GUID: return 16;
        case T_SHA1: return 20;
        default: return 4;
        }
    }
    // Finds a field by name, through the base classes. at gives its place in the file.
    const FieldDesc* find(u32 complex, size_t base, bool isInstance, u32 hash, size_t& at) const {
        if (complex >= complexes.size()) return nullptr;
        const ComplexDesc& c = complexes[complex];
        u32 shift = shiftOf(c, isInstance);
        for (u32 i = c.fieldStart; i < u32(c.fieldStart) + c.numField && i < fields.size(); ++i) {
            const FieldDesc& f = fields[i];
            size_t here = base + f.offset - shift;
            if (f.nameHash == hash) { at = here; return &f; }
            if (((f.type >> 4) & 0x1F) == T_VOID) {
                if (const FieldDesc* r = find(f.ref, here, false, hash, at)) return r;
            }
        }
        return nullptr;
    }
    std::string stringAt(u32 off) const {
        if (off == 0xFFFFFFFF) return {};
        size_t at = size_t(absStr) + off;
        if (at >= n) return {};
        const u8* e = (const u8*)memchr(d + at, 0, n - at);
        return std::string((const char*)d + at, e ? size_t(e - (d + at)) : n - at);
    }
    std::string stringField(const Instance& in, const char* name) const {
        size_t at = 0;
        const FieldDesc* f = find(in.complex, in.offset, true, keyHash(name), at);
        if (!f || ((f->type >> 4) & 0x1F) != T_STRING) return {};
        return stringAt(getU32(at));
    }
    // Every element's place in an array field.
    bool arrayOf(const FieldDesc& f, size_t at, const FieldDesc*& member, size_t& first, u32& reps, u32& stride) const {
        u32 idx = getU32(at);
        if (idx >= arrays.size() || f.ref >= complexes.size()) return false;
        const ComplexDesc& ac = complexes[f.ref];
        if (!ac.numField || ac.fieldStart >= fields.size()) return false;
        member = &fields[ac.fieldStart];
        first = arrayStart + arrays[idx].offset;
        reps = arrays[idx].reps;
        stride = fieldSize(*member);
        return true;
    }
};

// A readable dump of one asset, in the shape the game's own tools would print it.
static std::unordered_map<Guid, std::string, GuidHash> g_fileName;
static void dumpValue(std::ostream& o, const Ebx& e, const FieldDesc& f, size_t at, int ind);
static void dumpComplex(std::ostream& o, const Ebx& e, u32 complex, size_t base, int ind, bool isInstance) {
    if (complex >= e.complexes.size() || ind > 24) return;
    const ComplexDesc& c = e.complexes[complex];
    u32 shift = Ebx::shiftOf(c, isInstance);
    for (u32 i = c.fieldStart; i < u32(c.fieldStart) + c.numField && i < e.fields.size(); ++i) {
        const FieldDesc& f = e.fields[i];
        size_t here = base + f.offset - shift;
        if (((f.type >> 4) & 0x1F) == T_VOID) { dumpComplex(o, e, f.ref, here, ind, false); continue; }
        o << std::string(size_t(ind) * 2, ' ') << e.key(f.nameHash) << ": ";
        dumpValue(o, e, f, here, ind);
    }
}
static void dumpValue(std::ostream& o, const Ebx& e, const FieldDesc& f, size_t at, int ind) {
    u32 t = (f.type >> 4) & 0x1F;
    if (at + e.fieldSize(f) > e.n) { o << "(past the end)\n"; return; }
    const u8* p = e.d + at;
    char m[96];
    switch (t) {
    case T_STRUCT: o << e.typeName(f.ref) << "\n"; dumpComplex(o, e, f.ref, at, ind + 1, false); return;
    case T_CLASS: {
        u32 v = le32(p);
        if (!v) o << "null\n";
        else if (v & 0x80000000u) {
            u32 x = v & 0x7FFFFFFFu;
            if (x < e.ext.size()) {
                auto it = g_fileName.find(e.ext[x].first);
                o << "-> " << (it != g_fileName.end() ? it->second : guidText(e.ext[x].first)) << " / " << guidText(e.ext[x].second) << "\n";
            } else o << "-> bad external " << x << "\n";
        } else {
            if (v - 1 < e.instances.size()) o << "-> #" << (v - 1) << " " << e.typeName(e.instances[v - 1].complex) << "\n";
            else o << "-> bad instance " << v << "\n";
        }
        return;
    }
    case T_ARRAY: {
        const FieldDesc* mem; size_t first; u32 reps, stride;
        if (!e.arrayOf(f, at, mem, first, reps, stride)) { o << "(bad array)\n"; return; }
        o << "[" << reps << "]\n";
        for (u32 i = 0; i < reps && i < 4096; ++i) {
            o << std::string(size_t(ind + 1) * 2, ' ') << "[" << i << "] ";
            dumpValue(o, e, *mem, first + size_t(i) * stride, ind + 1);
        }
        return;
    }
    case T_STRING: case T_FILEREF: o << '"' << e.stringAt(le32(p)) << "\"\n"; return;
    case T_ENUM: {
        int v = int(le32(p));
        const char* nm = nullptr;
        if (f.ref < e.complexes.size()) {
            const ComplexDesc& c = e.complexes[f.ref];
            for (u32 i = c.fieldStart; i < u32(c.fieldStart) + c.numField && i < e.fields.size(); ++i)
                if (int(e.fields[i].offset) == v) nm = e.key(e.fields[i].nameHash);
        }
        if (nm) o << nm << "\n"; else o << v << "\n";
        return;
    }
    case T_BOOL: o << (p[0] ? "true" : "false") << "\n"; return;
    case T_I8: o << int(int8_t(p[0])) << "\n"; return;
    case T_U8: o << int(p[0]) << "\n"; return;
    case T_I16: o << int(int16_t(le16(p))) << "\n"; return;
    case T_U16: o << le16(p) << "\n"; return;
    case T_I32: o << int(le32(p)) << "\n"; return;
    case T_U32: o << le32(p) << "\n"; return;
    case T_I64: { long long v; memcpy(&v, p, 8); o << v << "\n"; return; }
    case T_U64: { u64 v; memcpy(&v, p, 8); o << v << "\n"; return; }
    case T_F32: { float v; memcpy(&v, p, 4); sprintf_s(m, "%g", double(v)); o << m << "\n"; return; }
    case T_F64: { double v; memcpy(&v, p, 8); sprintf_s(m, "%g", v); o << m << "\n"; return; }
    case T_GUID: { Guid g; memcpy(g.b, p, 16); o << guidText(g) << "\n"; return; }
    case T_SHA1: o << hexOf(p, 20) << "\n"; return;
    case T_RESREF: { u64 v; memcpy(&v, p, 8); sprintf_s(m, "%016llx", (unsigned long long)v); o << m << "\n"; return; }
    default: sprintf_s(m, "(type %04x)", f.type); o << m << "\n"; return;
    }
}
static void dumpFile(std::ostream& o, const Ebx& e, const std::string& name) {
    o << name << "\nfile " << guidText(e.fileGuid) << "  primary " << guidText(e.primary) << "\n";
    for (size_t i = 0; i < e.instances.size(); ++i) {
        const Instance& in = e.instances[i];
        o << "\n#" << i << " " << e.typeName(in.complex);
        if (in.hasGuid) o << " " << guidText(in.guid);
        o << "\n";
        dumpComplex(o, e, in.complex, in.offset, 1, true);
    }
}

// -------------------------------------------------------------------------------------------- what we keep of each asset
struct WaveChunk { Guid id; u32 size = 0; };
struct WaveSegment { u32 samplesOffset = 0, seekOffset = 0; float seconds = 0; };
// One playable take: which chunk, and where in it. A chunk may hold many takes; a seek table may stand before the audio.
struct WaveVariation {
    u32 chunkIndex = 0, seekTablesSize = 0;
    int firstLoop = 0, lastLoop = 0;
    std::vector<WaveSegment> segments;
};
struct Wave {
    std::string instName;
    std::vector<WaveChunk> chunks;
    std::vector<WaveVariation> variations;
    u32 channels = 0;
    bool seekable = false;
};
// A wave that a sound graph plays, and from which node.
struct GraphWave { std::string node; u32 instance = 0; int slot = -1; Guid file; };
struct ParsedFile {
    const EbxRef* ref = nullptr;
    std::string primaryType;
    std::vector<Guid> externals;
    std::vector<Wave> waves;
    std::vector<GraphWave> graph;
    std::map<std::string, u32> types;
};
static std::vector<ParsedFile> g_files;
static std::map<std::string, std::pair<u64, u64>> g_typeCount;  // type -> instances, files
static u64 g_ebxRead = 0, g_ebxBad = 0, g_ebxMissing = 0, g_bytesRead = 0;
static u64 g_ebxShort = 0;
static std::string g_firstBad, g_firstShort;

static const u32 H_WAVE = keyHash("SoundWaveAsset");
static void keepFile(const Ebx& e, const EbxRef& r) {
    ParsedFile pf;
    pf.ref = &r;
    for (auto& x : e.ext) pf.externals.push_back(x.first);
    g_fileName[e.fileGuid] = r.name;
    // Follows a class reference: the external file's GUID, when it points out of this asset.
    auto externalOf = [&](u32 v, Guid& g) {
        if (!(v & 0x80000000u)) return false;
        u32 x = v & 0x7FFFFFFFu;
        if (x >= e.ext.size()) return false;
        g = e.ext[x].first;
        return true;
    };
    for (u32 instIdx = 0; instIdx < e.instances.size(); ++instIdx) {
        const Instance& in = e.instances[instIdx];
        const char* tn = e.typeName(in.complex);
        pf.types[tn]++;
        if (in.hasGuid && in.guid == e.primary) pf.primaryType = tn;
        // The graph nodes that play a wave: a switcher's list, or a sampler's own.
        {
            size_t a = 0;
            if (const FieldDesc* f = e.find(in.complex, in.offset, true, keyHash("Waves"), a); f && ((f->type >> 4) & 0x1F) == T_ARRAY) {
                const FieldDesc* mem; size_t first; u32 reps, stride;
                if (e.arrayOf(*f, a, mem, first, reps, stride) && ((mem->type >> 4) & 0x1F) == T_CLASS)
                    for (u32 i = 0; i < reps; ++i) {
                        Guid g;
                        if (externalOf(e.getU32(first + size_t(i) * stride), g)) pf.graph.push_back({tn, instIdx, int(i), g});
                    }
            }
            if (const FieldDesc* f = e.find(in.complex, in.offset, true, keyHash("Wave"), a); f && ((f->type >> 4) & 0x1F) == T_CLASS) {
                Guid g;
                if (externalOf(e.getU32(a), g)) pf.graph.push_back({tn, instIdx, -1, g});
            }
        }
        if (e.complexes[in.complex].nameHash != H_WAVE) continue;
        Wave w;
        w.instName = e.stringField(in, "Name");
        size_t at = 0;
        if (const FieldDesc* f = e.find(in.complex, in.offset, true, keyHash("Chunks"), at)) {
            const FieldDesc* mem; size_t first; u32 reps, stride;
            if (((f->type >> 4) & 0x1F) == T_ARRAY && e.arrayOf(*f, at, mem, first, reps, stride)) {
                for (u32 i = 0; i < reps; ++i) {
                    size_t el = first + size_t(i) * stride;
                    size_t a = 0;
                    WaveChunk c;
                    if (const FieldDesc* g = e.find(mem->ref, el, false, keyHash("ChunkId"), a)) {
                        if (((g->type >> 4) & 0x1F) == T_GUID && a + 16 <= e.n) memcpy(c.id.b, e.d + a, 16);
                    }
                    if (const FieldDesc* g = e.find(mem->ref, el, false, keyHash("ChunkSize"), a)) { (void)g; c.size = e.getU32(a); }
                    w.chunks.push_back(c);
                }
            }
        }
        if (e.find(in.complex, in.offset, true, keyHash("ChannelCount"), at)) w.channels = at < e.n ? e.d[at] : 0;
        if (e.find(in.complex, in.offset, true, keyHash("Seekable"), at)) w.seekable = at < e.n && e.d[at];
        if (const FieldDesc* f = e.find(in.complex, in.offset, true, keyHash("Variations"), at); f && ((f->type >> 4) & 0x1F) == T_ARRAY) {
            const FieldDesc* mem; size_t first; u32 reps, stride;
            if (e.arrayOf(*f, at, mem, first, reps, stride) && ((mem->type >> 4) & 0x1F) == T_CLASS) {
                for (u32 i = 0; i < reps; ++i) {
                    u32 v = e.getU32(first + size_t(i) * stride);
                    if (!v || (v & 0x80000000u) || v - 1 >= e.instances.size()) continue;
                    const Instance& vi = e.instances[v - 1];
                    WaveVariation wv;
                    size_t a = 0;
                    if (e.find(vi.complex, vi.offset, true, keyHash("ChunkIndex"), a)) wv.chunkIndex = e.getU32(a);
                    if (e.find(vi.complex, vi.offset, true, keyHash("SeekTablesSize"), a)) wv.seekTablesSize = e.getU32(a);
                    // One byte each, side by side.
                    if (e.find(vi.complex, vi.offset, true, keyHash("FirstLoopSegmentIndex"), a) && a < e.n) wv.firstLoop = e.d[a];
                    if (e.find(vi.complex, vi.offset, true, keyHash("LastLoopSegmentIndex"), a) && a < e.n) wv.lastLoop = e.d[a];
                    if (const FieldDesc* s = e.find(vi.complex, vi.offset, true, keyHash("Segments"), a); s && ((s->type >> 4) & 0x1F) == T_ARRAY) {
                        const FieldDesc* sm; size_t sf; u32 sr, ss;
                        if (e.arrayOf(*s, a, sm, sf, sr, ss))
                            for (u32 k = 0; k < sr; ++k) {
                                size_t el = sf + size_t(k) * ss, b = 0;
                                WaveSegment seg;
                                if (e.find(sm->ref, el, false, keyHash("SamplesOffset"), b)) seg.samplesOffset = e.getU32(b);
                                if (e.find(sm->ref, el, false, keyHash("SeekTableOffset"), b)) seg.seekOffset = e.getU32(b);
                                if (e.find(sm->ref, el, false, keyHash("SegmentLength"), b)) { u32 raw = e.getU32(b); memcpy(&seg.seconds, &raw, 4); }
                                wv.segments.push_back(seg);
                            }
                    }
                    w.variations.push_back(std::move(wv));
                }
            }
        }
        pf.waves.push_back(std::move(w));
    }
    for (auto& t : pf.types) { auto& c = g_typeCount[t.first]; c.first += t.second; c.second += 1; }
    g_files.push_back(std::move(pf));
}

// ---------------------------------------------------------------------------------------------------------- the guns
struct Gun { const char* label; std::vector<std::string> words; };
static const std::vector<Gun> g_guns = {
    {"AN94", {"an94", "an-94", "an94a", "an94abakan"}},
    {"RPK", {"rpk", "rpk74", "rpk74m", "rpk-74m"}},
    {"AEK971", {"aek971", "aek", "aek-971"}},
    {"AK74M", {"ak74m", "ak74", "ak-74m"}},
    {"G3A3", {"g3a3", "g3", "g3a4"}},
    {"KH2002", {"kh2002", "khaybar"}},
    {"M16A4", {"m16a4", "m16", "m16a3"}},
    {"M4A1", {"m4a1", "m4"}},
    {"M240", {"m240", "m240b"}},
    {"M416", {"m416", "hk416"}},
    {"FIM92 Stinger", {"fim92", "fim92a", "stinger", "fim-92"}},
    {"SMAW", {"smaw"}},
    {"M9", {"m9"}},
    {"AKS74u", {"aks74u", "aks-74u", "aks74"}},
    {"UMP45", {"ump45", "ump-45", "ump"}},
    {"M1014", {"m1014", "benelli_m1014"}},
    {"Mk11", {"mk11"}},
};
// A word may be followed by digits-free text glued on ("m416reload"): the long, certain names match as plain text too.
static bool nameIsGun(const std::string& low, const Gun& g) {
    for (const std::string& w : g.words) {
        if (hasWord(low, w)) return true;
        if (w.size() >= 4 && w != "stinger" && holds(low, w.c_str())) return true;
    }
    return false;
}
static const char* partWord(const std::string& low) {
    static const char* parts[] = {"reload", "mag", "clip", "charg", "bolt", "cock", "slide", "lid", "belt", "box", "chamber", "handle", "pull", "release", "insert", "remove",
                                  "drum", "lever", "tube", "cover", "feed", "pump", "tail", "rocket", "cartridge", "bullet", "round", "empty", "dry"};
    for (const char* p : parts) if (holds(low, p)) return p;
    return nullptr;
}

// -------------------------------------------------------------------------------------------------- reading a chunk's head
struct ChunkHead {
    bool found = false, read = false, patched = false;
    u32 stored = 0;
    u32 cas = 0, root = 0, offset = 0;
    std::string sbFile;
    u32 audioOff = 0xFFFFFFFFu;  // where the audio starts in the chunk: after the seek tables, if any
    u8 first[32]{};              // the chunk's first bytes
    u32 firstLen = 0;
    u8 audio[32]{};              // the bytes at audioOff
    u32 audioLen = 0;
    int version = -1, codec = -1, channels = 0, rate = 0, type = 0, loop = 0;
    u32 samples = 0;
};
static std::string codecName(int c) {
    switch (c) {
    case 0x00: return "none";
    case 0x02: return "PCM16BE";
    case 0x03: return "EA-XMA";
    case 0x04: return "XAS1";
    case 0x05: return "EALayer3 v1";
    case 0x06: return "EALayer3 v2 PCM";
    case 0x07: return "EALayer3 v2 Spike";
    case 0x08: return "GC-ADPCM";
    case 0x09: return "EASpeex";
    case 0x0A: return "ATRAC9";
    case 0x0B: return "MP3";
    case 0x0C: return "Opus";
    default: return "?";
    }
}
static void parseHead(ChunkHead& h) {
    if (h.audioLen < 8) return;
    size_t at = 0;
    const u8* p = h.audio;
    // The 'H' block holds the audio header four bytes in; otherwise the header stands at the start.
    if (p[0] == 0x48) at = 4;
    if (h.audioLen < at + 8) return;
    u32 w1 = be32(p + at), w2 = be32(p + at + 4);
    h.version = int(w1 >> 28);
    h.codec = int((w1 >> 24) & 0xF);
    h.channels = int((w1 >> 18) & 0x3F) + 1;
    h.rate = int(w1 & 0x3FFFF);
    h.type = int(w2 >> 30);
    h.loop = int((w2 >> 29) & 1);
    h.samples = w2 & 0x1FFFFFFF;
}

// ------------------------------------------------------------------------------------------------- decoding the audio
// A segment is a row of blocks: id in the top byte and the size in the other three of a big-endian word. 0x48 the header,
// 0x44 data (a big-endian sample count, then the frames), 0x45 the end. The one codec the weapons use (and, from codecs.txt,
// every other sound of the campaign) is EA-XAS version 1: per channel a 76-byte frame, one after the other, of 128 samples in
// four groups of 32. A group's header holds its first two samples outright, so a group owes nothing to the one before it:
// that is what makes the seam check below a real one.
struct Decoded {
    int channels = 0, rate = 0;
    std::vector<int16_t> pcm;  // interleaved
    u64 declared = 0, clamped = 0, badCoef = 0, groups = 0;
    u32 blocks = 0, segments = 0, misfit = 0;
    double seamSum = 0, stepSum = 0;
    u64 seamCount = 0;
    std::string error;
    // Where each group's stored start lands against where the decoded samples before it got to, in units of the
    // group's own sample-to-sample movement. A right decode runs into the stored start: near 1. A wrong one lands anywhere.
    double seamRatio() const { return seamCount && stepSum > 0 ? seamSum / stepSum : 0; }
};
struct XasChannel { int last = 0; bool have = false; };

static void xasFrame(const u8* f, int take, int16_t* out, Decoded& d, XasChannel& ch, bool wrongLayout) {
    static const int coef[4][2] = {{0, 0}, {60, 0}, {115, -52}, {98, -55}};  // in 64ths
    int n = 0;
    for (int g = 0; g < 4; ++g) {
        const u32 head = le32(f + g * 4);
        const int idx = head & 15;
        if (idx > 3) ++d.badCoef;  // never in a real stream: the headers are being read wrong
        const int c1 = idx <= 3 ? coef[idx][0] : 0, c2 = idx <= 3 ? coef[idx][1] : 0;
        const int shift = (head >> 16) & 15;
        int hist2 = int16_t(head & 0xFFF0), hist1 = int16_t((head >> 16) & 0xFFF0);
        const int first = hist2;
        out[n++] = int16_t(hist2);
        out[n++] = int16_t(hist1);
        long long moved = std::abs(hist1 - hist2);
        ++d.groups;
        for (int r = 0; r < 15; ++r) {
            const u8 b = wrongLayout ? f[16 + g * 15 + r] : f[16 + r * 4 + g];
            for (int i = 0; i < 2; ++i) {
                const int nibble = i == 0 ? b >> 4 : b & 15;
                const int residual = int(int16_t(u16(nibble << 12))) >> shift;
                int v = (residual * 64 + hist1 * c1 + hist2 * c2) / 64;
                if (v > 32767) { v = 32767; ++d.clamped; }
                else if (v < -32768) { v = -32768; ++d.clamped; }
                out[n++] = int16_t(v);
                moved += std::abs(v - hist1);
                hist2 = hist1;
                hist1 = v;
            }
        }
        // Only where something is sounding: in near silence the 16-step grid of the stored start is all a seam shows.
        if (ch.have && n <= take && moved / 31 >= 32) {
            d.seamSum += std::abs(first - ch.last);
            d.stepSum += double(moved) / 31.0;
            ++d.seamCount;
        }
        ch.last = hist1;
        ch.have = true;
    }
}

static void decodeSegment(const std::vector<u8>& c, size_t at, Decoded& d, bool wrongLayout) {
    auto fail = [&](const std::string& what) { if (d.error.empty()) d.error = what + " at byte " + std::to_string(at); };
    std::vector<XasChannel> state;
    bool end = false;
    u64 out = 0, declared = 0;
    while (at + 4 <= c.size()) {
        const u8 id = c[at];
        const u32 size = be32(&c[at]) & 0xFFFFFF;
        if (size < 4 || at + size > c.size()) { fail("a block that does not fit"); return; }
        if (id == 0x48) {
            if (size < 12) { fail("short header"); return; }
            const u32 w1 = be32(&c[at + 4]), w2 = be32(&c[at + 8]);
            const int version = int(w1 >> 28), codec = int((w1 >> 24) & 15), channels = int((w1 >> 18) & 63) + 1, rate = int(w1 & 0x3FFFF), type = int(w2 >> 30);
            if (version != 1 || codec != 4 || type != 1) {
                fail("no decoder for version " + std::to_string(version) + " codec " + std::to_string(codec) + " (" + codecName(codec) + ") type " + std::to_string(type));
                return;
            }
            if (d.channels && (d.channels != channels || d.rate != rate)) { fail("segments of different formats"); return; }
            d.channels = channels;
            d.rate = rate;
            declared = w2 & 0x1FFFFFFF;
            d.declared += declared;
            state.assign(size_t(channels), {});
        } else if (id == 0x44) {
            if (state.empty()) { fail("data before the header"); return; }
            if (size < 8) { fail("short data block"); return; }
            const u32 samples = be32(&c[at + 4]);
            const size_t ch = state.size(), frameBytes = 76 * ch;
            size_t frames = (size_t(samples) + 127) / 128;
            if (8 + frames * frameBytes != size) ++d.misfit;
            frames = std::min(frames, size_t(size - 8) / frameBytes);
            const size_t base = d.pcm.size();
            d.pcm.resize(base + size_t(samples) * ch);
            int16_t tmp[128];
            for (size_t f = 0; f < frames; ++f) {
                const int take = int(std::min<size_t>(128, size_t(samples) - f * 128));
                for (size_t k = 0; k < ch; ++k) {
                    xasFrame(&c[at + 8 + f * frameBytes + k * 76], take, tmp, d, state[k], wrongLayout);
                    for (int i = 0; i < take; ++i) d.pcm[base + (f * 128 + size_t(i)) * ch + k] = tmp[i];
                }
            }
            out += samples;
            ++d.blocks;
        } else if (id == 0x45) {
            end = true;
            break;
        } else {
            char m[48];
            sprintf_s(m, "unknown block %02x", id);
            fail(m);
            return;
        }
        at += size;
    }
    if (!end) fail("no end block");
    else if (out != declared) fail("data holds " + std::to_string(out) + " samples, the header says " + std::to_string(declared));
    ++d.segments;
}

struct Measure {
    double peakDb = -96, peakAt = 0, loudDb = -96, medianDb = -96, floorDb = -96, crest = 0, attack = 0, attackAt = 0, startAt = 0, tailAt = 0, endDb = 0, lag1 = 0;
    u64 rails = 0;
    std::vector<std::pair<double, double>> events;  // when, how loud
};
static double dbOf(double rms) { return rms < 0.5 ? -96.0 : 20.0 * std::log10(rms / 32768.0); }

// Levels in 10 ms windows, all relative to full scale.
static Measure measure(const Decoded& d) {
    Measure m;
    if (!d.channels || !d.rate || d.pcm.empty()) return m;
    const size_t ch = size_t(d.channels), frames = d.pcm.size() / ch, win = std::max(1, d.rate / 100);
    std::vector<double> db;
    int peak = 0;
    size_t peakFrame = 0;
    for (size_t i = 0; i < d.pcm.size(); ++i) {
        const int a = std::abs(int(d.pcm[i]));
        if (a > peak) { peak = a; peakFrame = i / ch; }
        if (a >= 32767) ++m.rails;
    }
    m.peakDb = dbOf(peak);
    m.peakAt = double(peakFrame) / d.rate;
    for (size_t at = 0; at < frames; at += win) {
        const size_t end = std::min(frames, at + win);
        double sum = 0;
        for (size_t i = at * ch; i < end * ch; ++i) sum += double(d.pcm[i]) * d.pcm[i];
        db.push_back(dbOf(std::sqrt(sum / double((end - at) * ch))));
    }
    std::vector<double> sorted = db;
    std::sort(sorted.begin(), sorted.end());
    m.loudDb = sorted.back();
    m.medianDb = sorted[sorted.size() / 2];
    m.floorDb = sorted[sorted.size() / 10];
    m.crest = m.loudDb - m.medianDb;
    bool started = false;
    for (size_t i = 0; i < db.size(); ++i) {
        const double t = double(i * win) / d.rate;
        if (db[i] >= m.loudDb - 40) {
            if (!started) { m.startAt = t; started = true; }
            m.tailAt = t + 0.01;
        }
        const double earlier = i >= 2 ? db[i - 2] : -96.0;  // before the clip is silence
        if (db[i] >= m.loudDb - 20 && db[i] - earlier > m.attack) { m.attack = db[i] - earlier; m.attackAt = t; }
    }
    double tailSum = 0;
    const size_t tail = std::min(frames, win * 5);
    for (size_t i = (frames - tail) * ch; i < frames * ch; ++i) tailSum += double(d.pcm[i]) * d.pcm[i];
    m.endDb = dbOf(std::sqrt(tailSum / double(tail * ch))) - m.loudDb;
    double top = 0, bottom = 0;
    for (size_t i = 1; i < frames; ++i) { top += double(d.pcm[i * ch]) * d.pcm[(i - 1) * ch]; bottom += double(d.pcm[i * ch]) * d.pcm[i * ch]; }
    m.lag1 = bottom > 0 ? top / bottom : 0;
    // The separate sounds in a clip: a rise of 12 dB over the 30 ms before it, within 30 dB of the loudest.
    double lastEvent = -1;
    for (size_t i = 3; i < db.size(); ++i) {
        const double t = double(i * win) / d.rate;
        const double before = std::min({db[i - 1], db[i - 2], db[i - 3]});
        if (db[i] - before < 12 || db[i] < m.loudDb - 30 || (lastEvent >= 0 && t - lastEvent < 0.08)) continue;
        double top5 = db[i];
        for (size_t k = i; k < db.size() && k < i + 5; ++k) top5 = std::max(top5, db[k]);
        m.events.push_back({t, top5});
        lastEvent = t;
    }
    return m;
}

// A 16-bit PCM wave file's bytes, as writeWav writes them.
static std::vector<u8> wavBytes(const Decoded& d) {
    const u32 bytes = u32(d.pcm.size() * 2), riff = 36 + bytes, fmtSize = 16, rate = u32(d.rate), perSec = rate * u32(d.channels) * 2;
    const u16 tag = 1, channels = u16(d.channels), align = u16(d.channels * 2), bits = 16;
    std::vector<u8> o;
    o.reserve(44 + size_t(bytes));
    auto put = [&](const void* v, size_t n) { const u8* p = (const u8*)v; o.insert(o.end(), p, p + n); };
    put("RIFF", 4); put(&riff, 4); put("WAVE", 4);
    put("fmt ", 4); put(&fmtSize, 4); put(&tag, 2); put(&channels, 2); put(&rate, 4); put(&perSec, 4); put(&align, 2); put(&bits, 2);
    put("data", 4); put(&bytes, 4); put(d.pcm.data(), bytes);
    return o;
}
static bool writeWav(const fs::path& p, const Decoded& d) {
    const std::vector<u8> b = wavBytes(d);
    std::ofstream o(p, std::ios::binary);
    if (!o) return false;
    o.write((const char*)b.data(), std::streamsize(b.size()));
    return o.good();
}

// A whole audio chunk as it stands in the archive or its sb.
static bool readChunk(const Guid& id, std::vector<u8>& out) {
    auto ck = g_chunks.find(id);
    if (ck == g_chunks.end()) return false;
    if (!ck->second.sb.empty()) return readFilePart(ck->second.sb, ck->second.sbOff, ck->second.sbSize, out);
    auto ct = g_cat.find(ck->second.sha);
    return ct != g_cat.end() && readRaw(ct->second, ct->second.size, out);
}

// The reload waves of the first mission's guns, and the other pieces of their handling, as the sound graphs name them.
// The M416 has no reload of its own: its graph plays the M16's.
static const char* const g_reloadWaves[] = {
    "an94/an94_reload_long_wave", "an94/an94_reload_short_wave",
    "rpk/rpk_reload_long_wave", "rpk/rpk_reload_short_wave",
    "aek971/aek971_reload_long_wave", "aek971/aek971_reload_short_wave",
    "ak74/ak74_reload_1p_long_wave", "ak74/ak74_reload_1p_short_wave",
    "g3/g3_reload_1p_full_wave", "g3/g3_reload_1p_quick_wave",
    "kh2002/kh2002_reload_long_wave", "kh2002/kh2002_reload_short_wave",
    "m16a3/m16_reload_1p_long_wave", "m16a3/m16_reload_1p_short_wave",
    "m4a1/m4_reload_long_wave", "m4a1/m4_reload_short_wave",
    "m240/m240_reload_wave",
    "aks74u/aks74u_reload_1p_long_wave", "aks74u/aks74u_reload_1p_short_wave",
    "ump/ump_reload_1p_full_wave", "ump/ump_reload_1p_clip_wave",
    "fim92_stinger/fim92_stinger_reload_wave", "fim92_stinger/fim92_stinger_deploy_wave",
    "smaw/smaw_reload_wave",
    "m9/m9_reload_long_wave", "m9/m9_reload_short_wave", "m9/m9_deploy_wave",
    "gp30/gp30_reload_wave",
    "m26mass/m26mass_reload_wave", "m26mass/m26mass_bolt_action_wave", "m26mass/m26mass_mounted_long_wave", "m26mass/m26mass_mounted_short_wave",
    "m26mass/m26mass_standalone_long_wave", "m26mass/m26mass_standalone_short_wave",
};

struct DecodeJob {
    const ParsedFile* pf = nullptr;
    const Wave* wave = nullptr;
    const WaveVariation* var = nullptr;
    std::string file;
    bool write = false;
    u32 root = 0, cas = 0, offset = 0;
};
// Decodes one variation, with a second decode of the same bytes reading each group's data as one run instead of across the rows, as the control the
// seam ratio has to beat, and returns its line of numbers. bad comes back true when the numbers do not look like a sound.
static std::string decodeOne(const DecodeJob& job, const std::vector<u8>& chunk, const fs::path& wavDir, bool& bad, std::string& events) {
    Decoded d, wrong;
    float assetSeconds = 0;
    for (const WaveSegment& s : job.var->segments) {
        assetSeconds += s.seconds;
        decodeSegment(chunk, s.samplesOffset, d, false);
        decodeSegment(chunk, s.samplesOffset, wrong, true);
    }
    char line[640];
    if (!d.error.empty() || d.pcm.empty()) {
        bad = true;
        return job.file + "  FAILED: " + (d.error.empty() ? std::string("no audio") : d.error);
    }
    const Measure m = measure(d);
    const u64 samples = d.pcm.size() / size_t(d.channels);
    const double seconds = double(samples) / d.rate;
    const double seam = d.seamRatio(), seamWrong = wrong.seamRatio();
    std::string flags;
    if (samples != d.declared) flags += " samples";
    if (d.misfit) flags += " blocksize";
    if (d.badCoef) flags += " coef";
    if (std::fabs(seconds - assetSeconds) > 0.011) flags += " duration";
    if (m.crest < 15) flags += " crest";
    if (m.attack < 10) flags += " attack";
    if (m.endDb > -30) flags += " noend";
    if (m.lag1 < 0.3) flags += " lag1";
    if (d.clamped * 1000 > d.pcm.size()) flags += " clamped";
    if (d.seamCount >= 8 && (seam > 1.5 || seam * 1.5 > seamWrong)) flags += " seam";
    bad = !flags.empty();
    sprintf_s(line, "ch %d  %d Hz  %llu samples (header %llu)  %.3fs (asset %.3fs)  seg %u blocks %u | peak %.1f dB at %.3fs  loudest %.1f  median %.1f  floor %.1f  loudest-median %.1f dB | attack %.1f dB at %.3fs  from %.3fs  to %.3fs  end %.1f dB | clamped %llu  at rails %llu  badcoef %llu | seam %.2f (wrong read %.2f, over %llu groups)  lag1 %.3f",
              d.channels, d.rate, (unsigned long long)samples, (unsigned long long)d.declared, seconds, double(assetSeconds), d.segments, d.blocks, m.peakDb, m.peakAt, m.loudDb, m.medianDb,
              m.floorDb, m.crest, m.attack, m.attackAt, m.startAt, m.tailAt, m.endDb, (unsigned long long)d.clamped, (unsigned long long)m.rails, (unsigned long long)d.badCoef, seam, seamWrong,
              (unsigned long long)d.seamCount, m.lag1);
    std::string out = job.file + "  " + line;
    if (job.var->segments.size() > 1 && job.var->firstLoop != job.var->lastLoop + 1) out += "  (loop segments " + std::to_string(job.var->firstLoop) + "-" + std::to_string(job.var->lastLoop) + ")";
    if (d.channels > 2) out += "  (the mod's player takes mono or stereo only)";
    out += flags.empty() ? "  OK" : "  LOOK:" + flags;
    if (!m.events.empty()) {
        events = "      sounds at";
        char ev[32];
        for (const auto& e : m.events) { sprintf_s(ev, " %.2fs(%.0f)", e.first, e.second); events += ev; }
    }
    if (job.write) {
        fs::create_directories(wavDir);
        if (!writeWav(wavDir / (job.file + ".wav"), d)) { bad = true; out += "  COULD NOT WRITE"; }
    }
    return out;
}

// ---- Which part of a reload is which: the study behind the final clips. Everything here works on decoded game audio; the recordings
// in build\survey are only read to check identity, and none of their samples goes anywhere.

// One variation of a wave by its exact name, decoded from the data.
static bool decodeByName(const std::string& name, Decoded& d, int variation = 0) {
    const std::string low = lower(name);
    for (const ParsedFile& pf : g_files) {
        if (lower(pf.ref->name) != low || pf.waves.empty()) continue;
        const Wave& w = pf.waves[0];
        if (size_t(variation) >= w.variations.size()) return false;
        const WaveVariation& v = w.variations[size_t(variation)];
        if (v.chunkIndex >= w.chunks.size()) return false;
        std::vector<u8> chunk;
        if (!readChunk(w.chunks[v.chunkIndex].id, chunk)) return false;
        for (const WaveSegment& s : v.segments) decodeSegment(chunk, s.samplesOffset, d, false);
        return d.error.empty() && !d.pcm.empty();
    }
    return false;
}
// A wave file as mono numbers in the 16-bit scale (32768 is full scale): 16-bit PCM or 32-bit float, the channels averaged.
static bool readWavMono(const fs::path& p, std::vector<double>& out, int& rate) {
    std::vector<u8> all;
    try { all = readAll(p); } catch (const std::exception&) { return false; }
    if (all.size() < 12 || memcmp(all.data(), "RIFF", 4)) return false;
    int channels = 0, bits = 0, tag = 0;
    for (size_t at = 12; at + 8 <= all.size();) {
        u32 size = le32(&all[at + 4]);
        if (at + 8 + size > all.size()) size = u32(all.size() - at - 8);
        if (!memcmp(&all[at], "fmt ", 4)) {
            tag = le16(&all[at + 8]); channels = le16(&all[at + 10]); rate = int(le32(&all[at + 12])); bits = le16(&all[at + 22]);
            if (tag == 0xFFFE && size >= 26) tag = le16(&all[at + 32]);
        } else if (!memcmp(&all[at], "data", 4) && channels) {
            const size_t frames = size / (size_t(channels) * bits / 8);
            out.resize(frames);
            for (size_t f = 0; f < frames; ++f) {
                double sum = 0;
                for (int c = 0; c < channels; ++c) {
                    const u8* q = &all[at + 8 + (f * channels + c) * (bits / 8)];
                    if (tag == 3 && bits == 32) { float v; memcpy(&v, q, 4); sum += v * 32768.0; }
                    else if (tag == 1 && bits == 16) sum += int16_t(le16(q));
                    else return false;
                }
                out[f] = sum / channels;
            }
            return true;
        }
        at += 8 + size + (size & 1);
    }
    return false;
}
static std::vector<double> toMono(const Decoded& d) {
    const size_t ch = size_t(std::max(1, d.channels));
    std::vector<double> m(d.pcm.size() / ch);
    for (size_t i = 0; i < m.size(); ++i) { double s = 0; for (size_t c = 0; c < ch; ++c) s += d.pcm[i * ch + c]; m[i] = s / double(ch); }
    return m;
}
static void fft(std::vector<std::complex<double>>& a, bool inverse) {
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1) {
        const double ang = 6.283185307179586 / double(len) * (inverse ? 1 : -1);
        const std::complex<double> wl(std::cos(ang), std::sin(ang));
        for (size_t i = 0; i < n; i += len) {
            std::complex<double> w(1);
            for (size_t k = 0; k < len / 2; ++k) {
                const auto u = a[i + k], v = a[i + k + len / 2] * w;
                a[i + k] = u + v;
                a[i + k + len / 2] = u - v;
                w *= wl;
            }
        }
    }
    if (inverse) for (auto& x : a) x /= double(n);
}
// First difference: clicks of metal stay, a battlefield's rumble goes.
static std::vector<double> sharpened(const std::vector<double>& v) {
    std::vector<double> o(v.size());
    for (size_t i = 1; i < v.size(); ++i) o[i] = v[i] - v[i - 1];
    return o;
}
struct Corr { double peak = 0, peakAt = 0, second = 0, secondAt = 0, spread = 0; };
// How well the snippet fits the signal at every place: the normalised cross-correlation (1 is the same waveform at any level).
// A window whose energy is below a thousandth of the signal's average is left out.
static std::vector<double> nccAll(const std::vector<double>& snippet, const std::vector<double>& sig) {
    size_t n = 1;
    while (n < snippet.size() + sig.size()) n <<= 1;
    std::vector<std::complex<double>> a(n), b(n);
    for (size_t i = 0; i < snippet.size(); ++i) a[snippet.size() - 1 - i] = snippet[i];
    for (size_t i = 0; i < sig.size(); ++i) b[i] = sig[i];
    fft(a, false); fft(b, false);
    for (size_t i = 0; i < n; ++i) a[i] *= b[i];
    fft(a, true);
    double te = 0;
    for (double v : snippet) te += v * v;
    std::vector<double> cum(sig.size() + 1);
    for (size_t i = 0; i < sig.size(); ++i) cum[i + 1] = cum[i] + sig[i] * sig[i];
    const double floorE = cum.back() / double(std::max<size_t>(1, sig.size())) * double(snippet.size()) * 0.001;
    std::vector<double> r(sig.size() >= snippet.size() ? sig.size() - snippet.size() + 1 : 0);
    for (size_t i = 0; i < r.size(); ++i) {
        const double e = cum[i + snippet.size()] - cum[i];
        r[i] = e > floorE && te > 0 ? a[i + snippet.size() - 1].real() / std::sqrt(te * e) : 0;
    }
    return r;
}
// The best place, the best place at least 60 ms from it, the spread of all of them.
static Corr corrSummary(const std::vector<double>& r, int rate) {
    Corr c;
    if (r.empty()) return c;
    size_t at = 0;
    double sum = 0;
    for (size_t i = 0; i < r.size(); ++i) { sum += r[i] * r[i]; if (r[i] > r[at]) at = i; }
    c.peak = r[at]; c.peakAt = double(at) / rate;
    c.spread = std::sqrt(sum / double(r.size()));
    const size_t keep = size_t(rate) * 60 / 1000;
    size_t at2 = at;
    double best2 = -2;
    for (size_t i = 0; i < r.size(); ++i) if ((i + keep < at || i > at + keep) && r[i] > best2) { best2 = r[i]; at2 = i; }
    c.second = best2; c.secondAt = double(at2) / rate;
    return c;
}
static std::vector<double> slice(const std::vector<double>& v, double from, double to, int rate) {
    const size_t a = std::min(v.size(), size_t(std::llround(std::max(0.0, from) * rate))), b = std::min(v.size(), size_t(std::llround(std::max(0.0, to) * rate)));
    return std::vector<double>(v.begin() + std::ptrdiff_t(a), v.begin() + std::ptrdiff_t(std::max(a, b)));
}
static std::vector<double> envelopeDb(const std::vector<double>& v, int rate, double windowMs) {
    const size_t win = std::max<size_t>(1, size_t(rate * windowMs / 1000.0));
    std::vector<double> o;
    for (size_t at = 0; at < v.size(); at += win) {
        const size_t e = std::min(v.size(), at + win);
        double s = 0;
        for (size_t i = at; i < e; ++i) s += v[i] * v[i];
        o.push_back(dbOf(std::sqrt(s / double(e - at))));
    }
    return o;
}
// Where a sound's energy sits, in Hz: rough, enough to tell a click from a thud.
static double centroidHz(const std::vector<double>& v, int rate) {
    size_t n = 1;
    while (n < v.size() && n < (1u << 15)) n <<= 1;
    if (n < 16) return 0;
    std::vector<std::complex<double>> a(n);
    for (size_t i = 0; i < n && i < v.size(); ++i) a[i] = v[i] * (0.5 - 0.5 * std::cos(6.283185307 * double(i) / double(n)));
    fft(a, false);
    double top = 0, bot = 0;
    for (size_t i = 1; i < n / 2; ++i) { const double e = std::norm(a[i]); top += e * double(i) * rate / double(n); bot += e; }
    return bot > 0 ? top / bot : 0;
}
struct Level { double seconds = 0, peak = -96, loudest = -96, active = -96, median = -96; };
// Peak; loudest 10 ms; the level over the windows within 30 dB of the loudest (what the ear calls its loudness); the median window.
static Level levelOf(const std::vector<double>& v, int rate) {
    Level l;
    if (v.empty()) return l;
    l.seconds = double(v.size()) / rate;
    double pk = 0;
    for (double s : v) pk = std::max(pk, std::fabs(s));
    l.peak = dbOf(pk);
    std::vector<double> e = envelopeDb(v, rate, 10.0);
    std::vector<double> sorted = e;
    std::sort(sorted.begin(), sorted.end());
    l.loudest = sorted.back();
    l.median = sorted[sorted.size() / 2];
    double sum = 0; size_t n = 0;
    for (double d : e) if (d >= l.loudest - 30) { sum += std::pow(10.0, d / 10.0); ++n; }
    l.active = n ? 10.0 * std::log10(sum / double(n)) : -96;
    return l;
}
static const char* const g_studyWaves[] = {
    "an94/an94_reload_long_wave", "an94/an94_reload_short_wave", "rpk/rpk_reload_long_wave", "rpk/rpk_reload_short_wave",
    "aek971/aek971_reload_long_wave", "aek971/aek971_reload_short_wave", "ak74/ak74_reload_1p_long_wave", "ak74/ak74_reload_1p_short_wave",
    "g3/g3_reload_1p_full_wave", "g3/g3_reload_1p_quick_wave", "kh2002/kh2002_reload_long_wave", "kh2002/kh2002_reload_short_wave",
    "m16a3/m16_reload_1p_long_wave", "m16a3/m16_reload_1p_short_wave", "m4a1/m4_reload_long_wave", "m4a1/m4_reload_short_wave",
    "m240/m240_reload_wave", "fim92_stinger/fim92_stinger_reload_wave", "fim92_stinger/fim92_stinger_deploy_wave",
    // Semper Fidelis's two (the UMP-45's shorter reload is its "clip" wave)
    "aks74u/aks74u_reload_1p_long_wave", "aks74u/aks74u_reload_1p_short_wave", "ump/ump_reload_1p_full_wave", "ump/ump_reload_1p_clip_wave",
    // for what a T handle sounds like, in the guns that have one
    "m27iar/m27iar_reload_long_wave", "m27iar/m27iar_reload_short_wave", "xp2_hk417/hk417_reload_long_wave", "xp2_hk417/hk417_reload_short_wave",
    "mk11/mk11_reload_long_wave", "mk11/mk11_reload_short_wave", "m249/m249_1p_reload_wave", "xp2_lsat/lsat_reload_long_wave",
    // the small rifles whose empty reload does pull a handle
    "scar-h/scar-h_reload_long_wave", "scar-h/scar-h_reload_short_wave", "xp2_scar_l/scar-l_reload_long_wave", "xp2_scar_l/scar-l_reload_short_wave",
    "xp2_acr/acr_reload_long_wave", "xp2_acr/acr_reload_short_wave", "g36c/g36c_reload_long_wave", "g36c/g36c_reload_short_wave",
    "xp1_hk53/hk53_reload_long_wave", "xp1_hk53/hk53_reload_short_wave", "sg553/ssg553_reload_long_wave", "sg553/ssg553_reload_short_wave",
    "xp1_famas/famas_reload_long_wave", "xp1_famas/famas_reload_short_wave", "xp1_qbz95b/qbz95b_reload_long_wave", "xp1_qbz95b/qbz95b_reload_short_wave",
    "xp1_l85a2/l85a2_reload_long_wave", "xp1_l85a2/l85a2_reload_short_wave", "m39ebr/m39_reload_long_wave", "m39ebr/m39_reload_short_wave",
    "f2000/f2000_reload_long_wave", "f2000/f2000_reload_short_wave", "xp2_steyraug/aug_reload_long_wave", "xp2_steyraug/aug_reload_short_wave",
};
static void study(const fs::path& outDir) {
    const fs::path dir = outDir / "final";
    fs::create_directories(dir);
    std::ofstream o(dir / "study.txt", std::ios::binary);
    char m[400];
    std::map<std::string, std::vector<double>> waves;
    for (const char* w : g_studyWaves) {
        Decoded d;
        if (decodeByName(std::string("sound/weapons/handheld/") + w, d)) waves[w] = toMono(d);
        else o << "NOT DECODED " << w << "\n";
    }
    // Each wave: every 10 ms its level; each rise; of a long reload, whether that bit of it is in the short one too.
    for (const char* w : g_studyWaves) {
        auto it = waves.find(w);
        if (it == waves.end()) continue;
        const std::vector<double>& v = it->second;
        std::string name = w;
        std::string partner;
        const size_t lp = name.find("_long_");
        if (lp != std::string::npos) partner = name.substr(0, lp) + "_short_" + name.substr(lp + 6);
        const size_t fp = name.find("_full_");
        if (fp != std::string::npos) partner = name.substr(0, fp) + "_quick_" + name.substr(fp + 6);
        if (fp != std::string::npos && !waves.count(partner)) partner = name.substr(0, fp) + "_clip_" + name.substr(fp + 6);  // (the UMP-45's)
        o << "== " << name << " (" << double(v.size()) / 48000.0 << " s)\n";
        const std::vector<double> e = envelopeDb(v, 48000, 10.0);
        for (size_t i = 0; i < e.size(); i += 25) {
            sprintf_s(m, "%6.2f:", double(i) * 0.01);
            o << m;
            for (size_t k = i; k < e.size() && k < i + 25; ++k) { sprintf_s(m, " %3d", int(std::lround(e[k]))); o << m; }
            o << "\n";
        }
        const double top = *std::max_element(e.begin(), e.end());
        double last = -1;
        for (size_t i = 2; i < e.size(); ++i) {
            const double t = double(i) * 0.01;
            if (e[i] - std::min(e[i - 1], e[i - 2]) < 8 || e[i] < top - 45 || t - last < 0.05) continue;
            last = t;
            double pk = e[i];
            size_t end = i;
            for (size_t k = i; k < e.size() && k < i + 8; ++k) pk = std::max(pk, e[k]);
            while (end + 1 < e.size() && e[end + 1] > pk - 30 && double(end - i) * 0.01 < 0.4) ++end;
            sprintf_s(m, "  rise %.2f peak %.0f dB, decays by %.2f, centre %.0f Hz", t, pk, double(end + 1) * 0.01, centroidHz(slice(v, t - 0.005, t + 0.1, 48000), 48000));
            o << m;
            if (!partner.empty() && waves.count(partner)) {
                const std::vector<double> bit = slice(v, t - 0.005, t + 0.12, 48000);
                const Corr c = corrSummary(nccAll(bit, waves[partner]), 48000);
                sprintf_s(m, "   | in the shorter one: %.2f at %.2f", c.peak, c.peakAt + 0.005);
                o << m;
            }
            o << "\n";
        }
    }
    // Does any other gun's reload hold the very sound of the M16's and M4's bolt going home? The same recording used twice is the sound designers saying "the same kind of gun".
    o << "\n== the M16's and the M4's bolt in the other reloads (best correlation over every reload wave, above 0.5)\n";
    for (const auto& [from, t0, t1] : {std::tuple<const char*, double, double>{"m16a3/m16_reload_1p_long_wave", 1.945, 2.235}, {"m4a1/m4_reload_long_wave", 1.905, 2.16}}) {
        auto src = waves.find(from);
        if (src == waves.end()) continue;
        const std::vector<double> bit = slice(src->second, t0, t1, 48000);
        for (const ParsedFile& pf : g_files) {
            const std::string low = lower(pf.ref->name);
            if (!holds(low, "sound/weapons/handheld/") || !holds(low, "reload") || pf.waves.empty()) continue;
            Decoded d;
            if (!decodeByName(pf.ref->name, d) || d.channels != 1) continue;
            const Corr c = corrSummary(nccAll(bit, toMono(d)), 48000);
            if (c.peak < 0.5) continue;
            sprintf_s(m, "%s  %.2f at %.3f in %s\n", from, c.peak, c.peakAt, pf.ref->name.c_str());
            o << m;
        }
    }
    // The clips already liked, which are the game's own waves cut: where each came from and at what level.
    o << "\n== the clips in build\\sounds\n";
    const fs::path sounds = outDir.parent_path() / "sounds";
    for (const char* file : {"bolt.wav", "mag_in.wav", "mag_out.wav"}) {
        std::vector<double> v;
        int rate = 0;
        if (!readWavMono(sounds / file, v, rate)) { o << file << " unreadable\n"; continue; }
        const Level lv = levelOf(v, rate);
        const std::vector<double>& src = waves["m16a3/m16_reload_1p_long_wave"];
        const Corr c = corrSummary(nccAll(v, src), rate);
        const std::vector<double> at = slice(src, c.peakAt, c.peakAt + double(v.size()) / rate, rate);
        double xy = 0, xx = 0;
        for (size_t i = 0; i < at.size() && i < v.size(); ++i) { xy += at[i] * v[i]; xx += at[i] * at[i]; }
        sprintf_s(m, "%-12s %.3f s  peak %.1f  loudest %.1f  active %.1f  median %.1f | fits the M16's long reload at %.4f s, correlation %.4f, gain %.2f dB\n", file, lv.seconds, lv.peak, lv.loudest, lv.active, lv.median, c.peakAt, c.peak, 20 * std::log10(xy / xx));
        o << m;
    }
}
// How a part moves through a survey recording: the bone's distance from its own first place on the frame bone, in cm, as it changes.
static void traceBin(const fs::path& bin, int kind, int start, int frameBone, const std::vector<int>& bones, std::ostream& o) {
    std::ifstream f(bin, std::ios::binary);
    if (!f) { o << bin.string() << " missing\n"; return; }
    o << "-- " << bin.filename().string() << " draw " << kind << "/" << start << " frame bone " << frameBone << "\n";
    struct Pt { int ms; double p[3]; };
    std::map<int, std::vector<Pt>> tracks;
    std::vector<float> buf;
    int head[8];
    int firstMs = -1, lastLoaded = -999;
    std::string ammo;
    while (f.read(reinterpret_cast<char*>(head), sizeof head)) {
        const int count = head[7];
        if (count < 0 || count > 4096) break;
        if (head[0] != kind || head[3] != start) { f.seekg(std::streamoff(count) * 48, std::ios::cur); continue; }
        buf.resize(size_t(count) * 12);
        f.read(reinterpret_cast<char*>(buf.data()), std::streamsize(count) * 48);
        if (frameBone >= count) continue;
        const float* a = &buf[size_t(frameBone) * 12];
        if (std::fabs(a[3]) > 5 || std::fabs(a[7]) > 5 || std::fabs(a[11]) > 5 || (a[0] == 0 && a[5] == 0)) continue;
        if (firstMs < 0) firstMs = head[6];
        if (head[4] != lastLoaded) { char t[64]; sprintf_s(t, " %d ms: loaded %d;", head[6] - firstMs, head[4]); ammo += t; lastLoaded = head[4]; }
        for (int b : bones) {
            if (b >= count) continue;
            const float* m = &buf[size_t(b) * 12];
            const double dx = m[3] - a[3], dy = m[7] - a[7], dz = m[11] - a[11];
            tracks[b].push_back({head[6] - firstMs, {a[0] * dx + a[4] * dy + a[8] * dz, a[1] * dx + a[5] * dy + a[9] * dz, a[2] * dx + a[6] * dy + a[10] * dz}});
        }
    }
    o << "  ammo:" << ammo << "\n";
    for (auto& kv : tracks) {
        const auto& t = kv.second;
        if (t.empty()) continue;
        o << "  bone " << kv.first << " (ms:cm from its first place):";
        int lastMs = -1000;
        double lastD = 0;
        for (const Pt& p : t) {
            const double d = std::sqrt((p.p[0] - t[0].p[0]) * (p.p[0] - t[0].p[0]) + (p.p[1] - t[0].p[1]) * (p.p[1] - t[0].p[1]) + (p.p[2] - t[0].p[2]) * (p.p[2] - t[0].p[2])) * 100.0;
            if (p.ms - lastMs < 33 || std::fabs(d - lastD) < 0.2) continue;
            char c[40];
            sprintf_s(c, " %d:%.1f", p.ms, d);
            o << c;
            lastMs = p.ms; lastD = d;
        }
        o << "\n";
    }
}
static void traces(const fs::path& outDir) {
    const fs::path dir = outDir / "final", sv = outDir.parent_path() / "survey";
    fs::create_directories(dir);
    std::ofstream o(dir / "traces.txt", std::ios::binary);
    traceBin(sv / "lever_kh2002.bin", 15237, 0, 3, {1, 2}, o);
    traceBin(sv / "lever_m240.bin", 27588, 2364, 3, {2, 4, 5}, o);
    traceBin(sv / "KH2002_empty.bin", 15237, 0, 3, {1, 2}, o);
    traceBin(sv / "M240_empty.bin", 27588, 2364, 3, {2, 4, 5}, o);
    traceBin(sv / "M16A4_empty.bin", 17013, 312, 4, {0, 2}, o);
    traceBin(sv / "M4A1_empty.bin", 22194, 312, 2, {0, 3}, o);
    traceBin(sv / "AK74M_empty.bin", 21723, 0, 2, {1, 3}, o);
    traceBin(sv / "AEK971_empty.bin", 21585, 0, 4, {0, 2, 3}, o);
    traceBin(sv / "G3A3_empty.bin", 23064, 0, 0, {1, 3}, o);
    traceBin(sv / "FIM92A_empty.bin", 20298, 354, 0, {3, 4, 5, 6}, o);
    traceBin(sv / "AN94_partial.bin", 16188, 0, 0, {2, 4, 5}, o);
    traceBin(sv / "RPK_partial.bin", 24492, 0, 3, {0, 1}, o);
    traceBin(sv / "AK74M_partial.bin", 21723, 0, 2, {1, 3}, o);
    traceBin(sv / "M16A4_partial.bin", 17013, 312, 4, {0, 2}, o);
    traceBin(sv / "AKS74u_empty.bin", 17991, 0, 2, {0, 3}, o);
    traceBin(sv / "AKS74u_partial.bin", 17991, 0, 2, {0, 3}, o);
    traceBin(sv / "UMP45_empty.bin", 19314, 234, 2, {0, 1}, o);
    traceBin(sv / "UMP45_partial.bin", 19314, 234, 2, {0, 1}, o);
    traceBin(sv / "MK11_empty.bin", 12456, 2214, 0, {1, 3}, o);
    traceBin(sv / "MK11_partial.bin", 12456, 2214, 0, {1, 3}, o);
}

// The liked clips (build\sounds\bolt, mag_out) are the M16's long reload cut and turned down by exactly this, and the game's reload waves are mastered
// alike (each peaks near 0 dB), so one gain keeps every gun beside them and the guns' own balance as the game has it. --gain changes it.
constexpr double kFinalGainDb = -11.8;
constexpr double kFadeInMs = 3.0, kFadeOutMs = 20.0;
// A reload is one continuous recording per gun. How each cut was found, in order of weight: the empty reload ("long") against the same gun's partial one ("short"), whose
// difference is the lever (correlation of each rise with the short one, in study.txt); the bones' movement in the empty reload (traces.txt); then the valleys of the 10 ms level
// (study.txt), each cut ending where the sound has died and the next has not begun. For the KH2002 and the M240 the recordings also confirm it (verify.txt).
struct Cut {
    const char* part;
    const char* shape;
    const char* wave;   // under sound/weapons/handheld/ (or a whole name from sound/ on)
    double from, to;
    const char* sure;   // sure / fairly sure / unsure / borrowed
    const char* why;
    int variation = 0;   // which of the wave's takes (a sound with several: the casings' landings)
    double pitch = 1.0;  // played this much faster, as the game's own sound graph plays it (the case's landing at 0.725)
    double gain = NAN;   // dB on the wave when not the table's one (g_modClips: each older clip's own level, as --fit found it)
};
static const char* const kAn94Long = "an94/an94_reload_long_wave";
static const char* const kRpkLong = "rpk/rpk_reload_long_wave";
static const char* const kAekLong = "aek971/aek971_reload_long_wave";
static const char* const kAkLong = "ak74/ak74_reload_1p_long_wave";
static const char* const kG3Full = "g3/g3_reload_1p_full_wave";
static const char* const kKhLong = "kh2002/kh2002_reload_long_wave";
static const char* const kM16Long = "m16a3/m16_reload_1p_long_wave";
static const char* const kM4Long = "m4a1/m4_reload_long_wave";
static const char* const kM240 = "m240/m240_reload_wave";
static const char* const kStinger = "fim92_stinger/fim92_stinger_reload_wave";
static const char* const kAksLong = "aks74u/aks74u_reload_1p_long_wave";
static const char* const kUmpFull = "ump/ump_reload_1p_full_wave";
static const char* const kMk11Long = "mk11/mk11_reload_long_wave";
static const char* const kG36Long = "g36c/g36c_reload_long_wave";
static const char* const kScarLLong = "xp2_scar_l/scar-l_reload_long_wave";
// The grenade launcher under the M416 reloaded by hand (the user, 2026-11-05): its own reload wave (three takes joined by silence: the barrel swung open, the case pulled, the round
// in, the barrel shut), the dry click and the case landing as its sound graph has them (weapon_gl_m320_underslung: the landing is shellcasings_50cal_outdoor, ten takes, pitch
// 0.65-0.8, after 1.7-1.9 s).
// Loose things landing (src/loose.cpp's hits; the user, 2026-11-05: "I want magazine physics ... I want it to hit the floor"): the game's own sound for a gun dropped on each
// ground (soldier_ragdoll_and_weapon_drop_sp: weapon_on_<surface>), its generic impact sound's rattly metal (the magazine's later small hits; pitch 1.1 as the impact player
// plays it) and its hollow plastic (the AK family's polymer magazines).
// The heavy guns' smaller knocks once let go of (free hands: the launchers and the M240): the same impact player's heavy metal, which its patch (metal_heavy_scrape_patchconfig)
// plays at the same impact pitch, 1.1.
static const char* const kWeaponOnDirt = "sound/maoism/special_objects/handheld_weapons/weapon_on_dirt_wave";
static const char* const kWeaponOnTarmac = "sound/maoism/special_objects/handheld_weapons/weapon_on_tarmac_wave";
static const char* const kWeaponOnGrass = "sound/maoism/special_objects/handheld_weapons/weapon_on_grass_wave";
static const char* const kWeaponOnWood = "sound/maoism/special_objects/handheld_weapons/weapon_on_wood_wave";
static const char* const kWeaponOnMetal = "sound/maoism/special_objects/handheld_weapons/weapon_on_metal_wave";
static const char* const kMetalRattly = "sound/maoism/generic_materials/metal/metal_rattly_impact_wave";
static const char* const kMetalHeavy = "sound/maoism/generic_materials/metal/metal_heavy_impact_wave";
static const char* const kPlasticHollow = "sound/maoism/generic_materials/plastic/plastic_hollow_impact_wave";
static const char* const kM320Reload = "m320/m320_underslung_reload_wave";
static const char* const kTriggerClick = "sound/weapons/shared_content/handling/trigger_click_wave";
static const char* const kCase50Out = "sound/weapons/shared_content/shellcasings/shellcasings_50cal_outdoor_wave";
// The M1014's shells by hand (src/shellload.cpp; Semper Fidelis, 2026-11-18). Its two reload waves, each three pieces joined (a start, one loop a shell, an
// end). The tactical one (1.867 s: 0.57 + 0.50 + 0.80): the gun rolled over in the start, a shell sliding up from 0.57 and its click past the latch at
// 0.77 to 0.79 (the loudest of the wave, -1 dB), the gun rolled back in the end. The full one, from empty (3.148 s: 1.93 + 0.50 + 0.71): a shell
// dropped into the ejection port at 0.98 to 1.01, a clear silence, then the release and the bolt slamming home from 1.11, its peak at 1.196 (the loudest,
// 0 dB), died away by 1.33; its loop's click at 2.14. Nothing in either pulls the bolt handle (the game never does), so the handle drawn back has no
// sound of its own and none is borrowed: let go, the bolt's own slam plays.
static const char* const kM1014Tactical = "benelli_m1014/m1014_reload_tactical_wave";
static const char* const kM1014Full = "benelli_m1014/m1014_reload_full_wave";
static const char* const kShellShotgunOut = "sound/weapons/shared_content/shellcasings/shellcasings_shotgun_outdoor_wave";
// A gun drawn into the hand from where it lies (free hands' magnet take): the game's shared deploy sound, which every first-mission gun's sound graph plays
// when the gun is brought up (a WaveSwitcherNodeData in each patch: guns.txt). Eight takes, each about 1.1 s: the hand's grab in the first 0.2 s, then a
// dip and the gun settling. The grab is cut from take 0, the one with the clearest quiet before it and the deepest dip after it (its settling after the
// dip is the put-away's other sound, stow_settle).
static const char* const kGenericDeploy = "sound/weapons/shared_content/handling/generic_deploy_wave";
// A gun put away at its holster's place by hand: the game has no holster sound, so its own gear rustle, the vest's sound when the soldier changes pose
// (sound/character/soldier_movement_sounds_sp plays it). Six takes of 0.68 to 0.98 s, each a rustle with a knock or two in it; a short piece of one is cut.
static const char* const kVestPoseChange = "sound/character/clothing/clothes_vest_posechange_wave";
// The Stinger taken back from its place by hand: its own deploy wave, which the game plays as the launcher comes up (1.9 s before it can fire).
// One take of 2.30 s: knocks and clacks 0.24 to 0.70, a loud rattle 0.75 to 0.89, the settling to 1.3, then a sharp click at 1.53 and a low hum to 1.9.
static const char* const kStingerDeploy = "fim92_stinger/fim92_stinger_deploy_wave";
// The AT4 thrown away (Operation Swordbreaker): the hotel roof's own sound of an AT4 dropped on it, in the level's own bundle (rooftopsniper): its scene
// plays it as Matkovic lets the AT4 fall for the player to take. One take of 5.9 s: its clatter's first strike is the clip.
static const char* const kRoofDropAt = "sound/levels/sp03/sfx/sp03_sfx_rooftop_dropat_wave";
static const Cut g_cuts[] = {
    {"mag_out", "an94", kAn94Long, 0.995, 1.225, "fairly sure", "the fresh magazine striking the old one off (no eject button: the flick); in both reloads; the bones show the old one leave at this moment of the short reload (1.2 s there, 0.99 here)"},
    {"mag_in", "an94", kAn94Long, 1.735, 1.92, "fairly sure", "the last click of the rock into the seat; in both reloads; the docs have it seated by 1.9 s of the short reload (1.93 there)"},
    {"lever", "an94", kAn94Long, 2.525, 2.755, "sure", "only in the empty reload, right after the magazine; a rising scrape to a stop at 2.67: the handle drawn"},
    {"bolt", "an94", kAn94Long, 2.805, 3.00, "sure", "only in the empty reload, next after the draw, across a 25 ms gap: let go, the bolt home"},

    {"mag_out", "rpk", kRpkLong, 0.58, 0.80, "fairly sure", "the first loud sound; in the short reload the same kind of sound is where the bones pull it off (no recording of an empty RPK); a different take there, by waveform"},
    {"mag_in", "rpk", kRpkLong, 2.335, 2.50, "fairly sure", "the clicks at 2.41; the short reload has its like at 2.30, as the bones seat it; a different take there, by waveform"},
    {"lever", "rpk", kRpkLong, 3.265, 3.46, "sure", "only in the empty reload, after the magazine: a scrape to a stop at 3.41"},
    {"bolt", "rpk", kRpkLong, 3.48, 3.69, "sure", "only in the empty reload, next after the draw, past a clear gap at 3.47"},

    {"mag_out", "aek971", kAekLong, 1.245, 1.44, "fairly sure", "the strike and the old magazine going (bone 3 leaps at 1.34 of the bones, about 0.11 s later than the wave); in both reloads"},
    {"mag_in", "aek971", kAekLong, 1.985, 2.13, "sure", "the seat click at 2.02; the bones have the fresh one home at 2.06; in both reloads"},
    {"lever", "aek971", kAekLong, 2.745, 2.97, "sure", "only in the empty reload; the lever bone leaves at 2.86 and is fully drawn at 2.95, the wave about 0.11 s ahead of the bones in this recording; to the tick at its end, as the AN-94's same take has it"},
    {"bolt", "aek971", kAekLong, 3.015, 3.17, "sure", "only in the empty reload; the lever bone springs back 3.19 to 3.23 of the bones, on the same lead"},

    {"mag_out", "ak74", kAkLong, 0.425, 0.60, "sure", "first loud sound; the bones start pulling the magazine off at 0.48 to 0.53; a close take in the short reload"},
    {"mag_in", "ak74", kAkLong, 1.555, 1.72, "sure", "the last click, 1.59 here, and the bones seat it 1.53 to 1.67"},
    {"lever", "ak74", kAkLong, 2.275, 2.465, "sure", "only in the empty reload; the bones draw the lever 2.36 to 2.45, about 0.08 s behind the wave"},
    {"bolt", "ak74", kAkLong, 2.505, 2.68, "sure", "only in the empty reload; the bones let it back 2.56 to 2.66, on the same lead"},

    {"mag_out", "g3a3", kG3Full, 1.355, 1.52, "sure", "the bones pull it down from 1.45; in the quick reload too, correlation over 0.9"},
    {"mag_in", "g3a3", kG3Full, 2.435, 2.60, "sure", "the bones seat it 2.5 to 2.59; in the quick reload too"},
    {"lever", "g3a3", kG3Full, 0.445, 0.615, "sure", "only in the full reload, which starts with the handle: the bones draw it from 0.45 to 0.55. (0.77 is it turning into the notch, not used.) Part of the same click is in the KH2002's magazine"},
    {"bolt", "g3a3", kG3Full, 3.025, 3.32, "sure", "only in the full reload; the last thing in it: the bones have the handle slapped down 3.08 to 3.17; the loudest sound of the wave"},

    // Semper Fidelis's two (2026-11-18), cut as the others were: the empty reload's wave against the shorter one (study.txt) and the bones of the gun's own recordings
    // (build\survey\AKS74u_empty.bin, UMP45_empty.bin; the wave about 0.08 s ahead of the bones, as the AK-74M's). By the numbers, not heard.
    {"mag_out", "aks74u", kAksLong, 0.345, 0.52, "fairly sure", "the first loud sound, 0.36 to 0.46; the bones rock the magazine off 0.44 to 0.55; a close take in the short reload (0.55 at 0.53)"},
    {"mag_in", "aks74u", kAksLong, 1.565, 1.70, "fairly sure", "the last click, 1.60 here (the loudest of the magazine's), and the bones seat it 1.53 to 1.64; in the short reload too (0.88 at 1.52)"},
    {"lever", "aks74u", kAksLong, 2.185, 2.365, "fairly sure", "only in the empty reload; the bones draw the bolt's handle 2.27 to 2.41, about 0.08 s behind the wave; to the valley at 2.36"},
    {"bolt", "aks74u", kAksLong, 2.375, 2.55, "fairly sure", "only in the empty reload, next after the draw; the bones let it home 2.41 to 2.51, on the same lead; the loudest at 2.46"},

    {"lever", "ump45", kUmpFull, 0.345, 0.505, "fairly sure", "only in the full reload, which starts with the handle: the bones draw it 0.41 to 0.55; its loudest at 0.41"},
    {"notch", "ump45", kUmpFull, 0.515, 0.665, "fairly sure", "only in the full reload, right after the draw: the bones turn the handle up into its notch 0.58 to 0.66; two clicks, 0.56 and 0.59"},
    {"mag_out", "ump45", kUmpFull, 0.995, 1.16, "fairly sure", "the clicks at 1.01 to 1.06; the bones draw the magazine down from 1.09; another take of it in the clip reload (0.40 to 0.49 there)"},
    {"mag_in", "ump45", kUmpFull, 1.895, 2.03, "fairly sure", "the last click, 1.97 here; the bones have it home 1.97 to 2.08; another take in the clip reload (1.31 to 1.36 there)"},
    {"bolt", "ump45", kUmpFull, 2.255, 2.48, "fairly sure", "only in the full reload; the last thing in it: the bones have the handle knocked down 2.30 to 2.40; the knock at 2.27, the bolt home 2.33 to 2.36, the loudest sound of the wave"},

    {"mag_out", "kh2002", kKhLong, 0.675, 0.89, "sure", "the bones start at 0.77 (0.74 in the recording's); correlation 0.94 in the recording, to the millisecond: see verify.txt"},
    {"mag_in", "kh2002", kKhLong, 2.495, 2.60, "sure", "the click at 2.54; the bones have it home at 2.5; both reloads"},
    {"lever", "kh2002", kKhLong, 3.205, 3.385, "sure", "only in the empty reload; the bones draw the hook from 3.27 to 3.36; correlation 0.11 in the recording, at its exact place but hardly above the noise: see verify.txt"},
    {"bolt", "kh2002", kKhLong, 3.465, 3.63, "sure", "only in the empty reload; the bones let it back 3.47 to 3.56; correlation 0.39 in the recording, at its exact place: see verify.txt"},

    {"mag_out", "m16a4", kM16Long, 0.2275, 0.595, "sure", "the same cut and level as the liked build\\sounds\\mag_out.wav"},
    {"mag_in", "m16a4", kM16Long, 1.285, 1.5122, "sure", "the same cut and level as the liked build\\sounds\\mag_in.wav"},
    {"lever", "m16a4", kG36Long, 2.365, 2.585, "BORROWED", "the M16's own reloads never pull its T handle, the hand slaps the bolt catch; the M27, HK417 and Mk11 show only the same catch pattern by timing; this is the G36C's handle, drawn straight back on top, same calibre, only in its empty reload; chosen by numbers, not heard"},
    {"bolt", "m16a4", kM16Long, 1.945, 2.235, "sure", "the same cut and level as the liked build\\sounds\\bolt.wav: the catch slapped and the bolt home; only in the long reload"},

    {"mag_out", "m416", kM16Long, 0.2275, 0.595, "sure", "the M416's sound graph plays the M16's reload (the game's own choice); the liked clip"},
    {"mag_in", "m416", kM16Long, 1.285, 1.5122, "sure", "the M416's sound graph plays the M16's reload; the liked clip"},
    {"lever", "m416", kG36Long, 2.365, 2.585, "BORROWED", "as the M16: the M416 has no pull of its own in the game's data; the G36C's handle"},
    {"bolt", "m416", kM16Long, 1.945, 2.235, "sure", "the M416's sound graph plays the M16's reload; the liked clip"},

    {"mag_out", "m4a1", kM4Long, 0.195, 0.53, "sure", "the same place as the M16's; the bones start pulling it at 0.13 to 0.4"},
    {"mag_in", "m4a1", kM4Long, 1.285, 1.47, "sure", "the click at 1.31; the bones have it in by 1.41"},
    {"lever", "m4a1", kG36Long, 2.365, 2.585, "BORROWED", "as the M16: its empty reload ends on the bolt catch, the T handle is never pulled; the G36C's handle"},
    {"bolt", "m4a1", kM4Long, 1.895, 2.13, "sure", "only in the long reload; the hand at the catch by the bones at 1.7 to 2.0: the catch slapped and the bolt home"},

    // The Mk11 (Operation Swordbreaker, 2026-11-18), cut as the others were: its empty reload's wave against the shorter one (study.txt) and the bones of its own recording
    // (build\survey\MK11_empty.bin: the reload begins 0.42 s into it, and the wave runs with the bones from there). By the numbers, not heard.
    {"mag_out", "mk11", kMk11Long, 0.215, 0.43, "fairly sure", "the first loud sound: the rises at 0.23 and 0.30 (-15 and -17 dB), died by 0.44; the bones have the magazine down out of the well at 0.25; in the short reload too (0.67 to 0.97 at the same place)"},
    {"mag_in", "mk11", kMk11Long, 1.205, 1.40, "fairly sure", "the fresh one up the well and its click, the loudest at 1.33 to 1.34 (-10 dB); the bones seat it 1.27 to 1.33; the short reload has its like at 1.34 to 1.45"},
    {"lever", "mk11", kG36Long, 2.365, 2.585, "BORROWED", "as the M16 and the M4: its own reloads never pull its T handle, the hand goes to the bolt catch; the G36C's handle"},
    {"bolt", "mk11", kMk11Long, 1.895, 2.17, "fairly sure", "only in the long reload: the catch pressed at 1.91 (the bones have the hand at it 1.5 to 1.95) and the bolt home 2.03 to 2.08 (-10 dB, the loudest with the magazine's click), died by 2.15"},

    {"lever", "m240", kM240, 0.695, 0.88, "sure", "the bones draw it 0.73 to 0.83; correlation 0.24 in the recording, at its exact place: see verify.txt"},
    {"bolt", "m240", kM240, 1.085, 1.30, "sure", "the bones let it fly 1.08 to 1.22, the click at 1.22; correlation 0.18 in the recording, at its exact place: see verify.txt"},
    {"lid_latch", "m240", kM240, 2.30, 2.38, "fairly sure", "the latch at 2.31, as the hand takes the lid: the bones open it from 2.38"},
    {"lid_up", "m240", kM240, 2.665, 2.79, "fairly sure", "the lid swinging to its stop at 2.69, played when it's up: the bones have it open by 2.75"},
    {"mag_out", "m240", kM240, 3.05, 3.24, "sure", "the box coming off: the bones move it from 3.11; the same click comes again at 5.27 in the wave (belt_lay)"},
    {"mag_in", "m240", kM240, 4.30, 4.50, "sure", "the box home with its click at 4.41: the bones have it seated at 4.3 to 4.53"},
    {"belt", "m240", kM240, 4.505, 4.99, "fairly sure", "a run of link rattles right after the box goes in, up to 10 kHz: the hand taking up the belt; nothing else moves then"},
    {"belt_lay", "m240", kM240, 5.175, 5.38, "fairly sure", "the belt laid in the feed: the bones' hand there 4.8 to 5.3; the clear gap before and after"},
    {"lid_shut", "m240", kM240, 5.625, 5.87, "sure", "the hand down on the lid at 5.63 and it shutting 5.70 to 5.74: the bones shut it 5.73 to 5.83; among the loudest in the wave"},

    {"aerial_fold", "fim92", kStinger, 2.035, 2.33, "unsure", "the panels' last run (the bones jump 1.97 to 2.13) and the click at 2.20 when they're flat; before it the hands are moving too, so the wave's 1.61 to 1.9 could be either: alternatives\\aerial_fold_long"},
    {"aerial_take", "fim92", kStinger, 2.385, 2.46, "fairly sure", "the hand reaching the folded aerial: the bones have it there at 2.42"},
    {"aerial_open", "fim92", kStinger, 2.465, 2.66, "fairly sure", "the bones spring it open 2.50 to 2.65, and the wave has its burst 2.50 to 2.57"},

    {"open", "m320", kM320Reload, 0.014, 0.235, "fairly sure", "the release's two clicks (0.008, 0.056) and the clack at the barrel's stop (0.116): the first sounds of the wave; the other take has the same three"},
    {"open_latch", "m320", kM320Reload, 0.014, 0.118, "fairly sure", "the release alone, played as it's pressed (a swing by hand takes its own time)"},
    {"open_stop", "m320", kM320Reload, 0.122, 0.235, "fairly sure", "the clack at the stop alone, played as the barrel gets there"},
    {"case_out", "m320", kM320Reload, 0.910, 1.205, "fairly sure", "the first sound after the first silence: a scrape and the tick (1.102) as it clears the chamber"},
    {"round_in", "m320", kM320Reload, 1.940, 2.140, "fairly sure", "the loudest of the middle take: the thunk at 2.074 in both takes, just before the round counts (2.175)"},
    {"close", "m320", kM320Reload, 2.462, 2.660, "sure", "the loudest of the wave: the barrel hitting the frame (2.472), then the latch (2.528); in both takes"},
    {"dry", "m320", kTriggerClick, 0.0, 0.208, "sure", "its own sound graph's dry click"},
    {"case_land_v0", "m320", kCase50Out, 0.0, 9.0, "sure", "its sound graph's own landing, take 0, at its middle pitch", 0, 0.725},
    {"case_land_v1", "m320", kCase50Out, 0.0, 9.0, "sure", "take 1", 1, 0.725},
    {"case_land_v2", "m320", kCase50Out, 0.0, 9.0, "sure", "take 2", 2, 0.725},
    {"case_land_v3", "m320", kCase50Out, 0.0, 9.0, "sure", "take 3", 3, 0.725},
    {"case_land_v4", "m320", kCase50Out, 0.0, 9.0, "sure", "take 4", 4, 0.725},
    {"case_land_v5", "m320", kCase50Out, 0.0, 9.0, "sure", "take 5", 5, 0.725},
    {"case_land_v6", "m320", kCase50Out, 0.0, 9.0, "sure", "take 6", 6, 0.725},
    {"case_land_v7", "m320", kCase50Out, 0.0, 9.0, "sure", "take 7", 7, 0.725},
    {"case_land_v8", "m320", kCase50Out, 0.0, 9.0, "sure", "take 8", 8, 0.725},
    {"case_land_v9", "m320", kCase50Out, 0.0, 9.0, "sure", "take 9", 9, 0.725},

    {"shell_in", "m1014", kM1014Tactical, 0.755, 0.835, "fairly sure", "the click of a shell past the latch in its tactical reload's loop: the loudest of that wave (0.77 to 0.79, -1 dB), from the quiet before it (0.76, -34 dB) to -45 by 0.83; the full reload's loop has its like at 2.14"},
    {"port_in", "m1014", kM1014Full, 0.965, 1.040, "fairly sure", "the shell dropped into the ejection port in its empty reload: two knocks at 0.98 and 1.00 (-10 dB), down to -72 by 1.04, before the release; the game's own hand lays it there at this moment (M1014_dry.bin)"},
    {"bolt", "m1014", kM1014Full, 1.105, 1.325, "sure", "the release pressed and the bolt slamming home, next after the port's shell across a clear silence: from 1.11 to its peak at 1.196 (the loudest of the wave), down to -40 by 1.30"},
    {"dry", "m1014", kTriggerClick, 0.0, 0.208, "unsure", "the game's shared dry click, as the M320's own sound graph plays it; the M1014's own graph wasn't read"},
    {"shell_land_v0", "m1014", kShellShotgunOut, 0.0, 9.0, "fairly sure", "the game's own shotgun shell landing (shellcasings_shotgun_outdoor: eight takes of 0.2 to 0.33 s), take 0, whole, at its own pitch; which of its indoor and outdoor sets the game plays where wasn't read: the outdoor one for every level", 0},
    {"shell_land_v1", "m1014", kShellShotgunOut, 0.0, 9.0, "fairly sure", "take 1", 1},
    {"shell_land_v2", "m1014", kShellShotgunOut, 0.0, 9.0, "fairly sure", "take 2", 2},
    {"shell_land_v3", "m1014", kShellShotgunOut, 0.0, 9.0, "fairly sure", "take 3", 3},
    {"shell_land_v4", "m1014", kShellShotgunOut, 0.0, 9.0, "fairly sure", "take 4", 4},
    {"shell_land_v5", "m1014", kShellShotgunOut, 0.0, 9.0, "fairly sure", "take 5", 5},
    {"shell_land_v6", "m1014", kShellShotgunOut, 0.0, 9.0, "fairly sure", "take 6", 6},
    {"shell_land_v7", "m1014", kShellShotgunOut, 0.0, 9.0, "fairly sure", "take 7", 7},

    {"land_dirt_v0", "loose", kWeaponOnDirt, 0.0, 9.0, "sure", "the game's own gun dropped on dirt, take 0"},
    {"land_dirt_v1", "loose", kWeaponOnDirt, 0.0, 9.0, "sure", "take 1", 1},
    {"land_dirt_v2", "loose", kWeaponOnDirt, 0.0, 9.0, "sure", "take 2", 2},
    {"land_dirt_v3", "loose", kWeaponOnDirt, 0.0, 9.0, "sure", "take 3", 3},
    {"land_tarmac_v0", "loose", kWeaponOnTarmac, 0.0, 9.0, "sure", "the game's own gun dropped on tarmac, take 0"},
    {"land_tarmac_v1", "loose", kWeaponOnTarmac, 0.0, 9.0, "sure", "take 1", 1},
    {"land_tarmac_v2", "loose", kWeaponOnTarmac, 0.0, 9.0, "sure", "take 2", 2},
    {"land_tarmac_v3", "loose", kWeaponOnTarmac, 0.0, 9.0, "sure", "take 3", 3},
    {"land_grass_v0", "loose", kWeaponOnGrass, 0.0, 9.0, "sure", "the game's own gun dropped on grass, take 0"},
    {"land_grass_v1", "loose", kWeaponOnGrass, 0.0, 9.0, "sure", "take 1", 1},
    {"land_grass_v2", "loose", kWeaponOnGrass, 0.0, 9.0, "sure", "take 2", 2},
    {"land_grass_v3", "loose", kWeaponOnGrass, 0.0, 9.0, "sure", "take 3", 3},
    {"land_wood_v0", "loose", kWeaponOnWood, 0.0, 9.0, "sure", "the game's own gun dropped on wood, take 0"},
    {"land_wood_v1", "loose", kWeaponOnWood, 0.0, 9.0, "sure", "take 1", 1},
    {"land_wood_v2", "loose", kWeaponOnWood, 0.0, 9.0, "sure", "take 2", 2},
    {"land_wood_v3", "loose", kWeaponOnWood, 0.0, 9.0, "sure", "take 3", 3},
    {"land_metal_v0", "loose", kWeaponOnMetal, 0.0, 9.0, "sure", "the game's own gun dropped on metal, take 0"},
    {"land_metal_v1", "loose", kWeaponOnMetal, 0.0, 9.0, "sure", "take 1", 1},
    {"land_metal_v2", "loose", kWeaponOnMetal, 0.0, 9.0, "sure", "take 2", 2},
    {"hit_metal_v0", "loose", kMetalRattly, 0.0, 9.0, "sure", "the game's generic rattly metal impact, take 0, at its impact pitch", 0, 1.1},
    {"hit_metal_v1", "loose", kMetalRattly, 0.0, 9.0, "sure", "take 1", 1, 1.1},
    {"hit_metal_v2", "loose", kMetalRattly, 0.0, 9.0, "sure", "take 2", 2, 1.1},
    {"hit_metal_v3", "loose", kMetalRattly, 0.0, 9.0, "sure", "take 3", 3, 1.1},
    {"hit_metal_v4", "loose", kMetalRattly, 0.0, 9.0, "sure", "take 4", 4, 1.1},
    {"hit_heavy_v0", "loose", kMetalHeavy, 0.0, 9.0, "sure", "the game's generic heavy metal impact, take 0, at its impact pitch (the launchers' and the M240's knocks)", 0, 1.1},
    {"hit_heavy_v1", "loose", kMetalHeavy, 0.0, 9.0, "sure", "take 1", 1, 1.1},
    {"hit_heavy_v2", "loose", kMetalHeavy, 0.0, 9.0, "sure", "take 2", 2, 1.1},
    {"hit_heavy_v3", "loose", kMetalHeavy, 0.0, 9.0, "sure", "take 3", 3, 1.1},
    {"hit_heavy_v4", "loose", kMetalHeavy, 0.0, 9.0, "sure", "take 4", 4, 1.1},
    {"hit_heavy_v5", "loose", kMetalHeavy, 0.0, 9.0, "sure", "take 5", 5, 1.1},
    {"hit_plastic_v0", "loose", kPlasticHollow, 0.0, 9.0, "sure", "the game's generic hollow plastic impact, take 0"},
    {"hit_plastic_v1", "loose", kPlasticHollow, 0.0, 9.0, "sure", "take 1", 1},
    {"hit_plastic_v2", "loose", kPlasticHollow, 0.0, 9.0, "sure", "take 2", 2},
    {"hit_plastic_v3", "loose", kPlasticHollow, 0.0, 9.0, "sure", "take 3", 3},
    {"hit_plastic_v4", "loose", kPlasticHollow, 0.0, 9.0, "sure", "take 4", 4},
    {"hit_plastic_v5", "loose", kPlasticHollow, 0.0, 9.0, "sure", "take 5", 5},

    {"take", "generic", kGenericDeploy, 0.010, 0.195, "fairly sure", "the opening grab of the game's own deploy sound, take 0: it rises from 0.01 (-37 dB) to its peak at 0.037 and is down to -39 by 0.19, before the gun's settling rises at 0.20; copied into build\\sounds as take.wav", 0},
    {"stow", "generic", kVestPoseChange, 0.190, 0.445, "unsure", "any gun put away at its place: take 0, the loudest of the six; from a dip in its rustle (0.19, -35 dB in 5 ms) through a knock at 0.22 to the thump at 0.255 (its peak, -1 dB, low: about 1.4 kHz), two small knocks after (0.36, 0.41), down to -49 by 0.445; chosen by numbers, not heard; build\\sounds\\stow.wav (--sounds stow)", 0},
    {"stow_settle", "generic", kGenericDeploy, 0.190, 0.610, "unsure", "the put-away's other sound (holsters.txt \"stowsound settle\"): the deploy's take 0 after its grab (take), the gun settling; from the quiet between them (0.19, -45 dB in 2.5 ms) through the tick at 0.22 and the rattles to 0.58, to -51 by 0.61, before the small knocks at 0.615; chosen by numbers, not heard; build\\sounds\\stow_settle.wav (--sounds stow_settle)", 0},
    {"draw", "fim92", kStingerDeploy, 0.225, 1.310, "fairly sure", "the Stinger taken back from its place: its own deploy from just before the first knock (0.245) through the clacks to the loudest at 0.48, the rattle 0.75 to 0.89 and the settling, down to -46 dB by 1.31; not the click at 1.53 and the hum after it that end the game's 1.9 s deploy, as a gun taken back by hand is ready at once; chosen by numbers, not heard; build\\sounds\\draw_fim92.wav (--sounds draw_fim92)"},
    {"drop", "at4", kRoofDropAt, 4.04, 4.26, "unsure", "the AT4 thrown away spent (its first landing, hands.cpp): the hotel roof's own sound of an AT4 let fall on it (Operation Swordbreaker's RooftopSniperRPG plays it as Matkovic drops the AT4 for the player; 5.9 s: handling at 1.2-1.5, a lone knock at 2.16, then a clatter from 4.06 to 4.75 s, a first strike and its bounces): the clatter's first strike alone (4.08, the loudest of the wave, -12.4 dB at its peak) to just before its first bounce (4.28), as loose plays the tube's own bounces after it; chosen by the numbers, not heard"},
};
static const Cut g_alternatives[] = {
    {"lever_scar_l", "m16", kScarLLong, 1.835, 2.00, "BORROWED", "the other candidate for the T handle: the SCAR-L's draw, crisper, also 5.56 and only in its empty reload, but its handle is on the side"},
    {"aerial_fold_long", "fim92", kStinger, 1.61, 2.33, "unsure", "the whole loud stretch while the panels fold, for a slower fold"},
    {"stow_pat", "generic", kVestPoseChange, 0.000, 0.300, "unsure", "a lighter put-away: take 2's start, a soft pat at 0.08 (-6 dB peak) and the rustle after it, down to -42 by 0.30, before its zip at 0.32", 2},
    {"draw_short", "fim92", kStingerDeploy, 0.225, 0.530, "unsure", "only the deploy's first knocks and its loudest clack (0.48), to the dip at 0.53: a grab nearer take.wav's length (0.31 s against its 0.19)"},
    {"draw_whole", "fim92", kStingerDeploy, 0.0, 9.0, "sure", "the whole deploy as the game plays it, its click at 1.53 and the hum to 1.9 included"},
};
// The mod's other clips, cut from the game's waves before this table existed and played by name ever since: the M416's generic reload (the M16's
// long reload, as its sound graph plays it; its pull the G36C's, borrowed with the user's yes), the train pistol's slide and magazine (the M9's
// reloads) and the launcher loaded by hand (the SMAW's reload). Each one's place and level as --fit found the clip there in the game's own wave, so a
// player's own copy of the game gives the same clips (--mod). Not in --final's report.
static const char* const kM9Long = "m9/m9_reload_long_wave";
static const char* const kM9Short = "m9/m9_reload_short_wave";
static const char* const kSmaw = "smaw/smaw_reload_wave";
static const Cut g_modClips[] = {
    {"mag_out", "generic", kM16Long, 0.2275, 0.595, "sure", "the M416's (reload.cpp) magazine out: mag_out.wav, the M16's long reload at 0.2275 s, correlation 0.9999 (final\\study.txt)"},
    {"mag_in", "generic", kM16Long, 1.285, 1.5122, "sure", "the M416's magazine in: mag_in.wav, at 1.285 s, correlation 0.9999"},
    {"bolt", "generic", kM16Long, 1.945, 2.235, "sure", "the M416's bolt home: bolt.wav, at 1.945 s, correlation 1.0000"},
    {"lever", "generic", kG36Long, 2.365, 2.585, "BORROWED", "the M416's charging handle (reload.cpp), and any gun's without a pull of its own: the G36C's, as lever_m416"},
    {"pistol_slide", "generic", kM9Long, 1.278, 1.525, "sure", "the train pistol's slide slamming home: pistol_slide.wav, the M9's long reload at 1.278 s, correlation 1.0000", 0, 1.0, -8.40},
    {"pistol_mag_out", "generic", kM9Short, 0.412, 0.740, "sure", "the train pistol's magazine out: pistol_mag_out.wav, the M9's short reload at 0.412 s, correlation 0.9999", 0, 1.0, -8.40},
    {"pistol_mag_in", "generic", kM9Short, 1.021, 1.236, "sure", "the train pistol's magazine in: pistol_mag_in.wav, the M9's short reload at 1.021 s, correlation 1.0000", 0, 1.0, -8.40},
    {"rocket_unlock", "generic", kSmaw, 1.72, 2.00, "sure", "the launcher's spent case unlocked (rocket.cpp): rocket_unlock.wav, correlation 0.9997", 0, 1.0, -4.58},
    {"rocket_out", "generic", kSmaw, 2.00, 2.28, "sure", "the spent case pulled off: rocket_out.wav, correlation 0.9999", 0, 1.0, -10.45},
    {"rocket_take", "generic", kSmaw, 2.76, 3.05, "sure", "a new case taken: rocket_take.wav, correlation 0.9998", 0, 1.0, -3.64},
    {"rocket_catch", "generic", kSmaw, 3.82, 3.90, "sure", "the case caught at the launcher's back: rocket_catch.wav, correlation 0.9997", 0, 1.0, 5.69},
    {"rocket_in", "generic", kSmaw, 4.12, 4.42, "sure", "the case pushed in: rocket_in.wav, correlation 1.0000", 0, 1.0, -10.17},
    {"rocket_lock", "generic", kSmaw, 5.39, 5.50, "sure", "the case locked: rocket_lock.wav, correlation 0.9999", 0, 1.0, 2.82},
};
static std::vector<double> cutClip(const std::vector<double>& wave, const Cut& c, double gainDb) {
    std::vector<double> v = slice(wave, c.from, c.to, 48000);
    if (c.pitch > 0.0 && c.pitch != 1.0 && v.size() > 1) {  // played slower (or faster), as the game's graph plays it: resampled, straight lines between samples
        std::vector<double> r(size_t(double(v.size()) / c.pitch));
        for (size_t i = 0; i < r.size(); ++i) {
            const double at = double(i) * c.pitch;
            const size_t k = std::min(size_t(at), v.size() - 2);
            const double f = at - double(k);
            r[i] = v[k] * (1.0 - f) + v[k + 1] * f;
        }
        v = std::move(r);
    }
    const double g = std::pow(10.0, gainDb / 20.0);
    const size_t in = size_t(48.0 * kFadeInMs), out = size_t(48.0 * kFadeOutMs);
    for (size_t i = 0; i < v.size(); ++i) {
        double f = g;
        if (i < in) f *= std::sin(1.5707963 * double(i) / double(in));
        if (i + out > v.size()) f *= std::sin(1.5707963 * double(v.size() - 1 - i) / double(out));
        v[i] *= f;
    }
    return v;
}
static std::string partnerOf(const std::string& w) {
    size_t p = w.find("_long_");
    if (p != std::string::npos) return w.substr(0, p) + "_short_" + w.substr(p + 6);
    p = w.find("_full_");
    if (p != std::string::npos) return w.substr(0, p) + (w.rfind("ump/", 0) == 0 ? "_clip_" : "_quick_") + w.substr(p + 6);  // (the UMP-45's shorter reload is its "clip" wave)
    return {};
}
static const std::vector<double>& waveCached(const std::string& name, int variation = 0) {
    static std::map<std::string, std::vector<double>> cache;
    const std::string key = name + "#" + std::to_string(variation);
    auto it = cache.find(key);
    if (it != cache.end()) return it->second;
    Decoded d;
    std::vector<double> v;
    const std::string full = name.rfind("sound/", 0) == 0 ? name : "sound/weapons/handheld/" + name;
    if (decodeByName(full, d, variation) && d.channels == 1 && d.rate == 48000) v = toMono(d);
    else printf("could not decode %s (take %d) %s\n", name.c_str(), variation, d.error.c_str());
    return cache[key] = std::move(v);
}
// The final clips, report.txt beside them.
static void writeFinal(const fs::path& outDir, double gainDb) {
    const fs::path dir = outDir / "final", alt = dir / "alternatives";
    fs::create_directories(alt);
    std::ofstream rep(dir / "report.txt", std::ios::binary);
    char m[600];
    sprintf_s(m, "# every clip decoded from the game's own data; nothing recorded. Mono, 48000 Hz, 16-bit; %.1f dB on the wave, %.0f ms fade in, %.0f ms fade out.\n", gainDb, kFadeInMs, kFadeOutMs);
    rep << m;
    rep << "# dB of full scale. peak; loudest 10 ms; active: the level of the windows within 30 dB of the clip's loudest; median of the 10 ms windows; 'short': the clip's best correlation in the same gun's partial reload (lever and bolt should be low, magazines high).\n";
    const fs::path sounds = outDir.parent_path() / "sounds";
    for (const char* f : {"bolt.wav", "mag_in.wav", "mag_out.wav"}) {
        std::vector<double> v;
        int rate = 0;
        if (readWavMono(sounds / f, v, rate)) {
            const Level l = levelOf(v, rate);
            sprintf_s(m, "# the liked %-11s %.3f s  peak %.1f  loudest %.1f  active %.1f  median %.1f\n", f, l.seconds, l.peak, l.loudest, l.active, l.median);
            rep << m;
        }
    }
    // The same cuts as the liked clips should come out the same: the largest difference, sample by sample, ignoring this tool's fades.
    for (const Cut& c : g_cuts) {
        if (std::string(c.shape) != "m16a4" || std::string(c.part) == "lever") continue;
        std::vector<double> liked;
        int rate = 0;
        if (!readWavMono(sounds / (std::string(c.part) + ".wav"), liked, rate)) continue;
        const std::vector<double> mine = cutClip(waveCached(c.wave), c, gainDb);
        const size_t skip = size_t(48.0 * kFadeInMs), keep = size_t(48.0 * kFadeOutMs);
        double worst = 0;
        size_t n = std::min(mine.size(), liked.size());
        for (size_t i = skip; i + keep < n; ++i) worst = std::max(worst, std::fabs(mine[i] - liked[i]));
        sprintf_s(m, "# %s_m16a4 against the liked %s.wav: %zu and %zu samples, largest difference %.1f of 32768\n", c.part, c.part, mine.size(), liked.size(), worst);
        rep << m;
    }
    auto writeOne = [&](const Cut& c, const fs::path& folder) {
        const std::vector<double>& wave = waveCached(c.wave, c.variation);
        if (wave.empty()) { rep << c.part << "_" << c.shape << "  NOT DECODED " << c.wave << "\n"; return; }
        const std::vector<double> v = cutClip(wave, c, gainDb);
        Decoded out;
        out.channels = 1; out.rate = 48000;
        out.pcm.resize(v.size());
        for (size_t i = 0; i < v.size(); ++i) out.pcm[i] = int16_t(std::clamp(std::lround(v[i]), -32768L, 32767L));
        const std::string name = std::string(c.part) + "_" + c.shape;
        writeWav(folder / (name + ".wav"), out);
        const Level l = levelOf(v, 48000);
        std::string shortNote = " short  - ";
        const std::string partner = partnerOf(c.wave);
        if (!partner.empty()) {
            const std::vector<double>& pw = waveCached(partner);
            if (!pw.empty()) {
                const Corr pc = corrSummary(nccAll(slice(wave, c.from, c.to, 48000), pw), 48000);
                char s[32];
                sprintf_s(s, " short %.2f", pc.peak);
                shortNote = s;
            }
        }
        sprintf_s(m, "%-19s %-46s %.3f-%.3f  %.3f s  peak %5.1f  loudest %5.1f  active %5.1f  median %5.1f %s  %-11s ", name.c_str(), c.wave, c.from, c.to, l.seconds, l.peak, l.loudest, l.active, l.median, shortNote.c_str(), c.sure);
        rep << m << c.why << "\n";
    };
    for (const Cut& c : g_cuts) writeOne(c, dir);
    rep << "\n# alternatives\\, by ear:\n";
    for (const Cut& c : g_alternatives) writeOne(c, alt);
    printf("wrote %zu clips and report.txt in %s\n", std::size(g_cuts) + std::size(g_alternatives), dir.string().c_str());
}
// The name the mod plays a cut by (sounds::Play, build\sounds\<name>.wav): its part and shape ("bolt_an94", "draw_fim92"), or its part alone
// for a cut that is no gun's own ("generic": take, stow, stow_settle).
static std::string soundsName(const Cut& c) {
    return std::string(c.shape) == "generic" ? std::string(c.part) : std::string(c.part) + "_" + c.shape;
}
// --sounds: the named clips cut as --final cuts them, written straight into the mod's build\sounds. Only the names asked for; a file there that already
// holds the same bytes is left as it is, so asking again for a clip the mod has loaded writes nothing (sounds.cpp reads a clip again when its time
// changes). How many names failed.
static const Cut* cutNamed(const std::string& name) {
    const Cut* cut = nullptr;
    for (const Cut& c : g_cuts)
        if (soundsName(c) == name) cut = &c;
    for (const Cut& c : g_modClips)
        if (!cut && soundsName(c) == name) cut = &c;
    return cut;
}
static int writeSounds(const fs::path& soundsDir, const std::vector<std::string>& names, double gainDb) {
    int failed = 0;
    char m[400];
    for (const std::string& want : names) {
        const Cut* cut = cutNamed(want);
        if (!cut) { printf("--sounds %s: no cut in g_cuts or g_modClips is played by that name\n", want.c_str()); ++failed; continue; }
        const std::vector<double>& wave = waveCached(cut->wave, cut->variation);
        if (wave.empty()) { printf("--sounds %s: %s (take %d) not decoded\n", want.c_str(), cut->wave, cut->variation); ++failed; continue; }
        const double rowGain = std::isnan(cut->gain) ? gainDb : cut->gain;  // (g_modClips: the older clip's own level)
        const std::vector<double> v = cutClip(wave, *cut, rowGain);
        Decoded out;
        out.channels = 1; out.rate = 48000;
        out.pcm.resize(v.size());
        for (size_t i = 0; i < v.size(); ++i) out.pcm[i] = int16_t(std::clamp(std::lround(v[i]), -32768L, 32767L));
        const std::vector<u8> bytes = wavBytes(out);
        const fs::path file = soundsDir / (want + ".wav");
        std::vector<u8> there;
        std::error_code ec;
        if (fs::exists(file, ec)) {
            try { there = readAll(file); } catch (const std::exception&) { there.clear(); }
        }
        const char* done = "written";
        if (!there.empty() && there == bytes) done = "already there with the same bytes: not written";
        else {
            fs::create_directories(soundsDir, ec);
            std::ofstream o(file, std::ios::binary);
            o.write((const char*)bytes.data(), std::streamsize(bytes.size()));
            o.close();  // (a write that fails only as it's flushed counts too)
            if (o.fail()) { done = "COULD NOT BE WRITTEN"; ++failed; }
        }
        const Level l = levelOf(v, 48000);
        sprintf_s(m, "%s.wav: %s take %d, %.3f-%.3f s at %.1f dB: %.3f s  peak %.1f  loudest %.1f  active %.1f  median %.1f; %s\n", want.c_str(), cut->wave, cut->variation, cut->from,
                  cut->to, rowGain, l.seconds, l.peak, l.loudest, l.active, l.median, done);
        printf("%s", m);
    }
    printf("--sounds: %zu asked for, %d failed, in %s\n", names.size(), failed, shown(soundsDir).c_str());
    return failed;
}
// Which sound is which, against the two recordings of the game's own reloads: only their correlation is read, nothing of them goes into any clip.
// Each clip's cut against each recording: raw and with the battlefield's rumble taken out by a first difference. Height: the peak over the best anywhere else, and over the spread of all the places.
static void verify(const fs::path& outDir) {
    const fs::path dir = outDir / "final", sv = outDir.parent_path() / "survey";
    fs::create_directories(dir);
    std::ofstream o(dir / "verify.txt", std::ios::binary);
    struct Rec { const char* file; const char* shape; const char* wave; double bonesToWav; std::vector<double> v; std::vector<double> sharp; };
    Rec recs[] = {{"lever_kh2002.wav", "kh2002", kKhLong, 0.3, {}, {}}, {"lever_m240.wav", "m240", kM240, 0.3, {}, {}}};
    char m[400];
    for (Rec& r : recs) {
        int rate = 0;
        if (!readWavMono(sv / r.file, r.v, rate) || rate != 48000) { o << r.file << " unreadable\n"; continue; }
        r.sharp = sharpened(r.v);
        const Level l = levelOf(r.v, rate);
        sprintf_s(m, "%s: %.2f s, peak %.1f, median %.1f\n", r.file, l.seconds, l.peak, l.median);
        o << m;
    }
    auto line = [&](const std::string& label, const std::vector<double>& clip, const Rec& r, double wantAt) {
        const std::vector<double> sc = sharpened(clip);
        const Corr raw = corrSummary(nccAll(clip, r.v), 48000), sh = corrSummary(nccAll(sc, r.sharp), 48000);
        sprintf_s(m, "  %-28s in %-17s raw %.3f at %6.3f (next %.3f at %6.3f, %4.1f x spread) | sharp %.3f at %6.3f (next %.3f at %6.3f, %4.1f x spread)", label.c_str(), r.file, raw.peak, raw.peakAt, raw.second, raw.secondAt, raw.spread > 0 ? raw.peak / raw.spread : 0, sh.peak, sh.peakAt, sh.second, sh.secondAt,
                  sh.spread > 0 ? sh.peak / sh.spread : 0);
        o << m;
        if (wantAt >= 0) { sprintf_s(m, " | wanted near %.3f", wantAt); o << m; }
        o << "\n";
    };
    // The whole reload wave first: where it starts in the recording. From there every part has one place to be.
    o << "\n== the whole reload wave, each recording; a right gun against a wrong one\n";
    for (const Rec& r : recs) {
        if (r.v.empty()) continue;
        line("kh2002 long", waveCached(kKhLong), r, -1);
        line("m240", waveCached(kM240), r, -1);
    }
    for (const Rec& r : recs) {
        if (r.v.empty()) continue;
        const std::vector<double>& own = waveCached(r.wave);
        const std::vector<double> sc = sharpened(own);
        const Corr whole = corrSummary(nccAll(sc, r.sharp), 48000);
        sprintf_s(m, "\n== %s: the gun's own parts; the wave starts at %.3f in the recording (sharp correlation of the whole wave), so each part is wanted at that plus its time in the wave\n", r.file, whole.peakAt);
        o << m;
        for (const Cut& c : g_cuts) {
            if (std::string(c.shape) != r.shape) continue;
            const std::vector<double> clip = slice(own, c.from, c.to, 48000);
            line(std::string(c.part) + " " + std::to_string(c.from).substr(0, 5), clip, r, whole.peakAt + c.from);
        }
        o << "-- and for comparison, the same part from other guns, in this recording\n";
        for (const Cut& c : g_cuts) {
            if (std::string(c.shape) == r.shape || (std::string(c.part) != "lever" && std::string(c.part) != "bolt")) continue;
            const std::vector<double>& w = waveCached(c.wave);
            if (w.empty()) continue;
            line(std::string(c.part) + " " + c.shape, slice(w, c.from, c.to, 48000), r, -1);
        }
    }
    printf("wrote %s\n", (dir / "verify.txt").string().c_str());
}

static bool wantedEbx(const std::string& low, bool all, const std::string& dump) {
    if (all) return true;
    if (!dump.empty() && holds(low, dump.c_str())) return true;
    return holds(low, "sound") || holds(low, "audio") || holds(low, "weapon");
}

// A string under a registry key, or empty. view: KEY_WOW64_32KEY for the 32-bit programs' view, or 0.
static std::wstring registryString(HKEY root, const wchar_t* key, const wchar_t* name, REGSAM view) {
    HKEY k = nullptr;
    if (RegOpenKeyExW(root, key, 0, KEY_READ | view, &k) != ERROR_SUCCESS) return {};
    wchar_t value[1024] = {};
    DWORD size = sizeof(value) - sizeof(wchar_t), type = 0;
    const bool ok = RegQueryValueExW(k, name, nullptr, &type, reinterpret_cast<BYTE*>(value), &size) == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ);
    RegCloseKey(k);
    return ok ? value : std::wstring();
}
// A folder that holds the game's data (its catalogue).
static bool isGameFolder(const fs::path& p) {
    std::error_code ec;
    return !p.empty() && fs::exists(p / "Data" / "cas.cat", ec);
}
// The game's folder as it was installed: the EA app's (and Steam's, through it) "Install Dir", else Steam's own libraries
// (libraryfolders.vdf: each library's appmanifest_1238820.acf, Battlefield 3's, and its installdir). Empty: not found.
static fs::path findGame(std::string& how) {
    std::wstring dir = registryString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\EA Games\\Battlefield 3", L"Install Dir", KEY_WOW64_32KEY);
    if (isGameFolder(dir)) { how = "the registry's Install Dir"; return dir; }
    const std::wstring steam = registryString(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath", 0);
    if (steam.empty()) return {};
    std::vector<fs::path> libraries = {fs::path(steam)};
    try {
        // "path"		"D:\\SteamLibrary": each library's folder, its backslashes doubled.
        const std::vector<u8> vdf = readAll(fs::path(steam) / "steamapps" / "libraryfolders.vdf");
        const std::string text(vdf.begin(), vdf.end());
        for (size_t at = text.find("\"path\""); at != std::string::npos; at = text.find("\"path\"", at + 6)) {
            const size_t open = text.find('"', at + 6);
            if (open == std::string::npos) break;
            std::string path;
            size_t e = open + 1;
            for (; e < text.size() && text[e] != '"'; ++e) {
                if (text[e] == '\\' && e + 1 < text.size()) ++e;
                path += text[e];
            }
            // (The file is UTF-8: a library's name outside ASCII taken as such.)
            const int n = MultiByteToWideChar(CP_UTF8, 0, path.data(), int(path.size()), nullptr, 0);
            std::wstring wide(size_t(std::max(n, 0)), L'\0');
            if (n > 0) MultiByteToWideChar(CP_UTF8, 0, path.data(), int(path.size()), wide.data(), n);
            libraries.push_back(fs::path(wide));
            at = e;
        }
    } catch (const std::exception&) {}
    for (const fs::path& lib : libraries) {
        std::string installDir = "Battlefield 3";
        try {
            const std::vector<u8> acf = readAll(lib / "steamapps" / "appmanifest_1238820.acf");
            const std::string text(acf.begin(), acf.end());
            const size_t at = text.find("\"installdir\"");
            const size_t open = at == std::string::npos ? std::string::npos : text.find('"', at + 12);
            const size_t close = open == std::string::npos ? std::string::npos : text.find('"', open + 1);
            if (close != std::string::npos) installDir = text.substr(open + 1, close - open - 1);
        } catch (const std::exception&) {}
        const fs::path candidate = lib / "steamapps" / "common" / installDir;
        if (isGameFolder(candidate)) { how = "Steam's library"; return candidate; }
    }
    return {};
}

int wmain(int argc, wchar_t** argv) {
    SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
    SetPriorityClass(GetCurrentProcess(), PROCESS_MODE_BACKGROUND_BEGIN);

    fs::path game;
    wchar_t self[MAX_PATH];
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    fs::path outDir = fs::path(self).parent_path() / "gamesounds";
    bool all = false, modOnly = false, missingOnly = false, listOnly = false;
    std::string dumpText, rawText;
    std::vector<std::string> wavTexts, checkTexts, soundsNames;
    std::vector<std::pair<fs::path, std::string>> fits;
    bool reloads = false, doStudy = false, doTrace = false, doFinal = false, doVerify = false;
    double finalGain = kFinalGainDb;
    auto narrow = [](const wchar_t* w) { std::string s; for (; *w; ++w) s += char(*w); return lower(s); };
    for (int i = 1; i < argc; ++i) {
        std::wstring a = argv[i];
        if (a == L"--all") all = true;
        else if (a == L"--mod") modOnly = true;
        else if (a == L"--missing") modOnly = missingOnly = true;
        else if (a == L"--list") listOnly = true;
        else if (a == L"--fit" && i + 2 < argc) { fits.push_back({fs::path(argv[i + 1]), narrow(argv[i + 2])}); i += 2; }
        else if (a == L"--reloads") reloads = true;
        else if (a == L"--study") doStudy = true;
        else if (a == L"--trace") doTrace = true;
        else if (a == L"--final") doFinal = true;
        else if (a == L"--verify") doVerify = true;
        else if (a == L"--gain" && i + 1 < argc) finalGain = _wtof(argv[++i]);
        else if (a == L"--wav" && i + 1 < argc) wavTexts.push_back(narrow(argv[++i]));
        else if (a == L"--check" && i + 1 < argc) checkTexts.push_back(narrow(argv[++i]));
        else if (a == L"--sounds" && i + 1 < argc) soundsNames.push_back(narrow(argv[++i]));
        else if (a == L"--raw" && i + 1 < argc) {
            std::wstring w = argv[++i];
            for (wchar_t c : w) rawText += char(c);
            rawText = lower(rawText);
        }
        else if (a == L"--game" && i + 1 < argc) game = argv[++i];
        else if (a == L"--out" && i + 1 < argc) outDir = argv[++i];
        else if (a == L"--dump" && i + 1 < argc) {
            std::wstring w = argv[++i];
            for (wchar_t c : w) dumpText += char(c);
            dumpText = lower(dumpText);
        }
    }
    // --sounds writes into the mod's own folder of clips: build\sounds beside this program (and beside the DLL), whatever --out says.
    const fs::path soundsDir = fs::path(self).parent_path() / "sounds";
    // Every clip the mod plays, each name once (a name in both tables is g_cuts' row: cutNamed).
    std::vector<std::string> modNames;
    auto addName = [&](const Cut& c) {
        const std::string name = soundsName(c);
        if (std::find(modNames.begin(), modNames.end(), name) == modNames.end()) modNames.push_back(name);
    };
    for (const Cut& c : g_cuts) addName(c);
    for (const Cut& c : g_modClips) addName(c);
    if (listOnly) {
        for (const std::string& name : modNames) printf("%s\n", name.c_str());
        return 0;
    }
    // --missing: the clips not there yet (or cut short: no more than a wave file's header), found before the game is looked for.
    size_t modThere = 0;
    if (modOnly) {
        for (const std::string& name : modNames) {
            std::error_code ec;
            const fs::path file = soundsDir / (name + ".wav");
            if (missingOnly && fs::is_regular_file(file, ec) && fs::file_size(file, ec) > 44 && !ec) { ++modThere; continue; }
            soundsNames.push_back(name);
        }
        if (missingOnly && soundsNames.empty()) {
            printf("--missing: all %zu clips the mod plays are there in %s: nothing made\n", modNames.size(), shown(soundsDir).c_str());
            return 0;
        }
        if (missingOnly) {
            printf("--missing: %zu of the %zu clips the mod plays aren't there yet:", soundsNames.size(), modNames.size());
            for (const std::string& name : soundsNames) printf(" %s", name.c_str());
            printf("\n");
        }
    }
    if (game.empty()) {
        std::string how;
        game = findGame(how);
        if (game.empty()) {
            printf("Battlefield 3's folder wasn't found (neither the EA app's Install Dir in the registry nor Steam's libraries hold it): give it with --game <folder>\n");
            return 3;
        }
        printf("the game: %s (%s)\n", shown(game).c_str(), how.c_str());
    } else if (!isGameFolder(game)) {
        printf("%s holds no Data\\cas.cat: not Battlefield 3's folder\n", shown(game).c_str());
        return 3;
    }
    // Inside the game's folder (or the same): nothing is ever written there.
    auto inside = [&](const fs::path& p) {
        std::wstring a = p.wstring(), g = game.wstring();
        for (wchar_t& c : a) c = towlower(c);
        for (wchar_t& c : g) c = towlower(c);
        while (!g.empty() && (g.back() == L'\\' || g.back() == L'/')) g.pop_back();
        return a.find(g) != std::wstring::npos;
    };
    if (!modOnly) {
        if (inside(outDir)) { printf("refusing to write inside the game's folder\n"); return 1; }
        fs::create_directories(outDir);
    }
    if (!soundsNames.empty() && inside(soundsDir)) { printf("refusing to write inside the game's folder\n"); return 1; }
    // Every clip the mod plays is at the table's one gain (take.wav among them): another gain is for listening in final\, never for its folder.
    if (!soundsNames.empty() && finalGain != kFinalGainDb) {
        printf("--sounds: the mod's clips are cut at the table's %.1f dB only (--gain is for --final): nothing written\n", kFinalGainDb);
        return 2;
    }

    // The patch comes first: where both hold a thing, the patched one is what the game plays.
    if (fs::exists(game / "Update" / "Patch" / "Data")) g_roots.push_back({game / "Update" / "Patch" / "Data", "patch", {}});
    g_roots.push_back({game / "Data", "base", {}});
    for (u32 r = 0; r < g_roots.size(); ++r) {
        size_t c = loadCatalogue(r);
        printf("%s cas.cat: %zu entries\n", g_roots[r].label.c_str(), c);
    }

    // Every superbundle, the patched version first.
    std::map<std::string, std::vector<std::pair<u32, fs::path>>> tocs;
    for (u32 r = 0; r < g_roots.size(); ++r) {
        fs::path win = g_roots[r].data / "Win32";
        if (!fs::exists(win)) continue;
        for (auto& en : fs::recursive_directory_iterator(win)) {
            if (!en.is_regular_file() || lower(en.path().extension().string()) != ".toc") continue;
            std::string key = lower(fs::relative(en.path(), win).replace_extension("").generic_string());
            tocs[key].push_back({r, en.path()});
        }
    }
    for (auto& t : tocs) {
        std::set<std::string> seen;
        for (auto& rp : t.second) {
            try { readSuperbundle(rp.second, t.first, rp.first, seen); }
            catch (const std::exception& e) { printf("%s: %s\n", rp.second.string().c_str(), e.what()); }
        }
    }
    printf("superbundles %zu, bundles %zu, ebx names %zu, chunk ids %zu, name conflicts %zu, ebx without sha1 %zu, chunks without sha1 %zu\n", tocs.size(), g_bundleNames.size(),
           g_ebx.size(), g_chunks.size(), g_nameConflicts, g_ebxNoSha, g_noncasChunks);

    // Pull the assets, in the order they lie on the disk.
    struct Job { CatEntry c; const EbxRef* r; };
    std::vector<Job> jobs;
    for (auto& kv : g_ebx) {
        if (!wantedEbx(kv.first, all, dumpText)) continue;
        auto it = g_cat.find(kv.second.sha);
        if (it == g_cat.end()) { ++g_ebxMissing; continue; }
        jobs.push_back({it->second, &kv.second});
    }
    std::sort(jobs.begin(), jobs.end(), [](const Job& a, const Job& b) {
        if (a.c.root != b.c.root) return a.c.root < b.c.root;
        if (a.c.cas != b.c.cas) return a.c.cas < b.c.cas;
        return a.c.offset < b.c.offset;
    });
    printf("reading %zu assets (%zu not in any catalogue)\n", jobs.size(), size_t(g_ebxMissing));
    fflush(stdout);
    std::vector<u8> raw, plain;
    std::vector<const EbxRef*> toDump;
    for (size_t j = 0; j < jobs.size(); ++j) {
        const Job& job = jobs[j];
        if (j % 5000 == 0) { printf("  %zu of %zu\n", j, jobs.size()); fflush(stdout); }
        if (!readRaw(job.c, job.c.size, raw)) { ++g_ebxBad; continue; }
        g_bytesRead += raw.size();
        plain.clear();
        try {
            payloadOf(raw, plain, size_t(256) << 20);
            Ebx e;
            if (!e.parse(plain.data(), plain.size())) {
                ++g_ebxBad;
                if (g_firstBad.empty()) g_firstBad = job.r->name + ": " + hexOf(plain.data(), std::min<size_t>(plain.size(), 48), true);
                continue;
            }
            ++g_ebxRead;
            if (e.endOfInstances > e.arrayStart || e.arrayStart - e.endOfInstances > 16) {  // up to 16 bytes of padding before the arrays
                ++g_ebxShort;
                if (g_firstShort.empty()) { char m[80]; sprintf_s(m, ": instances end %zu, arrays start %zu", e.endOfInstances, e.arrayStart); g_firstShort = job.r->name + m; }
            }
            keepFile(e, *job.r);
            if (!dumpText.empty() && holds(lower(job.r->name), dumpText.c_str())) toDump.push_back(job.r);
        } catch (const std::exception& ex) {
            ++g_ebxBad;
            if (g_firstBad.empty()) g_firstBad = job.r->name + ": " + ex.what();
        }
    }
    printf("read %llu assets, %llu unreadable, %.1f MB\n", (unsigned long long)g_ebxRead, (unsigned long long)g_ebxBad, double(g_bytesRead) / 1048576.0);
    if (!g_firstBad.empty()) printf("first unreadable: %s\n", g_firstBad.c_str());
    if (g_ebxShort) printf("%llu assets whose instances do not end where the arrays start, first: %s\n", (unsigned long long)g_ebxShort, g_firstShort.c_str());

    // The readable dumps come after, once every name is known.
    if (!toDump.empty()) {
        fs::path dumpDir = outDir / "ebx";
        fs::create_directories(dumpDir);
        for (const EbxRef* r : toDump) {
            auto it = g_cat.find(r->sha);
            if (it == g_cat.end() || !readRaw(it->second, it->second.size, raw)) continue;
            plain.clear();
            try {
                payloadOf(raw, plain, size_t(256) << 20);
                Ebx e;
                if (!e.parse(plain.data(), plain.size())) continue;
                std::string fn = r->name;
                for (char& c : fn) if (c == '/' || c == '\\' || c == ':') c = '_';
                std::ofstream o(dumpDir / (fn + ".txt"), std::ios::binary);
                dumpFile(o, e, r->name);
            } catch (const std::exception&) {}
        }
        printf("dumped %zu assets to %s\n", toDump.size(), dumpDir.string().c_str());
    }

    // The head of every sound chunk.
    std::map<Guid, ChunkHead> heads;
    for (const ParsedFile& pf : g_files)
        for (const Wave& w : pf.waves) {
            for (const WaveChunk& c : w.chunks) heads.try_emplace(c.id);
            // The audio starts at the first segment: a seek table may stand before it.
            for (const WaveVariation& v : w.variations)
                if (v.chunkIndex < w.chunks.size())
                    for (const WaveSegment& s : v.segments) {
                        ChunkHead& h = heads[w.chunks[v.chunkIndex].id];
                        h.audioOff = std::min(h.audioOff, s.samplesOffset);
                    }
        }
    struct HeadJob { CatEntry c; ChunkHead* h; };
    std::vector<HeadJob> headJobs;
    size_t chunkNoId = 0, chunkNoCat = 0, chunkInSb = 0;
    for (auto& kv : heads) {
        if (kv.second.audioOff == 0xFFFFFFFFu) kv.second.audioOff = 0;
        auto it = g_chunks.find(kv.first);
        if (it == g_chunks.end()) { ++chunkNoId; continue; }
        if (!it->second.sb.empty()) {
            // Chunks without a sha1 lie in the sb beside their toc (the English speech, the expansions' patches).
            ChunkHead& h = kv.second;
            h.found = true;
            h.patched = it->second.patched;
            h.stored = it->second.sbSize;
            h.offset = it->second.sbOff;
            h.sbFile = it->second.sb.filename().string();
            ++chunkInSb;
            std::vector<u8> part;
            if (readFilePart(it->second.sb, it->second.sbOff, std::min<u64>(it->second.sbSize, u64(h.audioOff) + 64), part)) {
                h.read = true;
                h.firstLen = u32(std::min<size_t>(part.size(), sizeof h.first));
                memcpy(h.first, part.data(), h.firstLen);
                if (part.size() > h.audioOff) {
                    h.audioLen = u32(std::min<size_t>(part.size() - h.audioOff, sizeof h.audio));
                    memcpy(h.audio, part.data() + h.audioOff, h.audioLen);
                }
                parseHead(h);
            }
            continue;
        }
        auto ct = g_cat.find(it->second.sha);
        if (ct == g_cat.end()) { ++chunkNoCat; continue; }
        kv.second.found = true;
        kv.second.patched = it->second.patched;
        kv.second.stored = ct->second.size;
        kv.second.cas = ct->second.cas;
        kv.second.root = ct->second.root;
        kv.second.offset = ct->second.offset;
        headJobs.push_back({ct->second, &kv.second});
    }
    std::sort(headJobs.begin(), headJobs.end(), [](const HeadJob& a, const HeadJob& b) {
        if (a.c.root != b.c.root) return a.c.root < b.c.root;
        if (a.c.cas != b.c.cas) return a.c.cas < b.c.cas;
        return a.c.offset < b.c.offset;
    });
    printf("sound chunks: %zu, in the CAS %zu, in an sb %zu, id in no toc or bundle %zu, sha1 in no catalogue %zu\n", heads.size(), headJobs.size(), chunkInSb, chunkNoId, chunkNoCat);
    fflush(stdout);
    for (HeadJob& hj : headJobs) {
        if (!readRaw(hj.c, u32(std::min<u64>(u64(hj.h->audioOff) + 64, 1u << 20)), raw)) continue;
        // Sound chunks stand bare in the archive, not in compression blocks: the declared and stored sizes are the same.
        hj.h->read = true;
        hj.h->firstLen = u32(std::min<size_t>(raw.size(), sizeof hj.h->first));
        if (hj.h->firstLen) memcpy(hj.h->first, raw.data(), hj.h->firstLen);
        if (raw.size() > hj.h->audioOff) {
            hj.h->audioLen = u32(std::min<size_t>(raw.size() - hj.h->audioOff, sizeof hj.h->audio));
            memcpy(hj.h->audio, raw.data() + hj.h->audioOff, hj.h->audioLen);
        }
        parseHead(*hj.h);
    }
    // --raw: the chunks of the matching waves, whole, as they stand in the archive, each walked block by block as a check.
    if (!rawText.empty()) {
        fs::path rawDir = outDir / "raw";
        fs::create_directories(rawDir);
        std::ofstream rep(rawDir / "walk.txt", std::ios::binary);
        size_t files = 0, good = 0;
        for (const ParsedFile& pf : g_files) {
            if (!holds(lower(pf.ref->name), rawText.c_str())) continue;
            for (const Wave& w : pf.waves) {
                for (size_t ci = 0; ci < w.chunks.size(); ++ci) {
                    const ChunkHead& h = heads[w.chunks[ci].id];
                    if (!h.found) { rep << pf.ref->name << " chunk " << ci << " NOT FOUND\n"; continue; }
                    std::vector<u8> whole;
                    auto ck = g_chunks.find(w.chunks[ci].id);
                    bool ok = false;
                    if (!ck->second.sb.empty()) ok = readFilePart(ck->second.sb, ck->second.sbOff, ck->second.sbSize, whole);
                    else { auto ct = g_cat.find(ck->second.sha); ok = readRaw(ct->second, ct->second.size, whole); }
                    if (!ok) { rep << pf.ref->name << " chunk " << ci << " UNREADABLE\n"; continue; }
                    std::string fn = pf.ref->name;
                    for (char& c : fn) if (c == '/' || c == '\\' || c == ':') c = '_';
                    fn += w.chunks.size() > 1 ? "." + std::to_string(ci) + ".sps" : ".sps";
                    std::ofstream(rawDir / fn, std::ios::binary).write((const char*)whole.data(), std::streamsize(whole.size()));
                    ++files;
                    rep << pf.ref->name << " chunk " << ci << " " << whole.size() << " bytes";
                    if (h.audioOff) rep << ", seek table " << hexOf(whole.data(), std::min<size_t>(h.audioOff, whole.size()), true);
                    rep << "\n";
                    for (size_t vi = 0; vi < w.variations.size(); ++vi) {
                        const WaveVariation& v = w.variations[vi];
                        if (v.chunkIndex != ci) continue;
                        for (const WaveSegment& s : v.segments) {
                            size_t at = s.samplesOffset;
                            u32 heads_ = 0, datas = 0, samples = 0, ends = 0, declared = 0, rate = 0;
                            bool bad = false;
                            while (at + 4 <= whole.size()) {
                                u8 id = whole[at];
                                u32 size = be32(&whole[at]) & 0xFFFFFF;
                                if (size < 4 || at + size > whole.size()) { bad = true; break; }
                                if (id == 0x48 && size >= 12) { ++heads_; rate = be32(&whole[at + 4]) & 0x3FFFF; declared = be32(&whole[at + 8]) & 0x1FFFFFFF; }
                                else if (id == 0x44 && size >= 8) { ++datas; samples += be32(&whole[at + 4]); }
                                else if (id == 0x45) { ++ends; at += size; break; }
                                else { bad = true; break; }
                                at += size;
                            }
                            bool match = !bad && ends == 1 && samples == declared;
                            if (match) ++good;
                            char m[220];
                            sprintf_s(m, "    var %zu @%u: %u header, %u data, %u end, %u samples (header says %u) at %u Hz = %.3fs (asset says %.3fs), stops at %zu%s\n", vi, s.samplesOffset, heads_, datas, ends,
                                      samples, declared, rate, rate ? double(samples) / rate : 0.0, double(s.seconds), at, match ? "" : "  MISMATCH");
                            rep << m;
                        }
                    }
                }
            }
        }
        printf("raw chunks written: %zu, segments that walk clean to their end: %zu (raw\\walk.txt)\n", files, good);
    }
    // --wav, --reloads, --check: the audio itself. In the order the chunks lie on the disk.
    if (reloads || !wavTexts.empty() || !checkTexts.empty()) {
        std::set<std::string> exact;
        std::vector<std::string> absent;
        if (reloads) {
            for (const char* w : g_reloadWaves) exact.insert(std::string("sound/weapons/handheld/") + w);
            for (const std::string& n : exact) {
                bool found = false;
                for (const ParsedFile& pf : g_files) if (lower(pf.ref->name) == n && !pf.waves.empty()) { found = true; break; }
                if (!found) absent.push_back(n);
            }
        }
        auto holdsAny = [](const std::string& low, const std::vector<std::string>& texts) {
            for (const std::string& t : texts) if (holds(low, t.c_str())) return true;
            return false;
        };
        std::vector<DecodeJob> decodeJobs;
        std::vector<std::string> failures;
        for (const ParsedFile& pf : g_files) {
            const std::string low = lower(pf.ref->name);
            const bool write = exact.count(low) || holdsAny(low, wavTexts);
            if (!write && !holdsAny(low, checkTexts)) continue;
            std::string base = pf.ref->name;
            for (char& c : base) if (c == '/' || c == '\\' || c == ':') c = '_';
            for (size_t wi = 0; wi < pf.waves.size(); ++wi) {
                const Wave& w = pf.waves[wi];
                if (w.variations.empty()) failures.push_back(pf.ref->name + "  FAILED: no variations");
                for (size_t vi = 0; vi < w.variations.size(); ++vi) {
                    const WaveVariation& v = w.variations[vi];
                    std::string file = base;
                    if (pf.waves.size() > 1) file += "_w" + std::to_string(wi);
                    if (w.variations.size() > 1) file += "_v" + std::to_string(vi);
                    if (v.chunkIndex >= w.chunks.size() || !heads[w.chunks[v.chunkIndex].id].found) {
                        failures.push_back(file + "  FAILED: its chunk is not in the data");
                        continue;
                    }
                    const ChunkHead& h = heads[w.chunks[v.chunkIndex].id];
                    decodeJobs.push_back({&pf, &w, &v, file, write, h.sbFile.empty() ? h.root : 99u, h.cas, h.offset});
                }
            }
        }
        std::sort(decodeJobs.begin(), decodeJobs.end(), [](const DecodeJob& a, const DecodeJob& b) {
            if (a.root != b.root) return a.root < b.root;
            if (a.cas != b.cas) return a.cas < b.cas;
            return a.offset < b.offset;
        });
        printf("decoding %zu variations\n", decodeJobs.size());
        fflush(stdout);
        const fs::path wavDir = outDir / "wav";
        fs::create_directories(wavDir);
        std::map<std::string, std::string> lines;
        size_t good = 0, looked = 0, written = 0;
        Guid haveId;
        bool haveAny = false;
        std::vector<u8> chunk;
        for (const DecodeJob& job : decodeJobs) {
            const Guid& id = job.wave->chunks[job.var->chunkIndex].id;
            if (!haveAny || !(id == haveId)) {
                if (!readChunk(id, chunk)) { failures.push_back(job.file + "  FAILED: the chunk could not be read"); haveAny = false; continue; }
                haveId = id;
                haveAny = true;
            }
            bool bad = false;
            std::string events;
            std::string line = decodeOne(job, chunk, wavDir, bad, events);
            if (bad) ++looked; else ++good;
            if (job.write && !holds(line, "FAILED")) ++written;
            lines[job.file] = line + (events.empty() ? "" : "\n" + events);
        }
        for (const std::string& f : failures) lines[f] = f;
        for (const std::string& a : absent) lines[a] = a + "  ABSENT: no such wave in the data";
        const bool anyWrite = reloads || !wavTexts.empty();
        std::ofstream o(wavDir / (anyWrite ? "numbers.txt" : "check.txt"), std::ios::binary);
        o << "# every decode, levels from 10 ms windows in dB of full scale. seam: how far each group's stored first sample lies from where the decoded samples before it\n"
             "# got to, over the group's own average movement: near 1 for a right decode; 'wrong read' is the same bytes with each group's data taken as one run instead of across the rows.\n"
             "# attack: the largest rise over 20 ms within 20 dB of the loudest. from/to: first and last window within 40 dB of the loudest. end: the last 50 ms against the loudest.\n"
             "# lag1: correlation of neighbouring samples (flat noise near 0). LOOK: which number did not look like a sound. 'sounds at': each sharp rise, time(dB).\n";
        for (auto& kv : lines) {
            o << kv.second << "\n";
            if (lines.size() <= 80) printf("%s\n", kv.second.c_str());
        }
        printf("decoded %zu variations: %zu look right, %zu to look at, %zu failed or absent, %zu wave files in %s\n", good + looked, good, looked, failures.size() + absent.size(), written, wavDir.string().c_str());
        fflush(stdout);
    }
    if (doStudy) study(outDir);
    if (doTrace) traces(outDir);
    if (doFinal) writeFinal(outDir, finalGain);
    const int soundsFailed = soundsNames.empty() ? 0 : writeSounds(soundsDir, soundsNames, finalGain);
    // --fit: where each clip lies in its wave and at what level (as study() finds the liked M16 clips).
    for (const auto& fit : fits) {
        std::vector<double> clip;
        int rate = 0;
        const std::vector<double>& wave = waveCached(fit.second);
        if (!readWavMono(fit.first, clip, rate) || rate != 48000 || clip.empty()) { printf("--fit %s: not a 48 kHz wave file\n", shown(fit.first).c_str()); continue; }
        if (wave.empty()) { printf("--fit %s: %s not decoded\n", shown(fit.first).c_str(), fit.second.c_str()); continue; }
        const Corr c = corrSummary(nccAll(clip, wave), rate);
        const std::vector<double> at = slice(wave, c.peakAt, c.peakAt + double(clip.size()) / rate, rate);
        double xy = 0, xx = 0;
        for (size_t i = 0; i < at.size() && i < clip.size(); ++i) { xy += at[i] * clip[i]; xx += at[i] * at[i]; }
        const Level lv = levelOf(clip, rate);
        printf("--fit %s: %.3f s, peak %.1f | in %s at %.4f to %.4f s, correlation %.4f (next best %.4f at %.3f), gain %.2f dB\n", shown(fit.first.filename()).c_str(),
               lv.seconds, lv.peak, fit.second.c_str(), c.peakAt, c.peakAt + lv.seconds, c.peak, c.second, c.secondAt, xx > 0 && xy > 0 ? 20 * std::log10(xy / xx) : -999.0);
    }
    if (modOnly) {
        if (missingOnly)
            printf("--missing: %zu clips made, %d failed, %zu left as they were, in %s\n", soundsNames.size() - size_t(soundsFailed), soundsFailed, modThere,
                   shown(soundsDir).c_str());
        else
            printf("--mod: %zu clips the mod plays, %d failed, in %s\n", soundsNames.size(), soundsFailed, shown(soundsDir).c_str());
        return soundsFailed ? 2 : 0;
    }
    if (doVerify) verify(outDir);
    // What could not be found, by the first two parts of the name.
    {
        std::ofstream o(outDir / "missing.txt", std::ios::binary);
        std::map<std::string, std::pair<u64, u64>> byPrefix;
        for (const ParsedFile& pf : g_files) {
            size_t cut = pf.ref->name.find('/');
            if (cut != std::string::npos) cut = pf.ref->name.find('/', cut + 1);
            if (cut != std::string::npos) cut = pf.ref->name.find('/', cut + 1);
            std::string pre = lower(pf.ref->name.substr(0, cut));
            for (const Wave& w : pf.waves)
                for (const WaveChunk& c : w.chunks) {
                    auto& e = byPrefix[pre];
                    if (heads[c.id].found) e.first++; else e.second++;
                }
        }
        o << "# name prefix: chunks found, chunks not found\n";
        for (auto& kv : byPrefix) o << kv.first << ": " << kv.second.first << " " << kv.second.second << "\n";
    }

    // index.txt: every sound asset.
    size_t waveCount = 0, waveChunks = 0;
    {
        std::ofstream o(outDir / "index.txt", std::ios::binary);
        o << "# every SoundWaveAsset: asset name | instance's Name | channels, seekable\n";
        o << "#   chunk N: id, declared size, stored size, where, audio start in the chunk, codec, version, channels, rate, type (0 RAM 1 stream), samples\n";
        o << "#   var N: its chunk, seek tables' size, loop segments, then per segment @audio offset, seek-table offset, seconds\n";
        for (const ParsedFile& pf : g_files) {
            for (const Wave& w : pf.waves) {
                ++waveCount;
                o << pf.ref->name;
                if (!w.instName.empty()) o << " | " << w.instName;
                o << " | ch " << w.channels << (w.seekable ? " seekable" : "") << " | in " << pf.ref->bundles.size() << " bundles";
                if (!pf.ref->bundles.empty()) o << ", first: " << g_bundleNames[pf.ref->bundles[0]];
                o << "\n";
                for (size_t ci = 0; ci < w.chunks.size(); ++ci) {
                    const WaveChunk& c = w.chunks[ci];
                    ++waveChunks;
                    const ChunkHead& h = heads[c.id];
                    o << "    chunk " << ci << " " << guidText(c.id) << " size " << c.size;
                    if (h.found) {
                        char m[200];
                        if (!h.sbFile.empty()) sprintf_s(m, " stored %u in %s+%u", h.stored, h.sbFile.c_str(), h.offset);
                        else sprintf_s(m, " stored %u %s cas_%02u+%u", h.stored, g_roots[h.root].label.c_str(), h.cas, h.offset);
                        o << m << " audio@" << h.audioOff;
                        if (h.codec >= 0) {
                            sprintf_s(m, " codec %d (%s) v%d ch %d rate %d type %d samples %u", h.codec, codecName(h.codec).c_str(), h.version, h.channels, h.rate, h.type, h.samples);
                            o << m;
                        }
                    } else o << " NOT FOUND";
                    o << "\n";
                }
                for (size_t vi = 0; vi < w.variations.size(); ++vi) {
                    const WaveVariation& v = w.variations[vi];
                    o << "    var " << vi << " chunk " << v.chunkIndex << " seek " << v.seekTablesSize << " loop " << v.firstLoop << "-" << v.lastLoop;
                    for (const WaveSegment& s : v.segments) {
                        char m[64];
                        sprintf_s(m, " @%u/%u %.3fs", s.samplesOffset, s.seekOffset, double(s.seconds));
                        o << m;
                    }
                    o << "\n";
                }
            }
        }
    }
    // types.txt: which kinds of instance the assets hold.
    {
        std::ofstream o(outDir / "types.txt", std::ios::binary);
        std::vector<std::pair<std::string, std::pair<u64, u64>>> v(g_typeCount.begin(), g_typeCount.end());
        std::sort(v.begin(), v.end(), [](auto& a, auto& b) { return a.second.first > b.second.first; });
        o << "# instance type, instances, files\n";
        for (auto& t : v) o << t.first << " " << t.second.first << " " << t.second.second << "\n";
    }
    // refs.txt: which sound assets each read asset names.
    auto soundLike = [](const std::string& s) { std::string l = lower(s); return holds(l, "sound") || holds(l, "audio"); };
    std::map<std::string, const ParsedFile*> byName;
    for (const ParsedFile& pf : g_files) byName[pf.ref->name] = &pf;
    {
        std::ofstream o(outDir / "refs.txt", std::ios::binary);
        for (const ParsedFile& pf : g_files) {
            std::vector<std::string> names;
            for (const Guid& g : pf.externals) {
                auto it = g_fileName.find(g);
                if (it != g_fileName.end() && soundLike(it->second)) names.push_back(it->second);
            }
            if (names.empty()) continue;
            o << pf.ref->name << " (" << pf.primaryType << ")\n";
            for (auto& n : names) o << "    " << n << "\n";
        }
    }
    // weapons.txt: every sound name that has to do with a weapon; guns.txt: the named guns, reload parts first.
    size_t weaponNames = 0;
    {
        std::ofstream o(outDir / "weapons.txt", std::ios::binary);
        o << "# every sound asset whose name has to do with a weapon\n";
        for (const ParsedFile& pf : g_files) {
            std::string low = lower(pf.ref->name);
            if (!soundLike(low)) continue;
            bool isWeapon = holds(low, "weapon");
            if (!isWeapon) for (const Gun& g : g_guns) if (nameIsGun(low, g)) { isWeapon = true; break; }
            if (!isWeapon) continue;
            ++weaponNames;
            o << pf.ref->name << "  (" << pf.primaryType << ")";
            for (const Wave& w : pf.waves)
                for (const WaveChunk& c : w.chunks) {
                    const ChunkHead& h = heads[c.id];
                    o << "  [" << c.size << (h.codec >= 0 ? " codec " + std::to_string(h.codec) : std::string()) << "]";
                }
            o << "\n";
        }
    }
    {
        // guns.txt: each gun's sound patches, found by name and by what its own assets point at, and the waves those graphs play.
        // The gun's folder does not decide: the graph does (the M416's patch plays the M16's reload waves).
        std::ofstream o(outDir / "guns.txt", std::ios::binary);
        auto waveLine = [&](const std::string& name) {
            std::string line = name;
            auto nx = byName.find(name);
            if (nx == byName.end()) return line + "  (asset not read)";
            for (const Wave& w : nx->second->waves) {
                char m[96];
                sprintf_s(m, "  ch %u%s", w.channels, w.seekable ? " seekable" : "");
                line += m;
                for (const WaveVariation& v : w.variations) {
                    if (v.chunkIndex >= w.chunks.size()) continue;
                    const WaveChunk& c = w.chunks[v.chunkIndex];
                    const ChunkHead& h = heads[c.id];
                    float secs = 0;
                    for (const WaveSegment& s : v.segments) secs += s.seconds;
                    sprintf_s(m, "  [%u bytes, audio@%u, %.3fs, %zu seg, codec %d]", c.size, h.audioOff, double(secs), v.segments.size(), h.codec);
                    line += m;
                }
            }
            return line;
        };
        for (const Gun& g : g_guns) {
            o << "== " << g.label << " ==\n";
            std::set<std::string> patches;
            std::vector<std::string> pointers;
            for (const ParsedFile& pf : g_files) {
                std::string low = lower(pf.ref->name);
                if (!nameIsGun(low, g)) continue;
                if (pf.primaryType == "SoundPatchAsset") patches.insert(pf.ref->name);
                if (soundLike(low)) continue;
                for (const Guid& gd : pf.externals) {
                    auto it = g_fileName.find(gd);
                    if (it == g_fileName.end()) continue;
                    auto nx = byName.find(it->second);
                    if (nx != byName.end() && nx->second->primaryType == "SoundPatchAsset") {
                        patches.insert(it->second);
                        pointers.push_back(pf.ref->name + " -> " + it->second);
                    }
                }
            }
            o << "-- the gun's own assets point at --\n";
            for (auto& l : pointers) o << l << "\n";
            std::set<std::string> gunWaves;
            for (const std::string& pn : patches) {
                auto nx = byName.find(pn);
                if (nx == byName.end()) continue;
                o << "-- patch " << pn << "\n";
                std::vector<std::string> shared;
                std::set<std::string> listed;
                for (const GraphWave& gw : nx->second->graph) {
                    auto it = g_fileName.find(gw.file);
                    std::string wn = it != g_fileName.end() ? it->second : guidText(gw.file);
                    std::string low = lower(wn);
                    bool own = !holds(low, "shared_content") || holds(low, "handling");
                    if (own) {
                        o << "   " << gw.node << " #" << gw.instance;
                        if (gw.slot >= 0) o << " [" << gw.slot << "]";
                        o << "  " << waveLine(wn) << "\n";
                        gunWaves.insert(wn);
                    } else if (listed.insert(wn).second) shared.push_back(wn);
                }
                o << "   and " << shared.size() << " shared layers (firing beds, reflections, tails):";
                for (size_t i = 0; i < shared.size() && i < 6; ++i) o << " " << shared[i];
                o << "\n";
            }
            o << "-- every sound asset with the gun's name --\n";
            for (const ParsedFile& pf : g_files) {
                std::string low = lower(pf.ref->name);
                if (!soundLike(low) || !nameIsGun(low, g)) continue;
                o << (partWord(low) ? "P " : "  ") << pf.ref->name << "  (" << pf.primaryType << ")\n";
            }
            o << "\n";
        }
    }
    // codecs.txt: the first bytes, by codec, weapons apart from the rest.
    {
        std::ofstream o(outDir / "codecs.txt", std::ios::binary);
        std::map<std::string, u64> hist;
        std::map<std::string, std::vector<std::string>> samples;
        for (const ParsedFile& pf : g_files) {
            std::string low = lower(pf.ref->name);
            bool weapon = holds(low, "weapon");
            for (const Wave& w : pf.waves)
                for (const WaveChunk& c : w.chunks) {
                    const ChunkHead& h = heads[c.id];
                    if (!h.read) continue;
                    char key[128];
                    sprintf_s(key, "%s chunk starts %02x, audio@%s starts %02x, v%d codec %d (%s) ch %d rate %d type %d", weapon ? "weapon" : "other ", h.firstLen ? h.first[0] : 0, h.audioOff ? ">0" : "0",
                              h.audioLen ? h.audio[0] : 0, h.version, h.codec, codecName(h.codec).c_str(), h.channels, h.rate, h.type);
                    hist[key]++;
                    auto& sm = samples[key];
                    if (sm.size() < 3) sm.push_back(pf.ref->name + " audio@" + std::to_string(h.audioOff) + "\n        chunk " + hexOf(h.first, h.firstLen, true) + "\n        audio " + hexOf(h.audio, h.audioLen, true));
                }
        }
        for (auto& kv : hist) {
            o << kv.first << ": " << kv.second << "\n";
            for (auto& s : samples[kv.first]) o << "    " << s << "\n";
        }
    }
    // tocs.txt: how each superbundle was read.
    {
        std::ofstream o(outDir / "tocs.txt", std::ios::binary);
        for (const TocStat& t : g_tocStats) {
            o << t.root << " " << t.key << ": bundles " << t.bundles << " (base-flag " << t.baseFlag << ", delta-flag " << t.deltaFlag << ", read " << t.sbBundles << ") chunks " << t.chunks
              << " (no sha1 " << t.noSha << ") cas " << t.cas;
            if (!t.note.empty()) o << "  NOTE " << t.note;
            o << "\n";
        }
    }
    printf("sound wave assets %zu, their chunks %zu, weapon-named sound assets %zu\n", waveCount, waveChunks, weaponNames);
    printf("wrote %s\n", outDir.string().c_str());
    return soundsFailed ? 2 : 0;
}

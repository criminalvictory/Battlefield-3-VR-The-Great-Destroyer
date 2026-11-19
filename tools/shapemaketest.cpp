// The guns' collision shapes made on the player's PC (src/shapemake.cpp, src/shaperecipes.h), without the game:
//   T  the recipe table: every row a gunshape command that tools/gunshape.cpp's own argument reading takes back as the row says; names unique
//      and plain; draws whole triangles inside their index buffers; the frame bone never left out; no two rows waiting for the same draw;
//      a recipe for every shape skin.cpp's held guns and hands.cpp name (their sources read from ..\src beside this program's folder);
//   R  each row's bones at rest against the dump its shape was first made from (the bones its kept corners name, where each is on the frame
//      bone, none turned), and shaperecipes::Apart: a kept bone 2 cm back or turned 2 degrees is not the gun at rest, half a millimetre is;
//   M  the making, from a mesh dump as a capture (mesh_<count>_<start>_*.bin in the dump folder; the M416's gun_*.bin): the dump written to
//      guns\make\<name>\, gunshape.exe run on it, its .tmp checked and renamed, gunshape::Reload and Use loading it; a damaged shape file
//      replaced; and every way it can fail (no gunshape.exe, a mesh it refuses, a process stopped for taking too long, a grid that isn't the
//      gun's, a capture short of a draw) leaving no file, nothing in guns\make, no .tmp, the "none" kept, and one line;
//   G  the whole path on a software graphics card (WARP), as in the game: Want, the draw noted with its buffers bound, the staging copies
//      mapped without waiting, the thread, the shape loaded and Ready; a gun put away before its draw let go silently; one held and never
//      drawn, drawn only as another's, with another mesh's buffers, or with a kept bone 2 cm off (a handle pulled) not taken, said once and
//      waited for again later; then taken at rest.
// Everything is written under a folder of its own in %TEMP%, removed at the end; the dump folder is only read.
// Usage: shapemaketest [dump folder (default: this program's)] [all | rest]
//   rest: nothing is tested: the rows of src/shaperecipes.h's kRest tables are written from the dumps (for a new gun's row).
//   all: every recipe whose dump is there is made (about 2 minutes), not only the M16A4's, and each compared with <dump folder>\guns\<name>.bin.
// The exit code is the number of failed checks. Parts whose files aren't there (no dumps: a fresh clone) are said and not counted.

#include <windows.h>

#include <d3d11.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "../src/gunshape.h"
#include "../src/shapemake.h"
#include "../src/shaperecipes.h"

namespace {

using shaperecipes::Draw;
using shaperecipes::Recipe;

int g_failed = 0, g_checked = 0;

void Check(bool ok, const char* format, ...) {
    char text[512];
    va_list args;
    va_start(args, format);
    vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    ++g_checked;
    if (!ok)
        ++g_failed;
    std::printf("  %s  %s\n", ok ? "ok  " : "FAIL", text);
}

std::wstring Wide(const std::string& s) {
    return std::wstring(s.begin(), s.end());
}
std::string Narrow(const std::wstring& s) {
    std::string out;
    for (wchar_t c : s)
        out += c < 128 ? static_cast<char>(c) : '?';
    return out;
}

bool Exists(const std::wstring& path) {
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

std::vector<uint8_t> ReadAll(const std::wstring& path) {
    std::ifstream f(path.c_str(), std::ios::binary);
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}

std::string ReadText(const std::wstring& path) {
    const std::vector<uint8_t> b = ReadAll(path);
    return std::string(b.begin(), b.end());
}

void WriteFile(const std::wstring& path, const std::string& bytes) {
    std::ofstream f(path.c_str(), std::ios::binary);
    f.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

bool FolderEmptyOrGone(const std::wstring& dir) {
    WIN32_FIND_DATAW found;
    const HANDLE h = FindFirstFileW((dir + L"*").c_str(), &found);
    if (h == INVALID_HANDLE_VALUE)
        return true;
    bool any = false;
    do {
        if (wcscmp(found.cFileName, L".") && wcscmp(found.cFileName, L".."))
            any = true;
    } while (FindNextFileW(h, &found));
    FindClose(h);
    return !any;
}

bool AnyTmp(const std::wstring& guns) {
    WIN32_FIND_DATAW found;
    const HANDLE h = FindFirstFileW((guns + L"*.tmp").c_str(), &found);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    FindClose(h);
    return true;
}

// ---- T: the table ----

// tools/gunshape.cpp's own reading of its arguments (its main and tools/meshdump.h ParseDraw), on the words after the cell.
struct Parsed {
    int count = 0, start = 0, frame = 0, indices = 0;
    std::vector<int> leaveOut;
    struct Part {
        int count = 0, start = 0, ownBone = 0, firstBone = 0;
    };
    std::vector<Part> parts;
};
void ParseDraw(const char* text, int& count, int& start) {
    count = std::atoi(text);
    const char* at = std::strchr(text, '@');
    start = at ? std::atoi(at + 1) : 0;
}
bool Parse(const std::string& arguments, Parsed& out) {
    std::vector<std::string> argv = {"gunshape", ".", "out", "0.003"};
    std::istringstream words(arguments);
    for (std::string w; words >> w;)
        argv.push_back(w);
    const int argc = static_cast<int>(argv.size());
    if (argc <= 5)
        return false;  // (it would take the M416's old way: gun_*.bin)
    if (argc > 6)
        for (const char* p = argv[6].c_str(); *p;) {
            out.leaveOut.push_back(std::atoi(p));
            while (*p && *p != ',')
                ++p;
            if (*p == ',')
                ++p;
        }
    ParseDraw(argv[4].c_str(), out.count, out.start);
    out.frame = std::atoi(argv[5].c_str());
    out.indices = argc > 7 ? std::atoi(argv[7].c_str()) : 0;
    for (int a = 8; a < argc; ++a) {
        Parsed::Part part;
        ParseDraw(argv[a].c_str(), part.count, part.start);
        if (const char* colon = std::strchr(argv[a].c_str(), ':')) {
            part.ownBone = std::atoi(colon + 1);
            if (const char* second = std::strchr(colon + 1, ':'))
                part.firstBone = std::atoi(second + 1);
        }
        out.parts.push_back(part);
    }
    return true;
}

// The quoted lower-case names in text between the first "from" and the next "to".
std::vector<std::string> NamesBetween(const std::string& text, const char* from, const char* to) {
    std::vector<std::string> names;
    const size_t a = text.find(from);
    if (a == std::string::npos)
        return names;
    const size_t b = text.find(to, a);
    const std::string part = text.substr(a, b == std::string::npos ? std::string::npos : b - a);
    for (size_t i = 0; i < part.size(); ++i) {
        if (part[i] != '"')
            continue;
        size_t j = i + 1;
        while (j < part.size() && (std::islower(static_cast<unsigned char>(part[j])) || std::isdigit(static_cast<unsigned char>(part[j])) || part[j] == '_'))
            ++j;
        if (j < part.size() && part[j] == '"' && j > i + 1)
            names.push_back(part.substr(i + 1, j - i - 1));
        i = j;
    }
    return names;
}

bool LoadCapture(const std::wstring& dumps, const Recipe& r, shapemake::Capture& out) {
    out.draws.clear();
    for (int d = 0; d < r.drawCount; ++d) {
        const Draw& w = r.draws[d];
        std::wstring prefix = dumps + Wide(shaperecipes::FilePrefix(w));
        if (!Exists(prefix + L"vb0.bin") && w.count == 12768 && w.start == 0)
            prefix = dumps + L"gun_";  // the M416's first dump's names
        shapemake::CapturedDraw c;
        c.count = w.count, c.start = w.start, c.ibFormat = shaperecipes::kIndexFormat, c.stride = shaperecipes::kStride, c.vbOffset = w.vbOffset;
        c.ib = ReadAll(prefix + L"ib.bin"), c.vb = ReadAll(prefix + L"vb0.bin"), c.bones = ReadAll(prefix + L"bones.bin");
        if (c.ib.empty() || c.vb.empty() || c.bones.empty())
            return false;
        out.draws.push_back(std::move(c));
    }
    return true;
}

// The bones a draw's kept triangles are posed by (tools/meshdump.h LoadDraw: a triangle is kept when no corner's main bone is left out; each
// corner is then posed by every bone it has a weight on), and the highest bone any of the draw's vertices names.
struct BonesOf {
    std::set<int> kept;
    int highest = -1;
};
BonesOf Bones(const shapemake::CapturedDraw& c, const Draw& w, const std::set<int>& leaveOut) {
    BonesOf out;
    const size_t n = c.vb.size() > w.vbOffset ? (c.vb.size() - w.vbOffset) / shaperecipes::kStride : 0;
    for (uint32_t i = w.start; i + 2 < w.start + w.count && 2 * (static_cast<size_t>(i) + 3) <= c.ib.size(); i += 3) {
        bool keep = true;
        std::set<int> named;
        for (int k = 0; k < 3; ++k) {
            uint16_t v;
            std::memcpy(&v, &c.ib[2 * (i + k)], 2);
            if (v >= n) {
                keep = false;
                continue;
            }
            const uint8_t* o = &c.vb[w.vbOffset + v * static_cast<size_t>(shaperecipes::kStride)];
            int main = 0, mainWeight = -1;
            for (int s = 0; s < 4; ++s) {
                if (!o[12 + s])
                    continue;
                named.insert(o[8 + s]);
                out.highest = std::max(out.highest, static_cast<int>(o[8 + s]));
                if (o[12 + s] > mainWeight)
                    mainWeight = o[12 + s], main = o[8 + s];
            }
            if (leaveOut.count(main))
                keep = false;
        }
        if (keep)
            out.kept.insert(named.begin(), named.end());
    }
    return out;
}

std::set<int> LeftOut(const Recipe& r) {
    std::set<int> out;
    for (const char* p = r.leaveOut; *p;) {
        out.insert(std::atoi(p));
        while (*p && *p != ',')
            ++p;
        if (*p == ',')
            ++p;
    }
    return out;
}

// A recipe's draw d as its dump has it: the bones it keeps besides its frame bone, each on the frame bone (turned with its turn), and the bones its
// palette must have.
struct RestOf {
    std::vector<shaperecipes::Rest> rest;
    int bones = 0;
    bool leftOutPoses = false;  // a kept corner has a weight on a bone left out
    bool turned = false;        // a kept bone is turned against the frame bone (its Rest says so, with its turn)
};
RestOf RestFrom(const Recipe& r, int d, const shapemake::Capture& capture) {
    RestOf out;
    const Draw& w = r.draws[d];
    const shapemake::CapturedDraw& c = capture.draws[static_cast<size_t>(d)];
    const std::set<int> leaveOut = d == 0 ? LeftOut(r) : std::set<int>{};
    BonesOf of = Bones(c, w, leaveOut);
    const int frame = d == 0 ? r.frameBone : w.ownBone;
    if (d == 0)
        for (int more = 1; more < r.drawCount; ++more)
            of.kept.insert(r.draws[more].firstBone);  // (what a further draw hangs from)
    out.bones = std::max(of.highest, frame) + 1;
    const int held = static_cast<int>(c.bones.size() / 48);
    for (int bone : of.kept) {
        if (leaveOut.count(bone)) {
            out.leftOutPoses = true;
            continue;
        }
        if (bone == frame || bone >= held || frame >= held)
            continue;
        // Its place on the frame, as Apart works it out.
        float m[12], f[12], inv[12], on[12];
        std::memcpy(m, c.bones.data() + 48 * static_cast<size_t>(bone), 48);
        std::memcpy(f, c.bones.data() + 48 * static_cast<size_t>(frame), 48);
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                inv[i * 4 + j] = f[j * 4 + i];
        for (int i = 0; i < 3; ++i)
            inv[i * 4 + 3] = -(inv[i * 4] * f[3] + inv[i * 4 + 1] * f[7] + inv[i * 4 + 2] * f[11]);
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 4; ++j)
                on[i * 4 + j] = inv[i * 4] * m[j] + inv[i * 4 + 1] * m[4 + j] + inv[i * 4 + 2] * m[8 + j];
            on[i * 4 + 3] += inv[i * 4 + 3];
        }
        shaperecipes::Rest rest{bone, {on[3], on[7], on[11]}};
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                if (std::fabs(on[i * 4 + j] - (i == j ? 1.0f : 0.0f)) > 1e-5f)
                    rest.turned = true;
        if (rest.turned) {
            out.turned = true;
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    rest.turn[i * 3 + j] = on[i * 4 + j];
        }
        out.rest.push_back(rest);
    }
    return out;
}

// "rest": the rows for src/shaperecipes.h, from the dumps.
void PrintRest(const std::wstring& dumps) {
    for (const Recipe& r : shaperecipes::kRecipes) {
        shapemake::Capture capture;
        if (!LoadCapture(dumps, r, capture)) {
            std::printf("// (%s: its dump isn't there)\n", r.name);
            continue;
        }
        for (int d = 0; d < r.drawCount; ++d) {
            const RestOf of = RestFrom(r, d, capture);
            std::printf("// %s%s: bones %d%s%s\n", r.name, d ? "'s further draw" : "", of.bones,
                        of.leftOutPoses ? "; A KEPT CORNER HAS A WEIGHT ON A BONE LEFT OUT" : "", of.turned ? "; a kept bone rests turned: its row with its turn" : "");
            if (of.rest.empty())
                continue;
            std::printf("constexpr Rest kRest_%s%s[] = {", r.name, d ? "_b" : "");
            for (size_t k = 0; k < of.rest.size(); ++k) {
                const shaperecipes::Rest& rest = of.rest[k];
                std::printf("%s{%d, {%.9gf, %.9gf, %.9gf}", k ? ", " : "", rest.bone, static_cast<double>(rest.at[0]), static_cast<double>(rest.at[1]),
                            static_cast<double>(rest.at[2]));
                if (rest.turned)
                    for (int i = 0; i < 9; ++i)
                        std::printf("%s%.9gf%s", i ? ", " : ", true, {", static_cast<double>(rest.turn[i]), i == 8 ? "}" : "");
                std::printf("}");
            }
            std::printf("};\n");
        }
    }
}

void TestTable(const std::wstring& source) {
    std::printf("T: the recipes (%zu)\n", std::size(shaperecipes::kRecipes));
    std::set<std::string> names;
    std::set<uint64_t> firstDraws;
    for (const Recipe& r : shaperecipes::kRecipes) {
        bool plain = r.name && *r.name;
        for (const char* p = r.name; plain && *p; ++p)
            plain = std::islower(static_cast<unsigned char>(*p)) || std::isdigit(static_cast<unsigned char>(*p)) || *p == '_';
        bool ok = plain && names.insert(r.name).second && shaperecipes::Find(r.name) == &r;
        ok = ok && r.drawCount >= 1 && r.drawCount <= shaperecipes::kMostDraws && r.frameBone >= 0 && r.frameBone < 85 && r.leaveOut && *r.leaveOut;
        for (int k = 0; ok && k < 3; ++k)
            ok = r.cells[k] > 8 && r.cells[k] < 2000;
        for (int d = 0; ok && d < r.drawCount; ++d) {
            const Draw& w = r.draws[d];
            ok = w.count >= 3 && w.count % 3 == 0 && static_cast<uint64_t>(w.start + w.count) * 2 <= w.ibBytes && w.vbOffset % 4 == 0 &&
                 w.vbOffset + shaperecipes::kStride <= w.vbBytes && w.vbBytes <= (4u << 20) && w.ibBytes <= (4u << 20) && w.ownBone >= 0 && w.firstBone >= 0;
        }
        ok = ok && firstDraws.insert((static_cast<uint64_t>(r.draws[0].count) << 32) | r.draws[0].start).second;
        const std::string arguments = ok ? shaperecipes::Arguments(r) : "";
        Parsed p;
        ok = ok && Parse(arguments, p) && p.count == static_cast<int>(r.draws[0].count) && p.start == static_cast<int>(r.draws[0].start) &&
             p.frame == r.frameBone && !p.leaveOut.empty() && static_cast<int>(p.parts.size()) == r.drawCount - 1 && p.indices == 0;
        std::set<int> out;
        std::string again;
        for (size_t i = 0; ok && i < p.leaveOut.size(); ++i) {
            ok = p.leaveOut[i] >= 0 && p.leaveOut[i] < 85 && p.leaveOut[i] != r.frameBone && out.insert(p.leaveOut[i]).second;
            again += (i ? "," : "") + std::to_string(p.leaveOut[i]);
        }
        ok = ok && again == r.leaveOut;  // (nothing in it but numbers and commas)
        for (int d = 1; ok && d < r.drawCount; ++d) {
            const Parsed::Part& part = p.parts[static_cast<size_t>(d - 1)];
            ok = part.count == static_cast<int>(r.draws[d].count) && part.start == static_cast<int>(r.draws[d].start) && part.ownBone == r.draws[d].ownBone &&
                 part.firstBone == r.draws[d].firstBone;
        }
        Check(ok, "%-7s gunshape <dump> <out> 0.003 %s", r.name ? r.name : "(null)", arguments.c_str());
    }
    Check(!shaperecipes::Find("nosuchgun") && !shaperecipes::Find(nullptr) && !shaperecipes::Find("smaw_a"), "no recipe for a name not in the table");
    // The names the mod asks for: skin.cpp's held guns' shapes, and hands.cpp's for the launcher and the M416.
    const std::string skin = ReadText(source + L"skin.cpp"), hands = ReadText(source + L"hands.cpp");
    if (skin.empty() || hands.empty()) {
        std::printf("  (not checked: the held guns' names, %s isn't there)\n", Narrow(source + L"skin.cpp").c_str());
        return;
    }
    std::vector<std::string> asked = NamesBetween(skin, "constexpr HeldKind kHeldKinds[] = {", "\n};");
    Check(asked.size() >= 10, "skin.cpp's kHeldKinds names %zu shapes", asked.size());
    const std::vector<std::string> other = NamesBetween(hands, "const char* shape = pistolHeld", ";");
    Check(other.size() == 2, "hands.cpp names %zu more (the launcher's and the M416's)", other.size());
    asked.insert(asked.end(), other.begin(), other.end());
    for (const std::string& name : asked)
        Check(shaperecipes::Find(name.c_str()) != nullptr, "a recipe for \"%s\"", name.c_str());
}

// Each row's rest pose against the dump its shape was proved from, and Apart itself.
void TestRest(const std::wstring& dumps) {
    std::printf("R: the guns at rest\n");
    for (const Recipe& r : shaperecipes::kRecipes) {
        // Without a dump: the row is whole.
        const std::set<int> leaveOut = LeftOut(r);
        bool whole = true;
        for (int d = 0; d < r.drawCount; ++d) {
            const Draw& w = r.draws[d];
            const int frame = d == 0 ? r.frameBone : w.ownBone;
            std::set<int> bones;
            whole = whole && w.bones > frame && w.bones <= 85 && w.restCount >= 0 && (w.restCount == 0) == (w.rest == nullptr);
            for (int k = 0; whole && k < w.restCount; ++k)
                whole = w.rest[k].bone >= 0 && w.rest[k].bone < w.bones && w.rest[k].bone != frame && !(d == 0 && leaveOut.count(w.rest[k].bone)) &&
                        bones.insert(w.rest[k].bone).second;
            if (d > 0) {  // (what a further draw hangs from is kept at rest by the first)
                bool hung = w.firstBone == r.frameBone;
                for (int k = 0; k < r.draws[0].restCount; ++k)
                    hung = hung || r.draws[0].rest[k].bone == w.firstBone;
                whole = whole && hung;
            }
        }
        shapemake::Capture capture;
        if (!LoadCapture(dumps, r, capture)) {
            Check(whole, "%-7s its row's bones at rest are whole (its dump isn't there: not compared)", r.name);
            continue;
        }
        bool same = whole;
        float most = 0.0f;
        int turnedKept = 0;
        for (int d = 0; d < r.drawCount; ++d) {
            const Draw& w = r.draws[d];
            const RestOf of = RestFrom(r, d, capture);
            same = same && !of.leftOutPoses && of.bones == w.bones && static_cast<int>(of.rest.size()) == w.restCount;
            for (int k = 0; same && k < w.restCount; ++k) {
                const shaperecipes::Rest& got = of.rest[static_cast<size_t>(k)];
                same = got.bone == w.rest[k].bone && got.turned == w.rest[k].turned;
                for (int i = 0; same && i < 3; ++i)
                    same = std::fabs(got.at[i] - w.rest[k].at[i]) < 2e-5f;
                for (int i = 0; same && got.turned && i < 9; ++i)
                    same = std::fabs(got.turn[i] - w.rest[k].turn[i]) < 2e-5f;
                turnedKept += got.turned && same;
            }
            const std::vector<uint8_t>& p = capture.draws[static_cast<size_t>(d)].bones;
            most = std::max(most, shaperecipes::Apart(w, d == 0 ? r.frameBone : w.ownBone, p.data(), static_cast<int>(p.size() / 48)));
        }
        Check(same && most < 5e-5f, "%-7s its row is its dump: the bones its kept corners name, each where the row says, %s; %.3f mm apart", r.name,
              turnedKept ? "turned as it says (a still part resting turned)" : "none turned", static_cast<double>(most) * 1000.0);
    }
    // Apart, on the M416's row with a palette made up: its frame bone turned and moved, every kept bone at rest on it.
    const Recipe& r = *shaperecipes::Find("m416");
    const Draw& w = r.draws[0];
    const float c = std::cos(0.7f), s = std::sin(0.7f);
    const float frame[12] = {c, 0, s, 0.3f, 0, 1, 0, -0.2f, -s, 0, c, 0.5f};  // turned about y
    std::vector<float> palette(12 * 85, 0.0f);
    auto put = [&](std::vector<float>& into, int bone, const float on[3]) {  // frame * (identity moved by on)
        float* m = &into[static_cast<size_t>(bone) * 12];
        std::memcpy(m, frame, sizeof(frame));
        for (int i = 0; i < 3; ++i)
            m[i * 4 + 3] = frame[i * 4] * on[0] + frame[i * 4 + 1] * on[1] + frame[i * 4 + 2] * on[2] + frame[i * 4 + 3];
    };
    const float none[3] = {};
    put(palette, r.frameBone, none);
    for (int k = 0; k < w.restCount; ++k)
        put(palette, w.rest[k].bone, w.rest[k].at);
    int worst = -2;
    const float rest = shaperecipes::Apart(w, r.frameBone, palette.data(), 85, &worst);
    Check(rest < 1e-5f, "at rest on a frame bone turned and moved: %.4f mm apart", static_cast<double>(rest) * 1000.0);
    // Its charging handle (bone 0) pulled 2 cm back; half a millimetre; its dust cover (bone 3) swung 2 degrees.
    std::vector<float> moved = palette;
    const float back[3] = {w.rest[0].at[0], w.rest[0].at[1], w.rest[0].at[2] - 0.02f};
    put(moved, w.rest[0].bone, back);
    const float pulled = shaperecipes::Apart(w, r.frameBone, moved.data(), 85, &worst);
    Check(std::fabs(pulled - 0.02f) < 1e-5f && worst == w.rest[0].bone && pulled > shaperecipes::kRestApart, "a kept bone 2 cm back: %.2f mm apart, bone %d: not taken",
          static_cast<double>(pulled) * 1000.0, worst);
    moved = palette;
    const float slightly[3] = {w.rest[0].at[0], w.rest[0].at[1], w.rest[0].at[2] - 0.0005f};
    put(moved, w.rest[0].bone, slightly);
    const float slight = shaperecipes::Apart(w, r.frameBone, moved.data(), 85);
    Check(slight > 0.0004f && slight <= shaperecipes::kRestApart, "... half a millimetre: %.2f mm, taken", static_cast<double>(slight) * 1000.0);
    moved = palette;
    {
        float* m = &moved[static_cast<size_t>(w.rest[2].bone) * 12];
        const float a = 2.0f * 3.14159265f / 180.0f;
        float turned[12];
        for (int i = 0; i < 3; ++i) {  // m * a turn about z, in the bone's own space
            turned[i * 4] = m[i * 4] * std::cos(a) + m[i * 4 + 1] * std::sin(a);
            turned[i * 4 + 1] = -m[i * 4] * std::sin(a) + m[i * 4 + 1] * std::cos(a);
            turned[i * 4 + 2] = m[i * 4 + 2];
            turned[i * 4 + 3] = m[i * 4 + 3];
        }
        std::memcpy(m, turned, sizeof(turned));
    }
    const float swung = shaperecipes::Apart(w, r.frameBone, moved.data(), 85, &worst);
    Check(swung > 0.005f && swung < 0.01f && worst == w.rest[2].bone, "a kept bone turned 2 degrees: %.2f mm apart at 20 cm, bone %d: not taken", static_cast<double>(swung) * 1000.0,
          worst);
    moved = palette;
    moved[static_cast<size_t>(w.rest[1].bone) * 12 + 7] = std::nanf("");
    Check(shaperecipes::Apart(w, r.frameBone, moved.data(), 85) > 1.0f && shaperecipes::Apart(w, r.frameBone, palette.data(), w.bones - 1) > 1.0f &&
              shaperecipes::Apart(w, r.frameBone, nullptr, 85) > 1.0f && shaperecipes::Apart(w, r.frameBone, palette.data(), w.bones) < 1e-5f,
          "a palette with what isn't a number, one a bone short, none: far; one of just its bones: at rest");
    // A kept bone that rests turned (the M249's carrying handle, folded over to the right, the review of 2026-11-18): on the same frame bone turned and
    // moved, every kept bone at its rest, its turn and all: at rest; that handle turned 2 degrees more about its own z: not taken; and as an unturned rest
    // would have it (no turn: its axes the frame's): far off, not taken. (Before, a Rest held no turn, and the handle was left out of the shape.)
    if (const Recipe* m249 = shaperecipes::Find("m249"); m249 && m249->draws[0].restCount >= 2) {
        const Draw& w2 = m249->draws[0];
        int turnedAt = -1;
        for (int k = 0; k < w2.restCount; ++k)
            if (w2.rest[k].turned)
                turnedAt = k;
        auto putTurned = [&](std::vector<float>& into, int bone, const float on[3], const float* turn) {  // frame * (turn | on)
            float m[12] = {turn[0], turn[1], turn[2], on[0], turn[3], turn[4], turn[5], on[1], turn[6], turn[7], turn[8], on[2]};
            float* out = &into[static_cast<size_t>(bone) * 12];
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 4; ++j)
                    out[i * 4 + j] = frame[i * 4] * m[j] + frame[i * 4 + 1] * m[4 + j] + frame[i * 4 + 2] * m[8 + j];
                out[i * 4 + 3] += frame[i * 4 + 3];
            }
        };
        constexpr float kNone[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
        std::vector<float> pal(12 * 85, 0.0f);
        putTurned(pal, m249->frameBone, none, kNone);
        for (int k = 0; k < w2.restCount; ++k)
            putTurned(pal, w2.rest[k].bone, w2.rest[k].at, w2.rest[k].turned ? w2.rest[k].turn : kNone);
        const float atRest = shaperecipes::Apart(w2, m249->frameBone, pal.data(), 85);
        float swung2 = 0.0f, flat = 0.0f;
        int worst2 = -2;
        if (turnedAt >= 0) {
            const shaperecipes::Rest& rest = w2.rest[turnedAt];
            const float a = 2.0f * 3.14159265f / 180.0f, c2 = std::cos(a), s2 = std::sin(a);
            float more[9];
            for (int i = 0; i < 3; ++i) {  // its turn times a turn about z, in the bone's own space
                more[i * 3] = rest.turn[i * 3] * c2 + rest.turn[i * 3 + 1] * s2;
                more[i * 3 + 1] = -rest.turn[i * 3] * s2 + rest.turn[i * 3 + 1] * c2;
                more[i * 3 + 2] = rest.turn[i * 3 + 2];
            }
            std::vector<float> turned2 = pal;
            putTurned(turned2, rest.bone, rest.at, more);
            swung2 = shaperecipes::Apart(w2, m249->frameBone, turned2.data(), 85, &worst2);
            std::vector<float> unturned = pal;
            putTurned(unturned, rest.bone, rest.at, kNone);
            flat = shaperecipes::Apart(w2, m249->frameBone, unturned.data(), 85);
        }
        Check(turnedAt >= 0 && atRest < 1e-5f && swung2 > 0.005f && swung2 < 0.01f && worst2 == w2.rest[turnedAt >= 0 ? turnedAt : 0].bone &&
                  flat > shaperecipes::kRestApart,
              "a kept bone resting turned (the M249's carrying handle): at its rest %.4f mm apart; turned 2 degrees more %.2f mm at 20 cm, bone %d: not taken; "
              "unturned %.1f mm: not taken",
              static_cast<double>(atRest) * 1000.0, static_cast<double>(swung2) * 1000.0, worst2, static_cast<double>(flat) * 1000.0);
    }
}

// ---- M: the making ----

struct Places {
    std::wstring dumps, exe, root, guns;
};

// Nothing left of a try that failed: no shape, no .tmp, nothing in guns\make, the "none" kept, one line.
void CheckNothing(const Places& at, const Recipe& r, const shapemake::Result& made, const char* what, const char* word) {
    const bool line = made.line.find("Gun shape: " + std::string(r.name) + "'s shape couldn't be made (") == 0 && made.line.find('\n') == std::string::npos &&
                      made.line.find("the hand goes through this gun, as before") != std::string::npos && made.line.find(word) != std::string::npos;
    Check(!made.ok && line && !Exists(at.guns + Wide(r.name) + L".bin") && !AnyTmp(at.guns) && FolderEmptyOrGone(at.guns + L"make\\") &&
              !gunshape::Reload(r.name) && !gunshape::Use(r.name),
          "%s: no file, nothing left, none loaded, one line: %s", what, made.line.c_str());
}

bool MakeOne(const Places& at, const Recipe& r, bool compare) {
    shapemake::Capture capture;
    if (!LoadCapture(at.dumps, r, capture)) {
        std::printf("  (not made: %s, its dump isn't in %s)\n", r.name, Narrow(at.dumps).c_str());
        return false;
    }
    const std::wstring bin = at.guns + Wide(r.name) + L".bin";
    const ULONGLONG from = GetTickCount64();
    const shapemake::Result made = shapemake::Make(r, capture, at.guns, at.exe);
    const double seconds = static_cast<double>(GetTickCount64() - from) / 1000.0;
    const std::string refused = made.ok ? shapemake::Refused(bin, r) : "";
    Check(made.ok && refused.empty() && !AnyTmp(at.guns) && FolderEmptyOrGone(at.guns + L"make\\"), "%-7s made in %.1f s, its grid the recipe's, no .tmp, guns\\make empty%s%s",
          r.name, seconds, made.ok ? "" : ": ", made.ok ? refused.c_str() : made.line.c_str());
    if (made.ok && compare) {
        const std::wstring kept = at.dumps + L"guns\\" + Wide(r.name) + L".bin";
        if (Exists(kept))
            std::printf("        %s the shape kept in %s\n", ReadAll(kept) == ReadAll(bin) ? "byte for byte" : "NOT the same bytes as", Narrow(kept).c_str());
    }
    return made.ok;
}

void TestMake(const Places& at, bool all) {
    std::printf("M: the making (gunshape.exe: %s)\n", Narrow(at.exe).c_str());
    const Recipe& r = *shaperecipes::Find("m16a4");
    shapemake::Capture capture;
    if (!Exists(at.exe) || !LoadCapture(at.dumps, r, capture)) {
        std::printf("  (not run: gunshape.exe or the M16A4's dump mesh_17013_312_*.bin isn't there)\n");
        return;
    }
    gunshape::SetFolder(at.guns.c_str());
    Check(!gunshape::Use(r.name), "no shape yet: Use says none");
    // What a crash left behind.
    CreateDirectoryW((at.guns + L"make").c_str(), nullptr);
    CreateDirectoryW((at.guns + L"make\\old").c_str(), nullptr);
    WriteFile(at.guns + L"make\\old\\mesh_1_0_vb0.bin", "left");
    WriteFile(at.guns + L"old.bin.tmp", "left");
    WriteFile(at.guns + L"kept.bin", "kept");
    shapemake::Sweep(at.guns);
    Check(!Exists(at.guns + L"make") && !Exists(at.guns + L"old.bin.tmp") && Exists(at.guns + L"kept.bin"), "a crash's guns\\make and .tmp swept, a shape kept");
    DeleteFileW((at.guns + L"kept.bin").c_str());

    CheckNothing(at, r, shapemake::Make(r, capture, at.guns, at.root + L"nosuch.exe"), "no gunshape.exe", "isn't there");
    {
        shapemake::Capture broken = capture;
        broken.draws[0].vb.clear();
        CheckNothing(at, r, shapemake::Make(r, broken, at.guns, at.exe), "a mesh gunshape.exe refuses", "gave up, code 1");
        Check(ReadText(at.guns + L"gunshape.log").find("mesh dump isn't in") != std::string::npos, "guns\\gunshape.log has gunshape.exe's own words");
    }
    CheckNothing(at, r, shapemake::Make(r, capture, at.guns, at.exe, 1), "gunshape.exe stopped after 1 ms", "took too long");
    {
        Recipe other = r;
        other.cells[2] += 10;
        CheckNothing(at, other, shapemake::Make(other, capture, at.guns, at.exe), "a grid 10 cells longer than the gun's", "another pose or mesh");
    }
    {
        shapemake::Capture none;
        CheckNothing(at, r, shapemake::Make(r, none, at.guns, at.exe), "a capture with no draw", "wasn't all read");
    }
    {
        // A palette short of the frame bone, and one a bone short of what the draw's vertices name: gunshape.exe refuses both (and reads
        // nothing past the palette's end).
        shapemake::Capture cut = capture;
        cut.draws[0].bones.resize(48 * static_cast<size_t>(r.frameBone));
        CheckNothing(at, r, shapemake::Make(r, cut, at.guns, at.exe), "a palette short of the frame bone", "gave up, code 1");
        const Recipe& high = *shaperecipes::Find("an94");  // (its frame bone 0, its vertices name bones up to 5)
        if (LoadCapture(at.dumps, high, cut)) {
            cut.draws[0].bones.resize(48 * static_cast<size_t>(high.draws[0].bones - 1));
            CheckNothing(at, high, shapemake::Make(high, cut, at.guns, at.exe), "a palette one bone short of what its vertices name", "gave up, code 1");
        }
    }
    // A damaged shape file is replaced.
    WriteFile(at.guns + Wide(r.name) + L".bin", "GSDF damaged");
    Check(!gunshape::Reload(r.name), "a damaged shape file doesn't load");
    if (MakeOne(at, r, true)) {
        Check(gunshape::Reload(r.name) && gunshape::Use(r.name), "the new shape loads in place of the none kept (Reload, then Use)");
        // Along its middle from the receiver to the handguard somewhere is inside it; 10 cm above its line is air, with a way out.
        float deepest = 1.0f, above = 0.0f, way[3] = {};
        for (float z = 0.10f; z < 0.70f; z += 0.01f) {
            const float p[3] = {0.0f, 0.03f, z};
            if (float d = 0.0f; gunshape::Distance(p, d) && d < deepest)
                deepest = d;
        }
        const float air[3] = {0.0f, 0.10f, 0.30f};
        const bool b = gunshape::Distance(air, above, way);
        Check(b && deepest < -0.002f && above > 0.02f && way[1] > 0.5f, "its field reads: %.1f mm at its deepest along its middle, %.1f mm of air above the receiver, the way out up",
              deepest * 1000.0f, above * 1000.0f);
    }
    if (all)
        for (const Recipe& other : shaperecipes::kRecipes)
            if (&other != &r)
                MakeOne(at, other, true);
}

// ---- G: on a graphics card ----

struct Bound {
    ID3D11Buffer* ib = nullptr;
    ID3D11Buffer* vb = nullptr;
};
Bound Bind(ID3D11Device* device, ID3D11DeviceContext* ctx, const std::vector<uint8_t>& ib, const std::vector<uint8_t>& vb, UINT vbOffset) {
    Bound b;
    D3D11_BUFFER_DESC id{static_cast<UINT>(ib.size()), D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER, 0, 0, 0};
    D3D11_BUFFER_DESC vd{static_cast<UINT>(vb.size()), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0};
    D3D11_SUBRESOURCE_DATA idata{ib.data(), 0, 0}, vdata{vb.data(), 0, 0};
    device->CreateBuffer(&id, &idata, &b.ib);
    device->CreateBuffer(&vd, &vdata, &b.vb);
    const UINT stride = shaperecipes::kStride;
    ctx->IASetVertexBuffers(0, 1, &b.vb, &stride, &vbOffset);
    ctx->IASetIndexBuffer(b.ib, DXGI_FORMAT_R16_UINT, 0);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    return b;
}
void Let(Bound& b) {
    if (b.ib)
        b.ib->Release();
    if (b.vb)
        b.vb->Release();
    b = Bound{};
}

void TestCard(const Places& places) {
    std::printf("G: the whole path on a software graphics card\n");
    Places at = places;  // (a shapes' folder of its own: none of M's in it)
    at.guns = at.root + L"card\\";
    CreateDirectoryW(at.guns.substr(0, at.guns.size() - 1).c_str(), nullptr);
    const Recipe& r = *shaperecipes::Find("an94");
    const Recipe& wrongOne = *shaperecipes::Find("rpk");
    const Recipe& never = *shaperecipes::Find("m4a1");
    const Recipe& notOwn = *shaperecipes::Find("ak74");
    shapemake::Capture dump, rpk;
    if (!Exists(at.exe) || !LoadCapture(at.dumps, r, dump) || !LoadCapture(at.dumps, wrongOne, rpk)) {
        std::printf("  (not run: gunshape.exe or the AN-94's and RPK's dumps aren't there)\n");
        return;
    }
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    D3D_FEATURE_LEVEL level{};
    if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device, &level, &ctx))) {
        std::printf("  (not run: no software graphics card on this PC)\n");
        return;
    }
    const DWORD settle = 200, drawWait = 600, retry = 400;
    shapemake::ForTools(at.guns.c_str(), at.exe.c_str(), settle, drawWait, retry);
    gunshape::SetFolder(at.guns.c_str());
    auto frames = [&](DWORD ms) {
        for (const ULONGLONG until = GetTickCount64() + ms; GetTickCount64() < until; Sleep(5))
            shapemake::OnFrame(nullptr, nullptr);
    };
    const std::vector<uint8_t>& palette = dump.draws[0].bones;
    const Draw& w = r.draws[0];

    Check(!shapemake::Wants(w.count, w.start) && !shapemake::Ready(r.name), "nothing waited for before a gun is wanted");
    shapemake::Holding("nosuchgun", false);
    Check(!shapemake::g_frameWork.load() && shapemake::g_wantDraw.load() == 0, "a name with no recipe: nothing waited for");
    shapemake::Holding(never.name, true);
    Check(!shapemake::g_frameWork.load() && shapemake::g_wantDraw.load() == 0, "a gun in hand that has its shape: nothing waited for");

    // A gun put away before its draw came: let go at once without a line, and waited for again the moment it's held again.
    const Draw& n = never.draws[0];
    shapemake::Holding(never.name, false);
    Check(shapemake::Wants(n.count, n.start), "a gun in hand with no shape: its draw %u/%u is waited for", n.count, n.start);
    shapemake::Holding(nullptr, false);
    frames(50);
    Check(!shapemake::Wants(n.count, n.start) && shapemake::Lines() == 0, "put away before its draw came: let go at once, without a line");
    // Held and never drawn: one line, then waited for again later, without another.
    shapemake::Holding(never.name, false);
    Check(shapemake::Wants(n.count, n.start), "... and waited for again the moment it's held again");
    frames(drawWait + 200);
    std::string said = shapemake::LastLine();
    Check(!shapemake::Wants(n.count, n.start) && shapemake::Lines() == 1 && said.find("Gun shape: m4a1's shape couldn't be made (in 1 s of holding it its draw 22194/312 wasn't drawn") == 0 &&
              said.find("as before") != std::string::npos,
          "held and never drawn: one line: %s", said.c_str());
    shapemake::Want(never.name);
    Check(!shapemake::Wants(n.count, n.start), "... not waited for again at once");
    Sleep(retry + 50);
    shapemake::Want(never.name);
    Check(shapemake::Wants(n.count, n.start), "... but after %lu ms, while it's held", retry);
    frames(drawWait + 200);
    Check(!shapemake::Wants(n.count, n.start) && shapemake::Lines() == 1, "... and never drawn again: no second line");

    // Its draw there, but never as the player's own (other soldiers' copies, another buffer bound, a base vertex): one line with the counts.
    const Draw& o = notOwn.draws[0];
    shapemake::Holding(notOwn.name, false);
    Sleep(settle + 50);
    shapemake::NoteDraw(ctx, o.count, o.start, 0, nullptr, 0);
    shapemake::NoteDraw(ctx, o.count, o.start, 0, palette.data(), 48);
    shapemake::NoteDraw(ctx, o.count, o.start, 1, palette.data(), palette.size());
    frames(drawWait + 200);
    said = shapemake::LastLine();
    Check(shapemake::Lines() == 2 && said.find("Gun shape: ak74's shape couldn't be made (") == 0 &&
              said.find("never came as the player's own: 2 without the player's bones bound, 1 with a base vertex") != std::string::npos,
          "drawn, but never as the player's own: one line: %s", said.c_str());

    // Another model with the gun's counts (the AN-94's buffers bound for the RPK's draw): left alone, said once, nothing read back.
    const Draw& x = wrongOne.draws[0];
    shapemake::Holding(wrongOne.name, false);
    Bound other = Bind(device, ctx, dump.draws[0].ib, dump.draws[0].vb, 0);
    Sleep(settle + 50);
    for (int i = 0; i < 3; ++i)
        shapemake::NoteDraw(ctx, x.count, x.start, 0, rpk.draws[0].bones.data(), rpk.draws[0].bones.size());
    Check(shapemake::Wants(x.count, x.start), "a draw with its counts but another mesh's buffers isn't taken");
    frames(drawWait + 200);
    said = shapemake::LastLine();
    Check(!shapemake::Wants(x.count, x.start) && shapemake::Lines() == 3 &&
              said.find("Gun shape: rpk's shape couldn't be made (its draw 24492/0 wasn't the mesh known: 3 seen") == 0 && said.find("as before") != std::string::npos &&
              !Exists(at.guns + L"rpk.bin") && FolderEmptyOrGone(at.guns + L"make\\"),
          "... given up on with one line, nothing read back: %s", said.c_str());
    Let(other);

    // The gun itself.
    Check(!gunshape::Use(r.name), "the AN-94: no shape, Use says none");
    shapemake::Holding(r.name, false);
    shapemake::Want(r.name);
    Bound gun = Bind(device, ctx, dump.draws[0].ib, dump.draws[0].vb, w.vbOffset);
    shapemake::NoteDraw(ctx, w.count, w.start, 0, palette.data(), palette.size());
    Check(shapemake::Wants(w.count, w.start), "its draw isn't taken in the first %lu ms (the gun being raised)", settle);
    Sleep(settle + 50);
    shapemake::NoteDraw(ctx, w.count, w.start, 1, palette.data(), palette.size());
    shapemake::NoteDraw(ctx, w.count, w.start, 0, palette.data(), 48u * static_cast<size_t>(w.bones - 1));
    shapemake::NoteDraw(ctx, w.count, w.start, 0, nullptr, 0);
    Check(shapemake::Wants(w.count, w.start), "nor with a base vertex, nor with a palette short of a bone its vertices name, nor without the player's own bound");
    // A part moved: the bone kept by its shape 2 cm back, as a handle pulled is. Not taken, however long: one line, nothing read back.
    std::vector<uint8_t> pulled = palette;
    {
        float z = 0.0f;
        uint8_t* at11 = pulled.data() + 48 * static_cast<size_t>(w.rest[0].bone) + 4 * 11;
        std::memcpy(&z, at11, 4);
        z -= 0.02f;
        std::memcpy(at11, &z, 4);
    }
    for (int i = 0; i < 4; ++i)
        shapemake::NoteDraw(ctx, w.count, w.start, 0, pulled.data(), pulled.size());
    Check(shapemake::Wants(w.count, w.start), "nor with a kept bone 2 cm from where it is at rest (a handle pulled): the draw is waited for still");
    frames(drawWait + 200);
    said = shapemake::LastLine();
    char expect[96];
    std::snprintf(expect, sizeof(expect), "4 draws, the last with bone %d 20.0 mm off", w.rest[0].bone);
    Check(!shapemake::Wants(w.count, w.start) && shapemake::Lines() == 4 && said.find("Gun shape: an94's shape couldn't be made (") == 0 &&
              said.find("its parts weren't once where they are at rest") != std::string::npos && said.find(expect) != std::string::npos &&
              !Exists(at.guns + L"an94.bin") && FolderEmptyOrGone(at.guns + L"make\\") && !gunshape::Use(r.name),
          "never at rest in all the wait: no shape, nothing read back, one line: %s", said.c_str());
    // Later, at rest: taken.
    Sleep(retry + 50);
    shapemake::Want(r.name);
    Check(shapemake::Wants(w.count, w.start), "waited for again after %lu ms", retry);
    Sleep(settle + 50);
    shapemake::NoteDraw(ctx, w.count, w.start, 0, pulled.data(), pulled.size());
    Check(shapemake::Wants(w.count, w.start), "still pulled: still not taken");
    shapemake::NoteDraw(ctx, w.count, w.start, 0, palette.data(), palette.size());
    Check(!shapemake::Wants(w.count, w.start), "at rest: its draw taken, no draw waited for any more");
    shapemake::NoteDraw(ctx, w.count, w.start, 0, palette.data(), palette.size());  // (a second eye's: nothing)
    Let(gun);  // (the game may let go of its buffers: the mod holds them till the copy)
    shapemake::Holding(nullptr, false);  // (put away once its mesh is taken: made all the same)
    int polls = 0;
    const ULONGLONG from = GetTickCount64();
    bool ready = false;
    while (!ready && GetTickCount64() - from < 120000) {
        shapemake::OnFrame(nullptr, nullptr);
        ++polls;
        ready = shapemake::Ready(r.name);
        Sleep(5);
    }
    const std::string line = shapemake::LastLine();
    Check(ready && shapemake::Lines() == 5 && line.find("Gun shape: an94's shape made on this PC from the game's own mesh in ") == 0, "made and ready after %.1f s: %s",
          static_cast<double>(GetTickCount64() - from) / 1000.0, line.c_str());
    Check(!shapemake::Ready(r.name), "Ready says so once");
    Check(gunshape::Use(r.name) && shapemake::Refused(at.guns + L"an94.bin", r).empty() && !AnyTmp(at.guns) && FolderEmptyOrGone(at.guns + L"make\\"),
          "Use has it now; guns\\an94.bin the gun's grid, no .tmp, guns\\make empty");
    // What came back from the graphics card is the dump itself: the shape made straight from the dump is the same file.
    const std::wstring direct = at.root + L"direct\\";
    CreateDirectoryW(direct.substr(0, direct.size() - 1).c_str(), nullptr);
    const shapemake::Result straight = shapemake::Make(r, dump, direct, at.exe);
    Check(straight.ok && ReadAll(direct + L"an94.bin") == ReadAll(at.guns + L"an94.bin"), "read back from the graphics card, byte for byte the shape made straight from the dump");
    shapemake::Holding(r.name, false);
    Check(!shapemake::g_frameWork.load() && shapemake::g_wantDraw.load() == 0, "made: nothing more done a frame, no draw waited for, held or not");
    ctx->ClearState();
    ctx->Release();
    device->Release();
}

void RemoveAll(const std::wstring& dir) {
    WIN32_FIND_DATAW found;
    const HANDLE h = FindFirstFileW((dir + L"*").c_str(), &found);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!wcscmp(found.cFileName, L".") || !wcscmp(found.cFileName, L".."))
                continue;
            if (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                RemoveAll(dir + found.cFileName + L"\\");
            else
                DeleteFileW((dir + found.cFileName).c_str());
        } while (FindNextFileW(h, &found));
        FindClose(h);
    }
    RemoveDirectoryW(dir.substr(0, dir.size() - 1).c_str());
}

}

int wmain(int argc, wchar_t** argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring own = path;
    own.resize(own.find_last_of(L'\\') + 1);
    Places at;
    at.dumps = own;
    bool all = false, rest = false;
    for (int a = 1; a < argc; ++a) {
        if (!wcscmp(argv[a], L"all"))
            all = true;
        else if (!wcscmp(argv[a], L"rest"))
            rest = true;
        else
            at.dumps = argv[a];
    }
    if (!at.dumps.empty() && at.dumps.back() != L'\\')
        at.dumps += L'\\';
    if (rest) {
        PrintRest(at.dumps);
        return 0;
    }
    at.exe = own + L"gunshape.exe";
    wchar_t temp[MAX_PATH];
    GetTempPathW(MAX_PATH, temp);
    at.root = std::wstring(temp) + L"shapemaketest_" + std::to_wstring(GetCurrentProcessId()) + L"\\";
    at.guns = at.root + L"guns\\";
    CreateDirectoryW(at.root.substr(0, at.root.size() - 1).c_str(), nullptr);
    CreateDirectoryW(at.guns.substr(0, at.guns.size() - 1).c_str(), nullptr);

    TestTable(own + L"..\\src\\");
    TestRest(at.dumps);
    TestMake(at, all);
    TestCard(at);

    RemoveAll(at.root);
    std::printf("%d of %d checks failed\n", g_failed, g_checked);
    return g_failed;
}

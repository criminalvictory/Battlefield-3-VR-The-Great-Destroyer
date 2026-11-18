// The guns' collision shapes made on the player's PC (src/shapemake.cpp, src/shaperecipes.h), without the game:
//   T  the recipe table: every row a gunshape command that tools/gunshape.cpp's own argument reading takes back as the row says; names unique
//      and plain; draws whole triangles inside their index buffers; the frame bone never left out; no two rows waiting for the same draw;
//      a recipe for every shape skin.cpp's held guns and hands.cpp name (their sources read from ..\src beside this program's folder);
//   M  the making, from a mesh dump as a capture (mesh_<count>_<start>_*.bin in the dump folder; the M416's gun_*.bin): the dump written to
//      guns\make\<name>\, gunshape.exe run on it, its .tmp checked and renamed, gunshape::Reload and Use loading it; a damaged shape file
//      replaced; and every way it can fail (no gunshape.exe, a mesh it refuses, a process stopped for taking too long, a grid that isn't the
//      gun's, a capture short of a draw) leaving no file, nothing in guns\make, no .tmp, the "none" kept, and one line;
//   G  the whole path on a software graphics card (WARP), as in the game: Want, the draw noted with its buffers bound, the staging copies
//      mapped without waiting, the thread, the shape loaded and Ready; a draw with the gun's counts but another mesh's buffers left alone,
//      then given up on with a line and not tried again; a gun never drawn given up on silently and wanted again.
// Everything is written under a folder of its own in %TEMP%, removed at the end; the dump folder is only read.
// Usage: shapemaketest [dump folder (default: this program's)] [all]
//   all: every recipe whose dump is there is made (about 2 minutes), not only the M16A4's, and each compared with <dump folder>\guns\<name>.bin.
// The exit code is the number of failed checks. Parts whose files aren't there (no dumps: a fresh clone) are said and not counted.

#include <windows.h>

#include <d3d11.h>

#include <cctype>
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

// ---- M: the making ----

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

void TestCard(const Places& at) {
    std::printf("G: the whole path on a software graphics card\n");
    const Recipe& r = *shaperecipes::Find("an94");
    const Recipe& wrongOne = *shaperecipes::Find("rpk");
    const Recipe& never = *shaperecipes::Find("ak74");
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
    const DWORD settle = 200, drawWait = 600;
    shapemake::ForTools(at.guns.c_str(), at.exe.c_str(), settle, drawWait);
    gunshape::SetFolder(at.guns.c_str());
    auto frames = [&](DWORD ms) {
        for (const ULONGLONG until = GetTickCount64() + ms; GetTickCount64() < until; Sleep(5))
            shapemake::OnFrame(nullptr, nullptr);
    };
    const std::vector<uint8_t>& palette = dump.draws[0].bones;
    const Draw& w = r.draws[0];

    Check(!shapemake::Wants(w.count, w.start) && !shapemake::Ready(r.name), "nothing waited for before a gun is wanted");
    shapemake::Want("nosuchgun");
    Check(!shapemake::g_frameWork.load() && shapemake::g_wantDraw.load() == 0, "a name with no recipe: nothing waited for");

    // A gun never drawn: given up on silently, and wanted again the next time.
    const Draw& n = never.draws[0];
    shapemake::Want(never.name);
    Check(shapemake::Wants(n.count, n.start), "wanted: its draw %u/%u is waited for", n.count, n.start);
    frames(drawWait + 200);
    Check(!shapemake::Wants(n.count, n.start) && shapemake::LastLine().empty(), "never drawn: given up on without a line");
    shapemake::Want(never.name);
    Check(shapemake::Wants(n.count, n.start), "... and wanted again the next time it's held");
    frames(drawWait + 200);

    // Another model with the gun's counts (the RPK's buffers bound for the wrong gun's draw): left alone, then given up on with a line, for good.
    const Draw& x = wrongOne.draws[0];
    shapemake::Want(wrongOne.name);
    Bound other = Bind(device, ctx, dump.draws[0].ib, dump.draws[0].vb, 0);
    Sleep(settle + 50);
    for (int i = 0; i < 3; ++i)
        shapemake::NoteDraw(ctx, x.count, x.start, 0, rpk.draws[0].bones.data(), rpk.draws[0].bones.size());
    Check(shapemake::Wants(x.count, x.start), "a draw with its counts but another mesh's buffers isn't taken");
    frames(drawWait + 200);
    const std::string said = shapemake::LastLine();
    Check(!shapemake::Wants(x.count, x.start) && said.find("Gun shape: rpk's shape couldn't be made (its draw 24492/0 wasn't the mesh known: 3 seen") == 0 &&
              said.find("as before") != std::string::npos && !Exists(at.guns + L"rpk.bin"),
          "... given up on with one line: %s", said.c_str());
    shapemake::Want(wrongOne.name);
    Check(!shapemake::Wants(x.count, x.start) && !shapemake::g_frameWork.load(), "... and not tried again this launch");
    Let(other);

    // The gun itself.
    Check(!gunshape::Use(r.name), "the AN-94: no shape, Use says none");
    shapemake::Want(r.name);
    shapemake::Want(r.name);
    Bound gun = Bind(device, ctx, dump.draws[0].ib, dump.draws[0].vb, w.vbOffset);
    shapemake::NoteDraw(ctx, w.count, w.start, 0, palette.data(), palette.size());
    Check(shapemake::Wants(w.count, w.start), "its draw isn't taken in the first %lu ms (the gun being raised)", settle);
    Sleep(settle + 50);
    shapemake::NoteDraw(ctx, w.count, w.start, 1, palette.data(), palette.size());
    shapemake::NoteDraw(ctx, w.count, w.start, 0, palette.data(), 24);
    Check(shapemake::Wants(w.count, w.start), "nor with a base vertex, nor with a palette too short for its frame bone");
    shapemake::NoteDraw(ctx, w.count, w.start, 0, palette.data(), palette.size());
    Check(!shapemake::Wants(w.count, w.start), "its draw taken: no draw waited for any more");
    shapemake::NoteDraw(ctx, w.count, w.start, 0, palette.data(), palette.size());  // (a second eye's: nothing)
    Let(gun);  // (the game may let go of its buffers: the mod holds them till the copy)
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
    Check(ready && line.find("Gun shape: an94's shape made on this PC from the game's own mesh in ") == 0, "made and ready after %.1f s: %s",
          static_cast<double>(GetTickCount64() - from) / 1000.0, line.c_str());
    Check(!shapemake::Ready(r.name), "Ready says so once");
    Check(gunshape::Use(r.name) && shapemake::Refused(at.guns + L"an94.bin", r).empty() && !AnyTmp(at.guns) && FolderEmptyOrGone(at.guns + L"make\\"),
          "Use has it now; guns\\an94.bin the gun's grid, no .tmp, guns\\make empty");
    // What came back from the graphics card is the dump itself: the shape made straight from the dump is the same file.
    const std::wstring direct = at.root + L"direct\\";
    CreateDirectoryW(direct.substr(0, direct.size() - 1).c_str(), nullptr);
    const shapemake::Result straight = shapemake::Make(r, dump, direct, at.exe);
    Check(straight.ok && ReadAll(direct + L"an94.bin") == ReadAll(at.guns + L"an94.bin"), "read back from the graphics card, byte for byte the shape made straight from the dump");
    Check(!shapemake::g_frameWork.load() && shapemake::g_wantDraw.load() == 0, "nothing more done a frame, no draw waited for");
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
    bool all = false;
    for (int a = 1; a < argc; ++a) {
        if (!wcscmp(argv[a], L"all"))
            all = true;
        else
            at.dumps = argv[a];
    }
    if (!at.dumps.empty() && at.dumps.back() != L'\\')
        at.dumps += L'\\';
    at.exe = own + L"gunshape.exe";
    wchar_t temp[MAX_PATH];
    GetTempPathW(MAX_PATH, temp);
    at.root = std::wstring(temp) + L"shapemaketest_" + std::to_wstring(GetCurrentProcessId()) + L"\\";
    at.guns = at.root + L"guns\\";
    CreateDirectoryW(at.root.substr(0, at.root.size() - 1).c_str(), nullptr);
    CreateDirectoryW(at.guns.substr(0, at.guns.size() - 1).c_str(), nullptr);

    TestTable(own + L"..\\src\\");
    TestMake(at, all);
    TestCard(at);

    RemoveAll(at.root);
    std::printf("%d of %d checks failed\n", g_failed, g_checked);
    return g_failed;
}

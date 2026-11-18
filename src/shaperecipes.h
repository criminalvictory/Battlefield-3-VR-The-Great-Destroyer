#pragma once

// How each gun's collision shape (guns\<name>.bin, gunshape.h) is made from the game's own mesh: the draws to read back and tools/gunshape.exe's
// arguments. The shapes aren't in the download (they're the game's meshes), so each is made on the player's PC the first time that gun is held
// (shapemake.h). One row a shape, in one place; pure, no game: tools/shapemaketest.cpp checks every row.
//
// Every row was proved on 2026-11-18: made again by build\gunshape.exe from the mesh dumps the first shapes were made from, each came out byte for
// byte the file in build\guns. Not here: "smaw_a" (the launcher's part A alone, of 2026-11-05 03:13: nothing reads it, and from today's dump
// "3414 0 1" gives its grid with 4.7% of its cells up to 3 mm off, its dump having been taken again since).
//
// A draw is known by more than its counts, which other models share (the launcher's part A): its index buffer's and its vertex buffer's sizes and
// where the vertex buffer is bound must be these too, or it isn't read back. cells: the shape's grid as it came out here; one made on a player's PC
// must come out within kCellsApart of it on each axis, or it's some other pose or model and isn't kept.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace shaperecipes {

struct Draw {
    uint32_t count = 0, start = 0;          // its colour pass: the index count and where it starts
    uint32_t ibBytes = 0, vbBytes = 0;      // its buffers' sizes (ByteWidth)
    uint32_t vbOffset = 0;                  // where its vertex buffer is bound from
    int ownBone = 0, firstBone = 0;         // a further draw with a palette of its own: its bone ownBone is the first draw's bone firstBone
};
constexpr int kMostDraws = 2;
struct Recipe {
    const char* name;      // gunshape::Use's: skin.cpp's HeldKind::shape, "m416", "smaw"
    int frameBone;         // the first draw's rigid bone the shape is in the space of
    const char* leaveOut;  // the bones left out, comma separated: what moves or comes off (the magazine, the lever)
    int cells[3];          // the grid it comes out as, 3 mm cells
    Draw draws[kMostDraws];
    int drawCount;
};
constexpr float kCell = 0.003f;      // m
constexpr int kCellsApart = 2;       // cells: 6 mm
constexpr uint32_t kStride = 48;     // the colour passes' vertex layout (tools/meshdump.h kStride)
constexpr uint32_t kIndexFormat = 57;  // DXGI_FORMAT_R16_UINT

constexpr Recipe kRecipes[] = {
    // name      frame  left out      cells              count  start  ib      vb      bound from
    {"m416",     2, "4",         {57, 102, 299},  {{12768, 0, 52320, 278592, 0}}, 1},        // its magazine
    {"rpk",      3, "0,1",       {51, 108, 419},  {{24492, 0, 99360, 490368, 0}}, 1},        // its lever, its magazine
    {"an94",     0, "2,4,5",     {48, 97, 350},   {{16188, 0, 66360, 344976, 0}}, 1},        // its lever, its magazine and the spare parked behind it
    {"aek971",   4, "0,2,3",     {50, 105, 385},  {{21585, 0, 87732, 416736, 0}}, 1},        // its magazine, its lever, the spare
    {"ak74",     2, "1,3",       {49, 95, 347},   {{21723, 0, 91980, 447936, 0}}, 1},        // its lever, its magazine
    {"g3a3",     0, "1,3",       {54, 110, 387},  {{23064, 0, 92256, 437200, 0}}, 1},        // its magazine, its cocking handle
    {"kh2002",   3, "1,2",       {52, 99, 279},   {{15237, 0, 62196, 323952, 0}}, 1},        // its magazine, its hook
    {"m16a4",    4, "0,2",       {56, 99, 389},   {{17013, 312, 74700, 381952, 5040}}, 1},   // its T handle, its magazine
    {"m4a1",     2, "0,3",       {56, 100, 329},  {{22194, 312, 90024, 465168, 5040}}, 1},   // its T handle, its magazine
    {"m240",     3, "2,4",       {79, 112, 442},  {{27588, 2364, 119808, 660528, 56880}}, 1},  // its lever, its box
    {"fim92",    0, "2,3,4,5,6", {106, 135, 540}, {{20298, 354, 81900, 397072, 6096}}, 1},   // the Stinger: its aerial's panels (3 to 6) and its bone 2
    // The SMAW: its part A without its case (bone 1, which comes off), and its front, part B, whose bone 0 is part A's bone 2.
    {"smaw",     0, "1",         {98, 153, 311},  {{3414, 0, 87024, 483536, 0}, {18222, 3414, 87024, 483536, 55584, 0, 2}}, 2},
    // Measured 2026-11-18 for the guns to come (no hold names them yet: never asked for until one does).
    {"m1014",    3, "0",         {50, 101, 387},  {{13377, 0, 56604, 295952, 0}}, 1},
    {"aks74u",   2, "0,3",       {48, 97, 269},   {{17991, 0, 77076, 376416, 0}}, 1},
    {"ump45",    2, "0,1",       {52, 103, 246},  {{19314, 234, 78552, 411152, 3648}}, 1},
};

inline const Recipe* Find(const char* name) {
    if (name)
        for (const Recipe& r : kRecipes)
            if (!std::strcmp(r.name, name))
                return &r;
    return nullptr;
}

// tools/gunshape.exe's arguments after the dump folder, the file and the cell: "<count>[@start] <frame bone> <bones left out>", and for a gun in
// several draws " 0 <count>@<start>:<its bone>:<the first draw's bone>" each ("17013@312 4 0,2"; the SMAW's "3414 0 1 0 18222@3414:0:2").
inline std::string Arguments(const Recipe& r) {
    char text[96];
    std::string out;
    if (r.draws[0].start)
        std::snprintf(text, sizeof(text), "%u@%u %d %s", r.draws[0].count, r.draws[0].start, r.frameBone, r.leaveOut);
    else
        std::snprintf(text, sizeof(text), "%u %d %s", r.draws[0].count, r.frameBone, r.leaveOut);
    out = text;
    if (r.drawCount > 1)
        out += " 0";
    for (int d = 1; d < r.drawCount; ++d) {
        std::snprintf(text, sizeof(text), " %u@%u:%d:%d", r.draws[d].count, r.draws[d].start, r.draws[d].ownBone, r.draws[d].firstBone);
        out += text;
    }
    return out;
}

// A dump's files' names begin with this (tools/meshdump.h LoadDraw): "mesh_17013_312_".
inline std::string FilePrefix(const Draw& d) {
    char text[48];
    std::snprintf(text, sizeof(text), "mesh_%u_%u_", d.count, d.start);
    return text;
}

}

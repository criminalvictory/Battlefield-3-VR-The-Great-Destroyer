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
// The grid can't tell a part moved inside the gun's outline (a charging handle pulled, a dust cover or a lid open), so a draw is only read back
// while every bone the shape keeps is where it is at rest, within 1 mm (Rest, Apart): the gun's own pose in the dump its row was proved from.

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace shaperecipes {

// A bone the shape keeps, as it is on its draw's frame bone in the pose the shape is of (the gun at rest in the hands): where its origin is (m), and
// its turn there when it rests turned (a still part folded over: the M249's carrying handle, which never moves, 0.07 mm in its recordings; the review
// of 2026-11-18: left out, the hand went through it on its way to the charging handle): its axes on the frame bone, rows, as a 3x4's turn. Every other
// kept bone of these guns rests unturned on its frame bone, only moved (turned: false). From the dump each shape was first made from (tools/shapemaketest
// "rest" writes these rows, a turned bone's with its turn).
struct Rest {
    int bone;
    float at[3];
    bool turned = false;
    float turn[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
};
struct Draw {
    uint32_t count = 0, start = 0;          // its colour pass: the index count and where it starts
    uint32_t ibBytes = 0, vbBytes = 0;      // its buffers' sizes (ByteWidth)
    uint32_t vbOffset = 0;                  // where its vertex buffer is bound from
    int ownBone = 0, firstBone = 0;         // a further draw with a palette of its own: its bone ownBone is the first draw's bone firstBone
    int bones = 0;                          // the bones its palette must have: one more than the highest a vertex of it names
    const Rest* rest = nullptr;             // the bones it keeps besides its frame bone (the first draw's: Recipe::frameBone; a further draw's: ownBone)
    int restCount = 0;
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

// Where each gun's kept bones are at rest, on its frame bone (m).
constexpr Rest kRestM416[] = {{0, {0.0f, -0.02811f, -0.02959f}}, {1, {0.0f, 0.04473f, 0.17502f}}, {3, {-0.02204f, -0.08225f, 0.41483f}}};  // its charging handle, its trigger, its dust cover
constexpr Rest kRestRpk[] = {{2, {0.0f, 0.05233f, 0.14981f}}};
constexpr Rest kRestAn94[] = {{1, {0.0f, 0.07922f, 0.12710f}}, {3, {0.0f, 0.03202f, 0.43791f}}};
constexpr Rest kRestAek971[] = {{1, {0.0f, 0.05233f, 0.14981f}}};
constexpr Rest kRestAk74[] = {{0, {0.0f, 0.05233f, 0.14981f}}};
constexpr Rest kRestG3a3[] = {{2, {0.0f, 0.05325f, 0.20315f}}};
constexpr Rest kRestKh2002[] = {{0, {0.0f, 0.04038f, 0.18237f}}};
constexpr Rest kRestM16a4[] = {{1, {0.0f, 0.03464f, 0.20241f}}};
constexpr Rest kRestM4a1[] = {{1, {-0.01759f, -0.09575f, 0.41483f}}};  // its dust cover
constexpr Rest kRestM240[] = {{0, {0.0f, 0.01617f, 0.17290f}}, {1, {0.0f, 0.07121f, 0.22668f}}, {5, {0.0f, -0.07862f, 0.37827f}}};  // its lid: bone 5
constexpr Rest kRestFim92[] = {{1, {0.03650f, -0.07016f, 0.47790f}}};
constexpr Rest kRestSmaw[] = {{2, {0.0f, 0.07340f, -0.49291f}}};  // what its front hangs from
constexpr Rest kRestM1014[] = {{1, {0.0f, 0.12273f, 0.18612f}}, {2, {0.0f, 0.0f, 0.26562f}}};
constexpr Rest kRestAks74u[] = {{1, {0.0f, 0.05580f, 0.13449f}}};
constexpr Rest kRestMk11[] = {{1, {0.0f, 0.03051f, 0.20241f}}, {2, {-0.02086f, -0.09397f, 0.41483f}}};  // its trigger, its port's cover
// The M249's feed cover; its carrying handle, folded over to the right: turned 140.9 degrees about the gun's length on the frame.
constexpr Rest kRestM249[] = {{1, {0.0f, -0.08879f, 0.28432f}},
                               {2, {-0.03730f, 0.02851f, 0.09012f}, true, {-0.77566f, 0.63115f, 0.0f, -0.63115f, -0.77566f, 0.0f, 0.0f, 0.0f, 1.0f}}};

constexpr Recipe kRecipes[] = {
    // name      frame  left out      cells              count  start  ib      vb      bound from, (a further draw's bones), palette bones, at rest
    {"m416",     2, "4",         {57, 102, 299},  {{12768, 0, 52320, 278592, 0, 0, 0, 5, kRestM416, 3}}, 1},        // its magazine
    {"rpk",      3, "0,1",       {51, 108, 419},  {{24492, 0, 99360, 490368, 0, 0, 0, 4, kRestRpk, 1}}, 1},         // its lever, its magazine
    {"an94",     0, "2,4,5",     {48, 97, 350},   {{16188, 0, 66360, 344976, 0, 0, 0, 6, kRestAn94, 2}}, 1},        // its lever, its magazine and the spare parked behind it
    {"aek971",   4, "0,2,3",     {50, 105, 385},  {{21585, 0, 87732, 416736, 0, 0, 0, 5, kRestAek971, 1}}, 1},      // its magazine, its lever, the spare
    {"ak74",     2, "1,3",       {49, 95, 347},   {{21723, 0, 91980, 447936, 0, 0, 0, 4, kRestAk74, 1}}, 1},        // its lever, its magazine
    {"g3a3",     0, "1,3",       {54, 110, 387},  {{23064, 0, 92256, 437200, 0, 0, 0, 4, kRestG3a3, 1}}, 1},        // its magazine, its cocking handle
    {"kh2002",   3, "1,2",       {52, 99, 279},   {{15237, 0, 62196, 323952, 0, 0, 0, 4, kRestKh2002, 1}}, 1},      // its magazine, its hook
    {"m16a4",    4, "0,2",       {56, 99, 389},   {{17013, 312, 74700, 381952, 5040, 0, 0, 5, kRestM16a4, 1}}, 1},  // its T handle, its magazine
    {"m4a1",     2, "0,3",       {56, 100, 329},  {{22194, 312, 90024, 465168, 5040, 0, 0, 4, kRestM4a1, 1}}, 1},   // its T handle, its magazine
    {"m240",     3, "2,4",       {79, 112, 442},  {{27588, 2364, 119808, 660528, 56880, 0, 0, 6, kRestM240, 3}}, 1},  // its lever, its box
    {"fim92",    0, "2,3,4,5,6", {106, 135, 540}, {{20298, 354, 81900, 397072, 6096, 0, 0, 7, kRestFim92, 1}}, 1},  // the Stinger: its aerial's panels (3 to 6) and its bone 2
    // The SMAW: its part A without its case (bone 1, which comes off), and its front, part B, whose bone 0 is part A's bone 2.
    {"smaw",     0, "1",         {98, 153, 311},  {{3414, 0, 87024, 483536, 0, 0, 0, 3, kRestSmaw, 1}, {18222, 3414, 87024, 483536, 55584, 0, 2, 1}}, 2},
    // Measured 2026-11-18 for the guns to come (no hold names them yet: never asked for until one does).
    {"m1014",    3, "0",         {50, 101, 387},  {{13377, 0, 56604, 295952, 0, 0, 0, 4, kRestM1014, 2}}, 1},
    {"aks74u",   2, "0,3",       {48, 97, 269},   {{17991, 0, 77076, 376416, 0, 0, 0, 4, kRestAks74u, 1}}, 1},
    {"ump45",    2, "0,1",       {52, 103, 246},  {{19314, 234, 78552, 411152, 3648, 0, 0, 3}}, 1},
    // The Mk11 Mod 0 (Operation Swordbreaker, 2026-11-18): its body alone, without its magazine. (Its buffer's first draw, 1902/0, isn't in it: the plate
    // on the frame lies inside the stock's outline, and the rest of it is the magazine's sleeve, which goes with the magazine.)
    {"mk11",     0, "3",         {54, 99, 374},   {{12456, 2214, 58680, 313520, 24048, 0, 0, 4, kRestMk11, 2}}, 1},   // its magazine
    // The M249 (Operation Swordbreaker, 2026-11-18): its body without its charging handle. Its box (987/27099) and its belt (2808/0) are draws of their own,
    // so never in it: the hands take the box and the belt. Its feed cover stays in it, shut (it moves only with the hand on it, as the M240's lid). Its
    // carrying handle (bone 2) is in it as it rests, folded over to the right, turned on the frame (never moved in its recordings: 0.07 mm), where a hand
    // going for the charging handle passes (x -0.122 to -0.056, y 0.052 to 0.130, z 0.512 to 0.610): its rest, turn and all, is its row's.
    {"m249",     3, "0",         {94, 127, 376},  {{24291, 2808, 112344, 607200, 72624, 0, 0, 4, kRestM249, 2}}, 1},  // its handle
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

// How off2 a palette is from the pose the shape is of: the furthest any point within kRestReach of a kept bone's origin is from where that bone has
// it at rest (moved by Rest::at and turned by Rest::turn, or not turned), in the frame bone's space (m). A shape is only made from a palette within kRestApart: a charging handle pulled, a cover or a lid
// open, a bolt back, a gun still being raised or another model's bones would be kept in the shape for good, and its grid needn't show it.
// palette: 12 floats a bone, bones of them. worst: the bone furthest off. A palette too short, or not numbers: off2.
constexpr float kRestApart = 0.001f;  // m
constexpr float kRestReach = 0.2f;    // m
constexpr float kFar = 1.0e9f;
inline float Apart(const Draw& d, int frameBone, const void* palette, int bones, int* worst = nullptr) {
    if (worst)
        *worst = -1;
    if (!palette || bones < d.bones || frameBone < 0 || frameBone >= bones)
        return kFar;
    auto bone = [&](int i, float* m) { std::memcpy(m, static_cast<const char*>(palette) + 48 * static_cast<size_t>(i), 48); };
    float frame[12], inv[12];
    bone(frameBone, frame);
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            inv[i * 4 + j] = frame[j * 4 + i];
    for (int i = 0; i < 3; ++i)
        inv[i * 4 + 3] = -(inv[i * 4] * frame[3] + inv[i * 4 + 1] * frame[7] + inv[i * 4 + 2] * frame[11]);
    float most = 0.0f;
    for (int r = 0; r < d.restCount; ++r) {
        const Rest& rest = d.rest[r];
        if (rest.bone < 0 || rest.bone >= bones)
            return kFar;
        float m[12], on[12];
        bone(rest.bone, m);
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 4; ++j)
                on[i * 4 + j] = inv[i * 4] * m[j] + inv[i * 4 + 1] * m[4 + j] + inv[i * 4 + 2] * m[8 + j];
            on[i * 4 + 3] += inv[i * 4 + 3];
        }
        // The bone's origin, and a point kRestReach along each of its axes.
        float off2 = 0.0f;
        for (int p = -1; p < 3; ++p) {
            float sum = 0.0f;
            for (int i = 0; i < 3; ++i) {
                const float axis = p < 0 ? 0.0f : rest.turned ? rest.turn[i * 3 + p] : i == p ? 1.0f : 0.0f;  // (the origin's row reads no axis)
                const float by = on[i * 4 + 3] - rest.at[i] + (p < 0 ? 0.0f : (on[i * 4 + p] - axis) * kRestReach);
                sum += by * by;
            }
            if (!(sum <= off2))  // (not a number: far off)
                off2 = sum == sum ? sum : kFar;
        }
        off2 = off2 >= kFar ? kFar : static_cast<float>(std::sqrt(static_cast<double>(off2)));
        if (off2 > most) {
            most = off2;
            if (worst)
                *worst = rest.bone;
        }
    }
    return most;
}

// A dump's files' names begin with this (tools/meshdump.h LoadDraw): "mesh_17013_312_".
inline std::string FilePrefix(const Draw& d) {
    char text[48];
    std::snprintf(text, sizeof(text), "mesh_%u_%u_", d.count, d.start);
    return text;
}

}

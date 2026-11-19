#pragma once

#include <cstdint>

// A gun's charging handle with no bone of its own in the game's mesh (the Mk11's T handle: a part of its frame, on the frame's bone), moved with the hand
// as every other gun's handle is: drawn apart from the body as runs of the body's own indices in each pass that draws it (its colour pass and its depth
// pass), with the frame's bone pulled as a lever's bone is (skin.cpp's PullLever), as the M16A4's port cover is drawn apart. The game's own index and
// vertex buffers, nothing read back from the card, no buffer of the mod's but its bones: each row's runs found in its gun's mesh dump, and held to it
// offline (tools/handleruntest.cpp). A gun with no row, and a row's gun with its handle home, is drawn as before.
//
// The Mk11's handle is only its T and the stem into the receiver (1.7 cm: the game never moves it, so its mesh has no more of it). Drawn back 8 cm with
// nothing between it and the receiver it would hang in the air, so a shaft is drawn from the pulled stem to where the stem sits home: a box of the stem's
// own cross-section, its four long faces each the stem's own flat top (a run of two triangles) stretched along z, and for the underside and the two sides
// also turned about the stem's middle line. The top's normal is square to z and its tangent along it, so neither stretch nor turn changes their
// directions, and each face is lit as the top is lit. (The stem's own sides would not do: their vertex normals lean 15 degrees along z at their back edge,
// and the stretch would lean them 75.)
namespace handleruns {

struct Run {
    uint32_t start = 0, count = 0;
};
constexpr int kMaxRuns = 4;

// One pass of the gun's body: its own draw (its start and count in the buffer), the handle's runs of that draw's indices (in order, apart), and the stem's
// flat top drawn again as the shaft's faces (a count of 0: no shaft).
struct Pass {
    uint32_t drawStart = 0, drawCount = 0;
    Run runs[kMaxRuns] = {};
    int runCount = 0;
    Run shaftTop = {};
};

struct Handle {
    const char* name;
    uint32_t kind;  // the gun's colour draw's index count (skin.cpp's HeldKind::bodyIndices)
    Pass colour, depth;
    // The stem's flat top on the frame (x left, y up, z to the muzzle): from x right to x left, at y top, from z back to z front (its front inside the
    // receiver with the handle home). The shaft is the box from there down to y bottom (the stem's underside), from the pulled stem's front back home.
    float right = 0.0f, left = 0.0f, top = 0.0f, bottom = 0.0f, back = 0.0f, front = 0.0f;
};

// The Mk11 Mod 0 (Operation Swordbreaker; its mesh dump build\mesh_12456_2214_*, 2026-11-18): its T handle at the back of the receiver, x -0.0196 to
// 0.0198, y 0.0462 to 0.0565, z 0.2690 to 0.2878, all of it on the frame (bone 0) and none of its corners shared with the rest of the body: two parts, the
// T with its stem (218 triangles) and the latch on its left ear (60). In the colour pass (12456 from 2214) they lie together, 834 indices from 5499; in the
// depth pass (14670 from 14670, its own vertices and order) apart, the T 654 from 19068 and the latch 180 from 29004. The stem's last slice, z 0.2798 to
// 0.2856, is a plain box (x -0.0108 to 0.0093, y 0.0489 to 0.0539) with a flat top of two triangles: 6 indices from 6072, and from 19101 in the depth pass.
// (The box's numbers are the mesh's own halves, exactly: the top's corners, the underside's back edge.)
inline constexpr Handle kHandles[] = {
    {"Mk11 Mod 0", 12456,
     {2214, 12456, {{5499, 834}}, 1, {6072, 6}},
     {14670, 14670, {{19068, 654}, {29004, 180}}, 2, {19101, 6}},
     -0.01084136962890625f, 0.00933837890625f, 0.053924560546875f, 0.04888916015625f, 0.27978515625f, 0.28564453125f},
};

// A gun's row by its colour draw's index count; null for any other gun.
constexpr const Handle* For(uint32_t kind) {
    for (const Handle& h : kHandles)
        if (kind && h.kind == kind)
            return &h;
    return nullptr;
}

// The draw (start, count) less the runs cut out of it: the pieces left, in order, into gaps (room for cuts + 1); how many (0 when the runs are all of
// it). The runs in any order, each inside the draw and none overlapping another, else -1 (nothing cut: the draw is drawn whole).
constexpr int Gaps(uint32_t start, uint32_t count, const Run* cut, int cuts, Run* gaps) {
    if (cuts < 0 || cuts > kMaxRuns + 1)
        return -1;
    Run sorted[kMaxRuns + 1];
    for (int i = 0; i < cuts; ++i) {
        Run r = cut[i];
        int j = i;
        for (; j > 0 && sorted[j - 1].start > r.start; --j)
            sorted[j] = sorted[j - 1];
        sorted[j] = r;
    }
    const uint64_t end = static_cast<uint64_t>(start) + count;
    uint64_t at = start;
    int n = 0;
    for (int i = 0; i < cuts; ++i) {
        const uint64_t from = sorted[i].start, to = from + sorted[i].count;
        if (!sorted[i].count || from < at || to > end)
            return -1;
        if (from > at)
            gaps[n++] = {static_cast<uint32_t>(at), static_cast<uint32_t>(from - at)};
        at = to;
    }
    if (at < end)
        gaps[n++] = {static_cast<uint32_t>(at), static_cast<uint32_t>(end - at)};
    return n;
}

// The shaft's faces with the handle pulled `pull` metres back (above 0): face 0 its top, 1 its underside, 2 its right side, 3 its left side, each the
// stem's flat top taken onto that face on the frame (three rows: turn and stretch | place), to go after the frame's bone (its model space is the frame's).
// Along z each runs from the pulled stem's front (front - pull) home to front, the top's back edge to the pulled end; across, each covers its side of the
// box. Each turn keeps the top's winding (its determinant is positive) and takes its normal (0 1 0) to the face's own outward one. False with no pull, or
// no shaft in the row.
constexpr int kShaftFaces = 4;
constexpr bool ShaftFace(const Handle& h, int face, float pull, float out[12]) {
    const float length = h.front - h.back, width = h.left - h.right, height = h.top - h.bottom;
    if (!(pull > 0.0f) || !(length > 0.0f) || !(width > 0.0f) || !(height > 0.0f) || face < 0 || face >= kShaftFaces)
        return false;
    // Along z: z' = front + (z - front) * s, the top's back edge (z back) to front - pull, its front edge staying home.
    const float s = pull / length, zPlace = h.front * (1.0f - s);
    const float k = height / width;  // the top's width onto a side's height
    const float midX = 0.5f * (h.left + h.right), midY = 0.5f * (h.top + h.bottom);
    float m[12] = {};
    switch (face) {
    case 0:  // the top, where it is
        m[0] = 1.0f, m[5] = 1.0f;
        break;
    case 1:  // the underside: half a turn about the stem's middle line
        m[0] = -1.0f, m[3] = 2.0f * midX;
        m[5] = -1.0f, m[7] = 2.0f * midY;
        break;
    case 2:  // the right side (x right): a quarter turn, the top's y onto x right, its width onto the side's height (x right at the bottom)
        m[1] = -1.0f, m[3] = h.right + h.top;
        m[4] = k, m[7] = h.bottom - k * h.right;
        break;
    case 3:  // the left side (x left): the other quarter turn (x right at the top)
        m[1] = 1.0f, m[3] = h.left - h.top;
        m[4] = -k, m[7] = h.top + k * h.right;
        break;
    }
    m[10] = s, m[11] = zPlace;
    for (int i = 0; i < 12; ++i)
        out[i] = m[i];
    return true;
}

}  // namespace handleruns

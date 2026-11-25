#pragma once

// Hosting a gun that lies (the owner, 2026-11-18: "a smart player would holster their rifle before picking up the AT4, but they could momentarily drop
// their rifle on the floor, pick up the AT4, shoot it, discard it, then pick up their rifle again"; 2026-11-19: "Better to physically drop the rifle and
// pick up the AT"). A gun let go of is drawn on the ground by the game's own draws of it, moved by the mod (skin.cpp DrawHeldPart), so only while it's the
// game's weapon: when the roof's AT4 kit took slot 0 the M4A1 lying there was drawn nowhere till the kit gave it back, and the spent AT4 vanished at the
// give-back (the vanilla game leaves no tube). Hosting keeps what a lying gun's draws need while the game still draws it (gunhost.cpp: their states, the
// mesh and constants copied on the graphics card, never read by the processor) and draws them itself once the game doesn't: right after the draws of
// the gun the game draws then, in each eye's own passes, with that pass's own view, so it has both eyes, its depth and the game's own lighting (the
// G-buffer, as the lying gun's own draws have), lying where loose has it on the real ground.
//
// This file is the pure part (tools/hosttest.cpp runs it): a draw's bones on the gun's own frame and placed again, the gun's frame in a draw's space,
// what becomes of a lying gun at a change of gun, how a kept draw's constant buffers are bound again, its depth test, the once-a-pass guard and the
// list of hosted guns hands.cpp keeps. No game calls and no logs.

#include <cmath>
#include <cstdint>
#include <cstring>

#include "loose.h"

namespace hostrules {

constexpr int kBones = 60;                  // the bones a palette's draws use (skin.cpp's kPaletteBones: BF3's blocks hold up to 60)
constexpr unsigned kPaletteBytes = 4096;    // the game's per-draw block (skin.cpp's kGloveUploadSize): bones first, the rest as it was
constexpr unsigned kViewBytes = 352;        // the per-view constants (viewturn.h's kPerViewBytes): each pass's own
constexpr int kMostHosted = 4;              // guns hosted at once (the roof's: the M4A1 while the AT4 is out, then the spent AT4)
constexpr unsigned long long kIdleMs = 120000;  // a kind's kept draws neither hosted nor kept again this long are let go (pickrules::kLyingKeepsMs)

// ---------------------------------------------------------------- 3x4 rows (rotation | translation), as skin.cpp's

inline bool Finite(const float* m, int n) {
    for (int i = 0; i < n; ++i)
        if (!std::isfinite(m[i]))
            return false;
    return true;
}
// out = a b (out may be either).
inline void Compose(const float* a, const float* b, float* out) {
    float o[12];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j)
            o[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
        o[i * 4 + 3] += a[i * 4 + 3];
    }
    std::memcpy(out, o, sizeof(o));
}
// Any affine one (its 3x3 by its cofactors): false for one that can't be (flat, or numbers not finite).
inline bool Invert(const float* m, float* out) {
    const float a = m[0], b = m[1], c = m[2], d = m[4], e = m[5], f = m[6], g = m[8], h = m[9], k = m[10];
    const float A = e * k - f * h, B = f * g - d * k, C = d * h - e * g;
    const float det = a * A + b * B + c * C;
    if (!std::isfinite(det) || std::fabs(det) < 1e-12f)
        return false;
    const float s = 1.0f / det;
    float r[9] = {A * s, (c * h - b * k) * s, (b * f - c * e) * s, B * s, (a * k - c * g) * s, (c * d - a * f) * s, C * s, (b * g - a * h) * s, (a * e - b * d) * s};
    float o[12];
    for (int i = 0; i < 3; ++i) {
        o[i * 4] = r[i * 3], o[i * 4 + 1] = r[i * 3 + 1], o[i * 4 + 2] = r[i * 3 + 2];
        o[i * 4 + 3] = -(r[i * 3] * m[3] + r[i * 3 + 1] * m[7] + r[i * 3 + 2] * m[11]);
    }
    if (!Finite(o, 12))
        return false;
    std::memcpy(out, o, sizeof(o));
    return true;
}
inline bool AllZero(const float* m) {
    for (int i = 0; i < 12; ++i)
        if (m[i] != 0.0f)
            return false;
    return true;
}

// ---------------------------------------------------------------- a draw's bones on the gun's own frame, and placed again

// A palette as a lying gun's draw was drawn (bones first, 3x4 rows each), with the gun's frame where that draw placed it (pose: its model space into the
// draw's space, skin.cpp's HandTargets::launcherPose lying): each bone on the frame (rel = pose^-1 bone). A bone all zeros (a part folded onto one point:
// a hidden spare, the belt gone through the feed) stays all zeros; every other number of the block is kept as it was. False: the pose can't be inverted.
inline bool Rel(const float* pose, const float* palette, int bones, float* rel) {
    float inverse[12];
    if (!Finite(pose, 12) || !Invert(pose, inverse))
        return false;
    for (int k = 0; k < bones; ++k) {
        const float* b = palette + k * 12;
        float* r = rel + k * 12;
        if (AllZero(b))
            std::memset(r, 0, 12 * sizeof(float));
        else
            Compose(inverse, b, r);
    }
    return true;
}
// The other way: each bone of rel on the gun's frame at pose (this draw's space). All zeros stay all zeros.
inline void Place(const float* pose, const float* rel, int bones, float* palette) {
    for (int k = 0; k < bones; ++k) {
        const float* r = rel + k * 12;
        float* b = palette + k * 12;
        if (AllZero(r))
            std::memset(b, 0, 12 * sizeof(float));
        else
            Compose(pose, r, b);
    }
}

// The hosted gun's frame in a draw's space: loose's pose (its frame into the arms' space with origin, whole metres) moved to the draw's own origin
// (seenOrigin), then through that draw's way from the world (drawnFromWorld): skin.cpp ToArms's lying launcher, exactly.
inline void FrameInDraw(const float* drawnFromWorld, const float* bone, const int* origin, const float* seenOrigin, float* out) {
    float lying[12];
    std::memcpy(lying, bone, sizeof(lying));
    for (int i = 0; i < 3; ++i)
        lying[i * 4 + 3] += static_cast<float>(origin[i]) - seenOrigin[i];
    Compose(drawnFromWorld, lying, out);
}

// ---------------------------------------------------------------- what becomes of a lying gun at a change of gun

// hands.cpp asks at each place a change of gun took the lying gun's copy off the ground (its game gun gone with the game's switch):
//   kAsBefore  removed, as before hosting (the game draws the gun again when it's back: another slot's gun, a pickup's, or hosting off or not ready);
//   kKeep      flow (b): a scene's gift took its slot (pickrules::LyingTaken): it stays where it lies, drawn by the mod, till the game gives it back
//              (pickrules::LyingBack lays it down there as the game's again) or that ends (a take, a pick, another level): a gun, it lies till then;
//   kJunk      its gun is in none of the player's slots any more (the kit took the spent AT4 back): it stays where it lies, drawn by the mod, and goes as a
//              magazine does (loose's 30 s from its rest, then only unseen: the owner's "I don't want to flood the map"); nobody can take it (no game gun).
enum class Fate : int { kAsBefore = 0, kKeep = 1, kJunk = 2 };
struct FateIn {
    bool on = false;          // weapon.txt "lyinghost" (on unless "lyinghost 0")
    bool lies = false;        // the game's weapon before lay on the ground (its copy in loose)
    bool retired = false;     // ... a pickup's swap took it: it's in that pickup, the game's to draw
    bool ready = false;       // its draws kept (gunhost::Ready: a colour draw of its kind, kept while it lay)
    bool taken = false;       // flow (b): a scene's gift took its slot
    int slotsRead = 0;        // the player's slots read with a gun in them now (0: none read: not known)
    bool inSlots = false;     // its kind in one of them
};
inline Fate AtChange(const FateIn& in) {
    if (!in.on || !in.lies || in.retired || !in.ready)
        return Fate::kAsBefore;
    if (in.taken)
        return Fate::kKeep;
    if (in.slotsRead > 0 && !in.inSlots)
        return Fate::kJunk;
    return Fate::kAsBefore;
}
inline const char* FateName(Fate f) {
    return f == Fate::kKeep ? "kept lying till the game gives it back" : f == Fate::kJunk ? "lying as junk (30 s, then gone unseen)" : "as before";
}

// ---------------------------------------------------------------- a kept draw's constant buffers bound again

// kView: the pass's own (the 352-byte per-view constants, or a buffer the first-person views went into: skin::ViewBufferKind), each eye's own;
// kPalette: the mod's (the slots the game's per-draw block was bound in for that draw: the bones placed again); kOriginal: an immutable one, the
// game's own (it can't change); kCopy: a copy made on the graphics card as it was at that draw (a dynamic one is the next draw's a moment later).
enum class Cb : uint8_t { kNone = 0, kView = 1, kPalette = 2, kOriginal = 3, kCopy = 4 };
inline Cb RuleOf(bool bound, unsigned size, bool viewBuffer, bool palette, bool immutable) {
    if (!bound)
        return Cb::kNone;
    if (palette)
        return Cb::kPalette;
    if (viewBuffer || size == kViewBytes)
        return Cb::kView;
    return immutable ? Cb::kOriginal : Cb::kCopy;
}
inline const char* CbName(Cb c) {
    return c == Cb::kView ? "view" : c == Cb::kPalette ? "palette" : c == Cb::kOriginal ? "fixed" : c == Cb::kCopy ? "copy" : "none";
}

// ---------------------------------------------------------------- the depth test of a kept colour draw

// D3D11_COMPARISON_FUNC's numbers.
constexpr int kNever = 1, kLess = 2, kEqual = 3, kLessEqual = 4, kGreater = 5, kNotEqual = 6, kGreaterEqual = 7, kAlways = 8;
struct Depth {
    bool enable = true;
    bool writes = true;
    int func = kLess;
};
// A colour draw that tests EQUAL against its own depth draw (a depth pass drew it first): hosted, its bones are placed again in each pass from what was
// kept, and a last bit off between the two would leave its colour out at that pixel. So it's drawn LESS_EQUAL (GREATER_EQUAL with a reversed depth, as
// the kept depth draws test: convention) and writes its depth: it never passes behind anything nearer, it covers its own depth draw's pixels, and it
// stands alone if that draw wasn't hosted in that pass. exact (weapon.txt "lyinghostexact 1"): as the game drew it. Any other test: as it was.
inline Depth Relaxed(const Depth& colour, bool exact, int convention) {
    if (exact || !colour.enable || colour.func != kEqual)
        return colour;
    Depth d = colour;
    d.writes = true;
    d.func = convention == kGreater || convention == kGreaterEqual ? kGreaterEqual : kLessEqual;
    return d;
}

// ---------------------------------------------------------------- a pass, and once in it

// The kind of pass a draw is in: its targets from slot 0 (their formats), its depth target (format, size, samples), whether it writes colour (a depth
// pass's draws write none, with or without targets bound). A kept draw is drawn only in a pass like its own.
constexpr int kTargets = 8;
struct Pass {
    unsigned targets = 0;
    unsigned format[kTargets] = {};
    unsigned depthFormat = 0, width = 0, height = 0, samples = 0;
    bool depth = false;
    bool colour = false;
};
inline bool SamePass(const Pass& a, const Pass& b) {
    if (a.targets != b.targets || a.depth != b.depth || a.colour != b.colour || a.depthFormat != b.depthFormat || a.width != b.width ||
        a.height != b.height || a.samples != b.samples)
        return false;
    for (unsigned i = 0; i < a.targets && i < static_cast<unsigned>(kTargets); ++i)
        if (a.format[i] != b.format[i])
            return false;
    return true;
}
inline uint32_t PassHash(const Pass& p) {
    uint32_t h = 2166136261u;
    auto mix = [&h](uint32_t v) {
        for (int i = 0; i < 4; ++i)
            h = (h ^ ((v >> (i * 8)) & 0xFFu)) * 16777619u;
    };
    mix(p.targets), mix(p.depthFormat), mix(p.width), mix(p.height), mix(p.samples), mix(p.depth ? 1u : 0u), mix(p.colour ? 1u : 0u);
    for (unsigned i = 0; i < p.targets && i < static_cast<unsigned>(kTargets); ++i)
        mix(p.format[i]);
    return h ? h : 1u;
}

// Hosting runs after a draw of the gun the game draws (its body's colour draw, or its depth pass): once in each pass of each view (the frame, the view's
// constant buffer, the pass), though the game drew that gun twice there.
struct PassKey {
    uint32_t frame = 0;
    uintptr_t view = 0;
    uint32_t pass = 0;
};
struct Once {
    static constexpr int kKept = 16;
    PassKey seen[kKept] = {};
    int next = 0;
    // True the first time a pass is met (and it's noted); false again within the same frame, view and pass.
    bool First(const PassKey& k) {
        for (const PassKey& s : seen)
            if (s.pass && s.frame == k.frame && s.view == k.view && s.pass == k.pass)
                return false;
        seen[next] = k;
        next = (next + 1) % kKept;
        return true;
    }
};

// ---------------------------------------------------------------- the hosted guns (hands.cpp's list)

struct Hosted {
    int looseId = 0;                 // its body in loose (stepped there: it settles and lies on the real ground)
    uint32_t kind = 0;               // its draws' kind: the gun's profile id (a held gun's colour draw's index count)
    bool keep = false;               // kKeep (a gun, till the game gives it back) or kJunk (loose's 30 s, then gone unseen)
    const char* name = "gun";
    int sound = 0;                   // the caller's: the kind of its landing sound (hands.cpp's gunprofile::Sound)
    float bone[12] = {};             // where loose has it: its frame into the arms' space with origin (whole metres)
    int origin[3] = {};
    bool have = false;               // a pose from loose (from its first Step on)
    unsigned long long since = 0;    // ms, when it was hosted
};
struct List {
    Hosted h[kMostHosted];
    int n = 0;
};
inline int Find(const List& l, int looseId) {
    for (int i = 0; i < l.n; ++i)
        if (looseId && l.h[i].looseId == looseId)
            return i;
    return -1;
}
// False: no room, no body, or that body hosted already.
inline bool Add(List& l, const Hosted& h) {
    if (!h.looseId || !h.kind || l.n >= kMostHosted || Find(l, h.looseId) >= 0)
        return false;
    l.h[l.n++] = h;
    return true;
}
inline bool Drop(List& l, int looseId) {
    const int i = Find(l, looseId);
    if (i < 0)
        return false;
    for (int k = i; k + 1 < l.n; ++k)
        l.h[k] = l.h[k + 1];
    l.h[--l.n] = Hosted{};
    return true;
}
// After loose's Step (origin: that Step's Frame::origin): each one's pose from it; one told gone, or not among its poses (no ground under it for 5 s,
// broken numbers, a junk one's 30 s and out of view, a Clear: a new life), is dropped. The ids dropped go to dropped[] (kMostHosted of room); how many.
inline int Refresh(List& l, const loose::Result& r, const int* origin, int* dropped) {
    int count = 0;
    for (int i = 0; i < l.n;) {
        Hosted& h = l.h[i];
        bool found = false, gone = false;
        for (int k = 0; k < r.poses && !found; ++k)
            if (r.pose[k].id == h.looseId && Finite(r.pose[k].bone, 12)) {
                std::memcpy(h.bone, r.pose[k].bone, sizeof(h.bone));
                std::memcpy(h.origin, origin, sizeof(h.origin));
                h.have = found = true;
            }
        for (int k = 0; k < r.gone; ++k)
            gone = gone || r.goneId[k] == h.looseId;
        if (found && !gone) {
            ++i;
            continue;
        }
        if (dropped && count < kMostHosted)
            dropped[count] = h.looseId;
        ++count;
        Drop(l, h.looseId);
    }
    return count;
}

// ---------------------------------------------------------------- flow (b) after a death on its level

// The "RPG" checkpoint after a miss loads the level again with the AT4 in slot 0, and loose is cleared with the new life: the rifle flow (b)'s record
// keeps (it lies where it lay when the scene gives it back) is put down there again, hosted, so it's on the floor through the next try too. Only with
// its draws kept, the record on, no hosted body for it, and its gun not the one the game draws (then the game's own lies there: LyingBack).
struct RehostIn {
    bool on = false;            // weapon.txt "lyinghost"
    bool record = false;        // flow (b)'s record (hands.cpp's LyingGone)
    bool hosted = false;        // ... a hosted body for it now
    bool ready = false;         // its kind's draws kept
    bool body = false;          // its loose body known (gunbodies)
    uint32_t kind = 0, drawn = 0;  // its kind, the gun in the game's hands now (0 none known)
    bool placed = false;        // the arms' space known this frame (loose's Frame)
};
inline bool Rehost(const RehostIn& in) {
    return in.on && in.record && !in.hosted && in.ready && in.body && in.placed && in.kind != 0 && in.kind != in.drawn;
}

}  // namespace hostrules
